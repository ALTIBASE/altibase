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
 
#ifndef _O_SVM_EXPAND_CHUNK_H_
#define _O_SVM_EXPAND_CHUNK_H_ 1


/*
 * PROJ-1490 페이지리스트 다중화및 메모리반납
 * 
 * 데이터베이스를 확장하는 최소 단위인 ExpandChunk와 관련한 기능들을 지닌 클래스
 *
 * ExpandChunk의 구조는 다음과 같다.
 *
 * | Free List Info Page들 | Data Page 들 .... |
 *
 * Data Page에는 테이블이나 인덱스와 같이 데이터 저장을 위해
 * Page를 필요로 하는 객체들의 데이터가 저장된다.
 *
 * 하나의 ExpandChunk의 맨 앞에는 Free List Info Page가 존재한다.
 * Free List Info Page에는 Data Page와 1:1로 매핑되는 Slot이 존재하며,
 * 이 Slot에 Free Page의 Next Free Page정보가 기록된다.
 * Free List Info Page의 수는 ExpandChunk안에 속하는 Page의 수에 따라 달라진다.
 *
 * 예를들어, 하나의 Free List Info Page에 두 개의 데이터 페이지의
 * Next Free Page ID정보를 설정할 수 있는 상황을 가정해보자.
 * 하나의 ExpandChunk에 4개의 Data Page를 저장하도록 사용자가 Database 를 생성했다면,
 * 하나의 ExpandChunk는 Free List Info Page가 2개, 그리고 Data Page 가 4개가
 * 기록된다.
 * 
 * 이러한 ExpandChunk 가 두개 있다면, Data Page들의 Page ID는 다음과 같다.
 * 아래 예시에서 FLI-Page 는 Free List Info Page를 의미하고,
 * D-Page는 Data Page를 의미한다.
 * 
 * < ExpandChunk #0 >
 *   | FLI-Page#0 | FLI-Page#1 | D-Page#2 | D-Page#3 | D-Page#4  | D-Page#5  |
 * < ExpandChunk #1 >
 *   | FLI-Page#6 | FLI-Page#7 | D-Page#8 | D-Page#9 | D-Page#10 | D-Page#11 |
 *
 * 위의 예에서는 하나의 Free List Info Page 가
 * 두 Data Page의 Slot을 기록할 수 있으므로,
 * D-Page #2,3 의 Next Free Page ID 는 FLI-Page#0에,
 * D-Page #4,5 의 Next Free Page ID 는 FLI-Page#1에,
 * D-Page #8,9 의 Next Free Page ID 는 FLI-Page#6에,
 * D-Page #10,11 의 Next Free Page ID 는 FLI-Page#7에 기록된다.
 *
 * [ 참고 ]
 * 본 클래스에서 함수나 변수이름 뒤에 No가 붙은 것은,
 * 0부터 시작하여 1씩 증가하는 일련 번호로 간주하면 된다.
 */


// Free List Info(FLI) Page 안의 하나의 Slot을 표현한다.
// FLI Page안의 Slot은 곧 Next Free Page를 저장하는데 사용된다.
typedef struct svmFLISlot
{
    // 이 Slot과 1:1 매칭되는 Page가
    // Database Free Page인 경우   ==> 다음 Free Page의 ID
    // Table Allocated Page인 경우 ==> SVM_FLI_ALLOCATED_PID
    //
    // 서버 기동시 Restart Recovery가 필요하지 않은 경우에 한해서,
    // Free Page와 Allocated Page를 구분하기 위한 용도로 사용될 수 있다.
    scPageID mNextFreePageID;  
} svmFLISlot ;


// 하나의 Free Page의 Next Free Page ID가 저장될 Slot의 크기
#define SVM_FLI_SLOT_SIZE  ( ID_SIZEOF(svmFLISlot) )

// Expand Chunk
class svmExpandChunk
{
private:
    // 하나의 Free List Info Page에 기록할 수 있는 Slot의 수
    static UInt  mFLISlotCount;


