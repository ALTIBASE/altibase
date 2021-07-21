/***********************************************************************
 * Copyright 1999-2020, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: stfProj4.cpp 88007 2020-07-10 00:26:34Z cory.chae $
 **********************************************************************/

#include <stdUtils.h>
#include <ste.h>
#include <stfProj4.h>

#if defined(ST_ENABLE_PROJ4_LIBRARY)

IDE_RC stfProj4::doInit ( mtdCharType  * aFromProj4Text,
                          mtdCharType  * aToProj4Text,
                          projPJ       * aFromPJ,
                          projPJ       * aToPJ, 
                          projCtx      * aProjCtx )
{
    SChar       sFromProj4Text[2048];
    SChar       sToProj4Text[2048];
    projPJ      sFromPJ;
    projPJ      sToPJ;
    idBool      sIsFromInit             = ID_FALSE;
    idBool      sIsToInit               = ID_FALSE;

    projCtx     sProjCtx;
    idBool      sIsCtxInit              = ID_FALSE;

    sProjCtx = pj_ctx_alloc();
    
    IDE_TEST_RAISE( sProjCtx == NULL, ERR_INIT_FAILED );
    
    sIsCtxInit = ID_TRUE;

    IDE_TEST_RAISE( aFromProj4Text->length <= 0, ERR_INVALID_LENGTH );

    (void)idlOS::memcpy( &sFromProj4Text, 
                         &( aFromProj4Text->value ), 
                         aFromProj4Text->length );
    sFromProj4Text[aFromProj4Text->length] = '\0';
                         
    IDE_TEST_RAISE( aToProj4Text->length <= 0, ERR_INVALID_LENGTH );

    (void)idlOS::memcpy( &sToProj4Text, 
                         &( aToProj4Text->value ), 
                         aToProj4Text->length );
    sToProj4Text[aToProj4Text->length] = '\0';

    sFromPJ = pj_init_plus_ctx( sProjCtx, (const char *)sFromProj4Text );

    IDE_TEST_RAISE( sFromPJ == NULL, ERR_INIT_FAILED );
    
    sIsFromInit = ID_TRUE;

    sToPJ = pj_init_plus_ctx( sProjCtx, (const char *)sToProj4Text );

    IDE_TEST_RAISE( sToPJ == NULL, ERR_INIT_FAILED );
    
    sIsToInit = ID_TRUE;

    *aFromPJ = sFromPJ;
    *aToPJ = sToPJ;
    *aProjCtx = sProjCtx;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INIT_FAILED )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_PROJ4_INIT_FAILED, 
                                  pj_strerrno( pj_errno ), 
                                  sIsFromInit,
                                  sIsToInit ) );
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_LENGTH ) );
    }

    IDE_EXCEPTION_END;

    if ( sIsFromInit == ID_TRUE )
    {
        pj_free( sFromPJ );
    }
    else
    {
        /* Nothing to do */
    }
    if ( sIsToInit == ID_TRUE )
    {
        pj_free( sToPJ );
    }
    else
    {
        /* Nothing to do */
    }
    if ( sIsCtxInit == ID_TRUE )
    {
        pj_ctx_free( sProjCtx );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

void stfProj4::doFinalize ( projPJ    aFromPJ,
                            projPJ    aToPJ,
                            projCtx   aProjCtx )
{
    if ( aFromPJ != NULL )
    {
        pj_free( aFromPJ );
    }
    else
    {
        /* Nothing to do */
    }
    if ( aToPJ != NULL )
    {
        pj_free( aToPJ );
    }
    else
    {
        /* Nothing to do */
    }
    if ( aProjCtx != NULL )
    {
        pj_ctx_free( aProjCtx );
    }
    else
    {
        /* Nothing to do */
    }
}

IDE_RC stfProj4::doTransformInternal ( SDouble * aU,
                                       SDouble * aV,
                                       projPJ    aFromPJ,
                                       projPJ    aToPJ,
                                       projCtx   aProjCtx )
{
    UInt    sRet;

    if ( pj_is_latlong( aFromPJ ) == ID_TRUE )
    {
        *aU *= DEG_TO_RAD;
        *aV *= DEG_TO_RAD;
    }
    else
    {
        /* Nothing to do */
    }

    sRet = pj_transform( aFromPJ, aToPJ, 1, 0, aU, aV, NULL );

    IDE_TEST_RAISE( sRet != 0, ERR_TRANSFORM_FAILED );

    if ( pj_is_latlong( aToPJ ) == ID_TRUE )
    {
        *aU *= RAD_TO_DEG;
        *aV *= RAD_TO_DEG;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANSFORM_FAILED )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_PROJ4_TRANSFORM_FAILED, 
                                  pj_strerrno( pj_ctx_get_errno( aProjCtx ) ) ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfProj4::doTransform( mtcStack     * aStack,
                              mtdCharType  * aFromProj4Text,
                              mtdCharType  * aToProj4Text )
{
    stdGeometryHeader*    sFromGeometry         = NULL;
    stdGeometryHeader*    sToGeometry           = NULL;
    stdGeometryHeader*    sNextGeometry         = NULL;
    UInt                  sGeometryIndex        = 0;
    UInt                  sGeometryCount        = 0;

    stdPoint2DType*       sPoint2DType          = NULL;
    stdLineString2DType*  sLineString2DType     = NULL;
    UInt                  sPointCount           = 0;
    UInt                  sPointIndex           = 0;
    stdPoint2D*           sPoint2D              = NULL;
    stdPolygon2DType*     sPolygon2DType        = NULL;
    UInt                  sRingCount            = 0;
    UInt                  sRingIndex            = 0;
    stdLinearRing2D*      sRing2D               = NULL;

    projPJ                sFromPJ;
    projPJ                sToPJ;
    projCtx               sProjCtx;
    
    idBool                sIsProj4Init          = ID_FALSE;

    sFromGeometry = (stdGeometryHeader *)aStack[1].value;
    sToGeometry = (stdGeometryHeader *)aStack[0].value;

    IDE_TEST_RAISE( sFromGeometry == NULL, ERR_INVALID_GEOMETRY );
    IDE_TEST_RAISE( sToGeometry == NULL, ERR_INVALID_GEOMETRY );

    stdUtils::copyGeometry( sToGeometry, sFromGeometry );

    sGeometryCount = stdUtils::getGeometryNum( sToGeometry );
    sNextGeometry = stdUtils::getFirstGeometry( sToGeometry );
    
    IDE_TEST_RAISE( sNextGeometry == NULL, ERR_INVALID_GEOMETRY );

    IDE_TEST( doInit( aFromProj4Text, 
                      aToProj4Text, 
                      &sFromPJ, 
                      &sToPJ, 
                      &sProjCtx )
              != IDE_SUCCESS );

    sIsProj4Init = ID_TRUE;

    (void)stdUtils::initMBR( &sToGeometry->mMbr );

    for ( sGeometryIndex = 0 ; sGeometryIndex < sGeometryCount ; sGeometryIndex++ )
    {
        if ( sNextGeometry != sToGeometry )
        {
            (void)stdUtils::initMBR( &sNextGeometry->mMbr );
        }
        else
        {
            /* Do nothing */
        }

        switch ( sNextGeometry->mType )
        {
            case STD_POINT_2D_TYPE :
            case STD_POINT_2D_EXT_TYPE :
                sPoint2DType = (stdPoint2DType *)sNextGeometry;

                IDE_TEST( doTransformInternal( &sPoint2DType->mPoint.mX, 
                                               &sPoint2DType->mPoint.mY, 
                                               sFromPJ, 
                                               sToPJ,
                                               sProjCtx )
                          != IDE_SUCCESS );

                if ( sNextGeometry != sToGeometry )
                {
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( sNextGeometry, &sPoint2DType->mPoint )
                              != IDE_SUCCESS );
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( sToGeometry, &sPoint2DType->mPoint )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( sToGeometry, &sPoint2DType->mPoint )
                              != IDE_SUCCESS );
                }
                break;
            case STD_LINESTRING_2D_TYPE :
            case STD_LINESTRING_2D_EXT_TYPE :
                sLineString2DType = (stdLineString2DType *)sNextGeometry;
            
                sPointCount = STD_N_POINTS( sLineString2DType );
                sPoint2D = STD_FIRST_PT2D( sLineString2DType );
                
                for ( sPointIndex = 0 ; sPointIndex < sPointCount ; sPointIndex++ )
                {
                    IDE_TEST( doTransformInternal( &sPoint2D->mX, 
                                                   &sPoint2D->mY, 
                                                   sFromPJ, 
                                                   sToPJ,
                                                   sProjCtx )
                              != IDE_SUCCESS );

                    if ( sNextGeometry != sToGeometry )
                    {
                        IDE_TEST( stdUtils::mergeMBRFromPoint2D( sNextGeometry, sPoint2D )
                                  != IDE_SUCCESS );
                        IDE_TEST( stdUtils::mergeMBRFromPoint2D( sToGeometry, sPoint2D )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        IDE_TEST( stdUtils::mergeMBRFromPoint2D( sToGeometry, sPoint2D )
                                  != IDE_SUCCESS );
                    }
                    
                    sPoint2D = STD_NEXT_PT2D( sPoint2D );
                }
                break;
            case STD_POLYGON_2D_TYPE :
            case STD_POLYGON_2D_EXT_TYPE :
                sPolygon2DType = (stdPolygon2DType *)sNextGeometry;
                
                sRingCount = STD_N_RINGS( sPolygon2DType );
                sRing2D = STD_FIRST_RN2D( sPolygon2DType );
                
                for ( sRingIndex = 0 ; sRingIndex < sRingCount ; sRingIndex++ )
                {
                    sPointCount = STD_N_POINTS( sRing2D );
                    sPoint2D = STD_FIRST_PT2D ( sRing2D );

                    for ( sPointIndex = 0 ; sPointIndex < sPointCount ; sPointIndex++ )
                    {
                        IDE_TEST( doTransformInternal( &sPoint2D->mX, 
                                                       &sPoint2D->mY, 
                                                       sFromPJ, 
                                                       sToPJ,
                                                       sProjCtx )
                                  != IDE_SUCCESS );

                        if ( sNextGeometry != sToGeometry )
                        {
                            IDE_TEST( stdUtils::mergeMBRFromPoint2D( sNextGeometry, sPoint2D )
                                      != IDE_SUCCESS );
                            IDE_TEST( stdUtils::mergeMBRFromPoint2D( sToGeometry, sPoint2D )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( stdUtils::mergeMBRFromPoint2D( sToGeometry, sPoint2D )
                                      != IDE_SUCCESS );
                        }

                        sPoint2D = STD_NEXT_PT2D( sPoint2D );
                    }
                    
                    sRing2D = (stdLinearRing2D *)sPoint2D;
                }
                break;
            default :
                IDE_RAISE( ERR_INVALID_GEOMETRY_TYPE );
        }

        sNextGeometry = stdUtils::getNextGeometry( sNextGeometry );
    }
    
    sIsProj4Init = ID_FALSE;
    doFinalize( sFromPJ, 
                sToPJ, 
                sProjCtx );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_GEOMETRY )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY ) );
    }

    IDE_EXCEPTION( ERR_INVALID_GEOMETRY_TYPE )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_NOT_APPLICABLE ) );
    }

    IDE_EXCEPTION_END;
    
    if ( sIsProj4Init == ID_TRUE )
    {
        doFinalize( sFromPJ, 
                    sToPJ, 
                    sProjCtx );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

#endif /* ST_ENABLE_PROJ4_LIBRARY */

