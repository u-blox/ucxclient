/*
 * Minimal MD5 (RFC 1321) implementation - little-endian targets only.
 * Based on the public-domain implementation by Colin Plumb (1993).
 * Used by http_example.c on all targets (native and embedded).
 */
#ifndef MD5_H
#define MD5_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t buf[4];
    uint32_t bits[2];
    uint8_t  in[64];
} Md5Ctx_t;

void md5Init(Md5Ctx_t *pCtx);
void md5Update(Md5Ctx_t *pCtx, const uint8_t *pData, size_t len);
void md5Final(Md5Ctx_t *pCtx, uint8_t digest[16]);

#endif // MD5_H
