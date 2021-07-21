/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: idtBaseThread.cpp 82088 2018-01-18 09:21:15Z yoonhee.kim $
 ****************************************************************************/

/*****************************************************************************
 *   NAME
 *     idtBaseThread.cpp - 쓰레드 기본 클래스 
 *
 *   DESCRIPTION
 *      iSpeener에서 사용될 모든 쓰레드들의 Base Thread Class로 사용됨
 *
 *   PUBLIC FUNCTION(S)
 *
 *   PRIVATE FUNCTION(S)
 *
 *   NOTES
 *
 *   MODIFIED   (MM/DD/YY)
 ****************************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idtBaseThread.h>
#include <idtContainer.h>
#include <idErrorCode.h>
#include <iduProperty.h>
#include <iduFitManager.h>

/*----------------------- idtBaseThread 생성자--------------------------------
     NAME
	     idtBaseThread 생성자

     DESCRIPTION

     ARGUMENTS

     RETURNS
     	없음
----------------------------------------------------------------------------*/

idtBaseThread::idtBaseThread(SInt aFlag)
{
#define IDE_FN "idtBaseThread::idtBaseThread(SInt Flags, SInt Priority, void *Stack, UInt StackSize)"
    mContainer = NULL;
    mIsServiceThread = ID_FALSE;

    switch(aFlag)
    {
    case IDT_JOINABLE:
        mIsJoin = ID_TRUE;
        break;
    case IDT_DETACHED:
        mIsJoin = ID_FALSE;
        break;
    default:
        IDE_ASSERT(0);
    }
#undef IDE_FN
}

PDL_thread_t idtBaseThread::getTid()
{
    return (mContainer==NULL)? (PDL_thread_t)-1:(mContainer->getTid());
}

PDL_hthread_t idtBaseThread::getHandle()
{
    return (mContainer==NULL)? (PDL_hthread_t)-1:(mContainer->getHandle());
}

idBool idtBaseThread::isStarted()
{
    return (mContainer==NULL)? ID_FALSE:(mContainer->isStarted());
}

IDE_RC idtBaseThread::join()
{
    idtContainer* sContainer = mContainer;

    IDE_TEST_RAISE(sContainer == NULL, ENOTSTARTED);
    IDE_TEST_RAISE(mIsJoin !=  ID_TRUE, EUNBOUNDTHREAD);
    IDE_TEST_RAISE(sContainer->join() != IDE_SUCCESS, ESYSTEMERROR);
    mContainer = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ENOTSTARTED)
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_THREAD_NOTSTARTED));
    }
    IDE_EXCEPTION(EUNBOUNDTHREAD)
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_THREAD_UNBOUND));
    }
    IDE_EXCEPTION(ESYSTEMERROR)
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_THREAD_JOINERROR));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
/*--------------------------- start  ---------------------------------------
     NAME
	     start()

     DESCRIPTION
     	해당 클래스를 쓰레드로 동작시킨다. (run()이 호출됨)
        
     ARGUMENTS
     	없음
        
     RETURNS
     	idlOS::thr_create() 함수의 리턴값
----------------------------------------------------------------------------*/
IDE_RC idtBaseThread::start()
{
    idtContainer* sContainer;

#define IDE_FN "SInt idtBaseThread::start()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#if defined(WRS_VXWORKS) 
    static UInt gTid = 0; 

    if( taskLock() == OK ) 
    { 
        idlOS::snprintf( handle_.name, 
                         sizeof(handle_.name), 
                         "aSrv$%"ID_INT32_FMT"", 
                         gTid++ ); 
        tid_ = handle_.name; 
        (void)taskUnlock(); 
    } 
#endif 

    IDE_TEST_RAISE( idtContainer::pop( &sContainer ) != IDE_SUCCESS, create_error );

    /* TC/FIT/Limit/BUG-16241/BUG-16241.sql */
    IDU_FIT_POINT_RAISE( "idtBaseThread::start::Thread::sContainer", create_error );
    mAffinity.copyFrom(idtCPUSet::mProcessPset);

    IDE_TEST_RAISE(sContainer->start( (void*)this ) != IDE_SUCCESS, create_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(create_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_THR_CREATE_FAILED));
    }
    IDE_EXCEPTION_END;
   
    return IDE_FAILURE;
    

#undef IDE_FN
}

#define IDT_WAIT_LOOP_PER_SECOND     10
// 쓰레드로 동작할 때 까지 대기
IDE_RC idtBaseThread::waitToStart(UInt second)
{

#define IDE_FN "SInt idtBaseThread::waitToStart()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#if defined(ITRON)
    /* empty */
    return IDE_SUCCESS;
#else
    static PDL_Time_Value waitDelay;
    UInt i;
    SInt j;
    
    waitDelay.initialize(0, 1000000 / IDT_WAIT_LOOP_PER_SECOND);
    if (second == 0) second = UINT_MAX;
    
    for (i = 0; i < second; i++)
    {
        for (j = 0; j < IDT_WAIT_LOOP_PER_SECOND; j++)
        {
            if (isStarted() == ID_TRUE)
            {
                return IDE_SUCCESS;
            }
            idlOS::sleep(waitDelay);
        }
    }
    
    IDE_SET(ideSetErrorCode(idERR_FATAL_THR_NOT_STARTED, second));
    
    return IDE_FAILURE;
#endif

#undef IDE_FN
}

