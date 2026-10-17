  
                                              
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include "common.h"
#include "psa_crypto_core_common.h"

#if defined(MBEDTLS_PSA_CRYPTO_C)

#if defined(MBEDTLS_PSA_CRYPTO_CONFIG)
#include "check_crypto_config.h"
#endif

#include "psa/crypto.h"
#include "psa/crypto_values.h"

#include "psa_crypto_cipher.h"
#include "psa_crypto_core.h"
#include "psa_crypto_invasive.h"
#include "psa_crypto_driver_wrappers.h"
#include "psa_crypto_driver_wrappers_no_static.h"
#include "psa_crypto_ecp.h"
#include "psa_crypto_ffdh.h"
#include "psa_crypto_hash.h"
#include "psa_crypto_mac.h"
#include "psa_crypto_rsa.h"
#include "psa_crypto_ecp.h"
#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
#include "psa_crypto_se.h"
#endif
#include "psa_crypto_slot_management.h"
                                                                              
                  
#include "psa_crypto_storage.h"

#include "psa_crypto_random.h"
#include "psa_crypto_random_impl.h"

#include <stdlib.h>
#include <string.h>
#include "mbedtls/platform.h"

#include "mbedtls/aes.h"
#include "mbedtls/asn1.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/bignum.h"
#include "mbedtls/camellia.h"
#include "mbedtls/chacha20.h"
#include "mbedtls/chachapoly.h"
#include "mbedtls/cipher.h"
#include "mbedtls/ccm.h"
#include "mbedtls/cmac.h"
#include "mbedtls/constant_time.h"
#include "mbedtls/des.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/gcm.h"
#include "mbedtls/md5.h"
#include "mbedtls/pk.h"
#include "pk_wrap.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"
#include "mbedtls/ripemd160.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/psa_util.h"
#include "mbedtls/threading.h"

#include "constant_time_internal.h"

#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF) ||          \
    defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT) ||  \
    defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXPAND)
#define BUILTIN_ALG_ANY_HKDF 1
#endif

                                                                  
                                                           
                                                                  

static int key_type_is_raw_bytes(psa_key_type_t type)
{
    return PSA_KEY_TYPE_IS_UNSTRUCTURED(type);
}

                                             
#define RNG_NOT_INITIALIZED 0
#define RNG_INITIALIZED 1
#define RNG_SEEDED 2

                                                                              
                             
typedef enum {
    PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS = 1,
    PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS,
    PSA_CRYPTO_SUBSYSTEM_RNG,
    PSA_CRYPTO_SUBSYSTEM_TRANSACTION,
} mbedtls_psa_crypto_subsystem;

                                                       
#define PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED    0x01
#define PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS_INITIALIZED          0x02
#define PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED        0x04

#define PSA_CRYPTO_SUBSYSTEM_ALL_INITIALISED                ( \
        PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED | \
        PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS_INITIALIZED | \
        PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED)

typedef struct {
    uint8_t initialized;
    uint8_t rng_state;
    mbedtls_psa_random_context_t rng;
} psa_global_data_t;

static psa_global_data_t global_data;

static uint8_t psa_get_initialized(void)
{
    uint8_t initialized;

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_lock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   

    initialized = global_data.rng_state == RNG_SEEDED;

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_lock(&mbedtls_threading_psa_globaldata_mutex);
#endif                                   

    initialized =
        (initialized && (global_data.initialized == PSA_CRYPTO_SUBSYSTEM_ALL_INITIALISED));

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_globaldata_mutex);
#endif                                   

    return initialized;
}

static uint8_t psa_get_drivers_initialized(void)
{
    uint8_t initialized;

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_lock(&mbedtls_threading_psa_globaldata_mutex);
#endif                                   

    initialized = (global_data.initialized & PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED) != 0;

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_globaldata_mutex);
#endif                                   

    return initialized;
}

#define GUARD_MODULE_INITIALIZED        \
    if (psa_get_initialized() == 0)     \
    return PSA_ERROR_BAD_STATE;

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)

                                                                           
                                                 
  
                                                                          
                                                                           
  
               
                                                              
                                                                      
                                                                  
   
#define LOCAL_INPUT_DECLARE(input, input_copy_name) \
    psa_crypto_local_input_t LOCAL_INPUT_COPY_OF_##input = PSA_CRYPTO_LOCAL_INPUT_INIT; \
    const uint8_t *input_copy_name = NULL;

                                                                        
                                  
  
               
                               
                              
                                                              
                                                                      
   
#define LOCAL_INPUT_ALLOC(input, length, input_copy) \
    status = psa_crypto_local_input_alloc(input, length, \
                                          &LOCAL_INPUT_COPY_OF_##input); \
    if (status != PSA_SUCCESS) { \
        goto exit; \
    } \
    input_copy = LOCAL_INPUT_COPY_OF_##input.buffer;

                                                                        
  
               
                                                                                
                                                             
   
#define LOCAL_INPUT_FREE(input, input_copy) \
    input_copy = NULL; \
    psa_crypto_local_input_free(&LOCAL_INPUT_COPY_OF_##input);

                                                                            
                                                 
  
                                                                          
                                                                            
  
               
                                                               
                                                                        
                                                                   
   
#define LOCAL_OUTPUT_DECLARE(output, output_copy_name) \
    psa_crypto_local_output_t LOCAL_OUTPUT_COPY_OF_##output = PSA_CRYPTO_LOCAL_OUTPUT_INIT; \
    uint8_t *output_copy_name = NULL;

                                                                          
                                  
  
               
                               
                              
                                                               
                                                                         
   
#define LOCAL_OUTPUT_ALLOC(output, length, output_copy) \
    status = psa_crypto_local_output_alloc(output, length, \
                                           &LOCAL_OUTPUT_COPY_OF_##output); \
    if (status != PSA_SUCCESS) { \
        goto exit; \
    } \
    output_copy = LOCAL_OUTPUT_COPY_OF_##output.buffer;

                                                                          
                                                                
  
               
                               
                                                                                   
                                                              
   
#define LOCAL_OUTPUT_FREE(output, output_copy) \
    output_copy = NULL; \
    do { \
        psa_status_t local_output_status; \
        local_output_status = psa_crypto_local_output_free(&LOCAL_OUTPUT_COPY_OF_##output); \
        if (local_output_status != PSA_SUCCESS) { \
                                                                                  \
                                                                                  \
                          \
            status = local_output_status; \
        } \
    } while (0)
#else                                            
#define LOCAL_INPUT_DECLARE(input, input_copy_name) \
    const uint8_t *input_copy_name = NULL;
#define LOCAL_INPUT_ALLOC(input, length, input_copy) \
    input_copy = input;
#define LOCAL_INPUT_FREE(input, input_copy) \
    input_copy = NULL;
#define LOCAL_OUTPUT_DECLARE(output, output_copy_name) \
    uint8_t *output_copy_name = NULL;
#define LOCAL_OUTPUT_ALLOC(output, length, output_copy) \
    output_copy = output;
#define LOCAL_OUTPUT_FREE(output, output_copy) \
    output_copy = NULL;
#endif                                            


int psa_can_do_hash(psa_algorithm_t hash_alg)
{
    (void) hash_alg;
    return psa_get_drivers_initialized();
}

int psa_can_do_cipher(psa_key_type_t key_type, psa_algorithm_t cipher_alg)
{
    (void) key_type;
    (void) cipher_alg;
    return psa_get_drivers_initialized();
}


#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_KEY_PAIR_IMPORT) ||       \
    defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_PUBLIC_KEY) ||     \
    defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_GENERATE)
static int psa_is_dh_key_size_valid(size_t bits)
{
    switch (bits) {
#if defined(PSA_WANT_DH_RFC7919_2048)
        case 2048:
            return 1;
#endif                               
#if defined(PSA_WANT_DH_RFC7919_3072)
        case 3072:
            return 1;
#endif                               
#if defined(PSA_WANT_DH_RFC7919_4096)
        case 4096:
            return 1;
#endif                               
#if defined(PSA_WANT_DH_RFC7919_6144)
        case 6144:
            return 1;
#endif                               
#if defined(PSA_WANT_DH_RFC7919_8192)
        case 8192:
            return 1;
#endif                               
        default:
            return 0;
    }
}
#endif                                                     \
                                                      \
                                                   

psa_status_t mbedtls_to_psa_error(int ret)
{
                                                                     
                                                                     
                                                            
    int low_level_ret = -(-ret & 0x007f);
    switch (low_level_ret != 0 ? low_level_ret : ret) {
        case 0:
            return PSA_SUCCESS;

#if defined(MBEDTLS_AES_C)
        case MBEDTLS_ERR_AES_INVALID_KEY_LENGTH:
        case MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH:
            return PSA_ERROR_NOT_SUPPORTED;
        case MBEDTLS_ERR_AES_BAD_INPUT_DATA:
            return PSA_ERROR_INVALID_ARGUMENT;
#endif

#if defined(MBEDTLS_ASN1_PARSE_C) || defined(MBEDTLS_ASN1_WRITE_C)
        case MBEDTLS_ERR_ASN1_OUT_OF_DATA:
        case MBEDTLS_ERR_ASN1_UNEXPECTED_TAG:
        case MBEDTLS_ERR_ASN1_INVALID_LENGTH:
        case MBEDTLS_ERR_ASN1_LENGTH_MISMATCH:
        case MBEDTLS_ERR_ASN1_INVALID_DATA:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_ASN1_ALLOC_FAILED:
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        case MBEDTLS_ERR_ASN1_BUF_TOO_SMALL:
            return PSA_ERROR_BUFFER_TOO_SMALL;
#endif

#if defined(MBEDTLS_CAMELLIA_C)
        case MBEDTLS_ERR_CAMELLIA_BAD_INPUT_DATA:
        case MBEDTLS_ERR_CAMELLIA_INVALID_INPUT_LENGTH:
            return PSA_ERROR_NOT_SUPPORTED;
#endif

#if defined(MBEDTLS_CCM_C)
        case MBEDTLS_ERR_CCM_BAD_INPUT:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_CCM_AUTH_FAILED:
            return PSA_ERROR_INVALID_SIGNATURE;
#endif

#if defined(MBEDTLS_CHACHA20_C)
        case MBEDTLS_ERR_CHACHA20_BAD_INPUT_DATA:
            return PSA_ERROR_INVALID_ARGUMENT;
#endif

#if defined(MBEDTLS_CHACHAPOLY_C)
        case MBEDTLS_ERR_CHACHAPOLY_BAD_STATE:
            return PSA_ERROR_BAD_STATE;
        case MBEDTLS_ERR_CHACHAPOLY_AUTH_FAILED:
            return PSA_ERROR_INVALID_SIGNATURE;
#endif

#if defined(MBEDTLS_CIPHER_C)
        case MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE:
            return PSA_ERROR_NOT_SUPPORTED;
        case MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_CIPHER_ALLOC_FAILED:
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        case MBEDTLS_ERR_CIPHER_INVALID_PADDING:
            return PSA_ERROR_INVALID_PADDING;
        case MBEDTLS_ERR_CIPHER_FULL_BLOCK_EXPECTED:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_CIPHER_AUTH_FAILED:
            return PSA_ERROR_INVALID_SIGNATURE;
        case MBEDTLS_ERR_CIPHER_INVALID_CONTEXT:
            return PSA_ERROR_CORRUPTION_DETECTED;
#endif

#if !(defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG) ||      \
            defined(MBEDTLS_PSA_HMAC_DRBG_MD_TYPE))
                                                                    
                                                       
        case MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED:
            return PSA_ERROR_INSUFFICIENT_ENTROPY;
        case MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG:
        case MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG:
            return PSA_ERROR_NOT_SUPPORTED;
        case MBEDTLS_ERR_CTR_DRBG_FILE_IO_ERROR:
            return PSA_ERROR_INSUFFICIENT_ENTROPY;
#endif

#if defined(MBEDTLS_DES_C)
        case MBEDTLS_ERR_DES_INVALID_INPUT_LENGTH:
            return PSA_ERROR_NOT_SUPPORTED;
#endif

        case MBEDTLS_ERR_ENTROPY_NO_SOURCES_DEFINED:
        case MBEDTLS_ERR_ENTROPY_NO_STRONG_SOURCE:
        case MBEDTLS_ERR_ENTROPY_SOURCE_FAILED:
            return PSA_ERROR_INSUFFICIENT_ENTROPY;

#if defined(MBEDTLS_GCM_C)
        case MBEDTLS_ERR_GCM_AUTH_FAILED:
            return PSA_ERROR_INVALID_SIGNATURE;
        case MBEDTLS_ERR_GCM_BUFFER_TOO_SMALL:
            return PSA_ERROR_BUFFER_TOO_SMALL;
        case MBEDTLS_ERR_GCM_BAD_INPUT:
            return PSA_ERROR_INVALID_ARGUMENT;
#endif

#if !defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG) &&        \
            defined(MBEDTLS_PSA_HMAC_DRBG_MD_TYPE)
                                                                     
                                                        
        case MBEDTLS_ERR_HMAC_DRBG_ENTROPY_SOURCE_FAILED:
            return PSA_ERROR_INSUFFICIENT_ENTROPY;
        case MBEDTLS_ERR_HMAC_DRBG_REQUEST_TOO_BIG:
        case MBEDTLS_ERR_HMAC_DRBG_INPUT_TOO_BIG:
            return PSA_ERROR_NOT_SUPPORTED;
        case MBEDTLS_ERR_HMAC_DRBG_FILE_IO_ERROR:
            return PSA_ERROR_INSUFFICIENT_ENTROPY;
#endif

#if defined(MBEDTLS_MD_LIGHT)
        case MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE:
            return PSA_ERROR_NOT_SUPPORTED;
        case MBEDTLS_ERR_MD_BAD_INPUT_DATA:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_MD_ALLOC_FAILED:
            return PSA_ERROR_INSUFFICIENT_MEMORY;
#if defined(MBEDTLS_FS_IO)
        case MBEDTLS_ERR_MD_FILE_IO_ERROR:
            return PSA_ERROR_STORAGE_FAILURE;
#endif
#endif

#if defined(MBEDTLS_BIGNUM_C)
#if defined(MBEDTLS_FS_IO)
        case MBEDTLS_ERR_MPI_FILE_IO_ERROR:
            return PSA_ERROR_STORAGE_FAILURE;
#endif
        case MBEDTLS_ERR_MPI_BAD_INPUT_DATA:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_MPI_INVALID_CHARACTER:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL:
            return PSA_ERROR_BUFFER_TOO_SMALL;
        case MBEDTLS_ERR_MPI_NEGATIVE_VALUE:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_MPI_DIVISION_BY_ZERO:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_MPI_NOT_ACCEPTABLE:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_MPI_ALLOC_FAILED:
            return PSA_ERROR_INSUFFICIENT_MEMORY;
#endif

#if defined(MBEDTLS_PK_C)
        case MBEDTLS_ERR_PK_ALLOC_FAILED:
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        case MBEDTLS_ERR_PK_TYPE_MISMATCH:
        case MBEDTLS_ERR_PK_BAD_INPUT_DATA:
            return PSA_ERROR_INVALID_ARGUMENT;
#if defined(MBEDTLS_PSA_CRYPTO_STORAGE_C) || defined(MBEDTLS_FS_IO) || \
            defined(MBEDTLS_PSA_ITS_FILE_C)
        case MBEDTLS_ERR_PK_FILE_IO_ERROR:
            return PSA_ERROR_STORAGE_FAILURE;
#endif
        case MBEDTLS_ERR_PK_KEY_INVALID_VERSION:
        case MBEDTLS_ERR_PK_KEY_INVALID_FORMAT:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_PK_UNKNOWN_PK_ALG:
            return PSA_ERROR_NOT_SUPPORTED;
        case MBEDTLS_ERR_PK_PASSWORD_REQUIRED:
        case MBEDTLS_ERR_PK_PASSWORD_MISMATCH:
            return PSA_ERROR_NOT_PERMITTED;
        case MBEDTLS_ERR_PK_INVALID_PUBKEY:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_PK_INVALID_ALG:
        case MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE:
        case MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE:
            return PSA_ERROR_NOT_SUPPORTED;
        case MBEDTLS_ERR_PK_SIG_LEN_MISMATCH:
            return PSA_ERROR_INVALID_SIGNATURE;
        case MBEDTLS_ERR_PK_BUFFER_TOO_SMALL:
            return PSA_ERROR_BUFFER_TOO_SMALL;
#endif

        case MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED:
            return PSA_ERROR_HARDWARE_FAILURE;
        case MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED:
            return PSA_ERROR_NOT_SUPPORTED;

#if defined(MBEDTLS_RSA_C)
        case MBEDTLS_ERR_RSA_BAD_INPUT_DATA:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_RSA_INVALID_PADDING:
            return PSA_ERROR_INVALID_PADDING;
        case MBEDTLS_ERR_RSA_KEY_GEN_FAILED:
            return PSA_ERROR_HARDWARE_FAILURE;
        case MBEDTLS_ERR_RSA_KEY_CHECK_FAILED:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_RSA_PUBLIC_FAILED:
        case MBEDTLS_ERR_RSA_PRIVATE_FAILED:
            return PSA_ERROR_CORRUPTION_DETECTED;
        case MBEDTLS_ERR_RSA_VERIFY_FAILED:
            return PSA_ERROR_INVALID_SIGNATURE;
        case MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE:
            return PSA_ERROR_BUFFER_TOO_SMALL;
        case MBEDTLS_ERR_RSA_RNG_FAILED:
            return PSA_ERROR_INSUFFICIENT_ENTROPY;
#endif

#if defined(MBEDTLS_ECP_LIGHT)
        case MBEDTLS_ERR_ECP_BAD_INPUT_DATA:
        case MBEDTLS_ERR_ECP_INVALID_KEY:
            return PSA_ERROR_INVALID_ARGUMENT;
        case MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL:
            return PSA_ERROR_BUFFER_TOO_SMALL;
        case MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE:
            return PSA_ERROR_NOT_SUPPORTED;
        case MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH:
        case MBEDTLS_ERR_ECP_VERIFY_FAILED:
            return PSA_ERROR_INVALID_SIGNATURE;
        case MBEDTLS_ERR_ECP_ALLOC_FAILED:
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        case MBEDTLS_ERR_ECP_RANDOM_FAILED:
            return PSA_ERROR_INSUFFICIENT_ENTROPY;

#if defined(MBEDTLS_ECP_RESTARTABLE)
        case MBEDTLS_ERR_ECP_IN_PROGRESS:
            return PSA_OPERATION_INCOMPLETE;
#endif
#endif

        case MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED:
            return PSA_ERROR_CORRUPTION_DETECTED;

        default:
            return PSA_ERROR_GENERIC_ERROR;
    }
}

   
                                                                       
                                                                              
                                                                             
                                                                              
                                                                         
                                                                            
                                                                          
                                                                            
                                                       
  
                                                                          
                                                                     
                                                                     
                                                        
                                                                                
                                              
                                                                                
                                                               
   
static void psa_wipe_tag_output_buffer(uint8_t *output_buffer, psa_status_t status,
                                       size_t output_buffer_size, size_t output_buffer_length)
{
    size_t offset = 0;

    if (output_buffer_size == 0) {
                                                                             
                                                                        
        return;
    }

    if (status == PSA_SUCCESS) {
        offset = output_buffer_length;
    }

    memset(output_buffer + offset, '!', output_buffer_size - offset);
}


psa_status_t psa_validate_unstructured_key_bit_size(psa_key_type_t type,
                                                    size_t bits)
{
                                                                
    switch (type) {
        case PSA_KEY_TYPE_RAW_DATA:
        case PSA_KEY_TYPE_HMAC:
        case PSA_KEY_TYPE_DERIVE:
        case PSA_KEY_TYPE_PASSWORD:
        case PSA_KEY_TYPE_PASSWORD_HASH:
            break;
#if defined(PSA_WANT_KEY_TYPE_AES)
        case PSA_KEY_TYPE_AES:
            if (bits != 128 && bits != 192 && bits != 256) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            break;
#endif
#if defined(PSA_WANT_KEY_TYPE_ARIA)
        case PSA_KEY_TYPE_ARIA:
            if (bits != 128 && bits != 192 && bits != 256) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            break;
#endif
#if defined(PSA_WANT_KEY_TYPE_CAMELLIA)
        case PSA_KEY_TYPE_CAMELLIA:
            if (bits != 128 && bits != 192 && bits != 256) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            break;
#endif
#if defined(PSA_WANT_KEY_TYPE_DES)
        case PSA_KEY_TYPE_DES:
            if (bits != 64 && bits != 128 && bits != 192) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            break;
#endif
#if defined(PSA_WANT_KEY_TYPE_CHACHA20)
        case PSA_KEY_TYPE_CHACHA20:
            if (bits != 256) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            break;
#endif
        default:
            return PSA_ERROR_NOT_SUPPORTED;
    }
    if (bits % 8 != 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return PSA_SUCCESS;
}

                                                                              
  
                                                                           
                                                                               
  
                                                                         
                                                                       
                                         
  
                       
                                                                 
                                      
                                                                     
   
MBEDTLS_STATIC_TESTABLE psa_status_t psa_mac_key_can_do(
    psa_algorithm_t algorithm,
    psa_key_type_t key_type)
{
    if (PSA_ALG_IS_HMAC(algorithm)) {
        if (key_type == PSA_KEY_TYPE_HMAC) {
            return PSA_SUCCESS;
        }
    }

    if (PSA_ALG_IS_BLOCK_CIPHER_MAC(algorithm)) {
                                                                               
                  
        if ((key_type & PSA_KEY_TYPE_CATEGORY_MASK) ==
            PSA_KEY_TYPE_CATEGORY_SYMMETRIC) {
                                                                             
                                                                     
            if (PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) > 1) {
                return PSA_SUCCESS;
            }
        }
    }

    return PSA_ERROR_INVALID_ARGUMENT;
}

psa_status_t psa_allocate_buffer_to_slot(psa_key_slot_t *slot,
                                         size_t buffer_length)
{
#if defined(MBEDTLS_PSA_STATIC_KEY_SLOTS)
    if (buffer_length > ((size_t) MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }
#else
    if (slot->key.data != NULL) {
        return PSA_ERROR_ALREADY_EXISTS;
    }

    slot->key.data = mbedtls_calloc(1, buffer_length);
    if (slot->key.data == NULL) {
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    }
#endif

    slot->key.bytes = buffer_length;
    return PSA_SUCCESS;
}

psa_status_t psa_copy_key_material_into_slot(psa_key_slot_t *slot,
                                             const uint8_t *data,
                                             size_t data_length)
{
    psa_status_t status = psa_allocate_buffer_to_slot(slot,
                                                      data_length);
    if (status != PSA_SUCCESS) {
        return status;
    }

    memcpy(slot->key.data, data, data_length);
    return PSA_SUCCESS;
}

psa_status_t psa_import_key_into_slot(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key_buffer, size_t key_buffer_size,
    size_t *key_buffer_length, size_t *bits)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_type_t type = attributes->type;

                                               
    if (data_length == 0) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (key_type_is_raw_bytes(type)) {
        *bits = PSA_BYTES_TO_BITS(data_length);

        status = psa_validate_unstructured_key_bit_size(attributes->type,
                                                        *bits);
        if (status != PSA_SUCCESS) {
            return status;
        }

                                    
        memcpy(key_buffer, data, data_length);
        *key_buffer_length = data_length;
        (void) key_buffer_size;

        return PSA_SUCCESS;
    } else if (PSA_KEY_TYPE_IS_ASYMMETRIC(type)) {
#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_KEY_PAIR_IMPORT) || \
        defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_PUBLIC_KEY)
        if (PSA_KEY_TYPE_IS_DH(type)) {
            if (psa_is_dh_key_size_valid(PSA_BYTES_TO_BITS(data_length)) == 0) {
                return PSA_ERROR_NOT_SUPPORTED;
            }
            return mbedtls_psa_ffdh_import_key(attributes,
                                               data, data_length,
                                               key_buffer, key_buffer_size,
                                               key_buffer_length,
                                               bits);
        }
#endif                                                              \
                                                                
#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_IMPORT) || \
        defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_PUBLIC_KEY)
        if (PSA_KEY_TYPE_IS_ECC(type)) {
            return mbedtls_psa_ecp_import_key(attributes,
                                              data, data_length,
                                              key_buffer, key_buffer_size,
                                              key_buffer_length,
                                              bits);
        }
#endif                                                               \
                                                                 
#if (defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_KEY_PAIR_IMPORT) && \
        defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_KEY_PAIR_EXPORT)) || \
        defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_PUBLIC_KEY)
        if (PSA_KEY_TYPE_IS_RSA(type)) {
            return mbedtls_psa_rsa_import_key(attributes,
                                              data, data_length,
                                              key_buffer, key_buffer_size,
                                              key_buffer_length,
                                              bits);
        }
#endif                                                                \
                                                                       \
                                                                 
    }

    return PSA_ERROR_NOT_SUPPORTED;
}

                                                               
  
                                                           
   
static psa_algorithm_t psa_key_policy_algorithm_intersection(
    psa_key_type_t key_type,
    psa_algorithm_t alg1,
    psa_algorithm_t alg2)
{
                                                                   
    if (alg1 == alg2) {
        return alg1;
    }
                                                                    
                                                                           
    if (PSA_ALG_IS_SIGN_HASH(alg1) &&
        PSA_ALG_IS_SIGN_HASH(alg2) &&
        (alg1 & ~PSA_ALG_HASH_MASK) == (alg2 & ~PSA_ALG_HASH_MASK)) {
        if (PSA_ALG_SIGN_GET_HASH(alg1) == PSA_ALG_ANY_HASH) {
            return alg2;
        }
        if (PSA_ALG_SIGN_GET_HASH(alg2) == PSA_ALG_ANY_HASH) {
            return alg1;
        }
    }
                                                                   
                                                                       
                                 
    if (PSA_ALG_IS_AEAD(alg1) && PSA_ALG_IS_AEAD(alg2) &&
        (PSA_ALG_AEAD_WITH_SHORTENED_TAG(alg1, 0) ==
         PSA_ALG_AEAD_WITH_SHORTENED_TAG(alg2, 0))) {
        size_t alg1_len = PSA_ALG_AEAD_GET_TAG_LENGTH(alg1);
        size_t alg2_len = PSA_ALG_AEAD_GET_TAG_LENGTH(alg2);
        size_t restricted_len = alg1_len > alg2_len ? alg1_len : alg2_len;

                                                                     
        if (((alg1 & PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG) != 0) &&
            ((alg2 & PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG) != 0)) {
            return PSA_ALG_AEAD_WITH_AT_LEAST_THIS_LENGTH_TAG(
                alg1, restricted_len);
        }
                                                                                 
        if (((alg1 & PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG) != 0) &&
            (alg1_len <= alg2_len)) {
            return alg2;
        }
        if (((alg2 & PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG) != 0) &&
            (alg2_len <= alg1_len)) {
            return alg1;
        }
    }
                                                                      
                                                                 
                                 
    if (PSA_ALG_IS_MAC(alg1) && PSA_ALG_IS_MAC(alg2) &&
        (PSA_ALG_FULL_LENGTH_MAC(alg1) ==
         PSA_ALG_FULL_LENGTH_MAC(alg2))) {
                                                                             
                                                                              
        if (PSA_SUCCESS != psa_mac_key_can_do(alg1, key_type)) {
            return 0;
        }

                                                                           
                                                                             
                                                                                
                                                 
          
                                                                             
                                                                           
                                                                             
        size_t alg1_len = PSA_MAC_LENGTH(key_type, 0, alg1);
        size_t alg2_len = PSA_MAC_LENGTH(key_type, 0, alg2);
        size_t restricted_len = alg1_len > alg2_len ? alg1_len : alg2_len;

                                                                     
        if (((alg1 & PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG) != 0) &&
            ((alg2 & PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG) != 0)) {
            return PSA_ALG_AT_LEAST_THIS_LENGTH_MAC(alg1, restricted_len);
        }

                                                                                
                                                                             
                                                                  
        if ((alg1 & PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG) != 0) {
            return (alg1_len <= alg2_len) ? alg2 : 0;
        }
        if ((alg2 & PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG) != 0) {
            return (alg2_len <= alg1_len) ? alg1 : 0;
        }

                                                                                
                                                                             
                                                                 
                                                           
        if (alg1_len == alg2_len) {
            return PSA_ALG_TRUNCATED_MAC(alg1, alg1_len);
        }
    }
                                                          
    return 0;
}

