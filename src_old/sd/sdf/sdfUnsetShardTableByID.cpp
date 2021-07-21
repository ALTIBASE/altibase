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
 * $Id$
 *
 * Description :
 *
 *     ALTIBASE SHARD management function
 *
 * Syntax :
 *    SHARD_UNSET_SHARD_TABLE_BY_ID_LOCAL( table_id INTEGER )
 *    RETURN 0
 *
 **********************************************************************/

#include <sdf.h>
#include <sdm.h>
#include <smi.h>
#include <qcg.h>

extern mtdModule mtdInteger;

static mtcName sdfFunctionName[2] = {
    { sdfFunctionName+1, 29, (void*)"SHARD_UNSET_SHARD_TABLE_BY_ID" },
    { NULL,              33, (void*)"SHARD_UNSET_SHARD_PROCEDURE_BY_ID" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfUnsetShardTableByIDModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};


IDE_RC sdfCalculate_UnsetShardTableByID( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_UnsetShardTableByID,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC sdfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1] =
    {
        &mtdInteger // table id
    };
    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = sdfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfCalculate_UnsetShardTableByID( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *      unset shard table by id
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/
    
    qcStatement             * sStatement;
    mtdIntegerType            sShardID;
    UInt                      sTableRowCnt = 0;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    idBool                    sIsOldSessionShardMetaTouched = ID_FALSE;

    ULong                     sSMN = ID_ULONG(0);
    idBool                    sIsTableFound = ID_FALSE;
    sdiTableInfo              sTableInfo;
    sdiLocalMetaInfo          sLocalMetaInfo;
    UInt                      i = 0;
    SChar                     sBackupTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    sdiReplicaSetInfo         sReplicaSetInfo;
    sdiReplicaSetInfo         sFoundReplicaSetInfo;

    UInt                      sUserID = QC_EMPTY_USER_ID;
    SChar                     sSqlStr[SDF_QUERY_LEN];
    
    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    sStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    sStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

    /* BUG-47623 샤드 메타 변경에 대한 trc 로그중 commit 로그를 작성하기전에 DASSERT 로 죽는 경우가 있습니다. */
    if ( ( sStatement->session->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
         QC_SESSION_SHARD_META_TOUCH_TRUE )
    {
        sIsOldSessionShardMetaTouched = ID_TRUE;
    }

    // BUG-46366
    IDE_TEST_RAISE( ( QC_SMI_STMT(sStatement)->getTrans() == NULL ) ||
                    ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DML ) == QCI_STMT_MASK_DML ) ||
                    ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DCL ) == QCI_STMT_MASK_DCL ),
                    ERR_INSIDE_QUERY );

    // Check Privilege
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_ID(sStatement) != QCI_SYS_USER_ID,
                    ERR_NO_GRANT );

    if ( SDU_SHARD_LOCAL_FORCE != 1 )
    {
        /* Shard Local Operation은 internal 에서만 수행되어야 한다.  */
        IDE_TEST_RAISE( ( QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( sStatement ) != ID_TRUE ) &&
                        ( ( sStatement->session->mQPSpecific.mFlag & QC_SESSION_ALTER_META_MASK )
                             != QC_SESSION_ALTER_META_ENABLE),
                        ERR_INTERNAL_OPERATION );
    }

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_TRUE ) )
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        //---------------------------------
        // Argument Validation
        //---------------------------------
        
        // shard ID
        sShardID = *(mtdIntegerType*)aStack[1].value;

       IDE_TEST( sdi::getIncreasedSMNForMetaNode( ( QC_SMI_STMT( sStatement ) )->getTrans() , 
                                                   &sSMN ) 
                  != IDE_SUCCESS );

        sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
        sOldStmt                = QC_SMI_STMT(sStatement);
        QC_SMI_STMT(sStatement) = &sSmiStmt;
        sState = 1;

        IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                  sOldStmt,
                                  sSmiStmtFlag )
                  != IDE_SUCCESS );
        sState = 2;

        // Shard 등록된 Procedure 혹은 Partitioned Table만 대상으로한다.
        IDE_TEST( sdm::getTableInfoByID( QC_SMI_STMT( sStatement ),
                                         sShardID,
                                         sSMN, //sMetaNodeInfo.mShardMetaNumber,
                                         &sTableInfo,
                                         &sIsTableFound )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                        ERR_NOT_EXIST_TABLE );

        sState = 1;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

        sState = 0;
        QC_SMI_STMT(sStatement) = sOldStmt;

        if ( SDU_SHARD_LOCAL_FORCE != 1 )
        {
            /* Table일 경우 RP제거와 _BAK_Table 제거도 해주어야 한다. */
            if ( sTableInfo.mObjectType == 'T' )
            {
                IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

                idlOS::snprintf( sBackupTableNameStr,
                                 QC_MAX_OBJECT_NAME_LEN+1,
                                 "%s%s",
                                 SDI_BACKUP_TABLE_PREFIX,
                                 sTableInfo.mObjectName );

                //---------------------------------
                // Drop Replication
                //---------------------------------
                for ( i = 0 ; i < sLocalMetaInfo.mKSafety; i++ )
                {
                    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
                    sOldStmt                = QC_SMI_STMT(sStatement);
                    QC_SMI_STMT(sStatement) = &sSmiStmt;
                    sState = 1;

                    IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                              sOldStmt,
                                              sSmiStmtFlag )
                              != IDE_SUCCESS );
                    sState = 2;

                    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( sStatement ),
                                                                      &sReplicaSetInfo,
                                                                      sSMN ) 
                              != IDE_SUCCESS );

                    /* 내가 보내는 ReplName을 찾아야 한다. */
                    IDE_TEST( sdm::findSendReplInfoFromReplicaSet( &sReplicaSetInfo,
                                                                   sLocalMetaInfo.mNodeName,
                                                                   &sFoundReplicaSetInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( sdm::dropReplicationItemUsingRPSets( sStatement,
                                                                   &sFoundReplicaSetInfo,
                                                                   sTableInfo.mUserName,
                                                                   sTableInfo.mObjectName,
                                                                   i,
                                                                   SDM_REPL_SENDER,
                                                                   sSMN )
                              != IDE_SUCCESS );

                    /* 내가 받는 ReplName를 찾아야 한다. */
                    IDE_TEST( sdm::findRecvReplInfoFromReplicaSet( &sReplicaSetInfo,
                                                                   sLocalMetaInfo.mNodeName,
                                                                   i,
                                                                   &sFoundReplicaSetInfo )
                              != IDE_SUCCESS );

                    /* Node에서 받는 중인 Unset 대상 Table의 모든 Repl을 제거한다 */
                    IDE_TEST( sdm::dropReplicationItemUsingRPSets( sStatement,
                                                                   &sFoundReplicaSetInfo,
                                                                   sTableInfo.mUserName,
                                                                   sTableInfo.mObjectName,
                                                                   i,
                                                                   SDM_REPL_RECEIVER,
                                                                   sSMN )
                              != IDE_SUCCESS );

                    sState = 1;
                    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

                    sState = 0;
                    QC_SMI_STMT(sStatement) = sOldStmt;
                }

                //---------------------------------
                // Drop _BAK_ Table
                //---------------------------------
                idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                 "drop table %s.%s ",
                                 sTableInfo.mUserName,
                                 sBackupTableNameStr );

                sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
                sOldStmt                = QC_SMI_STMT(sStatement);
                QC_SMI_STMT(sStatement) = &sSmiStmt;
                sState = 1;

                IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                          sOldStmt,
                                          sSmiStmtFlag )
                          != IDE_SUCCESS );
                sState = 2;

                IDE_TEST( qciMisc::getUserIdByName( sTableInfo.mUserName, &sUserID ) != IDE_SUCCESS );
                ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s", sSqlStr);
                IDE_TEST( qciMisc::runRollbackableInternalDDL( sStatement,
                                                               QC_SMI_STMT( sStatement ),
                                                               sUserID,
                                                               sSqlStr )
                          != IDE_SUCCESS );

                sState = 1;
                IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

                sState = 0;
                QC_SMI_STMT(sStatement) = sOldStmt;
            }
        }

        //---------------------------------
        // Begin Statement for meta
        //---------------------------------
        sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
        sOldStmt                = QC_SMI_STMT(sStatement);
        QC_SMI_STMT(sStatement) = &sSmiStmt;
        sState = 1;

        IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                  sOldStmt,
                                  sSmiStmtFlag )
                  != IDE_SUCCESS );
        sState = 2;

        //---------------------------------
        // Delete Object
        //---------------------------------
        IDE_TEST( sdm::deleteObjectByID( sStatement,
                                         (UInt)sShardID,
                                         &sTableRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sTableRowCnt == 0,
                        ERR_INVALID_SHARD_TABLE );

        //---------------------------------
        // End Statement
        //---------------------------------
        sState = 1;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

        sState = 0;
        QC_SMI_STMT(sStatement) = sOldStmt;
    }
    
    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_OPERATION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdfCalculate_UnsetShardTableByID",
                                  "SHARD_INTERNAL_LOCAL_OPERATION is not 1" ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }

    IDE_EXCEPTION( ERR_INSIDE_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_PSM_INSIDE_QUERY ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_TABLE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_TABLE ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NO_GRANT ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_SD_1);
            }
            else
            {
                /* Nothing to do */
            }
        case 1:
            QC_SMI_STMT(sStatement) = sOldStmt;
        default:
            break;
    }

    /* BUG-47623 샤드 메타 변경에 대한 trc 로그중 commit 로그를 작성하기전에 DASSERT 로 죽는 경우가 있습니다. */
    if ( sIsOldSessionShardMetaTouched == ID_TRUE )
    {
        sdi::setShardMetaTouched( sStatement->session );
    }
    else
    {
        sdi::unsetShardMetaTouched( sStatement->session );
    }

    IDE_POP();
    
    return IDE_FAILURE;
}