/*
 * TASK-6764 CPU Affinity interfaces
 */

/*
 * 현재 스레드가 aCPUSet에 설정된 CPU에서 작동하도록
 * affinity를 설정한다.
 * idtBaseThread를 상속받은 클래스에서만 사용 가능하다.
 * aCPUSet 내부의 CPU가 하나이면 모든 운영체제에서 작동한다.
 * aCPUSet 내부의 CPU가 둘 이상이면 Linux에서만 작동하며 다른
 * 운영체제에서는 TRC 오류 메시지를 출력하고 IDE_FAILURE를 리턴한다.
*/
IDE_RC idtBaseThread::setAffinity(idtCPUSet& aCPUSet)
{
    IDE_TEST( aCPUSet.bindThread() != IDE_SUCCESS );
    mAffinity.copyFrom(aCPUSet);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * 현재 스레드가 aCPUNo에서 작동하도록 affinity를 설정한다.
 * idtBaseThread를 상속받은 클래스에서만 사용 가능하다.
 * 모든 운영체제에서 작동한다.
 */
IDE_RC idtBaseThread::setAffinity(const SInt aCPUNo)
{
    idtCPUSet sCPUSet(IDT_EMPTY);
    sCPUSet.addCPU(aCPUNo);
    IDE_TEST( sCPUSet.bindThread() != IDE_SUCCESS );
    mAffinity.copyFrom(sCPUSet);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * 현재 스레드가 aNUMANo에 해당하는 CPU에서 작동하도록
 * affinity를 설정한다.
 * idtBaseThread를 상속받은 클래스에서만 사용 가능하다.
 * Linux에서만 작동하며 다른 운영체제에서는
 * TRC 오류 메시지를 출력하고 IDE_FAILURE를 리턴한다.
 */
IDE_RC idtBaseThread::setNUMAAffinity(const SInt aNUMANo)
{
    return setAffinity(idtCPUSet::mNUMAPsets[aNUMANo]);
}

/*
 * 현재 스레드의 affinity를 알아내 aCPUSet에 저장한다.
 * idtBaseThread를 상속받은 클래스에서만 사용 가능하다.
 * unbind 상태라면 현재 DBMS 서버가 사용 가능한
 * CPU Set이 aCPUSet에 저장된다.
 * 라이센스로 CPU 개수를 제한한 경우 사용 가능한 CPU들만이 저장된다.
 */
IDE_RC idtBaseThread::getAffinity(idtCPUSet& aCPUSet)
{
    aCPUSet.copyFrom(mAffinity);
    return IDE_SUCCESS;
}

/*
 * 현재 스레드의 affinity를 알아내 aCPUSet에 저장한다.
 * idtBaseThread를 상속받은 클래스에서만 사용 가능하다.
 * unbind 상태라면 현재 DBMS 서버가 사용 가능한
 * CPU Set이 aCPUSet에 저장된다.
 * 라이센스로 CPU 개수를 제한한 경우 사용 가능한 CPU들만이 저장된다.
 */
IDE_RC idtBaseThread::getAffinity(idtCPUSet* aCPUSet)
{
    aCPUSet->copyFrom(mAffinity);
    return IDE_SUCCESS;
}

/*
 * 현재 스레드의 affinity를 알아내 aCPUNo에 저장한다.
 * idtBaseThread를 상속받은 클래스에서만 사용 가능하다.
 * 현재 스레드가 하나의 CPU에만 bind되어 있다면
 * 해당 CPU 번호를 aCPUNo에 설정한다.
 * 둘 이상의 CPU에 bind되어 있거나 unbind 상태라면
 * -1을 설정하고 * trc 오류 메시지를 출력한 후
 * IDE_FAILURE를 리턴한다.
 */
IDE_RC idtBaseThread::getAffinity(SInt& aCPUNo)
{
    IDE_TEST(mAffinity.getCPUCount() == 0);
    IDE_TEST(mAffinity.getCPUCount() >  1);

    aCPUNo = mAffinity.findFirstCPU();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idtBaseThread::getAffinity(SInt* aCPUNo)
{
    IDE_TEST(mAffinity.getCPUCount() == 0);
    IDE_TEST(mAffinity.getCPUCount() >  1);

    *aCPUNo = mAffinity.findFirstCPU();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * 현재 스레드의 CPU Affinity를 해제한다.
 */
IDE_RC idtBaseThread::unbindAffinity(void)
{
    return idtCPUSet::unbindThread();
}


