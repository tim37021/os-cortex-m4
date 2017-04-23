.syntax unified

.global syscall
syscall:
    svc 0
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

    /*ittt ls
    movls r0, #3
    msrls control, r0
    isbls
*/
    mov r0, #3
    msr control, r0
    bx lr