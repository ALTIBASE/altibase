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
 * $Id: qsfGetColumnStats.cpp 88191 2020-07-27 03:08:54Z mason.lee $
 *
 * Description :
 *
 * Syntax :
 *    GET_COLUMN_STATS (
 *       ownname          VARCHAR,
 *       tabname          VARCHAR,
 *       colname          VARCHAR,
 *       partname         VARCHAR DEFAULT NULL,
 *       numdist          BIGINT,
 *       numnull          BIGINT,
 *       avgrlen          BIGINT,
 *       minvalue         VARCHAR,
 *       maxvalue         VARCHAR )
 *    RETURN Integer
 *
 **********************************************************************/

#include <qdpPrivilege.h>
#include <qdpRole.h>
#include <qcmCache.h>
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
    { NULL, 19, (void*)"SP_GET_COLUMN_STATS" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfGetColumnStatsModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_GetColumnStats( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_GetColumnStats,
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
    const mtdModule* sModules[9] =
    {
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint,
        &mtdVarchar,
        &mtdVarchar
    };

    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 9,
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

IDE_RC qsfCalculate_GetColumnStats( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     GetColumnStats
 *
 * Implementation :
 *      Argument Validation
 *      CALL GetColumnStats
 *
 ***********************************************************************/

    qcStatement          * sStatement;
    mtdCharType          * sOwnerNameValue;
    mtdCharType          * sTableNameValue;
    mtdCharType          * sColumnNameValue;
    mtdCharType          * sPartitionNameValue;
    qcmColumn            * sColumnInfo;
    UInt                   sColumnIdx;
    mtdBigintType        * sNumDistPtr;
    mtdBigintType        * sNumNullPtr;
    mtdBigintType        * sAvgCLenPtr;
    mtdCharType          * sMinValuePtr;
    mtdCharType          * sMaxValuePtr;
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
    mtdEncodeFunc          sEncodeFunc;
    UInt                   sValueLen;
    IDE_RC                 sRet;
    ULong                  sMinValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)];
    ULong                  sMaxValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)];

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
    IDE_TEST( qsf::getArg( aStack, 0, ID_TRUE,  QS_IN,  (void**)&sReturnPtr )          != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 1, ID_TRUE,  QS_IN,  (void**)&sOwnerNameValue )     != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 2, ID_TRUE,  QS_IN,  (void**)&sTableNameValue )     != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 3, ID_TRUE,  QS_IN,  (void**)&sColumnNameValue )    != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 4, ID_FALSE, QS_IN,  (void**)&sPartitionNameValue ) != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 5, ID_FALSE, QS_OUT, (void**)&sNumDistPtr )         != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 6, ID_FALSE, QS_OUT, (void**)&sNumNullPtr )         != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 7, ID_FALSE, QS_OUT, (void**)&sAvgCLenPtr )         != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 8, ID_FALSE, QS_OUT, (void**)&sMinValuePtr )        != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 9, ID_FALSE, QS_OUT, (void**)&sMaxValuePtr )        != IDE_SUCCESS );

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

    IDE_TEST( qcmCache::getColumnByName( sTableInfo, 
                                         (SChar*)sColumnNameValue->value,
                                         sColumnNameValue->length,
                                         &sColumnInfo )
              != IDE_SUCCESS );
    sColumnIdx  = (sColumnInfo->basicInfo->column.id & SMI_COLUMN_ID_MASK);
    sEncodeFunc = sColumnInfo->basicInfo->module->encode;

    //------------------------------------------------
    // 통계정보 수집
    //------------------------------------------------

    IDE_TEST( smiStatistics::getColumnStatNumDist(
                sTableHandle,
                sColumnIdx,
                sNumDistPtr )
              != IDE_SUCCESS );

    IDE_TEST( smiStatistics::getColumnStatNumNull(
                sTableHandle,
                sColumnIdx,
                sNumNullPtr )
              != IDE_SUCCESS );

    IDE_TEST( smiStatistics::getColumnStatAverageColumnLength(
                sTableHandle,
                sColumnIdx,
                sAvgCLenPtr )
              != IDE_SUCCESS );

    // BUG-40290 GET_COLUMN_STATS min, max 지원
    if ( ((sColumnInfo->basicInfo->module->flag & MTD_SELECTIVITY_MASK) == MTD_SELECTIVITY_ENABLE) &&
         (smiStatistics::isValidColumnStat( sTableHandle, sColumnIdx ) == ID_TRUE) )
    {
        // min, max
        IDE_TEST( smiStatistics::getColumnStatMin(
                        sTableHandle,
                        sColumnIdx,
                        sMinValue )
                    != IDE_SUCCESS );

        IDE_TEST( smiStatistics::getColumnStatMax(
                        sTableHandle,
                        sColumnIdx,
                        sMaxValue )
                    != IDE_SUCCESS );

        //---------------------------------------------------
        // Min 값 Char 타입으로 컨버젼
        //---------------------------------------------------
        sValueLen = aStack[8].column->column.size;

        // sRet 는 버퍼의 길이가 모잘랄 때 IDE_FAILURE 가 세팅된다.
        // MIN, MAX stat은 40byte 만큼 있기때문에 문제가 되지 않는다.

        IDE_TEST( sEncodeFunc( sColumnInfo->basicInfo,
                               sMinValue,
                               sColumnInfo->basicInfo->column.size,
                               (UChar*)"YYYY-MM-DD HH:MI:SS",
                               idlOS::strlen("YYYY-MM-DD HH:MI:SS"),
                               sMinValuePtr->value,
                               &sValueLen,
                               &sRet ) != IDE_SUCCESS );
        sMinValuePtr->length = sValueLen;


        //---------------------------------------------------
        // Max 값 Char 타입으로 컨버젼
        //---------------------------------------------------
        sValueLen = aStack[9].column->column.size;

        IDE_TEST( sEncodeFunc( sColumnInfo->basicInfo,
                               sMaxValue,
                               sColumnInfo->basicInfo->column.size,
                               (UChar*)"YYYY-MM-DD HH:MI:SS",
                               idlOS::strlen("YYYY-MM-DD HH:MI:SS"),
                               sMaxValuePtr->value,
                               &sValueLen,
                               &sRet ) != IDE_SUCCESS );
        sMaxValuePtr->length = sValueLen;
    }
    else
    {
        sMinValuePtr->length = 0;
        sMaxValuePtr->length = 0;
    }

    if ( *sNumDistPtr == SMI_STAT_NULL )
    {
        *sNumDistPtr = MTD_BIGINT_NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( *sNumNullPtr == SMI_STAT_NULL )
    {
        *sNumNullPtr = MTD_BIGINT_NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( *sAvgCLenPtr == SMI_STAT_NULL )
    {
        *sAvgCLenPtr = MTD_BIGINT_NULL;
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
