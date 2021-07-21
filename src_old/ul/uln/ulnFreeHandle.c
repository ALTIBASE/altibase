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
#include <ulnFreeHandle.h>

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#include <ulnFetchOpAsync.h>

/*
 * State transition functions
 *
 * FreeHandle 의 경우 ulnFnContext 의 *mArgs 에 상위 핸들을 가리키는 포인터가 온다.
 */

/*
 * ULN_SFID_26
 * for ulnFreeHandleStmt, STMT state S1 through S7
 */
ACI_RC ulnSFID_26(ulnFnContext *aContext)
{
    ulnDbc *sDbc;

    /*
     * BUGBUG: This is absurd, 'cause at an exit point, we don't actually have ENV.
     *         It's already been freed.
     */
    if(aContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        // ULN_OBJ_SET_STATE(aContext->mHandle.mStmt, ULN_S_S0);

        sDbc = (ulnDbc *)(aContext->mArgs);

        /*
         * Note: You don't need to lock DBC cause you already have one from STMT.
         */
        if(ULN_OBJ_GET_STATE(sDbc) == ULN_S_C5)
        {
            ACI_TEST(ulnSFID_74(aContext) != ACI_SUCCESS);
        }
        else
        {
            if(ULN_OBJ_GET_STATE(sDbc) == ULN_S_C6)
            {
                ACI_TEST(ulnSFID_75(aContext) != ACI_SUCCESS);
            }
        }

        /*
         * BUGBUG: Gotta modify previous if-else clause using state machine.
         *         But, for now, just do it, cause we're running outta time.
         */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_74
 * for ulnFreeHandleStmt, DBC state C5
 *
 *	    C4[5]
 *	    --[6]
 *	where
 *	    [5] There was only one statement allocated on the connection.
 *	    [6] There were multiple statements allocated on the connection.
 *
 * Note: called directly from ulnSFID_26
 */
ACI_RC ulnSFID_74(ulnFnContext *aContext)
{
    ulnDbc *sDbc;

    if(aContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sDbc = (ulnDbc *)(aContext->mArgs);

        /*
         * Note: You don't NEED to lock DBC because when the STMT uses its parent DBC's lock.
         *       It's already locked.
         */
        if(ulnDbcGetStmtCount(sDbc) == 0)
        {
            /* [5] */
            ULN_OBJ_SET_STATE(sDbc, ULN_S_C4);
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_75
 * for ulnFreeHandleStmt, DBC state C6
 *
 *      --[7]
 *      C4[5] and [8]
 *      C5[6] and [8]
 *  where
 *	    [5] There was only one statement allocated on the connection.
 *	    [6] There were multiple statements allocated on the connection.
 *      [7] The connection was in manual-commit mode.
 *      [8] The connection was in auto-commit mode.
 *
 * Note: called directly from ulnSFID_26
 */
ACI_RC ulnSFID_75(ulnFnContext *aContext)
{
    ulnDbc *sDbc;

    if(aContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sDbc = (ulnDbc *)(aContext->mArgs);

        /*
         * Note: You don't need to lock DBC because STMT uses its parent DBC's lock.
         *       It's already locked.
         */
        if(sDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_ON)
        {
            /* [8] */
            if(ulnDbcGetStmtCount(sDbc) == 0)
            {
                /* [5] */
                ULN_OBJ_SET_STATE(sDbc, ULN_S_C4);
            }
            else
            {
                /* [6] */
                ULN_OBJ_SET_STATE(sDbc, ULN_S_C5);
            }
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_71
 * for SQLFreeHandleDbc, DBC state C2
 */
ACI_RC ulnSFID_71(ulnFnContext *aContext)
{
    /*
     * BUGBUG: This is absurd, 'cause at an exit point, we don't actually have ENV.
     *         It's already been freed.
     */
    if(aContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        // ULN_OBJ_SET_STATE(aContext->mHandle.mDbc, ULN_S_C1);

        /*
         * Do the parent object(ENV)'s state transition.
         * The pointer to the parent object is stored in the context structure's mArgs field.
         */
        ACI_TEST(ulnSFID_95(aContext) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_95
 * for SQLFreeHandleDbc, ENV state E2
 *
 *	    --[4]
 *	    E1[5]
 *	where
 *	    [4] There were other allocated connection handles.
 *	    [5] The connection handle specified in Handle was the only allocated connection handle.
 *
 * Note: Directly called by ulnSFID_71
 */
ACI_RC ulnSFID_95(ulnFnContext *aContext)
{
    ulnEnv *sEnv;

    if(aContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sEnv = (ulnEnv *)(aContext->mArgs);

        /*
         * Set new state onto ENV
         */
        if(ulnEnvGetDbcCount(sEnv) == 0)
        {
            /* [5] */
            ULN_OBJ_SET_STATE(sEnv, ULN_S_E1);
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_94
 * for SQLFreeHandleEnv, ENV state E1
 */
ACI_RC ulnSFID_94(ulnFnContext * aContext)
{
    ACP_UNUSED(aContext);

    /*
     * BUGBUG: This is absurd, 'cause at an exit point, we don't actually have ENV.
     *         It's already been freed.
     */
#if 0
    if(aContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        ULN_OBJ_SET_STATE(aContext->mHandle.mEnv, ULN_S_E0);
    }
#endif

    return ACI_SUCCESS;
}

ACI_RC ulnCallbackFreeResult(cmiProtocolContext *aProtocolContext,
                             cmiProtocol        *aProtocol,
                             void               *aServiceSession,
                             void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

static SQLRETURN ulnFreeHandleEnv(ulnEnv *aEnv)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedUnLock);

    ulnFnContext     sFnContext;
    acp_thr_mutex_t *sLockEnv   = NULL;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREEHANDLE_ENV, aEnv, ULN_OBJ_TYPE_ENV);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * ====================================
     * Function BEGIN
     * ====================================
     */

    sLockEnv = aEnv->mObj.mLock;
    ULN_FLAG_UP(sNeedUnLock);

    /*
     * Env 해제
     */
    ACI_TEST_RAISE(ulnEnvDestroy(aEnv) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);

    /*
     * ====================================
     * Function END
     * ====================================
     */

    ULN_FLAG_DOWN(sNeedUnLock);
    ACI_TEST(acpThrMutexUnlock(sLockEnv) != ACP_RC_SUCCESS);
    ACI_TEST(acpThrMutexDestroy(sLockEnv) != ACP_RC_SUCCESS);
    uluLockDestroy(sLockEnv);

    /*
     * Note : ENV 가 release 되어버렸으므로 ulnExit 를 호출해서는 안된다.
     *        state function 을 곧장 호출해야 한다. ??
     *
     * ulnSFID_94(&sFnContext);
     */

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "Freeing ENV handle.");
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_IS_FLAG_UP(sNeedUnLock)
    {
        acpThrMutexUnlock(sLockEnv);
    }

    return ACI_FAILURE;
}

static SQLRETURN ulnFreeHandleDbc(ulnDbc *aDbc)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedUnLockEnv);

    ulnFnContext    sFnContext;

    acp_thr_mutex_t *sLockEnv;
    acp_thr_mutex_t *sLockDbc;
    ulnEnv          *sParentEnv;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREEHANDLE_DBC, aDbc, ULN_OBJ_TYPE_DBC);

    /*
     * lock ENV : ENV 의 lock 을 잡아 줘야 한다. DBC 를 없애는 것과 ENV 의 상태 전이는
     *            atomic 해야 하기 때문이다.
     */

    sParentEnv = aDbc->mParentEnv;
    sLockEnv   = sParentEnv->mObj.mLock;

    ACI_TEST(acpThrMutexLock(sLockEnv) != ACP_RC_SUCCESS);
    ULN_FLAG_UP(sNeedUnLockEnv);

    /*
     * ====================================
     * ENVIRONMENT LOCK BEGIN
     * ====================================
     */

    ACI_TEST(ulnEnter(&sFnContext, (void *)(aDbc->mParentEnv)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    sLockDbc     = aDbc->mObj.mLock;

    /*
     * =====================================
     * Function BEGIN
     * =====================================
     */

#ifdef COMPILE_SHARDCLI
    /* PROJ-2658 altibase sharding */
    ACI_TEST_RAISE(ulsdModuleEnvRemoveDbc(sParentEnv, aDbc) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
#else /* COMPILE_SHARDCLI */
    /*
     * BUGBUG: ENV 의 DBC 리스트에서 삭제하는 것은 먼저 프리를 한 후에 해야 할텐데.. 귀찮다.
     * Remove DBC from ENV
     */

    ACI_TEST_RAISE(ulnEnvRemoveDbc(sParentEnv, aDbc) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
#endif /* COMPILE_SHARDCLI */

    /*
     * Free DBC and unlock it.
     * BUGBUG: obvious bug if destroying DBC fails.
     */
    ACI_TEST_RAISE(ulnDbcDestroy(aDbc) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);

    ACI_TEST(acpThrMutexUnlock(sLockDbc) != ACP_RC_SUCCESS);
    ACI_TEST(acpThrMutexDestroy(sLockDbc) != ACP_RC_SUCCESS);
    uluLockDestroy(sLockDbc);

    /*
     * =====================================
     * Function END
     * =====================================
     */

    /*
     * Note : ulnExit 를 불러서는 안된다. DBC 가 이미 해제되었기 때문이다.
     *        ENV 의 상태 전이만 수행한다.
     */

    sFnContext.mWhere = ULN_STATE_EXIT_POINT;
    ulnSFID_95(&sFnContext);

    /*
     * ====================================
     * ENVIRONMENT LOCK END
     * ====================================
     */

    ULN_FLAG_DOWN(sNeedUnLockEnv);
    ACI_TEST(acpThrMutexUnlock(sLockEnv) != ACP_RC_SUCCESS);

    /* ulnExist 가 호출되지 않기 때문에 수동으로 ShardCoordFixCtrlCtx 를 수행한다. */
    if ( sFnContext.mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( &sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "Freeing DBC handle.");
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_IS_FLAG_UP(sNeedUnLockEnv)
    {
        acpThrMutexUnlock(sLockEnv);
    }

    if ( sFnContext.mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( &sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

ACI_RC ulnFreeHandleStmtSendFreeREQ(ulnFnContext *aContext, ulnDbc *aParentDbc, acp_uint8_t aMode)
{
    cmiProtocolContext *sCtx = &(aParentDbc->mPtContext.mCmiPtContext);
    acp_bool_t    sNeedPtContextFin = ACP_FALSE;

    // BUG-17514
    // 모든 RESULT SET ID는 ACP_UINT16_MAX를 사용한다.
    acp_uint16_t  sResultSetID      = ACP_UINT16_MAX;

    ulnStmt        *sStmt;
    cmiProtocol     sPacket;
    acp_uint16_t    sState = 0;
    acp_uint16_t    sOrgWriteCursor = CMI_GET_CURSOR(sCtx);

    sStmt = aContext->mHandle.mStmt;

    /*
     * protocol context 초기화
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(aContext,
                                          &(aParentDbc->mPtContext),
                                          &(aParentDbc->mSession)) != ACI_SUCCESS);

    sNeedPtContextFin = ACP_TRUE;

    sPacket.mOpID = CMP_OP_DB_Free;

    CMI_WRITE_CHECK(sCtx, 8);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_Free);
    CMI_WR4(sCtx, &(sStmt->mStatementID));
    CMI_WR2(sCtx, &sResultSetID);
    CMI_WR1(sCtx, aMode);

    //fix BUG-17722
    ACI_TEST(ulnWriteProtocol(aContext, &(aParentDbc->mPtContext),
                               &sPacket) != ACI_SUCCESS);


    /* BUG-45286 Free with Drop은 전송 지연 */
    ACI_TEST_RAISE((aParentDbc->mAttrPDODeferProtocols >= 2) && (aMode == CMP_DB_FREE_DROP),
                   NO_FLUSH_PROTOCOL);

    /*
     * req 패킷 전송
     */
    ACI_TEST(ulnFlushProtocol(aContext,&(aParentDbc->mPtContext))
             != ACI_SUCCESS);

    /*
     * result 패킷 수신
     */
    //fix BUG-17722
    if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        ACI_TEST(ulnReadProtocolIPCDA(aContext,
                                      &(aParentDbc->mPtContext),
                                      aParentDbc->mConnTimeoutValue) != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnReadProtocol(aContext,
                                 &(aParentDbc->mPtContext),
                                 aParentDbc->mConnTimeoutValue) != ACI_SUCCESS);
    }

    /*
     * protocol context 정리
     */
    sNeedPtContextFin = ACP_FALSE;
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(aContext,
                                        &(aParentDbc->mPtContext))
             != ACI_SUCCESS);

    ACI_EXCEPTION_CONT(NO_FLUSH_PROTOCOL);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if(sNeedPtContextFin == ACP_TRUE)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(aContext,&(aParentDbc->mPtContext));
    }

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    if (sState == 1)
    {
        CMI_SET_CURSOR(sCtx, sOrgWriteCursor);
    }


    return ACI_FAILURE;
}

ACI_RC ulnFreeHandleStmtBody(ulnFnContext *aContext,
                             ulnDbc       *aParentDbc,
                             ulnStmt      *aStmt,
                             acp_bool_t    aSendFreeReq)
{
    ulnFnContext sFnContext;

    /*
     * 서버에 stmt 를 할당(prepare 함으로써) 했었다면 서버에서 우선 free 한다
     */
    if(ulnStmtIsPrepared(aStmt) == ACP_TRUE)
    {
        /*
         * Free stmt req 전송. prepare 가 된 경우에만.
         *
         * BUGBUG : 만약 연결된 상태가 아니라면 죽어도 될까? state machine 에 따르면, 죽어도
         *          된다. 왜냐하면 뭔가 심각하게 꼬였기 때문인데...
         */
        ACE_ASSERT(ULN_OBJ_GET_STATE(aParentDbc) >= ULN_S_C4);

        if(aSendFreeReq == ACP_TRUE)
        {
            if(ulnDbcIsConnected(aParentDbc) == ACP_TRUE)
            {
                ACI_TEST(ulnFreeHandleStmtSendFreeREQ(aContext,
                                                      aParentDbc,
                                                      CMP_DB_FREE_DROP) != ACI_SUCCESS);
            }
        }
    }

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    if (ulnDbcIsAsyncPrefetchStmt(aParentDbc, aStmt) == ACP_TRUE)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext,
                                  aContext->mFuncID,
                                  aStmt,
                                  ULN_OBJ_TYPE_STMT);

        ACI_TEST(ulnFetchEndAsync(&sFnContext, &aParentDbc->mPtContext) != ACI_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    /*
     * 상위 DBC 로부터 삭제
     */
    ACI_TEST_RAISE(ulnDbcRemoveStmt(aParentDbc, aStmt) != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR);

    /*
     * Note: dbc 와 stmt 가 같은 lock 을 공유함에 주의.
     */
    ACI_TEST_RAISE(ulnStmtDestroy(aStmt) != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(aContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "Freeing STMT handle.");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static SQLRETURN ulnFreeHandleStmt(ulnStmt *aStmt)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext     sFnContext;

    acp_thr_mutex_t *sLockStmt;
    ulnDbc          *sParentDbc;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREEHANDLE_STMT, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, (void *)(aStmt->mParentDbc)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    sLockStmt  = aStmt->mObj.mLock;
    sParentDbc = aStmt->mParentDbc;

    /* PROJ-1789 Updatable Scrollable Cursor
     * Rowset Cache를 위한 Stmt Free */
    if (aStmt->mRowsetStmt != SQL_NULL_HSTMT)
    {
        /* FnContext에 설정된 Stmt를 Free하므로 잠깐 바꿔줘야 한다. */
        sFnContext.mHandle.mStmt = aStmt->mRowsetStmt;
        ACI_TEST(ulnFreeHandleStmtBody(&sFnContext,
                                       sParentDbc,
                                       aStmt->mRowsetStmt,
                                       ACP_TRUE) != ACI_SUCCESS);
        aStmt->mRowsetStmt = SQL_NULL_HSTMT;
        sFnContext.mHandle.mStmt = aStmt;
    }
    else
    {
        /* Do nothing */
    }

    ACI_TEST(ulnFreeHandleStmtBody(&sFnContext,
                                   sParentDbc,
                                   aStmt,
                                   ACP_TRUE) != ACI_SUCCESS);

    /*
     * Note: ulnExit 를 부르면 안됨. 상태함수를 곧장 불러야 한다.
     *       stmt 는 이미 해제되었으므로 상태전이의 필요성 없음. 단지 dbc 의 상태전이만 하면 됨.
     */
    sFnContext.mWhere = ULN_STATE_EXIT_POINT;
    ulnSFID_26(&sFnContext);

    ACI_TEST(acpThrMutexUnlock(sLockStmt) != ACP_RC_SUCCESS);

    /* ulnExist 가 호출되지 않기 때문에 수동으로 ShardCoordFixCtrlCtx 를 수행한다. */
    if ( sFnContext.mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( &sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    if ( sFnContext.mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( &sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

ACI_RC ulnFreeHandleDescBody(ulnFnContext *aContext, ulnDbc *aParentDbc, ulnDesc *aDesc)
{
    acp_list_node_t *sIterator;

    /*
     * 해제하려는 디스크립터와 연계된 Stmt들의 SQL_ATTR_APP_ROW_DESC나
     * SQL_ATTR_APP_PARAM_DESC들을 원래의 implicit 디스크립터로 복구한다.
     */
    ACP_LIST_ITERATE(&(aDesc->mAssociatedStmtList), sIterator)
    {
        /*
         * 뭔가 이상하게 꼬여 있으면 에러
         */
        ACI_TEST_RAISE((((ulnStmtAssociatedWithDesc *)sIterator)->mStmt->mAttrApd == aDesc ||
                        ((ulnStmtAssociatedWithDesc *)sIterator)->mStmt->mAttrArd == aDesc),
                       LABEL_MEM_MANAGE_ERR);

        if(((ulnStmtAssociatedWithDesc *)sIterator)->mStmt->mAttrApd == aDesc)
        {
            ulnRecoverOriginalApd(((ulnStmtAssociatedWithDesc *)sIterator)->mStmt);
        }
        else
        {
            ulnRecoverOriginalArd(((ulnStmtAssociatedWithDesc *)sIterator)->mStmt);
        }
    }

    /*
     * 상위 DBC 로부터 삭제한다.
     * BUGBUG: 롤백할 자신이 없어서 temporary list 는 안만들고, 디스크립터 파괴 전에 삭제했다.
     */
    ACI_TEST_RAISE(ulnDbcRemoveDesc(aParentDbc, aDesc) != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR);

    /*
     * 디스크립터를 파괴한다.
     * BUGBUG : 롤백못하겠다. 그냥 에러내고 드러누워버리자.
     */
    ACI_TEST_RAISE(ulnDescDestroy(aDesc) != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(aContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "Freeing DESC handle.");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static SQLRETURN ulnFreeHandleDesc(ulnDesc *aDesc)
{
    acp_thr_mutex_t *sLockDesc;
    ulnDbc          *sParentDbc;
    ulnFnContext     sFnContext;

    acp_bool_t       sNeedExit = ACP_FALSE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREEHANDLE_DESC, aDesc, ULN_OBJ_TYPE_DESC);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(aDesc->mParentObject)) != ACI_SUCCESS);

    sNeedExit = ACP_TRUE;
    sLockDesc = aDesc->mObj.mLock;

    ACI_TEST_RAISE(aDesc->mHeader.mAllocType == ULN_DESC_ALLOCTYPE_IMPLICIT,
                   LABEL_IMP_DESC);

    sParentDbc = (ulnDbc *)(aDesc->mParentObject);

    /*
     * Note: DESC 의 상태전이는 없는 거나 마찬가지다.
     * Note: 상위 객체인 DBC 는 DESC 의 Free 에 따른 상태전이는 없다. 에러발생은 있어도.
     */
    ACI_TEST(ulnFreeHandleDescBody(&sFnContext, sParentDbc, aDesc) != ACI_SUCCESS);

    /*
     * unlock
     */
    ACI_TEST(acpThrMutexUnlock(sLockDesc) != ACP_RC_SUCCESS);

    /* ulnExist 가 호출되지 않기 때문에 수동으로 ShardCoordFixCtrlCtx 를 수행한다. */
    if ( sFnContext.mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( &sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_IMP_DESC)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_IMP_DESC);
    }

    ACI_EXCEPTION_END;

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    if ( sFnContext.mShardCoordFixCtrlCtx != NULL )
    {
        ulnDbcShardCoordFixCtrlExit( &sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

/**
 * ulnFreeHandle.
 *
 * SQLFreeHandle()함수가 곧장 매핑되는 함수이다.
 */
SQLRETURN ulnFreeHandle(acp_sint16_t aHandleType, void *aHandle)
{
    SQLRETURN sReturnValue = SQL_INVALID_HANDLE;
    acp_char_t  sTypeStr[80];

    /*
     * Check if handle is not null
     */
    ACI_TEST_RAISE(aHandle == NULL, LABEL_INVALID_HANDLE);

    switch(aHandleType)
    {
        case SQL_HANDLE_ENV:
            /* bug-35985 macro compile error on AIX
               env가 free된 후에는 filelock이 없어지므로 먼저 로깅 */
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "env");
            ULN_TRACE_LOG(NULL, ULN_TRACELOG_MID, NULL, 0,
                    "%-18s| %-4s [%p]",
                    "ulnFreeHandle", sTypeStr, aHandle);
            sReturnValue = ulnFreeHandleEnv((ulnEnv *)aHandle);
            break;

        case SQL_HANDLE_DBC:
            sReturnValue = ulnFreeHandleDbc((ulnDbc *)aHandle);
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "dbc");
            break;

        case SQL_HANDLE_STMT:
            sReturnValue = ulnFreeHandleStmt((ulnStmt *)aHandle);
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "stmt");
            break;

        case SQL_HANDLE_DESC:
            sReturnValue = ulnFreeHandleDesc((ulnDesc *)aHandle);
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "desc");
            break;

        default:
            /*
             * AllocHandle에서는 HY092를 리턴해야 하지만,
             * FreeHandle에서는 Invalid Handle
             */
            acpSnprintf(sTypeStr , ACI_SIZEOF(sTypeStr), "%s", "invalid");
            ACI_TEST_RAISE(ACP_TRUE, LABEL_INVALID_HANDLE);
            break;
    }

    if (aHandleType != SQL_HANDLE_ENV)
    {
        ULN_TRACE_LOG(NULL, ULN_TRACELOG_MID, NULL, 0,
                "%-18s| %-4s [%p] %"ACI_INT32_FMT,
                "ulnFreeHandle", sTypeStr, aHandle, sReturnValue);
    }

    return sReturnValue;

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        sReturnValue = SQL_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(NULL, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| %-4s [%p] fail: %" ACI_INT32_FMT,
            "ulnFreeHandle", sTypeStr, aHandle, sReturnValue);
    return sReturnValue;
}

