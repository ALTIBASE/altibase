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
#include <ulnGetEnvAttr.h>
#include <ulnSetEnvAttr.h>
#include <ulnExecute.h>
#include <ulnPrepare.h>
#include <ulnAllocHandle.h>
#include <ulnBindCol.h>
#include <ulnBindParameter.h>
#include <ulnFreeHandle.h>
#include <ulnExecDirect.h>
#include <ulnSetStmtAttr.h>
#include <ulnGetStmtAttr.h>
#include <ulnFreeStmt.h>
#include <ulnGetTypeInfo.h>
#include <ulnPutData.h>
#include <ulnGetData.h>
#include <ulnParamData.h>
#include <ulnFetch.h>
#include <ulnGetInfo.h>
#include <ulnGetConnectAttr.h>
#include <ulnMoreResults.h>
#include <ulnSetPos.h>
#include <ulnBulkOperations.h>
#include <ulnStateMachine.h>

#if defined(ALTI_CFG_OS_LINUX)
#include <sys/syscall.h>
#endif

/*
 * Implements state machine of ODBC.
 * Precautions that you should keep in mind :
 *
 * -#. All the top level functions which can be directly mapped to ODBC functions should be
 *     in the following format :
 *      @code
 *      01: SQLRETURN ulnGetEnvAttr(ulnEnv *aEnv, SInt aAttribute, void *aValuePtr)
 *      02: {
 *      03:     ulnFnContext sContext;
 *      04:     ULN_INIT_FUNCTION_CONTEXT(sContext, ULN_FID_GETENVATTR, aEnv, ULN_OBJ_TYPE_ENV);
 *      05:
 *      06:     ACI_TEST(ulnEnter(&sContext, NULL) != ACI_SUCCESS);
 *      07:
 *      08:     ACI_TEST_RAISE(ulnGetEnvAttrCheckArgs(&sContext, aValuePtr, aStringLengthPtr)
 *      09:                    != ACI_SUCCESS,
 *      10:                    ULN_LABEL_NEED_EXIT);
 *      11:
 *      12:     // function's main body
 *      13:
 *      14:     ACI_TEST(ulnExit(&sContext) != ACI_SUCCESS);
 *      15:
 *      16:     return sContext.mSqlReturn;
 *      17:
 *      18:     ACI_EXCEPTION(ULN_LABEL_NEED_EXIT)
 *      19:     {
 *      20:         ulnExit(&sContext);
 *      21:     }
 *      22:
 *      23:     ACI_EXCEPTION_END;
 *      24:
 *      25:     return sContext.mSqlReturn;
 *      26: }
 *      @endcode
 *
 * -#. If ACI_TEST macro is laid in between ulnEnter() and ulnExit(), you sould use
 *     ACI_TEST_RAISE with the label pointing to ulnExit() at the end of the function, as shown
 *     in line number 08.
 *
 * -#. If ACI_TEST macro is positioned outside of ulnEnter() and ulnExit(), you can freely
 *     use ACI_TEST or ACI_TEST_RAISE.
 *
 * -#. Checking arguments should be done after ulnEnter() is called. Refer to line 08.
 *
 * -#. The ulnEnter() and ulnExit() should be in ACI_TEST() macro, as in line number 06 and 16.
 *
 * -#. The ulnExit() of error situation should not be using ACI_TEST(), as you can see in line
 *     number 20.
 */

/*
 * ulnStateCheckR.
 *
 * check [r] : the statement will or did create a (possibly empty) result set.
 */
acp_bool_t ulnStateCheckR(ulnFnContext *aFnContext)
{
    acp_uint16_t sResultSetCount;

    sResultSetCount = ulnStmtGetResultSetCount(aFnContext->mHandle.mStmt);

    if (sResultSetCount == 0)
    {
        /*
         * [nr]
         */
        return ACP_FALSE;
    }
    else
    {
        /*
         * [r]
         */
        return ACP_TRUE;
    }
}

