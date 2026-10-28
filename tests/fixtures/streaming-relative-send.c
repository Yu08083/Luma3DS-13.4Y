static int referenceSendFrame(const JpegSlot *slot)
{
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(REMOTEPLAY_VIDEO_PORT),
        .sin_addr.s_addr = slot->destIp,
    };

    const u8 *jpeg = slot->data;
    u32 size = slot->size;
    bool top = slot->top;
    u8 *pkt = snd.packet;
    pkt[0] = slot->frameId;
    pkt[2] = 2;
    pkt[3] = 0;

    u32 packets = 0;
    u64 sendtoTicks = 0, sleepTicks = 0;

    while (size != 0 && !stopRequested && !linkDown)
    {
        u32 chunk = size < VIDEO_PACKET_SIZE - VIDEO_HEADER_SIZE ? size : VIDEO_PACKET_SIZE - VIDEO_HEADER_SIZE;
        size -= chunk;
        pkt[1] = (top ? 1 : 0) | (size == 0 ? 0x10 : 0);
        memcpy(pkt + VIDEO_HEADER_SIZE, jpeg, chunk);
        jpeg += chunk;

        u64 now = svcGetSystemTick();
        u64 elapsed = now - snd.lastPacketTick;
        while (elapsed < slot->packetIntervalTicks && !stopRequested && !linkDown)
        {
            u64 remaining = slot->packetIntervalTicks - elapsed;
            svcSleepThread((s64)((remaining * 1000000000ULL + SYSCLOCK_ARM11 - 1) / SYSCLOCK_ARM11));
            u64 woke = svcGetSystemTick();
            sleepTicks += woke - now;
            now = woke;
            elapsed = now - snd.lastPacketTick;
        }
        if (stopRequested || linkDown)
            break;

        snd.lastPacketTick = now;
        int res = socSendto(videoSock, pkt, VIDEO_HEADER_SIZE + chunk, 0, (struct sockaddr *)&addr, sizeof(addr));
        u64 sent = svcGetSystemTick();
        if (res != (int)(VIDEO_HEADER_SIZE + chunk))
        {
            statusIncrement(&status.sendErrors);
            return transient(res) ? 0 : -1;
        }

        sendtoTicks += sent - now;
        packets++;
        pkt[3]++;
    }

    LightLock_Lock(&settingsLock);
    status.packets = packets;
    status.sendtoUs = ticksToUs(sendtoTicks);
    status.sleepUs = ticksToUs(sleepTicks);
    LightLock_Unlock(&settingsLock);
    return size == 0 && !stopRequested && !linkDown ? 1 : 0;
}
