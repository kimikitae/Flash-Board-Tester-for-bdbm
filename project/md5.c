#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/md5.h"
#include "include/settings.h"

static MD5_CTX lctx;
static char buffer[PAGE_SIZE];

int md5get(void *data, unsigned char *digest, const size_t size) {
  assert(NULL != digest);
  if (0 == MD5_Init(&lctx)) {
    perror("md5 initialize failed");
    return -1;
  }

  if (0 == MD5_Update(&lctx, data, size)) {
    perror("md5 update failed");
    return -1;
  }

  if (0 == MD5_Final(digest, &lctx)) {
    perror("md5 finalize failed");
    return -1;
  }
  return 0;
}

char *get_md5_string(unsigned char *digest) {
  char *md5msg = buffer;
  md5msg = (char *)malloc(sizeof(char) * 64);
  for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    sprintf(md5msg + (i * 2), "%02x", digest[i]);
  }
  return md5msg;
}