static int psa_key_algorithm_permits(psa_key_type_t key_type,
                                     psa_algorithm_t policy_alg,
                                     psa_algorithm_t requested_alg)
{
                                                            
    if (requested_alg == policy_alg) {
        return 1;
    }
                                                                     
                                                                        
                                                          
    if (PSA_ALG_IS_SIGN_HASH(requested_alg) &&
        PSA_ALG_SIGN_GET_HASH(policy_alg) == PSA_ALG_ANY_HASH) {
        return (policy_alg & ~PSA_ALG_HASH_MASK) ==
               (requested_alg & ~PSA_ALG_HASH_MASK);
    }
                                                                     
                                                                    
                                                                    
    if (PSA_ALG_IS_AEAD(policy_alg) &&
        PSA_ALG_IS_AEAD(requested_alg) &&
        (PSA_ALG_AEAD_WITH_SHORTENED_TAG(policy_alg, 0) ==
         PSA_ALG_AEAD_WITH_SHORTENED_TAG(requested_alg, 0)) &&
        ((policy_alg & PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG) != 0)) {
        return PSA_ALG_AEAD_GET_TAG_LENGTH(policy_alg) <=
               PSA_ALG_AEAD_GET_TAG_LENGTH(requested_alg);
    }
                                                                         
                                                                    
    if (PSA_ALG_IS_MAC(policy_alg) &&
        PSA_ALG_IS_MAC(requested_alg) &&
        (PSA_ALG_FULL_LENGTH_MAC(policy_alg) ==
         PSA_ALG_FULL_LENGTH_MAC(requested_alg))) {
                                                                               
                                                                            
        if (PSA_SUCCESS != psa_mac_key_can_do(policy_alg, key_type)) {
            return 0;
        }

                                                                                
                                                                          
                                                                              
                                                                              
                                   
        size_t requested_output_length = PSA_MAC_LENGTH(
            key_type, 0, requested_alg);
        size_t default_output_length = PSA_MAC_LENGTH(
            key_type, 0,
            PSA_ALG_FULL_LENGTH_MAC(requested_alg));

                                                                        
                                                           
        if (PSA_MAC_TRUNCATED_LENGTH(policy_alg) == 0) {
            return requested_output_length == default_output_length;
        }

                                                                               
                                                        
        if (PSA_MAC_TRUNCATED_LENGTH(requested_alg) == 0 &&
            PSA_MAC_TRUNCATED_LENGTH(policy_alg) == default_output_length) {
            return 1;
        }

                                                                           
                                                                               
                                     
        if ((policy_alg & PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG) != 0) {
            return PSA_MAC_TRUNCATED_LENGTH(policy_alg) <=
                   requested_output_length;
        }
    }
                                                                            
                                                                            
                                                                                
    if (PSA_ALG_IS_RAW_KEY_AGREEMENT(policy_alg) &&
        PSA_ALG_IS_KEY_AGREEMENT(requested_alg)) {
        return PSA_ALG_KEY_AGREEMENT_GET_BASE(requested_alg) ==
               policy_alg;
    }
                                                           
    return 0;
}

                                               
  
                                               
  
                                                                              
                                                                            
                                                                         
                       
  
                                                                           
                                                                 
                                                                               
                                                                                
                                                                        
   
static psa_status_t psa_key_policy_permits(const psa_key_policy_t *policy,
                                           psa_key_type_t key_type,
                                           psa_algorithm_t alg)
{
                                      
    if (alg == 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

                                                     
    if (PSA_ALG_IS_WILDCARD(alg)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (psa_key_algorithm_permits(key_type, policy->alg, alg) ||
        psa_key_algorithm_permits(key_type, policy->alg2, alg)) {
        return PSA_SUCCESS;
    } else {
        return PSA_ERROR_NOT_PERMITTED;
    }
}

                                                
  
                                                                              
                                                                             
                                                                         
                       
  
                                                                         
                                                   
                                                           
  
                       
                                                                        
                                         
                                      
                                                                       
                                   
   
static psa_status_t psa_restrict_key_policy(
    psa_key_type_t key_type,
    psa_key_policy_t *policy,
    const psa_key_policy_t *constraint)
{
    psa_algorithm_t intersection_alg =
        psa_key_policy_algorithm_intersection(key_type, policy->alg,
                                              constraint->alg);
    psa_algorithm_t intersection_alg2 =
        psa_key_policy_algorithm_intersection(key_type, policy->alg2,
                                              constraint->alg2);
    if (intersection_alg == 0 && policy->alg != 0 && constraint->alg != 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }
    if (intersection_alg2 == 0 && policy->alg2 != 0 && constraint->alg2 != 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }
    policy->usage &= constraint->usage;
    policy->alg = intersection_alg;
    policy->alg2 = intersection_alg2;
    return PSA_SUCCESS;
}

                                                                            
                
  
                                                                            
                                                                           
                                      
  
                                                                             
                                       
  
                                                                     
                                                            
                                                            
                                                             
                                                                          
                                                                    
                                                                             
                                                        
   
static psa_status_t psa_get_and_lock_key_slot_with_policy(
    mbedtls_svc_key_id_t key,
    psa_key_slot_t **p_slot,
    psa_key_usage_t usage,
    psa_algorithm_t alg)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot = NULL;

    status = psa_get_and_lock_key_slot(key, p_slot);
    if (status != PSA_SUCCESS) {
        return status;
    }
    slot = *p_slot;

                                                                        
                                                                      
                                                                     
                                      
    if (PSA_KEY_TYPE_IS_PUBLIC_KEY(slot->attr.type)) {
        usage &= ~PSA_KEY_USAGE_EXPORT;
    }

    if ((slot->attr.policy.usage & usage) != usage) {
        status = PSA_ERROR_NOT_PERMITTED;
        goto error;
    }

                                                                        
    if (alg != 0) {
        status = psa_key_policy_permits(&slot->attr.policy,
                                        slot->attr.type,
                                        alg);
        if (status != PSA_SUCCESS) {
            goto error;
        }
    }

    return PSA_SUCCESS;

error:
    *p_slot = NULL;
    psa_unregister_read_under_mutex(slot);

    return status;
}

                                                            
  
                                                                    
                                                                       
                       
  
                                                           
                                                                              
                                 
  
                                                                     
                                                            
                                                            
                                                             
                                                                          
                                                                    
                                                                             
                                                               
   
static psa_status_t psa_get_and_lock_transparent_key_slot_with_policy(
    mbedtls_svc_key_id_t key,
    psa_key_slot_t **p_slot,
    psa_key_usage_t usage,
    psa_algorithm_t alg)
{
    psa_status_t status = psa_get_and_lock_key_slot_with_policy(key, p_slot,
                                                                usage, alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

    if (psa_key_lifetime_is_external((*p_slot)->attr.lifetime)) {
        psa_unregister_read_under_mutex(*p_slot);
        *p_slot = NULL;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t psa_remove_key_data_from_memory(psa_key_slot_t *slot)
{
#if defined(MBEDTLS_PSA_STATIC_KEY_SLOTS)
    if (slot->key.bytes > 0) {
        mbedtls_platform_zeroize(slot->key.data, MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE);
    }
#else
    if (slot->key.data != NULL) {
        mbedtls_zeroize_and_free(slot->key.data, slot->key.bytes);
    }

    slot->key.data = NULL;
#endif                                   

    slot->key.bytes = 0;

    return PSA_SUCCESS;
}

                                                           
                                         
psa_status_t psa_wipe_key_slot(psa_key_slot_t *slot)
{
    psa_status_t status = psa_remove_key_data_from_memory(slot);

      
                                                                              
                                                                          
                           
                                                                           
              
                                                                        
                                                                       
                                                                              
       
    switch (slot->state) {
        case PSA_SLOT_FULL:
                                                                       
                                        
        case PSA_SLOT_PENDING_DELETION:
                                                                           
                                            
            if (slot->var.occupied.registered_readers != 1) {
                MBEDTLS_TEST_HOOK_TEST_ASSERT(slot->var.occupied.registered_readers == 1);
                status = PSA_ERROR_CORRUPTION_DETECTED;
            }
            break;
        case PSA_SLOT_FILLING:
                                                             
            if (slot->var.occupied.registered_readers != 0) {
                MBEDTLS_TEST_HOOK_TEST_ASSERT(slot->var.occupied.registered_readers == 0);
                status = PSA_ERROR_CORRUPTION_DETECTED;
            }
            break;
        case PSA_SLOT_EMPTY:
                                                                
            MBEDTLS_TEST_HOOK_TEST_ASSERT(slot->state != PSA_SLOT_EMPTY);
            status = PSA_ERROR_CORRUPTION_DETECTED;
            break;
        default:
                                              
            status = PSA_ERROR_CORRUPTION_DETECTED;
    }

#if defined(MBEDTLS_PSA_KEY_STORE_DYNAMIC)
    size_t slice_index = slot->slice_index;
#endif                                    


                                                                    
                                                                   
                                                                   
                                                                    
                                                                     
                                                                      
                                                                       
                                                                  
                                                                   
    memset(slot, 0, sizeof(*slot));

#if defined(MBEDTLS_PSA_KEY_STORE_DYNAMIC)
                                                                     
                                                                    
                                                                       
                                                     
    if (status == PSA_SUCCESS) {
        status = psa_free_key_slot(slice_index, slot);
    }
#endif                                    

    return status;
}

psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key)
{
    psa_key_slot_t *slot;
    psa_status_t status;                                   
    psa_status_t overall_status = PSA_SUCCESS;
#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
    psa_se_drv_table_entry_t *driver;
#endif                              

    if (mbedtls_svc_key_id_is_null(key)) {
        return PSA_SUCCESS;
    }

      
                                                                             
                                                                          
                                              
                                                                  
                                                                            
                                     
    status = psa_get_and_lock_key_slot(key, &slot);
    if (status != PSA_SUCCESS) {
        return status;
    }

#if defined(MBEDTLS_THREADING_C)
                                                                     
                                                                     
                                                                 
                          
    PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_lock(
                                    &mbedtls_threading_key_slot_mutex));

    if (slot->state == PSA_SLOT_PENDING_DELETION) {
                                                                           
                                                              
                                                     
        status = psa_unregister_read(slot);

        PSA_THREADING_CHK_RET(mbedtls_mutex_unlock(
                                  &mbedtls_threading_key_slot_mutex));
        return (status == PSA_SUCCESS) ? PSA_ERROR_INVALID_HANDLE : status;
    }
#endif
                                                                 
                                                                   
                                                                      
                                                               
                                                                        
                                                                      
                                                                  
                                 
    overall_status = psa_key_slot_state_transition(slot, PSA_SLOT_FULL,
                                                   PSA_SLOT_PENDING_DELETION);

    if (overall_status != PSA_SUCCESS) {
        goto exit;
    }

    if (PSA_KEY_LIFETIME_IS_READ_ONLY(slot->attr.lifetime)) {
                                                                               
                                                                             
                                                       
                                                                       
                                                     
        overall_status = PSA_ERROR_NOT_PERMITTED;
        goto exit;
    }

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
    driver = psa_get_se_driver_entry(slot->attr.lifetime);
    if (driver != NULL) {
                                                                     
                                                               
                                                                 
                                                                         
                            
        psa_crypto_prepare_transaction(PSA_CRYPTO_TRANSACTION_DESTROY_KEY);
        psa_crypto_transaction.key.lifetime = slot->attr.lifetime;
        psa_crypto_transaction.key.slot = psa_key_slot_get_slot_number(slot);
        psa_crypto_transaction.key.id = slot->attr.id;
        status = psa_crypto_save_transaction();
        if (status != PSA_SUCCESS) {
            (void) psa_crypto_stop_transaction();
                                                                   
                                                                          
                                                                  
                                                                       
                                   
                                                                
               
            overall_status = status;
            goto exit;
        }

        status = psa_destroy_se_key(driver,
                                    psa_key_slot_get_slot_number(slot));
        if (overall_status == PSA_SUCCESS) {
            overall_status = status;
        }
    }
#endif                              

#if defined(MBEDTLS_PSA_CRYPTO_STORAGE_C)
    if (!PSA_KEY_LIFETIME_IS_VOLATILE(slot->attr.lifetime)) {
                                                               
                                                                           
                          
        status = psa_destroy_persistent_key(slot->attr.id);
        if (overall_status == PSA_SUCCESS) {
            overall_status = status;
        }
    }
#endif                                            

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
    if (driver != NULL) {
        status = psa_save_se_persistent_data(driver);
        if (overall_status == PSA_SUCCESS) {
            overall_status = status;
        }
        status = psa_crypto_stop_transaction();
        if (overall_status == PSA_SUCCESS) {
            overall_status = status;
        }
    }
#endif                              

exit:
                                                                         
                                       
    status = psa_unregister_read(slot);
                                                             
                          
    if (status != PSA_SUCCESS) {
        overall_status = status;
    }

#if defined(MBEDTLS_THREADING_C)
                                                              
    status = overall_status;
    PSA_THREADING_CHK_RET(mbedtls_mutex_unlock(
                              &mbedtls_threading_key_slot_mutex));
#endif

    return overall_status;
}

                                                             
   
psa_status_t psa_get_key_attributes(mbedtls_svc_key_id_t key,
                                    psa_key_attributes_t *attributes)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    psa_reset_key_attributes(attributes);

    status = psa_get_and_lock_key_slot_with_policy(key, &slot, 0, 0);
    if (status != PSA_SUCCESS) {
        return status;
    }

    *attributes = slot->attr;

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
    if (psa_get_se_driver_entry(slot->attr.lifetime) != NULL) {
        psa_set_key_slot_number(attributes,
                                psa_key_slot_get_slot_number(slot));
    }
#endif                              

    return psa_unregister_read_under_mutex(slot);
}

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
psa_status_t psa_get_key_slot_number(
    const psa_key_attributes_t *attributes,
    psa_key_slot_number_t *slot_number)
{
    if (attributes->has_slot_number) {
        *slot_number = attributes->slot_number;
        return PSA_SUCCESS;
    } else {
        return PSA_ERROR_INVALID_ARGUMENT;
    }
}
#endif                              

static psa_status_t psa_export_key_buffer_internal(const uint8_t *key_buffer,
                                                   size_t key_buffer_size,
                                                   uint8_t *data,
                                                   size_t data_size,
                                                   size_t *data_length)
{
    if (key_buffer_size > data_size) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }
    memcpy(data, key_buffer, key_buffer_size);
    memset(data + key_buffer_size, 0,
           data_size - key_buffer_size);
    *data_length = key_buffer_size;
    return PSA_SUCCESS;
}

psa_status_t psa_export_key_internal(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    uint8_t *data, size_t data_size, size_t *data_length)
{
    psa_key_type_t type = attributes->type;

    if (key_type_is_raw_bytes(type) ||
        PSA_KEY_TYPE_IS_RSA(type)   ||
        PSA_KEY_TYPE_IS_ECC(type)   ||
        PSA_KEY_TYPE_IS_DH(type)) {
        return psa_export_key_buffer_internal(
            key_buffer, key_buffer_size,
            data, data_size, data_length);
    } else {
                                                                    
                                                            
                                                      
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t psa_export_key(mbedtls_svc_key_id_t key,
                            uint8_t *data_external,
                            size_t data_size,
                            size_t *data_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;
    LOCAL_OUTPUT_DECLARE(data_external, data);

                                                                        
                                                                           
                                                                         
    if (data_size == 0) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }

                                                                              
                                                                            
                                                                               
                                           
    *data_length = 0;

                                                                              
                                        
                                                                  
       
    status = psa_get_and_lock_key_slot_with_policy(key, &slot,
                                                   PSA_KEY_USAGE_EXPORT, 0);
    if (status != PSA_SUCCESS) {
        return status;
    }

    LOCAL_OUTPUT_ALLOC(data_external, data_size, data);

    status = psa_driver_wrapper_export_key(&slot->attr,
                                           slot->key.data, slot->key.bytes,
                                           data, data_size, data_length);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    unlock_status = psa_unregister_read_under_mutex(slot);

    LOCAL_OUTPUT_FREE(data_external, data);
    return (status == PSA_SUCCESS) ? unlock_status : status;
}

psa_status_t psa_export_public_key_internal(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    uint8_t *data,
    size_t data_size,
    size_t *data_length)
{
    psa_key_type_t type = attributes->type;

    if (PSA_KEY_TYPE_IS_PUBLIC_KEY(type) &&
        (PSA_KEY_TYPE_IS_RSA(type) || PSA_KEY_TYPE_IS_ECC(type) ||
         PSA_KEY_TYPE_IS_DH(type))) {
                                        
        return psa_export_key_buffer_internal(
            key_buffer, key_buffer_size,
            data, data_size, data_length);
    } else if (PSA_KEY_TYPE_IS_RSA(type)) {
#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_KEY_PAIR_EXPORT) || \
        defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_PUBLIC_KEY)
        return mbedtls_psa_rsa_export_public_key(attributes,
                                                 key_buffer,
                                                 key_buffer_size,
                                                 data,
                                                 data_size,
                                                 data_length);
#else
                                                                       
        return PSA_ERROR_NOT_SUPPORTED;
#endif                                                               \
                                                                 
    } else if (PSA_KEY_TYPE_IS_ECC(type)) {
#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_EXPORT) || \
        defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_PUBLIC_KEY)
        return mbedtls_psa_ecp_export_public_key(attributes,
                                                 key_buffer,
                                                 key_buffer_size,
                                                 data,
                                                 data_size,
                                                 data_length);
#else
                                                                      
        return PSA_ERROR_NOT_SUPPORTED;
#endif                                                               \
                                                                 
    } else if (PSA_KEY_TYPE_IS_DH(type)) {
#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_KEY_PAIR_EXPORT) || \
        defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_PUBLIC_KEY)
        return mbedtls_psa_ffdh_export_public_key(attributes,
                                                  key_buffer,
                                                  key_buffer_size,
                                                  data, data_size,
                                                  data_length);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif                                                              \
                                                                
    } else {
        (void) key_buffer;
        (void) key_buffer_size;
        (void) data;
        (void) data_size;
        (void) data_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t psa_export_public_key(mbedtls_svc_key_id_t key,
                                   uint8_t *data_external,
                                   size_t data_size,
                                   size_t *data_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    LOCAL_OUTPUT_DECLARE(data_external, data);

                                                                        
                                                                           
                                                                         
    if (data_size == 0) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }

                                                                              
                                                                            
                                                                               
                                           
    *data_length = 0;

                                                              
    status = psa_get_and_lock_key_slot_with_policy(key, &slot, 0, 0);
    if (status != PSA_SUCCESS) {
        return status;
    }

    LOCAL_OUTPUT_ALLOC(data_external, data_size, data);

    if (!PSA_KEY_TYPE_IS_ASYMMETRIC(slot->attr.type)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    status = psa_driver_wrapper_export_public_key(
        &slot->attr, slot->key.data, slot->key.bytes,
        data, data_size, data_length);

exit:
    unlock_status = psa_unregister_read_under_mutex(slot);

    LOCAL_OUTPUT_FREE(data_external, data);
    return (status == PSA_SUCCESS) ? unlock_status : status;
}

                                                         
  
                                                                        
                                                                        
                        
   
static psa_status_t psa_validate_key_policy(const psa_key_policy_t *policy)
{
    if ((policy->usage & ~(PSA_KEY_USAGE_EXPORT |
                           PSA_KEY_USAGE_COPY |
                           PSA_KEY_USAGE_ENCRYPT |
                           PSA_KEY_USAGE_DECRYPT |
                           PSA_KEY_USAGE_SIGN_MESSAGE |
                           PSA_KEY_USAGE_VERIFY_MESSAGE |
                           PSA_KEY_USAGE_SIGN_HASH |
                           PSA_KEY_USAGE_VERIFY_HASH |
                           PSA_KEY_USAGE_VERIFY_DERIVATION |
                           PSA_KEY_USAGE_DERIVE)) != 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return PSA_SUCCESS;
}

                                                        
  
                                                                   
                                                                        
                                          
  
                                                        
  
                                                           
                                                                          
                                                       
  
   
static psa_status_t psa_validate_key_attributes(
    const psa_key_attributes_t *attributes,
    psa_se_drv_table_entry_t **p_drv)
{
    psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
    psa_key_lifetime_t lifetime = psa_get_key_lifetime(attributes);
    mbedtls_svc_key_id_t key = psa_get_key_id(attributes);

    status = psa_validate_key_location(lifetime, p_drv);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_validate_key_persistence(lifetime);
    if (status != PSA_SUCCESS) {
        return status;
    }

    if (PSA_KEY_LIFETIME_IS_VOLATILE(lifetime)) {
        if (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key) != 0) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    } else {
        if (!psa_is_valid_key_id(psa_get_key_id(attributes), 0)) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    status = psa_validate_key_policy(&attributes->policy);
    if (status != PSA_SUCCESS) {
        return status;
    }

                                          
                                                                       
                                                                    
                                                
    if (psa_get_key_bits(attributes) > PSA_MAX_KEY_BITS) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

                                               
  
                                                            
  
                                                        
  
                                                   
                                                                   
                                                                               
                                
                                              
                                                                          
                                                             
                           
  
                                                        
                                                                       
                                                      
  
                                                                      
                                                           
                                                                       
                                                                          
                                                       
  
                       
                                                         
                                                                    
                                                                           
   
static psa_status_t psa_start_key_creation(
    psa_key_creation_method_t method,
    const psa_key_attributes_t *attributes,
    psa_key_slot_t **p_slot,
    psa_se_drv_table_entry_t **p_drv)
{
    psa_status_t status;

    (void) method;
    *p_drv = NULL;

    status = psa_validate_key_attributes(attributes, p_drv);
    if (status != PSA_SUCCESS) {
        return status;
    }

    int key_is_volatile = PSA_KEY_LIFETIME_IS_VOLATILE(attributes->lifetime);
    psa_key_id_t volatile_key_id;

#if defined(MBEDTLS_THREADING_C)
    PSA_THREADING_CHK_RET(mbedtls_mutex_lock(
                              &mbedtls_threading_key_slot_mutex));
#endif
    status = psa_reserve_free_key_slot(
        key_is_volatile ? &volatile_key_id : NULL,
        p_slot);
#if defined(MBEDTLS_THREADING_C)
    PSA_THREADING_CHK_RET(mbedtls_mutex_unlock(
                              &mbedtls_threading_key_slot_mutex));
#endif
    if (status != PSA_SUCCESS) {
        return status;
    }
    psa_key_slot_t *slot = *p_slot;

                                                                      
                                                                     
                                                                         
                                                                       
                                                                           
                                                                             
                     

    slot->attr = *attributes;
    if (key_is_volatile) {
#if !defined(MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)
        slot->attr.id = volatile_key_id;
#else
        slot->attr.id.key_id = volatile_key_id;
#endif
    }

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
                                                                
                                                     
                                                          
                                                             
                                                                        
                                     
                                                                          
                                                                                
                                        
                                                                      
                                                                   
                                                                  
                                                                       
                                                                      
                                                                            
                                                                      
                                                                  
    if (*p_drv != NULL) {
        psa_key_slot_number_t slot_number;
        status = psa_find_se_slot_for_key(attributes, method, *p_drv,
                                          &slot_number);
        if (status != PSA_SUCCESS) {
            return status;
        }

        if (!PSA_KEY_LIFETIME_IS_VOLATILE(attributes->lifetime)) {
            psa_crypto_prepare_transaction(PSA_CRYPTO_TRANSACTION_CREATE_KEY);
            psa_crypto_transaction.key.lifetime = slot->attr.lifetime;
            psa_crypto_transaction.key.slot = slot_number;
            psa_crypto_transaction.key.id = slot->attr.id;
            status = psa_crypto_save_transaction();
            if (status != PSA_SUCCESS) {
                (void) psa_crypto_stop_transaction();
                return status;
            }
        }

        status = psa_copy_key_material_into_slot(
            slot, (uint8_t *) (&slot_number), sizeof(slot_number));
        if (status != PSA_SUCCESS) {
            return status;
        }
    }

    if (*p_drv == NULL && method == PSA_KEY_CREATION_REGISTER) {
                                                                      
        return PSA_ERROR_INVALID_ARGUMENT;
    }
#endif                              

    return PSA_SUCCESS;
}

                                                                      
  
                                                      
  
                                                        
                                                                         
                    
  
                                                                          
                                                                           
                        
  
                                                              
                                                              
                                                      
                                                                        
                                                                      
  
                       
                                            
                                                           
                                                            
                                                      
                                                    
                                                    
                                                       
  
                                                                    
                                                                           
   
static psa_status_t psa_finish_key_creation(
    psa_key_slot_t *slot,
    psa_se_drv_table_entry_t *driver,
    mbedtls_svc_key_id_t *key)
{
    psa_status_t status = PSA_SUCCESS;
    (void) slot;
    (void) driver;

#if defined(MBEDTLS_THREADING_C)
    PSA_THREADING_CHK_RET(mbedtls_mutex_lock(
                              &mbedtls_threading_key_slot_mutex));
#endif

#if defined(MBEDTLS_PSA_CRYPTO_STORAGE_C)
    if (!PSA_KEY_LIFETIME_IS_VOLATILE(slot->attr.lifetime)) {
#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
        if (driver != NULL) {
            psa_se_key_data_storage_t data;
            psa_key_slot_number_t slot_number =
                psa_key_slot_get_slot_number(slot);

            MBEDTLS_STATIC_ASSERT(sizeof(slot_number) ==
                                  sizeof(data.slot_number),
                                  "Slot number size does not match psa_se_key_data_storage_t");

            memcpy(&data.slot_number, &slot_number, sizeof(slot_number));
            status = psa_save_persistent_key(&slot->attr,
                                             (uint8_t *) &data,
                                             sizeof(data));
        } else
#endif                              
        {
                                                                             
                                                        
            status = psa_save_persistent_key(&slot->attr,
                                             slot->key.data,
                                             slot->key.bytes);
        }
    }
#endif                                            

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
                                                               
                                                                
                                                               
                                                                               
                                                                     
    if (driver != NULL &&
        psa_crypto_transaction.unknown.type == PSA_CRYPTO_TRANSACTION_CREATE_KEY) {
        status = psa_save_se_persistent_data(driver);
        if (status != PSA_SUCCESS) {
            psa_destroy_persistent_key(slot->attr.id);

#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_RET(mbedtls_mutex_unlock(
                                      &mbedtls_threading_key_slot_mutex));
#endif
            return status;
        }
        status = psa_crypto_stop_transaction();
    }
#endif                              

    if (status == PSA_SUCCESS) {
        *key = slot->attr.id;
        status = psa_key_slot_state_transition(slot, PSA_SLOT_FILLING,
                                               PSA_SLOT_FULL);
        if (status != PSA_SUCCESS) {
            *key = MBEDTLS_SVC_KEY_ID_INIT;
        }
    }

#if defined(MBEDTLS_THREADING_C)
    PSA_THREADING_CHK_RET(mbedtls_mutex_unlock(
                              &mbedtls_threading_key_slot_mutex));
#endif
    return status;
}

                                
  
                                                                     
                                                                         
                                                
                                                                         
                                                             
  
                                                              
                                                              
                                                      
   
static void psa_fail_key_creation(psa_key_slot_t *slot,
                                  psa_se_drv_table_entry_t *driver)
{
    (void) driver;

    if (slot == NULL) {
        return;
    }

#if defined(MBEDTLS_THREADING_C)
                                                          
                                                          
                                                                  
    mbedtls_mutex_lock(&mbedtls_threading_key_slot_mutex);
#endif

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
                                                              
                                                                    
                                                                     
               
                                                        
       

                                                                    
                                                                 
                                                                    
                                                                         
                                                                          
                 
    (void) psa_crypto_stop_transaction();
#endif                              

    psa_wipe_key_slot(slot);

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_key_slot_mutex);
#endif
}

                                                     
  
                                                                    
                                                                        
                             
  
                                                                     
                                                                           
   
static psa_status_t psa_validate_optional_attributes(
    const psa_key_slot_t *slot,
    const psa_key_attributes_t *attributes)
{
    if (attributes->type != 0) {
        if (attributes->type != slot->attr.type) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    if (attributes->bits != 0) {
        if (attributes->bits != slot->attr.bits) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    return PSA_SUCCESS;
}

psa_status_t psa_import_key(const psa_key_attributes_t *attributes,
                            const uint8_t *data_external,
                            size_t data_length,
                            mbedtls_svc_key_id_t *key)
{
    psa_status_t status;
    LOCAL_INPUT_DECLARE(data_external, data);
    psa_key_slot_t *slot = NULL;
    psa_se_drv_table_entry_t *driver = NULL;
    size_t bits;
    size_t storage_size = data_length;

    *key = MBEDTLS_SVC_KEY_ID_INIT;

                                                                          
                                                                           
                               
    if (data_length == 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

                                                                   
    if (data_length > SIZE_MAX / 8) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    LOCAL_INPUT_ALLOC(data_external, data_length, data);

    status = psa_start_key_creation(PSA_KEY_CREATION_IMPORT, attributes,
                                    &slot, &driver);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

                                                                        
                                                                            
                                                                       
                                                   
    if (slot->key.bytes == 0) {
        if (psa_key_lifetime_is_external(attributes->lifetime)) {
            status = psa_driver_wrapper_get_key_buffer_size_from_key_data(
                attributes, data, data_length, &storage_size);
            if (status != PSA_SUCCESS) {
                goto exit;
            }
        }
        status = psa_allocate_buffer_to_slot(slot, storage_size);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
    }

    bits = slot->attr.bits;
    status = psa_driver_wrapper_import_key(attributes,
                                           data, data_length,
                                           slot->key.data,
                                           slot->key.bytes,
                                           &slot->key.bytes, &bits);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (slot->attr.bits == 0) {
        slot->attr.bits = (psa_key_bits_t) bits;
    } else if (bits != slot->attr.bits) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

                                                                  
                                              
    if (bits > PSA_MAX_KEY_BITS) {
        status = PSA_ERROR_NOT_SUPPORTED;
        goto exit;
    }
    status = psa_validate_optional_attributes(slot, attributes);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_finish_key_creation(slot, driver, key);
exit:
    LOCAL_INPUT_FREE(data_external, data);
    if (status != PSA_SUCCESS) {
        psa_fail_key_creation(slot, driver);
    }

    return status;
}

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
psa_status_t mbedtls_psa_register_se_key(
    const psa_key_attributes_t *attributes)
{
    psa_status_t status;
    psa_key_slot_t *slot = NULL;
    psa_se_drv_table_entry_t *driver = NULL;
    mbedtls_svc_key_id_t key = MBEDTLS_SVC_KEY_ID_INIT;

                                                                 
                                                                  
                                                               
                                                          
    if (psa_get_key_type(attributes) == PSA_KEY_TYPE_NONE) {
        return PSA_ERROR_NOT_SUPPORTED;
    }
    if (psa_get_key_bits(attributes) == 0) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

                                                                        
                             
                                                      
       
    if (PSA_KEY_LIFETIME_IS_VOLATILE(psa_get_key_lifetime(attributes))) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    status = psa_start_key_creation(PSA_KEY_CREATION_REGISTER, attributes,
                                    &slot, &driver);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_finish_key_creation(slot, driver, &key);

exit:
    if (status != PSA_SUCCESS) {
        psa_fail_key_creation(slot, driver);
    }

                                                   
    psa_close_key(key);
    return status;
}
#endif                              

psa_status_t psa_copy_key(mbedtls_svc_key_id_t source_key,
                          const psa_key_attributes_t *specified_attributes,
                          mbedtls_svc_key_id_t *target_key)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *source_slot = NULL;
    psa_key_slot_t *target_slot = NULL;
    psa_key_attributes_t actual_attributes = *specified_attributes;
    psa_se_drv_table_entry_t *driver = NULL;
    size_t storage_size = 0;

    *target_key = MBEDTLS_SVC_KEY_ID_INIT;

    status = psa_get_and_lock_key_slot_with_policy(
        source_key, &source_slot, PSA_KEY_USAGE_COPY, 0);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_validate_optional_attributes(source_slot,
                                              specified_attributes);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

                                                                    
                                                                       
                                                                    
                                     
         
    actual_attributes.bits = source_slot->attr.bits;
    actual_attributes.type = source_slot->attr.type;


    status = psa_restrict_key_policy(source_slot->attr.type,
                                     &actual_attributes.policy,
                                     &source_slot->attr.policy);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_start_key_creation(PSA_KEY_CREATION_COPY, &actual_attributes,
                                    &target_slot, &driver);
    if (status != PSA_SUCCESS) {
        goto exit;
    }
    if (PSA_KEY_LIFETIME_GET_LOCATION(target_slot->attr.lifetime) !=
        PSA_KEY_LIFETIME_GET_LOCATION(source_slot->attr.lifetime)) {
          
                                                                           
                                                                                
                                                                               
                                                                   
                                                                       
             
        status = PSA_ERROR_NOT_SUPPORTED;
        goto exit;
    }
      
                                                                    
                                                                               
                                                                         
                                                         
         
    if (psa_key_lifetime_is_external(actual_attributes.lifetime)) {
        status = psa_driver_wrapper_get_key_buffer_size(&actual_attributes,
                                                        &storage_size);
        if (status != PSA_SUCCESS) {
            goto exit;
        }

        status = psa_allocate_buffer_to_slot(target_slot, storage_size);
        if (status != PSA_SUCCESS) {
            goto exit;
        }

        status = psa_driver_wrapper_copy_key(&actual_attributes,
                                             source_slot->key.data,
                                             source_slot->key.bytes,
                                             target_slot->key.data,
                                             target_slot->key.bytes,
                                             &target_slot->key.bytes);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
    } else {
        status = psa_copy_key_material_into_slot(target_slot,
                                                 source_slot->key.data,
                                                 source_slot->key.bytes);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
    }
    status = psa_finish_key_creation(target_slot, driver, target_key);
exit:
    if (status != PSA_SUCCESS) {
        psa_fail_key_creation(target_slot, driver);
    }

    unlock_status = psa_unregister_read_under_mutex(source_slot);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}



                                                                  
                     
                                                                  

static int is_hash_supported(psa_algorithm_t alg)
{
    switch (alg) {
#if defined(PSA_WANT_ALG_MD5)
        case PSA_ALG_MD5:
            return 1;
#endif
#if defined(PSA_WANT_ALG_RIPEMD160)
        case PSA_ALG_RIPEMD160:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA_1)
        case PSA_ALG_SHA_1:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA_224)
        case PSA_ALG_SHA_224:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA_256)
        case PSA_ALG_SHA_256:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA_384)
        case PSA_ALG_SHA_384:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA_512)
        case PSA_ALG_SHA_512:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA3_224)
        case PSA_ALG_SHA3_224:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA3_256)
        case PSA_ALG_SHA3_256:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA3_384)
        case PSA_ALG_SHA3_384:
            return 1;
