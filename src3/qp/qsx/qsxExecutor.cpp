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
 * $Id: qsxExecutor.cpp 90434 2021-04-02 06:56:50Z khkwak $
 **********************************************************************/

#include <ideErrorMgr.h>
#include <qcg.h>
#include <qsxExecutor.h>
#include <qsxDef.h>
#include <qsxUtil.h>
#include <qsxCursor.h>
#include <qsxRefCursor.h>
#include <qsvEnv.h>
#include <qsxExtProc.h> // PROJ-1685
#include <qcuSessionPkg.h>
#include <qsxArray.h>
#include <qsxLibrary.h>
#include <sdi.h>

IDE_RC qsxExecutor::initialize( qsxExecutorInfo  * aExecInfo,
                                qsProcParseTree  * aProcPlanTree,
                                qsPkgParseTree   * aPkgPlanTree,
                                qcTemplate       * aPkgTemplate,
                                qcTemplate       * aTmplate )
{
    unsetFlowControl(aExecInfo);

    aExecInfo->mIsReturnValueValid = ID_FALSE;
    aExecInfo->mProcPlanTree       = aProcPlanTree;
    aExecInfo->mPkgPlanTree        = aPkgPlanTree;
    aExecInfo->mPkgTemplate        = aPkgTemplate;
    aExecInfo->mTriggerTmplate     = aTmplate;

    if( aProcPlanTree != NULL )
    {
        // BUG-42322
        aExecInfo->mObjectType        = aProcPlanTree->objectNameInfo.objectType;
        aExecInfo->mUserAndObjectName = aProcPlanTree->objectNameInfo.name;
        aExecInfo->mObjectID          = aProcPlanTree->procOID;
        aExecInfo->mDefinerUserID     = aProcPlanTree->userID;
        aExecInfo->mIsDefiner         = aProcPlanTree->isDefiner;
    }
    else 
    {
        IDE_DASSERT( aPkgPlanTree != NULL );

        // BUG-42322
        aExecInfo->mObjectType        = aPkgPlanTree->objectNameInfo.objectType;
        aExecInfo->mUserAndObjectName = aPkgPlanTree->objectNameInfo.name;
        aExecInfo->mObjectID          = aPkgPlanTree->pkgOID;
        aExecInfo->mDefinerUserID     = aPkgPlanTree->userID;
        aExecInfo->mIsDefiner         = aPkgPlanTree->isDefiner;
    }

    aExecInfo->mSqlErrorMessage[0] = '\0';

    return IDE_SUCCESS ;
}

