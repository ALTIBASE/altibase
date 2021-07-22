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
 
#include <iduFileIOVec.h>
#include <iduMemMgr.h>

#define IDU_MINLEN 8

/***********************************************************************
 * Description : creator
 * 멤버 변수를 초기화해준다.
 ***********************************************************************/
iduFileIOVec::iduFileIOVec() :
    mIOVec(NULL), mCount(0), mSize(0)
{
}

/***********************************************************************
 * Description : destructor
 *
 * DEBUG 모드에서는 destroy가 호출되지 않으면 ASSERT로 경고한다.
 * RELEASE 모드에서는 mIOVec이 해제되지 않았으면 해제해준다.
 ***********************************************************************/
iduFileIOVec::~iduFileIOVec()
{
    IDE_DASSERT(mIOVec == NULL);

    if(mIOVec != NULL)
    {
        iduMemMgr::free(mIOVec);
    }
}

/***********************************************************************
 * Description : initialize
 * 멤버 변수에 메모리를 할당하고
 * 데이터를 저장할 포인터와 크기를 설정한다.
 *
 * aCount - [IN] 뒤에 가변인수로 입력되는
 *               포인터와 크기의 개수를 설정한다.
 *               포인터와 크기는 짝으로 들어오니 총 인수 개수는
 *               aCount * 2 + 1개이다.
 * 함수 사용 예제 :
 *      iduFileIOVec sVec;
 *      sVec.initialize();
 *      sVec.initialize(0);
 *      sVec.initialize(2, sPtr1, sLen1, sPtr2, sLen2);
 ***********************************************************************/
IDE_RC iduFileIOVec::initialize(SInt aCount, ...)
{
    SInt    sSize;
    void*   sPtr;
    size_t  sLen;
    va_list sVL;
    SInt    i;

    sSize = (aCount == 0)? IDU_MINLEN : idlOS::align8(aCount);
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_ID_IDU,
                                sizeof(struct iovec) * sSize,
                                (void**)&mIOVec)
            != IDE_SUCCESS );
    
    va_start(sVL, aCount);

    for(i = 0; i < aCount; i++)
    {
        sPtr = va_arg(sVL, void*);
        sLen = va_arg(sVL, size_t);

        mIOVec[i].iov_base = sPtr;
        mIOVec[i].iov_len  = sLen;
    }

    va_end(sVL);

    mCount = aCount;
    mSize  = sSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : initialize
 * 멤버 변수에 메모리를 할당하고
 * 데이터를 저장할 포인터와 크기를 설정한다.
 *
 * aCount - [IN] aPtr과 aLen에 들어있는 포인터와 크기 개수를 지정한다.
 * aPtr   - [IN] 데이터를 저장할 포인터 배열
 * aLen   - [IN] 데이터 크기 배열
 ***********************************************************************/
IDE_RC iduFileIOVec::initialize(SInt aCount, void** aPtr, size_t* aLen)
{
    SInt    sSize;
    SInt    i;

    sSize = idlOS::align8(aCount);
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_ID_IDU,
                                sizeof(struct iovec) * sSize,
                                (void**)&mIOVec)
            != IDE_SUCCESS );
    
    for(i = 0; i < aCount; i++)
    {
        mIOVec[i].iov_base = aPtr[i];
        mIOVec[i].iov_len  = aLen[i];
    }

    mCount = aCount;
    mSize  = sSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : destroy
 *
 * 멤버 변수를 초기화하고 할당받은 메모리를 모두 해제한다.
 ***********************************************************************/
IDE_RC iduFileIOVec::destroy(void)
{
    if(mIOVec != NULL)
    {
        IDE_TEST( iduMemMgr::free(mIOVec) != IDE_SUCCESS );
        mIOVec = NULL;
        mCount = 0;
        mSize  = 0;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
    
/***********************************************************************
 * Description : add
 * 포인터와 길이를 추가한다.
 *
 * aPtr   - [IN] 데이터를 저장할 포인터
 * aLen   - [IN] 데이터 크기
 ***********************************************************************/
IDE_RC iduFileIOVec::add(void* aPtr, size_t aLen)
{
    if(mCount == mSize)
    {
        IDE_TEST( iduMemMgr::realloc(IDU_MEM_ID_IDU,
                                     sizeof(struct iovec) * mSize * 2,
                                     (void**)&mIOVec)
                != IDE_SUCCESS );

        mSize *= 2;
    }
    else
    {
        /* do nothing */
    }

    mIOVec[mCount].iov_base = aPtr;
    mIOVec[mCount].iov_len  = aLen;

    mCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : add
 * 포인터와 길이를 추가한다.
 *
 * aCount - [IN] aPtr과 aLen에 들어있는 포인터와 크기 개수를 지정한다.
 * aPtr   - [IN] 데이터를 저장할 포인터 배열
 * aLen   - [IN] 데이터 크기 배열
 ***********************************************************************/
IDE_RC iduFileIOVec::add(SInt aCount, void** aPtr, size_t* aLen)
{
    SInt sNewCount;
    SInt i;

    sNewCount = idlOS::align8(mCount + aCount);

    if(sNewCount > mSize)
    {
        IDE_TEST( iduMemMgr::realloc(IDU_MEM_ID_IDU,
                                     sizeof(struct iovec) * sNewCount,
                                     (void**)&mIOVec)
                != IDE_SUCCESS );

        mSize = sNewCount;
    }
    else
    {
        /* do nothing */
    }

    for(i = 0; i < aCount; i++)
    {
        mIOVec[mCount + i].iov_base = aPtr[i];
        mIOVec[mCount + i].iov_len  = aLen[i];
    }

    mCount += aCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : clear
 * 포인터와 길이를 모두 해제한다.
 * 메모리를 해제하지는 않는다.
 ***********************************************************************/
IDE_RC iduFileIOVec::clear(void)
{
    mCount = 0;
    return IDE_SUCCESS;
}