#endif
#if defined(PSA_WANT_ALG_SHA3_512)
        case PSA_ALG_SHA3_512:
            return 1;
#endif
        default:
            return 0;
    }
}

psa_status_t psa_hash_abort(psa_hash_operation_t *operation)
{
                                                    
    if (operation->id == 0) {
        return PSA_SUCCESS;
    }

    psa_status_t status = psa_driver_wrapper_hash_abort(operation);
    operation->id = 0;

    return status;
}

psa_status_t psa_hash_setup(psa_hash_operation_t *operation,
                            psa_algorithm_t alg)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

                                                                        
    if (operation->id != 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (!PSA_ALG_IS_HASH(alg)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

                                                                      
                                                                         
                                                                   
                                                                     
                             
    memset(&operation->ctx, 0, sizeof(operation->ctx));

    status = psa_driver_wrapper_hash_setup(operation, alg);

exit:
    if (status != PSA_SUCCESS) {
        psa_hash_abort(operation);
    }

    return status;
}

psa_status_t psa_hash_update(psa_hash_operation_t *operation,
                             const uint8_t *input_external,
                             size_t input_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(input_external, input);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

                                                                  
                                                               
    if (input_length == 0) {
        return PSA_SUCCESS;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    status = psa_driver_wrapper_hash_update(operation, input, input_length);

exit:
    if (status != PSA_SUCCESS) {
        psa_hash_abort(operation);
    }

    LOCAL_INPUT_FREE(input_external, input);
    return status;
}

static psa_status_t psa_hash_finish_internal(psa_hash_operation_t *operation,
                                             uint8_t *hash,
                                             size_t hash_size,
                                             size_t *hash_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    *hash_length = 0;
    if (operation->id == 0) {
        return PSA_ERROR_BAD_STATE;
    }

    status = psa_driver_wrapper_hash_finish(
        operation, hash, hash_size, hash_length);
    psa_hash_abort(operation);

    return status;
}

psa_status_t psa_hash_finish(psa_hash_operation_t *operation,
                             uint8_t *hash_external,
                             size_t hash_size,
                             size_t *hash_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_OUTPUT_DECLARE(hash_external, hash);

    LOCAL_OUTPUT_ALLOC(hash_external, hash_size, hash);
    status = psa_hash_finish_internal(operation, hash, hash_size, hash_length);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_OUTPUT_FREE(hash_external, hash);
    return status;
}

psa_status_t psa_hash_verify(psa_hash_operation_t *operation,
                             const uint8_t *hash_external,
                             size_t hash_length)
{
    uint8_t actual_hash[PSA_HASH_MAX_SIZE];
    size_t actual_hash_length;
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(hash_external, hash);

    status = psa_hash_finish_internal(
        operation,
        actual_hash, sizeof(actual_hash),
        &actual_hash_length);

    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (actual_hash_length != hash_length) {
        status = PSA_ERROR_INVALID_SIGNATURE;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(hash_external, hash_length, hash);
    if (mbedtls_ct_memcmp(hash, actual_hash, actual_hash_length) != 0) {
        status = PSA_ERROR_INVALID_SIGNATURE;
    }

exit:
    mbedtls_platform_zeroize(actual_hash, sizeof(actual_hash));
    if (status != PSA_SUCCESS) {
        psa_hash_abort(operation);
    }
    LOCAL_INPUT_FREE(hash_external, hash);
    return status;
}

psa_status_t psa_hash_compute(psa_algorithm_t alg,
                              const uint8_t *input_external, size_t input_length,
                              uint8_t *hash_external, size_t hash_size,
                              size_t *hash_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_OUTPUT_DECLARE(hash_external, hash);

    *hash_length = 0;
    if (!PSA_ALG_IS_HASH(alg)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_OUTPUT_ALLOC(hash_external, hash_size, hash);
    status = psa_driver_wrapper_hash_compute(alg, input, input_length,
                                             hash, hash_size, hash_length);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_OUTPUT_FREE(hash_external, hash);
    return status;
}

psa_status_t psa_hash_compare(psa_algorithm_t alg,
                              const uint8_t *input_external, size_t input_length,
                              const uint8_t *hash_external, size_t hash_length)
{
    uint8_t actual_hash[PSA_HASH_MAX_SIZE];
    size_t actual_hash_length;
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_INPUT_DECLARE(hash_external, hash);

    if (!PSA_ALG_IS_HASH(alg)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        return status;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    status = psa_driver_wrapper_hash_compute(
        alg, input, input_length,
        actual_hash, sizeof(actual_hash),
        &actual_hash_length);
    if (status != PSA_SUCCESS) {
        goto exit;
    }
    if (actual_hash_length != hash_length) {
        status = PSA_ERROR_INVALID_SIGNATURE;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(hash_external, hash_length, hash);
    if (mbedtls_ct_memcmp(hash, actual_hash, actual_hash_length) != 0) {
        status = PSA_ERROR_INVALID_SIGNATURE;
    }

exit:
    mbedtls_platform_zeroize(actual_hash, sizeof(actual_hash));

    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_INPUT_FREE(hash_external, hash);

    return status;
}

psa_status_t psa_hash_clone(const psa_hash_operation_t *source_operation,
                            psa_hash_operation_t *target_operation)
{
    if (source_operation->id == 0 ||
        target_operation->id != 0) {
        return PSA_ERROR_BAD_STATE;
    }

                                                                      
                                                                         
                                                                   
                                                                     
                             
    memset(&target_operation->ctx, 0, sizeof(target_operation->ctx));

    psa_status_t status = psa_driver_wrapper_hash_clone(source_operation,
                                                        target_operation);
    if (status != PSA_SUCCESS) {
        psa_hash_abort(target_operation);
    }

    return status;
}


                                                                  
         
                                                                  

psa_status_t psa_mac_abort(psa_mac_operation_t *operation)
{
                                                    
    if (operation->id == 0) {
        return PSA_SUCCESS;
    }

    psa_status_t status = psa_driver_wrapper_mac_abort(operation);
    operation->mac_size = 0;
    operation->is_sign = 0;
    operation->id = 0;

    return status;
}

static psa_status_t psa_mac_finalize_alg_and_key_validation(
    psa_algorithm_t alg,
    const psa_key_attributes_t *attributes,
    uint8_t *mac_size)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_type_t key_type = psa_get_key_type(attributes);
    size_t key_bits = psa_get_key_bits(attributes);

    if (!PSA_ALG_IS_MAC(alg)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

                                                            
    status = psa_mac_key_can_do(alg, key_type);
    if (status != PSA_SUCCESS) {
        return status;
    }

                                                                     
    *mac_size = PSA_MAC_LENGTH(key_type, key_bits, alg);

    if (*mac_size < 4) {
                                                                     
                                                                     
                                                                    
                                     
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (*mac_size > PSA_MAC_LENGTH(key_type, key_bits,
                                   PSA_ALG_FULL_LENGTH_MAC(alg))) {
                                                                                
                               
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (*mac_size > PSA_MAC_MAX_SIZE) {
                                                                             
                                                                             
                                                                         
                                                                      
                                                                       
                                                                   
                                                                                 
                                                                                
                                             
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

static psa_status_t psa_mac_setup(psa_mac_operation_t *operation,
                                  mbedtls_svc_key_id_t key,
                                  psa_algorithm_t alg,
                                  int is_sign)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot = NULL;

                                                                        
    if (operation->id != 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

                                                                      
                                                                         
                                                                   
                                                                     
                             
    memset(&operation->ctx, 0, sizeof(operation->ctx));

    status = psa_get_and_lock_key_slot_with_policy(
        key,
        &slot,
        is_sign ? PSA_KEY_USAGE_SIGN_MESSAGE : PSA_KEY_USAGE_VERIFY_MESSAGE,
        alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_mac_finalize_alg_and_key_validation(alg, &slot->attr,
                                                     &operation->mac_size);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    operation->is_sign = is_sign;
                                                          
    if (is_sign) {
        status = psa_driver_wrapper_mac_sign_setup(operation,
                                                   &slot->attr,
                                                   slot->key.data,
                                                   slot->key.bytes,
                                                   alg);
    } else {
        status = psa_driver_wrapper_mac_verify_setup(operation,
                                                     &slot->attr,
                                                     slot->key.data,
                                                     slot->key.bytes,
                                                     alg);
    }

exit:
    if (status != PSA_SUCCESS) {
        psa_mac_abort(operation);
    }

    unlock_status = psa_unregister_read_under_mutex(slot);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}

psa_status_t psa_mac_sign_setup(psa_mac_operation_t *operation,
                                mbedtls_svc_key_id_t key,
                                psa_algorithm_t alg)
{
    return psa_mac_setup(operation, key, alg, 1);
}

psa_status_t psa_mac_verify_setup(psa_mac_operation_t *operation,
                                  mbedtls_svc_key_id_t key,
                                  psa_algorithm_t alg)
{
    return psa_mac_setup(operation, key, alg, 0);
}

psa_status_t psa_mac_update(psa_mac_operation_t *operation,
                            const uint8_t *input_external,
                            size_t input_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(input_external, input);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        return status;
    }

                                                                  
                                                               
    if (input_length == 0) {
        status = PSA_SUCCESS;
        return status;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    status = psa_driver_wrapper_mac_update(operation, input, input_length);

    if (status != PSA_SUCCESS) {
        psa_mac_abort(operation);
    }

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_INPUT_FREE(input_external, input);

    return status;
}

psa_status_t psa_mac_sign_finish(psa_mac_operation_t *operation,
                                 uint8_t *mac_external,
                                 size_t mac_size,
                                 size_t *mac_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t abort_status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_OUTPUT_DECLARE(mac_external, mac);
    LOCAL_OUTPUT_ALLOC(mac_external, mac_size, mac);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (!operation->is_sign) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

                                                                                
                                             
    if (operation->mac_size == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (mac_size < operation->mac_size) {
        status = PSA_ERROR_BUFFER_TOO_SMALL;
        goto exit;
    }


    status = psa_driver_wrapper_mac_sign_finish(operation,
                                                mac, operation->mac_size,
                                                mac_length);

exit:
                                                                             
                                                                      
                                                                             
                                                                              
                           
       
    if (status != PSA_SUCCESS) {
        *mac_length = mac_size;
        operation->mac_size = 0;
    }

    if (mac != NULL) {
        psa_wipe_tag_output_buffer(mac, status, mac_size, *mac_length);
    }

    abort_status = psa_mac_abort(operation);
    LOCAL_OUTPUT_FREE(mac_external, mac);

    return status == PSA_SUCCESS ? abort_status : status;
}

psa_status_t psa_mac_verify_finish(psa_mac_operation_t *operation,
                                   const uint8_t *mac_external,
                                   size_t mac_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t abort_status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(mac_external, mac);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->is_sign) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->mac_size != mac_length) {
        status = PSA_ERROR_INVALID_SIGNATURE;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(mac_external, mac_length, mac);
    status = psa_driver_wrapper_mac_verify_finish(operation,
                                                  mac, mac_length);

exit:
    abort_status = psa_mac_abort(operation);
    LOCAL_INPUT_FREE(mac_external, mac);

    return status == PSA_SUCCESS ? abort_status : status;
}

static psa_status_t psa_mac_compute_internal(mbedtls_svc_key_id_t key,
                                             psa_algorithm_t alg,
                                             const uint8_t *input,
                                             size_t input_length,
                                             uint8_t *mac,
                                             size_t mac_size,
                                             size_t *mac_length,
                                             int is_sign)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;
    uint8_t operation_mac_size = 0;

    status = psa_get_and_lock_key_slot_with_policy(
        key,
        &slot,
        is_sign ? PSA_KEY_USAGE_SIGN_MESSAGE : PSA_KEY_USAGE_VERIFY_MESSAGE,
        alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_mac_finalize_alg_and_key_validation(alg, &slot->attr,
                                                     &operation_mac_size);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (mac_size < operation_mac_size) {
        status = PSA_ERROR_BUFFER_TOO_SMALL;
        goto exit;
    }

    status = psa_driver_wrapper_mac_compute(
        &slot->attr,
        slot->key.data, slot->key.bytes,
        alg,
        input, input_length,
        mac, operation_mac_size, mac_length);

exit:
                                                                             
                                                                      
                                                                             
                                                                              
                           
       
    if (status != PSA_SUCCESS) {
        *mac_length = mac_size;
        operation_mac_size = 0;
    }

    psa_wipe_tag_output_buffer(mac, status, mac_size, *mac_length);

    unlock_status = psa_unregister_read_under_mutex(slot);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}

psa_status_t psa_mac_compute(mbedtls_svc_key_id_t key,
                             psa_algorithm_t alg,
                             const uint8_t *input_external,
                             size_t input_length,
                             uint8_t *mac_external,
                             size_t mac_size,
                             size_t *mac_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_OUTPUT_DECLARE(mac_external, mac);

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_OUTPUT_ALLOC(mac_external, mac_size, mac);
    status = psa_mac_compute_internal(key, alg,
                                      input, input_length,
                                      mac, mac_size, mac_length, 1);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_OUTPUT_FREE(mac_external, mac);

    return status;
}

psa_status_t psa_mac_verify(mbedtls_svc_key_id_t key,
                            psa_algorithm_t alg,
                            const uint8_t *input_external,
                            size_t input_length,
                            const uint8_t *mac_external,
                            size_t mac_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    uint8_t actual_mac[PSA_MAC_MAX_SIZE];
    size_t actual_mac_length;
    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_INPUT_DECLARE(mac_external, mac);

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    status = psa_mac_compute_internal(key, alg,
                                      input, input_length,
                                      actual_mac, sizeof(actual_mac),
                                      &actual_mac_length, 0);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (mac_length != actual_mac_length) {
        status = PSA_ERROR_INVALID_SIGNATURE;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(mac_external, mac_length, mac);
    if (mbedtls_ct_memcmp(mac, actual_mac, actual_mac_length) != 0) {
        status = PSA_ERROR_INVALID_SIGNATURE;
        goto exit;
    }

exit:
    mbedtls_platform_zeroize(actual_mac, sizeof(actual_mac));
    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_INPUT_FREE(mac_external, mac);

    return status;
}

                                                                  
                             
                                                                  

static psa_status_t psa_sign_verify_check_alg(int input_is_message,
                                              psa_algorithm_t alg)
{
    if (input_is_message) {
        if (!PSA_ALG_IS_SIGN_MESSAGE(alg)) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    psa_algorithm_t hash_alg = 0;
    if (PSA_ALG_IS_SIGN_HASH(alg)) {
        hash_alg = PSA_ALG_SIGN_GET_HASH(alg);
    }

                                                            
                                                                      
                                                                
                                  

    if (hash_alg == 0 && input_is_message) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }
    if (hash_alg == PSA_ALG_ANY_HASH) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }
                                                                 
                          
                                                            
                                                                 
                                                                   
                                                                   
                                                                 
                                                               
                                                                     
                                                                   
                                                    
                                                                    
                                                                     
                                                                   
                                               
                                                               
                                                  
       
    if (hash_alg != 0 && !is_hash_supported(hash_alg)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

static psa_status_t psa_sign_internal(mbedtls_svc_key_id_t key,
                                      int input_is_message,
                                      psa_algorithm_t alg,
                                      const uint8_t *input,
                                      size_t input_length,
                                      uint8_t *signature,
                                      size_t signature_size,
                                      size_t *signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    *signature_length = 0;

    status = psa_sign_verify_check_alg(input_is_message, alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

                                                                         
                                                                            
                                                                      
                        
    if (signature_size == 0) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }

    status = psa_get_and_lock_key_slot_with_policy(
        key, &slot,
        input_is_message ? PSA_KEY_USAGE_SIGN_MESSAGE :
        PSA_KEY_USAGE_SIGN_HASH,
        alg);

    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (!PSA_KEY_TYPE_IS_KEY_PAIR(slot->attr.type)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    if (input_is_message) {
        status = psa_driver_wrapper_sign_message(
            &slot->attr, slot->key.data, slot->key.bytes,
            alg, input, input_length,
            signature, signature_size, signature_length);
    } else {

        status = psa_driver_wrapper_sign_hash(
            &slot->attr, slot->key.data, slot->key.bytes,
            alg, input, input_length,
            signature, signature_size, signature_length);
    }


exit:
    psa_wipe_tag_output_buffer(signature, status, signature_size,
                               *signature_length);

    unlock_status = psa_unregister_read_under_mutex(slot);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}

static psa_status_t psa_verify_internal(mbedtls_svc_key_id_t key,
                                        int input_is_message,
                                        psa_algorithm_t alg,
                                        const uint8_t *input,
                                        size_t input_length,
                                        const uint8_t *signature,
                                        size_t signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    status = psa_sign_verify_check_alg(input_is_message, alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_get_and_lock_key_slot_with_policy(
        key, &slot,
        input_is_message ? PSA_KEY_USAGE_VERIFY_MESSAGE :
        PSA_KEY_USAGE_VERIFY_HASH,
        alg);

    if (status != PSA_SUCCESS) {
        return status;
    }

    if (input_is_message) {
        status = psa_driver_wrapper_verify_message(
            &slot->attr, slot->key.data, slot->key.bytes,
            alg, input, input_length,
            signature, signature_length);
    } else {
        status = psa_driver_wrapper_verify_hash(
            &slot->attr, slot->key.data, slot->key.bytes,
            alg, input, input_length,
            signature, signature_length);
    }

    unlock_status = psa_unregister_read_under_mutex(slot);

    return (status == PSA_SUCCESS) ? unlock_status : status;

}

psa_status_t psa_sign_message_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *input,
    size_t input_length,
    uint8_t *signature,
    size_t signature_size,
    size_t *signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (PSA_ALG_IS_SIGN_HASH(alg)) {
        size_t hash_length;
        uint8_t hash[PSA_HASH_MAX_SIZE];

        status = psa_driver_wrapper_hash_compute(
            PSA_ALG_SIGN_GET_HASH(alg),
            input, input_length,
            hash, sizeof(hash), &hash_length);

        if (status != PSA_SUCCESS) {
            return status;
        }

        return psa_driver_wrapper_sign_hash(
            attributes, key_buffer, key_buffer_size,
            alg, hash, hash_length,
            signature, signature_size, signature_length);
    }

    return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_sign_message(mbedtls_svc_key_id_t key,
                              psa_algorithm_t alg,
                              const uint8_t *input_external,
                              size_t input_length,
                              uint8_t *signature_external,
                              size_t signature_size,
                              size_t *signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_OUTPUT_DECLARE(signature_external, signature);

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_OUTPUT_ALLOC(signature_external, signature_size, signature);
    status = psa_sign_internal(key, 1, alg, input, input_length, signature,
                               signature_size, signature_length);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_OUTPUT_FREE(signature_external, signature);
    return status;
}

psa_status_t psa_verify_message_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *input,
    size_t input_length,
    const uint8_t *signature,
    size_t signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (PSA_ALG_IS_SIGN_HASH(alg)) {
        size_t hash_length;
        uint8_t hash[PSA_HASH_MAX_SIZE];

        status = psa_driver_wrapper_hash_compute(
            PSA_ALG_SIGN_GET_HASH(alg),
            input, input_length,
            hash, sizeof(hash), &hash_length);

        if (status != PSA_SUCCESS) {
            return status;
        }

        return psa_driver_wrapper_verify_hash(
            attributes, key_buffer, key_buffer_size,
            alg, hash, hash_length,
            signature, signature_length);
    }

    return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_verify_message(mbedtls_svc_key_id_t key,
                                psa_algorithm_t alg,
                                const uint8_t *input_external,
                                size_t input_length,
                                const uint8_t *signature_external,
                                size_t signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_INPUT_DECLARE(signature_external, signature);

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_INPUT_ALLOC(signature_external, signature_length, signature);
    status = psa_verify_internal(key, 1, alg, input, input_length, signature,
                                 signature_length);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_INPUT_FREE(signature_external, signature);

    return status;
}

psa_status_t psa_sign_hash_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
    uint8_t *signature, size_t signature_size, size_t *signature_length)
{
    if (attributes->type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
        if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) ||
            PSA_ALG_IS_RSA_PSS(alg)) {
#if defined(MBEDTLS_PSA_BUILTIN_ALG_RSA_PKCS1V15_SIGN) || \
            defined(MBEDTLS_PSA_BUILTIN_ALG_RSA_PSS)
            return mbedtls_psa_rsa_sign_hash(
                attributes,
                key_buffer, key_buffer_size,
                alg, hash, hash_length,
                signature, signature_size, signature_length);
#endif                                                        \
                                                     
        } else {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    } else if (PSA_KEY_TYPE_IS_ECC(attributes->type)) {
        if (PSA_ALG_IS_ECDSA(alg)) {
#if defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
            defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)
            return mbedtls_psa_ecdsa_sign_hash(
                attributes,
                key_buffer, key_buffer_size,
                alg, hash, hash_length,
                signature, signature_size, signature_length);
#endif                                            \
                                                                 
        } else {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    (void) key_buffer;
    (void) key_buffer_size;
    (void) hash;
    (void) hash_length;
    (void) signature;
    (void) signature_size;
    (void) signature_length;

    return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_sign_hash(mbedtls_svc_key_id_t key,
                           psa_algorithm_t alg,
                           const uint8_t *hash_external,
                           size_t hash_length,
                           uint8_t *signature_external,
                           size_t signature_size,
                           size_t *signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(hash_external, hash);
    LOCAL_OUTPUT_DECLARE(signature_external, signature);

    LOCAL_INPUT_ALLOC(hash_external, hash_length, hash);
    LOCAL_OUTPUT_ALLOC(signature_external, signature_size, signature);
    status = psa_sign_internal(key, 0, alg, hash, hash_length, signature,
                               signature_size, signature_length);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_INPUT_FREE(hash_external, hash);
    LOCAL_OUTPUT_FREE(signature_external, signature);

    return status;
}

psa_status_t psa_verify_hash_builtin(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length)
{
    if (PSA_KEY_TYPE_IS_RSA(attributes->type)) {
        if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) ||
            PSA_ALG_IS_RSA_PSS(alg)) {
#if defined(MBEDTLS_PSA_BUILTIN_ALG_RSA_PKCS1V15_SIGN) || \
            defined(MBEDTLS_PSA_BUILTIN_ALG_RSA_PSS)
            return mbedtls_psa_rsa_verify_hash(
                attributes,
                key_buffer, key_buffer_size,
                alg, hash, hash_length,
                signature, signature_length);
#endif                                                        \
                                                     
        } else {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    } else if (PSA_KEY_TYPE_IS_ECC(attributes->type)) {
        if (PSA_ALG_IS_ECDSA(alg)) {
#if defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
            defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)
            return mbedtls_psa_ecdsa_verify_hash(
                attributes,
                key_buffer, key_buffer_size,
                alg, hash, hash_length,
                signature, signature_length);
#endif                                            \
                                                                 
        } else {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    (void) key_buffer;
    (void) key_buffer_size;
    (void) hash;
    (void) hash_length;
    (void) signature;
    (void) signature_length;

    return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_verify_hash(mbedtls_svc_key_id_t key,
                             psa_algorithm_t alg,
                             const uint8_t *hash_external,
                             size_t hash_length,
                             const uint8_t *signature_external,
                             size_t signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(hash_external, hash);
    LOCAL_INPUT_DECLARE(signature_external, signature);

    LOCAL_INPUT_ALLOC(hash_external, hash_length, hash);
    LOCAL_INPUT_ALLOC(signature_external, signature_length, signature);
    status = psa_verify_internal(key, 0, alg, hash, hash_length, signature,
                                 signature_length);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_INPUT_FREE(hash_external, hash);
    LOCAL_INPUT_FREE(signature_external, signature);

    return status;
}

psa_status_t psa_asymmetric_encrypt(mbedtls_svc_key_id_t key,
                                    psa_algorithm_t alg,
                                    const uint8_t *input_external,
                                    size_t input_length,
                                    const uint8_t *salt_external,
                                    size_t salt_length,
                                    uint8_t *output_external,
                                    size_t output_size,
                                    size_t *output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_INPUT_DECLARE(salt_external, salt);
    LOCAL_OUTPUT_DECLARE(output_external, output);

    (void) input;
    (void) input_length;
    (void) salt;
    (void) output;
    (void) output_size;

    *output_length = 0;

    if (!PSA_ALG_IS_RSA_OAEP(alg) && salt_length != 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    status = psa_get_and_lock_key_slot_with_policy(
        key, &slot, PSA_KEY_USAGE_ENCRYPT, alg);
    if (status != PSA_SUCCESS) {
        return status;
    }
    if (!(PSA_KEY_TYPE_IS_PUBLIC_KEY(slot->attr.type) ||
          PSA_KEY_TYPE_IS_KEY_PAIR(slot->attr.type))) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_INPUT_ALLOC(salt_external, salt_length, salt);
    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    status = psa_driver_wrapper_asymmetric_encrypt(
        &slot->attr, slot->key.data, slot->key.bytes,
        alg, input, input_length, salt, salt_length,
        output, output_size, output_length);
exit:
    unlock_status = psa_unregister_read_under_mutex(slot);

    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_INPUT_FREE(salt_external, salt);
    LOCAL_OUTPUT_FREE(output_external, output);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}

psa_status_t psa_asymmetric_decrypt(mbedtls_svc_key_id_t key,
                                    psa_algorithm_t alg,
                                    const uint8_t *input_external,
                                    size_t input_length,
                                    const uint8_t *salt_external,
                                    size_t salt_length,
                                    uint8_t *output_external,
                                    size_t output_size,
                                    size_t *output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_INPUT_DECLARE(salt_external, salt);
    LOCAL_OUTPUT_DECLARE(output_external, output);

    (void) input;
    (void) input_length;
    (void) salt;
    (void) output;
    (void) output_size;

    *output_length = 0;

    if (!PSA_ALG_IS_RSA_OAEP(alg) && salt_length != 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    status = psa_get_and_lock_key_slot_with_policy(
        key, &slot, PSA_KEY_USAGE_DECRYPT, alg);
    if (status != PSA_SUCCESS) {
        return status;
    }
    if (!PSA_KEY_TYPE_IS_KEY_PAIR(slot->attr.type)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_INPUT_ALLOC(salt_external, salt_length, salt);
    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    status = psa_driver_wrapper_asymmetric_decrypt(
        &slot->attr, slot->key.data, slot->key.bytes,
        alg, input, input_length, salt, salt_length,
        output, output_size, output_length);

exit:
    unlock_status = psa_unregister_read_under_mutex(slot);

    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_INPUT_FREE(salt_external, salt);
    LOCAL_OUTPUT_FREE(output_external, output);

                                                        
                                              
                                                                       
                                                                              
                                                                               
    return (unlock_status != PSA_SUCCESS) ? unlock_status : status;
}

                                                                  
                                                                  
                                                                  

static uint32_t psa_interruptible_max_ops = PSA_INTERRUPTIBLE_MAX_OPS_UNLIMITED;

void psa_interruptible_set_max_ops(uint32_t max_ops)
{
    psa_interruptible_max_ops = max_ops;
}

uint32_t psa_interruptible_get_max_ops(void)
{
    return psa_interruptible_max_ops;
}

uint32_t psa_sign_hash_get_num_ops(
    const psa_sign_hash_interruptible_operation_t *operation)
{
    return operation->num_ops;
}

uint32_t psa_verify_hash_get_num_ops(
    const psa_verify_hash_interruptible_operation_t *operation)
{
    return operation->num_ops;
}

static psa_status_t psa_sign_hash_abort_internal(
    psa_sign_hash_interruptible_operation_t *operation)
{
    if (operation->id == 0) {
                                                                           
                                                                       
                            
        return PSA_SUCCESS;
    }

    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    status = psa_driver_wrapper_sign_hash_abort(operation);

    operation->id = 0;

                                                                              
                                                                             
                                                     

    return status;
}

psa_status_t psa_sign_hash_start(
    psa_sign_hash_interruptible_operation_t *operation,
    mbedtls_svc_key_id_t key, psa_algorithm_t alg,
    const uint8_t *hash_external, size_t hash_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    LOCAL_INPUT_DECLARE(hash_external, hash);

                                                                            
                             
    if (operation->id != 0 || operation->error_occurred) {
        return PSA_ERROR_BAD_STATE;
    }

                                                                      
                                                                         
                                                                   
                                                                     
                             
    memset(&operation->ctx, 0, sizeof(operation->ctx));

    status = psa_sign_verify_check_alg(0, alg);
    if (status != PSA_SUCCESS) {
        operation->error_occurred = 1;
        return status;
    }

    status = psa_get_and_lock_key_slot_with_policy(key, &slot,
                                                   PSA_KEY_USAGE_SIGN_HASH,
                                                   alg);

    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (!PSA_KEY_TYPE_IS_KEY_PAIR(slot->attr.type)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(hash_external, hash_length, hash);

                                                                   
    operation->num_ops = 0;

    status = psa_driver_wrapper_sign_hash_start(operation, &slot->attr,
                                                slot->key.data,
                                                slot->key.bytes, alg,
                                                hash, hash_length);
exit:

    if (status != PSA_SUCCESS) {
        operation->error_occurred = 1;
        psa_sign_hash_abort_internal(operation);
    }

    unlock_status = psa_unregister_read_under_mutex(slot);

    if (unlock_status != PSA_SUCCESS) {
        operation->error_occurred = 1;
    }

    LOCAL_INPUT_FREE(hash_external, hash);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}


psa_status_t psa_sign_hash_complete(
    psa_sign_hash_interruptible_operation_t *operation,
    uint8_t *signature_external, size_t signature_size,
    size_t *signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    LOCAL_OUTPUT_DECLARE(signature_external, signature);

    *signature_length = 0;

                                                                         
                             
    if (operation->id == 0 || operation->error_occurred) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

                                                                              
                                            
    if (signature_size == 0) {
        status = PSA_ERROR_BUFFER_TOO_SMALL;
        goto exit;
    }

    LOCAL_OUTPUT_ALLOC(signature_external, signature_size, signature);

    status = psa_driver_wrapper_sign_hash_complete(operation, signature,
                                                   signature_size,
                                                   signature_length);

                                          
    operation->num_ops = psa_driver_wrapper_sign_hash_get_num_ops(operation);

exit:

    if (signature != NULL) {
        psa_wipe_tag_output_buffer(signature, status, signature_size,
                                   *signature_length);
    }

    if (status != PSA_OPERATION_INCOMPLETE) {
        if (status != PSA_SUCCESS) {
            operation->error_occurred = 1;
        }

        psa_sign_hash_abort_internal(operation);
    }

    LOCAL_OUTPUT_FREE(signature_external, signature);

    return status;
}

psa_status_t psa_sign_hash_abort(
    psa_sign_hash_interruptible_operation_t *operation)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    status = psa_sign_hash_abort_internal(operation);

                                                                           
                                                                
    operation->num_ops = 0;

                                  
    operation->error_occurred = 0;

    return status;
}

static psa_status_t psa_verify_hash_abort_internal(
    psa_verify_hash_interruptible_operation_t *operation)
{
    if (operation->id == 0) {
                                                                           
                                                                       
                            
        return PSA_SUCCESS;
    }

    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    status = psa_driver_wrapper_verify_hash_abort(operation);

    operation->id = 0;

                                                                              
                                                                             
                                                     

    return status;
}

psa_status_t psa_verify_hash_start(
    psa_verify_hash_interruptible_operation_t *operation,
    mbedtls_svc_key_id_t key, psa_algorithm_t alg,
    const uint8_t *hash_external, size_t hash_length,
    const uint8_t *signature_external, size_t signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    LOCAL_INPUT_DECLARE(hash_external, hash);
    LOCAL_INPUT_DECLARE(signature_external, signature);

                                                                            
                             
    if (operation->id != 0 || operation->error_occurred) {
        return PSA_ERROR_BAD_STATE;
    }

                                                                      
                                                                         
                                                                   
                                                                     
                             
    memset(&operation->ctx, 0, sizeof(operation->ctx));

    status = psa_sign_verify_check_alg(0, alg);
    if (status != PSA_SUCCESS) {
        operation->error_occurred = 1;
        return status;
    }

    status = psa_get_and_lock_key_slot_with_policy(key, &slot,
                                                   PSA_KEY_USAGE_VERIFY_HASH,
                                                   alg);

    if (status != PSA_SUCCESS) {
        operation->error_occurred = 1;
        return status;
    }

    LOCAL_INPUT_ALLOC(hash_external, hash_length, hash);
    LOCAL_INPUT_ALLOC(signature_external, signature_length, signature);

                                                                   
    operation->num_ops = 0;

    status = psa_driver_wrapper_verify_hash_start(operation, &slot->attr,
                                                  slot->key.data,
                                                  slot->key.bytes,
                                                  alg, hash, hash_length,
                                                  signature, signature_length);
#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif

    if (status != PSA_SUCCESS) {
        operation->error_occurred = 1;
        psa_verify_hash_abort_internal(operation);
    }

    unlock_status = psa_unregister_read_under_mutex(slot);

    if (unlock_status != PSA_SUCCESS) {
        operation->error_occurred = 1;
    }

    LOCAL_INPUT_FREE(hash_external, hash);
    LOCAL_INPUT_FREE(signature_external, signature);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}

psa_status_t psa_verify_hash_complete(
    psa_verify_hash_interruptible_operation_t *operation)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

                                                                         
                             
    if (operation->id == 0 || operation->error_occurred) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    status = psa_driver_wrapper_verify_hash_complete(operation);

                                          
    operation->num_ops = psa_driver_wrapper_verify_hash_get_num_ops(
        operation);

exit:

    if (status != PSA_OPERATION_INCOMPLETE) {
        if (status != PSA_SUCCESS) {
            operation->error_occurred = 1;
        }

        psa_verify_hash_abort_internal(operation);
    }

    return status;
}

psa_status_t psa_verify_hash_abort(
    psa_verify_hash_interruptible_operation_t *operation)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    status = psa_verify_hash_abort_internal(operation);

                                                                           
                                                                
    operation->num_ops = 0;

                                  
    operation->error_occurred = 0;

    return status;
}

                                                                  
                                                                  
                                                                  
                                                                  

void mbedtls_psa_interruptible_set_max_ops(uint32_t max_ops)
{

#if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

                                                                             
                                                                  
    if (max_ops == 0) {
        max_ops = 1;
    }

    mbedtls_ecp_set_max_ops(max_ops);
#else
    (void) max_ops;
#endif                                            \
                                                                \
                                               
}

uint32_t mbedtls_psa_sign_hash_get_num_ops(
    const mbedtls_psa_sign_hash_interruptible_operation_t *operation)
{
#if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

    return operation->num_ops;
#else
    (void) operation;
    return 0;
#endif                                            \
                                                                \
                                               
}

uint32_t mbedtls_psa_verify_hash_get_num_ops(
    const mbedtls_psa_verify_hash_interruptible_operation_t *operation)
{
    #if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

    return operation->num_ops;
#else
    (void) operation;
    return 0;
#endif                                            \
                                                                \
                                               
}

                                                                   
                                                                       
                                                                    
                                                                        
                                                                         
                                                               
  
                                                                    
                                                                     
   
static inline int can_do_interruptible_sign_verify(psa_algorithm_t alg)
{
#if defined(MBEDTLS_ECP_RESTARTABLE)
#if defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)
    if (PSA_ALG_IS_DETERMINISTIC_ECDSA(alg)) {
        return 1;
    }
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA)
    if (PSA_ALG_IS_RANDOMIZED_ECDSA(alg)) {
        return 1;
    }
#endif
#endif                                       
    (void) alg;
    return 0;
}

psa_status_t mbedtls_psa_sign_hash_start(
    mbedtls_psa_sign_hash_interruptible_operation_t *operation,
    const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
    size_t key_buffer_size, psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    size_t required_hash_length;

    if (!PSA_KEY_TYPE_IS_ECC_KEY_PAIR(attributes->type)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }
    psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(attributes->type);
    if (!PSA_ECC_FAMILY_IS_WEIERSTRASS(curve)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (!can_do_interruptible_sign_verify(alg)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

#if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

    mbedtls_ecdsa_restart_init(&operation->restart_ctx);

                                                              
    operation->num_ops = 0;

    status = mbedtls_psa_ecp_load_representation(attributes->type,
                                                 attributes->bits,
                                                 key_buffer,
                                                 key_buffer_size,
                                                 &operation->ctx);

    if (status != PSA_SUCCESS) {
        return status;
    }

    operation->coordinate_bytes = PSA_BITS_TO_BYTES(
        operation->ctx->grp.nbits);

    psa_algorithm_t hash_alg = PSA_ALG_SIGN_GET_HASH(alg);
    operation->md_alg = mbedtls_md_type_from_psa_alg(hash_alg);
    operation->alg = alg;

                                                                            
                                                                            
    required_hash_length = (hash_length < operation->coordinate_bytes ?
                            hash_length : operation->coordinate_bytes);

    if (required_hash_length > sizeof(operation->hash)) {
                                                           
        return PSA_ERROR_CORRUPTION_DETECTED;
    }

    memcpy(operation->hash, hash, required_hash_length);
    operation->hash_length = required_hash_length;

    return PSA_SUCCESS;

#else
    (void) operation;
    (void) key_buffer;
    (void) key_buffer_size;
    (void) alg;
    (void) hash;
    (void) hash_length;
    (void) status;
    (void) required_hash_length;

    return PSA_ERROR_NOT_SUPPORTED;
#endif                                            \
                                                                \
                                               
}

psa_status_t mbedtls_psa_sign_hash_complete(
    mbedtls_psa_sign_hash_interruptible_operation_t *operation,
    uint8_t *signature, size_t signature_size,
    size_t *signature_length)
{
#if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi r;
    mbedtls_mpi s;

    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

                                                                  
    mbedtls_psa_interruptible_set_max_ops(psa_interruptible_get_max_ops());

    if (signature_size < 2 * operation->coordinate_bytes) {
        status = PSA_ERROR_BUFFER_TOO_SMALL;
        goto exit;
    }

    if (PSA_ALG_ECDSA_IS_DETERMINISTIC(operation->alg)) {

#if defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)
        status = mbedtls_to_psa_error(
            mbedtls_ecdsa_sign_det_restartable(&operation->ctx->grp,
                                               &r,
                                               &s,
                                               &operation->ctx->d,
                                               operation->hash,
                                               operation->hash_length,
                                               operation->md_alg,
                                               mbedtls_psa_get_random,
                                               MBEDTLS_PSA_RANDOM_STATE,
                                               &operation->restart_ctx));
#else                                                           
        status = PSA_ERROR_NOT_SUPPORTED;
        goto exit;
#endif                                                           
    } else {
        status = mbedtls_to_psa_error(
            mbedtls_ecdsa_sign_restartable(&operation->ctx->grp,
                                           &r,
                                           &s,
                                           &operation->ctx->d,
                                           operation->hash,
                                           operation->hash_length,
                                           mbedtls_psa_get_random,
                                           MBEDTLS_PSA_RANDOM_STATE,
                                           mbedtls_psa_get_random,
                                           MBEDTLS_PSA_RANDOM_STATE,
                                           &operation->restart_ctx));
    }

                                                                             
                                                                    
    operation->num_ops += operation->restart_ctx.ecp.ops_done;

    if (status == PSA_SUCCESS) {
        status =  mbedtls_to_psa_error(
            mbedtls_mpi_write_binary(&r,
                                     signature,
                                     operation->coordinate_bytes)
            );

        if (status != PSA_SUCCESS) {
            goto exit;
        }

        status =  mbedtls_to_psa_error(
            mbedtls_mpi_write_binary(&s,
                                     signature +
                                     operation->coordinate_bytes,
                                     operation->coordinate_bytes)
            );

        if (status != PSA_SUCCESS) {
            goto exit;
        }

        *signature_length = operation->coordinate_bytes * 2;

        status = PSA_SUCCESS;
    }

exit:

    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    return status;

 #else

    (void) operation;
    (void) signature;
    (void) signature_size;
    (void) signature_length;

    return PSA_ERROR_NOT_SUPPORTED;

#endif                                            \
                                                                \
                                               
}

psa_status_t mbedtls_psa_sign_hash_abort(
    mbedtls_psa_sign_hash_interruptible_operation_t *operation)
{

#if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

    if (operation->ctx) {
        mbedtls_ecdsa_free(operation->ctx);
        mbedtls_free(operation->ctx);
        operation->ctx = NULL;
    }

    mbedtls_ecdsa_restart_free(&operation->restart_ctx);

    operation->num_ops = 0;

    return PSA_SUCCESS;

#else

    (void) operation;

    return PSA_ERROR_NOT_SUPPORTED;

#endif                                            \
                                                                \
                                               
}

psa_status_t mbedtls_psa_verify_hash_start(
    mbedtls_psa_verify_hash_interruptible_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    size_t coordinate_bytes = 0;
    size_t required_hash_length = 0;

    if (!PSA_KEY_TYPE_IS_ECC(attributes->type)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }
    psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(attributes->type);
    if (!PSA_ECC_FAMILY_IS_WEIERSTRASS(curve)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (!can_do_interruptible_sign_verify(alg)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

#if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

    mbedtls_ecdsa_restart_init(&operation->restart_ctx);
    mbedtls_mpi_init(&operation->r);
    mbedtls_mpi_init(&operation->s);

                                                              
    operation->num_ops = 0;

    status = mbedtls_psa_ecp_load_representation(attributes->type,
                                                 attributes->bits,
                                                 key_buffer,
                                                 key_buffer_size,
                                                 &operation->ctx);

    if (status != PSA_SUCCESS) {
        return status;
    }

    coordinate_bytes = PSA_BITS_TO_BYTES(operation->ctx->grp.nbits);

    if (signature_length != 2 * coordinate_bytes) {
        return PSA_ERROR_INVALID_SIGNATURE;
    }

    status = mbedtls_to_psa_error(
        mbedtls_mpi_read_binary(&operation->r,
                                signature,
                                coordinate_bytes));

    if (status != PSA_SUCCESS) {
        return status;
    }

    status = mbedtls_to_psa_error(
        mbedtls_mpi_read_binary(&operation->s,
                                signature +
                                coordinate_bytes,
                                coordinate_bytes));

    if (status != PSA_SUCCESS) {
        return status;
    }

    status = mbedtls_psa_ecp_load_public_part(operation->ctx);

    if (status != PSA_SUCCESS) {
        return status;
    }

                                                                            
                                                                            
    required_hash_length = (hash_length < coordinate_bytes ? hash_length :
                            coordinate_bytes);

    if (required_hash_length > sizeof(operation->hash)) {
                                                           
        return PSA_ERROR_CORRUPTION_DETECTED;
    }

    memcpy(operation->hash, hash, required_hash_length);
    operation->hash_length = required_hash_length;

    return PSA_SUCCESS;
#else
    (void) operation;
    (void) key_buffer;
    (void) key_buffer_size;
    (void) alg;
    (void) hash;
    (void) hash_length;
    (void) signature;
    (void) signature_length;
    (void) status;
    (void) coordinate_bytes;
    (void) required_hash_length;

    return PSA_ERROR_NOT_SUPPORTED;
#endif                                            \
                                                                \
                                               
}

psa_status_t mbedtls_psa_verify_hash_complete(
    mbedtls_psa_verify_hash_interruptible_operation_t *operation)
{

#if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

                                                                  
    mbedtls_psa_interruptible_set_max_ops(psa_interruptible_get_max_ops());

    status = mbedtls_to_psa_error(
        mbedtls_ecdsa_verify_restartable(&operation->ctx->grp,
                                         operation->hash,
                                         operation->hash_length,
                                         &operation->ctx->Q,
                                         &operation->r,
                                         &operation->s,
                                         &operation->restart_ctx));

                                                                             
                                                                    
    operation->num_ops += operation->restart_ctx.ecp.ops_done;

    return status;
#else
    (void) operation;

    return PSA_ERROR_NOT_SUPPORTED;

#endif                                            \
                                                                \
                                               
}

psa_status_t mbedtls_psa_verify_hash_abort(
    mbedtls_psa_verify_hash_interruptible_operation_t *operation)
{

#if (defined(MBEDTLS_PSA_BUILTIN_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA)) && \
    defined(MBEDTLS_ECP_RESTARTABLE)

    if (operation->ctx) {
        mbedtls_ecdsa_free(operation->ctx);
        mbedtls_free(operation->ctx);
        operation->ctx = NULL;
    }

    mbedtls_ecdsa_restart_free(&operation->restart_ctx);

    operation->num_ops = 0;

    mbedtls_mpi_free(&operation->r);
    mbedtls_mpi_free(&operation->s);

    return PSA_SUCCESS;

#else
    (void) operation;

    return PSA_ERROR_NOT_SUPPORTED;

#endif                                            \
                                                                \
                                               
}

static psa_status_t psa_generate_random_internal(uint8_t *output,
                                                 size_t output_size)
{
    GUARD_MODULE_INITIALIZED;

#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)

    psa_status_t status;
    size_t output_length = 0;
    status = mbedtls_psa_external_get_random(&global_data.rng,
                                             output, output_size,
                                             &output_length);
    if (status != PSA_SUCCESS) {
        return status;
    }
                                                                           
                                         
    if (output_length != output_size) {
        return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }
    return PSA_SUCCESS;

#else                                      
    return psa_random_internal_generate(&global_data.rng,
                                        output, output_size);
#endif                                      
}


                                                                  
                            
                                                                  

static psa_status_t psa_cipher_setup(psa_cipher_operation_t *operation,
                                     mbedtls_svc_key_id_t key,
                                     psa_algorithm_t alg,
                                     mbedtls_operation_t cipher_operation)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot = NULL;
    psa_key_usage_t usage = (cipher_operation == MBEDTLS_ENCRYPT ?
                             PSA_KEY_USAGE_ENCRYPT :
                             PSA_KEY_USAGE_DECRYPT);

                                                                        
    if (operation->id != 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (!PSA_ALG_IS_CIPHER(alg)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    status = psa_get_and_lock_key_slot_with_policy(key, &slot, usage, alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

                                                                            
                                                                                
                                                                          
                                
    operation->iv_set = 0;
    if (alg == PSA_ALG_ECB_NO_PADDING) {
        operation->iv_required = 0;
    } else {
        operation->iv_required = 1;
    }
    operation->default_iv_length = PSA_CIPHER_IV_LENGTH(slot->attr.type, alg);


                                                                      
                                                                         
                                                                   
                                                                     
                             
    memset(&operation->ctx, 0, sizeof(operation->ctx));

                                                                                  
    if (cipher_operation == MBEDTLS_ENCRYPT) {
        status = psa_driver_wrapper_cipher_encrypt_setup(operation,
                                                         &slot->attr,
                                                         slot->key.data,
                                                         slot->key.bytes,
                                                         alg);
    } else {
        status = psa_driver_wrapper_cipher_decrypt_setup(operation,
                                                         &slot->attr,
                                                         slot->key.data,
                                                         slot->key.bytes,
                                                         alg);
    }

exit:
    if (status != PSA_SUCCESS) {
        psa_cipher_abort(operation);
    }

    unlock_status = psa_unregister_read_under_mutex(slot);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}

psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation,
                                      mbedtls_svc_key_id_t key,
                                      psa_algorithm_t alg)
{
    return psa_cipher_setup(operation, key, alg, MBEDTLS_ENCRYPT);
}

psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation,
                                      mbedtls_svc_key_id_t key,
                                      psa_algorithm_t alg)
{
    return psa_cipher_setup(operation, key, alg, MBEDTLS_DECRYPT);
}

psa_status_t psa_cipher_generate_iv(psa_cipher_operation_t *operation,
                                    uint8_t *iv_external,
                                    size_t iv_size,
                                    size_t *iv_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    size_t default_iv_length = 0;

    LOCAL_OUTPUT_DECLARE(iv_external, iv);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->iv_set || !operation->iv_required) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    default_iv_length = operation->default_iv_length;
    if (iv_size < default_iv_length) {
        status = PSA_ERROR_BUFFER_TOO_SMALL;
        goto exit;
    }

    if (default_iv_length > PSA_CIPHER_IV_MAX_SIZE) {
        status = PSA_ERROR_GENERIC_ERROR;
        goto exit;
    }

    LOCAL_OUTPUT_ALLOC(iv_external, default_iv_length, iv);

    status = psa_generate_random_internal(iv, default_iv_length);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_driver_wrapper_cipher_set_iv(operation,
                                              iv, default_iv_length);

exit:
    if (status == PSA_SUCCESS) {
        *iv_length = default_iv_length;
        operation->iv_set = 1;
    } else {
        *iv_length = 0;
        psa_cipher_abort(operation);
        if (iv != NULL) {
            mbedtls_platform_zeroize(iv, default_iv_length);
        }
    }

    LOCAL_OUTPUT_FREE(iv_external, iv);
    return status;
}

psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation,
                               const uint8_t *iv_external,
                               size_t iv_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    LOCAL_INPUT_DECLARE(iv_external, iv);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->iv_set || !operation->iv_required) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (iv_length > PSA_CIPHER_IV_MAX_SIZE) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(iv_external, iv_length, iv);

    status = psa_driver_wrapper_cipher_set_iv(operation,
                                              iv,
                                              iv_length);

exit:
    if (status == PSA_SUCCESS) {
        operation->iv_set = 1;
    } else {
        psa_cipher_abort(operation);
    }

    LOCAL_INPUT_FREE(iv_external, iv);

    return status;
}

psa_status_t psa_cipher_update(psa_cipher_operation_t *operation,
                               const uint8_t *input_external,
                               size_t input_length,
                               uint8_t *output_external,
                               size_t output_size,
                               size_t *output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_OUTPUT_DECLARE(output_external, output);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->iv_required && !operation->iv_set) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    status = psa_driver_wrapper_cipher_update(operation,
                                              input,
                                              input_length,
                                              output,
                                              output_size,
                                              output_length);

exit:
    if (status != PSA_SUCCESS) {
        psa_cipher_abort(operation);
    }

    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_OUTPUT_FREE(output_external, output);

    return status;
}

psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation,
                               uint8_t *output_external,
                               size_t output_size,
                               size_t *output_length)
{
    psa_status_t status = PSA_ERROR_GENERIC_ERROR;

    LOCAL_OUTPUT_DECLARE(output_external, output);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->iv_required && !operation->iv_set) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    status = psa_driver_wrapper_cipher_finish(operation,
                                              output,
                                              output_size,
                                              output_length);

exit:
                                                           ;
    psa_status_t abort_status = psa_cipher_abort(operation);
                                                                     
                                                                           
                                                                          
                                                                     
                                                                       
                                                                       
                                                                        
    if (abort_status != PSA_SUCCESS) {
        status = abort_status;
    }

                                                                
                                                                
                                                                   
                                                                         
    mbedtls_ct_condition_t success =
        mbedtls_ct_bool_not(mbedtls_ct_bool(status));
    *output_length = mbedtls_ct_size_if_else_0(success, *output_length);

    LOCAL_OUTPUT_FREE(output_external, output);

    return status;
}

psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation)
{
    if (operation->id == 0) {
                                                                           
                                                                       
                            
        return PSA_SUCCESS;
    }

    psa_driver_wrapper_cipher_abort(operation);

    operation->id = 0;
    operation->iv_set = 0;
    operation->iv_required = 0;

    return PSA_SUCCESS;
}

psa_status_t psa_cipher_encrypt(mbedtls_svc_key_id_t key,
                                psa_algorithm_t alg,
                                const uint8_t *input_external,
                                size_t input_length,
                                uint8_t *output_external,
                                size_t output_size,
                                size_t *output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot = NULL;
    uint8_t local_iv[PSA_CIPHER_IV_MAX_SIZE];
    size_t default_iv_length = 0;

    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_OUTPUT_DECLARE(output_external, output);

    if (!PSA_ALG_IS_CIPHER(alg)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    status = psa_get_and_lock_key_slot_with_policy(key, &slot,
                                                   PSA_KEY_USAGE_ENCRYPT,
                                                   alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    default_iv_length = PSA_CIPHER_IV_LENGTH(slot->attr.type, alg);
    if (default_iv_length > PSA_CIPHER_IV_MAX_SIZE) {
        status = PSA_ERROR_GENERIC_ERROR;
        goto exit;
    }

    if (default_iv_length > 0) {
        if (output_size < default_iv_length) {
            status = PSA_ERROR_BUFFER_TOO_SMALL;
            goto exit;
        }

        status = psa_generate_random_internal(local_iv, default_iv_length);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    status = psa_driver_wrapper_cipher_encrypt(
        &slot->attr, slot->key.data, slot->key.bytes,
        alg, local_iv, default_iv_length, input, input_length,
        psa_crypto_buffer_offset(output, default_iv_length),
        output_size - default_iv_length, output_length);

exit:
    unlock_status = psa_unregister_read_under_mutex(slot);
    if (status == PSA_SUCCESS) {
        status = unlock_status;
    }

    if (status == PSA_SUCCESS) {
        if (default_iv_length > 0) {
            memcpy(output, local_iv, default_iv_length);
        }
        *output_length += default_iv_length;
    } else {
        *output_length = 0;
    }

    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_OUTPUT_FREE(output_external, output);

    return status;
}

psa_status_t psa_cipher_decrypt(mbedtls_svc_key_id_t key,
                                psa_algorithm_t alg,
                                const uint8_t *input_external,
                                size_t input_length,
                                uint8_t *output_external,
                                size_t output_size,
                                size_t *output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot = NULL;

    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_OUTPUT_DECLARE(output_external, output);

    if (!PSA_ALG_IS_CIPHER(alg)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    status = psa_get_and_lock_key_slot_with_policy(key, &slot,
                                                   PSA_KEY_USAGE_DECRYPT,
                                                   alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (input_length < PSA_CIPHER_IV_LENGTH(slot->attr.type, alg)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    status = psa_driver_wrapper_cipher_decrypt(
        &slot->attr, slot->key.data, slot->key.bytes,
        alg, input, input_length,
        output, output_size, output_length);

exit:
    unlock_status = psa_unregister_read_under_mutex(slot);
    if (unlock_status != PSA_SUCCESS) {
        status = unlock_status;
    }

                                                                
                                                                
                                                                   
                                                                         
    mbedtls_ct_condition_t success =
        mbedtls_ct_bool_not(mbedtls_ct_bool(status));
    *output_length = mbedtls_ct_size_if_else_0(success, *output_length);

    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_OUTPUT_FREE(output_external, output);

    return status;
}


                                                                  
          
                                                                  

                                                                  
static psa_algorithm_t psa_aead_get_base_algorithm(psa_algorithm_t alg)
{
    return PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg);
}

                                                            
static psa_status_t psa_aead_check_nonce_length(psa_algorithm_t alg,
                                                size_t nonce_length)
{
    psa_algorithm_t base_alg = psa_aead_get_base_algorithm(alg);

    switch (base_alg) {
#if defined(PSA_WANT_ALG_GCM)
        case PSA_ALG_GCM:
                                                                         
                                                                             
                                                                   
                                                                               
                                                                            
                                          
            if (nonce_length != 0) {
                return PSA_SUCCESS;
            }
            break;
#endif                       
#if defined(PSA_WANT_ALG_CCM)
        case PSA_ALG_CCM:
            if (nonce_length >= 7 && nonce_length <= 13) {
                return PSA_SUCCESS;
            }
            break;
#endif                       
#if defined(PSA_WANT_ALG_CHACHA20_POLY1305)
        case PSA_ALG_CHACHA20_POLY1305:
            if (nonce_length == 12) {
                return PSA_SUCCESS;
            } else if (nonce_length == 8) {
                return PSA_ERROR_NOT_SUPPORTED;
            }
            break;
#endif                                     
        default:
            (void) nonce_length;
            return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_ERROR_INVALID_ARGUMENT;
}

static psa_status_t psa_aead_check_algorithm(psa_algorithm_t alg)
{
    if (!PSA_ALG_IS_AEAD(alg) || PSA_ALG_IS_WILDCARD(alg)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return PSA_SUCCESS;
}

psa_status_t psa_aead_encrypt(mbedtls_svc_key_id_t key,
                              psa_algorithm_t alg,
                              const uint8_t *nonce_external,
                              size_t nonce_length,
                              const uint8_t *additional_data_external,
                              size_t additional_data_length,
                              const uint8_t *plaintext_external,
                              size_t plaintext_length,
                              uint8_t *ciphertext_external,
                              size_t ciphertext_size,
                              size_t *ciphertext_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    LOCAL_INPUT_DECLARE(nonce_external, nonce);
    LOCAL_INPUT_DECLARE(additional_data_external, additional_data);
    LOCAL_INPUT_DECLARE(plaintext_external, plaintext);
    LOCAL_OUTPUT_DECLARE(ciphertext_external, ciphertext);

    *ciphertext_length = 0;

    status = psa_aead_check_algorithm(alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_get_and_lock_key_slot_with_policy(
        key, &slot, PSA_KEY_USAGE_ENCRYPT, alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

    LOCAL_INPUT_ALLOC(nonce_external, nonce_length, nonce);
    LOCAL_INPUT_ALLOC(additional_data_external, additional_data_length, additional_data);
    LOCAL_INPUT_ALLOC(plaintext_external, plaintext_length, plaintext);
    LOCAL_OUTPUT_ALLOC(ciphertext_external, ciphertext_size, ciphertext);

    status = psa_aead_check_nonce_length(alg, nonce_length);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_driver_wrapper_aead_encrypt(
        &slot->attr, slot->key.data, slot->key.bytes,
        alg,
        nonce, nonce_length,
        additional_data, additional_data_length,
        plaintext, plaintext_length,
        ciphertext, ciphertext_size, ciphertext_length);

    if (status != PSA_SUCCESS && ciphertext_size != 0) {
        memset(ciphertext, 0, ciphertext_size);
    }

exit:
    LOCAL_INPUT_FREE(nonce_external, nonce);
    LOCAL_INPUT_FREE(additional_data_external, additional_data);
    LOCAL_INPUT_FREE(plaintext_external, plaintext);
    LOCAL_OUTPUT_FREE(ciphertext_external, ciphertext);

    psa_unregister_read_under_mutex(slot);

    return status;
}

psa_status_t psa_aead_decrypt(mbedtls_svc_key_id_t key,
                              psa_algorithm_t alg,
                              const uint8_t *nonce_external,
                              size_t nonce_length,
                              const uint8_t *additional_data_external,
                              size_t additional_data_length,
                              const uint8_t *ciphertext_external,
                              size_t ciphertext_length,
                              uint8_t *plaintext_external,
                              size_t plaintext_size,
                              size_t *plaintext_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    LOCAL_INPUT_DECLARE(nonce_external, nonce);
    LOCAL_INPUT_DECLARE(additional_data_external, additional_data);
    LOCAL_INPUT_DECLARE(ciphertext_external, ciphertext);
    LOCAL_OUTPUT_DECLARE(plaintext_external, plaintext);

    *plaintext_length = 0;

    status = psa_aead_check_algorithm(alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_get_and_lock_key_slot_with_policy(
        key, &slot, PSA_KEY_USAGE_DECRYPT, alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

    LOCAL_INPUT_ALLOC(nonce_external, nonce_length, nonce);
    LOCAL_INPUT_ALLOC(additional_data_external, additional_data_length,
                      additional_data);
    LOCAL_INPUT_ALLOC(ciphertext_external, ciphertext_length, ciphertext);
    LOCAL_OUTPUT_ALLOC(plaintext_external, plaintext_size, plaintext);

    status = psa_aead_check_nonce_length(alg, nonce_length);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_driver_wrapper_aead_decrypt(
        &slot->attr, slot->key.data, slot->key.bytes,
        alg,
        nonce, nonce_length,
        additional_data, additional_data_length,
        ciphertext, ciphertext_length,
        plaintext, plaintext_size, plaintext_length);

    if (status != PSA_SUCCESS && plaintext_size != 0) {
        memset(plaintext, 0, plaintext_size);
    }

exit:
    LOCAL_INPUT_FREE(nonce_external, nonce);
    LOCAL_INPUT_FREE(additional_data_external, additional_data);
    LOCAL_INPUT_FREE(ciphertext_external, ciphertext);
    LOCAL_OUTPUT_FREE(plaintext_external, plaintext);

    psa_unregister_read_under_mutex(slot);

    return status;
}

static psa_status_t psa_validate_tag_length(psa_algorithm_t alg)
{
    const uint8_t tag_len = PSA_ALG_AEAD_GET_TAG_LENGTH(alg);

    switch (PSA_ALG_AEAD_WITH_SHORTENED_TAG(alg, 0)) {
#if defined(PSA_WANT_ALG_CCM)
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0):
                                                                               
            if (tag_len < 4 || tag_len > 16 || tag_len % 2) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            break;
#endif                       

#if defined(PSA_WANT_ALG_GCM)
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0):
                                                                                 
            if (tag_len != 4 && tag_len != 8 && (tag_len < 12 || tag_len > 16)) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            break;
#endif                       

#if defined(PSA_WANT_ALG_CHACHA20_POLY1305)
        case PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CHACHA20_POLY1305, 0):
                                                         
            if (tag_len != 16) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
            break;
#endif                                     

        default:
            (void) tag_len;
            return PSA_ERROR_NOT_SUPPORTED;
    }
    return PSA_SUCCESS;
}

                                                          
static psa_status_t psa_aead_setup(psa_aead_operation_t *operation,
                                   int is_encrypt,
                                   mbedtls_svc_key_id_t key,
                                   psa_algorithm_t alg)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot = NULL;
    psa_key_usage_t key_usage = 0;

    status = psa_aead_check_algorithm(alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (operation->id != 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->nonce_set || operation->lengths_set ||
        operation->ad_started || operation->body_started) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

                                                                      
                                                                         
                                                                   
                                                                     
                             
    memset(&operation->ctx, 0, sizeof(operation->ctx));

    if (is_encrypt) {
        key_usage = PSA_KEY_USAGE_ENCRYPT;
    } else {
        key_usage = PSA_KEY_USAGE_DECRYPT;
    }

    status = psa_get_and_lock_key_slot_with_policy(key, &slot, key_usage,
                                                   alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if ((status = psa_validate_tag_length(alg)) != PSA_SUCCESS) {
        goto exit;
    }

    if (is_encrypt) {
        status = psa_driver_wrapper_aead_encrypt_setup(operation,
                                                       &slot->attr,
                                                       slot->key.data,
                                                       slot->key.bytes,
                                                       alg);
    } else {
        status = psa_driver_wrapper_aead_decrypt_setup(operation,
                                                       &slot->attr,
                                                       slot->key.data,
                                                       slot->key.bytes,
                                                       alg);
    }
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    operation->key_type = psa_get_key_type(&slot->attr);

exit:
    unlock_status = psa_unregister_read_under_mutex(slot);

    if (status == PSA_SUCCESS) {
        status = unlock_status;
        operation->alg = psa_aead_get_base_algorithm(alg);
        operation->is_encrypt = is_encrypt;
    } else {
        psa_aead_abort(operation);
    }

    return status;
}

                                                                     
psa_status_t psa_aead_encrypt_setup(psa_aead_operation_t *operation,
                                    mbedtls_svc_key_id_t key,
                                    psa_algorithm_t alg)
{
    return psa_aead_setup(operation, 1, key, alg);
}

                                                                     
psa_status_t psa_aead_decrypt_setup(psa_aead_operation_t *operation,
                                    mbedtls_svc_key_id_t key,
                                    psa_algorithm_t alg)
{
    return psa_aead_setup(operation, 0, key, alg);
}

static psa_status_t psa_aead_set_nonce_internal(psa_aead_operation_t *operation,
                                                const uint8_t *nonce,
                                                size_t nonce_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->nonce_set) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    status = psa_aead_check_nonce_length(operation->alg, nonce_length);
    if (status != PSA_SUCCESS) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    status = psa_driver_wrapper_aead_set_nonce(operation, nonce,
                                               nonce_length);

exit:
    if (status == PSA_SUCCESS) {
        operation->nonce_set = 1;
    } else {
        psa_aead_abort(operation);
    }

    return status;
}

                                                               
psa_status_t psa_aead_generate_nonce(psa_aead_operation_t *operation,
                                     uint8_t *nonce_external,
                                     size_t nonce_size,
                                     size_t *nonce_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    uint8_t local_nonce[PSA_AEAD_NONCE_MAX_SIZE];
    size_t required_nonce_size = 0;

    LOCAL_OUTPUT_DECLARE(nonce_external, nonce);
    LOCAL_OUTPUT_ALLOC(nonce_external, nonce_size, nonce);

    *nonce_length = 0;

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->nonce_set || !operation->is_encrypt) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

                                                                 
                                                                
      
                                                                             
                                                                               
      
                                                                            
                                                                      
                                                                
    required_nonce_size = PSA_AEAD_NONCE_LENGTH(operation->key_type,
                                                operation->alg);
    if (nonce_size < required_nonce_size) {
        status = PSA_ERROR_BUFFER_TOO_SMALL;
        goto exit;
    }

    status = psa_generate_random_internal(local_nonce, required_nonce_size);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_aead_set_nonce_internal(operation, local_nonce,
                                         required_nonce_size);

exit:
    if (status == PSA_SUCCESS) {
        memcpy(nonce, local_nonce, required_nonce_size);
        *nonce_length = required_nonce_size;
    } else {
        psa_aead_abort(operation);
    }

    LOCAL_OUTPUT_FREE(nonce_external, nonce);

    return status;
}

                                                                       
               
psa_status_t psa_aead_set_nonce(psa_aead_operation_t *operation,
                                const uint8_t *nonce_external,
                                size_t nonce_length)
{
    psa_status_t status;

    LOCAL_INPUT_DECLARE(nonce_external, nonce);
    LOCAL_INPUT_ALLOC(nonce_external, nonce_length, nonce);

    status = psa_aead_set_nonce_internal(operation, nonce, nonce_length);

                                                                            
#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif

    LOCAL_INPUT_FREE(nonce_external, nonce);

    return status;
}

                                                                                
psa_status_t psa_aead_set_lengths(psa_aead_operation_t *operation,
                                  size_t ad_length,
                                  size_t plaintext_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->lengths_set || operation->ad_started ||
        operation->body_started) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    switch (operation->alg) {
#if defined(PSA_WANT_ALG_GCM)
        case PSA_ALG_GCM:
                                                                                
                                                                                
                         
#if SIZE_MAX > UINT32_MAX
            if (((uint64_t) ad_length) >> 61 != 0 ||
                ((uint64_t) plaintext_length) > 0xFFFFFFFE0ull) {
                status = PSA_ERROR_INVALID_ARGUMENT;
                goto exit;
            }
#endif
            break;
#endif                       
#if defined(PSA_WANT_ALG_CCM)
        case PSA_ALG_CCM:
            if (ad_length > 0xFF00) {
                status = PSA_ERROR_INVALID_ARGUMENT;
                goto exit;
            }
            break;
#endif                       
#if defined(PSA_WANT_ALG_CHACHA20_POLY1305)
        case PSA_ALG_CHACHA20_POLY1305:
#if SIZE_MAX > UINT32_MAX
            if (plaintext_length > (size_t) UINT32_MAX *
                64U) {
                status = PSA_ERROR_INVALID_ARGUMENT;
                goto exit;
            }
#endif
            break;
#endif                                     
        default:
            break;
    }

    status = psa_driver_wrapper_aead_set_lengths(operation, ad_length,
                                                 plaintext_length);

exit:
    if (status == PSA_SUCCESS) {
        operation->ad_remaining = ad_length;
        operation->body_remaining = plaintext_length;
        operation->lengths_set = 1;
    } else {
        psa_aead_abort(operation);
    }

    return status;
}

                                                                 
psa_status_t psa_aead_update_ad(psa_aead_operation_t *operation,
                                const uint8_t *input_external,
                                size_t input_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_INPUT_ALLOC(input_external, input_length, input);

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (!operation->nonce_set || operation->body_started) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

                                                       
    if (input_length == 0) {
        status = PSA_SUCCESS;
        goto exit;
    }

    if (operation->lengths_set) {
        if (operation->ad_remaining < input_length) {
            status = PSA_ERROR_INVALID_ARGUMENT;
            goto exit;
        }

        operation->ad_remaining -= input_length;
    }
#if defined(PSA_WANT_ALG_CCM)
    else if (operation->alg == PSA_ALG_CCM) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }
#endif                       

    status = psa_driver_wrapper_aead_update_ad(operation, input,
                                               input_length);

exit:
    if (status == PSA_SUCCESS) {
        operation->ad_started = 1;
    } else {
        psa_aead_abort(operation);
    }

    LOCAL_INPUT_FREE(input_external, input);

    return status;
}

                                                                    
               
psa_status_t psa_aead_update(psa_aead_operation_t *operation,
                             const uint8_t *input_external,
                             size_t input_length,
                             uint8_t *output_external,
                             size_t output_size,
                             size_t *output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;


    LOCAL_INPUT_DECLARE(input_external, input);
    LOCAL_OUTPUT_DECLARE(output_external, output);

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    *output_length = 0;

    if (operation->id == 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (!operation->nonce_set) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (operation->lengths_set) {
                                                                          
                               
        if (operation->ad_remaining != 0) {
            status = PSA_ERROR_INVALID_ARGUMENT;
            goto exit;
        }

                                     
        if (operation->body_remaining < input_length) {
            status = PSA_ERROR_INVALID_ARGUMENT;
            goto exit;
        }

        operation->body_remaining -= input_length;
    }
#if defined(PSA_WANT_ALG_CCM)
    else if (operation->alg == PSA_ALG_CCM) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }
#endif                       

    status = psa_driver_wrapper_aead_update(operation, input, input_length,
                                            output, output_size,
                                            output_length);

exit:
    if (status == PSA_SUCCESS) {
        operation->body_started = 1;
    } else {
        psa_aead_abort(operation);
    }

    LOCAL_INPUT_FREE(input_external, input);
    LOCAL_OUTPUT_FREE(output_external, output);

    return status;
}

static psa_status_t psa_aead_final_checks(const psa_aead_operation_t *operation)
{
    if (operation->alg == PSA_ALG_CCM && !operation->lengths_set) {
        return PSA_ERROR_BAD_STATE;
    }

    if (operation->id == 0 || !operation->nonce_set) {
        return PSA_ERROR_BAD_STATE;
    }

    if (operation->lengths_set && (operation->ad_remaining != 0 ||
                                   operation->body_remaining != 0)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return PSA_SUCCESS;
}

                                                                
psa_status_t psa_aead_finish(psa_aead_operation_t *operation,
                             uint8_t *ciphertext_external,
                             size_t ciphertext_size,
                             size_t *ciphertext_length,
                             uint8_t *tag_external,
                             size_t tag_size,
                             size_t *tag_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    LOCAL_OUTPUT_DECLARE(ciphertext_external, ciphertext);
    LOCAL_OUTPUT_DECLARE(tag_external, tag);

    LOCAL_OUTPUT_ALLOC(ciphertext_external, ciphertext_size, ciphertext);
    LOCAL_OUTPUT_ALLOC(tag_external, tag_size, tag);

    *ciphertext_length = 0;
    *tag_length = tag_size;

    status = psa_aead_final_checks(operation);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (!operation->is_encrypt) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    status = psa_driver_wrapper_aead_finish(operation, ciphertext,
                                            ciphertext_size,
                                            ciphertext_length,
                                            tag, tag_size, tag_length);

exit:


                                                                             
                                                                            
                                                                         
                                                                           
                         
    psa_wipe_tag_output_buffer(tag, status, tag_size, *tag_length);

    psa_aead_abort(operation);

    LOCAL_OUTPUT_FREE(ciphertext_external, ciphertext);
    LOCAL_OUTPUT_FREE(tag_external, tag);

    return status;
}

                                                                     
               
psa_status_t psa_aead_verify(psa_aead_operation_t *operation,
                             uint8_t *plaintext_external,
                             size_t plaintext_size,
                             size_t *plaintext_length,
                             const uint8_t *tag_external,
                             size_t tag_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    LOCAL_OUTPUT_DECLARE(plaintext_external, plaintext);
    LOCAL_INPUT_DECLARE(tag_external, tag);

    LOCAL_OUTPUT_ALLOC(plaintext_external, plaintext_size, plaintext);
    LOCAL_INPUT_ALLOC(tag_external, tag_length, tag);

    *plaintext_length = 0;

    status = psa_aead_final_checks(operation);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    if (operation->is_encrypt) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    status = psa_driver_wrapper_aead_verify(operation, plaintext,
                                            plaintext_size,
                                            plaintext_length,
                                            tag, tag_length);

exit:
    psa_aead_abort(operation);

    LOCAL_OUTPUT_FREE(plaintext_external, plaintext);
    LOCAL_INPUT_FREE(tag_external, tag);

    return status;
}

                              
psa_status_t psa_aead_abort(psa_aead_operation_t *operation)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (operation->id == 0) {
                                                                           
                                                                       
                            
        return PSA_SUCCESS;
    }

    status = psa_driver_wrapper_aead_abort(operation);

    memset(operation, 0, sizeof(*operation));

    return status;
}

                                                                  
                                       
                                                                  

#if defined(BUILTIN_ALG_ANY_HKDF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS) || \
    defined(PSA_HAVE_SOFT_PBKDF2)
#define AT_LEAST_ONE_BUILTIN_KDF
#endif                               

#if defined(BUILTIN_ALG_ANY_HKDF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)

                                                                           
  
                                                                           
                                               
                                                                 
                                             
                                                               
  
                             
   
static psa_status_t psa_key_derivation_start_hmac(
    psa_mac_operation_t *operation,
    psa_algorithm_t hash_alg,
    const uint8_t *hmac_key,
    size_t hmac_key_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(hmac_key_length));
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);

                                                   
                                                                       
                                                                          
                                                                  
                                                                       
                                                            
    memset(operation, 0, sizeof(*operation));

    operation->is_sign = 1;
    operation->mac_size = PSA_HASH_LENGTH(hash_alg);

    status = psa_driver_wrapper_mac_sign_setup(operation,
                                               &attributes,
                                               hmac_key, hmac_key_length,
                                               PSA_ALG_HMAC(hash_alg));

    psa_reset_key_attributes(&attributes);
    return status;
}
#endif                                     

#define HKDF_STATE_INIT 0                   
#define HKDF_STATE_STARTED 1               
#define HKDF_STATE_KEYED 2              
#define HKDF_STATE_OUTPUT 3                     

static psa_algorithm_t psa_key_derivation_get_kdf_alg(
    const psa_key_derivation_operation_t *operation)
{
    if (PSA_ALG_IS_KEY_AGREEMENT(operation->alg)) {
        return PSA_ALG_KEY_AGREEMENT_GET_KDF(operation->alg);
    } else {
        return operation->alg;
    }
}

psa_status_t psa_key_derivation_abort(psa_key_derivation_operation_t *operation)
{
    psa_status_t status = PSA_SUCCESS;
    psa_algorithm_t kdf_alg = psa_key_derivation_get_kdf_alg(operation);
    if (kdf_alg == 0) {
                                                                     
                                                                       
                            
    } else
#if defined(BUILTIN_ALG_ANY_HKDF)
    if (PSA_ALG_IS_ANY_HKDF(kdf_alg)) {
        mbedtls_free(operation->ctx.hkdf.info);
        status = psa_mac_abort(&operation->ctx.hkdf.hmac);
    } else
#endif                           
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
    if (PSA_ALG_IS_TLS12_PRF(kdf_alg) ||
                                                                     
        PSA_ALG_IS_TLS12_PSK_TO_MS(kdf_alg)) {
        if (operation->ctx.tls12_prf.secret != NULL) {
            mbedtls_zeroize_and_free(operation->ctx.tls12_prf.secret,
                                     operation->ctx.tls12_prf.secret_length);
        }

        if (operation->ctx.tls12_prf.seed != NULL) {
            mbedtls_zeroize_and_free(operation->ctx.tls12_prf.seed,
                                     operation->ctx.tls12_prf.seed_length);
        }

        if (operation->ctx.tls12_prf.label != NULL) {
            mbedtls_zeroize_and_free(operation->ctx.tls12_prf.label,
                                     operation->ctx.tls12_prf.label_length);
        }
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
        if (operation->ctx.tls12_prf.other_secret != NULL) {
            mbedtls_zeroize_and_free(operation->ctx.tls12_prf.other_secret,
                                     operation->ctx.tls12_prf.other_secret_length);
        }
#endif                                              
        status = PSA_SUCCESS;

                                                                             
                                                                     
    } else
#endif                                                \
                                                             
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS)
    if (kdf_alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
        mbedtls_platform_zeroize(operation->ctx.tls12_ecjpake_to_pms.data,
                                 sizeof(operation->ctx.tls12_ecjpake_to_pms.data));
    } else
#endif                                                            
#if defined(PSA_HAVE_SOFT_PBKDF2)
    if (PSA_ALG_IS_PBKDF2(kdf_alg)) {
        if (operation->ctx.pbkdf2.salt != NULL) {
            mbedtls_zeroize_and_free(operation->ctx.pbkdf2.salt,
                                     operation->ctx.pbkdf2.salt_length);
        }

        status = PSA_SUCCESS;
    } else
#endif                                    
    {
        status = PSA_ERROR_BAD_STATE;
    }
    mbedtls_platform_zeroize(operation, sizeof(*operation));
    return status;
}

psa_status_t psa_key_derivation_get_capacity(const psa_key_derivation_operation_t *operation,
                                             size_t *capacity)
{
    if (operation->alg == 0) {
                                                       
        return PSA_ERROR_BAD_STATE;
    }

    *capacity = operation->capacity;
    return PSA_SUCCESS;
}

psa_status_t psa_key_derivation_set_capacity(psa_key_derivation_operation_t *operation,
                                             size_t capacity)
{
    if (operation->alg == 0) {
        return PSA_ERROR_BAD_STATE;
    }
    if (capacity > operation->capacity) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }
    operation->capacity = capacity;
    return PSA_SUCCESS;
}

#if defined(BUILTIN_ALG_ANY_HKDF)
                                                   
static psa_status_t psa_key_derivation_hkdf_read(psa_hkdf_key_derivation_t *hkdf,
                                                 psa_algorithm_t kdf_alg,
                                                 uint8_t *output,
                                                 size_t output_length)
{
    psa_algorithm_t hash_alg = PSA_ALG_HKDF_GET_HASH(kdf_alg);
    uint8_t hash_length = PSA_HASH_LENGTH(hash_alg);
    size_t hmac_output_length;
    psa_status_t status;
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT)
    const uint8_t last_block = PSA_ALG_IS_HKDF_EXTRACT(kdf_alg) ? 0 : 0xff;
#else
    const uint8_t last_block = 0xff;
#endif                                           

    if (hkdf->state < HKDF_STATE_KEYED ||
        (!hkdf->info_set
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT)
         && !PSA_ALG_IS_HKDF_EXTRACT(kdf_alg)
#endif                                           
        )) {
        return PSA_ERROR_BAD_STATE;
    }
    hkdf->state = HKDF_STATE_OUTPUT;

    while (output_length != 0) {
                                                    
        uint8_t n = hash_length - hkdf->offset_in_block;
        if (n > output_length) {
            n = (uint8_t) output_length;
        }
        memcpy(output, hkdf->output_block + hkdf->offset_in_block, n);
        output += n;
        output_length -= n;
        hkdf->offset_in_block += n;
        if (output_length == 0) {
            break;
        }
                                                                          
                                                                             
                                                                     
                                                                      
                                 
        if (hkdf->block_number == last_block) {
            return PSA_ERROR_BAD_STATE;
        }

                                 
        ++hkdf->block_number;
        hkdf->offset_in_block = 0;

        status = psa_key_derivation_start_hmac(&hkdf->hmac,
                                               hash_alg,
                                               hkdf->prk,
                                               hash_length);
        if (status != PSA_SUCCESS) {
            return status;
        }

        if (hkdf->block_number != 1) {
            status = psa_mac_update(&hkdf->hmac,
                                    hkdf->output_block,
                                    hash_length);
            if (status != PSA_SUCCESS) {
                return status;
            }
        }
        status = psa_mac_update(&hkdf->hmac,
                                hkdf->info,
                                hkdf->info_length);
        if (status != PSA_SUCCESS) {
            return status;
        }
        status = psa_mac_update(&hkdf->hmac,
                                &hkdf->block_number, 1);
        if (status != PSA_SUCCESS) {
            return status;
        }
        status = psa_mac_sign_finish(&hkdf->hmac,
                                     hkdf->output_block,
                                     sizeof(hkdf->output_block),
                                     &hmac_output_length);
        if (status != PSA_SUCCESS) {
            return status;
        }
    }

    return PSA_SUCCESS;
}
#endif                           

#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
static psa_status_t psa_key_derivation_tls12_prf_generate_next_block(
    psa_tls12_prf_key_derivation_t *tls12_prf,
    psa_algorithm_t alg)
{
    psa_algorithm_t hash_alg = PSA_ALG_HKDF_GET_HASH(alg);
    uint8_t hash_length = PSA_HASH_LENGTH(hash_alg);
    psa_mac_operation_t hmac;
    size_t hmac_output_length;
    psa_status_t status, cleanup_status;

                                                                  
                                                                         
                                                                 
                                                                  
                             
    if (tls12_prf->block_number == 0xff) {
        return PSA_ERROR_CORRUPTION_DETECTED;
    }

                             
    ++tls12_prf->block_number;
    tls12_prf->left_in_block = hash_length;

                                                              
      
                                                                
      
                                                              
                                                              
                                                                  
      
                  
                                       
      
                                                                   
                                                             
                                                              
                      
       

    status = psa_key_derivation_start_hmac(&hmac,
                                           hash_alg,
                                           tls12_prf->secret,
                                           tls12_prf->secret_length);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }

                                                           
    if (tls12_prf->block_number == 1) {
                                                                                
                                                                                
                                                          
        status = psa_mac_update(&hmac,
                                tls12_prf->label,
                                tls12_prf->label_length);
        if (status != PSA_SUCCESS) {
            goto cleanup;
        }
        status = psa_mac_update(&hmac,
                                tls12_prf->seed,
                                tls12_prf->seed_length);
        if (status != PSA_SUCCESS) {
            goto cleanup;
        }
    } else {
                                              
        status = psa_mac_update(&hmac, tls12_prf->Ai, hash_length);
        if (status != PSA_SUCCESS) {
            goto cleanup;
        }
    }

    status = psa_mac_sign_finish(&hmac,
                                 tls12_prf->Ai, hash_length,
                                 &hmac_output_length);
    if (hmac_output_length != hash_length) {
        status = PSA_ERROR_CORRUPTION_DETECTED;
    }
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }

                                                           
    status = psa_key_derivation_start_hmac(&hmac,
                                           hash_alg,
                                           tls12_prf->secret,
                                           tls12_prf->secret_length);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }
    status = psa_mac_update(&hmac, tls12_prf->Ai, hash_length);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }
    status = psa_mac_update(&hmac, tls12_prf->label, tls12_prf->label_length);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }
    status = psa_mac_update(&hmac, tls12_prf->seed, tls12_prf->seed_length);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }
    status = psa_mac_sign_finish(&hmac,
                                 tls12_prf->output_block, hash_length,
                                 &hmac_output_length);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }


cleanup:
    cleanup_status = psa_mac_abort(&hmac);
    if (status == PSA_SUCCESS && cleanup_status != PSA_SUCCESS) {
        status = cleanup_status;
    }

    return status;
}

static psa_status_t psa_key_derivation_tls12_prf_read(
    psa_tls12_prf_key_derivation_t *tls12_prf,
    psa_algorithm_t alg,
    uint8_t *output,
    size_t output_length)
{
    psa_algorithm_t hash_alg = PSA_ALG_TLS12_PRF_GET_HASH(alg);
    uint8_t hash_length = PSA_HASH_LENGTH(hash_alg);
    psa_status_t status;
    uint8_t offset, length;

    switch (tls12_prf->state) {
        case PSA_TLS12_PRF_STATE_LABEL_SET:
            tls12_prf->state = PSA_TLS12_PRF_STATE_OUTPUT;
            break;
        case PSA_TLS12_PRF_STATE_OUTPUT:
            break;
        default:
            return PSA_ERROR_BAD_STATE;
    }

    while (output_length != 0) {
                                                                 
        if (tls12_prf->left_in_block == 0) {
            status = psa_key_derivation_tls12_prf_generate_next_block(tls12_prf,
                                                                      alg);
            if (status != PSA_SUCCESS) {
                return status;
            }

            continue;
        }

        if (tls12_prf->left_in_block > output_length) {
            length = (uint8_t) output_length;
        } else {
            length = tls12_prf->left_in_block;
        }

        offset = hash_length - tls12_prf->left_in_block;
        memcpy(output, tls12_prf->output_block + offset, length);
        output += length;
        output_length -= length;
        tls12_prf->left_in_block -= length;
    }

    return PSA_SUCCESS;
}
#endif                                       \
                                                    

#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS)
static psa_status_t psa_key_derivation_tls12_ecjpake_to_pms_read(
    psa_tls12_ecjpake_to_pms_t *ecjpake,
    uint8_t *output,
    size_t output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    size_t output_size = 0;

    if (output_length != 32) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    status = psa_hash_compute(PSA_ALG_SHA_256, ecjpake->data,
                              PSA_TLS12_ECJPAKE_TO_PMS_DATA_SIZE, output, output_length,
                              &output_size);
    if (status != PSA_SUCCESS) {
        return status;
    }

    if (output_size != output_length) {
        return PSA_ERROR_GENERIC_ERROR;
    }

    return PSA_SUCCESS;
}
#endif

#if defined(PSA_HAVE_SOFT_PBKDF2)
static psa_status_t psa_key_derivation_pbkdf2_generate_block(
    psa_pbkdf2_key_derivation_t *pbkdf2,
    psa_algorithm_t prf_alg,
    uint8_t prf_output_length,
    psa_key_attributes_t *attributes)
{
    psa_status_t status;
    psa_mac_operation_t mac_operation;
                                                   
                                                               
                                                                   
                                                 
                                                                  
                                         
    memset(&mac_operation, 0, sizeof(mac_operation));
    size_t mac_output_length;
    uint8_t U_i[PSA_MAC_MAX_SIZE];
    uint8_t *U_accumulator = pbkdf2->output_block;
    uint64_t i;
    uint8_t block_counter[4];

    mac_operation.is_sign = 1;
    mac_operation.mac_size = prf_output_length;
    MBEDTLS_PUT_UINT32_BE(pbkdf2->block_number, block_counter, 0);

    status = psa_driver_wrapper_mac_sign_setup(&mac_operation,
                                               attributes,
                                               pbkdf2->password,
                                               pbkdf2->password_length,
                                               prf_alg);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }
    status = psa_mac_update(&mac_operation, pbkdf2->salt, pbkdf2->salt_length);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }
    status = psa_mac_update(&mac_operation, block_counter, sizeof(block_counter));
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }
    status = psa_mac_sign_finish(&mac_operation, U_i, sizeof(U_i),
                                 &mac_output_length);
    if (status != PSA_SUCCESS) {
        goto cleanup;
    }

    if (mac_output_length != prf_output_length) {
        status = PSA_ERROR_CORRUPTION_DETECTED;
        goto cleanup;
    }

    memcpy(U_accumulator, U_i, prf_output_length);

    for (i = 1; i < pbkdf2->input_cost; i++) {
                                                                          
                                                                             
                                                                 
        status = psa_driver_wrapper_mac_compute(attributes,
                                                pbkdf2->password,
                                                pbkdf2->password_length,
                                                prf_alg, U_i, prf_output_length,
                                                U_i, prf_output_length,
                                                &mac_output_length);
        if (status != PSA_SUCCESS) {
            goto cleanup;
        }

        mbedtls_xor(U_accumulator, U_accumulator, U_i, prf_output_length);
    }

cleanup:
                                                              
    mbedtls_platform_zeroize(U_i, PSA_MAC_MAX_SIZE);
    return status;
}

static psa_status_t psa_key_derivation_pbkdf2_read(
    psa_pbkdf2_key_derivation_t *pbkdf2,
    psa_algorithm_t kdf_alg,
    uint8_t *output,
    size_t output_length)
{
    psa_status_t status;
    psa_algorithm_t prf_alg;
    uint8_t prf_output_length;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(pbkdf2->password_length));
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);

    if (PSA_ALG_IS_PBKDF2_HMAC(kdf_alg)) {
        prf_alg = PSA_ALG_HMAC(PSA_ALG_PBKDF2_HMAC_GET_HASH(kdf_alg));
        prf_output_length = PSA_HASH_LENGTH(prf_alg);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    } else if (kdf_alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
        prf_alg = PSA_ALG_CMAC;
        prf_output_length = PSA_MAC_LENGTH(PSA_KEY_TYPE_AES, 128U, PSA_ALG_CMAC);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    } else {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    switch (pbkdf2->state) {
        case PSA_PBKDF2_STATE_PASSWORD_SET:
                                                                                   
            pbkdf2->bytes_used = prf_output_length;
            pbkdf2->state = PSA_PBKDF2_STATE_OUTPUT;
            break;
        case PSA_PBKDF2_STATE_OUTPUT:
            break;
        default:
            return PSA_ERROR_BAD_STATE;
    }

    while (output_length != 0) {
        uint8_t n = prf_output_length - pbkdf2->bytes_used;
        if (n > output_length) {
            n = (uint8_t) output_length;
        }
        memcpy(output, pbkdf2->output_block + pbkdf2->bytes_used, n);
        output += n;
        output_length -= n;
        pbkdf2->bytes_used += n;

        if (output_length == 0) {
            break;
        }

                                 
        pbkdf2->bytes_used = 0;
        pbkdf2->block_number++;

        status = psa_key_derivation_pbkdf2_generate_block(pbkdf2, prf_alg,
                                                          prf_output_length,
                                                          &attributes);
        if (status != PSA_SUCCESS) {
            return status;
        }
    }

    return PSA_SUCCESS;
}
#endif                           

psa_status_t psa_key_derivation_output_bytes(
    psa_key_derivation_operation_t *operation,
    uint8_t *output_external,
    size_t output_length)
{
    psa_status_t status;
    LOCAL_OUTPUT_DECLARE(output_external, output);

    psa_algorithm_t kdf_alg = psa_key_derivation_get_kdf_alg(operation);

    if (operation->alg == 0) {
                                        
        return PSA_ERROR_BAD_STATE;
    }

    if (output_length == 0 && operation->capacity == 0) {
                                                               
                                                             
                                                               
                                                               
                                                        
                                
        return PSA_ERROR_INSUFFICIENT_DATA;
    }

    LOCAL_OUTPUT_ALLOC(output_external, output_length, output);
    if (output_length > operation->capacity) {
        operation->capacity = 0;
                                                                      
                                                   
        status = PSA_ERROR_INSUFFICIENT_DATA;
        goto exit;
    }

    operation->capacity -= output_length;

#if defined(BUILTIN_ALG_ANY_HKDF)
    if (PSA_ALG_IS_ANY_HKDF(kdf_alg)) {
        status = psa_key_derivation_hkdf_read(&operation->ctx.hkdf, kdf_alg,
                                              output, output_length);
    } else
#endif                           
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
    if (PSA_ALG_IS_TLS12_PRF(kdf_alg) ||
        PSA_ALG_IS_TLS12_PSK_TO_MS(kdf_alg)) {
        status = psa_key_derivation_tls12_prf_read(&operation->ctx.tls12_prf,
                                                   kdf_alg, output,
                                                   output_length);
    } else
#endif                                       \
                                                    
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS)
    if (kdf_alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
        status = psa_key_derivation_tls12_ecjpake_to_pms_read(
            &operation->ctx.tls12_ecjpake_to_pms, output, output_length);
    } else
#endif                                                   
#if defined(PSA_HAVE_SOFT_PBKDF2)
    if (PSA_ALG_IS_PBKDF2(kdf_alg)) {
        status = psa_key_derivation_pbkdf2_read(&operation->ctx.pbkdf2, kdf_alg,
                                                output, output_length);
    } else
#endif                           

    {
        (void) kdf_alg;
        status = PSA_ERROR_BAD_STATE;
        LOCAL_OUTPUT_FREE(output_external, output);

        return status;
    }

exit:
    if (status != PSA_SUCCESS) {
                                                                             
                                                                           
                                                                          
                         
        psa_algorithm_t alg = operation->alg;
        psa_key_derivation_abort(operation);
        operation->alg = alg;
        if (output != NULL) {
            memset(output, '!', output_length);
        }
    }

    LOCAL_OUTPUT_FREE(output_external, output);
    return status;
}

