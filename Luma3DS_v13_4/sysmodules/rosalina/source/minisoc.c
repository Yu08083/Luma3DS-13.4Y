
/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2020 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "minisoc.h"
#include <sys/socket.h>
#include <3ds.h>
#include <string.h>
#include "utils.h"

s32 miniSocRefCount = 0;
static s32 miniSocStateRefs;
static LightLock miniSocLifecycle = (LightLock)1;
static u32 socContextAddr = 0x08000000;
static u32 socContextSize = 0x100000;
static Handle miniSocHandle;
static Handle miniSocMemHandle;
static bool exclusiveStateEntered = false;
static bool miniSocShutdownSent;

bool miniSocEnabled = false;
static volatile u32 miniSocHostId;

u32 miniSocGetCachedHostId(void)
{
    return miniSocEnabled ? miniSocHostId : 0;
}

u32 miniSocRefreshHostId(void)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(0x16, 0, 0);
    Result result = svcSendSyncRequest(miniSocHandle);
    miniSocHostId = result == 0 && cmdbuf[1] == 0 ? cmdbuf[2] : 0;
    return miniSocHostId;
}

s32 _net_convert_error(s32 sock_retval);

static Result SOCU_Initialize(Handle memhandle, u32 memsize)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x1,1,4); 
    cmdbuf[1] = memsize;
    cmdbuf[2] = IPC_Desc_CurProcessId();
    cmdbuf[4] = IPC_Desc_SharedHandles(1);
    cmdbuf[5] = memhandle;

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0)
        return ret;

    return cmdbuf[1];
}

static Result SOCU_Shutdown(void)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x19,0,0); 

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0)
        return ret;

    return cmdbuf[1];
}


void miniSocLockState(void)
{
    Result res = 0;
    __dmb();
    if (!exclusiveStateEntered && isServiceUsable("ndm:u"))
    {
        ndmuInit();
        res = NDMU_EnterExclusiveState(NDM_EXCLUSIVE_STATE_INFRASTRUCTURE);
        if (R_SUCCEEDED(res))
            res = NDMU_LockState(); 
        exclusiveStateEntered = R_SUCCEEDED(res);
        __dmb();
    }
}

void miniSocUnlockState(bool force)
{
    Result res = 0;

    __dmb();
    if (exclusiveStateEntered)
    {
        if (!force)
        {
            res = NDMU_UnlockState();
            if (R_SUCCEEDED(res))
                res = NDMU_LeaveExclusiveState();
        }
        ndmuExit();
        exclusiveStateEntered = R_FAILED(res);
        __dmb();
    }
}

static Result miniSocInitUnlocked(bool lockState)
{
    if (miniSocRefCount > 0)
    {
        if (miniSocShutdownSent)
            return -1;
        miniSocRefCount++;
        if (lockState)
        {
            miniSocStateRefs++;
            miniSocLockState();
        }
        return 0;
    }

    u32 tmp = 0;
    Result ret = 0;
    bool isSocURegistered;

    ret = srvIsServiceRegistered(&isSocURegistered, "soc:U");
    if(ret != 0) goto cleanup;
    if(!isSocURegistered)
    {
        ret = -1;
        goto cleanup;
    }

    ret = srvGetServiceHandle(&miniSocHandle, "soc:U");
    if(ret != 0) goto cleanup;

    ret = svcControlMemoryEx(&tmp, socContextAddr, 0, socContextSize, MEMOP_ALLOC | MEMOP_REGION_SYSTEM, MEMPERM_READ | MEMPERM_WRITE, true);
    if(ret != 0) goto cleanup;

    socContextAddr = tmp;

    ret = svcCreateMemoryBlock(&miniSocMemHandle, (u32)socContextAddr, socContextSize, 0, 3);
    if(ret != 0) goto cleanup;



    ret = SOCU_Initialize(miniSocMemHandle, socContextSize);
    if(ret != 0) goto cleanup;

    if (lockState)
        miniSocLockState();

    svcKernelSetState(0x10000, 0x10);
    miniSocRefreshHostId();
    miniSocEnabled = true;
    miniSocRefCount = 1;
    miniSocStateRefs = lockState ? 1 : 0;
    miniSocShutdownSent = false;

    return 0;

cleanup:
    if(miniSocMemHandle != 0)
    {
        svcCloseHandle(miniSocMemHandle);
        miniSocMemHandle = 0;
    }

    if(miniSocHandle != 0)
    {
        SOCU_Shutdown();
        svcCloseHandle(miniSocHandle);
        miniSocHandle = 0;
    }

    if(tmp != 0)
        svcControlMemory(&tmp, socContextAddr, socContextAddr, socContextSize, MEMOP_FREE, MEMPERM_DONTCARE);

    return ret;
}

