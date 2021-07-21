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
 * $Id: acpSleep.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_SLEEP_H_)
#define _O_ACP_SLEEP_H_

/**
 * @file
 * @ingroup CoreThr
 * @ingroup CoreProc
 */

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN


/**
 * sleeps @a aSec second(s)
 *
 * @param aSec second(s) to sleep
 */
ACP_INLINE void acpSleepSec(acp_uint32_t aSec)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    Sleep(aSec * 1000);
#else
    struct timeval sTimeVal;

    sTimeVal.tv_sec  = aSec;
    sTimeVal.tv_usec = 0;

    (void)select(0, NULL, NULL, NULL, &sTimeVal);
#endif
}

/**
 * sleeps @a aMsec millisecond(s)
 *
 * @param aMsec millisecond(s) to sleep
 */
ACP_INLINE void acpSleepMsec(acp_uint32_t aMsec)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    Sleep(aMsec);
#else
    struct timeval sTimeVal;

    sTimeVal.tv_sec  = aMsec / 1000;
    sTimeVal.tv_usec = (aMsec % 1000) * 1000;

    (void)select(0, NULL, NULL, NULL, &sTimeVal);
#endif
}

/**
 * sleeps @a aUsec microsecond(s)
 *
 * @param aUsec microsecond(s) to sleep
 */
ACP_INLINE void acpSleepUsec(acp_uint32_t aUsec)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    DWORD sMsec = aUsec / 1000;

    if (sMsec == 0)
    {
        Sleep(1);
    }
    else
    {
        Sleep(sMsec);
    }
#else
    struct timeval sTimeVal;

    sTimeVal.tv_sec  = aUsec / 1000000;
    sTimeVal.tv_usec = aUsec % 1000000;

    (void)select(0, NULL, NULL, NULL, &sTimeVal);
#endif
}


ACP_EXTERN_C_END


#endif
