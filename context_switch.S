.syntax unified

.global syscall
syscall:
    push {lr}
    svc 12
    pop {lr}
    bx lr