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
 * $Id: mtfTo_raw.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfTo_raw;

extern mtdModule mtdVarbyte;
extern mtdModule mtdInteger;

static mtcName mtfTo_rawFunctionName[1] = {
    { NULL, 6, (void*)"TO_RAW" }
};

static IDE_RC mtfTo_rawEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfTo_raw = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfTo_rawFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfTo_rawEstimate
};

static IDE_RC mtfTo_rawCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_rawCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_rawEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt      /* aRemain */,
                          mtcCallBack* /*aCallBack*/ )
{
    UInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    IDE_TEST_RAISE( ( aStack[1].column->module->id == MTD_LIST_ID ) ||
                    ( aStack[1].column->module->id == MTD_ROWTYPE_ID ) ||
                    ( aStack[1].column->module->id == MTD_RECORDTYPE_ID ) ||
                    ( aStack[1].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) ||
                    ( aStack[1].column->module->id == MTD_BLOB_ID ) ||
                    ( aStack[1].column->module->id == MTD_CLOB_ID ) ||
                    ( aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                    ( aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                    ( aStack[1].column->module->id == MTD_ECHAR_ID ) ||
                    ( aStack[1].column->module->id == MTD_EVARCHAR_ID ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    sPrecision = aStack[1].column->column.size;
    
    if ( sPrecision > MTD_VARBYTE_PRECISION_MAXIMUM )
    {
        sPrecision = MTD_VARBYTE_PRECISION_MAXIMUM;
    }
    else
    {
        /* Nothing to do */
    }
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarbyte,
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
    IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfTo_rawCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : TO_RAW Calculate
 *
 * Implementation :
 *    TO_RAW( expression )
 ***********************************************************************/
    
    const mtdModule* sModule;
    mtdByteType*     sResult;
    UChar*           sValue;
    UInt             sActualSize;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sResult   = (mtdByteType*)aStack[0].value;

    sModule   = aStack[1].column->module;

    sValue = (UChar*)aStack[1].value;
    sActualSize = sModule->actualSize( aStack[1].column,
                                       aStack[1].value );
    
    IDE_TEST_RAISE( sActualSize > (UInt)(aStack[0].column->precision) , ERR_INVALID_LENGTH );
    
    idlOS::memcpy( sResult->value, sValue, sActualSize );
    
    sResult->length = sActualSize;
        
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
