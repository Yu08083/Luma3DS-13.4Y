/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

   
                    
  
                                 
   

#ifndef MBEDTLS_MPS_ERROR_H
#define MBEDTLS_MPS_ERROR_H


                                                         
  
                                                                
                                                                    
                                              
                                                               
                                                               
                                                                 
                
   

   
                                               
  
     
   

#ifndef MBEDTLS_MPS_ERR_BASE
#define MBEDTLS_MPS_ERR_BASE (0)
#endif

#define MBEDTLS_MPS_MAKE_ERROR(code) \
    (-(MBEDTLS_MPS_ERR_BASE | (code)))

#define MBEDTLS_ERR_MPS_OPERATION_UNEXPECTED  MBEDTLS_MPS_MAKE_ERROR(0x1)
#define MBEDTLS_ERR_MPS_INTERNAL_ERROR        MBEDTLS_MPS_MAKE_ERROR(0x2)

                                              

   
                                              
  
     
   

#ifndef MBEDTLS_MPS_READER_ERR_BASE
#define MBEDTLS_MPS_READER_ERR_BASE (1 << 8)
#endif

#define MBEDTLS_MPS_READER_MAKE_ERROR(code) \
    (-(MBEDTLS_MPS_READER_ERR_BASE | (code)))

                                                                      
                                                        
#define MBEDTLS_ERR_MPS_READER_DATA_LEFT             MBEDTLS_MPS_READER_MAKE_ERROR(0x1)

                                                    
#define MBEDTLS_ERR_MPS_READER_INVALID_ARG           MBEDTLS_MPS_READER_MAKE_ERROR(0x2)

                                                                                   
                                                                                 
                                             
#define MBEDTLS_ERR_MPS_READER_NEED_MORE             MBEDTLS_MPS_READER_MAKE_ERROR(0x3)

                                                                               
#define MBEDTLS_ERR_MPS_READER_OUT_OF_DATA           MBEDTLS_MPS_READER_MAKE_ERROR(0x4)

                                                                           
                                                                                
                                                                                
#define MBEDTLS_ERR_MPS_READER_INCONSISTENT_REQUESTS MBEDTLS_MPS_READER_MAKE_ERROR(0x5)

                                                                                 
                                                                                   
#define MBEDTLS_ERR_MPS_READER_NEED_ACCUMULATOR      MBEDTLS_MPS_READER_MAKE_ERROR(0x6)

                                                                          
                                                                         
                                                                               
#define MBEDTLS_ERR_MPS_READER_ACCUMULATOR_TOO_SMALL MBEDTLS_MPS_READER_MAKE_ERROR(0x7)

                                             

#endif                          
