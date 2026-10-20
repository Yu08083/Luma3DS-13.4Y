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

#include "fs.h"
#include "boot_rosalina.h"
#include "boot_language.h"
#include "memory.h"
#include "fmt.h"
#include "crypto.h"
#include "cache.h"
#include "screen.h"
#include "draw.h"
#include "utils.h"
#include "fatfs/ff.h"
#include "buttons.h"
#include "firm.h"
#include "crypto.h"
#include "strings.h"
#include "alignedseqmemcpy.h"
#include "i2c.h"

static FATFS sdFs,
             nandFs;

static bool switchToMainDir(bool isSd)
{
    const char *mainDir = isSd ? "/luma" : "/rw/luma";

    switch(f_chdir(mainDir))
    {
        case FR_OK:
            return true;
        case FR_NO_PATH:
        {
            if (f_mkdir(mainDir) != FR_OK)
            {
                error("Failed to create luma directory.");
                return false;
            }
            return switchToMainDir(isSd);
        }
        default:
            return false;
    }
}

bool mountSdCardPartition(bool switchMainDir)
{
    static bool sdInitialized = false;
    if (!sdInitialized)
        sdInitialized = f_mount(&sdFs, "sdmc:", 1) == FR_OK;

    if (sdInitialized && switchMainDir)
        return f_chdrive("sdmc:") == FR_OK && switchToMainDir(true);
    return sdInitialized;
}

bool remountCtrNandPartition(bool switchMainDir)
{
    static bool nandInitialized = false;
    int res = FR_OK;

    if (!nandInitialized)
    {
        res = f_mount(&nandFs, "nand:", 1);
        nandInitialized = res == FR_OK;
    }

    if (nandInitialized && switchMainDir)
        return f_chdrive("nand:") == FR_OK && switchToMainDir(false);
    return nandInitialized;
}

void unmountPartitions(void)
{
    f_unmount("nand:");
    f_unmount("sdmc:");
}

u32 fileRead(void *dest, const char *path, u32 maxSize)
{
    FIL file;
    FRESULT result = FR_OK;
    u32 ret = 0;

    if(f_open(&file, path, FA_READ) != FR_OK) return ret;

    u32 size = f_size(&file);
    if(dest == NULL) ret = size;
    else if(size <= maxSize)
        result = f_read(&file, dest, size, (unsigned int *)&ret);
    result |= f_close(&file);

    return result == FR_OK ? ret : 0;
}

u32 getFileSize(const char *path)
{
    return fileRead(NULL, path, 0);
}

bool fileWrite(const void *buffer, const char *path, u32 size)
{
    FIL file;
    FRESULT result = FR_OK;

    switch(f_open(&file, path, FA_WRITE | FA_OPEN_ALWAYS))
    {
        case FR_OK:
        {
            unsigned int written;
            result = f_write(&file, buffer, size, &written);
            if(result == FR_OK) result = f_truncate(&file);
            result |= f_close(&file);

            return result == FR_OK && (u32)written == size;
        }
        case FR_NO_PATH:

            for(u32 i = 1; path[i] != 0; i++)
                if(path[i] == '/')
                {
                    char folder[i + 1];
                    memcpy(folder, path, i);
                    folder[i] = 0;
                    result = f_mkdir(folder);
                }

            return result == FR_OK && fileWrite(buffer, path, size);
        default:
            return false;
    }
}

bool fileDelete(const char *path)
{
    return f_unlink(path) == FR_OK;
}

bool fileRename(const char *oldPath, const char *newPath)
{
    return f_rename(oldPath, newPath) == FR_OK;
}

