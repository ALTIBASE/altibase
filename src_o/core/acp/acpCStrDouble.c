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
 
/*******************************************************************************
 * $Id: acpCStrDouble.c 3423 2008-10-29 05:50:39Z jykim $
 ******************************************************************************/

/*
 * configurations for altibase core
 */

#include <aceAssert.h>
#include <acpCStr.h>
#include <acpError.h>
#include <acpBit.h>
#include <acpMem.h>
#if defined( HAVE_ECVT ) || defined( HAVE_FCVT )
#include <acpThrMutex.h>
#endif
#include <acpTypes.h>

ACP_EXTERN_C_BEGIN

/* Type definition of union of double and uint64 */
typedef union
{
    acp_double_t mDouble;
    acp_uint64_t mInt;
} acpUnionDoubleType;

/*
 * I cannot decide for sure if this or acpCStrShift() below is better
 * to use.  So I am leaving this function here commented out.
 *
static void acpCStrMove( acp_char_t *aDest, acp_char_t *aSrc, acp_size_t aMaxLen )
{
    acp_size_t sLen = 0;
    sLen = acpCStrLen( aSrc, aMaxLen ) - 1;
    acpMemMove( aDest, aSrc, sLen );
    aDest[ sLen ] = '\0';
}
*/

static void acpCStrShift( acp_char_t *aStr, acp_sint32_t aNum )
{
    for ( ; *aStr ; aStr++ )
    {
        *( aStr + aNum ) = *aStr;
    }
    *( aStr + aNum ) = '\0';
}

/*
 * 소수점을 지우고 문자열 시작을 기준으로 했을 때 위치를 반환
 */
static acp_size_t acpCStrRemoveDecimalPoint( acp_char_t *aStr, const acp_size_t aMaxLen )
{
    acp_char_t  *p = NULL;
    acp_rc_t     sRC;
    acp_sint32_t sFoundIndex;
    acp_size_t   sDecPoint;

    sRC = acpCStrFindChar( aStr,
                           '.',
                           &sFoundIndex,
                           0,
                           0 );
    if ( ACP_RC_NOT_SUCCESS( sRC ) )
    {
        /*
         * 소수점 없고 exponent도 없으므로 맨 끝에 있는 소수점이
         * 생략되어 있다.
         */
        sDecPoint = acpCStrLen( aStr, aMaxLen );
    }
    else
    {
        /*
         * Remove the decimal point.
         */
        for ( p = aStr ; *p != '.' ; p++ )
        {
            /* searching for '.' */
        }
        acpCStrShift( 1 + p, -1 );

        sDecPoint = sFoundIndex;
    }

    return sDecPoint;
}

/*
 * 수의 exponent부분을 지우고 exponent를 반환
 */
static acp_uint32_t acpCStrRemoveExp( acp_char_t *aStr, const acp_size_t aMaxLen )
{
    acp_rc_t     sRC;
    acp_sint32_t sFoundIndex;
    acp_sint32_t sSign;
    acp_uint32_t sResult;
    acp_size_t   sExp;

    sRC = acpCStrFindChar( aStr,
                           'e',
                           &sFoundIndex,
                           0,
                           ACP_CSTR_CASE_INSENSITIVE );
    if ( ACP_RC_IS_SUCCESS( sRC ) )
    {
        /*
         * 소수점 없으나 exponent는 있음
         */
        sRC = acpCStrToInt32( &aStr[ 1 + sFoundIndex ],
                              acpCStrLen( &aStr[ 1 + sFoundIndex ], aMaxLen - sFoundIndex - 1 ),
                              &sSign,
                              &sResult,
                              10,
                              NULL );
        ACE_ASSERT( ACP_RC_IS_SUCCESS( sRC ) );

        sExp = sResult * sSign;

        /*
         * e부터 지움
         */
        aStr[ sFoundIndex ] = '\0';
    }
    else
    {
        sExp = 0;
    }

    return sExp;
}

