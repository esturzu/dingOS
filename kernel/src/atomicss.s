.global atomic_load
atomic_load:                    // long atomic_load(long* obj)
    ldxr x1, [x0]               // Atomically load the obj
    dmb sy                      // Sets up a Data Memory Barrier
    mov x0, x1                  // Setting the obj to return 
    ret

.global atomic_store
atomic_store:                   // void atomic_store(long* obj, long desired)
    mov w2, #1                  // Initally set as failing
atomic_store_retry:
    ldxr x2, [x0]               // Establish exclusive monitor (value in x2 is unused)
    stxr w2, x1, [x0]           // Atomically store desired into obj
    cbnz w2, atomic_store_retry // On failure, try again
    dmb sy                      // Sets up a Data Memory Barrier
    ret

.global atomic_exchange
atomic_exchange:                    // long atomic_exchange(long* obj, long desired)
    mov w2, #1                      // Initally set as failing
atomic_exchange_retry:
    ldxr x3, [x0]                   // Establish exclusive monitor 
    stxr w2, x1, [x0]               // Atomically store desired into obj
    cbnz w2, atomic_store_retry     // On failure, try again
    dmb sy                          // Sets up a Data Memory Barrier
    mov x0, x3                      // Setting the old obj to return
    ret

.global atomic_compare_exchange
// int atomic_compare_exchange(long* obj, long* expected, long desired)
atomic_compare_exchange:
    ldxr x3, [x0]                           // Atomically load the obj
    ldr x4, [x1]                            // Norm load the expected
    cmp x3, x4                              // Compare obj and expected, setting ne condition
    b.ne atomic_compare_exchange_fail       // If not equal, jump
    atomic_compare_exchange_retry:
    stxr w4, x2, [x0]                       // Atomically store the desired into obj
    cbnz w4, atomic_compare_exchange_retry  // On failure, try again
    dmb sy                                  // Sets up a Data Memory Barrier
    mov w0, #1                              // return modify obj success
    ret
atomic_compare_exchange_fail:
    str x3, [x1]                            // Norm Store the obj value into expected
    dmb sy                                  // Sets up a Data Memory Barrier
    mov w0, #0                              // return modify obj failure
    ret