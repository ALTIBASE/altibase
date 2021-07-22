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
 * $Id: mtdTypes.h 37587 2010-01-06 06:11:10Z msg2me $
 **********************************************************************/

#ifndef _O_MTCD_TYPES_H_
# define _O_MTCD_TYPES_H_ 1

#include <mtccDef.h>
#include <mtcdTypeDef.h>

ACP_EXTERN_C_BEGIN

typedef struct mtdCharType {
    acp_uint16_t length;
    acp_uint8_t  value[1];
} mtdCharType;

typedef acp_uint64_t mtdLobLocator;

#define MTD_CHAR_ALIGN             (sizeof(acp_uint16_t))
#define MTD_CHAR_PRECISION_DEFAULT (1) // To Fix BUG-12597

#define MTD_CHAR_TYPE_STRUCT_SIZE( valueLength )    \
    ( sizeof( acp_uint16_t ) + (valueLength) )

extern const mtdCharType mtcdCharNull;

#define MTD_CHAR_PRECISION_MINIMUM (0)    // to fix BUG-12597
// BUG-19925 : 모든 데이터타입의 (GEOMETRY제외) 최대 크기를
// constant tuple의 최대 크기인 65536으로 제한함
#define MTD_CHAR_PRECISION_MAXIMUM ((acp_sint32_t)(65536 - sizeof(acp_uint16_t)))

// fix BUG-22220
#define MTD_VARCHAR_PRECISION_MINIMUM (MTD_CHAR_PRECISION_MINIMUM)
#define MTD_VARCHAR_PRECISION_MAXIMUM (MTD_CHAR_PRECISION_MAXIMUM)

#define MTD_CHAR_STORE_PRECISION_MAXIMUM (32000)

typedef struct mtdNumericType {
    acp_uint8_t length;
    acp_uint8_t signExponent;
    acp_uint8_t mantissa[1];
} mtdNumericType;

extern const mtdNumericType mtcdNumericNull;

#define MTD_NUMERIC_ALIGN             (sizeof(acp_uint8_t))
#define MTD_NUMERIC_PRECISION_DEFAULT (38)
#define MTD_NUMERIC_PRECISION_MINIMUM (0)    // to fix BUG-12597
#define MTD_NUMERIC_PRECISION_MAXIMUM (38)
#define MTD_NUMERIC_SCALE_DEFAULT     (0)
#define MTD_NUMERIC_SCALE_MINIMUM     (-84)
#define MTD_NUMERIC_SCALE_MAXIMUM     (128)
#define MTD_NUMERIC_MANTISSA_MAXIMUM  MTD_NUMERIC_MANTISSA( MTD_NUMERIC_PRECISION_MAXIMUM )
#define MTD_NUMERIC_MANTISSA( p )     ( ( (p) + 2 ) / 2 )
#define MTD_NUMERIC_SIZE( p, s )      ( 3 + ( ( p ) + 2 ) / 2 ) // to fix BUG-12944, BUG-12970
#define MTD_NUMERIC_SIZE_MAXIMUM      ( MTD_NUMERIC_SIZE(               \
                                            MTD_NUMERIC_PRECISION_MAXIMUM, 1 ) )
#define MTD_FLOAT_ALIGN             (sizeof(acp_uint8_t))
#define MTD_FLOAT_PRECISION_DEFAULT (38)
#define MTD_FLOAT_PRECISION_MINIMUM (0)    // to fix BUG-12597
#define MTD_FLOAT_PRECISION_MAXIMUM (38)
#define MTD_FLOAT_MANTISSA_MAXIMUM  MTD_FLOAT_MANTISSA( MTD_FLOAT_PRECISION_MAXIMUM )
#define MTD_FLOAT_MANTISSA( p )     ( ( (p) + 2 ) / 2 )
#define MTD_FLOAT_SIZE( p )         ( 3 + ( ( p ) + 2 ) / 2 )// to fix BUG-12970
#define MTD_FLOAT_SIZE_MAXIMUM      ( MTD_FLOAT_SIZE(                   \
                                          MTD_FLOAT_PRECISION_MAXIMUM ) )

typedef acp_sint32_t mtdIntegerType;
# define MTD_INTEGER_NULL    ((mtdIntegerType)0x80000000)
# define MTD_INTEGER_MAXIMUM ((mtdIntegerType)0x7FFFFFFF)
# define MTD_INTEGER_MINIMUM ((mtdIntegerType)0x80000001)

