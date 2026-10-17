  
                                                                  
                                
                                             
   
/*  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */


                          
#include "common.h"
#include "psa_crypto_aead.h"
#include "psa_crypto_cipher.h"
#include "psa_crypto_core.h"
#include "psa_crypto_driver_wrappers_no_static.h"
#include "psa_crypto_hash.h"
#include "psa_crypto_mac.h"
#include "psa_crypto_pake.h"
#include "psa_crypto_rsa.h"

#include "mbedtls/platform.h"
                        

#if defined(MBEDTLS_PSA_CRYPTO_C)

                          
                                            
#if defined(PSA_CRYPTO_DRIVER_TEST)
#include "test/drivers/test_driver.h"

#endif
                                                 
#if defined(PSA_CRYPTO_DRIVER_TEST)
#include "test/drivers/test_driver.h"

#endif
                                         
#if defined(MBEDTLS_PSA_P256M_DRIVER_ENABLED)
#include "../3rdparty/p256-m/p256-m_driver_entrypoints.h"

#endif

                        

                                                                   
                                               
                                                        
                                
#define PSA_CRYPTO_MBED_TLS_DRIVER_ID (1)
#define MBEDTLS_TEST_OPAQUE_DRIVER_ID (2)
#define MBEDTLS_TEST_TRANSPARENT_DRIVER_ID (3)
#define P256_TRANSPARENT_DRIVER_ID (4)

                   

                                    

                                  

                                                  
#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
                                                                            
                                                                            
#ifndef PSA_CRYPTO_DRIVER_PRESENT
#define PSA_CRYPTO_DRIVER_PRESENT
#endif
#include "psa_crypto_se.h"
#endif

                                                                       
                                     
  
                                             
                                                                               
  
                       
                                                                             
                                 
                                   
                                                                            
                                    
                                      
                                                               
   
psa_status_t psa_driver_wrapper_get_key_buffer_size(
    const psa_key_attributes_t *attributes,
    size_t *key_buffer_size )
{
    psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION( psa_get_key_lifetime(attributes) );
    psa_key_type_t key_type = psa_get_key_type(attributes);
    size_t key_bits = psa_get_key_bits(attributes);

    *key_buffer_size = 0;
    switch( location )
    {
#if defined(PSA_CRYPTO_DRIVER_TEST)
        case PSA_CRYPTO_TEST_DRIVER_LOCATION:
#if defined(MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
                                                     
            if( psa_key_id_is_builtin(
                    MBEDTLS_SVC_KEY_ID_GET_KEY_ID(
                        psa_get_key_id( attributes ) ) ) )
            {
                *key_buffer_size = sizeof( psa_drv_slot_number_t );
                return( PSA_SUCCESS );
            }
#endif                                      
            *key_buffer_size = mbedtls_test_opaque_size_function( key_type,
                                                                  key_bits );
            return( ( *key_buffer_size != 0 ) ?
                    PSA_SUCCESS : PSA_ERROR_NOT_SUPPORTED );
#endif                             

        default:
            (void)key_type;
            (void)key_bits;
            return( PSA_ERROR_INVALID_ARGUMENT );
    }
}

psa_status_t psa_driver_wrapper_export_public_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    uint8_t *data, size_t data_size, size_t *data_length )

{

    psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
    psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(
                                      psa_get_key_lifetime( attributes ) );

                                                       
#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
    const psa_drv_se_t *drv;
    psa_drv_se_context_t *drv_context;

    if( psa_get_se_driver( psa_get_key_lifetime(attributes), &drv, &drv_context ) )
    {
        if( ( drv->key_management == NULL ) ||
            ( drv->key_management->p_export_public == NULL ) )
        {
            return( PSA_ERROR_NOT_SUPPORTED );
        }

        return( drv->key_management->p_export_public(
                    drv_context,
                    *( (psa_key_slot_number_t *)key_buffer ),
                    data, data_size, data_length ) );
    }
#endif                              

    switch( location )
    {
        case PSA_KEY_LOCATION_LOCAL_STORAGE:
                                                                     
                                                                  
#if defined(PSA_CRYPTO_ACCELERATOR_DRIVER_PRESENT)

#if (defined(PSA_CRYPTO_DRIVER_TEST) )
            status = mbedtls_test_transparent_export_public_key
                (attributes,
                                key_buffer,
                                key_buffer_size,
                                data,
                                data_size,
                                data_length
            );

            if( status != PSA_ERROR_NOT_SUPPORTED )
                return( status );
#endif

#if (defined(MBEDTLS_PSA_P256M_DRIVER_ENABLED) )
            status = p256_transparent_export_public_key
                (attributes,
                                key_buffer,
                                key_buffer_size,
                                data,
                                data_size,
                                data_length
            );

            if( status != PSA_ERROR_NOT_SUPPORTED )
                return( status );
#endif


#endif                                            
                                                                              
            return( psa_export_public_key_internal( attributes,
                                                    key_buffer,
                                                    key_buffer_size,
                                                    data,
                                                    data_size,
                                                    data_length ) );

                                              
#if defined(PSA_CRYPTO_ACCELERATOR_DRIVER_PRESENT)

#if (defined(PSA_CRYPTO_DRIVER_TEST) )
        case 0x7fffff:
            return( mbedtls_test_opaque_export_public_key
            (attributes,
                            key_buffer,
                            key_buffer_size,
                            data,
                            data_size,
                            data_length
        ));
#endif


#endif                                            
        default:
                                                                 
            return( status );
    }

}

psa_status_t psa_driver_wrapper_get_builtin_key(
    psa_drv_slot_number_t slot_number,
    psa_key_attributes_t *attributes,
    uint8_t *key_buffer, size_t key_buffer_size, size_t *key_buffer_length )
{

    psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION( psa_get_key_lifetime(attributes) );
    switch( location )
    {
#if defined(PSA_CRYPTO_ACCELERATOR_DRIVER_PRESENT)

#if (defined(PSA_CRYPTO_DRIVER_TEST) )
        case 0x7fffff:
            return( mbedtls_test_opaque_get_builtin_key
            (slot_number,
                            attributes,
                            key_buffer,
                            key_buffer_size,
                            key_buffer_length
        ));
#endif


#endif                                            
        default:
            (void) slot_number;
            (void) key_buffer;
            (void) key_buffer_size;
            (void) key_buffer_length;
            return( PSA_ERROR_DOES_NOT_EXIST );
    }

}

#endif                           
