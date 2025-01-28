#ifndef ATOMICS_H
#define ATOMICS_H

extern long atomic_load(long* obj);
extern void atomic_store(long* obj, long desired);
extern long atomic_exchange(long* obj, long desired);
// actually returns a 1 or 0 depending on result but for some reason bool wouldnt work for me
extern int atomic_compare_exchange(long* obj, long* expected, long desired);

extern void basic_test_atomics();

#endif