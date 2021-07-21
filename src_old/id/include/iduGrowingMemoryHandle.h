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
 * $Id: iduGrowingMemoryHandle.h 15368 2006-03-23 01:14:43Z leekmo $
 **********************************************************************/

#ifndef _O_IDU_GROWING_MEMORY_HANDLE_H_
#define _O_IDU_GROWING_MEMORY_HANDLE_H_ 1

#include <idl.h>
#include <iduMemory.h>
#include <iduMemoryHandle.h>

/*
   Growing Memory Handle : 

   같은 영역의 메모리를 여러번 재사용 불가능한 경우
   사용하는 Memory Handle
   
   용도 :
      <prepare함수를 통해 전달받은 메모리를 재활용해서는 안되는 경우>
      
      1. SM에서 Redo시 압축해제 버퍼로 사용.

         Disk Log의 Redo시에 I/O를 최소화 하기 위해서 다음과 같은
         방식을 사용한다.
         
         하나의 Disk Log의 내용을 즉시 Buffer에 반영하지
         않고, Page ID를 기반으로 구축한 Hash Table에 반영할 로그의
         내용을 매달아두었다가 이러한 로그의 내용이 어느정도 쌓이면
         Page별로 Buffer에 로그를 Redo한다.

         그런데, 이때 Page별로 달아둔 로그가 복사된 것이 아니라,
         로그레코드 주소를 직접 포인팅하도록 설계되어 있다.
         (압축해제된 로그의 경우, 압축해제 버퍼를 직접 가리킨다.)

         만약 redo할 로그의 압축 해제를 위해 할당된 압축해제버퍼가
         재사용된다면, Disk Log를 Page별로 달아둔 로그의 포인터가
         엉뚱한 데이터를 가리키게 되어 이러한 제대로 동작할 수가 없다.

         이러한 경우, Growing Memory Handle을 이용하여 압축해제에 사용할
         로그의 메모리주소를 재활용하지 않고, 항상 새로 할당하도록 한다.
         
   구현 :
         iduMemory를 이용하여 Chunk단위로 메모리를 할당하도록 한다.
         
*/
class iduGrowingMemoryHandle : public iduMemoryHandle
{
public :

    // 객체 생성자,파괴자 => 아무일도 수행하지 않는다.
    iduGrowingMemoryHandle();
    virtual ~iduGrowingMemoryHandle();
    
    
    /*
       Growing Memory Handle을 초기화 한다.
     */
    IDE_RC initialize( iduMemoryClientIndex   aMemoryClient,
                       ULong                  aChunkSize );
    
    /*
       Growing Memory Handle을 파괴 한다.
     */
    IDE_RC destroy();
    
    /*
       Growing Memory Handle에 aSize이상의 메모리를 할당한다.

       항상 새로운 영역의 메모리를 할당한다.
     */
    virtual IDE_RC prepareMemory( UInt    aSize,
                                  void ** aPreparedMemory);

    // 이 Memory Handle을 통해 할당받은 메모리의 총량을 리턴
    virtual ULong getSize( void );

    // Growing Memory Handle이 할당한 메모리를 전부 해제
    IDE_RC clear();
    
private:
    /* Chunk단위로 OS의 메모리를 할당하여 작은 크기로 쪼개어
       메모리를 할당해 줄 메모리 관리자 */
    iduMemory  mAllocator;

    // prepareMemory를 호출하여 할당받아간 메모리 크기의 총합
    ULong mTotalPreparedSize;
    
};

    


#endif /*  _O_IDU_GROWING_MEMORY_HANDLE_H_ */
