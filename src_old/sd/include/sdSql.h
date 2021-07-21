/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id$ sdlSql.h
 **********************************************************************/

#ifndef _O_SDSQL_H_
# define _O_SDSQL_H_ 1

#include <acpTypes.h>
/* For LoadLibrary : dlopen's option flag.*/
#if !defined(RTLD_GLOBAL)
#  define RTLD_GLOBAL 4
#endif /* !RTLD_GLOBAL */

#ifndef SIZEOF_LONG
# if defined(__alpha) || defined(__sparcv9) || defined(__amd64) || defined(__LP64__) || defined(_LP64) || defined(__64BIT__)
# define SIZEOF_LONG        8
#else
# define SIZEOF_LONG        4
#endif
#endif

typedef UChar     SQLCHAR;
typedef SShort    SQLSMALLINT;
typedef UShort    SQLUSMALLINT;
typedef void *    SQLPOINTER;

#define SQL_C_DEFAULT   99

#if (SIZEOF_LONG == 8)
typedef SInt            SQLINTEGER;
typedef UInt            SQLUINTEGER;
typedef SLong           SQLBIGINT;  /* PROJ-2733-DistTxInfo */
typedef ULong           SQLUBIGINT;
#define SQLLEN          SQLINTEGER
#define SQLULEN         SQLUINTEGER
#define SQLSETPOSIROW   SQLUSMALLINT
typedef SQLULEN         SQLROWCOUNT;
typedef SQLULEN         SQLROWSETSIZE;
typedef SQLULEN         SQLTRANSID;
typedef SQLLEN          SQLROWOFFSET;
#else
typedef SInt            SQLINTEGER;
typedef UInt            SQLUINTEGER;
typedef SLong           SQLBIGINT;  /* PROJ-2733-DistTxInfo */
typedef ULong           SQLUBIGINT;
#define SQLLEN          SQLINTEGER
#define SQLULEN         SQLUINTEGER
#define SQLSETPOSIROW   SQLUSMALLINT
typedef SQLULEN         SQLROWCOUNT;
typedef SQLULEN         SQLROWSETSIZE;
typedef SQLULEN         SQLTRANSID;
typedef SQLLEN          SQLROWOFFSET;
#endif

#define SQL_SIGNED_OFFSET       (-20)
#define SQL_UNSIGNED_OFFSET     (-22)

/* SQL data type codes */
#define SQL_UNKNOWN_TYPE    0
#define SQL_CHAR            1
#define SQL_NUMERIC         2
#define SQL_DECIMAL         3
#define SQL_INTEGER         4
#define SQL_SMALLINT        5
#define SQL_FLOAT           6
#define SQL_REAL            7
#define SQL_DOUBLE          8
#define SQL_BIGINT          (-5)
#define SQL_VARCHAR        12

/* C datatype to SQL datatype mapping      SQL types
                                           ------------------- */
#define SQL_C_CHAR    SQL_CHAR             /* CHAR, VARCHAR, DECIMAL, NUMERIC */
#define SQL_C_LONG    SQL_INTEGER          /* INTEGER                      */
#define SQL_C_SHORT   SQL_SMALLINT         /* SMALLINT                     */
#define SQL_C_FLOAT   SQL_REAL             /* REAL                         */
#define SQL_C_DOUBLE  SQL_DOUBLE           /* FLOAT, DOUBLE                */
#define SQL_C_SBIGINT   (SQL_BIGINT+SQL_SIGNED_OFFSET)     /* SIGNED BIGINT */
#define SQL_C_UBIGINT   (SQL_BIGINT+SQL_UNSIGNED_OFFSET)   /* UNSIGNED BIGINT */

#ifndef SQL_DATA_AT_EXEC
# define SQL_DATA_AT_EXEC         (-2)
#endif //SQL_DATA_AT_EXEC
#ifndef SQL_STILL_EXECUTING
# define SQL_STILL_EXECUTING      2
#endif //SQL_STILL_EXECUTING
#ifndef SQL_NEED_DATA
# define SQL_NEED_DATA            99
#endif //SQL_NEED_DATA

/* Options for SQLDriverConnect */
#define SQL_DRIVER_NOPROMPT             0
#define SQL_DRIVER_COMPLETE             1
#define SQL_DRIVER_PROMPT               2
#define SQL_DRIVER_COMPLETE_REQUIRED    3

#define SQL_FETCH_NORMAL                0
#define SQL_FETCH_MTDATA                1

