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
 * Spatio-Temporal 내부 함수
 *
 ***********************************************************************/


#include <ulsByteOrder.h>
#include <ulsUtil.h>
#include <ulsSearchObject.h>

/*BUG - 28091*/
ACI_RC readWKB_Header( ulsHandle       *aHandle,
                       acp_uint8_t     *aBuf,
                       acp_bool_t      *aIsEquiEndian,
                       acp_uint32_t    *aType,
                       acp_uint32_t    *aOffset )
{
    acp_uint8_t* sValue;
    acp_uint8_t  sIntermediate;

    switch ((acp_uint8_t)*aBuf)
    {
#ifdef ENDIAN_IS_BIG_ENDIAN

        case STD_BIG_ENDIAN:
            *aIsEquiEndian = ACP_TRUE;
            break;
        case STD_LITTLE_ENDIAN:
            *aIsEquiEndian = ACP_FALSE;
            break;

#else /* ENDIAN_IS_BIG_ENDIAN */

        case STD_BIG_ENDIAN:
            *aIsEquiEndian = ACP_FALSE;
            break;
        case STD_LITTLE_ENDIAN:
            *aIsEquiEndian = ACP_TRUE;
            break;

#endif /* ENDIAN_IS_BIG_ENDIAN */

        default:
            ACI_RAISE(ERR_PARSING);
    }

    acpMemCpy( aType, aBuf+1, WKB_INT32_SIZE );

    if ( *aIsEquiEndian == ACP_FALSE )
    {
        sValue        = (acp_uint8_t*)aType;
        sIntermediate = sValue[0];
        sValue[0]     = sValue[3];
        sValue[3]     = sIntermediate;
        sIntermediate = sValue[1];
        sValue[1]     = sValue[2];
        sValue[2]     = sIntermediate;
    }

    *aOffset += 5; /*WKB_INT32_SIZE(4) + uchar(1) WKB 고정 사이즈*/

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARSING);
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*BUG - 28091*/
void readWKB_UInt( acp_uint8_t   *aBuf,
                   acp_uint32_t  *aVal,
                   acp_uint32_t  *aOffset,
                   acp_bool_t     aEquiEndian)
{
    acp_uint8_t *sValue;
    acp_uint8_t  sIntermediate;

    acpMemCpy( aVal, aBuf, WKB_INT32_SIZE );

    if (aEquiEndian == ACP_FALSE)
    {
        sValue        = (acp_uint8_t*)aVal;
        sIntermediate = sValue[0];
        sValue[0]     = sValue[3];
        sValue[3]     = sIntermediate;
        sIntermediate = sValue[1];
        sValue[1]     = sValue[2];
        sValue[2]     = sIntermediate;
    }
    *aOffset += WKB_INT32_SIZE;
}

void readWKB_SInt( acp_uint8_t   *aBuf,
                   acp_sint32_t  *aVal,
                   acp_uint32_t  *aOffset,
                   acp_bool_t     aEquiEndian )
{
    acp_sint8_t *sValue;
    acp_uint8_t  sIntermediate;

    acpMemCpy( aVal, aBuf, WKB_INT32_SIZE );

    if ( aEquiEndian == ACP_FALSE )
    {
        sValue        = (acp_sint8_t*)aVal;
        sIntermediate = sValue[0];
        sValue[0]     = sValue[3];
        sValue[3]     = sIntermediate;
        sIntermediate = sValue[1];
        sValue[1]     = sValue[2];
        sValue[2]     = sIntermediate;
    }
    else
    {
        // Nothing To Do
    }

    *aOffset += WKB_INT32_SIZE;
}

stdPoint2D *
ulsM_GetPointsLS2D( stdLineString2DType *aLineString )
{
    ACE_DASSERT( aLineString!=NULL );
    
    return (stdPoint2D*) ( aLineString + 1 );
    /* return aLineString->mPoints;*/
}


stdPoint2D *
ulsM_GetPointsLR2D( stdLinearRing2D *aLinearRing )
{
    ACE_DASSERT( aLinearRing!=NULL );
    
    return (stdPoint2D*) ( aLinearRing + 1 );
    /* return aLinearRing->mPoints;*/
}


void
ulsM_GetMBR2D( acp_uint32_t       aNumPoints,
               stdPoint2D       * aPoints,
               stdMBR           * aMbr )
{
    acp_uint32_t i;
    
    ACE_DASSERT( aPoints != NULL );
    ACE_DASSERT( aNumPoints != 0 );
    ACE_DASSERT( aMbr != NULL );

    aMbr->mMinX = aMbr->mMaxX = aPoints[0].mX;
    aMbr->mMinY = aMbr->mMaxY = aPoints[0].mY;

    for( i=1; i<aNumPoints; i++ )
    {
        if( aMbr->mMinX > aPoints[i].mX )  aMbr->mMinX = aPoints[i].mX;
        if( aMbr->mMaxX < aPoints[i].mX )  aMbr->mMaxX = aPoints[i].mX;
        if( aMbr->mMinY > aPoints[i].mY )  aMbr->mMinY = aPoints[i].mY;
        if( aMbr->mMaxY < aPoints[i].mY )  aMbr->mMaxY = aPoints[i].mY;
    }
}


void
ulsM_ExpandMBR( stdMBR *aMaster, stdMBR *aOther )
{
    ACE_DASSERT( aMaster != NULL );
    ACE_DASSERT( aOther  != NULL );

    if( aMaster->mMinX > aOther->mMinX )
        aMaster->mMinX = aOther->mMinX;
    if( aMaster->mMaxX < aOther->mMaxX )
        aMaster->mMaxX = aOther->mMaxX;
    if( aMaster->mMinY > aOther->mMinY )
        aMaster->mMinY = aOther->mMinY;
    if( aMaster->mMaxY < aOther->mMaxY )
        aMaster->mMaxY = aOther->mMaxY;
}

acp_sint32_t ulsM_IsGeometry2DType( stdGeoTypes aType )
{
    acp_uint32_t sDim = (acp_uint32_t)aType / 1000;
    if( sDim==2 )
        return 1;
    return 0;
}

acp_sint32_t ulsM_IsGeometry2DExtType( stdGeoTypes aType )
{
    acp_uint32_t sDim = (acp_uint32_t)aType / 1000;
    if ( sDim == 2 )
    {
        sDim = (acp_uint32_t)( aType - 2000 ) / 100;
        if( sDim == 5)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // Nothing To Do
    }

    return 0;
}

