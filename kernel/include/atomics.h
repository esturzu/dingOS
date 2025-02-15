#ifndef ATOMICS_H
#define ATOMICS_H


/**
 * @brief Atomic wrapper for GCC builtins
 * 
 * 
 * modification reference: https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
 * @tparam T type of the object to be atomically modified
 */
template <typename T>
class Atomic {
private:
  volatile T* obj;
public:
  /**
    * @brief Construct a new Atomic object
    * 
    * @param ptr   ptr to the object to be atomically modified
    */
  explicit Atomic(T* ptr) : obj(ptr) {}

  /**
    * @brief atomically loads obj
    * 
    * @return T obj
    */
  T load() const {
    return __atomic_load_n(obj, __ATOMIC_SEQ_CST);
  }

  /**
    * @brief atomically stores desired
    * 
    * @param desired desired value to be stored
    */
  void store(T desired) {
    __atomic_store_n(obj, desired, __ATOMIC_SEQ_CST);
  }

  /**
    * @brief atomically exchanges with desired
    * 
    * @param desired   desired value to be stored
    * @return T        previously stored value
    */
  T exchange(T desired) {
    return __atomic_exchange_n(obj, desired, __ATOMIC_SEQ_CST);
  }

  /**
    * @brief if obj == expected, store desired in obj. otherwise, store desired in expected
    * 
    * @param expected      expected value in atomic
    * @param desired       desired value to be stored
    * @return true         atomic obj is modified
    * @return false        atomic obj is not modified
    */
  bool compare_exchange(T* expected, T desired) {
    return __atomic_compare_exchange_n(
      obj,
      expected,
      desired,
      false,
      __ATOMIC_SEQ_CST,
      __ATOMIC_SEQ_CST
    );
  }
};

class SpinLock
{
  bool status = true;

public:
  
  SpinLock() {};

  void lock()
  {
    while (!__atomic_exchange_n (&status, false, __ATOMIC_SEQ_CST)) {};
  }

  void unlock()
  {
    __atomic_store_n(&status, true, __ATOMIC_SEQ_CST);
  }
};

template <typename T>
class LockGuard
{
  T* lock;

public:
  LockGuard(T &lk) : lock(&lk)
  {
    lock->lock();
  }

  ~LockGuard()
  {
    lock->unlock();
  }
};

#endif // ATOMICS_H