#define SQL_SQLSTATE_SIZE               ( 5 )       /* = SQL_SQLSTATE_SIZE of sqlext.h */


/* UL Error code defines */
#define SD_ALTIBASE_FAILOVER_SUCCESS    ( 0x51190 ) /* = ALTIBASE_FAILOVER_SUCCESS */


typedef void * LIBHANDLE;
typedef void * SQLHANDLE;
typedef SQLSMALLINT SQLRETURN;
typedef SQLHANDLE   SQLHENV;
typedef SQLHANDLE   SQLHDBC;
typedef SQLHANDLE   SQLHSTMT;
typedef SQLHANDLE   SQLHDESC;
typedef void*       SQLHWND;

typedef SQLRETURN (*ODBCDriverConnect)(SQLHDBC,
                                       SQLHWND,
                                       SQLCHAR *,
                                       SQLSMALLINT,
                                       SQLCHAR *,
                                       SQLSMALLINT,
                                       SQLSMALLINT *,
                                       SQLUSMALLINT );      // 01

typedef SQLRETURN (*ODBCAllocHandle)(SQLSMALLINT,
                                     SQLHANDLE,
                                     SQLHANDLE *);          // 02

typedef SQLRETURN (*ODBCFreeHandle)(SQLSMALLINT,
                                    SQLHANDLE);             // 03
typedef SQLRETURN (*ODBCExecDirect)(SQLHSTMT,
                                    SQLCHAR *,
                                    SQLINTEGER);            // 04
typedef SQLRETURN (*ODBCPrepare)(SQLHSTMT,
                                 SQLCHAR *,
                                 SQLINTEGER);               // 05
typedef SQLRETURN (*ODBCExecute)(SQLHSTMT);                 // 06
typedef SQLRETURN (*ODBCFetch)(SQLHSTMT);                   // 07
typedef SQLRETURN (*ODBCDisconnect)(SQLHDBC);               // 08
typedef SQLRETURN (*ODBCFreeConnect)(SQLHDBC);              // 09
typedef SQLRETURN (*ODBCFreeStmt)(SQLHSTMT,
                                  SQLUSMALLINT);            // 10
typedef SQLRETURN (*ODBCAllocEnv)(SQLHENV *);               // 11
typedef SQLRETURN (*ODBCAllocConnect)(SQLHENV,
                                      SQLHDBC *);           // 12
typedef SQLRETURN (*ODBCExecuteForMtDataRows)(SQLHSTMT,
                                              SQLCHAR *,
                                              SQLUINTEGER,
                                              SQLPOINTER *,
                                              SQLPOINTER *,
                                              SQLUSMALLINT);// 13
typedef SQLHENV   (*ODBCGetEnvHandle)(SQLHDBC);             // 14
typedef SQLRETURN (*ODBCGetDiagRec)(SQLSMALLINT,
                                     SQLHANDLE,
                                     SQLSMALLINT,
                                     SQLCHAR *,
                                     SQLINTEGER *,
                                     SQLCHAR *,
                                     SQLSMALLINT,
                                     SQLSMALLINT *);        // 15
typedef SQLRETURN  (*ODBCSetConnectOption)(SQLHDBC,
                                           SQLUSMALLINT,
                                           SQLULEN);        // 16
typedef SQLRETURN  (*ODBCGetConnectOption)(SQLHDBC,
                                           SQLUSMALLINT,
                                           SQLPOINTER);     // 17
typedef SQLRETURN (*ODBCEndTran)(SQLSMALLINT,
                                 SQLHANDLE,
                                 SQLSMALLINT);              // 18
typedef SQLRETURN (*ODBCTransact)(SQLHENV,
                                  SQLHDBC,
                                  SQLUSMALLINT);            // 19
typedef SQLRETURN (*ODBCRowCount)(SQLHSTMT,
                                  SQLLEN *);                // 20
typedef SQLRETURN (*ODBCGetPlan)(SQLHSTMT, SQLCHAR **);     // 21
typedef SQLRETURN (*ODBCDescribeCol)(SQLHSTMT,
                                      SQLUSMALLINT,
                                      SQLCHAR *,
                                      SQLSMALLINT,
                                      SQLSMALLINT *,
                                      SQLSMALLINT *,
                                      SQLULEN *,
                                      SQLSMALLINT *,
                                      SQLSMALLINT *);       // 22
