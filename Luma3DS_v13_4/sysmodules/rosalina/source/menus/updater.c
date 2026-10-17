#include <3ds.h>
#include <string.h>
#include "menus/updater.h"
#include "menus/lang.h"
#include "menus/ftpd.h"
#include "menu.h"
#include "draw.h"
#include "MyThread.h"
#include "updater_net.h"
#include "updater_storage.h"
#include "fmt.h"

typedef struct UpdateView {
    bool active,cancel,available;
    int result;
    UpdateStage stage;
    uint32_t done,total;
    char version[UPDATE_MAX_TAG];
} UpdateView;

typedef struct JsonBuffer {
    uint32_t used,limit;
} JsonBuffer;

static MyThread updateThread;
static unsigned char updateStack[32768] __attribute__((aligned(8)));
static LightLock updateLock=(LightLock)1;
static UpdateView view;
static UpdateManifest availableManifest;
static bool installRequested;

static const char *local(const char *english,const char *japanese)
{
    return g_language==LANG_JP?japanese:english;
}

static bool cancelUpdate(void *context)
{
    (void)context;
    LightLock_Lock(&updateLock);
    bool result=view.cancel||menuShouldExit||preTerminationRequested;
    LightLock_Unlock(&updateLock);
    return result;
}

static void progress(void *context,UpdateStage stage,uint32_t done,uint32_t total)
{
    (void)context;
    LightLock_Lock(&updateLock);
    view.stage=stage;
    view.done=done;
    view.total=total;
    LightLock_Unlock(&updateLock);
}

static int jsonBytes(void *context,const void *data,uint32_t bytes)
{
    JsonBuffer *buffer=context;
    if(bytes>buffer->limit-buffer->used)return UPDATE_ERR_SIZE;
    memcpy(g_menuWorkBuf+buffer->used,data,bytes);
    buffer->used+=bytes;
    return UPDATE_OK;
}

static int firmBytes(void *context,const void *data,uint32_t bytes)
{
    uint32_t *received=context;
    if(cancelUpdate(NULL))return UPDATE_CANCELLED;
    int result=UpdateStorage_Write(data,bytes);
    if(!result)
    {
        *received+=bytes;
        progress(NULL,UPDATE_STAGE_DOWNLOAD,*received,availableManifest.size);
    }
    return result;
}

static int checkRelease(const UpdateHooks *hooks)
{
    JsonBuffer buffer={.limit=UPDATE_MAX_JSON};
    progress(NULL,UPDATE_STAGE_RELEASE,0,0);
    int result=UpdateNet_Get("https://api.github.com/repos/Yu08083/Luma3DS-13.4Y/releases/latest",UPDATE_MAX_JSON,jsonBytes,&buffer,hooks);
    if(result)return result;
    char tag[UPDATE_MAX_TAG],url[UPDATE_MAX_URL];
    if(!Update_ParseRelease((const char *)g_menuWorkBuf,buffer.used,tag)||!Update_AssetUrl(url,tag,"update.json"))return UPDATE_ERR_METADATA;
    buffer.used=0;
    buffer.limit=UPDATE_MAX_MANIFEST;
    progress(NULL,UPDATE_STAGE_MANIFEST,0,0);
    result=UpdateNet_Get(url,UPDATE_MAX_MANIFEST,jsonBytes,&buffer,hooks);
    if(result)return result;
    UpdateManifest manifest;
    if(!Update_ParseManifest((const char *)g_menuWorkBuf,buffer.used,tag,&manifest))return UPDATE_ERR_METADATA;
    result=UpdateStorage_CurrentMatches(&manifest,hooks);
    if(result<0)return result;
    LightLock_Lock(&updateLock);
    availableManifest=manifest;
    strcpy(view.version,manifest.version);
    view.available=result==0;
    LightLock_Unlock(&updateLock);
    return result?UPDATE_CURRENT:UPDATE_OK;
}

