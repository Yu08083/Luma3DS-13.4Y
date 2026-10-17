   
                            
  
                                                                           
  
                                                                  
                        
  
                                                                  
                                       
  
                                                                    
                                                                          
                                              
  
                                                             
  
                                                                             
                                                                             
                                                                              
                                                                      
                          
  
                                                                          
                                                                           
                                                                             
                                                                             
                                                                      
             
  
                                                                               
                                                                           
                                                                          
                                                                   
                                                                              
                               
  
                                                                      
                                                                        
                                                                       
                                                                         
                   
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_STRUCT_H
#define PSA_CRYPTO_STRUCT_H
#include "mbedtls/private_access.h"

#ifdef __cplusplus
extern "C" {
#endif

  
                                                                           
                                                                            
                                                                           
                                                                  
                 
   
#include "psa/build_info.h"

                                                                               
                 
#include "psa/crypto_driver_contexts_primitives.h"

struct psa_hash_operation_s {
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
    mbedtls_psa_client_handle_t handle;
#else
                                                                
                                                                     
                                                              
                                                             
                                                                      
                                                                     
    unsigned int MBEDTLS_PRIVATE(id);
    psa_driver_hash_context_t MBEDTLS_PRIVATE(ctx);
#endif
};
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
#define PSA_HASH_OPERATION_INIT { 0 }
#else
#define PSA_HASH_OPERATION_INIT { 0, { 0 } }
#endif
static inline struct psa_hash_operation_s psa_hash_operation_init(void)
{
    const struct psa_hash_operation_s v = PSA_HASH_OPERATION_INIT;
    return v;
}

struct psa_cipher_operation_s {
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
    mbedtls_psa_client_handle_t handle;
#else
                                                                
                                                                     
                                                              
                                                                   
                                                                      
                                                                   
    unsigned int MBEDTLS_PRIVATE(id);

    unsigned int MBEDTLS_PRIVATE(iv_required) : 1;
    unsigned int MBEDTLS_PRIVATE(iv_set) : 1;

    uint8_t MBEDTLS_PRIVATE(default_iv_length);

    psa_driver_cipher_context_t MBEDTLS_PRIVATE(ctx);
#endif
};

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
#define PSA_CIPHER_OPERATION_INIT { 0 }
#else
#define PSA_CIPHER_OPERATION_INIT { 0, 0, 0, 0, { 0 } }
#endif
static inline struct psa_cipher_operation_s psa_cipher_operation_init(void)
{
    const struct psa_cipher_operation_s v = PSA_CIPHER_OPERATION_INIT;
    return v;
}

                                                                               
                 
#include "psa/crypto_driver_contexts_composites.h"

struct psa_mac_operation_s {
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
    mbedtls_psa_client_handle_t handle;
#else
                                                                
                                                                     
                                                              
                                                            
                                                                      
                                                                   
    unsigned int MBEDTLS_PRIVATE(id);
    uint8_t MBEDTLS_PRIVATE(mac_size);
    unsigned int MBEDTLS_PRIVATE(is_sign) : 1;
    psa_driver_mac_context_t MBEDTLS_PRIVATE(ctx);
#endif
};

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
#define PSA_MAC_OPERATION_INIT { 0 }
#else
#define PSA_MAC_OPERATION_INIT { 0, 0, 0, { 0 } }
#endif
static inline struct psa_mac_operation_s psa_mac_operation_init(void)
{
    const struct psa_mac_operation_s v = PSA_MAC_OPERATION_INIT;
    return v;
}

struct psa_aead_operation_s {
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
    mbedtls_psa_client_handle_t handle;
#else
                                                                
                                                                     
                                                              
                                                                   
                                                                      
                                                                   
    unsigned int MBEDTLS_PRIVATE(id);

    psa_algorithm_t MBEDTLS_PRIVATE(alg);
    psa_key_type_t MBEDTLS_PRIVATE(key_type);

    size_t MBEDTLS_PRIVATE(ad_remaining);
    size_t MBEDTLS_PRIVATE(body_remaining);

    unsigned int MBEDTLS_PRIVATE(nonce_set) : 1;
    unsigned int MBEDTLS_PRIVATE(lengths_set) : 1;
    unsigned int MBEDTLS_PRIVATE(ad_started) : 1;
    unsigned int MBEDTLS_PRIVATE(body_started) : 1;
    unsigned int MBEDTLS_PRIVATE(is_encrypt) : 1;

    psa_driver_aead_context_t MBEDTLS_PRIVATE(ctx);
#endif
};

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
#define PSA_AEAD_OPERATION_INIT { 0 }
#else
#define PSA_AEAD_OPERATION_INIT { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0 } }
#endif
static inline struct psa_aead_operation_s psa_aead_operation_init(void)
{
    const struct psa_aead_operation_s v = PSA_AEAD_OPERATION_INIT;
    return v;
}

                                                                         
                            
#include "psa/crypto_driver_contexts_key_derivation.h"

struct psa_key_derivation_s {
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
    mbedtls_psa_client_handle_t handle;
#else
    psa_algorithm_t MBEDTLS_PRIVATE(alg);
    unsigned int MBEDTLS_PRIVATE(can_output_key) : 1;
    size_t MBEDTLS_PRIVATE(capacity);
    psa_driver_key_derivation_context_t MBEDTLS_PRIVATE(ctx);
#endif
};

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
#define PSA_KEY_DERIVATION_OPERATION_INIT { 0 }
#else
                                                                                
#define PSA_KEY_DERIVATION_OPERATION_INIT { 0, 0, 0, { 0 } }
#endif
static inline struct psa_key_derivation_s psa_key_derivation_operation_init(
    void)
{
    const struct psa_key_derivation_s v = PSA_KEY_DERIVATION_OPERATION_INIT;
    return v;
}

struct psa_custom_key_parameters_s {
                                                                 
    uint32_t flags;
};

                                                                           
  
                                                                              
                                                                             
                                                                              
                
   
#define PSA_CUSTOM_KEY_PARAMETERS_INIT { 0 }

#ifndef __cplusplus
                                                                    
                                                                    
                
                                                  
   
                                                                        
                                                                          
                                                                        
                                             
                                                
   
struct psa_key_production_parameters_s {
    uint32_t flags;
    uint8_t data[];
};

                                                                           
  
                                                                        
                                                       
                                             
                                                                
                
   
#define PSA_KEY_PRODUCTION_PARAMETERS_INIT { 0 }
#endif                   

struct psa_key_policy_s {
    psa_key_usage_t MBEDTLS_PRIVATE(usage);
    psa_algorithm_t MBEDTLS_PRIVATE(alg);
    psa_algorithm_t MBEDTLS_PRIVATE(alg2);
};
typedef struct psa_key_policy_s psa_key_policy_t;

#define PSA_KEY_POLICY_INIT { 0, 0, 0 }
static inline struct psa_key_policy_s psa_key_policy_init(void)
{
    const struct psa_key_policy_s v = PSA_KEY_POLICY_INIT;
    return v;
}

                                          
                                                                         
typedef uint16_t psa_key_bits_t;
                                                             
                                               
#define PSA_KEY_BITS_TOO_LARGE          ((psa_key_bits_t) -1)
                                     
                                                                         
                              
                                                                 
                   
#define PSA_MAX_KEY_BITS 0xfff8

struct psa_key_attributes_s {
#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
    psa_key_slot_number_t MBEDTLS_PRIVATE(slot_number);
    int MBEDTLS_PRIVATE(has_slot_number);
#endif                              
    psa_key_type_t MBEDTLS_PRIVATE(type);
    psa_key_bits_t MBEDTLS_PRIVATE(bits);
    psa_key_lifetime_t MBEDTLS_PRIVATE(lifetime);
    psa_key_policy_t MBEDTLS_PRIVATE(policy);
                                                                  
                                                                  
                                                                       
                                                                   
                                                                     
                                                                
                                                                     
                                                                    
                                                                  
             
       
    mbedtls_svc_key_id_t MBEDTLS_PRIVATE(id);
};

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
#define PSA_KEY_ATTRIBUTES_MAYBE_SLOT_NUMBER 0, 0,
#else
#define PSA_KEY_ATTRIBUTES_MAYBE_SLOT_NUMBER
#endif
#define PSA_KEY_ATTRIBUTES_INIT { PSA_KEY_ATTRIBUTES_MAYBE_SLOT_NUMBER \
                                      PSA_KEY_TYPE_NONE, 0,            \
                                      PSA_KEY_LIFETIME_VOLATILE,       \
                                      PSA_KEY_POLICY_INIT,             \
                                      MBEDTLS_SVC_KEY_ID_INIT }

