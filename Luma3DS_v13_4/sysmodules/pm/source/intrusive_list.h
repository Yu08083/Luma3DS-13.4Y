#pragma once

#include <stddef.h>
#include <stdbool.h>


typedef struct IntrusiveNode {
    struct IntrusiveNode *prev;
    struct IntrusiveNode *next;
} IntrusiveNode;


typedef union IntrusiveList {
    IntrusiveNode node;
    struct {
        IntrusiveNode *last;
        IntrusiveNode *first;
    };
} IntrusiveList;


static inline void IntrusiveList_Init(IntrusiveList *ll)
{
    ll->node.prev = &ll->node;
    ll->node.next = &ll->node;
}


static inline bool IntrusiveList_TestEnd(const IntrusiveList *ll, const IntrusiveNode *nd)
{
    return nd == &ll->node;
}


static inline void IntrusiveList_InsertAfter(IntrusiveNode *pos, IntrusiveNode *nd)
{

    pos->next->prev = nd;
    nd->prev = pos;
    nd->next = pos->next;
    pos->next = nd;
}


static inline void IntrusiveList_Erase(const IntrusiveNode *nd)
{
    nd->prev->next = nd->next;
    nd->next->prev = nd->prev;
}


static inline void IntrusiveList_CreateFromBuffer(IntrusiveList *ll, void *buf, size_t elemSize, size_t totalSize)
{
    IntrusiveList_Init(ll);
    for (size_t pos = 0; pos < totalSize; pos += elemSize) {
        IntrusiveList_InsertAfter(ll->last, (IntrusiveNode *)((char *)buf + pos));
    }
}
