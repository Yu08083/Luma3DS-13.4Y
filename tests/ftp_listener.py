import os
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
generated = root / 'tests/generated'
generated.mkdir(exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')

def function(source, name):
    start = source.index(name + '(')
    start = source.rfind('\n', 0, start) + 1
    opened = source.index('{', start)
    depth, end = 1, opened + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

prefix = r"""
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t Result;
typedef u32 socklen_t;
struct in_addr { u32 s_addr; };
struct sockaddr { int unused; };
struct sockaddr_in { int sin_family; u16 sin_port; struct in_addr sin_addr; };
struct pollfd { int fd, events, revents; };
#define FTPD_PORT 5000
#define SYSCLOCK_ARM11 268123480ULL
#define AF_INET 2
#define SOCK_STREAM 1
#define SOL_SOCKET 65535
#define SO_REUSEADDR 4
#define INADDR_ANY 0
#define POLLIN 1
#define POLLERR 8
#define POLLHUP 4
#define POLLNVAL 32
#define R_FAILED(v) ((v) < 0)
static u16 htons(u16 v) { return (u16)((v << 8) | (v >> 8)); }
static volatile bool ftpdEnabled;
static int g_listenSock = -1, g_ctrlSock = -1, g_pasvSock = -1, g_dataSock = -1;
static u16 g_pasvPort = FTPD_PORT + 1;
static bool opened[4096], optionSet[4096], reused[4096], bound[4096], listening[4096];
static u16 ports[4096];
static u32 now, sleeps, ipReads, pollCalls, sessions, accepts, closes, initCalls, exitCalls;
static int nextFd, failStage, failRemaining, pollMode, ipMode, cancelSleep, initResult;
static bool sawListening;
static bool fails(int stage) { if(failStage != stage || !failRemaining) return false; --failRemaining; return true; }
static Result miniSocInit(void) { ++initCalls; return initResult; }
static Result miniSocExit(void) { ++exitCalls; return 0; }
static u32 miniSocRefreshHostId(void)
{
    ++ipReads;
    if(ipMode == 1) return 0;
    if(ipMode == 4) return UINT32_MAX;
    if(ipMode == 2 && ipReads <= 2) return ipReads == 1 ? 0 : UINT32_MAX;
    if(ipMode == 3 && ipReads > 1) return 0x0200007F;
    return 0x0100007F;
}
static uint64_t svcGetSystemTick(void) { return (uint64_t)now * SYSCLOCK_ARM11 / 1000; }
static void svcSleepThread(int64_t ns)
{
    assert(ns == 100000000LL);
    ++sleeps; now += 100;
    if(cancelSleep) ftpdEnabled = false;
}
static int socSocket(int family, int type, int protocol)
{
    assert(family == AF_INET && type == SOCK_STREAM && protocol == 0);
    if(fails(1)) return -87;
    assert(nextFd < 4096);
    int fd = nextFd++; opened[fd] = true; return fd;
}
static int socSetsockopt(int fd, int level, int option, const void *value, socklen_t size)
{
    assert(opened[fd] && level == SOL_SOCKET && option == SO_REUSEADDR);
    assert(size == sizeof(int) && *(const int *)value == 1);
    optionSet[fd] = true;
    if(fails(2)) return -88;
    reused[fd] = true; return 0;
}
static int socBind(int fd, const struct sockaddr *pointer, socklen_t size)
{
    const struct sockaddr_in *address = (const struct sockaddr_in *)pointer;
    assert(opened[fd] && optionSet[fd] && size == sizeof(*address));
    assert(address->sin_family == AF_INET && address->sin_addr.s_addr == INADDR_ANY);
    ports[fd] = htons(address->sin_port);
    if(fails(3)) return -89;
    bound[fd] = true; return 0;
}
static int socListen(int fd, int backlog)
{
    assert(opened[fd] && optionSet[fd] && bound[fd] && (backlog == 1 || backlog == 2));
    if(fails(4)) return -90;
    listening[fd] = true; sawListening = true; return 0;
}
static int socClose(int fd) { assert(opened[fd]); opened[fd] = false; ++closes; return 0; }
"""
network = r"""
static int socPoll(struct pollfd *fd, int count, int timeout)
{
    assert(count == 1 && timeout == 100 && listening[fd->fd]);
    assert(g_status == FTPD_LISTENING && g_listenSock == fd->fd);
    ++pollCalls; now += 100;
    if(pollMode == 1) { fd->revents = POLLERR; return 1; }
    if(pollCalls == 1) return -EINTR;
    if(pollCalls == 2) { fd->revents = POLLIN; return 1; }
    if(pollCalls == 20) ftpdEnabled = false;
    return 0;
}
static int socAccept(int fd, struct sockaddr *address, socklen_t *size)
{
    (void)address; (void)size;
    assert(opened[fd] && listening[fd]); ++accepts;
    int client = nextFd++; opened[client] = true; return client;
}
static void ftpdSession(int fd)
{
    assert(opened[fd] && g_status == FTPD_CONNECTED && g_activeClients == 1);
    ++sessions;
}
"""
main = r"""
static void reset(void)
{
    memset(opened, 0, sizeof(opened)); memset(reused, 0, sizeof(reused));
    memset(optionSet, 0, sizeof(optionSet));
    memset(bound, 0, sizeof(bound)); memset(listening, 0, sizeof(listening));
    ftpdEnabled = true; g_listenSock = g_ctrlSock = g_pasvSock = g_dataSock = -1;
    g_pasvPort = FTPD_PORT + 1;
    g_status = FTPD_STARTING; g_error = 0; g_hostId = 0;
    g_clients = g_activeClients = g_transfers = g_bytes = g_total = 0;
    now = sleeps = ipReads = pollCalls = sessions = accepts = closes = initCalls = exitCalls = 0;
    nextFd = 1; failStage = failRemaining = pollMode = ipMode = cancelSleep = initResult = 0;
    sawListening = false;
}
static void clean(void)
{
    assert(!ftpdEnabled && g_listenSock == -1 && g_ctrlSock == -1 && g_pasvSock == -1 && g_dataSock == -1);
    for(int i = 0; i < nextFd; ++i) assert(!opened[i]);
    assert(g_activeClients == 0 && initCalls == 1 && exitCalls == 1);
}
int main(void)
{
    reset(); ipMode = 2;
    ftpdThreadMain(); clean();
    assert(sawListening && sleeps == 2 && sessions == 1 && accepts == 1 && g_clients == 1 && g_status == FTPD_STOPPED);
    for(int stage = 1; stage <= 4; ++stage)
    {
        if(stage == 2) continue;
        reset(); failStage = stage; failRemaining = 2;
        ftpdThreadMain(); clean();
        assert(sawListening && sleeps == 2 && sessions == 1 && g_error == 0);
        reset(); failStage = stage; failRemaining = 100;
        ftpdThreadMain(); clean();
        assert(!sawListening && sleeps == 100 && g_status == FTPD_FAILED && g_error < 0);
    }
    reset(); failStage = 2; failRemaining = 100;
    ftpdThreadMain(); clean();
    assert(sawListening && sleeps == 0 && sessions == 1 && g_error == 0);
    reset(); ipMode = 1;
    ftpdThreadMain(); clean();
    assert(sleeps == 100 && ipReads == 100 && !sawListening && g_status == FTPD_FAILED);
    reset(); ipMode = 1; cancelSleep = 1;
    ftpdThreadMain(); clean();
    assert(sleeps == 1 && !sawListening && g_status == FTPD_STOPPED);
    reset(); ipMode = 3;
    ftpdThreadMain(); clean();
    assert(g_hostId == 0x0200007F && ipReads >= 2 && g_status == FTPD_STOPPED);
    reset(); pollMode = 1;
    ftpdThreadMain(); clean();
    assert(sawListening && g_status == FTPD_FAILED && g_error == -3 && sessions == 0);
    reset(); initResult = -31;
    ftpdThreadMain();
    assert(!ftpdEnabled && g_status == FTPD_FAILED && g_error == -31 && exitCalls == 0 && !sawListening);
    reset();
    for(int i = 0; i < 40; ++i)
    {
        assert(ftpdOpenPasvFixed());
        assert(ports[g_pasvSock] >= FTPD_PORT + 1 && ports[g_pasvSock] <= FTPD_PORT + 32);
        assert(listening[g_pasvSock] && reused[g_pasvSock]);
    }
    ftpdCloseData();
    for(int i = 0; i < nextFd; ++i) assert(!opened[i]);
    reset(); failStage = 3; failRemaining = 31;
    assert(ftpdOpenPasvFixed() && g_pasvPort == FTPD_PORT + 1);
    ftpdCloseData();
    for(int i = 0; i < nextFd; ++i) assert(!opened[i]);
    reset(); failStage = 4; failRemaining = 32;
    assert(!ftpdOpenPasvFixed() && g_pasvSock == -1 && closes == 32);
    for(int i = 0; i < nextFd; ++i) assert(!opened[i]);
    for(int ip = 1; ip <= 4; ip += 3)
    {
        reset(); assert(ftpdOpenPasvFixed());
        ipMode = ip;
        assert(!ftpdOpenPasvFixed() && g_pasvSock == -1 && g_hostId == (ip == 1 ? 0 : UINT32_MAX));
        for(int i = 0; i < nextFd; ++i) assert(!opened[i]);
    }
    puts("FTP production listener: delayed IP, every setup failure/recovery, cancel, IP change, interrupt, accept/session, PASV range and cleanup: OK");
    return 0;
}
"""
low_prefix = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t Result;
typedef int32_t s32;
typedef u32 socklen_t;
struct sockaddr { uint16_t sa_family; char sa_data[26]; };
"""
low_mocks = r"""
static u32 commands[128];
static u32 miniSocHandle = 1, SOCU_handle = 1, miniSocHostId;
static Result callResult;
static s32 serviceResult, networkResult;
static u32 *getThreadCommandBuffer(void) { return commands; }
#define IPC_MakeHeader(c,n,t) (((c) << 16) | ((n) << 6) | (t))
static s32 _net_convert_error(s32 result) { return result; }
static Result svcSendSyncRequest(u32 handle)
{
    assert(handle == 1);
    assert(commands[0] == 0x160000 || commands[0] == 0x80102);
    commands[1] = (u32)serviceResult;
    commands[2] = (u32)networkResult;
    return callResult;
}
"""
low_main = r"""
int main(void)
{
    callResult = 0; serviceResult = 0; networkResult = (s32)0xE001A8C0;
    assert(miniSocRefreshHostId() == 0xE001A8C0 && miniSocHostId == 0xE001A8C0);
    serviceResult = (s32)0xD8E007F7;
    assert(miniSocRefreshHostId() == 0 && miniSocHostId == 0);
    serviceResult = 0; callResult = -37;
    assert(miniSocRefreshHostId() == 0 && miniSocHostId == 0);
    callResult = 0; networkResult = 0;
    assert(miniSocRefreshHostId() == 0);
    networkResult = -1;
    assert(miniSocRefreshHostId() == UINT32_MAX);
#ifdef TEST_SMALL_RECV
    for(int mode = 0; mode < 4; ++mode)
    {
        callResult = mode == 1 ? -37 : 0;
        serviceResult = mode == 2 ? -44 : 0;
        networkResult = mode == 3 ? -11 : 5;
        for(u32 i = 64; i < 68; ++i) commands[i] = 0x11223300 + i;
        char data[32];
        ssize_t result = _socuipc_cmd8(7, data, sizeof(data), 0, NULL, NULL);
        assert(result == (mode == 0 ? 5 : mode == 1 ? -37 : -1));
        for(u32 i = 64; i < 68; ++i) assert(commands[i] == 0x11223300 + i);
    }
#endif
    puts("FTP raw host IP success/result distinction and small-recv TLS restoration: OK");
    return 0;
}
"""

def run(name, code, options=()):
    target = generated / (name + '.c')
    target.write_bytes(code.encode('utf-8'))
    exe = target.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-variable', '-Wno-pointer-to-int-cast', '-O2', *options, str(target), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)

for version in ('Luma3DS_v13_4',):
    base = root / version / 'sysmodules/rosalina/source'
    source = (base / 'menus/ftpd.c').read_text(encoding='utf-8')
    state = source[source.index('enum { FTPD_STOPPED'):source.index('static int ftpdOpenListener(')]
    code = ''.join(function(source, name) for name in ('ftpdOpenListener', 'ftpdCloseData', 'ftpdOpenPasvFixed', 'ftpdThreadMain'))
    run(version + '_ftp_listener', prefix + state + network + code + main)
    source = (base / 'minisoc.c').read_text(encoding='utf-8')
    code = function(source, 'miniSocRefreshHostId')
    options = []
    if version == 'Luma3DS_v13_4':
        code += function(source, '_socuipc_cmd8')
        options.append('-DTEST_SMALL_RECV')
    else:
        options.append('-Wno-unused-function')
    run(version + '_ftp_lowlevel', low_prefix + low_mocks + code + low_main, options)
