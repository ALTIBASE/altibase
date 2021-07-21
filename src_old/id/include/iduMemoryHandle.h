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
 * $Id: iduMemoryHandle.h 15368 2006-03-23 01:14:43Z leekmo $
 **********************************************************************/

#ifndef _O_IDU_MEMORY_HANDLE_H_
#define _O_IDU_MEMORY_HANDLE_H_ 1

#include <idl.h>

/*
   iduMemoryHandle : 
     연속된 메모리 공간을 alloc,realloc하는 표준 interface를 제공한다.
     
   용도 :
     메모리의 할당과 해제를 담당하는 모듈과
     메모리를 새로운 크기로 재할당을 하는 모듈이 서로 다른 경우
     Memory Handle을 이용하여 메모리를 할당/재할당/해제한다.

   사용예:
     로그 압축해제의 경우 압축해제를 하는 Thread가 로그의 압축
     해제 버퍼를 할당하고 해제한다.
     (iduMemoryHandle을 구현하는 Concrete class의 initialize, destroy를 호출)

     반면, 압축을 실제로 푸는 작업은 로깅 모듈에서 수행하기 때문에,
     로깅 모듈에서는 압축 해제 버퍼의 크기를 마음대로 조절이
     가능해야 한다.
     (iduMemoryHandle을 구현하는 Concrete class의 prepare를 호출하여
      필요한 메모리 크기를 알려줌.)
 */
class iduMemoryHandle
{
public :
    inline iduMemoryHandle()
    {
    }
    
    inline virtual ~iduMemoryHandle()
    {
    }
    
    
    /*
       Memory Handle로부터 aSize이상의 메모리를 할당한다.

       그리고 할당된 메모리 주소를 반환한다
       
     */
    virtual IDE_RC prepareMemory( UInt    aSize,
                                  void ** aPreparedMemory) = 0;

    /*
      이 Memory Handle을 통해 할당받은 메모리의 크기를 리턴
    */
    virtual ULong getSize( void ) = 0;

};

    


#endif /*  _O_IDU_RESIZABLE_MEMORY_H_ */
