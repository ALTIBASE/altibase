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
 
/* **********************************************************************
 *   $Id: idtBaseThread.h 82088 2018-01-18 09:21:15Z yoonhee.kim $
 *   NAME
 *     idtBaseThread.h - 쓰레드 기본 클래스 선언
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
 ********************************************************************** */

#ifndef O_IDT_BASETHREAD_H
#define O_IDT_BASETHREAD_H   1

#include <idl.h>
#include <idtCPUSet.h>

#define IDT_JOINABLE 0 
#define IDT_DETACHED 1

class idtContainer;

class idtBaseThread
{
private:
    /* Project 2379 - Thread container */
    idtContainer*   mContainer;
    idBool          mIsJoin;
    idtCPUSet       mAffinity;
    idBool          mIsServiceThread;

public:
    idtBaseThread(SInt aFlag = IDT_JOINABLE);
    virtual ~idtBaseThread() {};
            
	/*BUGBUG_NT ADD*/
    PDL_thread_t    getTid();
    PDL_hthread_t   getHandle();

    idBool          isStarted();
    IDE_RC          waitToStart(UInt second = 0);
    IDE_RC          join();
    idBool          isServiceThread()
    {
        return mIsServiceThread;
    }
    void            setIsServiceThread( idBool aIsServeiceThread )
    {
        mIsServiceThread = aIsServeiceThread ;
    }

    virtual IDE_RC  start();
    virtual IDE_RC  initializeThread() {return IDE_SUCCESS;}
    virtual void    run() = 0;
    virtual void    finalizeThread() {}

    /* TASK-6764 CPU Affinity interfaces */
public:
    IDE_RC          setAffinity(idtCPUSet&);
    IDE_RC          setAffinity(const SInt);
    IDE_RC          setNUMAAffinity(const SInt);
    IDE_RC          getAffinity(idtCPUSet&);
    IDE_RC          getAffinity(idtCPUSet*);
    IDE_RC          getAffinity(SInt&);
    IDE_RC          getAffinity(SInt*);
    IDE_RC          unbindAffinity(void);

    friend class idtContainer;
};

class idtThreadRunner : public idtBaseThread
{
private:
    void* (*mFunc)(void*);
    void* mArg;

public:
    idtThreadRunner(SInt aFlag = IDT_JOINABLE)
        : idtBaseThread(aFlag) {}
    virtual ~idtThreadRunner() {}

    inline IDE_RC launch(void* (*aFunc)(void*), void* aArg)
    {
        mFunc = aFunc;
        mArg = aArg;
        return start();
    }
    virtual void run() { (void)(*mFunc)(mArg); }
};
#endif 
