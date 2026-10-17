   
                              
  
                                       
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */
#ifndef MBEDTLS_MEMORY_BUFFER_ALLOC_H
#define MBEDTLS_MEMORY_BUFFER_ALLOC_H

#include "mbedtls/build_info.h"

#include <stddef.h>

   
                                 
  
                                                                             
                                                                                      
     
   

#if !defined(MBEDTLS_MEMORY_ALIGN_MULTIPLE)
#define MBEDTLS_MEMORY_ALIGN_MULTIPLE       4                                         
#endif

                                       

#define MBEDTLS_MEMORY_VERIFY_NONE         0
#define MBEDTLS_MEMORY_VERIFY_ALLOC        (1 << 0)
#define MBEDTLS_MEMORY_VERIFY_FREE         (1 << 1)
#define MBEDTLS_MEMORY_VERIFY_ALWAYS       (MBEDTLS_MEMORY_VERIFY_ALLOC | \
                                            MBEDTLS_MEMORY_VERIFY_FREE)

#ifdef __cplusplus
extern "C" {
#endif

   
                                                           
                                                                       
                                                                   
                                                                           
                                 
                                                                            
                                            
  
                                                                      
                                                             
  
                                     
                                  
   
void mbedtls_memory_buffer_alloc_init(unsigned char *buf, size_t len);

   
                                                                       
   
void mbedtls_memory_buffer_alloc_free(void);

   
                                                                              
                                                       
                                                 
  
                                                                                   
                                                                              
   
void mbedtls_memory_buffer_set_verify(int verify);

#if defined(MBEDTLS_MEMORY_DEBUG)
   
                                                                           
                                                                
                                                                         
                                                         
   
void mbedtls_memory_buffer_alloc_status(void);

   
                                                
  
                                              
                                        
   
void mbedtls_memory_buffer_alloc_count_get(size_t *alloc_count, size_t *free_count);

   
                                          
  
                                                                      
                                                                             
                                                                               
                                                                             
   
void mbedtls_memory_buffer_alloc_max_get(size_t *max_used, size_t *max_blocks);

   
                                 
   
void mbedtls_memory_buffer_alloc_max_reset(void);

   
                                      
  
                                                                         
                                                                             
                                                                               
                                                                                
   
void mbedtls_memory_buffer_alloc_cur_get(size_t *cur_used, size_t *cur_blocks);
#endif                           

   
                                                                      
                                                                        
  
                                                                        
                                                                      
                                                                           
                                                         
  
                                                 
   
int mbedtls_memory_buffer_alloc_verify(void);

#if defined(MBEDTLS_SELF_TEST)
   
                                  
  
                                                         
   
int mbedtls_memory_buffer_alloc_self_test(int verbose);
#endif

#ifdef __cplusplus
}
#endif

#endif                            
