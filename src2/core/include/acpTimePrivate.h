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
 * $Id: acpTimePrivate.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_TIME_PRIVATE_H_)
#define _O_ACP_TIME_PRIVATE_H_

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN


#if defined(ALTI_CFG_OS_WINDOWS)

/*
 * Number of micro-seconds between the beginning of the Windows epoch
 * (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970)
 */
#define ACP_TIME_DELTA_EPOCH_IN_USEC ACP_SINT64_LITERAL(11644473600000000)


ACP_INLINE acp_time_t acpTimeFromRelFileTime(FILETIME *aFileTime)
{
    acp_time_t sTime;

    /*
     * convert FILETIME one 64 bit number so we can work with it.
     */
    sTime   = aFileTime->dwHighDateTime;
    sTime <<= 32;
    sTime  |= aFileTime->dwLowDateTime;

    /*
     * convert unit from 100 nano-seconds to micro-seconds.
     */
    sTime  /= 10;

    return sTime;
}

ACP_INLINE acp_time_t acpTimeFromAbsFileTime(FILETIME *aFileTime)
{
    acp_time_t sTime = acpTimeFromRelFileTime(aFileTime);

    /*
     * Convert from Windows epoch to Unix epoch
     */
    sTime -= ACP_TIME_DELTA_EPOCH_IN_USEC;

    return sTime;
}

#endif


ACP_EXTERN_C_END


#endif
