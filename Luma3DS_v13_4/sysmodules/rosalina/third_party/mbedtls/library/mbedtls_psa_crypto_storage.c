  
                              
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include "common.h"

#if defined(MBEDTLS_PSA_CRYPTO_STORAGE_C)

#include <stdlib.h>
#include <string.h>

#include "psa/crypto.h"
#include "psa_crypto_storage.h"
#include "mbedtls/platform_util.h"

#if defined(MBEDTLS_PSA_ITS_FILE_C)
#include "psa_crypto_its.h"
#else                                
#include "psa/error.h"
#include "psa/internal_trusted_storage.h"
#endif

#include "mbedtls/platform.h"



                                                                  
                 
                                                                  

                                                                            
                                                                          
                                                                             
                                                                       
                 
static psa_storage_uid_t psa_its_identifier_of_slot(mbedtls_svc_key_id_t key)
{
#if defined(MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)
                                                                
                                                                
                                                                   
                                                                       
    uint32_t unsigned_owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(key);
    return ((uint64_t) unsigned_owner_id << 32) |
           MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key);
#else
                                              
                                                            
                                                                     
                                                   
    return key;
#endif
}

   
                                                             
  
                                                                            
          
  
                                                                               
                                                                   
                                                                   
                                                                
  
                                         
                                                    
                                                    
                                                       
                                                      
   
static psa_status_t psa_crypto_storage_load(
    const mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size)
{
    psa_status_t status;
    psa_storage_uid_t data_identifier = psa_its_identifier_of_slot(key);
    struct psa_storage_info_t data_identifier_info;
    size_t data_length = 0;

    status = psa_its_get_info(data_identifier, &data_identifier_info);
    if (status  != PSA_SUCCESS) {
        return status;
    }

    status = psa_its_get(data_identifier, 0, (uint32_t) data_size, data, &data_length);
    if (data_size  != data_length) {
        return PSA_ERROR_DATA_INVALID;
    }

    return status;
}

int psa_is_key_present_in_storage(const mbedtls_svc_key_id_t key)
{
    psa_status_t ret;
    psa_storage_uid_t data_identifier = psa_its_identifier_of_slot(key);
    struct psa_storage_info_t data_identifier_info;

    ret = psa_its_get_info(data_identifier, &data_identifier_info);

    if (ret == PSA_ERROR_DOES_NOT_EXIST) {
        return 0;
    }
    return 1;
}

   
                                                              
  
                                                                      
  
                                                                           
                                                                 
                                                                
                                           
                                              
  
                                         
                                                            
                                                      
                                                       
                                                    
   
static psa_status_t psa_crypto_storage_store(const mbedtls_svc_key_id_t key,
                                             const uint8_t *data,
                                             size_t data_length)
{
    psa_status_t status;
    psa_storage_uid_t data_identifier = psa_its_identifier_of_slot(key);
    struct psa_storage_info_t data_identifier_info;

    if (psa_is_key_present_in_storage(key) == 1) {
        return PSA_ERROR_ALREADY_EXISTS;
    }

    status = psa_its_set(data_identifier, (uint32_t) data_length, data, 0);
    if (status != PSA_SUCCESS) {
        return PSA_ERROR_DATA_INVALID;
    }

    status = psa_its_get_info(data_identifier, &data_identifier_info);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (data_identifier_info.size != data_length) {
        status = PSA_ERROR_DATA_INVALID;
        goto exit;
    }

exit:
    if (status != PSA_SUCCESS) {
                                                                        
                                                                            
                                                                             
                                     
        (void) psa_its_remove(data_identifier);
    }
    return status;
}

psa_status_t psa_destroy_persistent_key(const mbedtls_svc_key_id_t key)
{
    psa_status_t ret;
    psa_storage_uid_t data_identifier = psa_its_identifier_of_slot(key);
    struct psa_storage_info_t data_identifier_info;

    ret = psa_its_get_info(data_identifier, &data_identifier_info);
    if (ret == PSA_ERROR_DOES_NOT_EXIST) {
        return PSA_SUCCESS;
    }

    if (psa_its_remove(data_identifier) != PSA_SUCCESS) {
        return PSA_ERROR_DATA_INVALID;
    }

    ret = psa_its_get_info(data_identifier, &data_identifier_info);
    if (ret != PSA_ERROR_DOES_NOT_EXIST) {
        return PSA_ERROR_DATA_INVALID;
    }

    return PSA_SUCCESS;
}

   
                                                    
  
                                                                          
                                              
                                                                      
  
                                         
                                                       
                                                      
                                                    
   
