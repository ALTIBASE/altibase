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
 * $Id: smuJobThread.h
 *
 * TASK-6887 Improve index rebuild efficiency at server startup 에서 추가됨
 *
 * Description :
 * smuWorkerThread 의 Queue Interface 수정 버전
 *
 * Algorithm  :
 * JobQueue를 SingleWriterMulterReader 로 사용하여, 업무를 분배한다.
 * Queue Pop 시 Thread 들이 Lock 보호아래 경쟁적으로 Pop 한다
 *
 * Issue :
 *
 **********************************************************************/

#ifndef _O_SMU_JOB_THREAD_H_
#define _O_SMU_JOB_THREAD_H_ 1

#include <idu.h>
#include <idl.h>
#include <ide.h>
#include <idtBaseThread.h>

/* Example:
 *  smuJobThreadMgr sThreadMgr;
 *
 *  IDE_TEST( smuJobThread::initialize( 
 *        <업무를 처리하는 함수>,
 *        <ChildThread개수>,
 *        <Queue크기>,
 *        &sThreadMgr )
 *    != IDE_SUCCESS );
 *
 *  IDE_TEST( smuJobThread::addJob( &sThreadMgr, <업무변수> ) != IDE_SUCCESS );
 *  IDE_TEST( smuJobThread::finalize( &sThreadMgr ) != IDE_SUCCESS );
*/

#define SMU_JT_Q_NEXT( __X, __Q_SIZE ) ( ( __X + 1 ) % __Q_SIZE )
#define SMU_JT_Q_IS_EMPTY( __MGR )     ( __MGR->mJobHead == __MGR->mJobTail )
#define SMU_JT_Q_IS_FULL( __MGR )      \
    ( __MGR->mJobHead == SMU_JT_Q_NEXT( __MGR->mJobTail, __MGR->mQueueSize ) )

typedef void (*smuJobThreadFunc)(void * aParam );

class smuJobThread;

typedef struct smuJobThreadMgr
{
    smuJobThreadFunc   mThreadFunc;     /* 업무 처리용 함수 */
                    
    UInt               mJobHead;        /* Queue의 Head */
    UInt               mJobTail;        /* Queue의 Tail */
    UInt               mQueueSize;      /* Queue의 크기 */
    iduMutex           mQueueLock;      /* Queue Lock */
    void            ** mJobQueue;       /* Job Queue. */

    UInt               mWaitAddJob;
    iduMutex           mWaitLock;       
    iduCond            mConsumeCondVar; /* Thread signals to AddJob */

    idBool             mDone;           /* 더이상 할일이 없는가? */
    UInt               mThreadCnt;      /* Thread 개수 */
    smuJobThread     * mThreadArray;    /* ChildThread들 */
} smuJobThreadMgr;

class smuJobThread : public idtBaseThread
{
public:
    static UInt       mWaitIntvMsec;
    UInt              mJobIdx;          /* 자신이 가져올 Job의 Index */
    smuJobThreadMgr * mThreadMgr;       /* 자신을 관리하는 관리자 */

    smuJobThread() : idtBaseThread() {}

    static IDE_RC initialize( smuJobThreadFunc    aThreadFunc, 
                              UInt                aThreadCnt,
                              UInt                aQueueSize,
                              smuJobThreadMgr   * aThreadMgr );
    static IDE_RC finalize(   smuJobThreadMgr   * aThreadMgr );
    static IDE_RC addJob(     smuJobThreadMgr   * aThreadMgr, void * aParam );
    static void   wait(       smuJobThreadMgr   * aThreadMgr );

    virtual void run(); /* 상속받은 main 실행 루틴 */
};

#endif 

