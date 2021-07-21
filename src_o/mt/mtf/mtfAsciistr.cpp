/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtfAsciistr.cpp 26126 2008-05-23 07:21:56Z copyrei $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfAsciistr;

extern mtdModule mtdVarchar;

static mtcName mtfAsciistrFunctionName[1] = {
    { NULL, 8, (void*)"ASCIISTR" }
};

static IDE_RC mtfAsciistrEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule mtfAsciistr = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfAsciistrFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfAsciistrEstimate
};

static IDE_RC mtfAsciistrCalculate( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfAsciistrCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfAsciistrEstimate( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt      /* aRemain */,
                            mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];
    SInt             sPrecision = 0;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
        (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

        // ASCII 외의 문자가 있을 경우 유니코드 포인트로 표현되기 때문에 
        // 대략 최대 5배로 precision이 증가한다.(결과 타입이 varchar이기 때문)
        // ex) '안' => '\C548'
        sPrecision = aStack[1].column->precision * 5;
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

        // ASCII 외의 문자가 있을 경우 유니코드 포인트로 표현되기 때문에 
        // 대략 최대 3배로 precision이 증가한다.(결과 타입이 varchar이기 때문)
        // ex) '안' => '\C548'
        sPrecision = aStack[1].column->precision * 3;
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarchar,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAsciistrCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Asciistr Calculate for CHAR Type
 *
 * Implementation :
 *    ASCIISTR( char )
 *
 *    aStack[0] : ASCII 이외의 문자의 경우, 
 *                해당 문자의 유니코드 포인트를 반환한다.
 *    aStack[1] : char ( 주어진 문자열 )
 *
 *    ex) ASCIISTR( 'AB안CD' ) ==> result : AB\C548CD
 *
 ***********************************************************************/
    
    mtdCharType       * sSource;
    mtdCharType       * sResult;
    UChar             * sSourceIndex;
    UChar             * sSourceFence;
    UChar             * sResultValue;
    UChar             * sResultFence;
    const mtlModule   * sSourceCharSet;
    UInt                sCharLength = 0;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sSource = (mtdCharType*)aStack[1].value;
        sResult = (mtdCharType*)aStack[0].value;

        sSourceCharSet = aStack[1].column->language;

        sSourceIndex = sSource->value;
        sSourceFence = sSourceIndex + sSource->length;

        sResultValue = sResult->value;
        sResultFence = sResultValue + aStack[0].column->precision;

        IDE_TEST( mtf::makeUFromChar( sSourceCharSet,
                                 sSourceIndex,
                                 sSourceFence,
                                 sResultValue,
                                 sResultFence,
                                 & sCharLength )
                  != IDE_SUCCESS );

        sResult->length = sCharLength;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
