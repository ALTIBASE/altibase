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
 * $Id: qsfGatherIndexStats.cpp 29282 2008-11-13 08:03:38Z mhjeong $
 *
 * Description :
 *     TASK-4990 changing the method of collecting index statistics
 *     한 Index의 통계정보를 수집한다. 
 *
 * Syntax :
 *    GATHER_INDEX_STATS (
 *       ownname          VARCHAR,
 *       index            VARCHAR,
 *       estimate_percent FLOAT   DEFAULT 0,
 *       degree           INTEGER DEFAULT 0,
 *       no_invalidate    BOOLEAN DEFAULT FALSE )
 *    RETURN Integer
 *
 **********************************************************************/

#include <qdpPrivilege.h>
#include <qdpRole.h>
#include <qcmUser.h>
#include <qsf.h>
#include <qcmCache.h>
#include <qdx.h>
#include <qsxEnv.h>
#include <smiDef.h>
#include <smiStatistics.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdReal;
extern mtdModule mtdInteger;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 21, (void*)"SP_GATHER_INDEX_STATS" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );


mtfModule qsfGatherIndexStatsModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_GatherIndexStats( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_GatherIndexStats,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[5] =
    {
        &mtdVarchar,
        &mtdVarchar,
        &mtdReal,
        &mtdInteger,
        &mtdBoolean
    };

    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 5,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}



