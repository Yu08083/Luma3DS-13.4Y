#include <3ds.h>
#include <string.h>
#include "app.h"
#include "updater_net.h"
#include "storage.h"
#include "firm_gate.h"

typedef struct JsonBuffer {
    uint32_t used,limit;
} JsonBuffer;

static LightLock appLock;
static Thread worker;
static UpdaterAppView view;
static UpdateManifest availableManifest;
static unsigned char json[UPDATE_MAX_JSON];
static bool shutdownRequested,installRequested;

static bool cancelled(void *context)
{
    (void)context;
    LightLock_Lock(&appLock);
    bool result=view.cancelling||shutdownRequested;
    LightLock_Unlock(&appLock);
    return result;
}

static void progress(void *context,UpdateStage stage,uint32_t done,uint32_t total)
{
    (void)context;
    LightLock_Lock(&appLock);
    view.stage=stage;
    view.done=done;
    view.total=total;
    LightLock_Unlock(&appLock);
}

static int jsonBytes(void *context,const void *data,uint32_t bytes)
{
    JsonBuffer *buffer=context;
    if(bytes>buffer->limit-buffer->used)return UPDATE_ERR_SIZE;
    memcpy(json+buffer->used,data,bytes);
    buffer->used+=bytes;
    return UPDATE_OK;
}

static bool readFile(void *context,uint32_t offset,void *data,uint32_t bytes)
{
    Handle *file=context;
    u32 actual=0;
    return R_SUCCEEDED(FSFILE_Read(*file,&actual,offset,data,bytes))&&actual==bytes;
}

static UpdateResult gateFile(const char *path,uint32_t expected)
{
    FS_Archive archive;
    if(R_FAILED(FSUSER_OpenArchive(&archive,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""))))return UPDATE_ERR_SD;
    Handle file=0;
    UpdateResult result=UPDATE_ERR_SD;
    if(R_SUCCEEDED(FSUSER_OpenFile(&file,archive,fsMakePath(PATH_ASCII,path),FS_OPEN_READ,0)))
    {
        u64 bytes=0;
        if(R_SUCCEEDED(FSFILE_GetSize(file,&bytes))&&bytes==expected)
            result=UpdaterFirm_Gate(readFile,&file,expected);
        if(R_FAILED(FSFILE_Close(file)))result=UPDATE_ERR_SD;
    }
    if(R_FAILED(FSUSER_CloseArchive(archive)))result=UPDATE_ERR_SD;
    return result;
}

static int firmBytes(void *context,const void *data,uint32_t bytes)
{
    uint32_t *received=context;
    if(cancelled(NULL))return UPDATE_CANCELLED;
    int result=UpdateStorage_Write(data,bytes);
    if(!result)
    {
        *received+=bytes;
        progress(NULL,UPDATE_STAGE_DOWNLOAD,*received,availableManifest.size);
    }
    return result;
}

static UpdateResult checkRelease(const UpdateHooks *hooks)
{
    JsonBuffer buffer={.limit=UPDATE_MAX_JSON};
    progress(NULL,UPDATE_STAGE_RELEASE,0,0);
    int result=UpdateNet_Get("https://api.github.com/repos/Yu08083/Luma3DS-13.4Y/releases/latest",UPDATE_MAX_JSON,jsonBytes,&buffer,hooks);
    if(result)return (UpdateResult)result;
    char tag[UPDATE_MAX_TAG],url[UPDATE_MAX_URL];
    if(!Update_ParseRelease((const char *)json,buffer.used,tag)||!Update_AssetUrl(url,tag,"update.json"))return UPDATE_ERR_METADATA;
    buffer.used=0;
    buffer.limit=UPDATE_MAX_MANIFEST;
    progress(NULL,UPDATE_STAGE_MANIFEST,0,0);
    result=UpdateNet_Get(url,UPDATE_MAX_MANIFEST,jsonBytes,&buffer,hooks);
    if(result)return (UpdateResult)result;
    UpdateManifest manifest;
    if(!Update_ParseManifest((const char *)json,buffer.used,tag,&manifest))return UPDATE_ERR_METADATA;
    result=UpdateStorage_CurrentMatches(&manifest,hooks);
    if(result<0)return (UpdateResult)result;
    if(result==1)
    {
        UpdateResult gate=gateFile("/boot.firm",manifest.size);
        if(gate)return gate;
    }
    LightLock_Lock(&appLock);
    availableManifest=manifest;
    memcpy(view.version,manifest.version,sizeof(view.version));
    view.available=result==0;
    LightLock_Unlock(&appLock);
    return result?UPDATE_CURRENT:UPDATE_OK;
}