#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DES)
static void psa_des_set_key_parity(uint8_t *data, size_t data_size)
{
    if (data_size >= 8) {
        mbedtls_des_key_set_parity(data);
    }
    if (data_size >= 16) {
        mbedtls_des_key_set_parity(data + 8);
    }
    if (data_size >= 24) {
        mbedtls_des_key_set_parity(data + 16);
    }
}
#endif                                       

  
                                                                  
                                       
                                                                              
                                                           
                              
  
                                                            
                                                                       
  
                                                      
                                                           
                                                                          
                                                                                 
                                                            
                                      
  
                                                                                   
                                                                           
                                                                                     
                                                                             
                         
  
                                                                        
                                                               
  
                                                                             
                     
   
#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
static psa_status_t psa_generate_derived_ecc_key_weierstrass_helper(
    psa_key_slot_t *slot,
    size_t bits,
    psa_key_derivation_operation_t *operation,
    uint8_t **data
    )
{
    unsigned key_out_of_range = 1;
    mbedtls_mpi k;
    mbedtls_mpi diff_N_2;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    size_t m;
    size_t m_bytes = 0;

    mbedtls_mpi_init(&k);
    mbedtls_mpi_init(&diff_N_2);

    psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(
        slot->attr.type);
    mbedtls_ecp_group_id grp_id =
        mbedtls_ecc_group_from_psa(curve, bits);

    if (grp_id == MBEDTLS_ECP_DP_NONE) {
        ret = MBEDTLS_ERR_ASN1_INVALID_DATA;
        goto cleanup;
    }

    mbedtls_ecp_group ecp_group;
    mbedtls_ecp_group_init(&ecp_group);

    MBEDTLS_MPI_CHK(mbedtls_ecp_group_load(&ecp_group, grp_id));

                                                                    
                                     
    m = ecp_group.nbits;

    m_bytes = PSA_BITS_TO_BYTES(m);

                                                    
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int(&diff_N_2, &ecp_group.N, 2));

                                                                     
                                               
    *data = mbedtls_calloc(1, m_bytes);
    if (*data == NULL) {
        ret = MBEDTLS_ERR_ASN1_ALLOC_FAILED;
        goto cleanup;
    }

    while (key_out_of_range) {
                                                                 
        if ((status = psa_key_derivation_output_bytes(operation, *data, m_bytes)) != 0) {
            goto cleanup;
        }

                                            
        if (m % 8 != 0) {
                                       
                                                               
                                  
               
            uint8_t clear_bit_mask = (1 << (m % 8)) - 1;
            (*data)[0] &= clear_bit_mask;
        }

                                                                 
                                     
           
        MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&k, *data, m_bytes));

                                                                    
                                                                       
                                                 
           
        MBEDTLS_MPI_CHK(mbedtls_mpi_lt_mpi_ct(&diff_N_2, &k, &key_out_of_range));
    }

                                             
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_int(&k, &k, 1));
    MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&k, *data, m_bytes));
