  
                                           
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_ECP_H
#define PSA_CRYPTO_ECP_H

#include <psa/crypto.h>
#include <mbedtls/ecp.h>

                                                                         
  
                                                                 
                                                              
                                                                         
                                              
                                                                     
                                                                              
                                                                        
                                                                        
                                                          
                                                                             
                                                         
                                                                           
                                                                          
                                                                          
                                      
   
psa_status_t mbedtls_psa_ecp_load_representation(psa_key_type_t type,
                                                 size_t curve_bits,
                                                 const uint8_t *data,
                                                 size_t data_length,
                                                 mbedtls_ecp_keypair **p_ecp);

                                                         
  
                                                                        
  
                                                          
   

psa_status_t mbedtls_psa_ecp_load_public_part(mbedtls_ecp_keypair *ecp);

                                       
  
                                                               
                                                                       
                                                                             
                             
  
                                                                     
                                                                            
                                       
                                                                    
                                                                            
                                       
                                                                               
                                                                           
                                                                      
                                                      
                                                               
  
                                                               
                                      
                                                   
                                                     
                                                           
                                                           
   
psa_status_t mbedtls_psa_ecp_import_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key_buffer, size_t key_buffer_size,
    size_t *key_buffer_length, size_t *bits);

                                              
  
                                                                      
                                                                                
                                                   
                                                                 
                                                                  
   
psa_status_t mbedtls_psa_ecp_export_key(psa_key_type_t type,
                                        mbedtls_ecp_keypair *ecp,
                                        uint8_t *data,
                                        size_t data_size,
                                        size_t *data_length);

                                                                            
           
  
                                                               
                                                                   
                                                                             
                       
  
                                                                     
                                                                         
                                                                          
                                                                           
                                                                    
                                                                          
                                       
  
                                                                      
                                                     
                                                             
                                                        
                                                           
                                                       
                                                           
   
psa_status_t mbedtls_psa_ecp_export_public_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    uint8_t *data, size_t data_size, size_t *data_length);

   
                              
  
                                                                           
                     
  
                                                                             
                                                                             
                                                                 
                                                                            
                                                
  
                       
                                              
                                   
                                            
                                      
                                                  
   
psa_status_t mbedtls_psa_ecp_generate_key(
    const psa_key_attributes_t *attributes,
    uint8_t *key_buffer, size_t key_buffer_size, size_t *key_buffer_length);

                                               
  
                                                               
                                                                    
                                                                             
                             
  
                                                                            
                                          
                                                                          
                                       
                                                                          
                                                                            
                                                            
                                                                    
                                                                            
                                                                         
                                                               
                                                                          
  
                                         
                                      
                                                                    
                                                        
                                                                           
                                                                    
                                                     
                                                        
                                                           
                                                           
                                                            
   
psa_status_t mbedtls_psa_ecdsa_sign_hash(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
    uint8_t *signature, size_t signature_size, size_t *signature_length);

   
                                                          
  
                                                               
                                                                        
                                                                             
                             
  
                                                                            
                                          
                                                                          
                                       
                                                                          
                                                                            
                                                                            
                                         
                                                                    
                                                                          
                                                                         
  
                       
                                  
                                       
                                                                     
                                              
                                                     
                                                        
                                                           
   
psa_status_t mbedtls_psa_ecdsa_verify_hash(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length);


                                                                  
  
                                                               
                                                                            
                                                                             
                             
  
                                                                            
                                              
                                                                         
                                            
                                                                       
                                          
                                                                     
                                                                        
                                                                         
                                                             
                                                                     
                                          
                                                                         
                                                     
                                                                          
                                          
                                                                             
                                                                  
                       
                                                          
                                                      
                                                     
                                      
                                                      
                                                        
                                                                        
                          
                                      
                                             
                                   
                                                             
                                                           
                                                           
   
psa_status_t mbedtls_psa_key_agreement_ecdh(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *peer_key, size_t peer_key_length,
    uint8_t *shared_secret, size_t shared_secret_size,
    size_t *shared_secret_length);
#endif                       
