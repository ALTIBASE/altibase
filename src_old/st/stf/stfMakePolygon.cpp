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
 * $Id: stfMakePolygon.cpp 88597 2020-09-16 06:53:00Z emlee $
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

extern mtfModule stfMakePolygon;
extern mtdModule stdGeometry;  

static mtcName stfMakePolygonFunctionName[1] = {
    { NULL, 14, (void*)"ST_MAKEPOLYGON" }
};

static IDE_RC stfMakePolygonEstimate( mtcNode     * aNode,
                                      mtcTemplate * aTemplate,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      mtcCallBack * aCallBack );

mtfModule stfMakePolygon = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  
    stfMakePolygonFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfMakePolygonEstimate
};

IDE_RC stfMakePolygonCalculate( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfMakePolygonCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfMakePolygonEstimate( mtcNode     * aNode,
                               mtcTemplate * aTemplate,
                               mtcStack    * aStack,
                               SInt       /* aRemain */,
                               mtcCallBack * aCallBack )
{
    UInt     sObjBufSize;
    UInt     sArgc;

    const mtdModule* sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    sArgc = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    IDE_TEST_RAISE( sArgc != 1  ,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sObjBufSize = aTemplate->getSTObjBufSize( aCallBack );

    sModules[0] = &stdGeometry;

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

IDE_RC stfMakePolygonCalculate( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate )
{
    stdGeometryHeader      * sRet        = NULL;
    stdPolygon2DType       * sGeom       = NULL;
    stdLinearRing2D        * sRing       = NULL;
    stdPoint2D             * sSrcPt2D    = NULL;
    stdPoint2D             * sDstPt2D    = NULL;
    iduMemory              * sQmxMem     = NULL;
    qcTemplate             * sQcTmplate  = NULL;
    UInt                     sNumPoints  = 0;        
    UInt                     i           = 0;
    UInt                     j           = 0;
    UInt                     sSize       = 0;
    SInt                     sSRID       = 0; 
    SInt                     sCurrSRID   = 0;
    idBool                   isNull      = ID_FALSE;
    mtdIntegerType           sIsRing;
    stdGeometryHeader      * sArgs; 
    UInt                     sArgc;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /* Number of Arguments */                                         
    sArgc = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;              

    /* Calculate Total Size of Arguments */ 
    for ( i = 0 ; i < sArgc ; i++ ) 
    {
        sArgs = (stdGeometryHeader *)aStack[i+1].value;

        /* Check Null */ 
        if ( stdGeometry.isNull( NULL, sArgs ) == ID_TRUE )
        {
            isNull = ID_TRUE;
            break;
        }

        /* is Ring? */ 
        IDE_TEST( stfFunctions::testRing(sArgs, &sIsRing) != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsRing != 1, err_not_closed_ring );

        sCurrSRID = stfBasic::getSRID( sArgs );
        if ( i == 0 )
        {
            sSRID = sCurrSRID; 
        }
        else 
        {
            IDE_TEST_RAISE( sSRID != sCurrSRID, err_mixed_srid );
        }

        IDE_TEST_RAISE( ((sArgs->mType != STD_LINESTRING_2D_TYPE) &&
                        (sArgs->mType != STD_LINESTRING_2D_EXT_TYPE)), err_invalid_object_mType );

        sNumPoints = STD_N_POINTS( (stdLineString2DType *)sArgs );
        sSize += STD_RN2D_SIZE + ( STD_PT2D_SIZE * sNumPoints );
    }

    /* Fix BUG-15412 mtdModule.isNull »ç¿ë */
    if ( isNull == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sRet = (stdGeometryHeader *)aStack[0].value;

        /* Fix BUG-15290 */
        stdUtils::nullify( sRet );

        sGeom = (stdPolygon2DType *)sRet;
        sGeom->mType = STD_POLYGON_2D_TYPE;

        if ( sSRID == ST_SRID_UNDEFINED )
        {
            sGeom->mSize = STD_POLY2D_SIZE;
        }
        else 
        {
            stfBasic::setSRID( sRet, aStack[0].column->precision, sSRID ); 
            sGeom->mSize = STD_POLY2D_EXT_SIZE;
        }

        /* Check Size of Geometry Object for Return */
        if ( (SInt)sGeom->mSize > (SInt)( aStack[0].column->precision - sSize ) ) 
        {
            IDE_RAISE(err_object_buffer_overflow);
        }

        sGeom->mSize += sSize;
        sGeom->mNumRings = sArgc; 

        for ( i = 0 ; i < sArgc ; i++ )
        {
            sArgs = (stdGeometryHeader *)aStack[i+1].value;

            /* First Ring */ 
            if ( i == 0 ) 
            {
                sRing = STD_FIRST_RN2D( sGeom );
            }
            else
            {
                sRing = STD_NEXT_RN2D( sRing );
            }
            sNumPoints        = STD_N_POINTS( (stdLineString2DType*)sArgs );
            sRing->mNumPoints = sNumPoints;

            for ( j = 0 ; j < sNumPoints ; j++ )
            {
                if ( j == 0 ) 
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
    IDE_EXCEPTION(err_mixed_srid);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_MIXED_SRID, 
                                sSRID, sCurrSRID)); 
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
