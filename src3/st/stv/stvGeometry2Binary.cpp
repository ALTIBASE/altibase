/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: stvGeometry2Binary.cpp 13146 2005-08-12 09:20:06Z leekmo $
 *
 * Description
 *
 *    PROJ-1583, PR-15421
 *    Geometry => Binary Conversion
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtl.h>
#include <mtx.h>

#include <mtdTypes.h>
#include <stdTypes.h>

#include <ste.h>

extern mtvModule stvGeometry2Binary;

extern mtdModule stdGeometry;
extern mtdModule mtdBinary;

static IDE_RC stvGeo2BinaryEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

IDE_RC stvCalculate_Geometry2Binary( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtvModule stvGeometry2Binary = {
    &mtdBinary,
    &stdGeometry,
    MTV_COST_DEFAULT,
    stvGeo2BinaryEstimate
};

static const mtcExecute stvGeo2BinaryExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stvCalculate_Geometry2Binary,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC stvGeo2BinaryEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt,
                                     mtcCallBack* )
{
    SInt sBinPrecision;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column]
        = stvGeo2BinaryExecute;

    // To Fix BUG-16031
    // Binary 에 Geometry Data를 넣기 위해서는 Header크기를 포함해야 함.
    sBinPrecision = aStack[1].column->precision + ID_SIZEOF(stdGeometryHeader);
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBinary,
                                     1,
                                     sBinPrecision,
                                     0 )
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC stvCalculate_Geometry2Binary( mtcNode*,
                                     mtcStack*    aStack,
                                     SInt,
                                     void*,
                                     mtcTemplate* )
{
    UInt sPrecision;
    stdGeometryHeader * sGeoHeader;
    
    sGeoHeader = (stdGeometryHeader*) aStack[1].value;
    sPrecision = aStack[0].column->precision;


    // BUG-24514
    // geometry 타입의 null 을 검사해야 한다.
    if(aStack[1].column->module->isNull( aStack[1].column,
                                         aStack[1].value ) == ID_TRUE)
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // To Fix BUG-15421
        IDE_TEST_RAISE( sPrecision < sGeoHeader->mSize, ERR_INVALID_LENGTH );
        ((mtdBinaryType*)aStack[0].value)->mLength = sGeoHeader->mSize;
        
        idlOS::memcpy( ((mtdBinaryType*)aStack[0].value)->mValue,
                       (void*) sGeoHeader,
                       sGeoHeader->mSize );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_LENGTH);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
