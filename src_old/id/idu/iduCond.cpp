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
 
/***********************************************************************
 * $Id: iduCond.cpp 17293 2006-07-25 03:04:24Z sbjang $
 **********************************************************************/

#include <iduProperty.h>
#include <iduCond.h>
#include <iduMemMgr.h>
#include <iduFixedTable.h>

PDL_thread_mutex_t  iduCond::mCondMutex;
PDL_thread_mutex_t  iduCond::mInfoMutex;
iduCondCore         iduCond::mHead;
iduPeerType         iduCond::mCondMgrType = IDU_CLIENT_TYPE;
iduMemSmall         iduCond::mCondPool;
idBool              iduCond::mUsePool;

#define IDU_COND_INITIALIZED    0
#define IDU_COND_OCCUPIED       1
#define IDU_COND_WAIT           2
#define IDU_COND_TIMEDWAIT      3
#define IDU_COND_IDLE           4

class idvSQL;

static SChar* gConditionVariableStatus[] = 
{
    (SChar*)"INITIALIZED",
    (SChar*)"OCCUPIED",
    (SChar*)"WAIT",
    (SChar*)"TIMED WAIT",
    (SChar*)"IDLE"
};

IDE_RC iduCond::allocCond(iduCondCore** aCore)
{
    if(mUsePool == ID_TRUE)
    {
        IDE_TEST(mCondPool.malloc(IDU_MEM_ID_COND_MANAGER,
                                  sizeof(iduCondCore),
                                  (void**)aCore) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemMgr::malloc(IDU_MEM_ID_COND_MANAGER,
                                   sizeof(iduCondCore),
                                   (void**)aCore) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduCond::freeCond(iduCondCore* aCore)
{
    if(mUsePool == ID_TRUE)
    {
        IDE_TEST(mCondPool.free(aCore) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemMgr::free(aCore) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduCond::initializeStatic(iduPeerType aCondMgrType)
{
    mCondMgrType = aCondMgrType;

    if(mCondMgrType == IDU_SERVER_TYPE)
    {
        IDE_TEST( idlOS::thread_mutex_init( &mInfoMutex ) != 0 );
        IDE_TEST( idlOS::thread_mutex_init( &mCondMutex ) != 0 );

        mHead.mCondNext = NULL;
        mHead.mInfoNext = NULL;

        if(iduMemMgr::isUseResourcePool() == ID_TRUE)
        {
            mUsePool = ID_TRUE;
            IDE_TEST( mCondPool.initialize((SChar*)"CONDVAR_POOL",
                                           iduProperty::getMemoryPrivatePoolSize()
                                          ) != IDE_SUCCESS );
        }
        else
        {
            mUsePool = ID_FALSE;
        }
    }
    else
    {
        mUsePool = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduCond::destroyStatic()
{
    if(mCondMgrType == IDU_SERVER_TYPE)
    {
        iduCondCore* sCond;
        iduCondCore* sNext;

        sCond = iduCond::getFirstCond();
        sCond = iduCond::getNextCond(sCond);

        while(sCond != NULL)
        {
            sNext = iduCond::getNextCond(sCond);
            if(idlOS::cond_destroy(&(sCond->mCond)) != 0)
            {
                ideLog::logMessage(IDE_SERVER_0,
                                   "Error destroying condition variable %p.",
                                   &(sCond->mCond));
            }
            (void)freeCond(sCond);

            sCond = sNext;
        }

        if(mUsePool == ID_TRUE)
        {
            IDE_TEST( mCondPool.destroy() != IDE_SUCCESS );
            mUsePool = ID_FALSE;
        }
        else
        {
            /* fall through */
        }
        IDE_TEST( idlOS::thread_mutex_destroy( &mInfoMutex ) != 0 );
        IDE_TEST( idlOS::thread_mutex_destroy( &mCondMutex ) != 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduCond::initialize( SChar * aName )
{
    iduCondCore*    sCore;
     
    if(mCondMgrType == IDU_SERVER_TYPE)
    {
        IDE_TEST( iduCond::lockCond() != 0 );
        sCore = mHead.mCondNext;
        if(sCore != NULL)
        {
            mHead.mCondNext = sCore->mCondNext;
            sCore->mCondStatus = IDU_COND_OCCUPIED;
        }
        IDE_TEST( iduCond::unlockCond() != 0 );

        /* allocate a new condition variable */
        if(sCore == NULL)
        {
            IDE_TEST_RAISE(allocCond(&sCore) != IDE_SUCCESS,
                           ENOTENOUGHMEMORY);

            IDE_TEST( lockInfo() != 0 );
            sCore->mInfoNext = mHead.mInfoNext;
            mHead.mInfoNext = sCore;
            IDE_TEST( unlockInfo() != 0 );

            sCore->mCondStatus = IDU_COND_INITIALIZED;
        }
    }
    else
    {
        IDE_TEST_RAISE(allocCond(&sCore) != IDE_SUCCESS,
                       ENOTENOUGHMEMORY);
    }

    IDE_TEST_RAISE(idlOS::cond_init( &(sCore->mCond), (short)USYNC_THREAD ) != 0,
                   ECONDINITFAILED);

    if(aName == NULL)
    {
        aName = (SChar*)"UNNAMED";
    }
    idlOS::snprintf( sCore->mName, IDU_COND_NAME_SIZE, "%s", aName );
    sCore->mIsTimedOut = ID_FALSE;

    mCore = sCore;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ENOTENOUGHMEMORY)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_MEMORY_ALLOCATION, 
                                "iduCond::initialize"));
    }

    IDE_EXCEPTION(ECONDINITFAILED)
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondInit));
        IDE_ASSERT(freeCond(sCore) == IDE_SUCCESS);
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduCond::destroy()
{
    if((mCore->mCondStatus == IDU_COND_WAIT) ||
       (mCore->mCondStatus == IDU_COND_TIMEDWAIT))
    {
        ideLog::logMessage(IDE_SERVER_0,
                           "Warning : trying to destroy "
                           "busy condition variable %p(%p).\n",
                           this, mCore);
    }
    else
    {
        if(mCore->mCondStatus == IDU_COND_IDLE)
        {
            ideLog::logMessage(IDE_SERVER_0,
                               "Warning : duplicated destruction of "
                               "condition variable %p(%p).\n",
                               this, mCore);
        }
        else
        {
            /* fall through */
        }
    }

    if(mCondMgrType == IDU_SERVER_TYPE)
    {
        mCore->mCondStatus = IDU_COND_IDLE;
        mCore->mMutex = NULL;

        idlOS::strcpy(mCore->mName, "UNUSED");

        IDE_TEST( iduCond::lockCond() != 0 );
        mCore->mCondNext = mHead.mCondNext;
        mHead.mCondNext = mCore;
        IDE_TEST( iduCond::unlockCond() != 0 );
    }
    else
    {
        IDE_TEST(iduMemMgr::free(mCore) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;


    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduCond::signal(void)
{
    IDE_TEST(idlOS::cond_signal(&(mCore->mCond)) != 0);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondSignal));
    return IDE_FAILURE;
}

IDE_RC iduCond::broadcast(void)
{
    IDE_TEST(idlOS::cond_broadcast(&(mCore->mCond)) != 0);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduCond::wait(iduMutex* aMutex)
{
    mCore->mCondStatus = IDU_COND_WAIT;
    mCore->mIsTimedOut = ID_FALSE;
    mCore->mMutex = (void*)aMutex->getMutexForCond();
    IDE_TEST(idlOS::cond_wait(&(mCore->mCond),
                              (PDL_thread_mutex_t*)mCore->mMutex)
             != 0);
    mCore->mCondStatus = IDU_COND_OCCUPIED;
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondWait));
    return IDE_FAILURE;
}

IDE_RC iduCond::timedwait(iduMutex* aMutex,
                          PDL_Time_Value* aDuration,
                          idBool aIgnoreTimedOut)
{
    int sRC;

    mCore->mCondStatus = IDU_COND_WAIT;
    mCore->mIsTimedOut = ID_FALSE;

    mCore->mMutex = (void*)aMutex->getMutexForCond();
    sRC = idlOS::cond_timedwait(&(mCore->mCond),
                                (PDL_thread_mutex_t*)mCore->mMutex,
                                aDuration);
    if(sRC != 0)
    {
        if(sRC == ETIME || (sRC == -1 && errno == ETIME))
        {
            mCore->mIsTimedOut = ID_TRUE;

            if(aIgnoreTimedOut == IDU_IGNORE_TIMEDOUT)
            {
                sRC = 0;
            }
        }
    }

    mCore->mCondStatus = IDU_COND_OCCUPIED;

    IDE_TEST(sRC != 0);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondWait));
    return IDE_FAILURE;
}

SInt iduCond::lockCond()
{
    IDE_ASSERT(mCondMgrType == IDU_SERVER_TYPE);
    return idlOS::thread_mutex_lock( &mCondMutex );
}

SInt iduCond::unlockCond()
{
    IDE_ASSERT(mCondMgrType == IDU_SERVER_TYPE);
    return idlOS::thread_mutex_unlock( &mCondMutex );
}

SInt iduCond::lockInfo()
{
    IDE_ASSERT(mCondMgrType == IDU_SERVER_TYPE);
    return idlOS::thread_mutex_lock( &mInfoMutex );
}

SInt iduCond::unlockInfo()
{
    IDE_ASSERT(mCondMgrType == IDU_SERVER_TYPE);
    return idlOS::thread_mutex_unlock( &mInfoMutex );
}

iduCondCore* iduCond::getFirstCond()
{
    IDE_ASSERT(mCondMgrType == IDU_SERVER_TYPE);
    return &(iduCond::mHead);
}

iduCondCore* iduCond::getNextCond(iduCondCore* aCond)
{
    IDE_ASSERT(mCondMgrType == IDU_SERVER_TYPE);
    return aCond->mInfoNext;
}

static iduFixedTableColDesc gCondColDesc[] =
{
    {
        (SChar *)"NAME",
        offsetof(iduCondCore, mName),
        IDU_FT_SIZEOF(iduCondCore, mName),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"STATUS",
        offsetof(iduCondCore, mCondStatusString),
        IDU_COND_STATUS_SIZE,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MUTEX_PTR",
        offsetof(iduCondCore, mMutexPtr),
        IDU_COND_PTR_SIZE,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

static IDE_RC buildRecordForCond(idvSQL      *,
                                 void        *aHeader,
                                 void        * /* aDumpObj */,
                                 iduFixedTableMemory *aMemory)
{
    iduCondCore*    sCond;

    sCond = iduCond::getFirstCond();

    while((sCond = iduCond::getNextCond(sCond)) != NULL)
    {
        sCond->mCondStatusString =
            gConditionVariableStatus[sCond->mCondStatus];
        (void)idlOS::snprintf((char*)sCond->mMutexPtr,
                              IDU_COND_PTR_SIZE,
                              "%"ID_XPOINTER_FMT,
                              (void*)sCond->mMutex);

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)sCond)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
} 

iduFixedTableDesc gCondTableDesc =
{
    (SChar *)"X$COND",
    buildRecordForCond,
    gCondColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
