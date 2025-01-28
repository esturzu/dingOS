.global atomic_load
atomic_load:
    ldxr x1, [x0]
    dmb sy
    mov x0, x1
    ret

.global atomic_store
atomic_store:
    mov w2, #1 // setup that it is unsuccessful
atomic_store_retry:
    ldxr x2, [x0] // Establish exclusive monitor (value in x2 is unused)
    stxr w2, x1, [x0] // try to store 
    cbnz w2, atomic_store_retry // did we successfully store it?
    dmb sy
    ret

.global atomic_exchange
atomic_exchange:
    mov w2, #1 // setup that it is unsuccessful
atomic_exchange_retry:
    ldxr x3, [x0] // Establish exclusive monitor 
    stxr w2, x1, [x0] // try to store 
    cbnz w2, atomic_store_retry // did we successfully store it?
    dmb sy
    mov x0, x3 // set it up to return old value
    ret

.global atomic_compare_exchange
atomic_compare_exchange:
    // x0: long* obj
    // x1: long* old
    // x2: long new

    ldxr x3, [x0] // load the x0
    ldr x4, [x1] // load the x0
    cmp x3, x4 // current vs old
    b.ne atomic_compare_exchange_fail // if not equal jump
    stxr w4, x2, [x0] // store new into obj
    cbnz w4, atomic_compare_exchange // retry
    dmb sy
    mov w0, #1 // return success
    ret
atomic_compare_exchange_fail:
    str x3, [x1] // store  
    dmb sy
    mov w0, #0 // return failure
    ret