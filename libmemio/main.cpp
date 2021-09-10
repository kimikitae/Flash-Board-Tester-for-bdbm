/*
The MIT License (MIT)

Copyright (c) 2014-2015 CSAIL, MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include <set>

#include "libmemio.h"

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

// temporary buffer
char buffer[PAGE_SIZE];

// bad block set container
std::set<uint64_t> invalid_segments;

// dma information
typedef struct dma_info {
  uint32_t tag;
  uint32_t dma_type;
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

// end request
void end_req(async_bdbm_req *req);
void erase_end_req(uint64_t seg_num, uint8_t isbad);

// read/write test function
void write(memio_t *mio, uint32_t lpn);
void read(memio_t *mio, uint32_t lpn);

void print_progress(int i, int total) {
  if (i * 100 == (total - 1) * 10) {
    printf("10%% progressed\n");
  } else if (i * 100 == (total - 1) * 25) {
    printf("25%% progressed\n");
  } else if (i * 100 == (total - 1) * 50) {
    printf("50%% progressed\n");
  } else if (i * 100 == (total - 1) * 75) {
    printf("75%% progressed\n");
  } else if (i * 100 == (total - 1) * 100) {
    printf("100%% progressed\n");
  }
}

int main(int argc, char **argv) {
  memio_t *mio;
  if ((mio = memio_open()) == NULL) {
    printf("could not open memio\n");
    return -1;
  }

  // trim for badblock checking
  struct address addr;
  for (int i = 0; i < NUMBER_OF_BLOCKS; i++) {
    addr.lpn = 0;
    addr.format.block = i;
    memio_trim(mio, addr.lpn, PAGE_PER_SEGMENT * PAGE_SIZE, erase_end_req);
    print_progress(i, NUMBER_OF_BLOCKS);
  }
  memio_wait(mio);

  // R/W tester
  addr.lpn = 0;
  printf("%-6s%-16s%-6s%-6s%-6s%-6s\n", "type", "lpn", "bus", "chip", "block",
         "page");
  uint32_t bus, chip, block, page;
  for (bus = 0; bus < MAX_BUS; bus++) {
    for (chip = 0; chip < MAX_CHIPS_PER_BUS; chip++) {
      for (block = 0; block < MAX_BLOCKS_PER_CHIP; block++) {
        for (page = 0; page < MAX_PAGES_PER_BLOCK; page++) {
          addr.format.bus = bus;
          addr.format.chip = chip;
          addr.format.page = page;
          addr.format.block = block;
          if (invalid_segments.find(addr.lpn / (PAGE_PER_SEGMENT)) !=
              invalid_segments.end()) {
            continue;
          }
          printf("%-6s%-016d%-6d%-6d%-6d%-6d\n", "write", addr.lpn,
                 addr.format.bus, addr.format.chip, addr.format.block,
                 addr.format.page);
          write(mio, addr.lpn);
          read(mio, addr.lpn);
        }
      }
    }
  }

  // remove whole
  for (int i = 0; i < NUMBER_OF_BLOCKS; i++) {
    addr.lpn = 0;
    addr.format.block = i;
    memio_trim(mio, addr.lpn, PAGE_PER_SEGMENT * PAGE_SIZE, NULL);
    print_progress(i, NUMBER_OF_BLOCKS);
  }
  memio_close(mio);

  return 0;
}

void end_req(async_bdbm_req *req) {
  dma_info *dma = (dma_info *)req->private_data;
  switch (req->type) {
  case REQTYPE_IO_READ:
    struct address addr;
    addr.lpn = *(uint32_t *)dma->data;
    printf("%-6s%-016d%-6d%-6d%-6d%-6d\n", "read", addr.lpn, addr.format.bus,
           addr.format.chip, addr.format.block, addr.format.page);
    /*do something after read req*/
    memio_free_dma(DMA_READ_BUF, dma->tag);
    break;
  case REQTYPE_IO_WRITE:
    /*do something after write req*/
    memio_free_dma(DMA_WRITE_BUF, dma->tag);
    break;
  default:
    break;
  }
  free(dma);
  free(req);
}

void erase_end_req(uint64_t seg_num, uint8_t isbad) {
  /*managing block mapping when "isbad" set to 1 which mean the segments has
   * some bad blocks*/
  if (isbad) {
    invalid_segments.insert(seg_num);
  }
}

void write(memio_t *mio, uint32_t lpn) {
  memset(buffer, 0, sizeof(buffer));
  *(int *)buffer = lpn;
  /*allocation write dma*/
  dma_info *dma = (dma_info *)malloc(sizeof(dma_info));
  dma->tag = memio_alloc_dma(DMA_WRITE_BUF, &dma->data);
  memcpy(dma->data, buffer, PAGE_SIZE);

  async_bdbm_req *req = (async_bdbm_req *)malloc(sizeof(async_bdbm_req));
  req->type = REQTYPE_IO_WRITE;
  req->private_data = (void *)dma;
  req->end_req = end_req; // when the requset ends, the "end_req" is called

  memio_write(mio, lpn, PAGE_SIZE, (uint8_t *)dma->data, false, (void *)req,
              dma->tag);
}

void read(memio_t *mio, uint32_t lpn) {
  /*allocation read dma*/
  dma_info *dma = (dma_info *)malloc(sizeof(dma_info));
  dma->tag = memio_alloc_dma(DMA_READ_BUF, &dma->data);

  async_bdbm_req *req = (async_bdbm_req *)malloc(sizeof(async_bdbm_req));
  req->type = REQTYPE_IO_READ;
  req->end_req = end_req;
  req->private_data = (void *)dma;
  memio_read(mio, lpn, PAGE_SIZE, (uint8_t *)dma->data, false, (void *)req,
             dma->tag);
}