IDE_RC qsfCalculate_GatherIndexStats( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     GatherIndexStats
 *
 * Implementation :
 *      Argument Validation
 *      CALL GatherIndexStats
 *
 ***********************************************************************/

    qcStatement          * sStatement;
    mtdCharType          * sOwnerNameValue;
    mtdCharType          * sIndexNameValue;
    mtdRealType          * sEstimatePercentPtr;
    mtdIntegerType       * sDegreePtr;
    mtdBooleanType       * sNoInvalidatePtr;
    UChar                  sNoInvalidate;
    mtdIntegerType       * sReturnPtr;
    UInt                   sUserID;
    UInt                   sTableID;
    UInt                   sIndexID;
    qcNamePosition         sOwnerName;
    qcNamePosition         sIndexName;
    smSCN                  sTableSCN;
    void                 * sTableHandle;
    qcmTableInfo         * sTableInfo;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qdIndexTableList     * sIndexTable   = NULL;
    qcmIndex             * sIndexInfo    = NULL;
    smiStatement         * sOldStmt;
    smiStatement         * sDummyParentStmt;
    smiStatement           sDummyStmt;
    smiTrans               sSmiTrans;
    void                 * sMmSession;
    UInt                   sSmiStmtFlag;
    UInt                   sState = 0;
    
    sStatement = ((qcTemplate*)aTemplate)->stmt;
    sMmSession = sStatement->session->mMmSession;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /**********************************************************
     * Argument Validation
     *                     aStack, Id, NotNull, InOutType, ReturnParameter
     ***********************************************************/
    IDE_TEST( qsf::getArg( aStack, 0, ID_TRUE,  QS_IN, (void**)&sReturnPtr )        != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 1, ID_TRUE,  QS_IN, (void**)&sOwnerNameValue )   != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 2, ID_TRUE,  QS_IN, (void**)&sIndexNameValue )   != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 3, ID_TRUE,  QS_IN, (void**)&sEstimatePercentPtr)!= IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 4, ID_TRUE,  QS_IN, (void**)&sDegreePtr )        != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 5, ID_TRUE,  QS_IN, (void**)&sNoInvalidatePtr )  != IDE_SUCCESS );

    if( (*sNoInvalidatePtr) == MTD_BOOLEAN_TRUE )
    {
        sNoInvalidate = ID_TRUE;
    }
    else
    {
        sNoInvalidate = ID_FALSE;
    }

    // Commit beforehand
    if ( sNoInvalidate == ID_FALSE )
    {
        IDE_TEST( qci::mSessionCallback.mCommit( sMmSession, ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // 이전 Plan들을 invalidate 시킬 필요가 없다.
        // Nothing to do.
    }

    // transaction initialze
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;

    // transaction begin
    IDE_TEST( sSmiTrans.begin( &sDummyParentStmt,
                               sStatement->mStatistics )
              != IDE_SUCCESS );
    sState = 2;

    /* Begin Statement for meta scan */
    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    qcg::getSmiStmt( sStatement, &sOldStmt   );
    qcg::setSmiStmt( sStatement, &sDummyStmt );
    sState = 3;

    IDE_TEST( sDummyStmt.begin( sStatement->mStatistics, sDummyParentStmt, sSmiStmtFlag ) != IDE_SUCCESS);
    sState = 4;

    sOwnerName.stmtText = (SChar*)sOwnerNameValue->value;
    sOwnerName.offset   = 0;
    sOwnerName.size     = sOwnerNameValue->length;

    sIndexName.stmtText = (SChar*)sIndexNameValue->value;
    sIndexName.offset   = 0;
    sIndexName.size     = sIndexNameValue->length;

    IDE_TEST( qcm::checkIndexByUser( sStatement,
                                     sOwnerName,
                                     sIndexName,
                                     &sUserID,
                                     &sTableID,
                                     &sIndexID) != IDE_SUCCESS );

    IDE_TEST(qcm::getTableInfoByID(sStatement,
                                   sTableID,
                                   &sTableInfo,
                                   &sTableSCN,
                                   &sTableHandle )
             != IDE_SUCCESS);

    IDE_TEST( smiValidateAndLockObjects( (QC_SMI_STMT(sStatement))->getTrans(),
                                         sTableHandle,
                                         sTableSCN,
                                         SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                         SMI_TABLE_LOCK_IX,
                                         ID_ULONG_MAX,
                                         ID_FALSE )         // BUG-28752 isExplicitLock
              != IDE_SUCCESS );

    // BUG-34460 Check Privilege
    IDE_TEST( qdpRole::checkDMLSelectTablePriv(
                sStatement,
                sUserID,
                sTableInfo->privilegeCount,
                sTableInfo->privilegeInfo,
                ID_FALSE,
                NULL,
                NULL )
            != IDE_SUCCESS );

    IDE_TEST( qcmCache::getIndexByID( sTableInfo,
                                      sIndexID,
                                      &sIndexInfo )
              != IDE_SUCCESS );
        
    if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        // PROJ-1624 global non-partitioned index
        if ( sIndexInfo->indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
        {
            IDE_TEST( qdx::makeAndLockIndexTable(
                          sStatement,
                          ID_TRUE,  // in execution time, no lock
                          sIndexInfo->indexTableID,
                          &sIndexTable )
                      != IDE_SUCCESS );
            
            IDE_TEST( qdx::validateAndLockIndexTableList( sStatement,
                                                          sIndexTable,
                                                          SMI_TBSLV_DDL_DML,
                                                          SMI_TABLE_LOCK_IX,
                                                          ID_ULONG_MAX )
                      != IDE_SUCCESS );

            // index table 통계정보 갱신
            IDE_TEST( smiStatistics::gatherTableStats( 
                          sStatement->mStatistics,
                          (QC_SMI_STMT(sStatement))->getTrans(),
                          sIndexTable->tableHandle,
                          *(SFloat*)sEstimatePercentPtr,
                          *(SInt*)sDegreePtr,
                          NULL, /* aTotalTableArg */
                          sNoInvalidate )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Partitioned Index이면 List 순회하면서 ..*/
            IDE_TEST( qcmPartition::getPartitionInfoList(
                          sStatement,
                          QC_SMI_STMT( sStatement),
                          QC_QMX_MEM( sStatement),
                          sTableInfo->tableID,
                          &sPartInfoList )
                      != IDE_SUCCESS );
        
            IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( sStatement,
                                                                      sPartInfoList,
                                                                      SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                      SMI_TABLE_LOCK_IX,
                                                                      ID_ULONG_MAX )
                      != IDE_SUCCESS );

            while( sPartInfoList != NULL )
            {
                IDE_TEST( qcmCache::getIndexByID( sPartInfoList->partitionInfo,
                                                  sIndexID,
                                                  &sIndexInfo )
                          != IDE_SUCCESS );
            
                IDE_TEST( smiStatistics::gatherIndexStats( 
                              sStatement->mStatistics,
                              (QC_SMI_STMT(sStatement))->getTrans(),
                              sIndexInfo->indexHandle,
                              *(SFloat*)sEstimatePercentPtr,
                              *(SInt*)sDegreePtr,
                              sNoInvalidate )
                          != IDE_SUCCESS );

                sPartInfoList = sPartInfoList->next;
            }
        }
    }
    else
    {
        IDE_TEST( smiStatistics::gatherIndexStats( 
                      sStatement->mStatistics,
                      (QC_SMI_STMT(sStatement))->getTrans(),
                      sIndexInfo->indexHandle,
                      *(SFloat*)sEstimatePercentPtr,
                      *(SInt*)sDegreePtr,
                      sNoInvalidate )
                  != IDE_SUCCESS );
    }

    // BUG-39741 Statistics Built-in Procedure doesn't make a plan invalid at all
    if ( sNoInvalidate == ID_FALSE )
    {
        if ( sTableInfo->TBSType == SMI_MEMORY_SYSTEM_DICTIONARY )
        {
            // Nothing to do.
        }
        else
        {
            IDE_TEST( qcm::touchTable( QC_SMI_STMT( sStatement ),
                                       sTableInfo->tableID,
                                       SMI_TBSLV_DDL_DML )
                      != IDE_SUCCESS);
        }
    }
    else
    {
        // 이전 Plan들을 invalidate 시킬 필요가 없다.
        // Nothing to do.
    }

    // End Statement
    sState = 3;
    IDE_TEST( sDummyStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );

    // restore
    sState = 2;
    qcg::setSmiStmt( sStatement, sOldStmt );

    // transaction commit
    sState = 1;
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );

    // transaction destroy
    sState = 0;
    IDE_TEST( sSmiTrans.destroy( sStatement->mStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 4:
            if ( sDummyStmt.end( SMI_STATEMENT_RESULT_FAILURE ) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            else
            {
                // Nothing to do.
            }
            /* fall through */
        case 3:
            qcg::setSmiStmt( sStatement, sOldStmt );
            /* fall through */
        case 2:
            sSmiTrans.rollback();
            /* fall through */
        case 1:
            sSmiTrans.destroy( sStatement->mStatistics );
            /* fall through */
        case 0:
            break;
        default:
            break;
    }
    
    return IDE_FAILURE;
}


