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
 
#if !defined(AFX_GPKIAPI_H__E0B03FDB_D3C7_4437_9FFE_1CA9FBFC7885__INCLUDED_)
#define AFX_GPKIAPI_H__E0B03FDB_D3C7_4437_9FFE_1CA9FBFC7885__INCLUDED_

/* The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GPKIAPI_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GPKIAPI_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported. */

#ifdef WIN32
# ifdef GPKIAPI_EXPORTS
#  define GPKIAPI_API extern "C" __declspec(dllexport)
# else
#  ifdef __cplusplus
#   define GPKIAPI_API extern "C" __declspec(dllimport)
#  else
#   define GPKIAPI_API __declspec(dllimport)
#  endif
# endif
#else
# ifdef __cplusplus
#   define GPKIAPI_API extern "C"
# else
#   define GPKIAPI_API
# endif
#endif

#ifndef  __cplusplus
 #ifndef _BOOL
typedef enum {
 false = 0,
 true = 1
} bool;
#define _BOOL 1
 #endif
#endif

#include "gpkiapi_type.h"
#include "gpkiapi_error.h"

GPKIAPI_API int GPKI_API_Init(void OUT **ppCtx, char IN *pszWorkDir);
GPKIAPI_API int GPKI_API_Finish(void OUT **ppCtx);
GPKIAPI_API int GPKI_API_GetErrInfo(void IN *pCtx, int IN nAllocLen, char OUT *pszErrInfo);
GPKIAPI_API int GPKI_API_GetVersion(void IN *pCtx, int IN nAllocLen, char OUT *pszVersion);
GPKIAPI_API int GPKI_API_GetInfo(void IN *pCtx, int IN nAllocLen, char OUT *pszAPIInfo);
GPKIAPI_API int GPKI_API_SetOption(void IN *pCtx, int IN nOption);

GPKIAPI_API int GPKI_BINSTR_Create(BINSTR OUT *pBinStr);
GPKIAPI_API int GPKI_BINSTR_Delete(BINSTR OUT *pBinStr);
GPKIAPI_API int GPKI_BINSTR_SetData(unsigned char IN *pData, int IN nDataLen, BINSTR OUT *pBinStr);

