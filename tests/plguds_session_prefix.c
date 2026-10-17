#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint32_t u32;
typedef struct { void **vtable; } KAutoObject;
typedef struct { KAutoObject autoObject; } KSession;
typedef struct { KSession *session; char name[12]; } SessionInfo;
typedef struct { int depth; } KRecursiveLock;
#define MAX_SESSION 345
static SessionInfo sessionInfos[MAX_SESSION];
static u32 nbActiveSessions;
static KRecursiveLock sessionInfosLock, criticalLock;
static KRecursiveLock *criticalSectionLock = &criticalLock;
static void *customSessionVtable[16];
static void KRecursiveLock__Lock(KRecursiveLock *lock) { lock->depth++; }
static void KRecursiveLock__Unlock(KRecursiveLock *lock) { assert(lock->depth); lock->depth--; }
