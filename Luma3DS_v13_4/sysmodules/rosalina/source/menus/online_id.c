#include <3ds.h>
#include "online_id_record.h"
#include "menus/online_id.h"
#include "menus/lang.h"
#include "ifile.h"
#include "menu.h"
#include "draw.h"
#include "memory.h"

static LightLock onlineIdLock = 1;

static void onlineIdRead(OnlineIdRecord records[2])
{
    memset(records, 0, 2 * sizeof(*records));
    IFile file;
    if(R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, ONLINE_ID_PATH), FS_OPEN_READ))) return;
    for(unsigned int i = 0; i < 2; ++i)
    {
        file.pos = i * sizeof(*records);
        u64 read = 0;
        if(R_FAILED(IFile_Read(&file, &read, &records[i], sizeof(*records))) || read != sizeof(*records))
            memset(&records[i], 0, sizeof(*records));
    }
    IFile_Close(&file);
}

static bool onlineIdGenerate(OnlineIdRecord records[2], int selected)
{
    OnlineIdRecord candidate = { .magic = 0x3159324Cu, .generation = selected < 0 ? 1 : records[selected].generation + 1 };
    if(R_FAILED(psInit())) return false;
    bool generated = false;
    for(unsigned int i = 0; i < 4; ++i)
    {
        if(R_FAILED(PS_GenerateRandomBytes(&candidate.value, sizeof(candidate.value)))) break;
        if(candidate.value && (selected < 0 || candidate.value != records[selected].value))
        {
            generated = true;
            break;
        }
    }
    psExit();
    if(!generated) return false;
    candidate.checksum = OnlineId_Checksum(&candidate);
    IFile file;
    if(R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, ONLINE_ID_PATH), FS_OPEN_CREATE | FS_OPEN_WRITE))) return false;
    file.pos = selected == 0 ? sizeof(candidate) : 0;
    u64 written = 0;
    Result result = IFile_Write(&file, &written, &candidate, sizeof(candidate), FS_WRITE_FLUSH);
    IFile_Close(&file);
    bool saved = R_SUCCEEDED(result) && written == sizeof(candidate);
    if(saved)
    {
        OnlineIdRecord check[2];
        onlineIdRead(check);
        int current = OnlineId_Select(check);
        saved = current >= 0 && check[current].value == candidate.value && check[current].generation == candidate.generation;
        memset(check, 0, sizeof(check));
    }
    memset(&candidate, 0, sizeof(candidate));
    return saved;
}

static bool onlineIdUpdate(bool regenerate)
{
    if(g_safeMode) return false;
    LightLock_Lock(&onlineIdLock);
    OnlineIdRecord records[2];
    onlineIdRead(records);
    int selected = OnlineId_Select(records);
    bool result = selected >= 0 && !regenerate;
    if(!result) result = onlineIdGenerate(records, selected);
    memset(records, 0, sizeof(records));
    LightLock_Unlock(&onlineIdLock);
    return result;
}

bool OnlineId_Ensure(void)
{
    return onlineIdUpdate(false);
}

bool OnlineId_Regenerate(void)
{
    return onlineIdUpdate(true);
}

void RosalinaMenu_OnlineId(void)
{
    bool available = OnlineId_Ensure();
    const char *status = available ? "Ready" : "Could not save online ID.";
    bool redraw = true;
    while(!menuShouldExit)
    {
        if(redraw)
        {
            Draw_Lock();
            Draw_ClearFramebuffer();
            Draw_DrawString(10, 10, g_accentColor, Lang_Translate("Online ID"));
            Draw_DrawString(10, 40, g_textColor, Lang_Translate(status));
            Draw_DrawString(10, 75, g_textColor, Lang_Translate("Applies when the game starts again."));
            Draw_DrawString(10, 210, g_textColor, Lang_Translate("A: Regenerate   B: Back"));
            Draw_FlushFramebuffer();
            Draw_Unlock();
            redraw = false;
        }
        u32 keys = waitInput();
        if(keys & KEY_B) break;
        if(keys & KEY_A)
        {
            status = OnlineId_Regenerate() ? "Online ID regenerated." : "Could not save online ID.";
            redraw = true;
        }
    }
}
