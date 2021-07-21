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
 * $Id: smmSlotList.h 84167 2018-10-15 07:54:50Z justin.kwon $
 ***********************************************************************/ 

#ifndef _O_SMM_SLOT_LIST_H_
# define _O_SMM_SLOT_LIST_H_ 1

#include <smc.h>
#include <smu.h>

#include <smmDef.h>

class smmSlotList
{
 public:

    IDE_RC initialize( iduMemoryClientIndex aIndex,  
                       const SChar        * aName,
                       UInt                 aSlotSize,
                       UInt                 aMaximum = SMM_SLOT_LIST_MAXIMUM_DEFAULT,
                       UInt                 aCache   = SMM_SLOT_LIST_CACHE_DEFAULT );
   
    IDE_RC makeChild( UInt          aMaximum,
                      UInt          aCache,
                      smmSlotList * aChild );

    IDE_RC destroy( void );
    
    IDE_RC freeSlots( UInt       aNumber,
                      smmSlot  * aNodes );

    IDE_RC allocateSlots( UInt         aNumber,
                          smmSlot**    aSlots,
                          UInt         aFlag = SMM_SLOT_LIST_MUTEX_ACQUIRE );
    
    IDE_RC releaseSlots( UInt     aNumber,
                         smmSlot* aSlots,
                         UInt     aFlag = SMM_SLOT_LIST_MUTEX_ACQUIRE );
    
    IDE_RC release( );

    UInt testGetCount( void ) { return mNumber; }

    // BUG-18122 : MEM_BTREE_NODEPOOL performance view 추가
    UInt getAllocSlotCount( void ) { return mAllocSlotCount; }
    UInt getSlotPerPage( void )    { return mSlotPerPage;    }
    UInt getFreeSlotCount( void )  { return mNumber;         }
    UInt getSlotSize( void )       { return mSlotSize;       }
    ULong getTotalAllocReq( void ) { return mTotalAllocReq;  }
    ULong getTotalFreeReq( void )  { return mTotalFreeReq;   }

    idBool isParent( void ) { return ( mParent == NULL ) ? ID_TRUE : ID_FALSE; }

    inline IDE_RC  lock() { return mMutex.lock( NULL ); }
    inline IDE_RC  unlock() { return mMutex.unlock(); }
    
private:
    iduMutex               mMutex;
    smmSlotList*           mParent;
    UInt                   mMaximum;
    UInt                   mCache;
    UInt                   mNumber;
    smmSlot                mSlots;
    UInt                   mSlotSize;
    UInt                   mSlotPerPage;
    UInt                   mAllocSlotCount;
    iduMemPool             mMemPool;

    // BUG-18122 : MEM_BTREE_NODEPOOL performance view 추가
    ULong                  mTotalAllocReq;
    ULong                  mTotalFreeReq;
};

#endif /* _O_SMM_SLOT_LIST_H_ */
