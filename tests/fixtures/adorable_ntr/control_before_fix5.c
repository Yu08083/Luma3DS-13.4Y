/*
*   This file is part of Luma3DS (Level256 fork)
*   Copyright (C) 2026 Level256
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

static bool legacy_readFromConn(ControlConn *c)
{
    u8 scratch[256];
    int n;

    if (c->exdataLeft != 0)
    {
        u32 want = c->exdataLeft < sizeof(scratch) ? c->exdataLeft : sizeof(scratch);
        n = socRecv(c->sock, scratch, want, 0);
        if (n <= 0)
            return transient(n);
        c->lastProgress = svcGetSystemTick();
        c->exdataLeft -= n;
        return true;
    }

    n = socRecv(c->sock, c->header + c->headerGot, NTR_HEADER_SIZE - c->headerGot, 0);
    if (n <= 0)
        return transient(n);
    c->lastProgress = svcGetSystemTick();
    c->headerGot += n;
    if (c->headerGot < NTR_HEADER_SIZE)
        return true;

    c->headerGot = 0;
    return handlePacket(c);
}
static void legacy_controlThreadMain(void)
{
    ControlConn conn = { .sock = -1 };
    u64 lastLinkCheck = 0;

    while (!stopRequested && !preTerminationRequested)
    {
        if (Sleep__IsSleeping())
        {
            checkLinkNow = true;
            svcSleepThread(CONTROL_POLL_INTERVAL_MS * 1000 * 1000LL);
            continue;
        }

        u64 now = svcGetSystemTick();
        if (checkLinkNow || now - lastLinkCheck >= (u64)LINK_CHECK_INTERVAL_US * TICKS_PER_US)
        {
            lastLinkCheck = now;
            checkLink(&conn);
        }

        if (linkDown)
        {
            svcSleepThread(CONTROL_POLL_INTERVAL_MS * 1000 * 1000LL);
            continue;
        }

        if (listenSock < 0)
        {
            svcSleepThread(1000 * 1000 * 1000LL);
            listenSock = openListenSocket();
            continue;
        }

        struct pollfd fds[2] = {
            { .fd = listenSock, .events = POLLIN },
            { .fd = conn.sock, .events = conn.replySize ? POLLOUT : POLLIN },
        };
        nfds_t nfds = conn.sock >= 0 ? 2 : 1;

        if (conn.sock >= 0 && now - conn.lastProgress > (u64)15 * SYSCLOCK_ARM11)
        {
            closeSocket(&conn.sock);
            continue;
        }
        int res = socPoll(fds, nfds, 0);
        if (res < -10000)
            break;
        if (res <= 0)
        {
            svcSleepThread(CONTROL_POLL_INTERVAL_MS * 1000 * 1000LL);
            continue;
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            closeSocket(&listenSock);
            continue;
        }

        if (fds[0].revents & POLLIN)
        {
            struct sockaddr_in addr;
            socklen_t addrLen = sizeof(addr);
            int sock = socAccept(listenSock, (struct sockaddr *)&addr, &addrLen);
            if (sock >= 0 && socSetNonBlocking(sock) < 0)
                closeSocket(&sock);
            if (sock >= 0)
            {
                closeSocket(&conn.sock);
                memset(&conn, 0, sizeof(conn));
                conn.sock = sock;
                conn.peerIp = addr.sin_addr.s_addr;
                conn.lastProgress = svcGetSystemTick();
            }
            else if (!transient(sock))
                closeSocket(&listenSock);
        }
        else if (nfds == 2 && (fds[1].revents & (POLLIN | POLLOUT | POLLHUP | POLLERR | POLLNVAL)))
        {
            if ((fds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) ||
                !(conn.replySize ? writeToConn(&conn) : legacy_readFromConn(&conn)))
                closeSocket(&conn.sock);
        }
    }

    closeSocket(&conn.sock);
}