static Result miniSocExitUnlocked(void)
{
    Result ret = 0;
    u32 tmp;
    if (!miniSocShutdownSent)
    {
        ret = SOCU_Shutdown();
        if (R_FAILED(ret))
            return ret;
        miniSocShutdownSent = true;
    }
    if (miniSocMemHandle != 0)
    {
        ret = svcCloseHandle(miniSocMemHandle);
        if (R_FAILED(ret))
            return ret;
        miniSocMemHandle = 0;
    }
    if (miniSocHandle != 0)
    {
        ret = svcCloseHandle(miniSocHandle);
        if (R_FAILED(ret))
            return ret;
        miniSocHandle = 0;
    }
    ret = svcControlMemory(&tmp, socContextAddr, socContextAddr, socContextSize, MEMOP_FREE, MEMPERM_DONTCARE);
    if (R_FAILED(ret))
        return ret;
    miniSocUnlockState(false);
    miniSocEnabled = false;
    miniSocShutdownSent = false;
    svcKernelSetState(0x10000, 0x10);
    return 0;
}

Result miniSocInitEx(bool lockState)
{
    LightLock_Lock(&miniSocLifecycle);
    Result result = miniSocInitUnlocked(lockState);
    LightLock_Unlock(&miniSocLifecycle);
    return result;
}

Result miniSocInit(void)
{
    return miniSocInitEx(true);
}

Result miniSocExitDirect(void)
{
    LightLock_Lock(&miniSocLifecycle);
    Result result = miniSocExitUnlocked();
    if (R_SUCCEEDED(result))
        miniSocRefCount = miniSocStateRefs = 0;
    LightLock_Unlock(&miniSocLifecycle);
    return result;
}

Result miniSocExitEx(bool lockState)
{
    LightLock_Lock(&miniSocLifecycle);
    Result result = 0;
    if (miniSocEnabled && miniSocRefCount > 0)
    {
        s32 oldRefs = miniSocRefCount;
        s32 oldStateRefs = miniSocStateRefs;
        miniSocRefCount--;
        if (lockState && miniSocStateRefs > 0)
            miniSocStateRefs--;
        if (miniSocRefCount == 0)
        {
            result = miniSocExitUnlocked();
            if (R_FAILED(result))
            {
                miniSocRefCount = oldRefs;
                miniSocStateRefs = oldStateRefs;
            }
        }
        else if (miniSocStateRefs == 0)
            miniSocUnlockState(false);
    }
    LightLock_Unlock(&miniSocLifecycle);
    return result;
}

Result miniSocExit(void)
{
    return miniSocExitEx(true);
}

int socSocket(int domain, int type, int protocol)
{
    int ret = 0;

    u32 *cmdbuf = getThreadCommandBuffer();

    
                                                                          
    if (domain == AF_INET
    && type == SOCK_STREAM
    && protocol == IPPROTO_TCP) {
        protocol = 0; 
    }
    if (domain == AF_INET
    && type == SOCK_DGRAM
    && protocol == IPPROTO_UDP) {
        protocol = 0; 
    }

    cmdbuf[0] = IPC_MakeHeader(0x2,3,2); 
    cmdbuf[1] = domain;
    cmdbuf[2] = type;
    cmdbuf[3] = protocol;
    cmdbuf[4] = IPC_Desc_CurProcessId();

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0)
    {
        
        return R_FAILED(ret) ? ret : -1;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0) ret = cmdbuf[2];
    if(ret < 0)
    {
        
        
        return -1;
    }
    else
        return cmdbuf[2];
}

