   
                     
                                                            
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_H
#define PSA_CRYPTO_H

#if defined(MBEDTLS_PSA_CRYPTO_PLATFORM_FILE)
#include MBEDTLS_PSA_CRYPTO_PLATFORM_FILE
#else
#include "crypto_platform.h"
#endif

#include <stddef.h>

#ifdef __DOXYGEN_ONLY__
                                                                        
                                                                          
                                                                       
                                                    

                                                          
     
   

       
#endif                       

#ifdef __cplusplus
extern "C" {
#endif

                                                               
                                           
#include "crypto_types.h"

                                 
     
   

   
                                                                 
   
#define PSA_CRYPTO_API_VERSION_MAJOR 1

   
                                                                 
   
#define PSA_CRYPTO_API_VERSION_MINOR 0

       

                                                                         
                                                    
#include "crypto_values.h"

                                                                      
                                                           
#include "crypto_sizes.h"

                                                      
                                                              
#if defined(MBEDTLS_PSA_CRYPTO_STRUCT_FILE)
#include MBEDTLS_PSA_CRYPTO_STRUCT_FILE
#else
#include "crypto_struct.h"
#endif

                                                   
     
   

   
                                 
  
                                                                
                           
  
                                                                  
                                                        
  
                                                                             
                                                                              
                                                                    
                                                                      
                                                                    
                                                                             
                                          
  
                                         
                                                           
                                                            
                                                             
                                                        
                                                           
                                                            
                                                       
                                                    
                                                    
   
psa_status_t psa_crypto_init(void);

       

                          
     
   

                                
  
                                                                          
                                 
   

                                                           
   
static psa_key_attributes_t psa_key_attributes_init(void);

                                                           
  
                                                                           
                                                                        
                                                          
  
                                                                    
                          
                                                                   
                                                         
                                                                   
                                                                          
                     
  
                                                                   
                                                                    
                                                                        
  
                                                               
                                                                 
                                                                  
                                                                       
                                     
   
static void psa_set_key_id(psa_key_attributes_t *attributes,
                           mbedtls_svc_key_id_t key);

#ifdef MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER
                                      
  
                                                                           
                                                                      
                                                      
  
                                                                          
                                                                         
                                                                               
                      
  
                                                               
                                                    
   
static void mbedtls_set_key_owner_id(psa_key_attributes_t *attributes,
                                     mbedtls_key_owner_id_t owner);
#endif

                                         
  
                                                                         
                                                                            
                                                      
                                                                       
                                                      
  
                                                                    
                          
                                                                   
                                                         
                                                                   
                                                                          
                     
  
                                                                   
                                                                    
                                                                        
  
                                                                    
                                                         
                                                                          
                                                                            
                                                        
   
static void psa_set_key_lifetime(psa_key_attributes_t *attributes,
                                 psa_key_lifetime_t lifetime);

                                                    
  
                                                                   
                                                                    
                                                               
  
                                                                     
  
                                                                       
                                                                        
                               
   
static mbedtls_svc_key_id_t psa_get_key_id(
    const psa_key_attributes_t *attributes);

                                              
  
                                                                   
                                                                    
                                                               
  
                                                                     
  
                                                                
   
static psa_key_lifetime_t psa_get_key_lifetime(
    const psa_key_attributes_t *attributes);

                                  
  
                                                                 
                                                                 
                                                           
  
                                           
                                   
  
                                                                   
                                                                    
                                                                        
  
                                                                    
                                                         
   
static void psa_set_key_usage_flags(psa_key_attributes_t *attributes,
                                    psa_key_usage_t usage_flags);

                                                 
  
                                                                   
                                                                    
                                                               
  
                                                                     
  
                                                             
   
static psa_key_usage_t psa_get_key_usage_flags(
    const psa_key_attributes_t *attributes);

                                                     
  
                                                                     
                                                                   
                                    
                                                                       
                                                                    
                                   
                                                          
                                                                            
                                              
                                                                     
                                                                 
                                                                        
                                       
                                                                        
                                                                        
                                                                        
                                       
  
                                                
                                   
  
                                                                   
                                                                    
                                                                        
  
                                                                    
                                                                        
   
static void psa_set_key_algorithm(psa_key_attributes_t *attributes,
                                  psa_algorithm_t alg);


                                                      
  
                                                                   
                                                                    
                                                               
  
                                                                     
  
                                                           
   
static psa_algorithm_t psa_get_key_algorithm(
    const psa_key_attributes_t *attributes);

                              
  
                                        
                                   
  
                                                                   
                                                                    
                                                                        
  
                                                                    
                                                      
                                                                           
                                                    
   
static void psa_set_key_type(psa_key_attributes_t *attributes,
                             psa_key_type_t type);


                              
  
                                                                         
  
                                                                   
                                                                    
                                                                        
  
                                                                    
                                                     
                                                                           
                                                                       
                                              
   
static void psa_set_key_bits(psa_key_attributes_t *attributes,
                             size_t bits);

                                              
  
                                                                   
                                                                    
                                                               
  
                                                                     
  
                                                          
   
static psa_key_type_t psa_get_key_type(const psa_key_attributes_t *attributes);

                                              
  
                                                                   
                                                                    
                                                               
  
                                                                     
  
                                                                   
   
static size_t psa_get_key_bits(const psa_key_attributes_t *attributes);

                                     
  
                                                             
                                                               
                                                    
  
                                                              
                                                                      
                                                                          
  
                                                               
                                                                      
                                                           
                                                              
  
                                         
                                                      
                                                           
                                                             
                                                           
                                                       
                                                    
                                                    
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_get_key_attributes(mbedtls_svc_key_id_t key,
                                    psa_key_attributes_t *attributes);

                                                                   
  
                                                                  
                                                                      
                                                                       
                        
  
                                                                 
               
  
                                                                 
   
void psa_reset_key_attributes(psa_key_attributes_t *attributes);

       

                                           
     
   

                                                            
  
                                                                              
                                     
  
                                                                             
                                                                         
                                                                            
  
                                             
  
                       
                                                                           
                              
                                      
                                                
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_purge_key(mbedtls_svc_key_id_t key);

                         
  
                                                  
  
                                                                    
                                                               
                                                   
  
                                                                   
                                                                 
  
                                                        
                           
                                                                         
                                                              
                                                                  
                                                                     
                                                                    
                                                                 
                                                
  
                                                               
                                                                  
                               
                                                                    
                                                                           
                                                       
                                                                       
                                                                       
                                                                           
                                                 
                                                                        
                                                       
  
                                                                      
                          
  
                                                                    
                                                                              
                                                                            
                                                             
                                                           
                                                     
                                                                          
                                                                      
                                                          
                                                                     
                                                                     
                                            
                                                                     
                                                                 
                                                                      
                                                                 
                                                                              
                                                                           
                                                                     
                                                                
                                            
  
                                         
                                    
                                    
                                    
                                                                      
                                                              
                                      
                                                                      
                                                                
                                             
                                                         
                                                                 
                                   
                                                                              
                                                                     
                                                     
                                                           
                                                            
                                                             
                                                        
                                                    
                                                    
                                                       
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_copy_key(mbedtls_svc_key_id_t source_key,
                          const psa_key_attributes_t *attributes,
                          mbedtls_svc_key_id_t *target_key);


   
                        
  
                                                  
                                                                         
                                                                          
  
                                                                    
                                     
  
                                                                             
                                                  
  
                                                                   
                                                                      
                                                                              
                                                                         
  
                                                                              
                                   
  
                       
                                                                     
                                                                      
                                   
                                                 
                                                                             
                                    
                                                     
                                           
                                                                         
                                                                        
                                  
                                                                             
                                                                         
                                                             
                                     
                                                                             
                                                                         
                                                                          
                                                         
                                         
                                                                       
                                                                         
                            
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key);

       

                                                 
     
   

   
                                        
  
                                                                        
                                                                         
                                                                  
                   
  
                                                                      
                                                                     
                                                                     
                                                     
  
                                                                    
  
                                                                 
                                                                    
                                                                  
                                                                  
                                                                      
                                   
  
                                                           
                                                                      
                                           
                                                                        
                                                                      
                                                                               
                                                                           
                                                     
                                            
                                                                         
                                                                          
                                       
                                                                          
                                                    
                                                                        
                                                                      
                                                                          
                                                                       
                                                                           
                                                          
  
                       
                   
                                                                            
                                                 
                                    
                                                                      
                                                              
                                   
                                                                   
                                                                               
                                      
                                                          
                                                      
                                                                           
                           
                                                           
                                                            
                                                             
                                                    
                                                    
                                                       
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_import_key(const psa_key_attributes_t *attributes,
                            const uint8_t *data,
                            size_t data_length,
                            mbedtls_svc_key_id_t *key);



   
                                        
  
                                                                   
                               
  
                                                                   
                                                                     
                                                               
                  
  
                                                           
  
                                                               
                          
                                                                       
             
                                                           
                           
                                                               
                                                                       
                                                      
        
                                 
                                                   
                                           
                                           
                                           
                                           
                                           
                                                     
                                                     
                                                              
      
        
                                                      
                                                          
                                                                          
                                                                           
                                                                      
                                                              
                                                                        
                                                                           
                                          
                                                                             
                                                                           
                                                                             
                                                                          
                                                                          
                                                                   
                                               
                                                                             
                                                                           
                                       
                                                                        
                                                                  
  
                                                                            
  
                                                                              
                                                                              
                                
                                                                       
                                                                
                                                           
                                                      
  
                                         
                                                      
                                   
                                                                
                                                     
                                      
                                                                           
                                            
                                                        
                                        
                                               
                                                             
                                                        
                                                           
                                                       
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_export_key(mbedtls_svc_key_id_t key,
                            uint8_t *data,
                            size_t data_size,
                            size_t *data_length);

   
                                                                                
  
                                                                   
                                                         
  
                                                                 
                                                                    
                                                                  
                                                                  
                                                                      
                                   
  
                                                           
                                                                            
                                                                          
        
                                
                                           
                                           
        
                                                                            
                                                                             
                                                                           
                
                                                                          
                                                              
                                                                             
                                                                             
                                                                            
                                                                
                        
                                                            
                                                            
                                                                     
                                             
                                                                            
                                                                               
                             
  
                                                                    
                                                         
  
                                                            
                                                                       
                                                                
                                                           
                                                      
  
                                         
                                                      
                                      
                                                          
                                                     
                                      
                                                                           
                                            
                                                                                              
                                        
                                               
                                                             
                                                        
                                                           
                                                       
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_export_public_key(mbedtls_svc_key_id_t key,
                                   uint8_t *data,
                                   size_t data_size,
                                   size_t *data_length);



       

                                  
     
   

                                             
  
                                                   
                                                        
  
                                                                               
                                                                        
                                                                  
                                                                 
                                                                   
                                                                
                                                           
                                                                       
                                                     
  
                       
                   
                                   
                                                              
                                                        
                                      
                                    
                                                           
                                                             
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_hash_compute(psa_algorithm_t alg,
                              const uint8_t *input,
                              size_t input_length,
                              uint8_t *hash,
                              size_t hash_size,
                              size_t *hash_length);

                                                                  
                   
  
                                                                               
                                                                        
                                                                  
                                                                 
                                                                      
                                                                
  
                       
                                                                          
                                       
                                                                      
                                          
                                   
                                                              
                                      
                                                                                  
                                                           
                                                             
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_hash_compare(psa_algorithm_t alg,
                              const uint8_t *input,
                              size_t input_length,
                              const uint8_t *hash,
                              size_t hash_length);

                                                                       
  
                                                                               
                                               
                                                     
          
                                    
                                              
             
                                                                  
          
                                          
             
                                                                          
                 
          
                                                              
             
                                                                
                                   
          
                                    
                                           
             
  
                                                                       
                                                            
                                                                          
