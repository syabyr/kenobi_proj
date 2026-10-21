#pragma once
#include "stm32f0xx_hal.h"
#include <stdbool.h>

#define MIC_SAMPLE_RATE       16000U
#define MIC_CHANNELS          2U          /* I2S1 stereo only (single-bus test) */
/* USB output: 16-bit. I2S bus: 24-bit (ICS-43434 native, SCK ≥ 1.024 MHz) */
#define MIC_BITS_PER_SAMPLE   16U
#define MIC_I2S_BITS          24U
#define MIC_BYTES_PER_SAMPLE  (MIC_BITS_PER_SAMPLE / 8)
#define MIC_SAMPLES_PER_MS    (MIC_SAMPLE_RATE / 1000)   /* 16 */
/* Bytes per 1ms USB packet: 16 samples * 4 ch * 2 bytes = 128 */
#define MIC_USB_PKT_BYTES     (MIC_SAMPLES_PER_MS * MIC_CHANNELS * MIC_BYTES_PER_SAMPLE)

/* Called when 1ms of 4-ch audio is ready in buf (MIC_USB_PKT_BYTES bytes) */
typedef void (*mic_data_ready_cb_t)(const uint8_t *buf, uint32_t len);

void mic_array_init(mic_data_ready_cb_t cb);
void mic_array_start(void);
void mic_array_stop(void);

bool mic_array_is_group1_active(void);
bool mic_array_is_group2_active(void);
