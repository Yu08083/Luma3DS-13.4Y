  
                                
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include "common.h"

#if defined(MBEDTLS_PSA_CRYPTO_C) && !defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)

#include "psa_crypto_core.h"
#include "psa_crypto_random.h"
#include "psa_crypto_random_impl.h"
#include "threading_internal.h"

#if defined(MBEDTLS_PSA_INJECT_ENTROPY)
#include "entropy_poll.h"
#endif

#if defined(MBEDTLS_PLATFORM_IS_UNIXLIKE)
                                       
#include <unistd.h>
#if defined(MBEDTLS_HAVE_TIME)
#include <mbedtls/platform_time.h>
#else
                                                                    
#include <sys/time.h>
#endif
#endif

void psa_random_internal_init(mbedtls_psa_random_context_t *rng)
{
                                   
                                                                            
    if (rng->entropy_init == NULL) {
        rng->entropy_init = mbedtls_entropy_init;
    }
    if (rng->entropy_free == NULL) {
        rng->entropy_free = mbedtls_entropy_free;
    }

    rng->entropy_init(&rng->entropy);
#if defined(MBEDTLS_PSA_INJECT_ENTROPY) && \
    defined(MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES)
                                                                               
                                                                             
    mbedtls_entropy_add_source(&rng->entropy,
                               mbedtls_nv_seed_poll, NULL,
                               MBEDTLS_ENTROPY_BLOCK_SIZE,
                               MBEDTLS_ENTROPY_SOURCE_STRONG);
#endif

    mbedtls_psa_drbg_init(&rng->drbg);
}

void psa_random_internal_free(mbedtls_psa_random_context_t *rng)
{
    mbedtls_psa_drbg_free(&rng->drbg);
    rng->entropy_free(&rng->entropy);
}
psa_status_t psa_random_internal_seed(mbedtls_psa_random_context_t *rng)
{
    const unsigned char drbg_seed[] = "PSA";
    int ret = mbedtls_psa_drbg_seed(&rng->drbg, &rng->entropy,
                                    drbg_seed, sizeof(drbg_seed) - 1);
#if defined(MBEDTLS_PLATFORM_IS_UNIXLIKE)
    rng->pid = getpid();
#endif
    return mbedtls_to_psa_error(ret);
}

#if defined(MBEDTLS_PLATFORM_IS_UNIXLIKE)
static psa_status_t psa_random_internal_reseed_child(
    mbedtls_psa_random_context_t *rng,
    intmax_t pid)
{
                                                                       
                                                             
                                         
      
                                                                  
                                                                       
                                                               
                                                               
                                                               
                                                                
                                                           

                                                                     
                                                                          
                                                                       
                                                             
                                                    
       
    struct {
                                                                 
                           
                                                                    
                                                                  
                                                                      
                                                                 
                                                                 
                           
           
        intmax_t pid;
                                                                  
                                                                     
#if defined(MBEDTLS_HAVE_TIME)
        mbedtls_ms_time_t now;
#else
        struct timeval now;
#endif
    } perso;
    memset(&perso, 0, sizeof(perso));
    perso.pid = pid;
#if defined(MBEDTLS_HAVE_TIME)
    perso.now = mbedtls_ms_time();
#else
                                                                      
                                                                         
                                                                          
                                                                       
                                                                           
                                                                              
    if (gettimeofday(&perso.now, NULL) == -1) {
        return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }
#endif
    int ret = mbedtls_psa_drbg_reseed(&rng->drbg,
                                      (unsigned char *) &perso, sizeof(perso));
    return mbedtls_to_psa_error(ret);
}
#endif                                   

psa_status_t psa_random_internal_generate(
    mbedtls_psa_random_context_t *rng,
    uint8_t *output, size_t output_size)
{
#if defined(MBEDTLS_PLATFORM_IS_UNIXLIKE)
    intmax_t pid = getpid();
    if (pid != rng->pid) {
                                                                 
                                                                         
#if defined(MBEDTLS_THREADING_C)
        mbedtls_mutex_lock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   
        psa_status_t status = psa_random_internal_reseed_child(rng, pid);
        if (status == PSA_SUCCESS) {
            rng->pid = pid;
        }
#if defined(MBEDTLS_THREADING_C)
        mbedtls_mutex_unlock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   
        if (status != PSA_SUCCESS) {
            return status;
        }
    }
#endif                                   

    while (output_size > 0) {
        size_t request_size =
            (output_size > MBEDTLS_PSA_RANDOM_MAX_REQUEST ?
             MBEDTLS_PSA_RANDOM_MAX_REQUEST :
             output_size);
#if defined(MBEDTLS_CTR_DRBG_C)
        int ret = mbedtls_ctr_drbg_random(&rng->drbg, output, request_size);
#elif defined(MBEDTLS_HMAC_DRBG_C)
        int ret = mbedtls_hmac_drbg_random(&rng->drbg, output, request_size);
#endif                                                  
        if (ret != 0) {
            return mbedtls_to_psa_error(ret);
        }
        output_size -= request_size;
        output += request_size;
    }
    return PSA_SUCCESS;
}

#endif                                                               