acp_bool_t ulnStateCheckLastResult(ulnFnContext *aFnContext)
{
    ACP_UNUSED(aFnContext);

    /*
     * BUGBUG: Make it happen!
     */
    if (1)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

ACI_RC ulnSF_NotImplemented(ulnFnContext * aFnContext)
{
    ACP_UNUSED(aFnContext);

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_00
 * Common Function.
 *
 * 에러 혹은 상태전이 없이 그냥 통과하는 함수.
 */
ACI_RC ulnSFID_00(ulnFnContext *aFnContext)
{
    switch (aFnContext->mUlErrorCode)
    {
        case ulERR_IGNORE_NO_ERROR:
            break;

        case ulERR_ABORT_INVALID_HANDLE:
            ACI_RAISE(LABEL_INVALID_HANDLE);
            break;

            /*
             * BUGBUG : 상태머신에서 발생할 수 있는 에러들을 일일이 체크해 주어야 한다.
             */
        default:
            ACI_TEST(ulnError(aFnContext, aFnContext->mUlErrorCode) != ACI_SUCCESS);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_HANDLE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_01
 * Common Function
 *      NS [c]
 *      (HY010) [o]
 *  where
 *      [c] Current function. The current function was executing asynchronously.
 *      [o] Other function. Another function was executing asynchronously.
 *
 * STMT 상태전이 테이블의 S11-S12 컬럼에 있다.
 */
ACI_RC ulnSFID_01(ulnFnContext *aFnContext)
{
    ACP_UNUSED(aFnContext);

    return ACI_FAILURE;
}

/*
 * ULN_SFID_07 : Common function
 * SQLDescribeCol(), STMT, S3, S5-S7
 *      -- [s]
 *      S11 [x]
 *  where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 */
ACI_RC ulnSFID_07(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        /*
         * BUGBUG: check still Executing state
         */
    }

    return ACI_SUCCESS;
}

/*
 * The list of state transition functions.
 * These functions check the conditions stipulated in the ODBC state transition tables
 * and then move the object's state to the next state accordingly.
 *
 * 상태전이 함수들의 목록임.
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
ulnStateFunc *ulnStateFuncTbl[ULN_SFID_MAX] =
{
    ulnSFID_00,             /* No Transition - only check ErrorCode. */

    /*
     * STMT
     */
    ulnSFID_01,             /* 공통함수 NS[c], (HY010)[o], S11,S12 */

    ulnSFID_02,             /* SQLBulkOperations S5,S6 */
    ulnSFID_03,             /* SQLCancel S8-S10 */
    ulnSF_NotImplemented,   /* SQLCancel S11(Async) */
    ulnSFID_05,             /* SQLCloseCursor S5-S7 */
    ulnSF_NotImplemented,   /* SQLColAttribute S2 */

    ulnSFID_07,             /* 공통함수 --[s], S11[x] */

    /*
     * SFID_08 - SFID_10
     * SQLColumnPrivileges, SQLColumns, SQLForeignKeys, SQLGetTypeInfo, SQLPrimaryKeys,
     * SQLProcedureColumns, SQLProcedures, SQLSpecialColumns, SQLStatistics,
     * SQLTablePrivileges, and SQLTables
     */
    ulnSFID_08,
    ulnSFID_09,
    ulnSFID_10,

    ulnSF_NotImplemented,   /* SQLCopyDesc S11-S12 */
    ulnSF_NotImplemented,   /* SQLDisconnect S1-S7, Close All Statement */
    ulnSF_NotImplemented,   /* SQLEndTran S2-S3 */
    ulnSF_NotImplemented,   /* SQLEndTran S4 */
    ulnSF_NotImplemented,   /* SQLEndTran S5-S7 */

    ulnSFID_16,             /* SQLExecDirect S1 */
    ulnSFID_17,             /* SQLExecDirect S2-S3 */
    ulnSFID_18,             /* SQLExecDirect S4 */
    ulnSFID_19,             /* SQLExecute S2 */
    ulnSFID_20,             /* SQLExecute S3 */
    ulnSFID_21,             /* SQLExecute S4 */
    ulnSFID_22,             /* SQLExecute S5,S7 */
    ulnSFID_23,             /* SQLExecute S6 */
    ulnSFID_24,             /* SQLFetch and SQLSCroollFetch S5 */
    ulnSFID_25,             /* SQLFetch and SQLSCroollFetch S6 */
    ulnSFID_26,             /* SQLFreeHandleStmt S1-S7 */
    ulnSFID_27,             /* SQLFreeStmt S4 */
    ulnSFID_28,             /* SQLFreeStmt S5-S7 */
    ulnSFID_29,             /* SQLGetData S6,S7 */

    ulnSF_NotImplemented,   /* SQLGetDescField / SQLGetDescRec S1 */
    ulnSF_NotImplemented,   /* SQLGetDescField / SQLGetDescRec S2 */
    ulnSF_NotImplemented,   /* SQLGetDescField / SQLGetDescRec S3 */
    ulnSF_NotImplemented,   /* SQLGetDescField / SQLGetDescRec S4 */
    ulnSF_NotImplemented,   /* SQLGetDescField / SQLGetDescRec S5-S7 */
    ulnSF_NotImplemented,   /* SQLGetDescField / SQLGetDescRec S11,S12 */

    ulnSFID_36,             /* SQLGetStmtAttr S1, S2-S3, S4, S5 */
    ulnSFID_37,             /* SQLGetStmtAttr S6 */
    ulnSFID_38,             /* SQLGetStmtAttr S7 */

    ulnSFID_39,             /* SQLMoreResults S4 */
    ulnSFID_40,             /* SQLMoreResults S5-S7 */

    ulnSFID_41,             /* SQLParamData(NeedDataStates) S8(need data)*/
    ulnSFID_42,             /* SQLParamData(NeedDataStates) S10(can put) */
    ulnSFID_43,             /* SQLPrepare S1 */
    ulnSFID_44,             /* SQLPrepare S2-S3 */
    ulnSFID_45,             /* SQLPrepare S4 */
    ulnSFID_46,             /* SQLPutData S9 (must put) */
    ulnSFID_47,             /* SQLPutData S10 (can put) */

    ulnSF_NotImplemented,   /* SQLSetConnecAttr S5-S7 */
    ulnSFID_49,             /* SQLSetPos(Cursor State) S6, S7 */

    ulnSFID_50,             /* SQLSetStmtAttr S2-S3, S4, S5-S7 */
    ulnSFID_51,             /* SQLSetStmtAttr S8-S10, S11-S12 */
    ulnSFID_52,             /* SQLExtendedFetch S5 */

    /*
     * DBC
     */
    ulnSFID_60,             /* SQLAllocHandle C2, C3 */
    ulnSFID_61,             /* SQLAllocHandle C4 */

    ulnSF_NotImplemented,   /* SQLBrowseConnect C2 */
    ulnSF_NotImplemented,   /* SQLBrowseConnect C3 */
    ulnSF_NotImplemented,   /* SQLCloseCursor C6 (Transaction) */

    ulnSFID_65,             /* SQLColumnPrivileges, SQLGetTypeInfo, SQLTables etc... C5 */

    ulnSF_NotImplemented,   /* SQLCopyDesc, SQLGetDesc... C4 */

    ulnSFID_67,             /* SQLDisconnect C3, C4, C5 */
    ulnSFID_68,             /* SQLDriverConnect C2 */

    ulnSF_NotImplemented,   /* SQLEndTran C6 */

    ulnSFID_70,             /* SQLExecute, SQLExecDirect C5 (Transaction Begin) */
    ulnSFID_71,             /* SQLFreeHandleDbc C2 */

    ulnSF_NotImplemented,   /* SQLFreeHandle C3 -----> UNUSED */
    ulnSF_NotImplemented,   /* SQLFreeHandle C4 -----> UNUSED */

    ulnSFID_74,             /* SQLFreeHandleStmt C5 */
    ulnSFID_75,             /* SQLFreeHandleStmt C6 */
    ulnSFID_76,             /* SQLFreeStmt C6 */

    ulnSFID_77,             /* SQLGetConnectAttr C2 */
    ulnSFID_78,             /* SQLGetInfo C2 */
    ulnSF_NotImplemented,   /* SQLMoreResult C5 */
    ulnSF_NotImplemented,   /* SQLMoreResult C6 */
    ulnSF_NotImplemented,   /* SQLPrepare C5 */

    ulnSFID_82,             /* SQLSetConnectAttr C2 */
    ulnSFID_83,             /* SQLSetConnectAttr C4, C5 */
    ulnSFID_84,             /* SQLSetConnectAttr C6 */
    ulnSFID_85,             /* SQLConnect C2 */

    /*
     * ENV
     */
    ulnSFID_90,             /* SQLAllocHandle E1 */
    ulnSFID_91,             /* SQLDataSource E1,E2, SQLGetEnvAttr */

    ulnSF_NotImplemented,   /* SQLEndTran E1 */
    ulnSF_NotImplemented,   /* SQLEndTran E2 */

    ulnSFID_94,             /* SQLFreeHandleEnv E1 */
    ulnSFID_95,             /* SQLFreeHandleDbc E2 */
    ulnSFID_96,             /* SQLSetEnvAttr E1 */

    ulnSF_NotImplemented    /* Not Supported in Altibase */
};

/*
 * ENV handle's state transition table
 */

#define ULN_ENV_COMMON \
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}
#define ULN_ENV_NO_CHECK \
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}

