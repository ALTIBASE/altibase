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
 * $Id: mtdTypes.h 88554 2020-09-11 06:44:49Z donovan.seo $
 **********************************************************************/

#ifndef _O_MTD_TYPES_H_
#define _O_MTD_TYPES_H_ 1

#include <mtcDef.h>
#include <mtdTypeDef.h>

typedef struct mtdCharType {
    UShort length;
    UChar  value[1];
} mtdCharType;

#define MTD_CHAR_ALIGN             (ID_SIZEOF(UShort))
#define MTD_CHAR_PRECISION_DEFAULT (1) // To Fix BUG-12597

#define MTD_CHAR_TYPE_STRUCT_SIZE( valueLength )        \
    ( ID_SIZEOF( UShort ) + (valueLength) )

extern const mtdCharType mtdCharNull;

#define MTD_CHAR_PRECISION_MINIMUM (0)    // to fix BUG-12597
// BUG-19925 : 모든 데이터타입의 (GEOMETRY제외) 최대 크기를
// constant tuple의 최대 크기인 65536으로 제한함
#define MTD_CHAR_PRECISION_MAXIMUM ((SInt)(65536 - ID_SIZEOF(UShort)))

// fix BUG-22220
#define MTD_VARCHAR_PRECISION_MINIMUM (MTD_CHAR_PRECISION_MINIMUM)
#define MTD_VARCHAR_PRECISION_MAXIMUM (MTD_CHAR_PRECISION_MAXIMUM)

#define MTD_CHAR_STORE_PRECISION_MAXIMUM (32000)

typedef struct mtdNumericType {
    UChar length;
    UChar signExponent;
    UChar mantissa[1];
} mtdNumericType;

extern const mtdNumericType mtdNumericNull;

#define MTD_NUMERIC_ALIGN             (ID_SIZEOF(UChar))
#define MTD_NUMERIC_PRECISION_DEFAULT (38)
#define MTD_NUMERIC_PRECISION_MINIMUM (0)    // to fix BUG-12597
#define MTD_NUMERIC_PRECISION_MAXIMUM (38)
#define MTD_NUMERIC_SCALE_DEFAULT     (0)
#define MTD_NUMERIC_SCALE_MINIMUM     (-84)
#define MTD_NUMERIC_SCALE_MAXIMUM     (128)
#define MTD_NUMERIC_MANTISSA_MAXIMUM  MTD_NUMERIC_MANTISSA( MTD_NUMERIC_PRECISION_MAXIMUM )
#define MTD_NUMERIC_MANTISSA( p )     ( ( (p) + 2 ) / 2 )
#define MTD_NUMERIC_SIZE( p, s )      ( 3 + ( ( p ) + 2 ) / 2 ) // to fix BUG-12944, BUG-12970
#define MTD_NUMERIC_SIZE_MAXIMUM      ( MTD_NUMERIC_SIZE(                               \
                                            MTD_NUMERIC_PRECISION_MAXIMUM, 1 ) )
#define MTD_FLOAT_ALIGN             (ID_SIZEOF(UChar))
#define MTD_FLOAT_PRECISION_DEFAULT (38)
#define MTD_FLOAT_PRECISION_MINIMUM (0)    // to fix BUG-12597
#define MTD_FLOAT_PRECISION_MAXIMUM (38)
#define MTD_FLOAT_MANTISSA_MAXIMUM  MTD_FLOAT_MANTISSA( MTD_FLOAT_PRECISION_MAXIMUM )
#define MTD_FLOAT_MANTISSA( p )     ( ( (p) + 2 ) / 2 )
#define MTD_FLOAT_SIZE( p )         ( 3 + ( ( p ) + 2 ) / 2 )// to fix BUG-12970
#define MTD_FLOAT_SIZE_MAXIMUM      ( MTD_FLOAT_SIZE(                           \
                                          MTD_FLOAT_PRECISION_MAXIMUM ) )

#define MTD_NUMERIC_IS_POSITIVE( n ) ((((n)->signExponent & 0x80) == 0x80)?ID_TRUE:ID_FALSE)
#define MTD_NUMERIC_SIGN_EXPONENT_MAXIMUM   (127) // to fix BUG-39971

typedef SInt mtdIntegerType;
# define MTD_INTEGER_NULL    ((mtdIntegerType)0x80000000)
# define MTD_INTEGER_MAXIMUM ((mtdIntegerType)0x7FFFFFFF)
# define MTD_INTEGER_MINIMUM ((mtdIntegerType)0x80000001)

