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
 * $Id: mtvBinary2Blob.cpp 13146 2005-08-12 09:20:06Z leekmo $
 *
 * Description
 *
 *   PR-15636 (GEOMETRY=>BINARY=>BLOB)
 *
 *   Binary ==> Blob Conversion 모듈
 *
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvBinary2Blob;

extern mtdModule mtdBlob;
extern mtdModule mtdBinary;

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Binary2Blob( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

mtvModule mtvBinary2Blob = {
    &mtdBlob,
    &mtdBinary,
    MTV_COST_DEFAULT,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Binary2Blob,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;


    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBlob,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Binary2Blob( mtcNode*,
                                 mtcStack*    aStack,
                                 SInt,
                                 void*,
                                 mtcTemplate* )
{
    if ( ((mtdBinaryType*)aStack[1].value)->mLength == 0 )
    {
        mtdBlob.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
        IDE_TEST_RAISE( (UInt)aStack[0].column->precision < ((mtdBinaryType *)aStack[1].value)->mLength,
                        ERR_CONVERT );

        ((mtdBlobType*)aStack[0].value)->length =
            ((mtdBinaryType*)aStack[1].value)->mLength;
    
        idlOS::memcpy( ((mtdBlobType*)aStack[0].value)->value,
                       ((mtdBinaryType*)aStack[1].value)->mValue,
                       ((mtdBinaryType*)aStack[1].value)->mLength );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 
