  
                                                                     
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_CIPHER_H
#define PSA_CRYPTO_CIPHER_H

#include <mbedtls/cipher.h>
#include <psa/crypto.h>

                                                                             
                                                                          
              
  
                                                               
                                            
                                                                                    
                                                              
                                                    
                                                                    
  
                                                                                  
                                 
                                                                                 
                      
   

psa_status_t mbedtls_cipher_values_from_psa(psa_algorithm_t alg, psa_key_type_t key_type,
                                            size_t *key_bits, mbedtls_cipher_mode_t *mode,
                                            mbedtls_cipher_id_t *cipher_id);

#if defined(MBEDTLS_CIPHER_C)
                                                                             
                                                                          
              
  
                                                          
                                       
                                                  
                                                               
  
                                                                    
                                                                 
   
const mbedtls_cipher_info_t *mbedtls_cipher_info_from_psa(
    psa_algorithm_t alg, psa_key_type_t key_type, size_t key_bits,
    mbedtls_cipher_id_t *cipher_id);
#endif                       

   
                                                                     
  
                                                               
                                                                     
                                                                      
                                                         
  
                                                                           
                                                                        
                                                                           
                                                                        
                                          
                                                                      
                                                                          
                                                               
                                                               
                                                                    
  
                                         
                                                     
                                                           
                                                           
   
psa_status_t mbedtls_psa_cipher_encrypt_setup(
    mbedtls_psa_cipher_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg);

   
                                                                     
  
                                                               
                                                                     
                                                                      
                                                         
  
                                                                           
                                                                        
                                                                           
                                                                        
                                          
                                                                      
                                                                          
                                                               
                                                               
                                                                    
  
                                         
                                                     
                                                           
                                                           
   
psa_status_t mbedtls_psa_cipher_decrypt_setup(
    mbedtls_psa_cipher_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg);

                                                                  
  
                                                           
                                                                       
  
                                                               
                                                              
                                                               
                                                         
  
                                                        
                                                                
                                                                            
                                                               
                                                       
  
                                         
                                      
                                                                        
                                                      
                                                           
   
psa_status_t mbedtls_psa_cipher_set_iv(
    mbedtls_psa_cipher_operation_t *operation,
    const uint8_t *iv, size_t iv_length);

                                                                        
  
                                                               
                                                              
                                                               
                                                         
  
                                                        
                                                                         
                                                   
                                                                     
                                                                         
                                                                      
                                                               
                                                                 
  
                                         
                                      
                                                         
                                                           
   
psa_status_t mbedtls_psa_cipher_update(
    mbedtls_psa_cipher_operation_t *operation,
    const uint8_t *input, size_t input_length,
    uint8_t *output, size_t output_size, size_t *output_length);

                                                                    
  
                                                               
                                                              
                                                               
                                                         
  
                                                        
                                                                         
                                                                      
                                                               
                                                                 
  
                                         
                                      
                                                                         
                                                                           
                                                                         
                                                                
                                     
                                                                        
                                                                      
                                      
                                                         
                                                           
   
psa_status_t mbedtls_psa_cipher_finish(
    mbedtls_psa_cipher_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length);

                             
  
                                                                      
                                                                    
                                       
  
                                                               
                                                             
                                                              
                                                         
  
                                                             
  
                                         
   
psa_status_t mbedtls_psa_cipher_abort(mbedtls_psa_cipher_operation_t *operation);

                                               
  
                                                               
                                                               
                                                                
                                                         
  
                                                                        
                                          
                                                                      
                                                                          
                                                               
                                                               
                                                                    
                                                                            
                                                                  
                                                           
                                                                         
                                                                     
                                                                         
                                                                      
                                                                            
                                                                        
                                            
  
                                         
                                                     
                                                           
                                                           
                                      
                                                         
                                      
                                                                            
                                                      
                                                                         
                                                                           
                                                                         
                                                                
                                     
                                                                        
                                                                      
   
psa_status_t mbedtls_psa_cipher_encrypt(const psa_key_attributes_t *attributes,
                                        const uint8_t *key_buffer,
                                        size_t key_buffer_size,
                                        psa_algorithm_t alg,
                                        const uint8_t *iv,
                                        size_t iv_length,
                                        const uint8_t *input,
                                        size_t input_length,
                                        uint8_t *output,
                                        size_t output_size,
                                        size_t *output_length);

                                               
  
                                                               
                                                               
                                                                
                                                         
  
                                                                        
                                          
                                                                      
                                                                          
                                                               
                                                               
                                                                    
                                                                            
                                                                     
                                                                         
                                                                      
                                                                            
                                                                        
                                            
  
                                         
                                                     
                                                           
                                                           
                                      
                                                         
                                      
                                                                        
                                                      
                                                                         
                                                                           
                                                                         
                                                                
                                     
                                                                        
                                                                      
   
psa_status_t mbedtls_psa_cipher_decrypt(const psa_key_attributes_t *attributes,
                                        const uint8_t *key_buffer,
                                        size_t key_buffer_size,
                                        psa_algorithm_t alg,
                                        const uint8_t *input,
                                        size_t input_length,
                                        uint8_t *output,
                                        size_t output_size,
                                        size_t *output_length);

#endif                          