typedef SShort mtdSmallintType;
# define MTD_SMALLINT_NULL    ((mtdSmallintType)0x8000)
# define MTD_SMALLINT_MAXIMUM ((mtdSmallintType)0x7FFF)
# define MTD_SMALLINT_MINIMUM ((mtdSmallintType)0x8001)

typedef SLong mtdBigintType;
# define MTD_BIGINT_NULL    ((mtdBigintType)ID_LONG(0x8000000000000000))
# define MTD_BIGINT_MAXIMUM ((mtdBigintType)ID_LONG(0x7FFFFFFFFFFFFFFF))
# define MTD_BIGINT_MINIMUM ((mtdBigintType)ID_LONG(0x8000000000000001))

typedef SFloat mtdRealType;
# define MTD_REAL_EXPONENT_MASK (0x7F800000)
# define MTD_REAL_FRACTION_MASK (0x007FFFFF)
# define MTD_REAL_MINIMUM       (1.175494351E-37F)

extern const UInt mtdRealNull; /* PROJ-2632 */

typedef SDouble mtdDoubleType;
# define MTD_DOUBLE_EXPONENT_MASK  ID_ULONG(0x7FF0000000000000)
# define MTD_DOUBLE_FRACTION_MASK  ID_ULONG(0x000FFFFFFFFFFFFF)
# define MTD_DOUBLE_PLUS_INFINITE  ID_ULONG(0x7FF8000000000000)
# define MTD_DOUBLE_MINUS_INFINITE ID_ULONG(0xFFF8000000000000)
# define MTD_DOUBLE_MINIMUM        (2.2250738585072014E-307)

extern const ULong mtdDoubleNull; /* PROJ-2632 */

typedef struct mtdIntervalType {
    SLong second;
    SLong microsecond;
} mtdIntervalType;

extern const mtdIntervalType mtdIntervalNull;
# define MTD_INTERVAL_NULL (mtdIntervalNull)

#define MTD_INTERVAL_IS_NULL( i )                      \
    ( (i)->second      == mtdIntervalNull.second    && \
      (i)->microsecond == mtdIntervalNull.microsecond  )

typedef struct mtdDateType
{
    SShort year;
    UShort mon_day_hour;
    UInt   min_sec_mic;
} mtdDateType;

extern const mtdDateType mtdDateNull; /* PROJ-2632 */

# define MTD_DATE_MON_SHIFT    (10)
# define MTD_DATE_DAY_SHIFT    (5)
# define MTD_DATE_MIN_SHIFT    (26)
# define MTD_DATE_SEC_SHIFT    (20)

# define MTD_DATE_MON_MASK     (0x3C00)      /*  4bits */
# define MTD_DATE_DAY_MASK     (0x03E0)      /*  5bits */
# define MTD_DATE_HOUR_MASK    (0x001F)      /*  5bits */
# define MTD_DATE_MIN_MASK     (0xFC000000)  /*  6bits */
# define MTD_DATE_SEC_MASK     (0x03F00000)  /*  6bits */
# define MTD_DATE_MSEC_MASK    (0x000FFFFF)  /* 20bits */

#define MTD_DATE_IS_NULL( d ) ( (d)->year          == mtdDateNull.year         &&       \
                                (d)->mon_day_hour  == mtdDateNull.mon_day_hour &&       \
                                (d)->min_sec_mic   == mtdDateNull.min_sec_mic   )

typedef enum mtdTypesMTDNumber
{
    MTD_NUMBER_FORMAT_FM = 0,
    MTD_NUMBER_FORMAT_COMMA,
    MTD_NUMBER_FORMAT_PERIOD,
    MTD_NUMBER_FORMAT_DOLLAR,
    MTD_NUMBER_FORMAT_ZERO,
    MTD_NUMBER_FORMAT_NINE,
    MTD_NUMBER_FORMAT_B,
    MTD_NUMBER_FORMAT_EEEE,
    MTD_NUMBER_FORMAT_MI,
    MTD_NUMBER_FORMAT_PR,
    MTD_NUMBER_FORMAT_RN,
    MTD_NUMBER_FORMAT_S,
    MTD_NUMBER_FORMAT_V,
    MTD_NUMBER_FORMAT_XXXX,
    MTD_NUMBER_FORMAT_C,
    MTD_NUMBER_FORMAT_L,
    MTD_NUMBER_FORMAT_G,
    MTD_NUMBER_FORMAT_D,
    MTD_COUNT_NINE,
    MTD_COUNT_ZERO,
    MTD_COUNT_FLOAT_NINE,
    MTD_COUNT_FLOAT_ZERO,
    MTD_COUNT_VNINE_ZERO,
    MTD_COUNT_FIRST_S,
    MTD_COUNT_FIRST_PERIOD,
    MTD_COUNT_INTEGER,
    MTD_COUNT_FLOAT,
    MTD_COUNT_INVALID_FLOAT,
    MTD_INDEX_STRING,
    MTD_INDEX_STRING_LEN,
    MTD_INDEX_FORMAT,
    MTD_INDEX_FORMAT_LEN,
    MTD_INDEX_RESULT,
    MTD_INDEX_RESULT_LEN,
    MTD_NUMBER_MAX
} mtdTypesMTDNumber;

