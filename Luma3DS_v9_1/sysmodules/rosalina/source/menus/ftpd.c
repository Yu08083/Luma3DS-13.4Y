/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2021 Aurora Wright, TuxSH
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
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include <3ds.h>
#include <arpa/inet.h>
#include "menus/ftpd.h"
#include "menus/lang.h"
#include "minisoc.h"
#include "draw.h"
#include "fmt.h"
#include "ifile.h"
#include "memory.h"
#include "utils.h"

#define FTPD_CMD_MAX     1024
#define FTPD_PATH_MAX    512
#define FTPD_XFER_SIZE   0x8000
#define FTPD_LIST_MAX    1024

bool ftpdEnabled = false;

static MyThread ftpdThread;
static u8 ALIGN(8) ftpdThreadStack[0x4000];

static int  g_listenSock  = -1;
static int  g_ctrlSock    = -1;
static int  g_pasvSock    = -1;
static int  g_dataSock    = -1;

static char g_cwd[FTPD_PATH_MAX];
static char g_cmd[FTPD_CMD_MAX];
static u32  g_cmdLen;
static char g_renameFrom[FTPD_PATH_MAX];
static char g_scratch[FTPD_PATH_MAX];
static char g_line[FTPD_LIST_MAX];
static u8   g_xfer[FTPD_XFER_SIZE];

static u32  g_clients;
static u32  g_transfers;

// ---------------------------------------------------------------- utilities

static bool ftpdWait(int sock, int events)
{
    struct pollfd pfd;

    pfd.fd = sock;
    pfd.events = events;

    while (ftpdEnabled)
    {
        pfd.revents = 0;

        int pr = socPoll(&pfd, 1, 500);
        if (pr != 0)
            return pr > 0;
    }

    return false;
}

static void ftpdSendString(int sock, const char *s)
{
    u32 len = 0;
    while (s[len] != 0)
        ++len;

    u32 sent = 0;
    while (sent < len)
    {
        if (!ftpdWait(sock, POLLOUT))
            return;

        ssize_t n = soc_send(sock, s + sent, len - sent, 0);
        if (n <= 0)
            return;
        sent += (u32)n;
    }
}

static void ftpdReply(int sock, const char *fmt, ...)
{
    va_list args;
    char buf[512];

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    ftpdSendString(sock, buf);
}

static bool ftpdIsRoot(const char *path)
{
    return path[0] == '/' && path[1] == 0;
}

// FTP のパスを絶対パスへ正規化する。".." と "." を畳んで、
// 末尾スラッシュは（ルート以外）落とす。SD の外へは出さない。
static bool ftpdResolve(char *out, u32 outSize, const char *base, const char *arg)
{
    char tmp[FTPD_PATH_MAX * 2];
    u32 n = 0;

    if (arg == NULL || arg[0] == 0)
        arg = ".";

    if (arg[0] == '/')
    {
        tmp[n++] = '/';
    }
    else
    {
        for (u32 i = 0; base[i] != 0 && n + 1 < sizeof(tmp); ++i)
            tmp[n++] = base[i];
        if (n == 0 || tmp[n - 1] != '/')
            tmp[n++] = '/';
    }

    for (u32 i = 0; arg[i] != 0 && n + 1 < sizeof(tmp); ++i)
        tmp[n++] = arg[i];
    tmp[n] = 0;

    // 成分ごとに畳む
    u32 outLen = 0;
    out[outLen++] = '/';

    u32 i = 0;
    while (tmp[i] != 0)
    {
        while (tmp[i] == '/')
            ++i;
        if (tmp[i] == 0)
            break;

        u32 start = i;
        while (tmp[i] != 0 && tmp[i] != '/')
            ++i;
        u32 segLen = i - start;

        if (segLen == 1 && tmp[start] == '.')
            continue;

        if (segLen == 2 && tmp[start] == '.' && tmp[start + 1] == '.')
        {
            while (outLen > 1 && out[outLen - 1] != '/')
                --outLen;
            if (outLen > 1)
                --outLen;
            if (outLen == 0)
                outLen = 1;
            continue;
        }

        if (outLen > 1)
        {
            if (outLen + 1 >= outSize)
                return false;
            out[outLen++] = '/';
        }

        if (outLen + segLen >= outSize)
            return false;

        for (u32 k = 0; k < segLen; ++k)
            out[outLen++] = tmp[start + k];
    }

    out[outLen] = 0;
    return true;
}

