/***********************************************************************
 * Copyright 1999-2000, AltiBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemory.h 87767 2020-06-12 04:34:48Z seulki $
 **********************************************************************/

/***********************************************************************
 *
 * NAME
 *   iduMemory.h
 *
 * DESCRIPTION
 *   Dynamic memory allocator.
 *
 * PUBLIC FUNCTION(S)
 *   iduMemory( ULong BufferSize, ULong Align )
 *      BufferSize는 메모리 할당을 위한 중간 버퍼의 크기 할당받는
 *      메모리의 크기는 BufferSize를 초과할 수 없습니다.
 *      Align은 할당된 메모리의 포인터를 Align의 배수로 할것을
 *      지정하는 것입니다. 기본 값은 8Bytes 입니다.
 *
 *   void* alloc( size_t Size )
 *      Size만큼의 메모리를 할당해 줍니다.
 *
 *   void clear( )
 *      할당받은 모든 메모리를 해제 합니다.
 *
 * NOTES
 *
 * MODIFIED    (MM/DD/YYYY)
 *    assam     01/12/2000 - Created
 *
 **********************************************************************/

#ifndef _O_IDUMEMORY_H_
# define _O_IDUMEMORY_H_  1

/**
 * @file
 * @ingroup idu
*/

#include <idl.h>
#include <iduMemDefs.h>
#include <iduMemMgr.h>

/// iduMemory 에 속한 버퍼들의 관리
typedef struct iduMemoryHeader {
    iduMemoryHeader* mNext;      /// 다음 버퍼의 포인터
    char*            mBuffer;    /// 버퍼의 위치
    ULong            mChunkSize; /// 버퍼의 크기
    ULong            mCursor;    /// 버퍼 내부에서 빈 영역의 위치.
                                 /// 각 버퍼는 시작부분부터 사용되므로,
                                 /// 사용중인 영역의 끝 지점이 빈 영역의 시작 위치가 된다.
} iduMemoryHeader;

/// iduMemory 객체의 상태 정보 관리
typedef struct iduMemoryStatus {
    iduMemoryHeader* mSavedCurr;   /// 현재 사용중인 버퍼
    ULong            mSavedCursor; /// 버퍼에서 빈 영역의 위치
} iduMemoryStatus;


///
/// @class iduMemory
/// 주로 Query Processing에서 많이 요구되는 메모리 사용 패턴이
/// 데이터가 계속 늘어나다가 한순간에 데이터를 삭제하는 스택과 유사한 패턴이다.
/// 이 패턴에 따라 메모리를 할당받아서 데이터를 쓰고, 또 할당받아서 데이터를 쓰기를
/// 반복하다가 어느 순간 필요에 따라 특정한 시점으로 되돌아가서
/// 다시 데이터를 쓰거나 이전 데이터를 읽는 등의 일을 하게된다.
/// 또다른 중요한 특징은 다음순간에 얼마나 큰 데이터가 전달될지 모르므로
/// 할당받을 메모리 크기를 미리 알 수 없다는 것이다.
/// 따라서 iduMemory와 같이 일정한 크기의 메모리를 할당받아 사용하고
/// 다 쓴 이후에는 또 메모리를 할당받아 사용하기를 반복하는
/// 메모리 관리자가 필요해진다.
/// 객체가 가지는 메모리는 내부적으로는 여러개로 나누어져있지만
/// 마치 하나의 커다란 블럭인것처럼 사용하게된다.
/// 
/// @see struct iduMemoryHeader
class iduMemory {
private:
    iduMemoryHeader*        mHead;        // 최초로 생성된 버퍼
    iduMemoryHeader*        mCurHead;     // 현재 사용중인 버퍼
    ULong                   mDefaultChunkSize;   // 버퍼의 크기
    ULong                   mChunkCount;  // 버퍼의 갯수
    iduMemoryClientIndex    mIndex;   // 객체를 생성한 모듈
    iduMemAllocator         mPrivateAlloc; // Thread-local 메모리 할당자
    static idBool           mUsePrivate;   // Thread-local 메모리 할당자 사용 여부

    ULong                   mTotalChunkSize;

#if defined(ALTIBASE_MEMORY_CHECK)
    ULong                   mSize;
#endif
    
    IDE_RC header();
    IDE_RC extend( ULong aRequestedSize);
    IDE_RC release( iduMemoryHeader* Clue );
    IDE_RC checkMemoryMaximumLimit(ULong);
    IDE_RC malloc(size_t aSize, void** aPointer);
    IDE_RC free(void* aPointer);
public:

    /// 클래스 초기화
    static IDE_RC initializeStatic( void );

    static IDE_RC destroyStatic( void );

    /// 객체가 가진 모든 메모리를 해지하고 개인 할당자를 사용할 경우
    /// 개인 할당자까지 해지한다.
    void destroy(void);

    /// 객체의 상태를 초기화한다.
    /// @param aIndex 객체를 사용할 모듈의 인덱스
    /// @param BufferSize 내부 버퍼를 늘릴때의 크기
    IDE_RC init( iduMemoryClientIndex aIndex,
                 ULong BufferSize = 0);

    /// 0으로 초기화된 메모리를 할당받는다.
    /// @param aSize 필요한 메모리의 크기
    /// @param aMemPtr 할당받은 메모리의 포인터
    IDE_RC cralloc( size_t aSize, void** aMemPtr );

    /// 메모리를 할당받는다.
    /// 현재 객체내에 메모리가 부족하면 객체 내부에 새로운 버퍼를 추가해서 메모리를 반환한다.
    /// @param aSize 필요한 메모리의 크기
    /// @param aMemPtr 할당받은 메모리의 포인터
    IDE_RC alloc( size_t aSize, void** aMemPtr );

    /// 객체의 현재 상태를 얻어온다.
    /// @param Status 상태 정보
    SInt  getStatus( iduMemoryStatus* Status );
    /// 과거의 특정 시점으로 객체 상태를 되돌린다.
    /// A라는 시점에서부터 B라는 시점까지 메모리에 데이터를 기록한 후에
    /// A~B기간동안 사용한 데이터가 필요없을 때 A시점으로 돌아가기 위해
    /// 사용되는 함수이다. A시점의 상태 정보를 getStatus 함수를 이용해서 iduMemoryStatus에 보관한 후
    /// 함수 인자로 전달하면 A시점 이후에 사용된 메모리 블럭들의 데이터를 삭제하고
    /// 다시 사용할 수 있도록 초기화한다.
    SInt  setStatus( iduMemoryStatus* Status );

    /// 객체가 가진 메모리 버퍼들을 해지하지는 않지만
    /// 메모리 블럭의 데이터들을 삭제하고 메모리 블럭을 다시 사용할 수 있도록 한다.
    void  clear( void );

    /// 객체가 가진 메모리 버퍼들을 모두 해지한다. 개인 할당자를 사용할 경우
    /// 개인 할당자는 해지되지 않는다. 개인 할당자는 파괴자에서만 해지할 수 있다.
    void  freeAll( UInt a_min_chunk_no );

    /// 사용되지 않는 버퍼들만 해지한다.
    void  freeUnused( void );
    /// 객체가 가지고 있는 총 메모리의 크기를 반환한다.
    ULong getSize( void );

    /// 각 버퍼의 번호와 크기를 터미널에 출력한다.
    void dump();
};

#endif /* _O_IDUMEMORY_H_ */