#define MTD_NUMBER_FORMAT_BUFFER_LEN    (70)

// BUG-18788
// format에 대한 정보가 수정되는 경우, mtfTo_char.cpp의
// gFormatFuncSet 또한 적절하게 수정되어야 함
# define MTD_DATE_FORMAT_NONE                 (1)    // general character
# define MTD_DATE_FORMAT_AM_U                 (2)    // AM
# define MTD_DATE_FORMAT_AM_UL                (3)    // Am
# define MTD_DATE_FORMAT_AM_L                 (4)    // aM, am
# define MTD_DATE_FORMAT_CC                   (5)    // CC
# define MTD_DATE_FORMAT_DAY_U                (6)    // DAY
# define MTD_DATE_FORMAT_DAY_UL               (7)    // Day
# define MTD_DATE_FORMAT_DAY_L                (8)    // day
# define MTD_DATE_FORMAT_DDD                  (9)    // DDD
# define MTD_DATE_FORMAT_DD                   (10)   // DD
# define MTD_DATE_FORMAT_DY_U                 (11)   // DY
# define MTD_DATE_FORMAT_DY_UL                (12)   // Dy
# define MTD_DATE_FORMAT_DY_L                 (13)   // dy
# define MTD_DATE_FORMAT_D                    (14)   // D
# define MTD_DATE_FORMAT_FF                   (15)   // FF = FF6
# define MTD_DATE_FORMAT_FF1                  (16)   // FF1
# define MTD_DATE_FORMAT_FF2                  (17)   // FF2
# define MTD_DATE_FORMAT_FF3                  (18)   // FF3
# define MTD_DATE_FORMAT_FF4                  (19)   // FF4
# define MTD_DATE_FORMAT_FF5                  (20)   // FF5
# define MTD_DATE_FORMAT_FF6                  (21)   // FF6
# define MTD_DATE_FORMAT_HH                   (22)   // HH
# define MTD_DATE_FORMAT_HH12                 (23)   // HH12
# define MTD_DATE_FORMAT_HH24                 (24)   // HH24
# define MTD_DATE_FORMAT_MI                   (25)   // MI
# define MTD_DATE_FORMAT_MM                   (26)   // MM
# define MTD_DATE_FORMAT_MON_U                (27)   // MON
# define MTD_DATE_FORMAT_MON_UL               (28)   // Mon
# define MTD_DATE_FORMAT_MON_L                (29)   // mon
# define MTD_DATE_FORMAT_MONTH_U              (30)   // MONTH
# define MTD_DATE_FORMAT_MONTH_UL             (31)   // Month
# define MTD_DATE_FORMAT_MONTH_L              (32)   // month
# define MTD_DATE_FORMAT_PM_U                 (33)   // PM
# define MTD_DATE_FORMAT_PM_UL                (34)   // Pm
# define MTD_DATE_FORMAT_PM_L                 (35)   // pM, pm
# define MTD_DATE_FORMAT_Q                    (36)   // Q
# define MTD_DATE_FORMAT_RM_U                 (37)   // RM, Rm
# define MTD_DATE_FORMAT_RM_L                 (38)   // rM, rm
# define MTD_DATE_FORMAT_RRRR                 (39)   // RRRR
# define MTD_DATE_FORMAT_RR                   (40)   // RR
# define MTD_DATE_FORMAT_SSSSSSSS             (41)   // SSSSSSSS
# define MTD_DATE_FORMAT_SSSSSS               (42)   // SSSSSS
# define MTD_DATE_FORMAT_SSSSS                (43)   // SSSSS
# define MTD_DATE_FORMAT_SS                   (44)   // SS
# define MTD_DATE_FORMAT_WW                   (45)   // WW
# define MTD_DATE_FORMAT_W                    (46)   // W
# define MTD_DATE_FORMAT_YCYYY                (47)   // Y,YYY
# define MTD_DATE_FORMAT_YYYY                 (48)   // YYYY
# define MTD_DATE_FORMAT_YYY                  (49)   // YYY
# define MTD_DATE_FORMAT_YY                   (50)   // YY
# define MTD_DATE_FORMAT_Y                    (51)   // Y
# define MTD_DATE_FORMAT_DOUBLE_QUOTE_STRING  (52)   // "****"
# define MTD_DATE_FORMAT_FM                   (53)   // FM, To fix BUG-17693
# define MTD_DATE_FORMAT_SEPARATOR            (54)   // 구분자
# define MTD_DATE_FORMAT_IW                   (55)   /* BUG-42926 TO_CHAR()에 IW 추가 */
# define MTD_DATE_FORMAT_WW2                  (56)   /* BUG-42941 TO_CHAR()에 WW2(Oracle Version WW) 추가 */
# define MTD_DATE_FORMAT_SYYYY                (57)   /* BUG-36296 SYYYY Format 지원 */
# define MTD_DATE_FORMAT_SCC                  (58)   /* BUG-36296 SCC Format 지원 */
# define MTD_DATE_FORMAT_IYYY                 (59)   /* BUG-46727 TO_CHAR()에 IYYY 추가 */
# define MTD_DATE_FORMAT_IYY                  (60)   /* BUG-46727 TO_CHAR()에 IYY 추가 */
# define MTD_DATE_FORMAT_IY                   (61)   /* BUG-46727 TO_CHAR()에 IY 추가 */
# define MTD_DATE_FORMAT_I                    (62)   /* BUG-46727 TO_CHAR()에 I 추가 */

