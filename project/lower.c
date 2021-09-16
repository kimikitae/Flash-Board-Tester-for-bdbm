#include <assert.h>
#include <libmemio.h>
#include <map>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <openssl/md5.h>

#include "include/lower.h"
#include "include/md5.h"
#include "include/settings.h"

static MD5_CTX wctx[MAX_BLOCKS];
static MD5_CTX rctx[MAX_BLOCKS];
static uint32_t last_block = 0;
static uint32_t next_block = 0;
static FILE *fp;
static pthread_mutex_t mutex;

static void end_req(async_bdbm_req *req) {
  dma_info *dma = (dma_info *)req->private_data;
  struct address addr;
  addr.lpn = dma->lpn;

  switch (req->type) {
  case REQTYPE_IO_READ:
    /*do something after read req*/
    assert(0 != MD5_Update(&rctx[addr.format.block], dma->data, PAGE_SIZE));
    last_block = last_block < addr.format.block ? addr.format.block : last_block;
    memio_free_dma(DMA_READ_BUF, dma->tag);
    pthread_mutex_unlock(&mutex);
    break;
  case REQTYPE_IO_WRITE:
    /*do something after write req*/
    memio_free_dma(DMA_WRITE_BUF, dma->tag);
    pthread_mutex_unlock(&mutex);
    break;
  default:
    break;
  }
  free(dma);
  free(req);
}

void flashinit(void) {
  pthread_mutex_init(&mutex, NULL);
  assert(NULL != (fp = fopen("crashblock.log", "w")));
  for (int i = 0; i < MAX_BLOCKS; i++) {
    assert(0 != MD5_Init(&wctx[i]));
    assert(0 != MD5_Init(&rctx[i]));
  }
}

void flashwrite(memio_t *mio, char *buffer, uint32_t lpn) {
  /*allocation write dma*/
  dma_info *dma = (dma_info *)malloc(sizeof(dma_info));
  dma->lpn = lpn;
  dma->tag = memio_alloc_dma(DMA_WRITE_BUF, &dma->data);
  memcpy(dma->data, buffer, PAGE_SIZE);

  struct address addr;
  addr.lpn = lpn;
  assert(0 != MD5_Update(&wctx[addr.format.block], buffer, PAGE_SIZE));

  async_bdbm_req *req = (async_bdbm_req *)malloc(sizeof(async_bdbm_req));
  req->type = REQTYPE_IO_WRITE;
  req->private_data = (void *)dma;
  req->end_req = end_req; // when the requset ends, the "end_req" is called

  pthread_mutex_lock(&mutex);
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
  pthread_mutex_lock(&mutex);
  memio_read(mio, lpn, PAGE_SIZE, (uint8_t *)dma->data, false, (void *)req,
             dma->tag);
}

void flashupdate(const bool *badsegment) {
  unsigned char wdigest[MD5_DIGEST_LENGTH];
  unsigned char rdigest[MD5_DIGEST_LENGTH];
  for (uint32_t i = next_block; i < last_block; i++) {
    if (badsegment[i]) {
      next_block = i + 1;
      continue;
    }
    assert(0 != MD5_Final(wdigest, &wctx[i]));
    assert(0 != MD5_Final(rdigest, &rctx[i]));
    int diff = memcmp(wdigest, rdigest, sizeof(wdigest));
    if (diff != 0) {
      fprintf(fp, "%u %d\n", i, diff);
      printf("fail %s %s\n",get_md5_string(wdigest), get_md5_string(rdigest));
      fflush(fp);
    } else {
      printf("pass %s %s\n",get_md5_string(wdigest), get_md5_string(rdigest));
      fflush(fp);
    }
    next_block = i + 1;
  }
}

void flashfree(const bool *badsegment) {
  flashupdate(badsegment);
  pthread_mutex_destroy(&mutex);
  fclose(fp);
}
