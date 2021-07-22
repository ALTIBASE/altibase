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
 
/***********************************************************************
 * $Id: iduMemory.h 17293 2006-07-25 03:04:24Z mhjeong $
 **********************************************************************/

#ifndef _O_IDUVARMEMLIST_H_
# define _O_IDUVARMEMLIST_H_  1


/**
 * @file
 * @ingroup idu
*/

#include <idl.h>
#include <iduMemMgr.h>
#include <iduList.h>

typedef struct iduVarMemListStatus
{
    SChar *mCursor;
} iduVarMemListStatus;


/// @class iduVarMemList
/// 메모리를 한번 할당할때마다 할당된 메모리를 내부 관리 리스트에 추가한다.
class iduVarMemList {
    
public:
    /* iduVarMemList(); */
    /* ~iduVarMemList(); */

    static IDE_RC initializeStatic( void );
    static IDE_RC destroyStatic( void );
    
    /// 메모리 메니저의 초기화 작업을 수행한다.
    /// ST 함수 내에서는 이미 초기화된 메모리 매니저에 대한 포인터를 얻어서 사용하기 때문에 신경쓸 필요가 없다. QP에서는 현재 aIndex의 값으로 IDU_MEM_QMT를 사용하고 있다.
    /// @param aIndex 객체를 사용할 모듈 번호
    /// @param aMaxSize 메모리 최대 크기, 생략 가능
    IDE_RC               init( iduMemoryClientIndex aIndex, 
                               ULong                aMaxSize = ID_ULONG_MAX );
    /// 메모리 매니저의 삭제 작업을 수행한다.
    /// alloc이나 realloc 함수를 사용해서 이미 할당 되었지만 아직 해제되지 않은
    /// 메모리에 대한 해제 작업 및 내부적으로 사용되는 메모리 풀에 대한 삭제 작업을 수행한다.
    /// 개인 메모리 할당자를 사용하는 경우 메모리 할당자를 해지한다.
    /// ST 함수 내에서는 destroy 함수를 절대로 호출해서는 안된다.
    IDE_RC               destroy(); 

    /// @a aSize 에 지정된 크기 만큼의 메모리 공간을 할당한다.
    /// @param aSize 할당할 메모리 크기
    /// @param aBuffer 메모리 포인터
    IDE_RC               alloc( ULong aSize, void **aBuffer );

    /// 0으로 초기화된 메모리를 할당한다.
    IDE_RC               cralloc( ULong aSize, void **aBuffer );

    /// 할당된 메모리 공간을 해제한다.
    /// alloc 이나 realloc 함수를 통해서 할당되지 않은 포인터를 넘기는 경우 ASSERT 처리 한다.
    IDE_RC               free( void *aBuffer );
    /// 할당된 모든 메모리에 대한 해제 작업을 수행한다.
    /// 하지만 내부적으로 사용되는 메모리 풀에 대한 해제 작업은 수행되지 않는다.
    IDE_RC               freeAll();
    /// 지정된 범위의 블럭들을 해지한다.
    IDE_RC               freeRange( iduVarMemListStatus *aBegin,
                                    iduVarMemListStatus *aEnd );

    //fix PROJ-1596
    /// 현재 객체의 상태를 얻어온다.
    /// 내부적으로는 상태 정보를 얻는 시점을 표시하기위한
    /// 빈 블럭을 리스트에 추가하고, 이 블럭의 포인터를 넘겨준다.
    /// 따라서 getStatus로 얻은 블럭에 데이터를 기록하면 안된다.
    IDE_RC               getStatus( iduVarMemListStatus *aStatus );
    /// 객체를 특정 시점의 상태로 되돌린다.
    IDE_RC               setStatus( iduVarMemListStatus *aStatus );
    /// 사용하지 않는 블럭들만 해지한다.
    void                 freeUnused( void ) {};
    /// 모든 블럭을 해지한다.
    void                 clear() { (void)freeAll(); };

    // PROJ-1436
    /// 객체가 할당한 메모리의 총 합
    ULong                getAllocSize();
    /// 메모리 할당 횟수
    UInt                 getAllocCount();
    iduList *            getNodeList();
    /// 현재 객체가 가진 메모리 블럭들을 복사한다.
    /// @param aMem 블럭들의 리스트
    IDE_RC               clone( iduVarMemList *aMem );
    /// 메모리 블럭들의 내용을 비교한다.
    SInt                 compare( iduVarMemList *aMem );

    /// 각 메모리 블럭들의 크기와 블럭에 저장된 문자열을 터미널에 출력한다.
    void                 dump();

private:
    ULong                getSize( void *aBuffer );
    void                 setSize( void *aBuffer, ULong aSize );

    iduMemoryClientIndex mIndex;
    iduList              mNodeList;
    ULong                mSize;
    ULong                mMaxSize;
    UInt                 mCount;

    iduMemAllocator      mPrivateAlloc; // Thread-local 메모리 할당자
    static idBool        mUsePrivate;   // Thread-local 메모리 할당자 사용 여부
};

#endif /* _O_IDUVARMEMLIST_H_ */
