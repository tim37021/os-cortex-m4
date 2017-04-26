#include "syscall_number.h"

.syntax unified
.global syscall
syscall:
    svc 0
    bx lr

.global sleep
sleep:
    svc SLEEP_SVC_NUMBER
    bx lr

.global back_to_sv
    svc BACKTOSV_SVC_NUMBER
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