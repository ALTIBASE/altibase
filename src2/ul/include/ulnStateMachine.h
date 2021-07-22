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

#ifndef _O_ULN_STATE_MACHINE_H_
#define _O_ULN_STATE_MACHINE_H_   1

#define ULN_FID_MAX ULN_FID_NONE

/*
 * Note : 새로운 SQL 함수 추가시
 *        1. ulnFuncId enumeration 에 추가.
 *        2. ulnStateEnvTbl, ulnStateDbcTbl, ulnStateStmtTbl 의 적절한 위치에
 *           새로운 함수의 상태 전이 테이블 추가
 *
 * 새로운 상태전이 함수 추가와는 별개의 문제임을 명심할 것.
 */
typedef enum ulnFuncId
{
    ULN_FID_ALLOCHANDLE,
    ULN_FID_BINDCOL,
    ULN_FID_BINDPARAMETER,
    ULN_FID_BROWSECONNECT,
    ULN_FID_BULKOPERATIONS,
    ULN_FID_CANCEL,
    ULN_FID_CLOSECURSOR,
    ULN_FID_COLATTRIBUTE,
    ULN_FID_COLUMNPRIVILEGES,
    ULN_FID_CONNECT,
    ULN_FID_DRIVERCONNECT,  // 10
    ULN_FID_COPYDESC,
    ULN_FID_DATASOURCES,
    ULN_FID_DESCRIBECOL,
    ULN_FID_DESCRIBEPARAM,
    ULN_FID_DISCONNECT,
    ULN_FID_ENDTRAN,
    ULN_FID_EXECDIRECT,
    ULN_FID_EXECUTE,

    /*
     * Note : SQLFetch 와 SQLFetchScroll 의 stmt 상태전이는 동일하다.
     *        굳이 FID 를 구분한 이유는 단순히, stmt 를 잡고 있는 함수의 이름을 알기 위해서이다.
     */
    ULN_FID_FETCH,
    ULN_FID_FETCHSCROLL,    // 20
    ULN_FID_EXTENDEDFETCH,

    ULN_FID_FOREIGNKEYS,
    ULN_FID_FREEHANDLE_ENV,
    ULN_FID_FREEHANDLE_DBC,
    ULN_FID_FREEHANDLE_STMT,
    ULN_FID_FREEHANDLE_DESC,
    ULN_FID_FREESTMT,
    ULN_FID_GETCONNECTATTR,
    ULN_FID_GETCURSORNAME,
    ULN_FID_GETDATA,
    ULN_FID_GETDESCFIELD,   // 31
    ULN_FID_GETDESCREC,
    ULN_FID_GETDIAGREC,
    ULN_FID_GETDIAGFIELD,
    ULN_FID_GETENVATTR,
    ULN_FID_GETFUNCTIONS,
    ULN_FID_GETINFO,
    ULN_FID_GETSTMTATTR,
    ULN_FID_GETSTMTOPTION,
    ULN_FID_GETTYPEINFO,
    ULN_FID_MORERESULTS,    // 41
    ULN_FID_NATIVESQL,
    ULN_FID_NUMPARAMS,
    ULN_FID_NUMRESULTCOLS,
    ULN_FID_PARAMDATA,
    ULN_FID_PREPARE,
    ULN_FID_PRIMARYKEYS,
    ULN_FID_PROCEDURECOLUMNS,
    ULN_FID_PROCEDURES,
    ULN_FID_PUTDATA,
    ULN_FID_ROWCOUNT,       // 51
    ULN_FID_SETCONNECTATTR,
    ULN_FID_SETCURSORNAME,
    ULN_FID_SETDESCFIELD,
    ULN_FID_SETDESCREC,
    ULN_FID_SETENVATTR,
    ULN_FID_SETPOS,
    ULN_FID_SETSTMTATTR,
    ULN_FID_SPECIALCOLUMNS,
    ULN_FID_STATISTICS,
    ULN_FID_TABLEPRIVILEGES,
    ULN_FID_TABLES,
    ULN_FID_TRANSACT,
    ULN_FID_COLUMNS,

    /*
     * LOB functions
     */
    ULN_FID_BINDFILETOCOL,
    ULN_FID_BINDFILETOPARAM,
    ULN_FID_GETLOB,
    ULN_FID_PUTLOB,
    ULN_FID_GETLOBLENGTH,
    ULN_FID_FREELOB,

    /*
     * 비표준 함수들
     */
    ULN_FID_GETPLAN,
    ULN_FID_XA, /*PROJ-1573 XA */

    /* PROJ-2047 Strengthening LOB - Added Interfaces */
    ULN_FID_TRIMLOB,

    ULN_FID_FOR_SD,

    ULN_FID_NONE
} ulnFunctionId;

