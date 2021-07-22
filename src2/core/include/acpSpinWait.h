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
 * $Id: acpSpinWait.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_SPIN_WAIT_H_)
#define _O_ACP_SPIN_WAIT_H_

/**
 * @file
 * @ingroup CoreAtomic
 * @ingroup CoreThr
 */

#include <acpSleep.h>


ACP_EXTERN_C_BEGIN


#define ACP_SPIN_WAIT_DEFAULT_SPIN_COUNT 1000

#define ACP_SPIN_WAIT_SLEEP_TIME_MIN            200
#define ACP_SPIN_WAIT_SLEEP_TIME_MAX            100000


ACP_EXPORT acp_sint32_t acpSpinWaitGetDefaultSpinCount(void);
ACP_EXPORT void         acpSpinWaitSetDefaultSpinCount(acp_sint32_t aSpinCount);


/**
 * spin wait until the expression is true.
 * @param aExpr the expression
 * @param aSpinCount initial spin count
 *
 * after spinnig @a aSpinCount times,
 * it will start to sleep from 200us up to 100ms
 *
 * if @a aSpinCount is less than zero,
 * it will be the default spin count;
 * 1000 in multiprocessor system, 0 in uniprocessor system.
 */
#define ACP_SPIN_WAIT(aExpr, aSpinCount)                                    \
    do                                                                      \
    {                                                                       \
        acp_sint32_t sSpinLoop_MACRO_LOCAL_VAR;                             \
        acp_sint32_t sSpinCount_MACRO_LOCAL_VAR;                            \
        acp_uint32_t sSpinSleepTime_MACRO_LOCAL_VAR;                        \
                                                                            \
        if ((aSpinCount) < 0)                                               \
        {                                                                   \
            sSpinCount_MACRO_LOCAL_VAR = acpSpinWaitGetDefaultSpinCount();  \
        }                                                                   \
        else                                                                \
        {                                                                   \
            sSpinCount_MACRO_LOCAL_VAR = (aSpinCount);                      \
        }                                                                   \
                                                                            \
        for (sSpinLoop_MACRO_LOCAL_VAR = 0;                                 \
             sSpinLoop_MACRO_LOCAL_VAR < sSpinCount_MACRO_LOCAL_VAR;        \
             sSpinLoop_MACRO_LOCAL_VAR++)                                   \
        {                                                                   \
            if (aExpr)                                                      \
            {                                                               \
                break;                                                      \
            }                                                               \
            else                                                            \
            {                                                               \
            }                                                               \
        }                                                                   \
                                                                            \
        if (sSpinLoop_MACRO_LOCAL_VAR >= sSpinCount_MACRO_LOCAL_VAR)        \
        {                                                                   \
            sSpinSleepTime_MACRO_LOCAL_VAR = ACP_SPIN_WAIT_SLEEP_TIME_MIN;  \
                                                                            \
            while (!(aExpr))                                                \
            {                                                               \
                acpSleepUsec(sSpinSleepTime_MACRO_LOCAL_VAR);               \
                                                                            \
                if (sSpinSleepTime_MACRO_LOCAL_VAR >=                       \
                    (ACP_SPIN_WAIT_SLEEP_TIME_MAX / 2))                     \
                {                                                           \
                    sSpinSleepTime_MACRO_LOCAL_VAR =                        \
                        ACP_SPIN_WAIT_SLEEP_TIME_MAX;                       \
                }                                                           \
                else                                                        \
                {                                                           \
                    sSpinSleepTime_MACRO_LOCAL_VAR *= 2;                    \
                }                                                           \
            }                                                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
        }                                                                   \
    } while (0)


ACP_EXTERN_C_END


#endif
