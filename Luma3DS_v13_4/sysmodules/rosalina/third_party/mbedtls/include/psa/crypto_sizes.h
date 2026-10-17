   
                           
  
                                                              
  
                                                                  
                        
  
                                                                  
                                                                     
                                                                        
                                                                       
                   
  
                                                                    
                                                                    
                                                                  
                     
  
                                                              
                                  
   
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef PSA_CRYPTO_SIZES_H
#define PSA_CRYPTO_SIZES_H

  
                                                                           
                                                                            
                                                                           
                                                                  
                 
   
#include "psa/build_info.h"

#define PSA_BITS_TO_BYTES(bits) (((bits) + 7u) / 8u)
#define PSA_BYTES_TO_BITS(bytes) ((bytes) * 8u)
#define PSA_MAX_OF_THREE(a, b, c) ((a) <= (b) ? (b) <= (c) ? \
                                   (c) : (b) : (a) <= (c) ? (c) : (a))

#define PSA_ROUND_UP_TO_MULTIPLE(block_size, length) \
    (((length) + (block_size) - 1) / (block_size) * (block_size))

                                                          
  
                                                             
  
                                                                
                                                                       
                                                                  
                                
  
                                                          
                                                             
   
#define PSA_HASH_LENGTH(alg)                                        \
    (                                                               \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_MD5 ? 16u :           \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_RIPEMD160 ? 20u :     \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_1 ? 20u :         \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_224 ? 28u :       \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_256 ? 32u :       \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_384 ? 48u :       \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_512 ? 64u :       \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_512_224 ? 28u :   \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_512_256 ? 32u :   \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA3_224 ? 28u :      \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA3_256 ? 32u :      \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA3_384 ? 48u :      \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA3_512 ? 64u :      \
        0u)

                                                       
  
                                                                           
                                                                              
                                   
                                                   
  
                                                                
                                                 
  
                                                                         
                                                                  
                                                                               
                                                                        
   
#define PSA_HASH_BLOCK_LENGTH(alg)                                  \
    (                                                               \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_MD5 ? 64u :           \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_RIPEMD160 ? 64u :     \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_1 ? 64u :         \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_224 ? 64u :       \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_256 ? 64u :       \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_384 ? 128u :      \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_512 ? 128u :      \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_512_224 ? 128u :  \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_512_256 ? 128u :  \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA3_224 ? 144u :     \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA3_256 ? 136u :     \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA3_384 ? 104u :     \
        PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA3_512 ? 72u :      \
        0u)

                          
  
                          
  
                                                                    
                                          
   
                                                                       
                                                                    
                    
                                                                           
                                                      
#if defined(PSA_WANT_ALG_SHA3_224)
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 144u
#elif defined(PSA_WANT_ALG_SHA3_256)
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 136u
#elif defined(PSA_WANT_ALG_SHA_512)
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 128u
#elif defined(PSA_WANT_ALG_SHA_384)
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 128u
#elif defined(PSA_WANT_ALG_SHA3_384)
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 104u
#elif defined(PSA_WANT_ALG_SHA3_512)
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 72u
#elif defined(PSA_WANT_ALG_SHA_256)
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 64u
#elif defined(PSA_WANT_ALG_SHA_224)
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 64u
#else                       
#define PSA_HMAC_MAX_HASH_BLOCK_SIZE 64u
#endif

#if defined(PSA_WANT_ALG_SHA_512) || defined(PSA_WANT_ALG_SHA3_512)
#define PSA_HASH_MAX_SIZE 64u
#elif defined(PSA_WANT_ALG_SHA_384) || defined(PSA_WANT_ALG_SHA3_384)
#define PSA_HASH_MAX_SIZE 48u
#elif defined(PSA_WANT_ALG_SHA_256) || defined(PSA_WANT_ALG_SHA3_256)
#define PSA_HASH_MAX_SIZE 32u
#elif defined(PSA_WANT_ALG_SHA_224) || defined(PSA_WANT_ALG_SHA3_224)
#define PSA_HASH_MAX_SIZE 28u
#else                       
#define PSA_HASH_MAX_SIZE 20u
#endif

                         
  
                         
  
                                                                    
                                         
   
                                                                
                                                                         
                                                                       
               
   
