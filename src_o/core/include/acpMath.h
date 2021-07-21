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
 * $Id: acpMath.h 8994 2009-12-03 06:14:02Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_MATH_H_)
#define _O_ACP_MATH_H_

/**
 * @file
 * @ingroup CoreMath
 */

#include <acpTypes.h>
#include <acpError.h>

/* In some version of HP-UX, C++ header does not define isinf() */
/* so that reference to the C header is need. */
/* The native c++ compiler has no problem. */
#if defined (__GNUC__) && defined(ALTI_CFG_OS_HPUX) && defined(__cplusplus)
# if (ALTI_CFG_OS_MAJOR == 11) && (ALTI_CFG_OS_MINOR > 11)
ACP_EXTERN_C_BEGIN
acp_sint32_t isinf(acp_double_t);
ACP_EXTERN_C_END
# endif
#endif


ACP_EXTERN_C_BEGIN

/* 
 * error handling for math library 
 */
ACP_EXPORT acp_rc_t acpMathGetException(void);

    
/*
 * trigonometric functions
 */
ACP_EXPORT acp_rc_t acpMathAcos(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast arc cosine function
 */
ACP_INLINE acp_double_t acpMathFastAcos(acp_double_t aX)
{
    return acos(aX);
}


ACP_EXPORT acp_rc_t acpMathAsin(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast arc sine function
 */
ACP_INLINE acp_double_t acpMathFastAsin(acp_double_t aX)
{
    return asin(aX);
}


ACP_EXPORT acp_rc_t acpMathAtan(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast arc tangent function
 */
ACP_INLINE acp_double_t acpMathFastAtan(acp_double_t aX)
{
    return atan(aX);
}


ACP_EXPORT acp_rc_t acpMathAtan2(acp_double_t aY,
                                 acp_double_t aX,
                                 acp_double_t *aResult);

/**
 * Fast arc tangent function of two variables
 */
ACP_INLINE acp_double_t acpMathFastAtan2(acp_double_t aY, acp_double_t aX)
{
    return atan2(aY, aX);
}


ACP_EXPORT acp_rc_t acpMathCos(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast cosine function
 */
ACP_INLINE acp_double_t acpMathFastCos(acp_double_t aX)
{
    return cos(aX);
}


ACP_EXPORT acp_rc_t acpMathSin(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast sine function
 */
ACP_INLINE acp_double_t acpMathFastSin(acp_double_t aX)
{
    return sin(aX);
}


ACP_EXPORT acp_rc_t acpMathTan(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast tangent function
 */
ACP_INLINE acp_double_t acpMathFastTan(acp_double_t aX)
{
    return tan(aX);
}


/*
 * hyperbolic functions
 */

ACP_EXPORT acp_rc_t acpMathCosh(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast hyperbolic cosine function
 */
ACP_INLINE acp_double_t acpMathFastCosh(acp_double_t aX)
{
    return cosh(aX);
}

ACP_EXPORT acp_rc_t acpMathSinh(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast hyperbolic sine function
 */
ACP_INLINE acp_double_t acpMathFastSinh(acp_double_t aX)
{
    return sinh(aX);
}

ACP_EXPORT acp_rc_t acpMathTanh(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast hyperbolic tangent function
 */
ACP_INLINE acp_double_t acpMathFastTanh(acp_double_t aX)
{
    return tanh(aX);
}


/*
 * exponential and logarithmic functions
 */

ACP_EXPORT acp_rc_t acpMathExp(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast base-e exponential function
 */
ACP_INLINE acp_double_t acpMathFastExp(acp_double_t aX)
{
    return exp(aX);
}


ACP_EXPORT acp_rc_t acpMathFrexp(acp_double_t aX,
                                 acp_sint32_t *aExp,
                                 acp_double_t *aResult);

/**
 * Fast convert floating-point number to fractional and integral components
 */
ACP_INLINE acp_double_t acpMathFastFrexp(acp_double_t aX, acp_sint32_t *aExp)
{
    return frexp(aX, aExp);
}


ACP_EXPORT acp_rc_t acpMathLdexp(acp_double_t aX,
                                 acp_sint32_t aExp,
                                 acp_double_t *aResult);

/**
 * Fast multiply floating-point number by integral power of 2
 */
ACP_INLINE acp_double_t acpMathFastLdexp(acp_double_t aX, acp_sint32_t aExp)
{
    return ldexp(aX, aExp);
}


ACP_EXPORT acp_rc_t acpMathLog(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast natural logarithmic function
 */
ACP_INLINE acp_double_t acpMathFastLog(acp_double_t aX)
{
    return log(aX);
}



ACP_EXPORT acp_rc_t acpMathLog10(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast base-10 logarithmic function
 */
ACP_INLINE acp_double_t acpMathFastLog10(acp_double_t aX)
{
    return log10(aX);
}


ACP_EXPORT acp_rc_t acpMathModf(acp_double_t aX,
                                acp_double_t *aInt,
                                acp_double_t *aResult);

/**
 * Fast extract signed integral and fractional values from floating-point number
 */
ACP_INLINE acp_double_t acpMathFastModf(acp_double_t aX, acp_double_t *aInt)
{
    return modf(aX, aInt);
}


/*
 * power functions
 */

ACP_EXPORT acp_rc_t acpMathPow(acp_double_t aX,
                               acp_double_t aY,
                               acp_double_t *aResult);

/**
 * Fast power function
 */
ACP_INLINE acp_double_t acpMathFastPow(acp_double_t aX, acp_double_t aY)
{
    return pow(aX, aY);
}


ACP_EXPORT acp_rc_t acpMathSqrt(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast square root function
 */
ACP_INLINE acp_double_t acpMathFastSqrt(acp_double_t aX)
{
    return sqrt(aX);
}

/*
 * nearest integr, absolute value, and remainder functions
 */

ACP_EXPORT acp_rc_t acpMathCeil(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast ceiling function: smallest integral value not less than argument
 */
ACP_INLINE acp_double_t acpMathFastCeil(acp_double_t aX)
{
    return ceil(aX);
}


ACP_EXPORT acp_rc_t acpMathFabs(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast absolute value of floating-point number
 */
ACP_INLINE acp_double_t acpMathFastFabs(acp_double_t aX)
{
    return fabs(aX);
}


ACP_EXPORT acp_rc_t acpMathFloor(acp_double_t aX, acp_double_t *aResult);

/**
 * Fast largest integral value not greater than argument
 */
ACP_INLINE acp_double_t acpMathFastFloor(acp_double_t aX)
{
    return floor(aX);
}


ACP_EXPORT acp_rc_t acpMathFmod(acp_double_t aX,
                                acp_double_t aY,
                                acp_double_t *aResult);

/**
 * Fast floating-point remainder function
 */
ACP_INLINE acp_double_t acpMathFastFmod(acp_double_t aX, acp_double_t aY)
{
    return fmod(aX, aY);
}



/*
 * floating point classification functions
 */
/**
 * returns non-zero if the value is finite
 */
ACP_INLINE acp_bool_t acpMathIsFinite(acp_double_t aX)
{
#if defined(ALTI_CFG_OS_HPUX)
/* Some version of HP-UX does not support C99 including isfinite(). */
# if (ALTI_CFG_OS_MAJOR == 11) && (ALTI_CFG_OS_MINOR <= 11)
    return isfinite(aX) != 0 ? ACP_TRUE : ACP_FALSE;
# elif defined(ALTI_CFG_CPU_PARISC)
    return isfinite(aX) != 0 ? ACP_TRUE : ACP_FALSE;
# else
    extern acp_sint32_t finite(acp_double_t);
    return finite(aX) != 0 ? ACP_TRUE : ACP_FALSE;
# endif
#elif defined(ALTI_CFG_OS_WINDOWS)
    return _finite(aX) != 0 ? ACP_TRUE : ACP_FALSE;
#else
    return finite(aX) != 0 ? ACP_TRUE : ACP_FALSE;
#endif
}

/**
 * returns non-zero if the value is infinite
 */
ACP_INLINE acp_bool_t acpMathIsInf(acp_double_t aX)
{
#if defined(ALTI_CFG_OS_SOLARIS)
    return (((fpclass(aX) == FP_NINF) || (fpclass(aX) == FP_PINF))) ?
        ACP_TRUE : ACP_FALSE;
#elif defined(ALTI_CFG_OS_TRU64)
    return (((fp_class(aX) == FP_NEG_INF) || (fp_class(aX) == FP_POS_INF))) ?
        ACP_TRUE : ACP_FALSE;
#elif defined(ALTI_CFG_OS_WINDOWS)
    return (((_fpclass(aX) == _FPCLASS_NINF)
             || (_fpclass(aX) == _FPCLASS_PINF))
            ) ? ACP_TRUE : ACP_FALSE;
#else
    return isinf(aX) != 0 ? ACP_TRUE : ACP_FALSE;
#endif
}

/**
 * returns non-zero if the value is not a number
 */
ACP_INLINE acp_bool_t acpMathIsNan(acp_double_t aX)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    return _isnan(aX) != 0 ? ACP_TRUE : ACP_FALSE;
#else
    return isnan(aX) != 0 ? ACP_TRUE : ACP_FALSE;
#endif
}


ACP_EXTERN_C_END


#endif