#define ULN_EI_IH000 ulERR_ABORT_INVALID_HANDLE
#define ULN_EI_00000 ulERR_IGNORE_NO_ERROR
#define ULN_EI_HY010 ulERR_ABORT_FUNCTION_SEQUENCE_ERR
#define ULN_EI_HY011 ulERR_ABORT_ATTRIBUTE_CANNOT_BE_SET_NOW
#define ULN_EI_08002 ulERR_ABORT_CONNECTION_NAME_IN_USE
#define ULN_EI_08003 ulERR_ABORT_NO_CONNECTION
#define ULN_EI_24000 ulERR_ABORT_INVALID_CURSOR_STATE
#define ULN_EI_25000 ulERR_ABORT_INVALID_TRANSACTION_STATE
#define ULN_EI_07005 ulERR_ABORT_STMT_HAVE_NO_RESULT_SET

ulnStateTblEntry ulnStateEnvTbl[ULN_FID_MAX][ULN_MAX_ENV_STATE] =
{
    {
        /* ULN_FID_ALLOCHANDLE */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_90, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}
    },
    { ULN_ENV_COMMON }, /* ULN_FID_BINDCOL */
    { ULN_ENV_COMMON }, /* ULN_FID_BINDPARAMETER */
    { ULN_ENV_COMMON }, /* ULN_FID_BROWSECONNECT */
    { ULN_ENV_COMMON }, /* ULN_FID_BULKOPERATIONS */
    { ULN_ENV_COMMON }, /* ULN_FID_CANCEL */
    { ULN_ENV_COMMON }, /* ULN_FID_CLOSECURSOR */
    { ULN_ENV_COMMON }, /* ULN_FID_COLATTRIBUTE */
    { ULN_ENV_COMMON }, /* ULN_FID_COLUMNPRIVILEGES */
    { ULN_ENV_COMMON }, /* ULN_FID_CONNECT */
    { ULN_ENV_COMMON }, /* ULN_FID_DRIVERCONNECT */
    { ULN_ENV_COMMON }, /* ULN_FID_COPYDESC */
    {
        /* ULN_FID_DATASOURCES */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_91, ULN_EI_00000}, {ULN_SFID_91, ULN_EI_00000}
    },
    { ULN_ENV_COMMON }, /* ULN_FID_DESCRIBECOL */
    { ULN_ENV_COMMON }, /* ULN_FID_DESCRIBEPARAM */
    { ULN_ENV_COMMON }, /* ULN_FID_DISCONNECT */
    {
        /* ULN_FID_ENDTRAN */
        /*
         * BUGBUG : 만들어야 한다.
         */
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_92, ULN_EI_00000}, {ULN_SFID_93, ULN_EI_00000}
    },
    { ULN_ENV_COMMON }, /* ULN_FID_EXECDIRECT */
    { ULN_ENV_COMMON }, /* ULN_FID_EXECUTE */
    { ULN_ENV_COMMON }, /* ULN_FID_FETCH */
    { ULN_ENV_COMMON }, /* ULN_FID_FETCHSCROLL */
    { ULN_ENV_COMMON }, /* ULN_FID_EXTENDEDFETCH */
    { ULN_ENV_COMMON }, /* ULN_FID_FOREIGNKEYS */

    {
        /* ULN_FID_FREEHANDLE_ENV */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_94, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010}
    },
    {
        /* ULN_FID_FREEHANDLE_DBC */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_95, ULN_EI_00000}
    },
    { ULN_ENV_COMMON }, /* ULN_FID_FREEHANDLE_STMT */
    { ULN_ENV_COMMON }, /* ULN_FID_FREEHANDLE_DESC */

    { ULN_ENV_COMMON }, /* ULN_FID_FREESTMT */
    { ULN_ENV_COMMON }, /* ULN_FID_GETCONNECTATTR */
    { ULN_ENV_COMMON }, /* ULN_FID_GETCURSORNAME */
    { ULN_ENV_COMMON }, /* ULN_FID_GETDATA */
    { ULN_ENV_COMMON }, /* ULN_FID_GETDESCFIELD */
    { ULN_ENV_COMMON }, /* ULN_FID_GETDESCREC */
    { ULN_ENV_COMMON }, /* ULN_FID_GETDIAGREC,  */
    { ULN_ENV_COMMON }, /* ULN_FID_GETDIAGFIELD,  */
    {
        /* ULN_FID_GETENVATTR */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_91, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}
    },
    { ULN_ENV_COMMON }, /* ULN_FID_GETFUNCTIONS,  */
    { ULN_ENV_COMMON }, /* ULN_FID_GETINFO */
    { ULN_ENV_COMMON }, /* ULN_FID_GETSTMTATTR */
    { ULN_ENV_COMMON }, /* ULN_FID_GETSTMTOPTION */
    { ULN_ENV_COMMON }, /* ULN_FID_GETTYPEINFO */
    { ULN_ENV_COMMON }, /* ULN_FID_MORERESULTS */
    { ULN_ENV_COMMON }, /* ULN_FID_NATIVESQL */
    { ULN_ENV_COMMON }, /* ULN_FID_NUMPARAMS */
    { ULN_ENV_COMMON }, /* ULN_FID_NUMRESULTCOLS */
    { ULN_ENV_COMMON }, /* ULN_FID_PARAMDATA */
    { ULN_ENV_COMMON }, /* ULN_FID_PREPARE */
    { ULN_ENV_COMMON }, /* ULN_FID_PRIMARYKEYS */
    { ULN_ENV_COMMON }, /* ULN_FID_PROCEDURECOLUMNS */
    { ULN_ENV_COMMON }, /* ULN_FID_PROCEDURES */
    { ULN_ENV_COMMON }, /* ULN_FID_PUTDATA */
    { ULN_ENV_COMMON }, /* ULN_FID_ROWCOUNT */
    { ULN_ENV_COMMON }, /* ULN_FID_SETCONNECTATTR */
    { ULN_ENV_COMMON }, /* ULN_FID_SETCURSORNAME */
    { ULN_ENV_COMMON }, /* ULN_FID_SETDESCFIELD */
    { ULN_ENV_COMMON }, /* ULN_FID_SETDESCREC */
    {
        /* ULN_FID_SETENVATTR */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_96, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY011}
    },
    { ULN_ENV_COMMON }, /* ULN_FID_SETPOS */
    { ULN_ENV_COMMON }, /* ULN_FID_SETSTMTATTR */
    { ULN_ENV_COMMON }, /* ULN_FID_SPECIALCOLUMNS */
    { ULN_ENV_COMMON }, /* ULN_FID_STATISTICS */
    { ULN_ENV_COMMON }, /* ULN_FID_TABLEPRIVILEGES */
    { ULN_ENV_COMMON }, /* ULN_FID_TABLES */
    { ULN_ENV_COMMON }, /* ULN_FID_TRANSACT */
    { ULN_ENV_COMMON }, /* ULN_FID_COLUMNS */

    { ULN_ENV_COMMON }, /* ULN_FID_BINDFILETOCOL */
    { ULN_ENV_COMMON }, /* ULN_FID_BINDFILETOPARAM */
    { ULN_ENV_COMMON }, /* ULN_FID_GETLOB */
    { ULN_ENV_COMMON }, /* ULN_FID_PUTLOB */
    { ULN_ENV_COMMON }, /* ULN_FID_GETLOBLENGTH */
    { ULN_ENV_COMMON }, /* ULN_FID_FREELOB */
    { ULN_ENV_COMMON }, /* ULN_FID_GETPLAN */
    { ULN_ENV_COMMON }, /* ULN_FID_XA */

    /* PROJ-2047 Strengthening LOB - Added Interfaces */
    { ULN_ENV_COMMON }, /* ULN_FID_TRIMLOB */

    { ULN_ENV_COMMON }, /* ULN_FID_FOR_SD */
};