static UpdateResult installRelease(const UpdateHooks *hooks)
{
    UpdateResult result=cancelled(NULL)?UPDATE_CANCELLED:UpdateStorage_Begin(&availableManifest,hooks);
    if(result==UPDATE_OK)
    {
        char url[UPDATE_MAX_URL];
        uint32_t received=0;
        if(!Update_AssetUrl(url,availableManifest.version,"boot.firm"))result=UPDATE_ERR_URL;
        else result=(UpdateResult)UpdateNet_Get(url,availableManifest.size,firmBytes,&received,hooks);
        if(result==UPDATE_OK&&received!=availableManifest.size)result=UPDATE_ERR_SIZE;
        if(result==UPDATE_OK&&cancelled(NULL))result=UPDATE_CANCELLED;
        if(result==UPDATE_OK)result=UpdaterStorage_PrepareRead();
        if(result==UPDATE_OK)result=gateFile("/boot.firm.13.4Y.tmp",availableManifest.size);
        if(result==UPDATE_OK)result=UpdateStorage_Install();
    }
    UpdateStorage_Close();
    return result;
}

static void run(void *context)
{
    (void)context;
    UpdateHooks hooks={.cancel=cancelled,.progress=progress};
    UpdateResult result=installRequested?installRelease(&hooks):checkRelease(&hooks);
    LightLock_Lock(&appLock);
    view.result=result;
    view.busy=false;
    view.cancelling=false;
    view.stage=UPDATE_STAGE_DONE;
    if(installRequested&&result==UPDATE_OK)view.available=false;
    LightLock_Unlock(&appLock);
}

void UpdaterApp_Init(void)
{
    LightLock_Init(&appLock);
    memset(&view,0,sizeof(view));
    memset(&availableManifest,0,sizeof(availableManifest));
    worker=NULL;
    shutdownRequested=false;
}

static bool request(bool install)
{
    LightLock_Lock(&appLock);
    bool allowed=!view.busy&&!shutdownRequested&&(!install||view.available);
    LightLock_Unlock(&appLock);
    if(!allowed)return false;
    if(worker)
    {
        if(R_FAILED(threadJoin(worker,0)))return false;
        threadFree(worker);
        worker=NULL;
    }
    LightLock_Lock(&appLock);
    view.busy=true;
    view.cancelling=false;
    view.installing=install;
    view.result=UPDATE_OK;
    view.stage=install?UPDATE_STAGE_DOWNLOAD:UPDATE_STAGE_RELEASE;
    view.done=0;
    view.total=0;
    if(!install){view.available=false;view.version[0]=0;}
    installRequested=install;
    LightLock_Unlock(&appLock);
    worker=threadCreate(run,NULL,65536,0x30,-2,false);
    if(!worker)
    {
        LightLock_Lock(&appLock);
        view.busy=false;
        view.result=UPDATE_ERR_BUSY;
        view.stage=UPDATE_STAGE_DONE;
        LightLock_Unlock(&appLock);
        return false;
    }
    return true;
}

bool UpdaterApp_RequestCheck(void)
{
    return request(false);
}

bool UpdaterApp_RequestInstall(void)
{
    return request(true);
}

void UpdaterApp_Cancel(void)
{
    LightLock_Lock(&appLock);
    if(view.busy)view.cancelling=true;
    LightLock_Unlock(&appLock);
}

void UpdaterApp_Snapshot(UpdaterAppView *snapshot)
{
    if(!snapshot)return;
    LightLock_Lock(&appLock);
    *snapshot=view;
    LightLock_Unlock(&appLock);
}

void UpdaterApp_Shutdown(void)
{
    LightLock_Lock(&appLock);
    shutdownRequested=true;
    if(view.busy)view.cancelling=true;
    LightLock_Unlock(&appLock);
    if(worker)
    {
        threadJoin(worker,UINT64_MAX);
        threadFree(worker);
        worker=NULL;
    }
    UpdateStorage_Close();
}
