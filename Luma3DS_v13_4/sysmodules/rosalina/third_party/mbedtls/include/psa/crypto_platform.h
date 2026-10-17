   
                              
  
                                                                
  
                                                                  
                        
  
                                                          
  
                                                                    
                                                                    
                                                                  
                     
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_PLATFORM_H
#define PSA_CRYPTO_PLATFORM_H
#include "mbedtls/private_access.h"

  
                                                                           
                                                                            
                                                                           
                                                                  
                 
   
#include "psa/build_info.h"

                                                                
#include <stdint.h>

#if defined(MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)

                                                                              
                        
  
                                                                         
                                                                      
                                                                       
                                                                         
                
   
typedef int32_t mbedtls_key_owner_id_t;

                                      
  
                                         
                                          
  
                                                                               
   
static inline int mbedtls_key_owner_id_equal(mbedtls_key_owner_id_t id1,
                                             mbedtls_key_owner_id_t id2)
{
    return id1 == id2;
}

#endif                                              

  
                                                                          
                                                                           
                                                                             
                                                                               
            
   
#if defined(MBEDTLS_PSA_CRYPTO_SPM)
#define PSA_CRYPTO_SECURE 1
#include "crypto_spe.h"
#endif                          

#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
                                                                        
  
                                                                   
                                                        
  
                                                                 
                                                                            
                                                       
   
typedef struct {
    uintptr_t MBEDTLS_PRIVATE(opaque)[2];
} mbedtls_psa_external_random_context_t;
#endif                                      

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)
                                                            
  
                                                                      
                                                                     
                                          
   
typedef uint32_t mbedtls_psa_client_handle_t;
#endif

#endif                            
