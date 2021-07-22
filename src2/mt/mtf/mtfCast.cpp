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
 * $Id: mtfCast.cpp 19655 2007-06-27 01:56:25Z sungminee $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfCast;

static mtcName mtfCastFunctionName[1] = {
    { NULL, 4, (void*)"CAST" }
};

static IDE_RC mtfCastEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

mtfModule mtfCast = {
    1|MTC_NODE_OPERATOR_FUNCTION|
        MTC_NODE_PRINT_FMT_MISC,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfCastFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfCastEstimate
};

static IDE_RC mtfCastCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCastCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfCastEstimate( mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt      /* aRemain */,
                        mtcCallBack* aCallBack )
{
    mtcNode      * sNode;
    mtcColumn    * sColumn;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_ASSERT( aNode->funcArguments != NULL );

    sNode   = aNode->funcArguments;
    sColumn = aTemplate->rows[sNode->table].columns + sNode->column;

    // PROJ-2002 Column Security
    // 보안 타입으로는 cast 연산을 수행할 수 없다.
    IDE_TEST_RAISE( (sColumn->module->id == MTD_ECHAR_ID) ||
                    (sColumn->module->id == MTD_EVARCHAR_ID) ||
                    (sColumn->module->id == MTD_UNDEF_ID),
                    ERR_CONVERSION_NOT_APPLICABLE );

    // BUG-23102
    // mtcColumn으로 초기화한다.
    mtc::initializeColumn( aStack[0].column, sColumn );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        &(aStack[0].column->module) )
              != IDE_SUCCESS );

    // BUG-43858 인자가 undef type이더라도 cast연산자는 무시한다.
    if ( ( aNode->lflag & MTC_NODE_UNDEF_TYPE_MASK )
         == MTC_NODE_UNDEF_TYPE_EXIST )
    {
        aNode->lflag &= ~MTC_NODE_UNDEF_TYPE_MASK;
        aNode->lflag |= MTC_NODE_UNDEF_TYPE_ABSENT;
    }
    else
    {
        // Nothing to do.
    }

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

IDE_RC mtfCastCalculate( mtcNode*     aNode,
                         mtcStack*    aStack,
                         SInt         aRemain,
                         void*        aInfo,
                         mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : 
 *
 * Implementation :
 *    CAST( expr AS data-type )
 *
 *    ex) CAST( i1 AS INTEGER )
 *
 ***********************************************************************/
    
    SInt   sSrcLen;
    SInt   sDstLen;
    void * sDstValue;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_ASSERT_MSG( aStack[0].column->module->id == aStack[1].column->module->id,
                    "aStack[0].column->module->id : %"ID_UINT32_FMT"\n"
                    "aStack[1].column->module->id : %"ID_UINT32_FMT"\n",
                    aStack[0].column->module->id,
                    aStack[1].column->module->id );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        if ( (aStack[0].column->module->flag & MTD_CANON_MASK)
             == MTD_CANON_NEED_WITH_ALLOCATION )
        {
            sDstValue = aStack[0].value;
            IDE_TEST( aStack[0].column->module->canonize(
                          aStack[0].column,
                          & sDstValue,           // canonized value
                          NULL,
                          aStack[1].column,
                          aStack[1].value,       // original value
                          NULL,
                          aTemplate )
                      != IDE_SUCCESS );

            // canonize가 필요없어서 sDstValue의 값을
            // source pointer로 변경해서 리턴한 경우 (sDstValue == aStack[1].value)
            // stack[0].value에 stack[1].value의 값을 복사해준다.
            if ( sDstValue != aStack[0].value ) 
            {
                sSrcLen = aStack[1].column->module->actualSize( aStack[1].column,
                                                                aStack[1].value );
                sDstLen = aStack[0].column->column.size;
                
                IDE_TEST_RAISE( sSrcLen > sDstLen, ERR_INVALID_LENGTH );
                
                idlOS::memcpy( aStack[0].value, sDstValue, sSrcLen );
            }
            else
            {
                // Nothing to do.
            }            
        }
        else
        {
            sSrcLen = aStack[1].column->module->actualSize( aStack[1].column,
                                                            aStack[1].value );
            sDstLen = aStack[0].column->column.size;
            
            IDE_TEST_RAISE( sSrcLen > sDstLen, ERR_INVALID_LENGTH );
            
            idlOS::memcpy( aStack[0].value, aStack[1].value, sSrcLen );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );

    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