typedef acp_sint16_t mtdSmallintType;
# define MTD_SMALLINT_NULL    ((mtdSmallintType)0x8000)
# define MTD_SMALLINT_MAXIMUM ((mtdSmallintType)0x7FFF)
# define MTD_SMALLINT_MINIMUM ((mtdSmallintType)0x8001)

typedef acp_sint64_t mtdBigintType;
# define MTD_BIGINT_NULL    ((mtdBigintType)ACP_SINT64_LITERAL(0x8000000000000000))
# define MTD_BIGINT_MAXIMUM ((mtdBigintType)ACP_SINT64_LITERAL(0x7FFFFFFFFFFFFFFF))
# define MTD_BIGINT_MINIMUM ((mtdBigintType)ACP_SINT64_LITERAL(0x8000000000000001))

typedef acp_float_t mtdRealType;

extern const acp_uint32_t mtcdRealNull;

# define MTD_REAL_EXPONENT_MASK (0x7F800000)
# define MTD_REAL_FRACTION_MASK (0x007FFFFF)
# define MTD_REAL_MINIMUM       (1.175494351E-37F)

typedef acp_double_t mtdDoubleType;
# define MTD_DOUBLE_EXPONENT_MASK  ACP_UINT64_LITERAL(0x7FF0000000000000)
# define MTD_DOUBLE_FRACTION_MASK  ACP_UINT64_LITERAL(0x000FFFFFFFFFFFFF)
# define MTD_DOUBLE_PLUS_INFINITE  ACP_UINT64_LITERAL(0x7FF8000000000000)
# define MTD_DOUBLE_MINUS_INFINITE ACP_UINT64_LITERAL(0xFFF8000000000000)
# define MTD_DOUBLE_MINIMUM        (2.2250738585072014E-307)

extern acp_uint64_t mtcdDoubleNull;

typedef struct mtdIntervalType {
    acp_sint64_t second;
    acp_sint64_t microsecond;
} mtdIntervalType;

extern const mtdIntervalType mtcdIntervalNull;
# define MTD_INTERVAL_NULL (mtcdIntervalNull)

#define MTD_INTERVAL_IS_NULL( i )                           \
    ( (i)->second      == mtcdIntervalNull.second    &&      \
      (i)->microsecond == mtcdIntervalNull.microsecond  )

typedef struct mtdDateType
{
    acp_sint16_t year;
    acp_uint16_t mon_day_hour;
    acp_uint32_t min_sec_mic;
} mtdDateType;

extern mtdDateType mtcdDateNull;

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

#define MTD_DATE_IS_NULL( d ) ( (d)->year          == mtcdDateNull.year         && \
                                (d)->mon_day_hour  == mtcdDateNull.mon_day_hour && \
                                (d)->min_sec_mic   == mtcdDateNull.min_sec_mic   )

# define MTD_NUMBER_FORMAT_FM     (0)
# define MTD_NUMBER_FORMAT_COMMA  (1)
# define MTD_NUMBER_FORMAT_PERIOD (2)
# define MTD_NUMBER_FORMAT_DOLLAR (3)
# define MTD_NUMBER_FORMAT_ZERO   (4)
# define MTD_NUMBER_FORMAT_NINE   (5)
# define MTD_NUMBER_FORMAT_B      (6)
# define MTD_NUMBER_FORMAT_EEEE   (7)
# define MTD_NUMBER_FORMAT_MI     (8)
# define MTD_NUMBER_FORMAT_PR     (9)
# define MTD_NUMBER_FORMAT_RN     (10)
# define MTD_NUMBER_FORMAT_S      (11)
# define MTD_NUMBER_FORMAT_V      (12)
# define MTD_NUMBER_FORMAT_XXXX   (13)
# define MTD_NUMBER_FORMAT_C      (15)
# define MTD_NUMBER_FORMAT_L      (16)
# define MTD_NUMBER_FORMAT_G      (17)
# define MTD_NUMBER_FORMAT_D      (18)
# define MTD_COUNT_NINE           (19)
# define MTD_COUNT_ZERO           (20)
# define MTD_COUNT_FLOAT_NINE     (21)
# define MTD_COUNT_FLOAT_ZERO     (22)
# define MTD_COUNT_VNINE_ZERO     (23)
# define MTD_COUNT_FIRST_S        (24)
# define MTD_COUNT_FIRST_PERIOD   (25)
# define MTD_COUNT_INTEGER        (26)
# define MTD_COUNT_FLOAT          (27)
# define MTD_COUNT_INVALID_FLOAT  (28)
# define MTD_INDEX_STRING         (29)
# define MTD_INDEX_STRING_LEN     (30)
# define MTD_INDEX_FORMAT         (31)
# define MTD_INDEX_FORMAT_LEN     (32)
# define MTD_INDEX_RESULT         (33)
# define MTD_INDEX_RESULT_LEN     (34)
# define MTD_NUMBER_MAX           (35)

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
# define MTD_DATE_MAX_YEAR_PER_SECOND   ( ACP_SINT64_LITERAL(315538070399) )