static Result ftpdOpenArchive(FS_Archive *archive)
{
    return FSUSER_OpenArchive(archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
}

static bool ftpdStat(const char *path, bool *isDir, u64 *size)
{
    FS_Archive archive = 0;

    *isDir = false;
    *size = 0;

    if (ftpdIsRoot(path))
    {
        *isDir = true;
        return true;
    }

    if (R_FAILED(ftpdOpenArchive(&archive)))
        return false;

    Handle h = 0;
    bool found = false;

    if (R_SUCCEEDED(FSUSER_OpenDirectory(&h, archive, fsMakePath(PATH_ASCII, path))))
    {
        FSDIR_Close(h);
        *isDir = true;
        found = true;
    }
    else
    {
        IFile file;
        if (R_SUCCEEDED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                                   fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
        {
            IFile_GetSize(&file, size);
            IFile_Close(&file);
            found = true;
        }
    }

    FSUSER_CloseArchive(archive);
    return found;
}

// ---------------------------------------------------------------- data conn

static void ftpdCloseData(void)
{
    if (g_dataSock >= 0)
    {
        socClose(g_dataSock);
        g_dataSock = -1;
    }
    if (g_pasvSock >= 0)
    {
        socClose(g_pasvSock);
        g_pasvSock = -1;
    }
}

// minisoc には getsockname が無いため、PASV のポートは自前で決めて bind する。
static u16 g_pasvPort = FTPD_PORT + 1;

static bool ftpdOpenPasvFixed(void)
{
    struct sockaddr_in saddr;

    ftpdCloseData();

    for (u32 attempt = 0; attempt < 16; ++attempt)
    {
        g_pasvPort++;
        if (g_pasvPort < FTPD_PORT + 1 || g_pasvPort > FTPD_PORT + 32)
            g_pasvPort = FTPD_PORT + 1;

        g_pasvSock = socSocket(AF_INET, SOCK_STREAM, 0);
        if (g_pasvSock < 0)
            return false;

        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(g_pasvPort);
        saddr.sin_addr.s_addr = gethostid();

        if (socBind(g_pasvSock, (struct sockaddr *)&saddr, sizeof(saddr)) == 0 &&
            socListen(g_pasvSock, 1) == 0)
            return true;

        socClose(g_pasvSock);
        g_pasvSock = -1;
    }

    return false;
}

static bool ftpdAcceptData(void)
{
    struct pollfd pfd;

    if (g_pasvSock < 0)
        return false;

    pfd.fd = g_pasvSock;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if (socPoll(&pfd, 1, 10000) <= 0)
        return false;

    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);

    g_dataSock = socAccept(g_pasvSock, (struct sockaddr *)&caddr, &clen);

    socClose(g_pasvSock);
    g_pasvSock = -1;

    return g_dataSock >= 0;
}

// ---------------------------------------------------------------- commands

static void ftpdCmdList(int ctrl, const char *arg, bool namesOnly)
{
    FS_Archive archive = 0;
    Handle dir = 0;

    if (!ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg))
    {
        ftpdReply(ctrl, "550 Path too long.\r\n");
        return;
    }

    if (R_FAILED(ftpdOpenArchive(&archive)))
    {
        ftpdReply(ctrl, "550 Cannot open SD.\r\n");
        return;
    }

    if (R_FAILED(FSUSER_OpenDirectory(&dir, archive, fsMakePath(PATH_ASCII, g_scratch))))
    {
        FSUSER_CloseArchive(archive);
        ftpdReply(ctrl, "550 No such directory.\r\n");
        return;
    }

    ftpdReply(ctrl, "150 Here comes the directory listing.\r\n");

    if (!ftpdAcceptData())
    {
        FSDIR_Close(dir);
        FSUSER_CloseArchive(archive);
        ftpdReply(ctrl, "425 Cannot open data connection.\r\n");
        return;
    }

    for (;;)
    {
        FS_DirectoryEntry entry;
        u32 n = 0;

        if (R_FAILED(FSDIR_Read(dir, &n, 1, &entry)) || n == 0)
            break;

        char name[256];
        int units = utf16_to_utf8((u8 *)name, entry.name, sizeof(name) - 1);
        if (units < 0 || (u32)units >= sizeof(name))
            continue;
        name[units] = 0;

        bool isDir = (entry.attributes & 1) != 0;

        if (namesOnly)
            sprintf(g_line, "%s\r\n", name);
        else
            sprintf(g_line, "%crw-rw-rw- 1 3ds 3ds %llu Jan 1 2000 %s\r\n",
                    isDir ? 'd' : '-', isDir ? 0ULL : entry.fileSize, name);

        ftpdSendString(g_dataSock, g_line);
    }

    FSDIR_Close(dir);
    FSUSER_CloseArchive(archive);

    ftpdCloseData();
    ftpdReply(ctrl, "226 Directory send OK.\r\n");
}

