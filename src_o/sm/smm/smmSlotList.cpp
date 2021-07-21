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
 

/***********************************************************************
 * $Id: smmSlotList.cpp 84167 2018-10-15 07:54:50Z justin.kwon $
 **********************************************************************/

#include <ide.h>
#include <smm.h>

#include <smmSlotList.h>

IDE_RC smmSlotList::initialize( iduMemoryClientIndex aIndex, /* mempool idx */
                                const SChar        * aName,  /* mempool name */
                                UInt                 aSlotSize,
                                UInt                 aMaximum,
                                UInt                 aCache )
{
    aSlotSize    = aSlotSize > ID_SIZEOF(smmSlot) ? aSlotSize : ID_SIZEOF(smmSlot) ;
    mSlotSize    = idlOS::align(aSlotSize);
    mSlotPerPage = SMM_TEMP_PAGE_BODY_SIZE / mSlotSize;

    IDE_TEST( mMutex.initialize( (SChar*) "SMM_SLOT_LIST_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );

    mAllocSlotCount = 0;

    mMaximum = aMaximum > SMM_SLOT_LIST_MAXIMUM_MINIMUM ?
      aMaximum : SMM_SLOT_LIST_MAXIMUM_MINIMUM          ;
    mCache   = aCache   > SMM_SLOT_LIST_CACHE_MINIMUM   ?
      aCache   : SMM_SLOT_LIST_CACHE_MINIMUM            ;
    mCache   = mCache   < mMaximum                      ?
      mCache   : mMaximum - SMM_SLOT_LIST_CACHE_MINIMUM ;

    IDE_TEST(mMemPool.initialize( aIndex,
                                  (SChar *)aName,
                                  1,
                                  mSlotSize,
                                  smuProperty::getTempPageChunkCount(),
                                  IDU_AUTOFREE_CHUNK_LIMIT,            /* ChunkLimit */
                                  ID_TRUE,                             /* UseMutex */
                                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,     /* AlignByte */
                                  ID_FALSE,                            /* ForcePooling */
                                  ID_TRUE,                             /* GarbageCollection */
                                  ID_TRUE,                             /* HWCacheLine */
                                  IDU_MEMPOOL_TYPE_LEGACY )
            != IDE_SUCCESS);
    
    mParent      = NULL;
    mNumber      = 0;
    mSlots.prev  = &mSlots;
    mSlots.next  = &mSlots;

    // BUG-18122 : MEM_BTREE_NODEPOOL performance view 추가
    mTotalAllocReq = (ULong)0;
    mTotalFreeReq  = (ULong)0;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC smmSlotList::makeChild( UInt          aMaximum,
                               UInt          aCache,
                               smmSlotList * aChild )
{
    aChild->mSlotSize = mSlotSize;
    
    aChild->mMaximum = aMaximum > SMM_SLOT_LIST_MAXIMUM_MINIMUM ?
                       aMaximum : SMM_SLOT_LIST_MAXIMUM_MINIMUM;
    aChild->mCache   = aCache   > SMM_SLOT_LIST_CACHE_MINIMUM ?
                       aCache   : SMM_SLOT_LIST_CACHE_MINIMUM;
    aChild->mCache   = aChild->mCache < aChild->mMaximum ?
                       aChild->mCache : ( aChild->mMaximum - SMM_SLOT_LIST_CACHE_MINIMUM ) ;

    IDE_TEST( aChild->mMutex.initialize( (SChar*) "SMM_SLOT_LIST_MUTEX",
                                         IDU_MUTEX_KIND_NATIVE,
                                         IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );

    aChild->mParent        = this;
    aChild->mNumber        = 0;
    aChild->mSlots.prev    = &(aChild->mSlots);
    aChild->mSlots.next    = &(aChild->mSlots);

    aChild->mTotalAllocReq = (ULong)0;
    aChild->mTotalFreeReq  = (ULong)0;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC smmSlotList::destroy( void )
{
    if ( isParent() == ID_TRUE )
    {
        IDE_TEST( mMemPool.destroy() != IDE_SUCCESS );
    }

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* node list를 mempool free한다. */
IDE_RC smmSlotList::freeSlots( UInt       aNumber,
                               smmSlot  * aNodes )
{
    UInt      i;
    smmSlot * sCurr;
    smmSlot * sNext;

    sCurr = aNodes;

    IDE_ASSERT( aNumber != 0 );

    for ( i = 0 ; i < aNumber; i++ )
    {
        sNext = sCurr->next;

        IDE_TEST( mMemPool.memfree( sCurr )
                  != IDE_SUCCESS );

        sCurr = sNext;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smmSlotList::allocateSlots( UInt         aNumber,
                                   smmSlot**    aSlots,
                                   UInt         aFlag )
{
    UInt         sNumber;
    UInt         i;
    smmSlot*     sSlots;
    smmSlot*     sSlot;
    smmSlot*     sPrev;
    idBool       sLocked = ID_FALSE;

    if( ( aFlag & SMM_SLOT_LIST_MUTEX_MASK ) == SMM_SLOT_LIST_MUTEX_ACQUIRE )
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sLocked = ID_TRUE;
    }
    
    if( aNumber > mNumber )
    {
        sNumber = aNumber + mCache - mNumber;

        if( isParent() == ID_FALSE )
        {
            IDE_TEST( mParent->allocateSlots( sNumber, &sSlots )
                      != IDE_SUCCESS );

            sSlot             = sSlots->prev;
            sSlots->prev      = mSlots.prev;
            sSlot->next       = &mSlots;
            mSlots.prev->next = sSlots;
            mSlots.prev       = sSlot;
            mNumber += sNumber;
        }
        else
        {
            sPrev = mSlots.prev;

            for ( i = 0; i < sNumber; i++ )
            { 
                IDE_TEST( mMemPool.alloc( (void **)&sSlot ) != IDE_SUCCESS );

                sPrev->next = sSlot;
                sSlot->prev = sPrev;
                sPrev       = sSlot;

                mAllocSlotCount++;
            }

            sPrev->next = &mSlots;
            mSlots.prev = sPrev;
            mNumber += sNumber;
        }
    }

    // BUG-18122 : MEM_BTREE_NODEPOOL performance view 추가
    mTotalAllocReq += (ULong)aNumber;

    mNumber -= aNumber;
    for( *aSlots = sSlots = sSlot = mSlots.next,
         aNumber--;
         aNumber > 0;
         aNumber-- )
    {
        sSlot = sSlot->next;
    }
    mSlots.next       = sSlot->next;
    mSlots.next->prev = &mSlots;
    sSlot->next       = sSlots;
    sSlots->prev      = sSlot;
    
    if( sLocked == ID_TRUE )
    {
        sLocked = ID_FALSE;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();
    
    return IDE_FAILURE;

}

IDE_RC smmSlotList::releaseSlots( UInt     aNumber,
                                  smmSlot* aSlots,
                                  UInt     aFlag )
{

    smmSlot* sSlot;
    UInt     sNumber;
    idBool   sLocked = ID_FALSE;

    if( ( aFlag & SMM_SLOT_LIST_MUTEX_MASK ) == SMM_SLOT_LIST_MUTEX_ACQUIRE )
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sLocked = ID_TRUE;
    }

    sSlot             = aSlots->prev;
    sSlot->next       = &mSlots;
    aSlots->prev      = mSlots.prev;
    mSlots.prev->next = aSlots;
    mSlots.prev       = sSlot;

    mNumber += aNumber;

    if ( mNumber > mMaximum )
    {
        if ( isParent() == ID_FALSE )
        {
            sNumber = mNumber + mCache - mMaximum;
            for( aNumber = sNumber - 1, aSlots = sSlot = mSlots.next;
                 aNumber > 0;
                 aNumber--, aSlots = aSlots->next ) ;
            mSlots.next       = aSlots->next;
            mSlots.next->prev = &mSlots;
            aSlots->next      = sSlot;
            sSlot->prev       = aSlots;
            IDE_TEST_RAISE( mParent->releaseSlots( sNumber, aSlots )
                            != IDE_SUCCESS, ERR_RELEASE );
            mNumber -= sNumber;
        }
        else
        {
            /* PARENT */

            sNumber = mNumber + mCache - mMaximum;

            IDE_ASSERT( sNumber <= mNumber );

            for( aNumber = sNumber - 1, aSlots = sSlot = mSlots.next;
                 aNumber > 0;
                 aNumber--, aSlots = aSlots->next ) ;
            mSlots.next       = aSlots->next;
            mSlots.next->prev = &mSlots;
            aSlots->next      = sSlot;
            sSlot->prev       = aSlots;

            IDE_TEST( freeSlots( sNumber, aSlots )
                      != IDE_SUCCESS );

            mNumber -= sNumber;
        }
    }

    // BUG-18122 : MEM_BTREE_NODEPOOL performance view 추가
    mTotalFreeReq += (ULong)aNumber;
    
    if( sLocked == ID_TRUE )
    {
        sLocked = ID_FALSE;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RELEASE );
    aSlots->next       = mSlots.next;
    aSlots->next->prev = aSlots;
    mSlots.next        = sSlot;
    sSlot->prev        = &mSlots;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();
    
    return IDE_FAILURE;

}

IDE_RC smmSlotList::release( )
{

    smmSlot * sSlots  = NULL;
    idBool    sLocked = ID_FALSE;

    IDE_TEST( lock() != IDE_SUCCESS );

    sLocked = ID_TRUE;

    if ( mNumber > 0 )
    {
        if ( isParent() == ID_FALSE )
        {
            /* CHILD */
            /* 내 list를 모두 PARENT에게 반납 */
            mSlots.prev->next = mSlots.next;
            mSlots.next->prev = mSlots.prev;
            sSlots            = mSlots.next;

            mSlots.next = &mSlots;
            mSlots.prev = &mSlots;

            IDE_TEST( mParent->releaseSlots( mNumber, sSlots )
                      != IDE_SUCCESS );
            mNumber = 0;
        }
        else
        {
            /* PARENT */

            IDE_TEST( freeSlots( mNumber, mSlots.next )
                      != IDE_SUCCESS );

            mSlots.next = &mSlots;
            mSlots.prev = &mSlots;

            mNumber = 0;
        }
    }

    sLocked = ID_FALSE;

    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT(unlock() == IDE_SUCCESS );
    }

    IDE_POP();
    
    return IDE_FAILURE;

}

