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
 
#include <idl.h>
#include <ide.h>
#include <idsGPKI.h>

static PDL_SHLIB_HANDLE gIdsGPKIHandle;
static idBool           gIdsGPKIIsOpened;

static int (*gfnIdsGPKI_API_Init) ( void OUT **ppCtx , char IN *pszWorkDir);
static int (*gfnIdsGPKI_API_Finish) ( void OUT **ppCtx);
static int (*gfnIdsGPKI_API_GetVersion) ( void IN *pCtx , int IN nAllocLen , char OUT *pszVersion);
static int (*gfnIdsGPKI_BINSTR_Create) ( BINSTR OUT *pBinStr);
static int (*gfnIdsGPKI_BINSTR_Delete) ( BINSTR OUT *pBinStr);
static int (*gfnIdsGPKI_BINSTR_SetData) ( unsigned char IN *pData , int IN nDataLen , BINSTR OUT *pBinStr);
static int (*gfnIdsGPKI_CERT_Load) ( void IN *pCtx , BINSTR IN *pCert);
static int (*gfnIdsGPKI_CERT_Unload) (void IN *pCtx);
static int (*gfnIdsGPKI_CERT_GetSubjectName) ( void IN *pCtx , int IN nAllocLen , char OUT *pszSubjectName);
static int (*gfnIdsGPKI_CERT_Verify) (
        void IN *pCtx
        , BINSTR IN *pCert
        , int IN nCertType
        , char IN *pszCertPolicies
        , char IN *pszConfFilePath
        , bool IN bSign
        , BINSTR IN *pMyCert
        , BINSTR IN *pMyPriKey);
static int (*gfnIdsGPKI_PRIKEY_CheckKeyPair) (
        void IN *pCtx
        , BINSTR IN *pCert
        , BINSTR IN *pPriKey);
static int (*gfnIdsGPKI_STORAGE_ReadCert) (
        void IN *pCtx
        , int IN nMediaType
        , char IN *pszInfo
        , int IN nDataType
        , BINSTR OUT *pCert);
static int (*gfnIdsGPKI_STORAGE_ReadPriKey) (
        void IN *pCtx
        , int IN nMediaType
        , char IN *pszInfo
        , char IN *pszPasswd
        , int IN nDataType
        , BINSTR OUT *pPriKey);
static int (*gfnIdsGPKI_CMS_MakeSignedAndEnvData) (
        void IN *pCtx
        , BINSTR IN *pCert
        , BINSTR IN *pPriKey
        , BINSTR IN *pRecCert
        , BINSTR IN *pData
        , int IN nSymAlg
        , BINSTR OUT *pSignedAndEnvlopedData);
static int (*gfnIdsGPKI_CMS_ProcessSignedAndEnvData) (
        void IN *pCtx
        , BINSTR IN *pCert
        , BINSTR IN *pPriKey
        , BINSTR IN *pSignedAndEnvlopedData
        , BINSTR OUT *pData
        , BINSTR OUT *pSignerCert);
static int (*gfnIdsGPKI_CRYPT_GenRandom) ( void IN *pCtx , int IN nLen , BINSTR OUT *pRandom);
static int (*gfnIdsGPKI_CRYPT_GetKeyAndIV) (
        void IN *pCtx
        , int OUT *pSymAlg
        , BINSTR OUT *pKey
        , BINSTR OUT *pIV);
static int (*gfnIdsGPKI_CRYPT_Encrypt) ( void IN *pCtx , BINSTR IN *pPlainText , BINSTR OUT *pCipherText);
static int (*gfnIdsGPKI_CRYPT_Decrypt) ( void IN *pCtx , BINSTR IN *pCipherText , BINSTR OUT *pPlainText);


