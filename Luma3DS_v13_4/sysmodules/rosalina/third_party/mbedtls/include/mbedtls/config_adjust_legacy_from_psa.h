   
                                                
                                                                   
  
                                                          
  
                                                                             
                                                                        
                                                                       
                                                                       
          
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_CONFIG_ADJUST_LEGACY_FROM_PSA_H
#define MBEDTLS_CONFIG_ADJUST_LEGACY_FROM_PSA_H

#if !defined(MBEDTLS_CONFIG_FILES_READ)
#error "Do not include mbedtls/config_adjust_*.h manually! This can lead to problems, " \
    "up to and including runtime errors such as buffer overflows. " \
    "If you're trying to fix a complaint from check_config.h, just remove " \
    "it from your configuration file: since Mbed TLS 3.0, it is included " \
    "automatically at the right point."
#endif      

                                                         
                                                                              
   
#if defined(MBEDTLS_PSA_P256M_DRIVER_ENABLED)
#define MBEDTLS_PSA_ACCEL_ECC_SECP_R1_256
#define MBEDTLS_PSA_ACCEL_ALG_ECDSA
#define MBEDTLS_PSA_ACCEL_ALG_ECDH
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE
#endif

  
                                                                   
                                                                     
  
                                                                         
                                                 
  
                            
                                                                    
                                                                           
                                                                           
  
                                                                           
                                                                         
   

                                            
#if (defined(PSA_WANT_ECC_BRAINPOOL_P_R1_256) && \
    !defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_256)) || \
    (defined(PSA_WANT_ECC_BRAINPOOL_P_R1_384) && \
    !defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_384)) || \
    (defined(PSA_WANT_ECC_BRAINPOOL_P_R1_512) && \
    !defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_512)) || \
    (defined(PSA_WANT_ECC_SECP_R1_192) && !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_192)) || \
    (defined(PSA_WANT_ECC_SECP_R1_224) && !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_224)) || \
    (defined(PSA_WANT_ECC_SECP_R1_256) && !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_256)) || \
    (defined(PSA_WANT_ECC_SECP_R1_384) && !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_384)) || \
    (defined(PSA_WANT_ECC_SECP_R1_521) && !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_521)) || \
    (defined(PSA_WANT_ECC_SECP_K1_192) && !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_K1_192)) || \
    (defined(PSA_WANT_ECC_SECP_K1_256) && !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_K1_256))
#define MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES
#define MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_WEIERSTRASS_CURVES
#endif

#if (defined(PSA_WANT_ECC_MONTGOMERY_255) && !defined(MBEDTLS_PSA_ACCEL_ECC_MONTGOMERY_255)) || \
    (defined(PSA_WANT_ECC_MONTGOMERY_448) && !defined(MBEDTLS_PSA_ACCEL_ECC_MONTGOMERY_448))
#define MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES
#endif

                                          
#if (defined(PSA_WANT_ALG_ECDH) && !defined(MBEDTLS_PSA_ACCEL_ALG_ECDH)) || \
    (defined(PSA_WANT_ALG_ECDSA) && !defined(MBEDTLS_PSA_ACCEL_ALG_ECDSA)) || \
    (defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA) && \
    !defined(MBEDTLS_PSA_ACCEL_ALG_DETERMINISTIC_ECDSA)) || \
    (defined(PSA_WANT_ALG_JPAKE) && !defined(MBEDTLS_PSA_ACCEL_ALG_JPAKE))
#define MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS
#endif

                                               
#if (defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY)) || \
    (defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC))
#define MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES_BASIC
#endif

                                                                         
#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#undef MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_DERIVE
#endif

                                                                             
                     
#if (defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY)) || \
    (defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC)) || \
    (defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT)) || \
    (defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT)) || \
    (defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE)) || \
    (defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_DERIVE))
#define MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES
#endif

                                           
  
                              
                                
                                                        
                                                
   
#if defined(PSA_WANT_ECC_BRAINPOOL_P_R1_256)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_256) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_BRAINPOOL_P_R1_256 1
#define MBEDTLS_ECP_DP_BP256R1_ENABLED
#endif                    
#endif                                      

#if defined(PSA_WANT_ECC_BRAINPOOL_P_R1_384)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_384) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_BRAINPOOL_P_R1_384 1
#define MBEDTLS_ECP_DP_BP384R1_ENABLED
#endif                    
#endif                                      

