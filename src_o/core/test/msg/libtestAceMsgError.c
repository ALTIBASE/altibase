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
 
/*******************************************************************************
 * $Id:
 ******************************************************************************/

#include <acpCStr.h>
#include <acpError.h>
#include <acpPrintf.h>
#include <aclContext.h>
#include <testMsg.h>
#include <ace.h>
#include <act.h>

typedef struct testContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;
    acp_uint32_t mCount;
} testContext;

static void testCallback(testContext *aContext)
{
    ACT_CHECK(ACE_GET_ERROR_COMPARE(ACE_GET_ERROR(),
                                    TEST_ERR_ERROR4) == ACP_TRUE);

    aContext->mCount++;
}

static ace_error_callback_t *gTestErrorCallback[] =
{
    NULL,
    (ace_error_callback_t *)testCallback,
    (ace_error_callback_t *)testCallback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

ACE_ERROR_CALLBACK_DECLARE_GLOBAL_AREA;
static ace_error_callback_t*** gTestBackup;

ACP_EXPORT void libtestAceMsgInit(void)
{
    gTestBackup = aceErrorGetCallbackArea();
    ACE_ERROR_CALLBACK_SET_GLOBAL_AREA;

    aceSetErrorCallback(ACE_PRODUCT_TST, gTestErrorCallback);
}

ACP_EXPORT void libtestAceMsgSet(void* aParam)
{
    aceMsgTableSetTableArea(aParam);
}

ACP_EXPORT acp_rc_t libtestAceMsg(void*       aContext,
                                  acp_char_t* aBuffer,
                                  acp_size_t  aBufferSize)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    ACE_SET(ACE_SET_ERROR(ACE_LEVEL_1, TEST_ERR_ERROR4, 100));
    sRC = acpSnprintf(aBuffer, aBufferSize, "%s", ACE_GET_ERROR_MSG());
    ACE_SET(ACE_SET_ERROR(ACE_LEVEL_2, TEST_ERR_ERROR4, 100));

    return sRC;
}

ACP_EXPORT void libtestAceMsgFini(void)
{
    aceErrorSetCallbackArea(gTestBackup);
}
