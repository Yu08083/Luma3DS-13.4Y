/*
 *  TLS 1.3 client-side functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include "common.h"

#if defined(MBEDTLS_SSL_CLI_C) && defined(MBEDTLS_SSL_PROTO_TLS1_3)

#include <string.h>

#include "debug_internal.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"

#include "ssl_misc.h"
#include "ssl_client.h"
#include "ssl_tls13_keys.h"
#include "ssl_debug_helpers.h"
#include "mbedtls/psa_util.h"

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_EPHEMERAL_ENABLED)
                                                                              
                                          
static int local_err_translation(psa_status_t status)
{
    return psa_status_to_mbedtls(status, psa_to_ssl_errors,
                                 ARRAY_LENGTH(psa_to_ssl_errors),
                                 psa_generic_status_to_mbedtls);
}
#define PSA_TO_MBEDTLS_ERR(status) local_err_translation(status)
#endif

                      

  
                                            
  
           
                                         
                       
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_supported_versions_ext(mbedtls_ssl_context *ssl,
                                                  unsigned char *buf,
                                                  unsigned char *end,
                                                  size_t *out_len)
{
    unsigned char *p = buf;
    unsigned char versions_len = (ssl->handshake->min_tls_version <=
                                  MBEDTLS_SSL_VERSION_TLS1_2) ? 4 : 2;

    *out_len = 0;

    MBEDTLS_SSL_DEBUG_MSG(3, ("client hello, adding supported versions extension"));

                                                     
                                         
                                         
                                         
                                              
       
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 5 + versions_len);

    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_SUPPORTED_VERSIONS, p, 0);
    MBEDTLS_PUT_UINT16_BE(versions_len + 1, p, 2);
    p += 4;

                            
    *p++ = versions_len;

                                          
                                             
                                                                        
       
    mbedtls_ssl_write_version(p, MBEDTLS_SSL_TRANSPORT_STREAM,
                              MBEDTLS_SSL_VERSION_TLS1_3);
    MBEDTLS_SSL_DEBUG_MSG(3, ("supported version: [3:4]"));


    if (ssl->handshake->min_tls_version <= MBEDTLS_SSL_VERSION_TLS1_2) {
        mbedtls_ssl_write_version(p + 2, MBEDTLS_SSL_TRANSPORT_STREAM,
                                  MBEDTLS_SSL_VERSION_TLS1_2);
        MBEDTLS_SSL_DEBUG_MSG(3, ("supported version: [3:3]"));
    }

    *out_len = 5 + versions_len;

    mbedtls_ssl_tls13_set_hs_sent_ext_mask(
        ssl, MBEDTLS_TLS_EXT_SUPPORTED_VERSIONS);

    return 0;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_supported_versions_ext(mbedtls_ssl_context *ssl,
                                                  const unsigned char *buf,
                                                  const unsigned char *end)
{
    ((void) ssl);

    MBEDTLS_SSL_CHK_BUF_READ_PTR(buf, end, 2);
    if (mbedtls_ssl_read_version(buf, ssl->conf->transport) !=
        MBEDTLS_SSL_VERSION_TLS1_3) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("unexpected version"));

        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                                     MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
    }

    if (&buf[2] != end) {
        MBEDTLS_SSL_DEBUG_MSG(
            1, ("supported_versions ext data length incorrect"));
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR,
                                     MBEDTLS_ERR_SSL_DECODE_ERROR);
        return MBEDTLS_ERR_SSL_DECODE_ERROR;
    }

    return 0;
}

#if defined(MBEDTLS_SSL_ALPN)
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_alpn_ext(mbedtls_ssl_context *ssl,
                                    const unsigned char *buf, size_t len)
{
    const unsigned char *p = buf;
    const unsigned char *end = buf + len;
    size_t protocol_name_list_len, protocol_name_len;
    const unsigned char *protocol_name_list_end;

                                                            
    if (ssl->conf->alpn_list == NULL) {
        return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
    }

      
                                     
      
               
                                                     
                          
      
                                                                     
       

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    protocol_name_list_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, protocol_name_list_len);
    protocol_name_list_end = p + protocol_name_list_len;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, protocol_name_list_end, 1);
    protocol_name_len = *p++;

                                                                           
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, protocol_name_list_end, protocol_name_len);
    for (const char **alpn = ssl->conf->alpn_list; *alpn != NULL; alpn++) {
        if (protocol_name_len == strlen(*alpn) &&
            memcmp(p, *alpn, protocol_name_len) == 0) {
            ssl->alpn_chosen = *alpn;
            return 0;
        }
    }

    return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
}
#endif                       

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_reset_key_share(mbedtls_ssl_context *ssl)
{
    uint16_t group_id = ssl->handshake->offered_group_id;

    if (group_id == 0) {
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_EPHEMERAL_ENABLED)
    if (mbedtls_ssl_tls13_named_group_is_ecdhe(group_id) ||
        mbedtls_ssl_tls13_named_group_is_ffdh(group_id)) {
        int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
        psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

                                            
        status = psa_destroy_key(ssl->handshake->xxdh_psa_privkey);
        if (status != PSA_SUCCESS) {
            ret = PSA_TO_MBEDTLS_ERR(status);
            MBEDTLS_SSL_DEBUG_RET(1, "psa_destroy_key", ret);
            return ret;
        }

        ssl->handshake->xxdh_psa_privkey = MBEDTLS_SVC_KEY_ID_INIT;
        return 0;
    } else
#endif                                                                  
    if (0                  ) {
                          
    }

    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

  
                                             
   
#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_EPHEMERAL_ENABLED)
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_get_default_group_id(mbedtls_ssl_context *ssl,
                                          uint16_t *group_id)
{
    int ret = MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;


#if defined(PSA_WANT_ALG_ECDH) || defined(PSA_WANT_ALG_FFDH)
    const uint16_t *group_list = mbedtls_ssl_get_groups(ssl);
                                                                  
    if (group_list == NULL) {
        return MBEDTLS_ERR_SSL_BAD_CONFIG;
    }

    for (; *group_list != 0; group_list++) {
#if defined(PSA_WANT_ALG_ECDH)
        if ((mbedtls_ssl_get_psa_curve_info_from_tls_id(
                 *group_list, NULL, NULL) == PSA_SUCCESS) &&
            mbedtls_ssl_tls13_named_group_is_ecdhe(*group_list)) {
            *group_id = *group_list;
            return 0;
        }
#endif
#if defined(PSA_WANT_ALG_FFDH)
        if (mbedtls_ssl_tls13_named_group_is_ffdh(*group_list)) {
            *group_id = *group_list;
            return 0;
        }
#endif
    }
#else
    ((void) ssl);
    ((void) group_id);
#endif                                             

    return ret;
}

  
                                
  
                                                   
  
            
                             
                                           
                        
            
                                                   
                              
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_key_share_ext(mbedtls_ssl_context *ssl,
                                         unsigned char *buf,
                                         unsigned char *end,
                                         size_t *out_len)
{
    unsigned char *p = buf;
    unsigned char *client_shares;                             
    size_t client_shares_len;                                  
    uint16_t group_id;
    int ret = MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;

    *out_len = 0;

                                                           
                                         
                                         
                                         
       
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 6);
    p += 6;

    MBEDTLS_SSL_DEBUG_MSG(3, ("client hello: adding key share extension"));

                                                          
    group_id = ssl->handshake->offered_group_id;
    if (!mbedtls_ssl_tls13_named_group_is_ecdhe(group_id) &&
        !mbedtls_ssl_tls13_named_group_is_ffdh(group_id)) {
        MBEDTLS_SSL_PROC_CHK(ssl_tls13_get_default_group_id(ssl,
                                                            &group_id));
    }

      
                                                         
      
                                                                 
                                                                 
                                                                 
                                           
       
    client_shares = p;
#if defined(PSA_WANT_ALG_ECDH) || defined(PSA_WANT_ALG_FFDH)
    if (mbedtls_ssl_tls13_named_group_is_ecdhe(group_id) ||
        mbedtls_ssl_tls13_named_group_is_ffdh(group_id)) {
                              
        unsigned char *group = p;
                                    
        size_t key_exchange_len = 0;

                                                           
                                             
                                             
           
        MBEDTLS_SSL_CHK_BUF_PTR(p, end, 4);
        p += 4;
        ret = mbedtls_ssl_tls13_generate_and_write_xxdh_key_exchange(
            ssl, group_id, p, end, &key_exchange_len);
        p += key_exchange_len;
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("client hello: failed generating xxdh key exchange"));
            return ret;
        }

                         
        MBEDTLS_PUT_UINT16_BE(group_id, group, 0);
                                       
        MBEDTLS_PUT_UINT16_BE(key_exchange_len, group, 2);
    } else
#endif                                             
    if (0                  ) {
                          
    } else {
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

                                 
    client_shares_len = p - client_shares;
    if (client_shares_len == 0) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("No key share defined."));
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }
                              
    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_KEY_SHARE, buf, 0);
                                     
    MBEDTLS_PUT_UINT16_BE(client_shares_len + 2, buf, 2);
                                    
    MBEDTLS_PUT_UINT16_BE(client_shares_len, buf, 4);

                                       
    ssl->handshake->offered_group_id = group_id;

                                                         
    *out_len = p - buf;

    MBEDTLS_SSL_DEBUG_BUF(
        3, "client hello, key_share extension", buf, *out_len);

    mbedtls_ssl_tls13_set_hs_sent_ext_mask(ssl, MBEDTLS_TLS_EXT_KEY_SHARE);

cleanup:

    return ret;
}
#endif                                                                  

  
                                      
                                                        
  
           
                                    
                               
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_hrr_key_share_ext(mbedtls_ssl_context *ssl,
                                             const unsigned char *buf,
                                             const unsigned char *end)
{
#if defined(PSA_WANT_ALG_ECDH) || defined(PSA_WANT_ALG_FFDH)
    const unsigned char *p = buf;
    int selected_group;
    int found = 0;

    const uint16_t *group_list = mbedtls_ssl_get_groups(ssl);
    if (group_list == NULL) {
        return MBEDTLS_ERR_SSL_BAD_CONFIG;
    }

    MBEDTLS_SSL_DEBUG_BUF(3, "key_share extension", p, end - buf);

                             
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    selected_group = MBEDTLS_GET_UINT16_BE(p, 0);
    MBEDTLS_SSL_DEBUG_MSG(3, ("selected_group ( %d )", selected_group));

                                                                        
                                                                       
                                                                          
                            
                                                                          
      
                                                                              
                                                                                  
       
    for (; *group_list != 0; group_list++) {
        if (*group_list != selected_group) {
            continue;
        }
#if defined(PSA_WANT_ALG_ECDH)
        if (mbedtls_ssl_tls13_named_group_is_ecdhe(*group_list)) {
            if (mbedtls_ssl_get_psa_curve_info_from_tls_id(
                    *group_list, NULL, NULL) == PSA_ERROR_NOT_SUPPORTED) {
                continue;
            }
                                                                                       
            found = 1;
            break;
        }
#endif                        
#if defined(PSA_WANT_ALG_FFDH)
        if (mbedtls_ssl_tls13_named_group_is_ffdh(*group_list)) {
            found = 1;
            break;
        }
#endif                        
    }

                                                                
                                                                  
                                                                   
                                                           
                                                                
                                    
       
    if (found == 0 || selected_group == ssl->handshake->offered_group_id) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("Invalid key share in HRR"));
        MBEDTLS_SSL_PEND_FATAL_ALERT(
            MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
            MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
    }

                                                           
    ssl->handshake->offered_group_id = selected_group;

    return 0;
#else                                             
    (void) ssl;
    (void) buf;
    (void) end;
    return MBEDTLS_ERR_SSL_BAD_CONFIG;
#endif                                             
}

  
                                  
                                                 
  
           
                                     
                         
           
                           
                                         
                   
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_key_share_ext(mbedtls_ssl_context *ssl,
                                         const unsigned char *buf,
                                         const unsigned char *end)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const unsigned char *p = buf;
    uint16_t group, offered_group;

          
                                  
          
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    group = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

                                                                 
    offered_group = ssl->handshake->offered_group_id;
    if (offered_group != group) {
        MBEDTLS_SSL_DEBUG_MSG(
            1, ("Invalid server key share, our group %u, their group %u",
                (unsigned) offered_group, (unsigned) group));
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE,
                                     MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
        return MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE;
    }

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_EPHEMERAL_ENABLED)
    if (mbedtls_ssl_tls13_named_group_is_ecdhe(group) ||
        mbedtls_ssl_tls13_named_group_is_ffdh(group)) {
        MBEDTLS_SSL_DEBUG_MSG(2,
                              ("DHE group name: %s", mbedtls_ssl_named_group_to_str(group)));
        ret = mbedtls_ssl_tls13_read_public_xxdhe_share(ssl, p, end - p);
        if (ret != 0) {
            return ret;
        }
    } else
#endif                                                                  
    if (0                  ) {
                          
    } else {
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    return ret;
}

  
                               
                                                     
  
           
                                   
            
  
                                                                      
                                                                       
                                                                    
                                                                        
                                                                       
                                                                     
                                                                  
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_cookie_ext(mbedtls_ssl_context *ssl,
                                      const unsigned char *buf,
                                      const unsigned char *end)
{
    uint16_t cookie_len;
    const unsigned char *p = buf;
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;

                                         
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    cookie_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, cookie_len);
    MBEDTLS_SSL_DEBUG_BUF(3, "cookie extension", p, cookie_len);

    mbedtls_free(handshake->cookie);
    handshake->cookie_len = 0;
    handshake->cookie = mbedtls_calloc(1, cookie_len);
    if (handshake->cookie == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("alloc failed ( %ud bytes )",
                               cookie_len));
        return MBEDTLS_ERR_SSL_ALLOC_FAILED;
    }

    memcpy(handshake->cookie, p, cookie_len);
    handshake->cookie_len = cookie_len;

    return 0;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_cookie_ext(mbedtls_ssl_context *ssl,
                                      unsigned char *buf,
                                      unsigned char *end,
                                      size_t *out_len)
{
    unsigned char *p = buf;
    *out_len = 0;
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;

    if (handshake->cookie == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("no cookie to send; skip extension"));
        return 0;
    }

    MBEDTLS_SSL_DEBUG_BUF(3, "client hello, cookie",
                          handshake->cookie,
                          handshake->cookie_len);

    MBEDTLS_SSL_CHK_BUF_PTR(p, end, handshake->cookie_len + 6);

    MBEDTLS_SSL_DEBUG_MSG(3, ("client hello, adding cookie extension"));

    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_COOKIE, p, 0);
    MBEDTLS_PUT_UINT16_BE(handshake->cookie_len + 2, p, 2);
    MBEDTLS_PUT_UINT16_BE(handshake->cookie_len, p, 4);
    p += 6;

                
    memcpy(p, handshake->cookie, handshake->cookie_len);

    *out_len = handshake->cookie_len + 6;

    mbedtls_ssl_tls13_set_hs_sent_ext_mask(ssl, MBEDTLS_TLS_EXT_COOKIE);

    return 0;
}

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_PSK_ENABLED)
  
                                                          
  
                                                                     
  
           
                                           
                         
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_psk_key_exchange_modes_ext(mbedtls_ssl_context *ssl,
                                                      unsigned char *buf,
                                                      unsigned char *end,
                                                      size_t *out_len)
{
    unsigned char *p = buf;
    int ke_modes_len = 0;

    ((void) ke_modes_len);
    *out_len = 0;

                                                         
                                
       
    if (!mbedtls_ssl_conf_tls13_is_some_psk_enabled(ssl)) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("skip psk_key_exchange_modes extension"));
        return 0;
    }

                                               
                                          
       
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 7);
    MBEDTLS_SSL_DEBUG_MSG(
        3, ("client hello, adding psk_key_exchange_modes extension"));

    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_PSK_KEY_EXCHANGE_MODES, p, 0);

                                          
                                        
       
    p += 5;

    if (mbedtls_ssl_conf_tls13_is_psk_ephemeral_enabled(ssl)) {
        *p++ = MBEDTLS_SSL_TLS1_3_PSK_MODE_ECDHE;
        ke_modes_len++;

        MBEDTLS_SSL_DEBUG_MSG(4, ("Adding PSK-ECDHE key exchange mode"));
    }

    if (mbedtls_ssl_conf_tls13_is_psk_enabled(ssl)) {
        *p++ = MBEDTLS_SSL_TLS1_3_PSK_MODE_PURE;
        ke_modes_len++;

        MBEDTLS_SSL_DEBUG_MSG(4, ("Adding pure PSK key exchange mode"));
    }

                                                     
    MBEDTLS_PUT_UINT16_BE(ke_modes_len + 1, buf, 2);
    buf[4] = ke_modes_len;

    *out_len = p - buf;

    mbedtls_ssl_tls13_set_hs_sent_ext_mask(
        ssl, MBEDTLS_TLS_EXT_PSK_KEY_EXCHANGE_MODES);

    return 0;
}

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
static psa_algorithm_t ssl_tls13_get_ciphersuite_hash_alg(int ciphersuite)
{
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info = NULL;
    ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(ciphersuite);

    if (ciphersuite_info != NULL) {
        return mbedtls_md_psa_alg_from_type((mbedtls_md_type_t) ciphersuite_info->mac);
    }

    return PSA_ALG_NONE;
}

static int ssl_tls13_has_configured_ticket(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_session *session = ssl->session_negotiate;
    return ssl->handshake->resume &&
           session != NULL && session->ticket != NULL &&
           mbedtls_ssl_conf_tls13_is_kex_mode_enabled(
        ssl, mbedtls_ssl_tls13_session_get_ticket_flags(
            session, MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ALL));
}

#if defined(MBEDTLS_SSL_EARLY_DATA)
static int ssl_tls13_early_data_has_valid_ticket(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_session *session = ssl->session_negotiate;
    return ssl->handshake->resume &&
           session->tls_version == MBEDTLS_SSL_VERSION_TLS1_3 &&
           mbedtls_ssl_tls13_session_ticket_allow_early_data(session) &&
           mbedtls_ssl_tls13_cipher_suite_is_offered(ssl, session->ciphersuite);
}
#endif

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_ticket_get_identity(mbedtls_ssl_context *ssl,
                                         psa_algorithm_t *hash_alg,
                                         const unsigned char **identity,
                                         size_t *identity_len)
{
    mbedtls_ssl_session *session = ssl->session_negotiate;

    if (!ssl_tls13_has_configured_ticket(ssl)) {
        return -1;
    }

    *hash_alg = ssl_tls13_get_ciphersuite_hash_alg(session->ciphersuite);
    *identity = session->ticket;
    *identity_len = session->ticket_len;
    return 0;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_ticket_get_psk(mbedtls_ssl_context *ssl,
                                    psa_algorithm_t *hash_alg,
                                    const unsigned char **psk,
                                    size_t *psk_len)
{

    mbedtls_ssl_session *session = ssl->session_negotiate;

    if (!ssl_tls13_has_configured_ticket(ssl)) {
        return -1;
    }

    *hash_alg = ssl_tls13_get_ciphersuite_hash_alg(session->ciphersuite);
    *psk = session->resumption_key;
    *psk_len = session->resumption_key_len;

    return 0;
}
#endif                                  

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_psk_get_identity(mbedtls_ssl_context *ssl,
                                      psa_algorithm_t *hash_alg,
                                      const unsigned char **identity,
                                      size_t *identity_len)
{

    if (!mbedtls_ssl_conf_has_static_psk(ssl->conf)) {
        return -1;
    }

    *hash_alg = PSA_ALG_SHA_256;
    *identity = ssl->conf->psk_identity;
    *identity_len = ssl->conf->psk_identity_len;
    return 0;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_psk_get_psk(mbedtls_ssl_context *ssl,
                                 psa_algorithm_t *hash_alg,
                                 const unsigned char **psk,
                                 size_t *psk_len)
{

    if (!mbedtls_ssl_conf_has_static_psk(ssl->conf)) {
        return -1;
    }

    *hash_alg = PSA_ALG_SHA_256;
    *psk = ssl->conf->psk;
    *psk_len = ssl->conf->psk_len;
    return 0;
}

static int ssl_tls13_get_configured_psk_count(mbedtls_ssl_context *ssl)
{
    int configured_psk_count = 0;
#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    if (ssl_tls13_has_configured_ticket(ssl)) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("Ticket is configured"));
        configured_psk_count++;
    }
#endif
    if (mbedtls_ssl_conf_has_static_psk(ssl->conf)) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("PSK is configured"));
        configured_psk_count++;
    }
    return configured_psk_count;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_identity(mbedtls_ssl_context *ssl,
                                    unsigned char *buf,
                                    unsigned char *end,
                                    const unsigned char *identity,
                                    size_t identity_len,
                                    uint32_t obfuscated_ticket_age,
                                    size_t *out_len)
{
    ((void) ssl);
    *out_len = 0;

      
                                         
                                                        
                                         
       
    MBEDTLS_SSL_CHK_BUF_PTR(buf, end, 6 + identity_len);

    MBEDTLS_PUT_UINT16_BE(identity_len, buf, 0);
    memcpy(buf + 2, identity, identity_len);
    MBEDTLS_PUT_UINT32_BE(obfuscated_ticket_age, buf, 2 + identity_len);

    MBEDTLS_SSL_DEBUG_BUF(4, "write identity", buf, 6 + identity_len);

    *out_len = 6 + identity_len;

    return 0;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_binder(mbedtls_ssl_context *ssl,
                                  unsigned char *buf,
                                  unsigned char *end,
                                  int psk_type,
                                  psa_algorithm_t hash_alg,
                                  const unsigned char *psk,
                                  size_t psk_len,
                                  size_t *out_len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char binder_len;
    unsigned char transcript[MBEDTLS_TLS1_3_MD_MAX_SIZE];
    size_t transcript_len = 0;

    *out_len = 0;

    binder_len = PSA_HASH_LENGTH(hash_alg);

      
                                       
                                                
       
    MBEDTLS_SSL_CHK_BUF_PTR(buf, end, 1 + binder_len);

    buf[0] = binder_len;

                                                    
    ret = mbedtls_ssl_get_handshake_transcript(
        ssl, mbedtls_md_type_from_psa_alg(hash_alg),
        transcript, sizeof(transcript), &transcript_len);
    if (ret != 0) {
        return ret;
    }

    ret = mbedtls_ssl_tls13_create_psk_binder(ssl, hash_alg,
                                              psk, psk_len, psk_type,
                                              transcript, buf + 1);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_tls13_create_psk_binder", ret);
        return ret;
    }
    MBEDTLS_SSL_DEBUG_BUF(4, "write binder", buf, 1 + binder_len);

    *out_len = 1 + binder_len;

    return 0;
}

  
                                                                        
  
           
                                
                                  
                 
  
                                  
  
           
                                       
                                        
                 
  
           
                                  
                                       
           
       
                           
  
   
int mbedtls_ssl_tls13_write_identities_of_pre_shared_key_ext(
    mbedtls_ssl_context *ssl, unsigned char *buf, unsigned char *end,
    size_t *out_len, size_t *binders_len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int configured_psk_count = 0;
    unsigned char *p = buf;
    psa_algorithm_t hash_alg = PSA_ALG_NONE;
    const unsigned char *identity;
    size_t identity_len;
    size_t l_binders_len = 0;
    size_t output_len;

    *out_len = 0;
    *binders_len = 0;

                                                                        
    configured_psk_count = ssl_tls13_get_configured_psk_count(ssl);
    if (configured_psk_count == 0) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("skip pre_shared_key extensions"));
        return 0;
    }

    MBEDTLS_SSL_DEBUG_MSG(4, ("Pre-configured PSK number = %d",
                              configured_psk_count));

                                                                       
                                         
                                         
                                         
       
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 6);
    p += 6;

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    if (ssl_tls13_ticket_get_identity(
            ssl, &hash_alg, &identity, &identity_len) == 0) {
#if defined(MBEDTLS_HAVE_TIME)
        mbedtls_ms_time_t now = mbedtls_ms_time();
        mbedtls_ssl_session *session = ssl->session_negotiate;
                                                                 
                                                                                
                                                                               
                                                           
        uint32_t obfuscated_ticket_age =
            (uint32_t) (now - session->ticket_reception_time);
        obfuscated_ticket_age += session->ticket_age_add;

        ret = ssl_tls13_write_identity(ssl, p, end,
                                       identity, identity_len,
                                       obfuscated_ticket_age,
                                       &output_len);
#else
        ret = ssl_tls13_write_identity(ssl, p, end, identity, identity_len,
                                       0, &output_len);
#endif                        
        if (ret != 0) {
            return ret;
        }

        p += output_len;
        l_binders_len += 1 + PSA_HASH_LENGTH(hash_alg);
    }
#endif                                  

    if (ssl_tls13_psk_get_identity(
            ssl, &hash_alg, &identity, &identity_len) == 0) {

        ret = ssl_tls13_write_identity(ssl, p, end, identity, identity_len, 0,
                                       &output_len);
        if (ret != 0) {
            return ret;
        }

        p += output_len;
        l_binders_len += 1 + PSA_HASH_LENGTH(hash_alg);
    }

    MBEDTLS_SSL_DEBUG_MSG(3,
                          ("client hello, adding pre_shared_key extension, "
                           "omitting PSK binder list"));

                                                                        
    l_binders_len += 2;
                                                    
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, l_binders_len);

      
                                         
                                         
                                         
       
    MBEDTLS_PUT_UINT16_BE(MBEDTLS_TLS_EXT_PRE_SHARED_KEY, buf, 0);
    MBEDTLS_PUT_UINT16_BE(p - buf - 4 + l_binders_len, buf, 2);
    MBEDTLS_PUT_UINT16_BE(p - buf - 6, buf, 4);

    *out_len = (p - buf) + l_binders_len;
    *binders_len = l_binders_len;

    MBEDTLS_SSL_DEBUG_BUF(3, "pre_shared_key identities", buf, p - buf);

    return 0;
}

int mbedtls_ssl_tls13_write_binders_of_pre_shared_key_ext(
    mbedtls_ssl_context *ssl, unsigned char *buf, unsigned char *end)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = buf;
    psa_algorithm_t hash_alg = PSA_ALG_NONE;
    const unsigned char *psk;
    size_t psk_len;
    size_t output_len;

                                                   
                                      
       
    MBEDTLS_SSL_CHK_BUF_PTR(p, end, 2);
    p += 2;

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    if (ssl_tls13_ticket_get_psk(ssl, &hash_alg, &psk, &psk_len) == 0) {

        ret = ssl_tls13_write_binder(ssl, p, end,
                                     MBEDTLS_SSL_TLS1_3_PSK_RESUMPTION,
                                     hash_alg, psk, psk_len,
                                     &output_len);
        if (ret != 0) {
            return ret;
        }
        p += output_len;
    }
#endif                                  

    if (ssl_tls13_psk_get_psk(ssl, &hash_alg, &psk, &psk_len) == 0) {

        ret = ssl_tls13_write_binder(ssl, p, end,
                                     MBEDTLS_SSL_TLS1_3_PSK_EXTERNAL,
                                     hash_alg, psk, psk_len,
                                     &output_len);
        if (ret != 0) {
            return ret;
        }
        p += output_len;
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("client hello, adding PSK binder list."));

      
                                      
       
    MBEDTLS_PUT_UINT16_BE(p - buf - 2, buf, 0);

    MBEDTLS_SSL_DEBUG_BUF(3, "pre_shared_key binders", buf, p - buf);

    mbedtls_ssl_tls13_set_hs_sent_ext_mask(
        ssl, MBEDTLS_TLS_EXT_PRE_SHARED_KEY);

    return 0;
}

  
           
                                
                                  
                 
  
                                  
  
           
  
                                  
              
                                                       
       
  
                           
  
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_server_pre_shared_key_ext(mbedtls_ssl_context *ssl,
                                                     const unsigned char *buf,
                                                     const unsigned char *end)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int selected_identity;
    const unsigned char *psk;
    size_t psk_len;
    psa_algorithm_t hash_alg;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(buf, end, 2);
    selected_identity = MBEDTLS_GET_UINT16_BE(buf, 0);
    ssl->handshake->selected_identity = (uint16_t) selected_identity;

    MBEDTLS_SSL_DEBUG_MSG(3, ("selected_identity = %d", selected_identity));

    if (selected_identity >= ssl_tls13_get_configured_psk_count(ssl)) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("Invalid PSK identity."));

        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                                     MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
    }

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    if (selected_identity == 0 && ssl_tls13_has_configured_ticket(ssl)) {
        ret = ssl_tls13_ticket_get_psk(ssl, &hash_alg, &psk, &psk_len);
    } else
#endif
    if (mbedtls_ssl_conf_has_static_psk(ssl->conf)) {
        ret = ssl_tls13_psk_get_psk(ssl, &hash_alg, &psk, &psk_len);
    } else {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }
    if (ret != 0) {
        return ret;
    }

    if (mbedtls_md_psa_alg_from_type((mbedtls_md_type_t) ssl->handshake->ciphersuite_info->mac)
        != hash_alg) {
        MBEDTLS_SSL_DEBUG_MSG(
            1, ("Invalid ciphersuite for external psk."));

        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                                     MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
    }

    ret = mbedtls_ssl_set_hs_psk(ssl, psk, psk_len);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_set_hs_psk", ret);
        return ret;
    }

    return 0;
}
#endif                                                            

int mbedtls_ssl_tls13_write_client_hello_exts(mbedtls_ssl_context *ssl,
                                              unsigned char *buf,
                                              unsigned char *end,
                                              size_t *out_len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = buf;
    size_t ext_len;

    *out_len = 0;

    ret = mbedtls_ssl_tls13_crypto_init(ssl);
    if (ret != 0) {
        return ret;
    }

                                         
      
                                                              
       
    ret = ssl_tls13_write_supported_versions_ext(ssl, p, end, &ext_len);
    if (ret != 0) {
        return ret;
    }
    p += ext_len;

                                                                  
                                 
       
    ret = ssl_tls13_write_cookie_ext(ssl, p, end, &ext_len);
    if (ret != 0) {
        return ret;
    }
    p += ext_len;

#if defined(MBEDTLS_SSL_RECORD_SIZE_LIMIT)
    ret = mbedtls_ssl_tls13_write_record_size_limit_ext(
        ssl, p, end, &ext_len);
    if (ret != 0) {
        return ret;
    }
    p += ext_len;
#endif

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_EPHEMERAL_ENABLED)
    if (mbedtls_ssl_conf_tls13_is_some_ephemeral_enabled(ssl)) {
        ret = ssl_tls13_write_key_share_ext(ssl, p, end, &ext_len);
        if (ret != 0) {
            return ret;
        }
        p += ext_len;
    }
#endif

#if defined(MBEDTLS_SSL_EARLY_DATA)
                                                                             
                                                 
                                                                        
                                                                           
                                                                  
                                                  
                                             
       
    if (!ssl->handshake->hello_retry_request_flag) {
        if (mbedtls_ssl_conf_tls13_is_some_psk_enabled(ssl) &&
            ssl_tls13_early_data_has_valid_ticket(ssl) &&
            ssl->conf->early_data_enabled == MBEDTLS_SSL_EARLY_DATA_ENABLED) {
            ret = mbedtls_ssl_tls13_write_early_data_ext(
                ssl, 0, p, end, &ext_len);
            if (ret != 0) {
                return ret;
            }
            p += ext_len;

            ssl->early_data_state = MBEDTLS_SSL_EARLY_DATA_STATE_IND_SENT;
        } else {
            ssl->early_data_state = MBEDTLS_SSL_EARLY_DATA_STATE_NO_IND_SENT;
        }
    }
#endif                             

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_PSK_ENABLED)
                                                                      
                                                
      
                                                                     
                                                                        
                                                                      
      
                                                
       
    ret = ssl_tls13_write_psk_key_exchange_modes_ext(ssl, p, end, &ext_len);
    if (ret != 0) {
        return ret;
    }
    p += ext_len;
#endif

    *out_len = p - buf;

    return 0;
}

int mbedtls_ssl_tls13_finalize_client_hello(mbedtls_ssl_context *ssl)
{
    ((void) ssl);

#if defined(MBEDTLS_SSL_EARLY_DATA)
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    psa_algorithm_t hash_alg = PSA_ALG_NONE;
    const unsigned char *psk;
    size_t psk_len;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info;

    if (ssl->early_data_state == MBEDTLS_SSL_EARLY_DATA_STATE_IND_SENT) {
        MBEDTLS_SSL_DEBUG_MSG(
            1, ("Set hs psk for early data when writing the first psk"));

        ret = ssl_tls13_ticket_get_psk(ssl, &hash_alg, &psk, &psk_len);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(
                1, "ssl_tls13_ticket_get_psk", ret);
            return ret;
        }

        ret = mbedtls_ssl_set_hs_psk(ssl, psk, psk_len);
        if (ret  != 0) {
            MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_set_hs_psk", ret);
            return ret;
        }

          
                                                                     
                                                                     
                                                                    
                                                                
                                                                     
                                                                     
                                                                  
                                                            
                                                                        
                                   
           
        ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(
            ssl->session_negotiate->ciphersuite);
        ssl->handshake->ciphersuite_info = ciphersuite_info;

                                                                    
        ssl->handshake->key_exchange_mode =
            MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ALL;

                                          
                                                   
           
        ret = mbedtls_ssl_tls13_key_schedule_stage_early(ssl);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(
                1, "mbedtls_ssl_tls13_key_schedule_stage_early", ret);
            return ret;
        }

                                            
        ret = mbedtls_ssl_tls13_compute_early_transform(ssl);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(
                1, "mbedtls_ssl_tls13_compute_early_transform", ret);
            return ret;
        }

#if defined(MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE)
        mbedtls_ssl_handshake_set_state(
            ssl, MBEDTLS_SSL_CLIENT_CCS_AFTER_CLIENT_HELLO);
#else
        MBEDTLS_SSL_DEBUG_MSG(
            1, ("Switch to early data keys for outbound traffic"));
        mbedtls_ssl_set_outbound_transform(
            ssl, ssl->handshake->transform_earlydata);
        ssl->early_data_state = MBEDTLS_SSL_EARLY_DATA_STATE_CAN_WRITE;
#endif
    }
#endif                             
    return 0;
}
  
                                                    
   

   
                                                                           
                 
  
                              
                                                            
                                                                       
  
                                                                               
                                                                       
                                                                               
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_is_supported_versions_ext_present(
    mbedtls_ssl_context *ssl,
    const unsigned char *buf,
    const unsigned char *end)
{
    const unsigned char *p = buf;
    size_t legacy_session_id_echo_len;
    const unsigned char *supported_versions_data;
    const unsigned char *supported_versions_data_end;

      
                                                                             
              
                                               
                                                                             
                                              
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, MBEDTLS_SERVER_HELLO_RANDOM_LEN + 3);
    p += MBEDTLS_SERVER_HELLO_RANDOM_LEN + 2;
    legacy_session_id_echo_len = *p;

      
                                            
                                                                          
                                           
                                          
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, legacy_session_id_echo_len + 4);
    p += legacy_session_id_echo_len + 4;

    return mbedtls_ssl_tls13_is_supported_versions_ext_present_in_exts(
        ssl, p, end,
        &supported_versions_data, &supported_versions_data_end);
}

                                                     
                                                                            
                                                                      
                
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_is_downgrade_negotiation(mbedtls_ssl_context *ssl,
                                              const unsigned char *buf,
                                              const unsigned char *end)
{
                                                                              
    static const unsigned char magic_downgrade_string[] =
    { 0x44, 0x4F, 0x57, 0x4E, 0x47, 0x52, 0x44 };
    const unsigned char *last_eight_bytes_of_random;
    unsigned char last_byte_of_random;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(buf, end, MBEDTLS_SERVER_HELLO_RANDOM_LEN + 2);
    last_eight_bytes_of_random = buf + 2 + MBEDTLS_SERVER_HELLO_RANDOM_LEN - 8;

    if (memcmp(last_eight_bytes_of_random,
               magic_downgrade_string,
               sizeof(magic_downgrade_string)) == 0) {
        last_byte_of_random = last_eight_bytes_of_random[7];
        return last_byte_of_random == 0 ||
               last_byte_of_random == 1;
    }

    return 0;
}

                                                     
                        
                         
                                                               
   
#define SSL_SERVER_HELLO 0
#define SSL_SERVER_HELLO_HRR 1
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_server_hello_is_hrr(mbedtls_ssl_context *ssl,
                                   const unsigned char *buf,
                                   const unsigned char *end)
{

                                                                         
      
                                                                       
                                                           
      
               
                                                  
                        
                                               
                                   
                                              
                                          
                     
      
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(
        buf, end, 2 + sizeof(mbedtls_ssl_tls13_hello_retry_request_magic));

    if (memcmp(buf + 2, mbedtls_ssl_tls13_hello_retry_request_magic,
               sizeof(mbedtls_ssl_tls13_hello_retry_request_magic)) == 0) {
        return SSL_SERVER_HELLO_HRR;
    }

    return SSL_SERVER_HELLO;
}

  
                                                     
                        
                            
                            
   
#define SSL_SERVER_HELLO_TLS1_2 2
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_preprocess_server_hello(mbedtls_ssl_context *ssl,
                                             const unsigned char *buf,
                                             const unsigned char *end)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;

    MBEDTLS_SSL_PROC_CHK_NEG(ssl_tls13_is_supported_versions_ext_present(
                                 ssl, buf, end));

    if (ret == 0) {
        MBEDTLS_SSL_PROC_CHK_NEG(
            ssl_tls13_is_downgrade_negotiation(ssl, buf, end));

                                                             
                                          
                                                                               
                                                                           
                                                                 
           
        if (handshake->min_tls_version > MBEDTLS_SSL_VERSION_TLS1_2 || ret) {
            MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                                         MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
            return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
        }

          
                                                                   
                                                                            
                                                                              
                                                                                   
                                                      
           
        ssl->keep_current_message = 1;
        ssl->tls_version = MBEDTLS_SSL_VERSION_TLS1_2;
        MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_add_hs_msg_to_checksum(
                                 ssl, MBEDTLS_SSL_HS_SERVER_HELLO,
                                 buf, (size_t) (end - buf)));

        if (mbedtls_ssl_conf_tls13_is_some_ephemeral_enabled(ssl)) {
            ret = ssl_tls13_reset_key_share(ssl);
            if (ret != 0) {
                return ret;
            }
        }

        return SSL_SERVER_HELLO_TLS1_2;
    }

    ssl->session_negotiate->tls_version = ssl->tls_version;
    ssl->session_negotiate->endpoint = ssl->conf->endpoint;

    handshake->received_extensions = MBEDTLS_SSL_EXT_MASK_NONE;

    ret = ssl_server_hello_is_hrr(ssl, buf, end);
    switch (ret) {
        case SSL_SERVER_HELLO:
            MBEDTLS_SSL_DEBUG_MSG(2, ("received ServerHello message"));
            break;
        case SSL_SERVER_HELLO_HRR:
            MBEDTLS_SSL_DEBUG_MSG(2, ("received HelloRetryRequest message"));
                                                                          
                                                                             
                                                                           
                                          
               
            if (handshake->hello_retry_request_flag) {
                MBEDTLS_SSL_DEBUG_MSG(1, ("Multiple HRRs received"));
                MBEDTLS_SSL_PEND_FATAL_ALERT(
                    MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE,
                    MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
                return MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE;
            }
              
                                                                           
                                                                            
                                  
                                                              
               
            if (!mbedtls_ssl_conf_tls13_is_some_ephemeral_enabled(ssl)) {
                MBEDTLS_SSL_DEBUG_MSG(1,
                                      ("Unexpected HRR in pure PSK key exchange."));
                MBEDTLS_SSL_PEND_FATAL_ALERT(
                    MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                    MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
                return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
            }

            handshake->hello_retry_request_flag = 1;

            break;
    }

cleanup:

    return ret;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_check_server_hello_session_id_echo(mbedtls_ssl_context *ssl,
                                                        const unsigned char **buf,
                                                        const unsigned char *end)
{
    const unsigned char *p = *buf;
    size_t legacy_session_id_echo_len;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 1);
    legacy_session_id_echo_len = *p++;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, legacy_session_id_echo_len);

                                
    if (ssl->session_negotiate->id_len != legacy_session_id_echo_len ||
        memcmp(ssl->session_negotiate->id, p, legacy_session_id_echo_len) != 0) {
        MBEDTLS_SSL_DEBUG_BUF(3, "Expected Session ID",
                              ssl->session_negotiate->id,
                              ssl->session_negotiate->id_len);
        MBEDTLS_SSL_DEBUG_BUF(3, "Received Session ID", p,
                              legacy_session_id_echo_len);

        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                                     MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);

        return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
    }

    p += legacy_session_id_echo_len;
    *buf = p;

    MBEDTLS_SSL_DEBUG_BUF(3, "Session ID", ssl->session_negotiate->id,
                          ssl->session_negotiate->id_len);
    return 0;
}

                                                  
  
           
                                                         
                    
                                           
                               
                                          
                                      
                 
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_server_hello(mbedtls_ssl_context *ssl,
                                        const unsigned char *buf,
                                        const unsigned char *end,
                                        int is_hrr)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const unsigned char *p = buf;
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;
    size_t extensions_len;
    const unsigned char *extensions_end;
    uint16_t cipher_suite;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info;
    int fatal_alert = 0;
    uint32_t allowed_extensions_mask;
    int hs_msg_type = is_hrr ? MBEDTLS_SSL_TLS1_3_HS_HELLO_RETRY_REQUEST :
                      MBEDTLS_SSL_HS_SERVER_HELLO;

      
                                              
      
                                              
                                                                           
                                                            
                                              
                                              
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, MBEDTLS_SERVER_HELLO_RANDOM_LEN + 6);

    MBEDTLS_SSL_DEBUG_BUF(4, "server hello", p, end - p);
    MBEDTLS_SSL_DEBUG_BUF(3, "server hello, version", p, 2);

          
                                                          
          
                                       
                              
       
    if (mbedtls_ssl_read_version(p, ssl->conf->transport) !=
        MBEDTLS_SSL_VERSION_TLS1_2) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("Unsupported version of TLS."));
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_PROTOCOL_VERSION,
                                     MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION);
        ret = MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION;
        goto cleanup;
    }
    p += 2;

          
                     
          
                              
                                                      
       
    if (!is_hrr) {
        memcpy(&handshake->randbytes[MBEDTLS_CLIENT_HELLO_RANDOM_LEN], p,
               MBEDTLS_SERVER_HELLO_RANDOM_LEN);
        MBEDTLS_SSL_DEBUG_BUF(3, "server hello, random bytes",
                              p, MBEDTLS_SERVER_HELLO_RANDOM_LEN);
    }
    p += MBEDTLS_SERVER_HELLO_RANDOM_LEN;

          
                                            
          
       
    if (ssl_tls13_check_server_hello_session_id_echo(ssl, &p, end) != 0) {
        fatal_alert = MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER;
        goto cleanup;
    }

          
                                
          
                                   
                            
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    cipher_suite = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;


    ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(cipher_suite);
      
                                                           
       
    if ((mbedtls_ssl_validate_ciphersuite(ssl, ciphersuite_info,
                                          ssl->tls_version,
                                          ssl->tls_version) != 0) ||
        !mbedtls_ssl_tls13_cipher_suite_is_offered(ssl, cipher_suite)) {
        fatal_alert = MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER;
    }
      
                                                                  
                                                                  
                                                              
                                 
       
    else if ((!is_hrr) && handshake->hello_retry_request_flag &&
             (cipher_suite != ssl->session_negotiate->ciphersuite)) {
        fatal_alert = MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER;
    }

    if (fatal_alert == MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("invalid ciphersuite(%04x) parameter",
                                  cipher_suite));
        goto cleanup;
    }

                                
    mbedtls_ssl_optimize_checksum(ssl, ciphersuite_info);

    handshake->ciphersuite_info = ciphersuite_info;
    MBEDTLS_SSL_DEBUG_MSG(3, ("server hello, chosen ciphersuite: ( %04x ) - %s",
                              cipher_suite, ciphersuite_info->name));

#if defined(MBEDTLS_HAVE_TIME)
    ssl->session_negotiate->start = mbedtls_time(NULL);
#endif                        

          
                                           
          
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 1);
    if (p[0] != MBEDTLS_SSL_COMPRESS_NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("bad legacy compression method"));
        fatal_alert = MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER;
        goto cleanup;
    }
    p++;

          
                                       
          
               
                                                   
                                             
                   
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    extensions_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

                                                               
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, extensions_len);
    extensions_end = p + extensions_len;

    MBEDTLS_SSL_DEBUG_BUF(3, "server hello extensions", p, extensions_len);

    handshake->received_extensions = MBEDTLS_SSL_EXT_MASK_NONE;
    allowed_extensions_mask = is_hrr ?
                              MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_HRR :
                              MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_SH;

    while (p < extensions_end) {
        unsigned int extension_type;
        size_t extension_data_len;
        const unsigned char *extension_data_end;

        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, extensions_end, 4);
        extension_type = MBEDTLS_GET_UINT16_BE(p, 0);
        extension_data_len = MBEDTLS_GET_UINT16_BE(p, 2);
        p += 4;

        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, extensions_end, extension_data_len);
        extension_data_end = p + extension_data_len;

        ret = mbedtls_ssl_tls13_check_received_extension(
            ssl, hs_msg_type, extension_type, allowed_extensions_mask);
        if (ret != 0) {
            return ret;
        }

        switch (extension_type) {
            case MBEDTLS_TLS_EXT_COOKIE:

                ret = ssl_tls13_parse_cookie_ext(ssl,
                                                 p, extension_data_end);
                if (ret != 0) {
                    MBEDTLS_SSL_DEBUG_RET(1,
                                          "ssl_tls13_parse_cookie_ext",
                                          ret);
                    goto cleanup;
                }
                break;

            case MBEDTLS_TLS_EXT_SUPPORTED_VERSIONS:
                ret = ssl_tls13_parse_supported_versions_ext(ssl,
                                                             p,
                                                             extension_data_end);
                if (ret != 0) {
                    goto cleanup;
                }
                break;

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_SOME_PSK_ENABLED)
            case MBEDTLS_TLS_EXT_PRE_SHARED_KEY:
                MBEDTLS_SSL_DEBUG_MSG(3, ("found pre_shared_key extension"));

                if ((ret = ssl_tls13_parse_server_pre_shared_key_ext(
                         ssl, p, extension_data_end)) != 0) {
                    MBEDTLS_SSL_DEBUG_RET(
                        1, ("ssl_tls13_parse_server_pre_shared_key_ext"), ret);
                    return ret;
                }
                break;
#endif

            case MBEDTLS_TLS_EXT_KEY_SHARE:
                MBEDTLS_SSL_DEBUG_MSG(3, ("found key_shares extension"));
                if (!mbedtls_ssl_conf_tls13_is_some_ephemeral_enabled(ssl)) {
                    fatal_alert = MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_EXT;
                    goto cleanup;
                }

                if (is_hrr) {
                    ret = ssl_tls13_parse_hrr_key_share_ext(ssl,
                                                            p, extension_data_end);
                } else {
                    ret = ssl_tls13_parse_key_share_ext(ssl,
                                                        p, extension_data_end);
                }
                if (ret != 0) {
                    MBEDTLS_SSL_DEBUG_RET(1,
                                          "ssl_tls13_parse_key_share_ext",
                                          ret);
                    goto cleanup;
                }
                break;

            default:
                ret = MBEDTLS_ERR_SSL_INTERNAL_ERROR;
                goto cleanup;
        }

        p += extension_data_len;
    }

    MBEDTLS_SSL_PRINT_EXTS(3, hs_msg_type, handshake->received_extensions);

cleanup:

    if (fatal_alert == MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_EXT) {
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_UNSUPPORTED_EXT,
                                     MBEDTLS_ERR_SSL_UNSUPPORTED_EXTENSION);
        ret = MBEDTLS_ERR_SSL_UNSUPPORTED_EXTENSION;
    } else if (fatal_alert == MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER) {
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                                     MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        ret = MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
    }
    return ret;
}

#if defined(MBEDTLS_DEBUG_C)
static const char *ssl_tls13_get_kex_mode_str(int mode)
{
    switch (mode) {
        case MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK:
            return "psk";
        case MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL:
            return "ephemeral";
        case MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL:
            return "psk_ephemeral";
        default:
            return "unknown mode";
    }
}
#endif                      

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_postprocess_server_hello(mbedtls_ssl_context *ssl)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;

                                       
                                                                           
                                                           
                                                                        
                                    
                                                                   
                                          
       
    switch (handshake->received_extensions &
            (MBEDTLS_SSL_EXT_MASK(PRE_SHARED_KEY) |
             MBEDTLS_SSL_EXT_MASK(KEY_SHARE))) {
                                                            
        case MBEDTLS_SSL_EXT_MASK(PRE_SHARED_KEY):
            handshake->key_exchange_mode =
                MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK;
            break;

                                                       
        case MBEDTLS_SSL_EXT_MASK(KEY_SHARE):
            handshake->key_exchange_mode =
                MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL;
            break;

                                                                            
        case (MBEDTLS_SSL_EXT_MASK(PRE_SHARED_KEY) |
              MBEDTLS_SSL_EXT_MASK(KEY_SHARE)):
            handshake->key_exchange_mode =
                MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL;
            break;

                                                                         
        default:
            MBEDTLS_SSL_DEBUG_MSG(1, ("Unknown key exchange."));
            ret = MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE;
            goto cleanup;
    }

    if (!mbedtls_ssl_conf_tls13_is_kex_mode_enabled(
            ssl, handshake->key_exchange_mode)) {
        ret = MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE;
        MBEDTLS_SSL_DEBUG_MSG(
            2, ("Key exchange mode(%s) is not supported.",
                ssl_tls13_get_kex_mode_str(handshake->key_exchange_mode)));
        goto cleanup;
    }

    MBEDTLS_SSL_DEBUG_MSG(
        3, ("Selected key exchange mode: %s",
            ssl_tls13_get_kex_mode_str(handshake->key_exchange_mode)));

                                                            
      
                                                                
                                                                   
                                                                  
                                                            
                                                                
                                
       
#if defined(MBEDTLS_SSL_EARLY_DATA)
    if (ssl->early_data_state == MBEDTLS_SSL_EARLY_DATA_STATE_NO_IND_SENT ||
        handshake->key_exchange_mode ==
        MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL)
#endif
    {
        ret = mbedtls_ssl_tls13_key_schedule_stage_early(ssl);
        if (ret != 0) {
            MBEDTLS_SSL_DEBUG_RET(
                1, "mbedtls_ssl_tls13_key_schedule_stage_early", ret);
            goto cleanup;
        }
    }

    ret = mbedtls_ssl_tls13_compute_handshake_transform(ssl);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1,
                              "mbedtls_ssl_tls13_compute_handshake_transform",
                              ret);
        goto cleanup;
    }

    mbedtls_ssl_set_inbound_transform(ssl, handshake->transform_handshake);
    MBEDTLS_SSL_DEBUG_MSG(1, ("Switch to handshake keys for inbound traffic"));
    ssl->session_in = ssl->session_negotiate;

cleanup:
    if (ret != 0) {
        MBEDTLS_SSL_PEND_FATAL_ALERT(
            MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE,
            MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
    }

    return ret;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_postprocess_hrr(mbedtls_ssl_context *ssl)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    mbedtls_ssl_session_reset_msg_layer(ssl, 0);

      
                                                                             
                                                                             
                                                           
                                     
       
    ret = ssl_tls13_reset_key_share(ssl);
    if (ret != 0) {
        return ret;
    }

    ssl->session_negotiate->ciphersuite = ssl->handshake->ciphersuite_info->id;

#if defined(MBEDTLS_SSL_EARLY_DATA)
    if (ssl->early_data_state != MBEDTLS_SSL_EARLY_DATA_STATE_NO_IND_SENT) {
        ssl->early_data_state = MBEDTLS_SSL_EARLY_DATA_STATE_REJECTED;
    }
#endif

    return 0;
}

  
                                                
                                       
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_process_server_hello(mbedtls_ssl_context *ssl)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *buf = NULL;
    size_t buf_len = 0;
    int is_hrr = 0;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> %s", __func__));

    MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_tls13_fetch_handshake_msg(
                             ssl, MBEDTLS_SSL_HS_SERVER_HELLO, &buf, &buf_len));

    ret = ssl_tls13_preprocess_server_hello(ssl, buf, buf + buf_len);
    if (ret < 0) {
        goto cleanup;
    } else {
        is_hrr = (ret == SSL_SERVER_HELLO_HRR);
    }

    if (ret == SSL_SERVER_HELLO_TLS1_2) {
        ret = 0;
        goto cleanup;
    }

      
                                                                         
                                                                          
                                             
       
    if (ssl->in_hslen != ssl->in_msglen) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("ServerHello end not aligned with a record boundary"));
        ret = MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE;
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_UNEXPECTED_MESSAGE,
                                     MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE);
        goto cleanup;
    }

    MBEDTLS_SSL_PROC_CHK(ssl_tls13_parse_server_hello(ssl, buf,
                                                      buf + buf_len,
                                                      is_hrr));
    if (is_hrr) {
        MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_reset_transcript_for_hrr(ssl));
    }

    MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_add_hs_msg_to_checksum(
                             ssl, MBEDTLS_SSL_HS_SERVER_HELLO, buf, buf_len));

    if (is_hrr) {
        MBEDTLS_SSL_PROC_CHK(ssl_tls13_postprocess_hrr(ssl));
#if defined(MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE)
                                                                          
                                                                          
                                                                           
           
        mbedtls_ssl_handshake_set_state(
            ssl, MBEDTLS_SSL_CLIENT_CCS_BEFORE_2ND_CLIENT_HELLO);
#else
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_HELLO);
#endif                                            
    } else {
        MBEDTLS_SSL_PROC_CHK(ssl_tls13_postprocess_server_hello(ssl));
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_ENCRYPTED_EXTENSIONS);
    }

cleanup:
    MBEDTLS_SSL_DEBUG_MSG(2, ("<= %s ( %s )", __func__,
                              is_hrr ? "HelloRetryRequest" : "ServerHello"));
    return ret;
}

  
  
                                               
  
                                                                
                                                                   
                             
   

                                    
           
                                       
                         
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_encrypted_extensions(mbedtls_ssl_context *ssl,
                                                const unsigned char *buf,
                                                const unsigned char *end)
{
    int ret = 0;
    size_t extensions_len;
    const unsigned char *p = buf;
    const unsigned char *extensions_end;
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    extensions_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, extensions_len);
    extensions_end = p + extensions_len;

    MBEDTLS_SSL_DEBUG_BUF(3, "encrypted extensions", p, extensions_len);

    handshake->received_extensions = MBEDTLS_SSL_EXT_MASK_NONE;

    while (p < extensions_end) {
        unsigned int extension_type;
        size_t extension_data_len;

          
                   
                                                      
                                                
                       
           
        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, extensions_end, 4);
        extension_type = MBEDTLS_GET_UINT16_BE(p, 0);
        extension_data_len = MBEDTLS_GET_UINT16_BE(p, 2);
        p += 4;

        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, extensions_end, extension_data_len);

        ret = mbedtls_ssl_tls13_check_received_extension(
            ssl, MBEDTLS_SSL_HS_ENCRYPTED_EXTENSIONS, extension_type,
            MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_EE);
        if (ret != 0) {
            return ret;
        }

        switch (extension_type) {
#if defined(MBEDTLS_SSL_ALPN)
            case MBEDTLS_TLS_EXT_ALPN:
                MBEDTLS_SSL_DEBUG_MSG(3, ("found alpn extension"));

                if ((ret = ssl_tls13_parse_alpn_ext(
                         ssl, p, (size_t) extension_data_len)) != 0) {
                    return ret;
                }

                break;
#endif                       

#if defined(MBEDTLS_SSL_EARLY_DATA)
            case MBEDTLS_TLS_EXT_EARLY_DATA:

                if (extension_data_len != 0) {
                                                    
                    MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR,
                                                 MBEDTLS_ERR_SSL_DECODE_ERROR);
                    return MBEDTLS_ERR_SSL_DECODE_ERROR;
                }

                break;
#endif                             

#if defined(MBEDTLS_SSL_RECORD_SIZE_LIMIT)
            case MBEDTLS_TLS_EXT_RECORD_SIZE_LIMIT:
                MBEDTLS_SSL_DEBUG_MSG(3, ("found record_size_limit extension"));

                ret = mbedtls_ssl_tls13_parse_record_size_limit_ext(
                    ssl, p, p + extension_data_len);
                if (ret != 0) {
                    MBEDTLS_SSL_DEBUG_RET(
                        1, ("mbedtls_ssl_tls13_parse_record_size_limit_ext"), ret);
                    return ret;
                }
                break;
#endif                                    

            default:
                MBEDTLS_SSL_PRINT_EXT(
                    3, MBEDTLS_SSL_HS_ENCRYPTED_EXTENSIONS,
                    extension_type, "( ignored )");
                break;
        }

        p += extension_data_len;
    }

    if ((handshake->received_extensions & MBEDTLS_SSL_EXT_MASK(RECORD_SIZE_LIMIT)) &&
        (handshake->received_extensions & MBEDTLS_SSL_EXT_MASK(MAX_FRAGMENT_LENGTH))) {
        MBEDTLS_SSL_DEBUG_MSG(3,
                              (
                                  "Record size limit extension cannot be used with max fragment length extension"));
        MBEDTLS_SSL_PEND_FATAL_ALERT(
            MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
            MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
        return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
    }

    MBEDTLS_SSL_PRINT_EXTS(3, MBEDTLS_SSL_HS_ENCRYPTED_EXTENSIONS,
                           handshake->received_extensions);

                                                 
    if (p != end) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("EncryptedExtension lengths misaligned"));
        MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR,
                                     MBEDTLS_ERR_SSL_DECODE_ERROR);
        return MBEDTLS_ERR_SSL_DECODE_ERROR;
    }

    return ret;
}

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_process_encrypted_extensions(mbedtls_ssl_context *ssl)
{
    int ret;
    unsigned char *buf;
    size_t buf_len;
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse encrypted extensions"));

    MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_tls13_fetch_handshake_msg(
                             ssl, MBEDTLS_SSL_HS_ENCRYPTED_EXTENSIONS,
                             &buf, &buf_len));

                                      
    MBEDTLS_SSL_PROC_CHK(
        ssl_tls13_parse_encrypted_extensions(ssl, buf, buf + buf_len));

#if defined(MBEDTLS_SSL_EARLY_DATA)
    if (handshake->received_extensions & MBEDTLS_SSL_EXT_MASK(EARLY_DATA)) {
                         
                                                                
                                                                 
                                                                
                                                                 
          
                          
                                                                             
                                                                          
                                                                           
                                                                     
                        
                                   
                                      
                                                         
          
                                                                       
                                                                    
                                                                      
                                                              
                  
                                                      
                                                                          
                           
                                                                          
                                              
           
        if ((!mbedtls_ssl_tls13_key_exchange_mode_with_psk(ssl)) ||
            handshake->selected_identity != 0 ||
            handshake->ciphersuite_info->id !=
            ssl->session_negotiate->ciphersuite) {

            MBEDTLS_SSL_PEND_FATAL_ALERT(
                MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
                MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER);
            return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
        }

        ssl->early_data_state = MBEDTLS_SSL_EARLY_DATA_STATE_ACCEPTED;
    } else if (ssl->early_data_state !=
               MBEDTLS_SSL_EARLY_DATA_STATE_NO_IND_SENT) {
        ssl->early_data_state = MBEDTLS_SSL_EARLY_DATA_STATE_REJECTED;
    }
#endif

      
                                                                      
                                                                             
                                                                             
                                                                        
                                                                         
                                                                     
                                                                          
                                              
       
    ssl->session_negotiate->ciphersuite = handshake->ciphersuite_info->id;

    MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_add_hs_msg_to_checksum(
                             ssl, MBEDTLS_SSL_HS_ENCRYPTED_EXTENSIONS,
                             buf, buf_len));

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
    if (mbedtls_ssl_tls13_key_exchange_mode_with_psk(ssl)) {
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_SERVER_FINISHED);

                                                                               
        ssl->session_negotiate->verify_result = 0;
    } else {
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CERTIFICATE_REQUEST);
    }
#else
    ((void) ssl);
    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_SERVER_FINISHED);
#endif

cleanup:

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse encrypted extensions"));
    return ret;

}

#if defined(MBEDTLS_SSL_EARLY_DATA)
  
                                            
  
                       
  
                            
  
                                                                           
                                                                        
                                                                           
   

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_end_of_early_data(mbedtls_ssl_context *ssl)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *buf = NULL;
    size_t buf_len;
    MBEDTLS_SSL_DEBUG_MSG(2, ("=> write EndOfEarlyData"));

    MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_start_handshake_msg(
                             ssl, MBEDTLS_SSL_HS_END_OF_EARLY_DATA,
                             &buf, &buf_len));

    MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_add_hs_hdr_to_checksum(
                             ssl, MBEDTLS_SSL_HS_END_OF_EARLY_DATA, 0));

    MBEDTLS_SSL_PROC_CHK(
        mbedtls_ssl_finish_handshake_msg(ssl, buf_len, 0));

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_CERTIFICATE);

cleanup:

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= write EndOfEarlyData"));
    return ret;
}

int mbedtls_ssl_get_early_data_status(mbedtls_ssl_context *ssl)
{
    if ((ssl->conf->endpoint != MBEDTLS_SSL_IS_CLIENT) ||
        (!mbedtls_ssl_is_handshake_over(ssl))) {
        return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
    }

    switch (ssl->early_data_state) {
        case MBEDTLS_SSL_EARLY_DATA_STATE_NO_IND_SENT:
            return MBEDTLS_SSL_EARLY_DATA_STATUS_NOT_INDICATED;
            break;

        case MBEDTLS_SSL_EARLY_DATA_STATE_REJECTED:
            return MBEDTLS_SSL_EARLY_DATA_STATUS_REJECTED;
            break;

        case MBEDTLS_SSL_EARLY_DATA_STATE_SERVER_FINISHED_RECEIVED:
            return MBEDTLS_SSL_EARLY_DATA_STATUS_ACCEPTED;
            break;

        default:
            return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }
}
#endif                             

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
  
                                     
  
   
#define SSL_CERTIFICATE_REQUEST_EXPECT_REQUEST 0
#define SSL_CERTIFICATE_REQUEST_SKIP           1
                
                                                                  
                                                       
                                           
                                 
                                                          
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_certificate_request_coordinate(mbedtls_ssl_context *ssl)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    if ((ret = mbedtls_ssl_read_record(ssl, 0)) != 0) {
        MBEDTLS_SSL_DEBUG_RET(1, "mbedtls_ssl_read_record", ret);
        return ret;
    }
    ssl->keep_current_message = 1;

    if ((ssl->in_msgtype == MBEDTLS_SSL_MSG_HANDSHAKE) &&
        (ssl->in_msg[0] == MBEDTLS_SSL_HS_CERTIFICATE_REQUEST)) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("got a certificate request"));
        return SSL_CERTIFICATE_REQUEST_EXPECT_REQUEST;
    }

    MBEDTLS_SSL_DEBUG_MSG(3, ("got no certificate request"));

    return SSL_CERTIFICATE_REQUEST_SKIP;
}

  
                                        
                                
           
                                                  
                                     
                        
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_certificate_request(mbedtls_ssl_context *ssl,
                                               const unsigned char *buf,
                                               const unsigned char *end)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const unsigned char *p = buf;
    size_t certificate_request_context_len = 0;
    size_t extensions_len = 0;
    const unsigned char *extensions_end;
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;

          
                                                   
          
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 1);
    certificate_request_context_len = (size_t) p[0];
    p += 1;

    if (certificate_request_context_len > 0) {
        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, certificate_request_context_len);
        MBEDTLS_SSL_DEBUG_BUF(3, "Certificate Request Context",
                              p, certificate_request_context_len);

        handshake->certificate_request_context =
            mbedtls_calloc(1, certificate_request_context_len);
        if (handshake->certificate_request_context == NULL) {
            MBEDTLS_SSL_DEBUG_MSG(1, ("buffer too small"));
            return MBEDTLS_ERR_SSL_ALLOC_FAILED;
        }
        memcpy(handshake->certificate_request_context, p,
               certificate_request_context_len);
        p += certificate_request_context_len;
    }

          
                                       
          
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    extensions_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, extensions_len);
    extensions_end = p + extensions_len;

    handshake->received_extensions = MBEDTLS_SSL_EXT_MASK_NONE;

    while (p < extensions_end) {
        unsigned int extension_type;
        size_t extension_data_len;

        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, extensions_end, 4);
        extension_type = MBEDTLS_GET_UINT16_BE(p, 0);
        extension_data_len = MBEDTLS_GET_UINT16_BE(p, 2);
        p += 4;

        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, extensions_end, extension_data_len);

        ret = mbedtls_ssl_tls13_check_received_extension(
            ssl, MBEDTLS_SSL_HS_CERTIFICATE_REQUEST, extension_type,
            MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_CR);
        if (ret != 0) {
            return ret;
        }

        switch (extension_type) {
            case MBEDTLS_TLS_EXT_SIG_ALG:
                MBEDTLS_SSL_DEBUG_MSG(3,
                                      ("found signature algorithms extension"));
                ret = mbedtls_ssl_parse_sig_alg_ext(ssl, p,
                                                    p + extension_data_len);
                if (ret != 0) {
                    return ret;
                }

                break;

            default:
                MBEDTLS_SSL_PRINT_EXT(
                    3, MBEDTLS_SSL_HS_CERTIFICATE_REQUEST,
                    extension_type, "( ignored )");
                break;
        }

        p += extension_data_len;
    }

    MBEDTLS_SSL_PRINT_EXTS(3, MBEDTLS_SSL_HS_CERTIFICATE_REQUEST,
                           handshake->received_extensions);

                                                 
    if (p != end) {
        MBEDTLS_SSL_DEBUG_MSG(1,
                              ("CertificateRequest misaligned"));
        goto decode_error;
    }

                             
      
                                                             
       
    if ((handshake->received_extensions & MBEDTLS_SSL_EXT_MASK(SIG_ALG)) == 0) {
        MBEDTLS_SSL_DEBUG_MSG(3,
                              ("no signature algorithms extension found"));
        goto decode_error;
    }

    ssl->handshake->client_auth = 1;
    return 0;

decode_error:
    MBEDTLS_SSL_PEND_FATAL_ALERT(MBEDTLS_SSL_ALERT_MSG_DECODE_ERROR,
                                 MBEDTLS_ERR_SSL_DECODE_ERROR);
    return MBEDTLS_ERR_SSL_DECODE_ERROR;
}

  
                                               
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_process_certificate_request(mbedtls_ssl_context *ssl)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse certificate request"));

    MBEDTLS_SSL_PROC_CHK_NEG(ssl_tls13_certificate_request_coordinate(ssl));

    if (ret == SSL_CERTIFICATE_REQUEST_EXPECT_REQUEST) {
        unsigned char *buf;
        size_t buf_len;

        MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_tls13_fetch_handshake_msg(
                                 ssl, MBEDTLS_SSL_HS_CERTIFICATE_REQUEST,
                                 &buf, &buf_len));

        MBEDTLS_SSL_PROC_CHK(ssl_tls13_parse_certificate_request(
                                 ssl, buf, buf + buf_len));

        MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_add_hs_msg_to_checksum(
                                 ssl, MBEDTLS_SSL_HS_CERTIFICATE_REQUEST,
                                 buf, buf_len));
    } else if (ret == SSL_CERTIFICATE_REQUEST_SKIP) {
        ret = 0;
    } else {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        ret = MBEDTLS_ERR_SSL_INTERNAL_ERROR;
        goto cleanup;
    }

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_SERVER_CERTIFICATE);

cleanup:

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse certificate request"));
    return ret;
}

  
                                             
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_process_server_certificate(mbedtls_ssl_context *ssl)
{
    int ret;

    ret = mbedtls_ssl_tls13_process_certificate(ssl);
    if (ret != 0) {
        return ret;
    }

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CERTIFICATE_VERIFY);
    return 0;
}

  
                                             
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_process_certificate_verify(mbedtls_ssl_context *ssl)
{
    int ret;

    ret = mbedtls_ssl_tls13_process_certificate_verify(ssl);
    if (ret != 0) {
        return ret;
    }

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_SERVER_FINISHED);
    return 0;
}
#endif                                                             

  
                                          
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_process_server_finished(mbedtls_ssl_context *ssl)
{
    int ret;

    ret = mbedtls_ssl_tls13_process_finished_message(ssl);
    if (ret != 0) {
        return ret;
    }

    ret = mbedtls_ssl_tls13_compute_application_transform(ssl);
    if (ret != 0) {
        MBEDTLS_SSL_PEND_FATAL_ALERT(
            MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE,
            MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE);
        return ret;
    }

#if defined(MBEDTLS_SSL_EARLY_DATA)
    if (ssl->early_data_state == MBEDTLS_SSL_EARLY_DATA_STATE_ACCEPTED) {
        ssl->early_data_state = MBEDTLS_SSL_EARLY_DATA_STATE_SERVER_FINISHED_RECEIVED;
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_END_OF_EARLY_DATA);
    } else
#endif                             
    {
#if defined(MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE)
        mbedtls_ssl_handshake_set_state(
            ssl, MBEDTLS_SSL_CLIENT_CCS_AFTER_SERVER_FINISHED);
#else
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_CERTIFICATE);
#endif                                            
    }

    return 0;
}

  
                                             
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_client_certificate(mbedtls_ssl_context *ssl)
{
    int non_empty_certificate_msg = 0;

    MBEDTLS_SSL_DEBUG_MSG(1,
                          ("Switch to handshake traffic keys for outbound traffic"));
    mbedtls_ssl_set_outbound_transform(ssl, ssl->handshake->transform_handshake);

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
    if (ssl->handshake->client_auth) {
        int ret = mbedtls_ssl_tls13_write_certificate(ssl);
        if (ret != 0) {
            return ret;
        }

        if (mbedtls_ssl_own_cert(ssl) != NULL) {
            non_empty_certificate_msg = 1;
        }
    } else {
        MBEDTLS_SSL_DEBUG_MSG(2, ("skip write certificate"));
    }
#endif

    if (non_empty_certificate_msg) {
        mbedtls_ssl_handshake_set_state(ssl,
                                        MBEDTLS_SSL_CLIENT_CERTIFICATE_VERIFY);
    } else {
        MBEDTLS_SSL_DEBUG_MSG(2, ("skip write certificate verify"));
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_FINISHED);
    }

    return 0;
}

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
  
                                                    
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_client_certificate_verify(mbedtls_ssl_context *ssl)
{
    int ret = mbedtls_ssl_tls13_write_certificate_verify(ssl);

    if (ret == 0) {
        mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_FINISHED);
    }

    return ret;
}
#endif                                                             

  
                                          
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_write_client_finished(mbedtls_ssl_context *ssl)
{
    int ret;

    ret = mbedtls_ssl_tls13_write_finished_message(ssl);
    if (ret != 0) {
        return ret;
    }

    ret = mbedtls_ssl_tls13_compute_resumption_master_secret(ssl);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(
            1, "mbedtls_ssl_tls13_compute_resumption_master_secret ", ret);
        return ret;
    }

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_FLUSH_BUFFERS);
    return 0;
}

  
                                        
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_flush_buffers(mbedtls_ssl_context *ssl)
{
    MBEDTLS_SSL_DEBUG_MSG(2, ("handshake: done"));
    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_HANDSHAKE_WRAPUP);
    return 0;
}

  
                                           
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_handshake_wrapup(mbedtls_ssl_context *ssl)
{

    mbedtls_ssl_tls13_handshake_wrapup(ssl);

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_HANDSHAKE_OVER);
    return 0;
}

#if defined(MBEDTLS_SSL_SESSION_TICKETS)

#if defined(MBEDTLS_SSL_EARLY_DATA)
                               
  
           
                                    
                                                                 
              
         
                         
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_new_session_ticket_early_data_ext(
    mbedtls_ssl_context *ssl,
    const unsigned char *buf,
    const unsigned char *end)
{
    mbedtls_ssl_session *session = ssl->session;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(buf, end, 4);

    session->max_early_data_size = MBEDTLS_GET_UINT32_BE(buf, 0);
    mbedtls_ssl_tls13_session_set_ticket_flags(
        session, MBEDTLS_SSL_TLS1_3_TICKET_ALLOW_EARLY_DATA);
    MBEDTLS_SSL_DEBUG_MSG(
        3, ("received max_early_data_size: %u",
            (unsigned int) session->max_early_data_size));

    return 0;
}
#endif                             

MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_new_session_ticket_exts(mbedtls_ssl_context *ssl,
                                                   const unsigned char *buf,
                                                   const unsigned char *end)
{
    mbedtls_ssl_handshake_params *handshake = ssl->handshake;
    const unsigned char *p = buf;


    handshake->received_extensions = MBEDTLS_SSL_EXT_MASK_NONE;

    while (p < end) {
        unsigned int extension_type;
        size_t extension_data_len;
        int ret;

        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 4);
        extension_type = MBEDTLS_GET_UINT16_BE(p, 0);
        extension_data_len = MBEDTLS_GET_UINT16_BE(p, 2);
        p += 4;

        MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, extension_data_len);

        ret = mbedtls_ssl_tls13_check_received_extension(
            ssl, MBEDTLS_SSL_HS_NEW_SESSION_TICKET, extension_type,
            MBEDTLS_SSL_TLS1_3_ALLOWED_EXTS_OF_NST);
        if (ret != 0) {
            return ret;
        }

        switch (extension_type) {
#if defined(MBEDTLS_SSL_EARLY_DATA)
            case MBEDTLS_TLS_EXT_EARLY_DATA:
                ret = ssl_tls13_parse_new_session_ticket_early_data_ext(
                    ssl, p, p + extension_data_len);
                if (ret != 0) {
                    MBEDTLS_SSL_DEBUG_RET(
                        1, "ssl_tls13_parse_new_session_ticket_early_data_ext",
                        ret);
                    return ret;
                }
                break;
#endif                             

            default:
                MBEDTLS_SSL_PRINT_EXT(
                    3, MBEDTLS_SSL_HS_NEW_SESSION_TICKET,
                    extension_type, "( ignored )");
                break;
        }

        p +=  extension_data_len;
    }

    MBEDTLS_SSL_PRINT_EXTS(3, MBEDTLS_SSL_HS_NEW_SESSION_TICKET,
                           handshake->received_extensions);

    return 0;
}

  
                        
  
           
                             
                            
                                  
                               
                                      
                      
  
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_parse_new_session_ticket(mbedtls_ssl_context *ssl,
                                              unsigned char *buf,
                                              unsigned char *end,
                                              unsigned char **ticket_nonce,
                                              size_t *ticket_nonce_len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = buf;
    mbedtls_ssl_session *session = ssl->session;
    size_t ticket_len;
    unsigned char *ticket;
    size_t extensions_len;

    *ticket_nonce = NULL;
    *ticket_nonce_len = 0;
      
                                   
                                   
                                  
       
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 9);

    session->ticket_lifetime = MBEDTLS_GET_UINT32_BE(p, 0);
    MBEDTLS_SSL_DEBUG_MSG(3,
                          ("ticket_lifetime: %u",
                           (unsigned int) session->ticket_lifetime));
    if (session->ticket_lifetime >
        MBEDTLS_SSL_TLS1_3_MAX_ALLOWED_TICKET_LIFETIME) {
        MBEDTLS_SSL_DEBUG_MSG(3, ("ticket_lifetime exceeds 7 days."));
        return MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER;
    }

    session->ticket_age_add = MBEDTLS_GET_UINT32_BE(p, 4);
    MBEDTLS_SSL_DEBUG_MSG(3,
                          ("ticket_age_add: %u",
                           (unsigned int) session->ticket_age_add));

    *ticket_nonce_len = p[8];
    p += 9;

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, *ticket_nonce_len);
    *ticket_nonce = p;
    MBEDTLS_SSL_DEBUG_BUF(3, "ticket_nonce:", *ticket_nonce, *ticket_nonce_len);
    p += *ticket_nonce_len;

                
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    ticket_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, ticket_len);
    MBEDTLS_SSL_DEBUG_BUF(3, "received ticket", p, ticket_len);

                                                           
    if (session->ticket != NULL || session->ticket_len > 0) {
        mbedtls_free(session->ticket);
        session->ticket = NULL;
        session->ticket_len = 0;
    }

    if ((ticket = mbedtls_calloc(1, ticket_len)) == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("ticket alloc failed"));
        return MBEDTLS_ERR_SSL_ALLOC_FAILED;
    }
    memcpy(ticket, p, ticket_len);
    p += ticket_len;
    session->ticket = ticket;
    session->ticket_len = ticket_len;

                                         
    mbedtls_ssl_tls13_session_clear_ticket_flags(
        session, MBEDTLS_SSL_TLS1_3_TICKET_FLAGS_MASK);

    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, 2);
    extensions_len = MBEDTLS_GET_UINT16_BE(p, 0);
    p += 2;
    MBEDTLS_SSL_CHK_BUF_READ_PTR(p, end, extensions_len);

    MBEDTLS_SSL_DEBUG_BUF(3, "ticket extension", p, extensions_len);

    ret = ssl_tls13_parse_new_session_ticket_exts(ssl, p, p + extensions_len);
    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(1,
                              "ssl_tls13_parse_new_session_ticket_exts",
                              ret);
        return ret;
    }

    return 0;
}

                                                                             
                                                                           
                                                       
                                                                        
                                                                         
   
#define POSTPROCESS_NEW_SESSION_TICKET_SIGNAL 0
#define POSTPROCESS_NEW_SESSION_TICKET_DISCARD 1
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_postprocess_new_session_ticket(mbedtls_ssl_context *ssl,
                                                    unsigned char *ticket_nonce,
                                                    size_t ticket_nonce_len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_ssl_session *session = ssl->session;
    const mbedtls_ssl_ciphersuite_t *ciphersuite_info;
    psa_algorithm_t psa_hash_alg;
    int hash_length;

    if (session->ticket_lifetime == 0) {
        return POSTPROCESS_NEW_SESSION_TICKET_DISCARD;
    }

#if defined(MBEDTLS_HAVE_TIME)
                                    
    session->ticket_reception_time = mbedtls_ms_time();
#endif

    ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(session->ciphersuite);
    if (ciphersuite_info == NULL) {
        MBEDTLS_SSL_DEBUG_MSG(1, ("should never happen"));
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    psa_hash_alg = mbedtls_md_psa_alg_from_type((mbedtls_md_type_t) ciphersuite_info->mac);
    hash_length = PSA_HASH_LENGTH(psa_hash_alg);
    if (hash_length == -1 ||
        (size_t) hash_length > sizeof(session->resumption_key)) {
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }


    MBEDTLS_SSL_DEBUG_BUF(3, "resumption_master_secret",
                          session->app_secrets.resumption_master_secret,
                          hash_length);

                             
      
                                                    
                                                                   
       
    ret = mbedtls_ssl_tls13_hkdf_expand_label(
        psa_hash_alg,
        session->app_secrets.resumption_master_secret,
        hash_length,
        MBEDTLS_SSL_TLS1_3_LBL_WITH_LEN(resumption),
        ticket_nonce,
        ticket_nonce_len,
        session->resumption_key,
        hash_length);

    if (ret != 0) {
        MBEDTLS_SSL_DEBUG_RET(2,
                              "Creating the ticket-resumed PSK failed",
                              ret);
        return ret;
    }

    session->resumption_key_len = hash_length;

    MBEDTLS_SSL_DEBUG_BUF(3, "Ticket-resumed PSK",
                          session->resumption_key,
                          session->resumption_key_len);

                                                                     
    mbedtls_ssl_tls13_session_set_ticket_flags(
        session, ssl->conf->tls13_kex_modes);
    MBEDTLS_SSL_PRINT_TICKET_FLAGS(4, session->ticket_flags);

    return POSTPROCESS_NEW_SESSION_TICKET_SIGNAL;
}

  
                                                    
   
MBEDTLS_CHECK_RETURN_CRITICAL
static int ssl_tls13_process_new_session_ticket(mbedtls_ssl_context *ssl)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *buf;
    size_t buf_len;
    unsigned char *ticket_nonce;
    size_t ticket_nonce_len;

    MBEDTLS_SSL_DEBUG_MSG(2, ("=> parse new session ticket"));

    MBEDTLS_SSL_PROC_CHK(mbedtls_ssl_tls13_fetch_handshake_msg(
                             ssl, MBEDTLS_SSL_HS_NEW_SESSION_TICKET,
                             &buf, &buf_len));

      
                                                                           
                                             
       
    ssl->session->exported = 1;

    MBEDTLS_SSL_PROC_CHK(ssl_tls13_parse_new_session_ticket(
                             ssl, buf, buf + buf_len,
                             &ticket_nonce, &ticket_nonce_len));

    MBEDTLS_SSL_PROC_CHK_NEG(ssl_tls13_postprocess_new_session_ticket(
                                 ssl, ticket_nonce, ticket_nonce_len));

    switch (ret) {
        case POSTPROCESS_NEW_SESSION_TICKET_SIGNAL:
              
                                                                              
                                                                           
               
            ssl->session->exported = 0;
            ret = MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET;
            break;

        case POSTPROCESS_NEW_SESSION_TICKET_DISCARD:
            ret = 0;
            MBEDTLS_SSL_DEBUG_MSG(2, ("Discard new session ticket"));
            break;

        default:
            ret = MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_HANDSHAKE_OVER);

cleanup:

    MBEDTLS_SSL_DEBUG_MSG(2, ("<= parse new session ticket"));
    return ret;
}
#endif                                  

int mbedtls_ssl_tls13_handshake_client_step(mbedtls_ssl_context *ssl)
{
    int ret = 0;

    switch (ssl->state) {
        case MBEDTLS_SSL_HELLO_REQUEST:
            mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_HELLO);
            break;

        case MBEDTLS_SSL_CLIENT_HELLO:
            ret = mbedtls_ssl_write_client_hello(ssl);
            break;

        case MBEDTLS_SSL_SERVER_HELLO:
            ret = ssl_tls13_process_server_hello(ssl);
            break;

        case MBEDTLS_SSL_ENCRYPTED_EXTENSIONS:
            ret = ssl_tls13_process_encrypted_extensions(ssl);
            break;

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
        case MBEDTLS_SSL_CERTIFICATE_REQUEST:
            ret = ssl_tls13_process_certificate_request(ssl);
            break;

        case MBEDTLS_SSL_SERVER_CERTIFICATE:
            ret = ssl_tls13_process_server_certificate(ssl);
            break;

        case MBEDTLS_SSL_CERTIFICATE_VERIFY:
            ret = ssl_tls13_process_certificate_verify(ssl);
            break;
#endif                                                             

        case MBEDTLS_SSL_SERVER_FINISHED:
            ret = ssl_tls13_process_server_finished(ssl);
            break;

#if defined(MBEDTLS_SSL_EARLY_DATA)
        case MBEDTLS_SSL_END_OF_EARLY_DATA:
            ret = ssl_tls13_write_end_of_early_data(ssl);
            break;
#endif

        case MBEDTLS_SSL_CLIENT_CERTIFICATE:
            ret = ssl_tls13_write_client_certificate(ssl);
            break;

#if defined(MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED)
        case MBEDTLS_SSL_CLIENT_CERTIFICATE_VERIFY:
            ret = ssl_tls13_write_client_certificate_verify(ssl);
            break;
#endif                                                             

        case MBEDTLS_SSL_CLIENT_FINISHED:
            ret = ssl_tls13_write_client_finished(ssl);
            break;

        case MBEDTLS_SSL_FLUSH_BUFFERS:
            ret = ssl_tls13_flush_buffers(ssl);
            break;

        case MBEDTLS_SSL_HANDSHAKE_WRAPUP:
            ret = ssl_tls13_handshake_wrapup(ssl);
            break;

              
                                                                   
               
#if defined(MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE)
        case MBEDTLS_SSL_CLIENT_CCS_BEFORE_2ND_CLIENT_HELLO:
            ret = mbedtls_ssl_tls13_write_change_cipher_spec(ssl);
            if (ret != 0) {
                break;
            }
            mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_HELLO);
            break;

        case MBEDTLS_SSL_CLIENT_CCS_AFTER_SERVER_FINISHED:
            ret = mbedtls_ssl_tls13_write_change_cipher_spec(ssl);
            if (ret != 0) {
                break;
            }
            mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_CLIENT_CERTIFICATE);
            break;

#if defined(MBEDTLS_SSL_EARLY_DATA)
        case MBEDTLS_SSL_CLIENT_CCS_AFTER_CLIENT_HELLO:
            ret = mbedtls_ssl_tls13_write_change_cipher_spec(ssl);
            if (ret == 0) {
                mbedtls_ssl_handshake_set_state(ssl, MBEDTLS_SSL_SERVER_HELLO);

                MBEDTLS_SSL_DEBUG_MSG(
                    1, ("Switch to early data keys for outbound traffic"));
                mbedtls_ssl_set_outbound_transform(
                    ssl, ssl->handshake->transform_earlydata);
                ssl->early_data_state = MBEDTLS_SSL_EARLY_DATA_STATE_CAN_WRITE;
            }
            break;
#endif                             
#endif                                            

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
        case MBEDTLS_SSL_TLS1_3_NEW_SESSION_TICKET:
            ret = ssl_tls13_process_new_session_ticket(ssl);
            break;
#endif                                  

        default:
            MBEDTLS_SSL_DEBUG_MSG(1, ("invalid state %d", ssl->state));
            return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
    }

    return ret;
}

#endif                                                    
