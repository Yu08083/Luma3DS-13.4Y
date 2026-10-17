   
                             
  
                                                       
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_STORAGE_H
#define PSA_CRYPTO_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "psa/crypto.h"
#include "psa/crypto_se_driver.h"

#include <stdint.h>
#include <string.h>

                                            
#if defined(MBEDTLS_PSA_STATIC_KEY_SLOTS)
                                                  
#define PSA_CRYPTO_MAX_STORAGE_SIZE (MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE)
#else
                                                                             
                           
#define PSA_CRYPTO_MAX_STORAGE_SIZE (PSA_BITS_TO_BYTES(PSA_MAX_KEY_BITS))
#endif

                                                                  
                       
#if PSA_CRYPTO_MAX_STORAGE_SIZE > 0xffff0000
#error "PSA_CRYPTO_MAX_STORAGE_SIZE > 0xffff0000"
#endif

                                                 
  
                         
                                                         
                                                                
                                                                      
                                                       
                                                                          
  
                                                                        
                                                                           
  
                                                                     
                              
   
#define PSA_MAX_PERSISTENT_KEY_IDENTIFIER PSA_KEY_ID_VENDOR_MAX

   
                                                                           
  
                                                                              
                          
  
                                                       
  
            
                                                     
            
                                                  
   
int psa_is_key_present_in_storage(const mbedtls_svc_key_id_t key);

   
                                                                           
               
  
                                                                    
                                                                        
                                                                           
                                                                             
                                                                          
                                                                           
             
  
                                                                      
                                                                         
                                    
  
                                                              
                                                                      
                                                          
                                                           
                                                                          
  
                                         
                                                        
                                                           
                                                            
                                                       
                                                      
                                                    
                                                    
   
psa_status_t psa_save_persistent_key(const psa_key_attributes_t *attr,
                                     const uint8_t *data,
                                     const size_t data_length);

   
                                                                        
                   
  
                                                                      
                                                                 
  
                                                                             
                                                                            
                                                           
                                                                   
                                              
  
                                                                         
                                                                      
                                                                        
                                                                          
                                                                              
                                                                          
  
                                         
                                                           
                                                    
                                                    
                                                      
   
psa_status_t psa_load_persistent_key(psa_key_attributes_t *attr,
                                     uint8_t **data,
                                     size_t *data_length);

   
                                                               
  
                                                                  
                                                
  
                       
                                            
                                    
                                                    
   
psa_status_t psa_destroy_persistent_key(const mbedtls_svc_key_id_t key);

   
                                                                           
  
                                                                             
                                                                           
  
                                                  
                                                      
  
   
void psa_free_persistent_key_data(uint8_t *key_data, size_t key_data_length);

   
                                                              
  
                                                           
                                                          
                                                           
                                                                 
  
   
void psa_format_key_data_for_storage(const uint8_t *data,
                                     const size_t data_length,
                                     const psa_key_attributes_t *attr,
                                     uint8_t *storage_data);

   
                                                                   
  
                                                           
                                                                
                                                                             
                                                                          
                                                                   
                                                            
                                                                            
                                                            
  
                                         
                                                           
                                                    
   
psa_status_t psa_parse_key_data_from_storage(const uint8_t *storage_data,
                                             size_t storage_data_length,
                                             uint8_t **key_data,
                                             size_t *key_data_length,
                                             psa_key_attributes_t *attr);

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
                                                                 
#define PSA_CRYPTO_STORAGE_HAS_TRANSACTIONS 1
#endif

#if defined(PSA_CRYPTO_STORAGE_HAS_TRANSACTIONS)

                                                
   
                                                                          
                                                                         
                         
   
typedef uint16_t psa_crypto_transaction_type_t;

                                  
  
                                                                            
              
   
#define PSA_CRYPTO_TRANSACTION_NONE             ((psa_crypto_transaction_type_t) 0x0000)

                               
  
                                                                              
                                                                       
                                                                       
   
#define PSA_CRYPTO_TRANSACTION_CREATE_KEY       ((psa_crypto_transaction_type_t) 0x0001)

                                  
  
                                                                              
                                                                         
                                                                       
   
#define PSA_CRYPTO_TRANSACTION_DESTROY_KEY      ((psa_crypto_transaction_type_t) 0x0002)

                     
  
                                                                              
                                          
  
                                                                            
                                             
                                                           
  
                                                                               
                                             
  
                                                                     
                                        
  
                                                                         
                                                                            
                                                                  
                                                                       
                                                     
                                                                        
                                                                           
                                                                      
                                                                     
                                               
                                                                 
                                                    
                                                                     
                                                                         
                                                           
  
                                                                              
                                                                      
                                                                              
                                                            
                                                               
   
typedef union {
                                                                    
                                                       
      
                                 
                                                                               
                                                                     
                    
       
    struct psa_crypto_transaction_unknown_s {
        psa_crypto_transaction_type_t type;
        uint16_t unused1;
        uint32_t unused2;
        uint64_t unused3;
        uint64_t unused4;
    } unknown;
                                                      
                                              
    struct psa_crypto_transaction_key_s {
        psa_crypto_transaction_type_t type;
        uint16_t unused1;
        psa_key_lifetime_t lifetime;
        psa_key_slot_number_t slot;
        mbedtls_svc_key_id_t id;
    } key;
} psa_crypto_transaction_t;

                                  
   
extern psa_crypto_transaction_t psa_crypto_transaction;

                              
  
                                            
  
                                                         
   
static inline void psa_crypto_prepare_transaction(
    psa_crypto_transaction_type_t type)
{
    psa_crypto_transaction.unknown.type = type;
}

                                         
  
                                                                    
                                           
  
                                         
                                                    
                                                            
                                                       
   
psa_status_t psa_crypto_save_transaction(void);

                                                   
  
                                                                        
                                                           
  
                       
                                                                    
                                   
                                    
                                           
                                                       
                                                    
                                                    
   
psa_status_t psa_crypto_load_transaction(void);

                                                      
  
                                                                
                                                                          
                                                                    
                                                   
  
                                                                    
                                         
  
                       
                                                 
                                    
                                                    
                                     
                                                                            
                                                                   
   
psa_status_t psa_crypto_stop_transaction(void);

                                                     
  
                                                         
   
#define PSA_CRYPTO_ITS_TRANSACTION_UID ((psa_key_id_t) 0xffffff74)

#endif                                          

#if defined(MBEDTLS_PSA_INJECT_ENTROPY)
                                                 
  
                                                                     
  
                       
                  
                                                       
                                                            
                                   
                                                
   
psa_status_t mbedtls_psa_storage_inject_entropy(const unsigned char *seed,
                                                size_t seed_size);
#endif                                 

#ifdef __cplusplus
}
#endif

#endif                           
