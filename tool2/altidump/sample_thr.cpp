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
 
#include <idl.h>
#include <idtBaseThread.h>
#include <ideErrorMgr.h>
#include <ideMsgLog.h>
#include <iduStack.h>
#include <sample.h>

ideMsgLog mMsgLogForce;

void setMsgLog()
{
    SChar               msglogFilename[256];
    
    idlOS::memset(msglogFilename, 0, 256);
    idlOS::sprintf(msglogFilename, "sample_thr.log");

    assert(mMsgLogForce.initialize(1, msglogFilename,
                                   1000000,
                                   0) == IDE_SUCCESS);
    ideLogSetTarget(&mMsgLogForce);
}


class mmtTaskThread : public idtBaseThread
{
public:
    mmtTaskThread();
    IDE_RC initialize();
    
    IDE_RC destroy();
    virtual void run();
    
};

mmtTaskThread::mmtTaskThread() 
    : idtBaseThread(IDT_DETACHED)
{
}

IDE_RC mmtTaskThread::initialize()
{
    return IDE_SUCCESS;
}

IDE_RC mmtTaskThread::destroy()
{
    return IDE_SUCCESS;
}

void mmtTaskThread::run()
{
//    assert(setupDefaultThreadSignal() == IDE_SUCCESS);
    assert(setupDefaultAltibaseSignal() == IDE_SUCCESS);

    fprintf(stderr, "pid=%d thr id=%d\n", idlOS::getpid(), 
            (UInt)idlOS::getThreadID());
    ideLog("=========== before normal stack dump ============");
    extPrint();
    ideLog("=========== end normal stack dump ============");

    
    ideLog("=========== before SEGV stack dump ============");
    extCore();
    ideLog("=========== end  SEGV stack dump ============");
}

mmtTaskThread gThr;

int main()
{
    setMsgLog();
    ideLog("testing");
    printf("size is %d\n", sizeof(long));

    assert(setupDefaultAltibaseSignal() == IDE_SUCCESS);

    assert(gThr.initialize() == IDE_SUCCESS);

    assert(gThr.start() == IDE_SUCCESS);
}
