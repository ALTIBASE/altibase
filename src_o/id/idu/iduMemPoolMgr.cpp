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
 
/**********************************************************************
 * $Id: iduMemPoolMgr.cpp 80575 2017-07-21 07:06:35Z yoonhee.kim $
 **********************************************************************/
#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideLog.h>
#include <iduMemList.h>
#include <iduMemPool.h>
#include <iduMemPoolMgr.h>

iduMemPool * iduMemPoolMgr::mListHead;
iduMutex     iduMemPoolMgr::mMutex;
UInt         iduMemPoolMgr::mMemPoolCnt;

IDE_RC iduMemPoolMgr::initialize()
{
    IDE_ASSERT( IDU_USE_MEMORY_POOL == 1 );
    
    IDE_TEST(mMutex.initialize( (SChar*)"IDU_MEMPOOLMGR_MUTEX",
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
    mMemPoolCnt = 0;
    mListHead = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemPoolMgr::destroy()
{
    IDE_ASSERT( IDU_USE_MEMORY_POOL == 1 );

    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * mempool list가 이상이 없는지 check한다.
 * iduMemPoolMgr::mMutex가 잡혀있는상태에서 호출되어야함.
 */
void iduMemPoolMgr::verifyPoolList(void)
{
    iduMemPool * sCurr=NULL;
    UInt         t=0;

    IDE_ASSERT( IDU_USE_MEMORY_POOL == 1 );

    sCurr = mListHead;
    if(sCurr == NULL)
    {
        t = 0;
    }
    else
    {
        do
        {
            sCurr = sCurr->mNext;
            t++;
        } while( sCurr != mListHead );
    }
    IDE_ASSERT( t == mMemPoolCnt );
}

/*
 *  mem pool list에 새로운 mem pool을 추가한다.
 */
IDE_RC iduMemPoolMgr::addMemPool(iduMemPool * aMemPool)
{
    IDE_ASSERT( IDU_USE_MEMORY_POOL == 1 );
    IDE_ASSERT( aMemPool != NULL );
    IDE_ASSERT( aMemPool->mFlag & IDU_MEMPOOL_USE_MUTEX );

    IDE_TEST( mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS);

    if(mListHead == NULL)
    {
        mListHead = aMemPool;
        mListHead->mPrev = mListHead;
        mListHead->mNext = mListHead;
    }
    else
    {
        aMemPool->mNext = mListHead;
        aMemPool->mPrev = mListHead->mPrev;

        mListHead->mPrev->mNext = aMemPool;
        mListHead->mPrev = aMemPool;
    }
    mMemPoolCnt++;

#ifdef MEMORY_ASSERT
    verifyPoolList();
#endif

    IDE_TEST( mMutex.unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 *  mem pool list에서 인자로 받은 mem pool을 제거한다.
 */
IDE_RC iduMemPoolMgr::delMemPool(iduMemPool * aMemPool)
{
    IDE_ASSERT( IDU_USE_MEMORY_POOL == 1 );
    IDE_ASSERT( aMemPool != NULL );
    IDE_ASSERT( aMemPool->mFlag & IDU_MEMPOOL_USE_MUTEX );

    IDE_TEST( mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS);

    if(aMemPool == mListHead)
    {
        if(mListHead->mNext == mListHead)
        {
            mListHead = NULL;
        }
        else
        {
            /* unlink */
            mListHead->mPrev->mNext = mListHead->mNext;
            mListHead->mNext->mPrev = mListHead->mPrev;
            mListHead = mListHead->mNext;
        }
    }
    else
    {
        /* unlink */
        aMemPool->mPrev->mNext = aMemPool->mNext;
        aMemPool->mNext->mPrev = aMemPool->mPrev;
    }
    mMemPoolCnt--;

#ifdef MEMORY_ASSERT
    verifyPoolList();
#endif

    IDE_TEST( mMutex.unlock() != IDE_SUCCESS);
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * mem pool list에서 free chunk들을 모두 OS에 반납한다.
 */
IDE_RC iduMemPoolMgr::shrinkMemPools(void)
{
    iduMemPool * sCur=NULL;
    UInt         t=0;
    UInt         sSize=0;
    UInt         sSum=0;

    IDE_TEST_RAISE( (iduMemMgr::isServerMemType() == ID_FALSE) || (IDU_USE_MEMORY_POOL == 0),
                    no_mempoolmgr);

    IDE_TEST( mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS);

    sCur = mListHead;//mListHead는 연결된 첫번째 mem pool을 가르키고 있음.
    if(sCur == NULL)
    {
        t = 0;
    }
    else
    {
        do
        {
            IDE_TEST( sCur->shrink( &sSize ) != IDE_SUCCESS);
            sSum += sSize;
            t++;
            sCur = sCur->mNext;
        } while( sCur != mListHead );
    }
    IDE_ASSERT( t == mMemPoolCnt );

    IDE_TEST( mMutex.unlock() != IDE_SUCCESS);

    IDE_EXCEPTION_CONT( no_mempoolmgr );

    ideLog::log( IDE_SERVER_0, 
                 "Freeing unused memory: %u bytes freed\n",sSum );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 *  X$MEMPOOL 
 */
static iduFixedTableColDesc gDumpMemPoolColDesc[]=
{
    {
        (SChar*)"NAME",
        offsetof( iduMemPoolInfo  , mName ),
        IDU_FT_SIZEOF(iduMemPoolInfo , mName ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"WHERE",
        offsetof( iduMemPoolInfo  , mWhere ),
        IDU_FT_SIZEOF(iduMemPoolInfo , mWhere ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"MEMLIST_COUNT",
        offsetof(iduMemPoolInfo  , mMemListCnt ),
        IDU_FT_SIZEOF(iduMemPoolInfo , mMemListCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_CNT",
        offsetof( iduMemPoolInfo , mFreeCnt ),
        IDU_FT_SIZEOF( iduMemPoolInfo, mFreeCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FULL_CNT",
        offsetof( iduMemPoolInfo , mFullCnt ),
        IDU_FT_SIZEOF( iduMemPoolInfo, mFullCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARTIAL_CNT",
        offsetof( iduMemPoolInfo , mPartialCnt ),
        IDU_FT_SIZEOF( iduMemPoolInfo, mPartialCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CHUNK_LIMIT",
        offsetof( iduMemPoolInfo , mChunkLimit ),
        IDU_FT_SIZEOF( iduMemPoolInfo, mChunkLimit ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CHUNK_SIZE",
        offsetof( iduMemPoolInfo , mChunkSize ),
        IDU_FT_SIZEOF( iduMemPoolInfo, mChunkSize ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ELEM_CNT",
        offsetof( iduMemPoolInfo , mElemCnt ),
        IDU_FT_SIZEOF( iduMemPoolInfo, mElemCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ELEM_SIZE",
        offsetof( iduMemPoolInfo , mElemSize ),
        IDU_FT_SIZEOF( iduMemPoolInfo, mElemSize ),
        IDU_FT_TYPE_UBIGINT,
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

iduFixedTableDesc  gDumpMemPoolDesc =
{
    (SChar *)"X$MEMPOOL",
    iduMemPoolMgr::buildRecord4MemPool,
    gDumpMemPoolColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC iduMemPoolMgr::buildRecord4MemPool(
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * /*aDumpObj*/,
    iduFixedTableMemory * aMemory )
{
    UInt             i=0;
    iduMemPool      *sPool;
    iduMemList      *sList;

    struct iduMemPoolInfo sInfo;

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );

    IDE_TEST_RAISE( (iduMemMgr::isServerMemType() == ID_FALSE) || (IDU_USE_MEMORY_POOL == 0),
                    no_mempoolmgr);

    IDE_TEST( mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS);
    sPool = mListHead;
    if(sPool == NULL)
    {
    }
    else
    {
        do
        {
            idlOS::memcpy( sInfo.mName, sPool->mName, IDU_MEM_POOL_NAME_LEN );
            sInfo.mName[IDU_MEM_POOL_NAME_LEN - 1] = 0; //just in case...
            sInfo.mWhere      = sPool->mIndex;
            sInfo.mMemListCnt = sPool->mListCount;

            while ( i <  sInfo.mMemListCnt )
            {
                sList = sPool->mMemList[i];


                sList->fillMemPoolInfo( &sInfo );

                IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                      aMemory,
                                                      (void *)&sInfo )
                          != IDE_SUCCESS );

                i++;
            }

            i = 0;
            sPool = sPool->mNext;
        } while( sPool != mListHead );
    }
    IDE_TEST( mMutex.unlock() != IDE_SUCCESS);

    IDE_EXCEPTION_CONT( no_mempoolmgr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