# define MTD_DATE_GREGORY_ONLY       (0)
# define MTD_DATE_GREGORY_AND_JULIAN (1)
# define MTD_DATE_JULIAN_ONLY        (2)

/* '9999-12-31 23:59:59' - '0001-01-01 00:00:00' */
# define MTD_DATE_MAX_YEAR_PER_SECOND   ( ID_LONG(315538070399) )

/* '-9999-01-01 00:00:00' - '0001-01-01 00:00:00' */
# define MTD_DATE_MIN_YEAR_PER_SECOND   ( ID_LONG(-315576000000) )

typedef IDE_RC (*mtfFormatModuleFunc)( mtdDateType* aDate,
                                     SChar*       aBuffer,
                                     SInt*        aBufferCur,
                                     SInt*        aBufferFence,
                                     SChar*       aString,
                                     idBool       aIsFillMode );

typedef struct mtfTo_charCalcInfo {
    mtfFormatModuleFunc   applyDateFormat;
    SChar*                string;
    idBool                isFillMode; // To fix BUG-17693
} mtfTo_charCalcInfo;

typedef struct mtdFormatInfo {
    UShort               count;
    mtfTo_charCalcInfo*  format;
} mtdFormatInfo;

/* PROJ-2207 Password policy support */
typedef enum
{
    MTD_DATE_DIFF_CENTURY = 0,
    MTD_DATE_DIFF_YEAR,
    MTD_DATE_DIFF_QUARTER,
    MTD_DATE_DIFF_MONTH,
    MTD_DATE_DIFF_WEEK,
    MTD_DATE_DIFF_DAY,
    MTD_DATE_DIFF_HOUR,
    MTD_DATE_DIFF_MINUTE,
    MTD_DATE_DIFF_SECOND,
    MTD_DATE_DIFF_MICROSEC
} mtdDateField;

class mtdDateInterface
{
private:
    // toDate의 input string에 대한 symbol table
    static const UChar mInputST[256];
    // symbole table을 구성하는 값
    static const UChar mNONE;        // 정의되지 않은 심볼
    static const UChar mDIGIT;       // 숫자 0~9
    static const UChar mALPHA;       // 알파벳
    static const UChar mSEPAR;       // 순수 구분자 7개 - / , . : ; '
    static const UChar mWHSP;        // 구분자중, 공백, 탭, 줄바꿈

    static const UChar mDaysOfMonth[2][13];         // 월별 날 수
    static const UInt  mAccDaysOfMonth[2][13];      // 월별 누적 날 수

    static const UInt  mHashMonth[12];           // 문자열 해시 값 저장

    /* toDate()에서 필요로 하는 함수 */
    static IDE_RC toDateGetInteger( UChar** aString,
                                    UInt*   aLength,
                                    UInt    aMax,
                                    UInt*   aValue );

    static IDE_RC toDateGetMonth( UChar** aString,
                                  UInt*   aLength,
                                  UInt*   aMonth );

