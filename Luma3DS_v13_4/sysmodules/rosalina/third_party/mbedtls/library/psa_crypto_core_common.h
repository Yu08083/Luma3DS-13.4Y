   
                                 
  
                                                                       
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_CORE_COMMON_H
#define PSA_CRYPTO_CORE_COMMON_H

                                   
  
                                                                        
                                                                            
                                                                        
                                                                           
                                                   
  
                                                        
                                                    
                                   
                                                          
                                                                         
                                            
   
static inline unsigned char *psa_crypto_buffer_offset(
    unsigned char *p, size_t n)
{
    return p == NULL ? NULL : p + n;
}

                                             
  
                                                              
  
                                                        
                                                    
                                   
                                                          
                                                                         
                                            
   
static inline const unsigned char *psa_crypto_buffer_offset_const(
    const unsigned char *p, size_t n)
{
    return p == NULL ? NULL : p + n;
}

#endif                               