int socBind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    Result ret = 0;
    socklen_t tmp_addrlen = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    u8 tmpaddr[0x1c];

    memset(tmpaddr, 0, 0x1c);

    if(addr->sa_family == AF_INET)
        tmp_addrlen = 8;
    else
        tmp_addrlen = 0x1c;

    if(addrlen < tmp_addrlen)
    {
        
        return -1;
    }

    tmpaddr[0] = tmp_addrlen;
    tmpaddr[1] = addr->sa_family;
    memcpy(&tmpaddr[2], &addr->sa_data, tmp_addrlen-2);

    cmdbuf[0] = IPC_MakeHeader(0x5,2,4); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)tmp_addrlen;
    cmdbuf[3] = IPC_Desc_CurProcessId();
    cmdbuf[5] = IPC_Desc_StaticBuffer(tmp_addrlen,0);
    cmdbuf[6] = (u32)tmpaddr;

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0) {
        
        return ret;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    if(ret < 0) {
        
        return -1;
    }

    return ret;
}

int socListen(int sockfd, int max_connections)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x3,2,2); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)max_connections;
    cmdbuf[3] = IPC_Desc_CurProcessId();

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0)
    {
        
        return ret;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    if(ret < 0) {
        
        return -1;
    }

    return 0;
}

static void copySockaddr(struct sockaddr *addr, socklen_t *addrlen, const u8 *source)
{
    if(addr == NULL || addrlen == NULL)
        return;
    u32 length = source[0];
    if(length < 2 || length > 0x1c)
    {
        *addrlen = 0;
        return;
    }
    u32 size = *addrlen < length ? *addrlen : length;
    u16 family = source[1];
    memcpy(addr, &family, size < 2 ? size : 2);
    if(size > 2)
        memcpy(addr->sa_data, source + 2, size - 2);
    *addrlen = length;
}

int socAccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    Result ret = 0;
    int tmp_addrlen = 0x1c;

    u32 *cmdbuf = getThreadCommandBuffer();
    u8 tmpaddr[0x1c];
    u32 saved_threadstorage[2];

    memset(tmpaddr, 0, 0x1c);

    cmdbuf[0] = IPC_MakeHeader(0x4,2,2); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)tmp_addrlen;
    cmdbuf[3] = IPC_Desc_CurProcessId();

    u32 *staticbufs = getThreadStaticBuffers();
    saved_threadstorage[0] = staticbufs[0];
    saved_threadstorage[1] = staticbufs[1];

    staticbufs[0] = IPC_Desc_StaticBuffer(tmp_addrlen,0);
    staticbufs[1] = (u32)tmpaddr;

    ret = svcSendSyncRequest(miniSocHandle);

    staticbufs[0] = saved_threadstorage[0];
    staticbufs[1] = saved_threadstorage[1];

    if(ret != 0)
        return ret;

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    
        

    if(ret >= 0 && addr != NULL)
        copySockaddr(addr, addrlen, tmpaddr);

    if(ret < 0)
        return ret;

    return ret;
}

int socConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int ret = 0;
    socklen_t tmp_addrlen = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    u8 tmpaddr[0x1c];

    memset(tmpaddr, 0, 0x1c);

    if(addr->sa_family == AF_INET)
        tmp_addrlen = 8;
    else
        tmp_addrlen = 0x1c;

    if(addrlen < tmp_addrlen)
        return -1;

    tmpaddr[0] = tmp_addrlen;
    tmpaddr[1] = addr->sa_family;
    memcpy(&tmpaddr[2], &addr->sa_data, tmp_addrlen-2);

    cmdbuf[0] = IPC_MakeHeader(0x6,2,4); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)addrlen;
    cmdbuf[3] = IPC_Desc_CurProcessId();
    cmdbuf[5] = IPC_Desc_StaticBuffer(tmp_addrlen,0);
    cmdbuf[6] = (u32)tmpaddr;

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0) return -1;

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    if(ret < 0)
        return -1;

    return 0;
}

