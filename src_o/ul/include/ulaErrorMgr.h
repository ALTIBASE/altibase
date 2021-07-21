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
 
#ifndef _O_ULA_ERROR_MGR_H_
#define _O_ULA_ERROR_MGR_H_ 1

#include <cmErrorCodeClient.h>
#include <mtcErrorCode.h>
#if 0
#include <mmErrorCode.h>
#endif
#include <ulaErrorCode.h>
#include <ulnError.h>

typedef aci_client_error_mgr_t ulaErrorMgr;

void ulaClearErrorMgr(ulaErrorMgr *aOutErrorMgr);

void ulaSetErrorCode(ulaErrorMgr *aErrorMgr, acp_uint32_t aErrorCode, ...);

acp_uint32_t ulaGetErrorCode(const ulaErrorMgr *aErrorMgr);

const acp_char_t *ulaGetErrorMessage(const ulaErrorMgr *aErrorMgr);

#endif /* _O_ULA_ERROR_MGR_H_ */
