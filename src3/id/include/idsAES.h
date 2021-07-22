/***********************************************************************
 * Copyright 1999-2014, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idsAES.h 65634 2014-06-23 01:52:10Z sparrow $
 **********************************************************************/

#ifndef _O_IDSAES_H_
#define _O_IDSAES_H_

#include <idl.h>

#define MAXKC    (256/32)
#define MAXKB    (256/8)
#define MAXNR    (14)

#define IDS_AES_ENCRYPT  (1)
#define IDS_AES_DECRYPT  (0)

/*  The structure for key information */
typedef struct
{
    SInt    decrypt;
    SInt    Nr;                   /* key-length-dependent number of rounds */
    UInt    ek[4*(MAXNR + 1)];    /* encrypt key schedule */
    UInt    dk[4*(MAXNR + 1)];    /* decrypt key schedule */
} idsAESCTX;

class idsAES
{
    
public:
    static void  setKey( idsAESCTX *aCtx,
                         UChar     *aKey,
                         SInt       aBits,
                         SInt       aDoEncrypt );
    static void decrypt( idsAESCTX *aCtx,
                         UChar     *aSrc,
                         UChar     *aDst );
    static void encrypt( idsAESCTX *aCtx,
                         UChar     *aSrc,
                         UChar     *aDst );
    static void blockXOR( UChar *aTarget, UChar * aSource );
    
private:
    static SInt keySetupEnc( UInt        aRk[/*4*(Nr + 1)*/],
                             const UChar aCipherKey[],
                             SInt        aKeyBits );
    static SInt keySetupDec( UInt        aRk[/*4*(Nr + 1)*/],
                             const UChar aCipherKey[],
                             SInt        aKeyBits,
                             SInt        aHaveEncrypt );
    static void encryptInternal( const UInt  aRk[/*4*(Nr + 1)*/],
                                 SInt        aNr,
                                 const UChar aPt[16],
                                 UChar       aCt[16] );
    static void decryptInternal( const UInt  aRk[/*4*(Nr + 1)*/],
                                 SInt        aNr,
                                 const UChar aCt[16],
                                 UChar       aPt[16] );
};

#endif /* _O_IDSAES_H_ */
