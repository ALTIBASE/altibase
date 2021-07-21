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
 

#ifndef _O_IDU_HEAP_SORT_H_
#define _O_IDU_HEAP_SORT_H_ 1

/*------------------------------------------------------------------------------
 * TASK-2457 heap sort를 구현합니다.
 * 
 * heap sort의 자세한 내용은 INTRODUCTION TO ALGORITHMS 2판의 6장을 보기 바란다.
 *
 *
 * 입력받은 배열을 힙트리로 보는 법
 * ---------------------------------
 * 1.정렬이 되어있지 않은 배열을 입력으로 받는다.
 * 
 * 2.배열을 완전2진 트리로 본다. (가장낮은 층은 제외, 가장 낮은 층은 왼쪽부터 차있다.)
 * 
 * 3.배열의 가장 처음 원소를 전체 힙트리의 root로 한다.
 * 
 * 4.left child는 '현재 원소의 인덱스(배열내의 순서)'*2 로 구한다.
 * 
 * 5.right child는 '현재 원소의 인덱스(배열내의 순서)'*2 +1로 구한다.
 * 
 * 6.즉,root가 인덱스 1일때(원래는 0이지만 구현에서 1로 바꾼다) root의 left child는
 *    2번째 인덱스를 가진 원소, rigth child는 3번째 인덱스를 가진 원소가 되고,
 *    각 노드는 또한 하위 트리의 root가 되어 자신의 left child와 right child를
 *    구하게 된다.
 *
 *
 * 힙의 특성
 * ----------------------------------
 * 1. 힙의 특성 = 자식 노드는 절대로 부모 노드보다 큰 값을 가질 수 없다.
 * 
 * 2. 힙의 특성을 만족한다면 배열의 1번째 원소는 모든 힙의 부모이기 때문에 가장
 *      큰 값을 가지게 된다.
 *
 *
 * 
 * 힙의 특성을 만족 시키는 법
 * ----------------------------------
 * 1. MAX_HEAPIFY(i)
 * 
 *   전제: i노드를 root로하는 힙트리에서 현재 left child(i)와 right child(i)모두
 *        힙의 특성을 만족한다. 즉, left child(i)를 루트로 하는 힙트리와
 *        right child(i)를 루트로 하는 힙트리 모두 힙의 특성을 만족한다.
 *        
 *     a. i노드가 left child(i)보다 크고, right child(i)보다 크다면 힙의 특성을
 *        만족하므로 이 함수는 아무런 일도 하지 않는다.
 *        
 *     b. i노드가 child보다 작다면, left와 right 중 더 큰 child node와 SWAP한다.
 *        즉, 위치를 변경한다. 예를 들면 left가 right보다 커서 left와 i(root)와
 *        변경을 했다. 그러면, 변경된 i는 자식들 보다 크므로 힙의 특성을 만족한다.
 *        하지만 변경된 left는 자신의 서브 트리에 대해 힙의 특성을 만족하지 않는다.
 *        그렇기 때문에 left에 대해서 MAX_HEAPIFY를 재귀적으로 수행한다.
 *        이 동작은 leaf에 도달할 때까지 수행한다.
 *        (개념적으로는 재귀적이지만 실제로는 그렇지 않다. 왜냐하면, MAX_HEAPIFY는
 *        매우 많이 호출되는 핵심부분으로 성능향상을 위해 매크로로 구현 하였다.
 *        매크로함수로는 recusive를 구현할 수 없기 때문에 루프로 구현을 하였다.)
 *          
 *     c. 전제를 만족시키기 위해 leaf에서 부터 각 노드에 대해 MAX_HEAPIFY를 적용한다.
 *        하지만, leaf node의 경우에는 자식이 전혀 없기 때문에 전제를 만족한다.
 *        그러므로 leaf 노드에 대해서는 MAX_HEAPIFY를 수행하지 않는다.
 *
 *        즉, 자식이 존재하는 노드만 MAX_HEAPIFY를 수행한다.
 *        자식이 존재하는 가장 마지막 노드 = 마지막 노드의 부모노드 = array_size / 2
 *
 *        즉, for( i = array_size/2; i != 0; i-- )
 *               MAX_HEAPIFY(i);
 *
 *     d. 루트 노드에 대해 MAX_HEAPIFY를 수행하고 나면 전체 array가 힙의 특성을
 *        만족하게 된다.
 *
 *
 *     
 * 소팅하는 법
 * ---------------------------------
 * 1. array[1]에는 항상 가장 큰 값이 들어있다. 이값을 array의 가장 마지막과
 *    교환(SWAP)한다.그러면, array[1]은 작은 값이 들어가고, array의 마지막은
 *    가장 큰값이 들어간다. 그런후, array의 크기를 줄인다. 즉 가장 큰값은
 *    이제 heap에서 제외된다.
 *    
 * 2. 힙의 루트에 대해서만 MAX_HEAPIFY를 수행한다. 왜냐하면, 위의 MAX_HEAPIFY가
 *    수행할 전제 조건을 만족시키기 때문에, 단지 루트에 대해서만 수행하면
 *    전체 array가 힙의 특성을 만족한다.
 *    
 * 3. 이것을 계속 수행한다. 즉,
 *    for( i = array_length; i > 1 ; i--)
 *        SWAP( array[1], array[array_length]);
 *        array_length -= 1;
 *        MAX_HEAPIFY(1);
 *        
 * 4. 루프가 종료된후, 가장 작은 것 부터 큰 순으로 정렬되어있다.
 *
 *------------------------------------------------------------------------------*/