IDE_RC idsGPKIOpen()
{
#if defined(WRS_VXWORKS) || defined(VC_WINCE) || defined(ANDROID)
    return IDE_SUCCESS;
#else
    PDL_SHLIB_HANDLE sLDAPHandle;

    gIdsGPKIIsOpened = ID_FALSE;

    sLDAPHandle = idlOS::dlopen("libldap.so", RTLD_NOW|RTLD_GLOBAL);
    IDE_TEST_RAISE(sLDAPHandle == PDL_SHLIB_INVALID_HANDLE, IDSGPKI_ERR_NO_LIB)

    gIdsGPKIHandle = idlOS::dlopen("libgpkiapi.so", RTLD_NOW|RTLD_LOCAL);
    IDE_TEST_RAISE(gIdsGPKIHandle == PDL_SHLIB_INVALID_HANDLE, IDSGPKI_ERR_NO_LIB)

    *(void**)&gfnIdsGPKI_API_Finish                  = idlOS::dlsym(gIdsGPKIHandle,"GPKI_API_Finish");
    *(void**)&gfnIdsGPKI_API_GetVersion              = idlOS::dlsym(gIdsGPKIHandle,"GPKI_API_GetVersion");
    *(void**)&gfnIdsGPKI_API_Init                    = idlOS::dlsym(gIdsGPKIHandle,"GPKI_API_Init");
    *(void**)&gfnIdsGPKI_BINSTR_Create               = idlOS::dlsym(gIdsGPKIHandle,"GPKI_BINSTR_Create");
    *(void**)&gfnIdsGPKI_BINSTR_Delete               = idlOS::dlsym(gIdsGPKIHandle,"GPKI_BINSTR_Delete");
    *(void**)&gfnIdsGPKI_BINSTR_SetData              = idlOS::dlsym(gIdsGPKIHandle,"GPKI_BINSTR_SetData");
    *(void**)&gfnIdsGPKI_CERT_GetSubjectName         = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CERT_GetSubjectName");
    *(void**)&gfnIdsGPKI_CERT_Load                   = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CERT_Load");
    *(void**)&gfnIdsGPKI_CERT_Unload                 = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CERT_Unload");
    *(void**)&gfnIdsGPKI_CERT_Verify                 = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CERT_Verify");
    *(void**)&gfnIdsGPKI_CMS_MakeSignedAndEnvData    = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CMS_MakeSignedAndEnvData");
    *(void**)&gfnIdsGPKI_CMS_ProcessSignedAndEnvData = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CMS_ProcessSignedAndEnvData");
    *(void**)&gfnIdsGPKI_CRYPT_Decrypt               = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CRYPT_Decrypt");
    *(void**)&gfnIdsGPKI_CRYPT_Encrypt               = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CRYPT_Encrypt");
    *(void**)&gfnIdsGPKI_CRYPT_GenRandom             = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CRYPT_GenRandom");
    *(void**)&gfnIdsGPKI_CRYPT_GetKeyAndIV           = idlOS::dlsym(gIdsGPKIHandle,"GPKI_CRYPT_GetKeyAndIV");
    *(void**)&gfnIdsGPKI_PRIKEY_CheckKeyPair         = idlOS::dlsym(gIdsGPKIHandle,"GPKI_PRIKEY_CheckKeyPair");
    *(void**)&gfnIdsGPKI_STORAGE_ReadCert            = idlOS::dlsym(gIdsGPKIHandle,"GPKI_STORAGE_ReadCert");
    *(void**)&gfnIdsGPKI_STORAGE_ReadPriKey          = idlOS::dlsym(gIdsGPKIHandle,"GPKI_STORAGE_ReadPriKey");
    IDE_TEST_RAISE(gfnIdsGPKI_API_Finish                  == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_API_GetVersion              == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_API_Init                    == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_BINSTR_Create               == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_BINSTR_Delete               == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_BINSTR_SetData              == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CERT_GetSubjectName         == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CERT_Load                   == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CERT_Unload                 == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CERT_Verify                 == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CMS_MakeSignedAndEnvData    == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CMS_ProcessSignedAndEnvData == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CRYPT_Decrypt               == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CRYPT_Encrypt               == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CRYPT_GenRandom             == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_CRYPT_GetKeyAndIV           == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_PRIKEY_CheckKeyPair         == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_STORAGE_ReadCert            == NULL, IDSGPKI_ERR_NO_FUNC);
    IDE_TEST_RAISE(gfnIdsGPKI_STORAGE_ReadPriKey          == NULL, IDSGPKI_ERR_NO_FUNC);

    gIdsGPKIIsOpened = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_NO_LIB);
    {
        SChar * sError;
        SChar   sMsgBuf[512];
        if ((sError = idlOS::dlerror()) != NULL)
        {
            idlOS::snprintf( sMsgBuf, ID_SIZEOF(sMsgBuf), "libgpkiapi.so open error %s", sError);
        }
        else
        {
            sMsgBuf[0] = '\0';
        }
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_dlopen,sMsgBuf));
        //fix BUG-18226.
        errno = 0;       
    }
    IDE_EXCEPTION(IDSGPKI_ERR_NO_FUNC);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_dlsym));
    }
    IDE_EXCEPTION_END;

    gIdsGPKIIsOpened = ID_FALSE;

    return IDE_FAILURE;
