int main(void)
{
    KSession sessions[MAX_SESSION * 2 + 2] = {0};
    for (unsigned index = 0; index < MAX_SESSION * 2 + 2; index++) sessions[index].autoObject.vtable = customSessionVtable;
    assert(!SessionInfo_Lookup(&sessions[0]));
    SessionInfo_Remove(&sessions[0]);
    assert(!nbActiveSessions);
    nbActiveSessions = 3;
    for (unsigned index = 0; index < 3; index++) sessionInfos[index].session = &sessions[index * 2 + 2];
    assert(!SessionInfo_Lookup(&sessions[3]));
    assert(SessionInfo_Lookup(&sessions[4]) == &sessionInfos[1]);
    SessionInfo_Remove(&sessions[3]);
    assert(nbActiveSessions == 3);
    SessionInfo_Remove(&sessions[2]);
    assert(nbActiveSessions == 2 && sessionInfos[0].session == &sessions[4]);
    SessionInfo_Remove(&sessions[6]);
    SessionInfo_Remove(&sessions[4]);
    assert(!nbActiveSessions);
    nbActiveSessions = MAX_SESSION;
    for (unsigned index = 0; index < MAX_SESSION; index++) sessionInfos[index].session = &sessions[index * 2];
    SessionInfo_Remove(&sessions[1]);
    assert(nbActiveSessions == MAX_SESSION);
    SessionInfo_Remove(&sessions[26]);
    assert(nbActiveSessions == MAX_SESSION - 1 && !SessionInfo_Lookup(&sessions[26]));
    assert(SessionInfo_Lookup(&sessions[(MAX_SESSION - 1) * 2]));
    while (nbActiveSessions) SessionInfo_Remove(sessionInfos[0].session);
    assert(!sessionInfosLock.depth && !criticalLock.depth);
    puts("PLGUDS session registry: exact identity, full capacity removal and unregistered destruction passed");
}