/*
 * The list of state transition functions.
 * These functions check the conditions stipulated in the ODBC state transition tables
 * and then move the object's state to the next state accordingly.
 *
 * 상태전이 함수id 들의 enumeration 임.
 *
 * 새로운 상태전이 함수를 추가하는 법 :
 *      비어 있는 슬롯을 찾아서 함수명을 적는다.
 *      ulnStateMachine.h 에 정의되어 있는
 *      ulnStateFuncId enumeration 에 해당하는 ULN_SFID_XXX 상수를 추가한다.
 *      enum 을 추가한 위치와 이곳에서 함수 포인터를 추가한 위치는
 *      정확히 일치해야 한다.
 *
 *      ulnStateFuncTbl[] 배열에 해당하는 함수의 포인터가 세팅되어 있는것을 확인하여 
 *      double check 해야 한다.
 *
 * 새로운 SQL 함수 추가와는 별개의 문제임을 명심할 것.
 */
typedef enum ulnStateFunctionId
{
    /*
     * Common function
     */
    ULN_SFID_00,    /* No Transition - only check ErrorCode. */

    /*
     * for STMT tables
     */
    ULN_SFID_01,    /* NS[c], (HY010)[o], S11,S12 */
    ULN_SFID_02,    /* SQLBulkOperations S5,S6 */
    ULN_SFID_03,    /* SQLCancel S8-S10 */
    ULN_SFID_04,    /* SQLCancel S11(Async) */
    ULN_SFID_05,    /* SQLCloseCursor S5-S7 */
    ULN_SFID_06,    /* SQLColAttribute S2 */
    ULN_SFID_07,    /* SQLColAttribute S3,S5-S7 : --[s], S11[x] */
    ULN_SFID_08,    /* SYS_.SYS_ request S1 */
    ULN_SFID_09,    /* SYS_.SYS_ request S2-S3 */
    ULN_SFID_10,    /* SYS_.SYS_ request S4 */
    ULN_SFID_11,    /* SQLCopyDesc S11-S12 */
    ULN_SFID_12,    /* SQLDisconnect S1-S7, Close All Statement */
    ULN_SFID_13,    /* SQLEndTran S2-S3 */
    ULN_SFID_14,    /* SQLEndTran S4 */
    ULN_SFID_15,    /* SQLEndTran S5-S7 */
    ULN_SFID_16,    /* SQLExecDirect S1 */
    ULN_SFID_17,    /* SQLExecDirect S2-S3 */
    ULN_SFID_18,    /* SQLExecDirect S4 */
    ULN_SFID_19,    /* SQLExecute S2 */
    ULN_SFID_20,    /* SQLExecute S3 */
    ULN_SFID_21,    /* SQLExecute S4 */
    ULN_SFID_22,    /* SQLExecute S5,S7 */
    ULN_SFID_23,    /* SQLExecute S6 */
    ULN_SFID_24,    /* SQLFetch and SQLSCroollFetch S5 */
    ULN_SFID_25,    /* SQLFetch and SQLSCroollFetch S6 */
    ULN_SFID_26,    /* SQLFreeHandle S1-S7 */
    ULN_SFID_27,    /* SQLFreeStmt S4 */
    ULN_SFID_28,    /* SQLFreeStmt S5-S7 */
    ULN_SFID_29,    /* SQLGetData(Cursor State) S6,S7 */
    ULN_SFID_30,    /* SQLGetDescField / SQLGetDescRec S1 */
    ULN_SFID_31,    /* SQLGetDescField / SQLGetDescRec S2 */
    ULN_SFID_32,    /* SQLGetDescField / SQLGetDescRec S3 */
    ULN_SFID_33,    /* SQLGetDescField / SQLGetDescRec S4 */
    ULN_SFID_34,    /* SQLGetDescField / SQLGetDescRec S5-S7 */
    ULN_SFID_35,    /* SQLGetDescField / SQLGetDescRec S11,S12 */
    ULN_SFID_36,    /* SQLGetStmtAttr S1, S2-S3, S4, S5 */
    ULN_SFID_37,    /* SQLGetStmtAttr S6 */
    ULN_SFID_38,    /* SQLGetStmtAttr S7 */
    ULN_SFID_39,    /* SQLMoreResult S4 */
    ULN_SFID_40,    /* SQLMoreResult S5-S7 */
    ULN_SFID_41,    /* SQLParamData(NeedDataStates) S8 (need data) */
    ULN_SFID_42,    /* SQLParamData(NeedDataStates) S10 (can put) */
    ULN_SFID_43,    /* SQLPrepare S1 */
    ULN_SFID_44,    /* SQLPrepare S2-S3 */
    ULN_SFID_45,    /* SQLPrepare S4 */
    ULN_SFID_46,    /* SQLPutData S9 (must put) */
    ULN_SFID_47,    /* SQLPutData S10 (can put) */
    ULN_SFID_48,    /* SQLSetConnecAttr S5-S7 */
    ULN_SFID_49,    /* SQLSetPos(Cursor State) S6, S7 */
    ULN_SFID_50,    /* SQLSetStmtAttr S2-S3, S4, S5-S7 */
    ULN_SFID_51,    /* SQLSetStmtAttr S8-S10, S11-S12 */
    ULN_SFID_52,    /* SQLExtendedFetch S5 */

    /*
     * For DBC tables
     */

    ULN_SFID_60,    /* SQLAllocHandle C2, C3 */
    ULN_SFID_61,    /* SQLAllocHandle C4 */
    ULN_SFID_62,    /* SQLBrowseConnect C2 */
    ULN_SFID_63,    /* SQLBrowseConnect C3 */
    ULN_SFID_64,    /* SQLCloseCursor C6 (Transaction) */
    ULN_SFID_65,    /* SYS_.SYS_ C5 */
    ULN_SFID_66,    /* SQLCopyDesc, SQLGetDesc... C4 */
    ULN_SFID_67,    /* SQLDisconnect C3, C4, C5 */
    ULN_SFID_68,    /* SQLDriverConnect C2 */
    ULN_SFID_69,    /* SQLEndTran C6 */
    ULN_SFID_70,    /* SQLExecDirect C5 (Transaction Begin) */
    ULN_SFID_71,    /* SQLFreeHandle C2 */
    ULN_SFID_72,    /* SQLFreeHandle C3 */
    ULN_SFID_73,    /* SQLFreeHandle C4 */
    ULN_SFID_74,    /* SQLFreeHandle C5 */
    ULN_SFID_75,    /* SQLFreeHandle C6 */
    ULN_SFID_76,    /* SQLFreeStmt C6 */
    ULN_SFID_77,    /* SQLGetConnectAttr C2 */
    ULN_SFID_78,    /* SQLGetInfo C2 */
    ULN_SFID_79,    /* SQLMoreResult C5 */
    ULN_SFID_80,    /* SQLMoreResult C6 */
    ULN_SFID_81,    /* SQLPrepare C5 */
    ULN_SFID_82,    /* SQLSetConnectAttr C2 */
    ULN_SFID_83,    /* SQLSetConnectAttr C4, C5 */
    ULN_SFID_84,    /* SQLSetConnectAttr C6 */
    ULN_SFID_85,    /* SQLConnect C2 */

    /*
     * For ENV tables
     */
    ULN_SFID_90,    /* SQLAllocHandle E1 */
    ULN_SFID_91,    /* SQLDataSource E1, E2 */
    ULN_SFID_92,    /* SQLEndTran E1 */
    ULN_SFID_93,    /* SQLEndTran E2 */
    ULN_SFID_94,    /* SQLFreeaHandle E1 */
    ULN_SFID_95,    /* SQLFreeHandle E2 */
    ULN_SFID_96,    /* SQLSetEnvAttr E1 */

    ULN_SFID_127,   /* Not Supported in Altibase */

    ULN_SFID_MAX
} ulnStateFuncId;