/* '-9999-01-01 00:00:00' - '0001-01-01 00:00:00' */
# define MTD_DATE_MIN_YEAR_PER_SECOND   ( ACP_SINT64_LITERAL(-315576000000) )

typedef ACI_RC (*mtfFormatModuleFunc)( mtdDateType  * aDate,
                                       acp_char_t   * aBuffer,
                                       acp_sint32_t * aBufferCur,
                                       acp_sint32_t * aBufferFence,
                                       acp_char_t   * aString,
                                       acp_bool_t     aIsFillMode );

typedef struct mtfTo_charCalcInfo {
    mtfFormatModuleFunc   applyDateFormat;
    acp_char_t          * string;
    acp_bool_t            isFillMode; // To fix BUG-17693
} mtfTo_charCalcInfo;

typedef struct mtdFormatInfo {
    acp_uint16_t         count;
    mtfTo_charCalcInfo * format;
} mtdFormatInfo;


/* toDate()에서 필요로 하는 함수 */
ACI_RC mtdDateInterfaceToDateGetInteger( acp_uint8_t  ** aString,
                                         acp_uint32_t *  aLength,
                                         acp_uint32_t    aMax,
                                         acp_uint32_t *  aValue );

ACI_RC mtdDateInterfaceToDateGetMonth( acp_uint8_t  ** aString,
                                       acp_uint32_t *  aLength,
                                       acp_uint32_t *  aMonth );

ACI_RC mtdDateInterfaceToDateGetRMMonth( acp_uint8_t  ** aString,
                                         acp_uint32_t *  aLength,
                                         acp_uint32_t *  aMonth );

ACP_INLINE acp_bool_t mtdDateInterfaceIsLeapYear( acp_sint16_t aYear )
{
    // BUG-22710
    // 4로 나누어떨어지지만 100으로 나누어떨어지면 안되고
    // 혹은 400으로 나누어떨어지지만 4000으로 나누어떨어지지지
    // 않으면 윤년이다.
    /* BUG-36296 그레고리력의 윤년 규칙은 1583년부터 적용한다. 1582년 이전에는 4년마다 윤년이다. */
    if ( ( ( aYear % 4 ) == 0 ) &&
         ( ( aYear < 1583 ) || ( ( aYear % 100 ) != 0 ) || ( ( aYear % 400 ) == 0 ) ) )
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}


ACP_INLINE acp_sint16_t mtdDateInterfaceYear(mtdDateType* aDate)
{
    return aDate->year;
}

ACP_INLINE acp_uint8_t mtdDateInterfaceMonth(mtdDateType* aDate)
{
    return (acp_uint8_t)((aDate->mon_day_hour
                          & MTD_DATE_MON_MASK)
                         >> MTD_DATE_MON_SHIFT);
}

ACP_INLINE acp_uint8_t mtdDateInterfaceDay(mtdDateType* aDate)
{
    return (acp_uint8_t)((aDate->mon_day_hour
                          & MTD_DATE_DAY_MASK)
                         >> MTD_DATE_DAY_SHIFT);
}

ACP_INLINE acp_uint8_t mtdDateInterfaceHour(mtdDateType* aDate)
{
    return (acp_uint8_t)(aDate->mon_day_hour
                         & MTD_DATE_HOUR_MASK);
}


