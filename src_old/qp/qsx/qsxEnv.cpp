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
 * $Id: qsxEnv.cpp 90088 2021-02-26 05:02:04Z khkwak $
 **********************************************************************/
/*
  NAME
  qsxEnv.cpp

  DESCRIPTION

  PUBLIC FUNCTION(S)

  PRIVATE FUNCTION(S)

  NOTES

  MODIFIED   (MM/DD/YY)
*/

#include <idl.h>
#include <ida.h>
#include <idu.h>
#include <smi.h>
#include <smErrorCode.h>
#include <qcuError.h>
#include <qsx.h>
#include <qsxEnv.h>
#include <qsvEnv.h>
#include <qsxDef.h>
#include <qsxCursor.h>
#include <qcuProperty.h>
#include <qsxArray.h>
#include <qcuSessionPkg.h>
#include <qsvPkgStmts.h>
#include <qsxUtil.h>
#include <sdi.h>

#if defined ( DEBUG )
#   define QSX_ENV_CHECK_INITIALIZED( env )             \
    IDE_ASSERT( (env) -> mIsInitialized == ID_TRUE );
#else
#   define QSX_ENV_CHECK_INITIALIZED( env )
#endif /* DEBUG */

void qsxEnv::initialize(qsxEnvInfo     * aEnv,
                        qcSession      * aSession  //
// BUGBUG QCI interface does not exist yet.
/*
  ,
  QCIDBC             * dbc*/ )
{
    // session정보를 얻기 위해서,
    // qciSessionCallback함수를 이용해야 하는데,
    // 이때, mmSession정보가 필요함.
    (aEnv)-> mSession        = aSession;
// BUGBUG QCI interface does not exist yet.
/*
  (aEnv)-> dbc_            = dbc;
*/
    (aEnv)-> mIsInitialized   = ID_TRUE;

    reset( aEnv );
}

void qsxEnv::resetForInvocation ( qsxEnvInfo   * aEnv )
{
    // ++ for every invocation, these variables should be cleared and restored
    (aEnv)-> mOthersClauseDepth = 0;
    (aEnv)-> mProcPlan          = NULL;
    (aEnv)-> mPkgPlan           = NULL;
    // -- for every invocation, these variables should be cleared and restored
}

// mm uses this function
void qsxEnv::reset ( qsxEnvInfo   * aEnv )
{
    resetForInvocation( aEnv );

    // To fix BUG-12642 SQLCODE는 프로시져 시작, 종료시에만
    // 초기화 되어야 한다.
    clearErrorVariables( aEnv );

    aEnv->mCursorsInUse            = NULL;
    aEnv->mCursorsInUseFence       = NULL;
    aEnv->mReturnVar               = NULL;
    aEnv->mCallDepth               = 0;
    // BUG-41279
    // Prevent parallel execution while executing 'select for update' clause.
    aEnv->mFlag                    = QSX_ENV_FLAG_INIT;

    // BUG-42322
    aEnv->mStackCursor             = -1;
    idlOS::memset( &aEnv->mPrevStackInfo,
                   '\0',
                   ID_SIZEOF(qsxStackFrame) );

    /* BUG-43154
       password_verify_function이 autonomous_transaction(AT) pragma가 선언된 function일 경우 비정상 종료 */
    aEnv->mExecPWVerifyFunc = ID_FALSE;

    /* BUG-43160 */
    idlOS::memset( &aEnv->mRaisedExcpInfo ,
                   '\0',
                   ID_SIZEOF(qsxRaisedExcpInfo) );
    (void)initializeRaisedExcpInfo( &aEnv->mRaisedExcpInfo ); 

    // BUG-44856
    SET_EMPTY_POSITION( aEnv->mSqlInfo );

    // BUG-46074 Multiple trigger event
    aEnv->mTriggerUptColList = NULL;
    aEnv->mTriggerEventType  = QCM_TRIGGER_EVENT_NONE;

    // TASK-7244 DBMS_SHARD_GET_DIAGNOSTICS package
    IDU_LIST_INIT(&aEnv->mErrorList);
    aEnv->mErrorListCount = 0;
}