acp_sint32_t ulsM_GetSRID( stdGeometryType * aGeometry )
{
    acp_sint32_t sSRID = ST_SRID_UNDEFINED;
    
    switch ( aGeometry->u.header.mType )
    {
        case STD_POINT_2D_EXT_TYPE:
            sSRID = aGeometry->u.point2DExt.mSRID;
            break;
        case STD_LINESTRING_2D_EXT_TYPE:
            sSRID = aGeometry->u.linestring2DExt.mSRID;
            break;
        case STD_POLYGON_2D_EXT_TYPE:
            sSRID = aGeometry->u.polygon2DExt.mSRID;
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sSRID = aGeometry->u.mpoint2DExt.mSRID;
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sSRID = aGeometry->u.mlinestring2DExt.mSRID;
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE:   
            sSRID = aGeometry->u.mpolygon2DExt.mSRID;
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            sSRID = aGeometry->u.collection2DExt.mSRID;
            break;
        default:
            break;
    }

    return sSRID;
}

void ulsM_SetSRID( stdGeometryType * aGeometry, acp_sint32_t aSRID )
{
    switch ( aGeometry->u.header.mType )
    {
        case STD_POINT_2D_EXT_TYPE:
            aGeometry->u.point2DExt.mSRID = aSRID;
            break;
        case STD_LINESTRING_2D_EXT_TYPE:
            aGeometry->u.linestring2DExt.mSRID = aSRID;
            break;
        case STD_POLYGON_2D_EXT_TYPE:
            aGeometry->u.polygon2DExt.mSRID = aSRID;
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE:
            aGeometry->u.mpoint2DExt.mSRID = aSRID;
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            aGeometry->u.mlinestring2DExt.mSRID = aSRID;
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE:   
            aGeometry->u.mpolygon2DExt.mSRID = aSRID;
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            aGeometry->u.collection2DExt.mSRID = aSRID;
            break;
        default:
            break;
    }
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Point 객체의 크기를 구한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACI_RC
ulsGetPoint2DSize( ulsHandle          * aHandle,
                   acp_uint32_t       * aSize )
{
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    /* BUG-28414 : warnning 제거 */
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aSize   != NULL );

    /*------------------------------*/
    /* Get Size*/
    /*------------------------------*/
    
    *aSize = ACI_SIZEOF(stdPoint2DType) ;
    
    return ACI_SUCCESS;

    /* ACI_EXCEPTION_END;*/
    
    /* return ACI_FAILURE;*/
}

