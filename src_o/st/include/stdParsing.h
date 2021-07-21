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
 * $Id: stdParsing.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * 입력 버퍼로 부터 WKT(Well Known Text) 또는 WKB(Well Known Binary)를 읽어
 * Geometry 객체로 저장하는 모듈
 **********************************************************************/

#ifndef _O_STD_GEO_PARSING_H_
#define _O_STD_GEO_PARSING_H_        1

#include <idTypes.h>
#include <mtcDef.h>
#include <stdTypes.h>

/* BUG-44399 ST_RECTFROMTEXT(), ST_RECTFROMWKB()를 지원해야 합니다.
 *  ST_RECTFROMTEXT(), ST_RECTFROMWKB()에서만 사용하는 비공식 타입이므로, 외부에 공개하지 않는다.
 */
#define WKB_RECTANGLE_TYPE  (8)

typedef enum
{
    STT_SPACE_TOKEN = 0,
    STT_NUM_TOKEN,
    STT_COMMA_TOKEN,
    STT_CHAR_TOKEN,
    STT_LPAREN_TOKEN,
    STT_RPAREN_TOKEN,
    STT_LBRACK_TOKEN,
    STT_RBRACK_TOKEN,
    STT_SEMICOLON_TOKEN,
    STT_EQUAL_TOKEN
} STT_TOKEN;

class stdParsing
{
public:

    static inline idBool isSpace(UChar aVal)
    {
        switch(aVal)
        {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            return ID_TRUE;
        default:
            return ID_FALSE;
        }
    }
    static IDE_RC skipSpace(UChar** aPtr, UChar* aWKTFence);

/* BUG-32531 Consider for GIS EMPTY */
	static IDE_RC emptyCheckSpace(UChar** aPtr, UChar* aWKTFence);
    
    // BUG-27002 add aValidateOption
    static IDE_RC stdValue( iduMemory*   aQmxMem,
                            UChar*       aWKT,
                            UInt         aWKTLength,
                            void*        aBuf,
                            void*        aFence,
                            IDE_RC      *aResult,
                            UInt         aValidateOption,
                            idBool       aSRIDOption,
                            SInt         aSRID );

    // BUG-27002 add aValidateOption
    static IDE_RC stdBinValue( iduMemory*   aQmxMem,
                               UChar*       aWKB,
                               UInt         aWKBLength,
                               void*        aBuf,
                               void*        aFence,
                               IDE_RC*      aResult,
                               UInt         aValidateOption );

    static void setValidHeader( stdGeometryHeader * aGeom,
                                idBool              aIsValid,
                                UInt                aValidateOption);
    
    // PROJ-2422 srid 지원
    static IDE_RC skipSRID( UChar**    aPtr,
                            UChar*     aWKTFence );
    
    static IDE_RC getSRID( UChar**     aPtr,
                           UChar*      aWKTFence,
                           SInt*       aSRID );
    
    // BUG-27002
    // Add aValidateOption for GeomFromText validation level.
    static IDE_RC getPoint( UChar**                    aPtr,
                            UChar*                     aWKTFence,
                            stdGeometryHeader*         aGeom,
                            void*                      aFence,
                            IDE_RC*                    aResult,
                            UInt                       aValidateOption,
                            SInt                       aSRID );
    
    static IDE_RC getLineString( UChar**                    aPtr,
                                 UChar*                     aWKTFence,
                                 stdGeometryHeader*         aGeom,
                                 void*                      aFence,
                                 IDE_RC*                    aResult,
                                 UInt                       aValidateOption,
                                 SInt                       aSRID );
    
    static IDE_RC getPolygon( iduMemory*                 aQmxMem,
                              UChar**                    aPtr,
                              UChar*                     aWKTFence,
                              stdGeometryHeader*         aGeom,
                              void*                      aFence,
                              IDE_RC*                    aResult,
                              UInt                       aValidateOption,
                              SInt                       aSRID );

    /* BUG-44399 ST_RECTFROMTEXT(), ST_RECTFROMWKB()를 지원해야 합니다. */
    static IDE_RC getRectangle(
                    iduMemory                * aQmxMem,
                    UChar                   ** aPtr,
                    UChar                    * aWKTFence,
                    stdGeometryHeader        * aGeom,
                    void                     * aFence,
                    IDE_RC                   * aResult,
                    UInt                       aValidateOption );

    static IDE_RC getMultiPoint( UChar**                    aPtr,
                                 UChar*                     aWKTFence,
                                 stdGeometryHeader*         aGeom,
                                 void*                      aFence,
                                 IDE_RC*                    aResult,
                                 UInt                       aValidateOption,
                                 SInt                       aSRID );
    
    static IDE_RC getMultiLineString( UChar**                    aPtr,
                                      UChar*                     aWKTFence,
                                      stdGeometryHeader*         aGeom,
                                      void*                      aFence,
                                      IDE_RC*                    aResult,
                                      UInt                       aValidateOption,
                                      SInt                       aSRID );
    
    static IDE_RC getMultiPolygon( iduMemory*                 aQmxMem,
                                   UChar**                    aPtr,
                                   UChar*                     aWKTFence,
                                   stdGeometryHeader*         aGeom,
                                   void*                      aFence,
                                   IDE_RC*                    aResult,
                                   UInt                       aValidateOption,
                                   SInt                       aSRID );
    
    static IDE_RC getGeoCollection( iduMemory*                 aQmxMem,
                                    UChar**                    aPtr,
                                    UChar*                     aWKTFence,
                                    stdGeometryHeader*         aGeom,
                                    void*                      aFence,
                                    IDE_RC*                    aResult,
                                    UInt                       aValidateOption,
                                    SInt                       aSRID );