int socPoll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int ret = 0;
    nfds_t i;
    u32 size = sizeof(struct pollfd)*nfds;
    u32 *cmdbuf = getThreadCommandBuffer();
    u32 saved_threadstorage[2];

    if(nfds == 0) {
        return -1;
    }

    for(i = 0; i < nfds; ++i) {
        fds[i].revents = 0;
    }

    cmdbuf[0] = IPC_MakeHeader(0x14,2,4); 
    cmdbuf[1] = (u32)nfds;
    cmdbuf[2] = (u32)timeout;
    cmdbuf[3] = IPC_Desc_CurProcessId();
    cmdbuf[5] = IPC_Desc_StaticBuffer(size,10);
    cmdbuf[6] = (u32)fds;

    u32 * staticbufs = getThreadStaticBuffers();
    saved_threadstorage[0] = staticbufs[0];
    saved_threadstorage[1] = staticbufs[1];

    staticbufs[0] = IPC_Desc_StaticBuffer(size,0);
    staticbufs[1] = (u32)fds;

    ret = svcSendSyncRequest(miniSocHandle);

    staticbufs[0] = saved_threadstorage[0];
    staticbufs[1] = saved_threadstorage[1];

    if(ret != 0) {
        return R_FAILED(ret) ? ret : -1;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    return ret;
}

int socClose(int sockfd)
{
    int ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0xB,1,2); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = IPC_Desc_CurProcessId();

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0) {
        
        return ret;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret =_net_convert_error(cmdbuf[2]);

    if(ret < 0) {
        
        return -1;
    }

    return 0;
}

int socSetsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    int ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x12,4,4); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)level;
    cmdbuf[3] = (u32)optname;
    cmdbuf[4] = (u32)optlen;
    cmdbuf[5] = IPC_Desc_CurProcessId();
    cmdbuf[7] = IPC_Desc_StaticBuffer(optlen,9);
    cmdbuf[8] = (u32)optval;

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0) {
        return ret;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    if(ret < 0) {
        
        return -1;
    }

    return ret;
}

long socGethostid(void)
{
    int ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x16,0,0); 

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0) {
        
        return -1;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = cmdbuf[2];

    return ret;
}

static ssize_t _socuipc_cmd7(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    int ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    u32 tmp_addrlen = 0;
    u8 tmpaddr[0x1c];
    u32 saved_threadstorage[2];

    memset(tmpaddr, 0, 0x1c);

    if(src_addr)
        tmp_addrlen = 0x1c;

    cmdbuf[0] = IPC_MakeHeader(0x7,4,4); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)len;
    cmdbuf[3] = (u32)flags;
    cmdbuf[4] = (u32)tmp_addrlen;
    cmdbuf[5] = IPC_Desc_CurProcessId();
    cmdbuf[7] = IPC_Desc_Buffer(len,IPC_BUFFER_W);
    cmdbuf[8] = (u32)buf;

    u32 * staticbufs = getThreadStaticBuffers();
    saved_threadstorage[0] = staticbufs[0];
    saved_threadstorage[1] = staticbufs[1];

    staticbufs[0] = IPC_Desc_StaticBuffer(tmp_addrlen,0);
    staticbufs[1] = (u32)tmpaddr;

    ret = svcSendSyncRequest(miniSocHandle);

    staticbufs[0] = saved_threadstorage[0];
    staticbufs[1] = saved_threadstorage[1];

    if(ret != 0) {
        
        return -1;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    if(ret < 0) {
        
        return ret;
    }

    if(src_addr != NULL)
        copySockaddr(src_addr, addrlen, tmpaddr);

    return ret;
}