static int installRelease(const UpdateHooks *hooks)
{
    bool ftp=ftpdEnabled;
    if(R_FAILED(Ftpd_Disable(2000000000LL)))return UPDATE_ERR_BUSY;
    int result;
    if(cancelUpdate(NULL))result=UPDATE_CANCELLED;
    else result=UpdateStorage_Begin(&availableManifest,hooks);
    if(!result)
    {
        char url[UPDATE_MAX_URL];
        uint32_t received=0;
        if(!Update_AssetUrl(url,availableManifest.version,"boot.firm"))result=UPDATE_ERR_URL;
        else result=UpdateNet_Get(url,availableManifest.size,firmBytes,&received,hooks);
        if(!result&&received!=availableManifest.size)result=UPDATE_ERR_SIZE;
        if(!result)result=UpdateStorage_Install();
    }
    UpdateStorage_Close();
    if(ftp&&!menuShouldExit&&!preTerminationRequested)
    {
        ftpdEnabled=true;
        ftpdCreateThread();
    }
    return result;
}

static void updateWorker(void)
{
    UpdateHooks hooks={.cancel=cancelUpdate,.progress=progress};
    int result=installRequested?installRelease(&hooks):checkRelease(&hooks);
    LightLock_Lock(&updateLock);
    view.result=result;
    view.active=false;
    view.stage=UPDATE_STAGE_DONE;
    if(installRequested&&result==UPDATE_OK)view.available=false;
    LightLock_Unlock(&updateLock);
}

static bool begin(bool install)
{
    if(updateThread.handle&&R_FAILED(MyThread_Join(&updateThread,0)))return false;
    LightLock_Lock(&updateLock);
    view.active=true;
    view.cancel=false;
    view.result=UPDATE_OK;
    view.stage=install?UPDATE_STAGE_DOWNLOAD:UPDATE_STAGE_RELEASE;
    view.done=0;
    view.total=0;
    if(!install){view.available=false;view.version[0]=0;}
    installRequested=install;
    LightLock_Unlock(&updateLock);
    if(R_FAILED(MyThread_Create(&updateThread,updateWorker,updateStack,sizeof(updateStack),57,CORE_SYSTEM)))
    {
        updateThread.handle=0;
        LightLock_Lock(&updateLock);
        view.active=false;
        view.result=UPDATE_ERR_BUSY;
        view.stage=UPDATE_STAGE_DONE;
        LightLock_Unlock(&updateLock);
        return false;
    }
    return true;
}

static const char *status(const UpdateView *snapshot)
{
    if(snapshot->active)
    {
        if(snapshot->cancel)return local("Cancelling...","中止しています...");
        switch(snapshot->stage)
        {
            case UPDATE_STAGE_RELEASE:return local("Checking the latest release...","最新リリースを確認中...");
            case UPDATE_STAGE_MANIFEST:return local("Checking update information...","更新情報を確認中...");
            case UPDATE_STAGE_CONNECT:return local("Connecting securely...","安全な接続を確認中...");
            case UPDATE_STAGE_DOWNLOAD:return local("Downloading boot.firm...","boot.firmを取得中...");
            case UPDATE_STAGE_VERIFY:return local("Verifying the file...","ファイルを検証中...");
            case UPDATE_STAGE_BACKUP:return local("Saving the current boot.firm...","現在のboot.firmを保存中...");
            case UPDATE_STAGE_INSTALL:return local("Installing the update...","更新しています...");
            case UPDATE_STAGE_ROLLBACK:return local("Restoring the previous file...","元のファイルに戻しています...");
            default:return local("Please wait...","お待ちください...");
        }
    }
    if(snapshot->stage==UPDATE_STAGE_NONE)return local("Check for a new version.","最新の更新を確認できます。");
    switch(snapshot->result)
    {
        case UPDATE_OK:
            return snapshot->available?local("An update is ready.","更新を取得できます。"):local("Updated. Restart to apply.","更新しました。再起動で反映します。");
        case UPDATE_CURRENT:return local("Your boot.firm is up to date.","boot.firmは最新です。");
        case UPDATE_CANCELLED:return local("Cancelled.","中止しました。");
        case UPDATE_ERR_TLS:return local("Secure connection failed.\nCheck the system date and Wi-Fi.","安全な接続に失敗しました。\n本体の日時とWi-Fiを確認してください。");
        case UPDATE_ERR_NETWORK:return local("Could not connect.\nCheck Wi-Fi and try again.","接続できませんでした。\nWi-Fiを確認して再度お試しください。");
        case UPDATE_ERR_HTTP:
        case UPDATE_ERR_METADATA:return local("No usable update was found.\nTry again later.","利用できる更新が見つかりません。\n時間をおいて再度お試しください。");
        case UPDATE_ERR_SIZE:
        case UPDATE_ERR_HASH:
        case UPDATE_ERR_FIRM:
        case UPDATE_ERR_URL:return local("The update could not be verified.\nThe update was not installed.","更新データを検証できませんでした。\n更新は適用していません。");
        case UPDATE_ERR_ROLLBACK:return local("SD recovery needs attention.\nKeep the backup on the SD card.","SDの復旧を確認してください。\nSD上のバックアップを残してください。");
        case UPDATE_ERR_BACKUP:return local("Could not save the current file.\nCheck the SD card and free space.","現在のファイルを保存できません。\nSDカードの空き容量を確認してください。");
        case UPDATE_ERR_BUSY:return local("Another task is still stopping.\nTry again shortly.","別の処理が終了中です。\n少し待って再度お試しください。");
        default:return local("SD operation failed.\nCheck the SD card and try again.","SDの操作に失敗しました。\nSDカードを確認してください。");
    }
}

