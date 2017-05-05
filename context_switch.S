#include "syscall_number.h"

.syntax unified
.global sleep
sleep:
    svc SLEEP_SVC_NUMBER
    bx lr

.global get_ticks
get_ticks:
    svc GET_TICKS_SVC_NUMBER
    bx lr

.global sbrk
sbrk:
    svc SBRK_SVC_NUMBER
    bx lr

.global register_fifo
register_fifo:
    svc REGISTER_FIFO_SVC_NUMBER
    bx lr

.global unregister_fifo
unregister_fifo:
    svc UNREGISTER_FIFO_SVC_NUMBER
    bx lr

.global activate
activate:
    /* save kernel state in ip register */
    mrs ip, psr
    push {r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}

    /* load user state */
    ldmia r0!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}

    /* switch to process stack */
    msr psp, r0

    mov r0, #3
    msr control, r0

    bx lr