#include <3ds.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "minisoc.h"

static void *socketPool;
static bool socketsReady;

Result miniSocInit(void)
{
    if(socketsReady)return 0;
    socketPool=memalign(4096,1024*1024);
    if(!socketPool)return -1;
    Result result=socInit(socketPool,1024*1024);
    if(R_FAILED(result))
    {
        free(socketPool);
        socketPool=NULL;
        return result;
    }
    socketsReady=true;
    return 0;
}

Result miniSocExit(void)
{
    Result result=0;
    if(socketsReady)
    {
        result=socExit();
        socketsReady=false;
    }
    free(socketPool);
    socketPool=NULL;
    return result;
}

int socResolveIPv4(const char *hostname,u32 addresses[4])
{
    if(!socketsReady||!hostname||!addresses)return -ENETDOWN;
    struct hostent *entry=gethostbyname(hostname);
    if(!entry||entry->h_addrtype!=AF_INET||entry->h_length!=4||!entry->h_addr_list)return -ENOENT;
    unsigned used=0;
    for(unsigned index=0;index<16&&entry->h_addr_list[index]&&used<4;++index)
    {
        u32 value;
        memcpy(&value,entry->h_addr_list[index],4);
        if(!value||value==0xFFFFFFFF)continue;
        bool duplicate=false;
        for(unsigned k=0;k<used;++k)if(addresses[k]==value)duplicate=true;
        if(!duplicate)addresses[used++]=value;
    }
    return used?(int)used:-ENOENT;
}

int socSocket(int domain,int type,int protocol)
{
    return socket(domain,type,protocol);
}

int socSetNonBlocking(int socket)
{
    int flags=fcntl(socket,F_GETFL,0);
    if(flags<0)return -errno;
    int result=fcntl(socket,F_SETFL,flags|O_NONBLOCK);
    return result<0?-errno:result;
}

int socConnectAsync(int socket,const struct sockaddr *address,socklen_t size)
{
    int result=connect(socket,address,size);
    return result<0?-errno:result;
}

int socPoll(struct pollfd *descriptors,nfds_t count,int timeout)
{
    return poll(descriptors,count,timeout);
}

int socClose(int socket)
{
    return closesocket(socket);
}

ssize_t socSend(int socket,const void *data,size_t size,int flags)
{
    return send(socket,data,size,flags);
}

ssize_t socRecv(int socket,void *data,size_t size,int flags)
{
    return recv(socket,data,size,flags);
}
