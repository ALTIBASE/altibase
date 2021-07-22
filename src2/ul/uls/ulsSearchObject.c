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
 *
 * Spatio-Temporal Date 검색 함수
 *
 ***********************************************************************/

#include <ulsSearchObject.h>
#include <ulsCreateObject.h>
#include <ulsByteOrder.h>
#include <ulsUtil.h>

/*----------------------------------------------------------------*
 *  External Interfaces
 *----------------------------------------------------------------*/


/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Geometry 객체 타입을 얻어온다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACSRETURN
ulsGetGeometryType( ulsHandle               * aHandle,
                    stdGeometryType         * aGeometry,
                    stdGeoTypes             * aGeoType )
{
    stdGeoTypes             sGeoType;
    
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST_RAISE( aGeometry  == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aGeoType   == NULL, ERR_NULL_PARAMETER );

    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*)aGeometry,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );


    /* 결과값 설정*/
    *aGeoType = sGeoType;
    
    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}


/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Geometry 객체의 크기를 구한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetGeometrySize(  ulsHandle              * aHandle,
                     stdGeometryType        * aGeometry,
                     ulvSLen                * aSize  )
{
    acp_uint32_t             i;
    acp_uint32_t             sNum;
    
    acp_uint32_t             sTotalSize = 0;
    ulvSLen                  sSizeVSLong;
    acp_uint32_t             sObjSize;
    
    stdLinearRing2D *sRing2D;
    stdGeometryType *sSubGeometry;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    ACI_TEST_RAISE( aSize  == NULL, ERR_NULL_PARAMETER );

    ACI_TEST( ulsCheckGeometry( aHandle, aGeometry ) != ACI_SUCCESS );

    switch( aGeometry->u.header.mType )
    {
        case STD_POINT_2D_EXT_TYPE :
            ulsGetPoint2DExtSize( aHandle, & sObjSize );
            *aSize = sObjSize;
            
            break;
        case STD_POINT_2D_TYPE :
            ulsGetPoint2DSize( aHandle, & sObjSize );
            *aSize = sObjSize;
            
            break;
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
            ulsGetLineString2DSize( aHandle,
                                    aGeometry->u.linestring2D.mNumPoints,
                                    & sObjSize );
            *aSize = sObjSize;
            
            break;
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
            sTotalSize = ACI_SIZEOF( stdPolygon2DType);
            
            sNum = aGeometry->u.polygon2D.mNumRings;
            ACI_TEST( ulsSeekFirstRing2D( aHandle,
                                          &aGeometry->u.polygon2D,
                                          &sRing2D )
                      != ACI_SUCCESS );
            for( i=0; i<sNum; i++ )
            {
                ulsGetLinearRing2DSize( aHandle,
                                        sRing2D->mNumPoints,
                                        &sObjSize );
                sTotalSize += sObjSize;
                ACI_TEST( ulsSeekNextRing2D( aHandle, sRing2D, &sRing2D )
                          != ACI_SUCCESS );
            }
            *aSize = sTotalSize;
            break;
            
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
        case STD_GEOCOLLECTION_2D_TYPE :
            sTotalSize = ACI_SIZEOF( stdMultiPoint2DType );
            sNum = aGeometry->u.mpoint2D.mNumObjects;
            ACI_TEST( ulsSeekFirstGeometry( aHandle,
                                            aGeometry,
                                           &sSubGeometry )
                      != ACI_SUCCESS );
                      
            for( i=0; i<sNum; i++ )
            {
                ACI_TEST( ulsGetGeometrySize( aHandle,
                                              sSubGeometry,
                                              & sSizeVSLong )
                          != ACS_SUCCESS );
                          
                sTotalSize += sSizeVSLong;
                
                sSubGeometry = (stdGeometryType*)
                               ( ((acp_char_t*)sSubGeometry) + sSizeVSLong );
            }
            *aSize = sTotalSize;
            break;
            
/*        
    STD_TEMPORAL_POINT_2D_TYPE      = 2101,
    STD_TEMPORAL_POINT_3D_TYPE      = 3101,
    STD_TEMPORAL_LINESTRING_2D_TYPE = 2103,
    STD_TEMPORAL_LINESTRING_3D_TYPE = 3103,
*/    
        case STD_NULL_TYPE :
        case STD_EMPTY_TYPE  :
            *aSize = ACI_SIZEOF(stdGeometryHeader);
            break;
        default :
            *aSize = 0;
            ACI_RAISE( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
            break;
    }

    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}



/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Geometry 객체내의 갯수를 을 얻어온다.
 *
 * Implementation:
 *   NULL or EMPTY Geometry       : [out] 0
 *   Point, LineString, Polygon   : [out] 1
 *   MultiPoint, MultiLineString,
 *   MultiPolygon, GeomCollection : [out] n
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER
 *                 ERR_ABORT_ACS_NOT_APPLCABLE_GEOMETRY_TYPE
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetNumGeometries( ulsHandle               * aHandle,
                     stdGeometryType         * aGeometry,
                     acp_uint32_t            * aNumGeometries )
{
    stdGeoTypes             sGeoType;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST_RAISE( aNumGeometries  == NULL, ERR_NULL_PARAMETER );

    ACI_TEST( ulsCheckGeometry( aHandle, aGeometry ) != ACI_SUCCESS );
    
    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*)aGeometry,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );

    switch( sGeoType )
    {
        case STD_NULL_TYPE :
        case STD_EMPTY_TYPE :
            *aNumGeometries = 0;
            break;
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
            *aNumGeometries = 1;
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
            *aNumGeometries = ((stdMultiPoint2DType*)aGeometry)->mNumObjects;
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
            *aNumGeometries = ((stdMultiLineString2DType*)aGeometry)->mNumObjects;
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
            *aNumGeometries = ((stdMultiPolygon2DType*)aGeometry)->mNumObjects;
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
        case STD_GEOCOLLECTION_2D_TYPE :
            *aNumGeometries = ((stdGeoCollection2DType*)aGeometry)->mNumGeometries;
            break;
        default :
            ACI_RAISE( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
            break;
    }
    
    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   N번째 Geometry 객체내의 하위 Geomtry를 얻어온다.
 *
 * Implementation:
 *   NULL or EMPTY Geometry       : Error: NOT_APPLCABLE_GEOMETRY_TYPE
 *   Point, LineString, Polygon   : range [1-1] : [out] self
 *   MultiPoint, MultiLineString,
 *   MultiPolygon, GeomCollection : range[ 1 - numGeometries]
 *                                : sub geometry
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER
 *                 ERR_ABORT_ACS_NOT_APPLCABLE_GEOMETRY_TYPE
 *                 ERR_ABORT_ACS_INVALID_PARAMETER_RANGE
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetGeometryN( ulsHandle               * aHandle,
                 stdGeometryType         * aGeometry,
                 acp_uint32_t              aNth,
                 stdGeometryType        ** aSubGeometry )
{
    stdGeoTypes             sGeoType;
    acp_uint32_t            i;
    stdGeometryType       * sSubGeometry;
    
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST( ulsCheckGeometry( aHandle, aGeometry ) != ACI_SUCCESS );
                    
    ACI_TEST_RAISE( aSubGeometry == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aNth<1 , ERR_INVALID_RANGE_PARAMETER );

    
    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*)aGeometry,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );

    switch( sGeoType )
    {
        case STD_NULL_TYPE :
        case STD_EMPTY_TYPE :
            ACI_RAISE( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
            break;
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
            ACI_TEST_RAISE( aNth!=1 , ERR_INVALID_RANGE_PARAMETER );
            *aSubGeometry = (stdGeometryType*) aGeometry;
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
        case STD_GEOCOLLECTION_2D_TYPE :
            ACI_TEST_RAISE( ((stdGeoCollection2DType*)aGeometry)->mNumGeometries < aNth,
                            ERR_INVALID_RANGE_PARAMETER );
                            
            ACI_TEST_RAISE( ulsSeekFirstGeometry( aHandle,
                                                  aGeometry,
                                                  &sSubGeometry )
                            !=ACI_SUCCESS,
                            ERR_INVALID_GEOMETRY );
            for( i=1; i<aNth; i++ )
            {
                ACI_TEST_RAISE( ulsSeekNextGeometry( aHandle,
                                                     sSubGeometry,
                                                    &sSubGeometry )
                                !=ACI_SUCCESS,
                                ERR_INVALID_GEOMETRY );
            }
            *aSubGeometry = sSubGeometry;
            break;
        default :
            ACI_RAISE( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
            break;
    }
    
    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_INVALID_RANGE_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_INVALID_PARAMETER_RANGE );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Polygon Geometry 객체내의 ExteriorRing을 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER
 *                 ERR_ABORT_ACS_NOT_APPLCABLE_GEOMETRY_TYPE
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetExteriorRing2D( ulsHandle                * aHandle,
                      stdPolygon2DType         * aPolygon,
                      stdLinearRing2D         ** aLinearRing )
{
    stdGeoTypes             sGeoType;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST( ulsCheckGeometry( aHandle, (stdGeometryType*) aPolygon )
              != ACI_SUCCESS );
    
    ACI_TEST_RAISE( aLinearRing == NULL, ERR_NULL_PARAMETER );
                    
    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*) aPolygon,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );

    /* Type검사하기*/
    ACI_TEST_RAISE( ( sGeoType != STD_POLYGON_2D_TYPE ) &&
                    ( sGeoType != STD_POLYGON_2D_EXT_TYPE ),
                    ERR_NOT_APPLICABLE_GEOMETRY_TYPE );

    /* To Do*/
    ACI_TEST_RAISE( ulsSeekFirstRing2D( aHandle, aPolygon, aLinearRing )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );
            
    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Polygon Geometry 객체내의 ExteriorRing 갯수를 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER
 *                 ERR_ABORT_ACS_NOT_APPLCABLE_GEOMETRY_TYPE
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetNumInteriorRing2D( ulsHandle               * aHandle,
                         stdPolygon2DType        * aPolygon,
                         acp_uint32_t            * aNumInterinor )
{
    stdGeoTypes             sGeoType;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST( ulsCheckGeometry( aHandle, (stdGeometryType*)aPolygon )
              != ACI_SUCCESS );
    
    ACI_TEST_RAISE( aNumInterinor == NULL, ERR_NULL_PARAMETER );
                    
    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*)aPolygon,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );

    /* Type검사하기*/
    ACI_TEST_RAISE( ( sGeoType != STD_POLYGON_2D_TYPE ) &&
                    ( sGeoType != STD_POLYGON_2D_EXT_TYPE ),
                    ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    
    *aNumInterinor = aPolygon->mNumRings - 1;

    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}