/*
 * DBC handle's state transition table
 */

#define ULN_DBC_COMMON \
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},  {ULN_SFID_00, ULN_EI_IH000}, \
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},  {ULN_SFID_00, ULN_EI_IH000}, \
        {ULN_SFID_00, ULN_EI_IH000}

#define ULN_DBC_NO_CHECK \
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},  \
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_65, ULN_EI_00000},  \
        {ULN_SFID_00, ULN_EI_00000}
ulnStateTblEntry ulnStateDbcTbl[ULN_FID_MAX][ULN_MAX_DBC_STATE] =
{
    {
        /* ULN_FID_ALLOCHANDLE */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_60, ULN_EI_00000},
        {ULN_SFID_60, ULN_EI_00000}, {ULN_SFID_61, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_DBC_COMMON }, /* ULN_FID_BINDCOL */
    { ULN_DBC_COMMON }, /* ULN_FID_BINDPARAMETER */
    { ULN_DBC_COMMON }, /* ULN_FID_BROWSECONNECT */
    { ULN_DBC_COMMON }, /* ULN_FID_BULKOPERATIONS */
    { ULN_DBC_COMMON }, /* ULN_FID_CANCEL */
    { ULN_DBC_COMMON }, /* ULN_FID_CLOSECURSOR */
    { ULN_DBC_COMMON }, /* ULN_FID_COLATTRIBUTE */
    { ULN_DBC_COMMON }, /* ULN_FID_COLUMNPRIVILEGES */

    {
        /* ULN_FID_CONNECT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_85, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002},
        {ULN_SFID_00, ULN_EI_08002}
    },

    {
        /* ULN_FID_DRIVERCONNECT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_68, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002},
        {ULN_SFID_00, ULN_EI_08002}
    },

    { ULN_DBC_COMMON }, /* ULN_FID_COPYDESC */
    { ULN_DBC_COMMON }, /* ULN_FID_DATASOURCES */
    { ULN_DBC_COMMON }, /* ULN_FID_DESCRIBECOL */
    { ULN_DBC_COMMON }, /* ULN_FID_DESCRIBEPARAM */
    {
        /* ULN_FID_DISCONNECT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_08003},
        {ULN_SFID_67, ULN_EI_00000}, {ULN_SFID_67, ULN_EI_00000}, {ULN_SFID_67, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_25000}
    },
    {
        /* ULN_FID_ENDTRAN */
        /*
         * BUGBUG : 만들어야 한다.
         */
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_70, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    {
        /* ULN_FID_EXECDIRECT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_70, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    {
        /* ULN_FID_EXECUTE */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_70, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_DBC_COMMON }, /* ULN_FID_FETCH */
    { ULN_DBC_COMMON }, /* ULN_FID_FETCHSCROLL */
    { ULN_DBC_COMMON }, /* ULN_FID_EXTENDEDFETCH */
    { ULN_DBC_COMMON }, /* ULN_FID_FOREIGNKEYS */

    /*
     * SQLFreeHandle subsidiaries in Connection State Transition Table
     */
    { ULN_DBC_COMMON }, /* ULN_FID_FREEHANDLE_ENV */
    {
        /* ULN_FID_FREEHANDLE_DBC */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_71, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}
    },
    {
        /* ULN_FID_FREEHANDLE_STMT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_74, ULN_EI_00000},
        {ULN_SFID_75, ULN_EI_00000},
    },
    {
        /* ULN_FID_FREEHANDLE_DESC */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    {
        /* ULN_FID_FREESTMT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_76, ULN_EI_00000}
    },

    {
        /* ULN_FID_GETCONNECTATTR */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_77, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_DBC_COMMON }, /* ULN_FID_GETCURSORNAME */
    { ULN_DBC_COMMON }, /* ULN_FID_GETDATA */
    { ULN_DBC_COMMON }, /* ULN_FID_GETDESCFIELD */
    { ULN_DBC_COMMON }, /* ULN_FID_GETDESCREC */
    { ULN_DBC_COMMON }, /* ULN_FID_GETDIAGREC,  */
    { ULN_DBC_COMMON }, /* ULN_FID_GETDIAGFIELD,  */
    { ULN_DBC_COMMON }, /* ULN_FID_GETDBCATTR */

    {
        /* ULN_FID_GETFUNCTIONS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    {
        /* ULN_FID_GETINFO */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_78, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_08003}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_DBC_COMMON }, /* ULN_FID_GETSTMTATTR */
    { ULN_DBC_COMMON }, /* ULN_FID_GETSTMTOPTION */
    {
        /* ULN_FID_GETTYPEINFO */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_65, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },
    { ULN_DBC_COMMON }, /* ULN_FID_MORERESULTS */

    {
        /* ULN_FID_NATIVESQL */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_08003},
        {ULN_SFID_00, ULN_EI_08003}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_DBC_COMMON }, /* ULN_FID_NUMPARAMS */
    { ULN_DBC_COMMON }, /* ULN_FID_NUMRESULTCOLS */
    { ULN_DBC_COMMON }, /* ULN_FID_PARAMDATA */
    { ULN_DBC_COMMON }, /* ULN_FID_PREPARE */
    { ULN_DBC_COMMON }, /* ULN_FID_PRIMARYKEYS */
    { ULN_DBC_COMMON }, /* ULN_FID_PROCEDURECOLUMNS */
    { ULN_DBC_COMMON }, /* ULN_FID_PROCEDURES */
    { ULN_DBC_COMMON }, /* ULN_FID_PUTDATA */
    { ULN_DBC_COMMON }, /* ULN_FID_ROWCOUNT */
    {
        /* ULN_FID_SETCONNECTATTR */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_82, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_83, ULN_EI_00000}, {ULN_SFID_83, ULN_EI_00000},
        {ULN_SFID_84, ULN_EI_00000}
    },
    { ULN_DBC_COMMON }, /* ULN_FID_SETCURSORNAME */
    { ULN_DBC_COMMON }, /* ULN_FID_SETDESCFIELD */
    { ULN_DBC_COMMON }, /* ULN_FID_SETDESCREC */
    { ULN_DBC_COMMON }, /* ULN_FID_SETDBCATTR */
    { ULN_DBC_COMMON }, /* ULN_FID_SETPOS */
    { ULN_DBC_COMMON }, /* ULN_FID_SETSTMTATTR */
    { ULN_DBC_COMMON }, /* ULN_FID_SPECIALCOLUMNS */
    { ULN_DBC_COMMON }, /* ULN_FID_STATISTICS */
    { ULN_DBC_COMMON }, /* ULN_FID_TABLEPRIVILEGES */

    {
        /* ULN_FID_TABLES */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_65, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_DBC_COMMON }, /* ULN_FID_TRANSACT */

    {
        /* ULN_FID_COLUMNS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_65, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_DBC_COMMON }, /* ULN_FID_BINDFILETOCOL */
    { ULN_DBC_COMMON }, /* ULN_FID_BINDFILETOPARAM */
    {
        /* ULN_FID_GETLOB */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_08003},
        {ULN_SFID_00, ULN_EI_08003}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },
    {
        /* ULN_FID_PUTLOB */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_08003},
        {ULN_SFID_00, ULN_EI_08003}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },
    {
        /* ULN_FID_GETLOBLENGTH */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_08003},
        {ULN_SFID_00, ULN_EI_08003}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },
    {
        /* ULN_FID_FREELOB */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_08003},
        {ULN_SFID_00, ULN_EI_08003}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },
    { ULN_DBC_COMMON }, /* ULN_FID_GETPLAN */
    { ULN_DBC_COMMON }, /* ULN_FID_XA */

    /* PROJ-2047 Strengthening LOB - Added Interfaces */
    {
        /* ULN_FID_TRIMLOB */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_08003},
        {ULN_SFID_00, ULN_EI_08003}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },
    {
        /* ULN_FID_FOR_SD */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_08003},
        {ULN_SFID_00, ULN_EI_08003}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}
    },
};