#endif
}

IDE_RC idsGPKIClose()
{
#if defined(WRS_VXWORKS) || defined (VC_WINCE) || defined(ANDROID)
    return IDE_SUCCESS;
#else
    IDE_TEST_RAISE(gIdsGPKIHandle == PDL_SHLIB_INVALID_HANDLE, IDSGPKI_GOTO_SUCCESS)

    IDE_TEST_RAISE(idlOS::dlclose(gIdsGPKIHandle) != 0, IDSGPKI_ERR_LD_CLOSE);

    IDE_EXCEPTION_CONT( IDSGPKI_GOTO_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_LD_CLOSE);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_dlclose));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

idBool idsGPKIIsOpened()
{
    return gIdsGPKIIsOpened;
}

/*
 * idsGPKIInitialize
 *
 * Actually, the following nine variables are input parameters.
 * Before this function called,
 * those variables are initialized at cmiAddSession(),
 * and are set at SQLSetConnectAttr() and cmpCallbackBASESecureVersion().
 * So, the initializion code for the following nine variables should not be here.
 * aCtx->mWorkDir            IN -
 * aCtx->mConfDir            IN -
 * aCtx->mUsrCert            IN -
 * aCtx->mUsrPri             IN -
 * aCtx->mUsrPassword        IN -
 * aCtx->mUsrAID             IN -
 * aCtx->mSvrCert            IN -
 * aCtx->mSvrPri             IN -
 * aCtx->mSvrPassword        IN -
 *
 * aCtx->mCtx                INOUT -
 *
 * aCtx->mVersionInfo        OUT - NULL
 * aCtx->mSymAlg             OUT - NULL
 * aCtx->mSvrDN              OUT - NULL
 * aCtx->mUsrDN              OUT - NULL
 * aCtx->bsUsrCert           OUT - NULL
 * aCtx->bsUsrPri            OUT - NULL
 * aCtx->bsSvrCert           OUT - NULL
 * aCtx->bsSvrPri            OUT - NULL
 * aCtx->bsSvrRand           OUT - NULL
 * aCtx->bsKey               OUT - NULL
 * aCtx->bsIV                OUT - NULL
 * aCtx->bsSignedAndEnvelopedData OUT - NULL
 * aCtx->bsProcessedData     OUT - NULL
 * aCtx->bsPlainText         OUT - NULL
 * aCtx->bsCipherText        OUT - NULL
 */
