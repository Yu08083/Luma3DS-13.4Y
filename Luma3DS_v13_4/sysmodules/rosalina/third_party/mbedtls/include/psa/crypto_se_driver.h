   
                               
                                                    
  
                                                                      
                                                          
                                                                               
                                                                      
                  
  
                                                                               
                                                                             
                                                               
                                                                        
                                                                              
   

/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */
#ifndef PSA_CRYPTO_SE_DRIVER_H
#define PSA_CRYPTO_SE_DRIVER_H
#include "mbedtls/private_access.h"

#include "crypto_driver_common.h"

#ifdef __cplusplus
extern "C" {
#endif

                                                          
   
       

                                   
  
                                                        
                                                             
  
                                                             
                            
   
typedef struct {
                                                            
      
                                                                  
                                                                  
                                                                   
                                                                    
                                                                  
                                                               
                       
      
                                                                  
                                                                          
                                                                    
                                                                
      
                                                                         
                      
      
                                                                          
                                                                           
                                                                             
                                                                            
                                    
      
                                                                         
                                                                       
                                             
                             
                                                
                                               
      
                                                                   
                                                                            
                                                                         
                          
                                                                              
                                                                     
                                                 
                                               
                                                                         
                                                                        
                                           
                                                
       
    const void *const MBEDTLS_PRIVATE(persistent_data);

                                                
      
                                                                            
                                                                     
       
    const size_t MBEDTLS_PRIVATE(persistent_data_size);

                              
      
                                                                          
                                                                        
       
    uintptr_t MBEDTLS_PRIVATE(transient_data);
} psa_drv_se_context_t;

                                            
  
                                                                 
                                                                    
                                                        
                                                                            
                                                                             
                                                                          
                                             
  
                       
                                     
                                                               
          
                                                                        
                        
                                                                   
   
typedef psa_status_t (*psa_drv_se_init_t)(psa_drv_se_context_t *drv_context,
                                          void *persistent_data,
                                          psa_key_location_t location);

#if defined(__DOXYGEN_ONLY__) || !defined(MBEDTLS_PSA_CRYPTO_SE_C)
                                                                    
                                                                       
                                     
                                                                    
                             
                                                                      
                                                                      
                          
typedef uint64_t psa_key_slot_number_t;
#endif                                                   

       

                                                                
                                                                             
                                                                         
                                                                             
                                      
                             
                              
                              
        
                                                                  
  
                                                                               
                                                                             
                                                                      
            
   
       
                                                                           
                               
  
                                                             
                                                                 
                                                             
                                                                      
                                         
                                                                           
                                         
  
                        
                    
   
typedef psa_status_t (*psa_drv_se_mac_setup_t)(psa_drv_se_context_t *drv_context,
                                               void *op_context,
                                               psa_key_slot_number_t key_slot,
                                               psa_algorithm_t algorithm);

                                                                            
            
  
                                                                     
                                                                          
                                       
                                                                              
                                                    
                                                                             
   
typedef psa_status_t (*psa_drv_se_mac_update_t)(void *op_context,
                                                const uint8_t *p_input,
                                                size_t input_length);

                                                                            
                                            
  
                                                                     
                                                                      
                                        
                                                                        
                                      
                                                                             
                                                                 
                                                                            
                                                                  
  
                        
                    
   
typedef psa_status_t (*psa_drv_se_mac_finish_t)(void *op_context,
                                                uint8_t *p_mac,
                                                size_t mac_size,
                                                size_t *p_mac_length);

                                                                            
                                                                    
  
                                                                                
                                                                    
                                                                             
                                                        
                                                                                
  
                       
                                                                         
                
                                       
                                                                           
                                     
   
typedef psa_status_t (*psa_drv_se_mac_finish_verify_t)(void *op_context,
                                                       const uint8_t *p_mac,
                                                       size_t mac_length);

                                                                       
            
  
                                                                                
                                                                   
   
typedef psa_status_t (*psa_drv_se_mac_abort_t)(void *op_context);

                                                                         
                                         
  
                                                             
                                                                           
                                                              
                                                              
                                                                            
                                         
                                                                        
                                      
                                                                       
                                                                            
                                                                   
  
                       
                   
   
typedef psa_status_t (*psa_drv_se_mac_generate_t)(psa_drv_se_context_t *drv_context,
                                                  const uint8_t *p_input,
                                                  size_t input_length,
                                                  psa_key_slot_number_t key_slot,
                                                  psa_algorithm_t alg,
                                                  uint8_t *p_mac,
                                                  size_t mac_size,
                                                  size_t *p_mac_length);

                                                                         
                                                                  
  
                                                                 
                                                                       
                                                        
                                                          
                                                                        
                                     
                                                                              
                                               
                                                     
  
                       
                                                                         
                
                                       
                                                                           
                                     
   
typedef psa_status_t (*psa_drv_se_mac_verify_t)(psa_drv_se_context_t *drv_context,
                                                const uint8_t *p_input,
                                                size_t input_length,
                                                psa_key_slot_number_t key_slot,
                                                psa_algorithm_t alg,
                                                const uint8_t *p_mac,
                                                size_t mac_length);

                                                                     
                                        
  
                                                                          
                
  
                                                      
                                                          
  
                                                                             
                                                                            
                                                                    
                                                                   
  
   
typedef struct {
                                                                            
                
       
    size_t                    MBEDTLS_PRIVATE(context_size);
                                                    
       
    psa_drv_se_mac_setup_t          MBEDTLS_PRIVATE(p_setup);
                                                     
       
    psa_drv_se_mac_update_t         MBEDTLS_PRIVATE(p_update);
                                               
       
    psa_drv_se_mac_finish_t         MBEDTLS_PRIVATE(p_finish);
                                                                   
       
    psa_drv_se_mac_finish_verify_t  MBEDTLS_PRIVATE(p_finish_verify);
                                                               
       
    psa_drv_se_mac_abort_t          MBEDTLS_PRIVATE(p_abort);
                                                          
       
    psa_drv_se_mac_generate_t       MBEDTLS_PRIVATE(p_mac);
                                                                     
       
    psa_drv_se_mac_verify_t         MBEDTLS_PRIVATE(p_mac_verify);
} psa_drv_se_mac_t;
       

                                                        
  
                                                                           
                                                                     
                                
                                                                      
                                 
                                 
        
                                 
  
                                                                      
                                                                               
                                                                         
                      
  
                                                                              
                                                                            
                                                                              
                             
   
       

                                                                   
                        
  
                                                             
                                                                 
                                                                 
                                                                      
                                         
                                                                      
                                         
                                                                             
                                          
  
                                         
                                                     
   
typedef psa_status_t (*psa_drv_se_cipher_setup_t)(psa_drv_se_context_t *drv_context,
                                                  void *op_context,
                                                  psa_key_slot_number_t key_slot,
                                                  psa_algorithm_t algorithm,
                                                  psa_encrypt_or_decrypt_t direction);

                                                             
                                                   
  
                                                                              
                                                                              
                                                                             
                                                                      
  
                                                                               
                                                                
                                                                             
                                                                        
  
                                         
   
typedef psa_status_t (*psa_drv_se_cipher_set_iv_t)(void *op_context,
                                                   const uint8_t *p_iv,
                                                   size_t iv_length);

                                                                               
            
  
                                                                         
                                                                       
                                                                      
                                                       
                                                                              
                                                
                                                                         
                                                         
                                                                      
                                                     
                                                                             
                                                                            
  
                                         
   
typedef psa_status_t (*psa_drv_se_cipher_update_t)(void *op_context,
                                                   const uint8_t *p_input,
                                                   size_t input_size,
                                                   uint8_t *p_output,
                                                   size_t output_size,
                                                   size_t *p_output_length);

                                                                               
            
  
                                                                     
                                                                   
                                                                            
                                              
                                                                             
                                      
                                                                            
                                                                     
  
                                         
   
typedef psa_status_t (*psa_drv_se_cipher_finish_t)(void *op_context,
                                                   uint8_t *p_output,
                                                   size_t output_size,
                                                   size_t *p_output_length);

                                                                            
            
  
                                                                     
                                                                   
   
typedef psa_status_t (*psa_drv_se_cipher_abort_t)(void *op_context);

                                                                         
                    
  
                                                                           
                                           
  
                                                             
                                                                                
                                                                                
                                                                                
                                       
                                                                  
                                                   
                                                                             
                                         
                                                                            
                                              
                                                                             
                                      
  
                                         
                                                     
   
typedef psa_status_t (*psa_drv_se_cipher_ecb_t)(psa_drv_se_context_t *drv_context,
                                                psa_key_slot_number_t key_slot,
                                                psa_algorithm_t algorithm,
                                                psa_encrypt_or_decrypt_t direction,
                                                const uint8_t *p_input,
                                                size_t input_size,
                                                uint8_t *p_output,
                                                size_t output_size);

   
                                                                              
                                           
  
                                                                           
                                             
  
                                                      
                                                        
   
typedef struct {
                                                                        
                        
       
    size_t               MBEDTLS_PRIVATE(context_size);
                                                          
    psa_drv_se_cipher_setup_t  MBEDTLS_PRIVATE(p_setup);
                                                        
    psa_drv_se_cipher_set_iv_t MBEDTLS_PRIVATE(p_set_iv);
                                                           
    psa_drv_se_cipher_update_t MBEDTLS_PRIVATE(p_update);
                                                     
    psa_drv_se_cipher_finish_t MBEDTLS_PRIVATE(p_finish);
                                                  
    psa_drv_se_cipher_abort_t  MBEDTLS_PRIVATE(p_abort);
                                                              
                                                                          
                         
       
    psa_drv_se_cipher_ecb_t    MBEDTLS_PRIVATE(p_ecb);
} psa_drv_se_cipher_t;

       

                                                                  
  
                                                                             
                                                                              
                                                                  
   
       

   
                                                                             
                   
  
                                                                 
                                                                      
                                                                            
                                                          
                                                    
                                                                        
                                                                               
                                                                             
                                                                   
                                                                             
  
                                         
   
typedef psa_status_t (*psa_drv_se_asymmetric_sign_t)(psa_drv_se_context_t *drv_context,
                                                     psa_key_slot_number_t key_slot,
                                                     psa_algorithm_t alg,
                                                     const uint8_t *p_hash,
                                                     size_t hash_length,
                                                     uint8_t *p_signature,
                                                     size_t signature_size,
                                                     size_t *p_signature_length);

   
                                                                              
                                               
  
                                                             
                                                                             
                                    
                                                                             
                                                 
                                                                          
                                                                    
                                                                         
                                                                         
  
                       
                                  
   
typedef psa_status_t (*psa_drv_se_asymmetric_verify_t)(psa_drv_se_context_t *drv_context,
                                                       psa_key_slot_number_t key_slot,
                                                       psa_algorithm_t alg,
                                                       const uint8_t *p_hash,
                                                       size_t hash_length,
                                                       const uint8_t *p_signature,
                                                       size_t signature_length);

   
                                                                            
                          
  
                                                             
                                                                             
                                    
                                                                          
                                                                 
                                                      
                                                                     
                                                                    
                                                    
                                                                   
                                                  
                                                                     
                                                                        
                                            
                                                                           
                                          
                                                                    
                                                              
                                                                        
                                          
                                                                      
                                                                            
                                                   
  
                                         
   
typedef psa_status_t (*psa_drv_se_asymmetric_encrypt_t)(psa_drv_se_context_t *drv_context,
                                                        psa_key_slot_number_t key_slot,
                                                        psa_algorithm_t alg,
                                                        const uint8_t *p_input,
                                                        size_t input_length,
                                                        const uint8_t *p_salt,
                                                        size_t salt_length,
                                                        uint8_t *p_output,
                                                        size_t output_size,
                                                        size_t *p_output_length);

   
                                                                             
                           
  
                                                             
                                                                  
                                                                          
                                                                 
                                                      
                                                                     
                                                                    
                                                    
                                                                   
                                                  
                                                                     
                                                                        
                                            
                                                                           
                                          
                                                                    
                                                              
                                                                        
                                          
                                                                      
                                                               
                                                                
  
                                         
   
typedef psa_status_t (*psa_drv_se_asymmetric_decrypt_t)(psa_drv_se_context_t *drv_context,
                                                        psa_key_slot_number_t key_slot,
                                                        psa_algorithm_t alg,
                                                        const uint8_t *p_input,
                                                        size_t input_length,
                                                        const uint8_t *p_salt,
                                                        size_t salt_length,
                                                        uint8_t *p_output,
                                                        size_t output_size,
                                                        size_t *p_output_length);

   
                                                                              
                                                             
  
                                                                           
                                             
  
                                                                        
   
typedef struct {
                                                              
    psa_drv_se_asymmetric_sign_t    MBEDTLS_PRIVATE(p_sign);
                                                                
    psa_drv_se_asymmetric_verify_t  MBEDTLS_PRIVATE(p_verify);
                                                                 
    psa_drv_se_asymmetric_encrypt_t MBEDTLS_PRIVATE(p_encrypt);
                                                                 
    psa_drv_se_asymmetric_decrypt_t MBEDTLS_PRIVATE(p_decrypt);
} psa_drv_se_asymmetric_t;

       

                                                                                  
                                                                              
                                                                              
                                                                          
                                                                           
                                                                              
   
       

                                                                             
            
  
                                                                     
                                                                       
                                                                     
                                                                       
                                                                        
                                                          
                                                                             
                                                                    
                                                                       
                                                                            
                                                                           
                                                 
                                                                      
                                                                               
                                                                              
                                                                               
                                                                        
                                                                         
                                                                            
                                                                              
                                                                            
                                             
                                                                             
                                                                 
  
                       
                   
   
typedef psa_status_t (*psa_drv_se_aead_encrypt_t)(psa_drv_se_context_t *drv_context,
                                                  psa_key_slot_number_t key_slot,
                                                  psa_algorithm_t algorithm,
                                                  const uint8_t *p_nonce,
                                                  size_t nonce_length,
                                                  const uint8_t *p_additional_data,
                                                  size_t additional_data_length,
                                                  const uint8_t *p_plaintext,
                                                  size_t plaintext_length,
                                                  uint8_t *p_ciphertext,
                                                  size_t ciphertext_size,
                                                  size_t *p_ciphertext_length);

                                                                                
  
                                                                     
                                                                      
                                                                     
                                                                       
                                                                        
                                                          
                                                                             
                                                                     
                                                                       
                                                                            
                                                                            
                                                  
                                                                               
                                                                              
                                                                           
                                                                              
                                                               
                                                                       
                                                                            
                                                                           
                                             
                                                                             
                                                                
  
                       
                   
   
typedef psa_status_t (*psa_drv_se_aead_decrypt_t)(psa_drv_se_context_t *drv_context,
                                                  psa_key_slot_number_t key_slot,
                                                  psa_algorithm_t algorithm,
                                                  const uint8_t *p_nonce,
                                                  size_t nonce_length,
                                                  const uint8_t *p_additional_data,
                                                  size_t additional_data_length,
                                                  const uint8_t *p_ciphertext,
                                                  size_t ciphertext_length,
                                                  uint8_t *p_plaintext,
                                                  size_t plaintext_size,
                                                  size_t *p_plaintext_length);

   
                                                                              
                                                                          
  
                                                                           
                            
  
                                                                        
   
typedef struct {
                                                            
    psa_drv_se_aead_encrypt_t MBEDTLS_PRIVATE(p_encrypt);
                                                            
    psa_drv_se_aead_decrypt_t MBEDTLS_PRIVATE(p_decrypt);
} psa_drv_se_aead_t;
       

                                                             
                                                                       
                                                    
                                                                           
                   
   
       

                                                   
   
typedef enum {
    PSA_KEY_CREATION_IMPORT,                                
    PSA_KEY_CREATION_GENERATE,                                  
    PSA_KEY_CREATION_DERIVE,                                               
    PSA_KEY_CREATION_COPY,                              

#ifndef __DOXYGEN_ONLY__
                                                                     
      
                                         
                                                                  
                                                           
                                                                        
                                                                           
                                                                           
            
      
                                                                        
                                                                             
                                                                           
                                                
      
                                     
       
    PSA_KEY_CREATION_REGISTER,
#endif
} psa_key_creation_method_t;

                                                      
  
                                                                   
                                                              
                                                                 
                                                       
                                                                          
                                                                     
             
                                                            
                                 
                                                                      
                                                                            
                                          
                                                           
  
                                                              
                                   
                                                                     
                                                          
                                     
                                                                       
                                                          
                                                  
                                                                     
                                                          
                                 
                                                                   
                                                          
  
                                                   
                                                                 
                                                            
                                                                  
                                                            
                                         
                                                                   
                                                                             
                                          
                                                                   
                                                                             
  
                                                                          
                                                                      
                                                                       
                                                                      
                                                                              
  
                                                                 
                                                                    
                                                        
                                                          
                                                                              
                                                                      
                                                                              
                                                                       
  
                       
                   
                                                                          
                                                                    
                                                     
                                                            
   
typedef psa_status_t (*psa_drv_se_allocate_key_t)(
    psa_drv_se_context_t *drv_context,
    void *persistent_data,
    const psa_key_attributes_t *attributes,
    psa_key_creation_method_t method,
    psa_key_slot_number_t *key_slot);

                                                                    
             
  
                                                                   
                                                                   
                                                                 
                                                                        
  
                                                                            
                                                                    
                                                                     
                                                                       
                                                                          
                                                                          
  
                                                                
                                                                            
                                                          
                                                    
  
                                                             
                                                                    
                                                        
                                                      
                                                                          
                                                                   
  
                       
                                                                  
                      
                                      
                                                                
                                                                  
                                      
                                    
                                                                 
                                                               
                                     
   
typedef psa_status_t (*psa_drv_se_validate_slot_number_t)(
    psa_drv_se_context_t *drv_context,
    void *persistent_data,
    const psa_key_attributes_t *attributes,
    psa_key_creation_method_t method,
    psa_key_slot_number_t key_slot);

                                                                               
  
                                                                           
                                                                      
  
                                                             
                                                                  
                                                                          
                                                                   
                                                                           
                                                                  
                                                                             
                                                                       
                                                       
                                                                        
                                                   
                                                             
                                                                      
                                            
                                                               
                                                                    
                                                                            
                                                                           
                                                              
                                                                             
  
                       
                   
   
typedef psa_status_t (*psa_drv_se_import_key_t)(
    psa_drv_se_context_t *drv_context,
    psa_key_slot_number_t key_slot,
    const psa_key_attributes_t *attributes,
    const uint8_t *data,
    size_t data_length,
    size_t *bits);

   
                                                                               
                    
  
                                                                       
                                                                               
                                
  
                                                                 
  
                                                                 
                                                                    
                                                        
                                                          
  
                       
                                                       
   
typedef psa_status_t (*psa_drv_se_destroy_key_t)(
    psa_drv_se_context_t *drv_context,
    void *persistent_data,
    psa_key_slot_number_t key_slot);

   
                                                                       
  
                                                                   
                               
  
                                                                     
                                                                 
                                                                      
                                            
  
                                                               
                                        
                                                                        
  
                                                             
                                                                               
                                                        
                                                                           
                                                                     
                                                               
                                                          
  
                                         
                                                      
                                                     
                                                     
                                                             
                                                        
                                                           
   
typedef psa_status_t (*psa_drv_se_export_key_t)(psa_drv_se_context_t *drv_context,
                                                psa_key_slot_number_t key,
                                                uint8_t *p_data,
                                                size_t data_size,
                                                size_t *p_data_length);

   
                                                                             
          
  
                                                    
                                                            
                                                                  
                                                                     
                           
                                                                 
                                                                      
                                                            
                                        
                                                                    
                                                                  
                                                
  
                                                             
                                                                  
                                                                          
                                                                   
                                                                           
                                                                            
                                                                        
                                                                       
                                                             
                                                                      
                                            
                                                                       
                                                                         
                                         
                                                                           
                                                                   
                                                                            
                                                                         
                                                                     
                                                                   
                                                                            
                                                           
                                                                          
                                                                               
                                                                            
                                                                  
                                                                         
                                                                              
   
typedef psa_status_t (*psa_drv_se_generate_key_t)(
    psa_drv_se_context_t *drv_context,
    psa_key_slot_number_t key_slot,
    const psa_key_attributes_t *attributes,
    uint8_t *pubkey, size_t pubkey_size, size_t *pubkey_length);

   
                                                                               
                         
  
                                                                           
                                             
  
                                                                        
   
typedef struct {
                                                    
    psa_drv_se_allocate_key_t   MBEDTLS_PRIVATE(p_allocate);
                                                                 
    psa_drv_se_validate_slot_number_t MBEDTLS_PRIVATE(p_validate_slot_number);
                                                        
    psa_drv_se_import_key_t     MBEDTLS_PRIVATE(p_import);
                                              
    psa_drv_se_generate_key_t   MBEDTLS_PRIVATE(p_generate);
                                                         
    psa_drv_se_destroy_key_t    MBEDTLS_PRIVATE(p_destroy);
                                                        
    psa_drv_se_export_key_t     MBEDTLS_PRIVATE(p_export);
                                                               
    psa_drv_se_export_key_t     MBEDTLS_PRIVATE(p_export_public);
} psa_drv_se_key_management_t;

       

                                                                           
                                                                        
                                                                    
                                          
                                                                             
                                                                           
            
                                                                                 
                         
  
                                                         
                                                                             
                                                                             
                                                                  
                                                                             
                                                        
  
                                                                            
                                                                             
                                                                            
                                                                             
                                                                            
                                                                             
                  
                    
                                                                                   
                                                                      
                                                       
                                                           
                                                                      
                                                       
                                                           
                                                                      
                                                       
                                                           
                                      
                
  
                         
                    
                                                                         
                                                                           
                                                  
                                                     
                                                         
                
   
       

                                                                               
                                                  
  
                                                             
                                                                            
                                                                          
                                                                               
                                                                             
                                                  
  
                                         
   
typedef psa_status_t (*psa_drv_se_key_derivation_setup_t)(psa_drv_se_context_t *drv_context,
                                                          void *op_context,
                                                          psa_algorithm_t kdf_alg,
                                                          psa_key_slot_number_t source_key);

                                                                               
                                                    
  
                                                                          
                                                                        
                                                                      
  
                                                                            
                                                                          
                                                                       
                                                                       
                                                                   
  
                                         
   
typedef psa_status_t (*psa_drv_se_key_derivation_collateral_t)(void *op_context,
                                                               uint32_t collateral_id,
                                                               const uint8_t *p_collateral,
                                                               size_t collateral_size);

                                                                           
                                                      
  
                                                                            
                                                                          
                                                                         
                                                
  
                                         
   
typedef psa_status_t (*psa_drv_se_key_derivation_derive_t)(void *op_context,
                                                           psa_key_slot_number_t dest_key);

                                                                          
                                                             
  
                                                                          
                                        
                                                               
                                                                             
                                                                 
  
                                         
   
typedef psa_status_t (*psa_drv_se_key_derivation_export_t)(void *op_context,
                                                           uint8_t *p_output,
                                                           size_t output_size,
                                                           size_t *p_output_length);

   
                                                                               
                                       
  
                                                                           
                            
  
                                                                        
   
typedef struct {
                                                                 
    size_t                           MBEDTLS_PRIVATE(context_size);
                                                        
    psa_drv_se_key_derivation_setup_t      MBEDTLS_PRIVATE(p_setup);
                                                       
    psa_drv_se_key_derivation_collateral_t MBEDTLS_PRIVATE(p_collateral);
                                                             
    psa_drv_se_key_derivation_derive_t     MBEDTLS_PRIVATE(p_derive);
                                                                      
                         
    psa_drv_se_key_derivation_export_t     MBEDTLS_PRIVATE(p_export);
} psa_drv_se_key_derivation_t;

       

                                                                
   
       

                                                                
                         
  
                                                                       
                             
   
typedef struct {
                                                                  
                                                                         
                                                 
                                   
       
    uint32_t MBEDTLS_PRIVATE(hal_version);

                                                          
      
                                                                 
      
                                                                     
                                                              
                       
       
    size_t MBEDTLS_PRIVATE(persistent_data_size);

                                           
      
                                                                    
                                                                   
                                                                   
                                                                         
      
                                                                         
                                        
       
    psa_drv_se_init_t MBEDTLS_PRIVATE(p_init);

    const psa_drv_se_key_management_t *MBEDTLS_PRIVATE(key_management);
    const psa_drv_se_mac_t *MBEDTLS_PRIVATE(mac);
    const psa_drv_se_cipher_t *MBEDTLS_PRIVATE(cipher);
    const psa_drv_se_aead_t *MBEDTLS_PRIVATE(aead);
    const psa_drv_se_asymmetric_t *MBEDTLS_PRIVATE(asymmetric);
    const psa_drv_se_key_derivation_t *MBEDTLS_PRIVATE(derivation);
} psa_drv_se_t;

                                                         
   
                        
#define PSA_DRV_SE_HAL_VERSION 0x00000005

                                                                 
  
                                                                   
                                                                   
                                                                 
                                                                    
                                                                     
                                       
  
                                                              
                                                             
                           
  
                                                                         
                                                                          
                                                       
                                                                             
                                                                        
                                                                        
                                      
  
                                                                         
                                                   
                                                                       
                                                                                 
                                                                             
                                                                        
                                                 
                                                                           
                                                                    
                                                                      
                                 
  
                       
                                                                       
                                                                       
                         
                               
                                                                   
                                                                        
                                             
                                    
                                                                              
                                      
                                           
                                   
                                                                          
                                         
                                   
                                     
                                  
   
psa_status_t psa_register_se_driver(
    psa_key_location_t location,
    const psa_drv_se_t *methods);

       

#ifdef __cplusplus
}
#endif

#endif                             