/*----------------------------------------------------------------*
 *
 * Description: PROJ-2422 SRID
 *
 *   Point 객체의 크기를 구한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACI_RC
ulsGetPoint2DExtSize( ulsHandle          * aHandle,
                      acp_uint32_t       * aSize )
{
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    /* BUG-28414 : warnning 제거 */
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aSize   != NULL );

    /*------------------------------*/
    /* Get Size*/
    /*------------------------------*/
    
    *aSize = ACI_SIZEOF( stdPoint2DExtType ) ;
    
    return ACI_SUCCESS;

    /* ACI_EXCEPTION_END;*/
    
    /* return ACI_FAILURE;*/
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   LineString 객체의 크기를 구한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC ulsGetLineString2DSize( ulsHandle              * aHandle,
                               acp_uint32_t             aNumPoints,
                               acp_uint32_t           * aSize  )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    /* BUG-28414 : warnning 제거 */
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aSize   != NULL );

    /*------------------------------*/
    /* Get Size*/
    /*------------------------------*/
    
    *aSize = ACI_SIZEOF(stdLineString2DType) +
             ACI_SIZEOF(stdPoint2D) * aNumPoints;
    
    return ACI_SUCCESS;

    /*ACI_EXCEPTION_END;*/
    
    /*return ACI_FAILURE;*/
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   LinearRing 객체의 크기를 구한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACI_RC ulsGetLinearRing2DSize( ulsHandle              * aHandle,
                               acp_uint32_t             aNumPoints,
                               acp_uint32_t           * aSize  )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    /* BUG-28414 : warnning 제거 */
    ACE_ASSERT( aHandle != NULL );
    ACE_ASSERT( aSize   != NULL );

    /*------------------------------*/
    /* Get Size*/
    /*------------------------------*/
    
    *aSize = ACI_SIZEOF(stdLinearRing2D)
             + ACI_SIZEOF(stdPoint2D) * aNumPoints;
    
    return ACI_SUCCESS;

    /* ACI_EXCEPTION_END;*/
    
    /* return ACI_FAILURE;*/
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Polygon 객체의 크기를 구한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACI_RC ulsGetPolygon2DSize(    ulsHandle              * aHandle,
                               acp_uint32_t             aNumRings,
                               stdLinearRing2D       ** aRings,
                               acp_uint32_t           * aSize  )
{
    acp_uint32_t sTotalSize, sSize, i;
    
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACE_DASSERT( aHandle != NULL );
    ACE_DASSERT( aSize   != NULL );

    /*------------------------------*/
    /* Get Size*/
    /*------------------------------*/
    
    sTotalSize = ACI_SIZEOF(stdPolygon2DType);
    for( i=0; i<aNumRings; i++ )
    {
        ACI_TEST( aRings[i] == NULL );
        
        ulsGetLinearRing2DSize( aHandle,
                                aRings[i]->mNumPoints,
                                &sSize );
        sTotalSize += sSize;
    }

    *aSize = sTotalSize;
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Polygon의 첫번째Ring의 Pointer를 얻어온다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC
ulsSeekFirstRing2D( ulsHandle                * aHandle,
                    stdPolygon2DType         * aPolygon,
                    stdLinearRing2D         ** aRing )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    /* BUG-28414 : warnning 제거 */
    ACE_ASSERT( aHandle   != NULL );
    ACE_ASSERT( aPolygon  != NULL );
    ACE_ASSERT( aRing     != NULL );

    *aRing = (stdLinearRing2D*)
             ( ( (acp_char_t*)aPolygon ) + ACI_SIZEOF(stdPolygon2DType) );
   
        
    return ACI_SUCCESS;

    /* ACI_EXCEPTION_END;*/
    
    /* return ACI_FAILURE;*/
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Polygon의 기준Ring 의 다음번 Ring Pointer를 얻어온다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC
ulsSeekNextRing2D( ulsHandle                 * aHandle,
                   stdLinearRing2D           * aRing,
                   stdLinearRing2D          ** aNextRing )
{
    acp_uint32_t sSize;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACE_DASSERT( aHandle   != NULL );
    ACE_DASSERT( aRing     != NULL );
    ACE_DASSERT( aNextRing != NULL );

    ACI_TEST( ulsGetLinearRing2DSize( aHandle,
                                      aRing->mNumPoints,
                                      &sSize )!=ACI_SUCCESS );
    
    *aNextRing = (stdLinearRing2D*) (((acp_char_t*)aRing) + sSize );
   
        
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}


/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   MultiGeometry의 처음 Geometry Pointer를 얻어온다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC ulsSeekFirstGeometry(   ulsHandle              * aHandle,
                               stdGeometryType        * aGeometry,
                               stdGeometryType       ** aFirstGeometry )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    /* BUG-28414 : warnning 제거 */
    ACE_ASSERT( aHandle        != NULL );
    ACE_ASSERT( aGeometry      != NULL );
    ACE_ASSERT( aFirstGeometry != NULL );

    *aFirstGeometry = (stdGeometryType*) (((acp_char_t*)aGeometry)
                      + ACI_SIZEOF(stdMultiPoint2DType));
   
        
    return ACI_SUCCESS;

    /* ACI_EXCEPTION_END;*/
    
    /* return ACI_FAILURE;*/
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   MultiGeometry의 기준 Geometry의 다음번 Geometry Pointer를 얻어온다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/
ACI_RC ulsSeekNextGeometry(    ulsHandle              * aHandle,
                               stdGeometryType        * aGeometry,
                               stdGeometryType       ** aNextGeometry )
{
    ulvSLen sSize;
    
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACE_DASSERT( aHandle        != NULL );
    ACE_DASSERT( aGeometry      != NULL );
    ACE_DASSERT( aNextGeometry != NULL );

    ACI_TEST( ulsGetGeometrySize( aHandle,
                                  aGeometry,
                                  &sSize )!=ACS_SUCCESS );

    *aNextGeometry = (stdGeometryType*) (((acp_char_t*)aGeometry) + sSize );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/***********************************************************************
 * Description: BUG-28091
 * WKB(Well Known Binary)를 읽어들여 크기를 구한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 * ulsHandle *aHandle(In): 에러등을 기록 하는 등의 환경에 대한 핸들이다.
 * acp_uint8_t **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * acp_uint8_t * aFence(In): WKB 버퍼의 Fence
 * acp_uint32_t  * aSize(Out):  Size
 **********************************************************************/
ACSRETURN
ulsGetPointSizeFromWKB( ulsHandle    * aHandle,
                        acp_uint8_t ** aPtr,
                        acp_uint8_t  * aFence,
                        acp_uint32_t * aSize)
{
    acp_uint8_t*            sPtr       = *aPtr;
    acp_uint32_t            sWKBSize   = WKB_POINT_SIZE;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( sPtr + sWKBSize > aFence, ERR_PARSING );

    *aPtr    = sPtr + sWKBSize;
    *aSize   = STD_POINT2D_SIZE;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

ACSRETURN
ulsGetPointSizeFromEWKB( ulsHandle    * aHandle,
                         acp_uint8_t ** aPtr,
                         acp_uint8_t  * aFence,
                         acp_uint32_t * aSize )
{
    acp_uint8_t*            sPtr       = *aPtr;
    acp_uint32_t            sWKBSize   = EWKB_POINT_SIZE;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( sPtr + sWKBSize > aFence, ERR_PARSING );

    *aPtr    = sPtr + sWKBSize;
    *aSize   = STD_POINT2D_EXT_SIZE;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB) );
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/***********************************************************************
 * Description: BUG-28091
 * WKB(Well Known Binary)를 읽어들여 크기를 구한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 * ulsHandle *aHandle(In): 에러등을 기록 하는 등의 환경에 대한 핸들이다.
 * acp_uint8_t **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * acp_uint8_t * aFence(In): WKB 버퍼의 Fence
 * acp_uint32_t  * aSize(Out):  Size
 **********************************************************************/
ACSRETURN
ulsGetLineStringSizeFromWKB( ulsHandle    * aHandle,
                             acp_uint8_t ** aPtr,
                             acp_uint8_t  * aFence,
                             acp_uint32_t * aSize)
{
    acp_uint8_t*                    sPtr        = *aPtr;
    acp_uint32_t                    sPtCnt      = 0;
    acp_uint32_t                    sWKBSize    = 0;
    acp_uint32_t                    sWkbOffset  = 0;
    acp_uint32_t                    sWkbType;
    acp_bool_t                      sEquiEndian = ACP_FALSE;
    WKBLineString*                  sBLine      = (WKBLineString*)*aPtr;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( (sPtr + WKB_GEOHEAD_SIZE + WKB_INT32_SIZE) > aFence,
                    ERR_PARSING );
    /* { Calc WKB Size*/
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBLine,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS);

    ACI_TEST_RAISE( sWkbType != WKB_LINESTRING_TYPE , ERR_PARSING );
    readWKB_UInt( sBLine->mNumPoints, &sPtCnt, &sWkbOffset, sEquiEndian);
    sWKBSize = WKB_LINE_SIZE + WKB_PT_SIZE*sPtCnt;
    /* } Calc WKB Size*/

    ACI_TEST_RAISE( sPtr + sWKBSize > aFence, ERR_PARSING );

    *aPtr    = *aPtr + sWKBSize;
    *aSize   = STD_LINE2D_SIZE + STD_PT2D_SIZE*sPtCnt;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

ACSRETURN
ulsGetLineStringSizeFromEWKB( ulsHandle    * aHandle,
                              acp_uint8_t ** aPtr,
                              acp_uint8_t  * aFence,
                              acp_uint32_t * aSize )
{
    acp_uint8_t*                    sPtr        = *aPtr;
    acp_uint32_t                    sPtCnt      = 0;
    acp_uint32_t                    sWKBSize    = 0;
    acp_uint32_t                    sWkbOffset  = 0;
    acp_uint32_t                    sWkbType;
    acp_bool_t                      sEquiEndian = ACP_FALSE;
    EWKBLineString*                 sBLine      = (EWKBLineString*)*aPtr;
    acp_sint32_t                    sSRID;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( ( sPtr + WKB_GEOHEAD_SIZE + EWKB_SRID_SIZE + WKB_INT32_SIZE ) > aFence,
                    ERR_PARSING );
    /* { Calc WKB Size*/
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBLine,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS);

    ACI_TEST_RAISE( sWkbType != EWKB_LINESTRING_TYPE , ERR_PARSING );
    readWKB_SInt( sBLine->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBLine->mNumPoints, &sPtCnt, &sWkbOffset, sEquiEndian );
    sWKBSize = EWKB_LINE_SIZE + WKB_PT_SIZE*sPtCnt;
    /* } Calc WKB Size*/

    ACI_TEST_RAISE( sPtr + sWKBSize > aFence, ERR_PARSING );

    *aPtr    = *aPtr + sWKBSize;
    *aSize   = STD_LINE2D_EXT_SIZE + STD_PT2D_SIZE*sPtCnt;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB) );
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/***********************************************************************
 * Description: BUG-28091
 * WKB(Well Known Binary)를 읽어들여 크기를 구한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 * ulsHandle *aHandle(In): 에러등을 기록 하는 등의 환경에 대한 핸들이다.
 * acp_uint8_t **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * acp_uint8_t * aFence(In): WKB 버퍼의 Fence
 * acp_uint32_t  * aSize(Out):  Size
 **********************************************************************/
