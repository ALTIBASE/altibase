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
 * $Id: mtfNullif.cpp 90785 2021-05-06 07:26:22Z hykim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNullif;

extern mtdModule mtdNull;
extern mtdModule mtdList;
extern mtdModule mtdNumeric;
extern mtdModule mtdFloat;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdEchar;
extern mtdModule mtdEvarchar;

static mtcName mtfNullifFunctionName[1] = {
    { NULL, 6, (void*)"NULLIF" }
};

static IDE_RC mtfNullifEstimate( mtcNode     * aNode,
                                 mtcTemplate * aTemplate,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 mtcCallBack * aCallBack );

mtfModule mtfNullif = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfNullifFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNullifEstimate
};

IDE_RC mtfNullifCalculate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNullifCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNullifEstimate( mtcNode     * aNode,
                          mtcTemplate * aTemplate,
                          mtcStack    * aStack,
                          SInt          /* aRemain */,
                          mtcCallBack * aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    IDE_TEST_RAISE ( ( aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                     ( aStack[1].column->module->id == MTD_CLOB_ID ) ||
                     ( aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                     ( aStack[1].column->module->id == MTD_BLOB_ID ),
                     ERR_ARGUMENT_NOT_APPLICABLE );

    IDE_TEST_RAISE ( ( aStack[2].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                     ( aStack[2].column->module->id == MTD_CLOB_ID ) ||
                     ( aStack[2].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                     ( aStack[2].column->module->id == MTD_BLOB_ID ),
                     ERR_ARGUMENT_NOT_APPLICABLE );

    sModules[0] = aStack[1].column->module;
    sModules[1] = aStack[1].column->module;

    // To fix BUG-15093
    // numeric module이 선택된 경우 float으로 바꾸어야 함.

    // PROJ-2002 Column Security
    // 보안 컬럼인 경우 원본 컬럼으로 바꾼다.
    if( sModules[0] == &mtdNull )
    {
        sModules[0] = &mtdVarchar;
        sModules[1] = &mtdVarchar;
    }
    else if( sModules[0] == &mtdNumeric )
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
    else
    {
        /* Nothing to do */
    }
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    mtc::initializeColumn( aStack[0].column, aStack[1].column );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_INVALID_PRECISION )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 *  Nullif ( expr1, expr 2 )
 *
 *    두개의 expression을 비교해서 같다면 NULL을 반환하고, 다르다면 첫번 째 것 을 반환한다.
 *
 *    select nullif(10,10) from dual; -> NULL
 *    select nullif(10,9) from dual;  -> 10
 *    select nullif(9,10) from dual;  -> 9
 */
IDE_RC mtfNullifCalculate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate )
{
    const mtdModule* sModule;
    mtdValueInfo     sValueInfo1;
    mtdValueInfo     sValueInfo2;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value )
         == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        if ( aStack[2].column->module->isNull( aStack[2].column,
                                               aStack[2].value )
             == ID_TRUE )
        {
            idlOS::memcpy( aStack[0].value,
                           aStack[1].value,
                           aStack[1].column->module->actualSize( aStack[1].column,
                                                                 aStack[1].value ) );
        }
        else
        {
            sModule = aStack[1].column->module;

            sValueInfo1.column = aStack[1].column;
            sValueInfo1.value  = aStack[1].value;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aStack[2].column;
            sValueInfo2.value  = aStack[2].value;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                 &sValueInfo2 )
                == 0 )
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
    }
   
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
