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

#ifndef CDBC_ERROR_H
#define CDBC_ERROR_H 1

#include <ulErrorCode.h>

ACP_EXTERN_C_BEGIN



#define CDBC_EXPNO_INVALID_ARGS         -1  /**< 잘못된 인자 사용 */
#define CDBC_EXPNO_BUF_NOT_ENOUGH       -2  /**< 버퍼가 충분하지 않음 */
#define CDBC_EXPNO_INVALID_VERFORM      -3  /**< 올바르지않은 버전 문자열 */
#define CDBC_EXPNO_VARSIZE_TYPE         -4  /**< 크기가 정해져있지 않은 타입 */
#define CDBC_EXPNO_INVALID_BIND_TYPE    -5  /**< 유효하지 않은 바인드 타입 */
#define CDBC_EXPNO_INVALID_FIELD_TYPE   -6  /**< 유효하지 않은 필드 타입 */



/* NULL-term을 제외한 길이 */
#define ALTIBASE_MAX_ERRORMSG_LEN       2048 /* ACI_MAX_ERROR_MSG_LEN */
#define ALTIBASE_MAX_SQLSTATE_LEN       5



CDBC_INTERNAL void altibase_init_errinfo (cdbcABDiagRec *aDiagRec);
CDBC_INTERNAL void altibase_set_errinfo_v (cdbcABDiagRec *aErrorMgr,
                                           acp_uint32_t   aErrorCode,
                                           va_list        aArgs);
CDBC_INTERNAL void altibase_set_errinfo (cdbcABDiagRec *aDiagRec,
                                         acp_uint32_t   aErrorCode,
                                         ...);
CDBC_INTERNAL void altibase_set_errinfo_by_sqlhandle (cdbcABDiagRec *aDiagRec,
                                                      SQLSMALLINT    aHandleType,
                                                      SQLHANDLE      aHandle,
                                                      acp_rc_t       aRC);
CDBC_INLINE void altibase_set_errinfo_by_conndbc (cdbcABConn *aABConn, acp_rc_t aRC)
{
    altibase_set_errinfo_by_sqlhandle( &(aABConn->mDiagRec), SQL_HANDLE_DBC, aABConn->mHdbc, aRC );
}
CDBC_INLINE void altibase_set_errinfo_by_connstmt (cdbcABConn *aABConn, acp_rc_t aRC)
{
    altibase_set_errinfo_by_sqlhandle( &(aABConn->mDiagRec), SQL_HANDLE_STMT, aABConn->mHstmt, aRC );
}
CDBC_INLINE void altibase_set_errinfo_by_stmt (cdbcABStmt *aABStmt, acp_rc_t aRC)
{
    altibase_set_errinfo_by_sqlhandle( &(aABStmt->mDiagRec), SQL_HANDLE_STMT, aABStmt->mHstmt, aRC );
}
CDBC_INLINE void altibase_set_errinfo_by_res (cdbcABRes *aABRes, acp_rc_t aRC)
{
    altibase_set_errinfo_by_sqlhandle( aABRes->mDiagRec, SQL_HANDLE_STMT, aABRes->mHstmt, aRC );
}
CDBC_INTERNAL acp_uint32_t altibase_result_errno (ALTIBASE_RES aABRes);



ACP_EXTERN_C_END

#endif /* CDBC_ERROR_H */