#define PSA_MAC_MAX_SIZE PSA_HASH_MAX_SIZE

                                                        
  
                                                                              
                                     
  
                                   
  
                                                       
                                                               
                                               
                                                             
                                                                
  
                                                                                 
                                                                               
                                                                            
                                                       
                                                                      
                                                                             
                                       
   
#define PSA_AEAD_TAG_LENGTH(key_type, key_bits, alg)                        \
    (PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 ?                            \
     PSA_ALG_AEAD_GET_TAG_LENGTH(alg) :                                     \
     ((void) (key_bits), 0u))

                                                                     
  
                                                                   
   
#define PSA_AEAD_TAG_MAX_SIZE       16u

                                                                  
                                   
  
                                                                      
                                                                       
                                                                       
                                                                       
                                                               
                                                                   
                                                            
  
                                                                          
                                                                            
#define PSA_VENDOR_RSA_MAX_KEY_BITS 4096u

                                                                  
                                   
  
                                                                         
                                           
   
#if defined(MBEDTLS_RSA_GEN_KEY_MIN_BITS)
#define PSA_VENDOR_RSA_GENERATE_MIN_KEY_BITS MBEDTLS_RSA_GEN_KEY_MIN_BITS
#else
#define PSA_VENDOR_RSA_GENERATE_MIN_KEY_BITS 1024
#endif

                                                                 
                                     
#if defined(PSA_WANT_DH_RFC7919_8192)
#define PSA_VENDOR_FFDH_MAX_KEY_BITS 8192u
#elif defined(PSA_WANT_DH_RFC7919_6144)
#define PSA_VENDOR_FFDH_MAX_KEY_BITS 6144u
#elif defined(PSA_WANT_DH_RFC7919_4096)
#define PSA_VENDOR_FFDH_MAX_KEY_BITS 4096u
#elif defined(PSA_WANT_DH_RFC7919_3072)
#define PSA_VENDOR_FFDH_MAX_KEY_BITS 3072u
#elif defined(PSA_WANT_DH_RFC7919_2048)
#define PSA_VENDOR_FFDH_MAX_KEY_BITS 2048u
#else
#define PSA_VENDOR_FFDH_MAX_KEY_BITS 0u
#endif

                                                                  
                                      
#if defined(PSA_WANT_ECC_SECP_R1_521)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 521u
#elif defined(PSA_WANT_ECC_BRAINPOOL_P_R1_512)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 512u
#elif defined(PSA_WANT_ECC_MONTGOMERY_448)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 448u
#elif defined(PSA_WANT_ECC_SECP_R1_384)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 384u
#elif defined(PSA_WANT_ECC_BRAINPOOL_P_R1_384)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 384u
#elif defined(PSA_WANT_ECC_SECP_R1_256)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 256u
#elif defined(PSA_WANT_ECC_SECP_K1_256)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 256u
#elif defined(PSA_WANT_ECC_BRAINPOOL_P_R1_256)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 256u
#elif defined(PSA_WANT_ECC_MONTGOMERY_255)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 255u
#elif defined(PSA_WANT_ECC_SECP_R1_224)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 224u
#elif defined(PSA_WANT_ECC_SECP_K1_224)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 224u
#elif defined(PSA_WANT_ECC_SECP_R1_192)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 192u
#elif defined(PSA_WANT_ECC_SECP_K1_192)
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 192u
#else
#define PSA_VENDOR_ECC_MAX_CURVE_BITS 0u
#endif

                                                                      
                                   
                                           
  
                                                                             
  
                              
                                                                 
                                                                     
                                                                    
                       
  
                                                                     
                                         
   
#define PSA_TLS12_PSK_TO_MS_PSK_MAX_SIZE 128u

                                                                       
                                                      
#define PSA_TLS12_ECJPAKE_TO_PMS_INPUT_SIZE 65u

                                                        
                                                                       
            
#define PSA_TLS12_ECJPAKE_TO_PMS_DATA_SIZE 32u

                                                                               
                                                                          