ACSRETURN
ulsGetPolygonSizeFromWKB( ulsHandle    * aHandle,
                          acp_uint8_t ** aPtr,
                          acp_uint8_t  * aFence,
                          acp_uint32_t * aSize)
{
    acp_uint8_t*                sPtr        = *aPtr;
    acp_uint32_t                sPtCnt      = 0;
    acp_uint32_t                sRingCnt    = 0;
    acp_uint32_t                sWKBSize    = 0;
    acp_uint32_t                i;
    WKBPolygon*                 sBPolygon   = (WKBPolygon*)*aPtr;
    wkbLinearRing*              sBRing      = NULL;
    acp_uint32_t                sWkbOffset  = 0;
    acp_uint32_t                sWkbType;
    acp_uint32_t                sNumPoints;
    acp_bool_t                  sEquiEndian = ACP_FALSE;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( (sPtr + WKB_GEOHEAD_SIZE + WKB_INT32_SIZE) > aFence,
                    ERR_PARSING );
    /* { Calc WKB Size*/
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBPolygon,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset) != ACI_SUCCESS );
    ACI_TEST_RAISE( sWkbType != WKB_POLYGON_TYPE, ERR_PARSING );
    readWKB_UInt( sBPolygon->mNumRings, &sRingCnt, &sWkbOffset, sEquiEndian);

    sBRing = WKB_FIRST_RN( sBPolygon );
    ACI_TEST_RAISE( (acp_uint8_t*)sBRing > aFence, ERR_PARSING );

    for(i = 0; i < sRingCnt; i++)
    {
        /* check valid position of numpoints*/
        ACI_TEST_RAISE( sBRing->mNumPoints > aFence, ERR_PARSING );

        readWKB_UInt( sBRing->mNumPoints, &sNumPoints, &sWkbOffset, sEquiEndian );

        sPtCnt += sNumPoints;
        /* check valid postion of next ring.*/
        ACI_TEST_RAISE( (sPtr + sWkbOffset + WKB_PT_SIZE*sNumPoints) > aFence,
                        ERR_PARSING );
        sBRing = (wkbLinearRing*)((acp_uint8_t*)(sBRing) +
                                  WKB_RN_SIZE + WKB_PT_SIZE*sNumPoints);
    }

    sWKBSize = WKB_POLY_SIZE + WKB_INT32_SIZE*sRingCnt + WKB_PT_SIZE*sPtCnt;
    /* } Calc WKB Size*/

    ACI_TEST_RAISE(sPtr + sWKBSize > aFence, ERR_PARSING );

    *aPtr    = sPtr + sWKBSize;
    *aSize   = STD_POLY2D_SIZE + STD_RN2D_SIZE*sRingCnt + STD_PT2D_SIZE*sPtCnt;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

ACSRETURN
ulsGetPolygonSizeFromEWKB( ulsHandle    * aHandle,
                           acp_uint8_t ** aPtr,
                           acp_uint8_t  * aFence,
                           acp_uint32_t * aSize )
{
    acp_uint8_t*                sPtr        = *aPtr;
    acp_uint32_t                sPtCnt      = 0;
    acp_uint32_t                sRingCnt    = 0;
    acp_uint32_t                sWKBSize    = 0;
    acp_uint32_t                i;
    EWKBPolygon*                sBPolygon   = (EWKBPolygon*)*aPtr;
    wkbLinearRing*              sBRing      = NULL;
    acp_uint32_t                sWkbOffset  = 0;
    acp_uint32_t                sWkbType;
    acp_uint32_t                sNumPoints;
    acp_bool_t                  sEquiEndian = ACP_FALSE;
    acp_sint32_t                sSRID;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( ( sPtr + WKB_GEOHEAD_SIZE + EWKB_SRID_SIZE + WKB_INT32_SIZE ) > aFence,
                    ERR_PARSING );
    /* { Calc WKB Size*/
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBPolygon,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS );
    ACI_TEST_RAISE( sWkbType != EWKB_POLYGON_TYPE, ERR_PARSING );
    readWKB_SInt( sBPolygon->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBPolygon->mNumRings, &sRingCnt, &sWkbOffset, sEquiEndian );

    sBRing = EWKB_FIRST_RN( sBPolygon );
    ACI_TEST_RAISE( (acp_uint8_t*)sBRing > aFence, ERR_PARSING );

    for ( i = 0; i < sRingCnt; i++ )
    {
        /* check valid position of numpoints*/
        ACI_TEST_RAISE( sBRing->mNumPoints > aFence, ERR_PARSING );

        readWKB_UInt( sBRing->mNumPoints, &sNumPoints, &sWkbOffset, sEquiEndian );

        sPtCnt += sNumPoints;
        /* check valid postion of next ring.*/
        ACI_TEST_RAISE( ( sPtr + sWkbOffset + WKB_PT_SIZE*sNumPoints ) > aFence,
                        ERR_PARSING );
        sBRing = (wkbLinearRing*)( (acp_uint8_t*)(sBRing) +
                                   WKB_RN_SIZE + WKB_PT_SIZE*sNumPoints );
    }

    sWKBSize = EWKB_POLY_SIZE + WKB_INT32_SIZE*sRingCnt + WKB_PT_SIZE*sPtCnt;
    /* } Calc WKB Size*/

    ACI_TEST_RAISE( sPtr + sWKBSize > aFence, ERR_PARSING );

    *aPtr    = sPtr + sWKBSize;
    *aSize   = STD_POLY2D_EXT_SIZE + STD_RN2D_SIZE*sRingCnt + STD_PT2D_SIZE*sPtCnt;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB) );
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/***********************************************************************
 * Description: BUG-28091
 * WKB(Well Known Binary)를 읽어들여 크기를 구한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 * ulsHandle *aHandle(In): 에러등을 기록 하는 등의 환경에 대한 핸들이다.
 * acp_uint8_t **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * acp_uint8_t * aFence(In): WKB 버퍼의 Fence
 * acp_uint32_t  * aSize(Out):  Size
 **********************************************************************/
