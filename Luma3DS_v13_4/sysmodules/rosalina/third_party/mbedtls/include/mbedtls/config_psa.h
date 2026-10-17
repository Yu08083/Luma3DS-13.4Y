   
                             
                                                           
  
                                                              
                                                                             
                                                                 
  
                                                                
                                                                
                                                          
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_CONFIG_PSA_H
#define MBEDTLS_CONFIG_PSA_H

#include "psa/crypto_legacy.h"

#include "psa/crypto_adjust_config_synonyms.h"

#include "psa/crypto_adjust_config_dependencies.h"

#include "mbedtls/config_adjust_psa_superset_legacy.h"

#if defined(MBEDTLS_PSA_CRYPTO_CONFIG)

                                                                

                                                       
                                                   
#include "psa/crypto_adjust_config_key_pair_types.h"

#if defined(MBEDTLS_PSA_CRYPTO_C)
                                                                          
                                                                                 
#include "mbedtls/config_adjust_legacy_from_psa.h"
#endif

#else                                

                                                       

#include "mbedtls/config_adjust_psa_from_legacy.h"

                                                                             
                                                                      
#include "psa/crypto_adjust_config_key_pair_types.h"

#endif                                

#if defined(PSA_WANT_ALG_JPAKE)
#define PSA_WANT_ALG_SOME_PAKE 1
#endif

#include "psa/crypto_adjust_auto_enabled.h"

#endif                           
