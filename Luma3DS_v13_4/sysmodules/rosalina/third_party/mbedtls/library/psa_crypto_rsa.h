  
                                           
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_RSA_H
#define PSA_CRYPTO_RSA_H

#include <psa/crypto.h>
#include <mbedtls/rsa.h>

                                                                         
  
                                                                 
                                                                             
                                                         
                                                                           
                                                                          
                                                                          
                                      
   
psa_status_t mbedtls_psa_rsa_load_representation(psa_key_type_t type,
                                                 const uint8_t *data,
                                                 size_t data_length,
                                                 mbedtls_rsa_context **p_rsa);

                                       
  
                                                               
                                                                       
                                                                             
                             
  
                                                                     
                                                                            
                                       
                                                                    
                                                                            
                                       
                                                                               
                                                                           
                                                                      
                                                      
                                                               
  
                                                               
                                      
                                                   
                                                     
                                                           
                                                           
   
psa_status_t mbedtls_psa_rsa_import_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key_buffer, size_t key_buffer_size,
    size_t *key_buffer_length, size_t *bits);

                                              
  
                                                                      
                                                                                
                                                   
                                                                 
                                                                  
   
psa_status_t mbedtls_psa_rsa_export_key(psa_key_type_t type,
                                        mbedtls_rsa_context *rsa,
                                        uint8_t *data,
                                        size_t data_size,
                                        size_t *data_length);

                                                                           
           
  
                                                               
                                                                   
                                                                             
                       
  
                                                                     
                                                                         
                                                                          
                                                                           
                                                                    
                                                                          
                                        
  
                                                                      
                                                     
                                                             
                                                        
                                                           
                                                       
                                                           
   
psa_status_t mbedtls_psa_rsa_export_public_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    uint8_t *data, size_t data_size, size_t *data_length);

   
                              
  
                                                                             
                                                             
                                                               
                                                             
                                                                    
                                                                  
                                                                
                                                                             
                                                                 
                                                                            
                                                
  
                       
                                              
                                   
                                            
                                      
                                                  
   
psa_status_t mbedtls_psa_rsa_generate_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *custom_data, size_t custom_data_length,
    uint8_t *key_buffer, size_t key_buffer_size, size_t *key_buffer_length);

                                                            
  
                                                               
                                                                    
                                                                             
                             
  
                                                                            
                                          
                                                                          
                                       
                                                                          
                                                                             
                                           
                                                            
                                                                    
                                                                            
                                                                         
                                                               
                                                                          
  
                                         
                                      
                                                                    
                                                        
                                                                           
                                                                    
                                                     
                                                        
                                                           
                                                           
                                                            
   
psa_status_t mbedtls_psa_rsa_sign_hash(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
    uint8_t *signature, size_t signature_size, size_t *signature_length);

   
                                                                              
  
                                                               
                                                                        
                                                                             
                             
  
                                                                            
                                          
                                                                          
                                       
                                                                          
                                                                             
                                           
                                                                            
                                         
                                                                    
                                                                          
                                                                         
  
                       
                                  
                                       
                                                                     
                                              
                                                     
                                                        
                                                           
   
psa_status_t mbedtls_psa_rsa_verify_hash(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length);

   
                                                    
  
                                                                     
                                                                           
                                                                          
                                                                     
                                                                    
                                                     
                                                                   
                                                   
                                                                     
                                                                        
                                             
  
                                                                             
                                            
                                                                    
                                                              
                                                                        
                                           
                                                                      
                                                               
                                                                 
  
                                         
                                      
                                                                 
                                                        
                                                                                
                                                                      
                                  
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                            
                               
                                                                                
                                                                         
                                      
   
psa_status_t mbedtls_psa_asymmetric_encrypt(const psa_key_attributes_t *attributes,
                                            const uint8_t *key_buffer,
                                            size_t key_buffer_size,
                                            psa_algorithm_t alg,
                                            const uint8_t *input,
                                            size_t input_length,
                                            const uint8_t *salt,
                                            size_t salt_length,
                                            uint8_t *output,
                                            size_t output_size,
                                            size_t *output_length);

   
                                                     
  
                                                                     
                                                                           
                                                                          
                                                       
                                                                     
                                                                    
                                                     
                                                                   
                                                   
                                                                     
                                                                        
                                             
  
                                                                             
                                            
                                                                    
                                                              
                                                                        
                                           
                                                                      
                                                               
                                                                 
  
                                         
                                      
                                                                 
                                                        
                                                                                
                                                                      
                                  
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                            
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t mbedtls_psa_asymmetric_decrypt(const psa_key_attributes_t *attributes,
                                            const uint8_t *key_buffer,
                                            size_t key_buffer_size,
                                            psa_algorithm_t alg,
                                            const uint8_t *input,
                                            size_t input_length,
                                            const uint8_t *salt,
                                            size_t salt_length,
                                            uint8_t *output,
                                            size_t output_size,
                                            size_t *output_length);

#endif                       
