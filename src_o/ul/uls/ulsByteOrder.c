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
 * Spatio-Temporal Byte Order 조정 함수
 *
 ***********************************************************************/

#include <ulsByteOrder.h>
#include <ulsCreateObject.h>
#include <ulsUtil.h>

/*----------------------------------------------------------------*
 *  External Interfaces
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   전송받은 Geometry 객체를 System에 맞도록 Byte Order를 조정한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACSRETURN
ulsAdjustByteOrder( ulsHandle         * aHandle,
                    stdGeometryType   * aObject )
{
    acp_bool_t sEquiEndian;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST_RAISE( aObject == NULL, ERR_NULL_PARAMETER );

    /*------------------------------*/
    /* Endian 변경이 필요한지를 검사*/
    /*------------------------------*/

    ACI_TEST( ulsIsEquiEndian( aHandle,
                               (stdGeometryHeader*) aObject,
                               & sEquiEndian ) != ACI_SUCCESS );

    /*------------------------------*/
    /* Endian 변경*/
    /*------------------------------*/

    if ( sEquiEndian == ACP_TRUE )
    {
        /* 동일한 Byte Order임.*/
    }
    else
    {
        /* 서로 다른 Byte Order로 Endian을 변경함.*/
        ACI_TEST( ulsEndian( aHandle, aObject ) != ACS_SUCCESS );
    }
    
    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}

    
/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Geometry 객체의 Endian을 변경한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACSRETURN
ulsEndian( ulsHandle         * aHandle,
           stdGeometryType   * aObject )
{
    stdGeoTypes sType;
    acp_bool_t  sEquiEndian;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST_RAISE( aObject == NULL, ERR_NULL_PARAMETER );

    /*------------------------------*/
    /* Change Endian */
    /*------------------------------*/

    ACI_TEST( ulsGetGeoType( aHandle,
                             (stdGeometryHeader*) aObject,
                             & sType )
              != ACI_SUCCESS );

    ACI_TEST( ulsIsEquiEndian( aHandle,
                               (stdGeometryHeader*) aObject,
                               & sEquiEndian ) != ACI_SUCCESS );
    
    switch ( sType )
    {
        case STD_UNKNOWN_TYPE:
            ACI_RAISE( ERR_INVALID_DATA_TYPE );
            break;
        case STD_POINT_2D_EXT_TYPE:
            ACI_TEST( ulsEndianSRID( aHandle,
                                     &(((stdPoint2DExtType*) aObject)->mSRID) )
                      != ACI_SUCCESS );
        case STD_POINT_2D_TYPE:
            ACI_TEST( ulsEndianPoint2D( aHandle,
                                        sEquiEndian,
                                        (stdPoint2DType*) aObject )
                      != ACI_SUCCESS );
            break;
        case STD_LINESTRING_2D_EXT_TYPE:
            ACI_TEST( ulsEndianSRID( aHandle,
                                     &(((stdLineString2DExtType*) aObject)->mSRID) )
                      != ACI_SUCCESS );
        case STD_LINESTRING_2D_TYPE:
            ACI_TEST( ulsEndianLineString2D( aHandle,
                                             sEquiEndian,
                                             (stdLineString2DType*) aObject )
                      != ACI_SUCCESS );
            break;
        case STD_POLYGON_2D_EXT_TYPE:
            ACI_TEST( ulsEndianSRID( aHandle,
                                     &(((stdPolygon2DExtType*) aObject)->mSRID) )
                      != ACI_SUCCESS );
        case STD_POLYGON_2D_TYPE:
            ACI_TEST( ulsEndianPolygon2D( aHandle,
                                          sEquiEndian,
                                          (stdPolygon2DType*) aObject )
                      != ACI_SUCCESS );
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE:
            ACI_TEST( ulsEndianSRID( aHandle,
                                     &(((stdMultiPoint2DExtType*) aObject)->mSRID) )
                      != ACI_SUCCESS );
        case STD_MULTIPOINT_2D_TYPE:
            ACI_TEST( ulsEndianMultiPoint2D( aHandle,
                                             sEquiEndian,
                                             (stdMultiPoint2DType*) aObject )
                      != ACI_SUCCESS );
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            ACI_TEST( ulsEndianSRID( aHandle,
                                     &(((stdMultiLineString2DExtType*) aObject)->mSRID) )
                      != ACI_SUCCESS );
        case STD_MULTILINESTRING_2D_TYPE:
            ACI_TEST( ulsEndianMultiLineString2D( aHandle,
                                                  sEquiEndian,
                                                  (stdMultiLineString2DType*) aObject )
                      != ACI_SUCCESS );
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            ACI_TEST( ulsEndianSRID( aHandle,
                                     &(((stdMultiPolygon2DExtType*) aObject)->mSRID) )
                      != ACI_SUCCESS );
        case STD_MULTIPOLYGON_2D_TYPE:
            ACI_TEST( ulsEndianMultiPolygon2D( aHandle,
                                               sEquiEndian,
                                               (stdMultiPolygon2DType*) aObject )
                      != ACI_SUCCESS );
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            ACI_TEST( ulsEndianSRID( aHandle,
                                     &(((stdGeoCollection2DExtType*) aObject)->mSRID) )
                      != ACI_SUCCESS );
        case STD_GEOCOLLECTION_2D_TYPE:
            ACI_TEST( ulsEndianGeomCollection2D( aHandle,
                                                 sEquiEndian,
                                                 (stdGeoCollection2DType*) aObject )
                      != ACI_SUCCESS );
            break;
        case STD_NULL_TYPE:
            ulsEndianHeader( aHandle, (stdGeometryHeader * )aObject );
            break;
        case STD_EMPTY_TYPE:
            ulsEndianHeader( aHandle, (stdGeometryHeader * )aObject );
            break;
        default:
            ACI_RAISE( ERR_INVALID_DATA_TYPE );
    }
    
    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( ERR_INVALID_DATA_TYPE );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_DATA_TYPE );
    }

    ACI_EXCEPTION_END;
    
    return ACS_ERROR;
}


