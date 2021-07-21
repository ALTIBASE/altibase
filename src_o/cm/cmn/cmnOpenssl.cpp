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

#include <cm.h>
#include <cmnOpenssl.h>

#if !defined(CM_DISABLE_SSL)

PDL_SHLIB_HANDLE cmnOpenssl::mSslHandle;
PDL_SHLIB_HANDLE cmnOpenssl::mCryptoHandle;
cmnOpensslFuncs  cmnOpenssl::mFuncs;
idBool           cmnOpenssl::mLibInitialized = ID_FALSE;

/* BUG-45235 */
PDL_thread_mutex_t *cmnOpenssl::mMutex = NULL;
SInt                cmnOpenssl::mMutexCount = 0;

IDE_RC cmnOpenssl::initialize()
{
    SInt  i;
    SChar sOpensslSoName[1024];
    SChar sCryptoSoName[1024];

    idlOS::sprintf(sOpensslSoName, "%s%s%s", PDL_DLL_PREFIX, ALTIBASE_OPENSSL_LIB_NAME, PDL_DLL_SUFFIX);
    idlOS::sprintf(sCryptoSoName, "%s%s%s", PDL_DLL_PREFIX, ALTIBASE_CRYPTO_LIB_NAME, PDL_DLL_SUFFIX);

    /* RTDL_LAZY: Only resolve symbols as the code that references them is executed. 
     * RTLD_LOCAL: Symbols of the openssl library are not made available 
     *             to resolve references in subsequently loaded libraries. */
    mSslHandle = idlOS::dlopen(sOpensslSoName, RTLD_LAZY | RTLD_LOCAL);
    IDE_TEST_RAISE(mSslHandle == PDL_SHLIB_INVALID_HANDLE, ERR_DLOPEN_LIBSSL);

    mCryptoHandle = idlOS::dlopen(sCryptoSoName, RTLD_LAZY | RTLD_LOCAL);
    IDE_TEST_RAISE(mCryptoHandle == PDL_SHLIB_INVALID_HANDLE, ERR_DLOPEN_LIBCRYPTO);

    /* load functions */
    *(void**)&mFuncs.SSL_library_init = idlOS::dlsym(mSslHandle, "SSL_library_init");
    IDE_TEST_RAISE(mFuncs.SSL_library_init == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.OPENSSL_add_all_algorithms_noconf = idlOS::dlsym(mCryptoHandle, 
                                                                     "OPENSSL_add_all_algorithms_noconf");
    IDE_TEST_RAISE(mFuncs.OPENSSL_add_all_algorithms_noconf == NULL, ERR_DLSYM_LIBCRYPTO);

    mFuncs.OpenSSL_add_all_algorithms = mFuncs.OPENSSL_add_all_algorithms_noconf;

    *(void**)&mFuncs.SSL_CTX_new = idlOS::dlsym(mSslHandle, "SSL_CTX_new");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_new == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.TLSv1_server_method = idlOS::dlsym(mSslHandle, "TLSv1_server_method");
    IDE_TEST_RAISE(mFuncs.TLSv1_server_method == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.TLSv1_client_method = idlOS::dlsym(mSslHandle, "TLSv1_client_method");
    IDE_TEST_RAISE(mFuncs.TLSv1_client_method == NULL, ERR_DLSYM_LIBSSL);

#if (DEBUG)  /* BUG-47037 OPENSSL_VERSION_NUMBER >= 0x00909000L */
    /* This function can be null depending on the library version
     * and is used only for debugging. */
    *(void**)&mFuncs.SSL_CTX_set_info_callback = idlOS::dlsym(mSslHandle, "SSL_CTX_set_info_callback");
#endif

    *(void**)&mFuncs.SSL_CTX_use_certificate_file = idlOS::dlsym(mSslHandle, "SSL_CTX_use_certificate_file");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_use_certificate_file == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_set_default_passwd_cb = idlOS::dlsym(mSslHandle, "SSL_CTX_set_default_passwd_cb");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_set_default_passwd_cb == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_use_PrivateKey_file = idlOS::dlsym(mSslHandle, "SSL_CTX_use_PrivateKey_file");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_use_PrivateKey_file == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_check_private_key = idlOS::dlsym(mSslHandle, "SSL_CTX_check_private_key");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_check_private_key == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_set_cipher_list = idlOS::dlsym(mSslHandle, "SSL_CTX_set_cipher_list");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_set_cipher_list == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_get_current_cipher = idlOS::dlsym(mSslHandle, "SSL_get_current_cipher");
    IDE_TEST_RAISE(mFuncs.SSL_get_current_cipher == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CIPHER_get_name = idlOS::dlsym(mSslHandle, "SSL_CIPHER_get_name");
    IDE_TEST_RAISE(mFuncs.SSL_CIPHER_get_name == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_set_verify_depth = idlOS::dlsym(mSslHandle, "SSL_CTX_set_verify_depth");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_set_verify_depth == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_load_verify_locations = idlOS::dlsym(mSslHandle, "SSL_CTX_load_verify_locations");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_load_verify_locations == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_set_default_verify_paths = idlOS::dlsym(mSslHandle, "SSL_CTX_set_default_verify_paths");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_set_default_verify_paths == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_ctrl = idlOS::dlsym(mSslHandle, "SSL_CTX_ctrl");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_ctrl == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_set_client_CA_list = idlOS::dlsym(mSslHandle, "SSL_CTX_set_client_CA_list");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_set_client_CA_list == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_set_verify = idlOS::dlsym(mSslHandle, "SSL_CTX_set_verify");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_set_verify == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_CTX_free = idlOS::dlsym(mSslHandle, "SSL_CTX_free");
    IDE_TEST_RAISE(mFuncs.SSL_CTX_free == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_set_quiet_shutdown = idlOS::dlsym(mSslHandle, "SSL_set_quiet_shutdown");
    IDE_TEST_RAISE(mFuncs.SSL_set_quiet_shutdown == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_get_peer_certificate = idlOS::dlsym(mSslHandle, "SSL_get_peer_certificate");
    IDE_TEST_RAISE(mFuncs.SSL_get_peer_certificate == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_get_verify_result = idlOS::dlsym(mSslHandle, "SSL_get_verify_result");
    IDE_TEST_RAISE(mFuncs.SSL_get_verify_result == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_load_client_CA_file = idlOS::dlsym(mSslHandle, "SSL_load_client_CA_file");
    IDE_TEST_RAISE(mFuncs.SSL_load_client_CA_file == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_set_cipher_list = idlOS::dlsym(mSslHandle, "SSL_set_cipher_list");
    IDE_TEST_RAISE(mFuncs.SSL_set_cipher_list == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_shutdown = idlOS::dlsym(mSslHandle, "SSL_shutdown");
    IDE_TEST_RAISE(mFuncs.SSL_shutdown == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_free = idlOS::dlsym(mSslHandle, "SSL_free");
    IDE_TEST_RAISE(mFuncs.SSL_free == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_get_error = idlOS::dlsym(mSslHandle, "SSL_get_error");
    IDE_TEST_RAISE(mFuncs.SSL_get_error == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_load_error_strings = idlOS::dlsym(mSslHandle, "SSL_load_error_strings");
    IDE_TEST_RAISE(mFuncs.SSL_load_error_strings == NULL, ERR_DLSYM_LIBSSL);

    /* BUG-44488 */
    /* OpenSSL does not provide any cleanup functions for the resources allocated by itself.
     * Therefore, the following functions should be called to avoid memory leaks.
     * See also https://wiki.openssl.org/index.php/Library_Initialization */
    *(void**)&mFuncs.ERR_free_strings = idlOS::dlsym(mSslHandle, "ERR_free_strings");
    IDE_TEST_RAISE(mFuncs.ERR_free_strings == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.ERR_remove_state = idlOS::dlsym(mSslHandle, "ERR_remove_state");
    IDE_TEST_RAISE(mFuncs.ERR_remove_state == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.EVP_cleanup = idlOS::dlsym(mSslHandle, "EVP_cleanup");
    IDE_TEST_RAISE(mFuncs.EVP_cleanup == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.ENGINE_cleanup = idlOS::dlsym(mCryptoHandle, "ENGINE_cleanup");
    IDE_TEST_RAISE(mFuncs.ENGINE_cleanup == NULL, ERR_DLSYM_LIBCRYPTO);

    *(void**)&mFuncs.CONF_modules_unload = idlOS::dlsym(mCryptoHandle, "CONF_modules_unload");
    IDE_TEST_RAISE(mFuncs.CONF_modules_unload == NULL, ERR_DLSYM_LIBCRYPTO);

    *(void**)&mFuncs.CRYPTO_cleanup_all_ex_data = idlOS::dlsym(mCryptoHandle, "CRYPTO_cleanup_all_ex_data");
    IDE_TEST_RAISE(mFuncs.CRYPTO_cleanup_all_ex_data == NULL, ERR_DLSYM_LIBCRYPTO);

    *(void**)&mFuncs.FIPS_mode_set = idlOS::dlsym(mCryptoHandle, "FIPS_mode_set");

    *(void**)&mFuncs.CRYPTO_set_locking_callback = idlOS::dlsym(mCryptoHandle, "CRYPTO_set_locking_callback");
    IDE_TEST_RAISE(mFuncs.CRYPTO_set_locking_callback == NULL, ERR_DLSYM_LIBCRYPTO);

    *(void**)&mFuncs.CRYPTO_set_id_callback = idlOS::dlsym(mCryptoHandle, "CRYPTO_set_id_callback");
    IDE_TEST_RAISE(mFuncs.CRYPTO_set_id_callback == NULL, ERR_DLSYM_LIBCRYPTO);

    /* BUG-45235 since OpenSSL 0.9.4 */
    *(void**)&mFuncs.CRYPTO_num_locks = idlOS::dlsym(mCryptoHandle, "CRYPTO_num_locks");
    IDE_TEST_RAISE(mFuncs.CRYPTO_num_locks == NULL, ERR_DLSYM_LIBCRYPTO);

    /* BUG-47037 OPENSSL_VERSION_NUMBER >= 0x1000200fL */
    *(void**)&mFuncs.SSL_COMP_free_compression_methods = idlOS::dlsym(mSslHandle, "SSL_COMP_free_compression_methods");

    /* BUG-41166 SSL multi platform */
    *(void**)&mFuncs.ERR_error_string = idlOS::dlsym(mSslHandle, "ERR_error_string");
    IDE_TEST_RAISE(mFuncs.ERR_error_string == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.ERR_get_error = idlOS::dlsym(mSslHandle, "ERR_get_error");
    IDE_TEST_RAISE(mFuncs.ERR_get_error == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_read = idlOS::dlsym(mSslHandle, "SSL_read");
    IDE_TEST_RAISE(mFuncs.SSL_read == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_write = idlOS::dlsym(mSslHandle, "SSL_write");
    IDE_TEST_RAISE(mFuncs.SSL_write == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_new = idlOS::dlsym(mSslHandle, "SSL_new");
    IDE_TEST_RAISE(mFuncs.SSL_new == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_connect = idlOS::dlsym(mSslHandle, "SSL_connect");
    IDE_TEST_RAISE(mFuncs.SSL_connect == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_set_fd = idlOS::dlsym(mSslHandle, "SSL_set_fd");
    IDE_TEST_RAISE(mFuncs.SSL_set_fd == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_accept = idlOS::dlsym(mSslHandle, "SSL_accept");
    IDE_TEST_RAISE(mFuncs.SSL_accept == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_state_string_long = idlOS::dlsym(mSslHandle, "SSL_state_string_long");
    IDE_TEST_RAISE(mFuncs.SSL_state_string_long == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_alert_type_string_long = idlOS::dlsym(mSslHandle, "SSL_alert_type_string_long");
    IDE_TEST_RAISE(mFuncs.SSL_alert_type_string_long == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.SSL_alert_desc_string_long = idlOS::dlsym(mSslHandle, "SSL_alert_desc_string_long");
    IDE_TEST_RAISE(mFuncs.SSL_alert_desc_string_long == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.X509_NAME_oneline = idlOS::dlsym(mSslHandle, "X509_NAME_oneline");
    IDE_TEST_RAISE(mFuncs.X509_NAME_oneline == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.X509_get_subject_name = idlOS::dlsym(mSslHandle, "X509_get_subject_name");
    IDE_TEST_RAISE(mFuncs.X509_get_subject_name == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.X509_NAME_oneline = idlOS::dlsym(mSslHandle, "X509_NAME_oneline");
    IDE_TEST_RAISE(mFuncs.X509_NAME_oneline == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.X509_get_issuer_name = idlOS::dlsym(mSslHandle, "X509_get_issuer_name");
    IDE_TEST_RAISE(mFuncs.X509_get_issuer_name == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.X509_NAME_get_text_by_NID = idlOS::dlsym(mSslHandle, "X509_NAME_get_text_by_NID");
    IDE_TEST_RAISE(mFuncs.X509_NAME_get_text_by_NID == NULL, ERR_DLSYM_LIBSSL);

    *(void**)&mFuncs.X509_free = idlOS::dlsym(mSslHandle, "X509_free");
    IDE_TEST_RAISE(mFuncs.X509_free == NULL, ERR_DLSYM_LIBSSL);

    /* BUG-46352 Output OpenSSL version */
    mFuncs.SSL_version_str = (const SChar **)idlOS::dlsym(mSslHandle, "SSL_version_str");
    *(void**)&mFuncs.SSLeay_version = idlOS::dlsym(mCryptoHandle, "SSLeay_version");

    /* Initialize SSL/TLS */
    /* register all ciphers and has algorithms used in SSL APIs. */
    mFuncs.SSL_library_init();
    /* load and register all cryptos, etc */
    mFuncs.OpenSSL_add_all_algorithms();
    /* load error strings for SSL APIs as well as for Crypto APIs. */
    mFuncs.SSL_load_error_strings();

    /* BUG-46352 Output OpenSSL version */
    if (mFuncs.SSL_version_str != NULL)
    {
        if (*mFuncs.SSL_version_str != NULL)
        {
            ideLog::log(IDE_SERVER_0, "SSL_version_str  : %s", *mFuncs.SSL_version_str);
        }
    }
    if (mFuncs.SSLeay_version != NULL)
    {
        ideLog::log(IDE_SERVER_0, "SSLeay_version() : %s", mFuncs.SSLeay_version(SSLEAY_VERSION));
    }

    mLibInitialized = ID_TRUE;

    /* BUG-45235 */
    mMutexCount = mFuncs.CRYPTO_num_locks();

    IDU_FIT_POINT_RAISE( "cmnOpenssl::initialize::calloc::Mutex",
                          InsufficientMemory );

    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_CMN,
                                     mMutexCount,
                                     sizeof(PDL_thread_mutex_t),
                                     (void **)&mMutex,
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

    for (i = 0; i < mMutexCount; ++i)
    {
        IDE_TEST(idlOS::thread_mutex_init(&mMutex[i]) != IDE_SUCCESS);
    }

    mFuncs.CRYPTO_set_id_callback(callbackCryptoThreadId);
    mFuncs.CRYPTO_set_locking_callback(callbackCryptoLocking);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DLSYM_LIBSSL )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_DLSYM, sOpensslSoName, idlOS::dlerror()));
    }
    IDE_EXCEPTION( ERR_DLOPEN_LIBSSL )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_DLOPEN, sOpensslSoName, idlOS::dlerror()));
    }
    IDE_EXCEPTION( ERR_DLSYM_LIBCRYPTO )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_DLSYM, sCryptoSoName, idlOS::dlerror()));
    }
    IDE_EXCEPTION( ERR_DLOPEN_LIBCRYPTO )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_DLOPEN, sCryptoSoName, idlOS::dlerror()));
    }
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    destroy();

    return IDE_FAILURE;     
}

static IDE_RC cleanupSslLibrary(cmnOpensslFuncs *aFuncs)
{
    IDE_TEST(aFuncs == NULL);

    if (aFuncs->SSL_COMP_free_compression_methods != NULL)  /* BUG-47037 */
    {
        /* Release the internal table of compression methods built internally */
        aFuncs->SSL_COMP_free_compression_methods();
    }

    /* Exit the FIPS mode of operation */
    if (aFuncs->FIPS_mode_set != NULL) /* BUG-44362 */
    {
        (void)aFuncs->FIPS_mode_set(0);
    }
    else
    {
        /* ignore, if the OpenSSL doesn't have FIPS_mode_set() function for FIPS module. */
    }

    /* Crypto locking doesn't need anymore. */
    aFuncs->CRYPTO_set_locking_callback(NULL);
    aFuncs->CRYPTO_set_id_callback(NULL);

    /* According to the OpenSSL website, 
     * CRYPTO_set_id_callback() should be called on each thread;
     * however, this causes an application crash by a double-free error 
     * if every thread calls the function. */
    aFuncs->CRYPTO_cleanup_all_ex_data();

    /* Free error strings */
    aFuncs->ERR_free_strings();

    /* Remove all ciphers and digests from the internal table for them */
    aFuncs->EVP_cleanup();

    /* Unload ENGINE objects */
    aFuncs->ENGINE_cleanup();

    /* Finish and unload configuration modules (1: All modules) */
    aFuncs->CONF_modules_unload(1);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnOpenssl::destroy()
{
    SInt i;

    if( mLibInitialized == ID_TRUE )
    {
        (void)cleanupSslLibrary(&mFuncs);
        mLibInitialized = ID_FALSE;
    }
    else
    {
        /* No need to clean up */
    }

    if (mSslHandle != NULL)
    {
        (void)idlOS::dlclose(mSslHandle);
        mSslHandle = NULL;
    }
    else
    {
        /* mSslHandle is null */
    }

    if (mCryptoHandle != NULL)
    {
        (void)idlOS::dlclose(mCryptoHandle);
        mCryptoHandle = NULL;
    }
    else
    {
        /* mCryptoHandle is null */
    }

    /* BUG-45235 */
    if (mMutex != NULL)
    {
        for (i = 0; i < mMutexCount; ++i)
        {
            (void)idlOS::thread_mutex_destroy(&mMutex[i]);
        }

        (void)iduMemMgr::free(mMutex);
        mMutex = NULL;
        mMutexCount = 0;
    }
    else
    {
        /* already destroyed */
    }

    return IDE_SUCCESS;
}

ULong cmnOpenssl::callbackCryptoThreadId()
{
    return (acp_uint64_t)idlOS::thr_self();
}

void cmnOpenssl::callbackCryptoLocking(SInt aMode, SInt aType, const SChar *aFile, SInt aLine)
{
    PDL_UNUSED_ARG(aFile);
    PDL_UNUSED_ARG(aLine);

    if (aMode & CRYPTO_LOCK)
    {
        (void)idlOS::thread_mutex_lock(&mMutex[aType]);
    }
    else
    {
        (void)idlOS::thread_mutex_unlock(&mMutex[aType]);
    }
}

#endif /* CM_DISABLE_SSL */
