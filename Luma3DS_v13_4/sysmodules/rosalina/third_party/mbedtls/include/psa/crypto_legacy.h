   
                            
  
                                                                       
                                   
  
                                                                             
                          
                                                                              
                                                                        
                                                                         
                      
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_PSA_CRYPTO_LEGACY_H
#define MBEDTLS_PSA_CRYPTO_LEGACY_H

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR)                 
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC      1
#endif
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT   1
#endif
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT   1
#endif
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE 1
#endif
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE   1
#endif
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR)                 
#if !defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC      1
#endif
#if !defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT   1
#endif
#if !defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT   1
#endif
#if !defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE 1
#endif
#endif

#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR)                 
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_DERIVE
#endif
#endif

#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR)                 
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_BASIC
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_IMPORT)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_IMPORT
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_EXPORT)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_EXPORT
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_GENERATE
#endif
#endif

#endif                                  
