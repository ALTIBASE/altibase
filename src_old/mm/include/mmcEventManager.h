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

#ifndef _O_MMC_EVENT_MANAGER_H_
#define _O_MMC_EVENT_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <cm.h>
#include <smiTrans.h>
#include <qci.h>
#include <mmcConvNumeric.h>
#include <mmcDef.h>
#include <mmcLob.h>
#include <mmcTrans.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmqQueueInfo.h>
#include <mmtSessionManager.h>

typedef struct mmcEventInfo
{
    SChar * mName;
    SChar * mMessage;
} mmcEventInfo;

class mmcEventManager
{
public:
    mmcEventManager() {};
    ~mmcEventManager() {};

    // 초기화 작업을 수행한다.
    IDE_RC         initialize( mmcSession * aSession );

    // 마무리 작업을 수행한다.
    IDE_RC         finalize();

    // 이벤트 등록 해쉬에 이벤트를 등록한다.
    IDE_RC         regist( const SChar * aName,
                           UShort        aNameSize );

    // 이벤트 등록 해쉬에 주어진 이름이 등록되어 있는지 확인한다.
    idBool         isRegistered( const SChar * aName,
                                 UShort        aNameSize );

    // 이벤트 등록 해쉬에서 주어진 이름의 이벤트를 제거한다.
    IDE_RC         remove(  const SChar * aName,
                            UShort        aNameSize );

    // 이벤트 등록 해쉬를 초기화 한다.
    IDE_RC         removeall();

    // polling interval의 값을 지정한다.
    IDE_RC         setDefaults( SInt aPollingInterval );

    // 이벤트 pending 리스트에 이벤트를 추가한다.
    IDE_RC         signal( const SChar * aName,
                           UShort        aNameSize,
                           const SChar * aMessage,
                           UShort        aMessageSize );

    mmcEventInfo * isSignaled( const SChar * aName,
                               UShort        aNameSize );

    // 임의의 이벤트를 기다린다.
    IDE_RC         waitany( idvSQL   * aStatistics,
                            SChar    * aName,
                            UShort   * aNameSize,
                            SChar    * aMessage,
                            UShort   * aMessageSize,
                            SInt     * aStatus,
                            const SInt aTimeout );

    // 주어진 이름의 이벤트를 기다린다.
    IDE_RC         waitone( idvSQL       * aStatistics,
                            const SChar  * aName,
                            const UShort * aNameSize,
                            SChar        * aMessage,
                            UShort       * aMessageSize,
                            SInt         * aStatus,
                            const SInt     aTimeout );

    IDE_RC         commit();

    IDE_RC         rollback( SChar * aSvpName );

    IDE_RC         savepoint( SChar * aSvpName );

    IDE_RC         apply( mmcSession * aSession );

    inline iduList * getPendingList();

    inline IDE_RC  lock();

    inline IDE_RC  unlock();

    inline IDE_RC  lockForCV();

    inline IDE_RC  unlockForCV();

private:
    // 이벤트 pending 리스트
    iduList        mPendingList;

    // 이벤트 등록 리스트
    // BUGBUG 
    // hash를 사용하도록 변경 해야함.
    iduList        mList;

    // 이벤트 큐
    iduList        mQueue;

    // Polling Interval
    SInt           mPollingInterval;

    // alloc/free가 자유로운 메모리 관리자
    iduVarMemList  mMemory;

    // 세션 정보
    mmcSession   * mSession;

    // cond_timedwait 호출시에 사용
    iduCond        mCV;

    PDL_Time_Value mTV;

    iduMutex       mMutex;

    // 내부 자료구조에 대한 동시성 제어시에 사용
    iduMutex       mSync;

    // 등록된 이벤트의 개수
    SInt           mSize;

    // pending된 이벤트의 개수
    SInt           mPendingSize;

private:
    // pending 리스트에서 savepoint 정보를 제거한다.
    IDE_RC removeSvp( SChar * aSvpName );

    IDE_RC freeNode( iduListNode * aNode );

    void   dump();
};

inline iduList * mmcEventManager::getPendingList()
{
    return &mPendingList;
}

inline IDE_RC mmcEventManager::lock()
{
    return mSync.lock( NULL );
}

inline IDE_RC mmcEventManager::unlock()
{
    return mSync.unlock();
}

inline IDE_RC mmcEventManager::lockForCV()
{
    return mMutex.lock( NULL );
}

inline IDE_RC mmcEventManager::unlockForCV()
{
    return mMutex.unlock();
}

#endif
