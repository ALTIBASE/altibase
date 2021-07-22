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
 
#ifndef IDSGPKI
#define IDSGPKI

#include <idTypes.h>
#include "gpkiapi_main.h"

typedef struct idsGPKICtx {
    SChar   mWorkDir[1024];
    SChar   mConfDir[1024];
    SChar * mUsrCert;
    SChar * mUsrPri;
    SChar * mUsrPassword;
    SChar * mUsrAID;
    SChar   mSvrCert[1024];
    SChar   mSvrPri[1024];
    SChar   mSvrPassword[256];
  
    void  * mCtx;
  
    SInt    mSymAlg;
    SChar   mVersionInfo[16];
    SChar   mSvrDN[2048];
    SChar   mUsrDN[2048];
  
    BINSTR  bsUsrCert;
    BINSTR  bsUsrPri;
    BINSTR  bsSvrCert;
    BINSTR  bsSvrPri;
    BINSTR  bsSvrRand;
    BINSTR  bsKey;
    BINSTR  bsIV;
    BINSTR  bsSignedAndEnvelopedData;
    BINSTR  bsProcessedData;
    BINSTR  bsPlainText;
    BINSTR  bsCipherText;
} idsGPKICtx;

IDE_RC idsGPKIOpen();
IDE_RC idsGPKIClose();
idBool idsGPKIIsOpened();

IDE_RC idsGPKIInitialize( idsGPKICtx * aCtx );
IDE_RC idsGPKIFinalize( idsGPKICtx * aCtx );

IDE_RC idsGPKIHandshake1_client( idsGPKICtx * aCtx );
IDE_RC idsGPKIHandshake1_server( idsGPKICtx * aCtx, UChar * aVersion );

IDE_RC idsGPKIHandshake2_client( idsGPKICtx * aCtx
        , UChar  * aSvrRandVal
        , UInt     aSvrRandLen
        , UChar  * aSvrCertVal
        , UInt     aSvrCertLen );
IDE_RC idsGPKIHandshake2_server( idsGPKICtx * aCtx
        , UChar  * aSignedAndEnvVal
        , UInt     aSignedAndEnvLen );

IDE_RC idsGPKIEncrypt( idsGPKICtx * aCtx
        , UChar  * aData
        , UShort * aDataLen
        , UShort   aMaxDataLen );
IDE_RC idsGPKIDecrypt( idsGPKICtx * aCtx
        , UChar  * aData
        , UShort * aDataLen
        , UShort   aMaxDataLen );

#endif /* IDSGPKI */