static void ftpdCmdRetr(int ctrl, const char *arg)
{
    IFile file;
    u64 size = 0;

    if (!ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg))
    {
        ftpdReply(ctrl, "550 Path too long.\r\n");
        return;
    }

    if (R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, g_scratch), FS_OPEN_READ)))
    {
        ftpdReply(ctrl, "550 Failed to open file.\r\n");
        return;
    }

    IFile_GetSize(&file, &size);
    file.pos = 0;

    ftpdReply(ctrl, "150 Opening data connection.\r\n");

    if (!ftpdAcceptData())
    {
        IFile_Close(&file);
        ftpdReply(ctrl, "425 Cannot open data connection.\r\n");
        return;
    }

    bool ok = true;

    while (file.pos < size)
    {
        u64 read = 0;

        if (R_FAILED(IFile_Read(&file, &read, g_xfer, sizeof(g_xfer))) || read == 0)
            break;

        u32 sent = 0;
        while (sent < (u32)read)
        {
            ssize_t w = -1;
            if (ftpdWait(g_dataSock, POLLOUT))
                w = soc_send(g_dataSock, g_xfer + sent, (u32)read - sent, 0);
            if (w <= 0)
            {
                ok = false;
                break;
            }
            sent += (u32)w;
        }

        if (!ok)
            break;
    }

    IFile_Close(&file);
    ftpdCloseData();

    ++g_transfers;
    ftpdReply(ctrl, ok ? "226 Transfer complete.\r\n" : "426 Transfer aborted.\r\n");
}

static void ftpdCmdStor(int ctrl, const char *arg, bool append)
{
    IFile file;
    FS_Archive archive = 0;

    if (!ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg))
    {
        ftpdReply(ctrl, "550 Path too long.\r\n");
        return;
    }

    if (!append && R_SUCCEEDED(ftpdOpenArchive(&archive)))
    {
        FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, g_scratch));
        FSUSER_CloseArchive(archive);
    }

    if (R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, g_scratch), FS_OPEN_CREATE | FS_OPEN_WRITE)))
    {
        ftpdReply(ctrl, "550 Cannot create file.\r\n");
        return;
    }

    if (append)
    {
        u64 size = 0;
        IFile_GetSize(&file, &size);
        file.pos = size;
    }
    else
    {
        file.pos = 0;
    }

    ftpdReply(ctrl, "150 Ready to receive data.\r\n");

    if (!ftpdAcceptData())
    {
        IFile_Close(&file);
        ftpdReply(ctrl, "425 Cannot open data connection.\r\n");
        return;
    }

    bool ok = true;

    for (;;)
    {
        ssize_t n = -1;
        if (ftpdWait(g_dataSock, POLLIN))
            n = soc_recv(g_dataSock, g_xfer, sizeof(g_xfer), 0);
        if (n == 0)
            break;
        if (n < 0)
        {
            ok = false;
            break;
        }

        u64 written = 0;
        if (R_FAILED(IFile_Write(&file, &written, g_xfer, (u32)n, FS_WRITE_FLUSH)))
        {
            ok = false;
            break;
        }
    }

    IFile_Close(&file);
    ftpdCloseData();

    ++g_transfers;
    ftpdReply(ctrl, ok ? "226 Transfer complete.\r\n" : "426 Transfer aborted.\r\n");
}

