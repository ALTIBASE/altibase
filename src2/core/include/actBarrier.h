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
 * $Id: actBarrier.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACT_BARRIER_H_)
#define _O_ACT_BARRIER_H_

/**
 * @file
 * @ingroup CoreUnit
 *
 * wrapper for #acp_thr_barrier_t
 */

#include <acpThrBarrier.h>


ACP_EXTERN_C_BEGIN


/**
 * thread execution barrier
 */
typedef acp_thr_barrier_t act_barrier_t;


/**
 * creates a thread barrier
 * @param aBarrier pointer to the #act_barrier_t
 */
#define ACT_BARRIER_CREATE(aBarrier)                                        \
    do                                                                      \
    {                                                                       \
        acp_rc_t sActBarrierRC_MACRO_LOCAL_VAR =                            \
            acpThrBarrierCreate(aBarrier);                                  \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sActBarrierRC_MACRO_LOCAL_VAR),    \
                       ("actBarrierCreate failed with return code %d",      \
                        sActBarrierRC_MACRO_LOCAL_VAR));                    \
    } while (0)

/**
 * destroys a thread barrier
 * @param aBarrier pointer to the #act_barrier_t
 */
#define ACT_BARRIER_DESTROY(aBarrier)                                          \
    do                                                                         \
    {                                                                          \
        acp_rc_t sActBarrierRC_MACRO_LOCAL_VAR =                            \
            acpThrBarrierDestroy(aBarrier);                                 \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sActBarrierRC_MACRO_LOCAL_VAR),    \
                       ("actBarrierDestroy failed with return code %d",     \
                        sActBarrierRC_MACRO_LOCAL_VAR));                    \
    } while (0)

/**
 * initializes the counter of the thread barrier to zero
 * @param aBarrier pointer to the #act_barrier_t
 */
#define ACT_BARRIER_INIT(aBarrier) acpThrBarrierInit(aBarrier)

/**
 * increases the counter of the thread barrier and broadcasts to the others
 * @param aBarrier pointer to the #act_barrier_t
 */
#define ACT_BARRIER_TOUCH(aBarrier)                                         \
    do                                                                      \
    {                                                                       \
        acp_rc_t sActBarrierRC_MACRO_LOCAL_VAR =                            \
            acpThrBarrierTouch(aBarrier);                                   \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sActBarrierRC_MACRO_LOCAL_VAR),    \
                       ("actBarrierTouch failed with return code %d",       \
                        sActBarrierRC_MACRO_LOCAL_VAR));                    \
    } while (0)

/**
 * waits for the others to touch the counter of thread barrier
 * @param aBarrier pointer to the #act_barrier_t
 * @param aCounter counter to wait
 */
#define ACT_BARRIER_WAIT(aBarrier, aCounter)                                \
    do                                                                      \
    {                                                                       \
        acp_rc_t sActBarrierRC_MACRO_LOCAL_VAR = acpThrBarrierWait(aBarrier, \
                                                                   aCounter); \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sActBarrierRC_MACRO_LOCAL_VAR),    \
                       ("actBarrierWait failed with return code %d",        \
                        sActBarrierRC_MACRO_LOCAL_VAR));                    \
    } while (0)

/**
 * touches and waits for the thread barrier
 * @param aBarrier pointer to the #act_barrier_t
 * @param aCounter counter
 */
#define ACT_BARRIER_TOUCH_AND_WAIT(aBarrier, aCounter)                      \
    do                                                                      \
    {                                                                       \
        acp_rc_t sActBarrierRC_MACRO_LOCAL_VAR =                            \
            acpThrBarrierTouchAndWait(aBarrier,                             \
                                      aCounter);                            \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sActBarrierRC_MACRO_LOCAL_VAR),    \
                       ("actBarrierTouchAndWait failed with return code %d", \
                        sActBarrierRC_MACRO_LOCAL_VAR));                    \
    } while (0)


ACP_EXTERN_C_END


#endif
