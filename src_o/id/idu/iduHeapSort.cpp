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
#include <iduHeapSort.h>


/*----------------------------------------------------------------------
  task-2457 heap sort를 구현합니다.

  Description  : 항상 수행시간 O(NlnN)을 가지는 heap sort입니다. 기본적으로 c에서
  제공하는 qsort와 같은 인터페이스를 가집니다.
  속도는 qsort에 비해서 2배정도 느립니다. heap sort는 별도의
  메모리 공간을 요구하지 않는다는 점이 qsort대한 장점입니다.
                 
  aArray       -   [IN/OUT]  정렬을 시키기 원하는 배열,
  이 함수 수행후, 이 배열은 정렬되어 있다.
  aArrayNum    -   [IN]      배열의 원소의 총 개수
  aDataSize    -   [IN]      배열의 한 원소의 크기(byte단위)
  aCompar      -   [IN]      정렬을 수행하기 위해선 값을 비교 할 수 있어야 한다.
  
  두 원소를 받아서 비교를 수행하는 함수.
  오름차순 = 앞에원소가 클경우 return 1,같을 경우 return 0,
  뒤의 원소가 더 클경우 return -1
  내림차순 = 앞에원소가 클경우 return -1, 같을 경우 return 0,
  뒤의 원소가 더 클경우 return 1
  ----------------------------------------------------------------------*/
void iduHeapSort::sort( void  *aArray,
                        UInt aArrayNum,
                        UInt aDataSize,
                        SInt (*aCompar)(const void *, const void *))
{
    SChar *sFirstNode, *sLastNode;
    
    UInt sNodeIdx;

    IDE_ASSERT( aDataSize > 0);
    IDE_ASSERT( aArrayNum > 0);

    if (aArrayNum != 1)
    {
        /* 본 함수 내에서 item은 1부터 존재한다고 가정하므로, 포인터를 한칸
         * 뒤로 돌린다. 즉, c에서는 aArray[0]이 첫번째를 가리키지만 아래
         * 연산을 적용하면, aArray[1]이 첫번째를 가리키게 된다.
         * 1부터 시작하는 이유는 자식 노드를 구할때, 0부터 시작하면 자식노드가 0이
         * 나오기 때문이다.*/
        aArray = (void*)((SChar*)aArray - aDataSize);

        /*  자식을 가지는 최소 노드 부터 루트까지 올라가면서 각 노드에 대해
         *  MAX_HEAPIFY를 수행한다.
         *  
         *  마지막 부터 하는 이유 =>
         *  1. MAX_HEAPIFY가 수행하기 위한 조건에 left child 서브 트리와
         *     right child 서브트리가 힙트리여야 한다는 조건이 있다.
         *  2. MAX_HEAPIFY를 수행하고 나면 힙트리 조건을 만족한다.
         *  3. 어떤 sNodeIdx에 대해 MAX_HEAPIFY를 수행할때,
         *     그 left child와 right child는 이미 MAX_HEAPIFY를
         *     수행하고 난 후이므로 힙트리 일 것이므로 MAX_HEAPIFY를
         *     수행할 수 있다.
         *  4. sNodeIdx가 1일때가 가장 마지막으로, 이것은 전체 트리의 루트이다.
         *     그러므로 이것을 수행후, 전체 트리가 힙트리가 됨을 보장할 수 있다.
         */
        for (sNodeIdx = aArrayNum / 2; sNodeIdx != 0 ; --sNodeIdx)
        {
            IDU_HEAPSORT_MAX_HEAPIFY(sNodeIdx, aArray, aArrayNum, aDataSize, aCompar);
        }
    

        /*
         * 힙트리로 부터 정렬을 수행하는 부분, aArray[1], 즉 트리의 루트가
         * 가장 현재 트리(배열,aArray)내에서 가장 큰 값을 가지고 있다는 것을
         * 이용하여 정렬을 한다.
         */
        while (aArrayNum > 1)
        {
            /*트리의 루트, 현재 트리내에서 가장 큰 값을 가짐*/
            sFirstNode = IDU_HEAPSORT_GET_NTH_DATA(1, aArray, aDataSize);
        
            /*트리의 마지막 노드*/
            sLastNode  = IDU_HEAPSORT_GET_NTH_DATA(aArrayNum,aArray, aDataSize);

            /*현재 트리에서 가장 큰 값이 배열의 가장 마지막으로 간다.*/
            IDU_HEAPSORT_SWAP(sFirstNode, sLastNode, aDataSize);

            /*배열의 마지막으로 간 값은 더이상 트리내의 원소가 아니다.*/
            --aArrayNum;
        
            /*현재 트리의 루트, 즉 aArray[1]값은 더이상 가장 큰 값이 아니다.  하지만
             *이것의 left child와 rigth child는 모두 힙트리이므로 루트에 대해서만
             *MAX_HEAPIFY를 수행하면, 전체 트리는 힙트리가 된다.*/
            IDU_HEAPSORT_MAX_HEAPIFY(1, aArray, aArrayNum, aDataSize, aCompar);
        
            /*이러한 행동을 트리의 크기가 1이 될때까지 수행한다. 그러면 결국 가장
             * 작은 값부터 큰 순으로 정렬된 값을 얻을 수 있다.*/
        }
    }    
}


