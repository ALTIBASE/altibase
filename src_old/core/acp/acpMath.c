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
 * $Id: acpMath.c 8994 2009-12-03 06:14:02Z gurugio $
 ******************************************************************************/

#include <acpMath.h>

/*
 * All math functions are following C89 standard, not C99.
 * Because too many platforms/compilers do not support C99.
 */

/**
 * Get error code from Math library
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_ERANGE divide by zero or overflow or
 *         underflow exceptions occur
 * @return #ACP_RC_SUCCESS successful
 */
ACP_EXPORT acp_rc_t acpMathGetException(void)
{
    acp_rc_t sRC;

    /* overflow, underflow, out of range */
    if (errno == ERANGE)
    {
        sRC = ACP_RC_ERANGE;
    }
    /* domain error (argument value falls outsize the domain) */
    else if (errno == EDOM)
    {
        sRC = ACP_RC_EINVAL;
    }
    else
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    return sRC;
}


/**
 * arc cosine function
 * 
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_SUCCESS successful
 *
 * If x is outside the range [-1, 1], a  domain  error
 * occurs, and a NaN is returned.
 */
ACP_EXPORT acp_rc_t acpMathAcos(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = acos(aX);

    return acpMathGetException();
}

/**
 * arc sine function
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_SUCCESS successful
 *
 * If x is outside the range [-1, 1], a domain error
 * occurs, and a NaN is returned.
 */
ACP_EXPORT acp_rc_t acpMathAsin(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = asin(aX);

    return acpMathGetException();
}

/**
 * arc tangent function
 *
 * @return always #ACP_RC_SUCCESS successful
 */

ACP_EXPORT acp_rc_t acpMathAtan(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = atan(aX);

    /* no errors occur */
    return ACP_RC_GET_OS_ERROR();
}

/**
 * arc tangent function of two variables
 *
 * @return always #ACP_RC_SUCCESS successful
 */
ACP_EXPORT acp_rc_t acpMathAtan2(acp_double_t aY,
                                 acp_double_t aX,
                                 acp_double_t *aResult)
{
    errno = 0;
    *aResult = atan2(aY, aX);

    /* no errors occur */
    return ACP_RC_GET_OS_ERROR();
}

/**
 * cosine function
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_SUCCESS successful
 *
 * If x is positive infinity or negative infinity,
 * a domain error occurs,  and a NaN is returned.
 */
ACP_EXPORT acp_rc_t acpMathCos(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = cos(aX);
    
    return acpMathGetException();
}

/**
 * sine function
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_SUCCESS successful
 *
 * If x is positive infinity or negative infinity,
 * a domain error occurs,  and a NaN is returned.
 */
ACP_EXPORT acp_rc_t acpMathSin(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = sin(aX);

    return acpMathGetException();
}

/**
 * tangent function
 *
 * @return #ACP_RC_ERANGE divide by zero or overflow or
 *         underflow exceptions occur
 * @return #ACP_RC_SUCCESS successful
 *
 * If  the  correct result would overflow, a range error
 * occurs, and the functions return HUGE_VAL, HUGE_VALF,
 * or  HUGE_VALL,  respectively,  with  the
 * mathematically correct sign.
 */
ACP_EXPORT acp_rc_t acpMathTan(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = tan(aX);

    return acpMathGetException();
}


/**
 * hyperbolic cosine function
 *
 * @return #ACP_RC_ERANGE divide by zero or overflow or
 *         underflow exceptions occur
 * @return #ACP_RC_SUCCESS successful
 *
 * If the result overflows, a range error occurs,
 * and  the  functions  return +HUGE_VAL, +HUGE_VALF,
 * or +HUGE_VALL, respectively.
 */
ACP_EXPORT acp_rc_t acpMathCosh(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = cosh(aX);

    return acpMathGetException();
}

/**
 * hyperbolic sine function
 *
 * @return #ACP_RC_ERANGE divide by zero or overflow or
 *         underflow exceptions occur
 * @return #ACP_RC_SUCCESS successful
 *
 * If the result overflows, a range error occurs,
 * and  the  functions  return HUGE_VAL, HUGE_VALF,
 * or HUGE_VALL, respectively, with the same sign as x.
 */
ACP_EXPORT acp_rc_t acpMathSinh(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = sinh(aX);

    return acpMathGetException();
}

/**
 * hyperbolic tangent function
 *
 * @return always #ACP_RC_SUCCESS successful
 */
ACP_EXPORT acp_rc_t acpMathTanh(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = tanh(aX);

    return ACP_RC_GET_OS_ERROR();
}


/**
 * base-e eponential function
 *
 * @return #ACP_RC_ERANGE divide by zero or overflow or
 *         underflow exceptions occur
 * @return #ACP_RC_SUCCESS successful
 *
 * If  the  result  overflows,  a range error occurs,
 * and the functions return +HUGE_VAL, +HUGE_VALF,
 * or +HUGE_VALL, respectively.
 */
