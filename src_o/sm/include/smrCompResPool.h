/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: smrCompResPool.h 15368 2006-03-23 01:14:43Z kmkim $
 **********************************************************************/

#ifndef _O_SMR_COMP_RES_POOL_H_
#define _O_SMR_COMP_RES_POOL_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smrDef.h>
#include <smrCompResList.h>
#include <iduMemoryHandle.h>

/*
   Log Compression을 위한 Resource Pool

   본 클래스는 로그 압축을 위한 리소스를 관리한다.

   로그 압축을 위한 리소스는 다음과 같다
     - 압축된 로그가 기록될 메모리
     - 로그 압축에 임시적으로 사용할 작업 메모리

도입배경 :
   로그 압축은 Log의 끝단 Mutex를 잡지 않은 채로 이루어지기 때문에,
   같은 로그파일에 기록할 서로 다른 로그를
   동시에 여러 Thread가 함께 압축할 수 있다.
   
   손쉬운 구현 방법으로는
   Transaction이 Log 압축을 위한 Resource를 가지도록 하는 것을 생각해
   볼 수 있다.

   하지만, Active Trasaction이 몇개 없는 상황에서
   Active하지 않은 Transaction들이 Log 압축을 위한 Resource를
   가지게 되어 불필요한 메모리 낭비가 발생한다.

본 모듈의 역할 :   
   Log를 기록할때만 임시적으로 사용하는
   로그 압축을 위한 리소스(메모리)를
   여러 Transaction이 재활용할 수 있도록 Pool에 넣어 관리한다.
*/

class smrCompResPool
{
public :
    /* 객체 초기화 */
    IDE_RC initialize( SChar * aPoolName,
                       UInt    aInitialResourceCount,    
                       UInt    aMinimumResourceCount,
                       UInt    aGarbageCollectionSecond );
    
    /* 객체 파괴 */
    IDE_RC destroy();

    /* 로그 압축을 위한 Resource 를 얻는다 */
    IDE_RC allocCompRes( smrCompRes ** aCompRes );

    /* 로그 압축을 위한 Resource 를 반납 */
    IDE_RC freeCompRes( smrCompRes * aCompRes );

    /* BUG-47365 로그 압축을 위한 Resource의 크기를 재조절 */
    IDE_RC tuneCompRes( smrCompRes * aCompRes,
                        UInt         aSize );

private :

    /* 로그 압축을 위한 Resource 를 생성한다 */
    IDE_RC createCompRes( smrCompRes ** aCompRes );

    /* 로그 압축을 위한 Resource 를 파괴한다 */
    IDE_RC destroyCompRes( smrCompRes * aCompRes );


    /* 리소스 풀 내에서의 가장 오래된 리소스 하나에 대해
       Garbage Collection을 실시한다. */
    IDE_RC garbageCollectOldestRes();
    
    /* 이 Pool이 생성한 모든 압축 Resource 를 파괴한다 */
    IDE_RC destroyAllCompRes();

    /* 리소스가 몇 마이크로 초 이상 사용되지 않을 경우
       Garbage Collection할지? */
    UInt           mGarbageCollectionMicro; 

    /* 리소스 풀 안에 유지할 최소한의 리소스 갯수
       Garbage Collection할만큼 오랫동안 사용되지 않더라도
       이만큼의 리소스는 풀에 유지한다. 
     */
    UInt           mMinimumResourceCount;
    
    smrCompResList mCompResList;    /* 재사용 List */
    iduMemPool     mCompResMemPool; /* 압축 리소스를 위한 메모리 풀 */
};

#endif /* _O_SMR_COMP_RES_POOL_H_ */
