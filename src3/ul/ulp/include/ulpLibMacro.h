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

#ifndef _ULP_LIB_MACRO_
#define _ULP_LIB_MACRO_ 1

/* ulpLibInterface */
#define ULP_LIB_INTER_FUNC_MAX (31)

/* ulpLibHash */
#define MAX_NUM_CONN_HASH_TAB_ROW (33)
#define MAX_NUM_STMT_HASH_TAB_ROW (32)

/* ulpLibConnection */
/* MAX_NUM_CONN = -1 : no limit*/
#define MAX_NUM_CONN           (-1)
#define NUM_UNNAME_STMT_CACHE  (1000)
#define MAX_CONN_NAME_LEN      (50)

/* ulpLibStmtCur */
#define MAX_STMT_NAME_LEN   (51)
#define MAX_CUR_NAME_LEN    (51)
#define MAX_QUERY_STR_LEN   (1024*256)

/* scrollable cursor type*/
typedef enum
{
    F_NONE = 0,
    F_FIRST,
    F_PRIOR,
    F_NEXT,
    F_LAST,
    F_CURRENT,
    F_RELATIVE,
    F_ABSOLUTE
} ulpFetchType;

/* ulpLibError */
#define MAX_ERRSTATE_LEN  (6)
#define MAX_ERRMSG_LEN (2048)
#define MAX_SQLSTATE_LEN  (6)
#define ERRCODE_NULL  (-9999)

/* ulpLibInterFunc */
#define MAX_CONN_STR   (1024)

#define SQLCODE_ALREADY_CONNECTED (0x90002)
#define SQLCODE_CONNAME_OVERFLOW  (0x90011)
#define SQLCODE_NO_STMTNAME       (0x90022)
#define SQLCODE_SQL_NO_DATA       (100)
#define SQLCODE_NO_CURSOR         (0x90021)

#define SQLMSG_SUCCESS             "Execute successfully"
#define SQLMSG_SQL_NO_DATA         "Not found data"
#define SQLMSG_BATCH_NOT_SUPPORTED "BATCH not supported"

#define SQLSTATE_SUCCESS     "00000"
#define SQLSTATE_SQL_NO_DATA "02000"
#define SQLSTATE_UNSAFE_NULL "22002"

#define STMT_ROLLBACK_RELEASE (3)


#define ULPGETCONNECTION(aConnName,aConnNode)                                       \
{                                                                                   \
        if ( aConnName != NULL )                                                    \
        {                                                                           \
            aConnNode = ulpLibConLookupConn( aConnName );                           \
            ACI_TEST_RAISE( (aConnNode == NULL) || (aConnNode->mValid == ACP_FALSE),\
                            ERR_NO_CONN );                                          \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            aConnNode = ulpLibConGetDefaultConn();                                  \
            ACI_TEST_RAISE( aConnNode->mHenv == SQL_NULL_HENV, ERR_NO_CONN );       \
        }                                                                           \
}

#define DEFAULT_CONNECTION_NAME "default connection"

#define ULPCHECKCONNECTIONNAME( aConnName ) (aConnName!=NULL)?aConnName:(acp_char_t*)DEFAULT_CONNECTION_NAME

/* for serialize code*/
#ifdef __CSURF__

#define ULP_SERIAL_BEGIN(serial_code) { serial_code; };
#define ULP_SERIAL_EXEC(serial_code,num) { serial_code; };
#define ULP_SERIAL_END(serial_code) { serial_code; };

#else

#define ULP_SERIAL_BEGIN(serial_code) {\
volatile acp_uint32_t vstSeed = 0;\
 if ( vstSeed++ == 0)\
{\
     serial_code;\
}

#define ULP_SERIAL_EXEC(serial_code,num) \
  if (vstSeed++ == num) \
{\
    serial_code;\
}\
  else \
{ \
     ACE_ASSERT(0);\
}

#define ULP_SERIAL_END(serial_code)\
  if (vstSeed++ != 0 )\
{\
      serial_code;\
}\
}
#endif

#define ENV_ALTIBASE_DATE_FORMAT  "ALTIBASE_DATE_FORMAT"

#define IS_DYNAMIC_VARIABLE( aSqlstmt )  ((aSqlstmt)->numofhostvar == 1) && ((aSqlstmt)->hostvalue[0].mType == H_SQLDA)

/* TASK-7218 Handling Multiple Errors */
#define MULTI_ERROR_INIT_SIZE (4)

#endif