ACP_INLINE acp_uint8_t mtdDateInterfaceMinute(mtdDateType* aDate)
{
    return (acp_uint8_t)((aDate->min_sec_mic
                          & MTD_DATE_MIN_MASK)
                         >> MTD_DATE_MIN_SHIFT);
}

ACP_INLINE acp_uint8_t mtdDateInterfaceSecond(mtdDateType* aDate)
{
    return (acp_uint8_t)((aDate->min_sec_mic
                          & MTD_DATE_SEC_MASK)
                         >> MTD_DATE_SEC_SHIFT);
}

ACP_INLINE acp_uint32_t mtdDateInterfaceMicroSecond(mtdDateType* aDate)
{
    return (acp_uint32_t)(aDate->min_sec_mic
                          & MTD_DATE_MSEC_MASK);
}

ACP_INLINE acp_sint32_t mtdDateInterfaceCompare(mtdDateType* aDate1, mtdDateType* aDate2)
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

ACI_RC mtdDateInterfaceSetYear(mtdDateType* aDate, acp_sint16_t aYear);

ACI_RC mtdDateInterfaceSetMonth(mtdDateType* aDate, acp_uint8_t aMonth);

ACI_RC mtdDateInterfaceSetDay(mtdDateType* aDate, acp_uint8_t aDay);

ACI_RC mtdDateInterfaceSetHour(mtdDateType* aDate, acp_uint8_t aHour);

ACI_RC mtdDateInterfaceSetMinute(mtdDateType* aDate, acp_uint8_t aMinute);

ACI_RC mtdDateInterfaceSetSecond(mtdDateType* aDate, acp_uint8_t aSec);

ACI_RC mtdDateInterfaceSetMicroSecond(mtdDateType* aDate, acp_uint32_t aMicroSec);

ACI_RC mtdDateInterfaceMakeDate(mtdDateType  * aDate,
                                acp_sint16_t   aYear,
                                acp_uint8_t    aMonth,
                                acp_uint8_t    aDay,
                                acp_uint8_t    aHour,
                                acp_uint8_t    aMinute,
                                acp_uint8_t    aSec,
                                acp_uint32_t   aMicroSec);

ACI_RC mtdDateInterfaceConvertDate2Interval(mtdDateType     * aDate,
                                            mtdIntervalType * aInterval);

ACI_RC mtdDateInterfaceToDate(mtdDateType  * aDate,
                              acp_uint8_t  * aString,
                              acp_uint32_t   aStringLen,
                              acp_uint8_t  * aFormat,
                              acp_uint32_t   aFormatLen);

// date format 정보를 모르는 경우
ACI_RC mtdDateInterfaceToChar(mtdDateType * aDate,
                              acp_uint8_t * aString,
                              acp_uint32_t* aStringLen,
                              acp_sint32_t  aStringMaxLen,
                              acp_uint8_t * aFormat,
                              acp_uint32_t  aFormatLen );

ACI_RC mtdDateInterfaceCheckYearMonthDayAndSetDateValue( mtdDateType* aDate,
                                                         acp_sint16_t aYear,
                                                         acp_uint8_t  aMonth,
                                                         acp_uint8_t  aDay);

typedef acp_uint8_t mtdBooleanType;
# define MTD_BOOLEAN_TRUE  ((mtdBooleanType)0x00)
# define MTD_BOOLEAN_FALSE ((mtdBooleanType)0x01)
# define MTD_BOOLEAN_NULL  ((mtdBooleanType)0x02)

typedef struct mtsFileType
{
    acp_std_file_t * fp;
    acp_char_t       mode[2];
} mtsFileType;

// BUG-40854
typedef struct mtsConnectType
{
    acp_sint64_t connectionNodeKey;
} mtsConnectType;

typedef struct mtdBitType {
    acp_uint32_t  length;
    acp_uint8_t   value[1];
} mtdBitType;

extern const mtdBitType mtcdBitNull;

#define MTD_BIT_ALIGN             (sizeof(acp_uint32_t))
#define MTD_BIT_PRECISION_DEFAULT (1)
#define MTD_BIT_PRECISION_MINIMUM (MTD_CHAR_PRECISION_MINIMUM)  // to fix BUG-12597
#define MTD_BIT_PRECISION_MAXIMUM (2*MTD_CHAR_PRECISION_MAXIMUM)// to fix BUG-12597

