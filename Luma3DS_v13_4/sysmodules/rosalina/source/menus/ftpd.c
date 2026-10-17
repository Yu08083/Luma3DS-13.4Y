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
#include <stdint.h>
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

volatile bool ftpdEnabled = false;

static MyThread ftpdThread;
static u8 CTR_ALIGN(8) ftpdThreadStack[0x4000];

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

enum { FTPD_STOPPED, FTPD_STARTING, FTPD_LISTENING, FTPD_CONNECTED, FTPD_LISTING, FTPD_SENDING, FTPD_RECEIVING, FTPD_STOPPING, FTPD_FAILED };
static volatile u32 g_status;
static volatile u32 g_hostId;
static volatile u32 g_clients;
static volatile u32 g_transfers;
static volatile u32 g_activeClients;
static volatile u32 g_bytes;
static volatile u32 g_total;
static volatile Result g_error;



static int ftpdOpenListener(u16 port, int backlog)
{
    int sock = socSocket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) { g_error = sock; return -1; }
    int reuse = 1;
    socSetsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;
    int result = socBind(sock, (struct sockaddr *)&address, sizeof(address));
    if(result == 0) result = socListen(sock, backlog);
    if(result != 0)
    {
        g_error = result;
        socClose(sock);
        return -1;
    }
    return sock;
}

static bool ftpdWait(int sock, int events)
{
    struct pollfd pfd;

    pfd.fd = sock;
    pfd.events = events;

    while (ftpdEnabled)
    {
        pfd.revents = 0;

        int pr = socPoll(&pfd, 1, 100);
        if(pr == -EINTR || pr == -EAGAIN) continue;
        if(pr < 0 || (pfd.revents & (POLLERR | POLLNVAL))) return false;
        if(pr > 0 && (pfd.revents & events)) return true;
        if(pr > 0 && (pfd.revents & POLLHUP)) return (events & POLLIN) != 0;
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

        ssize_t n = socSend(sock, s + sent, len - sent, 0);
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


static u16 g_pasvPort = FTPD_PORT + 1;

static bool ftpdOpenPasvFixed(void)
{
    ftpdCloseData();
    g_hostId = miniSocRefreshHostId();
    if(g_hostId == 0 || g_hostId == UINT32_MAX) return false;
    for(u32 attempt = 0; attempt < 32; ++attempt)
    {
        g_pasvPort++;
        if(g_pasvPort < FTPD_PORT + 1 || g_pasvPort > FTPD_PORT + 32)
            g_pasvPort = FTPD_PORT + 1;
        g_pasvSock = ftpdOpenListener(g_pasvPort, 1);
        if(g_pasvSock >= 0) return true;
    }
    return false;
}

static bool ftpdAcceptData(void)
{
    if(g_pasvSock < 0) return false;
    u64 started = svcGetSystemTick();
    while(ftpdEnabled)
    {
        if((u64)(svcGetSystemTick() - started) >= 10ULL * SYSCLOCK_ARM11) break;
        struct pollfd pfd = { .fd = g_pasvSock, .events = POLLIN, .revents = 0 };
        int ready = socPoll(&pfd, 1, 100);
        if(ready == -EINTR || ready == -EAGAIN) continue;
        if(ready < 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))) break;
        if(!ftpdEnabled) break;
        if(ready > 0 && (pfd.revents & POLLIN))
        {
            struct sockaddr_in client;
            socklen_t length = sizeof(client);
            g_dataSock = socAccept(g_pasvSock, (struct sockaddr *)&client, &length);
            if(g_dataSock < 0) break;
            socClose(g_pasvSock);
            g_pasvSock = -1;
            return true;
        }
    }
    ftpdCloseData();
    return false;
}



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

    g_bytes = g_total = 0;
    g_status = FTPD_LISTING;
    ftpdReply(ctrl, "150 Here comes the directory listing.\r\n");

    if (!ftpdAcceptData())
    {
        FSDIR_Close(dir);
        FSUSER_CloseArchive(archive);
        ftpdReply(ctrl, "425 Cannot open data connection.\r\n");
        if(ftpdEnabled) g_status = FTPD_CONNECTED;
        return;
    }

    while(ftpdEnabled)
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
    if(ftpdEnabled) g_status = FTPD_CONNECTED;
    ftpdReply(ctrl, "226 Directory send OK.\r\n");
}