bool fileCopy(const char *pathSrc, const char *pathDst, bool replace, void *tmpBuffer, size_t bufferSize)
{
    FIL fileSrc, fileDst;
    FRESULT res;

    res = f_open(&fileSrc, pathSrc, FA_READ);
    if (res != FR_OK)
        return true;

    size_t szSrc = f_size(&fileSrc), rem = szSrc;

    res = f_open(&fileDst, pathDst, FA_WRITE | (replace ? FA_CREATE_ALWAYS : FA_CREATE_NEW));

    if (res == FR_EXIST)
    {

        f_close(&fileSrc);
        return true;
    }
    else if (res == FR_NO_PATH)
    {

        const char *c;
        for (c = pathDst + strlen(pathDst); *c != '/' && c >= pathDst; --c);
        if (c >= pathDst && c - pathDst <= FF_MAX_LFN && *c != '\0')
        {
            char path[FF_MAX_LFN + 1];
            strncpy(path, pathDst, c - pathDst);
            path[c - pathDst] = '\0';
            res = f_mkdir(path);
        }

        if (res == FR_OK)
            res = f_open(&fileDst, pathDst, FA_WRITE | (replace ? FA_CREATE_ALWAYS : FA_CREATE_NEW));
    }

    if (res != FR_OK)
    {
        f_close(&fileSrc);
        return false;
    }

    while (rem > 0)
    {
        size_t sz = rem >= bufferSize ? bufferSize : rem;
        UINT n = 0;

        res = f_read(&fileSrc, tmpBuffer, sz, &n);
        if (n != sz)
            res = FR_INT_ERR;

        if (res == FR_OK)
        {
            res = f_write(&fileDst, tmpBuffer, sz, &n);
            if (n != sz)
                res = FR_DENIED;
        }

        if (res != FR_OK)
        {
            f_close(&fileSrc);
            f_close(&fileDst);
            f_unlink(pathDst);
            return false;
        }
        rem -= sz;
    }

    f_close(&fileSrc);
    f_close(&fileDst);

    return true;
}

bool createDir(const char *path)
{
    FRESULT res = f_mkdir(path);
    return res == FR_OK || res == FR_EXIST;
}

bool findPayload(char *path, u32 pressed)
{
    const char *pattern;

    if(pressed & BUTTON_LEFT) pattern = PATTERN("left");
    else if(pressed & BUTTON_RIGHT) pattern = PATTERN("right");
    else if(pressed & BUTTON_UP) pattern = PATTERN("up");
    else if(pressed & BUTTON_DOWN) pattern = PATTERN("down");
    else if(pressed & BUTTON_START) pattern = PATTERN("start");
    else if(pressed & BUTTON_B) pattern = PATTERN("b");
    else if(pressed & BUTTON_X) pattern = PATTERN("x");
    else if(pressed & BUTTON_Y) pattern = PATTERN("y");
    else if(pressed & BUTTON_R1) pattern = PATTERN("r");
    else if(pressed & BUTTON_A) pattern = PATTERN("a");
    else pattern = PATTERN("select");

    DIR dir;
    FILINFO info;
    FRESULT result;

    result = f_findfirst(&dir, &info, "payloads", pattern);

    if(result != FR_OK) return false;

    f_closedir(&dir);

    if(!info.fname[0]) return false;

    sprintf(path, "payloads/%s", info.fname);

    return true;
}

bool payloadMenu(char *path, bool *hasDisplayedMenu)
{
    DIR dir;
    FILINFO info;
    u32 payloadNum=0;
    char payloadList[20][49];
    if(f_opendir(&dir,"payloads")==FR_OK)
    {
        while(f_readdir(&dir,&info)==FR_OK && info.fname[0] && payloadNum<20)
        {
            if(info.fname[0]=='.')continue;
            u32 length=strlen(info.fname);
            if(length<6 || length>52 || memcmp(info.fname+length-5,".firm",5))continue;
            memcpy(payloadList[payloadNum],info.fname,length-5);
            payloadList[payloadNum++][length-5]=0;
        }
        if(f_closedir(&dir)!=FR_OK)payloadNum=0;
    }
    u32 selected=0, first=0;
    bool redraw=true;
    initScreens();
    *hasDisplayedMenu=true;
    while(true)
    {
        if(redraw)
        {
            clearScreens(false);
            drawString(true,10,10,COLOR_TITLE,"Luma3DS chainloader");
            drawString(true,10,10+SPACING_Y,COLOR_TITLE,BootLanguage_IsJapanese()?"A: 選択   START / B: 起動へ戻る":"A: Select    START / B: Continue boot");
            for(u32 row=0;row<15 && first+row<=payloadNum;row++)
            {
                u32 index=first+row;
                const char *label=index==payloadNum?(BootLanguage_IsJapanese()?"Rosalinaを開くボタン...":"Rosalina menu buttons..."):payloadList[index];
                drawString(true,10,10+(3+row)*SPACING_Y,index==selected?COLOR_RED:COLOR_WHITE,label);
            }
            redraw=false;
        }
        u32 pressed=waitInput(true);
        if(pressed&(BUTTON_START|BUTTON_B))break;
        if(pressed&BUTTON_A)
        {
            if(selected<payloadNum)
            {
                sprintf(path,"payloads/%s.firm",payloadList[selected]);
                return true;
            }
            BootRosalina_Configure();redraw=true;continue;
        }
        if(pressed&BUTTON_UP)selected=selected?selected-1:payloadNum;
        else if(pressed&BUTTON_DOWN)selected=selected<payloadNum?selected+1:0;
        else if(pressed&BUTTON_LEFT)selected=0;
        else if(pressed&BUTTON_RIGHT)selected=payloadNum;
        else continue;
        if(selected<first)first=selected;
        if(selected>=first+15)first=selected-14;
        redraw=true;
    }
    while(HID_PAD&PIN_BUTTONS);
    wait(2000ULL);
    return false;
}

