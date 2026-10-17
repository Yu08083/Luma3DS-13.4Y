   
                    
  
                                               
  
                                                                        
                                                                            
                                            
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */
#ifndef MBEDTLS_HMAC_DRBG_H
#define MBEDTLS_HMAC_DRBG_H
#include "mbedtls/private_access.h"

#include "mbedtls/build_info.h"

#include "mbedtls/md.h"

#if defined(MBEDTLS_THREADING_C)
#include "mbedtls/threading.h"
#endif

  
              
   
                                                
#define MBEDTLS_ERR_HMAC_DRBG_REQUEST_TOO_BIG              -0x0003
                                              
#define MBEDTLS_ERR_HMAC_DRBG_INPUT_TOO_BIG                -0x0005
                                
#define MBEDTLS_ERR_HMAC_DRBG_FILE_IO_ERROR                -0x0007
                                 
#define MBEDTLS_ERR_HMAC_DRBG_ENTROPY_SOURCE_FAILED        -0x0009

   
                                 
  
                                                                             
                                                                                      
     
   

#if !defined(MBEDTLS_HMAC_DRBG_RESEED_INTERVAL)
#define MBEDTLS_HMAC_DRBG_RESEED_INTERVAL   10000                                                         
#endif

#if !defined(MBEDTLS_HMAC_DRBG_MAX_INPUT)
#define MBEDTLS_HMAC_DRBG_MAX_INPUT         256                                                     
#endif

#if !defined(MBEDTLS_HMAC_DRBG_MAX_REQUEST)
#define MBEDTLS_HMAC_DRBG_MAX_REQUEST       1024                                                      
#endif

#if !defined(MBEDTLS_HMAC_DRBG_MAX_SEED_INPUT)
#define MBEDTLS_HMAC_DRBG_MAX_SEED_INPUT    384                                            
#endif

                                       

#define MBEDTLS_HMAC_DRBG_PR_OFF   0                                         
#define MBEDTLS_HMAC_DRBG_PR_ON    1                                         

