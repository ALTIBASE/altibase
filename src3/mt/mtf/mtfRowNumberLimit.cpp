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
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfRowNumberLimit;

extern mtdModule mtdBigint;

static mtcName mtfRowNumberLimitFunctionName[1] = {
    { NULL, 16, (void*)"ROW_NUMBER_LIMIT" }
};

static IDE_RC mtfRowNumberLimitEstimate( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

mtfModule mtfRowNumberLimit = {
    1 | MTC_NODE_OPERATOR_AGGREGATION  |
    MTC_NODE_FUNCTION_ANALYTIC_TRUE |
    MTC_NODE_FUNCTION_RANKING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfRowNumberLimitFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRowNumberLimitEstimate
};

IDE_RC mtfRowNumberLimitInitialize(  mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC mtfRowNumberLimitAggregate(  mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfRowNumberLimitFinalize(  mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfRowNumberLimitCalculate(  mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtfRowNumberLimitInitialize,
    mtfRowNumberLimitAggregate,
    mtf::calculateNA,
    mtfRowNumberLimitFinalize,
    mtfRowNumberLimitCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRowNumberLimitEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt,
                                  mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdBigint
    };
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    //IDE_TEST( mtdBigint.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRowNumberLimitInitialize( mtcNode*     aNode,
                                    mtcStack*,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn->column.offset) = 0;
    
    return IDE_SUCCESS;
}

IDE_RC mtfRowNumberLimitAggregate( mtcNode*     aNode,
                                   mtcStack*,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn->column.offset) += 1;
    
    return IDE_SUCCESS;
}

IDE_RC mtfRowNumberLimitFinalize( mtcNode*,
                                  mtcStack*,
                                  SInt,
                                  void*,
                                  mtcTemplate* )
{
    return IDE_SUCCESS;
}

IDE_RC mtfRowNumberLimitCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    return IDE_SUCCESS;
}