/*
 * aStr에 parse되어 있는 숫자를 significant figure만 남기고 다음
 * 정보를 반환:
 * -  처리된 문자열 시작 위치 (이전 문자열을 덮어 씀)
 * -  문자열 시작을 기준으로 했을 때 소수점 위치
 * -  싸인 정보
 *
 * 이 함수의 목표는 gcvt의 결과물을 acpPrintfRender 모듈이 사용할 수
 * 있으며 acpCStrDoubleToString의 2, 3번 모드들과 일관성 있는 값으로
 * 변환하는 것이다.
 */
static void acpCStrGcvtPostProc( acp_char_t       *aStr,
                                 const acp_size_t  aMaxLen,
                                 acp_sint32_t     *aDecimalPoint,
                                 acp_sint32_t     *aSign )
{
    *aSign = ( aStr[ 0 ] == '-' ) ? 1 : 0;

    if ( *aSign > 0 )
    {
        /*
         * 문자열에서 싸인을 삭제
         */
        acpCStrShift( 1 + aStr, -1 );
    }
    else
    {
        /* do nothing */
    }

    *aDecimalPoint = acpCStrRemoveExp( aStr, aMaxLen ) +
        acpCStrRemoveDecimalPoint( aStr, aMaxLen );
}

void acpCStrDoubleToStringFree( acp_char_t *aStr )
{
    if(aStr != NULL)
    {
        (void)acpMemFree(aStr);
    }
}

/*
 * ecvt(), fcvt()나 gcvt()를 사용해 double을 문자열로 전환 후
 * acpPrintfRender가 받아 드리는 일관성 있는 형태로 처리.
 *
 * 내부적으로 버퍼를 할당하기 때문에 사용 후
 * acpCStrDoubleToStringFree()를 호출해야 함.
 *
 * aMode:
 *   0 ==> 쓰이지 않음
 *   1 ==> 쓰이지 않음
 *   2 ==> ecvt(): max(1,ndigits) significant digits.
 *   3 ==> fcvt(): through ndigits past the decimal point.
 *   4 ==> gcvt(): fcvt()와 ecvt() 중 문자열 길이가 짧은 쪽.
 */
acpCStrDoubleType acpCStrDoubleToString(acp_double_t   aValue,
                                        acp_sint32_t   aMode,
                                        acp_sint32_t   aPrecision,
                                        acp_sint32_t  *aDecimalPoint,
                                        acp_bool_t    *aNegative,
                                        acp_char_t   **aString,
                                        acp_char_t   **aEndPtr)
{
    acp_char_t*     sString = NULL;
    acp_rc_t        sRC;
    acp_sint32_t    sResult;
    acp_sint32_t    sSign;
    acpCStrDoubleType sReturn;

#if defined( HAVE_ECVT ) || defined( HAVE_FCVT )
    static acp_thr_mutex_t sEcvtMutex = ACP_THR_MUTEX_INITIALIZER;
    static acp_thr_mutex_t sFcvtMutex = ACP_THR_MUTEX_INITIALIZER;
#endif

    sRC = acpMemAlloc((void**)&sString, 1024);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_NOMEM);

    switch(aMode)
    {
    case 2:
#if defined( HAVE_ECVT_R )
        sResult = ecvt_r(aValue, aPrecision, aDecimalPoint, &sSign,
                         sString, 1024);
#elif defined( HAVE_ECVT_S )
        sResult = _ecvt_s( sString,
                           1024,
                           aValue,
                           aPrecision,
                           aDecimalPoint,
                           &sSign );
#else
        {
            acp_char_t *sTemp = NULL;

            acpThrMutexLock( &sEcvtMutex );
            sTemp = ecvt( aValue,
                          aPrecision,
                          aDecimalPoint,
                          &sSign );
            sRC = acpCStrCpy( sString, 1024, sTemp, 1024 );
            acpThrMutexUnlock( &sEcvtMutex );

            ACP_TEST_RAISE( ACP_RC_NOT_SUCCESS( sRC ), strcpy_error );
            sResult = 0;
        }
#endif
        break;

    case 3:
#if defined( HAVE_FCVT_R )
        sResult = fcvt_r(aValue, aPrecision, aDecimalPoint, &sSign,
                         sString, 1024);
#elif defined( HAVE_FCVT_S )
        sResult = _fcvt_s( sString,
                           1024,
                           aValue,
                           aPrecision,
                           aDecimalPoint,
                           &sSign );
#else
        {
            acp_char_t *sTemp = NULL;

            acpThrMutexLock( &sFcvtMutex );
            sTemp = fcvt( aValue,
                          aPrecision,
                          aDecimalPoint,
                          &sSign );
            sRC = acpCStrCpy( sString, 1024, sTemp, 1024 );
            acpThrMutexUnlock( &sFcvtMutex );

            ACP_TEST_RAISE( ACP_RC_NOT_SUCCESS( sRC ), strcpy_error );
            sResult = 0;
        }
#endif
        break;

    case 4:
#if defined( HAVE_GCVT_S )
        (void)_gcvt_s( sString, 1024, aValue, aPrecision );
#else
        (void)gcvt( aValue, aPrecision, sString );
#endif
        sResult = 0;
        acpCStrGcvtPostProc( sString, 1024, aDecimalPoint, &sSign );
        break;

    default:
        ACE_ASSERT(0);
        break;
    }

    /*
     * 다음 섹션은 ecvt, fcvt, gcvt의 결과를 acpPrintfRender모듈이
     * 처리할 자료 구조로 변환한다.  플랫폼마다 이 함수들의 결과가
     * 약간씩 다르기 때문.
     */

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - -
     * Handle Edge Cases (sometimes platform-specific)
     * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    if ( aValue == 0.0 )
    {
        /*
         * Normalise parameters for zero value while skipping needless
         * string comparisons.
         */
        sReturn        = ACP_DOUBLE_NORMAL;
        *aNegative     = ACP_FALSE;
        *aString       = sString;
        *aEndPtr       = sString + acpCStrLen( sString, 1024 );
        *aDecimalPoint = 1;
    }
