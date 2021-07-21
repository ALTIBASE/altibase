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
 *
 * Spatio-Temporal Error 관리 함수
 *
 ***********************************************************************/


#include <ulsError.h>

/*----------------------------------------------------------------*
 *  Error Frame
 *  copy from ulnError.c
 *----------------------------------------------------------------*/


aci_client_error_factory_t gULSErrorFactory[] =
{
#include "../uln/E_UL_US7ASCII.c"
};

static aci_client_error_factory_t *gULSClientFactory[] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gULSErrorFactory,
    NULL,
    NULL,
    NULL,
    NULL /* gUtErrorFactory,*/
};




/*----------------------------------------------------------------*
 *  Internal Interfaces
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Error 를 초기화한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

void ulsClearError( ulsErrorMgr * aErrMgr )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACE_DASSERT( aErrMgr != NULL );

    /*------------------------------*/
    /* Initialize Error */
    /*------------------------------*/
    
    aErrMgr->mErrorCode = 0;
    aErrMgr->mErrorMessage[0] = 0;
    acpSnprintf(aErrMgr->mErrorState, 6, "00000");
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Error Code를 설정한다.
 *
 * Implementation:
 *
 *   copy from ulnErrorMgr.cpp
 *
 *---------------------------------------------------------------*/

void ulsErrorSetError( ulsErrorMgr *aErrMgr, acp_uint32_t aErrorCode, va_list aArgs )
{
    acp_uint32_t                sSection;
    aci_client_error_factory_t *sCurFactory;

    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACE_DASSERT( aErrMgr != NULL );

    /*------------------------------*/
    /* Set Error Code*/
    /*------------------------------*/

    sSection = (aErrorCode & ACI_E_MODULE_MASK) >> 28;

    sCurFactory = gULSClientFactory[sSection];

    aciSetClientErrorCode(aErrMgr,
                          sCurFactory,
                          aErrorCode,
                          aArgs);
}