ACSRETURN
ulsGetMultiPointSizeFromWKB( ulsHandle    * aHandle,
                             acp_uint8_t ** aPtr,
                             acp_uint8_t  * aFence,
                             acp_uint32_t * aSize)
{
    acp_uint8_t*                    sPtr        = *aPtr;
    acp_uint32_t                    sPtCnt      = 0;
    acp_uint32_t                    sWKBSize    = 0;
    WKBMultiPoint*                  sBMpoint    = (WKBMultiPoint*)*aPtr;
    acp_uint32_t                    sWkbOffset  = 0;
    acp_uint32_t                    sWkbType;
    acp_bool_t                      sEquiEndian = ACP_FALSE;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    /* { Calc WKB Size*/
    ACI_TEST_RAISE( (sPtr + WKB_GEOHEAD_SIZE + WKB_INT32_SIZE) > aFence,
                    ERR_PARSING );
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBMpoint,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS);
    ACI_TEST_RAISE( sWkbType != WKB_MULTIPOINT_TYPE , ERR_PARSING );
    readWKB_UInt( sBMpoint->mNumWKBPoints, &sPtCnt, &sWkbOffset, sEquiEndian );

    sWKBSize = WKB_MPOINT_SIZE + WKB_POINT_SIZE*sPtCnt;
    /* } Calc WKB Size*/

    ACI_TEST_RAISE( sPtr + sWKBSize > aFence, ERR_PARSING );

    *aPtr  = sPtr + sWKBSize;
    *aSize = STD_MPOINT2D_SIZE + STD_POINT2D_SIZE * sPtCnt;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/***********************************************************************
 * Description:BUG-28091
 * WKB(Well Known Binary)를 읽어들여 크기를 구한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 * ulsHandle *aHandle(In): 에러등을 기록 하는 등의 환경에 대한 핸들이다.
 * acp_uint8_t **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * acp_uint8_t * aFence(In): WKB 버퍼의 Fence
 * acp_uint32_t  * aSize(Out):  Size
 **********************************************************************/
ACSRETURN
ulsGetMultiLineStringSizeFromWKB( ulsHandle    * aHandle,
                                  acp_uint8_t ** aPtr,
                                  acp_uint8_t  * aFence,
                                  acp_uint32_t * aSize)
{
    acp_uint8_t*                        sPtr        = *aPtr;
    acp_uint32_t                        sObjectCnt;
    acp_uint32_t                        sTotalSize  = 0;
    acp_uint32_t                        i;
    acp_uint32_t                        sWkbOffset  = 0;
    acp_uint32_t                        sWkbSize;
    acp_uint32_t                        sWkbType;
    acp_uint32_t                        sTempSize;
    WKBMultiLineString*                 sBMLine     = (WKBMultiLineString*)*aPtr;
    WKBLineString*                      sBLine;
    acp_bool_t                          sEquiEndian = ACP_FALSE;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( (sPtr + WKB_GEOHEAD_SIZE + WKB_INT32_SIZE) > aFence,
                    ERR_PARSING );
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBMLine,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS);
    ACI_TEST_RAISE( sWkbType != WKB_MULTILINESTRING_TYPE, ERR_PARSING );
    readWKB_UInt( sBMLine->mNumWKBLineStrings, &sObjectCnt, &sWkbOffset,
                  sEquiEndian);

    sBLine = WKB_FIRST_LINE(sBMLine);
    for(i = 0; i < sObjectCnt; i++)
    {
        ACI_TEST_RAISE( (sPtr + sWkbOffset + WKB_GEOHEAD_SIZE) > aFence,
                        ERR_PARSING);
        ACI_TEST( readWKB_Header( aHandle,
                                  (acp_uint8_t*)sBLine,
                                  &sEquiEndian,
                                  &sWkbType,
                                  &sWkbOffset ) != ACI_SUCCESS);
        ACI_TEST_RAISE( sWkbType != WKB_LINESTRING_TYPE, ERR_PARSING );

        sWkbOffset -= WKB_GEOHEAD_SIZE;

        ACI_TEST( ulsGetLineStringSizeFromWKB( aHandle,
                                               (acp_uint8_t**)&sBLine,
                                               aFence,
                                               &sTempSize) != ACS_SUCCESS );

        sTotalSize += sTempSize;
    } /* while*/

    sWkbSize   = ( ((acp_char_t*)sBLine) - ((acp_char_t*)sBMLine) );
    sTotalSize += STD_MLINE2D_SIZE;


    ACI_TEST( (sPtr + sWkbSize) > aFence);
    *aPtr  = sPtr + sWkbSize;
    *aSize = sTotalSize;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/***********************************************************************
 * Description:BUG-28091
 * WKB(Well Known Binary)를 읽어들여 크기를 구한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 * ulsHandle *aHandle(In): 에러등을 기록 하는 등의 환경에 대한 핸들이다.
 * acp_uint8_t **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * acp_uint8_t * aFence(In): WKB 버퍼의 Fence
 * acp_uint32_t  * aSize(Out):  Size
 **********************************************************************/
