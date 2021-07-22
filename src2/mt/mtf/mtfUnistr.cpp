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
 * $Id: mtfUnistr.cpp 26126 2008-05-23 07:21:56Z copyrei $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfUnistr;

extern mtdModule mtdNvarchar;

static mtcName mtfUnistrFunctionName[1] = {
    { NULL, 6, (void*)"UNISTR" }
};

static IDE_RC mtfUnistrEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfUnistr = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfUnistrFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfUnistrEstimate
};

static IDE_RC mtfUnistrCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfUnistrCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfUnistrEstimate( mtcNode*     aNode,
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

    // NCHAR 타입을 인자로 받을 수 없다.
    IDE_TEST_RAISE( (aStack[1].column->module->id == MTD_NCHAR_ID) || 
                    (aStack[1].column->module->id == MTD_NVARCHAR_ID),
                    ERR_CONVERSION_NOT_APPLICABLE );

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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    // 내셔널 캐릭터가 UTF8이면 *2, UTF16이면 인자의 precision보다 작다.
    if( mtl::mNationalCharSet->id == MTL_UTF8_ID )
    {
        sPrecision = aStack[1].column->precision * 2;
    }
    else
    {
        sPrecision = aStack[1].column->precision;
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNvarchar,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfUnistrCalculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Unistr Calculate
 *
 * Implementation :
 *    UNISTR( char )
 *
 *    aStack[0] : 유니코드 포인트일 경우, 내셔널 캐릭터에 맞게 변환하여 
 *                해당 문자를 반환한다.
 *    aStack[1] : char ( 주어진 문자열 )
 *
 *    ex) UNISTR( 'AB\C548CD' ) ==> result : AB안CD
 *
 ***********************************************************************/
    
    mtdCharType       * sSource;
    mtdNcharType      * sResult;
    UChar             * sSourceIndex;
    UChar             * sSourceFence;
    UChar             * sResultValue;
    UChar             * sResultFence;
    const mtlModule   * sSourceCharSet;
    const mtlModule   * sResultCharSet;
    UInt                sNcharCnt = 0;
    
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
        sResult = (mtdNcharType*)aStack[0].value;

        sSourceCharSet = aStack[1].column->language;
        sResultCharSet = aStack[0].column->language;

        sSourceIndex = sSource->value;
        sSourceFence = sSourceIndex + sSource->length;

        sResultValue = sResult->value;
        sResultFence = sResultValue + 
                   sResultCharSet->maxPrecision(aStack[0].column->precision);

        IDE_TEST( mtdNcharInterface::toNchar4UnicodeLiteral( sSourceCharSet,
                                                             sResultCharSet,
                                                             sSourceIndex,
                                                             sSourceFence,
                                                             & sResultValue,
                                                             sResultFence,
                                                             & sNcharCnt )
                  != IDE_SUCCESS );

        sResult->length = sResultValue - sResult->value;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
