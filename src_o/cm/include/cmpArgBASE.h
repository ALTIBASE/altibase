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

#ifndef _O_CMP_ARG_BASE_H_
#define _O_CMP_ARG_BASE_H_ 1


typedef struct cmpArgBASEError
{
    UInt        mErrorCode;
    cmtVariable mErrorMessage;
} cmpArgBASEError;

typedef struct cmpArgBASEHandshake
{
    UChar       mBaseVersion;
    UChar       mModuleID;
    UChar       mModuleVersion;
} cmpArgBASEHandshake;

typedef struct cmpArgBASESecureVersion
{
    cmtVariable      mSecureVersion;
} cmpArgBASESecureVersion;

typedef struct cmpArgBASESecureVersionResult
{
    cmtVariable      mRandomValue;
    cmtVariable      mSvrCertificate;
} cmpArgBASESecureVersionResult;

typedef struct cmpArgBASESecureEnvelope
{
    cmtVariable      mEnvelope;
} cmpArgBASEEnvelope;

typedef struct cmpArgBASESecureEnvelopeResult
{
    cmtVariable      mValue;
} cmpArgBASEEnvelopeResult;

typedef union cmpArgBASE
{
    cmpArgBASEError                mError;
    cmpArgBASEHandshake            mHandshake;
    cmpArgBASESecureVersion        mSecureVersion;
    cmpArgBASESecureVersionResult  mSecureVersionResult;
    cmpArgBASESecureEnvelope       mSecureEnvelope;
    cmpArgBASESecureEnvelopeResult mSecureEnvelopeResult;
} cmpArgBASE;


#endif
