/*
 * I2S microphone driver for 4x ICS-43434 MEMS microphones.
 *
 * Group 1 (SPI1/I2S1): PA15=WS, PA7=SD, PB3=SCK
 * Group 2 (SPI2/I2S2): PB12=WS, PB15=SD, PB10=SCK (AF5!)
 */
#include "i2s_mic.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* DMA buffer: 24-bit I2S → 2 half-words/sample × 16 samples/ms × 2 ch × 2 (double-buffer) */
#define I2S_DMA_BUF_LEN  (MIC_SAMPLES_PER_MS * 2 * 2 * 2)  /* 128 half-words */

static I2S_HandleTypeDef hi2s1;
static I2S_HandleTypeDef hi2s2;
static DMA_HandleTypeDef hdma_spi1_rx;
static DMA_HandleTypeDef hdma_spi2_rx;

static uint16_t i2s1_dma_buf[I2S_DMA_BUF_LEN];
static uint16_t i2s2_dma_buf[I2S_DMA_BUF_LEN];
static uint8_t  usb_pkt[MIC_USB_PKT_BYTES];

static mic_data_ready_cb_t s_data_cb;

static volatile bool s_group1_active = false;
static volatile bool s_group2_active = false;

/* ------------------------------------------------------------------ */
/* Copy one half of DMA buffer (1 ms of audio) into USB packet        */
/* ------------------------------------------------------------------ */
extern uint16_t usb_audio_get_gain_q8(uint8_t ch);

/* 24-bit I2S → each sample = 2 half-words. Take MSB half-word (bits [23:8]). */
static void fill_pkt(uint16_t dma_off)
{
    uint16_t *dst = (uint16_t *)usb_pkt;
    uint32_t gain_q8 = usb_audio_get_gain_q8(0);
    for (uint16_t i = 0; i < MIC_SAMPLES_PER_MS; i++) {
        int32_t l = (int16_t)i2s1_dma_buf[dma_off + i * 4 + 0];
        int32_t r = (int16_t)i2s1_dma_buf[dma_off + i * 4 + 2];
        l = (l * (int32_t)gain_q8) >> 8;
        r = (r * (int32_t)gain_q8) >> 8;
        if (l > 32767)  l = 32767;
        if (l < -32768) l = -32768;
        if (r > 32767)  r = 32767;
        if (r < -32768) r = -32768;
        dst[i * 2 + 0] = (uint16_t)l;
        dst[i * 2 + 1] = (uint16_t)r;
        if (l || r) s_group1_active = true;
    }
}

/* ------------------------------------------------------------------ */
/* HAL MSP init — called by HAL_I2S_Init()                            */
/* ------------------------------------------------------------------ */
void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
    GPIO_InitTypeDef gpio = {0};

    if (hi2s->Instance == SPI1) {
        /* Enable clocks */
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

        /* PA7 = SD (AF0) */
        gpio.Pin       = GPIO_PIN_7;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF0_SPI1;
        HAL_GPIO_Init(GPIOA, &gpio);

        /* PA15 = WS (AF0) */
        gpio.Pin       = GPIO_PIN_15;
        gpio.Alternate = GPIO_AF0_SPI1;
        HAL_GPIO_Init(GPIOA, &gpio);

        /* PB3 = SCK (AF0) */
        gpio.Pin       = GPIO_PIN_3;
        gpio.Alternate = GPIO_AF0_SPI1;
        HAL_GPIO_Init(GPIOB, &gpio);

        /* DMA1 Channel 2 for SPI1_RX */
        hdma_spi1_rx.Instance                 = DMA1_Channel2;
        hdma_spi1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma_spi1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_spi1_rx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_spi1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
        hdma_spi1_rx.Init.Mode                = DMA_CIRCULAR;
        hdma_spi1_rx.Init.Priority            = DMA_PRIORITY_HIGH;
        HAL_DMA_Init(&hdma_spi1_rx);
        __HAL_LINKDMA(hi2s, hdmarx, hdma_spi1_rx);

        /* DMA1_Channel2_3 IRQ (both channels share one IRQn) */
        HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

    } else if (hi2s->Instance == SPI2) {
        /* Enable clocks */
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

        /* PB15 = SD (AF0) */
        gpio.Pin       = GPIO_PIN_15;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF0_SPI2;
        HAL_GPIO_Init(GPIOB, &gpio);

        /* PB12 = WS (AF0) */
        gpio.Pin       = GPIO_PIN_12;
        gpio.Alternate = GPIO_AF0_SPI2;
        HAL_GPIO_Init(GPIOB, &gpio);

        /* PB10 = SCK — MUST use AF5 on STM32F072 */
        gpio.Pin       = GPIO_PIN_10;
        gpio.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &gpio);

        /* DMA1 Channel 4 for SPI2_RX */
        hdma_spi2_rx.Instance                 = DMA1_Channel4;
        hdma_spi2_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma_spi2_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_spi2_rx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_spi2_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
        hdma_spi2_rx.Init.Mode                = DMA_CIRCULAR;
        hdma_spi2_rx.Init.Priority            = DMA_PRIORITY_HIGH;
        HAL_DMA_Init(&hdma_spi2_rx);
        __HAL_LINKDMA(hi2s, hdmarx, hdma_spi2_rx);

        /* DMA1_Channel4_5_6_7 IRQ */
        HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
    }
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI1) {
        __HAL_RCC_SPI1_FORCE_RESET();
        __HAL_RCC_SPI1_RELEASE_RESET();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_7 | GPIO_PIN_15);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3);
        HAL_DMA_DeInit(&hdma_spi1_rx);
        HAL_NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
    } else if (hi2s->Instance == SPI2) {
        __HAL_RCC_SPI2_FORCE_RESET();
        __HAL_RCC_SPI2_RELEASE_RESET();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_15);
        HAL_DMA_DeInit(&hdma_spi2_rx);
        HAL_NVIC_DisableIRQ(DMA1_Channel4_5_6_7_IRQn);
    }
}