#if defined(PSA_WANT_ECC_BRAINPOOL_P_R1_512)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_BRAINPOOL_P_R1_512) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_BRAINPOOL_P_R1_512 1
#define MBEDTLS_ECP_DP_BP512R1_ENABLED
#endif                    
#endif                                      

#if defined(PSA_WANT_ECC_MONTGOMERY_255)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_MONTGOMERY_255) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_MONTGOMERY_255 1
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED
#endif                    
#endif                                  

#if defined(PSA_WANT_ECC_MONTGOMERY_448)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_MONTGOMERY_448) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_MONTGOMERY_448 1
#define MBEDTLS_ECP_DP_CURVE448_ENABLED
#endif                    
#endif                                  

#if defined(PSA_WANT_ECC_SECP_R1_192)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_192) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_SECP_R1_192 1
#define MBEDTLS_ECP_DP_SECP192R1_ENABLED
#endif                    
#endif                               

#if defined(PSA_WANT_ECC_SECP_R1_224)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_224) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_SECP_R1_224 1
#define MBEDTLS_ECP_DP_SECP224R1_ENABLED
#endif                    
#endif                               

#if defined(PSA_WANT_ECC_SECP_R1_256)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_256) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_SECP_R1_256 1
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#endif                    
#endif                               

#if defined(PSA_WANT_ECC_SECP_R1_384)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_384) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_SECP_R1_384 1
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#endif                    
#endif                               

#if defined(PSA_WANT_ECC_SECP_R1_521)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_521) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_SECP_R1_521 1
#define MBEDTLS_ECP_DP_SECP521R1_ENABLED
#endif                    
#endif                               

#if defined(PSA_WANT_ECC_SECP_K1_192)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_K1_192) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_SECP_K1_192 1
#define MBEDTLS_ECP_DP_SECP192K1_ENABLED
#endif                    
#endif                               

#if defined(PSA_WANT_ECC_SECP_K1_256)
#if !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_K1_256) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_ECC_SECP_K1_256 1
#define MBEDTLS_ECP_DP_SECP256K1_ENABLED
#endif                    
#endif                               

                                         
  
                            
                                
                                                                          
                                                                           
  
                       
                        
                                                 
                            
   
#if defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_DETERMINISTIC_ECDSA) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_WEIERSTRASS_CURVES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES_BASIC)
#define MBEDTLS_PSA_BUILTIN_ALG_DETERMINISTIC_ECDSA 1
#define MBEDTLS_ECDSA_DETERMINISTIC
#define MBEDTLS_HMAC_DRBG_C
#define MBEDTLS_MD_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#endif                    
#endif                                       

#if defined(PSA_WANT_ALG_ECDH)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_ECDH) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES_BASIC)
#define MBEDTLS_PSA_BUILTIN_ALG_ECDH 1
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECP_C
#define MBEDTLS_BIGNUM_C
#endif                    
#endif                        

#if defined(PSA_WANT_ALG_ECDSA)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_ECDSA) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_WEIERSTRASS_CURVES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES_BASIC)
#define MBEDTLS_PSA_BUILTIN_ALG_ECDSA 1
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#endif                    
#endif                         

#if defined(PSA_WANT_ALG_JPAKE)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_JPAKE) || \
    !defined(MBEDTLS_PSA_ACCEL_ECC_SECP_R1_256) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_KEY_TYPES_BASIC)
#define MBEDTLS_PSA_BUILTIN_PAKE 1
#define MBEDTLS_PSA_BUILTIN_ALG_JPAKE 1
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECJPAKE_C
#endif                    
#endif                         

                                              
  
                                 
                                
                                                     
                                                                        
   
#if defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_PUBLIC_KEY 1
#endif                    
#endif                                       

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_BASIC 1
#endif                    
#endif                                           

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_IMPORT 1
#endif                    
#endif                                            

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_EXPORT 1
#endif                    
#endif                                            

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_GENERATE 1
#endif                    
#endif                                              

                                                                           
#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_DERIVE) || \
    defined(MBEDTLS_PSA_ECC_ACCEL_INCOMPLETE_CURVES)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_DERIVE 1
#endif                    
#endif                                                      

#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_PUBLIC_KEY) || \
    defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_BASIC) || \
    defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_IMPORT) || \
    defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#define MBEDTLS_ECP_LIGHT
#define MBEDTLS_BIGNUM_C
#endif

