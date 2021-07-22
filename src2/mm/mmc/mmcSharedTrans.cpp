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

#include <mmcSharedTrans.h>
#include <mmcTrans.h>
#include <mmcSession.h>
#include <mmuProperty.h>
#include <sdi.h>
#include <iduHashUtil.h>

iduMemPool          mmcSharedTrans::mPool;
iduList             mmcSharedTrans::mFreeTransChain;
iduLatch            mmcSharedTrans::mFreeTransLatch;
UInt                mmcSharedTrans::mFreeTransCount;
UInt                mmcSharedTrans::mFreeTransMaxCount;
UInt                mmcSharedTrans::mBucketCount;
mmcTransBucket    * mmcSharedTrans::mHash;

IDE_RC mmcSharedTrans::initialize()
{
    UInt sTransSize;
    UInt sIndex = 0;

    mBucketCount       = mmuProperty::getSharedTransHashBucketCount();
    mFreeTransMaxCount = mmuProperty::getSharedTransHashBucketCount();
    mHash        = NULL;
    sTransSize   = ID_SIZEOF(mmcTransObj) + ID_SIZEOF(mmcTransShareInfo);

    IDU_LIST_INIT( &mFreeTransChain );

    IDE_TEST( mPool.initialize( IDU_MEM_MMC,
                                (SChar *)"MMC_SHARED_TRANS_POOL",
                                ID_SCALABILITY_SYS,
                                sTransSize,
                                4,
                                IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                ID_TRUE,							/* UseMutex */
                                IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                ID_FALSE,							/* ForcePooling */
                                ID_TRUE,							/* GarbageCollection */
                                ID_TRUE,                            /* HWCacheLine */
                                IDU_MEMPOOL_TYPE_LEGACY             /* mempool type*/) 
              != IDE_SUCCESS);

    IDE_TEST( mFreeTransLatch.initialize( (SChar *)"MMC_FREE_TRANS_MUTEX",
                                          IDU_LATCH_TYPE_NATIVE )
              != IDE_SUCCESS );

    mFreeTransCount = 0;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_MMC,
                                       ID_SIZEOF(mmcTransBucket) * mBucketCount,
                                       (void**)&mHash)
                    != IDE_SUCCESS,
                    InsufficientMemory);

    for ( sIndex = 0; sIndex < mBucketCount ; ++sIndex )
    {
        IDE_TEST( initializeBucket( &mHash[ sIndex ] ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InsufficientMemory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcSharedTrans::initializeBucket( mmcTransBucket * aBucket )
{
    IDU_LIST_INIT( &(aBucket->mChain) );

    IDE_TEST( aBucket->mBucketLatch.initialize( (SChar *)"MMC_TRANS_BUCKET_MUTEX",
                                                IDU_LATCH_TYPE_NATIVE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcSharedTrans::finalize()
{
    UInt sIndex = 0;

    for ( sIndex = 0; sIndex < mBucketCount ; ++sIndex )
    {
        IDE_TEST( finalizeBucket( &mHash[ sIndex ] ) != IDE_SUCCESS );
    }

    IDE_ASSERT( iduMemMgr::free( mHash ) == IDE_SUCCESS );

    finalizeFreeTrans();

    IDE_TEST( mPool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcSharedTrans::finalizeBucket( mmcTransBucket * aBucket )
{
    IDE_ASSERT( aBucket->mBucketLatch.lockWrite( NULL, NULL )
                == IDE_SUCCESS );

    freeChain( &aBucket->mChain );

    IDU_LIST_INIT( &(aBucket->mChain) );

    IDE_ASSERT( aBucket->mBucketLatch.unlock()
                == IDE_SUCCESS );

    IDE_ASSERT( aBucket->mBucketLatch.destroy()
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

void mmcSharedTrans::finalizeFreeTrans()
{
    IDE_ASSERT( mFreeTransLatch.lockWrite( NULL, NULL )
                == IDE_SUCCESS );

    freeChain( &mFreeTransChain );

    IDU_LIST_INIT( &mFreeTransChain );

    IDE_ASSERT( mFreeTransLatch.unlock()
                == IDE_SUCCESS );

    IDE_ASSERT( mFreeTransLatch.destroy()
                == IDE_SUCCESS );
}

void mmcSharedTrans::freeChain( iduList * aNode )
{
    iduListNode   * sIterator;
    iduListNode   * sNext;
    mmcTransObj   * sTrans;
    
    IDU_LIST_ITERATE_SAFE( aNode, sIterator, sNext )   
    {
        sTrans = (mmcTransObj *)sIterator->mObj;

        freeTransToMemPool( sTrans );
    }   

    mFreeTransCount = 0;
}

UInt mmcSharedTrans::getBucketIndex( void * aKey )
{
    UInt sHashVal;
    
    sHashVal = iduHashUtil::hashBinary( aKey, ID_SIZEOF(sdiShardPin) );;

    return (sHashVal % mBucketCount);
}

idBool mmcSharedTrans::findTrans( mmcTransBucket *  sBucket,
                                  mmcSession     *  aSession,
                                  mmcTransObj    ** aTransOut )
{
    iduListNode       * sIterator        = NULL;
    mmcTransObj       * sTrans           = NULL;
    SChar             * sMyNodeName      = NULL;
    SChar             * sTransNodeName   = NULL;
    sdiShardPin         sShardPin        = SDI_SHARD_PIN_INVALID;

    *aTransOut  = NULL;
    sShardPin   = aSession->getShardPIN();

    IDU_LIST_ITERATE( &sBucket->mChain, sIterator )
    {
        sTrans = (mmcTransObj *)sIterator->mObj;

        if ( sTrans->mShareInfo->mShardPin == sShardPin )
        {
            sTransNodeName = mmcTrans::getShardNodeName( sTrans );
            sMyNodeName = aSession->getShardNodeName();

            if ( ( sTransNodeName[0] != '\0' ) && ( sMyNodeName[0] != '\0' ) )
            {
                if ( idlOS::strncmp( sMyNodeName,
                                     sTransNodeName,
                                     SDI_NODE_NAME_MAX_SIZE ) == 0 )
                {
                    /*do nothing: ok transaction share*/
                }
                else
                {
                    IDE_WARNING(IDE_SERVER_0, "Multiple shard nodes exist on a server.");
                    continue;
                }
            }

            /* Shared transaction is already exist. */
            *aTransOut = sTrans;
            break;
        }
    }

    return ( *aTransOut != NULL ) ? ID_TRUE : ID_FALSE;
}

IDE_RC mmcSharedTrans::allocTrans( mmcTransObj ** aTrans, mmcSession * aSession )
{
    UInt                sState           = 0;
    UInt                sBucketIndex;
    sdiShardPin         sShardPin        = SDI_SHARD_PIN_INVALID;
    mmcTransBucket    * sBucket          = NULL;
    mmcTransObj       * sTransOut        = NULL;
    idBool              sIsFound         = ID_FALSE;
    SChar             * sMyNodeName      = NULL;
    SChar             * sTransNodeName   = NULL;
    idBool              sIsFixed         = ID_FALSE;

    sShardPin    = aSession->getShardPIN();
    sBucketIndex = getBucketIndex( &sShardPin );
    sBucket      = &mHash[ sBucketIndex ];

    IDE_ASSERT( sBucket->mBucketLatch.lockWrite( NULL, NULL )
                == IDE_SUCCESS );
    sState = 1;

    sIsFound = findTrans( sBucket, aSession, &sTransOut );

    if ( sIsFound == ID_FALSE )
    {
        IDE_TEST( allocate( &sTransOut, aSession )
                  != IDE_SUCCESS );

        IDU_LIST_ADD_FIRST( &sBucket->mChain, &sTransOut->mShareInfo->mListNode );
        sState = 2;
    }

    mmcTrans::fixSharedTrans( sTransOut, aSession->getSessionID() );
    sIsFixed = ID_TRUE;

    if ( sTransOut->mShareInfo->mTxInfo.mState == MMC_TRANS_STATE_NONE )
    {
        IDE_TEST( sTransOut->mSmiTrans.initialize()
                  != IDE_SUCCESS );
        sTransOut->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_INIT_DONE;
    }

    /* Transaction 의 내용이 수정된다. 이제 execption 이 발생하면 안된다. */
    sTransNodeName = mmcTrans::getShardNodeName( sTransOut );
    sMyNodeName = aSession->getShardNodeName();

    if ( ( sTransNodeName[0] == '\0' ) && ( sMyNodeName[0] != '\0') )
    {
        mmcTrans::setTransShardNodeName( sTransOut, sMyNodeName );
    }

    if ( aSession->isShardUserSession() == ID_TRUE )
    {
        sTransOut->mShareInfo->mTxInfo.mUserSession = aSession;
    }

    ++sTransOut->mShareInfo->mTxInfo.mAllocRefCnt;

    sIsFixed = ID_FALSE;
    mmcTrans::unfixSharedTrans( sTransOut, aSession->getSessionID() );

    sState = 0;
    IDE_ASSERT( sBucket->mBucketLatch.unlock() == IDE_SUCCESS );

    *aTrans = sTransOut;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixed == ID_TRUE )
    {
        sIsFixed = ID_FALSE;
        mmcTrans::unfixSharedTrans( sTransOut, aSession->getSessionID() );
    }

    switch ( sState )
    {
        case 2:
            IDU_LIST_REMOVE( &sTransOut->mShareInfo->mListNode );
            free( sTransOut );
        case 1:
            IDE_ASSERT( sBucket->mBucketLatch.unlock() == IDE_SUCCESS );
            break;
        default:
            break;
    }

    *aTrans = NULL;

    return IDE_FAILURE;
}

IDE_RC mmcSharedTrans::allocate( mmcTransObj ** aTrans,
                                 mmcSession   * aSession )
{
    *aTrans = NULL;

    IDE_ASSERT( mFreeTransLatch.lockWrite( NULL, NULL )
                == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &mFreeTransChain ) == ID_TRUE )
    {
        IDE_ASSERT( mFreeTransLatch.unlock()
                    == IDE_SUCCESS );

        IDE_TEST( allocTransFromMemPool( aTrans )
                  != IDE_SUCCESS );
    }
    else
    {
        allocTransFromFreeList( aTrans );

        IDE_ASSERT( mFreeTransLatch.unlock()
                    == IDE_SUCCESS );
    }

    (*aTrans)->mShareInfo->mShardPin = aSession->getShardPIN();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcSharedTrans::allocTransFromMemPool( mmcTransObj ** aTrans )
{
    UInt          sState    = 0;
    mmcTransObj * sTransOut = NULL;

    IDE_TEST( mPool.alloc( (void **)&sTransOut )
              != IDE_SUCCESS);
    sState = 1;

    /* Mutex create */
    IDE_TEST( initTransInfo( sTransOut )
              != IDE_SUCCESS );
    sState = 2;

    *aTrans = sTransOut;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            finiTransInfo( sTransOut );
            /* fall through */
        case 1:
            (void) mPool.memfree( sTransOut );
            break;
        default:
            break;
    }

    *aTrans = NULL;

    return IDE_FAILURE;
}

void mmcSharedTrans::allocTransFromFreeList( mmcTransObj ** aTrans )
{
    mmcTransObj * sTransOut = NULL;
    iduListNode * sIterator = NULL;

    IDE_DASSERT( IDU_LIST_IS_EMPTY( &mFreeTransChain ) == ID_FALSE );

    sIterator = IDU_LIST_GET_FIRST( &mFreeTransChain );
    sTransOut = (mmcTransObj *)sIterator->mObj;

    IDU_LIST_REMOVE( sIterator );

    reuseTransInfo( sTransOut );

    mFreeTransCount--;

    *aTrans = sTransOut;
}

void mmcSharedTrans::free( mmcTransObj * aTrans )
{
    IDE_ASSERT( mFreeTransLatch.lockWrite( NULL, NULL )
                == IDE_SUCCESS );

    if ( mFreeTransCount >= mFreeTransMaxCount )
    {
        IDE_ASSERT( mFreeTransLatch.unlock() == IDE_SUCCESS );

        mmcSharedTrans::freeTransToMemPool( aTrans );
    }
    else
    {
        mmcSharedTrans::freeTransToFreeList( aTrans );

        IDE_ASSERT( mFreeTransLatch.unlock() == IDE_SUCCESS );
    }
}

void mmcSharedTrans::freeTransToMemPool( mmcTransObj * aTrans )
{
    finiTransInfo( aTrans );

    (void)mPool.memfree( aTrans );
}


void mmcSharedTrans::freeTransToFreeList( mmcTransObj * aTrans )
{
    IDU_LIST_ADD_FIRST( &mFreeTransChain, &aTrans->mShareInfo->mListNode );

    mFreeTransCount++;
}

IDE_RC mmcSharedTrans::freeTrans( mmcTransObj ** aTrans, mmcSession * aSession )
{
    mmcTransObj    * sTrans           = *aTrans;
    mmcTransBucket * sBucket          = NULL;
    UInt             sBucketIndex;
    idBool           sNeedBucketLatch = ID_FALSE;
    idBool           sIsBucketLatched = ID_FALSE;
    idBool           sIsFixed         = ID_FALSE;
    smiTrans       * sSmiTx           = NULL;
    idvSQL         * sStatistics      = NULL;

    sBucketIndex = getBucketIndex( &sTrans->mShareInfo->mShardPin );
    sBucket      = &mHash[ sBucketIndex ];

restart:

    if ( sNeedBucketLatch == ID_TRUE )
    {
        IDE_ASSERT( sBucket->mBucketLatch.lockWrite( NULL, NULL ) == IDE_SUCCESS );
        sIsBucketLatched = ID_TRUE;
    }

    mmcTrans::fixSharedTrans( sTrans, aSession->getSessionID() );
    sIsFixed = ID_TRUE;

    if ( ( sTrans->mShareInfo->mTxInfo.mAllocRefCnt == 1 ) && /* AllocRefCnt를 줄이기 전이니 1이다. */
         ( sIsBucketLatched == ID_FALSE ) )
    {
        /* 공유TX를 FREE해야하는경우 mBucketLatch를 잡아야한다. */

        IDE_DASSERT( sNeedBucketLatch == ID_FALSE );

        sNeedBucketLatch = ID_TRUE;

        sIsFixed = ID_FALSE;
        mmcTrans::unfixSharedTrans( sTrans, aSession->getSessionID() );

        IDE_CONT(restart);
    }

    sSmiTx = mmcTrans::getSmiTrans( sTrans );

    sStatistics = sSmiTx->getStatistics();
    if ( sStatistics != NULL )
    {
        if ( sStatistics->mSess->mSession == aSession )
        {
            sSmiTx->setStatistics( NULL );
        }
    }

    if ( sTrans->mShareInfo->mTxInfo.mUserSession == aSession )
    {
        sTrans->mShareInfo->mTxInfo.mUserSession = NULL;
    }

    /* AllocRefCnt 줄인다.*/
    --sTrans->mShareInfo->mTxInfo.mAllocRefCnt;

    if ( sTrans->mShareInfo->mTxInfo.mAllocRefCnt == 0 )
    {
        /* Shared Transaction FSM: Free */
        switch ( sTrans->mShareInfo->mTxInfo.mState )
        {
            case MMC_TRANS_STATE_PREPARE :
                /* smxTrans is dettach from smiTrans */
                /* Nothing to do */
                break;

            case MMC_TRANS_STATE_INIT_DONE :
            case MMC_TRANS_STATE_END :
                IDE_DASSERT(sSmiTx->isBegin() == ID_FALSE);

                MMC_SHARED_TRANS_TRACE( aSession,
                                        sTrans,
                                        "freeTrans: smiTrans destroy");

                /* 실패할 경우 mAllocRefCnt를 다시 올려야할까? 일단 올리지 않도록 처리함. */
                IDE_TEST(sSmiTx->destroy( aSession->getStatSQL() ) != IDE_SUCCESS);

                MMC_SHARED_TRANS_TRACE( aSession,
                                        sTrans,
                                        "freeTrans: smiTrans destroy success");
                break;

            default :
                IDE_DASSERT( 0 );
                break;
        }
        sTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_NONE;

        /* 공유TX를 반납하기 전에 fixSharedTrans를 풀어야 한다. */
        sIsFixed = ID_FALSE;
        mmcTrans::unfixSharedTrans( sTrans, aSession->getSessionID() );

        IDU_LIST_REMOVE( &sTrans->mShareInfo->mListNode );
        free( sTrans );

        *aTrans = NULL;

        sIsBucketLatched = ID_FALSE;
        IDE_ASSERT( sBucket->mBucketLatch.unlock() == IDE_SUCCESS );
    }

    if ( sIsFixed == ID_TRUE )
    {
        sIsFixed = ID_FALSE;
        mmcTrans::unfixSharedTrans( sTrans, aSession->getSessionID() );
    }
    
    if ( sIsBucketLatched == ID_TRUE )
    {
        sIsBucketLatched = ID_FALSE;
        IDE_ASSERT( sBucket->mBucketLatch.unlock() == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixed == ID_TRUE )
    {
        sIsFixed = ID_FALSE;
        mmcTrans::unfixSharedTrans( sTrans, aSession->getSessionID() );
    }
    
    if ( sIsBucketLatched == ID_TRUE )
    {
        sIsBucketLatched = ID_FALSE;
        IDE_ASSERT( sBucket->mBucketLatch.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC mmcSharedTrans::initTransInfo( mmcTransObj * aTransObj )
{
    mmcTransShareInfo   * sShareInfo;

    sShareInfo = (mmcTransShareInfo*)((SChar*)aTransObj + ID_SIZEOF(mmcTransObj));

    aTransObj->mShareInfo = sShareInfo;

    IDU_LIST_INIT_OBJ( &sShareInfo->mListNode, aTransObj );

    sShareInfo->mShardPin                  = SDI_SHARD_PIN_INVALID;

    sShareInfo->mTxInfo.mState             = MMC_TRANS_STATE_NONE;
    sShareInfo->mTxInfo.mIsBroken          = ID_FALSE;
    sShareInfo->mTxInfo.mAllocRefCnt       = 0;
    sShareInfo->mTxInfo.mPrepareSlot       = MMC_TRANS_NULL_SLOT_NO;
    sShareInfo->mTxInfo.mTransRefCnt       = 0;
    sShareInfo->mTxInfo.mDelegatedSessions = NULL;
    sShareInfo->mTxInfo.mUserSession       = NULL;
    sShareInfo->mTxInfo.mNodeName[0]       = '\0';
    SM_INIT_SCN( &sShareInfo->mTxInfo.mCommitSCN );

    IDE_TEST( sShareInfo->mConcurrency.mMutex.initialize( (SChar*)"Shared Transaction Mutex",
                                                          IDU_MUTEX_KIND_POSIX,
                                                          IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);

    IDE_TEST( sShareInfo->mConcurrency.mCondVar.initialize( (SChar*)"Shared Transaction Condition Var")
              != IDE_SUCCESS);

    sShareInfo->mConcurrency.mAllowRecursive    = ID_FALSE;
    sShareInfo->mConcurrency.mFixCount          = 0;
    sShareInfo->mConcurrency.mWaiterCount       = 0;
    sShareInfo->mConcurrency.mFixOwner          = (ULong)PDL_INVALID_HANDLE;
    sShareInfo->mConcurrency.mBlockCount        = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mmcSharedTrans::reuseTransInfo( mmcTransObj * aTransObj )
{
    mmcTransShareInfo   * sShareInfo;

    sShareInfo = aTransObj->mShareInfo;

    sShareInfo->mShardPin          = SDI_SHARD_PIN_INVALID;

    sShareInfo->mTxInfo.mState             = MMC_TRANS_STATE_NONE;
    sShareInfo->mTxInfo.mIsBroken          = ID_FALSE;
    sShareInfo->mTxInfo.mAllocRefCnt       = 0;
    sShareInfo->mTxInfo.mPrepareSlot       = MMC_TRANS_NULL_SLOT_NO;
    sShareInfo->mTxInfo.mTransRefCnt       = 0;
    sShareInfo->mTxInfo.mDelegatedSessions = NULL;
    sShareInfo->mTxInfo.mUserSession     = NULL;
    sShareInfo->mTxInfo.mNodeName[0]       = '\0';
    SM_INIT_SCN( &sShareInfo->mTxInfo.mCommitSCN );

    sShareInfo->mConcurrency.mAllowRecursive    = ID_FALSE;
    sShareInfo->mConcurrency.mFixCount          = 0;
    sShareInfo->mConcurrency.mWaiterCount       = 0;
    sShareInfo->mConcurrency.mFixOwner          = (ULong)PDL_INVALID_HANDLE;
    sShareInfo->mConcurrency.mBlockCount        = 0;
}

void mmcSharedTrans::finiTransInfo( mmcTransObj * aTransObj )
{
    mmcTransShareInfo   * sShareInfo;

    sShareInfo = aTransObj->mShareInfo;

    IDU_LIST_INIT_OBJ( &sShareInfo->mListNode, aTransObj );

    (void)sShareInfo->mConcurrency.mMutex.destroy();
    (void)sShareInfo->mConcurrency.mCondVar.destroy();

    aTransObj->mShareInfo = NULL;
}