typedef struct psa_hash_operation_s psa_hash_operation_t;

                                
  
                                                                        
                                 
   

                                                        
   
static psa_hash_operation_t psa_hash_operation_init(void);

                                      
  
                                                                  
                 
                                                                            
                  
                                                                              
                                                                             
                                                     
                                                                        
                                                                       
                                                      
                                                    
                                                                         
  
                                                                       
                                                                     
                                                                        
                        
  
                                                                    
                                                                        
             
                                                                 
                                
  
                                                                        
                                                                         
                                                                     
                                                                               
                                                                        
  
                       
                   
                                   
                                                    
                                      
                                          
                                                           
                                                             
                                                        
                                                           
                               
                                                                     
                                                                                
                                                                         
                                      
   
psa_status_t psa_hash_setup(psa_hash_operation_t *operation,
                            psa_algorithm_t alg);

                                                         
  
                                                                           
  
                                                                          
                                                         
  
                                                  
                                                                           
                                                                 
  
                       
                   
                                                           
                                                             
                                                        
                                                           
                               
                                                                   
                                                                                
                                                                         
                                      
   
psa_status_t psa_hash_update(psa_hash_operation_t *operation,
                             const uint8_t *input,
                             size_t input_length);

                                                    
  
                                                                           
                                                                           
                                                             
  
                                                                           
                                                                          
                                                         
  
                                                                     
                                                                          
                                                                        
                                                                  
                                                                         
                                                                        
                                                              
  
                                                      
                                                                       
                                                                    
                                                               
                                                                           
                                                                            
                                                                  
  
                       
                   
                                      
                                                                           
                                                                     
                                                                 
                                                           
                                                             
                                                        
                                                           
                               
                                                                   
                                                                                
                                                                         
                                      
   
psa_status_t psa_hash_finish(psa_hash_operation_t *operation,
                             uint8_t *hash,
                             size_t hash_size,
                             size_t *hash_length);

                                                                       
                     
  
                                                                           
                                                                           
                                                                     
                                                                  
                              
  
                                                                           
                                                                          
                                                         
  
                                                                      
                                                                        
                    
  
                                                      
                                                                          
                                                                    
  
                       
                                                                            
                                       
                                                                      
                                          
                                                           
                                                             
                                                        
                                                           
                               
                                                                   
                                                                                
                                                                         
                                      
   
psa_status_t psa_hash_verify(psa_hash_operation_t *operation,
                             const uint8_t *hash,
                             size_t hash_length);

                           
  
                                                                      
                                                                    
                                                 
                          
  
                                                                     
                                                                             
  
                                                                       
                                                                 
                                               
  
                                                           
  
                                         
                                                             
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_hash_abort(psa_hash_operation_t *operation);

                           
  
                                                                 
                                                                      
                                                                   
                                                          
                                                                    
                                                                           
                                                                      
                                              
  
                                                                       
                                                                   
                                                                          
  
                                         
                                                             
                                                        
                                                           
                                                           
                               
                                                                             
                                                                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_hash_clone(const psa_hash_operation_t *source_operation,
                            psa_hash_operation_t *target_operation);

       

                                              
     
   

                                                                 
  
                                                  
                                                      
                                                                     
                                                              
                                                                      
                                                                   
                                                          
  
                                                                              
                                                                            
                                                                              
                                                                       
                                                                
                                                                 
                                                                        
                                                               
                                                           
                                                       
  
                       
                   
                                                      
                                                     
                                      
                                                
                                   
                                                             
                                      
                                   
                                                           
                                                             
                                                        
                                                           
                                     
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_mac_compute(mbedtls_svc_key_id_t key,
                             psa_algorithm_t alg,
                             const uint8_t *input,
                             size_t input_length,
                             uint8_t *mac,
                             size_t mac_size,
                             size_t *mac_length);

                                                                         
  
                                                                              
                                                                              
                                                                              
                                                                       
                                                                
                                                                 
                                                                     
                                                               
  
                       
                                                                        
                                       
                                                                     
                                           
                                                      
                                                     
                                      
                                                
                                   
                                                             
                                                           
                                                             
                                                        
                                                           
                                     
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_mac_verify(mbedtls_svc_key_id_t key,
                            psa_algorithm_t alg,
                            const uint8_t *input,
                            size_t input_length,
                            const uint8_t *mac,
                            size_t mac_length);

                                                                      
  
                                                                              
                                               
                                                     
          
                                   
                                              
             
                                                                  
          
                                         
             
                                                                         
                 
          
                                                            
             
                                                               
                                   
          
                                   
                                          
             
  
  
                                                                       
                                                            
                                                                          
typedef struct psa_mac_operation_s psa_mac_operation_t;

                               
  
                                                                               
                        
   

                                                       
   
static psa_mac_operation_t psa_mac_operation_init(void);

                                                 
  
                                                   
                                                  
                                            
                                                      
  
                                                               
                                                                            
                  
                                                                              
                                                                           
                                                                 
                                                                       
                                                                     
                                                      
                                                                     
                                                
  
                                                                           
                                                                    
                                                                       
                        
  
                                                                        
                                                                           
                                                
                               
  
                                                                        
                                                                         
                                                                    
                                                                              
                                                                             
                                                                               
                                                                              
                                                                       
  
                       
                   
                                                      
                                                     
                                      
                                                
                                   
                                                             
                                                           
                                                             
                                                        
                                                           
                                     
                                                       
                               
                                                                     
                                                                                
                                                                         
                                      
   
