# Build settings
PROJECT   = blink_test
CHIP      = STM32F072CBU6

# Tools
CC        = arm-none-eabi-gcc
AS        = arm-none-eabi-gcc
OBJCOPY   = arm-none-eabi-objcopy
SIZE      = arm-none-eabi-size

# Source files
SRCS      = main.c
ASMS      = startup.s

# Flags
CFLAGS    = -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
CFLAGS   += -Os -ffunction-sections -fdata-sections
CFLAGS   += -Wall -Wextra
CFLAGS   += -DSTM32F072xB
CFLAGS   += -I.

ASFLAGS   = -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
ASFLAGS   += -x assembler-with-cpp

LDFLAGS   = -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
LDFLAGS   += -T linker.ld
LDFLAGS   += -nostartfiles -nostdlib
LDFLAGS   += -Wl,--gc-sections
LDFLAGS   += -specs=nosys.specs
LDFLAGS   += -Wl,-Map=$(PROJECT).map

# UF2 settings (matching CherryUF2 for STM32F0)
UF2_FAMILY_ID = 0x647824b6
UF2_FLASH_BASE = 0x08004000

# Build objects
OBJS = $(ASMS:.s=.o) $(SRCS:.c=.o)

.PHONY: all clean uf2

all: $(PROJECT).bin $(PROJECT).uf2

startup.o: startup.s
	$(AS) $(ASFLAGS) -c -o $@ $<

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(PROJECT).elf: startup.o main.o linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ startup.o main.o
	$(SIZE) $@

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECT).uf2: $(PROJECT).bin uf2conv.py
	python3 uf2conv.py $(PROJECT).bin $(PROJECT).uf2 $(UF2_FAMILY_ID) $(UF2_FLASH_BASE)

clean:
	rm -f *.o $(PROJECT).elf $(PROJECT).bin $(PROJECT).uf2 $(PROJECT).map