IDE_RC idsGPKIInitialize( idsGPKICtx * aCtx )
{
    SInt   sRc = GPKI_OK;

    IDE_TEST_RAISE( aCtx->mCtx != NULL, IDSGPKI_GOTO_SUCCESS );

    idlOS::memset(aCtx->mVersionInfo, 0, ID_SIZEOF(aCtx->mVersionInfo));

    gfnIdsGPKI_BINSTR_Create(&aCtx->bsUsrCert);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsUsrPri);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsSvrCert);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsSvrPri);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsSvrRand);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsKey);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsIV);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsSignedAndEnvelopedData);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsProcessedData);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsPlainText);
    gfnIdsGPKI_BINSTR_Create(&aCtx->bsCipherText);

    sRc = gfnIdsGPKI_API_Init(&aCtx->mCtx, aCtx->mWorkDir);
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_API_Init );

    IDE_EXCEPTION_CONT( IDSGPKI_GOTO_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_API_Init);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_API_Init,sRc));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * idsGPKIFinalize
 *
 * aCtx->mCtx                IN -
 *
 * aCtx->bsUsrCert           OUT - NULL
 * aCtx->bsUsrPri            OUT - NULL
 * aCtx->bsSvrCert           OUT - NULL
 * aCtx->bsSvrPri            OUT - NULL
 * aCtx->bsSvrRand           OUT - NULL
 * aCtx->bsKey               OUT - NULL
 * aCtx->bsIV                OUT - NULL
 * aCtx->bsSignedAndEnvelopedData OUT - NULL
 * aCtx->bsProcessedData     OUT - NULL
 * aCtx->bsPlainText         OUT - NULL
 * aCtx->bsCipherText        OUT - NULL
 */
IDE_RC idsGPKIFinalize( idsGPKICtx * aCtx )
{
    SInt   sRc  = GPKI_OK;

    IDE_TEST_RAISE( aCtx->mCtx == NULL, IDSGPKI_GOTO_SUCCESS );

    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsUsrCert);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsUsrPri);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsSvrCert);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsSvrPri);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsSvrRand);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsKey);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsIV);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsSignedAndEnvelopedData);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsProcessedData);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsPlainText);
    gfnIdsGPKI_BINSTR_Delete(&aCtx->bsCipherText);

    sRc = gfnIdsGPKI_API_Finish(&aCtx->mCtx);
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_API_Finish );

    IDE_EXCEPTION_CONT( IDSGPKI_GOTO_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_API_Finish);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_API_Finish,sRc));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * idsGPKIHandshake1_client
 *
 * aCtx->mCtx                 IN -
 * aCtx->mUsrCert             IN -
 * aCtx->mUsrPri              IN -
 * aCtx->mUsrPassword         IN -
 * aCtx->mUsrAID              IN -
 *
 * aCtx->mVersionInfo         OUT -
 * aCtx->bsUsrCert            OUT -
 * aCtx->bsUsrPri             OUT -
 */
IDE_RC idsGPKIHandshake1_client(idsGPKICtx * aCtx)
{
    SInt   sRc  = GPKI_OK;

    sRc = gfnIdsGPKI_API_GetVersion(
                  aCtx->mCtx
                  , sizeof(aCtx->mVersionInfo)
                  , aCtx->mVersionInfo );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_API_GetVersion );

    sRc = gfnIdsGPKI_STORAGE_ReadCert(
                      aCtx->mCtx
                    , MEDIA_TYPE_FILE_PATH
                    , aCtx->mUsrCert
                    , DATA_TYPE_GPKI_SIGN
                    , &aCtx->bsUsrCert );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_STORAGE_ReadCert );

    sRc = gfnIdsGPKI_STORAGE_ReadPriKey(
                      aCtx->mCtx
                    , MEDIA_TYPE_FILE_PATH
                    , aCtx->mUsrPri
                    , aCtx->mUsrPassword
                    , DATA_TYPE_GPKI_SIGN
                    , &aCtx->bsUsrPri );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_STORAGE_ReadPriKey );

    sRc = gfnIdsGPKI_PRIKEY_CheckKeyPair(
                     aCtx->mCtx
                   , &aCtx->bsUsrCert
                   , &aCtx->bsUsrPri );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_PRIKEY_CheckKeyPair );

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_API_GetVersion);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_API_GetVersion,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_STORAGE_ReadCert);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_STORAGE_ReadCert,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_STORAGE_ReadPriKey);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_STORAGE_ReadPriKey,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_PRIKEY_CheckKeyPair);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_PRIKEY_CheckKeyPair,sRc));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * idsGPKIHandshake1_server
 *
 * aCtx->mCtx                  IN -
 * aCtx->mSvrCert              IN -
 * aCtx->mSvrPri               IN -
 * aCtx->mSvrPassword          IN -
 * aVersion                    IN -
 *
 * aCtx->mVersionInfo          OUT -
 * aCtx->bsSvrCert             OUT -
 * aCtx->bsSvrPri              OUT -
 * aCtx->bsSvrRand             OUT -
 */