u32 firmRead(void *dest, u32 firmType)
{
    static const char *firmFolders[][2] = {{"00000002", "20000002"},
                                           {"00000102", "20000102"},
                                           {"00000202", "20000202"},
                                           {"00000003", "20000003"},
                                           {"00000001", "20000001"}};

    char folderPath[64],
         path[128];

    sprintf(folderPath, "nand:/title/00040138/%s/content", firmFolders[firmType][ISN3DS ? 1 : 0]);

    DIR dir;
    u32 firmVersion = 0xFFFFFFFF;

    if(f_opendir(&dir, folderPath) != FR_OK) goto exit;

    FILINFO info;


    while(f_readdir(&dir, &info) == FR_OK && info.fname[0] != 0)
    {

        if(info.fname[9] != 'a' || strlen(info.fname) != 12) continue;

        u32 tempVersion = hexAtoi(info.altname, 8);


        if(tempVersion < firmVersion) firmVersion = tempVersion;
    }

    if(f_closedir(&dir) != FR_OK || firmVersion == 0xFFFFFFFF) goto exit;


    sprintf(path, "%s/%08lx.app", folderPath, firmVersion);

    if(fileRead(dest, path, 0x400000 + sizeof(Cxi) + 0x200) <= sizeof(Cxi) + 0x400) firmVersion = 0xFFFFFFFF;

exit:
    return firmVersion;
}

void findDumpFile(const char *folderPath, char *fileName)
{
    DIR dir;
    FRESULT result;

    for(u32 n = 0; n <= 99999999; n++)
    {
        FILINFO info;

        sprintf(fileName, "crash_dump_%08lu.dmp", n);
        result = f_findfirst(&dir, &info, folderPath, fileName);

        if(result != FR_OK || !info.fname[0]) break;
    }

    if(result == FR_OK) f_closedir(&dir);
}

static u8 fileCopyBuffer[0x10000];

static const u8 boot9Sha256[32] = {
    0x2F, 0x88, 0x74, 0x4F, 0xEE, 0xD7, 0x17, 0x85, 0x63, 0x86, 0x40, 0x0A, 0x44, 0xBB, 0xA4, 0xB9,
    0xCA, 0x62, 0xE7, 0x6A, 0x32, 0xC7, 0x15, 0xD4, 0xF3, 0x09, 0xC3, 0x99, 0xBF, 0x28, 0x16, 0x6F
};

static const u8 boot11Sha256[32] = {
    0x74, 0xDA, 0xAC, 0xE1, 0xF8, 0x06, 0x7B, 0x66, 0xCC, 0x81, 0xFC, 0x30, 0x7A, 0x3F, 0xDB, 0x50,
    0x9C, 0xBE, 0xDC, 0x32, 0xF9, 0x03, 0xAE, 0xBE, 0x90, 0x61, 0x44, 0xDE, 0xA7, 0xA0, 0x75, 0x12
};

