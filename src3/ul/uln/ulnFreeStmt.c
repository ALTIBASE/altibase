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
#include <ulnFreeStmt.h>

/*
 * ULN_SFID_27
 * SQLFreeStmt(), STMT, S4
 *      S1 [np] and [1]
 *      S2 [p] and [1]
 *      -- [2]
 *  where
 *      [1]  This row shows transitions when Option was SQL_CLOSE.
 *      [2]  This row shows transitions when Option was SQL_UNBIND or SQL_RESET_PARAMS.
 *
 *      [np] Not prepared. The statement was not prepared.
 *      [p]  Prepared. The statement was prepared.
 */
ACI_RC ulnSFID_27(ulnFnContext *aFnContext)
{
    acp_uint16_t sOption;

    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sOption = *(acp_uint16_t *)(aFnContext->mArgs);

        switch (sOption)
        {
            case SQL_CLOSE:
                /* [1] */
                if (ulnStmtIsPrepared(aFnContext->mHandle.mStmt) == ACP_TRUE)
                {
                    /* [p] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
                }
                else
                {
                    /* [np] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
                }
                break;
            case SQL_UNBIND:
            case SQL_RESET_PARAMS:
                /* [2] */
                break;
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_28
 * SQLFreeStmt(), STMT, S5-S7
 *      S1 [np] and [1]
 *      S3 [p] and [1]
 *      -- [2]
 *  where
 *      [1] This row shows transitions when Option was SQL_CLOSE.
 *      [2] This row shows transitions when Option was SQL_UNBIND or SQL_RESET_PARAMS.
 *
 *      [np] Not prepared. The statement was not prepared.
 *      [p]  Prepared. The statement was prepared.
 */
ACI_RC ulnSFID_28(ulnFnContext *aFnContext)
{
    acp_uint16_t sOption;

    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sOption = *(acp_uint16_t *)(aFnContext->mArgs);

        switch (sOption)
        {
            case SQL_CLOSE:
                /* [1] */
                if (ulnStmtIsPrepared(aFnContext->mHandle.mStmt) == ACP_TRUE)
                {
                    /* [p] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
                }
                else
                {
                    /* [np] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
                }
                break;
            case SQL_UNBIND:
            case SQL_RESET_PARAMS:
                /* [2] */
                break;
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_76
 * SQLFreeStmt(), DBC, C6
 *      C5 [3] and [1]
 *      -- [4] and [1]
 *      -- [2]
 *  where
 *      [1] This row shows transitions when Option was SQL_CLOSE.
 *      [2] This row shows transitions when Option was SQL_UNBIND or SQL_RESET_PARAMS.
 *      [3] The connection was in auto-commit mode,
 *          and no cursors were open on any statements except this one.
 *      [4] The connection was in manual-commit mode,
 *          or it was in auto-commit mode and a cursor was open on at least one other statement.
 */
ACI_RC ulnSFID_76(ulnFnContext *aFnContext)
{
    acp_uint16_t sOption;

    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sOption = *(acp_uint16_t *)(aFnContext->mArgs);

        switch (sOption)
        {
            case SQL_CLOSE:
                /* [1] */
                /*
                 * BUGBUG : Parent Object's state transition.
                 */
                break;
            case SQL_UNBIND:
            case SQL_RESET_PARAMS:
                /* [2] */
                break;
        }
    }

    return ACI_SUCCESS;
}

ACI_RC ulnFreeStmtClose(ulnFnContext *aFnContext, ulnStmt *aStmt)
{
    ACI_TEST(ulnCursorClose(aFnContext, &aStmt->mCursor) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnFreeStmtUnbind(ulnFnContext *aFnContext, ulnStmt *aStmt)
{
    ulnDesc      *sDescArd = NULL;
    ulnObject    *sArdParentObject;

    acp_uint32_t  sTempSP = 0;
    acp_list_t    sTempStmtList;

    acpListInit(&sTempStmtList);

    /*
     * Note : IRD 정보는 서버에서 받아온 정보이므로
     *        ODBC 스펙대로 ARD 만 초기화 시킨다.
     */

    sDescArd = ulnStmtGetArd(aStmt);
    ACI_TEST_RAISE(sDescArd == NULL, LABEL_MEM_MAN_ERR);

    sArdParentObject = sDescArd->mParentObject;

    /*
     * Desc 가 explicit desc 이며, 연결된 stmt 들이 존재한다면, 리스트 백업
     */
    if (acpListIsEmpty(&sDescArd->mAssociatedStmtList) != ACP_TRUE)
    {
        ACI_TEST_RAISE(ulnDescSaveAssociatedStmtList(sDescArd,
                                                     sArdParentObject->mMemory,
                                                     &sTempStmtList,
                                                     &sTempSP) != ACI_SUCCESS,
                       LABEL_MEM_MAN_ERR);
    }

    /*
     * Desc 를 갓 생성된 상태로 되돌리기
     */
    ACI_TEST_RAISE(ulnDescRollBackToInitial(sDescArd) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
    ACI_TEST_RAISE(ulnDescInitialize(sDescArd, sArdParentObject) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    /*
     * Note : number of result columns 는 초기화되면 안된다. !!!!!!!!!!!!!!
     */

    /*
     * 복사해 둔 associated stmt 가 존재하면 desc 에 원복시킨다.
     */
    if (acpListIsEmpty(&sTempStmtList) != ACP_TRUE)
    {
        ACI_TEST_RAISE(ulnDescRecoverAssociatedStmtList(sDescArd,
                                                        sArdParentObject->mMemory,
                                                        &sTempStmtList,
                                                        sTempSP) != ACI_SUCCESS,
                       LABEL_MEM_MAN_ERR);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "FreeStmtUnbind");
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnFreeStmtUnbind");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnFreeStmtResetParams(ulnFnContext *aFnContext, ulnStmt *aStmt)
{
    ulnDesc     *sDescApd = NULL;
    ulnDesc     *sDescIpd = NULL;
    ulnObject   *sApdParentObject;
    ulnObject   *sIpdParentObject;

    acp_uint32_t sTempSP = 0;
    acp_list_t   sTempStmtList;

    acpListInit(&sTempStmtList);

    /*
     * Note : APD, IPD 는 동시에 생기는 데다가, 서버의 정보와는 전혀 무관하므로
     *        함께 없애 줘야 한다.
     */

    sDescApd = ulnStmtGetApd(aStmt);
    ACI_TEST_RAISE(sDescApd == NULL, LABEL_MEM_MAN_ERR);

    sDescIpd = ulnStmtGetIpd(aStmt);
    ACI_TEST_RAISE(sDescIpd == NULL, LABEL_MEM_MAN_ERR);

    sIpdParentObject = sDescIpd->mParentObject;
    sApdParentObject = sDescApd->mParentObject;

    /*
     * Desc 가 explicit desc 이며, 연결된 stmt 들이 존재한다면, 리스트 백업
     */

    if (acpListIsEmpty(&sDescApd->mAssociatedStmtList) != ACP_TRUE)
    {
        ACI_TEST_RAISE(ulnDescSaveAssociatedStmtList(sDescApd,
                                                     sApdParentObject->mMemory,
                                                     &sTempStmtList,
                                                     &sTempSP) != ACI_SUCCESS,
                       LABEL_MEM_MAN_ERR);
    }

    /*
     * Desc 를 갓 생성된 상태로 되돌리기
     */
    ACI_TEST_RAISE(ulnDescRollBackToInitial(sDescApd) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
    ACI_TEST_RAISE(ulnDescInitialize(sDescIpd, sIpdParentObject) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    ACI_TEST_RAISE(ulnDescRollBackToInitial(sDescIpd) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
    ACI_TEST_RAISE(ulnDescInitialize(sDescApd, sApdParentObject) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    /*
     * BUGBUG : 파라미터 카운트도 초기화 되어야 한다.
     *          초기화시키면 안될거 같등
     */
    // ulnStmtSetParamCount(aStmt, 0);

    /*
     * 복사해 둔 associated stmt 가 존재하면 desc 에 원복시킨다.
     */
    if (acpListIsEmpty(&sTempStmtList) != ACP_TRUE)
    {
        ACI_TEST_RAISE(ulnDescRecoverAssociatedStmtList(sDescApd,
                                                        sApdParentObject->mMemory,
                                                        &sTempStmtList,
                                                        sTempSP) != ACI_SUCCESS,
                       LABEL_MEM_MAN_ERR);
    }

    ulnStmtSetExecutedParamSetMaxSize(aStmt, 0);  /* BUG-42096 */

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnFreeStmtResetParams");
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnFreeStmtResetParams");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * ulnFreeStmt
 * SQLFreeStmt() 함수가 곧장 매핑되는 함수이다.
 * 이 함수는 SQLFreeConnect() 라든지 SQLFreeEnv() 처럼 deprecate 되지 않고
 * SQLFreeHandle() 과는 다른 용도로 사용된다.
 *
 * SQL_DROP 옵션으로 호출되면
 * SQLFreeHandle()함수로 곧장 매핑된다.
 * SQL_DROP 옵션이 주어지면 정말 모든 핸들을 프리해버리고 메모리까지
 * 해제한다.
 */
SQLRETURN ulnFreeStmt(ulnStmt *aStmt, acp_uint16_t aOption)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext  sFnContext;

    if (aOption == SQL_DROP)
    {
        /*
         * BUGBUG : 좀 더 우아하게 코드를 바꿔야 한다.
         */
        return ulnFreeHandle(SQL_HANDLE_STMT, (void *)aStmt);
    }
    else
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREESTMT, aStmt, ULN_OBJ_TYPE_STMT);

        ACI_TEST(ulnEnter(&sFnContext, (void *)(&aOption)) != ACI_SUCCESS);
        ULN_FLAG_UP(sNeedExit);

        switch (aOption)
        {
            case SQL_CLOSE:
                ACI_TEST(ulnFreeStmtClose(&sFnContext, aStmt) != ACI_SUCCESS);
                break;

            case SQL_UNBIND:
                ACI_TEST(ulnFreeStmtUnbind(&sFnContext, aStmt) != ACI_SUCCESS);
                break;

            case SQL_RESET_PARAMS:
                ACI_TEST(ulnFreeStmtResetParams(&sFnContext, aStmt) != ACI_SUCCESS);
                break;

            default:
                /* Option type out of range */
                ACI_RAISE(LABEL_UNKNOWN_OPTION);
                break;
        }

        ULN_FLAG_DOWN(sNeedExit);
        ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_UNKNOWN_OPTION)
    {
        /*
         * HY092
         */
        ulnError(&sFnContext, ulERR_ABORT_INVALID_ATTR_OPTION, aOption);
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
