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
 * $Id: stfCollect.cpp 88239 2020-07-30 08:41:20Z ykim $
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

extern mtfModule stfCollect;
extern mtdModule stdGeometry;

static mtcName stfCollectFunctionName[1] = {
    { NULL, 10, (void*)"ST_COLLECT" }
};

static IDE_RC stfCollectEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

mtfModule stfCollect = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfCollectFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfCollectEstimate
};

IDE_RC stfCollectCalculate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfCollectCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfCollectEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt       /* aRemain */,
                           mtcCallBack * aCallBack )
{
    UInt     sArgc;
    UInt     i;

    const mtdModule* sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];

    UInt sObjBufSize = aTemplate->getSTObjBufSize( aCallBack );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    sArgc = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    IDE_TEST_RAISE( sArgc != 2  ,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    for ( i = 0 ; i < sArgc ; i++ )
    {
        sModules[i] = &stdGeometry;
    }

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

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfCollectCalculate(
                        mtcNode*     aNode,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        void*        aInfo,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader        * sRet        = NULL;
    stdGeometryHeader        * sValue1     = NULL;
    stdGeometryHeader        * sValue2     = NULL;
    stdPoint2DType           * sPt2DType   = NULL;
    stdLineString2DType      * sLs2DType   = NULL;
    stdPolygon2DType         * sPg2DType   = NULL;
    stdGeometryType          * sObj        = NULL;
    iduMemory                * sQmxMem     = NULL;
    qcTemplate               * sQcTmplate  = NULL;
    UInt                       sSize       = 0;
    SInt                       sSRID1      = ST_SRID_INIT; 
    SInt                       sSRID2      = ST_SRID_INIT; 
    idBool                     sIsMulti    = ID_FALSE;
    idBool                     sIsExtended = ID_TRUE;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

    /* Calculate Total Size of Arguments */ 
    sValue1 = (stdGeometryHeader *)aStack[1].value;
    sValue2 = (stdGeometryHeader *)aStack[2].value;

    /* Check Null */ 
    if ( (stdGeometry.isNull( NULL, sValue1 ) == ID_TRUE) ||
         (stdGeometry.isNull( NULL, sValue2 ) == ID_TRUE) )
    {
        /* input에 NULL 값이 하나라도 존재하면 NULL 리턴 */  
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        /* Check Geometry Type */ 
        IDE_TEST_RAISE( ((sValue1->mType == STD_EMPTY_TYPE) ||
                         (sValue1->mType == STD_GEOCOLLECTION_2D_TYPE) || 
                         (sValue1->mType == STD_GEOCOLLECTION_2D_EXT_TYPE)) , err_not_supported_object_mType );

        IDE_TEST_RAISE( ((sValue2->mType == STD_EMPTY_TYPE) ||
                         (sValue2->mType == STD_GEOCOLLECTION_2D_TYPE) || 
                         (sValue2->mType == STD_GEOCOLLECTION_2D_EXT_TYPE)) , err_not_supported_object_mType );

        /* Check SRID */ 
        sSRID1 = stfBasic::getSRID( sValue1 );
        sSRID2 = stfBasic::getSRID( sValue2 );
        IDE_TEST_RAISE( sSRID1 != sSRID2, err_mixed_srid );

        /* 확장type의 argument는 기본type으로 변경한다. */ 
        if ( stdUtils::isExtendedType( sValue1->mType ) == ID_TRUE )
        {
            stfBasic::changeExtTypeToBasic( sValue1 );  
        } 
        else 
        {
            sIsExtended = ID_FALSE;
        }

        if ( stdUtils::isExtendedType( sValue2->mType ) == ID_TRUE )
        {
            stfBasic::changeExtTypeToBasic( sValue2 );  
        } 
        else 
        {
            sIsExtended = ID_FALSE;
        }
        
        /* Check multi */ 
        if ( sValue1->mType == sValue2->mType ) 
        {
            /* 두 인자의 type이 같기 때문에 하나의 인자만 확인해도 된다. */ 
            if ( (sValue1->mType == STD_POINT_2D_TYPE) ||
                 (sValue1->mType == STD_LINESTRING_2D_TYPE) ||
                 (sValue1->mType == STD_POLYGON_2D_TYPE) ) 
            {   
                sIsMulti = ID_TRUE; 
            }
            else 
            {
                sIsMulti = ID_FALSE; 
            }
        }
        else
        {
            sIsMulti = ID_FALSE; 
        }

        sRet = (stdGeometryHeader *)aStack[0].value;

        /* Fix BUG-15290 */
        stdUtils::nullify( sRet );

        /* initialize MBR */ 
        stdUtils::initMBR(&sRet->mMbr);

        /* Check Size of Geometry Object for Return */
        /* MultiPoint, MultiLineString, MultiPolygon, GeometryCollection의 사이즈는 모두 동일하므로 
           모두 STD_COLL2D_SIZE로 설정해도 문제 없다. */ 
        sRet->mSize = STD_COLL2D_SIZE; 

        /* Check Geometry Size */ 
        sSize += STD_GEOM_SIZE( sValue1 );
        sSize += STD_GEOM_SIZE( sValue2 );

        if ( (SInt)sRet->mSize > (SInt)( aStack[0].column->precision - sSize ) ) 
        {
            IDE_RAISE(err_object_buffer_overflow);
        }
        sRet->mSize += sSize;

        /* Multi */ 
        if ( sIsMulti == ID_TRUE ) 
        {
            switch( sValue1->mType )
            {
                case STD_POINT_2D_TYPE :
                    if ( sIsExtended == ID_TRUE ) 
                    {
                        sRet->mType = STD_MULTIPOINT_2D_EXT_TYPE;
                        ((stdMultiPoint2DExtType *)sRet)->mSRID = sSRID1;
                    } 
                    else
                    {
                        sRet->mType = STD_MULTIPOINT_2D_TYPE;
                    }
                    ((stdMultiPoint2DType *)sRet)->mNumObjects = 2;

                    sPt2DType = STD_FIRST_POINT2D( (stdMultiPoint2DType *)sRet );
                    stdUtils::copyGeometry( (stdGeometryHeader *)sPt2DType, sValue1 );

                    sPt2DType = STD_NEXT_POINT2D(sPt2DType);
                    stdUtils::copyGeometry( (stdGeometryHeader *)sPt2DType, sValue2 );
                    break;

                case STD_LINESTRING_2D_TYPE :
                    if ( sIsExtended == ID_TRUE ) 
                    {
                        sRet->mType = STD_MULTILINESTRING_2D_EXT_TYPE;
                        ((stdMultiLineString2DExtType *)sRet)->mSRID = sSRID1;
                    } 
                    else
                    {
                        sRet->mType = STD_MULTILINESTRING_2D_TYPE;
                    }
                    ((stdMultiLineString2DType *)sRet)->mNumObjects = 2;

                    sLs2DType = STD_FIRST_LINE2D( (stdMultiLineString2DType *)sRet );
                    stdUtils::copyGeometry( (stdGeometryHeader *)sLs2DType, sValue1 );

                    sLs2DType = STD_NEXT_LINE2D(sLs2DType);
                    stdUtils::copyGeometry( (stdGeometryHeader *)sLs2DType, sValue2 );
                    break;

                case STD_POLYGON_2D_TYPE :
                    if ( sIsExtended == ID_TRUE ) 
                    {
                        sRet->mType = STD_MULTIPOLYGON_2D_EXT_TYPE;
                        ((stdMultiPolygon2DExtType *)sRet)->mSRID = sSRID1;
                    } 
                    else
                    {
                        sRet->mType = STD_MULTIPOLYGON_2D_TYPE;
                    }
                    ((stdMultiPolygon2DType *)sRet)->mNumObjects = 2;

                    sPg2DType = STD_FIRST_POLY2D( (stdMultiPolygon2DType *)sRet );
                    stdUtils::copyGeometry( (stdGeometryHeader *)sPg2DType, sValue1 );

                    sPg2DType = STD_NEXT_POLY2D(sPg2DType);
                    stdUtils::copyGeometry( (stdGeometryHeader *)sPg2DType, sValue2 );
                    break;

                default :
                    IDE_RAISE(err_invalid_object_mType);
                    break;
            }

        }
        /* Collection */ 
        else 
        {
            if ( sIsExtended == ID_TRUE ) 
            {
                sRet->mType = STD_GEOCOLLECTION_2D_EXT_TYPE;
                ((stdGeoCollection2DExtType *)sRet)->mSRID = sSRID1;
            } 
            else
            {
                sRet->mType = STD_GEOCOLLECTION_2D_TYPE;
            }
            ((stdGeoCollection2DType *)sRet)->mNumGeometries = 2;

            sObj = STD_FIRST_COLL2D( (stdGeoCollection2DType *)sRet );
            stdUtils::copyGeometry( (stdGeometryHeader *)sObj, sValue1 );

            sObj = STD_NEXT_GEOM(sObj);
            stdUtils::copyGeometry( (stdGeometryHeader *)sObj, sValue2 );
        }

        /* merge MBR */ 
        IDE_TEST( stdUtils::mergeMBRFromHeader( sRet, sValue1 ) 
                  != IDE_SUCCESS );

        IDE_TEST( stdUtils::mergeMBRFromHeader( sRet, sValue2 ) 
                  != IDE_SUCCESS );

        /* validation check */
        if ( STU_VALIDATION_ENABLE == STU_VALIDATION_ENABLE_TRUE )
        {
            if ( stdPrimitive::validate( sQmxMem, sRet, sRet->mSize )
                 == IDE_SUCCESS )
            {
                sRet->mIsValid = ST_VALID;
            }
            else
            {
                sRet->mIsValid = ST_INVALID;
            }
        }
        else
        {
            sRet->mIsValid = ST_INVALID;
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
    IDE_EXCEPTION(err_not_supported_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }
    IDE_EXCEPTION(err_mixed_srid);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_MIXED_SRID, 
                                sSRID1, sSRID2)); 
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
