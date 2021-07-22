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
 

/***********************************************************************
 * $Id: ulaErrorMgr.c 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <acp.h>
#include <acl.h>
#include <ace.h>

#include <ulaErrorMgr.h>

aci_client_error_factory_t gULAErrorFactory[] =
{
#include "E_LA_US7ASCII.c"
};

void ulaClearErrorMgr(ulaErrorMgr * aOutErrorMgr)
{
    if (aOutErrorMgr != NULL)
    {
        aOutErrorMgr->mErrorCode       = 0;
        aOutErrorMgr->mErrorMessage[0] = '\0';

        acpMemSet(aOutErrorMgr->mErrorState,
                  0x00,
                  ACI_SIZEOF(aOutErrorMgr->mErrorState));
    }
}

void ulaSetErrorCode(ulaErrorMgr *aErrorMgr, acp_uint32_t aErrorCode, ...)
{
    va_list sArgs;

    if (aErrorMgr != NULL)
    {
        va_start(sArgs, aErrorCode);

        aciSetClientErrorCode((aci_client_error_mgr_t *)aErrorMgr,
                              gULAErrorFactory,
                              aErrorCode,
                              sArgs);

        va_end(sArgs);
    }
}

acp_uint32_t ulaGetErrorCode(const ulaErrorMgr *aErrorMgr)
{
    if (aErrorMgr != NULL)
    {
        return aErrorMgr->mErrorCode;
    }
    else
    {
        return 0;
    }
}

const acp_char_t *ulaGetErrorMessage(const ulaErrorMgr *aErrorMgr)
{
    if (aErrorMgr != NULL)
    {
        return aErrorMgr->mErrorMessage;
    }
    else
    {
        return NULL;
    }
}
