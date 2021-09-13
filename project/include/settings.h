#ifndef SETTINGS_H
#define SETTINGS_H

/**
 * NAND Specification
 * - # of Channels: 4
 * - # of Ways: 8
 * - # of Blocks: 32768 (PAGE_SIZE(# of Blocks in a Chip(x2 Ways)) * 4 (# of
 * Channel))
 * - # of Pages(per Block): 128
 * - Page Size: 8k
 * - Capacity per NAND: PAGE_SIZE * 128 * 32768 bytes (around 32GB)
 *
 * Total Flash Size
 * - # of NAND: 16
 * - Total Capacity: 512GB
 */

// DEVICE INFORMATION
#define MAX_BUS (4)
#define MAX_CHIPS_PER_BUS (2)
#define MAX_BLOCKS_PER_CHIP (4096)
#define MAX_PAGES_PER_BLOCK (128)

#define NUMBER_OF_BLOCKS (MAX_BUS * MAX_CHIPS_PER_BUS * MAX_BLOCKS_PER_CHIP)

#define PAGE_SIZE (8192)           // bytes
#define PAGE_PER_SEGMENT (1 << 13) // Based on the address format

#endif
