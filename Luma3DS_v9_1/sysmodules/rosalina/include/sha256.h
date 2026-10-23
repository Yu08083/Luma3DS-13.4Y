#pragma once

#include <3ds/types.h>

typedef struct Sha256
{
    u32 state[8];
    u64 length;
    u8  block[64];
    u32 used;
} Sha256;

void Sha256_Init(Sha256 *ctx);
void Sha256_Update(Sha256 *ctx, const void *data, u32 size);
void Sha256_Final(Sha256 *ctx, u8 *digest);
