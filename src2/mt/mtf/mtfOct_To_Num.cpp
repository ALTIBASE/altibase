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
 * $Id: mtfOct_To_Num.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfOct_To_Num;

extern mtdModule mtdInteger;

static mtcName mtfOct_To_NumFunctionName[1] = {
    { NULL, 10, (void*)"OCT_TO_NUM" }
};

static IDE_RC mtfOct_To_NumEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

mtfModule mtfOct_To_Num = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfOct_To_NumFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfOct_To_NumEstimate
};

static IDE_RC mtfOct_To_NumCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfOct_To_NumCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfOct_To_NumEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt      /* aRemain */,
                              mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // PROJ-1579 NCHAR
    IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[0], 
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

    //IDE_TEST( sModule->estimate( aStack[0].column, 0, 0, 0)
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInteger,
                                     0,
                                     0,
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

IDE_RC mtfOct_To_NumCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Oct_To_Num Calculate
 *
 * Implementation :
 *    OCT_TO_NUM ( char )
 *
 *    aStack[0] : 주어진 8진수 형태의 char를 10진수로 변환한 값
 *    aStack[1] : char (문자는 11개까지 올 수 있다.) 
 *
 *    ex) OCT_TO_NUM ('7604') ==> 32644
 *
 ***********************************************************************/
    
    mtdCharType*      sInput;
    UShort            sLength = 0;
    UShort            sIndex = 0;
    UShort            sCnt = 0;
    UInt              sResult = 0;
    UChar             sOneChar;
 
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
        sInput = (mtdCharType*)aStack[1].value;
        sLength = sInput->length;
        sCnt = 3;
        sIndex = 0;
        sResult = 0;

        IDE_TEST_RAISE ( sInput->length > 11,
                         ERR_INVALID_LENGTH );

        sIndex = 0;
        while ( sIndex < sLength )
        {
            sOneChar = sInput->value[sIndex];

            if ( sOneChar >= '0' && sOneChar <= '7' )
            {
                sResult |= ( sInput->value[sIndex] - '0' ) 
                           << ( sLength * 3 - sCnt );
            }
            else
            {
                IDE_RAISE ( ERR_INVALID_LITERAL );
            }

            // 32bit를 초과하는 33번째 비트는 버린다.
            if ( sIndex == 10 )
            {
                sCnt += 4;
            }
            else
            {
                sCnt += 3;
            }

            sIndex++;
        }

        *(mtdIntegerType*)aStack[0].value = sResult;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
 
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