typedef SQLRETURN (*ODBCDescribeColEx)(SQLHSTMT,
                                       SQLUSMALLINT,
                                       SQLCHAR *,
                                       SQLINTEGER,
                                       SQLUINTEGER *,
                                       SQLUINTEGER *,
                                       SQLINTEGER *,
                                       SQLSMALLINT *,
                                       SQLSMALLINT *);      // 23
typedef SQLRETURN (*ODBCBindParameter)(SQLHSTMT,
                                       SQLUSMALLINT,
                                       SQLSMALLINT,
                                       SQLSMALLINT,
                                       SQLSMALLINT,
                                       SQLULEN,
                                       SQLSMALLINT,
                                       SQLPOINTER,
                                       SQLLEN,
                                       SQLLEN *);           // 24
typedef SQLRETURN (*ODBCGetConnectAttr)(SQLHDBC,
                                        SQLINTEGER,
                                        SQLPOINTER,
                                        SQLINTEGER,
                                        SQLINTEGER *);      // 25
typedef SQLRETURN (*ODBCSetConnectAttr)(SQLHDBC,
                                        SQLINTEGER,
                                        SQLPOINTER,
                                        SQLINTEGER);        // 26
typedef SQLRETURN (*ODBCBindCol)(SQLHSTMT,
                                 SQLSMALLINT,
                                 SQLSMALLINT,
                                 SQLPOINTER,
                                 SQLLEN,
                                 SQLLEN *);        // 27
typedef void      (*ODBCGetDbcShardTargetDataNodeName)(SQLHDBC,
                                                       SQLCHAR *,
                                                       SQLINTEGER); // 31
typedef void      (*ODBCGetStmtShardTargetDataNodeName)(SQLHSTMT,
                                                        SQLCHAR *,
                                                        SQLINTEGER);// 32
typedef void      (*ODBCSetDbcShardTargetDataNodeName)(SQLHDBC,
                                                       SQLCHAR *);  // 33
typedef void      (*ODBCSetStmtShardTargetDataNodeName)(SQLHSTMT,
                                                        SQLCHAR *); // 34
typedef void      (*ODBCGetDbcLinkInfo)(SQLHDBC,
                                        SQLCHAR*,
                                        SQLINTEGER,
                                        SQLINTEGER);        // 35
typedef SQLRETURN (*ODBCGetParameterCount)(SQLHSTMT,
                                           SQLUSMALLINT*);  // 36
typedef SQLRETURN (*ODBCDisconnectLocal)(SQLHDBC);          // 37
typedef void      (*ODBCSetShardPin)(SQLHDBC, ULong);       // 38

typedef SQLRETURN (*ODBCEndPendingTran)(SQLHDBC,
                                        SQLUINTEGER,
                                        SQLPOINTER*,
                                        SQLSMALLINT);       // 39 

typedef SQLRETURN (*ODBCPrepareAddCallback)(SQLUINTEGER,
                                            SQLHSTMT,
                                            SQLCHAR*,
                                            SQLINTEGER,
                                            SQLPOINTER**);  // 40
typedef SQLRETURN (*ODBCExecuteForMtDataRowsAddCallback)(SQLUINTEGER,
                                                         SQLHSTMT,
                                                         SQLCHAR *,
                                                         SQLUINTEGER,
                                                         SQLPOINTER *,
                                                         SQLPOINTER *,
                                                         SQLUSMALLINT,
                                                         SQLPOINTER**); // 41
typedef SQLRETURN (*ODBCExecuteForMtDataAddCallback)(SQLUINTEGER,
                                                     SQLHSTMT,
                                                     SQLPOINTER**); // 42
typedef SQLRETURN (*ODBCPrepareTranAddCallback)(SQLUINTEGER,
                                                SQLHDBC,
                                                SQLUINTEGER,
                                                SQLPOINTER*,
                                                SQLCHAR*,
                                                SQLPOINTER**);  // 43
typedef SQLRETURN (*ODBCEndTranAddCallback)(SQLUINTEGER,
                                            SQLHDBC,
                                            SQLSMALLINT,
                                            SQLPOINTER**);  // 44

typedef void      (*ODBCDoCallback)(SQLPOINTER*);           // 45
typedef SQLRETURN (*ODBCGetResultCallback)(SQLUINTEGER,
                                           SQLPOINTER*,
                                           acp_uint8_t);    // 46
typedef void      (*ODBCRemoveCallback)(SQLPOINTER*);       // 47

typedef void      (*ODBCSetShardMetaNumber)(SQLHDBC, ULong);// 48