static void ftpdCmdDele(int ctrl, const char *arg)
{
    FS_Archive archive = 0;

    if (!ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg) ||
        R_FAILED(ftpdOpenArchive(&archive)))
    {
        ftpdReply(ctrl, "550 Failed.\r\n");
        return;
    }

    Result res = FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, g_scratch));
    FSUSER_CloseArchive(archive);

    ftpdReply(ctrl, R_SUCCEEDED(res) ? "250 File deleted.\r\n" : "550 Delete failed.\r\n");
}

static void ftpdCmdMkd(int ctrl, const char *arg)
{
    FS_Archive archive = 0;

    if (!ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg) ||
        R_FAILED(ftpdOpenArchive(&archive)))
    {
        ftpdReply(ctrl, "550 Failed.\r\n");
        return;
    }

    Result res = FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, g_scratch), 0);
    FSUSER_CloseArchive(archive);

    if (R_SUCCEEDED(res))
        ftpdReply(ctrl, "257 \"%s\" created.\r\n", g_scratch);
    else
        ftpdReply(ctrl, "550 Create failed.\r\n");
}

static void ftpdCmdRmd(int ctrl, const char *arg)
{
    FS_Archive archive = 0;

    if (!ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg) ||
        R_FAILED(ftpdOpenArchive(&archive)))
    {
        ftpdReply(ctrl, "550 Failed.\r\n");
        return;
    }

    Result res = FSUSER_DeleteDirectory(archive, fsMakePath(PATH_ASCII, g_scratch));
    FSUSER_CloseArchive(archive);

    ftpdReply(ctrl, R_SUCCEEDED(res) ? "250 Directory removed.\r\n"
                                     : "550 Remove failed (not empty?).\r\n");
}

static void ftpdCmdRnto(int ctrl, const char *arg)
{
    FS_Archive archive = 0;
    bool isDir = false;
    u64 size = 0;

    if (g_renameFrom[0] == 0)
    {
        ftpdReply(ctrl, "503 RNFR required first.\r\n");
        return;
    }

    if (!ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg) ||
        R_FAILED(ftpdOpenArchive(&archive)))
    {
        ftpdReply(ctrl, "550 Failed.\r\n");
        return;
    }

    ftpdStat(g_renameFrom, &isDir, &size);

    Result res;
    if (isDir)
        res = FSUSER_RenameDirectory(archive, fsMakePath(PATH_ASCII, g_renameFrom),
                                     archive, fsMakePath(PATH_ASCII, g_scratch));
    else
        res = FSUSER_RenameFile(archive, fsMakePath(PATH_ASCII, g_renameFrom),
                                archive, fsMakePath(PATH_ASCII, g_scratch));

    FSUSER_CloseArchive(archive);
    g_renameFrom[0] = 0;

    ftpdReply(ctrl, R_SUCCEEDED(res) ? "250 Rename successful.\r\n" : "550 Rename failed.\r\n");
}

// ---------------------------------------------------------------- dispatch

static bool ftpdMatch(const char *cmd, const char *name)
{
    u32 i = 0;

    while (name[i] != 0)
    {
        char c = cmd[i];
        if (c >= 'a' && c <= 'z')
            c = (char)(c - 'a' + 'A');
        if (c != name[i])
            return false;
        ++i;
    }

    return cmd[i] == 0 || cmd[i] == ' ';
}

