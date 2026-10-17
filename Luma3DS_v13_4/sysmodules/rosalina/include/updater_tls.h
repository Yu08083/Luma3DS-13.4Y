#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
typedef struct UpdateTlsIo {
    void *context;
    int (*send)(void *,const unsigned char *,size_t);
    int (*receive)(void *,unsigned char *,size_t);
    int (*random)(void *,unsigned char *,size_t);
    int64_t (*time)(void *);
    int64_t (*milliseconds)(void *);
} UpdateTlsIo;
typedef struct UpdateTls {
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_x509_crt roots;
    UpdateTlsIo io;
    int error;
    uint32_t verifyFlags,clockSeen;
    int64_t clockFrom,clockUntil;
    bool ownsPool,connected,clockMode,clockPinned,clockInvalid;
} UpdateTls;
int UpdateTls_Init(UpdateTls *tls,const char *hostname,const UpdateTlsIo *io);
int UpdateTls_InitClock(UpdateTls *tls,const UpdateTlsIo *io);
bool UpdateTls_ConfirmClock(UpdateTls *tls,int64_t seconds);
bool UpdateTls_DateFailure(const UpdateTls *tls);
int UpdateTls_Handshake(UpdateTls *tls);
int UpdateTls_Read(UpdateTls *tls,unsigned char *data,size_t size);
int UpdateTls_Write(UpdateTls *tls,const unsigned char *data,size_t size);
void UpdateTls_Close(UpdateTls *tls);
#define UPDATE_TLS_AGAIN (-1)
#define UPDATE_TLS_ERROR (-2)
