   
                           
  
                                                             
  
                                                                  
                        
  
                                                         
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_EXTRA_H
#define PSA_CRYPTO_EXTRA_H
#include "mbedtls/private_access.h"

#include "crypto_types.h"
#include "crypto_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

                                 
#define PSA_CRYPTO_ITS_RANDOM_SEED_UID 0xFFFFFF52

                                         
#if !defined(MBEDTLS_PSA_KEY_SLOT_COUNT)
#define MBEDTLS_PSA_KEY_SLOT_COUNT 32
#endif

                                                                              
                                                                                       
                                                                                         
#if !defined(MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE)

#define MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE 1

#if PSA_EXPORT_ASYMMETRIC_KEY_MAX_SIZE > MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE
#undef MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE
#define MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE PSA_EXPORT_ASYMMETRIC_KEY_MAX_SIZE
#endif

                                          
#if PSA_CIPHER_MAX_KEY_LENGTH > MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE
#undef MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE
#define MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE PSA_CIPHER_MAX_KEY_LENGTH
#endif

                                                                              
                    
#if defined(PSA_WANT_ALG_HMAC)
#if PSA_HASH_MAX_SIZE > MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE
#undef MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE
#define MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE PSA_HASH_MAX_SIZE
#endif
#endif                        

#endif                                              

                          
     
   

                                                      
  
                                                                     
                                                 
  
                                                                    
                                                                           
                                                                          
                                                        
  
                                                                       
                                                                       
                                                                   
                                                                         
                                                                       
                                                                          
                                                                       
                    
   
static inline void psa_set_key_enrollment_algorithm(
    psa_key_attributes_t *attributes,
    psa_algorithm_t alg2)
{
    attributes->MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(alg2) = alg2;
}

                                                                 
  
                                                                     
  
                                                                      
   
static inline psa_algorithm_t psa_get_key_enrollment_algorithm(
    const psa_key_attributes_t *attributes)
{
    return attributes->MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(alg2);
}

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)

                                                   
  
                                                                     
           
  
                                                                        
                                                                   
                                                                           
                                    
  
                                                                     
                                                                               
  
                       
                                                                      
                                                      
                                   
                                                                
                                                         
                                      
                                                      
   
psa_status_t psa_get_key_slot_number(
    const psa_key_attributes_t *attributes,
    psa_key_slot_number_t *slot_number);

                                                 
  
                                                                  
             
  
                                                                        
                                                                        
                                                                       
  
                                                                       
                                                          
                                                                       
                                                       
                                                                     
                                                                          
                                                                       
                                                               
                                                                     
                         
  
                                                                    
                                                       
   
static inline void psa_set_key_slot_number(
    psa_key_attributes_t *attributes,
    psa_key_slot_number_t slot_number)
{
    attributes->MBEDTLS_PRIVATE(has_slot_number) = 1;
    attributes->MBEDTLS_PRIVATE(slot_number) = slot_number;
}

                                                                    
  
                                                                
  
                                                                    
   
static inline void psa_clear_key_slot_number(
    psa_key_attributes_t *attributes)
{
    attributes->MBEDTLS_PRIVATE(has_slot_number) = 0;
}

                                                               
  
                                                                
                                                        
                                                        
                                                                   
                              
  
                                                                   
                                                                            
                                                                             
                                                          
                                                                         
                                                            
                                                                             
                                                                          
                                                        
  
                       
                                               
                                                                           
                                                                          
                                                           
                                    
                                                                  
                         
                                   
                                                                        
                                     
                                      
                                                                               
                                    
                                                                  
                                                                                
                                                     
                                   
                                                                           
                                                           
                                                            
                                                             
                                                    
                                                    
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t mbedtls_psa_register_se_key(
    const psa_key_attributes_t *attributes);

#endif                              

       

   
                                   
  
                                                               
                                 
                                                                          
                                                                           
  
                                 
   
void mbedtls_psa_crypto_free(void);

                           
                                                    
  
                                                                            
                                                                        
   
typedef struct mbedtls_psa_stats_s {
                                                                      
    size_t MBEDTLS_PRIVATE(volatile_slots);
                                                                     
                                      
    size_t MBEDTLS_PRIVATE(persistent_slots);
                                                            
                         
    size_t MBEDTLS_PRIVATE(external_slots);
                                                              
                           
    size_t MBEDTLS_PRIVATE(half_filled_slots);
                                                   
    size_t MBEDTLS_PRIVATE(cache_slots);
                                                          
    size_t MBEDTLS_PRIVATE(empty_slots);
                                           
    size_t MBEDTLS_PRIVATE(locked_slots);
                                                                               
    psa_key_id_t MBEDTLS_PRIVATE(max_open_internal_key_id);
                                                                   
    psa_key_id_t MBEDTLS_PRIVATE(max_open_external_key_id);
} mbedtls_psa_stats_t;

                               
                                                    
  
                                                                    
                                                                     
                                      
   
void mbedtls_psa_get_stats(mbedtls_psa_stats_t *stats);

   
                                                                      
                         
  
                                                                           
                                                                        
                                                                  
                                                                 
                    
  
                                                                     
                                                                     
                                                               
                                                                    
                                                                     
  
                                                                  
                                                                    
                         
  
                                                                       
                                                                       
                     
  
                                                                             
  
                                                                    
                                                                        
                                                                 
                                                                              
                                                                        
                                                                        
                                                                    
                                                                      
                                                                    
                                                                        
                                       
  
                                 
  
                                                                    
                                                                      
                                                             
                                                    
                                                                            
                                         
                                                                       
                                                       
                                                                         
                                                                        
                                                                            
                                                                              
                                 
                                                       
                                                           
  
                       
                                                                         
                                                                 
                                                                    
                          
                                      
                                        
                                     
                                                               
                                   
                                                                    
                                          
   
psa_status_t mbedtls_psa_inject_entropy(const uint8_t *seed,
                                        size_t seed_size);

                            
     
   

                   
  
                                      
                                                                        
                                                                            
            
   
