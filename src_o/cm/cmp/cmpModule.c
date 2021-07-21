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


extern cmpModule gCmpModuleClientBASE;
extern cmpModule gCmpModuleClientDB;
extern cmpModule gCmpModuleClientRP;

cmpModule *gCmpModuleClient[CMP_MODULE_MAX] =
{
    &gCmpModuleClientBASE,
    &gCmpModuleClientDB,
    &gCmpModuleClientRP,
};

ACI_RC cmpArgNULL(cmpProtocol *aProtocol)
{
    ACP_UNUSED(aProtocol);

    return ACI_SUCCESS;
}

struct cmiProtocolContext;

ACI_RC cmpCallbackNULL(struct cmiProtocolContext *aProtocolContext,
                       struct cmpProtocol        *aProtocol,
                       void                      *aSessionOwner,
                       void                      *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aSessionOwner);
    ACP_UNUSED(aUserContext);

    ACI_SET(aciSetErrorCode(cmERR_ABORT_CALLBACK_DOES_NOT_EXIST));

    return ACI_FAILURE;
}

ACI_RC cmpModuleInitializeStatic()
{
    cmpModule   *sModule;
    acp_uint32_t i;
    acp_uint32_t j;

    /* BUG-43080 Marshal을 위한 설정은 mOpMaxA5를 사용해야 한다. */
#ifdef DEBUG
    for (i = CMP_MODULE_BASE; i < CMP_MODULE_MAX; i++)
    {
        sModule = gCmpModuleClient[i];

        for (j = 0; j < sModule->mOpMaxA5; j++)
        {
            ACE_ASSERT(sModule->mArgInitializeFunction[j] != NULL);
        }

        for (j = 0; j < sModule->mOpMaxA5; j++)
        {
            ACE_ASSERT(sModule->mArgFinalizeFunction[j] != NULL);
        }

        for (j = 0; j < sModule->mOpMaxA5; j++)
        {
            ACE_ASSERT(sModule->mReadFunction[j] != NULL);
        }

        for (j = 0; j < sModule->mOpMaxA5; j++)
        {
            ACE_ASSERT(sModule->mWriteFunction[j] != NULL);
        }
    }
#endif

    /* BUG-46400 mCallbackFunction은 A7용이므로 mOpMax를 사용해야 한다. */
    for (i = CMP_MODULE_BASE + 1; i < CMP_MODULE_MAX; i++)
    {
        sModule = gCmpModuleClient[i];

        for (j = 0; j < sModule->mOpMax; j++)
        {
            sModule->mCallbackFunction[j] = cmpCallbackNULL;
        }
    }

    return ACI_SUCCESS;
}

ACI_RC cmpModuleFinalizeStatic()
{
    return ACI_SUCCESS;
}
