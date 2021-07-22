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
 
#ifndef _O_IDSSHA512_H_
#define _O_IDSSHA512_H_

#include <idl.h>

class idsSHA512
{
private:
    static void processBlock( ULong * aDigest, UChar * aBlock );

    static void processRemains( ULong * aDigest,
                                UChar * aMessage,
                                ULong   aMessageLen,
                                ULong   aRemainsLen );

    static void makeHexCode( UChar * aByte, ULong * aDigest );
    
    static void convertHexToString( UChar * aString, UChar * aByte );
    
public:
    /* requires aByte[64]  */
    static void digestToHexCode( UChar * aByte, UChar * aMessage, ULong aMessageLen );
    
    /* requires aResult[128] */
    static void digest( UChar * aResult, UChar * aMessage, ULong aMessageLen );
};

#endif /* _O_IDSSHA512_H_ */
