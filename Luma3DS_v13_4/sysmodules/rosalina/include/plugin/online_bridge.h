#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint32_t magic[2], abi, bytes;
    uint32_t requestSeq, requestedEnabled, acknowledgedSeq, state, error;
    uint32_t reserved[7];
} OnlineBridgeMailbox;

typedef struct {
    uint32_t requestSeq, requestedEnabled, acknowledgedSeq, state, error;
} OnlineBridgeStatus;

_Static_assert(sizeof(OnlineBridgeMailbox) == 64, "Invalid online bridge size");

static inline bool OnlineBridge_Valid(const volatile OnlineBridgeMailbox *mailbox)
{
    return mailbox && !((uintptr_t)mailbox & 3u) && mailbox->magic[0] == 0x3635324Cu && mailbox->magic[1] == 0x3158424Du
        && mailbox->abi == 1 && mailbox->bytes == sizeof(*mailbox);
}

static inline volatile OnlineBridgeMailbox *OnlineBridge_Find(void *memory, uint32_t size)
{
    if(!memory || ((uintptr_t)memory & 3u) || size < 0x140) return NULL;
    uint32_t minimum = size > 0x1100 ? size - 0x1000 : 0x100;
    uint32_t offset = (size - sizeof(OnlineBridgeMailbox)) & ~15u;
    while(offset >= minimum)
    {
        volatile OnlineBridgeMailbox *candidate = (volatile OnlineBridgeMailbox *)((uint8_t *)memory + offset);
        if(OnlineBridge_Valid(candidate)) return candidate;
        if(offset < 16) break;
        offset -= 16;
    }
    return NULL;
}

bool MemoryBlock__OnlineBridgeGet(OnlineBridgeStatus *status);
bool MemoryBlock__OnlineBridgeSet(bool enabled);
