// blackbox.c – API contractual heapless/logging Sprint 02 (stub QA)
#include "blackbox.h"
#include "gzip.h"
#include <string.h>
#define BB_MAX 16
#define BB_SZ  256
static uint8_t storage[BB_MAX][BB_SZ];
static size_t  sizes[BB_MAX];
static size_t  count=0, cursor=0;

int blackbox_enqueue(const uint8_t* data, size_t len) {
    if (count>=BB_MAX) return -1; // full
    uint8_t gz[BB_SZ];
    size_t gzlen=gzip_compress(data,len,gz,sizeof(gz));
    if (!gzlen) return -2;
    memcpy(storage[count],gz,gzlen); sizes[count]=gzlen; ++count;
    return 0;
}
int blackbox_read_next(uint8_t* out_buf, size_t* out_len) {
    if (cursor>=count) return -1;
    size_t rlen=gzip_decompress(storage[cursor],sizes[cursor],out_buf,*out_len);
    if (!rlen) return -2;
    *out_len = rlen;
    ++cursor;
    return 0;
}
size_t blackbox_get_stats(void) { return count; }
