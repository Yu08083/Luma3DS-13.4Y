  
                                              
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_SLOT_MANAGEMENT_H
#define PSA_CRYPTO_SLOT_MANAGEMENT_H

#include "psa/crypto.h"
#include "psa_crypto_core.h"
#include "psa_crypto_se.h"

                                      
  
                                                                           
                                                                       
  
                                                                             
                                                                      
                                             
   

                                                    
   
#define PSA_KEY_ID_VOLATILE_MIN  PSA_KEY_ID_VENDOR_MIN

                                                    
   
#if defined(MBEDTLS_PSA_KEY_STORE_DYNAMIC)
#define PSA_KEY_ID_VOLATILE_MAX (MBEDTLS_PSA_KEY_ID_BUILTIN_MIN - 1)
#else                                    
#define PSA_KEY_ID_VOLATILE_MAX                                 \
    (PSA_KEY_ID_VOLATILE_MIN + MBEDTLS_PSA_KEY_SLOT_COUNT - 1)
#endif                                    

                                                               
  
                                         
  
            
                                                           
            
                                                               
   
static inline int psa_key_id_is_volatile(psa_key_id_t key_id)
{
    return (key_id >= PSA_KEY_ID_VOLATILE_MIN) &&
           (key_id <= PSA_KEY_ID_VOLATILE_MAX);
}

                                                                  
  
                                                                             
                                                                            
                                             
  
                                                                             
                                       
  
                                                                     
                                                                           
                                                            
  
                                                                             
                                                    
  
                                                
                                                                       
                                                                      
                                             
  
                       
                                                                       
                                                       
                                                     
                               
                                                
                                    
                                                
                                         
                                                                             
                                                                            
                                                                
                                    
                                                      
                                                           
                                                       
                                                    
   
psa_status_t psa_get_and_lock_key_slot(mbedtls_svc_key_id_t key,
                                       psa_key_slot_t **p_slot);

                                       
  
                       
                                                   
   
psa_status_t psa_initialize_key_slots(void);

#if defined(MBEDTLS_TEST_HOOKS) && defined(MBEDTLS_PSA_KEY_STORE_DYNAMIC)
                                                                          
                                                                          
              
  
                                                 
                                            
  
                                                           
                                
   
extern size_t (*mbedtls_test_hook_psa_volatile_key_slice_length)(
    size_t slice_idx);

                                        
size_t psa_key_slot_volatile_slice_count(void);
#endif

                                             
                                                                          
                                                                           
  
                                              
void psa_wipe_all_key_slots(void);

                                                                
  
                                               
                                                                
  
                                                        
                                                                       
                                                      
  
                                                          
                         
  
                                                                    
                                                                
                                                                   
                                                              
                                                             
                                                                  
                                                                    
                                                                            
                                                     
                                                                   
  
                                         
                                         
                                        
                                                                        
                                                
                                                 
                                         
                                                                           
                            
   
psa_status_t psa_reserve_free_key_slot(psa_key_id_t *volatile_key_id,
                                       psa_key_slot_t **p_slot);

#if defined(MBEDTLS_PSA_KEY_STORE_DYNAMIC)
                                       
  
                                                                           
                       
  
                                                          
                         
  
                                                              
                                                                         
                                                                             
                                             
  
                                         
                                         
                                                                           
                            
   
psa_status_t psa_free_key_slot(size_t slice_idx,
                               psa_key_slot_t *slot);
#endif                                    

                                   
  
                                                                         
                                                                           
             
  
                                                          
                         
  
                                           
                                                            
                                                        
  
                       
                                                          
                                         
                                                       
   
static inline psa_status_t psa_key_slot_state_transition(
    psa_key_slot_t *slot, psa_key_slot_state_t expected_state,
    psa_key_slot_state_t new_state)
{
    if (slot->state != expected_state) {
        return PSA_ERROR_CORRUPTION_DETECTED;
    }
    slot->state = new_state;
    return PSA_SUCCESS;
}

                                       
  
                                                                          
                                                          
                         
  
                                 
  
                       
                                                                      
                                         
                                                                               
                                                                    
   
static inline psa_status_t psa_register_read(psa_key_slot_t *slot)
{
    if ((slot->state != PSA_SLOT_FULL) ||
        (slot->var.occupied.registered_readers >= SIZE_MAX)) {
        return PSA_ERROR_CORRUPTION_DETECTED;
    }
    slot->var.occupied.registered_readers++;

    return PSA_SUCCESS;
}

                                       
  
                                                                          
                                                         
                                                        
                                               
                                                          
                         
  
                                                                
                                                                
                                                          
  
                                 
                       
                                                                      
                                                                
                                         
                                                             
                                         
                                                                   
                                         
                                                    
   
psa_status_t psa_unregister_read(psa_key_slot_t *slot);

                                                                    
  
                                                                   
  
                                                                
                                                                
                                                          
  
                                 
                       
                                                                      
                                                                
                                         
                                                             
                                         
                                                                   
                                         
                                                    
   
psa_status_t psa_unregister_read_under_mutex(psa_key_slot_t *slot);

                                                                            
  
                                             
  
            
                                                                     
                                                                        
                                     
            
                                                                        
                   
   
static inline int psa_key_lifetime_is_external(psa_key_lifetime_t lifetime)
{
    return PSA_KEY_LIFETIME_GET_LOCATION(lifetime)
           != PSA_KEY_LOCATION_LOCAL_STORAGE;
}

                              
  
                                                                             
                                                                             
                                          
  
                                                       
                                                                         
                                                                          
                                                                       
  
                                         
                                                        
   
psa_status_t psa_validate_key_location(psa_key_lifetime_t lifetime,
                                       psa_se_drv_table_entry_t **p_drv);

                                      
  
                                                   
  
                                         
                                                                             
                                 
   
psa_status_t psa_validate_key_persistence(psa_key_lifetime_t lifetime);

                              
  
                                               
                                                                            
                                                                              
                                                    
  
                                                            
   
int psa_is_valid_key_id(mbedtls_svc_key_id_t key, int vendor_ok);

#endif                                   
