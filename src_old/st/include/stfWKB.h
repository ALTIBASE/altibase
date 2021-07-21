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
 * $Id: stfWKB.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * WKB(Well Known Binary)로부터 Geometry 객체 생성하는 함수
 * 상세 구현은 stdParsing.cpp 에 있다.
 **********************************************************************/

#ifndef _O_STF_WKB_H_
#define _O_STF_WKB_H_        1

#include <idTypes.h>
#include <stuProperty.h>

class stfWKB
{
public:
    
    /* BUG-32531 Consider for GIS EMPTY */
    static IDE_RC cntNumFromWKB( UChar *aWKB,
                                 UInt  *aVal );
    
    static IDE_RC typeFromWKB( UChar*    aWKB,
                               UInt*    aType  );

    // BUG-27002 add aValidationOption for GeomFromWKB
    
    static IDE_RC geomFromWKB( iduMemory*   aQmxMem,
                               void*        aWKB,
                               void*        aBuf,
                               void*        aFence,
                               IDE_RC*      aResult,
                               UInt         aValidateOption );

    static IDE_RC pointFromWKB(
                    iduMemory* aQmxMem,           
                    void*      aWKB,
                    void*      aBuf,
                    void*      aFence,
                    IDE_RC*    aResult,
                    UInt       aValidateOption );

    static IDE_RC lineFromWKB(
                   iduMemory*  aQmxMem,
                    void*      aWKB,
                    void*      aBuf,
                    void*      aFence,
                    IDE_RC*    aResult,
                    UInt       aValidateOption ); 

    static IDE_RC polyFromWKB(
                    iduMemory*  aQmxMem,
                    void*       aWKB,
                    void*       aBuf,
                    void*       aFence,
                    IDE_RC*     aResult,
                    UInt        aValidateOption );

    /* BUG-44399 ST_RECTFROMTEXT(), ST_RECTFROMWKB()를 지원해야 합니다. */
    static IDE_RC rectFromWKB( iduMemory * aQmxMem,
                               void      * aWKB,
                               void      * aBuf,
                               void      * aFence,
                               IDE_RC    * aResult,
                               UInt        aValidateOption );

    static IDE_RC mpointFromWKB(
                    iduMemory*  aQmxMem,
                    void*       aWKB,
                    void*       aBuf,
                    void*       aFence,
                    IDE_RC*     aResult,
                    UInt        aValidateOption );
    
    static IDE_RC mlineFromWKB(
                    iduMemory*  aQmxMem,
                    void*       aWKB,
                    void*       aBuf,
                    void*       aFence,
                    IDE_RC*     aResult,
                    UInt        aValidateOption );

    static IDE_RC mpolyFromWKB(
                    iduMemory*  aQmxMem,
                    void*       aWKB,
                    void*       aBuf,
                    void*       aFence,
                    IDE_RC*     aResult,
                    UInt        aValidateOption );
    
    static IDE_RC geoCollFromWKB(
                    iduMemory*  aQmxMem,
                    void*       aWKB,
                    void*       aBuf,
                    void*       aFence,
                    IDE_RC*     aResult,
                    UInt        aValidateOption );
};

#endif /* _O_STF_WKB_H_ */