/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Polygon Geometry 객체내의 Nth InteriorRing을 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER
 *                 ERR_ABORT_ACS_NOT_APPLCABLE_GEOMETRY_TYPE
 *                 ERR_ABORT_ACS_INVALID_PARAMETER_RANGE
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetInteriorRingNPolygon2D( ulsHandle                * aHandle,
                              stdPolygon2DType         * aPolygon,
                              acp_uint32_t               aNth,
                              stdLinearRing2D         ** aLinearRing )
{
    stdGeoTypes             sGeoType;
    acp_uint32_t            i;
    stdLinearRing2D       * sLinearRing;

    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST( ulsCheckGeometry( aHandle, (stdGeometryType*)aPolygon )
              != ACI_SUCCESS );
    
    ACI_TEST_RAISE( aLinearRing == NULL, ERR_NULL_PARAMETER );
                    
    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*)aPolygon,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );

    /* Type검사하기*/
    ACI_TEST_RAISE( ( sGeoType != STD_POLYGON_2D_TYPE ) &&
                    ( sGeoType != STD_POLYGON_2D_EXT_TYPE ),
                    ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    
    ACI_TEST_RAISE( aNth >= aPolygon->mNumRings,
                    ERR_INVALID_RANGE_PARAMETER );
            
    ACI_TEST_RAISE( ulsSeekFirstRing2D( aHandle, aPolygon, &sLinearRing )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );
    for( i=1; i <= aNth; i++ )
    {
        ACI_TEST_RAISE( ulsSeekNextRing2D( aHandle, sLinearRing, &sLinearRing )
                        != ACI_SUCCESS,
                        ERR_INVALID_GEOMETRY );
    }
    *aLinearRing = sLinearRing;
 
    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }
    ACI_EXCEPTION( ERR_INVALID_RANGE_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_INVALID_PARAMETER_RANGE );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   LineString Geometry의 Point 갯수를 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER
 *                 ERR_ABORT_ACS_NOT_APPLCABLE_GEOMETRY_TYPE
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetNumPointsLineString2D( ulsHandle                   * aHandle,
                             stdLineString2DType         * aLineString,
                             acp_uint32_t                * aNumPoints )
{
    stdGeoTypes             sGeoType;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST( ulsCheckGeometry( aHandle, (stdGeometryType*)aLineString )
              != ACI_SUCCESS );
    
    ACI_TEST_RAISE( aNumPoints == NULL, ERR_NULL_PARAMETER );
                    
    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*)aLineString,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );

    /* Type검사하기*/
    ACI_TEST_RAISE( ( sGeoType != STD_LINESTRING_2D_TYPE ) &&
                    ( sGeoType != STD_LINESTRING_2D_EXT_TYPE ),
                    ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    
    *aNumPoints = aLineString->mNumPoints;

    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;

}



