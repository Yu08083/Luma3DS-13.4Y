#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "plguds_ipc.h"

typedef uint32_t u32;
typedef uint8_t u8;
typedef int32_t s32;
typedef int64_t s64;
typedef uint32_t Handle;
typedef int32_t Result;
typedef struct KAutoObject KAutoObject;
typedef struct { void (*DecrementReferenceCount)(KAutoObject *); } MockVtable;
struct KAutoObject { MockVtable *vtable; const char *name; unsigned references; };
typedef struct { KAutoObject autoObject; } MockSync;
typedef struct KSession KSession;
typedef struct KPort KPort;
typedef struct { KPort *parentKPort; } KClientPort;
struct KPort { struct { MockSync syncObject; } serverPort; };
typedef struct { MockSync syncObject; KSession *parentSession; KClientPort *clientPort; } KClientSession;
struct KSession { KAutoObject autoObject; struct { MockSync syncObject; } serverSession; KClientSession clientSession; };
typedef struct { int depth; } KObjectMutex;
typedef struct { KAutoObject *pointer; } HandleDescriptor;
typedef struct { HandleDescriptor *handleTable; int maxHandleCount; KObjectMutex mutex; HandleDescriptor internalTable[40]; } KProcessHandleTable;
typedef struct { unsigned unused; } KProcessHwInfo;
typedef struct { KAutoObject autoObject; u32 pid; s64 creationTime; unsigned status; KProcessHandleTable handles; KProcessHwInfo hardware; } KProcess;
typedef struct { void *threadLocalStorage; } MockThread;
typedef struct { KProcess *currentProcess; MockThread *currentThread; } MockObjects;
typedef struct { MockObjects objectContext; } MockCore;
typedef struct { KSession *session; char name[12]; } SessionInfo;
typedef struct { int depth; } KRecursiveLock;

static uint32_t tls[128];
static KProcess processes[2] = {{.pid=8, .creationTime=1234}, {.pid=9, .creationTime=5678}};
static MockThread currentThread = {tls};
static MockCore core = {{&processes[0], &currentThread}};
static MockCore *currentCoreContext = &core;
static KAutoObject *handles[256];
static KSession sessions[8];
static KPort port;
static KClientPort clientPort = {&port};
static SessionInfo infos[8];
static unsigned locks, native_calls, temp_handles, temp_closes, event_calls[2], soc_calls;
static bool fail_event, fail_create, fail_initialize;
static bool bad_srv_result, bad_srv_descriptor, bad_srv_shape;
static unsigned registrations, home_events, plugin_status = 1;
static uint32_t observed_header, observed_word, observed_pointer;
static char observed_service[9];
static unsigned nbSection0Modules = 6;
static bool hasStartedRosalinaNetworkFuncsOnce;
static unsigned kernelVersion;
static KRecursiveLock criticalLock;
static KRecursiveLock *criticalSectionLock = &criticalLock;
#define KPROCESS_GET_RVALUE(process, field) ((process)->field)
#define GET_VERSION_MINOR(version) (((version) >> 16) & 255)
#define PLG_CFG_RUNNING 1
#define PLG_CFG_INHOME 2
#define PLG_CFG_HOME_EVENT 65536
#define PROCESS_AVAILABLE 1

static void decrement(KAutoObject *object) { assert(object->references); object->references--; }
static MockVtable vtable = {decrement};
static void KAutoObject__AddReference(KAutoObject *object) { object->references++; }
static void KRecursiveLock__Lock(KRecursiveLock *lock) { lock->depth++; locks++; }
static void KRecursiveLock__Unlock(KRecursiveLock *lock) { assert(lock->depth > 0); lock->depth--; locks--; }
static void KObjectMutex__Acquire(KObjectMutex *lock) { lock->depth++; locks++; }
static void KObjectMutex__Release(KObjectMutex *lock) { assert(lock->depth > 0); lock->depth--; locks--; }
static const char *classNameOfAutoObject(KAutoObject *object) { return object->name; }
static KProcessHandleTable *handleTableOfProcess(KProcess *process) { return &process->handles; }
static KProcessHwInfo *hwInfoOfProcess(KProcess *process) { return &process->hardware; }
static u32 idOfProcess(KProcess *process) { return process->pid; }
static KAutoObject *KProcessHandleTable__ToKAutoObject(KProcessHandleTable *table, Handle handle)
{
    (void)table;
    KAutoObject *object = handle < 256 ? handles[handle] : NULL;
    if (object) KAutoObject__AddReference(object);
    return object;
}
static u32 KProcessHwInfo__GetAddressUserPerm(KProcessHwInfo *hardware, u32 address)
{
    (void)hardware;
    return address >= 0x00100000 && address < 0x00300000 ? 3 : address >= 0x00300000 && address < 0x00400000 ? 1 : 0;
}
static SessionInfo *SessionInfo_Lookup(KSession *session)
{
    for (unsigned index = 0; index < 8; index++) if (infos[index].session == session) return &infos[index];
    return NULL;
}
static void SessionInfo_Add(KSession *session, const char *name)
{
    registrations++;
    SessionInfo *info = SessionInfo_Lookup(session);
    assert(info);
    strncpy(info->name, name, 11);
}
static Result createHandleForThisProcess(Handle *output, KAutoObject *object)
{
    if (fail_create) return -12;
    assert(!handles[200]);
    handles[200] = object;
    *output = 200;
    temp_handles++;
    return 0;
}
static Result CloseHandle(Handle handle)
{
    assert(handle == 200 && handles[handle]);
    handles[handle] = NULL;
    temp_closes++;
    return 0;
}
static Result SendSyncRequest(Handle handle)
{
    assert(!locks);
    native_calls++;
    uint32_t *command = tls + 32;
    unsigned id = command[0] >> 16;
    observed_header = command[0];
    observed_word = command[1];
    observed_pointer = command[4];
    if (handle == 200)
    {
        assert(id == 102 && command[1] <= 1);
        event_calls[command[1]]++;
        for (unsigned index = 0; index < 96; index++) command[index] = 0xCACACA00 + index;
        command[0] = PlgUdsHeader(102, 1, 0);
        command[1] = 0;
        return fail_event ? -13 : 0;
    }
    if (handle == 2 && id == 5)
    {
        memcpy(observed_service, command + 1, 8);
        observed_service[8] = 0;
        command[0] = bad_srv_shape ? PlgUdsHeader(5, 0, 0) : PlgUdsHeader(5, 1, 2);
        command[1] = bad_srv_result ? (uint32_t)-19 : 0;
        command[2] = bad_srv_descriptor ? 0x20 : 0;
        command[3] = 7;
        return 0;
    }
    if (handle == 4) soc_calls++;
    command[0] = PlgUdsHeader(id, 1, 0);
    command[1] = fail_initialize && id == 100 ? (uint32_t)-15 : 0;
    return 0;
}
static bool doLangEmu(Result *result, u32 *command) { (void)result; (void)command; return false; }
static bool doErrfThrowHook(u32 *command) { (void)command; return false; }
static void PLG__WakeAppThread(void) {}
static void PLG_SignalEvent(unsigned event) { assert(event == PLG_CFG_HOME_EVENT); home_events++; }
static u32 PLG_GetStatus(void) { return plugin_status; }
