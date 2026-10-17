  
                                                     
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_PAKE_H
#define PSA_CRYPTO_PAKE_H

#include <psa/crypto.h>

                                                                          
  
                                                               
                                                                      
                                                                             
                             
  
                                                                            
                                                                    
                                                                                   
                                                           
  
                       
                   
                                   
                                                                              
                                                                           
                                                                       
                                                                           
                                   
                                                             
                                                             
   
psa_status_t mbedtls_psa_pake_setup(mbedtls_psa_pake_operation_t *operation,
                                    const psa_crypto_driver_pake_inputs_t *inputs);


                                                                   
  
                                                               
                                                                        
                                                                             
                             
  
                                                     
                                                                                
                                         
                                                                              
                                                                                   
                                                                                     
                                                
                                                                               
                                                                           
                                                                   
                                                                                
                                                                              
                                               
  
                                                                              
                                      
  
                       
                   
                                      
                                                         
                                                             
                                                             
                                                             
                                                             
   
psa_status_t mbedtls_psa_pake_output(mbedtls_psa_pake_operation_t *operation,
                                     psa_crypto_driver_pake_step_t step,
                                     uint8_t *output,
                                     size_t output_size,
                                     size_t *output_length);

                                                                      
  
                                                               
                                                                      
                                                                             
                             
  
                                                                                   
  
                                                     
                                                                               
                                                                        
                                                                         
                                                                             
                                                    
                                                                    
  
                       
                   
                                       
                                                                  
                                      
                                                                                   
                      
                                   
                                                                                 
                            
                                                             
                                                             
                                                             
                                                             
   
psa_status_t mbedtls_psa_pake_input(mbedtls_psa_pake_operation_t *operation,
                                    psa_crypto_driver_pake_step_t step,
                                    const uint8_t *input,
                                    size_t input_length);

                                                       
  
                                                               
                                                                      
                                                                       
                                                         
  
                                                     
                                                              
                                                                  
                                                                                   
  
                       
                   
                                   
                                                                               
                                    
                                                             
                                                             
                                                             
                                                             
   
psa_status_t mbedtls_psa_pake_get_implicit_key(
    mbedtls_psa_pake_operation_t *operation,
    uint8_t *output, size_t output_size,
    size_t *output_length);

                           
  
                                                               
                                                                      
                                                                             
                             
  
                                                      
  
                       
                   
                                                             
   
psa_status_t mbedtls_psa_pake_abort(mbedtls_psa_pake_operation_t *operation);

#endif                        
