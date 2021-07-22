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
 * $Id: acpRand.h 11193 2010-06-08 01:28:43Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_RAND_H_)
#define _O_ACP_RAND_H_

/**
 * @file
 * @ingroup CoreMath
 */

#include <acpTypes.h>
#include <acpTime.h>

ACP_EXTERN_C_BEGIN

/**
 * returns a psuedo-random integer
 *
 * @param aNextSeed pointer to store next seed number
 * @return a psuedo-random integer
 */
ACP_INLINE acp_sint32_t acpRand(acp_uint32_t* aNextSeed)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_sint64_t sRand = (acp_sint64_t)(*aNextSeed);
    sRand = 16807 * (sRand % 127773) - 2836 * (sRand / 127773);
    sRand = (sRand > 0)? sRand : sRand + ACP_SINT32_MAX;
    *aNextSeed = ((acp_uint32_t)sRand) & ACP_SINT32_MAX;
    return (acp_sint32_t)(*aNextSeed);
#else
    return rand_r(aNextSeed);
#endif
}

/**
 * generate an initial seed to generate pseudo-random numbers.
 * This seed is used only the first time of calling #acpRand.
 * The next seeds should be initialized by #acpRand().
 *
 * @return seed number for random number generator
 */
ACP_INLINE acp_uint32_t acpRandSeedAuto(void)
{
    return (acp_uint32_t)acpTimeNow();
}

ACP_EXTERN_C_END

#endif
