#include "md5.h"

static char *make_digest(const unsigned char *digest, int len) /* {{{ */
{
    int md5len = sizeof(char) * (len * 2 + 1);
    char *md5str = (char *) calloc(1, md5len);
    static const char hexits[17] = "0123456789abcdef";
    int i;

    for (i = 0; i < len; i++)
    {
        md5str[i * 2]       = hexits[digest[i] >> 4];
        md5str[(i * 2) + 1] = hexits[digest[i] &  0x0F];
    }

    return md5str;
}

/*
 * The basic MD5 functions.
 *
 * F and G are optimized compared to their RFC 1321 definitions for
 * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
 * implementation.
 */
#define F(x, y, z)          ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)          ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)          ((x) ^ (y) ^ (z))
#define I(x, y, z)          ((y) ^ ((x) | ~(z)))

/*
 * The MD5 transformation for all four rounds.
 */
#define STEP(f, a, b, c, d, x, t, s) \
    (a) += f((b), (c), (d)) + (x) + (t); \
    (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
    (a) += (b);

/*
 * SET reads 4 input bytes in little-endian byte order and stores them
 * in a properly aligned word in host byte order.
 *
 * The check for little-endian architectures that tolerate unaligned
 * memory accesses is just an optimization.  Nothing will break if it
 * doesn't work.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
# define SET(n) \
    (*(MD5_u32plus *)&ptr[(n) * 4])
# define GET(n) \
    SET(n)
#else
# define SET(n) \
    (ctx->block[(n)] = \
    (MD5_u32plus)ptr[(n) * 4] | \
    ((MD5_u32plus)ptr[(n) * 4 + 1] << 8) | \
    ((MD5_u32plus)ptr[(n) * 4 + 2] << 16) | \
    ((MD5_u32plus)ptr[(n) * 4 + 3] << 24))
# define GET(n) \
    (ctx->block[(n)])
#endif

/*
 * This processes one or more 64-byte data blocks, but does NOT update
 * the bit counters.  There are no alignment requirements.
 */
static const void *body(void *ctxBuf, const void *data, size_t size)
{
    MD5_CTX *ctx = (MD5_CTX *)ctxBuf;
    const unsigned char *ptr;
    MD5_u32plus a, b, c, d;

    ptr = (unsigned char *)data;

    a = ctx->a;
    b = ctx->b;
    c = ctx->c;
    d = ctx->d;

    do
    {
        MD5_u32plus saved_a, saved_b, saved_c, saved_d;
        saved_a = a;
        saved_b = b;
        saved_c = c;
        saved_d = d;

        /* Round 1 */
        STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
        STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
        STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
        STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
        STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
        STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
        STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
        STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
        STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
        STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
        STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
        STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
        STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
        STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
        STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
        STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

        /* Round 2 */
        STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
        STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
        STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
        STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
        STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
        STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
        STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
        STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
        STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
        STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
        STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
        STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
        STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
        STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
        STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
        STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

        /* Round 3 */
        STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
        STEP(H, d, a, b, c, GET(8), 0x8771f681, 11)
        STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
        STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23)
        STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
        STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11)
        STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
        STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23)
        STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
        STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11)
        STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
        STEP(H, b, c, d, a, GET(6), 0x04881d05, 23)
        STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
        STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11)
        STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
        STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23)

        /* Round 4 */
        STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
        STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
        STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
        STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
        STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
        STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
        STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
        STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
        STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
        STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
        STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
        STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
        STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
        STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
        STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
        STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

        a += saved_a;
        b += saved_b;
        c += saved_c;
        d += saved_d;

        ptr += 64;
    }
    while (size -= 64);

    ctx->a = a;
    ctx->b = b;
    ctx->c = c;
    ctx->d = d;

    return ptr;
}

static void MD5Init(void *ctxBuf)
{
    MD5_CTX *ctx = (MD5_CTX *)ctxBuf;
    ctx->a = 0x67452301;
    ctx->b = 0xefcdab89;
    ctx->c = 0x98badcfe;
    ctx->d = 0x10325476;

    ctx->lo = 0;
    ctx->hi = 0;
}

static void MD5Update(void *ctxBuf, const void *data, size_t size)
{
    MD5_CTX *ctx = (MD5_CTX *)ctxBuf;
    MD5_u32plus saved_lo;
    MD5_u32plus used;

    saved_lo = ctx->lo;

    if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
    {
        ctx->hi++;
    }

    ctx->hi += size >> 29;

    used = saved_lo & 0x3f;

    if (used)
    {
        MD5_u32plus free;
        free = 64 - used;

        if (size < free)
        {
            memcpy(&ctx->buffer[used], data, size);
            return;
        }

        memcpy(&ctx->buffer[used], data, free);
        data = (unsigned char *)data + free;
        size -= free;
        body(ctx, ctx->buffer, 64);
    }

    if (size >= 64)
    {
        data = body(ctx, data, size & ~(size_t)0x3f);
        size &= 0x3f;
    }

    memcpy(ctx->buffer, data, size);
}

static void MD5Final(unsigned char *result, void *ctxBuf)
{
    MD5_CTX *ctx = (MD5_CTX *)ctxBuf;
    MD5_u32plus used, free;

    used = ctx->lo & 0x3f;

    ctx->buffer[used++] = 0x80;

    free = 64 - used;

    if (free < 8)
    {
        memset(&ctx->buffer[used], 0, free);
        body(ctx, ctx->buffer, 64);
        used = 0;
        free = 64;
    }

    memset(&ctx->buffer[used], 0, free - 8);

    ctx->lo <<= 3;
    ctx->buffer[56] = ctx->lo;
    ctx->buffer[57] = ctx->lo >> 8;
    ctx->buffer[58] = ctx->lo >> 16;
    ctx->buffer[59] = ctx->lo >> 24;
    ctx->buffer[60] = ctx->hi;
    ctx->buffer[61] = ctx->hi >> 8;
    ctx->buffer[62] = ctx->hi >> 16;
    ctx->buffer[63] = ctx->hi >> 24;

    body(ctx, ctx->buffer, 64);

    result[0] = ctx->a;
    result[1] = ctx->a >> 8;
    result[2] = ctx->a >> 16;
    result[3] = ctx->a >> 24;
    result[4] = ctx->b;
    result[5] = ctx->b >> 8;
    result[6] = ctx->b >> 16;
    result[7] = ctx->b >> 24;
    result[8] = ctx->c;
    result[9] = ctx->c >> 8;
    result[10] = ctx->c >> 16;
    result[11] = ctx->c >> 24;
    result[12] = ctx->d;
    result[13] = ctx->d >> 8;
    result[14] = ctx->d >> 16;
    result[15] = ctx->d >> 24;

    memset(ctx, 0, sizeof(*ctx));
}

static unsigned char *make_hash(const char *arg)
{
    MD5_CTX context;
    static unsigned char digest[65];
    MD5Init(&context);
    MD5Update(&context, arg, strlen(arg));
    MD5Final(digest, &context);
    return digest;
}

char *rig_make_md5(const char *pass)
{
    const unsigned char *hash = make_hash(pass);
    char *md5str = make_digest(hash, 16);
    return md5str;
}

//#define TEST
#ifdef TEST
#include <stdio.h>
int main()
{
    const char *md5str = make_md5("password");
    printf("md5=%s\n", md5str);
    return 0;
}
#endif
