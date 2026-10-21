/*
 * Linker script for STM32F072CBU6 application with CherryUF2 bootloader.
 *
 * Bootloader occupies  0x08000000..0x08003FFF (16 KB)
 * Application starts at 0x08004000 (CONFIG_BOOTUF2_APP_START_ADDR)
 *
 * Low 192 bytes of SRAM (0x20000000..0x200000BF) are reserved for the
 * relocated vector table (48 entries × 4 bytes, as done by CherryUF2
 * when BOARD_VECTOR_REDIRECT_BOOTLOADER is active).
 */

_ram_full_start = 0x20000000;
_ram_full_size  = 16K;               /* STM32F072CBU6 */
_vect_ram_addr   = _ram_full_start;
_vect_ram_size   = 192;              /* 48 × 4 */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08004000, LENGTH = 112K
    RAM   (xrw): ORIGIN = _ram_full_start + _vect_ram_size,
                  LENGTH = _ram_full_size - _vect_ram_size
}

_estack = ORIGIN(RAM) + LENGTH(RAM); /* top of usable stack region */
PROVIDE(_estack = _estack);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
