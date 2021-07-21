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
 * $Id: qsc.cpp 87536 2020-05-18 06:56:41Z khkwak $
 **********************************************************************/

#include <qsc.h>

// Called by qci::initializeSession
IDE_RC qsc::initialize( qmcThrMgr  ** aThrMgr,
                        qscConcMgr ** aConcMgr )
{
    qmcThrMgr  * sThrMgr  = NULL;
    qscConcMgr * sConcMgr = NULL;
    void       * sMemMgr  = NULL;
    UInt         sStage   = 0;

    //PROJ-2451 FIT TEST
    IDU_FIT_POINT("qsc::initialize::malloc::qmcThrMgr",
                  idERR_ABORT_InsufficientMemory);
    // INITIALIZE THREAD MANAGER
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSC,
                                 ID_SIZEOF(qmcThrMgr),
                                 (void**)&sThrMgr )
              != IDE_SUCCESS);
    sStage = 1;

    sThrMgr->mThrCnt = 0;
    sThrMgr->mMemory = NULL;

    //PROJ-2451 FIT TEST
    IDU_FIT_POINT("qsc::initialize::malloc::iduMemory",
                  idERR_ABORT_InsufficientMemory);
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSC,
                                 ID_SIZEOF(iduMemory),
                                 (void**)&sThrMgr->mMemory )
              != IDE_SUCCESS);
    sStage = 2;

    sThrMgr->mMemory = new (sThrMgr->mMemory) iduMemory;

    // always returns IDE_SUCCESS.
    IDE_TEST( sThrMgr->mMemory->init( IDU_MEM_QSC )
              != IDE_SUCCESS );
    sStage = 3;

    IDU_LIST_INIT( &sThrMgr->mUseList );
    IDU_LIST_INIT( &sThrMgr->mFreeList );

    //PROJ-2451 FIT TEST
    IDU_FIT_POINT("qsc::initialize::malloc::qscConcMgr",
                  idERR_ABORT_InsufficientMemory);
    // INITIALIZE CONCCURRENT MANAGER
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSC,
                                 ID_SIZEOF(qscConcMgr),
                                 (void**)&sConcMgr )
              != IDE_SUCCESS);
    sStage = 4;

    // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
    IDU_FIT_POINT("qsc::initialize::qcg::allocIduVarMemList");
    IDE_TEST( qcg::allocIduVarMemList((void**)&sMemMgr)
              != IDE_SUCCESS );
    sStage = 5;

    sConcMgr->memory = new (sMemMgr) iduVarMemList;

    // always returns IDE_SUCCESS.
    // 2번째 인자는 메모리 최대 크기 생략하면 ID_UINT_MAX.
    IDE_TEST( sConcMgr->memory->init( IDU_MEM_QSC )
              != IDE_SUCCESS );
    sStage = 6;

    IDE_TEST( sConcMgr->mutex.initialize( (SChar*)"CONCURRENT_EXEC_PACKAGE_MUTEX",
                                          IDU_MUTEX_KIND_NATIVE,
                                          IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sStage = 7;

    sConcMgr->errMaxCnt = 0;
    sConcMgr->errCnt    = 0;
    sConcMgr->errArr    = NULL;

    *aThrMgr  = sThrMgr;
    *aConcMgr = sConcMgr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 6:
        {
            (void)sConcMgr->memory->destroy();
            /* fall through */
        }
        case 5:
        {
            (void)qcg::freeIduVarMemList( sConcMgr->memory );
            sConcMgr->memory = NULL;
            /* fall through */
        }
        case 4:
        {
            (void)iduMemMgr::free( sConcMgr );
            sConcMgr = NULL;
            /* fall through */
        }
        case 3:
        {
            (void)sThrMgr->mMemory->destroy();
            /* fall through */
        }
        case 2:
        {
            (void)iduMemMgr::free( sThrMgr->mMemory );
            sThrMgr->mMemory = NULL;
            /* fall through */
        }
        case 1:
        {
            (void)iduMemMgr::free( sThrMgr );
            sThrMgr = NULL;
            /* fall through */
            break;
        }
        default:
        {
            // Nothing to do.
            break;
        }
    }

    return IDE_FAILURE;
}

