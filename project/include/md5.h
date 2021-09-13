#ifndef MD5_H
#define MD5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/md5.h>

// sizeof(result) must be equal to the MD5_DIGEST_LENGTH
int md5get(void *data, unsigned char *digest, const size_t size);
char *get_md5_string(unsigned char*digest);

#ifdef __cplusplus
}
#endif
#endif
