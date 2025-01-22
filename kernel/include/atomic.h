// Citations
// https://gcc.gnu.org/onlinedocs/gcc-4.9.4/gcc/_005f_005fatomic-Builtins.html

#ifndef ATOMIC_H
#define ATOMIC_H

template <typename T>
class Atomic
{
  volatile T value;

public:

  Atomic() {}

  ~Atomic() {}

  T load()
  {
    return __atomic_load_n(&value, __ATOMIC_RELAXED);
  }

  void store(T val)
  {
    __atomic_store_n(&value, val, __ATOMIC_RELAXED);
  }

  T exchange(T val)
  {
    return __atomic_exchange_n(&value, val, __ATOMIC_RELAXED);
  }

  bool compare_exchange(T expected, T desired)
  {
    return __atomic_compare_exchange_n(&value, &expected, desired, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
  }
};

#endif