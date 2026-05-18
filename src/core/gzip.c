// gzip.c – Stub Sprint02: sin compresión real, solo marca
#include "gzip.h"
#include <string.h>
size_t gzip_compress(const uint8_t* src, size_t len, uint8_t* dst, size_t dstcap) {
    // Simplemente copia y pone 0x1F 0x8B header GZIP
    if (dstcap < len + 2) return 0;
    dst[0]=0x1F; dst[1]=0x8B; memcpy(dst+2,src,len); return len+2;
}
size_t gzip_decompress(const uint8_t* src, size_t len, uint8_t* dst, size_t dstcap) {
    // Copia si header GZIP (stub)
    if (len<2 || src[0]!=0x1F || src[1]!=0x8B) return 0;
    size_t cpy = len-2; if(cpy>dstcap) cpy=dstcap;
    memcpy(dst,src+2,cpy); return cpy;
}
