   
                            
  
                                                           
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_PSA_UTIL_INTERNAL_H
#define MBEDTLS_PSA_UTIL_INTERNAL_H

                                                                    
#include "mbedtls/psa_util.h"

#include "psa/crypto.h"

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT)

                                                                          
       
                                                                          

#define MBEDTLS_PSA_MAX_FFDH_PUBKEY_LENGTH \
    PSA_KEY_EXPORT_FFDH_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_FFDH_MAX_KEY_BITS)

                                                                          
      
                                                                          

#define MBEDTLS_PSA_MAX_EC_PUBKEY_LENGTH \
    PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)

#define MBEDTLS_PSA_MAX_EC_KEY_PAIR_LENGTH \
    PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)

                                                                          
                    
                                                                          

typedef struct {
                                                                               
    int16_t psa_status;
                                                            
                                                                         
                                                     
    int16_t mbedtls_error;
} mbedtls_error_pair_t;

#if defined(MBEDTLS_MD_LIGHT)
extern const mbedtls_error_pair_t psa_to_md_errors[4];
#endif

#if defined(MBEDTLS_BLOCK_CIPHER_SOME_PSA)
extern const mbedtls_error_pair_t psa_to_cipher_errors[4];
#endif

#if defined(MBEDTLS_LMS_C)
extern const mbedtls_error_pair_t psa_to_lms_errors[3];
#endif

#if defined(MBEDTLS_USE_PSA_CRYPTO) || defined(MBEDTLS_SSL_PROTO_TLS1_3)
extern const mbedtls_error_pair_t psa_to_ssl_errors[7];
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY) ||    \
    defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC)
extern const mbedtls_error_pair_t psa_to_pk_rsa_errors[8];
#endif

#if defined(MBEDTLS_USE_PSA_CRYPTO) && \
    defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY)
extern const mbedtls_error_pair_t psa_to_pk_ecdsa_errors[7];
#endif

                                                   
                                                      
int psa_generic_status_to_mbedtls(psa_status_t status);

                                                                 
                                                                                
int psa_status_to_mbedtls(psa_status_t status,
                          const mbedtls_error_pair_t *local_translations,
                          size_t local_errors_num,
                          int (*fallback_f)(psa_status_t));

                                                                           
                                                                          
                                                              
int psa_pk_status_to_mbedtls(psa_status_t status);

                                                                          
#define PSA_TO_MBEDTLS_ERR_LIST(status, error_list, fallback_f)       \
    psa_status_to_mbedtls(status, error_list,                         \
                          sizeof(error_list)/sizeof(error_list[0]),   \
                          fallback_f)

#endif                                
#endif                                  
