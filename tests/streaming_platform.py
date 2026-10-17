from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import re
import shutil
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/streaming-platform-tests'

PREFIX=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;typedef int32_t s32;typedef int64_t s64;typedef int32_t Result;typedef uint32_t Handle;typedef int LightLock;typedef int LightEvent;
typedef struct {Handle handle;void (*ep)(void);bool finished;void *stacktop;} MyThread;
typedef struct {Handle handle;u64 pos;} IFile;
typedef struct {int type;const void *data;} FS_Path;
typedef unsigned socklen_t;typedef unsigned nfds_t;
struct in_addr {u32 s_addr;};struct sockaddr {u16 sa_family;char data[14];};struct sockaddr_in {u16 sin_family,sin_port;struct in_addr sin_addr;char pad[8];};struct pollfd {int fd;short events,revents;};struct linger {int l_onoff,l_linger;};
#define AF_INET 2
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOL_SOCKET 1
#define SO_LINGER 2
#define SO_REUSEADDR 3
#define INADDR_ANY 0
#define POLLIN 1
#define POLLOUT 2
#define POLLERR 4
#define POLLHUP 8
#define POLLNVAL 16
#define ARCHIVE_SDMC 0
#define PATH_EMPTY 0
#define PATH_ASCII 1
#define FS_OPEN_READ 1
#define FS_OPEN_WRITE 2
#define FS_OPEN_CREATE 4
#define MEMOP_ALLOC 1
#define MEMOP_FREE 2
#define MEMOP_REGION_SYSTEM 4
#define MEMPERM_READWRITE 3
#define RESET_ONESHOT 0
#define CORE_SYSTEM 1
#define CUR_PROCESS_HANDLE 0xffff8001u
#define BIT(n) (1u<<(n))
#define SYSCLOCK_ARM11 268123480ULL
#define GET_VERSION_MINOR(v) (((v)>>8)&255)
#define CTR_ALIGN(n) __attribute__((aligned(n)))
#define R_SUCCEEDED(v) ((v)>=0)
#define R_FAILED(v) ((v)<0)
#define __dmb() ((void)0)
#define fsMakePath(t,d) ((FS_Path){t,d})
#include "remote_play.h"
#include "rp_jpeg.h"
#include "rp_kcopy.h"
static unsigned checks,drawDepth,allocs,frees,socStarts,socStops,createCalls,joinCalls,copyCalls,flushes,closeCalls,nonblocks,clockRestores;
static unsigned failStage,joinMask,copyChanges,metaChanges,formatChanges,selectorChanges,stopAfterSleeps,socExitFailures,jpegResult;
static unsigned statusAccesses,historyAccesses;
static bool stopOnCopy,stopOnEncode,stopOnSlotFreed,udpTransient;
static volatile bool stopRequested;
static LightLock settingsLock;
static LightEvent slotFreed;
static RemotePlayStatus *statusAccess(void);
static u32 *statusCounterAccess(u32 *);
static void *historyAccess(void *);
static bool alive[4],allocationLive;
static u32 allocationAddress=0x0e100000,allocationBytes;
static u64 mockTick;
static bool sleeping,network=true,copyTop;
bool isN3DS=true,preTerminationRequested=false;
static u32 GPU_FB_TOP_LEFT_ADDR_1,GPU_FB_TOP_LEFT_ADDR_2,GPU_FB_TOP_RIGHT_ADDR_1,GPU_FB_TOP_RIGHT_ADDR_2,GPU_FB_TOP_FMT,GPU_FB_TOP_SEL,GPU_FB_TOP_STRIDE;
static u32 GPU_FB_BOTTOM_ADDR_1,GPU_FB_BOTTOM_ADDR_2,GPU_FB_BOTTOM_FMT,GPU_FB_BOTTOM_SEL,GPU_FB_BOTTOM_STRIDE;
static u32 GPU_PSC0_CNT,GPU_PSC1_CNT,GPU_TRANSFER_CNT,GPU_CMDLIST_CNT,LCD_BOT_FILLCOLOR;
#define FB_BOTTOM_VRAM_ADDR ((void*)0x1f48f000)
#define FB_BOTTOM_VRAM_PA 0x1848f000
#define FB_BOTTOM_SIZE (320*240*2)
#define SCREEN_BOT_HEIGHT 240
#define LCD_FILLCOLOR_ENABLE (1u<<24)
static bool framebufferActive;
static u8 drawCache[FB_BOTTOM_SIZE];
static void *framebufferCache=drawCache;
static u32 framebufferCacheSize=sizeof(drawCache),gpuSavedFramebufferAddr1,gpuSavedFramebufferAddr2,gpuSavedFramebufferFormat,gpuSavedFramebufferStride,gpuSavedFramebufferSelect,gpuSavedFillColor;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(1);}}while(0)
static void Draw_Lock(void){CHECK(!drawDepth&&settingsLock==1);++drawDepth;}
static void Draw_Unlock(void){CHECK(drawDepth==1);--drawDepth;}
static void Draw_InvalidateFramebuffer(void){}
static void Draw_FlushFramebuffer(void){++flushes;}
static void Draw_ClearFramebuffer(void){memset(FB_BOTTOM_VRAM_ADDR,0x34,FB_BOTTOM_SIZE);}
static void LightLock_Lock(LightLock *lock){CHECK(*lock==1);*lock=0;}
static void LightLock_Unlock(LightLock *lock){CHECK(*lock==0);*lock=1;}
static void LightEvent_Init(LightEvent *event,int reset){(void)reset;*event=0;}
static void LightEvent_Signal(LightEvent *event){*event=1;if(stopOnSlotFreed&&event==&slotFreed)stopRequested=true;}
static void LightEvent_WaitTimeout(LightEvent *event,s64 timeout){(void)event;CHECK(timeout>0&&timeout<=10000000&&settingsLock==1);if(stopAfterSleeps&&!--stopAfterSleeps)stopRequested=true;}
static u64 svcGetSystemTick(void){mockTick+=268123;return mockTick;}
static void svcSleepThread(s64 time){CHECK(time>=0&&settingsLock==1);mockTick+=(u64)time/1000*268;if(stopAfterSleeps&&!--stopAfterSleeps)stopRequested=true;}
static u32 osGetKernelVersion(void){return 44<<8;}
static Result svcGetSystemInfo(s64 *value,u32 type,u32 selector){CHECK(!drawDepth&&type==0x10001);*value=selector==0?804:1;return 0;}
static Result svcKernelSetState(u32 op,u32 value){CHECK(!drawDepth&&op==10);if(value==3)++clockRestores;return 0;}
static void svcFlushEntireDataCache(void){CHECK(drawDepth==1);++flushes;}
static Result svcFlushProcessDataCache(Handle h,u32 address,u32 bytes){CHECK(drawDepth==1&&h==CUR_PROCESS_HANDLE&&address>=0x1f000000&&bytes<=0x600000);++flushes;return 0;}
static Result svcControlMemoryEx(u32 *,u32,u32,u32,u32,u32,bool);
static Result svcControlMemory(u32 *,u32,u32,u32,u32,u32);
static Result MyThread_Create(MyThread *,void (*)(void),void *,u32,int,int);
static Result MyThread_Join(MyThread *,s64);
static Result miniSocInitEx(bool);
static Result miniSocExitEx(bool);
static u32 miniSocRefreshHostId(void){CHECK(!drawDepth);return failStage==3?0:0x0100007f;}
static u32 socGethostid(void){return miniSocRefreshHostId();}
static int socSocket(int,int,int);
static int socSetNonBlocking(int);
static int socSetsockopt(int,int,int,const void *,socklen_t);
static int socBind(int,const struct sockaddr *,socklen_t);
static int socListen(int,int);
static int socAccept(int,struct sockaddr *,socklen_t *);
static int socPoll(struct pollfd *,nfds_t,int);
static int socClose(int);
static int socSend(int,const void *,size_t,int);
static int socRecv(int,void *,size_t,int);
static int socSendto(int,const void *,size_t,int,const struct sockaddr *,socklen_t);
static bool Wifi__IsConnected(void){CHECK(!drawDepth);return network;}
static bool Sleep__IsSleeping(void){return sleeping;}
static u16 htons(u16 x){return (u16)((x>>8)|(x<<8));}
static Result IFile_Open(IFile *f,int archive,FS_Path base,FS_Path path,int flags){CHECK(!drawDepth&&archive==ARCHIVE_SDMC&&base.type==PATH_EMPTY&&path.type==PATH_ASCII);(void)f;(void)flags;return -1;}
static Result IFile_Read(IFile *f,u64 *total,void *out,u32 size){(void)f;(void)out;(void)size;*total=0;return -1;}
static Result IFile_Write(IFile *f,u64 *total,const void *out,u32 size,int flags){(void)f;(void)out;(void)flags;*total=size;return 0;}
static Result IFile_SetSize(IFile *f,u64 size){(void)f;(void)size;return 0;}
static Result IFile_Close(IFile *f){(void)f;return 0;}
static Result IFile_Flush(IFile *f){(void)f;return 0;}
'''

SUFFIX=r'''
static void mapAt(uintptr_t address,size_t size){
#ifdef _WIN32
CHECK(VirtualAlloc((void*)address,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)==(void*)address);
#else
CHECK(mmap((void*)address,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==(void*)address);
#endif
}
static Result svcControlMemoryEx(u32 *out,u32 requested,u32 source,u32 size,u32 op,u32 perm,bool linear){CHECK(!drawDepth&&requested==BUFFER_ADDR&&!source&&size==BUFFER_SIZE&&op==(MEMOP_ALLOC|MEMOP_REGION_SYSTEM)&&perm==MEMPERM_READWRITE&&linear);++allocs;if(failStage==1)return -11;CHECK(!allocationLive);allocationLive=true;allocationBytes=size;*out=allocationAddress;return 0;}
static Result svcControlMemory(u32 *out,u32 address,u32 source,u32 size,u32 op,u32 perm){CHECK(!drawDepth&&!source&&op==MEMOP_FREE&&!perm&&allocationLive&&address==allocationAddress&&size==allocationBytes);for(unsigned k=1;k<4;++k)CHECK(!alive[k]);++frees;allocationLive=false;*out=address;return 0;}
static Result MyThread_Create(MyThread *thread,void (*entry)(void),void *stack,u32 bytes,int priority,int affinity){CHECK(!drawDepth&&allocationLive&&stack>=(void*)buffers&&(uintptr_t)stack+bytes<=(uintptr_t)buffers+BUFFER_SIZE);++createCalls;if(failStage==6+createCalls)return -20-(int)createCalls;CHECK(createCalls<=3&&!alive[createCalls]);CHECK(priority==(createCalls==1?CONTROL_THREAD_PRIO:createCalls==2?SENDER_THREAD_PRIO:ENCODER_THREAD_PRIO));CHECK(affinity==(createCalls==1?CORE_SYSTEM:VIDEO_THREAD_CORE));thread->handle=createCalls;thread->ep=entry;thread->stacktop=(u8*)stack+bytes;alive[createCalls]=true;return 0;}
static Result MyThread_Join(MyThread *thread,s64 timeout){CHECK(!drawDepth&&settingsLock==1&&thread->handle>0&&thread->handle<4&&timeout==5000000000LL&&allocationLive);++joinCalls;unsigned handle=thread->handle;CHECK(alive[handle]);if(joinMask&(1u<<handle))return -99;alive[handle]=false;thread->handle=0;return 0;}
static Result miniSocInitEx(bool exclusive){CHECK(!drawDepth&&!exclusive);++socStarts;return failStage==2?-12:0;}
static Result miniSocExitEx(bool exclusive){CHECK(!drawDepth&&!exclusive);for(unsigned k=1;k<4;++k)CHECK(!alive[k]);++socStops;if(socExitFailures){--socExitFailures;return -88;}return 0;}
static int socSocket(int domain,int type,int protocol){CHECK(!drawDepth&&domain==AF_INET&&!protocol);return failStage==4?-14:type==SOCK_STREAM?40:41;}
static int socSetNonBlocking(int sock){CHECK(!drawDepth&&(sock==40||sock==41));++nonblocks;return failStage==5?-15:0;}
static int socSetsockopt(int sock,int level,int option,const void *value,socklen_t size){CHECK(!drawDepth&&sock>=0&&level==SOL_SOCKET&&value&&size);(void)option;return 0;}
static int socBind(int sock,const struct sockaddr *address,socklen_t bytes){CHECK(!drawDepth&&sock==40&&bytes==sizeof(struct sockaddr_in));const struct sockaddr_in *a=(const void*)address;CHECK(a->sin_family==AF_INET&&a->sin_addr.s_addr==INADDR_ANY&&a->sin_port==htons(REMOTEPLAY_CONTROL_PORT));return failStage==6?-16:0;}
static int socListen(int sock,int backlog){CHECK(!drawDepth&&sock==40&&backlog==1);return 0;}
static int socAccept(int sock,struct sockaddr *address,socklen_t *bytes){CHECK(!drawDepth);(void)sock;(void)address;(void)bytes;return -EAGAIN;}
static int socPoll(struct pollfd *fds,nfds_t count,int timeout){CHECK(!drawDepth&&timeout>=0&&timeout<=100);(void)fds;(void)count;return 0;}
static int socClose(int sock){CHECK(!drawDepth&&sock>=0);++closeCalls;return 0;}
static int socSend(int sock,const void *data,size_t bytes,int flags){CHECK(!drawDepth);(void)sock;(void)data;(void)flags;return (int)bytes;}
static int socRecv(int sock,void *data,size_t bytes,int flags){CHECK(!drawDepth);(void)sock;(void)data;(void)bytes;(void)flags;return -EAGAIN;}
static int socSendto(int sock,const void *data,size_t bytes,int flags,const struct sockaddr *address,socklen_t size){CHECK(!drawDepth&&settingsLock==1&&bytes<=1448&&size==sizeof(struct sockaddr_in));(void)sock;(void)data;(void)flags;(void)address;return udpTransient?-EAGAIN:(int)bytes;}
uint32_t RpJpeg_BytesPerPixel(int format){return format==0?4:format==1?3:format>=2&&format<=4?2:0;}
void RpJpeg_SetQuality(RpJpeg *jpeg,int quality){jpeg->quality=quality;}
uint32_t RpJpeg_Encode(const RpJpeg *jpeg,u8 *out,u32 outCap,const u8 *src,u32 srcCap,u32 width,u32 rows,u32 stride,u32 rowStep,int format){CHECK(!drawDepth&&settingsLock==1&&jpeg&&out&&src&&outCap==JPEG_SLOT_CAP&&srcCap==CAPTURE_CAP&&width==240&&(rows==320||rows==400)&&stride==width*RpJpeg_BytesPerPixel(format)&&rowStep==1);if(stopOnEncode)stopRequested=true;return jpegResult;}
static void afterCopy(void){if(copyChanges){--copyChanges;if(copyTop)GPU_FB_TOP_LEFT_ADDR_1^=0x1000;else GPU_FB_BOTTOM_ADDR_1^=0x1000;}if(metaChanges){--metaChanges;if(copyTop)GPU_FB_TOP_STRIDE+=4;else GPU_FB_BOTTOM_STRIDE+=4;}if(formatChanges){--formatChanges;if(copyTop)GPU_FB_TOP_FMT=(GPU_FB_TOP_FMT&~7u)|((GPU_FB_TOP_FMT&7u)?0:2);else GPU_FB_BOTTOM_FMT=(GPU_FB_BOTTOM_FMT&~7u)|((GPU_FB_BOTTOM_FMT&7u)?0:2);}if(selectorChanges){--selectorChanges;if(copyTop)GPU_FB_TOP_SEL^=1;else GPU_FB_BOTTOM_SEL^=1;}}
bool RpCopy_Rows(void *dst,u32 dstCap,const void *src,u32 srcCap,u32 rows,u32 stride,u32 rowStep,u32 rowBytes,u32 *hash){CHECK(drawDepth==1&&settingsLock==1&&dst==buffers->capture&&dstCap==CAPTURE_CAP&&srcCap>=(rows-1)*stride*rowStep+rowBytes&&src&&(rows==320||rows==400)&&rowStep>=1&&rowStep<=2&&rows*rowBytes<=CAPTURE_CAP);++copyCalls;*hash=123;afterCopy();if(stopOnCopy)stopRequested=true;return true;}
bool RpKernelCopy_Rows(void *dst,u32 cap,u32 source,u32 rows,u32 stride,u32 step,u32 bytes,u32 *hash){CHECK(drawDepth==1&&source>=0xe0000000&&source<0xf0000000);return RpCopy_Rows(dst,cap,(void*)(uintptr_t)source,(rows-1)*stride*step+bytes,rows,stride,step,bytes,hash);}
static void reset(void){CHECK(!allocationLive&&!buffers&&!controlThread.handle&&!senderThread.handle&&!encoderThread.handle);failStage=joinMask=copyCalls=copyChanges=metaChanges=createCalls=joinCalls=0;allocs=frees=socStarts=socStops=closeCalls=nonblocks=clockRestores=0;selectorChanges=formatChanges=jpegResult=stopAfterSleeps=socExitFailures=0;stopOnCopy=stopOnEncode=stopOnSlotFreed=udpTransient=false;memset(alive,0,sizeof(alive));isN3DS=true;preTerminationRequested=false;running=stopRequested=false;framebufferActive=false;sleeping=false;settingsLock=lifecycleLock=1;allocationAddress=0x0e100000;GPU_FB_TOP_FMT=2|BIT(6);GPU_FB_BOTTOM_FMT=2;GPU_FB_TOP_STRIDE=GPU_FB_BOTTOM_STRIDE=480;GPU_FB_TOP_SEL=GPU_FB_BOTTOM_SEL=0;GPU_FB_TOP_LEFT_ADDR_1=0x18010000;GPU_FB_TOP_LEFT_ADDR_2=0x18110000;GPU_FB_TOP_RIGHT_ADDR_1=0x18010000;GPU_FB_TOP_RIGHT_ADDR_2=0x18110000;GPU_FB_BOTTOM_ADDR_1=0x18210000;GPU_FB_BOTTOM_ADDR_2=0x18310000;}
static void captures(void){Capture c;copyTop=true;CHECK(captureScreen(true,&c)&&c.pa==GPU_FB_TOP_LEFT_ADDR_1&&c.rows==400&&!drawDepth);GPU_FB_TOP_SEL=1;CHECK(captureScreen(true,&c)&&c.pa==GPU_FB_TOP_LEFT_ADDR_2);GPU_FB_TOP_SEL=0;GPU_FB_BOTTOM_SEL=1;CHECK(captureScreen(true,&c)&&c.pa==GPU_FB_TOP_LEFT_ADDR_1);copyTop=false;CHECK(captureScreen(false,&c)&&c.pa==GPU_FB_BOTTOM_ADDR_2&&c.rows==320);GPU_FB_BOTTOM_SEL=0;
unsigned before=copyCalls;Draw_Lock();Draw_SetupFramebuffer();Draw_Unlock();CHECK(framebufferActive&&!captureScreen(false,&c)&&copyCalls==before);Draw_Lock();Draw_RestoreFramebuffer();Draw_Unlock();CHECK(!framebufferActive&&captureScreen(false,&c));
for(unsigned fmt=0;fmt<8;++fmt){copyTop=true;GPU_FB_TOP_FMT=fmt|BIT(6);GPU_FB_TOP_STRIDE=fmt<5?240*RpJpeg_BytesPerPixel(fmt):480;before=copyCalls;CHECK(captureScreen(true,&c)==(fmt<5));CHECK(copyCalls-before==(fmt<5));}
GPU_FB_TOP_FMT=2|BIT(6);for(unsigned i=0;i<4;++i){const u32 bad[]={0,479,4097,UINT32_MAX};GPU_FB_TOP_STRIDE=bad[i];before=copyCalls;CHECK(!captureScreen(true,&c)&&copyCalls==before);}GPU_FB_TOP_STRIDE=480;
for(unsigned i=0;i<4;++i){const u32 bad[]={0,0x17ffffff,0x185ffffc,0x2ffffffc};GPU_FB_TOP_LEFT_ADDR_1=bad[i];before=copyCalls;CHECK(!captureScreen(true,&c)&&copyCalls==before);}GPU_FB_TOP_LEFT_ADDR_1=0x20010000;CHECK(captureScreen(true,&c)&&status.lastFromFcram);GPU_FB_TOP_LEFT_ADDR_1=0x18010000;
GPU_FB_TOP_FMT=2;CHECK(captureScreen(true,&c)&&c.rows==400);GPU_FB_TOP_FMT=2|BIT(5);CHECK(captureScreen(true,&c)&&c.rows==400);GPU_FB_TOP_FMT=2|BIT(6);
copyChanges=1;before=copyCalls;CHECK(captureScreen(true,&c)&&copyCalls==before+2);copyChanges=3;before=copyCalls;CHECK(!captureScreen(true,&c)&&copyCalls==before+3);copyChanges=0;
metaChanges=1;before=copyCalls;CHECK(captureScreen(true,&c)&&copyCalls==before+2);metaChanges=3;before=copyCalls;CHECK(!captureScreen(true,&c)&&copyCalls==before+3);metaChanges=0;GPU_FB_TOP_STRIDE=960;GPU_FB_TOP_FMT=BIT(6);formatChanges=1;before=copyCalls;CHECK(captureScreen(true,&c)&&c.format==2&&copyCalls==before+2);formatChanges=3;before=copyCalls;CHECK(!captureScreen(true,&c)&&copyCalls==before+3);formatChanges=0;GPU_FB_TOP_STRIDE=480;GPU_FB_TOP_FMT=2|BIT(6);
GPU_FB_TOP_LEFT_ADDR_2=GPU_FB_TOP_LEFT_ADDR_1;selectorChanges=1;before=copyCalls;CHECK(captureScreen(true,&c)&&copyCalls==before+2);selectorChanges=3;before=copyCalls;CHECK(!captureScreen(true,&c)&&copyCalls==before+3);selectorChanges=0;GPU_FB_TOP_SEL=0;
before=copyCalls;stopRequested=true;CHECK(!captureScreen(true,&c)&&copyCalls==before);stopRequested=false;preTerminationRequested=true;CHECK(!captureScreen(true,&c)&&copyCalls==before);preTerminationRequested=false;}
static void statistics(void){reset();CHECK(!RemotePlay_Start());RemotePlayStatus snapshot;u64 first=buffers->startTick;encoderSpent(ENC_BUSY,first,first+SYSCLOCK_ARM11);mockTick=first+SYSCLOCK_ARM11;RemotePlay_GetStatus(&snapshot);CHECK(snapshot.busyPct==100&&snapshot.clockMhz==804&&snapshot.l2Cache);
applySettings(0x0100007f,0x100,75,QOS_DEFAULT,false);jpegResult=4;stopOnEncode=true;stopRequested=false;copyTop=true;encoderThreadMain();CHECK(stopRequested&&buffers->slots[0].size==4&&status.copyUs>0&&status.encodeUs>0);stopOnEncode=false;stopRequested=false;stopOnSlotFreed=true;senderThreadMain();CHECK(stopRequested&&status.framesTop==1&&status.lastFrameBytes==4);stopOnSlotFreed=false;
JpegSlot *slot=&buffers->slots[0];slot->size=4;stopRequested=false;udpTransient=true;CHECK(sendFrame(slot)==0&&status.sendErrors==1);udpTransient=false;slot->size=0;
memset(enc.screens,0,sizeof(enc.screens));jpegResult=0;stopOnEncode=true;stopRequested=false;encoderThreadMain();CHECK(status.skippedFrames==1&&stopRequested);stopOnEncode=false;
ScreenState *sc=&enc.screens[1];sc->haveHash=true;sc->lastHash=123;sc->lastFrameTick=svcGetSystemTick();sc->lastPa=0;stopOnCopy=true;stopRequested=false;encoderThreadMain();CHECK(status.sameFrames==1&&stopRequested);stopOnCopy=false;RemotePlay_GetStatus(&snapshot);CHECK(snapshot.framesTop==1&&snapshot.sendErrors==1&&snapshot.sameFrames==1&&snapshot.skippedFrames==1);CHECK(statusAccesses>50&&historyAccesses>5);CHECK(!RemotePlay_Stop());}
int main(void){mapAt(0x0e100000,0x100000);mapAt(0x1f000000,0x600000);reset();isN3DS=false;CHECK(RemotePlay_Start()<0&&!allocs&&!socStarts);reset();preTerminationRequested=true;CHECK(RemotePlay_Start()<0&&!allocs);reset();
for(unsigned stage=1;stage<=9;++stage){reset();failStage=stage;CHECK(RemotePlay_Start()<0&&!running&&!buffers&&!allocationLive);CHECK(frees==(stage!=1)&&socStops==(stage>=3));}
for(unsigned stage=8;stage<=9;++stage){for(unsigned mask=2;mask<=(stage==9?4u:2u);mask*=2){reset();failStage=stage;joinMask=mask;CHECK(RemotePlay_Start()<0&&running&&buffers&&allocationLive&&!frees&&!socStops);CHECK(RemotePlay_Start()<0);joinMask=0;CHECK(!RemotePlay_Stop()&&!running&&!buffers&&!allocationLive&&frees==1&&socStops==1);}}
reset();allocationAddress=0x10000000;CHECK(RemotePlay_Start()<0&&!buffers&&!allocationLive&&frees==1&&!socStarts);
reset();CHECK(!RemotePlay_Start()&&running&&buffers==(void*)(uintptr_t)allocationAddress&&allocs==1&&nonblocks==2&&createCalls==3);captures();sleeping=true;void (*entries[])(void)={controlThreadMain,encoderThreadMain,senderThreadMain};for(unsigned i=0;i<3;++i){stopRequested=false;stopAfterSleeps=1;buffers->slots[snd.slot].size=1;unsigned copies=copyCalls;entries[i]();CHECK(stopRequested&&!stopAfterSleeps&&copyCalls==copies);}sleeping=false;stopRequested=false;joinMask=4;CHECK(RemotePlay_Stop()<0&&running&&buffers&&allocationLive&&!frees&&!socStops);joinMask=0;CHECK(!RemotePlay_Stop()&&!running&&!buffers&&!allocationLive&&frees==1&&socStops==1);CHECK(!RemotePlay_Stop()&&frees==1);
reset();CHECK(!RemotePlay_Start());socExitFailures=1;CHECK(RemotePlay_Stop()<0&&running&&socInitialized&&!frees&&buffers);CHECK(!RemotePlay_Stop()&&!running&&!socInitialized&&frees==1&&socStops==2);statistics();printf("%u %u %u\n",checks,statusAccesses,historyAccesses);return 0;}
'''

def extract(text,name):
    found=re.search(r'^\w[^\n]*\b'+re.escape(name)+r'\([^\n]*\)\s*\{',text,re.M)
    assert found,name
    depth=0
    for index in range(text.index('{',found.start()),len(text)):
        if text[index]=='{':depth+=1
        elif text[index]=='}':
            depth-=1
            if not depth:return text[found.start():index+1]+'\n'
    raise AssertionError(name)

def arm_proof(compiler,ctrulib,env):
    directory=OUT/'arm'
    directory.mkdir(exist_ok=True)
    flags=['-std=gnu11','-Os','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-fstack-usage','-march=armv6k','-mtune=mpcore','-mfpu=vfp','-mfloat-abi=hard','-mtp=soft','-D__3DS__','-I'+str(ROS/'include'),'-I'+str(ctrulib/'include')]
    commands=[]
    for name in ['remote_play','rp_jpeg','rp_kcopy','minisoc']:
        command=[str(compiler),*flags,'-c',str(ROS/'source'/(name+'.c')),'-o',str(directory/(name+'.o'))]
        result=subprocess.run(command,env=env,capture_output=True,text=True)
        assert result.returncode==0,result.stdout+result.stderr
        commands.append(command)
    probe=directory/'layout.c'
    probe.write_bytes(('#include "'+(ROS/'source/remote_play.c').as_posix()+'"\nconst unsigned platform_layout[]={sizeof(Buffers),BUFFER_SIZE,sizeof(JpegSlot),sizeof(EncoderState),sizeof(SenderState),sizeof(RpJpeg),CAPTURE_CAP,JPEG_SLOT_CAP,JPEG_SLOTS,CONTROL_STACK_SIZE,ENCODER_STACK_SIZE,SENDER_STACK_SIZE};\n').encode())
    command=[str(compiler),*flags,'-c',str(probe),'-o',str(directory/'layout.o')]
    result=subprocess.run(command,env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    commands.append(command)
    suffix='.exe' if compiler.suffix.lower()=='.exe' else ''
    objcopy=compiler.with_name('arm-none-eabi-objcopy'+suffix)
    result=subprocess.run([str(objcopy),'-O','binary','-j','.rodata.platform_layout',str(directory/'layout.o'),str(directory/'layout.bin')],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    values=struct.unpack('<12I',(directory/'layout.bin').read_bytes())
    keys=['buffers_size','page_aligned_buffers_size','jpeg_slot_size','encoder_state_size','sender_state_size','jpeg_encoder_size','capture_bytes','jpeg_slot_bytes','jpeg_slots','control_stack','encoder_stack','sender_stack']
    layout=dict(zip(keys,values))
    assert layout['page_aligned_buffers_size']<=0xb0000 and layout['capture_bytes']==384000
    usage={}
    for name in ['remote_play','rp_jpeg','rp_kcopy','minisoc']:
        for row in (directory/(name+'.su')).read_text().splitlines():
            fields=row.split('\t')
            if len(fields)<3:continue
            function=fields[0].rsplit(':',1)[-1].split('.')[0]
            usage[function]=max(usage.get(function,0),int(fields[1]))
    frame=lambda name:usage.get(name,0)
    encode=frame('encoderThreadMain')+frame('RpJpeg_Encode')+max(frame('encodeBlock')+frame('fdct1d')+frame('putBits'),frame('loadMcu')+frame('loadPixels'),frame('writeHeaders')+frame('putHuffTable'))
    copy=frame('encoderThreadMain')+frame('captureScreen')+frame('captureOnce')+frame('RpKernelCopy_Rows')+frame('copyRows')
    assert max(encode,copy)+4096<layout['encoder_stack']
    return dict(result='pass',translation_units=4,layout=layout,encoder_known_C_chain_bytes=encode,copy_known_C_chain_bytes=copy,encoder_reserved_boundary_bytes=4096,stack_usage=usage,commands=commands,boundary='ARM compiler stack frames plus explicit C call chains; OS/libctru and IRQ stacks remain platform boundaries')

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--firm',type=Path);parser.add_argument('--arm-cc',type=Path);parser.add_argument('--ctrulib',type=Path);args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'3ds').mkdir(exist_ok=True)
    (OUT/'3ds/types.h').write_bytes(b'#pragma once\n')
    source=(ROS/'source/remote_play.c').read_text();draw=(ROS/'source/draw.c').read_text()
    actual=re.sub(r'^#include[^\n]*\n','',source,flags=re.M)
    actual=actual.replace('static RemotePlayStatus status;','static RemotePlayStatus rawStatus;\n#define status (*statusAccess())')
    actual=actual.replace('statusIncrement(&status.','statusIncrement(&rawStatus.')
    actual=actual.replace('++*counter;','++*statusCounterAccess(counter);')
    actual=actual.replace('buffers->encHistory','((EncSecond *)historyAccess(buffers->encHistory))')
    actual=actual.replace('buffers->sndHistory','((SndSecond *)historyAccess(buffers->sndHistory))')
    guards='\n#undef status\nstatic RemotePlayStatus *statusAccess(void){CHECK(settingsLock==0);++statusAccesses;return &rawStatus;}\nstatic u32 *statusCounterAccess(u32 *counter){CHECK(settingsLock==0&&(uintptr_t)counter>=(uintptr_t)&rawStatus&&(uintptr_t)counter+sizeof(*counter)<=(uintptr_t)&rawStatus+sizeof(rawStatus));++statusAccesses;return counter;}\nstatic void *historyAccess(void *history){CHECK(settingsLock==0);++historyAccesses;return history;}\n#define status rawStatus\n'
    selected='\n'.join(extract(draw,name) for name in ['Draw_IsFramebufferActive','Draw_SetupFramebuffer','Draw_RestoreFramebuffer','Draw_GetCurrentFramebufferAddress'])
    (OUT/'fixture.c').write_bytes((PREFIX+selected+actual+guards+SUFFIX).encode())
    compiler=os.environ.get('CC','cc')
    if not shutil.which(compiler) and os.name=='nt':compiler='C:/msys64/ucrt64/bin/gcc.exe'
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    target=OUT/('platform.exe' if os.name=='nt' else 'platform')
    result=subprocess.run([compiler,'-std=gnu11','-O1','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Wno-pointer-to-int-cast','-Wno-int-to-pointer-cast','-I'+str(OUT),'-I'+str(ROS/'include'),str(OUT/'fixture.c'),'-o',str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    result=subprocess.run([str(target)],env=env,capture_output=True,text=True,timeout=30)
    assert result.returncode==0,result.stdout+result.stderr
    checks,status_accesses,history_accesses=map(int,result.stdout.split())
    report=dict(result='pass',checks=checks,locked_status_accesses=status_accesses,locked_history_accesses=history_accesses,source_sha256=hashlib.sha256((ROS/'source/remote_play.c').read_bytes()).hexdigest(),draw_sha256=hashlib.sha256((ROS/'source/draw.c').read_bytes()).hexdigest(),hardware_tested=False,boundary='actual backend and draw lifecycle; instrumented status/history accesses require settings lock; OS threads/socket/copy engines mocked; fixed VA allocations belong to native test process only')
    if args.arm_cc:
        assert args.ctrulib,'--ctrulib is required with --arm-cc'
        report['arm']=arm_proof(args.arm_cc.resolve(),args.ctrulib.resolve(),env)
    if args.firm:
        spec=importlib.util.spec_from_file_location('firm_modules',ROOT/'tools/firm_modules.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
        report['firm_modules']=mod.bundled_native_modules(args.firm.read_bytes())
    (OUT/'proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))

if __name__=='__main__':main()
