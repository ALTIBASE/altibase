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

#include <mmtLoadBalancer.h>
#include <mmtThreadManager.h>
#include <mmuProperty.h>
#include <mmtSessionManager.h>

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
 	 Load Balancing module must be separated from the session manager thread.  
*/

mmtLoadBalancer::mmtLoadBalancer() : idtBaseThread()
{
    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    initialize();
}

IDE_RC mmtLoadBalancer::initialize() {
    if ( mmuProperty::getIsDedicatedMode() == 1 )
    {
        IDE_TEST(mMutexForStop.initialize((SChar *)"MMT_LOAD_BALANCER_MUTEX_FOR_STOP",
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
        IDE_TEST(mLoadBalancerCV.initialize() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtLoadBalancer::finalize()
{
     /* PROJ-2108 Dedicated thread mode which uses less CPU */
    if ( mmuProperty::getIsDedicatedMode() == 1 )
    {
        IDE_TEST(mMutexForStop.destroy() != IDE_SUCCESS);
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void  mmtLoadBalancer::run()
{
    PDL_Time_Value sSleepTime;
    if ( mmuProperty::getIsDedicatedMode() == 0 )
    {
        UInt sMultiplexingCheckInterval;
        mRun = ID_TRUE;

        while ( mRun == ID_TRUE )
        {
            sMultiplexingCheckInterval = mmuProperty::getMultiplexingCheckInterval() ;
            sSleepTime.set(0,sMultiplexingCheckInterval );
            idlOS::sleep(sSleepTime);
            mmtThreadManager::checkServiceThreads();
        }
    }
    /* PROJ-2108 Dedicated thread mode which uses less CPU
     * Stops expanded idle threads periodically
     */
    else
    {
        PDL_Time_Value sRelativeSleepTime;
        if ( mmuProperty::getThreadCheckInterval() == 0 )
        {
            mRun = ID_FALSE;
        }
        else
        {
            sSleepTime.set(mmuProperty::getThreadCheckInterval(), 0);
            mRun = ID_TRUE;
        }
        while ( mRun == ID_TRUE )
        {
            IDE_ASSERT(mMutexForStop.lock(NULL) == IDE_SUCCESS);
            sRelativeSleepTime = sSleepTime + idlOS::gettimeofday();
            (void)mLoadBalancerCV.timedwait(&mMutexForStop, &sRelativeSleepTime);
            IDE_ASSERT(mMutexForStop.unlock() == IDE_SUCCESS);

            if ( mRun == ID_FALSE )
            {
                break;
            }

            if ( mmuProperty::getDedicatedThreadInitCount() <
                 mmtThreadManager::getServiceThreadSocketCount() )
            {
                mmtThreadManager::stopExpandedIdleServiceThreads();
            }
        }
        finalize();
    }
}

void mmtLoadBalancer::stop()
{

    mRun = ID_FALSE;
    if ( mmuProperty::getIsDedicatedMode() == 1 )
    {
        IDE_ASSERT(mMutexForStop.lock(NULL) == IDE_SUCCESS);
        (void)mLoadBalancerCV.signal();
        IDE_ASSERT(mMutexForStop.unlock() == IDE_SUCCESS);
    }
}

