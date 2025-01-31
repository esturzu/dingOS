#include "atomics.h"

#include "debug.h"

void print_test_results(long x, long y, const char* test_title){
    char out[] = " ";
    debug_print(test_title);
    
    if(x == y){
        debug_print(": Success!\n");
    }else{
        debug_print(": Failure! Expected ");
        out[0] = x;
        debug_print(out);
        debug_print(", got ");
        out[0] = y;
        debug_print(out);
        debug_print("\n");
    }
}


void basic_test_atomics(){
    long x = 'a';

    print_test_results(atomic_load(&x), 'a', "atomic load with no mods");

    atomic_store(&x, 'b');
    print_test_results(atomic_load(&x), 'b', "atomic store and load");

    
    print_test_results(atomic_exchange(&x, 'c'), 'b', "atomic exchange");
    print_test_results(atomic_load(&x), 'c', "atomic load after exchange");

    long y = 'd';
    print_test_results(atomic_compare_exchange(&x, &y, 'd'), 0, "atomic compare exchange failure");
    print_test_results(atomic_load(&x), 'c', "atomic load after failed compare exchange");
    print_test_results(atomic_load(&y), 'c', "atomic load after failed compare exchange");

    
    print_test_results(atomic_compare_exchange(&x, &y, 'd'), 1, "atomic compare exchange success");
    print_test_results(atomic_load(&x), 'd', "atomic load after successful compare exchange");
    print_test_results(atomic_load(&y), 'c', "atomic load after successful compare exchange");
}