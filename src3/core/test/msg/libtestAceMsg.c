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
#include <aceMsgTable.h>
#include <testMsg.h>


ACP_EXPORT void libtestAceErrSet(void* aParam)
{
    aceErrorSetCallbackArea(aParam);
}

ACP_EXPORT void libtestAceMsgSet(void* aParam)
{
    aceMsgTableSetTableArea(aParam);
}

ACP_EXPORT acp_rc_t libtestAceMsg(void*       aContext,
                                  acp_char_t* aBuffer,
                                  acp_size_t  aBufferSize)
{
    acp_rc_t sRC;
    ACE_SET(ACE_SET_ERROR(ACE_LEVEL_1, TEST_ERR_ERROR4, 100));
    sRC = acpSnprintf(aBuffer, aBufferSize, "%s", ACE_GET_ERROR_MSG());

    return sRC;
}