psa_status_t psa_mac_sign_setup(psa_mac_operation_t *operation,
                                mbedtls_svc_key_id_t key,
                                psa_algorithm_t alg);

                                                  
  
                                                    
                                                                            
  
                                                            
                                                                            
                  
                                                                              
                                                                           
                                                                   
                                                                       
                                                                     
                                                      
                                                                       
                                                                     
                         
  
                                                                             
                                                                    
                                                                       
                        
  
                                                                          
                                                                           
                                                  
                               
  
                                                                        
                                                                         
                                                                    
                                                                              
                                                                             
                                                   
                                                         
                                                                              
                                                                       
  
                       
                   
                                                      
                                                     
                                      
                                                
                                   
                                                             
                                                           
                                                             
                                                        
                                                           
                                     
                                                       
                               
                                                                     
                                                                                
                                                                         
                                      
   
psa_status_t psa_mac_verify_setup(psa_mac_operation_t *operation,
                                  mbedtls_svc_key_id_t key,
                                  psa_algorithm_t alg);

                                                        
  
                                                                           
                                
  
                                                                          
                                                        
  
                                                 
                                                                            
                                                
                                                                 
  
                       
                   
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                   
                                                                                
                                                                         
                                      
   
psa_status_t psa_mac_update(psa_mac_operation_t *operation,
                            const uint8_t *input,
                            size_t input_length);

                                                   
  
                                                                               
                                                                          
                                                            
  
                                                                           
                                                                          
                                                        
  
                                                                     
                                                                               
                                                                        
                                                                 
                                                                         
                                                                      
                                                             
  
                                                 
                                                                        
                                                               
                                                           
                                                                      
                                                                             
                                                                              
                                                                              
                                                             
  
                       
                   
                                      
                                                                          
                                                              
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                          
                                                                         
                                
                                                                         
                                      
   
psa_status_t psa_mac_sign_finish(psa_mac_operation_t *operation,
                                 uint8_t *mac,
                                 size_t mac_size,
                                 size_t *mac_length);

                                                                      
                     
  
                                                                                 
                                                                          
                                                                    
                                                                
                              
  
                                                                           
                                                                          
                                                        
  
                                                                      
                                                                      
                    
  
                                                 
                                                                     
                                                               
  
                       
                                                                          
                                       
                                                                     
                                         
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                            
                                                                         
                                
                                                                         
                                      
   
psa_status_t psa_mac_verify_finish(psa_mac_operation_t *operation,
                                   const uint8_t *mac,
                                   size_t mac_length);

                          
  
                                                                      
                                                                    
                                                 
                                                        
  
                                                                     
                                                                            
  
                                                                      
                                                                    
                                                     
  
                                                      
  
                                         
                                                             
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_mac_abort(psa_mac_operation_t *operation);

       

                                      
     
   

                                               
  
                                                                    
                                                        
                                                               
  
                                                                               
                                                                               
                                                               
                                                               
                                                                    
                                                                         
                                                                     
                                                                         
                                                                      
                                                      
                                                                      
                                                               
                                                        
  
                       
                   
                                                      
                                                     
                                      
                                                
                                   
                                                                
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_cipher_encrypt(mbedtls_svc_key_id_t key,
                                psa_algorithm_t alg,
                                const uint8_t *input,
                                size_t input_length,
                                uint8_t *output,
                                size_t output_size,
                                size_t *output_length);

                                               
  
                                                                      
  
                                                                               
                                                                        
                                                                   
                                                       
                                                               
                                                               
                                                                    
                                                                         
                                                                       
                                                  
                                                                     
                                                                            
                                                                      
                                                               
                                                        
  
                       
                   
                                                      
                                                     
                                      
                                                
                                   
                                                                
                                                        
                                                           
                                                             
                                                        
                                                       
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_cipher_decrypt(mbedtls_svc_key_id_t key,
                                psa_algorithm_t alg,
                                const uint8_t *input,
                                size_t input_length,
                                uint8_t *output,
                                size_t output_size,
                                size_t *output_length);

                                                                         
  
                                                                            
                                                    
                                                     
          
                                      
                                              
             
                                                                  
          
                                            
             
                                                                            
                 
          
                                                                  
             
                                                                  
                                   
          
                                      
                                             
             
  
                                                                       
                                                            
                                                                          
typedef struct psa_cipher_operation_s psa_cipher_operation_t;

                                  
  
                                                                             
                                
   

                                                          
   
static psa_cipher_operation_t psa_cipher_operation_init(void);

                                                               
  
                                                                          
                 
                                                                            
                  
                                                                              
                                                     
                                 
                                                                       
                                                                    
                                                                    
                                                                       
                                   
                                                                          
                               
                               
  
                                                                             
                                                                           
                                                                          
                        
  
                                                                              
                                                                        
             
                                              
                                  
  
                                                                            
                                                                             
                                                                           
                                                                               
                                                                        
                                                                   
                                                       
                                                               
                                                               
                                                                    
  
                       
                   
                                                      
                                                     
                                      
                                                
                                   
                                                                
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                     
                                                                                
                                                                         
                                      
   
psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation,
                                      mbedtls_svc_key_id_t key,
                                      psa_algorithm_t alg);

                                                               
  
                                                                          
                 
                                                                            
                  
                                                                              
                                                     
                                 
                                                                       
                                                                          
                                                                        
                                                                       
                               
                                                                          
                               
                               
  
                                                                             
                                                                           
                                                                          
                        
  
                                                                              
                                                                        
             
                                              
                                  
  
                                                                            
                                                                             
                                                                           
                                                                               
                                                                        
                                                                   
                                                       
                                                               
                                                               
                                                                    
  
                       
                   
                                                      
                                                     
                                      
                                                
                                   
                                                                
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                     
                                                                                
                                                                         
                                      
   
psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation,
                                      mbedtls_svc_key_id_t key,
                                      psa_algorithm_t alg);

                                                        
  
                                                                     
                                                                       
                                                   
  
                                                              
                         
  
                                                                          
                                                           
  
                                                        
                                                                               
                                                                  
                                                                      
                                             
  
                       
                   
                                      
                                                     
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                                
                                                             
                                
                                                                         
                                      
   
psa_status_t psa_cipher_generate_iv(psa_cipher_operation_t *operation,
                                    uint8_t *iv,
                                    size_t iv_size,
                                    size_t *iv_length);

                                                                  
  
                                                           
                                                                       
  
                                                              
                         
  
                                                                          
                                                           
  
                                                                          
                                                                         
                   
  
                                                        
                                                                
                                                        
  
                       
                   
                                      
                                                                        
                                                      
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                        
                                                                          
                                                       
                                                                         
                                      
   
psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation,
                               const uint8_t *iv,
                               size_t iv_length);

                                                                        
  
                                          
                                                                           
                                                                   
                                     
                                                                    
                                                           
  
                                                                          
                                                           
  
                                                        
                                                                         
                                                   
                                                                     
                                                                         
                                                                      
                                                               
                                                                 
  
                       
                   
                                      
                                                         
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                              
                                                                      
                                                       
                                                                         
                                      
   
psa_status_t psa_cipher_update(psa_cipher_operation_t *operation,
                               const uint8_t *input,
                               size_t input_length,
                               uint8_t *output,
                               size_t output_size,
                               size_t *output_length);

                                                                    
  
                                                          
                                                                      
                                                                 
                      
  
                                                                     
                                                                  
                       
  
                                                                           
                                                                          
                                                           
  
                                                        
                                                                         
                                                                      
                                                               
                                                                 
  
                       
                   
                                      
                                                                         
                                                                           
                                                                         
                                                                
                                     
                                                                        
                                                                      
                                      
                                                         
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                              
                                                                      
                                                       
                                                                         
                                      
   
psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation,
                               uint8_t *output,
                               size_t output_size,
                               size_t *output_length);

                             
  
                                                                      
                                                                    
                                                 
                                                                  
  
                                                                     
                                                            
  
                                                                         
                                                                    
                             
  
                                                             
  
                                         
                                                             
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation);

       

                                                                       
     
   

                                                  
  
                                                                      
                                                                    
                                                         
                                                               
                                                                 
                                                                    
                                                     
                                                                       
                                                                            
                                                    
                                                                      
                                                                     
                                            
                                                                
                                                                           
                                                                     
                                                                         
                                                                            
                                                                               
                                                                           
                                                                      
                                                                       
                                                 
                                                                            
                                                                           
                                                    
                                                               
                                                                              
                                                                      
                                                                      
                                                                        
                                                                              
                                                                         
                                               
                                                                    
                                                              
  
                       
                   
                                                      
                                                     
                                      
                                                                       
                                        
                                   
                                                               
                                                           
                                      
                                           
                                                             
                                  
                                                                                
                                              
                                                             
                                                        
                                                           
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_aead_encrypt(mbedtls_svc_key_id_t key,
                              psa_algorithm_t alg,
                              const uint8_t *nonce,
                              size_t nonce_length,
                              const uint8_t *additional_data,
                              size_t additional_data_length,
                              const uint8_t *plaintext,
                              size_t plaintext_length,
                              uint8_t *ciphertext,
                              size_t ciphertext_size,
                              size_t *ciphertext_length);

                                                  
  
                                                                      
                                                                    
                                                         
                                                               
                                                                 
                                                                    
                                                     
                                                                       
                                                                             
                                                    
                                                                      
                                                                      
                                                                     
                                                                           
                                                                            
                                                                          
                                                            
                                                                 
                                                                     
                                                                      
                                                                     
                                                          
                                                                       
                                                                           
                                                                           
                                                    
                                                               
                                                                              
                                                                       
                                                                      
                                                                        
                                                                               
                                                                        
                                               
                                                                    
                                                             
  
                       
                   
                                                      
                                       
                                           
                                                     
                                      
                                                                             
                                        
                                   
                                                               
                                                           
                                      
                                          
                                                             
                                   
                                                                              
                                                 
                                                             
                                                        
                                                           
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_aead_decrypt(mbedtls_svc_key_id_t key,
                              psa_algorithm_t alg,
                              const uint8_t *nonce,
                              size_t nonce_length,
                              const uint8_t *additional_data,
                              size_t additional_data_length,
                              const uint8_t *ciphertext,
                              size_t ciphertext_length,
                              uint8_t *plaintext,
                              size_t plaintext_size,
                              size_t *plaintext_length);

                                                                       
  
                                                                           
                                                    
                                                     
          
                                    
                                              
             
                                                                  
          
                                          
             
                                                                          
                 
          
                                                              
             
                                                                
                                   
          
                                    
                                           
             
  
                                                                       
                                                            
                                                                          
typedef struct psa_aead_operation_s psa_aead_operation_t;

                                
  
                                                                            
                              
   

                                                         
   
static psa_aead_operation_t psa_aead_operation_init(void);

                                                                   
  
                                                                      
                 
                                                                            
                  
                                                                              
                                                   
                               
                                                                     
                                                                         
                                                                
                                                                        
                  
                                                                      
                                               
                                                                        
                                      
                                                                           
                                                                   
                                                                        
                                          
                             
  
                                                                           
                                                                         
                                                                        
                        
  
                                                                            
                                                                        
             
                                            
                                
  
                                                                            
                                                                             
                                                                         
                                                                               
                                                                        
                                                                   
                                                       
                                                             
                                                               
                                                                  
  
                       
                   
                               
                                                                     
                                                                                
                                                      
                                                     
                                      
                                                
                                   
                                                               
                                                           
                                                             
                                                        
                                                           
                                     
                                                                                
                                                                         
                                      
   
psa_status_t psa_aead_encrypt_setup(psa_aead_operation_t *operation,
                                    mbedtls_svc_key_id_t key,
                                    psa_algorithm_t alg);

                                                                   
  
                                                                      
                 
                                                                            
                  
                                                                              
                                                   
                               
                                                                     
                                                                         
                                                                
                                                                        
                  
                                                                  
                                                                           
                                                                   
                                                                        
                                             
                             
  
                                                                           
                                                                         
                                                                        
                        
  
                                                                            
                                                                        
             
                                            
                                
  
                                                                            
                                                                             
                                                                         
                                                                               
                                                                        
                                                                   
                                                       
                                                             
                                                               
                                                                  
  
                       
                   
                                                      
                                                     
                                      
                                                
                                   
                                                               
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                         
                                                                            
                                                                         
                                      
   
psa_status_t psa_aead_decrypt_setup(psa_aead_operation_t *operation,
                                    mbedtls_svc_key_id_t key,
                                    psa_algorithm_t alg);

                                                                      
  
                                                                          
                                                                        
                
  
                                                            
                         
  
                                                                          
                                                         
  
                                                      
                                                                         
                                        
                                                                     
                                                                      
                                                
  
                       
                   
                                      
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                              
                                                                     
                                                       
                                                                         
                                      
   
psa_status_t psa_aead_generate_nonce(psa_aead_operation_t *operation,
                                     uint8_t *nonce,
                                     size_t nonce_size,
                                     size_t *nonce_length);

                                                                          
  
                                                     
                                      
  
                                                        
                                                         
  
                                                                          
                                                         
  
                                                                           
                                                                         
                   
  
                                                      
                                                                   
                                                           
  
                       
                   
                                      
                                                                           
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                             
                                                                   
                                
                                                                         
                                      
   
psa_status_t psa_aead_set_nonce(psa_aead_operation_t *operation,
                                const uint8_t *nonce,
                                size_t nonce_length);

                                                                    
  
                                                         
                                                                 
                                                                   
                                                                    
                                                    
  
                                                                    
                                                     
  
                                                         
                                                                  
                                    
                                                                         
                                   
                                                                     
  
                                                                          
                                                         
  
                                                      
                                                                    
                                                            
                                                                          
  
                       
                   
                                      
                                                                       
                     
                                                           
                                                             
                                                        
                                                           
                               
                                                                   
                                                                        
                                                                          
                                
                                                                         
                                      
   
psa_status_t psa_aead_set_lengths(psa_aead_operation_t *operation,
                                  size_t ad_length,
                                  size_t plaintext_length);

                                                     
  
                                                       
  
                                                                         
                                                                       
                                                     
  
                                          
                                                                       
                                                                           
  
                                                                          
                                                         
  
                                                                               
                                                                           
                                                                               
                                                                            
                                                                              
  
                                                      
                                                                 
                                                
                                                                     
  
                       
                   
                                      
                                                                           
                                                                
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                            
                                                                  
                                                                           
                                                                    
                                                                         
                                      
   
psa_status_t psa_aead_update_ad(psa_aead_operation_t *operation,
                                const uint8_t *input,
                                size_t input_length);

                                                                      
  
                                          
                                                                       
                                                                   
                                     
                                                                           
                                                                
  
                                                                          
                                                         
  
                                                                               
                                                                           
                                                                               
                                                                         
                                                                        
                                                                          
                                                                         
                                                                     
                                                                        
                                                                         
                       
                                                                     
                                                                        
  
                                                                
                                                                    
                                                                       
                                                                      
                                                                    
                                                                    
                                                          
  
                                                      
                                                                         
                                                   
                                                                     
                                                                         
                                                                      
                                                                         
                                                    
                                                               
                                                                             
                                                                  
                                                                                
                                                                              
                                              
                                                                       
                                                                          
                                                                     
                                              
                                                               
                                                                 
  
                       
                   
                                      
                                                         
                                                                                
                                                                           
                                              
                                      
                                                                      
                                                                   
                                                    
                                                                     
                                                                   
                                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                            
                                                                          
                                                                            
                                                                         
                                      
   
psa_status_t psa_aead_update(psa_aead_operation_t *operation,
                             const uint8_t *input,
                             size_t input_length,
                             uint8_t *output,
                             size_t output_size,
                             size_t *output_length);

                                                     
  
                                                                     
  
                                                                   
                                                                  
                                                                      
                                                         
  
                                        
                                                                      
                                          
                                            
  
                                                                           
                                                                          
                                                         
  
                                                      
                                                                            
                                                 
                                                                          
                                                                         
                                                  
                                                             
                                                                           
                                                                              
                                                                               
                                                       
                                                                               
                                                                               
                                            
                                                                   
                                                    
                                                                      
                                              
                                                                   
                                                                         
                                                  
                                                                               
                                                                      
                                                                              
                                                                        
                                                                         
                                                           
                                                                         
                                                                        
                                            
                                                               
                                                              
  
                       
                   
                                      
                                                                       
                                                               
                                                                        
                                                                                
                                                                        
                                                     
                                      
                                                                      
                                                                   
                                                    
                                                                   
                                                             
                                                 
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                            
                                                                              
                                            
                                                                         
                                      
   
psa_status_t psa_aead_finish(psa_aead_operation_t *operation,
                             uint8_t *ciphertext,
                             size_t ciphertext_size,
                             size_t *ciphertext_length,
                             uint8_t *tag,
                             size_t tag_size,
                             size_t *tag_length);

                                                                        
  
                                                                     
  
                                                                     
              
  
                                                                       
                                                        
                                                                            
                                           
                                           
  
                                                                            
                                                                           
                                                      
  
                                                                           
                                                                          
                                                         
  
                                                                      
                                                                      
                    
  
                                                      
                                                                           
                                                                            
                                                                        
                                                                         
                                             
                                                                         
                                                                                            
                                                             
                                                                           
                                                                              
                                                                               
                                                       
                                                                               
                                                                               
                                            
                                                                  
                                                   
                                                                         
                                                                   
  
                       
                   
                                       
                                                                          
                       
                                      
                                                            
                                                               
                                                                        
                                
                                      
                                                                      
                                                                   
                                                    
                                                                   
                                                             
                                                 
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                            
                                                                              
                                            
                                                                         
                                      
   
