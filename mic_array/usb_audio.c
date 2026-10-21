/*
 * USB Audio v1 device for 4-channel 16kHz microphone.
 * Based on CherryUSB audio_v1_mic_multichan_template.c
 */
#include "usbd_core.h"
#include "usbd_audio.h"
#include "stm32f0xx_hal.h"
#include "usb_audio.h"

/* ------------------------------------------------------------------ */
#define USBD_VID           0x1234
#define USBD_PID           0x5678
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#ifdef CONFIG_USB_HS
#define EP_INTERVAL 0x04
#else
#define EP_INTERVAL 0x01
#endif

#define AUDIO_IN_EP    0x81
#define AUDIO_IN_FU_ID 0x02
#define AUDIO_FREQ     16000U
#define IN_CHANNEL_NUM 2

#define INPUT_CTRL      0x03, 0x03, 0x03
#define INPUT_CH_ENABLE 0x0003

/* 16-bit × 2 ch × 16 samples/ms = 64 bytes/packet */
#define AUDIO_IN_PACKET ((uint32_t)((AUDIO_FREQ * 2 * IN_CHANNEL_NUM) / 1000))

#define USB_AUDIO_CONFIG_DESC_SIZ (unsigned long)(9 +                                                    \
                                                  AUDIO_AC_DESCRIPTOR_INIT_LEN(1) +                      \
                                                  AUDIO_SIZEOF_AC_INPUT_TERMINAL_DESC +                  \
                                                  AUDIO_SIZEOF_AC_FEATURE_UNIT_DESC(IN_CHANNEL_NUM, 1) + \
                                                  AUDIO_SIZEOF_AC_OUTPUT_TERMINAL_DESC +                 \
                                                  AUDIO_AS_DESCRIPTOR_INIT_LEN(1))

#define AUDIO_AC_SIZ (AUDIO_SIZEOF_AC_HEADER_DESC(1) +                      \
                      AUDIO_SIZEOF_AC_INPUT_TERMINAL_DESC +                  \
                      AUDIO_SIZEOF_AC_FEATURE_UNIT_DESC(IN_CHANNEL_NUM, 1) + \
                      AUDIO_SIZEOF_AC_OUTPUT_TERMINAL_DESC)

/* ------------------------------------------------------------------ */
/* Descriptor table                                                    */
/* ------------------------------------------------------------------ */
static const uint8_t audio_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xef, 0x02, 0x01, USBD_VID, USBD_PID, 0x0001, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_AUDIO_CONFIG_DESC_SIZ, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    AUDIO_AC_DESCRIPTOR_INIT(0x00, 0x02, AUDIO_AC_SIZ, 0x00, 0x01),
    AUDIO_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(0x01, AUDIO_INTERM_MIC, IN_CHANNEL_NUM, INPUT_CH_ENABLE),
    AUDIO_AC_FEATURE_UNIT_DESCRIPTOR_INIT(AUDIO_IN_FU_ID, 0x01, 0x01, INPUT_CTRL),
    AUDIO_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(0x03, AUDIO_TERMINAL_STREAMING, AUDIO_IN_FU_ID),
    AUDIO_AS_DESCRIPTOR_INIT(0x01, 0x03, IN_CHANNEL_NUM, 2, 16, AUDIO_IN_EP, 0x05, AUDIO_IN_PACKET, EP_INTERVAL, AUDIO_SAMPLE_FREQ_3B(AUDIO_FREQ)),
    /* String 0: language */
    USB_LANGID_INIT(USBD_LANGID_STRING),
    /* String 1: Manufacturer */
    0x14,
    USB_DESCRIPTOR_TYPE_STRING,
    'C', 0x00, 'h', 0x00, 'e', 0x00, 'r', 0x00,
    'r', 0x00, 'y', 0x00, 'U', 0x00, 'S', 0x00,
    'B', 0x00,
    /* String 2: Product */
    0x1a,
    USB_DESCRIPTOR_TYPE_STRING,
    '2', 0x00, 'C', 0x00, 'h', 0x00, ' ', 0x00,
    'M', 0x00, 'i', 0x00, 'c', 0x00, ' ', 0x00,
    'A', 0x00, 'r', 0x00, 'r', 0x00, 'a', 0x00,
    'y', 0x00,
    /* String 3: Serial */
    0x16,
    USB_DESCRIPTOR_TYPE_STRING,
    '2', 0x00, '0', 0x00, '2', 0x00, '4', 0x00,
    '0', 0x00, '1', 0x00, '0', 0x00, '1', 0x00,
    '0', 0x00, '0', 0x00,
    0x00
};

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */
static volatile bool tx_flag       = false;
static volatile bool ep_tx_busy    = false;

