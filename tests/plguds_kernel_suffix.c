static void setup(void)
{
    PlgUdsResetProcess(&processes[0]);
    PlgUdsResetProcess(&processes[1]);
    memset(tls, 0, sizeof(tls));
    memset(handles, 0, sizeof(handles));
    memset(sessions, 0, sizeof(sessions));
    memset(infos, 0, sizeof(infos));
    native_calls = temp_handles = temp_closes = event_calls[0] = event_calls[1] = soc_calls = 0;
    fail_event = fail_create = fail_initialize = false;
    bad_srv_result = bad_srv_descriptor = bad_srv_shape = false;
    registrations = home_events = 0;
    plugin_status = 1;
    core.objectContext.currentProcess = &processes[0];
    for (unsigned index = 0; index < 2; index++)
    {
        processes[index].autoObject = (KAutoObject){&vtable, "KProcess", 1};
        processes[index].status = PROCESS_AVAILABLE;
    }
    const char *names[8] = {"plg:UDS", "srv:", "ndm:u", "soc:U", "unrelated", "plg:UDS", "plg:UDS", "other"};
    for (unsigned index = 0; index < 8; index++)
    {
        sessions[index].autoObject = (KAutoObject){&vtable, "KSession", 1};
        sessions[index].serverSession.syncObject.autoObject = (KAutoObject){&vtable, "KServerSession", 1};
        sessions[index].clientSession.syncObject.autoObject = (KAutoObject){&vtable, "KClientSession", 1};
        sessions[index].clientSession.parentSession = &sessions[index];
        handles[index + 1] = &sessions[index].clientSession.syncObject.autoObject;
        infos[index].session = &sessions[index];
        strcpy(infos[index].name, names[index]);
    }
    processes[0].handles = (KProcessHandleTable){.maxHandleCount=40};
    processes[1].handles = (KProcessHandleTable){.maxHandleCount=40};
    processes[0].handles.internalTable[0].pointer = &sessions[0].serverSession.syncObject.autoObject;
    processes[1].handles.internalTable[0].pointer = &sessions[5].serverSession.syncObject.autoObject;
}
static void request(unsigned id, unsigned normal, unsigned translate)
{
    memset(tls + 32, 0, 96 * sizeof(uint32_t));
    tls[32] = PlgUdsHeader(id, normal, translate);
}
static void initialize(void)
{
    request(100, 2, 0); tls[33] = 8;
    assert(SendSyncRequestHook(1) == 0 && tls[33] == 0);
    assert(PlgUdsSnapshot(&processes[0]).active);
    assert(sessions[0].autoObject.references == 2);
}
static void redirect(bool active)
{
    request(5, 4, 0); memcpy(tls + 33, "nwm::UDS", 8); tls[35] = 8;
    assert(SendSyncRequestHook(2) == 0);
    assert(!strcmp(observed_service, active ? "plg:UDS" : "nwm::UDS"));
}
int main(void)
{
    setup();
    redirect(false);
    initialize();
    redirect(true);
    request(105, 1, 0); tls[33] = 1;
    assert(SendSyncRequestHook(1) == 0 && !tls[33]);
    redirect(false);
    request(105, 1, 0); tls[33] = 0;
    assert(SendSyncRequestHook(1) == 0 && !tls[33]);
    redirect(true);
    core.objectContext.currentProcess = &processes[1];
    redirect(false);
    request(100, 2, 0); tls[33] = 9;
    assert(SendSyncRequestHook(6) == 0 && (s32)tls[33] < 0);
    assert(!PlgUdsSnapshot(&processes[1]).active && PlgUdsSnapshot(&processes[0]).active);
    core.objectContext.currentProcess = &processes[0];
    request(17, 1, 0); tls[33] = 16; tls[96] = (16 << 14) | 2; tls[97] = 0x00300000;
    unsigned calls = native_calls;
    assert(SendSyncRequestHook(1) == 0 && (s32)tls[33] < 0 && native_calls == calls);
    request(20, 3, 0); tls[34] = 16; tls[35] = 64; tls[96] = (64 << 14) | 2; tls[97] = 0x00201000;
    assert(SendSyncRequestHook(1) == 0 && observed_header == PlgUdsHeader(20, 4, 0) && observed_pointer == 0x00201000);
    request(104, 1, 0); tls[33] = 1;
    assert(SendSyncRequestHook(1) == 0);
    request(1, 1, 4); tls[33] = 0x1234;
    uint32_t before[128]; memcpy(before, tls, sizeof(before));
    assert(SendSyncRequestHook(4) == 0 && event_calls[0] == 1 && soc_calls == 1);
    assert(temp_handles == 1 && temp_closes == 1);
    assert(!memcmp(tls + 96, before + 96, 32 * sizeof(uint32_t)));
    assert(tls[32] == PlgUdsHeader(1, 1, 0));
    request(25, 0, 0);
    assert(SendSyncRequestHook(4) == 0 && event_calls[1] == 1 && temp_handles == temp_closes);
    request(101, 0, 0);
    assert(SendSyncRequestHook(1) == 0 && !PlgUdsSnapshot(&processes[0]).active);
    assert(sessions[0].autoObject.references == 1);
    redirect(false);

    setup(); fail_initialize = true;
    request(100, 2, 0); tls[33] = 8;
    assert(SendSyncRequestHook(1) == 0 && !PlgUdsSnapshot(&processes[0]).active);
    assert(sessions[0].autoObject.references == 1);
    setup(); request(100, 2, 0); tls[33] = 9;
    assert(SendSyncRequestHook(1) == 0 && (s32)tls[33] < 0 && !native_calls);
    setup(); initialize();
    request(3, 0, 0);
    unsigned initial_calls = native_calls;
    assert(SendSyncRequestHook(7) == 0 && (s32)tls[33] < 0 && native_calls == initial_calls);
    PlgUdsResetProcess(&processes[0]);
    for (unsigned failure = 0; failure < 3; failure++)
    {
        setup();
        bad_srv_result = failure == 0;
        bad_srv_descriptor = failure == 1;
        bad_srv_shape = failure == 2;
        request(5, 4, 0); memcpy(tls + 33, "plg:UDS", 8); tls[35] = 7;
        assert(SendSyncRequestHook(2) == 0 && registrations == 0);
        if (failure == 0) assert((s32)tls[33] == -19 && tls[35] == 7);
    }
    setup();
    strcpy(infos[7].name, "APT:U");
    plugin_status = 2;
    const unsigned notifications[] = {0, 3, 9, 10, 11, 12, 13, 99, 0xFFFFFFFF};
    for (unsigned index = 0; index < sizeof(notifications) / sizeof(notifications[0]); index++)
    {
        unsigned count = home_events;
        request(13, 2, 0); tls[33] = 0x300; tls[35] = notifications[index];
        assert(SendSyncRequestHook(8) == 0);
        assert(home_events == count + (notifications[index] >= 10 && notifications[index] <= 12));
    }
    plugin_status = 1;
    request(13, 2, 0); tls[33] = 0x300; tls[35] = 3;
    unsigned count = home_events;
    assert(SendSyncRequestHook(8) == 0 && home_events == count + 1);
    setup(); initialize();
    assert(processes[0].autoObject.references == 2);
    processes[0].status = 3;
    core.objectContext.currentProcess = &processes[1];
    request(100, 2, 0); tls[33] = 9;
    assert(SendSyncRequestHook(6) == 0 && !tls[33] && PlgUdsSnapshot(&processes[1]).active);
    assert(processes[0].autoObject.references == 1 && sessions[0].autoObject.references == 1);
    PlgUdsResetProcess(&processes[1]);
    assert(processes[1].autoObject.references == 1);
    setup(); request(100, 2, 0); tls[33] = 8; tls[34] = 1;
    assert(SendSyncRequestHook(1) == 0 && (s32)tls[33] < 0 && !native_calls);
    setup(); initialize();
    request(104, 1, 0); tls[33] = 1; assert(SendSyncRequestHook(1) == 0);
    fail_event = true;
    request(1, 1, 4); tls[33] = 0xABCD; memcpy(before, tls, sizeof(before));
    assert(SendSyncRequestHook(4) == -13 && !soc_calls && temp_handles == temp_closes);
    assert(!memcmp(before, tls, sizeof(before)) && !PlgUdsSnapshot(&processes[0]).active);
    assert(sessions[0].autoObject.references == 1);
    setup(); initialize();
    request(104, 1, 0); tls[33] = 1; assert(SendSyncRequestHook(1) == 0);
    fail_create = true;
    request(1, 1, 4);
    assert(SendSyncRequestHook(4) == -12 && !soc_calls && !temp_handles && !temp_closes);
    setup(); initialize();
    processes[0].creationTime++;
    redirect(false);
    PlgUdsResetProcess(&processes[0]);
    assert(sessions[0].autoObject.references == 1);
    setup(); initialize(); PlgUdsResetProcess(&processes[0]); redirect(false);
    assert(!locks && temp_handles == temp_closes);
    for (unsigned index = 0; index < 8; index++) assert(sessions[index].clientSession.syncObject.autoObject.references == 1);
    setup();
    sessions[0].clientSession.clientPort = &clientPort;
    processes[0].handles.internalTable[0].pointer = &port.serverPort.syncObject.autoObject;
    initialize();
    PlgUdsResetProcess(&processes[0]);
    setup();
    processes[0].handles.internalTable[0].pointer = NULL;
    request(100, 2, 0); tls[33] = 8;
    assert(SendSyncRequestHook(1) == 0 && (s32)tls[33] < 0 && !native_calls);
    setup(); initialize();
    processes[0].handles.internalTable[0].pointer = NULL;
    unsigned finalize_calls = native_calls;
    request(101, 0, 0);
    assert(SendSyncRequestHook(1) == 0 && tls[32] == PlgUdsHeader(101, 1, 0) && tls[33] == 0);
    assert(native_calls == finalize_calls && !PlgUdsSnapshot(&processes[0]).active);
    assert(sessions[0].autoObject.references == 1 && processes[0].autoObject.references == 1);
    redirect(false);
    setup(); initialize();
    processes[0].handles.internalTable[0].pointer = NULL;
    finalize_calls = native_calls;
    request(101, 0, 0);
    assert(SendSyncRequestHook(7) == 0 && (s32)tls[33] < 0);
    assert(native_calls == finalize_calls && PlgUdsSnapshot(&processes[0]).active);
    request(101, 1, 0);
    assert(SendSyncRequestHook(1) == 0 && (s32)tls[33] < 0 && PlgUdsSnapshot(&processes[0]).active);
    core.objectContext.currentProcess = &processes[1];
    request(101, 0, 0);
    assert(SendSyncRequestHook(1) == 0 && (s32)tls[33] < 0 && PlgUdsSnapshot(&processes[0]).active);
    core.objectContext.currentProcess = &processes[0];
    processes[0].creationTime++;
    request(101, 0, 0);
    assert(SendSyncRequestHook(1) == 0 && (s32)tls[33] < 0);
    assert(sessions[0].autoObject.references == 2 && processes[0].autoObject.references == 2);
    PlgUdsResetProcess(&processes[0]);
    assert(sessions[0].autoObject.references == 1 && processes[0].autoObject.references == 1 && !locks);
    puts("PLGUDS kernel hook: process ownership, TLS restoration, handle cleanup, redirection and failure paths passed");
}
