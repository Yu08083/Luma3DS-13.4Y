/*
memory.c

(c) TuxSH, 2017
This is part of 3ds_sm, which is licensed under the MIT license (see LICENSE for details).
*/

#include "memory.h"

 

s32 strnlen(const char *string, s32 maxlen)
{
    s32 size;
    for(size = 0; size < maxlen && *string; string++, size++);

    return size;
}
