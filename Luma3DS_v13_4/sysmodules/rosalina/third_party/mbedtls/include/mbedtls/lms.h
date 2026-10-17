   
              
  
                                                                               
                                                                                
                                                                            
                                                                               
                                                                              
                                              
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */
#ifndef MBEDTLS_LMS_H
#define MBEDTLS_LMS_H

#include <stdint.h>
#include <stddef.h>

#include "mbedtls/private_access.h"
#include "mbedtls/build_info.h"

#define MBEDTLS_ERR_LMS_BAD_INPUT_DATA   -0x0011                                                   
#define MBEDTLS_ERR_LMS_OUT_OF_PRIVATE_KEYS -0x0013                                                               
#define MBEDTLS_ERR_LMS_VERIFY_FAILED    -0x0015                                          
#define MBEDTLS_ERR_LMS_ALLOC_FAILED     -0x0017                                                       
#define MBEDTLS_ERR_LMS_BUFFER_TOO_SMALL -0x0019                                                                  

                                                                       
#define MBEDTLS_LMOTS_N_HASH_LEN_MAX           (32u)
#define MBEDTLS_LMOTS_P_SIG_DIGIT_COUNT_MAX    (34u)
#define MBEDTLS_LMOTS_N_HASH_LEN(type)         ((type) == MBEDTLS_LMOTS_SHA256_N32_W8 ? 32u : 0)
#define MBEDTLS_LMOTS_I_KEY_ID_LEN             (16u)
#define MBEDTLS_LMOTS_Q_LEAF_ID_LEN            (4u)
#define MBEDTLS_LMOTS_TYPE_LEN                 (4u)
#define MBEDTLS_LMOTS_P_SIG_DIGIT_COUNT(type)  ((type) == MBEDTLS_LMOTS_SHA256_N32_W8 ? 34u : 0)
#define MBEDTLS_LMOTS_C_RANDOM_VALUE_LEN(type) (MBEDTLS_LMOTS_N_HASH_LEN(type))

#define MBEDTLS_LMOTS_SIG_LEN(type) (MBEDTLS_LMOTS_TYPE_LEN + \
                                     MBEDTLS_LMOTS_C_RANDOM_VALUE_LEN(type) + \
                                     (MBEDTLS_LMOTS_P_SIG_DIGIT_COUNT(type) * \
                                      MBEDTLS_LMOTS_N_HASH_LEN(type)))


#define MBEDTLS_LMS_TYPE_LEN            (4)
#define MBEDTLS_LMS_H_TREE_HEIGHT(type) ((type) == MBEDTLS_LMS_SHA256_M32_H10 ? 10u : 0)

                                                                      
                   
   
#define MBEDTLS_LMS_M_NODE_BYTES(type) ((type) == MBEDTLS_LMS_SHA256_M32_H10 ? 32 : 0)
#define MBEDTLS_LMS_M_NODE_BYTES_MAX 32

#define MBEDTLS_LMS_SIG_LEN(type, otstype) (MBEDTLS_LMOTS_Q_LEAF_ID_LEN + \
                                            MBEDTLS_LMOTS_SIG_LEN(otstype) + \
                                            MBEDTLS_LMS_TYPE_LEN + \
                                            (MBEDTLS_LMS_H_TREE_HEIGHT(type) * \
                                             MBEDTLS_LMS_M_NODE_BYTES(type)))

#define MBEDTLS_LMS_PUBLIC_KEY_LEN(type) (MBEDTLS_LMS_TYPE_LEN + \
                                          MBEDTLS_LMOTS_TYPE_LEN + \
                                          MBEDTLS_LMOTS_I_KEY_ID_LEN + \
                                          MBEDTLS_LMS_M_NODE_BYTES(type))


