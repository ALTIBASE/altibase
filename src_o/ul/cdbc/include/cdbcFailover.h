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

#ifndef CDBC_FAILOVER_H
#define CDBC_FAILOVER_H 1

ACP_EXTERN_C_BEGIN



#define SAFE_FAILOVER_POST_PROC(stmt) do {\
    if (altibase_stmt_errno(stmt) == ALTIBASE_FAILOVER_SUCCESS)\
    {\
        /* error를 반환하고 사용자에게 처리를 맡기므로 리턴값은 무시해도 된다. */\
        sRC = altibase_stmt_failover_postproc(stmt);\
    }\
} while (ACP_FALSE)

#define MAX_FAILOVER_RETRY 1

#define SAFE_FAILOVER_RETRY_INIT() \
    acp_uint32_t sFORetryCount = 0

#define SAFE_FAILOVER_POST_PROC_AND_RETRY(stmt, label) do {\
    if ((sFORetryCount < MAX_FAILOVER_RETRY)\
     && (altibase_stmt_errno(stmt) == ALTIBASE_FAILOVER_SUCCESS) )\
    {\
        sRC = altibase_stmt_failover_postproc(stmt);\
        if (sRC == ALTIBASE_SUCCESS)\
        {\
            sFORetryCount++;\
            CDBCLOG_PRINT_VAL("%d", sFORetryCount);\
            goto label;\
        }\
    }\
} while (ACP_FALSE)



CDBC_INTERNAL ALTIBASE_RC altibase_stmt_failover_postproc (cdbcABStmt *aABStmt);



ACP_EXTERN_C_END

#endif /* CDBC_FAILOVER_H */

