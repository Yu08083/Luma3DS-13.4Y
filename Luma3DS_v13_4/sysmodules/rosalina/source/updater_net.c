#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "minisoc.h"
#include "updater_net.h"
#include "updater_tls.h"
#include "updater_clock.h"

typedef struct Connection {
    int socket;
    const UpdateHooks *hooks;
    uint64_t deadline,idle;
    int failure;
    UpdateTls tls;
} Connection;

static UpdateNetDiagnostics diagnostics;
static int64_t clockUtc;
static uint64_t clockTick;
static bool clockReady;

static uint64_t milliseconds(void)
{
    return svcGetSystemTick()/268123;
}

static bool cancelled(const UpdateHooks *hooks)
{
    return hooks&&hooks->cancel&&hooks->cancel(hooks->context);
}

static int ready(Connection *connection,short events)
{
    if(cancelled(connection->hooks)){connection->failure=UPDATE_CANCELLED;return -1;}
    uint64_t now=milliseconds();
    if(now>=connection->deadline||now>=connection->idle){connection->failure=UPDATE_ERR_NETWORK;return -1;}
    struct pollfd descriptor={.fd=connection->socket,.events=events};
    int result=socPoll(&descriptor,1,0);
    if(result<0||(descriptor.revents&(POLLERR|POLLNVAL))){connection->failure=UPDATE_ERR_NETWORK;return -1;}
    if(descriptor.revents&events)return 1;
    if(descriptor.revents&POLLHUP)
    {
        if(events==POLLIN)return 1;
        connection->failure=UPDATE_ERR_NETWORK;
        return -1;
    }
    return 0;
}

static int sendBytes(void *context,const unsigned char *data,size_t size)
{
    Connection *connection=context;
    int available=ready(connection,POLLOUT);
    if(available<0)return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    if(!available)return MBEDTLS_ERR_SSL_WANT_WRITE;
    int result=(int)socSend(connection->socket,data,size,0);
    if(result==-EAGAIN||result==-EWOULDBLOCK||result==-EINTR)return MBEDTLS_ERR_SSL_WANT_WRITE;
    if(result<=0){connection->failure=UPDATE_ERR_NETWORK;return MBEDTLS_ERR_SSL_INTERNAL_ERROR;}
    connection->idle=milliseconds()+15000;
    return result;
}

static int receiveBytes(void *context,unsigned char *data,size_t size)
{
    Connection *connection=context;
    int available=ready(connection,POLLIN);
    if(available<0)return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    if(!available)return MBEDTLS_ERR_SSL_WANT_READ;
    int result=(int)socRecv(connection->socket,data,size,0);
    if(result==-EAGAIN||result==-EWOULDBLOCK||result==-EINTR)return MBEDTLS_ERR_SSL_WANT_READ;
    if(result<0){connection->failure=UPDATE_ERR_NETWORK;return MBEDTLS_ERR_SSL_INTERNAL_ERROR;}
    if(result)connection->idle=milliseconds()+15000;
    return result;
}

static int randomBytes(void *context,unsigned char *data,size_t size)
{
    Connection *connection=context;
    if(cancelled(connection->hooks)){connection->failure=UPDATE_CANCELLED;return -1;}
    Result result=PS_GenerateRandomBytes(data,size);
    if(R_FAILED(result))diagnostics.serviceResult=(uint32_t)result;
    return R_SUCCEEDED(result)?0:-1;
}

static int64_t elapsedMilliseconds(void *context)
{
    (void)context;
    return (int64_t)milliseconds();
}

static int64_t utcSeconds(void *context)
{
    (void)context;
    if(clockReady)return clockUtc+(int64_t)((milliseconds()-clockTick)/1000);
    uint64_t seconds=osGetTime()/1000;
    return seconds>=2208988800ULL?(int64_t)(seconds-2208988800ULL):-1;
}

static int readBody(void *context,void *data,size_t size)
{
    Connection *connection=context;
    for(;;)
    {
        if(ready(connection,POLLIN)<0)return connection->failure;
        int result=UpdateTls_Read(&connection->tls,data,size);
        if(result==UPDATE_TLS_ERROR)return connection->failure?connection->failure:UPDATE_ERR_TLS;
        if(result!=UPDATE_TLS_AGAIN)return result;
        svcSleepThread(10000000);
    }
}

