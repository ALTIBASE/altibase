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
 

#include <idu.h>
#include <iduPriorityQueue.h>

/*------------------------------------------------------------------------------
  iduPriorityQueue를 초기화 한다.

  aIndex        - [IN]  Memory Mgr Index
  aDataMaxCnt   - [IN]  iduHeap가 가질 수 있는 Data의 최대 갯수
  aDataSize     - [IN]  Data의 크기, byte단위
  aCompar       - [IN]  Data들을 비교하기 위해 사용하는 Function, 2개의 data를 인자로 받아
                        값을 비교한다. 이 함수를 적절하게 사용하면 iduHeap를
                        최대우선순위 큐 또는 최소우선순위 큐로 사용할 수 있다.
         최대 우선순위큐 = 큰값을 먼저 리턴한다. 이것을 위해서,
                        aCompar함수내에서 첫번째 인자가 더 클때 1을 리턴, 같을때 0을 리턴,
                        두번째 인자가 더 클때 -1을 리턴하면 된다.
         최소 우선순위큐 = 작은값을 먼저 리턴한다. 이것을 위해서,
                        aCompar함수내에서 첫번째 인자가 더 작을때 1을 리턴, 같을때 0을 리턴,
                        두번째 인자가 더 작을때 -1을 리턴하면 된다.
  -----------------------------------------------------------------------------*/
IDE_RC iduPriorityQueue::initialize( iduMemoryClientIndex aIndex,
                                     UInt                 aDataMaxCnt,
                                     UInt                 aDataSize,
                                     SInt (*aCompar)(const void *, const void *))
{
    return mHeap.initialize(aIndex, aDataMaxCnt, aDataSize, aCompar);
}

IDE_RC iduPriorityQueue::initialize( iduMemory          * aMemory,
                                     UInt                 aDataMaxCnt,
                                     UInt                 aDataSize,
                                     SInt (*aCompar)(const void *, const void *))
{
    return mHeap.initialize(aMemory, aDataMaxCnt, aDataSize, aCompar);
}
