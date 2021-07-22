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
 * $Id: idn.h 80575 2017-07-21 07:06:35Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_IDN_H_
# define  _O_IDN_H_  1

# include <idTypes.h>
# include <idnCharSet.h>
# include <idnConv.h>

#define idnSpaceChar               (idnAsciiChars[0x20])
#define idnExclamationPointChar    (idnAsciiChars[0x21])
#define idnQuotationMarkChar       (idnAsciiChars[0x22])
#define idnNumberSignChar          (idnAsciiChars[0x23])
#define idnDollarSignChar          (idnAsciiChars[0x24])
#define idnPercentSignChar         (idnAsciiChars[0x25])
#define idnAmpersandChar           (idnAsciiChars[0x26])
#define idnApostropheChar          (idnAsciiChars[0x27])
#define idnOpeningParenthesisChar  (idnAsciiChars[0x28])
#define idnClosingParenthesisChar  (idnAsciiChars[0x29])
#define idnAsteriskChar            (idnAsciiChars[0x2A])
#define idnPlusSignChar            (idnAsciiChars[0x2B])
#define idnCommaChar               (idnAsciiChars[0x2C])
#define idnHyphenChar              (idnAsciiChars[0x2D])
#define idnMinusSignChar           (idnAsciiChars[0x2D])
#define idnPeriodChar              (idnAsciiChars[0x2E])
#define idnSlashChar               (idnAsciiChars[0x2F])
#define idnZeroChar                (idnAsciiChars[0x30])
#define idnOneChar                 (idnAsciiChars[0x31])
#define idnTwoChar                 (idnAsciiChars[0x32])
#define idnThreeChar               (idnAsciiChars[0x33])
#define idnFourChar                (idnAsciiChars[0x34])
#define idnFiveChar                (idnAsciiChars[0x35])
#define idnSixChar                 (idnAsciiChars[0x36])
#define idnSevenChar               (idnAsciiChars[0x37])
#define idnEightChar               (idnAsciiChars[0x38])
#define idnNineChar                (idnAsciiChars[0x39])
#define idnColonChar               (idnAsciiChars[0x3A])
#define idnSemicolonChar           (idnAsciiChars[0x3B])
#define idnLessThanSignChar        (idnAsciiChars[0x3C])
#define idnEqualSignChar           (idnAsciiChars[0x3D])
#define idnGreaterThanSignChar     (idnAsciiChars[0x3E])
#define idnQuestionMarkChar        (idnAsciiChars[0x3F])
#define idnAtSignChar              (idnAsciiChars[0x40])
#define idnCapitalAChar            (idnAsciiChars[0x41])
#define idnCapitalBChar            (idnAsciiChars[0x42])
#define idnCapitalCChar            (idnAsciiChars[0x43])
#define idnCapitalDChar            (idnAsciiChars[0x44])
#define idnCapitalEChar            (idnAsciiChars[0x45])
#define idnCapitalFChar            (idnAsciiChars[0x46])
#define idnCapitalGChar            (idnAsciiChars[0x47])
#define idnCapitalHChar            (idnAsciiChars[0x48])
#define idnCapitalIChar            (idnAsciiChars[0x49])
#define idnCapitalJChar            (idnAsciiChars[0x4A])
#define idnCapitalKChar            (idnAsciiChars[0x4B])
#define idnCapitalLChar            (idnAsciiChars[0x4C])
#define idnCapitalMChar            (idnAsciiChars[0x4D])
#define idnCapitalNChar            (idnAsciiChars[0x4E])
#define idnCapitalOChar            (idnAsciiChars[0x4F])
#define idnCapitalPChar            (idnAsciiChars[0x50])
#define idnCapitalQChar            (idnAsciiChars[0x51])
#define idnCapitalRChar            (idnAsciiChars[0x52])
#define idnCapitalSChar            (idnAsciiChars[0x53])
#define idnCapitalTChar            (idnAsciiChars[0x54])
#define idnCapitalUChar            (idnAsciiChars[0x55])
#define idnCapitalVChar            (idnAsciiChars[0x56])
#define idnCapitalWChar            (idnAsciiChars[0x57])
#define idnCapitalXChar            (idnAsciiChars[0x58])
#define idnCapitalYChar            (idnAsciiChars[0x59])
#define idnCapitalZChar            (idnAsciiChars[0x5A])
#define idnOpeningBracketChar      (idnAsciiChars[0x5B])
#define idnBackwardSlashChar       (idnAsciiChars[0x5C])
#define idnClosingBracketChar      (idnAsciiChars[0x5D])
#define idnCaretChar               (idnAsciiChars[0x5E])
#define idnUnderscoreChar          (idnAsciiChars[0x5F])
#define idnGraveChar               (idnAsciiChars[0x60])
#define idnLowercaseAChar          (idnAsciiChars[0x61])
#define idnLowercaseBChar          (idnAsciiChars[0x62])
#define idnLowercaseCChar          (idnAsciiChars[0x63])
#define idnLowercaseDChar          (idnAsciiChars[0x64])
#define idnLowercaseEChar          (idnAsciiChars[0x65])
#define idnLowercaseFChar          (idnAsciiChars[0x66])
#define idnLowercaseGChar          (idnAsciiChars[0x67])
#define idnLowercaseHChar          (idnAsciiChars[0x68])
#define idnLowercaseIChar          (idnAsciiChars[0x69])
#define idnLowercaseJChar          (idnAsciiChars[0x6A])
#define idnLowercaseKChar          (idnAsciiChars[0x6B])
#define idnLowercaseLChar          (idnAsciiChars[0x6C])
#define idnLowercaseMChar          (idnAsciiChars[0x6D])
#define idnLowercaseNChar          (idnAsciiChars[0x6E])
#define idnLowercaseOChar          (idnAsciiChars[0x6F])
#define idnLowercasePChar          (idnAsciiChars[0x70])
#define idnLowercaseQChar          (idnAsciiChars[0x71])
#define idnLowercaseRChar          (idnAsciiChars[0x72])
#define idnLowercaseSChar          (idnAsciiChars[0x73])
#define idnLowercaseTChar          (idnAsciiChars[0x74])
#define idnLowercaseUChar          (idnAsciiChars[0x75])
#define idnLowercaseVChar          (idnAsciiChars[0x76])
#define idnLowercaseWChar          (idnAsciiChars[0x77])
#define idnLowercaseXChar          (idnAsciiChars[0x78])
#define idnLowercaseYChar          (idnAsciiChars[0x79])
#define idnLowercaseZChar          (idnAsciiChars[0x7A])
#define idnOpeningBraceChar        (idnAsciiChars[0x7B])
#define idnVerticalLineChar        (idnAsciiChars[0x7C])
#define idnClosingBraceChar        (idnAsciiChars[0x7D])
#define idnTildeChar               (idnAsciiChars[0x7E])