ACSRETURN
ulsGetMultiPolygonSizeFromWKB( ulsHandle    * aHandle,
                               acp_uint8_t ** aPtr,
                               acp_uint8_t  * aFence,
                               acp_uint32_t * aSize)
{
    acp_uint8_t*                    sPtr        = *aPtr;
    acp_uint32_t                    sObjectCnt;
    acp_uint32_t                    sTotalSize  = 0;
    acp_uint32_t                    i;
    acp_uint32_t                    sWkbSize;
    acp_uint32_t                    sWkbOffset  = 0;
    acp_uint32_t                    sWkbType;
    acp_uint32_t                    sTempSize;
    WKBMultiPolygon*                sBMPolygon  = (WKBMultiPolygon*)*aPtr;
    WKBPolygon*                     sBPolygon;
    acp_bool_t                      sEquiEndian = ACP_FALSE;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( (sPtr + WKB_GEOHEAD_SIZE + WKB_INT32_SIZE) > aFence,
                    ERR_PARSING );
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBMPolygon,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS);
    ACI_TEST_RAISE( sWkbType != WKB_MULTIPOLYGON_TYPE, ERR_PARSING );
    readWKB_UInt( sBMPolygon->mNumWKBPolygons, &sObjectCnt, &sWkbOffset,
                  sEquiEndian);

    sBPolygon = WKB_FIRST_POLY(sBMPolygon);
    for(i = 0; i < sObjectCnt; i++)
    {
        ACI_TEST_RAISE( (sPtr + sWkbOffset + WKB_GEOHEAD_SIZE) > aFence,
                        ERR_PARSING );
        ACI_TEST( readWKB_Header( aHandle,
                                  (acp_uint8_t*)sBPolygon,
                                  &sEquiEndian,
                                  &sWkbType,
                                  &sWkbOffset ) != ACI_SUCCESS);
        ACI_TEST_RAISE( sWkbType != WKB_POLYGON_TYPE, ERR_PARSING );
        sWkbOffset -= WKB_GEOHEAD_SIZE;

        ACI_TEST( ulsGetPolygonSizeFromWKB( aHandle,
                                            (acp_uint8_t**)&sBPolygon,
                                            aFence,
                                            &sTempSize) != ACS_SUCCESS );

        sTotalSize += sTempSize;

    } /* while*/

    sWkbSize   = ( ((acp_char_t*)sBPolygon) - ((acp_char_t*)sBMPolygon) );
    sTotalSize += STD_MPOLY2D_SIZE;

    ACI_TEST( (sPtr + sWkbSize) > aFence);

    *aPtr  = sPtr + sWkbSize;
    *aSize = sTotalSize;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/***********************************************************************
 * Description:BUG-28091
 * WKB(Well Known Binary)를 읽어들여 크기를 구한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 * ulsHandle *aHandle(In): 에러등을 기록 하는 등의 환경에 대한 핸들이다.
 * acp_uint8_t **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * acp_uint8_t * aFence(In): WKB 버퍼의 Fence
 * acp_uint32_t  * aSize(Out):  Size
 **********************************************************************/
ACSRETURN
ulsGetGeoCollectionSizeFromWKB( ulsHandle    * aHandle,
                                acp_uint8_t ** aPtr,
                                acp_uint8_t  * aFence,
                                acp_uint32_t * aSize)
{
    acp_uint8_t*                    sPtr        = *aPtr;
    acp_uint32_t                    sObjectCnt;
    acp_uint32_t                    sTotalSize  = 0;
    acp_uint32_t                    sWkbOffset  = 0;
    acp_uint32_t                    sWkbType;
    acp_uint32_t                    sSize;
    acp_uint32_t                    i;
    WKBGeometryCollection*          sBColl      = (WKBGeometryCollection*)*aPtr;
    WKBGeometry*                    sBCurrObj;
    acp_bool_t                      sEquiEndian = ACP_FALSE;
    

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( (sPtr + WKB_GEOHEAD_SIZE + WKB_INT32_SIZE) > aFence, ERR_PARSING);
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBColl,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS);
    ACI_TEST_RAISE( sWkbType != WKB_COLLECTION_TYPE, ERR_PARSING );
    readWKB_UInt( sBColl->mNumWKBGeometries, &sObjectCnt, &sWkbOffset, sEquiEndian );

    sBCurrObj = WKB_FIRST_COLL(sBColl);

    for(i = 0; i < sObjectCnt; i++)
    {
        ACI_TEST( readWKB_Header( aHandle,
                                  (acp_uint8_t*)sBCurrObj,
                                  &sEquiEndian,
                                  &sWkbType,
                                  &sWkbOffset ) != ACI_SUCCESS);
        sWkbOffset -= WKB_GEOHEAD_SIZE;

        switch( sWkbType )
        {
            case WKB_POINT_TYPE :
                ACI_TEST( ulsGetPointSizeFromWKB( aHandle,
                                                  (acp_uint8_t**)&sBCurrObj,
                                                  aFence,
                                                  &sSize) != ACS_SUCCESS );
                break;
            case WKB_LINESTRING_TYPE :
                ACI_TEST( ulsGetLineStringSizeFromWKB( aHandle,
                                                       (acp_uint8_t**)&sBCurrObj,
                                                       aFence,
                                                       &sSize) != ACS_SUCCESS );
                break;
            case WKB_POLYGON_TYPE :
                ACI_TEST( ulsGetPolygonSizeFromWKB( aHandle,
                                                    (acp_uint8_t**)&sBCurrObj,
                                                    aFence,
                                                    &sSize) != ACS_SUCCESS );
                break;
            case WKB_MULTIPOINT_TYPE :
                ACI_TEST( ulsGetMultiPointSizeFromWKB( aHandle,
                                                       (acp_uint8_t**)&sBCurrObj,
                                                       aFence,
                                                       &sSize) != ACS_SUCCESS );
                break;
            case WKB_MULTILINESTRING_TYPE :
                ACI_TEST( ulsGetMultiLineStringSizeFromWKB( aHandle,
                                                            (acp_uint8_t**)&sBCurrObj,
                                                            aFence,
                                                            &sSize) != ACS_SUCCESS );
                break;
            case WKB_MULTIPOLYGON_TYPE :
                ACI_TEST( ulsGetMultiPolygonSizeFromWKB( aHandle,
                                                         (acp_uint8_t**)&sBCurrObj,
                                                         aFence,
                                                         &sSize) != ACS_SUCCESS );
                break;
            default :
                ACI_RAISE( ERR_INVALID_DATA_TYPE );
        } /* switch*/
        sTotalSize += sSize;
    } /* while*/

    sTotalSize += STD_COLL2D_SIZE;

    *aPtr  = sPtr;
    *aSize = sTotalSize;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB));
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION( ERR_INVALID_DATA_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_DATA_TYPE) );
    }
    
    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

