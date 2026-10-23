 
#pragma once


Result srvSysInit(void);


Result srvSysExit(void);

 
Result srvSysGetServiceHandle(Handle* out, const char* name);


Result srvSysRegisterClient(void);

 
Result srvSysEnableNotification(Handle* semaphoreOut);

 
Result srvSysReceiveNotification(u32* notificationIdOut);

 
Result srvSysRegisterService(Handle* out, const char* name, int maxSessions);

 
Result srvSysUnregisterService(const char* name);
