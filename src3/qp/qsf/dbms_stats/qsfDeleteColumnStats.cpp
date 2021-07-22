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
 *     BUG-38238 Interfaces for removing statistics information
 *
 * Syntax :
 *    DELETE_COLUMN_STATS (
 *       ownname          VARCHAR,
 *       tabname          VARCHAR,
 *       colname          VARCHAR,
 *       partname         VARCHAR DEFAULT NULL,
 *       cascade_part     BOOLEAN DEFAULT TRUE,
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

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 22, (void*)"SP_DELETE_COLUMN_STATS" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfDeleteColumnStatsModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_DeleteColumnStats( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_DeleteColumnStats,
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
    const mtdModule* sModules[6] =
    {
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdBoolean,
        &mtdBoolean
    };

    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 6,
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

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qsfCalculate_DeleteColumnStats( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     DeleteColumnStats
 *
 * Implementation :
 *      Argument Validation
 *      CALL smiStatistics::clearColumnStats()
 *
 ***********************************************************************/

    qcStatement          * sStatement;
    mtdCharType          * sOwnerNameValue;
    mtdCharType          * sTableNameValue;
    mtdCharType          * sColumnNameValue;
    qcmColumn            * sColumnInfo;
    UInt                   sColumnIdx;
    mtdCharType          * sPartitionNameValue;
    mtdBooleanType       * sNoInvalidatePtr;
    UChar                  sNoInvalidate;
    mtdBooleanType       * sCascadePartPtr;
    mtdIntegerType       * sReturnPtr;
    idBool                 sCascadePart;
    UInt                   sUserID;
    smSCN                  sTableSCN;
    void                 * sTableHandle;
    qcmTableInfo         * sTableInfo;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmTableInfo         * sPartInfo     = NULL;
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
    IDE_TEST( qsf::getArg( aStack, 0, ID_TRUE,  QS_IN, (void**)&sReturnPtr )          != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 1, ID_TRUE,  QS_IN, (void**)&sOwnerNameValue )     != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 2, ID_TRUE,  QS_IN, (void**)&sTableNameValue )     != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 3, ID_TRUE,  QS_IN, (void**)&sColumnNameValue )    != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 4, ID_FALSE, QS_IN, (void**)&sPartitionNameValue ) != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 5, ID_TRUE,  QS_IN, (void**)&sCascadePartPtr )     != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 6, ID_TRUE,  QS_IN, (void**)&sNoInvalidatePtr )    != IDE_SUCCESS );

    if ( (*sCascadePartPtr) == MTD_BOOLEAN_TRUE )
    {
        sCascadePart = ID_TRUE;
    }
    else
    {
        sCascadePart = ID_FALSE;
    }

    if ( (*sNoInvalidatePtr) == MTD_BOOLEAN_TRUE )
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

    /* Table 정보 획득 */
    IDE_TEST( qcmUser::getUserID( sStatement,
                                  (SChar*)sOwnerNameValue->value,
                                  sOwnerNameValue->length,
                                  &sUserID )
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

    /* Partition 이름이 입력된 경우, Partition 하나에 대해서만 고려 */
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
    else
    {
        // Nothing to do.
    }
    
    /* Column 정보 획득 */
    IDE_TEST( qcmCache::getColumnByName( sTableInfo, 
                                         (SChar*)sColumnNameValue->value,
                                         sColumnNameValue->length,
                                         &sColumnInfo )
              != IDE_SUCCESS );

    sColumnIdx = sColumnInfo->basicInfo->column.id;

    /* Table이 Partitioned Table이면, Partition List 순회하면서 통계 정보 삭제 
     * cascade_part 값이 TRUE여야 하위 Partition의 통계 정보를 삭제한다 */
    if ( ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) &&
         ( sCascadePart == ID_TRUE ) )
    {
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

        while ( sPartInfoList != NULL )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            // 통계정보를 지워야 하는 Column에 한해 삭제
            IDE_TEST( smiStatistics::clearColumnStats( (QC_SMI_STMT(sStatement))->getTrans(),
                                                       sPartInfo->tableHandle,
                                                       sColumnIdx,
                                                       sNoInvalidate )
                     != IDE_SUCCESS );

            sPartInfoList = sPartInfoList->next;
        }
    }
    else
    {
        // Nothing to do.
    }

    /* 위에서는 Partitioned Table의 각 Partition에 대해서만 통계 정보를 삭제했으므로
     * 현재 Table의 특정 Column에 대해서도 통계 정보를 삭제해야 한다. */
    IDE_TEST( smiStatistics::clearColumnStats( (QC_SMI_STMT(sStatement))->getTrans(),
                                               sTableHandle,
                                               sColumnIdx,
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