#define PSA_VENDOR_PBKDF2_MAX_ITERATIONS 0xffffffffU

                                          
#define PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE 16u

                                                              
  
                                                                  
  
                                                                   
                                                                     
                         
  
                                                
                                                        
                                                                       
                                                         
  
                                                                     
                                                     
                                                                 
                                                                             
                                                                            
                                                                            
                                           
   
#define PSA_MAC_LENGTH(key_type, key_bits, alg)                                   \
    ((alg) & PSA_ALG_MAC_TRUNCATION_MASK ? PSA_MAC_TRUNCATED_LENGTH(alg) :        \
     PSA_ALG_IS_HMAC(alg) ? PSA_HASH_LENGTH(PSA_ALG_HMAC_GET_HASH(alg)) :         \
     PSA_ALG_IS_BLOCK_CIPHER_MAC(alg) ? PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) : \
     ((void) (key_type), (void) (key_bits), 0u))

                                                                   
  
                                                                     
                                                             
                                                                           
                                 
  
                                                                   
  
                                                                   
                                                                     
                         
  
                                                          
                                                               
                                               
                                                             
                                                                
                                                             
  
                                                                        
                                        
                                                                      
                                                                             
                                       
   
#define PSA_AEAD_ENCRYPT_OUTPUT_SIZE(key_type, alg, plaintext_length) \
    (PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 ?                      \
     (plaintext_length) + PSA_ALG_AEAD_GET_TAG_LENGTH(alg) :          \
     0u)

                                                                          
                                            
  
                                                                                
                                                                            
  
                                                                        
                                
  
                                                              
                        
  
                                                             
  
                                                                            
                                                                      
  
   
#define PSA_AEAD_ENCRYPT_OUTPUT_MAX_SIZE(plaintext_length)          \
    ((plaintext_length) + PSA_AEAD_TAG_MAX_SIZE)


                                                                   
  
                                                                    
                                                             
                                                                           
                                
  
                                                                    
  
                                                                   
                                                                     
                         
  
                                                          
                                                               
                                               
                                                             
                                                                
                                                             
  
                                                                        
                                        
                                                                      
                                                                             
                                       
   
#define PSA_AEAD_DECRYPT_OUTPUT_SIZE(key_type, alg, ciphertext_length) \
    (PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 &&                      \
     (ciphertext_length) > PSA_ALG_AEAD_GET_TAG_LENGTH(alg) ?      \
     (ciphertext_length) - PSA_ALG_AEAD_GET_TAG_LENGTH(alg) :      \
     0u)

                                                                          
                                            
  
                                                                               
                                                                            
  
                                                                        
                                
  
                                                              
                         
  
                                                              
  
                                                                            
                                                                      
  
   
#define PSA_AEAD_DECRYPT_OUTPUT_MAX_SIZE(ciphertext_length)     \
    (ciphertext_length)

                                                           
  
                                                                    
                                                          
  
                                     
  
                                                                    
                                                                           
                                                                               
  
                                                                   
                                                                     
                         
  
                                                                
                                     
  
                                                                     
                                                      
  
                                                                           
                                                               
                                                        
   
#define PSA_AEAD_NONCE_LENGTH(key_type, alg) \
    (PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) == 16 ? \
     MBEDTLS_PSA_ALG_AEAD_EQUAL(alg, PSA_ALG_CCM) ? 13u : \
     MBEDTLS_PSA_ALG_AEAD_EQUAL(alg, PSA_ALG_GCM) ? 12u : \
     0u : \
     (key_type) == PSA_KEY_TYPE_CHACHA20 && \
     MBEDTLS_PSA_ALG_AEAD_EQUAL(alg, PSA_ALG_CHACHA20_POLY1305) ? 12u : \
     0u)

                                                                             
                              
  
                                                                           
              
  
                                                                    
                                                                           
                                                       
                                    
   
#define PSA_AEAD_NONCE_MAX_SIZE 13u

                                                          
  
                                                                 
                                                            
                                                                         
                     
  
                                                              
  
                                                                   
                                                                     
                         
  
                                                          
                                                               
                                               
                                                             
                                                                
                                                         
  
                                                                               
                                        
                                                                      
                                                                             
                                       
   
                                                                       
                                                                    
                                                                   
                                                                   