#define MTD_VARBIT_ALIGN             MTD_BIT_ALIGN
#define MTD_VARBIT_PRECISION_DEFAULT MTD_BIT_PRECISION_DEFAULT
#define MTD_VARBIT_PRECISION_MINIMUM MTD_BIT_PRECISION_MINIMUM
#define MTD_VARBIT_PRECISION_MAXIMUM MTD_BIT_PRECISION_MAXIMUM

#define BIT_TO_BYTE(n)            ( ((n) + 7) >> 3 )

#define MTD_BIT_TYPE_STRUCT_SIZE( valueLength )             \
    ( sizeof( acp_uint32_t ) + BIT_TO_BYTE(valueLength) )

// PROJ-1583, PR-15722
// SQL_BINARY에 대응하는 mtdBinaryType
// PROJ-1583 large geometry 
// mLength의 타입을 acp_uint32_t로 변경
typedef struct mtdBinaryType
{
    acp_uint32_t mLength;
    acp_uint8_t  mPadding[4];
    acp_uint8_t  mValue[1];
} mtdBinaryType;

extern mtdBinaryType mtcdBinaryNull;

#define MTD_BINARY_PRECISION_DEFAULT (0)
#define MTD_BINARY_PRECISION_MINIMUM (0)
#define MTD_BINARY_PRECISION_MAXIMUM ( ACP_SINT32_MAX ) // 2G

#define MTD_BINARY_TYPE_STRUCT_SIZE( valueLength )  \
    ( sizeof( acp_uint32_t ) + 4 + (valueLength) )

#define MTD_CLOB_PRECISION_MINIMUM (0) // To Fix BUG-12597
#define MTD_CLOB_PRECISION_MAXIMUM ( (acp_sint32_t) (65536 - sizeof(acp_sint32_t)) )

// PROJ-1362
typedef struct mtdLobType {
    acp_uint32_t length;
    acp_uint8_t  value[1];
} mtdLobType;

#define MTD_LOB_TYPE_STRUCT_SIZE( valueLength ) \
    ( sizeof( acp_uint32_t ) + (valueLength) )

typedef mtdLobType mtdBlobType;
#define MTD_BLOB_TYPE_STRUCT_SIZE MTD_LOB_TYPE_STRUCT_SIZE

typedef mtdLobType mtdClobType;
#define MTD_CLOB_TYPE_STRUCT_SIZE MTD_LOB_TYPE_STRUCT_SIZE

typedef mtdLobLocator mtdBlobLocatorType;
#define MTD_LOCATOR_NULL       ((mtdBlobLocatorType)(mtdLobLocator) 0)
#define MTD_BLOB_LOCATOR_NULL  MTD_LOCATOR_NULL

typedef mtdLobLocator mtdClobLocatorType;
#define MTD_CLOB_LOCATOR_NULL  MTD_LOCATOR_NULL

typedef struct mtdByteType {
    acp_uint16_t length;
    acp_uint8_t  value[1];
} mtdByteType;

extern const mtdByteType mtcdByteNull;

#define MTD_BYTE_ALIGN             ( (acp_sint32_t) sizeof(acp_uint16_t) )
#define MTD_BYTE_PRECISION_DEFAULT (1)
#define MTD_BYTE_PRECISION_MINIMUM (0) // To Fix BUG-12597
#define MTD_BYTE_PRECISION_MAXIMUM ( (acp_sint32_t) (65536 - sizeof(acp_uint16_t)) )

#define MTD_VARBYTE_ALIGN             MTD_BYTE_ALIGN
#define MTD_VARBYTE_PRECISION_DEFAULT MTD_BYTE_PRECISION_DEFAULT
#define MTD_VARBYTE_PRECISION_MINIMUM MTD_BYTE_PRECISION_MINIMUM
#define MTD_VARBYTE_PRECISION_MAXIMUM MTD_BYTE_PRECISION_MAXIMUM

#define MTD_BYTE_TYPE_STRUCT_SIZE( valueLength )    \
    ( sizeof( acp_uint16_t ) + (valueLength) )

#define MTD_PIE 3.1415926535897932384626
#define MTD_COMPILEDFMT_MAXLEN (256)

typedef struct mtdNibbleType {
    acp_uint8_t length;
    acp_uint8_t value[1];
} mtdNibbleType;

