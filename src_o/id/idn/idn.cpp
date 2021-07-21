/***********************************************************************
 * Copyright 1999-2008, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idn.cpp 66405 2014-08-13 07:15:26Z djin $
 ***********************************************************************/

#include <idl.h>
#include <idn.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideLog.h>
#include <ida.h>

extern SInt idnSkip( const UChar* s1,
                     UShort s1Length,
                     idnIndex* index,
                     UShort skip );

extern SInt idnCharAt( idnChar* c,
                       const UChar* s1,
                       UShort s1Length,
                       idnIndex* index );

extern SInt idnAddChar( UChar* s1,
                        UShort s1Max,
                        UShort* s1Length,
                        idnChar c );

extern SInt idnToUpper( idnChar* c );

extern SInt idnToLower( idnChar* c );

extern SInt idnToAscii( idnChar* c );

extern SInt idnToLocal( idnChar* c );

extern SInt idnToCompareOrder( idnChar* c );

extern SInt idnToCaselessCompareOrder( idnChar* c );

extern SInt idnNextChar( idnChar* c, SInt* carry );

extern SInt idnCompareUsingMemCmp( SInt* order,
                                   const UChar* s1,
                                   UShort s1Length,
                                   const UChar* s2,
                                   UShort s2Length );

extern SInt idnLengthWithoutSpaceUsingByteScan( UShort* length,
                                                const UChar* s1,
                                                UShort s1Length );


idnChar idnAsciiChars[128];

UChar idnSpaceTemplete[128];
UChar idnWeekDaysTemplete[7][128];
UChar idnTruncateTemplete[7][128];

UShort idnSpaceTempleteLength;
UShort idnWeekDaysTempleteLength[7];
UShort idnTruncateTempleteLength[7];


