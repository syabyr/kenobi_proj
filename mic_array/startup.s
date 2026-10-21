/**
 * Startup code for STM32F072CBU6 application with CherryUF2 bootloader.
 *
 * - Defines the vector table at APP_ADDR (0x08004000)
 * - Initial SP comes from linker symbol _estack
 * - Reset_Handler copies vector table to SRAM and remaps for Cortex-M0
 * - Full STM32F072 IRQ vector table (48 entries total)
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
    .word   _estack                             /*  0: Initial SP */
    .word   Reset_Handler                       /*  1: Reset */
    .word   NMI_Handler                         /*  2: NMI */
    .word   HardFault_Handler                   /*  3: HardFault */
    .word   0                                   /*  4: Reserved */
    .word   0                                   /*  5: Reserved */
    .word   0                                   /*  6: Reserved */
    .word   0                                   /*  7: Reserved */
    .word   0                                   /*  8: Reserved */
    .word   0                                   /*  9: Reserved */
    .word   0                                   /* 10: Reserved */
    .word   SVCall_Handler                      /* 11: SVCall */
    .word   0                                   /* 12: Reserved */
    .word   0                                   /* 13: Reserved */
    .word   PendSV_Handler                      /* 14: PendSV */
    .word   SysTick_Handler                     /* 15: SysTick */
    /* External IRQs 0..31 */
    .word   WWDG_IRQHandler                     /* IRQ0:  WWDG */
    .word   PVD_VDDIO2_IRQHandler               /* IRQ1:  PVD/VDDIO2 */
    .word   RTC_IRQHandler                      /* IRQ2:  RTC */
    .word   FLASH_IRQHandler                    /* IRQ3:  Flash */
    .word   RCC_CRS_IRQHandler                  /* IRQ4:  RCC/CRS */
    .word   EXTI0_1_IRQHandler                  /* IRQ5:  EXTI[1:0] */
    .word   EXTI2_3_IRQHandler                  /* IRQ6:  EXTI[3:2] */
    .word   EXTI4_15_IRQHandler                 /* IRQ7:  EXTI[15:4] */
    .word   TSC_IRQHandler                      /* IRQ8:  TSC */
    .word   DMA1_Channel1_IRQHandler            /* IRQ9:  DMA1 Ch1 */
    .word   DMA1_Channel2_3_IRQHandler          /* IRQ10: DMA1 Ch2-3 */
    .word   DMA1_Channel4_5_6_7_IRQHandler      /* IRQ11: DMA1 Ch4-7 */
    .word   ADC1_COMP_IRQHandler                /* IRQ12: ADC1/COMP */
    .word   TIM1_BRK_UP_TRG_COM_IRQHandler      /* IRQ13: TIM1 BRK/UP/TRG/COM */
    .word   TIM1_CC_IRQHandler                  /* IRQ14: TIM1 CC */
    .word   TIM2_IRQHandler                     /* IRQ15: TIM2 */
    .word   TIM3_IRQHandler                     /* IRQ16: TIM3 */
    .word   TIM6_DAC_IRQHandler                 /* IRQ17: TIM6/DAC */
    .word   TIM7_IRQHandler                     /* IRQ18: TIM7 */
    .word   TIM14_IRQHandler                    /* IRQ19: TIM14 */
    .word   TIM15_IRQHandler                    /* IRQ20: TIM15 */
    .word   TIM16_IRQHandler                    /* IRQ21: TIM16 */
    .word   TIM17_IRQHandler                    /* IRQ22: TIM17 */
    .word   I2C1_IRQHandler                     /* IRQ23: I2C1 */
    .word   I2C2_IRQHandler                     /* IRQ24: I2C2 */
    .word   SPI1_IRQHandler                     /* IRQ25: SPI1 */
    .word   SPI2_IRQHandler                     /* IRQ26: SPI2 */
    .word   USART1_IRQHandler                   /* IRQ27: USART1 */
    .word   USART2_IRQHandler                   /* IRQ28: USART2 */
    .word   USART3_4_IRQHandler                 /* IRQ29: USART3/4 */
    .word   CEC_CAN_IRQHandler                  /* IRQ30: CEC/CAN */
    .word   USB_IRQHandler                      /* IRQ31: USB */

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
    movs    r2, #(48 * 4)       /* 48 entries × 4 bytes = 192 */

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

    /* Zero-initialize .bss section */
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0
2:  cmp     r0, r1
    bge     3f
    stmia   r0!, {r2}
    b       2b