static bool backupEssentialFiles(void)
{
    size_t sz = sizeof(fileCopyBuffer);

    u32 deviceID = *(vu32*)0x01FFB804;
    char pathStart[0x20];
    sprintf(pathStart, "backups/%08lX/", deviceID);
    char fullPath[0x80];



    f_mkdir("backups");
    f_mkdir(pathStart);

    bool ok = true;
    sprintf(fullPath, "%sHWCAL0.dat", pathStart);
    ok = ok && fileCopy("nand:/ro/sys/HWCAL0.dat", fullPath, false, fileCopyBuffer, sz);
    sprintf(fullPath, "%sHWCAL1.dat", pathStart);
    ok = ok && fileCopy("nand:/ro/sys/HWCAL1.dat", fullPath, false, fileCopyBuffer, sz);

    sprintf(fullPath, "%sLocalFriendCodeSeed_A", pathStart);
    ok = ok && fileCopy("nand:/rw/sys/LocalFriendCodeSeed_A", fullPath, false, fileCopyBuffer, sz);
    sprintf(fullPath, "%sLocalFriendCodeSeed_B", pathStart);
    ok = ok && fileCopy("nand:/rw/sys/LocalFriendCodeSeed_B", fullPath, false, fileCopyBuffer, sz);

    sprintf(fullPath, "%sSecureInfo_A", pathStart);
    ok = ok && fileCopy("nand:/rw/sys/SecureInfo_A", fullPath, false, fileCopyBuffer, sz);
    sprintf(fullPath, "%sSecureInfo_B", pathStart);
    ok = ok && fileCopy("nand:/rw/sys/SecureInfo_B", fullPath, false, fileCopyBuffer, sz);

    if (!ok) return false;

    alignedseqmemcpy(fileCopyBuffer, (const void *)0x10012000, 0x100);
    sprintf(fullPath, "%sotp.bin", pathStart);
    if (getFileSize(fullPath) != 0x100)
        ok = ok && fileWrite(fileCopyBuffer, fullPath, 0x100);

    if (!ok) return false;


    u8 c = mcuConsoleInfo[0];
    if (c == 2 || c == 4 || (ISN3DS && c == 5) || c == 6)
    {
        sprintf(fullPath, "%sHWCAL_01_EEPROM.dat", pathStart);
        I2C_readRegBuf(I2C_DEV_EEPROM, 0, fileCopyBuffer, 0x1000);
        if (getFileSize(fullPath) != 0x1000)
            ok = ok && fileWrite(fileCopyBuffer, fullPath, 0x1000);
    }


    u32 hash[32/4];
    sha(hash, (const void *)0x08080000, 0x10000, SHA_256_MODE);
    sprintf(fullPath, "%sboot9.bin", pathStart);
    if (memcmp(hash, boot9Sha256, 32) == 0 && getFileSize(fullPath) != 0x10000)
        ok = ok && fileWrite((const void *)0x08080000, fullPath, 0x10000);
    sha(hash, (const void *)0x08090000, 0x10000, SHA_256_MODE);
    sprintf(fullPath, "%sboot11.bin", pathStart);
    if (memcmp(hash, boot11Sha256, 32) == 0 && getFileSize(fullPath) != 0x10000)
        ok = ok && fileWrite((const void *)0x08090000, fullPath, 0x10000);

    return ok;
}

bool doLumaUpgradeProcess(void)
{
    bool ok = true, ok2 = true;


    remountCtrNandPartition(false);


#ifndef BUILD_FOR_EXPLOIT_DEV
    if (isSdMode && memcmp(launchedPathForFatfs, "sdmc:", 5) == 0)
        ok = fileCopy(launchedPathForFatfs, "nand:/boot.firm", true, fileCopyBuffer, sizeof(fileCopyBuffer));
#endif


    ok2 = backupEssentialFiles();


    fileDelete("sdmc:/luma/config.bin");
    fileDelete("nand:/rw/luma/config.bin");

    createDir("sdmc:/luma/payloads");
    createDir("nand:/rw/luma/payloads");

    return ok && ok2;
}
