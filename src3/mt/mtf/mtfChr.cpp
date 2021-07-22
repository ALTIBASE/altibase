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
 * $Id: mtfChr.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfChr;

extern mtdModule mtdInteger;

static mtcName mtfChrFunctionName[1] = {
    { NULL, 3, (void*)"CHR" }
};

static IDE_RC mtfChrEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              mtcCallBack* aCallBack );

mtfModule mtfChr = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfChrFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfChrEstimate
};

static IDE_RC mtfChrCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfChrCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfChrEstimate( mtcNode*     aNode,
                       mtcTemplate* aTemplate,
                       mtcStack*    aStack,
                       SInt      /* aRemain */,
                       mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];
    const mtdModule* sModule;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdInteger;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    IDE_TEST( mtf::getCharFuncResultModule( &sModule, NULL ) != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    // aStack[0].column의 초기화
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     1,
                                     mtl::mDBCharSet->maxPrecision(1),
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

IDE_RC mtfChrCalculate( mtcNode*     aNode,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        void*        aInfo,
                        mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Chr Calculate
 *
 * Implementation :
 *    CHR( n )
 *
 *    aStack[0] : 입력된 Integer Ascii Code를 문자로 변환한 값 
 *    aStack[1] : n ( 입력된 Integer Ascii Code )
 *
 ***********************************************************************/
    
    const mtlModule* sLanguage;
    mtdCharType    * sResult;
    mtdIntegerType   sSource;
    UChar            sValue1;
    UChar            sValue2;
    UChar            sValue3;
    
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
        sResult = (mtdCharType*)aStack[0].value;
        sSource = *(mtdIntegerType*)aStack[1].value;   
        sLanguage = aStack[0].column->language;

        IDE_TEST_RAISE( sSource < 0, ERR_OVERFLOW );
        
        sValue1 = sSource & 0xFF;
        sValue2 = (sSource>>8) & 0xFF;
        sValue3 = (sSource>>16) & 0xFF;

        // precision 1
        if( sLanguage->maxPrecision(1) == 1 )
        {
            // 1바이트짜리인 경우는 첫자리만
            sResult->value[0] = sValue1;
            sResult->length = 1;
        }
        // precision 2
        else if ( sLanguage->maxPrecision(1) == 2 )
        {
            // 2바이트짜리인 경우는 숫자 크기에 따라서 다름
            if ( sLanguage->maxPrecision(1) == 2 )
            {
                // 1바이트짜리
                if( sValue2 == 0 )
                {
                    sResult->value[0] = sValue1;
                    sResult->length = 1;
                }
                // 2바이트짜리
                else
                {
                    sResult->value[0] = sValue2;
                    sResult->value[1] = sValue1;
                    sResult->length = 2;
                }
            }
        }
        else // precision 3
        {
            // 2바이트짜리일 가능성
            if( sValue3 == 0 )
            {
                // 1바이트짜리
                if( sValue2 == 0 )
                {
                    sResult->value[0] = sValue1;
                    sResult->length = 1;
                }
                // 2바이트짜리
                else
                {
                    sResult->value[0] = sValue2;
                    sResult->value[1] = sValue1;
                    sResult->length = 2;
                }
            }
            else
            {
                // 3바이트짜리
                sResult->value[0] = sValue3;
                sResult->value[1] = sValue2;
                sResult->value[2] = sValue1;
                sResult->length = 3; 
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW );
    IDE_SET(ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 
