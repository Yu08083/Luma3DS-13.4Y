



#include "ff.h"


#if FF_USE_LFN == 3





#include <stdlib.h>


void* ff_memalloc (
	UINT msize
)
{
	return malloc((size_t)msize);
}


void ff_memfree (
	void* mblock
)
{
	free(mblock);
}

#endif




#if FF_FS_REENTRANT




#define OS_TYPE	0


#if   OS_TYPE == 0
#include <windows.h>
static HANDLE Mutex[FF_VOLUMES + 1];

#elif OS_TYPE == 1
#include "itron.h"
#include "kernel.h"
static mtxid Mutex[FF_VOLUMES + 1];

#elif OS_TYPE == 2
#include "includes.h"
static OS_EVENT *Mutex[FF_VOLUMES + 1];

#elif OS_TYPE == 3
#include "FreeRTOS.h"
#include "semphr.h"
static SemaphoreHandle_t Mutex[FF_VOLUMES + 1];

#elif OS_TYPE == 4
#include "cmsis_os.h"
static osMutexId Mutex[FF_VOLUMES + 1];

#endif











int ff_mutex_create (
	int vol
)
{
#if OS_TYPE == 0
	Mutex[vol] = CreateMutex(NULL, FALSE, NULL);
	return (int)(Mutex[vol] != INVALID_HANDLE_VALUE);

#elif OS_TYPE == 1
	T_CMTX cmtx = {TA_TPRI,1};

	Mutex[vol] = acre_mtx(&cmtx);
	return (int)(Mutex[vol] > 0);

#elif OS_TYPE == 2
	OS_ERR err;

	Mutex[vol] = OSMutexCreate(0, &err);
	return (int)(err == OS_NO_ERR);

#elif OS_TYPE == 3
	Mutex[vol] = xSemaphoreCreateMutex();
	return (int)(Mutex[vol] != NULL);

#elif OS_TYPE == 4
	osMutexDef(cmsis_os_mutex);

	Mutex[vol] = osMutexCreate(osMutex(cmsis_os_mutex));
	return (int)(Mutex[vol] != NULL);

#endif
}









void ff_mutex_delete (
	int vol
)
{
#if OS_TYPE == 0
	CloseHandle(Mutex[vol]);

#elif OS_TYPE == 1
	del_mtx(Mutex[vol]);

#elif OS_TYPE == 2
	OS_ERR err;

	OSMutexDel(Mutex[vol], OS_DEL_ALWAYS, &err);

#elif OS_TYPE == 3
	vSemaphoreDelete(Mutex[vol]);

#elif OS_TYPE == 4
	osMutexDelete(Mutex[vol]);

#endif
}









int ff_mutex_take (
	int vol
)
{
#if OS_TYPE == 0
	return (int)(WaitForSingleObject(Mutex[vol], FF_FS_TIMEOUT) == WAIT_OBJECT_0);

#elif OS_TYPE == 1
	return (int)(tloc_mtx(Mutex[vol], FF_FS_TIMEOUT) == E_OK);

#elif OS_TYPE == 2
	OS_ERR err;

	OSMutexPend(Mutex[vol], FF_FS_TIMEOUT, &err));
	return (int)(err == OS_NO_ERR);

#elif OS_TYPE == 3
	return (int)(xSemaphoreTake(Mutex[vol], FF_FS_TIMEOUT) == pdTRUE);

#elif OS_TYPE == 4
	return (int)(osMutexWait(Mutex[vol], FF_FS_TIMEOUT) == osOK);

#endif
}









void ff_mutex_give (
	int vol
)
{
#if OS_TYPE == 0
	ReleaseMutex(Mutex[vol]);

#elif OS_TYPE == 1
	unl_mtx(Mutex[vol]);

#elif OS_TYPE == 2
	OSMutexPost(Mutex[vol]);

#elif OS_TYPE == 3
	xSemaphoreGive(Mutex[vol]);

#elif OS_TYPE == 4
	osMutexRelease(Mutex[vol]);

#endif
}

#endif