    // 특정 Data Page가 속한 Expand Chunk를 알아낸다.
    static scPageID getExpandChunkPID( svmTBSNode * aTBSNode,
                                       scPageID     aDataPageID );


    // 하나의 Expand Chunk안에서 Free List Info Page를 제외한
    // 데이터 페이지들에 대해 순서대로 0부터 1씩 증가하는 번호를 매긴
    // Data Page No 를 리턴한다.
    static vULong getDataPageNoInChunk( svmTBSNode * aTBSNode,
                                        scPageID     aDataPageID );
    
    
public :
    svmExpandChunk();
    ~svmExpandChunk();
    
        
    // Expand Chunk관리자를 초기화한다.
    static IDE_RC initialize(svmTBSNode * aTBSNode);


    // 특정 Data Page의 Next Free Page ID를 기록할 Slot이 존재하는
    // Free List Info Page를 알아낸다.
    static scPageID getFLIPageID( svmTBSNode * aTBSNode,
                                  scPageID     aDataPageID );
    
    // FLI Page 내에서 Data Page와 1:1로 매핑되는 Slot의 offset을 계산한다.
    static UInt getSlotOffsetInFLIPage( svmTBSNode * aTBSNode,
                                        scPageID     aDataPageID );
    
    // ExpandChunk당 Page수를 설정한다.
    static IDE_RC setChunkPageCnt( svmTBSNode * aTBSNode,
                                   vULong       aChunkPageCnt );
    
    // Expand Chunk관리자를 파괴한다.
    static IDE_RC destroy(svmTBSNode * aTBSNode);
    
    // Next Free Page ID를 설정한다.
    // 여러 트랜잭션이 같은 Page의 Next Free Page ID를
    // 동시에 수정을 가하지 않도록 동시성 제어를 해야 한다.
    static IDE_RC setNextFreePage( svmTBSNode * aTBSNode,
                                   scPageID     aPageID,
                                   scPageID     aNextFreePageID );
    
    // Next Free Page ID를 가져온다.
    static IDE_RC getNextFreePage( svmTBSNode * aTBSNode,
                                   scPageID     aPageID,
                                   scPageID *   aNextFreePageID );

    // 최소한 특정 Page수 만큼 저장할 수 있는 Expand Chunk의 수를 계산한다.
    static vULong getExpandChunkCount( svmTBSNode * aTBSNode,
                                       vULong       aRequiredPageCnt );

    // Data Page인지의 여부를 판단한다.
    static idBool isDataPageID( svmTBSNode * aTBSNode,
                                scPageID     aPageID );

    // FLI Page인지의 여부를 판단한다.
    static idBool isFLIPageID( svmTBSNode * aTBSNode, 
                               scPageID     aPageID );


    // 하나의 Chunk당 Free List Info Page의 수를 리턴한다.
    static UInt getChunkFLIPageCnt(svmTBSNode * aTBSNode);

    // 데이터베이스 Page가 Free Page인지 여부를 리턴한다.
    static IDE_RC isFreePageID(svmTBSNode * aTBSNode,
                               scPageID     aPageID,
                               idBool *     aIsFreePage );

    /* BUG-32461 [sm-mem-resource] add getPageState functions to 
     * smmExpandChunk module */
    static IDE_RC getPageState( svmTBSNode     * aTBSNode,
                                scPageID         aPageID,
                                svmPageState   * aPageState );
};


/* 하나의 Chunk당 Free List Info Page의 수를 리턴한다.
 */
inline UInt svmExpandChunk::getChunkFLIPageCnt(svmTBSNode * aTBSNode)
{
    IDE_DASSERT( aTBSNode->mChunkPageCnt != 0 );
    IDE_DASSERT( aTBSNode->mChunkFLIPageCnt != 0 );
    
    return aTBSNode->mChunkFLIPageCnt;
}

#endif /* _O_SVM_EXPAND_CHUNK_H_ */

