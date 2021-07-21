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
 * $Id: mtfBinary_length.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfBinary_length;

extern mtdModule mtdBigint;
extern mtdModule mtdBlob;
extern mtdModule mtdByte;
extern mtdModule mtdNibble;

static mtcName mtfBinary_lengthFunctionName[1] = {
    { NULL, 13, (void*)"BINARY_LENGTH" }
};

static IDE_RC mtfBinary_lengthEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

mtfModule mtfBinary_length = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfBinary_lengthFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfBinary_lengthEstimate
};

static IDE_RC mtfBinary_lengthCalculate( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static IDE_RC mtfBinary_lengthCalculateBlobLocator( mtcNode*     aNode,
                                                    mtcStack*    aStack,
                                                    SInt         aRemain,
                                                    void*        aInfo,
                                                    mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfBinary_lengthCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteBlobLocator = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfBinary_lengthCalculateBlobLocator,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfBinary_lengthEstimate( mtcNode*     aNode,
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

    if ( (aStack[1].column->module->id == MTD_BLOB_ID) ||
         (aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID) ||
         (aStack[1].column->module->id == MTD_BINARY_ID) ||
         (aStack[1].column->module->id == MTD_BYTE_ID) ||
         (aStack[1].column->module->id == MTD_VARBYTE_ID) ||
         (aStack[1].column->module->id == MTD_NIBBLE_ID) ||
         (aStack[1].column->module->id == MTD_BIT_ID) ||
         (aStack[1].column->module->id == MTD_VARBIT_ID) ||
         (aStack[1].column->module->id == MTD_UNDEF_ID) )
    {
        // nothing to do
    }
    else
    {
        IDE_RAISE ( ERR_CONVERT );
    }

    if ( aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteBlobLocator;
    }
    else if ( aStack[1].column->module->id == MTD_BLOB_ID )
    {
        if ( aTemplate->isBaseTable( aTemplate, aNode->arguments->table ) == ID_TRUE )
        {
            IDE_TEST( mtf::getLobFuncResultModule( &sModules[0],
                                                   aStack[1].column->module )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteBlobLocator;
        }
        else
        {
            /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
        }
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfBinary_lengthCalculate( mtcNode*     aNode,
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
 *    BINARY_LENGTH( char )
 *
 *    aStack[0] : 입력된 문자열의 길이 
 *    aStack[1] : char ( 입력된 문자열 )
 *
 ***********************************************************************/

    mtdBlobType*      sBlobInput;
    mtdBinaryType*    sBinaryInput;
    mtdByteType*      sByteInput;
    mtdNibbleType*    sNibbleInput;
    mtdBitType*       sBitInput;

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
        if ( aStack[1].column->module->id == MTD_BLOB_ID )
        {
            sBlobInput = (mtdBlobType*)aStack[1].value;
            *(mtdBigintType*)aStack[0].value = sBlobInput->length;
        }
        else if ( aStack[1].column->module->id == MTD_BINARY_ID )
        {
            sBinaryInput = (mtdBinaryType*)aStack[1].value;
            *(mtdBigintType*)aStack[0].value = sBinaryInput->mLength;
        }
        else if ( ( aStack[1].column->module->id == MTD_BYTE_ID ) ||
                  ( aStack[1].column->module->id == MTD_VARBYTE_ID ) )
        {
            sByteInput = (mtdByteType*)aStack[1].value;
            *(mtdBigintType*)aStack[0].value = sByteInput->length;
        }
        else if ( aStack[1].column->module->id == MTD_NIBBLE_ID )
        {
            sNibbleInput = (mtdNibbleType*)aStack[1].value;
            *(mtdBigintType*)aStack[0].value = sNibbleInput->length;
        }
        else if ( ( aStack[1].column->module->id == MTD_BIT_ID ) ||
                  ( aStack[1].column->module->id == MTD_VARBIT_ID ) )
        {
            sBitInput = (mtdBitType*)aStack[1].value;
            *(mtdBigintType*)aStack[0].value = sBitInput->length;
        }
        else
        {
            IDE_RAISE ( ERR_CONVERT );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfBinary_lengthCalculateBlobLocator( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate )
{
    mtdBlobLocatorType  sLocator = MTD_LOCATOR_NULL;
    UInt                sLength;
    idBool              sIsNull;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sLocator = *(mtdBlobLocatorType*)aStack[1].value;
    
    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );
    
    if ( sIsNull == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        *(mtdBigintType*)aStack[0].value = sLength;
    }
    
    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}
 