#define PSA_KEY_TYPE_DSA_PUBLIC_KEY                 ((psa_key_type_t) 0x4002)

                                          
  
                                      
                                                                         
                                                                             
                     
  
                                                                         
                                                               
                                                            
                                                                   
                                                               
                                                   
                                                                      
  
   
#define PSA_KEY_TYPE_DSA_KEY_PAIR                    ((psa_key_type_t) 0x7002)

                                                             
#define PSA_KEY_TYPE_IS_DSA(type)                                       \
    (PSA_KEY_TYPE_PUBLIC_KEY_OF_KEY_PAIR(type) == PSA_KEY_TYPE_DSA_PUBLIC_KEY)

#define PSA_ALG_DSA_BASE                        ((psa_algorithm_t) 0x06000400)
                               
  
                                                      
                                                 
  
                                                                        
                                                               
                                                       
                                                                        
  
                                                                  
                                                                     
                                       
   
#define PSA_ALG_DSA(hash_alg)                             \
    (PSA_ALG_DSA_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
#define PSA_ALG_DETERMINISTIC_DSA_BASE          ((psa_algorithm_t) 0x06000500)
#define PSA_ALG_DSA_DETERMINISTIC_FLAG PSA_ALG_ECDSA_DETERMINISTIC_FLAG
                                             
  
                                                           
                                              
  
                                                                        
                                                               
                                                       
                                                                        
  
                                                                  
                                                                     
                                       
   
#define PSA_ALG_DETERMINISTIC_DSA(hash_alg)                             \
    (PSA_ALG_DETERMINISTIC_DSA_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
#define PSA_ALG_IS_DSA(alg)                                             \
    (((alg) & ~PSA_ALG_HASH_MASK & ~PSA_ALG_DSA_DETERMINISTIC_FLAG) ==  \
     PSA_ALG_DSA_BASE)
#define PSA_ALG_DSA_IS_DETERMINISTIC(alg)               \
    (((alg) & PSA_ALG_DSA_DETERMINISTIC_FLAG) != 0)
#define PSA_ALG_IS_DETERMINISTIC_DSA(alg)                       \
    (PSA_ALG_IS_DSA(alg) && PSA_ALG_DSA_IS_DETERMINISTIC(alg))
#define PSA_ALG_IS_RANDOMIZED_DSA(alg)                          \
    (PSA_ALG_IS_DSA(alg) && !PSA_ALG_DSA_IS_DETERMINISTIC(alg))


                                                             
                         
#undef PSA_ALG_IS_VENDOR_HASH_AND_SIGN
#define PSA_ALG_IS_VENDOR_HASH_AND_SIGN(alg)    \
    PSA_ALG_IS_DSA(alg)

       

                          
     
   

                             
#define PSA_PAKE_OPERATION_STAGE_SETUP 0
#define PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS 1
#define PSA_PAKE_OPERATION_STAGE_COMPUTATION 2

       


                                      
     
   

#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
                                                                    
  
                                                                            
                                                                     
                                                         
  
                                                                             
                                                                           
                                                                         
                                                                        
                                                                           
                                                                        
                                                                           
  
                                                                        
                                                                       
                                                                       
                                                                      
                                                                   
                                             
                                                                          
                                                                             
  
                       
                                                                      
                                                                      
                                 
                                          
                                                                      
                                                                      
                                                                        
                                                                        
                                                                        
                                                                       
                                                 
                                      
                                                                        
                                              
   
psa_status_t mbedtls_psa_external_get_random(
    mbedtls_psa_external_random_context_t *context,
    uint8_t *output, size_t output_size, size_t *output_length);
#endif                                      

                                                          
  
                                                                 
  
                                                                      
                                                                   
                                                  
  
                                                                          
                                                                          
                                                                   
                                                                     
                                                                        
                                                                     
                                                                           
  
                                                                     
                                                                    
                                     
                                                                     
                                                                   
                                                            
                                                                 
                                                                    
               
  
                                                                          
                                                       
                                                                       
                                                          
                                                                         
                                                                      
                                                                       
                                           
  
                                                                 
                                                      
  
                                                                     
                                                                       
                              
  
                                                                           
                                                                     
                                                                       
                                                                    
                                                                      
                    
  
                                                                       
                                                                          
                                                                      
                                                                    
                                                                      
                                                                            
                                                                          
                               
                                                    
  
                       
                                
                               
                                                  
                                   
                                                                        
                                                                   
                                                             
                                          
                                     
   
psa_status_t psa_random_reseed(const uint8_t *perso, size_t perso_size);

                                                                        
  
                                                                 
  
                                                                      
                                                                   
                                                  
  
                                                              
                                                                         
                                                                     
                                                                        
                                   
  
                                                                          
                    
  
                                                                           
                                                                   
                                                                          
  
                       
                                
                               
                                                  
                                   
                                                                        
                                                                   
                                                             
   
psa_status_t psa_random_deplete(void);

                                                                        
  
                                                              
                                                                    
                                                                   
                                                      
                                                                   
                                                                
                                                                
  
                                                                       
                                                   
                                                           
                                                               
  
                                                                 
                                                                          
                                                                     
                      
  
                                                                          
                    
  
                                                                             
                                                                          
                                                      
  
                                                         
                                                          
  
                       
                                                                        
                                                  
                               
                                                  
                                      
                                   
                                   
                                                                        
                                                              
                                                                              
                                                                
   
psa_status_t psa_random_set_prediction_resistance(unsigned enabled);

       

                                            
     
   

                                                                 
                  
  
                                                                    
                                                           
                                                                           
                                                               
  
                                                                             
                                                          
   
#define MBEDTLS_PSA_KEY_ID_BUILTIN_MIN          ((psa_key_id_t) 0x7fff0000)

                                                                 
                  
  
                                                            
   
#define MBEDTLS_PSA_KEY_ID_BUILTIN_MAX          ((psa_key_id_t) 0x7fffefff)

                                                
  
                                                          
   
typedef uint64_t psa_drv_slot_number_t;

#if defined(MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
                                                                   
  
                                         
  
            
                                                          
            
                                                              
   
static inline int psa_key_id_is_builtin(psa_key_id_t key_id)
{
    return (key_id >= MBEDTLS_PSA_KEY_ID_BUILTIN_MIN) &&
           (key_id <= MBEDTLS_PSA_KEY_ID_BUILTIN_MAX);
}

                                                                               
  
                                                                              
                                                                                
                                        
  
                                                                          
                                                                      
  
                                       
                                                           
                                                                              
                                   
  
                                                                    
                                                             
                                                                                
                                                                         
                                                                             
                                                                          
                                                                    
                                                                        
                                                                              
                                                                               
                                                             
                                                                               
                                                                            
                                                                            
                                                       
  
                       
                                                                  
                                                                    
                                   
                                    
                                                                            
                                                                         
                                                              
                            
                                                                                
                                 
                                                                             
                                         
   
psa_status_t mbedtls_psa_platform_get_builtin_key(
    mbedtls_svc_key_id_t key_id,
    psa_key_lifetime_t *lifetime,
    psa_drv_slot_number_t *slot_number);
#endif                                      

         

                                                                      
  
                                                                          
                                                                         
                                    
  
                                                               
                                                                
  
     
   

                                                                     
  
                                                                             
                                                        
  
                                                              
                                                                          
                                                              
                                              
  
                                       
  
                                                            
   
int psa_can_do_hash(psa_algorithm_t hash_alg);

   
                                        
  
                                                              
                                                                          
                                                                           
                                              
  
                                   
                                           
  
                                                              
   
int psa_can_do_cipher(psa_key_type_t key_type, psa_algorithm_t cipher_alg);

       

                            
     
   

#define PSA_ALG_CATEGORY_PAKE                   ((psa_algorithm_t) 0x0a000000)

                                                                             
  
                                                                       
  
                                                                      
                                  
                                                                           
                                
   
#define PSA_ALG_IS_PAKE(alg)                                        \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_PAKE)

/** The Password-authenticated key exchange by juggling (J-PAKE) algorithm.
 *
 * This is J-PAKE as defined by RFC 8236, instantiated with the following
 * parameters:
 *
 * - The group can be either an elliptic curve or defined over a finite field.
 * - Schnorr NIZK proof as defined by RFC 8235 and using the same group as the
 *   J-PAKE algorithm.
 * - A cryptographic hash function.
 *
 * To select these parameters and set up the cipher suite, call these functions
 * in any order:
 *
 * \code
 * psa_pake_cs_set_algorithm(cipher_suite, PSA_ALG_JPAKE);
 * psa_pake_cs_set_primitive(cipher_suite,
 *                           PSA_PAKE_PRIMITIVE(type, family, bits));
 * psa_pake_cs_set_hash(cipher_suite, hash);
 * \endcode
 *
 * For more information on how to set a specific curve or field, refer to the
 * documentation of the individual \c PSA_PAKE_PRIMITIVE_TYPE_XXX constants.
 *
 * After initializing a J-PAKE operation, call
 *
 * \code
 * psa_pake_setup(operation, cipher_suite);
 * psa_pake_set_user(operation, ...);
 * psa_pake_set_peer(operation, ...);
 * psa_pake_set_password_key(operation, ...);
 * \endcode
 *
 * The password is provided as a key. This can be the password text itself,
 * in an agreed character encoding, or some value derived from the password
 * as required by a higher level protocol.
 *
 * (The implementation converts the key material to a number as described in
 * Section 2.3.8 of _SEC 1: Elliptic Curve Cryptography_
 * (https://www.secg.org/sec1-v2.pdf), before reducing it modulo \c q. Here
 * \c q is order of the group defined by the primitive set in the cipher suite.
 * The \c psa_pake_set_password_key() function returns an error if the result
 * of the reduction is 0.)
 *
 * The key exchange flow for J-PAKE is as follows:
 * -# To get the first round data that needs to be sent to the peer, call
 *    \code
 *    // Get g1
 *    psa_pake_output(operation, #PSA_PAKE_STEP_KEY_SHARE, ...);
 *    // Get the ZKP public key for x1
 *    psa_pake_output(operation, #PSA_PAKE_STEP_ZK_PUBLIC, ...);
 *    // Get the ZKP proof for x1
 *    psa_pake_output(operation, #PSA_PAKE_STEP_ZK_PROOF, ...);
 *    // Get g2
 *    psa_pake_output(operation, #PSA_PAKE_STEP_KEY_SHARE, ...);
 *    // Get the ZKP public key for x2
 *    psa_pake_output(operation, #PSA_PAKE_STEP_ZK_PUBLIC, ...);
 *    // Get the ZKP proof for x2
 *    psa_pake_output(operation, #PSA_PAKE_STEP_ZK_PROOF, ...);
 *    \endcode
 * -# To provide the first round data received from the peer to the operation,
 *    call
 *    \code
 *    // Set g3
 *    psa_pake_input(operation, #PSA_PAKE_STEP_KEY_SHARE, ...);
 *    // Set the ZKP public key for x3
 *    psa_pake_input(operation, #PSA_PAKE_STEP_ZK_PUBLIC, ...);
 *    // Set the ZKP proof for x3
 *    psa_pake_input(operation, #PSA_PAKE_STEP_ZK_PROOF, ...);
 *    // Set g4
 *    psa_pake_input(operation, #PSA_PAKE_STEP_KEY_SHARE, ...);
 *    // Set the ZKP public key for x4
 *    psa_pake_input(operation, #PSA_PAKE_STEP_ZK_PUBLIC, ...);
 *    // Set the ZKP proof for x4
 *    psa_pake_input(operation, #PSA_PAKE_STEP_ZK_PROOF, ...);
 *    \endcode
 * -# To get the second round data that needs to be sent to the peer, call
 *    \code
 *    // Get A
 *    psa_pake_output(operation, #PSA_PAKE_STEP_KEY_SHARE, ...);
 *    // Get ZKP public key for x2*s
 *    psa_pake_output(operation, #PSA_PAKE_STEP_ZK_PUBLIC, ...);
 *    // Get ZKP proof for x2*s
 *    psa_pake_output(operation, #PSA_PAKE_STEP_ZK_PROOF, ...);
 *    \endcode
 * -# To provide the second round data received from the peer to the operation,
 *    call
 *    \code
 *    // Set B
 *    psa_pake_input(operation, #PSA_PAKE_STEP_KEY_SHARE, ...);
 *    // Set ZKP public key for x4*s
 *    psa_pake_input(operation, #PSA_PAKE_STEP_ZK_PUBLIC, ...);
 *    // Set ZKP proof for x4*s
 *    psa_pake_input(operation, #PSA_PAKE_STEP_ZK_PROOF, ...);
 *    \endcode
 * -# To access the shared secret call
 *    \code
 *    // Get Ka=Kb=K
 *    psa_pake_get_implicit_key()
 *    \endcode
 *
 * For more information consult the documentation of the individual
 * \c PSA_PAKE_STEP_XXX constants.
 *
 * At this point there is a cryptographic guarantee that only the authenticated
 * party who used the same password is able to compute the key. But there is no
 * guarantee that the peer is the party it claims to be and was able to do so.
 *
 * That is, the authentication is only implicit (the peer is not authenticated
 * at this point, and no action should be taken that assume that they are - like
 * for example accessing restricted files).
 *
 * To make the authentication explicit there are various methods, see Section 5
 * of RFC 8236 for two examples.
 *
 * \note The JPAKE implementation has the following limitations:
 *       - The only supported primitive is ECC on the curve secp256r1, i.e.
 *         `PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,
 *          PSA_ECC_FAMILY_SECP_R1, 256)`.
 *       - The only supported hash algorithm is SHA-256, i.e.
 *         `PSA_ALG_SHA_256`.
 *       - When using the built-in implementation, the user ID and the peer ID
 *         must be `"client"` (6-byte string) and `"server"` (6-byte string),
 *         or the other way round.
 *         Third-party drivers may or may not have this limitation.
 *
 */
#define PSA_ALG_JPAKE                   ((psa_algorithm_t) 0x0a000100)

         

                                                             
  
                                                                              
                                   
  
                                                                          
                                                                        
                                                                           
                        
     
   

                                                   
  
                                                                              
                                                                       
             
   
typedef uint8_t psa_pake_role_t;

                                                     
  
                                                                                
                                                                      
              
   
typedef uint8_t psa_pake_step_t;

                                                 
  
                                                                        
                                                                          
  
                                                           
                                            
   
typedef uint8_t psa_pake_primitive_type_t;

                                                                            
  
                                                           
                                            
   
typedef uint8_t psa_pake_family_t;

                                                              
  
                                                                               
   
typedef uint32_t psa_pake_primitive_t;

                                                    
                                                                             
                                        
   
#define PSA_PAKE_ROLE_NONE                  ((psa_pake_role_t) 0x00)

                                      
  
                                                                         
                                                                               
                                                                     
            
   
#define PSA_PAKE_ROLE_FIRST                ((psa_pake_role_t) 0x01)

                                       
  
                                                                         
                                                                               
                                                                      
            
   
#define PSA_PAKE_ROLE_SECOND                ((psa_pake_role_t) 0x02)

                                    
  
                                                                             
   
#define PSA_PAKE_ROLE_CLIENT                ((psa_pake_role_t) 0x11)

                                    
  
                                                                             
   
#define PSA_PAKE_ROLE_SERVER                ((psa_pake_role_t) 0x12)

                                                                  
  
                                                                                
                                                                       
                             
  
                                                                           
                                                               
  
                                                                              
          
                                                                         
                                                                               
                            
                                                                            
                                                                      
                     
   
#define PSA_PAKE_PRIMITIVE_TYPE_ECC       ((psa_pake_primitive_type_t) 0x01)

                                                                        
  
                                                                              
                                                                           
                                           
  
                                                                           
                                                               
  
                                                                              
          
                                                                         
                                                                               
                            
                                                                            
                                                                      
                     
   
#define PSA_PAKE_PRIMITIVE_TYPE_DH       ((psa_pake_primitive_type_t) 0x02)

                                                              
  
                                                 
                                                                    
                                                   
                                                                              
                                                                         
                                                                               
                                   
                                                     
                                                                    
                                                                             
                                                                           
                                                               
  
                                                                          
                                                                  
                                  
   
#define PSA_PAKE_PRIMITIVE(pake_type, pake_family, pake_bits) \
    ((pake_bits & 0xFFFF) != pake_bits) ? 0 :                 \
    ((psa_pake_primitive_t) (((pake_type) << 24 |             \
                              (pake_family) << 16) | (pake_bits)))

                                                          
  
                                                                              
                                                                               
      
  
                                                                   
                           
  
                                                                     
                                     
   
#define PSA_PAKE_STEP_KEY_SHARE                 ((psa_pake_step_t) 0x01)

                               
  
                                                                  
                                                                          
  
                                                                              
                                                                               
      
  
                                                                   
                           
  
                                                                     
                                     
   
#define PSA_PAKE_STEP_ZK_PUBLIC                 ((psa_pake_step_t) 0x02)

                          
  
                                                                             
                                                
  
                                                                            
                                                                             
                     
  
                                                          
                                                                         
                                                                               
  
                                                                               
                                                 
  
                                                                     
                                     
   
#define PSA_PAKE_STEP_ZK_PROOF                  ((psa_pake_step_t) 0x03)

       

                                                          
  
                                                                            
                                                                            
                                                                          
  
                                     
  
                                                                        
                                                          
                                                                          
                                                         
                                                                               
                                         
                                                                         
                                                                          
                                                                        
                                                                       
                                 
   
#define PSA_PAKE_OUTPUT_SIZE(alg, primitive, output_step)               \
    (alg == PSA_ALG_JPAKE &&                                           \
     primitive == PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,      \
                                     PSA_ECC_FAMILY_SECP_R1, 256) ?    \
     (                                                                 \
         output_step == PSA_PAKE_STEP_KEY_SHARE ? 65 :                   \
         output_step == PSA_PAKE_STEP_ZK_PUBLIC ? 65 :                   \
         32                                                              \
     ) :                                                               \
     0)

                                                        
  
                                                                            
                                                                     
              
  
                                    
  
                                                                        
                                                          
                                                                          
                                                         
                                                                               
                                         
                                                                        
                                                                               
                                                                              
                                                                  
   
#define PSA_PAKE_INPUT_SIZE(alg, primitive, input_step)                 \
    (alg == PSA_ALG_JPAKE &&                                           \
     primitive == PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,      \
                                     PSA_ECC_FAMILY_SECP_R1, 256) ?    \
     (                                                                 \
         input_step == PSA_PAKE_STEP_KEY_SHARE ? 65 :                    \
         input_step == PSA_PAKE_STEP_ZK_PUBLIC ? 65 :                    \
         32                                                              \
     ) :                                                               \
     0)

                                                                          
                                                  
  
                                                             
  
                                                                         
                                     
  
                                                                        
   
