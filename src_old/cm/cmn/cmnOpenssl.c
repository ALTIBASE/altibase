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

#include <cmAllClient.h>
#include <cmnOpensslClient.h>

#if !defined(CM_DISABLE_SSL)

/* In case of client connections, it is allowable
 * if the client couldn't load the openssl library at cmiInitialize().
 * Therefore, the following two variables store a real error code and message
 * to let the client know the accurate reason for the failure of the library initialization
 * and are returned to the client at the appropriate time. */
#define SSL_LIB_ERROR_MSG_LEN (ACI_MAX_ERROR_MSG_LEN+256)  /* aciErrorMsg.h */
acp_uint32_t     gSslLibErrorCode = cmERR_IGNORE_NoError;
acp_char_t       gSslLibErrorMsg[SSL_LIB_ERROR_MSG_LEN] = {0,};

cmnOpenssl *gOpenssl = NULL;

/* BUG-45235 */
static acp_uint64_t cmnOpensslCryptoThreadId();
static void cmnOpensslCryptoLocking(int aMode, int aType, const char *aFile, int aLine);

ACI_RC cmnOpensslInitialize(cmnOpenssl **aOpenssl)
{
    acp_rc_t      sRC = ACP_RC_SUCCESS;
    cmnOpenssl   *sOpenssl = NULL;
    acp_sint32_t  i;

    ACI_TEST(acpMemCalloc((void **)&sOpenssl, 1, sizeof(cmnOpenssl)) != ACP_RC_SUCCESS);

    sRC = acpDlOpen(&sOpenssl->mSslHandle, NULL, ALTIBASE_OPENSSL_LIB_NAME, ACP_TRUE);
    ACI_TEST_RAISE(sRC != ACP_RC_SUCCESS, ERR_DLOPEN_LIBSSL);

    sRC = acpDlOpen(&sOpenssl->mCryptoHandle, NULL, ALTIBASE_CRYPTO_LIB_NAME, ACP_TRUE);
    ACI_TEST_RAISE(sRC != ACP_RC_SUCCESS, ERR_DLOPEN_LIBCRYPTO);

    /* load functions */
    *(void**)&sOpenssl->mFuncs.SSL_library_init = acpDlSym(&sOpenssl->mSslHandle, "SSL_library_init");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_library_init == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.OPENSSL_add_all_algorithms_noconf = acpDlSym(&sOpenssl->mCryptoHandle, 
                                                                     "OPENSSL_add_all_algorithms_noconf");
    ACI_TEST_RAISE(sOpenssl->mFuncs.OPENSSL_add_all_algorithms_noconf == NULL, ERR_DLSYM_LIBCRYPTO);

    sOpenssl->mFuncs.OpenSSL_add_all_algorithms = sOpenssl->mFuncs.OPENSSL_add_all_algorithms_noconf;

    *(void**)&sOpenssl->mFuncs.SSL_CTX_new = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_new");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_new == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.TLSv1_server_method = acpDlSym(&sOpenssl->mSslHandle, "TLSv1_server_method");
    ACI_TEST_RAISE(sOpenssl->mFuncs.TLSv1_server_method == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.TLSv1_client_method = acpDlSym(&sOpenssl->mSslHandle, "TLSv1_client_method");
    ACI_TEST_RAISE(sOpenssl->mFuncs.TLSv1_client_method == NULL, ERR_DLSYM_LIBSSL);

#if (DEBUG)  /* BUG-47037 OPENSSL_VERSION_NUMBER >= 0x00909000L */
    /* This function can be null depending on the library version
     * and is used only for debugging. */
    *(void**)&sOpenssl->mFuncs.SSL_CTX_set_info_callback = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_set_info_callback");
#endif

    *(void**)&sOpenssl->mFuncs.SSL_CTX_use_certificate_file = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_use_certificate_file");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_use_certificate_file == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_set_default_passwd_cb = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_set_default_passwd_cb");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_set_default_passwd_cb == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_use_PrivateKey_file = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_use_PrivateKey_file");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_use_PrivateKey_file == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_check_private_key = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_check_private_key");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_check_private_key == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_set_cipher_list = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_set_cipher_list");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_set_cipher_list == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_get_current_cipher = acpDlSym(&sOpenssl->mSslHandle, "SSL_get_current_cipher");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_get_current_cipher == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CIPHER_get_name = acpDlSym(&sOpenssl->mSslHandle, "SSL_CIPHER_get_name");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CIPHER_get_name == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_set_verify_depth = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_set_verify_depth");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_set_verify_depth == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_load_verify_locations = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_load_verify_locations");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_load_verify_locations == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_set_default_verify_paths = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_set_default_verify_paths");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_set_default_verify_paths == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_ctrl = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_ctrl");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_ctrl == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_set_client_CA_list = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_set_client_CA_list");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_set_client_CA_list == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_set_verify = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_set_verify");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_set_verify == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_CTX_free = acpDlSym(&sOpenssl->mSslHandle, "SSL_CTX_free");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_CTX_free == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_set_quiet_shutdown = acpDlSym(&sOpenssl->mSslHandle, "SSL_set_quiet_shutdown");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_set_quiet_shutdown == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_get_peer_certificate = acpDlSym(&sOpenssl->mSslHandle, "SSL_get_peer_certificate");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_get_peer_certificate == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_get_verify_result = acpDlSym(&sOpenssl->mSslHandle, "SSL_get_verify_result");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_get_verify_result == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_load_client_CA_file = acpDlSym(&sOpenssl->mSslHandle, "SSL_load_client_CA_file");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_load_client_CA_file == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_set_cipher_list = acpDlSym(&sOpenssl->mSslHandle, "SSL_set_cipher_list");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_set_cipher_list == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_shutdown = acpDlSym(&sOpenssl->mSslHandle, "SSL_shutdown");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_shutdown == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_free = acpDlSym(&sOpenssl->mSslHandle, "SSL_free");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_free == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_get_error = acpDlSym(&sOpenssl->mSslHandle, "SSL_get_error");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_get_error == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_load_error_strings = acpDlSym(&sOpenssl->mSslHandle, "SSL_load_error_strings");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_load_error_strings == NULL, ERR_DLSYM_LIBSSL);

    /* BUG-44488 */
    /* OpenSSL does not provide any cleanup functions for the resources allocated by itself.
     * Therefore, the following functions should be called to avoid memory leaks.
     * See also https://wiki.openssl.org/index.php/Library_Initialization */
    *(void**)&sOpenssl->mFuncs.ERR_free_strings = acpDlSym(&sOpenssl->mSslHandle, "ERR_free_strings");
    ACI_TEST_RAISE(sOpenssl->mFuncs.ERR_free_strings == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.ERR_remove_state = acpDlSym(&sOpenssl->mSslHandle, "ERR_remove_state");
    ACI_TEST_RAISE(sOpenssl->mFuncs.ERR_remove_state == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.EVP_cleanup = acpDlSym(&sOpenssl->mSslHandle, "EVP_cleanup");
    ACI_TEST_RAISE(sOpenssl->mFuncs.EVP_cleanup == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.ENGINE_cleanup = acpDlSym(&sOpenssl->mCryptoHandle, "ENGINE_cleanup");
    ACI_TEST_RAISE(sOpenssl->mFuncs.ENGINE_cleanup == NULL, ERR_DLSYM_LIBCRYPTO);

    *(void**)&sOpenssl->mFuncs.CONF_modules_unload = acpDlSym(&sOpenssl->mCryptoHandle, "CONF_modules_unload");
    ACI_TEST_RAISE(sOpenssl->mFuncs.CONF_modules_unload == NULL, ERR_DLSYM_LIBCRYPTO);

    *(void**)&sOpenssl->mFuncs.CRYPTO_cleanup_all_ex_data = acpDlSym(&sOpenssl->mCryptoHandle, "CRYPTO_cleanup_all_ex_data");
    ACI_TEST_RAISE(sOpenssl->mFuncs.CRYPTO_cleanup_all_ex_data == NULL, ERR_DLSYM_LIBCRYPTO);

    *(void**)&sOpenssl->mFuncs.FIPS_mode_set = acpDlSym(&sOpenssl->mCryptoHandle, "FIPS_mode_set");

    *(void**)&sOpenssl->mFuncs.CRYPTO_set_locking_callback = acpDlSym(&sOpenssl->mCryptoHandle, "CRYPTO_set_locking_callback");
    ACI_TEST_RAISE(sOpenssl->mFuncs.CRYPTO_set_locking_callback == NULL, ERR_DLSYM_LIBCRYPTO);

    *(void**)&sOpenssl->mFuncs.CRYPTO_set_id_callback = acpDlSym(&sOpenssl->mCryptoHandle, "CRYPTO_set_id_callback");
    ACI_TEST_RAISE(sOpenssl->mFuncs.CRYPTO_set_id_callback == NULL, ERR_DLSYM_LIBCRYPTO);

    /* BUG-45235 since OpenSSL 0.9.4 */
    *(void**)&sOpenssl->mFuncs.CRYPTO_num_locks = acpDlSym(&sOpenssl->mCryptoHandle, "CRYPTO_num_locks");
    ACI_TEST_RAISE(sOpenssl->mFuncs.CRYPTO_num_locks == NULL, ERR_DLSYM_LIBCRYPTO);

    /* BUG-47037 OPENSSL_VERSION_NUMBER >= 0x1000200fL */
    *(void**)&sOpenssl->mFuncs.SSL_COMP_free_compression_methods = acpDlSym(&sOpenssl->mSslHandle, "SSL_COMP_free_compression_methods");

    /* BUG-41166 SSL multi platform */
    *(void**)&sOpenssl->mFuncs.ERR_error_string = acpDlSym(&sOpenssl->mSslHandle, "ERR_error_string");
    ACI_TEST_RAISE(sOpenssl->mFuncs.ERR_error_string == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.ERR_get_error = acpDlSym(&sOpenssl->mSslHandle, "ERR_get_error");
    ACI_TEST_RAISE(sOpenssl->mFuncs.ERR_get_error == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_read = acpDlSym(&sOpenssl->mSslHandle, "SSL_read");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_read == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_write = acpDlSym(&sOpenssl->mSslHandle, "SSL_write");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_write == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_new = acpDlSym(&sOpenssl->mSslHandle, "SSL_new");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_new == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_connect = acpDlSym(&sOpenssl->mSslHandle, "SSL_connect");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_connect == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_set_fd = acpDlSym(&sOpenssl->mSslHandle, "SSL_set_fd");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_set_fd == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_accept = acpDlSym(&sOpenssl->mSslHandle, "SSL_accept");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_accept == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_state_string_long = acpDlSym(&sOpenssl->mSslHandle, "SSL_state_string_long");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_state_string_long == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_alert_type_string_long = acpDlSym(&sOpenssl->mSslHandle, "SSL_alert_type_string_long");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_alert_type_string_long == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.SSL_alert_desc_string_long = acpDlSym(&sOpenssl->mSslHandle, "SSL_alert_desc_string_long");
    ACI_TEST_RAISE(sOpenssl->mFuncs.SSL_alert_desc_string_long == NULL, ERR_DLSYM_LIBSSL);
    
    *(void**)&sOpenssl->mFuncs.X509_NAME_oneline = acpDlSym(&sOpenssl->mSslHandle, "X509_NAME_oneline");
    ACI_TEST_RAISE(sOpenssl->mFuncs.X509_NAME_oneline == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.X509_get_subject_name = acpDlSym(&sOpenssl->mSslHandle, "X509_get_subject_name");
    ACI_TEST_RAISE(sOpenssl->mFuncs.X509_get_subject_name == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.X509_NAME_oneline = acpDlSym(&sOpenssl->mSslHandle, "X509_NAME_oneline");
    ACI_TEST_RAISE(sOpenssl->mFuncs.X509_NAME_oneline == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.X509_get_issuer_name = acpDlSym(&sOpenssl->mSslHandle, "X509_get_issuer_name");
    ACI_TEST_RAISE(sOpenssl->mFuncs.X509_get_issuer_name == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.X509_NAME_get_text_by_NID = acpDlSym(&sOpenssl->mSslHandle, "X509_NAME_get_text_by_NID");
    ACI_TEST_RAISE(sOpenssl->mFuncs.X509_NAME_get_text_by_NID == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&sOpenssl->mFuncs.X509_free = acpDlSym(&sOpenssl->mSslHandle, "X509_free");
    ACI_TEST_RAISE(sOpenssl->mFuncs.X509_free == NULL, ERR_DLSYM_LIBSSL);

    /* Initialize SSL/TLS */
    sOpenssl->mFuncs.SSL_library_init(); /* register all ciphers and has algorithms used in SSL APIs. */
    sOpenssl->mFuncs.OpenSSL_add_all_algorithms(); /* load and register all cryptos, etc */
    sOpenssl->mFuncs.SSL_load_error_strings(); /* load error strings for SSL APIs as well as for Crypto APIs. */

    sOpenssl->mLibInitialized = ACP_TRUE;

    /* BUG-45235 */
    sOpenssl->mMutexCount = sOpenssl->mFuncs.CRYPTO_num_locks();

    ACI_TEST(acpMemCalloc((void **)&sOpenssl->mMutex, sOpenssl->mMutexCount, sizeof(acp_thr_mutex_t)) != ACP_RC_SUCCESS);

    for (i = 0; i < sOpenssl->mMutexCount; ++i)
    {
        ACI_TEST(acpThrMutexCreate(&sOpenssl->mMutex[i], ACP_THR_MUTEX_DEFAULT) != ACI_SUCCESS);
    }

    sOpenssl->mFuncs.CRYPTO_set_id_callback(cmnOpensslCryptoThreadId);
    sOpenssl->mFuncs.CRYPTO_set_locking_callback(cmnOpensslCryptoLocking);

    *aOpenssl = sOpenssl;

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_DLSYM_LIBSSL )
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_DLSYM, ALTIBASE_OPENSSL_LIB_NAME, acpDlError(&sOpenssl->mSslHandle)));
    }
    ACI_EXCEPTION( ERR_DLSYM_LIBCRYPTO )
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_DLSYM, ALTIBASE_CRYPTO_LIB_NAME, acpDlError(&sOpenssl->mCryptoHandle)));
    }
    ACI_EXCEPTION( ERR_DLOPEN_LIBSSL)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_DLOPEN, ALTIBASE_OPENSSL_LIB_NAME, acpDlError(&sOpenssl->mSslHandle)));
    }
    ACI_EXCEPTION( ERR_DLOPEN_LIBCRYPTO )
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_DLOPEN, ALTIBASE_CRYPTO_LIB_NAME, acpDlError(&sOpenssl->mCryptoHandle)));
    }
    ACI_EXCEPTION_END;

    gSslLibErrorCode = aciGetErrorCode();
    acpSnprintf(gSslLibErrorMsg, SSL_LIB_ERROR_MSG_LEN, "%s", aciGetErrorMsg(gSslLibErrorCode));

    if (sOpenssl != NULL) /* BUG-45235 */
    {
        (void)cmnOpensslDestroy(&sOpenssl);
    }
    else
    {
        /* failed to allocate */
    }

    return ACI_FAILURE;
}

