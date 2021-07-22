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
 *    USER_SRS_    
 *
 * Syntax :
 *    DELETE_USER_SRS( SRID        INTEGER,
 *                     AUTH_NAME   VARCHAR(256))
 *
 *    RETURN 0
 *
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <qsf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxUtil.h>
#include <qcuSessionObj.h>
#include <qcmUser.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName qsfFunctionName[1] = {
    { NULL, 15, (void*)"DELETE_USER_SRS" }
};

#define QSF_AUTH_NAME_LEN               (256)
#define QSF_SPATIAL_TEXT_LEN            (2048)
 
static IDE_RC qsfEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack );

mtfModule qsfDeleteUserSrsModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0, /* default selectivity (비교 연산자 아님) */
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};

IDE_RC qsfCalculate_DeleteUserSrs( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_DeleteUserSrs,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2] =
    {
        &mtdInteger, // SRID
        &mtdVarchar  // AUTH_NAME
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

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qsfCalculate_DeleteUserSrs( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     USER_SRS_ (SPATIAL_REF_SYS) META DELETE  
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/

    qcStatement             * sStatement;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    mtdIntegerType            sSRID;
    mtdCharType             * sAuthName;
    SChar                     sAuthNameStr[QSF_AUTH_NAME_LEN + 1];
    SChar                     sTableName[QC_MAX_OBJECT_NAME_LEN + 1];
    smSCN                     sTableSCN;
    void                    * sTableHandle;
    qcmTableInfo            * sTableInfo;
    SChar                     sBuffer[QD_MAX_SQL_LENGTH];
    vSLong                    sRowCnt;
    UInt                      sTableLen;
    SInt                      sState = 0;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE )
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        //---------------------------------
        // Argument Validation
        //---------------------------------

        //SRID
        sSRID = *(mtdIntegerType*)aStack[1].value;

        // AUTH_NAME
        sAuthName = (mtdCharType*)aStack[2].value;

        IDE_TEST_RAISE( sAuthName->length > QSF_AUTH_NAME_LEN,
                        ERR_LENGTH_OVERFLOW );

        idlOS::strncpy( sAuthNameStr,
                        (SChar*)sAuthName->value,
                        sAuthName->length );

        sAuthNameStr[sAuthName->length] = '\0';
        
        //---------------------------------
        // Begin Statement for meta
        //---------------------------------

        IDU_FIT_POINT( "qsfCalculate_DeleteUserSrs::beforeQuery" );

        sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;

        sOldStmt = QC_SMI_STMT(sStatement);

        QC_SMI_STMT( sStatement ) = &sSmiStmt;

        sState = 1;

        IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                  sOldStmt,
                                  sSmiStmtFlag )
                  != IDE_SUCCESS );
        sState = 2;
        //---------------------------------
        // Begin Statement for meta
        //---------------------------------

        sTableLen = idlOS::snprintf( (SChar*)sTableName,
                                     QC_MAX_OBJECT_NAME_LEN + 1,
                                     "USER_SRS_" );

        /* Table 정보 획득 */
        IDE_TEST( qcm::getTableInfo( sStatement,
                                     QCI_SYSTEM_USER_ID,
                                     (UChar*)sTableName,
                                     sTableLen,
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
                                             ID_FALSE )
                  != IDE_SUCCESS );

        //---------------------------------
        // Insert Meta
        //---------------------------------
        if ( sAuthName->length != 0 )
        {
            idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                             "DELETE USER_SRS_ WHERE "
                             "SRID = INTEGER'%"ID_INT32_FMT"' AND "
                             "AUTH_NAME = VARCHAR'%s' ",
                             sSRID,
                             sAuthNameStr );
        }
        else
        {
            // BUT-47854 AUTH_NAME이 NULL인 경우를 찾을 수 없어서 Meta Crash에러가 발생합니다.
            idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                             "DELETE USER_SRS_ WHERE "
                             "SRID = INTEGER'%"ID_INT32_FMT"' AND "
                             "AUTH_NAME IS NULL ",
                             sSRID );
        }

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( sStatement ),
                                     sBuffer,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, NO_ROWS_DELETED );

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

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_LENGTH_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_OVERFLOW));
    }
    IDE_EXCEPTION( NO_ROWS_DELETED )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_NO_ROWS_DELETED));
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
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

    return IDE_FAILURE;
}
