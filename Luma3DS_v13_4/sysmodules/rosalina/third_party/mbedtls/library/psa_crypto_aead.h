  
                                
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_AEAD_H
#define PSA_CRYPTO_AEAD_H

#include <psa/crypto.h>

   
                                                        
  
                                                               
                                                                           
                                                                             
                             
  
                                                                          
                                            
                                                                        
                                                                            
                                                                
                                                     
                                                                              
                                                                            
                                                           
                                                                            
                                                              
                                                                            
                                                    
                                                                        
                                                                                
                                                             
                                                                         
                                                                            
                                                                               
                                                                               
                                                                  
                                                                       
                                                 
                                                                              
                                                                                
                                          
                                                               
                                                                               
                                                                                
                                           
                                                                     
                                                                              
                                                                         
                                               
                                                                           
                                                    
  
                                
                                   
                                   
                                                           
                                      
                                        
                                                           
   
psa_status_t mbedtls_psa_aead_encrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *nonce, size_t nonce_length,
    const uint8_t *additional_data, size_t additional_data_length,
    const uint8_t *plaintext, size_t plaintext_length,
    uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_length);

   
                                                        
  
                                                               
                                                                           
                                                                             
                             
  
                                                                          
                                            
                                                                        
                                                                            
                                                                
                                                     
                                                                              
                                                                            
                                                           
                                                                            
                                                              
                                                                             
                                                    
                                                                        
                                                                      
                                                                               
                                                                             
                                                                         
                                                                               
                                      
                                                              
                                                                       
                                                                             
                                                                                
                                          
                                                               
                                                                               
                                                                            
                                                
                                                                     
                                                                               
                                                                        
                                               
                                                                           
                                                   
  
                                
                                       
                                       
                                   
                                   
                                                           
                                      
                                       
                                                           
   
psa_status_t mbedtls_psa_aead_decrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *nonce, size_t nonce_length,
    const uint8_t *additional_data, size_t additional_data_length,
    const uint8_t *ciphertext, size_t ciphertext_length,
    uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_length);

                                                                   
  
                                                                
                                                                    
                                                                              
                                               
  
                                                 
                                                                                
                                                          
                                                                    
                                                                          
  
                                                                            
                                                                             
                                                                            
                                    
                                                                        
                                          
                                                                      
                                                                          
                                                                           
                                                          
                                                             
                                                               
                                                                  
  
                       
                   
                                      
                                                
                                   
                                   
                                         
                                                     
   
psa_status_t mbedtls_psa_aead_encrypt_setup(
    mbedtls_psa_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg);

                                                                   
  
                                                               
                                                                    
                                                                              
                                               
  
                                                 
                                                                           
                                                          
                                                                    
                                                                          
  
                                                                            
                                                                             
                                                                            
                                    
                                                                        
                                          
                                                                      
                                                                          
                                                                           
                                                          
                                                             
                                                               
                                                                  
  
                       
                   
                                      
                                                
                                   
                                   
                                         
                                                     
   
psa_status_t mbedtls_psa_aead_decrypt_setup(
    mbedtls_psa_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg);

                                                                          
  
                                                                              
                                                                               
                                                                          
                 
  
                                                     
                                      
  
                                                         
                                                                 
  
                                                                   
                            
  
                                                      
                                                                   
                                                           
  
                       
                   
                                      
                                                                           
                                   
                                                                             
                       
   
psa_status_t mbedtls_psa_aead_set_nonce(
    mbedtls_psa_aead_operation_t *operation,
    const uint8_t *nonce,
    size_t nonce_length);

                                                                    
  
                                                                                
                                                                              
                                                                             
                 
  
                                                                               
                                                                               
                                                                               
                                                                               
  
                                                                             
                                
  
                                                         
                                                                         
                                   
  
                                                               
                            
  
                                                      
                                                                    
                                                            
                                                                          
  
                       
                   
                                      
                                                                       
                     
                                   
                                                                             
                       
   
psa_status_t mbedtls_psa_aead_set_lengths(
    mbedtls_psa_aead_operation_t *operation,
    size_t ad_length,
    size_t plaintext_length);

                                                     
  
                                                                
                                                                               
                                                                             
                             
  
                                                       
  
                                                                        
                                                                         
                                                                     
  
                                                   
                                                        
                                          
                                                         
  
                                                                   
                            
  
                                                      
                                                                 
                                                
                                                                     
  
                       
                   
                                   
                                                                             
                       
   
psa_status_t mbedtls_psa_aead_update_ad(
    mbedtls_psa_aead_operation_t *operation,
    const uint8_t *input,
    size_t input_length);

                                                                      
  
                                                                
                                                                               
                                                                       
                             
  
                                                   
                                                        
                                                                       
                                                                         
                                                         
                                                                           
  
                                                                   
                            
  
                                                                
                                                                    
                                                                       
                                                                      
                                                                        
                                                                          
                                           
  
                                                      
                                                                         
                                                   
                                                                     
                                                                         
                                                                      
                                                                         
                                                    
                                                               
                                                                             
                                                                  
                                                                                
                                                                              
                                              
                                                                       
                                                                          
                                                                     
                                              
                                                               
                                                                 
  
                       
                   
  
                                      
                                                         
                                                                                
                                                                           
                                              
   
psa_status_t mbedtls_psa_aead_update(
    mbedtls_psa_aead_operation_t *operation,
    const uint8_t *input,
    size_t input_length,
    uint8_t *output,
    size_t output_size,
    size_t *output_length);

                                                     
  
                                                                
                                                                               
                                                                       
                             
  
                                                           
                                    
  
                                                                   
                                                                  
                                                                              
                                                                 
  
                                        
                                                                      
                                                  
                                            
  
                                                                               
                                                              
  
                                                      
                                                                            
                                                 
                                                                          
                                                                         
                                                  
                                                             
                                                                           
                                                                              
                                                                               
                                                       
                                                                               
                                                                               
                                            
                                                                   
                                                    
                                                                      
                                              
                                                                   
                                                                         
                                                  
                                                                               
                                                                      
                                                                              
                                                                         
                                                                         
                                                                   
                                                                         
                                                                        
                                            
                                                               
                                                              
  
                       
                   
                                      
                                                      
                                                                 
                                                                              
                       
   
psa_status_t mbedtls_psa_aead_finish(
    mbedtls_psa_aead_operation_t *operation,
    uint8_t *ciphertext,
    size_t ciphertext_size,
    size_t *ciphertext_length,
    uint8_t *tag,
    size_t tag_size,
    size_t *tag_length);

                            
  
                                                                
                                                                             
                                                                       
                             
  
                                                                      
                                                                    
                                                                    
                                                                              
  
                                                                              
                                                                  
  
                                                                               
                                                      
                                                       
  
                                                           
  
                       
                   
   
psa_status_t mbedtls_psa_aead_abort(
    mbedtls_psa_aead_operation_t *operation);

#endif                        
