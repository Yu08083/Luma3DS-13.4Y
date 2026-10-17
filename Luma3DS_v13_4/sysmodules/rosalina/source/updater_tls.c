#include "updater_tls.h"
#include "updater_ca.h"
#include "updater_clock.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/platform.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/sha256.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static unsigned char tlsPool[192*1024] __attribute__((aligned(16)));
static bool tlsPoolBusy;
static UpdateTls *tlsClock;
static mbedtls_time_t (*tlsPreviousTime)(mbedtls_time_t *);

mbedtls_ms_time_t mbedtls_ms_time(void) {
    return tlsClock && tlsClock->io.milliseconds ? tlsClock->io.milliseconds(tlsClock->io.context) : 0;
}

static mbedtls_time_t tlsTime(mbedtls_time_t *out) {
    int64_t seconds=tlsClock && tlsClock->io.time ? tlsClock->io.time(tlsClock->io.context) : -1;
    mbedtls_time_t result=(seconds<0 || (sizeof(mbedtls_time_t)==4 && seconds>INT32_MAX))?(mbedtls_time_t)-1:(mbedtls_time_t)seconds;
    if(out) *out=result;
    return result;
}

static int tlsSend(void *context,const unsigned char *data,size_t size) {
    UpdateTls *tls=context;
    return tls->io.send(tls->io.context,data,size);
}

static int tlsReceive(void *context,unsigned char *data,size_t size) {
    UpdateTls *tls=context;
    return tls->io.receive(tls->io.context,data,size);
}

static int tlsRandom(void *context,unsigned char *data,size_t size) {
    UpdateTls *tls=context;
    return tls->io.random(tls->io.context,data,size);
}

static int clockVerify(void *context,mbedtls_x509_crt *certificate,int depth,uint32_t *flags) {
    static const unsigned char pin[]={0x4B,0x62,0xD4,0x21,0xBA,0xB8,0xC9,0x48,0x39,0xC3,0xE3,0x18,0x6E,0x3E,0x4B,0x64,0xE5,0x80,0x64,0x0C,0xDA,0x78,0xD1,0x89,0xF6,0xB4,0xD3,0x73,0x81,0xA3,0xBC,0x14};
    const uint32_t dates=MBEDTLS_X509_BADCERT_EXPIRED|MBEDTLS_X509_BADCERT_FUTURE;
    UpdateTls *tls=context;
    if(!certificate||depth<0||depth>=16||(tls->clockSeen&(1u<<depth))) {
        tls->clockInvalid=true;
        *flags|=MBEDTLS_X509_BADCERT_OTHER;
        return 0;
    }
    tls->clockSeen|=1u<<depth;
    int64_t first=UpdateClock_FromDate(certificate->valid_from.year,certificate->valid_from.mon,certificate->valid_from.day,certificate->valid_from.hour,certificate->valid_from.min,certificate->valid_from.sec);
    int64_t last=UpdateClock_FromDate(certificate->valid_to.year,certificate->valid_to.mon,certificate->valid_to.day,certificate->valid_to.hour,certificate->valid_to.min,certificate->valid_to.sec);
    if(first<0||last<first||(*flags&~dates))tls->clockInvalid=true;
    if(first>tls->clockFrom)tls->clockFrom=first;
    if(last<tls->clockUntil)tls->clockUntil=last;
    if(depth==0) {
        unsigned char digest[32];
        unsigned different=0;
        if(!certificate->pk_raw.p||!certificate->pk_raw.len||mbedtls_sha256(certificate->pk_raw.p,certificate->pk_raw.len,digest,0))tls->clockInvalid=true;
        else {
            for(unsigned index=0;index<sizeof(pin);++index)different|=digest[index]^pin[index];
            tls->clockPinned=different==0;
        }
        mbedtls_platform_zeroize(digest,sizeof(digest));
        if(!tls->clockPinned)tls->clockInvalid=true;
    }
    *flags&=~dates;
    if(tls->clockInvalid)*flags|=MBEDTLS_X509_BADCERT_OTHER;
    return 0;
}