    static IDE_RC toDateGetRMMonth( UChar** aString,
                                    UInt*   aLength,
                                    UInt*   aMonth );

public:
    static inline SShort year(mtdDateType* aDate)
    {
        return aDate->year;
    }

    static inline UChar month(mtdDateType* aDate)
    {
        return (UChar)((aDate->mon_day_hour
                        & MTD_DATE_MON_MASK)
                       >> MTD_DATE_MON_SHIFT);
    }

    static inline UChar day(mtdDateType* aDate)
    {
        return (UChar)((aDate->mon_day_hour
                        & MTD_DATE_DAY_MASK)
                       >> MTD_DATE_DAY_SHIFT);
    }

    static inline UChar hour(mtdDateType* aDate)
    {
        return (UChar)(aDate->mon_day_hour
                       & MTD_DATE_HOUR_MASK);
    }

    static inline UChar minute(mtdDateType* aDate)
    {
        return (UChar)((aDate->min_sec_mic
                        & MTD_DATE_MIN_MASK)
                       >> MTD_DATE_MIN_SHIFT);
    }

    static inline UChar second(mtdDateType* aDate)
    {
        return (UChar)((aDate->min_sec_mic
                        & MTD_DATE_SEC_MASK)
                       >> MTD_DATE_SEC_SHIFT);
    }

    static inline UInt microSecond(mtdDateType* aDate)
    {
        return (UInt)(aDate->min_sec_mic
                      & MTD_DATE_MSEC_MASK);
    }

    static inline SInt compare(mtdDateType* aDate1, mtdDateType* aDate2)
    {
        if( aDate1->year > aDate2->year )
            return 1;
        if( aDate1->year < aDate2->year )
            return -1;

        if( aDate1->mon_day_hour > aDate2->mon_day_hour )
            return 1;
        if( aDate1->mon_day_hour < aDate2->mon_day_hour )
            return -1;

        if( aDate1->min_sec_mic > aDate2->min_sec_mic )
            return 1;
        if( aDate1->min_sec_mic < aDate2->min_sec_mic )
            return -1;

        return 0;
    }

