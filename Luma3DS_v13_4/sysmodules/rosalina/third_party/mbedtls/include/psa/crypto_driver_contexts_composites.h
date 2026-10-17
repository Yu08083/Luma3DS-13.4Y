  
                                                                         
                                                                        
                                                                                
                                                             
  
                                                            
  
                                                                  
                        
  
                                                                         
                                                                        
                                                                            
                                                                             
                                                                              
   
/*  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_DRIVER_CONTEXTS_COMPOSITES_H
#define PSA_CRYPTO_DRIVER_CONTEXTS_COMPOSITES_H

#include "psa/crypto_driver_common.h"

                                                                                 
#include "psa/crypto_builtin_composites.h"

                                                                        
                                                 

#if defined(MBEDTLS_TEST_LIBTESTDRIVER1)
#include <libtestdriver1/include/psa/crypto.h>
#endif

#if defined(PSA_CRYPTO_DRIVER_TEST)
#if defined(MBEDTLS_TEST_LIBTESTDRIVER1) && \
    defined(LIBTESTDRIVER1_MBEDTLS_PSA_BUILTIN_MAC)
typedef libtestdriver1_mbedtls_psa_mac_operation_t
    mbedtls_transparent_test_driver_mac_operation_t;
typedef libtestdriver1_mbedtls_psa_mac_operation_t
    mbedtls_opaque_test_driver_mac_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_MAC_OPERATION_INIT \
    LIBTESTDRIVER1_MBEDTLS_PSA_MAC_OPERATION_INIT
#define MBEDTLS_OPAQUE_TEST_DRIVER_MAC_OPERATION_INIT \
    LIBTESTDRIVER1_MBEDTLS_PSA_MAC_OPERATION_INIT

#else
typedef mbedtls_psa_mac_operation_t
    mbedtls_transparent_test_driver_mac_operation_t;
typedef mbedtls_psa_mac_operation_t
    mbedtls_opaque_test_driver_mac_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_MAC_OPERATION_INIT \
    MBEDTLS_PSA_MAC_OPERATION_INIT
#define MBEDTLS_OPAQUE_TEST_DRIVER_MAC_OPERATION_INIT \
    MBEDTLS_PSA_MAC_OPERATION_INIT

#endif                                                                            

#if defined(MBEDTLS_TEST_LIBTESTDRIVER1) && \
    defined(LIBTESTDRIVER1_MBEDTLS_PSA_BUILTIN_AEAD)
typedef libtestdriver1_mbedtls_psa_aead_operation_t
    mbedtls_transparent_test_driver_aead_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_AEAD_OPERATION_INIT \
    LIBTESTDRIVER1_MBEDTLS_PSA_AEAD_OPERATION_INIT
#else
typedef mbedtls_psa_aead_operation_t
    mbedtls_transparent_test_driver_aead_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_AEAD_OPERATION_INIT \
    MBEDTLS_PSA_AEAD_OPERATION_INIT

#endif                                                                             

#if defined(MBEDTLS_TEST_LIBTESTDRIVER1) && \
    defined(LIBTESTDRIVER1_MBEDTLS_PSA_BUILTIN_PAKE)

typedef libtestdriver1_mbedtls_psa_pake_operation_t
    mbedtls_transparent_test_driver_pake_operation_t;
typedef libtestdriver1_mbedtls_psa_pake_operation_t
    mbedtls_opaque_test_driver_pake_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_PAKE_OPERATION_INIT \
    LIBTESTDRIVER1_MBEDTLS_PSA_PAKE_OPERATION_INIT
#define MBEDTLS_OPAQUE_TEST_DRIVER_PAKE_OPERATION_INIT \
    LIBTESTDRIVER1_MBEDTLS_PSA_PAKE_OPERATION_INIT

#else
typedef mbedtls_psa_pake_operation_t
    mbedtls_transparent_test_driver_pake_operation_t;
typedef mbedtls_psa_pake_operation_t
    mbedtls_opaque_test_driver_pake_operation_t;

#define MBEDTLS_TRANSPARENT_TEST_DRIVER_PAKE_OPERATION_INIT \
    MBEDTLS_PSA_PAKE_OPERATION_INIT
#define MBEDTLS_OPAQUE_TEST_DRIVER_PAKE_OPERATION_INIT \
    MBEDTLS_PSA_PAKE_OPERATION_INIT

#endif                                                                             

#endif                             

                                                                              
                                                                           
  
                                                                              
                                                                             
                                                                       

typedef union {
    unsigned dummy;                                               
    mbedtls_psa_mac_operation_t mbedtls_ctx;
#if defined(PSA_CRYPTO_DRIVER_TEST)
    mbedtls_transparent_test_driver_mac_operation_t transparent_test_driver_ctx;
    mbedtls_opaque_test_driver_mac_operation_t opaque_test_driver_ctx;
#endif
} psa_driver_mac_context_t;

typedef union {
    unsigned dummy;                                               
    mbedtls_psa_aead_operation_t mbedtls_ctx;
#if defined(PSA_CRYPTO_DRIVER_TEST)
    mbedtls_transparent_test_driver_aead_operation_t transparent_test_driver_ctx;
#endif
} psa_driver_aead_context_t;

typedef union {
    unsigned dummy;                                               
    mbedtls_psa_sign_hash_interruptible_operation_t mbedtls_ctx;
} psa_driver_sign_hash_interruptible_context_t;

typedef union {
    unsigned dummy;                                               
    mbedtls_psa_verify_hash_interruptible_operation_t mbedtls_ctx;
} psa_driver_verify_hash_interruptible_context_t;

typedef union {
    unsigned dummy;                                               
    mbedtls_psa_pake_operation_t mbedtls_ctx;
#if defined(PSA_CRYPTO_DRIVER_TEST)
    mbedtls_transparent_test_driver_pake_operation_t transparent_test_driver_ctx;
    mbedtls_opaque_test_driver_pake_operation_t opaque_test_driver_ctx;
#endif
} psa_driver_pake_context_t;

#endif                                              
                                          