void qsxEnv::backupReturnValue( qsxEnvInfo        * aEnv,
                                qsxReturnVarList ** aValue )
{
    *aValue = aEnv->mReturnVar;
    aEnv->mReturnVar = NULL;
}

void qsxEnv::restoreReturnValue( qsxEnvInfo       * aEnv,
                                 qsxReturnVarList * aValue )
{
    aEnv->mReturnVar = aValue;
}

IDE_RC qsxEnv::increaseCallDepth( qsxEnvInfo   * aEnv )
{
#define IDE_FN "IDE_RC qsxEnv::increaseCallDepth()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    QSX_ENV_CHECK_INITIALIZED( aEnv );

    IDE_TEST_RAISE( (aEnv)-> mCallDepth >= QSX_MAX_CALL_DEPTH,
                    err_too_higt_call_depth );

    ( (aEnv)-> mCallDepth ) ++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_too_higt_call_depth);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_TOO_HIGH_CALL_DEPTH_ARG1,
                                QSX_MAX_CALL_DEPTH));

        // BUG-44155
        qsxEnv::setErrorVariables( aEnv );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

IDE_RC qsxEnv::decreaseCallDepth( qsxEnvInfo   * aEnv )
{
#define IDE_FN "IDE_RC qsxEnv::decreaseCallDepth()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    QSX_ENV_CHECK_INITIALIZED( aEnv );

    ( (aEnv)-> mCallDepth ) --;

    return IDE_SUCCESS;

#undef IDE_FN
}



/*
  func0(func1(func2(v1)));
  과 같은 호출을 bind할 수 있어야 함.
  맨 첫번째 qsxEnv::invoke시점에서 bind를 끝을 봐야함.
  func0's arg0 :
  func1
  func1's arg0
  func2
  func2's arg0
  v1

  qrxExecutor가 invoke할때만 qrxExecutor::exec호출전에
  모든 argument(argument의 argument포함) 를 바인드.
  ( aIsBindCallSpecArguments  == ID_TRUE )

  그외의 경우 바인딩은 전혀 하지 않음.
  ( aIsBindCallSpecArguments  == ID_FALSE )
*/