static ACI_RC cmnOpensslCleanUpLibrary(cmnOpenssl *aOpenssl)
{
    ACI_TEST(aOpenssl == NULL);

    if (aOpenssl->mFuncs.SSL_COMP_free_compression_methods != NULL)  /* BUG-47037 */
    {
        /* Release the internal table of compression methods built internally */
        aOpenssl->mFuncs.SSL_COMP_free_compression_methods();
    }

    /* Exit the FIPS mode of operation */
    if (aOpenssl->mFuncs.FIPS_mode_set != NULL) /* BUG-44362 */
    {
        (void)aOpenssl->mFuncs.FIPS_mode_set(0);
    }
    else
    {
        /* ignore, if the OpenSSL doesn't have FIPS_mode_set() function for FIPS module. */
    }

    /* Crypto locking doesn't need anymore. */
    aOpenssl->mFuncs.CRYPTO_set_locking_callback(NULL);
    aOpenssl->mFuncs.CRYPTO_set_id_callback(NULL);

    /* According to the OpenSSL website, 
     * CRYPTO_set_id_callback() should be called on each thread;
     * however, this causes an application crash by a double-free error 
     * if every thread calls the function. */
    aOpenssl->mFuncs.CRYPTO_cleanup_all_ex_data();

    /* Free error strings */
    aOpenssl->mFuncs.ERR_free_strings();

    /* Remove all ciphers and digests from the internal table for them */
    aOpenssl->mFuncs.EVP_cleanup();

    /* Unload ENGINE objects */
    aOpenssl->mFuncs.ENGINE_cleanup();

    /* Finish and unload configuration modules (1: All modules) */
    aOpenssl->mFuncs.CONF_modules_unload(1);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnOpensslDestroy(cmnOpenssl **aOpenssl)
{
    cmnOpenssl   *sOpenssl = *aOpenssl;
    acp_sint32_t  i;

    if (sOpenssl != NULL)
    {
        if (sOpenssl->mLibInitialized == ACP_TRUE)
        {
            (void)cmnOpensslCleanUpLibrary(sOpenssl);
            sOpenssl->mLibInitialized = ACP_FALSE;
        }
        else
        {
            /* not initialized */
        }

        if (sOpenssl->mSslHandle.mHandle != NULL)
        {
            (void)acpDlClose(&sOpenssl->mSslHandle);
            sOpenssl->mSslHandle.mHandle = NULL;
        }
        else
        {
            /* already closed */
        }

        if (sOpenssl->mCryptoHandle.mHandle != NULL)
        {
            (void)acpDlClose(&sOpenssl->mCryptoHandle);
            sOpenssl->mCryptoHandle.mHandle = NULL;
        }
        else
        {
            /* already closed */
        }

        if (sOpenssl->mMutex != NULL)
        {
            for (i = 0; i < sOpenssl->mMutexCount; ++i)
            {
                (void)acpThrMutexDestroy(&sOpenssl->mMutex[i]);
            }

            acpMemFree(sOpenssl->mMutex);
            sOpenssl->mMutex = NULL;
            sOpenssl->mMutexCount = 0;
        }
        else
        {
            /* already destroyed */
        }

        acpMemFree(*aOpenssl);
        *aOpenssl = NULL;
    }
    else
    {
        /* already freed */
    }

    return ACI_SUCCESS;
}

const acp_char_t *cmnOpensslErrorMessage(cmnOpenssl *aOpenssl)
{
    ACI_TEST_RAISE(gOpenssl == NULL, NoSslLibrary);

    return aOpenssl->mFuncs.ERR_error_string(aOpenssl->mFuncs.ERR_get_error(), NULL);

    ACI_EXCEPTION(NoSslLibrary)
    {
        aciSetErrorCodeAndMsg(gSslLibErrorCode, gSslLibErrorMsg);
    }
    ACI_EXCEPTION_END;

    return (acp_char_t *)NULL;
}

/* BUG-45235 */
static acp_uint64_t cmnOpensslCryptoThreadId()
{
    return acpThrGetSelfID();
}

static void cmnOpensslCryptoLocking(int aMode, int aType, const char *aFile, int aLine)
{
    ACP_UNUSED(aFile);
    ACP_UNUSED(aLine);

    if (aMode & CRYPTO_LOCK)
    {
        (void)acpThrMutexLock(&gOpenssl->mMutex[aType]);
    }
    else
    {
        (void)acpThrMutexUnlock(&gOpenssl->mMutex[aType]);
    }
}

#endif /* CM_DISABLE_SSL */
