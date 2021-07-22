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
 * $Id: qsfGetTableStats.cpp 88191 2020-07-27 03:08:54Z mason.lee $
 *
 * Description :
 *
 * Syntax :
 *    GET_TABLE_STATS (
 *       ownname          VARCHAR,
 *       tabname          VARCHAR,
 *       partname         VARCHAR DEFAULT NULL,
 *       numrow           BIGINT,
 *       numblk           BIGINT,
 *       avgrlen          BIGINT,
 *       cachedpage       BIGINT,
 *       onerowreadtime   DOUBLE  DEFAULT NULL )
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
extern mtdModule mtdDouble;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 18, (void*)"SP_GET_TABLE_STATS" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );


mtfModule qsfGetTableStatsModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_GetTableStats( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_GetTableStats,
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
    const mtdModule* sModules[8] =
    {
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint,
        &mtdDouble,
    };

    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 8,
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

IDE_RC qsfCalculate_GetTableStats( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     GetTableStats
 *
 * Implementation :
 *      Argument Validation
 *      CALL GetTableStats
 *
 ***********************************************************************/

    qcStatement          * sStatement;
    mtdCharType          * sOwnerNameValue;
    mtdCharType          * sTableNameValue;
    mtdCharType          * sPartitionNameValue;
    mtdBigintType        * sNumRowPtr;
    mtdBigintType        * sNumPagePtr;
    mtdBigintType        * sAvgRLenPtr;
    mtdBigintType        * sCachePagePtr;   /* BUG-42095 */
    mtdDoubleType        * sOneRowReadTimePtr;
    mtdIntegerType       * sReturnPtr;
    UInt                   sUserID;
    smSCN                  sTableSCN;
    void                 * sTableHandle;
    qcmTableInfo         * sTableInfo;
    smiStatement         * sOldStmt;
    smiStatement         * sDummyParentStmt;
    smiStatement           sDummyStmt;
    smiTrans               sSmiTrans;
    UInt                   sSmiStmtFlag;
    UInt                   sState = 0;

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
    IDE_TEST( qsf::getArg( aStack, 0, ID_TRUE,  QS_IN, (void**)&sReturnPtr )         != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 1, ID_TRUE,  QS_IN, (void**)&sOwnerNameValue )    != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 2, ID_TRUE,  QS_IN, (void**)&sTableNameValue )    != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 3, ID_FALSE, QS_IN, (void**)&sPartitionNameValue )!= IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 4, ID_FALSE, QS_OUT, (void**)&sNumRowPtr )         != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 5, ID_FALSE, QS_OUT, (void**)&sNumPagePtr )        != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 6, ID_FALSE, QS_OUT, (void**)&sAvgRLenPtr )        != IDE_SUCCESS );
    
    /* BUG-42095 : PROJ-2281 "buffer pool에 load된 page 통계 정보 제공" 기능을 제거한다.
     * sCachePagePtr와 연결되는 aStack이 array이기 때문에 이 코드를 삭제할 경우 과도한 
     * 수정 사항이 발생할 것이기 때문에 다음 코드는 삭제 하지 않는다. */
    IDE_TEST( qsf::getArg( aStack, 7, ID_FALSE, QS_OUT, (void**)&sCachePagePtr )      != IDE_SUCCESS );
    
    IDE_TEST( qsf::getArg( aStack, 8, ID_FALSE, QS_OUT, (void**)&sOneRowReadTimePtr ) != IDE_SUCCESS );

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

    /* Table정보 획득 */
    IDE_TEST( qcmUser::getUserID( sStatement,
                                  (SChar*)sOwnerNameValue->value,
                                  sOwnerNameValue->length,
                                  &sUserID)
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfo( sStatement,
                                 sUserID,
                                 sTableNameValue->value,
                                 sTableNameValue->length,
                                 &sTableInfo,
                                 &sTableSCN,
                                 &sTableHandle )
              != IDE_SUCCESS );

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
    if( sPartitionNameValue != NULL )
    {
        IDE_TEST( qcmPartition::getPartitionInfo( 
                    sStatement,
                    sTableInfo->tableID,
                    sPartitionNameValue->value,
                    sPartitionNameValue->length,
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

    IDE_TEST( smiStatistics::getTableStatNumRow(
                sTableHandle,
                ID_FALSE,
                (QC_SMI_STMT(sStatement))->getTrans(),
                sNumRowPtr )
                != IDE_SUCCESS );

    IDE_TEST( smiStatistics::getTableStatNumPage(
                sTableHandle,
                ID_FALSE,
                sNumPagePtr )
                != IDE_SUCCESS );

    IDE_TEST( smiStatistics::getTableStatAverageRowLength(
                sTableHandle,
                sAvgRLenPtr )
                != IDE_SUCCESS );
    
    IDE_TEST( smiStatistics::getTableStatOneRowReadTime(
                sTableHandle,
                sOneRowReadTimePtr )
                != IDE_SUCCESS );

    if ( *sNumRowPtr == SMI_STAT_NULL )
    {
        *sNumRowPtr = MTD_BIGINT_NULL;
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

    if ( *sAvgRLenPtr == SMI_STAT_NULL )
    {
        *sAvgRLenPtr = MTD_BIGINT_NULL;
    }
    else
    {
        // Nothing to do.
    }

    *sCachePagePtr = MTD_BIGINT_NULL;   // BUG-42095

    if ( *sOneRowReadTimePtr == SMI_STAT_READTIME_NULL )
    {
        aStack[8].column->module->null( aStack[8].column,
                                        aStack[8].value );
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