ACSRETURN
ulsGetMultiPolygonSizeFromEWKB( ulsHandle    * aHandle,
                                acp_uint8_t ** aPtr,
                                acp_uint8_t  * aFence,
                                acp_uint32_t * aSize )
{
    acp_uint8_t*                    sPtr        = *aPtr;
    acp_uint32_t                    sObjectCnt;
    acp_uint32_t                    sTotalSize  = 0;
    acp_uint32_t                    i;
    acp_uint32_t                    sWkbSize;
    acp_uint32_t                    sWkbOffset  = 0;
    acp_uint32_t                    sWkbType;
    acp_uint32_t                    sTempSize;
    EWKBMultiPolygon*               sBMPolygon  = (EWKBMultiPolygon*)*aPtr;
    WKBPolygon*                     sBPolygon;
    acp_bool_t                      sEquiEndian = ACP_FALSE;
    acp_sint32_t                    sSRID;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( ( sPtr + WKB_GEOHEAD_SIZE + EWKB_SRID_SIZE + WKB_INT32_SIZE ) > aFence,
                    ERR_PARSING );
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBMPolygon,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS );
    ACI_TEST_RAISE( sWkbType != EWKB_MULTIPOLYGON_TYPE, ERR_PARSING );
    readWKB_SInt( sBMPolygon->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBMPolygon->mNumWKBPolygons, &sObjectCnt, &sWkbOffset,
                  sEquiEndian );

    sBPolygon = EWKB_FIRST_POLY( sBMPolygon );
    for ( i = 0; i < sObjectCnt; i++ )
    {
        ACI_TEST_RAISE( ( sPtr + sWkbOffset + WKB_GEOHEAD_SIZE ) > aFence,
                        ERR_PARSING );
        ACI_TEST( readWKB_Header( aHandle,
                                  (acp_uint8_t*)sBPolygon,
                                  &sEquiEndian,
                                  &sWkbType,
                                  &sWkbOffset ) != ACI_SUCCESS );
        ACI_TEST_RAISE( sWkbType != WKB_POLYGON_TYPE, ERR_PARSING );
        sWkbOffset -= WKB_GEOHEAD_SIZE;

        switch( sWkbType )
        {
            case WKB_POLYGON_TYPE :
                ACI_TEST( ulsGetPolygonSizeFromWKB( aHandle,
                                                    (acp_uint8_t**)&sBPolygon,
                                                    aFence,
                                                    &sTempSize ) != ACS_SUCCESS );
                break;
            default :
                ACI_RAISE( ACI_EXCEPTION_END_LABEL );
        } /* switch*/

        sTotalSize += sTempSize;

    } /* while*/

    sWkbSize   = ( ((acp_char_t*)sBPolygon) - ((acp_char_t*)sBMPolygon) );
    sTotalSize += STD_MPOLY2D_EXT_SIZE;

    ACI_TEST( ( sPtr + sWkbSize ) > aFence);

    *aPtr  = sPtr + sWkbSize;
    *aSize = sTotalSize;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB) );
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

ACSRETURN
ulsGetMultiPointSizeFromEWKB( ulsHandle    * aHandle,
                              acp_uint8_t ** aPtr,
                              acp_uint8_t  * aFence,
                              acp_uint32_t * aSize )
{
    acp_uint8_t*                    sPtr        = *aPtr;
    acp_uint32_t                    sPtCnt      = 0;
    acp_uint32_t                    sWKBSize    = 0;
    EWKBMultiPoint*                 sBMpoint    = (EWKBMultiPoint*)*aPtr;
    acp_uint32_t                    sWkbOffset  = 0;
    acp_uint32_t                    sWkbType;
    acp_bool_t                      sEquiEndian = ACP_FALSE;
    acp_sint32_t                    sSRID;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    /* { Calc WKB Size*/
    ACI_TEST_RAISE( ( sPtr + WKB_GEOHEAD_SIZE + EWKB_SRID_SIZE + WKB_INT32_SIZE ) > aFence,
                    ERR_PARSING );
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBMpoint,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS );
    ACI_TEST_RAISE( sWkbType != EWKB_MULTIPOINT_TYPE , ERR_PARSING );
    readWKB_SInt( sBMpoint->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBMpoint->mNumWKBPoints, &sPtCnt, &sWkbOffset, sEquiEndian );

    sWKBSize = EWKB_MPOINT_SIZE + WKB_POINT_SIZE*sPtCnt;
    /* } Calc WKB Size*/

    ACI_TEST_RAISE( sPtr + sWKBSize > aFence, ERR_PARSING );

    *aPtr  = sPtr + sWKBSize;
    *aSize = STD_MPOINT2D_EXT_SIZE + STD_POINT2D_SIZE * sPtCnt;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB) );
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

ACSRETURN
ulsGetMultiLineStringSizeFromEWKB( ulsHandle    * aHandle,
                                   acp_uint8_t ** aPtr,
                                   acp_uint8_t  * aFence,
                                   acp_uint32_t * aSize )
{
    acp_uint8_t*                        sPtr        = *aPtr;
    acp_uint32_t                        sObjectCnt;
    acp_uint32_t                        sTotalSize  = 0;
    acp_uint32_t                        i;
    acp_uint32_t                        sWkbOffset  = 0;
    acp_uint32_t                        sWkbSize;
    acp_uint32_t                        sWkbType;
    acp_uint32_t                        sTempSize;
    EWKBMultiLineString*                sBMLine     = (EWKBMultiLineString*)*aPtr;
    WKBLineString*                      sBLine;
    acp_bool_t                          sEquiEndian = ACP_FALSE;
    acp_sint32_t                        sSRID;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( ( sPtr + WKB_GEOHEAD_SIZE + EWKB_SRID_SIZE + WKB_INT32_SIZE ) > aFence,
                    ERR_PARSING );
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBMLine,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS );
    ACI_TEST_RAISE( sWkbType != EWKB_MULTILINESTRING_TYPE, ERR_PARSING );
    readWKB_SInt( sBMLine->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBMLine->mNumWKBLineStrings, &sObjectCnt, &sWkbOffset,
                  sEquiEndian );

    sBLine = EWKB_FIRST_LINE( sBMLine );
    for ( i = 0; i < sObjectCnt; i++ )
    {
        ACI_TEST_RAISE( ( sPtr + sWkbOffset + WKB_GEOHEAD_SIZE ) > aFence,
                        ERR_PARSING );
        ACI_TEST( readWKB_Header( aHandle,
                                  (acp_uint8_t*)sBLine,
                                  &sEquiEndian,
                                  &sWkbType,
                                  &sWkbOffset ) != ACI_SUCCESS );
        ACI_TEST_RAISE( sWkbType != WKB_LINESTRING_TYPE, ERR_PARSING );

        sWkbOffset -= WKB_GEOHEAD_SIZE;

        switch( sWkbType )
        {
            case WKB_LINESTRING_TYPE :
                ACI_TEST( ulsGetLineStringSizeFromWKB( aHandle,
                                                       (acp_uint8_t**)&sBLine,
                                                       aFence,
                                                       &sTempSize ) != ACS_SUCCESS );
                break;
            default :
                ACI_RAISE( ACI_EXCEPTION_END_LABEL );
        } /* switch*/

        sTotalSize += sTempSize;
    } /* while*/

    sWkbSize   = ( ((acp_char_t*)sBLine) - ((acp_char_t*)sBMLine) );
    sTotalSize += STD_MLINE2D_EXT_SIZE;


    ACI_TEST( ( sPtr + sWkbSize ) > aFence );
    *aPtr  = sPtr + sWkbSize;
    *aSize = sTotalSize;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB) );
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

