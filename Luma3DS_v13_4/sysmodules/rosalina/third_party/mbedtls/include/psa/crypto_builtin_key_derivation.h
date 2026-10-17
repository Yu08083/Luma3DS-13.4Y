  
                                                                            
                                                        
                                                                          
                                           
  
                                                                  
                        
  
                                                                         
                                                                        
                                                                           
                                                                              
                                                                
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_BUILTIN_KEY_DERIVATION_H
#define PSA_CRYPTO_BUILTIN_KEY_DERIVATION_H
#include "mbedtls/private_access.h"

#include <psa/crypto_driver_common.h>

#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXPAND)
typedef struct {
    uint8_t *MBEDTLS_PRIVATE(info);
    size_t MBEDTLS_PRIVATE(info_length);
#if PSA_HASH_MAX_SIZE > 0xff
#error "PSA_HASH_MAX_SIZE does not fit in uint8_t"
#endif
    uint8_t MBEDTLS_PRIVATE(offset_in_block);
    uint8_t MBEDTLS_PRIVATE(block_number);
    unsigned int MBEDTLS_PRIVATE(state) : 2;
    unsigned int MBEDTLS_PRIVATE(info_set) : 1;
    uint8_t MBEDTLS_PRIVATE(output_block)[PSA_HASH_MAX_SIZE];
    uint8_t MBEDTLS_PRIVATE(prk)[PSA_HASH_MAX_SIZE];
    struct psa_mac_operation_s MBEDTLS_PRIVATE(hmac);
} psa_hkdf_key_derivation_t;
#endif                                  \
                                                \
                                                
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS)
typedef struct {
    uint8_t MBEDTLS_PRIVATE(data)[PSA_TLS12_ECJPAKE_TO_PMS_DATA_SIZE];
} psa_tls12_ecjpake_to_pms_t;
#endif                                                   

#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
typedef enum {
    PSA_TLS12_PRF_STATE_INIT,                                    
    PSA_TLS12_PRF_STATE_SEED_SET,                                
    PSA_TLS12_PRF_STATE_OTHER_KEY_SET,                                           
    PSA_TLS12_PRF_STATE_KEY_SET,                                
    PSA_TLS12_PRF_STATE_LABEL_SET,                                
    PSA_TLS12_PRF_STATE_OUTPUT                                         
} psa_tls12_prf_key_derivation_state_t;

typedef struct psa_tls12_prf_key_derivation_s {
#if PSA_HASH_MAX_SIZE > 0xff
#error "PSA_HASH_MAX_SIZE does not fit in uint8_t"
#endif

                                                              
                                        
    uint8_t MBEDTLS_PRIVATE(left_in_block);

                                          
    uint8_t MBEDTLS_PRIVATE(block_number);

    psa_tls12_prf_key_derivation_state_t MBEDTLS_PRIVATE(state);

    uint8_t *MBEDTLS_PRIVATE(secret);
    size_t MBEDTLS_PRIVATE(secret_length);
    uint8_t *MBEDTLS_PRIVATE(seed);
    size_t MBEDTLS_PRIVATE(seed_length);
    uint8_t *MBEDTLS_PRIVATE(label);
    size_t MBEDTLS_PRIVATE(label_length);
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
    uint8_t *MBEDTLS_PRIVATE(other_secret);
    size_t MBEDTLS_PRIVATE(other_secret_length);
#endif                                              

    uint8_t MBEDTLS_PRIVATE(Ai)[PSA_HASH_MAX_SIZE];

                                                                                 
    uint8_t MBEDTLS_PRIVATE(output_block)[PSA_HASH_MAX_SIZE];
} psa_tls12_prf_key_derivation_t;
#endif                                        \
                                                    
#if defined(PSA_HAVE_SOFT_PBKDF2)
typedef enum {
    PSA_PBKDF2_STATE_INIT,                                    
    PSA_PBKDF2_STATE_INPUT_COST_SET,                                
    PSA_PBKDF2_STATE_SALT_SET,                                
    PSA_PBKDF2_STATE_PASSWORD_SET,                                
    PSA_PBKDF2_STATE_OUTPUT                                         
} psa_pbkdf2_key_derivation_state_t;

typedef struct {
    psa_pbkdf2_key_derivation_state_t MBEDTLS_PRIVATE(state);
    uint64_t MBEDTLS_PRIVATE(input_cost);
    uint8_t *MBEDTLS_PRIVATE(salt);
    size_t MBEDTLS_PRIVATE(salt_length);
    uint8_t MBEDTLS_PRIVATE(password)[PSA_HMAC_MAX_HASH_BLOCK_SIZE];
    size_t MBEDTLS_PRIVATE(password_length);
    uint8_t MBEDTLS_PRIVATE(output_block)[PSA_HASH_MAX_SIZE];
    uint8_t MBEDTLS_PRIVATE(bytes_used);
    uint32_t MBEDTLS_PRIVATE(block_number);
} psa_pbkdf2_key_derivation_t;
#endif                           

#endif                                          
