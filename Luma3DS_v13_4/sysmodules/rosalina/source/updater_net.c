#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "minisoc.h"
#include "updater_net.h"
#include "updater_tls.h"

typedef struct Connection {
    int socket;
    const UpdateHooks *hooks;
    uint64_t deadline,idle;
    int failure;
    UpdateTls tls;
} Connection;

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
    if(result<0){connection->failure=UPDATE_ERR_NETWORK;return MBEDTLS_ERR_SSL_INTERNAL_ERROR;}
    if(result)connection->idle=milliseconds()+15000;
    return result;
}

static int randomBytes(void *context,unsigned char *data,size_t size)
{
    Connection *connection=context;
    if(cancelled(connection->hooks)){connection->failure=UPDATE_CANCELLED;return -1;}
    return R_SUCCEEDED(PS_GenerateRandomBytes(data,size))?0:-1;
}

static int64_t elapsedMilliseconds(void *context)
{
    (void)context;
    return (int64_t)milliseconds();
}

static int64_t utcSeconds(void *context)
{
    (void)context;
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

static int request(Connection *connection,const char *url,uint32_t limit,UpdateHttpSink sink,void *context,UpdateHttpResponse *response)
{
    const char *path=strchr(url+8,'/');
    if(!path)return UPDATE_ERR_URL;
    size_t hostSize=(size_t)(path-(url+8));
    char host[64];
    if(hostSize>=sizeof(host))return UPDATE_ERR_URL;
    memcpy(host,url+8,hostSize);
    host[hostSize]=0;
    if(connection->hooks&&connection->hooks->progress)
        connection->hooks->progress(connection->hooks->context,UPDATE_STAGE_CONNECT,0,0);
    int result=connectSocket(connection,host);
    if(result)return result;
    UpdateTlsIo io={.context=connection,.send=sendBytes,.receive=receiveBytes,.random=randomBytes,.time=utcSeconds,.milliseconds=elapsedMilliseconds};
    if(UpdateTls_Init(&connection->tls,host,&io))return UPDATE_ERR_TLS;
    for(;;)
    {
        result=UpdateTls_Handshake(&connection->tls);
        if(result==1)break;
        if(result<0)return connection->failure?connection->failure:UPDATE_ERR_TLS;
        if(ready(connection,POLLIN)<0)return connection->failure;
        svcSleepThread(10000000);
    }
    char headers[UPDATE_MAX_URL+256];
    int bytes=snprintf(headers,sizeof(headers),"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Luma3DS-13.4Y\r\nAccept: application/vnd.github+json\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n",path,host);
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

int UpdateNet_Get(const char *initial,uint32_t limit,UpdateHttpSink sink,void *context,const UpdateHooks *hooks)
{
    if(!Update_TrustedUrl(initial)||!sink)return UPDATE_ERR_URL;
    if(cancelled(hooks))return UPDATE_CANCELLED;
    if(R_FAILED(miniSocInit()))return UPDATE_ERR_NETWORK;
    if(R_FAILED(psInit())){miniSocExit();return UPDATE_ERR_TLS;}
    char url[UPDATE_MAX_URL];
    strcpy(url,initial);
    Connection connection={.socket=-1,.hooks=hooks,.deadline=milliseconds()+120000};
    int result=UPDATE_ERR_HTTP;
    for(unsigned redirect=0;redirect<=5;++redirect)
    {
        connection.failure=0;
        connection.idle=milliseconds()+15000;
        UpdateHttpResponse response;
        result=request(&connection,url,limit,sink,context,&response);
        UpdateTls_Close(&connection.tls);
        if(connection.socket>=0)socClose(connection.socket);
        connection.socket=-1;
        if(result||response.status==200)break;
        if(redirect==5){result=UPDATE_ERR_URL;break;}
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