GPKIAPI_API int GPKI_CERT_Load(void IN *pCtx, BINSTR IN *pCert);
GPKIAPI_API int GPKI_CERT_Unload(void IN *pCtx);
GPKIAPI_API int GPKI_CERT_GetUID(void IN *pCtx, int IN nAllocLen, char OUT *pszUID);
GPKIAPI_API int GPKI_CERT_GetVersion(void IN *pCtx, int OUT *pVersion);
GPKIAPI_API int GPKI_CERT_GetSerialNum(void IN *pCtx, int IN nAllocLen, char OUT *pszSerialNum);
GPKIAPI_API int GPKI_CERT_GetSignatureAlgorithm(void IN *pCtx, int IN nAllocLen, char OUT *pszSignAlg);
GPKIAPI_API int GPKI_CERT_GetIssuerName(void IN *pCtx, int IN nAllocLen, char OUT *pszIssuerName);
GPKIAPI_API int GPKI_CERT_GetValidity(void IN *pCtx, int IN nAllocLen, char OUT *pszValidity);
GPKIAPI_API int GPKI_CERT_GetSubjectName(void IN *pCtx, int IN nAllocLen, char OUT *pszSubjectName);
GPKIAPI_API int GPKI_CERT_GetPubKeyAlg(void IN *pCtx, int IN nAllocLen, char OUT *pszAlg);
GPKIAPI_API int GPKI_CERT_GetPubKeyLen(void IN *pCtx, int IN nAllocLen, char OUT *pszLen);
GPKIAPI_API int GPKI_CERT_GetPubKey(void IN *pCtx, int IN nAllocLen, char OUT *pszPubKey);
GPKIAPI_API int GPKI_CERT_GetSignature(void IN *pCtx, int IN nAllocLen, char OUT *pszSignature);
GPKIAPI_API int GPKI_CERT_GetKeyUsage(void IN *pCtx, int IN nAllocLen, char OUT *pszKeyUsage);
GPKIAPI_API int GPKI_CERT_GetBasicConstraints(void IN *pCtx, int IN nAllocLen, char OUT *pszBasicConstraints);
GPKIAPI_API int GPKI_CERT_GetCertPolicy(void IN *pCtx, int IN nAllocLen, char OUT *pszCertPolicy);
GPKIAPI_API int GPKI_CERT_GetCertPolicyID(void IN *pCtx, int IN nAllocLen, char OUT *pszCertPolicyID);
GPKIAPI_API int GPKI_CERT_GetExtKeyUsage(void IN *pCtx, int IN nAllocLen, char OUT *pszExtKeyUsage);
GPKIAPI_API int GPKI_CERT_GetSubjectAltName(void IN *pCtx, int IN nAllocLen, char OUT *pszSubAltName);
GPKIAPI_API int GPKI_CERT_GetAuthKeyID(void IN *pCtx, int IN nAllocLen, char OUT *pszAKI);
GPKIAPI_API int GPKI_CERT_GetSubKeyID(void IN *pCtx, int IN nAllocLen, char OUT *pszSKI);
GPKIAPI_API int GPKI_CERT_GetCRLDP(void IN *pCtx, int IN nAllocLen, char OUT *pszCRLDP);
GPKIAPI_API int GPKI_CERT_GetAIA(void IN *pCtx, int IN nAllocLen, char OUT *pszAIA);
GPKIAPI_API int GPKI_CERT_GetRemainDays(void IN *pCtx, BINSTR IN *pCert, int OUT *pRemainDays);
GPKIAPI_API int GPKI_CERT_AddCert(void IN *pCtx, BINSTR IN *pCert, BINSTR OUT *pCerts);
GPKIAPI_API int GPKI_CERT_GetCertCount(void IN *pCtx, BINSTR IN *pCerts, int OUT *pCnt);
GPKIAPI_API int GPKI_CERT_GetCert(void IN *pCtx, BINSTR IN *pCerts, int IN nIndex, BINSTR OUT *pCert);
GPKIAPI_API int GPKI_CERT_VerifyByIVS(void IN *pCtx, char IN *pszConfFilePath, BINSTR IN *pCert, BINSTR IN *pMyCert);

GPKIAPI_API int GPKI_CERT_AddTrustedCert(void IN *pCtx, BINSTR IN *pTrustedCert);
GPKIAPI_API int GPKI_CERT_SetVerifyEnv(void IN *pCtx, int IN nRange, int IN nCertCheck, 
									   bool IN bUseCache, char IN *pszTime, char IN *pszOCSPURL);
GPKIAPI_API int GPKI_CERT_Verify(void IN *pCtx, BINSTR IN *pCert, int IN nCertType, char IN *pszCertPolicies, 
								 char IN *pszConfFilePath, bool IN bSign, BINSTR IN *pMyCert, BINSTR IN *pMyPriKey);


GPKIAPI_API int GPKI_PRIKEY_Encrypt(void IN *pCtx, int IN nSymAlg, char IN *pszPasswd, BINSTR IN *pPriKey, BINSTR OUT *pEncPriKey);
GPKIAPI_API int GPKI_PRIKEY_Decrypt(void IN *pCtx, char IN *pszPasswd, BINSTR IN *pEncPriKey, BINSTR OUT *pPriKey);
GPKIAPI_API int GPKI_PRIKEY_ChangePasswd(void IN *pCtx, char IN *pszOldPasswd, char IN *pszNewPasswd, BINSTR IN *pEncPriKey, BINSTR OUT *pNewPriKey);
GPKIAPI_API int GPKI_PRIKEY_CheckKeyPair(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey);