/*
 * ulnStateCheckPoint.
 *
 * ulnStateMachine() 함수가 호출되는 위치를 나타내는 상수
 *      ENTRY_POINT : ulnEnter() 함수에서 호출됨
 *      EXIT_POINT  : ulnExit() 함수에서 호출됨
 */
typedef enum ulnStateCheckPoint
{
    ULN_STATE_ENTRY_POINT,
    ULN_STATE_EXIT_POINT,
    ULN_STATE_NOWHERE
} ulnStateCheckPoint;

typedef ACI_RC ulnStateFunc(ulnFnContext *aContext);

typedef struct ulnStateTblEntry ulnStateTblEntry;

struct ulnStateTblEntry
{
    ulnStateFuncId mFuncId;
    acp_uint32_t   mErrCode;
};

/*
 * Note: ULN_S_ 는 ULN_STATE_ 의 약자임.
 * 이렇게 짧게 한 이유는 State Transition Table 배열을 적는데
 * 이름이 너무 길면 알아보기가 너무 힘들어서이다.
 */

/************************************************************
 * Environment States
 ***********************************************************/
typedef enum
{
    ULN_S_E0,   /* Unallocated environment */
    ULN_S_E1,   /* Allocated environment, unallocated connection */
    ULN_S_E2,   /* Allocated environment, allocated connection */
    ULN_MAX_ENV_STATE
} ulnEnvState;

