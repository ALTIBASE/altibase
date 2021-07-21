/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemMgrFT.cpp 81932 2017-12-20 09:47:16Z yoonhee.kim $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduMemList.h>
#include <iduMemMgr.h>
#include <idp.h>
#include <idtContainer.h>

class idvSQL;

/* ------------------------------------------------
 *  Fixed Table Define for  MemoryMgr
 * ----------------------------------------------*/

static iduFixedTableColDesc gMemoryMgrColDesc[] =
{
    {
        (SChar *)"OWNER",
        IDU_FT_OFFSETOF(iduMemClientInfo, mOwner),
        64,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"MODULE",
        IDU_FT_OFFSETOF(iduMemClientInfo, mModule),
        IDU_FT_SIZEOF(iduMemClientInfo, mModule),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NAME",
        IDU_FT_OFFSETOF(iduMemClientInfo, mName),
        IDU_FT_SIZEOF(iduMemClientInfo, mName),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ALLOC_SIZE",
        IDU_FT_OFFSETOF(iduMemClientInfo, mAllocSize),
        IDU_FT_SIZEOF(iduMemClientInfo, mAllocSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ALLOC_COUNT",
        IDU_FT_OFFSETOF(iduMemClientInfo, mAllocCount),
        IDU_FT_SIZEOF(iduMemClientInfo, mAllocCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    //===============================================================
    // To Fix PR-13959
    // 현재까지 사용한 최대 메모리 사용량
    //===============================================================
    {
        (SChar *)"MAX_TOTAL_SIZE",
        IDU_FT_OFFSETOF(iduMemClientInfo, mMaxTotSize),
        IDU_FT_SIZEOF(iduMemClientInfo, mMaxTotSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};


static IDE_RC buildRecordForMemoryMgr(idvSQL      *,
                                      void        *aHeader,
                                      void        * /* aDumpObj */,
                                      iduFixedTableMemory *aMemory)
{
    idtContainer*       sContainer;
    iduMemTlsf*         sAlloc;
    ULong               sNeedRecCount;
    UInt                i;

    sNeedRecCount = IDU_MAX_CLIENT_COUNT;

    if ( idtContainer::getThreadReuseEnable() == ID_FALSE )
    {
        IDE_ASSERT( idtContainer::mMainThread.mInfoLock.lock( NULL ) == IDE_SUCCESS );
    }
    else
    {
        /* Do nothing */
    }
    sContainer = idtContainer::getFirstInfo();


    while(sContainer != NULL)
    {
        for (i = 0; i < sNeedRecCount; i++)
        {
            // To Fix BUG-16821 select name from v$memstat 시 공백 행이 나옴
            //   => iduMemoryClientIndex 만 추가하고
            //      mClientInfo를 추가하지 않은 경우를
            //      IDE_DASSERT로 Detect하기 위함 
            IDE_DASSERT( (UInt)sContainer->mMemInfo[i].mClientIndex == i );

            IDE_TEST_RAISE( iduFixedTable::buildRecord(aHeader,
                                                       aMemory,
                                                       (void *) &sContainer->mMemInfo[i])
                            != IDE_SUCCESS, ECONTERROR);
        }

        sContainer = sContainer->getNextInfo();
    }

    if ( idtContainer::getThreadReuseEnable() == ID_FALSE )
    {
        IDE_ASSERT( idtContainer::mMainThread.mInfoLock.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* Do nothing */
    }

    IDE_ASSERT( idlOS::thread_mutex_lock( &(iduMemMgr::mAllocListLock) ) == 0 );
    switch(iduMemMgr::getAllocatorType())
    {
    /* 
     * BUG-44183
     * when new memory allocator type would be added
     */
    /* case IDU_MEMMGR_SOMETHING?: */
    case IDU_MEMMGR_TLSF:
        sAlloc = (iduMemTlsf*)iduMemMgr::mAllocList.mNext;

        while(sAlloc != &(iduMemMgr::mAllocList))
        {
            IDE_ASSERT(sAlloc->lock() == IDE_SUCCESS);
            sAlloc->updateReserved();

            for (i = 0; i < sNeedRecCount; i++)
            {
                // To Fix BUG-16821 select name from v$memstat 시 공백 행이 나옴
                //   => iduMemoryClientIndex 만 추가하고
                //      mClientInfo를 추가하지 않은 경우를
                //      IDE_DASSERT로 Detect하기 위함 
                IDE_DASSERT( (UInt)sAlloc->mMemInfo[i].mClientIndex == i );

                IDE_TEST_RAISE( iduFixedTable::buildRecord(aHeader,
                            aMemory,
                            (void *) &sAlloc->mMemInfo[i])
                        != IDE_SUCCESS, EBUILDERROR);
            }

            IDE_ASSERT(sAlloc->unlock() == IDE_SUCCESS);
            sAlloc = (iduMemTlsf*)sAlloc->mNext;
        }
        break;

    default:
        break;
    }

    IDE_ASSERT(idlOS::thread_mutex_unlock(&(iduMemMgr::mAllocListLock)) == 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ECONTERROR )
    {
        if ( idtContainer::getThreadReuseEnable() == ID_FALSE )
        {
            IDE_ASSERT( idtContainer::mMainThread.mInfoLock.unlock() == IDE_SUCCESS );
        }
        else
        {
            /* Do nothing */
        }
    }
    IDE_EXCEPTION(EBUILDERROR)
    {
        IDE_ASSERT(sAlloc->unlock() == IDE_SUCCESS);
        IDE_ASSERT(idlOS::thread_mutex_unlock(&(iduMemMgr::mAllocListLock)) == 0);
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gMemoryMgrTableDesc =
{
    (SChar *)"X$MEMSTAT",
    buildRecordForMemoryMgr,
    gMemoryMgrColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


static iduFixedTableColDesc gMemAllocColDesc[] =
{
    {
        (SChar *)"INSTANCE",
        IDU_FT_OFFSETOF(iduMemAllocCore, mAddr),
        IDU_FT_SIZEOF(iduMemAllocCore, mAddr),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ALLOC_NAME",
        IDU_FT_OFFSETOF(iduMemAllocCore, mName),
        IDU_FT_SIZEOF(iduMemAllocCore, mName),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ALLOC_TYPE",
        IDU_FT_OFFSETOF(iduMemAllocCore, mType),
        IDU_FT_SIZEOF(iduMemAllocCore, mType),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"USED_SIZE",
        IDU_FT_OFFSETOF(iduMemAllocCore, mUsedSize),
        IDU_FT_SIZEOF(iduMemAllocCore, mUsedSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"POOL_SIZE",
        IDU_FT_OFFSETOF(iduMemAllocCore, mPoolSize),
        IDU_FT_SIZEOF(iduMemAllocCore, mPoolSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

static IDE_RC buildRecordForAllocs(idvSQL      *,
                                   void        *aHeader,
                                   void        * /* aDumpObj */,
                                   iduFixedTableMemory *aMemory)
{
    iduMemAlloc*        sAlloc;
    iduMemAllocCore*    sInfo;

    IDE_ASSERT(idlOS::thread_mutex_lock(&(iduMemMgr::mAllocListLock)) == 0);
    sAlloc = (iduMemAlloc*)iduMemMgr::mAllocList.mNext;

    while(sAlloc != (iduMemAlloc*)&(iduMemMgr::mAllocList))
    {
        IDE_ASSERT(sAlloc->lock() == IDE_SUCCESS);
        sInfo = (iduMemAllocCore*)sAlloc;
            
        IDE_TEST_RAISE( iduFixedTable::buildRecord(aHeader,
                                                   aMemory,
                                                   (void *)sInfo)
                        != IDE_SUCCESS, EBUILDERROR);

        IDE_ASSERT(sAlloc->unlock() == IDE_SUCCESS);
        sAlloc = (iduMemAlloc*)sAlloc->mNext;
    }

    IDE_ASSERT(idlOS::thread_mutex_unlock(&(iduMemMgr::mAllocListLock)) == 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EBUILDERROR)
    {
        if(sAlloc != (iduMemAlloc*)(&iduMemMgr::mAllocList))
        {
            IDE_ASSERT(sAlloc->unlock() == IDE_SUCCESS);
        }
        else
        {
            /* fall through */
        }

        IDE_ASSERT(idlOS::thread_mutex_unlock(&(iduMemMgr::mAllocListLock)) == 0);
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableDesc gMemAllocTableDesc =
{
    (SChar *)"X$MEMALLOC",
    buildRecordForAllocs,
    gMemAllocColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


