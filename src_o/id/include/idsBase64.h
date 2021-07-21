/***********************************************************************
 * Copyright 1999-2015, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idsBase64.h 70364 2015-04-16 09:45:57Z song87 $
 **********************************************************************/

#ifndef _IDSBASE64_H_
#define _IDSBASE64_H_

#include <idl.h>
#include <ide.h>



class idsBase64
{
public:
    /* Encoding */
    static IDE_RC base64Encode( UChar  * aSrcText,
                                UInt     aSrcTextLen,
                                UChar ** aDstText,
                                UInt   * aDstTextLen );

    /* Decoding */
    static IDE_RC base64Decode( UChar  * aSrcText,
                                UInt     aSrcTextLen,
                                UChar ** aDstText,
                                UInt   * aDstTextLen );

private:
    /* Encoding */
    static void base64EncodeBlk( UChar * aIn,
                                 UChar * aOut,
                                 UInt    aLen );

    /* Decoding */
    static void base64DecodeBlk( UChar *  aIn,
                                 UChar *  aOut );

    static void searchTable( UChar          aValue,
                             const UChar ** aValuePos );
};

#endif /* _IDSBASE64_H_ */
