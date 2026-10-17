   
                            
  
                                                                 
  
                                                                  
                                                   
                                                              
  
                                                                  
                        
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_COMPAT_H
#define PSA_CRYPTO_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

  
                                                                       
                                                                        
                                                                              
                         
   
typedef mbedtls_svc_key_id_t psa_key_handle_t;

#define PSA_KEY_HANDLE_INIT MBEDTLS_SVC_KEY_ID_INIT

                                   
  
                        
  
                                                          
   
static inline int psa_key_handle_is_null(psa_key_handle_t handle)
{
    return mbedtls_svc_key_id_is_null(handle);
}

/** Open a handle to an existing persistent key.
 *
 * Open a handle to a persistent key. A key is persistent if it was created
 * with a lifetime other than #PSA_KEY_LIFETIME_VOLATILE. A persistent key
 * always has a nonzero key identifier, set with psa_set_key_id() when
 * creating the key. Implementations may provide additional pre-provisioned
 * keys that can be opened with psa_open_key(). Such keys have an application
 * key identifier in the vendor range, as documented in the description of
 * #psa_key_id_t.
 *
 * The application must eventually close the handle with psa_close_key() or
 * psa_destroy_key() to release associated resources. If the application dies
 * without calling one of these functions, the implementation should perform
 * the equivalent of a call to psa_close_key().
 *
 * Some implementations permit an application to open the same key multiple
 * times. If this is successful, each call to psa_open_key() will return a
 * different key handle.
 *
 * \note This API is not part of the PSA Cryptography API Release 1.0.0
 * specification. It was defined in the 1.0 Beta 3 version of the
 * specification but was removed in the 1.0.0 released version. This API is
 * kept for the time being to not break applications relying on it. It is not
 * deprecated yet but will be in the near future.
 *
 * \note Applications that rely on opening a key multiple times will not be
 * portable to implementations that only permit a single key handle to be
 * opened. See also :ref:\`key-handles\`.
 *
 *
 * \param key           The persistent identifier of the key.
 * \param[out] handle   On success, a handle to the key.
 *
 * \retval #PSA_SUCCESS
 *         Success. The application can now use the value of `*handle`
 *         to access the key.
 * \retval #PSA_ERROR_INSUFFICIENT_MEMORY
 *         The implementation does not have sufficient resources to open the
 *         key. This can be due to reaching an implementation limit on the
 *         number of open keys, the number of open key handles, or available
 *         memory.
 * \retval #PSA_ERROR_DOES_NOT_EXIST
 *         There is no persistent key with key identifier \p key.
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 *         \p key is not a valid persistent key identifier.
 * \retval #PSA_ERROR_NOT_PERMITTED
 *         The specified key exists, but the application does not have the
 *         permission to access it. Note that this specification does not
 *         define any way to create such a key, but it may be possible
 *         through implementation-specific means.
 * \retval #PSA_ERROR_COMMUNICATION_FAILURE \emptydescription
 * \retval #PSA_ERROR_CORRUPTION_DETECTED \emptydescription
 * \retval #PSA_ERROR_STORAGE_FAILURE \emptydescription
 * \retval #PSA_ERROR_DATA_INVALID \emptydescription
 * \retval #PSA_ERROR_DATA_CORRUPT \emptydescription
 * \retval #PSA_ERROR_BAD_STATE
 *         The library has not been previously initialized by psa_crypto_init().
 *         It is implementation-dependent whether a failure to initialize
 *         results in this error code.
 */
psa_status_t psa_open_key(mbedtls_svc_key_id_t key,
                          psa_key_handle_t *handle);

                       
  
                                                                              
                                                                  
  
                                                                               
                                                                              
                                                                           
                                 
  
                                                                      
                                             
  
                                                                       
                                                                 
                                                                           
                                                                             
                                                 
  
                                                       
                                                                        
                                                                            
                                                                            
  
                                                
                                                                              
  
                       
                                                                  
                                    
                                                    
                                                             
                                                           
                               
                                                                                
                                                                         
                                      
   
psa_status_t psa_close_key(psa_key_handle_t handle);

                          
     
   

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
                                
  
                                              
  
                                                                         
                                               
   
#define PSA_DH_FAMILY_CUSTOM                                            \
    ((psa_dh_family_t) MBEDTLS_DEPRECATED_NUMERIC_CONSTANT(0x7e))

   
                                          
  
                                                                  
                                                         
                                                                          
                               
  
                                                                              
                                                                       
                                        
                                          
  
                                         
                                                     
   
static inline psa_status_t MBEDTLS_DEPRECATED psa_set_key_domain_parameters(
    psa_key_attributes_t *attributes,
    psa_key_type_t type, const uint8_t *data, size_t data_length)
{
    (void) data;
    if (data_length != 0) {
        return PSA_ERROR_NOT_SUPPORTED;
    }
    psa_set_key_type(attributes, type);
    return PSA_SUCCESS;
}

   
                                          
  
                                                                  
                                                                    
                                                           

                                        
                                        
                                        
                                         
  
                                         
   
static inline psa_status_t MBEDTLS_DEPRECATED psa_get_key_domain_parameters(
    const psa_key_attributes_t *attributes,
    uint8_t *data, size_t data_size, size_t *data_length)
{
    (void) attributes;
    (void) data;
    (void) data_size;
    *data_length = 0;
    return PSA_SUCCESS;
}

                                                                
  
   
#define PSA_KEY_DOMAIN_PARAMETERS_SIZE(key_type, key_bits)      \
    MBEDTLS_DEPRECATED_NUMERIC_CONSTANT(1u)
#endif                                 

       

#ifdef __cplusplus
}
#endif

#endif                          