#define PSA_PAKE_OUTPUT_MAX_SIZE 65

                                                                        
                                                 
  
                                                             
  
                                                                         
                                    
  
                                                                       
   
#define PSA_PAKE_INPUT_MAX_SIZE 65

                                                                         
                           
   
#define PSA_PAKE_CIPHER_SUITE_INIT { PSA_ALG_NONE, 0, 0, 0, PSA_ALG_NONE }

                                                                      
                        
   
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
#define PSA_PAKE_OPERATION_INIT { 0 }
#else
#define PSA_PAKE_OPERATION_INIT { 0, PSA_ALG_NONE, 0, PSA_PAKE_OPERATION_STAGE_SETUP, \
                                  { 0 }, { { 0 } } }
#endif

struct psa_pake_cipher_suite_s {
    psa_algorithm_t algorithm;
    psa_pake_primitive_type_t type;
    psa_pake_family_t family;
    uint16_t  bits;
    psa_algorithm_t hash;
};

struct psa_crypto_driver_pake_inputs_s {
    uint8_t *MBEDTLS_PRIVATE(password);
    size_t MBEDTLS_PRIVATE(password_len);
    uint8_t *MBEDTLS_PRIVATE(user);
    size_t MBEDTLS_PRIVATE(user_len);
    uint8_t *MBEDTLS_PRIVATE(peer);
    size_t MBEDTLS_PRIVATE(peer_len);
    psa_key_attributes_t MBEDTLS_PRIVATE(attributes);
    struct psa_pake_cipher_suite_s MBEDTLS_PRIVATE(cipher_suite);
};