static int tlsInit(UpdateTls *tls,const char *hostname,const UpdateTlsIo *io,bool clockMode) {
    if(!tls || !hostname || !*hostname || !io || !io->send || !io->receive || !io->random || !io->time || !io->milliseconds || tlsPoolBusy) return UPDATE_TLS_ERROR;
    memset(tls,0,sizeof(*tls));
    tlsPoolBusy=true;
    tls->ownsPool=true;
    tls->clockMode=clockMode;
    tls->clockUntil=INT64_MAX;
    tls->io=*io;
    tlsClock=tls;
    tlsPreviousTime=mbedtls_time;
    mbedtls_platform_set_time(tlsTime);
    mbedtls_memory_buffer_alloc_init(tlsPool,sizeof(tlsPool));
    mbedtls_ssl_init(&tls->ssl);
    mbedtls_ssl_config_init(&tls->config);
    mbedtls_x509_crt_init(&tls->roots);
    int result=0;
    for(unsigned int i=0;i<2 && !result;i++)
        result=mbedtls_x509_crt_parse_der(&tls->roots,updateCaRoots[i].der,updateCaRoots[i].bytes);
    if(!result) result=mbedtls_ssl_config_defaults(&tls->config,MBEDTLS_SSL_IS_CLIENT,MBEDTLS_SSL_TRANSPORT_STREAM,MBEDTLS_SSL_PRESET_DEFAULT);
    if(!result) {
        static const int suites[]={
            MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
            MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
            MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
            MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,0
        };
        mbedtls_ssl_conf_ciphersuites(&tls->config,suites);
        mbedtls_ssl_conf_min_tls_version(&tls->config,MBEDTLS_SSL_VERSION_TLS1_2);
        mbedtls_ssl_conf_max_tls_version(&tls->config,MBEDTLS_SSL_VERSION_TLS1_2);
        mbedtls_ssl_conf_authmode(&tls->config,MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(&tls->config,&tls->roots,NULL);
        mbedtls_ssl_conf_rng(&tls->config,tlsRandom,tls);
        if(clockMode)mbedtls_ssl_conf_verify(&tls->config,clockVerify,tls);
        result=mbedtls_ssl_setup(&tls->ssl,&tls->config);
    }
    if(!result) result=mbedtls_ssl_set_hostname(&tls->ssl,hostname);
    if(!result) mbedtls_ssl_set_bio(&tls->ssl,tls,tlsSend,tlsReceive,NULL);
    tls->error=result;
    return result ? UPDATE_TLS_ERROR : 0;
}

int UpdateTls_Init(UpdateTls *tls,const char *hostname,const UpdateTlsIo *io) {
    return tlsInit(tls,hostname,io,false);
}

int UpdateTls_InitClock(UpdateTls *tls,const UpdateTlsIo *io) {
    return tlsInit(tls,"api.github.com",io,true);
}

bool UpdateTls_ConfirmClock(UpdateTls *tls,int64_t seconds) {
    return tls&&tls->ownsPool&&tls->connected&&!tls->error&&!tls->verifyFlags&&tls->clockMode&&!tls->clockInvalid&&tls->clockPinned&&tls->clockSeen&&!(tls->clockSeen&(tls->clockSeen+1))&&seconds>=UPDATE_CLOCK_MIN_SECONDS&&seconds>=tls->clockFrom&&seconds<=tls->clockUntil;
}

bool UpdateTls_DateFailure(const UpdateTls *tls) {
    const uint32_t dates=MBEDTLS_X509_BADCERT_EXPIRED|MBEDTLS_X509_BADCERT_FUTURE;
    return tls&&tls->ownsPool&&!tls->clockMode&&!tls->connected&&tls->error==MBEDTLS_ERR_X509_CERT_VERIFY_FAILED&&tls->verifyFlags&&!(tls->verifyFlags&~dates);
}

int UpdateTls_Handshake(UpdateTls *tls) {
    if(!tls || !tls->ownsPool || tls->error) return UPDATE_TLS_ERROR;
    int result=mbedtls_ssl_handshake(&tls->ssl);
    tls->verifyFlags=mbedtls_ssl_get_verify_result(&tls->ssl);
    if(result==MBEDTLS_ERR_SSL_WANT_READ || result==MBEDTLS_ERR_SSL_WANT_WRITE) return 0;
    if(!result && (mbedtls_ssl_get_verify_result(&tls->ssl) || !mbedtls_ssl_get_peer_cert(&tls->ssl))) result=MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
    if(!result&&tls->clockMode&&(!tls->clockPinned||tls->clockInvalid||!tls->clockSeen||(tls->clockSeen&(tls->clockSeen+1))||tls->clockFrom>tls->clockUntil))result=MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
    if(result) {tls->error=result;return UPDATE_TLS_ERROR;}
    tls->connected=true;
    return 1;
}

static int tlsIoResult(UpdateTls *tls,int result) {
    if(result==MBEDTLS_ERR_SSL_WANT_READ || result==MBEDTLS_ERR_SSL_WANT_WRITE) return UPDATE_TLS_AGAIN;
    if(result==MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) return 0;
    if(result<0) {tls->error=result;return UPDATE_TLS_ERROR;}
    return result;
}

int UpdateTls_Read(UpdateTls *tls,unsigned char *data,size_t size) {
    if(!tls || !tls->connected || tls->error || !data || !size || size>INT_MAX) return UPDATE_TLS_ERROR;
    return tlsIoResult(tls,mbedtls_ssl_read(&tls->ssl,data,size));
}

int UpdateTls_Write(UpdateTls *tls,const unsigned char *data,size_t size) {
    if(!tls || !tls->connected || tls->error || !data || !size || size>INT_MAX) return UPDATE_TLS_ERROR;
    return tlsIoResult(tls,mbedtls_ssl_write(&tls->ssl,data,size));
}

void UpdateTls_Close(UpdateTls *tls) {
    if(!tls || !tls->ownsPool) return;
    if(tls->connected && !tls->error) mbedtls_ssl_close_notify(&tls->ssl);
    mbedtls_ssl_free(&tls->ssl);
    mbedtls_ssl_config_free(&tls->config);
    mbedtls_x509_crt_free(&tls->roots);
    mbedtls_memory_buffer_alloc_free();
    mbedtls_platform_set_calloc_free(calloc,free);
    mbedtls_platform_set_time(tlsPreviousTime);
    mbedtls_platform_zeroize(tlsPool,sizeof(tlsPool));
    mbedtls_platform_zeroize(tls,sizeof(*tls));
    tlsClock=NULL;
    tlsPoolBusy=false;
}
