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
#include <ulnAllocHandle.h>

/*
 * State transition functions
 *
 * AllocHandle 의 경우 ulnFnContext 의 *mArgs 에 SShort aHandleType 이 넘어온다.
 */

/*
 * ULN_SFID_90
 *      env
 *          --[4]
 *      dbc
 *          E2[5]
 *          (HY010)[6]
 *      stmt and desc
 *          (IH)
 *  where
 *      [4] Calling SQLAllocHandle with OutputHandlePtr pointing to a valid handle overwrites
 *          that handle. This might be an application programming error.
 *      [5] The SQL_ATTR_ODBC_VERSION environment attribute had been set on the environment.
 *      [6] The SQL_ATTR_ODBC_VERSION environment attribute had NOT been set on the environment.
 */
ACI_RC ulnSFID_90(ulnFnContext *aContext)
{
    acp_sint16_t sHandleType;

    sHandleType = *(acp_sint16_t *)(aContext->mArgs);

    switch(sHandleType)
    {
        case SQL_HANDLE_ENV:
            break;

        case SQL_HANDLE_DBC:
            if(ulnEnvGetOdbcVersion(aContext->mHandle.mEnv) == 0)
            {
                /*
                 * [6] ODBC version's NOT been set
                 */
                ACI_RAISE(LABEL_FUNC_SEQUENCE_ERR);
            }
            else
            {
                if(aContext->mWhere == ULN_STATE_EXIT_POINT)
                {
                    ULN_OBJ_SET_STATE(aContext->mHandle.mEnv, ULN_S_E2);
                }
            }
            break;

        case SQL_HANDLE_STMT:
        case SQL_HANDLE_DESC:
            ACI_RAISE(LABEL_INVALID_HANDLE);
            break;

        default:
            ACI_RAISE(LABEL_INVALID_HANDLE);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_FUNC_SEQUENCE_ERR)
    {
        ulnError(aContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(aContext, ulERR_ABORT_INVALID_HANDLE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_60
 * SQLAllocHandle, DBC, C2, C3
 */
ACI_RC ulnSFID_60(ulnFnContext *aContext)
{
    acp_sint16_t sHandleType;

    sHandleType = *(acp_sint16_t *)(aContext->mArgs);

    switch(sHandleType)
    {
        case SQL_HANDLE_ENV:
        case SQL_HANDLE_DBC:
            break;

        case SQL_HANDLE_STMT:
        case SQL_HANDLE_DESC:
            ACI_RAISE(LABEL_NO_CONNECTION);
            break;

        default:
            ACI_RAISE(LABEL_INVALID_HANDLE_TYPE);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NO_CONNECTION)
    {
        /*
         * 08003
         */
        ulnError(aContext, ulERR_ABORT_NO_CONNECTION, "");
    }

    ACI_EXCEPTION(LABEL_INVALID_HANDLE_TYPE)
    {
        /*
         * HY092
         */
        ulnError(aContext, ulERR_ABORT_INVALID_ATTR_OPTION, sHandleType);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_61
 */
ACI_RC ulnSFID_61(ulnFnContext *aContext)
{
    acp_sint16_t sHandleType;

    if(aContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sHandleType = *(acp_sint16_t *)(aContext->mArgs);

        switch(sHandleType)
        {
            case SQL_HANDLE_ENV:
            case SQL_HANDLE_DBC:
            case SQL_HANDLE_DESC:
                break;

            case SQL_HANDLE_STMT:
                ULN_OBJ_SET_STATE(aContext->mHandle.mDbc, ULN_S_C5);
                break;

            default:
                ACI_RAISE(LABEL_INVALID_HANDLE_TYPE);
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_HANDLE_TYPE)
    {
        /*
         * HY092
         */
        ulnError(aContext, ulERR_ABORT_INVALID_ATTR_OPTION, sHandleType);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnAllocHandleCheckArgs(ulnFnContext *aContext,
                                      ulnObjType    aObjType,
                                      ulnObject    *aInputHandle,
                                      ulnObject   **aOutputHandlePtr)
{
    ACI_TEST_RAISE( (aInputHandle == NULL && aObjType != ULN_OBJ_TYPE_ENV), LABEL_INVALID_HANDLE );

    ACI_TEST_RAISE( aOutputHandlePtr == NULL, LABEL_INVALID_USE_OF_NULL );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        /*
         * Invalid Handle
         */
        ulnError(aContext, ulERR_ABORT_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        /*
         * HY009 : Invalid Use of Null Pointer
         */
        ulnError(aContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * ulnAllocEnv.
 *
 * ENV 객체를 하나 할당해서 그 포인터를 리턴한다.
 *
 * @param[in] aContext
 *  ulnAllocHandle() 의 Context 를 가리키는 포인터.
 *  이 Context 에 상위 핸들을 가리키는 포인터가 들어 있다.
 *  이 함수의 경우, 상위핸들이 없으므로 aContext->mHandle.mObject 에 NULL 이 들어 있다.
 * @param[out] aOutputHandlePtr
 *  함수 실행 결과로 할당된 ENV 객체의 포인터가 저장될 변수의 포인터.
 */
static SQLRETURN ulnAllocHandleEnv(ulnEnv **aOutputHandlePtr)
{
    ulnEnv *sEnv;
    /*
     * ENV 는 없던걸 만드는 것이므로 Context 가 필요 없다.
     */

    /*
     * ENV의 인스턴스 생성
     */
    ACI_TEST(ulnEnvCreate(&sEnv) != ACI_SUCCESS);

    /*
     * BUGBUG: 인스턴스를 생성하고 곧장 lock 해야 하지 않을까?
     */

    /*
     * ulnEnv 의 나머지 부분의 초기화 (디폴트 값으로)
     */
    ulnEnvInitialize(sEnv);

    /* initialize the CM module */
    //!BUGBUG!//  IDE_ASSERT(cmiInitialize() == ACI_SUCCESS);

    *aOutputHandlePtr = sEnv;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

/**
 * ulnAllocDbc.
 *
 * DBC 객체를 하나 할당해서 그 포인터를 리턴한다.
 *
 * @param[in] aContext
 *  ulnAllocHandle() 의 Context 를 가리키는 포인터.
 *  이 Context 에 상위 핸들을 가리키는 포인터가 들어 있다.
 * @param[out] aOutputHandlePtr
 *  할당된 DBC 객체의 포인터가 저장될 변수의 포인터
 *
 * MSDN ODBC 3.0 :
 * Application 은, 에러에 대한 정보를 상위 핸들인
 * InputHandle 에 매달려 있는 Diagnostic 구조체로부터
 * 얻어올 수 있다.
 * --> 즉, 상위핸들의 DiagHeader 에 발생한 에러를 넣어 주어야 한다.
 */
static SQLRETURN ulnAllocHandleDbc(ulnEnv *aEnv, ulnDbc **aOutputHandlePtr)
{
    ulnDbc       *sDbc;
    ulnFnContext  sFnContext;

    acp_bool_t    sNeedExit   = ACP_FALSE;
    acp_sint16_t  sHandleType = SQL_HANDLE_DBC;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext,
                              ULN_FID_ALLOCHANDLE,
                              aEnv,
                              ULN_OBJ_TYPE_ENV);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(&sHandleType)) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    /*
     * Check if arguments are valid.
     */
    ACI_TEST(ulnAllocHandleCheckArgs(&sFnContext,
                                     ULN_OBJ_TYPE_DBC,
                                     (ulnObject *)aEnv,
                                     (ulnObject **)aOutputHandlePtr) != ACI_SUCCESS);

    /*
     * DBC 인스턴스 생성
     */
    // BUG-25315 [CodeSonar] 초기화되지 않는 변수 사용
    ACI_TEST_RAISE(ulnDbcCreate(&sDbc) != ACI_SUCCESS, MEMORY_ERROR);

    /*
     * DBC 인스턴스의 초기화
     */
    // BUG-25315 [CodeSonar] 초기화되지 않는 변수 사용
    ACI_TEST_RAISE(ulnDbcInitialize(&sFnContext, sDbc) != ACI_SUCCESS, INIT_ERROR);

    /*
     * DBC 를 ENV 에 매달기
     */
    ulnEnvAddDbc(sFnContext.mHandle.mEnv, sDbc);

    /*
     * Inheration Of ODBC Version, but could be overvrite
     * by Connection String !!!
     */
    sDbc->mOdbcVersion = (acp_uint32_t)sFnContext.mHandle.mEnv->mOdbcVersion;

    /* 
     * BUG-35332 The socket files can be moved
     *
     * 환경변수 or 프로퍼티 파일에서 읽은 값으로 초기화한다.
     */
    ulnStrCpyToCStr( sDbc->mUnixdomainFilepath,
                     IPC_FILE_PATH_LEN,
                     &aEnv->mProperties.mUnixdomainFilepath );

    ulnStrCpyToCStr( sDbc->mIpcFilepath,
                     IPC_FILE_PATH_LEN,
                     &aEnv->mProperties.mIpcFilepath );
    
    ulnStrCpyToCStr( sDbc->mIPCDAFilepath,
                     IPC_FILE_PATH_LEN,
                     &aEnv->mProperties.mIPCDAFilepath );
    sDbc->mIPCDAMicroSleepTime = aEnv->mProperties.mIpcDaClientSleepTime;
    sDbc->mIPCDAULExpireCount  = aEnv->mProperties.mIpcDaClientExpireCount;

    *aOutputHandlePtr = sDbc;

    /*
     * Exit
     */
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    // BUG-25315 [CodeSonar] 초기화되지 않는 변수 사용
    ACI_EXCEPTION(MEMORY_ERROR);
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnAllocHandleDbc");
    }
    ACI_EXCEPTION(INIT_ERROR);
    {
        ulnDbcDestroy(sDbc);
    }
    ACI_EXCEPTION_END;

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

/**
 * ulnAllocStmt.
 *
 * STMT 객체를 하나 할당해서 그 포인터를 넘겨준다.
 *
 * @param[in] aContext
 *  ulnAllocHandle() 의 Context 를 가리키는 포인터.
 *  이 Context 에 상위 핸들을 가리키는 포인터가 들어 있다.
 * @param[out] aOutputHandlePtr
 *  할당된 STMT 객체의 포인터가 저장될 변수의 포인터
 *
 * 함수 수행시 발생한 에러는 상위 핸들인 DBC 개게의 Diagnostic
 * 구조체에다가 매달아 주어야 한다.
 */
static SQLRETURN ulnAllocHandleStmt(ulnDbc *aDbc, ulnStmt **aOutputHandlePtr)
{
    ulnDbc       *sDbc;
    ulnStmt      *sStmt;
    ulnFnContext  sFnContext;

    acp_bool_t    sNeedExit   = ACP_FALSE;
    acp_sint16_t  sHandleType = SQL_HANDLE_STMT;

    /* PROJ-1573 XA */
    sDbc = ulnDbcSwitch(aDbc);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext,
                              ULN_FID_ALLOCHANDLE,
                              sDbc,
                              ULN_OBJ_TYPE_DBC);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(&sHandleType)) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;
    /*
     * Check if arguments are valid.
     */
    ACI_TEST(ulnAllocHandleCheckArgs(&sFnContext,
                                     ULN_OBJ_TYPE_STMT,
                                     (ulnObject *)sDbc,
                                     (ulnObject **)aOutputHandlePtr) != ACI_SUCCESS);

    /* PROJ-2177 Stmt 개수 제한 */
    ACI_TEST_RAISE(ulnDbcGetStmtCount(sDbc) >= ULN_DBC_MAX_STMT, TOO_MANY_STMT_ERROR);

    /*
     * ulnStmt 인스턴스 생성
     */
    // BUG-25315 [CodeSonar] 초기화되지 않는 변수 사용
    ACI_TEST_RAISE(ulnStmtCreate(sDbc, &sStmt) != ACI_SUCCESS, MEMORY_ERROR);

    /*
     * ulnStmt초기화
     */
    ulnStmtInitialize(sStmt);

    /*
     * DBC 에 매달기
     */
    ulnDbcAddStmt(sDbc, sStmt);

    /* PROJ-2616 IPCDA Cache Buffer 생성하기 */
    ulnCacheCreateIPCDA(&sFnContext, sDbc);

    /* 필요한 DBC 의 속성들을 상속받기 */
    ulnStmtSetAttrPrefetchRows(sStmt, ulnDbcGetAttrFetchAheadRows(sStmt->mParentDbc));
    ulnStmtSetAttrDeferredPrepare(sStmt, ulnDbcGetAttrDeferredPrepare(sStmt->mParentDbc));

    *aOutputHandlePtr = sStmt;

    /* Exit */
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* PROJ-2177 Stmt 개수 제한 */
    ACI_EXCEPTION(TOO_MANY_STMT_ERROR);
    {
        ulnError(&sFnContext, ulERR_ABORT_TOO_MANY_STATEMENT);
    }
    // BUG-25315 [CodeSonar] 초기화되지 않는 변수 사용
    ACI_EXCEPTION(MEMORY_ERROR);
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "AllocHandleStmt");
    }
    ACI_EXCEPTION_END;

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }
    
    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

/**
 * ulnAllocHandleDesc.
 *
 * @param[in] aContext
 *  ulnAllocHandle() 의 Context 를 가리키는 포인터.
 *  이 Context 에 상위 핸들을 가리키는 포인터가 들어 있다.
 * @param[out] aOutputHandlePtr
 *  할당된 DESC 객체의 포인터가 저장될 변수의 포인터
 *
 * 디스크립터를 할당한다.
 * 이 경우는 사용자가 SQLAllocHandle()함수를 이용해서 Explicit하게 할당하는 경우이다.
 * 사용자는 APD나 ARD만 생성할 수 있다.
 */
static SQLRETURN ulnAllocHandleDesc(ulnDbc *aDbc, ulnDesc **aOutputHandlePtr)
{
    ulnDbc       *sDbc;
    ulnDesc      *sDesc;
    ulnFnContext  sFnContext;

    acp_bool_t    sNeedExit   = ACP_FALSE;
    acp_sint16_t  sHandleType = SQL_HANDLE_DESC;

    /* PROJ-1573 XA */

    sDbc = ulnDbcSwitch(aDbc);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext,
                              ULN_FID_ALLOCHANDLE,
                              sDbc,
                              ULN_OBJ_TYPE_DBC);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(&sHandleType)) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    /*
     * ulnDesc 생성, 초기화
     */
    ACI_TEST_RAISE(ulnDescCreate(sFnContext.mHandle.mObj,
                                 &sDesc,
                                 ULN_DESC_TYPE_ARD, /* BUGBUG: 일단 ARD 로 타입을 잡자  */
                                 ULN_DESC_ALLOCTYPE_EXPLICIT) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    ACI_TEST_RAISE(ulnDescInitialize(sDesc, sFnContext.mHandle.mObj) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM_ROLLBACK);
    ulnDescInitializeUserPart(sDesc);

    /*
     * DBC 에 매달기
     */
    ulnDbcAddDesc(sFnContext.mHandle.mDbc, sDesc);

    /*
     * Note: DESC 의 상태 전이는 필요 없다. 할당 성공하면 무조건 1
     */

    *aOutputHandlePtr = sDesc;

    /*
     * Exit
     */
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM_ROLLBACK)
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "AllocHandleDesc rollback");

        if(ulnDescDestroy(sDesc) != ACI_SUCCESS)
        {
            ulnError(&sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "AllocHandleDesc rollback");
        }
    }

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "AllocHandleDesc");
    }

    ACI_EXCEPTION_END;

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

/**
 * ulnAllocHandle.
 *
 * SQLAllocHandle 이 곧장 매핑되는 함수이다.
 * ODBC 3.0 이전의 스펙에서 사용되던 SQLAllocEnv, SQLAllocDbc, SQLAllocStmt등의
 * 함수들은 각각 ulnAllocEnv와 같은 함수를 따로 부르지 말고 ulnAllocHandle 함수를
 * 호출하는 방식으로 매핑이 이루어져야 하겠다.
 * 즉, 이 함수는 모든 SQLAllocXXXX 함수의 uln 으로의 엔트리 포인트라 하겠다.
 */
SQLRETURN ulnAllocHandle(acp_sint16_t   aHandleType,
                         void          *aInputHandle,
                         void         **aOutputHandlePtr)
{
    SQLRETURN sReturnCode;
    acp_char_t  sTypeStr[80];

	switch(aHandleType)
	{
        case SQL_HANDLE_ENV:
            sReturnCode = ulnAllocHandleEnv((ulnEnv**)aOutputHandlePtr);
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "env");
            break;

        case SQL_HANDLE_DBC:
            sReturnCode = ulnAllocHandleDbc((ulnEnv *)aInputHandle, (ulnDbc**)aOutputHandlePtr);
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "dbc");
            break;

        case SQL_HANDLE_STMT:
            sReturnCode = ulnAllocHandleStmt((ulnDbc *)aInputHandle, (ulnStmt **)aOutputHandlePtr);
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "stmt");
            break;

        case SQL_HANDLE_DESC:
            sReturnCode = ulnAllocHandleDesc((ulnDbc *)aInputHandle, (ulnDesc **)aOutputHandlePtr);
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "desc");
            break;

        default:
            /*
             * HY092 : Invalid Attribute/Option Identifier
             *
             * BUGBUG:
             * 여기서는 context 가 없으므로 객체가 유효한 경우에 잠시 lock 을 하고,
             * 그런 후 직접 diagnostic record 를 매어달도록 하자.
             */
            // ACI_TEST_RAISE(ulnProcessErrorSituation(&sFnContext, 1, ULN_EI_HY092)
                           // != ACI_SUCCESS,
                           // ULN_LABEL_NEED_EXIT);
            sReturnCode = SQL_ERROR;
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "invalid");
            break;
	}

    /*
     * MSDN ODBC 3.0 SQLAllocHandle() Returns 섹션 :
     * 함수가 SQL_ERROR 를 리턴하면 OutputHandlePtr 을 NULL 로 세팅한다.
     */
    if(sReturnCode == SQL_ERROR)
    {
        *aOutputHandlePtr = NULL;
    }

    ULN_TRACE_LOG(NULL, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| %-4s [in: %p out: %p] %"ACI_INT32_FMT,
            "ulnAllocHandle", sTypeStr,
            aInputHandle, *aOutputHandlePtr, sReturnCode);

    return sReturnCode;
}

/**
 * SQL_API_SQLALLOCHANDLESTD (73) - ISO-92 std.
 * AllocateHandle Function, SQL_OV_ODBC3 - default
 */
SQLRETURN ulnAllocHandleStd(acp_sint16_t   aHandleType,
                            void          *aInputHandle,
                            void         **aOutputHandlePtr)
{
    SQLRETURN sRetCode = ulnAllocHandle(aHandleType,
                                        aInputHandle,
                                        aOutputHandlePtr);

    if( SQL_SUCCEEDED(sRetCode) && aHandleType == SQL_HANDLE_ENV)
    {
        (void)ulnEnvSetOdbcVersion( *((ulnEnv**)aOutputHandlePtr), SQL_OV_ODBC3);
    }
    return sRetCode;
}