void RosalinaMenu_Update(void)
{
    if(g_safeMode)return;
    bool first=true,back=false;
    UpdateView previous={0};
    for(;;)
    {
        LightLock_Lock(&updateLock);
        UpdateView snapshot=view;
        if(menuShouldExit||preTerminationRequested){view.cancel=true;back=true;}
        LightLock_Unlock(&updateLock);
        if(!snapshot.active&&updateThread.handle)MyThread_Join(&updateThread,0);
        if(back&&!snapshot.active&&!updateThread.handle)break;
        if(first||memcmp(&snapshot,&previous,sizeof(snapshot)))
        {
            Draw_Lock();
            Draw_ClearFramebuffer();
            Draw_DrawString(10,10,g_accentColor,local("Update Luma3DS","Luma3DSを更新"));
            Draw_DrawString(10,40,g_textColor,status(&snapshot));
            if(snapshot.version[0])Draw_DrawFormattedString(10,95,g_textColor,"%s",snapshot.version);
            if(snapshot.total)Draw_DrawFormattedString(10,115,g_textColor,"%lu / %lu KiB",(unsigned long)(snapshot.done/1024),(unsigned long)((snapshot.total+1023)/1024));
            Draw_DrawString(10,150,g_textColor,local("Settings and themes are kept.\nOnly SD:/boot.firm is updated.","設定とテーマ画像は保持します。\nSD:/boot.firmを更新します。"));
            Draw_DrawString(10,205,g_textColor,snapshot.active?local("B: Cancel","B: 中止"):snapshot.available?local("A: Install   X: Check   B: Back","A: 更新  X: 再確認  B: 戻る"):local("A: Check   B: Back","A: 更新を確認  B: 戻る"));
            Draw_FlushFramebuffer();
            Draw_Unlock();
            previous=snapshot;
            first=false;
        }
        u32 keys=waitInputWithTimeout(100);
        if(keys&KEY_B)
        {
            if(snapshot.active)
            {
                LightLock_Lock(&updateLock);
                view.cancel=true;
                LightLock_Unlock(&updateLock);
                back=true;
            }
            else back=true;
        }
        if(!snapshot.active&&!back)
        {
            if((keys&KEY_X)&&snapshot.available)begin(false);
            else if(keys&KEY_A)begin(snapshot.available);
        }
        if(back&&snapshot.active)svcSleepThread(10000000);
    }
}