static ssize_t _socuipc_cmd8(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    int ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    u32 tmp_addrlen = 0;
    u8 tmpaddr[0x1c];
    u32 saved_threadstorage[4];

    if(src_addr)
        tmp_addrlen = 0x1c;

    memset(tmpaddr, 0, 0x1c);

    cmdbuf[0] = 0x00080102;
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)len;
    cmdbuf[3] = (u32)flags;
    cmdbuf[4] = (u32)tmp_addrlen;
    cmdbuf[5] = 0x20;

    saved_threadstorage[0] = cmdbuf[0x100>>2];
    saved_threadstorage[1] = cmdbuf[0x104>>2];
    saved_threadstorage[2] = cmdbuf[0x108>>2];
    saved_threadstorage[3] = cmdbuf[0x10c>>2];

    cmdbuf[0x100>>2] = (((u32)len)<<14) | 2;
    cmdbuf[0x104>>2] = (u32)buf;
    cmdbuf[0x108>>2] = (tmp_addrlen<<14) | 2;
    cmdbuf[0x10c>>2] = (u32)tmpaddr;

    ret = svcSendSyncRequest(miniSocHandle);

    cmdbuf[0x100>>2] = saved_threadstorage[0];
    cmdbuf[0x104>>2] = saved_threadstorage[1];
    cmdbuf[0x108>>2] = saved_threadstorage[2];
    cmdbuf[0x10c>>2] = saved_threadstorage[3];
    if(ret != 0) return ret;

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    if(ret < 0) {
        
        return ret;
    }

    if(src_addr != NULL)
        copySockaddr(src_addr, addrlen, tmpaddr);

    return ret;
}

static ssize_t _socuipc_cmd9(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    int ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    u32 tmp_addrlen = 0;
    u8 tmpaddr[0x1c];

    memset(tmpaddr, 0, 0x1c);

    if(dest_addr) {
        if(dest_addr->sa_family == AF_INET)
            tmp_addrlen = 8;
        else
            tmp_addrlen = 0x1c;

        if(addrlen < tmp_addrlen) {
            
            return -1;
        }

        tmpaddr[0] = tmp_addrlen;
        tmpaddr[1] = dest_addr->sa_family;
        memcpy(&tmpaddr[2], &dest_addr->sa_data, tmp_addrlen-2);
    }

    cmdbuf[0] = IPC_MakeHeader(0x9,4,6); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)len;
    cmdbuf[3] = (u32)flags;
    cmdbuf[4] = (u32)tmp_addrlen;
    cmdbuf[5] = IPC_Desc_CurProcessId();
    cmdbuf[7] = IPC_Desc_StaticBuffer(tmp_addrlen,1);
    cmdbuf[8] = (u32)tmpaddr;
    cmdbuf[9] = IPC_Desc_Buffer(len,IPC_BUFFER_R);
    cmdbuf[10] = (u32)buf;

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0) {
        
        return ret;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    if(ret < 0) {
        
        return ret;
    }

    return ret;
}

static ssize_t _socuipc_cmda(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    int ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    u32 tmp_addrlen = 0;
    u8 tmpaddr[0x1c];

    memset(tmpaddr, 0, 0x1c);

    if(dest_addr) {
        if(dest_addr->sa_family == AF_INET)
            tmp_addrlen = 8;
        else
            tmp_addrlen = 0x1c;

        if(addrlen < tmp_addrlen) {
            
            return -1;
        }

        tmpaddr[0] = tmp_addrlen;
        tmpaddr[1] = dest_addr->sa_family;
        memcpy(&tmpaddr[2], &dest_addr->sa_data, tmp_addrlen-2);
    }

    cmdbuf[0] = IPC_MakeHeader(0xA,4,6); 
    cmdbuf[1] = (u32)sockfd;
    cmdbuf[2] = (u32)len;
    cmdbuf[3] = (u32)flags;
    cmdbuf[4] = (u32)tmp_addrlen;
    cmdbuf[5] = IPC_Desc_CurProcessId();
    cmdbuf[7] = IPC_Desc_StaticBuffer(len,2);
    cmdbuf[8] = (u32)buf;
    cmdbuf[9] = IPC_Desc_StaticBuffer(tmp_addrlen,1);
    cmdbuf[10] = (u32)tmpaddr;

    ret = svcSendSyncRequest(miniSocHandle);
    if(ret != 0) {
        
        return ret;
    }

    ret = (int)cmdbuf[1];
    if(ret == 0)
        ret = _net_convert_error(cmdbuf[2]);

    if(ret < 0) {
        
        return ret;
    }

    return ret;
}

