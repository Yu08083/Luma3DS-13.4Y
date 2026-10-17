#include "updater_storage.h"
#include "updater_storage_io.h"
#include "sha256.h"
#include <string.h>
#ifndef UPDATE_STORAGE_TEST
#include <3ds.h>
#endif

static const char bootPath[]="/boot.firm",temporaryPath[]="/boot.firm.13.4Y.tmp";
static const char backupPath[]="/boot.firm.13.4Y.bak",backupTemporaryPath[]="/boot.firm.13.4Y.bak.tmp",previousPath[]="/boot.firm.13.4Y.previous";
static uint8_t storageBuffer[8192];
static UpdateManifest storageManifest;
static UpdateHooks storageHooks;
static const UpdateStorageIo *storageIo;
static uint32_t storageFile,storageWritten;
static bool storageActive,storageOpen,storageArchive;

#ifndef UPDATE_STORAGE_TEST
static FS_Archive storageSd;
static bool sdBegin(void *context) {(void)context;return R_SUCCEEDED(FSUSER_OpenArchive(&storageSd,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,"")));}
static void sdEnd(void *context) {(void)context;FSUSER_CloseArchive(storageSd);}
static int sdExists(void *context,const char *path,uint64_t *size) {
    (void)context;Handle file=0;Result result=FSUSER_OpenFile(&file,storageSd,fsMakePath(PATH_ASCII,path),FS_OPEN_READ,0);
    if(R_FAILED(result))return R_SUMMARY(result)==RS_NOTFOUND?0:-1;
    u64 bytes=0;result=FSFILE_GetSize(file,&bytes);Result closed=FSFILE_Close(file);
    if(R_FAILED(result) || R_FAILED(closed))return -1;
    *size=bytes;return 1;
}
static bool sdOpen(void *context,const char *path,bool write,uint32_t *file) {
    (void)context;Result result=FSUSER_OpenFile(file,storageSd,fsMakePath(PATH_ASCII,path),write?(FS_OPEN_WRITE|FS_OPEN_CREATE):FS_OPEN_READ,0);
    if(R_FAILED(result))return false;
    if(write && R_FAILED(FSFILE_SetSize(*file,0))){FSFILE_Close(*file);return false;}
    return true;
}
static bool sdRead(void *context,uint32_t file,uint64_t offset,void *data,uint32_t size,uint32_t *actual) {(void)context;return R_SUCCEEDED(FSFILE_Read(file,actual,offset,data,size));}
static bool sdWrite(void *context,uint32_t file,uint64_t offset,const void *data,uint32_t size,uint32_t *actual) {(void)context;return R_SUCCEEDED(FSFILE_Write(file,actual,offset,data,size,0));}
static bool sdFlush(void *context,uint32_t file) {(void)context;return R_SUCCEEDED(FSFILE_Flush(file));}
static bool sdClose(void *context,uint32_t file) {(void)context;return R_SUCCEEDED(FSFILE_Close(file));}
static bool sdRename(void *context,const char *from,const char *to) {(void)context;return R_SUCCEEDED(FSUSER_RenameFile(storageSd,fsMakePath(PATH_ASCII,from),storageSd,fsMakePath(PATH_ASCII,to)));}
static bool sdRemove(void *context,const char *path) {(void)context;return R_SUCCEEDED(FSUSER_DeleteFile(storageSd,fsMakePath(PATH_ASCII,path)));}
static const UpdateStorageIo sdIo={0,sdBegin,sdEnd,sdExists,sdOpen,sdRead,sdWrite,sdFlush,sdClose,sdRename,sdRemove};
#else
void UpdateStorage_TestIo(const UpdateStorageIo *io) {UpdateStorage_Close();storageIo=io;}
#endif

static void progress(UpdateStage stage,uint32_t current,uint32_t total) {if(storageHooks.progress)storageHooks.progress(storageHooks.context,stage,current,total);}
static bool cancelled(void) {return storageHooks.cancel && storageHooks.cancel(storageHooks.context);}
static int exists(const char *path,uint64_t *size) {return storageIo->exists(storageIo->context,path,size);}
static bool removeFile(const char *path) {uint64_t size;int found=exists(path,&size);return found==0 || (found==1 && storageIo->remove(storageIo->context,path));}
static bool beginArchive(void) {
#ifndef UPDATE_STORAGE_TEST
    storageIo=&sdIo;
#endif
    if(!storageIo || !storageIo->begin(storageIo->context))return false;
    storageArchive=true;return true;
}
static void endArchive(void) {if(storageArchive){storageIo->end(storageIo->context);storageArchive=false;}}
static bool exactRead(uint32_t file,uint64_t offset,void *data,uint32_t size) {uint32_t actual=0;return storageIo->read(storageIo->context,file,offset,data,size,&actual) && actual==size;}
static bool exactWrite(uint32_t file,uint64_t offset,const void *data,uint32_t size) {uint32_t actual=0;return storageIo->write(storageIo->context,file,offset,data,size,&actual) && actual==size;}
static bool closeFile(uint32_t file) {bool result=storageIo->close(storageIo->context,file);if(!result)storageIo->close(storageIo->context,file);return result;}

