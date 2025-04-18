.global _start

.text
_start:
    mov x0, 97
    svc 2
    mov x0, 10
    svc 2
    svc 0