/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   LineString N번째 Point를 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER
 *                 ERR_ABORT_ACS_NOT_APPLCABLE_GEOMETRY_TYPE
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetPointNLineString2D( ulsHandle                   * aHandle,
                          stdLineString2DType         * aLineString,
                          acp_uint32_t                  aNth,
                          stdPoint2D                  * aPoint )
{
    stdGeoTypes             sGeoType;
    stdPoint2D            * sPoints;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST( ulsCheckGeometry( aHandle, (stdGeometryType*)aLineString )
              != ACI_SUCCESS );
    
    ACI_TEST_RAISE( aPoint == NULL, ERR_NULL_PARAMETER );
                    
    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*)aLineString,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );

    /* Type검사하기*/
    ACI_TEST_RAISE( ( sGeoType != STD_LINESTRING_2D_TYPE ) &&
                    ( sGeoType != STD_LINESTRING_2D_EXT_TYPE ),
                    ERR_NOT_APPLICABLE_GEOMETRY_TYPE );

    /* To Do*/
    ACI_TEST_RAISE( aNth < 1, ERR_INVALID_RANGE_PARAMETER );
    ACI_TEST_RAISE( aNth > aLineString->mNumPoints,
                    ERR_INVALID_RANGE_PARAMETER );
    sPoints = ulsM_GetPointsLS2D( aLineString );
    *aPoint = sPoints[ aNth-1 ];

    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }
    ACI_EXCEPTION( ERR_INVALID_RANGE_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_INVALID_PARAMETER_RANGE );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   LineString Points pointer를 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY
 *                 ERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER
 *                 ERR_ABORT_ACS_NOT_APPLCABLE_GEOMETRY_TYPE
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetPointsLineString2D( ulsHandle                   * aHandle,
                          stdLineString2DType         * aLineString,
                          stdPoint2D                 ** aPoints )
{
    stdGeoTypes             sGeoType;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST( ulsCheckGeometry( aHandle, (stdGeometryType*)aLineString )
              != ACI_SUCCESS );
    
    ACI_TEST_RAISE( aPoints == NULL, ERR_NULL_PARAMETER );
                    
    /* Type 얻어오기*/
    ACI_TEST_RAISE( ulsGetGeoType( aHandle,
                                   (stdGeometryHeader*)aLineString,
                                   &sGeoType )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );

    /* Type검사하기*/
    ACI_TEST_RAISE( ( sGeoType != STD_LINESTRING_2D_TYPE ) &&
                    ( sGeoType != STD_LINESTRING_2D_EXT_TYPE ),
                    ERR_NOT_APPLICABLE_GEOMETRY_TYPE );

    /* To Do*/
    *aPoints = ulsM_GetPointsLS2D( aLineString );

    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_NOT_APPLICABLE_GEOMETRY_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}