    static inline idBool isLeapYear( SShort aYear )
    {
        // BUG-22710
        // 4로 나누어떨어지지만 100으로 나누어떨어지면 안되고
        // 혹은 400으로 나누어떨어지지만 4000으로 나누어떨어지지지
        // 않으면 윤년이다.
        /* BUG-36296 그레고리력의 윤년 규칙은 1583년부터 적용한다. 1582년 이전에는 4년마다 윤년이다. */
        if ( ( ( aYear % 4 ) == 0 ) &&
             ( ( aYear < 1583 ) || ( ( aYear % 100 ) != 0 ) || ( ( aYear % 400 ) == 0 ) ) )
        {
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }

    static IDE_RC setYear(mtdDateType* aDate, SShort aYear);

    static IDE_RC setMonth(mtdDateType* aDate, UChar aMonth);

    static IDE_RC setDay(mtdDateType* aDate, UChar aDay);

    static IDE_RC setHour(mtdDateType* aDate, UChar aHour);

    static IDE_RC setMinute(mtdDateType* aDate, UChar aMinute);

    static IDE_RC setSecond(mtdDateType* aDate, UChar aSec);

    static IDE_RC setMicroSecond(mtdDateType* aDate, UInt aMicroSec);

    static IDE_RC makeDate(mtdDateType* aDate,
                           SShort       aYear,
                           UChar        aMonth,
                           UChar        aDay,
                           UChar        aHour,
                           UChar        aMinute,
                           UChar        aSec,
                           UInt         aMicroSec);

    static IDE_RC addMonth( mtdDateType* aResult,
                            mtdDateType* aDate,
                            SLong        aNumber );

    static IDE_RC addDay( mtdDateType* aResult,
                          mtdDateType* aDate,
                          SLong        aNumber );

    static IDE_RC addSecond( mtdDateType* aResult,
                             mtdDateType* aDate,
                             SLong        aSecond,
                             SLong        aMicroSecond );
    
    static IDE_RC addMicroSecond( mtdDateType* aResult,
                                  mtdDateType* aDate,
                                  SLong        aNumber );

    static IDE_RC convertDate2Interval(mtdDateType*     aDate,
                                       mtdIntervalType* aInterval);

    static IDE_RC convertInterval2Date(mtdIntervalType* aInterval,
                                       mtdDateType*     aDate);

    static IDE_RC toDate(mtdDateType* aDate,
                         UChar*       aString,
                         UInt         aStringLen,
                         UChar*       aFormat,
                         UInt         aFormatLen);

    // date format 정보를 모르는 경우
    static IDE_RC toChar(mtdDateType* aDate,
                         UChar*       aString,
                         UInt*        aStringLen,
                         SInt         aStringMaxLen,
                         UChar*       aFormat,
                         UInt         aFormatLen );

    static IDE_RC toDateInternal(mtdDateType * aDate,
                                 UChar       * aString,
                                 UInt          aStringLen,
                                 UChar       * aFormat,
                                 UInt          aFormatLen,
                                 void        * aScanner );

    // date format 정보를 모르는 경우
    static IDE_RC toCharInternal(mtdDateType * aDate,
                                 UChar       * aString,
                                 UInt        * aStringLen,
                                 SInt          aStringMaxLen,
                                 UChar       * aFormat,
                                 UInt          aFormatLen,
                                 void        * aScanner );

    static IDE_RC getMaxCharLength( UChar*       aFormat,
				    UInt         aFormatLen,
				    UInt*        aLength);

    static IDE_RC checkYearMonthDayAndSetDateValue( mtdDateType  * aDate,
                                                    SShort         aYear,
                                                    UChar          aMonth,
                                                    UChar          aDay );

    /* PROJ-2207 Password policy support */
    static IDE_RC dateDiff( mtdBigintType * aResult,
                            mtdDateType   * aStartDate,
                            mtdDateType   * aEndDate,
                            mtdDateField    aExtractSet );
};


typedef UChar mtdBooleanType;
# define MTD_BOOLEAN_TRUE  ((mtdBooleanType)0x00)
# define MTD_BOOLEAN_FALSE ((mtdBooleanType)0x01)
# define MTD_BOOLEAN_NULL  ((mtdBooleanType)0x02)

typedef struct mtsFileType
{
    FILE*     fp;
    SChar     mode[2];
} mtsFileType;

// BUG-40854
typedef struct mtsConnectType
{
    SLong      connectionNodeKey;
} mtsConnectType;

typedef struct mtdBitType {
    UInt  length;
    UChar value[1];
} mtdBitType;

extern const mtdBitType mtdBitNull; /* PROJ-2632 */

#define MTD_BIT_ALIGN             (ID_SIZEOF(UInt))
#define MTD_BIT_PRECISION_DEFAULT (1)
#define MTD_BIT_PRECISION_MINIMUM (MTD_CHAR_PRECISION_MINIMUM)  // to fix BUG-12597
#define MTD_BIT_PRECISION_MAXIMUM (2*MTD_CHAR_PRECISION_MAXIMUM)// to fix BUG-12597

#define MTD_VARBIT_ALIGN             MTD_BIT_ALIGN
#define MTD_VARBIT_PRECISION_DEFAULT MTD_BIT_PRECISION_DEFAULT
#define MTD_VARBIT_PRECISION_MINIMUM MTD_BIT_PRECISION_MINIMUM
#define MTD_VARBIT_PRECISION_MAXIMUM MTD_BIT_PRECISION_MAXIMUM

#define BIT_TO_BYTE(n)            ( ((n) + 7) >> 3 )

#define MTD_BIT_TYPE_STRUCT_SIZE( valueLength )         \
    ( ID_SIZEOF( UInt ) + BIT_TO_BYTE(valueLength) )

#define MTD_BIT_STORE_PRECISION_MAXIMUM (64000)     // BUG-28921

// PROJ-1583, PR-15722
// SQL_BINARY에 대응하는 mtdBinaryType
// PROJ-1583 large geometry 
// mLength의 타입을 UInt로 변경
typedef struct mtdBinaryType
{
    UInt   mLength;
    UChar  mPadding[4];
    UChar  mValue[1];
} mtdBinaryType;

#define MTD_BINARY_TYPE_STRUCT_SIZE( valueLength )      \
    ( ID_SIZEOF( UInt ) + 4 + (valueLength) )

#define MTD_CLOB_PRECISION_MINIMUM (0) // To Fix BUG-12597

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
#define MTD_CLOB_PRECISION_MAXIMUM (104857600)

// PROJ-1362
typedef struct mtdLobType {
    SLong length;
    UChar value[1];
} mtdLobType;

#define MTD_LOB_NULL_LENGTH  (-1)
#define MTD_LOB_IS_NULL( lobLength )  \
    ( lobLength == MTD_LOB_NULL_LENGTH ? ID_TRUE : ID_FALSE )

#define MTD_LOB_EMPTY_LENGTH (0)
#define MTD_LOB_ALIGN        ( (SInt) ID_SIZEOF(SLong) )

#define MTD_LOB_TYPE_STRUCT_SIZE( valueLength ) \
    ( ID_SIZEOF( SLong ) + (valueLength) )

typedef mtdLobType mtdBlobType;
#define MTD_BLOB_TYPE_STRUCT_SIZE MTD_LOB_TYPE_STRUCT_SIZE

typedef mtdLobType mtdClobType;
#define MTD_CLOB_TYPE_STRUCT_SIZE MTD_LOB_TYPE_STRUCT_SIZE

typedef smLobLocator mtdBlobLocatorType;
#define MTD_LOCATOR_NULL       ((mtdBlobLocatorType)(smLobLocator) 0)
#define MTD_BLOB_LOCATOR_NULL  MTD_LOCATOR_NULL

typedef smLobLocator mtdClobLocatorType;
#define MTD_CLOB_LOCATOR_NULL  MTD_LOCATOR_NULL

typedef struct mtdByteType {
    UShort length;
    UChar  value[1];
} mtdByteType;

extern const mtdByteType mtdByteNull;

#define MTD_BYTE_ALIGN             ( (SInt) ID_SIZEOF(UShort) )
#define MTD_BYTE_PRECISION_DEFAULT (1)
#define MTD_BYTE_PRECISION_MINIMUM (0) // To Fix BUG-12597
#define MTD_BYTE_PRECISION_MAXIMUM ( (SInt) (65536 - ID_SIZEOF(UShort)) )

#define MTD_VARBYTE_ALIGN             MTD_BYTE_ALIGN
#define MTD_VARBYTE_PRECISION_DEFAULT MTD_BYTE_PRECISION_DEFAULT
#define MTD_VARBYTE_PRECISION_MINIMUM MTD_BYTE_PRECISION_MINIMUM
#define MTD_VARBYTE_PRECISION_MAXIMUM MTD_BYTE_PRECISION_MAXIMUM

#define MTD_BYTE_TYPE_STRUCT_SIZE( valueLength )        \
    ( ID_SIZEOF( UShort ) + (valueLength) )

#define MTD_BYTE_STORE_PRECISION_MAXIMUM (32000)     // BUG-28921

#define MTD_PIE 3.1415926535897932384626
#define MTD_COMPILEDFMT_MAXLEN (256)

typedef struct mtdNibbleType {
    UChar length;
    UChar value[1];
} mtdNibbleType;

extern const mtdNibbleType mtdNibbleNull; /* PROJ-2632 */

#define MTD_NIBBLE_PRECISION_DEFAULT (1)
#define MTD_NIBBLE_PRECISION_MINIMUM (0)  // to fix BUG-12597
#define MTD_NIBBLE_PRECISION_MAXIMUM (254)// to fix BUG-12597
#define MTD_NIBBLE_NULL_LENGTH       (255)

#define MTD_NIBBLE_TYPE_STRUCT_SIZE( valueLength )  \
    ( ID_SIZEOF( UChar ) + ((valueLength+1)/2) )


// --------------------------------
// PROJ-1579 NCHAR
// --------------------------------

#define MTD_NCHAR_ALIGN                     (ID_SIZEOF(UShort))
#define MTD_NCHAR_PRECISION_MINIMUM         (0)
#define MTD_NCHAR_PRECISION_DEFAULT         (1)
#define MTD_NVARCHAR_PRECISION_DEFAULT      (1)

// BUG-25914 : UTF8의 한 글자의 크기는 3Byte임.
// MTD_NCHAR_PRECISION_MAXIMUM / MTL_UTF8_PRECISION
#define MTD_UTF8_NCHAR_PRECISION_MAXIMUM    ( (65536/3) - ID_SIZEOF(UShort) )
// MTD_NCHAR_PRECISION_MAXIMUM / MTL_UTF16_PRECISION
#define MTD_UTF16_NCHAR_PRECISION_MAXIMUM   ( (65536/2) - ID_SIZEOF(UShort) )

#define MTD_NCHAR_PRECISION_MINIMUM (0)    // to fix BUG-12597
#define MTD_NCHAR_PRECISION_MAXIMUM (65536 - ID_SIZEOF(UShort) )

// CHAR_PRECISION_MAXIMUM과 SYNC를 맞추어야 함
// MTD_NCHAR_PRECISION_MAXIMUM / MTL_UTF8_PRECISION
#define MTD_UTF8_NCHAR_STORE_PRECISION_MAXIMUM    (10666)

// MTD_NCHAR_PRECISION_MAXIMUM / MTL_UTF16_PRECISION
#define MTD_UTF16_NCHAR_STORE_PRECISION_MAXIMUM   (16000)


typedef struct mtdNcharType {
    UShort length;      // 바이트수
    UChar  value[1];
} mtdNcharType;

extern const mtdNcharType mtdNcharNull;

// PROJ-1579 NCHAR
class mtdNcharInterface
{
public:
    // CHAR => NCHAR
    static IDE_RC toNchar( mtcStack         * aStack,
                           const mtlModule  * aSrcCharSet,
                           const mtlModule  * aDestCharSet,
                           mtdCharType      * aSource,
                           mtdNcharType     * aResult );

