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
 * $Id: stfWKT.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * WKT(Well Known Text)로부터 Geometry 객체 생성하는 함수
 * 상세 구현은 stdParsing.cpp 에 있다.
 **********************************************************************/

#ifndef _O_STF_WKT_H_
#define _O_STF_WKT_H_        1

#include <idTypes.h>
#include <stuProperty.h>

class stfWKT
{

    // BUG-27002 add aValidateOption
public:
    static IDE_RC geomFromText( iduMemory*   aQmxMem,
                                void*        aWKT,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption,
                                idBool       aSRIDOption,
                                SInt         aSRID );
    
    static IDE_RC pointFromText( iduMemory*   aQmxMem,
                                 void*        aWKT,
                                 void*        aBuf,
                                 void*        aFence,
                                 IDE_RC*      aResult,
                                 UInt         aValidateOption,
                                 idBool       aSRIDOption,
                                 SInt         aSRID );
    
    static IDE_RC lineFromText( iduMemory*   aQmxMem,
                                void*        aWKT,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption,
                                idBool       aSRIDOption,
                                SInt         aSRID );
    
    static IDE_RC polyFromText( iduMemory*   aQmxMem,
                                void*        aWKT,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption,
                                idBool       aSRIDOption,
                                SInt         aSRID );
    
    /* BUG-44399 ST_RECTFROMTEXT(), ST_RECTFROMWKB()를 지원해야 합니다. */
    static IDE_RC rectFromText( iduMemory  * aQmxMem,
                                void       * aWKT,
                                void       * aBuf,
                                void       * aFence,
                                IDE_RC     * aResult,
                                UInt         aValidateOption,
                                idBool       aSRIDOption,
                                SInt         aSRID );

    static IDE_RC mpointFromText( iduMemory*   aQmxMem,
                                  void*        aWKT,
                                  void*        aBuf,
                                  void*        aFence,
                                  IDE_RC*      aResult,
                                  UInt         aValidateOption,
                                  idBool       aSRIDOption,
                                  SInt         aSRID );
    
    static IDE_RC mlineFromText( iduMemory*   aQmxMem,
                                 void*        aWKT,
                                 void*        aBuf,
                                 void*        aFence,
                                 IDE_RC*      aResult,
                                 UInt         aValidateOption,
                                 idBool       aSRIDOption,
                                 SInt         aSRID );
    
    static IDE_RC mpolyFromText( iduMemory*   aQmxMem,
                                 void*        aWKT,
                                 void*        aBuf,
                                 void*        aFence,
                                 IDE_RC*      aResult,
                                 UInt         aValidateOption,
                                 idBool       aSRIDOption,
                                 SInt         aSRID );
    
    static IDE_RC geoCollFromText( iduMemory*   aQmxMem,
                                   void*        aWKT,
                                   void*        aBuf,
                                   void*        aFence,
                                   IDE_RC*      aResult,
                                   UInt         aValidateOption,
                                   idBool       aSRIDOption,
                                   SInt         aSRID );
};

#endif /* _O_STF_WKT_H_ */
