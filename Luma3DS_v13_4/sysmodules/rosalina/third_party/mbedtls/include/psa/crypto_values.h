   
                            
  
                                                                              
  
                                                                  
                                                                    
               
  
                                                                         
                                                                         
                                                                        
               
  
                                                                       
                                                                     
                                                                            
                           
  
                                                     
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_VALUES_H
#define PSA_CRYPTO_VALUES_H
#include "mbedtls/private_access.h"

                               
     
   

                     

                                                                               
                                                                         
                                                                    
                                                               
                                                                    
                                                                       
                                  
  
                        
                                                                       
                                   

                                                                   
                                                                     
                                     
  
               
   

                                             
#define PSA_SUCCESS ((psa_status_t)0)

                                                             
                 
  
                                                                        
                                 
#define PSA_ERROR_GENERIC_ERROR         ((psa_status_t)-132)

                                                           
                          
  
                                                                    
                                                                   
                                                                 
                                                            
#define PSA_ERROR_NOT_SUPPORTED         ((psa_status_t)-134)

                                               
  
                                                                    
                                                                 
                                  
  
                                                              
                                                                
                                                                     
                                                                
                                  
#define PSA_ERROR_NOT_PERMITTED         ((psa_status_t)-133)

                                  
  
                                                                         
                                                     
  
                                                                
                                                              
                                                                
                                                                        
                                                                        
#define PSA_ERROR_BUFFER_TOO_SMALL      ((psa_status_t)-138)

                                          
  
                                                            
                                                        
#define PSA_ERROR_ALREADY_EXISTS        ((psa_status_t)-139)

                                         
  
                                                                      
                            
#define PSA_ERROR_DOES_NOT_EXIST        ((psa_status_t)-140)

                                                                  
  
                                                         
                                                             
                                                      
  
                                                               
                                   
                                                                                  
                 
  
                                                                      
                                                                        
              
#define PSA_ERROR_BAD_STATE             ((psa_status_t)-137)

                                                      
  
                                                                
                                                       
  
                                                                      
                                                                        
           
   
#define PSA_ERROR_INVALID_ARGUMENT      ((psa_status_t)-135)

                                       
  
                                                                     
                                                                        
#define PSA_ERROR_INSUFFICIENT_MEMORY   ((psa_status_t)-141)

                                           
  
                                                                  
                                                                      
                                                                      
                                                                      
                                                             
#define PSA_ERROR_INSUFFICIENT_STORAGE  ((psa_status_t)-142)

                                                                
  
                                                                    
                                                                     
                                                                     
                                                        
  
                                                                
                                                                     
                                                               
                                                                          
                                                                    
                                                                    
                                                                  
   
#define PSA_ERROR_COMMUNICATION_FAILURE ((psa_status_t)-145)

                                                               
  
                                                                  
                                                            
                                                                  
                                                            
                                                            
                                                                      
  
                                                                       
                                                                       
                                   
  
                                                                    
                                                                
                                                                      
                                                                  
                                      
  
                                                              
                                                                   
                                                                      
                                     
#define PSA_ERROR_STORAGE_FAILURE       ((psa_status_t)-146)

                                    
  
                                                                    
            
#define PSA_ERROR_HARDWARE_FAILURE      ((psa_status_t)-147)

                                     
  
                                                                    
                                                                    
                                                                      
                                         
  
                                                                       
                                                                      
                                                                          
                                                                           
                                                             
  
                                                                      
                                                                  
                                                               
                                                                 
                                                                  
                                                                       
                                                                        
                                                                    
                                                                         
                                                           
                                                                 
            
  
                                                                          
                                                                       
                             
#define PSA_ERROR_CORRUPTION_DETECTED    ((psa_status_t)-151)

                                                              
                            
  
                                                                 
                                                                      
                                                                   
                                                                     
                                                                   
                                                                     
                      
  
                                                                            
                                                            
                                                                         
                                                                           
                                                                      
                              
#define PSA_ERROR_INSUFFICIENT_ENTROPY  ((psa_status_t)-148)

                                            
  
                                                               
                                                                    
                                  
  
                                                                         
                                                                         
#define PSA_ERROR_INVALID_SIGNATURE     ((psa_status_t)-149)

                                       
  
                                                                         
                                                                         
                                                                 
                                                                
                                                              
                                                                    
                                            
  
                                                                  
                                                                     
                                                                 
                                            
#define PSA_ERROR_INVALID_PADDING       ((psa_status_t)-150)

                                                                    
                              
#define PSA_ERROR_INSUFFICIENT_DATA     ((psa_status_t)-143)

                                                                       
                                                                  
                               
#define PSA_ERROR_SERVICE_FAILURE       ((psa_status_t)-144)

                                                                   
   
#define PSA_ERROR_INVALID_HANDLE        ((psa_status_t)-136)

                                   
  
                                                                             
                                                                           
         
  
                                                                          
                                                                       
                                                    
                                                           
                                     
                                                   
                                
                                                                     
  
                                                                      
                                                                         
                                   
  
                                                                        
                                    
   
#define PSA_ERROR_DATA_CORRUPT          ((psa_status_t)-152)

                                                               
  
                                                                              
                                                                             
               
  
                                                                               
                                                                              
                                                                     
  
                                                                     
                                                                 
                                                     
   
#define PSA_ERROR_DATA_INVALID          ((psa_status_t)-153)

                                                                         
                                                                               
                                                                            
                                                                      
   
#define PSA_OPERATION_INCOMPLETE           ((psa_status_t)-248)

                 

       

                                                  
     
   

                                                                           
                                                                      
                                                                           
            
   

                              
  
                                            
   
#define PSA_KEY_TYPE_NONE                           ((psa_key_type_t) 0x0000)

                                 
  
                                                         
                                                                             
                                                                             
                                                                               
   
#define PSA_KEY_TYPE_VENDOR_FLAG                    ((psa_key_type_t) 0x8000)

#define PSA_KEY_TYPE_CATEGORY_MASK                  ((psa_key_type_t) 0x7000)
#define PSA_KEY_TYPE_CATEGORY_RAW                   ((psa_key_type_t) 0x1000)
#define PSA_KEY_TYPE_CATEGORY_SYMMETRIC             ((psa_key_type_t) 0x2000)
#define PSA_KEY_TYPE_CATEGORY_PUBLIC_KEY            ((psa_key_type_t) 0x4000)
#define PSA_KEY_TYPE_CATEGORY_KEY_PAIR              ((psa_key_type_t) 0x7000)

#define PSA_KEY_TYPE_CATEGORY_FLAG_PAIR             ((psa_key_type_t) 0x3000)

                                         
  
                                      
   
#define PSA_KEY_TYPE_IS_VENDOR_DEFINED(type) \
    (((type) & PSA_KEY_TYPE_VENDOR_FLAG) != 0)

                                                         
  
                                                         
   
