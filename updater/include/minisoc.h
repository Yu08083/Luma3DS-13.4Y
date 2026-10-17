#pragma once
#include <3ds.h>
#include <poll.h>
#include <sys/socket.h>

Result miniSocInit(void);
Result miniSocExit(void);
int socResolveIPv4(const char *,u32[4]);
int socSocket(int,int,int);
int socSetNonBlocking(int);
int socConnectAsync(int,const struct sockaddr *,socklen_t);
int socPoll(struct pollfd *,nfds_t,int);
int socClose(int);
ssize_t socSend(int,const void *,size_t,int);
ssize_t socRecv(int,void *,size_t,int);