#if defined( ALTI_CFG_OS_WINDOWS ) || defined( ALTI_CFG_OS_HPUX )
    else if ( acpCStrCmp( sString, "", 1 ) == 0 )
    {
        /*
         * Normalise the parameters for the case where the absolute of
         * the value is less than 1 and precision is zero.  Windows and
         * HP-UX's fcvt() has different behaviour from other platforms
         * so I handle it as a special case here.
         */
        sReturn        = ACP_DOUBLE_NORMAL;
        *aNegative     = ACP_FALSE;
        sString[0]     = '0';
        sString[1]     = '\0';
        *aString       = sString;
        *aEndPtr       = 1 + sString;
        *aDecimalPoint = 1;
    }
#endif
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - -
     * Handle Not a Number
     * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    else if (
#if defined( ALTI_CFG_OS_WINDOWS )
        acpCStrCmp( sString, "1#IND", 5 ) == 0 ||
        acpCStrCmp( sString, "-1#IND", 6 ) == 0
#elif defined( ALTI_CFG_OS_AIX )
        acpCStrCmp( sString, "NaNQ", 4 ) == 0
#elif defined( ALTI_CFG_OS_SOLARIS )
        acpCStrCmp( sString, "NaN", 3 ) == 0
#elif defined( ALTI_CFG_OS_HPUX )
        acpCStrCmp( sString, "?", 1 ) == 0 ||
        acpCStrCmp( sString, "-?", 2 ) == 0
#else
        acpCStrCmp( sString, "nan", 3 ) == 0 ||
        acpCStrCmp( sString, "NAN", 3 ) == 0 ||
        acpCStrCmp( sString, "-nan", 4 ) == 0 ||
        acpCStrCmp( sString, "-NAN", 4 ) == 0
#endif
        )
    {
        sReturn = ACP_DOUBLE_NAN;
        *aNegative = ACP_FALSE;
        *aString = sString;
        *aEndPtr = NULL;
    }
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - -
     * Handle Positive (or Negative) Infinity
     * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    else if (
#if defined( ALTI_CFG_OS_WINDOWS )
        acpCStrCmp( sString, "1#INF", 5 ) == 0
#elif defined( ALTI_CFG_OS_SOLARIS )
        acpCStrCmp( sString, "Inf", 3 ) == 0
#elif defined( ALTI_CFG_OS_HPUX )
        acpCStrCmp( sString, "++", 2 ) == 0
#else
        acpCStrCmp( sString, "inf", 3 ) == 0 ||
        acpCStrCmp( sString, "INF", 3 ) == 0
#endif
        )
    {
        sReturn = ACP_DOUBLE_INF;
        *aNegative = ( sSign == 0 ) ? ACP_FALSE : ACP_TRUE;
        *aString = sString;
        *aEndPtr = NULL;
    }
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - -
     * Handle Negative Infinity
     * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    else if (
#if defined( ALTI_CFG_OS_WINDOWS )
        acpCStrCmp( sString, "-1#INF", 6 ) == 0
#elif defined( ALTI_CFG_OS_SOLARIS )
        acpCStrCmp( sString, "-Inf", 3 ) == 0
#elif defined( ALTI_CFG_OS_HPUX )
        acpCStrCmp( sString, "--", 2 ) == 0
#else
        acpCStrCmp( sString, "-inf", 4 ) == 0 ||
        acpCStrCmp( sString, "-INF", 4 ) == 0
#endif
        )
    {
        sReturn = ACP_DOUBLE_INF;
        *aNegative = ACP_TRUE;
        *aString = sString;
        *aEndPtr = NULL;
    }
    else
    {
        sReturn = ACP_DOUBLE_NORMAL;
        *aNegative = (sSign == 0)? ACP_FALSE:ACP_TRUE;
        *aString = sString;
        *aEndPtr = sString + acpCStrLen( sString, 1024 );
    }

    ACE_ASSERT(sResult == 0);

    return sReturn;

    ACP_EXCEPTION(E_NOMEM)
    {
    }