    // CHAR => NCHAR
    static IDE_RC toNchar( mtcStack         * aStack,
                           const mtlModule  * aSrcCharSet,
                           const mtlModule  * aDestCharSet,
                           UChar            * aSource,
                           UShort             aSourceLen,
                           mtdNcharType     * aResult );

    // NCHAR => CHAR
    static IDE_RC toChar( mtcStack         * aStack,
                          const mtlModule  * aSrcCharSet,
                          const mtlModule  * aDestCharSet,
                          mtdNcharType     * aSource,
                          mtdCharType      * aResult );

    // NCHAR => CHAR
    static IDE_RC toChar( mtcStack         * aStack,
                          const mtlModule  * aSrcCharSet,
                          const mtlModule  * aDestCharSet,
                          mtdNcharType     * aSource,
                          UChar            * aResult,
                          UShort           * aResultLen );

    /* PROJ-2632 */
    static IDE_RC toNchar( SInt               aPrecision,
                           const mtlModule  * aSrcCharSet,
                           const mtlModule  * aDestCharSet,
                           mtdCharType      * aSource,
                           mtdNcharType     * aResult );

    static IDE_RC toChar( SInt               aPrecision,
                          const mtlModule  * aSrcCharSet,
                          const mtlModule  * aDestCharSet,
                          mtdNcharType     * aSource,
                          mtdCharType      * aResult );

