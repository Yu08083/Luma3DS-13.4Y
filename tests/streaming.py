import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
base = root / 'Luma3DS_v13_4/sysmodules/rosalina'
generated = root / 'tests/generated/streaming'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
source = (base / 'source/remote_play.c').read_text(encoding='utf-8')


def function(name, text=source):
    start = text.index(name + '(')
    start = text.rfind('\n', 0, start) + 1
    opened = text.index('{', start)
    depth, end = 1, opened + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def declaration(name):
    start = source.index('typedef struct ' + name + '\n')
    end = source.index('} ' + name + ';', start) + len('} ' + name + ';')
    return source[start:end] + '\n'


defines = '\n'.join(line for line in source.splitlines()
                    if line.startswith('#define ') and line.split()[1] in {
                        'NTR_MAGIC', 'NTR_HEADER_SIZE', 'NTR_CMD_HEARTBEAT', 'NTR_CMD_REMOTEPLAY',
                        'NTR_MAX_EXDATA', 'VIDEO_PACKET_SIZE', 'VIDEO_HEADER_SIZE', 'QOS_PER_MBPS',
                        'QOS_DEFAULT', 'QOS_MIN', 'QOS_MAX', 'QUALITY_DEFAULT', 'JPEG_SLOT_CAP',
                        'TICKS_PER_US'})
stub = generated / 'include/3ds/types.h'
stub.parent.mkdir(parents=True, exist_ok=True)
stub.write_bytes(b'#pragma once\n#include <stdint.h>\n#include <stdbool.h>\ntypedef uint8_t u8;\ntypedef uint16_t u16;\ntypedef uint32_t u32;\ntypedef uint64_t u64;\ntypedef int64_t s64;\ntypedef int32_t Result;\n')

prefix = r'''
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "remote_play.h"
#define SYSCLOCK_ARM11 268123480ULL
#define AF_INET 2
typedef int LightLock;
struct in_addr { u32 s_addr; };
struct sockaddr { int unused; };
struct sockaddr_in { int sin_family; u16 sin_port; struct in_addr sin_addr; };
static u16 htons(u16 v) { return (u16)((v << 8) | (v >> 8)); }
static void LightLock_Lock(LightLock *lock) { assert(*lock == 0); *lock = 1; }
static void LightLock_Unlock(LightLock *lock) { assert(*lock == 1); *lock = 0; }
static LightLock settingsLock;
static u32 viewerIp, settingMode, settingQuality, settingQos, settingsGeneration;
static RemotePlayStatus status;
static volatile bool stopRequested, linkDown;
static int videoSock = 42;
static u64 tick;
static u32 sleepCount, recvCalls, sendCalls, datagramCount, udpErrorAt, tcpLimit;
static int readError, writeError, udpError;
static u8 readBytes[1049000], writeBytes[4096], datagrams[128][1448];
static u32 readSize, readOffset, writeSize, datagramSizes[128];
static u64 datagramTicks[128], udpDelayTicks;
static struct sockaddr_in destinations[128];
static u64 svcGetSystemTick(void) { tick += 268; return tick; }
static void svcSleepThread(s64 ns)
{
    assert(ns >= 0 && ns <= 10000000);
    tick += (u64)ns * SYSCLOCK_ARM11 / 1000000000;
    ++sleepCount;
}
static int socRecv(int sock, void *data, u32 length, int flags)
{
    assert(sock == 17 && flags == 0 && length > 0);
    ++recvCalls;
    if(readError) { int result = readError; readError = 0; return result; }
    if(readOffset == readSize) return 0;
    u32 size = readSize - readOffset;
    if(size > length) size = length;
    if(size > tcpLimit) size = tcpLimit;
    memcpy(data, readBytes + readOffset, size);
    readOffset += size;
    return (int)size;
}
static int socSend(int sock, const void *data, u32 size, int flags)
{
    assert(sock == 17 && flags == 0 && size > 0);
    ++sendCalls;
    if(writeError) { int result = writeError; writeError = 0; return result; }
    if(size > tcpLimit) size = tcpLimit;
    assert(writeSize + size <= sizeof(writeBytes));
    memcpy(writeBytes + writeSize, data, size);
    writeSize += size;
    return (int)size;
}
static int socSendto(int sock, const void *data, u32 size, int flags,
                     const struct sockaddr *address, u32 addressSize)
{
    assert(sock == 42 && flags == 0 && addressSize == sizeof(struct sockaddr_in));
    assert(size >= 5 && size <= 1448 && datagramCount < 128);
    const struct sockaddr_in *target = (const struct sockaddr_in *)address;
    assert(target->sin_family == AF_INET && htons(target->sin_port) == 8001);
    u32 index = datagramCount++;
    memcpy(datagrams[index], data, size);
    datagramSizes[index] = size;
    destinations[index] = *target;
    datagramTicks[index] = tick;
    tick += udpDelayTicks;
    if(udpError && datagramCount == udpErrorAt) return udpError;
    return (int)size;
}
'''