static const char *ftpdArg(const char *cmd)
{
    const char *p = cmd;

    while (*p != 0 && *p != ' ')
        ++p;
    while (*p == ' ')
        ++p;

    return p;
}

// true を返すと接続を閉じる
static bool ftpdHandle(int ctrl, const char *cmd)
{
    const char *arg = ftpdArg(cmd);

    if (ftpdMatch(cmd, "USER"))          ftpdReply(ctrl, "230 Logged in.\r\n");
    else if (ftpdMatch(cmd, "PASS"))     ftpdReply(ctrl, "230 Logged in.\r\n");
    else if (ftpdMatch(cmd, "SYST"))     ftpdReply(ctrl, "215 UNIX Type: L8\r\n");
    else if (ftpdMatch(cmd, "NOOP"))     ftpdReply(ctrl, "200 OK.\r\n");
    else if (ftpdMatch(cmd, "TYPE"))     ftpdReply(ctrl, "200 Type set.\r\n");
    else if (ftpdMatch(cmd, "MODE"))     ftpdReply(ctrl, "200 Mode set.\r\n");
    else if (ftpdMatch(cmd, "STRU"))     ftpdReply(ctrl, "200 Structure set.\r\n");
    else if (ftpdMatch(cmd, "OPTS"))     ftpdReply(ctrl, "200 OK.\r\n");
    else if (ftpdMatch(cmd, "FEAT"))     ftpdReply(ctrl, "211-Features:\r\n SIZE\r\n211 End\r\n");
    else if (ftpdMatch(cmd, "PORT"))     ftpdReply(ctrl, "502 Use passive mode.\r\n");
    else if (ftpdMatch(cmd, "EPRT"))     ftpdReply(ctrl, "502 Use passive mode.\r\n");
    else if (ftpdMatch(cmd, "EPSV"))     ftpdReply(ctrl, "502 Use PASV.\r\n");
    else if (ftpdMatch(cmd, "QUIT"))
    {
        ftpdReply(ctrl, "221 Goodbye.\r\n");
        return true;
    }
    else if (ftpdMatch(cmd, "PWD") || ftpdMatch(cmd, "XPWD"))
    {
        ftpdReply(ctrl, "257 \"%s\" is the current directory.\r\n", g_cwd);
    }
    else if (ftpdMatch(cmd, "CWD") || ftpdMatch(cmd, "XCWD"))
    {
        bool isDir = false;
        u64 size = 0;

        if (ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg) &&
            ftpdStat(g_scratch, &isDir, &size) && isDir)
        {
            strncpy(g_cwd, g_scratch, sizeof(g_cwd) - 1);
            g_cwd[sizeof(g_cwd) - 1] = 0;
            ftpdReply(ctrl, "250 Directory changed.\r\n");
        }
        else
        {
            ftpdReply(ctrl, "550 No such directory.\r\n");
        }
    }
    else if (ftpdMatch(cmd, "CDUP") || ftpdMatch(cmd, "XCUP"))
    {
        if (ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, ".."))
        {
            strncpy(g_cwd, g_scratch, sizeof(g_cwd) - 1);
            g_cwd[sizeof(g_cwd) - 1] = 0;
        }
        ftpdReply(ctrl, "250 Directory changed.\r\n");
    }
    else if (ftpdMatch(cmd, "PASV"))
    {
        if (!ftpdOpenPasvFixed())
        {
            ftpdReply(ctrl, "425 Cannot open passive socket.\r\n");
        }
        else
        {
            u32 ip = gethostid();
            u32 p = g_pasvPort;

            ftpdReply(ctrl, "227 Entering Passive Mode (%lu,%lu,%lu,%lu,%lu,%lu).\r\n",
                      ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF,
                      p >> 8, p & 0xFF);
        }
    }
    else if (ftpdMatch(cmd, "LIST"))     ftpdCmdList(ctrl, arg, false);
    else if (ftpdMatch(cmd, "NLST"))     ftpdCmdList(ctrl, arg, true);
    else if (ftpdMatch(cmd, "RETR"))     ftpdCmdRetr(ctrl, arg);
    else if (ftpdMatch(cmd, "STOR"))     ftpdCmdStor(ctrl, arg, false);
    else if (ftpdMatch(cmd, "APPE"))     ftpdCmdStor(ctrl, arg, true);
    else if (ftpdMatch(cmd, "DELE"))     ftpdCmdDele(ctrl, arg);
    else if (ftpdMatch(cmd, "MKD") || ftpdMatch(cmd, "XMKD"))  ftpdCmdMkd(ctrl, arg);
    else if (ftpdMatch(cmd, "RMD") || ftpdMatch(cmd, "XRMD"))  ftpdCmdRmd(ctrl, arg);
    else if (ftpdMatch(cmd, "RNFR"))
    {
        bool isDir = false;
        u64 size = 0;

        if (ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg) &&
            ftpdStat(g_scratch, &isDir, &size))
        {
            strncpy(g_renameFrom, g_scratch, sizeof(g_renameFrom) - 1);
            g_renameFrom[sizeof(g_renameFrom) - 1] = 0;
            ftpdReply(ctrl, "350 Ready for RNTO.\r\n");
        }
        else
        {
            ftpdReply(ctrl, "550 No such file.\r\n");
        }
    }
    else if (ftpdMatch(cmd, "RNTO"))     ftpdCmdRnto(ctrl, arg);
    else if (ftpdMatch(cmd, "SIZE"))
    {
        bool isDir = false;
        u64 size = 0;

        if (ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg) &&
            ftpdStat(g_scratch, &isDir, &size) && !isDir)
            ftpdReply(ctrl, "213 %llu\r\n", size);
        else
            ftpdReply(ctrl, "550 Not a regular file.\r\n");
    }
    else
    {
        ftpdReply(ctrl, "502 Command not implemented.\r\n");
    }

    return false;
}

