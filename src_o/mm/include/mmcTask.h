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

#ifndef _O_MMC_TASK_H_
#define _O_MMC_TASK_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <cm.h>
#include <qci.h>
#include <mmcSession.h>


class mmcStatement;
class mmtServiceThread;

class mmcTask
{
private:
    iduListNode         mAllocListNode;
    iduListNode         mThreadListNode;

    mmcTaskState        mTaskState;

    UInt                mConnectTime;
    UInt                mLinkCheckTime;
    /*fix BUG-29717 When a task is terminated or migrated ,
      a busy degree of service thread which the task was assigned
      to should be decreased.*/
    UInt                mBusyExprienceCnt;
    idBool              mIsShutdownTask;
    // fix BUG-27965 LoginTimeout 발생 여부 체크
    idBool              mIsLoginTimeoutTask;

    cmiProtocolContext  mProtocolContext;
    cmiSession          mCmiSession;
    cmiLink            *mLink;

    mmcSession         *mSession;
    mmtServiceThread   *mThread;
    
public:

public:
    IDE_RC initialize();
    IDE_RC finalize();

    IDE_RC setLink(cmiLink *aLink);

    IDE_RC authenticate(qciUserInfo *aUserInfo);

    IDE_RC log(ideLogEntry &aLog);
    /*fix BUG-29717 When a task is terminated or migrated ,
      a busy degree of service thread which the task was assigned
      to should be decreased.*/
    void   incBusyExprienceCnt();
    void   decBusyExprienceCnt(UInt aDelta);
    UInt   getBusyExprienceCnt();
    
private:
    IDE_RC logGeneralInfo(SChar *aBuffer, UInt aBufferLen);
    IDE_RC logTransactionInfo(smiTrans *aTrans, SChar *aBuffer, UInt aBufferLen, const SChar *aTitle);
    IDE_RC logStatementInfo(mmcStatement *aStmt, SChar *aBuffer, UInt aBufferLen);

public:
    /*
     * Accessor
     */

    mmcTaskState       *getTaskStatePtr();

    mmcTaskState        getTaskState();
    void                setTaskState(mmcTaskState aTaskState);

    iduListNode        *getAllocListNode();
    iduListNode        *getThreadListNode();

    UInt                getConnectTime();
    UInt               *getLinkCheckTime();

    idBool              isShutdownTask();
    void                setShutdownTask(idBool aFlag);

    idBool              isLoginTimeoutTask();
    void                setLoginTimeoutTask(idBool aFlag);

    cmiProtocolContext *getProtocolContext();
    cmiSession         *getCmiSession();
    cmiLink            *getLink();

    mmcSession         *getSession();
    void                setSession(mmcSession *aSession);

    mmtServiceThread   *getThread();
    void                setThread(mmtServiceThread *aThread);


};


inline mmcTaskState *mmcTask::getTaskStatePtr()
{
    return &mTaskState;
}

inline mmcTaskState mmcTask::getTaskState()
{
    return mTaskState;
}

inline void mmcTask::setTaskState(mmcTaskState aTaskState)
{
    mTaskState = aTaskState;
}

inline iduListNode *mmcTask::getAllocListNode()
{
    return &mAllocListNode;
}

inline iduListNode *mmcTask::getThreadListNode()
{
    return &mThreadListNode;
}

inline UInt mmcTask::getConnectTime()
{
    return mConnectTime;
}

inline UInt *mmcTask::getLinkCheckTime()
{
    return &mLinkCheckTime;
}

inline idBool mmcTask::isShutdownTask()
{
    return mIsShutdownTask;
}

inline void mmcTask::setShutdownTask(idBool aFlag)
{
    mIsShutdownTask = aFlag;
}

inline idBool mmcTask::isLoginTimeoutTask()
{
    return mIsLoginTimeoutTask;
}

inline void mmcTask::setLoginTimeoutTask(idBool aFlag)
{
    mIsLoginTimeoutTask = aFlag;
}

inline cmiProtocolContext *mmcTask::getProtocolContext()
{
    return &mProtocolContext;
}

inline cmiSession *mmcTask::getCmiSession()
{
    return &mCmiSession;
}

inline cmiLink *mmcTask::getLink()
{
    return mLink;
}

inline mmcSession *mmcTask::getSession()
{
    return mSession;
}

inline void mmcTask::setSession(mmcSession *aSession)
{
    mSession = aSession;
}

inline mmtServiceThread *mmcTask::getThread()
{
    return mThread;
}

inline void mmcTask::setThread(mmtServiceThread *aThread)
{
    mThread = aThread;

    if (getSession() != NULL)
    {
        /* fix BUG-31002, Remove unnecessary code dependency from the inline function of mm module. */
        if (aThread != NULL)
        {
            getSession()->setIdleStartTime(0);
        }
        else
        {
            getSession()->setIdleStartTime(mmtSessionManager::getBaseTime());
        }
    }
}

/*fix BUG-29717 When a task is terminated or migrated ,
  a busy degree of service thread which the task was assigned
  to should be decreased.*/
inline  void   mmcTask::incBusyExprienceCnt()
{
    mBusyExprienceCnt++;
    
}
inline  void   mmcTask::decBusyExprienceCnt(UInt aDelta)
{
    if(mBusyExprienceCnt != 0)
    {   
        if(mBusyExprienceCnt >= aDelta)
        {    
            mBusyExprienceCnt -= aDelta;
        }
        else
        {
            mBusyExprienceCnt = 0;
        }
    }
}
inline  UInt    mmcTask::getBusyExprienceCnt()
{
    return mBusyExprienceCnt;
}
    
#endif
