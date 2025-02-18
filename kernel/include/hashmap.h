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

#endif  // HASHMAP_H
