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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnConnAttribute.h>
#include <ulnConnString.h>
#include <ulnString.h>

/*
 * Note : gULN_CM_PROTOCOL 은 url 파싱할 때만 쓴다.
 */
const ulnKeyLinkImpl gULN_CM_PROTOCOL[]=
{
    { "tcp"   , CMI_LINK_IMPL_TCP      },
    { "unix"  , CMI_LINK_IMPL_UNIX     },
    { "ipc"   , CMI_LINK_IMPL_IPC      },
    { "ipcda" , CMI_LINK_IMPL_IPCDA    },
    { "ssl"   , CMI_LINK_IMPL_SSL      },
    { "ib"    , CMI_LINK_IMPL_IB       },    /* PROJ-2681 */
    {  NULL   , CMI_LINK_IMPL_INVALID  }
};

/*
 * Note : Altibase property 파일의 모든 숫자에는 K, M, G 등의 metric 이 붙을 수 있다.
 *        ul 도 따라하자.
 */

const ulnMetricPrefixInt gULN_MUL[]=
{
    { "k",  1024 },
    { "K",  1024 },
    { "m",  1024*1024 }, 
    { "M",  1024*1024 },
    { "g",  1024*1024*1024 }, 
    { "G",  1024*1024*1024 },
    { NULL, ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnMetricPrefixInt gULN_MUL_TIME[]=
{
    { "s",     1 },
    { "sec",   1 },
    { "m",     60 },
    { "min",   60 },
    { "h",     3600 },
    { "hour",  3600 },
    { "hours", 3600 },
    { "d",     24*3600 },
    { "days",  24*3600 },

    { NULL,    ULN_CONN_ATTR_INVALID_VALUE }
};

/*
 * 몇가지 특정 값만 가지는 속성들
 */

const ulnDiscreteInt gULN_BOOL[]=
{
    /*
     * 이거, output connection string 만들 때 gULN_BOOL[SQL_TRUE]->mKey 처럼 참조한다 -0-;;
     * 거기의 것을 수정하지 않으면 최초의 on, off 는 바꾸면 안된다.
     */
    { "OFF",    SQL_FALSE },
    { "ON",     SQL_TRUE },
    //PROJ-1646 UL Fail-Over.
    { "off",    SQL_FALSE },
    { "on",     SQL_TRUE },

    { "YES",    SQL_TRUE },
    { "Y",      SQL_TRUE },
    { "TRUE",   SQL_TRUE },
    { "T",      SQL_TRUE },
    { "1",      SQL_TRUE },

    { "NO",     SQL_FALSE },
    { "N",      SQL_FALSE },
    { "FALSE",  SQL_FALSE },
    { "F",      SQL_FALSE },
    { "0",      SQL_FALSE },

    { NULL,     ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnDiscreteInt gULN_EXPLAIN_PLAN[]=
{
    { "OFF",    0 },
    { "ON",     1 },
    { "ONLY",   2 },
    { NULL,     ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnDiscreteInt gULN_OPTIMIZER_MODE[]=
{
    { "1",    ALTIBASE_OPTIMIZER_MODE_COST },
    { "2",    ALTIBASE_OPTIMIZER_MODE_RULE },
    { "COST", ALTIBASE_OPTIMIZER_MODE_COST }, 
    { "RULE", ALTIBASE_OPTIMIZER_MODE_RULE },

    {  NULL , ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnDiscreteInt gULN_CONNTYPE[] = 
{
    { "TCP"  , ULN_CONNTYPE_TCP },
    { "1"    , ULN_CONNTYPE_TCP },
    { "UNIX" , ULN_CONNTYPE_UNIX },
    { "2"    , ULN_CONNTYPE_UNIX },
    { "IPC"  , ULN_CONNTYPE_IPC },
    { "3"    , ULN_CONNTYPE_IPC },
    { "SSL"  , ULN_CONNTYPE_SSL },
    { "6"    , ULN_CONNTYPE_SSL },
    { "IPCDA", ULN_CONNTYPE_IPCDA },
    { "7"    , ULN_CONNTYPE_IPCDA },
    { "IB"   , ULN_CONNTYPE_IB },    /* PROJ-2681 */
    { "8"    , ULN_CONNTYPE_IB },

    { NULL   , ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnDiscreteInt gULN_SHARD_CONNTYPE[] = 
{
    { "TCP", ULN_CONNTYPE_TCP },
    { "1"  , ULN_CONNTYPE_TCP },
    { "SSL", ULN_CONNTYPE_SSL },
    { "6"  , ULN_CONNTYPE_SSL },
    { "IB" , ULN_CONNTYPE_IB },
    { "8"  , ULN_CONNTYPE_IB },

    { NULL , ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnDiscreteInt gULN_POOL[] =
{
    { "RETURN_TO_POOL", SQL_DB_RETURN_TO_POOL },
    { "DISCONNECT",     SQL_DB_DISCONNECT },

    { NULL,             ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnDiscreteInt gULN_SQL_TXN[] =
{
    { "READ_UNCOMMITTED", SQL_TXN_READ_UNCOMMITTED },
    { "READ_COMMITTED",   SQL_TXN_READ_COMMITTED },
    { "REPEATABLE_READ",  SQL_TXN_REPEATABLE_READ },
    { "SERIALIZABLE",     SQL_TXN_SERIALIZABLE },

    { NULL,               ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnDiscreteInt gSQL_OV_ODBC[] =
{
    { "2",        SQL_OV_ODBC2  },
    { "ODBC2",    SQL_OV_ODBC2  },
    { "OV_ODBC2", SQL_OV_ODBC2  },
    { "3",        SQL_OV_ODBC3  },
    { "ODBC3",    SQL_OV_ODBC3  },
    { "OV_ODBC3", SQL_OV_ODBC3  },

    { NULL,       ULN_CONN_ATTR_INVALID_VALUE }
};

const ulnDiscreteInt gULN_ACCESS_MODE[] = 
{
    { "READ_WRITE", SQL_MODE_READ_WRITE },
    { "READ_ONLY",  SQL_MODE_READ_ONLY },

    { NULL,         ULN_CONN_ATTR_INVALID_VALUE }
};


/*
 * 특정 문자열이면 정해진 값으로 치환하는 속성들
 */

const ulnReservedString gSQL_DATE[]=
{
    { "ISO", "yyyy-mm-dd" },
    { "USA", "mm/dd/yyyy" },
    { "EUR", "dd.mm.yyyy" },
    { "MDY", "mm/dd/yyyy" },
    { "DMY", "dd/mm/yyyy" },
    { "YMD", "yy/mm/dd" },

    { NULL,  "" }
};

const ulnReservedString gNULL_LIST[] = 
{ 
    { NULL  ,  "" } 
};

const ulnDiscreteInt gULN_PRIVILEGE[] =
{
    { "SYSDBA",  ULN_PRIVILEGE_SYSDBA },

    { NULL, ULN_CONN_ATTR_INVALID_VALUE }
};

/* bug-31468: adding conn-attr for trace logging */
/* bug-35142 cli trace log
   TraceLog 연결속성은 더이상 사용하지 않는다 */
const ulnDiscreteInt gULN_TRACELOG[] =
{
    { "off",     ULN_TRACELOG_OFF },
    { "low",     ULN_TRACELOG_LOW },
    { "mid",     ULN_TRACELOG_MID },
    { "high",    ULN_TRACELOG_HIGH},

    { NULL,      ULN_CONN_ATTR_INVALID_VALUE }
};

#define tINT        ULN_CONN_ATTR_TYPE_INT
#define tDINT       ULN_CONN_ATTR_TYPE_DISCRETE_INT
#define tSTR        ULN_CONN_ATTR_TYPE_STRING
#define tUSTR       ULN_CONN_ATTR_TYPE_UPPERCASE_STRING
#define tCB         ULN_CONN_ATTR_TYPE_CALLBACK
#define tPTR        ULN_CONN_ATTR_TYPE_POINTER

/*
 * ==============================================================================
 * BUGBUG : 속성이 세팅될 수 있는 시점도 이 표에 함게 넣어야 한다. Before, Either
 * ==============================================================================
 */

/* ulnConnAttribute.h의 ulnConnAttrID과 순서가 동일해야 한다. */
const ulnConnAttribute gUlnConnAttrTable[ULN_CONN_ATTR_MAX]  =
{
    { /* ConnAttrID  */ ULN_CONN_ATTR_DSN,
      /* Keyword     */ "DSN",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_UID,
      /* Keyword     */ "UID",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* SQL_ATTR_ID */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_PWD,
      /* Keyword     */ "PWD",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_URL,
      /* Keyword     */ "URL",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ NULL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    /* PROJ-1579 NCHAR */
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_NCHAR_LITERAL_REPLACE,
      /* Keyword     */ "NLS_NCHAR_LITERAL_REPLACE",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_NCHAR_LITERAL_REPLACE,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_FALSE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_CHARACTERSET_VALIDATION,
      /* Keyword     */ "NLS_CHARACTERSET_VALIDATION",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_CHARACTERSET_VALIDATION,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_TRUE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_USE,
      /* Keyword     */ "NLS_USE",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_USE,
      /* AttrType    */ tUSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_DATE_FORMAT,
      /* Keyword     */ "DATE_FORMAT",
      /* SQL_ATTR_ID */ ALTIBASE_DATE_FORMAT,
      /* AttrType    */ tSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 64,
      /* Check       */ gSQL_DATE,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_QUERY_TIMEOUT,
      /* Keyword     */ "QUERY_TIMEOUT",
      /* SQL_ATTR_ID */ SQL_ATTR_QUERY_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ ULN_QUERY_TMOUT_DEFAULT,
      /* Max         */ ULN_TMOUT_MAX,
      /* Check       */ gULN_MUL_TIME,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_AUTOCOMMIT,
      /* Keyword     */ "AUTOCOMMIT",
      /* SQL_ATTR_ID */ SQL_ATTR_AUTOCOMMIT,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_AUTOCOMMIT_OFF,
      /* Default     */ SQL_AUTOCOMMIT_ON,
      /* Max         */ SQL_AUTOCOMMIT_ON,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_LOGIN_TIMEOUT,
      /* Keyword     */ "LOGIN_TIMEOUT",
      /* SQL_ATTR_ID */ SQL_ATTR_LOGIN_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ ULN_LOGIN_TMOUT_DEFAULT,
      /* Max         */ ULN_TMOUT_MAX,
      /* Check       */ gULN_MUL_TIME,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_CONNECTION_TIMEOUT,
      /* Keyword     */ "CONNECTION_TIMEOUT",
      /* SQL_ATTR_ID */ SQL_ATTR_CONNECTION_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ ULN_CONN_TMOUT_DEFAULT,
      /* Max         */ ULN_TMOUT_MAX,
      /* Check       */ gULN_MUL_TIME,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_PACKET_SIZE,
      /* Keyword     */ "PACKET_SIZE",
      /* SQL_ATTR_ID */ SQL_ATTR_PACKET_SIZE,
      /* AttrType    */ tINT,
      /* Min         */ ULN_PACKET_SIZE_MIN,
      /* Default     */ ULN_PACKET_SIZE_DEFAULT,
      /* Max         */ ULN_PACKET_SIZE_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TXN_ISOLATION,
      /* Keyword     */ "TXN_ISOLATION",
      /* SQL_ATTR_ID */ SQL_ATTR_TXN_ISOLATION,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_TXN_READ_UNCOMMITTED,
      /* Default     */ ULN_ISOLATION_LEVEL_DEFAULT,
      /* Max         */ SQL_TXN_SERIALIZABLE,
      /* Check       */ gULN_SQL_TXN,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_ODBC_VERSION,
      /* Keyword     */ "ODBC_VERSION",
      /* SQL_ATTR_ID */ SQL_ATTR_ODBC_VERSION,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_OV_ODBC2,
      /* Default     */ SQL_OV_ODBC2,
      /* Max         */ SQL_OV_ODBC3,
      /* Check       */ gSQL_OV_ODBC,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_DISCONNECT_BEHAVIOR,
      /* Keyword     */ "DISCONNECT_BEHAVIOR",
      /* SQL_ATTR_ID */ SQL_ATTR_DISCONNECT_BEHAVIOR,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_DB_RETURN_TO_POOL,
      /* Default     */ SQL_DB_DISCONNECT,
      /* Max         */ SQL_DB_DISCONNECT,
      /* Check       */ gULN_POOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_CONNECTION_POOLING,
      /* Keyword     */ "CONNECTION_POOLING",
      /* SQL_ATTR_ID */ SQL_ATTR_CONNECTION_POOLING,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_FALSE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    /* DSN=192.168.1.11;UID=SYS;PWD=MANAGER;CONNTYPE=1;NLS_USE=US7ASCII;NLS_NCHAR_LITERAL_REPLACE=0;PORT_NO=20545" */
    { /* ConnAttrID  */ ULN_CONN_ATTR_CONNTYPE,
      /* Keyword     */ "CONNTYPE",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tDINT,
      /* Min         */ ULN_CONNTYPE_TCP,
      /* Default     */ ULN_CONNTYPE_TCP,
      /* Max         */ ULN_CONNTYPE_IB,    /* PROJ-2681 */
      /* Check       */ gULN_CONNTYPE,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_PORT_NO,
      /* Keyword     */ "PORT_NO",
      /* SQL_ATTR_ID */ SQL_ATTR_PORT,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 20300,
      /* Max         */ 65535,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_ASYNC_ENABLE,
      /* Keyword     */ "ASYNC_ENABLE",
      /* SQL_ATTR_ID */ SQL_ATTR_ASYNC_ENABLE,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_FALSE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_METADATA_ID,
      /* Keyword     */ "METADATA_ID",
      /* SQL_ATTR_ID */ SQL_ATTR_METADATA_ID,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ NULL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_EXPLAIN_PLAN,
      /* Keyword     */ "EXPLAIN_PLAN",
      /* SQL_ATTR_ID */ ALTIBASE_EXPLAIN_PLAN,
      /* AttrType    */ tDINT,
      /* Min         */ ALTIBASE_EXPLAIN_PLAN_OFF,
      /* Default     */ ALTIBASE_EXPLAIN_PLAN_OFF,
      /* Max         */ ALTIBASE_EXPLAIN_PLAN_ONLY,
      /* Check       */ gULN_EXPLAIN_PLAN,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_STACK_SIZE,
      /* Keyword     */ "STACK_SIZE",
      /* SQL_ATTR_ID */ ALTIBASE_STACK_SIZE,
      /* AttrType    */ tINT,
      /* Min         */ ULN_STACK_SIZE_MIN,
      /* Default     */ ULN_STACK_SIZE_DEFAULT,
      /* Max         */ ULN_STACK_SIZE_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_ACCESS_MODE,
      /* Keyword     */ "ACCESS_MODE",
      /* SQL_ATTR_ID */ SQL_ATTR_ACCESS_MODE,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_MODE_READ_WRITE,
      /* Default     */ SQL_MODE_DEFAULT,
      /* Max         */ SQL_MODE_READ_ONLY,
      /* Check       */ gULN_ACCESS_MODE,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    /* Optimizer Mode 가 뭐지? */
    { /* ConnAttrID  */ ULN_CONN_ATTR_OPTIMIZER_MODE,
      /* Keyword     */ "OPTIMIZER_MODE",
      /* SQL_ATTR_ID */ ALTIBASE_OPTIMIZER_MODE,
      /* AttrType    */ tDINT,
      /* Min         */ ALTIBASE_OPTIMIZER_MODE_COST,
      /* Default     */ ALTIBASE_OPTIMIZER_MODE_RULE,
      /* Max         */ ALTIBASE_OPTIMIZER_MODE_RULE,
      /* Check       */ gULN_OPTIMIZER_MODE,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* default ID_UITN_MAX mean server side default do get */
    { /* ConnAttrID  */ ULN_CONN_ATTR_UTRANS_TIMEOUT,
      /* Keyword     */ "UTRANS_TIMEOUT",
      /* SQL_ATTR_ID */ ALTIBASE_UTRANS_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ ACP_SINT32_MAX,
      /* Max         */ ULN_TMOUT_MAX,
      /* Check       */ gULN_MUL_TIME,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_FETCH_TIMEOUT,
      /* Keyword     */ "FETCH_TIMEOUT",
      /* SQL_ATTR_ID */ ALTIBASE_FETCH_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ ACP_SINT32_MAX,
      /* Max         */ ULN_TMOUT_MAX,
      /* Check       */ gULN_MUL_TIME,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_IDLE_TIMEOUT,
      /* Keyword     */ "IDLE_TIMEOUT",
      /* SQL_ATTR_ID */ ALTIBASE_IDLE_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ ACP_SINT32_MAX,
      /* Max         */ ULN_TMOUT_MAX,
      /* Check       */ gULN_MUL_TIME,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_HEADER_DISPLAY_MODE,
      /* Keyword     */ "HEADER_DISPLAY_MODE",
      /* SQL_ATTR_ID */ ALTIBASE_HEADER_DISPLAY_MODE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ ACP_SINT32_MAX,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_APP_INFO,
      /* Keyword     */ "APP_INFO",
      /* SQL_ATTR_ID */ ALTIBASE_APP_INFO,
      /* AttrType    */ tSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 128,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_LONGDATA_COMPAT,
      /* Keyword     */ "LONGDATA_COMPTAT",
      /* SQL_ATTR_ID */ SQL_ATTR_LONGDATA_COMPAT,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_FALSE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_HOSTNAME,
      /* Keyword     */ "Server",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_MESSAGE_CALLBACK,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ ALTIBASE_MESSAGE_CALLBACK,
      /* AttrType    */ tCB,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ NULL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_CURRENT_CATALOG,
      /* Keyword     */ "Database",
      /* SQL_ATTR_ID */ SQL_ATTR_CURRENT_CATALOG,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    /* BUGBUG : 구현도 안해놨으면서 그냥 세팅만 하고 모른척 하는건 찝찝한데... HYC00 내 줘야 할 것 같은데... 이 아랫쪽에 전부다 그런데...
     * Note   : 구현 안하고 그냥 세팅하려니 어떤 값도 받아주기 위해서 tDINT 인데도 tINT-gULN_MUL짝을, tSTR 이면 gNULL_LIST 를 씀 */
    { /* ConnAttrID  */ ULN_CONN_ATTR_ODBC_CURSORS,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_ODBC_CURSORS,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_ENLIST_IN_DTC,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_ENLIST_IN_DTC,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_ENLIST_IN_XA,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_ENLIST_IN_XA,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRACEFILE,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_TRACEFILE,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRANSLATE_LIB,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_TRANSLATE_LIB,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRANSLATE_OPTION,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_TRANSLATE_OPTION,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRACE,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_TRACE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_QUIET_MODE,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_QUIET_MODE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_GPKIWORKDIR,
      /* Keyword     */ "GPKIWORKDIR",
      /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_GPKIWORKDIR,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_GPKICONFDIR,
      /* Keyword     */ "GPKICONFDIR",
      /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_GPKICONFDIR,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_USERCERT,
      /* Keyword     */ "USERCERT",
      /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_USERCERT,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_USERKEY,
      /* Keyword     */ "USERKEY",
      /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_USERKEY,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_USERAID,
      /* Keyword     */ "USERAID",
      /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_USERAID,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_USERPASSWD,
      /* Keyword     */ "USERPASSWD",
      /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_USERPASSWD,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_XA_RMID,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ ALTIBASE_XA_RMID,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_XA_NAME,
      /* Keyword     */ "XA_NAME",
      /* SQL_ATTR_ID */ ALTIBASE_XA_NAME,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_XA_LOG_DIR,
      /* Keyword     */ "XA_LOG_DIR",
      /* SQL_ATTR_ID */ ALTIBASE_XA_LOG_DIR,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_UNIXDOMAIN_FILEPATH,
      /* Keyword     */ "UNIXDOMAIN_FILEPATH",
      /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_UNIXDOMAIN_FILEPATH,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_IPC_FILEPATH,
      /* Keyword     */ "IPC_FILEPATH",
      /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_IPC_FILEPATH,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_CHARACTERSET,
      /* Keyword     */ "NLS_CHARACTERSET",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_CHARACTERSET,
      /* AttrType    */ tUSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_NCHAR_CHARACTERSET,
      /* Keyword     */ "NLS_NCHAR_CHARACTERSET",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_NCHAR_CHARACTERSET,
      /* AttrType    */ tUSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_ANSI_APP,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ SQL_ATTR_ANSI_APP,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_FALSE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },                                      
    // bug-19279 remote sysdba enable
    { /* ConnAttrID  */ ULN_CONN_ATTR_PRIVILEGE,
      /* Keyword     */ "PRIVILEGE",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tDINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gULN_PRIVILEGE,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    //PROJ-1645 UL Fail-Over.
    { /* ConnAttrID  */ ULN_CONN_ATTR_ALTERNATE_SERVERS,
      /* Keyword     */ "ALTERNATE_SERVERS",
      /* SQL_ATTR_ID */ ALTIBASE_ALTERNATE_SERVERS,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_LOAD_BALANCE,
      /* Keyword     */ "LOAD_BALANCE",
      /* SQL_ATTR_ID */ ALTIBASE_LOAD_BALANCE,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_FALSE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_CONNECTION_RETRY_COUNT,
      /* Keyword     */ "CONNECTION_RETRY_COUNT",
      /* SQL_ATTR_ID */ ALTIBASE_CONNECTION_RETRY_COUNT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_CONN_RETRY_COUNT_MIN,
      /* Default     */ ULN_CONN_RETRY_COUNT_DEFAULT,
      /* Max         */ ULN_CONN_RETRY_COUNT_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_CONNECTION_RETRY_DELAY,
      /* Keyword     */ "CONNECTION_RETRY_DELAY",
      /* SQL_ATTR_ID */ ALTIBASE_CONNECTION_RETRY_DELAY,
      /* AttrType    */ tINT,
      /* Min         */ ULN_CONN_RETRY_DELAY_MIN,
      /* Default     */ ULN_CONN_RETRY_DELAY_DEFAULT,
      /* Max         */ ULN_CONN_RETRY_DELAY_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SESSION_FAILOVER,
      /* Keyword     */ "SESSION_FAILOVER",
      /* SQL_ATTR_ID */ ALTIBASE_SESSION_FAILOVER,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_FALSE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_FAILOVER_CALLBACK,
      /* Keyword     */ "",
      /* SQL_ATTR_ID */ ALTIBASE_FAILOVER_CALLBACK,
      /* AttrType    */ tCB,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ NULL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_PREFER_IPV6,
      /* Keyword     */ "PREFER_IPV6",
      /* SQL_ATTR_ID */ ALTIBASE_PREFER_IPV6,
      /* AttrType    */ tDINT,
      /* Min         */ SQL_FALSE,
      /* Default     */ SQL_FALSE,
      /* Max         */ SQL_TRUE,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-31144 */
    { /* ConnAttrID  */ ULN_CONN_ATTR_MAX_STATEMENTS_PER_SESSION,
      /* Keyword     */ "MAX_STATEMENTS_PER_SESSION",
      /* SQL_ATTR_ID */ ALTIBASE_MAX_STATEMENTS_PER_SESSION,
      /* AttrType    */ tINT,
      /* Min         */ 1,
      /* Default     */ 1024,
      /* Max         */ ACP_SINT32_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRACELOG,
      /* Keyword     */ "TRACELOG",
      /* SQL_ATTR_ID */ ALTIBASE_TRACELOG,
      /* AttrType    */ tDINT,
      /* Min         */ ULN_TRACELOG_OFF,
      /* Default     */ ULN_TRACELOG_OFF,
      /* Max         */ ULN_TRACELOG_HIGH,
      /* Check       */ gULN_TRACELOG,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    { /* ConnAttrID  */ ULN_CONN_ATTR_DDL_TIMEOUT,
      /* Keyword     */ "DDL_TIMEOUT",
      /* SQL_ATTR_ID */ ALTIBASE_DDL_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ ULN_DDL_TMOUT_DEFAULT,
      /* Max         */ ULN_TMOUT_MAX,
      /* Check       */ gULN_MUL_TIME,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* PROJ-2209 DBTIMEZONE */
    { /* ConnAttrID  */ ULN_CONN_ATTR_TIME_ZONE,
      /* Keyword     */ "TIME_ZONE",
      /* SQL_ATTR_ID */ ALTIBASE_TIME_ZONE,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 40,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* PROJ-1891 Deferred Prepare */
    { /* ConnAttrID  */ ULN_CONN_ATTR_DEFERRED_PREPARE,
      /* Keyword     */ "DEFER_PREPARES",
      /* SQL_ATTR_ID */ SQL_ATTR_DEFERRED_PREPARE,
      /* AttrType    */ tDINT,
      /* Min         */ ULN_CONN_DEFERRED_PREPARE_OFF,
      /* Default     */ ULN_CONN_DEFERRED_PREPARE_DEFAULT,
      /* Max         */ ULN_CONN_DEFERRED_PREPARE_ON,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    { /* ConnAttrID  */ ULN_CONN_ATTR_LOB_CACHE_THRESHOLD,
      /* Keyword     */ "LOB_CACHE_THRESHOLD",
      /* SQL_ATTR_ID */ ALTIBASE_LOB_CACHE_THRESHOLD,
      /* AttrType    */ tINT,
      /* Min         */ ULN_LOB_CACHE_THRESHOLD_MIN,
      /* Default     */ ULN_LOB_CACHE_THRESHOLD_DEFAULT,
      /* Max         */ ULN_LOB_CACHE_THRESHOLD_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-36548 Return code of client functions should be differed by ODBC version */
    { /* ConnAttrID  */ ULN_CONN_ATTR_ODBC_COMPATIBILITY,
      /* Keyword     */ "ODBC_COMPATIBILITY",
      /* SQL_ATTR_ID */ ALTIBASE_ODBC_COMPATIBILITY,
      /* AttrType    */ tINT,
      /* Min         */ 2,
      /* Default     */ 3,
      /* Max         */ 3,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
    { /* ConnAttrID  */ ULN_CONN_ATTR_FORCE_UNLOCK,
      /* Keyword     */ "FORCE_UNLOCK",
      /* SQL_ATTR_ID */ ALTIBASE_FORCE_UNLOCK,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* PROJ-2474 SSL/TLS */
    { /* ConnAttrID  */ ULN_CONN_ATTR_SSL_CA,
      /* Keyword     */ "SSL_CA",  /* CA file path */
      /* SQL_ATTR_ID */ ALTIBASE_SSL_CA,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    }, 
    { /* ConnAttrID  */ ULN_CONN_ATTR_SSL_CAPATH,
      /* Keyword     */ "SSL_CAPATH",
      /* SQL_ATTR_ID */ ALTIBASE_SSL_CA,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SSL_CERT,
      /* Keyword     */ "SSL_CERT",
      /* SQL_ATTR_ID */ ALTIBASE_SSL_CERT,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    }, 
    { /* ConnAttrID  */ ULN_CONN_ATTR_SSL_KEY,
      /* Keyword     */ "SSL_KEY",
      /* SQL_ATTR_ID */ ALTIBASE_SSL_KEY,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    }, 
    { /* ConnAttrID  */ ULN_CONN_ATTR_SSL_VERIFY,
      /* Keyword     */ "SSL_VERIFY",
      /* SQL_ATTR_ID */ ALTIBASE_SSL_VERIFY,
      /* AttrType    */ tDINT,
      /* Min         */ ULN_CONN_SSL_VERIFY_OFF,
      /* Default     */ ULN_CONN_SSL_VERIFY_DEFAULT,
      /* Max         */ ULN_CONN_SSL_VERIFY_ON,
      /* Check       */ gULN_BOOL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    }, 
    { /* ConnAttrID  */ ULN_CONN_ATTR_SSL_CIPHER,
      /* Keyword     */ "SSL_CIPHER",
      /* SQL_ATTR_ID */ ALTIBASE_SSL_CIPHER,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_IPCDA_FILEPATH,
        /* Keyword     */ "IPCDA_FILEPATH",
        /* SQL_ATTR_ID */ ALTIBASE_CONN_ATTR_IPCDA_FILEPATH,
        /* AttrType    */ tSTR,
        /* Min         */ 0,
        /* Default     */ 0,
        /* Max         */ 0,
        /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    { /* ConnAttrID  */ ULN_CONN_ATTR_SOCK_RCVBUF_BLOCK_RATIO,
      /* Keyword     */ "SOCK_RCVBUF_BLOCK_RATIO",
      /* SQL_ATTR_ID */ ALTIBASE_SOCK_RCVBUF_BLOCK_RATIO,
      /* AttrType    */ tINT,
      /* Min         */ ULN_CONN_SOCK_RCVBUF_BLOCK_RATIO_MIN,
      /* Default     */ ULN_CONN_SOCK_RCVBUF_BLOCK_RATIO_DEFAULT,
      /* Max         */ ULN_CONN_SOCK_RCVBUF_BLOCK_RATIO_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-44271 */
    { /* ConnAttrID  */ ULN_CONN_ATTR_SOCK_BIND_ADDR,
      /* Keyword     */ "SOCK_BIND_ADDR",
      /* SQL_ATTR_ID */ ALTIBASE_SOCK_BIND_ADDR,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_LINKER_TYPE,
      /* Keyword     */ "SHARD_LINKER_TYPE",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_NODE_NAME,
      /* Keyword     */ "SHARD_NODE_NAME",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_PIN,
      /* Keyword     */ "SHARD_PIN",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tINT,
      /* Min         */ ACP_SINT64_MIN,
      /* Default     */ 0,
      /* Max         */ ACP_SINT64_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-46090 Meta Node SMN 전파 */
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_META_NUMBER,
      /* Keyword     */ "SHARD_META_NUMBER",
      /* SQL_ATTR_ID */ ALTIBASE_UNUSED_07,
      /* AttrType    */ tINT,
      /* Min         */ ACP_UINT64_MIN,
      /* Default     */ 0,
      /* Max         */ ACP_UINT64_MAX,
      /* Check       */ gULN_MUL,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /*
     * BUG-45286 (http://nok.altibase.com/x/nRhrAg 참고)
     *
     * @0 : 지연없음, Default
     * @1 : ParamInfoSetList, ParamDataIn 지연
     * @2 : ParamInfoSetList, ParamDataIn, Free with Drop 지연
     */
    { /* ConnAttrID  */ ULN_CONN_ATTR_PDO_DEFER_PROTOCOLS,
      /* Keyword     */ "PDO_DEFER_PROTOCOLS",
      /* SQL_ATTR_ID */ ALTIBASE_PDO_DEFER_PROTOCOLS,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 2,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    /* PROJ-2681 */
    { /* ConnAttrID  */ ULN_CONN_ATTR_IB_LATENCY,
      /* Keyword     */ "IB_LATENCY",
      /* SQL_ATTR_ID */ ALTIBASE_IB_LATENCY,
      /* AttrType    */ tINT,
      /* Min         */ ALTIBASE_IB_LATENCY_NORMAL,
      /* Default     */ ALTIBASE_IB_LATENCY_NORMAL,
      /* Max         */ ALTIBASE_IB_LATENCY_PREFERRED,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_IB_CONCHKSPIN,
      /* Keyword     */ "IB_CONCHKSPIN",
      /* SQL_ATTR_ID */ ALTIBASE_IB_CONCHKSPIN,
      /* AttrType    */ tINT,
      /* Min         */ ALTIBASE_IB_CONCHKSPIN_MIN,
      /* Default     */ ALTIBASE_IB_CONCHKSPIN_DEFAULT,
      /* Max         */ ALTIBASE_IB_CONCHKSPIN_MAX,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-45998 Support Infiniband on the both of shardcli connections and meta (internal) connections. */
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_CONNTYPE,
      /* Keyword     */ "SHARD_CONNTYPE",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tDINT,
      /* Min         */ ULN_CONNTYPE_INVALID,
      /* Default     */ ULN_CONNTYPE_INVALID,
      /* Max         */ ULN_CONNTYPE_IB,
      /* Check       */ gULN_SHARD_CONNTYPE,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-45707 */
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_CLIENT,
      /* Keyword     */ "SHARD_CLIENT",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_SESSION_TYPE,
      /* Keyword     */ "SHARD_SESSION_TYPE",
      /* SQL_ATTR_ID */ ALTIBASE_UNUSED_04,  /* PROJ-2733-DistTxInfo */
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 2,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    /* BUG-46092 */
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_CLIENT_CONNECTION_REPORT,
      /* Keyword     */ "SHARD_CLIENT_CONNECTION_REPORT",
      /* SQL_ATTR_ID */ ACP_SINT32_MIN,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    /* BUG-47257 */
    { /* ConnAttrID  */ ULN_CONN_ATTR_GLOBAL_TRANSACTION_LEVEL,
      /* Keyword     */ "GLOBAL_TRANSACTION_LEVEL",
      /* SQL_ATTR_ID */ ALTIBASE_GLOBAL_TRANSACTION_LEVEL,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 1,
      /* Max         */ 3,  /* PROJ-2733-DistTxInfo */
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    // PROJ-2727 add connect attr
    { /* ConnAttrID  */ ULN_CONN_ATTR_COMMIT_WRITE_WAIT_MODE,
      /* Keyword     */ "COMMIT_WRITE_WAIT_MODE",
      /* SQL_ATTR_ID */ ALTIBASE_COMMIT_WRITE_WAIT_MODE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    }, 
    { /* ConnAttrID  */ ULN_CONN_ATTR_ST_OBJECT_BUFFER_SIZE,
      /* Keyword     */ "ST_OBJECT_BUFFER_SIZE",
      /* SQL_ATTR_ID */ ALTIBASE_ST_OBJECT_BUFFER_SIZE,
      /* AttrType    */ tINT,
      /* Min         */ 2000,
      /* Default     */ 32000,
      /* Max         */ 104857600,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRX_UPDATE_MAX_LOGSIZE,
      /* Keyword     */ "TRX_UPDATE_MAX_LOGSIZE",
      /* SQL_ATTR_ID */ ALTIBASE_TRX_UPDATE_MAX_LOGSIZE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 10485760,
      /* Max         */ ACP_UINT64_MAX,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_PARALLEL_DML_MODE,
      /* Keyword     */ "PARALLEL_DML_MODE",
      /* SQL_ATTR_ID */ ALTIBASE_PARALLEL_DML_MODE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ ACP_UINT32_MAX,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_NCHAR_CONV_EXCP,
      /* Keyword     */ "NLS_NCHAR_CONV_EXCP",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_NCHAR_CONV_EXCP,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_AUTO_REMOTE_EXEC,
      /* Keyword     */ "AUTO_REMOTE_EXEC",
      /* SQL_ATTR_ID */ ALTIBASE_AUTO_REMOTE_EXEC,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRCLOG_DETAIL_PREDICATE,
      /* Keyword     */ "TRCLOG_DETAIL_PREDICATE",
      /* SQL_ATTR_ID */ ALTIBASE_TRCLOG_DETAIL_PREDICATE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_OPTIMIZER_DISK_INDEX_COST_ADJ,
      /* Keyword     */ "OPTIMIZER_DISK_INDEX_COST_ADJ",
      /* SQL_ATTR_ID */ ALTIBASE_OPTIMIZER_DISK_INDEX_COST_ADJ,
      /* AttrType    */ tINT,
      /* Min         */ 1,
      /* Default     */ 100,
      /* Max         */ 10000,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_OPTIMIZER_MEMORY_INDEX_COST_ADJ,
      /* Keyword     */ "OPTIMIZER_MEMORY_INDEX_COST_ADJ",
      /* SQL_ATTR_ID */ ALTIBASE_OPTIMIZER_MEMORY_INDEX_COST_ADJ,
      /* AttrType    */ tINT,
      /* Min         */ 1,
      /* Default     */ 100,
      /* Max         */ 10000,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_TERRITORY,
      /* Keyword     */ "NLS_TERRITORY",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_TERRITORY,
      /* AttrType    */ tSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 64,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_ISO_CURRENCY,
      /* Keyword     */ "NLS_ISO_CURRENCY",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_ISO_CURRENCY,
      /* AttrType    */ tSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 64,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_CURRENCY,
      /* Keyword     */ "NLS_CURRENCY",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_CURRENCY,
      /* AttrType    */ tSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 64,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NLS_NUMERIC_CHARACTERS,
      /* Keyword     */ "NLS_NUMERIC_CHARACTERS",
      /* SQL_ATTR_ID */ ALTIBASE_NLS_NUMERIC_CHARACTERS,
      /* AttrType    */ tSTR,
      /* Min         */ 1,
      /* Default     */ 0,
      /* Max         */ 64,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_QUERY_REWRITE_ENABLE,
      /* Keyword     */ "QUERY_REWRITE_ENABLE",
      /* SQL_ATTR_ID */ ALTIBASE_QUERY_REWRITE_ENABLE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT,
      /* Keyword     */ "DBLINK_REMOTE_STATEMENT_AUTOCOMMIT",
      /* SQL_ATTR_ID */ ALTIBASE_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_RECYCLEBIN_ENABLE,
      /* Keyword     */ "RECYCLEBIN_ENABLE",
      /* SQL_ATTR_ID */ ALTIBASE_RECYCLEBIN_ENABLE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },    
    { /* ConnAttrID  */ ULN_CONN_ATTR___USE_OLD_SORT,
      /* Keyword     */ "__USE_OLD_SORT",
      /* SQL_ATTR_ID */ ALTIBASE___USE_OLD_SORT,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },    
    { /* ConnAttrID  */ ULN_CONN_ATTR_ARITHMETIC_OPERATION_MODE,
      /* Keyword     */ "ARITHMETIC_OPERATION_MODE",
      /* SQL_ATTR_ID */ ALTIBASE_ARITHMETIC_OPERATION_MODE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 1,
      /* Max         */ 3,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },    
    { /* ConnAttrID  */ ULN_CONN_ATTR_RESULT_CACHE_ENABLE,
      /* Keyword     */ "RESULT_CACHE_ENABLE",
      /* SQL_ATTR_ID */ ALTIBASE_RESULT_CACHE_ENABLE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },    
    { /* ConnAttrID  */ ULN_CONN_ATTR_TOP_RESULT_CACHE_MODE,
      /* Keyword     */ "TOP_RESULT_CACHE_MODE",
      /* SQL_ATTR_ID */ ALTIBASE_TOP_RESULT_CACHE_MODE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 3,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_OPTIMIZER_AUTO_STATS,
      /* Keyword     */ "OPTIMIZER_AUTO_STATS",
      /* SQL_ATTR_ID */ ALTIBASE_OPTIMIZER_AUTO_STATS,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 10,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR___OPTIMIZER_TRANSITIVITY_OLD_RULE,
      /* Keyword     */ "__OPTIMIZER_TRANSITIVITY_OLD_RULE",
      /* SQL_ATTR_ID */ ALTIBASE___OPTIMIZER_TRANSITIVITY_OLD_RULE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_OPTIMIZER_PERFORMANCE_VIEW,
      /* Keyword     */ "OPTIMIZER_PERFORMANCE_VIEW",
      /* SQL_ATTR_ID */ ALTIBASE_OPTIMIZER_PERFORMANCE_VIEW,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 1,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_REPLICATION_DDL_SYNC,
      /* Keyword     */ "REPLICATION_DDL_SYNC",
      /* SQL_ATTR_ID */ ALTIBASE_REPLICATION_DDL_SYNC,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_REPLICATION_DDL_SYNC_TIMEOUT,
      /* Keyword     */ "REPLICATION_DDL_SYNC_TIMEOUT",
      /* SQL_ATTR_ID */ ALTIBASE_REPLICATION_DDL_SYNC_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 7200,
      /* Max         */ ACP_UINT32_MAX,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR___PRINT_OUT_ENABLE,
      /* Keyword     */ "__PRINT_OUT_ENABLE",
      /* SQL_ATTR_ID */ ALTIBASE___PRINT_OUT_ENABLE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 1,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRCLOG_DETAIL_SHARD,
      /* Keyword     */ "TRCLOG_DETAIL_SHARD",
      /* SQL_ATTR_ID */ ALTIBASE_TRCLOG_DETAIL_SHARD,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SERIAL_EXECUTE_MODE,
      /* Keyword     */ "SERIAL_EXECUTE_MODE",
      /* SQL_ATTR_ID */ ALTIBASE_SERIAL_EXECUTE_MODE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 1,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRCLOG_DETAIL_INFORMATION,
      /* Keyword     */ "TRCLOG_DETAIL_INFORMATION",
      /* SQL_ATTR_ID */ ALTIBASE_TRCLOG_DETAIL_INFORMATION,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE,
      /* Keyword     */ "__OPTIMIZER_DEFAULT_TEMP_TBS_TYPE",
      /* SQL_ATTR_ID */ ALTIBASE___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 2,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_NORMALFORM_MAXIMUM,
      /* Keyword     */ "NORMALFORM_MAXIMUM",
      /* SQL_ATTR_ID */ ALTIBASE_NORMALFORM_MAXIMUM,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 2048,
      /* Max         */ ACP_UINT32_MAX,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },    
    { /* ConnAttrID  */ ULN_CONN_ATTR___REDUCE_PARTITION_PREPARE_MEMORY,
      /* Keyword     */ "__REDUCE_PARTITION_PREPARE_MEMORY",
      /* SQL_ATTR_ID */ ALTIBASE___REDUCE_PARTITION_PREPARE_MEMORY,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 1,
      /* Max         */ 3,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_TRANSACTIONAL_DDL,
      /* Keyword     */ "TRANSACTIONAL_DDL",
      /* SQL_ATTR_ID */ ALTIBASE_TRANSACTIONAL_DDL,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_INTERNAL_LOCAL_OPERATION,
      /* Keyword     */ "SHARD_INTERNAL_LOCAL_OPERATION",
      /* SQL_ATTR_ID */ ALTIBASE_UNUSED_02,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 3,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_FALSE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_INVOKE_USER,
      /* Keyword     */ "INVOKE_USER",
      /* SQL_ATTR_ID */ ALTIBASE_UNUSED_03,
      /* AttrType    */ tSTR,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 0,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_GLOBAL_DDL,
      /* Keyword     */ "GLOBAL_DDL",
      /* SQL_ATTR_ID */ ALTIBASE_GLOBAL_DDL,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 0,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_FALSE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_STATEMENT_RETRY,
      /* Keyword     */ "SHARD_STATEMENT_RETRY",
      /* SQL_ATTR_ID */ ALTIBASE_SHARD_STATEMENT_RETRY,
      /* AttrType    */ tINT,
      /* Min         */ ULN_SHARD_STATEMENT_RETRY_MIN,
      /* Default     */ ULN_SHARD_STATEMENT_RETRY_DEFAULT,
      /* Max         */ ULN_SHARD_STATEMENT_RETRY_MAX,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_INDOUBT_FETCH_TIMEOUT,
      /* Keyword     */ "INDOUBT_FETCH_TIMEOUT",
      /* SQL_ATTR_ID */ ALTIBASE_INDOUBT_FETCH_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ 6,
      /* Max         */ 86400,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_INDOUBT_FETCH_METHOD,
      /* Keyword     */ "INDOUBT_FETCH_METHOD",
      /* SQL_ATTR_ID */ ALTIBASE_INDOUBT_FETCH_METHOD,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 1,
      /* Max         */ 1,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
     },
     { /* ConnAttrID  */ ULN_CONN_ATTR_SHARD_COORD_FIX_CTRL_CALLBACK,
       /* Keyword     */ "SHARD_COORD_FIX_CTRL_CALLBACK",
       /* SQL_ATTR_ID */ ALTIBASE_UNUSED_06,
       /* AttrType    */ tCB,
       /* Min         */ 0,
       /* Default     */ 0,
       /* Max         */ 0,
       /* Check       */ NULL,
       /* User Conn   */ ACP_TRUE,
       /* Lib  Conn   */ ACP_FALSE
     },
     { /* ConnAttrID  */ ULN_CONN_ATTR___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD,
       /* Keyword     */ "__OPTIMIZER_PLAN_HASH_OR_SORT_METHOD",
       /* SQL_ATTR_ID */ ALTIBASE___REDUCE_PARTITION_PREPARE_MEMORY,
       /* AttrType    */ tINT,
       /* Min         */ 0,
       /* Default     */ 0,
       /* Max         */ 2,
       /* Check       */ gNULL_LIST,
       /* User Conn   */ ACP_TRUE,
       /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR___OPTIMIZER_BUCKET_COUNT_MAX,
      /* Keyword     */ "__OPTIMIZER_BUCKET_COUNT_MAX",
      /* SQL_ATTR_ID */ ALTIBASE___OPTIMIZER_BUCKET_COUNT_MAX,
      /* AttrType    */ tINT,
      /* Min         */ 1024,
      /* Default     */ 102400000,
      /* Max         */ 102400000,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR_DDL_LOCK_TIMEOUT,
      /* Keyword     */ "DDL_LOCK_TIMEOUT",
      /* SQL_ATTR_ID */ ALTIBASE_DDL_LOCK_TIMEOUT,
      /* AttrType    */ tINT,
      /* Min         */ ULN_TMOUT_MIN,
      /* Default     */ 0,
      /* Max         */ ULN_TMOUT_MAX,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
    { /* ConnAttrID  */ ULN_CONN_ATTR___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION,
      /* Keyword     */ "__OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION",
      /* SQL_ATTR_ID */ ALTIBASE___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION,
      /* AttrType    */ tINT,
      /* Min         */ 0,
      /* Default     */ 1,
      /* Max         */ 2,
      /* Check       */ gNULL_LIST,
      /* User Conn   */ ACP_TRUE,
      /* Lib  Conn   */ ACP_TRUE
    },
     { /* ConnAttrID  */ ULN_CONN_ATTR_REBUILD_SHARD_META_NUMBER,
       /* Keyword     */ "REBUILD_SHARD_META_NUMBER",
       /* SQL_ATTR_ID */ ALTIBASE_UNUSED_08,
       /* AttrType    */ tINT,
       /* Min         */ ACP_UINT64_MIN,
       /* Default     */ 0,
       /* Max         */ ACP_UINT64_MAX,
       /* Check       */ gULN_MUL,
       /* User Conn   */ ACP_FALSE,
       /* Lib  Conn   */ ACP_FALSE
     },
};

#undef tINT
#undef tDINT
#undef tSTR
#undef tUSTR
#undef tCB
#undef tPTR

/*
 * Aliases and allowd key strings
 */

const ulnAttrNameIdPair gUlnConnAttrMap_PROFILE[] =
{
    { "AutoCommit"                , ULN_CONN_ATTR_AUTOCOMMIT                },
    { "AlternateServers"          , ULN_CONN_ATTR_ALTERNATE_SERVERS         },
    { "LoadBalance"               , ULN_CONN_ATTR_LOAD_BALANCE              },
    { "ConnectTimeOut"            , ULN_CONN_ATTR_CONNECTION_TIMEOUT        },
    { "ConnectionPool"            , ULN_CONN_ATTR_CONNECTION_POOLING        },
    { "ConnectionRetryCount"      , ULN_CONN_ATTR_CONNECTION_RETRY_COUNT    },
    { "ConnectionRetryDelay"      , ULN_CONN_ATTR_CONNECTION_RETRY_DELAY    },
    { "DataSource"                , ULN_CONN_ATTR_DSN                       },
    { "DateFormat"                , ULN_CONN_ATTR_DATE_FORMAT               },
    { "DisconnectBV"              , ULN_CONN_ATTR_DISCONNECT_BEHAVIOR       },
    { "LoginTimeOut"              , ULN_CONN_ATTR_LOGIN_TIMEOUT             },
    { "NLS_USE"                   , ULN_CONN_ATTR_NLS_USE                   },
    { "ODBCVersion"               , ULN_CONN_ATTR_ODBC_VERSION              },
    { "PacketSize"                , ULN_CONN_ATTR_PACKET_SIZE               },
    { "Password"                  , ULN_CONN_ATTR_PWD                       }, /* CLI2 old style password ? */
    { "Port"                      , ULN_CONN_ATTR_PORT_NO                   }, /* CLI2 old style port Number*/
    { "QueryTimeOut"              , ULN_CONN_ATTR_QUERY_TIMEOUT             },
    { "Server"                    , ULN_CONN_ATTR_HOSTNAME                  }, /* CLI2 old style alias name*/
    { "SessionFailover"           , ULN_CONN_ATTR_SESSION_FAILOVER          },
    { "ShardInternalLocalOperation", ULN_CONN_ATTR_SHARD_INTERNAL_LOCAL_OPERATION }, 
    { "ShardLinkerType"           , ULN_CONN_ATTR_SHARD_LINKER_TYPE         }, /* PROJ-2638 shard native linker */
    { "ShardNodeName"             , ULN_CONN_ATTR_SHARD_NODE_NAME           }, /* PROJ-2638 shard native linker */
    { "ShardPin"                  , ULN_CONN_ATTR_SHARD_PIN                 }, /* PROJ-2638 shard native linker */
    { "ShardMetaNumber"           , ULN_CONN_ATTR_SHARD_META_NUMBER         }, /* BUG-46090 Meta Node SMN 전파 */
    { "ShardClient"               , ULN_CONN_ATTR_SHARD_CLIENT              }, /* BUG-45707 */
    { "ShardSessionType"          , ULN_CONN_ATTR_SHARD_SESSION_TYPE        }, /* BUG-45707 */
    { "TimeOut"                   , ULN_CONN_ATTR_LOGIN_TIMEOUT             }, /* BUG-40769 */
    { "TraceLog"                  , ULN_CONN_ATTR_TRACELOG                  },
    { "TxIsolation"               , ULN_CONN_ATTR_TXN_ISOLATION             },
    { "URL"                       , ULN_CONN_ATTR_URL                       },
    { "User"                      , ULN_CONN_ATTR_UID                       },
    { "UserName"                  , ULN_CONN_ATTR_UID                       },
    { "StackSize"                 , ULN_CONN_ATTR_STACK_SIZE                },
    { "OptimizerMode"             , ULN_CONN_ATTR_OPTIMIZER_MODE            },
    { "LongDataCompat"            , ULN_CONN_ATTR_LONGDATA_COMPAT           },
    { "NLS_NCHAR_LITERAL_REPLACE" , ULN_CONN_ATTR_NLS_NCHAR_LITERAL_REPLACE },
    { "PREFER_IPV6"               , ULN_CONN_ATTR_PREFER_IPV6               },
    { "TIME_ZONE"                 , ULN_CONN_ATTR_TIME_ZONE                 }, /* PROJ-2209 DBTIMEZONE */
    { "ODBC_COMPATIBILITY"        , ULN_CONN_ATTR_ODBC_COMPATIBILITY        }, /* BUG-36548 */
    { "LOB_CACHE_THRESHOLD"       , ULN_CONN_ATTR_LOB_CACHE_THRESHOLD       }, /* BUG-36966 */
    { "Database"                  , ULN_CONN_ATTR_CURRENT_CATALOG           }, /* BUG-44132 */
    { NULL                        , ULN_CONN_ATTR_MAX                       }
};

// bug-20706 아래 테이블은 반드시 아스키 코드 순서로 되어야 한다.
// Connection Attribute 는 case in-sensitive하게 처리한다.
// 따라서 여기는 편의상 소문자로 기술해 주세요.
const struct ulnAttrNameIdPair gUlnConnAttrMap_KEYWORD[] =
{
    { "access_mode"               , ULN_CONN_ATTR_ACCESS_MODE               },
    { "alternate_servers"         , ULN_CONN_ATTR_ALTERNATE_SERVERS         }, /* PROJ-1645 */
    { "alternateservers"          , ULN_CONN_ATTR_ALTERNATE_SERVERS         }, /* PROJ-1645 */
    { "app_info"                  , ULN_CONN_ATTR_APP_INFO                  },
    { "arithetic_operation_mode"  , ULN_CONN_ATTR_ARITHMETIC_OPERATION_MODE },
    { "async_enable"              , ULN_CONN_ATTR_ASYNC_ENABLE              },
    { "auto_remote_exec"          , ULN_CONN_ATTR_AUTO_REMOTE_EXEC          },
    { "autocommit"                , ULN_CONN_ATTR_AUTOCOMMIT                },    
    { "commit_write_wait_mode"    , ULN_CONN_ATTR_COMMIT_WRITE_WAIT_MODE    },
    { "connection_pooling"        , ULN_CONN_ATTR_CONNECTION_POOLING        },
    { "connection_timeout"        , ULN_CONN_ATTR_CONNECTION_TIMEOUT        },
    { "connectionretrycount"      , ULN_CONN_ATTR_CONNECTION_RETRY_COUNT    },
    { "connectionretrydelay"      , ULN_CONN_ATTR_CONNECTION_RETRY_DELAY    },
    { "conntype"                  , ULN_CONN_ATTR_CONNTYPE                  },
    { "database"                  , ULN_CONN_ATTR_CURRENT_CATALOG           }, /* BUG-44132 */
    { "datasourcename"            , ULN_CONN_ATTR_DSN                       },
    { "date_fmt"                  , ULN_CONN_ATTR_DATE_FORMAT               },
    { "date_format"               , ULN_CONN_ATTR_DATE_FORMAT               },
    { "dateformat"                , ULN_CONN_ATTR_DATE_FORMAT               },
    { "dblink_remote_statement_autocommit" , ULN_CONN_ATTR_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT },
    { "dbname"                    , ULN_CONN_ATTR_CURRENT_CATALOG           }, /* BUG-44132 */
    { "ddl_lock_timeout"          , ULN_CONN_ATTR_DDL_LOCK_TIMEOUT          },
    { "ddl_timeout"               , ULN_CONN_ATTR_DDL_TIMEOUT               }, /* BUG-32885 */

    { "default_date_format"       , ULN_CONN_ATTR_DATE_FORMAT               },
    { "defer_prepares"            , ULN_CONN_ATTR_DEFERRED_PREPARE          }, /* PROJ-1891 */
    { "disconnect_behavior"       , ULN_CONN_ATTR_DISCONNECT_BEHAVIOR       },
    { "dsn"                       , ULN_CONN_ATTR_DSN                       },
    { "explain_plan"              , ULN_CONN_ATTR_EXPLAIN_PLAN              },
    { "fetch_timeout"             , ULN_CONN_ATTR_FETCH_TIMEOUT             },
    { "force_unlock"              , ULN_CONN_ATTR_FORCE_UNLOCK              }, /* BUG-36729 */
    { "global_ddl"                , ULN_CONN_ATTR_GLOBAL_DDL                }, /* PROJ-2736 */
    { "global_transaction_level"  , ULN_CONN_ATTR_GLOBAL_TRANSACTION_LEVEL  }, /* BUG-47257 */
    { "gpkiconfdir"               , ULN_CONN_ATTR_GPKICONFDIR               },
    { "gpkiworkdir"               , ULN_CONN_ATTR_GPKIWORKDIR               },
    { "header_display_mode"       , ULN_CONN_ATTR_HEADER_DISPLAY_MODE       },
    { "host"                      , ULN_CONN_ATTR_HOSTNAME                  },
    { "hostname"                  , ULN_CONN_ATTR_HOSTNAME                  },
    { "ib_conchkspin"             , ULN_CONN_ATTR_IB_CONCHKSPIN             }, /* PROJ-2681 */
    { "ib_latency"                , ULN_CONN_ATTR_IB_LATENCY                }, /* PROJ-2681 */
    { "idle_timeout"              , ULN_CONN_ATTR_IDLE_TIMEOUT              },
    { "idn_lang"                  , ULN_CONN_ATTR_NLS_USE                   },
    { "indoubt_fetch_method"      , ULN_CONN_ATTR_INDOUBT_FETCH_METHOD     },
    { "indoubt_fetch_timeout"     , ULN_CONN_ATTR_INDOUBT_FETCH_TIMEOUT     },
    { "ipc_filepath"              , ULN_CONN_ATTR_IPC_FILEPATH              },
    { "ipcdafilepath"             , ULN_CONN_ATTR_IPCDA_FILEPATH            },
    { "ipcfilepath"               , ULN_CONN_ATTR_IPC_FILEPATH              },
    { "load_balance"              , ULN_CONN_ATTR_LOAD_BALANCE              },
    { "loadbalance"               , ULN_CONN_ATTR_LOAD_BALANCE              },
    { "lob_cache_threshold"       , ULN_CONN_ATTR_LOB_CACHE_THRESHOLD       }, /* PROJ-2047 */
    { "login_timeout"             , ULN_CONN_ATTR_LOGIN_TIMEOUT             },
    { "longdatacompat"            , ULN_CONN_ATTR_LONGDATA_COMPAT           },
    { "max_statements_per_session", ULN_CONN_ATTR_MAX_STATEMENTS_PER_SESSION}, /* BUG-31144 */
    { "metadata_id"               , ULN_CONN_ATTR_METADATA_ID               },
    { "nls_characterset"          , ULN_CONN_ATTR_NLS_CHARACTERSET          },
    { "nls_currency"              , ULN_CONN_ATTR_NLS_CURRENCY              },
    { "nls_iso_currency"          , ULN_CONN_ATTR_NLS_ISO_CURRENCY          },
    { "nls_nchar_characterset"    , ULN_CONN_ATTR_NLS_NCHAR_CHARACTERSET    },
    { "nls_nchar_conv_excp"       , ULN_CONN_ATTR_NLS_NCHAR_CONV_EXCP       },
    { "nls_nchar_literal_replace" , ULN_CONN_ATTR_NLS_NCHAR_LITERAL_REPLACE },
    { "nls_numeric_characters"    , ULN_CONN_ATTR_NLS_NUMERIC_CHARACTERS    },
    { "nls_territory"             , ULN_CONN_ATTR_NLS_TERRITORY             },
    { "nls_use"                   , ULN_CONN_ATTR_NLS_USE                   },
    { "normalform_maximum"        , ULN_CONN_ATTR_NORMALFORM_MAXIMUM        },    
    { "odbc_compatibility"        , ULN_CONN_ATTR_ODBC_COMPATIBILITY        }, /* BUG-36548 */
    { "odbc_version"              , ULN_CONN_ATTR_ODBC_VERSION              },
    { "optimizer_auto_stats"      , ULN_CONN_ATTR_OPTIMIZER_AUTO_STATS      },
    { "optimizer_disk_index_cost_adj"      , ULN_CONN_ATTR_OPTIMIZER_DISK_INDEX_COST_ADJ   },
    { "optimizer_memory_index_cost_adj"    , ULN_CONN_ATTR_OPTIMIZER_MEMORY_INDEX_COST_ADJ },
    { "optimizer_mode"            , ULN_CONN_ATTR_OPTIMIZER_MODE            },
    { "optimizer_performance_view"         , ULN_CONN_ATTR_OPTIMIZER_PERFORMANCE_VIEW        },
    { "packet_size"               , ULN_CONN_ATTR_PACKET_SIZE               },
    { "parallel_dml_mode"         , ULN_CONN_ATTR_PARALLEL_DML_MODE         },
    { "password"                  , ULN_CONN_ATTR_PWD                       },
    { "pdo_defer_protocols"       , ULN_CONN_ATTR_PDO_DEFER_PROTOCOLS       }, /* BUG-45286 */
    { "port"                      , ULN_CONN_ATTR_PORT_NO                   },
    { "port_no"                   , ULN_CONN_ATTR_PORT_NO                   },
    { "prefer_ipv6"               , ULN_CONN_ATTR_PREFER_IPV6               },
    { "privilege"                 , ULN_CONN_ATTR_PRIVILEGE                 }, /* BUG-19279 */
    { "pwd"                       , ULN_CONN_ATTR_PWD                       },
    { "query_rewrite_enable"      , ULN_CONN_ATTR_QUERY_REWRITE_ENABLE      },
    { "query_timeout"             , ULN_CONN_ATTR_QUERY_TIMEOUT             },    
    { "recyclebin_enable"         , ULN_CONN_ATTR_RECYCLEBIN_ENABLE         },
    { "replication_ddl_sync"      , ULN_CONN_ATTR_REPLICATION_DDL_SYNC      },
    { "replication_ddl_sync_timeout"       , ULN_CONN_ATTR_REPLICATION_DDL_SYNC_TIMEOUT      },
    { "result_cache_enable"       , ULN_CONN_ATTR_RESULT_CACHE_ENABLE       },
    { "serial_execute_mode"       , ULN_CONN_ATTR_SERIAL_EXECUTE_MODE       },
    { "server"                    , ULN_CONN_ATTR_HOSTNAME                  }, /* BUG-20809 4.3.9 style*/
    { "servername"                , ULN_CONN_ATTR_HOSTNAME                  }, /* BUG-20809 4.3.9 style*/
    { "session_failover"          , ULN_CONN_ATTR_SESSION_FAILOVER          },
    { "sessionfailover"           , ULN_CONN_ATTR_SESSION_FAILOVER          },
    { "shard_client"              , ULN_CONN_ATTR_SHARD_CLIENT              }, /* BUG-45707 */
    { "shard_conntype"            , ULN_CONN_ATTR_SHARD_CONNTYPE            }, /* BUG-45998 */
    { "shard_internal_local_operation", ULN_CONN_ATTR_SHARD_INTERNAL_LOCAL_OPERATION }, 
    { "shard_linker_type"         , ULN_CONN_ATTR_SHARD_LINKER_TYPE         }, /* PROJ-2638 shard native linker */
    { "shard_meta_number"         , ULN_CONN_ATTR_SHARD_META_NUMBER         }, /* BUG-46090 Meta Node SMN 전파 */
    { "shard_node_name"           , ULN_CONN_ATTR_SHARD_NODE_NAME           }, /* PROJ-2638 shard native linker */
    { "shard_pin"                 , ULN_CONN_ATTR_SHARD_PIN                 }, /* PROJ-2660 shard native linker */
    { "shard_session_type"        , ULN_CONN_ATTR_SHARD_SESSION_TYPE        }, /* BUG-45707 */
    { "shard_statement_retry"     , ULN_CONN_ATTR_SHARD_STATEMENT_RETRY     },
    { "sock_bind_addr"            , ULN_CONN_ATTR_SOCK_BIND_ADDR            }, /* BUG-44271 */
    { "sock_rcvbuf_block_ratio"   , ULN_CONN_ATTR_SOCK_RCVBUF_BLOCK_RATIO   }, /* PROJ-2625 */
    { "ssl_ca"                    , ULN_CONN_ATTR_SSL_CA                    }, /* PROJ-2474 SSL/TLS */
    { "ssl_capath"                , ULN_CONN_ATTR_SSL_CAPATH                }, /* PROJ-2474 SSL/TLS */
    { "ssl_cert"                  , ULN_CONN_ATTR_SSL_CERT                  }, /* PROJ-2474 SSL/TLS */
    { "ssl_cipher"                , ULN_CONN_ATTR_SSL_CIPHER                }, /* PROJ-2474 SSL/TLS */
    { "ssl_key"                   , ULN_CONN_ATTR_SSL_KEY                   }, /* PROJ-2474 SSL/TLS */
    { "ssl_verify"                , ULN_CONN_ATTR_SSL_VERIFY                }, /* PROJ-2474 SSL/TLS */
    { "st_object_buffer_size"     , ULN_CONN_ATTR_ST_OBJECT_BUFFER_SIZE     },
    { "stack_size"                , ULN_CONN_ATTR_STACK_SIZE                },
    { "time_zone"                 , ULN_CONN_ATTR_TIME_ZONE                 }, /* PROJ-2209 DBTIMEZONE */
    { "timeout"                   , ULN_CONN_ATTR_LOGIN_TIMEOUT             }, /* BUG-40769 */
    { "top_result_cache_mode"     , ULN_CONN_ATTR_TOP_RESULT_CACHE_MODE     },
    { "tracelog"                  , ULN_CONN_ATTR_TRACELOG                  },
    { "transactional_ddl"         , ULN_CONN_ATTR_TRANSACTIONAL_DDL         }, /* PROJ-2735 */
    { "trclog_detail_information" , ULN_CONN_ATTR_TRCLOG_DETAIL_INFORMATION },
    { "trclog_detail_predicate"   , ULN_CONN_ATTR_TRCLOG_DETAIL_PREDICATE   },  
    { "trclog_detail_shard"       , ULN_CONN_ATTR_TRCLOG_DETAIL_SHARD       },
    { "trx_update_max_logsize"    , ULN_CONN_ATTR_TRX_UPDATE_MAX_LOGSIZE    },
    { "txn_isolation"             , ULN_CONN_ATTR_TXN_ISOLATION             },
    { "uid"                       , ULN_CONN_ATTR_UID                       },
    { "unixdomain_filepath"       , ULN_CONN_ATTR_UNIXDOMAIN_FILEPATH       },
    { "unixdomainfilepath"        , ULN_CONN_ATTR_UNIXDOMAIN_FILEPATH       },
    { "uri"                       , ULN_CONN_ATTR_URL                       },
    { "url"                       , ULN_CONN_ATTR_URL                       },
    { "user"                      , ULN_CONN_ATTR_UID                       },
    { "useraid"                   , ULN_CONN_ATTR_USERAID                   },
    { "usercert"                  , ULN_CONN_ATTR_USERCERT                  },
    { "userid"                    , ULN_CONN_ATTR_UID                       },
    { "userkey"                   , ULN_CONN_ATTR_USERKEY                   },
    { "username"                  , ULN_CONN_ATTR_UID                       },
    { "userpasswd"                , ULN_CONN_ATTR_USERPASSWD                },
    { "utrans_timeout"            , ULN_CONN_ATTR_UTRANS_TIMEOUT            },
    { "xa_log_dir"                , ULN_CONN_ATTR_XA_LOG_DIR                },
    { "xa_name"                   , ULN_CONN_ATTR_XA_NAME                   },
    { NULL                        , ULN_CONN_ATTR_MAX                       }    
};

ulnConnAttrID ulnGetConnAttrIDfromKEYWORD(const acp_char_t *aName, acp_sint32_t aNameLen)
{
    ulnConnAttrID sConnAttrID = ULN_CONN_ATTR_MAX;
    acp_sint32_t  sRC;
    acp_uint32_t  sLow;
    acp_uint32_t  sHigh;
    acp_uint32_t  sMid;

    sHigh = (ACI_SIZEOF(gUlnConnAttrMap_KEYWORD) / ACI_SIZEOF(ulnAttrNameIdPair)) - 1;

    for ( sLow = 0, sMid = sHigh >> 1;
          sLow <= sHigh;
          sMid = (sHigh + sLow) >> 1 )
    {
        ACI_TEST( gUlnConnAttrMap_KEYWORD[sMid].mKey == NULL );

        sRC = acpCStrCaseCmp(gUlnConnAttrMap_KEYWORD[sMid].mKey, aName, aNameLen);
        if (sRC < 0)
        {
            sLow = sMid + 1;
        }
        else if (sRC > 0 || gUlnConnAttrMap_KEYWORD[sMid].mKey[aNameLen] != '\0')
        {
            ACI_TEST( sMid == 0 )

            sHigh = sMid - 1;
        }
        else
        {
            sConnAttrID = gUlnConnAttrMap_KEYWORD[sMid].mConnAttrID;
            break;
        }
    }

    return sConnAttrID;

    ACI_EXCEPTION_END;

    return ULN_CONN_ATTR_MAX;
}

ulnConnAttrID ulnGetConnAttrIDfromSQL_ATTR_ID(acp_sint32_t aSQL_ATTR_ID)
{
    acp_uint32_t i;

    /*
     * switch - case 를 쓰는게 더 빠를까? 그냥 돌자.
     */

    for (i = 0; i < ULN_CONN_ATTR_MAX; i++)
    {
        if (gUlnConnAttrTable[i].mSQL_ATTR_ID == aSQL_ATTR_ID)
        {
            /*
             * Bingo! We got it.
             */

            return gUlnConnAttrTable[i].mConnAttrID;
        }
    }

    return ULN_CONN_ATTR_MAX;
}

/*
 * BUG-36256 Improve property's communication
 */
void ulnConnAttrArrInit(ulnConnAttrArr *aUnsupportedConnAttr)
{
    aUnsupportedConnAttr->mArrId    = NULL;
    aUnsupportedConnAttr->mArrIndex = 0;
    aUnsupportedConnAttr->mArrSize  = 0;

    return;
}

void ulnConnAttrArrReInit(ulnConnAttrArr *aUnsupportedConnAttr)
{
    aUnsupportedConnAttr->mArrIndex         = 0;

    return;
}

void ulnConnAttrArrFinal(ulnConnAttrArr *aUnsupportedConnAttr)
{
    if (aUnsupportedConnAttr->mArrId != NULL)
    {
        acpMemFree(aUnsupportedConnAttr->mArrId);
        aUnsupportedConnAttr->mArrId = NULL;
    }
    else
    {
        /* Nothing */
    }

    aUnsupportedConnAttr->mArrIndex = 0;
    aUnsupportedConnAttr->mArrSize  = 0;

    return;
}

/**
 * ulnConnAttrArrAddId
 *
 * aId를 ulnConnAttrArr에 저장한다.
 */
ACI_RC ulnConnAttrArrAddId(ulnConnAttrArr *aUnsupportedConnAttr,
                           ulnConnAttrID   aId)
{
    acp_rc_t sRC = ACP_RC_MAX;

    if (aUnsupportedConnAttr->mArrId == NULL)
    {
        sRC = acpMemCalloc((void **)&aUnsupportedConnAttr->mArrId,
                           ACI_SIZEOF(ulnConnAttrID),
                           8);
        ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));

        aUnsupportedConnAttr->mArrSize = ACI_SIZEOF(ulnConnAttrID) * 8;
    }
    else
    {
        /* 공간이 부족하면 2배로 늘리자 */
        if (aUnsupportedConnAttr->mArrIndex *
            (acp_sint32_t)ACI_SIZEOF(ulnConnAttrID) == aUnsupportedConnAttr->mArrSize)
        {
            aUnsupportedConnAttr->mArrSize *= 2;

            sRC = acpMemRealloc((void **)&aUnsupportedConnAttr->mArrId,
                                aUnsupportedConnAttr->mArrSize);
            ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));
        }
        else
        {
            /* Nothing */
        }
    }

    if (ulnConnAttrArrHasId(aUnsupportedConnAttr, aId) != ACI_SUCCESS)
    {
        aUnsupportedConnAttr->mArrId[aUnsupportedConnAttr->mArrIndex] = aId;
        aUnsupportedConnAttr->mArrIndex += 1;
    }
    else
    {
        /* Nothing */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ulnConnAttrArrFinal(aUnsupportedConnAttr);

    return ACI_FAILURE;
}

/**
 * ulnConnAttrArrHasId
 *
 * ulnConnAttrArr에 aId가 저장되어 있는지 확인한다.
 */
ACI_RC ulnConnAttrArrHasId(ulnConnAttrArr *aUnsupportedConnAttr,
                           ulnConnAttrID   aId)
{
    acp_sint32_t i = 0;
    ACI_RC sRC = ACI_FAILURE;
    
    for (i = 0; i < aUnsupportedConnAttr->mArrIndex; i++)
    {
        if (aUnsupportedConnAttr->mArrId[i] == aId)
        {
            sRC = ACI_SUCCESS;
            break;
        }
        else
        {
            /* Nothing */
        }
    }

    return sRC;
}

/**
 * ulnGetConnAttrKEYWORDfromConnAttrId
 *
 * ConnAttrId를 이용해 Keyword를 알아온다.
 */
acp_char_t *ulnGetConnAttrKEYWORDfromConnAttrId(ulnConnAttrID aConnAttrId)
{
    acp_uint32_t i = 0;
    acp_char_t  *sKey = "UNKNOWN";

    for (i = 0; i < ULN_CONN_ATTR_MAX; i++)
    {
        if (gUlnConnAttrTable[i].mConnAttrID == aConnAttrId)
        {
            sKey = (acp_char_t *)gUlnConnAttrTable[i].mKey;
            break;
        }
        else
        {
            /* Nothing */
        }
    }

    return sKey;
}