cleanup:
    if (ret != 0) {
        status = mbedtls_to_psa_error(ret);
    }
    if (status != PSA_SUCCESS) {
        mbedtls_zeroize_and_free(*data, m_bytes);
        *data = NULL;
    }
    mbedtls_mpi_free(&k);
    mbedtls_mpi_free(&diff_N_2);
    return status;
}

                                                                           
                                                                                
  
                                                      
                                                         
                                               
  
                                                    
                                                                       
  
                                                                             
                     
   

static psa_status_t psa_generate_derived_ecc_key_montgomery_helper(
    size_t bits,
    psa_key_derivation_operation_t *operation,
    uint8_t **data
    )
{
    size_t output_length;
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    switch (bits) {
        case 255:
            output_length = 32;
            break;
        case 448:
            output_length = 56;
            break;
        default:
            return PSA_ERROR_INVALID_ARGUMENT;
            break;
    }

    *data = mbedtls_calloc(1, output_length);

    if (*data == NULL) {
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    }

    status = psa_key_derivation_output_bytes(operation, *data, output_length);

    if (status != PSA_SUCCESS) {
        return status;
    }

    switch (bits) {
        case 255:
            (*data)[0] &= 248;
            (*data)[31] &= 127;
            (*data)[31] |= 64;
            break;
        case 448:
            (*data)[0] &= 252;
            (*data)[55] |= 128;
            break;
        default:
            return PSA_ERROR_CORRUPTION_DETECTED;
            break;
    }

    return status;
}
#else                                                       
static psa_status_t psa_generate_derived_ecc_key_weierstrass_helper(
    psa_key_slot_t *slot, size_t bits,
    psa_key_derivation_operation_t *operation, uint8_t **data)
{
    (void) slot;
    (void) bits;
    (void) operation;
    (void) data;
    return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t psa_generate_derived_ecc_key_montgomery_helper(
    size_t bits, psa_key_derivation_operation_t *operation, uint8_t **data)
{
    (void) bits;
    (void) operation;
    (void) data;
    return PSA_ERROR_NOT_SUPPORTED;
}
#endif                                                       
#endif                                            

static psa_status_t psa_generate_derived_key_internal(
    psa_key_slot_t *slot,
    size_t bits,
    psa_key_derivation_operation_t *operation)
{
    uint8_t *data = NULL;
    size_t bytes = PSA_BITS_TO_BYTES(bits);
    size_t storage_size = bytes;
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (PSA_KEY_TYPE_IS_PUBLIC_KEY(slot->attr.type)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE) || \
    defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
    if (PSA_KEY_TYPE_IS_ECC(slot->attr.type)) {
        psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(slot->attr.type);
        if (PSA_ECC_FAMILY_IS_WEIERSTRASS(curve)) {
                                            
            status = psa_generate_derived_ecc_key_weierstrass_helper(slot, bits, operation, &data);
            if (status != PSA_SUCCESS) {
                goto exit;
            }
        } else {
                                           
            status = psa_generate_derived_ecc_key_montgomery_helper(bits, operation, &data);
            if (status != PSA_SUCCESS) {
                goto exit;
            }
        }
    } else
#endif                                                    \
                                                                      
    if (key_type_is_raw_bytes(slot->attr.type)) {
        if (bits % 8 != 0) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
        data = mbedtls_calloc(1, bytes);
        if (data == NULL) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }

        status = psa_key_derivation_output_bytes(operation, data, bytes);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DES)
        if (slot->attr.type == PSA_KEY_TYPE_DES) {
            psa_des_set_key_parity(data, bytes);
        }
#endif                                                
    } else {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    slot->attr.bits = (psa_key_bits_t) bits;

    if (psa_key_lifetime_is_external(slot->attr.lifetime)) {
        status = psa_driver_wrapper_get_key_buffer_size(&slot->attr,
                                                        &storage_size);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
    }
    status = psa_allocate_buffer_to_slot(slot, storage_size);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_driver_wrapper_import_key(&slot->attr,
                                           data, bytes,
                                           slot->key.data,
                                           slot->key.bytes,
                                           &slot->key.bytes, &bits);
    if (bits != slot->attr.bits) {
        status = PSA_ERROR_INVALID_ARGUMENT;
    }

exit:
    mbedtls_zeroize_and_free(data, bytes);
    return status;
}

