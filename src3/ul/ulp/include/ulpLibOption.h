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

#ifndef _ULP_LIB_OPTION_
#define _ULP_LIB_OPTION_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <ulpLibInterface.h>

/*
 * Structure Definition of the Library Option.
 */

typedef struct ulpLibOption
{
    /* Set when embedded option mutithread=true or command-line option -mt used */
    acp_bool_t mIsMultiThread;
    /* Set when command-line option -NCHAR_UTF16 used */
    acp_bool_t mIsNCharUTF16;
} ulpLibOption;

extern ulpLibOption gUlpLibOption;

/* BUG-43429 unsafe_null옵션을 stmt 옵션으로 변경 */
typedef enum
{
    /* Set when command-line option -n used */
    ULP_OPT_NOT_NULL_PAD = 0,
    /* Set when command-line option -unsafe_null used */
    ULP_OPT_UNSAFE_NULL,
    /* Set when command-line option stmtcache={on|off} used (현재 존재하지 않음.)*/
    ULP_OPT_STMT_CACHE
} ulpStmtOptionFlag;

acp_bool_t ulpGetStmtOption( ulpSqlstmt *aSqlstmt, ulpStmtOptionFlag aOptName );

#endif
