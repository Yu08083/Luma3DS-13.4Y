                                  
  
                                                                 
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_RANDOM_IMPL_H
#define PSA_CRYPTO_RANDOM_IMPL_H

#include "psa_util_internal.h"

#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)

typedef mbedtls_psa_external_random_context_t mbedtls_psa_random_context_t;

#else                                      

#include "mbedtls/entropy.h"

                                                           
#if defined(MBEDTLS_CTR_DRBG_C)

#include "mbedtls/ctr_drbg.h"
#undef MBEDTLS_PSA_HMAC_DRBG_MD_TYPE

#elif defined(MBEDTLS_HMAC_DRBG_C)

#include "mbedtls/hmac_drbg.h"
#if defined(MBEDTLS_MD_CAN_SHA512) && defined(MBEDTLS_MD_CAN_SHA256)
#include <limits.h>
#if SIZE_MAX > 0xffffffff
                                                    
#define MBEDTLS_PSA_HMAC_DRBG_MD_TYPE MBEDTLS_MD_SHA512
#else
                                                    
#define MBEDTLS_PSA_HMAC_DRBG_MD_TYPE MBEDTLS_MD_SHA256
#endif
#elif defined(MBEDTLS_MD_CAN_SHA512)
#define MBEDTLS_PSA_HMAC_DRBG_MD_TYPE MBEDTLS_MD_SHA512
#elif defined(MBEDTLS_MD_CAN_SHA256)
#define MBEDTLS_PSA_HMAC_DRBG_MD_TYPE MBEDTLS_MD_SHA256
#else
#error "No hash algorithm available for HMAC_DBRG."
#endif

#else                                                 

#error "No DRBG module available for the psa_crypto module."

#endif                                                 

                                                                                      
#if defined(MBEDTLS_CTR_DRBG_C)
#define MBEDTLS_PSA_RANDOM_MAX_REQUEST MBEDTLS_CTR_DRBG_MAX_REQUEST
#elif defined(MBEDTLS_HMAC_DRBG_C)
#define MBEDTLS_PSA_RANDOM_MAX_REQUEST MBEDTLS_HMAC_DRBG_MAX_REQUEST
#endif

#if defined(MBEDTLS_CTR_DRBG_C)
typedef mbedtls_ctr_drbg_context            mbedtls_psa_drbg_context_t;
#elif defined(MBEDTLS_HMAC_DRBG_C)
typedef mbedtls_hmac_drbg_context           mbedtls_psa_drbg_context_t;
#endif                                                  

typedef struct {
    void (* entropy_init)(mbedtls_entropy_context *ctx);
    void (* entropy_free)(mbedtls_entropy_context *ctx);
    mbedtls_entropy_context entropy;
    mbedtls_psa_drbg_context_t drbg;
#if defined(MBEDTLS_PLATFORM_IS_UNIXLIKE)
                                                                      
                                                                   
                                                                     
                                                               
      
                                                                
                                                                    
                                                                    
                                                                    
                                                                    
                                                                 
                                                                   
                                                                      
       
    intmax_t pid;
#endif
} mbedtls_psa_random_context_t;

                            
  
                                                          
   
static inline void mbedtls_psa_drbg_init(mbedtls_psa_drbg_context_t *p_rng)
{
#if defined(MBEDTLS_CTR_DRBG_C)
    mbedtls_ctr_drbg_init(p_rng);
#elif defined(MBEDTLS_HMAC_DRBG_C)
    mbedtls_hmac_drbg_init(p_rng);
#endif
}

                              
  
                                                          
   
static inline void mbedtls_psa_drbg_free(mbedtls_psa_drbg_context_t *p_rng)
{
#if defined(MBEDTLS_CTR_DRBG_C)
    mbedtls_ctr_drbg_free(p_rng);
#elif defined(MBEDTLS_HMAC_DRBG_C)
    mbedtls_hmac_drbg_free(p_rng);
#endif
}

                      
  
                                                 
                                                              
                                                                 
                                                   
                                                                              
                                                                          
                                                                 
  
                                        
                                                                               
   
static inline int mbedtls_psa_drbg_seed(mbedtls_psa_drbg_context_t *drbg_ctx,
                                        mbedtls_entropy_context *entropy,
                                        const unsigned char *custom, size_t len)
{
#if defined(MBEDTLS_CTR_DRBG_C)
    return mbedtls_ctr_drbg_seed(drbg_ctx, mbedtls_entropy_func, entropy, custom, len);
#elif defined(MBEDTLS_HMAC_DRBG_C)
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_PSA_HMAC_DRBG_MD_TYPE);
    return mbedtls_hmac_drbg_seed(drbg_ctx, md_info, mbedtls_entropy_func, entropy, custom, len);
#endif
}

                        
  
                                                   
                                          
                                                  
                                                             
                                                                               
  
                                        
                                                                               
   
static inline int mbedtls_psa_drbg_reseed(mbedtls_psa_drbg_context_t *drbg_ctx,
                                          const unsigned char *additional,
                                          size_t len)
{
#if defined(MBEDTLS_CTR_DRBG_C)
    return mbedtls_ctr_drbg_reseed(drbg_ctx, additional, len);
#elif defined(MBEDTLS_HMAC_DRBG_C)
    return mbedtls_hmac_drbg_reseed(drbg_ctx, additional, len);
#endif
}

                                                                           
  
                                          
  
                                                    
                                          
   
static inline void mbedtls_psa_drbg_deplete(mbedtls_psa_drbg_context_t *drbg_ctx)
{
    drbg_ctx->reseed_counter = drbg_ctx->reseed_interval;
}

#if MBEDTLS_ENTROPY_TRUE_SOURCES > 0
                                              
  
                                          
  
                                                        
                                          
                                                           
   
static inline void mbedtls_psa_drbg_set_prediction_resistance(
    mbedtls_psa_drbg_context_t *drbg_ctx,
    unsigned enabled)
{
#if defined(MBEDTLS_CTR_DRBG_C)
    mbedtls_ctr_drbg_set_prediction_resistance(drbg_ctx, enabled);
#elif defined(MBEDTLS_HMAC_DRBG_C)
    mbedtls_hmac_drbg_set_prediction_resistance(drbg_ctx, enabled);
#endif
}
#endif                                       

#endif                                      

#endif                               
