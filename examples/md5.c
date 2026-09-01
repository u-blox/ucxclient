/*
 * Minimal MD5 (RFC 1321) implementation - little-endian targets only.
 * Based on the public-domain implementation by Colin Plumb (1993):
 * "This code is in the public domain; do with it what you wish."
 * Used by http_example.c on all targets (native and embedded).
 */
#include <string.h>
#include "md5.h"

static void md5Transform(uint32_t buf[4], const uint32_t in[16])
{
    uint32_t a = buf[0];
    uint32_t b = buf[1];
    uint32_t c = buf[2];
    uint32_t d = buf[3];

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))
#define MD5STEP(f, w, x, y, z, data, s) \
    (w += f(x, y, z) + (data), w = (w << (s)) | (w >> (32 - (s))), w += (x))

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478U, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756U, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070dbU, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceeeU, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0fafU, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62aU, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613U, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501U, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8U, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7afU, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1U, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7beU, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122U, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193U, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438eU, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821U, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562U, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340U, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51U, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aaU, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105dU, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453U, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681U, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8U, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6U, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6U, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87U, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14edU, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905U, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8U, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9U, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8aU, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942U, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681U, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122U, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380cU, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44U, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9U, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60U, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70U, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6U, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127faU, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085U, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05U, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039U, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5U, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8U, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665U, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244U, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97U, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7U, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039U, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3U, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92U, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47dU, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1U, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4fU, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0U, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314U, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1U, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82U, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235U, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bbU, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391U, 21);

#undef F1
#undef F2
#undef F3
#undef F4
#undef MD5STEP

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

void md5Init(Md5Ctx_t *pCtx)
{
    pCtx->buf[0] = 0x67452301U;
    pCtx->buf[1] = 0xefcdab89U;
    pCtx->buf[2] = 0x98badcfeU;
    pCtx->buf[3] = 0x10325476U;
    pCtx->bits[0] = 0;
    pCtx->bits[1] = 0;
}

void md5Update(Md5Ctx_t *pCtx, const uint8_t *pData, size_t len)
{
    uint32_t t = pCtx->bits[0];
    if ((pCtx->bits[0] = t + ((uint32_t)len << 3)) < t) {
        pCtx->bits[1]++;
    }
    pCtx->bits[1] += (uint32_t)(len >> 29);

    t = (t >> 3) & 0x3fU;
    if (t != 0) {
        uint8_t *pFill = pCtx->in + t;
        size_t space = 64 - t;
        if (len < space) {
            memcpy(pFill, pData, len);
            return;
        }
        memcpy(pFill, pData, space);
        md5Transform(pCtx->buf, (const uint32_t *)(const void *)pCtx->in);
        pData += space;
        len -= space;
    }

    while (len >= 64) {
        memcpy(pCtx->in, pData, 64);
        md5Transform(pCtx->buf, (const uint32_t *)(const void *)pCtx->in);
        pData += 64;
        len -= 64;
    }

    memcpy(pCtx->in, pData, len);
}

void md5Final(Md5Ctx_t *pCtx, uint8_t digest[16])
{
    uint32_t count = (pCtx->bits[0] >> 3) & 0x3fU;
    uint8_t *pPad = pCtx->in + count;

    *pPad++ = 0x80U;
    count = 64U - 1U - count;

    if (count < 8U) {
        memset(pPad, 0, count);
        md5Transform(pCtx->buf, (const uint32_t *)(const void *)pCtx->in);
        memset(pCtx->in, 0, 56);
    } else {
        memset(pPad, 0, count - 8U);
    }

    ((uint32_t *)(void *)pCtx->in)[14] = pCtx->bits[0];
    ((uint32_t *)(void *)pCtx->in)[15] = pCtx->bits[1];

    md5Transform(pCtx->buf, (const uint32_t *)(const void *)pCtx->in);
    memcpy(digest, pCtx->buf, 16);
    memset(pCtx, 0, sizeof(*pCtx));
}
