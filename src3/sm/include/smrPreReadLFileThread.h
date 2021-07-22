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
 * $Id: smrPreReadLFileThread.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_SMR_PREREADLFILE_THREAD_H_
#define _O_SMR_PREREADLFILE_THREAD_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <smrDef.h>
#include <smrLogFile.h>
#include <iduMemListOld.h>

/* PreReadInfo의 mFlag값 */
#define SMR_PRE_READ_FILE_MASK  (0x00000003)
/* 초기값 */
#define SMR_PRE_READ_FILE_NON   (0x00000000)
/* File의 open이 완료되었을 때 */
#define SMR_PRE_READ_FILE_OPEN  (0x00000001)
/* File의 Close가 요청되었을 때 */
#define SMR_PRE_READ_FILE_CLOSE (0x00000002)

/*
  Pre Read Thread가 자신에게 들어오는 Open Request
  를 관리하기 위해서 Request가 들어올때 마다 하나식
  만들어진다.
*/
typedef struct smrPreReadLFInfo
{
    // 읽기를 요청한 FileNo
    UInt        mFileNo;
    // Open된 LogFile
    smrLogFile *mLogFilePtr;
    
    // 현재 File이 Open되었으면 SMR_PRE_READ_FILE_OPEN,
    // 아니면 SMR_PRE_READ_FILE_CLOSE
    UInt        mFlag;

    struct smrPreReadLFInfo *mNext;
    struct smrPreReadLFInfo *mPrev;
} smrPreReadLFInfo;

/*
  smrPreReadLFileThread는 Replication 의 Sender를
  위해서 만들어진 것으로 Sender가 읽어야 할 파일에 대해서
  미리 Read를 수행하여 Sender가 Disk/IO때문에 waiting이
  발생하는것을 방지하기위해 만들어졌다.
*/
class smrPreReadLFileThread : public idtBaseThread
{
//Member Function
public:
    /* 초기화를 수행한다.*/
    IDE_RC initialize();
    /* open된 Logfile을 close하고 할당된 Resource를 반환한다.*/
    IDE_RC destroy();

    /* PreReadThread에게 aFileNo에 해당하는 파일에
       대해서 open을 요청한다.*/
    IDE_RC addOpenLFRequest( UInt aFileNo );
    /* aFileNo에 해당하는 File의 Close를 요청한다.*/
    IDE_RC closeLogFile( UInt aFileNo );

    /* Thread를 종료시킨다.*/
    IDE_RC shutdown();
    
    virtual void run();

    /* PreRead Thread가 Sleep중이면 깨운다.*/
    IDE_RC resume();

    smrPreReadLFileThread();
    virtual ~smrPreReadLFileThread();
    
//Member Function
private:
    /* Request List, Open Log File List에대서 접근시 mMutex.lock수행*/
    inline IDE_RC lock();
    /* mMutex.unlock수행*/
    inline IDE_RC unlock();
    
    inline IDE_RC lockCond() { return mCondMutex.lock( NULL /* idvSQL* */ ); }
    inline IDE_RC unlockCond() { return mCondMutex.unlock(); }
    
    /* aInfo를 Request List에 추가*/
    inline void addToLFRequestList(smrPreReadLFInfo *aInfo);
    /* aInfo를 Request List에서 제거*/
    inline void removeFromLFRequestList(smrPreReadLFInfo *aInfo);
    /* aInfo를 Open Log File List에 추가*/
    inline void addToLFList(smrPreReadLFInfo *aInfo);
    /* aInfo를 Open Log File List에서 제거*/
    inline void removeFromLFList(smrPreReadLFInfo *aInfo);
    /* Request List가 비어있는지 check */
    inline idBool isEmptyOpenLFRequestList() 
        { return mOpenLFRequestList.mNext == &mOpenLFRequestList ? ID_TRUE : ID_FALSE; }
    /* smrPreReadLFInfo를 초기화*/
    inline void initPreReadInfo(smrPreReadLFInfo *aInfo);

    /* aFileNo에 해당하는 PreRequestInfo를 Request List에서 찾는다.*/
    smrPreReadLFInfo* findInOpenLFRequestList( UInt aFileNo );
    /* aFileNo에 해당하는 PreRequestInfo를 Open Logfile List에서 찾는다.*/
    smrPreReadLFInfo* findInOpenLFList( UInt aFileNo );
    /* logfile을 Open을 수행할 smrPreReadInfo를 찾는다.*/
    IDE_RC getJobOfPreReadInfo(smrPreReadLFInfo **aPreReadInfo);

//Member Variable    
private:
    /* Open LogFile Request List*/
    smrPreReadLFInfo mOpenLFRequestList;
    /* Open LogFile List*/
    smrPreReadLFInfo mOpenLFList;
    /* List관리 Mutex*/
    iduMutex         mMutex;
    /* Pre Read Info Memory Pool */
    iduMemListOld    mPreReadLFInfoPool;
    /* Thread 종료 Check */
    idBool           mFinish;

    /* Condition Variable */
    iduCond          mCV;
    /* Time Value */
    PDL_Time_Value   mTV;

    /* Waiting관련 Mutex로서 Thread가 sleep할 경우
     이 Mutex를 풀고 waiting한다.*/
    iduMutex         mCondMutex;

    /* Thread를 Wake up할 경우 이값을 ID_TRUE로 한다. */
    idBool           mResume;
};

inline IDE_RC smrPreReadLFileThread::lock()
{
    return mMutex.lock( NULL /* idvSQL* */ );
}

inline IDE_RC smrPreReadLFileThread::unlock()
{
    return mMutex.unlock();
}

inline void smrPreReadLFileThread::addToLFRequestList(smrPreReadLFInfo *aInfo)
{
    aInfo->mNext = &mOpenLFRequestList;
    aInfo->mPrev = mOpenLFRequestList.mPrev;
    mOpenLFRequestList.mPrev->mNext = aInfo;
    mOpenLFRequestList.mPrev = aInfo;
}

inline void smrPreReadLFileThread::removeFromLFRequestList(smrPreReadLFInfo *aInfo)
{
    aInfo->mNext->mPrev = aInfo->mPrev;
    aInfo->mPrev->mNext = aInfo->mNext;
}

inline void smrPreReadLFileThread::addToLFList(smrPreReadLFInfo *aInfo)
{
    aInfo->mNext = &mOpenLFList;
    aInfo->mPrev = mOpenLFList.mPrev;
    mOpenLFList.mPrev->mNext = aInfo;
    mOpenLFList.mPrev = aInfo;
}

inline void smrPreReadLFileThread::removeFromLFList(smrPreReadLFInfo *aInfo)
{
    aInfo->mNext->mPrev = aInfo->mPrev;
    aInfo->mPrev->mNext = aInfo->mNext;
}

inline void smrPreReadLFileThread::initPreReadInfo(smrPreReadLFInfo *aInfo)
{
    aInfo->mNext   = NULL;
    aInfo->mPrev   = NULL;
    aInfo->mFileNo = ID_UINT_MAX;
    aInfo->mLogFilePtr = NULL;
    aInfo->mFlag   = SMR_PRE_READ_FILE_NON;
}

#endif /* _O_SMR_PREREADLFILE_THREAD_H_ */


