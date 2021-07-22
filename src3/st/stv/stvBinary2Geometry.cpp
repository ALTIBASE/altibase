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
 * $Id: stvBinary2Geometry.cpp 13146 2005-08-12 09:20:06Z leekmo $
 *
 * Description
 *
 *    PROJ-1583, PR-15722
 *    Binary => Geometry Conversion
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtl.h>

#include <mtdTypes.h>
#include <stdTypes.h>

#include <qc.h>

#include <ste.h>
#include <stdPrimitive.h>
#include <stdUtils.h>
#include <stdParsing.h>
#include <stuProperty.h>

extern mtvModule stvBinary2Geometry;

extern mtdModule mtdBinary;
extern mtdModule stdGeometry;

static IDE_RC stvBinary2GeoEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

IDE_RC stvCalculate_Binary2Geometry( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtvModule stvBinary2Geometry = {
    &stdGeometry,
    &mtdBinary,
    MTV_COST_ERROR_PENALTY,
    stvBinary2GeoEstimate
};

static const mtcExecute stvBinary2GeoExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stvCalculate_Binary2Geometry,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC stvBinary2GeoEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt,
                                     mtcCallBack* )
{
    SInt sGeoPrecision;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = stvBinary2GeoExecute;
    
    // To Fix BUG-16031
    // Binary 에 Geometry Header를 포함하고 있음.
    sGeoPrecision = aStack[1].column->precision - ID_SIZEOF(stdGeometryHeader);
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     1,
                                     sGeoPrecision,
                                     0 )
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC stvCalculate_Binary2Geometry( mtcNode*,
                                     mtcStack*    aStack,
                                     SInt,
                                     void*,
                                     mtcTemplate* aTemplate )
{
    idBool sEquiEndian;

    UInt               sPrecision;
    UInt               sValueLength;
    qcTemplate       * sQcTmplate;
    iduMemory        * sQmxMem = NULL;
    iduMemoryStatus    sQmxMemStatus;
    UInt               sStage = 0;
    idBool             sIsValid = ID_TRUE;

    sPrecision   = aStack[0].column->precision;
    sValueLength = ((mtdBinaryType*)aStack[1].value)->mLength;

    // BUG-24425
    // 바이너리 타입의 null 을 검사해야 한다.
    if(aStack[1].column->module->isNull( aStack[1].column,
                                         aStack[1].value ) == ID_TRUE)
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // To Fix BUG-16388
        // To Fix BUG-15421
        IDE_TEST_RAISE( sPrecision < (sValueLength-ID_SIZEOF(stdGeometryHeader) ),
                        ERR_INVALID_LENGTH );

        idlOS::memcpy( aStack[0].value,
                    ((mtdBinaryType*)aStack[1].value)->mValue,
                    sValueLength );

        //-----------------------
        // To Fix BUG-15854
        // Endian 변경
        //-----------------------

        IDE_TEST( stdUtils::isEquiEndian( (stdGeometryHeader*) aStack[0].value,
                                          & sEquiEndian )
                  != IDE_SUCCESS );

        if ( sEquiEndian == ID_TRUE )
        {
            // 동일한 Endian임.
        }
        else
        {
            // 서로 다른 Endian임.
            stdGeometry.endian( aStack[0].value );
        }

        IDE_TEST_RAISE( sValueLength < ( (stdGeometryHeader*) aStack[0].value )->mSize,
                        ERR_INVALID_DATA_LENGTH );

        sQcTmplate = (qcTemplate*) aTemplate;
        sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
        sStage = 1;
        
        // To Fix BUG-16346
        // Binary 객체에 대한 Validation을 수행한다.

        if ( STU_VALIDATION_ENABLE == STU_VALIDATION_ENABLE_TRUE )
        {
            if ( stdPrimitive::validate( sQmxMem,
                                         aStack[0].value,
                                         sValueLength )
                 == IDE_SUCCESS )
            {
                sIsValid = ID_TRUE;
            }
            else
            {
                sIsValid = ID_FALSE;
            }
        }
        else
        {
            sIsValid = ID_FALSE;
        }

        sStage = 0;
        IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);

        stdParsing::setValidHeader( (stdGeometryHeader*) aStack[0].value,
                                    sIsValid,
                                    STU_VALIDATION_ENABLE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_LENGTH);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_LENGTH));
    }
    IDE_EXCEPTION(ERR_INVALID_DATA_LENGTH);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
