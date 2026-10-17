#pragma once

#include <3ds/types.h>
#include <3ds/result.h>
#include <3ds/os.h>
#include <3ds/srv.h>

#define REG32(reg)              (*(vu32 *)reg)
#define REG64(reg)              (*(vu64 *)reg)

#define TRY(expr)               if(R_FAILED(res = (expr))) return res;
#define TRYG(expr, label)       if(R_FAILED(res = (expr))) goto label;

typedef struct Ncch {
    u8 sig[0x100];
    char magic[4];
    u32 contentSize;
    u8 partitionId[8];
    u8 makerCode[2];
    u16 version;
    u8 reserved1[4];
    u8 programID[8];
    u8 reserved2[0x10];
    u8 logoHash[0x20];
    char productCode[0x10];
    u8 exHeaderHash[0x20];
    u32 exHeaderSize;
    u32 reserved3;
    u8 flags[8];
    u32 plainOffset;
    u32 plainSize;
    u32 logoOffset;
    u32 logoSize;
    u32 exeFsOffset;
    u32 exeFsSize;
    u32 exeFsHashSize;
    u32 reserved4;
    u32 romFsOffset;
    u32 romFsSize;
    u32 romFsHashSize;
    u32 reserved5;
    u8 exeFsHash[0x20];
    u8 romFsHash[0x20];
} Ncch;

typedef struct ExeFsFileHeader {
    char name[8];
    u32 offset;
    u32 size;
} ExeFsFileHeader;

typedef struct ExeFsHeader {
    ExeFsFileHeader fileHeaders[10];
    u8 _reserved_0xa0[0xC0 - 0xA0];
    u8 fileHashes[10][32];
} ExeFsHeader;

#ifdef XDS
static void hexItoa(u64 number, char *out, u32 digits, bool uppercase)
{
    const char hexDigits[] = "0123456789ABCDEF";
    const char hexDigitsLowercase[] = "0123456789abcdef";
    u32 i = 0;

    while(number > 0)
    {
        out[digits - 1 - i++] = uppercase ? hexDigits[number & 0xF] : hexDigitsLowercase[number & 0xF];
        number >>= 4;
    }

    while(i < digits) out[digits - 1 - i++] = '0';
}

static inline void debugOutputHex(u64 number, u32 digits)
{
    char buf[16+2];
    hexItoa(number, buf, digits, false);
    buf[digits] = '\n';
    buf[digits + 1] = '\0';

    svcOutputDebugString(buf, digits + 1);
}

#endif

static void __attribute__((noinline)) panic(Result res)
{
#ifndef XDS
    (void)res;
    __builtin_trap();
#else
    debugOutputHex(res, 8);
    svcBreak(USERBREAK_PANIC);
#endif
}

static inline Result assertSuccess(Result res)
{
    if(R_FAILED(res)) {
        panic(res);
    }

    return res;
}
