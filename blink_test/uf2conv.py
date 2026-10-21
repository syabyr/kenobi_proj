#!/usr/bin/env python3
"""
Convert a binary file to UF2 format for STM32F0 CherryUF2 bootloader.

Usage: python3 uf2conv.py <input.bin> <output.uf2> [family_id] [flash_base]
"""
import struct
import sys
import os

UF2_MAGIC_START0   = 0x0A324655  # "UF2\n"
UF2_MAGIC_START1   = 0x9E5D5157
UF2_MAGIC_END      = 0x0AB16F30
UF2_FLAG_FAMILY_ID = 0x00002000

BLOCK_SIZE  = 256
UF2_TOTAL   = 512

# CherryUF2 STM32F0 defaults
DEFAULT_FAMILY_ID  = 0x647824B6
DEFAULT_FLASH_BASE = 0x08004000


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.bin> <output.uf2> [family_id] [flash_base]")
        sys.exit(1)

    input_path  = sys.argv[1]
    output_path = sys.argv[2]
    family_id   = int(sys.argv[3], 0) if len(sys.argv) > 3 else DEFAULT_FAMILY_ID
    flash_base  = int(sys.argv[4], 0) if len(sys.argv) > 4 else DEFAULT_FLASH_BASE

    with open(input_path, "rb") as f:
        data = f.read()

    # Pad to 256-byte boundary
    if len(data) % BLOCK_SIZE:
        data += b"\x00" * (BLOCK_SIZE - len(data) % BLOCK_SIZE)

    num_blocks = len(data) // BLOCK_SIZE

    with open(output_path, "wb") as out:
        for i in range(num_blocks):
            addr  = flash_base + i * BLOCK_SIZE
            block = bytearray(UF2_TOTAL)

            # Header (32 bytes)
            struct.pack_into("<IIIIIIII", block, 0,
                UF2_MAGIC_START0,   #  0: magicStart0
                UF2_MAGIC_START1,   #  4: magicStart1
                UF2_FLAG_FAMILY_ID, #  8: flags (familyID present)
                addr,               # 12: targetAddr
                BLOCK_SIZE,         # 16: payloadSize
                i,                  # 20: blockNo
                num_blocks,         # 24: numBlocks
                family_id,          # 28: reserved → familyID
            )

            # Payload
            offset = i * BLOCK_SIZE
            chunk  = data[offset : offset + BLOCK_SIZE]
            block[32 : 32 + len(chunk)] = chunk

            # Magic at end (last 4 bytes of 476-byte data region)
            struct.pack_into("<I", block, 512 - 4, UF2_MAGIC_END)

            out.write(block)

    out_size = os.path.getsize(output_path)
    print(f"  UF2: {num_blocks} blocks, {len(data)} bytes payload → {out_size} bytes")
    print(f"  Family: 0x{family_id:08X}  Base: 0x{flash_base:08X}")


if __name__ == "__main__":
    main()