GPKIAPI_API int GPKI_STORAGE_Load(void IN *pCtx, char IN *pszLibPath);
GPKIAPI_API int GPKI_STORAGE_Unload(void IN *pCtx);
GPKIAPI_API int GPKI_STORAGE_ReadCert(void IN *pCtx, int IN nMediaType, char IN *pszInfo, int IN nDataType, BINSTR OUT *pCert);
GPKIAPI_API int GPKI_STORAGE_ReadPriKey(void IN *pCtx, int IN nMediaType, char IN *pszInfo, char IN *pszPasswd, int IN nDataType, BINSTR OUT *pPriKey);
GPKIAPI_API int GPKI_STORAGE_WriteCert(void IN *pCtx, int IN nMediaType, char IN *pszInfo, int IN nDataType, BINSTR IN *pCert);
GPKIAPI_API int GPKI_STORAGE_WritePriKey(void IN *pCtx, int IN nMediaType, char IN *pszInfo, char IN *pszPasswd, int IN nDataType, int IN nSymAlg, BINSTR IN *pPriKey);
GPKIAPI_API int GPKI_STORAGE_DeleteCert(void IN *pCtx, int IN nMediaType, char IN *pszInfo, int IN nDataType);
GPKIAPI_API int GPKI_STORAGE_DeletePriKey(void IN *pCtx, int IN nMediaType, char IN *pszInfo, char IN *pszPasswd, int IN nDataType);
GPKIAPI_API int GPKI_STORAGE_ReadFile(void IN *pCtx, char IN *pszFilePath, BINSTR OUT *pData);
GPKIAPI_API int GPKI_STORAGE_WriteFile(void IN *pCtx, char IN *pszFilePath, BINSTR IN *pData);

GPKIAPI_API int GPKI_CMS_MakeSignedData(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR IN *pTBSData, char IN *pszSignTime, BINSTR OUT *pSignedData);
GPKIAPI_API int GPKI_CMS_MakeSignedData_File(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, char IN *pszTBSDataFilePath, char IN *pszSignTime, char IN *pszSignedDataFilePath);
GPKIAPI_API int GPKI_CMS_MakeSignedData_NoContent_File(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, char IN *pszTBSDataFilePath, char IN *pszSignTime, char IN *pszSignedDataFilePath);
GPKIAPI_API int GPKI_CMS_MakeSignedDataWithAddSigner(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR IN *pSignedDataIn, char IN *pszSignTime, BINSTR OUT *pSignedDataOut);
GPKIAPI_API int GPKI_CMS_ProcessSignedData(void IN *pCtx, BINSTR IN *pSignedData, BINSTR OUT *pData, BINSTR OUT *pSignerCert, char OUT szSignTime[20]);
GPKIAPI_API int GPKI_CMS_ProcessSignedData2(void IN *pCtx, BINSTR IN *pSignedData, BINSTR IN *pData, int OUT *pSignerCnt);
GPKIAPI_API int GPKI_CMS_ProcessSignedData_File(void IN *pCtx, char IN *pszSignedDataFilePath, char IN *pszDataFile, BINSTR OUT *pSignerCert, char OUT szSignTime[20]);
GPKIAPI_API int GPKI_CMS_ProcessSignedData_NoContent_File(void IN *pCtx, char IN *pszSignedDataFilePath, char IN *pszDataFile, BINSTR OUT *pSignerCert, char OUT szSignTime[20]);
GPKIAPI_API int GPKI_CMS_GetSigner(void IN *pCtx, int IN nIndex, BINSTR OUT *pCert, char OUT szSignTime[20]);
GPKIAPI_API int GPKI_CMS_MakeEnvelopedData(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pTBEData, int IN nSymAlg, BINSTR OUT *pEnvelopedData);
GPKIAPI_API int GPKI_CMS_MakeEnvelopedData_File(void IN *pCtx, BINSTR IN *pCert, char IN *pszTBEDataFilePath, int IN nSymAlg, char IN *pszEnvelopedDataFilePath);
GPKIAPI_API int GPKI_CMS_MakeEnvelopedData_NoContent_File(void IN *pCtx, BINSTR IN *pCert, char IN *pszTBEDataFilePath, int IN nSymAlg, char IN *pszEnvelopedDataFilePath, char IN *pszEncryptedContentFilePath);
GPKIAPI_API int GPKI_CMS_MakeEnvelopedDataWithMultiRecipients(void IN *pCtx, BINSTR IN *pCerts, BINSTR IN *pTBEData, int IN nSymAlg, BINSTR OUT *pEnvelopedData);
GPKIAPI_API int GPKI_CMS_ProcessEnvelopedData(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR IN *pEnvelopedData, BINSTR OUT *pData);
GPKIAPI_API int GPKI_CMS_ProcessEnvelopedData_File(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, char IN *pszEnvelopedDataFilePath, char OUT *pszDataFilePath);
GPKIAPI_API int GPKI_CMS_ProcessEnvelopedData_NoContent_File(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, char IN *pszEnvelopedDataFilePath, char IN *pszEncryptedConentFilePath, char IN *pszDataFilePath);
GPKIAPI_API int GPKI_CMS_MakeSignedAndEnvData(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR IN *pRecCert, BINSTR IN *pData, int IN nSymAlg, BINSTR OUT *pSignedAndEnvlopedData);
GPKIAPI_API int GPKI_CMS_ProcessSignedAndEnvData(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR IN *pSignedAndEnvlopedData, BINSTR OUT *pData, BINSTR OUT *pSignerCert);
GPKIAPI_API int GPKI_CMS_MakeEncryptedData(void IN *pCtx, BINSTR IN *pTBEData, BINSTR OUT *pEncryptedData);
GPKIAPI_API int GPKI_CMS_ProcessEncryptedData(void IN *pCtx, BINSTR IN *pKey, BINSTR IN *pEncryptedData, BINSTR OUT *pData);