typedef SQLRETURN (*ODBCGetNeedFailover)( SQLSMALLINT,
                                          SQLHANDLE,
                                          SQLINTEGER *);    // 50

typedef SQLRETURN (*ODBCReconnect)( SQLSMALLINT,
                                    SQLHANDLE );            // 51

typedef SQLRETURN (*ODBCSetSavepoint)( SQLHDBC,
                                       const SQLCHAR *,
                                       SQLINTEGER );   // 53

typedef SQLRETURN (*ODBCRollbackToSavepoint)( SQLHDBC,
                                              const SQLCHAR *,
                                              SQLINTEGER );   // 54

typedef SQLRETURN (*ODBCShardStmtPartialRollback)( SQLHDBC );    // 55

typedef SQLRETURN (*ODBCEndPendingTranAddCallback)( SQLUINTEGER,
                                                    SQLHDBC,
                                                    SQLUINTEGER,
                                                    SQLPOINTER*,
                                                    SQLSMALLINT,
                                                    SQLPOINTER** );  // 56

typedef void (*ODBCSetFailoverSuspend)( SQLHDBC    aConnectionHandle,
                                        SQLINTEGER aSuspendOnOff ); // 57

/* PROJ-2728 Sharding LOB */
typedef SQLRETURN (*ODBCGetLobForSd)(SQLSMALLINT  aHandleType,
                                     SQLHANDLE    aHandle,
                                     SQLSMALLINT  aLocatorCType,
                                     SQLUBIGINT   aLocator,
                                     SQLUINTEGER  aStartOffset,
                                     SQLUINTEGER  aSizeToGet,
                                     SQLSMALLINT  aTargetCType,
                                     SQLPOINTER   aBufferToStoreData,
                                     SQLUINTEGER  aSizeBuffer,
                                     SQLUINTEGER *aSizeReadPtr);    // 58

typedef SQLRETURN (*ODBCPutLobForSd)(SQLSMALLINT  aHandleType,
                                     SQLHANDLE    aHandle,
                                     SQLSMALLINT  aLocatorCType,
                                     SQLUBIGINT   aLocator,
                                     SQLUINTEGER  aStartOffset,
                                     SQLUINTEGER  aSizeToBeUpdated,
                                     SQLSMALLINT  aSourceCType,
                                     SQLPOINTER   aDataToPut,
                                     SQLUINTEGER  aSizeDataToPut);  // 59

typedef SQLRETURN (*ODBCGetLobLengthForSd)(SQLSMALLINT   aHandleType,
                                           SQLHANDLE     aHandle,
                                           SQLUBIGINT    aLocator,
                                           SQLSMALLINT   aLocatorCType,
                                           SQLUINTEGER  *aLobLengthPtr,
                                           SQLUSMALLINT *aIsNullLob); // 60

typedef SQLRETURN (*ODBCFreeLobForSd)(SQLSMALLINT  aHandleType,
                                      SQLHANDLE    aHandle,
                                      SQLUBIGINT   aLocator);       // 61

typedef SQLRETURN (*ODBCTrimLobForSd)(SQLSMALLINT aHandleType,
                                      SQLHANDLE   aHandle,
                                      SQLSMALLINT aLocatorCType,
                                      SQLUBIGINT  aLocator,
                                      SQLUINTEGER aStartOffset);    // 62

typedef SQLRETURN (*ODBCLobPrepare4Write)(
                                  SQLSMALLINT  aHandleType,
                                  SQLHANDLE    aHandle,
                                  SQLSMALLINT  aLocatorCType,
                                  SQLUBIGINT   aLocator,
                                  SQLUINTEGER  aStartOffset,
                                  SQLUINTEGER  aSize);              // 63

typedef SQLRETURN (*ODBCLobWrite)(SQLSMALLINT  aHandleType,
                                  SQLHANDLE    aHandle,
                                  SQLSMALLINT  aLocatorCType,
                                  SQLUBIGINT   aLocator,
                                  SQLSMALLINT  aSourceCType,
                                  SQLPOINTER   aDataToPut,
                                  SQLUINTEGER  aSizeDataToPut);     // 64

typedef SQLRETURN (*ODBCLobFinishWrite)(
                                  SQLSMALLINT  aHandleType,
                                  SQLHANDLE    aHandle,
                                  SQLSMALLINT  aLocatorCType,
                                  SQLUBIGINT   aLocator);           // 65

/* PROJ-2733-DistTxInfo */
typedef SQLRETURN (*ODBCGetScn)( SQLHDBC     ConnectionHandle,
                                 SQLUBIGINT *Scn );  // 66

