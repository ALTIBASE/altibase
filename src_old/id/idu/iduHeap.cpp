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
 
#include <idl.h>
#include <ide.h>
#include <iduHeap.h>
#include <iduHeapSort.h>

/*-------------------------------------------------------------------------
 * TASK-2457 heap sort를 구현합니다.
 * 
 * 아래에 사용한 매크로 함수는 iduHeapSort.h에 정의된 매크로 함수를 그대로 사용
 * 하고 있다. 함수에 대한 자세한 설명과 Heap에 대한 자세한 설명은
 * iduHeapSort.h에 기술되어 있다. 여기서는 간략한 설명만을 한다.
 * ----------------------------------------------------------------------*/
//heap array내에서 n번째 원소를 리턴한다.  원소는 1부터 시작한다.
#define IDU_HEAP_GET_NTH_DATA(idx) \
    IDU_HEAPSORT_GET_NTH_DATA( idx, mArray, mDataSize)

//두 원소를 서로 교환한다.
#define IDU_HEAP_SWAP(a,b)      IDU_HEAPSORT_SWAP(a,b,mDataSize)

//'aSubRoot의  각 child'를 루트로 하는 두 서브트리가 모두 heap트리의 특성을
//만족할때,이 매크로 수행하면 aSubRoot를 루트로 하는 트리가 heap트리의 특성을
//만족한다.
#define IDU_HEAP_MAX_HEAPIFY(aSubRoot) \
    IDU_HEAPSORT_MAX_HEAPIFY(aSubRoot, mArray, mDataCnt, mDataSize, mCompar)



/*------------------------------------------------------------------------------
  iduHeap를 초기화 한다.

  aIndex        - [IN]  Memory Mgr Index
  aDataMaxCnt   - [IN]  iduHeap가 가질 수 있는 Data의 최대 갯수
  aDataSize     - [IN]  Data의 크기, byte단위
  aCompar       - [IN]  Data들을 비교하기 위해 사용하는 Function, 2개의 data를
                        인자로 받아 값을 비교한다. 이 함수를 적절하게 사용하면
                        iduHeap를 최대우선순위큐 또는 최소우선순위큐로 사용할 수 있다.
                        
    가장 큰값이   root =  aCompar함수내에서 첫번째 인자가 더 클때 1을 리턴,
                       같을때 0을 리턴, 두번째 인자가 더 클때 -1을 리턴하면 된다.
    가장 작은값이 root  = aCompar함수내에서 첫번째 인자가 더 작을때 1을 리턴,
                       같을때 0을 리턴, 두번째 인자가 더 작을때 -1을 리턴하면 된다.
  -----------------------------------------------------------------------------*/
