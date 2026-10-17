from pathlib import Path
import hashlib
import json
import os
import subprocess

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-tests/net-fixture'

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    for folder in ('netinet','arpa'):(OUT/folder).mkdir(exist_ok=True)
    files={
    '3ds.h':r"""#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef uint32_t u32;
typedef int32_t Result;
#define R_FAILED(r) ((r)<0)
#define R_SUCCEEDED(r) ((r)>=0)
uint64_t svcGetSystemTick(void);
void svcSleepThread(int64_t);
Result psInit(void);
void psExit(void);
Result PS_GenerateRandomBytes(void *,size_t);
uint64_t osGetTime(void);
""",
    'netinet/in.h':r"""#pragma once
#include <stdint.h>
typedef unsigned socklen_t;
struct in_addr {uint32_t s_addr;};
struct sockaddr {uint16_t sa_family;char sa_data[14];};
struct sockaddr_in {uint16_t sin_family,sin_port;struct in_addr sin_addr;char padding[8];};
#define AF_INET 2
#define SOCK_STREAM 1
""",
    'arpa/inet.h':r"""#pragma once
#include <stdint.h>
uint16_t htons(uint16_t);
""",
    'minisoc.h':r"""#pragma once
#include "3ds.h"
#include "netinet/in.h"
struct pollfd {int fd;short events,revents;};
#define POLLIN 1
#define POLLOUT 4
#define POLLERR 8
#define POLLHUP 16
#define POLLNVAL 32
int miniSocInit(void);
int miniSocExit(void);
int socResolveIPv4(const char *,u32[4]);
int socSocket(int,int,int);
int socSetNonBlocking(int);
int socConnectAsync(int,const struct sockaddr *,socklen_t);
int socPoll(struct pollfd *,unsigned,int);
int socSend(int,const void *,size_t,int);
int socRecv(int,void *,size_t,int);
int socClose(int);
""",
    'updater_tls.h':r"""#pragma once
#include <stdint.h>
#include <stddef.h>
#define MBEDTLS_ERR_SSL_INTERNAL_ERROR -100
#define MBEDTLS_ERR_SSL_WANT_WRITE -101
#define MBEDTLS_ERR_SSL_WANT_READ -102
#define UPDATE_TLS_ERROR -2
#define UPDATE_TLS_AGAIN -1
typedef struct UpdateTlsIo {
void *context;
int (*send)(void *,const unsigned char *,size_t);
int (*receive)(void *,unsigned char *,size_t);
int (*random)(void *,unsigned char *,size_t);
int64_t (*time)(void *);
int64_t (*milliseconds)(void *);
} UpdateTlsIo;
typedef struct UpdateTls {UpdateTlsIo io;int open,error;uint32_t verifyFlags;bool clock;} UpdateTls;
int UpdateTls_Init(UpdateTls *,const char *,const UpdateTlsIo *);
int UpdateTls_Handshake(UpdateTls *);
int UpdateTls_Read(UpdateTls *,unsigned char *,size_t);
int UpdateTls_Write(UpdateTls *,const unsigned char *,size_t);
void UpdateTls_Close(UpdateTls *);
int UpdateTls_InitClock(UpdateTls *,const UpdateTlsIo *);
bool UpdateTls_ConfirmClock(UpdateTls *,int64_t);
bool UpdateTls_DateFailure(const UpdateTls *);
""",
    'fixture.c':r"""#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "minisoc.h"
#include "updater_tls.h"
#include "updater_net.h"
#include NET_SOURCE
static unsigned checks;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(1);}}while(0)
static const char *responses[8];
static unsigned requests,responseOffset,received,closed,initCount,exitCount,psCount,psExitCount,tlsCloseCount,tlsOpenCount;
static unsigned cancelledAfter,operations,mode;
static uint64_t ticks;
static char requestText[8192],hostname[64],output[65536];
static unsigned requestUsed;
static void reset(void){
memset(responses,0,sizeof(responses));requests=responseOffset=received=closed=initCount=exitCount=psCount=psExitCount=tlsCloseCount=tlsOpenCount=cancelledAfter=operations=mode=requestUsed=0;ticks=0;requestText[0]=output[0]=hostname[0]=0;
clockReady=false;clockUtc=0;clockTick=0;
}
uint64_t svcGetSystemTick(void){ticks+=268123;return ticks;}
void svcSleepThread(int64_t n){ticks+=(uint64_t)n*268123/1000000;}
int psInit(void){++psCount;return mode==2?-1:0;}
void psExit(void){++psExitCount;}
int PS_GenerateRandomBytes(void *p,size_t n){memset(p,1,n);return mode==13||mode==25?-1:0;}
uint64_t osGetTime(void){return 3990000000000ULL;}
uint16_t htons(uint16_t x){return (uint16_t)(x>>8|x<<8);}
int miniSocInit(void){++initCount;return mode==1?-1:0;}
int miniSocExit(void){++exitCount;return 0;}
int socResolveIPv4(const char *name,u32 addresses[4]){++operations;strcpy(hostname,name);addresses[0]=0x0100007F;return mode==3?-1:1;}
int socSocket(int d,int t,int p){CHECK(d==AF_INET&&t==SOCK_STREAM&&p==0);if(mode==4)return -1;++requests;responseOffset=0;return (int)requests+100;}
int socSetNonBlocking(int fd){CHECK(fd>=100);return mode==5?-1:0;}
int socConnectAsync(int fd,const struct sockaddr *s,socklen_t n){const struct sockaddr_in *a=(const void *)s;CHECK(fd>=100&&n==16&&a->sin_family==AF_INET&&a->sin_port==htons(443));return mode==6?-ECONNREFUSED:mode==14?0:-EINPROGRESS;}
int socPoll(struct pollfd *fds,unsigned n,int timeout){CHECK(n==1&&timeout==0);++operations;fds[0].revents=mode==7?POLLERR:mode==8?0:fds[0].events;return mode==7?1:mode==8?0:1;}
int socSend(int fd,const void *data,size_t n,int flags){CHECK(fd>=100&&flags==0);if(mode==9)return -1;if(requestUsed+n>=sizeof(requestText))return -1;memcpy(requestText+requestUsed,data,n);requestUsed+=(unsigned)n;requestText[requestUsed]=0;return (int)n;}
int socRecv(int fd,void *data,size_t n,int flags){CHECK(fd>=100&&flags==0);if(mode==10)return -1;const char *s=responses[requests-1];if(!s)return 0;size_t total=strlen(s);if(responseOffset>=total)return 0;size_t count=total-responseOffset;if(count>n)count=n;if(count>37)count=37;memcpy(data,s+responseOffset,count);responseOffset+=(unsigned)count;return (int)count;}
int socClose(int fd){CHECK(fd>=100);++closed;return 0;}
int UpdateTls_Init(UpdateTls *t,const char *name,const UpdateTlsIo *io){CHECK(!strcmp(name,hostname));memset(t,0,sizeof(*t));t->io=*io;t->open=1;++tlsOpenCount;return mode==11?-2:0;}
int UpdateTls_InitClock(UpdateTls *t,const UpdateTlsIo *io){CHECK(!strcmp(hostname,"api.github.com"));int result=UpdateTls_Init(t,"api.github.com",io);t->clock=true;return result;}
bool UpdateTls_DateFailure(const UpdateTls *t){return t->verifyFlags==1;}
bool UpdateTls_ConfirmClock(UpdateTls *t,int64_t utc){CHECK(t->clock);return utc>=1792249200LL&&utc<1795823999LL;}
int UpdateTls_Handshake(UpdateTls *t){
if(mode>=16&&!t->clock&&(!clockReady||mode==22)){t->error=-9984;t->verifyFlags=mode==18?8:1;return -2;}
if(t->clock&&mode==17){t->error=-9984;t->verifyFlags=8;return -2;}
if(t->clock&&mode==23)cancelledAfter=operations;
unsigned char b[16];if(t->io.random(t->io.context,b,sizeof(b)))return -2;CHECK(t->io.time(t->io.context)>0);return mode==12?-2:1;}
int UpdateTls_Write(UpdateTls *t,const unsigned char *p,size_t n){int x=t->io.send(t->io.context,p,n);return x==MBEDTLS_ERR_SSL_WANT_WRITE?-1:x<0?-2:x;}
int UpdateTls_Read(UpdateTls *t,unsigned char *p,size_t n){int x=t->io.receive(t->io.context,p,n);return x==MBEDTLS_ERR_SSL_WANT_READ?-1:x<0?-2:x;}
void UpdateTls_Close(UpdateTls *t){if(t->open){++tlsCloseCount;t->open=0;}}
static int sink(void *context,const void *data,uint32_t n){(void)context;if(mode==15)return UPDATE_ERR_SD;CHECK(received+n<=sizeof(output));memcpy(output+received,data,n);received+=n;return 0;}
static bool cancel(void *context){(void)context;return cancelledAfter&&operations>=cancelledAfter;}
static UpdateHooks hooks={.cancel=cancel};
static void balance(void){CHECK(exitCount==(mode==1?0:1));CHECK(psExitCount==((mode==1||mode==2)?0:1));CHECK(closed==(mode==4?0:requests));CHECK(tlsOpenCount==tlsCloseCount);}
static int get(uint32_t limit){return UpdateNet_Get("https://api.github.com/repos/Yu08083/Luma3DS-13.4Y/releases/latest",limit,sink,0,&hooks);}
int main(void){
reset();responses[0]="HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";CHECK(get(5)==0);CHECK(received==5&&!memcmp(output,"hello",5));CHECK(strstr(requestText,"GET /repos/Yu08083/Luma3DS-13.4Y/releases/latest HTTP/1.1\r\nHost: api.github.com\r\n"));balance();
for(unsigned m=1;m<=15;++m){reset();mode=m;responses[0]="HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";int result=get(5);CHECK(m==14?result==0:result<0);balance();}
reset();responses[0]="HTTP/1.1 302 Found\r\nLocation: https://github.com/a\r\n\r\n";responses[1]="HTTP/1.1 302 Found\r\nLocation: https://release-assets.githubusercontent.com/b?key=value\r\n\r\n";responses[2]="HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n";CHECK(get(5)==0&&requests==3&&received==5);balance();
reset();responses[0]="HTTP/1.1 307 Redirect\r\nLocation: /same-host\r\n\r\n";responses[1]="HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";CHECK(get(5)==0&&requests==2);CHECK(strstr(requestText,"GET /same-host HTTP/1.1"));balance();
const char *bad[]={"http://github.com/a","https://evil.example/a","//evil.example/a","https://github.com.evil.example/a","https://github.com@evil.example/a","https://127.0.0.1/a","https://github.com:443/a"};
for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){char wire[300];snprintf(wire,sizeof(wire),"HTTP/1.1 302 Found\r\nLocation: %s\r\n\r\n",bad[i]);reset();responses[0]=wire;CHECK(get(5)==UPDATE_ERR_URL&&requests==1&&received==0);balance();}
reset();for(unsigned i=0;i<8;++i)responses[i]="HTTP/1.1 302 Found\r\nLocation: https://github.com/a\r\n\r\n";CHECK(get(5)==UPDATE_ERR_URL&&requests==6);balance();
for(unsigned at=1;at<7;++at){reset();responses[0]="HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";cancelledAfter=at;int value=get(5);CHECK(value==UPDATE_CANCELLED);balance();}
reset();responses[0]="HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nhello!";CHECK(get(5)==UPDATE_ERR_SIZE);balance();
reset();CHECK(UpdateNet_Get("http://github.com/a",5,sink,0,&hooks)==UPDATE_ERR_URL&&initCount==0);
for(unsigned m=16;m<=25;++m){
reset();mode=m;
const char *date=m==19?"Sat, 31 Feb 2026 16:00:00 GMT":"Sat, 17 Oct 2026 16:00:00 GMT";
char clockResponse[512];
snprintf(clockResponse,sizeof(clockResponse),"HTTP/1.1 200 OK\r\n%s%s%s%sContent-Length: 2\r\n\r\n{}",m==24?"":"Date: ",m==24?"":date,m==24?"":"\r\n",m==20?"Age: 1\r\n":"Age: 0\r\n");
if(m==21)strcpy(clockResponse,"HTTP/1.1 302 Found\r\nLocation: https://github.com/evil\r\n\r\n");
responses[1]=clockResponse;responses[2]="HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
int result=get(5);
CHECK(result==(m==16?UPDATE_OK:m==18||m==22||m==25?UPDATE_ERR_TLS:m==23?UPDATE_CANCELLED:UPDATE_ERR_CLOCK));
CHECK(requests==(m==18||m==25?1:m==16||m==22?3:2));
CHECK(received==(m==16?5:0));
if(m==16){CHECK(clockReady);CHECK(strstr(requestText,"GET /?luma-clock=0101010101010101 HTTP/1.1"));CHECK(strstr(requestText,"Cache-Control: no-cache, no-store"));}
UpdateNetDiagnostics d;UpdateNet_Diagnostics(&d);CHECK(d.clockAdjusted==(m==16||m==22));
if(m==25)CHECK(d.serviceResult==UINT32_MAX);
balance();
}
printf("%u\n",checks);return 0;
}
"""
    }
    for name,value in files.items():(OUT/name).write_bytes(value.encode())
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    target=OUT/('fixture.exe' if os.name=='nt' else 'fixture')
    sources=[ROS/'source/updater_http.c',ROS/'source/updater_format.c',ROS/'source/updater_clock.c',OUT/'fixture.c']
    result=subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(OUT),'-I'+str(ROS/'include'),'-DNET_SOURCE="'+str(ROS/'source/updater_net.c').replace('\\','/')+'"',*map(str,sources),'-o',str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    result=subprocess.run([str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    checks=int(result.stdout.strip())
    report=dict(result='pass',checks=checks,source_sha256={x.name:hashlib.sha256(x.read_bytes()).hexdigest() for x in sources[:3]+[ROS/'source/updater_net.c']},hardware_tested=False,boundary='actual network/HTTP/URL/clock code; mock miniSoc and TLS engine; crypto verified separately by updater_tls.py')
    (OUT.parent/'net-proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))

if __name__=='__main__':main()