#if defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_EXPORT) || \
    defined(MBEDTLS_PSA_BUILTIN_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
#define MBEDTLS_ECP_C
#define MBEDTLS_BIGNUM_C
#endif

                        

  
                                                                                
                                                                           
                                                                           
   

                                           
#if (defined(PSA_WANT_DH_RFC7919_2048) && !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_2048)) || \
    (defined(PSA_WANT_DH_RFC7919_3072) && !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_3072)) || \
    (defined(PSA_WANT_DH_RFC7919_4096) && !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_4096)) || \
    (defined(PSA_WANT_DH_RFC7919_6144) && !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_6144)) || \
    (defined(PSA_WANT_DH_RFC7919_8192) && !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_8192))
#define MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_GROUPS
#endif

                                         
#if defined(PSA_WANT_ALG_FFDH) && !defined(MBEDTLS_PSA_ACCEL_ALG_FFDH)
#define MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS
#endif

                                              
#if (defined(PSA_WANT_KEY_TYPE_DH_PUBLIC_KEY) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_PUBLIC_KEY)) || \
    (defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_BASIC) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_KEY_PAIR_BASIC)) || \
    (defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_IMPORT) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_KEY_PAIR_IMPORT)) || \
    (defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_EXPORT) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_KEY_PAIR_EXPORT)) || \
    (defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_GENERATE) && \
    !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_KEY_PAIR_GENERATE))
#define MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_KEY_TYPES
#endif

#if defined(PSA_WANT_DH_RFC7919_2048)
#if !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_2048) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_KEY_TYPES)
#define MBEDTLS_PSA_BUILTIN_DH_RFC7919_2048 1
#endif                                           
#endif                               

#if defined(PSA_WANT_DH_RFC7919_3072)
#if !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_3072) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_KEY_TYPES)
#define MBEDTLS_PSA_BUILTIN_DH_RFC7919_3072 1
#endif                                           
#endif                               

#if defined(PSA_WANT_DH_RFC7919_4096)
#if !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_4096) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_KEY_TYPES)
#define MBEDTLS_PSA_BUILTIN_DH_RFC7919_4096 1
#endif                                           
#endif                               

#if defined(PSA_WANT_DH_RFC7919_6144)
#if !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_6144) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_KEY_TYPES)
#define MBEDTLS_PSA_BUILTIN_DH_RFC7919_6144 1
#endif                                           
#endif                               

#if defined(PSA_WANT_DH_RFC7919_8192)
#if !defined(MBEDTLS_PSA_ACCEL_DH_RFC7919_8192) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_KEY_TYPES)
#define MBEDTLS_PSA_BUILTIN_DH_RFC7919_8192 1
#endif                                           
#endif                               

#if defined(PSA_WANT_ALG_FFDH)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_FFDH) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_GROUPS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_KEY_TYPES)
#define MBEDTLS_PSA_BUILTIN_ALG_FFDH 1
#define MBEDTLS_BIGNUM_C
#endif                                  
#endif                        

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_IMPORT)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_KEY_PAIR_IMPORT) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_GROUPS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_KEY_PAIR_IMPORT 1
#endif                                                     
#endif                                           

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_EXPORT)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_KEY_PAIR_EXPORT) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_GROUPS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_KEY_PAIR_EXPORT 1
#endif                                                     
#endif                                           

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_GENERATE)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_KEY_PAIR_GENERATE)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_KEY_PAIR_GENERATE 1
#endif                                                       
#endif                                             

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_BASIC)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_KEY_PAIR_BASIC) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_GROUPS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_KEY_PAIR_BASIC 1
#endif                                                    
#endif                                          

#if defined(PSA_WANT_KEY_TYPE_DH_PUBLIC_KEY)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DH_PUBLIC_KEY) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_GROUPS) || \
    defined(MBEDTLS_PSA_DH_ACCEL_INCOMPLETE_ALGS)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_DH_PUBLIC_KEY 1
#define MBEDTLS_BIGNUM_C
#endif                                                
#endif                                      

                       

#if defined(PSA_WANT_ALG_HKDF)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_HKDF)
  
                                                                           
                                                 
   
#define MBEDTLS_PSA_BUILTIN_ALG_HKDF 1
#endif                                  
#endif                        

#if defined(PSA_WANT_ALG_HKDF_EXTRACT)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_HKDF_EXTRACT)
  
                                                                           
                                                 
   
#define MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXTRACT 1
#endif                                          
#endif                                

