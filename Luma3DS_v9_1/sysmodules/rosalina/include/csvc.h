/*   This paricular file is licensed under the following terms: */

/*
*   This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable
*   for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it
*   and redistribute it freely, subject to the following restrictions:
*
*    The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
*    If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
*
*    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
*    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <3ds/types.h>


typedef enum ServiceOp
{
    SERVICEOP_STEAL_CLIENT_SESSION = 0, 
    SERVICEOP_GET_NAME,                 
} ServiceOp;

 
Result svcCustomBackdoor(void *func, ...);



 
u32 svcConvertVAToPA(const void *VA, bool writeCheck);

 
void svcFlushDataCacheRange(void *addr, u32 len);

 
void svcFlushEntireDataCache(void);

 
void svcInvalidateInstructionCacheRange(void *addr, u32 len);

 
void svcInvalidateEntireInstructionCache(void);




 
Result svcMapProcessMemoryEx(Handle dstProcessHandle, u32 destAddress, Handle srcProcessHandle, u32 vaSrc, u32 size);

 
Result svcUnmapProcessMemoryEx(Handle process, u32 destAddress, u32 size);

 
Result svcControlMemoryEx(u32* addr_out, u32 addr0, u32 addr1, u32 size, MemOp op, MemPerm perm, bool isLoader);




 
Result svcControlService(ServiceOp op, ...);

 
Result svcCopyHandle(Handle *out, Handle outProcess, Handle in, Handle inProcess);

 
Result svcTranslateHandle(u32 *outKAddr, char *outClassName, Handle in);


typedef enum ProcessOp
{
    PROCESSOP_GET_ALL_HANDLES,  
                                
    PROCESSOP_SET_MMU_TO_RWX,   
                                
    PROCESSOP_GET_ON_MEMORY_CHANGE_EVENT,
    PROCESSOP_GET_ON_EXIT_EVENT,
    PROCESSOP_GET_PA_FROM_VA,   
                                
} ProcessOp;

Result  svcControlProcess(Handle process, ProcessOp op, u32 varg2, u32 varg3);