ACSRETURN
ulsGetGeoCollectionSizeFromEWKB( ulsHandle    * aHandle,
                                 acp_uint8_t ** aPtr,
                                 acp_uint8_t  * aFence,
                                 acp_uint32_t * aSize )
{
    acp_uint8_t*                    sPtr        = *aPtr;
    acp_uint32_t                    sObjectCnt;
    acp_uint32_t                    sTotalSize  = 0;
    acp_uint32_t                    sWkbOffset  = 0;
    acp_uint32_t                    sWkbType;
    acp_uint32_t                    sSize;
    acp_uint32_t                    i;
    EWKBGeometryCollection*         sBColl      = (EWKBGeometryCollection*)*aPtr;
    WKBGeometry*                    sBCurrObj;
    acp_bool_t                      sEquiEndian = ACP_FALSE;
    acp_sint32_t                    sSRID;

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( ( sPtr + WKB_GEOHEAD_SIZE + EWKB_SRID_SIZE + WKB_INT32_SIZE ) > aFence,
                    ERR_PARSING );
    ACI_TEST( readWKB_Header( aHandle,
                              (acp_uint8_t*)sBColl,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset ) != ACI_SUCCESS);
    ACI_TEST_RAISE( sWkbType != EWKB_COLLECTION_TYPE, ERR_PARSING );
    readWKB_SInt( sBColl->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBColl->mNumWKBGeometries, &sObjectCnt, &sWkbOffset, sEquiEndian );

    sBCurrObj = EWKB_FIRST_COLL( sBColl );

    for ( i = 0; i < sObjectCnt; i++ )
    {
        ACI_TEST( readWKB_Header( aHandle,
                                  (acp_uint8_t*)sBCurrObj,
                                  &sEquiEndian,
                                  &sWkbType,
                                  &sWkbOffset ) != ACI_SUCCESS );
        sWkbOffset -= WKB_GEOHEAD_SIZE;

        switch( sWkbType )
        {
            case WKB_POINT_TYPE :
                ACI_TEST( ulsGetPointSizeFromWKB( aHandle,
                                                  (acp_uint8_t**)&sBCurrObj,
                                                  aFence,
                                                  &sSize ) != ACS_SUCCESS );
                break;
            case WKB_LINESTRING_TYPE :
                ACI_TEST( ulsGetLineStringSizeFromWKB( aHandle,
                                                       (acp_uint8_t**)&sBCurrObj,
                                                       aFence,
                                                       &sSize ) != ACS_SUCCESS );
                break;
            case WKB_POLYGON_TYPE :
                ACI_TEST( ulsGetPolygonSizeFromWKB( aHandle,
                                                    (acp_uint8_t**)&sBCurrObj,
                                                    aFence,
                                                    &sSize ) != ACS_SUCCESS );
                break;
            case WKB_MULTIPOINT_TYPE :
                ACI_TEST( ulsGetMultiPointSizeFromWKB( aHandle,
                                                       (acp_uint8_t**)&sBCurrObj,
                                                       aFence,
                                                       &sSize ) != ACS_SUCCESS );
                break;
            case WKB_MULTILINESTRING_TYPE :
                ACI_TEST( ulsGetMultiLineStringSizeFromWKB( aHandle,
                                                            (acp_uint8_t**)&sBCurrObj,
                                                            aFence,
                                                            &sSize ) != ACS_SUCCESS );
                break;
            case WKB_MULTIPOLYGON_TYPE :
                ACI_TEST( ulsGetMultiPolygonSizeFromWKB( aHandle,
                                                         (acp_uint8_t**)&sBCurrObj,
                                                         aFence,
                                                         &sSize ) != ACS_SUCCESS );
                break;
            case EWKB_POINT_TYPE :
                ACI_TEST( ulsGetPointSizeFromEWKB( aHandle,
                                                   (acp_uint8_t**)&sBCurrObj,
                                                   aFence,
                                                   &sSize ) != ACS_SUCCESS );
                break;
            case EWKB_LINESTRING_TYPE :
                ACI_TEST( ulsGetLineStringSizeFromEWKB( aHandle,
                                                        (acp_uint8_t**)&sBCurrObj,
                                                        aFence,
                                                        &sSize ) != ACS_SUCCESS );
                break;
            case EWKB_POLYGON_TYPE :
                ACI_TEST( ulsGetPolygonSizeFromEWKB( aHandle,
                                                     (acp_uint8_t**)&sBCurrObj,
                                                     aFence,
                                                     &sSize ) != ACS_SUCCESS );
                break;
            case EWKB_MULTIPOINT_TYPE :
                ACI_TEST( ulsGetMultiPointSizeFromEWKB( aHandle,
                                                        (acp_uint8_t**)&sBCurrObj,
                                                        aFence,
                                                        &sSize ) != ACS_SUCCESS );
                break;
            case EWKB_MULTILINESTRING_TYPE :
                ACI_TEST( ulsGetMultiLineStringSizeFromEWKB( aHandle,
                                                             (acp_uint8_t**)&sBCurrObj,
                                                             aFence,
                                                             &sSize ) != ACS_SUCCESS );
                break;
            case EWKB_MULTIPOLYGON_TYPE :
                ACI_TEST( ulsGetMultiPolygonSizeFromEWKB( aHandle,
                                                          (acp_uint8_t**)&sBCurrObj,
                                                          aFence,
                                                          &sSize ) != ACS_SUCCESS );
                break;
            default :
                ACI_RAISE( ERR_INVALID_DATA_TYPE );
        } /* switch*/
        sTotalSize += sSize;
    } /* while*/

    sTotalSize += STD_COLL2D_EXT_SIZE;

    *aPtr  = sPtr;
    *aSize = sTotalSize;

    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_PARSING );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_WKB) );
    }

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }

    ACI_EXCEPTION( ERR_INVALID_DATA_TYPE );
    {
        ulsSetErrorCode( aHandle,
                         (ulERR_ABORT_ACS_INVALID_DATA_TYPE) );
    }
    
    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

