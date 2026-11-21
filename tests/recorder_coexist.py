import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def run(source):
    code = source.read_text(encoding="utf-8")
    start = code.index("typedef struct {", code.index("#define ORDER_OWNER"))
    params = code[start:code.index("_Static_assert", start)]

    def function(name):
        match = re.search(r"^static (?:int|bool|void) " + name + r"\([^\n]*\)\n\{.*?^\}", code, re.M | re.S)
        assert match, name
        return match[0]

    defs = "\n".join(re.findall(r"^#define (?:ORDER_OWNER|ORDER_RESUME|USER_PARAM_MAGIC)\s+0x\w+", code, re.M))
    root = Path(__file__).resolve().parents[1]
    loader = (root / "Luma3DS_v13_4/sysmodules/rosalina/source/plugin/plgloader.c").read_text("utf-8")
    predicate = re.search(r"^static bool IsOldRecorderOrder\([^\n]+\n\{.*?^\}", loader, re.M | re.S)
    assert predicate and "ctx->useUserLoadParameters = !IsOldRecorderOrder(params);" in loader
    src = BASE + params + defs + "\n" + MOCK + predicate[0] + "\n"
    src += "\n".join(function(name) for name in ("ReadUserParam", "GameHasPlugin", "AfterReboot", "WriteGameOrder"))
    src += function("MapPluginMemory").replace("*(const volatile u32 *)0x1FF800F0", "pluginStatus")
    src += CASES
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        cfile = root / "test.c"
        binary = root / ("test.exe" if os.name == "nt" else "test")
        cfile.write_text(src, encoding="ascii")
        compiler = os.environ.get("CC") or shutil.which("cc") or shutil.which("gcc")
        subprocess.run([compiler, "-std=c11", "-O2", str(cfile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


BASE = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t s64;
typedef int Handle, Result;
typedef const char *FS_Path;
#define FS_OPEN_READ 1
#define FS_OPEN_WRITE 2
#define FS_OPEN_CREATE 4
#define FS_WRITE_FLUSH 1
#define R_FAILED(x) ((x)<0)
#define R_SUCCEEDED(x) ((x)>=0)
#define R_SUMMARY(x) (-(x))
#define RS_NOTFOUND 2
#define FS_ATTRIBUTE_DIRECTORY 1
#define LAUNCH_3GX_FS "/3ds/recorder/rec3ds-launch.3gx"
#define USER_PARAM_FS "/luma/plugins/user_param.bin"
#define PLG_BY_US_FS "/3ds/recorder/.plugin_loader_on"
#define ORDER_TMP_FS "/3ds/recorder/.launcher-order.tmp"
#define STOPPED_FS "/3ds/recorder/stopped"
#define DIR_FS "/3ds/recorder"
#define fsMakePath(type,path) (path)
#define Log(...) ((void)0)
"""

MOCK = r"""
static struct { u32 magic; PluginLoadParams p; } order, staged;
static struct { const char *skip; Result rc; u64 tid; u32 done; } g_order;
static struct { u32 app_memtype; } kernel;
#define OS_KernelConfig (&kernel)
typedef struct { u16 name[262]; u32 attributes; } FS_DirectoryEntry;
static FS_DirectoryEntry entries[33];
static int count, pos, dirRc, defaultRc, paramRc, tmpRc, readRc, renameRace, shortWrite;
static int toggled, deleted, g_sd;
static bool flag, hasTmp, g_bootStart, g_orderKept, g_isNew3ds;
static u32 g_plgTid;
static u64 fileSize, target=0x0004000000123400ull;
static u32 g_plgSize,g_plgHeapVa,header[64],pluginStatus=1,heapMaps;
static u64 gameTitle;
#define MB (1024u*1024u)
#define CUR_PROCESS_HANDLE 0
#define PLG_HDR_VA ((uintptr_t)header)
#define PLG_MEM_VA 1u
#define PLG_MAGIC 0x24584733u
#define PluginLoadParameters PluginLoadParams
#define isN3DS g_isNew3ds
#define pluginMemoryStrategy memoryStrategy
#define PLG_STRATEGY_MODE3 1
static int ApplicationPid(void){return 10;}
static int svcOpenProcess(Handle *game,u32 pid){*game=11;return 0;}
static int svcGetProcessInfo(s64 *v,Handle h,int p){*v=gameTitle;return 0;}
static int svcMapProcessMemoryEx(Handle dst,uintptr_t addr,Handle proc,u32 at,u32 length,int flags){
    if(addr==PLG_HDR_VA)return 0;
    ++heapMaps;return -1;
}
static int svcUnmapProcessMemoryEx(Handle proc,uintptr_t addr,u32 length){return 0;}
static void MapRingMirror(void){assert(0);}
static int FSUSER_OpenFile(Handle *h, int a, FS_Path p, int flags, int mode){
    if(!strcmp(p,USER_PARAM_FS)){*h=1;return paramRc;}
    if(!strcmp(p,ORDER_TMP_FS)){*h=2;hasTmp=true;return tmpRc;}
    assert(!strcmp(p,"/luma/plugins/default.3gx"));*h=3;return defaultRc;
}
static int FSFILE_Close(Handle h){return 0;}
static int FSFILE_GetSize(Handle h, u64 *size){*size=fileSize;return 0;}
static int FSFILE_Read(Handle h,u32 *got,u32 off,void *out,u32 size){*got=size;memcpy(out,&order,size);return readRc;}
static int FSFILE_Write(Handle h,u32 *done,u32 off,const void *in,u32 size,int flush){
    assert(h==2&&size==sizeof(staged));memcpy(&staged,in,size);*done=size-shortWrite;return 0;
}
static int FSUSER_OpenDirectory(Handle *h,int a,FS_Path p){assert(!strcmp(p,"/luma/plugins/0004000000123400"));*h=4;pos=0;return dirRc;}
static int FSDIR_Read(Handle h,u32 *n,u32 max,FS_DirectoryEntry *e){
    if(pos==count){*n=0;return 0;}*n=1;*e=entries[pos++];return 0;
}
static int FSDIR_Close(Handle h){return 0;}
static int FSUSER_CreateDirectory(int a,FS_Path p,int flags){return 0;}
static int FSUSER_DeleteFile(int a,FS_Path p){
    if(!strcmp(p,USER_PARAM_FS)){deleted++;paramRc=-2;}
    if(!strcmp(p,PLG_BY_US_FS))flag=false;
    if(!strcmp(p,ORDER_TMP_FS))hasTmp=false;
    return 0;
}
static int FSUSER_RenameFile(int a,FS_Path src,int b,FS_Path dst){
    assert(!strcmp(src,ORDER_TMP_FS)&&!strcmp(dst,USER_PARAM_FS)&&hasTmp);
    if(renameRace){paramRc=0;memset(&order,0,sizeof(order));}
    if(!paramRc)return -1;
    order=staged;paramRc=0;hasTmp=false;return 0;
}
static bool SdFileExists(const char *path){return !strcmp(path,LAUNCH_3GX_FS)||flag;}
static int PluginLoader(int set){assert(set==0);++toggled;return 0;}
static int svcGetSystemInfo(s64 *out,int type,int param){*out=g_isNew3ds;return 0;}
static int StealSession(const char *s){return 1;}
static int *pmAppGetSessionHandle(void){static int h;return &h;}
static int PMAPP_GetFIRMLaunchParams(u8 *p,u32 size){memset(p,0,size);memcpy(p+0x508,&target,8);return 0;}
static int svcCloseHandle(Handle h){return 0;}
static bool NoPlugin(u32 tid){return false;}
"""

CASES = r"""
static void reset(void){
    memset(&order,0,sizeof(order)); memset(entries,0,sizeof(entries)); count=pos=0;
    defaultRc=paramRc=-RS_NOTFOUND;dirRc=tmpRc=readRc=renameRace=shortWrite=0;
    fileSize=sizeof(order);flag=hasTmp=g_bootStart=g_orderKept=g_isNew3ds=false;
    toggled=deleted=0;g_plgTid=0;kernel.app_memtype=3;
}
static void setname(unsigned i,const char *name, int directory){
    for(unsigned j=0;name[j];j++)entries[i].name[j]=(unsigned char)name[j];
    entries[i].attributes=directory;count=i+1;
}
int main(void){
    reset();WriteGameOrder(true);assert(!g_order.skip&&!deleted&&paramRc==0&&order.p.config[1]==ORDER_OWNER&&order.p.config[2]==(u32)target);
    flag=true;assert(AfterReboot()&&g_plgTid==(u32)target&&toggled==1&&deleted==1&&!flag);
    reset();setname(0,"note.txt",0);setname(1,".hidden.3gx",0);setname(2,"folder.3gx",1);assert(!GameHasPlugin(target));
    setname(3,"my_plugin.3GX",0);WriteGameOrder(true);assert(g_order.skip&&paramRc==-2&&!deleted);
    reset();defaultRc=0;WriteGameOrder(true);assert(g_order.skip&&!deleted&&paramRc==-2);
    reset();count=33;WriteGameOrder(true);assert(g_order.skip&&pos==32&&paramRc==-2);
    reset();dirRc=-7;WriteGameOrder(true);assert(g_order.skip&&paramRc==-2);
    reset();dirRc=-2;WriteGameOrder(false);assert(!g_order.skip&&order.p.config[0]==0);
    reset();defaultRc=-7;WriteGameOrder(true);assert(g_order.skip&&paramRc==-2);
    reset();paramRc=-7;WriteGameOrder(true);assert(g_order.skip&&!deleted);
    reset();paramRc=0;memset(order.p.path,'X',sizeof(order.p.path));order.magic=USER_PARAM_MAGIC;
    WriteGameOrder(true);assert(g_order.skip&&!deleted&&order.p.path[0]=='X');
    reset();renameRace=1;WriteGameOrder(true);assert(g_order.skip&&!deleted&&!hasTmp&&paramRc==0&&!order.magic);
    reset();shortWrite=1;WriteGameOrder(true);assert(g_order.skip&&!deleted&&!hasTmp&&paramRc==-2);
    reset();g_isNew3ds=true;flag=true;WriteGameOrder(true);assert(g_order.skip&&paramRc==-2);
    assert(!AfterReboot()&&!toggled&&!flag&&!deleted&&!g_plgTid);
    reset();WriteGameOrder(true);g_isNew3ds=true;flag=true;assert(!AfterReboot()&&!toggled&&!flag&&deleted==1&&!g_plgTid);
    reset();paramRc=0;flag=true;assert(!AfterReboot()&&!deleted&&!toggled&&!flag);
    reset();WriteGameOrder(true);fileSize++;assert(ReadUserParam(NULL,NULL)<0);
    assert(!IsOldRecorderOrder(&order.p));g_isNew3ds=true;assert(IsOldRecorderOrder(&order.p));
    order.p.path[sizeof(LAUNCH_3GX_FS)-1]='x';assert(!IsOldRecorderOrder(&order.p));
    memset(order.p.path,'x',sizeof(order.p.path));assert(!IsOldRecorderOrder(&order.p));strcpy(order.p.path,LAUNCH_3GX_FS);
    order.p.noFlash=0;assert(!IsOldRecorderOrder(&order.p));order.p.noFlash=1;
    order.p.persistent=1;assert(!IsOldRecorderOrder(&order.p));order.p.persistent=0;
    order.p.lowTitleId=0;assert(!IsOldRecorderOrder(&order.p));order.p.lowTitleId=(u32)target;
    order.p.memoryStrategy=0;assert(!IsOldRecorderOrder(&order.p));
    reset();gameTitle=target;g_plgTid=(u32)target;header[0]=PLG_MAGIC;header[2]=0x6000000;header[3]=2*MB;
    assert(!MapPluginMemory()&&!heapMaps);
    header[33]=ORDER_OWNER;assert(!MapPluginMemory()&&!heapMaps);
    header[34]=g_plgTid;gameTitle=target+1;assert(!MapPluginMemory()&&!heapMaps);
    gameTitle=target;g_isNew3ds=true;assert(!MapPluginMemory()&&!heapMaps);
    g_isNew3ds=false;assert(!MapPluginMemory()&&heapMaps==1);
    puts("Recorder coexistence checks passed");
}
"""

if __name__ == "__main__":
    run(Path(sys.argv[1]))
