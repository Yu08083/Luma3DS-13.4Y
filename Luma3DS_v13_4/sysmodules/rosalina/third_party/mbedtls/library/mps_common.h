/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

   
                     
  
                                                 
   

#ifndef MBEDTLS_MPS_COMMON_H
#define MBEDTLS_MPS_COMMON_H

#include "mps_error.h"

#include <stdio.h>

   
                                         
  
     
   

                                                          
                                                         
                                                      
  
                                                                          
                                                                               
                                                                             
                                                                              
                                                                            
                                                           
                                                                     
                                                                             
                                                  
                                                                          
                                                                              
                                                                               
                                                                         
                                                                            
                                                                       
                                                                            
                                                                             
                                                                       
                                                                    
  
                                                              
                                                                  
                                                                  
                                                                         
                                                                          
                                                     
                                                                           
                                                                     
                                                                       
                    
  
                                                               
                                                              
                                            
  
                                            
   
#define MBEDTLS_MPS_STATE_VALIDATION

                                                                       
  
                                                                   
                                                           
  
                                                             
                                                                 
  
                                                          
                                                            
  
                                       
   
#define MBEDTLS_MPS_ENABLE_ASSERTIONS

                                                                    
                                  

#if defined(MBEDTLS_MPS_STATE_VALIDATION)

#define MBEDTLS_MPS_STATE_VALIDATE_RAW(cond, string)                         \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                          \
        {                                                                      \
            MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_ERROR, string);         \
            MBEDTLS_MPS_TRACE_RETURN(MBEDTLS_ERR_MPS_OPERATION_UNEXPECTED);  \
        }                                                                      \
    } while (0)

#else                                   

#define MBEDTLS_MPS_STATE_VALIDATE_RAW(cond, string)           \
    do                                                           \
    {                                                            \
        (cond);                                                \
    } while (0)

#endif                                   

#if defined(MBEDTLS_MPS_ENABLE_ASSERTIONS)

#define MBEDTLS_MPS_ASSERT_RAW(cond, string)                          \
    do                                                                  \
    {                                                                   \
        if (!(cond))                                                   \
        {                                                               \
            MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_ERROR, string);  \
            MBEDTLS_MPS_TRACE_RETURN(MBEDTLS_ERR_MPS_INTERNAL_ERROR); \
        }                                                               \
    } while (0)

#else                                    

#define MBEDTLS_MPS_ASSERT_RAW(cond, string) do {} while (0)

#endif                                    


                                        

   
                                    
  
                                            
     
   

                                                                         
  
                                                                            
                                                                       
  
                                                                    
                                                  
  
                                                            
                                                      
                                                    
  
                                                                        
                                                                         
                                                                 
  
                                                                        
                                                                
                                                    
  
   
typedef size_t mbedtls_mps_stored_size_t;
#define MBEDTLS_MPS_STORED_SIZE_MAX  (SIZE_MAX)

                                                                   
                              
  
                                                                       
                                                                       
                               
  
                                                                          
                                                                                
                         
   
typedef size_t mbedtls_mps_size_t;
#define MBEDTLS_MPS_SIZE_MAX  (SIZE_MAX)

#if MBEDTLS_MPS_STORED_SIZE_MAX > MBEDTLS_MPS_SIZE_MAX
#error "Misconfiguration of mbedtls_mps_size_t and mbedtls_mps_stored_size_t."
#endif

                              


#endif                           