/*----------------------------------------------------------------*
 *  Internal Interfaces
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Double Data의 Endian을 변경
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

void
ulsEndianDouble( void* aValue )
{
    acp_uint32_t   sCount;
    acp_uint8_t*   sValue;
    acp_uint8_t    sBuffer[ACI_SIZEOF(acp_double_t)];
    acp_uint32_t   sDoubleSize = ACI_SIZEOF(acp_double_t);

    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aValue != NULL );

    /*------------------------------*/
    /* Change Endian*/
    /*------------------------------*/
    
    sValue = (acp_uint8_t*)aValue;
    for( sCount = 0; sCount < sDoubleSize; sCount++ )
    {
        sBuffer[sDoubleSize - sCount - 1] = sValue[sCount];
    }
    for( sCount = 0; sCount < sDoubleSize; sCount++ )
    {
        sValue[sCount] = sBuffer[sCount];
    }
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Integer Data의 Endian을 변경
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

void
ulsEndianInteger( void* aValue )
{
    acp_uint8_t* sValue;
    acp_uint8_t  sIntermediate;

    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aValue != NULL );

    /*------------------------------*/
    /* Change Endian*/
    /*------------------------------*/
    
    sValue        = (acp_uint8_t*)aValue;
    sIntermediate = sValue[0];
    sValue[0]     = sValue[3];
    sValue[3]     = sIntermediate;
    sIntermediate = sValue[1];
    sValue[1]     = sValue[2];
    sValue[2]     = sIntermediate;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Short Data의 Endian을 변경
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

