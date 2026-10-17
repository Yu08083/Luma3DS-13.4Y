   
                                                  
                                                      
  
                                                          
  
                                                    
                                                                            
                                   
                                                                           
              
                                                                              
                                                                            
                                                              
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_ADJUST_KEYPAIR_TYPES_H
#define PSA_CRYPTO_ADJUST_KEYPAIR_TYPES_H

#if !defined(MBEDTLS_CONFIG_FILES_READ)
#error "Do not include psa/crypto_adjust_*.h manually! This can lead to problems, " \
    "up to and including runtime errors such as buffer overflows. " \
    "If you're trying to fix a complaint from check_config.h, just remove " \
    "it from your configuration file: since Mbed TLS 3.0, it is included " \
    "automatically at the right point."
#endif      

                                                                  
                    
                                                                  

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT) || \
    defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT) || \
    defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE) || \
    defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC 1
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT) || \
    defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT) || \
    defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE) || \
    defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC 1
#endif

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_IMPORT) || \
    defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_EXPORT) || \
    defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_GENERATE) || \
    defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_DERIVE)
#define PSA_WANT_KEY_TYPE_DH_KEY_PAIR_BASIC 1
#endif

                                                                  
                                
                                                                  

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY 1
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY 1
#endif

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_DH_PUBLIC_KEY 1
#endif

                                                                  
                         
  
                                                       
                                                                  

                                                                                 
                                                                              
                                                                                
                                                                         
             
                                                                            
                                  
#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT 1
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT 1
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT 1
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT 1
#endif

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_DH_KEY_PAIR_IMPORT 1
#define PSA_WANT_KEY_TYPE_DH_KEY_PAIR_EXPORT 1
#endif

#endif                                        
