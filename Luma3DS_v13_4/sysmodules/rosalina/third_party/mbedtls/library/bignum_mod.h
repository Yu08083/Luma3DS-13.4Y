   
                            
  
                                                                           
  
                                                                     
                                  
  
                                                                              
                                                                           
                                                                             
                                                                            
                                                                           
                                                                          
                                                                         
                                                                          
                                                                            
                                                                  
                                                                        
                                                                      
                                                                           
                                                                              
                                                                           
                                                                       
                                                                         
                                        
                                                                             
                                                              
                                                                        
                                                                   
                                                                          
                                                                       
                                                                          
                                                                      
                                                                            
                                                                              
               
                                                                           
                                                      
                                                                               
                                                                 
                                                                           
                                                                              
                                                                             
                     
                                                                            
                                                                           
                           
                                                                         
                                                                            
                                                                         
                       
                                                                          
                                                                          
                                                                            
                                                                     
                                                                            
                                                                           
                                                                            
                                                                           
             
   

/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_BIGNUM_MOD_H
#define MBEDTLS_BIGNUM_MOD_H

#include "common.h"

#if defined(MBEDTLS_BIGNUM_C)
#include "mbedtls/bignum.h"
#endif

                                                           
  
                                                                           
                                                          
   
typedef enum {
                                                                           
    MBEDTLS_MPI_MOD_REP_INVALID    = 0,
                                                                            
                                     
    MBEDTLS_MPI_MOD_REP_MONTGOMERY = 2,
                                                                             
                                                             
                                                                                  
                        
                                                          
    MBEDTLS_MPI_MOD_REP_OPT_RED,
} mbedtls_mpi_mod_rep_selector;

                                                                            
                                                                 
typedef enum {
    MBEDTLS_MPI_MOD_EXT_REP_INVALID = 0,
    MBEDTLS_MPI_MOD_EXT_REP_LE      = 8,
    MBEDTLS_MPI_MOD_EXT_REP_BE
} mbedtls_mpi_mod_ext_rep;

typedef struct {
    mbedtls_mpi_uint *p;
    size_t limbs;
} mbedtls_mpi_mod_residue;

typedef struct {
    mbedtls_mpi_uint const *rr;                                         
    mbedtls_mpi_uint mm;                                                       
} mbedtls_mpi_mont_struct;

typedef int (*mbedtls_mpi_modp_fn)(mbedtls_mpi_uint *X, size_t X_limbs);

typedef struct {
    mbedtls_mpi_modp_fn modp;                                                  
} mbedtls_mpi_opt_red_struct;

typedef struct {
    const mbedtls_mpi_uint *p;
    size_t limbs;                                              
    size_t bits;                                           
    mbedtls_mpi_mod_rep_selector int_rep;                                                        
    union rep {
                                                           
        mbedtls_mpi_mont_struct mont;
                                                        
        mbedtls_mpi_opt_red_struct ored;
    } rep;
} mbedtls_mpi_mod_modulus;

                              
  
                                                                    
  
                                                                                 
                                                                                
                                                                            
                                                                    
  
                                                                              
                                                               
  
                                                        
                                                               
                                                                               
                                                                               
                                                          
                                                                         
                                                                               
                                                                           
                                              
                                                                               
                                                 
  
                                   
                                                                              
                                                               
   
int mbedtls_mpi_mod_residue_setup(mbedtls_mpi_mod_residue *r,
                                  const mbedtls_mpi_mod_modulus *N,
                                  mbedtls_mpi_uint *p,
                                  size_t p_limbs);

                                           
  
                                                                           
                                                                        
  
                                                                     
                                                         
  
                                                       
   
void mbedtls_mpi_mod_residue_release(mbedtls_mpi_mod_residue *r);

                                   
  
                                                                        
   
void mbedtls_mpi_mod_modulus_init(mbedtls_mpi_mod_modulus *N);

                              
  
                                                                     
                                                                            
                                                                               
                                                          
                                                             
                                                
  
                                   
   
int mbedtls_mpi_mod_modulus_setup(mbedtls_mpi_mod_modulus *N,
                                  const mbedtls_mpi_uint *p,
                                  size_t p_limbs);

                                                              
  
                                                                     
                                                                            
                                                                               
                                                          
                                                             
                                                
                                                                               
  
                                   
   
