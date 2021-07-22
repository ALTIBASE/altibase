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
 * $Id: stfPolygon.cpp 88116 2020-07-20 07:40:01Z ykim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <stfBasic.h>
#include <mtdTypes.h>
#include <stfFunctions.h>
#include <stdPrimitive.h>  
#include <stdUtils.h>
#include <ste.h>
#include <qc.h>
#include <stuProperty.h>

extern mtfModule stfPolygon;
extern mtdModule stdGeometry;  
extern mtdModule mtdInteger;  

static mtcName stfPolygonFunctionName[1] = {
    { NULL, 10, (void*)"ST_POLYGON" }
};

static IDE_RC stfPolygonEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

mtfModule stfPolygon = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  
    stfPolygonFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfPolygonEstimate
};

IDE_RC stfPolygonCalculate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfPolygonCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfPolygonEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt       /* aRemain */,
                           mtcCallBack * aCallBack )
{
    UInt             sObjBufSize;

    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sModules[0] = &stdGeometry;
    sModules[1] = &mtdInteger;

    sObjBufSize = aTemplate->getSTObjBufSize( aCallBack );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = stfExecute;
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     1,
                                     sObjBufSize,
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

IDE_RC stfPolygonCalculate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate )
{
    stdGeometryHeader         * sRet        = NULL;
    stdPolygon2DExtType       * sGeom       = NULL;
    stdLinearRing2D           * sRing       = NULL;
    stdPoint2D                * sSrcPt2D    = NULL;
    stdPoint2D                * sDstPt2D    = NULL;
    iduMemory                 * sQmxMem     = NULL;
    qcTemplate                * sQcTmplate  = NULL;
    UInt                        sNumPoints  = 0;        
    UInt                        i           = 0;
    UInt                        sSize       = 0;
    mtdIntegerType            * sSRID;
    mtdIntegerType              sIsRing;
    stdGeometryHeader         * sArgs; 

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sArgs = (stdGeometryHeader *)aStack[1].value;
    sSRID = (mtdIntegerType *)(aStack[2].value);
    
    /* Check Null */ 
    if ( (stdGeometry.isNull( NULL, sArgs )) ||
         (mtdInteger.isNull( NULL, sSRID )) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        /* is Ring? */ 
        IDE_TEST( stfFunctions::testRing(sArgs, &sIsRing) != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsRing != 1, err_not_closed_ring );

        IDE_TEST_RAISE( ((sArgs->mType != STD_LINESTRING_2D_TYPE) && 
                         (sArgs->mType != STD_LINESTRING_2D_EXT_TYPE)), err_invalid_object_mType );

        sNumPoints = STD_N_POINTS( (stdLineString2DType *)sArgs );
        sSize += STD_RN2D_SIZE + ( STD_PT2D_SIZE * sNumPoints );

        sRet = (stdGeometryHeader *)aStack[0].value;

        /* Fix BUG-15290 */
        stdUtils::nullify( sRet );

        sGeom = (stdPolygon2DExtType *)sRet;
        sGeom->mType = STD_POLYGON_2D_EXT_TYPE;
        sGeom->mSize = STD_POLY2D_EXT_SIZE;
        sGeom->mSRID = *sSRID; 

        /* Check Size of Geometry Object for Return */
        if ( (SInt)sGeom->mSize > (SInt)( aStack[0].column->precision - sSize ) ) 
        {
            IDE_RAISE(err_object_buffer_overflow);
        }

        sGeom->mSize += sSize;
        sGeom->mNumRings = 1; 

        sRing             = STD_FIRST_RN2D( sGeom );
        sNumPoints        = STD_N_POINTS( (stdLineString2DType*)sArgs );
        sRing->mNumPoints = sNumPoints;

        for ( i = 0 ; i < sNumPoints ; i++ )
        {
            if ( i == 0 ) 
            {
                /* Move First Point */
                sDstPt2D = STD_FIRST_PT2D( sRing );
                sSrcPt2D = STD_FIRST_PT2D( (stdLineString2DType*)sArgs );
            }
            else 
            {
                /* Move Next Point */
                sDstPt2D = STD_NEXT_PT2D( sDstPt2D );
                sSrcPt2D = STD_NEXT_PT2D( sSrcPt2D );
            }

            sDstPt2D->mX = sSrcPt2D->mX;
            sDstPt2D->mY = sSrcPt2D->mY;

            IDE_TEST( stdUtils::mergeMBRFromPoint2D( sRet, sDstPt2D ) != IDE_SUCCESS );
        
        }

        if ( STU_VALIDATION_ENABLE == STU_VALIDATION_ENABLE_TRUE )
        {
            if ( stdPrimitive::validatePolygon2D( sQmxMem,
                                                  sGeom,
                                                  sGeom->mSize )
                 == IDE_SUCCESS )
            {
                sGeom->mIsValid = ST_VALID;
            }
            else
            {
                sGeom->mIsValid = ST_INVALID;
            }
        }
        else
        {
            sGeom->mIsValid = ST_INVALID;
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_object_buffer_overflow);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_BUFFER_OVERFLOW));
    }
    IDE_EXCEPTION(err_not_closed_ring);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_CLOSED_RING, i+1));
    }
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
