#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// DEVICE INFORMATION
#define MAX_BLOCKS (4096)
#define PAGE_SIZE (8192) // bytes
#define NR_PUNITS (64)   // # of blocks in a segment
#define PAGE_PER_BLOCK (128)
#define BLOCK_PER_SEGMENT (NR_PUNITS)
#define PAGE_PER_SEGMENT (BLOCK_PER_SEGMENT * PAGE_PER_BLOCK)

// dma information
typedef struct dma_info {
  uint32_t tag;
  uint32_t dma_type;
  uint32_t lpn;
  char *data;
} dma_info;

// generic address format
struct address {
  union {
    struct {
      uint32_t bus : 3;
      uint32_t chip : 3;
      uint32_t page : 7;
      uint32_t block : 19;
    } format;
    uint32_t lpn;
  };
};

#ifdef __cplusplus
}
#endif

#endif