#define PSA_KEY_TYPE_IS_UNSTRUCTURED(type) \
    (((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_RAW || \
     ((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_SYMMETRIC)

                                                                           
#define PSA_KEY_TYPE_IS_ASYMMETRIC(type)                                \
    (((type) & PSA_KEY_TYPE_CATEGORY_MASK                               \
      & ~PSA_KEY_TYPE_CATEGORY_FLAG_PAIR) ==                            \
     PSA_KEY_TYPE_CATEGORY_PUBLIC_KEY)
                                                           
#define PSA_KEY_TYPE_IS_PUBLIC_KEY(type)                                \
    (((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_PUBLIC_KEY)
                                                                           
           
#define PSA_KEY_TYPE_IS_KEY_PAIR(type)                                   \
    (((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_KEY_PAIR)
                                                         
  
                                                                           
  
                                                       
  
                                                    
                                                                 
                                                  
   
#define PSA_KEY_TYPE_KEY_PAIR_OF_PUBLIC_KEY(type)        \
    ((type) | PSA_KEY_TYPE_CATEGORY_FLAG_PAIR)
                                                         
  
                                                                             
  
                                                       
  
                                                      
                                                                 
                                                  
   
#define PSA_KEY_TYPE_PUBLIC_KEY_OF_KEY_PAIR(type)        \
    ((type) & ~PSA_KEY_TYPE_CATEGORY_FLAG_PAIR)

             
  
                                                                       
                                                                             
#define PSA_KEY_TYPE_RAW_DATA                       ((psa_key_type_t) 0x1001)

             
  
                                                                           
            
  
                                                                        
                                                                  
                                                                    
#define PSA_KEY_TYPE_HMAC                           ((psa_key_type_t) 0x1100)

                                
  
                                                                           
                                                 
  
                                                                    
                                                                         
  
                                                                   
                   
   
#define PSA_KEY_TYPE_DERIVE                         ((psa_key_type_t) 0x1200)

                                                                
  
                                                                              
                                                                           
                                                                         
                                                                             
                                                                              
                          
  
                                                                            
                                                                       
                                                                             
                             
  
                                                                             
                                                                              
                         
  
                                                                          
                                                        
   
#define PSA_KEY_TYPE_PASSWORD                       ((psa_key_type_t) 0x1203)

                                                              
  
                                                                   
                                                            
                          
   
#define PSA_KEY_TYPE_PASSWORD_HASH                  ((psa_key_type_t) 0x1205)

                                                                      
  
                                                                   
                                                                       
   
#define PSA_KEY_TYPE_PEPPER                         ((psa_key_type_t) 0x1206)

                                                                          
  
                                                                       
                      
   
#define PSA_KEY_TYPE_AES                            ((psa_key_type_t) 0x2400)

                                                        
                        
#define PSA_KEY_TYPE_ARIA                           ((psa_key_type_t) 0x2406)

                                                                        
  
                                                                            
                         
  
                                                            
                                                                        
                                                                      
   
#define PSA_KEY_TYPE_DES                            ((psa_key_type_t) 0x2301)

                                                        
                            
#define PSA_KEY_TYPE_CAMELLIA                       ((psa_key_type_t) 0x2403)

                                                                               
  
                                                                           
  
                                                                   
                        
  
                                                                            
                                                                        
                                            
   
#define PSA_KEY_TYPE_CHACHA20                       ((psa_key_type_t) 0x2004)

                   
  
                                                         
   
#define PSA_KEY_TYPE_RSA_PUBLIC_KEY                 ((psa_key_type_t) 0x4001)
                                          
  
                                                         
   
#define PSA_KEY_TYPE_RSA_KEY_PAIR                   ((psa_key_type_t) 0x7001)
                                                              
#define PSA_KEY_TYPE_IS_RSA(type)                                       \
    (PSA_KEY_TYPE_PUBLIC_KEY_OF_KEY_PAIR(type) == PSA_KEY_TYPE_RSA_PUBLIC_KEY)

#define PSA_KEY_TYPE_ECC_PUBLIC_KEY_BASE            ((psa_key_type_t) 0x4100)
#define PSA_KEY_TYPE_ECC_KEY_PAIR_BASE              ((psa_key_type_t) 0x7100)
#define PSA_KEY_TYPE_ECC_CURVE_MASK                 ((psa_key_type_t) 0x00ff)
                            
  
                                                                               
                                                                     
                                                                            
  
                                                           
                                                        
   
#define PSA_KEY_TYPE_ECC_KEY_PAIR(curve)         \
    (PSA_KEY_TYPE_ECC_KEY_PAIR_BASE | (curve))
                              
  
                                                                            
                                                                       
                                        
  
                                                           
                                                        
   
#define PSA_KEY_TYPE_ECC_PUBLIC_KEY(curve)              \
    (PSA_KEY_TYPE_ECC_PUBLIC_KEY_BASE | (curve))

                                                                         
#define PSA_KEY_TYPE_IS_ECC(type)                                       \
    ((PSA_KEY_TYPE_PUBLIC_KEY_OF_KEY_PAIR(type) &                        \
      ~PSA_KEY_TYPE_ECC_CURVE_MASK) == PSA_KEY_TYPE_ECC_PUBLIC_KEY_BASE)
                                                        
#define PSA_KEY_TYPE_IS_ECC_KEY_PAIR(type)                               \
    (((type) & ~PSA_KEY_TYPE_ECC_CURVE_MASK) ==                         \
     PSA_KEY_TYPE_ECC_KEY_PAIR_BASE)
                                                          
#define PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(type)                            \
    (((type) & ~PSA_KEY_TYPE_ECC_CURVE_MASK) ==                         \
     PSA_KEY_TYPE_ECC_PUBLIC_KEY_BASE)

                                                         
#define PSA_KEY_TYPE_ECC_GET_FAMILY(type)                        \
    ((psa_ecc_family_t) (PSA_KEY_TYPE_IS_ECC(type) ?             \
                         ((type) & PSA_KEY_TYPE_ECC_CURVE_MASK) : \
                         0))

                                                                        
#define PSA_ECC_FAMILY_IS_WEIERSTRASS(family) ((family & 0xc0) == 0)

                                         
  
                                              
                                   
                                                              
                                                         
                                   
  
                                                                      
  
                                                        
   
#define PSA_ECC_FAMILY_SECP_K1           ((psa_ecc_family_t) 0x17)

                                        
  
                                              
                                                         
                                                              
                                                         
                                   
   
#define PSA_ECC_FAMILY_SECP_R1           ((psa_ecc_family_t) 0x12)
                                                              
#define PSA_ECC_FAMILY_SECP_R2           ((psa_ecc_family_t) 0x1b)

                                          
  
                                              
                                                                    
                                                              
                                                         
                                   
  
                                                            
   
#define PSA_ECC_FAMILY_SECT_K1           ((psa_ecc_family_t) 0x27)

                                         
  
                                              
                                                         
                                                              
                                                         
                                   
  
                                                            
   
#define PSA_ECC_FAMILY_SECT_R1           ((psa_ecc_family_t) 0x22)

                                                    
  
                                             
             
                                                           
                                                         
                                   
  
                                                            
   
#define PSA_ECC_FAMILY_SECT_R2           ((psa_ecc_family_t) 0x2b)

                              
  
                                              
                                                                      
                                                     
                             
  
                                                                       
                        
   
#define PSA_ECC_FAMILY_BRAINPOOL_P_R1    ((psa_ecc_family_t) 0x30)

                            
  
                                                         
                               
                                                                     
                                                                           
                      
                                                                       
                                                                         
   
#define PSA_ECC_FAMILY_MONTGOMERY        ((psa_ecc_family_t) 0x41)

                                                 
  
                                                                            
                                            
                                           
  
                                                              
                                                                             
                   
                                                                  
                                                                           
                 
                                                                                
  
                                                      
   
#define PSA_ECC_FAMILY_TWISTED_EDWARDS   ((psa_ecc_family_t) 0x42)

#define PSA_KEY_TYPE_DH_PUBLIC_KEY_BASE             ((psa_key_type_t) 0x4200)
#define PSA_KEY_TYPE_DH_KEY_PAIR_BASE               ((psa_key_type_t) 0x7200)
#define PSA_KEY_TYPE_DH_GROUP_MASK                  ((psa_key_type_t) 0x00ff)
                            
  
                                                                         
                                                    
   
#define PSA_KEY_TYPE_DH_KEY_PAIR(group)          \
    (PSA_KEY_TYPE_DH_KEY_PAIR_BASE | (group))
                              
  
                                                                         
                                                    
   
#define PSA_KEY_TYPE_DH_PUBLIC_KEY(group)               \
    (PSA_KEY_TYPE_DH_PUBLIC_KEY_BASE | (group))

                                                                        
#define PSA_KEY_TYPE_IS_DH(type)                                        \
    ((PSA_KEY_TYPE_PUBLIC_KEY_OF_KEY_PAIR(type) &                        \
      ~PSA_KEY_TYPE_DH_GROUP_MASK) == PSA_KEY_TYPE_DH_PUBLIC_KEY_BASE)
                                                       
#define PSA_KEY_TYPE_IS_DH_KEY_PAIR(type)                               \
    (((type) & ~PSA_KEY_TYPE_DH_GROUP_MASK) ==                         \
     PSA_KEY_TYPE_DH_KEY_PAIR_BASE)
                                                         
#define PSA_KEY_TYPE_IS_DH_PUBLIC_KEY(type)                            \
    (((type) & ~PSA_KEY_TYPE_DH_GROUP_MASK) ==                         \
     PSA_KEY_TYPE_DH_PUBLIC_KEY_BASE)

                                                        
#define PSA_KEY_TYPE_DH_GET_FAMILY(type)                        \
    ((psa_dh_family_t) (PSA_KEY_TYPE_IS_DH(type) ?              \
                        ((type) & PSA_KEY_TYPE_DH_GROUP_MASK) :  \
                        0))

                                                         
  
                                                                      
                                                                   
                                       
   
#define PSA_DH_FAMILY_RFC7919            ((psa_dh_family_t) 0x03)

#define PSA_GET_KEY_TYPE_BLOCK_SIZE_EXPONENT(type)      \
    (((type) >> 8) & 7)
                                     
  
                                                                  
  
                                                                            
                                                                           
                                
  
                                                                           
                                                     
                                                                         
                                                                         
                                                       
  
                                                                           
  
                                                                
   
#define PSA_BLOCK_CIPHER_BLOCK_LENGTH(type)                                     \
    (((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_SYMMETRIC ? \
     1u << PSA_GET_KEY_TYPE_BLOCK_SIZE_EXPONENT(type) :                         \
        0u)

                                                                       
                                                                      
                                               
   

                                  
  
                                                                               
                                                                              
                                                                            
                                                 
   
#define PSA_ALG_VENDOR_FLAG                     ((psa_algorithm_t) 0x80000000)

#define PSA_ALG_CATEGORY_MASK                   ((psa_algorithm_t) 0x7f000000)
#define PSA_ALG_CATEGORY_HASH                   ((psa_algorithm_t) 0x02000000)
#define PSA_ALG_CATEGORY_MAC                    ((psa_algorithm_t) 0x03000000)
#define PSA_ALG_CATEGORY_CIPHER                 ((psa_algorithm_t) 0x04000000)
#define PSA_ALG_CATEGORY_AEAD                   ((psa_algorithm_t) 0x05000000)
#define PSA_ALG_CATEGORY_SIGN                   ((psa_algorithm_t) 0x06000000)
#define PSA_ALG_CATEGORY_ASYMMETRIC_ENCRYPTION  ((psa_algorithm_t) 0x07000000)
#define PSA_ALG_CATEGORY_KEY_DERIVATION         ((psa_algorithm_t) 0x08000000)
#define PSA_ALG_CATEGORY_KEY_AGREEMENT          ((psa_algorithm_t) 0x09000000)

                                           
  
                                 
   
#define PSA_ALG_IS_VENDOR_DEFINED(alg)                                  \
    (((alg) & PSA_ALG_VENDOR_FLAG) != 0)

                                                        
  
                                                                       
  
                                                        
                                                                           
                                
   
#define PSA_ALG_IS_HASH(alg)                                            \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_HASH)

                                                       
  
                                                                       
  
                                                       
                                                                           
                                
   
#define PSA_ALG_IS_MAC(alg)                                             \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_MAC)

                                                                    
  
                                                                       
  
                                                                    
                                                                           
                                
   
#define PSA_ALG_IS_CIPHER(alg)                                          \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_CIPHER)

                                                                  
                                         
  
                                                                       
  
                                                         
                                                                           
                                
   
#define PSA_ALG_IS_AEAD(alg)                                            \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_AEAD)

                                                                         
                                                
  
                                                                       
  
                                                                         
                                                                           
                                
   
#define PSA_ALG_IS_SIGN(alg)                                            \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_SIGN)

                                                                          
                                                 
  
                                                                       
  
                                                                          
                                                                           
                                
   
#define PSA_ALG_IS_ASYMMETRIC_ENCRYPTION(alg)                           \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_ASYMMETRIC_ENCRYPTION)

                                                                 
  
                                                                       
  
                                                                 
                                                                           
                                
   
#define PSA_ALG_IS_KEY_AGREEMENT(alg)                                   \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_KEY_AGREEMENT)

                                                                  
  
                                                                       
  
                                                                  
                                                                           
                                
   
#define PSA_ALG_IS_KEY_DERIVATION(alg)                                  \
    (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_KEY_DERIVATION)

                                                                          
             
  
                                                                              
                                                                         
                                                            
                                                 
  
                                                                       
  
                                                                          
                                                                            
                                          
   
#define PSA_ALG_IS_KEY_DERIVATION_STRETCHING(alg)                                  \
    (PSA_ALG_IS_KEY_DERIVATION(alg) &&              \
     (alg) & PSA_ALG_KEY_DERIVATION_STRETCHING_FLAG)

                                             
                                                                              
#define PSA_ALG_NONE                            ((psa_algorithm_t)0)
                 

#define PSA_ALG_HASH_MASK                       ((psa_algorithm_t) 0x000000ff)
          
#define PSA_ALG_MD5                             ((psa_algorithm_t) 0x02000003)
                        
#define PSA_ALG_RIPEMD160                       ((psa_algorithm_t) 0x02000004)
           
#define PSA_ALG_SHA_1                           ((psa_algorithm_t) 0x02000005)
               
#define PSA_ALG_SHA_224                         ((psa_algorithm_t) 0x02000008)
               
#define PSA_ALG_SHA_256                         ((psa_algorithm_t) 0x02000009)
               
#define PSA_ALG_SHA_384                         ((psa_algorithm_t) 0x0200000a)
               
#define PSA_ALG_SHA_512                         ((psa_algorithm_t) 0x0200000b)
                   
#define PSA_ALG_SHA_512_224                     ((psa_algorithm_t) 0x0200000c)
                   
#define PSA_ALG_SHA_512_256                     ((psa_algorithm_t) 0x0200000d)
               
#define PSA_ALG_SHA3_224                        ((psa_algorithm_t) 0x02000010)
               
#define PSA_ALG_SHA3_256                        ((psa_algorithm_t) 0x02000011)
               
#define PSA_ALG_SHA3_384                        ((psa_algorithm_t) 0x02000012)
               
#define PSA_ALG_SHA3_512                        ((psa_algorithm_t) 0x02000013)
                                                         
  
                                                                       
                                                                           
                                                                           
   
#define PSA_ALG_SHAKE256_512                    ((psa_algorithm_t) 0x02000015)

                                                                  
  
                                                                       
                                                                    
                                                                  
                                                  
  
                                                                            
                                                                             
                                                  
                                                
                                                                  
        
                                                                                
                                                                             
        
                                     
                                                       
                                                                          
                                                               
        
                                                                 
                                                                 
                                                                  
        
  
                                                                
                                                                     
                                                                  
  
                                                                    
                                                            
   
#define PSA_ALG_ANY_HASH                        ((psa_algorithm_t) 0x020000ff)

#define PSA_ALG_MAC_SUBCATEGORY_MASK            ((psa_algorithm_t) 0x00c00000)
#define PSA_ALG_HMAC_BASE                       ((psa_algorithm_t) 0x03800000)
                                     
  
                                                                
  
                                                                        
                                                               
  
                                                         
                                                                     
                                       
   
#define PSA_ALG_HMAC(hash_alg)                                  \
    (PSA_ALG_HMAC_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))

#define PSA_ALG_HMAC_GET_HASH(hmac_alg)                             \
    (PSA_ALG_CATEGORY_HASH | ((hmac_alg) & PSA_ALG_HASH_MASK))

                                                         
  
                                                                        
  
                                                                       
  
                                                         
                                                                           
                                
   
#define PSA_ALG_IS_HMAC(alg)                                            \
    (((alg) & (PSA_ALG_CATEGORY_MASK | PSA_ALG_MAC_SUBCATEGORY_MASK)) == \
     PSA_ALG_HMAC_BASE)

                                                                
                                                                    
                                                                           
                                                                     
                                                                        
                                                                          
                                                       
#define PSA_ALG_MAC_TRUNCATION_MASK             ((psa_algorithm_t) 0x003f0000)
#define PSA_MAC_TRUNCATION_OFFSET 16

                                                               
                                                                             
                                                                           
                                                                       
                                                                             
                                                                 
#define PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG   ((psa_algorithm_t) 0x00008000)

                                             
  
                                                                  
                                                                  
                                                                  
                                 
  
                                                                        
                                                                        
                                                                       
                                                         
  
                                                                     
                                                                         
                                                                          
                                           
  
                                                                 
                                                                              
                                                                        
                                      
                                                                     
                                                                       
                                                                        
                                                                     
                                          
  
                                                                          
                               
                                                                    
                                                                         
                                                                  
   
#define PSA_ALG_TRUNCATED_MAC(mac_alg, mac_length)              \
    (((mac_alg) & ~(PSA_ALG_MAC_TRUNCATION_MASK |               \
                    PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG)) |   \
     ((mac_length) << PSA_MAC_TRUNCATION_OFFSET & PSA_ALG_MAC_TRUNCATION_MASK))

                                                                      
                 
  
                                                                 
                                                                              
                                                                        
                                      
  
                                                             
                                                                    
                                      
   
#define PSA_ALG_FULL_LENGTH_MAC(mac_alg)                        \
    ((mac_alg) & ~(PSA_ALG_MAC_TRUNCATION_MASK |                \
                   PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG))

                                                 
  
                                                                 
                                                                              
                                 
  
                                                             
                                                                         
                                                                    
                                      
   
#define PSA_MAC_TRUNCATED_LENGTH(mac_alg)                               \
    (((mac_alg) & PSA_ALG_MAC_TRUNCATION_MASK) >> PSA_MAC_TRUNCATION_OFFSET)

                                                               
  
                                                                         
                                                                             
                                                                           
                                  
  
                                                                         
                                                                               
                                                                               
  
                                                                   
                                                                                
                                   
                                                                              
                                                                             
                                                                   
  
                                                                           
                                                   
                                                                          
                                                                             
                                                                    
   
#define PSA_ALG_AT_LEAST_THIS_LENGTH_MAC(mac_alg, min_mac_length)   \
    (PSA_ALG_TRUNCATED_MAC(mac_alg, min_mac_length) |              \
     PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG)

#define PSA_ALG_CIPHER_MAC_BASE                 ((psa_algorithm_t) 0x03c00000)
                                                
  
                                              
                                                             
   
#define PSA_ALG_CBC_MAC                         ((psa_algorithm_t) 0x03c00100)
                                                
#define PSA_ALG_CMAC                            ((psa_algorithm_t) 0x03c00200)

                                                                               
  
                                                                       
  
                                                                               
                                                                           
                                
   
#define PSA_ALG_IS_BLOCK_CIPHER_MAC(alg)                                \
    (((alg) & (PSA_ALG_CATEGORY_MASK | PSA_ALG_MAC_SUBCATEGORY_MASK)) == \
     PSA_ALG_CIPHER_MAC_BASE)

#define PSA_ALG_CIPHER_STREAM_FLAG              ((psa_algorithm_t) 0x00800000)
#define PSA_ALG_CIPHER_FROM_BLOCK_FLAG          ((psa_algorithm_t) 0x00400000)

                                                       
  
                                                                           
                                                                     
              
  
                                                                       
  
                                                                 
                                                                           
                                                                             
   
#define PSA_ALG_IS_STREAM_CIPHER(alg)            \
    (((alg) & (PSA_ALG_CATEGORY_MASK | PSA_ALG_CIPHER_STREAM_FLAG)) == \
     (PSA_ALG_CATEGORY_CIPHER | PSA_ALG_CIPHER_STREAM_FLAG))

                                                        
  
                                                              
                                                               
   
#define PSA_ALG_STREAM_CIPHER                   ((psa_algorithm_t) 0x04800100)

                               
  
                                                             
                                                             
                                                           
                                                                       
   
#define PSA_ALG_CTR                             ((psa_algorithm_t) 0x04c01000)

                               
  
                                                             
   
#define PSA_ALG_CFB                             ((psa_algorithm_t) 0x04c01100)

                               
  
                                                             
   
#define PSA_ALG_OFB                             ((psa_algorithm_t) 0x04c01200)

                        
  
                                                                          
                                                                   
                                                
   
#define PSA_ALG_XTS                             ((psa_algorithm_t) 0x0440ff00)

                                                                           
  
                                                                               
                                                                                
                                                                    
                                                                             
                                                                             
          
  
                                                             
  
                                                                                
                                                         
  
                                                                       
                                                                            
                                              
   
#define PSA_ALG_ECB_NO_PADDING                  ((psa_algorithm_t) 0x04404400)

                                                        
  
                                                             
  
                                                                          
                                                          
   
#define PSA_ALG_CBC_NO_PADDING                  ((psa_algorithm_t) 0x04404000)

                                                           
  
                                                             
  
                                                                      
   
#define PSA_ALG_CBC_PKCS7                       ((psa_algorithm_t) 0x04404100)

#define PSA_ALG_AEAD_FROM_BLOCK_FLAG            ((psa_algorithm_t) 0x00400000)

                                                                      
  
                                                                       
  
                                                                          
                                       
                                                                           
                                
   
#define PSA_ALG_IS_AEAD_ON_BLOCK_CIPHER(alg)    \
    (((alg) & (PSA_ALG_CATEGORY_MASK | PSA_ALG_AEAD_FROM_BLOCK_FLAG)) == \
     (PSA_ALG_CATEGORY_AEAD | PSA_ALG_AEAD_FROM_BLOCK_FLAG))

                                               
  
                                                             
   
#define PSA_ALG_CCM                             ((psa_algorithm_t) 0x05500100)

                                                
  
                                                                         
                                                                           
  
                                                             
  
                                                  
   
#define PSA_ALG_CCM_STAR_NO_TAG                 ((psa_algorithm_t) 0x04c01300)

                                               
  
                                                             
   
#define PSA_ALG_GCM                             ((psa_algorithm_t) 0x05500200)

                                         
  
                                                             
  
                                                                          
                                 
  
                                                                           
   
#define PSA_ALG_CHACHA20_POLY1305               ((psa_algorithm_t) 0x05100500)

                                                                  
                                                                  
                                                          
   
#define PSA_ALG_AEAD_TAG_LENGTH_MASK            ((psa_algorithm_t) 0x003f0000)
#define PSA_AEAD_TAG_LENGTH_OFFSET 16

                                                                 
                                                                              
                                                                           
                                                                       
                                                                           
                                               
#define PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG  ((psa_algorithm_t) 0x00008000)

                                              
  
                                                                  
                                                                         
                                                                        
                     
  
                                                                   
                                                                                
                                 
                                                                          
  
                                                                           
                               
                                                                     
                                                                       
                                                         
   
#define PSA_ALG_AEAD_WITH_SHORTENED_TAG(aead_alg, tag_length)           \
    (((aead_alg) & ~(PSA_ALG_AEAD_TAG_LENGTH_MASK |                     \
                     PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG)) |         \
     ((tag_length) << PSA_AEAD_TAG_LENGTH_OFFSET &                      \
        PSA_ALG_AEAD_TAG_LENGTH_MASK))

                                                         
  
                                                                   
                                                                                
                                 
  
                                                                        
                                                                     
                                       
   
#define PSA_ALG_AEAD_GET_TAG_LENGTH(aead_alg)                           \
    (((aead_alg) & PSA_ALG_AEAD_TAG_LENGTH_MASK) >>                     \
     PSA_AEAD_TAG_LENGTH_OFFSET)

                                                                           
  
                                                                         
                                                               
  
                                                                         
                                                      
   
#define PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(aead_alg)                   \
    (                                                                    \
        PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG_CASE(aead_alg, PSA_ALG_CCM) \
        PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG_CASE(aead_alg, PSA_ALG_GCM) \
        PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG_CASE(aead_alg, PSA_ALG_CHACHA20_POLY1305) \
        0)
#define PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG_CASE(aead_alg, ref)         \
    PSA_ALG_AEAD_WITH_SHORTENED_TAG(aead_alg, 0) ==                      \
    PSA_ALG_AEAD_WITH_SHORTENED_TAG(ref, 0) ?                            \
    ref :

                                                                 
  
                                                                           
                                                                            
                                                                               
                      
  
                                                                         
                                                                               
                                                                               
  
                                                                     
                                                    
                                                                 
                                                                             
                                                                                
                                                              
  
                                                                            
                                                   
                                                                       
                                                                               
                                                                        
   
#define PSA_ALG_AEAD_WITH_AT_LEAST_THIS_LENGTH_TAG(aead_alg, min_tag_length) \
    (PSA_ALG_AEAD_WITH_SHORTENED_TAG(aead_alg, min_tag_length) |            \
     PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG)

#define PSA_ALG_RSA_PKCS1V15_SIGN_BASE          ((psa_algorithm_t) 0x06000200)
                                           
  
                                                   
                                                           
                     
  
                                                                        
                                                               
                                                       
                                                                        
  
                                                                              
                                                                     
                                       
   
#define PSA_ALG_RSA_PKCS1V15_SIGN(hash_alg)                             \
    (PSA_ALG_RSA_PKCS1V15_SIGN_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
                              
  
                                                                  
                                                                
                   
   
#define PSA_ALG_RSA_PKCS1V15_SIGN_RAW PSA_ALG_RSA_PKCS1V15_SIGN_BASE
#define PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)                               \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_RSA_PKCS1V15_SIGN_BASE)

#define PSA_ALG_RSA_PSS_BASE               ((psa_algorithm_t) 0x06000300)
#define PSA_ALG_RSA_PSS_ANY_SALT_BASE      ((psa_algorithm_t) 0x06001300)
                                   
  
                                                   
                                                           
                                                                  
                                                                
                                                                 
                                                                
                                                                 
                           
  
                                                                        
                                                               
                                                       
                                                                        
  
                                                                      
                                                                     
                                       
   
#define PSA_ALG_RSA_PSS(hash_alg)                               \
    (PSA_ALG_RSA_PSS_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))

                                                             
  
                                                                         
                                                                        
             
  
                                                                        
                                                               
                                                       
                                                                        
  
                                                                      
                                                                     
                                       
   
#define PSA_ALG_RSA_PSS_ANY_SALT(hash_alg)                      \
    (PSA_ALG_RSA_PSS_ANY_SALT_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))

                                                                  
  
                                                                           
  
                                                  
                                                      
                                                                
                                                       
                                                                            
                                                                   
   
#define PSA_ALG_IS_RSA_PSS_STANDARD_SALT(alg)                   \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_RSA_PSS_BASE)

                                                             
  
                                                                           
  
                                                  
                                                                    
                                                                
                                                       
                                                                            
                                                                   
   
#define PSA_ALG_IS_RSA_PSS_ANY_SALT(alg)                                \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_RSA_PSS_ANY_SALT_BASE)

                                               
  
                                                                         
                              
  
                                                                           
  
                                                  
                                                        
                                                                    
                                                                
                                                       
                                                                            
                                                                   
   
#define PSA_ALG_IS_RSA_PSS(alg)                                 \
    (PSA_ALG_IS_RSA_PSS_STANDARD_SALT(alg) ||                   \
     PSA_ALG_IS_RSA_PSS_ANY_SALT(alg))

#define PSA_ALG_ECDSA_BASE                      ((psa_algorithm_t) 0x06000600)
                                 
  
                                                            
                                                 
  
                                                                   
                                                                 
                                                                         
                                                                      
                                                      
  
                                                                        
                                                               
                                                       
                                                                        
  
                                                                    
                                                                     
                                       
   
#define PSA_ALG_ECDSA(hash_alg)                                 \
    (PSA_ALG_ECDSA_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
                                    
  
                                                             
                                                                  
                                                               
                                                              
                                                                
                  
   
#define PSA_ALG_ECDSA_ANY PSA_ALG_ECDSA_BASE
#define PSA_ALG_DETERMINISTIC_ECDSA_BASE        ((psa_algorithm_t) 0x06000700)
                                               
  
                                                                        
  
                                                                          
  
                                                                     
                                                                    
                                                 
                                                         
                                                                        
  
                                                                        
                                                               
                                                       
                                                                        
  
                                                                       
                                  
                                                                     
                                       
   
#define PSA_ALG_DETERMINISTIC_ECDSA(hash_alg)                           \
    (PSA_ALG_DETERMINISTIC_ECDSA_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
#define PSA_ALG_ECDSA_DETERMINISTIC_FLAG        ((psa_algorithm_t) 0x00000100)
#define PSA_ALG_IS_ECDSA(alg)                                           \
    (((alg) & ~PSA_ALG_HASH_MASK & ~PSA_ALG_ECDSA_DETERMINISTIC_FLAG) ==  \
     PSA_ALG_ECDSA_BASE)
#define PSA_ALG_ECDSA_IS_DETERMINISTIC(alg)             \
    (((alg) & PSA_ALG_ECDSA_DETERMINISTIC_FLAG) != 0)
#define PSA_ALG_IS_DETERMINISTIC_ECDSA(alg)                             \
    (PSA_ALG_IS_ECDSA(alg) && PSA_ALG_ECDSA_IS_DETERMINISTIC(alg))
#define PSA_ALG_IS_RANDOMIZED_ECDSA(alg)                                \
    (PSA_ALG_IS_ECDSA(alg) && !PSA_ALG_ECDSA_IS_DETERMINISTIC(alg))

                                                                             
                             
  
                                                                          
                                                                     
                                                                         
                                                                    
  
                                                                       
                                                             
                                                                   
                 
                               
                                                  
                                                                 
                 
                             
                                                                    
                     
  
                                                         
                                                                        
                                             
  
                                                                     
                                                                       
                     
   
#define PSA_ALG_PURE_EDDSA                      ((psa_algorithm_t) 0x06000800)

#define PSA_ALG_HASH_EDDSA_BASE                 ((psa_algorithm_t) 0x06000900)
#define PSA_ALG_IS_HASH_EDDSA(alg)              \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_HASH_EDDSA_BASE)

                                                                          
                                            
  
                                                                              
  
                                                      
                             
                          
                                                
  
                                                               
                  
                                            
                                                 
                            
                                                                       
                                               
                                                        
                                                                  
                                                       
   
#define PSA_ALG_ED25519PH                               \
    (PSA_ALG_HASH_EDDSA_BASE | (PSA_ALG_SHA_512 & PSA_ALG_HASH_MASK))

                                                                          
                                           
  
                                                                              
  
                                                    
                           
                                                            
                                                                  
                   
  
                                                               
                  
                                            
                                                                          
                            
                                                                       
                                                    
                                                        
                                                                  
                                                       
   
#define PSA_ALG_ED448PH                                 \
    (PSA_ALG_HASH_EDDSA_BASE | (PSA_ALG_SHAKE256_512 & PSA_ALG_HASH_MASK))

                                                                       
                                                                        
           
#define PSA_ALG_IS_VENDOR_HASH_AND_SIGN(alg) 0

                                                                             
                                              
  
                                                                      
                                                                    
                         
                                                                             
                                                                        
  
                                                                      
  
                                                                       
                                                                        
                                                                    
                                                              
                                          
   
#define PSA_ALG_IS_SIGN_HASH(alg)                                       \
    (PSA_ALG_IS_RSA_PSS(alg) || PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) ||    \
     PSA_ALG_IS_ECDSA(alg) || PSA_ALG_IS_HASH_EDDSA(alg) ||             \
     PSA_ALG_IS_VENDOR_HASH_AND_SIGN(alg))

                                                                             
                                                    
  
                                                                       
  
                                                                       
                                                                              
                                                                             
                                                                            
                                          
   
#define PSA_ALG_IS_SIGN_MESSAGE(alg)                                    \
    (PSA_ALG_IS_SIGN_HASH(alg) || (alg) == PSA_ALG_PURE_EDDSA)

                                                                 
  
                                                                            
                                                                         
                                                                           
                                                                   
                                                                     
                             
  
                                       
                                                            
      
                                                                       
                                                      
      
                                                                        
                                                                              
                                                                        
                                                           
  
                                                                       
  
                                                                 
                                                                           
                                
   
#define PSA_ALG_IS_HASH_AND_SIGN(alg)                                   \
    (PSA_ALG_IS_SIGN_HASH(alg) &&                                       \
     ((alg) & PSA_ALG_HASH_MASK) != 0)

                                                             
  
                                                              
                                                                   
                                                                   
                                                                 
          
  
                                                                     
                                                  
  
                                                                          
                          
                                                                  
                                                   
                                                                     
                                                             
   
#define PSA_ALG_SIGN_GET_HASH(alg)                                     \
    (PSA_ALG_IS_HASH_AND_SIGN(alg) ?                                   \
     ((alg) & PSA_ALG_HASH_MASK) | PSA_ALG_CATEGORY_HASH :             \
     0)

                               
  
                                                                         
                                                                        
                                                                            
                                                    
  
   
#define PSA_ALG_RSA_PKCS1V15_CRYPT              ((psa_algorithm_t) 0x07000200)

#define PSA_ALG_RSA_OAEP_BASE                   ((psa_algorithm_t) 0x07000300)
                        
  
                                                    
                                                           
                                                         
  
                                                                          
                                                                     
                                 
  
                                                                        
                                                                     
                                       
   
#define PSA_ALG_RSA_OAEP(hash_alg)                              \
    (PSA_ALG_RSA_OAEP_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
#define PSA_ALG_IS_RSA_OAEP(alg)                                \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_RSA_OAEP_BASE)
#define PSA_ALG_RSA_OAEP_GET_HASH(alg)                          \
    (PSA_ALG_IS_RSA_OAEP(alg) ?                                 \
     ((alg) & PSA_ALG_HASH_MASK) | PSA_ALG_CATEGORY_HASH :      \
     0)

#define PSA_ALG_HKDF_BASE                       ((psa_algorithm_t) 0x08000100)
                                     
  
                                                                           
  
                                                           
                                                                           
                                                                   
                                                                                   
                                                                                 
                                                                                        
                                                                                 
                               
  
                                                                             
                                                                         
                                                                         
                                                                     
                                                                            
  
                                                                        
                                                               
  
                                                         
                                                                     
                                       
   
#define PSA_ALG_HKDF(hash_alg)                                  \
    (PSA_ALG_HKDF_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
                                                         
  
                                                                         
                                      
  
                                                                       
  
                                                         
                                                                           
                                               
   
#define PSA_ALG_IS_HKDF(alg)                            \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_HKDF_BASE)
#define PSA_ALG_HKDF_GET_HASH(hkdf_alg)                         \
    (PSA_ALG_CATEGORY_HASH | ((hkdf_alg) & PSA_ALG_HASH_MASK))

#define PSA_ALG_HKDF_EXTRACT_BASE                       ((psa_algorithm_t) 0x08000400)
                                             
  
                                                          
                                   
  
                                                           
                                                
                                                                              
                     
                                                                  
                                      
  
                                                                          
                                                                        
                                                                      
                                                                    
                                                                     
                           
  
                                                                             
                                                                         
                                                                         
                                                                     
                                                                            
  
                                                                        
                                                               
  
                                                                 
                                                                     
                                       
   
#define PSA_ALG_HKDF_EXTRACT(hash_alg)                                  \
    (PSA_ALG_HKDF_EXTRACT_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
                                                                 
  
                                                                       
                                                
  
                                                                       
  
                                                                 
                                                                           
                                               
   
#define PSA_ALG_IS_HKDF_EXTRACT(alg)                            \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_HKDF_EXTRACT_BASE)

#define PSA_ALG_HKDF_EXPAND_BASE                       ((psa_algorithm_t) 0x08000500)
                                            
  
                                                         
                                  
  
                                                           
                                                                    
                                                       
  
                                                                   
                                       
  
                                                                           
                                                                            
                                                                            
                                                                           
                                                                             
  
                                                                        
                                                               
  
                                                                
                                                                     
                                       
   
#define PSA_ALG_HKDF_EXPAND(hash_alg)                                  \
    (PSA_ALG_HKDF_EXPAND_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))
                                                                
  
                                                                      
                                                
  
                                                                       
  
                                                                
                                                                           
                                               
   
#define PSA_ALG_IS_HKDF_EXPAND(alg)                            \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_HKDF_EXPAND_BASE)

                                                                 
                          
  
  
                                                                       
  
                                                               
                                                                           
                                               
   
#define PSA_ALG_IS_ANY_HKDF(alg)                                   \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_HKDF_BASE ||          \
     ((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_HKDF_EXTRACT_BASE ||  \
     ((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_HKDF_EXPAND_BASE)

#define PSA_ALG_TLS12_PRF_BASE                  ((psa_algorithm_t) 0x08000200)
                                           
  
                                                                      
                                                                     
                                       
  
                                                                         
                                  
                                                
                                                        
                                                  
  
                                                                
                                                            
                                    
  
                                                                   
                                  
  
                                                                        
                                                               
  
                                                                
                                                                     
                                       
   
#define PSA_ALG_TLS12_PRF(hash_alg)                                  \
    (PSA_ALG_TLS12_PRF_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))

                                                               
  
                                                                       
  
                                                               
                                                                           
                                               
   
#define PSA_ALG_IS_TLS12_PRF(alg)                                    \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_TLS12_PRF_BASE)
#define PSA_ALG_TLS12_PRF_GET_HASH(hkdf_alg)                         \
    (PSA_ALG_CATEGORY_HASH | ((hkdf_alg) & PSA_ALG_HASH_MASK))

#define PSA_ALG_TLS12_PSK_TO_MS_BASE            ((psa_algorithm_t) 0x08000300)
                                                           
  
                                                                   
                                                                 
                                                                   
                                                                  
              
  
                                                                         
                                  
                                                
                                                                       
                                                                 
                                                                           
                               
                                                        
                                                  
  
                                                     
                                                            
                                           
                                                               
                                                                              
                                                             
                                          
                                                                  
                                                                        
                                                                        
                                                                      
                             
                                                                     
                                                                   
                                                                         
                                                                         
                                                                        
                                                                       
                                                                   
                                                                      
                                                                         
                                                          
                                                                       
                                             
  
                                                                         
                                                                 
  
                                                                        
                                                               
  
                                                                      
                                                                     
                                       
   
#define PSA_ALG_TLS12_PSK_TO_MS(hash_alg)                                  \
    (PSA_ALG_TLS12_PSK_TO_MS_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))

                                                                     
  
                                                                       
  
                                                                     
                                                                           
                                               
   
#define PSA_ALG_IS_TLS12_PSK_TO_MS(alg)                                    \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_TLS12_PSK_TO_MS_BASE)
#define PSA_ALG_TLS12_PSK_TO_MS_GET_HASH(hkdf_alg)                         \
    (PSA_ALG_CATEGORY_HASH | ((hkdf_alg) & PSA_ALG_HASH_MASK))

                                                                            
                                                                            
                                                                 
                                                                                 
                        
                                      
                                                                          
                                                              
                                                                   
                                                                      
                                      
   
#define PSA_ALG_TLS12_ECJPAKE_TO_PMS            ((psa_algorithm_t) 0x08000609)

                                                                           
                                                                          
                                                                          
                                                                             
  
                                                                      
   
#define PSA_ALG_KEY_DERIVATION_STRETCHING_FLAG  ((psa_algorithm_t) 0x00800000)

#define PSA_ALG_PBKDF2_HMAC_BASE                ((psa_algorithm_t) 0x08800100)
                                                                             
  
                                                                      
                                                                             
                                
                                                                       
                              
  
                                                                         
                                   
                                                           
                                               
                                                
                                                                               
                                                                          
                                                                          
                                                                     
                                               
  
                                                                        
                                                               
  
                                                                    
                                                                     
                                       
   
#define PSA_ALG_PBKDF2_HMAC(hash_alg)                                  \
    (PSA_ALG_PBKDF2_HMAC_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))

                                                               
  
                                                                       
  
                                                               
                                                                           
                                               
   
#define PSA_ALG_IS_PBKDF2_HMAC(alg)                                    \
    (((alg) & ~PSA_ALG_HASH_MASK) == PSA_ALG_PBKDF2_HMAC_BASE)
#define PSA_ALG_PBKDF2_HMAC_GET_HASH(pbkdf2_alg)                         \
    (PSA_ALG_CATEGORY_HASH | ((pbkdf2_alg) & PSA_ALG_HASH_MASK))
                                                                            
  
                                                                      
                                                                  
                                              
  
                                                        
                                                    
   
#define PSA_ALG_PBKDF2_AES_CMAC_PRF_128         ((psa_algorithm_t) 0x08800200)

#define PSA_ALG_IS_PBKDF2(kdf_alg)                                      \
    (PSA_ALG_IS_PBKDF2_HMAC(kdf_alg) || \
     ((kdf_alg) == PSA_ALG_PBKDF2_AES_CMAC_PRF_128))

#define PSA_ALG_KEY_DERIVATION_MASK             ((psa_algorithm_t) 0xfe00ffff)
#define PSA_ALG_KEY_AGREEMENT_MASK              ((psa_algorithm_t) 0xffff0000)

                                                                        
                    
  
                                                                            
                                                                           
                                                                             
                                                                             
  
                                                                      
                                  
                                                                   
                                                                      
                                                           
   
#define PSA_ALG_KEY_AGREEMENT(ka_alg, kdf_alg)  \
    ((ka_alg) | (kdf_alg))

#define PSA_ALG_KEY_AGREEMENT_GET_KDF(alg)                              \
    (((alg) & PSA_ALG_KEY_DERIVATION_MASK) | PSA_ALG_CATEGORY_KEY_DERIVATION)

#define PSA_ALG_KEY_AGREEMENT_GET_BASE(alg)                             \
    (((alg) & PSA_ALG_KEY_AGREEMENT_MASK) | PSA_ALG_CATEGORY_KEY_AGREEMENT)

                                                                     
  
                                                             
                             
                                                                      
                                                                    
                                             
  
                                                                       
  
                                                                     
                                                                           
                                
   
#define PSA_ALG_IS_RAW_KEY_AGREEMENT(alg)                               \
    (PSA_ALG_IS_KEY_AGREEMENT(alg) &&                                   \
     PSA_ALG_KEY_AGREEMENT_GET_KDF(alg) == PSA_ALG_CATEGORY_KEY_DERIVATION)

#define PSA_ALG_IS_KEY_DERIVATION_OR_AGREEMENT(alg)     \
    ((PSA_ALG_IS_KEY_DERIVATION(alg) || PSA_ALG_IS_KEY_AGREEMENT(alg)))

                                                                 
  
                                                 
                                 
                                                                           
           
   
#define PSA_ALG_FFDH                            ((psa_algorithm_t) 0x09010000)

                                                                               
  
                                                                         
                                                                       
             
  
                                                                       
  
                                                                               
                                                                           
                                              
   
#define PSA_ALG_IS_FFDH(alg) \
    (PSA_ALG_KEY_AGREEMENT_GET_BASE(alg) == PSA_ALG_FFDH)

                                                                     
  
                                                                     
                                                                          
                                                                          
                                                                          
                                                                    
                                                                   
                                             
  
                                                                   
                                                                 
                                 
                                                             
                                                          
                                                                   
                                                                 
                              
                                                                 
                                                           
                               
                                                                 
                              
                                                 
   
#define PSA_ALG_ECDH                            ((psa_algorithm_t) 0x09020000)

                                                                       
             
  
                                                                           
                                                                         
             
  
                                                                       
  
                                                                     
                       
                                                                           
                                              
   
#define PSA_ALG_IS_ECDH(alg) \
    (PSA_ALG_KEY_AGREEMENT_GET_BASE(alg) == PSA_ALG_ECDH)

                                                           
  
                                                                       
                                         
  
                                                                       
  
                                                        
                                                                         
                         
                                                                           
                                
   
#define PSA_ALG_IS_WILDCARD(alg)                            \
    (PSA_ALG_IS_HASH_AND_SIGN(alg) ?                        \
     PSA_ALG_SIGN_GET_HASH(alg) == PSA_ALG_ANY_HASH :       \
     PSA_ALG_IS_MAC(alg) ?                                  \
     (alg & PSA_ALG_MAC_AT_LEAST_THIS_LENGTH_FLAG) != 0 :   \
     PSA_ALG_IS_AEAD(alg) ?                                 \
     (alg & PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG) != 0 :  \
     (alg) == PSA_ALG_ANY_HASH)

                                               
  
                                                                       
  
                                                                             
                         
  
                                                                     
   
#define PSA_ALG_GET_HASH(alg) \
    (((alg) & 0x000000ff) == 0 ? ((psa_algorithm_t) 0) : 0x02000000 | ((alg) & 0x000000ff))

       

                                         
     
   

                                                                      
                                                                        
                                                                   
   

                                           
  
                                                                               
                                                                
  
                                                                      
                                                                  
                                                                           
                                                                          
                                                      
   
#define PSA_KEY_LIFETIME_VOLATILE               ((psa_key_lifetime_t) 0x00000000)

                                             
  
                                                                          
                                                                         
                                                                        
                                                                       
                                                                        
  
                                                                  
                                                                                
                                                 
   
#define PSA_KEY_LIFETIME_PERSISTENT             ((psa_key_lifetime_t) 0x00000001)

                                           
  
                                                    
   
#define PSA_KEY_PERSISTENCE_VOLATILE            ((psa_key_persistence_t) 0x00)

                                                      
  
                                                    
   
#define PSA_KEY_PERSISTENCE_DEFAULT             ((psa_key_persistence_t) 0x01)

                                                                 
  
                                                    
   
#define PSA_KEY_PERSISTENCE_READ_ONLY           ((psa_key_persistence_t) 0xff)

#define PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime)      \
    ((psa_key_persistence_t) ((lifetime) & 0x000000ff))

#define PSA_KEY_LIFETIME_GET_LOCATION(lifetime)      \
    ((psa_key_location_t) ((lifetime) >> 8))

                                                              
  
                                                                       
                                                                     
                                                             
  
                                                                
                                                                       
                                                                       
                    
  
                                                                  
                                              
  
                                                       
   
#define PSA_KEY_LIFETIME_IS_VOLATILE(lifetime)  \
    (PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime) == \
     PSA_KEY_PERSISTENCE_VOLATILE)

                                                               
  
                                                                            
                                                                            
  
                                                                        
                                                                 
                                                                        
                                                                           
                                                                    
                                         
  
                                                                  
                                              
  
                                                        
   
#define PSA_KEY_LIFETIME_IS_READ_ONLY(lifetime)  \
    (PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime) == \
     PSA_KEY_PERSISTENCE_READ_ONLY)

                                                                 
  
                                             
                                                                
                                              
                                                             
  
                                          
   
#define PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(persistence, location) \
    ((location) << 8 | (persistence))

                                               
  
                                                                          
                                                                        
  
                                                 
   
#define PSA_KEY_LOCATION_LOCAL_STORAGE          ((psa_key_location_t) 0x000000)

#define PSA_KEY_LOCATION_VENDOR_FLAG            ((psa_key_location_t) 0x800000)

                                                      
                                                                        
                                                                   
   

                            
   
                                                                              
#define PSA_KEY_ID_NULL                         ((psa_key_id_t)0)
                 
                                                                     
   
#define PSA_KEY_ID_USER_MIN                     ((psa_key_id_t) 0x00000001)
                                                                     
   
#define PSA_KEY_ID_USER_MAX                     ((psa_key_id_t) 0x3fffffff)
                                                                        
   
#define PSA_KEY_ID_VENDOR_MIN                   ((psa_key_id_t) 0x40000000)
                                                                        
   
#define PSA_KEY_ID_VENDOR_MAX                   ((psa_key_id_t) 0x7fffffff)


#if !defined(MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)

#define MBEDTLS_SVC_KEY_ID_INIT ((psa_key_id_t) 0)
#define MBEDTLS_SVC_KEY_ID_GET_KEY_ID(id) (id)
#define MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(id) (0)

                                                      
  
                                   
                                        
   
static inline mbedtls_svc_key_id_t mbedtls_svc_key_id_make(
    unsigned int unused, psa_key_id_t key_id)
{
    (void) unused;

    return key_id;
}

                                
  
                                   
                                    
  
                                                                        
   
static inline int mbedtls_svc_key_id_equal(mbedtls_svc_key_id_t id1,
                                           mbedtls_svc_key_id_t id2)
{
    return id1 == id2;
}

                                           
  
                             
  
                                                                  
   
static inline int mbedtls_svc_key_id_is_null(mbedtls_svc_key_id_t key)
{
    return key == 0;
}

#else                                              

#define MBEDTLS_SVC_KEY_ID_INIT ((mbedtls_svc_key_id_t){ 0, 0 })
#define MBEDTLS_SVC_KEY_ID_GET_KEY_ID(id) ((id).MBEDTLS_PRIVATE(key_id))
#define MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(id) ((id).MBEDTLS_PRIVATE(owner))

                                                      
  
                                               
                                         
   
static inline mbedtls_svc_key_id_t mbedtls_svc_key_id_make(
    mbedtls_key_owner_id_t owner_id, psa_key_id_t key_id)
{
    return (mbedtls_svc_key_id_t){ .MBEDTLS_PRIVATE(key_id) = key_id,
                                   .MBEDTLS_PRIVATE(owner) = owner_id };
}

                                
  
                                   
                                    
  
                                                                        
   
static inline int mbedtls_svc_key_id_equal(mbedtls_svc_key_id_t id1,
                                           mbedtls_svc_key_id_t id2)
{
    return (id1.MBEDTLS_PRIVATE(key_id) == id2.MBEDTLS_PRIVATE(key_id)) &&
           mbedtls_key_owner_id_equal(id1.MBEDTLS_PRIVATE(owner), id2.MBEDTLS_PRIVATE(owner));
}

                                           
  
                             
  
                                                                  
   
static inline int mbedtls_svc_key_id_is_null(mbedtls_svc_key_id_t key)
{
    return key.MBEDTLS_PRIVATE(key_id) == 0;
}

#endif                                               

       

                                 
     
   

                                                
                                                                        
                                                                   
   

                                    
  
                                                                       
                                                   
  
                                                                      
                                                                       
                                                                       
                                                                      
                                        
   
#define PSA_KEY_USAGE_EXPORT                    ((psa_key_usage_t) 0x00000001)

                                  
  
                                                                       
                                                     
  
                                                                       
                                                                          
                                                              
                                                                   
                                                         
                                                           
                                                                   
                                    
   
#define PSA_KEY_USAGE_COPY                      ((psa_key_usage_t) 0x00000002)

                                                     
  
                                                                            
                                                       
                                             
                                                       
  
                                                
   
#define PSA_KEY_USAGE_ENCRYPT                   ((psa_key_usage_t) 0x00000100)

                                                     
  
                                                                            
                                                     
                                             
                                                       
  
                                                 
   
#define PSA_KEY_USAGE_DECRYPT                   ((psa_key_usage_t) 0x00000200)

                                                  
  
                                                                             
                                                                           
                         
  
                                                 
   
#define PSA_KEY_USAGE_SIGN_MESSAGE              ((psa_key_usage_t) 0x00000400)

                                                    
  
                                                                              
                                                                       
                                          
  
                                                
   
#define PSA_KEY_USAGE_VERIFY_MESSAGE            ((psa_key_usage_t) 0x00000800)

                                                  
  
                                                                      
                                            
                                                       
  
                                                 
   
#define PSA_KEY_USAGE_SIGN_HASH                 ((psa_key_usage_t) 0x00001000)

                                                              
  
                                                                       
                                                         
                                                       
  
                                                
   
#define PSA_KEY_USAGE_VERIFY_HASH               ((psa_key_usage_t) 0x00002000)

                                                                          
        
  
                                                                            
                                                                          
          
  
                                                       
                                                                         
                                                       
                                                               
   
#define PSA_KEY_USAGE_DERIVE                    ((psa_key_usage_t) 0x00004000)

                                                                         
                              
  
                                       
  
                                                                        
                                                    
  
                                                       
                                                                         
                                                       
                                                               
   
#define PSA_KEY_USAGE_VERIFY_DERIVATION         ((psa_key_usage_t) 0x00008000)

       

                                       
     
   

                                                                         
                                                     

                                      
  
                                                    
                                             
                                                      
                                                     
  
                                                   
                                                                        
                                                                
                                     
                                        
                                           
                                   
   
#define PSA_KEY_DERIVATION_INPUT_SECRET     ((psa_key_derivation_step_t) 0x0101)

                                                                     
  
                                                                  
                                                               
                                                                             
                                                                            
                                                    
  
                                                   
                                                                        
                                                                
                                     
                                        
                                           
                                   
   
#define PSA_KEY_DERIVATION_INPUT_PASSWORD   ((psa_key_derivation_step_t) 0x0102)

                                                              
  
                                                                              
                                                                             
                                                                            
                                                               
   
#define PSA_KEY_DERIVATION_INPUT_OTHER_SECRET \
    ((psa_key_derivation_step_t) 0x0103)

                               
  
                                 
                                                       
   
#define PSA_KEY_DERIVATION_INPUT_LABEL      ((psa_key_derivation_step_t) 0x0201)

                              
  
                                 
                                                         
                        
   
#define PSA_KEY_DERIVATION_INPUT_SALT       ((psa_key_derivation_step_t) 0x0202)

                                             
  
                                 
                                                       
   
#define PSA_KEY_DERIVATION_INPUT_INFO       ((psa_key_derivation_step_t) 0x0203)

                              
  
                                 
                                                       
   
#define PSA_KEY_DERIVATION_INPUT_SEED       ((psa_key_derivation_step_t) 0x0204)

                                                           
  
                                                                             
   
#define PSA_KEY_DERIVATION_INPUT_COST       ((psa_key_derivation_step_t) 0x0205)

       

                                         
     
   

                   

                                                                            
                                             
  
                                                  
                                                  
  
                                                                            
                                 
                                                                               
                                                
   
#define MBEDTLS_PSA_ALG_AEAD_EQUAL(aead_alg_1, aead_alg_2) \
    (!(((aead_alg_1) ^ (aead_alg_2)) & \
       ~(PSA_ALG_AEAD_TAG_LENGTH_MASK | PSA_ALG_AEAD_AT_LEAST_THIS_LENGTH_FLAG)))

       

       

                                                    
     
   

                                                                              
                                                                        
                              
   
#define PSA_INTERRUPTIBLE_MAX_OPS_UNLIMITED UINT32_MAX

       

#endif                          
