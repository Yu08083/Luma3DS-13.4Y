import hashlib
import json
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
generated = root / 'tests/generated/adorable-ntr'
generated.mkdir(parents=True, exist_ok=True)
setup = (root / 'tests/streaming.py').read_text(encoding='utf-8').split("target = generated / 'protocol.c'", 1)[0]
setup = setup.replace("generated = root / 'tests/generated/streaming'", "generated = root / 'tests/generated/adorable-ntr'")
namespace = {'__file__': str(root / 'tests/streaming.py')}
exec(compile(setup, str(root / 'tests/streaming.py'), 'exec'), namespace)
source = namespace['source']
function = namespace['function']
fixture_dir = root / 'tests/fixtures/adorable_ntr'
packets = json.loads((fixture_dir / 'packets.json').read_text(encoding='utf-8'))
assert packets['commit'] == '4be9fd70dca6e5de914bd83b3091a359b98b24ef'
assert packets['source_sha256'] == 'a4007eaf48dcfab23c57aff7bbb1d68dff31c858e4680e3c2eb4beb9547ddffc'
assert packets['events'] == [['write', 84], ['write', 84], ['end'], ['destroy'], ['stop-heartbeat'], ['reconnect-delay', 3000]]
wire = [bytes.fromhex(value) for value in packets['packets']]
assert len(wire) == 2 and all(len(value) == 84 for value in wire)
arrays = '\n'.join('static const u8 adorablePacket%d[84] = {%s};' % (index, ','.join(str(v) for v in data)) for index, data in enumerate(wire))
protocol = ''.join(namespace['parts']).replace('int main(void)', 'int protocol_fixture_unused_main(void)')
platform = r'''
#define POLLIN 1
#define POLLOUT 4
#define POLLERR 8
#define POLLHUP 16
#define POLLNVAL 32
#define SOL_SOCKET 0xFFFF
#define SO_LINGER 0x80
#define CONTROL_POLL_INTERVAL_MS 10
#define LINK_CHECK_INTERVAL_US 1000000
typedef u32 nfds_t;
typedef u32 socklen_t;
struct pollfd { int fd; short events, revents; };
struct linger { int l_onoff, l_linger; };
static volatile bool preTerminationRequested, checkLinkNow;
static int listenSock;
static u32 pollCalls, connectionPolls, closeCalls, optionCalls, accepts;
static int hangupMode, cancelMode, forcedEvent, pollError;
static bool accepted;
static bool Sleep__IsSleeping(void) { return false; }
static void checkLink(ControlConn *conn) { (void)conn; checkLinkNow = false; }
static int openListenSocket(void) { assert(false); return -1; }
static int socSetsockopt(int sock, int level, int option, const void *value, u32 size)
{
    assert(sock == 17 && level == SOL_SOCKET && option == SO_LINGER && size == sizeof(struct linger));
    const struct linger *linger = value;
    assert(linger->l_onoff == 1 && linger->l_linger == 0);
    ++optionCalls;
    return 0;
}
static int socClose(int sock) { assert(sock == 17 && closeCalls == 0); ++closeCalls; return 0; }
static int socSetNonBlocking(int sock) { assert(sock == 17); return 0; }
static int socAccept(int sock, struct sockaddr *address, socklen_t *size)
{
    assert(sock == 16 && *size == sizeof(struct sockaddr_in) && !accepted);
    struct sockaddr_in *addr = (struct sockaddr_in *)address;
    memset(addr, 0, sizeof(*addr));
    addr->sin_addr.s_addr = 0x6401A8C0;
    accepted = true;
    ++accepts;
    return 17;
}
static int socPoll(struct pollfd *fds, nfds_t count, int timeout)
{
    assert(++pollCalls < 5000 && timeout == 0 && fds[0].fd == 16 && fds[0].events == POLLIN);
    fds[0].revents = 0;
    if (!accepted) { assert(count == 1); fds[0].revents = POLLIN; return 1; }
    if (count == 1) { assert(closeCalls == 1); stopRequested = true; return 0; }
    assert(count == 2 && fds[1].fd == 17);
    ++connectionPolls;
    fds[1].revents = 0;
    if (cancelMode && connectionPolls == 1)
    {
        if (cancelMode == 1) stopRequested = true;
        else preTerminationRequested = true;
        fds[1].revents = POLLIN | POLLOUT | POLLHUP;
        return 1;
    }
    if (pollError) { int result = pollError; pollError = 0; return result; }
    if (forcedEvent) { fds[1].revents = forcedEvent; return 1; }
    bool hung = hangupMode == 1 || hangupMode == 3 || (hangupMode == 2 && readOffset >= 84);
    if (readOffset < readSize && (fds[1].events & POLLIN) && hangupMode != 3)
        fds[1].revents |= POLLIN;
    if ((fds[1].events & POLLOUT) && !hung)
        fds[1].revents |= POLLOUT;
    if (hung) fds[1].revents |= POLLHUP;
    if (fds[1].revents) return 1;
    stopRequested = true;
    return 0;
}
'''
checks = r'''
static u32 compatibilityCases;
static void scenario(u32 fragment, int hup)
{
    reset();
    tcpLimit = fragment;
    listenSock = 16;
    preTerminationRequested = checkLinkNow = false;
    pollCalls = connectionPolls = closeCalls = optionCalls = accepts = 0;
    hangupMode = hup;
    cancelMode = forcedEvent = pollError = 0;
    accepted = false;
}
static void run(bool legacy, bool enabled)
{
    if (legacy) legacy_controlThreadMain();
    else controlThreadMain();
    assert(accepts == 1 && closeCalls == 1 && optionCalls == 1);
    assert(viewerIp == (enabled ? 0x6401A8C0U : 0));
    if (enabled)
    {
        assert(settingMode == 0x105 && settingQuality == 60 && settingQos == QOS_DEFAULT);
        assert(status.state == REMOTEPLAY_STREAMING && !status.fromSavedViewer);
    }
    ++compatibilityCases;
}
static void feedActual(void)
{
    feed(adorablePacket0, sizeof(adorablePacket0));
    feed(adorablePacket1, sizeof(adorablePacket1));
}
static void compatibility(void)
{
    scenario(84, 1); feedActual(); run(true, false); assert(recvCalls == 0);
    scenario(84, 2); feedActual(); run(true, false); assert(readOffset == 84);
    const u32 fragments[] = {1, 3, 83, 84, 168};
    for (u32 i = 0; i < sizeof(fragments) / sizeof(fragments[0]); ++i)
        for (int hup = 1; hup <= 3; ++hup)
        {
            scenario(fragments[i], hup); feedActual(); run(false, true);
            assert(readOffset == readSize && writeSize == 0);
        }
    scenario(3, 0); feedActual(); run(false, true);
    assert(writeSize == 84 && !memcmp(writeBytes, adorablePacket0, 84));
    scenario(3, 0); feedActual(); readError = -EAGAIN; writeError = -EWOULDBLOCK; run(false, true);
    assert(writeSize == 84 && !memcmp(writeBytes, adorablePacket0, 84));
    scenario(1, 0); feedActual(); readError = -EINTR; writeError = -EINTR; run(false, true);
    assert(writeSize == 84 && !memcmp(writeBytes, adorablePacket0, 84));
    scenario(84, 1); feedActual(); readError = -EINTR; run(false, true);
    scenario(84, 0); feedActual(); writeError = -EPIPE; run(false, true); assert(sendCalls == 1);
    for (int cancel = 1; cancel <= 2; ++cancel)
    {
        scenario(84, 0); feed(adorablePacket1, 84); cancelMode = cancel; run(false, false);
        assert(recvCalls == 0 && sendCalls == 0);
    }
    for (u32 i = 0; i < 5; ++i)
    {
        scenario(84, 1);
        NtrHeader invalid; memcpy(&invalid, adorablePacket1, 84);
        if (i == 0) invalid.magic ^= 1;
        if (i == 1) invalid.type = 1;
        if (i == 2) invalid.exdataLen = NTR_MAX_EXDATA + 1;
        if (i == 3) invalid.exdataLen = 1;
        if (i == 4) { invalid.cmd = NTR_CMD_HEARTBEAT; invalid.exdataLen = 1; }
        feed(&invalid, 84); feed(adorablePacket1, 84); run(false, false); assert(readOffset == 84);
    }
    scenario(3, 1);
    NtrHeader unknown = header(0x1234); unknown.exdataLen = 257;
    u8 body[257]; memset(body, 0xA5, sizeof(body));
    feed(&unknown, 84); feed(body, sizeof(body)); feed(adorablePacket1, 84);
    run(false, true); assert(readOffset == readSize);
    scenario(84, 0); feedActual(); forcedEvent = POLLNVAL; run(false, false); assert(recvCalls == 0);
    scenario(84, 0); feedActual(); forcedEvent = POLLERR; run(false, true);
    scenario(84, 1); feedActual(); readError = -EAGAIN; run(false, false); assert(recvCalls == 1);
    scenario(84, 3); feedActual(); readError = -ECONNRESET; run(false, false); assert(recvCalls == 1);
    scenario(84, 0); feedActual(); pollError = -20000; run(false, false); assert(recvCalls == 0);
    scenario(84, 1); feed(adorablePacket1, 83); run(false, false); assert(readOffset == 83);
    printf("{\"cases\":%u,\"legacy_failures_reproduced\":2,\"actual_app_packets\":2,\"live_device_tested\":false}\n", compatibilityCases);
}
int main(void) { compatibility(); return 0; }
'''
legacy = (fixture_dir / 'control_before_fix5.c').read_text(encoding='utf-8')
translation = '\n'.join((protocol, platform, function('closeSocket'), legacy,
                         function('controlThreadMain'), arrays, checks))
target = generated / 'adorable_ntr.c'
target.write_bytes(translation.encode('utf-8'))
exe = target.with_suffix('.exe' if os.name == 'nt' else '')
subprocess.run([namespace['compiler'], '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2',
                '-I' + str(generated / 'include'), '-I' + str(namespace['base'] / 'include'),
                str(target), '-o', str(exe)], check=True, env=namespace['env'])
run = subprocess.run([str(exe)], capture_output=True, text=True, env=namespace['env'])
if run.returncode:
    raise RuntimeError(run.stderr or ('fixture exit ' + str(run.returncode)))
report = json.loads(run.stdout)
report['result'] = 'pass'
report['remote_play_sha256'] = hashlib.sha256(source.encode('utf-8')).hexdigest()
report['app'] = {key: packets[key] for key in ('repository', 'commit', 'release', 'source', 'source_sha256', 'method')}
(generated / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))
print(json.dumps(report, indent=2))
