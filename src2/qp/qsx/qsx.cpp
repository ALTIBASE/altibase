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
 * $Id: qsx.cpp 90976 2021-06-09 01:45:59Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idx.h>
#include <mtuProperty.h>
#include <qcuProperty.h>
#include <qcmProc.h>
#include <qcmPkg.h>
#include <qcg.h>
#include <qsv.h>
#include <qcpManager.h>
#include <qcuSqlSourceInfo.h>
#include <qcmPriv.h>
#include <qcmUser.h>
#include <qcmView.h>
#include <qsvEnv.h>
#include <qsx.h>
#include <qsxProc.h>
#include <qsxEnv.h>
#include <qsxUtil.h>
#include <qsxExecutor.h>
#include <qsxRefCursor.h>
#include <qcgPlan.h>
#include <qcmAudit.h>
#include <qcuSessionPkg.h>
#include <qsxLibrary.h>
#include <sdi.h>

IDE_RC qsx::makeProcInfoMembers( qsxProcInfo * aProcInfo,
                                 qcStatement * aStatement )
{
    // TASK-7244 Set shard split method to PSM info
    sdiSplitMethod  sShardSplitMethod = SDI_SPLIT_NONE;

    aProcInfo->relatedObjects = aStatement->spvEnv->relatedObjects;

    if( QC_PRIVATE_TMPLATE( aStatement ) != NULL )
    {
        aProcInfo->tmplate = QC_PRIVATE_TMPLATE(aStatement);
    }
    else
    {
        aProcInfo->tmplate = QC_SHARED_TMPLATE(aStatement);
    }

    // BUG-31422
    // procedure template의 statement는 procedure 생성 후에는 참조할 수 없다.
    aProcInfo->tmplate->stmt = NULL;

    IDE_TEST(qcmPriv::getGranteeOfPSM(
                 QC_SMI_STMT(aStatement),
                 QCM_OID_TO_BIGINT(aProcInfo->procOID),
                 &(aProcInfo->privilegeCount),
                 &(aProcInfo->granteeID) )
             != IDE_SUCCESS);

    IDE_TEST( qsxRelatedProc::unlatchObjects( aStatement->spvEnv->procPlanList )
              != IDE_SUCCESS );

    aStatement->spvEnv->latched = ID_FALSE;

    aProcInfo->planTree = (qsProcParseTree *)aStatement->myPlan->parseTree;

    aProcInfo->planTree->procOID     = aProcInfo->procOID;
    aProcInfo->planTree->stmtText    = aStatement->myPlan->stmtText;
    aProcInfo->planTree->stmtTextLen = aStatement->myPlan->stmtTextLen;
    aProcInfo->planTree->procInfo    = aProcInfo;

    // TASK-7244 Set shard split method to PSM info
    if ( SDU_SHARD_ENABLE == 1 )
    {
        if ( sdi::getProcedureShardSplitMethod( aStatement,
                                                aProcInfo->planTree->userID,
                                                aProcInfo->planTree->userNamePos,
                                                aProcInfo->planTree->procNamePos,
                                                &sShardSplitMethod )
             == IDE_SUCCESS )
        {
            aProcInfo->shardSplitMethod = sShardSplitMethod;
        }
        else
        {
            IDE_CLEAR();
            aProcInfo->shardSplitMethod = SDI_SPLIT_NONE;
        }
    }
    else
    {
        aProcInfo->shardSplitMethod = SDI_SPLIT_NONE;
    }

    // BUG-36203 PSM Optimize
    IDE_TEST( QC_QMP_MEM(aStatement)->cralloc(
            ID_SIZEOF(qsxTemplateCache),
            (void**)&aProcInfo->cache)
        != IDE_SUCCESS);

    if( QCU_PSM_TEMPLATE_CACHE_COUNT > 0 )
    {
        IDE_TEST( makeTemplateCache( aStatement,
                                     aProcInfo,
                                     aProcInfo->tmplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::makePkgInfoMembers( qsxPkgInfo  * aPkgInfo,
                                qcStatement * aStatement )
{
    aPkgInfo->relatedObjects = aStatement->spvEnv->relatedObjects;

    if( QC_PRIVATE_TMPLATE( aStatement ) != NULL )
    {
        aPkgInfo->tmplate = QC_PRIVATE_TMPLATE(aStatement);
    }
    else
    {
        aPkgInfo->tmplate = QC_SHARED_TMPLATE(aStatement);
    }

    // BUG-31422
    // procedure template의 statement는 procedure 생성 후에는 참조할 수 없다.
    aPkgInfo->tmplate->stmt = NULL;

    IDE_TEST(qcmPriv::getGranteeOfPkg(
                 QC_SMI_STMT(aStatement),
                 QCM_OID_TO_BIGINT( aPkgInfo->pkgOID ),
                 &(aPkgInfo->privilegeCount),
                 &(aPkgInfo->granteeID) )
             != IDE_SUCCESS);

    IDE_TEST( qsxRelatedProc::unlatchObjects( aStatement->spvEnv->procPlanList )
              != IDE_SUCCESS );

    aStatement->spvEnv->latched = ID_FALSE;

    aPkgInfo->planTree = (qsPkgParseTree *)aStatement->myPlan->parseTree;
    aPkgInfo->objType = aPkgInfo->planTree->objType;

    aPkgInfo->planTree->pkgOID      = aPkgInfo->pkgOID;
    aPkgInfo->planTree->stmtText    = aStatement->myPlan->stmtText;
    aPkgInfo->planTree->stmtTextLen = aStatement->myPlan->stmtTextLen;
    aPkgInfo->planTree->pkgInfo     = aPkgInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::createProcOrFunc(qcStatement * aStatement)
{
    qsProcParseTree    * sParseTree;
    qsxProcObjectInfo  * sProcObjectInfo;
    qsxProcInfo        * sProcInfo ;
    qsOID                sProcOID = 0;
    const void         * sSmiObjHandle;
    iduVarMemList      * sQmsMem = NULL;
    UInt                 sStage = 0;

    UInt                 sLibStage = 0;
    qsCallSpec         * sCallSpec;

    sParseTree = (qsProcParseTree *)(aStatement->myPlan->parseTree);
    sCallSpec  = sParseTree->expCallSpec;
    
    IDE_TEST( smiObject::createObject( QC_SMI_STMT(aStatement),
                                       NULL,
                                       0,
                                       NULL,
                                       SMI_OBJECT_PROCEDURE,
                                       &sSmiObjHandle )
              != IDE_SUCCESS );

    sProcOID = (qsOID) smiGetTableId( sSmiObjHandle );
    sStage   = 1;

    IDE_TEST( qsxProc::createProcObjectInfo( sProcOID, &sProcObjectInfo )
              != IDE_SUCCESS );
    sStage   = 2;

    IDE_TEST( qsxProc::setProcObjectInfo(  sProcOID,
                                           sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_TEST( qsxProc::createProcInfo( sProcOID, &sProcInfo )
              != IDE_SUCCESS );
    sStage   = 3;

    IDE_TEST( qsxProc::setProcInfo( sProcOID,
                                    sProcInfo )
              != IDE_SUCCESS );

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_FALSE );

    IDE_TEST( qsx::makeProcInfoMembers ( sProcInfo, aStatement )
              != IDE_SUCCESS );

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_TRUE );

    IDE_TEST( qsxProc::latchX( sProcOID,
                               ID_TRUE )
              != IDE_SUCCESS );
    sStage = 4;

    sProcInfo->isValid = ID_TRUE;

    if ( sParseTree->procType == QS_INTERNAL_C )
    {
        IDE_TEST( qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                           &(sCallSpec->functionPtr),
                                           sCallSpec->fileSpec,
                                           sCallSpec->functionName )
                  != IDE_SUCCESS );

        IDE_ERROR( sCallSpec->libraryNode != NULL );
        sLibStage = 1;

        if ( (sCallSpec->libraryNode->mHandle == PDL_SHLIB_INVALID_HANDLE) ||
             (sCallSpec->libraryNode->mFunctionPtr == NULL) ||
             (sCallSpec->functionPtr == NULL) )
        {
            //   Library를 load하지 못했거나, function을 제대로 찾지 못한경우
            //   invalid 상태로 만들어 다음에 rebuild하면서
            //   library를 load하거나 function을 찾도록 한다.
            sProcInfo->isValid = ID_FALSE;
        }
    }
    else if ( sParseTree->procType == QS_EXTERNAL_C )
    {
        sCallSpec->libraryNode = NULL;
        sCallSpec->functionPtr = NULL;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qcmProc::insert( aStatement, sParseTree )
              != IDE_SUCCESS );

    IDE_TEST( qsxProc::setProcText( QC_SMI_STMT( aStatement ),
                                    sProcOID,
                                    aStatement->myPlan->stmtText,
                                    aStatement->myPlan->stmtTextLen )
              != IDE_SUCCESS );

    sQmsMem = QC_QMP_MEM( aStatement );
    IDE_TEST( qsx::makeNewPreparedMem( aStatement ) != IDE_SUCCESS );

    sProcInfo->qmsMem  = sQmsMem;

    sStage = 3;
    IDE_TEST( qsxProc::unlatch( sProcOID ) != IDE_SUCCESS );

    // BUG-11266
    IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
                 aStatement,
                 sParseTree->userID,
                 (SChar *) (sParseTree->procNamePos.stmtText +
                            sParseTree->procNamePos.offset),
                 sParseTree->procNamePos.size,
                 QS_PROC)
             != IDE_SUCCESS);

    IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
                 aStatement,
                 sParseTree->userID,
                 (SChar *) (sParseTree->procNamePos.stmtText +
                            sParseTree->procNamePos.offset),
                 sParseTree->procNamePos.size,
                 QS_FUNC)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLibStage == 1 )
    {
        (void)qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                         &(sCallSpec->functionPtr) );
    }

    switch( sStage)
    {
        case 4:
            if ( qsxProc::unlatch( sProcOID ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 3:
            if ( qsxProc::destroyProcInfo ( &sProcInfo ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 2:
            if ( qsxProc::destroyProcObjectInfo ( &sProcObjectInfo )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 1:
            if ( smiObject::dropObject( QC_SMI_STMT( aStatement ), sSmiObjHandle )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        default:
            break;
    }

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_TRUE );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsx::replaceProcOrFunc(qcStatement * aStatement)
{
    qsProcParseTree    * sPlanTree;
    qsxProcInfo        * sProcInfo ;
    qsOID                sProcOID = 0;
    UInt                 sProcUserID;
    UInt                 sStage = 0;
    qsxProcInfo          sOriProcInfo;
    iduVarMemList      * sQmsMem = NULL;

    qsCallSpec         * sCallSpec;
    UInt                 sLibStage = 0;

    sPlanTree = (qsProcParseTree *)(aStatement->myPlan->parseTree);

    sProcOID = sPlanTree->procOID;

    IDE_TEST( qsxProc::latchX( sProcOID,
                               ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( qsxProc::latchXForStatus( sProcOID )
              != IDE_SUCCESS );
    sStage = 2;

    /* BUG-48290 shard object에 대한 DDL 차단 */
    // BUG-48919 Execute 시점에서 shard object 여부를 확인해야 한다.
    if ( SDU_SHARD_ENABLE == 1 )
    {
        IDE_TEST( sdi::checkShardObjectForDDL( aStatement, SDI_DDL_TYPE_PROCEDURE ) != IDE_SUCCESS );
    }

    IDE_TEST( qsxProc::getProcInfo( sProcOID,
                                    &sProcInfo ) != IDE_SUCCESS );

    idlOS::memcpy( &sOriProcInfo,
                   sProcInfo,
                   ID_SIZEOF(qsxProcInfo) );
    sProcInfo->cache = NULL;
    sStage = 3;

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_FALSE );

    //To Fix BUG-19839 : ProcedureOID를 사용하여 소유자의 UserID를 얻는다.
    IDE_TEST( qcmProc::getProcUserID( aStatement, sProcOID, &sProcUserID )
              != IDE_SUCCESS );

    IDE_TEST( qsx::makeProcInfoMembers( sProcInfo, aStatement )
              != IDE_SUCCESS );

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_TRUE );

    if ( sOriProcInfo.planTree != NULL )
    {
        if ( sOriProcInfo.planTree->procType == QS_INTERNAL_C )
        {
            sCallSpec = sOriProcInfo.planTree->expCallSpec;

            IDE_TEST( qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                 &(sCallSpec->functionPtr) )
                      != IDE_SUCCESS );
            sLibStage = 1;
        }
    }

    sProcInfo->isValid = ID_TRUE;

    if ( sProcInfo->planTree->procType == QS_INTERNAL_C )
    {
        sCallSpec = sProcInfo->planTree->expCallSpec;

        IDE_TEST( qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                           &(sCallSpec->functionPtr),
                                          sCallSpec->fileSpec,
                                          sCallSpec->functionName )
                  != IDE_SUCCESS );

        IDE_ERROR( sCallSpec->libraryNode != NULL );
        sLibStage += 2;

        //   Library를 load하지 못했거나, function을 제대로 찾지 못한경우
        //   invalid 상태로 만들어 다음에 rebuild하면서
        //   library를 load하거나 function을 찾도록 한다.
        if ( (sCallSpec->libraryNode->mHandle == PDL_SHLIB_INVALID_HANDLE) ||
             (sCallSpec->libraryNode->mFunctionPtr == NULL) ||
             (sCallSpec->functionPtr == NULL) )
        {
            sProcInfo->isValid = ID_FALSE;
        }
    }
    else if ( sProcInfo->planTree->procType == QS_EXTERNAL_C )
    {
        sCallSpec = sProcInfo->planTree->expCallSpec;

        sCallSpec->libraryNode  = NULL;
        sCallSpec->functionPtr = NULL;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qcmProc::remove( aStatement,
                               sProcOID,
                               sPlanTree->userNamePos,
                               sPlanTree->procNamePos,
                               ID_TRUE /* aPreservePrivInfo */  ) != IDE_SUCCESS );

    IDE_TEST( qcmProc::insert( aStatement, sPlanTree ) != IDE_SUCCESS );

    IDE_TEST( qsxProc::setProcText( QC_SMI_STMT( aStatement ),
                                    sProcOID,
                                    aStatement->myPlan->stmtText,
                                    aStatement->myPlan->stmtTextLen )
              != IDE_SUCCESS );

    IDE_TEST( qsxProc::setProcInfo( sProcOID,
                                    sProcInfo )
              != IDE_SUCCESS );

    sQmsMem = QC_QMP_MEM( aStatement );
    IDE_TEST( qsx::makeNewPreparedMem( aStatement ) != IDE_SUCCESS );

    sProcInfo->qmsMem  = sQmsMem;
    sProcInfo->modifyCount ++;
    sStage = 2;

    // BUG-37382
    IDE_TEST( qsx::freeTemplateCache( &sOriProcInfo ) != IDE_SUCCESS );

    if( sOriProcInfo.qmsMem != NULL )
    {
        IDE_TEST( sOriProcInfo.qmsMem->destroy() != IDE_SUCCESS );

        IDE_TEST( qcg::freeIduVarMemList( sOriProcInfo.qmsMem ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-11266
    IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
                 aStatement,
                 sPlanTree->userID,
                 (SChar *) (sPlanTree->procNamePos.stmtText +
                            sPlanTree->procNamePos.offset),
                 sPlanTree->procNamePos.size,
                 QS_PROC)
             != IDE_SUCCESS);

    IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
                 aStatement,
                 sPlanTree->userID,
                 (SChar *) (sPlanTree->procNamePos.stmtText +
                            sPlanTree->procNamePos.offset),
                 sPlanTree->procNamePos.size,
                 QS_FUNC)
             != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST( qsxProc::unlatchForStatus( sProcOID ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( qsxProc::unlatch( sProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sLibStage )
    {
        case 3:
            /* fall through */
        case 2:
            if ( sProcInfo->planTree->procType == QS_INTERNAL_C )
            {
                sCallSpec = sProcInfo->planTree->expCallSpec;
                (void)qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                 &(sCallSpec->functionPtr) );
            }
            /* fall through */
        case 1:
            if ( (sLibStage != 2) &&
                 (sOriProcInfo.planTree != NULL) )
            {
                if ( sOriProcInfo.planTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sOriProcInfo.planTree->expCallSpec;
                    (void)qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                                   &(sCallSpec->functionPtr),
                                                   sCallSpec->fileSpec,
                                                   sCallSpec->functionName );
                }
            }
            break;
        default:
            break;
    }

    switch( sStage)
    {
        case 3:
            if( ( sProcInfo->cache != sOriProcInfo.cache ) &&
                ( sProcInfo->cache != NULL ) )
            {
                (void)qsx::freeTemplateCache( sProcInfo );
            }
            idlOS::memcpy( sProcInfo, &sOriProcInfo, ID_SIZEOF(qsxProcInfo) );
            /* fall through */
        case 2:
            if ( qsxProc::unlatchForStatus( sProcOID ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1:
            if ( qsxProc::unlatch( sProcOID ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_TRUE );

    return IDE_FAILURE;
}

IDE_RC qsx::dropProcOrFunc(qcStatement * aStatement)
{
    qsDropParseTree * sPlanTree;
    qsOID             sProcOID;
    UInt              sUserID;
    SChar             sProcName[QC_MAX_OBJECT_NAME_LEN + 1];

    sPlanTree = (qsDropParseTree *)(aStatement->myPlan->parseTree);
    sProcOID  = sPlanTree->procOID;

    IDE_TEST( qcmUser::getUserID( aStatement,
                                  sPlanTree->userNamePos,
                                  &sUserID ) != IDE_SUCCESS );

    IDE_TEST( dropProcOrFuncCommon ( aStatement,
                                     sProcOID,
                                     sUserID,
                                     sPlanTree->procNamePos.stmtText +
                                     sPlanTree->procNamePos.offset,
                                     sPlanTree->procNamePos.size )
              != IDE_SUCCESS );

    // BUG-36693 procName을 parse tree에서 참조한다.
    QC_STR_COPY( sProcName, sPlanTree->procNamePos );

    // PROJ-2223 audit
    IDE_TEST( qcmAudit::deleteObject( aStatement,
                                      sUserID,
                                      sProcName )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::doRecompile(
    qcStatement    * aStatement,
    qsOID            aProcOID,
    qsxProcInfo    * aProcInfo,
    idBool           aIsUseTx )
{
    SChar         * sProcStmtText = NULL;
    SInt            sProcStmtTextLen;
    UInt            sProcUserID;

    qcStatement     sStatement;
    smiStatement  * sSmiStmt;
    iduVarMemList * sQmsMem = NULL;
    UInt            sStage  = 0;
    UInt            sLibStage = 0;

    qsxProcInfo     sOriProcInfo;
    idBool          sIsStmtAlloc = ID_FALSE;

    qsCallSpec    * sCallSpec;

    sQmsMem         = aProcInfo->qmsMem;

    idlOS::memcpy( &sOriProcInfo,
                   aProcInfo,
                   ID_SIZEOF(qsxProcInfo) );
    aProcInfo->cache = NULL;
    sStage = 1;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   aStatement->mStatistics )
              != IDE_SUCCESS );
    sIsStmtAlloc = ID_TRUE;

    // BUG-38800 Server startup시 Function-Based Index에서 사용 중인 Function을
    // recompile 할 수 있어야 한다.
    if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_LOAD_PROC_MASK )
         == QC_SESSION_INTERNAL_LOAD_PROC_TRUE )
    {
        sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_INTERNAL_LOAD_PROC_MASK;
        sStatement.session->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_LOAD_PROC_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    /* TASK-7307 DML Data Consistency in Shard
     *   CLONE 테이블은 GLOBAL_TRANSACTION_LEVEL에 따라 접근여부가 결정되므로 이 값이 필요함 */
    sStatement.session->mQPSpecific.mIsGCTx = aStatement->session->mQPSpecific.mIsGCTx;

    IDE_TEST( qsxProc::getProcText( &sStatement,
                                    aProcOID,
                                    &sProcStmtText,
                                    &sProcStmtTextLen )
              != IDE_SUCCESS );

    qcg::getSmiStmt( aStatement, &sSmiStmt );
    qcg::setSmiStmt( &sStatement, sSmiStmt );

    //To Fix BUG-19839 : ProcedureOID를 사용하여 소유자의 UserID를 얻는다.
    IDE_TEST( qcmProc::getProcUserID( &sStatement,
                                      aProcOID,
                                      &sProcUserID )
              != IDE_SUCCESS );

    /* BUG-37093 */
    sStatement.session->mMmSessionForDatabaseLink =
        aStatement->session->mMmSession;
        
    QCG_SET_SESSION_USER_ID( &sStatement, sProcUserID );

    qsxEnv::initialize(sStatement.spxEnv, NULL);

    sStatement.myPlan->stmtText    = sProcStmtText;
    sStatement.myPlan->stmtTextLen = sProcStmtTextLen;

    IDE_TEST( qcpManager::parseIt( &sStatement ) != IDE_SUCCESS );

    if( sStatement.myPlan->parseTree->validate == qsv::validateCreateProc )
    {
        sStatement.myPlan->parseTree->validate = qsv::validateReplaceProcForRecompile;
    }
    else
    {
        if( sStatement.myPlan->parseTree->validate == qsv::validateCreateFunc )
        {
            sStatement.myPlan->parseTree->validate = qsv::validateReplaceFunc;
        }
        else
        {
            /* Do Nothing */
        }
    }

    /* BUG-48290 */
    if( sStatement.myPlan->parseTree->validate == qsv::validateReplaceProc )
    {
        sStatement.myPlan->parseTree->validate = qsv::validateReplaceProcForRecompile;
    }

    IDE_TEST( qsxProc::makeStatusInvalidForRecompile( aStatement,
                                                      aProcOID )
              != IDE_SUCCESS );

    IDE_TEST( sStatement.myPlan->parseTree->parse( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( qcg::fixAfterParsing( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( sStatement.myPlan->parseTree->validate( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( qcg::fixAfterValidation( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( sStatement.myPlan->parseTree->optimize( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( qcg::fixAfterOptimization( &sStatement ) != IDE_SUCCESS );

    IDE_TEST( qsx::makeProcInfoMembers( aProcInfo, &sStatement )
              != IDE_SUCCESS );

    sQmsMem = QC_QMP_MEM( &sStatement );
    IDE_TEST( qsx::makeNewPreparedMem( &sStatement ) != IDE_SUCCESS );

    aProcInfo->qmsMem  = sQmsMem;
    aProcInfo->isValid = ID_TRUE;
    aProcInfo->modifyCount++;
    sStage = 0;

    // Server startup시에는 plan tree가 NULL이다.
    if ( sOriProcInfo.planTree != NULL )
    {
        if ( sOriProcInfo.planTree->procType == QS_INTERNAL_C )
        {
            sCallSpec = sOriProcInfo.planTree->expCallSpec;

            IDE_ERROR( sCallSpec->libraryNode != NULL );

            IDE_TEST( qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                 &(sCallSpec->functionPtr) )
                      != IDE_SUCCESS );
            sLibStage = 1;

            IDE_ERROR( sCallSpec->functionPtr == NULL );
        }
    }

    if ( aProcInfo->planTree->procType == QS_INTERNAL_C )
    {
        sCallSpec = aProcInfo->planTree->expCallSpec;

        IDE_TEST( qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                           &(sCallSpec->functionPtr),
                                           sCallSpec->fileSpec,
                                           sCallSpec->functionName )
                  != IDE_SUCCESS );
        sLibStage += 2;

        IDE_ERROR( sCallSpec->libraryNode != NULL );

        //   Library를 load하지 못했거나, function을 제대로 찾지 못한경우
        //   invalid 상태로 만들어 다음에 rebuild하면서
        //   library를 load하거나 function을 찾도록 한다.
        if ( (sCallSpec->libraryNode->mHandle == PDL_SHLIB_INVALID_HANDLE) ||
             (sCallSpec->libraryNode->mFunctionPtr == NULL) ||
             (sCallSpec->functionPtr == NULL) )
        {
            aProcInfo->isValid = ID_FALSE;
        }
    }
    else if ( aProcInfo->planTree->procType == QS_EXTERNAL_C )
    {
        sCallSpec = aProcInfo->planTree->expCallSpec;
        sCallSpec->libraryNode  = NULL;
        sCallSpec->functionPtr = NULL;
    }
    else
    {
        // Nothing to do.
    }

    // BUG-37382
    IDE_TEST( qsx::freeTemplateCache( &sOriProcInfo ) != IDE_SUCCESS );

    if( sOriProcInfo.qmsMem != NULL )
    {
        IDE_TEST( sOriProcInfo.qmsMem->destroy() != IDE_SUCCESS );

        IDE_TEST( qcg::freeIduVarMemList( sOriProcInfo.qmsMem ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( aProcInfo->isValid == ID_TRUE )
    {
        if( aIsUseTx == ID_TRUE )
        {
            IDE_TEST( qsxProc::makeStatusValidTx( aStatement,
                                                  aProcOID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qsxProc::makeStatusValid( aStatement,
                                                aProcOID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        if( aIsUseTx == ID_TRUE )
        {
            IDE_TEST( qsxProc::makeStatusInvalidTx( aStatement,
                                                    aProcOID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qsxProc::makeStatusInvalid( aStatement,
                                                  aProcOID )
                      != IDE_SUCCESS );
        }
    }

    sIsStmtAlloc = ID_FALSE;
    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sLibStage )
    {
        case 3:
        case 2:
            if ( aProcInfo->planTree->procType == QS_INTERNAL_C )
            {
                sCallSpec = aProcInfo->planTree->expCallSpec;
                (void)qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                 &(sCallSpec->functionPtr) );
            }
            /* fall through */
        case 1:
            if ( (sOriProcInfo.planTree != NULL) &&
                 (sLibStage != 2) )
            {
                if ( sOriProcInfo.planTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sOriProcInfo.planTree->expCallSpec;
                    (void)qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                                   &(sCallSpec->functionPtr),
                                                   sCallSpec->fileSpec,
                                                   sCallSpec->functionName );
                }
            }
            break;
        default:
            break;
    }

    switch( sStage )
    {
        case 1:
            if( ( aProcInfo->cache != sOriProcInfo.cache ) &&
                ( aProcInfo->cache != NULL ) )
            {
                (void)qsx::freeTemplateCache( aProcInfo );
            }
            idlOS::memcpy( aProcInfo, &sOriProcInfo, ID_SIZEOF(qsxProcInfo) );
            break;
        default:
            break;
    }

    if( sIsStmtAlloc == ID_TRUE )
    {
        if( sStatement.spvEnv->latched == ID_TRUE )
        {
            if ( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                 != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            else
            {
                // Nothing to do.
            }

            sStatement.spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }

        (void) qcg::freeStatement(&sStatement);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsx::dropProcOrFuncCommon(qcStatement * aStatement,
                                 qsOID         aProcOID,
                                 UInt          aUserID,
                                 SChar       * aProcName,
                                 UInt          aProcNameSize )
{
#define IDE_FN "qsx::dropProcOrFunc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcInfo * sProcInfo;
    qsCallSpec  * sCallSpec;
    UInt          sStage = 0;

    IDE_TEST( qsxProc::latchX( aProcOID,
                               ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( qsxProc::latchXForStatus( aProcOID )
              != IDE_SUCCESS );
    sStage = 2;

    /* BUG-48290 shard object에 대한 DDL 차단 */
    // BUG-48919 Execute 시점에서 shard object 여부를 확인해야 한다.
    if ( SDU_SHARD_ENABLE == 1 )
    {
        IDE_TEST( sdi::checkShardObjectForDDL( aStatement, SDI_DDL_TYPE_PROCEDURE_DROP ) != IDE_SUCCESS );
    }

    IDE_TEST( qsxProc::getProcInfo( aProcOID,
                                    &sProcInfo ) != IDE_SUCCESS );

    // Drop object should be done at first time
    // to ensure that there's no more transactions accessing to this one.
    IDE_TEST( smiObject::dropObject( QC_SMI_STMT(aStatement),
                                     smiGetTable( aProcOID ) )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::remove( aStatement,
                               aProcOID,
                               aUserID,
                               aProcName,
                               aProcNameSize,
                               ID_FALSE /* aPreservePrivInfo */ )
              != IDE_SUCCESS );

    if ( sProcInfo->planTree != NULL )
    {
        if ( sProcInfo->planTree->procType == QS_INTERNAL_C )
        {
            sCallSpec = sProcInfo->planTree->expCallSpec;

            IDE_ERROR( sCallSpec->libraryNode != NULL );

            IDE_TEST( qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                 &(sCallSpec->functionPtr) )
                      != IDE_SUCCESS );

            IDE_ERROR( sCallSpec->functionPtr == NULL );
        }
    }

    IDE_TEST( qsxProc::destroyProcInfo( &sProcInfo ) != IDE_SUCCESS );

    IDE_TEST( qsxProc::disableProcObjectInfo( aProcOID )
              != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST( qsxProc::unlatchForStatus( aProcOID ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( qsxProc::unlatch( aProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 2:
            if ( qsxProc::unlatchForStatus( aProcOID ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1:
            if ( qsxProc::unlatch( aProcOID ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsx::alterProcOrFunc(qcStatement * aStatement)
{
    qsAlterParseTree * sPlanTree;
    qsxProcInfo      * sProcInfo = NULL;  // TASK-3876 Code Sonar
    qsOID              sProcOID;
    UInt               sUserID;
    UInt               sStage = 0;

    sPlanTree = (qsAlterParseTree *)(aStatement->myPlan->parseTree);

    sProcOID  = sPlanTree->procOID;

    IDE_TEST( qsxProc::latchX( sProcOID,
                               ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( qsxProc::getProcInfo( sProcOID,
                                    &sProcInfo )
              != IDE_SUCCESS );

    // BUG-38627
    if ( sProcInfo->isValid == ID_FALSE )
    {
        // proj-1535
        // vaild 상태의 PSM도 recompile을 허용한다.
        // refereneced PSM을 invalid로 변경한다.
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      sPlanTree->userNamePos,
                                      &sUserID ) != IDE_SUCCESS );

        // fix BUG-18704
        qcg::setPlanTreeState( aStatement, ID_FALSE );
        sStage = 2;

        IDE_TEST( qsx::doRecompile( aStatement,
                                    sProcOID,
                                    sProcInfo,
                                    ID_FALSE )
                  != IDE_SUCCESS );

        sStage = 1;
        // fix BUG-18704
        qcg::setPlanTreeState( aStatement, ID_TRUE );
    }
    else
    {
        // Nothing to do.
    }

    sStage = 0;
    IDE_TEST( qsxProc::unlatch( sProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 2:
            // fix BUG-18704
            qcg::setPlanTreeState( aStatement, ID_TRUE );
            /* fall through */
        case 1:
            if ( qsxProc::unlatch( sProcOID ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsx::executeProcOrFunc(qcStatement * aStatement)
{
    qsExecParseTree  * sPlanTree;
    qsxProcPlanList  * sFoundProcPlan;
    qsProcParseTree  * sProcPlanTree;
    qsxPkgInfo       * sPkgInfo;
    qcTemplate       * sPkgTemplate = NULL;
    qcuSqlSourceInfo   sqlInfo;

    sPlanTree = (qsExecParseTree *)(aStatement->myPlan->parseTree);

    // exec pkg1.proc1
    if( sPlanTree->subprogramID != QS_PSM_SUBPROGRAM_ID )
    {
        if ( sPlanTree->pkgBodyOID != QS_EMPTY_OID )
        {
            IDE_TEST( qsxPkg::getPkgInfo( sPlanTree->pkgBodyOID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );

            IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( 
                          aStatement,
                          sPkgInfo,
                          QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack,
                          QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain,
                          &sPkgTemplate )
                      != IDE_SUCCESS );

            /* BUG-38844 package subprogram의 plan tree를 찾음 */
            IDE_TEST( qsxPkg::findSubprogramPlanTree(
                          sPkgInfo,
                          sPlanTree->subprogramID,
                          &sProcPlanTree )
                      != IDE_SUCCESS );

            sProcPlanTree->pkgBodyOID = sPlanTree->pkgBodyOID;
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sPlanTree->callSpecNode->tableName );
            IDE_RAISE( ERR_NOT_EXIST_PKG_BODY_NAME );
        }
    }
    else  // exec proc1;
    {
        IDE_TEST( qsxRelatedProc::findPlanTree( aStatement,
                                                sPlanTree->procOID,
                                                &sFoundProcPlan )
                  != IDE_SUCCESS );

        sProcPlanTree = (qsProcParseTree *)(sFoundProcPlan->planTree);
    }

    // fix BUG-12553
    IDE_TEST( qtc::setDatePseudoColumn( QC_PRIVATE_TMPLATE( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( callProcWithNode( aStatement,
                                sProcPlanTree,
                                sPlanTree->callSpecNode,
                                sPkgTemplate, 
                                NULL )
              != IDE_SUCCESS );

    if ( sPlanTree->leftNode != NULL )
    {
        IDE_TEST( qsxUtil::assignValue( QC_QMX_MEM( aStatement ),
                                        QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack,
                                        QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain,
                                        sPlanTree->leftNode,
                                        QC_PRIVATE_TMPLATE(aStatement),
                                        ID_FALSE ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_PKG_BODY_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_PKG_BODY,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::callProcWithNode (
    qcStatement            * aStatement,
    qsProcParseTree        * aPlanTree,
    qtcNode                * aProcCallSpecNode,
    qcTemplate             * aPkgTemplate,
    qcTemplate             * aTmplate )
{
#define IDE_FN "qsx::callProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsVariableItems  * sParaDecls;
    qtcNode          * sArgument;
    mtcTemplate      * sTmplate;
    mtcStack         * sOriStack = NULL;
    SInt               sOriStackRemain = 0;
    mtcStack         * sNewProcStack;
    SInt               sNewProcStackRemain;
    UInt               sStage = 0;
    iduMemoryStatus    sQmxMemStatus;
    qmcCursor        * sOriCursorMgr = NULL;
    qmcdTempTableMgr * sOriTempTableMgr = NULL;

    sTmplate = & QC_PRIVATE_TMPLATE(aStatement)->tmplate;

    IDE_TEST( QC_QMX_MEM(aStatement)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage = 1;

    sOriStack       = sTmplate->stack;
    sOriStackRemain = sTmplate->stackRemain;
    sStage = 2;

    // set return value pointer.
    if( aPlanTree->objType == QS_FUNC )
    {
        // BUG-33674
        IDE_TEST_RAISE( sTmplate->stackRemain < 1,
                        err_stack_overflow );

        sTmplate->stack->column = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aStatement),
                                                   aProcCallSpecNode );
        sTmplate->stack->value  = QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aStatement),
                                                      aProcCallSpecNode );
    }

    // skip stack[0] for return value.
    sTmplate->stackRemain--;
    sTmplate->stack++;

    IDE_TEST_RAISE( sTmplate->stackRemain < 1,
                    err_stack_overflow );

    // exchange cursorMgr to get the opened cursors while evaluating
    // arguments. ( eg. subquery on argument )
    sOriCursorMgr    = ((qcTemplate*) sTmplate)->cursorMgr;
    sOriTempTableMgr = ((qcTemplate*) sTmplate)->tempTableMgr;

    sStage = 3;

    for ( sParaDecls = aPlanTree->paraDecls,
          sArgument = (qtcNode * )aProcCallSpecNode->node.arguments ;
          sParaDecls != NULL && sArgument != NULL ;
          sParaDecls = sParaDecls->next,
          sArgument = (qtcNode * )sArgument->node.next,
          sTmplate->stack ++,
          sTmplate->stackRemain-- )
    {
        IDE_TEST_RAISE( sTmplate->stackRemain < 1,
                        err_stack_overflow );

        IDE_TEST( qtc::calculate( sArgument,
                                  (qcTemplate *) sTmplate )
                  != IDE_SUCCESS );
    }

    ((qcTemplate *) sTmplate)->cursorMgr    = sOriCursorMgr ;
    ((qcTemplate *) sTmplate)->tempTableMgr = sOriTempTableMgr ;

    sStage = 2;

    sNewProcStack       = sTmplate->stack;
    sNewProcStackRemain = sTmplate->stackRemain;

    sTmplate->stack       = sOriStack;
    sTmplate->stackRemain = sOriStackRemain;
    sStage = 1;

    IDE_TEST( callProcWithStack( aStatement,
                                 aPlanTree,
                                 sNewProcStack,
                                 sNewProcStackRemain,
                                 aPkgTemplate,
                                 aTmplate )
              != IDE_SUCCESS );

    sStage=0;
    IDE_TEST( QC_QMX_MEM(aStatement)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_stack_overflow);
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_STACK_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 3:
            ((qcTemplate *) sTmplate)->cursorMgr    = sOriCursorMgr;
            ((qcTemplate *) sTmplate)->tempTableMgr = sOriTempTableMgr;
        case 2:
            sTmplate->stack       = sOriStack;
            sTmplate->stackRemain = sOriStackRemain;
        case 1:
            if ( QC_QMX_MEM(aStatement)->setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }
    sStage = 0;

    return IDE_FAILURE;

#undef IDE_FN
}

/*
   PROCEDURE
   input stack[1] ~ stack[n+1]
   unused stack[0]

   FUNCTION
   input stack[1] ~ stack[n+1]
   output stack[0]
*/

IDE_RC qsx::callProcWithStack (
    qcStatement            * aStatement,
    qsProcParseTree        * aPlanTree,
    mtcStack               * aStack,
    SInt                     aStackRemain,
    qcTemplate             * aPkgTemplate,
    qcTemplate             * aTmplate )
{
    qsxExecutorInfo sExecInfo;

    // PROJ-1073 Package
    sExecInfo.mSourceTemplateStackBuffer = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackBuffer;
    sExecInfo.mSourceTemplateStack       = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack;
    sExecInfo.mSourceTemplateStackCount  = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackCount;
    sExecInfo.mSourceTemplateStackRemain = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain;
    sExecInfo.mCurrentUserID             = QCG_GET_SESSION_USER_ID( aStatement );

    IDE_TEST( qsxExecutor::initialize( &sExecInfo,
                                       aPlanTree,
                                       NULL,
                                       aPkgTemplate,
                                       aTmplate ) != IDE_SUCCESS );

    /* BUG-38509 autonomous transaction */
    if ( QSX_CHECK_AT_PSM_BLOCK( QC_QSX_ENV(aStatement),
                                 aPlanTree )
         == ID_TRUE )
    {
        IDE_TEST( execAT( &sExecInfo,
                          aStatement,
                          aStack,
                          aStackRemain )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qsxExecutor::execPlan( &sExecInfo,
                                         aStatement,
                                         aStack,
                                         aStackRemain )
                  != IDE_SUCCESS );
    }

    // PROJ-1073 Package
    /* QC_PRIVATE_TMPLATE(aStatement)->
             template.stackBuffer/stack/stackCount/stackRemain 원복 */
    QC_CONNECT_TEMPLATE_STACK(    
        QC_PRIVATE_TMPLATE(aStatement),
        sExecInfo.mSourceTemplateStackBuffer,
        sExecInfo.mSourceTemplateStack,
        sExecInfo.mSourceTemplateStackCount,
        sExecInfo.mSourceTemplateStackRemain );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // PROJ-1073 Package
    /* QC_PRIVATE_TMPLATE(aStatement)->
       template.stackBuffer/stack/stackCount/stackRemain 원복 */
    QC_CONNECT_TEMPLATE_STACK(
        QC_PRIVATE_TMPLATE(aStatement),
        sExecInfo.mSourceTemplateStackBuffer,
        sExecInfo.mSourceTemplateStack,
        sExecInfo.mSourceTemplateStackCount,
        sExecInfo.mSourceTemplateStackRemain );

    return IDE_FAILURE;
}


IDE_RC qsx::doJobForEachProc (
    smiStatement    * aSmiStmt,
    iduMemory       * aIduMem,
    idBool            aIsUseTx,
    qsxJobForProc     aJobFunc)
{
#define IDE_FN "qsx::DoJobForEachProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    vSLong           sProcCount;
    vSLong           sSelectedProcCount;
    qcmProcedures  * sQcmProcedures;

    UInt             i;

    IDE_TEST( qcmProc::procSelectCount( aSmiStmt, &sProcCount )
              != IDE_SUCCESS );

    if (sProcCount > 0)
    {
        IDU_FIT_POINT( "qsx::doJobForEachProc::cralloc::sQcmProcedures",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aIduMem->cralloc(
                idlOS::align8((UInt)ID_SIZEOF( qcmProcedures )) * sProcCount,
                (void**)&sQcmProcedures)
            != IDE_SUCCESS);

        IDE_TEST( qcmProc::procSelectAll(
                aSmiStmt,
                sProcCount,
                &sSelectedProcCount,
                sQcmProcedures )
            != IDE_SUCCESS );

        IDE_TEST_RAISE( sProcCount != sSelectedProcCount,
                        err_proc_count_mismatch );

        for ( i = 0; i < (UInt) sProcCount; i++ )
        {
            IDE_TEST( (*aJobFunc) ( aSmiStmt, &sQcmProcedures[ i ], aIsUseTx )
                      != IDE_SUCCESS );

        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_proc_count_mismatch);
    {
        IDE_SET(ideSetErrorCode(
                qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                "[qsx::doJobForEachProc] err_proc_count_mismatch"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxLoadProc( smiStatement  * aSmiStmt,
                    qcmProcedures * aProc,
                    idBool          aIsUseTx )
{
    /* PROJ-2446 ONE SOURCE */
    return qsx::qsxLoadProcByProcOID( aSmiStmt,
                                      aProc->procOID,
                                      ID_FALSE,    // aCreateProcInfo
                                      aIsUseTx );
}

IDE_RC qsx::qsxLoadProcByProcOID( smiStatement   * aSmiStmt,
                                  smOID            aProcOID,
                                  idBool           aCreateProcInfo,
                                  idBool           aIsUseTx )
{
    qcStatement         sStatement;
    qsxProcObjectInfo * sProcObjectInfo;
    qsxProcInfo       * sProcInfo;
    UInt                sStage = 0;

    // make qcStatement : alloc the members of qcStatement
    // 실제 Stack Count는 수행 시점에 Template 복사에 의하여
    // Session 정보로부터 결정된다.
    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL ) != IDE_SUCCESS );

    qsxEnv::initialize(sStatement.spxEnv, NULL);
    sStage = 1;

    sStatement.mStatistics = aSmiStmt->mStatistics;

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    if( aCreateProcInfo == ID_FALSE )
    {
        IDE_TEST( qsxProc::getProcObjectInfo( aProcOID,
                                              &sProcObjectInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        sProcObjectInfo = NULL;
    }

    // proj-1535
    // 이미 생성된 PSM은 skip
    if ( sProcObjectInfo == NULL )
    {
        IDE_TEST( qsxProc::createProcObjectInfo( aProcOID, &sProcObjectInfo )
                  != IDE_SUCCESS );
        sStage   = 2;

        IDE_TEST( qsxProc::setProcObjectInfo(  aProcOID,
                                               sProcObjectInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qsxProc::createProcInfo( aProcOID, &sProcInfo )
                  != IDE_SUCCESS );
        sStage   = 3;

        IDE_TEST( qsxProc::setProcInfo( aProcOID,
                                        sProcInfo )
                  != IDE_SUCCESS );

        if ( aIsUseTx == ID_TRUE )
        {
            IDE_TEST( qsxProc::makeStatusInvalidTx( &sStatement,
                                                    aProcOID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qsxProc::makeStatusInvalid( &sStatement,
                                                  aProcOID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    sStage = 0;
    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 3:
            if ( qsxProc::destroyProcInfo( &sProcInfo ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 2:
            if ( qsxProc::destroyProcObjectInfo( &sProcObjectInfo )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 1:
            (void) qcg::freeStatement(&sStatement);
    }

    return IDE_FAILURE;
}

IDE_RC qsxRecompileProc( smiStatement   * aSmiStmt,
                         qcmProcedures  * aProc,
                         idBool           aIsUseTx )
{
#define IDE_FN "IDE_RC qsxRecompileProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement       sStatement;
    qsxProcInfo     * sProcInfo;
    qsOID             sProcOID;
    UInt              sStage = 0;

    sProcOID = aProc->procOID;

    // make qcStatement : alloc the members of qcStatement
    // 실제 Stack Count는 수행 시점에 Template 복사에 의하여
    // Session 정보로부터 결정된다.
    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL ) != IDE_SUCCESS );

    // BUG-26017
    // [PSM] server restart시 수행되는
    // psm load과정에서 관련프로퍼티를 참조하지 못하는 경우 있음.
    sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_INTERNAL_LOAD_PROC_MASK;
    sStatement.session->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_LOAD_PROC_TRUE;   

    qsxEnv::initialize(sStatement.spxEnv, NULL);
    sStage = 1;

    sStatement.mStatistics = aSmiStmt->mStatistics;

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    IDE_TEST( qsxProc::getProcInfo( sProcOID,
                                    &sProcInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcInfo != NULL );

    // BUG-13707 isValid가 ID_FALSE 인 경우에 doRecompile 한다.
    if (sProcInfo->isValid != ID_TRUE )
    {
        // server start 중에는 doRecompile에 실패할 수 있다.
        if( qsx::doRecompile( &sStatement,
                              sProcOID,
                              sProcInfo,
                              aIsUseTx )
            != IDE_SUCCESS )
        {
            IDE_TEST( qsxProc::makeStatusInvalidTx( &sStatement,
                                                    sProcOID )
                      != IDE_SUCCESS );
        }
    }

    sStage = 0;
    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 1:
            (void) qcg::freeStatement(&sStatement);
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

// BUG-45571 Upgrade meta for authid clause of PSM
IDE_RC qsx::loadAllProcForMetaUpgrade(
    smiStatement    * aSmiStmt,
    iduMemory       * aIduMem)
{
    IDE_TEST( doJobForEachProc ( aSmiStmt,
                                 aIduMem,
                                 ID_FALSE, // aIsUseTx
                                 qsxLoadProc )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::loadAllProc(
    smiStatement    * aSmiStmt,
    iduMemory       * aIduMem)
{
#define IDE_FN "IDE_RC qsx::loadAllProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( doJobForEachProc ( aSmiStmt,
                                 aIduMem,
                                 ID_TRUE, // aIsUseTx
                                 qsxLoadProc )
              != IDE_SUCCESS );

    // BUG-24338
    // _STORE_PROC_MODE가 1(startup시 compile)이면 recompile을 시도한다.
    if ( ! ( QCU_STORED_PROC_MODE & QCU_SPM_MASK_DISABLE ) )
    {
        IDE_TEST( doJobForEachProc ( aSmiStmt,
                                     aIduMem,
                                     ID_TRUE, // aIsUseTx
                                     qsxRecompileProc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxUnloadProc( smiStatement   * /* aSmiStmt */,
                      qcmProcedures  * aProc,
                      idBool           /* aIsUseTx */ )
{
#define IDE_FN "IDE_RC qsxUnloadProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcInfo     * sProcInfo;
    qsOID             sProcOID;
    UInt              sStage = 0;

    sProcOID = aProc->procOID;

    IDE_TEST( qsxProc::latchX( sProcOID,
                               ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( qsxProc::latchXForStatus( sProcOID )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( qsxProc::getProcInfo( sProcOID, &sProcInfo ) != IDE_SUCCESS );

    IDE_TEST( qsxProc::destroyProcInfo( &sProcInfo ) != IDE_SUCCESS );

    IDE_TEST( qsxProc::disableProcObjectInfo( sProcOID ) != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST( qsxProc::unlatchForStatus( sProcOID )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( qsxProc::unlatch( sProcOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 2:
            if ( qsxProc::unlatchForStatus( sProcOID )
                != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1:
            if ( qsxProc::unlatch( sProcOID )
                 != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsx::unloadAllProc(
    smiStatement    * aSmiStmt,
    iduMemory       * aIduMem)
{
#define IDE_FN "IDE_RC qsx::unloadAllProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( doJobForEachProc ( aSmiStmt,
                                 aIduMem,
                                 ID_TRUE,  // aIsUseTx, 사용안함.
                                 qsxUnloadProc )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// PROJ-1073 Package
IDE_RC qsx::doPkgRecompile( qcStatement   * aStatement,
                            qsOID           aPkgOID,
                            qsxPkgInfo    * aPkgInfo,
                            idBool          aIsUseTx )

{
    SChar         * sPkgStmtText = NULL;
    SInt            sPkgStmtTextLen;
    UInt            sPkgUserID;

    qcStatement     sStatement;
    smiStatement  * sSmiStmt;
    iduVarMemList * sQmsMem = NULL;
    UInt            sStage  = 0;

    qsxPkgInfo      sOriPkgInfo;
    idBool          sIsStmtAlloc = ID_FALSE;

    qsPkgStmts        * sPkgStmt = NULL;
    qsPkgSubprograms  * sSubprogram;
    qsProcParseTree   * sSubprogramPlanTree = NULL;
    qsCallSpec        * sCallSpec;

    UInt                sLibraryUnloadCount = 0;
    UInt                sLibraryLoadCount = 0;

    idlOS::memcpy( &sOriPkgInfo,
                   aPkgInfo,
                   ID_SIZEOF(qsxPkgInfo) );
    sStage = 1;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   aStatement->mStatistics ) != IDE_SUCCESS );
    sIsStmtAlloc = ID_TRUE;

    // BUG-38800 Server startup시 Function-Based Index에서 사용 중인 Function을
    // recompile 할 수 있어야 한다.
    if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_LOAD_PROC_MASK )
         == QC_SESSION_INTERNAL_LOAD_PROC_TRUE )
    {
        sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_INTERNAL_LOAD_PROC_MASK;
        sStatement.session->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_LOAD_PROC_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    /* TASK-7307 DML Data Consistency in Shard
     *   CLONE 테이블은 GLOBAL_TRANSACTION_LEVEL에 따라 접근여부가 결정되므로 이 값이 필요함 */
    sStatement.session->mQPSpecific.mIsGCTx = aStatement->session->mQPSpecific.mIsGCTx;

    IDE_TEST( qsxPkg::getPkgText( &sStatement,
                                  aPkgOID,
                                  &sPkgStmtText,
                                  &sPkgStmtTextLen ) != IDE_SUCCESS );

    qcg::getSmiStmt( aStatement, &sSmiStmt );
    qcg::setSmiStmt( &sStatement, sSmiStmt );

    //To Fix BUG-19839 : PackageOID를 사용하여 소유자의 UserID를 얻는다.
    IDE_TEST( qcmPkg::getPkgUserID( &sStatement,
                                    aPkgOID,
                                    &sPkgUserID )
              != IDE_SUCCESS );

    /* BUG-37093 */
    sStatement.session->mMmSessionForDatabaseLink =
        aStatement->session->mMmSession;
    
    QCG_SET_SESSION_USER_ID( &sStatement, sPkgUserID );

    qsxEnv::initialize(sStatement.spxEnv, NULL);

    sStatement.myPlan->stmtText    = sPkgStmtText;
    sStatement.myPlan->stmtTextLen = sPkgStmtTextLen;

    IDE_TEST( qcg::allocSharedTemplate( &sStatement,
                                        QCG_GET_SESSION_STACK_SIZE( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( qcpManager::parseIt( &sStatement ) != IDE_SUCCESS );

    if( sStatement.myPlan->parseTree->validate == qsv::validateCreatePkg )
    {
        sStatement.myPlan->parseTree->validate = qsv::validateReplacePkg;
    }
    else
    {
        if( sStatement.myPlan->parseTree->validate == qsv::validateCreatePkgBody )
        {
            sStatement.myPlan->parseTree->validate = qsv::validateReplacePkgBody;
        }
        else
        {
            /* Do Nothing */
        }
    }

    IDE_TEST( qsxPkg::makeStatusInvalidForRecompile( aStatement,
                                                     aPkgOID )
              != IDE_SUCCESS );

    IDE_TEST( sStatement.myPlan->parseTree->parse( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( qcg::fixAfterParsing( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( sStatement.myPlan->parseTree->validate( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( qcg::fixAfterValidation( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( sStatement.myPlan->parseTree->optimize( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( qcg::fixAfterOptimization( &sStatement ) != IDE_SUCCESS );

    IDE_TEST( qsx::makePkgInfoMembers ( aPkgInfo,
                                        &sStatement )
              != IDE_SUCCESS );

    sQmsMem = QC_QMP_MEM( &sStatement );
    IDE_TEST( qsx::makeNewPreparedMem( &sStatement ) != IDE_SUCCESS );

    aPkgInfo->qmsMem  = sQmsMem;
    aPkgInfo->isValid = ID_TRUE;
    aPkgInfo->modifyCount++;
    sStage = 0;

    if ( aPkgInfo->objType == QS_PKG_BODY )
    {
        if ( sOriPkgInfo.planTree != NULL )
        {
            for ( sPkgStmt = sOriPkgInfo.planTree->block->subprograms;
                  sPkgStmt != NULL;
                  sPkgStmt = sPkgStmt->next )
            {
                if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
                {
                    sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                    sSubprogramPlanTree = sSubprogram->parseTree;

                    if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                    {
                        sCallSpec = sSubprogramPlanTree->expCallSpec;

                        IDE_TEST( qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                             &(sCallSpec->functionPtr) )
                                  != IDE_SUCCESS );
                        sLibraryUnloadCount++;
                    }
                }
            }
        }

        for ( sPkgStmt = aPkgInfo->planTree->block->subprograms;
              sPkgStmt != NULL;
              sPkgStmt = sPkgStmt->next )
        {
            if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
            {
                sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                sSubprogramPlanTree = sSubprogram->parseTree;

                if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sSubprogramPlanTree->expCallSpec;

                    IDE_TEST( qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                                       &(sCallSpec->functionPtr),
                                                       sCallSpec->fileSpec,
                                                       sCallSpec->functionName )
                              != IDE_SUCCESS );

                    IDE_ERROR( sCallSpec->libraryNode != NULL );
                    sLibraryLoadCount++;

                    //   Library를 load하지 못했거나, function을 제대로 찾지 못한경우
                    //   invalid 상태로 만들어 다음에 rebuild하면서
                    //   library를 load하거나 function을 찾도록 한다.
                    if ( (sCallSpec->libraryNode->mHandle == PDL_SHLIB_INVALID_HANDLE) ||
                         (sCallSpec->libraryNode->mFunctionPtr == NULL) ||
                         (sCallSpec->functionPtr == NULL) )
                    {
                        aPkgInfo->isValid = ID_FALSE;
                    }
                }
                else if ( sSubprogramPlanTree->procType == QS_EXTERNAL_C )
                {
                    sCallSpec = sSubprogramPlanTree->expCallSpec;

                    sCallSpec->libraryNode = NULL;
                    sCallSpec->functionPtr = NULL;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }

    if( sOriPkgInfo.qmsMem != NULL )
    {
        IDE_TEST( sOriPkgInfo.qmsMem->destroy() != IDE_SUCCESS );

        IDE_TEST( qcg::freeIduVarMemList( sOriPkgInfo.qmsMem) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( aIsUseTx == ID_TRUE)
    {
        if ( aPkgInfo->isValid == ID_TRUE )
        {
            IDE_TEST( qsxPkg::makeStatusValidTx( aStatement,
                                                 aPkgOID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qsxPkg::makeStatusInvalidTx( aStatement,
                                                   aPkgOID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        if ( aPkgInfo->isValid == ID_TRUE )
        {
            IDE_TEST( qsxPkg::makeStatusValid( aStatement,
                                               aPkgOID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qsxPkg::makeStatusInvalid( aStatement,
                                                 aPkgOID )
                      != IDE_SUCCESS );
        }
    }

    sIsStmtAlloc = ID_FALSE;
    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLibraryLoadCount > 0 )
    {
        for ( sPkgStmt = aPkgInfo->planTree->block->subprograms;
              ( (sPkgStmt != NULL) && (sLibraryLoadCount > 0) );
              sPkgStmt = sPkgStmt->next )
        {
            if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
            {
                sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                sSubprogramPlanTree = sSubprogram->parseTree;

                if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sSubprogramPlanTree->expCallSpec;

                    (void)qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                     &(sCallSpec->functionPtr) );
                    sLibraryLoadCount--;
                }
            }
        }
    }

    if ( sLibraryUnloadCount > 0 )
    {
        for ( sPkgStmt = sOriPkgInfo.planTree->block->subprograms;
              ( (sPkgStmt != NULL) && (sLibraryUnloadCount > 0) );
              sPkgStmt = sPkgStmt->next )
        {
            if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
            {
                sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                sSubprogramPlanTree = sSubprogram->parseTree;

                if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sSubprogramPlanTree->expCallSpec;

                    (void)qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                                   &(sCallSpec->functionPtr),
                                                   sCallSpec->fileSpec,
                                                   sCallSpec->functionName );
                    sLibraryUnloadCount--;
                }
            }
        }
    }

    switch( sStage )
    {
        case 1:
            idlOS::memcpy( aPkgInfo, &sOriPkgInfo, ID_SIZEOF(qsxPkgInfo) );
            break;
        default:
            break;
    }

    if( sIsStmtAlloc == ID_TRUE )
    {
        if( sStatement.spvEnv->latched == ID_TRUE )
        {
            if ( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                 != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            sStatement.spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }

        (void) qcg::freeStatement(&sStatement);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsx::doJobForEachPkg (
    smiStatement    * aSmiStmt,
    iduMemory       * aIduMem,
    qsObjectType      aObjType,
    qsxJobForPkg      aJobPkg)
{
    vSLong      sPkgCount;
    vSLong      sSelectedPkgCount;
    qcmPkgs   * sQcmPkgs;
    UInt        i;

    IDE_TEST( qcmPkg::pkgSelectCountWithObjectType( aSmiStmt,
                                                    aObjType,
                                                    &sPkgCount )
              != IDE_SUCCESS );

    if (sPkgCount > 0)
    {
        IDE_TEST( aIduMem->cralloc(
                idlOS::align8((UInt)ID_SIZEOF( qcmPkgs )) * sPkgCount,
                (void**)&sQcmPkgs)
            != IDE_SUCCESS);

        IDE_TEST( qcmPkg::pkgSelectAllWithObjectType (
                        aSmiStmt,
                        aObjType,
                        sPkgCount,
                        &sSelectedPkgCount,
                        sQcmPkgs )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sPkgCount != sSelectedPkgCount,
                        err_pkg_count_mismatch );

        for ( i = 0; i < (UInt) sPkgCount; i++ )
        {
            IDE_TEST( (*aJobPkg) ( aSmiStmt, &sQcmPkgs[ i ] )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_pkg_count_mismatch);
    {
        IDE_SET(ideSetErrorCode(
                qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                "[qsx::doJobForEachPkg] err_pkg_count_mismatch"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxLoadPkg( smiStatement   * aSmiStmt,
                   qcmPkgs        * aPkg )
{
    /* PROJ-2446 ONE SOURCE FOR SUPPOTING PACKAGE */
    return qsx::qsxLoadPkgByPkgOID( aSmiStmt,
                                    aPkg->pkgOID,
                                    ID_FALSE );
}

IDE_RC qsx::qsxLoadPkgByPkgOID( smiStatement   * aSmiStmt,
                                smOID            aPkgOID,
                                idBool           aCreatePkgInfo )
{
    qcStatement         sStatement;
    qsxPkgObjectInfo  * sPkgObjectInfo;
    qsxPkgInfo        * sPkgInfo;
    UInt                sStage = 0;

    // make qcStatement : alloc the members of qcStatement
    // 실제 Stack Count는 수행 시점에 Template 복사에 의하여
    // Session 정보로부터 결정된다.
    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL ) != IDE_SUCCESS );

    qsxEnv::initialize(sStatement.spxEnv, NULL);
    sStage = 1;

    sStatement.mStatistics = aSmiStmt->mStatistics;

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    if ( aCreatePkgInfo == ID_FALSE )
    {
        IDE_TEST( qsxPkg::getPkgObjectInfo( aPkgOID,
                                            &sPkgObjectInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        sPkgObjectInfo = NULL;
    }

    // proj-1535
    // 이미 생성된 PSM은 skip
    if ( sPkgObjectInfo == NULL )
    {
        IDE_TEST( qsxPkg::createPkgObjectInfo( aPkgOID, &sPkgObjectInfo )
                  != IDE_SUCCESS );
        sStage   = 2;

        IDE_TEST( qsxPkg::setPkgObjectInfo( aPkgOID,
                                            sPkgObjectInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qsxPkg::createPkgInfo( aPkgOID, &sPkgInfo )
                  != IDE_SUCCESS );
        sStage   = 3;

        IDE_TEST( qsxPkg::setPkgInfo( aPkgOID,
                                      sPkgInfo ) != IDE_SUCCESS );

        IDE_TEST( qsxPkg::makeStatusInvalidTx( &sStatement,
                                               aPkgOID )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sStage = 0;
    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 3:
            if ( qsxPkg::destroyPkgInfo( &sPkgInfo ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 2:
            if ( qsxPkg::destroyPkgObjectInfo( aPkgOID, &sPkgObjectInfo )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 1:
            (void) qcg::freeStatement(&sStatement);
    }

    return IDE_FAILURE;
}

IDE_RC qsxRecompilePkg( smiStatement   * aSmiStmt,
                        qcmPkgs        * aPkg )
{
    qcStatement      sStatement;
    qsxPkgInfo     * sPkgInfo;
    qsOID            sPkgOID;
    UInt             sStage = 0;

    sPkgOID = aPkg->pkgOID;

    // make qcStatement : alloc the members of qcStatement
    // 실제 Stack Count는 수행 시점에 Template 복사에 의하여
    // Session 정보로부터 결정된다.
    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL ) != IDE_SUCCESS );

    // BUG-26017
    // [PSM] server restart시 수행되는
    // psm load과정에서 관련프로퍼티를 참조하지 못하는 경우 있음.
    sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_INTERNAL_LOAD_PROC_MASK;
    sStatement.session->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_LOAD_PROC_TRUE;

    qsxEnv::initialize(sStatement.spxEnv, NULL);
    sStage = 1;

    sStatement.mStatistics = aSmiStmt->mStatistics;

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                  &sPkgInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgInfo != NULL );

    // BUG-13707 isValid가 ID_FALSE 인 경우에 doPkgRecompile 한다.
    if (sPkgInfo->isValid != ID_TRUE )
    {
        // server start 중에는 doPkgRecompile에 실패할 수 있다.
        if( qsx::doPkgRecompile( &sStatement,
                                 sPkgOID,
                                 sPkgInfo,
                                 ID_TRUE ) != IDE_SUCCESS )
        {
            IDE_TEST( qsxPkg::makeStatusInvalidTx( &sStatement,
                                                   sPkgOID )
                      != IDE_SUCCESS );
        }
    }

    sStage = 0;
    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 1:
            (void) qcg::freeStatement(&sStatement);
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsx::loadAllPkgSpec( smiStatement    * aSmiStmt,
                            iduMemory       * aIduMem)
{
    IDE_TEST( doJobForEachPkg ( aSmiStmt,
                                aIduMem,
                                QS_PKG,
                                qsxLoadPkg )
              != IDE_SUCCESS );

    // BUG-24338
    // _STORE_PROC_MODE가 1(startup시 compile)이면 recompile을 시도한다.
    if ( ! ( QCU_STORED_PROC_MODE & QCU_SPM_MASK_DISABLE ) )
    {
        IDE_TEST( doJobForEachPkg ( aSmiStmt,
                                    aIduMem,
                                    QS_PKG,
                                    qsxRecompilePkg )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::loadAllPkgBody( smiStatement    * aSmiStmt,
                            iduMemory       * aIduMem)
{
    IDE_TEST( doJobForEachPkg ( aSmiStmt,
                                aIduMem,
                                QS_PKG_BODY,
                                qsxLoadPkg )
              != IDE_SUCCESS );

    // BUG-24338
    // _STORE_PROC_MODE가 1(startup시 compile)이면 recompile을 시도한다.
    if ( ! ( QCU_STORED_PROC_MODE & QCU_SPM_MASK_DISABLE ) )
    {
        IDE_TEST( doJobForEachPkg ( aSmiStmt,
                                    aIduMem,
                                    QS_PKG_BODY,
                                    qsxRecompilePkg )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC qsxUnloadPkg( smiStatement   * /* aSmiStmt */,
                     qcmPkgs        * aPkg )
{
    qsxPkgInfo     * sPkgInfo;
    qsOID            sPkgOID;
    UInt             sStage = 0;

    sPkgOID = aPkg->pkgOID;

    IDE_TEST( qsxPkg::latchX( sPkgOID,
                              ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( qsxPkg::latchXForStatus( sPkgOID ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                  &sPkgInfo ) != IDE_SUCCESS );

    IDE_TEST( qsxPkg::destroyPkgInfo( &sPkgInfo ) != IDE_SUCCESS );

    IDE_TEST( qsxPkg::disablePkgObjectInfo( sPkgOID ) != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST( qsxPkg::unlatchForStatus( sPkgOID )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( qsxPkg::unlatch( sPkgOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 2:
            if ( qsxPkg::unlatchForStatus( sPkgOID )
                 != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1:
            if ( qsxPkg::unlatch( sPkgOID )
                 != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsx::unloadAllPkg( smiStatement * aSmiStmt,
                          iduMemory    * aIduMem)
{
    IDE_TEST( doJobForEachPkg ( aSmiStmt,
                                aIduMem,
                                QS_PKG_BODY,
                                qsxUnloadPkg )
              != IDE_SUCCESS );

    IDE_TEST( doJobForEachPkg ( aSmiStmt,
                                aIduMem,
                                QS_PKG,
                                qsxUnloadPkg )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::rebuildQsxProcInfoPrivilege( qcStatement     * aStatement,
                                         qsOID             aProcOID )
{
    qcuSqlSourceInfo     sqlInfo;
    qsxProcInfo        * sProcInfo;
    UInt                 sStage = 0;
    UInt               * sGranteeID = NULL;

    IDE_TEST( qsxProc::latchX( aProcOID,
                               ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( qsxProc::getProcInfo( aProcOID,
                                    &sProcInfo )
              != IDE_SUCCESS );

    // BUG-38994
    // procedure, function이 참조하는 객체가 존재하지 않으면
    // 해당 객체의 상태는 invalid가 된다.
    // 객체가 invalid한 경우에는 meta able의 정보만 변경한다.
    if ( sProcInfo->isValid == ID_TRUE )
    {
        // to fix BUG-24905
        // granteeid를 새로 build하는 경우
        // getGranteeOfPSM함수를 호출하면
        // granteeid가 새롭게 생성된다.
        // 따라서, 기존의 granteeid는 free
        sGranteeID = sProcInfo->granteeID;

        // rebuild only privilege information
        IDE_TEST(qcmPriv::getGranteeOfPSM(
                QC_SMI_STMT(aStatement),
                QCM_OID_TO_BIGINT( sProcInfo->planTree->procOID ),
                &sProcInfo->privilegeCount,
                &sProcInfo->granteeID )
            != IDE_SUCCESS);

        if( sGranteeID != NULL )
        {
            IDE_TEST(iduMemMgr::free(sGranteeID));
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

    sStage = 0;
    // unlatch실패하면 fatal이므로
    // 메모리 원복에 대해 고려하지 않는다.
    IDE_TEST( qsxProc::unlatch( aProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 1:
            if ( qsxProc::unlatch( aProcOID ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }

    return IDE_FAILURE;
}

UShort
qsx::getResultSetCount( qcStatement * aStatement )
{
    qsExecParseTree  * sParseTree;
    qsxRefCursorInfo * sRefCursorInfo;
    qsxRefCursorInfo * sTmpRefCursorInfo;
    qtcNode          * sRefCursorNode;
    qtcNode          * sTmpRefCursorNode;
    UShort             sResultSetCount = 0;
    iduListNode      * sIterator1 = NULL;
    iduListNode      * sIterator2 = NULL;

    sParseTree = (qsExecParseTree*)aStatement->myPlan->parseTree;

    IDU_LIST_ITERATE( &sParseTree->refCurList, sIterator1 )
    {
        sRefCursorNode = (qtcNode*)sIterator1->mObj;

        sRefCursorInfo = (qsxRefCursorInfo*)QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aStatement),
                                                                sRefCursorNode );

        IDU_LIST_ITERATE( &sParseTree->refCurList, sIterator2 )
        {
            sTmpRefCursorNode = (qtcNode*)sIterator2->mObj;

            sTmpRefCursorInfo = (qsxRefCursorInfo*)QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aStatement),
                                                                       sTmpRefCursorNode );

            if( (sRefCursorInfo != sTmpRefCursorInfo) &&
                (sRefCursorInfo->hStmt == sTmpRefCursorInfo->hStmt) )
            {
                sTmpRefCursorInfo->isOpen = ID_FALSE;
            }
        }
    }
    
    IDU_LIST_ITERATE( &sParseTree->refCurList, sIterator1 )
    {
        sRefCursorNode = (qtcNode*)sIterator1->mObj;

        sRefCursorInfo = (qsxRefCursorInfo*)QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aStatement),
                                                                sRefCursorNode );

        if( QSX_REF_CURSOR_IS_OPEN( sRefCursorInfo ) == ID_TRUE )
        {
            sResultSetCount++;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sResultSetCount;
}

IDE_RC
qsx::getResultSetInfo( qcStatement * aStatement,
                       UShort        aResultSetID,
                       void       ** aResultSetStmt,
                       idBool      * aRecordExist )
{
    qsExecParseTree  * sParseTree;
    qsxRefCursorInfo * sRefCursorInfo;
    qtcNode          * sRefCursorNode;
    UInt               sResultSetCount = 0;
    iduListNode      * sIterator = NULL;

    sParseTree = (qsExecParseTree*)aStatement->myPlan->parseTree;

    *aResultSetStmt = NULL;
    *aRecordExist = ID_FALSE;

    IDU_LIST_ITERATE( &sParseTree->refCurList, sIterator )
    {
        sRefCursorNode = (qtcNode*)sIterator->mObj;

        sRefCursorInfo = (qsxRefCursorInfo*)QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aStatement),
                                                                sRefCursorNode );

        if( QSX_REF_CURSOR_IS_OPEN( sRefCursorInfo ) == ID_TRUE )
        {
            if( sResultSetCount == aResultSetID )
            {
                *aResultSetStmt = sRefCursorInfo->hStmt;

                *aRecordExist = sRefCursorInfo->nextRecordExist;

                break;
            }
            else
            {
                // Nothing to do.
            }

            sResultSetCount++;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_ASSERT( *aResultSetStmt != NULL );

    return IDE_SUCCESS;
}

IDE_RC qsx::createPkg(qcStatement * aStatement)
{
    qsPkgParseTree    * sPlanTree;
    qsxPkgObjectInfo  * sPkgObjectInfo;
    qsxPkgInfo        * sPkgInfo ;
    qsOID               sPkgOID     = 0;
    const void        * sSmiObjHandle;
    SInt                sStage      = 0;
    iduVarMemList     * sQmsMem     = NULL;

    sPlanTree = (qsPkgParseTree*)aStatement->myPlan->parseTree;

    IDE_DASSERT( sPlanTree->objType == QS_PKG );

    IDE_TEST( smiObject::createObject( QC_SMI_STMT(aStatement),
                                       NULL,
                                       0,
                                       NULL,
                                       SMI_OBJECT_PACKAGE,
                                       &sSmiObjHandle )
              != IDE_SUCCESS );

    sPkgOID = (qsOID) smiGetTableId( sSmiObjHandle );
    sStage = 1;

    IDE_TEST( qsxPkg::createPkgObjectInfo( sPkgOID, &sPkgObjectInfo )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( qsxPkg::setPkgObjectInfo( sPkgOID,
                                        sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_TEST( qsxPkg::createPkgInfo( sPkgOID, &sPkgInfo )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( qsxPkg::setPkgInfo( sPkgOID,
                                  sPkgInfo )
              != IDE_SUCCESS );

    qcg::setPlanTreeState( aStatement, ID_FALSE );

    IDE_TEST( qsx::makePkgInfoMembers ( sPkgInfo,
                                        aStatement )
              != IDE_SUCCESS );

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_TRUE );

    IDE_TEST( qsxPkg::latchX( sPkgOID,
                              ID_TRUE ) != IDE_SUCCESS );
    sStage = 4;

    sPkgInfo->isValid = ID_TRUE;
    IDE_TEST( qcmPkg::insert( aStatement, sPlanTree ) != IDE_SUCCESS );

    IDE_TEST( qsxPkg::setPkgText( QC_SMI_STMT( aStatement ),
                                  sPkgOID,
                                  aStatement->myPlan->stmtText,
                                  aStatement->myPlan->stmtTextLen )
              != IDE_SUCCESS );

    sQmsMem = QC_QMP_MEM( aStatement );
    IDE_TEST( qsx::makeNewPreparedMem( aStatement ) != IDE_SUCCESS );

    sPkgInfo->qmsMem  = sQmsMem;

    sStage = 3;
    IDE_TEST( qsxPkg::unlatch( sPkgOID ) != IDE_SUCCESS );

    // BUG-11266
    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated(
            aStatement,
            sPlanTree->userID,
            (SChar *) (sPlanTree->pkgNamePos.stmtText +
                       sPlanTree->pkgNamePos.offset),
            sPlanTree->pkgNamePos.size,
            QS_PKG)
        != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 4:
            if ( qsxPkg::unlatch( sPkgOID ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 3:
            if ( qsxPkg::destroyPkgInfo ( &sPkgInfo ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 2:
            if ( qsxPkg::destroyPkgObjectInfo ( sPkgOID, &sPkgObjectInfo )
                 != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1:
            if ( smiObject::dropObject( QC_SMI_STMT( aStatement ),
                                        sSmiObjHandle ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsx::createPkgBody(qcStatement * aStatement)
{
    qsPkgParseTree    * sPkgPlanTree;
    qsxPkgObjectInfo  * sPkgObjectInfo;
    qsxPkgInfo        * sPkgInfo ;
    qsOID               sPkgOID     = 0;
    const void        * sSmiObjHandle;
    iduVarMemList     * sQmsMem     = NULL;

    SInt                sStage      = 0;
    UInt                sLibraryLoadCount = 0;

    qsPkgStmts        * sPkgStmt = NULL;
    qsPkgSubprograms  * sSubprogram;
    qsProcParseTree   * sSubprogramPlanTree = NULL;
    qsCallSpec        * sCallSpec;

    sPkgPlanTree = (qsPkgParseTree*)aStatement->myPlan->parseTree;

    IDE_DASSERT( sPkgPlanTree->objType == QS_PKG_BODY );

    IDE_TEST( smiObject::createObject( QC_SMI_STMT(aStatement),
                                       NULL,
                                       0,
                                       NULL,
                                       SMI_OBJECT_PACKAGE,
                                       &sSmiObjHandle )
              != IDE_SUCCESS );

    sPkgOID = (qsOID) smiGetTableId( sSmiObjHandle );
    sStage = 1;

    IDE_TEST( qsxPkg::createPkgObjectInfo( sPkgOID, &sPkgObjectInfo )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( qsxPkg::setPkgObjectInfo( sPkgOID,
                                        sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_TEST( qsxPkg::createPkgInfo( sPkgOID, &sPkgInfo )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( qsxPkg::setPkgInfo( sPkgOID,
                                  sPkgInfo )
              != IDE_SUCCESS );

    qcg::setPlanTreeState( aStatement, ID_FALSE );

    /* BUG-38844
       create or replace package body인 경우, spec에 S latch를 잡는다. */
    IDE_TEST( qsxPkg::latchS( aStatement->spvEnv->pkgPlanTree->pkgOID )
              != IDE_SUCCESS );
    sStage = 4;

    IDE_TEST( qsx::makePkgInfoMembers ( sPkgInfo,
                                        aStatement )
              != IDE_SUCCESS );

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_TRUE );

    IDE_TEST( qsxPkg::latchX( sPkgOID,
                              ID_TRUE )
              != IDE_SUCCESS );
    sStage = 5;

    /* BUG-39094
       해당 package를 related object로 가지는 객체의 상태를 invalid 상태로 바꾼다.
       이는 package body가 없는 상태에서 package를 참조하는 객체를 만들 수 있는데,
       차후 실행 시 package body의 정보가 없어서 무한 rebuild에 빠지는 것을 방지하기 위해
       recompile 시켜줘야 하기 때문이다. */
    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated (
                  aStatement,
                  sPkgPlanTree->userID,
                  sPkgPlanTree->pkgNamePos.stmtText +
                  sPkgPlanTree->pkgNamePos.offset,
                  sPkgPlanTree->pkgNamePos.size,
                  QS_PKG )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::relSetInvalidProcOfRelated (
                  aStatement,
                  sPkgPlanTree->userID,
                  sPkgPlanTree->pkgNamePos.stmtText +
                  sPkgPlanTree->pkgNamePos.offset,
                  sPkgPlanTree->pkgNamePos.size,
                  QS_PKG )
              != IDE_SUCCESS );

    IDE_TEST( qcmView::setInvalidViewOfRelated(
                  aStatement,
                  sPkgPlanTree->userID,
                  sPkgPlanTree->pkgNamePos.stmtText +
                  sPkgPlanTree->pkgNamePos.offset,
                  sPkgPlanTree->pkgNamePos.size,
                  QS_PKG )
              != IDE_SUCCESS );

    sPkgInfo->isValid = ID_TRUE;
    IDE_TEST( qcmPkg::insert( aStatement, sPkgPlanTree ) != IDE_SUCCESS );

    IDE_TEST( qsxPkg::setPkgText( QC_SMI_STMT( aStatement ),
                                  sPkgOID,
                                  aStatement->myPlan->stmtText,
                                  aStatement->myPlan->stmtTextLen )
              != IDE_SUCCESS );

    sQmsMem = QC_QMP_MEM( aStatement );
    IDE_TEST( qsx::makeNewPreparedMem( aStatement ) != IDE_SUCCESS );

    sPkgInfo->qmsMem  = sQmsMem;

    for ( sPkgStmt = sPkgPlanTree->block->subprograms;
          sPkgStmt != NULL;
          sPkgStmt = sPkgStmt->next )
    {
        if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
        {
            sSubprogram = (qsPkgSubprograms*)sPkgStmt;
            sSubprogramPlanTree = sSubprogram->parseTree;

            if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
            {
                sCallSpec = sSubprogramPlanTree->expCallSpec;

                IDE_TEST( qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                                   &(sCallSpec->functionPtr),
                                                   sCallSpec->fileSpec,
                                                   sCallSpec->functionName )
                          != IDE_SUCCESS );

                IDE_ERROR( sCallSpec->libraryNode != NULL );
                sLibraryLoadCount++;

                //   Library를 load하지 못했거나, function을 제대로 찾지 못한경우
                //   invalid 상태로 만들어 다음에 rebuild하면서
                //   library를 load하거나 function을 찾도록 한다.
                if ( (sCallSpec->libraryNode->mHandle == PDL_SHLIB_INVALID_HANDLE) ||
                     (sCallSpec->libraryNode->mFunctionPtr == NULL) ||
                     (sCallSpec->functionPtr == NULL) )
                {
                    sPkgInfo->isValid = ID_FALSE;
                }
            }
            else if ( sSubprogramPlanTree->procType == QS_EXTERNAL_C )
            {
                sCallSpec = sSubprogramPlanTree->expCallSpec;

                sCallSpec->libraryNode = NULL;
                sCallSpec->functionPtr = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    sStage = 4;
    IDE_TEST( qsxPkg::unlatch( sPkgOID ) != IDE_SUCCESS );

    /* BUG-38844
       create or replace package body인 경우, 잡았던 latchS를 unlatch 한다. */
    sStage = 3;
    IDE_TEST( qsxPkg::unlatch( aStatement->spvEnv->pkgPlanTree->pkgOID)
              != IDE_SUCCESS );

    // BUG-11266
    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated(
            aStatement,
            sPkgPlanTree->userID,
            (SChar *) (sPkgPlanTree->pkgNamePos.stmtText +
                       sPkgPlanTree->pkgNamePos.offset),
            sPkgPlanTree->pkgNamePos.size,
            QS_PKG)
        != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 5:
            {
                if ( sLibraryLoadCount > 0 )
                {
                    for ( sPkgStmt = sPkgPlanTree->block->subprograms;
                          ( (sPkgStmt != NULL) && (sLibraryLoadCount > 0) );
                          sPkgStmt = sPkgStmt->next )
                    {
                        if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
                        {
                            sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                            sSubprogramPlanTree = sSubprogram->parseTree;

                            if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                            {
                                sCallSpec = sSubprogramPlanTree->expCallSpec;

                                (void)qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                                 &(sCallSpec->functionPtr) );
                                sLibraryLoadCount--;
                            }
                        }
                    }
                }

                if ( qsxPkg::unlatch( sPkgOID ) != IDE_SUCCESS )
                {
                    (void) IDE_ERRLOG(IDE_QP_1);
                }
                /* fall through */
            }
        case 4:
            if ( qsxPkg::unlatch( aStatement->spvEnv->pkgPlanTree->pkgOID)
                 != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 3:
            if ( qsxPkg::destroyPkgInfo ( &sPkgInfo ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 2:
            if ( qsxPkg::destroyPkgObjectInfo ( sPkgOID, &sPkgObjectInfo )
                 != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1:
            if ( smiObject::dropObject( QC_SMI_STMT( aStatement ),
                                        sSmiObjHandle ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsx::replacePkgOrPkgBody(qcStatement * aStatement)
{
    qsPkgParseTree    * sPlanTree;
    qsxPkgInfo        * sPkgInfo ;
    qsOID               sPkgOID = 0;
    UInt                sPkgUserID;    
    SInt                sStage = 0;
    qsxPkgInfo          sOriPkgInfo;
    iduVarMemList     * sQmsMem = NULL;

    qsPkgStmts        * sPkgStmt = NULL;
    qsPkgSubprograms  * sSubprogram;
    qsProcParseTree   * sSubprogramPlanTree = NULL;
    qsCallSpec        * sCallSpec;

    UInt                sLibraryUnloadCount = 0;
    UInt                sLibraryLoadCount = 0;

    sPlanTree = (qsPkgParseTree *)(aStatement->myPlan->parseTree);

    sPkgOID = sPlanTree->pkgOID;

    /* BUG-38844
       create or replace package body인 경우, spec에 S latch를 잡는다. */
    if ( sPlanTree->objType == QS_PKG_BODY )
    {
        IDE_TEST( qsxPkg::latchS( aStatement->spvEnv->pkgPlanTree->pkgOID )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    sStage = 1;

    IDE_TEST( qsxPkg::latchX( sPkgOID,
                              ID_TRUE ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                  &sPkgInfo )
              != IDE_SUCCESS );

    idlOS::memcpy( &sOriPkgInfo,
                   sPkgInfo,
                   ID_SIZEOF(qsxPkgInfo) );
    sStage = 3;

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_FALSE );

    //To Fix BUG-19839 : ProcedureOID를 사용하여 소유자의 UserID를 얻는다.
    IDE_TEST( qcmPkg::getPkgUserID( aStatement,
                                    sPkgOID,
                                    &sPkgUserID )
              != IDE_SUCCESS );

    IDE_TEST( qsx::makePkgInfoMembers ( sPkgInfo,
                                        aStatement )
              != IDE_SUCCESS );

    sPkgInfo->isValid = ID_TRUE;

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_TRUE );

    if ( sPlanTree->objType == QS_PKG_BODY )
    {
        if ( sOriPkgInfo.planTree != NULL )
        {
            for ( sPkgStmt = sOriPkgInfo.planTree->block->subprograms;
                  sPkgStmt != NULL;
                  sPkgStmt = sPkgStmt->next )
            {
                if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
                {
                    sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                    sSubprogramPlanTree = sSubprogram->parseTree;

                    if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                    {
                        sCallSpec = sSubprogramPlanTree->expCallSpec;

                        IDE_TEST( qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                             &(sCallSpec->functionPtr) )
                                  != IDE_SUCCESS );
                        sLibraryUnloadCount++;
                    }
                }
            }
        }

        for ( sPkgStmt = sPkgInfo->planTree->block->subprograms;
              sPkgStmt != NULL;
              sPkgStmt = sPkgStmt->next )
        {
            if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
            {
                sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                sSubprogramPlanTree = sSubprogram->parseTree;

                if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sSubprogramPlanTree->expCallSpec;

                    IDE_TEST( qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                                       &(sCallSpec->functionPtr),
                                                       sCallSpec->fileSpec,
                                                       sCallSpec->functionName )
                              != IDE_SUCCESS );

                    IDE_ERROR( sCallSpec->libraryNode != NULL );
                    sLibraryLoadCount++;

                    //   Library를 load하지 못했거나, function을 제대로 찾지 못한경우
                    //   invalid 상태로 만들어 다음에 rebuild하면서
                    //   library를 load하거나 function을 찾도록 한다.
                    if ( (sCallSpec->libraryNode->mHandle == PDL_SHLIB_INVALID_HANDLE) ||
                         (sCallSpec->libraryNode->mFunctionPtr == NULL) ||
                         (sCallSpec->functionPtr == NULL) )
                    {
                        sPkgInfo->isValid = ID_FALSE;
                    }
                }
                else if ( sSubprogramPlanTree->procType == QS_EXTERNAL_C )
                {
                    sCallSpec = sSubprogramPlanTree->expCallSpec;

                    sCallSpec->libraryNode = NULL;
                    sCallSpec->functionPtr = NULL;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }

    IDE_TEST( qcmPkg::remove( aStatement,
                              sPkgOID,
                              sPlanTree->userNamePos,
                              sPlanTree->pkgNamePos,
                              sPlanTree->objType,
                              ID_TRUE /* aPreservePrivInfo */  ) != IDE_SUCCESS );

    if ( sPlanTree->objType == QS_PKG_BODY )
    {
        /* BUG-39094
           해당 package를 related object로 가지는 객체의 상태를 invalid 상태로 바꾼다.
           이는 package body가 없는 상태에서 package를 참조하는 객체를 만들 수 있는데,
           차후 실행 시 package body의 정보가 없어서 무한 rebuild에 빠지는 것을 방지하기 위해
           recompile 시켜줘야 하기 때문이다. */
        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated (
                      aStatement,
                      sPlanTree->userID,
                      sPlanTree->pkgNamePos.stmtText +
                      sPlanTree->pkgNamePos.offset,
                      sPlanTree->pkgNamePos.size,
                      QS_PKG )
                  != IDE_SUCCESS );

        IDE_TEST( qcmProc::relSetInvalidProcOfRelated (
                      aStatement,
                      sPlanTree->userID,
                      sPlanTree->pkgNamePos.stmtText +
                      sPlanTree->pkgNamePos.offset,
                      sPlanTree->pkgNamePos.size,
                      QS_PKG )
                  != IDE_SUCCESS );

        IDE_TEST( qcmView::setInvalidViewOfRelated(
                      aStatement,
                      sPlanTree->userID,
                      sPlanTree->pkgNamePos.stmtText +
                      sPlanTree->pkgNamePos.offset,
                      sPlanTree->pkgNamePos.size,
                      QS_PKG )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qcmPkg::insert( aStatement,
                              sPlanTree ) != IDE_SUCCESS );

    IDE_TEST( qsxPkg::setPkgText( QC_SMI_STMT( aStatement ),
                                  sPkgOID,
                                  aStatement->myPlan->stmtText,
                                  aStatement->myPlan->stmtTextLen )
              != IDE_SUCCESS );

    IDE_TEST( qsxPkg::setPkgInfo( sPkgOID,
                                  sPkgInfo ) != IDE_SUCCESS );

    sQmsMem = QC_QMP_MEM( aStatement );
    IDE_TEST( qsx::makeNewPreparedMem( aStatement ) != IDE_SUCCESS );

    sPkgInfo->qmsMem  = sQmsMem;
    sPkgInfo->modifyCount ++;

    sStage = 2;
    if( sOriPkgInfo.qmsMem != NULL )
    {
        IDE_TEST( sOriPkgInfo.qmsMem->destroy() != IDE_SUCCESS );

        IDE_TEST( qcg::freeIduVarMemList( sOriPkgInfo.qmsMem ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-11266
    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated(
                  aStatement,
                  sPlanTree->userID,
                  (SChar *) (sPlanTree->pkgNamePos.stmtText +
                             sPlanTree->pkgNamePos.offset),
                  sPlanTree->pkgNamePos.size,
                  QS_PKG)
              != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST( qsxPkg::unlatch( sPkgOID ) != IDE_SUCCESS );

    /* BUG-38844
       create or replace package body인 경우, 잡았던 latchS를 unlatch 한다. */
    sStage = 0;
    if ( sPlanTree->objType == QS_PKG_BODY )
    {
        IDE_TEST( qsxPkg::unlatch( aStatement->spvEnv->pkgPlanTree->pkgOID)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLibraryLoadCount > 0 )
    {
        for ( sPkgStmt = sPkgInfo->planTree->block->subprograms;
              ( (sPkgStmt != NULL) && (sLibraryLoadCount > 0) );
              sPkgStmt = sPkgStmt->next )
        {
            if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
            {
                sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                sSubprogramPlanTree = sSubprogram->parseTree;

                if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sSubprogramPlanTree->expCallSpec;

                    (void)qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                     &(sCallSpec->functionPtr) );
                    sLibraryLoadCount--;
                }
            }
        }
    }

    if ( sLibraryUnloadCount > 0 )
    {
        for ( sPkgStmt = sOriPkgInfo.planTree->block->subprograms;
              ( (sPkgStmt != NULL) && (sLibraryUnloadCount > 0) );
              sPkgStmt = sPkgStmt->next )
        {
            if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
            {
                sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                sSubprogramPlanTree = sSubprogram->parseTree;

                if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sSubprogramPlanTree->expCallSpec;

                    (void)qsxLibrary::loadLibrary( &(sCallSpec->libraryNode),
                                                   &(sCallSpec->functionPtr),
                                                   sCallSpec->fileSpec,
                                                   sCallSpec->functionName );
                    sLibraryUnloadCount--;
                }
            }
        }
    }

    switch( sStage )
    {
        case 3:
            idlOS::memcpy( sPkgInfo,
                           &sOriPkgInfo,
                           ID_SIZEOF(qsxPkgInfo) );
            /* fall through */
        case 2:
            if ( qsxPkg::unlatch( sPkgOID ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1:
            if ( sPlanTree->objType == QS_PKG_BODY )
            {
                if ( qsxPkg::unlatch( aStatement->spvEnv->pkgPlanTree->pkgOID)
                     != IDE_SUCCESS )
                {
                    (void) IDE_ERRLOG(IDE_QP_1);
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

    // fix BUG-18704
    qcg::setPlanTreeState( aStatement, ID_TRUE );

    return IDE_FAILURE;
}

IDE_RC qsx::dropPkg(qcStatement * aStatement)
{
    qsDropPkgParseTree * sPlanTree;
    qsOID                sPkgSpecOID;
    qsOID                sPkgBodyOID;
    UInt                 sUserID;
    SInt                 sStage = 0;

    sPlanTree   = (qsDropPkgParseTree *)(aStatement->myPlan->parseTree);
    sPkgSpecOID = sPlanTree->specOID;
    sPkgBodyOID = sPlanTree->bodyOID;

    IDE_TEST( qcmUser::getUserID( aStatement,
                                  sPlanTree->userNamePos,
                                  &sUserID )
              != IDE_SUCCESS );

    /* BUG-38844 
       package는 package spec / body 2개의 객체가 존재하며,
       drop 시 위의 두 객체 모두 latch를 둘 다 잡아야 한다.
       latch를 잡는 순서는 drop package body 이든 drop package이든
       spec->body 순으로 잡는다.
       단, package body의 경우, optional하기 때문에 존재 유무 파악 후 잡아줘야 한다. */
    IDE_TEST( qsxPkg::latchX( sPkgSpecOID,
                              ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    if ( sPkgBodyOID != QS_EMPTY_OID )
    {
        IDE_TEST( qsxPkg::latchX( sPkgBodyOID,
                                  ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    sStage = 2;

    IDE_TEST( qsxPkg::latchXForStatus( sPkgSpecOID )
              != IDE_SUCCESS );
    sStage = 3;

    if ( sPkgBodyOID != QS_EMPTY_OID )
    {
        IDE_TEST( qsxPkg::latchXForStatus( sPkgBodyOID )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    sStage = 4;

    IDE_TEST( dropPkgCommon( aStatement,
                             sPkgSpecOID,
                             sPkgBodyOID,
                             sUserID,
                             sPlanTree->pkgNamePos.stmtText +
                             sPlanTree->pkgNamePos.offset,
                             sPlanTree->pkgNamePos.size,
                             sPlanTree->option ) 
              != IDE_SUCCESS );

    sStage = 3;
    if ( sPkgBodyOID != QS_EMPTY_OID )
    {
        IDE_TEST( qsxPkg::unlatchForStatus( sPkgBodyOID ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sStage = 2;
    IDE_TEST( qsxPkg::unlatchForStatus( sPkgSpecOID ) != IDE_SUCCESS );

    sStage = 1;
    if ( sPkgBodyOID != QS_EMPTY_OID )
    {
        IDE_TEST( qsxPkg::unlatch( sPkgBodyOID ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sStage = 0;
    IDE_TEST( qsxPkg::unlatch( sPkgSpecOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 4:
            if ( sPkgBodyOID != QS_EMPTY_OID )
            {
                if ( qsxPkg::unlatchForStatus( sPkgBodyOID ) != IDE_SUCCESS )
                {
                    (void) IDE_ERRLOG(IDE_QP_1);
                }
            }
            /* fall through */
        case 3:
            if ( qsxPkg::unlatchForStatus( sPkgSpecOID ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 2:
            if ( sPkgBodyOID != QS_EMPTY_OID )
            {
                if ( qsxPkg::unlatch( sPkgBodyOID ) != IDE_SUCCESS )
                {
                    (void) IDE_ERRLOG(IDE_QP_1);
                }
            }
            /* fall through */
        case 1:
            if ( qsxPkg::unlatch( sPkgSpecOID ) != IDE_SUCCESS )
            {
                (void) IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsx::alterPkg( qcStatement * aStatement )
{
    qsAlterPkgParseTree * sPlanTree;
    qsOID                 sPkgSpecOID;
    qsOID                 sPkgBodyOID;
    SInt                  sLatchStatus = 0;

    sPlanTree = (qsAlterPkgParseTree *)(aStatement->myPlan->parseTree);
    sPkgSpecOID = sPlanTree->specOID;
    sPkgBodyOID = sPlanTree->bodyOID;

    /* BUG-38844
       package는 package spec / body 2개의 객체가 존재하며,
       alter 시 위의 두 객체 모두 latch를 둘 다 잡아야 한다.
       latch를 잡는 순서는 alter package compile의 모든 option에 대해서
       spec->body 순으로 잡는다.
       단, package body의 경우, optional하기 때문에 존재 유무 파악 후 잡아줘야 한다. */
    IDE_TEST( qsxPkg::latchX( sPkgSpecOID,
                              ID_TRUE )
              != IDE_SUCCESS );
    sLatchStatus = 1;

    if ( sPkgBodyOID != QS_EMPTY_OID )
    {
        IDE_TEST( qsxPkg::latchX( sPkgBodyOID,
                                  ID_TRUE )
                  != IDE_SUCCESS );
        sLatchStatus = 2;
    }
    else
    {
        // Nothing to do.
    }

    if( sPlanTree->option == QS_PKG_SPEC_ONLY )
    {
        // alter package spec
        IDE_TEST( alterPkgCommon( aStatement,
                                  sPlanTree,
                                  sPkgSpecOID )
                  != IDE_SUCCESS );
    }
    else
    {
        // alter package spec
        IDE_TEST( alterPkgCommon( aStatement,
                                  sPlanTree,
                                  sPkgSpecOID )
                  != IDE_SUCCESS );

        // atler package body
        if( sPkgBodyOID != QS_EMPTY_OID )
        {
            IDE_TEST( alterPkgCommon( aStatement,
                                      sPlanTree,
                                      sPkgBodyOID )
                      != IDE_SUCCESS );
        }
    }

    /* BUG-38844 unlatch */
    if ( sPkgBodyOID != QS_EMPTY_OID )
    {
        sLatchStatus = 1;
        IDE_TEST( qsxPkg::unlatch( sPkgBodyOID ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sLatchStatus = 0;
    IDE_TEST( qsxPkg::unlatch( sPkgSpecOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sLatchStatus )
    {
        case 2:
            if ( qsxPkg::unlatch( sPkgBodyOID ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            /* fall through */
        case 1:
            if ( qsxPkg::unlatch( sPkgSpecOID ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsx::dropPkgCommonForMeta( qcStatement * aStatement,
                                  qsOID         aPkgOID,
                                  UInt          aUserID,
                                  SChar       * aPkgName,
                                  UInt          aPkgNameSize,
                                  qsObjectType  aPkgType )
{
    qcNamePosition       sPkgNamePos;
    SChar                sPkgName[QC_MAX_OBJECT_NAME_LEN + 1];

    // BUG-36973
    if( aPkgType == QS_PKG )
    {
        sPkgNamePos.stmtText = aPkgName;
        sPkgNamePos.offset = 0;
        sPkgNamePos.size = aPkgNameSize;

        QC_STR_COPY( sPkgName, sPkgNamePos );

        IDE_TEST( qcmAudit::deleteObject( aStatement,
                                          aUserID,
                                          sPkgName )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
        // package body가 지워져도 package가 지워진 것이 아님.
    }

    // Drop object should be done at first time
    // to ensure that there's no more transactions accessing to this one.
    IDE_TEST( smiObject::dropObject( QC_SMI_STMT( aStatement ) ,
                                     smiGetTable( aPkgOID ) )
              != IDE_SUCCESS );

    IDE_TEST( qcmPkg::remove( aStatement,
                              aPkgOID,
                              aUserID,
                              aPkgName,
                              aPkgNameSize,
                              aPkgType,
                              ID_FALSE /* aPreservePrivInfo */ )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::dropPkgCommon( qcStatement * aStatement,
                           qsOID         aPkgSpecOID,
                           qsOID         aPkgBodyOID,
                           UInt          aUserID,
                           SChar       * aPkgName,
                           UInt          aPkgNameSize,
                           qsPkgOption   aOption )
{
    qsxPkgInfo * sPkgSpecInfo = NULL;
    qsxPkgInfo * sPkgBodyInfo = NULL;

    qsPkgStmts        * sPkgStmt = NULL;
    qsPkgSubprograms  * sSubprogram;
    qsProcParseTree   * sSubprogramPlanTree = NULL;
    qsCallSpec        * sCallSpec;

    IDE_TEST( qsxPkg::getPkgInfo( aPkgSpecOID,
                                  &sPkgSpecInfo )
              != IDE_SUCCESS );

    if ( aPkgBodyOID != QS_EMPTY_OID )
    {
        IDE_TEST( qsxPkg::getPkgInfo( aPkgBodyOID,
                                      &sPkgBodyInfo )
                  != IDE_SUCCESS );

        IDE_TEST( dropPkgCommonForMeta( aStatement,
                                        aPkgBodyOID,
                                        aUserID,
                                        aPkgName,
                                        aPkgNameSize,
                                        QS_PKG_BODY )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( aOption != QS_PKG_BODY_ONLY )
    {
        IDE_TEST( dropPkgCommonForMeta( aStatement,
                                        aPkgSpecOID,
                                        aUserID,
                                        aPkgName,
                                        aPkgNameSize,
                                        QS_PKG )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( sPkgBodyInfo != NULL )
    {
        if ( sPkgBodyInfo->planTree != NULL )
        {
            for ( sPkgStmt = sPkgBodyInfo->planTree->block->subprograms;
                  sPkgStmt != NULL;
                  sPkgStmt = sPkgStmt->next )
            {
                if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
                {
                    sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                    sSubprogramPlanTree = sSubprogram->parseTree;

                    if ( sSubprogramPlanTree->procType == QS_INTERNAL_C )
                    {
                        sCallSpec = sSubprogramPlanTree->expCallSpec;

                        IDE_TEST( qsxLibrary::unloadLibrary( sCallSpec->libraryNode,
                                                             &(sCallSpec->functionPtr) )
                                  != IDE_SUCCESS );
                    }
                }
            }
        }
    }

    if ( aPkgBodyOID != QS_EMPTY_OID )
    {
        IDE_TEST(dropPkgCommonForPkgInfo( aPkgBodyOID,
                                          sPkgBodyInfo )
                 != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( aOption != QS_PKG_BODY_ONLY )
    {
        IDE_TEST(dropPkgCommonForPkgInfo( aPkgSpecOID,
                                          sPkgSpecInfo )
                 != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::dropPkgCommonForPkgInfo( qsOID          aPkgOID,
                                     qsxPkgInfo   * aPkgInfo )
{
    IDE_TEST( qsxPkg::destroyPkgInfo ( &aPkgInfo )
              != IDE_SUCCESS );

    IDE_TEST( qsxPkg::disablePkgObjectInfo( aPkgOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::alterPkgCommon( qcStatement         * aStatement,
                            qsAlterPkgParseTree * aPlanTree, 
                            qsOID                 aPkgOID )
{
    qsxPkgInfo         * sPkgInfo = NULL;  // TASK-3876 Code Sonar
    qsObjectType         sObjType;
    UInt                 sUserID;
    UInt                 sStage = 0;

    IDE_TEST( qsxPkg::getPkgInfo( aPkgOID,
                                  &sPkgInfo )
              != IDE_SUCCESS );

    sObjType = sPkgInfo->objType;

    // BUG-38627
    if ( ( sPkgInfo->isValid == ID_FALSE ) ||
         ( sObjType == QS_PKG ) )
    {
        // proj-1535
        // vaild 상태의 PSM도 recompile을 허용한다.
        // refereneced PSM을 invalid로 변경한다.
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      aPlanTree->userNamePos,
                                      &sUserID ) != IDE_SUCCESS );

        /* BUG-39340
           ALTER PACKAGE package_name COMPILE SPECIFICATION;
           구문 시 invalid 되는 객체는 해당 package의 body 객체 뿐이다. */
        if ( sObjType == QS_PKG )
        {
            IDE_TEST( qcmPkg::relSetInvalidPkgBody(
                          aStatement,
                          sUserID,
                          aPlanTree->pkgNamePos.stmtText + aPlanTree->pkgNamePos.offset,
                          aPlanTree->pkgNamePos.size,
                          QS_PKG_BODY )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // fix BUG-18704
        qcg::setPlanTreeState( aStatement, ID_FALSE );
        sStage = 1;

        IDE_TEST( qsx::doPkgRecompile( aStatement,
                                       aPkgOID,
                                       sPkgInfo,
                                       ID_FALSE )
                  != IDE_SUCCESS );

        sStage = 0;
        // fix BUG-18704
        qcg::setPlanTreeState( aStatement, ID_TRUE );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        // fix BUG-18704
        qcg::setPlanTreeState( aStatement, ID_TRUE );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

/* PROJ-1073 Package
 * execute pkg1.v1 := 10;
 * 등을 처리하기 위한 함수 
 * leftNode는 session에 저장되어 있는 tmplate에 값이다.
 * indirect caculate를 통해서 session tmplate에 접근한다. */
IDE_RC qsx::executePkgAssign ( qcStatement * aQcStmt )
{
    qsExecParseTree   * sParseTree;
    qsxPkgInfo        * sPkgInfo;
    qcTemplate        * sSrcTmplate;
    qcTemplate        * sDstTmplate;
    mtcStack            sAssignStack[2];
    qcuSqlSourceInfo    sSqlInfo;
    SChar             * sErrorMsg = NULL;

    sParseTree = (qsExecParseTree*)aQcStmt->myPlan->parseTree;

    IDE_TEST( qsxPkg::getPkgInfo( sParseTree->procOID,
                                  &sPkgInfo )
              != IDE_SUCCESS );

    IDE_TEST( qtc::setDatePseudoColumn( QC_PRIVATE_TMPLATE( aQcStmt ) )
              != IDE_SUCCESS );

    sSrcTmplate = QC_PRIVATE_TMPLATE( aQcStmt );

    IDE_TEST( qcuSessionPkg::searchPkgInfoFromSession( aQcStmt,
                                                       sPkgInfo,
                                                       QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                                                       QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                                                       &sDstTmplate )
              != IDE_SUCCESS );

    // exec pkg1.v1 := 10;
    sSrcTmplate = QC_PRIVATE_TMPLATE( aQcStmt );

    /* qtcCalculate_indirectColumn을 호출한다. */
    if( qtc::calculate( sParseTree->leftNode,
                        sSrcTmplate )
        != IDE_SUCCESS )
    {
        sSqlInfo.setSourceInfo( aQcStmt,
                                &(sParseTree->leftNode->position) );
        IDE_RAISE(err_pass_wrap_sqltext);
    }
    else
    {
        // stack의 두번째 부분에 rightNode의 결과값 세팅
        sAssignStack[1].column = sSrcTmplate->tmplate.stack[0].column;
        sAssignStack[1].value  = sSrcTmplate->tmplate.stack[0].value;
    }

    sSrcTmplate->tmplate.stack++;
    sSrcTmplate->tmplate.stackRemain--;

    if( qtc::calculate( sParseTree->callSpecNode,
                        sSrcTmplate )
        != IDE_SUCCESS )
    {
        sSqlInfo.setSourceInfo( aQcStmt,
                                &(sParseTree->callSpecNode->position) );
        IDE_RAISE(err_pass_wrap_sqltext);
    }
    else
    {
        // stack의 첫번째 부분에 leftNode의 결과값 세팅
        sAssignStack[0].column = sSrcTmplate->tmplate.stack[0].column;
        sAssignStack[0].value  = sSrcTmplate->tmplate.stack[0].value;
    }

    sSrcTmplate->tmplate.stack--;
    sSrcTmplate->tmplate.stackRemain++;

    if ( qsxUtil::assignValue ( QC_QMX_MEM( aQcStmt ),
                                sAssignStack[0].column, /* Source column */
                                sAssignStack[0].value,  /* Source value  */
                                sAssignStack[1].column, /* Dest column   */
                                sAssignStack[1].value,  /* Dest value    */
                                &sDstTmplate->tmplate,  /* Dest template */
                                ID_FALSE )
         != IDE_SUCCESS )
    {
        sSqlInfo.setSourceInfo( aQcStmt,
                                &(sParseTree->common.stmtPos) );
        IDE_RAISE(err_pass_wrap_sqltext);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        // To fix BUG-13208
        // system_유저가 만든 프로시져는 내부공개 안함.
        if( sParseTree->userID == QC_SYSTEM_USER_ID )
        {
            qsxEnv::setErrorCode( QC_QSX_ENV(aQcStmt) );
            qsxEnv::setErrorMessage( QC_QSX_ENV(aQcStmt) );
        }
        else
        {
            // set original error code.
            qsxEnv::setErrorCode( QC_QSX_ENV(aQcStmt) );

            (void)sSqlInfo.initWithBeforeMessage( QC_QMX_MEM(aQcStmt) );

            // BUG-42322
            if ( QCU_PSM_SHOW_ERROR_STACK == 1 )
            {
                sErrorMsg = sSqlInfo.getErrMessage();
            }
            else // QCU_PSM_SHOW_ERROR_STACK = 0
            {
                sErrorMsg = QC_QSX_ENV(aQcStmt)->mSqlErrorMessage;

                idlOS::snprintf( sErrorMsg,
                                 MAX_ERROR_MSG_LEN + 1,
                                 "\nat line %"ID_INT32_FMT"",
                                 1 );    
            }

            IDE_SET(
                ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                sSqlInfo.getBeforeErrMessage(),
                                sErrorMsg));
            (void)sSqlInfo.fini();

            // set sophisticated error message.
            qsxEnv::setErrorMessage( QC_QSX_ENV(aQcStmt) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::pkgInitWithNode( qcStatement    * aStatement,
                             mtcStack       * aStack,
                             SInt             aStackRemain,
                             qsPkgParseTree * aPlanTree,
                             qcTemplate     * aPkgTemplate )
{
    /* BUG-41847
       package 변수의 default 값으로 function을 사용할 수 있어야 합니다. */
    qsPkgParseTree  * sOriPkgPlanTree       = NULL;
    qsProcParseTree * sOriProcPlanTree      = NULL;
    SInt              sOriOthersClauseDepth = 0;

    sOriPkgPlanTree       = QSX_ENV_PKG_PLAN_TREE( QC_QSX_ENV(aStatement) );
    sOriProcPlanTree      = QSX_ENV_PLAN_TREE( QC_QSX_ENV(aStatement) );
    sOriOthersClauseDepth = aStatement->spxEnv->mOthersClauseDepth;

    IDE_TEST_RAISE( aStackRemain < 1, err_stack_overflow );

    IDE_TEST( pkgInitWithStack( aStatement,
                                aPlanTree,
                                aStack,
                                aStackRemain,
                                aPkgTemplate ) != IDE_SUCCESS );

    aStatement->spxEnv->mOthersClauseDepth          = sOriOthersClauseDepth;
    QSX_ENV_PLAN_TREE( QC_QSX_ENV(aStatement) )     = sOriProcPlanTree;
    QSX_ENV_PKG_PLAN_TREE( QC_QSX_ENV(aStatement) ) = sOriPkgPlanTree;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_stack_overflow);
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_STACK_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    aStatement->spxEnv->mOthersClauseDepth          = sOriOthersClauseDepth;
    QSX_ENV_PLAN_TREE( QC_QSX_ENV(aStatement) )     = sOriProcPlanTree;
    QSX_ENV_PKG_PLAN_TREE( QC_QSX_ENV(aStatement) ) = sOriPkgPlanTree;

    return IDE_FAILURE;
}

IDE_RC qsx::pkgInitWithStack( qcStatement    * aStatement,
                              qsPkgParseTree * aPlanTree,
                              mtcStack       * aStack,
                              SInt             aStackRemain,
                              qcTemplate     * aPkgTemplate)
{
    qsxExecutorInfo sExecInfo;

    sExecInfo.mSourceTemplateStackBuffer = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackBuffer;
    sExecInfo.mSourceTemplateStack       = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack;
    sExecInfo.mSourceTemplateStackCount  = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackCount;
    sExecInfo.mSourceTemplateStackRemain = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain;
    sExecInfo.mCurrentUserID             = QCG_GET_SESSION_USER_ID( aStatement );

    IDE_TEST( qsxExecutor::initialize( &sExecInfo,
                                       NULL,
                                       aPlanTree,
                                       aPkgTemplate,
                                       NULL ) != IDE_SUCCESS );

    IDE_TEST( qsxExecutor::execPkgPlan( &sExecInfo,                                       
                                        aStatement,
                                        aStack,
                                        aStackRemain )
              != IDE_SUCCESS );

    /* QC_PRIVATE_TMPLATE(aStatement)->
             template.stackBuffer/stack/stackCount/stackRemain 원복 */
    QC_CONNECT_TEMPLATE_STACK(    
        QC_PRIVATE_TMPLATE(aStatement),
        sExecInfo.mSourceTemplateStackBuffer,
        sExecInfo.mSourceTemplateStack,
        sExecInfo.mSourceTemplateStackCount,
        sExecInfo.mSourceTemplateStackRemain );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* QC_PRIVATE_TMPLATE(aStatement)->
             template.stackBuffer/stack/stackCount/stackRemain 원복 */
    QC_CONNECT_TEMPLATE_STACK(    
        QC_PRIVATE_TMPLATE(aStatement),
        sExecInfo.mSourceTemplateStackBuffer,
        sExecInfo.mSourceTemplateStack,
        sExecInfo.mSourceTemplateStackCount,
        sExecInfo.mSourceTemplateStackRemain );

    return IDE_FAILURE;

}

// PROJ-1073 Package
IDE_RC qsx::rebuildQsxPkgInfoPrivilege( qcStatement     * aStatement,
                                        qsOID             aPkgOID )
{
    qcuSqlSourceInfo   sqlInfo;
    qsxPkgInfo       * sPkgInfo;
    UInt               sStage = 0;
    UInt             * sGranteeID = NULL;

    IDE_TEST( qsxPkg::latchX( aPkgOID,
                              ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( qsxPkg::getPkgInfo( aPkgOID,
                                  &sPkgInfo )
              != IDE_SUCCESS );

    // BUG-38994
    // package가 참조하는 객체가 존재하지 않으면
    // 해당 객체의 상태는 invalid가 된다.
    // 객체가 invalid한 경우에는 meta able의 정보만 변경한다.
    if ( sPkgInfo->isValid == ID_TRUE )
    {
        // to fix BUG-24905
        // granteeid를 새로 build하는 경우
        // getGranteeOfPkg함수를 호출하면
        // granteeid가 새롭게 생성된다.
        // 따라서, 기존의 granteeid는 free
        sGranteeID = sPkgInfo->granteeID;

        // rebuild only privilege information
        IDE_TEST(qcmPriv::getGranteeOfPkg(
                QC_SMI_STMT(aStatement),
                QCM_OID_TO_BIGINT( sPkgInfo->pkgOID ),
                &sPkgInfo->privilegeCount,
                &sPkgInfo->granteeID )
            != IDE_SUCCESS);

        if( sGranteeID != NULL )
        {
            IDE_TEST(iduMemMgr::free(sGranteeID));
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

    sStage = 0;
    // unlatch실패하면 fatal이므로
    // 메모리 원복에 대해 고려하지 않는다.
    IDE_TEST( qsxPkg::unlatch( aPkgOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage)
    {
        case 1:
            if ( qsxPkg::unlatch( aPkgOID ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }

    return IDE_FAILURE;
}

// BUG-36203
IDE_RC qsx::makeTemplateCache( qcStatement * aStatement,
                               qsxProcInfo * aProcInfo,
                               qcTemplate  * aTemplate )
{
    IDE_TEST( aProcInfo->cache->build( aStatement,
                                       aTemplate ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-36203
IDE_RC qsx::freeTemplateCache( qsxProcInfo * aProcInfo )
{
    if( aProcInfo->cache != NULL )
    {
        IDE_TEST( aProcInfo->cache->destroy() != IDE_SUCCESS );
        aProcInfo->cache = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsx::addObject( iduList       * aList,
                void          * aObject,
                iduMemory     * aNodeMemory )
{
    iduListNode *sNode = NULL;

    IDU_FIT_POINT( "qsx::addObject::calloc::sNode",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aNodeMemory->alloc( ID_SIZEOF(iduListNode),
                                  (void **)&sNode ) != IDE_SUCCESS);
    sNode->mObj = aObject;

    IDU_LIST_ADD_FIRST( aList, sNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsx::makeNewPreparedMem( qcStatement * aStatement )
{
    iduVarMemList * sQmpMem = NULL;
    iduVarMemList * sOrgQmpMem;
    SInt            sStackCount;
    idBool          sIsInited = ID_FALSE;

    sOrgQmpMem = QC_QMP_MEM(aStatement);

    IDE_TEST( qcg::allocIduVarMemList((void**)&sQmpMem) != IDE_SUCCESS );

    sQmpMem = new (sQmpMem) iduVarMemList;

    QC_QMP_MEM(aStatement) = sQmpMem;

    IDE_ASSERT( sQmpMem != NULL );

    IDE_TEST( sQmpMem->init( IDU_MEM_QMP,
                             iduProperty::getPrepareMemoryMax())
              != IDE_SUCCESS);
    sIsInited = ID_TRUE;

    if( QC_PRIVATE_TMPLATE(aStatement) != NULL )
    {
        sStackCount = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackCount;
    }
    else
    {
        sStackCount = QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount;
    }

    IDE_TEST( qcg::allocSharedTemplate( aStatement, sStackCount )
              != IDE_SUCCESS );

    QC_PRIVATE_TMPLATE(aStatement) = QC_SHARED_TMPLATE(aStatement);

    QC_PRIVATE_TMPLATE(aStatement)->stmt = aStatement;

    IDE_TEST( aStatement->myPlan->qmpMem->getStatus(
                  &aStatement->qmpStackPosition )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sQmpMem != NULL )
    {
        if ( sIsInited == ID_TRUE )
        {
            (void)sQmpMem->destroy();
        }
        else
        {
            /* Nothing to do */
        }

        (void)qcg::freeIduVarMemList( sQmpMem );
    }
    else
    {
        /* Nothing to do */
    }

    QC_QMP_MEM(aStatement) = sOrgQmpMem;

    return IDE_FAILURE;
}

/* BUG-43113 autonomous transaction으로 psm 실행 */
IDE_RC qsx::execAT( qsxExecutorInfo * aExecInfo,
                    qcStatement     * aQcStmt,
                    mtcStack        * aStack,
                    SInt              aStackRemain )
{
    qciSwapTransContext   sContext;
    UInt                  sOriFlag      = 0;
    UInt                  sOrgNumRows   = 0;
    idBool                sIsSwapTransSuccess = ID_FALSE;
    qcuSqlSourceInfo      sSqlInfo;
    SChar               * sErrorMsg = NULL;

    /* BUG-43197 */
    sOriFlag = aQcStmt->spxEnv->mFlag;
    sOrgNumRows = QC_PRIVATE_TMPLATE(aQcStmt)->numRows; 

    // TASK-7244 Shard에서는 Autonomous transaction을 지원하지 않는다.
    IDE_TEST_RAISE( SDU_SHARD_ENABLE == 1,
                    ERR_NOT_SUPPORT_AT );

    /* set sContext for chaning transaction */
    sContext.mmStatement = QC_MM_STMT( aQcStmt );
    sContext.mmSession   = aQcStmt->session->mMmSession;
    sContext.mNewMmcTrans = NULL;
    sContext.mOriMmcTrans = NULL;
    sContext.mOriSmiStmt  = NULL;
    sContext.mIsExecSuccess   = ID_FALSE;
    /* swap transaction to alloc new transaction for autonomous transaction &
       back up for original transaction */
    IDE_TEST( qci::mSessionCallback.mSwapTransaction( &sContext , ID_TRUE )
              != IDE_SUCCESS );
    sIsSwapTransSuccess = ID_TRUE;
    aQcStmt->spxEnv->mFlag = QSX_ENV_DURING_AT;

    IDE_TEST( qsxExecutor::execPlan( aExecInfo,
                                     aQcStmt,
                                     aStack,
                                     aStackRemain )
              != IDE_SUCCESS );
    sContext.mIsExecSuccess   = ID_TRUE;
    aQcStmt->spxEnv->mFlag = sOriFlag;
    QC_PRIVATE_TMPLATE(aQcStmt)->numRows = sOrgNumRows;

    sIsSwapTransSuccess = ID_FALSE;
    /* swap transaction to original transaction */
    IDE_TEST( qci::mSessionCallback.mSwapTransaction( &sContext , ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_AT );
    {
        // TASK-7244 Shard에서는 Autonomous transaction을 지원하지 않는다.
        IDE_SET(
            ideSetErrorCode(sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                            "Autonomous transaction exists.",
                            "" ) );

        // set original error code.
        qsxEnv::setErrorCode( QC_QSX_ENV(aQcStmt) );

        (void)sSqlInfo.initWithBeforeMessage( QC_QMX_MEM(aQcStmt) );

        sErrorMsg = aExecInfo->mSqlErrorMessage;

        idlOS::snprintf( sErrorMsg,
                         MAX_ERROR_MSG_LEN + 1,
                         "\nat \"%s\"",
                         aExecInfo->mUserAndObjectName );

        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                            sSqlInfo.getBeforeErrMessage(),
                            sErrorMsg));
        (void)sSqlInfo.fini();

        // set sophisticated error message.
        qsxEnv::setErrorMessage( QC_QSX_ENV(aQcStmt) );
    }
    IDE_EXCEPTION_END;

    if ( sIsSwapTransSuccess == ID_TRUE )
    {
        aQcStmt->spxEnv->mFlag = sOriFlag;
        QC_PRIVATE_TMPLATE(aQcStmt)->numRows = sOrgNumRows;
        (void)qci::mSessionCallback.mSwapTransaction( &sContext , ID_FALSE );
    }

    return IDE_FAILURE;
}

IDE_RC qsx::executeAB( qcStatement * aQcStmt )
{
    iduMemoryStatus    sQmxMemStatus;
    UInt               sStage = 0;

    IDE_TEST( QC_QMX_MEM(aQcStmt)->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST( callProcWithStack ( aQcStmt,
                                  (qsProcParseTree*)aQcStmt->myPlan->parseTree,
                                  QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                                  QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                                  NULL,
                                  QC_PRIVATE_TMPLATE(aQcStmt) )
              != IDE_SUCCESS );

    sStage=0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        if ( QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus )
             != IDE_SUCCESS )
        {
            IDE_ERRLOG(IDE_QP_1);
        }
    }

    return IDE_FAILURE;
}