#if defined( HAVE_ECVT ) || defined( HAVE_FCVT )
    ACP_EXCEPTION( strcpy_error )
    {
    }
#endif

    ACP_EXCEPTION_END;
    ACE_ASSERT(0);
    return 0;
}

/* Pick up sign character */
ACP_INLINE acp_sint32_t acpCStrInternalGetSign(const acp_char_t** aStr)
{
    if(**aStr == '-')
    {
        (*aStr)++;
        return -1;
    }
    else
    {
        if(**aStr == '+')
        {
            (*aStr)++;
        }
        else
        {
            /* Do nothing */
        }

        return 1;
    }
}

static acp_rc_t acpCStrInternalCharToInt(
    acp_sint32_t* aValue,
    const acp_char_t aChar)
{
    ACP_TEST(ACP_FALSE == acpCharIsDigit(aChar));
    *aValue = aChar - '0';
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_ERANGE;
}

static void acpCStrToDoubleGetNumberPart(
    const acp_char_t* aStr,
    acp_size_t aStrLen,
    acp_uint64_t* aNumber,
    acp_uint64_t* aExponent,
    acp_bool_t* aInRange,
    acp_char_t** aEnd)
{
    const acp_char_t*   sStr = aStr;
    const acp_uint64_t  sMaxValue = (acp_uint64_t)(ACP_UINT64_MAX / 10);
    const acp_sint64_t  sMaxDigit = (acp_sint64_t)(ACP_UINT64_MAX % 10);
    acp_uint64_t        sNumber = 0;
    acp_uint64_t        sExponent = 0;
    acp_sint32_t        sDigit;
    acp_bool_t          sInRange = ACP_TRUE;
    acp_rc_t            sRC;

    while((ACP_CSTR_TERM != *sStr) && (aStrLen > 0))
    {
        sRC = acpCStrInternalCharToInt(&sDigit, *sStr);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            if(
                (sNumber > sMaxValue ) ||
                ((sMaxValue == sNumber) && (sDigit > sMaxDigit))
              )
            {
                /* Just increase exponent */
                sInRange = ACP_FALSE;
                sExponent++;
            }
            else
            {
                sNumber = sNumber * 10 + sDigit;
            }
        }
        else
        {
            break;
        }

        sStr++;
        aStrLen--;
    }

    *aNumber = sNumber;
    *aEnd = (acp_char_t*)sStr;
    *aExponent = sExponent;
    *aInRange = sInRange;
}