// ---------------------------------------------------------------- session

static void ftpdSession(int ctrl)
{
    g_cwd[0] = '/';
    g_cwd[1] = 0;
    g_cmdLen = 0;
    g_renameFrom[0] = 0;

    ftpdReply(ctrl, "220 Luma3DS ftpd ready.\r\n");

    while (ftpdEnabled)
    {
        struct pollfd pfd;
        pfd.fd = ctrl;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pr = socPoll(&pfd, 1, 500);
        if (pr < 0)
            break;
        if (pr == 0)
            continue;

        char chunk[256];
        ssize_t n = soc_recv(ctrl, chunk, sizeof(chunk), 0);
        if (n <= 0)
            break;

        bool quit = false;

        for (ssize_t i = 0; i < n; ++i)
        {
            char c = chunk[i];

            if (c == '\r')
                continue;

            if (c == '\n')
            {
                g_cmd[g_cmdLen] = 0;
                g_cmdLen = 0;

                if (g_cmd[0] != 0 && ftpdHandle(ctrl, g_cmd))
                {
                    quit = true;
                    break;
                }
                continue;
            }

            if (g_cmdLen + 1 < FTPD_CMD_MAX)
                g_cmd[g_cmdLen++] = c;
        }

        if (quit)
            break;
    }

    ftpdCloseData();
}

MyThread *ftpdCreateThread(void)
{
    if (R_FAILED(MyThread_Create(&ftpdThread, ftpdThreadMain, ftpdThreadStack,
                                 sizeof(ftpdThreadStack), 0x20, CORE_SYSTEM)))
        svcBreak(USERBREAK_PANIC);

    return &ftpdThread;
}

