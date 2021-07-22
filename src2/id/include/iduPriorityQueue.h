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
 
#ifndef _O_IDU_PRIORITY_QUEUE_H_
#define _O_IDU_PRIORITY_QUEUE_H_ 1

#include <idl.h>
#include <iduHeap.h>
#include <iduMemMgr.h>

/*-----------------------------------------------------------------
 TASK-2457 heap sort를 구현합니다.

 -----------------------------------------------------------------*/

class iduPriorityQueue
{
public:
    iduPriorityQueue(){};
    ~iduPriorityQueue(){};

    IDE_RC  initialize ( iduMemoryClientIndex aIndex,
                         UInt                 aDataMaxCnt,
                         UInt                 aDataSize,
                         SInt (*aCompar)(const void *, const void *));
    
    IDE_RC  initialize ( iduMemory          * aMemory,
                         UInt                 aDataMaxCnt,
                         UInt                 aDataSize,
                         SInt (*aCompar)(const void *, const void *));
    
    IDE_RC  destroy(){ return mHeap.destroy();};


    //큐에 새로운 원소를 넣는다.
    void    enqueue (void *aData,idBool *aOverflow) {mHeap.insert(aData,aOverflow);};
    //현재 큐에서 가장 큰 원소를 제거한다.
    void    dequeue (void *aData,idBool *aUnderflow){mHeap.remove(aData,aUnderflow);};

    //현재 큐의 원소를 모두 삭제한다.
    void    empty(){ mHeap.empty();};

    UInt    getDataMaxCnt() { return mHeap.getDataMaxCnt();};
    UInt    getDataCnt()    { return mHeap.getDataCnt();};

private:
    
    iduHeap     mHeap;
};

#endif   // _O_IDU_PRIORITY_QUEUE_H_