#define PSA_AEAD_UPDATE_OUTPUT_SIZE(key_type, alg, input_length)                             \
    (PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 ?                                             \
     PSA_ALG_IS_AEAD_ON_BLOCK_CIPHER(alg) ?                                              \
     PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type), (input_length)) : \
     (input_length) : \
     0u)

                                                                         
                                            
  
                                                                            
                                                                           
  
                                                                               
  
                                                       
   
#define PSA_AEAD_UPDATE_OUTPUT_MAX_SIZE(input_length)                           \
    (PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE, (input_length)))

                                                              
  
                                                                     
                                                            
                                                                         
                                
  
                                             
  
                                                          
                                                               
                                               
                                                             
                                                                
  
                                                                         
                                                  
                                                                      
                                                                             
                                       
   
#define PSA_AEAD_FINISH_OUTPUT_SIZE(key_type, alg) \
    (PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 &&  \
     PSA_ALG_IS_AEAD_ON_BLOCK_CIPHER(alg) ?    \
     PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) : \
     0u)

                                                                             
                                            
  
                                                              
   
#define PSA_AEAD_FINISH_OUTPUT_MAX_SIZE     (PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE)

                                                             
  
                                                                    
                                                            
                                                                        
                                
  
                                             
  
                                                          
                                                               
                                               
                                                             
                                                                
  
                                                                        
                                                  
                                                                      
                                                                             
                                       
   
#define PSA_AEAD_VERIFY_OUTPUT_SIZE(key_type, alg) \
    (PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 &&  \
     PSA_ALG_IS_AEAD_ON_BLOCK_CIPHER(alg) ?    \
     PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) : \
     0u)

                                                                            
                                            
  
                                                              
   
#define PSA_AEAD_VERIFY_OUTPUT_MAX_SIZE     (PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE)

#define PSA_RSA_MINIMUM_PADDING_SIZE(alg)                         \
    (PSA_ALG_IS_RSA_OAEP(alg) ?                                   \
     2u * PSA_HASH_LENGTH(PSA_ALG_RSA_OAEP_GET_HASH(alg)) + 1u :   \
     11u               )

   
                                                         
  
                                           
                                                
  
                                                                           
   
#define PSA_ECDSA_SIGNATURE_SIZE(curve_bits)    \
    (PSA_BITS_TO_BYTES(curve_bits) * 2u)

                                                         
  
                                                                          
                                                                
                                                            
                                                       
  
                                                                  
                                                                     
                         
  
                                                                       
                                                        
                                                
                                            
  
                                                            
                                                      
                                             
                                       
                                                                           
                                              
                                               
                                       
   
#define PSA_SIGN_OUTPUT_SIZE(key_type, key_bits, alg)        \
    (PSA_KEY_TYPE_IS_RSA(key_type) ? ((void) alg, PSA_BITS_TO_BYTES(key_bits)) : \
     PSA_KEY_TYPE_IS_ECC(key_type) ? PSA_ECDSA_SIGNATURE_SIZE(key_bits) : \
     ((void) alg, 0u))

#define PSA_VENDOR_ECDSA_SIGNATURE_MAX_SIZE     \
    PSA_ECDSA_SIGNATURE_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)

                               
  
                                           
  
                                                                    
                                               
   
#define PSA_SIGNATURE_MAX_SIZE      1

#if (defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA)) && \
    (PSA_VENDOR_ECDSA_SIGNATURE_MAX_SIZE > PSA_SIGNATURE_MAX_SIZE)
#undef PSA_SIGNATURE_MAX_SIZE
#define PSA_SIGNATURE_MAX_SIZE      PSA_VENDOR_ECDSA_SIGNATURE_MAX_SIZE
#endif
#if (defined(PSA_WANT_ALG_RSA_PKCS1V15_SIGN) || defined(PSA_WANT_ALG_RSA_PSS)) && \
    (PSA_BITS_TO_BYTES(PSA_VENDOR_RSA_MAX_KEY_BITS) > PSA_SIGNATURE_MAX_SIZE)