static psa_status_t psa_crypto_storage_get_data_length(
    const mbedtls_svc_key_id_t key,
    size_t *data_length)
{
    psa_status_t status;
    psa_storage_uid_t data_identifier = psa_its_identifier_of_slot(key);
    struct psa_storage_info_t data_identifier_info;

    status = psa_its_get_info(data_identifier, &data_identifier_info);
    if (status != PSA_SUCCESS) {
        return status;
    }

    *data_length = (size_t) data_identifier_info.size;

    return PSA_SUCCESS;
}

   
                                       
   
#define PSA_KEY_STORAGE_MAGIC_HEADER "PSA\0KEY"
#define PSA_KEY_STORAGE_MAGIC_HEADER_LENGTH (sizeof(PSA_KEY_STORAGE_MAGIC_HEADER))

typedef struct {
    uint8_t magic[PSA_KEY_STORAGE_MAGIC_HEADER_LENGTH];
    uint8_t version[4];
    uint8_t lifetime[sizeof(psa_key_lifetime_t)];
    uint8_t type[2];
    uint8_t bits[2];
    uint8_t policy[sizeof(psa_key_policy_t)];
    uint8_t data_len[4];
    uint8_t key_data[];
} psa_persistent_key_storage_format;

void psa_format_key_data_for_storage(const uint8_t *data,
                                     const size_t data_length,
                                     const psa_key_attributes_t *attr,
                                     uint8_t *storage_data)
{
    psa_persistent_key_storage_format *storage_format =
        (psa_persistent_key_storage_format *) storage_data;

    memcpy(storage_format->magic, PSA_KEY_STORAGE_MAGIC_HEADER,
           PSA_KEY_STORAGE_MAGIC_HEADER_LENGTH);
    MBEDTLS_PUT_UINT32_LE(0, storage_format->version, 0);
    MBEDTLS_PUT_UINT32_LE(attr->lifetime, storage_format->lifetime, 0);
    MBEDTLS_PUT_UINT16_LE((uint16_t) attr->type, storage_format->type, 0);
    MBEDTLS_PUT_UINT16_LE((uint16_t) attr->bits, storage_format->bits, 0);
    MBEDTLS_PUT_UINT32_LE(attr->policy.usage, storage_format->policy, 0);
    MBEDTLS_PUT_UINT32_LE(attr->policy.alg, storage_format->policy, sizeof(uint32_t));
    MBEDTLS_PUT_UINT32_LE(attr->policy.alg2, storage_format->policy, 2 * sizeof(uint32_t));
    MBEDTLS_PUT_UINT32_LE(data_length, storage_format->data_len, 0);
    memcpy(storage_format->key_data, data, data_length);
}

static psa_status_t check_magic_header(const uint8_t *data)
{
    if (memcmp(data, PSA_KEY_STORAGE_MAGIC_HEADER,
               PSA_KEY_STORAGE_MAGIC_HEADER_LENGTH) != 0) {
        return PSA_ERROR_DATA_INVALID;
    }
    return PSA_SUCCESS;
}

psa_status_t psa_parse_key_data_from_storage(const uint8_t *storage_data,
                                             size_t storage_data_length,
                                             uint8_t **key_data,
                                             size_t *key_data_length,
                                             psa_key_attributes_t *attr)
{
    psa_status_t status;
    const psa_persistent_key_storage_format *storage_format =
        (const psa_persistent_key_storage_format *) storage_data;
    uint32_t version;

    if (storage_data_length < sizeof(*storage_format)) {
        return PSA_ERROR_DATA_INVALID;
    }

    status = check_magic_header(storage_data);
    if (status != PSA_SUCCESS) {
        return status;
    }

    version = MBEDTLS_GET_UINT32_LE(storage_format->version, 0);
    if (version != 0) {
        return PSA_ERROR_DATA_INVALID;
    }

    *key_data_length = MBEDTLS_GET_UINT32_LE(storage_format->data_len, 0);
    if (*key_data_length > (storage_data_length - sizeof(*storage_format)) ||
        *key_data_length > PSA_CRYPTO_MAX_STORAGE_SIZE) {
        return PSA_ERROR_DATA_INVALID;
    }

    if (*key_data_length == 0) {
        *key_data = NULL;
    } else {
        *key_data = mbedtls_calloc(1, *key_data_length);
        if (*key_data == NULL) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }
        memcpy(*key_data, storage_format->key_data, *key_data_length);
    }

    attr->lifetime = MBEDTLS_GET_UINT32_LE(storage_format->lifetime, 0);
    attr->type = MBEDTLS_GET_UINT16_LE(storage_format->type, 0);
    attr->bits = MBEDTLS_GET_UINT16_LE(storage_format->bits, 0);
    attr->policy.usage = MBEDTLS_GET_UINT32_LE(storage_format->policy, 0);
    attr->policy.alg = MBEDTLS_GET_UINT32_LE(storage_format->policy, sizeof(uint32_t));
    attr->policy.alg2 = MBEDTLS_GET_UINT32_LE(storage_format->policy, 2 * sizeof(uint32_t));

    return PSA_SUCCESS;
}

