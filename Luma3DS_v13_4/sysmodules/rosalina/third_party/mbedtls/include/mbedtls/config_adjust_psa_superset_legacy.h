   
                                                    
                                                                    
  
                                                          
  
                                                                             
                                                                               
                                                                         
                                                                           
                                                                      
                                       
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_CONFIG_ADJUST_PSA_SUPERSET_LEGACY_H
#define MBEDTLS_CONFIG_ADJUST_PSA_SUPERSET_LEGACY_H

#if !defined(MBEDTLS_CONFIG_FILES_READ)
#error "Do not include mbedtls/config_adjust_*.h manually! This can lead to problems, " \
    "up to and including runtime errors such as buffer overflows. " \
    "If you're trying to fix a complaint from check_config.h, just remove " \
    "it from your configuration file: since Mbed TLS 3.0, it is included " \
    "automatically at the right point."
#endif      

                                                                  
                                                    
                                                     
                                                   
                                                                  

#if defined(MBEDTLS_MD5_C)
#define PSA_WANT_ALG_MD5 1
#endif

#if defined(MBEDTLS_RIPEMD160_C)
#define PSA_WANT_ALG_RIPEMD160 1
#endif

#if defined(MBEDTLS_SHA1_C)
#define PSA_WANT_ALG_SHA_1 1
#endif

#if defined(MBEDTLS_SHA224_C)
#define PSA_WANT_ALG_SHA_224 1
#endif

#if defined(MBEDTLS_SHA256_C)
#define PSA_WANT_ALG_SHA_256 1
#endif

#if defined(MBEDTLS_SHA384_C)
#define PSA_WANT_ALG_SHA_384 1
#endif

#if defined(MBEDTLS_SHA512_C)
#define PSA_WANT_ALG_SHA_512 1
#endif

#if defined(MBEDTLS_SHA3_C)
#define PSA_WANT_ALG_SHA3_224 1
#define PSA_WANT_ALG_SHA3_256 1
#define PSA_WANT_ALG_SHA3_384 1
#define PSA_WANT_ALG_SHA3_512 1
#endif

                                                                         
                                                        
#if defined(MBEDTLS_ECP_DP_BP256R1_ENABLED)
#if !defined(PSA_WANT_ECC_BRAINPOOL_P_R1_256)
#define PSA_WANT_ECC_BRAINPOOL_P_R1_256 1
#endif                                      
#endif                                     

#if defined(MBEDTLS_ECP_DP_BP384R1_ENABLED)
#if !defined(PSA_WANT_ECC_BRAINPOOL_P_R1_384)
#define PSA_WANT_ECC_BRAINPOOL_P_R1_384 1
#endif                                      
#endif                                     

#if defined(MBEDTLS_ECP_DP_BP512R1_ENABLED)
#if !defined(PSA_WANT_ECC_BRAINPOOL_P_R1_512)
#define PSA_WANT_ECC_BRAINPOOL_P_R1_512 1
#endif                                      
#endif                                     

#if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
#if !defined(PSA_WANT_ECC_MONTGOMERY_255)
#define PSA_WANT_ECC_MONTGOMERY_255 1
#endif                                  
#endif                                        

#if defined(MBEDTLS_ECP_DP_CURVE448_ENABLED)
#if !defined(PSA_WANT_ECC_MONTGOMERY_448)
#define PSA_WANT_ECC_MONTGOMERY_448 1
#endif                                  
#endif                                      

#if defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED)
#if !defined(PSA_WANT_ECC_SECP_R1_192)
#define PSA_WANT_ECC_SECP_R1_192 1
#endif                               
#endif                                       

#if defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED)
#if !defined(PSA_WANT_ECC_SECP_R1_224)
#define PSA_WANT_ECC_SECP_R1_224 1
#endif                               
#endif                                       

#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
#if !defined(PSA_WANT_ECC_SECP_R1_256)
#define PSA_WANT_ECC_SECP_R1_256 1
#endif                               
#endif                                       

#if defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED)
#if !defined(PSA_WANT_ECC_SECP_R1_384)
#define PSA_WANT_ECC_SECP_R1_384 1
#endif                               
#endif                                       

#if defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED)
#if !defined(PSA_WANT_ECC_SECP_R1_521)
#define PSA_WANT_ECC_SECP_R1_521 1
#endif                               
#endif                                       

#if defined(MBEDTLS_ECP_DP_SECP192K1_ENABLED)
#if !defined(PSA_WANT_ECC_SECP_K1_192)
#define PSA_WANT_ECC_SECP_K1_192 1
#endif                               
#endif                                       

#if defined(MBEDTLS_ECP_DP_SECP256K1_ENABLED)
#if !defined(PSA_WANT_ECC_SECP_K1_256)
#define PSA_WANT_ECC_SECP_K1_256 1
#endif                               
#endif                                       

#endif                                                  
