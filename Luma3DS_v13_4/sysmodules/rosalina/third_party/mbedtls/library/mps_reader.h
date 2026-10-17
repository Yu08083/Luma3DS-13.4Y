/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

   
                     
  
                                                                     
                                                                     
                                                                     
                                                                      
                                                       
  
                                                             
                                                            
                                                           
                                                    
  
                                                             
                                                                
                                                            
                                       
                                                     
                                                           
                        
  
                                                         
                                                  
                                                              
                                                    
                                                               
                                                                    
                                                                    
                                                                   
                    
                                                                   
                                                              
                                                                 
                    
                      
  
                                                            
                                         
  
                                                           
                                      
                                                             
                                                          
                                                             
                                                                   
                                                         
                                    
                                                        
                                                             
                                                                       
                                                                  
                                          
  
                                                                        
                                                 
                                                                         
                                                                       
                                                                        
                                            
  
                                                                        
                                                                          
                                                                       
                               
                                                                      
                                                  
  
                                                                    
                                                                
                                                                         
                                                                        
  
                                                                
  
                                                  
                                                                               
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                            
                                                                                
                                                                                
                                                                                
                                                                                
                                                                                
                                                                               
                                                                  
                                                            
               
               
                                    
                                
                                            
                                            
                                                                     
                                            
  
   

#ifndef MBEDTLS_READER_H
#define MBEDTLS_READER_H

#include <stdio.h>

#include "mps_common.h"
#include "mps_error.h"

struct mbedtls_mps_reader;
typedef struct mbedtls_mps_reader mbedtls_mps_reader;

  
                        
   

struct mbedtls_mps_reader {
    unsigned char *frag;                                               
                                                                       
                                                                            
                                                                     
                                                                       
                                                                         
                            
                                                                 
                                                                              
    mbedtls_mps_stored_size_t frag_len;
                                            
                                                        
    mbedtls_mps_stored_size_t commit;
                                                
                                              
                                                     
                                                    
                                                     
                                                         
      
                                                
                                                 
                                                        
    mbedtls_mps_stored_size_t end;
                                                
                                             
                                                        
                                                   
                                                     
                                                        
                                                  
                                      
      
                                                
                                                 
                                                        
    mbedtls_mps_stored_size_t pending;
                                                   
                                               
                                                   
                        
                                          
                                                       
                                                 
                                                     
                                                     
                            
                                                
                                                 
                                                        

                                                                    
                                                                 
                                                   

    unsigned char *acc;                                                  
                                                                                  
                                                                               
    mbedtls_mps_stored_size_t acc_len;
                                                        
    mbedtls_mps_stored_size_t acc_available;
                                                  
                                              
                                         
                                               
                                                        
                                                  
                                           
                            
                                                         
    union {
        mbedtls_mps_stored_size_t acc_remaining;
                                                    
                                                     
                                         
                                                         
        mbedtls_mps_stored_size_t frag_offset;
                                                          
                                                      
                                               
                                                      
                                                            
                                               
                                                         
    } acc_share;
};

  
                    
                                                     
                                                       
                                                         
                                                     
   

  
                                
   

   
                                              
  
                                                 
                                                                    
                                                                         
                                                                            
                                                                        
                                                                          
                                                                              
                                                                             
                                                                           
                                                                        
                                                
  
                                    
                                                                               
   
int mbedtls_mps_reader_init(mbedtls_mps_reader *reader,
                            unsigned char *acc,
                            mbedtls_mps_size_t acc_len);

   
                                        
  
                                           
  
                                    
                                                                               
   
int mbedtls_mps_reader_free(mbedtls_mps_reader *reader);

   
                                                                
  
                                                                 
                                      
                                                           
                                                   
  
                                                                     
                                                                   
                                                                
                                                                     
                                                                         
                                                                         
                                                     
                                                                             
                                                                                       
                                                                          
                                                                               
                                     
                                                                            
                                                
   
int mbedtls_mps_reader_feed(mbedtls_mps_reader *reader,
                            unsigned char *buf,
                            mbedtls_mps_size_t buflen);

   
                                                                        
  
                                                                 
                                      
                                                                            
                                                                           
                                                                              
                                                                 
                                                                           
                                 
  
                                    
                                                                               
   
int mbedtls_mps_reader_reclaim(mbedtls_mps_reader *reader,
                               int *paused);

  
                          
   

   
                                                 
  
                                                              
                                         
                                                                    
                                                               
                                             
                                                          
                                          
  
                                                                    
                                                          
                                                           
                                                                       
                                                                        
                                                                        
                                                                              
                                                                              
                                                                                 
                                                                     
                                                                               
                                                                             
                                                                         
                                                                          
                                                                         
                                                    
                                                                    
                                                        
  
                                                                       
                                                                 
                                                                   
                                                                           
                               
   
int mbedtls_mps_reader_get(mbedtls_mps_reader *reader,
                           mbedtls_mps_size_t desired,
                           unsigned char **buffer,
                           mbedtls_mps_size_t *buflen);

   
                                                                                
  
                                                                                
                                                                     
                                                                        
                                                 
  
                                                                   
                                                                        
                                                                       
                                                                     
  
                                            
  
                                  
                                                                             
  
   
int mbedtls_mps_reader_commit(mbedtls_mps_reader *reader);

#endif                       
