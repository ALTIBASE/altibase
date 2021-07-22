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

#include <mmdXid.h>
#include <mmdXidManager.h>
#include <ida.h>
#include <mmuProperty.h>
#include <mmcTrans.h>


iduMemPool         mmdXidManager::mPool;
iduMemPool         mmdXidManager::mPool4IdXidNode;
mmdXidHashBucket*  mmdXidManager::mHash;
iduMemPool         mmdXidManager::mXidMutexPool;


IDE_RC mmdXidManager::initialize()
{
    UInt i;
    /* bug-35381 XID_MEMPOOL_ELEMENT_COUNT */
    UInt sXidMemPoolElementCount = mmuProperty::getXidMemPoolElementCount();
    UInt sXaHashSize             = mmuProperty::getXAHashSize();
    UInt sXidMutexPoolSize       = mmuProperty::getXidMutexPoolSize();

    IDE_TEST(mPool.initialize(IDU_MEM_MMD,
                              (SChar *)"MMD_XID_POOL",
                              ID_SCALABILITY_SYS,
                              ID_SIZEOF(mmdXid),
                              sXidMemPoolElementCount,
                              IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                              ID_TRUE,							/* UseMutex */
                              IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                              ID_FALSE,							/* ForcePooling */
                              ID_TRUE,							/* GarbageCollection */
                              ID_TRUE,                          /* HWCacheLine */
                              IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
                  != IDE_SUCCESS);			
    IDE_TEST(mPool4IdXidNode.initialize(IDU_MEM_MMD,
                              (SChar *)"MMD_POOL4_IDXID_NODE",
                              ID_SCALABILITY_SYS,
                              ID_SIZEOF(mmdIdXidNode),
                              sXidMemPoolElementCount,
                              IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                              ID_TRUE,							/* UseMutex */
                              IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                              ID_FALSE,							/* ForcePooling */
                              ID_TRUE,							/* GarbageCollection */
                              ID_TRUE,                          /* HWCacheLine */
                              IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
              != IDE_SUCCESS);			

    // bug-35382: mutex optimization during alloc and dealloc
    // 최종 pool size 는 XaHashSize * XidMutexPoolSize
    IDE_TEST(mXidMutexPool.initialize(IDU_MEM_MMD,
                              (SChar *)"MMD_XID_MUTEX_POOL",
                              ID_SCALABILITY_SYS,
                              ID_SIZEOF(mmdXidMutex),
                              sXaHashSize * sXidMutexPoolSize,
                              IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                              ID_TRUE,							/* UseMutex */
                              IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                              ID_FALSE,							/* ForcePooling */
                              ID_TRUE,							/* GarbageCollection */
                              ID_TRUE,                          /* HWCacheLine */
                              IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			

/* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now.
  that is to say , chanage the granularity from global to bucket level.
 */

    IDU_FIT_POINT( "mmdXidManager::initialize::malloc::Hash" );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MMD,
                               ID_SIZEOF(mmdXidHashBucket) * mmuProperty::getXAHashSize(),
                               (void**)&mHash) != IDE_SUCCESS);
    
    for(i = 0 ; i < mmuProperty::getXAHashSize(); i++)
    {
        IDE_TEST(initBucket(&(mHash[i])) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC  mmdXidManager::initBucket(mmdXidHashBucket* aBucket)
{
    UInt            sCnt;
    mmdXidMutex    *sMutex = NULL;
    UInt            sXidMutexPoolSize = mmuProperty::getXidMutexPoolSize();

    IDU_LIST_INIT(&(aBucket->mChain));

    // PROJ-2408
    IDE_ASSERT( aBucket->mBucketLatch.initialize( (SChar *)"MMD_XID_MANAGER_LATCH" )
                == IDE_SUCCESS );

    // bug-35382: mutex optimization during alloc and dealloc
    IDU_LIST_INIT(&(aBucket->mXidMutexChain));

    // PROJ-2408
    IDE_ASSERT( aBucket->mXidMutexBucketLatch.initialize( (SChar *)"MMD_XID_MUTEX_LATCH" )
                == IDE_SUCCESS );

    // mutex pool을 hash 테이블에 구성한다
    for(sCnt = 0; sCnt < sXidMutexPoolSize ; sCnt++)
    {
        IDU_FIT_POINT( "mmdXidManager::initBucket::alloc::Mutex" );

        IDE_TEST(mXidMutexPool.alloc((void **)&sMutex) != IDE_SUCCESS);
        IDE_TEST(sMutex->mMutex.initialize((SChar *)"MMD_XID_MUTEX",
                    IDU_MUTEX_KIND_POSIX,
                    IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
        IDU_LIST_INIT_OBJ(&sMutex->mListNode, sMutex);
        IDU_LIST_ADD_LAST(&(aBucket->mXidMutexChain), &(sMutex->mListNode));
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmdXidManager::finalize()
{
    UInt i = 0;
    iduListNode   *sIterator;
    iduListNode   *sNodeNext;
    mmdXidMutex   *sMutex = NULL;
    UInt           sState = 0;
    
    for(i = 0; i < mmuProperty::getXAHashSize() ; i++)
    {
        // PROJ-2408
        IDE_ASSERT( mHash[i].mBucketLatch.lockWrite(NULL, NULL) == IDE_SUCCESS);
        // free chain 
        freeBucketChain(&(mHash[i].mChain));
                   
        IDE_ASSERT( mHash[i].mBucketLatch.unlock() == IDE_SUCCESS );
        
        IDE_ASSERT( mHash[i].mBucketLatch.destroy() == IDE_SUCCESS );
    }//for


    // bug-35382: mutex optimization during alloc and dealloc
    // mutex pool (hash chain)을 날려 버린다
    for(i = 0; i < mmuProperty::getXAHashSize() ; i++)
    {
        // PROJ-2408
        IDE_ASSERT( mHash[i].mXidMutexBucketLatch.lockWrite ( NULL, NULL)  == IDE_SUCCESS );
        sState = 1;

        IDU_LIST_ITERATE_SAFE(&(mHash[i].mXidMutexChain),sIterator,sNodeNext)   
        {
            sMutex = (mmdXidMutex*)(sIterator->mObj);

            IDE_TEST(sMutex->mMutex.destroy() != IDE_SUCCESS);
            IDU_FIT_POINT("mmdXidManager::finalize::lock::memfree");
            IDE_TEST(mXidMutexPool.memfree(sMutex) != IDE_SUCCESS);
        }    

        sState = 0;
        IDE_ASSERT( mHash[i].mXidMutexBucketLatch.unlock() == IDE_SUCCESS );

        IDE_ASSERT( mHash[i].mXidMutexBucketLatch.destroy() == IDE_SUCCESS );
    }//for

    IDE_ASSERT( iduMemMgr::free(mHash) == IDE_SUCCESS);


    IDE_TEST(mPool.destroy() != IDE_SUCCESS);
    IDE_TEST(mPool4IdXidNode.destroy() != IDE_SUCCESS);
    IDE_TEST(mXidMutexPool.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    {
        switch(sState)
        {
            case 1:
                IDE_ASSERT( mHash[i].mXidMutexBucketLatch.unlock() == IDE_SUCCESS );
            default:
                break;
        }
    }

    return IDE_FAILURE;
}

void   mmdXidManager::freeBucketChain(iduList*  aChain)
{

    iduListNode   *sIterator;
    iduListNode   *sNodeNext;
    mmdXid        *sXid;
    UInt           sBucketIdx;
    
    IDU_LIST_ITERATE_SAFE(aChain,sIterator,sNodeNext)   
    {
        sXid = (mmdXid*)sIterator->mObj;
        sBucketIdx = getBucketPos(&(sXid->mXid));
        (void)sXid->finalize(&(mHash[sBucketIdx]), ID_TRUE);
        mPool.memfree(sXid);
    }    
}


/* BUG-18981 */
IDE_RC mmdXidManager::alloc(mmdXid     **aXid,
                            ID_XID      *aUserXid,
                            mmcTransObj *aTrans)
{
    mmdXid            *sXid = NULL;
    UInt               sBucketIdx;

    IDU_FIT_POINT( "mmdXidManager::alloc::alloc::Xid" );

    IDE_TEST(mPool.alloc((void **)&sXid) != IDE_SUCCESS);

    sBucketIdx = getBucketPos(aUserXid);
    IDE_TEST(sXid->initialize(aUserXid, aTrans, &(mHash[sBucketIdx]))
            != IDE_SUCCESS);

    *aXid = sXid;

    ideLog::logLine(IDE_XA_2, "XID ALLOC -> 0x%x", sXid);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sXid != NULL)
        {
            mPool.memfree(sXid);
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmdXidManager::free(mmdXid *aXid, idBool aFreeTrans)
{
    UInt               sBucketIdx;

    ideLog::logLine(IDE_XA_2, "XID FREE -> 0x%x", aXid);

    sBucketIdx = getBucketPos(&(aXid->mXid));
    IDE_TEST(aXid->finalize(&(mHash[sBucketIdx]), aFreeTrans)
            != IDE_SUCCESS);

    IDE_TEST(mPool.memfree(aXid) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}



IDE_RC mmdXidManager::add(mmdXid *aXid,UInt aBucket)
{
    
    aXid->fix();
    /* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
       that is to say , chanage the granularity from global to bucket level.
    */           
    IDU_LIST_ADD_LAST(&(mHash[aBucket].mChain),aXid->getLstNode());
    
    
    ideLog::logLine(IDE_XA_2, "XID ADD -> 0x%x", aXid);

    return IDE_SUCCESS;
}

IDE_RC mmdXidManager::remove(mmdXid *aXid,UInt *aFixCount)
{
    /* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
  that is to say , chanage the granularity from global to bucket level. */           
    IDU_LIST_REMOVE(aXid->getLstNode());
    
    /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
     XA Unfix 시에 latch duaration을 줄이기위하여 xid fix-Count를 xid list latch release전에
     구한다.*/
    aXid->unfix(aFixCount);

    ideLog::logLine(IDE_XA_2, "XID REMOVE -> 0x%x", aXid);

    return IDE_SUCCESS;
}

/* BUG-18981 */
IDE_RC mmdXidManager::find(mmdXid **aXid, ID_XID *aUserXid, UInt aBucket,mmdXaLogFlag aXaLogFlag)
{
    iduList      *sChain;
    iduListNode  *sIterator;
    mmdXid       *sXid;
    SInt          sRet;
    
    *aXid = NULL;
/* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
  that is to say , chanage the granularity from global to bucket level. */               
    sChain =&(mHash[aBucket].mChain);
    IDU_LIST_ITERATE(sChain,sIterator)
    {
        sXid = (mmdXid*)sIterator->mObj;
        sRet = mmdXid::compFunc(aUserXid,sXid->getXid());
        if(sRet == 0)
        {
            *aXid = sXid;
            break;
        }//if 
    }//IDU_LIST 
    
    if( aXaLogFlag == MMD_XA_DO_LOG)
    {
        
        ideLog::logLine(IDE_XA_2, "XID FIND -> 0x%x", *aXid);
    }
    return IDE_SUCCESS;
}

//fix BUG-21794
IDE_RC mmdXidManager::alloc(mmdIdXidNode **aXidNode, ID_XID *aXid)
{
    mmdIdXidNode* sXidNode = NULL;

    IDU_FIT_POINT( "mmdXidManager::alloc::alloc::XidNode" );

    IDE_TEST(mPool4IdXidNode.alloc((void **)&sXidNode) != IDE_SUCCESS);

    idlOS::memcpy(&(sXidNode->mXID), aXid, ID_SIZEOF(ID_XID));
    
    IDU_LIST_INIT_OBJ(&(sXidNode->mLstNode),sXidNode );

    *aXidNode = sXidNode;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
//fix BUG-21794
IDE_RC mmdXidManager::free(mmdIdXidNode *aXidNode)
{
    
    IDE_TEST(mPool4IdXidNode.memfree(aXidNode) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

UInt mmdXidManager::getBucketPos(ID_XID *aXid)
{
    vULong sHashVal;
    
    sHashVal = mmdXid::hashFunc(aXid);
    return (sHashVal % mmuProperty::getXAHashSize());
}

//fix BUG-22669 XID list에 대한 performance view 필요.
IDE_RC  mmdXidManager::buildRecordForXID(idvSQL               * /* aStatistics */,
                                         void                 *aHeader,
                                         void                 */*aDummyObj*/,
                                         iduFixedTableMemory  *aMemory)
{
    mmdXid             *sXid = NULL;
    UInt               sState = 0;
    UInt               i;
    mmcTransObj       *sTrans;
    mmdXidInfo4PerfV   sXidInfo;
    iduListNode       *sIterator;
    
/* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
  that is to say , chanage the granularity from global to bucket level.
 */               
    for( i = 0; i < mmuProperty::getXAHashSize() ; i++)
    {
            
        IDE_ASSERT(mmdXidManager::lockRead(i) == IDE_SUCCESS);
        sState = 1;

        IDU_LIST_ITERATE(&(mHash[i].mChain),sIterator)
        {
            sXid = (mmdXid*)sIterator->mObj;
            //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
            //그들간의 관계를 정확히 유지해야 함.
            // build record에서 XID의 트랜잭션 ID를 assign함.
            sTrans = sXid->getTransPtr();
            if(sTrans != NULL)
            {
                sXidInfo.mTransID = mmcTrans::getTransID(sTrans);
            }
            else
            {
                sXidInfo.mTransID = 0; /* BUG-44967 */
            }
            
            idlOS::memcpy(&(sXidInfo.mXIDValue), sXid->getXid(), ID_SIZEOF(ID_XID) );
            sXidInfo.mAssocSessionID = sXid->mAssocSessionID;
            sXidInfo.mState          = sXid->mState;
            
            //BUG-25078 - State가  시작된 시각과 Duration 
            sXidInfo.mStateStartTime = sXid->mStateStartTime;
            sXidInfo.mStateDuration  = mmtSessionManager::getBaseTime() - sXid->mStateStartTime;
            
            sXidInfo.mBeginFlag      = sXid->mBeginFlag;
            sXidInfo.mFixCount    = sXid->mFixCount;
            IDE_TEST(iduFixedTable::buildRecord(aHeader, aMemory,(void*)&sXidInfo)
                     != IDE_SUCCESS);
        }
        sState = 0;
        IDE_ASSERT(mmdXidManager::unlock(i) == IDE_SUCCESS);
    }//for i


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch(sState)
        {
            case 1:
                IDE_ASSERT(mmdXidManager::unlock(i) == IDE_SUCCESS);
            default:
                break;
        }
    }
    
    return IDE_FAILURE;
}



static iduFixedTableColDesc gXidColDesc[]=
{
    //XID VALUE
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    {
        (SChar *)"XID_VALUE",
        offsetof(mmdXidInfo4PerfV,mXIDValue),
        XID_DATA_MAX_LEN,
        IDU_FT_TYPE_VARCHAR,
        idaXaConvertXIDToString,
        0, 0, NULL // for internal use
        },
    //ASSOC_SESSION_ID
    {
        (SChar *)"ASSOC_SESSION_ID",
        offsetof(mmdXidInfo4PerfV,mAssocSessionID),
        IDU_FT_SIZEOF(mmdXidInfo4PerfV,mAssocSessionID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    //TRANS_ID
    {
        (SChar *)"TRANS_ID",
        offsetof(mmdXidInfo4PerfV,mTransID),
        IDU_FT_SIZEOF(mmdXidInfo4PerfV,mTransID),
        IDU_FT_TYPE_UBIGINT,    // BUG-47379 unsigned int -> bit int
        NULL,
        0, 0, NULL // for internal use
    },
    
    {
        (SChar *)"STATE",
        offsetof(mmdXidInfo4PerfV,mState),
        IDU_FT_SIZEOF(mmdXidInfo4PerfV,mState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    
    //BUG-25078 현재 State가 시작된 시각
    {
        (SChar *)"STATE_START_TIME",
        offsetof(mmdXidInfo4PerfV,mStateStartTime),
        IDU_FT_SIZEOF(mmdXidInfo4PerfV,mStateStartTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    
    {
        (SChar *)"STATE_DURATION",
        offsetof(mmdXidInfo4PerfV,mStateDuration),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
        
    {
        (SChar *)"TX_BEGIN_FLAG",
        offsetof(mmdXidInfo4PerfV,mBeginFlag),
        IDU_FT_SIZEOF(mmdXidInfo4PerfV,mBeginFlag),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },

    {
        (SChar *)"REF_COUNT",
        offsetof(mmdXidInfo4PerfV,mFixCount),
        IDU_FT_SIZEOF(mmdXidInfo4PerfV,mFixCount),
        IDU_FT_TYPE_UINTEGER,
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

iduFixedTableDesc   gXidTableDesc =
{
    (SChar*)"X$XID",
    mmdXidManager::buildRecordForXID,
    gXidColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
