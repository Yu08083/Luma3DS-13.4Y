  
                                       
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_CORE_H
#define PSA_CRYPTO_CORE_H

  
                                                                           
                                                                            
                                                                           
                                                                  
                 
   
#include "psa/build_info.h"

#include "psa/crypto.h"
#include "psa/crypto_se_driver.h"
#if defined(MBEDTLS_THREADING_C)
#include "mbedtls/threading.h"
#endif

typedef enum {
    PSA_SLOT_EMPTY = 0,
    PSA_SLOT_FILLING,
    PSA_SLOT_FULL,
    PSA_SLOT_PENDING_DELETION,
} psa_key_slot_state_t;

                                                                       
                            
   
typedef struct {
                                                                  
                                
    psa_key_attributes_t attr;

      
                                                         
                                                                
      
                                                                      
                                     
      
                                                                             
                                                                               
                                                                   
                                                                             
                                                        
      
                                                                            
                                                           
                                                                           
                                                                               
                     
       
    psa_key_slot_state_t state;

#if defined(MBEDTLS_PSA_KEY_STORE_DYNAMIC)
                                                   
                                                           
                                                                     
                                         
      
                                                                            
                                                                           
                                                                          
                                                                       
                                                                           
                 
         
    uint8_t slice_index;
#endif                                    

    union {
        struct {
                                                                   
                                                           
              
                                                                    
                                                                    
                                      
              
                                                                              
                               
              
                                                                            
                                                                            
                                                                             
                                                  
               
            int32_t next_free_relative_to_next;
        } free;

        struct {
              
                                                                                      
              
                                                                              
              
                                                                          
                                                                 
                                                                           
                                                                            
                                                                           
                                         
                                                     
                                                                              
                                              
              
                                                                           
                                                                             
                                          
                                                                          
                                                                   
                                                                   
                                                                   
                                                                       
                                    
                                                                         
                                                                           
                                                          
            size_t registered_readers;
        } occupied;
    } var;

                                             
                                                  
    struct key_data {
#if defined(MBEDTLS_PSA_STATIC_KEY_SLOTS)
        uint8_t data[MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE];
#else
        uint8_t *data;
#endif
        size_t bytes;
    } key;
} psa_key_slot_t;

#if defined(MBEDTLS_THREADING_C)

                                                                  
  
                                                           
                              
  
               
                               
                                                        
   
#define PSA_THREADING_CHK_RET(f)                       \
    do                                                 \
    {                                                  \
        if ((f) != 0) {                                \
            if (status == PSA_SUCCESS) {               \
                return PSA_ERROR_SERVICE_FAILURE;      \
            }                                          \
            return status;                             \
        }                                              \
    } while (0);

                                                       
  
                                                                      
  
               
                               
                       
                                                        
   
#define PSA_THREADING_CHK_GOTO_EXIT(f)                 \
    do                                                 \
    {                                                  \
        if ((f) != 0) {                                \
            if (status == PSA_SUCCESS) {               \
                status = PSA_ERROR_SERVICE_FAILURE;    \
            }                                          \
            goto exit;                                 \
        }                                              \
    } while (0);
#endif

                                                       
                                                          
                         
  
                                             
  
                                                                 
   
static inline int psa_key_slot_has_readers(const psa_key_slot_t *slot)
{
    return slot->var.occupied.registered_readers > 0;
}

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
                                                                              
  
                                                                           
                                                                         
                                                                          
   
static inline psa_key_slot_number_t psa_key_slot_get_slot_number(
    const psa_key_slot_t *slot)
{
    return *((psa_key_slot_number_t *) (slot->key.data));
}
#endif

                                                           
  
                                      
                                           
                                                          
                         
  
                                             
  
                       
                                                
                                         
                                                                               
                                                                   
                                                   
                                                                
                                                    
   
psa_status_t psa_wipe_key_slot(psa_key_slot_t *slot);

                                                  
  
                                                             
                                                             
  
                       
                                                      
                                         
                                                          
                                    
                                                               
   
psa_status_t psa_allocate_buffer_to_slot(psa_key_slot_t *slot,
                                         size_t buffer_length);

                                                                        
psa_status_t psa_remove_key_data_from_memory(psa_key_slot_t *slot);

                                                            
  
                                                       
                                                                   
  
                                                              
                                                                   
                                                       
  
                       
                                                
                                         
                                                                
                       
                                    
                                                                    
   
psa_status_t psa_copy_key_material_into_slot(psa_key_slot_t *slot,
                                             const uint8_t *data,
                                             size_t data_length);

                                                      
  
                                                                
                                                                
  
                                                     
  
                                                        
   
psa_status_t mbedtls_to_psa_error(int ret);

                                  
  
                                                               
                                                                       
                                                                             
                             
  
                                                                     
                                                                            
                                       
                                                                    
                                                                            
                                                              
                                                                               
                                                                           
                                                                      
                                                      
                                                               
  
                                                           
                                      
                                                   
                                                     
                                                           
                                                           
   
