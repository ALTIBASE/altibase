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

#ifndef _O_CMP_MODULE_CLIENT_H_
#define _O_CMP_MODULE_CLIENT_H_ 1


typedef ACI_RC (*cmpArgFunction)(cmpProtocol *aProtocol);

typedef ACI_RC (*cmpMarshalFunction)(cmbBlock        *aBlock,
                                     cmpProtocol     *aProtocol,
                                     cmpMarshalState *aMarshalState);


typedef struct cmpModule
{
    const acp_char_t    *mName;

    acp_uint8_t          mModuleID;

    acp_uint8_t          mVersionMax;
    acp_uint8_t          mOpMaxA5;  /* BUG-43080 */
    acp_uint8_t          mOpMax;

    cmpArgFunction      *mArgInitializeFunction;
    cmpArgFunction      *mArgFinalizeFunction;

    cmpMarshalFunction  *mReadFunction;
    cmpMarshalFunction  *mWriteFunction;

    cmpCallbackFunction *mCallbackFunction;
    cmpCallbackFunction *mCallbackFunctionA5; /* proj_2160 cm_type removal */
} cmpModule;


extern cmpModule *gCmpModuleClient[CMP_MODULE_MAX];
/*
 * fix BUG-17947.
 */
extern cmpModule gCmpModuleClientBASE;
extern cmpModule gCmpModuleClientDB;
extern cmpModule gCmpModuleClientRP;

struct cmiProtocolContext;

ACI_RC cmpArgNULL(cmpProtocol *aProtocol);
ACI_RC cmpCallbackNULL(struct cmiProtocolContext *aProtocolContext,
                       struct cmpProtocol        *aProtocol,
                       void                      *aSessionOwner,
                       void                      *aUserContext);

ACI_RC cmpModuleInitializeStatic();
ACI_RC cmpModuleFinalizeStatic();


#endif