3:
    /* Copy .data from flash to RAM */
    ldr     r0, =_sidata
    ldr     r1, =_sdata
    ldr     r2, =_edata
4:  cmp     r1, r2
    bge     5f
    ldr     r3, [r0]
    stmia   r1!, {r3}
    adds    r0, r0, #4
    b       4b
5:
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
/* Weak aliases for Cortex-M core exception handlers                   */
/* =================================================================== */
    .weak   NMI_Handler
    .thumb_set NMI_Handler, Default_Handler

    .weak   HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler

    .weak   SVCall_Handler
    .thumb_set SVCall_Handler, Default_Handler

    .weak   PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler

    .weak   SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

/* =================================================================== */
/* Weak aliases for all STM32F072 external IRQ handlers                */
/* =================================================================== */
    .weak   WWDG_IRQHandler
    .thumb_set WWDG_IRQHandler, Default_Handler

    .weak   PVD_VDDIO2_IRQHandler
    .thumb_set PVD_VDDIO2_IRQHandler, Default_Handler

    .weak   RTC_IRQHandler
    .thumb_set RTC_IRQHandler, Default_Handler

    .weak   FLASH_IRQHandler
    .thumb_set FLASH_IRQHandler, Default_Handler

    .weak   RCC_CRS_IRQHandler
    .thumb_set RCC_CRS_IRQHandler, Default_Handler

    .weak   EXTI0_1_IRQHandler
    .thumb_set EXTI0_1_IRQHandler, Default_Handler

    .weak   EXTI2_3_IRQHandler
    .thumb_set EXTI2_3_IRQHandler, Default_Handler

    .weak   EXTI4_15_IRQHandler
    .thumb_set EXTI4_15_IRQHandler, Default_Handler

    .weak   TSC_IRQHandler
    .thumb_set TSC_IRQHandler, Default_Handler

    .weak   DMA1_Channel1_IRQHandler
    .thumb_set DMA1_Channel1_IRQHandler, Default_Handler

    .weak   DMA1_Channel2_3_IRQHandler
    .thumb_set DMA1_Channel2_3_IRQHandler, Default_Handler

    .weak   DMA1_Channel4_5_6_7_IRQHandler
    .thumb_set DMA1_Channel4_5_6_7_IRQHandler, Default_Handler

    .weak   ADC1_COMP_IRQHandler
    .thumb_set ADC1_COMP_IRQHandler, Default_Handler

    .weak   TIM1_BRK_UP_TRG_COM_IRQHandler
    .thumb_set TIM1_BRK_UP_TRG_COM_IRQHandler, Default_Handler

    .weak   TIM1_CC_IRQHandler
    .thumb_set TIM1_CC_IRQHandler, Default_Handler

    .weak   TIM2_IRQHandler
    .thumb_set TIM2_IRQHandler, Default_Handler

    .weak   TIM3_IRQHandler
    .thumb_set TIM3_IRQHandler, Default_Handler

    .weak   TIM6_DAC_IRQHandler
    .thumb_set TIM6_DAC_IRQHandler, Default_Handler

    .weak   TIM7_IRQHandler
    .thumb_set TIM7_IRQHandler, Default_Handler

    .weak   TIM14_IRQHandler
    .thumb_set TIM14_IRQHandler, Default_Handler

    .weak   TIM15_IRQHandler
    .thumb_set TIM15_IRQHandler, Default_Handler

    .weak   TIM16_IRQHandler
    .thumb_set TIM16_IRQHandler, Default_Handler

    .weak   TIM17_IRQHandler
    .thumb_set TIM17_IRQHandler, Default_Handler

    .weak   I2C1_IRQHandler
    .thumb_set I2C1_IRQHandler, Default_Handler

    .weak   I2C2_IRQHandler
    .thumb_set I2C2_IRQHandler, Default_Handler

    .weak   SPI1_IRQHandler
    .thumb_set SPI1_IRQHandler, Default_Handler

    .weak   SPI2_IRQHandler
    .thumb_set SPI2_IRQHandler, Default_Handler

    .weak   USART1_IRQHandler
    .thumb_set USART1_IRQHandler, Default_Handler

    .weak   USART2_IRQHandler
    .thumb_set USART2_IRQHandler, Default_Handler

    .weak   USART3_4_IRQHandler
    .thumb_set USART3_4_IRQHandler, Default_Handler

    .weak   CEC_CAN_IRQHandler
    .thumb_set CEC_CAN_IRQHandler, Default_Handler

    .weak   USB_IRQHandler
    .thumb_set USB_IRQHandler, Default_Handler