// NOTE if you chage this function change qsxEnv::invokeWithNode also
IDE_RC qsxEnv::invoke (
    qsxEnvInfo         * aEnv,
    qcStatement        * aQcStmt,
    qsOID                aProcOID,
    qsOID                aPkgBodyOID,
    UInt                 aSubprogramID,
    qtcNode            * aCallSpecNode )
{
    iduMemoryStatus        sQmxMemStatus;
    UInt                   sStage = 0;
    qsxProcPlanList      * sFoundProcPlan;
    qsProcParseTree      * sOriProcPlan = NULL;
    qsPkgParseTree       * sOriPkgPlan  = NULL;
    SInt                   sOriOthersClauseDepth;
    qsxPkgInfo           * sPkgBodyInfo = NULL;
    qcTemplate           * sPkgTemplate = NULL;
    qsProcParseTree      * sPlanTree    = NULL;
    qsOID                  sPkgBodyOID  = QS_EMPTY_OID;
    qcuSqlSourceInfo       sqlInfo;

    sOriOthersClauseDepth = aEnv->mOthersClauseDepth;
    sOriProcPlan          = aEnv->mProcPlan;
    sOriPkgPlan           = aEnv->mPkgPlan;

    resetForInvocation( aEnv );

    QSX_ENV_CHECK_INITIALIZED( aEnv );

    // check QUERY_TIMEOUT, connection
    IDE_TEST( iduCheckSessionEvent( aQcStmt->mStatistics )
              != IDE_SUCCESS );

    IDE_TEST( QC_QMX_MEM(aQcStmt)-> getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );
    sStage = 1;

    /* aSubprogram이 QS_PSM_SUBPROGRAM_ID인 경우는 일반 procedure를 실행시킬 때이며( exec proc1 ),
       위의 경우가 아닐 경우는, package의 subprogram을 실행 시키는 경우이다( exec pkg1.proc1 ).*/
    if( aSubprogramID == QS_PSM_SUBPROGRAM_ID )
    {
        if( aProcOID == QS_EMPTY_OID )
        {
            // recursive call
            sPlanTree = sOriProcPlan;
        }
        else
        {
            IDE_TEST( qsxRelatedProc::findPlanTree(
                    aQcStmt,
                    aProcOID,
                    &sFoundProcPlan )
                != IDE_SUCCESS );

            sPlanTree = (qsProcParseTree *)( sFoundProcPlan->planTree );
        }
    }
    else
    {
        if ( aProcOID == QS_EMPTY_OID )
        {
            /* BUG-39481
               package의 initialize section 실행 시에는 qsProcParseTree가 없을 수도 있다. */
            if ( sOriProcPlan != NULL )
            {
                sPkgBodyOID = sOriProcPlan->pkgBodyOID;
            }
            else
            {
                sPkgBodyOID = sOriPkgPlan->pkgOID;
            }
        }
        else
        {
            if ( aPkgBodyOID != QS_EMPTY_OID )
            {
                sPkgBodyOID = aPkgBodyOID;
            }
            else
            {
                IDE_RAISE( ERR_NOT_EXIST_PKG_BODY_NAME );
            }
        }

        IDE_TEST( qsxPkg::getPkgInfo( sPkgBodyOID,
                                      & sPkgBodyInfo )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sPkgBodyInfo == NULL , ERR_NOT_EXIST_PKG_BODY_NAME );

        IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( aQcStmt,
                                                           sPkgBodyInfo,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                                                           QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                                                           &sPkgTemplate )
                  != IDE_SUCCESS );

        /* BUG-38844 package subprogram의 plan tree를 찾음 */
        IDE_TEST( qsxPkg::findSubprogramPlanTree(
                sPkgBodyInfo,
                aSubprogramID,
                &sPlanTree )
            != IDE_SUCCESS );

        sPlanTree->pkgBodyOID = sPkgBodyOID;
    }

    /* BUG-39481
       println( recursive fucntion ); 이면, 비정상 종료
       원 qsProcParseTree 및 qsPkgParseTree 정보를 가지고 있어야 
       argument의 calculate 시 사용할 수 있다. */
    QSX_ENV_PLAN_TREE( QC_QSX_ENV(aQcStmt) )     = sOriProcPlan;
    QSX_ENV_PKG_PLAN_TREE( QC_QSX_ENV(aQcStmt) ) = sOriPkgPlan;

    IDE_TEST( qsx::callProcWithNode ( aQcStmt,
                                      sPlanTree,
                                      aCallSpecNode,
                                      sPkgTemplate,
                                      NULL )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)-> setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    aEnv->mOthersClauseDepth   = sOriOthersClauseDepth;
    aEnv->mProcPlan            = sOriProcPlan;
    aEnv->mPkgPlan             = sOriPkgPlan;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_PKG_BODY_NAME);
    {
        sqlInfo.setSourceInfo( aQcStmt,
                               & aCallSpecNode->tableName );

        (void)sqlInfo.init(aQcStmt->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_PKG_BODY,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
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

    aEnv->mOthersClauseDepth    = sOriOthersClauseDepth;
    aEnv->mProcPlan             = sOriProcPlan;

    sStage = 0;

    return IDE_FAILURE;
}

// NOTE if you chage this function change qsxEnv::invoke also

// NTOE that iduMemoryStatus is already got in
// qtcSpFunctionCall.cpp::qtcCalculateStoredProcedure
IDE_RC qsxEnv::invokeWithStack (
    qsxEnvInfo             * aEnv,
    qcStatement            * aQcStmt,
    qsOID                    aProcOID,
    qsOID                    aPkgBodyOID,
    UInt                     aSubprogramID,
    qtcNode                * aCallSpecNode,
    mtcStack               * aStack,
    SInt                     aStackRemain )
{
    qsxProcPlanList      * sFoundProcPlan;
    qsProcParseTree      * sOriProcPlan = NULL;
    qsPkgParseTree       * sOriPkgPlan  = NULL;
    SInt                   sOriOthersClauseDepth;
    qsxPkgInfo           * sPkgBodyInfo = NULL;
    qcTemplate           * sPkgTemplate = NULL;
    qsProcParseTree      * sPlanTree    = NULL;
    qsOID                  sPkgBodyOID  = QS_EMPTY_OID;
    qcuSqlSourceInfo       sqlInfo;

    sOriOthersClauseDepth    = aEnv->mOthersClauseDepth;
    sOriProcPlan             = aEnv->mProcPlan;
    sOriPkgPlan              = aEnv->mPkgPlan;

    resetForInvocation( aEnv );

    QSX_ENV_CHECK_INITIALIZED( aEnv );

    // check QUERY_TIMEOUT, connection
    IDE_TEST( iduCheckSessionEvent( aQcStmt->mStatistics )
              != IDE_SUCCESS );

    /* aSubprogram이 QS_PSM_SUBPROGRAM_ID인 경우는 일반 procedure를 실행시킬 때이며( exec proc1 ),
       위의 경우가 아닐 경우는, package의 subprogram을 실행 시키는 경우이다( exec pkg1.proc1 ).
       qsxEnv::invoke와 비슷 */
    if( aSubprogramID ==  QS_PSM_SUBPROGRAM_ID )
    {
      if( aProcOID == QS_EMPTY_OID )
        {
            // recursive call
            sPlanTree = sOriProcPlan;
        }
        else
        {
            IDE_TEST( qsxRelatedProc::findPlanTree(
                    aQcStmt,
                    aProcOID,
                    &sFoundProcPlan )
                != IDE_SUCCESS );

            sPlanTree = (qsProcParseTree *)( sFoundProcPlan->planTree );
        }
    }
    else
    {
        if ( aProcOID == QS_EMPTY_OID )
        {
            /* BUG-39481
               package의 initialize section 실행 시에는 qsProcParseTree가 없을 수도 있다. */
            if ( sOriProcPlan != NULL )
            {
                sPkgBodyOID = sOriProcPlan->pkgBodyOID;
            }
            else
            {
                sPkgBodyOID = sOriPkgPlan->pkgOID;
            }
        }
        else
        {
            if ( aPkgBodyOID != QS_EMPTY_OID )
            {
                sPkgBodyOID = aPkgBodyOID;
            }
            else
            {
                IDE_RAISE( ERR_NOT_EXIST_PKG_BODY_NAME );
            }
        }

        IDE_TEST( qsxPkg::getPkgInfo( sPkgBodyOID,
                                      & sPkgBodyInfo )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sPkgBodyInfo == NULL , ERR_NOT_EXIST_PKG_BODY_NAME );

        IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( aQcStmt,
                                                           sPkgBodyInfo,
                                                           aStack,
                                                           aStackRemain,
                                                           &sPkgTemplate )
                  != IDE_SUCCESS );

        /* BUG-38844 package subprogram의 plan tree를 찾음 */
        IDE_TEST( qsxPkg::findSubprogramPlanTree(
                      sPkgBodyInfo,
                      aSubprogramID,
                      &sPlanTree )
                  != IDE_SUCCESS );

        sPlanTree->pkgBodyOID = sPkgBodyOID;
    }

    /* BUG-39481
       println( recursive fucntion ); 이면, 비정상 종료
       원 qsProcParseTree 및 qsPkgParseTree 정보를 가지고 있어야
       argument의 calculate 시 사용할 수 있다. */
    QSX_ENV_PLAN_TREE( QC_QSX_ENV(aQcStmt) )     = sOriProcPlan;
    QSX_ENV_PKG_PLAN_TREE( QC_QSX_ENV(aQcStmt) ) = sOriPkgPlan;

    IDE_TEST( qsx::callProcWithStack(
                aQcStmt,
                sPlanTree,
                aStack,
                aStackRemain,
                sPkgTemplate,
                NULL )
            != IDE_SUCCESS );

    aEnv->mOthersClauseDepth   = sOriOthersClauseDepth;
    aEnv->mProcPlan            = sOriProcPlan;
    aEnv->mPkgPlan             = sOriPkgPlan;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_PKG_BODY_NAME);
    {
        sqlInfo.setSourceInfo( aQcStmt,
                               & aCallSpecNode->tableName );

        (void)sqlInfo.init(aQcStmt->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_PKG_BODY,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    aEnv->mOthersClauseDepth    = sOriOthersClauseDepth;
    aEnv->mProcPlan             = sOriProcPlan;

    return IDE_FAILURE;
}


// SQLCODE, SQLERRM ATTRIBUTE.
void qsxEnv::setErrorVariables ( qsxEnvInfo   * aEnv )
{
#define IDE_FN "void qsxEnv::setErrorVariables"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxEnv::setErrorCode( aEnv );
    qsxEnv::setErrorMessage( aEnv );

#undef IDE_FN
}

void qsxEnv::setErrorCode ( qsxEnvInfo   * aEnv )       // SQLCODE
{
#define IDE_FN "void qsxEnv::setErrorCode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // do not overwrite the original error code.
    if (ideGetErrorCode() != qpERR_ABORT_QSX_SQLTEXT_WRAPPER)
    {
        (aEnv)-> mSqlCode = sdi::getMultiErrorCode();
        (aEnv)-> mSqlCode4Restore = ideGetErrorCode();
    }

#undef IDE_FN
}

void qsxEnv::setErrorMessage ( qsxEnvInfo   * aEnv )    // SQLERRM
{
#define IDE_FN "void qsxEnv::setErrorMessage"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idlOS::strncpy( (aEnv)-> mSqlErrorMessage,
                    sdi::getMultiErrorMsg(),
                    MAX_ERROR_MSG_LEN );

    (aEnv)-> mSqlErrorMessage[ MAX_ERROR_MSG_LEN ] = '\0';

    // overwrite wrapped errorcode to original error code.
    ideGetErrorMgr()->Stack.LastError = (aEnv)-> mSqlCode4Restore;

#undef IDE_FN
}


