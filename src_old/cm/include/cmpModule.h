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

#ifndef _O_CMP_MODULE_H_
#define _O_CMP_MODULE_H_ 1


typedef IDE_RC (*cmpArgFunction)(cmpProtocol *aProtocol);

typedef IDE_RC (*cmpMarshalFunction)(cmbBlock        *aBlock,
                                     cmpProtocol     *aProtocol,
                                     cmpMarshalState *aMarshalState);


typedef struct cmpModule
{
    const SChar         *mName;

    UChar                mModuleID;

    UChar                mVersionMax;
    UChar                mOpMaxA5;  /* BUG-43080 */
    UChar                mOpMax;

    cmpArgFunction      *mArgInitializeFunction;
    cmpArgFunction      *mArgFinalizeFunction;

    cmpMarshalFunction  *mReadFunction;
    cmpMarshalFunction  *mWriteFunction;

    cmpCallbackFunction *mCallbackFunction;
    cmpCallbackFunction *mCallbackFunctionA5; /* proj_2160 cm_type removal */
} cmpModule;


extern cmpModule *gCmpModule[CMP_MODULE_MAX];
//fix BUG-17947.
extern cmpModule gCmpModuleBASE;
extern cmpModule gCmpModuleDB;
extern cmpModule gCmpModuleRP;
extern cmpModule gCmpModuleDK;
extern cmpModule gCmpModuleDR; // deprecated

IDE_RC cmpArgNULL(cmpProtocol *aProtocol);
IDE_RC cmpCallbackNULL(struct cmiProtocolContext *, cmpProtocol *, void *, void *);

IDE_RC cmpModuleInitializeStatic();
IDE_RC cmpModuleFinalizeStatic();


#endif