static UChar idn_US7ASCII_UpperTable[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
	0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
	0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
	0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
	0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static UChar idn_US7ASCII_LowerTable[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
	0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
	0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
	0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
	0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static UChar* idn_US7ASCII_CaselessCompareTable = idn_US7ASCII_UpperTable;


SInt idnAsciiAt( idnChar*c, const UChar* s1, UShort s1Length, idnIndex* index )
{

#define IDE_FN "SInt idnAsciiAt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if( idnCharAt( c, s1, s1Length, index ) != IDE_SUCCESS )
    {
        IDE_RAISE( ERR_THROUGH );
    }

    if( idnToAscii( c ) != IDE_SUCCESS )
    {
        IDE_RAISE( ERR_THROUGH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnAddAscii( UChar* s1, UShort s1Max, UShort* s1Length, idnChar c )
{

#define IDE_FN "SInt idnAddAscii"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if( idnToLocal( &c ) != IDE_SUCCESS )
    {
        IDE_RAISE( ERR_THROUGH );
    }
    if( idnAddChar( s1, s1Max, s1Length, c ) != IDE_SUCCESS )
    {
        IDE_RAISE( ERR_THROUGH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnLength( UShort* length, const UChar* s1, UShort s1Length )
{

#define IDE_FN "SInt idnLength"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index;
    idnChar  char1;

    *length = 0;
    idnSkip( s1, s1Length, &index, 0 );
    while( 1 )
    {
        if( idnCharAt( &char1, s1, s1Length, &index ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE(
                ideGetErrorCode() == idERR_ABORT_idnInvalidCharacter,
                ERR_THROUGH );
            break;
        }
        (*length)++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnLengthWithoutSpaceUsingByteScan( UShort* length,
                                         const UChar* s1,
                                         UShort s1Length )
{

#define IDE_FN "SInt idnLengthWithoutSpaceUsingByteScan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const UChar* t;

    for( t = s1 + s1Length - 1; t >= s1; t-- ){
        if( *t != idnSpaceChar ){
            break;
        }
    }
    *length = t - s1 + 1;;

    return IDE_SUCCESS;


#undef IDE_FN
}

SInt idnCompareUsingMemCmp( SInt* order,
                            const UChar* s1,
                            UShort s1Length,
                            const UChar* s2,
                            UShort s2Length )
{

#define IDE_FN "SInt idnCompareUsingMemCmp"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    *order =
        idlOS::memcmp( (char*)s1, (char*)s2,
                       ( s1Length < s2Length )? s1Length : s2Length );
    if( *order == 0 )
    {
        if( s1Length > s2Length )
        {
            *order = 1;
        }
        if( s1Length < s2Length )
        {
            *order = -1;
        }
    }

    return IDE_SUCCESS;


#undef IDE_FN
}

SInt idnCompareUsingNls( SInt* order,
                         const UChar* s1,
                         UShort s1Length,
                         const UChar* s2,
                         UShort s2Length )
{

#define IDE_FN "SInt idnCompareUsingNls"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index1;
    idnIndex index2;
    idnChar  char1 = 0;
    idnChar  char2 = 0;

    *order = 0;
    idnSkip( s1, s1Length, &index1, 0 );
    idnSkip( s2, s2Length, &index2, 0 );
    while( 1 )
    {
        if( idnCharAt( &char1, s1, s1Length, &index1 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode() ==
                            idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );

            if( ideGetErrorCode() == idERR_ABORT_idnReachEnd )
            {
                if( idnCharAt( &char2, s2, s2Length, &index2 ) != IDE_SUCCESS )
                {
                    IDE_TEST_RAISE( ideGetErrorCode() ==
                                    idERR_ABORT_idnInvalidCharacter,
                                    ERR_THROUGH );
                }
                else
                {
                    *order = -1;
                }
                break;
            }
        }
        if( idnCharAt( &char2, s2, s2Length, &index2 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode() ==
                            idERR_ABORT_idnInvalidCharacter,
                            ERR_THROUGH );
            if( ideGetErrorCode() == idERR_ABORT_idnReachEnd )
            {
                *order = 1;
                break;
            }
        }
        IDE_TEST_RAISE ( idnToCompareOrder( &char1 ) != IDE_SUCCESS,
                         ERR_THROUGH );
        IDE_TEST_RAISE ( idnToCompareOrder( &char2 ) != IDE_SUCCESS,
                         ERR_THROUGH );
        if( char1 > char2 )
        {
            *order = 1;
            break;
        }
        if( char1 < char2 )
        {
            *order = -1;
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnCompareWithSpace( SInt* order,
                          const UChar* s1,
                          UShort s1Length,
                          const UChar* s2,
                          UShort s2Length )
{

#define IDE_FN "SInt idnCompareWithSpace"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // BUGBUG(assam): do not support nls.
    if( s1Length <= s2Length )
    {
        *order = idlOS::memcmp( s1, s2, s1Length );
        if( *order == 0 )
        {
            for( ; s1Length < s2Length; s1Length++ )
            {
                if( s2[s1Length] != ' ' )
                {
                    *order = -1;
                    break;
                }
            }
        }
    }
    else
    {
        *order = idlOS::memcmp( s1, s2, s2Length );
        if( *order == 0 )
        {
            for( ; s2Length < s1Length; s2Length++ )
            {
                if( s1[s2Length] != ' ' )
                {
                    *order = 1;
                    break;
                }
            }
        }
    }

    return IDE_SUCCESS;


#undef IDE_FN
}

SInt idnCaselessCompare( SInt* order,
                         const UChar* s1,
                         UShort s1Length,
                         const UChar* s2,
                         UShort s2Length )
{

#define IDE_FN "SInt idnCaselessCompare"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index1;
    idnIndex index2;
    idnChar  char1 = 0;
    idnChar  char2 = 0;

    *order = 0;
    idnSkip( s1, s1Length, &index1, 0 );
    idnSkip( s2, s2Length, &index2, 0 );
    while( 1 )
    {
        if( idnCharAt( &char1, s1, s1Length, &index1 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode() ==
                            idERR_ABORT_idnInvalidCharacter,
                            ERR_THROUGH );

            if( ideGetErrorCode() == idERR_ABORT_idnReachEnd )
            {
                if( idnCharAt( &char2, s2, s2Length, &index2 ) != IDE_SUCCESS )
                {
                    IDE_TEST_RAISE( ideGetErrorCode()
                                    == idERR_ABORT_idnInvalidCharacter,
                                    ERR_THROUGH );
                }
                else
                {
                    *order = -1;
                }
                break;
            }
        }

        if( idnCharAt( &char2, s2, s2Length, &index2 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter,
                            ERR_THROUGH );

            if( ideGetErrorCode() == idERR_ABORT_idnReachEnd )
            {
                *order = 1;
                break;
            }
        }
        IDE_TEST_RAISE ( idnToCaselessCompareOrder( &char1 ) != IDE_SUCCESS,
                         ERR_THROUGH );
        IDE_TEST_RAISE ( idnToCaselessCompareOrder( &char2 ) != IDE_SUCCESS,
                         ERR_THROUGH );
        if( char1 > char2 )
        {
            *order = 1;
            break;
        }
        if( char1 < char2 )
        {
            *order = -1;
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnCopy( UChar* s1,
              UShort s1Max,
              UShort* s1Length,
              const UChar* s2,
              UShort s2Length  )
{

#define IDE_FN "SInt idnCopy"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // BUGBUG: This function don't care NLS.
    if( s1Max < s2Length ){
        idlOS::memcpy( s1, s2, s1Max );
        IDE_RAISE( ERR_REACH_END );
    }else{
        idlOS::memcpy( s1, s2, s2Length );
        *s1Length = s2Length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REACH_END );
    IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnConcat( UChar* s1,
                UShort s1Max,
                UShort* s1Length,
                const UChar* s2,
                UShort s2Length )
{

#define IDE_FN "SInt idnConcat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index;
    idnChar  c;

    idnSkip( s1, *s1Length, &index, 0 );
    while( 1 )
    {
        if( idnCharAt( &c, s1, *s1Length, &index ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter,
                            ERR_THROUGH );
            break;
        }
    }

    idnSkip( s2, s2Length, &index, 0 );
    while( 1 )
    {
        if( idnCharAt( &c, s2, s2Length, &index ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter,
                            ERR_THROUGH );
            break;
        }
        if( idnAddChar( s1, s1Max, s1Length, c ) != IDE_SUCCESS )
        {
            IDE_RAISE( ERR_THROUGH );
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnSubstr( UChar* s1,
                UShort s1Max,
                UShort* s1Length,
                const UChar* s2,
                UShort s2Length,
                UShort start,
                UShort length )
{

#define IDE_FN "SInt idnSubstr"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index;
    idnChar  c;

    *s1Length = 0;
    if( idnSkip( s2, s2Length, &index, start ) != IDE_SUCCESS )
    {
        IDE_TEST_RAISE( ideGetErrorCode()
                        == idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );
    }

    for( ; length > 0 ; length-- )
    {
        if( idnCharAt( &c, s2, s2Length, &index ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );
            break;
        }
        if( idnAddChar( s1, s1Max, s1Length, c ) != IDE_SUCCESS )
        {
            IDE_RAISE( ERR_THROUGH );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnUpper( UChar* s1,
               UShort s1Max,
               UShort* s1Length,
               const UChar* s2,
               UShort s2Length )
{

#define IDE_FN "SInt idnUpper"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index;
    idnChar  c;

    *s1Length = 0;
    idnSkip( s2, s2Length, &index, 0 );
    while( 1 )
    {
        if( idnCharAt( &c, s2, s2Length, &index ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );
            break;
        }
        IDE_TEST_RAISE( idnToUpper( &c ) != IDE_SUCCESS, ERR_THROUGH );
        IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c ) != IDE_SUCCESS,
                        ERR_THROUGH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnLower( UChar* s1,
               UShort s1Max,
               UShort* s1Length,
               const UChar* s2,
               UShort s2Length )
{

#define IDE_FN "SInt idnLower"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index;
    idnChar  c;

    *s1Length = 0;
    idnSkip( s2, s2Length, &index, 0 );
    while( 1 )
    {
        if( idnCharAt( &c, s2, s2Length, &index ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );
            break;
        }

        IDE_TEST_RAISE( idnToLower( &c ) != IDE_SUCCESS, ERR_THROUGH );
        IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c ) != IDE_SUCCESS,
                        ERR_THROUGH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnLtrim( UChar* s1,
               UShort s1Max,
               UShort* s1Length,
               const UChar* s2,
               UShort s2Length,
               const UChar* s3,
               UShort s3Length )
{

#define IDE_FN "SInt idnLtrim"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index1;
    idnIndex index2;
    idnChar  c1;
    idnChar  c2;

    *s1Length = 0;
    idnSkip( s2, s2Length, &index1, 0 );
    while( 1 )
    {
        if( idnCharAt( &c1, s2, s2Length, &index1 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );
            break;
        }

        idnSkip( s3, s3Length, &index2, 0 );
        while( 1 )
        {
            if( idnCharAt( &c2, s3, s3Length, &index2 ) != IDE_SUCCESS )
            {
                if( ideGetErrorCode() == idERR_ABORT_idnReachEnd )
                {
                    IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c1 )
                                    != IDE_SUCCESS, ERR_THROUGH );
                    while( 1 )
                    {
                        if( idnCharAt( &c1, s2, s2Length, &index1 )
                            != IDE_SUCCESS )
                        {
                            IDE_TEST_RAISE( ideGetErrorCode()
                                            == idERR_ABORT_idnInvalidCharacter,
                                            ERR_THROUGH );
                            break;
                        }
                        IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c1 )
                                        != IDE_SUCCESS, ERR_THROUGH );
                    }
                    goto success;
                }
                IDE_RAISE( ERR_THROUGH );
            }
            if( c1 == c2 )
            {
                break;
            }
        }
    }

  success:
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnRtrim( UChar* s1,
               UShort s1Max,
               UShort* s1Length,
               const UChar* s2,
               UShort s2Length,
               const UChar* s3,
               UShort s3Length )
{

#define IDE_FN "SInt idnRtrim"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index1;
    idnIndex index2;
    UShort   lastCount;
    UShort   count;
    idnChar  c1;
    idnChar  c2;

    *s1Length = 0;
    lastCount = 0;
    count     = 0;
    idnSkip( s2, s2Length, &index1, 0 );
    while( 1 )
    {
        if( idnCharAt( &c1, s2, s2Length, &index1 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );
            break;
        }

        idnSkip( s3, s3Length, &index2, 0 );
        while( 1 )
        {
            if( idnCharAt( &c2, s3, s3Length, &index2 ) != IDE_SUCCESS )
            {
                IDE_TEST_RAISE( ideGetErrorCode()
                                == idERR_ABORT_idnInvalidCharacter,
                                ERR_THROUGH );
                count++;
                lastCount = count;
                break;
            }
            if( c1 == c2 )
            {
                count++;
                break;
            }
        }
    }

    idnSkip( s2, s2Length, &index1, 0 );
    for( ; lastCount > 0; lastCount-- )
    {
        if( idnCharAt( &c1, s2, s2Length, &index1 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );
            break;
        }
        IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c1 ) != IDE_SUCCESS,
                        ERR_THROUGH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnTrim( UChar* s1,
              UShort s1Max,
              UShort* s1Length,
              const UChar* s2,
              UShort s2Length,
              const UChar* s3,
              UShort s3Length )
{

#define IDE_FN "SInt idnTrim"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index1;
    idnIndex index2;
    idnIndex index3;
    UShort   lastCount;
    UShort   count;
    idnChar  c1;
    idnChar  c2;

    *s1Length = 0;
    idnSkip( s2, s2Length, &index1, 0 );
    while( 1 )
    {
        if( idnCharAt( &c1, s2, s2Length, &index1 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode()
                            == idERR_ABORT_idnInvalidCharacter, ERR_THROUGH );
            break;
        }
        idnSkip( s3, s3Length, &index2, 0 );
        while( 1 )
        {
            if( idnCharAt( &c2, s3, s3Length, &index2 ) != IDE_SUCCESS )
            {
                if( ideGetErrorCode() == idERR_ABORT_idnReachEnd )
                {
                    IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c1 )
                                    != IDE_SUCCESS, ERR_THROUGH );
                    lastCount = 0;
                    count     = 0;
                    index2    = index1;
                    while( 1 )
                    {
                        if( idnCharAt( &c1, s2, s2Length, &index2 )
                            != IDE_SUCCESS )
                        {
                            IDE_TEST_RAISE( ideGetErrorCode()
                                            == idERR_ABORT_idnInvalidCharacter,
                                            ERR_THROUGH );
                            break;
                        }
                        idnSkip( s3, s3Length, &index3, 0 );
                        while( 1 )
                        {
                            if( idnCharAt( &c2, s3, s3Length, &index3 )
                                != IDE_SUCCESS )
                            {
                                IDE_TEST_RAISE(
                                    ideGetErrorCode()
                                    == idERR_ABORT_idnInvalidCharacter,
                                    ERR_THROUGH );
                                count++;
                                lastCount = count;
                                break;
                            }
                            if( c1 == c2 )
                            {
                                count++;
                                break;
                            }
                        }
                    }
                    for( ; lastCount > 0; lastCount-- )
                    {
                        if( idnCharAt( &c1, s2, s2Length, &index1 )
                            != IDE_SUCCESS )
                        {
                            IDE_TEST_RAISE(
                                ideGetErrorCode()
                                == idERR_ABORT_idnInvalidCharacter,
                                ERR_THROUGH );
                            break;
                        }
                        IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c1 )
                                        != IDE_SUCCESS, ERR_THROUGH );
                    }
                    goto success;
                }
                IDE_RAISE( ERR_THROUGH );
            }
            if( c1 == c2 )
            {
                break;
            }
        }
    }

  success:
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnPosition( UShort* position,
                  const UChar* s1,
                  UShort s1Length,
                  const UChar* s2,
                  UShort s2Length )
{

#define IDE_FN "SInt idnPosition"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index1;
    idnIndex index2;
    idnIndex index3;
    idnChar  c1;
    idnChar  c2;

    *position = 0;
    idnSkip( s1, s1Length, &index1, 0 );
    while( 1 )
    {
        index2 = index1;
        idnSkip( s2, s2Length, &index3, 0 );
        while( 1 )
        {
            if( idnCharAt( &c2, s2, s2Length, &index3 ) != IDE_SUCCESS )
            {
                if( ideGetErrorCode() == idERR_ABORT_idnReachEnd )
                {
                    goto success;
                }
                IDE_RAISE( ERR_THROUGH );
            }
            if( idnCharAt( &c1, s1, s1Length, &index2 ) != IDE_SUCCESS )
            {
                IDE_TEST_RAISE( ideGetErrorCode() == idERR_ABORT_idnReachEnd,
                                ERR_NOT_FOUND );
                IDE_RAISE( ERR_THROUGH );
            }
            if( c1 != c2 )
            {
                break;
            }
        }
        if( idnCharAt( &c1, s1, s1Length, &index1 ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode() == idERR_ABORT_idnReachEnd,
                            ERR_NOT_FOUND );
            IDE_RAISE( ERR_THROUGH );
        }
        (*position)++;
    }

  success:
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND );
    IDE_SET(ideSetErrorCode( idERR_ABORT_idnNotFound ));

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnNextString( UChar* s1,
                    UShort s1Max,
                    UShort* s1Length,
                    const UChar* s2, UShort s2Length )
{

#define IDE_FN "SInt idnNextString"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnChar  str[4000*8];
    idnIndex index;
    UShort   max;
    SInt     i;
    SInt     carry;

    *s1Length = 0;
    idnSkip( s2, s2Length, &index, 0 );
    for( max = 0; ; max++ )
    {
        IDE_TEST_RAISE( max >= sizeof(str)/sizeof(idnChar), ERR_TOO_LARGE );
        if( idnCharAt( &(str[max]), s2, s2Length, &index ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode() != idERR_ABORT_idnReachEnd,
                            ERR_THROUGH );
            break;
        }
    }
    carry = 0;
    for( i = max - 1; i >= 0; i-- )
    {
        IDE_TEST_RAISE( idnNextChar( &(str[i]), &carry ) != IDE_SUCCESS,
                        ERR_THROUGH );
        if( carry )
        {
            max = (UShort)i;
        }
        else
        {
            break;
        }
    }
    if( !carry )
    {
        for( i = 0; i < max; i++ )
        {
            IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, str[i] )
                            != IDE_SUCCESS, ERR_THROUGH );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LARGE );
    IDE_SET(ideSetErrorCode( idERR_ABORT_idnTooLarge ));

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

#define IDN_NO_ESCAPE ID_ULONG(0xFFFFFFFFFFFFFFFF)

SInt idnLikeKey( UChar* s1,
                 UShort s1Max,
                 UShort* s1Length,
                 const UChar* s2,
                 UShort s2Length,
                 const UChar* s3,
                 UShort s3Length )
{

#define IDE_FN "SInt idnLikeKey"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index;
    idnChar  escape;
    idnChar  c;

    idnSkip( s3, s3Length, &index, 0 );
    if( idnCharAt( &escape, s3, s3Length, &index ) != IDE_SUCCESS )
    {
        IDE_TEST_RAISE( ideGetErrorCode() != idERR_ABORT_idnReachEnd,
                        ERR_THROUGH );
        escape = IDN_NO_ESCAPE;
    }
    else
    {
        IDE_TEST_RAISE( idnCharAt( &c, s3, s3Length, &index ) == IDE_SUCCESS,
                        ERR_ESCAPE );
        IDE_TEST_RAISE( ideGetErrorCode() != idERR_ABORT_idnReachEnd,
                        ERR_THROUGH );
    }

    *s1Length = 0;
    idnSkip( s2, s2Length, &index, 0 );
    while( 1 )
    {
        if( idnCharAt( &c, s2, s2Length, &index ) != IDE_SUCCESS )
        {
            IDE_TEST_RAISE( ideGetErrorCode() != idERR_ABORT_idnReachEnd,
                            ERR_THROUGH );
            break;
        }

        if( escape != IDN_NO_ESCAPE && c == escape )
        {
            IDE_TEST_RAISE( idnCharAt( &c, s2, s2Length, &index )
                            != IDE_SUCCESS , ERR_THROUGH );

            IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c )
                            != IDE_SUCCESS, ERR_THROUGH );

        }
        else
        {
            if( c == idnPercentSignChar || c == idnUnderscoreChar )
            {
                break;
            }

            IDE_TEST_RAISE( idnAddChar( s1, s1Max, s1Length, c )
                            != IDE_SUCCESS, ERR_THROUGH );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ESCAPE );
    IDE_SET(ideSetErrorCode( idERR_ABORT_idnLikeEscape ));

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnLike( SInt* match,
              const UChar* s1,
              UShort s1Length,
              const UChar* s2,
              UShort s2Length,
              const UChar* s3,
              UShort s3Length )
{

#define IDE_FN "SInt idnLike"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idnIndex index1;
    idnIndex index2;
    idnIndex index3;
    idnIndex index4;
    idnChar  escape;
    idnChar  c1;
    idnChar  c2;
    SInt     percent;

    idnSkip( s3, s3Length, &index1, 0 );
    if( idnCharAt( &escape, s3, s3Length, &index1 ) != IDE_SUCCESS )
    {
        IDE_TEST_RAISE( ideGetErrorCode() != idERR_ABORT_idnReachEnd,
                        ERR_THROUGH );
        escape = IDN_NO_ESCAPE;
    }
    else
    {
        IDE_TEST_RAISE( idnCharAt( &c1, s3, s3Length, &index1 ) == IDE_SUCCESS,
                        ERR_ESCAPE );
        IDE_TEST_RAISE( ideGetErrorCode() != idERR_ABORT_idnReachEnd,
                        ERR_THROUGH );
    }

    *match  = 0;
    percent = 0;
    idnSkip( s1, s1Length, &index1, 0 );
    idnSkip( s2, s2Length, &index2, 0 );
    index3 = index1;
    index4 = index2;
    while( 1 )
    {
        while( 1 )
        {
            if( idnCharAt( &c2, s2, s2Length, &index4 ) != IDE_SUCCESS )
            {
                IDE_TEST_RAISE( ideGetErrorCode() != idERR_ABORT_idnReachEnd,
                                ERR_THROUGH );
                if( idnCharAt( &c1, s1, s1Length, &index3 ) != IDE_SUCCESS )
                {
                    IDE_TEST_RAISE( ideGetErrorCode()
                                    != idERR_ABORT_idnReachEnd, ERR_THROUGH );
                    *match = 1;
                    goto final;
                }
                else
                {
                    break;
                }
            }

            if( escape != IDN_NO_ESCAPE && c2 == escape )
            {
                IDE_TEST_RAISE( idnCharAt( &c2, s2, s2Length, &index4 )
                                != IDE_SUCCESS, ERR_THROUGH );

                if( idnCharAt( &c1, s1, s1Length, &index3 ) != IDE_SUCCESS )
                {
                    IDE_TEST_RAISE( ideGetErrorCode()
                                    != idERR_ABORT_idnReachEnd, ERR_THROUGH );
                    goto final;
                }
                if( c1 != c2 )
                {
                    break;
                }
            }
            else
            {
                if( c2 == idnPercentSignChar )
                {
                    index1  = index3;
                    index2  = index4;
                    percent = 1;
                }
                else
                {
                    if( idnCharAt( &c1, s1, s1Length, &index3 )
                        != IDE_SUCCESS )
                    {
                        IDE_TEST_RAISE( ideGetErrorCode()
                                        != idERR_ABORT_idnReachEnd,
                                        ERR_THROUGH );
                        goto final;
                    }

                    if( c2 != idnUnderscoreChar && c1 != c2 )
                    {
                        break;
                    }
                }
            }
        }

        if( percent )
        {
            if( idnCharAt( &c1, s1, s1Length, &index1 ) != IDE_SUCCESS )
            {
                IDE_TEST_RAISE( ideGetErrorCode() != idERR_ABORT_idnReachEnd,
                                ERR_THROUGH );
            }
            index3 = index1;
            index4 = index2;
        }else{
            goto final;
        }
    }
  final:

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ESCAPE );
    IDE_SET(ideSetErrorCode( idERR_ABORT_idnLikeEscape ));

    IDE_EXCEPTION( ERR_THROUGH );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnSkip( const UChar* /*_ s1 _*/,
              UShort s1Length,
              idnIndex* index,
              UShort skip )
{

#define IDE_FN "static SInt idnSkip"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    index->US7ASCII.index = 0;
    for( ; skip > 0; skip-- )
    {
        if( (index->US7ASCII.index) >= s1Length )
        {
            break;
        }
        (index->US7ASCII.index)++;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

SInt idnCharAt( idnChar* c,
                const UChar* s1,
                UShort s1Length,
                idnIndex* index )
{

#define IDE_FN "static SInt idnCharAt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE( (index->US7ASCII.index) >= s1Length, ERR_REACH_END );
    *c = s1[index->US7ASCII.index];
    (index->US7ASCII.index)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REACH_END );
    IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnAddChar( UChar* s1,
                 UShort s1Max,
                 UShort* s1Length,
                 idnChar c )
{

#define IDE_FN "static SInt idnAddChar"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE( (*s1Length) >= s1Max, ERR_REACH_END );
    s1[*s1Length] = (UChar)c;
    (*s1Length)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REACH_END );
    IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

SInt idnToUpper( idnChar* c )
{

#define IDE_FN "static SInt idnToUpper"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    *c = idn_US7ASCII_UpperTable[*c];

    return IDE_SUCCESS;

#undef IDE_FN
}

SInt idnToLower( idnChar* c )
{

#define IDE_FN "static SInt idnToLower"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    *c = idn_US7ASCII_LowerTable[*c];

    return IDE_SUCCESS;

#undef IDE_FN
}

SInt idnToAscii( idnChar* /*_ c _*/)
{

#define IDE_FN "static SInt idnToAscii"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;

#undef IDE_FN
}

SInt idnToLocal( idnChar* /*_ c _*/)
{

#define IDE_FN "static SInt idnToLocal"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;

#undef IDE_FN
}

SInt idnToCompareOrder( idnChar* /*_ c  _*/)
{

#define IDE_FN "static SInt idnToCompareOrder"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;

#undef IDE_FN
}

SInt idnToCaselessCompareOrder( idnChar* c )
{

#define IDE_FN "static SInt idnToCaselessCompareOrder"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    *c = idn_US7ASCII_CaselessCompareTable[*c];

    return IDE_SUCCESS;

#undef IDE_FN
}

SInt idnNextChar( idnChar* c, SInt* carry )
{

#define IDE_FN "static SInt idnNextChar"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    (*c)++;
    if( *c == 0xFF )
    {
        *c = 0x00;
        *carry = 1;
    }
    else
    {
        *carry = 0;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}
 