/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   LinearRing의 Point 갯수를 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetNumPointsLinearRing2D( ulsHandle                   * aHandle,
                             stdLinearRing2D             * aLinearRing,
                             acp_uint32_t                * aNumPoints )
{
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( aLinearRing == NULL, ERR_NULL_PARAMETER );
    
    ACI_TEST_RAISE( aNumPoints == NULL, ERR_NULL_PARAMETER );
                    
    *aNumPoints = aLinearRing->mNumPoints;

    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   LinearRing의 N번째 Point를 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *                 ERR_ABORT_ACS_INVALID_PARAMETER_RANGE;
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetPointNLinearRing2D( ulsHandle                   * aHandle,
                          stdLinearRing2D             * aLinearRing,
                          acp_uint32_t                  aNth,
                          stdPoint2D                  * aPoint )
{
    stdPoint2D            * sPoints;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST_RAISE( aLinearRing == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aPoint == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aNth < 1, ERR_INVALID_RANGE_PARAMETER );
    ACI_TEST_RAISE( aNth > aLinearRing->mNumPoints,
                    ERR_INVALID_RANGE_PARAMETER );
    
    sPoints  = ulsM_GetPointsLR2D( aLinearRing );
    *aPoint  = sPoints[ aNth - 1 ];

    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_INVALID_RANGE_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_INVALID_PARAMETER_RANGE );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;

}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   LinearRing의 Points pointer를 얻어온다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetPointsLinearRing2D( ulsHandle                   * aHandle,
                          stdLinearRing2D             * aLinearRing,
                          stdPoint2D                 ** aPoints )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST_RAISE( aLinearRing == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aPoints == NULL, ERR_NULL_PARAMETER );
    
    *aPoints  = ulsM_GetPointsLR2D( aLinearRing );

    return ACS_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}

