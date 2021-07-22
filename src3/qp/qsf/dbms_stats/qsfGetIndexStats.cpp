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
 * $Id: qsfGetIndexStats.cpp 88191 2020-07-27 03:08:54Z mason.lee $
 *
 * Description :
 *
 * Syntax :
 *    GET_INDEX_STATS (
 *       ownname          VARCHAR,
 *       index            VARCHAR,
 *       partname         VARCHAR DEFAULT NULL,
 *       keycnt           BIGINT,
 *       numpage          BIGINT,
 *       numdist          BIGINT,
 *       clusfct          BIGINT,
 *       idxheight        BIGINT,
 *       cachedpage       BIGINT,
 *       avgslotcnt       BIGINT DEFAULT NULL )
 *    RETURN Integer
 *
 **********************************************************************/

#include <qdpPrivilege.h>
#include <qdpRole.h>
#include <qcmUser.h>
#include <qsf.h>
#include <qsxEnv.h>
#include <smiDef.h>
#include <smiStatistics.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdBigint;
extern mtdModule mtdInteger;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 18, (void*)"SP_GET_INDEX_STATS" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );


mtfModule qsfGetIndexStatsModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_GetIndexStats( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_GetIndexStats,
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
    const mtdModule* sModules[10] =
    {
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint
    };

    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 10,
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

IDE_RC qsfCalculate_GetIndexStats( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     SetIndexStats
 *
 * Implementation :
 *      Argument Validation
 *      CALL SetIndexStats
 *
 ***********************************************************************/

    qcStatement          * sStatement;
    mtdCharType          * sOwnerNameValue;
    mtdCharType          * sIndexNameValue;
    mtdCharType          * sPartNameValue;
    mtdBigintType        * sKeyCntPtr;
    mtdBigintType        * sNumPagePtr;
    mtdBigintType        * sNumDistPtr;
    mtdBigintType        * sClusFctPtr;
    mtdBigintType        * sIdxHeightPtr;
    mtdBigintType        * sCachedPagePtr;  /* BUG-42095 */
    mtdBigintType        * sAvgSlotCntPtr;
    mtdIntegerType       * sReturnPtr;
    UInt                   sUserID;
    UInt                   sTableID;
    UInt                   sIndexID;
    qcNamePosition         sOwnerName;
    qcNamePosition         sIndexName;
    smSCN                  sTableSCN;
    void                 * sTableHandle;
    qcmTableInfo         * sTableInfo;
    qcmIndex             * sIndexInfo    = NULL;
    smiStatement         * sOldStmt;
    smiStatement         * sDummyParentStmt;
    smiStatement           sDummyStmt;
    smiTrans               sSmiTrans;
    UInt                   sSmiStmtFlag;
    UInt                   sState = 0;
    UInt                   i;

    sStatement = ((qcTemplate*)aTemplate)->stmt;

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
    IDE_TEST( qsf::getArg( aStack, 0, ID_TRUE,  QS_IN, (void**)&sReturnPtr )       != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 1, ID_TRUE,  QS_IN, (void**)&sOwnerNameValue )  != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 2, ID_TRUE,  QS_IN, (void**)&sIndexNameValue )  != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 3, ID_FALSE, QS_IN, (void**)&sPartNameValue )   != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 4, ID_FALSE, QS_OUT, (void**)&sKeyCntPtr )       != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 5, ID_FALSE, QS_OUT, (void**)&sNumPagePtr )      != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 6, ID_FALSE, QS_OUT, (void**)&sNumDistPtr )      != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 7, ID_FALSE, QS_OUT, (void**)&sClusFctPtr )      != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 8, ID_FALSE, QS_OUT, (void**)&sIdxHeightPtr )    != IDE_SUCCESS );
    
    /* BUG-42095 : PROJ-2281 "buffer pool에 load된 page 통계 정보 제공" 기능을 제거한다. 
     * sCachePagePtr와 연결되는 aStack이 array이기 때문에 이 코드를 삭제할 경우 과도한
     * 수정 사항이 발생할 것이기 때문에 다음 코드는 삭제 하지 않는다. */ 
    IDE_TEST( qsf::getArg( aStack, 9, ID_FALSE, QS_OUT, (void**)&sCachedPagePtr )   != IDE_SUCCESS );
    
    IDE_TEST( qsf::getArg( aStack, 10, ID_FALSE, QS_OUT, (void**)&sAvgSlotCntPtr )  != IDE_SUCCESS );

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
                                         SMI_TABLE_LOCK_IS,
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

    /* Partition 하나에 대해서만 통계정보 획득 */
    if( sPartNameValue != NULL )
    {
        IDE_TEST( qcmPartition::getPartitionInfo( 
                    sStatement,
                    sTableInfo->tableID,
                    sPartNameValue->value,
                    sPartNameValue->length,
                    &sTableInfo,
                    &sTableSCN,
                    &sTableHandle )
            != IDE_SUCCESS );

        IDE_TEST( qcmPartition::validateAndLockOnePartition( sStatement,
                                                             sTableHandle,
                                                             sTableSCN,
                                                             SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                             SMI_TABLE_LOCK_IS,
                                                             ID_ULONG_MAX )
                  != IDE_SUCCESS );
    }

    for ( i = 0; i < sTableInfo->indexCount; i++)
    {
        if (sTableInfo->indices[i].indexId == sIndexID)
        {
            sIndexInfo = &sTableInfo->indices[i];

            IDE_TEST( smiStatistics::getIndexStatKeyCount(
                        sIndexInfo->indexHandle,
                        sKeyCntPtr )
                        != IDE_SUCCESS );

            IDE_TEST( smiStatistics::getIndexStatNumPage(
                        sIndexInfo->indexHandle,
                        ID_FALSE,
                        sNumPagePtr )
                        != IDE_SUCCESS );

            IDE_TEST( smiStatistics::getIndexStatNumDist(
                        sIndexInfo->indexHandle,
                        sNumDistPtr )
                        != IDE_SUCCESS );

            IDE_TEST( smiStatistics::getIndexStatClusteringFactor(
                        sIndexInfo->indexHandle,
                        sClusFctPtr )
                        != IDE_SUCCESS );

            IDE_TEST( smiStatistics::getIndexStatIndexHeight(
                        sIndexInfo->indexHandle,
                        sIdxHeightPtr )
                        != IDE_SUCCESS );

            IDE_TEST( smiStatistics::getIndexStatAvgSlotCnt(
                        sIndexInfo->indexHandle,
                        sAvgSlotCntPtr )
                        != IDE_SUCCESS );
            break;
        }
    }

    if ( *sKeyCntPtr == SMI_STAT_NULL )
    {
        *sKeyCntPtr = MTD_BIGINT_NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( *sNumPagePtr == SMI_STAT_NULL )
    {
        *sNumPagePtr = MTD_BIGINT_NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( *sNumDistPtr == SMI_STAT_NULL )
    {
        *sNumDistPtr = MTD_BIGINT_NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( *sClusFctPtr == SMI_STAT_NULL )
    {
        *sClusFctPtr = MTD_BIGINT_NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( *sIdxHeightPtr == SMI_STAT_NULL )
    {
        *sIdxHeightPtr = MTD_BIGINT_NULL;
    }
    else
    {
        // Nothing to do.
    }

    *sCachedPagePtr = MTD_BIGINT_NULL;      // BUG-42095

    if ( *sAvgSlotCntPtr == SMI_STAT_NULL )
    {
        *sAvgSlotCntPtr = MTD_BIGINT_NULL;
    }
    else
    {
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

    IDE_TEST_RAISE(sIndexInfo == NULL, ERR_NOT_EXIST_INDEX);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_INDEX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXISTS_INDEX));
    }
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
