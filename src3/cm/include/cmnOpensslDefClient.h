/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _O_CMN_OPENSSL_DEF_CLIENT_H_
#define _O_CMN_OPENSSL_DEF_CLIENT_H_ 1

#if !defined(CM_DISABLE_SSL)
#include <openssl/ssl.h>

typedef struct cmnOpensslFuncs
{
    acp_sint32_t         (*SSL_library_init)(void);
    void                 (*OpenSSL_add_all_algorithms)(void);
    void                 (*OPENSSL_add_all_algorithms_noconf)(void);
    void                 (*CONF_modules_free)(void);

    SSL_CTX*             (*SSL_CTX_new)(SSL_METHOD *aMethod);
    void                 (*SSL_CTX_set_info_callback)(SSL_CTX *aCtx, 
                                                      void (*aCallback)(const SSL*, 
                                                      acp_sint32_t, 
                                                      acp_sint32_t));
    acp_sint32_t         (*SSL_CTX_use_certificate_file)(SSL_CTX *aCtx, 
                                                         const acp_char_t *aFile, 
                                                         acp_sint32_t aType);
    void                 (*SSL_CTX_set_default_passwd_cb)(SSL_CTX *aCtx, pem_password_cb *aCb);
    acp_sint32_t         (*SSL_CTX_use_PrivateKey_file)(SSL_CTX *aCtx, 
                                                        const acp_char_t *aFile, 
                                                        acp_sint32_t aType);
    acp_sint32_t         (*SSL_CTX_check_private_key)(const SSL_CTX *aCtx);

    const SSL_METHOD*    (*TLSv1_server_method)(void);
    const SSL_METHOD*    (*TLSv1_client_method)(void);

    acp_sint32_t         (*SSL_CTX_set_cipher_list)(SSL_CTX *aSsl, const acp_char_t *aStr);
    const acp_char_t*    (*SSL_CIPHER_get_name)(const SSL_CIPHER *aCipher);
    SSL_CIPHER*          (*SSL_get_current_cipher)(const SSL *aSsl);

    void                 (*SSL_CTX_set_verify_depth)(SSL *aSsl, acp_sint32_t aDepth);
    acp_sint32_t         (*SSL_CTX_load_verify_locations)(SSL_CTX *aCtx, 
                                                          const acp_char_t *aCAfile, 
                                                          const acp_char_t *aCApath);
    acp_sint32_t         (*SSL_CTX_set_default_verify_paths)(SSL_CTX *aCtx);
    acp_slong_t          (*SSL_CTX_ctrl)(SSL_CTX *aCtx, acp_sint32_t aCmd, acp_slong_t aMode, void *aParg);
    void                 (*SSL_CTX_set_client_CA_list)(SSL_CTX *aCtx, STACK_OF(X509_NAME) *aList);
    void                 (*SSL_CTX_set_verify)(SSL_CTX *aCtx, 
                                               acp_sint32_t aMode, 
                                               acp_sint32_t (*aVerifyCallback)(acp_sint32_t, X509_STORE_CTX *));
    void                 (*SSL_CTX_free)(SSL_CTX *aCtx);

    X509*                (*SSL_get_peer_certificate)(const SSL *aSsl);
    acp_slong_t          (*SSL_get_verify_result)(const SSL *aSsl);
    STACK_OF(X509_NAME)* (*SSL_load_client_CA_file)(const acp_char_t *aFile);
    acp_sint32_t         (*SSL_set_cipher_list)(SSL_CTX *aSsl, const acp_char_t *aStr);

    void                 (*SSL_set_quiet_shutdown)(SSL *aSsl, acp_sint32_t aMode);
    acp_sint32_t         (*SSL_shutdown)(SSL *aSsl);
    void                 (*SSL_free)(SSL  *aSsl);

    acp_sint32_t         (*SSL_get_error)(const SSL *aSsl, acp_sint32_t aRet);
    void                 (*SSL_load_error_strings)(void);
    const acp_char_t*    (*ERR_error_string)(acp_ulong_t aError, acp_char_t *aBuf);

    void                 (*ERR_free_strings)(void);
    void                 (*ERR_remove_state)(acp_ulong_t aPid);
    void                 (*EVP_cleanup)(void);
    void                 (*ENGINE_cleanup)(void);
    void                 (*CONF_modules_unload)(acp_sint32_t aAll);
    void                 (*CRYPTO_cleanup_all_ex_data)(void);
    acp_ulong_t          (*ERR_get_error)(void);

    acp_sint32_t         (*FIPS_mode_set)(acp_sint32_t aOnOff);
    void                 (*CRYPTO_set_locking_callback)(void (*aLockingFunction)
                                                             (acp_sint32_t      aMode, 
                                                              acp_sint32_t      aN, 
                                                              const acp_char_t *aFile, 
                                                              acp_sint32_t      aLine));
    void                 (*CRYPTO_set_id_callback)(acp_ulong_t (*aIdFunction)(void));
    acp_sint32_t         (*CRYPTO_num_locks)(void); /* BUG-45235 */

    acp_sint32_t         (*SSL_read)(SSL *aSsl, void *aBuf, acp_sint32_t aNum);
    acp_sint32_t         (*SSL_write)(SSL *aSsl, const void *aBuf, acp_sint32_t aNum);
    
    SSL*                 (*SSL_new)(SSL_CTX *aCtx);
    acp_sint32_t         (*SSL_connect)(SSL *aSsl);
    acp_sint32_t         (*SSL_set_fd)(SSL *aSsl, acp_sint32_t fd);
    acp_sint32_t         (*SSL_accept)(SSL *aSsl);

    const acp_char_t*    (*SSL_state_string_long)(const SSL *aSsl);
    const acp_char_t*    (*SSL_alert_type_string_long)(acp_sint32_t aValue);
    const acp_char_t*    (*SSL_alert_desc_string_long)(acp_sint32_t aValue);

    X509_NAME*           (*X509_get_subject_name)(X509 *aA);
    acp_char_t*          (*X509_NAME_oneline)(X509_NAME *aA, acp_char_t *aBuf, acp_sint32_t aSize);
    acp_sint32_t         (*X509_NAME_get_text_by_NID)(X509_NAME *aName, 
                                                      acp_sint32_t aNid, 
                                                      acp_char_t *aBuf, 
                                                      acp_sint32_t aLen);
    X509_NAME*           (*X509_get_issuer_name)(X509 *aA);
    void                 (*X509_free)(const X509 *aCert);

    void                 (*SSL_COMP_free_compression_methods)(void);
} cmnOpensslFuncs;

#define ALTIBASE_OPENSSL_LIB_NAME "ssl"
#define ALTIBASE_CRYPTO_LIB_NAME  "crypto"

#endif /* CM_DISABLE_SSL */

#endif /* _O_CMN_OPENSSL_DEF_CLIENT_H_ */
