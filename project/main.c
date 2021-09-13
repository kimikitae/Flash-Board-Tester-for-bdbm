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

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libmemio.h>

#include "include/lower.h"
#include "include/settings.h"

#define TARGET_SEGMENT (2)

static bool badsegment[MAX_BLOCKS];

static void erase_end_req(uint64_t seg_num, uint8_t isbad) {
  /*managing block mapping when "isbad" set to 1 which mean the segments has
   * some bad blocks*/
  if (isbad) {
    printf("bad segment detected %lu\n", seg_num);
    badsegment[seg_num] = true;
  }
}

static char buffer[PAGE_SIZE];

void generate_sample_data(char *buffer, int seed) {
  srand(seed * time(NULL));
  for (int i = 0; i < PAGE_SIZE; i++) {
    buffer[i] = rand() % CHAR_MAX;
  }
}

int main(int argc, char **argv) {
  memio_t *mio;
  struct address addr;

  memset(badsegment, false, sizeof(badsegment));

  if ((mio = memio_open()) == NULL) {
    printf("could not open memio\n");
    return -1;
  }
  printf("successfully open\n");
  // trim for badsegment checking
  addr.lpn = 0; addr.lpn = TARGET_SEGMENT * (1 << 13);
  memio_trim(mio, addr.lpn, PAGE_PER_SEGMENT * PAGE_SIZE, erase_end_req);
  printf("trim the bad segment\n");

  // remove whole
  addr.lpn = 0; addr.lpn = TARGET_SEGMENT * (1 << 13);
  memio_trim(mio, addr.lpn, PAGE_PER_SEGMENT * PAGE_SIZE, NULL);
  printf("remove the segment\n");

  // R/W tester
  printf("%-6s%-16s%-6s%-6s%-6s%-6s\n", "type", "lpn", "bus", "chip", "block",
         "page");
  flashinit();
  for (uint32_t i = 0; i < (1 << 13); i++) {
    addr.lpn = TARGET_SEGMENT*(1 << 13) + i;
    if (badsegment[addr.lpn / (1 << 13)]) {
      continue;
    }
    generate_sample_data(buffer, addr.lpn);
    flashwrite(mio, buffer, addr.lpn);
    flashread(mio, addr.lpn);
  }

  // remove whole
  addr.lpn = 0; addr.lpn = TARGET_SEGMENT * (1 << 13);
  memio_trim(mio, addr.lpn, PAGE_PER_SEGMENT * PAGE_SIZE, NULL);
  memio_wait(mio);
  for (uint32_t i = 0; i < (1 << 13); i++) {
    addr.lpn = TARGET_SEGMENT*(1 << 13) + i;
    if (badsegment[addr.lpn / (1 << 13)]) {
      continue;
    }
    flashread(mio, addr.lpn);
  }
  memio_wait(mio);


  memio_close(mio);
  flashfree();

  return 0;
}
