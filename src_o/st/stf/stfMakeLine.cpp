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
 * $Id: stfMakeLine.cpp 88116 2020-07-20 07:40:01Z ykim $
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
#include <stuProperty.h>

extern mtfModule stfMakeLine;
extern mtdModule stdGeometry;  

static mtcName stfMakeLineFunctionName[1] = {
    { NULL, 11, (void*)"ST_MAKELINE" }
};

static IDE_RC stfMakeLineEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule stfMakeLine = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,
    stfMakeLineFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfMakeLineEstimate
};

IDE_RC stfMakeLineCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfMakeLineCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfMakeLineEstimate( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt      /* aRemain */,
                            mtcCallBack* aCallBack )
{
    UInt                    sArgc;
    UInt                    i;
    UInt                    sObjBufSize;

    const  mtdModule*       sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];


    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    sArgc = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    IDE_TEST_RAISE( sArgc != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    for ( i = 0 ; i < sArgc ; i++ )
    {
        sModules[i] = &stdGeometry;
    }

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

IDE_RC stfMakeLineCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*     /* aInfo */,
                             mtcTemplate* aTemplate )
{
    stdGeometryHeader      * sRet        = NULL;
    stdLineString2DExtType * sGeom       = NULL; 
    stdLineString2DType    * sLs2DType   = NULL;
    stdMultiPoint2DType    * sMp2DType   = NULL;
    stdPoint2DType         * sPt2DType   = NULL;
    stdPoint2D             * sSrcPt2D    = NULL;
    stdPoint2D             * sPt2D       = NULL;
    UInt                     sNumPoints  = 0;        
    UInt                     i           = 0;
    UInt                     j           = 0;
    UInt                     sSize       = 0;
    SInt                     sSRID       = 0; 
    SInt                     sCurrSRID   = 0;
    idBool                   isNull      = ID_FALSE;
    stdGeometryHeader      * sArgs; 
    UInt                     sArgc;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );


    /* Number of Arguments */ 
    sArgc = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    for ( i = 0 ; i < sArgc ; i++ ) 
    {
        /* Set Arguments */ 
        sArgs = (stdGeometryHeader *)aStack[i+1].value;

        /* Fix BUG-15412 mtdModule.isNull 사용 */
        if ( stdGeometry.isNull( NULL, sArgs ) == ID_TRUE )
        {
            isNull = ID_TRUE;
            break;
        }
            
        sCurrSRID = stfBasic::getSRID( sArgs );
        if ( i == 0 )
        {
            sSRID = sCurrSRID; 
        }
        else 
        {
            IDE_TEST_RAISE( sSRID != sCurrSRID, err_mixed_srid );
        }

        /* Calculate Total Size of Arguments and Check SRID */  
        switch( sArgs->mType )
        {
            case STD_POINT_2D_EXT_TYPE :
            case STD_POINT_2D_TYPE :
                sSize += STD_PT2D_SIZE;
                break;

            case STD_LINESTRING_2D_EXT_TYPE :
            case STD_LINESTRING_2D_TYPE :
                sNumPoints = STD_N_POINTS( (stdLineString2DType *)sArgs );
                sSize += STD_PT2D_SIZE * sNumPoints;
                break;

            case STD_MULTIPOINT_2D_EXT_TYPE :
            case STD_MULTIPOINT_2D_TYPE :
                sNumPoints = STD_N_OBJECTS( (stdMultiPoint2DType*)sArgs );
                sSize += STD_PT2D_SIZE * sNumPoints;
                break;

            default :
                IDE_RAISE(err_invalid_object_mType);
                break;
        }
    } 

    if ( isNull == ID_TRUE )
    {
        /* input에 NULL 값이 하나라도 존재하면 NULL 리턴 */  
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sRet = (stdGeometryHeader *)aStack[0].value;

        /* Fix BUG-15290 */
        stdUtils::nullify( sRet );
        
        sGeom = (stdLineString2DExtType *)sRet;
        if ( sSRID == 0 )
        {
            sGeom->mType = STD_LINESTRING_2D_TYPE;
            sGeom->mSize = STD_LINE2D_SIZE;
        }
        else 
        {
            sGeom->mType = STD_LINESTRING_2D_EXT_TYPE;
            sGeom->mSize = STD_LINE2D_EXT_SIZE;
            sGeom->mSRID = sSRID; 
        }

        /* Check Size of Geometry Object for Return */
        if ( (SInt)sGeom->mSize > (SInt)( aStack[0].column->precision - sSize ) ) 
        {
            IDE_RAISE(err_object_buffer_overflow);
        }
        sGeom->mSize += sSize;
        sGeom->mNumPoints = 0; 

        for ( i = 0 ; i < sArgc ; i++ )
        {
            sArgs = (stdGeometryHeader *)aStack[i+1].value;

            switch(sArgs->mType)
            {
                case STD_POINT_2D_EXT_TYPE :
                case STD_POINT_2D_TYPE :
                    sGeom->mNumPoints++; 

                    /* Move First Point */
                    if ( i == 0 ) 
                    {
                        /* First Gemetry Object */ 
                        sPt2D = STD_FIRST_PT2D( sGeom );
                    }
                    else
                    { 
                        /* Next Gemetry Objects */ 
                        sPt2D = STD_NEXT_PT2D( sPt2D );
                    }

                    sPt2D->mX = sArgs->mMbr.mMinX;
                    sPt2D->mY = sArgs->mMbr.mMinY;
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( sRet, sPt2D ) != IDE_SUCCESS );
                    break;
                case STD_LINESTRING_2D_EXT_TYPE :
                case STD_LINESTRING_2D_TYPE :
                    sNumPoints         = STD_N_POINTS( (stdLineString2DType *)sArgs );
                    sGeom->mNumPoints += sNumPoints; 

                    sLs2DType = (stdLineString2DType *)sArgs;  
                    sSrcPt2D  = STD_FIRST_PT2D( sLs2DType );

                    /* Move First Point */
                    if ( i == 0 ) 
                    {
                        /* First Gemetry Object */ 
                        sPt2D = STD_FIRST_PT2D( sGeom );
                    }
                    else
                    { 
                        /* Next Gemetry Objects */ 
                        sPt2D = STD_NEXT_PT2D( sPt2D );
                    }

                    sPt2D->mX = sSrcPt2D->mX; 
                    sPt2D->mY = sSrcPt2D->mY;
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( sRet, sPt2D ) != IDE_SUCCESS );

                    /* Move Next Points of LineString */
                    for ( j = 1 ; j < sNumPoints ; j++ )
                    {
                        sSrcPt2D  = STD_NEXT_PT2D( sSrcPt2D );
                        sPt2D     = STD_NEXT_PT2D( sPt2D );
                        sPt2D->mX = sSrcPt2D->mX;
                        sPt2D->mY = sSrcPt2D->mY; 
                        IDE_TEST( stdUtils::mergeMBRFromPoint2D( sRet, sPt2D ) != IDE_SUCCESS );
                    }
                    break;
                case STD_MULTIPOINT_2D_EXT_TYPE :
                case STD_MULTIPOINT_2D_TYPE :
                    sNumPoints         = STD_N_OBJECTS( (stdMultiPoint2DType*)sArgs );
                    sGeom->mNumPoints += sNumPoints; 

                    sMp2DType = (stdMultiPoint2DType *)sArgs;  
                    sPt2DType = STD_FIRST_POINT2D( sMp2DType );

                    /* Move First Point */
                    if ( i == 0 ) 
                    {
                        /* First Gemetry Object */ 
                        sPt2D = STD_FIRST_PT2D( sGeom );
                    }
                    else
                    { 
                        /* Next Gemetry Objects */ 
                        sPt2D = STD_NEXT_PT2D( sPt2D );
                    }

                    sPt2D->mX = sPt2DType->mPoint.mX; 
                    sPt2D->mY = sPt2DType->mPoint.mY; 
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( sRet, sPt2D ) != IDE_SUCCESS );

                    /* Move Next Points of MultiPoint */
                    for ( j = 1 ; j < sNumPoints ; j++ )
                    {
                        sPt2DType = STD_NEXT_POINT2D( sPt2DType );
                        sPt2D     = STD_NEXT_PT2D( sPt2D );
                        sPt2D->mX = sPt2DType->mPoint.mX; 
                        sPt2D->mY = sPt2DType->mPoint.mY; 
                        IDE_TEST( stdUtils::mergeMBRFromPoint2D( sRet, sPt2D ) != IDE_SUCCESS );
                    }
                    break;

                case STD_POLYGON_2D_TYPE :
                case STD_MULTILINESTRING_2D_TYPE :
                case STD_MULTIPOLYGON_2D_TYPE :
                case STD_GEOCOLLECTION_2D_TYPE :
                case STD_EMPTY_TYPE :
                default :
                    IDE_RAISE(err_invalid_object_mType);
                    break;
            }
        }

        /* Validating LineString */ 
        if ( STU_VALIDATION_ENABLE == STU_VALIDATION_ENABLE_TRUE )
        {
            if ( stdPrimitive::validateLineString2D( sGeom,
                                                     STD_GEOM_SIZE(sGeom) )
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
