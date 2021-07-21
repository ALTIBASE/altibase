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
 * $Id: mtfNth_element.cpp 61652 2013-10-23 07:54:56Z sparrow $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNth_element;

extern mtdModule mtdList;
extern mtdModule mtdInteger;
extern mtdModule mtdBoolean;

static mtcName mtfNth_elementFunctionName[1] = {
    { NULL, 11, (void*)"NTH_ELEMENT" }
};

static IDE_RC mtfNth_elementEstimate( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

mtfModule mtfNth_element = {
    1|MTC_NODE_OPERATOR_FUNCTION|
    MTC_NODE_PRINT_FMT_PREFIX_PA,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfNth_elementFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNth_elementEstimate
};

IDE_RC mtfNth_elementCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNth_elementCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNth_elementCalculateList( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfExecuteList = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNth_elementCalculateList,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNth_elementEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         /*aRemain*/,
                               mtcCallBack* aCallBack )
{
    const mtdModule  * sModules[1] = { &mtdInteger };
    mtcStack         * sStack;
    UInt               sCount;
    UInt               sFence;
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                      ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    IDE_TEST_RAISE( aStack[1].column->module == &mtdBoolean,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( aStack[1].column->module == &mtdList )
    {
        IDE_TEST_RAISE( aStack[1].column->precision <= 0,
                        ERR_INVALID_FUNCTION_ARGUMENT );
        
        sStack = (mtcStack*)aStack[1].value;

        // list의 모든 element의 type이 같아야 한다.
        for( sCount = 0, sFence = aStack[1].column->precision;
             sCount < sFence;
             sCount++ )
        {
            // BUG-41310
            // list의 argument로 rowtype / recordtype / array / lob 이 올 수 없다.
            IDE_TEST_RAISE( ( sStack[sCount].column->module->id == MTD_ROWTYPE_ID ) ||
                            ( sStack[sCount].column->module->id == MTD_RECORDTYPE_ID ) ||
                            ( sStack[sCount].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) ||
                            ( sStack[sCount].column->module->id == MTD_BLOB_ID ) ||
                            ( sStack[sCount].column->module->id == MTD_CLOB_ID ) ||
                            ( sStack[sCount].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                            ( sStack[sCount].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                            ( sStack[sCount].column->module->id == MTD_GEOMETRY_ID ),
                            ERR_CONVERSION_NOT_APPLICABLE );
            
            IDE_TEST_RAISE( ( sStack[0].column->module->id !=
                              sStack[sCount].column->module->id ) ||
                            ( sStack[0].column->language->id !=
                              sStack[sCount].column->language->id ) ||
                            ( sStack[0].column->precision !=
                              sStack[sCount].column->precision ) ||
                            ( sStack[0].column->scale !=
                              sStack[sCount].column->scale ),
                            ERR_CONVERSION_NOT_APPLICABLE );
        }
        
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteList;
        
        // 첫번째 element로 초기화한다.
        mtc::initializeColumn( aStack[0].column, sStack[0].column );

        // BUG-39511 list를 반환하는 경우 value도 설정한다.
        if ( aStack[0].column->module == &mtdList )
        {
            aStack[0].value = sStack[0].value;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // BUG-41310
        // list의 argument로 rowtype / recordtype / array / lob 이 올 수 없다.
        IDE_TEST_RAISE( ( aStack[1].column->module->id == MTD_ROWTYPE_ID ) ||
                        ( aStack[1].column->module->id == MTD_RECORDTYPE_ID ) ||
                        ( aStack[1].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) ||
                        ( aStack[1].column->module->id == MTD_BLOB_ID ) ||
                        ( aStack[1].column->module->id == MTD_CLOB_ID ) ||
                        ( aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                        ( aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                        ( aStack[1].column->module->id == MTD_GEOMETRY_ID ),
                        ERR_CONVERSION_NOT_APPLICABLE );

        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
        
        mtc::initializeColumn( aStack[0].column, aStack[1].column );
    }
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments->next,
                                        aTemplate,
                                        aStack + 2,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNth_elementCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
    mtdIntegerType  sValue;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sValue = *(mtdIntegerType*)aStack[2].value;
    
    if ( sValue == MTD_INTEGER_NULL )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // 0 혹은 1부터 시작한다.
        if ( sValue == 0 )
        {
            sValue++;
        }
        else
        {
            // Nothing to do.
        }

        // 1이나 -1이 아니면 에러
        IDE_TEST_RAISE( ( sValue != -1 ) && ( sValue != 1 ),
                        ERR_ARGUMENT_NOT_APPLICABLE );
        
        IDE_DASSERT( aStack[0].column->module->id ==
                     aStack[1].column->module->id );
        
        if ( aStack[1].column->module->isNull( aStack[1].column,
                                               aStack[1].value ) == ID_TRUE )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            idlOS::memcpy( aStack[0].value,
                           aStack[1].value,
                           aStack[1].column->module->actualSize( aStack[1].column,
                                                                 aStack[1].value ) );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNth_elementCalculateList( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
    mtdIntegerType     sValue;
    mtcStack         * sStack;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sValue = *(mtdIntegerType*)aStack[2].value;
    
    if ( sValue == MTD_INTEGER_NULL )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // 0 혹은 1부터 시작한다.
        if ( sValue == 0 )
        {
            sValue++;
        }
        else
        {
            // Nothing to do.
        }

        // -1이면 끝에서 첫번째
        if ( sValue < 0 )
        {
            sValue += aStack[1].column->precision + 1;
        }
        else
        {
            // Nothing to do.
        }
        
        // 범위를 넘으면 에러
        IDE_TEST_RAISE( ( sValue <= 0 ) || ( sValue > aStack[1].column->precision ),
                        ERR_ARGUMENT_NOT_APPLICABLE );
        
        sStack = ((mtcStack*)aStack[1].value) + sValue - 1;
        
        IDE_DASSERT( aStack[0].column->module->id ==
                     sStack->column->module->id );
        
        if ( sStack->column->module->isNull( sStack->column,
                                             sStack->value ) == ID_TRUE )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            idlOS::memcpy( aStack[0].value,
                           sStack->value,
                           sStack->column->module->actualSize( sStack->column,
                                                               sStack->value ) );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