psa_status_t psa_save_persistent_key(const psa_key_attributes_t *attr,
                                     const uint8_t *data,
                                     const size_t data_length)
{
    size_t storage_data_length;
    uint8_t *storage_data;
    psa_status_t status;

                                                                        
    if (data == NULL || data_length == 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (data_length > PSA_CRYPTO_MAX_STORAGE_SIZE) {
        return PSA_ERROR_INSUFFICIENT_STORAGE;
    }
    storage_data_length = data_length + sizeof(psa_persistent_key_storage_format);

    storage_data = mbedtls_calloc(1, storage_data_length);
    if (storage_data == NULL) {
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    }

    psa_format_key_data_for_storage(data, data_length, attr, storage_data);

    status = psa_crypto_storage_store(attr->id,
                                      storage_data, storage_data_length);

    mbedtls_zeroize_and_free(storage_data, storage_data_length);

    return status;
}

void psa_free_persistent_key_data(uint8_t *key_data, size_t key_data_length)
{
    mbedtls_zeroize_and_free(key_data, key_data_length);
}

psa_status_t psa_load_persistent_key(psa_key_attributes_t *attr,
                                     uint8_t **data,
                                     size_t *data_length)
{
    psa_status_t status = PSA_SUCCESS;
    uint8_t *loaded_data;
    size_t storage_data_length = 0;
    mbedtls_svc_key_id_t key = attr->id;

    status = psa_crypto_storage_get_data_length(key, &storage_data_length);
    if (status != PSA_SUCCESS) {
        return status;
    }

    loaded_data = mbedtls_calloc(1, storage_data_length);

    if (loaded_data == NULL) {
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    }

    status = psa_crypto_storage_load(key, loaded_data, storage_data_length);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_parse_key_data_from_storage(loaded_data, storage_data_length,
                                             data, data_length, attr);

                                                                        
    if (status == PSA_SUCCESS &&
        (*data == NULL || *data_length == 0)) {
        status = PSA_ERROR_STORAGE_FAILURE;
    }

exit:
    mbedtls_zeroize_and_free(loaded_data, storage_data_length);
    return status;
}



                                                                  
                  
                                                                  

#if defined(PSA_CRYPTO_STORAGE_HAS_TRANSACTIONS)

psa_crypto_transaction_t psa_crypto_transaction;

psa_status_t psa_crypto_save_transaction(void)
{
    struct psa_storage_info_t p_info;
    psa_status_t status;
    status = psa_its_get_info(PSA_CRYPTO_ITS_TRANSACTION_UID, &p_info);
    if (status == PSA_SUCCESS) {
                                                                           
                                                                     
        return PSA_ERROR_CORRUPTION_DETECTED;
    } else if (status != PSA_ERROR_DOES_NOT_EXIST) {
        return status;
    }
    return psa_its_set(PSA_CRYPTO_ITS_TRANSACTION_UID,
                       sizeof(psa_crypto_transaction),
                       &psa_crypto_transaction,
                       0);
}

psa_status_t psa_crypto_load_transaction(void)
{
    psa_status_t status;
    size_t length;
    status = psa_its_get(PSA_CRYPTO_ITS_TRANSACTION_UID, 0,
                         sizeof(psa_crypto_transaction),
                         &psa_crypto_transaction, &length);
    if (status != PSA_SUCCESS) {
        return status;
    }
    if (length != sizeof(psa_crypto_transaction)) {
        return PSA_ERROR_DATA_INVALID;
    }
    return PSA_SUCCESS;
}

psa_status_t psa_crypto_stop_transaction(void)
{
    psa_status_t status = psa_its_remove(PSA_CRYPTO_ITS_TRANSACTION_UID);
                                                                        
                                                                        
               
    memset(&psa_crypto_transaction, 0, sizeof(psa_crypto_transaction));
    return status;
}

#endif                                          



                                                                  
                            
                                                                  

#if defined(MBEDTLS_PSA_INJECT_ENTROPY)
psa_status_t mbedtls_psa_storage_inject_entropy(const unsigned char *seed,
                                                size_t seed_size)
{
    psa_status_t status;
    struct psa_storage_info_t p_info;

    status = psa_its_get_info(PSA_CRYPTO_ITS_RANDOM_SEED_UID, &p_info);

    if (PSA_ERROR_DOES_NOT_EXIST == status) {                     
        status = psa_its_set(PSA_CRYPTO_ITS_RANDOM_SEED_UID, seed_size, seed, 0);
    } else if (PSA_SUCCESS == status) {
                                                                         
        status = PSA_ERROR_NOT_PERMITTED;
    }
    return status;
}
#endif                                 



                                                                  
             
                                                                  

#endif                                   
