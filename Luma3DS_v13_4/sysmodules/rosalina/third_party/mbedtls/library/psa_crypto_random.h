  
                                                   
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_RANDOM_H
#define PSA_CRYPTO_RANDOM_H

#include "common.h"

#if !defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)

#include <psa/crypto.h>
#include "psa_crypto_random_impl.h"

                                        
  
                                                                   
   
void psa_random_internal_init(mbedtls_psa_random_context_t *rng);

                                          
  
                                                                     
   
void psa_random_internal_free(mbedtls_psa_random_context_t *rng);

                                  
  
                                          
  
                                                             
  
                       
                   
                                          
                                     
   
psa_status_t psa_random_internal_seed(mbedtls_psa_random_context_t *rng);

   
                                                                        
                             
  
                                
  
                                                                            
                                                                           
                                       
  
                                                                 
                                                                 
                                                                   
  
                       
                   
                                          
                                                                 
                         
                                      
                                         
   
psa_status_t psa_random_internal_generate(
    mbedtls_psa_random_context_t *rng,
    uint8_t *output, size_t output_size);

#endif                                                

#endif                          