#undef PSA_SIGNATURE_MAX_SIZE
#define PSA_SIGNATURE_MAX_SIZE      PSA_BITS_TO_BYTES(PSA_VENDOR_RSA_MAX_KEY_BITS)
#endif

                                                               
  
                                                                              
                                                                      
                                                                        
                    
  
                                                                  
                                                                     
                         
  
                                                                       
                                                        
                                                
                                                        
  
                                                            
                                                      
                                                      
                                       
                                                                           
                                              
                                               
                                       
   
#define PSA_ASYMMETRIC_ENCRYPT_OUTPUT_SIZE(key_type, key_bits, alg)     \
    (PSA_KEY_TYPE_IS_RSA(key_type) ?                                    \
     ((void) alg, PSA_BITS_TO_BYTES(key_bits)) :                         \
     0u)

                                                                         
                                    
  
                                                                                  
   
                                                                              
#define PSA_ASYMMETRIC_ENCRYPT_OUTPUT_MAX_SIZE          \
    (PSA_BITS_TO_BYTES(PSA_VENDOR_RSA_MAX_KEY_BITS))

                                                               
  
                                                                             
                                                                      
                                                                       
                    
  
                                                                  
                                                                     
                         
  
                                                                       
                                                        
                                                
                                                        
  
                                                            
                                                      
                                                      
                                       
                                                                           
                                              
                                               
                                       
   
#define PSA_ASYMMETRIC_DECRYPT_OUTPUT_SIZE(key_type, key_bits, alg)     \
    (PSA_KEY_TYPE_IS_RSA(key_type) ?                                    \
     PSA_BITS_TO_BYTES(key_bits) - PSA_RSA_MINIMUM_PADDING_SIZE(alg) :  \
     0u)

                                                                         
                                    
  
                                                                           
  
                                                                                  
   
#define PSA_ASYMMETRIC_DECRYPT_OUTPUT_MAX_SIZE          \
    (PSA_BITS_TO_BYTES(PSA_VENDOR_RSA_MAX_KEY_BITS))

                                                                      
                  
  
                                                                         
                                                                      
                                                     
                 
                         
                                                   
   
#define PSA_KEY_EXPORT_ASN1_INTEGER_MAX_SIZE(bits)      \
    ((bits) / 8u + 5u)

                                                            
                                                      
  
                                 
                                         
                                         
  
                                  
                 
                                     
   
#define PSA_KEY_EXPORT_RSA_PUBLIC_KEY_MAX_SIZE(key_bits)        \
    (PSA_KEY_EXPORT_ASN1_INTEGER_MAX_SIZE(key_bits) + 11u)

                                                          
                                                                       
                                                      
  
                               
                                       
                                           
                                            
                                           
                                             
                                             
                                             
                                             
                                             
    
  
                                  
                        
                                                    
                                              
                                     
   
#define PSA_KEY_EXPORT_RSA_KEY_PAIR_MAX_SIZE(key_bits)   \
    (9u * PSA_KEY_EXPORT_ASN1_INTEGER_MAX_SIZE((key_bits) / 2u + 1u) + 14u)

                                                           
  
                                         
                                                 
                                                                   
                                        
                                                  
                                                                       
                                              
  
                                      
                                            
                                    
                                    
                                                             
   
#define PSA_KEY_EXPORT_DSA_PUBLIC_KEY_MAX_SIZE(key_bits)        \
    (PSA_KEY_EXPORT_ASN1_INTEGER_MAX_SIZE(key_bits) * 3u + 59u)

                                                         
  
                               
                                         
                                         
                                         
                                         
                                         
                                         
    
  
                                  
                        
                                    
                                                                       
   
#define PSA_KEY_EXPORT_DSA_KEY_PAIR_MAX_SIZE(key_bits)   \
    (PSA_KEY_EXPORT_ASN1_INTEGER_MAX_SIZE(key_bits) * 3u + 75u)

                                                            
  
                                              
                        
                                                            
                                                            
                                                            
  
                             
   
#define PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(key_bits)        \
    (2u * PSA_BITS_TO_BYTES(key_bits) + 1u)

                                                          
  
                                                      
   
#define PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(key_bits)   \
    (PSA_BITS_TO_BYTES(key_bits))

                                                         
  
                                                     
   
#define PSA_KEY_EXPORT_FFDH_KEY_PAIR_MAX_SIZE(key_bits)   \
    (PSA_BITS_TO_BYTES(key_bits))

                                                           
   