/************************************************************
 * Connection States
 ***********************************************************/
typedef enum
{
    ULN_S_C0,   /* Unallocated environment, unallocated connection */
    ULN_S_C1,   /* Allocated environment, unallocated connection */
    ULN_S_C2,   /* Allocated environment, allocated connection */
    ULN_S_C3,   /* Connection function needs data */
    ULN_S_C4,   /* Connected connection */
    ULN_S_C5,   /* Connected connection, allocated statement */
    ULN_S_C6,
    ULN_MAX_DBC_STATE
} ulnDbcState;

/************************************************************
 * Statement States
 ***********************************************************/
typedef enum
{
    ULN_S_S0,
    ULN_S_S1,
    ULN_S_S2,
    ULN_S_S3,
    ULN_S_S4,
    ULN_S_S5,
    ULN_S_S6,
    ULN_S_S7,
    ULN_S_S8,
    ULN_S_S9,
    ULN_S_S10,
    ULN_S_S11,

    /*
     * Note: S12 상태로 가는 경우는 절대로 없다. SQLCancel() 함수가 유일하게 S12 로의 상태 전이를
     * 정의하고 있는 함수인데, S12 로 전이할 이벤트 자체를 지원하지 않겠다.
     * SQL_AM_CONNECTION, SQL_AM_STATEMENT 둘 다 지원하지 않을 계획이기 때문이다.
     * 그래서 S12 상태는 아예 없애버렸다.
     *
     * ---> 일단은 남겨 두자.
     */
    ULN_S_S12,

    ULN_MAX_STMT_STATE
} ulnStmtState;

/************************************************************
 * Descriptor States
 ***********************************************************/
typedef enum
{
    ULN_S_D0,
    ULN_S_D1i,
    ULN_S_D1e
} ulnDescState;

/*
 * Function Declarations
 */

ACI_RC ulnEnter(ulnFnContext *aFnContext, void *aArgs);
ACI_RC ulnExit(ulnFnContext *aFnContext);

acp_bool_t ulnStateCheckR(ulnFnContext *aFnContext);
acp_bool_t ulnStateCheckLastResult(ulnFnContext *aFnContext);

/* PROJ-1891 Deferred Prepare */
void          ulnUpdateDeferredState(ulnFnContext *aFnContext, ulnStmt *aStmt);
ulnStateFunc *ulnStateGetStateFunc(ulnFnContext *aFnContext);

#endif  /* _O_ULN_STATE_MACHINE_H_ */