extern const mtdNibbleType mtcdNibbleNull;

#define MTD_NIBBLE_PRECISION_DEFAULT (1)
#define MTD_NIBBLE_PRECISION_MINIMUM (0)  // to fix BUG-12597
#define MTD_NIBBLE_PRECISION_MAXIMUM (254)// to fix BUG-12597
#define MTD_NIBBLE_NULL_LENGTH       (255)

#define MTD_NIBBLE_TYPE_STRUCT_SIZE( valueLength )  \
    ( sizeof( acp_uint8_t ) + ((valueLength+1)/2) )


// --------------------------------
// PROJ-1579 NCHAR
// --------------------------------

#define MTD_NCHAR_ALIGN                     (sizeof(acp_uint16_t))
#define MTD_NCHAR_PRECISION_DEFAULT         (1)

// BUG-25914 : UTF8의 한 글자의 크기는 3Byte임.
// MTD_NCHAR_PRECISION_MAXIMUM / MTL_UTF8_PRECISION
#define MTD_UTF8_NCHAR_PRECISION_MAXIMUM    ( (acp_sint32_t)((65536/3) - sizeof(acp_uint16_t)) )
// MTD_NCHAR_PRECISION_MAXIMUM / MTL_UTF16_PRECISION
#define MTD_UTF16_NCHAR_PRECISION_MAXIMUM   ( (acp_sint32_t)((65536/2) - sizeof(acp_uint16_t)) )

#define MTD_NCHAR_PRECISION_MINIMUM (0)    // to fix BUG-12597
#define MTD_NCHAR_PRECISION_MAXIMUM ((acp_sint32_t)65536 - sizeof(acp_uint16_t) )

// CHAR_PRECISION_MAXIMUM과 SYNC를 맞추어야 함
// MTD_NCHAR_PRECISION_MAXIMUM / MTL_UTF8_PRECISION
#define MTD_UTF8_NCHAR_STORE_PRECISION_MAXIMUM    (10666)

// MTD_NCHAR_PRECISION_MAXIMUM / MTL_UTF16_PRECISION
#define MTD_UTF16_NCHAR_STORE_PRECISION_MAXIMUM   (16000)


typedef struct mtdNcharType {
    acp_uint16_t length;      // 바이트수
    acp_uint8_t  value[1];
} mtdNcharType;

extern const mtdNcharType mtcdNcharNull;

// PROJ-1579 NCHAR

// CHAR => NCHAR
ACI_RC mtdNcharInterfaceToNchar( mtcStack         * aStack,
                                 const mtlModule  * aSrcCharSet,
                                 const mtlModule  * aDestCharSet,
                                 mtdCharType      * aSource,
                                 mtdNcharType     * aResult );

// CHAR => NCHAR
ACI_RC mtdNcharInterfaceToNchar2( mtcStack         * aStack,
                                  const mtlModule  * aSrcCharSet,
                                  const mtlModule  * aDestCharSet,
                                  acp_uint8_t      * aSource,
                                  acp_uint16_t       aSourceLen,
                                  mtdNcharType     * aResult );

// NCHAR => CHAR
ACI_RC mtdNcharInterfaceToChar( mtcStack         * aStack,
                                const mtlModule  * aSrcCharSet,
                                const mtlModule  * aDestCharSet,
                                mtdNcharType     * aSource,
                                mtdCharType      * aResult );

// NCHAR => CHAR
ACI_RC mtdNcharInterfaceToChar2( mtcStack         * aStack,
                                 const mtlModule  * aSrcCharSet,
                                 const mtlModule  * aDestCharSet,
                                 mtdNcharType     * aSource,
                                 acp_uint8_t      * aResult,
                                 acp_uint16_t     * aResultLen );

ACI_RC mtdNcharInterfaceToNchar4UnicodeLiteral(
    const mtlModule *  aSourceCharSet,
    const mtlModule *  aResultCharSet,
    acp_uint8_t     *  aSourceIndex,
    acp_uint8_t     *  aSourceFence,
    acp_uint8_t     ** aResultValue,
    acp_uint8_t     *  aResultFence,
    acp_uint32_t    *  aNcharCnt );

#define MTC_1_BYTE_ASSIGN(dst, src){                            \
        *((acp_uint8_t*) (dst))    = *((acp_uint8_t*) (src));   \
    }