/* 2-slot FIFO smooths HSI48 ±1% clock drift vs crystal-accurate I2S timing */
#define FIFO_SLOTS 2
static uint8_t  s_fifo_buf[FIFO_SLOTS][AUDIO_IN_PACKET];
static uint8_t  s_fifo_head  = 0;  /* next write index */
static uint8_t  s_fifo_tail  = 0;
static uint8_t  s_fifo_count = 0;

/* Diagnostic counters */
static volatile uint32_t s_tx_count   = 0;  /* packets sent to hardware */
static volatile uint32_t s_drop_count = 0;  /* packets dropped (FIFO full) */

/* ------------------------------------------------------------------ */
/* CherryUSB callbacks                                                 */
/* ------------------------------------------------------------------ */
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;
    (void)event;
}

void usbd_audio_open(uint8_t busid, uint8_t intf)
{
    (void)busid;
    (void)intf;
    tx_flag       = true;
    ep_tx_busy    = false;
    s_fifo_head   = 0;
    s_fifo_tail   = 0;
    s_fifo_count  = 0;
}

void usbd_audio_close(uint8_t busid, uint8_t intf)
{
    (void)busid;
    (void)intf;
    tx_flag       = false;
    ep_tx_busy    = false;
    s_fifo_head   = 0;
    s_fifo_tail   = 0;
    s_fifo_count  = 0;
}

static void usbd_audio_iso_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;
    ep_tx_busy = false;
    if (s_fifo_count > 0) {
        ep_tx_busy = true;
        s_tx_count++;
        usbd_ep_start_write(0, AUDIO_IN_EP, s_fifo_buf[s_fifo_tail], AUDIO_IN_PACKET);
        s_fifo_tail = (s_fifo_tail + 1) % FIFO_SLOTS;
        s_fifo_count--;
    }
}

uint32_t usb_audio_get_drop_count(void) { return s_drop_count; }
uint32_t usb_audio_get_tx_count(void)  { return s_tx_count; }

static struct usbd_endpoint audio_in_ep = {
    .ep_cb   = usbd_audio_iso_callback,
    .ep_addr = AUDIO_IN_EP
};

static struct usbd_interface intf0;
static struct usbd_interface intf1;

static struct audio_entity_info audio_entity_table[] = {
    {
        .bEntityId          = AUDIO_IN_FU_ID,
        .bDescriptorSubtype = AUDIO_CONTROL_FEATURE_UNIT,
        .ep                 = AUDIO_IN_EP
    },
};