static const psa_custom_key_parameters_t default_custom_production =
    PSA_CUSTOM_KEY_PARAMETERS_INIT;

int psa_custom_key_parameters_are_default(
    const psa_custom_key_parameters_t *custom,
    size_t custom_data_length)
{
    if (custom->flags != 0) {
        return 0;
    }
    if (custom_data_length != 0) {
        return 0;
    }
    return 1;
}

psa_status_t psa_key_derivation_output_key_custom(
    const psa_key_attributes_t *attributes,
    psa_key_derivation_operation_t *operation,
    const psa_custom_key_parameters_t *custom,
    const uint8_t *custom_data,
    size_t custom_data_length,
    mbedtls_svc_key_id_t *key)
{
    psa_status_t status;
    psa_key_slot_t *slot = NULL;
    psa_se_drv_table_entry_t *driver = NULL;

    *key = MBEDTLS_SVC_KEY_ID_INIT;

                                                                      
                                                                        
    if (psa_get_key_bits(attributes) == 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    (void) custom_data;                                           
    if (!psa_custom_key_parameters_are_default(custom, custom_data_length)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (operation->alg == PSA_ALG_NONE) {
        return PSA_ERROR_BAD_STATE;
    }

    if (!operation->can_output_key) {
        return PSA_ERROR_NOT_PERMITTED;
    }

    status = psa_start_key_creation(PSA_KEY_CREATION_DERIVE, attributes,
                                    &slot, &driver);
#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
    if (driver != NULL) {
                                                                        
        status = PSA_ERROR_NOT_SUPPORTED;
    }
#endif                              
    if (status == PSA_SUCCESS) {
        status = psa_generate_derived_key_internal(slot,
                                                   attributes->bits,
                                                   operation);
    }
    if (status == PSA_SUCCESS) {
        status = psa_finish_key_creation(slot, driver, key);
    }
    if (status != PSA_SUCCESS) {
        psa_fail_key_creation(slot, driver);
    }

    return status;
}

psa_status_t psa_key_derivation_output_key_ext(
    const psa_key_attributes_t *attributes,
    psa_key_derivation_operation_t *operation,
    const psa_key_production_parameters_t *params,
    size_t params_data_length,
    mbedtls_svc_key_id_t *key)
{
    return psa_key_derivation_output_key_custom(
        attributes, operation,
        (const psa_custom_key_parameters_t *) params,
        params->data, params_data_length,
        key);
}

psa_status_t psa_key_derivation_output_key(
    const psa_key_attributes_t *attributes,
    psa_key_derivation_operation_t *operation,
    mbedtls_svc_key_id_t *key)
{
    return psa_key_derivation_output_key_custom(attributes, operation,
                                                &default_custom_production,
                                                NULL, 0,
                                                key);
}


                                                                  
                                          
                                                                  

#if defined(AT_LEAST_ONE_BUILTIN_KDF)
static int is_kdf_alg_supported(psa_algorithm_t kdf_alg)
{
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF)
    if (PSA_ALG_IS_HKDF(kdf_alg)) {
        return 1;
    }
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT)
    if (PSA_ALG_IS_HKDF_EXTRACT(kdf_alg)) {
        return 1;
    }
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXPAND)
    if (PSA_ALG_IS_HKDF_EXPAND(kdf_alg)) {
        return 1;
    }
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF)
    if (PSA_ALG_IS_TLS12_PRF(kdf_alg)) {
        return 1;
    }
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
    if (PSA_ALG_IS_TLS12_PSK_TO_MS(kdf_alg)) {
        return 1;
    }
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS)
    if (kdf_alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
        return 1;
    }
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_PBKDF2_HMAC)
    if (PSA_ALG_IS_PBKDF2_HMAC(kdf_alg)) {
        return 1;
    }
#endif
#if defined(MBEDTLS_PSA_BUILTIN_ALG_PBKDF2_AES_CMAC_PRF_128)
    if (kdf_alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
        return 1;
    }
#endif
    return 0;
}

static psa_status_t psa_hash_try_support(psa_algorithm_t alg)
{
    psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
    psa_status_t status = psa_hash_setup(&operation, alg);
    psa_hash_abort(&operation);
    return status;
}

static psa_status_t psa_key_derivation_set_maximum_capacity(
    psa_key_derivation_operation_t *operation,
    psa_algorithm_t kdf_alg)
{
#if defined(PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS)
    if (kdf_alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
        operation->capacity = PSA_HASH_LENGTH(PSA_ALG_SHA_256);
        return PSA_SUCCESS;
    }
#endif
#if defined(PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128)
    if (kdf_alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
#if (SIZE_MAX > UINT32_MAX)
        operation->capacity = UINT32_MAX * (size_t) PSA_MAC_LENGTH(
            PSA_KEY_TYPE_AES,
            128U,
            PSA_ALG_CMAC);
#else
        operation->capacity = SIZE_MAX;
#endif
        return PSA_SUCCESS;
    }
#endif                                           

                                                                              
                                                                     
    psa_algorithm_t hash_alg = PSA_ALG_GET_HASH(kdf_alg);
    size_t hash_size = PSA_HASH_LENGTH(hash_alg);
    if (hash_size == 0) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

                                                                       
                                                                        
                     
    psa_status_t status = psa_hash_try_support(hash_alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

#if defined(PSA_WANT_ALG_HKDF)
    if (PSA_ALG_IS_HKDF(kdf_alg)) {
        operation->capacity = 255 * hash_size;
    } else
#endif
#if defined(PSA_WANT_ALG_HKDF_EXTRACT)
    if (PSA_ALG_IS_HKDF_EXTRACT(kdf_alg)) {
        operation->capacity = hash_size;
    } else
#endif
#if defined(PSA_WANT_ALG_HKDF_EXPAND)
    if (PSA_ALG_IS_HKDF_EXPAND(kdf_alg)) {
        operation->capacity = 255 * hash_size;
    } else
#endif
#if defined(PSA_WANT_ALG_TLS12_PRF)
    if (PSA_ALG_IS_TLS12_PRF(kdf_alg) &&
        (hash_alg == PSA_ALG_SHA_256 || hash_alg == PSA_ALG_SHA_384)) {
        operation->capacity = SIZE_MAX;
    } else
#endif
#if defined(PSA_WANT_ALG_TLS12_PSK_TO_MS)
    if (PSA_ALG_IS_TLS12_PSK_TO_MS(kdf_alg) &&
        (hash_alg == PSA_ALG_SHA_256 || hash_alg == PSA_ALG_SHA_384)) {
                                           
                                                                            
        operation->capacity = 48U;
    } else
#endif
#if defined(PSA_WANT_ALG_PBKDF2_HMAC)
    if (PSA_ALG_IS_PBKDF2_HMAC(kdf_alg)) {
#if (SIZE_MAX > UINT32_MAX)
        operation->capacity = UINT32_MAX * hash_size;
#else
        operation->capacity = SIZE_MAX;
#endif
    } else
#endif                               
    {
        (void) hash_size;
        status = PSA_ERROR_NOT_SUPPORTED;
    }
    return status;
}

static psa_status_t psa_key_derivation_setup_kdf(
    psa_key_derivation_operation_t *operation,
    psa_algorithm_t kdf_alg)
{
                                                                         
                                                                    
    memset(&operation->ctx, 0, sizeof(operation->ctx));

                                                                         
    if (!is_kdf_alg_supported(kdf_alg)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    psa_status_t status = psa_key_derivation_set_maximum_capacity(operation,
                                                                  kdf_alg);
    return status;
}

static psa_status_t psa_key_agreement_try_support(psa_algorithm_t alg)
{
#if defined(PSA_WANT_ALG_ECDH)
    if (alg == PSA_ALG_ECDH) {
        return PSA_SUCCESS;
    }
#endif
#if defined(PSA_WANT_ALG_FFDH)
    if (alg == PSA_ALG_FFDH) {
        return PSA_SUCCESS;
    }
#endif
    (void) alg;
    return PSA_ERROR_NOT_SUPPORTED;
}

static int psa_key_derivation_allows_free_form_secret_input(
    psa_algorithm_t kdf_alg)
{
#if defined(PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS)
    if (kdf_alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
        return 0;
    }
#endif
    (void) kdf_alg;
    return 1;
}
#endif                               

psa_status_t psa_key_derivation_setup(psa_key_derivation_operation_t *operation,
                                      psa_algorithm_t alg)
{
#if defined(AT_LEAST_ONE_BUILTIN_KDF)
    psa_status_t status;
#endif

    if (operation->alg != 0) {
        return PSA_ERROR_BAD_STATE;
    }

    if (PSA_ALG_IS_RAW_KEY_AGREEMENT(alg)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    } else if (PSA_ALG_IS_KEY_AGREEMENT(alg)) {
#if defined(AT_LEAST_ONE_BUILTIN_KDF)
        psa_algorithm_t kdf_alg = PSA_ALG_KEY_AGREEMENT_GET_KDF(alg);
        psa_algorithm_t ka_alg = PSA_ALG_KEY_AGREEMENT_GET_BASE(alg);
        status = psa_key_agreement_try_support(ka_alg);
        if (status != PSA_SUCCESS) {
            return status;
        }
        if (!psa_key_derivation_allows_free_form_secret_input(kdf_alg)) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
        status = psa_key_derivation_setup_kdf(operation, kdf_alg);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif                               
    } else if (PSA_ALG_IS_KEY_DERIVATION(alg)) {
#if defined(AT_LEAST_ONE_BUILTIN_KDF)
        status = psa_key_derivation_setup_kdf(operation, alg);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif                               
    } else {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

#if defined(AT_LEAST_ONE_BUILTIN_KDF)
    if (status == PSA_SUCCESS) {
        operation->alg = alg;
    }
    return status;
#endif                               
}

#if defined(BUILTIN_ALG_ANY_HKDF)
static psa_status_t psa_hkdf_input(psa_hkdf_key_derivation_t *hkdf,
                                   psa_algorithm_t kdf_alg,
                                   psa_key_derivation_step_t step,
                                   const uint8_t *data,
                                   size_t data_length)
{
    psa_algorithm_t hash_alg = PSA_ALG_HKDF_GET_HASH(kdf_alg);
    psa_status_t status;
    switch (step) {
        case PSA_KEY_DERIVATION_INPUT_SALT:
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXPAND)
            if (PSA_ALG_IS_HKDF_EXPAND(kdf_alg)) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
#endif                                          
            if (hkdf->state != HKDF_STATE_INIT) {
                return PSA_ERROR_BAD_STATE;
            } else {
                status = psa_key_derivation_start_hmac(&hkdf->hmac,
                                                       hash_alg,
                                                       data, data_length);
                if (status != PSA_SUCCESS) {
                    return status;
                }
                hkdf->state = HKDF_STATE_STARTED;
                return PSA_SUCCESS;
            }
        case PSA_KEY_DERIVATION_INPUT_SECRET:
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXPAND)
            if (PSA_ALG_IS_HKDF_EXPAND(kdf_alg)) {
                                                                                
                                                                                
                                                              
                                           
                if (hkdf->state != HKDF_STATE_INIT) {
                    return PSA_ERROR_BAD_STATE;
                }

                                                                  
                if (data_length != PSA_HASH_LENGTH(hash_alg)) {
                    return PSA_ERROR_INVALID_ARGUMENT;
                }

                memcpy(hkdf->prk, data, data_length);
            } else
#endif                                          
            {
                                                                    
                                                      
                if (hkdf->state == HKDF_STATE_INIT) {
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT)
                    if (PSA_ALG_IS_HKDF_EXTRACT(kdf_alg)) {
                        return PSA_ERROR_BAD_STATE;
                    }
#endif                                           
                    status = psa_key_derivation_start_hmac(&hkdf->hmac,
                                                           hash_alg,
                                                           NULL, 0);
                    if (status != PSA_SUCCESS) {
                        return status;
                    }
                    hkdf->state = HKDF_STATE_STARTED;
                }
                if (hkdf->state != HKDF_STATE_STARTED) {
                    return PSA_ERROR_BAD_STATE;
                }
                status = psa_mac_update(&hkdf->hmac,
                                        data, data_length);
                if (status != PSA_SUCCESS) {
                    return status;
                }
                status = psa_mac_sign_finish(&hkdf->hmac,
                                             hkdf->prk,
                                             sizeof(hkdf->prk),
                                             &data_length);
                if (status != PSA_SUCCESS) {
                    return status;
                }
            }

            hkdf->state = HKDF_STATE_KEYED;
            hkdf->block_number = 0;
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT)
            if (PSA_ALG_IS_HKDF_EXTRACT(kdf_alg)) {
                                                          
                memcpy(hkdf->output_block, hkdf->prk, PSA_HASH_LENGTH(hash_alg));
                hkdf->offset_in_block = 0;
            } else
#endif                                           
            {
                                                               
                                                                  
                hkdf->offset_in_block = PSA_HASH_LENGTH(hash_alg);
            }

            return PSA_SUCCESS;
        case PSA_KEY_DERIVATION_INPUT_INFO:
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT)
            if (PSA_ALG_IS_HKDF_EXTRACT(kdf_alg)) {
                return PSA_ERROR_INVALID_ARGUMENT;
            }
#endif                                           
#if defined(MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXPAND)
            if (PSA_ALG_IS_HKDF_EXPAND(kdf_alg) &&
                hkdf->state == HKDF_STATE_INIT) {
                return PSA_ERROR_BAD_STATE;
            }
#endif                                           
            if (hkdf->state == HKDF_STATE_OUTPUT) {
                return PSA_ERROR_BAD_STATE;
            }
            if (hkdf->info_set) {
                return PSA_ERROR_BAD_STATE;
            }
            hkdf->info_length = data_length;
            if (data_length != 0) {
                hkdf->info = mbedtls_calloc(1, data_length);
                if (hkdf->info == NULL) {
                    return PSA_ERROR_INSUFFICIENT_MEMORY;
                }
                memcpy(hkdf->info, data, data_length);
            }
            hkdf->info_set = 1;
            return PSA_SUCCESS;
        default:
            return PSA_ERROR_INVALID_ARGUMENT;
    }
}
#endif                           

#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
static psa_status_t psa_tls12_prf_set_seed(psa_tls12_prf_key_derivation_t *prf,
                                           const uint8_t *data,
                                           size_t data_length)
{
    if (prf->state != PSA_TLS12_PRF_STATE_INIT) {
        return PSA_ERROR_BAD_STATE;
    }

    if (data_length != 0) {
        prf->seed = mbedtls_calloc(1, data_length);
        if (prf->seed == NULL) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }

        memcpy(prf->seed, data, data_length);
        prf->seed_length = data_length;
    }

    prf->state = PSA_TLS12_PRF_STATE_SEED_SET;

    return PSA_SUCCESS;
}

static psa_status_t psa_tls12_prf_set_key(psa_tls12_prf_key_derivation_t *prf,
                                          const uint8_t *data,
                                          size_t data_length)
{
    if (prf->state != PSA_TLS12_PRF_STATE_SEED_SET &&
        prf->state != PSA_TLS12_PRF_STATE_OTHER_KEY_SET) {
        return PSA_ERROR_BAD_STATE;
    }

    if (data_length != 0) {
        prf->secret = mbedtls_calloc(1, data_length);
        if (prf->secret == NULL) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }

        memcpy(prf->secret, data, data_length);
        prf->secret_length = data_length;
    }

    prf->state = PSA_TLS12_PRF_STATE_KEY_SET;

    return PSA_SUCCESS;
}

static psa_status_t psa_tls12_prf_set_label(psa_tls12_prf_key_derivation_t *prf,
                                            const uint8_t *data,
                                            size_t data_length)
{
    if (prf->state != PSA_TLS12_PRF_STATE_KEY_SET) {
        return PSA_ERROR_BAD_STATE;
    }

    if (data_length != 0) {
        prf->label = mbedtls_calloc(1, data_length);
        if (prf->label == NULL) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }

        memcpy(prf->label, data, data_length);
        prf->label_length = data_length;
    }

    prf->state = PSA_TLS12_PRF_STATE_LABEL_SET;

    return PSA_SUCCESS;
}

static psa_status_t psa_tls12_prf_input(psa_tls12_prf_key_derivation_t *prf,
                                        psa_key_derivation_step_t step,
                                        const uint8_t *data,
                                        size_t data_length)
{
    switch (step) {
        case PSA_KEY_DERIVATION_INPUT_SEED:
            return psa_tls12_prf_set_seed(prf, data, data_length);
        case PSA_KEY_DERIVATION_INPUT_SECRET:
            return psa_tls12_prf_set_key(prf, data, data_length);
        case PSA_KEY_DERIVATION_INPUT_LABEL:
            return psa_tls12_prf_set_label(prf, data, data_length);
        default:
            return PSA_ERROR_INVALID_ARGUMENT;
    }
}
#endif                                        \
                                                    

#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
static psa_status_t psa_tls12_prf_psk_to_ms_set_key(
    psa_tls12_prf_key_derivation_t *prf,
    const uint8_t *data,
    size_t data_length)
{
    psa_status_t status;
    const size_t pms_len = (prf->state == PSA_TLS12_PRF_STATE_OTHER_KEY_SET ?
                            4 + data_length + prf->other_secret_length :
                            4 + 2 * data_length);

    if (data_length > PSA_TLS12_PSK_TO_MS_PSK_MAX_SIZE) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    uint8_t *pms = mbedtls_calloc(1, pms_len);
    if (pms == NULL) {
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    }
    uint8_t *cur = pms;

                
                                   
      
                                                                        
                                                                           
                                                   
      
                 
                                                                         
                                                                         
                                                                      
                  
                                
                                                           
                                                           
                                                  
       

    if (prf->state == PSA_TLS12_PRF_STATE_OTHER_KEY_SET) {
        *cur++ = MBEDTLS_BYTE_1(prf->other_secret_length);
        *cur++ = MBEDTLS_BYTE_0(prf->other_secret_length);
        if (prf->other_secret_length != 0) {
            memcpy(cur, prf->other_secret, prf->other_secret_length);
            mbedtls_platform_zeroize(prf->other_secret, prf->other_secret_length);
            cur += prf->other_secret_length;
        }
    } else {
        *cur++ = MBEDTLS_BYTE_1(data_length);
        *cur++ = MBEDTLS_BYTE_0(data_length);
        memset(cur, 0, data_length);
        cur += data_length;
    }

    *cur++ = MBEDTLS_BYTE_1(data_length);
    *cur++ = MBEDTLS_BYTE_0(data_length);
    memcpy(cur, data, data_length);
    cur += data_length;

    status = psa_tls12_prf_set_key(prf, pms, (size_t) (cur - pms));

    mbedtls_zeroize_and_free(pms, pms_len);
    return status;
}

static psa_status_t psa_tls12_prf_psk_to_ms_set_other_key(
    psa_tls12_prf_key_derivation_t *prf,
    const uint8_t *data,
    size_t data_length)
{
    if (prf->state != PSA_TLS12_PRF_STATE_SEED_SET) {
        return PSA_ERROR_BAD_STATE;
    }

    if (data_length != 0) {
        prf->other_secret = mbedtls_calloc(1, data_length);
        if (prf->other_secret == NULL) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }

        memcpy(prf->other_secret, data, data_length);
        prf->other_secret_length = data_length;
    } else {
        prf->other_secret_length = 0;
    }

    prf->state = PSA_TLS12_PRF_STATE_OTHER_KEY_SET;

    return PSA_SUCCESS;
}