/* ------------------------------------------------------------------ */
/* mic_array_init                                                      */
/* ------------------------------------------------------------------ */
void mic_array_init(mic_data_ready_cb_t cb)
{
    s_data_cb = cb;
    memset(i2s1_dma_buf, 0, sizeof(i2s1_dma_buf));
    memset(i2s2_dma_buf, 0, sizeof(i2s2_dma_buf));
    memset(usb_pkt, 0, sizeof(usb_pkt));

    /* I2S1 / SPI1 */
    hi2s1.Instance          = SPI1;
    hi2s1.Init.Mode         = I2S_MODE_MASTER_RX;
    hi2s1.Init.Standard     = I2S_STANDARD_PHILIPS;
    hi2s1.Init.DataFormat   = I2S_DATAFORMAT_24B;
    hi2s1.Init.MCLKOutput   = I2S_MCLKOUTPUT_DISABLE;
    hi2s1.Init.AudioFreq    = I2S_AUDIOFREQ_16K;
    hi2s1.Init.CPOL         = I2S_CPOL_LOW;
    HAL_I2S_Init(&hi2s1);

    /* I2S2 not used in single-bus test */
    (void)hi2s2;
}

void mic_array_start(void)
{
    /* HAL doubles Size for 24-bit mode internally, so pass half the buffer depth */
    HAL_I2S_Receive_DMA(&hi2s1, i2s1_dma_buf, I2S_DMA_BUF_LEN / 2);
}

void mic_array_stop(void)
{
    HAL_I2S_DMAStop(&hi2s1);
    HAL_I2S_DMAStop(&hi2s2);
}

bool mic_array_is_group1_active(void)
{
    return s_group1_active;
}

bool mic_array_is_group2_active(void)
{
    return s_group2_active;
}

/* ------------------------------------------------------------------ */
/* HAL callbacks (called from DMA ISR context)                         */
/* ------------------------------------------------------------------ */
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI1) {
        fill_pkt(0);
        if (s_data_cb) {
            s_data_cb(usb_pkt, MIC_USB_PKT_BYTES);
        }
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI1) {
        fill_pkt(MIC_SAMPLES_PER_MS * 2 * 2);
        if (s_data_cb) {
            s_data_cb(usb_pkt, MIC_USB_PKT_BYTES);
        }
    }
}

/* ------------------------------------------------------------------ */
/* IRQ handlers                                                        */
/* ------------------------------------------------------------------ */
void DMA1_Channel2_3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi1_rx);
}

void DMA1_Channel4_5_6_7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

void SPI1_IRQHandler(void)
{
    HAL_I2S_IRQHandler(&hi2s1);
}

void SPI2_IRQHandler(void)
{
    HAL_I2S_IRQHandler(&hi2s2);
}
