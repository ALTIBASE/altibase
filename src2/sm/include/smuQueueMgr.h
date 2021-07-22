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
 * $Id: smuQueueMgr.h
 **********************************************************************/

#ifndef _O_SMU_QUEUEMGR_H_
#define _O_SMU_QUEUEMGR_H_ 1

#include <idl.h>
#include <idu.h>
#include <iduMutex.h>

typedef struct smuQueuePage
{
    struct smuQueuePage* m_pNext;
    SChar   m_Data[1];
}smuQueuePage;

class smuQueueMgr 
{
public:
    smuQueueMgr();
    ~smuQueueMgr();
    
    IDE_RC  initialize(iduMemoryClientIndex aIndex,
                       UInt a_nSize);
    IDE_RC  destroy();

    IDE_RC  lock() { return m_mutex.lock( NULL ); }
    IDE_RC  unlock()  { return m_mutex.unlock(); }
    
    idBool  isQueueEmpty();

    IDE_RC  dequeue(idBool  a_bLock,
                    void*   a_pItem,
                    idBool *a_pEmtpy);
    
    IDE_RC  enqueue(idBool  a_bMutex,
                    void*);
    UInt getQueueLength();
    
public:
    iduMemoryClientIndex mIndex;
    
    smuQueuePage* m_head;

    smuQueuePage* m_pCurQueue;

    UInt          m_nItemSize;
    UInt          mRear;
    UInt          mFront;
    UInt          mQueueLength;  // Queue length
    
    UInt          m_nPageSize;
    UInt          m_cItemInPage;

private:
    iduMutex      m_mutex;
}; 

#endif