IDE_RC idsGPKIHandshake1_server( idsGPKICtx * aCtx, UChar * aVersion )
{
    SInt   sRc  = GPKI_OK;

    sRc = gfnIdsGPKI_API_GetVersion(
                  aCtx->mCtx
                  , sizeof(aCtx->mVersionInfo)
                  , aCtx->mVersionInfo );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_API_GetVersion );

    IDE_TEST_RAISE(idlOS::memcmp(
                    aCtx->mVersionInfo,
                    aVersion,
                    idlOS::strlen(aCtx->mVersionInfo)) != 0, IDSGPKI_ERR_VersionMismatched );

    sRc = gfnIdsGPKI_STORAGE_ReadCert(
                    aCtx->mCtx
                    , MEDIA_TYPE_FILE_PATH
                    , aCtx->mSvrCert
                    , DATA_TYPE_GPKI_SIGN
                    , &aCtx->bsSvrCert );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_STORAGE_ReadCert );

    sRc = gfnIdsGPKI_STORAGE_ReadPriKey(
                    aCtx->mCtx
                    , MEDIA_TYPE_FILE_PATH
                    , aCtx->mSvrPri
                    , aCtx->mSvrPassword
                    , DATA_TYPE_GPKI_SIGN
                    , &aCtx->bsSvrPri );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_STORAGE_ReadPriKey );

    sRc = gfnIdsGPKI_CRYPT_GenRandom(
                   aCtx->mCtx
                   , 20
                   , &aCtx->bsSvrRand );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CRYPT_GenRandom );

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_API_GetVersion);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_API_GetVersion,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_STORAGE_ReadCert);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_STORAGE_ReadCert,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_STORAGE_ReadPriKey);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_STORAGE_ReadPriKey,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CRYPT_GenRandom);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_PRIKEY_CheckKeyPair,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_VersionMismatched);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_VersionMismatched));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * idsGPKIHandshake2_client
 *
 * aCtx->mCtx                     IN -
 * aCtx->bsUsrCert                IN -
 * aCtx->bsUsrPri                 IN -
 * aSvrRandVal                    IN -
 * aSvrRandLen                    IN -
 * aSvrCertVal                    IN -
 * aSvrCertLen                    IN -
 *
 * aCtx->bsSvrRand                OUT -
 * aCtx->bsSvrCert                OUT -
 * aCtx->bsSignedAndEnvelopedData OUT -
 * aCtx->mSymAlg                  OUT -
 * aCtx->bsKey                    OUT -
 * aCtx->bsIV                     OUT -
 */
