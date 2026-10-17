  
                                                                         
                                                                           
               
  
                                                            
  
                                                                  
                        
  
                                                                         
                                                                        
                                                                            
                                                                             
                                                                              
   
/*  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_DRIVER_CONTEXTS_KEY_DERIVATION_H
#define PSA_CRYPTO_DRIVER_CONTEXTS_KEY_DERIVATION_H

#include "psa/crypto_driver_common.h"

                                                                                 
#include "psa/crypto_builtin_key_derivation.h"

                                                                        
                                                 

typedef union {
    unsigned dummy;                                               
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXPAND)
    psa_hkdf_key_derivation_t MBEDTLS_PRIVATE(hkdf);
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
    psa_tls12_prf_key_derivation_t MBEDTLS_PRIVATE(tls12_prf);
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS)
    psa_tls12_ecjpake_to_pms_t MBEDTLS_PRIVATE(tls12_ecjpake_to_pms);
#endif
#if defined(PSA_HAVE_SOFT_PBKDF2)
    psa_pbkdf2_key_derivation_t MBEDTLS_PRIVATE(pbkdf2);
#endif
} psa_driver_key_derivation_context_t;

#endif                                                  
                                          
