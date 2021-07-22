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

#ifndef _O_ULO_H_
#define _O_ULO_H_ 1

#include <sqlcli.h>
#include <uln.h>

ACP_EXTERN_C_BEGIN

#define SQL_ATTR_EXPLAIN_PLAN           3001

/*
 * --------------------------------------------
 * SQL_ATTR_INPUT_NTS
 *      가질 수 있는 값 : SQL_TRUE | SQL_FALSE
 * --------------------------------------------
 * SES 의 옵션 중 -n 옵션이 있다.
 *
 * SQLBindParameter() 의 마지막 인자인 indicator 포인터를 NULL 로 주면,
 * SQLBindParameter() 는 전달된 버퍼가 SQL_C_BINARY 이든, SQL_C_CHAR 이든간에
 * Null terminated 라고 간주한다.
 *
 * 그런데, SES 에서 그렇게 해서는 안될 경우들이 발견된다.
 * 그를 위해서 -n 옵션을 제공하는데,
 * 이 옵션을 들어갈 경우, indicator 가 NULL 이더라도 버퍼의 내용을
 * Null terminated 라고 가정해서는 안된다.
 * 버퍼의 사이즈와 정확히 같은 양의 내용이 들어 있다고 가정하고서 동작해야 한다.
 *
 * fix for BUG-13704
 *
 * 이 STMT 속성은 다음과 같은 값을 가진다 :
 *      SQL_TRUE  : default 값. indicator 가 NULL 이면 버퍼의 내용이 null terminated 라고 가정.
 *      SQL_FALSE : indocator 가 NULL 이더라도 버퍼의 내용은 null terminated 가 아니라고 가정.
 */
#define SQL_ATTR_INPUT_NTS              20011

/* PROJ-1719 */
typedef void (*ulxCallbackForSesConn)(acp_sint32_t aRmid,
                                      SQLHENV      aEnv,
                                      SQLHDBC      aDbc);
void ulxSetCallbackSesConn( ulxCallbackForSesConn );

//BUG-26374 XA_CLOSE시 Client 자원 정리가 되지 않습니다. 
typedef void (*ulxCallbackForSesDisConn)();
void ulxSetCallbackSesDisConn( ulxCallbackForSesDisConn );

ACP_EXTERN_C_END

#endif /* _O_ULO_H_ */