// Called by qci::finalizeSession
IDE_RC qsc::finalize( qmcThrMgr  ** aThrMgr,
                      qscConcMgr ** aConcMgr )
{
    qmcThrMgr  * sThrMgr;
    qscConcMgr * sConcMgr;
    UInt         sThrCnt;

    // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
    IDU_FIT_POINT_RAISE("qsc::finalize::invalid::condition1",
                         ERR_INVALID_CONDITION);
    IDE_ERROR_RAISE( aThrMgr  != NULL, ERR_INVALID_CONDITION );
    IDE_ERROR_RAISE( aConcMgr != NULL, ERR_INVALID_CONDITION );

    sThrMgr  = *aThrMgr;
    sConcMgr = *aConcMgr;

    // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
    IDU_FIT_POINT_RAISE("qsc::finalize::invalid::condition2",
                         ERR_INVALID_CONDITION);
    IDE_ERROR_RAISE( sThrMgr  != NULL, ERR_INVALID_CONDITION );
    IDE_ERROR_RAISE( sConcMgr != NULL, ERR_INVALID_CONDITION );

    // 실패하는 경우에는 FATAL 발생한다.
    IDE_TEST( sConcMgr->mutex.destroy() != IDE_SUCCESS );

    sThrCnt = sThrMgr->mThrCnt;

    // 실패하는 경우에는 함수 내부에서 FATAL로 설정한다.
    IDE_TEST( qmcThrObjFinal( sThrMgr )
              != IDE_SUCCESS );

    // 실패하는 경우에는 함수 내부에서 FATAL로 설정한다.
    if ( sThrCnt > 0 )
    {
        IDE_TEST( qcg::releaseConcThr( sThrCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( sThrMgr->mMemory != NULL )
    {
        (void)sThrMgr->mMemory->destroy();
        (void)iduMemMgr::free( sThrMgr->mMemory );
        sThrMgr->mMemory = NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( sConcMgr->memory != NULL )
    {
        (void)sConcMgr->memory->destroy();
        (void)qcg::freeIduVarMemList(sConcMgr->memory);
        sConcMgr->memory = NULL;
    }
    else
    {
        // Nothing to do.
    }

    (void)iduMemMgr::free( sThrMgr );
    (void)iduMemMgr::free( sConcMgr );

    *aThrMgr  = NULL;
    *aConcMgr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_CONDITION )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qsc::finalize",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// Called by DBMS_CONCURRENT_EXEC.INITIALIZE
IDE_RC qsc::prepare( qmcThrMgr  * aThrMgr,
                     qscConcMgr * aConcMgr,
                     UInt         aThrCnt )
{
    UInt sStage = 0;

    // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
    IDU_FIT_POINT_RAISE("qsc::prepare::invalid::condition1",
                         ERR_INVALID_CONDITION);
    IDE_ERROR_RAISE( aThrMgr != NULL,  ERR_INVALID_CONDITION );
    IDE_ERROR_RAISE( aConcMgr != NULL, ERR_INVALID_CONDITION );
    IDE_ERROR_RAISE( aThrCnt != 0,     ERR_INVALID_CONDITION );
    IDE_ERROR_RAISE( aConcMgr->memory != NULL, ERR_INVALID_CONDITION );

    IDE_TEST( qmcThrObjCreate( aThrMgr, aThrCnt )
              != IDE_SUCCESS );
    sStage = 1;

    aConcMgr->errMaxCnt = aThrCnt * 2;
    aConcMgr->errCnt    = 0;

    //PROJ-2451 FIT TEST
    IDU_FIT_POINT("qsc::prepare::alloc::qscErrors",
                  idERR_ABORT_InsufficientMemory);

    IDE_TEST( aConcMgr->memory->alloc( ID_SIZEOF(qscErrors) * aConcMgr->errMaxCnt,
                                       (void**)&aConcMgr->errArr )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_CONDITION )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qsc::prepare",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)qmcThrObjFinal( aThrMgr );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

// Called by DBMS_CONCURRENT_EXEC.REQUEST -> function for thread execution
// Execute a procedure.
IDE_RC qsc::execute( qmcThrObj * aThrArg )
{
    qscConcMgr   * sConcMgr;
    qscExecInfo  * sExecInfo;
    smiTrans     * sTrans;
    QCD_HSTMT      sHstmt;
    void         * sMmSession;
    qcStatement  * sStatement;
    SChar        * sProcStr;
    UInt           sProcStrLen;
    qciStmtType    sStmtType;
    vSLong         sAffectedRowCount = 0;
    idBool         sResultSetExist   = ID_FALSE;
    idBool         sRecordExist      = ID_FALSE;
    UInt           sStage   = 1;

    sExecInfo   = (qscExecInfo*)aThrArg->mPrivateArg;
    sMmSession  = sExecInfo->mmSession;
    sConcMgr    = sExecInfo->concMgr;
    sProcStr    = sExecInfo->execStr;
    sProcStrLen = idlOS::strlen(sProcStr);

    sTrans     = qci::mSessionCallback.mGetTrans( sMmSession );

    // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
    IDU_FIT_POINT_RAISE("qsc::execute::invalid::condition1",
                        ERR_INVALID_CONDITION);
    IDE_ERROR_RAISE( sTrans != NULL, ERR_INVALID_CONDITION );
    sStage = 2;

    IDE_TEST( qcd::allocStmtNoParent( sMmSession,
                                      ID_TRUE,  // dedicated mode
                                      & sHstmt )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sStatement )
              != IDE_SUCCESS );

    sStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_INTERNAL_EXEC_MASK;
    sStatement->session->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_EXEC_TRUE;

    IDE_TEST( qcd::prepare( sHstmt,
                            NULL,
                            NULL,
                            & sStmtType,
                            sProcStr,
                            sProcStrLen,
                            ID_TRUE )  // direct-execute mode
              != IDE_SUCCESS );

    IDE_TEST( qcd::executeNoParent( sHstmt,
                                    NULL,
                                    & sAffectedRowCount,
                                    & sResultSetExist,
                                    & sRecordExist,
                                    ID_TRUE )
              != IDE_SUCCESS );

    sStage = 2;
    IDE_TEST( qcd::freeStmt( sHstmt,
                             ID_TRUE )  // free & drop
              != IDE_SUCCESS );

    sStage = 1;
    /* 4. 성공시  Commit을 한다 */
    if ( qci::mSessionCallback.mCommit( sMmSession, ID_FALSE ) != IDE_SUCCESS )
    {
        ideLog::log( IDE_QP_0, "[FAILURE] error code 0x%05X %s",
                     E_ERROR_CODE(ideGetErrorCode()),
                     ideGetErrorMsg(ideGetErrorCode()));

        IDE_TEST( 1 );
    }
    else
    {
        /* Nothing to do */
    }

    sStage = 0;
    qci::mSessionCallback.mFreeInternalSession( sExecInfo->mmSession,
                                                ID_TRUE /* aIsSuccess */ );
    sExecInfo->mmSession = NULL;

    IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );

    (void)sConcMgr->memory->free( sExecInfo->execStr );
    sExecInfo->execStr = NULL;

    (void)sConcMgr->memory->free( sExecInfo );
    aThrArg->mPrivateArg = NULL;

    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_CONDITION )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qsc::execute",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;

    if ( sStage != 0 )
    {
        (void)qsc::setError( aThrArg );
    }
    else
    {
        // Nothing to do.
    }

    switch ( sStage )
    {
        case 3:
            (void)qcd::freeStmt( sHstmt, ID_TRUE );
            /* fall through */
        case 2:
            (void)qci::mSessionCallback.mRollback( sMmSession, NULL, ID_FALSE );
            /* fall through */
        case 1:
            (void) qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                               ID_FALSE /* aIsSuccess */ );
            /* fall through */
            break;
        default:
            // Nothing to do.
            break;
    }

    return IDE_FAILURE;
}

// Called by qsc::execute
// Setting an error to qscConcMgr->errArr
void qsc::setError( qmcThrObj * aThrArg )
{
    qscExecInfo  * sExecInfo;
    qscConcMgr   * sConcMgr;
    SChar        * sErrMsg;
    SChar        * sProcStr;
    qscErrors    * sError;

    sExecInfo   = (qscExecInfo*)aThrArg->mPrivateArg;
    sProcStr    = sExecInfo->execStr;
    sConcMgr    = sExecInfo->concMgr;

    IDE_ASSERT( sConcMgr->mutex.lock(NULL) == IDE_SUCCESS );

    if ( sConcMgr->errCnt < sConcMgr->errMaxCnt )
    {
        IDE_DASSERT( sExecInfo          != NULL );
        IDE_DASSERT( sExecInfo->execStr != NULL );

        sError = &(sConcMgr->errArr[sConcMgr->errCnt]);
        sConcMgr->errCnt++;

        // BUG-40498 An error code shown to user must be masked.
        sError->errCode = E_ERROR_CODE(ideGetErrorCode());
        sErrMsg         = ideGetErrorMsg(ideGetErrorCode());

        if ( sErrMsg != NULL )
        {
            (void)sConcMgr->memory->alloc( idlOS::strlen(sErrMsg) + 1,
                                           (void**)&sError->errMsg );

            if ( sError->errMsg != NULL )
            {
                idlOS::strncpy( sError->errMsg, sErrMsg, idlOS::strlen(sErrMsg) );
                sError->errMsg[idlOS::strlen(sErrMsg)] = '\0';
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sError->errMsg = NULL;
        }
     
        // sProcStr을 sConcMgr->memory로 할당했으므로
        // 메모리를 새로 할당해서 copy하지 않는다.
        // double-free를 방지하기 위해 sExecInfo->execStr을 NULL로 변경한다.
        sError->execStr    = sProcStr;
        sError->reqID      = sExecInfo->reqID;
    }
    else
    {
        // Nothing to do.
        IDE_DASSERT(0);
    }

    sExecInfo->execStr   = NULL;
    sExecInfo->mmSession = NULL;
    sExecInfo->concMgr   = NULL;

    (void)sConcMgr->memory->free( sExecInfo );
    // aThrArg->mPrivateArg == sExecInfo
    aThrArg->mPrivateArg = NULL;
 
    IDE_ASSERT( sConcMgr->mutex.unlock() == IDE_SUCCESS );
}
