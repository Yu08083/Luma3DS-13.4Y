import os
import re
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
        if source[end] == '{':
            depth += 1
        if source[end] == '}':
            depth -= 1
        end += 1
    return source[start:end] + '\n'

prefix = r'''
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t s64;
typedef int32_t Result;
typedef u32 socklen_t;
typedef struct { u32 handle; u64 pos; } IFile;
typedef struct { u32 handle; } MyThread;
struct pollfd { int fd, events, revents; };
struct sockaddr { int unused; };
struct sockaddr_in { int unused; };
#define POLLIN 1
#define POLLOUT 2
#define POLLERR 4
#define POLLHUP 8
#define POLLNVAL 16
#define FTPD_PORT 5000
#define SYSCLOCK_ARM11 268123480
#define ARCHIVE_SDMC 0
#define PATH_EMPTY 0
#define PATH_ASCII 1
#define FS_OPEN_READ 1
#define FS_OPEN_WRITE 2
#define FS_OPEN_CREATE 4
#define COLOR_RED 1
#define SPACING_Y 11
#define fsMakePath(type, path) (path)
#define R_FAILED(result) ((result) < 0)
#define R_SUCCEEDED(result) ((result) >= 0)
#define KEY_A 1
#define KEY_B 2
#define BUTTON_A KEY_A
#define BUTTON_B KEY_B
enum { STR_ANY_USERNAME_AND_PASSWORD, STR_PASSIVE_MODE_ONLY, STR_KEEPS_RUNNING_WITH_THE_MENU_CLOSED,
       STR_SERVES_THE_SD_CARD_OVER_FTP_SO_YOU, STR_CAN_MANAGE_FILES_FROM_A_PC, STR_WI_FI_MUST_BE_CONNECTED_FIRST,
       STR_A_STOP_B_BACK, STR_A_START_B_BACK };
static bool menuShouldExit, terminationRequest;
static volatile bool ftpdEnabled;
static int g_dataSock = 3, g_pasvSock = 4;
static char g_scratch[512], g_cwd[512], g_xfer[32768], reply[256];
static MyThread ftpdThread;
static u32 g_accentColor, g_textColor, g_menuBgColor;
static u32 now, polls, recvCalls, flushes, writes, clears, renders, frames, sleeps, creates;
static u32 keyIndex, keyList[4];
static int mode, receiveMode, readMode, joinTimeout;
static bool partialWrite;
static u64 fileSize = 6;
static uint64_t svcGetSystemTick(void) { return (uint64_t)now * (SYSCLOCK_ARM11 / 1000); }
'''
mocks = r'''
static int socPoll(struct pollfd *fd, int count, int timeout)
{
    assert(count == 1 && timeout == 100);
    ++polls; now += 100;
    if(mode == 1) { ftpdEnabled = false; return 0; }
    if(mode == 2) { fd->revents = POLLNVAL; return 1; }
    if(mode == 3) return 0;
    fd->revents = fd->events;
    return 1;
}
static int socAccept(int socket, struct sockaddr *address, socklen_t *length)
{ assert(socket == 4); (void)address; (void)length; return 3; }
static void socClose(int socket) { (void)socket; }
static bool ftpdResolve(char *dst, u32 size, const char *cwd, const char *arg)
{ (void)size; (void)cwd; strcpy(dst, arg); return true; }
static void ftpdReply(int ctrl, const char *fmt, ...)
{
    (void)ctrl;
    va_list args; va_start(args, fmt); vsnprintf(reply, sizeof(reply), fmt, args); va_end(args);
}
static Result IFile_Open(IFile *file, u32 archive, const char *empty, const char *path, u32 flags)
{ (void)archive; (void)empty; (void)path; (void)flags; file->handle = 1; file->pos = 0; return 0; }
static Result IFile_GetSize(IFile *file, u64 *size) { (void)file; *size = fileSize; return 0; }
static Result IFile_Read(IFile *file, u64 *read, void *buffer, u32 size)
{
    (void)buffer; assert(size == sizeof(g_xfer));
    if(readMode == 1) return -1;
    *read = readMode == 2 ? 0 : 3;
    file->pos += *read;
    return 0;
}
static Result IFile_Write(IFile *file, u64 *written, const void *data, u32 size, u32 flags)
{ (void)data; assert(flags == 0); ++writes; *written = partialWrite ? size - 1 : size; file->pos += *written; return 0; }
static Result FSFILE_Flush(u32 handle) { assert(handle == 1); ++flushes; return 0; }
static void IFile_Close(IFile *file) { (void)file; }
static void ftpdCloseData(void) { g_dataSock = g_pasvSock = -1; }
static Result ftpdOpenArchive(u32 *archive) { *archive = 1; return 0; }
typedef u32 FS_Archive;
static void FSUSER_DeleteFile(FS_Archive archive, const char *path) { (void)archive; (void)path; }
static void FSUSER_CloseArchive(FS_Archive archive) { (void)archive; }
static ssize_t socSend(int socket, const void *data, u32 length, int flags)
{ (void)data; (void)flags; assert(socket == 3 && length > 0); return length > 2 ? 2 : (ssize_t)length; }
static ssize_t socRecv(int socket, void *data, u32 size, int flags)
{
    (void)data; (void)flags; assert(socket == 3 && size == sizeof(g_xfer));
    if(receiveMode == 1) ftpdEnabled = false;
    return recvCalls++ < 2 ? 3 : 0;
}
#define soc_send socSend
#define soc_recv socRecv
static Result MyThread_Join(MyThread *thread, s64 timeout)
{ (void)thread; joinTimeout = (int)timeout; assert(timeout == 0); return -1; }
static MyThread *ftpdCreateThread(void) { ++creates; ftpdThread.handle = 9; return &ftpdThread; }
static const char *Lang(u32 id) { (void)id; return "text"; }
static const char *Lang_Translate(const char *text) { return text; }
static void Draw_Lock(void) {}
static void Draw_Unlock(void) {}
static void Draw_ClearFramebuffer(void) { ++clears; }
static void Draw_FlushFramebuffer(void) { ++frames; }
static u32 Draw_DrawString(u32 x, u32 y, u32 color, const char *text)
{ (void)x; (void)color; (void)text; ++renders; return y; }
static void Draw_DrawFormattedString(u32 x, u32 y, u32 color, const char *text, ...)
{ (void)x; (void)y; (void)color; (void)text; ++renders; }
static void Draw_FillRect(u32 x, u32 y, u32 width, u32 height, u32 color)
{ (void)x; (void)y; (void)width; (void)height; (void)color; }
static u32 waitInputWithTimeout(u32 timeout) { assert(timeout == 100 && keyIndex < 2); return keyList[keyIndex++]; }
'''
main = r'''
static void reset(void)
{
    ftpdEnabled = true; mode = receiveMode = readMode = 0;
    partialWrite = false; now = polls = recvCalls = writes = flushes = 0;
    g_dataSock = 3; g_pasvSock = 4; g_bytes = g_total = 0;
    reply[0] = 0;
}
int main(void)
{
    reset(); mode = 1;
    assert(!ftpdAcceptData() && now == 100);
    reset(); mode = 3;
    assert(!ftpdAcceptData() && now >= 10000 && now < 10200);
    reset(); mode = 2;
    assert(!ftpdWait(3, POLLIN) && polls == 1);
    reset();
    ftpdCmdRetr(1, "/file");
    assert(strncmp(reply, "226", 3) == 0 && g_transfers == 1 && g_bytes == 6 && g_status == FTPD_CONNECTED);
    reset(); readMode = 1;
    ftpdCmdRetr(1, "/file");
    assert(strncmp(reply, "426", 3) == 0 && g_transfers == 1);
    reset(); readMode = 2;
    ftpdCmdRetr(1, "/file");
    assert(strncmp(reply, "426", 3) == 0 && g_transfers == 1);
    reset();
    ftpdCmdStor(1, "/file", false);
    assert(strncmp(reply, "226", 3) == 0 && g_transfers == 2 && writes == 2 && flushes == 1 && g_bytes == 6);
    reset(); partialWrite = true;
    ftpdCmdStor(1, "/file", false);
    assert(strncmp(reply, "426", 3) == 0 && g_transfers == 2 && writes == 1 && flushes == 1);
    reset(); receiveMode = 1;
    ftpdCmdStor(1, "/file", true);
    assert(strncmp(reply, "426", 3) == 0 && g_transfers == 2);
    FtpdView previous = {0}, view = { .status = FTPD_LISTENING, .ip = 0x0100007F };
    clears = renders = frames = 0;
    ftpdDrawView(&view, &previous, true);
    assert(clears == 1 && frames == 1);
    previous = view;
    u32 before = renders;
    ftpdDrawView(&view, &previous, false);
    assert(renders == before && frames == 1);
    view.status = FTPD_SENDING; view.bytes = 4096; view.clients = 1;
    ftpdDrawView(&view, &previous, false);
    assert(clears == 1 && frames == 2);
    ftpdEnabled = false; ftpdThread.handle = 0; g_status = FTPD_STOPPED;
    keyIndex = 0; keyList[0] = KEY_A; keyList[1] = KEY_B;
    RosalinaMenu_Ftpd();
    assert(creates == 1 && g_status == FTPD_STARTING && ftpdEnabled && sleeps == 0);
    keyIndex = 0; joinTimeout = -1;
    RosalinaMenu_Ftpd();
    assert(!ftpdEnabled && g_status == FTPD_STOPPING && joinTimeout == 0);
    puts("FTP cancelable polling, partial I/O, completion flush, progress states, stable idle view and nonblocking controls: OK");
    return 0;
}
'''

for version in ('Luma3DS_v13_4',):
    base = root / version / 'sysmodules/rosalina'
    source = (base / 'source/menus/ftpd.c').read_text(encoding='utf-8')
    state = source[source.index('enum { FTPD_STOPPED'):source.index('static int ftpdOpenListener(')]
    code = ''.join(function(source, name) for name in ('ftpdWait', 'ftpdAcceptData', 'ftpdCmdRetr', 'ftpdCmdStor', 'Ftpd_Disable'))
    code += source[source.index('typedef struct {\n    u32 status, ip, clients'):]
    target = generated / (version + '_ftp.c')
    target.write_text(prefix + state + mocks + code + main, encoding='utf-8', newline='\n')
    exe = target.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-variable', '-O2', str(target), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
