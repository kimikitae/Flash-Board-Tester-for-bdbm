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

#include "libmemio.h"

typedef struct dma_info {
  uint32_t tag;
  uint32_t dma_type;
  char *data;
} dma_info;

memio_t *mio;
void end_req(async_bdbm_req *req) {
  dma_info *dma = (dma_info *)req->private_data;
  switch (req->type) {
  case REQTYPE_IO_READ:
    printf("read --> 0x%x\n", *(int *)dma->data);
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
}

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

int main(int argc, char **argv) {
  if ((mio = memio_open()) == NULL) {
    printf("could not open memio\n");
    return -1;
  }

  struct address addr;
  addr.lpn = 0;
  char temp[8192] = {0};
  for (addr.format.block = 0; addr.format.block < 5; addr.format.block += 1) {
    for (addr.format.page = 0; addr.format.page < (1 << 7);
         addr.format.page++) {
      memset(temp, 0, sizeof(temp));
      printf("current address: 0x%x\n", addr.lpn);
      *(int *)temp = addr.lpn;
      // memio_wait(mio);
      /*allocation write dma*/
      dma_info *dma = (dma_info *)malloc(sizeof(dma_info));
      dma->tag = memio_alloc_dma(DMA_WRITE_BUF, &dma->data);
      memcpy(dma->data, temp, 8192);

      async_bdbm_req *temp_req =
          (async_bdbm_req *)malloc(sizeof(async_bdbm_req));
      temp_req->type = REQTYPE_IO_WRITE;
      temp_req->private_data = (void *)dma;
      temp_req->end_req =
          end_req; // when the requset ends, the "end_req" is called

      memio_write(mio, addr.lpn, 8192, (uint8_t *)dma->data, false,
                  (void *)temp_req, dma->tag);

      /*allocation read dma*/
      dma = (dma_info *)malloc(sizeof(dma_info));
      dma->tag = memio_alloc_dma(DMA_READ_BUF, &dma->data);

      temp_req = (async_bdbm_req *)malloc(sizeof(async_bdbm_req));
      temp_req->type = REQTYPE_IO_READ;
      temp_req->end_req = end_req;
      temp_req->private_data = (void *)dma;
      memio_read(mio, addr.lpn, 8192, (uint8_t *)dma->data, false,
                 (void *)temp_req, dma->tag);
    }
    addr.format.page = 0;
    /*trim for erase data*/
    memio_trim(mio, addr.lpn, 16384 * 8192, NULL);
  }
#if 0
    /* trim for badblock checking */
    memio_trim(mio, 0, 16384*8192, erase_end_req);
#endif

  memio_close(mio);
  return 0;
}
