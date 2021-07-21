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
 * $id$
 **********************************************************************/

#ifndef _O_DKD_DATA_BUFFER_MGR_H_
#define _O_DKD_DATA_BUFFER_MGR_H_ 1


#include <dkd.h>

/************************************************************************
 * DK DATA BUFFER MANAGER (추후보완을 위한 comment)
 *
 *  1. Concept
 *
 *   여기서 data buffer 는 DK 모듈을 통해 remote query 가 수행된 경우,
 *  AltiLinker  프로세스를 통해 원격 서버로부터 전송받은 원격객체의
 *  데이터를 QP 로 fetch 가 완료되기 전까지 저장하기 위해 일시적으로 
 *  제공되는 record buffer 들의 총합을 의미한다. 
 *   그러나 모든 메모리영역을 대상으로 buffer 를 할당하도록 할 수는 없고
 *  제한된 크기 ( DKU_DBLINK_DATA_BUFFER_SIZE ) 를 시스템 프로퍼티로 
 *  입력받아 해당 크기만큼만 사용할 수 있도록 한다.
 *
 *  2. Implementation
 *  
 *   프로퍼티로부터 입력받은 최대 크기 안에서 동적할당하는 방법으로 
 *  구현한다. iduMemPool 을 이용하여 block * count 만큼 할당하는 방법을 
 *  사용하는 경우, data buffer allocation mechanism 설계상 alloc list 와 
 *  free list 를 따로 생성하여 유지해야 하는 번거로움이 있으므로 일단은 
 *  record buffer 를 할당받을 때마다 data buffer 최대 크기를 비교하여 
 *  그 안에서만 할당받을 수 있도록 하는 방법으로 한다. 따라서 iduMemMgr
 *  를 이용하여 필요할 때마다 record buffer 의 할당 및 해제를 하는 식으로 
 *  구현하며 dkdDataBufferMgr 초기화 시 별도의 memory pool 을 할당받을 
 *  필요가 없고 그러므로 mutex 등을 통해 memory 의 동시접근을 막을 필요도 
 *  없다. 
 *   만약 record buffer 의 할당 및 해제가 성능상 overhead 가 된다면 추후 
 *  iduMemPool 을 이용하는 방식을 생각해보도록 한다. 
 *
 *   그러나 추후 개선의 여지를 생각하여 다음과 같은 틀안에서 구현한다.
 *
 *      data_buffer_size >= SUM( allocated_record_buffer_size )
 *      record_buffer_size = block_size * count
 *
 ***********************************************************************/
class dkdDataBufferMgr
{
private:
    static UInt     mBufferBlockSize;        /* 버퍼블록의 크기 */
    static UInt     mMaxBufferBlockCnt;      /* 최대 할당가능한 버퍼블록개수 */
    static UInt     mUsedBufferBlockCnt;     /* 사용한 버퍼블록개수 */
    static UInt     mRecordBufferAllocRatio; /* 레코드버퍼 할당비율 *//* BUG-36895: SDouble -> UInt */

    static iduMemAllocator * mAllocator;       /* TLSF memory allocator BUG-37215 */
    static iduMutex mDbmMutex;

public:
    /* Initialize / Finalize */
    static IDE_RC       initializeStatic(); /* allocate DK buffer blocks */
    static IDE_RC       finalizeStatic();   /* free DK buffer blocks */

    /* 입력받은 갯수만큼 buffer block 을 할당 및 반환 */
    static IDE_RC       allocRecordBuffer( UInt *aSize, void  **aRecBuf );
    static IDE_RC       deallocRecordBuffer( void *aRecBuf, UInt aBlockCnt );

    /* TLSF memory allocator BUG-37215 */
    static inline iduMemAllocator* getTlsfAllocator(); 

    /* Data buffer block */
    static inline UInt      getBufferBlockSize();
    static inline UInt      getAllocableBufferBlockCnt();
    static inline void      incUsedBufferBlockCount( UInt aUsed );
    static inline void      decUsedBufferBlockCount( UInt aUsed );
    static inline IDE_RC    lock();
    static inline IDE_RC    unlock();
};

/* BUG-37215 */
inline iduMemAllocator* dkdDataBufferMgr::getTlsfAllocator()
{
    return mAllocator;
}

inline UInt  dkdDataBufferMgr::getBufferBlockSize()
{
    return mBufferBlockSize;
}

/* BUG-36895 */
inline UInt  dkdDataBufferMgr::getAllocableBufferBlockCnt()
{
    return (UInt)( ( mMaxBufferBlockCnt - mUsedBufferBlockCnt ) 
                   * mRecordBufferAllocRatio ) / 100;
}

inline void  dkdDataBufferMgr::incUsedBufferBlockCount( UInt aUsed )
{
    mUsedBufferBlockCnt += aUsed;
}

inline void  dkdDataBufferMgr::decUsedBufferBlockCount( UInt aUsed )
{
    mUsedBufferBlockCnt -= aUsed;
}

inline IDE_RC dkdDataBufferMgr::lock()
{
    return mDbmMutex.lock( NULL /* idvSQL* */ );
}

inline IDE_RC dkdDataBufferMgr::unlock()
{
    return mDbmMutex.unlock();
}

#endif /* _O_DKD_DATA_BUFFER_MGR_H_ */