#if defined(PSA_WANT_ALG_HKDF_EXPAND)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_HKDF_EXPAND)
  
                                                                           
                                                 
   
#define MBEDTLS_PSA_BUILTIN_ALG_HKDF_EXPAND 1
#endif                                         
#endif                               

#if defined(PSA_WANT_ALG_HMAC)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_HMAC)
#define MBEDTLS_PSA_BUILTIN_ALG_HMAC 1
#endif                                  
#endif                        

#if defined(PSA_WANT_ALG_MD5) && !defined(MBEDTLS_PSA_ACCEL_ALG_MD5)
#define MBEDTLS_PSA_BUILTIN_ALG_MD5 1
#define MBEDTLS_MD5_C
#endif

#if defined(PSA_WANT_ALG_RIPEMD160) && !defined(MBEDTLS_PSA_ACCEL_ALG_RIPEMD160)
#define MBEDTLS_PSA_BUILTIN_ALG_RIPEMD160 1
#define MBEDTLS_RIPEMD160_C
#endif

#if defined(PSA_WANT_ALG_RSA_OAEP)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_RSA_OAEP)
#define MBEDTLS_PSA_BUILTIN_ALG_RSA_OAEP 1
#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_PKCS1_V21
#endif                                      
#endif                            

#if defined(PSA_WANT_ALG_RSA_PKCS1V15_CRYPT)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_RSA_PKCS1V15_CRYPT)
#define MBEDTLS_PSA_BUILTIN_ALG_RSA_PKCS1V15_CRYPT 1
#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_PKCS1_V15
#endif                                                
#endif                                      

#if defined(PSA_WANT_ALG_RSA_PKCS1V15_SIGN)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_RSA_PKCS1V15_SIGN)
#define MBEDTLS_PSA_BUILTIN_ALG_RSA_PKCS1V15_SIGN 1
#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_PKCS1_V15
#endif                                               
#endif                                     

#if defined(PSA_WANT_ALG_RSA_PSS)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_RSA_PSS)
#define MBEDTLS_PSA_BUILTIN_ALG_RSA_PSS 1
#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_PKCS1_V21
#endif                                     
#endif                           

#if defined(PSA_WANT_ALG_SHA_1) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA_1)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA_1 1
#define MBEDTLS_SHA1_C
#endif

#if defined(PSA_WANT_ALG_SHA_224) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA_224)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA_224 1
#define MBEDTLS_SHA224_C
#endif

#if defined(PSA_WANT_ALG_SHA_256) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA_256)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA_256 1
#define MBEDTLS_SHA256_C
#endif

#if defined(PSA_WANT_ALG_SHA_384) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA_384)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA_384 1
#define MBEDTLS_SHA384_C
#endif

#if defined(PSA_WANT_ALG_SHA_512) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA_512)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA_512 1
#define MBEDTLS_SHA512_C
#endif

#if defined(PSA_WANT_ALG_SHA3_224) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA3_224)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA3_224 1
#define MBEDTLS_SHA3_C
#endif

#if defined(PSA_WANT_ALG_SHA3_256) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA3_256)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA3_256 1
#define MBEDTLS_SHA3_C
#endif

#if defined(PSA_WANT_ALG_SHA3_384) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA3_384)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA3_384 1
#define MBEDTLS_SHA3_C
#endif

#if defined(PSA_WANT_ALG_SHA3_512) && !defined(MBEDTLS_PSA_ACCEL_ALG_SHA3_512)
#define MBEDTLS_PSA_BUILTIN_ALG_SHA3_512 1
#define MBEDTLS_SHA3_C
#endif

#if defined(PSA_WANT_ALG_PBKDF2_HMAC)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_PBKDF2_HMAC)
#define MBEDTLS_PSA_BUILTIN_ALG_PBKDF2_HMAC 1
#define PSA_HAVE_SOFT_PBKDF2_HMAC 1
#endif                                           
#endif                               

#if defined(PSA_WANT_ALG_TLS12_PRF)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_TLS12_PRF)
#define MBEDTLS_PSA_BUILTIN_ALG_TLS12_PRF 1
#endif                                       
#endif                             

#if defined(PSA_WANT_ALG_TLS12_PSK_TO_MS)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_TLS12_PSK_TO_MS)
#define MBEDTLS_PSA_BUILTIN_ALG_TLS12_PSK_TO_MS 1
#endif                                             
#endif                                   

