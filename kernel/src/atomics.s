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
    stxr w2, x1, [x0]
    cbnz w2, atomic_store_retry
    dmb sy
    ret
