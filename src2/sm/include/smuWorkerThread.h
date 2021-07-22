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
 * $Id: smuWorkerThread.h
 *
 * PROJ-2162 RestartRiskReduction 에서 추가됨
 *
 * Description :
 * Thread에 대한 Wrapping class.
 * 쉽게 ChildThread에게 일을 분배할 수 있는 가장 간단한 클래스
 *
 * Algorithm  :
 * JobQueue를 SingleWriterMulterReader 로 사용하여, 업무를 분배한다.
 *
 * ChildThread가 4개 있다고 했을때,
 * ThreadMgr가 Job을 Queue의 1번,2번,3번,4번,5번 등의 Slot에 등록하면,
 * 0번  - 0,4,8,12
 * 1번  - 1,5,9,13
 * 2번  - 2,6,10,14
 * 3번  - 3,7,11,15
 * 위 업무들을 각각 실행한다.
 *
 * 따라서 각 Job의 소요시간에 따라, 특정 ClientThread의 처리가 늦어지는
 * 현상이 나타날 수도 있다.
 *
 * Issue :
 * 1) Queue에 대해서는 DirtyWrite/DirtyRead를 수행한다. Pointer변수
 *    이기 때문에, 값을 가져오는 것 자체는 Atomic하기 때문에 가능하다.
 * 2) 하지만 ABA Problem이 발생했을 경우, 다른 Core에 CacheMiss를
 *    알려주지 않는 문제가 발생할 수 있으므로, 해당 경우에만
 *    volatile 을 사용한다.
 * 3) ThreadCnt가 1일 경우, ThreadMgr가 addJob 함수에서 직접 Job을
 *    처리한다. 만약의 경우 ThreadMgr가 제대로 동작하지 않아 Hang에
 *    빠졌을 경우를 대비해, ThreadCnt가 1이면 SingleThread처럼
 *    동작하도록 하기 위함이다. 
 *
 **********************************************************************/

#ifndef _O_SMU_WORKER_THREAD_H_
#define _O_SMU_WORKER_THREAD_H_ 1

#include <idu.h>
#include <ide.h>
#include <idtBaseThread.h>

/* Example:
 *  smuWorkerThreadMgr     sThreadMgr;
 *
 *  IDE_TEST( smuWorkerThread::initialize( 
 *        <업무를 처리하는 함수>,
 *        <ChildThread개수>,
 *        <Queue크기>,
 *        &sThreadMgr )
 *    != IDE_SUCCESS );
 *
 *  IDE_TEST( smuWorkerThread::addJob( &sThreadMgr, <업무변수> ) != IDE_SUCCESS );
 *  IDE_TEST( smuWorkerThread::finalize( &sThreadMgr ) != IDE_SUCCESS );
*/

typedef void (*smuWorkerThreadFunc)(void * aParam );

class smuWorkerThread;

typedef struct smuWorkerThreadMgr
{
    smuWorkerThreadFunc    mThreadFunc;    /* 업무 처리용 함수 */
    UInt                   mThreadCnt;     /* Thread 개수 */
    idBool                 mDone;          /* 더이상 할일이 없는가? */
                    
    void                ** mJobQueue;      /* Job Queue. */
    UInt                   mJobTail;       /* Queue의 Tail */
    UInt                   mQueueSize;     /* Queue의 크기 */

    smuWorkerThread      * mThreadArray;   /* ChildThread들 */
} smuWorkerThreadMgr;

class smuWorkerThread : public idtBaseThread
{
public:
    UInt                 mJobIdx;          /* 자신이 가져올 Job의 Index */
    smuWorkerThreadMgr * mThreadMgr;       /* 자신을 관리하는 관리자 */

    smuWorkerThread() : idtBaseThread() {}

    static IDE_RC initialize( smuWorkerThreadFunc    aThreadFunc, 
                              UInt             aThreadCnt,
                              UInt             aQueueSize,
                              smuWorkerThreadMgr   * aThreadMgr );
    static IDE_RC finalize( smuWorkerThreadMgr * aThreadMgr );
    static IDE_RC addJob( smuWorkerThreadMgr * aThreadMgr, void * aParam );
    static void   wait( smuWorkerThreadMgr * aThreadMgr );

    virtual void run(); /* 상속받은 main 실행 루틴 */
};

#endif 

