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

#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idvProfile.h>
#include <idtContainer.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcTask.h>
#include <mmtAdminManager.h>
#include <mmuOS.h>
#include <mmuProperty.h>
#include <idmSNMP.h>
#include <mmtIPCDAProcMonitor.h>
#include <acpProc.h>

mmtIPCDAProcThread *mmtIPCDAProcMonitor::mIPCDAProcThread;
idBool              mmtIPCDAProcMonitor::mStarted = ID_FALSE;

IDE_RC mmtIPCDAProcMonitor::initialize()
{
    mmtIPCDAProcThread *sIPCDAProcThread = NULL;
    UInt sState = 0;

    IDU_FIT_POINT_RAISE( "mmtIPCDAProcMonitor::mmtIPCDAProcMonitor::initialize::malloc",
                         InsufficientMemory );
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_MMT, 
                                     ID_SIZEOF(mmtIPCDAProcThread), 
                                     (void**)&sIPCDAProcThread), InsufficientMemory);
    new (sIPCDAProcThread) mmtIPCDAProcThread();
    sState = 1;

    IDE_TEST(sIPCDAProcThread->initialize() != IDE_SUCCESS );
    sState = 2;

    IDE_TEST(sIPCDAProcThread->start() != IDE_SUCCESS );

    mIPCDAProcThread = sIPCDAProcThread;

    mStarted = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch(sState)
    {
    case 2:
        sIPCDAProcThread->finalize();
    case 1:
        IDE_ASSERT( iduMemMgr::free(sIPCDAProcThread) == IDE_SUCCESS );
        sIPCDAProcThread = NULL;
    case 0:
    default:
        break;
    }

    mStarted = ID_FALSE;

    return IDE_FAILURE;

}

IDE_RC mmtIPCDAProcMonitor::addIPCDAProcInfo(cmiProtocolContext *aCtx)
{
    return mIPCDAProcThread->addIPCDAProcInfo(aCtx);
}

IDE_RC mmtIPCDAProcMonitor::delIPCDAProcInfo(cmiProtocolContext *aCtx)
{
    return mIPCDAProcThread->delIPCDAProcInfo(aCtx);
}

IDE_RC mmtIPCDAProcMonitor::finalize()
{
    if (mStarted == ID_TRUE)
    {
        mIPCDAProcThread->stop();
        IDE_TEST(mIPCDAProcThread->finalize() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtIPCDAProcThread::initialize()
{
    mRun = ID_FALSE;
    
    IDE_TEST( mLatch.initialize( (SChar*)"MMT_IPCDA_PROC_MONITOR_MUTEX",
                                 IDU_LATCH_TYPE_NATIVE ) != IDE_SUCCESS )

    IDE_TEST( mIPCDAClientInfoHash.initialize(IDU_MEM_MMT, 1024, 1024) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtIPCDAProcThread::finalize()
{
    return this->join();
}

/* cm connect 성공후 ctx와 cmlinkIPCDA 에서 pid값을 넘겨 받는다. */
IDE_RC mmtIPCDAProcThread::addIPCDAProcInfo(cmiProtocolContext *aCtx)
{
    lock();
    IDE_TEST( mIPCDAClientInfoHash.insert((vULong)aCtx, aCtx) != IDE_SUCCESS );

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    unlock();

    return IDE_FAILURE;
}

/* cm disconnect 성공후 ctx와 cmlinkIPCDA 에서 pid값을 넘겨 받는다. */
IDE_RC mmtIPCDAProcThread::delIPCDAProcInfo(cmiProtocolContext *aCtx)
{
    cmiProtocolContext * sCtx  = NULL;

    lock();
    sCtx = (cmiProtocolContext *)mIPCDAClientInfoHash.search((vULong)aCtx);

    if (sCtx != NULL)
    {
        IDE_TEST( mIPCDAClientInfoHash.remove((vULong)aCtx) != IDE_SUCCESS );
    }

    unlock();
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    unlock();

    return IDE_SUCCESS;
}

IDE_RC mmtIPCDAProcThread::checkProc(vULong /* aKey */, void *aData, void */* aVisitorArg */)
{
    acp_bool_t aIsAlive;
    cmiProtocolContext *sCtx = NULL;
    UInt                sPID = 0;

    sCtx = (cmiProtocolContext*)aData;

    if( sCtx->mIsDisconnect == ID_TRUE )
    {
        /* do nothing */
    }
    else
    {
        sPID = ((cmnLinkPeerIPCDA *)sCtx->mLink)->mClientPID;
        IDE_TEST( acpProcIsAliveByPID( sPID, &aIsAlive ) != ACP_RC_SUCCESS );
    
        if( aIsAlive )
        {
            /* do nothing */
        }
        else
        {
            /* ctx->mIsDisconnect 를 true로 설정해준다. */
            sCtx->mIsDisconnect = ID_TRUE;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mmtIPCDAProcThread::run()
{
    PDL_Time_Value sSleepTime;

    /* 1 sec */
    sSleepTime.set(1, 0);
    mRun = ID_TRUE;

    while (mRun == ID_TRUE)
    {
        idlOS::sleep(sSleepTime);
        
        lock();
        mIPCDAClientInfoHash.traverse( mmtIPCDAProcThread::checkProc, NULL );
        unlock();
    }

    mIPCDAClientInfoHash.destroy();
    mLatch.destroy();

    return;
}

void mmtIPCDAProcThread::stop()
{
    mRun = ID_FALSE;
}
