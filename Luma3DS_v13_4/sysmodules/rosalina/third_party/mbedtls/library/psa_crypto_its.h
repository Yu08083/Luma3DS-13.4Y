                          
                                                               
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_ITS_H
#define PSA_CRYPTO_ITS_H

#include <stddef.h>
#include <stdint.h>

#include <psa/crypto_types.h>
#include <psa/crypto_values.h>

#ifdef __cplusplus
extern "C" {
#endif

                                                
   
typedef uint32_t psa_storage_create_flags_t;

                                                    
   
typedef uint64_t psa_storage_uid_t;

#define PSA_STORAGE_FLAG_NONE        0                                 
#define PSA_STORAGE_FLAG_WRITE_ONCE (1 << 0)                                                                                                                                                    

   
                                                                 
   
struct psa_storage_info_t {
    uint32_t size;                                                                     
    psa_storage_create_flags_t flags;                                                   
};

                                                                                                   
#define PSA_STORAGE_SUPPORT_SET_EXTENDED (1 << 0)

#define PSA_ITS_API_VERSION_MAJOR  1                                                                                                                                      
#define PSA_ITS_API_VERSION_MINOR  1                                                                                                                                            

   
                                                           
  
                                                       
                                                                     
                                                        
                                                                       
  
                                                                        
  
                                                                                     
                                                                                                                                                           
                                                                                                                                                                  
                                                                                                                                
                                                                                                                           
                                                                                                                    
                                                                                                                                
   
psa_status_t psa_its_set(psa_storage_uid_t uid,
                         uint32_t data_length,
                         const void *p_data,
                         psa_storage_create_flags_t create_flags);

   
                                                           
  
                                             
                                                                         
                                                                                                                    
                                                                                                   
                                                                                
  
  
                                                                        
  
                                                                                 
                                                                                                                               
                                                                                                                       
                                                                                                        
                                                                                                                                 
                                                                                                                             
                                                                                                                 
   
psa_status_t psa_its_get(psa_storage_uid_t uid,
                         uint32_t data_offset,
                         uint32_t data_length,
                         void *p_data,
                         size_t *p_data_length);

   
                                                      
  
                                         
                                                                                                                 
  
                                                                        
  
                                                                                 
                                                                                                                             
                                                                                                        
                                                                                                                
                                                                                                                            
   
psa_status_t psa_its_get_info(psa_storage_uid_t uid,
                              struct psa_storage_info_t *p_info);

   
                                                                          
  
                                 
  
                                                                    
  
                                                                                  
                                                                                                                              
                                                                                                                                              
                                                                                                                        
   
psa_status_t psa_its_remove(psa_storage_uid_t uid);

#ifdef __cplusplus
}
#endif

#endif                       
