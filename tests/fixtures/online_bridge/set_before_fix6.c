bool LegacyOnlineBridgeSet(bool enabled)
{
    LightLock_Lock(&onlineBridgeLock);
    bool valid = OnlineBridge_Valid(onlineBridge);
    if(valid)
    {
        onlineBridge->requestedEnabled = enabled;
        __sync_synchronize();
        ++onlineBridge->requestSeq;
        svcFlushDataCacheRange((const void *)onlineBridge, sizeof(*onlineBridge));
    }
    LightLock_Unlock(&onlineBridgeLock);
    return valid;
}
