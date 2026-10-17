#include "../../Luma3DS_v13_4/sysmodules/rosalina/source/updater_storage.c"
#include "storage.h"

UpdateResult UpdaterStorage_PrepareRead(void)
{
    if(!storageActive||!storageOpen)return UPDATE_ERR_BUSY;
    return storageIo->flush(storageIo->context,storageFile)?UPDATE_OK:UPDATE_ERR_SD;
}
