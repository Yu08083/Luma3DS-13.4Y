   
                    
  
                                    
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */
#ifndef MBEDTLS_RIPEMD160_H
#define MBEDTLS_RIPEMD160_H
#include "mbedtls/private_access.h"

#include "mbedtls/build_info.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_RIPEMD160_ALT)
                         
  

   
                                               
   
typedef struct mbedtls_ripemd160_context {
    uint32_t MBEDTLS_PRIVATE(total)[2];                                            
    uint32_t MBEDTLS_PRIVATE(state)[5];                                            
    unsigned char MBEDTLS_PRIVATE(buffer)[64];                                     
}
mbedtls_ripemd160_context;

#else                             
#include "ripemd160_alt.h"
#endif                            

   
                                                
  
                                                       
   
void mbedtls_ripemd160_init(mbedtls_ripemd160_context *ctx);

   
                                           
  
                                                   
   
void mbedtls_ripemd160_free(mbedtls_ripemd160_context *ctx);

   
                                                            
  
                                          
                                           
   
void mbedtls_ripemd160_clone(mbedtls_ripemd160_context *dst,
                             const mbedtls_ripemd160_context *src);

   
                                           
  
                                            
  
                                  
   
int mbedtls_ripemd160_starts(mbedtls_ripemd160_context *ctx);

   
                                            
  
                                     
                                          
                                           
  
                                  
   
int mbedtls_ripemd160_update(mbedtls_ripemd160_context *ctx,
                             const unsigned char *input,
                             size_t ilen);

   
                                          
  
                                     
                                             
  
                                  
   
int mbedtls_ripemd160_finish(mbedtls_ripemd160_context *ctx,
                             unsigned char output[20]);

   
                                                                    
  
                                     
                                                   
  
                                  
   
int mbedtls_internal_ripemd160_process(mbedtls_ripemd160_context *ctx,
                                       const unsigned char data[64]);

   
                                                      
  
                                          
                                           
                                             
  
                                  
   
int mbedtls_ripemd160(const unsigned char *input,
                      size_t ilen,
                      unsigned char output[20]);

#if defined(MBEDTLS_SELF_TEST)

   
                                  
  
                                                           
   
int mbedtls_ripemd160_self_test(int verbose);

#endif                        

#ifdef __cplusplus
}
#endif

#endif                          
