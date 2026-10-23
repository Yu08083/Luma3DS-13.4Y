from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ROS = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina'
OUT = ROOT / 'build/streaming-audio-tests'
OUT.mkdir(parents=True, exist_ok=True)
source = (ROS / 'source/remote_play.c').read_text()
minisoc = (ROS / 'source/minisoc.c').read_text()
dsp = (ROS / 'source/dsp_audio.c').read_text()

def extract(text, name):
    found = re.search(r'^\w[^\n]*\b' + name + r'\([^\n]*\)\s*\{', text, re.M)
    assert found, name
    depth = 0
    for i in range(text.index('{', found.start()), len(text)):
        depth += (text[i] == '{') - (text[i] == '}')
        if not depth:
            return text[found.start():i + 1] + '\n'
    raise AssertionError(name)

def declaration(name):
    start = source.index('typedef struct ' + name + '\n')
    end = source.index('} ' + name + ';', start) + len('} ' + name + ';')
    return source[start:end] + '\n'

prefix = r'''
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;typedef int16_t s16;typedef int64_t s64;typedef volatile uint16_t vu16;typedef int LightLock;
typedef unsigned socklen_t;
#define CUR_PROCESS_HANDLE 0xffff8001u
#define SYSCLOCK_ARM11 268123480ULL
#define CTR_ALIGN(n) __attribute__((aligned(n)))
#define AF_INET 2
#define SOCK_DGRAM 2
#define SOL_SOCKET 1
#define SO_LINGER 2
#define POLLIN 1
#define POLLERR 4
#define POLLHUP 8
#define POLLNVAL 16
struct in_addr{u32 s_addr;};struct sockaddr{u16 sa_family;char sa_data[14];};
struct sockaddr_in{u16 sin_family,sin_port;struct in_addr sin_addr;char pad[8];};
struct pollfd{int fd;short events,revents;};
struct linger{int l_onoff,l_linger;};
#define CHECK(v) do{++checks;assert(v);}while(0)
static unsigned checks,cacheCalls,fcReads[2],tearRegion=3,sleeps,queue,received,sent,closed,readerPolls;
static int sendResult=-999,recvResult=-999,pollResult=1;
static u64 nowTick;static bool sleeping,stopRequested,preTerminationRequested,linkDown,checkLinkNow;
static int audioSock=42;static LightLock settingsLock=1;
static struct{u32 audioPlayerIp,audioFrames,audioMissed,audioSendErrors;}status;
static u8 wire[1400];static unsigned wireSize;static struct sockaddr_in destination;
static bool validHello=true,endToEndAudio;static u8 peerLength=8,peerFamily=AF_INET;static u32 peerIp=0x04030201;static u16 peerPort=0x1234;
static u16 htons(u16 n){return (n<<8)|(n>>8);}
static void LightLock_Lock(LightLock *l){CHECK(*l==1);*l=0;}
static void LightLock_Unlock(LightLock *l){CHECK(*l==0);*l=1;}
static void statusIncrement(u32 *c){LightLock_Lock(&settingsLock);++*c;LightLock_Unlock(&settingsLock);}
static int svcInvalidateProcessDataCache(u32 process,u32 address,u32 size){CHECK(process==CUR_PROCESS_HANDLE);++cacheCalls;for(unsigned i=0;i<2;++i)if(address==0x1ff57ffe + i*0x20000){CHECK(size==2);if(++fcReads[i]==2&&tearRegion==i)*(vu16*)(uintptr_t)address+=2;}return 0;}
static int socSetsockopt(int socket,int level,int opt,const void *data,unsigned bytes){CHECK(socket==42&&level==SOL_SOCKET&&opt==SO_LINGER&&data&&bytes==sizeof(struct linger));return 0;}
static int socClose(int socket){CHECK(socket==42);++closed;return 0;}
static int openSocket(int kind){CHECK(kind==SOCK_DGRAM);return 42;}
static u32 socGethostid(void){return 0x0100007f;}
static int socBind(int socket,const struct sockaddr *address,socklen_t size){const struct sockaddr_in *a=(const void*)address;CHECK(socket==42&&size==sizeof(*a)&&a->sin_port==htons(8002)&&a->sin_addr.s_addr==0x0100007f);return 0;}
static int socPoll(struct pollfd *fd,unsigned n,int ms){CHECK(fd->fd==42&&n==1&&!ms);fd->revents=queue?POLLIN:0;return queue?pollResult:0;}
static int socRecvfrom(int socket,void *data,unsigned size,int flags,struct sockaddr *address,socklen_t *length){CHECK(socket==42&&size==16&&!flags&&queue&&*length==sizeof(struct sockaddr_in));++received;--queue;u8 nativeAddress[28]={peerLength,peerFamily};memcpy(nativeAddress+2,&peerPort,2);memcpy(nativeAddress+4,&peerIp,4);copySockaddr(address,length,nativeAddress);memcpy(data,validHello?"L2AH":"BAD!",4);return recvResult==-999?4:recvResult;}
static int socSendto(int socket,const void *data,unsigned size,int flags,const struct sockaddr *address,socklen_t length){CHECK(socket==42&&!flags&&size<=sizeof(wire)&&length==sizeof(destination));++sent;memcpy(wire,data,size);wireSize=size;memcpy(&destination,address,length);return sendResult==-999?(int)size:sendResult;}
static bool Sleep__IsSleeping(void){return sleeping;}
static u64 svcGetSystemTick(void){return nowTick;}
static void svcSleepThread(s64 ns){CHECK(ns>0&&ns<=1000000000LL);++sleeps;if(endToEndAudio && sleeps==1){*(vu16*)0x1ff57ffe += 2;*(vu16*)0x1ff77ffe += 2;return;}stopRequested=true;}
'''
prefix = prefix.replace('static int socRecvfrom', extract(minisoc, 'copySockaddr') + '\nstatic int socRecvfrom')
header = re.sub(r'^#(?:pragma|include)[^\n]*\n', '', (ROS / 'include/dsp_audio.h').read_text(), flags=re.M)
defines = '\n'.join(line for line in source.splitlines() if line.startswith('#define ') and
                    (line.split()[1].startswith('AUDIO_') or line.split()[1] in ('TICKS_PER_US', 'LINK_DOWN_SLEEP_NS')))
