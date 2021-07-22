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
 

#ifndef _O_IDU_HEAP_H_
#define _O_IDU_HEAP_H_ 1

#include <idl.h>
#include <iduMemMgr.h>
#include <iduMemory.h>
#include <iduHeapSort.h>

/*-----------------------------------------------------------------
 TASK-2457 heap sort를 구현합니다.

 heap에 insert와 remove, 그리고 aCompar사용법.

 heap은 insert된 값을 aDataSize만큼 memcpy해서 복사한다.
 이때, 데이터 형이 정해져 있지 않기 때문에 void*로 값을 입력 받는다.
 
 즉, int형을 insert할 경우에는
 -----------------------------------
 compar(const void *a, const void *b)
 {
 //내부에서 배열 원소의 종류를 알 수 없다. 그래서 a를 (int*)로 캐스팅 해서
 //연산을 해야 한다.
    return *((int*)a) - *((int*)b); 
 }
 
 heap.initialize( ..., aDataSize = ID_SIZEOF(int), ..); 크기를 지정
 int a = 10;
 heap.insert((void*) &a); 이런식으로 변수의 포인터를 건네 주어야 한다.
 heap.remove((void*) &a); a에 값을 세팅한다.


 
 char 형을 insert할 경우에는
 -----------------------------------
 compar(const void *a, const void *b)
 {
 //내부에서 배열 원소의 종류를 알 수 없다. 그래서 a를 (char*)로 캐스팅 해서
 //연산을 해야 한다.
    return *((char*)a) - *((char*)b); 
 }

 heap.initialize( ..., aDataSize = ID_SIZEOF(char), ..); 크기를 지정
 char a = 10;
 heap.insert((void*) &a); 이런식으로 변수의 포인터를 건네 주어야 한다.
 heap.remove((void*) &a); a에 값을 세팅한다.


 
 구조체를 insert할 경우에는
 -----------------------------------
 compar(const void *a, const void *b)
 {
 //내부에서 배열 원소의 종류를 알 수 없다. 그래서 a를 (구조체*)로 캐스팅 해서
 //연산을 해야 한다.
    return ((구조체*)a)->num - ((구조체*)b)->num; 
 }
 
 heap.initialize( ..., aDataSize = ID_SIZEOF(구조체), ..); 크기를 지정
 구조체 a.num = 10;
 heap.insert((void*) &a); 이런식으로 변수의 포인터를 건네 주어야 한다.
 heap.remove((void*) &a); a에 값을 세팅한다.


 

 만약 포인터를 insert하고자 할 경우에는
 -----------------------------------
 compar(const void *a, const void *b)
 {
 //내부에서 배열 원소의 종류를 알 수 없다. 그래서 a를 (구조체**)로 캐스팅 해서
 //연산을 해야 한다.
 //여기서 위에서 처럼 (구조체*)로 캐스팅을 하면 안된다. 왜냐하면 내부에서
 //배열원소의 종류가 (구조체*)이기 때문에, 이것을 다시 reference해야 실제 구조체
 //가 나오게 된다. 그렇기 때문에 (구조체**)으로 캐스팅한다.
    return (*(구조체**)a)->num - (*(구조체**)b)->num; 
 }
 
  heap.initialize( ..., aDataSize = ID_SIZEOF(구조체 *), ..);  크기를 지정
 구조체 a = malloc(ID_SIZEOF(구조체));
 a->num = 10;
 heap.insert((void*) &a); 이런식으로 변수의 포인터를 건네 주어야 한다.
 heap.remove((void*) &a); a에 값을 세팅한다.
 free(a); a에는 malloc할때의 그 주소값이 들어있기 때문에 a를 바로 free할 수 있다.

 -----------------------------------------------------------------*/
class iduHeap{
public:
    iduHeap(){};
    ~iduHeap(){};
    
    IDE_RC  initialize ( iduMemoryClientIndex aIndex,
                         UInt                 aDataMaxCnt,
                         UInt                 aDataSize,
                         SInt (*aCompar)(const void *, const void *));
    
    IDE_RC  initialize ( iduMemory          * aMemory,
                         UInt                 aDataMaxCnt,
                         UInt                 aDataSize,
                         SInt (*aCompar)(const void *, const void *));
    
    IDE_RC  destroy();

    //힙에 새로운 원소를 넣는다.
    void    insert (void *aData, idBool *aOverflow);
    
    //힙에서 가장 큰 원소를 제거한다.
    void    remove (void *aData, idBool *aUnderflow);

    //현재 힙의 원소를 모두 삭제한다.
    void    empty(){ mDataCnt = 0;};

    UInt    getDataMaxCnt() { return mDataMaxCnt;   };
    UInt    getDataCnt()    { return mDataCnt;      };

private:
    void    maxHeapInsert( void  *aKey);
    

    iduMemoryClientIndex    mIndex;
    void                   *mArray;
    UInt                    mDataMaxCnt;
    UInt                    mDataCnt;
    UInt                    mDataSize;
    
    //사용자가 지정한 비교 함수, 실제 사용은 mCompar내에서 사용된다.
    SInt                  (*mComparUser)  (const void *, const void *);
    
    //실제 힙내부에서 사용하는 방법이 사용자가 원하는 것과 반대이기
    //때문에 이것을 맞추어 주기 위해서 사용한다.
    SInt                   mCompar(const void *a,const void *b){ return (-1)*mComparUser(a,b);};
};

#endif   // _O_IDU_HEAP_H_

