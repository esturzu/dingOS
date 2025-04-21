#ifndef ATOMICS_H
#define ATOMICS_H

/**
 * @brief Atomic wrapper for GCC builtins
 *
 *
 * modification reference:
 * https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
 * @tparam T type of the object to be atomically modified
 */
template <typename T>
class Atomic {
 private:
  volatile T obj;

 public:
  Atomic() : obj(T()) {}
  /**
   * @brief Construct a new Atomic object
   *
   * @param obj   initial value
   */
  explicit Atomic(T obj) : obj(obj) {}

  /**
   * @brief atomically loads obj
   *
   * @return T obj
   */
  T load() const { return __atomic_load_n(&obj, __ATOMIC_SEQ_CST); }

  /**
   * @brief atomically stores desired
   *
   * @param desired desired value to be stored
   */
  void store(T desired) { __atomic_store_n(&obj, desired, __ATOMIC_SEQ_CST); }

  /**
   * @brief atomically exchanges with desired
   *
   * @param desired   desired value to be stored
   * @return T        previously stored value
   */
  T exchange(T desired) {
    return __atomic_exchange_n(&obj, desired, __ATOMIC_SEQ_CST);
  }

  /**
   * @brief if obj == expected, store desired in obj. otherwise, store desired
   * in expected
   *
   * @param expected      expected value in atomic
   * @param desired       desired value to be stored
   * @return true         atomic obj is modified
   * @return false        atomic obj is not modified
   */
  bool compare_exchange_weak(T* expected, T desired) {
    return __atomic_compare_exchange_n(&obj, expected, desired, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  }
  /**
   * @brief if obj == expected, store desired in obj. otherwise, store desired
   * in expected
   *
   * @param expected      expected value in atomic
   * @param desired       desired value to be stored
   * @return true         atomic obj is modified
   * @return false        atomic obj is not modified
   */
  bool compare_exchange_strong(T* expected, T desired) {
    return __atomic_compare_exchange_n(&obj, expected, desired, true,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  }

  /**
   * @brief atomically adds val to obj
   *
   * @param val   value to be added
   * @return T    new value
   */
  T add_fetch(T val) { return __atomic_add_fetch(&obj, val, __ATOMIC_SEQ_CST); }
};

class SpinLock {
  bool status = true;

 public:
  SpinLock() {}

  void lock() {
    while (!__atomic_exchange_n(&status, false, __ATOMIC_SEQ_CST)) {
    };
  }

  void unlock() { __atomic_store_n(&status, true, __ATOMIC_SEQ_CST); }
};

template <typename T>
class LockGuard {
  T* lock;

public:
  LockGuard(T &lk) : lock(&lk)
  {
    lock->lock();
  }

  ~LockGuard() { lock->unlock(); }
};

// AKA shared-exclusive lock
class RWLock {
 public:
  RWLock() : state(0) {}

  /**
   * @brief Acquires read lock, which is shared among multiple readers.
   *
   * @note This is a blocking call. If the lock is not acquired, the caller will
   * spin until success.
   */
  void read_lock() {
    while (true) {
      int current_state = state.load();
      if (current_state == -1) {
        // Exclusive writer currently holds lock
        continue;
      }

      if (state.compare_exchange_weak(&current_state, current_state + 1)) {
        // Read lock acquired
        break;
      }

      // Read lock not acquired, try again
    }
  }

  /**
   * @brief Releases read lock.
   *
   * Decrements the count of active read locks. If the count reaches zero, the
   * lock is released.
   */
  void read_unlock() { state.add_fetch(-1); }

  /**
   * @brief Acquires exclusive write lock.
   *
   * @note This is a blocking call. If the lock is not acquired, the caller will
   * spin until success.
   */
  void write_lock() {
    while (true) {
      int expected = 0;
      if (state.compare_exchange_weak(&expected, -1)) {
        // Write lock acquired
        break;
      }

      // Write lock not acquired, try again
    }
  }

  /**
   * @brief Releases exclusive write lock.
   *
   * Sets the state to 0, indicating that no one holds the lock.
   */
  void write_unlock() { state.store(0); }

 private:
  /*
   * Lock state:
   * state >= 0: number of readers
   * state == -1: exclusive writer
   */
  Atomic<int> state;
};

#endif  // ATOMICS_H
