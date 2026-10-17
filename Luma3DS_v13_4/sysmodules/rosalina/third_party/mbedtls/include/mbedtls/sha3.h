   
               
  
                                                             
  
                                           
                                                              
                                                                
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_SHA3_H
#define MBEDTLS_SHA3_H
#include "mbedtls/private_access.h"

#include "mbedtls/build_info.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

                                      
#define MBEDTLS_ERR_SHA3_BAD_INPUT_DATA                 -0x0076

   
                   
  
                                                      
   

typedef enum {
    MBEDTLS_SHA3_NONE = 0,                               
    MBEDTLS_SHA3_224,                 
    MBEDTLS_SHA3_256,                 
    MBEDTLS_SHA3_384,                 
    MBEDTLS_SHA3_512,                 
} mbedtls_sha3_id;

   
                                               
  
                                                                     
   
typedef struct {
    uint64_t MBEDTLS_PRIVATE(state[25]);
    uint32_t MBEDTLS_PRIVATE(index);
    uint16_t MBEDTLS_PRIVATE(olen);
    uint16_t MBEDTLS_PRIVATE(max_block_size);
}
mbedtls_sha3_context;

   
                                                             
  
                                                                             
   
void mbedtls_sha3_init(mbedtls_sha3_context *ctx);

   
                                                        
  
                                                                            
                                                                                
                                                                 
   
void mbedtls_sha3_free(mbedtls_sha3_context *ctx);

   
                                                                     
  
                                                                     
                                                                  
   
void mbedtls_sha3_clone(mbedtls_sha3_context *dst,
                        const mbedtls_sha3_context *src);

   
                                                        
                               
  
                                                                
                                              
  
                                   
                                                    
   
int mbedtls_sha3_starts(mbedtls_sha3_context *ctx, mbedtls_sha3_id id);

   
                                                                      
                                              
  
                                                              
                                                     
                                                                       
                                                  
                                                         
  
                                   
                                                    
   
int mbedtls_sha3_update(mbedtls_sha3_context *ctx,
                        const uint8_t *input,
                        size_t ilen);

   
                                                                         
                                                   
  
                                                              
                                                     
                                             
                                                                          
                                                                                            
                                                                                   
                                
  
                                   
                                                    
   
int mbedtls_sha3_finish(mbedtls_sha3_context *ctx,
                        uint8_t *output, size_t olen);

   
                                                     
                                        
  
                                                                   
                                                      
  
                                                    
                                                       
  
                                              
                                                                       
                                                  
                                                         
                                             
                                                                          
                                                                                            
                                                                                   
                                
  
                                   
                                                    
   
int mbedtls_sha3(mbedtls_sha3_id id, const uint8_t *input,
                 size_t ilen,
                 uint8_t *output,
                 size_t olen);

#if defined(MBEDTLS_SELF_TEST)
   
                                                                 
                                                                          
  
                                                            
   
int mbedtls_sha3_self_test(int verbose);
#endif                        

#ifdef __cplusplus
}
#endif

#endif                     
