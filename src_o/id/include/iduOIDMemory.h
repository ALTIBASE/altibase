/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduOIDMemory.h 31545 2009-03-09 05:24:18Z a464 $
 **********************************************************************/

#ifndef _O_IDU_OID_MEMORY_H_
#define _O_IDU_OID_MEMORY_H_ 1

#include <idl.h>
#include <idu.h>
#include <iduMutex.h>

#define IDU_OID_MEMORY_AUTOFREE_LIMIT 5  // free page Count in free page list

struct iduOIDMemAllocPage
{
    vULong               freeIncCount;    // free call count
    vULong               allocIncCount;   // alloc call count
    iduOIDMemAllocPage * prev;            // previous page
    iduOIDMemAllocPage * next;            // next page

};

// BUG-22877
// alloc list는 이중 연결 list이고 free list는 단일 연결 list 이므로
// 혼동을 피하기 위해, free list용 single link list page header를
// 별도로 선언해서 캐스팅하여 사용한다.
struct iduOIDMemFreePage
{
    iduOIDMemFreePage * next;      // next page
};


struct iduOIDMemItem
{
    iduOIDMemAllocPage * myPage;  // item이 소속된 page
};

/*----------------------------------------------------------------
  Name : iduOIDMemory Class
  Arguments :
  Description :
      OID용 메모리 관리자인 iduMemMgr의 단점을 극복하기 위하여 구현.
      - iduMemMgr의 장점 수용
          : free시 latch를 획득하지 않음
      - iduMemMgr의 단점 극복
          : kernel로의 메모리 반납을 허용함
      - allocation page list와 free page list를 관리하며,
        메모리 할당은 allocation page list의 top page로부터
        순차적으로 할당하며(iduMemMgr와 다름),
        메모리 해제시 해당 Page가 모두 빌 경우 free page list에
        등록하거나 kernel에 반납하여 메모리 해제를 할 수 있도록 한다.
        iduMemMgr에서 free slot list중 top list는 alloc하지 않는 방법을
        allocation page list의 top과 free page list의 tail에 적용하여
        memfree()시에 latch를 사용하지 않도록 한다.
  ----------------------------------------------------------------*/

class iduOIDMemory
{
public:
    iduOIDMemory();
    ~iduOIDMemory();

    IDE_RC     initialize(iduMemoryClientIndex aIndex,
                          vULong               aElemSize,
                          vULong               aElemCount );
    IDE_RC     destroy();

    IDE_RC     alloc(void ** aMem );
    IDE_RC     memfree( void * aMem );

    void       dumpState( SChar * aBuffer, UInt aBufferSize );
    void       status();
    ULong      getMemorySize();

    IDE_RC     lockMtx() { return mMutex.lock(NULL /* idvSQL* */); }
    IDE_RC     unlockMtx() { return mMutex.unlock(); }

private:
    IDE_RC     grow();         // add to allocation page list

private:
    iduMemoryClientIndex  mIndex;

    iduMutex   mMutex;

    vULong     mElemSize;      // size of element
    vULong     mItemSize;      // size of item (= element + OIDMemItem)
    vULong     mItemCnt;       // item count in a page
    vULong     mPageSize;      // page size

    iduOIDMemAllocPage mAllocPageHeader;   // alloc page list
    ULong              mPageCntInAllocLst; // alloc page 수

    iduOIDMemFreePage  mFreePageHeader;    // free page list
    ULong              mPageCntInFreeLst;  // free page 수
};

#endif /* _O_IDU_OID_MEMORY_H_ */






