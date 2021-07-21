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

#include <cmAll.h>


extern cmpModule gCmpModuleBASE;
extern cmpModule gCmpModuleDB;
extern cmpModule gCmpModuleRP;
extern cmpModule gCmpModuleDK;
extern cmpModule gCmpModuleDR; // deprecated


cmpModule *gCmpModule[CMP_MODULE_MAX] =
{
    &gCmpModuleBASE,
    &gCmpModuleDB,
    &gCmpModuleRP,
    &gCmpModuleDK,
    &gCmpModuleDR, // deprecated
};

IDE_RC cmpArgNULL(cmpProtocol * /*aProtocol*/)
{
    
    return IDE_SUCCESS;
}

IDE_RC cmpCallbackNULL(cmiProtocolContext * /*aProtocolContext*/,
                       cmiProtocol        * /*aProtocol*/,
                       void               * /*aSessionOwner*/,
                       void               * /*aUserContext*/)
{
    
    IDE_SET(ideSetErrorCode(cmERR_ABORT_CALLBACK_DOES_NOT_EXIST));

    return IDE_FAILURE;
}

IDE_RC cmpModuleInitializeStatic()
{
    cmpModule *sModule;
    UInt       i;
    UInt       j;

    /* BUG-43080 Marshal을 위한 설정은 mOpMaxA5를 사용해야 한다. */
#ifdef DEBUG
    for (i = CMP_MODULE_BASE; i < CMP_MODULE_MAX; i++)
    {
        sModule = gCmpModule[i];

        for (j = 0; j < sModule->mOpMaxA5; j++)
        {
            IDE_ASSERT(sModule->mArgInitializeFunction[j] != NULL);
        }

        for (j = 0; j < sModule->mOpMaxA5; j++)
        {
            IDE_ASSERT(sModule->mArgFinalizeFunction[j] != NULL);
        }

        for (j = 0; j < sModule->mOpMaxA5; j++)
        {
            IDE_ASSERT(sModule->mReadFunction[j] != NULL);
        }

        for (j = 0; j < sModule->mOpMaxA5; j++)
        {
            IDE_ASSERT(sModule->mWriteFunction[j] != NULL);
        }
    }
#endif

    /* BUG-46400 mCallbackFunction은 A7용이므로 mOpMax를 사용해야 한다. */
    for (i = CMP_MODULE_BASE + 1; i < CMP_MODULE_MAX; i++)
    {
        sModule = gCmpModule[i];

        for (j = 0; j < sModule->mOpMax; j++)
        {
            sModule->mCallbackFunction[j] = cmpCallbackNULL;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC cmpModuleFinalizeStatic()
{
    return IDE_SUCCESS;
}
