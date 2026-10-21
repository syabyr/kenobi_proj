/**
 * Startup code for STM32F072CBU6 application with CherryUF2 bootloader.
 *
 * - Defines the vector table at APP_ADDR (0x08004000)
 * - Initial SP comes from linker symbol _estack
 * - Reset_Handler copies vector table to SRAM and remaps for Cortex-M0
 */
    .syntax unified
    .cpu    cortex-m0
    .thumb

/* =================================================================== */
/* Vector table — placed at start of app flash (APP_ADDR)              */
/* =================================================================== */
    .section .isr_vector, "a"
    .globl  __vectors__

__vectors__:
    .word   _estack                         /*  0: Initial SP */
    .word   Reset_Handler                   /*  1: Reset */
    .word   Default_Handler                 /*  2: NMI */
    .word   Default_Handler                 /*  3: HardFault */
    .word   0                               /*  4: Reserved */
    .word   0                               /*  5: Reserved */
    .word   0                               /*  6: Reserved */
    .word   0                               /*  7: Reserved */
    .word   0                               /*  8: Reserved */
    .word   0                               /*  9: Reserved */
    .word   0                               /* 10: Reserved */
    .word   Default_Handler                 /* 11: SVCall */
    .word   0                               /* 12: Reserved */
    .word   0                               /* 13: Reserved */
    .word   Default_Handler                 /* 14: PendSV */
    .word   SysTick_Handler                 /* 15: SysTick */
    /* IRQ 16..47 — unused, trapped by bootloader SRAM remap */
    .space  (32 * 4)                        /* 16..47 */

    .size   __vectors__, . - __vectors__

/* =================================================================== */
/* Reset Handler                                                       */
/* =================================================================== */
    .section .text.Reset_Handler, "ax"
    .globl  Reset_Handler
    .type   Reset_Handler, %function

Reset_Handler:
    /*
     * Cortex-M0 has no VTOR. Copy the vector table to SRAM and remap
     * SRAM to 0x00000000 so that our ISRs fire instead of the bootloader's.
     * This mirrors CherryUF2's board_app_jump with BOARD_VECTOR_REDIRECT_BOOTLOADER.
     */
    ldr     r0, =__vectors__
    ldr     r1, =0x20000000
    movs    r2, #(48 * 4)       /* 48 entries × 4 bytes = 192 (0xC0) */

1:  ldr     r3, [r0]
    stmia   r1!, {r3}
    adds    r0, r0, #4
    subs    r2, r2, #4
    bne     1b

    /* Remap SRAM to 0x00000000: SYSCFG_CFGR1 bits[1:0] = 0b11 */
    ldr     r0, =0x40010000
    ldr     r1, [r0]
    movs    r2, #3
    orrs    r1, r1, r2
    str     r1, [r0]

    /* Re-load SP from the linker-provided _estack (for safety) */
    ldr     r0, =_estack
    msr     MSP, r0

    /* Branch to C system init + main loop */
    bl      SystemInit

    bl      main
    b       .

    .size   Reset_Handler, . - Reset_Handler

/* =================================================================== */
/* Default Handler — infinite loop                                     */
/* =================================================================== */
    .section .text.Default_Handler, "ax"
    .globl  Default_Handler
    .type   Default_Handler, %function

Default_Handler:
    b       .

    .size   Default_Handler, . - Default_Handler

/* =================================================================== */
/* Weak aliases for unused handlers so they point to Default_Handler   */
/* =================================================================== */
    .weak   NMI_Handler
    .thumb_set NMI_Handler, Default_Handler

    .weak   HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler

    .weak   SVCall_Handler
    .thumb_set SVCall_Handler, Default_Handler

    .weak   PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler
