   
                                      
  
                                                                         
                                                                             
                                                                             
                                              
  
                                                                       
                                                                        
                                                                       
                                                                        
                                                                        
  
                                                                     
                                  
                                                                              
                                                                           
                                                                             
                                                                            
                                          
                                                                         
                                                                              
                                                                         
                                  
                                                                         
                                                                  
                                                                        
                                                                   
                                                                         
                                                                             
                                                                            
                                                                              
               
                                                                           
                                                                    
                                                                         
                                                                               
                                                                           
               
                                                                           
                                                                              
                                                                             
                     
                                                                       
                                                                           
                           
                                                                               
                                                                          
                                                                            
                                                                        
                                                                        
                                                            
                                                                          
                                                                             
                                                                           
                                                                               
                             
   

/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_BIGNUM_MOD_RAW_H
#define MBEDTLS_BIGNUM_MOD_RAW_H

#include "common.h"

#if defined(MBEDTLS_BIGNUM_C)
#include "mbedtls/bignum.h"
#endif

#include "bignum_mod.h"

   
                                                                          
                                                   
  
                                          
  
                                                           
                                                                           
                                                     
                                                                                
                                                                            
                                                                     
                                                                 
                                                               
                                                                
  
                                                                             
                                                  
  
                                                                               
                                                                             
                                                                    
   
void mbedtls_mpi_mod_raw_cond_assign(mbedtls_mpi_uint *X,
                                     const mbedtls_mpi_uint *A,
                                     const mbedtls_mpi_mod_modulus *N,
                                     unsigned char assign);

   
                                                                            
                                             
  
                                          
  
                                                                               
                                                                                
                                                                            
                                                                 
                                                               
                                                                 
                                                                          
  
                                                                             
                                            
  
                                                                             
                                                                             
                                                                   
   
void mbedtls_mpi_mod_raw_cond_swap(mbedtls_mpi_uint *X,
                                   mbedtls_mpi_uint *Y,
                                   const mbedtls_mpi_mod_modulus *N,
                                   unsigned char swap);

                                       
  
                                                                            
                                             
  
                                                                               
                                                                              
                                          
                                                                   
                                                        
                                                        
                                                                         
  
                                    
                                                                
                                                            
                                                                               
                                                                  
   
int mbedtls_mpi_mod_raw_read(mbedtls_mpi_uint *X,
                             const mbedtls_mpi_mod_modulus *N,
                             const unsigned char *input,
                             size_t input_length,
                             mbedtls_mpi_mod_ext_rep ext_rep);

                                       
  
                                                                               
                                                                              
                                          
                                                                   
                                                       
                                                         
                                                                                                    
  
                                    
                                                                     
                                                        
                                                                               
                                    
   
int mbedtls_mpi_mod_raw_write(const mbedtls_mpi_uint *A,
                              const mbedtls_mpi_mod_modulus *N,
                              unsigned char *output,
                              size_t output_length,
                              mbedtls_mpi_mod_ext_rep ext_rep);

                                                                         
                    
  
                                                                           
                                    
  
                                                                         
                    
  
                                                      
                                                                           
                                                           
                                                                               
                                                                                
                                                                            
                                                                   
   
void mbedtls_mpi_mod_raw_sub(mbedtls_mpi_uint *X,
                             const mbedtls_mpi_uint *A,
                             const mbedtls_mpi_uint *B,
                             const mbedtls_mpi_mod_modulus *N);

                                                                         
                    
  
                                                        
                                  
  
                                                                            
                                                                            
                    
  
                                                                         
                                                                             
                              
  
                                                                         
                                                
                                                                             
                                                                     
                                                 
                                                     
                                                      
                                                                            
                                                                      
                                                                           
                                                                
                                                           
                                                                         
                                         
   
void mbedtls_mpi_mod_raw_mul(mbedtls_mpi_uint *X,
                             const mbedtls_mpi_uint *A,
                             const mbedtls_mpi_uint *B,
                             const mbedtls_mpi_mod_modulus *N,
                             mbedtls_mpi_uint *T);

   
                                                                             
                                                               
  
                                               
                                                                      
                                                                              
  
                                                                           
                                                                     
                                                     
  
                                                                    
                                                     
   
size_t mbedtls_mpi_mod_raw_inv_prime_working_limbs(size_t AN_limbs);

   
                                                                             
                                                       
  
                                                         
  
                                                                      
                                                
                                                                    
                                                               
                                                                                 
                                   
                                                                 
                                                                                
                                                        
                                                                               
                                                                        
                                                                           
                                   
                                                                            
                                
                                                                         
                                                                               
                               
   
void mbedtls_mpi_mod_raw_inv_prime(mbedtls_mpi_uint *X,
                                   const mbedtls_mpi_uint *A,
                                   const mbedtls_mpi_uint *N,
                                   size_t AN_limbs,
                                   const mbedtls_mpi_uint *RR,
                                   mbedtls_mpi_uint *T);

   
                                                
  
                              
  
                                                                       
                
  
                                                                         
                    
  
                                                       
                                                                        
                                              
                                                                         
                                              
                                               
   
void mbedtls_mpi_mod_raw_add(mbedtls_mpi_uint *X,
                             const mbedtls_mpi_uint *A,
                             const mbedtls_mpi_uint *B,
                             const mbedtls_mpi_mod_modulus *N);

                                                                           
                                                     
  
                                              
                                                       
                                             
                                                                          
                                   
                                          
  
                                       
                                                                   
   
int mbedtls_mpi_mod_raw_canonical_to_modulus_rep(
    mbedtls_mpi_uint *X,
    const mbedtls_mpi_mod_modulus *N);

                                                                      
                                                          
  
                                              
                                                       
                                             
                                                                          
                                   
                                          
  
                                       
                                                                   
   
int mbedtls_mpi_mod_raw_modulus_to_canonical_rep(
    mbedtls_mpi_uint *X,
    const mbedtls_mpi_mod_modulus *N);

                                                  
  
                                                                       
                  
  
                                                                  
                                                                
                      
  
                                                                        
                                                                          
  
                                                                                
                                                                            
                                                                           
                             
                               
                                                                          
                                                                     
                                                              
  
                                      
                                                                            
                                                                          
                                                                         
                                                                         
                                                            
   
int mbedtls_mpi_mod_raw_random(mbedtls_mpi_uint *X,
                               mbedtls_mpi_uint min,
                               const mbedtls_mpi_mod_modulus *N,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng);

                                        
  
                                        
                                                            
                                                                    
                                                 
  
                                    
   
int mbedtls_mpi_mod_raw_to_mont_rep(mbedtls_mpi_uint *X,
                                    const mbedtls_mpi_mod_modulus *N);

                                                       
  
                                        
                                                            
                                                                    
                                                
  
                                    
   
int mbedtls_mpi_mod_raw_from_mont_rep(mbedtls_mpi_uint *X,
                                      const mbedtls_mpi_mod_modulus *N);

                                                 
  
                                                                  
                                    
  
                               
  
                                                           
                                                 
                                                                             
                                                           
                                           
   
void mbedtls_mpi_mod_raw_neg(mbedtls_mpi_uint *X,
                             const mbedtls_mpi_uint *A,
                             const mbedtls_mpi_mod_modulus *N);

#endif                               