typedef enum psa_crypto_driver_pake_step {
    PSA_JPAKE_STEP_INVALID        = 0,                    
    PSA_JPAKE_X1_STEP_KEY_SHARE   = 1,                                                                      
    PSA_JPAKE_X1_STEP_ZK_PUBLIC   = 2,                                                                     
    PSA_JPAKE_X1_STEP_ZK_PROOF    = 3,                                                                
    PSA_JPAKE_X2_STEP_KEY_SHARE   = 4,                                                                      
    PSA_JPAKE_X2_STEP_ZK_PUBLIC   = 5,                                                                     
    PSA_JPAKE_X2_STEP_ZK_PROOF    = 6,                                                                
    PSA_JPAKE_X2S_STEP_KEY_SHARE  = 7,                                         
    PSA_JPAKE_X2S_STEP_ZK_PUBLIC  = 8,                                                                          
    PSA_JPAKE_X2S_STEP_ZK_PROOF   = 9,                                                                     
    PSA_JPAKE_X4S_STEP_KEY_SHARE  = 10,                                         
    PSA_JPAKE_X4S_STEP_ZK_PUBLIC  = 11,                                                                          
    PSA_JPAKE_X4S_STEP_ZK_PROOF   = 12                                                                      
} psa_crypto_driver_pake_step_t;

