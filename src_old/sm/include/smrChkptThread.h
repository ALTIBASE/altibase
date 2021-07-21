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
 * $Id: smrChkptThread.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMR_CHKPT_THREAD_H_
#define _O_SMR_CHKPT_THREAD_H_ 1

#include <idu.h>
#include <idtBaseThread.h>

class smrChkptThread : public idtBaseThread
{
//For Operation
public:
    smrChkptThread();
    virtual ~smrChkptThread();

    IDE_RC initialize();
    IDE_RC destroy();

    IDE_RC resumeAndWait( idvSQL * aStatistics );
    
    IDE_RC resumeAndNoWait( SInt         aByWho,
                            smrChkptType aChkptType );

    // To Fix BUG-9366
    IDE_RC setCheckPTLSwitchInterval();
    IDE_RC setCheckPTTimeInterval();

    IDE_RC clearCheckPTInterval(idBool, idBool);
    
    IDE_RC startThread();
    IDE_RC disable();
    
    inline IDE_RC lock( idvSQL* aStatistics );
    inline IDE_RC unlock();

    IDE_RC shutdown();
    
    virtual void run();

    // Checkpoint가 수행되지 않도록 막는다.
    static IDE_RC blockCheckpoint();

    // Checkpoint가 다시 수행되도록 Checkpoint를 Unblock한다.
    static IDE_RC unblockCheckpoint();

    void applyStatisticsForSystem();

    static IDE_RC flushForCheckpoint( idvSQL       *aStatistics,
                                      ULong         aMinFlushCount,
                                      ULong         aRedoDirtyPageCnt,
                                      UInt          aRedoLogFileCount,
                                      ULong        *aFlushedCount);
    
//For Member
public:
    iduCond           mCV;
    PDL_Time_Value    mTV;
    idBool            mResume;
    idBool            mFinish;
    SInt              mReason;
    smrChkptType      mChkptType;
    iduMutex          mMutex;

private:
    idvSQL            mStatistics;
    idvSession        mCurrSess;
    idvSession        mOldSess;
    sdbFlusher        mFlusher;
};

inline IDE_RC smrChkptThread::lock( idvSQL* aStatistics )
{ return mMutex.lock( aStatistics ); }

inline IDE_RC smrChkptThread::unlock()
{ return mMutex.unlock(); }

extern smrChkptThread gSmrChkptThread;

#endif /* _O_SMR_CHKPT_THREAD_H_ */