#if defined(PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_TLS12_ECJPAKE_TO_PMS)
#define MBEDTLS_PSA_BUILTIN_ALG_TLS12_ECJPAKE_TO_PMS 1
#endif                                                  
#endif                                        

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_IMPORT)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_KEY_PAIR_IMPORT 1
#endif                                                      
#endif                                            

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_EXPORT)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_KEY_PAIR_EXPORT 1
#endif                                                      
#endif                                            

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_KEY_PAIR_GENERATE 1
#define MBEDTLS_GENPRIME
#endif                                                        
#endif                                              

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_KEY_PAIR_BASIC 1
#endif                                                     
#endif                                           

#if defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_PUBLIC_KEY)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_RSA_PUBLIC_KEY 1
#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#endif                                                 
#endif                                       

                                                             
                                                                     
                                    
#if (defined(PSA_WANT_ALG_CTR) && !defined(MBEDTLS_PSA_ACCEL_ALG_CTR)) || \
    (defined(PSA_WANT_ALG_CFB) && !defined(MBEDTLS_PSA_ACCEL_ALG_CFB)) || \
    (defined(PSA_WANT_ALG_OFB) && !defined(MBEDTLS_PSA_ACCEL_ALG_OFB)) || \
    (defined(PSA_WANT_ALG_ECB_NO_PADDING) && !defined(MBEDTLS_PSA_ACCEL_ALG_ECB_NO_PADDING)) || \
    (defined(PSA_WANT_ALG_CBC_NO_PADDING) && !defined(MBEDTLS_PSA_ACCEL_ALG_CBC_NO_PADDING)) || \
    (defined(PSA_WANT_ALG_CBC_PKCS7) && !defined(MBEDTLS_PSA_ACCEL_ALG_CBC_PKCS7)) || \
    (defined(PSA_WANT_ALG_CMAC) && !defined(MBEDTLS_PSA_ACCEL_ALG_CMAC))
#define PSA_HAVE_SOFT_BLOCK_MODE 1
#endif

#if defined(PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_PBKDF2_AES_CMAC_PRF_128)
#define MBEDTLS_PSA_BUILTIN_ALG_PBKDF2_AES_CMAC_PRF_128 1
#define PSA_HAVE_SOFT_PBKDF2_CMAC 1
#endif                                                     
#endif                                           

#if defined(PSA_WANT_KEY_TYPE_AES)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_AES)
#define PSA_HAVE_SOFT_KEY_TYPE_AES 1
#endif                                      
#if defined(PSA_HAVE_SOFT_KEY_TYPE_AES) || \
    defined(PSA_HAVE_SOFT_BLOCK_MODE)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_AES 1
#define MBEDTLS_AES_C
#endif                                                             
#endif                            

#if defined(PSA_WANT_KEY_TYPE_ARIA)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ARIA)
#define PSA_HAVE_SOFT_KEY_TYPE_ARIA 1
#endif                                       
#if defined(PSA_HAVE_SOFT_KEY_TYPE_ARIA) || \
    defined(PSA_HAVE_SOFT_BLOCK_MODE)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_ARIA 1
#define MBEDTLS_ARIA_C
#endif                                                              
#endif                             

#if defined(PSA_WANT_KEY_TYPE_CAMELLIA)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_CAMELLIA)
#define PSA_HAVE_SOFT_KEY_TYPE_CAMELLIA 1
#endif                                           
#if defined(PSA_HAVE_SOFT_KEY_TYPE_CAMELLIA) || \
    defined(PSA_HAVE_SOFT_BLOCK_MODE)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_CAMELLIA 1
#define MBEDTLS_CAMELLIA_C
#endif                                                                  
#endif                                 

#if defined(PSA_WANT_KEY_TYPE_DES)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_DES)
#define PSA_HAVE_SOFT_KEY_TYPE_DES 1
#endif                                      
#if defined(PSA_HAVE_SOFT_KEY_TYPE_DES) || \
    defined(PSA_HAVE_SOFT_BLOCK_MODE)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_DES 1
#define MBEDTLS_DES_C
#endif                                                            
#endif                            

#if defined(PSA_WANT_ALG_STREAM_CIPHER)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_STREAM_CIPHER)
#define MBEDTLS_PSA_BUILTIN_ALG_STREAM_CIPHER 1
#endif                                          
#endif                                 

#if defined(PSA_WANT_KEY_TYPE_CHACHA20)
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_CHACHA20) || \
    defined(MBEDTLS_PSA_BUILTIN_ALG_STREAM_CIPHER)