psa_status_t psa_aead_verify(psa_aead_operation_t *operation,
                             uint8_t *plaintext,
                             size_t plaintext_size,
                             size_t *plaintext_length,
                             const uint8_t *tag,
                             size_t tag_length);

                            
  
                                                                      
                                                                    
                                                 
                                                              
  
                                                                     
                                                          
  
                                                                       
                                                                 
                                               
  
                                                           
  
                                         
                                                             
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_aead_abort(psa_aead_operation_t *operation);

       

                                                
     
   

   
                                                                          
                                         
  
                                                                             
                                                                        
                                                                               
                               
  
                                                                               
                                                                               
                                                                            
                                                                              
                                                                                
                                                                             
                                       
                                                          
                                                                     
                                                                            
                                                                              
                                                                    
                                                  
                                                                
                                                                                         
                                                                                    
                                                               
                                                                          
                                                                         
                                                      
                                                                            
                                                             
  
                                         
                                                      
                                   
                                                                      
                                                         
                                      
                                                                    
                                                        
                                                                  
                                                                      
                                  
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                    
                                                    
                                                            
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_sign_message(mbedtls_svc_key_id_t key,
                              psa_algorithm_t alg,
                              const uint8_t *input,
                              size_t input_length,
                              uint8_t *signature,
                              size_t signature_size,
                              size_t *signature_length);

                                                                     
                                                  
  
                                                                     
                                                                             
                                                               
                                                                             
                
  
                                                                               
                                                                            
                                                                  
                                                              
                                                                              
                                                                                
                                                                             
                                       
                                                                              
                                                                     
                                                                          
                                                                         
  
                                         
                                                      
                                   
                                                                      
                                                         
                                       
                                                                               
                                    
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                    
                                                    
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_verify_message(mbedtls_svc_key_id_t key,
                                psa_algorithm_t alg,
                                const uint8_t *input,
                                size_t input_length,
                                const uint8_t *signature,
                                size_t signature_length);

   
                                                          
  
                                                                     
                                                                          
                                                                         
                                              
                                                                         
                                          
  
                                                                               
                                                                               
                                                                         
                                                                  
                                                                             
                                                                 
                                                   
                                                            
                                                                    
                                                                            
                                                                         
                                                               
                                                                          
  
                                         
                                                      
                                                     
                                      
                                                                    
                                                        
                                                                  
                                                                      
                                  
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                            
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_sign_hash(mbedtls_svc_key_id_t key,
                           psa_algorithm_t alg,
                           const uint8_t *hash,
                           size_t hash_length,
                           uint8_t *signature,
                           size_t signature_size,
                           size_t *signature_length);

   
                                                                             
  
                                                                     
                                                                          
                                                                         
                                              
                                                                         
                                          
  
                                                                              
                                                                               
                                                    
                                                       
                                                              
                                                                         
                                                             
                                               
                                                                        
                                     
                                                                
                                                                      
                                                                     
  
                       
                                  
                                                      
                                                     
                                       
                                                                     
                                              
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_verify_hash(mbedtls_svc_key_id_t key,
                             psa_algorithm_t alg,
                             const uint8_t *hash,
                             size_t hash_length,
                             const uint8_t *signature,
                             size_t signature_length);

   
                                                    
  
                                                                               
                                                                            
                                                             
                                                       
                                                                          
                                                                   
                                                       
                                                                     
                                                                    
                                                     
                                                                   
                                                   
                                                                     
                                                                        
                                             
  
                                                                             
                                            
                                                                    
                                                              
                                                                        
                                           
                                                                      
                                                               
                                                                 
  
                                         
                                                      
                                                     
                                      
                                                                 
                                                        
                                                                                
                                                                      
                                  
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                            
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_asymmetric_encrypt(mbedtls_svc_key_id_t key,
                                    psa_algorithm_t alg,
                                    const uint8_t *input,
                                    size_t input_length,
                                    const uint8_t *salt,
                                    size_t salt_length,
                                    uint8_t *output,
                                    size_t output_size,
                                    size_t *output_length);

   
                                                     
  
                                                                          
                                                                              
                                                                         
                                  
                                                                            
                                                                               
                                                                               
                                                                         
                                                              
  
                                                                               
                                                                          
                                                                       
                                                                          
                                                                   
                                                       
                                                                     
                                                                    
                                                     
                                                                   
                                                   
                                                                     
                                                                        
                                             
  
                                                                             
                                            
                                                                    
                                                              
                                                                        
                                           
                                                                      
                                                               
                                                                 
  
                                         
                                                      
                                                     
                                      
                                                                 
                                                        
                                                                                
                                                                      
                                  
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                            
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_asymmetric_decrypt(mbedtls_svc_key_id_t key,
                                    psa_algorithm_t alg,
                                    const uint8_t *input,
                                    size_t input_length,
                                    const uint8_t *salt,
                                    size_t salt_length,
                                    uint8_t *output,
                                    size_t output_size,
                                    size_t *output_length);

       

                                                                       
     
   

                                                                       
  
                                                                        
                                                                
                                                     
          
                                              
                                              
             
                                                                  
          
                                                    
             
                                                                                    
                 
          
                                                                                  
             
                                                                          
                                   
          
                                              
                                                     
             
  
                                                                       
                                                            
                                                                       
   
typedef struct psa_key_derivation_s psa_key_derivation_operation_t;

                                          
  
                                                                           
                                                  
   

                                                                  
   
static psa_key_derivation_operation_t psa_key_derivation_operation_init(void);

                                      
  
                                                                         
                                        
                                                         
                          
  
                   
                                                                               
                                                              
                                                          
                                                                        
                                                                         
                                                                       
                                                          
                                                                              
                                                                               
                                                         
                                                              
                                             
                                                           
                                        
                                                                     
                                                 
                                                       
                                 
  
                                                                            
               
  
                                                                             
                                                                               
  
                                                                    
  
                                                                     
                                                    
                                                                           
                                                                         
                                                                 
                                                                              
  
                       
                   
                                      
                                                    
                                   
                                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                     
                                                                                
                                                                         
                                      
   
psa_status_t psa_key_derivation_setup(
    psa_key_derivation_operation_t *operation,
    psa_algorithm_t alg);

                                                                
  
                                                                              
                                                                            
                                    
  
                                                   
                                                                      
  
                                         
                                                             
                                                        
                                                           
                               
                                                                   
                                                                                
                                                                         
                                      
   
psa_status_t psa_key_derivation_get_capacity(
    const psa_key_derivation_operation_t *operation,
    size_t *capacity);

                                                           
  
                                                                            
                                                                        
  
                                                                          
                                                              
                                                                       
                                             
  
                                         
                                      
                                                                       
                                                                            
                             
                                                             
                                                        
                                                           
                               
                                                                       
                                                                            
                                                                         
                                      
   
psa_status_t psa_key_derivation_set_capacity(
    psa_key_derivation_operation_t *operation,
    size_t capacity);

                                                                     
  
                                                                           
                                                                            
                                                                           
             
   
#define PSA_KEY_DERIVATION_UNLIMITED_CAPACITY ((size_t) (-1))

                                                         
  
                                                                        
                                                                     
                             
  
                                                                   
                                                                      
                                                                        
                                                      
                                                                              
                        
  
                                                                          
                                                                   
  
                                                                             
                                                               
                                                                         
                                                               
                                                                   
                                                    
                                                                      
  
                       
                   
                                      
                                                                       
                                                
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                      
                                                                                
                                                                         
                                      
   
psa_status_t psa_key_derivation_input_bytes(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    const uint8_t *data,
    size_t data_length);

                                                                
  
                                                                        
                                                                         
                                                                  
                                                                     
                             
  
                                                                     
            
  
                                                                          
                                                                   
  
                                                                             
                                                               
                                                                         
                                                               
                                                                   
                                                                 
  
                       
                   
                                      
                                                                       
                                                 
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                      
                                                                                
                                                                         
                                      
   
psa_status_t psa_key_derivation_input_integer(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    uint64_t value);

                                                             
  
                                                                        
                                                                     
                             
  
                                                                              
                                                                          
                                                                       
                                                                  
                                                      
                                                                              
                        
  
                                                                          
                                                                   
  
                                                                             
                                                               
                                                                         
                                                               
                                                                   
                                                                        
                                                                              
                                                                
                                                                             
                                                                          
  
                                                                        
                                                                              
                                              
                                                                              
                          
                                                                           
                                                                            
                                                                   
                                                                              
                                                         
                                                                 
                                       
  
                       
                   
                                                      
                                   
                                                           
                                                                     
                     
                                      
                                                                       
                                                              
                                               
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                      
                                                                                
                                                                         
                                      
   
psa_status_t psa_key_derivation_input_key(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    mbedtls_svc_key_id_t key);

                                                                       
              
  
                                                                           
                            
                                                                      
                                                                         
                                                                        
  
                                                                          
                                                                   
  
                                                                             
                                                               
                                                                   
                                                                        
                                                                        
                                                                           
                                                                           
                                            
                                                                    
                                                                     
                                                                   
                                                                               
                                                                        
                                                                               
                                                                             
                                                                        
                                                                            
                                         
                                                
                                                                
                                                                         
                                                                            
                                                                              
                                                                           
                                                                              
                                                                       
                                                    
                                                               
  
                       
                   
                                                      
                                                     
                                      
                                                        
                                                                        
                                                                       
                                
                                   
                                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                           
                                                                                   
                                                                         
                                      
   