static void ftpdCmdRetr(int ctrl, const char *arg)
{
    IFile file;
    u64 size = 0;
    if(!ftpdResolve(g_scratch, sizeof(g_scratch), g_cwd, arg)) { ftpdReply(ctrl, "550 Path too long.\r\n"); return; }
    if(R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, g_scratch), FS_OPEN_READ)))
    { ftpdReply(ctrl, "550 Failed to open file.\r\n"); return; }
    if(R_FAILED(IFile_GetSize(&file, &size)))
    { IFile_Close(&file); ftpdReply(ctrl, "550 Failed to read file size.\r\n"); return; }
    file.pos = 0;
    g_bytes = 0;
    g_total = size > UINT32_MAX ? UINT32_MAX : (u32)size;
    g_status = FTPD_SENDING;
    ftpdReply(ctrl, "150 Opening data connection.\r\n");
    if(!ftpdAcceptData())
    { IFile_Close(&file); ftpdReply(ctrl, "425 Cannot open data connection.\r\n"); if(ftpdEnabled) g_status = FTPD_CONNECTED; return; }
    bool ok = true;
    while(ftpdEnabled && file.pos < size)
    {
        u64 read = 0;
        if(R_FAILED(IFile_Read(&file, &read, g_xfer, sizeof(g_xfer))) || read == 0) { ok = false; break; }
        u32 sent = 0;
        while(sent < read)
        {
            ssize_t written = -1;
            if(ftpdWait(g_dataSock, POLLOUT)) written = socSend(g_dataSock, g_xfer + sent, (u32)read - sent, 0);
            if(written <= 0) { ok = false; break; }
            sent += (u32)written;
            g_bytes = UINT32_MAX - g_bytes < (u32)written ? UINT32_MAX : g_bytes + (u32)written;
        }
        if(!ok) break;
    }
    ok = ok && ftpdEnabled && file.pos == size;
    IFile_Close(&file);
    ftpdCloseData();
    if(ok) ++g_transfers;
    if(ftpdEnabled) g_status = FTPD_CONNECTED;
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

    g_bytes = g_total = 0;
    g_status = FTPD_RECEIVING;
    ftpdReply(ctrl, "150 Ready to receive data.\r\n");

    if (!ftpdAcceptData())
    {
        IFile_Close(&file);
        ftpdReply(ctrl, "425 Cannot open data connection.\r\n");
        if(ftpdEnabled) g_status = FTPD_CONNECTED;
        return;
    }

    bool ok = true;

    while(ftpdEnabled)
    {
        ssize_t n = -1;
        if (ftpdWait(g_dataSock, POLLIN))
            n = socRecv(g_dataSock, g_xfer, sizeof(g_xfer), 0);
        if (n == 0)
            break;
        if (n < 0)
        {
            ok = false;
            break;
        }

        u64 written = 0;
        if (R_FAILED(IFile_Write(&file, &written, g_xfer, (u32)n, 0)) || written != (u32)n)
        {
            ok = false;
            break;
        }
        g_bytes = UINT32_MAX - g_bytes < (u32)n ? UINT32_MAX : g_bytes + (u32)n;
    }

    ok = ok && ftpdEnabled;
    if(R_FAILED(FSFILE_Flush(file.handle))) ok = false;
    IFile_Close(&file);
    ftpdCloseData();

    if(ok) ++g_transfers;
    if(ftpdEnabled) g_status = FTPD_CONNECTED;
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
            u32 ip = g_hostId;
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

        int pr = socPoll(&pfd, 1, 100);
        if(pr == -EINTR || pr == -EAGAIN) continue;
        if(pr < 0 || (pfd.revents & (POLLERR | POLLNVAL))) break;
        if(pr == 0) continue;
        if(!(pfd.revents & POLLIN))
        {
            if(pfd.revents & POLLHUP) break;
            continue;
        }

        char chunk[256];
        ssize_t n = socRecv(ctrl, chunk, sizeof(chunk), 0);
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
    Result result = MyThread_Create(&ftpdThread, ftpdThreadMain, ftpdThreadStack, sizeof(ftpdThreadStack), 56, CORE_SYSTEM);
    if(R_FAILED(result))
    {
        g_error = result;
        ftpdEnabled = false;
        g_status = FTPD_FAILED;
        return NULL;
    }
    return &ftpdThread;
}

