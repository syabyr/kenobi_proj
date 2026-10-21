# Makefile for mic_array — STM32F072 4-channel USB audio firmware
PROJECT   = mic_array
CHIP      = STM32F072CBU6

# Tools
CC        = arm-none-eabi-gcc
AS        = arm-none-eabi-gcc
OBJCOPY   = arm-none-eabi-objcopy
SIZE      = arm-none-eabi-size

# Paths
CHERRYUF2   = ../Cherryuf2
HAL_ROOT    = $(CHERRYUF2)/sdk/ST/stm32f0xx_library/stm32f0xx-hal-driver
CMSIS_F0    = $(CHERRYUF2)/sdk/ST/stm32f0xx_library/cmsis_device_f0
CMSIS_5     = $(CHERRYUF2)/sdk/CMSIS_5
CHERRYUSB   = $(CHERRYUF2)/lib/CherryUSB

BUILD_DIR   = build

# UF2 settings
UF2_FAMILY_ID  = 0x647824b6
UF2_FLASH_BASE = 0x08004000

# ------------------------------------------------------------------ #
# Source files                                                        #
# ------------------------------------------------------------------ #
APP_SRCS = \
    main.c \
    i2s_mic.c \
    usb_audio.c

HAL_SRCS = \
    $(HAL_ROOT)/Src/stm32f0xx_hal.c \
    $(HAL_ROOT)/Src/stm32f0xx_hal_cortex.c \
    $(HAL_ROOT)/Src/stm32f0xx_hal_rcc.c \
    $(HAL_ROOT)/Src/stm32f0xx_hal_rcc_ex.c \
    $(HAL_ROOT)/Src/stm32f0xx_hal_gpio.c \
    $(HAL_ROOT)/Src/stm32f0xx_hal_dma.c \
    $(HAL_ROOT)/Src/stm32f0xx_hal_i2s.c

CMSIS_SRCS = \
    $(CMSIS_F0)/Source/Templates/system_stm32f0xx.c

USBSRCS = \
    $(CHERRYUSB)/core/usbd_core.c \
    $(CHERRYUSB)/class/audio/usbd_audio.c \
    $(CHERRYUSB)/port/fsdev/usb_dc_fsdev.c

ALL_CSRCS = $(APP_SRCS) $(HAL_SRCS) $(CMSIS_SRCS) $(USBSRCS)
ASMS      = startup.s

# ------------------------------------------------------------------ #
# Include paths                                                       #
# ------------------------------------------------------------------ #
INCLUDES  = -I.
INCLUDES += -I$(HAL_ROOT)/Inc
INCLUDES += -I$(CMSIS_F0)/Include
INCLUDES += -I$(CMSIS_5)/CMSIS/Core/Include
INCLUDES += -I$(CHERRYUSB)/core
INCLUDES += -I$(CHERRYUSB)/common
INCLUDES += -I$(CHERRYUSB)/class/audio
INCLUDES += -I$(CHERRYUSB)/port/fsdev

# ------------------------------------------------------------------ #
# Compiler flags                                                      #
# ------------------------------------------------------------------ #
CFLAGS  = -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
CFLAGS += -Os -ffunction-sections -fdata-sections
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-empty-body
CFLAGS += -DSTM32F072xB -DUSE_HAL_DRIVER
CFLAGS += $(INCLUDES)

ASFLAGS = -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
ASFLAGS += -x assembler-with-cpp

LDFLAGS = -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
LDFLAGS += -T linker.ld
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -specs=nosys.specs
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map

# ------------------------------------------------------------------ #
# Build rules                                                         #
# ------------------------------------------------------------------ #

# Derive object list — all .c map to build/basename.o
C_OBJS   = $(addprefix $(BUILD_DIR)/, $(notdir $(ALL_CSRCS:.c=.o)))
ASM_OBJS = $(addprefix $(BUILD_DIR)/, $(notdir $(ASMS:.s=.o)))
ALL_OBJS = $(ASM_OBJS) $(C_OBJS)

# VPATH lets make find source files by basename
VPATH = $(sort $(dir $(ALL_CSRCS)))

.PHONY: all clean uf2

all: $(PROJECT).bin $(PROJECT).uf2

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(PROJECT).elf: $(ALL_OBJS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(ALL_OBJS)
	$(SIZE) $@

$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECT).uf2: $(PROJECT).bin uf2conv.py
	python3 uf2conv.py $(PROJECT).bin $(PROJECT).uf2 $(UF2_FAMILY_ID) $(UF2_FLASH_BASE)

clean:
	rm -rf $(BUILD_DIR) $(PROJECT).bin $(PROJECT).uf2
