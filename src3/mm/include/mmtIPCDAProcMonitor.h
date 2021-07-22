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


#ifndef _O_MMT_IPCDA_PROC_MANAGER_H_
#define _O_MMT_IPCDA_PROC_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idtBaseThread.h>
#include <idu.h>
#include <iduHash.h>
#include <cm.h>

class mmtIPCDAProcThread : public idtBaseThread
{
private:
    idBool              mRun;
    iduLatch            mLatch;
    iduCond             mThreadCV;

    iduHash             mIPCDAClientInfoHash;

    void lock();
    void unlock();

    static IDE_RC checkProc(vULong aKey, void *aData, void *aVisitorArg);

    void watchSockHandle();

public:

    IDE_RC initialize();
    IDE_RC finalize();

    IDE_RC addIPCDAProcInfo(cmiProtocolContext *aCtx);
    IDE_RC delIPCDAProcInfo(cmiProtocolContext *aCtx);

    void run();
    void stop();
};

inline void mmtIPCDAProcThread::lock()
{
    IDE_ASSERT( mLatch.lockWrite(NULL, NULL) == IDE_SUCCESS );
}

inline void mmtIPCDAProcThread::unlock()
{
    IDE_ASSERT( mLatch.unlock() == IDE_SUCCESS );
}

class mmtIPCDAProcMonitor
{
    private:
        static mmtIPCDAProcThread *mIPCDAProcThread;
        static idBool              mStarted;

    public:
        static IDE_RC initialize();
        static IDE_RC addIPCDAProcInfo(cmiProtocolContext *aCtx);
        static IDE_RC delIPCDAProcInfo(cmiProtocolContext *aCtx);
        static IDE_RC finalize();
};

#endif /* _O_MMT_IPCDA_PROC_MANAGER_H_ */