typedef enum psa_jpake_round {
    PSA_JPAKE_FIRST = 0,
    PSA_JPAKE_SECOND = 1,
    PSA_JPAKE_FINISHED = 2
} psa_jpake_round_t;

typedef enum psa_jpake_io_mode {
    PSA_JPAKE_INPUT = 0,
    PSA_JPAKE_OUTPUT = 1
} psa_jpake_io_mode_t;

struct psa_jpake_computation_stage_s {
                                              
    psa_jpake_round_t MBEDTLS_PRIVATE(round);
                                                                  
    psa_jpake_io_mode_t MBEDTLS_PRIVATE(io_mode);
                                                          
    uint8_t MBEDTLS_PRIVATE(inputs);
                                                           
    uint8_t MBEDTLS_PRIVATE(outputs);
                                                                   
    psa_pake_step_t MBEDTLS_PRIVATE(step);
};

#define PSA_JPAKE_EXPECTED_INPUTS(round) ((round) == PSA_JPAKE_FINISHED ? 0 : \
                                          ((round) == PSA_JPAKE_FIRST ? 2 : 1))
#define PSA_JPAKE_EXPECTED_OUTPUTS(round) ((round) == PSA_JPAKE_FINISHED ? 0 : \
                                           ((round) == PSA_JPAKE_FIRST ? 2 : 1))