main = r'''
static u32 cases;
static void reset(void)
{
    memset(&status, 0, sizeof(status));
    memset(&sender, 0, sizeof(sender));
    settingsLock = 0;
    viewerIp = settingMode = settingQuality = settingQos = settingsGeneration = 0;
    stopRequested = linkDown = false;
    tick = 1000000000;
    sleepCount = recvCalls = sendCalls = datagramCount = udpErrorAt = 0;
    readError = writeError = udpError = 0;
    readSize = readOffset = writeSize = 0;
    tcpLimit = 3;
}
static void feed(const void *data, u32 size)
{
    assert(readSize + size <= sizeof(readBytes));
    memcpy(readBytes + readSize, data, size);
    readSize += size;
}
static ControlConn connection(void)
{
    ControlConn value = { .sock = 17, .peerIp = 0x6401A8C0 };
    return value;
}
static NtrHeader header(u32 command)
{
    NtrHeader value = { .magic = NTR_MAGIC, .seq = 0xFEDCBA98, .type = 0, .cmd = command };
    return value;
}
static void consume(ControlConn *conn)
{
    while(readOffset < readSize)
    {
        u32 previous = readOffset;
        assert(readFromConn(conn));
        assert(readOffset > previous);
    }
}
static void clampTests(void)
{
    const u32 qualityValues[] = {0, 1, 10, 75, 100, 101, UINT32_MAX};
    const u32 qosValues[] = {0, 511999, 512000, 1048576, 4194304, 4194305, UINT32_MAX};
    const u32 modes[] = {0, 1, 0x101, 0xFF00, 0xFFFFFFFF};
    for(u32 a = 0; a < sizeof(qualityValues)/sizeof(*qualityValues); ++a)
        for(u32 b = 0; b < sizeof(qosValues)/sizeof(*qosValues); ++b)
            for(u32 c = 0; c < sizeof(modes)/sizeof(*modes); ++c)
            {
                reset();
                u32 quality = qualityValues[a], qos = qosValues[b], mode = modes[c];
                applySettings(0x6401A8C0, mode, quality, qos, c & 1);
                assert(settingQuality == (quality == 0 || quality > 100 ? 75 : quality));
                assert(settingQos == (qos < 512000 ? 1048576 : qos > 4194304 ? 4194304 : qos));
                assert(settingMode == mode && viewerIp == 0x6401A8C0 && settingsGeneration == 1);
                assert(status.state == REMOTEPLAY_STREAMING && status.viewerIp == viewerIp);
                assert(status.quality == settingQuality && status.qosBytesPerSecond == settingQos);
                assert(status.priorityFactor == (mode & 255));
                assert(status.topPriority == (((mode >> 8) & 255) != 0));
                assert(status.fromSavedViewer == ((c & 1) != 0));
                assert(settingsLock == 0);
                ++cases;
            }
    reset(); settingsGeneration = UINT32_MAX;
    applySettings(1, 0, 75, 1048576, false);
    assert(settingsGeneration == 0);
    ++cases;
}
static void controlTests(void)
{
    _Static_assert(sizeof(NtrHeader) == 84, "header size");
    _Static_assert(offsetof(NtrHeader, args) == 16, "args offset");
    _Static_assert(offsetof(NtrHeader, exdataLen) == 80, "data length offset");
    for(u32 limit = 1; limit <= 90; ++limit)
    {
        reset(); tcpLimit = limit;
        ControlConn conn = connection();
        NtrHeader input = header(0);
        const u8 golden[16] = {0x78,0x56,0x34,0x12,0x98,0xBA,0xDC,0xFE,0,0,0,0,0,0,0,0};
        assert(memcmp(&input, golden, sizeof(golden)) == 0);
        feed(&input, sizeof(input)); consume(&conn);
        assert(conn.headerGot == 0 && conn.replySize == 84 && conn.replySent == 0);
        while(conn.replySize) assert(writeToConn(&conn));
        assert(conn.replySent == 0 && writeSize == 84 && memcmp(writeBytes, &input, 84) == 0);
        assert(status.state == REMOTEPLAY_OFF && settingsGeneration == 0);
        assert(!readFromConn(&conn));
        ++cases;
    }
    const int transients[] = {-EAGAIN, -EWOULDBLOCK, -EINTR};
    for(u32 i = 0; i < sizeof(transients)/sizeof(*transients); ++i)
    {
        reset(); ControlConn conn = connection(); NtrHeader input = header(0);
        feed(&input, 84);
        readError = transients[i]; assert(readFromConn(&conn));
        assert(conn.headerGot == 0 && readOffset == 0 && conn.lastProgress == 0);
        assert(readFromConn(&conn)); assert(conn.headerGot == 3);
        readError = transients[i]; assert(readFromConn(&conn)); assert(conn.headerGot == 3);
        consume(&conn);
        assert(conn.replySize == 84);
        writeError = transients[i]; assert(writeToConn(&conn)); assert(conn.replySent == 0);
        assert(writeToConn(&conn)); assert(conn.replySent == 3);
        writeError = transients[i]; assert(writeToConn(&conn)); assert(conn.replySent == 3);
        while(conn.replySize) assert(writeToConn(&conn));
        assert(writeSize == 84 && memcmp(writeBytes, &input, 84) == 0);
        ++cases;
    }
    for(int field = 0; field < 5; ++field)
    {
        reset(); ControlConn conn = connection(); NtrHeader input = header(901);
        if(field == 0) input.magic ^= 1;
        if(field == 1) input.type = 1;
        if(field == 2) input.exdataLen = (1u << 20) + 1;
        if(field == 3) input.exdataLen = 1;
        if(field == 4) { input.cmd = 0; input.exdataLen = 1; }
        memcpy(conn.header, &input, 84);
        assert(!handlePacket(&conn) && settingsGeneration == 0 && conn.replySize == 0);
        ++cases;
    }
    reset();
    ControlConn conn = connection(); NtrHeader input = header(0);
    memcpy(conn.header, &input, 84);
    assert(handlePacket(&conn) && !handlePacket(&conn));
    ++cases;
    for(u32 length = 0; length <= (1u << 20); length = length == 0 ? 1 : length == 1 ? 257 : (length == 257 ? 1u << 20 : (1u << 20) + 1))
    {
        reset(); tcpLimit = 257; conn = connection();
        input = header(10); input.exdataLen = length;
        feed(&input, 84);
        memset(readBytes + readSize, 0xA5, length); readSize += length;
        input = header(901); input.args[0] = 0x105; input.args[1] = 91; input.args[2] = 2000000;
        feed(&input, 84); consume(&conn);
        assert(conn.exdataLeft == 0 && conn.headerGot == 0);
        assert(settingsGeneration == 1 && viewerIp == conn.peerIp && settingMode == 0x105);
        assert(settingQuality == 91 && settingQos == 2000000 && conn.replySize == 0);
        ++cases;
    }
    const u32 unsupported[] = {1, 9, 10, 11, 1000, 1001, 1002, UINT32_MAX};
    for(u32 i = 0; i < sizeof(unsupported)/sizeof(*unsupported); ++i)
    {
        reset(); conn = connection(); input = header(unsupported[i]);
        memset(input.args, 0xFF, sizeof(input.args));
        memcpy(conn.header, &input, 84);
        assert(handlePacket(&conn) && settingsGeneration == 0 && conn.replySize == 0);
        ++cases;
    }
    reset(); conn = connection(); conn.exdataLeft = 100;
    readError = -EINTR; assert(readFromConn(&conn) && conn.exdataLeft == 100);
    assert(!readFromConn(&conn));
    readError = -ECONNRESET; assert(!readFromConn(&conn));
    input = header(0); memcpy(conn.header, &input, 84); assert(handlePacket(&conn));
    writeError = -ECONNRESET; assert(!writeToConn(&conn) && conn.replySent == 0);
    ++cases;
}
static void udpTests(void)
{
    const u32 sizes[] = {1, 2, 1443, 1444, 1445, 2888, 2889, 65536, 131072};
    static JpegSlot slot;
    for(u32 sizeIndex = 0; sizeIndex < sizeof(sizes)/sizeof(*sizes); ++sizeIndex)
        for(int top = 0; top <= 1; ++top)
            for(u32 frame = 1; frame <= 255; frame += 127)
            {
                reset(); memset(&slot, 0, sizeof(slot));
                slot.size = sizes[sizeIndex]; slot.top = top; slot.frameId = frame;
                slot.destIp = 0x6401A8C0;
                for(u32 i = 0; i < slot.size; ++i) slot.data[i] = (u8)(i * 37 + i / 1444);
                assert(sendFrame(&slot) == 1);
                assert(datagramCount == (slot.size + 1443) / 1444);
                u32 copied = 0;
                for(u32 i = 0; i < datagramCount; ++i)
                {
                    u32 length = datagramSizes[i];
                    assert(datagrams[i][0] == frame && datagrams[i][2] == 2 && datagrams[i][3] == i);
                    assert(datagrams[i][1] == (top | (i + 1 == datagramCount ? 0x10 : 0)));
                    assert(destinations[i].sin_addr.s_addr == slot.destIp);
                    assert(length <= 1448 && length > 4);
                    if(i + 1 < datagramCount) assert(length == 1448);
                    assert(memcmp(datagrams[i] + 4, slot.data + copied, length - 4) == 0);
                    copied += length - 4;
                }
                assert(copied == slot.size && status.packets == datagramCount && status.sendErrors == 0);
                ++cases;
            }
    const int errors[] = {-EAGAIN, -EWOULDBLOCK, -EINTR, -ECONNRESET, 7};
    for(u32 error = 0; error < sizeof(errors)/sizeof(*errors); ++error)
        for(u32 position = 1; position <= 4; ++position)
        {
            reset(); memset(&slot, 0, sizeof(slot));
            slot.size = 5000; slot.frameId = 5; slot.destIp = 0x6401A8C0;
            udpError = errors[error]; udpErrorAt = position;
            int expected = error < 3 ? 0 : -1;
            assert(sendFrame(&slot) == expected);
            assert(datagramCount == position && status.sendErrors == 1);
            ++cases;
        }
    for(int mode = 0; mode < 2; ++mode)
    {
        reset(); slot.size = 5000; stopRequested = mode == 0; linkDown = mode == 1;
        assert(sendFrame(&slot) == 0 && datagramCount == 0);
        ++cases;
    }
    reset(); slot.size = 5000; slot.packetIntervalTicks = SYSCLOCK_ARM11 / 1000;
    sender.lastPacketTick = tick;
    assert(sendFrame(&slot) == 1 && datagramCount == 4 && sleepCount == 4 && status.sleepUs > 0);
    ++cases;
    const u32 delayUs[] = {0, 100, 900, 1500, 5000};
    for(u32 delay = 0; delay < sizeof(delayUs) / sizeof(*delayUs); ++delay)
    {
        reset(); slot.size = 5000; slot.packetIntervalTicks = SYSCLOCK_ARM11 / 1000;
        sender.lastPacketTick = tick;
        udpDelayTicks = (u64)delayUs[delay] * SYSCLOCK_ARM11 / 1000000;
        assert(sendFrame(&slot) == 1 && datagramCount == 4);
        u64 minimum = slot.packetIntervalTicks > udpDelayTicks ? slot.packetIntervalTicks : udpDelayTicks;
        for(u32 i = 1; i < datagramCount; ++i)
        {
            u64 spacing = datagramTicks[i] - datagramTicks[i - 1];
            assert(spacing >= slot.packetIntervalTicks && spacing >= udpDelayTicks);
            assert(spacing <= minimum + 3 * TICKS_PER_US);
        }
        assert(sender.lastPacketTick == datagramTicks[datagramCount - 1]);
        ++cases;
    }
    udpDelayTicks = 0;
}
int main(void)
{
    clampTests(); controlTests(); udpTests();
    printf("{\"result\":\"pass\",\"cases\":%u,\"tcp_header_bytes\":84,\"control_port\":8000,\"udp_port\":8001,\"udp_packet_bytes\":1448}\n", cases);
    return 0;
}
'''
parts = [prefix, defines, '\n', declaration('NtrHeader'), declaration('ControlConn'),
         declaration('JpegSlot'), declaration('SenderState'),
         'static SenderState sender;\n#define snd sender\n',
         ''.join(function(name) for name in ('statusIncrement', 'ticksToUs', 'transient', 'applySettings',
                 'handleRemotePlayRequest', 'handlePacket', 'readFromConn', 'writeToConn', 'sendFrame')),
         main]
