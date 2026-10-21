#pragma once
#include <stdint.h>
#include <stdbool.h>

void usb_audio_init(void);
bool usb_audio_is_open(void);
bool usb_audio_send(const uint8_t *buf, uint32_t len);
void USB_IRQHandler(void);

/* Get linear gain factor for a channel (Q8 fixed-point: 256 = 1.0x) */
uint16_t usb_audio_get_gain_q8(uint8_t ch);

/* Diagnostic counters */
uint32_t usb_audio_get_drop_count(void);
uint32_t usb_audio_get_tx_count(void);
