#include <assert.h>
#include <libmemio.h>
#include <map>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/lower.h"
#include "include/md5.h"
#include "include/settings.h"

static bool check_md5(unsigned char *d1, unsigned char *d2) {
  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    if (d1[i] != d2[i]) {
      return false;
    }
  }
  return true;
}

static void end_req(async_bdbm_req *req) {
  dma_info *dma = (dma_info *)req->private_data;
  struct address addr;
  addr.lpn = dma->lpn;

  switch (req->type) {
  case REQTYPE_IO_READ:
    unsigned char digest[MD5_DIGEST_LENGTH];
    memset(digest, 0, sizeof(digest));
    assert(0 == md5get(dma->data, digest, PAGE_SIZE));
    printf("%-6s%-16u%-6u%-6u%-6u%-6u%s\n", "R", addr.lpn, addr.format.bus,
           addr.format.chip, addr.format.block, addr.format.page,
           get_md5_string(digest));
#if 0
    if (dma->data != NULL && !check_md5(digest, info[dma->tag].digest)) {
      printf("%-6s%-016u", "read", info[dma->tag].lpn);
      printf("%s\n", get_md5_string(info[dma->tag].digest));
    } 
    fflush(stdout);
#endif
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

void flashinit(void) {}

void flashwrite(memio_t *mio, char *buffer, uint32_t lpn) {
  /*allocation write dma*/
  dma_info *dma = (dma_info *)malloc(sizeof(dma_info));
  dma->lpn = lpn;
  dma->tag = memio_alloc_dma(DMA_WRITE_BUF, &dma->data);
  memcpy(dma->data, buffer, PAGE_SIZE);
  unsigned char digest[MD5_DIGEST_LENGTH];
  memset(digest, 0, sizeof(digest));
  assert(0 == md5get(dma->data, digest, PAGE_SIZE));
  struct address addr;
  addr.lpn = lpn;
  printf("%-6s%-16u%-6u%-6u%-6u%-6u%s\n", "W", addr.lpn, addr.format.bus,
         addr.format.chip, addr.format.block, addr.format.page,
         get_md5_string(digest));

  async_bdbm_req *req = (async_bdbm_req *)malloc(sizeof(async_bdbm_req));
  req->type = REQTYPE_IO_WRITE;
  req->private_data = (void *)dma;
  req->end_req = end_req; // when the requset ends, the "end_req" is called

  memio_write(mio, lpn, PAGE_SIZE, (uint8_t *)dma->data, false, (void *)req,
              dma->tag);
}

void flashread(memio_t *mio, uint32_t lpn) {
  /*allocation read dma*/
  dma_info *dma = (dma_info *)malloc(sizeof(dma_info));
  dma->tag = memio_alloc_dma(DMA_READ_BUF, &dma->data);
  dma->lpn = lpn;

  async_bdbm_req *req = (async_bdbm_req *)malloc(sizeof(async_bdbm_req));
  req->type = REQTYPE_IO_READ;
  req->end_req = end_req;
  req->private_data = (void *)dma;
  memio_read(mio, lpn, PAGE_SIZE, (uint8_t *)dma->data, false, (void *)req,
             dma->tag);
}

void flashfree() {}