target = generated / 'protocol.c'
target.write_bytes(''.join(parts).encode('utf-8'))
exe = target.with_suffix('.exe' if os.name == 'nt' else '')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2',
                '-I' + str(generated / 'include'), '-I' + str(base / 'include'),
                str(target), '-o', str(exe)], check=True, env=env)
run = subprocess.run([str(exe)], check=True, capture_output=True, text=True, env=env)
report = json.loads(run.stdout)
low_source = (base / 'source/minisoc.c').read_text(encoding='utf-8')
low_prefix = r'''
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t Result;
typedef u32 socklen_t;
struct sockaddr { u16 sa_family; char sa_data[26]; };
#define AF_INET 2
#define IPC_BUFFER_W 2
#define IPC_BUFFER_R 1
#define IPC_MakeHeader(c,n,t) (((c)<<16)|((n)<<6)|(t))
#define IPC_Desc_CurProcessId() 0x20
#define IPC_Desc_StaticBuffer(n,i) (((n)<<14)|((i)<<10)|2)
#define IPC_Desc_Buffer(n,i) (((n)<<4)|((i)<<1)|8)
static u32 miniSocHandle = 77, commands[128], storage[8], expectedCommand, svcCalls;
static int networkResult, serviceResult;
static u32 *getThreadCommandBuffer(void) { return commands; }
static u32 *getThreadStaticBuffers(void) { return storage; }
static int _net_convert_error(int value) { return value; }
static Result svcSendSyncRequest(u32 handle)
{
    assert(handle == miniSocHandle && commands[0] >> 16 == expectedCommand);
    ++svcCalls;
    commands[1] = (u32)serviceResult;
    commands[2] = (u32)networkResult;
    return 0;
}
'''
low_main = r'''
int main(void)
{
    const int results[] = {-EAGAIN, -EWOULDBLOCK, -EINTR, -ECONNRESET, 0, 7};
    char bytes[32];
    u32 cases = 0;
    for(u32 variant = 0; variant < 2; ++variant)
        for(u32 index = 0; index < sizeof(results)/sizeof(*results); ++index)
        {
            networkResult = variant ? 123 : results[index];
            serviceResult = variant ? results[index] : 0;
            int expected = serviceResult ? serviceResult : networkResult;
            for(u32 word = 0; word < 128; ++word) commands[word] = 0x76540000 + word;
            for(u32 word = 0; word < 8; ++word) storage[word] = 0xABC00000 + word;
            expectedCommand = 4;
            assert(socAccept(5, NULL, NULL) == expected);
            for(u32 word = 0; word < 8; ++word) assert(storage[word] == 0xABC00000 + word);
            expectedCommand = 7;
            assert(_socuipc_cmd7(5, bytes, sizeof(bytes), 0, NULL, NULL) == expected);
            for(u32 word = 0; word < 8; ++word) assert(storage[word] == 0xABC00000 + word);
            expectedCommand = 8;
            assert(_socuipc_cmd8(5, bytes, sizeof(bytes), 0, NULL, NULL) == expected);
            for(u32 word = 64; word < 68; ++word) assert(commands[word] == 0x76540000 + word);
            expectedCommand = 9;
            assert(_socuipc_cmd9(5, bytes, sizeof(bytes), 0, NULL, 0) == expected);
            expectedCommand = 10;
            assert(_socuipc_cmda(5, bytes, sizeof(bytes), 0, NULL, 0) == expected);
            cases += 5;
        }
    assert(svcCalls == cases);
    for(u32 length = 0; length < 256; ++length)
        for(u32 capacity = 0; capacity <= 35; ++capacity)
        {
            u8 source[28], output[48], expected[48];
            memset(source, 0x67, sizeof(source));
            source[0] = length; source[1] = AF_INET;
            memset(output, 0xA5, sizeof(output));
            memcpy(expected, output, sizeof(expected));
            socklen_t size = capacity;
            copySockaddr((struct sockaddr *)(void *)(output + 8), &size, source);
            if(length >= 2 && length <= 28)
            {
                assert(size == length);
                for(u32 byte = 0; byte < capacity && byte < length; ++byte)
                    expected[8 + byte] = byte == 0 ? AF_INET : byte == 1 ? 0 : 0x67;
            }
            else
                assert(size == 0);
            assert(memcmp(output, expected, sizeof(output)) == 0);
            ++cases;
        }
    u8 address[28] = {8, AF_INET};
    socklen_t size = 0x12345678;
    copySockaddr(NULL, &size, address);
    assert(size == 0x12345678);
    copySockaddr((struct sockaddr *)(void *)address, NULL, address);
    printf("%u\n", cases);
    return 0;
}
'''
low_target = generated / 'minisoc.c'
low_target.write_bytes((low_prefix + ''.join(function(name, low_source) for name in
    ('copySockaddr', 'socAccept', '_socuipc_cmd7', '_socuipc_cmd8', '_socuipc_cmd9', '_socuipc_cmda')) + low_main).encode('utf-8'))
low_exe = low_target.with_suffix('.exe' if os.name == 'nt' else '')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-pointer-to-int-cast',
                '-O2', str(low_target), '-o', str(low_exe)], check=True, env=env)
low_run = subprocess.run([str(low_exe)], check=True, capture_output=True, text=True, env=env)
report['soc_ipc_cases'] = int(low_run.stdout.strip())
report['source_sha256'] = hashlib.sha256((base / 'source/remote_play.c').read_bytes()).hexdigest()
report['minisoc_sha256'] = hashlib.sha256((base / 'source/minisoc.c').read_bytes()).hexdigest()
report['hardware_connected'] = False
report['actual_network_used'] = False
(generated / 'proof.json').write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))
print(json.dumps(report))
