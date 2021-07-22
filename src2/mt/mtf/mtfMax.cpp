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
 * $Id: mtfMax.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfMax;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;

static mtcName mtfMaxFunctionName[1] = {
    { NULL, 3, (void*)"MAX" }
};

static IDE_RC mtfMaxEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              mtcCallBack* aCallBack );

mtfModule mtfMax = {
    1|MTC_NODE_OPERATOR_AGGREGATION|MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfMaxFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfMaxEstimate
};

IDE_RC mtfMaxInitialize(    mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

IDE_RC mtfMaxAggregate(    mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

IDE_RC mtfMaxMerge(    mtcNode*     aNode,
                       mtcStack*    aStack,
                       SInt         aRemain,
                       void*        aInfo,
                       mtcTemplate* aTemplate );

IDE_RC mtfMaxFinalize(    mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

IDE_RC mtfMaxCalculate(    mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtfMaxInitialize,
    mtfMaxAggregate,
    mtfMaxMerge,
    mtfMaxFinalize,
    mtfMaxCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMaxEstimate( mtcNode*     aNode,
                       mtcTemplate* aTemplate,
                       mtcStack*    aStack,
                       SInt         /* aRemain */,
                       mtcCallBack* /* aCallBack */)
{
    aNode->lflag &= ~MTC_NODE_DISTINCT_MASK;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // PROJ-2002 Column Security
    // min함수와 같다.
    aNode->baseTable = aNode->arguments->baseTable;
    aNode->baseColumn = aNode->arguments->baseColumn;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList ||
                    aStack[1].column->module == &mtdBoolean,
                    ERR_CONVERSION_NOT_APPLICABLE );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    // BUG-23102
    // mtcColumn으로 초기화한다.
    mtc::initializeColumn( aStack[0].column, aStack[1].column );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMaxInitialize( mtcNode*     aNode,
                         mtcStack*,
                         SInt,
                         void*,
                         mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    void*            sValueTemp;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValueTemp = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

    sColumn->module->null( sColumn,
                           sValueTemp );
    
    return IDE_SUCCESS;
}

IDE_RC mtfMaxAggregate( mtcNode*     aNode,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        void*,
                        mtcTemplate* aTemplate )
{
    const mtdModule* sModule;
    mtcNode*         sNode;
    mtdValueInfo     sValueInfo1;
    mtdValueInfo     sValueInfo2;
    
    
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );
    
    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                   aStack + 1,
                                                                  aRemain - 1,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                + aStack[0].column->column.offset );
    sModule          = aStack[0].column->module;
    
    // NULL을 비교 대상에서 제외하기 위하여 Descending Key Compare를 사용함.
    sValueInfo1.column = aStack[0].column;
    sValueInfo1.value  = aStack[0].value;
    sValueInfo1.flag   = MTD_OFFSET_USELESS;

    sValueInfo2.column = aStack[1].column;
    sValueInfo2.value  = aStack[1].value;
    sValueInfo2.flag   = MTD_OFFSET_USELESS;

    if ( sModule->logicalCompare[MTD_COMPARE_DESCENDING]( &sValueInfo1,
                                                          &sValueInfo2 ) > 0 )
    {
        idlOS::memcpy( aStack[0].value,
                       aStack[1].value,
                       sModule->actualSize( aStack[1].column,
                                            aStack[1].value ) );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMaxMerge(    mtcNode*     aNode,
                       mtcStack*    ,
                       SInt         ,
                       void*        aInfo,
                       mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    const mtdModule* sModule;
    mtdValueInfo     sValueInfo1;
    mtdValueInfo     sValueInfo2;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sModule = sColumn->module;

    // NULL을 비교 대상에서 제외하기 위하여 Descending Compare를 사용함.
    sValueInfo1.column = sColumn;
    sValueInfo1.value  = (void*)((UChar*)aTemplate->rows[aNode->table].row +
                                         sColumn->column.offset);
    sValueInfo1.flag   = MTD_OFFSET_USELESS;

    sValueInfo2.column = sColumn;
    sValueInfo2.value  = (void*)((UChar*)aInfo +
                                         sColumn->column.offset);
    sValueInfo2.flag   = MTD_OFFSET_USELESS;

    if ( sModule->logicalCompare[MTD_COMPARE_DESCENDING]( &sValueInfo1,
                                                          &sValueInfo2 ) > 0 )
    {
        idlOS::memcpy( (void*)sValueInfo1.value,
                       (void*)sValueInfo2.value,
                       sModule->actualSize( sValueInfo2.column,
                                            sValueInfo2.value ) );
    }

    return IDE_SUCCESS;
}

IDE_RC mtfMaxFinalize( mtcNode*,
                       mtcStack*,
                       SInt,
                       void*,
                       mtcTemplate* )
{
    return IDE_SUCCESS;
}

IDE_RC mtfMaxCalculate( mtcNode*     aNode,
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