static inline struct psa_key_attributes_s psa_key_attributes_init(void)
{
    const struct psa_key_attributes_s v = PSA_KEY_ATTRIBUTES_INIT;
    return v;
}

static inline void psa_set_key_id(psa_key_attributes_t *attributes,
                                  mbedtls_svc_key_id_t key)
{
    psa_key_lifetime_t lifetime = attributes->MBEDTLS_PRIVATE(lifetime);

    attributes->MBEDTLS_PRIVATE(id) = key;

    if (PSA_KEY_LIFETIME_IS_VOLATILE(lifetime)) {
        attributes->MBEDTLS_PRIVATE(lifetime) =
            PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
                PSA_KEY_LIFETIME_PERSISTENT,
                PSA_KEY_LIFETIME_GET_LOCATION(lifetime));
    }
}

static inline mbedtls_svc_key_id_t psa_get_key_id(
    const psa_key_attributes_t *attributes)
{
    return attributes->MBEDTLS_PRIVATE(id);
}

#ifdef MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER
static inline void mbedtls_set_key_owner_id(psa_key_attributes_t *attributes,
                                            mbedtls_key_owner_id_t owner)
{
    attributes->MBEDTLS_PRIVATE(id).MBEDTLS_PRIVATE(owner) = owner;
}
#endif

