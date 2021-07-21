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
 * Spatio-Temporal ¿¡·¯ °ü¸®ÀÚ 
 *
 ***********************************************************************/

#ifndef _O_ULS_ERROR_H_
#define _O_ULS_ERROR_H_    (1)

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>

#include <ulErrorCode.h>

typedef aci_client_error_mgr_t ulsErrorMgr;

/*----------------------------------------------------------------*
 *  Internal Interfaces
 *----------------------------------------------------------------*/

/* Error ÃÊ±âÈ­*/
void ulsClearError( ulsErrorMgr * aErrMgr );

/* Error Code ¼ÂÆÃ*/
void ulsErrorSetError( ulsErrorMgr *aErrMgr, acp_uint32_t aErrorCode, va_list aArgs );

/* Error Code È¹µæ*/
ACP_INLINE acp_uint32_t ulsGetErrorCode( ulsErrorMgr * aErrMgr )
{
    return ACI_E_ERROR_CODE( aErrMgr->mErrorCode );
}

/* Error Message È¹µæ */
ACP_INLINE acp_char_t *ulsGetErrorMsg( ulsErrorMgr * aErrMgr )
{
    return aErrMgr->mErrorMessage;
}
    
#endif /* _O_ULS_ERROR_H_ */