#ifdef __cplusplus
extern "C" {
#endif

                                                   
                                                                                               
                                                                                                
   
typedef enum {
    MBEDTLS_LMS_SHA256_M32_H10 = 0x6,
} mbedtls_lms_algorithm_type_t;

                                                     
                                                                                                 
                                                                                                    
   
typedef enum {
    MBEDTLS_LMOTS_SHA256_N32_W8 = 4
} mbedtls_lmots_algorithm_type_t;

                               
  
                                                                         
                                                                               
             
   
typedef struct {
    unsigned char MBEDTLS_PRIVATE(I_key_identifier[MBEDTLS_LMOTS_I_KEY_ID_LEN]);             
                                                                                                  
    unsigned char MBEDTLS_PRIVATE(q_leaf_identifier[MBEDTLS_LMOTS_Q_LEAF_ID_LEN]);           
                                                                                                                  
                                                                                                                                
    mbedtls_lmots_algorithm_type_t MBEDTLS_PRIVATE(type);                                       
                                                                                              
                                                                                      
} mbedtls_lmots_parameters_t;

                                   
  
                                                                         
  
                                                                              
                                                   
  
       
                           
                                          
                                            
                                                         
                                                                             
                                                                    
    
          
   
typedef struct {
    mbedtls_lmots_parameters_t MBEDTLS_PRIVATE(params);
    unsigned char MBEDTLS_PRIVATE(public_key)[MBEDTLS_LMOTS_N_HASH_LEN_MAX];
    unsigned char MBEDTLS_PRIVATE(have_public_key);                                                
                                                                              
} mbedtls_lmots_public_t;

#if defined(MBEDTLS_LMS_PRIVATE)
                                    
  
                                                                           
                                              
  
                                                                              
                                                   
  
       
                           
                                          
                                             
                                                             
                                             
    
          
   
typedef struct {
    mbedtls_lmots_parameters_t MBEDTLS_PRIVATE(params);
    unsigned char MBEDTLS_PRIVATE(private_key)[MBEDTLS_LMOTS_P_SIG_DIGIT_COUNT_MAX][
        MBEDTLS_LMOTS_N_HASH_LEN_MAX];
    unsigned char MBEDTLS_PRIVATE(have_private_key);                                                 
                                                                               
} mbedtls_lmots_private_t;
#endif                                   


                             
  
                                                                       
                                                                            
   
typedef struct {
    unsigned char MBEDTLS_PRIVATE(I_key_identifier[MBEDTLS_LMOTS_I_KEY_ID_LEN]);             
                                                                                                  
    mbedtls_lmots_algorithm_type_t MBEDTLS_PRIVATE(otstype);                                       
                                                                                               
                                                                                       
    mbedtls_lms_algorithm_type_t MBEDTLS_PRIVATE(type);                                        
                                                                                                   
                                                                          
} mbedtls_lms_parameters_t;

                                 
  
                                                                               
                               
  
                                                                              
                                                   
  
       
                         
                                          
                                            
                                                         
                                                                             
                                                                    
    
          
   
typedef struct {
    mbedtls_lms_parameters_t MBEDTLS_PRIVATE(params);
    unsigned char MBEDTLS_PRIVATE(T_1_pub_key)[MBEDTLS_LMS_M_NODE_BYTES_MAX];                        
                                                                                                                          
    unsigned char MBEDTLS_PRIVATE(have_public_key);                                                
                                                                              
} mbedtls_lms_public_t;


#if defined(MBEDTLS_LMS_PRIVATE)
                                  
  
                                                                                
                                         
  
                                                                              
                                                   
  
       
                         
                                          
                                             
                                                             
    
          
   
typedef struct {
    mbedtls_lms_parameters_t MBEDTLS_PRIVATE(params);
    uint32_t MBEDTLS_PRIVATE(q_next_usable_key);                                                
                                                                   
    mbedtls_lmots_private_t *MBEDTLS_PRIVATE(ots_private_keys);                                           
                                                                                                              
                                                                                                                     
                                                                                                                     
    mbedtls_lmots_public_t *MBEDTLS_PRIVATE(ots_public_keys);                                      
                                                                                              
                                                                                                 
                                                                                      
                                                                                                       
                                                                                
    unsigned char MBEDTLS_PRIVATE(have_private_key);                                                 
                                                                               
} mbedtls_lms_private_t;
#endif                                   

   
                                                                            
  
                                                                            
                                         
   
void mbedtls_lms_public_init(mbedtls_lms_public_t *ctx);

   
                                                                              
  
                                                                          
                                           
   
void mbedtls_lms_public_free(mbedtls_lms_public_t *ctx);

   
                                                                           
                                                
  
                                                                             
                                                   
  
                                                                            
                                             
  
                                                                          
                                                                        
                                                                                
                                  
                                                                
  
                                   
                                                    
   
int mbedtls_lms_import_public_key(mbedtls_lms_public_t *ctx,
                                  const unsigned char *key, size_t key_size);

   
                                                                           
                                                                              
                                 
  
                                                                             
                                                                               
                                          
  
                                                                            
                                             
  
                                                                             
                                            
                                                                               
                                                                             
                                                        
                                                                              
                                 
  
                                   
                                                    
   
int mbedtls_lms_export_public_key(const mbedtls_lms_public_t *ctx,
                                  unsigned char *key, size_t key_size,
                                  size_t *key_len);

   
                                                                            
                                                                    
  
                                                                             
                                                                                
                                                              
  
                                                                              
                                                     
                                                                            
                                                                       
                                                                           
                                                                         
                                  
                                                                      
  
                                                   
                                                    
   
int mbedtls_lms_verify(const mbedtls_lms_public_t *ctx,
                       const unsigned char *msg, size_t msg_size,
                       const unsigned char *sig, size_t sig_size);

#if defined(MBEDTLS_LMS_PRIVATE)
   
                                                                             
  
                                                                            
                                                    
void mbedtls_lms_private_init(mbedtls_lms_private_t *ctx);

   
                                                                               
  
                                                                               
                                              
   
void mbedtls_lms_private_free(mbedtls_lms_private_t *ctx);

   
                                                                            
                                                                   
  
                                                                       
                                                                               
                                                                              
                                                                        
  
                                                                             
  
                                                                              
                                  
                                                              
                                                                
                                                                                
                                                                  
                                                                            
                                 
                                                    
  
                                   
                                                    
   
int mbedtls_lms_generate_private_key(mbedtls_lms_private_t *ctx,
                                     mbedtls_lms_algorithm_type_t type,
                                     mbedtls_lmots_algorithm_type_t otstype,
                                     mbedtls_f_rng_t *f_rng,
                                     void *p_rng, const unsigned char *seed,
                                     size_t seed_size);

   
                                                                              
                                                                             
  
                                                                             
                                                                               
                                           
  
                                                                                    
                                                    
  
                                                                            
                                                                                
                                         
  
                                   
                                                    
   
int mbedtls_lms_calculate_public_key(mbedtls_lms_public_t *ctx,
                                     const mbedtls_lms_private_t *priv_ctx);

   
                                                                           
                                                                           
  
                                                                       
                                                                               
                                                                              
                                                                        
  
                                                                             
                                                                             
                                 
  
                                                                                
                                                                                
                                                                                
                                                                              
                                                                            
                                                                        
  
                                                                               
                                                      
                                                                      
                                        
                                                                  
                                                                            
                                                                       
                                                                             
                                                                           
                                                                         
                                          
                                                                              
                                       
  
                                   
                                                    
   
int mbedtls_lms_sign(mbedtls_lms_private_t *ctx,
                     mbedtls_f_rng_t *f_rng,
                     void *p_rng, const unsigned char *msg,
                     unsigned int msg_size, unsigned char *sig, size_t sig_size,
                     size_t *sig_len);
#endif                                   

#ifdef __cplusplus
}
#endif

#endif                    
