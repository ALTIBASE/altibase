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
 * $Id: mtfNvl.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNvl;

extern mtdModule mtdNull;
extern mtdModule mtdList;
extern mtdModule mtdNumeric;
extern mtdModule mtdFloat;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdEchar;
extern mtdModule mtdEvarchar;

static mtcName mtfNvlFunctionName[1] = {
    { NULL, 3, (void*)"NVL" }
};

static IDE_RC mtfNvlEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              mtcCallBack* aCallBack );

mtfModule mtfNvl = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfNvlFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNvlEstimate
};

IDE_RC mtfNvlCalculate(    mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNvlCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNvlEstimate( mtcNode*     aNode,
                       mtcTemplate* aTemplate,
                       mtcStack*    aStack,
                       SInt      /* aRemain */,
                       mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if( aStack[1].column->module != &mtdNull )
    {
        sModules[0] = aStack[1].column->module;
        sModules[1] = aStack[1].column->module;
    }
    else
    {
        sModules[0] = aStack[2].column->module;
        sModules[1] = aStack[2].column->module;
    }

    // To fix BUG-15093
    // numeric module이 선택된 경우 float으로 바꾸어야 함.

    // PROJ-2002 Column Security
    // 보안 컬럼인 경우 원본 컬럼으로 바꾼다.
    if( sModules[0] == &mtdNumeric )
    {
        sModules[0] = &mtdFloat;
        sModules[1] = &mtdFloat;
    }
    else if( sModules[0] == &mtdEchar )
    {
        sModules[0] = &mtdChar;
        sModules[1] = &mtdChar;
    }
    else if( sModules[0] == &mtdEvarchar )
    {
        sModules[0] = &mtdVarchar;
        sModules[1] = &mtdVarchar;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST_RAISE( sModules[0] == &mtdList, ERR_CONVERSION_NOT_APPLICABLE );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    if( ( aStack[1].column->module->flag & MTD_NON_LENGTH_MASK ) == MTD_NON_LENGTH_TYPE )
    {
        IDE_TEST_RAISE( !mtc::isSameType( aStack[1].column, aStack[2].column ),
                        ERR_INVALID_PRECISION );
    }

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( (aStack[1].column->module->id == MTD_BLOB_ID) ||
         (aStack[1].column->module->id == MTD_CLOB_ID) )
    {
        if ( aTemplate->isBaseTable( aTemplate, aNode->arguments->table ) == ID_TRUE )
        {
            IDE_RAISE( ERR_CONVERSION_NOT_APPLICABLE );
        }
        else
        {
            /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
        }
    }
    else if ( (aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID) ||
              (aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID) )
    {
        IDE_RAISE( ERR_CONVERSION_NOT_APPLICABLE );
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }

    // BUG-23102
    // mtcColumn으로 초기화한다.
    if( aStack[1].column->column.size > aStack[2].column->column.size )
    {
        mtc::initializeColumn( aStack[0].column, aStack[1].column );
    }
    else
    {
        mtc::initializeColumn( aStack[0].column, aStack[2].column );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_INVALID_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNvlCalculate( mtcNode*     aNode,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        void*,
                        mtcTemplate* aTemplate )
{
    mtcNode   * sNode;
    mtcExecute* sExecute;
    
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                + aStack->column->column.offset );
    
    sNode    = aNode->arguments;
    sExecute = MTC_TUPLE_EXECUTE(&aTemplate->rows[sNode->table], sNode);

    IDE_TEST( sExecute->calculate( sNode,
                                   aStack + 1,
                                   aRemain - 1,
                                   sExecute->calculateInfo,
                                   aTemplate )
              != IDE_SUCCESS );

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        sNode    = sNode->next;
        sExecute = MTC_TUPLE_EXECUTE(&aTemplate->rows[sNode->table], sNode);

        IDE_TEST( sExecute->calculate( sNode,
                                       aStack + 1,
                                       aRemain - 1,
                                       sExecute->calculateInfo,
                                       aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-2002 Column Security
    // aNode->arguments의 conversion 노드를 연산해야 한다.
    if( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode,
                                         aStack + 1,
                                         aRemain - 1,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }

    idlOS::memcpy( aStack[0].value,
                   aStack[1].value,
                   aStack[1].column->module->actualSize( aStack[1].column,
                                                         aStack[1].value )
                   );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
