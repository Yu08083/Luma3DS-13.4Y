   
                   
  
                               
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */
#ifndef MBEDTLS_CAMELLIA_H
#define MBEDTLS_CAMELLIA_H
#include "mbedtls/private_access.h"

#include "mbedtls/build_info.h"

#include <stddef.h>
#include <stdint.h>

#include "mbedtls/platform_util.h"

#define MBEDTLS_CAMELLIA_ENCRYPT     1
#define MBEDTLS_CAMELLIA_DECRYPT     0

                      
#define MBEDTLS_ERR_CAMELLIA_BAD_INPUT_DATA -0x0024

                                 
#define MBEDTLS_ERR_CAMELLIA_INVALID_INPUT_LENGTH -0x0026

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_CAMELLIA_ALT)
                         
  

   
                                             
   
typedef struct mbedtls_camellia_context {
    int MBEDTLS_PRIVATE(nr);                                               
    uint32_t MBEDTLS_PRIVATE(rk)[68];                                           
}
mbedtls_camellia_context;

#else                            
#include "camellia_alt.h"
#endif                           

   
                                                 
  
                                                          
                                            
   
void mbedtls_camellia_init(mbedtls_camellia_context *ctx);

   
                                            
  
                                                                           
                                                                                
                                                   
   
void mbedtls_camellia_free(mbedtls_camellia_context *ctx);

   
                                                                            
  
                                                                         
                                                                            
                                           
                                                                            
                                    
  
                                      
                                                    
   
int mbedtls_camellia_setkey_enc(mbedtls_camellia_context *ctx,
                                const unsigned char *key,
                                unsigned int keybits);

#if !defined(MBEDTLS_BLOCK_CIPHER_NO_DECRYPT)
   
                                                                            
  
                                                                         
                                                                     
                                           
                                                                            
                                    
  
                                      
                                                    
   
int mbedtls_camellia_setkey_dec(mbedtls_camellia_context *ctx,
                                const unsigned char *key,
                                unsigned int keybits);
#endif                                       

   
                                                                                
  
                                                                        
                                      
                                                             
                                                                          
                                                                  
                                       
                                                                   
                                       
  
                                      
                                                    
   
int mbedtls_camellia_crypt_ecb(mbedtls_camellia_context *ctx,
                               int mode,
                               const unsigned char input[16],
                               unsigned char output[16]);

#if defined(MBEDTLS_CIPHER_MODE_CBC)
   
                                                                                 
  
                                                                              
                                                                         
                                                                        
                                                                          
                                                                              
                                                                           
                                  
  
                                                                        
                                      
                                                             
                                                                          
                                                                  
                                                          
                                                                              
                                                                          
                                          
                                                                          
                                                             
                                                                           
                                                             
  
                                      
                                                    
   
int mbedtls_camellia_crypt_cbc(mbedtls_camellia_context *ctx,
                               int mode,
                               size_t length,
                               unsigned char iv[16],
                               const unsigned char *input,
                               unsigned char *output);
#endif                              

#if defined(MBEDTLS_CIPHER_MODE_CFB)
   
                                                                         
                             
  
                                                                         
                                                                               
                                                                            
                                                                              
                                                                             
  
                                                                              
                                                                         
                                                                        
                                                                          
                                                                              
                                                                           
                                  
  
                                                                        
                                      
                                                             
                                                                          
                                                                               
                                                                     
                                                                           
                                                      
                                                                              
                                                                          
                                                            
                                                                             
                                                  
                                                                              
                                                    
  
                                      
                                                    
   
int mbedtls_camellia_crypt_cfb128(mbedtls_camellia_context *ctx,
                                  int mode,
                                  size_t length,
                                  size_t *iv_off,
                                  unsigned char iv[16],
                                  const unsigned char *input,
                                  unsigned char *output);
#endif                              

#if defined(MBEDTLS_CIPHER_MODE_CTR)
   
                                                                             
  
                                                                     
                                                                           
                                                                        
                                                                           
                                                                         
  
                                                                             
                                                                            
                                      
  
                                                                            
  
                                                                              
                                                                           
                                                                              
                                                                             
                                                                         
                                        
  
                                                                        
                                                
  
                                                                      
                                                                          
                                                                         
                                                   
  
                                                                           
                                                                           
                                                                              
                                                                            
                                                                               
                                                                              
                                                                             
                                       
  
                                                                              
                                                                           
                                                                           
                                                                            
                                                                          
                                                                            
                                                                      
  
                                                                              
                                                    
  
                                                                        
                                                                            
                                                                   
  
                                                                             
                                           
                                                                       
                                             
                                                                               
                                                                            
                                                                              
                                                
                                                                                
                                                     
                                                                           
                                                                
                                                                                
                                             
                                                                              
                                                
  
                                           
                                                         
   
int mbedtls_camellia_crypt_ctr(mbedtls_camellia_context *ctx,
                               size_t length,
                               size_t *nc_off,
                               unsigned char nonce_counter[16],
                               unsigned char stream_block[16],
                               const unsigned char *input,
                               unsigned char *output);
#endif                              

#if defined(MBEDTLS_SELF_TEST)

   
                                  
  
                                                           
   
int mbedtls_camellia_self_test(int verbose);

#endif                        

#ifdef __cplusplus
}
#endif

#endif                 