static psa_status_t psa_tls12_prf_psk_to_ms_input(
    psa_tls12_prf_key_derivation_t *prf,
    psa_key_derivation_step_t step,
    const uint8_t *data,
    size_t data_length)
{
    switch (step) {
        case PSA_KEY_DERIVATION_INPUT_SECRET:
            return psa_tls12_prf_psk_to_ms_set_key(prf,
                                                   data, data_length);
            break;
        case PSA_KEY_DERIVATION_INPUT_OTHER_SECRET:
            return psa_tls12_prf_psk_to_ms_set_other_key(prf,
                                                         data,
                                                         data_length);
            break;
        default:
            return psa_tls12_prf_input(prf, step, data, data_length);
            break;

    }
}
#endif                                              

#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS)
static psa_status_t psa_tls12_ecjpake_to_pms_input(
    psa_tls12_ecjpake_to_pms_t *ecjpake,
    psa_key_derivation_step_t step,
    const uint8_t *data,
    size_t data_length)
{
    if (data_length != PSA_TLS12_ECJPAKE_TO_PMS_INPUT_SIZE ||
        step != PSA_KEY_DERIVATION_INPUT_SECRET) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

                                                              
    if (data[0] != 0x04) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

                                                                 
    memcpy(ecjpake->data, data + 1, PSA_TLS12_ECJPAKE_TO_PMS_DATA_SIZE);

    return PSA_SUCCESS;
}
#endif                                                   

#if defined(PSA_HAVE_SOFT_PBKDF2)
static psa_status_t psa_pbkdf2_set_input_cost(
    psa_pbkdf2_key_derivation_t *pbkdf2,
    psa_key_derivation_step_t step,
    uint64_t data)
{
    if (step != PSA_KEY_DERIVATION_INPUT_COST) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (pbkdf2->state != PSA_PBKDF2_STATE_INIT) {
        return PSA_ERROR_BAD_STATE;
    }

    if (data > PSA_VENDOR_PBKDF2_MAX_ITERATIONS) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (data == 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    pbkdf2->input_cost = data;
    pbkdf2->state = PSA_PBKDF2_STATE_INPUT_COST_SET;

    return PSA_SUCCESS;
}

static psa_status_t psa_pbkdf2_set_salt(psa_pbkdf2_key_derivation_t *pbkdf2,
                                        const uint8_t *data,
                                        size_t data_length)
{
    if (pbkdf2->state == PSA_PBKDF2_STATE_INPUT_COST_SET) {
        pbkdf2->state = PSA_PBKDF2_STATE_SALT_SET;
    } else if (pbkdf2->state == PSA_PBKDF2_STATE_SALT_SET) {
                                                          
    } else {
        return PSA_ERROR_BAD_STATE;
    }

    if (data_length == 0) {
                                                       
    } else {
        uint8_t *next_salt;

        next_salt = mbedtls_calloc(1, data_length + pbkdf2->salt_length);
        if (next_salt == NULL) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }

        if (pbkdf2->salt_length != 0) {
            memcpy(next_salt, pbkdf2->salt, pbkdf2->salt_length);
        }
        memcpy(next_salt + pbkdf2->salt_length, data, data_length);
        pbkdf2->salt_length += data_length;
        mbedtls_free(pbkdf2->salt);
        pbkdf2->salt = next_salt;
    }
    return PSA_SUCCESS;
}

#if defined(MBEDTLS_PSA_BUILTIN_ALG_PBKDF2_HMAC)
static psa_status_t psa_pbkdf2_hmac_set_password(psa_algorithm_t hash_alg,
                                                 const uint8_t *input,
                                                 size_t input_len,
                                                 uint8_t *output,
                                                 size_t *output_len)
{
    psa_status_t status = PSA_SUCCESS;
    if (input_len > PSA_HASH_BLOCK_LENGTH(hash_alg)) {
        return psa_hash_compute(hash_alg, input, input_len, output,
                                PSA_HMAC_MAX_HASH_BLOCK_SIZE, output_len);
    } else if (input_len > 0) {
        memcpy(output, input, input_len);
    }
    *output_len = PSA_HASH_BLOCK_LENGTH(hash_alg);
    return status;
}
#endif                                          

#if defined(MBEDTLS_PSA_BUILTIN_ALG_PBKDF2_AES_CMAC_PRF_128)
static psa_status_t psa_pbkdf2_cmac_set_password(const uint8_t *input,
                                                 size_t input_len,
                                                 uint8_t *output,
                                                 size_t *output_len)
{
    psa_status_t status = PSA_SUCCESS;
    if (input_len != PSA_MAC_LENGTH(PSA_KEY_TYPE_AES, 128U, PSA_ALG_CMAC)) {
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        uint8_t zeros[16] = { 0 };
        psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
        psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(sizeof(zeros)));
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
                                                                          
                                                                            
                                                                             
        status = psa_driver_wrapper_mac_compute(&attributes,
                                                zeros, sizeof(zeros),
                                                PSA_ALG_CMAC, input, input_len,
                                                output,
                                                PSA_MAC_LENGTH(PSA_KEY_TYPE_AES,
                                                               128U,
                                                               PSA_ALG_CMAC),
                                                output_len);
    } else {
        memcpy(output, input, input_len);
        *output_len = PSA_MAC_LENGTH(PSA_KEY_TYPE_AES, 128U, PSA_ALG_CMAC);
    }
    return status;
}
#endif                                                      

static psa_status_t psa_pbkdf2_set_password(psa_pbkdf2_key_derivation_t *pbkdf2,
                                            psa_algorithm_t kdf_alg,
                                            const uint8_t *data,
                                            size_t data_length)
{
    psa_status_t status = PSA_SUCCESS;
    if (pbkdf2->state != PSA_PBKDF2_STATE_SALT_SET) {
        return PSA_ERROR_BAD_STATE;
    }

#if defined(MBEDTLS_PSA_BUILTIN_ALG_PBKDF2_HMAC)
    if (PSA_ALG_IS_PBKDF2_HMAC(kdf_alg)) {
        psa_algorithm_t hash_alg = PSA_ALG_PBKDF2_HMAC_GET_HASH(kdf_alg);
        status = psa_pbkdf2_hmac_set_password(hash_alg, data, data_length,
                                              pbkdf2->password,
                                              &pbkdf2->password_length);
    } else
#endif                                          
#if defined(MBEDTLS_PSA_BUILTIN_ALG_PBKDF2_AES_CMAC_PRF_128)
    if (kdf_alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
        status = psa_pbkdf2_cmac_set_password(data, data_length,
                                              pbkdf2->password,
                                              &pbkdf2->password_length);
    } else
#endif                                                      
    {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    pbkdf2->state = PSA_PBKDF2_STATE_PASSWORD_SET;

    return status;
}

static psa_status_t psa_pbkdf2_input(psa_pbkdf2_key_derivation_t *pbkdf2,
                                     psa_algorithm_t kdf_alg,
                                     psa_key_derivation_step_t step,
                                     const uint8_t *data,
                                     size_t data_length)
{
    switch (step) {
        case PSA_KEY_DERIVATION_INPUT_SALT:
            return psa_pbkdf2_set_salt(pbkdf2, data, data_length);
        case PSA_KEY_DERIVATION_INPUT_PASSWORD:
            return psa_pbkdf2_set_password(pbkdf2, kdf_alg, data, data_length);
        default:
            return PSA_ERROR_INVALID_ARGUMENT;
    }
}
#endif                           

                                                                
                                  
  
                                                         
                                                               
                                                                    
                                                                       
                                                                      
   
static int psa_key_derivation_check_input_type(
    psa_key_derivation_step_t step,
    psa_key_type_t key_type)
{
    switch (step) {
        case PSA_KEY_DERIVATION_INPUT_SECRET:
            if (key_type == PSA_KEY_TYPE_DERIVE) {
                return PSA_SUCCESS;
            }
            if (key_type == PSA_KEY_TYPE_NONE) {
                return PSA_SUCCESS;
            }
            break;
        case PSA_KEY_DERIVATION_INPUT_OTHER_SECRET:
            if (key_type == PSA_KEY_TYPE_DERIVE) {
                return PSA_SUCCESS;
            }
            if (key_type == PSA_KEY_TYPE_NONE) {
                return PSA_SUCCESS;
            }
            break;
        case PSA_KEY_DERIVATION_INPUT_LABEL:
        case PSA_KEY_DERIVATION_INPUT_SALT:
        case PSA_KEY_DERIVATION_INPUT_INFO:
        case PSA_KEY_DERIVATION_INPUT_SEED:
            if (key_type == PSA_KEY_TYPE_RAW_DATA) {
                return PSA_SUCCESS;
            }
            if (key_type == PSA_KEY_TYPE_NONE) {
                return PSA_SUCCESS;
            }
            break;
        case PSA_KEY_DERIVATION_INPUT_PASSWORD:
            if (key_type == PSA_KEY_TYPE_PASSWORD) {
                return PSA_SUCCESS;
            }
            if (key_type == PSA_KEY_TYPE_DERIVE) {
                return PSA_SUCCESS;
            }
            if (key_type == PSA_KEY_TYPE_NONE) {
                return PSA_SUCCESS;
            }
            break;
    }
    return PSA_ERROR_INVALID_ARGUMENT;
}

static psa_status_t psa_key_derivation_input_internal(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    psa_key_type_t key_type,
    const uint8_t *data,
    size_t data_length)
{
    psa_status_t status;
    psa_algorithm_t kdf_alg = psa_key_derivation_get_kdf_alg(operation);

    if (kdf_alg == PSA_ALG_NONE) {
                                                   
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    status = psa_key_derivation_check_input_type(step, key_type);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

#if defined(BUILTIN_ALG_ANY_HKDF)
    if (PSA_ALG_IS_ANY_HKDF(kdf_alg)) {
        status = psa_hkdf_input(&operation->ctx.hkdf, kdf_alg,
                                step, data, data_length);
    } else
#endif                           
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF)
    if (PSA_ALG_IS_TLS12_PRF(kdf_alg)) {
        status = psa_tls12_prf_input(&operation->ctx.tls12_prf,
                                     step, data, data_length);
    } else
#endif                                        
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS)
    if (PSA_ALG_IS_TLS12_PSK_TO_MS(kdf_alg)) {
        status = psa_tls12_prf_psk_to_ms_input(&operation->ctx.tls12_prf,
                                               step, data, data_length);
    } else
#endif                                              
#if defined(MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS)
    if (kdf_alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
        status = psa_tls12_ecjpake_to_pms_input(
            &operation->ctx.tls12_ecjpake_to_pms, step, data, data_length);
    } else
#endif                                                   
#if defined(PSA_HAVE_SOFT_PBKDF2)
    if (PSA_ALG_IS_PBKDF2(kdf_alg)) {
        status = psa_pbkdf2_input(&operation->ctx.pbkdf2, kdf_alg,
                                  step, data, data_length);
    } else
#endif                           
    {
                                                                               
        (void) data;
        (void) data_length;
        (void) kdf_alg;
        return PSA_ERROR_BAD_STATE;
    }

exit:
    if (status != PSA_SUCCESS) {
        psa_key_derivation_abort(operation);
    }
    return status;
}

static psa_status_t psa_key_derivation_input_integer_internal(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    uint64_t value)
{
    psa_status_t status;
    psa_algorithm_t kdf_alg = psa_key_derivation_get_kdf_alg(operation);

    if (kdf_alg == PSA_ALG_NONE) {
                                                   
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

#if defined(PSA_HAVE_SOFT_PBKDF2)
    if (PSA_ALG_IS_PBKDF2(kdf_alg)) {
        status = psa_pbkdf2_set_input_cost(
            &operation->ctx.pbkdf2, step, value);
    } else
#endif                           
    {
        (void) step;
        (void) value;
        (void) kdf_alg;
        status = PSA_ERROR_INVALID_ARGUMENT;
    }

exit:
    if (status != PSA_SUCCESS) {
        psa_key_derivation_abort(operation);
    }
    return status;
}

psa_status_t psa_key_derivation_input_bytes(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    const uint8_t *data_external,
    size_t data_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(data_external, data);

    LOCAL_INPUT_ALLOC(data_external, data_length, data);

    status = psa_key_derivation_input_internal(operation, step,
                                               PSA_KEY_TYPE_NONE,
                                               data, data_length);
#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_INPUT_FREE(data_external, data);
    return status;
}

psa_status_t psa_key_derivation_input_integer(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    uint64_t value)
{
    return psa_key_derivation_input_integer_internal(operation, step, value);
}

psa_status_t psa_key_derivation_input_key(
    psa_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    mbedtls_svc_key_id_t key)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;

    status = psa_get_and_lock_transparent_key_slot_with_policy(
        key, &slot, PSA_KEY_USAGE_DERIVE, operation->alg);
    if (status != PSA_SUCCESS) {
        psa_key_derivation_abort(operation);
        return status;
    }

    /* Passing a key object as a SECRET or PASSWORD input unlocks the
     * permission to output to a key object. */
    if (step == PSA_KEY_DERIVATION_INPUT_SECRET ||
        step == PSA_KEY_DERIVATION_INPUT_PASSWORD) {
        operation->can_output_key = 1;
    }

    status = psa_key_derivation_input_internal(operation,
                                               step, slot->attr.type,
                                               slot->key.data,
                                               slot->key.bytes);

    unlock_status = psa_unregister_read_under_mutex(slot);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}



                                                                  
                   
                                                                  

psa_status_t psa_key_agreement_raw_builtin(const psa_key_attributes_t *attributes,
                                           const uint8_t *key_buffer,
                                           size_t key_buffer_size,
                                           psa_algorithm_t alg,
                                           const uint8_t *peer_key,
                                           size_t peer_key_length,
                                           uint8_t *shared_secret,
                                           size_t shared_secret_size,
                                           size_t *shared_secret_length)
{
    switch (alg) {
#if defined(MBEDTLS_PSA_BUILTIN_ALG_ECDH)
        case PSA_ALG_ECDH:
            return mbedtls_psa_key_agreement_ecdh(attributes, key_buffer,
                                                  key_buffer_size, alg,
                                                  peer_key, peer_key_length,
                                                  shared_secret,
                                                  shared_secret_size,
                                                  shared_secret_length);
#endif                                   

#if defined(MBEDTLS_PSA_BUILTIN_ALG_FFDH)
        case PSA_ALG_FFDH:
            return mbedtls_psa_ffdh_key_agreement(attributes,
                                                  peer_key,
                                                  peer_key_length,
                                                  key_buffer,
                                                  key_buffer_size,
                                                  shared_secret,
                                                  shared_secret_size,
                                                  shared_secret_length);
#endif                                   

        default:
            (void) attributes;
            (void) key_buffer;
            (void) key_buffer_size;
            (void) peer_key;
            (void) peer_key_length;
            (void) shared_secret;
            (void) shared_secret_size;
            (void) shared_secret_length;
            return PSA_ERROR_NOT_SUPPORTED;
    }
}

                                           
                                                                   
                                                          
                                                                          
                                    
   
static psa_status_t psa_key_agreement_raw_internal(psa_algorithm_t alg,
                                                   psa_key_slot_t *private_key,
                                                   const uint8_t *peer_key,
                                                   size_t peer_key_length,
                                                   uint8_t *shared_secret,
                                                   size_t shared_secret_size,
                                                   size_t *shared_secret_length)
{
    if (!PSA_ALG_IS_RAW_KEY_AGREEMENT(alg)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return psa_driver_wrapper_key_agreement(&private_key->attr,
                                            private_key->key.data,
                                            private_key->key.bytes, alg,
                                            peer_key, peer_key_length,
                                            shared_secret,
                                            shared_secret_size,
                                            shared_secret_length);
}

                                                                             
                                                                           
   
static psa_status_t psa_key_agreement_internal(psa_key_derivation_operation_t *operation,
                                               psa_key_derivation_step_t step,
                                               psa_key_slot_t *private_key,
                                               const uint8_t *peer_key,
                                               size_t peer_key_length)
{
    psa_status_t status;
    uint8_t shared_secret[PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE] = { 0 };
    size_t shared_secret_length = 0;
    psa_algorithm_t ka_alg = PSA_ALG_KEY_AGREEMENT_GET_BASE(operation->alg);

                                                                        
                 
    status = psa_key_agreement_raw_internal(ka_alg,
                                            private_key,
                                            peer_key, peer_key_length,
                                            shared_secret,
                                            sizeof(shared_secret),
                                            &shared_secret_length);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

                                                                      
                                                                     
                                      
    status = psa_key_derivation_input_internal(operation, step,
                                               PSA_KEY_TYPE_DERIVE,
                                               shared_secret,
                                               shared_secret_length);
exit:
    mbedtls_platform_zeroize(shared_secret, shared_secret_length);
    return status;
}

psa_status_t psa_key_derivation_key_agreement(psa_key_derivation_operation_t *operation,
                                              psa_key_derivation_step_t step,
                                              mbedtls_svc_key_id_t private_key,
                                              const uint8_t *peer_key_external,
                                              size_t peer_key_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot;
    LOCAL_INPUT_DECLARE(peer_key_external, peer_key);

    if (!PSA_ALG_IS_KEY_AGREEMENT(operation->alg)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }
    status = psa_get_and_lock_transparent_key_slot_with_policy(
        private_key, &slot, PSA_KEY_USAGE_DERIVE, operation->alg);
    if (status != PSA_SUCCESS) {
        return status;
    }

    LOCAL_INPUT_ALLOC(peer_key_external, peer_key_length, peer_key);
    status = psa_key_agreement_internal(operation, step,
                                        slot,
                                        peer_key, peer_key_length);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    if (status != PSA_SUCCESS) {
        psa_key_derivation_abort(operation);
    } else {
                                                                          
                                                             
        if (step == PSA_KEY_DERIVATION_INPUT_SECRET) {
            operation->can_output_key = 1;
        }
    }

    unlock_status = psa_unregister_read_under_mutex(slot);
    LOCAL_INPUT_FREE(peer_key_external, peer_key);

    return (status == PSA_SUCCESS) ? unlock_status : status;
}

psa_status_t psa_raw_key_agreement(psa_algorithm_t alg,
                                   mbedtls_svc_key_id_t private_key,
                                   const uint8_t *peer_key_external,
                                   size_t peer_key_length,
                                   uint8_t *output_external,
                                   size_t output_size,
                                   size_t *output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot = NULL;
    size_t expected_length;
    LOCAL_INPUT_DECLARE(peer_key_external, peer_key);
    LOCAL_OUTPUT_DECLARE(output_external, output);
    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    if (!PSA_ALG_IS_KEY_AGREEMENT(alg)) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }
    status = psa_get_and_lock_transparent_key_slot_with_policy(
        private_key, &slot, PSA_KEY_USAGE_DERIVE, alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

                                                                       
                                                                           
                                                                               
                                                                        
                                                                   
                                                                            
    expected_length =
        PSA_RAW_KEY_AGREEMENT_OUTPUT_SIZE(slot->attr.type, slot->attr.bits);
    if (output_size < expected_length) {
        status = PSA_ERROR_BUFFER_TOO_SMALL;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(peer_key_external, peer_key_length, peer_key);
    status = psa_key_agreement_raw_internal(alg, slot,
                                            peer_key, peer_key_length,
                                            output, output_size,
                                            output_length);

exit:
                                                 
                                      
    if (output != NULL && status != PSA_SUCCESS) {
                                                                      
                                                                           
                                                                         
                                                                           
                                                                           
                                                                    
           
        psa_generate_random_internal(output, output_size);
        *output_length = output_size;
    }

    if (output == NULL) {
                                       
        *output_length = 0;
    }

    unlock_status = psa_unregister_read_under_mutex(slot);

    LOCAL_INPUT_FREE(peer_key_external, peer_key);
    LOCAL_OUTPUT_FREE(output_external, output);
    return (status == PSA_SUCCESS) ? unlock_status : status;
}


                                                                  
                       
                                                                  

#if defined(MBEDTLS_PSA_INJECT_ENTROPY)
#include "entropy_poll.h"
#endif

                                        
  
                                                                             
                                         
   
static void mbedtls_psa_random_init(mbedtls_psa_random_context_t *rng)
{
#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
    memset(rng, 0, sizeof(*rng));
#else                                      
    psa_random_internal_init(rng);
#endif                                      
}

                                          
  
                                                                             
                                         
   
static void mbedtls_psa_random_free(mbedtls_psa_random_context_t *rng)
{
#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
    memset(rng, 0, sizeof(*rng));
#else                                      
    psa_random_internal_free(rng);
#endif                                      
}

                                  
   
static psa_status_t mbedtls_psa_random_seed(mbedtls_psa_random_context_t *rng)
{
#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
                                                    
    (void) rng;
    return PSA_SUCCESS;
#else                                      
    return psa_random_internal_seed(rng);
#endif                                      
}

psa_status_t psa_random_reseed(const uint8_t *perso, size_t perso_size)
{
    GUARD_MODULE_INITIALIZED;
#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
    (void) perso;
    (void) perso_size;
    return PSA_ERROR_NOT_SUPPORTED;
#else                                      
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&mbedtls_threading_psa_rngdata_mutex) != 0) {
        return PSA_ERROR_SERVICE_FAILURE;
    }
#endif                                   
    int ret = mbedtls_psa_drbg_reseed(&global_data.rng.drbg,
                                      perso, perso_size);
#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   
    return mbedtls_to_psa_error(ret);
#endif                                      
}

psa_status_t psa_random_deplete(void)
{
    GUARD_MODULE_INITIALIZED;
#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
    return PSA_ERROR_NOT_SUPPORTED;
#else                                      
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&mbedtls_threading_psa_rngdata_mutex) != 0) {
        return PSA_ERROR_SERVICE_FAILURE;
    }
#endif                                   
    mbedtls_psa_drbg_deplete(&global_data.rng.drbg);
#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   
    return PSA_SUCCESS;
#endif                                      
}

psa_status_t psa_random_set_prediction_resistance(unsigned enabled)
{
    GUARD_MODULE_INITIALIZED;

#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
    (void) enabled;
    return PSA_ERROR_NOT_SUPPORTED;
#else                                      

    if (enabled != 0 && enabled != 1) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

#if MBEDTLS_ENTROPY_TRUE_SOURCES > 0
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&mbedtls_threading_psa_rngdata_mutex) != 0) {
        return PSA_ERROR_SERVICE_FAILURE;
    }
#endif                                   
    mbedtls_psa_drbg_set_prediction_resistance(&global_data.rng.drbg, enabled);
#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   
    return PSA_SUCCESS;

#else                                       
    if (enabled) {
        return PSA_ERROR_NOT_SUPPORTED;
    } else {
        return PSA_SUCCESS;
    }

#endif                                       
#endif                                      
}

psa_status_t psa_generate_random(uint8_t *output_external,
                                 size_t output_size)
{
    psa_status_t status;

    LOCAL_OUTPUT_DECLARE(output_external, output);
    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    status = psa_generate_random_internal(output, output_size);

#if !defined(MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS)
exit:
#endif
    LOCAL_OUTPUT_FREE(output_external, output);
    return status;
}

#if defined(MBEDTLS_PSA_INJECT_ENTROPY)
psa_status_t mbedtls_psa_inject_entropy(const uint8_t *seed,
                                        size_t seed_size)
{
    if (psa_get_initialized()) {
        return PSA_ERROR_NOT_PERMITTED;
    }

    if (((seed_size < MBEDTLS_ENTROPY_MIN_PLATFORM) ||
         (seed_size < MBEDTLS_ENTROPY_BLOCK_SIZE)) ||
        (seed_size > MBEDTLS_ENTROPY_MAX_SEED_SIZE)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return mbedtls_psa_storage_inject_entropy(seed, seed_size);
}
#endif                                 

                                                     
  
                             
                                              
  
                       
                                           
                                      
                                                    
                                   
                                                                            
                                    
   
static psa_status_t psa_validate_key_type_and_size_for_key_generation(
    psa_key_type_t type, size_t bits)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (key_type_is_raw_bytes(type)) {
        status = psa_validate_unstructured_key_bit_size(type, bits);
        if (status != PSA_SUCCESS) {
            return status;
        }
    } else
#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
    if (PSA_KEY_TYPE_IS_RSA(type) && PSA_KEY_TYPE_IS_KEY_PAIR(type)) {
        if (bits > PSA_VENDOR_RSA_MAX_KEY_BITS) {
            return PSA_ERROR_NOT_SUPPORTED;
        }
        if (bits < PSA_VENDOR_RSA_GENERATE_MIN_KEY_BITS) {
            return PSA_ERROR_NOT_SUPPORTED;
        }

                                                                 
                                      
        if (bits % 8 != 0) {
            return PSA_ERROR_NOT_SUPPORTED;
        }
    } else
#endif                                                       

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
    if (PSA_KEY_TYPE_IS_ECC(type) && PSA_KEY_TYPE_IS_KEY_PAIR(type)) {
                                                             
        return PSA_SUCCESS;
    } else
#endif                                                       

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_GENERATE)
    if (PSA_KEY_TYPE_IS_DH(type) && PSA_KEY_TYPE_IS_KEY_PAIR(type)) {
        if (psa_is_dh_key_size_valid(bits) == 0) {
            return PSA_ERROR_NOT_SUPPORTED;
        }
    } else
#endif                                                      
    {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t psa_generate_key_internal(
    const psa_key_attributes_t *attributes,
    const psa_custom_key_parameters_t *custom,
    const uint8_t *custom_data,
    size_t custom_data_length,
    uint8_t *key_buffer, size_t key_buffer_size, size_t *key_buffer_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_type_t type = attributes->type;

                           
    (void) custom;
    (void) custom_data;
    (void) custom_data_length;

    if (key_type_is_raw_bytes(type)) {
        status = psa_generate_random_internal(key_buffer, key_buffer_size);
        if (status != PSA_SUCCESS) {
            return status;
        }

#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DES)
        if (type == PSA_KEY_TYPE_DES) {
            psa_des_set_key_parity(key_buffer, key_buffer_size);
        }
#endif                                       
    } else

#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
    if (type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
        return mbedtls_psa_rsa_generate_key(attributes,
                                            custom_data, custom_data_length,
                                            key_buffer,
                                            key_buffer_size,
                                            key_buffer_length);
    } else
#endif                                                                  

#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
    if (PSA_KEY_TYPE_IS_ECC(type) && PSA_KEY_TYPE_IS_KEY_PAIR(type)) {
        return mbedtls_psa_ecp_generate_key(attributes,
                                            key_buffer,
                                            key_buffer_size,
                                            key_buffer_length);
    } else
#endif                                                                  

#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_KEY_PAIR_GENERATE)
    if (PSA_KEY_TYPE_IS_DH(type) && PSA_KEY_TYPE_IS_KEY_PAIR(type)) {
        return mbedtls_psa_ffdh_generate_key(attributes,
                                             key_buffer,
                                             key_buffer_size,
                                             key_buffer_length);
    } else
#endif                                                                 
    {
        (void) key_buffer_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t psa_generate_key_custom(const psa_key_attributes_t *attributes,
                                     const psa_custom_key_parameters_t *custom,
                                     const uint8_t *custom_data,
                                     size_t custom_data_length,
                                     mbedtls_svc_key_id_t *key)
{
    psa_status_t status;
    psa_key_slot_t *slot = NULL;
    psa_se_drv_table_entry_t *driver = NULL;
    size_t key_buffer_size;

    *key = MBEDTLS_SVC_KEY_ID_INIT;

                                                                      
                                                                        
    if (psa_get_key_bits(attributes) == 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

                                                    
    if (PSA_KEY_TYPE_IS_PUBLIC_KEY(attributes->type)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
    if (attributes->type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
        if (custom->flags != 0) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    } else
#endif
    if (!psa_custom_key_parameters_are_default(custom, custom_data_length)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    status = psa_start_key_creation(PSA_KEY_CREATION_GENERATE, attributes,
                                    &slot, &driver);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

                                                                        
                                                                             
                                                                       
                                                    
    if (slot->key.bytes == 0) {
        if (PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime) ==
            PSA_KEY_LOCATION_LOCAL_STORAGE) {
            status = psa_validate_key_type_and_size_for_key_generation(
                attributes->type, attributes->bits);
            if (status != PSA_SUCCESS) {
                goto exit;
            }

            key_buffer_size = PSA_EXPORT_KEY_OUTPUT_SIZE(
                attributes->type,
                attributes->bits);
        } else {
            status = psa_driver_wrapper_get_key_buffer_size(
                attributes, &key_buffer_size);
            if (status != PSA_SUCCESS) {
                goto exit;
            }
        }

        status = psa_allocate_buffer_to_slot(slot, key_buffer_size);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
    }

    status = psa_driver_wrapper_generate_key(attributes,
                                             custom,
                                             custom_data, custom_data_length,
                                             slot->key.data, slot->key.bytes,
                                             &slot->key.bytes);
    if (status != PSA_SUCCESS) {
        psa_remove_key_data_from_memory(slot);
    }

exit:
    if (status == PSA_SUCCESS) {
        status = psa_finish_key_creation(slot, driver, key);
    }
    if (status != PSA_SUCCESS) {
        psa_fail_key_creation(slot, driver);
    }

    return status;
}

psa_status_t psa_generate_key_ext(const psa_key_attributes_t *attributes,
                                  const psa_key_production_parameters_t *params,
                                  size_t params_data_length,
                                  mbedtls_svc_key_id_t *key)
{
    return psa_generate_key_custom(
        attributes,
        (const psa_custom_key_parameters_t *) params,
        params->data, params_data_length,
        key);
}

psa_status_t psa_generate_key(const psa_key_attributes_t *attributes,
                              mbedtls_svc_key_id_t *key)
{
    return psa_generate_key_custom(attributes,
                                   &default_custom_production,
                                   NULL, 0,
                                   key);
}



                                                                  
                  
                                                                  

#if !defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
psa_status_t mbedtls_psa_crypto_configure_entropy_sources(
    void (* entropy_init)(mbedtls_entropy_context *ctx),
    void (* entropy_free)(mbedtls_entropy_context *ctx))
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_lock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   

    if (global_data.rng_state != RNG_NOT_INITIALIZED) {
        status = PSA_ERROR_BAD_STATE;
    } else {
        global_data.rng.entropy_init = entropy_init;
        global_data.rng.entropy_free = entropy_free;
        status = PSA_SUCCESS;
    }

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   

    return status;
}
#endif                                                

void mbedtls_psa_crypto_free(void)
{

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_lock(&mbedtls_threading_psa_globaldata_mutex);
#endif                                   

                                            
    if (global_data.initialized & PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED) {
        global_data.initialized &= ~PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED;
    }

    if (global_data.initialized & PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS_INITIALIZED) {
        psa_wipe_all_key_slots();
        global_data.initialized &= ~PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS_INITIALIZED;
    }

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_globaldata_mutex);
#endif                                   

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_lock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   

    if (global_data.rng_state != RNG_NOT_INITIALIZED) {
        mbedtls_psa_random_free(&global_data.rng);
    }
    global_data.rng_state = RNG_NOT_INITIALIZED;
    mbedtls_platform_zeroize(&global_data.rng, sizeof(global_data.rng));

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_rngdata_mutex);
#endif                                   

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_lock(&mbedtls_threading_psa_globaldata_mutex);
#endif                                   

                           
    if (global_data.initialized & PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED) {
        psa_driver_wrapper_free();
        global_data.initialized &= ~PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED;
    }

#if defined(MBEDTLS_THREADING_C)
    mbedtls_mutex_unlock(&mbedtls_threading_psa_globaldata_mutex);
#endif                                   

}

#if defined(PSA_CRYPTO_STORAGE_HAS_TRANSACTIONS)
                                                                  
  
                                                                          
                                                                         
         
   
static psa_status_t psa_crypto_recover_transaction(
    const psa_crypto_transaction_t *transaction)
{
    switch (transaction->unknown.type) {
        case PSA_CRYPTO_TRANSACTION_CREATE_KEY:
        case PSA_CRYPTO_TRANSACTION_DESTROY_KEY:
                                                             
                          
                                                            
           
        default:
                                                                  
                                                                      
            return PSA_ERROR_DATA_INVALID;
    }
}
#endif                                          

static psa_status_t mbedtls_psa_crypto_init_subsystem(mbedtls_psa_crypto_subsystem subsystem)
{
    psa_status_t status = PSA_SUCCESS;
    uint8_t driver_wrappers_initialized = 0;

    switch (subsystem) {
        case PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS:

#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_lock(&mbedtls_threading_psa_globaldata_mutex));
#endif                                   

            if (!(global_data.initialized & PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED)) {
                                  
                status = psa_driver_wrapper_init();

                                                                         
                global_data.initialized |= PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED;


            }
#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_unlock(
                                            &mbedtls_threading_psa_globaldata_mutex));
#endif                                   

            break;

        case PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS:

#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_lock(&mbedtls_threading_psa_globaldata_mutex));
#endif                                   

            if (!(global_data.initialized & PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS_INITIALIZED)) {
                status = psa_initialize_key_slots();

                                                                     
                global_data.initialized |= PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS_INITIALIZED;

            }
#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_unlock(
                                            &mbedtls_threading_psa_globaldata_mutex));