#define MTC_2_BYTE_ASSIGN(dst, src){                                \
        *((acp_uint8_t*) (dst))    = *((acp_uint8_t*) (src));       \
        *(((acp_uint8_t*)(dst))+1) = *(((acp_uint8_t*)(src))+1);    \
    }

#define MTC_4_BYTE_ASSIGN(dst, src){                                \
        *((acp_uint8_t*) (dst))    = *((acp_uint8_t*) (src));       \
        *(((acp_uint8_t*)(dst))+1) = *(((acp_uint8_t*)(src))+1);    \
        *(((acp_uint8_t*)(dst))+2) = *(((acp_uint8_t*)(src))+2);    \
        *(((acp_uint8_t*)(dst))+3) = *(((acp_uint8_t*)(src))+3);    \
    }

#define MTC_8_BYTE_ASSIGN(dst, src){                                \
        *((acp_uint8_t*) (dst))    = *((acp_uint8_t*) (src));       \
        *(((acp_uint8_t*)(dst))+1) = *(((acp_uint8_t*)(src))+1);    \
        *(((acp_uint8_t*)(dst))+2) = *(((acp_uint8_t*)(src))+2);    \
        *(((acp_uint8_t*)(dst))+3) = *(((acp_uint8_t*)(src))+3);    \
        *(((acp_uint8_t*)(dst))+4) = *(((acp_uint8_t*)(src))+4);    \
        *(((acp_uint8_t*)(dst))+5) = *(((acp_uint8_t*)(src))+5);    \
        *(((acp_uint8_t*)(dst))+6) = *(((acp_uint8_t*)(src))+6);    \
        *(((acp_uint8_t*)(dst))+7) = *(((acp_uint8_t*)(src))+7);    \
    }

#define MTC_CHAR_BYTE_ASSIGN   MTC_1_BYTE_ASSIGN
#define MTC_SHORT_BYTE_ASSIGN  MTC_2_BYTE_ASSIGN
#define MTC_INT_BYTE_ASSIGN    MTC_4_BYTE_ASSIGN
#define MTC_FLOAT_BYTE_ASSIGN  MTC_4_BYTE_ASSIGN
#define MTC_DOUBLE_BYTE_ASSIGN MTC_8_BYTE_ASSIGN
#define MTC_LONG_BYTE_ASSIGN   MTC_8_BYTE_ASSIGN

acp_sint32_t mtcStrCaselessMatch(const void *aValue1,
                                 acp_size_t  aLength1,
                                 const void *aValue2,
                                 acp_size_t  aLength2);

acp_sint32_t mtcStrMatch(const void *aValue1,
                         acp_size_t  aLength1,
                         const void *aValue2,
                         acp_size_t  aLength2);

/***********************************************************************
 * PROJ-2002 COLUMN SECURITY 
 **********************************************************************/

// ECHAR
typedef struct mtdEcharType 
{
    acp_uint16_t mCipherLength;
    acp_uint16_t mEccLength;
    acp_uint8_t  mValue[1];         // cipher value + ecc value
} mtdEcharType;

#define MTD_ECHAR_TYPE_STRUCT_SIZE( valueLength )                       \
    ( sizeof( acp_uint16_t ) + sizeof( acp_uint16_t ) + (valueLength) )

extern const mtdEcharType mtcdEcharNull;

#define MTD_ECHAR_ALIGN                    (sizeof(acp_uint16_t))
#define MTD_ECHAR_PRECISION_MINIMUM        (0)
#define MTD_ECHAR_PRECISION_DEFAULT        (1)

#define MTD_ECHAR_PRECISION_MINIMUM        (0)
#define MTD_ECHAR_PRECISION_MAXIMUM        ((acp_sint32_t)(65536 - sizeof(acp_uint16_t) \
                                                           - sizeof(acp_uint16_t)))

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
#if defined(VC_WINCE) || (_MSC_VER < 1300)
#define MTC_ULTODB(aData) (acp_double_t)(acp_sint64_t)(aData)
#else
#define MTC_ULTODB(aData) (acp_double_t)(aData)
#endif /* VC_WINCE */

ACP_EXTERN_C_END

#endif /* _O_MTCD_TYPES_H_ */


 
