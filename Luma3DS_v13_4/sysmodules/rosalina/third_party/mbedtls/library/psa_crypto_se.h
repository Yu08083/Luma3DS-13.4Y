  
                                                 
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_SE_H
#define PSA_CRYPTO_SE_H

  
                                                                           
                                                                            
                                                                           
                                                                  
                 
   
#include "psa/build_info.h"

#include "psa/crypto.h"
#include "psa/crypto_se_driver.h"

                                                                
                        
  
                                                                       
                                                                          
                                                      
  
                                                                    
                                                  
   
#define PSA_MAX_SE_LOCATION 255

                                                                    
                          
  
                                                                              
                                                                          
                                                                         
                                                                            
                               
   
#define PSA_CRYPTO_SE_DRIVER_ITS_UID_BASE ((psa_key_id_t) 0xfffffe00)

                                                                        
#define PSA_MAX_SE_DRIVERS 4

                                          
  
                                                                             
                                                                         
                                         
   
void psa_unregister_all_se_drivers(void);

                                          
  
                                 
   
psa_status_t psa_init_all_se_drivers(void);

                                                                  
  
                                                                
                                                                 
   
typedef struct psa_se_drv_table_entry_s psa_se_drv_table_entry_t;

                                                                      
  
                                                            
                                                                
                                                                       
                                                                 
                                                                
                                                                     
                                          
                                                                     
  
            
                                                          
            
                                                                  
   
int psa_get_se_driver(psa_key_lifetime_t lifetime,
                      const psa_drv_se_t **p_methods,
                      psa_drv_se_context_t **p_drv_context);

                                                                      
  
                                                    
  
                                                     
                                                                             
   
psa_se_drv_table_entry_t *psa_get_se_driver_entry(
    psa_key_lifetime_t lifetime);

                                                        
  
                                                                     
  
                                     
                                           
   
const psa_drv_se_t *psa_get_se_driver_methods(
    const psa_se_drv_table_entry_t *driver);

                                                  
  
                                                                     
  
                                           
                                           
   
psa_drv_se_context_t *psa_get_se_driver_context(
    psa_se_drv_table_entry_t *driver);

                                                     
  
                                                                           
                                            
  
                                                                               
                                                            
                                                                          
                                           
   
psa_status_t psa_find_se_slot_for_key(
    const psa_key_attributes_t *attributes,
    psa_key_creation_method_t method,
    psa_se_drv_table_entry_t *driver,
    psa_key_slot_number_t *slot_number);

                                      
  
                                                                  
                                            
   
psa_status_t psa_destroy_se_key(psa_se_drv_table_entry_t *driver,
                                psa_key_slot_number_t slot_number);

                                                        
  
                                                                        
                                                  
  
                       
                                   
                                    
                                     
                                  
                                      
   
psa_status_t psa_load_se_persistent_data(
    const psa_se_drv_table_entry_t *driver);

                                                        
  
                                                                        
                                                
  
                       
                                   
                                   
                                   
                                          
                                     
                                      
   
psa_status_t psa_save_se_persistent_data(
    const psa_se_drv_table_entry_t *driver);

                                                           
  
                                           
  
                                                                    
                                                        
   
psa_status_t psa_destroy_se_persistent_data(psa_key_location_t location);


                                                                          
   
typedef struct {
    uint8_t slot_number[sizeof(psa_key_slot_number_t)];
} psa_se_key_data_storage_t;

#endif                      
