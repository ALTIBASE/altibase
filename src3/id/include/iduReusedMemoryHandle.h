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
 * $Id: iduReusedMemoryHandle.h 15368 2006-03-23 01:14:43Z leekmo $
 **********************************************************************/

#ifndef _O_IDU_REUSED_MEMORY_HANDLE_H_
#define _O_IDU_REUSED_MEMORY_HANDLE_H_ 1

#include <idl.h>
#include <iduMemoryHandle.h>

/*
   Reused Memory Handle : 

   같은 영역의 메모리를 여러번 재사용가능한 경우 사용하는 Memory Handle
   
   용도 :
      <prepare를 통해 전달받은 메모리를 일회용으로 사용하는 경우>
      
      1. Logging시에 압축을 수행할 메모리로 사용.
      
         로그를 로그파일에 기록하기 전에 압축을 하기위한 임시 버퍼로
         사용하는 경우, 압축된 로그를 로그파일에 기록한 후에는
         압축로그 버퍼를 다시 쓸 일이 없기 때문에
         Reused Memory Handle이 지니고 있는 메모리를 재활용한다.

      2. Transaction Rollback시 Undo할 로그를 압축해제하는데 사용.
      
         Transaction Rollback시 압축된 로그의 압축해제를 하는 경우,
         Undo시에 사용한 로그의 주소는 다음 Undo로그 수행중에는
         참조할 일이 없으므로, Reused Memory Handle을 이용하여
         Handle이 지니고 있는 메모리를 재활용한다.
*/
class iduReusedMemoryHandle : public iduMemoryHandle
{
public :
    iduReusedMemoryHandle();
    virtual ~iduReusedMemoryHandle();
    
    ///
    /// Reused Memory Handle을 초기화 한다.
    /// @param aMemoryClient 클래스를 사용할 모듈 인덱스
    IDE_RC initialize( iduMemoryClientIndex   aMemoryClient );
    
    ///
    /// Reused Memory Handle을 파괴 한다.
    ///
    IDE_RC destroy();
    
    ///
    /// Reused Memory Handle에 aSize이상의 메모리를 할당한다.
    /// 이미 aSize이상 메모리가 할당되어 있다면 아무일도 하지 않는다.
    /// 즉, 메모리는 커지기만하고 작아지지 않는다.
    /// @param aSize 메모리 할당 크기. 2의 제곱승으로 정렬됨 (1000->1024)
    /// @param aPrepareMemory 새롭게 할당된 메모리의 주소 반환
    virtual IDE_RC prepareMemory( UInt    aSize,
                                  void ** aPreparedMemory);

    ///
    /// 이 Memory Handle을 통해 OS로부터 할당받은 메모리의 총량을 리턴
    ///
    virtual ULong getSize( void );

    /* BUG-47365 이 Memory Handle의 Size를 조절한다. */
    IDE_RC tuneSize( UInt aSize );
    
private:
    /*
       Reused Memory Handle의 적합성 검토(ASSERT)
     */
    void assertConsistency();

    /*
       aSize를 2의 N승의 크기로 Align up한다.
     */
    static UInt alignUpPowerOfTwo( UInt aSize );

private:
    UInt                     mSize;        /// 할당된 크기
    void                 * mMemory;        /// 할당된 메모리
    iduMemoryClientIndex   mMemoryClient;  /// 메모리 할당 Client 인식번호
};

    


#endif /*  _O_IDU_REUSED_MEMORY_HANDLE_H_ */