int mbedtls_mpi_mod_optred_modulus_setup(mbedtls_mpi_mod_modulus *N,
                                         const mbedtls_mpi_uint *p,
                                         size_t p_limbs,
                                         mbedtls_mpi_modp_fn modp);

                                         
  
                                                                               
  
                                                                
                                                                             
                                                      
  
                                                                     
   
void mbedtls_mpi_mod_modulus_free(mbedtls_mpi_mod_modulus *N);

                                                                             
                    
  
                                                        
                                  
  
                                                                            
                                                                            
                    
  
                                                                         
                                                                             
                              
  
                                                                         
                                                
                                                                             
                                                                     
                                                 
                                                     
                                                      
                                                                            
                                                                      
  
                                   
                                                                            
                                                                 
                                                                           
   
int mbedtls_mpi_mod_mul(mbedtls_mpi_mod_residue *X,
                        const mbedtls_mpi_mod_residue *A,
                        const mbedtls_mpi_mod_residue *B,
                        const mbedtls_mpi_mod_modulus *N);

   
                                                   
  
                              
  
                                                                      
  
                                                                         
                    
  
                                                                        
                                                                 
                                         
  
                                                                       
                                                                           
                                                 
                                                  
                                                                        
                                                               
  
                                       
                                                                            
                                                     
   
int mbedtls_mpi_mod_sub(mbedtls_mpi_mod_residue *X,
                        const mbedtls_mpi_mod_residue *A,
                        const mbedtls_mpi_mod_residue *B,
                        const mbedtls_mpi_mod_modulus *N);

   
                                                                             
  
                                                                            
                                         
  
                               
  
                                                                               
  
                                                                    
                                                                  
                                 
                                      
  
                                      
                                                                          
                                                 
                                                                   
                                                                            
                                                                            
                                                                             
                                                        
   

int mbedtls_mpi_mod_inv(mbedtls_mpi_mod_residue *X,
                        const mbedtls_mpi_mod_residue *A,
                        const mbedtls_mpi_mod_modulus *N);
   
                                                
  
                              
  
                                                                            
                                             
  
                                                                         
                    
  
                                                                        
                                                                 
                                         
  
                                                                           
                                                                           
                                                           
                                                            
                                                                        
                                                            
  
                                       
                                                                            
                                                     
   
int mbedtls_mpi_mod_add(mbedtls_mpi_mod_residue *X,
                        const mbedtls_mpi_mod_residue *A,
                        const mbedtls_mpi_mod_residue *B,
                        const mbedtls_mpi_mod_modulus *N);

                                                  
  
                                                                       
                  
  
                                                                  
                                                                
                      
  
                                                                        
                                                                          
  
                                           
                                                                           
                             
                               
                                                                          
                                                                     
                                                              
  
                                      
                                                                            
                                                                          
                                                                         
                                                                         
                                                            
   
int mbedtls_mpi_mod_random(mbedtls_mpi_mod_residue *X,
                           mbedtls_mpi_uint min,
                           const mbedtls_mpi_mod_modulus *N,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng);

                                      
  
                                                                             
                                                
  
                                                                              
                                                                           
                                                                              
                                     
  
                                                                             
                                                        
                                               
                                                    
                                                  
                                                                     
  
                                    
                                                                
                                                          
                                                              
                                                                             
   
int mbedtls_mpi_mod_read(mbedtls_mpi_mod_residue *r,
                         const mbedtls_mpi_mod_modulus *N,
                         const unsigned char *buf,
                         size_t buflen,
                         mbedtls_mpi_mod_ext_rep ext_rep);

                                       
  
                                                                 
                                                               
  
                                                                               
                                            
  
                                                                      
                                                                   
                                                                         
                                
  
                                                                               
                                                                               
                                                                            
                                                 
                                                                    
                                                   
                                                  
                                                                             
                                      
  
                                    
                                                                  
                                                                        
                         
                                                                          
                                                                          
                                                                      
                                                
   
int mbedtls_mpi_mod_write(const mbedtls_mpi_mod_residue *r,
                          const mbedtls_mpi_mod_modulus *N,
                          unsigned char *buf,
                          size_t buflen,
                          mbedtls_mpi_mod_ext_rep ext_rep);

#endif                           