void
ulsEndianShort( void* aValue )
{
    acp_uint8_t* sValue;
    acp_uint8_t  sIntermediate;

    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aValue != NULL );

    /*------------------------------*/
    /* Change Endian*/
    /*------------------------------*/
    
    sValue        = (acp_uint8_t*)aValue;
    sIntermediate = sValue[0];
    sValue[0]     = sValue[1];
    sValue[1]     = sIntermediate;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   System의 Endian과 Data의 Endian이 동일한지를 판단
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC
ulsIsEquiEndian( ulsHandle         * aHandle,
                 stdGeometryHeader * aObjHeader,
                 acp_bool_t        * aIsEquiEndian )
{
    acp_bool_t sIsEquiEndian;

    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObjHeader != NULL );
    ACE_ASSERT( aIsEquiEndian != NULL );

    /*------------------------------*/
    /* Check Byte Order*/
    /*------------------------------*/
    
    switch ( aObjHeader->mByteOrder )
    {
#ifdef ENDIAN_IS_BIG_ENDIAN

        case STD_BIG_ENDIAN:
            sIsEquiEndian = ACP_TRUE;
            break;
        case STD_LITTLE_ENDIAN:
            sIsEquiEndian = ACP_FALSE;
            break;
            
#else /* ENDIAN_IS_BIG_ENDIAN */
            
        case STD_BIG_ENDIAN:
            sIsEquiEndian = ACP_FALSE;
            break;
        case STD_LITTLE_ENDIAN:
            sIsEquiEndian = ACP_TRUE;
            break;
            
#endif /* ENDIAN_IS_BIG_ENDIAN */
            
        default:
            ACI_RAISE( ERR_INVALID_BYTE_ORDER );
            break;
    }

    /*------------------------------*/
    /* Set Result*/
    /*------------------------------*/
    
    *aIsEquiEndian = sIsEquiEndian;
    
    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_BYTE_ORDER);
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_ACS_INVALID_BYTE_ORDER );
    }
    
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Geometry Header의 Endian을 변경한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC
ulsEndianHeader( ulsHandle         * aHandle,
                 stdGeometryHeader * aObjHeader )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObjHeader != NULL );

    /*------------------------------*/
    /* Change Endian*/
    /*------------------------------*/

    /* Type*/
    ulsEndianShort( & aObjHeader->mType );

    /* Byte Order*/
    aObjHeader->mByteOrder = ( aObjHeader->mByteOrder == STD_BIG_ENDIAN ) ?
        STD_LITTLE_ENDIAN : STD_BIG_ENDIAN;
    
    /* Size*/
    ulsEndianInteger( & aObjHeader->mSize );

    /* MBR*/
    ulsEndianDouble( & aObjHeader->mMbr.mMinX );
    ulsEndianDouble( & aObjHeader->mMbr.mMinY );
    ulsEndianDouble( & aObjHeader->mMbr.mMaxX );
    ulsEndianDouble( & aObjHeader->mMbr.mMaxY );

    return ACI_SUCCESS;
}