psa_status_t psa_key_derivation_key_agreement(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    mbedtls_svc_key_id_t private_key,
    const uint8_t *peer_key,
    size_t peer_key_length);

                                                   
  
                                                                            
                      
                                                                     
                                                                      
          
                                                                  
  
                                                      
                                                              
                                                                   
  
                                                                             
                                                                    
                                                      
  
                                         
                                   
                                                                
                                 
                                       
                                                                  
                                                                           
                                                                      
                                                                      
                                                                      
                                                                       
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                            
                                                                            
                                            
                                                                         
                                      
   
psa_status_t psa_key_derivation_output_bytes(
    psa_key_derivation_operation_t *operation,
    uint8_t *output,
    size_t output_length);

/** Derive a key from an ongoing key derivation operation.
 *
 * This function calculates output bytes from a key derivation algorithm
 * and uses those bytes to generate a key deterministically.
 * The key's location, usage policy, type and size are taken from
 * \p attributes.
 *
 * If you view the key derivation's output as a stream of bytes, this
 * function destructively reads as many bytes as required from the
 * stream.
 * The operation's capacity decreases by the number of bytes read.
 *
 * If this function returns an error status other than
 * #PSA_ERROR_INSUFFICIENT_DATA, the operation enters an error
 * state and must be aborted by calling psa_key_derivation_abort().
 *
 * How much output is produced and consumed from the operation, and how
 * the key is derived, depends on the key type and on the key size
 * (denoted \c bits below):
 *
 * - For key types for which the key is an arbitrary sequence of bytes
 *   of a given size, this function is functionally equivalent to
 *   calling #psa_key_derivation_output_bytes
 *   and passing the resulting output to #psa_import_key.
 *   However, this function has a security benefit:
 *   if the implementation provides an isolation boundary then
 *   the key material is not exposed outside the isolation boundary.
 *   As a consequence, for these key types, this function always consumes
 *   exactly (\c bits / 8) bytes from the operation.
 *   The following key types defined in this specification follow this scheme:
 *
 *     - #PSA_KEY_TYPE_AES;
 *     - #PSA_KEY_TYPE_ARIA;
 *     - #PSA_KEY_TYPE_CAMELLIA;
 *     - #PSA_KEY_TYPE_DERIVE;
 *     - #PSA_KEY_TYPE_HMAC;
 *     - #PSA_KEY_TYPE_PASSWORD_HASH.
 *
 * - For ECC keys on a Montgomery elliptic curve
 *   (#PSA_KEY_TYPE_ECC_KEY_PAIR(\c curve) where \c curve designates a
 *   Montgomery curve), this function always draws a byte string whose
 *   length is determined by the curve, and sets the mandatory bits
 *   accordingly. That is:
 *
 *     - Curve25519 (#PSA_ECC_FAMILY_MONTGOMERY, 255 bits): draw a 32-byte
 *       string and process it as specified in RFC 7748 &sect;5.
 *     - Curve448 (#PSA_ECC_FAMILY_MONTGOMERY, 448 bits): draw a 56-byte
 *       string and process it as specified in RFC 7748 &sect;5.
 *
 * - For key types for which the key is represented by a single sequence of
 *   \c bits bits with constraints as to which bit sequences are acceptable,
 *   this function draws a byte string of length (\c bits / 8) bytes rounded
 *   up to the nearest whole number of bytes. If the resulting byte string
 *   is acceptable, it becomes the key, otherwise the drawn bytes are discarded.
 *   This process is repeated until an acceptable byte string is drawn.
 *   The byte string drawn from the operation is interpreted as specified
 *   for the output produced by psa_export_key().
 *   The following key types defined in this specification follow this scheme:
 *
 *     - #PSA_KEY_TYPE_DES.
 *       Force-set the parity bits, but discard forbidden weak keys.
 *       For 2-key and 3-key triple-DES, the three keys are generated
 *       successively (for example, for 3-key triple-DES,
 *       if the first 8 bytes specify a weak key and the next 8 bytes do not,
 *       discard the first 8 bytes, use the next 8 bytes as the first key,
 *       and continue reading output from the operation to derive the other
 *       two keys).
 *     - Finite-field Diffie-Hellman keys (#PSA_KEY_TYPE_DH_KEY_PAIR(\c group)
 *       where \c group designates any Diffie-Hellman group) and
 *       ECC keys on a Weierstrass elliptic curve
 *       (#PSA_KEY_TYPE_ECC_KEY_PAIR(\c curve) where \c curve designates a
 *       Weierstrass curve).
 *       For these key types, interpret the byte string as integer
 *       in big-endian order. Discard it if it is not in the range
 *       [0, *N* - 2] where *N* is the boundary of the private key domain
 *       (the prime *p* for Diffie-Hellman, the subprime *q* for DSA,
 *       or the order of the curve's base point for ECC).
 *       Add 1 to the resulting integer and use this as the private key *x*.
 *       This method allows compliance to NIST standards, specifically
 *       the methods titled "key-pair generation by testing candidates"
 *       in NIST SP 800-56A &sect;5.6.1.1.4 for Diffie-Hellman,
 *       in FIPS 186-4 &sect;B.1.2 for DSA, and
 *       in NIST SP 800-56A &sect;5.6.1.2.2 or
 *       FIPS 186-4 &sect;B.4.2 for elliptic curve keys.
 *
 * - For other key types, including #PSA_KEY_TYPE_RSA_KEY_PAIR,
 *   the way in which the operation output is consumed is
 *   implementation-defined.
 *
 * In all cases, the data that is read is discarded from the operation.
 * The operation's capacity is decreased by the number of bytes read.
 *
 * For algorithms that take an input step #PSA_KEY_DERIVATION_INPUT_SECRET,
 * the input to that step must be provided with psa_key_derivation_input_key().
 * Future versions of this specification may include additional restrictions
 * on the derived key based on the attributes and strength of the secret key.
 *
 * \note This function is equivalent to calling
 *       psa_key_derivation_output_key_custom()
 *       with the custom production parameters #PSA_CUSTOM_KEY_PARAMETERS_INIT
 *       and `custom_data_length == 0` (i.e. `custom_data` is empty).
 *
 * \param[in] attributes    The attributes for the new key.
 *                          If the key type to be created is
 *                          #PSA_KEY_TYPE_PASSWORD_HASH then the algorithm in
 *                          the policy must be the same as in the current
 *                          operation.
 * \param[in,out] operation The key derivation operation object to read from.
 * \param[out] key          On success, an identifier for the newly created
 *                          key. For persistent keys, this is the key
 *                          identifier defined in \p attributes.
 *                          \c 0 on failure.
 *
 * \retval #PSA_SUCCESS
 *         Success.
 *         If the key is persistent, the key material and the key's metadata
 *         have been saved to persistent storage.
 * \retval #PSA_ERROR_ALREADY_EXISTS
 *         This is an attempt to create a persistent key, and there is
 *         already a persistent key with the given identifier.
 * \retval #PSA_ERROR_INSUFFICIENT_DATA
 *         There was not enough data to create the desired key.
 *         Note that in this case, no output is written to the output buffer.
 *         The operation's capacity is set to 0, thus subsequent calls to
 *         this function will not succeed, even with a smaller output buffer.
 * \retval #PSA_ERROR_NOT_SUPPORTED
 *         The key type or key size is not supported, either by the
 *         implementation in general or in this particular location.
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 *         The provided key attributes are not valid for the operation.
 * \retval #PSA_ERROR_NOT_PERMITTED
 *         The #PSA_KEY_DERIVATION_INPUT_SECRET or
 *         #PSA_KEY_DERIVATION_INPUT_PASSWORD input was not provided through a
 *         key; or one of the inputs was a key whose policy didn't allow
 *         #PSA_KEY_USAGE_DERIVE.
 * \retval #PSA_ERROR_INSUFFICIENT_MEMORY \emptydescription
 * \retval #PSA_ERROR_INSUFFICIENT_STORAGE \emptydescription
 * \retval #PSA_ERROR_COMMUNICATION_FAILURE \emptydescription
 * \retval #PSA_ERROR_HARDWARE_FAILURE \emptydescription
 * \retval #PSA_ERROR_CORRUPTION_DETECTED \emptydescription
 * \retval #PSA_ERROR_DATA_INVALID \emptydescription
 * \retval #PSA_ERROR_DATA_CORRUPT \emptydescription
 * \retval #PSA_ERROR_STORAGE_FAILURE \emptydescription
 * \retval #PSA_ERROR_BAD_STATE
 *         The operation state is not valid (it must be active and completed
 *         all required input steps), or the library has not been previously
 *         initialized by psa_crypto_init().
 *         It is implementation-dependent whether a failure to initialize
 *         results in this error code.
 */
