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

#ifndef _O_ULN_INIT_H_
#define _O_ULN_INIT_H_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>

ACP_EXTERN_C_BEGIN

ACI_RC ulnInitialize();
ACI_RC ulnFinalize();
void   ulnDestroy();

 /*fix BUG-25597 APRE에서 AIX플랫폼 턱시도 연동문제를 해결해야 합니다.
 */
void   ulnLoadOpenedXaConnections2APRE();

ACP_EXTERN_C_END

#endif /* _O_ULN_INIT_H_ */