/*----------------------------------------------------------------*
 *
 * Description: PROJ-2422 SRID
 *
 *   SRID의 Endian을 변경한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC
ulsEndianSRID( ulsHandle     * aHandle,
               acp_sint32_t  * aSRID )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aSRID != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/

    ulsEndianInteger( aSRID );
    
    return ACI_SUCCESS;
}


/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   2D Point Geometry 객체의 Endian을 변경한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC
ulsEndianPoint2D( ulsHandle         * aHandle,
                  acp_bool_t          aIsEquiEndian,
                  stdPoint2DType    * aObject )
{
    ACP_UNUSED(aIsEquiEndian);
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObject != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/

    ulsEndianDouble( & aObject->mPoint.mX );
    ulsEndianDouble( & aObject->mPoint.mY );
        
    /*------------------------------*/
    /* Change Object Header*/
    /*------------------------------*/

    ACI_TEST( ulsEndianHeader( aHandle,
                               (stdGeometryHeader*) aObject )
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC
ulsEndianLineString2D( ulsHandle              * aHandle,
                       acp_bool_t               aIsEquiEndian,
                       stdLineString2DType    * aObject )
{
    acp_uint32_t  sNumPoints, i;
    stdPoint2D   *sPoints;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObject != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/
    
    sNumPoints = aObject->mNumPoints;
    if( aIsEquiEndian!=ACP_TRUE )
    {
        ulsEndianInteger( &sNumPoints );
    }

    sPoints = ulsM_GetPointsLS2D( aObject );
    for( i = 0; i < sNumPoints; i++ )
    {
        ulsEndianDouble( & sPoints[ i ].mX );
        ulsEndianDouble( & sPoints[ i ].mY );
    }

    /* Chage NumPoints*/
    ulsEndianInteger( &aObject->mNumPoints );
    
    /*------------------------------*/
    /* Change Object Header*/
    /*------------------------------*/

    ACI_TEST( ulsEndianHeader( aHandle,
                               (stdGeometryHeader*) aObject )
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}



ACI_RC
ulsEndianLinearRing2D(  ulsHandle        * aHandle,
                        acp_bool_t         aIsEquiEndian,
                        stdLinearRing2D  * aObject )
{
    acp_uint32_t  sNumPoints, i;
    stdPoint2D   *sPoints;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    /* BUG-28414 : warnning 제거 */ 
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObject != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/
    
    sNumPoints = aObject->mNumPoints;
    if( aIsEquiEndian!=ACP_TRUE )
    {
        ulsEndianInteger( &sNumPoints );
    }

    sPoints = ulsM_GetPointsLR2D( aObject );
    for( i = 0; i < sNumPoints; i++ )
    {
        ulsEndianDouble( & sPoints[ i ].mX );
        ulsEndianDouble( & sPoints[ i ].mY );
    }

    /* Chage NumPoints*/
    ulsEndianInteger( &aObject->mNumPoints );
    
    return ACI_SUCCESS;
}


ACI_RC
ulsEndianPolygon2D(    ulsHandle        * aHandle,
                       acp_bool_t         aIsEquiEndian,
                       stdPolygon2DType * aObject )
{
    acp_uint32_t     sNumRings, i;
    stdLinearRing2D *sLinearRing, *sNextRing;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObject != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/
    
    sNumRings = aObject->mNumRings;
    if( aIsEquiEndian!=ACP_TRUE )
    {
        ulsEndianInteger( &sNumRings );
    }

    ulsSeekFirstRing2D( aHandle, aObject, &sLinearRing );
    for( i = 0; i < sNumRings; i++ )
    {
        if( aIsEquiEndian==ACP_TRUE )
        {
            ACI_TEST (ulsSeekNextRing2D( aHandle, sLinearRing, &sNextRing )
                      != ACI_SUCCESS );
            ulsEndianLinearRing2D( aHandle, aIsEquiEndian, sLinearRing  );
            sLinearRing = sNextRing;
        }
        else
        {
            ulsEndianLinearRing2D( aHandle, aIsEquiEndian, sLinearRing  );
            ACI_TEST (ulsSeekNextRing2D( aHandle, sLinearRing, &sNextRing )
                      != ACI_SUCCESS );
            sLinearRing = sNextRing;
        }
    }

    /* Chage NumPoints*/
    ulsEndianInteger( &aObject->mNumRings );
    
    /*------------------------------*/
    /* Change Object Header*/
    /*------------------------------*/
    ACI_TEST( ulsEndianHeader( aHandle,
                               (stdGeometryHeader*) aObject )
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC
ulsEndianMultiPoint2D( ulsHandle            * aHandle,
                       acp_bool_t             aIsEquiEndian,
                       stdMultiPoint2DType  * aObject )
{
    acp_uint32_t     sNumObj, i;
    stdGeometryType *sSubObj, *sNextObj;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObject != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/
    
    sNumObj  = aObject->mNumObjects;
    if( aIsEquiEndian!=ACP_TRUE )
    {
        ulsEndianInteger( &sNumObj );
    }

    ulsSeekFirstGeometry( aHandle, (stdGeometryType*)aObject, &sSubObj );
    for( i = 0; i < sNumObj; i++ )
    {
        if( aIsEquiEndian==ACP_TRUE )
        {
            ACI_TEST( ulsSeekNextGeometry( aHandle, sSubObj, &sNextObj )
                      != ACI_SUCCESS);
            ulsEndian( aHandle, sSubObj  );
        }
        else
        {
            ulsEndian( aHandle, sSubObj  );
            ACI_TEST( ulsSeekNextGeometry( aHandle, sSubObj, &sNextObj )
                      != ACI_SUCCESS);
        }
        sSubObj = sNextObj;
    }

    /* Chage */
    ulsEndianInteger( &aObject->mNumObjects );
    
    /*------------------------------*/
    /* Change Object Header*/
    /*------------------------------*/
    ACI_TEST( ulsEndianHeader( aHandle,
                               (stdGeometryHeader*) aObject )
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC
ulsEndianMultiLineString2D( ulsHandle                 * aHandle,
                            acp_bool_t                  aIsEquiEndian,
                            stdMultiLineString2DType  * aObject )
{
    acp_uint32_t     sNumObj, i;
    stdGeometryType *sSubObj, *sNextObj;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObject != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/
    
    sNumObj  = aObject->mNumObjects;
    if( aIsEquiEndian!=ACP_TRUE )
    {
        ulsEndianInteger( &sNumObj );
    }

    ulsSeekFirstGeometry( aHandle, (stdGeometryType*)aObject, &sSubObj );
    for( i = 0; i < sNumObj; i++ )
    {
        if( aIsEquiEndian==ACP_TRUE )
        {
            ACI_TEST( ulsSeekNextGeometry( aHandle, sSubObj, &sNextObj )
                      != ACI_SUCCESS);
            ulsEndian( aHandle, sSubObj  );
        }
        else
        {
            ulsEndian( aHandle, sSubObj  );
            ACI_TEST( ulsSeekNextGeometry( aHandle, sSubObj, &sNextObj )
                      != ACI_SUCCESS);
        }
        sSubObj = sNextObj;
    }

    /* Chage */
    ulsEndianInteger( &aObject->mNumObjects );
    
    /*------------------------------*/
    /* Change Object Header*/
    /*------------------------------*/
    ACI_TEST( ulsEndianHeader( aHandle,
                               (stdGeometryHeader*) aObject )
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC
ulsEndianMultiPolygon2D( ulsHandle              * aHandle,
                         acp_bool_t               aIsEquiEndian,
                         stdMultiPolygon2DType  * aObject )
{
    acp_uint32_t     sNumObj, i;
    stdGeometryType *sSubObj, *sNextObj;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObject != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/
    
    sNumObj  = aObject->mNumObjects;
    if( aIsEquiEndian!=ACP_TRUE )
    {
        ulsEndianInteger( &sNumObj );
    }

    ulsSeekFirstGeometry( aHandle, (stdGeometryType*)aObject, &sSubObj );
    for( i = 0; i < sNumObj; i++ )
    {
        if( aIsEquiEndian==ACP_TRUE )
        {
            ACI_TEST( ulsSeekNextGeometry( aHandle, sSubObj, &sNextObj )
                      != ACI_SUCCESS);
            ulsEndian( aHandle, sSubObj  );
        }
        else
        {
            ulsEndian( aHandle, sSubObj  );
            ACI_TEST( ulsSeekNextGeometry( aHandle, sSubObj, &sNextObj )
                      != ACI_SUCCESS);
        }
        sSubObj = sNextObj;
    }

    /* Chage */
    ulsEndianInteger( &aObject->mNumObjects );
    
    /*------------------------------*/
    /* Change Object Header*/
    /*------------------------------*/
    ACI_TEST( ulsEndianHeader( aHandle,
                               (stdGeometryHeader*) aObject )
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC
ulsEndianGeomCollection2D(  ulsHandle               * aHandle,
                            acp_bool_t                aIsEquiEndian,
                            stdGeoCollection2DType  * aObject )
{
    acp_uint32_t     sNumObj, i;
    stdGeometryType *sSubObj, *sNextObj;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aObject != NULL );

    /*------------------------------*/
    /* Change Object Body*/
    /*------------------------------*/
    
    sNumObj  = aObject->mNumGeometries;
    if( aIsEquiEndian!=ACP_TRUE )
    {
        ulsEndianInteger( &sNumObj );
    }

    ulsSeekFirstGeometry( aHandle, (stdGeometryType*)aObject, &sSubObj );
    for( i = 0; i < sNumObj; i++ )
    {
        if( aIsEquiEndian==ACP_TRUE )
        {
            ACI_TEST( ulsSeekNextGeometry( aHandle, sSubObj, &sNextObj )
                       != ACI_SUCCESS);
            ulsEndian( aHandle, sSubObj  );
        }
        else
        {
            ulsEndian( aHandle, sSubObj  );
            ACI_TEST(ulsSeekNextGeometry( aHandle, sSubObj, &sNextObj )
                      != ACI_SUCCESS);
        }
        sSubObj = sNextObj;
    }

    /* Chage */
    ulsEndianInteger( &aObject->mNumGeometries );
    
    /*------------------------------*/
    /* Change Object Header*/
    /*------------------------------*/
    ACI_TEST( ulsEndianHeader( aHandle,
                               (stdGeometryHeader*) aObject )
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


