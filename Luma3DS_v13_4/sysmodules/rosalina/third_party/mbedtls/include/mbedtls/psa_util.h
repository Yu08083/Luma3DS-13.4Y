   
                   
  
                                                                  
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_PSA_UTIL_H
#define MBEDTLS_PSA_UTIL_H
#include "mbedtls/private_access.h"

#include "mbedtls/build_info.h"

#include "psa/crypto.h"

                                                       
                                                                          
                                                                               
#include <mbedtls/asn1write.h>

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT)

                                                        
  
                                                                     
                                             
  
                                                                          
           
  
                                                                              
                                                                           
                                                                         
                                   
  
                                                                              
                                                                               
                                                                 
                                                                 
                                             
                                                                  
                                                                       
                                                                         
                                                                    
                                                                   
                                      
  
                                        
                                                     
                                                  
                                                     
                                                             
   
int mbedtls_psa_get_random(void *p_rng,
                           unsigned char *output,
                           size_t output_size);

                                                     
  
                                                                            
                                                                      
                 
   
#define MBEDTLS_PSA_RANDOM_STATE    NULL

                                                  
     
   
#if defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY)
#include <mbedtls/ecp.h>

                                                                      
  
                                                             
                                               
                                                                           
  
                                                                               
                                                               
                                                                               
                                                        
                                                                          
   
psa_ecc_family_t mbedtls_ecc_group_to_psa(mbedtls_ecp_group_id grpid,
                                          size_t *bits);

                                                                      
  
                                                              
                                               
                                                                    
  
                                                                               
                                                                         
                                                          
                                                                           
                                                     
   
mbedtls_ecp_group_id mbedtls_ecc_group_from_psa(psa_ecc_family_t family,
                                                size_t bits);
#endif                                       

   
                                                                      
                                                          
  
                                                                       
  
                                                                           
                                                                         
  
                                                                    
                                                                              
  
                                                                            
                                                                 
   
static inline psa_algorithm_t mbedtls_md_psa_alg_from_type(mbedtls_md_type_t md_type)
{
    return PSA_ALG_CATEGORY_HASH | (psa_algorithm_t) md_type;
}

   
                                                               
                                                                 
  
                                                               
  
                                                                    
                                                                              
  
                                                           
                                                                 
   
static inline mbedtls_md_type_t mbedtls_md_type_from_psa_alg(psa_algorithm_t psa_alg)
{
    return (mbedtls_md_type_t) (psa_alg & PSA_ALG_HASH_MASK);
}
#endif                                

#if defined(MBEDTLS_PSA_UTIL_HAVE_ECDSA)

                                                                   
  
                                                            
                                                                             
                                                                   
                                                          
                                                                             
                                                                   
                                                                                
                                                                                
                                                                               
                                                                   
                                                                              
                                                                              
                                                   
                                                                           
                                                                        
                                               
  
                                                                        
                                                     
  
                                            
                                                                          
                                                                         
                                                    
                                                                        
                                                          
   
int mbedtls_ecdsa_raw_to_der(size_t bits, const unsigned char *raw, size_t raw_len,
                             unsigned char *der, size_t der_size, size_t *der_len);

                                                                   
  
                                                            
                                                                             
                                                    
                                                                             
                                                                      
                                                                     
                                                              
                                                                             
                                                                             
                                               
  
                                            
                                                                          
                                                                         
                                                    
                                                                         
                                                                
                                                                    
                                                
   
int mbedtls_ecdsa_der_to_raw(size_t bits, const unsigned char *der, size_t der_len,
                             unsigned char *raw, size_t raw_size, size_t *raw_len);

#endif                                  

       

#endif                         
