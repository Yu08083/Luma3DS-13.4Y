   
                
  
                                                                             
                                                                                
                                                                            
                                                                    
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_LMOTS_H
#define MBEDTLS_LMOTS_H

#include "mbedtls/build_info.h"

#include "psa/crypto.h"

#include "mbedtls/lms.h"

#include <stdint.h>
#include <stddef.h>


#define MBEDTLS_LMOTS_PUBLIC_KEY_LEN(type) (MBEDTLS_LMOTS_TYPE_LEN + \
                                            MBEDTLS_LMOTS_I_KEY_ID_LEN + \
                                            MBEDTLS_LMOTS_Q_LEAF_ID_LEN + \
                                            MBEDTLS_LMOTS_N_HASH_LEN(type))

#define MBEDTLS_LMOTS_SIG_TYPE_OFFSET       (0)
#define MBEDTLS_LMOTS_SIG_C_RANDOM_OFFSET (MBEDTLS_LMOTS_SIG_TYPE_OFFSET + \
                                           MBEDTLS_LMOTS_TYPE_LEN)
#define MBEDTLS_LMOTS_SIG_SIGNATURE_OFFSET(type) (MBEDTLS_LMOTS_SIG_C_RANDOM_OFFSET + \
                                                  MBEDTLS_LMOTS_C_RANDOM_VALUE_LEN(type))

#ifdef __cplusplus
extern "C" {
#endif


#if defined(MBEDTLS_TEST_HOOKS)
extern int (*mbedtls_lmots_sign_private_key_invalidated_hook)(unsigned char *);
#endif                                  

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
   
                                                                            
                                                      
  
                                                        
  
                                                              
   
int MBEDTLS_DEPRECATED mbedtls_lms_error_from_psa(psa_status_t status);
#endif

   
                                                                             
  
                                                                              
                                         
   
void mbedtls_lmots_public_init(mbedtls_lmots_public_t *ctx);

   
                                                                               
  
                                                                            
                                           
   
void mbedtls_lmots_public_free(mbedtls_lmots_public_t *ctx);

   
                                                                             
                                           
  
                                                                             
                                                   
  
                                                                            
                                             
  
                                                                            
                                                                        
                                                                             
                                       
  
                                   
                                                    
   
int mbedtls_lmots_import_public_key(mbedtls_lmots_public_t *ctx,
                                    const unsigned char *key, size_t key_size);

   
                                                                             
                                                                              
  
                                                                             
                                                                               
                                          
  
                                                                            
                                             
  
                                                                            
                                        
                                                                               
                                                                               
  
                                   
                                                    
   
int mbedtls_lmots_export_public_key(const mbedtls_lmots_public_t *ctx,
                                    unsigned char *key, size_t key_size,
                                    size_t *key_len);

   
                                                                              
                                                                             
                                                                             
                                           
  
                                                                                
                                                                        
                                                                        
                                                    
  
                                                                          
                                                               
                                                                            
                                                                       
                                                                              
                                                                           
                                  
                                                                              
                                                                               
                                           
  
                                   
                                                    
   
int mbedtls_lmots_calculate_public_key_candidate(const mbedtls_lmots_parameters_t *params,
                                                 const unsigned char *msg,
                                                 size_t msg_size,
                                                 const unsigned char *sig,
                                                 size_t sig_size,
                                                 unsigned char *out,
                                                 size_t out_size,
                                                 size_t *out_len);

   
                                                                              
                                                                      
  
                                                                       
                                                                               
                                                                              
                                                                        
  
                                                                             
                                                                                
                                                                            
                                  
  
                                                                                
                                              
                                                                            
                                                                       
                                                                           
                                                                           
                                  
  
                                                   
                                                    
   
int mbedtls_lmots_verify(const mbedtls_lmots_public_t *ctx,
                         const unsigned char *msg,
                         size_t msg_size, const unsigned char *sig,
                         size_t sig_size);

#if defined(MBEDTLS_LMS_PRIVATE)

   
                                                                              
  
                                                                              
                                         
   
void mbedtls_lmots_private_init(mbedtls_lmots_private_t *ctx);

   
                                                                                
  
                                                                            
                                           
   
void mbedtls_lmots_private_free(mbedtls_lmots_private_t *ctx);

   
                                                                               
                                                             
  
                                                                       
                                                                               
                                                                              
                                                                        
  
                                                                             
  
                                                                              
                                  
                                                                                
                                                                             
                                                                              
                                         
                                                                            
                                 
                                                    
  
                                   
                                                    
   
int mbedtls_lmots_generate_private_key(mbedtls_lmots_private_t *ctx,
                                       mbedtls_lmots_algorithm_type_t type,
                                       const unsigned char I_key_identifier[MBEDTLS_LMOTS_I_KEY_ID_LEN],
                                       uint32_t q_leaf_identifier,
                                       const unsigned char *seed,
                                       size_t seed_size);

   
                                                                               
                                                                               
  
                                                                             
                                                                               
                                           
  
                                                                              
                                                    
  
                                   
                                                    
   
int mbedtls_lmots_calculate_public_key(mbedtls_lmots_public_t *ctx,
                                       const mbedtls_lmots_private_t *priv_ctx);

   
                                                                             
                                                                       
  
                                                                             
                                                                             
                                 
  
                                                                                
                                                                               
                                                                                
                                                                              
                                                                               
                                      
  
                                                                         
                                                      
                                                                      
                                        
                                                                  
                                                                            
                                                                       
                                                                             
                                                                             
  
                                   
                                                    
   
int mbedtls_lmots_sign(mbedtls_lmots_private_t *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng, const unsigned char *msg, size_t msg_size,
                       unsigned char *sig, size_t sig_size, size_t *sig_len);

#endif                                   

#ifdef __cplusplus
}
#endif

#endif                      
