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
 
#ifndef _O_IDSSHA1_H_
#define _O_IDSSHA1_H_

#include <idl.h>

class idsSHA1
{
private:
    static void processMessageBlock( UInt *aDigest, UChar *aBlock );

    static void padMessage( UInt  *aDigest,
                            UChar *aBlock,
                            UInt   aIndex,
                            UInt   aMessageLenBitLow,
                            UInt   aMessageLenBitHigh );

    static void makeHexCode( UChar *aByte, UInt *aDigest );
    
    static void convertHexToString( UChar *aString, UChar *aByte );
    
public:
    /* requires aByte[20]  */
    static IDE_RC digestToHexCode( UChar *aByte, const UChar *aMessage, UInt aMessageLen );
    
    /* requires aResult[40] */
    static IDE_RC digest( UChar *aResult, const UChar *aMessage, UInt aMessageLen );
};

#endif /* _O_IDSSHA1_H_ */
