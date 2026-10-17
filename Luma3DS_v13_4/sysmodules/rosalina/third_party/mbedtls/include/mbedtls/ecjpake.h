   
                  
  
                               
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */
#ifndef MBEDTLS_ECJPAKE_H
#define MBEDTLS_ECJPAKE_H
#include "mbedtls/private_access.h"

  
                                                                         
                                                                   
                                                              
                                                                                
  
                                                             
                                                              
                                                                   
  
                                                                  
                                                                         
                                                                       
                                                                          
                           
   
#include "mbedtls/build_info.h"

#include "mbedtls/ecp.h"
#include "mbedtls/md.h"

#ifdef __cplusplus
extern "C" {
#endif

   
                                  
   
typedef enum {
    MBEDTLS_ECJPAKE_CLIENT = 0,                                               
    MBEDTLS_ECJPAKE_SERVER,                                                   
    MBEDTLS_ECJPAKE_NONE,                                                     
} mbedtls_ecjpake_role;

#if !defined(MBEDTLS_ECJPAKE_ALT)
   
                               
  
                                                                     
                                                                     
                                               
  
                                                                       
                                                                           
                                                       
   
typedef struct mbedtls_ecjpake_context {
    mbedtls_md_type_t MBEDTLS_PRIVATE(md_type);                                                
    mbedtls_ecp_group MBEDTLS_PRIVATE(grp);                                                    
    mbedtls_ecjpake_role MBEDTLS_PRIVATE(role);                                                
    int MBEDTLS_PRIVATE(point_format);                                                         

    mbedtls_ecp_point MBEDTLS_PRIVATE(Xm1);                                                    
    mbedtls_ecp_point MBEDTLS_PRIVATE(Xm2);                                                    
    mbedtls_ecp_point MBEDTLS_PRIVATE(Xp1);                                                    
    mbedtls_ecp_point MBEDTLS_PRIVATE(Xp2);                                                    
    mbedtls_ecp_point MBEDTLS_PRIVATE(Xp);                                                     

    mbedtls_mpi MBEDTLS_PRIVATE(xm1);                                                          
    mbedtls_mpi MBEDTLS_PRIVATE(xm2);                                                          

    mbedtls_mpi MBEDTLS_PRIVATE(s);                                                            
} mbedtls_ecjpake_context;

#else                           
#include "ecjpake_alt.h"
#endif                          

   
                                                  
  
                                                      
                                             
   
void mbedtls_ecjpake_init(mbedtls_ecjpake_context *ctx);

   
                                                      
  
                                                                           
                                                                              
  
                                                                            
                                                               
                                                                       
                                                               
                                                   
                                                                
                                                          
                                                                    
                                                                               
                                                                
                                                                  
  
                                       
                                                     
   
int mbedtls_ecjpake_setup(mbedtls_ecjpake_context *ctx,
                          mbedtls_ecjpake_role role,
                          mbedtls_md_type_t hash,
                          mbedtls_ecp_group_id curve,
                          const unsigned char *secret,
                          size_t len);

   
                                                                         
  
                                                         
                                                
                                                              
                                                      
  
                                           
                                                                          
                                   
   
int mbedtls_ecjpake_set_point_format(mbedtls_ecjpake_context *ctx,
                                     int point_format);

   
                                                                 
  
                                                              
                                
  
                                                         
                                                              
   
int mbedtls_ecjpake_check(const mbedtls_ecjpake_context *ctx);

   
                                                              
                                                                       
                                                               
  
                                                            
                                           
                                                                       
                                                           
                                                  
                                                                  
                                                                         
                                                                      
                                                                    
                                                                     
  
                                       
                                                     
   
int mbedtls_ecjpake_write_round_one(mbedtls_ecjpake_context *ctx,
                                    unsigned char *buf, size_t len, size_t *olen,
                                    mbedtls_f_rng_t *f_rng,
                                    void *p_rng);

   
                                                            
                                                                       
                                                               
  
                                                                        
                               
                                                                         
                                                                
                                                  
  
                                       
                                                     
   
int mbedtls_ecjpake_read_round_one(mbedtls_ecjpake_context *ctx,
                                   const unsigned char *buf,
                                   size_t len);

   
                                                               
                                                                    
  
                                                                         
                                                                 
                                                                  
                                                                          
                                                
                                                                           
                                                                
                                                                      
                                                                    
                                                                     
  
                                       
                                                     
   
int mbedtls_ecjpake_write_round_two(mbedtls_ecjpake_context *ctx,
                                    unsigned char *buf, size_t len, size_t *olen,
                                    mbedtls_f_rng_t *f_rng,
                                    void *p_rng);

   
                                                             
                                                                    
  
                                                                        
                                                                    
                                                                          
                                                                
                                                  
  
                                       
                                                     
   
int mbedtls_ecjpake_read_round_two(mbedtls_ecjpake_context *ctx,
                                   const unsigned char *buf,
                                   size_t len);

   
                                            
                                             
  
                                                                         
                                                                     
                                                                        
                                                                
                                                  
                                                                           
                                                                
                                                                      
                                                                    
                                                                     
  
                                       
                                                     
   
int mbedtls_ecjpake_derive_secret(mbedtls_ecjpake_context *ctx,
                                  unsigned char *buf, size_t len, size_t *olen,
                                  mbedtls_f_rng_t *f_rng,
                                  void *p_rng);

   
                                                                       
                                                                
  
                                                                         
                                                                     
                                                                        
                                                                
                                                  
                                                                           
                                                                
                                                                      
                                                                    
                                                                     
  
                                       
                                                     
   
int mbedtls_ecjpake_write_shared_key(mbedtls_ecjpake_context *ctx,
                                     unsigned char *buf, size_t len, size_t *olen,
                                     mbedtls_f_rng_t *f_rng,
                                     void *p_rng);

   
                                                                
                                            
  
                                                                     
                                                                          
                                                                             
   
void mbedtls_ecjpake_free(mbedtls_ecjpake_context *ctx);

#if defined(MBEDTLS_SELF_TEST)

   
                                  
  
                                                         
   
int mbedtls_ecjpake_self_test(int verbose);

#endif                        

#ifdef __cplusplus
}
#endif


#endif                