/* ------------------------------------------------------------------ */
/* usb_audio_init                                                      */
/* ------------------------------------------------------------------ */
void usb_audio_init(void)
{
    /* PA11 (DM) and PA12 (DP): configure as AF2 for USB */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF2_USB;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* Enable USB clock */
    __HAL_RCC_USB_CLK_ENABLE();

    /* Enable USB interrupt at high priority */
    HAL_NVIC_SetPriority(USB_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USB_IRQn);

    /* Register descriptors and initialize stack */
    usbd_desc_register(0, audio_descriptor);
    usbd_add_interface(0, usbd_audio_init_intf(0, &intf0, 0x0100, audio_entity_table, 1));
    usbd_add_interface(0, usbd_audio_init_intf(0, &intf1, 0x0100, audio_entity_table, 1));
    usbd_add_endpoint(0, &audio_in_ep);
    usbd_initialize(0, USB_DEVICE_SPEED_FS, USB_BASE, usbd_event_handler);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */
bool usb_audio_is_open(void)
{
    return tx_flag;
}

bool usb_audio_send(const uint8_t *buf, uint32_t len)
{
    if (!tx_flag) {
        return false;
    }
    if (ep_tx_busy) {
        if (s_fifo_count < FIFO_SLOTS) {
            uint32_t copy = (len <= AUDIO_IN_PACKET) ? len : AUDIO_IN_PACKET;
            const uint8_t *s = buf;
            uint8_t *d = s_fifo_buf[s_fifo_head];
            for (uint32_t i = 0; i < copy; i++) d[i] = s[i];
            s_fifo_head = (s_fifo_head + 1) % FIFO_SLOTS;
            s_fifo_count++;
        } else {
            s_drop_count++;
        }
        return false;
    }
    ep_tx_busy = true;
    s_tx_count++;
    usbd_ep_start_write(0, AUDIO_IN_EP, buf, len);
    return true;
}

/* ------------------------------------------------------------------ */
/* Volume control — override CherryUSB weak functions                  */
/* ------------------------------------------------------------------ */

/* +30 dB pre-gain to compensate for ICS-43434 low sensitivity (~-26 dBFS @ 94 dB SPL) */
#define MIC_PRE_GAIN_Q8  256   /* Q8: 256 = 0 dB (unity gain — test clean signal first) */

static int8_t s_vol_db[IN_CHANNEL_NUM]; /* initialized to 0 (max) by BSS */

/* dB to linear gain Q8 (index = -dB, 0..100) */
static const uint16_t db_to_gain_q8[101] = {
    256, 228, 203, 181, 162, 144, 128, 114, 102,  91,   /*   0 ..  -9 */
     81,  72,  64,  57,  51,  46,  41,  36,  32,  29,   /* -10 .. -19 */
     26,  23,  20,  18,  16,  14,  13,  11,  10,   9,   /* -20 .. -29 */
      8,   7,   6,   6,   5,   5,   4,   4,   3,   3,   /* -30 .. -39 */
      3,   2,   2,   2,   2,   1,   1,   1,   1,   1,   /* -40 .. -49 */
      1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   /* -50 .. -59 */
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* -60 .. -69 */
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* -70 .. -79 */
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* -80 .. -89 */
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* -90 .. -99 */
      0,                                                  /* -100 */
};

void usbd_audio_set_volume(uint8_t busid, uint8_t ep, uint8_t ch, int volume_db)
{
    (void)busid; (void)ep;
    if (ch < IN_CHANNEL_NUM) {
        if (volume_db > 0) volume_db = 0;
        if (volume_db < -100) volume_db = -100;
        s_vol_db[ch] = (int8_t)volume_db;
    }
}

int usbd_audio_get_volume(uint8_t busid, uint8_t ep, uint8_t ch)
{
    (void)busid; (void)ep;
    return (ch < IN_CHANNEL_NUM) ? s_vol_db[ch] : 0;
}

uint16_t usb_audio_get_gain_q8(uint8_t ch)
{
    int idx = (ch < IN_CHANNEL_NUM) ? -(int)s_vol_db[ch] : 0;
    if (idx < 0) idx = 0;
    if (idx > 100) idx = 100;
    return (uint16_t)((MIC_PRE_GAIN_Q8 * (uint32_t)db_to_gain_q8[idx]) / 256);
}

/* ------------------------------------------------------------------ */
/* USB IRQ handler — forwards to CherryUSB                            */
/* ------------------------------------------------------------------ */
extern void USBD_IRQHandler(uint8_t busid);

void USB_IRQHandler(void)
{
    USBD_IRQHandler(0);
}
