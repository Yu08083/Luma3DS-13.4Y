  
                                                        
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_HASH_H
#define PSA_CRYPTO_HASH_H

#include <psa/crypto.h>

                                                                     
  
                                                                            
                                                                            
                                                                          
                 
  
                                                                               
                                                                        
                                                                  
                                                                 
                                                                   
                                                                
                                                           
                                                                       
                                                     
  
                       
                   
                                   
                                  
                                      
                                    
                                                           
                                                           
   
psa_status_t mbedtls_psa_hash_compute(
    psa_algorithm_t alg,
    const uint8_t *input,
    size_t input_length,
    uint8_t *hash,
    size_t hash_size,
    size_t *hash_length);

                                                              
  
                                                                          
                                                                          
                                                                          
                 
  
                                                                               
                                                                             
                                                                         
                        
  
                                                                     
                                                                        
             
                                                                                 
                                        
  
                                                                        
                                                                               
                                                                               
                                                                        
  
                       
                   
                                   
                                  
                               
                                                                  
                                                           
                                                           
   
psa_status_t mbedtls_psa_hash_setup(
    mbedtls_psa_hash_operation_t *operation,
    psa_algorithm_t alg);

                                     
  
                                                                          
                                                                          
                                                                          
                 
  
                                                                 
                                                                      
                                                                           
                                                          
                                                                            
                                                                           
                                                                      
                                              
  
                                                                       
                                                                   
                                                                          
  
                                         
                               
                                                                          
                               
                                                                            
                                                           
                                                           
   
psa_status_t mbedtls_psa_hash_clone(
    const mbedtls_psa_hash_operation_t *source_operation,
    mbedtls_psa_hash_operation_t *target_operation);

                                                                  
  
                                                                           
                                                                           
                                                                          
                 
  
                                                                                   
  
                                                                          
                                                                 
  
                                                  
                                                                           
                                                                 
  
                       
                   
                               
                                                                
                                                           
                                                           
   
psa_status_t mbedtls_psa_hash_update(
    mbedtls_psa_hash_operation_t *operation,
    const uint8_t *input,
    size_t input_length);

                                                                        
  
                                                                           
                                                                           
                                                                          
                 
  
                                                                                   
                                                                           
                                                                     
  
                                                                           
                                                                          
                                                                 
  
                                                      
                                                                       
                                                                    
                                                               
                                                                           
                                                                            
                                                                  
  
                       
                   
                               
                                                                
                                      
                                                                           
                                                                     
                                                                 
                                                           
                                                           
   
psa_status_t mbedtls_psa_hash_finish(
    mbedtls_psa_hash_operation_t *operation,
    uint8_t *hash,
    size_t hash_size,
    size_t *hash_length);

                                     
  
                                                                          
                                                                          
                                                                          
                 
  
                                                                      
                                                                    
                                                 
                                  
  
                                                                     
                                                                             
  
                                                                               
                                                                                 
                                                       
  
                                                           
  
                                         
                                                           
   
psa_status_t mbedtls_psa_hash_abort(
    mbedtls_psa_hash_operation_t *operation);

#endif                        
