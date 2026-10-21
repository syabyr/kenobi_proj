/*
 * STM32F0xx HAL configuration for mic_array project.
 */
#ifndef STM32F0XX_HAL_CONF_H
#define STM32F0XX_HAL_CONF_H

/* ======================== Module selection ========================= */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2S_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED

/* ======================== Oscillator values ======================== */
#define HSI_VALUE    ((uint32_t)8000000)
#define HSI48_VALUE  ((uint32_t)48000000)
#define HSE_VALUE    ((uint32_t)12288000)
#define HSE_STARTUP_TIMEOUT    ((uint32_t)100)
#define LSE_STARTUP_TIMEOUT    ((uint32_t)5000)
#define LSE_VALUE    ((uint32_t)32768)
#define LSI_VALUE    ((uint32_t)40000)

/* ======================== Misc ===================================== */
#define VDD_VALUE                    ((uint32_t)3300)
#define TICK_INT_PRIORITY            ((uint32_t)0)
#define USE_RTOS                     0
#define PREFETCH_ENABLE              1
#define USE_HAL_I2S_REGISTER_CALLBACKS 0
#define USE_HAL_SPI_REGISTER_CALLBACKS 0

/* Disable assert_param to reduce code size */
#define assert_param(expr)  ((void)0)

/* ======================== HAL headers ============================== */
#include "stm32f0xx_hal_rcc.h"
#include "stm32f0xx_hal_rcc_ex.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_gpio_ex.h"
#include "stm32f0xx_hal_dma.h"
#include "stm32f0xx_hal_cortex.h"
#include "stm32f0xx_hal_pwr.h"
#include "stm32f0xx_hal_pwr_ex.h"
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include "stm32f0xx_hal_i2s.h"

#endif /* STM32F0XX_HAL_CONF_H */