IDE_RC idsGPKIHandshake2_client(
  idsGPKICtx * aCtx
, UChar      * aSvrRandVal
, UInt         aSvrRandLen
, UChar      * aSvrCertVal
, UInt         aSvrCertLen )
{
    SInt   sRc  = GPKI_OK;

    sRc = gfnIdsGPKI_BINSTR_SetData(
                      aSvrRandVal
                    , aSvrRandLen
                    , &aCtx->bsSvrRand );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_BINSTR_SetData );

    sRc = gfnIdsGPKI_BINSTR_SetData(
                      aSvrCertVal
                    , aSvrCertLen
                    , &aCtx->bsSvrCert );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_BINSTR_SetData );

    sRc = gfnIdsGPKI_CMS_MakeSignedAndEnvData(
                    aCtx->mCtx
                    , &aCtx->bsUsrCert
                    , &aCtx->bsUsrPri
                    , &aCtx->bsSvrCert
                    , &aCtx->bsSvrRand
                    , SYM_ALG_SEED_CBC
                    , &aCtx->bsSignedAndEnvelopedData );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CMS_MakeSignedAndEnvData );

    sRc = gfnIdsGPKI_CRYPT_GetKeyAndIV(
                    aCtx->mCtx
                    , &aCtx->mSymAlg
                    , &aCtx->bsKey
                    , &aCtx->bsIV );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CRYPT_GetKeyAndIV );

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_BINSTR_SetData);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_BINSTR_SetData,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CMS_MakeSignedAndEnvData);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CMS_MakeSignedAndEnvData,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CRYPT_GetKeyAndIV);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CRYPT_GetKeyAndIV,sRc));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * idsGPKIHandshake2_server
 *
 * aCtx->mCtx                     IN -
 * aCtx->mConfDir                 IN -
 * aCtx->bsSvrCert                IN -
 * aCtx->bsSvrPri                 IN -
 * aCtx->bsSvrRand                IN -
 * aSignedAndEnvVal               IN -
 * aSignedAndEnvLen               IN -
 *
 * aCtx->bsSignedAndEnvelopedData OUT -
 * aCtx->bsProcessedData          OUT -
 * aCtx->bsUsrCert                OUT -
 * aCtx->mSymAlg                  OUT -
 * aCtx->bsKey                    OUT -
 * aCtx->bsIV                     OUT -
 * aCtx->mSvrDN                   OUT -
 * aCtx->mUsrDN                   OUT -
 */
IDE_RC idsGPKIHandshake2_server(
  idsGPKICtx * aCtx
, UChar      * aSignedAndEnvVal
, UInt         aSignedAndEnvLen )
{
    SInt   sRc  = GPKI_OK;

    sRc = gfnIdsGPKI_BINSTR_SetData(
                    aSignedAndEnvVal
                    , aSignedAndEnvLen
                    , &aCtx->bsSignedAndEnvelopedData );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_BINSTR_SetData );

    sRc = gfnIdsGPKI_CMS_ProcessSignedAndEnvData(
                       aCtx->mCtx
                       , &aCtx->bsSvrCert
                       , &aCtx->bsSvrPri
                       , &aCtx->bsSignedAndEnvelopedData
                       , &aCtx->bsProcessedData
                       , &aCtx->bsUsrCert );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CMS_ProcessSignedAndEnvData );

    sRc = gfnIdsGPKI_CRYPT_GetKeyAndIV(
                     aCtx->mCtx
                     , &aCtx->mSymAlg
                     , &aCtx->bsKey
                     , &aCtx->bsIV );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CRYPT_GetKeyAndIV );

    /*
     * jdlee@BUGBUG - need LDAP_INFO, error 2011
     */
    /*
    sRc = gfnIdsGPKI_CERT_Verify(
                    aCtx->mCtx
                    , &aCtx->bsUsrCert
                    , CERT_TYPE_SIGN
                    , NULL
                    , aCtx->mConfDir
                    , 1
                    , &aCtx->bsSvrCert
                    , &aCtx->bsSvrPri );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CERT_Verify );
    */

    sRc = gfnIdsGPKI_CERT_Load(
                 aCtx->mCtx
                 , &aCtx->bsSvrCert );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CERT_Load );

    sRc = gfnIdsGPKI_CERT_GetSubjectName(
                    aCtx->mCtx
                    , sizeof(aCtx->mSvrDN)
                    , aCtx->mSvrDN);
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CERT_GetSubjectName );

    sRc = gfnIdsGPKI_CERT_Unload( aCtx->mCtx );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CERT_Unload );

    sRc = gfnIdsGPKI_CERT_Load(
                 aCtx->mCtx
                 , &aCtx->bsUsrCert );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CERT_Load );

    sRc = gfnIdsGPKI_CERT_GetSubjectName(
                    aCtx->mCtx
                    , sizeof(aCtx->mUsrDN)
                    , aCtx->mUsrDN);
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CERT_GetSubjectName );

    sRc = gfnIdsGPKI_CERT_Unload( aCtx->mCtx );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CERT_Unload );

    IDE_TEST_RAISE(idlOS::memcmp(
            aCtx->bsSvrRand.pData,
            aCtx->bsProcessedData.pData,
            aCtx->bsSvrRand.nLen) != 0, IDSGPKI_ERR_ValueMismatched);

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_BINSTR_SetData);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_BINSTR_SetData,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CMS_ProcessSignedAndEnvData);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CMS_ProcessSignedAndEnvData,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CRYPT_GetKeyAndIV);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CRYPT_GetKeyAndIV,sRc));
    }
    /*
    IDE_EXCEPTION(IDSGPKI_ERR_CERT_Verify);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CERT_Verify,sRc));
    }
    */
    IDE_EXCEPTION(IDSGPKI_ERR_CERT_Load);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CERT_Load,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CERT_GetSubjectName);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CERT_GetSubjectName,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CERT_Unload);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CERT_Unload,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_ValueMismatched);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_ValueMismatched));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * idsGPKIEncrypt
 *
 * aCtx->mCtx                     IN -
 * aData                          INOUT - 
 * aMaxDataLen                    IN -
 *
 * aCtx->bsPlainText              OUT -
 * aCtx->bsCipherText             OUT -
 * aDataLen                       OUT -
 */
