/***********************************************************************
 * Copyright 1999-2007, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idsAltiWrap.cpp 90502 2021-04-08 01:52:26Z ahra.cho $ 
 **********************************************************************/

#include <idsAltiWrap.h>



IDE_RC idsAltiWrap::allocAltiWrapInfo( idsAltiWrapInfo ** aAltiWrapInfo )
{
    idsAltiWrapInfo * sAltiWrapInfo = NULL;

    IDE_DASSERT( aAltiWrapInfo != NULL );

    IDU_FIT_POINT( "idsAltiWrap::allocAltiWrapInfo::malloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP, 
                                 ID_SIZEOF(idsAltiWrapInfo),
                                 (void **)&sAltiWrapInfo,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    sAltiWrapInfo->mPlainText        = NULL;
    sAltiWrapInfo->mPlainTextLen     = 0;
    sAltiWrapInfo->mEncryptedText    = NULL;
    sAltiWrapInfo->mEncryptedTextLen = 0;
    sAltiWrapInfo->mCompText         = NULL;
    sAltiWrapInfo->mCompTextLen      = 0;
    sAltiWrapInfo->mSHA1Text         = NULL;
    sAltiWrapInfo->mSHA1TextLen      = 0;
    sAltiWrapInfo->mBase64Text       = NULL;
    sAltiWrapInfo->mBase64TextLen    = 0;

    *aAltiWrapInfo = sAltiWrapInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::freeResultMem( SChar * aResultMem )
{
    IDE_TEST( iduMemMgr::free( aResultMem )
              != IDE_SUCCESS );
    aResultMem = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Encryption 
 **********************************************************************/
IDE_RC idsAltiWrap::doCompression( SChar  * aPlainText,
                                   SInt     aPlainTextLen,
                                   UChar ** aCompText,
                                   UInt   * aCompTextLen )
{
/***********************************************************************
 *
 * Description :
 *     plain text를 압축하여 compressed text를 얻는다.
 *     
 *     - Input : idsAltiWrap->mPlainText,
 *               idsAltiWrap->mPlainTextLen
 *     - Output : idsAltiWrap->mCompText 
 *                idsAltiWrap->mCompTextLen
 *
 ***********************************************************************/

    void  * sCompWorkMem[ (IDU_COMPRESSION_WORK_SIZE + ID_SIZEOF(void *))
                           / ID_SIZEOF(void *) ];
    UChar * sCompBuf       = NULL;
    UInt    sCompBufSize   = IDU_COMPRESSION_MAX_OUTSIZE( aPlainTextLen );
    UInt    sCompResultLen = 0;
    SInt    sState         = 0;

    /* sCompWorkMem 초기화 */
    // BUG-42625 Memory used for PSM encryption is initialized incorrectly.
    idlOS::memset( sCompWorkMem,
                   0x00,
                   IDU_COMPRESSION_WORK_SIZE );

    /* compression에서 사용할 메모리 할당 */
    IDU_FIT_POINT( "idsAltiWrap::doCompression::calloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID_ALTIWRAP, 
                                 1,
                                 sCompBufSize,
                                 (void**)&sCompBuf )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( iduCompression::compress(
                  (UChar*)aPlainText,
                  (UInt)aPlainTextLen,
                  sCompBuf,
                  sCompBufSize,
                  &sCompResultLen,
                  sCompWorkMem )
              != IDE_SUCCESS );

    /* set compressing result */
    (*aCompText)    = sCompBuf;
    (*aCompTextLen) = sCompResultLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        (void)iduMemMgr::free( sCompBuf );
        sCompBuf = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::doSHA1( UChar  * aCompText,
                            UInt     aCompTextLen,
                            UChar ** aSHA1Text,
                            UInt   * aSHA1TextLen )
{
/***********************************************************************
 *
 * Description :
 *     compressed text에 SHA1기법을 적용해 SHA1 hash key를 얻는다.
 *
 *     - Input : idsAltiWrap->mCompText,
 *               idsAltiWrap->mCompTextLen
 *     - Output : idsAltiWrap->mSHA1Text 
 *                idsAltiWrap->mSHA1TextLen
 *
 ***********************************************************************/

    UChar   sSHA1[ IDS_SHA1_TEXT_LEN + 1 ];
    UChar * sSHA1Result    = NULL;
    UInt    sSHA1ResultLen = 0;

    idlOS::memset( sSHA1, 0x00, IDS_SHA1_TEXT_LEN + 1 );

    IDE_TEST( idsSHA1::digest( sSHA1,
                               aCompText,
                               aCompTextLen )
              != IDE_SUCCESS );

    sSHA1ResultLen = idlOS::strlen((SChar*)sSHA1);

    IDU_FIT_POINT( "idsAltiWrap::doSHA::malloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP,
                                 sSHA1ResultLen + 1,
                                 (void**)&sSHA1Result )
              != IDE_SUCCESS );

    idlOS::memcpy( sSHA1Result,
                   sSHA1,
                   sSHA1ResultLen );
    sSHA1Result[sSHA1ResultLen] = '\0';

    (*aSHA1Text)    = sSHA1Result;
    (*aSHA1TextLen) = sSHA1ResultLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::doBase64Encoding( UChar  * aSrcText,
                                      UInt     aSrcTextLen,
                                      UChar ** aDstText,
                                      SInt   * aDstLen )
{
/***********************************************************************
 *
 * Description :
 *     idsBase64::base64Encode 함수 실행하여,
 *     base64 encoding 실행한 결과를 얻은 후,
 *     new line을 추가한다.
 *
 ***********************************************************************/

    UInt sDstLen = 0;

    IDE_TEST( idsBase64::base64Encode( aSrcText,
                                       aSrcTextLen,
                                       aDstText,
                                       &sDstLen )
              != IDE_SUCCESS );

    IDE_DASSERT( sDstLen%4 == 0 );

    idlOS::memcpy( (*aDstText) + sDstLen, "\n", 1 );
    (*aDstLen) = (SInt)sDstLen + 1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::makeBase64Result( idsAltiWrapInfo * aAltiWrapInfo,
                                      SChar           * aPlainTextLen,
                                      SChar           * aCompTextLen,
                                      UChar           * aText )
{
/***********************************************************************
 *
 * Description :
 *     string을 변화시킨 plain text 길이와 compressed text 길이 
 *     그리고, compressed text + SHA1 text에 대해서,
 *     각각 base64 encoing을 실행하고,
 *     최종값을 aAltiWrapInfo->mBase64Text 저장한다.
 *
 ***********************************************************************/

    SInt    sLen4PlainTextLen = 0;
    SInt    sLen4CompTextLen  = 0;
    SInt    sLen4Text         = 0;
    UChar * sBase64Text       = NULL;
    SInt    sBase64TextLen    = 0;
    SInt    sCalculatedLen     = 0;
    UChar * sTmpBuf           = NULL;
    SInt    sTmpBufLen        = 0;
    SInt    sState            = 0;

    IDE_DASSERT( aAltiWrapInfo != NULL );

    sLen4PlainTextLen = idlOS::strlen(aPlainTextLen);
    sLen4CompTextLen  = idlOS::strlen(aCompTextLen);
    sLen4Text         = aAltiWrapInfo->mCompTextLen +
                        aAltiWrapInfo->mSHA1TextLen;
    /* sCalculatedLen은
       plain text 길이(aPlainTextLen)를 base64기법으로 인코딩했을 때의 길이
       compressed text 길이(aCompTextLen)를 base64기법으로 인코딩했을 때의 길이
       compreseed text와 SHA1 text를 합친 것(aText)을 base64로 인코딩했을 때의 길이와
       각각 인코딩 후 newline(\n)이 포함되는 길이 + null padding을 위한 길이를 의미한다.

       예를 들면,

       aPlainTextLen : "107" , aCompTextLen : "104"
       그리고 sLen4Text = 144라고 가정하자.
       
       sLen4PlainTextLen = 3, sLen4CompTextLen = 3이 된다.

       sLen4PalinTextLen =>  estimateBase64BufSize( sLen4PlainTextLen )
                         => ( ( 3 + 2 - ( ( 3 + 2 ) % 3 ) ) / 3 * 4 ) + 1 = 5
       sLen4CompTextLen =>  estimateBase64BufSize( sLen4CompTextLen )
                        => ( ( 3 + 2 - ( ( 3 + 2 ) % 3 ) ) / 3 * 4 ) + 1 = 5
       sLen4Text => estimateBase64BufSize( sLen4Text ) 
                 => ( ( 144 + 2 - ( ( 144 + 2 ) % 3 ) ) / 3 * 4 ) + 1 = 193

       sCalculatedLen = 5 + 5 + 193 + 4 = 207
       이 된다. */
    sCalculatedLen = IDS_CALC_BASE64_BUFSIZE( sLen4PlainTextLen ) +
                     IDS_CALC_BASE64_BUFSIZE( sLen4CompTextLen ) +
                     IDS_CALC_BASE64_BUFSIZE( sLen4Text ) + 4;

    IDU_FIT_POINT( "idsAltiWrap::makeBase64Result::malloc::sBase64Text",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP,
                                 sCalculatedLen + 1, 
                                 (void **)&sBase64Text )
              != IDE_SUCCESS );
    sState = 1;

    /* for plain text length */
    IDE_DASSERT( sLen4PlainTextLen > 0 );
    sTmpBuf = sBase64Text + sBase64TextLen;
    IDE_TEST( doBase64Encoding( (UChar*)aPlainTextLen,
                                sLen4PlainTextLen,
                                &sTmpBuf,
                                &sTmpBufLen )
              != IDE_SUCCESS );
    sBase64TextLen += sTmpBufLen;

    /* for compressed text length */
    IDE_DASSERT( sLen4CompTextLen > 0 );
    sTmpBuf    = sBase64Text + sBase64TextLen;
    sTmpBufLen = 0;
    IDE_TEST( doBase64Encoding( (UChar*)aCompTextLen,
                                sLen4CompTextLen,
                                &sTmpBuf,
                                &sTmpBufLen )
              != IDE_SUCCESS );
    sBase64TextLen += sTmpBufLen;

    /* for text */
    IDE_DASSERT( sLen4Text > 0 );
    sTmpBuf    = sBase64Text + sBase64TextLen;
    sTmpBufLen = 0;
    IDE_TEST( doBase64Encoding( aText,
                                sLen4Text,
                                &sTmpBuf,
                                &sTmpBufLen )
              != IDE_SUCCESS );
    sBase64TextLen += sTmpBufLen;

    sBase64Text[sBase64TextLen]   = '\0';

    aAltiWrapInfo->mBase64Text    = sBase64Text;
    aAltiWrapInfo->mBase64TextLen = sBase64TextLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            (void)iduMemMgr::free( sBase64Text );
            sBase64Text = NULL;
        case 0:
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::getBase64Result( idsAltiWrapInfo * aAltiWrapInfo )
{
/***********************************************************************
 *
 * Description :
 *     base64를 실행하기 위한 초입단계의 함수.
 *     int형인 aAltiWrapInfo->mPlainTextLen, aAltiWrapInfo->mCompText를
 *     string으로 변환시키고,
 *     aAltiWrapInfo->mCompText + aAltiWrapInfo->mSHA1Text 해준다.
 *     이들이 실제 base64 인코딩하는 함수는 makeBase64Result에서
 *     호출된다.
 *
 *     - Input :  (UChar *) idsAltiWrap->mPlainTextLen
 *                (UChar *) idsAltiWrap->mCompTextLen
 *                (UChar *) idsAltiWrap->mCompText + 
 *                (UChar *) idsAltiWrap->mSHA1Text
 *     - Output : idsAltiWrap->mBase64Text 
 *                idsAltiWrap->mBase64TextLen
 *
 ***********************************************************************/

    /* for plain text length */
    SChar   sText4PlainTextLen[IDS_ALTIWRAP_MAX_STRING_LEN];
    /* for compressed text length */
    SChar   sText4CompTextLen[IDS_ALTIWRAP_MAX_STRING_LEN];
    /* for text */
    UChar * sText = 0;
    UInt    sCalculatedLen;
    /* etc */
    SInt    sState = 0;

    IDE_DASSERT( aAltiWrapInfo != NULL );

    idlOS::memset( sText4PlainTextLen,
                   0x00,
                   IDS_ALTIWRAP_MAX_STRING_LEN );
    idlOS::memset( sText4CompTextLen,
                   0x00,
                   IDS_ALTIWRAP_MAX_STRING_LEN );
    sCalculatedLen = aAltiWrapInfo->mCompTextLen + aAltiWrapInfo->mSHA1TextLen;

    IDU_FIT_POINT( "idsAltiWrap::getBase64Result::malloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP,
                                 sCalculatedLen + 1, 
                                 (void **)&sText )
              != IDE_SUCCESS );
    sState = 1;

    /* encoding for plain text length */
    idlOS::snprintf( sText4PlainTextLen,
                     IDS_ALTIWRAP_MAX_STRING_LEN,
                     "%"ID_INT32_FMT,
                     aAltiWrapInfo->mPlainTextLen );

    /* encoding for compressed text length( decompression 시 필요 ) */
    idlOS::snprintf( sText4CompTextLen,
                     IDS_ALTIWRAP_MAX_STRING_LEN, 
                     "%"ID_INT32_FMT,
                     aAltiWrapInfo->mCompTextLen );

    /* encoding for compressed text + SHA1 result
       SHA1의 특성에 따라 항상 40byte가 된다. 
       따라서 decryption 시 쉽게 compressed text를 얻을 수 있다. */
    idlOS::memcpy( sText,
                   aAltiWrapInfo->mCompText,
                   aAltiWrapInfo->mCompTextLen );
    idlOS::memcpy( sText + aAltiWrapInfo->mCompTextLen,
                   aAltiWrapInfo->mSHA1Text,
                   aAltiWrapInfo->mSHA1TextLen );

    IDE_TEST( makeBase64Result( aAltiWrapInfo,
                                sText4PlainTextLen,
                                sText4CompTextLen,
                                sText )
              != IDE_SUCCESS );

    IDE_DASSERT( aAltiWrapInfo->mBase64Text != NULL );
    IDE_DASSERT( aAltiWrapInfo->mBase64TextLen != 0 );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sText ) != IDE_SUCCESS );
    sText = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        (void) iduMemMgr::free( sText );
        sText = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::setEncryptedText( idsAltiWrapInfo  * aAltiWrapInfo,
                                      SChar           ** aResult,
                                      SInt             * aResultLen )
{
/***********************************************************************
 *
 * Description :
 *     최종 encrypted text가 셋팅되는 함수.
 *
 ***********************************************************************/

    SChar * sResult;
    SInt    sResultLen;

    sResultLen = aAltiWrapInfo->mBase64TextLen;

    IDU_FIT_POINT( "idsAltiWrap::setEncryptedText::malloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP, 
                                 sResultLen + 1,
                                 (void **)&sResult,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    idlOS::memcpy( sResult,
                   aAltiWrapInfo->mBase64Text,
                   sResultLen );
    sResult[sResultLen]='\0';

    (*aResult)     = sResult;
    (*aResultLen ) = sResultLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::encryption(  SChar  * aSrcText,
                                 SInt     aSrcTextLen,
                                 SChar ** aDstText,
                                 SInt   * aDstTextLen )
{
    idsAltiWrapInfo * sAltiWrapInfo;
    SInt              sState = 0;

    IDE_DASSERT( aSrcText != NULL );
    IDE_DASSERT( aSrcTextLen != 0 );
    IDE_DASSERT( aDstText != NULL );
    IDE_DASSERT( aDstTextLen != NULL );

    IDE_TEST( allocAltiWrapInfo( &sAltiWrapInfo ) != IDE_SUCCESS );
    sState = 1;

    /* plain text 정보 셋팅 */
    sAltiWrapInfo->mPlainText    = aSrcText;
    sAltiWrapInfo->mPlainTextLen = aSrcTextLen;

    /* Compression 수행 */
    IDE_TEST( doCompression( sAltiWrapInfo->mPlainText,
                             sAltiWrapInfo->mPlainTextLen,
                             &(sAltiWrapInfo->mCompText),
                             &(sAltiWrapInfo->mCompTextLen) )
              != IDE_SUCCESS );
    sState = 2;

    /* SHA1 수행 */
    IDE_TEST( doSHA1( sAltiWrapInfo->mCompText,
                      sAltiWrapInfo->mCompTextLen,
                      &(sAltiWrapInfo->mSHA1Text),
                      &(sAltiWrapInfo->mSHA1TextLen) )
              != IDE_SUCCESS );
    sState = 3;

    /* Base64 */
    IDE_TEST( getBase64Result( sAltiWrapInfo ) != IDE_SUCCESS );
    sState = 4;

    IDE_TEST( setEncryptedText( sAltiWrapInfo,
                                aDstText,
                                aDstTextLen ) != IDE_SUCCESS );

    sState = 3;
    IDE_TEST( iduMemMgr::free( sAltiWrapInfo->mBase64Text ) != IDE_SUCCESS );
    sAltiWrapInfo->mBase64Text    = NULL;
    sAltiWrapInfo->mBase64TextLen = 0;

    sState = 2;
    IDE_TEST( iduMemMgr::free( sAltiWrapInfo->mSHA1Text ) != IDE_SUCCESS );
    sAltiWrapInfo->mSHA1Text    = NULL;
    sAltiWrapInfo->mSHA1TextLen = 0;

    sState = 1;
    IDE_TEST( iduMemMgr::free( sAltiWrapInfo->mCompText ) != IDE_SUCCESS );
    sAltiWrapInfo->mCompText    = NULL;
    sAltiWrapInfo->mCompTextLen = 0;

    sState = 0;
    IDE_TEST( iduMemMgr::free( sAltiWrapInfo ) != IDE_SUCCESS );
    sAltiWrapInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 4:
            (void) iduMemMgr::free( sAltiWrapInfo->mBase64Text );
            sAltiWrapInfo->mBase64Text    = NULL;
            sAltiWrapInfo->mBase64TextLen = 0;
        case 3:
            (void) iduMemMgr::free( sAltiWrapInfo->mSHA1Text );
            sAltiWrapInfo->mSHA1Text    = NULL;
            sAltiWrapInfo->mSHA1TextLen = 0;
        case 2:
            (void) iduMemMgr::free( sAltiWrapInfo->mCompText );
            sAltiWrapInfo->mCompText    = NULL;
            sAltiWrapInfo->mCompTextLen = 0;
        case 1:
            (void) iduMemMgr::free( sAltiWrapInfo );
            sAltiWrapInfo = NULL;
            break;
        case 0:
            break;
        default:
            break;
    }

    /* free하면서 exception으로 넘어왔을 때는,
       (*aDstText)에 대해서 free해줘야 한다.  */
    if ( (*aDstText) != NULL )
    {
        (void) iduMemMgr::free( (*aDstText) );
        (*aDstText) = NULL;
    }
    else
    {
        // Nohting to do.
    }

    return IDE_FAILURE;
}



/***********************************************************************
 * Decryption 
 **********************************************************************/
IDE_RC idsAltiWrap::doDecompression( UChar  * aCompText,
                                     UInt     aCompTextLen,
                                     SInt     aPlainTextLen,
                                     SChar ** aPlainText )
{
/***********************************************************************
 *
 * Description :
 *     aCompText를 decompression하여 plain text를 얻어낸다.
 *
 *     - Input :  idsAltiWrap->mCompText
 *                idsAltiWrap->mCompTextLen
 *                idsAltiWrap->mPlainTextLen
 *     - Output : idsAltiWrap->mPlainText 
 *
 ***********************************************************************/

    UChar * sDecompText    = NULL;
    UInt    sDecompTextLen = 0;
    UInt    sResultLen     = 0;
    SInt    sState         = 0;

    sDecompTextLen = aPlainTextLen;

    IDU_FIT_POINT( "idsAltiWrap::doDecompression::calloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID_ALTIWRAP,
                                 1,
                                 sDecompTextLen + 1,
                                 (void **)&sDecompText )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( iduCompression::decompress( aCompText,
                                          aCompTextLen,
                                          sDecompText,
                                          sDecompTextLen,
                                          &sResultLen )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sResultLen != (UInt)aPlainTextLen,
                    ERR_INVALID_ENCRYPTED_TEXT );

    sDecompText[sResultLen] = '\0';
    (*aPlainText) = (SChar*)sDecompText;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_TEXT )
    {
        IDE_SET( ideSetErrorCode(idERR_ABORT_INVALID_ENCRYPTED_TEXT) );
    }
    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        (void)iduMemMgr::free( sDecompText );
        sDecompText = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::checkEncryptedText( idsAltiWrapInfo * aAltiWrapInfo )
{
/***********************************************************************
 *
 * Description :
 *     decoding하여 얻어낸 compressed text에
 *     SHA1 기법을 적용하여 얻어낸 SHA1 hash와 
 *     decoding하여 얻어낸 SHA1 hash가 동일한지 확인하여,
 *     encrypted text가 오염되었는지 확인한다.
 *
 ***********************************************************************/

    UChar   sSHA1Result[ IDS_SHA1_TEXT_LEN + 1 ];
    UInt    sSHA1ResultLen = 0;

    IDE_DASSERT( aAltiWrapInfo != NULL );

    idlOS::memset( sSHA1Result, 0x00, IDS_SHA1_TEXT_LEN + 1 );

    IDE_TEST( idsSHA1::digest( sSHA1Result,
                               aAltiWrapInfo->mCompText,
                               aAltiWrapInfo->mCompTextLen )
              != IDE_SUCCESS );

    sSHA1ResultLen = idlOS::strlen((SChar*)sSHA1Result);

    IDE_TEST_RAISE( (aAltiWrapInfo->mSHA1TextLen != sSHA1ResultLen) || 
                    (idlOS::strncmp( (SChar*)aAltiWrapInfo->mSHA1Text,
                                     (SChar*)sSHA1Result,
                                     aAltiWrapInfo->mSHA1TextLen ) != 0),
                    ERR_INVALIDE_ENCRYPTED_TEXT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALIDE_ENCRYPTED_TEXT )
    {
        IDE_SET( ideSetErrorCode(idERR_ABORT_INVALID_ENCRYPTED_TEXT) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::doBase64DecodingInternal( UChar  * aSrcText,
                                              UInt     aSrcTextLen,
                                              UChar ** aDstText,
                                              UInt   * aDstTextLen )
{
/***********************************************************************
 *
 * Description :
 *     base64decoding 수행하는 함수.
 *
 ***********************************************************************/

    IDE_DASSERT( aSrcText != NULL );
    IDE_DASSERT( aSrcTextLen != 0 );
    IDE_DASSERT( aDstText != NULL );
    IDE_DASSERT( aDstTextLen != NULL );

    IDE_TEST( idsBase64::base64Decode( aSrcText,
                                       aSrcTextLen,
                                       aDstText,
                                       aDstTextLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::doBase64Decoding( idsAltiWrapInfo * aAltiWrapInfo )
{
/***********************************************************************
 *
 * Description :
 *     각각의 정보를 분류하여, base64 decoding을 수행.
 *
 *     - Input :  idsAltiWrap->mEncryptedText
 *                idsAltiWrap->mEncryptedTextLen
 *     - Output : idsAltiWrap->mPlainTextLen
 *                idsAltiWrap->mCompText
 *                idsAltiWrap->mCompTextLen
 *                idsAltiWrap->mSHA1Text
 *                idsAltiWrap->mSHA1TextLen 
 *
 ***********************************************************************/

    SInt    sLen            = 0;
    SInt    sEncTextLen     = 0;
    UChar * sResult         = NULL;
    UInt    sResultLen      = 0;
    SChar * sPrevPos        = NULL;
    SChar * sCurrPos        = NULL;
    SInt    sState          = 0;
    SChar * sFence          = NULL;

    IDE_DASSERT( aAltiWrapInfo != NULL );

    sLen   = aAltiWrapInfo->mEncryptedTextLen;
    sFence = aAltiWrapInfo->mEncryptedText + sLen; 

    IDU_FIT_POINT( "idsAltiWrap::doBase64Decoding::malloc::sResult", 
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP,
                                 sLen + 1,
                                 (void **)&sResult,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sState = 1;

    /* for plain text length */
    idlOS::memset( sResult, 0x00, sLen + 1 );

    sPrevPos = aAltiWrapInfo->mEncryptedText;
    sCurrPos = idlOS::strchr( sPrevPos, '\n' );
    IDE_TEST_RAISE( sCurrPos == NULL, ERR_INVALIDE_ENCRYPTED_TEXT );
    // BUG-48436
    IDE_TEST_RAISE( sCurrPos >= sFence, ERR_INVALIDE_ENCRYPTED_TEXT );

    sEncTextLen = idlOS::strlen(sPrevPos) - idlOS::strlen(sCurrPos);
    IDE_TEST_RAISE( sEncTextLen <= 0, ERR_INVALIDE_ENCRYPTED_TEXT );
    // BUG-48436
    IDE_TEST_RAISE( sEncTextLen > sLen, ERR_INVALIDE_ENCRYPTED_TEXT );

    IDE_TEST( doBase64DecodingInternal( (UChar*)sPrevPos,
                                        sEncTextLen,
                                        &sResult,
                                        &sResultLen )
              != IDE_SUCCESS );
    aAltiWrapInfo->mPlainTextLen = idlOS::atoi((SChar*)sResult);
    IDE_TEST_RAISE( aAltiWrapInfo->mPlainTextLen == 0 , ERR_INVALIDE_ENCRYPTED_TEXT );

    /* for compressed text length */
    idlOS::memset( sResult, 0x00, sLen + 1 );

    sPrevPos = sCurrPos + 1;
    sCurrPos = idlOS::strchr( sPrevPos, '\n' );
    IDE_TEST_RAISE( sCurrPos == NULL, ERR_INVALIDE_ENCRYPTED_TEXT );
    // BUG-48436
    IDE_TEST_RAISE( sCurrPos >= sFence, ERR_INVALIDE_ENCRYPTED_TEXT );

    sEncTextLen = idlOS::strlen(sPrevPos) - idlOS::strlen(sCurrPos);
    IDE_TEST_RAISE( sEncTextLen <= 0, ERR_INVALIDE_ENCRYPTED_TEXT );
    // BUG-48436
    IDE_TEST_RAISE( sEncTextLen > sLen, ERR_INVALIDE_ENCRYPTED_TEXT );

    IDE_TEST( doBase64DecodingInternal( (UChar*)sPrevPos,
                                        sEncTextLen,
                                        &sResult,
                                        &sResultLen )
              != IDE_SUCCESS );
    aAltiWrapInfo->mCompTextLen = idlOS::atoi((SChar*)sResult);
    IDE_TEST_RAISE( aAltiWrapInfo->mCompTextLen == 0 , ERR_INVALIDE_ENCRYPTED_TEXT );
    // BUG-48436
    IDE_TEST_RAISE( aAltiWrapInfo->mCompTextLen > sLen - IDS_SHA1_TEXT_LEN, ERR_INVALIDE_ENCRYPTED_TEXT );

    /* for text length */
    idlOS::memset( sResult, 0x00, sLen + 1 );

    sPrevPos = sCurrPos + 1;
    sCurrPos = idlOS::strchr( sPrevPos, '\n' );
    IDE_TEST_RAISE( sCurrPos == NULL, ERR_INVALIDE_ENCRYPTED_TEXT );
    // BUG-48436
    IDE_TEST_RAISE( sCurrPos >= sFence, ERR_INVALIDE_ENCRYPTED_TEXT );

    sEncTextLen = idlOS::strlen(sPrevPos) - idlOS::strlen(sCurrPos);
    IDE_TEST_RAISE( sEncTextLen <= 0, ERR_INVALIDE_ENCRYPTED_TEXT );

    IDE_TEST( doBase64DecodingInternal( (UChar*)sPrevPos,
                                        sEncTextLen,
                                        &sResult,
                                        &sResultLen )
              != IDE_SUCCESS );

    /* compressed text 셋팅 */
    IDU_FIT_POINT( "idsAltiWrap::doBase64Decoding::malloc::mCompText",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP,
                                 (aAltiWrapInfo->mCompTextLen) + 1,
                                 (void **)&aAltiWrapInfo->mCompText,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sState = 2;

    idlOS::memcpy( aAltiWrapInfo->mCompText,
                   sResult,
                   aAltiWrapInfo->mCompTextLen );
    aAltiWrapInfo->mCompText[aAltiWrapInfo->mCompTextLen] = '\0';
 
    /* SHA1 text 셋팅
       SHA1의 특성상 40byte의 고정길이를 가진다. */
    aAltiWrapInfo->mSHA1TextLen = IDS_SHA1_TEXT_LEN;

    IDU_FIT_POINT( "idsAltiWrap::doBase64Decoding::malloc::mSHAText",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP,
                                 aAltiWrapInfo->mSHA1TextLen + 1,
                                 (void **)&aAltiWrapInfo->mSHA1Text,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sState = 3;

    idlOS::memcpy( aAltiWrapInfo->mSHA1Text,
                   sResult + aAltiWrapInfo->mCompTextLen,
                   aAltiWrapInfo->mSHA1TextLen );
    aAltiWrapInfo->mSHA1Text[aAltiWrapInfo->mSHA1TextLen] = '\0';

    sState = 0;
    IDE_TEST( iduMemMgr::free( sResult ) != IDE_SUCCESS );
    sResult = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALIDE_ENCRYPTED_TEXT )
    {
        IDE_SET( ideSetErrorCode(idERR_ABORT_INVALID_ENCRYPTED_TEXT) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            (void)iduMemMgr::free( aAltiWrapInfo->mSHA1Text );
            aAltiWrapInfo->mSHA1Text = NULL;
        case 2:
            (void)iduMemMgr::free( aAltiWrapInfo->mCompText );
            aAltiWrapInfo->mCompText = NULL;
        case 1:
            (void)iduMemMgr::free( sResult );
            sResult = NULL;
        case 0:
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::setDecryptedText( idsAltiWrapInfo  * aAltiWrapInfo,
                                      SChar           ** aResult,
                                      SInt             * aResultLen )
{
/***********************************************************************
 *
 * Description :
 *     최종 decrypted text가 셋팅되는 함수.
 *
 ***********************************************************************/

    SChar * sResult;
    SInt    sResultLen;

    sResultLen = aAltiWrapInfo->mPlainTextLen;

    IDU_FIT_POINT( "idsAltiWrap::setDecryptedText::malloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_ALTIWRAP, 
                                 sResultLen + 1,
                                 (void **)&sResult,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    idlOS::memcpy( sResult,
                   aAltiWrapInfo->mPlainText,
                   sResultLen );
    sResult[sResultLen]='\0';

    (*aResult) = sResult;
    (*aResultLen ) = sResultLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idsAltiWrap::decryption( SChar  * aSrcText,
                                SInt     aSrcTextLen,
                                SChar ** aDstText,
                                SInt   * aDstTextLen )
{
    idsAltiWrapInfo * sAltiWrapInfo = NULL;
    SInt              sState        = 0;

    IDE_DASSERT( aSrcText != NULL );
    IDE_DASSERT( aSrcTextLen != 0 );
    IDE_DASSERT( aDstText != NULL );
    IDE_DASSERT( aDstTextLen != NULL );

    IDE_TEST( allocAltiWrapInfo( &sAltiWrapInfo ) != IDE_SUCCESS );
    sState = 1;

    /* encrypted text 정보 셋팅 */
    sAltiWrapInfo->mEncryptedText    = aSrcText;
    sAltiWrapInfo->mEncryptedTextLen = aSrcTextLen;

    /* base64 */
    IDE_TEST( doBase64Decoding( sAltiWrapInfo ) != IDE_SUCCESS );
    sState = 2;

    /* check compressed text using SHA1 Hash */
    IDE_TEST( checkEncryptedText( sAltiWrapInfo ) != IDE_SUCCESS );

    /* decompression */
    IDE_TEST( doDecompression( sAltiWrapInfo->mCompText,
                               sAltiWrapInfo->mCompTextLen,
                               sAltiWrapInfo->mPlainTextLen,
                               &(sAltiWrapInfo->mPlainText) )
              != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( setDecryptedText( sAltiWrapInfo,
                                aDstText,
                                aDstTextLen )
              != IDE_SUCCESS );

    sState = 2;
    IDE_TEST(iduMemMgr::free( sAltiWrapInfo->mPlainText ) != IDE_SUCCESS );
    sAltiWrapInfo->mPlainText = NULL;

    sState = 1;
    if ( sAltiWrapInfo->mSHA1Text != NULL )
    {
        IDE_TEST( iduMemMgr::free( sAltiWrapInfo->mSHA1Text ) != IDE_SUCCESS );
        sAltiWrapInfo->mSHA1Text = NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( sAltiWrapInfo->mCompText != NULL )
    {
        IDE_TEST( iduMemMgr::free( sAltiWrapInfo->mCompText ) != IDE_SUCCESS );
        sAltiWrapInfo->mCompText = NULL;
    }
    else
    {
        // Nothing to do.
    }

    sState = 0;
    IDE_TEST(iduMemMgr::free( sAltiWrapInfo ) != IDE_SUCCESS );
    sAltiWrapInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 3:
            (void)iduMemMgr::free( sAltiWrapInfo->mPlainText );
            sAltiWrapInfo->mPlainText = NULL;
        case 2:
            if ( sAltiWrapInfo->mSHA1Text != NULL )
            {
                (void)iduMemMgr::free( sAltiWrapInfo->mSHA1Text );
                sAltiWrapInfo->mSHA1Text = NULL;
            }
            else
            {
                // Nothing to do.
            }

            if ( sAltiWrapInfo->mCompText != NULL )
            {
                (void)iduMemMgr::free( sAltiWrapInfo->mCompText );
                sAltiWrapInfo->mCompText = NULL;
            }
            else
            {
                // Nothing to do.
            }
        case 1:
            (void)iduMemMgr::free( sAltiWrapInfo );
            sAltiWrapInfo = NULL;
            break;
        case 0:
            break;
        default:
            break;
    }

    /* free하면서 exception으로 넘어왔을 때는,
       (*aDstText)에 대해서 free해줘야 한다.  */
    if ( (*aDstText) != NULL )
    {
        (void) iduMemMgr::free( (*aDstText) );
        (*aDstText) = NULL;
    }
    else
    {
        // Nohting to do.
    }

    return IDE_FAILURE;
}
