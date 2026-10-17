  
                                            
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_FFDH_H
#define PSA_CRYPTO_FFDH_H

#include <psa/crypto.h>

                                                              
  
                                                                            
                                              
                                                                         
                                                             
                                                                     
                                          
                                                                         
                                            
                                                                       
                                          
                                                                         
                                                     
                                                                          
                                          
                                                                             
                                                                  
                       
                                                          
                                      
                                                                        
                       
                                                             
                                                             
   
psa_status_t mbedtls_psa_ffdh_key_agreement(
    const psa_key_attributes_t *attributes,
    const uint8_t *peer_key,
    size_t peer_key_length,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    uint8_t *shared_secret,
    size_t shared_secret_size,
    size_t *shared_secret_length);

                                                                             
  
                                                                     
                                                                         
                                                                          
                                                                           
                                                                    
                                                                          
                                       
  
                                                                  
                                      
                                                  
                                                             
                                                             
                                                             
   
psa_status_t mbedtls_psa_ffdh_export_public_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    uint8_t *data,
    size_t data_size,
    size_t *data_length);

   
                          
  
                                                                           
                     
  
                                                                         
                                                                             
                                                                 
                                                                            
                                                
  
                       
                                              
                                   
                                       
                                      
                                                  
                                                             
                                                             
   
psa_status_t mbedtls_psa_ffdh_generate_key(
    const psa_key_attributes_t *attributes,
    uint8_t *key_buffer,
    size_t key_buffer_size,
    size_t *key_buffer_length);

   
                        
  
                                                                         
                     
  
                                                                     
                                                                            
                                       
                                                                    
                                                                            
                                       
                                                                               
                                                                           
                                                                      
                                                      
                                                               
  
                       
                                              
                                      
                                                  
   
psa_status_t mbedtls_psa_ffdh_import_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key_buffer, size_t key_buffer_size,
    size_t *key_buffer_length, size_t *bits);

#endif                        
