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
 * $Id$
 ******************************************************************************/
#include <idl.h>
#include <ideErrorMgr.h>
#include <ideMsgLog.h>
#include <idErrorCode.h>
#include <iduSessionEvent.h>
#include <iduShmDef.h>
#include <idrLogMgr.h>
#include <iduShmLatch.h>
#include <iduVLogShmLatch.h>
#include <iduSXLatch.h>
#include <iduProperty.h>

void iduSXLatchAcquire( iduSXLatch     * aSXLatch,
                        iduSXLatchMode   aMode )
{
    SInt                sSpinLoop;
    SInt                sSpinCount;
    SInt                sVal;
    SInt                sLatchValue  = 0x00000001;
    idBool              sIsLock      = ID_FALSE;

    if( aSXLatch->mSpinCount == 0 )
    {
        sSpinCount = IDU_SPIN_WAIT_DEFAULT_SPIN_COUNT;
    }
    else
    {
        sSpinCount = aSXLatch->mSpinCount;
    }

    while(1)
    {
        for( sSpinLoop = 0;
             sSpinLoop <= sSpinCount; /* sSpinCount can be 0 in UP! */
             sSpinLoop++ )
        {
            /* for cache hit, more frequency case is first */
            if( aSXLatch->mLock == 0 )
            {
                sVal = idCore::acpAtomicCas32( &aSXLatch->mLock,
                                               sLatchValue,
                                               0/*Compare Value*/ );

                if( sVal == 0 )
                {
                    ACP_MEM_BARRIER();

                    sIsLock = ID_TRUE;

                    break;
                }
            }
        }

        if( sIsLock == ID_TRUE )
        {
            break;
        }


        idlOS::thr_yield();
    }


    if( aMode == IDU_SX_LATCH_MODE_SHARED )
    {
        aSXLatch->mSharedLatchCnt++;
        ACP_MEM_BARRIER();
        (void)idCore::acpAtomicSet32( &aSXLatch->mLock, 0 );
    }
}

void iduSXLatchRelease( iduSXLatch * aSXLatch )
{
    if( aSXLatch->mSharedLatchCnt != 0 )
    {
        iduSXLatchAcquire( aSXLatch, IDU_SX_LATCH_MODE_EXCLUSIVE );

        if( aSXLatch->mSharedLatchCnt != 0 )
        {
            aSXLatch->mSharedLatchCnt--;
        }
    }

    ACP_MEM_BARRIER();
    (void)idCore::acpAtomicSet32( &aSXLatch->mLock, 0 );
}