ACP_EXPORT acp_rc_t acpMathExp(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = exp(aX);

    return acpMathGetException();
}


/**
 * convert floating-point number to fractional and
 * integral components
 *
 * @return always #ACP_RC_SUCCESS successful
 */
ACP_EXPORT acp_rc_t acpMathFrexp(acp_double_t aX,
                                 acp_sint32_t *aExp,
                                 acp_double_t *aResult)
{
    errno = 0;
    *aResult = frexp(aX, aExp);
    return ACP_RC_GET_OS_ERROR();
}


/**
 * multiply floating-point number by integral power of 2
 *
 * @return #ACP_RC_ERANGE divide by zero or overflow or
 *         underflow exceptions occur
 * @return #ACP_RC_SUCCESS successful
 *
 * If  the  result  overflows,  a range error occurs,
 * and the functions return HUGE_VAL, HUGE_VALF, or
 * HUGE_VALL, respectively, with a sign the same as x.
 */
ACP_EXPORT acp_rc_t acpMathLdexp(acp_double_t aX,
                                 acp_sint32_t aExp,
                                 acp_double_t *aResult)
{
    errno = 0;
    *aResult = ldexp(aX, aExp);

    return acpMathGetException();
}


/**
 * natural logarithmic function
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_SUCCESS successful
 *
 * If x is negative (including negative infinity),
 * then a domain error occurs, and a NaN (not a number)
 * is returned.
 */
ACP_EXPORT acp_rc_t acpMathLog(acp_double_t aX,
                               acp_double_t *aResult)
{
    errno = 0;
    *aResult = log(aX);

    return acpMathGetException();
}


/**
 * base-10 logarithmic function
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_SUCCESS successful
 *
 * If x is negative (including negative infinity),
 * then a domain error occurs, and a NaN (not a number)
 * is returned.
 */
ACP_EXPORT acp_rc_t acpMathLog10(acp_double_t aX,
                                 acp_double_t *aResult)
{
    errno = 0;
    *aResult = log10(aX);

    return acpMathGetException();
}


/**
 * extract signed integral and fractional values from
 * floating-point number
 *
 * @return always #ACP_RC_SUCCESS successful
 */
ACP_EXPORT acp_rc_t acpMathModf(acp_double_t aX,
                                acp_double_t *aInt,
                                acp_double_t *aResult)
{
    errno = 0;
    *aResult = modf(aX, aInt);
    return ACP_RC_GET_OS_ERROR();
}


/**
 * power function
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_ERANGE divide by zero or overflow or
 *         underflow exceptions occur
 * @return #ACP_RC_SUCCESS successful
 *
 * Domain error: x is negative, and y is a finite
 * Pole error: x is zero, and y is negative
 * Range error: the result overflows
 * Range error: the result underflows
 */
ACP_EXPORT acp_rc_t acpMathPow(acp_double_t aX,
                               acp_double_t aY,
                               acp_double_t *aResult)
{
    errno = 0;
    *aResult = pow(aX, aY);

    return acpMathGetException();
}


/**
 * square root function
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_SUCCESS successful
 * 
 * If x is less than -0, a domain error occurs,
 * and a NaN is returned.
 */
ACP_EXPORT acp_rc_t acpMathSqrt(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = sqrt(aX);

    return acpMathGetException();
}

/**
 * ceiling function: smallest integral value not less
 * than argument
 *
 * @return always #ACP_RC_SUCCESS successful
 */
ACP_EXPORT acp_rc_t acpMathCeil(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = ceil(aX);
    return ACP_RC_GET_OS_ERROR();
}

/**
 * absolute value of floating-point number
 *
 * @return always #ACP_RC_SUCCESS successful
 */
ACP_EXPORT acp_rc_t acpMathFabs(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = fabs(aX);
    return ACP_RC_GET_OS_ERROR();
}

/**
 * largest integral value not greater than argument
 *
 * @return always #ACP_RC_SUCCESS successful
 */
ACP_EXPORT acp_rc_t acpMathFloor(acp_double_t aX, acp_double_t *aResult)
{
    errno = 0;
    *aResult = floor(aX);
    return ACP_RC_GET_OS_ERROR();
}

/**
 * floating-point remainder function
 *
 * @return #ACP_RC_EINVAL domain exception occurs
 * @return #ACP_RC_SUCCESS successful
 *
 * If x is an infinity, a domain error occurs, and a NaN
 * is returned. If y is zero, a domain error occurs,
 * and a NaN is returned.
 */
ACP_EXPORT acp_rc_t acpMathFmod(acp_double_t aX,
                                acp_double_t aY,
                                acp_double_t *aResult)
{
    errno = 0;
    *aResult = fmod(aX, aY);
    
    return acpMathGetException();    
}

