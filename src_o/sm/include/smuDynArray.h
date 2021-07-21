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
 * $Id: smuDynArray.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMU_DYN_ARRAY_H_
#define _O_SMU_DYN_ARRAY_H_ 1

#include <idl.h>
#include <iduMemPool.h>
#include <smDef.h>
#include <smuList.h>
#include <iduLatch.h>
/*
 *  smuDynArray의 목적
 *
 *  미리 크기를 결정할 수 없는 데이타(예를 들면 로깅)의 저장을 위해
 *  가변적인 메모리 공간을 관리하고, 이후에 데이터를 꺼내기 위한
 *  목적으로 설계되었음.
 *
 *  크게 저장 함수 store()와 꺼내기 함수 load()로 나뉘어지며,
 *  store()의 경우 호출자가 임의의 크기의 데이터를 저장할 수 있도록 한다.
 *
 *  load()의 경우 smuDynArray 객체에 저장된 데이터를 입력된 대상 메모리
 *  공간에 순차적으로 복사를 하도록 한다.
 *
 *  그외의 API는 부차적인 것으로 요구사항이 발생할 경우 구현한다.
 *
 *
 *  LayOut :
 *
 *  BaseNode     SubNode     SubNode
 *  +------+    +------+    +------+
 *  | Size |<-->|      |<-->|      |<-- 계속 증가함.
 *  | Off  |    |      |    |      |
 *  | mem  |    | mem  |    | mem  |
 *  | (2k) |    | (?k) |    | (?k) |
 *  +------+    +------+    +------+
 *
 *  구현 이슈 
 *  - BaseNode에는 현재 저장된 크기 및 다음에 저장될 노드 및 포인터를 유지한다.
 *    특히, 첫번째 노드에 대한 메모리 할당 비용을 줄이기 위해,
 *    BaseNode 내부에 정적크기의 버퍼를(SMU_DA_BASE_SIZE) 마련해두어,
 *    크기가 적은 경우 빠른 처리를 보장한다. 
 *
 *
 *  주의 사항
 *  - 응용 프로그램의 구동시  initializeStatic() 함수를 불러
 *    전역 메모리 객체를 초기화하고, 종료시에는 destroyStatic()을 불러
 *    객체를 소멸시켜야 한다. 
 *
 */


#define SMU_DA_BASE_SIZE      (2 * 1024)  // use 2k for base

typedef struct smuDynArrayNode
{
    smuList mList;       // List 구성 
    UInt    mStoredSize; // mChunkSize 만큼 커질 수 있음.
    UInt    mCapacity;   // 저장 최대 크기 
    ULong   mBuffer[1];  // Node의 실제 메모리 영역 시작 주소
                         //  smuDynArrayNode가 별도의 메모리 공간을 가리키는 
                         //  포인터를 유지하는 대신 Node 할당시에 
                         //  (ID_SIZEOF(smuDynArrayNode) + 메모리크기) 만큼 아예
                         //  할당할 목적으로 선언되었음.
                         //  ULong인 이유는 저장될 데이터 구조멤버중 ULong이
                         // 있을 경우 SIGBUS를 막기 위함임.
}smuDynArrayNode;

typedef struct smuDynArrayBase
{
    UInt            mTotalSize;  // 이 객체에 저장된 전체 메모리 크기
    smuDynArrayNode mNode;
    ULong           mBuffer[SMU_DA_BASE_SIZE / ID_SIZEOF(ULong) - 1];// for align8
}smuDynArrayBase;


class smuDynArray
{
    static iduMemPool mNodePool;
    static UInt       mChunkSize;

    static IDE_RC allocDynNode(smuDynArrayNode **aNode);
    static IDE_RC freeDynNode(smuDynArrayNode *aNode);

    // 지정된 메모리 노드 공간에 데이터를 복사한다.
    static IDE_RC copyData(smuDynArrayBase *aBase,
                           void *aDest, UInt *aStoredSize, UInt aDestCapacity,
                           void *aSrc,  UInt aSize);
    
public:
    static IDE_RC initializeStatic(UInt aNodeMemSize); // 전역 초기화 
    static IDE_RC destroyStatic();                     // 전역 소멸 

    static IDE_RC initialize(smuDynArrayBase *aBase);  // 객체 지역 초기화 
    static IDE_RC destroy(smuDynArrayBase *aBase);     // 객체 지역 해제 

    // DynArray에 메모리 저장
    inline static IDE_RC store( smuDynArrayBase * aBase,
                                void            * aSrc,
                                UInt              aSize );
    // DynArray의 메모리를 Dest로 복사.
    static void load(smuDynArrayBase *aBase, void *aDest, UInt aDestBuffSize);
    // 현재 저장된 메모리 크기를 반환함.
    inline static UInt getSize( smuDynArrayBase * aBase );
    
};

/*
 *  Tail Node를 인자로 넘겨, copyData를 수행한다. 
 */
inline IDE_RC smuDynArray::store( smuDynArrayBase * aBase,
                                  void            * aSrc,
                                  UInt              aSize )
{
    smuList *sTailList;    // for interation
    smuDynArrayNode *sTailNode;
    
    sTailList = SMU_LIST_GET_PREV( &(aBase->mNode.mList) );
    
    sTailNode = (smuDynArrayNode *)sTailList->mData;
    
    IDE_TEST( copyData( aBase,
                        (void *)(sTailNode->mBuffer),
                        &(sTailNode->mStoredSize),
                        sTailNode->mCapacity,
                        aSrc,
                        aSize ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 크기 리턴 */
inline UInt smuDynArray::getSize( smuDynArrayBase * aBase )
{
    return aBase->mTotalSize;
}

#endif  // _O_SMU_DYN_ARRAY_H_
    
