/***********************************************************************
 * Copyright 1999-2015, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idsAltiWrap.h 70364 2015-04-16 09:45:57Z song87 $ 
 **********************************************************************/

#ifndef _IDSALTIWRAP_H_
#define _IDSALTIWRAP_H_

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idsSHA1.h>
#include <idsBase64.h>



#define IDS_ALTIWRAP_MAX_STRING_LEN   (1024)
#define IDS_SHA1_TEXT_LEN             (40)

    /*
     * 3byte -> 4byte로 변환되므로 아래와 같이 base64 결과는 아래와 같이 계산 가능하다.
     * aSrcLen = 3
     *    ( 3 + 2 - ( ( 3 + 2 ) % 3 ) ) / 3 * 4 = 4
     * aSrcLen = 4
     *    ( 4 + 2 - ( ( 4 + 2 ) % 3 ) ) / 3 * 4 = 8
     * aSrcLen = 8
     *    ( 8 + 2 - ( ( 8 + 2 ) % 3 ) ) / 3 * 4 = 12
     */
#define IDS_CALC_BASE64_BUFSIZE( _aSrcLen_ ) ( ( ( _aSrcLen_ + 2 - ( ( _aSrcLen_ + 2 ) % 3 ) ) / 3 * 4 ) + 1 )

typedef struct idsAltiWrapInfo
{
    /* Encryption 시점 : input 
       Decryption 시점 : 최종 결과( decompression 후 결과 ) */
    SChar         * mPlainText;
    SInt            mPlainTextLen;
    /* Encryption 시점 : 최종 결과( base64 encoding 후 결과 )
       Decryption 시점 : input */
    SChar         * mEncryptedText;
    SInt            mEncryptedTextLen;
    /* compression */
    UChar         * mCompText;             
    UInt            mCompTextLen;
    /* SHA1 */
    UChar         * mSHA1Text;
    UInt            mSHA1TextLen;
    /* base64 */
    UChar         * mBase64Text;
    UInt            mBase64TextLen;
} idsAltiWrapInfo;

class idsAltiWrap
{
public:
    static IDE_RC encryption( SChar  * aSrcText,
                              SInt     aSrcTextLen,
                              SChar ** aDstText,
                              SInt   * aDstTextLen );

    static IDE_RC decryption( SChar  * aSrcText,
                              SInt     aSrcTextLen,
                              SChar ** aDstText,
                              SInt   * aDstTextLen );

    static IDE_RC freeResultMem( SChar * aResultMem );

private:
    /* Reresource를 위한 메모리 할당 */
    static IDE_RC allocAltiWrapInfo( idsAltiWrapInfo ** aAltiWrapInfo );

    /* Encryption */
    static IDE_RC doCompression( SChar  * aPlainText,
                                 SInt     aPlainTextLen,
                                 UChar ** aCompText,
                                 UInt   * aCompTextLen );

    static IDE_RC doSHA1( UChar  * aCompText,
                          UInt     aCompTextLen,
                          UChar ** aSHA1Text,
                          UInt   * aSHA1TextLen );

    static IDE_RC getBase64Result( idsAltiWrapInfo * aAltiWrapInfo );

    static SInt estimateBase64BufSize( SInt aSrcLen );

    static IDE_RC makeBase64Result( idsAltiWrapInfo * aAltiWrapInfo,
                                    SChar           * aPlainTextLen,
                                    SChar           * aCompTextLen,
                                    UChar           * aText );

    static IDE_RC doBase64Encoding( UChar  * aSrcText,
                                    UInt     aSrcTextLen,
                                    UChar ** aDstText,
                                    SInt   * aDstTextLen );

    static IDE_RC setEncryptedText( idsAltiWrapInfo  * aAltiWrapInfo,
                                    SChar           ** aResult,
                                    SInt             * aResultLen );

    /* Decryption */
    static IDE_RC doDecompression( UChar  * aCompText,
                                   UInt     aCompTextLen,
                                   SInt     aPlainTextLen,
                                   SChar ** aPlainText );

    static IDE_RC checkEncryptedText( idsAltiWrapInfo * aAltiWrapInfo );

    static IDE_RC doBase64Decoding( idsAltiWrapInfo * aAltiWrapInfo );

    static IDE_RC doBase64DecodingInternal( UChar  * aSrcText,
                                            UInt     aSrcTextLen,
                                            UChar ** aDstText,
                                            UInt   * aDstTextLen );

    static IDE_RC setDecryptedText( idsAltiWrapInfo  * aAltiWrapInfo,
                                    SChar           ** aResult,
                                    SInt             * aResultLen );
};

#endif /* _IDSALTIWRAP_H_ */