/*----------------------------------------------------------------*
 *
 * Description: BUG-28091
 *
 *   WKB로부터 Geometry의 크기를 얻어 온다.
 *   여기서 사용 되는 Size는 Precision + Geometry Header의 크기이다.
 *   따라서 이 함수를 통해 예로 MultiPolygon이 256의 크기가 나왔다면 실제의 PRECISION은
 *   Geometry Header Size(56)을 뺀 200이 된다.
 *
 * Implementation:
 * Return :
 *     ACS_SUCCESS
 *     ACS_INVALID_HANDLE
 *     ACS_ERROR :
 *                 ERR_ABORT_INVALID_USE_OF_NULL_POINTER
 *---------------------------------------------------------------*/
ACSRETURN
ulsGetGeometrySizeFromWKB( ulsHandle   * aHandle,
                           acp_uint8_t * aWKB,
                           acp_uint32_t  aWKBLength,
                           ulvSLen     * aSize)
{
    acp_uint8_t*   sWKBFence     = (acp_uint8_t*)aWKB + aWKBLength;
    acp_uint32_t   sWkbType;
    acp_uint32_t   sWkbOffset    = 0;
    acp_uint32_t   sObjSize      = 0;
    acp_bool_t     sIsEquiEndian = ACP_FALSE;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );


    if((aWKBLength != 0) && (aWKB != NULL))
    {
        ACI_TEST(readWKB_Header(aHandle,
                                aWKB,
                                &sIsEquiEndian,
                                &sWkbType,
                                &sWkbOffset) != ACI_SUCCESS);

        switch(sWkbType)
        {
            case WKB_POINT_TYPE :
                ACI_TEST_RAISE( ulsGetPointSizeFromWKB( aHandle,
                                                        &aWKB,
                                                        sWKBFence,
                                                        &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case WKB_LINESTRING_TYPE :
                ACI_TEST_RAISE( ulsGetLineStringSizeFromWKB( aHandle,
                                                             &aWKB,
                                                             sWKBFence,
                                                             &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case WKB_POLYGON_TYPE :
                ACI_TEST_RAISE( ulsGetPolygonSizeFromWKB( aHandle,
                                                          &aWKB,
                                                          sWKBFence,
                                                          &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case WKB_MULTIPOINT_TYPE :
                ACI_TEST_RAISE( ulsGetMultiPointSizeFromWKB( aHandle,
                                                             &aWKB,
                                                             sWKBFence,
                                                             &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case WKB_MULTILINESTRING_TYPE :
                ACI_TEST_RAISE( ulsGetMultiLineStringSizeFromWKB( aHandle,
                                                                  &aWKB,
                                                                  sWKBFence,
                                                                  &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case WKB_MULTIPOLYGON_TYPE :
                ACI_TEST_RAISE( ulsGetMultiPolygonSizeFromWKB( aHandle,
                                                               &aWKB,
                                                               sWKBFence,
                                                               &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case WKB_COLLECTION_TYPE :
                ACI_TEST_RAISE( ulsGetGeoCollectionSizeFromWKB( aHandle,
                                                                &aWKB,
                                                                sWKBFence,
                                                                &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case EWKB_POINT_TYPE :
                ACI_TEST_RAISE( ulsGetPointSizeFromEWKB( aHandle,
                                                         &aWKB,
                                                         sWKBFence,
                                                         &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case EWKB_LINESTRING_TYPE :
                ACI_TEST_RAISE( ulsGetLineStringSizeFromEWKB( aHandle,
                                                              &aWKB,
                                                              sWKBFence,
                                                              &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case EWKB_POLYGON_TYPE :
                ACI_TEST_RAISE( ulsGetPolygonSizeFromEWKB( aHandle,
                                                           &aWKB,
                                                           sWKBFence,
                                                           &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case EWKB_MULTIPOINT_TYPE :
                ACI_TEST_RAISE( ulsGetMultiPointSizeFromEWKB( aHandle,
                                                              &aWKB,
                                                              sWKBFence,
                                                              &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case EWKB_MULTILINESTRING_TYPE :
                ACI_TEST_RAISE( ulsGetMultiLineStringSizeFromEWKB( aHandle,
                                                                   &aWKB,
                                                                   sWKBFence,
                                                                   &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case EWKB_MULTIPOLYGON_TYPE :
                ACI_TEST_RAISE( ulsGetMultiPolygonSizeFromEWKB( aHandle,
                                                                &aWKB,
                                                                sWKBFence,
                                                                &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            case EWKB_COLLECTION_TYPE :
                ACI_TEST_RAISE( ulsGetGeoCollectionSizeFromEWKB( aHandle,
                                                                 &aWKB,
                                                                 sWKBFence,
                                                                 &sObjSize )
                                != ACS_SUCCESS, ERR_PARSING );
                break;
            default :
                ACI_RAISE( ERR_INVALID_OBJECT_MTYPE);
        }
    }
    else
    {
        sObjSize = ACI_SIZEOF(stdGeometryHeader);
    }

    *aSize = sObjSize;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }
    ACI_EXCEPTION( ERR_INVALID_OBJECT_MTYPE);
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE));
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/**/
/* interal Function*/
/**/
ACI_RC
ulsCheckGeometry( ulsHandle       * aHandle,
                  stdGeometryType * aGeometry )
{
    acp_bool_t    sIsEquiEndian;
    
    ACI_TEST_RAISE( aGeometry  == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( ulsIsEquiEndian( aHandle,
                                     (stdGeometryHeader*)aGeometry,
                                     &sIsEquiEndian )
                    != ACI_SUCCESS,
                    ERR_INVALID_GEOMETRY );
    ACI_TEST_RAISE( sIsEquiEndian != ACP_TRUE,
                    ERR_INVALID_GEOMETRY_BYTEORDER );

    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_GEOMETRY );
    }
    ACI_EXCEPTION( ERR_INVALID_GEOMETRY_BYTEORDER );
    {
        ulsSetErrorCode( aHandle,
                         ulERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER );
    }
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}
