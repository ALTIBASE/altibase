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
 * $Id: qsfPrint.cpp 91200 2021-07-12 02:29:48Z hykim $
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


//BUG-24432 PRINTLN, PRINT 에 대해서 사용할수 있는 길이 제한에 대한 개선
#define QSF_PRINT_VARCHAR_MAX (MTD_VARCHAR_PRECISION_MAXIMUM)

static mtcName qsfFunctionName[1] = {
    { NULL, 9, (void*)"PRINT_OUT" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfPrintModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpPrint( 
                            mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpPrint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt         /* aRemain */,
                    mtcCallBack* /* aCallBack */)
{
    const mtdModule *sVarcharModule;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( & sVarcharModule, NULL )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sVarcharModule,
                                     1,
                                     QSF_PRINT_VARCHAR_MAX,
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

IDE_RC qsfCalculate_SpPrint( 
                     mtcNode*     aNode,
                     mtcStack*    aStack,
                     SInt         aRemain,
                     void*        aInfo,
                     mtcTemplate* aTemplate )
{
    qcStatement * sStatement;
    mtdCharType * sPrintString;
    SInt          sLen;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_TEST( qsxUtil::assignValue (
                  sStatement->qmxMem,
                  aStack[1].column,
                  aStack[1].value,
                  aStack[0].column,
                  aStack[0].value,
                  &QC_PRIVATE_TMPLATE(sStatement)->tmplate,
                  ID_FALSE ) != IDE_SUCCESS );

    sPrintString = (mtdCharType *) aStack[0].value ;

    /* PROJ-1438 Job Scheduler */
    /* PROJ-2451 Concurrent Exec Package */
    if ( ( QC_SMI_STMT_SESSION_IS_JOB( sStatement ) == ID_FALSE ) &&
         ( QC_SESSION_IS_INTERNAL_EXEC( sStatement ) == ID_FALSE ) &&
         ( QC_SESSION_IS_TEMP_SQL( sStatement ) == ID_FALSE ) )
    {
        if ( (QC_SMI_STMT(sStatement))->isDummy() == ID_TRUE )
        {
            if ( QCG_GET_SESSION_PRINT_OUT_ENABLE( sStatement ) == 1 )
            {
                IDE_TEST( QCG_SESSION_PRINT_TO_CLIENT( sStatement,
                                                       sPrintString->value,
                                                       sPrintString->length )
                          != IDE_SUCCESS );
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // BUG-39276
            // Trigger 또는 DML에서 사용한 PSM에서 호출한 PRINT_OUT은
            // client로 출력하지 않는다.
        }
    }
    else
    {
        if ( QC_SESSION_IS_INTERNAL_EXEC( sStatement ) == ID_TRUE )
        {
            sLen = (SInt)sPrintString->length;
            ideLog::log( IDE_QP_3, "[DBMS_CONCURRENT_EXEC : PRINT] %.*s\n", sLen, sPrintString->value );
            ideLog::log( IDE_JOB_1, "[DBMS_CONCURRENT_EXEC : PRINT] %.*s\n", sLen, sPrintString->value );
        }
        else if ( QC_SMI_STMT_SESSION_IS_JOB( sStatement ) == ID_TRUE )
        {
            sLen = (SInt)sPrintString->length;
            ideLog::log( IDE_QP_3, "[JOB : PRINT] %.*s\n", sLen, sPrintString->value );
            ideLog::log( IDE_JOB_1, "[JOB : PRINT] %.*s\n", sLen, sPrintString->value );
        }
        else if ( QC_SESSION_IS_TEMP_SQL( sStatement ) == ID_TRUE )
        {
            sLen = (SInt)sPrintString->length;
            ideLog::log( IDE_SD_17, "[Temporary SQL] %.*s\n", sLen, sPrintString->value );
        }
        else
        {
            IDE_DASSERT(0);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