void ftpdThreadMain(void)
{
    if (R_FAILED(miniSocInit()))
    {
        ftpdEnabled = false;
        return;
    }

    g_listenSock = socSocket(AF_INET, SOCK_STREAM, 0);

    u32 tries = 15;
    while (g_listenSock < 0 && --tries > 0)
    {
        svcSleepThread(100 * 1000 * 1000LL);
        g_listenSock = socSocket(AF_INET, SOCK_STREAM, 0);
    }

    if (g_listenSock < 0 || tries == 0)
    {
        miniSocExit();
        ftpdEnabled = false;
        return;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(FTPD_PORT);
    saddr.sin_addr.s_addr = gethostid();

    if (socBind(g_listenSock, (struct sockaddr *)&saddr, sizeof(saddr)) != 0 ||
        socListen(g_listenSock, 2) != 0)
    {
        socClose(g_listenSock);
        g_listenSock = -1;
        miniSocExit();
        ftpdEnabled = false;
        return;
    }

    while (ftpdEnabled)
    {
        struct pollfd pfd;
        pfd.fd = g_listenSock;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pr = socPoll(&pfd, 1, 500);
        if (pr <= 0)
            continue;

        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);

        g_ctrlSock = socAccept(g_listenSock, (struct sockaddr *)&caddr, &clen);
        if (g_ctrlSock < 0)
            continue;

        ++g_clients;
        ftpdSession(g_ctrlSock);

        socClose(g_ctrlSock);
        g_ctrlSock = -1;
    }

    ftpdCloseData();

    if (g_listenSock >= 0)
    {
        socClose(g_listenSock);
        g_listenSock = -1;
    }

    miniSocExit();
}

Result Ftpd_Disable(s64 timeout)
{
    if (!ftpdEnabled)
        return 0;

    ftpdEnabled = false;

    return MyThread_Join(&ftpdThread, timeout);
}

// ---------------------------------------------------------------- menu

void RosalinaMenu_Ftpd(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 4, COLOR_TITLE, "FTP server");

        u32 y = 4 + 3 * SPACING_Y;

        if (ftpdEnabled)
        {
            u32 ip = gethostid();

            y = Draw_DrawString(10, y, COLOR_LIME, "Running.\n\n");
            y = Draw_DrawFormattedString(10, y, COLOR_WHITE, "ftp://%lu.%lu.%lu.%lu:%lu\n\n",
                                         ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF,
                                         (ip >> 24) & 0xFF, (u32)FTPD_PORT);
            y = Draw_DrawString(10, y, COLOR_WHITE, Lang(STR_ANY_USERNAME_AND_PASSWORD));
            y = Draw_DrawString(10, y, COLOR_WHITE, Lang(STR_PASSIVE_MODE_ONLY));
            y = Draw_DrawFormattedString(10, y, COLOR_WHITE, "Sessions:  %lu\n", g_clients);
            y = Draw_DrawFormattedString(10, y, COLOR_WHITE, "Transfers: %lu\n\n", g_transfers);
            y = Draw_DrawString(10, y, COLOR_WHITE, Lang(STR_KEEPS_RUNNING_WITH_THE_MENU_CLOSED));

            Draw_DrawString(10, 225, COLOR_WHITE, Lang(STR_A_STOP_B_BACK));
        }
        else
        {
            y = Draw_DrawString(10, y, COLOR_WHITE, "Stopped.\n\n");
            y = Draw_DrawString(10, y, COLOR_WHITE, Lang(STR_SERVES_THE_SD_CARD_OVER_FTP_SO_YOU));
            y = Draw_DrawString(10, y, COLOR_WHITE, Lang(STR_CAN_MANAGE_FILES_FROM_A_PC));
            y = Draw_DrawString(10, y, COLOR_WHITE, Lang(STR_WI_FI_MUST_BE_CONNECTED_FIRST));

            Draw_DrawString(10, 225, COLOR_WHITE, Lang(STR_A_START_B_BACK));
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 k = waitInputWithTimeout(1000);

        if (terminationRequest || (k & BUTTON_B))
            return;

        if (k & BUTTON_A)
        {
            if (ftpdEnabled)
            {
                Ftpd_Disable(5 * 1000 * 1000 * 1000LL);
            }
            else if (ftpdThread.handle == 0 || R_SUCCEEDED(MyThread_Join(&ftpdThread, 0)))
            {
                ftpdEnabled = true;
                ftpdCreateThread();
                svcSleepThread(500 * 1000 * 1000LL);
            }
        }
    }
    while (!terminationRequest);
}