psa_status_t psa_key_derivation_output_key(
    const psa_key_attributes_t *attributes,
    psa_key_derivation_operation_t *operation,
    mbedtls_svc_key_id_t *key);

                                                                     
                          
  
                                                                           
                                                        
                                                                           
              
  
                                                                     
                              
  
                                                           
                                                            
                                                                             
                                                                         
                                      
                                                                             
                                                                            
                                                                        
                                                           
                                                          
                                                            
                                                                           
                            
                                                             
                                                                           
                                                                     
                                                                
                                            
  
                       
                   
                                                                            
                                                 
                                    
                                                                      
                                                              
                                       
                                                               
                                                                             
                                                                         
                                                                             
                                   
                                                                   
                                                                    
                                      
                                                                       
                                   
                                                  
                                                                              
                                                                        
                                 
                                                           
                                                            
                                                             
                                                        
                                                           
                                                    
                                                    
                                                       
                               
                                                                            
                                                                            
                                            
                                                                         
                                      
   
psa_status_t psa_key_derivation_output_key_custom(
    const psa_key_attributes_t *attributes,
    psa_key_derivation_operation_t *operation,
    const psa_custom_key_parameters_t *custom,
    const uint8_t *custom_data,
    size_t custom_data_length,
    mbedtls_svc_key_id_t *key);

#ifndef __cplusplus
                                                                    
                                                                    
                
                                                  
   
                                                                     
                          
  
        
                                                                          
                                                                   
                                                               
                                                                 
  
                                                           
                                                            
                                                                             
                                                                         
                                      
                                                                             
                                                                            
                                                                            
                                                           
                                                          
                                                            
                                                                        
                                                       
                                                                
                                              
                            
                                                              
                                                                           
                                                                     
                                                                
                                            
  
                       
                   
                                                                            
                                                 
                                    
                                                                      
                                                              
                                       
                                                               
                                                                             
                                                                         
                                                                             
                                   
                                                                   
                                                                    
                                      
                                                                       
                                   
                                                  
                                                                              
                                                                        
                                 
                                                           
                                                            
                                                             
                                                        
                                                           
                                                    
                                                    
                                                       
                               
                                                                            
                                                                            
                                            
                                                                         
                                      
   
psa_status_t psa_key_derivation_output_key_ext(
    const psa_key_attributes_t *attributes,
    psa_key_derivation_operation_t *operation,
    const psa_key_production_parameters_t *params,
    size_t params_data_length,
    mbedtls_svc_key_id_t *key);
#endif                   

                                                                             
  
                                                                            
                                                              
                                                                     
                                                                     
                                
                                                                  
  
                                                         
        
                                                                  
                                               
                                          
           
                                                                             
                                                               
  
                                                      
                                                                
                                                                     
                              
  
                                                                             
                                                                             
                                                                           
                                                              
  
                                         
                                       
                                                                             
                  
                                   
                                                                
                                            
                                       
                                                                  
                                                                           
                                                                      
                                                                      
                                                                         
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                            
                                                                            
                                            
                                                                         
                                      
   
psa_status_t psa_key_derivation_verify_bytes(
    psa_key_derivation_operation_t *operation,
    const uint8_t *expected,
    size_t expected_length);

                                                                            
                          
  
                                                                            
                                                                     
                               
                                                                     
                                                                        
                                                                      
                                                                  
  
                                                                   
                                                                  
                                            
  
                                                      
                                                                
                                                                     
                              
  
                                                                             
                                                                     
                                                                           
                                                                             
                                                                      
                                                                       
                                                          
                                                              
                                                                   
  
                                         
                                       
                                                                             
                  
                                    
                                                               
                                      
                                                                    
                                   
                                                                            
                                                                             
                                                  
                                       
                                                                  
                                                                           
                                                                      
                                                                      
                                                                         
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                            
                                                                            
                                            
                                                                         
                                      
   
psa_status_t psa_key_derivation_verify_key(
    psa_key_derivation_operation_t *operation,
    psa_key_id_t expected);

                                     
  
                                                                         
                                                                               
                                                                     
  
                                                              
                                                                               
  
                                                                             
                                                                            
  
                                                      
  
                                         
                                                             
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_key_derivation_abort(
    psa_key_derivation_operation_t *operation);

                                                             
  
                                                                            
                                                                        
                                                                       
                                                                     
                                                                     
                                               
  
                                                                        
                                                                 
                                                                       
                                           
                                                                               
                                                                        
                                                                    
                                                                          
                                                                          
                                                                          
                                                             
                                                               
                                                                          
                                             
                                                                        
                                                                 
                                                                   
  
                       
                   
                                                      
                                                     
                                      
                                                      
                                                        
                                                                        
                          
                                      
                                      
                                   
                                                             
                                                           
                                                             
                                                        
                                                           
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_raw_key_agreement(psa_algorithm_t alg,
                                   mbedtls_svc_key_id_t private_key,
                                   const uint8_t *peer_key,
                                   size_t peer_key_length,
                                   uint8_t *output,
                                   size_t output_size,
                                   size_t *output_length);

       

                                      
     
   

   
                                
  
                                                                            
                                                                           
                                       
  
                                                              
  
                                                                 
                                                                   
  
                                         
                                                     
                                                            
                                                           
                                                             
                                                        
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_generate_random(uint8_t *output,
                                 size_t output_size);

/**
 * \brief Generate a key or key pair.
 *
 * The key is generated randomly.
 * Its location, usage policy, type and size are taken from \p attributes.
 *
 * Implementations must reject an attempt to generate a key of size 0.
 *
 * The following type-specific considerations apply:
 * - For RSA keys (#PSA_KEY_TYPE_RSA_KEY_PAIR),
 *   the public exponent is 65537.
 *   The modulus is a product of two probabilistic primes
 *   between 2^{n-1} and 2^n where n is the bit size specified in the
 *   attributes.
 *
 * \note This function is equivalent to calling psa_generate_key_custom()
 *       with the custom production parameters #PSA_CUSTOM_KEY_PARAMETERS_INIT
 *       and `custom_data_length == 0` (i.e. `custom_data` is empty).
 *
 * \param[in] attributes    The attributes for the new key.
 * \param[out] key          On success, an identifier for the newly created
 *                          key. For persistent keys, this is the key
 *                          identifier defined in \p attributes.
 *                          \c 0 on failure.
 *
 * \retval #PSA_SUCCESS
 *         Success.
 *         If the key is persistent, the key material and the key's metadata
 *         have been saved to persistent storage.
 * \retval #PSA_ERROR_ALREADY_EXISTS
 *         This is an attempt to create a persistent key, and there is
 *         already a persistent key with the given identifier.
 * \retval #PSA_ERROR_NOT_SUPPORTED \emptydescription
 * \retval #PSA_ERROR_INVALID_ARGUMENT \emptydescription
 * \retval #PSA_ERROR_INSUFFICIENT_MEMORY \emptydescription
 * \retval #PSA_ERROR_INSUFFICIENT_ENTROPY \emptydescription
 * \retval #PSA_ERROR_COMMUNICATION_FAILURE \emptydescription
 * \retval #PSA_ERROR_HARDWARE_FAILURE \emptydescription
 * \retval #PSA_ERROR_CORRUPTION_DETECTED \emptydescription
 * \retval #PSA_ERROR_INSUFFICIENT_STORAGE \emptydescription
 * \retval #PSA_ERROR_DATA_INVALID \emptydescription
 * \retval #PSA_ERROR_DATA_CORRUPT \emptydescription
 * \retval #PSA_ERROR_STORAGE_FAILURE \emptydescription
 * \retval #PSA_ERROR_BAD_STATE
 *         The library has not been previously initialized by psa_crypto_init().
 *         It is implementation-dependent whether a failure to initialize
 *         results in this error code.
 */
psa_status_t psa_generate_key(const psa_key_attributes_t *attributes,
                              mbedtls_svc_key_id_t *key);

   
                                                                        
  
                                                                      
                                                                              
                                                                             
                                                         
  
                                                        
  
                                                                     
                              
  
                                                           
                                                                            
                                                                        
                                                           
                                                          
                                               
                                                                           
                            
                                                             
                                                                           
                                                                     
                                                                
                                            
  
                       
                   
                                                                            
                                                 
                                    
                                                                      
                                                              
                                                     
                                                        
                                                           
                                                            
                                                             
                                                        
                                                           
                                                            
                                                    
                                                    
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_generate_key_custom(const psa_key_attributes_t *attributes,
                                     const psa_custom_key_parameters_t *custom,
                                     const uint8_t *custom_data,
                                     size_t custom_data_length,
                                     mbedtls_svc_key_id_t *key);