#define PSA_KEY_EXPORT_FFDH_PUBLIC_KEY_MAX_SIZE(key_bits)   \
    (PSA_BITS_TO_BYTES(key_bits))

                                                         
                           
  
                                                                  
                          
  
                                                                   
                                                                     
                         
  
                                                                         
                                                      
           
                                                             
                       
                                                     
                                                
                                                           
                                                   
                                                                       
                                         
                                         
                                         
                        
                                                                     
                                                
           
  
                                         
                                                
  
                                                            
                                                      
                                                                         
                                       
                                                                           
                                              
                                                                            
   
#define PSA_EXPORT_KEY_OUTPUT_SIZE(key_type, key_bits)                                              \
    ((key_type) == PSA_KEY_TYPE_RSA_KEY_PAIR ? PSA_KEY_EXPORT_RSA_KEY_PAIR_MAX_SIZE(key_bits) :     \
     (key_type) == PSA_KEY_TYPE_RSA_PUBLIC_KEY ? PSA_KEY_EXPORT_RSA_PUBLIC_KEY_MAX_SIZE(key_bits) : \
     PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type) ? PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(key_bits) :      \
     PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(key_type) ? PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(key_bits) :  \
     PSA_BITS_TO_BYTES(key_bits))                                         

                                                              
  
                                                                  
                          
  
                                                                   
                                                                     
                         
  
                                                                         
                                                             
           
                                                             
                       
                                                     
                                                
                                                           
                                                   
                                                                              
                                         
                                         
                                         
                        
                                                                            
                                                
           
  
                                                          
                                                    
  
                                                                         
                                                                   
                                                                  
                                                    
                                                                             
                                                                      
                                                        
                                                        
  
                                                                  
                                                 
                                                    
                                                                                 
                                         
   
#define PSA_EXPORT_PUBLIC_KEY_OUTPUT_SIZE(key_type, key_bits)                           \
    (PSA_KEY_TYPE_IS_RSA(key_type) ? PSA_KEY_EXPORT_RSA_PUBLIC_KEY_MAX_SIZE(key_bits) : \
     PSA_KEY_TYPE_IS_ECC(key_type) ? PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(key_bits) : \
     PSA_KEY_TYPE_IS_DH(key_type) ? PSA_BITS_TO_BYTES(key_bits) : \
     0u)

                                                                 
  
                                                                       
                                                                       
                                                                      
  
                                                                  
   
#define PSA_EXPORT_KEY_PAIR_MAX_SIZE            1

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC) && \
    (PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS) > \
     PSA_EXPORT_KEY_PAIR_MAX_SIZE)
#undef PSA_EXPORT_KEY_PAIR_MAX_SIZE
#define PSA_EXPORT_KEY_PAIR_MAX_SIZE    \
    PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)
#endif
#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC) && \
    (PSA_KEY_EXPORT_RSA_KEY_PAIR_MAX_SIZE(PSA_VENDOR_RSA_MAX_KEY_BITS) > \
     PSA_EXPORT_KEY_PAIR_MAX_SIZE)
#undef PSA_EXPORT_KEY_PAIR_MAX_SIZE
#define PSA_EXPORT_KEY_PAIR_MAX_SIZE    \
    PSA_KEY_EXPORT_RSA_KEY_PAIR_MAX_SIZE(PSA_VENDOR_RSA_MAX_KEY_BITS)
#endif
#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_BASIC) && \
    (PSA_KEY_EXPORT_FFDH_KEY_PAIR_MAX_SIZE(PSA_VENDOR_FFDH_MAX_KEY_BITS) > \
     PSA_EXPORT_KEY_PAIR_MAX_SIZE)
#undef PSA_EXPORT_KEY_PAIR_MAX_SIZE
#define PSA_EXPORT_KEY_PAIR_MAX_SIZE    \
    PSA_KEY_EXPORT_FFDH_KEY_PAIR_MAX_SIZE(PSA_VENDOR_FFDH_MAX_KEY_BITS)
#endif

                                                                   
  
                                                                       
                                                            
                                                               
                                                 
  
                                                                         
   
#define PSA_EXPORT_PUBLIC_KEY_MAX_SIZE            1

#if defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY) && \
    (PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS) > \
     PSA_EXPORT_PUBLIC_KEY_MAX_SIZE)