static inline void psa_set_key_lifetime(psa_key_attributes_t *attributes,
                                        psa_key_lifetime_t lifetime)
{
    attributes->MBEDTLS_PRIVATE(lifetime) = lifetime;
    if (PSA_KEY_LIFETIME_IS_VOLATILE(lifetime)) {
#ifdef MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER
        attributes->MBEDTLS_PRIVATE(id).MBEDTLS_PRIVATE(key_id) = 0;
#else
        attributes->MBEDTLS_PRIVATE(id) = 0;
#endif
    }
}

static inline psa_key_lifetime_t psa_get_key_lifetime(
    const psa_key_attributes_t *attributes)
{
    return attributes->MBEDTLS_PRIVATE(lifetime);
}

static inline void psa_extend_key_usage_flags(psa_key_usage_t *usage_flags)
{
    if (*usage_flags & PSA_KEY_USAGE_SIGN_HASH) {
        *usage_flags |= PSA_KEY_USAGE_SIGN_MESSAGE;
    }

    if (*usage_flags & PSA_KEY_USAGE_VERIFY_HASH) {
        *usage_flags |= PSA_KEY_USAGE_VERIFY_MESSAGE;
    }
}

static inline void psa_set_key_usage_flags(psa_key_attributes_t *attributes,
                                           psa_key_usage_t usage_flags)
{
    psa_extend_key_usage_flags(&usage_flags);
    attributes->MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(usage) = usage_flags;
}

static inline psa_key_usage_t psa_get_key_usage_flags(
    const psa_key_attributes_t *attributes)
{
    return attributes->MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(usage);
}

static inline void psa_set_key_algorithm(psa_key_attributes_t *attributes,
                                         psa_algorithm_t alg)
{
    attributes->MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(alg) = alg;
}

static inline psa_algorithm_t psa_get_key_algorithm(
    const psa_key_attributes_t *attributes)
{
    return attributes->MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(alg);
}

static inline void psa_set_key_type(psa_key_attributes_t *attributes,
                                    psa_key_type_t type)
{
    attributes->MBEDTLS_PRIVATE(type) = type;
}

static inline psa_key_type_t psa_get_key_type(
    const psa_key_attributes_t *attributes)
{
    return attributes->MBEDTLS_PRIVATE(type);
}

static inline void psa_set_key_bits(psa_key_attributes_t *attributes,
                                    size_t bits)
{
    if (bits > PSA_MAX_KEY_BITS) {
        attributes->MBEDTLS_PRIVATE(bits) = PSA_KEY_BITS_TOO_LARGE;
    } else {
        attributes->MBEDTLS_PRIVATE(bits) = (psa_key_bits_t) bits;
    }
}

static inline size_t psa_get_key_bits(
    const psa_key_attributes_t *attributes)
{
    return attributes->MBEDTLS_PRIVATE(bits);
}

   
                                                         
   
struct psa_sign_hash_interruptible_operation_s {
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
    mbedtls_psa_client_handle_t handle;
#else
                                                                
                                                                     
                                                              
                                                                   
                                                                      
                                                                   
    unsigned int MBEDTLS_PRIVATE(id);

    psa_driver_sign_hash_interruptible_context_t MBEDTLS_PRIVATE(ctx);

    unsigned int MBEDTLS_PRIVATE(error_occurred) : 1;

    uint32_t MBEDTLS_PRIVATE(num_ops);
#endif
};

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
#define PSA_SIGN_HASH_INTERRUPTIBLE_OPERATION_INIT { 0 }
#else
#define PSA_SIGN_HASH_INTERRUPTIBLE_OPERATION_INIT { 0, { 0 }, 0, 0 }
#endif

static inline struct psa_sign_hash_interruptible_operation_s
psa_sign_hash_interruptible_operation_init(void)
{
    const struct psa_sign_hash_interruptible_operation_s v =
        PSA_SIGN_HASH_INTERRUPTIBLE_OPERATION_INIT;

    return v;
}

   
                                                              
   
struct psa_verify_hash_interruptible_operation_s {
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
    mbedtls_psa_client_handle_t handle;
#else
                                                                
                                                                     
                                                              
                                                                   
                                                                      
                                                                   
    unsigned int MBEDTLS_PRIVATE(id);

    psa_driver_verify_hash_interruptible_context_t MBEDTLS_PRIVATE(ctx);

    unsigned int MBEDTLS_PRIVATE(error_occurred) : 1;

    uint32_t MBEDTLS_PRIVATE(num_ops);
#endif
};

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
#define PSA_VERIFY_HASH_INTERRUPTIBLE_OPERATION_INIT { 0 }
#else
#define PSA_VERIFY_HASH_INTERRUPTIBLE_OPERATION_INIT { 0, { 0 }, 0, 0 }
#endif

static inline struct psa_verify_hash_interruptible_operation_s
psa_verify_hash_interruptible_operation_init(void)
{
    const struct psa_verify_hash_interruptible_operation_s v =
        PSA_VERIFY_HASH_INTERRUPTIBLE_OPERATION_INIT;

    return v;
}

#ifdef __cplusplus
}
#endif

#endif                          