typedef ULong idnChar;

typedef union idnIndex{
    struct US7ASCII {
	ULong index;
    } US7ASCII;
    struct KO16KSC5601 {
	ULong index;
    } KO16KSC5601;
} idnIndex;

extern idnChar idnAsciiChars[128];

extern UChar idnSpaceTemplete[128];
extern UChar idnWeekDaysTemplete[7][128];
extern UChar idnTruncateTemplete[7][128];

extern UShort idnSpaceTempleteLength;
extern UShort idnWeekDaysTempleteLength[7];
extern UShort idnTruncateTempleteLength[7];

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

SInt idnAsciiAt( idnChar*c,
                 const UChar* s1,
                 UShort s1Length,
                 idnIndex* index );

SInt idnAddAscii( UChar* s1, UShort s1Max, UShort* s1Length, idnChar c );

SInt idnLength( UShort* length, const UChar* s1, UShort s1Length );

SInt idnLengthWithoutSpaceUsingByteScan( UShort* length,
                                         const UChar* s1,
                                         UShort s1Length );

SInt idnCompareUsingMemCmp( SInt* order,
                            const UChar* s1,
                            UShort s1Length,
                            const UChar* s2,
                            UShort s2Length );

SInt idnCompareUsingNls   ( SInt* order,
                            const UChar* s1,
                            UShort s1Length,
                            const UChar* s2,
                            UShort s2Length );

SInt idnCompareWithSpace( SInt* order,
                          const UChar* s1,
                          UShort s1Length,
                          const UChar* s2,
                          UShort s2Length );

SInt idnCaselessCompare( SInt* order,
                         const UChar* s1,
                         UShort s1Length,
                         const UChar* s2,
                         UShort s2Length );

SInt idnCopy( UChar* s1,
              UShort s1Max,
              UShort* s1Length,
              const UChar* s2,
              UShort s2Length  );

SInt idnConcat( UChar* s1,
                UShort s1Max,
                UShort* s1Length,
                const UChar* s2,
                UShort s2Length );

SInt idnSubstr( UChar* s1,
                UShort s1Max,
                UShort* s1Length,
                const UChar* s2,
                UShort s2Length,
                UShort start,
                UShort length );

SInt idnUpper( UChar* s1,
               UShort s1Max,
               UShort* s1Length,
               const UChar* s2,
               UShort s2Length );

SInt idnLower( UChar* s1,
               UShort s1Max,
               UShort* s1Length,
               const UChar* s2,
               UShort s2Length );

SInt idnLtrim( UChar* s1,
               UShort v1Max,
               UShort* s1Length,
               const UChar* s2,
               UShort s2Length,
               const UChar* s3,
               UShort s3Length );

SInt idnRtrim( UChar* s1,
               UShort v1Max,
               UShort* s1Length,
               const UChar* s2,
               UShort s2Length,
               const UChar* s3,
               UShort s3Length );

SInt idnTrim( UChar* s1,
              UShort s1Max,
              UShort* s1Length,
              const UChar* s2,
              UShort s2Length,
              const UChar* s3,
              UShort s3Length );

SInt idnPosition( UShort* position,
                  const UChar* s1,
                  UShort s1Length,
                  const UChar* s2,
                  UShort s2Length );

SInt idnNextString( UChar* s1,
                    UShort s1Max,
                    UShort* s1Length,
                    const UChar* s2,
                    UShort s2Length );

SInt idnLikeKey( UChar* s1,
                 UShort s1Max,
                 UShort* s1Length,
                 const UChar* s2,
                 UShort s2Length,
                 const UChar* s3,
                 UShort s3Length );

SInt idnLike( SInt* match,
              const UChar* s1,
              UShort s1Length,
              const UChar* s2,
              UShort s2Length,
              const UChar* s3,
              UShort s3Length );

#endif /* _O_IDN_H_ */
 