GPKIAPI_API int GPKI_WCMS_MakeSignedContent(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR IN *pTBSData, char IN *pszSignTime, BINSTR OUT *pSignedContent);
GPKIAPI_API int GPKI_WCMS_ProcessSignedContent(void IN *pCtx, BINSTR IN *pSignedContent, BINSTR OUT *pData, BINSTR OUT *pSignerCert, char OUT szSignTime[20]);
GPKIAPI_API int GPKI_WCMS_MakeWapEnvelopedData(void IN *pCtx, BINSTR IN *pRecCert, BINSTR IN *pTBEData, int IN nSymAlg, BINSTR OUT *pWapEnvelopedData);
GPKIAPI_API int GPKI_WCMS_ProcessWapEnvelopedData(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR IN *pWapEnvelopedData, BINSTR OUT *pData);

GPKIAPI_API int GPKI_TSP_MakeReqMsg(void IN *pCtx, BINSTR IN *pMsg, int IN nHashAlg, char IN *pszPolicy, bool IN bSign, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR OUT *pReqMsg);
GPKIAPI_API int GPKI_TSP_SendAndRecv(void IN *pCtx, char IN *pszIP, int IN nPort, BINSTR IN *pReqMsg, BINSTR OUT *pResMsg);
GPKIAPI_API int GPKI_TSP_VerifyResMsg(void IN *pCtx, BINSTR IN *pResMsg, BINSTR OUT *pTSACert, BINSTR OUT *pToken);
GPKIAPI_API int GPKI_TSP_VerifyToken(void IN *pCtx, BINSTR IN *pDoc, BINSTR IN *pToken); 
GPKIAPI_API int GPKI_TSP_GetTokenInfo(void IN *pCtx, BINSTR IN *pToken, int IN nAllocLen, char OUT *pszCN, char OUT *pszDN, char OUT *pszPolicy, 
									  char OUT *pszHashAlg, char OUT *pszHashValue, char OUT *pszSerialNum, char OUT *pszGenTime, char OUT *pszNonce);

GPKIAPI_API int GPKI_VID_GetRandomFromPriKey(void IN *pCtx, BINSTR IN *pPriKey, BINSTR OUT *pRandom);
GPKIAPI_API int GPKI_VID_Verify(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pRandom, char IN *pszIDN);
GPKIAPI_API int GPKI_VID_VerifyByIVS(void IN *pCtx, char IN *pszConfFilePath, BINSTR IN *pCert, BINSTR IN *pRandom, 
									 char IN *pszIDN, BINSTR IN *pMyCert);