void qsxEnv::clearErrorVariables ( qsxEnvInfo   * aEnv )
{
#define IDE_FN "void qsxEnv::clearErrorVariables ()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    (aEnv)-> mSqlCode = QSX_DEFAULT_SQLCODE;
    idlOS::strcpy( (aEnv)-> mSqlErrorMessage, QSX_DEFAULT_SQLERRM ) ;

#undef IDE_FN
}

IDE_RC qsxEnv::beginOthersClause ( qsxEnvInfo   * aEnv )
{
#define IDE_FN "IDE_RC qsxEnv::beginOthersClause()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE( (aEnv)-> mOthersClauseDepth < 0,
                    err_internal_others_depth_lt_0 );

    ( (aEnv)-> mOthersClauseDepth ) ++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_internal_others_depth_lt_0);
    {
        IDE_SET(ideSetErrorCode(
                qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                "[qsxEnv::beginOthersClause]err_internal_others_depth_lt_0"));

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN

}


IDE_RC qsxEnv::endOthersClause ( qsxEnvInfo   * aEnv )
{
#define IDE_FN "IDE_RC qsxEnv::endOthersClause()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));


    ( (aEnv)-> mOthersClauseDepth ) --;

    IDE_TEST_RAISE( (aEnv)-> mOthersClauseDepth < 0,
                    err_internal_others_depth_lt_0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_internal_others_depth_lt_0);
    {
        IDE_SET(ideSetErrorCode(
                qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                "[qsxEnv::endOthersClause]err_internal_others_depth_lt_0"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

IDE_RC qsxEnv::savepoint(
    qsxEnvInfo   * aEnv,
    const SChar* aSavePoint )
{
#define IDE_FN "IDE_RC qsxEnv::savepoint(const SChar* aSavePoint)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST ( QCG_SESSION_SAVEPOINT( aEnv,
                                      aSavePoint,
                                      ID_TRUE )
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN

}

IDE_RC qsxEnv::closeCursorsInUse( qsxEnvInfo    * aEnv,
                                  qcStatement   * aQcStmt )
{
    qsxCursorInfo * sCurInfo;
    qsxCursorInfo * sNxtInfo;
    UInt            sFailedCount = 0;

    QSX_ENV_CHECK_INITIALIZED( aEnv );

    // BUG-34331
    for ( sCurInfo = aEnv->mCursorsInUse;
          sCurInfo != NULL && sCurInfo != aEnv->mCursorsInUseFence;
          sCurInfo = sNxtInfo )
    {
        sNxtInfo = sCurInfo->mNext;

        // To Fix Bug-8986
        // stored procedure 내에서 commit 시에 cursor를 finalize
        // 하면 정보를 잃어버리므로 open된 cursor를 close만 해야 함
        if ( QSX_CURSOR_IS_OPEN(sCurInfo) == ID_TRUE )
        {
            if ( qsxCursor::close( sCurInfo, aQcStmt )
                 != IDE_SUCCESS )
            {
                IDE_TEST( (ideGetErrorCode() & E_ACTION_MASK)
                          != E_ACTION_FATAL );

                sFailedCount++;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // Nothing To Do
        }
    }

    IDE_TEST_RAISE( sFailedCount != 0, err_close );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_close);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                "[IDE_RC qsxEnv::closeCursorsInUse] closing cursors"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxEnv::addReturnArray( qsxEnvInfo    * aEnv,
                               qsxArrayInfo  * aArrayInfo )
{
    qsxReturnVarList  * sReturnVar;
    
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSN,
                                 ID_SIZEOF( qsxReturnVarList ),
                                 (void **)&sReturnVar )
              != IDE_SUCCESS );

    aArrayInfo->avlTree.refCount++;
    
    sReturnVar->mArrayInfo = aArrayInfo;
    sReturnVar->mNext      = aEnv->mReturnVar;

    aEnv->mReturnVar = sReturnVar;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qsxEnv::freeReturnArray( qsxEnvInfo * aEnv )
{
/***********************************************************************
 *
 * Description : PROJ-1075 function의 return값으로 넘어온 array 변수를
 *               할당 해제함.
 *
 * Implementation : iduMemMgr로 할당되어 있다.
 *
 ***********************************************************************/

    qsxReturnVarList  * sVarInfo;
    qsxReturnVarList  * sNextVarInfo;
 
    for ( sVarInfo = aEnv->mReturnVar;
          sVarInfo != NULL;
          sVarInfo = sNextVarInfo )
    {
        sNextVarInfo = sVarInfo->mNext;

        sVarInfo->mArrayInfo->avlTree.refCount--;

        // PROJ-1904 variable finalize
        (void) qsxArray::finalizeArray( &sVarInfo->mArrayInfo );

        (void) iduMemMgr::free( sVarInfo );
        sVarInfo = NULL;
    }

    aEnv->mReturnVar = NULL;
}

IDE_RC qsxEnv::commit ( qsxEnvInfo   * aEnv )
{
    qsxCursorInfo * sOriCursorsInUseFence;
    UInt            sStage;

    QSX_ENV_CHECK_INITIALIZED( aEnv );
    sStage = 0;

    if ( (aEnv)-> mSession != NULL )
    {
        /*
         * PROJ-1381: Fetch Across Commit
         *
         * commit 을 해도 cursor 를 닫지 않는다.
         * cursor 는 명시적으로 close cursor 를 하거나
         * psm 수행이 끝나면 닫힌다.
         *
         * fence 를 조정하는 이유는 다음과 같은 경우 때문이다.
         *
         * open cursor c1
         * commit
         * open cursor c2
         * rollback
         *
         * 이 경우 c1 cursor 는 계속 유효해야하지만
         * rollback 이후에 c2 cursor 는 남아있으면 안된다.
         * rollback 시 c1 이 닫히지 않도록 하기위해 fence 를 조정한다.
         */

        sOriCursorsInUseFence    = aEnv->mCursorsInUseFence;
        aEnv->mCursorsInUseFence = aEnv->mCursorsInUse;
        sStage = 1;

        // commit with preserving session-statements.
        IDE_TEST( QCG_SESSION_COMMIT( aEnv, ID_TRUE ) != IDE_SUCCESS );
        sStage = 0;
    }
// BUGBUG QCI interface does not exist yet.
/*
   else if ( (aEnv)-> dbc_ != NULL)
   {
   IDE_TEST_RAISE( (*( (aEnv)-> dbc_->sqlfTransact))( NULL, // env
   dbc_,
   SQL_COMMIT )
   != SQL_SUCCESS, err_qci_commit );
   }
*/
    else
    {
        IDE_RAISE(err_both_session_and_dbc_is_null);
    }

    return IDE_SUCCESS;

// BUGBUG QCI interface does not exist yet.
/*
   IDE_EXCEPTION(err_qci_commit);
   {
   IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_COMMIT_USING_QCI));
   }
*/
    IDE_EXCEPTION(err_both_session_and_dbc_is_null);
    {
        IDE_SET(ideSetErrorCode(
                qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                "[qsxEnv::commit]err_both_session_and_dbc_is_null"));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            aEnv->mCursorsInUseFence = sOriCursorsInUseFence;
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxEnv::rollback(
    qsxEnvInfo   * aEnv,
    qcStatement  * aQcStmt,
    const SChar  * aSavePoint )
{
    smiTrans    * sTrans = NULL;

#define IDE_FN "IDE_RC qsxEnv::rollback()"

    if( ( aQcStmt != NULL) && (aQcStmt->session != NULL ) )
    {
        sTrans = qci::mSessionCallback.mGetTrans( aQcStmt->session->mMmSession );
    }

    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    QSX_ENV_CHECK_INITIALIZED( aEnv );

    if ( (aEnv)-> mSession != NULL)
    {
        if( sTrans != NULL )
        {
            if( sTrans->isReusableRollback() == ID_FALSE )
            {
                // close all procedure cursors
                IDE_TEST( qsxEnv::closeCursorsInUse( aEnv,
                                                     aQcStmt )
                          != IDE_SUCCESS);
            }
            else
            {
                /* PROJ-2964 Fetch Across Rollback 
                 * fetch across rollback을 위해 rollback 후에
                 * view가 깨지지 않는 경우 rollback을 수행해도 cursor를 닫지 않는다. */
            }
        }
        // rollback with preserving session-statements.

        if ( aSavePoint != NULL )
        {
            if ( aSavePoint[0] == '\0' )
            {
                aSavePoint = NULL;
            }
        }

        // rollback with preserving session-statements.
        IDE_TEST( QCG_SESSION_ROLLBACK( aEnv, aSavePoint, ID_TRUE )
                  != IDE_SUCCESS);
    }
// BUGBUG QCI interface does not exist yet.
/*
   else if (dbc_ != NULL)
   {
   IDE_TEST_RAISE( (*( (aEnv)-> dbc_->sqlfTransact))( NULL, // env
   dbc_,
   SQL_ROLLBACK )
   != SQL_SUCCESS, err_qci_rollback );
   }
*/
    else
    {
        IDE_RAISE(err_both_session_and_dbc_is_null);
    }

    return IDE_SUCCESS;

// BUGBUG QCI interface does not exist yet.
/*
   IDE_EXCEPTION(err_qci_rollback);
   {
   IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_ROLLBACK_USING_QCI));
   }
*/
    IDE_EXCEPTION(err_both_session_and_dbc_is_null);
    {
        IDE_SET(ideSetErrorCode(
                qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                "[qsxEnv::rollback]err_both_session_and_dbc_is_null"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN

}

// critical errors are not catched by others clause
idBool qsxEnv::isCriticalError(UInt aErrorCode)
{
    // fix BUG-12552
    switch( aErrorCode & E_ACTION_MASK )
    {
        case E_ACTION_FATAL:
            // BUG-24281
            // PSM의 fetch 구문 수행시 발생하는 retry 에러는 PSM에서 abort 에러로 바꿔
            // retry 에러가 상위 모듈로 전달되지 않도록 처리한다.
            // case E_ACTION_RETRY:
            // case E_ACTION_REBUILD:

            return ID_TRUE;
    }

    switch(aErrorCode)
    {
        case qpERR_ABORT_QCM_INTERNAL_ARG :

            /* BUGBUG list up fatal errors that is defined as _ABORT_  in QP2 */
        case smERR_ABORT_INTERNAL_ARG :

        case qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG :
        case qpERR_ABORT_QSX_NOT_ENOUGH_MEMORY_ARG2:
        case qpERR_ABORT_QSX_NOT_ENOUGH_MEMORY_ARG_SQLTEXT :
        case qpERR_ABORT_QSX_FUNCTION_WITH_NO_RETURN :
        case qpERR_ABORT_QSX_CONNECTION_CLOSED :

            return ID_TRUE;
    }

    return ID_FALSE;
}

// fix BUG-32565
void 
qsxEnv::fixErrorMessage( qsxEnvInfo * aEnv )
{
    if( aEnv->mSqlCode == 0 )
    {
        clearErrorVariables( aEnv );
    }
}

// BUG-42322
void qsxEnv::pushStack( qsxEnvInfo * aEnv )
{
    aEnv->mStackCursor++;

    if ( aEnv->mStackCursor <= QSX_MAX_CALL_DEPTH )
    { 
        aEnv->mStackBuffer[aEnv->mStackCursor] = aEnv->mPrevStackInfo;
    }
    else
    {
        IDE_DASSERT( aEnv->mStackCursor <= QSX_MAX_CALL_DEPTH );
    }
}

void qsxEnv::popStack( qsxEnvInfo * aEnv )
{
    aEnv->mStackCursor--;

    if ( aEnv->mStackCursor >= -1 )
    {
        // Nothing to do.
    }
    else
    {
        IDE_DASSERT( aEnv->mStackCursor >= -1 );
    } 
}

void qsxEnv::copyStack( qsxEnvInfo * aTargetEnv,
                        qsxEnvInfo * aSourceEnv )
{
    if ( aSourceEnv->mStackCursor >= 0 )
    {
        idlOS::memcpy( aTargetEnv->mStackBuffer,
                       aSourceEnv->mStackBuffer,
                       ID_SIZEOF(qsxStackFrame) * ( aSourceEnv->mStackCursor + 1 ) );
 
        aTargetEnv->mStackCursor   = aSourceEnv->mStackCursor;

        aTargetEnv->mPrevStackInfo = aSourceEnv->mPrevStackInfo;
    }
    else
    {
        // Nothing to do.
    }
}

void qsxEnv::setStackInfo( qsxEnvInfo * aEnv,
                           qsOID        aOID,
                           SInt         aLineno,
                           SChar      * aObjectType,
                           SChar      * aUserAndObjectName )
{
    if ( ( QCU_PSM_FORMAT_CALL_STACK_OID != 0 ) && ( aOID != 0 ) )
    {
        aOID = QCU_PSM_FORMAT_CALL_STACK_OID; 
    }
    else
    {
        // Nothing to do.
    }

    aEnv->mPrevStackInfo.mOID               = aOID;
    aEnv->mPrevStackInfo.mLineno            = aLineno;
    aEnv->mPrevStackInfo.mObjectType        = aObjectType;
    aEnv->mPrevStackInfo.mUserAndObjectName = aUserAndObjectName;
}

void qsxEnv::initializeRaisedExcpInfo( qsxRaisedExcpInfo * aRaisedExcpInfo )
{
    /* 초기값 셋팅 */
    aRaisedExcpInfo->mRaisedExcpOID         = QS_EMPTY_OID;
    aRaisedExcpInfo->mRaisedExcpId          = QSX_FLOW_ID_INVALID;
    aRaisedExcpInfo->mRaisedExcpErrorMsgLen = 0;
    /* qcg::allocStatement에서 qsxEnvInfo을 calloc으로 하기 때문에
       mRaisedExcpErrorMsg에 대해서는 초기화 되어 있다. */
}

// TASK-7244 PSM partial rollback in Sharding
void qsxEnv::setBeginSP( qsxEnvInfo * aEnv )
{
    aEnv->mFlag |= QSX_ENV_BEGIN_PROC;
}

// TASK-7244 PSM partial rollback in Sharding
void qsxEnv::unsetBeginSP( qsxEnvInfo * aEnv )
{
    aEnv->mFlag &= ~(QSX_ENV_BEGIN_PROC);
}

// TASK-7244 PSM partial rollback in Sharding
idBool qsxEnv::isBeginSP( qsxEnvInfo * aEnv )
{
    return  ((aEnv->mFlag & QSX_ENV_BEGIN_PROC) == QSX_ENV_BEGIN_PROC)?ID_TRUE:ID_FALSE;
}