static void acpCStrToDoubleGetFractionPart(
    const acp_char_t* aStr,
    acp_size_t aStrLen,
    acp_uint64_t* aNumber,
    acp_uint64_t* aExponent,
    acp_uint64_t* aDepth,
    acp_char_t** aEnd)
{
    const acp_char_t*   sStr = aStr;
    const acp_uint64_t  sMaxValue = (acp_uint64_t)(ACP_UINT64_MAX / 10);
    const acp_sint64_t  sMaxDigit = (acp_sint64_t)(ACP_UINT64_MAX % 10);
    acp_uint64_t        sNumber = 0;
    acp_uint64_t        sDepth = 1;
    acp_uint64_t        sExponent = 0;
    acp_sint32_t        sDigit;
    acp_rc_t            sRC;

    while((ACP_CSTR_TERM != *sStr) && (aStrLen > 0))
    {
        sRC = acpCStrInternalCharToInt(&sDigit, *sStr);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            if(
                (sNumber > sMaxValue ) ||
                ((sMaxValue == sNumber) && (sDigit > sMaxDigit)) ||
                (sDepth > sMaxValue)
              )
            {
                /* Just increase pointer to find the end */
            }
            else
            {
                if((0 == sDigit) && (1 == sDepth))
                {
                    sExponent++;
                }
                else
                {
                    sNumber = sNumber * 10 + sDigit;
                    sDepth *= 10;
                }
            }
        }
        else
        {
            break;
        }

        sStr++;
        aStrLen--;
    }

    *aNumber = sNumber;
    *aEnd = (acp_char_t*)sStr;
    *aExponent = sExponent;
    *aDepth = sDepth;
}