psa_status_t psa_import_key_into_slot(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key_buffer, size_t key_buffer_size,
    size_t *key_buffer_length, size_t *bits);

                                 
  
                                                                          
                                                                           
                                                           
  
                                                                     
                                                                         
                                                                          
                                                                           
                                                                    
                                                                          
                                       
  
                                                           
                                                     
                                                             
                                                        
                                                           
                                                       
                                                           
   
psa_status_t psa_export_key_internal(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    uint8_t *data, size_t data_size, size_t *data_length);

                                                                          
  
                                                               
                                                                   
                                                                             
                       
  
                                                                     
                                                                         
                                                                          
                                                                           
                                                                    
                                                                          
                                       
  
                                                                  
                                                     
                                                             
                                                        
                                                           
                                                       
                                                           
   
psa_status_t psa_export_public_key_internal(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    uint8_t *data, size_t data_size, size_t *data_length);

                                                                        
  
                                                                                 
                                                            
  
                                                                              
                                                                           
                                         
   
int psa_custom_key_parameters_are_default(
    const psa_custom_key_parameters_t *custom,
    size_t custom_data_length);

   
                         
  
                                                                           
                     
  
                                                                         
                                                                           
                                                                                 
                                                                   
                                                                             
                                                                 
                                                                            
                                                
  
                       
                                              
                                                        
                                   
                                                  
                                      
                                                  
   
psa_status_t psa_generate_key_internal(const psa_key_attributes_t *attributes,
                                       const psa_custom_key_parameters_t *custom,
                                       const uint8_t *custom_data,
                                       size_t custom_data_length,
                                       uint8_t *key_buffer,
                                       size_t key_buffer_size,
                                       size_t *key_buffer_length);

                                                                    
                                   
  
                                                               
                                                                          
                                                                             
                             
  
                                                             
                                              
  
                                                                        
                                          
                                                                      
                                                                          
                                                                             
                                                    
                                                          
                                                                     
                                                                            
                                                                         
                                                               
                                                                          
  
                                         
                                      
                                                                    
                                                        
                                                                  
                                                                      
                                   
                                                     
                                                        
                                                           
                                                           
                                                            
   
psa_status_t psa_sign_message_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *input, size_t input_length,
    uint8_t *signature, size_t signature_size, size_t *signature_length);

                                                              
                                           
  
                                                               
                                                                              
                                                                             
                             
  
                                                               
                                              
  
                                                                        
                                          
                                                                      
                                                                          
                                                                             
                                                    
                                                                              
                                                                     
                                                                          
                                                                         
  
                       
                                  
                                       
                                                                     
                                              
                                                     
                                                        
                                                           
   
psa_status_t psa_verify_message_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *input, size_t input_length,
    const uint8_t *signature, size_t signature_length);

                                                       
  
                                                               
                                                                    
                                                                             
                             
  
                                                                        
                                          
                                                                      
                                                                          
                                                                             
                                                    
                                                            
                                                                    
                                                                            
                                                                         
                                                               
                                                                          
  
                                         
                                      
                                                                    
                                                        
                                                                  
                                                                      
                                   
                                                     
                                                        
                                                           
                                                           
                                                            
   
psa_status_t psa_sign_hash_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
    uint8_t *signature, size_t signature_size, size_t *signature_length);

   
                                                                          
  
                                                               
                                                                        
                                                                             
                             
  
                                                                        
                                          
                                                                      
                                                                          
                                                                             
                                                    
                                                                            
                                         
                                                                    
                                                                          
                                                                         
  
                       
                                  
                                       
                                                                     
                                              
                                                     
                                                        
                                                           
   
psa_status_t psa_verify_hash_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length);

   
                                                          
  
                                                                        
                            
  
                                 
                                                  
  
                       
                                           
                                      
                                                    
                                   
                                                                            
                                    
   
psa_status_t psa_validate_unstructured_key_bit_size(psa_key_type_t type,
                                                    size_t bits);

                                                                   
                                         
  
                                                               
                                                                            
                                                                             
                             
  
                                                                            
                                              
                                                                         
                                            
                                                                       
                                          
                                                                     
                                                                        
                                                                         
                                                             
                                                                     
                                          
                                                                         
                                                     
                                                                          
                                          
                                                                             
                                                                  
                       
                                                          
                                                      
                                                     
                                      
                                                      
                                                        
                                                                        
                          
                                      
                                             
                                   
                                                             
                                                           
                                                             
                                                        
                                                           
                                                       
                                                 
   
psa_status_t psa_key_agreement_raw_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *peer_key,
    size_t peer_key_length,
    uint8_t *shared_secret,
    size_t shared_secret_size,
    size_t *shared_secret_length);

   
                                                                    
                                                  
  
                                                               
                                                                           
                                                                           
                                                         
  
                                                                             
                                                                           
                                                                             
                                                                              
                                     
   
void mbedtls_psa_interruptible_set_max_ops(uint32_t max_ops);

   
                                                                    
                                                  
  
                                                               
                                                                           
                                                                           
                                                         
  
                                                                            
                                                                              
   
