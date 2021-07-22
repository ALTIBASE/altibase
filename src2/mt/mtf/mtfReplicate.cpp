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
 * $Id: mtfReplicate.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfReplicate;

extern mtdModule mtdInteger;

static mtcName mtfReplicateFunctionName[1] = {
    { NULL, 9, (void*)"REPLICATE" }
};

static IDE_RC mtfReplicateEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfReplicate = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfReplicateFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfReplicateEstimate
};

static IDE_RC mtfReplicateCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfReplicateCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfReplicateEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt      /* aRemain */,
                             mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];
    SInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    sModules[1] = &mtdInteger;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    // PROJ-1579 NCHAR
    if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
        (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

        if( aStack[1].column->language->id == MTL_UTF16_ID )
        {
            sPrecision = (UShort)idlOS::floor(4000 / MTL_UTF16_PRECISION);
        }
        else
        {
            sPrecision = (UShort)idlOS::floor(4000 / MTL_UTF8_PRECISION);
        }
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

        sPrecision = 4000;
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
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

IDE_RC mtfReplicateCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Replicate Calculate
 *
 * Implementation :
 *    REPLICATE ( char, number )
 *
 *    aStack[0] : 첫번째 문자값을 두번째 숫자만큼 반복한 문자값
 *    aStack[1] : char ( 문자값 )
 *    aStack[2] : number ( 숫자값 )
 *    
 *    ex) REPLICATE ( 'kyn', 3 )  ==> kynkynkyn
 *
 ***********************************************************************/
    
    mtdCharType*   sResult;
    mtdCharType*   sString;
    SInt           sNum;
    SInt           sIndex; 
    UInt           sResultIndex = 0;
    UChar*         sStringIndex;
    UChar*         sStringFence;
    UShort         sStringCharCount = 0;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if ( *(mtdIntegerType*)aStack[2].value <= 0 )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult          = (mtdCharType*)aStack[0].value;
        sString          = (mtdCharType*)aStack[1].value;
        sNum             = *(mtdIntegerType*)aStack[2].value;

        IDE_TEST_RAISE( sNum < 0,
                        ERR_ARGUMENT2_VALUE_OUT_OF_RANGE ); 
        
        // BUG-25914
        // replicate 수행 결과가 결과노드의 precision을 넘을 수 없음
        if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
            (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
        {
            sStringCharCount = 0;
            sStringIndex     = sString->value;
            sStringFence     = sStringIndex + sString->length;

            while ( sStringIndex < sStringFence )
            {
                (void)aStack[1].column->language->nextCharPtr( & sStringIndex,
                                                               sStringFence );
            
                sStringCharCount++;
            }

            IDE_TEST_RAISE( sStringCharCount * sNum >
                            (SInt)(aStack[0].column->precision),
                            ERR_INVALID_LENGTH );
        }
        else
        {
            IDE_TEST_RAISE( sString->length * sNum >
                            (SInt)(aStack[0].column->precision),
                            ERR_INVALID_LENGTH );
        }

        sResultIndex = 0;
        for (sIndex = 0; sIndex < sNum; sIndex++)
        {
            idlOS::memcpy( sResult->value + sResultIndex,
                           sString->value,
                           sString->length );
            sResultIndex += sString->length;
        } 

        sResult->length = sString->length * sNum; 
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }    
    IDE_EXCEPTION( ERR_ARGUMENT2_VALUE_OUT_OF_RANGE );
    {        
        IDE_SET( ideSetErrorCode(mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE,
                                 sNum ) );
    }   
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 