void ftpdThreadMain(void)
{
    Result result = miniSocInit();
    if(R_FAILED(result))
    {
        g_error = result;
        ftpdEnabled = false;
        g_status = FTPD_FAILED;
        return;
    }
    g_activeClients = 0;
    g_bytes = g_total = 0;
    g_listenSock = -1;
    for(u32 tries = 0; ftpdEnabled && tries < 100; ++tries)
    {
        g_hostId = miniSocRefreshHostId();
        if(g_hostId != 0 && g_hostId != UINT32_MAX)
        {
            g_listenSock = ftpdOpenListener(FTPD_PORT, 2);
            if(g_listenSock >= 0) break;
        }
        svcSleepThread(100 * 1000 * 1000LL);
    }
    if(!ftpdEnabled) goto cleanup;
    if(g_listenSock < 0)
    {
        if(g_error == 0) g_error = -1;
        g_status = FTPD_FAILED;
        goto cleanup;
    }
    g_error = 0;
    g_status = FTPD_LISTENING;
    u64 hostRefreshed = svcGetSystemTick();
    while(ftpdEnabled)
    {
        struct pollfd pfd = { .fd = g_listenSock, .events = POLLIN, .revents = 0 };
        int ready = socPoll(&pfd, 1, 100);
        if((u64)(svcGetSystemTick() - hostRefreshed) >= SYSCLOCK_ARM11)
        {
            g_hostId = miniSocRefreshHostId();
            hostRefreshed = svcGetSystemTick();
        }
        if(ready == -EINTR || ready == -EAGAIN) continue;
        if(ready < 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)))
        {
            g_error = ready < 0 ? ready : -3;
            g_status = FTPD_FAILED;
            break;
        }
        if(!ftpdEnabled) break;
        if(ready == 0 || !(pfd.revents & POLLIN)) continue;
        struct sockaddr_in client;
        socklen_t length = sizeof(client);
        g_ctrlSock = socAccept(g_listenSock, (struct sockaddr *)&client, &length);
        if(g_ctrlSock < 0) continue;
        ++g_clients;
        g_activeClients = 1;
        g_status = FTPD_CONNECTED;
        ftpdSession(g_ctrlSock);
        socClose(g_ctrlSock);
        g_ctrlSock = -1;
        g_activeClients = 0;
        if(ftpdEnabled) g_status = FTPD_LISTENING;
    }
cleanup:
    ftpdCloseData();
    if(g_ctrlSock >= 0) { socClose(g_ctrlSock); g_ctrlSock = -1; }
    if(g_listenSock >= 0) { socClose(g_listenSock); g_listenSock = -1; }
    miniSocExit();
    g_activeClients = 0;
    ftpdEnabled = false;
    if(g_status != FTPD_FAILED) g_status = FTPD_STOPPED;
}

Result Ftpd_Disable(s64 timeout)
{
    ftpdEnabled = false;
    if(ftpdThread.handle == 0) return 0;
    if(g_status != FTPD_STOPPED && g_status != FTPD_FAILED) g_status = FTPD_STOPPING;
    return MyThread_Join(&ftpdThread, timeout);
}

typedef struct {
    u32 status, ip, clients, transfers, bytes, total;
    Result error;
} FtpdView;

static const char *ftpdStatusText(u32 status)
{
    static const char *const labels[] = {
        "Stopped.", "Starting...", "Waiting for connection", "Connected", "Sending directory list",
        "Sending file", "Receiving file", "Stopping...", "Unable to start FTP."
    };
    return Lang_Translate(labels[status < sizeof(labels) / sizeof(labels[0]) ? status : FTPD_FAILED]);
}

