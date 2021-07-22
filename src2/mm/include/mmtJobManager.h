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
 *
 * PROJ-1438 Job Scheduler
 *
 **********************************************************************/

#ifndef _O_MMT_JOB_MANAGER_H_
#define _O_MMT_JOB_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idtBaseThread.h>
#include <idu.h>
#include <mmcSession.h>

#define MMT_JOB_MANAGER_MAX_THREADS (128)
#define MMT_JOB_MAX_ITEMS           (128)

class mmtJobThread : public idtBaseThread
{
private:
    idBool              mRun;
    iduMutex            mMutex;
    iduCond             mThreadCV;
    mmcSession        * mSession;
    UInt                mIndex;
    UInt                mExecCount;
    
    iduQueue          * mQueue;

public:

    mmtJobThread();

    IDE_RC initialize( UInt aIndex, iduQueue * aQueue );
    void finalize();

    IDE_RC initializeThread();
    void   finalizeThread();
    
    void signalToJobThread();

    void run();
    void stop();
    void signalLock();
    void signalUnlock();
};

inline void mmtJobThread::signalLock()
{
    IDE_ASSERT( mMutex.lock(NULL) == IDE_SUCCESS );
}

inline void mmtJobThread::signalUnlock()
{
    IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
}

class mmtJobScheduler : public idtBaseThread
{
private:
    idBool          mRun;
    UInt            mThreadCount;
    mmtJobThread  * mJobThreads[MMT_JOB_MANAGER_MAX_THREADS];

    iduQueue        mQueue;
    
public:
    mmtJobScheduler();

    IDE_RC initialize();
    void   finalize();

    IDE_RC initializeThread();
    void   finalizeThread();
    
    void   run();
    void   stop();
};

class mmtJobManager
{
private:
    static mmtJobScheduler * mJobScheduler;

public:

    static IDE_RC initialize();
    static IDE_RC finalize();
};

#endif /* _O_MMT_JOB_MANAGER_H_ */
