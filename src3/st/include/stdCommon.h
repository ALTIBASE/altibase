/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: stdCommon.h,v 0.01 2006/04/19 11:42:57 hoonkim Exp $
 **********************************************************************/

#ifndef _O_STD_COMMON_H_
#define _O_STD_COMMON_H_        1

#include <iduMemory.h>

#ifndef _PIE_DEFINE
#define _PIE_DEFINE
    #define _RADIAN_PER_DEGREE  0.0174532925199432957692369076848833
    #define _DEGREE_PER_RADIAN  57.2957795130823208767981548170142
    #define _PIE        3.14159265358979323846264338327950288419716939937510
    #define _HALF_PIE           1.5707963267948966192313216916395
    #define _TWO_PIE            6.283185307179586476925286766558
#endif

#ifndef MATH_ERROR_TOL
#define MATH_ERROR_TOL
    #define  MATH_TOL           0.0000001       // normal tollerance
    #define  MATH_TOL_SQ        0.0000001       // square root tollerance
    #define  MATH_TOL_DIST      MATH_TOL_SQ     // square root tollerance
    #define  MATH_TOL2          1.0e-16         // square tollerance
    #define  MATH_TOL0          1.0e-8          // is zero tollerance
    #define  MATH_TOL1_00       1.00000001      // 
    #define  MATH_TOL0_99       0.99999999      // 
#endif

#if !defined(WRS_VXWORKS)
#define min( a, b ) ( ( (a) > (b) ) ? (b) : (a) )
#define max( a, b ) ( ( (a) > (b) ) ? (a) : (b) )
#endif

#define STD_DATE_IS_NULL( d ) \
            ( ((d)->year          == stdDateNull.year)  && \
              ((d)->mon_day_hour  == stdDateNull.mon_day_hour) && \
              ((d)->min_sec_mic   == stdDateNull.min_sec_mic) )

#define STD_MATH_TOL_SQ (1.0e-50)
#define STD_MATH_TOL    (1.0e-20)

#endif /* _O_STD_COMMON_H_ */