struct psa_pake_operation_s {
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
    mbedtls_psa_client_handle_t handle;
#else
                                                                
                                                                     
                                                              
                                                                   
                                                                      
                                                                   
    unsigned int MBEDTLS_PRIVATE(id);
                                         
    psa_algorithm_t MBEDTLS_PRIVATE(alg);
                                                       
    psa_pake_primitive_t MBEDTLS_PRIVATE(primitive);
                                                                            
                       
    uint8_t MBEDTLS_PRIVATE(stage);
                                                         
    union {
        uint8_t MBEDTLS_PRIVATE(dummy);
#if defined(PSA_WANT_ALG_JPAKE)
        struct psa_jpake_computation_stage_s MBEDTLS_PRIVATE(jpake);
#endif
    } MBEDTLS_PRIVATE(computation_stage);
    union {
        psa_driver_pake_context_t MBEDTLS_PRIVATE(ctx);
        struct psa_crypto_driver_pake_inputs_s MBEDTLS_PRIVATE(inputs);
    } MBEDTLS_PRIVATE(data);
#endif
};

                    
     
   

                                                          
  
                                                                       
                                                            
                                                                       
   
typedef struct psa_pake_cipher_suite_s psa_pake_cipher_suite_t;

                                                           
   
static psa_pake_cipher_suite_t psa_pake_cipher_suite_init(void);

                                                         
  
                                                                   
  
                                                                   
   
static psa_algorithm_t psa_pake_cs_get_algorithm(
    const psa_pake_cipher_suite_t *cipher_suite);

                                                    
  
                                              
                                     
  
                                                                          
  
                                                                      
                                                           
                                                                              
                                                                           
                                                                  
                                                                   
   
static void psa_pake_cs_set_algorithm(psa_pake_cipher_suite_t *cipher_suite,
                                      psa_algorithm_t algorithm);

                                                    
  
                                                                   
  
                                                              
   
static psa_pake_primitive_t psa_pake_cs_get_primitive(
    const psa_pake_cipher_suite_t *cipher_suite);

                                                  
  
                                                                            
  
                                                                             
                                                                         
                                       
  
                                                                      
                                                                        
                                                                        
                                           
   
static void psa_pake_cs_set_primitive(psa_pake_cipher_suite_t *cipher_suite,
                                      psa_pake_primitive_t primitive);

                                                      
  
                                                                   
  
                                                                
   
static psa_pake_family_t psa_pake_cs_get_family(
    const psa_pake_cipher_suite_t *cipher_suite);

                                                                  
  
                                                                   
  
                                                                            
   
static uint16_t psa_pake_cs_get_bits(
    const psa_pake_cipher_suite_t *cipher_suite);

                                                         
  
                                                                    
  
                                                                              
                                                                               
                                         
   
static psa_algorithm_t psa_pake_cs_get_hash(
    const psa_pake_cipher_suite_t *cipher_suite);

                                                       
  
                                              
                                     
  
                                                                               
                                                                               
                        
  
                                                                       
                                                                      
                                                                               
                                                                            
                                                                   
                                                                    
   
static void psa_pake_cs_set_hash(psa_pake_cipher_suite_t *cipher_suite,
                                 psa_algorithm_t hash);

                                                             
  
                                                                          
                                                    
                                                     
          
                                    
                                              
             
                                                                  
          
                                          
             
                                                                          
                 
          
                                                              
             
                                                                
                                   
          
                                    
                                           
             
  
                                                                       
                                                            
                                                                          
typedef struct psa_pake_operation_s psa_pake_operation_t;

                                                    
typedef struct psa_crypto_driver_pake_inputs_s psa_crypto_driver_pake_inputs_t;

                                                           
typedef struct psa_jpake_computation_stage_s psa_jpake_computation_stage_t;

                                                        
   
static psa_pake_operation_t psa_pake_operation_init(void);

                                                              
  
                                                 
                                                
  
                       
                   
                               
                                        
   
psa_status_t psa_crypto_driver_pake_get_password_len(
    const psa_crypto_driver_pake_inputs_t *inputs,
    size_t *password_len);

                                       
  
                                                 
                                                           
                                                                   
                                                                     
  
                       
                   
                               
                                        
   
psa_status_t psa_crypto_driver_pake_get_password(
    const psa_crypto_driver_pake_inputs_t *inputs,
    uint8_t *buffer, size_t buffer_size, size_t *buffer_length);

                                                             
  
                                                 
                                               
  
                       
                   
                               
                                       
   
psa_status_t psa_crypto_driver_pake_get_user_len(
    const psa_crypto_driver_pake_inputs_t *inputs,
    size_t *user_len);

                                                             
  
                                                 
                                               
  
                       
                   
                               
                                       
   