#ifdef __cplusplus
extern "C" {
#endif

   
                     
   
typedef struct mbedtls_hmac_drbg_context {
                                                         
                                            
    mbedtls_md_context_t MBEDTLS_PRIVATE(md_ctx);                                                  
    unsigned char MBEDTLS_PRIVATE(V)[MBEDTLS_MD_MAX_SIZE];                                
    int MBEDTLS_PRIVATE(reseed_counter);                                                   

                              
    size_t MBEDTLS_PRIVATE(entropy_len);                                                       
    int MBEDTLS_PRIVATE(prediction_resistance);                                              
                                                                                             
    int MBEDTLS_PRIVATE(reseed_interval);                                 

                   
    int(*MBEDTLS_PRIVATE(f_entropy))(void *, unsigned char *, size_t);                          
    void *MBEDTLS_PRIVATE(p_entropy);                                                           

#if defined(MBEDTLS_THREADING_C)
                                                         
                                                                        
                                                                
                                                                         
      
                                                                            
                                                                
       
    mbedtls_threading_mutex_t MBEDTLS_PRIVATE(mutex);
#endif
} mbedtls_hmac_drbg_context;

   
                                                         
  
                                                                      
                                                            
  
                                                                                 
                                                                  
                                                                
  
                                                            
   
void mbedtls_hmac_drbg_init(mbedtls_hmac_drbg_context *ctx);

   
                                                  
  
                                                                         
  
                                                                       
                             
                                            
                                                                         
                                                                        
                      
  
                                                              
                                                                    
  
                                                                                
                                                          
                                                            
                                                                                
                                                                          
                                                                         
                                                  
                                                            
  
                                                                           
                                                                        
                                                                          
  
                                                                       
                                                            
                                                                
   
#if defined(MBEDTLS_THREADING_C)
   
                                                                      
                                                                 
                                                                     
                                                                          
                                                       
   
#endif                          
   
                                                       
                                                          
                                                                     
                                                                         
                                             
                                                                           
                                                                 
                                                                    
                                                   
                                                                              
                                                                          
                                                                 
                                                                         
                                        
                                                                                  
                                                                  
                                        
  
                                           
                                                                       
                                
                                                                            
                                                        
                                                                    
                                                           
   
int mbedtls_hmac_drbg_seed(mbedtls_hmac_drbg_context *ctx,
                           const mbedtls_md_info_t *md_info,
                           int (*f_entropy)(void *, unsigned char *, size_t),
                           void *p_entropy,
                           const unsigned char *custom,
                           size_t len);

   
                                                                               
  
                                                                        
                                     
   
#if defined(MBEDTLS_THREADING_C)
   
                                                                      
                                                                 
                                                                     
                                                                          
                                                       
   
#endif                          
   
                                                            
                                                          
                                                                       
                                            
                                                   
  
                                              
                                                                       
                                
                                                                            
                                                        
   
int mbedtls_hmac_drbg_seed_buf(mbedtls_hmac_drbg_context *ctx,
                               const mbedtls_md_info_t *md_info,
                               const unsigned char *data, size_t data_len);

   
                                                                            
                                                 
  
                                                                           
                                                                         
                                                      
                                                                           
                                   
  
                                              
                                                                              
   
void mbedtls_hmac_drbg_set_prediction_resistance(mbedtls_hmac_drbg_context *ctx,
                                                 int resistance);

   
                                                                                
                                       
  
                                                                           
  
                                              
                                                                
   
void mbedtls_hmac_drbg_set_entropy_len(mbedtls_hmac_drbg_context *ctx,
                                       size_t len);

   
                                                
  
                                                                           
                                                                          
                   
  
                                                           
  
                                              
                                            
   
void mbedtls_hmac_drbg_set_reseed_interval(mbedtls_hmac_drbg_context *ctx,
                                           int interval);

   
                                                                                 
  
                                                                        
                                                                        
                                                                           
                                                                          
  
                                              
                                                          
                                                                        
                                                         
                                                           
  
                                                                        
                                         
   
int mbedtls_hmac_drbg_update(mbedtls_hmac_drbg_context *ctx,
                             const unsigned char *additional, size_t add_len);

   
                                                                            
                                                              
  
                                                                        
                                                                        
                                                                           
                                                                          
  
                                              
                                                            
                                                                       
                                                  
                                                          
                                                                         
                                        
                                                                          
                                                                  
                                                                  
  
                                           
                                                                    
                                                                 
   
int mbedtls_hmac_drbg_reseed(mbedtls_hmac_drbg_context *ctx,
                             const unsigned char *additional, size_t len);

   
                                                                       
                                                     
  
                                                                        
                                       
  
                                                                        
                                                                        
                                                                           
                                                                          
  
                                                                          
                                                             
                                           
                                                          
                                                                            
                                                       
                                                                       
                                                      
                                                          
                                                                          
  
                                           
                                                                    
                                                               
                                                                 
                                                                       
                                                               
                                                                  
   
int mbedtls_hmac_drbg_random_with_add(void *p_rng,
                                      unsigned char *output, size_t output_len,
                                      const unsigned char *additional,
                                      size_t add_len);

   
                                                                 
  
                                                                        
                                       
   
#if defined(MBEDTLS_THREADING_C)
   
                                                                      
                                                                    
                                                                          
                                                       
   
#endif                          
   
                                                                          
                                                             
                                           
                                                          
                                                                            
  
                                           
                                                                    
                                                               
                                                                 
                                                                    
   
int mbedtls_hmac_drbg_random(void *p_rng, unsigned char *output, size_t out_len);

   
                                                                                       
                                                                       
  
                                                      
   
void mbedtls_hmac_drbg_free(mbedtls_hmac_drbg_context *ctx);

#if defined(MBEDTLS_FS_IO)
   
                                                         
  
                                              
                                             
  
                                        
                                                                           
                                                                              
                                
   
int mbedtls_hmac_drbg_write_seed_file(mbedtls_hmac_drbg_context *ctx, const char *path);

   
                                                                             
                                                  
  
                                              
                                             
  
                                        
                                                                           
                                                                       
                                       
                                                                            
                                               
   
int mbedtls_hmac_drbg_update_seed_file(mbedtls_hmac_drbg_context *ctx, const char *path);
#endif                    


#if defined(MBEDTLS_SELF_TEST)
   
                                                      
  
                                           
                                                
   
int mbedtls_hmac_drbg_self_test(int verbose);
#endif

#ifdef __cplusplus
}
#endif

#endif                  