static UpdateResult hashFile(const char *path,uint32_t expected,uint8_t digest[32],bool firm,const UpdateManifest *manifest,bool canCancel) {
    uint64_t size=0;int found=exists(path,&size);
    if(found!=1)return UPDATE_ERR_SD;
    if(size<512 || size>UPDATE_MAX_FIRM || (expected && size!=expected))return UPDATE_ERR_SIZE;
    uint32_t file=0;if(!storageIo->open(storageIo->context,path,false,&file))return UPDATE_ERR_SD;
    UpdateResult result=UPDATE_OK;
    UpdateSection sections[4];uint32_t sectionCount=0;
    Sha256 whole,sectionHashes[4];Sha256_Init(&whole);
    if(firm) {
        uint8_t header[512];
        if(!exactRead(file,0,header,sizeof(header)))result=UPDATE_ERR_SD;
        else if(!Update_FirmHeader(header,(uint32_t)size,sections,&sectionCount))result=UPDATE_ERR_FIRM;
        for(uint32_t i=0;i<sectionCount;i++)Sha256_Init(sectionHashes+i);
    }
    for(uint32_t offset=0;result==UPDATE_OK && offset<(uint32_t)size;) {
        if(canCancel && cancelled()){result=UPDATE_CANCELLED;break;}
        uint32_t bytes=(uint32_t)size-offset;if(bytes>sizeof(storageBuffer))bytes=sizeof(storageBuffer);
        if(!exactRead(file,offset,storageBuffer,bytes)){result=UPDATE_ERR_SD;break;}
        Sha256_Update(&whole,storageBuffer,bytes);
        for(uint32_t i=0;i<sectionCount;i++) {
            uint32_t low=offset>sections[i].offset?offset:sections[i].offset;
            uint32_t high=offset+bytes<sections[i].offset+sections[i].size?offset+bytes:sections[i].offset+sections[i].size;
            if(low<high)Sha256_Update(sectionHashes+i,storageBuffer+low-offset,high-low);
        }
        offset+=bytes;
        if(firm)progress(UPDATE_STAGE_VERIFY,offset,(uint32_t)size);
    }
    if(!closeFile(file) && result==UPDATE_OK)result=UPDATE_ERR_SD;
    if(result!=UPDATE_OK)return result;
    uint64_t after=0;if(exists(path,&after)!=1 || after!=size)return UPDATE_ERR_SD;
    Sha256_Final(&whole,digest);
    if(manifest && memcmp(digest,manifest->sha256,32))return UPDATE_ERR_HASH;
    for(uint32_t i=0;i<sectionCount;i++){uint8_t actual[32];Sha256_Final(sectionHashes+i,actual);if(memcmp(actual,sections[i].hash,32))return UPDATE_ERR_FIRM;}
    return UPDATE_OK;
}
static UpdateResult recoverPrevious(void) {
    uint64_t previousSize=0,bootSize=0;int previous=exists(previousPath,&previousSize),boot=exists(bootPath,&bootSize);
    if(previous<0 || boot<0)return UPDATE_ERR_SD;
    if(!previous)return UPDATE_OK;
    uint8_t oldHash[32],backupHash[32],bootHash[32];
    if(previousSize<512 || previousSize>UPDATE_MAX_FIRM)return UPDATE_ERR_ROLLBACK;
    UpdateResult checked=hashFile(previousPath,(uint32_t)previousSize,oldHash,false,0,true);
    if(checked!=UPDATE_OK)return checked==UPDATE_CANCELLED?checked:UPDATE_ERR_ROLLBACK;
    checked=hashFile(backupPath,(uint32_t)previousSize,backupHash,false,0,true);
    if(checked!=UPDATE_OK || memcmp(oldHash,backupHash,32))return checked==UPDATE_CANCELLED?checked:UPDATE_ERR_ROLLBACK;
    if(boot) {
        checked=hashFile(bootPath,0,bootHash,true,0,true);
        if(checked==UPDATE_CANCELLED)return checked;
        if(checked==UPDATE_ERR_SD)return checked;
        if(checked==UPDATE_OK)return removeFile(previousPath)?UPDATE_OK:UPDATE_ERR_SD;
    }
    if(cancelled())return UPDATE_CANCELLED;
    progress(UPDATE_STAGE_ROLLBACK,0,1);
    if(boot && !removeFile(bootPath))return UPDATE_ERR_ROLLBACK;
    if(!storageIo->rename(storageIo->context,previousPath,bootPath))return UPDATE_ERR_ROLLBACK;
    return hashFile(bootPath,(uint32_t)previousSize,bootHash,false,0,false)==UPDATE_OK && !memcmp(oldHash,bootHash,32)?UPDATE_OK:UPDATE_ERR_ROLLBACK;
}
int UpdateStorage_CurrentMatches(const UpdateManifest *manifest,const UpdateHooks *hooks) {
    if(storageActive)return UPDATE_ERR_BUSY;
    if(!manifest || manifest->size<512 || manifest->size>UPDATE_MAX_FIRM)return UPDATE_ERR_METADATA;
    memset(&storageHooks,0,sizeof(storageHooks));if(hooks)storageHooks=*hooks;
    if(!beginArchive())return UPDATE_ERR_SD;
    uint64_t bytes=0;int found=exists(bootPath,&bytes);uint8_t digest[32];
    int result=found<0?UPDATE_ERR_SD:0;
    if(found==1 && bytes==manifest->size){UpdateResult checked=hashFile(bootPath,manifest->size,digest,true,manifest,true);result=checked==UPDATE_OK?1:checked==UPDATE_ERR_SD || checked==UPDATE_CANCELLED?checked:0;}
    endArchive();return result;
}
UpdateResult UpdateStorage_Begin(const UpdateManifest *manifest,const UpdateHooks *hooks) {
    if(storageActive)return UPDATE_ERR_BUSY;
    if(!manifest || manifest->size<512 || manifest->size>UPDATE_MAX_FIRM)return UPDATE_ERR_METADATA;
    memset(&storageHooks,0,sizeof(storageHooks));if(hooks)storageHooks=*hooks;
    if(!beginArchive())return UPDATE_ERR_SD;
    storageActive=true;storageManifest=*manifest;storageWritten=0;
    UpdateResult result=recoverPrevious();
    if(result==UPDATE_OK && cancelled())result=UPDATE_CANCELLED;
    if(result==UPDATE_OK && (!removeFile(temporaryPath) || !removeFile(backupTemporaryPath)))result=UPDATE_ERR_SD;
    if(result==UPDATE_OK && !storageIo->open(storageIo->context,temporaryPath,true,&storageFile))result=UPDATE_ERR_SD;
    else if(result==UPDATE_OK)storageOpen=true;
    if(result!=UPDATE_OK)UpdateStorage_Close();
    return result;
}
UpdateResult UpdateStorage_Write(const void *data,uint32_t size) {
    if(!storageActive || !storageOpen)return UPDATE_ERR_BUSY;
    if(cancelled())return UPDATE_CANCELLED;
    if(!data || !size || size>storageManifest.size-storageWritten)return UPDATE_ERR_SIZE;
    if(!exactWrite(storageFile,storageWritten,data,size))return UPDATE_ERR_SD;
    storageWritten+=size;progress(UPDATE_STAGE_DOWNLOAD,storageWritten,storageManifest.size);return UPDATE_OK;
}
static UpdateResult createBackup(uint32_t size,uint8_t originalHash[32]) {
    uint32_t source=0,target=0;bool sourceOpen=false,targetOpen=false;UpdateResult result=UPDATE_OK;
    Sha256 sourceHash;Sha256_Init(&sourceHash);
    if(!storageIo->open(storageIo->context,bootPath,false,&source))return UPDATE_ERR_BACKUP;
    sourceOpen=true;
    if(!storageIo->open(storageIo->context,backupTemporaryPath,true,&target))result=UPDATE_ERR_BACKUP;
    else targetOpen=true;
    for(uint32_t offset=0;result==UPDATE_OK && offset<size;) {
        if(cancelled()){result=UPDATE_CANCELLED;break;}
        uint32_t bytes=size-offset;if(bytes>sizeof(storageBuffer))bytes=sizeof(storageBuffer);
        if(!exactRead(source,offset,storageBuffer,bytes) || !exactWrite(target,offset,storageBuffer,bytes)){result=UPDATE_ERR_BACKUP;break;}
        Sha256_Update(&sourceHash,storageBuffer,bytes);offset+=bytes;progress(UPDATE_STAGE_BACKUP,offset,size);
    }
    if(targetOpen && !storageIo->flush(storageIo->context,target) && result==UPDATE_OK)result=UPDATE_ERR_BACKUP;
    if(sourceOpen && !closeFile(source) && result==UPDATE_OK)result=UPDATE_ERR_BACKUP;
    if(targetOpen && !closeFile(target) && result==UPDATE_OK)result=UPDATE_ERR_BACKUP;
    if(result!=UPDATE_OK)return result;
    Sha256_Final(&sourceHash,originalHash);uint8_t verified[32];
    result=hashFile(backupTemporaryPath,size,verified,false,0,true);
    if(result!=UPDATE_OK || memcmp(originalHash,verified,32))return result==UPDATE_CANCELLED?result:UPDATE_ERR_BACKUP;
    result=hashFile(bootPath,size,verified,false,0,true);
    if(result!=UPDATE_OK || memcmp(originalHash,verified,32))return result==UPDATE_CANCELLED?result:UPDATE_ERR_BACKUP;
    if(cancelled())return UPDATE_CANCELLED;
    if(!removeFile(backupPath) || !storageIo->rename(storageIo->context,backupTemporaryPath,backupPath))return UPDATE_ERR_BACKUP;
    return UPDATE_OK;
}
static UpdateResult rollback(uint32_t oldSize,const uint8_t oldHash[32],UpdateResult error) {
    progress(UPDATE_STAGE_ROLLBACK,0,1);
    if(!removeFile(bootPath) || !storageIo->rename(storageIo->context,previousPath,bootPath))return UPDATE_ERR_ROLLBACK;
    uint8_t verified[32];
    if(hashFile(bootPath,oldSize,verified,false,0,false)!=UPDATE_OK || memcmp(verified,oldHash,32))return UPDATE_ERR_ROLLBACK;
    return error;
}
UpdateResult UpdateStorage_Install(void) {
    if(!storageActive || !storageOpen)return UPDATE_ERR_BUSY;
    if(storageWritten!=storageManifest.size)return UPDATE_ERR_SIZE;
    if(cancelled())return UPDATE_CANCELLED;
    bool flushed=storageIo->flush(storageIo->context,storageFile),closed=storageIo->close(storageIo->context,storageFile);storageOpen=!closed;
    if(!flushed || !closed)return UPDATE_ERR_SD;
    uint8_t digest[32];UpdateResult result=hashFile(temporaryPath,storageManifest.size,digest,true,&storageManifest,true);
    if(result!=UPDATE_OK)return result;
    uint64_t oldBytes=0;int old=exists(bootPath,&oldBytes);if(old<0)return UPDATE_ERR_SD;
    if(old && (oldBytes<512 || oldBytes>UPDATE_MAX_FIRM))return UPDATE_ERR_BACKUP;
    uint8_t oldHash[32];
    if(old){result=createBackup((uint32_t)oldBytes,oldHash);if(result!=UPDATE_OK)return result;}
    if(cancelled())return UPDATE_CANCELLED;
    progress(UPDATE_STAGE_INSTALL,0,1);
    if(old && !storageIo->rename(storageIo->context,bootPath,previousPath))return UPDATE_ERR_SD;
    if(!storageIo->rename(storageIo->context,temporaryPath,bootPath))return old?rollback((uint32_t)oldBytes,oldHash,UPDATE_ERR_SD):UPDATE_ERR_SD;
    result=hashFile(bootPath,storageManifest.size,digest,true,&storageManifest,false);
    if(result!=UPDATE_OK)return old?rollback((uint32_t)oldBytes,oldHash,result):removeFile(bootPath)?result:UPDATE_ERR_ROLLBACK;
    if(old)removeFile(previousPath);
    progress(UPDATE_STAGE_DONE,1,1);storageActive=false;endArchive();return UPDATE_OK;
}
void UpdateStorage_Close(void) {
    if(storageActive) {
        if(storageOpen){closeFile(storageFile);storageOpen=false;}
        removeFile(temporaryPath);removeFile(backupTemporaryPath);storageActive=false;
    }
    endArchive();
}
