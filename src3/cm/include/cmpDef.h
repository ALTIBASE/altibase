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

#ifndef _O_CMP_DEF_H_
#define _O_CMP_DEF_H_ 1


/*
 * Protocol Modules
 */

enum
{
    CMP_MODULE_BASE = 0,
    CMP_MODULE_DB,
    CMP_MODULE_RP,
    CMP_MODULE_DK,
    CMP_MODULE_DR, // deprecated
    CMP_MODULE_MAX
};

// PROJ-1697 Performance view for protocol
#define CMP_OP_NAME_LEN 50

typedef struct cmpModuleMap
{
    const char *mCmpModuleName;
} cmpModuleMap;

typedef struct cmpOpMap
{
    const char *mCmpOpName;
} cmpOpMap;

/*
 * Protocol Super-Structure
 */

typedef struct cmpProtocol
{
    UChar  mOpID;

    /* PROJ-2733-Protocol 클라이언트가 지원하는 마지막 OpID, Handshake로 전달 받는다. */
    UChar  mClientLastOpID;

    void  *mFinalizeFunction;

    union
    {
        cmpArgBASE mBASE;
        cmpArgDB   mDB;
        cmpArgRP   mRP;
        cmpArgDK   mDK;

    } mArg;
} cmpProtocol;


/*
 * Protocol Callback Function
 */

typedef IDE_RC (*cmpCallbackFunction)(struct cmiProtocolContext *aProtocolContext,
                                      struct cmpProtocol        *aProtocol,
                                      void                      *aSessionOwner,
                                      void                      *aUserContext);


/*
 * Protocol Marshal State
 */

typedef struct cmpMarshalState
{
    UInt mFieldLeft;
    UInt mSizeLeft;
} cmpMarshalState;


#endif
