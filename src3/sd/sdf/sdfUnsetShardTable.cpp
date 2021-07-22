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
 *    SHARD_UNSET_SHARD_TABLE_LOCAL( user_name    VARCHAR,
 *                                   table_name   VARCHAR )
 *    RETURN 0
 *
 **********************************************************************/

#include <sdf.h>
#include <sdm.h>
#include <smi.h>
#include <qcg.h>
#include <qcm.h>
#include <qcmUser.h>
#include <qcmProc.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName sdfFunctionName[2] = {
    { sdfFunctionName+1, 23, (void*)"SHARD_UNSET_SHARD_TABLE" },
    { NULL,              27, (void*)"SHARD_UNSET_SHARD_PROCEDURE" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfUnsetShardTableModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};


IDE_RC sdfCalculate_UnsetShardTable( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_UnsetShardTable,
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
    const mtdModule* sModules[2] =
    {
        &mtdVarchar, // user_name
        &mtdVarchar  // table_name
    };
    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
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

IDE_RC sdfCalculate_UnsetShardTable( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *      Unset Shard Table
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/
    
    qcStatement             * sStatement;
    mtdCharType             * sUserName;
    SChar                     sUserNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sTableName;
    SChar                     sTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
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
    IDE_RC                    sRc = IDE_SUCCESS;

    sdiReplicaSetInfo         sReplicaSetInfo;
    sdiReplicaSetInfo         sFoundReplicaSetInfo;

    UInt                      sLatchState = 0;
    UInt                      sUserID;
    qsOID                     sProcOID;
    
    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    sStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    sStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

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
                                             aStack[1].value ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) )
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        //---------------------------------
        // Argument Validation
        //---------------------------------
        
        // user name
        sUserName = (mtdCharType*)aStack[1].value;
        
        IDE_TEST_RAISE( sUserName->length > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SHARD_USER_NAME_TOO_LONG );
        idlOS::strncpy( sUserNameStr,
                        (SChar*)sUserName->value,
                        sUserName->length );
        sUserNameStr[sUserName->length] = '\0';
        
        // table name
        sTableName = (mtdCharType*)aStack[2].value;
        
        IDE_TEST_RAISE( sTableName->length > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SHARD_TABLE_NAME_TOO_LONG );
        idlOS::strncpy( sTableNameStr,
                        (SChar*)sTableName->value,
                        sTableName->length );
        sTableNameStr[sTableName->length] = '\0';
        
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
        IDE_TEST( sdm::getTableInfo( QC_SMI_STMT( sStatement ),
                                     sUserNameStr,
                                     sTableNameStr,
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

                    /* Node에서 전송 중인 Unset 대상 Table의 모든 Repl을 제거한다. */
                    IDE_TEST( sdm::dropReplicationItemUsingRPSets( sStatement,
                                                                   &sFoundReplicaSetInfo,
                                                                   sUserNameStr,
                                                                   sTableNameStr,
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
                                                                   sUserNameStr,
                                                                   sTableNameStr,
                                                                   i,
                                                                   SDM_REPL_RECEIVER,
                                                                   sSMN )
                              != IDE_SUCCESS );

                    sState = 1;
                    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

                    sState = 0;
                    QC_SMI_STMT(sStatement) = sOldStmt;
                }
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

        // TASK-7244 Set shard split method to PSM info
        if ( sTableInfo.mObjectType == 'P' )
        {
            IDE_TEST( qcmUser::getUserID( NULL,
                                          QC_SMI_STMT( sStatement ),
                                          sUserNameStr,
                                          sUserName->length,
                                          &sUserID )
                      != IDE_SUCCESS );

            IDE_TEST( qcmProc::getProcExistWithEmptyByNamePtr( sStatement,
                                                               sUserID,
                                                               sTableNameStr,
                                                               sTableName->length,
                                                               &sProcOID )
                      != IDE_SUCCESS );

            if ( sProcOID != QS_EMPTY_OID )
            {
                IDE_TEST( qsxProc::latchX( sProcOID,
                                           ID_TRUE )
                          != IDE_SUCCESS );
                sLatchState = 1;

                // BUG-48911
                // A shard split method in a procedure info is lost due to a related object modified.
                //   => A 3rd argument is changed to "N" from NULL.
                if ( qciMisc::makeProcStatusInvalidAndSetShardSplitMethodByName( sStatement,
                                                                                 sProcOID,
                                                                                 (SChar*)"N" )
                     != IDE_SUCCESS )
                {
                    // BUG-48919 이미 procedure가 drop 된 경우 이를 무시한다.
                    IDE_CLEAR();
                }
            }
        }

        //---------------------------------
        // Delete Object
        //---------------------------------
        IDE_TEST( sdm::deleteObject( sStatement,
                                     (SChar*)sUserNameStr,
                                     (SChar*)sTableNameStr,
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

        /* TASK-7307 DML Data Consistency in Shard
         *   ALTER TABLE table SHARD NONE */
        if ( ( SDU_SHARD_LOCAL_FORCE != 1 ) &&
             ( sTableInfo.mObjectType == 'T' ) )
        {
            sRc = sdm::alterShardFlag( sStatement,
                                       sUserNameStr,
                                       sTableNameStr,
                                       SDI_SPLIT_NONE,
                                       ID_FALSE ); // IsNewTrans

            if ( ( sRc != IDE_SUCCESS ) &&
                 ( ideGetErrorCode() != qpERR_ABORT_QCV_NOT_EXISTS_TABLE ) )
            {
                ideLog::log(IDE_SD_0, "[DEBUG] SHARD NONE ERR-%"ID_XINT32_FMT" %s\n",
                                      ideGetErrorCode(),
                                      ideGetErrorMsg());
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    if ( sLatchState == 1 )
    {
        sLatchState = 0;
        IDE_TEST( qsxProc::unlatch( sProcOID ) != IDE_SUCCESS );
    }

    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_OPERATION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdfCalculate_UnsetShardTable",
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
    IDE_EXCEPTION( ERR_SHARD_USER_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_USER_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_SHARD_TABLE_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_TABLE_NAME_TOO_LONG ) );
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

    if ( sIsOldSessionShardMetaTouched == ID_TRUE )
    {
        sdi::setShardMetaTouched( sStatement->session );
    }
    else
    {
        sdi::unsetShardMetaTouched( sStatement->session );
    }

    if ( sLatchState == 1 )
    {
        if ( qsxProc::unlatch( sProcOID ) != IDE_SUCCESS )
        {
            (void) IDE_ERRLOG(IDE_QP_1);
        }
    }

    IDE_POP();
    
    return IDE_FAILURE;
}
