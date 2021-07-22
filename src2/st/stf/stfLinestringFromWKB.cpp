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
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <stfWKB.h>
#include <mtdTypes.h>
#include <stdTypes.h> // BUG-24594
#include <qc.h>
#include <stdParsing.h>

extern mtfModule stfLinestringFromWKB;

extern mtdModule stdGeometry;
extern mtdModule mtdBinary;   // Fix BUG-15834
extern mtdModule mtdInteger;

static mtcName stfLinestringFromWKBFunctionName[1] = {
    { NULL, 20, (void*)"ST_LINESTRINGFROMWKB" },
};
static IDE_RC stfLinestringFromWKBEstimate(
                        mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        mtcCallBack* aCallBack );

mtfModule stfLinestringFromWKB = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfLinestringFromWKBFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfLinestringFromWKBEstimate
};

IDE_RC stfLinestringFromWKBCalculate(
                        mtcNode*     aNode,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        void*        aInfo,
                        mtcTemplate* aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfLinestringFromWKBCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfLinestringFromWKBEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt      /* aRemain */,
                                     mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];
    // Fix Bug-24594
    SInt  sNativeGeomSize = 0;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                      MTC_NODE_QUANTIFIER_TRUE ),
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1 ) ||
                      ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2 ) ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sModules[0] = &mtdBinary;

    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
    {
        sModules[1] = &mtdInteger;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = stfExecute;

    // BUG-24594
    if ( aStack[1].column->precision < (WKB_GEOHEAD_SIZE +
                                        WKB_INT32_SIZE +
                                        WKB_GEOHEAD_SIZE +
                                        WKB_POINT_SIZE ) )
    {
        // BUG-24594 Consider Binary Null Binding.
        // so minimum size of data must be grater than GEOHEAD_SIZE
        sNativeGeomSize = aStack[1].column->precision + STD_GEOHEAD_SIZE - 9;
        if (sNativeGeomSize < (SInt)STD_GEOHEAD_SIZE)
        {
            sNativeGeomSize = (SInt)STD_GEOHEAD_SIZE;
        }
    }
    else
    {
        // Native Size  = (WKB - header size = Point data size ) + native line header size.
        // WKB_LINESTRING_SIZE = WKB_GEOHEAD_SIZE + WKB_INT32_SIZE = 9
        // EWKB_LINESTRING_SIZE = WKB_GEOHEAD_SIZE + WKB_INT32_SIZE + WKB_INT32_SIZE = 13
        sNativeGeomSize = ( (aStack[1].column->precision - 
                             (WKB_GEOHEAD_SIZE + WKB_INT32_SIZE)) + 
                            STD_LINE2D_SIZE );
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     1,
                                     sNativeGeomSize,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfLinestringFromWKBCalculate( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
    IDE_RC               sRc;
    qcTemplate         * sQcTmplate;
    iduMemory          * sQmxMem;
    iduMemoryStatus      sQmxMemStatus;
    UInt                 sStage          = 0;
    idBool               sIsReturnNULL   = ID_FALSE;
    SInt                 sSRID           = ST_SRID_INIT;
    UInt                 sWkbType;
    UChar              * sWKB;
    UInt                 sWKBLength;
    stdGeometryHeader  * sBuf; 
   
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    // Fix BUG-24514 WKB Null problem.
    if ( aStack[1].column->module->isNull(aStack[1].column,
                                         aStack[1].value) == ID_TRUE)
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // PROJ-2422 srid 지원
        if ( (aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 )
        {
            if ( aStack[2].column->module->isNull( aStack[2].column,
                                                   aStack[2].value ) == ID_TRUE )
            {
                // SRID 인자가 NULL인 경우 NULL을 리턴합니다.
                sIsReturnNULL = ID_TRUE;
                aStack[0].column->module->null( aStack[0].column,
                                                aStack[0].value );
            }
            else
            {
                //sSRIDOption = ID_TRUE;
                sSRID = *(mtdIntegerType*)aStack[2].value;
            }
        }
        else
        {
            // Nothing To Do
        }

        if ( sIsReturnNULL == ID_FALSE )
        {
            sQcTmplate = (qcTemplate*) aTemplate;
            sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

            // Memory 재사용을 위하여 현재 위치 기록
            IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
            sStage = 1;

            sWKB       = (UChar*)((mtdBinaryType*)aStack[1].value)->mValue;
            sWKBLength = ((mtdBinaryType*)aStack[1].value)->mLength;
            sBuf       = (stdGeometryHeader *)aStack[0].value;

            IDE_TEST( stdParsing::stdBinValue( sQmxMem,
                                               sWKB,
                                               sWKBLength,
                                               sBuf,
                                               ((SChar*)(aStack[0].value) +
                                                aStack[0].column->column.size),
                                               &sRc,
                                               STU_VALIDATION_ENABLE )
                      != IDE_SUCCESS );

            // LINESTRING이 아닌 경우(LINESTRING EMPTY 제외) -> NULL
            // LINESTRING인 경우 입력한 SRID로 변경
            if ( sBuf->mType == STD_EMPTY_TYPE )
            {
                // empty type은 객체의 형태를 알수 없어서 재 parsing을 수행한다.
                IDE_TEST( stfWKB::typeFromWKB( sWKB, &sWkbType ) 
                          != IDE_SUCCESS );

                if ( ( sWkbType != WKB_LINESTRING_TYPE ) && 
                     ( sWkbType != EWKB_LINESTRING_TYPE ) )
                {
                    // return NULL
                    aStack[0].column->module->null( aStack[0].column,
                                                    aStack[0].value );
                }
                else
                {
                    // linestring empty : SRID를 세팅하지 않는다.
                    // Nothing to do.
                }
            }
            else
            {
                if ( sBuf->mType == STD_LINESTRING_2D_TYPE )
                {
                    if ( sSRID != ST_SRID_INIT )
                    {
                        // STD_LINE2D_SIZE와 STD_LINE2D_EXT_SIZE 같기 때문에
                        // sBuf의 precision을 넘을 수 없다.
                        IDE_DASSERT( STD_LINE2D_SIZE == STD_LINE2D_EXT_SIZE );
                        sBuf->mType = STD_LINESTRING_2D_EXT_TYPE;
                        ((stdLineString2DExtType*)sBuf)->mSRID = sSRID;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else if ( sBuf->mType == STD_LINESTRING_2D_EXT_TYPE )
                {
                    if ( sSRID != ST_SRID_INIT )
                    {
                        ((stdLineString2DExtType*)sBuf)->mSRID = sSRID;
                    }
                    else
                    {
                        // Noting to do.
                    }
                }
                else
                {
                    // retrun NULL
                    aStack[0].column->module->null( aStack[0].column,
                                                    aStack[0].value );
                }
            }

            // Memory 재사용을 위한 Memory 이동
            sStage = 0;
            IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);   
        }
        else
        {
            // Nothing To Do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}
 
