#include <3ds.h>

#include "manager.h"
#include "reslimit.h"
#include "launch.h"
#include "firmlaunch.h"
#include "termination.h"
#include "exheader_info_heap.h"
#include "task_runner.h"
#include "process_monitor.h"
#include "pmapp.h"
#include "pmdbg.h"
#include "util.h"
#include "my_thread.h"
#include "service_manager.h"
#include "luma.h"

static MyThread processMonitorThread, taskRunnerThread;
static u8 CTR_ALIGN(8) processDataBuffer[0x40 * sizeof(ProcessData)] = {0};
static u8 CTR_ALIGN(8) exheaderInfoBuffer[6 * sizeof(ExHeader_Info)] = {0};
static u8 CTR_ALIGN(8) threadStacks[2][THREAD_STACK_SIZE] = {0};


void __wrap_exit(int rc)
{
    (void)rc;

    svcExitProcess();
}

Result __sync_init(void);



void initSystem()
{
    __sync_init();
    readLumaConfig();



    for(Result res = 0xD88007FA; res == (Result)0xD88007FA; svcSleepThread(500 * 1000LL)) {
        res = srvPmInit();
        if(R_FAILED(res) && res != (Result)0xD88007FA)
            panic(res);
    }

    loaderInit();
    fsRegInit();


    Manager_Init(processDataBuffer, 0x40);
    ExHeaderInfoHeap_Init(exheaderInfoBuffer, 6);
    TaskRunner_Init();
}

static const ServiceManagerServiceEntry services[] = {
    { "pm:app",  4, pmAppHandleCommands,  false },
    { "pm:dbg",  2, pmDbgHandleCommands,  false },
    { NULL },
};

static void handleRestartHbAppNotification(u32 notificationId)
{




    (void)notificationId;
    ChainloadHomebrewDirty();
}

static const ServiceManagerNotificationEntry notifications[] = {
    { 0x3000, handleRestartHbAppNotification },
    { 0x0000, NULL },
};

void __ctru_exit(int rc) { (void)rc; }

int main(void)
{
    Result res = 0;


    initializeReslimits();
    Manager_RegisterKips();
    mapFirmlaunchParameters();


    assertSuccess(MyThread_Create(&processMonitorThread, processMonitor, NULL, threadStacks[0], THREAD_STACK_SIZE, 0x17, -2));
    assertSuccess(MyThread_Create(&taskRunnerThread, TaskRunner_HandleTasks, NULL, threadStacks[1], THREAD_STACK_SIZE, 0x17, -2));


    autolaunchSysmodules();

    if (R_FAILED(res = ServiceManager_Run(services, notifications, NULL))) {
        panic(res);
    }
    return 0;
}