    static IDE_RC toNchar4UnicodeLiteral(
            const mtlModule * aSourceCharSet,
            const mtlModule * aResultCharSet,
            UChar           * aSourceIndex,
            UChar           * aSourceFence,
            UChar          ** aResultValue,
            UChar           * aResultFence,
            UInt            * aNcharCnt );

};

/***********************************************************************
 * PROJ-2002 COLUMN SECURITY 
 **********************************************************************/

// ECHAR
typedef struct mtdEcharType 
{
    UShort mCipherLength;
    UShort mEccLength;
    UChar  mValue[1];         // cipher value + ecc value
} mtdEcharType;

#define MTD_ECHAR_TYPE_STRUCT_SIZE( valueLength )        \
    ( ID_SIZEOF( UShort ) + ID_SIZEOF( UShort ) + (valueLength) )

extern const mtdEcharType mtdEcharNull;

#define MTD_ECHAR_ALIGN                    (ID_SIZEOF(UShort))
#define MTD_ECHAR_PRECISION_MINIMUM        (0)
#define MTD_ECHAR_PRECISION_DEFAULT        (1)

#define MTD_ECHAR_PRECISION_MINIMUM        (0)
#define MTD_ECHAR_PRECISION_MAXIMUM        ((SInt)(65536 - ID_SIZEOF(UShort) \
                                                   - ID_SIZEOF(UShort)))

#define MTD_EVARCHAR_PRECISION_MINIMUM     (MTD_ECHAR_PRECISION_MINIMUM)
#define MTD_EVARCHAR_PRECISION_MAXIMUM     (MTD_ECHAR_PRECISION_MAXIMUM)

#define MTD_ECHAR_STORE_PRECISION_MAXIMUM  (10000)

// decrypt도 encrypt와 마찬가지로 block단위로 복호화를 수행하므로
// block 크기만큼의 공간을 더 사용하여 복호화를 수행한다.
// 그러므로 decrypt시 별도의 decrypt buffer를 사용해야 한다.
// 또한, salt(initial vector)를 사용하는 경우 block size에
// salt size를 추가로 고려해야 한다.
//
// 32byte block size + 32byte salt size -> 64 byte
//
#define MTD_ECHAR_DECRYPT_BLOCK_SIZE       (64)
#define MTD_ECHAR_DECRYPT_BUFFER_SIZE      (MTD_ECHAR_STORE_PRECISION_MAXIMUM \
                                            + MTD_ECHAR_DECRYPT_BLOCK_SIZE)

// PROJ-2163 바인드 재정립 - mtdUndef 추가
typedef UChar mtdUndefType;

#endif /* _O_MTD_TYPES_H_ */
