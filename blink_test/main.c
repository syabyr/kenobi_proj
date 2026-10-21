#include <stdint.h>

/*--------------------------------------------------------------------+
 * App base address (matches CherryUF2 CONFIG_BOOTUF2_APP_START_ADDR)
 *--------------------------------------------------------------------*/
#define APP_ADDR 0x08004000U

/*--------------------------------------------------------------------+
 * RCC
 *--------------------------------------------------------------------*/
#define RCC_BASE           0x40021000UL
#define RCC_AHBENR         (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_AHBENR_GPIOAEN (1UL << 17)

/*--------------------------------------------------------------------+
 * GPIOA
 *--------------------------------------------------------------------*/
#define GPIOA_BASE         0x48000000UL
#define GPIOA_MODER        (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR          (*(volatile uint32_t *)(GPIOA_BASE + 0x14))

/*--------------------------------------------------------------------+
 * SysTick
 *--------------------------------------------------------------------*/
#define SysTick_Base        0xE000E010UL
#define SysTick_CTRL        (*(volatile uint32_t *)(SysTick_Base + 0x00))
#define SysTick_LOAD        (*(volatile uint32_t *)(SysTick_Base + 0x04))
#define SysTick_VAL         (*(volatile uint32_t *)(SysTick_Base + 0x08))

#define SysTick_CTRL_ENABLE    (1UL << 0)
#define SysTick_CTRL_TICKINT   (1UL << 1)
#define SysTick_CTRL_CLKSOURCE (1UL << 2)

/*--------------------------------------------------------------------+
 * Clock: HSI = 8 MHz
 *--------------------------------------------------------------------*/
#define HSI_VALUE 8000000UL

/*--------------------------------------------------------------------+
 * SysTick — 1ms tick counter
 * SysTick clock = AHB/8 = 1 MHz, so LOAD = 999 → 1ms period
 *--------------------------------------------------------------------*/
static volatile uint32_t ms_tick;

void SysTick_Handler(void) {
    ms_tick++;
}

static void delay_ms(uint32_t ms) {
    uint32_t target = ms_tick + ms;
    while (ms_tick < target) { }
}

/*--------------------------------------------------------------------+
 * SystemInit — called from startup before main()
 *--------------------------------------------------------------------*/
void SystemInit(void) {
    /* Enable GPIOA clock */
    RCC_AHBENR |= RCC_AHBENR_GPIOAEN;
    __asm volatile("dsb");

    /* PA2: push-pull output (MODER[5:4] = 01) */
    GPIOA_MODER = (GPIOA_MODER & ~(0x3UL << (2 * 2))) | (0x1UL << (2 * 2));

    /* SysTick: 1ms period. HSI=8MHz, AHB/8=1MHz → LOAD=999 */
    SysTick_LOAD = (HSI_VALUE / 8) / 1000 - 1;
    SysTick_VAL  = 0;
    SysTick_CTRL = SysTick_CTRL_ENABLE
                 | SysTick_CTRL_TICKINT
                 | SysTick_CTRL_CLKSOURCE;  /* AHB/8 */
}

/*--------------------------------------------------------------------+
 * main — toggle PA2 every 1 second
 *--------------------------------------------------------------------*/
int main(void) {
    while (1) {
        delay_ms(1000);
        GPIOA_ODR ^= (1UL << 2);  /* toggle PA2 */
    }
    return 0;
}
