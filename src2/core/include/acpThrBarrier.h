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
 * $Id: acpThrBarrier.h 11192 2010-06-08 01:24:21Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_THR_BARRIER_H_)
#define _O_ACP_THR_BARRIER_H_

/**
 * @file
 * @ingroup CoreThr
 */

#include <acpMemBarrier.h>
#include <acpThrCond.h>
#include <acpThrMutex.h>


ACP_EXTERN_C_BEGIN

#define ACP_THR_BARRIER_MAGIC (acp_uint32_t)(0xAA5555AA)


/**
 * thread barrier
 */
typedef struct acp_thr_barrier_t
{
    acp_uint32_t          mCreated;
    acp_thr_cond_t        mCond;
    acp_thr_mutex_t       mMutex;
    volatile acp_uint32_t mCount;
} acp_thr_barrier_t;


ACP_EXPORT acp_rc_t acpThrBarrierCreate(acp_thr_barrier_t *aBarrier);
ACP_EXPORT acp_rc_t acpThrBarrierDestroy(acp_thr_barrier_t *aBarrier);

/**
 * initializes the counter of the thread barrier to zero
 * @param aBarrier pointer to the #acp_thr_barrier_t
 */
ACP_INLINE void acpThrBarrierInit(acp_thr_barrier_t *aBarrier)
{
    aBarrier->mCount = 0;
}

ACP_EXPORT acp_rc_t acpThrBarrierTouch(acp_thr_barrier_t *aBarrier);
ACP_EXPORT acp_rc_t acpThrBarrierWait(acp_thr_barrier_t *aBarrier,
                                      acp_uint32_t       aCounter);

/**
 * touches and waits for the thread barrier
 * @param aBarrier pointer to the #acp_thr_barrier_t
 * @param aCounter counter to wait
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrBarrierTouchAndWait(acp_thr_barrier_t *aBarrier,
                                              acp_uint32_t       aCounter)
{
    acp_rc_t sRC;

    sRC = acpThrBarrierTouch(aBarrier);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        return acpThrBarrierWait(aBarrier, aCounter);
    }
    else
    {
        return sRC;
    }
}


ACP_EXTERN_C_END


#endif
