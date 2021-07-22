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
 
/***********************************************************************
 * $Id: idnKorean.h 80575 2017-07-21 07:06:35Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_IDN_KOREAN_H_
# define  _O_IDN_KOREAN_H_  1

# include <idTypes.h>
# include <idnCharSet.h>

#define idnKoreanSpaceChar               (0x20)
#define idnKoreanExclamationPointChar    (0x21)
#define idnKoreanQuotationMarkChar       (0x22)
#define idnKoreanNumberSignChar          (0x23)
#define idnKoreanDollarSignChar          (0x24)
#define idnKoreanPercentSignChar         (0x25)
#define idnKoreanAmpersandChar           (0x26)
#define idnKoreanApostropheChar          (0x27)
#define idnKoreanOpeningParenthesisChar  (0x28)
#define idnKoreanClosingParenthesisChar  (0x29)
#define idnKoreanAsteriskChar            (0x2A)
#define idnKoreanPlusSignChar            (0x2B)
#define idnKoreanCommaChar               (0x2C)
#define idnKoreanHyphenChar              (0x2D)
#define idnKoreanMinusSignChar           (0x2D)
#define idnKoreanPeriodChar              (0x2E)
#define idnKoreanSlashChar               (0x2F)
#define idnKoreanZeroChar                (0x30)
#define idnKoreanOneChar                 (0x31)
#define idnKoreanTwoChar                 (0x32)
#define idnKoreanThreeChar               (0x33)
#define idnKoreanFourChar                (0x34)
#define idnKoreanFiveChar                (0x35)
#define idnKoreanSixChar                 (0x36)
#define idnKoreanSevenChar               (0x37)
#define idnKoreanEightChar               (0x38)
#define idnKoreanNineChar                (0x39)
#define idnKoreanColonChar               (0x3A)
#define idnKoreanSemicolonChar           (0x3B)
#define idnKoreanLessThanSignChar        (0x3C)
#define idnKoreanEqualSignChar           (0x3D)
#define idnKoreanGreaterThanSignChar     (0x3E)
#define idnKoreanQuestionMarkChar        (0x3F)
#define idnKoreanAtSignChar              (0x40)
#define idnKoreanCapitalAChar            (0x41)
#define idnKoreanCapitalBChar            (0x42)
#define idnKoreanCapitalCChar            (0x43)
#define idnKoreanCapitalDChar            (0x44)
#define idnKoreanCapitalEChar            (0x45)
#define idnKoreanCapitalFChar            (0x46)
#define idnKoreanCapitalGChar            (0x47)
#define idnKoreanCapitalHChar            (0x48)
#define idnKoreanCapitalIChar            (0x49)
#define idnKoreanCapitalJChar            (0x4A)
#define idnKoreanCapitalKChar            (0x4B)
#define idnKoreanCapitalLChar            (0x4C)
#define idnKoreanCapitalMChar            (0x4D)
#define idnKoreanCapitalNChar            (0x4E)
#define idnKoreanCapitalOChar            (0x4F)
#define idnKoreanCapitalPChar            (0x50)
#define idnKoreanCapitalQChar            (0x51)
#define idnKoreanCapitalRChar            (0x52)
#define idnKoreanCapitalSChar            (0x53)
#define idnKoreanCapitalTChar            (0x54)
#define idnKoreanCapitalUChar            (0x55)
#define idnKoreanCapitalVChar            (0x56)
#define idnKoreanCapitalWChar            (0x57)
#define idnKoreanCapitalXChar            (0x58)
#define idnKoreanCapitalYChar            (0x59)
#define idnKoreanCapitalZChar            (0x5A)
#define idnKoreanOpeningBracketChar      (0x5B)
#define idnKoreanBackwardSlashChar       (0x5C)
#define idnKoreanClosingBracketChar      (0x5D)
#define idnKoreanCaretChar               (0x5E)
#define idnKoreanUnderscoreChar          (0x5F)
#define idnKoreanGraveChar               (0x60)
#define idnKoreanLowercaseAChar          (0x61)
#define idnKoreanLowercaseBChar          (0x62)
#define idnKoreanLowercaseCChar          (0x63)
#define idnKoreanLowercaseDChar          (0x64)
#define idnKoreanLowercaseEChar          (0x65)
#define idnKoreanLowercaseFChar          (0x66)
#define idnKoreanLowercaseGChar          (0x67)
#define idnKoreanLowercaseHChar          (0x68)
#define idnKoreanLowercaseIChar          (0x69)
#define idnKoreanLowercaseJChar          (0x6A)
#define idnKoreanLowercaseKChar          (0x6B)
#define idnKoreanLowercaseLChar          (0x6C)
#define idnKoreanLowercaseMChar          (0x6D)
#define idnKoreanLowercaseNChar          (0x6E)
#define idnKoreanLowercaseOChar          (0x6F)
#define idnKoreanLowercasePChar          (0x70)
#define idnKoreanLowercaseQChar          (0x71)
#define idnKoreanLowercaseRChar          (0x72)
#define idnKoreanLowercaseSChar          (0x73)
#define idnKoreanLowercaseTChar          (0x74)
#define idnKoreanLowercaseUChar          (0x75)
#define idnKoreanLowercaseVChar          (0x76)
#define idnKoreanLowercaseWChar          (0x77)
#define idnKoreanLowercaseXChar          (0x78)
#define idnKoreanLowercaseYChar          (0x79)
#define idnKoreanLowercaseZChar          (0x7A)
#define idnKoreanOpeningBraceChar        (0x7B)
#define idnKoreanVerticalLineChar        (0x7C)
#define idnKoreanClosingBraceChar        (0x7D)
#define idnKoreanTildeChar               (0x7E)

typedef ULong idnKoreanChar;

typedef union idnKoreanIndex{
    struct US7ASCII {
	ULong index;
    } US7ASCII;
    struct KO16KSC5601 {
	ULong index;
    } KO16KSC5601;
} idnKoreanIndex;

extern idnKoreanChar idnKoreanAsciiChars[128];

extern UChar idnKoreanSpaceTemplete[128];
extern UChar idnKoreanWeekDaysTemplete[7][128];
extern UChar idnKoreanTruncateTemplete[7][128];

extern UShort idnKoreanSpaceTempleteLength;
extern UShort idnKoreanWeekDaysTempleteLength[7];
extern UShort idnKoreanTruncateTempleteLength[7];

SInt idnKoreanSkip( const UChar* s1, UShort s1Length, idnKoreanIndex* index, UShort skip );
SInt idnKoreanCharAt( idnKoreanChar* c, const UChar* s1, UShort s1Length, idnKoreanIndex* index );
SInt idnKoreanAddChar( UChar* s1, UShort s1Max, UShort* s1Length, idnKoreanChar c );
SInt idnKoreanToUpper( idnKoreanChar* c );
SInt idnKoreanToLower( idnKoreanChar* c );
SInt idnKoreanToAscii( idnKoreanChar* c );
SInt idnKoreanToLocal( idnKoreanChar* c );
SInt idnKoreanToCompareOrder(idnKoreanChar* c );
SInt idnKoreanToCaselessCompareOrder( idnKoreanChar* c );
SInt idnKoreanNextChar( idnKoreanChar* c, SInt* carry );
SInt idnKoreanCompare( SInt* order, const UChar* s1, UShort s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanLengthWithoutSpace( UShort* length, const UChar* s1, UShort s1Length );

SInt idnKoreanAsciiAt( idnKoreanChar*c, const UChar* s1, UShort s1Length, idnKoreanIndex* index );
SInt idnKoreanAddAscii( UChar* s1, UShort s1Max, UShort* s1Length, idnKoreanChar c );
SInt idnKoreanLength( UShort* length, const UChar* s1, UShort s1Length );
SInt idnKoreanLengthWithoutSpaceUsingByteScan( UShort* length, const UChar* s1, UShort s1Length );
SInt idnKoreanCompareUsingMemCmp( SInt* order, const UChar* s1, UShort s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanCompareUsingNls   ( SInt* order, const UChar* s1, UShort s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanCompareWithSpace( SInt* order, const UChar* s1, UShort s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanCaselessCompare( SInt* order, const UChar* s1, UShort s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanCopy( UChar* s1, UShort s1Max, UShort* s1Length, const UChar* s2, UShort s2Length  );
SInt idnKoreanConcat( UChar* s1, UShort s1Max, UShort* s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanSubstr( UChar* s1, UShort s1Max, UShort* s1Length, const UChar* s2, UShort s2Length, UShort start, UShort length );
SInt idnKoreanUpper( UChar* s1, UShort s1Max, UShort* s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanLower( UChar* s1, UShort s1Max, UShort* s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanLtrim( UChar* s1, UShort v1Max, UShort* s1Length, const UChar* s2, UShort s2Length, const UChar* s3, UShort s3Length );
SInt idnKoreanRtrim( UChar* s1, UShort v1Max, UShort* s1Length, const UChar* s2, UShort s2Length, const UChar* s3, UShort s3Length );
SInt idnKoreanTrim( UChar* s1, UShort s1Max, UShort* s1Length, const UChar* s2, UShort s2Length, const UChar* s3, UShort s3Length );
SInt idnKoreanPosition( UShort* position, const UChar* s1, UShort s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanNextString( UChar* s1, UShort s1Max, UShort* s1Length, const UChar* s2, UShort s2Length );
SInt idnKoreanLikeKey( UChar* s1, UShort s1Max, UShort* s1Length, const UChar* s2, UShort s2Length, const UChar* s3, UShort s3Length );
SInt idnKoreanLike( SInt* match, const UChar* s1, UShort s1Length, const UChar* s2, UShort s2Length, const UChar* s3, UShort s3Length );

#endif /* _O_IDN_KOREAN_H_ */