ssize_t socRecvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    if(len < 0x2000)
        return _socuipc_cmd8(sockfd, buf, len, flags, src_addr, addrlen);
    return _socuipc_cmd7(sockfd, buf, len, flags, src_addr, addrlen);
}

ssize_t socSendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    if(len < 0x2000)
        return _socuipc_cmda(sockfd, buf, len, flags, dest_addr, addrlen);
    return _socuipc_cmd9(sockfd, buf, len, flags, dest_addr, addrlen);
}

int socResolveIPv4(const char *hostname,u32 addresses[4])
{
    static u8 output[0x1A88];
    if(!miniSocEnabled || !hostname || !addresses) return -ENETDOWN;
    u32 length=0;
    while(hostname[length])
    {
        u8 c=(u8)hostname[length];
        if(length>=253 || !((c>='a' && c<='z') || (c>='A' && c<='Z') ||
           (c>='0' && c<='9') || c=='.' || c=='-')) return -EINVAL;
        length++;
    }
    if(!length) return -EINVAL;
    memset(output,0,sizeof(output));
    u32 *cmdbuf=getThreadCommandBuffer();
    u32 *slots=getThreadStaticBuffers();
    u32 saved[2]={slots[0],slots[1]};
    cmdbuf[0]=IPC_MakeHeader(0xD,2,2);
    cmdbuf[1]=length+1;
    cmdbuf[2]=sizeof(output);
    cmdbuf[3]=IPC_Desc_StaticBuffer(length+1,3);
    cmdbuf[4]=(u32)hostname;
    slots[0]=IPC_Desc_StaticBuffer(sizeof(output),0);
    slots[1]=(u32)output;
    Result result=svcSendSyncRequest(miniSocHandle);
    slots[0]=saved[0];
    slots[1]=saved[1];
    if(result!=0 || cmdbuf[1]) return -SYNC_ERROR;
    int status=_net_convert_error(cmdbuf[2]);
    if(status<0) return status;
    u32 count;
    memcpy(&count,output+4,4);
    if(!count || count>16) return -ENOENT;
    unsigned int used=0;
    for(u32 i=0;i<count && used<4;i++)
    {
        u32 address;
        memcpy(&address,output+0x1908+i*0x10,4);
        if(!address || address==0xFFFFFFFF) continue;
        bool duplicate=false;
        for(unsigned int k=0;k<used;k++) if(addresses[k]==address) duplicate=true;
        if(!duplicate) addresses[used++]=address;
    }
    return used ? (int)used : -ENOENT;
}

int socSetNonBlocking(int sockfd)
{
    if(!miniSocEnabled || sockfd<0) return -EINVAL;
    u32 *cmdbuf=getThreadCommandBuffer();
    cmdbuf[0]=IPC_MakeHeader(0x13,3,2);
    cmdbuf[1]=(u32)sockfd;
    cmdbuf[2]=4;
    cmdbuf[3]=4;
    cmdbuf[4]=IPC_Desc_CurProcessId();
    Result result=svcSendSyncRequest(miniSocHandle);
    if(result!=0 || cmdbuf[1]) return -SYNC_ERROR;
    return _net_convert_error(cmdbuf[2]);
}

int socConnectAsync(int sockfd,const struct sockaddr *addr,socklen_t addrlen)
{
    if(!miniSocEnabled || sockfd<0 || !addr || addr->sa_family!=AF_INET || addrlen!=sizeof(struct sockaddr_in)) return -EINVAL;
    u8 target[8]={8,AF_INET};
    memcpy(target+2,addr->sa_data,6);
    u32 *cmdbuf=getThreadCommandBuffer();
    cmdbuf[0]=IPC_MakeHeader(0x6,2,4);
    cmdbuf[1]=(u32)sockfd;
    cmdbuf[2]=addrlen;
    cmdbuf[3]=IPC_Desc_CurProcessId();
    cmdbuf[5]=IPC_Desc_StaticBuffer(sizeof(target),0);
    cmdbuf[6]=(u32)target;
    Result result=svcSendSyncRequest(miniSocHandle);
    if(result!=0 || cmdbuf[1]) return -SYNC_ERROR;
    return _net_convert_error(cmdbuf[2]);
}
