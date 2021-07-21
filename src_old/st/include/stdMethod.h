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
 * $Id: stdMethod.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체를 WKT(Well Known Text) 또는 WKB(Well Known Binary)로
 * 출력하는 모듈
 **********************************************************************/

#ifndef _O_STD_GEO_METHOD_H_
#define _O_STD_GEO_METHOD_H_        1

#include <idTypes.h>
#include <mtcDef.h>

class stdMethod
{
public:
    static IDE_RC writePointWKT2D(
                    stdPoint2DType*            aPoint,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt  *                    aOffset);
    static IDE_RC writeLineStringWKT2D(
                    stdLineString2DType*       aLine,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);
    static IDE_RC writePolygonWKT2D(
                    stdPolygon2DType*          aPolygon,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);
    static IDE_RC writeMultiPointWKT2D(
                    stdMultiPoint2DType*       aMPoint,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);
    static IDE_RC writeMultiLineStringWKT2D(
                    stdMultiLineString2DType*  aMLine,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);
    static IDE_RC writeMultiPolygonWKT2D(
                    stdMultiPolygon2DType*     aMPolygon,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);
    static IDE_RC writeGeoCollectionWKT2D(
                    stdGeoCollection2DType*    aCollection,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);

    // PROJ-2422 srid 지원
    static IDE_RC writePointEWKT2D( stdPoint2DExtType*         aPoint,
                                    UChar*                     aBuf,
                                    UInt                       aMaxSize,
                                    UInt  *                    aOffset );
    
    static IDE_RC writeLineStringEWKT2D( stdLineString2DExtType*    aLine,
                                         UChar*                     aBuf,
                                         UInt                       aMaxSize,
                                         UInt*                      aOffset );
    
    static IDE_RC writePolygonEWKT2D( stdPolygon2DExtType*       aPolygon,
                                      UChar*                     aBuf,
                                      UInt                       aMaxSize,
                                      UInt*                      aOffset );
    
    static IDE_RC writeMultiPointEWKT2D( stdMultiPoint2DExtType*    aMPoint,
                                         UChar*                     aBuf,
                                         UInt                       aMaxSize,
                                         UInt*                      aOffset );
    
    static IDE_RC writeMultiLineStringEWKT2D( stdMultiLineString2DExtType* aMLine,
                                              UChar*                       aBuf,
                                              UInt                         aMaxSize,
                                              UInt*                        aOffset );
    
    static IDE_RC writeMultiPolygonEWKT2D( stdMultiPolygon2DExtType*  aMPolygon,
                                           UChar*                     aBuf,
                                           UInt                       aMaxSize,
                                           UInt*                      aOffset );
    
    static IDE_RC writeGeoCollectionEWKT2D( stdGeoCollection2DExtType* aCollection,
                                            UChar*                     aBuf,
                                            UInt                       aMaxSize,
                                            UInt*                      aOffset );

    static IDE_RC writeSRID( SInt       aSRID,
                             UChar*     aBuf,
                             UInt       aMaxSize,
                             UInt*      aOffset );

    /* BUG-32531 Consider for GIS EMPTY */
    static IDE_RC writeEmptyWKB2D( stdMultiPoint2DType* aMPoint,
                                   UChar*               aBuf,
                                   UInt                 aMaxSize,
                                   UInt*                aOffset);    

    static IDE_RC writePointWKB2D(
                    stdPoint2DType*            aPoint,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);    // Fix BUG-15834
    static IDE_RC writeLineStringWKB2D(
                    stdLineString2DType*       aLine,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);    // Fix BUG-15834
    static IDE_RC writePolygonWKB2D(
                    stdPolygon2DType*          aPolygon,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);    // Fix BUG-15834
    static IDE_RC writeMultiPointWKB2D(
                    stdMultiPoint2DType*       aMPoint,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);    // Fix BUG-15834
    static IDE_RC writeMultiLineStringWKB2D(
                    stdMultiLineString2DType*  aMLine,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);    // Fix BUG-15834
    static IDE_RC writeMultiPolygonWKB2D(
                    stdMultiPolygon2DType*     aMPolygon,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);    // Fix BUG-15834
    static IDE_RC writeGeoCollectionWKB2D(
                    stdGeoCollection2DType*    aCollection,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset);    // Fix BUG-15834
    
    // PROJ-2422 srid 지원
    static IDE_RC writePointEWKB2D( stdPoint2DExtType*         aPoint,
                                    UChar*                     aBuf,
                                    UInt                       aMaxSize,
                                    UInt*                      aOffset );    // Fix BUG-15834

    static IDE_RC writeLineStringEWKB2D( stdLineString2DExtType*    aLine,
                                         UChar*                     aBuf,
                                         UInt                       aMaxSize,
                                         UInt*                      aOffset );    // Fix BUG-15834

    static IDE_RC writePolygonEWKB2D( stdPolygon2DExtType*       aPolygon,
                                      UChar*                     aBuf,
                                      UInt                       aMaxSize,
                                      UInt*                      aOffset );    // Fix BUG-15834

    static IDE_RC writeMultiPointEWKB2D( stdMultiPoint2DExtType*    aMPoint,
                                         UChar*                     aBuf,
                                         UInt                       aMaxSize,
                                         UInt*                      aOffset );    // Fix BUG-15834

    static IDE_RC writeMultiLineStringEWKB2D( stdMultiLineString2DExtType* aMLine,
                                              UChar*                       aBuf,
                                              UInt                         aMaxSize,
                                              UInt*                        aOffset );    // Fix BUG-15834

    static IDE_RC writeMultiPolygonEWKB2D( stdMultiPolygon2DExtType*  aMPolygon,
                                           UChar*                     aBuf,
                                           UInt                       aMaxSize,
                                           UInt*                      aOffset );    // Fix BUG-15834

    static IDE_RC writeGeoCollectionEWKB2D( stdGeoCollection2DExtType* aCollection,
                                            UChar*                     aBuf,
                                            UInt                       aMaxSize,
                                            UInt*                      aOffset );    // Fix BUG-15834
    
private:
    // Internal For WKB Write
    static UChar *writeWKB_Char( UChar *aBuf, UChar aVal, UInt *aOffset );
    static UChar *writeWKB_UInt( UChar *aBuf, UInt aVal, UInt *aOffset );
    static UChar *writeWKB_SInt( UChar *aBuf, SInt aVal, UInt *aOffset );
    static UChar *writeWKB_SDouble( UChar *aBuf, SDouble aVal, UInt *aOffset );
    static UChar *writeWKB_Header(  UChar * aBuf,
                                    UInt    aType,
                                    UInt  * aOffset );
    static UChar *writeWKB_Point2D( UChar            * aBuf,
                                    const stdPoint2D *  aPoint,
                                    UInt             * aOffset );

    static UChar *writeWKB_Point2Ds( UChar            * aBuf,
                                     UInt               aNumPoints,
                                     const stdPoint2D * aPoints,
                                     UInt             * aOffset );

    // TASK-1915 remove ASTEXT platform dependency
    static void fill2DCoordString( SChar      * aBuffer,
                                   SInt         aBufSize,
                                   stdPoint2D * aPoint );
};

#endif /* _O_STD_GEO_METHOD_H_ */