#undef PSA_EXPORT_PUBLIC_KEY_MAX_SIZE
#define PSA_EXPORT_PUBLIC_KEY_MAX_SIZE    \
    PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)
#endif
#if defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY) && \
    (PSA_KEY_EXPORT_RSA_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_RSA_MAX_KEY_BITS) > \
     PSA_EXPORT_PUBLIC_KEY_MAX_SIZE)
#undef PSA_EXPORT_PUBLIC_KEY_MAX_SIZE
#define PSA_EXPORT_PUBLIC_KEY_MAX_SIZE    \
    PSA_KEY_EXPORT_RSA_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_RSA_MAX_KEY_BITS)
#endif
#if defined(PSA_WANT_KEY_TYPE_DH_PUBLIC_KEY) && \
    (PSA_KEY_EXPORT_FFDH_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_FFDH_MAX_KEY_BITS) > \
     PSA_EXPORT_PUBLIC_KEY_MAX_SIZE)
#undef PSA_EXPORT_PUBLIC_KEY_MAX_SIZE
#define PSA_EXPORT_PUBLIC_KEY_MAX_SIZE    \
    PSA_KEY_EXPORT_FFDH_PUBLIC_KEY_MAX_SIZE(PSA_VENDOR_FFDH_MAX_KEY_BITS)
#endif

                                                               
#define PSA_EXPORT_ASYMMETRIC_KEY_MAX_SIZE \
    ((PSA_EXPORT_KEY_PAIR_MAX_SIZE > PSA_EXPORT_PUBLIC_KEY_MAX_SIZE) ? \
     PSA_EXPORT_KEY_PAIR_MAX_SIZE : PSA_EXPORT_PUBLIC_KEY_MAX_SIZE)

                                                              
                                                      
#define PSA_EXPORT_KEY_PAIR_OR_PUBLIC_MAX_SIZE \
    PSA_EXPORT_ASYMMETRIC_KEY_MAX_SIZE

                                                              
  
                                                                  
                          
  
                                                                   
                                                                     
                         
  
                                                   
  
                                             
                                                    
  
                                                                         
                                                                   
                                                                  
                                                    
                                                                      
                                                                             
                                                        
                                                        
   
#define PSA_RAW_KEY_AGREEMENT_OUTPUT_SIZE(key_type, key_bits)   \
    ((PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type) || \
      PSA_KEY_TYPE_IS_DH_KEY_PAIR(key_type)) ? PSA_BITS_TO_BYTES(key_bits) : 0u)

                                                            
  
                                                                           
                                                                        
  
                                                                         
   
#define PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE       1

#if defined(PSA_WANT_ALG_ECDH) && \
    (PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS) > PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE)
#undef PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE
#define PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE    PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)
#endif
#if defined(PSA_WANT_ALG_FFDH) && \
    (PSA_BITS_TO_BYTES(PSA_VENDOR_FFDH_MAX_KEY_BITS) > PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE)
#undef PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE
#define PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE    PSA_BITS_TO_BYTES(PSA_VENDOR_FFDH_MAX_KEY_BITS)
#endif

                                   
  
                                                                           
                                                                          
                                                                          
                                                                              
                                
  
                                                                               
                                                                               
              
   
#if (defined(PSA_WANT_KEY_TYPE_AES) || defined(PSA_WANT_KEY_TYPE_ARIA) || \
    defined(PSA_WANT_KEY_TYPE_CAMELLIA) || defined(PSA_WANT_KEY_TYPE_CHACHA20))
#define PSA_CIPHER_MAX_KEY_LENGTH       32u
#elif defined(PSA_WANT_KEY_TYPE_DES)
#define PSA_CIPHER_MAX_KEY_LENGTH       24u
#else
#define PSA_CIPHER_MAX_KEY_LENGTH       0u
#endif

                                                         
  
                                                                                
                                           
  
                                                                    
                                                                
                                 
  
                                    
  
                                                                   
                                                                     
                         
  
                                                                                  
  
                                                                                                           
  
                                                                        
                                                         
                                                                 
                                                        
   