#define GET_LEFT_CHILD_IDX(i)           ((i)<<1)
#define GET_PARENT_IDX(i)               ((i)>>1)

/*-----------------------------------------------------------------------------
  IDU_HEAPSORT_GET_NTH_DATA
 
 aArray에서 aNodeIdx번째 data를 리턴한다. 주의 할점은 배열과 다르게 1부터 시작한다.
 idx            - [IN]      구하고자 하는 데이터 index
 aArray         - [IN]      배열
 aDataSize      - [IN]      데이터 크기
 -----------------------------------------------------------------------------*/
#define IDU_HEAPSORT_GET_NTH_DATA(idx,aArray,aDataSize) \
    ((SChar*)aArray + (aDataSize * (idx)))


//매크로 swap이다. 이 매크로 사용후에 이 함수에 인자로
//넘겨준 a와 b의 값이 변하므로 적절히 처리해야 한다.
#define	IDU_HEAPSORT_SWAP(a, b,aDataSize) { \
    UInt  sCnt;                             \
    SChar sTmp;                             \
    sCnt = aDataSize;                       \
    do {                                    \
	sTmp = *a;                          \
	*a++ = *b;                          \
	*b++ = sTmp;                        \
    } while (--sCnt);                       \
}


/*------------------------------------------------------------------------------
  배열 aArray를 완전2진 트리로 볼때, aNodeIdx를 루트로 하는 서브 트리를 힙의
  특성을 만족하는 힙트리로 변경시킨다.
  이때, aNodeIdx를 루트로 하는 서브 트리에서 aNodeIdx의 left child를
  루트로 하는 서브 트리와 rigth child를 루트로 하는 서브트리는 반드시
  힙의 특성을 만족하는 힙트리여야 한다.
  
  aSubRoot  - [IN]      aArray내에서 서브루트의 인덱스값.
  
  aArray    - [IN/OUT]  힙트리, 이 함수를 수행하기전에 aArray가 힙의 특성을
                        만족한다면, 이 함수를 수행한 후에도 aArray는
                        힙의 특성을 만족한다.
                           
  aArrayNum - [IN]      현재 힙트리의 크기, 이것은 절대 aArray의 전체 크기가
                        아니다. 현재 aArray힙트리의 원소의 갯수를 뜻한다.
                        이 값은 maxHeapInsert 함수를 수행함으로 인해 1씩 증가한다.
                           
  aDataSize - [IN]      배열의 한 원소의 크기(byte단위)
  
  aCompar   - [IN]      힙을 만드는데 있어 void값들을 캐스팅해서 비교하는 함수가 필요하다.
                            
  부모가 자식보다 더 크기를 원할때 = 앞에원소가 클경우 return 1,
  같을 경우 return 0, 뒤의 원소가 더 클경우 return -1
  부모가 자식보다 더 작기를 원할때 =  앞에원소가 클경우 return -1,
  같을 경우 return 0, 뒤의 원소가 더 클경우 return 1
                                        
  ------------------------------------------------------------------------------*/
#define	IDU_HEAPSORT_MAX_HEAPIFY(aSubRoot, aArray, aArrayNum, aDataSize, aCompar) \
{                                                                               \
    UInt   i,j;                                                                 \
    SChar *sParent, *sChild;                                                    \
    for (i = aSubRoot; (j = GET_LEFT_CHILD_IDX(i)) <= aArrayNum; i = j)         \
    {                                                                           \
	sChild = (SChar *)aArray + j * aDataSize;/*get left child*/             \
                                                                                \
        /*left와 right child중에서 더 큰 child를 sChild값으로 세팅*/                 \
	if ((j < aArrayNum)  /*j(left)가 이것을 만족하면 right값은 존재한다.*/          \
          &&(aCompar(sChild/*left child*/, sChild + aDataSize/*Right Child*/ ))< 0)\
        {                                                                       \
            /*right가 존재하고 right가 더 큰경우 sChild를 right로 세팅*/              \
            sChild += aDataSize;                                                \
            ++j;                                                                \
	}                                                                       \
                                                                                \
	sParent = (SChar *)aArray + i * aDataSize;                              \
                                                                                \
	if (aCompar(sChild, sParent) <= 0)                                      \
        {                                                                       \
            /*부모 노드가 두 child보다 크다면 힙의 특성 만족하므로 아무런 행동도 하지않음*/ \
            break;                                                              \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            /*그렇지 않다면, 부모 노드와 child중 큰 노드와 위치 변경*/                  \
            IDU_HEAPSORT_SWAP(sParent, sChild, aDataSize);                      \
        }                                                                       \
    }                                                                           \
}


/*------------------------------------------------------------------------
  iduHeapSort class
  -----------------------------------------------------------------------*/
class iduHeapSort{
public:
    static void sort( void  *aArray,
                      UInt aArrayNum,
                      UInt aDataSize,
                      SInt (*aCompar)(const void *, const void *));
};


#endif   // _O_IDU_HEAP_H
