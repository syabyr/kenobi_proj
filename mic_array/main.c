#include "stm32f0xx_hal.h"
#include "i2s_mic.h"
#include "usb_audio.h"

static void clock_init(void);
static void gpio_led_init(void);
static void audio_data_callback(const uint8_t *buf, uint32_t len);

void SysTick_Handler(void)
{
    HAL_IncTick();
}

int main(void)
{
    HAL_Init();
    clock_init();
    gpio_led_init();

    mic_array_init(audio_data_callback);
    usb_audio_init();
    mic_array_start();

    uint32_t last_tick = 0;
    uint32_t last_drops = 0;
    uint32_t last_tx = 0;
    while (1) {
        uint32_t t = HAL_GetTick();
        if (t - last_tick >= 1000) {
            uint32_t cur_drops = usb_audio_get_drop_count();
            uint32_t cur_tx    = usb_audio_get_tx_count();
            uint32_t drop_rate = cur_drops - last_drops;
            uint32_t tx_rate   = cur_tx - last_tx;
            last_drops = cur_drops;
            last_tx    = cur_tx;
            last_tick  = t;

            if (drop_rate > 0) {
                /* Double-blink: packet drops detected in this second */
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2,
                    ((t % 400) < 80) || ((t % 400) >= 160 && (t % 400) < 240));
            } else if (tx_rate > 0 && mic_array_is_group1_active()) {
                /* Slow blink: no drops, audio flowing (tx_rate ≈ 1000) */
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, (t / 500) & 1);
            } else {
                /* Fast blink: no activity yet */
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, (t / 100) & 1);
            }
        }
    }
}

static void audio_data_callback(const uint8_t *buf, uint32_t len)
{
    usb_audio_send(buf, len);
}

static void clock_init(void)
{
    /* HSE 12.288 MHz → SYSCLK (exact 16 kHz I2S), HSI48 → USB */
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSI48State     = RCC_HSI48_ON;
    osc.PLL.PLLState   = RCC_PLL_OFF;
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSE;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);

    RCC_PeriphCLKInitTypeDef periph = {0};
    periph.PeriphClockSelection = RCC_PERIPHCLK_USB;
    periph.UsbClockSelection    = RCC_USBCLKSOURCE_HSI48;
    HAL_RCCEx_PeriphCLKConfig(&periph);

    /* CRS: lock HSI48 to USB SOF (1 kHz) — brings ±1% RC down to ≤0.1% */
    RCC_CRSInitTypeDef crs = {0};
    crs.Prescaler             = RCC_CRS_SYNC_DIV1;
    crs.Source                = RCC_CRS_SYNC_SOURCE_USB;
    crs.Polarity              = RCC_CRS_SYNC_POLARITY_RISING;
    crs.ReloadValue           = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000U, 1000U);
    crs.ErrorLimitValue       = RCC_CRS_ERRORLIMIT_DEFAULT;
    crs.HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT;
    HAL_RCCEx_CRSConfig(&crs);
    /* Trim happens automatically in hardware — no ISR needed for basic operation */
}

static void gpio_led_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_2;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);
}