typedef SQLRETURN (*ODBCSetScn)( SQLHDBC     ConnectionHandle,
                                 SQLUBIGINT *Scn );  // 67

typedef SQLRETURN (*ODBCSetTxFirstStmtScn)( SQLHDBC     ConnectionHandle,
                                            SQLUBIGINT *TxFirstStmtSCN );  // 68

typedef SQLRETURN (*ODBCSetTxFirstStmtTime)( SQLHDBC     ConnectionHandle,
                                             SQLUBIGINT  TxFirstStmtTime );  // 69

typedef SQLRETURN (*ODBCSetDistLevel)( SQLHDBC      ConnectionHandle,
                                       SQLUSMALLINT DistLevel );  // 70

typedef SQLRETURN (*ODBCExecDirectAddCallback)( SQLUINTEGER   aIndex,
                                                SQLHSTMT      aStatementHandle,
                                                SQLCHAR      *aStatementText,
                                                SQLINTEGER    aTextLength,
                                                SQLPOINTER  **aCallback );  // 71

typedef void (*ODBCSetTargetShardMetaNumber)( SQLHDBC aConnectionHandle,
                                              ULong   aShardMetaNumber );   // 72

/* TASK-7219 Non-shard DML */
typedef SQLRETURN (*ODBCSetPartialExecType)( SQLHSTMT   aStmt,
                                             SQLINTEGER aPartialExecType ); // 73

typedef void      (*ODBCSetStmtExecSeq)( SQLHDBC      aConnectionHandle,
                                         SQLUINTEGER  aExecSequence ); // 74

/* SQL Function Name Tag */
#define STR_SQLAllocHandle                    "SQLAllocHandle"                    // 01
#define STR_SQLFreeHandle                     "SQLFreeHandle"                     // 02
#define STR_SQLDriverConnect                  "SQLDriverConnect"                  // 03
#define STR_SQLExecDirect                     "SQLExecDirect"                     // 04
#define STR_SQLPrepare                        "SQLPrepare"                        // 05
#define STR_SQLExecute                        "SQLExecute"                        // 06
#define STR_SQLFetch                          "SQLFetch"                          // 07
#define STR_SQLDisconnect                     "SQLDisconnect"                     // 08
#define STR_SQLFreeConnect                    "SQLFreeConnect"                    // 09
#define STR_SQLFreeStmt                       "SQLFreeStmt"                       // 10
#define STR_SQLAllocEnv                       "SQLAllocEnv"                       // 11
#define STR_SQLAllocConnect                   "SQLAllocConnect"                   // 12
#define STR_SQLExecuteForMtDataRows           "SQLExecuteForMtDataRows"           // 13
#define STR_SQLGetEnvHandle                   "SQLGetEnvHandle"                   // 14
#define STR_SQLGetDiagRec                     "SQLGetDiagRec"                     // 15
#define STR_SQLSetConnectOption               "SQLSetConnectOption"               // 16
#define STR_SQLGetConnectOption               "SQLGetConnectOption"               // 17
#define STR_SQLEndTran                        "SQLEndTran"                        // 18
#define STR_SQLTransact                       "SQLTransact"                       // 19
#define STR_SQLRowCount                       "SQLRowCount"                       // 20
#define STR_SQLGetPlan                        "SQLGetPlan"                        // 21
#define STR_SQLDescribeCol                    "SQLDescribeCol"                    // 22
#define STR_SQLDescribeColEx                  "SQLDescribeColEx"                  // 23
#define STR_SQLBindParameter                  "SQLBindParameter"                  // 24
#define STR_SQLGetConnectAttr                 "SQLGetConnectAttr"                 // 25
#define STR_SQLSetConnectAttr                 "SQLSetConnectAttr"                 // 26
#define STR_SQLBindCol                        "SQLBindCol"                        // 27
#define STR_SQLGetDbcShardTargetDataNodeName  "SQLGetDbcShardTargetDataNodeName"  // 31
#define STR_SQLGetStmtShardTargetDataNodeName "SQLGetStmtShardTargetDataNodeName" // 32
#define STR_SQLSetDbcShardTargetDataNodeName  "SQLSetDbcShardTargetDataNodeName"  // 33
#define STR_SQLSetStmtShardTargetDataNodeName "SQLSetStmtShardTargetDataNodeName" // 34
#define STR_SQLGetDbcLinkInfo                 "SQLGetDbcLinkInfo"                 // 35
#define STR_SQLGetParameterCount              "SQLGetParameterCount"              // 36
#define STR_SQLDisconnectLocal                "SQLDisconnectLocal"                // 37
#define STR_SQLSetShardPin                    "SQLSetShardPin"                    // 38
#define STR_SQLEndPendingTran                 "SQLEndPendingTran"                 // 39
#define STR_SQLPrepareAddCallback             "SQLPrepareAddCallback"             // 40
#define STR_SQLExecuteForMtDataRowsAddCallback "SQLExecuteForMtDataRowsAddCallback" // 41
#define STR_SQLExecuteForMtDataAddCallback    "SQLExecuteForMtDataAddCallback"    // 42
#define STR_SQLPrepareTranAddCallback         "SQLPrepareTranAddCallback"         // 43
#define STR_SQLEndTranAddCallback             "SQLEndTranAddCallback"             // 44
#define STR_SQLDoCallback                     "SQLDoCallback"                     // 45
#define STR_SQLGetResultCallback              "SQLGetResultCallback"              // 46
#define STR_SQLRemoveCallback                 "SQLRemoveCallback"                 // 47
#define STR_SQLSetShardMetaNumber             "SQLSetShardMetaNumber"             // 48
#define STR_SQLGetSMNOfDataNode               "SQLGetSMNOfDataNode"               // 49
#define STR_SQLGetNeedToDisconnect            "SQLGetNeedToDisconnect"            // 50
#define STR_SQLGetNeedFailover                "SQLGetNeedFailover"                // 51
#define STR_SQLReconnect                      "SQLReconnect"                      // 52
#define STR_SQLSetSavepoint                   "SQLSetSavepoint"                   // 53
#define STR_SQLRollbackToSavepoint            "SQLRollbackToSavepoint"            // 54
#define STR_SQLShardStmtPartialRollback       "SQLShardStmtPartialRollback"       // 55
#define STR_SQLEndPendingTranAddCallback      "SQLEndPendingTranAddCallback"      // 56
#define STR_SQLSetFailoverSuspend             "SQLSetFailoverSuspend"             // 57