static int connectSocket(Connection *connection,const char *host)
{
    uint32_t addresses[4];
    int count=socResolveIPv4(host,addresses);
    if(cancelled(connection->hooks))return UPDATE_CANCELLED;
    if(count<=0)return UPDATE_ERR_NETWORK;
    for(int index=0;index<count;++index)
    {
        connection->socket=socSocket(AF_INET,SOCK_STREAM,0);
        if(connection->socket<0)return UPDATE_ERR_NETWORK;
        if(socSetNonBlocking(connection->socket)<0)
        {
            socClose(connection->socket);
            connection->socket=-1;
            return UPDATE_ERR_NETWORK;
        }
        struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(443)};
        address.sin_addr.s_addr=addresses[index];
        int result=socConnectAsync(connection->socket,(struct sockaddr *)&address,sizeof(address));
        if(result==0||result==-EISCONN)return UPDATE_OK;
        if(result==-EINPROGRESS||result==-EALREADY||result==-EWOULDBLOCK)
        {
            connection->idle=milliseconds()+15000;
            for(;;)
            {
                int available=ready(connection,POLLOUT);
                if(available==1)return UPDATE_OK;
                if(available<0)break;
                svcSleepThread(10000000);
            }
        }
        socClose(connection->socket);
        connection->socket=-1;
        if(cancelled(connection->hooks))return UPDATE_CANCELLED;
        if(milliseconds()>=connection->deadline)return UPDATE_ERR_NETWORK;
        connection->failure=0;
    }
    return UPDATE_ERR_NETWORK;
}

static int request(Connection *connection,const char *url,uint32_t limit,UpdateHttpSink sink,void *context,UpdateHttpResponse *response,bool clock)
{
    const char *path=strchr(url+8,'/');
    if(!path)return UPDATE_ERR_URL;
    size_t hostSize=(size_t)(path-(url+8));
    char host[64];
    if(hostSize>=sizeof(host))return UPDATE_ERR_URL;
    memcpy(host,url+8,hostSize);
    host[hostSize]=0;
    if(connection->hooks&&connection->hooks->progress)
        connection->hooks->progress(connection->hooks->context,clock?UPDATE_STAGE_CLOCK:UPDATE_STAGE_CONNECT,0,0);
    int result=connectSocket(connection,host);
    if(result)return result;
    UpdateTlsIo io={.context=connection,.send=sendBytes,.receive=receiveBytes,.random=randomBytes,.time=utcSeconds,.milliseconds=elapsedMilliseconds};
    if(clock?UpdateTls_InitClock(&connection->tls,&io):UpdateTls_Init(&connection->tls,host,&io))return UPDATE_ERR_TLS;
    for(;;)
    {
        result=UpdateTls_Handshake(&connection->tls);
        if(result==1)break;
        if(result<0)return connection->failure?connection->failure:UPDATE_ERR_TLS;
        if(ready(connection,POLLIN)<0)return connection->failure;
        svcSleepThread(10000000);
    }
    char headers[UPDATE_MAX_URL+256];
    int bytes=snprintf(headers,sizeof(headers),"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Luma3DS-13.4Y\r\nAccept: application/vnd.github+json\r\nAccept-Encoding: identity\r\nCache-Control: no-cache, no-store\r\nConnection: close\r\n\r\n",path,host);
    if(bytes<0||(size_t)bytes>=sizeof(headers))return UPDATE_ERR_URL;
    size_t sent=0;
    while(sent<(size_t)bytes)
    {
        result=UpdateTls_Write(&connection->tls,(const unsigned char *)headers+sent,(size_t)bytes-sent);
        if(result==UPDATE_TLS_ERROR)return connection->failure?connection->failure:UPDATE_ERR_TLS;
        if(result==UPDATE_TLS_AGAIN)
        {
            if(ready(connection,POLLOUT)<0)return connection->failure;
            svcSleepThread(10000000);
        }
        else if(result<=0)return UPDATE_ERR_NETWORK;
        else sent+=(size_t)result;
    }
    UpdateHttpIo http={.context=connection,.read=readBody};
    return UpdateHttp_Read(&http,limit,sink,context,response);
}

static void capture(Connection *connection,const UpdateHttpResponse *response)
{
    diagnostics.tlsError=connection->tls.error;
    diagnostics.verifyFlags=connection->tls.verifyFlags;
    diagnostics.httpStatus=response->status;
    diagnostics.utc=utcSeconds(NULL);
    diagnostics.clockAdjusted=clockReady;
}