uint32_t mbedtls_psa_interruptible_get_max_ops(void);

   
                                                                               
                                                                             
               
  
                                                               
                                                                       
                                                                       
                                                         
  
                                      
                                                                               
                                                                       
  
                                                                 
                                                      
                                                                 
   
uint32_t mbedtls_psa_sign_hash_get_num_ops(
    const mbedtls_psa_sign_hash_interruptible_operation_t *operation);

   
                                                                                
                                                                          
                      
  
                                                               
                                                                         
                                                                         
                                                         
  
                                      
                                                                                 
                                                                       
  
                                                                 
                                                      
                                                                   
   
uint32_t mbedtls_psa_verify_hash_get_num_ops(
    const mbedtls_psa_verify_hash_interruptible_operation_t *operation);

   
                                                                          
                                
  
                                                               
                                                                
                                                                           
                                               
  
                                      
                                                                               
                                                                       
                                                                        
                                          
                                                                      
                                                                          
                                                                             
                                                    
                                                            
                                                                    
  
                       
                                                                                
                                                          
                                      
                                                                             
                
                                                                               
                                                                             
                                         
                                                                        
   
psa_status_t mbedtls_psa_sign_hash_start(
    mbedtls_psa_sign_hash_interruptible_operation_t *operation,
    const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
    size_t key_buffer_size, psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length);

   
                                                                          
                                                                       
  
                                                               
                                                                   
                                                                              
                                               
  
                                      
                                                                               
                                                                       
  
                                                                            
                                                                              
                                                                    
                                                  
                                                                            
                                                             
  
                       
                                           
  
                                    
                                                             
                                                                           
                                                                          
  
                                      
                                                                    
                                                        
                                                                  
                                                                      
                                  
  
                                                     
                                                        
                                                           
                                                           
                                                            
   
psa_status_t mbedtls_psa_sign_hash_complete(
    mbedtls_psa_sign_hash_interruptible_operation_t *operation,
    uint8_t *signature, size_t signature_size,
    size_t *signature_length);

   
                                      
  
                                                                               
                                                                               
                                                                          
                 
  
                                      
                                                                               
                                         
  
                       
                                                  
   
psa_status_t mbedtls_psa_sign_hash_abort(
    mbedtls_psa_sign_hash_interruptible_operation_t *operation);

   
                                                                     
                                
  
                                                               
                                                                  
                                                                             
                                               
  
                                      
                                                                                 
                                                                       
                                                                        
                                          
                                                                      
                                                                          
                                                                             
                                                    
                                                                           
                                                                    
                                                                          
                                                                         
  
                       
                                                                                
                                                          
                                      
                                                            
                                   
                                                                              
                                                    
                                         
                                                                              
                                      
   
psa_status_t mbedtls_psa_verify_hash_start(
    mbedtls_psa_verify_hash_interruptible_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length);

   
                                                                          
                                                                       
  
                                                               
                                                                   
                                                                              
                                               
  
                                      
                                                                               
                                                                       
  
                       
                                                                               
  
                                    
                                                             
                                                                           
                                                                          
  
                                       
                                                                     
                                              
  
                                                     
                                                        
                                                           
   
psa_status_t mbedtls_psa_verify_hash_complete(
    mbedtls_psa_verify_hash_interruptible_operation_t *operation);

   
                                               
  
                                                               
                                                                  
                                                                             
                                               
  
                                      
                                                                                 
                                         
  
                       
                                                  
   
psa_status_t mbedtls_psa_verify_hash_abort(
    mbedtls_psa_verify_hash_interruptible_operation_t *operation);

typedef struct psa_crypto_local_input_s {
    uint8_t *buffer;
    size_t length;
} psa_crypto_local_input_t;

#define PSA_CRYPTO_LOCAL_INPUT_INIT ((psa_crypto_local_input_t) { NULL, 0 })

                                                                           
  
                                                        
                                                           
                                                                            
                                                              
                                                                            
                                       
                                                                            
                                                               
   
psa_status_t psa_crypto_local_input_alloc(const uint8_t *input, size_t input_len,
                                          psa_crypto_local_input_t *local_input);

                                         
  
                                                                            
                                                               
                                                               
   
void psa_crypto_local_input_free(psa_crypto_local_input_t *local_input);

typedef struct psa_crypto_local_output_s {
    uint8_t *original;
    uint8_t *buffer;
    size_t length;
} psa_crypto_local_output_t;

#define PSA_CRYPTO_LOCAL_OUTPUT_INIT ((psa_crypto_local_output_t) { NULL, NULL, 0 })

                                              
  
                                                                             
                                                                         
                                                                         
                                                            
                                                               
  
                                                         
                                                            
                                                                                
                                                                    
                                                                            
                                       
                                                                            
                                                               
   
psa_status_t psa_crypto_local_output_alloc(uint8_t *output, size_t output_len,
                                           psa_crypto_local_output_t *local_output);

                                                                         
                        
  
                                                                             
                                                               
                                                                
                                                                     
                                                                         
                                                                             
                                                                         
   
psa_status_t psa_crypto_local_output_free(psa_crypto_local_output_t *local_output);

#endif                        
