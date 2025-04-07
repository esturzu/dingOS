#ifndef HASHMAP_H
#define HASHMAP_H

#include "atomics.h"
#include "stdint.h"

// Reference: https://stackoverflow.com/a/12996028
struct DefaultIntegerHash {
  uint64_t operator()(uint64_t x) const {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    x = x ^ (x >> 31);
    return x;
  }
};

/*
 * ConcurrentHashMap
 *  - K:    key type
 *  - V:    value type
 *  - Hash: hash functor type
 *
 * Internally uses:
 *  - Bucket array
 *  - Singly linked list per bucket
 *  - RWLock per bucket to allow multiple readers or single writer, per bucket
 *  - Global RWLock for resizing
 */
template <typename K, typename V, typename Hash = DefaultIntegerHash>
class HashMap {
 public:
  /**
   * @brief Construct a new HashMap object.
   *
   * @param num_buckets initial number of buckets
   * @param max_load_factor the load factor that determines when to resize
   * @param hash hash functor to use
   */
  explicit HashMap(int num_buckets = 16, double max_load_factor = 0.75,
                   Hash const& hash = Hash());

  ~HashMap();

  /**
   * @brief Insert or assign a key-value pair.
   *
   * @return true if the key-value pair was inserted, false otherwise
   */
  bool insert_or_assign(K const& key, V const& value);

  /**
   * @brief Find a key-value pair. If found, stores the value in value.
   * Otherwise, value is unchanged.
   *
   * @return true if the key-value pair was found, false otherwise
   */
  bool find(K const& key, V& value) const;

  /**
   * @brief Erase a key-value pair if it exists.
   *
   * @return true if the key-value pair was erased, false otherwise
   */
  bool erase(K const& key);

  /**
   * @brief Returns the number of key-value pairs.
   */
  size_t size() const;

 private:
  void resize();

  // Singly linked list node
  struct Node {
    K key;
    V value;
    uint64_t hash_value;
    Node* next;

    Node(K const& key, V const& value, uint64_t hash_value, Node* next)
        : key(key), value(value), hash_value(hash_value), next(next) {}
  };

  // Bucket with RWLock and list head
  struct Bucket {
    // mutable is not strictly necessary here since there's no const member
    // functions in Bucket, but it's good practice
    RWLock mutable lock;
    Node* head;

    Bucket() : head(nullptr) {}

    ~Bucket() {
      Node* cur = head;
      while (cur) {
        Node* next = cur->next;
        delete cur;
        cur = next;
      }
    }
  };

  /**
   * @brief Minimum number of buckets HashMap can have.
   *
   * HashMap will not resize to have fewer than this number of buckets, ensuring
   * that no excessive resizing occurs.
   *
   * @note HashMap can start with fewer number of buckets if desired, but once
   * it reaches this minimum, it will not shrink below this value.
   */
  static size_t constexpr MIN_NUM_BUCKETS = 16;

  Bucket* buckets;               // Dynamic array of buckets
  size_t num_buckets;            // Number of buckets
  Atomic<size_t> num_entries;    // Number of key-value pairs
  double const max_load_factor;  // Max load factor
  Hash hash;                     // Hash functor
  RWLock mutable global_lock;    // Global lock for resizing
};

////////////////////
// Implementation //
////////////////////
template <typename K, typename V, typename Hash>
HashMap<K, V, Hash>::HashMap(int num_buckets, double max_load_factor,
                             Hash const& hash)
    : num_buckets(num_buckets),
      num_entries(0),
      max_load_factor(max_load_factor),
      hash(hash) {
  buckets = new Bucket[num_buckets];
}

template <typename K, typename V, typename Hash>
HashMap<K, V, Hash>::~HashMap() {
  delete[] buckets;
}

