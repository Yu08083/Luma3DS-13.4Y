  
                                                    
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_MAC_H
#define PSA_CRYPTO_MAC_H

#include <psa/crypto.h>

                                                                                
  
                                                                           
                                                                           
                                                                          
                 
  
                                                                        
                                          
                                                                        
                                                                               
                                                                      
                                                                          
                                                                          
                                                                              
                                                                           
                                                                    
                                                                     
                                                                            
                                                                   
                                                               
                                                           
  
                       
                   
                                   
                                   
                                      
                                   
                                                           
                                                           
   
psa_status_t mbedtls_psa_mac_compute(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *input,
    size_t input_length,
    uint8_t *mac,
    size_t mac_size,
    size_t *mac_length);

                                                                
  
                                                                              
                                                                              
                                                                          
                 
  
                                                                            
                                                                    
                                                                        
                                          
                                                                        
                                                                               
                                                                      
                                                                          
                                                                          
                                                                              
                                                                           
  
                       
                   
                                   
                                   
                                                           
                                                           
                               
                                                                  
   
psa_status_t mbedtls_psa_mac_sign_setup(
    mbedtls_psa_mac_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg);

                                                                 
  
                                                                                
                                                                                
                                                                          
                 
  
                                                                            
                                                                    
                                                                        
                                          
                                                                        
                                                                               
                                                                      
                                                                          
                                                                          
                                                                              
                                                                           
  
                       
                   
                                   
                                   
                                                           
                                                           
                               
                                                                  
   
psa_status_t mbedtls_psa_mac_verify_setup(
    mbedtls_psa_mac_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg);

                                                                       
  
                                                                          
                                                                          
                                                                          
                 
  
                                                     
                                                               
  
                                                                    
                                                
  
                                                 
                                                                            
                                                
                                                                 
  
                       
                   
                               
                                                                
                                                           
                                                           
   
psa_status_t mbedtls_psa_mac_update(
    mbedtls_psa_mac_operation_t *operation,
    const uint8_t *input,
    size_t input_length);

                                                                  
  
                                                                               
                                                                               
                                                                          
                 
  
                                                                                
                                                                          
                                                                    
  
                                                                               
                                                           
  
                                                 
                                                                        
                                                                                
                                                                              
                                                                   
                                                                                
                                                                        
                                                  
                                                                            
                                                                               
                                        
  
                       
                   
                               
                                                                          
                      
                                      
                                                                               
                                                         
                                                           
                                                           
   
psa_status_t mbedtls_psa_mac_sign_finish(
    mbedtls_psa_mac_operation_t *operation,
    uint8_t *mac,
    size_t mac_size,
    size_t *mac_length);

                                                                      
                                    
  
                                                               
                                                                  
                                                                             
                                               
  
                                                                        
                                                                      
                                                        
                                                                         
                                                       
  
                                                                               
                                                           
  
                                                 
                                                                     
                                                                              
                                                                           
                                                                               
                                                              
  
                       
                                                                          
                                       
                                                                     
                                         
                               
                                                                            
                      
                                                           
                                                           
   
psa_status_t mbedtls_psa_mac_verify_finish(
    mbedtls_psa_mac_operation_t *operation,
    const uint8_t *mac,
    size_t mac_length);

                                         
  
                                                                      
                                                                    
                                                 
                                                                        
  
                                                                              
                                                      
                                
  
                                                                              
                                                   
                                                                               
                 
  
                                                      
  
                                         
                                                           
   
psa_status_t mbedtls_psa_mac_abort(
    mbedtls_psa_mac_operation_t *operation);

#endif                       