    /* BUG-32531 Consider for GIS EMPTY */
    static IDE_RC getEmpty( stdGeometryHeader* aGeom,
                            void*              aFence,
                            IDE_RC*            aResult);			
    
private:

    static inline idBool isSymbol(UChar aVal)
    {
        switch(aVal)
        {
        case ')':
        case '(':
        case ',':
        case ';':
            return ID_TRUE;
        default:
            return ID_FALSE;
        }
    }

    static STT_TOKEN testNextToken(UChar **aPtr, UChar *aWKTFence);
    static IDE_RC skipLParen(UChar** aPtr, UChar* aWKTFence);
    static IDE_RC skipRParen(UChar** aPtr, UChar* aWKTFence);
    static IDE_RC skipToLast(UChar** aPtr, UChar* aWKTFence);
    static UChar* findSubObjFence(UChar** aPtr, UChar* aWKTFence);
    static IDE_RC skipComma(UChar** aPtr, UChar *aWKTFence);
    static IDE_RC skipSemicolon( UChar** aPtr, UChar *aWKTFence );
    static IDE_RC skipEqual( UChar** aPtr, UChar *aWKTFence );
    static IDE_RC skipDate(UChar** aPtr, UChar *aWKTFence);
    static IDE_RC getDate(UChar** aPtr, UChar *aWKTFence, mtdDateType* aDate );
    static IDE_RC skipNumber(UChar** aPtr, UChar *aWKTFence);
    static IDE_RC getNumber(UChar** aPtr, UChar* aWKTFence, SDouble* aD);
    static IDE_RC skipLong( UChar** aPtr, UChar *aWKTFence );
    static IDE_RC getLong( UChar** aPtr, UChar* aWKTFence, SLong* aL );

    
    /* BUG-32531 Consider for GIS EMPTY */
    static IDE_RC checkValidEmpty(UChar** aPtr, UChar* aWKTFence);

public:
    static IDE_RC getWKBPoint(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult);
    
    static IDE_RC getWKBLineString(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult);
    
    static IDE_RC getWKBPolygon(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult);

    /* BUG-44399 ST_RECTFROMTEXT(), ST_RECTFROMWKB()를 지원해야 합니다. */
    static IDE_RC getWKBRectangle(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult );
    
    static IDE_RC getWKBMultiPoint(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult);
    
    static IDE_RC getWKBMultiLineString(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult);
    
    static IDE_RC getWKBMultiPolygon(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult);
        
    static IDE_RC getWKBGeoCollection(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult);

    static IDE_RC getEWKBPoint( UChar**                    aPtr,
                                UChar*                     aWKBFence,
                                stdGeometryHeader*         aGeom,
                                void*                      aFence,
                                IDE_RC*                    aResult );
    
    static IDE_RC getEWKBLineString( UChar**                    aPtr,
                                     UChar*                     aWKBFence,
                                     stdGeometryHeader*         aGeom,
                                     void*                      aFence,
                                     IDE_RC*                    aResult );
    
    static IDE_RC getEWKBPolygon( UChar**                    aPtr,
                                  UChar*                     aWKBFence,
                                  stdGeometryHeader*         aGeom,
                                  void*                      aFence,
                                  IDE_RC*                    aResult );
    
    static IDE_RC getEWKBMultiPoint( UChar**                    aPtr,
                                     UChar*                     aWKBFence,
                                     stdGeometryHeader*         aGeom,
                                     void*                      aFence,
                                     IDE_RC*                    aResult );
    
    static IDE_RC getEWKBMultiLineString( UChar**                    aPtr,
                                          UChar*                     aWKBFence,
                                          stdGeometryHeader*         aGeom,
                                          void*                      aFence,
                                          IDE_RC*                    aResult );
    
    static IDE_RC getEWKBMultiPolygon( UChar**                    aPtr,
                                       UChar*                     aWKBFence,
                                       stdGeometryHeader*         aGeom,
                                       void*                      aFence,
                                       IDE_RC*                    aResult );
        
    static IDE_RC getEWKBGeoCollection( UChar**                    aPtr,
                                        UChar*                     aWKBFence,
                                        stdGeometryHeader*         aGeom,
                                        void*                      aFence,
                                        IDE_RC*                    aResult );
    
    // BUG-24357 WKB Endian.
    static IDE_RC readWKB_Header( UChar     * aBuf,
                                  idBool    * aIsEquiEndian,
                                  UInt      * aType,
                                  UInt      * aOffset,
                                  UChar    ** aNext );

    // WKB 함수 ////////////////////////////////////////////////////////////////
    //  { Internal WKB read Function
private:
    static UChar *readWKB_Char( UChar  * aBuf,
                                UChar  * aVal,
                                UInt   * aOffset );

    static UChar *readWKB_UInt( UChar  * aBuf,
                                UInt   * aVal,
                                UInt   * aOffset,
                                idBool   aEquiEndian);
    
    static UChar *readWKB_SInt( UChar  * aBuf,
                                SInt   * aVal,
                                UInt   * aOffset,
                                idBool   aEquiEndian );
    
    static UChar *readWKB_SDouble( UChar    * aBuf,
                                   SDouble  * aVal,
                                   UInt     * aOffset,
                                   idBool   aEquiEndian);
   
    static UChar *readWKB_Point( UChar      * aBuf,
                                 stdPoint2D * aPoint,
                                 UInt       * aOffset,
                                 idBool   aEquiEndian);
    //  } Internal WKB read Function
};

#endif /* _O_STD_GEO_PARSING_H_ */