GPKIAPI_API int GPKI_CRYPT_GenRandom(void IN *pCtx, int IN nLen, BINSTR OUT *pRandom);
GPKIAPI_API int GPKI_CRYPT_GenKeyAndIV(void IN *pCtx, int IN nSymAlg);
GPKIAPI_API int GPKI_CRYPT_SetKeyAndIV(void IN *pCtx, int IN nSymAlg, BINSTR IN *pKey, BINSTR IN *pIV);
GPKIAPI_API int GPKI_CRYPT_GetKeyAndIV(void IN *pCtx, int OUT *pSymAlg, BINSTR OUT *pKey, BINSTR OUT *pIV);
GPKIAPI_API int GPKI_CRYPT_ClearKeyAndIV(void IN *pCtx);
GPKIAPI_API int GPKI_CRYPT_Encrypt(void IN *pCtx, BINSTR IN *pPlainText, BINSTR OUT *pCipherText);
GPKIAPI_API int GPKI_CRYPT_Encrypt_File(void *pCtx, char IN *pszPlainTextFile, char IN *pszCipherTextFile);
GPKIAPI_API int GPKI_CRYPT_Decrypt(void IN *pCtx, BINSTR IN *pCipherText, BINSTR OUT *pPlainText);
GPKIAPI_API int GPKI_CRYPT_Decrypt_File(void *pCtx, char IN *pszCipherTextFile, char IN *pszPlainTextFile);
GPKIAPI_API int GPKI_CRYPT_Sign(void IN *pCtx, BINSTR IN *pCert, BINSTR IN *pPriKey, int IN nHashAlg, BINSTR IN *pTBSData, BINSTR OUT *pSignature);
GPKIAPI_API int GPKI_CRYPT_Verify(void IN *pCtx, BINSTR IN *pCert, int IN nHashAlg, BINSTR IN *pData, BINSTR IN *pSignature);
GPKIAPI_API int GPKI_CRYPT_AsymEncrypt(void IN *pCtx, int IN nKeyType, BINSTR IN *pKey, BINSTR IN *pTBEData, BINSTR OUT *pEncData);
GPKIAPI_API int GPKI_CRYPT_AsymDecrypt(void IN *pCtx, int IN nKeyType, BINSTR IN *pKey, BINSTR IN *pEncData, BINSTR OUT *pData);
GPKIAPI_API int GPKI_CRYPT_Hash(void IN *pCtx, int IN nHashAlg, BINSTR IN *pTBHData, BINSTR OUT *pDigest);
GPKIAPI_API int GPKI_CRYPT_GenMAC(void IN *pCtx, int IN nMACAlg, char IN *pszPasswd, BINSTR IN *pTBMData, BINSTR OUT *pMAC);
GPKIAPI_API int GPKI_CRYPT_VerifyMAC(void IN *pCtx, int IN nMACAlg, char IN *pszPasswd, BINSTR IN *pData, BINSTR IN *pMAC);

GPKIAPI_API int GPKI_BASE64_Encode(void IN *pCtx, BINSTR IN *pData, BINSTR OUT *pEncData);
GPKIAPI_API int GPKI_BASE64_Decode(void IN *pCtx, BINSTR IN *pEncData, BINSTR OUT *pData);

GPKIAPI_API int GPKI_LDAP_GetDataByURL(void IN *pCtx, int IN nDataType, char IN *pszURL, BINSTR OUT *pData);
GPKIAPI_API int GPKI_LDAP_GetDataByCN(void IN *pCtx, char IN *pszIP, int IN nPort, char IN *pszCN, int IN nDataType, char OUT szFullDN[1024], BINSTR OUT *pData);
GPKIAPI_API int GPKI_LDAP_GetAnyDataByURL(void IN *pCtx, char IN *pszAttribute, char IN *pszURL, BINSTR OUT *pData);
GPKIAPI_API int GPKI_LDAP_GetCRLByCert(void IN *pCtx, BINSTR IN *pCert, BINSTR OUT *pCRL);
GPKIAPI_API int GPKI_LDAP_GetCertPath(void IN *pCtx, BINSTR IN *pCert, char IN *pszConfFilePath, BINSTR OUT *pPath);

GPKIAPI_API int GPKI_SIGEA_MakeChallenge(void IN *pCtx, BINSTR OUT *pChallenge);
GPKIAPI_API int GPKI_SIGEA_MakeResponse(void IN *pCtx, BINSTR IN *pChallenge, BINSTR IN *pCert, BINSTR IN *pPriKey, BINSTR OUT *pResponse);
GPKIAPI_API int GPKI_SIGEA_VerifyResponse(void IN *pCtx, BINSTR IN *pResponse, BINSTR IN *pChallenge, BINSTR IN *pCert);

#endif /* !defined(AFX_GPKIAPI_H__E0B03FDB_D3C7_4437_9FFE_1CA9FBFC7885__INCLUDED_) */
