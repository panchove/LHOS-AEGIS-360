// gzip.h – Stub contractual GZIP compresión (Sprint 02, ODIN)
#ifndef GZIP_H
#define GZIP_H
#include <stddef.h>
#include <stdint.h>

// Comprime buffer src a dst (fake stub: sólo copia y marca)
size_t gzip_compress(const uint8_t* src, size_t len, uint8_t* dst, size_t dstcap);
// Descomprime buffer src a dst (fake stub: sólo copia)
size_t gzip_decompress(const uint8_t* src, size_t len, uint8_t* dst, size_t dstcap);

#endif // GZIP_H
