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

#ifndef _O_MMC_MUTEXPOOL_H_
#define _O_MMC_MUTEXPOOL_H_ 1

#include <idl.h>
#include <idu.h>

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
/* To eliminate the mutex contention between threads, we need mmcMutexPool.
   NOK ref. = http://nok.altibase.com/x/Fyp9
*/
class mmcMutexPool
{
private:
    /* MemPool for allocating iduMutex */
    iduMemPool      mMutexPool;
    /* MemPool for allocating iduListNode */
    iduMemPool      mListNodePool;
    /* A List that contains currently being used mutexes. */
    iduList         mUseList;
    /* A List that contains currently not used mutexes. */
    iduList         mFreeList;

    /* The number of elements in the mFreeList. */
    UInt            mFreeListCnt;

private:
    /* A function that allocates/initializes iduListNode and iduMutex. */
    IDE_RC        allocMutexPoolListNode( iduListNode **aListNode );
public:
    IDE_RC        initialize();
    IDE_RC        finalize();

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* A function that requests a mutex from mFreeList in the mutex pool. */
    IDE_RC        getMutexFromPool( iduMutex **aMutexObj, SChar *aMutexName );
    /* A function that frees a mutex from mUseList in the mutex pool. */
    IDE_RC        freeMutexFromPool( iduMutex *aMutexObj );
};

#endif
