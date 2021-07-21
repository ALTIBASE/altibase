/***********************************************************************
 * Copyright 1999-2006, ALTIBase Corporation or its subsididiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 *
 * Description:
 * EWKT(Extended Well Known Text)로부터 Geometry 객체 생성하는 함수
 * 상세 구현은 stdParsing.cpp 에 있다.
 **********************************************************************/

#ifndef _O_STF_EWKT_H_
#define _O_STF_EWKT_H_        1

#include <idTypes.h>
#include <stuProperty.h>

class stfEWKT
{

    // BUG-27002 add aValidateOption
public:
    static IDE_RC geomFromEWKT( iduMemory*   aQmxMem,
                                void*        aWKT,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption );
    
    static IDE_RC pointFromEWKT( iduMemory*   aQmxMem,
                                 void*        aWKT,
                                 void*        aBuf,
                                 void*        aFence,
                                 IDE_RC*      aResult,
                                 UInt         aValidateOption );
    
    static IDE_RC lineFromEWKT( iduMemory*   aQmxMem,
                                void*        aWKT,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption );
    
    static IDE_RC polyFromEWKT( iduMemory*   aQmxMem,
                                void*        aWKT,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption, 
                                SInt         aSRID /* BUG-47966 */ );
    
    static IDE_RC mpointFromEWKT( iduMemory*   aQmxMem,
                                  void*        aWKT,
                                  void*        aBuf,
                                  void*        aFence,
                                  IDE_RC*      aResult,
                                  UInt         aValidateOption );
    
    static IDE_RC mlineFromEWKT( iduMemory*   aQmxMem,
                                 void*        aWKT,
                                 void*        aBuf,
                                 void*        aFence,
                                 IDE_RC*      aResult,
                                 UInt         aValidateOption );
    
    static IDE_RC mpolyFromEWKT( iduMemory*   aQmxMem,
                                 void*        aWKT,
                                 void*        aBuf,
                                 void*        aFence,
                                 IDE_RC*      aResult,
                                 UInt         aValidateOption );
    
    static IDE_RC geoCollFromEWKT( iduMemory*   aQmxMem,
                                   void*        aWKT,
                                   void*        aBuf,
                                   void*        aFence,
                                   IDE_RC*      aResult,
                                   UInt         aValidateOption );
};

#endif /* _O_STF_EWKT_H_ */
