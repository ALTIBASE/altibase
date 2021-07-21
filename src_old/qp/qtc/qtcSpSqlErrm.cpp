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
 * $Id: qtcSpSqlErrm.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxCursor.h>

static mtcName mtfFunctionName[1] = {
    { NULL, 9, (void*)"SPSQLERRM" }
};

static IDE_RC mtfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qtc::spSqlErrmModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_DML_UNUSABLE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    mtfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfEstimate
};


IDE_RC mtfCalculate_SpSqlErrm( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCalculate_SpSqlErrm,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC mtfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt         /* aRemain */,
                    mtcCallBack* /* aCallBack */ )
{
#define IDE_FN "IDE_RC mtfEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule *sVarcharModule;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::getCharFuncResultModule( & sVarcharModule, NULL )
              != IDE_SUCCESS );

    /*
    IDE_TEST( sVarcharModule->estimate( aStack[0].column, 1, 4000, 0 )
              != IDE_SUCCESS );
    */

    // To Fix BUG-12564
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sVarcharModule,
                                     1,
                                     4000,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE((aNode->lflag & MTC_NODE_BIND_MASK) != MTC_NODE_BIND_ABSENT,
                    ERR_INVALID_FUNCTION_ARGUMENT_HOSTVAR );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT_HOSTVAR );
    IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_NO_HOSTVAR_ALLOWED));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfCalculate_SpSqlErrm( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfCalculate_SpSqlErrm"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement         * sStatement;
    UInt                  sErrmLen = 0;
    SInt                  sErrmSrcLen = 0;
    mtdCharType         * sErrmCharType;

    sStatement    = ((qcTemplate*)aTemplate)->stmt ;
   
    // fix BUG-32565
    QSX_FIX_ERROR_MESSAGE( sStatement->spxEnv );    
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    sErrmSrcLen = idlOS::strlen( QSX_ENV_ERROR_MESSAGE( sStatement->spxEnv ) );

    if ( aStack[0].column->precision > sErrmSrcLen )
    {
        sErrmLen = sErrmSrcLen;
    }
    else
    {
        sErrmLen = aStack[0].column->precision;
    }
    
    
    sErrmCharType = (mtdCharType*) aStack[0].value;
    sErrmCharType->length = sErrmLen;
    idlOS::memcpy( sErrmCharType->value,
                   QSX_ENV_ERROR_MESSAGE( sStatement->spxEnv ),
                   sErrmLen );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}
 