definitions = prefix + header + defines + '\n#define REMOTEPLAY_AUDIO_PORT 8002\n' + declaration('AudioState') + 'static AudioState aud;\n'
actual_dsp = re.sub(r'^#include[^\n]*\n', '', dsp, flags=re.M)
functions = '\n'.join(extract(source, name) for name in ('closeSocket', 'transient', 'openAudioSocket', 'forgetPlayer', 'startPlayer', 'checkHello', 'putLe16', 'putLe32', 'sendAudio', 'addFrame', 'audioThreadMain'))
suffix = r'''
static void fc(unsigned a,unsigned b){*(vu16*)0x1ff57ffe=a;*(vu16*)0x1ff77ffe=b;memset(fcReads,0,sizeof(fcReads));tearRegion=3;}
static void reset(void){memset(&aud,0,sizeof(aud));memset(&status,0,sizeof(status));settingsLock=1;audioSock=42;closed=received=sent=queue=sleeps=0;sendResult=recvResult=-999;pollResult=1;stopRequested=preTerminationRequested=linkDown=checkLinkNow=sleeping=false;validHello=true;endToEndAudio=false;peerLength=8;peerFamily=AF_INET;peerIp=0x04030201;peerPort=0x1234;nowTick=1000000;fc(100,101);}
static void reader_tests(void){DspAudioReader r;DspAudioBlock out[2];fc(100,101);memset((void*)0x1ff50a80,0x12,640);memset((void*)0x1ff70a80,0x34,640);DspAudio_ReaderInit(&r);CHECK(DspAudio_Poll(&r,out)==0);fc(104,103);CHECK(DspAudio_Poll(&r,out)==2&&out[0].frame==103&&out[1].frame==104);for(unsigned i=0;i<640;++i)CHECK(out[0].samples[i]==0x34&&out[1].samples[i]==0x12);
fc(65534,65535);DspAudio_ReaderInit(&r);fc(0,1);CHECK(DspAudio_Poll(&r,out)==2&&out[0].frame==0&&out[1].frame==1);fc(10,11);DspAudio_ReaderInit(&r);fc(11,11);for(unsigned i=0;i<3;++i)CHECK(DspAudio_Poll(&r,out)==0);CHECK(r.lastFc[0]==11);fc(13,11);CHECK(DspAudio_Poll(&r,out)==1&&out[0].frame==13);
fc(20,21);DspAudio_ReaderInit(&r);fc(100,21);for(unsigned i=0;i<3;++i)CHECK(DspAudio_Poll(&r,out)==0);fc(102,21);CHECK(DspAudio_Poll(&r,out)==1&&out[0].frame==102);
fc(100,101);DspAudio_ReaderInit(&r);fc(102,101);tearRegion=0;CHECK(DspAudio_Poll(&r,out)==0);tearRegion=3;CHECK(DspAudio_Poll(&r,out)==1&&out[0].frame==104);}
static void hello_contract(void){
reset();queue=1;checkHello(nowTick);CHECK(received==1&&aud.playerIp==peerIp&&aud.playerPort==peerPort&&cacheCalls>0);
for(unsigned length=0;length<8;++length){reset();peerLength=length;queue=1;unsigned before=cacheCalls;checkHello(nowTick);CHECK(!aud.playerIp&&!status.audioPlayerIp&&cacheCalls==before);}
for(unsigned mode=0;mode<5;++mode){reset();queue=1;if(mode==0)peerFamily=10;if(mode==1)peerIp=0;if(mode==2)peerPort=0;if(mode==3)validHello=false;if(mode==4)peerLength=29;unsigned before=cacheCalls;checkHello(nowTick);CHECK(!aud.playerIp&&!status.audioPlayerIp&&cacheCalls==before);}
reset();queue=1;endToEndAudio=true;nowTick=(u64)AUDIO_HELLO_CHECK_US*TICKS_PER_US+1;memset((void*)0x1ff50a80,0x12,640);memset((void*)0x1ff70a80,0x34,640);audioThreadMain();CHECK(received==1&&sleeps==2&&sent==1&&status.audioFrames==2&&wireSize==1292&&!memcmp(wire,"L2AU",4)&&destination.sin_addr.s_addr==peerIp&&destination.sin_port==peerPort);
for(unsigned i=0;i<640;++i)CHECK(wire[12+i]==0x12&&wire[652+i]==0x34);
}
static void packets(void){reset();CHECK(openAudioSocket()==42);queue=20;checkHello(nowTick);CHECK(received==8&&queue==12&&aud.playerIp==peerIp&&aud.playerPort==peerPort&&status.audioPlayerIp==peerIp);queue=0;
DspAudioBlock b={.frame=65535};memset(b.samples,0x5a,sizeof(b.samples));addFrame(&b,nowTick);CHECK(aud.pending==1&&!sent);b.frame=0;memset(b.samples,0xa5,sizeof(b.samples));addFrame(&b,nowTick);CHECK(sent==1&&wireSize==1292&&!memcmp(wire,"L2AU\x01\x00\x02\x00\xff\xff\x00\x00",12)&&destination.sin_port==peerPort&&destination.sin_addr.s_addr==peerIp&&status.audioFrames==2);for(unsigned i=0;i<640;++i)CHECK(wire[12+i]==0x5a&&wire[652+i]==0xa5);CHECK(aud.lastIndex==65536);addFrame(&b,nowTick);CHECK(sent==1&&!aud.pending);b.frame=65535;addFrame(&b,nowTick);CHECK(sent==1&&!aud.pending);
b.frame=2;addFrame(&b,nowTick);sendAudio();CHECK(sent==2&&wireSize==652&&wire[6]==1&&wire[8]==2&&wire[10]==1&&status.audioMissed==1);b.frame=3;addFrame(&b,nowTick);sendResult=-EAGAIN;sendAudio();CHECK(audioSock==42&&!aud.pending&&status.audioSendErrors==1);b.frame=4;addFrame(&b,nowTick);sendResult=7;sendAudio();CHECK(audioSock==-1&&closed==1&&!aud.pending&&checkLinkNow&&status.audioSendErrors==2);
reset();queue=1;validHello=false;checkHello(nowTick);CHECK(!aud.playerIp&&!status.audioPlayerIp);queue=1;validHello=true;recvResult=-EINTR;checkHello(nowTick);CHECK(audioSock==42&&!aud.playerIp);queue=1;recvResult=-EIO;checkHello(nowTick);CHECK(audioSock==-1);reset();queue=1;peerPort=0;checkHello(nowTick);CHECK(!aud.playerIp);
reset();startPlayer(peerIp,peerPort);b.frame=20000;addFrame(&b,nowTick);b.frame=0;addFrame(&b,nowTick);sendAudio();CHECK(sent==2&&aud.lastIndex==65536&&wire[8]==0&&wire[9]==0&&wire[10]==1);}
static void threads(void){reset();unsigned before=cacheCalls;audioThreadMain();CHECK(stopRequested&&sleeps==1&&cacheCalls==before);reset();sleeping=true;aud.playerIp=peerIp;aud.pending=1;before=cacheCalls;audioThreadMain();CHECK(audioSock==-1&&!aud.playerIp&&!aud.pending&&status.audioPlayerIp==0&&cacheCalls==before);reset();linkDown=true;before=cacheCalls;audioThreadMain();CHECK(audioSock==-1&&cacheCalls==before);
reset();startPlayer(peerIp,peerPort);aud.lastHelloTick=0;nowTick=(u64)AUDIO_PLAYER_TIMEOUT_US*TICKS_PER_US+1;before=cacheCalls;audioThreadMain();CHECK(!aud.playerIp&&cacheCalls==before);reset();startPlayer(peerIp,peerPort);fc(102,103);aud.lastHelloTick=nowTick;audioThreadMain();CHECK(sent==1&&wireSize==1292&&status.audioFrames==2&&stopRequested);}
int main(void){
#ifdef _WIN32
CHECK(VirtualAlloc((void*)0x1ff50000,0x30000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)==(void*)0x1ff50000);
#else
CHECK(mmap((void*)0x1ff50000,0x30000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==(void*)0x1ff50000);
#endif
reader_tests();hello_contract();packets();threads();printf("%u\n",checks);return 0;}
'''
(OUT / 'fixture.c').write_text(definitions + actual_dsp + functions + suffix, encoding='utf8')
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
target = OUT / ('audio.exe' if os.name == 'nt' else 'audio')
command = [compiler, '-std=gnu11', '-O1', '-Wall', '-Wextra', '-Werror', '-Wno-pointer-to-int-cast', '-Wno-int-to-pointer-cast', '-Wno-unused-variable', str(OUT / 'fixture.c'), '-o', str(target)]
result = subprocess.run(command, env=environment, capture_output=True, text=True)
assert result.returncode == 0, result.stdout + result.stderr
result = subprocess.run([str(target)], env=environment, capture_output=True, text=True, timeout=10)
assert result.returncode == 0, result.stdout + result.stderr
report = dict(result='pass', checks=int(result.stdout.strip()), source_sha256=hashlib.sha256((ROS / 'source/remote_play.c').read_bytes()).hexdigest(), dsp_sha256=hashlib.sha256((ROS / 'source/dsp_audio.c').read_bytes()).hexdigest(), minisoc_sha256=hashlib.sha256((ROS / 'source/minisoc.c').read_bytes()).hexdigest(), hardware_tested=False, boundary='Actual minisoc sockaddr conversion, DSP reader and audio transport C; fixed DSP memory, cache service, clock, socket IPC and thread scheduler are host providers.', coverage=['native IPv4 address length8 with four-byte L2AH', 'short address, wrong family, zero peer and malformed hello rejection', 'L2AH through actual sockaddr conversion to reader init, DSP polling and L2AU UDP output', 'stereo PCM ordering', 'counter wrap', 'odd and large counter resync', 'post-copy tearing rejection', 'bounded hello and malformed peer', 'L2AU packet golden bytes', 'packet index wrap and gap', 'duplicate and stale frame rejection', 'short send and transient errors', 'sleep and network loss', 'peer timeout', 'audio thread DSP-to-packet path'])
(OUT / 'proof.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf8')
print(json.dumps(report))