#define PSA_CIPHER_IV_LENGTH(key_type, alg) \
    (PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) > 1 && \
     ((alg) == PSA_ALG_CTR || \
      (alg) == PSA_ALG_CFB || \
      (alg) == PSA_ALG_OFB || \
      (alg) == PSA_ALG_XTS || \
      (alg) == PSA_ALG_CBC_NO_PADDING || \
      (alg) == PSA_ALG_CBC_PKCS7) ? PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) : \
     (key_type) == PSA_KEY_TYPE_CHACHA20 && \
     (alg) == PSA_ALG_STREAM_CIPHER ? 12u : \
     (alg) == PSA_ALG_CCM_STAR_NO_TAG ? 13u : \
     0u)

                                                                      
  
                                    
   
#define PSA_CIPHER_IV_MAX_SIZE 16u

                                                                     
  
                                                                            
                                                                              
                                                                              
  
                                                                 
  
                                                                   
                                                                     
                         
  
                                                                              
                            
                                                                          
                                                            
                                                   
  
                                                                               
                                                                             
                                                                       
                                 
   
#define PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(key_type, alg, input_length)     \
    (alg == PSA_ALG_CBC_PKCS7 ?                                         \
     (PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) != 0 ?                    \
      PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type), \
                               (input_length) + 1u) +                   \
      PSA_CIPHER_IV_LENGTH((key_type), (alg)) : 0u) :                   \
     (PSA_ALG_IS_CIPHER(alg) ?                                          \
      (input_length) + PSA_CIPHER_IV_LENGTH((key_type), (alg)) :        \
      0u))

                                                                            
                                              
  
                                                                            
                                                                              
  
                                                                                  
  
                                                   
  
   
#define PSA_CIPHER_ENCRYPT_OUTPUT_MAX_SIZE(input_length)                \
    (PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE,          \
                              (input_length) + 1u) +                    \
     PSA_CIPHER_IV_MAX_SIZE)

                                                                     
  
                                                                            
                                                                              
                                                                              
  
                                                                 
  
                                                                              
                            
                                                                          
                                                            
                                                   
  
                                                                               
                                                                             
                                                                       
                                 
   
#define PSA_CIPHER_DECRYPT_OUTPUT_SIZE(key_type, alg, input_length)     \
    (PSA_ALG_IS_CIPHER(alg) &&                                          \
     ((key_type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_SYMMETRIC ? \
     (input_length) :                                                   \
     0u)

                                                                            
                                              
  
                                                                            
                                                                              
  
                                                                                  
  
                                                   
   
#define PSA_CIPHER_DECRYPT_OUTPUT_MAX_SIZE(input_length)    \
    (input_length)

                                                            
  
                                                                            
                                                                             
                                                                    
  
                                                                
  
                                                                              
                            
                                                                       
                                                            
                                                   
  
                                                                               
                                                                             
                                                                                 
   
#define PSA_CIPHER_UPDATE_OUTPUT_SIZE(key_type, alg, input_length)      \
    (PSA_ALG_IS_CIPHER(alg) ?                                           \
     (PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) != 0 ?                    \
      (((alg) == PSA_ALG_CBC_PKCS7      ||                              \
        (alg) == PSA_ALG_CBC_NO_PADDING ||                              \
        (alg) == PSA_ALG_ECB_NO_PADDING) ?                              \
       PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type), \
                                input_length) :                         \
       (input_length)) : 0u) :                                          \
     0u)

                                                                           
                                              
  
                                                                            
                                                                             
  
                                                                                 
  
                                                   
   
#define PSA_CIPHER_UPDATE_OUTPUT_MAX_SIZE(input_length)     \
    (PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE, input_length))

                                                                
  
                                                                     
                                                                           
                                                                            
                  
  
                                                 
  
                                                                              
                            
                                                                       
                                                            
                                                                               
                                                                             
                                                                                 
   
#define PSA_CIPHER_FINISH_OUTPUT_SIZE(key_type, alg)    \
    (PSA_ALG_IS_CIPHER(alg) ?                           \
     (alg == PSA_ALG_CBC_PKCS7 ?                        \
      PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) :         \
      0u) :                                             \
     0u)

                                                                               
                                              
  
                                                                
   
#define PSA_CIPHER_FINISH_OUTPUT_MAX_SIZE           \
    (PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE)

#endif                         
