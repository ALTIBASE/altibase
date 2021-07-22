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

#include <ulpErrorMgr.h>


ideClientErrorFactory gULPErrorFactory[] =
{
#include "E_LP_US7ASCII.c"
};

static ideClientErrorFactory *gClientFactory[] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gULPErrorFactory,
    NULL,
    NULL,
    NULL,
    NULL
};


SChar *ulpGetErrorSTATE(ulpErrorMgr *aMgr)
{
    return aMgr->mErrorState;
}

UInt   ulpGetErrorCODE (ulpErrorMgr *aMgr)
{
    return E_ERROR_CODE(aMgr->mErrorCode);
}

SChar *ulpGetErrorMSG  (ulpErrorMgr *aMgr)
{
    return aMgr->mErrorMessage;
}

void ulpClearError(ulpErrorMgr *aMgr)
{
    aMgr->mErrorCode       = 0;
    aMgr->mErrorMessage[0] = 0;

    idlOS::snprintf(aMgr->mErrorState, ID_SIZEOF(aMgr->mErrorState), "00000");
}

void ulpSetErrorCode(ulpErrorMgr * aMgr, UInt aErrorCode, ...)
{
    va_list                sArgs;
    UInt                   sSection;
    ideClientErrorFactory *sCurFactory;

    sSection = (aErrorCode & E_MODULE_MASK) >> 28;

    va_start(sArgs, aErrorCode);

    sCurFactory = gClientFactory[sSection];

    ideSetClientErrorCode( aMgr,
                           sCurFactory,
                           aErrorCode,
                           sArgs);

    va_end(sArgs);
}

#define ULP_PRINTF_FORMAT_CODE  "[ERR-%05"ID_XINT32_FMT" : %s]\n"
#define ULP_PRINTF_FORMAT_STATE "[ERR-%05"ID_XINT32_FMT"(%s) : %s]\n"

// format ERR-00000:MSG
void ulpPrintfErrorCode(FILE *aFP, ulpErrorMgr *aMgr)
{
    idlOS::fprintf( aFP,
                    ULP_PRINTF_FORMAT_CODE,
                    ulpGetErrorCODE(aMgr),
                    ulpGetErrorMSG(aMgr));
}
