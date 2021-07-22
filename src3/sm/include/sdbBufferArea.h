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
 * $Id:$
 **********************************************************************/
/***********************************************************************
 * PROJ-1568 BUFFER MANAGER RENEWAL
 ***********************************************************************/

#ifndef _O_SDB_BUFFER_AREA_H_
#define _O_SDB_BUFFER_AREA_H_ 1

#include <sdbDef.h>
#include <sdbBCB.h>
#include <idu.h>
#include <idv.h>

/* applyFuncToEachBCBs 함수 내에서 각 BCB에 적용하는 함수의 형식 */
typedef IDE_RC (*sdbBufferAreaActFunc)( sdbBCB *aBCB, void *aFiltAgr);

class sdbBufferArea
{
public:
    IDE_RC initialize(UInt aChunkPageCount,
                      UInt aChunkCount,
                      UInt aPageSize);

    IDE_RC destroy();

    IDE_RC expandArea(idvSQL *aStatistics,
                      UInt    aChunkCount);

    //BUG-48042: Buffer Area Parallel 생성  
    static void expandAreaParallel( void * aJob );

    IDE_RC shrinkArea(idvSQL *aStatistics,
                      UInt    aChunkCount);

    void addBCBs(idvSQL *aStatistics,
                 UInt    aCount,
                 sdbBCB *aFirst,
                 sdbBCB *aLast);

    sdbBCB* removeLast(idvSQL *aStatistics);

    void getAllBCBs(idvSQL  *aStatistics,
                    sdbBCB **aFirst,
                    sdbBCB **aLast,
                    UInt    *aCount);

    UInt getTotalCount();

    UInt getBCBCount();

    UInt getChunkPageCount();

    void freeAllAllocatedMem();

    IDE_RC applyFuncToEachBCBs(idvSQL                *aStatistics,
                               sdbBufferAreaActFunc   aFunc,
                               void                  *aObj);

    void lockBufferAreaX(idvSQL   *aStatistics);
    void lockBufferAreaS(idvSQL   *aStatistics);
    void unlockBufferArea();

    /* BUG-32528 disk page header의 BCB Pointer 가 긁혔을 경우에 대한
     * 디버깅 정보 추가. */
    idBool isValidBCBPtrRange( sdbBCB * aBCBPtr )
    {
        if( ( aBCBPtr < mBCBPtrMin ) ||
            ( aBCBPtr > mBCBPtrMax ) )
        {
            ideLog::log( IDE_DUMP_0,
                         "Invalid BCB Pointer\n"
                         "BCB Ptr : %"ID_xPOINTER_FMT"\n"
                         "Min Ptr : %"ID_xPOINTER_FMT"\n"
                         "Max Ptr : %"ID_xPOINTER_FMT"\n",
                         aBCBPtr,
                         mBCBPtrMin,
                         mBCBPtrMax );

            return ID_FALSE;
        }
        return ID_TRUE;
    }
private:
    void initBCBPtrRange( )
    {
        mBCBPtrMin = (sdbBCB*)ID_ULONG_MAX;
        mBCBPtrMax = NULL;
    }

    void setBCBPtrRange( sdbBCB * aBCBPtr )
    {
        if( aBCBPtr > mBCBPtrMax )
        {
            mBCBPtrMax = aBCBPtr;
        }
        if( aBCBPtr < mBCBPtrMin )
        {
            mBCBPtrMin = aBCBPtr;
        }
    }

private:
    /* chunk당 page 개수 */
    UInt         mChunkPageCount;
    
    /* buffer area가 가진 chunk 개수 */
    UInt         mChunkCount;
    
    /* 하나의 frame 크기, 현재 8K */
    UInt         mPageSize;
    
    /* buffer area의 free BCB 개수 */
    UInt         mBCBCount;
   
    /* buffer area의 free BCB들의 리스트 */
    smuList      mUnUsedBCBListBase;
    
    /* buffer area의 모든 BCB들의 리스트 */
    smuList      mAllBCBList;
    
    /* BCB 할당을 위한 메모리 풀 */
    iduMemPool   mBCBMemPool;

    /* BUG-20796: BUFFER_AREA_SIZE에 지정된 크기보다 두배의 메모리를
     *            사용하고 있습니다.
     * iduMemPool의 구조적인 문제임. iduMemPool2를 사용하도록 함. */

    /* BCB의 Frame에 대한 Memory를 관리한다. */
    iduMemPool2  mFrameMemPool;
    
    /* BCB의 리스트를 위한 메모리 풀*/
    iduMemPool   mListMemPool;
    
    /* buffer area 동시성 제어를 위해 잡는 래치 */
    iduLatch  mBufferAreaLatch;

    /* BUG-32528 disk page header의 BCB Pointer 가 긁혔을 경우에 대한
     * 디버깅 정보 추가. */
    sdbBCB * mBCBPtrMin;
    sdbBCB * mBCBPtrMax;
};

inline UInt sdbBufferArea::getTotalCount()
{
    return mChunkPageCount * mChunkCount;
}

inline UInt sdbBufferArea::getBCBCount()
{
    return mBCBCount;
}

inline UInt sdbBufferArea::getChunkPageCount()
{
    return mChunkPageCount;
}

inline void sdbBufferArea::lockBufferAreaX(idvSQL   *aStatistics)
{
    IDE_ASSERT( mBufferAreaLatch.lockWrite( aStatistics,
                                            NULL )
                == IDE_SUCCESS );
}

inline void sdbBufferArea::lockBufferAreaS(idvSQL   *aStatistics)
{
    IDE_ASSERT( mBufferAreaLatch.lockRead( aStatistics,
                                           NULL )
                == IDE_SUCCESS );
}

inline void sdbBufferArea::unlockBufferArea()
{
    IDE_ASSERT( mBufferAreaLatch.unlock( ) == IDE_SUCCESS );
}

#endif // _O_SDB_BUFFER_AREA_H_