static void closeConnection(Connection *connection)
{
    UpdateTls_Close(&connection->tls);
    if(connection->socket>=0)socClose(connection->socket);
    connection->socket=-1;
}

static int discard(void *context,const void *data,uint32_t size)
{
    (void)context;(void)data;(void)size;
    return UPDATE_OK;
}

static int synchronizeClock(Connection *connection)
{
    unsigned char nonce[8];
    if(randomBytes(connection,nonce,sizeof(nonce)))return connection->failure?connection->failure:UPDATE_ERR_TLS;
    char url[80];
    static const char hex[]="0123456789abcdef";
    strcpy(url,"https://api.github.com/?luma-clock=");
    size_t offset=strlen(url);
    for(unsigned i=0;i<sizeof(nonce);++i)
    {
        url[offset++]=hex[nonce[i]>>4];
        url[offset++]=hex[nonce[i]&15];
    }
    url[offset]=0;
    connection->failure=0;
    connection->idle=milliseconds()+15000;
    UpdateHttpResponse response={0};
    int result=request(connection,url,32768,discard,NULL,&response,true);
    int64_t utc=0;
    if(!result&&(response.status!=200||(response.hasAge&&response.age)||!UpdateClock_ParseHttpDate(response.date,&utc)||!UpdateTls_ConfirmClock(&connection->tls,utc)))result=UPDATE_ERR_CLOCK;
    capture(connection,&response);
    closeConnection(connection);
    if(result)return result==UPDATE_ERR_TLS||result==UPDATE_ERR_HTTP?UPDATE_ERR_CLOCK:result;
    clockUtc=utc;
    clockTick=milliseconds();
    clockReady=true;
    diagnostics.clockAdjusted=true;
    diagnostics.utc=utc;
    return UPDATE_OK;
}

void UpdateNet_Diagnostics(UpdateNetDiagnostics *out)
{
    if(out)*out=diagnostics;
}

int UpdateNet_Get(const char *initial,uint32_t limit,UpdateHttpSink sink,void *context,const UpdateHooks *hooks)
{
    memset(&diagnostics,0,sizeof(diagnostics));
    if(!Update_TrustedUrl(initial)||!sink)return UPDATE_ERR_URL;
    if(cancelled(hooks))return UPDATE_CANCELLED;
    Result service=miniSocInit();
    if(R_FAILED(service)){diagnostics.serviceResult=(uint32_t)service;return UPDATE_ERR_NETWORK;}
    service=psInit();
    if(R_FAILED(service)){diagnostics.serviceResult=(uint32_t)service;miniSocExit();return UPDATE_ERR_TLS;}
    char url[UPDATE_MAX_URL];
    strcpy(url,initial);
    Connection connection={.socket=-1,.hooks=hooks,.deadline=milliseconds()+120000};
    int result=UPDATE_ERR_HTTP;
    unsigned redirect=0;
    bool triedClock=false;
    for(;;)
    {
        connection.failure=0;
        connection.idle=milliseconds()+15000;
        UpdateHttpResponse response={0};
        result=request(&connection,url,limit,sink,context,&response,false);
        bool adjust=result==UPDATE_ERR_TLS&&!triedClock&&UpdateTls_DateFailure(&connection.tls);
        capture(&connection,&response);
        closeConnection(&connection);
        if(adjust)
        {
            triedClock=true;
            result=synchronizeClock(&connection);
            if(result)break;
            continue;
        }
        if(result||response.status==200)break;
        if(redirect==5){result=UPDATE_ERR_URL;break;}
        ++redirect;
        if(response.location[0]=='/')
        {
            if(response.location[1]=='/'){result=UPDATE_ERR_URL;break;}
            const char *path=strchr(url+8,'/');
            size_t prefix=(size_t)(path-url);
            size_t suffix=strlen(response.location);
            if(prefix+suffix>=sizeof(url)){result=UPDATE_ERR_URL;break;}
            memcpy(url+prefix,response.location,suffix+1);
        }
        else
        {
            if(!Update_TrustedUrl(response.location)){result=UPDATE_ERR_URL;break;}
            strcpy(url,response.location);
        }
        if(!Update_TrustedUrl(url)){result=UPDATE_ERR_URL;break;}
    }
    psExit();
    miniSocExit();
    return result;
}