IDE_RC qsxExecutor::initializeSqlCursor( qsxExecutorInfo  * aExecInfo,
                                         qcStatement      * aQcStmt)
{
#define IDE_FN "IDE_RC qsxExecutor::initialzieSqlCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if( aExecInfo->mProcPlanTree != NULL )
    {
        aExecInfo->mSqlCursorInfo = (qsxCursorInfo *)
            QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aQcStmt),
                                aExecInfo->mProcPlanTree->sqlCursorTypeNode );
    }
    else
    {
        /* PROJ-1073 Package
         * package body의 initialize section에서 DML문이 실행 가능해야합니다.
         * 그렇기 때문에 SP cursor가 필요하며, 이 cursor에 대해 initialize를 해줘야 합니다. */
        aExecInfo->mSqlCursorInfo = (qsxCursorInfo *)
            QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aQcStmt),
                                aExecInfo->mPkgPlanTree->sqlCursorTypeNode );
    }

    IDE_TEST( qsxCursor::initialize( aExecInfo->mSqlCursorInfo,
                                     ID_TRUE )
              != IDE_SUCCESS );

    QSX_CURSOR_SET_OPEN_FALSE( (aExecInfo->mSqlCursorInfo) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxExecutor::execPlan ( qsxExecutorInfo  * aExecInfo,
                               qcStatement      * aQcStmt,
                               mtcStack         * aStack,
                               SInt               aStackRemain )
{
#ifdef DEBUG
#if _QSX_DBG_LV_ > _QSX_DBG_LV_PLN_TREE_
    out.printf("==========IDE_RC qsxExecutor::exec =========\n");
    tpr.print(aProcPlan);
#endif
#endif

    qsProcParseTree   * sParseTree      = NULL;
    qsxProcInfo       * sProcInfo       = NULL;
    iduMemoryStatus     sQmxMemStatus;
    qcTemplate        * sSourceTemplate = NULL;
    qsxArrayInfo     ** sArrayInfo      = NULL;
    UInt                sStage          = 0;
    UInt                sVarStage       = 0;
    SChar             * sOriStmtText    = NULL;
    SInt                sOriStmtTextLen = 0;
    UInt                sTopExec        = 0;
    iduListNode       * sCacheNode      = NULL;
    // PROJ-1073 Package - package cursor
    qsxProcPlanList   * sPlanList       = NULL;
    idBool              sEnvTrigger     = ID_FALSE;
    idBool              sOrgPSMFlag     = ID_FALSE;
    SChar             * sOrgInvokeUserName = QCG_GET_SESSION_INVOKE_USER_NAME(aQcStmt);
    UInt                sUserID            = QCG_GET_SESSION_USER_ID(aQcStmt);      /* BUG-45306 PSM AUTHID */

    UInt                i = 0;
    qsxStmtList       * sOrgStmtList  = aQcStmt->spvEnv->mStmtList;
    qsxStmtList2      * sOrgStmtList2 = aQcStmt->spvEnv->mStmtList2;
    qcStmtListInfo    * sStmtListInfo = &(aQcStmt->session->mQPSpecific.mStmtListInfo);
    qsxStmtList       * sStmtList = NULL;
    qsxStmtList2      * sStmtList2 = NULL;

    idBool              sShardInPSMEnable = ID_TRUE;
    idBool              sShardInPSMEnableOrg = ID_TRUE;

    UInt                sOrgSpxEnvFlag;

    // BUG-41030 Backup called by PSM flag
    sOrgPSMFlag = aQcStmt->calledByPSMFlag;
    sParseTree  = aExecInfo->mProcPlanTree;
    sProcInfo   = sParseTree->procInfo;

    // BUG-43158 Enhance statement list caching in PSM
    aQcStmt->spvEnv->mStmtList  = NULL;
    aQcStmt->spvEnv->mStmtList2 = NULL;

    // TASK-7244 Check restrictions for Sharding
    sOrgSpxEnvFlag = aQcStmt->spxEnv->mFlag;

    if ( sProcInfo != NULL )
    {
        switch ( sProcInfo->shardSplitMethod )
        {
            case SDI_SPLIT_HASH:
                aQcStmt->spxEnv->mFlag |= QSX_ENV_SHARD_HASH_PROC;
                break;
            case SDI_SPLIT_RANGE:
                aQcStmt->spxEnv->mFlag |= QSX_ENV_SHARD_RANGE_PROC;
                break;
            case SDI_SPLIT_LIST:
                aQcStmt->spxEnv->mFlag |= QSX_ENV_SHARD_LIST_PROC;
                break;
            case SDI_SPLIT_CLONE:
                aQcStmt->spxEnv->mFlag |= QSX_ENV_SHARD_CLONE_PROC;
                break;
            case SDI_SPLIT_SOLO:
                aQcStmt->spxEnv->mFlag |= QSX_ENV_SHARD_SOLO_PROC;
                break;
            default:
                // Nothing to do.
                break;
        }
    }

    if( ( ( aQcStmt->spvEnv->flag & QSV_ENV_TRIGGER_MASK ) == QSV_ENV_TRIGGER_TRUE ) && 
        ( aExecInfo->mTriggerTmplate != NULL ) )
    {
        sEnvTrigger = ID_TRUE;
    }
    else
    {
        sEnvTrigger = ID_FALSE;
    }

    if ( SDU_SHARD_ENABLE == 1 )
    {
        sShardInPSMEnableOrg = QCG_GET_SESSION_SHARD_IN_PSM_ENABLE(aQcStmt);

        // BUG-48706
        // Global에서 local로만 변경할 수 있다.
        if ( sShardInPSMEnableOrg == ID_TRUE )
        {
            // BUG-48700
            // User session이 아닌 경우 PSM은 local로 동작합니다.
            // Local로 동작할 때 clone table에 write하는 것을 제한하기 위해서
            // local로 동작할 때 plan을 별도로 생성해야 합니다.
            // 따라서 user sessino이 아니면 session property에 local로 동작함을 명시합니다.
            if ( QCG_GET_SESSION_SHARD_SESSION_TYPE( aQcStmt ) != SDI_SESSION_TYPE_USER )
            {
                sShardInPSMEnable = ID_FALSE;
            }
            else
            {
                // BUG-46349, BUG-48706
                // 1. Trigger는 local로 동작한다.
                // 2. Query에서 실행한 function은 local로 동작한다.
                // 3. 그 외에는 global로 동작한다.
                // 4. Local로 동작한 이후에는 local로 계속 동작한다.
                if ( sEnvTrigger == ID_TRUE )
                {
                    // 1. Trigger는 local로 동작한다.
                    sShardInPSMEnable = ID_FALSE;
                }
                else
                {
                    if( (sParseTree->returnTypeVar != NULL) &&
                        (((aQcStmt->spxEnv->mFlag) & (QSX_ENV_DURING_SELECT | QSX_ENV_DURING_DML)) != QSX_ENV_FLAG_INIT) )
                    {
                        // 2. Query에서 실행한 function은 local로 동작한다.
                        sShardInPSMEnable = ID_FALSE;
                    }
                    else
                    {
                        // 3. 그 외에는 global로 동작한다.
                        sShardInPSMEnable = ID_TRUE;
                    }
                }
            }

            // BUG-48685
            // MM session callback을 통해서 실행하므로 변경해야 할 때만 값을 변경한다.
            if ( sShardInPSMEnableOrg != sShardInPSMEnable )
            {
                QCG_SET_SESSION_SHARD_IN_PSM_ENABLE(aQcStmt, sShardInPSMEnable);
            }
        }
    }
    else
    {
        // Shard가 아니면 default (ID_TRUE)
        // sShardInPSMEnable과 sShardInPSMEnableOrg가 항상 동일하다.
    }

   // BUG-17489
    // 최상위 PSM호출인 경우 planTreeFlag를 FALSE로 바꾼다.
    qcg::lock( aQcStmt );
    if ( aQcStmt->planTreeFlag == ID_TRUE )
    {
        aQcStmt->planTreeFlag = ID_FALSE;
        sTopExec = 1;
    }
    qcg::unlock( aQcStmt );

    // BUG-42322
    qsxEnv::pushStack( QC_QSX_ENV(aQcStmt) );

    IDE_TEST_RAISE( sParseTree == NULL, err_stmt_is_null );

    QSX_ENV_PLAN_TREE( QC_QSX_ENV(aQcStmt) )     = sParseTree;
    QSX_ENV_PKG_PLAN_TREE( QC_QSX_ENV(aQcStmt) ) = aExecInfo->mPkgPlanTree; /* BUG-39481 */

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage = 1;

    /* BUG-45306 PSM AUTHID */
    if ( aExecInfo->mIsDefiner == ID_TRUE )
    {
        QCG_SET_SESSION_USER_ID( aQcStmt, aExecInfo->mDefinerUserID );
        // BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
        QCG_SET_SESSION_INVOKE_USER_NAME( aQcStmt, sParseTree->objectNameInfo.userName );

        if ( SDU_SHARD_ENABLE == 1 )
        {
            IDE_TEST( qci::mSessionCallback.mSetInvokeUserPropertyInternal(
                        aQcStmt->session->mMmSession,
                        (SChar*)"INVOKE_USER",
                        11,
                        sParseTree->objectNameInfo.userName,
                        idlOS::strlen(sParseTree->objectNameInfo.userName) )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        QCG_SET_SESSION_USER_ID( aQcStmt, aExecInfo->mCurrentUserID );
    }

    sSourceTemplate = QC_PRIVATE_TMPLATE(aQcStmt);
    sStage = 2;

    // 트리거가 아니면 mTriggerTmplate의 값은 NULL이다.
    // package subprogram이 아니면 mPkgTemplate의 값은 NULL이다.
    if( aExecInfo->mTriggerTmplate == NULL )
    { 
        if( aExecInfo->mPkgTemplate == NULL )
        {
            // BUG-36203 PSM Optimize
            if( sProcInfo->cache != NULL )
            {
                (void)sProcInfo->cache->get( &QC_PRIVATE_TMPLATE(aQcStmt),
                                             &sCacheNode );
            }
            else
            {
                sCacheNode = NULL;
            }

            if( sCacheNode == NULL )
            {
                IDE_TEST( qcg::allocPrivateTemplate( aQcStmt,
                                                     aQcStmt->qmxMem )
                          != IDE_SUCCESS );

                IDE_TEST( qcg::cloneAndInitTemplate( aQcStmt->qmxMem,
                                                     sProcInfo->tmplate,
                                                     QC_PRIVATE_TMPLATE(aQcStmt) )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            QC_PRIVATE_TMPLATE(aQcStmt) = aExecInfo->mPkgTemplate;
        }
    }
    else
    {
        // 트리거는 이미 복사가 되서 넘어온다.
        QC_PRIVATE_TMPLATE(aQcStmt) = aExecInfo->mTriggerTmplate;
    }

    // BUG-43158 Enhance statement list caching in PSM
    if ( (sParseTree->procSqlCount > 0) &&
         (sParseTree->procSqlCount <= sStmtListInfo->mStmtPoolCount) )
    {
        if ( sStmtListInfo->mUsePtr == ID_TRUE )
        {
            for ( sStmtList = aQcStmt->mStmtList; sStmtList != NULL; sStmtList = sStmtList->mNext )
            {
                if ( sStmtList->mParseTree == sParseTree )
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            // 기존에 등록한 것이 없는 경우.
            if ( sStmtList == NULL )
            {
                if ( sStmtListInfo->mStmtListFreedCount > 0 )
                {
                    for ( i = 0; i < sStmtListInfo->mStmtListCursor; i++ )
                    {
                        if ( sStmtListInfo->mStmtList[i].mParseTree == NULL )
                        {
                            sStmtList = &sStmtListInfo->mStmtList[i];

                            sStmtList->mParseTree = sParseTree;
                            sStmtList->mNext = aQcStmt->mStmtList;

                            aQcStmt->mStmtList = sStmtList;

                            sStmtListInfo->mStmtListFreedCount--;
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    // Nothing to do.
                }

                if ( (sStmtList == NULL) &&
                     (sStmtListInfo->mStmtListCursor < sStmtListInfo->mStmtListCount) )
                {
                    sStmtList = &sStmtListInfo->mStmtList[sStmtListInfo->mStmtListCursor];

                    sStmtList->mParseTree = sParseTree;
                    sStmtList->mNext = aQcStmt->mStmtList;

                    aQcStmt->mStmtList = sStmtList;

                    sStmtListInfo->mStmtListCursor++;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
            aQcStmt->spvEnv->mStmtList  = sStmtList;
        }
        else
        {
            for ( sStmtList2 = aQcStmt->mStmtList2; sStmtList2 != NULL; sStmtList2 = sStmtList2->mNext )
            {
                if ( sStmtList2->mParseTree == sParseTree )
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            // 기존에 등록한 것이 없는 경우.
            if ( sStmtList2 == NULL )
            {
                if ( sStmtListInfo->mStmtListFreedCount > 0 )
                {
                    for ( i = 0; i < sStmtListInfo->mStmtListCursor; i++ )
                    {
                        if ( sStmtListInfo->mStmtList2[i].mParseTree == NULL )
                        {
                            sStmtList2 = &sStmtListInfo->mStmtList2[i];

                            sStmtList2->mParseTree = sParseTree;
                            sStmtList2->mNext = aQcStmt->mStmtList2;

                            aQcStmt->mStmtList2 = sStmtList2;

                            sStmtListInfo->mStmtListFreedCount--;
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    // Nothing to do.
                }

                if ( (sStmtList2 == NULL) &&
                     (sStmtListInfo->mStmtListCursor < sStmtListInfo->mStmtListCount) )
                {
                    sStmtList2 = &sStmtListInfo->mStmtList2[sStmtListInfo->mStmtListCursor];

                    sStmtList2->mParseTree = sParseTree;
                    sStmtList2->mNext = aQcStmt->mStmtList2;

                    aQcStmt->mStmtList2 = sStmtList2;

                    sStmtListInfo->mStmtListCursor++;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
            aQcStmt->spvEnv->mStmtList2 = sStmtList2;
        }
    }
    else
    {
        // Nothing to do.
    }

    // set stack buffer PR2475
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackBuffer = aStack;
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack       = aStack;
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackCount  = aStackRemain;
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain = aStackRemain;
    QC_PRIVATE_TMPLATE(aQcStmt)->stmt                = aQcStmt;
 
    // BUG-11192 date format session property 추가
    // clone된 template의 dateFormat은 다른 세션의 값이므로
    // 새로이 assign해주어야 한다.
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.dateFormat     = sSourceTemplate->tmplate.dateFormat;

    /* PROJ-2208 Multi Currency */
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.nlsCurrency    = sSourceTemplate->tmplate.nlsCurrency;

    // PROJ-1579 NCHAR
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.nlsUse     = sSourceTemplate->tmplate.nlsUse;
    sStage = 3;

    sOriStmtText                 = aQcStmt->myPlan->stmtText;
    sOriStmtTextLen              = aQcStmt->myPlan->stmtTextLen;
    aQcStmt->myPlan->stmtText    = sParseTree->stmtText;
    aQcStmt->myPlan->stmtTextLen = sParseTree->stmtTextLen;
    sStage = 4;

    IDE_TEST( qsxEnv::increaseCallDepth( QC_QSX_ENV(aQcStmt) )
              != IDE_SUCCESS );
    sStage = 5;

    IDE_TEST( initializeSqlCursor( aExecInfo,
                                   aQcStmt )
              != IDE_SUCCESS );

    /* PROJ-1073 Package - package cursor
       package에 선언된 cursor 역시 procedure의 cursor와 동일하게
       실행 시 초기화 시키고 실행 종료와 동시에 close 시킨다. */
    if ( sTopExec == 1 )
    {
        for( sPlanList = aQcStmt->spvEnv->procPlanList ;
             sPlanList != NULL ;
             sPlanList = sPlanList->next )
        {
            if( sPlanList->objectType == QS_PKG )
            {
                IDE_TEST( qcuSessionPkg::initPkgCursors( aQcStmt,
                                                         sPlanList->objectID )
                          != IDE_SUCCESS );

                if ( sPlanList->pkgBodyOID != QS_EMPTY_OID )
                {
                    IDE_TEST( qcuSessionPkg::initPkgCursors( aQcStmt,
                                                             sPlanList->pkgBodyOID )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }
    else
    {
        // Nothing to do.
    }
    sVarStage = 1;

    // PROJ-1075 returnTypeVar에 대한 초기화가 이루어 져야 한다.
    // array변수인 경우 초기화가 반드시 필요함.
    // 초기화는 하지만 함수 종료 직전까지 할당 해제는 하지 않음
    // ->이 함수의 호출자가 해제
    if( sParseTree->returnTypeVar != NULL )
    {
        IDE_TEST( initVariableItems( aExecInfo,
                                     aQcStmt,
                                     (qsVariableItems*)
                                     sParseTree->returnTypeVar,
                                     ID_FALSE /* check out param */ )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    sVarStage = 2;

    // BUG-37854
    if ( sParseTree->paramUseDate == ID_TRUE )
    {
        IDE_TEST( qtc::setDatePseudoColumn( QC_PRIVATE_TMPLATE( aQcStmt ) ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1075 parameter에 대한 초기화가 이루어 져야 한다.
    // array변수인 경우 초기화가 반드시 필요함.
    IDE_TEST( initVariableItems( aExecInfo,
                                 aQcStmt,
                                 sParseTree->paraDecls,
                                 ID_TRUE /* check out param */ )
              != IDE_SUCCESS );
    sVarStage = 3;

    // PROJ-1502 PARTITIONED DISK TABLE
    // PSM일 경우에만 argument를 복사한다.
    if( sEnvTrigger != ID_TRUE )
    {
        IDE_TEST( setArgumentValuesFromSourceTemplate(
                    aExecInfo,
                    aQcStmt ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-41030 Set called by PSM flag
    aQcStmt->calledByPSMFlag = ID_TRUE;

    // PROJ-1685
    if( sParseTree->procType == QS_NORMAL )
    {
        IDE_TEST( execBlock( aExecInfo,
                             aQcStmt,
                             (qsProcStmts* ) sParseTree->block )
                  != IDE_SUCCESS );
    }
    else if ( sParseTree->procType == QS_EXTERNAL_C )
    {
        aExecInfo->mIsReturnValueValid = ID_TRUE;

        IDE_TEST( execExtproc( aExecInfo,
                               aQcStmt ) != IDE_SUCCESS ); 
    }
    else if ( sParseTree->procType == QS_INTERNAL_C )
    {
        // PROJ-2717 Internal procedure
        aExecInfo->mIsReturnValueValid = ID_TRUE;

        IDE_TEST( execIntproc( aExecInfo,
                               aQcStmt ) != IDE_SUCCESS );
    }
    else
    {
        IDE_ERROR_RAISE( 0,
                         ERR_INVALID_OPTION );
    }

    // PROJ-1502 PARTITIONED DISK TABLE
    // PSM일 경우에만 argument를 복사한다.
    if( sEnvTrigger != ID_TRUE )
    {
        IDE_TEST( setOutArgumentValuesToSourceTemplate(
                aExecInfo,
                aQcStmt,
                sSourceTemplate ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // BUGBUG 2505
    // transfer SQL cursor to calle
    if (aExecInfo->mFlow == QSX_FLOW_RAISE)
    {
        if ( aExecInfo->mFlowIdSetBySystem )
        {
            switch(aExecInfo->mFlowId)
            {
                case QSX_CURSOR_ALREADY_OPEN_NO :
                case QSX_DUP_VAL_ON_INDEX_NO :
                case QSX_INVALID_CURSOR_NO :
                case QSX_INVALID_NUMBER_NO :
                case QSX_NO_DATA_FOUND_NO :
                case QSX_PROGRAM_ERROR_NO :
                case QSX_STORAGE_ERROR_NO :
                case QSX_TIMEOUT_ON_RESOURCE_NO :
                case QSX_TOO_MANY_ROWS_NO :
                case QSX_VALUE_ERROR_NO :
                case QSX_ZERO_DIVIDE_NO :

                    // PROJ-1371 File handling exception.
                case QSX_INVALID_PATH_NO:
                case QSX_INVALID_MODE_NO:
                case QSX_INVALID_FILEHANDLE_NO:
                case QSX_INVALID_OPERATION_NO:
                case QSX_READ_ERROR_NO:
                case QSX_WRITE_ERROR_NO:
                case QSX_ACCESS_DENIED_NO:
                case QSX_DELETE_FAILED_NO:
                case QSX_RENAME_FAILED_NO:

                    // TASK-7218 Handling Shard Multiple Errors
                case QSX_SHARD_MULTIPLE_ERRORS_NO:
                case QSX_OTHER_SYSTEM_ERROR_NO :
                    unsetFlowControl(aExecInfo, ID_FALSE);
                    // error code and message is already set.
                    IDE_RAISE( err_pass );
                    /* fall through */
                default:
                    break;
            }
        }

        IDE_RAISE(err_unhandled_exception);
    }

    if (sParseTree->returnTypeVar != NULL)
    {
        IDE_TEST_RAISE( aExecInfo->mIsReturnValueValid != ID_TRUE,
                        err_no_return );

        /* PROJ-2586 PSM Parameters and return without precision
           package의 경우, session에 있는 template을 가져와 사용함으로
           이전 실행했던 결과에 대한 precision, scale 및 smiColumn size 정보를 가지고 있다.
           이를 precision, scale 및 smiColumn size를 default값으로 원복 시켜주 후
           현재 실행하는 결과의 precision, scale 및 smiColumn size를 조정한다. */
        IDE_TEST( qsxUtil::finalizeParamAndReturnColumnInfo( aExecInfo->mSourceTemplateStack->column )
                  != IDE_SUCCESS );

        // set return value to stack[0] of source template.
        // PROJ-1075 return value가 만약 array인 경우
        // stack의 value는 psm변수가 아니기 때문에 초기화가 되어 있지 않다.
        // 따라서 이 때는 reference copy를 함.
        // ->assignValue의 마지막 인자가 TRUE 이면 reference copy
        IDE_TEST( qsxUtil::assignValue( QC_QMX_MEM( aQcStmt ),
                                        sParseTree->returnTypeVar->variableTypeNode,  // source
                                        QC_PRIVATE_TMPLATE(aQcStmt),                  // source
                                        // PROJ-1073 Package
                                        aExecInfo->mSourceTemplateStack,              // dest
                                        aExecInfo->mSourceTemplateStackRemain,        // dest
                                        sSourceTemplate,
                                        ID_TRUE,                                      // parameter 또는 return 여부
                                        ID_TRUE )                                     // copyRef
                  != IDE_SUCCESS );

        // PROJ-1075
        // 만약 return type이 array이면
        // qsxEnv의 returnedArray에 연결.
        if( sParseTree->returnTypeVar->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
        {
            IDE_DASSERT( aExecInfo->mSourceTemplateStack->column->module->id
                         == MTD_ASSOCIATIVE_ARRAY_ID );

            sArrayInfo = (qsxArrayInfo**)QTC_TMPL_FIXEDDATA(
                        QC_PRIVATE_TMPLATE(aQcStmt),
                        sParseTree->returnTypeVar->variableTypeNode );

            IDE_TEST_RAISE( *sArrayInfo == NULL, ERR_INVALID_ARRAY );

            // qsxEnv의 returnedArray에 연결.
            IDE_TEST( qsxEnv::addReturnArray( QC_QSX_ENV(aQcStmt),
                                              *sArrayInfo )           // source tmplate value
                      != IDE_SUCCESS );

            *sArrayInfo = NULL;
        }
        else
        {
            // Nothing to do.
        }
    }

    sVarStage = 2;
    // PROJ-1075 parameter 변수의 finalization
    // array변수인 경우 할당 해제를 이 안에서 해준다.
    IDE_TEST( finiVariableItems( aQcStmt,
                                 sParseTree->paraDecls )
              != IDE_SUCCESS );

    sVarStage = 1;
    if (sParseTree->returnTypeVar != NULL)
    {
        IDE_TEST( finiVariableItems( aQcStmt,
                                     (qsVariableItems*) sParseTree->returnTypeVar )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sVarStage = 0;
    /* PROJ-1073 Package - package cursor
       package에 선언된 cursor 역시 procedure의 cursor와 동일하게
       실행 시 초기화 시키고 실행 종료와 동시에 close 시킨다. */
    if ( sTopExec == 1 )
    {
        for( sPlanList = aQcStmt->spvEnv->procPlanList ;
             sPlanList != NULL ;
             sPlanList = sPlanList->next )
        {
            if( sPlanList->objectType == QS_PKG )
            {
                if ( sPlanList->pkgBodyOID != QS_EMPTY_OID )
                {
                    IDE_TEST( qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                             sPlanList->pkgBodyOID )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                IDE_TEST( qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                         sPlanList->objectID )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    sStage = 4;
    IDE_TEST( qsxEnv::decreaseCallDepth( QC_QSX_ENV(aQcStmt) )
              != IDE_SUCCESS );

    sStage = 3;
    aQcStmt->myPlan->stmtText    = sOriStmtText;
    aQcStmt->myPlan->stmtTextLen = sOriStmtTextLen;

    sStage = 2;
    if( aExecInfo->mPkgTemplate == NULL )
    {
        // Nothing to do.
    }
    else
    {
        QC_CONNECT_TEMPLATE_STACK(
            QC_PRIVATE_TMPLATE(aQcStmt),
            NULL, NULL, 0 , 0 );
    }

    // BUG-36203 PSM Optimze
    if( sCacheNode != NULL )
    {
        (void)sProcInfo->cache->put( sCacheNode );
        sCacheNode = NULL;
    }
    else
    {
        // Nothing to do.
    }

    sStage = 1;
    QC_PRIVATE_TMPLATE(aQcStmt) = sSourceTemplate;

    // BUG-44294 PSM내에서 실행한 DML이 변경한 row 수를 반환하도록 합니다.
    if ( ( aExecInfo->mTriggerTmplate == NULL ) &&
         ( (aQcStmt->spxEnv->mFlag & QSX_ENV_STMT_TYPE_MASK) == QSX_ENV_FLAG_INIT ) )
    {
        QC_PRIVATE_TMPLATE(aQcStmt)->numRows  = aExecInfo->mSqlCursorInfo->mRowCount;
        QC_PRIVATE_TMPLATE(aQcStmt)->stmtType = aExecInfo->mSqlCursorInfo->mStmtType;
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-45306 PSM AUTHID */
    QCG_SET_SESSION_USER_ID( aQcStmt, sUserID );
    // BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
    QCG_SET_SESSION_INVOKE_USER_NAME( aQcStmt, sOrgInvokeUserName );

    if ( SDU_SHARD_ENABLE == 1 )
    {
        IDE_TEST( qci::mSessionCallback.mSetInvokeUserPropertyInternal(
                    aQcStmt->session->mMmSession,
                    (SChar*)"INVOKE_USER",
                    11,
                    sOrgInvokeUserName,
                    idlOS::strlen(sOrgInvokeUserName) )
                  != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    // BUG-17489
    if ( sTopExec == 1 )
    {
        qcg::setPlanTreeState( aQcStmt, ID_TRUE );
    }

    // TASK-7244 Check restrictions for Sharding
    aQcStmt->spxEnv->mFlag = sOrgSpxEnvFlag;

    // BUG-41030 Restore called by PSM flag
    aQcStmt->calledByPSMFlag = sOrgPSMFlag;

    // BUG-42322
    qsxEnv::popStack( QC_QSX_ENV(aQcStmt) );

    // BUG-43158 Enhance statement list caching in PSM
    aQcStmt->spvEnv->mStmtList  = sOrgStmtList;
    aQcStmt->spvEnv->mStmtList2 = sOrgStmtList2;

    // BUG-48685
    // 원복이 필요한 경우에만 호출한다.
    if ( sShardInPSMEnableOrg != sShardInPSMEnable )
    {
        QCG_SET_SESSION_SHARD_IN_PSM_ENABLE(aQcStmt, sShardInPSMEnableOrg);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_unhandled_exception)
    {
        if ( getRaisedExceptionName( aExecInfo,
                                     aQcStmt )
             != IDE_SUCCESS)
        {
            idlOS::snprintf( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg,
                             ID_SIZEOF(QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg),
                             "%s",
                             "UNKNOWN SYMBOL");
        }
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_UNHANDLED_EXCEPTION,
                                 QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg) );
    }
    IDE_EXCEPTION(err_no_return);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_FUNCTION_WITH_NO_RETURN,
                                sParseTree->objectNameInfo.name));
    }
    IDE_EXCEPTION(err_stmt_is_null);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                "plan = null (exec)"));
    }
    IDE_EXCEPTION( ERR_INVALID_ARRAY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION( ERR_INVALID_OPTION );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                "invalid procedure type"));
    }
    IDE_EXCEPTION(err_pass);
    IDE_EXCEPTION_END;

    // PROJ-1075 에러가 나는 경우
    // parameter 뿐만 아니라 return value도 같이 할당 해제를 해주어야 한다.
    switch( sVarStage )
    {
        case 3:
            if ( finiVariableItems(
                    aQcStmt,
                    sParseTree->paraDecls )
                != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 2:
            if ( finiVariableItems(
                    aQcStmt,
                    (qsVariableItems*)
                    sParseTree->returnTypeVar )
                != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1 : 
            if ( sTopExec == 1 )
            {
                if ( aExecInfo->mPkgTemplate != NULL )
                {
                    (void)qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                         sParseTree->pkgBodyOID );

                    (void)qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                         sParseTree->procOID );
                }
                else
                {
                    // Nothing to do.
                }

                for( sPlanList = aQcStmt->spvEnv->procPlanList ;
                     sPlanList != NULL ;
                     sPlanList = sPlanList->next )
                {
                    if( sPlanList->objectType == QS_PKG )
                    {
                        if ( sPlanList->pkgBodyOID != QS_EMPTY_OID )
                        {
                            (void)qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                                 sPlanList->pkgBodyOID );
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        (void)qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                             sPlanList->objectID );
                    }
                }

            }
            else
            {
                // Nothing to do.
            }
            break;
        default:
            break;
    }
    sVarStage = 0;

    switch(sStage)
    {
        case 5:
            if ( qsxEnv::decreaseCallDepth( QC_QSX_ENV(aQcStmt) )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 4:
            aQcStmt->myPlan->stmtText    = sOriStmtText;
            aQcStmt->myPlan->stmtTextLen = sOriStmtTextLen;
            /* fall through */
        case 3:
            if( aExecInfo->mPkgTemplate == NULL )
            {
                // Nothing to do.
            }
            else
            {
                QC_CONNECT_TEMPLATE_STACK(
                    QC_PRIVATE_TMPLATE(aQcStmt),
                    NULL, NULL, 0 , 0 );
            }
            /* fall through */
        case 2:
            QC_PRIVATE_TMPLATE(aQcStmt) = sSourceTemplate;
            /* fall through */
        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);

            }
            break;
        default:
            break;
    }

    // BUG-36203 PSM Optimze
    if( sCacheNode != NULL )
    {
        (void)sProcInfo->cache->put( sCacheNode );
        sCacheNode = NULL;
    }
    else
    {
        // Nothing to do.
    }

    // BUG-41030 Restore called by PSM flag
    aQcStmt->calledByPSMFlag = sOrgPSMFlag;

    // BUG-17489
    if ( sTopExec == 1 )
    {
        qcg::setPlanTreeState( aQcStmt, ID_TRUE );
    }

    /* BUG-45306 PSM AUTHID */
    QCG_SET_SESSION_USER_ID( aQcStmt, sUserID );
    // BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
    QCG_SET_SESSION_INVOKE_USER_NAME( aQcStmt, sOrgInvokeUserName );

    if ( SDU_SHARD_ENABLE == 1 )
    {
        (void)qci::mSessionCallback.mSetInvokeUserPropertyInternal(
                aQcStmt->session->mMmSession,
                (SChar*)"INVOKE_USER",
                11,
                sOrgInvokeUserName,
                idlOS::strlen(sOrgInvokeUserName) );
    }

    // BUG-42322
    qsxEnv::popStack( QC_QSX_ENV(aQcStmt) );

    // TASK-7244 Check restrictions for Sharding
    aQcStmt->spxEnv->mFlag = sOrgSpxEnvFlag;

    // BUG-43158 Enhance statement list caching in PSM
    aQcStmt->spvEnv->mStmtList  = sOrgStmtList;
    aQcStmt->spvEnv->mStmtList2 = sOrgStmtList2;

    // BUG-48685
    // 원복이 필요한 경우에만 호출한다.
    if ( sShardInPSMEnableOrg != sShardInPSMEnable )
    {
        QCG_SET_SESSION_SHARD_IN_PSM_ENABLE(aQcStmt, sShardInPSMEnableOrg);
    }

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::setArgumentValuesFromSourceTemplate (
    qsxExecutorInfo * aExecInfo,
    qcStatement     * aQcStmt )
{
    qsVariableItems * sParaDecls;
    qsVariables     * sParaVar;
    mtcStack        * sStack;
    SInt              sStackRemain;
    idBool            sCopyRef     = ID_TRUE;
    mtcColumn       * sParamColumn = NULL;

    // PROJ-1073 Package
    sStack       = aExecInfo->mSourceTemplateStack;
    sStackRemain = aExecInfo->mSourceTemplateStackRemain;

    // skip stack[0] for return value.
    sStackRemain--;
    sStack++;

    for ( sParaDecls = aExecInfo->mProcPlanTree->paraDecls;
          sParaDecls != NULL;
          sParaDecls = sParaDecls->next,
          sStack ++,
          sStackRemain-- )
    {
        sParaVar = (qsVariables*)sParaDecls;
        sParamColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                        sParaVar->variableTypeNode );

        IDE_TEST( qsxUtil::adjustParamAndReturnColumnInfo( QC_QMX_MEM(aQcStmt),
                                                           sStack->column,
                                                           sParamColumn,
                                                           &QC_PRIVATE_TMPLATE(aQcStmt)->tmplate )
                  != IDE_SUCCESS );

        switch ( sParaVar->inOutType )
        {
            case QS_OUT :
                // To fix BUG-15195
                // 파라미터는 initVariableItems에서 초기화 했으므로
                // 여기서 초기화 하면 안됨.
                break;
            case QS_IN :
            case QS_INOUT :
                if ( sParaVar->nocopyType == QS_NOCOPY )
                {
                    sCopyRef = ID_TRUE;
                }
                else
                {
                    sCopyRef = ID_FALSE;
                }

                IDE_TEST( qsxUtil::assignValue( QC_QMX_MEM( aQcStmt ),
                                                sStack,
                                                sStackRemain,
                                                sParaVar->variableTypeNode,
                                                QC_PRIVATE_TMPLATE(aQcStmt),
                                                sCopyRef )
                          != IDE_SUCCESS );
                break;
            default :
                // IN-OUT Type이 정해지지 않을 수 없음
                IDE_ERROR( 0 );
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::setOutArgumentValuesToSourceTemplate (
    qsxExecutorInfo * aExecInfo,
    qcStatement     * aQcStmt,
    qcTemplate      * aSourceTmplate )
{
    qsVariableItems * sParaDecls;
    qsVariables     * sParaVar;
    mtcStack        * sStack;
    SInt              sStackRemain;
    idBool            sCopyRef = ID_TRUE;

    // PROJ-1073 Package
    sStack       = aExecInfo->mSourceTemplateStack;
    sStackRemain = aExecInfo->mSourceTemplateStackRemain;

    // skip stack[0] for return value.
    sStackRemain--;
    sStack++;

    for ( sParaDecls = aExecInfo->mProcPlanTree->paraDecls ;
          sParaDecls != NULL  ;
          sParaDecls = sParaDecls->next,
          sStack ++,
          sStackRemain -- )
    {
        sParaVar = (qsVariables*)sParaDecls;

        switch ( sParaVar->inOutType )
        {
            case QS_INOUT :
            case QS_OUT :
                if ( sParaVar->nocopyType == QS_NOCOPY )
                {
                    sCopyRef = ID_TRUE;
                }
                else
                {
                    sCopyRef = ID_FALSE;
                }

                IDE_TEST( qsxUtil::assignValue(
                        QC_QMX_MEM( aQcStmt ),
                        sParaVar->variableTypeNode,
                        QC_PRIVATE_TMPLATE(aQcStmt),
                        sStack,
                        sStackRemain,
                        aSourceTmplate,
                        ID_TRUE,      // parameter 또는 return 여부
                        sCopyRef )
                    != IDE_SUCCESS );
                break;
            case QS_IN :
                break;
            default :
                // IN-OUT Type이 정해지지 않을 수 없음
                IDE_ERROR( 0 );
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::getRaisedExceptionName( qsxExecutorInfo * aExecInfo,
                                            qcStatement     * aQcStmt )
{
    SChar * sSystemExcpName;
    UInt    sLength      = 0;
    UInt    sTotalLength = 0;

    if ( ( QS_ID_INIT_VALUE < aExecInfo->mFlowId ) &&
         ( QSX_USER_DEFINED_EXCEPTION_NO != aExecInfo->mFlowId) )
    {
        IDE_TEST( qsxUtil::getSystemDefinedExceptionName (
                aExecInfo->mFlowId,
                &sSystemExcpName )
            != IDE_SUCCESS );

        idlOS::snprintf( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg,
                         ID_SIZEOF(QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg),
                         "%s",
                         sSystemExcpName );
    }
    else
    {
        if ( aExecInfo->mRecentRaise == NULL )
        {
            if ( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsgLen == 0 )
            {
                idlOS::snprintf( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg,
                                 idlOS::strlen("Unable to find exception name") + 1,
                                 "%s",
                                 "Unable to find exception name");
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( QC_IS_NULL_NAME( aExecInfo->mRecentRaise->exception->userNamePos )
                 != ID_TRUE )
            {
                // Buffer Length를 넘지 않는 범위에서 복사하기 위한 길이 측정
                sLength = IDL_MIN( aExecInfo->mRecentRaise->exception->userNamePos.size , QC_MAX_OBJECT_NAME_LEN );

                idlOS::snprintf( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg,
                                 sLength + 1,
                                 "%s",
                                 aExecInfo->mRecentRaise->exception->userNamePos.stmtText +
                                 aExecInfo->mRecentRaise->exception->userNamePos.offset );
                sTotalLength = sLength;

                QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg[sTotalLength] = '.';
                sTotalLength++;
            }
            else
            {
                // Nothing to do.
            }

            if ( QC_IS_NULL_NAME( aExecInfo->mRecentRaise->exception->labelNamePos )
                 != ID_TRUE )
            {
                // Buffer Length를 넘지 않는 범위에서 복사하기 위한 길이 측정
                sLength = IDL_MIN( aExecInfo->mRecentRaise->exception->labelNamePos.size , QC_MAX_OBJECT_NAME_LEN );

                idlOS::snprintf( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg + sTotalLength,
                                 sLength + 1,
                                 "%s",
                                 aExecInfo->mRecentRaise->exception->labelNamePos.stmtText +
                                 aExecInfo->mRecentRaise->exception->labelNamePos.offset );
                sTotalLength = sTotalLength + sLength;

                QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg[sTotalLength] = '.';
                sTotalLength++;
            }
            else
            {
                // Nothing to do.
            }

            // Buffer Length를 넘지 않는 범위에서 복사하기 위한 길이 측정
            sLength = IDL_MIN( aExecInfo->mRecentRaise->exception->exceptionNamePos.size , QC_MAX_OBJECT_NAME_LEN );

            idlOS::snprintf( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg + sTotalLength,
                             sLength + 1,
                             "%s",
                             aExecInfo->mRecentRaise->exception->exceptionNamePos.stmtText +
                             aExecInfo->mRecentRaise->exception->exceptionNamePos.offset );
            sTotalLength = sTotalLength + sLength;

            QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg[sTotalLength] = '\0';

            sTotalLength = IDL_MIN( sTotalLength , MAX_ERROR_MSG_LEN );

            QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsgLen = sTotalLength;
        }
    }

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execBlock (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcBlock)
{
    iduMemoryStatus       sQmxMemStatus;
    SInt                  sStage = 0;
    qsProcStmtBlock     * sProcBlock;
    qsProcStmtException * sExcpBlock;
    UInt                  sOriSqlCode;
    SChar               * sOriSqlErrorMsg;

    sProcBlock = ( qsProcStmtBlock * ) aProcBlock;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage=1;

    // BUG-37854
    if ( aProcBlock->useDate == ID_TRUE )
    {
        IDE_TEST( qtc::setDatePseudoColumn( QC_PRIVATE_TMPLATE( aQcStmt ) ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( initVariableItems(
            aExecInfo,
            aQcStmt,
            sProcBlock->variableItems,
            ID_FALSE /* check out param */ )
        != IDE_SUCCESS );

    sStage=2;

    // PROJ-1335 RAISE 지원
    // To fix BUG-12642
    // exception handling이 이루어지면 블록 시작 이전의
    // 에러코드로 돌아가야 함
    IDE_TEST( QC_QMX_MEM(aQcStmt)->alloc(
            MAX_ERROR_MSG_LEN + 1,
            (void**) & sOriSqlErrorMsg )
        != IDE_SUCCESS);

    sOriSqlCode = QC_QSX_ENV(aQcStmt)->mSqlCode;
    // fix BUG-32565
    if( sOriSqlCode != 0 )
    {
        idlOS::strncpy( sOriSqlErrorMsg,
                        QC_QSX_ENV(aQcStmt)->mSqlErrorMessage,
                        MAX_ERROR_MSG_LEN );
        sOriSqlErrorMsg[ MAX_ERROR_MSG_LEN ] = '\0';
    }

    IDE_TEST( execStmtList ( aExecInfo,
                             aQcStmt,
                             sProcBlock->bodyStmts )
              != IDE_SUCCESS);

    if( sProcBlock->exception != NULL )
    {
        sExcpBlock = (qsProcStmtException *)(sProcBlock->exception);
        QC_PRIVATE_TMPLATE( aQcStmt )->stmt = aQcStmt;

        IDE_TEST( catchException ( aExecInfo,
                                   aQcStmt,
                                   sExcpBlock->exceptionHandlers)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1335 RAISE 지원
    // To fix BUG-12642
    // re-raise되지 않았다면 이전 에러코드로 복귀.
    if( aExecInfo->mFlow != QSX_FLOW_RAISE )
    {
        QC_QSX_ENV(aQcStmt)->mSqlCode = sOriSqlCode;
        // fix BUG-32565
        if( sOriSqlCode != 0 )
        {
            idlOS::strncpy( QC_QSX_ENV(aQcStmt)->mSqlErrorMessage,
                            sOriSqlErrorMsg,
                            MAX_ERROR_MSG_LEN );
            QC_QSX_ENV(aQcStmt)->mSqlErrorMessage[ MAX_ERROR_MSG_LEN ] = '\0';
        }
    }
    else
    {
        // Nothing to do.
    }

    sStage=1;
    IDE_TEST( finiVariableItems(
            aQcStmt,
            sProcBlock->variableItems )
        != IDE_SUCCESS );

    sStage=0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 2:
            if ( finiVariableItems(
                    aQcStmt,
                    sProcBlock->variableItems )
                != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }

        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);

            }
    }

    sStage = 0;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxExecutor::initVariableItems(
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsVariableItems     * aVarItems,
    idBool                aCheckOutParam )
{
    qsVariables       * sVariable;
    qsCursors         * sCursor;
    qsxCursorInfo     * sCursorInfo;
    qsxArrayInfo     ** sArrayInfo;
    qcuSqlSourceInfo    sSqlInfo;
    idBool              sCopyRef = ID_FALSE;
    SChar             * sErrorMsg = NULL;
    qsxStackFrame       sStackInfo = QC_QSX_ENV(aQcStmt)->mPrevStackInfo;

    for ( ;
          aVarItems != NULL ;
          aVarItems = aVarItems->next )
    {
        // BUG-42322
        qsxEnv::setStackInfo( QC_QSX_ENV(aQcStmt),
                              aExecInfo->mObjectID,
                              aVarItems->lineno,
                              aExecInfo->mObjectType,
                              aExecInfo->mUserAndObjectName );

        switch( aVarItems->itemType )
        {
            case QS_VARIABLE :
                // set default value or null
                sVariable = ( qsVariables * ) aVarItems;

                if ( sVariable->nocopyType == QS_COPY )
                {
                    sCopyRef = ID_FALSE;
                }
                else
                {
                    sCopyRef = ID_TRUE;
                }

                // BUG-36131
                if( ( aCheckOutParam == ID_TRUE ) &&
                    ( aQcStmt->calledByPSMFlag == ID_FALSE ) )
                {
                    if( ( ( sVariable->inOutType == QS_OUT ) ||
                          ( sVariable->inOutType == QS_INOUT ) ) &&
                        ( ( aQcStmt->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK ) 
                            == QCI_STMT_MASK_DML ) )
                    {
                        (void)sSqlInfo.init( QC_QMX_MEM(aQcStmt) );

                        IDE_SET(
                            ideSetErrorCode( qpERR_ABORT_QSX_FUNCTION_HAS_OUT_ARGUMENTS ) );
                        (void)sSqlInfo.fini();

                        IDE_RAISE( err_pass_wrap_sqltext );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }

                if( sVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                {
                    sArrayInfo = (qsxArrayInfo**) QTC_TMPL_FIXEDDATA(
                        QC_PRIVATE_TMPLATE(aQcStmt),
                        sVariable->variableTypeNode );

                    *sArrayInfo = NULL;

                    if ( sCopyRef == ID_FALSE )
                    {
                        IDE_TEST( qsxArray::initArray( aQcStmt,
                                                       sArrayInfo,
                                                       sVariable->typeInfo->columns,
                                                       aQcStmt->mStatistics )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else if ( (sVariable->variableType == QS_ROW_TYPE) ||
                          (sVariable->variableType == QS_RECORD_TYPE) )
                {
                    IDE_TEST( qsxUtil::initRecordVar( aQcStmt,
                                                      QC_PRIVATE_TMPLATE(aQcStmt),
                                                      sVariable->variableTypeNode,
                                                      sCopyRef )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                if ( sVariable->defaultValueNode != NULL )
                {
                    if ( (sVariable->variableTypeNode->lflag & QTC_NODE_SP_PARAM_OR_RETURN_MASK)
                         == QTC_NODE_SP_PARAM_OR_RETURN_TRUE )
                    {
                        /* parameter는 qsxExecutor::setArgumentFromSourceTemplate에서
                           default value를 할당한다. */
                        // Nothing to do.
                    }
                    else
                    {
                        if ( qsxUtil::calculateAndAssign (
                                QC_QMX_MEM ( aQcStmt ),
                                sVariable->defaultValueNode,
                                QC_PRIVATE_TMPLATE(aQcStmt),
                                sVariable->variableTypeNode,
                                QC_PRIVATE_TMPLATE(aQcStmt),
                                sCopyRef )
                            != IDE_SUCCESS )
                        {
                            IDE_RAISE( err_pass_wrap_sqltext );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    if ( (sVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE) ||
                         (sVariable->variableType == QS_ROW_TYPE) ||
                         (sVariable->variableType == QS_RECORD_TYPE) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        IDE_TEST( qsxUtil::assignNull( sVariable->variableTypeNode,
                                                       QC_PRIVATE_TMPLATE(aQcStmt) )
                                  != IDE_SUCCESS );
                    }
                }
                break;
            case QS_TRIGGER_NEW_VARIABLE:
                // BUG-46074
                // Delete trigger의 new row는 NULL이다.
                if ( aQcStmt->spxEnv->mTriggerEventType == QCM_TRIGGER_EVENT_DELETE )
                {
                    sVariable = ( qsVariables * ) aVarItems;

                    IDE_TEST( qsxUtil::assignNull( sVariable->variableTypeNode,
                                                   QC_PRIVATE_TMPLATE(aQcStmt) )
                              != IDE_SUCCESS );
                }

                break;
            case QS_TRIGGER_OLD_VARIABLE:
                // BUG-46074
                // Insert trigger의 old row는 NULL이다.
                if ( aQcStmt->spxEnv->mTriggerEventType == QCM_TRIGGER_EVENT_INSERT )
                {
                    sVariable = ( qsVariables * ) aVarItems;

                    IDE_TEST( qsxUtil::assignNull( sVariable->variableTypeNode,
                                                   QC_PRIVATE_TMPLATE(aQcStmt) )
                              != IDE_SUCCESS );
                }

                break;
            case QS_CURSOR :
                sCursor     = ( qsCursors * ) aVarItems;

                sCursorInfo = (qsxCursorInfo *) QTC_TMPL_FIXEDDATA(
                    QC_PRIVATE_TMPLATE(aQcStmt),
                    sCursor->cursorTypeNode );

                IDE_TEST( qsxCursor::initialize( sCursorInfo,
                                                 ID_FALSE )
                          != IDE_SUCCESS );

                sCursorInfo->mCursor = sCursor;

                // PROJ-2197에서 실시간 rebuild를 제거하여
                // execOpen, execOpenFor에서 각각 수행하던 것을 원복함.
                (void)qsxCursor::setCursorSpec( sCursorInfo,
                                                sCursor->paraDecls );

                // BUG-44716 Initialize & finalize parameters of Cursor
                IDE_TEST( initVariableItems( aExecInfo,
                                             aQcStmt,
                                             sCursorInfo->mCursorParaDecls,
                                             ID_TRUE )
                          != IDE_SUCCESS );
                break;
            case QS_EXCEPTION :
            case QS_TYPE :
            case QS_PRAGMA_AUTONOMOUS_TRANS :
            case QS_PRAGMA_EXCEPTION_INIT :
                break; 
            default:
                break;
        }
    }

    QC_QSX_ENV(aQcStmt)->mPrevStackInfo = sStackInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext )
    {
        // To fix BUG-13208
        // system_유저가 만든 프로시져는 내부공개 안함.
        if ( aExecInfo->mDefinerUserID == QC_SYSTEM_USER_ID )
        {
            qsxEnv::setErrorCode( QC_QSX_ENV(aQcStmt) );
            qsxEnv::setErrorMessage( QC_QSX_ENV(aQcStmt) );
        }
        else
        {
            // set original error code.
            qsxEnv::setErrorCode( QC_QSX_ENV(aQcStmt) );

            // BUG-43998
            // PSM 생성 오류 발생시 오류 발생 위치를 한 번만 출력하도록 합니다.
            if ( ideHasErrorPosition() == ID_FALSE )
            {
                (void)sSqlInfo.initWithBeforeMessage( QC_QMX_MEM(aQcStmt) );
  
                // BUG-42322
                if ( QCU_PSM_SHOW_ERROR_STACK == 1 )
                {
                    sErrorMsg = sSqlInfo.getErrMessage();
                }
                else // QCU_PSM_SHOW_ERROR_STACK = 0
                {
                    sErrorMsg = aExecInfo->mSqlErrorMessage;
  
                    IDE_DASSERT( aVarItems != NULL );
  
                    idlOS::snprintf( sErrorMsg,
                                     MAX_ERROR_MSG_LEN + 1,
                                     "\nat \"%s\", line %"ID_INT32_FMT"",
                                     aExecInfo->mUserAndObjectName,
                                     aVarItems->lineno );
                }
  
                IDE_SET(
                    ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                    sSqlInfo.getBeforeErrMessage(),
                                    sErrorMsg));
                (void)sSqlInfo.fini();
            }

            // set sophisticated error message.
            qsxEnv::setErrorMessage( QC_QSX_ENV(aQcStmt) );
        }
    }
    IDE_EXCEPTION_END;

    QC_QSX_ENV(aQcStmt)->mPrevStackInfo = sStackInfo;
 
    return IDE_FAILURE;
}

IDE_RC qsxExecutor::finiVariableItems(
    qcStatement         * aQcStmt,
    qsVariableItems     * aVarItems )
{
    qsCursors      * sCursor;
    qsxCursorInfo  * sCursorInfo;
    qsVariables    * sVariable;
    qsxArrayInfo  ** sArrayInfo;
    mtcColumn      * sColumn = NULL;

    for ( ;
          aVarItems != NULL ;
          aVarItems = aVarItems->next )
    {

        switch( aVarItems->itemType )
        {
            case QS_CURSOR :
                sCursor  = ( qsCursors * ) aVarItems;

                // BUG-27037 QP Code Sonar
                IDE_ASSERT( sCursor->cursorTypeNode != NULL );

                sCursorInfo = (qsxCursorInfo *) QTC_TMPL_FIXEDDATA(
                    QC_PRIVATE_TMPLATE(aQcStmt),
                    sCursor->cursorTypeNode );

                // BUG-44716 Initialize & finalize parameters of Cursor
                IDE_TEST( finiVariableItems( aQcStmt,
                                             sCursorInfo->mCursorParaDecls )
                          != IDE_SUCCESS );

                IDE_TEST( qsxCursor::finalize( sCursorInfo,
                                               aQcStmt )
                          != IDE_SUCCESS );

                break;
            case QS_VARIABLE :
                sVariable = ( qsVariables * ) aVarItems;

                if ( sVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                {
                    // BUG-27037 QP Code Sonar
                    IDE_ASSERT( sVariable->variableTypeNode != NULL );

                    sArrayInfo = ((qsxArrayInfo**) QTC_TMPL_FIXEDDATA(
                        QC_PRIVATE_TMPLATE(aQcStmt),
                        sVariable->variableTypeNode ));

                    IDE_TEST( qsxArray::finalizeArray( sArrayInfo )
                              != IDE_SUCCESS );
                }
                else if ( (sVariable->variableType == QS_ROW_TYPE) ||
                          (sVariable->variableType == QS_RECORD_TYPE) )
                {
                    // record type에 array type을 포함하는 경우 각 array도 finalize 해야 한다.
                    IDE_TEST( qsxUtil::finalizeRecordVar( QC_PRIVATE_TMPLATE(aQcStmt),
                                                          sVariable->variableTypeNode )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                if ( (sVariable->variableTypeNode->lflag & QTC_NODE_SP_PARAM_OR_RETURN_MASK)
                     == QTC_NODE_SP_PARAM_OR_RETURN_TRUE )
                {
                    sColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                               sVariable->variableTypeNode );

                    IDE_TEST( qsxUtil::finalizeParamAndReturnColumnInfo( sColumn )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
                break;
            case QS_TRIGGER_NEW_VARIABLE :
            case QS_TRIGGER_OLD_VARIABLE :
            case QS_EXCEPTION :
            case QS_TYPE :
            case QS_PRAGMA_AUTONOMOUS_TRANS :
            case QS_PRAGMA_EXCEPTION_INIT :
                break;
            default:
                break;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execStmtList (
    qsxExecutorInfo * aExecInfo,
    qcStatement     * aQcStmt,
    qsProcStmts     * aProcStmts )
{
    IDE_RC              sRc = IDE_SUCCESS;
    idBool              sStopExecution = ID_FALSE;
    UInt                sErrorCode;
    qcuSqlSourceInfo    sSqlInfo;
    qsProcStmts       * sProcStmts;
    qsProcStmts       * sProcNextStmts;
    qsLabels          * sLabel;
    qsxReturnVarList  * sReturnValue;
    idBool              sBackuped = ID_FALSE;
    qsxStackFrame       sStackInfo = QC_QSX_ENV(aQcStmt)->mPrevStackInfo;

    sProcStmts = aProcStmts;

    for ( ;
          sProcStmts != NULL && !sStopExecution ;
          sProcStmts = sProcNextStmts )
    {
        // BUG-42322
        qsxEnv::setStackInfo( QC_QSX_ENV(aQcStmt),
                              aExecInfo->mObjectID,
                              sProcStmts->lineno,
                              aExecInfo->mObjectType,
                              aExecInfo->mUserAndObjectName );

        sProcNextStmts = sProcStmts->next;

        // BUG-41311
        qsxEnv::backupReturnValue( QC_QSX_ENV(aQcStmt),
                                   & sReturnValue );
        sBackuped = ID_TRUE;
            
        // BUG-36902
        if( sProcStmts->useDate == ID_TRUE )
        {
            sRc = qtc::setDatePseudoColumn( QC_PRIVATE_TMPLATE( aQcStmt ) );
        }
        else
        {
            // Nothing to do.
        }

        if ( sRc == IDE_SUCCESS )
        {
            sRc = sProcStmts->execute( aExecInfo,
                                       aQcStmt,
                                       sProcStmts );
        }

        if ( sRc != IDE_SUCCESS )
        {
            sErrorCode = sdi::getMultiErrorCode();

            // do critical errors should not be sent to
            // others section .
            // eg> internal server error of qsx
            if ( qsxEnv::isCriticalError( sErrorCode ) == ID_TRUE )
            {
                IDE_RAISE(err_pass);
            }

            // if already the wrapper wrapped the error,
            // no need to wrap it again.
            if ( QSX_ENV_ERROR_CODE( QC_QSX_ENV(aQcStmt) ) != sErrorCode ) // not wrapped yet.
            {
                (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                                   aQcStmt,
                                                   sProcStmts,
                                                   &sSqlInfo,
                                                   ID_TRUE );
            }

            raiseConvertedException( aExecInfo,
                                     sErrorCode );
        }

        switch( aExecInfo->mFlow )
        {
            case QSX_FLOW_NONE :
                break;
            case QSX_FLOW_RETURN :
                sStopExecution = ID_TRUE;
                break;
            case QSX_FLOW_GOTO :
                if( findLabel( sProcStmts->parent,
                               aExecInfo->mFlowId,
                               &sLabel )
                    == ID_TRUE )
                {
                    // goto가 참조하는 label이 parent에 있는 경우
                    // 다음 실행 statement를 label->stmt로 지정
                    sProcNextStmts = sLabel->stmt;
                    unsetFlowControl(aExecInfo);
                }
                else
                {
                    sStopExecution = ID_TRUE;
                }
                break;
            case QSX_FLOW_RAISE :
                sStopExecution = ID_TRUE;
                break;
            case QSX_FLOW_EXIT :
                sStopExecution = ID_TRUE;
                break;
            case QSX_FLOW_CONTINUE :
                sStopExecution = ID_TRUE;
                break;
            default :
                IDE_RAISE(err_unknown_flow_control);
        }

        // PROJ-1075 하나의 procedure statement를 실행한 이후
        // 혹시 function에서 return된 array변수가 있나 체크하고
        // 있으면 할당 해제.
        qsxEnv::freeReturnArray( QC_QSX_ENV(aQcStmt) );

        // BUG-41311
        sBackuped = ID_FALSE;
        qsxEnv::restoreReturnValue( QC_QSX_ENV(aQcStmt),
                                    sReturnValue );
    } // end of for loop

    QC_QSX_ENV(aQcStmt)->mPrevStackInfo = sStackInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_unknown_flow_control);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                "unknown flow control"));
    }
    IDE_EXCEPTION(err_pass)
    {
        // propagation current error code
    }
    IDE_EXCEPTION_END;

    if ( sBackuped == ID_TRUE )
    {
        qsxEnv::restoreReturnValue( QC_QSX_ENV(aQcStmt),
                                    sReturnValue );
    }
    else
    {
        // Nothing to do.
    }

    QC_QSX_ENV(aQcStmt)->mPrevStackInfo = sStackInfo;
    
    return IDE_FAILURE;
}

void qsxExecutor::raiseSystemException(
    qsxExecutorInfo * aExecInfo,
    SInt              aExcpId )
{
#define IDE_FN "IDE_RC qsxExecutor::raiseSystemException"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aExecInfo->mFlow = QSX_FLOW_RAISE;
    aExecInfo->mFlowId = aExcpId;
    aExecInfo->mRecentRaise = NULL;

#undef IDE_FN
}

void qsxExecutor::raiseConvertedException(
    qsxExecutorInfo * aExecInfo,
    UInt              aIdeErrorCode)
{
#define IDE_FN "IDE_RC qsxExecutor::raiseConvertedException"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SInt exception = QSX_INVALID_EXCEPTION_ID;
    switch(E_ERROR_CODE(aIdeErrorCode))
    {
        case E_ERROR_CODE(qpERR_ABORT_QSX_CURSOR_ALREADY_OPEN) :
            exception = QSX_CURSOR_ALREADY_OPEN_NO;
            break;
        case E_ERROR_CODE(smERR_ABORT_smnUniqueViolation) :
        case E_ERROR_CODE(qpERR_ABORT_QSX_DUP_VAL_ON_INDEX) :
            exception = QSX_DUP_VAL_ON_INDEX_NO;
            break;
        case E_ERROR_CODE(qpERR_ABORT_QSX_INVALID_CURSOR) :
            exception = QSX_INVALID_CURSOR_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_idaInvalidNumeric) :
        case E_ERROR_CODE(mtERR_ABORT_INVALID_LITERAL) :
        case E_ERROR_CODE(qpERR_ABORT_QSX_INVALID_NUMBER) :
            exception = QSX_INVALID_NUMBER_NO;
            break;
        case E_ERROR_CODE(qpERR_ABORT_QSX_NO_DATA_FOUND) :
        case E_ERROR_CODE(idERR_ABORT_IDU_FILE_NO_DATA_FOUND) :
            exception = QSX_NO_DATA_FOUND_NO;
            break;
        case E_ERROR_CODE(qpERR_ABORT_QSX_PROGRAM_ERROR) :
            exception = QSX_PROGRAM_ERROR_NO;
            break;
        case E_ERROR_CODE(qpERR_ABORT_QSX_STORAGE_ERROR) :
            exception = QSX_STORAGE_ERROR_NO;
            break;
        case E_ERROR_CODE(qpERR_ABORT_QSX_TIMEOUT_ON_RESOURCE) :
            exception = QSX_TIMEOUT_ON_RESOURCE_NO;
            break;
        case E_ERROR_CODE(qpERR_ABORT_QSX_TOO_MANY_ROWS) :
            exception = QSX_TOO_MANY_ROWS_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_idaDivideByZero) :
        case E_ERROR_CODE(mtERR_ABORT_DIVIDE_BY_ZERO) :
        case E_ERROR_CODE(qpERR_ABORT_QSX_ZERO_DIVIDE) :
            exception = QSX_ZERO_DIVIDE_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_idaOverflow) :
        case E_ERROR_CODE(idERR_ABORT_idaLengthOutbound) :
        case E_ERROR_CODE(idERR_ABORT_idaPrecisionOutbound) :
        case E_ERROR_CODE(idERR_ABORT_idaScaleOutbound) :
        case E_ERROR_CODE(idERR_ABORT_idaLargerThanPrecision) :
        case E_ERROR_CODE(mtERR_FATAL_CONVERSION_COLLISION ) :
        case E_ERROR_CODE(mtERR_FATAL_INCOMPATIBLE_TYPE) :
        case E_ERROR_CODE(mtERR_ABORT_CONVERSION_NOT_APPLICABLE) :
        case E_ERROR_CODE(mtERR_ABORT_INVALID_LENGTH) :
        case E_ERROR_CODE(mtERR_ABORT_INVALID_PRECISION) :
        case E_ERROR_CODE(mtERR_ABORT_INVALID_SCALE) :
        case E_ERROR_CODE(mtERR_ABORT_VALUE_OVERFLOW) :
        case E_ERROR_CODE(mtERR_ABORT_STACK_OVERFLOW) :
        case E_ERROR_CODE(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE) :
        case E_ERROR_CODE(mtERR_ABORT_INVALID_WKT) :
        case E_ERROR_CODE(mtERR_ABORT_TO_CHAR_MAX_PRECISION) :
        case E_ERROR_CODE(qpERR_ABORT_QTC_MULTIPLE_ROWS) :
        case E_ERROR_CODE(qpERR_ABORT_QSX_VALUE_ERROR) :
            exception = QSX_VALUE_ERROR_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_IDU_FILE_INVALID_PATH) :
            exception = QSX_INVALID_PATH_NO;
            break;
        case E_ERROR_CODE(qpERR_ABORT_QSF_INVALID_FILEOPEN_MODE) :
            exception = QSX_INVALID_MODE_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_IDU_FILE_INVALID_FILEHANDLE) :
            exception = QSX_INVALID_FILEHANDLE_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_IDU_FILE_INVALID_OPERATION) :
            exception = QSX_INVALID_OPERATION_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_IDU_FILE_READ_ERROR) :
            exception = QSX_READ_ERROR_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_IDU_FILE_WRITE_ERROR) :
            exception = QSX_WRITE_ERROR_NO;
            break;
        case E_ERROR_CODE(qpERR_ABORT_QSF_DIRECTORY_ACCESS_DENIED) :
            exception = QSX_ACCESS_DENIED_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_IDU_FILE_DELETE_FAILED) :
            exception = QSX_DELETE_FAILED_NO;
            break;
        case E_ERROR_CODE(idERR_ABORT_IDU_FILE_RENAME_FAILED) :
            exception = QSX_RENAME_FAILED_NO;
            break;
        case E_ERROR_CODE(sdERR_ABORT_SHARD_MULTIPLE_ERRORS) :
            exception = QSX_SHARD_MULTIPLE_ERRORS_NO;
            break;
        default :
            exception = QSX_OTHER_SYSTEM_ERROR_NO;
    }
    raiseSystemException( aExecInfo,
                          exception );
    aExecInfo->mFlowIdSetBySystem = ID_TRUE;

#undef IDE_FN
}

void qsxExecutor::unsetFlowControl(
    qsxExecutorInfo   * aExecInfo,
    idBool              aIsClearIdeError /* = ID_TRUE */ )
{
#define IDE_FN "IDE_RC qsxExecutor::unsetFlowControl"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aExecInfo->mFlow = QSX_FLOW_NONE;
    aExecInfo->mFlowId = QSX_FLOW_ID_INVALID;
    aExecInfo->mRecentRaise = NULL;
    aExecInfo->mExitLoopStmt = NULL;
    aExecInfo->mContLoopStmt = NULL;
    aExecInfo->mFlowIdSetBySystem = ID_FALSE;

    if (aIsClearIdeError)
    {
        IDE_CLEAR();
    }

#undef IDE_FN
}

IDE_RC qsxExecutor::catchException (
    qsxExecutorInfo      * aExecInfo,
    qcStatement          * aQcStmt,
    qsExceptionHandlers  * aExcpHdlrs)
{
    qsExceptionHandlers  * sExcpHdlr;
    qsExceptions         * sExcp;
    idBool                 sIsExcpFound = ID_FALSE;
    idBool                 sOthersFound = ID_FALSE;

    // TASK-7244 DBMS_SHARD_GET_DIAGNOSTICS package
    UInt                   sFlag = 0;
    UInt                   sOrgErrorListCount;
    iduList                sOrgErrorList;
    ideErrorCollection   * sErrors;

    if ( aExecInfo->mFlow == QSX_FLOW_RAISE )
    {
        IDE_TEST_RAISE( aExecInfo->mFlowId == QSX_FLOW_ID_INVALID,
                        err_flow_id_invalid);

        IDE_TEST_RAISE( aExecInfo->mFlowId == 0, err_flow_id_is_not_set );

        for (sExcpHdlr = aExcpHdlrs;
             sExcpHdlr != NULL && aExecInfo->mFlow == QSX_FLOW_RAISE;
             sExcpHdlr = sExcpHdlr->next )
        {
            // OTHERS
            if (sExcpHdlr->exceptions == NULL &&
                aExecInfo->mFlow == QSX_FLOW_RAISE)
            {
                sOthersFound = ID_TRUE;
                sIsExcpFound = ID_TRUE;
            }
            else // EXCP1 OR EXCP2 OR ...
            {
                sIsExcpFound = ID_FALSE;
                for (sExcp = sExcpHdlr->exceptions;
                     sExcp != NULL;
                     sExcp = sExcp->next)
                {
                    if ( ((sExcp->id == aExecInfo->mFlowId) &&
                          (sExcp->objectID == QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpOID)) ||
                         (sExcp->errorCode == QSX_ENV_ERROR_CODE(QC_QSX_ENV(aQcStmt))) ||
                         (sExcp->userErrorCode == E_ERROR_CODE(QSX_ENV_ERROR_CODE(QC_QSX_ENV(aQcStmt)))) )
                    {
                        sIsExcpFound = ID_TRUE;
                        break;
                    }
                }
            }

            if ( sIsExcpFound == ID_TRUE )
            {
                break;
            }
        }

        if ( sIsExcpFound == ID_TRUE )
        {
            if ( SDU_SHARD_ENABLE == 1 )
            {
                /* ================= BEGIN error adjustment =======================
                 *  완료 전까지 EXCEPTION이 발생하면 안됨
                 *  1. SPXENV => BACKUP
                 *  2. ERRMGR => SPXENV
                 *  3. ERRMGR INIT */
                // 1. SPXENV => BACKUP
                sOrgErrorListCount = aQcStmt->spxEnv->mErrorListCount;
                IDU_LIST_INIT( &sOrgErrorList );
                IDU_LIST_JOIN_LIST( &sOrgErrorList, &(aQcStmt->spxEnv->mErrorList) );

                sErrors = &(ideGetErrorMgr()->mErrors);
                // 2. ERRORMGR => SPXENV
                aQcStmt->spxEnv->mErrorListCount = sErrors->mErrorListCnt;
                IDU_LIST_JOIN_LIST( &(aQcStmt->spxEnv->mErrorList), &(sErrors->mErrorList) );

                // 3. ERRMGR INIT (mErrorList는 IDU_LIST_JOIN_LIST하며 초기화 함)
                sErrors->mErrorListCnt = 0;
                /* =================== END error adjustment ======================= */
                sFlag = 1;
            }

            unsetFlowControl( aExecInfo );

            if ( sOthersFound == ID_TRUE )
            {
                IDE_TEST( qsxEnv::beginOthersClause( QC_QSX_ENV(aQcStmt) )
                          != IDE_SUCCESS );
            }

            IDE_TEST(execStmtList (aExecInfo,
                                   aQcStmt,
                                   sExcpHdlr->actionStmt)
                     != IDE_SUCCESS);

            if ( sOthersFound == ID_TRUE )
            {
                IDE_TEST( qsxEnv::endOthersClause( QC_QSX_ENV(aQcStmt) )
                          != IDE_SUCCESS );
            }

            // sFlag가 1인 것은 SDU_SHARD_ENABLE == 1 을 포함함.
            if ( sFlag == 1 )
            {
                sFlag = 0;
                /* ================= BEGIN error adjustment =======================
                 *  완료 전까지 EXCEPTION이 발생하면 안됨
                 *  1. SPXENV => ERRMGR (FREE LIST)
                 *  2. SPXENV INIT
                 *  3. Exception handler내에서 error 발생 유무에 따라 동작을 달리함
                 *    3-1. ERROR 발생X
                 *         BACKUP => SPXENV
                 *    3-2. ERROR 발생O
                 *         BACKUP => ERRMGR (FREE LIST)
                 */
                // 1. SPXENV => ERRMGR (FREE LIST)
                sErrors->mFreeListCnt += aQcStmt->spxEnv->mErrorListCount;
                IDU_LIST_JOIN_LIST( &(sErrors->mFreeList), &(aQcStmt->spxEnv->mErrorList) );
                // 2. SPXENV INIT
                aQcStmt->spxEnv->mErrorListCount = 0;

                // 3. BACKUP 처리
                if ( sErrors->mErrorListCnt != 0 ) // 3-1. 다른 error가 발생한 경우
                {
                    
                    sErrors->mFreeListCnt += sOrgErrorListCount;
                    IDU_LIST_JOIN_LIST( &(sErrors->mFreeList), &sOrgErrorList );
                }
                else // 3-2. 다른 error가 발생하지 않은 경우
                {
                    aQcStmt->spxEnv->mErrorListCount = sOrgErrorListCount;
                    IDU_LIST_JOIN_LIST( &(aQcStmt->spxEnv->mErrorList), &sOrgErrorList );
                }
                /* =================== END error adjustment ======================= */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_flow_id_invalid);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                "aExecInfo->mFlowId is invalid"));
    }
    IDE_EXCEPTION(err_flow_id_is_not_set);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                "aExecInfo->mFlowId is not set by validation step"));
    }
    IDE_EXCEPTION_END;

    if (sOthersFound == ID_TRUE)
    {
        (void) qsxEnv::endOthersClause( QC_QSX_ENV(aQcStmt) );
    }

    // catchException에서 exception으로 오는 경우는 critical error거나 internal error이다.
    // 따라서 일반적인 경우에는 이곳으로 오는 경우는 없다.
    // sFlag가 1인 것은 SDU_SHARD_ENABLE == 1 을 포함함.
    if ( sFlag == 1 )
    {
        // 1. SPXENV => ERRMGR (FREE LIST)
        sErrors->mFreeListCnt += aQcStmt->spxEnv->mErrorListCount;
        IDU_LIST_JOIN_LIST( &(sErrors->mFreeList), &(aQcStmt->spxEnv->mErrorList) );
        // 2. BACKUP => ERRMGR (FREE LIST)
        sErrors->mFreeListCnt += sOrgErrorListCount;
        IDU_LIST_JOIN_LIST( &(sErrors->mFreeList), &sOrgErrorList );
    }

    return IDE_FAILURE;
}



IDE_RC qsxExecutor::execInvoke (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcSql)
{
    qsExecParseTree  * sExecParse;
    qsProcStmtSql    * sProcSql;
    qcuSqlSourceInfo   sSqlInfo;

    sProcSql = ( qsProcStmtSql * ) aProcSql;

    sExecParse = (qsExecParseTree*) sProcSql->parseTree;

    if( qsxEnv::invoke( QC_QSX_ENV(aQcStmt),
                        aQcStmt,
                        sExecParse->procOID,
                        sExecParse->pkgBodyOID,
                        sExecParse->subprogramID,
                        sExecParse->callSpecNode ) // bind variables
        != IDE_SUCCESS )
    {
        /* BUG-43160
           unhandling 된 exception value가 package exception value일 수도 있다.
           따라서, error를 보고 unhandling error일 경우,
           qsxEnvInfo에 저장해 놓고 정보를 qsxExecutorInfo에 셋팅해준다.
           execInvoke에서 실행기간 사용된 qsxExecutorInfo와 aExecInfo는 다른 qsxExecutorInfo이다. */
        if ( ideGetErrorCode() != qpERR_ABORT_QSX_UNHANDLED_EXCEPTION )
        {
            IDE_RAISE(err_pass_wrap_sqltext);
        }
        else
        {
            (void)setRaisedExcpErrorMsg( aExecInfo,
                                         aQcStmt,
                                         aProcSql,
                                         &sSqlInfo,
                                         ID_TRUE );

            aExecInfo->mFlow   = QSX_FLOW_RAISE;
            aExecInfo->mFlowId = QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpId;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS ;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                           aQcStmt,
                                           aProcSql,
                                           &sSqlInfo,
                                           ID_TRUE );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execSelect (
    qsxExecutorInfo    * aExecInfo,
    qcStatement        * aQcStmt,
    qsProcStmts        * aProcSql)
{
    iduMemoryStatus    sQmxMemStatus;
    SInt               sStage = 0;
    qsProcStmtSql    * sProcSql;
    QCD_HSTMT          sHstmt;
    qciStmtType        sStmtType;
    vSLong             sAffectedRowCount;
    idBool             sResultSetExist;
    idBool             sRecordExist;
    qcuSqlSourceInfo   sSqlInfo;
    qcStatement      * sExecQcStmt = NULL;
    idBool             sIsFirst    = ID_TRUE;
    idBool             sIsNeedFree = ID_FALSE;
    UInt               sPoolCount  = 0;
    qsxStmtList      * sStmtList   = aQcStmt->spvEnv->mStmtList;
    qsxStmtList2     * sStmtList2  = aQcStmt->spvEnv->mStmtList2;

    sProcSql = (qsProcStmtSql*) aProcSql;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    qcd::initStmt(&sHstmt);

    // BUG-36203 PSM Optimize
    if ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) )
    {
        IDU_FIT_POINT_RAISE( "qsxExecutor::execSelect::is_unused", err_sql_index );
        sPoolCount = aQcStmt->session->mQPSpecific.mStmtListInfo.mStmtPoolCount;
        IDE_TEST_RAISE( sProcSql->sqlIdx >= sPoolCount, err_sql_index );

        if ( sStmtList != NULL )
        {
            // BUG-43158 Enhance statement list caching in PSM
            if ( QSX_STMT_LIST_IS_UNUSED( sStmtList->mStmtPoolStatus, sProcSql->sqlIdx)
                 == ID_TRUE )
            {
                // stmt alloc
                IDE_TEST( qcd::allocStmt( aQcStmt, &sHstmt )
                          != IDE_SUCCESS );

                sIsNeedFree = ID_TRUE;

                sStmtList->mStmtPool[sProcSql->sqlIdx] = sHstmt;
            }
            else
            {
                sHstmt = (void*)sStmtList->mStmtPool[sProcSql->sqlIdx];

                sIsFirst = ID_FALSE;
            }
        }

        if ( sStmtList2 != NULL )
        {
            // BUG-43158 Enhance statement list caching in PSM
            if ( QSX_STMT_LIST_IS_UNUSED( sStmtList2->mStmtPoolStatus, sProcSql->sqlIdx)
                 == ID_TRUE )
            {
                // stmt alloc
                IDE_TEST( qcd::allocStmt( aQcStmt, &sHstmt )
                          != IDE_SUCCESS );

                sIsNeedFree = ID_TRUE;
                sStmtList2->mStmtPool[sProcSql->sqlIdx] = sHstmt;
            }
            else
            {
                sHstmt = (void*)sStmtList2->mStmtPool[sProcSql->sqlIdx];
                sIsFirst = ID_FALSE;
            }
        }
    }
    else
    {
        // stmt alloc
        IDE_TEST( qcd::allocStmt( aQcStmt,
                                  &sHstmt )
                  != IDE_SUCCESS );

        sIsNeedFree = ID_TRUE;
    }

    sStage = 2;

    /* PROJ-2197 PSM Renewal
     * 1. mmStmt에서 qcStmt를 얻어와서
     * 2. callDepth를 설정한다.
     * qcd를 통해 실행하면 qcStmt의 상속관계가 없기 때문에
     * stack overflow로 server가 비정상 종료할 수 있다. */
    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sExecQcStmt )
              != IDE_SUCCESS );

    // BUG-36203 PSM Optimize
    if( sIsFirst == ID_TRUE )
    {
        // stmt prepare
        IDE_TEST( qcd::prepare( sHstmt,
                                aQcStmt,
                                sExecQcStmt,
                                &sStmtType,
                                sProcSql->sqlText,
                                sProcSql->sqlTextLen,
                                ID_FALSE )
                  != IDE_SUCCESS );

        aExecInfo->mSqlCursorInfo->mStmtType = QCI_STMT_SELECT;

        IDE_TEST( qcd::checkBindParamCount( sHstmt,
                                            (UShort)sProcSql->usingParamCount )
                  != IDE_SUCCESS );

        // BUG-43158 Enhance statement list caching in PSM
        if ( ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) ) &&
             ( sIsNeedFree == ID_TRUE ) )
        {
            IDU_FIT_POINT_RAISE( "qsxExecutor::execSelect::set_used", err_sql_index );
            IDE_TEST_RAISE( sProcSql->sqlIdx >= sPoolCount, err_sql_index );
            sIsNeedFree = ID_FALSE;
            if ( sStmtList != NULL )
            {
                QSX_STMT_LIST_SET_USED( sStmtList->mStmtPoolStatus, sProcSql->sqlIdx);
            }
            if ( sStmtList2 != NULL )
            {
                QSX_STMT_LIST_SET_USED( sStmtList2->mStmtPoolStatus, sProcSql->sqlIdx);
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qsxExecutor::bindParam( aQcStmt,
                                      sHstmt,
                                      sProcSql->usingParams,
                                      NULL,
                                      sIsFirst )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcSql->intoVariables != NULL );

    if( sProcSql->intoVariables->bulkCollect == ID_TRUE )
    {
        // BUG-46032
        IDE_TEST( qsxUtil::preCalculateArray (
                aQcStmt,
                sProcSql->intoVariables->intoNodes )
            != IDE_SUCCESS );

        IDE_TEST( qsxUtil::truncateArray(
                    aQcStmt,
                    sProcSql->intoVariables->intoNodes )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qcd::execute( sHstmt,
                            aQcStmt,
                            NULL /* out param */,
                            &sAffectedRowCount,
                            &sResultSetExist,
                            &sRecordExist,
                            sIsFirst )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( processIntoClause( aExecInfo,
                                 aQcStmt,
                                 sHstmt,
                                 sProcSql->intoVariables,
                                 sProcSql->isIntoVarRecordType,
                                 sRecordExist )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( qcd::endFetch( sHstmt )
              != IDE_SUCCESS );

    sStage = 1;

    if( sIsNeedFree == ID_TRUE )
    {
        (void)qcd::freeStmt( sHstmt, ID_TRUE );
        sHstmt = NULL;
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_sql_index)
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                 "[qsxExecutor::execSelect_sql_index_is_wrong]"));
    }
    IDE_EXCEPTION_END;

    (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                       aQcStmt,
                                       aProcSql,
                                       &sSqlInfo,
                                       ID_TRUE );

    switch( sStage )
    {
        case 3:
            (void)qcd::endFetch( sHstmt );
        case 2:
            if( sIsNeedFree == ID_TRUE )
            {
                (void)qcd::freeStmt( sHstmt,
                                     ID_TRUE );
            }
            else
            {
                // Nothing to do.
            }
        case 1:
            if( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execUpdate (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcSql)
{
#define IDE_FN "IDE_RC qsxExecutor::execUpdate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( execNonSelectDML ( aExecInfo,
                                 aQcStmt,
                                 aProcSql )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxExecutor::execDelete (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcSql)
{
#define IDE_FN "IDE_RC qsxExecutor::execDelete"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( execNonSelectDML ( aExecInfo,
                                 aQcStmt,
                                 aProcSql )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxExecutor::execInsert (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcSql)
{
#define IDE_FN "IDE_RC qsxExecutor::execInsert"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( execNonSelectDML ( aExecInfo,
                                 aQcStmt,
                                 aProcSql )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxExecutor::execMove (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcSql)
{
#define IDE_FN "IDE_RC qsxExecutor::execMove"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( execNonSelectDML ( aExecInfo,
                                 aQcStmt,
                                 aProcSql )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxExecutor::execMerge (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcSql)
{
#define IDE_FN "IDE_RC qsxExecutor::execMergee"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( execNonSelectDML ( aExecInfo,
                                 aQcStmt,
                                 aProcSql )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxExecutor::execNonSelectDML (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcSql)
{
    iduMemoryStatus    sQmxMemStatus;
    SInt               sStage = 0;
    qsProcStmtSql    * sProcSql;
    QCD_HSTMT          sHstmt;
    qciStmtType        sStmtType;
    vSLong             sAffectedRowCount;
    idBool             sResultSetExist;
    idBool             sRecordExist;
    qcuSqlSourceInfo   sSqlInfo;
    qcStatement      * sExecQcStmt = NULL;
    idBool             sIsFirst    = ID_TRUE;
    idBool             sIsNeedFree = ID_FALSE;
    UInt               sPoolCount = 0;
    qsxStmtList      * sStmtList  = aQcStmt->spvEnv->mStmtList;
    qsxStmtList2     * sStmtList2 = aQcStmt->spvEnv->mStmtList2;
    qciBindData      * sOutBindParamDataList = NULL;

    sProcSql = (qsProcStmtSql*) aProcSql;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST_RAISE( ((aQcStmt->spxEnv->mFlag & QSX_ENV_DURING_SELECT) ==
                    QSX_ENV_DURING_SELECT),
                    ERR_PSM_INSIDE_QUERY );

    // BUG-36203 PSM Optimize
    QSX_CURSOR_SET_ROWCOUNT_NULL( aExecInfo->mSqlCursorInfo );

    qcd::initStmt(&sHstmt);

    // BUG-36203 PSM Optimize
    if ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) )
    {
        IDU_FIT_POINT_RAISE( "qsxExecutor::execNonSelectDML::is_unused", err_sql_index );
        sPoolCount = aQcStmt->session->mQPSpecific.mStmtListInfo.mStmtPoolCount;
        IDE_TEST_RAISE( sProcSql->sqlIdx >= sPoolCount, err_sql_index );

        if ( sStmtList != NULL )
        {
            // BUG-43158 Enhance statement list caching in PSM
            if ( QSX_STMT_LIST_IS_UNUSED( sStmtList->mStmtPoolStatus, sProcSql->sqlIdx)
                 == ID_TRUE )
            {
                // stmt alloc
                IDE_TEST( qcd::allocStmt( aQcStmt, &sHstmt )
                          != IDE_SUCCESS );

                sIsNeedFree = ID_TRUE;
                sStmtList->mStmtPool[sProcSql->sqlIdx] = sHstmt;
            }
            else
            {
                sHstmt = (void*)sStmtList->mStmtPool[sProcSql->sqlIdx];
                sIsFirst = ID_FALSE;
            }
        }

        if ( sStmtList2 != NULL )
        {
            // BUG-43158 Enhance statement list caching in PSM
            if ( QSX_STMT_LIST_IS_UNUSED( sStmtList2->mStmtPoolStatus, sProcSql->sqlIdx)
                 == ID_TRUE )
            {
                // stmt alloc
                IDE_TEST( qcd::allocStmt( aQcStmt, &sHstmt )
                          != IDE_SUCCESS );

                sIsNeedFree = ID_TRUE;
                sStmtList2->mStmtPool[sProcSql->sqlIdx] = sHstmt;
            }
            else
            {
                sHstmt = (void*)sStmtList2->mStmtPool[sProcSql->sqlIdx];
                sIsFirst = ID_FALSE;
            }
        }
    }
    else
    {
        // stmt alloc
        IDE_TEST( qcd::allocStmt( aQcStmt,
                                  &sHstmt )
                  != IDE_SUCCESS );

        sIsNeedFree = ID_TRUE;
    }

    sStage = 2;

    /* PROJ-2197 PSM Renewal
     * 1. mmStmt에서 qcStmt를 얻어와서
     * 2. callDepth를 설정한다.
     * qcd를 통해 실행하면 qcStmt의 상속관계가 없기 때문에
     * stack overflow로 server가 비정상 종료할 수 있다. */
    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sExecQcStmt )
              != IDE_SUCCESS );

    // BUG-36203 PSM Optimize
    if( sIsFirst == ID_TRUE )
    {
        // stmt prepare
        IDE_TEST( qcd::prepare( sHstmt,
                                aQcStmt,
                                sExecQcStmt,
                                &sStmtType,
                                sProcSql->sqlText,
                                sProcSql->sqlTextLen,
                                ID_FALSE )
                  != IDE_SUCCESS );

        aExecInfo->mSqlCursorInfo->mStmtType = sStmtType;

        IDE_TEST( qcd::checkBindParamCount( sHstmt,
                                            (UShort)sProcSql->usingParamCount )
                  != IDE_SUCCESS );

        // BUG-43158 Enhance statement list caching in PSM
        if ( ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) )&&
             ( sIsNeedFree == ID_TRUE ) )
        {
            IDU_FIT_POINT_RAISE( "qsxExecutor::execNonSelectDML::set_used", err_sql_index );
            IDE_TEST_RAISE( sProcSql->sqlIdx >= sPoolCount, err_sql_index );
            sIsNeedFree = ID_FALSE;

            if ( sStmtList != NULL )
            {
                QSX_STMT_LIST_SET_USED( sStmtList->mStmtPoolStatus, sProcSql->sqlIdx);
            }
            if ( sStmtList2 != NULL )
            {
                QSX_STMT_LIST_SET_USED( sStmtList2->mStmtPoolStatus, sProcSql->sqlIdx);
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qsxExecutor::bindParam( aQcStmt,
                                      sHstmt,
                                      sProcSql->usingParams,
                                      &sOutBindParamDataList,
                                      sIsFirst )
              != IDE_SUCCESS );

    IDE_DASSERT( sExecQcStmt->parentInfo->parentTmplate    == NULL );
    IDE_DASSERT( sExecQcStmt->parentInfo->parentReturnInto == NULL );

    switch( sProcSql->common.stmtType )
    {
        case QS_PROC_STMT_INSERT:
            sExecQcStmt->parentInfo->parentTmplate = QC_PRIVATE_TMPLATE(aQcStmt);
            sExecQcStmt->parentInfo->parentReturnInto = ((qmmInsParseTree*)sProcSql->parseTree)->returnInto;
            break;
        case QS_PROC_STMT_UPDATE:
            sExecQcStmt->parentInfo->parentTmplate = QC_PRIVATE_TMPLATE(aQcStmt);
            sExecQcStmt->parentInfo->parentReturnInto = ((qmmUptParseTree*)sProcSql->parseTree)->returnInto;
            break;
        case QS_PROC_STMT_DELETE:
            sExecQcStmt->parentInfo->parentTmplate = QC_PRIVATE_TMPLATE(aQcStmt);
            sExecQcStmt->parentInfo->parentReturnInto = ((qmmDelParseTree*)sProcSql->parseTree)->returnInto;
            break;
        default :
            break;
    }

    // BUG-37011
    // associative array를 truncate 해야 한다.
    if( sExecQcStmt->parentInfo->parentReturnInto != NULL )
    {
        if( sExecQcStmt->parentInfo->parentReturnInto->bulkCollect == ID_TRUE )
        {
            IDE_TEST( qsxUtil::truncateArray(
                        aQcStmt,
                        sExecQcStmt->parentInfo->parentReturnInto->returnIntoValue->returningInto )
                      != IDE_SUCCESS );
        }
        else
        {
            // bulk collect into가 아니면 bind-execute로 동작한다.
            sExecQcStmt->parentInfo->parentTmplate = NULL;
            sExecQcStmt->parentInfo->parentReturnInto = NULL;
        }
    }

    IDE_TEST( qcd::execute( sHstmt,
                            aQcStmt,
                            sOutBindParamDataList /* out param */,
                            &sAffectedRowCount,
                            &sResultSetExist,
                            &sRecordExist,
                            sIsFirst )
              != IDE_SUCCESS );

    QSX_CURSOR_SET_ROWCOUNT( aExecInfo->mSqlCursorInfo,
                             sAffectedRowCount );

    sStage = 1;
    sExecQcStmt->parentInfo->parentTmplate    = NULL;
    sExecQcStmt->parentInfo->parentReturnInto = NULL;

    if( sIsNeedFree == ID_TRUE )
    {
        (void)qcd::freeStmt( sHstmt, ID_TRUE );
        sHstmt = NULL;
    }
    else
    {
        // Nothing to do.
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_PSM_INSIDE_QUERY)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_PSM_INSIDE_QUERY));
    }
    IDE_EXCEPTION(err_sql_index)
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                 "[qsxExecutor::execNonSelectDML_sql_index_is_wrong]"));
    }
    IDE_EXCEPTION_END;

    (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                       aQcStmt,
                                       aProcSql,
                                       &sSqlInfo,
                                       ID_TRUE );

    switch( sStage )
    {
        case 2:
            if ( sExecQcStmt != NULL )
            {
                sExecQcStmt->parentInfo->parentTmplate    = NULL;
                sExecQcStmt->parentInfo->parentReturnInto = NULL;
            }
            else
            {
                // Nothing to do.
            }

            if( sIsNeedFree == ID_TRUE )
            {
                (void)qcd::freeStmt( sHstmt,
                                     ID_TRUE );
            }
            else
            {
                // Nothing to do.
            }
        case 1:
            if( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execSavepoint (
    qsxExecutorInfo * /* aExecInfo */,
    qcStatement     * aQcStmt,
    qsProcStmts     * aProcSql)
{
#define IDE_FN "IDE_RC qsxExecutor::execSavepoint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTransParseTree * sTransParse;
    qsProcStmtSql    * sProcSql;
    SChar              sSavepointName [QC_MAX_OBJECT_NAME_LEN + 1];

    sProcSql    = (qsProcStmtSql *)aProcSql;
    sTransParse = (qdTransParseTree *)sProcSql->parseTree;

    QC_STR_COPY( sSavepointName, sTransParse->savepointName );

    // TASK-7244 Check restrictions for Sharding
    if ( SDU_SHARD_ENABLE == 1 )
    {
        IDE_TEST( checkRestrictionsForShard( aQcStmt ) != IDE_SUCCESS );
    }

    IDE_TEST( qsxEnv::savepoint( QC_QSX_ENV( aQcStmt ) ,
                                 sSavepointName )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxExecutor::execCommit (
    qsxExecutorInfo * /* aExecInfo */,
    qcStatement     * aQcStmt,
    qsProcStmts     * /* aProcSql */)
{
    IDE_TEST_RAISE( ((aQcStmt->spxEnv->mFlag & QSX_ENV_DURING_SELECT) ==
                     QSX_ENV_DURING_SELECT),
                    ERR_PSM_INSIDE_QUERY );

    // TASK-7244 Check restrictions for Sharding
    if ( SDU_SHARD_ENABLE == 1 )
    {
        IDE_TEST( checkRestrictionsForShard( aQcStmt ) != IDE_SUCCESS );
    }

    IDE_TEST( qsxEnv::commit( QC_QSX_ENV( aQcStmt ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION(ERR_PSM_INSIDE_QUERY)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_PSM_INSIDE_QUERY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execRollback (
    qsxExecutorInfo * /* aExecInfo */,
    qcStatement     * aQcStmt,
    qsProcStmts     * aProcSql)
{
    qdTransParseTree * sTransParse;
    qsProcStmtSql    * sProcSql;
    SChar              sSavepointName [QC_MAX_OBJECT_NAME_LEN + 1];

    IDE_TEST_RAISE( ((aQcStmt->spxEnv->mFlag & QSX_ENV_DURING_SELECT) ==
                     QSX_ENV_DURING_SELECT),
                    ERR_PSM_INSIDE_QUERY );

    // TASK-7244 Check restrictions for Sharding
    if ( SDU_SHARD_ENABLE == 1 )
    {
        IDE_TEST( checkRestrictionsForShard( aQcStmt ) != IDE_SUCCESS );
    }

    sProcSql    = (qsProcStmtSql *)aProcSql;
    sTransParse = (qdTransParseTree *)sProcSql->parseTree;

    if ( QC_IS_NULL_NAME( sTransParse->savepointName ) != ID_TRUE )
    {
        QC_STR_COPY( sSavepointName, sTransParse->savepointName );
    }
    else
    {
        sSavepointName[0] = '\0';
    }

    IDE_TEST( qsxEnv::rollback( QC_QSX_ENV( aQcStmt ),
                                aQcStmt,
                                sSavepointName )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION(ERR_PSM_INSIDE_QUERY)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_PSM_INSIDE_QUERY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execIf (
    qsxExecutorInfo     * aExecInfo ,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcIf)
{
#define QSX_IF ( ( qsProcStmtIf * ) aProcIf )

    iduMemoryStatus    sQmxMemStatus;
    idBool             sBoolValue;
    qcuSqlSourceInfo   sSqlInfo;
    SInt               sStage = 0;
    qsProcStmtSql    * sProcSql;
    QCD_HSTMT          sHstmt;
    qciStmtType        sStmtType;
    vSLong             sAffectedRowCount;
    idBool             sResultSetExist;
    idBool             sRecordExist;
    idBool             sIsFirst    = ID_TRUE;
    idBool             sIsNeedFree = ID_FALSE;
    qcStatement      * sExecQcStmt = NULL;
    UInt               sPoolCount  = 0;
    qsxStmtList      * sStmtList  = aQcStmt->spvEnv->mStmtList;
    qsxStmtList2     * sStmtList2 = aQcStmt->spvEnv->mStmtList2;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    // condition
    if ( QSX_IF->conditionType == QS_CONDITION_NORMAL )
    {
        if ( qsxUtil::calculateBoolean(
                QSX_IF->conditionNode,
                QC_PRIVATE_TMPLATE(aQcStmt),
                & sBoolValue )
            != IDE_SUCCESS )
        {
            sSqlInfo.setSourceInfo( aQcStmt,
                                    & QSX_IF->conditionNode->position );
            IDE_RAISE(err_pass_wrap_sqltext);
        }
    }
    else
    {
        sProcSql = (qsProcStmtSql *) QSX_IF->existsSql;

        qcd::initStmt(&sHstmt);

        // BUG-36203 PSM Optimize
        if ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) )
        {
            IDU_FIT_POINT_RAISE( "qsxExecutor::execIf::is_unused", err_sql_index );
            sPoolCount = aQcStmt->session->mQPSpecific.mStmtListInfo.mStmtPoolCount;
            IDE_TEST_RAISE( sProcSql->sqlIdx >= sPoolCount, err_sql_index );

            if ( sStmtList != NULL )
            {
                // BUG-43158 Enhance statement list caching in PSM
                if ( QSX_STMT_LIST_IS_UNUSED( sStmtList->mStmtPoolStatus, sProcSql->sqlIdx)
                     == ID_TRUE )
                {
                    // stmt alloc
                    IDE_TEST( qcd::allocStmt( aQcStmt, &sHstmt )
                              != IDE_SUCCESS );

                    sIsNeedFree = ID_TRUE;
                    sStmtList->mStmtPool[sProcSql->sqlIdx]= sHstmt;
                }
                else
                {
                    sHstmt = (void*)sStmtList->mStmtPool[sProcSql->sqlIdx];
                    sIsFirst = ID_FALSE;
                }
            }

            if ( sStmtList2 != NULL )
            {
                // BUG-43158 Enhance statement list caching in PSM
                if ( QSX_STMT_LIST_IS_UNUSED( sStmtList2->mStmtPoolStatus, sProcSql->sqlIdx)
                     == ID_TRUE )
                {
                    // stmt alloc
                    IDE_TEST( qcd::allocStmt( aQcStmt, &sHstmt )
                              != IDE_SUCCESS );

                    sIsNeedFree = ID_TRUE;
                    sStmtList2->mStmtPool[sProcSql->sqlIdx]= sHstmt;
                }
                else
                {
                    sHstmt = (void*)sStmtList2->mStmtPool[sProcSql->sqlIdx];
                    sIsFirst = ID_FALSE;
                }
            }
        }
        else
        {
            // stmt alloc
            IDE_TEST( qcd::allocStmt( aQcStmt,
                                      &sHstmt )
                      != IDE_SUCCESS );
            sIsNeedFree = ID_TRUE;
        }

        sStage = 2;

        /* PROJ-2197 PSM Renewal
         * 1. mmStmt에서 qcStmt를 얻어와서
         * 2. callDepth를 설정한다.
         * qcd를 통해 실행하면 qcStmt의 상속관계가 없기 때문에
         * stack overflow로 server가 비정상 종료할 수 있다. */
        IDE_TEST( qcd::getQcStmt( sHstmt,
                                  &sExecQcStmt )
                  != IDE_SUCCESS );

        // BUG-36203 PSM Optimize
        if( sIsFirst == ID_TRUE )
        {
            // stmt prepare
            IDE_TEST( qcd::prepare( sHstmt,
                                    aQcStmt,
                                    sExecQcStmt,
                                    &sStmtType,
                                    sProcSql->sqlText,
                                    sProcSql->sqlTextLen,
                                    ID_FALSE )
                      != IDE_SUCCESS );

            IDE_TEST( qcd::checkBindParamCount( sHstmt,
                                                (UShort)sProcSql->usingParamCount )
                      != IDE_SUCCESS );

            // BUG-43158 Enhance statement list caching in PSM
            if ( ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) )&&
                 ( sIsNeedFree == ID_TRUE ) )
            {
                IDU_FIT_POINT_RAISE( "qsxExecutor::execIf::set_used", err_sql_index );
                IDE_TEST_RAISE( sProcSql->sqlIdx >= sPoolCount, err_sql_index );
                sIsNeedFree = ID_FALSE;
                if ( sStmtList != NULL )
                {
                    QSX_STMT_LIST_SET_USED( sStmtList->mStmtPoolStatus, sProcSql->sqlIdx);
                }
                if ( sStmtList2 != NULL )
                {
                    QSX_STMT_LIST_SET_USED( sStmtList2->mStmtPoolStatus, sProcSql->sqlIdx);
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( qsxExecutor::bindParam( aQcStmt,
                                          sHstmt,
                                          sProcSql->usingParams,
                                          NULL,
                                          sIsFirst )
                  != IDE_SUCCESS );

        IDE_TEST( qcd::execute( sHstmt,
                                aQcStmt,
                                NULL /* out param */,
                                &sAffectedRowCount,
                                &sResultSetExist,
                                &sRecordExist,
                                sIsFirst )
                  != IDE_SUCCESS );

        if ( QSX_IF->conditionType == QS_CONDITION_EXISTS )
        {
            if( sRecordExist != 0 )
            {
                sBoolValue = ID_TRUE;
            }
            else
            {
                sBoolValue = ID_FALSE;
            }
        }
        else
        {
            IDE_DASSERT( QSX_IF->conditionType == QS_CONDITION_NOT_EXISTS );

            if( sRecordExist != 0 )
            {
                sBoolValue = ID_FALSE;
            }
            else
            {
                sBoolValue = ID_TRUE;
            }
        }

        IDE_TEST( qcd::endFetch( sHstmt )
                  != IDE_SUCCESS );

        sStage = 1;
        if( sIsNeedFree == ID_TRUE )
        {
            (void)qcd::freeStmt( sHstmt, ID_TRUE );
            sHstmt = NULL;
        }
    }

    if ( sBoolValue == ID_TRUE )
    {
        IDE_TEST( execStmtList ( aExecInfo,
                                 aQcStmt,
                                 QSX_IF->thenStmt )
                  != IDE_SUCCESS );
    }
    else
    {
        if( QSX_IF->elseStmt != NULL )
        {
            IDE_TEST( execStmtList ( aExecInfo,
                                     aQcStmt,
                                     QSX_IF->elseStmt )
                      != IDE_SUCCESS );
        }
        else
        {
            // else 구문은 없을 수도 있다.
        }
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                           aQcStmt,
                                           aProcIf,
                                           &sSqlInfo,
                                           ID_FALSE );
    }
    IDE_EXCEPTION(err_sql_index)
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                 "[qsxExecutor::execIf_sql_index_is_wrong]"));
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 2:
            if( sIsNeedFree == ID_TRUE )
            {
                (void)qcd::freeStmt( sHstmt,
                                     ID_TRUE );
            }
        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        default:
            break;
    }
    sStage = 0;

    return IDE_FAILURE;

#undef QSX_IF
}

IDE_RC qsxExecutor::execThen (
    qsxExecutorInfo     * aExecInfo ,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcThen)
{
#define IDE_FN "IDE_RC qsxExecutor::execThen"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#define QSX_THEN ( ( qsProcStmtThen * ) aProcThen )

    IDE_TEST( execStmtList ( aExecInfo,
                             aQcStmt,
                             QSX_THEN->thenStmts )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef QSX_THEN

#undef IDE_FN
}

IDE_RC qsxExecutor::execElse (
    qsxExecutorInfo     * aExecInfo ,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcElse)
{
#define IDE_FN "IDE_RC qsxExecutor::execElse"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#define QSX_ELSE ( ( qsProcStmtElse * ) aProcElse )

    IDE_TEST( execStmtList ( aExecInfo,
                             aQcStmt,
                             QSX_ELSE->elseStmts )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef QSX_ELSE

#undef IDE_FN
}

IDE_RC qsxExecutor::execWhile (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcWhile)
{
#define IDE_FN "IDE_RC qsxExecutor::execWhile"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#define QSX_WHILE ( ( qsProcStmtWhile * ) aProcWhile )

    iduMemoryStatus     sQmxMemStatus;
    iduMemoryStatus     sLoopMemStatus;
    idBool              sBoolValue;
    qcuSqlSourceInfo    sSqlInfo;
    SInt                sStage = 0;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    for (;;)
    {
        // check QUERY_TIMEOUT, connection
        IDE_TEST( iduCheckSessionEvent( aQcStmt->mStatistics )
                  != IDE_SUCCESS );

        IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sLoopMemStatus )
                  != IDE_SUCCESS );
        sStage = 2;

        // infinite loop has it'sS condition NULL
        if (QSX_WHILE->conditionNode != NULL)
        {
            if ( qsxUtil::calculateBoolean(
                    QSX_WHILE->conditionNode,
                    QC_PRIVATE_TMPLATE(aQcStmt),
                    & sBoolValue )
                != IDE_SUCCESS)
            {
                sSqlInfo.setSourceInfo( aQcStmt,
                                        & QSX_WHILE->conditionNode->position );
                IDE_RAISE(err_pass_wrap_sqltext);
            }

            if ( sBoolValue == ID_FALSE )
            {
                IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sLoopMemStatus)
                          != IDE_SUCCESS );
                break;
            }
        }

        IDE_TEST( execStmtList ( aExecInfo,
                                 aQcStmt,
                                 QSX_WHILE->loopStmts )
                  != IDE_SUCCESS );

        sStage = 1;
        IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sLoopMemStatus )
                  != IDE_SUCCESS );

        if (aExecInfo->mFlow == QSX_FLOW_EXIT)
        {
            if (aExecInfo->mExitLoopStmt == NULL ||
                aExecInfo->mExitLoopStmt == aProcWhile)
            {
                unsetFlowControl(aExecInfo);
                break;
            }
        }

        if (aExecInfo->mFlow == QSX_FLOW_CONTINUE)
        {
            if (aExecInfo->mContLoopStmt == NULL ||
                aExecInfo->mContLoopStmt == aProcWhile)
            {
                unsetFlowControl(aExecInfo);
                continue;
            }
        }

        if (aExecInfo->mFlow != QSX_FLOW_NONE)
        {
            break;
        }
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                           aQcStmt,
                                           aProcWhile,
                                           &sSqlInfo,
                                           ID_FALSE );
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }
    sStage = 0;

    return IDE_FAILURE;

#undef QSX_WHILE

#undef IDE_FN
}


IDE_RC qsxExecutor::execFor (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcFor)
{
/***********************************************************************
 *
 * Description : For loop의 execution ( fix BUG-11391 )
 *
 * Implementation :
 *    (1) lowerVar := lowerNode
 *    (2) upperVar := upperNode
 *    (3) stepVar := stepNode
 *    (4) counterVar := lowerVar( or upperVar reverse일때)
 *    (5) stepVar가 0보다 큰지 체크(isStepOkNode)
 *    (6) lowerVar <= upperVar인지 체크(isIntervalOkNode)
 *    (7) loop
 *    (7.1) lower <= counterVar <= upperVar인지 체크(conditionNode)
 *    (7.2) execStmtList
 *    (7.3) counterVar := counterVar + stepVar (newCounterNode)
 *    (7.3) flow 체크
 *    (8) end loop
 *
 ***********************************************************************/
#define IDE_FN "IDE_RC qsxExecutor::execFor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduMemoryStatus           sQmxMemStatus;
    iduMemoryStatus           sLoopMemStatus;
    idBool                    sBoolValue;
    qcuSqlSourceInfo          sSqlInfo;
    SInt                      sStage = 0;
    qsProcStmtFor           * sProcFor;

    sProcFor   = (qsProcStmtFor *) aProcFor;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    if( qsxUtil::calculateAndAssign( QC_QMX_MEM( aQcStmt ),
                                     sProcFor->lowerNode,
                                     QC_PRIVATE_TMPLATE(aQcStmt),
                                     sProcFor->lowerVar,
                                     QC_PRIVATE_TMPLATE(aQcStmt),
                                     ID_FALSE )
        != IDE_SUCCESS )
    {
        sSqlInfo.setSourceInfo( aQcStmt,
                                & sProcFor->lowerNode->position
                              );
        IDE_RAISE(err_pass_wrap_sqltext);
    }

    if( qsxUtil::calculateAndAssign( QC_QMX_MEM( aQcStmt ),
                                     sProcFor->upperNode,
                                     QC_PRIVATE_TMPLATE(aQcStmt),
                                     sProcFor->upperVar,
                                     QC_PRIVATE_TMPLATE(aQcStmt),
                                     ID_FALSE )
        != IDE_SUCCESS )
    {
        sSqlInfo.setSourceInfo( aQcStmt,
                                & sProcFor->upperNode->position
                              );
        IDE_RAISE(err_pass_wrap_sqltext);
    }

    if( qsxUtil::calculateAndAssign( QC_QMX_MEM( aQcStmt ),
                                     sProcFor->stepNode,
                                     QC_PRIVATE_TMPLATE(aQcStmt),
                                     sProcFor->stepVar,
                                     QC_PRIVATE_TMPLATE(aQcStmt),
                                     ID_FALSE )
        != IDE_SUCCESS )
    {
        sSqlInfo.setSourceInfo( aQcStmt,
                                & sProcFor->stepNode->position
                              );
        IDE_RAISE(err_pass_wrap_sqltext);
    }

    if ( qsxUtil::calculateAndAssign( QC_QMX_MEM( aQcStmt ),
                                      ( sProcFor->isReverse ?
                                        sProcFor->upperVar :
                                        sProcFor->lowerVar ),
                                      QC_PRIVATE_TMPLATE(aQcStmt),
                                      sProcFor->counterVar,
                                      QC_PRIVATE_TMPLATE(aQcStmt),
                                      ID_FALSE )
         != IDE_SUCCESS)
    {
        sSqlInfo.setSourceInfo( aQcStmt,
                                ( sProcFor->isReverse ?
                                  & sProcFor->upperNode->position :
                                  & sProcFor->lowerNode->position )
                              );
        IDE_RAISE(err_pass_wrap_sqltext);
    }


    IDE_TEST( qsxUtil::calculateBoolean( sProcFor->isStepOkNode,
                                         QC_PRIVATE_TMPLATE(aQcStmt),
                                         &sBoolValue ) != IDE_SUCCESS );

    if ( sBoolValue != ID_TRUE)
    {
        sSqlInfo.setSourceInfo( aQcStmt,
                                & sProcFor->stepNode->position );

        // BUG-42322
        (void)sSqlInfo.init( QC_QMX_MEM(aQcStmt) );
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_STEP_SIZE_LE_ZERO_SQLTEXT,
                                sSqlInfo.getErrMessage() ));
        (void)sSqlInfo.fini();

        IDE_RAISE(err_pass_wrap_sqltext);
    }

    IDE_TEST( qsxUtil::calculateBoolean( sProcFor->isIntervalOkNode,
                                         QC_PRIVATE_TMPLATE(aQcStmt),
                                         &sBoolValue ) != IDE_SUCCESS);

    // oracle does not execute for loop when interval is invalid.
    if ( sBoolValue == ID_TRUE )
    {
        for (;;)
        {
            // check QUERY_TIMEOUT, connection
            IDE_TEST( iduCheckSessionEvent( aQcStmt->mStatistics )
                      != IDE_SUCCESS );

            IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sLoopMemStatus )
                      != IDE_SUCCESS );
            sStage = 2;

            // Do not use sSqlInfo.
            // error message is hardly possible to be shown to user
            // becuase sCondNode is internal expression
            IDE_TEST( qsxUtil::calculateBoolean(
                    sProcFor->conditionNode,
                    QC_PRIVATE_TMPLATE(aQcStmt),
                    & sBoolValue )
                != IDE_SUCCESS );

            if (sBoolValue == ID_FALSE)
            {
                IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sLoopMemStatus )
                          != IDE_SUCCESS );
                break;
            }

            IDE_TEST( execStmtList ( aExecInfo,
                                     aQcStmt,
                                     sProcFor->loopStmts )
                      != IDE_SUCCESS );

            // evaluate a new counter
            // Do not use sSqlInfo.
            // error message is hardly possible to be shown to user
            // becuase sNewCounterNode is internal expression
            IDE_TEST( qsxUtil::calculateAndAssign (
                    QC_QMX_MEM( aQcStmt ),
                    sProcFor->newCounterNode,
                    QC_PRIVATE_TMPLATE(aQcStmt),
                    sProcFor->counterVar,
                    QC_PRIVATE_TMPLATE(aQcStmt),
                    ID_FALSE )
                != IDE_SUCCESS);

            sStage = 1;
            IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sLoopMemStatus )
                      != IDE_SUCCESS );

            if (aExecInfo->mFlow == QSX_FLOW_EXIT)
            {
                if (aExecInfo->mExitLoopStmt == NULL ||
                    aExecInfo->mExitLoopStmt == (qsProcStmts*)aProcFor)
                {
                    unsetFlowControl(aExecInfo);
                    break;
                }
            }

            if (aExecInfo->mFlow == QSX_FLOW_CONTINUE)
            {
                if (aExecInfo->mContLoopStmt == NULL ||
                    aExecInfo->mContLoopStmt == (qsProcStmts*)aProcFor)
                {
                    unsetFlowControl(aExecInfo);
                    continue;
                }
            }

            if (aExecInfo->mFlow != QSX_FLOW_NONE)
            {
                break;
            }
        }
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                           aQcStmt,
                                           aProcFor,
                                           &sSqlInfo,
                                           ID_FALSE );
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 2:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sLoopMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }

        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }
    sStage = 0;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxExecutor::execCursorFor (
    qsxExecutorInfo      * aExecInfo,
    qcStatement          * aQcStmt,
    qsProcStmts          * aProcCursorFor)
{
    qsProcStmtCursorFor * sProcCursorFor = ( ( qsProcStmtCursorFor * ) aProcCursorFor );
    iduMemoryStatus       sQmxMemStatus;
    qmsInto               sIntoVars;
    qsxCursorInfo       * sCursorInfo = NULL;
    UInt                  sStage = 0;
    // BUG-37981
    qsxPkgInfo          * sPkgInfo;
    qcTemplate          * sTemplate;

    if( sProcCursorFor->openCursorSpecNode->node.objectID != QS_EMPTY_OID )
    {
        // objectID의 pkgInfo를 가져온다.
        IDE_TEST( qsxPkg::getPkgInfo( sProcCursorFor->openCursorSpecNode->node.objectID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );

        // objectID의 template을 가져온다.
        IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( 
                     aQcStmt,
                     sPkgInfo,
                     QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                     QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                     &sTemplate)
                  != IDE_SUCCESS);
    }
    else
    {
        sTemplate = QC_PRIVATE_TMPLATE(aQcStmt);
    }

    sCursorInfo = (qsxCursorInfo *)QTC_TMPL_FIXEDDATA(
        sTemplate,
        sProcCursorFor->openCursorSpecNode );

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( qsxCursor::open( sCursorInfo,
                               aQcStmt,
                               (qtcNode*)sProcCursorFor->openCursorSpecNode->node.arguments,
                               sTemplate,
                               sProcCursorFor->sqlIdx,
                               aExecInfo->mIsDefiner )
              != IDE_SUCCESS );
    sStage = 2;

    for (;;)
    {
        // check QUERY_TIMEOUT, connection
        IDE_TEST( iduCheckSessionEvent( aQcStmt->mStatistics )
                  != IDE_SUCCESS );

        sIntoVars.intoNodes   = sProcCursorFor->counterRowTypeNode;
        sIntoVars.bulkCollect = ID_FALSE;

        IDE_TEST( qsxCursor::fetchInto ( sCursorInfo,
                                         aQcStmt,
                                         &sIntoVars,
                                         NULL )
                  != IDE_SUCCESS );

        if ( QSX_CURSOR_IS_FOUND( sCursorInfo ) != ID_TRUE)
        {
            break;
        }

        IDE_TEST( execStmtList ( aExecInfo,
                                 aQcStmt,
                                 sProcCursorFor->loopStmts )
                  != IDE_SUCCESS );

        if (aExecInfo->mFlow == QSX_FLOW_EXIT)
        {
            if (aExecInfo->mExitLoopStmt == NULL ||
                aExecInfo->mExitLoopStmt == aProcCursorFor)
            {
                unsetFlowControl( aExecInfo );
                break;
            }
        }

        if (aExecInfo->mFlow == QSX_FLOW_CONTINUE)
        {
            if (aExecInfo->mContLoopStmt == NULL ||
                aExecInfo->mContLoopStmt == aProcCursorFor)
            {
                unsetFlowControl( aExecInfo );
                continue;
            }
        }

        if (aExecInfo->mFlow != QSX_FLOW_NONE)
        {
            break;
        }
    }

    sStage = 1;
    IDE_TEST( qsxCursor::close( sCursorInfo,
                                aQcStmt )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 2:
            if ( qsxCursor::close( sCursorInfo,
                                   aQcStmt )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }
    sStage = 0;

    return IDE_FAILURE;
}


IDE_RC qsxExecutor::execExit (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcExit)
{
#define IDE_FN "IDE_RC qsxExecutor::execExit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#define QSX_EXIT ( ( qsProcStmtExit * ) aProcExit )

    iduMemoryStatus     sQmxMemStatus;
    idBool              sBoolValue;
    idBool              sIsExit = ID_TRUE;
    qcuSqlSourceInfo    sSqlInfo;
    SInt                sStage = 0;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    if (QSX_EXIT->conditionNode != NULL)
    {
        if ( qsxUtil::calculateBoolean(
                QSX_EXIT->conditionNode,
                QC_PRIVATE_TMPLATE(aQcStmt),
                &sBoolValue )
            != IDE_SUCCESS )
        {
            sSqlInfo.setSourceInfo( aQcStmt,
                                    & QSX_EXIT->conditionNode->position );
            IDE_RAISE(err_pass_wrap_sqltext);
        }

        if ( sBoolValue == ID_FALSE)
        {
            sIsExit = ID_FALSE;
        }
    }

    if (sIsExit)
    {
        aExecInfo->mFlow    = QSX_FLOW_EXIT;
        aExecInfo->mFlowId  = QSX_EXIT->labelID;

        aExecInfo->mExitLoopStmt = QSX_EXIT->stmt ;
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                           aQcStmt,
                                           aProcExit,
                                           &sSqlInfo,
                                           ID_FALSE );
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }
    sStage = 0;

    return IDE_FAILURE;

#undef QSX_EXIT

#undef IDE_FN
}


IDE_RC qsxExecutor::execContinue (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * /* aQcStmt */,
    qsProcStmts         * /* aProcContinue */)
{
#define IDE_FN "IDE_RC qsxExecutor::execContinue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#define QSX_CONT ( ( qsProcStmtContinue * ) aProcContinue )

    aExecInfo->mFlow = QSX_FLOW_CONTINUE;

    return IDE_SUCCESS ;

#undef QSX_CONT

#undef IDE_FN
}

IDE_RC qsxExecutor::execOpen (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcOpen)
{
    qsxCursorInfo     * sCursorInfo;
    qsProcStmtOpen    * sProcOpen = ( ( qsProcStmtOpen * ) aProcOpen );
    qsxPkgInfo        * sPkgInfo;
    qcTemplate        * sTemplate;
    qcuSqlSourceInfo    sSqlInfo;

    if( sProcOpen->openCursorSpecNode->node.objectID != QS_EMPTY_OID )
    {
        // objectID의 pkgInfo를 가져온다.
        IDE_TEST( qsxPkg::getPkgInfo( sProcOpen->openCursorSpecNode->node.objectID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );
        // objectID의 template을 가져온다.
        IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( aQcStmt,
                                                           sPkgInfo,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                                                           &sTemplate)
                  != IDE_SUCCESS);
    }
    else
    {
        sTemplate = QC_PRIVATE_TMPLATE(aQcStmt);
    }

    sCursorInfo = (qsxCursorInfo *)QTC_TMPL_FIXEDDATA(
            sTemplate,
            sProcOpen->openCursorSpecNode );

    IDE_TEST( qsxCursor::open( sCursorInfo,
                               aQcStmt,
                               (qtcNode*)sProcOpen->openCursorSpecNode->node.arguments,
                               sTemplate,
                               sProcOpen->sqlIdx,
                               aExecInfo->mIsDefiner )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxExecutor::execFetch (
    qsxExecutorInfo     * /*aExecInfo*/,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcFetch)
{
    qsProcStmtFetch   * sProcStmtFetch = (qsProcStmtFetch *)aProcFetch;
    qsxCursorInfo     * sCursorInfo;
    qsxRefCursorInfo  * sRefCursorInfo;
    qsxPkgInfo        * sPkgInfo;
    qcTemplate        * sTemplate;

    // check QUERY_TIMEOUT, connection
    IDE_TEST( iduCheckSessionEvent( aQcStmt->mStatistics )
              != IDE_SUCCESS );

    if ( sProcStmtFetch->cursorNode->node.objectID != QS_EMPTY_OID )
    {
        // objectID의 pkgInfo를 가져온다.
        IDE_TEST( qsxPkg::getPkgInfo( sProcStmtFetch->cursorNode->node.objectID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );
        // objectID의 template을 가져온다.
        IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( aQcStmt,
                                                           sPkgInfo,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                                                           &sTemplate )
                  != IDE_SUCCESS);
    }
    else
    {
        sTemplate = QC_PRIVATE_TMPLATE( aQcStmt );
    }
    
    /* BUG-41242, BUG-41707 */
    IDE_DASSERT( sProcStmtFetch->intoVariableNodes != NULL );

    if ( sProcStmtFetch->intoVariableNodes->bulkCollect == ID_TRUE )
    {
        IDE_TEST( qsxUtil::truncateArray(
                       aQcStmt,
                       sProcStmtFetch->intoVariableNodes->intoNodes )
            != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( sProcStmtFetch->isRefCursor == ID_TRUE )
    {
        sRefCursorInfo = (qsxRefCursorInfo *)QTC_TMPL_FIXEDDATA( sTemplate,
                                                                 sProcStmtFetch->cursorNode );

        IDE_TEST( qsxRefCursor::fetchInto( sRefCursorInfo,
                                           aQcStmt,
                                           sProcStmtFetch )
                  != IDE_SUCCESS );
    }
    else
    {
        sCursorInfo = (qsxCursorInfo *) QTC_TMPL_FIXEDDATA( sTemplate,
                                                            sProcStmtFetch->cursorNode );
        
        IDE_TEST( qsxCursor::fetchInto( sCursorInfo,
                                        aQcStmt,
                                        sProcStmtFetch->intoVariableNodes,
                                        sProcStmtFetch->limit )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxExecutor::execClose (
    qsxExecutorInfo     * /* aExecInfo */,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcClose)
{
    qsProcStmtClose  * sProcStmtClose;
    qsxCursorInfo    * sCursorInfo;
    qsxRefCursorInfo * sRefCursorInfo;
    qsxPkgInfo       * sPkgInfo;
    qcTemplate       * sTemplate;

    sProcStmtClose = (qsProcStmtClose*)aProcClose;

    if( sProcStmtClose->cursorNode->node.objectID != QS_EMPTY_OID )
    {
        // objectID의 pkgInfo를 가져온다.
        IDE_TEST( qsxPkg::getPkgInfo( sProcStmtClose->cursorNode->node.objectID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );
        // objectID의 template을 가져온다.
        IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( aQcStmt,
                                                           sPkgInfo,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                                                           &sTemplate)
                  != IDE_SUCCESS);
    }
    else
    {
        sTemplate = QC_PRIVATE_TMPLATE(aQcStmt);
    }

    if( sProcStmtClose->isRefCursor == ID_TRUE )
    {
        sRefCursorInfo = (qsxRefCursorInfo *)QTC_TMPL_FIXEDDATA(
            sTemplate,
            sProcStmtClose->cursorNode );

        IDE_TEST( qsxRefCursor::close( sRefCursorInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        sCursorInfo = (qsxCursorInfo *)QTC_TMPL_FIXEDDATA(
            sTemplate,
            sProcStmtClose->cursorNode );

        IDE_TEST( qsxCursor::close( sCursorInfo,
                                    aQcStmt )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxExecutor::execAssign (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcAssign)
{
    qsProcStmtAssign  * sProcAssign = NULL; 
    iduMemoryStatus     sQmxMemStatus;
    qcuSqlSourceInfo    sSqlInfo;
    SInt                sStage = 0;
    mtcStack            sAssignStack[2];

    sProcAssign = (qsProcStmtAssign *)aProcAssign;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    // fix BUG-32758
    if( qtc::calculate( sProcAssign->rightNode,
                        QC_PRIVATE_TMPLATE(aQcStmt) )
        != IDE_SUCCESS )
    {
        IDE_RAISE(err_pass_wrap_sqltext);
    }
    else
    {
        // stack의 두번째 부분에 rightNode의 결과값 세팅
        sAssignStack[1].column = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].column;
        sAssignStack[1].value = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;
    }

    // PROJ-1075
    // array type의 index참조를 위해 left node도 calculate해야 함.
    if( qtc::calculate( sProcAssign->leftNode,
                        QC_PRIVATE_TMPLATE(aQcStmt) )
        != IDE_SUCCESS )
    {
        IDE_RAISE(err_pass_wrap_sqltext);
    }
    else
    {
        // stack의 첫번째 부분에 leftNode의 결과값 세팅
        sAssignStack[0].column = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].column;
        sAssignStack[0].value = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;
    }

    if ( qsxUtil::assignValue ( QC_QMX_MEM( aQcStmt ),
                                sAssignStack[1].column,
                                sAssignStack[1].value,
                                sAssignStack[0].column,
                                sAssignStack[0].value,
                                &QC_PRIVATE_TMPLATE(aQcStmt)->tmplate,
                                sProcAssign->copyRef )
            != IDE_SUCCESS )
    {
        IDE_RAISE(err_pass_wrap_sqltext);
    }
    else
    {
        // Nothing to do.
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                           aQcStmt,
                                           aProcAssign,
                                           &sSqlInfo,
                                           ID_TRUE );
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }
    sStage = 0;

    return IDE_FAILURE;
}


IDE_RC qsxExecutor::execRaise (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcRaise)
{
/***********************************************************************
 *
 * Description : RAISE exception_name; 및 RAISE; 의 execute함수
 *
 * Implementation :
 *    (1) RAISE exception_name;은 다음 두가지 경우로 나눔
 *    (1.1) RAISE use-defined-exception_name;
 *    (1.2) RAISE system-defined-exception_name;
 *    (2) RAISE;은 다음 두가지 경우로 나눔
 *    (2.1) user-defined-exception을 re-raise하는 경우
 *    (2.2) system-defined-exception을 re-raise하는 경우
 *
 ***********************************************************************/

#define QSX_RAISE ( ( qsProcStmtRaise * ) aProcRaise )
    ideErrorCollection   * sErrors;

    if( QSX_RAISE->exception != NULL )
    {
        if ( QSX_RAISE->exception->isSystemDefinedError == ID_TRUE )
        {
            IDE_RAISE(RAISE_SYSTEM_ERROR);
        }
        else
        {
            aExecInfo->mFlow = QSX_FLOW_RAISE;
            aExecInfo->mFlowId = QSX_RAISE->exception->id;
            aExecInfo->mRecentRaise = QSX_RAISE;

            /* BUG-43160 */
            QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpOID = QSX_RAISE->exception->objectID;
            QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpId  = QSX_RAISE->exception->id;

            if ( QSX_RAISE->exception->errorCode == 0 )
            {
                // user-defined exception인 경우
                // 실제 에러가 발생하진 않고 sqlcode, sqlerrm에만
                // user-defined exception이라고 에러 세팅
                IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_USER_DEFINED_EXCEPTION) );
            }
            else
            {
                IDE_DASSERT( 0 );
                IDE_SET( ideSetErrorCode( QSX_RAISE->exception->errorCode) ); 
            }

            qsxEnv::setErrorCode( QC_QSX_ENV(aQcStmt) );
            qsxEnv::setErrorMessage( QC_QSX_ENV(aQcStmt) );
            ideClearError();
        }
    }
    else
    {
        // re-raise하는 경우.
        if( QSX_ENV_ERROR_CODE(aQcStmt->spxEnv) ==
            qpERR_ABORT_QSX_USER_DEFINED_EXCEPTION )
        {
            // User-defined Exception인 경우 flow와 flowId만 바꿈.
            aExecInfo->mFlow = QSX_FLOW_RAISE;
            aExecInfo->mFlowId = QSX_USER_DEFINED_EXCEPTION_NO;
        }
        else
        {
            // qcStatement의 qsxEnv에서 errorCode, errorMsg를 가져와서
            // raise를 해야 함
            IDE_RAISE(RAISE_CURRENT_ERROR);
        }
    }

    return IDE_SUCCESS ;

    IDE_EXCEPTION(RAISE_SYSTEM_ERROR);
    {
        IDE_SET(ideSetErrorCode(QSX_RAISE->exception->errorCode));
    }
    IDE_EXCEPTION(RAISE_CURRENT_ERROR);
    {
        IDE_SET(ideSetErrorCodeAndMsg(
                QSX_ENV_ERROR_CODE(aQcStmt->spxEnv),
                QSX_ENV_ERROR_MESSAGE(aQcStmt->spxEnv) ) );

        if ( SDU_SHARD_ENABLE == 1 )
        {
            sErrors = &(ideGetErrorMgr()->mErrors);

            if ( sErrors->mErrorListCnt == 0 )
            {
                sErrors->mErrorListCnt = aQcStmt->spxEnv->mErrorListCount;
                IDU_LIST_JOIN_LIST( &(sErrors->mErrorList), &(aQcStmt->spxEnv->mErrorList) );

                aQcStmt->spxEnv->mErrorListCount = 0;
                IDU_LIST_INIT( &(aQcStmt->spxEnv->mErrorList) );
            }
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef QSX_RAISE

}


IDE_RC qsxExecutor::execReturn (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcReturn)
{
    qsProcStmtReturn * sProcReturn   = NULL;
    iduMemoryStatus    sQmxMemStatus;
    qcuSqlSourceInfo   sSqlInfo;
    SInt               sStage = 0;

    sProcReturn = (qsProcStmtReturn *)aProcReturn;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    // [VALIDATION] ERROR LIST
    // PROCEDURE && return_expr != NULL
    // FUNCTION  && return_expr == NULL
    if ( sProcReturn->returnNode != NULL)
    {
        // 1. assign to current template.
        // later on, in execPlan, assign to source template.
        if( aExecInfo->mProcPlanTree != NULL )
        {
            if ( qsxUtil::calculateAndAssign ( QC_QMX_MEM( aQcStmt ),
                                               sProcReturn->returnNode,      // source node
                                               QC_PRIVATE_TMPLATE(aQcStmt), // source template
                                               aExecInfo->mProcPlanTree->returnTypeVar->variableTypeNode, // dest node
                                               QC_PRIVATE_TMPLATE(aQcStmt), // dest template 
                                               ID_TRUE )                     // copyRef
                 != IDE_SUCCESS )
            {
                IDE_RAISE(err_pass_wrap_sqltext);
            }

            aExecInfo->mIsReturnValueValid = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    aExecInfo->mFlow = QSX_FLOW_RETURN;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                           aQcStmt,
                                           aProcReturn,
                                           &sSqlInfo,
                                           ID_TRUE );
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }
    sStage = 0;

    return IDE_FAILURE;
}

//***********************************************************
// does nothing.
//***********************************************************

IDE_RC qsxExecutor::execLabel (
    qsxExecutorInfo   * /* aExecInfo */,
    qcStatement       * /* aQcStmt */,
    qsProcStmts       * /* aProcLabel */ )
{
#define IDE_FN "IDE_RC qsxExecutor::execLabel"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#define QSX_LABEL ( ( qsProcStmtLabel * ) aProcLabel )

    // do nothing

    return IDE_SUCCESS;

#undef QSX_LABEL

#undef IDE_FN
}


IDE_RC qsxExecutor::execNull (
    qsxExecutorInfo   * /* aExecInfo */,
    qcStatement       * /* aQcStmt */,
    qsProcStmts       * /* aProcLabel */ )
{
#define IDE_FN "IDE_RC qsxExecutor::execNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#define QSX_LABEL ( ( qsProcStmtLabel * ) aProcLabel )

    // do nothing

    return IDE_SUCCESS;

#undef QSX_LABEL

#undef IDE_FN
}


//***********************************************************
// not available statements
//***********************************************************

IDE_RC qsxExecutor::execSetTrans (
    qsxExecutorInfo  * aExecInfo,
    qcStatement      * aQcStmt,
    qsProcStmts      * aProcSql)
{
#define IDE_FN "IDE_RC qsxExecutor::execSetTrans"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( notSupportedError ( aExecInfo,
                                  aQcStmt,
                                  aProcSql )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxExecutor::execGoto (
    qsxExecutorInfo     * aExecInfo,
    qcStatement         * /* aQcStmt */,
    qsProcStmts         * aProcGoto)
{
#define IDE_FN "IDE_RC qsxExecutor::execGoto"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#define QSX_GOTO ( ( qsProcStmtGoto * ) aProcGoto )

    // PROJ-1335, To fix BUG-12475
    // GOTO는 QSX_FLOW_GOTO를 발생.
    // aExecInfo->mFlowId는 labelID
    // parent statement의 label과 같은지 검사하기 위함

    aExecInfo->mFlow = QSX_FLOW_GOTO;
    aExecInfo->mFlowId = QSX_GOTO->labelID;

    return IDE_SUCCESS;

#undef QSX_GOTO

#undef IDE_FN
}

IDE_RC qsxExecutor::execExecImm (
    qsxExecutorInfo     * aExecInfo ,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcExecImm )
{
    iduMemoryStatus     sQmxMemStatus;
    mtdCharType       * sCharData;
    qcuSqlSourceInfo    sSqlInfo;
    QCD_HSTMT           sHstmt;
    qsProcStmtExecImm * sExecImm;
    qciBindData       * sOutBindParamDataList = NULL;
    qciStmtType         sStmtType;

    idBool              sResultSetExist;
    idBool              sRecordExist;
    SInt                sStage = 0;
    vSLong              sAffectedRowCount;

    qcStatement       * sExecQcStmt = NULL;

    sExecImm = (qsProcStmtExecImm*)aProcExecImm;

    QSX_CURSOR_SET_ROWCOUNT_NULL( aExecInfo->mSqlCursorInfo );

    // stmt초기화
    qcd::initStmt(&sHstmt);

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST( qtc::calculate( sExecImm->sqlStringNode,
                              QC_PRIVATE_TMPLATE(aQcStmt) )
              != IDE_SUCCESS );

    sCharData = (mtdCharType*)(QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value);

    // stmt alloc
    IDE_TEST( qcd::allocStmt( aQcStmt,
                              &sHstmt )
              != IDE_SUCCESS );

    sStage = 2;

    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sExecQcStmt )
              != IDE_SUCCESS );

    // stmt prepare
    IDE_TEST( qcd::prepare( sHstmt,
                            aQcStmt,
                            sExecQcStmt,
                            &sStmtType,
                            (SChar*)sCharData->value,
                            sCharData->length,
                            ID_TRUE )
              != IDE_SUCCESS );

    /* BUG-45678
     * Select 중에 non select dml을 수행할 수 없다. */
    IDE_TEST_RAISE( ((aQcStmt->spxEnv->mFlag & QSX_ENV_DURING_SELECT) == QSX_ENV_DURING_SELECT) &&
                    ((sStmtType & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DCL) ,
                    ERR_PSM_INSIDE_QUERY );

    // TASK-7244 Check restrictions for Sharding
    if ( SDU_SHARD_ENABLE == 1 )
    {
        if ( ((sStmtType & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DCL) ||
             ((sStmtType & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DDL) )
        {
            IDE_TEST( checkRestrictionsForShard( aQcStmt ) != IDE_SUCCESS );
        }
    }

    aExecInfo->mSqlCursorInfo->mStmtType = sStmtType;

    if( sExecImm->intoVariableNodes != NULL ) 
    {
        if( sExecImm->intoVariableNodes->bulkCollect == ID_TRUE )
        {
            // BUG-46032
            IDE_TEST( qsxUtil::preCalculateArray (
                    aQcStmt,
                    sExecImm->intoVariableNodes->intoNodes )
                != IDE_SUCCESS );

            IDE_TEST( qsxUtil::truncateArray(
                    aQcStmt,
                    sExecImm->intoVariableNodes->intoNodes )
                != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        /* BUG-43415
           execute immediate에서 실행하고자 하는 query가 fetch type이 아닌 경우에는
           into절 또는 bulk collect into절이 있으면 에러를 발생 해 준다. */
        if ( (sStmtType == QCI_STMT_SELECT) || 
             (sStmtType == QCI_STMT_SELECT_FOR_UPDATE) ||
             (sStmtType == QCI_STMT_DEQUEUE) ||
             (sStmtType == QCI_STMT_SELECT_FOR_FIXED_TABLE) )
        {
            // Nothing to do.
        }
        else
        {
            IDE_RAISE( ERR_INTO_OR_BULK_COLLECT_INTO_NOT_ALLOWED );
        }
    }
    else
    {
        // Nothing to do.
    }
 
    IDE_TEST( qcd::checkBindParamCount( sHstmt,
                                        (UShort)sExecImm->usingParamCount )
              != IDE_SUCCESS );

    IDE_TEST( qsxExecutor::bindParam( aQcStmt,
                                      sHstmt,
                                      sExecImm->usingParams,
                                      &sOutBindParamDataList,
                                      ID_TRUE )
              != IDE_SUCCESS );
  
    IDE_TEST( qcd::execute( sHstmt,
                            aQcStmt,
                            sOutBindParamDataList,
                            &sAffectedRowCount,
                            &sResultSetExist,
                            &sRecordExist,
                            ID_TRUE )
              != IDE_SUCCESS );
    sStage = 3;

    if ( sExecImm->intoVariableNodes != NULL )
    {
        IDE_TEST( processIntoClause( aExecInfo,
                                     aQcStmt,
                                     sHstmt,
                                     sExecImm->intoVariableNodes,
                                     sExecImm->isIntoVarRecordType,
                                     sRecordExist )
                  != IDE_SUCCESS );
        sStage = 2;

        IDE_TEST( qcd::endFetch( sHstmt )
                  != IDE_SUCCESS );
    } /* sExecImm->intoVariableNodes != NULL */
    else
    {
        sStage = 2;
        QSX_CURSOR_SET_ROWCOUNT( aExecInfo->mSqlCursorInfo,
                                 sAffectedRowCount );
    }

    sStage = 1;
    (void)qcd::freeStmt( sHstmt, ID_TRUE );
    sHstmt = NULL;

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTO_OR_BULK_COLLECT_INTO_NOT_ALLOWED )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_INTO_OR_BULK_COLLECT_INTO_CLAUSE_NOT_ALLOWED) );
    }
    IDE_EXCEPTION(ERR_PSM_INSIDE_QUERY)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_PSM_INSIDE_QUERY));
    }
    IDE_EXCEPTION_END;

    (void)qsxExecutor::adjustErrorMsg( aExecInfo,
                                       aQcStmt,
                                       aProcExecImm,
                                       &sSqlInfo,
                                       ID_TRUE );

    switch( sStage )
    {
        case 3:
            (void)qcd::endFetch(sHstmt);
        case 2:
            (void)qcd::freeStmt( sHstmt,
                                 ID_TRUE );
        case 1:
            if( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
                != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            else
            {
                // Nothing to do.
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execOpenFor (
    qsxExecutorInfo     * /* aExecInfo */,
    qcStatement         * aQcStmt,
    qsProcStmts         * aProcOpenFor )
{
/***********************************************************************
 *
 *  Description : PROJ-1386 Dynamic-SQL
 *                ref cursor variable에 대해 open
 *
 *  Implementation :
 *
 ***********************************************************************/

    qsProcStmtOpenFor * sOpenFor;
    qsxRefCursorInfo  * sRefCursorInfo;
    mtdCharType       * sCharData;

    mtcStack          * sOriStackBuffer = NULL;
    mtcStack          * sOriStack       = NULL;
    SInt                sOriStackCount  = 0;
    SInt                sOriStackRemain = 0;
    qcTemplate        * sTemplate;
    qsxPkgInfo        * sPkgInfo;
    UInt                sStage = 0;

    sOpenFor = (qsProcStmtOpenFor*)aProcOpenFor;

    if( sOpenFor->refCursorVarNode->node.objectID != QS_EMPTY_OID )
    {
        // objectID의 pkgInfo를 가져온다.
        IDE_TEST( qsxPkg::getPkgInfo( sOpenFor->refCursorVarNode->node.objectID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );
        // objectID의 template을 가져온다.
        IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( aQcStmt,
                                                           sPkgInfo,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                                                           &sTemplate)
                  != IDE_SUCCESS);

        sOriStackBuffer = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackBuffer;
        sOriStack       = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack;
        sOriStackCount  = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackCount;
        sOriStackRemain = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain;

        QC_CONNECT_TEMPLATE_STACK(
            sTemplate,
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackBuffer,
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackCount,
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain );
        sTemplate->stmt = QC_PRIVATE_TMPLATE(aQcStmt)->stmt;
        sStage = 1;
    }
    else
    {
        sTemplate = QC_PRIVATE_TMPLATE(aQcStmt);
    }

    IDE_TEST( qtc::calculate( sOpenFor->refCursorVarNode,
                              sTemplate )
              != IDE_SUCCESS );

    sRefCursorInfo = (qsxRefCursorInfo*)(sTemplate->tmplate.stack[0].value);

    if ( sOpenFor->common.stmtType == QS_PROC_STMT_OPEN_FOR )
    {
        IDE_TEST( qtc::calculate( sOpenFor->sqlStringNode,
                                  QC_PRIVATE_TMPLATE(aQcStmt) )
                  != IDE_SUCCESS );

        sCharData = (mtdCharType*)( QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value);

        IDE_TEST( qsxRefCursor::openFor( sRefCursorInfo,
                                         aQcStmt,
                                         sOpenFor->usingParams,
                                         sOpenFor->usingParamCount,
                                         (SChar*)sCharData->value,
                                         sCharData->length,
                                         sOpenFor->sqlIdx )
                  != IDE_SUCCESS );
    }
    else
    {
        // BUG-42397 Ref Cursor Static SQL
        IDE_DASSERT( sOpenFor->common.stmtType == QS_PROC_STMT_OPEN_FOR_STATIC_SQL );

        IDE_TEST( qsxRefCursor::openFor( sRefCursorInfo,
                                         aQcStmt,
                                         sOpenFor->usingParams,
                                         sOpenFor->usingParamCount,
                                         sOpenFor->staticSql->sqlText,
                                         sOpenFor->staticSql->sqlTextLen,
                                         sOpenFor->sqlIdx )
                  != IDE_SUCCESS );
    }

    // package subprogram일 때만 원복한다.
    if( sOpenFor->refCursorVarNode->node.objectID != QS_EMPTY_OID ) 
    {
        sStage = 0;
        QC_DISCONNECT_TEMPLATE_STACK( sTemplate );
        sTemplate->stmt = NULL;

        /* QC_PRIVATE_TMPLATE(aQcStmt)->
                 template.stackBuffer/stack/stackCount/stackRemain 원복 */
        QC_CONNECT_TEMPLATE_STACK(
            QC_PRIVATE_TMPLATE(aQcStmt),
            sOriStackBuffer,
            sOriStack,
            sOriStackCount,
            sOriStackRemain );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sStage == 1 )
    {
        QC_DISCONNECT_TEMPLATE_STACK( sTemplate );
        sTemplate->stmt = NULL;

        /* QC_PRIVATE_TMPLATE(aQcStmt)->
           template.stackBuffer/stack/stackCount/stackRemain 원복 */
        QC_CONNECT_TEMPLATE_STACK(
            QC_PRIVATE_TMPLATE(aQcStmt),
            sOriStackBuffer,
            sOriStack,
            sOriStackCount,
            sOriStackRemain );
    }
    sStage = 0;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::notSupportedError (
    qsxExecutorInfo    * /* aExecInfo */,
    qcStatement        * aQcStmt,
    qsProcStmts        * aProcStmt )
{
#define IDE_FN "IDE_RC qsxExecutor::notSupportedError"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcuSqlSourceInfo sSqlInfo;

    sSqlInfo.setSourceInfo( aQcStmt,
                            & aProcStmt->pos ) ;

    IDE_RAISE( err_unsupported_statement );

    return IDE_SUCCESS;

    IDE_EXCEPTION ( err_unsupported_statement )
    {
        (void)sSqlInfo.init(aQcStmt->qmxMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT,
                            sSqlInfo.getErrMessage() ));
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/* BUG-24383 Support enqueue statement at PSM */
IDE_RC qsxExecutor::execEnqueue ( 
    qsxExecutorInfo    * aExecInfo,
    qcStatement        * aQcStmt,
    qsProcStmts        * aProcSql)
{
    IDE_TEST( execNonSelectDML ( aExecInfo,
                                 aQcStmt,
                                 aProcSql )
              != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qsxExecutor::findLabel( qsProcStmts *  aProcStmt,
                               SInt           aLabelID,
                               qsLabels    ** aLabel )
{
/***********************************************************************
 *
 * Description : PROJ-1335, To fix BUG-12475
 *               statement에 aLabelID와 동일한 label이 존재하는지 검사
 * Implementation :
 *
 ***********************************************************************/
    qsLabels    * sLabel;
    idBool        sFind = ID_FALSE;

    for( sLabel = aProcStmt->childLabels;
         (sLabel != NULL) && (sFind == ID_FALSE);
         sLabel = sLabel->next )
    {
        if( sLabel->id == aLabelID )
        {
            *aLabel = sLabel;
            sFind = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sFind;
}

// PROJ-1685
IDE_RC 
qsxExecutor::execExtproc( qsxExecutorInfo * aExecInfo,
                          qcStatement     * aQcStmt )
{
    qsCallSpecParam   * sParam      = NULL;
    idxExtProcMsg     * sMsg        = NULL;

    qsProcParseTree   * sParseTree;
    qcTemplate        * sTmplate;

    UInt                sIndex = 0;

    // Preparation
    sParseTree = aExecInfo->mProcPlanTree;
    sTmplate   = QC_PRIVATE_TMPLATE(aQcStmt);

    // Message allocation
    IDE_TEST( aQcStmt->qmxMem->alloc( ID_SIZEOF(idxExtProcMsg), (void **)&sMsg )
              != IDE_SUCCESS );

    // Message initialization
    QSX_EXTPROC_INIT_MSG( sMsg );

    // LibName, FuncName
    idlOS::strcpy( sMsg->mLibName,
                   sParseTree->expCallSpec->fileSpec );

    idlOS::strcpy( sMsg->mFuncName, sParseTree->expCallSpec->functionName );

    // paramCount
    sMsg->mParamCount = sParseTree->expCallSpec->paramCount;

    // paramInfo
    if( sMsg->mParamCount > 0 )
    {
        IDE_TEST( aQcStmt->qmxMem->alloc( ( ID_SIZEOF(idxParamInfo) * sMsg->mParamCount ),
                                          (void **)&sMsg->mParamInfos )
                  != IDE_SUCCESS );

        sParam = sParseTree->expCallSpec->param;

        while( sParam != NULL )
        {
            IDE_TEST( qsxExtProc::fillParamInfo( aQcStmt->qmxMem,
                                                 sParam,
                                                 sTmplate,
                                                 &sMsg->mParamInfos[sIndex],
                                                 sIndex,
                                                 sParseTree->expCallSpec->procType )
                      != IDE_SUCCESS );

            sParam = sParam->next;
            sIndex++;
        }
    }

    // returnInfo
    if( sParseTree->returnTypeVar != NULL )
    {
        // sParseTree->returnTypeVar->common 에 table/column
        IDE_TEST( qsxExtProc::fillReturnInfo( aQcStmt->qmxMem,
                                              sParseTree->returnTypeVar->common,
                                              sTmplate,
                                              &sMsg->mReturnInfo,
                                              sParseTree->expCallSpec->procType )
                  != IDE_SUCCESS );
    }
    else
    {
        /* intialize return info */
        QSX_EXTPROC_INIT_PARAM_INFO( &sMsg->mReturnInfo );

        /* set data area to zero */
        idlOS::memset( &sMsg->mReturnInfo.mD, 0, ID_SIZEOF(sMsg->mReturnInfo.mD) );
    }

    // call external procedure/function
    IDE_TEST( idxProc::callExtProc( aQcStmt->qmxMem,
                                    QCG_GET_SESSION_ID(aQcStmt),
                                    sMsg ) != IDE_SUCCESS );

    // return result value(s) to tmplate
    IDE_TEST( qsxExtProc::returnAllParams( aQcStmt->qmxMem,
                                           sMsg,
                                           sTmplate ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execIntproc( qsxExecutorInfo * aExecInfo,
                                 qcStatement     * aQcStmt )
{
    qsCallSpecParam   * sParam      = NULL;
    idxIntProcMsg       sMsg;

    qsProcParseTree   * sParseTree;
    qcTemplate        * sTmplate;

    UInt                sIndex = 0;

    intProcUtilFuncArg sArg;

    // Preparation
    sParseTree = aExecInfo->mProcPlanTree;
    sTmplate   = QC_PRIVATE_TMPLATE(aQcStmt);

    // Message initialization
    sMsg.mParamCount = 0;

    // LibName, FuncName
    sMsg.mLibName = sParseTree->expCallSpec->fileSpec;

    IDE_TEST_RAISE( sParseTree->expCallSpec->libraryNode == NULL,
                    ERR_LIB_NOT_FOUND );

    sMsg.mFuncName = sParseTree->expCallSpec->functionName;
    sMsg.mHandle = sParseTree->expCallSpec->libraryNode->mHandle;
    sMsg.mFuncPtr = sParseTree->expCallSpec->libraryNode->mFunctionPtr;

    // paramCount
    sMsg.mParamCount = sParseTree->expCallSpec->paramCount;

    IDE_TEST_RAISE( sMsg.mHandle == PDL_SHLIB_INVALID_HANDLE,
                    ERR_LIB_NOT_FOUND );

    IDE_TEST_RAISE( sParseTree->expCallSpec->functionPtr == NULL,
                    ERR_ENTRY_FUNCTION_NOT_FOUND );

    IDE_TEST_RAISE( sMsg.mFuncPtr == NULL,
                    ERR_FUNCTION_NOT_FOUND );

    // paramInfo
    if( sMsg.mParamCount > 0 )
    {
        IDE_TEST( aQcStmt->qmxMem->alloc( ( ID_SIZEOF(idxParamInfo) * sMsg.mParamCount ),
                                          (void **)&sMsg.mParamInfos )
                  != IDE_SUCCESS );

        sParam = sParseTree->expCallSpec->param;

        while( sParam != NULL )
        {
            IDE_TEST( qsxExtProc::fillParamInfo( aQcStmt->qmxMem,
                                                 sParam,
                                                 sTmplate,
                                                 &sMsg.mParamInfos[sIndex],
                                                 sIndex,
                                                 sParseTree->expCallSpec->procType )
                      != IDE_SUCCESS );

            sParam = sParam->next;
            sIndex++;
        }
    }

    // returnInfo
    if( sParseTree->returnTypeVar != NULL )
    {
        // sParseTree->returnTypeVar->common 에 table/column
        IDE_TEST( qsxExtProc::fillReturnInfo( aQcStmt->qmxMem,
                                              sParseTree->returnTypeVar->common,
                                              sTmplate,
                                              &sMsg.mReturnInfo,
                                              sParseTree->expCallSpec->procType )
                  != IDE_SUCCESS );
    }
    else
    {
        /* intialize return info */
        QSX_EXTPROC_INIT_PARAM_INFO( &sMsg.mReturnInfo );

        /* set data area to zero */
        idlOS::memset( &sMsg.mReturnInfo.mD, 0, ID_SIZEOF(sMsg.mReturnInfo.mD) );
    }

    sArg.mOption     = ID_UINT_MAX;
    sArg.mResult     = 0;
    sArg.mMemory     = aQcStmt->qixMem;
    sArg.mFunction   = (void*)qsxExecutor::utilFunc4IntProc;

    // call external procedure/function
    IDE_TEST( qsxLibrary::callintProc( aQcStmt->qmxMem,
                                       &sMsg,
                                       &sArg ) != IDE_SUCCESS );

    // return result value(s) to tmplate
    IDE_TEST( qsxExtProc::returnAllParams4IntProc( aQcStmt->qmxMem,
                                                   &sMsg,
                                                   sTmplate ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LIB_NOT_FOUND )
    {
        ideSetErrorCode( idERR_ABORT_IDX_LIBRARY_NOT_FOUND,
                         idxLocalSock::mHomePath,
                         IDX_LIB_DEFAULT_DIR,
                         sParseTree->expCallSpec->libraryNode->mLibPath );
    }
    IDE_EXCEPTION( ERR_ENTRY_FUNCTION_NOT_FOUND )
    {
        ideSetErrorCode( idERR_ABORT_IDX_ENTRY_FUNCTION_NOT_FOUND );
    }
    IDE_EXCEPTION( ERR_FUNCTION_NOT_FOUND )
    {
        ideSetErrorCode( idERR_ABORT_IDX_FUNCTION_NOT_FOUND );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::utilFunc4IntProc( intProcUtilFuncArg * aArg )
{
    IDE_TEST( aArg == NULL );

    switch (aArg->mOption)
    {
    case 1: // malloc
        {
            IDE_TEST( ((iduVarMemList*)aArg->mMemory)->alloc(*((int*)(aArg->mArgument1)),
                                                             (void**)(aArg->mArgument3))
                      != IDE_SUCCESS );
        }
        break;
    case 2: // free
        {
            IDE_TEST( ((iduVarMemList*)aArg->mMemory)->free(aArg->mArgument1)
                      != IDE_SUCCESS );
        }
        break;
    default: // invalid option
        {
            IDE_TEST(1);
        }
    }

    aArg->mResult = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aArg->mResult = -1;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execPkgPlan( qsxExecutorInfo * aExecInfo,
                                 qcStatement     * aQcStmt,
                                 mtcStack        * aStack,
                                 SInt              aStackRemain )
{
    qsPkgParseTree    * sParseTree;
    qsxPkgInfo        * sPkgInfo;
    iduMemoryStatus     sQmxMemStatus;
    qcTemplate        * sSourceTemplate = NULL;
    SChar             * sOriStmtText    = NULL;
    SInt                sOriStmtTextLen = 0;
    UInt                sTopExec     = 0;
    qsxPkgInfo        * sPkgSpecInfo = NULL;
    UInt                sStage          = 0;
    idBool              sOrgPSMFlag  = ID_FALSE;
    SChar             * sOrgInvokeUserName = QCG_GET_SESSION_INVOKE_USER_NAME(aQcStmt);
    UInt                sUserID            = QCG_GET_SESSION_USER_ID( aQcStmt );      /* BUG-45306 PSM AUTHID */
    UInt                sVarStage    = 0;
    qsxStmtList       * sOrgStmtList  = NULL;
    qsxStmtList2      * sOrgStmtList2 = NULL;

    sParseTree  = aExecInfo->mPkgPlanTree;
    sPkgInfo    = sParseTree->pkgInfo;
    // BUG-41030 Backup called by PSM flag
    sOrgPSMFlag = aQcStmt->calledByPSMFlag;

    // BUG-43158 Enhance statement list caching in PSM
    sOrgStmtList  = aQcStmt->spvEnv->mStmtList;
    sOrgStmtList2 = aQcStmt->spvEnv->mStmtList2;

    aQcStmt->spvEnv->mStmtList = NULL;
    aQcStmt->spvEnv->mStmtList2 = NULL;

    // BUG-17489
    // 최상위 PSM호출인 경우 planTreeFlag를 FALSE로 바꾼다.
    qcg::lock( aQcStmt );
    if ( aQcStmt->planTreeFlag == ID_TRUE )
    {
        aQcStmt->planTreeFlag = ID_FALSE;
        sTopExec = 1;
    }
    qcg::unlock( aQcStmt );

    IDE_TEST_RAISE( sParseTree == NULL, err_stmt_is_null );

    QSX_ENV_PKG_PLAN_TREE( QC_QSX_ENV(aQcStmt) ) = sParseTree;

    IDE_TEST( QC_QMX_MEM(aQcStmt)-> getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage = 1;

    /* BUG-45306 PSM AUTHID */
    if ( aExecInfo->mIsDefiner == ID_TRUE )
    {
        QCG_SET_SESSION_USER_ID( aQcStmt, aExecInfo->mDefinerUserID );
        // BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
        QCG_SET_SESSION_INVOKE_USER_NAME( aQcStmt, sParseTree->objectNameInfo.userName );

        if ( SDU_SHARD_ENABLE == 1 )
        {
            IDE_TEST( qci::mSessionCallback.mSetInvokeUserPropertyInternal(
                        aQcStmt->session->mMmSession,
                        (SChar*)"INVOKE_USER",
                        11,
                        sParseTree->objectNameInfo.userName,
                        idlOS::strlen(sParseTree->objectNameInfo.userName) )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        QCG_SET_SESSION_USER_ID( aQcStmt, aExecInfo->mCurrentUserID );
    }

    sSourceTemplate = QC_PRIVATE_TMPLATE(aQcStmt);
    sStage = 2;

    /* 각 세션에서 package를 처음 실행 시 타는 함수이다.
       따라서, package의 template은 무조건 존재한다. */
    QC_PRIVATE_TMPLATE(aQcStmt) = aExecInfo->mPkgTemplate;

    // set stack buffer PR2475
    /* QC_PRIVATE_TMPLATE(aQcStmt)->
             template.stackBuffer/stack/stackCount/stackRemain setting */
    QC_CONNECT_TEMPLATE_STACK(
        QC_PRIVATE_TMPLATE(aQcStmt),
        aStack,
        aStack,
        aStackRemain,
        aStackRemain );
    QC_PRIVATE_TMPLATE(aQcStmt)->stmt = aQcStmt;
    // BUG-11192 date format session property 추가
    // clone된 template의 dateFormat은 다른 세션의 값이므로
    // 새로이 assign해주어야 한다.
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.dateFormat  = sSourceTemplate->tmplate.dateFormat;
    /* PROJ-2208 Multi Currency */
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.nlsCurrency = sSourceTemplate->tmplate.nlsCurrency;
    // PROJ-1579 NCHAR
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.nlsUse      = sSourceTemplate->tmplate.nlsUse;
    sStage = 3;

    sOriStmtText                 = aQcStmt->myPlan->stmtText;
    sOriStmtTextLen              = aQcStmt->myPlan->stmtTextLen;
    aQcStmt->myPlan->stmtText    = sParseTree->stmtText;
    aQcStmt->myPlan->stmtTextLen = sParseTree->stmtTextLen;
    sStage = 4;

    IDE_TEST( qsxEnv::increaseCallDepth( QC_QSX_ENV(aQcStmt) )
              != IDE_SUCCESS );
    sStage = 5;

    IDE_TEST( qcuSessionPkg::initPkgVariable( aExecInfo,
                                              aQcStmt,
                                              sPkgInfo,
                                              QC_PRIVATE_TMPLATE(aQcStmt) )
              != IDE_SUCCESS );
    sVarStage = 1;

    if( sPkgInfo->objType == QS_PKG_BODY )
    {
        IDE_TEST( qcuSessionPkg::getPkgInfo( aQcStmt,
                                             sParseTree->userID,
                                             sParseTree->pkgNamePos,
                                             QS_PKG,
                                             &sPkgSpecInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initializeSqlCursor( aExecInfo,
                                       aQcStmt )
                  != IDE_SUCCESS );

        IDE_TEST( qcuSessionPkg::initPkgCursors( aQcStmt,
                                                 sPkgSpecInfo->pkgOID )
                  != IDE_SUCCESS );

        IDE_TEST( qcuSessionPkg::initPkgCursors( aQcStmt,
                                                 sPkgInfo->pkgOID )
                  != IDE_SUCCESS );
        sStage = 6;

        // BUG-41030 Set called by PSM flag
        aQcStmt->calledByPSMFlag = ID_TRUE;

        IDE_TEST( execPkgBlock ( aExecInfo,
                                 aQcStmt,
                                 (qsProcStmts* ) sParseTree->block )
                  != IDE_SUCCESS );

        sStage = 5;
        IDE_TEST( qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                 sPkgInfo->pkgOID )
                  != IDE_SUCCESS );

        IDE_TEST( qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                 sPkgSpecInfo->pkgOID )
                  != IDE_SUCCESS );

        // BUGBUG 2505
        // transfer SQL cursor to calle
        if (aExecInfo->mFlow == QSX_FLOW_RAISE)
        {
            if ( aExecInfo->mFlowIdSetBySystem )
            {
                switch(aExecInfo->mFlowId)
                {
                    case QSX_CURSOR_ALREADY_OPEN_NO :
                    case QSX_DUP_VAL_ON_INDEX_NO :
                    case QSX_INVALID_CURSOR_NO :
                    case QSX_INVALID_NUMBER_NO :
                    case QSX_NO_DATA_FOUND_NO :
                    case QSX_PROGRAM_ERROR_NO :
                    case QSX_STORAGE_ERROR_NO :
                    case QSX_TIMEOUT_ON_RESOURCE_NO :
                    case QSX_TOO_MANY_ROWS_NO :
                    case QSX_VALUE_ERROR_NO :
                    case QSX_ZERO_DIVIDE_NO :

                        // PROJ-1371 File handling exception.
                    case QSX_INVALID_PATH_NO:
                    case QSX_INVALID_MODE_NO:
                    case QSX_INVALID_FILEHANDLE_NO:
                    case QSX_INVALID_OPERATION_NO:
                    case QSX_READ_ERROR_NO:
                    case QSX_WRITE_ERROR_NO:
                    case QSX_ACCESS_DENIED_NO:
                    case QSX_DELETE_FAILED_NO:
                    case QSX_RENAME_FAILED_NO:

                        // TASK-7218 Handling Shard Multiple Errors
                    case QSX_SHARD_MULTIPLE_ERRORS_NO:
                    case QSX_OTHER_SYSTEM_ERROR_NO :
                        unsetFlowControl(aExecInfo, ID_FALSE);
                        // error code and message is already set.
                        IDE_RAISE( err_pass );
                        /* fall through */
                    default :
                        break;
                }
            }

            IDE_RAISE(err_unhandled_exception);
        }
    }
    else
    {
        // Nothing to do.
    }

    sStage = 4;
    IDE_TEST( qsxEnv::decreaseCallDepth( QC_QSX_ENV(aQcStmt) )
              != IDE_SUCCESS );

    sStage = 3;
    aQcStmt->myPlan->stmtText    = sOriStmtText;
    aQcStmt->myPlan->stmtTextLen = sOriStmtTextLen;

    sStage = 2;
    QC_DISCONNECT_TEMPLATE_STACK( QC_PRIVATE_TMPLATE(aQcStmt) );
    QC_PRIVATE_TMPLATE(aQcStmt)->stmt                = NULL;
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.dateFormat  = 
        sPkgInfo->tmplate->tmplate.dateFormat;
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.nlsCurrency =
        sPkgInfo->tmplate->tmplate.nlsCurrency;
    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.nlsUse      =
        sPkgInfo->tmplate->tmplate.nlsUse;

    sStage = 1;
    QC_PRIVATE_TMPLATE(aQcStmt) = sSourceTemplate;

    /* BUG-45306 PSM AUTHID */
    QCG_SET_SESSION_USER_ID( aQcStmt, sUserID );
    // BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
    QCG_SET_SESSION_INVOKE_USER_NAME( aQcStmt, sOrgInvokeUserName );

    if ( SDU_SHARD_ENABLE == 1 )
    {
        IDE_TEST( qci::mSessionCallback.mSetInvokeUserPropertyInternal(
                    aQcStmt->session->mMmSession,
                    (SChar*)"INVOKE_USER",
                    11,
                    sOrgInvokeUserName,
                    idlOS::strlen(sOrgInvokeUserName) )
                  != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    if ( sTopExec == 1 )
    {
        qcg::setPlanTreeState( aQcStmt, ID_TRUE );
    }

    // BUG-41030 Restore called by PSM flag
    aQcStmt->calledByPSMFlag = sOrgPSMFlag;

    // BUG-43158 Enhance statement list caching in PSM
    aQcStmt->spvEnv->mStmtList  = sOrgStmtList;
    aQcStmt->spvEnv->mStmtList2 = sOrgStmtList2;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_unhandled_exception)
    {
        if ( getRaisedExceptionName( aExecInfo,
                                     aQcStmt )
             != IDE_SUCCESS)
        {
            idlOS::snprintf( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg,
                             ID_SIZEOF(QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg),
                             "%s",
                             "UNKNOWN SYMBOL");
        }
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_UNHANDLED_EXCEPTION,
                                 QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg) );
    }
    IDE_EXCEPTION(err_stmt_is_null);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                "plan = null (exec)"));
    }
    IDE_EXCEPTION(err_pass);
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 6:
            if ( sPkgInfo->objType == QS_PKG_BODY )
            {
                if( qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                   sPkgInfo->pkgOID )
                    != IDE_SUCCESS )
                {
                    IDE_ERRLOG(IDE_QP_1);
                }

                if( qcuSessionPkg::finiPkgCursors( aQcStmt,
                                                   sPkgSpecInfo->pkgOID )
                    != IDE_SUCCESS )
                {
                    IDE_ERRLOG(IDE_QP_1);
                }
            }
            else
            {
                // Nothing to do.
            }
            /* fall through */
        case 5:
            if ( qsxEnv::decreaseCallDepth( QC_QSX_ENV(aQcStmt) )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 4:
            aQcStmt->myPlan->stmtText    = sOriStmtText;
            aQcStmt->myPlan->stmtTextLen = sOriStmtTextLen;
            /* fall through */
        case 3:
            QC_DISCONNECT_TEMPLATE_STACK( QC_PRIVATE_TMPLATE(aQcStmt) );
            QC_PRIVATE_TMPLATE(aQcStmt)->stmt                = NULL;
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.dateFormat  =
                sPkgInfo->tmplate->tmplate.dateFormat;
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.nlsCurrency =
                sPkgInfo->tmplate->tmplate.nlsCurrency;
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.nlsUse      =
                sPkgInfo->tmplate->tmplate.nlsUse;
            /* fall through */
        case 2:
            QC_PRIVATE_TMPLATE(aQcStmt) = sSourceTemplate;
            /* fall through */
        case 1:
            if ( QC_QMX_MEM(aQcStmt)-> setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    if ( sVarStage == 1 )
    { 
        if ( qcuSessionPkg::finiPkgVariable(aQcStmt,
                                            sPkgInfo,
                                            aExecInfo->mPkgTemplate )
             != IDE_SUCCESS )
        {
            IDE_ERRLOG(IDE_QP_1);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( sTopExec == 1 )
    {
        qcg::setPlanTreeState( aQcStmt, ID_TRUE );
    }

    // BUG-41030 Restore called by PSM flag
    aQcStmt->calledByPSMFlag = sOrgPSMFlag;

    /* BUG-45306 PSM AUTHID */
    QCG_SET_SESSION_USER_ID( aQcStmt, sUserID );
    // BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
    QCG_SET_SESSION_INVOKE_USER_NAME( aQcStmt, sOrgInvokeUserName );

    if ( SDU_SHARD_ENABLE == 1 )
    {
        (void)qci::mSessionCallback.mSetInvokeUserPropertyInternal(
                aQcStmt->session->mMmSession,
                (SChar*)"INVOKE_USER",
                11,
                sOrgInvokeUserName,
                idlOS::strlen(sOrgInvokeUserName) );
    }

    // BUG-43158 Enhance statement list caching in PSM
    aQcStmt->spvEnv->mStmtList  = sOrgStmtList;
    aQcStmt->spvEnv->mStmtList2 = sOrgStmtList2;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::execPkgBlock( qsxExecutorInfo * aExecInfo,
                                  qcStatement     * aQcStmt,
                                  qsProcStmts     * aPkgBlock )
{
    iduMemoryStatus       sQmxMemStatus;
    SInt                  sStage = 0;
    qsPkgStmtBlock      * sPkgBlock;
    UInt                  sOriSqlCode;
    SChar               * sOriSqlErrorMsg;
    qsProcStmtException * sExcpBlock;

    sPkgBlock = ( qsPkgStmtBlock * ) aPkgBlock;

    IDE_TEST( QC_QMX_MEM(aQcStmt)-> getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage=1;

    // PROJ-1335 RAISE 지원
    // To fix BUG-12642
    // exception handling이 이루어지면 블록 시작 이전의
    // 에러코드로 돌아가야 함
    IDE_TEST( QC_QMX_MEM(aQcStmt)->alloc(
            MAX_ERROR_MSG_LEN + 1,
            (void**) & sOriSqlErrorMsg )
        != IDE_SUCCESS);

    sOriSqlCode = QC_QSX_ENV(aQcStmt)->mSqlCode;

    if( sOriSqlCode != 0 )
    {
        idlOS::strncpy( sOriSqlErrorMsg,
                        QC_QSX_ENV(aQcStmt)->mSqlErrorMessage,
                        MAX_ERROR_MSG_LEN );
        sOriSqlErrorMsg[ MAX_ERROR_MSG_LEN ] = '\0';
    }

    IDE_TEST( execStmtList ( aExecInfo,
                             aQcStmt,
                             sPkgBlock->bodyStmts )
              != IDE_SUCCESS );

    if( sPkgBlock->exception != NULL )
    {
        sExcpBlock = (qsProcStmtException *)(sPkgBlock->exception);

        IDE_TEST( catchException ( aExecInfo,
                                   aQcStmt,
                                   sExcpBlock->exceptionHandlers)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1335 RAISE 지원
    // To fix BUG-12642
    // re-raise되지 않았다면 이전 에러코드로 복귀.
    if( aExecInfo->mFlow != QSX_FLOW_RAISE )
    {
        QC_QSX_ENV(aQcStmt)->mSqlCode = sOriSqlCode;

        if( sOriSqlCode != 0 )
        {
            idlOS::strncpy( QC_QSX_ENV(aQcStmt)->mSqlErrorMessage,
                            sOriSqlErrorMsg,
                            MAX_ERROR_MSG_LEN );
            QC_QSX_ENV(aQcStmt)->mSqlErrorMessage[ MAX_ERROR_MSG_LEN ] = '\0';
        }
    }
    else
    {
        // Nothing to do.
    }

    sStage=0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)-> setStatus( &sQmxMemStatus )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            if ( QC_QMX_MEM(aQcStmt)-> setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);

            }
    }

    sStage = 0;

    return IDE_FAILURE;
}

IDE_RC qsxExecutor::bindParam( qcStatement   * aQcStmt,
                               QCD_HSTMT       aHstmt,
                               qsUsingParam  * aUsingParam,
                               qciBindData  ** aOutBindParamDataList,
                               idBool          aIsFirst )
{
    UShort         sBindParamId = 0;
    qsUsingParam * sUsingParam;
    void         * sValue;
    UInt           sValueSize;
    mtcColumn    * sMtcColumn;
    idBool         sExist = ID_FALSE;

    /* BUG-36907
     * 처음 실행하는 경우에만 bindParmaInfoSet 함수를 호출한다. */
    if( aIsFirst == ID_TRUE )
    {
        // param info bind
        for ( sUsingParam = aUsingParam;
              sUsingParam != NULL;
              sUsingParam = sUsingParam->next )
        {
            sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                          sUsingParam->paramNode );

            IDE_TEST( qcd::bindParamInfoSet( aHstmt,
                                             sMtcColumn,
                                             sBindParamId,
                                             sUsingParam->inOutType )
                      != IDE_SUCCESS );

            sBindParamId++;
        }
    }
    else
    {
        if ( QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE > 0 )
        {
            /* BUG-47959 procedure 에서 잘못된 메모리 침범으로인한  FATAL 발생*/
            for ( sUsingParam = aUsingParam;
                  sUsingParam != NULL;
                  sUsingParam = sUsingParam->next )
            {
                sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                              sUsingParam->paramNode );

                if ( ( sMtcColumn->flag & MTC_COLUMN_SP_SET_PRECISION_MASK )
                     == MTC_COLUMN_SP_SET_PRECISION_TRUE )
                {
                    sExist = ID_TRUE;
                    break;
                }
            }
           
            /* precision이 없는 bind param이 존재할경우 bindParamInfo를
             * 다시 설정해준다
             */
            if ( sExist == ID_TRUE )
            {
                for ( sUsingParam = aUsingParam;
                      sUsingParam != NULL;
                      sUsingParam = sUsingParam->next )
                {
                    sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                                  sUsingParam->paramNode );

                    IDE_TEST( qcd::bindParamInfoSet( aHstmt,
                                                     sMtcColumn,
                                                     sBindParamId,
                                                     sUsingParam->inOutType )
                              != IDE_SUCCESS );

                    sBindParamId++;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    sBindParamId = 0;

    // param data bind
    for ( sUsingParam = aUsingParam;
          sUsingParam != NULL;
          sUsingParam = sUsingParam->next )
    {
        IDE_TEST( qtc::calculate( sUsingParam->paramNode,
                                  QC_PRIVATE_TMPLATE(aQcStmt) )
                  != IDE_SUCCESS );

        sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                      sUsingParam->paramNode );

        sValue = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;

        sValueSize = sMtcColumn->module->actualSize( sMtcColumn,
                                                     sValue );

        IDE_TEST( qcd::bindParamData( aHstmt,
                                      sValue,
                                      sValueSize,
                                      sBindParamId,
                                      aQcStmt->qmxMem,
                                      aOutBindParamDataList,
                                      sUsingParam->inOutType )
                  != IDE_SUCCESS );

        sBindParamId++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qsxExecutor::adjustErrorMsg( qsxExecutorInfo  * aExecInfo,
                                  qcStatement      * aQcStmt,
                                  qsProcStmts      * aProcStmts,
                                  qcuSqlSourceInfo * aSqlInfo,
                                  idBool             aIsSetPosition )
{
    SChar * sErrorMsg = NULL;

    IDE_DASSERT( aExecInfo  != NULL );
    IDE_DASSERT( aQcStmt    != NULL );
    IDE_DASSERT( aProcStmts != NULL );
    IDE_DASSERT( aSqlInfo   != NULL );
    IDE_DASSERT( ( aIsSetPosition == ID_TRUE ) ||
                 ( aIsSetPosition == ID_FALSE ) );

    // set original error code.
    qsxEnv::setErrorCode( QC_QSX_ENV(aQcStmt) );

    // To fix BUG-13208
    // system_유저가 만든 프로시져는 내부공개 안함.
    if( ( aExecInfo->mDefinerUserID == QC_SYSTEM_USER_ID ) ||
        ( QCU_PSM_SHOW_ERROR_STACK == 2 ) )
    {
        // Nothing to do.
    }
    else
    {
        if( aIsSetPosition == ID_TRUE )
        {
            aSqlInfo->setSourceInfo( aQcStmt,
                                     &aProcStmts->pos );
        }
        else
        {
            // Nothing to do.
        }

        (void)aSqlInfo->initWithBeforeMessage( QC_QMX_MEM(aQcStmt) );

        // fix BUG-36522
        if( QCU_PSM_SHOW_ERROR_STACK == 1 )
        {
            sErrorMsg = aSqlInfo->getErrMessage();
        }
        else // QCU_PSM_SHOW_ERROR_STACK = 0
        {
            sErrorMsg = aExecInfo->mSqlErrorMessage;

            idlOS::snprintf( sErrorMsg,
                             MAX_ERROR_MSG_LEN + 1,
                             "\nat \"%s\", line %"ID_INT32_FMT"",
                             aExecInfo->mUserAndObjectName,
                             aProcStmts->lineno );
        }

        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                            aSqlInfo->getBeforeErrMessage(),
                            sErrorMsg ));

        (void)aSqlInfo->fini();
    }

    // set sophisticated error message.
    qsxEnv::setErrorMessage( QC_QSX_ENV(aQcStmt) );
}

IDE_RC qsxExecutor::processIntoClause( qsxExecutorInfo * aExecInfo,
                                       qcStatement     * aQcStmt,
                                       QCD_HSTMT         aHstmt,
                                       qmsInto         * aIntoVariables,
                                       idBool            aIsIntoVarRecordType,
                                       idBool            aRecordExist )
{
/***********************************************************************
 *
 * Description : 
 *    BUG-37273
 *    select into 구문 또는 execute immediate에서 사용되는 into 구문을
 *    처리하기 위한 함수.
 *
 * Implementation :
 *
 ************************************************************************/
    qtcNode     * sIntoNode;
    UInt          sRowCount = 0;
    qtcNode     * sIndexNode;
    mtcColumn   * sIndexColumn;
    void        * sIndexValue;
    qcmColumn   * sQcmColumn;
    void        * sColumnValue;
    mtcColumn   * sMtcColumn;
    void        * sValue;
    UShort        sBindColumnId;
    qciBindData * sBindColumnDataList;
    idBool        sNextRecordExist = ID_FALSE;
    qcNamePosition     sPos;
    qcuSqlSourceInfo   sqlInfo;

    sPos.stmtText = aIntoVariables->intoPosition.stmtText;
    sPos.offset   = aIntoVariables->intoNodes->position.offset;
    sPos.size     = 0;

    if( aIntoVariables->bulkCollect == ID_TRUE )
    {
        QSX_CURSOR_SET_ROWCOUNT( aExecInfo->mSqlCursorInfo, 0);
        IDE_TEST_CONT( aRecordExist == ID_FALSE, ERR_PASS );
    }
    else
    {
        IDE_TEST_RAISE( aRecordExist == ID_FALSE, ERR_NO_DATA_FOUND );
    }

    while( aRecordExist == ID_TRUE )
    {
        for( sIntoNode = aIntoVariables->intoNodes;
             sIntoNode != NULL;
             sIntoNode = (qtcNode*)sIntoNode->node.next )
        {
            /* BUG-37273
               첫번째 노드에서만 size 계산,
               그 다음부터는 동일하기 때문에 size를 계산할 필요가 없다.*/
            if( sRowCount == 0 )
            {
                sPos.size = sIntoNode->position.offset + 
                            sIntoNode->position.size -
                            sPos.offset;
            }
            else
            {
                // Nothing to do.
            }

            if( aIntoVariables->bulkCollect == ID_TRUE )
            {
                // index overflow
                IDE_TEST_RAISE( sRowCount >= (vSLong)MTD_INTEGER_MAXIMUM,
                                ERR_TOO_MANY_ROWS );

                sIndexNode   = (qtcNode*)sIntoNode->node.arguments;
                sIndexColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                                sIndexNode );
                sIndexValue  = QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aQcStmt),
                                                   sIndexNode );

                IDE_ASSERT( sIndexColumn->module->id == MTD_INTEGER_ID );
                *(mtdIntegerType*)sIndexValue =
                    (mtdIntegerType)(sRowCount + 1);
            }
            else
            {
                // Nothing to do.
            }
        }

        sBindColumnId = 0;
        sBindColumnDataList = NULL;

        // bindColumnDataList생성
        if( aIsIntoVarRecordType == ID_TRUE )
        {
            IDE_DASSERT( aIntoVariables->intoNodes != NULL );

            sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                          aIntoVariables->intoNodes );

            IDE_TEST( qtc::calculate( aIntoVariables->intoNodes,
                                      QC_PRIVATE_TMPLATE(aQcStmt) )
                      != IDE_SUCCESS );

            sValue = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;

            for( sQcmColumn = ((qtcModule*)(sMtcColumn->module))->typeInfo->columns;
                 sQcmColumn != NULL;
                 sQcmColumn = sQcmColumn->next )
            {
                sColumnValue = (void*)mtc::value( sQcmColumn->basicInfo,
                                                  sValue,
                                                  MTD_OFFSET_USE );

                IDE_TEST( qcd::addBindColumnDataList( aQcStmt->qmxMem,
                                                      &sBindColumnDataList,
                                                      sQcmColumn->basicInfo,
                                                      sColumnValue,
                                                      sBindColumnId )
                          != IDE_SUCCESS );

                sBindColumnId++;
            }
        }
        else
        {
            for( sIntoNode = aIntoVariables->intoNodes;
                 sIntoNode != NULL;
                 sIntoNode = (qtcNode*)sIntoNode->node.next )
            {
                sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                              sIntoNode );

                IDE_TEST( qtc::calculate( sIntoNode,
                                          QC_PRIVATE_TMPLATE(aQcStmt) )
                          != IDE_SUCCESS );

                sValue = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;

                IDE_TEST( qcd::addBindColumnDataList( aQcStmt->qmxMem,
                                                      &sBindColumnDataList,
                                                      sMtcColumn,
                                                      sValue,
                                                      sBindColumnId )
                          != IDE_SUCCESS );

                sBindColumnId++;
            }
        }

        if( sRowCount == 0 )
        {
            IDE_TEST_RAISE( qcd::checkBindColumnCount( aHstmt,
                                                       sBindColumnId )
                            != IDE_SUCCESS, ERR_MISMATCH_INTO_COUNT );
        }
        else
        {
            // Nothing to do.
        }

        // fetch
        IDE_TEST( qcd::fetch( aQcStmt,
                              QC_QMX_MEM(aQcStmt),
                              aHstmt,
                              sBindColumnDataList,
                              &sNextRecordExist )
                  != IDE_SUCCESS );

        sRowCount++;

        QSX_CURSOR_SET_ROWCOUNT( aExecInfo->mSqlCursorInfo, sRowCount );

        if( aIntoVariables->bulkCollect == ID_FALSE )
        {
            IDE_TEST_RAISE( sNextRecordExist == ID_TRUE,
                            ERR_TOO_MANY_ROWS );

            break;
        }
        else
        {
            if( sNextRecordExist == ID_FALSE )
            {
                break;
            }
            else
            {
                /* continue to fetch rows */
                IDE_TEST_RAISE( sRowCount >= (vSLong)MTD_INTEGER_MAXIMUM,
                                ERR_TOO_MANY_ROWS );
            }
        }
    }

    IDE_EXCEPTION_CONT(ERR_PASS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_DATA_FOUND);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_NO_DATA_FOUND));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_ROWS);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_TOO_MANY_ROWS));
    }
    IDE_EXCEPTION( ERR_MISMATCH_INTO_COUNT );
    {
        sqlInfo.setSourceInfo( aQcStmt,
                               &sPos );

        (void)sqlInfo.init(aQcStmt->qmxMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_MISMATCHED_INTO_LIST_SQLTEXT,
                                sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qsxExecutor::setRaisedExcpErrorMsg( qsxExecutorInfo  * aExecInfo,
                                         qcStatement      * aQcStmt,
                                         qsProcStmts      * aProcStmts,
                                         qcuSqlSourceInfo * aSqlInfo,
                                         idBool             aIsSetPosition )
{
    SChar * sErrorMsg    = NULL;
    UInt    sErrorMsgLen = 0;

    if( aIsSetPosition == ID_TRUE )
    {
        aSqlInfo->setSourceInfo( aQcStmt,
                                 &aProcStmts->pos );
    }
    else
    {
        // Nothing to do.
    }

    (void)aSqlInfo->initWithBeforeMessage( QC_QMX_MEM(aQcStmt) );

    if ( QCU_PSM_SHOW_ERROR_STACK == 1 )
    {
        sErrorMsg = aSqlInfo->getErrMessage();
    }
    else // QCU_PSM_SHOW_ERROR_STACK = 0
    {
        sErrorMsg = aExecInfo->mSqlErrorMessage;

        idlOS::snprintf( sErrorMsg,
                         MAX_ERROR_MSG_LEN + 1,
                         "\nat \"%s\", line %"ID_INT32_FMT"",
                         aExecInfo->mUserAndObjectName,
                         aProcStmts->lineno );
    }

    sErrorMsgLen = idlOS::strlen( sErrorMsg );

    if ( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsgLen + sErrorMsgLen +
         idlOS::strlen(QC_QSX_ENV(aQcStmt)->mSqlErrorMessage) <= MAX_ERROR_MSG_LEN )
    {
        idlOS::snprintf( QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg +
                         QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsgLen,
                         ID_SIZEOF(QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsg) -
                         QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsgLen,
                         "%s",
                         sErrorMsg );

        QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsgLen = 
            QC_QSX_ENV(aQcStmt)->mRaisedExcpInfo.mRaisedExcpErrorMsgLen + sErrorMsgLen;
    }
    else
    {
        // Nothing to do.
    }

    (void)aSqlInfo->fini();
} 

// TASK-7244 Global PSM - commit/rollback/savepoint/DDL 정책
IDE_RC qsxExecutor::checkRestrictionsForShard( qcStatement * aQcStmt )
{
    UShort sClientTouchNodeCount;

    if ( (QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( aQcStmt ) == ID_FALSE) &&
         (QCG_GET_SESSION_GTX_LEVEL( aQcStmt ) != 0) &&
         (SDU_SHARD_ENABLE == 1) )
    {
        switch ( QCG_GET_SESSION_SHARD_SESSION_TYPE( aQcStmt ) )
        {
            case SDI_SESSION_TYPE_USER:
                sClientTouchNodeCount = QCG_GET_SESSION_SHARD_CLIENT_TOUCH_NODE_COUNT( aQcStmt );

                // Global procedure에서는 touch한 node가 있고, multi node transaction이면 불가능
                IDE_TEST_RAISE( (sClientTouchNodeCount > 0) &&
                                (QCG_GET_SESSION_GTX_LEVEL( aQcStmt ) == 1),
                                ERR_BY_USER_SESSION );
                break;
            case SDI_SESSION_TYPE_COORD:
            case SDI_SESSION_TYPE_LIB:
                // sharded procedure에서는 무조건 불가능 (clone, solo 포함)
                IDE_TEST_RAISE( (aQcStmt->spxEnv->mFlag & QSX_ENV_SHARD_FLAG) != QSX_ENV_FLAG_INIT,
                                ERR_BY_SHARD_PROCEDURE );
                break;
            default:
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BY_USER_SESSION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "DCL/DDL restrictions in Multiple Node Transaction.",
                                  "" ) );
    }
    IDE_EXCEPTION( ERR_BY_SHARD_PROCEDURE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "DCL/DDL in shard procedure.",
                                  "" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