#endif                                   

            break;

        case PSA_CRYPTO_SUBSYSTEM_RNG:

#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_lock(&mbedtls_threading_psa_globaldata_mutex));
#endif                                   

            driver_wrappers_initialized =
                (global_data.initialized & PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED);

#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_unlock(
                                            &mbedtls_threading_psa_globaldata_mutex));
#endif                                   

                                                                             
                                                                            
                        
#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_lock(&mbedtls_threading_psa_rngdata_mutex));
#endif                                   

                                                           
            if (global_data.rng_state == RNG_NOT_INITIALIZED && driver_wrappers_initialized) {
                mbedtls_psa_random_init(&global_data.rng);
                global_data.rng_state = RNG_INITIALIZED;

                status = mbedtls_psa_random_seed(&global_data.rng);
                if (status == PSA_SUCCESS) {
                    global_data.rng_state = RNG_SEEDED;
                }
            }

#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_unlock(
                                            &mbedtls_threading_psa_rngdata_mutex));
#endif                                   

            break;

        case PSA_CRYPTO_SUBSYSTEM_TRANSACTION:

#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_lock(&mbedtls_threading_psa_globaldata_mutex));
#endif                                   

            if (!(global_data.initialized & PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED)) {
#if defined(PSA_CRYPTO_STORAGE_HAS_TRANSACTIONS)
                status = psa_crypto_load_transaction();
                if (status == PSA_SUCCESS) {
                    status = psa_crypto_recover_transaction(&psa_crypto_transaction);
                    if (status == PSA_SUCCESS) {
                        global_data.initialized |= PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED;
                    }
                    status = psa_crypto_stop_transaction();
                } else if (status == PSA_ERROR_DOES_NOT_EXIST) {
                                                                            
                    global_data.initialized |= PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED;
                    status = PSA_SUCCESS;
                }
#else                                                   
                global_data.initialized |= PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED;
                status = PSA_SUCCESS;
#endif                                                   
            }

#if defined(MBEDTLS_THREADING_C)
            PSA_THREADING_CHK_GOTO_EXIT(mbedtls_mutex_unlock(
                                            &mbedtls_threading_psa_globaldata_mutex));
#endif                                   

            break;

        default:
            status = PSA_ERROR_CORRUPTION_DETECTED;
    }

                                                               
#if defined(MBEDTLS_THREADING_C)
exit:
#endif                                   

    return status;
}

psa_status_t psa_crypto_init(void)
{
    psa_status_t status;

                                                                              
               
    if (psa_get_initialized()) {
        return PSA_SUCCESS;
    }

    status = mbedtls_psa_crypto_init_subsystem(PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = mbedtls_psa_crypto_init_subsystem(PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = mbedtls_psa_crypto_init_subsystem(PSA_CRYPTO_SUBSYSTEM_RNG);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = mbedtls_psa_crypto_init_subsystem(PSA_CRYPTO_SUBSYSTEM_TRANSACTION);

exit:

    if (status != PSA_SUCCESS) {
        mbedtls_psa_crypto_free();
    }

    return status;
}



                                                                  
          
                                                                  

#if defined(PSA_WANT_ALG_SOME_PAKE)
psa_status_t psa_crypto_driver_pake_get_password_len(
    const psa_crypto_driver_pake_inputs_t *inputs,
    size_t *password_len)
{
    if (inputs->password_len == 0) {
        return PSA_ERROR_BAD_STATE;
    }

    *password_len = inputs->password_len;

    return PSA_SUCCESS;
}

psa_status_t psa_crypto_driver_pake_get_password(
    const psa_crypto_driver_pake_inputs_t *inputs,
    uint8_t *buffer, size_t buffer_size, size_t *buffer_length)
{
    if (inputs->password_len == 0) {
        return PSA_ERROR_BAD_STATE;
    }

    if (buffer_size < inputs->password_len) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(buffer, inputs->password, inputs->password_len);
    *buffer_length = inputs->password_len;

    return PSA_SUCCESS;
}

psa_status_t psa_crypto_driver_pake_get_user_len(
    const psa_crypto_driver_pake_inputs_t *inputs,
    size_t *user_len)
{
    if (inputs->user_len == 0) {
        return PSA_ERROR_BAD_STATE;
    }

    *user_len = inputs->user_len;

    return PSA_SUCCESS;
}

psa_status_t psa_crypto_driver_pake_get_user(
    const psa_crypto_driver_pake_inputs_t *inputs,
    uint8_t *user_id, size_t user_id_size, size_t *user_id_len)
{
    if (inputs->user_len == 0) {
        return PSA_ERROR_BAD_STATE;
    }

    if (user_id_size < inputs->user_len) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(user_id, inputs->user, inputs->user_len);
    *user_id_len = inputs->user_len;

    return PSA_SUCCESS;
}

psa_status_t psa_crypto_driver_pake_get_peer_len(
    const psa_crypto_driver_pake_inputs_t *inputs,
    size_t *peer_len)
{
    if (inputs->peer_len == 0) {
        return PSA_ERROR_BAD_STATE;
    }

    *peer_len = inputs->peer_len;

    return PSA_SUCCESS;
}

psa_status_t psa_crypto_driver_pake_get_peer(
    const psa_crypto_driver_pake_inputs_t *inputs,
    uint8_t *peer_id, size_t peer_id_size, size_t *peer_id_length)
{
    if (inputs->peer_len == 0) {
        return PSA_ERROR_BAD_STATE;
    }

    if (peer_id_size < inputs->peer_len) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(peer_id, inputs->peer, inputs->peer_len);
    *peer_id_length = inputs->peer_len;

    return PSA_SUCCESS;
}

psa_status_t psa_crypto_driver_pake_get_cipher_suite(
    const psa_crypto_driver_pake_inputs_t *inputs,
    psa_pake_cipher_suite_t *cipher_suite)
{
    if (inputs->cipher_suite.algorithm == PSA_ALG_NONE) {
        return PSA_ERROR_BAD_STATE;
    }

    *cipher_suite = inputs->cipher_suite;

    return PSA_SUCCESS;
}

psa_status_t psa_pake_setup(
    psa_pake_operation_t *operation,
    const psa_pake_cipher_suite_t *cipher_suite)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (operation->stage != PSA_PAKE_OPERATION_STAGE_SETUP) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (PSA_ALG_IS_PAKE(cipher_suite->algorithm) == 0 ||
        PSA_ALG_IS_HASH(cipher_suite->hash) == 0) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

                                                                      
                                                                       
                                                                     
                                                    
    memset(&operation->data, 0, sizeof(operation->data));

    operation->alg = cipher_suite->algorithm;
    operation->primitive = PSA_PAKE_PRIMITIVE(cipher_suite->type,
                                              cipher_suite->family, cipher_suite->bits);
    operation->data.inputs.cipher_suite = *cipher_suite;

#if defined(PSA_WANT_ALG_JPAKE)
    if (operation->alg == PSA_ALG_JPAKE) {
        psa_jpake_computation_stage_t *computation_stage =
            &operation->computation_stage.jpake;

        memset(computation_stage, 0, sizeof(*computation_stage));
        computation_stage->step = PSA_PAKE_STEP_KEY_SHARE;
    } else
#endif                         
    {
        status = PSA_ERROR_NOT_SUPPORTED;
        goto exit;
    }

    operation->stage = PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS;

    return PSA_SUCCESS;
exit:
    psa_pake_abort(operation);
    return status;
}

psa_status_t psa_pake_set_password_key(
    psa_pake_operation_t *operation,
    mbedtls_svc_key_id_t password)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t unlock_status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_key_slot_t *slot = NULL;
    psa_key_type_t type;

    if (operation->stage != PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    status = psa_get_and_lock_key_slot_with_policy(password, &slot,
                                                   PSA_KEY_USAGE_DERIVE,
                                                   operation->alg);
    if (status != PSA_SUCCESS) {
        goto exit;
    }

    type = psa_get_key_type(&slot->attr);

    if (type != PSA_KEY_TYPE_PASSWORD &&
        type != PSA_KEY_TYPE_PASSWORD_HASH) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    operation->data.inputs.password = mbedtls_calloc(1, slot->key.bytes);
    if (operation->data.inputs.password == NULL) {
        status = PSA_ERROR_INSUFFICIENT_MEMORY;
        goto exit;
    }

    memcpy(operation->data.inputs.password, slot->key.data, slot->key.bytes);
    operation->data.inputs.password_len = slot->key.bytes;
    operation->data.inputs.attributes = slot->attr;

exit:
    if (status != PSA_SUCCESS) {
        psa_pake_abort(operation);
    }
    unlock_status = psa_unregister_read_under_mutex(slot);
    return (status == PSA_SUCCESS) ? unlock_status : status;
}

psa_status_t psa_pake_set_user(
    psa_pake_operation_t *operation,
    const uint8_t *user_id_external,
    size_t user_id_len)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(user_id_external, user_id);

    if (operation->stage != PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (user_id_len == 0) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    if (operation->data.inputs.user_len != 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    operation->data.inputs.user = mbedtls_calloc(1, user_id_len);
    if (operation->data.inputs.user == NULL) {
        status = PSA_ERROR_INSUFFICIENT_MEMORY;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(user_id_external, user_id_len, user_id);

    memcpy(operation->data.inputs.user, user_id, user_id_len);
    operation->data.inputs.user_len = user_id_len;

    status = PSA_SUCCESS;

exit:
    LOCAL_INPUT_FREE(user_id_external, user_id);
    if (status != PSA_SUCCESS) {
        psa_pake_abort(operation);
    }
    return status;
}

psa_status_t psa_pake_set_peer(
    psa_pake_operation_t *operation,
    const uint8_t *peer_id_external,
    size_t peer_id_len)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    LOCAL_INPUT_DECLARE(peer_id_external, peer_id);

    if (operation->stage != PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (peer_id_len == 0) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    if (operation->data.inputs.peer_len != 0) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    operation->data.inputs.peer = mbedtls_calloc(1, peer_id_len);
    if (operation->data.inputs.peer == NULL) {
        status = PSA_ERROR_INSUFFICIENT_MEMORY;
        goto exit;
    }

    LOCAL_INPUT_ALLOC(peer_id_external, peer_id_len, peer_id);

    memcpy(operation->data.inputs.peer, peer_id, peer_id_len);
    operation->data.inputs.peer_len = peer_id_len;

    status = PSA_SUCCESS;

exit:
    LOCAL_INPUT_FREE(peer_id_external, peer_id);
    if (status != PSA_SUCCESS) {
        psa_pake_abort(operation);
    }
    return status;
}

psa_status_t psa_pake_set_role(
    psa_pake_operation_t *operation,
    psa_pake_role_t role)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

    if (operation->stage != PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    switch (operation->alg) {
#if defined(PSA_WANT_ALG_JPAKE)
        case PSA_ALG_JPAKE:
            if (role == PSA_PAKE_ROLE_NONE) {
                return PSA_SUCCESS;
            }
            status = PSA_ERROR_INVALID_ARGUMENT;
            break;
#endif
        default:
            (void) role;
            status = PSA_ERROR_NOT_SUPPORTED;
            goto exit;
    }
exit:
    psa_pake_abort(operation);
    return status;
}

                                                                                 
#if defined(PSA_WANT_ALG_JPAKE)
static psa_crypto_driver_pake_step_t convert_jpake_computation_stage_to_driver_step(
    psa_jpake_computation_stage_t *stage)
{
    psa_crypto_driver_pake_step_t key_share_step;
    if (stage->round == PSA_JPAKE_FIRST) {
        int is_x1;

        if (stage->io_mode == PSA_JPAKE_OUTPUT) {
            is_x1 = (stage->outputs < 1);
        } else {
            is_x1 = (stage->inputs < 1);
        }

        key_share_step = is_x1 ?
                         PSA_JPAKE_X1_STEP_KEY_SHARE :
                         PSA_JPAKE_X2_STEP_KEY_SHARE;
    } else if (stage->round == PSA_JPAKE_SECOND) {
        key_share_step = (stage->io_mode == PSA_JPAKE_OUTPUT) ?
                         PSA_JPAKE_X2S_STEP_KEY_SHARE :
                         PSA_JPAKE_X4S_STEP_KEY_SHARE;
    } else {
        return PSA_JPAKE_STEP_INVALID;
    }
    return (psa_crypto_driver_pake_step_t) (key_share_step + stage->step - PSA_PAKE_STEP_KEY_SHARE);
}
#endif                         

static psa_status_t psa_pake_complete_inputs(
    psa_pake_operation_t *operation)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
                                                                
                                                                    
    psa_crypto_driver_pake_inputs_t inputs = operation->data.inputs;

    if (inputs.password_len == 0) {
        return PSA_ERROR_BAD_STATE;
    }

    if (operation->alg == PSA_ALG_JPAKE) {
        if (inputs.user_len == 0 || inputs.peer_len == 0) {
            return PSA_ERROR_BAD_STATE;
        }
    }

                              
    mbedtls_platform_zeroize(&operation->data, sizeof(operation->data));

    status = psa_driver_wrapper_pake_setup(operation, &inputs);

                                                                          
    mbedtls_zeroize_and_free(inputs.password, inputs.password_len);

                                               
    mbedtls_free(inputs.user);
    mbedtls_free(inputs.peer);

    if (status == PSA_SUCCESS) {
#if defined(PSA_WANT_ALG_JPAKE)
        if (operation->alg == PSA_ALG_JPAKE) {
            operation->stage = PSA_PAKE_OPERATION_STAGE_COMPUTATION;
        } else
#endif                         
        {
            status = PSA_ERROR_NOT_SUPPORTED;
        }
    }
    return status;
}

#if defined(PSA_WANT_ALG_JPAKE)
static psa_status_t psa_jpake_prologue(
    psa_pake_operation_t *operation,
    psa_pake_step_t step,
    psa_jpake_io_mode_t io_mode)
{
    if (step != PSA_PAKE_STEP_KEY_SHARE &&
        step != PSA_PAKE_STEP_ZK_PUBLIC &&
        step != PSA_PAKE_STEP_ZK_PROOF) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    psa_jpake_computation_stage_t *computation_stage =
        &operation->computation_stage.jpake;

    if (computation_stage->round != PSA_JPAKE_FIRST &&
        computation_stage->round != PSA_JPAKE_SECOND) {
        return PSA_ERROR_BAD_STATE;
    }

                                                                       
    if (step != computation_stage->step) {
        return PSA_ERROR_BAD_STATE;
    }

    if (step == PSA_PAKE_STEP_KEY_SHARE &&
        computation_stage->inputs == 0 &&
        computation_stage->outputs == 0) {
                                                                           
                           
        computation_stage->io_mode = io_mode;
    } else if (computation_stage->io_mode != io_mode) {
                                                                            
                                
        return PSA_ERROR_BAD_STATE;
    }

    return PSA_SUCCESS;
}

static psa_status_t psa_jpake_epilogue(
    psa_pake_operation_t *operation,
    psa_jpake_io_mode_t io_mode)
{
    psa_jpake_computation_stage_t *stage =
        &operation->computation_stage.jpake;

    if (stage->step == PSA_PAKE_STEP_ZK_PROOF) {
                                    
        if (io_mode == PSA_JPAKE_INPUT) {
            stage->inputs++;
            if (stage->inputs == PSA_JPAKE_EXPECTED_INPUTS(stage->round)) {
                stage->io_mode = PSA_JPAKE_OUTPUT;
            }
        }
        if (io_mode == PSA_JPAKE_OUTPUT) {
            stage->outputs++;
            if (stage->outputs == PSA_JPAKE_EXPECTED_OUTPUTS(stage->round)) {
                stage->io_mode = PSA_JPAKE_INPUT;
            }
        }
        if (stage->inputs == PSA_JPAKE_EXPECTED_INPUTS(stage->round) &&
            stage->outputs == PSA_JPAKE_EXPECTED_OUTPUTS(stage->round)) {
                                                        
            stage->inputs = 0;
            stage->outputs = 0;
            stage->round++;
        }
        stage->step = PSA_PAKE_STEP_KEY_SHARE;
    } else {
        stage->step++;
    }
    return PSA_SUCCESS;
}

#endif                         

psa_status_t psa_pake_output(
    psa_pake_operation_t *operation,
    psa_pake_step_t step,
    uint8_t *output_external,
    size_t output_size,
    size_t *output_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_crypto_driver_pake_step_t driver_step = PSA_JPAKE_STEP_INVALID;
    LOCAL_OUTPUT_DECLARE(output_external, output);
    *output_length = 0;

    if (operation->stage == PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS) {
        status = psa_pake_complete_inputs(operation);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
    }

    if (operation->stage != PSA_PAKE_OPERATION_STAGE_COMPUTATION) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (output_size == 0) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    switch (operation->alg) {
#if defined(PSA_WANT_ALG_JPAKE)
        case PSA_ALG_JPAKE:
            status = psa_jpake_prologue(operation, step, PSA_JPAKE_OUTPUT);
            if (status != PSA_SUCCESS) {
                goto exit;
            }
            driver_step = convert_jpake_computation_stage_to_driver_step(
                &operation->computation_stage.jpake);
            break;
#endif                         
        default:
            (void) step;
            status = PSA_ERROR_NOT_SUPPORTED;
            goto exit;
    }

    LOCAL_OUTPUT_ALLOC(output_external, output_size, output);

    status = psa_driver_wrapper_pake_output(operation, driver_step,
                                            output, output_size, output_length);

    if (status != PSA_SUCCESS) {
        goto exit;
    }

    switch (operation->alg) {
#if defined(PSA_WANT_ALG_JPAKE)
        case PSA_ALG_JPAKE:
            status = psa_jpake_epilogue(operation, PSA_JPAKE_OUTPUT);
            if (status != PSA_SUCCESS) {
                goto exit;
            }
            break;
#endif                         
        default:
            status = PSA_ERROR_NOT_SUPPORTED;
            goto exit;
    }

exit:
    LOCAL_OUTPUT_FREE(output_external, output);
    if (status != PSA_SUCCESS) {
        psa_pake_abort(operation);
    }
    return status;
}

psa_status_t psa_pake_input(
    psa_pake_operation_t *operation,
    psa_pake_step_t step,
    const uint8_t *input_external,
    size_t input_length)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_crypto_driver_pake_step_t driver_step = PSA_JPAKE_STEP_INVALID;
    const size_t max_input_length = (size_t) PSA_PAKE_INPUT_SIZE(operation->alg,
                                                                 operation->primitive,
                                                                 step);
    LOCAL_INPUT_DECLARE(input_external, input);

    if (operation->stage == PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS) {
        status = psa_pake_complete_inputs(operation);
        if (status != PSA_SUCCESS) {
            goto exit;
        }
    }

    if (operation->stage != PSA_PAKE_OPERATION_STAGE_COMPUTATION) {
        status =  PSA_ERROR_BAD_STATE;
        goto exit;
    }

    if (input_length == 0 || input_length > max_input_length) {
        status = PSA_ERROR_INVALID_ARGUMENT;
        goto exit;
    }

    switch (operation->alg) {
#if defined(PSA_WANT_ALG_JPAKE)
        case PSA_ALG_JPAKE:
            status = psa_jpake_prologue(operation, step, PSA_JPAKE_INPUT);
            if (status != PSA_SUCCESS) {
                goto exit;
            }
            driver_step = convert_jpake_computation_stage_to_driver_step(
                &operation->computation_stage.jpake);
            break;
#endif                         
        default:
            (void) step;
            status = PSA_ERROR_NOT_SUPPORTED;
            goto exit;
    }

    LOCAL_INPUT_ALLOC(input_external, input_length, input);
    status = psa_driver_wrapper_pake_input(operation, driver_step,
                                           input, input_length);

    if (status != PSA_SUCCESS) {
        goto exit;
    }

    switch (operation->alg) {
#if defined(PSA_WANT_ALG_JPAKE)
        case PSA_ALG_JPAKE:
            status = psa_jpake_epilogue(operation, PSA_JPAKE_INPUT);
            if (status != PSA_SUCCESS) {
                goto exit;
            }
            break;
#endif                         
        default:
            status = PSA_ERROR_NOT_SUPPORTED;
            goto exit;
    }

exit:
    LOCAL_INPUT_FREE(input_external, input);
    if (status != PSA_SUCCESS) {
        psa_pake_abort(operation);
    }
    return status;
}

psa_status_t psa_pake_get_implicit_key(
    psa_pake_operation_t *operation,
    psa_key_derivation_operation_t *output)
{
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_status_t abort_status = PSA_ERROR_CORRUPTION_DETECTED;
    uint8_t shared_key[MBEDTLS_PSA_JPAKE_BUFFER_SIZE];
    size_t shared_key_len = 0;

    if (operation->stage != PSA_PAKE_OPERATION_STAGE_COMPUTATION) {
        status = PSA_ERROR_BAD_STATE;
        goto exit;
    }

#if defined(PSA_WANT_ALG_JPAKE)
    if (operation->alg == PSA_ALG_JPAKE) {
        psa_jpake_computation_stage_t *computation_stage =
            &operation->computation_stage.jpake;
        if (computation_stage->round != PSA_JPAKE_FINISHED) {
            status = PSA_ERROR_BAD_STATE;
            goto exit;
        }
    } else
#endif                         
    {
        status = PSA_ERROR_NOT_SUPPORTED;
        goto exit;
    }

    status = psa_driver_wrapper_pake_get_implicit_key(operation,
                                                      shared_key,
                                                      sizeof(shared_key),
                                                      &shared_key_len);

    if (status != PSA_SUCCESS) {
        goto exit;
    }

    status = psa_key_derivation_input_bytes(output,
                                            PSA_KEY_DERIVATION_INPUT_SECRET,
                                            shared_key,
                                            shared_key_len);

    mbedtls_platform_zeroize(shared_key, sizeof(shared_key));
exit:
    abort_status = psa_pake_abort(operation);
    return status == PSA_SUCCESS ? abort_status : status;
}

psa_status_t psa_pake_abort(
    psa_pake_operation_t *operation)
{
    psa_status_t status = PSA_SUCCESS;

    if (operation->stage == PSA_PAKE_OPERATION_STAGE_COMPUTATION) {
        status = psa_driver_wrapper_pake_abort(operation);
    }

    if (operation->stage == PSA_PAKE_OPERATION_STAGE_COLLECT_INPUTS) {
        if (operation->data.inputs.password != NULL) {
            mbedtls_zeroize_and_free(operation->data.inputs.password,
                                     operation->data.inputs.password_len);
        }
        if (operation->data.inputs.user != NULL) {
            mbedtls_free(operation->data.inputs.user);
        }
        if (operation->data.inputs.peer != NULL) {
            mbedtls_free(operation->data.inputs.peer);
        }
    }
    memset(operation, 0, sizeof(psa_pake_operation_t));

    return status;
}
#endif                             

                                                                             
                                                                
                                                                          
                            
#if defined(MBEDTLS_TEST_HOOKS)
void (*psa_input_pre_copy_hook)(const uint8_t *input, size_t input_len) = NULL;
void (*psa_input_post_copy_hook)(const uint8_t *input, size_t input_len) = NULL;
void (*psa_output_pre_copy_hook)(const uint8_t *output, size_t output_len) = NULL;
void (*psa_output_post_copy_hook)(const uint8_t *output, size_t output_len) = NULL;
#endif

                                              
  
                                                        
                                                           
                                                                                         
                                                                
                                                                            
                                       
                                                                            
                                                                         
                                             
   
MBEDTLS_STATIC_TESTABLE
psa_status_t psa_crypto_copy_input(const uint8_t *input, size_t input_len,
                                   uint8_t *input_copy, size_t input_copy_len)
{
    if (input_len > input_copy_len) {
        return PSA_ERROR_CORRUPTION_DETECTED;
    }

#if defined(MBEDTLS_TEST_HOOKS)
    if (psa_input_pre_copy_hook != NULL) {
        psa_input_pre_copy_hook(input, input_len);
    }
#endif

    if (input_len > 0) {
        memcpy(input_copy, input, input_len);
    }

#if defined(MBEDTLS_TEST_HOOKS)
    if (psa_input_post_copy_hook != NULL) {
        psa_input_post_copy_hook(input, input_len);
    }
#endif

    return PSA_SUCCESS;
}

                                                             
  
                                                                                
                                                           
                                                                       
                                                                          
                                                                            
                                       
                                                                   
                                                                           
                                                                      
   
MBEDTLS_STATIC_TESTABLE
psa_status_t psa_crypto_copy_output(const uint8_t *output_copy, size_t output_copy_len,
                                    uint8_t *output, size_t output_len)
{
    if (output_len < output_copy_len) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    }

#if defined(MBEDTLS_TEST_HOOKS)
    if (psa_output_pre_copy_hook != NULL) {
        psa_output_pre_copy_hook(output, output_len);
    }
#endif

    if (output_copy_len > 0) {
        memcpy(output, output_copy, output_copy_len);
    }

#if defined(MBEDTLS_TEST_HOOKS)
    if (psa_output_post_copy_hook != NULL) {
        psa_output_post_copy_hook(output, output_len);
    }
#endif

    return PSA_SUCCESS;
}

psa_status_t psa_crypto_local_input_alloc(const uint8_t *input, size_t input_len,
                                          psa_crypto_local_input_t *local_input)
{
    psa_status_t status;

    *local_input = PSA_CRYPTO_LOCAL_INPUT_INIT;

    if (input_len == 0) {
        return PSA_SUCCESS;
    }

    local_input->buffer = mbedtls_calloc(input_len, 1);
    if (local_input->buffer == NULL) {
                                                                       
                                                                
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    }
                                                                 

    local_input->length = input_len;

    status = psa_crypto_copy_input(input, input_len,
                                   local_input->buffer, local_input->length);
    if (status != PSA_SUCCESS) {
        goto error;
    }

    return PSA_SUCCESS;

error:
    mbedtls_zeroize_and_free(local_input->buffer, local_input->length);
    local_input->buffer = NULL;
    local_input->length = 0;
    return status;
}

void psa_crypto_local_input_free(psa_crypto_local_input_t *local_input)
{
    mbedtls_zeroize_and_free(local_input->buffer, local_input->length);
    local_input->buffer = NULL;
    local_input->length = 0;
}

psa_status_t psa_crypto_local_output_alloc(uint8_t *output, size_t output_len,
                                           psa_crypto_local_output_t *local_output)
{
    *local_output = PSA_CRYPTO_LOCAL_OUTPUT_INIT;

    if (output_len == 0) {
        return PSA_SUCCESS;
    }
    local_output->buffer = mbedtls_calloc(output_len, 1);
    if (local_output->buffer == NULL) {
                                                                       
                                                                
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    }
    local_output->length = output_len;
    local_output->original = output;

    return PSA_SUCCESS;
}

psa_status_t psa_crypto_local_output_free(psa_crypto_local_output_t *local_output)
{
    psa_status_t status;

    if (local_output->buffer == NULL) {
        local_output->length = 0;
        return PSA_SUCCESS;
    }
    if (local_output->original == NULL) {
                                                                   
        return PSA_ERROR_CORRUPTION_DETECTED;
    }

    status = psa_crypto_copy_output(local_output->buffer, local_output->length,
                                    local_output->original, local_output->length);
    if (status != PSA_SUCCESS) {
        return status;
    }

    mbedtls_zeroize_and_free(local_output->buffer, local_output->length);
    local_output->buffer = NULL;
    local_output->length = 0;

    return PSA_SUCCESS;
}

#endif                           
