#include "hashmap.h"

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
