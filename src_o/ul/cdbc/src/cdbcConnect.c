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

#include <cdbc.h>



/**
 * 연결 문자열을 이용해 서버에 접속한다.
 *
 * @param[in] aABConn 연결 핸들
 * @param[in] aConnStr 연결 문자열
 * @return 성공하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_connect (ALTIBASE aABConn, const acp_char_t *aConnStr)
{
    #define CDBC_FUNC_NAME "altibase_connect"

    cdbcABConn   *sABConn = (cdbcABConn *) aABConn;
    ALTIBASE_RC   sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HCONN_NOT_VALID(sABConn), InvalidHandle);

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBCLOG_CALL("SQLDriverConnect");
    sRC = SQLDriverConnect(sABConn->mHdbc, NULL, (SQLCHAR *) aConnStr, SQL_NTS,
                           NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CDBCLOG_BACK_VAL("SQLDriverConnect", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

    CONN_SET_CONNECTED(sABConn);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(DBCError);
    {
        altibase_set_errinfo_by_conndbc(sABConn, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

