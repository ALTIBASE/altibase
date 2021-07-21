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
 * $Id: qsfSetColumnStats.cpp 29282 2008-11-13 08:03:38Z mhjeong $
 *
 * Description :
 *     TASK-4990 changing the method of collecting index statistics
 *     한 Column의 통계정보를 수집한다. 
 *
 * Syntax :
 *    SET_COLUMN_STATS (
 *       ownname          VARCHAR,
 *       tabname          VARCHAR,
 *       colname          VARCHAR,
 *       partname         VARCHAR DEFAULT NULL,
 *       numdist          BIGINT  DEFAULT NULL,
 *       numnull          BIGINT  DEFAULT NULL,
 *       avgrlen          BIGINT  DEFAULT NULL,
 *       minvalue         VARCHAR DEFAULT NULL,
 *       maxvalue         VARCHAR DEFAULT NULL,
 *       no_invalidate    BOOLEAN DEFAULT FALSE )
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
#include <qdbCommon.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdChar;
extern mtdModule mtdBigint;
extern mtdModule mtdInteger;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 19, (void*)"SP_SET_COLUMN_STATS" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfSetColumnStatsModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SetColumnStats( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SetColumnStats,
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
        &mtdVarchar,
        &mtdBigint,
        &mtdBigint,
        &mtdBigint,
        &mtdVarchar,
        &mtdVarchar,
        &mtdBoolean
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

IDE_RC qsfCalculate_SetColumnStats( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     SetColumnStats
 *
 * Implementation :
 *      Argument Validation
 *      CALL SetColumnStats
 *
 ***********************************************************************/

    qcStatement          * sStatement;
    mtdCharType          * sOwnerNameValue;
    mtdCharType          * sTableNameValue;
    mtdCharType          * sColumnNameValue;
    qcmColumn            * sColumnInfo;
    UInt                   sColumnIdx;
    mtdCharType          * sPartitionNameValue;
    mtdBigintType        * sNumDistPtr;
    mtdBigintType        * sNumNullPtr;
    mtdBigintType        * sAvgCLenPtr;
    mtdCharType          * sMinValuePtr;
    mtdCharType          * sMaxValuePtr;
    mtdBooleanType       * sNoInvalidatePtr;
    UChar                  sNoInvalidate;
    mtdIntegerType       * sReturnPtr;
    UInt                   sUserID;
    smSCN                  sTableSCN;
    void                 * sTableHandle;
    qcmTableInfo         * sTableInfo;
    smiStatement         * sOldStmt;
    smiStatement         * sDummyParentStmt;
    smiStatement           sDummyStmt;
    smiTrans               sSmiTrans;
    void                 * sMmSession;
    UInt                   sSmiStmtFlag;
    UInt                   sState = 0;
    mtdDecodeFunc          sDecodeFunc;
    UInt                   sValueLen;
    IDE_RC                 sRet;
    UChar                * sSetMinValue;
    UChar                * sSetMaxValue;
    ULong                  sMinValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)];
    ULong                  sMaxValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)];
    SChar                  sTemp[SMI_MAX_MINMAX_VALUE_SIZE*3];
    SChar                * sTempPtr;
    mtcColumn              sDecodeColumn;

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
    IDE_TEST( qsf::getArg( aStack, 0, ID_TRUE,  QS_IN, (void**)&sReturnPtr )          != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 1, ID_TRUE,  QS_IN, (void**)&sOwnerNameValue )     != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 2, ID_TRUE,  QS_IN, (void**)&sTableNameValue )     != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 3, ID_TRUE,  QS_IN, (void**)&sColumnNameValue )    != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 4, ID_FALSE, QS_IN, (void**)&sPartitionNameValue ) != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 5, ID_FALSE, QS_IN, (void**)&sNumDistPtr )         != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 6, ID_FALSE, QS_IN, (void**)&sNumNullPtr )         != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 7, ID_FALSE, QS_IN, (void**)&sAvgCLenPtr )         != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 8, ID_FALSE, QS_IN, (void**)&sMinValuePtr )        != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 9, ID_FALSE, QS_IN, (void**)&sMaxValuePtr )        != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 10, ID_TRUE,  QS_IN, (void**)&sNoInvalidatePtr )    != IDE_SUCCESS );

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
                                                             SMI_TABLE_LOCK_IX,
                                                             ID_ULONG_MAX )
                  != IDE_SUCCESS );
    }
    
    IDE_TEST( qcmCache::getColumnByName( sTableInfo, 
                                         (SChar*)sColumnNameValue->value,
                                         sColumnNameValue->length,
                                         &sColumnInfo )
              != IDE_SUCCESS );
    sColumnIdx  = sColumnInfo->basicInfo->column.id;
    sDecodeFunc = sColumnInfo->basicInfo->module->decode;

    // BUG-46508
    idlOS::memcpy( &sDecodeColumn,
                   sColumnInfo->basicInfo,
                   ID_SIZEOF(mtcColumn) );

    // BUG-40290 SET_COLUMN_STATS min, max 지원
    if ( ((sDecodeColumn.module->flag & MTD_SELECTIVITY_MASK) == MTD_SELECTIVITY_ENABLE) &&
         (sMinValuePtr != NULL) )
    {
        sValueLen = SMI_MAX_MINMAX_VALUE_SIZE;

        //---------------------------------------------------
        // Min 값 컬럼의 데이타 타입으로 컨버젼
        //---------------------------------------------------

        if ( ( sDecodeColumn.module == &mtdVarchar ) ||
             ( sDecodeColumn.module == &mtdChar ) )
        {
            /* BUG-47659 SET_COLUMN_STATS 사용시 작은 따옴표( ' ) 포함된 경우 * error 발생 */
            sTempPtr = sTemp;
            IDE_TEST( qdbCommon::addSingleQuote4PartValue( (SChar *)sMinValuePtr->value,
                                                           (SInt)sMinValuePtr->length,
                                                           &sTempPtr )
                      != IDE_SUCCESS );
            
            IDE_TEST( sDecodeFunc( aTemplate,
                                   &sDecodeColumn,
                                   sMinValue,
                                   &sValueLen,
                                   (UChar*)"YYYY-MM-DD HH:MI:SS",
                                   idlOS::strlen("YYYY-MM-DD HH:MI:SS"),
                                   (UChar *)sTempPtr,
                                   idlOS::strlen( sTempPtr ),
                                   &sRet ) != IDE_SUCCESS );
        }
        else
        {
            // sRet 는 버퍼의 길이가 모잘랄 때 IDE_FAILURE 가 세팅된다.
            // 모자랄 때는 잘라서 저장한다.
            IDE_TEST( sDecodeFunc( aTemplate,
                                   &sDecodeColumn,
                                   sMinValue,
                                   &sValueLen,
                                   (UChar*)"YYYY-MM-DD HH:MI:SS",
                                   idlOS::strlen("YYYY-MM-DD HH:MI:SS"),
                                   sMinValuePtr->value,
                                   sMinValuePtr->length,
                                   &sRet ) != IDE_SUCCESS );
        }

        sSetMinValue = (UChar*)sMinValue;
    }
    else
    {
        sSetMinValue = NULL;
    }

    // BUG-40290 SET_COLUMN_STATS min, max 지원
    if ( ((sDecodeColumn.module->flag & MTD_SELECTIVITY_MASK) == MTD_SELECTIVITY_ENABLE) &&
         (sMaxValuePtr != NULL) )
    {
        sValueLen = SMI_MAX_MINMAX_VALUE_SIZE;

        //---------------------------------------------------
        // Max 값 컬럼의 데이타 타입으로 컨버젼
        //---------------------------------------------------

        if ( ( sDecodeColumn.module == &mtdVarchar ) ||
             ( sDecodeColumn.module == &mtdChar ) )
        {
            /* BUG-47659 SET_COLUMN_STATS 사용시 작은 따옴표( ' ) 포함된 경우 * error 발생 */
            sTempPtr = sTemp;
            IDE_TEST( qdbCommon::addSingleQuote4PartValue( (SChar *)sMaxValuePtr->value,
                                                           (SInt)sMaxValuePtr->length,
                                                           &sTempPtr )
                      != IDE_SUCCESS );
            
            IDE_TEST( sDecodeFunc( aTemplate,
                                   &sDecodeColumn,
                                   sMaxValue,
                                   &sValueLen,
                                   (UChar*)"YYYY-MM-DD HH:MI:SS",
                                   idlOS::strlen("YYYY-MM-DD HH:MI:SS"),
                                   (UChar *)sTempPtr,
                                   idlOS::strlen( sTempPtr ),
                                   &sRet ) != IDE_SUCCESS );
        }
        else
        {
            // sRet 는 버퍼의 길이가 모잘랄 때 IDE_FAILURE 가 세팅된다.
            // 모자랄 때는 잘라서 저장한다.
            IDE_TEST( sDecodeFunc( aTemplate,
                                   &sDecodeColumn,
                                   sMaxValue,
                                   &sValueLen,
                                   (UChar*)"YYYY-MM-DD HH:MI:SS",
                                   idlOS::strlen("YYYY-MM-DD HH:MI:SS"),
                                   sMaxValuePtr->value,
                                   sMaxValuePtr->length,
                                   &sRet ) != IDE_SUCCESS );
        }

        sSetMaxValue = (UChar*)sMaxValue;
    }
    else
    {
        sSetMaxValue = NULL;
    }

   /* PROJ-2339
    * T1은 10개의 Partition을 가지고 있는 Partitioned Table이라고 하자.
    * 다음을 수행할 때, i1의 Column NDV는?
    * iSQL> exec set_column_stats( 'sys' ,'t1' ,'i1' ,NULL ,10 );
    *
    * PROJ-2339 전에는, i1의 Column NDV가 10이 아닌 101로 설정된다. 
    * PROJ-2339를 통해 이를 수정했다. 
    */
   IDE_TEST( smiStatistics::setColumnStatsByUser( 
           (QC_SMI_STMT(sStatement))->getTrans(),
           sTableHandle,
           sColumnIdx,
           (SLong*)sNumDistPtr,
           (SLong*)sNumNullPtr,
           (SLong*)sAvgCLenPtr,
           sSetMinValue,
           sSetMaxValue,
           sNoInvalidate )
       != IDE_SUCCESS );

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

    switch ( sState )
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