#define MBEDTLS_PSA_BUILTIN_KEY_TYPE_CHACHA20 1
#define MBEDTLS_CHACHA20_C
#endif                                          
#endif                                 

                                                            
                                                                
                 
#if defined(PSA_HAVE_SOFT_KEY_TYPE_AES) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_ARIA) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_DES) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_CAMELLIA)
#define PSA_HAVE_SOFT_BLOCK_CIPHER 1
#endif

#if defined(PSA_WANT_ALG_CMAC)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_CMAC) || \
    defined(PSA_HAVE_SOFT_BLOCK_CIPHER)
#define MBEDTLS_PSA_BUILTIN_ALG_CMAC 1
#define MBEDTLS_CMAC_C
#endif                                  
#endif                        

#if defined(PSA_HAVE_SOFT_PBKDF2_HMAC) || \
    defined(PSA_HAVE_SOFT_PBKDF2_CMAC)
#define PSA_HAVE_SOFT_PBKDF2 1
#endif                                                             

#if defined(PSA_WANT_ALG_CTR)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_CTR) || \
    defined(PSA_HAVE_SOFT_BLOCK_CIPHER)
#define MBEDTLS_PSA_BUILTIN_ALG_CTR 1
#define MBEDTLS_CIPHER_MODE_CTR
#endif
#endif                       

#if defined(PSA_WANT_ALG_CFB)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_CFB) || \
    defined(PSA_HAVE_SOFT_BLOCK_CIPHER)
#define MBEDTLS_PSA_BUILTIN_ALG_CFB 1
#define MBEDTLS_CIPHER_MODE_CFB
#endif
#endif                       

#if defined(PSA_WANT_ALG_OFB)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_OFB) || \
    defined(PSA_HAVE_SOFT_BLOCK_CIPHER)
#define MBEDTLS_PSA_BUILTIN_ALG_OFB 1
#define MBEDTLS_CIPHER_MODE_OFB
#endif
#endif                       

#if defined(PSA_WANT_ALG_ECB_NO_PADDING) &&     \
    !defined(MBEDTLS_PSA_ACCEL_ALG_ECB_NO_PADDING)
#define MBEDTLS_PSA_BUILTIN_ALG_ECB_NO_PADDING 1
#endif

#if defined(PSA_WANT_ALG_CBC_NO_PADDING)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_CBC_NO_PADDING) || \
    defined(PSA_HAVE_SOFT_BLOCK_CIPHER)
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_PSA_BUILTIN_ALG_CBC_NO_PADDING 1
#endif
#endif                                  

#if defined(PSA_WANT_ALG_CBC_PKCS7)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_CBC_PKCS7) || \
    defined(PSA_HAVE_SOFT_BLOCK_CIPHER)
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_PSA_BUILTIN_ALG_CBC_PKCS7 1
#define MBEDTLS_CIPHER_PADDING_PKCS7
#endif
#endif                             

#if defined(PSA_WANT_ALG_CCM)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_CCM) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_AES) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_ARIA) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_CAMELLIA)
#define MBEDTLS_PSA_BUILTIN_ALG_CCM 1
#define MBEDTLS_CCM_C
#endif
#endif                       

#if defined(PSA_WANT_ALG_CCM_STAR_NO_TAG)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_CCM_STAR_NO_TAG) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_AES) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_ARIA) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_CAMELLIA)
#define MBEDTLS_PSA_BUILTIN_ALG_CCM_STAR_NO_TAG 1
#define MBEDTLS_CCM_C
#endif
#endif                                   

#if defined(PSA_WANT_ALG_GCM)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_GCM) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_AES) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_ARIA) || \
    defined(PSA_HAVE_SOFT_KEY_TYPE_CAMELLIA)
#define MBEDTLS_PSA_BUILTIN_ALG_GCM 1
#define MBEDTLS_GCM_C
#endif
#endif                       

#if defined(PSA_WANT_ALG_CHACHA20_POLY1305)
#if !defined(MBEDTLS_PSA_ACCEL_ALG_CHACHA20_POLY1305)
#if defined(PSA_WANT_KEY_TYPE_CHACHA20)
#define MBEDTLS_CHACHAPOLY_C
#define MBEDTLS_CHACHA20_C
#define MBEDTLS_POLY1305_C
#define MBEDTLS_PSA_BUILTIN_ALG_CHACHA20_POLY1305 1
#endif                                 
#endif                                               
#endif                                     

#endif                                              