psa_status_t psa_crypto_driver_pake_get_peer_len(
    const psa_crypto_driver_pake_inputs_t *inputs,
    size_t *peer_len);

                                      
  
                                                 
                                        
                                                            
                                                             
  
                       
                   
                               
                                       
                                      
                                                   
   
psa_status_t psa_crypto_driver_pake_get_user(
    const psa_crypto_driver_pake_inputs_t *inputs,
    uint8_t *user_id, size_t user_id_size, size_t *user_id_len);

                                      
  
                                                 
                                        
                                                            
                                                             
  
                       
                   
                               
                                       
                                      
                                                   
   
psa_status_t psa_crypto_driver_pake_get_peer(
    const psa_crypto_driver_pake_inputs_t *inputs,
    uint8_t *peer_id, size_t peer_id_size, size_t *peer_id_length);

                                           
  
                                                 
                                                       
  
                       
                   
                               
                                            
   
psa_status_t psa_crypto_driver_pake_get_cipher_suite(
    const psa_crypto_driver_pake_inputs_t *inputs,
    psa_pake_cipher_suite_t *cipher_suite);

                                                                          
  
                                                                             
                 
                                                                            
                  
                                                                              
                                                   
                               
                                                        
                                                                           
                                                                             
                                                   
  
                                                                               
                                                                               
                        
  
                                                                      
            
                                                                               
                                                  
                                                                              
                                                    
                                                                          
                                          
                                                                       
  
                                                                               
                                                                               
                        
  
                                                                   
                                                                         
                                                                        
                        
  
                                                                    
                                                                        
             
                                
                                                      
  
                                                                            
                                                                    
                                                                              
                                                                             
                                                       
  
                       
                   
                                      
                                                                           
                                                                       
                                                                              
                                                                   
                                   
                                                                              
                                                                           
                                                                       
                                                                           
                                   
                                                             
                                                           
                               
                                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_pake_setup(psa_pake_operation_t *operation,
                            const psa_pake_cipher_suite_t *cipher_suite);

/** Set the password for a password-authenticated key exchange from key ID.
 *
 * Call this function when the password, or a value derived from the password,
 * is already present in the key store.
 *
 * \param[in,out] operation     The operation object to set the password for. It
 *                              must have been set up by psa_pake_setup() and
 *                              not yet in use (neither psa_pake_output() nor
 *                              psa_pake_input() has been called yet). It must
 *                              be on operation for which the password hasn't
 *                              been set yet (psa_pake_set_password_key()
 *                              hasn't been called yet).
 * \param password              Identifier of the key holding the password or a
 *                              value derived from the password (eg. by a
 *                              memory-hard function).  It must remain valid
 *                              until the operation terminates. It must be of
 *                              type #PSA_KEY_TYPE_PASSWORD or
 *                              #PSA_KEY_TYPE_PASSWORD_HASH. It has to allow
 *                              the usage #PSA_KEY_USAGE_DERIVE.
 *
 * \retval #PSA_SUCCESS
 *         Success.
 * \retval #PSA_ERROR_INVALID_HANDLE
 *         \p password is not a valid key identifier.
 * \retval #PSA_ERROR_NOT_PERMITTED
 *         The key does not have the #PSA_KEY_USAGE_DERIVE flag, or it does not
 *         permit the \p operation's algorithm.
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 *         The key type for \p password is not #PSA_KEY_TYPE_PASSWORD or
 *         #PSA_KEY_TYPE_PASSWORD_HASH, or \p password is not compatible with
 *         the \p operation's cipher suite.
 * \retval #PSA_ERROR_NOT_SUPPORTED
 *         The key type or key size of \p password is not supported with the
 *         \p operation's cipher suite.
 * \retval #PSA_ERROR_COMMUNICATION_FAILURE \emptydescription
 * \retval #PSA_ERROR_CORRUPTION_DETECTED \emptydescription
 * \retval #PSA_ERROR_STORAGE_FAILURE \emptydescription
 * \retval #PSA_ERROR_DATA_CORRUPT \emptydescription
 * \retval #PSA_ERROR_DATA_INVALID \emptydescription
 * \retval #PSA_ERROR_BAD_STATE
 *         The operation state is not valid (it must have been set up.), or
 *         the library has not been previously initialized by psa_crypto_init().
 *         It is implementation-dependent whether a failure to initialize
 *         results in this error code.
 */
psa_status_t psa_pake_set_password_key(psa_pake_operation_t *operation,
                                       mbedtls_svc_key_id_t password);

                                                              
  
                                                                              
                                                                 
                                                                           
                                                                   
  
                                                                               
                                                                               
                        
  
                                                                              
                                                                          
                                                                 
  
                                                                               
                                                                             
                                                                             
                                                                              
                                                                            
                                                                         
                                            
                                                                 
                                                                       
  
                       
                   
                                      
                                                                              
                 
                                   
                                                                          
                                                           
                                                             
                                                           
                               
                                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_pake_set_user(psa_pake_operation_t *operation,
                               const uint8_t *user_id,
                               size_t user_id_len);

                                                              
  
                                                                            
                                                                           
                                                                            
                            
  
                                                                               
                                                                               
                        
  
                                                                              
                                                                          
                                                                 
  
                                                                               
                                                                             
                                                                             
                                                                              
                                                                            
                                                                         
                                            
                                                              
                                                                       
  
                       
                   
                                      
                                                                              
                 
                                   
                                                                              
                                                           
                                                             
                                                           
                               
                                                                         
                                                                              
                                                            
                                                                         
                                      
   