template <typename K, typename V, typename Hash>
void HashMap<K, V, Hash>::resize() {
  // Enter global exclusive section
  global_lock.write_lock();

  size_t new_num_buckets;
  double const load_factor =
      static_cast<double>(num_entries.load()) / num_buckets;

  // Expand or shrink as necessary
  if (load_factor > max_load_factor) {
    new_num_buckets = num_buckets * 2;
  } else if (load_factor < max_load_factor / 4) {
    new_num_buckets = num_buckets / 2;

    // Check if we shrink below MIN_NUM_BUCKETS
    if (new_num_buckets < MIN_NUM_BUCKETS) {
      // Exit global exclusive section
      global_lock.write_unlock();
      return;
    }
  } else {
    // Exit global exclusive section
    global_lock.write_unlock();
    return;
  }

  // Resize
  Bucket* old_buckets = buckets;
  size_t const old_num_buckets = num_buckets;
  num_buckets = new_num_buckets;
  buckets = new Bucket[num_buckets];

  // Reinsert all entries
  for (size_t i = 0; i < old_num_buckets; ++i) {
    Node* cur = old_buckets[i].head;
    while (cur) {
      Node* next = cur->next;

      // Reinsert entry
      size_t const index = cur->hash_value % num_buckets;
      cur->next = buckets[index].head;
      buckets[index].head = cur;

      cur = next;
    }

    // Set old_buckets[i] to own no nodes so those nodes aren't freed by
    // old_buckets' destructor (the new buckets now own those nodes)
    old_buckets[i].head = nullptr;
  }

  // Exit global exclusive section
  global_lock.write_unlock();

  delete[] old_buckets;
}

template <typename K, typename V, typename Hash>
bool HashMap<K, V, Hash>::insert_or_assign(K const& key, V const& value) {
  // Enter global shared section
  global_lock.read_lock();

  uint64_t const hash_value = hash(key);
  size_t const index = hash_value % num_buckets;
  Bucket& bucket = buckets[index];

  // Enter bucket exclusive section
  bucket.lock.write_lock();

  Node* cur = bucket.head;
  while (cur) {
    // Short-circuit if hashes don't match
    if (cur->hash_value == hash_value && cur->key == key) {
      // Key already exists, assign value
      cur->value = value;
      bucket.lock.write_unlock();
      global_lock.read_unlock();
      return false;
    }

    cur = cur->next;
  }

  // Key doesn't exist, insert
  Node* new_head = new Node(key, value, hash_value, bucket.head);
  bucket.head = new_head;

  // Exit bucket exclusive section
  bucket.lock.write_unlock();

  // Atomically increment num_entries
  // Done before exiting global shared section, to ensure resize() always sees
  // the latest num_entries value
  num_entries.add_fetch(1);

  // Exit global shared section
  global_lock.read_unlock();

  // Resize if load factor exceeds max_load_factor
  if (static_cast<double>(num_entries.load()) / num_buckets > max_load_factor) {
    resize();
  }

  return true;
}

template <typename K, typename V, typename Hash>
bool HashMap<K, V, Hash>::find(K const& key, V& value) const {
  // Enter global shared section
  global_lock.read_lock();

  uint64_t const hash_value = hash(key);
  size_t const index = hash_value % num_buckets;
  Bucket& bucket = buckets[index];

  // Enter bucket shared section
  bucket.lock.read_lock();

  Node const* cur = bucket.head;
  while (cur) {
    // Short-circuit if hashes don't match
    if (cur->hash_value == hash_value && cur->key == key) {
      // Found key, store value into user's value parameter
      value = cur->value;
      bucket.lock.read_unlock();
      global_lock.read_unlock();
      return true;
    }

    cur = cur->next;
  }

  // Exit bucket and global shared sections
  bucket.lock.read_unlock();
  global_lock.read_unlock();
  return false;
}

template <typename K, typename V, typename Hash>
bool HashMap<K, V, Hash>::erase(K const& key) {
  // Enter global shared section
  global_lock.read_lock();

  uint64_t const hash_value = hash(key);
  size_t const index = hash_value % num_buckets;
  Bucket& bucket = buckets[index];

  // Enter bucket exclusive section
  bucket.lock.write_lock();

  Node* cur = bucket.head;
  Node* prev = nullptr;
  while (cur) {
    // Short-circuit if hashes don't match
    if (cur->hash_value == hash_value && cur->key == key) {
      // Found key, remove
      if (prev) {
        prev->next = cur->next;
      } else {
        bucket.head = cur->next;
      }

      // Exit bucket exclusive section
      bucket.lock.write_unlock();

      // Remove key-value pair
      delete cur;
      num_entries.add_fetch(-1);

      // Exit global shared section
      global_lock.read_unlock();

      // Resize if load factor falls below max_load_factor / 4
      if (static_cast<double>(num_entries.load()) / num_buckets <
          max_load_factor / 4) {
        resize();
      }

      return true;
    }

    prev = cur;
    cur = cur->next;
  }

  // Exit bucket exclusive and global shared sections
  bucket.lock.write_unlock();
  global_lock.read_unlock();

  return false;
}

template <typename K, typename V, typename Hash>
size_t HashMap<K, V, Hash>::size() const {
  return num_entries.load();
}

#endif  // HASHMAP_H
