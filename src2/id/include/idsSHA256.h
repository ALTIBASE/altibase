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
 
#ifndef _O_IDSSHA256_H_
#define _O_IDSSHA256_H_

#include <idl.h>

/* BUG-39303 */
/* SHA-256 Block size */
#define SHA256_BLK_SIZE      (64)
#define SHA256_MSG_BYTE_SIZE (32)

typedef struct idsSHA256Context {
    UInt  mTotalSize; /* for processReamins */
    UInt  mDigest[8];
    UInt  mBufferSize;
    UChar mBuffer[SHA256_BLK_SIZE];
    UInt  mMessageSize;
    UChar mMessage[1];
} idsSHA256Context;

class idsSHA256
{
private:
    static void processBlock( UInt * aDigest, UChar * aBlock );

    static void processRemains( UInt  * aDigest,
                                UChar * aMessage,
                                UInt    aMessageLen,
                                UInt    aRemainsLen );

    static void makeHexCode( UChar * aByte, UInt * aDigest );

    static void convertHexToString( UChar * aString, UChar * aByte );

public:
    /* requires aByte[32]  */
    static void digestToHexCode( UChar * aByte, UChar * aMessage, UInt aMessageLen );

    /* requires aResult[64] */
    static void digest( UChar * aResult, UChar * aMessage, UInt aMessageLen );

    /* BUG-39303 */
    static void digestToByte( UChar * aResult, UChar * aMessage, UInt aMessageLen );

    static void initializeForGroup( idsSHA256Context * aContext );
    static void digestForGroup( idsSHA256Context * aContext );
    static void finalizeForGroup( UChar * aResult, idsSHA256Context * aContext );
};

#endif /* _O_IDSSHA256_H_ */