IDE_RC iduHeap::initialize( iduMemoryClientIndex aIndex,
                            UInt                 aDataMaxCnt,
                            UInt                 aDataSize,
                            SInt (*aCompar)(const void *, const void *))
{
    mIndex      = aIndex;
    mDataMaxCnt = aDataMaxCnt;
    mDataSize   = aDataSize;
    mComparUser = aCompar;
    mDataCnt    = 0;
    mArray      = NULL;


    IDE_TEST(iduMemMgr::malloc( mIndex,
                                mDataSize * mDataMaxCnt,
                                (void**)&mArray)
             != IDE_SUCCESS);
    
    //heap은 1부터 인덱스가 시작한다. 왜냐하면, root가 0이라면 child를 구할수 없기 때문이다.
    mArray = (void*)((SChar*)mArray - aDataSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduHeap::initialize( iduMemory          * aMemory,
                            UInt                 aDataMaxCnt,
                            UInt                 aDataSize,
                            SInt (*aCompar)(const void *, const void *))
{
    mIndex      = IDU_MAX_CLIENT_COUNT;
    mDataMaxCnt = aDataMaxCnt;
    mDataSize   = aDataSize;
    mComparUser = aCompar;
    mDataCnt    = 0;
    mArray      = NULL;


    IDE_TEST(aMemory->alloc( mDataSize * mDataMaxCnt,
                             (void**)&mArray)
             != IDE_SUCCESS);
    
    //heap은 1부터 인덱스가 시작한다. 왜냐하면, root가 0이라면 child를 구할수 없기 때문이다.
    mArray = (void*)((SChar*)mArray - aDataSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduHeap::destroy()
{
    if ( mIndex < IDU_MAX_CLIENT_COUNT )
    {
        IDE_TEST(iduMemMgr::free( (void*)((SChar*)mArray + mDataSize) )
                 != IDE_SUCCESS);
    }
    else
    {
        // iduMemory는 free할 필요가 없다.
    }
    
    mArray  = NULL;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*------------------------------------------------------------------------------
  iduHeap에 값을 저장한다.

  aData         - [IN]  현재 값을 복사해서 저장한다.
  aOverflow     - [OUT] 힙이 overflow되었다면 ID_TRUE, 그렇지 않다면 ID_FALSE
                        를 리턴한다.
  -----------------------------------------------------------------------------*/
void iduHeap::insert(void *aData, idBool *aOverflow)
{
    if( mDataCnt > mDataMaxCnt)
    {
        *aOverflow = ID_TRUE;
    }
    else
    {
        *aOverflow = ID_FALSE;
        maxHeapInsert(aData);
    }
}


/*------------------------------------------------------------------------------
  iduHeap의 가장 큰 원소를 리턴한다. root가 가장 큰 값을 가지고 있으므로 root를 리턴한다.

  aData         - [OUT] 가장 큰 원소의 값을 복사한후에 리턴한다.
  aUnderflow    - [OUT] 힙에 원소가 더이상 없을 경우 ID_TRUE를 리턴한다.
                        그렇지 않을 경우 ID_FALSE를 리턴한다.
  -----------------------------------------------------------------------------*/
void iduHeap::remove (void *aData, idBool *aUnderflow)
{
    if( mDataCnt == 0)
    {
        *aUnderflow = ID_TRUE;
    }
    else
    {
        *aUnderflow = ID_FALSE;

        //첫번째 원소가 가장 큰 값을 가지고 있으므로 이값을 리턴값에 복사한다.
        idlOS::memcpy( aData,
                       IDU_HEAP_GET_NTH_DATA(1),
                       mDataSize);
    
        //가장 마지막에 있는 값을 가장 앞으로 복사한다. 이렇게 되면 mArray의 루트만
        //힙의 특성을 만족하지 못하게 된다.
        if( mDataCnt != 1 )
        {
            idlOS::memcpy( IDU_HEAP_GET_NTH_DATA(1),
                           IDU_HEAP_GET_NTH_DATA(mDataCnt),
                           mDataSize);
        }
        
        //루트를 제외한 나머지 data는 모두 힙의 특성을 만족하므로, 루트에 대해서만
        //maxHeapify를 수행해주면 된다.
        IDU_HEAP_MAX_HEAPIFY(1);
    
        //mArray에서 하나의 원소를 제거했으므로 mDataCnt를 1 줄인다.
        mDataCnt--;
    }
}



/*------------------------------------------------------------------------------
  힙의 특성을 만족하는 배열 aArray에 대해, 새로운 원소를 추가한다. 
  이 함수를 호출하기 전에 지켜야 할 조건은
  1. aArray는 힙트리의 특성을 반드시 만족해야 한다.
 
  이 함수를 수행후,
  1. aArray는 힙트리의 특성을 만족한다.
  2. aKey가 새로운 원소로 aArray내에 들어가 있다.
 
                                   
  aKey         - [IN]      입력할 원소의 값
  ------------------------------------------------------------------------------*/
void iduHeap::maxHeapInsert(void *aKey)
{
    UInt    i;
    UInt    sParentIdx;
    SChar  *sNode       = NULL;
    SChar  *sParentNode = NULL;

    //mDataCnt는 절대로 mDataMaxCnt를 넘어서면 안된다.
    IDE_ASSERT( mDataCnt < mDataMaxCnt);
    
    mDataCnt++;
    
    idlOS::memcpy( IDU_HEAP_GET_NTH_DATA(mDataCnt),
                   aKey,
                   mDataSize);
    
    for( i = mDataCnt; i > 1; i = sParentIdx)
    {
        sNode       = IDU_HEAP_GET_NTH_DATA(i);
        sParentIdx  = GET_PARENT_IDX(i);
        sParentNode = IDU_HEAP_GET_NTH_DATA(sParentIdx);
        
        if( mCompar( sParentNode, sNode) >= 0)
        {
            break;
        }
        
        IDU_HEAP_SWAP(sParentNode, sNode);
    }
}
    