#define STR_SQLGetLobForSd                    "SQLGetLobForSd"                    // 58
#define STR_SQLPutLobForSd                    "SQLPutLobForSd"                    // 59
#define STR_SQLGetLobLengthForSd              "SQLGetLobLengthForSd"              // 60
#define STR_SQLFreeLobForSd                   "SQLFreeLobForSd"                   // 61
#define STR_SQLTrimLobForSd                   "SQLTrimLobForSd"                   // 62
#define STR_SQLLobPrepare4Write               "SQLLobPrepare4Write"               // 63
#define STR_SQLLobWrite                       "SQLLobWrite"                       // 64
#define STR_SQLLobFinishWrite                 "SQLLobFinishWrite"                 // 65
#define STR_SQLGetScn                         "SQLGetScn"                         // 66
#define STR_SQLSetScn                         "SQLSetScn"                         // 67
#define STR_SQLSetTxFirstStmtScn              "SQLSetTxFirstStmtScn"              // 68
#define STR_SQLSetTxFirstStmtTime             "SQLSetTxFirstStmtTime"             // 69
#define STR_SQLSetDistLevel                   "SQLSetDistLevel"                   // 70
#define STR_SQLExecDirectAddCallback          "SQLExecDirectAddCallback"          // 71
#define STR_SQLSetTargetShardMetaNumber       "SQLSetTargetShardMetaNumber"       // 72
#define STR_SQLSetPartialExecType             "SQLSetPartialExecType"             // 73
#define STR_SQLSetStmtExecSeq                 "SQLSetStmtExecSeq"                 // 74

typedef enum sdlTransactionOp
{
    SDL_TRANSACT_COMMIT = 0,
    SDL_TRANSACT_ROLLBACK,
    SDL_TRANSACT_INVALID_OP
} sdlTransactionOp;

typedef enum
{
    SDL_COMMITMODE_NONAUTOCOMMIT = 0,
    SDL_COMMITMODE_AUTOCOMMIT,
    SDL_COMMITMODE_MAX
} sdlCommitMode;

typedef enum
{
    SDL_EXPLAIN_PLAN_OFF = 0,
    SDL_EXPLAIN_PLAN_ON,
    SDL_EXPLAIN_PLAN_ONLY,
    SDL_EXPLAIN_PLAN_MAX
} sdlExplainPlanOp;

#endif /* _O_SDE_H_ */