psa_status_t psa_pake_set_peer(psa_pake_operation_t *operation,
                               const uint8_t *peer_id,
                               size_t peer_id_len);

                                                                       
  
                                                                            
                                                                           
                                                                             
                                                   
  
                                                                               
                                                                               
                        
  
                                                                   
                                                                                
                                                                               
                                                                               
                                                                              
                                                                            
                                                                             
                                                                                
                                                                            
                                                                              
                                                                         
                                          
  
                       
                   
                                      
                                                                                
                                   
                                                                           
                                                             
                                                           
                               
                                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_pake_set_role(psa_pake_operation_t *operation,
                               psa_pake_role_t role);

                                                                   
  
                                                                         
                                                                    
  
                                                                      
                                                                           
                                                                
                                                                         
               
  
                                                                          
                                                         
  
                                                     
                                                                                
                                         
                                                                              
                                                                            
                                                                  
                                                                      
                                           
                                                                               
                                                                           
                                                                          
                                                                                
                                                                              
                                               
  
                                                                              
                                      
  
                       
                   
                                      
                                                         
                                      
                                                                    
                                   
                                                                   
                                                            
                                                           
                                                             
                                                           
                                                       
                                                    
                                                    
                               
                                                                             
                                                                         
                                                      
                                                                                
                                                                         
                                      
   
psa_status_t psa_pake_output(psa_pake_operation_t *operation,
                             psa_pake_step_t step,
                             uint8_t *output,
                             size_t output_size,
                             size_t *output_length);

                                                                      
  
                                                                         
                                                                    
  
                                                                      
                                                                           
                                                                
                                                                         
               
  
                                                                          
                                                         
  
                                                     
                                                                        
                                                                        
                                                                         
                                                              
                                                                      
                                           
                                                                    
  
                       
                   
                                       
                                                                           
                                      
                                                                               
                                                                         
                                   
                                   
                                                                               
                                                                             
                            
                                                           
                                                             
                                                           
                                                       
                                                    
                                                    
                               
                                                                             
                                                                         
                                                      
                                                                                
                                                                         
                                      
   
psa_status_t psa_pake_input(psa_pake_operation_t *operation,
                            psa_pake_step_t step,
                            const uint8_t *input,
                            size_t input_length);

                                                       
  
                                                                               
                                                                               
                                                                              
  
                                                                      
                                                                              
                                                                             
                                                                
  
                                                                            
                                                                          
                                                
                                                                       
                                            
  
                                                                      
                                                                           
                                                                
                                                                         
               
  
                                                                          
                                                              
                                                                               
                                                                        
  
                                                     
                                                                       
                                                        
                                                                
  
                       
                   
                                      
                                                                      
                                                               
                                   
                                                                               
                                    
                                                           
                                                             
                                                           
                                                       
                                                    
                                                    
                               
                                                                               
                                                          
                                                                                
                                                     
                                                                            
                                                                             
                                      
                                                                         
                                      
   
psa_status_t psa_pake_get_implicit_key(psa_pake_operation_t *operation,
                                       psa_key_derivation_operation_t *output);

                           
  
                                                                         
                                                                               
                                                           
  
                                                              
                                                                     
  
                                                                       
                                                                          
                             
  
                                                      
  
                       
                   
                                                             
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_pake_abort(psa_pake_operation_t *operation);

       

static inline psa_algorithm_t psa_pake_cs_get_algorithm(
    const psa_pake_cipher_suite_t *cipher_suite)
{
    return cipher_suite->algorithm;
}

static inline void psa_pake_cs_set_algorithm(
    psa_pake_cipher_suite_t *cipher_suite,
    psa_algorithm_t algorithm)
{
    if (!PSA_ALG_IS_PAKE(algorithm)) {
        cipher_suite->algorithm = 0;
    } else {
        cipher_suite->algorithm = algorithm;
    }
}

static inline psa_pake_primitive_t psa_pake_cs_get_primitive(
    const psa_pake_cipher_suite_t *cipher_suite)
{
    return PSA_PAKE_PRIMITIVE(cipher_suite->type, cipher_suite->family,
                              cipher_suite->bits);
}

static inline void psa_pake_cs_set_primitive(
    psa_pake_cipher_suite_t *cipher_suite,
    psa_pake_primitive_t primitive)
{
    cipher_suite->type = (psa_pake_primitive_type_t) (primitive >> 24);
    cipher_suite->family = (psa_pake_family_t) (0xFF & (primitive >> 16));
    cipher_suite->bits = (uint16_t) (0xFFFF & primitive);
}

static inline psa_pake_family_t psa_pake_cs_get_family(
    const psa_pake_cipher_suite_t *cipher_suite)
{
    return cipher_suite->family;
}

static inline uint16_t psa_pake_cs_get_bits(
    const psa_pake_cipher_suite_t *cipher_suite)
{
    return cipher_suite->bits;
}

static inline psa_algorithm_t psa_pake_cs_get_hash(
    const psa_pake_cipher_suite_t *cipher_suite)
{
    return cipher_suite->hash;
}

static inline void psa_pake_cs_set_hash(psa_pake_cipher_suite_t *cipher_suite,
                                        psa_algorithm_t hash)
{
    if (!PSA_ALG_IS_HASH(hash)) {
        cipher_suite->hash = 0;
    } else {
        cipher_suite->hash = hash;
    }
}

static inline struct psa_pake_cipher_suite_s psa_pake_cipher_suite_init(void)
{
    const struct psa_pake_cipher_suite_s v = PSA_PAKE_CIPHER_SUITE_INIT;
    return v;
}

static inline struct psa_pake_operation_s psa_pake_operation_init(void)
{
    const struct psa_pake_operation_s v = PSA_PAKE_OPERATION_INIT;
    return v;
}

#ifdef __cplusplus
}
#endif

#endif                         
