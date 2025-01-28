#include "atomics.h"

#include "debug.h"


void basic_test_atomics(){
    char* out = "                                    \n";
    long x = 'a';
    out[0] = atomic_load(&x);
    debug_print(out); //'a'

    atomic_store(&x, 'b');
    out[0] = x;
    debug_print(out); // 'b'

    out[0] = atomic_exchange(&x, 'c');
    out[1] = atomic_load(&x);
    debug_print(out); // 'bc'
    out[1] = ' ';

    long y = 'd';
    out[0] = atomic_compare_exchange(&x, &y, 'd') == 1 ? '1' : '0';
    out[1] = atomic_load(&x);
    out[2] = atomic_load(&y);
    debug_print(out); // '0cc'


    out[0] = atomic_compare_exchange(&x, &y, 'd') == 1 ? '1' : '0';
    out[1] = atomic_load(&x);
    out[2] = atomic_load(&y);
    debug_print(out); // '1dc'
}