#ifndef __cplusplus
                                                                    
                                                                    
                
                                                  
   
   
                                                                        
  
        
                                                                          
                                                                   
                                                               
                                                                 
  
                                                           
                                                                            
                                                                            
                                                           
                                                          
                                               
                            
                                                              
                                                                           
                                                                     
                                                                
                                            
  
                       
                   
                                                                            
                                                 
                                    
                                                                      
                                                              
                                                     
                                                        
                                                           
                                                            
                                                             
                                                        
                                                           
                                                            
                                                    
                                                    
                                                       
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_generate_key_ext(const psa_key_attributes_t *attributes,
                                  const psa_key_production_parameters_t *params,
                                  size_t params_data_length,
                                  mbedtls_svc_key_id_t *key);
#endif                   

       

                                                               
     
   

                                                               
                       
  
                                                                   
                                                                
                                                     
          
                                                       
                                              
             
                                                                  
          
                                                             
             
                                                
                                                              
          
                                                        
                                                
             
                                      
                                                                       
             
          
                                                       
                                                              
             
  
                                                                       
                                                            
                                                                          
typedef struct psa_sign_hash_interruptible_operation_s psa_sign_hash_interruptible_operation_t;

                                                               
                            
  
                                                                   
                                                                
                                                     
          
                                                         
                                              
             
                                                                  
          
                                                               
             
                                                
                                                                
          
                                                          
                                                  
             
                                      
                                                                         
             
          
                                                         
                                                                
             
  
                                                                       
                                                            
                                                                          
typedef struct psa_verify_hash_interruptible_operation_s psa_verify_hash_interruptible_operation_t;

   
                                                                           
                                                                          
                                            
  
                                                                              
                                                                          
                                                             
  
                                                                        
                                                                      
                                                                               
                                                                             
                                                                            
                                                                             
                                                                               
                                                                            
                                                                           
                                                                             
                                                                        
  
                                                      
                                                                           
                                                                             
                                                                             
  
                                                                    
                                                                              
                                                                       
                                                                    
                                                                            
                                                                               
                                                                         
                                                                             
  
                                                                        
                                                                           
                                                                               
                                                                         
                                                                               
                                                                            
                                                                              
                                                                              
                                                                     
                                                                             
                                                                               
                                                                              
                                                                         
                                                                      
  
                                                                             
                                                                   
                                                                               
                                                                             
                                      
  
                                                                               
                                                                              
                                                                            
                                                                            
                                                                            
                                                                               
  
                                                                    
                                                                      
                                                          
  
                                                                             
                                                                           
                                                                             
                                                                          
   
void psa_interruptible_set_max_ops(uint32_t max_ops);

   
                                                                           
                                                                          
                                                                      
                                            
                                                                     
                                                                       
                                                                    
  
                                                                              
                                                                          
                                                             
  
                                                                   
                                                                          
                                            
   
uint32_t psa_interruptible_get_max_ops(void);

   
                                                                         
                                                                            
                                                                           
                                                                     
                                                                          
                                                                         
                                                                             
  
                                                                           
                                                                    
  
                                                                              
                                                                          
                                                             
  
                                                                              
                                                  
                                                                
  
                                                                              
                                                                       
  
                                                                             
                                    
   
uint32_t psa_sign_hash_get_num_ops(
    const psa_sign_hash_interruptible_operation_t *operation);

   
                                                                              
                                                                            
                                                                           
                                                                     
                                                                             
                                                                            
                                                                         
  
                                                                              
                                                                          
                                                             
  
                                                                              
                                                  
                                                                
  
                                      
                                                                            
                                                                    
  
                                                                             
                                    
   
uint32_t psa_verify_hash_get_num_ops(
    const psa_verify_hash_interruptible_operation_t *operation);

   
                                                                            
                                                                        
  
                                                           
  
                                                                              
                                                                          
                                                             
  
                                                              
                                                                         
                                                      
                                                                                
                                                                         
                                                                             
                                                                      
  
                                                                             
                                                                      
                                                                         
                                                                               
                                                                              
                                                                               
                                                                   
                                                                                
                                                       
  
                                                                             
                                                                           
                                                                            
  
                                                                              
                                                                       
  
                                                                               
                                                                               
                                                                         
                                                                     
                                                                             
                                                                 
                                                   
                                                            
                                                                    
  
                       
                                                                                
                                                          
  
                                                      
                                   
                                                                              
                                              
                               
                                                                           
                             
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                    
                                                    
                                                            
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_sign_hash_start(
    psa_sign_hash_interruptible_operation_t *operation,
    mbedtls_svc_key_id_t key, psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length);

   
                                                                              
                                                                              
                                                                
  
                                                        
  
                                                                              
                                                                          
                                                             
  
                                                              
                                                                      
                                                                               
                                                                                
                                                                                
                                                                      
  
                                                                           
                                                                           
                                                                                
                                                                               
                                                                   
                                                                                
                                                       
  
                                                                            
                                                                            
                                                                                
                                                                          
                                                         
  
                                                                              
                                                                                
                                                                           
                                      
  
                                                                            
                                                                              
                                                                    
                                                  
                                                                
                                                                       
                                                                            
                                                                    
                                                      
                                                                          
                                                                         
                                                      
                                                                            
                                                             
  
                       
                                           
  
                                    
                                                             
                                                                           
                                                                   
  
                                      
                                                                    
                                                        
                                                                  
                                                                      
                                  
  
                               
                                                                      
                                    
  
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                    
                                                    
                                                            
                               
                                                                    
                                                           
                                                                  
                                                                              
                           
   
psa_status_t psa_sign_hash_complete(
    psa_sign_hash_interruptible_operation_t *operation,
    uint8_t *signature, size_t signature_size,
    size_t *signature_length);

   
                                                            
  
                                                                              
                                                                          
                                                             
  
                                                                              
                                                                          
                                                                                
                                                                          
                                               
  
                                                                          
                                                                               
                                                                              
                                                                             
                                                            
  
                                                                             
                                                                         
                                                                            
                                                                               
                                                                        
                                                                 
  
                                                                
  
                       
                                                  
  
                                                     
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_sign_hash_abort(
    psa_sign_hash_interruptible_operation_t *operation);

   
                                                                           
                                                                    
  
                                                             
  
                                                                              
                                                                          
                                                             
  
                                                              
                                                                           
                                                           
                                                                               
                                                                         
                                                                             
                                                                 
  
                                                                               
                                                                               
                                                                         
                                                                              
                                                                            
                                                                            
                                                                             
                                                                               
                                                          
  
                                                                             
                                                                           
                                                                              
  
                                                                                
                                                                       
  
                                                                               
                                                            
                                                           
                                                                     
                                                                             
                                                                 
                                                   
                                                                           
                                                                    
                                                                          
                                                                         
  
                       
                                                              
                                                                           
                     
  
                               
                                                                             
                             
  
                                   
                                                                                
                                              
  
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                   
                                                   
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_verify_hash_start(
    psa_verify_hash_interruptible_operation_t *operation,
    mbedtls_svc_key_id_t key, psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length);

   
                                                                              
                                                                             
                                                                              
                                       
  
                                                          
  
                                                                              
                                                                          
                                                             
  
                                                              
                                                                        
                                                                          
                                                                              
                                                                              
                                                                           
                                     
  
                                                                           
                                                                           
                                                                                
                                                                               
                                                                   
                                                                               
                                                          
  
                                                                            
                                                                            
                                                                                
                                                                          
                                                           
  
                                                                                
                                                                                
                                                                             
                                      
  
                       
                                                                               
  
                                    
                                                             
                                                                           
                                                                   
  
                                                      
                                       
                                                                     
                                              
                               
                                                                      
                                      
                                                     
                                                        
                                                           
                                                             
                                                        
                                                           
                                                       
                                                    
                                                    
                                                            
                               
                                                                    
                                                           
                                                        
                                                                              
                           
   
psa_status_t psa_verify_hash_complete(
    psa_verify_hash_interruptible_operation_t *operation);

   
                                                            
  
                                                                               
                                                                               
                                                 
  
                                                                                
                                                                               
                                                                   
                                                                          
                                             
  
                                                                        
                                                                          
                                                                               
                                                                        
                                                            
  
                                                                           
                                                                    
                                                                               
                                                                                
                                                                     
                                                                 
  
                                                                
  
                       
                                                  
  
                                                     
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_verify_hash_abort(
    psa_verify_hash_interruptible_operation_t *operation);


       

#ifdef __cplusplus
}
#endif

                                                                       
                                                                  
#include "crypto_extra.h"

#endif                   