IDE_RC idsGPKIEncrypt( idsGPKICtx * aCtx, UChar * aData, UShort * aDataLen, UShort aMaxDataLen )
{
    SInt   sRc  = GPKI_OK;

    sRc = gfnIdsGPKI_BINSTR_SetData(
                      aData
                    , *aDataLen
                    , &aCtx->bsPlainText );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_BINSTR_SetData );

    sRc = gfnIdsGPKI_CRYPT_Encrypt(
                    aCtx->mCtx
                    , &aCtx->bsPlainText
                    , &aCtx->bsCipherText );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CRYPT_Encrypt );

    IDE_TEST_RAISE(aCtx->bsCipherText.nLen > aMaxDataLen, IDSGPKI_ERR_CypherTextTooLong);

    idlOS::memcpy(aData
            , aCtx->bsCipherText.pData
            , aCtx->bsCipherText.nLen );

    *aDataLen = aCtx->bsCipherText.nLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_BINSTR_SetData);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_BINSTR_SetData,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CRYPT_Encrypt);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CRYPT_Encrypt,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CypherTextTooLong);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CypherTextTooLong));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * idsGPKIDecrypt
 *
 * aCtx->mCtx                     IN -
 * aData                          INOUT - 
 * aMaxDataLen                    IN -
 *
 * aCtx->bsCipherText             OUT -
 * aCtx->bsPlainText              OUT -
 * aDataLen                       OUT -
 */
IDE_RC idsGPKIDecrypt( idsGPKICtx * aCtx , UChar * aData, UShort * aDataLen, UShort aMaxDataLen )
{
    SInt   sRc  = GPKI_OK;

    sRc = gfnIdsGPKI_BINSTR_SetData(
                      aData
                    , *aDataLen
                    , &aCtx->bsCipherText );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_BINSTR_SetData );

    sRc = gfnIdsGPKI_CRYPT_Decrypt(
                    aCtx->mCtx
                    , &aCtx->bsCipherText
                    , &aCtx->bsPlainText );
    IDE_TEST_RAISE( sRc != GPKI_OK, IDSGPKI_ERR_CRYPT_Decrypt );

    IDE_TEST_RAISE(aCtx->bsPlainText.nLen > aMaxDataLen, IDSGPKI_ERR_PlainTextTooLong);

    idlOS::memcpy(aData
            , aCtx->bsPlainText.pData
            , aCtx->bsPlainText.nLen );

    *aDataLen = aCtx->bsPlainText.nLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION(IDSGPKI_ERR_BINSTR_SetData);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_BINSTR_SetData,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_CRYPT_Decrypt);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_CRYPT_Decrypt,sRc));
    }
    IDE_EXCEPTION(IDSGPKI_ERR_PlainTextTooLong);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_GPKI_PlainTextTooLong));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

