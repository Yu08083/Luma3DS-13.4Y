#include "sha256.h"

static const u32 g_sha256K[64] =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

static u32 Sha256_Rotr(u32 x, u32 n)
{
    return (x >> n) | (x << (32 - n));
}

static void Sha256_Block(Sha256 *ctx, const u8 *p)
{
    u32 w[64], s[8];

    for (u32 i = 0; i < 16; ++i)
        w[i] = ((u32)p[i * 4] << 24) | ((u32)p[i * 4 + 1] << 16) | ((u32)p[i * 4 + 2] << 8) | p[i * 4 + 3];

    for (u32 i = 16; i < 64; ++i)
    {
        u32 s0 = Sha256_Rotr(w[i - 15], 7) ^ Sha256_Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        u32 s1 = Sha256_Rotr(w[i - 2], 17) ^ Sha256_Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    for (u32 i = 0; i < 8; ++i)
        s[i] = ctx->state[i];

    for (u32 i = 0; i < 64; ++i)
    {
        u32 t1 = s[7] + (Sha256_Rotr(s[4], 6) ^ Sha256_Rotr(s[4], 11) ^ Sha256_Rotr(s[4], 25))
               + ((s[4] & s[5]) ^ (~s[4] & s[6])) + g_sha256K[i] + w[i];
        u32 t2 = (Sha256_Rotr(s[0], 2) ^ Sha256_Rotr(s[0], 13) ^ Sha256_Rotr(s[0], 22))
               + ((s[0] & s[1]) ^ (s[0] & s[2]) ^ (s[1] & s[2]));

        for (u32 j = 7; j > 0; --j)
            s[j] = s[j - 1];
        s[4] += t1;
        s[0] = t1 + t2;
    }

    for (u32 i = 0; i < 8; ++i)
        ctx->state[i] += s[i];
}

void Sha256_Init(Sha256 *ctx)
{
    static const u32 init[8] =
    {
        0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
    };

    for (u32 i = 0; i < 8; ++i)
        ctx->state[i] = init[i];
    ctx->length = 0;
    ctx->used = 0;
}

void Sha256_Update(Sha256 *ctx, const void *data, u32 size)
{
    const u8 *p = (const u8 *)data;

    ctx->length += size;
    while (size != 0)
    {
        if (ctx->used == 0 && size >= 64)
        {
            Sha256_Block(ctx, p);
            p += 64;
            size -= 64;
            continue;
        }

        ctx->block[ctx->used++] = *p++;
        --size;

        if (ctx->used == 64)
        {
            Sha256_Block(ctx, ctx->block);
            ctx->used = 0;
        }
    }
}

void Sha256_Final(Sha256 *ctx, u8 *digest)
{
    u64 bits = ctx->length * 8;

    ctx->block[ctx->used++] = 0x80;
    if (ctx->used > 56)
    {
        while (ctx->used < 64)
            ctx->block[ctx->used++] = 0;
        Sha256_Block(ctx, ctx->block);
        ctx->used = 0;
    }

    while (ctx->used < 56)
        ctx->block[ctx->used++] = 0;
    for (u32 i = 0; i < 8; ++i)
        ctx->block[56 + i] = (u8)(bits >> (56 - i * 8));
    Sha256_Block(ctx, ctx->block);

    for (u32 i = 0; i < 32; ++i)
        digest[i] = (u8)(ctx->state[i / 4] >> (24 - (i % 4) * 8));
}