static acp_rc_t acpCStrToDoubleBuildDouble(
    acp_double_t* aResult,
    acp_sint32_t  aSign,
    acp_uint64_t  aIntegral,
    acp_uint64_t  aIntExp,
    acp_uint64_t  aFractional,
    acp_uint64_t  aFractDepth,
    acp_sint32_t  aExpSign,
    acp_uint64_t  aExp,
    acp_bool_t    aNeedFract,
    acp_uint64_t  aFractExp)
{
    acpUnionDoubleType sResult;
    acpUnionDoubleType sToCheck;
    acp_sint64_t sBinExp = 0;
    acp_uint64_t sFractBit = 0;
    acp_uint64_t sMantissa;
    acp_uint32_t i;
    acp_rc_t sRC;

    const acp_uint32_t sSize = sizeof(acp_double_t) * 8;

    if(ACP_TRUE == aNeedFract)
    {
        acp_uint64_t sPos;
        acp_uint64_t sFractShift = 0;

        /* bulid fractional bits */
        for(i = 0; i < sizeof(acp_uint64_t) * 8; i++)
        {
            if(aFractional > (aFractDepth / 2))
            {
                sFractBit |= ACP_UINT64_LITERAL(0x8000000000000000) >> i;
                aFractional -= aFractDepth / 2;
            }
            else
            {
                /* Do nothing */
            }

            aFractional *= 2;
        }

        for(i = 0; i < aFractExp; i++)
        {
            sFractBit /= 10;
            sPos = sSize - acpBitFls64(sFractBit) - 1;
            sFractShift += sPos;

            sFractBit = sFractBit << sPos;

        }

        /* build mantissa part */
        if(0 == aIntegral)
        {
            sBinExp = -(64 - acpBitFls64(sFractBit));
            sMantissa = sFractBit << (acp_uint64_t)(-sBinExp - 1);
            sBinExp -= sFractShift;
        }
        else
        {
            sBinExp = acpBitFls64(aIntegral);
            sMantissa = 
                (aIntegral << (acp_uint64_t)(sSize - sBinExp - 1)) |
                ((sFractBit >> (acp_uint64_t)(sBinExp + 1)) >> sFractShift);
        }
    }
    else
    {
        acp_uint32_t sPos;

        sBinExp = acpBitFls64(aIntegral);
        sMantissa = aIntegral << (acp_uint64_t)(sSize - sBinExp - 1);

        for(i = 0; i < aIntExp; i++)
        {
            sMantissa /= 16;
            sMantissa *= 10;
            sPos = sSize - acpBitFls64(sMantissa) - 1;
            sBinExp += 4 - sPos;

            sMantissa = sMantissa << sPos;
        }
    }

    /* find binary exponent */
    if(aExpSign == 1)
    {
        /* upward */
        acp_uint32_t sPos;
        for(i = 0; i < aExp; i++)
        {
            sMantissa /= 16;
            sMantissa *= 10;
            sPos = sSize - acpBitFls64(sMantissa) - 1;
            sBinExp += 4 - sPos;

            sMantissa = sMantissa << sPos;
        }
    }
    else
    {
        /* downward */
        acp_uint32_t sPos;
        for(i = 0; i < aExp; i++)
        {
            sMantissa /= 10;
            sPos = sSize - acpBitFls64(sMantissa) - 1;
            sBinExp -= sPos;

            sMantissa = sMantissa << sPos;
        }
    }

    ACP_TEST_RAISE(sBinExp >=  1024, HANDLE_OVERFLOW);

    /* 52bit of mantissa */
    sResult.mInt = ACP_UINT64_LITERAL(0x000FFFFFFFFFFFFF) & (sMantissa >> 11);
    sResult.mInt += (sMantissa % 2048 >= ACP_UINT64_LITERAL(1024))? 1:0;
    /* Check overflow in mantissa */
    sBinExp += (sResult.mInt > ACP_UINT64_LITERAL(0x000FFFFFFFFFFFFF))? 1:0;
    sResult.mInt &= ACP_UINT64_LITERAL(0x000FFFFFFFFFFFFF);
    sToCheck.mInt = sResult.mInt;
    /* handle overflow */
    if(sBinExp > 0)
    {
        sToCheck.mInt |= (acp_uint64_t)(1023 + sBinExp - 1) << 52;

        ACP_TEST_RAISE(
            sToCheck.mDouble > DBL_MAX / 2.,
            HANDLE_OVERFLOW);
    }
    else
    {
        /* Do nothing */
    }

    /* Handle subnormal values */
    if(sBinExp <= -1024)
    {
        sResult.mInt >>= 1;
        sResult.mInt |= ACP_UINT64_LITERAL(1) << 51;
        sResult.mInt >>= -sBinExp - 1023;
        /* No need to set exponent. It's already reset to 0 before */
    }
    else
    {
        /* 11bit of exponent */
        sResult.mInt |= (acp_uint64_t)(1023 + sBinExp) << 52;
    }

    /* Check subnormal underflow */
    ACP_TEST_RAISE(sResult.mInt == 0, HANDLE_UNDERFLOW);

    /* 1bit of sign */
    sResult.mInt |= (1 == aSign)? 0 : ACP_UINT64_LITERAL(0x8000000000000000);
    *aResult = sResult.mDouble;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(HANDLE_OVERFLOW)
    {
        acpUnionDoubleType sInf;
        sInf.mInt = (1 == aSign)?
            ACP_UINT64_LITERAL(0x7FF0000000000000) :
            ACP_UINT64_LITERAL(0xFFF0000000000000);
        *aResult = sInf.mDouble;
        sRC = ACP_RC_ERANGE;
    }

    ACP_EXCEPTION(HANDLE_UNDERFLOW)
    {
        *aResult = 0.0 * aSign;
        sRC = ACP_RC_ERANGE;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

static acp_rc_t acpCStrToDoubleDivideString(
    const acp_char_t* aStr,
    acp_size_t aStrLen,
    acp_uint64_t* aIntegral,
    acp_uint64_t* aIntExp,
    acp_bool_t* aNeedFract,
    acp_uint64_t* aFractional,
    acp_uint64_t* aFractExp,
    acp_uint64_t* aFractDepth,
    acp_uint64_t* aExp,
    acp_sint32_t* aExpSign,
    acp_char_t** aEnd)
{
    const acp_char_t*   sStr = aStr;
    acp_char_t*         sEnd = NULL;
    acp_size_t          sStrLen;
    acp_rc_t sRC;

    /* Get integral part */
    acpCStrToDoubleGetNumberPart(
        sStr, aStrLen, aIntegral,
        aIntExp, aNeedFract, &sEnd);

    /* Adjust length and start position */
    sStr = sEnd;
    sStrLen = sStr - aStr;

    if(sStrLen < aStrLen)
    {
        sStrLen = aStrLen - (sStr - aStr);
        if('.' == *sStr)
        {
            /* Number is in the format of "MMM.NNN*"
             * Get fractional part */
            sStr++;
            sStrLen--;
            acpCStrToDoubleGetFractionPart(
                sStr, sStrLen, aFractional, aFractExp, aFractDepth, &sEnd);

            /* Adjust length and start position */
            sStr = sEnd;
            sStrLen = aStrLen - (sStr - aStr);
        }
        else
        {
            /* Number is in the format of "^.MMM*" */
        }
    }
    else
    {
        /* Do nothing */
    }

    /* Number is "0.0*" */
    ACP_TEST_RAISE(
        (0 == *aIntegral) && (0 == *aFractional) && (sStr == aStr),
        HANDLE_ZERO);

    sStrLen = sStr - aStr;
    if(sStrLen < aStrLen)
    {
        sStrLen = aStrLen - (sStr - aStr);
        /* Get Exponent part */
        if(('e' == *sStr) || ('E' == *sStr) || ('f' == *sStr) || ('F' == *sStr))
        {
            sStr++;
            sStrLen--;
            sRC = acpCStrToInt64(sStr, sStrLen, aExpSign, aExp, 10, &sEnd);

            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), HANDLE_EXP_ERROR);

            if(sStr == sEnd)
            {
                /* No exponent. sStr[-1] is the first invalid character */
                sStr--;
                sEnd = (acp_char_t*)sStr;

                *aExp = 0;
            }
            else
            {
                /* Do nothing */
            }
        }
        else
        {
            /* Gotcha! first invalid character */
            sEnd =(acp_char_t*)sStr;
        }
    }
    else
    {
        sEnd =(acp_char_t*)sStr;
    }

    *aEnd = sEnd;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(HANDLE_ZERO);
    {
        sRC = ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(HANDLE_EXP_ERROR)
    {
        /* We already know error code */
    }

    ACP_EXCEPTION_END;
    *aEnd = sEnd;
    return sRC;
}

static acp_bool_t acpCStrToDoubleIsNan(
    const acp_char_t* aStr,
    acp_size_t aStrLen,
    acp_char_t** aEnd
    )
{
    acp_bool_t sReturn;
    if(
        (3 <= aStrLen) &&
        ('n' == acpCharToLower(aStr[0])) &&
        ('a' == acpCharToLower(aStr[1])) &&
        ('n' == acpCharToLower(aStr[2]))
      )
    {
        /* String begins with NaN */
        *aEnd = (acp_char_t*)(aStr + 3);
        /* Process "(...)" */
        if('(' == (**aEnd))
        {
            acp_char_t*  sNext  = *aEnd;
            acp_uint32_t sIndex = aStrLen - 3;

            while((ACP_CSTR_TERM != *sNext) && (sIndex > 0))
            {
                if(')' == (*sNext))
                {
                    *aEnd = sNext + 1;
                    break;
                }
                else
                {
                    sNext++;
                }
            }
        }
        else
        {
            /* Do nothing */
        }

        sReturn = ACP_TRUE;
    }
    else
    {
        sReturn = ACP_FALSE;
    }

    return sReturn;
}

static acp_bool_t acpCStrToDoubleIsInf(
    const acp_char_t* aStr,
    acp_size_t aStrLen,
    acp_char_t** aEnd
    )
{
    acp_bool_t sReturn;

    if(
        (3 <= aStrLen) &&
        ('i' == acpCharToLower(aStr[0])) &&
        ('n' == acpCharToLower(aStr[1])) &&
        ('f' == acpCharToLower(aStr[2]))
      )
    {
        /* String begins with Inf */
        *aEnd = (acp_char_t*)aStr + 3;
        sReturn = ACP_TRUE;
    }
    else
    {
        sReturn = ACP_FALSE;
    }

    return sReturn;
}

ACP_EXPORT acp_rc_t acpCStrToDouble(
    const acp_char_t* aStr,
    acp_size_t aStrLen,
    acp_double_t* aResult,
    acp_char_t** aEnd)
{
    const acp_char_t*   sStr;
    acp_sint32_t        sSign;
    acp_sint32_t        sDummy;
    acp_uint64_t        sIntegral = 0;
    acp_uint64_t        sFractional = 0;
    acp_uint64_t        sFractDepth = 1;
    acp_sint32_t        sExpSign = 1;
    acp_uint64_t        sExp = 0;
    acp_uint64_t        sIntExp = 1;
    acp_bool_t          sNeedFract = ACP_TRUE;
    acp_uint64_t        sFractExp = 0;
    acp_char_t*         sEnd = NULL;
    acp_rc_t            sRC;

    /* Delete whitespaces */
    while((ACP_TRUE == acpCharIsSpace(*aStr)) && (aStrLen > 0))
    {
        aStr++;
        aStrLen--;
    }

    /* Empty String */
    ACP_TEST_RAISE((ACP_CSTR_TERM == *aStr) || (0 == aStrLen),
                   HANDLE_EMPTY_STRING);

    /* Get Sign */
    sStr = aStr;
    sSign = acpCStrInternalGetSign(&sStr);
    aStrLen -= sStr - aStr;

    /* Handle NAN */
    ACP_TEST_RAISE(
        ACP_TRUE == acpCStrToDoubleIsNan(sStr, aStrLen, &sEnd),
        HANDLE_NAN);

    /* Handle INF */
    ACP_TEST_RAISE(
        ACP_TRUE == acpCStrToDoubleIsInf(sStr, aStrLen, &sEnd),
        HANDLE_INF);

    /* String begins with improper character */
    ACP_TEST_RAISE(
        ACP_RC_NOT_SUCCESS(acpCStrInternalCharToInt(&sDummy, *sStr)) &&
        ('.' != *sStr),
        HANDLE_EMPTY_STRING);

    sRC = acpCStrToDoubleDivideString(
        sStr, aStrLen, &sIntegral, &sIntExp, &sNeedFract,
        &sFractional, &sFractExp, &sFractDepth,
        &sExp, &sExpSign, &sEnd);

    /* Error while dividing  */
    ACP_TEST_RAISE(ACP_RC_IS_ERANGE(sRC), HANDLE_RANGE);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), HANDLE_OTHER);

    /* Number is "0.0*" */
    ACP_TEST_RAISE((0 == sIntegral) && (0 == sFractional), HANDLE_ZERO);

    if(NULL != aEnd)
    {
        *aEnd = sEnd;
    }
    else
    {
        /* Do nothing */
    }

    sRC = acpCStrToDoubleBuildDouble(
        aResult,
        sSign, sIntegral, sIntExp, sFractional, sFractDepth,
        sExpSign, sExp, sNeedFract, sFractExp);

    return sRC;

    ACP_EXCEPTION(HANDLE_EMPTY_STRING)
    {
        *aResult = 0.0;
        /* Set aEnd to the end of whitespaces */
        sEnd = (acp_char_t*)aStr;

        /* Originally, Error handler must return error code,
         * but without this, there would be too many level of
         * nested if's.
         * So, as a special case, the error handler returns success */
        sRC = ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(HANDLE_NAN)
    {
        /* Not a number */
        acpUnionDoubleType sNan;
        sNan.mInt = ACP_UINT64_LITERAL(0x7FFFFFFFFFFFFFFF);
        *aResult = sNan.mDouble;
        /* Here too */
        sRC = ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(HANDLE_INF)
    {
        acpUnionDoubleType sInf;
        sInf.mInt = (1 == sSign)?
            ACP_UINT64_LITERAL(0x7FF0000000000000) :
            ACP_UINT64_LITERAL(0xFFF0000000000000);
        *aResult = sInf.mDouble;
        /* Here again */
        sRC = ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(HANDLE_ZERO)
    {
        *aResult = 0.0;
        /* Success again */
        sRC = ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(HANDLE_RANGE)
    {
        if(sExpSign > 0)
        {
            acpUnionDoubleType sInf;
            sInf.mInt = (1 == sSign)?
                ACP_UINT64_LITERAL(0x7FF0000000000000) :
                ACP_UINT64_LITERAL(0xFFF0000000000000);
            *aResult = sInf.mDouble;
        }
        else
        {
            *aResult = 0.0 * sSign;
        }
    }

    ACP_EXCEPTION(HANDLE_OTHER)
    {
        /* Nothing to handle */
    }

    ACP_EXCEPTION_END;
    if(NULL != aEnd)
    {
        *aEnd = sEnd;
    }
    else
    {
        /* Do nothing */
    }
    return sRC;
}

ACP_EXTERN_C_END
