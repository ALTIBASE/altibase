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
 * $Id: testAcpMath.c 9058 2009-12-07 01:57:31Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpMath.h>


/*
 * BUG-27543
 * Failure test cases are not available,
 * because every platform has different floating register size.
 * A certain value is out of range in A platform,
 * but B platform can process the value.
 *
 * And some platforms support C89, others support C90 or C99.
 * Even though one platform check a return value is NAN,
 * another platform check it right.
 */

acp_bool_t testCmpDoubles(acp_double_t aX, acp_double_t aY)
{
    if (acpMathFastFabs(aX - aY) < 0.0001)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

acp_sint32_t main(void)
{
    acp_rc_t sRC;
    
    acp_sint32_t sInt;
    acp_sint32_t sIntFast;
    acp_double_t sDouble;
    acp_double_t sDoubleFast;
    
    acp_double_t sResult;
    acp_double_t sCmp;
    
    ACT_TEST_BEGIN();

    /*
     * trigonometric functions
     */


    /* log is used to check infinite number and non-number.
     * log(0) -> infinite number
     * log(-1) -> not a number
     */

    /* Do NEVER! compare result values of math function.
     */

    /* success cases */
    
    sRC = acpMathAcos(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastAcos(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));
    
    sRC = acpMathAsin(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastAsin(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathAtan(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastAtan(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));
    
    sRC = acpMathAtan2(1.0, 1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastAtan2(1.0, 1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathCos(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastCos(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathSin(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastSin(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathTan(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastTan(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));


    /* atan, atan2 do not occur error */

    /* failing test for cos,sin,tan are not implemented
     * because of lack of C99 support of DEC/AIX platform. */

    /*
     * hyperbolic functions
     */

    /* success cases */
    
    sRC = acpMathCosh(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastCosh(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));
    
    sRC = acpMathSinh(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastSinh(1.0);    
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathTanh(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastTanh(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));


    /*
     * exponential and logarithmic functions
     */

    /* success cases */
    
    sRC = acpMathExp(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastExp(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathFrexp(1.0, &sInt, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastFrexp(1.0, &sIntFast);
    ACT_CHECK(sInt == sIntFast);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));


    sRC = acpMathLdexp(1.0, 1, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastLdexp(1.0, 1);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathLog(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastLog(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathLog10(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastLog10(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathModf(1.0, &sDouble, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastModf(1.0, &sDoubleFast);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));
    ACT_CHECK(testCmpDoubles(sDouble, sDoubleFast));


    /*
     * power functions
     */

    /* success cases */
    
    sRC = acpMathPow(1.0, 1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastPow(1.0, 1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));
    
    sRC = acpMathSqrt(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastSqrt(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));
    

    /* failure case for sqrt is removed,
     * because DEC and WINDOWS do not support error handling standard.
     */

    /*
     * nearest integr, absolute value, and remainder functions
     */

    /* success cases */
    
    sRC = acpMathCeil(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastCeil(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathFabs(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastFabs(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathFloor(1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastFloor(1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));

    sRC = acpMathFmod(1.0, 1.0, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sCmp = acpMathFastFmod(1.0, 1.0);
    ACT_CHECK(testCmpDoubles(sResult, sCmp));
    


    /*
     * floating point classification functions
     */
    

    /* failing test for IsFinite and IsInf are not implemented
     * because of lack of C99 support of DEC/AIX platform. */
    ACT_CHECK(acpMathIsFinite(1.0) == ACP_TRUE);
    ACT_CHECK(acpMathIsInf(1.0) == ACP_FALSE);

    ACT_CHECK(acpMathIsNan(1.0) == ACP_FALSE);

    ACT_TEST_END();

    return 0;
}