static void ftpdDrawView(const FtpdView *view, const FtpdView *previous, bool first)
{
    bool active = view->status != FTPD_STOPPED && view->status != FTPD_FAILED;
    bool oldActive = previous->status != FTPD_STOPPED && previous->status != FTPD_FAILED;
    bool full = first || active != oldActive;
    bool changed = full || view->status != previous->status || view->ip != previous->ip || view->clients != previous->clients || view->transfers != previous->transfers || view->bytes / 1024 != previous->bytes / 1024 || view->total != previous->total || view->error != previous->error;
    if(!changed) return;
    Draw_Lock();
    if(full)
    {
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 4, g_accentColor, Lang_Translate("FTP server"));
        if(active)
        {
            Draw_DrawString(10, 175, g_textColor, Lang(STR_ANY_USERNAME_AND_PASSWORD));
            Draw_DrawString(10, 188, g_textColor, Lang(STR_PASSIVE_MODE_ONLY));
            Draw_DrawString(10, 210, g_textColor, Lang(STR_KEEPS_RUNNING_WITH_THE_MENU_CLOSED));
        }
        else
        {
            u32 y = Draw_DrawString(10, 70, g_textColor, Lang(STR_SERVES_THE_SD_CARD_OVER_FTP_SO_YOU));
            y = Draw_DrawString(10, y + SPACING_Y, g_textColor, Lang(STR_CAN_MANAGE_FILES_FROM_A_PC));
            Draw_DrawString(10, y + SPACING_Y, g_textColor, Lang(STR_WI_FI_MUST_BE_CONNECTED_FIRST));
        }
    }
    if(full || view->status != previous->status)
    {
        Draw_FillRect(10, 37, 300, SPACING_Y, g_menuBgColor);
        Draw_DrawString(10, 37, view->status == FTPD_FAILED ? COLOR_RED : g_textColor, ftpdStatusText(view->status));
        Draw_FillRect(10, 225, 300, SPACING_Y, g_menuBgColor);
        Draw_DrawString(10, 225, g_textColor, Lang(active ? STR_A_STOP_B_BACK : STR_A_START_B_BACK));
    }
    if(active)
    {
        if(full || view->ip != previous->ip)
        {
            Draw_FillRect(10, 59, 300, SPACING_Y, g_menuBgColor);
            if(view->ip && view->ip != UINT32_MAX)
                Draw_DrawFormattedString(10, 59, g_textColor, "ftp://%lu.%lu.%lu.%lu:%lu", view->ip & 255u, (view->ip >> 8) & 255u, (view->ip >> 16) & 255u, (view->ip >> 24) & 255u, (u32)FTPD_PORT);
        }
        if(full || view->clients != previous->clients)
        {
            Draw_FillRect(10, 81, 300, SPACING_Y, g_menuBgColor);
            Draw_DrawFormattedString(10, 81, g_textColor, Lang_Translate("Connections: %lu"), view->clients);
        }
        if(full || view->transfers != previous->transfers)
        {
            Draw_FillRect(10, 103, 300, SPACING_Y, g_menuBgColor);
            Draw_DrawFormattedString(10, 103, g_textColor, Lang_Translate("Completed transfers: %lu"), view->transfers);
        }
        if(full || view->bytes / 1024 != previous->bytes / 1024)
        {
            Draw_FillRect(10, 125, 300, SPACING_Y, g_menuBgColor);
            Draw_DrawFormattedString(10, 125, g_textColor, Lang_Translate("Transferred: %lu KiB"), view->bytes / 1024);
        }
        if(full || view->total != previous->total)
        {
            Draw_FillRect(10, 147, 300, SPACING_Y, g_menuBgColor);
            if(view->total)
                Draw_DrawFormattedString(10, 147, g_textColor, Lang_Translate("File size: %lu KiB"), (view->total / 1024) + (view->total % 1024 != 0));
        }
    }
    else if(view->status == FTPD_FAILED)
    {
        Draw_FillRect(10, 147, 300, SPACING_Y, g_menuBgColor);
        Draw_DrawFormattedString(10, 147, COLOR_RED, Lang_Translate("Error: 0x%08lx"), (u32)view->error);
    }
    Draw_FlushFramebuffer();
    Draw_Unlock();
}

void RosalinaMenu_Ftpd(void)
{
    FtpdView previous = {0};
    bool first = true;
    while(!menuShouldExit)
    {
        FtpdView view = { g_status, g_hostId, g_activeClients, g_transfers, g_bytes, g_total, g_error };
        ftpdDrawView(&view, &previous, first);
        previous = view;
        first = false;
        if(!ftpdEnabled && ftpdThread.handle != 0) MyThread_Join(&ftpdThread, 0);
        u32 pressed = waitInputWithTimeout(100);
        if(pressed & KEY_B) return;
        if(pressed & KEY_A)
        {
            if(ftpdEnabled) Ftpd_Disable(0);
            else if(ftpdThread.handle == 0)
            {
                g_error = 0;
                g_hostId = 0;
                g_status = FTPD_STARTING;
                ftpdEnabled = true;
                ftpdCreateThread();
            }
        }
    }
}