/*
 * STMT handle's state transition table
 */

#define ULN_STMT_NOT_IMPLEMENTED \
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, \
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, \
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, \
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}, \
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_IH000}

ulnStateTblEntry ulnStateStmtTbl[ULN_FID_MAX][ULN_MAX_STMT_STATE] =
{
    {
        /* ULN_FID_ALLOCHANDLE */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}
    },

    {
        /* ULN_FID_BINDCOL */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_BINDPARAMETER */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_BROWSECONNECT */

    /* PROJ-1789 Updatable Scrollable Cursor */
    {
        /* ULN_FID_BULKOPERATIONS */
        {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_02, ULN_EI_00000}, {ULN_SFID_02, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_03, ULN_EI_HY010}, {ULN_SFID_03, ULN_EI_HY010}, {ULN_SFID_03, ULN_EI_HY010},
        /* Async is not Implemented */
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    /* PROJ-2177 User Interface - Cancel
     * http://nok.altibase.com/x/jqCM */
    {
        /* ULN_FID_CANCEL */
        {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_03, ULN_EI_00000}, {ULN_SFID_03, ULN_EI_00000}, {ULN_SFID_03, ULN_EI_00000},
        /* Async is not Implemented */
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_CLOSECURSOR */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_05, ULN_EI_00000},
        {ULN_SFID_05, ULN_EI_00000}, {ULN_SFID_05, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_COLATTRIBUTE */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_06, ULN_EI_00000},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_07, ULN_EI_00000},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_COLUMNPRIVILEGES */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_CONNECT */

    {
        /* ULN_FID_DRIVERCONNECT */
        {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002},
        {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002},
        {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002},
        {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002},
        {ULN_SFID_00, ULN_EI_08002}, {ULN_SFID_00, ULN_EI_08002}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_COPYDESC */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_DATASOURCES */

    {
        /* ULN_FID_DESCRIBECOL */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_07005},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_07, ULN_EI_00000},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000},
    },

    {
        /* ULN_FID_DESCRIBEPARAM */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_07, ULN_EI_00000},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000},
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_DISCONNECT : 특이케이스. ulnSFID_67() 의 Note 참조 */

    {
        /* ULN_FID_ENDTRAN */
        /*
         * BUGBUG : 만들어야 한다.
         */
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}
    },
    {
        /* ULN_FID_EXECDIRECT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_16, ULN_EI_00000}, {ULN_SFID_17, ULN_EI_00000},
        {ULN_SFID_17, ULN_EI_00000}, {ULN_SFID_18, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}
    },

    {
        /* ULN_FID_EXECUTE */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_19, ULN_EI_00000},
        {ULN_SFID_20, ULN_EI_00000}, {ULN_SFID_21, ULN_EI_00000}, {ULN_SFID_22, ULN_EI_00000},
        {ULN_SFID_23, ULN_EI_00000}, {ULN_SFID_22, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_FETCH, SQLFetchScroll 과 똑같음. */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_24, ULN_EI_00000},
        {ULN_SFID_25, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_FETCHSCROLL, SQLFetch 와 똑같음. */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_24, ULN_EI_00000},
        {ULN_SFID_25, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_EXTENDEDFETCH */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_24, ULN_EI_00000},
        {ULN_SFID_25, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_FOREIGNKEYS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_FREEHANDLE_ENV */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_FREEHANDLE_DBC */
    {
        /* ULN_FID_FREEHANDLE_STMT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_26, ULN_EI_00000}, {ULN_SFID_26, ULN_EI_00000},
        {ULN_SFID_26, ULN_EI_00000}, {ULN_SFID_26, ULN_EI_00000}, {ULN_SFID_26, ULN_EI_00000},
        {ULN_SFID_26, ULN_EI_00000}, {ULN_SFID_26, ULN_EI_00000}, {ULN_SFID_26, ULN_EI_HY010},
        {ULN_SFID_26, ULN_EI_HY010}, {ULN_SFID_26, ULN_EI_HY010},
        {ULN_SFID_26, ULN_EI_HY010}, {ULN_SFID_26, ULN_EI_HY010},

    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_FREEHANDLE_DESC */

    {
        /* ULN_FID_FREESTMT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_27, ULN_EI_00000}, {ULN_SFID_28, ULN_EI_00000},
        {ULN_SFID_28, ULN_EI_00000}, {ULN_SFID_28, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETCONNECTATTR */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETCURSORNAME  */

    {
        /* ULN_FID_GETDATA */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_29, ULN_EI_00000}, {ULN_SFID_29, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_HY010}, {ULN_SFID_01, ULN_EI_HY010}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETDESCFIELD */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETDESCREC */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETDIAGREC,  */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETDIAGFIELD,  */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETENVATTR */

    {
        /* ULN_FID_GETFUNCTIONS */
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETINFO */

    {
        /* ULN_FID_GETSTMTATTR */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_36, ULN_EI_24000}, {ULN_SFID_36, ULN_EI_24000},
        {ULN_SFID_36, ULN_EI_24000}, {ULN_SFID_36, ULN_EI_24000}, {ULN_SFID_36, ULN_EI_24000},
        {ULN_SFID_37, ULN_EI_00000}, {ULN_SFID_38, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_GETSTMTOPTION */

    {
        /* ULN_FID_GETTYPEINFO */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    { 
        /* ULN_FID_MORERESULTS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_39, ULN_EI_00000}, {ULN_SFID_40, ULN_EI_00000},
        {ULN_SFID_40, ULN_EI_00000}, {ULN_SFID_40, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_NATIVESQL */

    {
        /* ULN_FID_NUMPARAMS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_07, ULN_EI_00000},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_07, ULN_EI_00000},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_NUMRESULTCOLS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_07, ULN_EI_00000},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_07, ULN_EI_00000},
        {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_07, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },
    {
        /* ULN_FID_PARAMDATA */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_41, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_42, ULN_EI_00000},
        {ULN_SFID_01, ULN_EI_HY010}, {ULN_SFID_01, ULN_EI_HY010}
    },
    {
        /* ULN_FID_PREPARE */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_43, ULN_EI_00000}, {ULN_SFID_44, ULN_EI_00000},
        {ULN_SFID_44, ULN_EI_00000}, {ULN_SFID_45, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_PRIMARYKEYS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_PROCEDURECOLUMNS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_PROCEDURES */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_PUTDATA */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_46, ULN_EI_00000}, {ULN_SFID_47, ULN_EI_00000},
        {ULN_SFID_01, ULN_EI_HY010}, {ULN_SFID_01, ULN_EI_HY010}
    },
    {
        /* ULN_FID_ROWCOUNT */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    /* SetConnectAttr() 함수의 STMT 상태전이표는 ul 구현상 타지 않는다.
     * 구조상 문제는 아니고, 애초에 DBC 함수가 STMT 상태전이표를 가진게 좀 이상한 듯.
     * 추후 필요하면 DBC 상태전이표 C5에서 처리하면 된다. */
    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_SETCONNECTATTR */

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_SETCURSORNAME */

    {
        /* ULN_FID_SETDESCFIELD */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_SETDESCREC */
#if 0
    {
        /* ULN_FID_SETDESCREC : ULN_FID_SETDESCFIELD 와 완전히 같음. 함수 구현하면 주석 풀고,
         * 윗줄 지우기. */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },
#endif

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_SETENVATTR */

    /* PROJ-1789 Updatable Scrollable Cursor
     * http://nok.altibase.com/x/jqCM */
    {
        /* ULN_FID_SETPOS */
        {ULN_SFID_00, ULN_EI_IH000},
        {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_49, ULN_EI_00000}, {ULN_SFID_49, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        /* Async is not Implemented */
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_SETSTMTATTR */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_50, ULN_EI_HY011},
        {ULN_SFID_50, ULN_EI_HY011}, {ULN_SFID_50, ULN_EI_24000}, {ULN_SFID_50, ULN_EI_24000},
        {ULN_SFID_50, ULN_EI_24000}, {ULN_SFID_50, ULN_EI_24000}, {ULN_SFID_51, ULN_EI_00000},
        {ULN_SFID_51, ULN_EI_00000}, {ULN_SFID_51, ULN_EI_00000},
        {ULN_SFID_51, ULN_EI_00000}, {ULN_SFID_51, ULN_EI_00000}
    },

    {
        /* ULN_FID_SPECIALCOLUMNS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_STATISTICS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_TABLEPRIVILEGES */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_TABLES */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_TRANSACT */

    {
        /* ULN_FID_COLUMNS */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_08, ULN_EI_00000}, {ULN_SFID_09, ULN_EI_00000},
        {ULN_SFID_09, ULN_EI_00000}, {ULN_SFID_10, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_24000},
        {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_24000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_01, ULN_EI_00000}, {ULN_SFID_01, ULN_EI_00000}
    },

    {
        /* ULN_FID_BINDFILETOCOL */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_BINDFILETOPARAM */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_GETLOB */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_PUTLOB */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_GETLOBLENGTH */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_FREELOB */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },

    {
        /* ULN_FID_GETPLAN : BUGBUG 우선 무조건 통과 */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}
    },

    { ULN_STMT_NOT_IMPLEMENTED }, /* ULN_FID_XA */

    {
        /* ULN_FID_TRIMLOB */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },
    {
        /* ULN_FID_FOR_SD */
        {ULN_SFID_00, ULN_EI_IH000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_00000}, {ULN_SFID_00, ULN_EI_00000},
        {ULN_SFID_00, ULN_EI_HY010}, {ULN_SFID_00, ULN_EI_HY010}
    },
};

ulnStateFunc *ulnStateGetStateFunc(ulnFnContext *aFnContext)
{
    ulnStateFuncId sStateFuncID = ULN_SFID_00;
    acp_sint16_t   sCurrentState;

    /*
     * If there comes a parent object that we have to take into an account,
     * we do the checking(or shifting state) on the parent object first.
     */

    sCurrentState = aFnContext->mHandle.mObj->mState;

    switch (aFnContext->mHandle.mObj->mType)
    {
        case ULN_OBJ_TYPE_ENV:
            aFnContext->mUlErrorCode = ulnStateEnvTbl[aFnContext->mFuncID][sCurrentState].mErrCode;
            sStateFuncID             = ulnStateEnvTbl[aFnContext->mFuncID][sCurrentState].mFuncId;
            break;

        case ULN_OBJ_TYPE_DBC:
            aFnContext->mUlErrorCode = ulnStateDbcTbl[aFnContext->mFuncID][sCurrentState].mErrCode;
            sStateFuncID             = ulnStateDbcTbl[aFnContext->mFuncID][sCurrentState].mFuncId;
            break;

        case ULN_OBJ_TYPE_STMT:
            aFnContext->mUlErrorCode = ulnStateStmtTbl[aFnContext->mFuncID][sCurrentState].mErrCode;
            sStateFuncID             = ulnStateStmtTbl[aFnContext->mFuncID][sCurrentState].mFuncId;
            break;

        case ULN_OBJ_TYPE_DESC:
            /*
             * Note : DESC 의 경우는 상태전이 필요없다. 그냥 죽죽 통과시키고,
             *        SQLFreeHandle() 시에만 수작업으로 좀 수정해 주자.
             */
            sStateFuncID             = ULN_SFID_00;
            aFnContext->mUlErrorCode = ulERR_IGNORE_NO_ERROR;
            break;

        default:
            ACE_ASSERT(0);
            break;
    }

    return ulnStateFuncTbl[sStateFuncID];
}

void ulnUpdateDeferredState(ulnFnContext *aFnContext, ulnStmt *aStmt)
{
    if (aStmt->mDeferredPrepareStateFunc != NULL)
    {
        aFnContext->mStateFunc = ulnStateGetStateFunc(aFnContext);

        aStmt->mDeferredPrepareStateFunc = NULL;
    }
}


/*
 * Common entry point function.
 *
 * First, it locks the object of the context.
 * This function checks the object's validity.
 * If it's valid, it cleans up its diagnostic records.
 * And then it decides whether the function which called this can proceed or not by
 * checking the state transition table and calling approperiate state function.
 */
ACI_RC ulnEnter(ulnFnContext *aFnContext, void *aArgs)
{
    acp_rc_t sRc;
    ULN_FLAG(sNeedUnlock);

    ulnObjType    sObjectType;
    ulnShardCoordFixCtrlContext * sShardCoordFixCtrlCtx = NULL;

    /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
    ulnDbc *sDbc = NULL;

    /*
     * BUGBUG : 상위 객체의 상태전이를 고려해야 한다.
     */

    aFnContext->mArgs = aArgs;

    ACI_TEST_RAISE(aFnContext->mHandle.mObj == NULL, LABEL_INVALID_HANDLE);

    /*
     * 아래의 두 값이 일치하는가의 체크만으로 INVALID HANDLE 판단은 충분하다 :
     *
     *      a. 주어진 객체의   object type (sObjectType)
     *      b. 함수가 기대하는 object type (aFnContext->mObjType)
     */

    sObjectType = aFnContext->mHandle.mObj->mType;
    ACI_TEST_RAISE(aFnContext->mObjType != sObjectType, LABEL_INVALID_HANDLE);

    /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
    ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );
    if ( sDbc != NULL )
    {
        if ( ( sDbc->mAttrForceUnlock == 1 ) && 
             ( ( aFnContext->mFuncID == ULN_FID_FREEHANDLE_STMT ) ||
               ( aFnContext->mFuncID == ULN_FID_ENDTRAN ) ||
               ( aFnContext->mFuncID == ULN_FID_DISCONNECT ) ) )
        {
            acpThrMutexTryLock(aFnContext->mHandle.mObj->mLock);
            /* BUG-36775 Codesonar warning : Double Unlock */
            ULN_FLAG_DOWN(aFnContext->mNeedUnlock);
            ULN_OBJECT_UNLOCK(aFnContext->mHandle.mObj, aFnContext->mFuncID);
        }
        else
        {
            /* do nothing */
        }

        sShardCoordFixCtrlCtx = ulnDbcGetShardCoordFixCtrlContext( sDbc );
        if ( sShardCoordFixCtrlCtx != NULL )
        {
            ulnDbcShardCoordFixCtrlEnter( aFnContext, sShardCoordFixCtrlCtx );
        }
    }
    else
    {
        /* do nothing */
    }

    /*
     * ====================
     * LOCK
     * ====================
     */

    /* PROJ-2177 User Interface - Cancel
     * Cancel은 Execute 중에도 쓸 수 있어야 하므로 Lock을 잡지 않는다.
     * 단, NEED DATA 처리 중일때는 상태가 꼬이는걸 막기 위해서 Lock을 잡는다. */
    if ((aFnContext->mFuncID != ULN_FID_CANCEL)
     || (ulnStmtGetNeedDataFuncID(aFnContext->mHandle.mStmt) != ULN_FID_NONE))
    {
        sRc = acpThrMutexTryLock(aFnContext->mHandle.mObj->mLock);
        if ( sRc == ACP_RC_EBUSY )
        {
            /* BUG-48253 동일한 thread가 lock을 중복 시도할 경우 error 처리 */
#if defined(ALTI_CFG_OS_LINUX)
            ACI_TEST_RAISE( aFnContext->mHandle.mObj->mLock->mMutex.__data.__owner == syscall(SYS_gettid), LABEL_INVALID_LOCK );
#endif
        }
        
        if ( sRc != ACP_RC_SUCCESS )
        {
            ULN_OBJECT_LOCK(aFnContext->mHandle.mObj, aFnContext->mFuncID);
        }
        ULN_FLAG_UP(aFnContext->mNeedUnlock);
        ULN_FLAG_UP(sNeedUnlock);

        /* BUG-38755 Improve to check invalid handle in the CLI */
        sObjectType = aFnContext->mHandle.mObj->mType;
        ACI_TEST_RAISE(aFnContext->mObjType != sObjectType, LABEL_INVALID_HANDLE);
    }

    ACI_TEST_RAISE(ulnClearDiagnosticInfoFromObject(aFnContext->mHandle.mObj) != ACI_SUCCESS,
                   LABEL_MEM_MAN_ERR);

    /*
     * ===============================
     * Enter 상태전이 BEGIN
     * ===============================
     */

    aFnContext->mWhere     = ULN_STATE_ENTRY_POINT;
    aFnContext->mStateFunc = ulnStateGetStateFunc(aFnContext);
    ACI_TEST(aFnContext->mStateFunc(aFnContext) != ACI_SUCCESS);

    /*
     * ===============================
     * Enter 상태전이 END
     * ===============================
     */

    aFnContext->mHandle.mObj->mExecFuncID = aFnContext->mFuncID;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnEnter");
    }

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LOCK)
    {
        /* BUG-48253 이미 lock이 되어 있으므로 unlock이 필요함. */
        ULN_FLAG_UP(sNeedUnlock);

        /* BUG-48253 socket close */
        if ( sDbc != NULL)
        {
            ulnDbcFreeLink(sDbc);
            ulnDbcSetIsConnected(sDbc, ACP_FALSE);
        }

        /* BUG-48253 error seting */
        ulnError(aFnContext, ulERR_ABORT_LOCK_SEQUENCE_ERR);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedUnlock)
    {
        ULN_FLAG_DOWN(aFnContext->mNeedUnlock); /* PROJ-2177 */
        ULN_OBJECT_UNLOCK(aFnContext->mHandle.mObj, aFnContext->mFuncID);
    }

    if ( aFnContext->mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( aFnContext );
    }

    return ACI_FAILURE;
}

/*
 * Common exit point function.
 *
 * This function conducts the state transition by calling ulnStateMachine().
 * And then it releases the lock previously held by ulnEnter()
 */
ACI_RC ulnExit(ulnFnContext *aFnContext)
{
    ulnFunctionId sExecFuncID;

    /*
     * Note : exit point 에서 state function 은 에러를 리턴하지 않는다.
     *        ACI_TEST() 를 없애도 될 텐데..
     */

    /*
     * ===============================
     * Exit 상태전이 BEGIN
     * ===============================
     */

    aFnContext->mWhere = ULN_STATE_EXIT_POINT;
    ACI_TEST(aFnContext->mStateFunc(aFnContext) != ACI_SUCCESS);

    /*
     * ===============================
     * Exit 상태전이 END
     * ===============================
     */

    /* PROJ-2177: Cancel일 때는 Lock을 안잡을 때도 있다. */
    ULN_IS_FLAG_UP(aFnContext->mNeedUnlock)
    {
        aFnContext->mHandle.mObj->mExecFuncID = ULN_FID_NONE;

        ULN_FLAG_DOWN(aFnContext->mNeedUnlock);
        ULN_OBJECT_UNLOCK(aFnContext->mHandle.mObj, aFnContext->mFuncID);
    }

    /*
     * ====================
     * UNLOCK DONE
     * ====================
     */

    if ( aFnContext->mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( aFnContext );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    sExecFuncID                           = aFnContext->mHandle.mObj->mExecFuncID;
    aFnContext->mHandle.mObj->mExecFuncID = ULN_FID_NONE;

    /* PROJ-2177: Cancel일 때는 Lock을 안잡을 때도 있다. */
    ULN_IS_FLAG_UP(aFnContext->mNeedUnlock)
    {
        ULN_FLAG_DOWN(aFnContext->mNeedUnlock);
        ULN_OBJECT_UNLOCK(aFnContext->mHandle.mObj, sExecFuncID);
    }

    if ( aFnContext->mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( aFnContext );
    }

    return ACI_FAILURE;
}

