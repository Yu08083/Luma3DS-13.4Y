  
                                                                         
                                                                        
                                                                          
  
                                                            
  
                                                                  
                        
  
                                                                         
                                                                        
                                                                            
                                                                             
                                                                              
   
/*  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_DRIVER_CONTEXTS_PRIMITIVES_H
#define PSA_CRYPTO_DRIVER_CONTEXTS_PRIMITIVES_H

#include "psa/crypto_driver_common.h"

                                                                                 
#include "psa/crypto_builtin_primitives.h"

                                                                        
                                                 

#if defined(MBEDTLS_TEST_LIBTESTDRIVER1)
#include <libtestdriver1/include/psa/crypto.h>
#endif

#if defined(PSA_CRYPTO_DRIVER_TEST)

#if defined(MBEDTLS_TEST_LIBTESTDRIVER1) && \
    defined(LIBTESTDRIVER1_MBEDTLS_PSA_BUILTIN_CIPHER)
typedef libtestdriver1_mbedtls_psa_cipher_operation_t
    mbedtls_transparent_test_driver_cipher_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_CIPHER_OPERATION_INIT \
    LIBTESTDRIVER1_MBEDTLS_PSA_CIPHER_OPERATION_INIT
#else
typedef mbedtls_psa_cipher_operation_t
    mbedtls_transparent_test_driver_cipher_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_CIPHER_OPERATION_INIT \
    MBEDTLS_PSA_CIPHER_OPERATION_INIT
#endif                                 \
                                                      

#if defined(MBEDTLS_TEST_LIBTESTDRIVER1) && \
    defined(LIBTESTDRIVER1_MBEDTLS_PSA_BUILTIN_HASH)
typedef libtestdriver1_mbedtls_psa_hash_operation_t
    mbedtls_transparent_test_driver_hash_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_HASH_OPERATION_INIT \
    LIBTESTDRIVER1_MBEDTLS_PSA_HASH_OPERATION_INIT
#else
typedef mbedtls_psa_hash_operation_t
    mbedtls_transparent_test_driver_hash_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_HASH_OPERATION_INIT \
    MBEDTLS_PSA_HASH_OPERATION_INIT
#endif                                 \
                                                    

typedef struct {
    unsigned int initialised : 1;
    mbedtls_transparent_test_driver_cipher_operation_t ctx;
} mbedtls_opaque_test_driver_cipher_operation_t;

#define MBEDTLS_OPAQUE_TEST_DRIVER_CIPHER_OPERATION_INIT \
    { 0, MBEDTLS_TRANSPARENT_TEST_DRIVER_CIPHER_OPERATION_INIT }

#endif                             

                                                                              
                                                                           
  
                                                                              
                                                                             
                                                                       

typedef union {
    unsigned dummy;                                               
    mbedtls_psa_hash_operation_t mbedtls_ctx;
#if defined(PSA_CRYPTO_DRIVER_TEST)
    mbedtls_transparent_test_driver_hash_operation_t test_driver_ctx;
#endif
} psa_driver_hash_context_t;

typedef union {
    unsigned dummy;                                               
    mbedtls_psa_cipher_operation_t mbedtls_ctx;
#if defined(PSA_CRYPTO_DRIVER_TEST)
    mbedtls_transparent_test_driver_cipher_operation_t transparent_test_driver_ctx;
    mbedtls_opaque_test_driver_cipher_operation_t opaque_test_driver_ctx;
#endif
} psa_driver_cipher_context_t;

#endif                                              
                                          
