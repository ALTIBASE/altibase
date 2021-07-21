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
 
#ifndef _O_SVM_MANAGER_H_
#define _O_SVM_MANAGER_H_ 1

#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <svmDef.h>
#include <sctTableSpaceMgr.h>
#include <smu.h>

// svmManager를 초기화 하는 모드
typedef enum
{
    // 일반 STARTUP - 디스크상에 데이터베이스 파일 존재
    SVM_MGR_INIT_MODE_NORMAL_STARTUP = 1,
    // CREATEDB도중 - 아직 디스크상에 데이터베이스 파일이 존재하지 않음
    SVM_MGR_INIT_MODE_CREATEDB = 2
} svmMgrInitMode ;

// fillPCHEntry 함수에서 페이지 메모리의 복사여부를 결정
typedef enum svmFillPCHOption {
    SVM_FILL_PCH_OP_NONE = 0 ,
    /** 페이지 메모리의 내용을 PCH 안의 Page에 복사한다 */
    SVM_FILL_PCH_OP_COPY_PAGE = 1,
    /** 페이지 메모리의 포인터를 PCH 안의 Page에 세팅한다 */
    SVM_FILL_PCH_OP_SET_PAGE = 2
} svmFillPCHOption ;


// allocNewExpandChunk 함수의 옵션
typedef enum svmAllocChunkOption 
{
    SVM_ALLOC_CHUNK_OP_NONE = 0,              // 옵션없음
    SVM_ALLOC_CHUNK_OP_CREATEDB         = 1   // createdb 도중임을 알림
} svmAllocChunkOption ;

/*********************************************************************
  메모리 데이터베이스의 페이지 사용
 *********************************************************************   
  --------------------------------------------------
   Page#1  ~ Page#10  Expand Chunk #0
  --------------------------------------------------
   Page#11 ~ Page#20  Expand Chunk #1
  --------------------------------------------------
 *********************************************************************/

class svmManager
{
private :
    // 일반 메모리 Page Pool을 초기화한다.
    static IDE_RC initializeDynMemPool(svmTBSNode * aTBSNode);
    
    // Tablespace에 할당된 Page Pool을 파괴한다.
    static IDE_RC destroyPagePool( svmTBSNode * aTBSNode );

    // 새로 할당된 Expand Chunk안에 속하는 Page들의 PCH Entry를 할당한다.
    // Chunk안의 Free List Info Page의 Page Memory도 할당한다.
    static IDE_RC fillPCHEntry4AllocChunk(svmTBSNode * aTBSNode,
                                          scPageID     aNewChunkFirstPID,
                                          scPageID     aNewChunkLastPID );
    
    static void dump(FILE     *a_fp,
                     scPageID  a_no);

    // Prepare-> Restore의 과정에서 Prepare중에만 쓰이는
    // 임시 Page Memory를 관리하는 Memory Pool 
    static SChar * mPageBuffer4PrepareTBS; 
    static UInt  mPageBuffer4PrepareTBSSize;

    // 특정 Page의 PCH안의 Page Memory를 할당한다.
    static IDE_RC allocPageMemory( svmTBSNode * aTBSNode,
                                   scPageID     aPID );

    // 페이지 메모리를 할당하고, 해당 Page를 초기화한다.
    // 필요한 경우, 페이지 초기화에 대한 로깅을 실시한다
    static IDE_RC allocAndLinkPageMemory( svmTBSNode * aTBSNode,
                                          scPageID     aPID,
                                          scPageID     aPrevPID,
                                          scPageID     aNextPID );
    
    // 특정 Page의 PCH안의 Page Memory를 해제한다.
    static IDE_RC freePageMemory( svmTBSNode * aTBSNode, scPageID aPID );


    // FLI Page에 Next Free Page ID로 링크된 페이지들에 대해
    // PCH안의 Page 메모리를 할당하고 Page Header의 Prev/Next포인터를 연결한다.
    static IDE_RC allocFreePageMemoryList( svmTBSNode * aTBSNode,
                                           scPageID     aHeadPID,
                                           scPageID     aTailPID,
                                           vULong     * aPageCount );

    // PCH의 Page속의 Page Header의 Prev/Next포인터를 기반으로
    // FLI Page에 Next Free Page ID를 설정한다.
    static IDE_RC linkFreePageList( svmTBSNode * aTBSNode,
                                    void       * aHeadPage,
                                    void       * aTailPage,
                                    vULong     * aPageCount );
    
    /*
     * Page의 PCH(Page Control Header)정보를 구성한다.
     */
    // PCH 할당및 초기화
    static IDE_RC allocPCHEntry(svmTBSNode *  aTBSNode,
                                scPageID      a_pid);

    // PCH 해제
    static IDE_RC freePCHEntry(svmTBSNode *  aTBSNode,
                               scPageID     a_pid);

    // 모든 PCH 메모리를 해제한다.
    static IDE_RC freeAll(svmTBSNode * aTBSNode);

    // 데이터 베이스의 Base Page (Page#0) 와 관련한 정보를 설정한다.
    // 이와 관련된 정보로는  MemBase와 Catalog Table정보가 있다.
    static IDE_RC setupBasePageInfo( svmTBSNode * aTBSNode, UChar *aBasePage );
    
    static IDE_RC freeFreePageMemoryList( svmTBSNode * aTBSNode,
                                          void       * aHeadPage,
                                          void       * aTailPage,
                                          vULong     * aPageCount );

    // BUG-47487: DATA / FLI 페이지 여부 추가 ( Volatile ) 
    static IDE_RC allocPage( svmTBSNode *   aTBSNode, 
                             svmTempPage ** aPage,
                             idBool         aIsDataPage = ID_TRUE );
    // BUG-47487: DATA / FLI 페이지 여부 추가 ( Volatile )
    static IDE_RC freePage( svmTBSNode  * aTBSNode, 
                            svmTempPage * aPage,
                            idBool        aIsDataPage = ID_TRUE );

public:
    
    /* --------------------
     * Class Control
     * -------------------*/
    svmManager();
    // svmManager를 초기화한다.
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    // TBSNode를 초기화한다.
    static IDE_RC allocTBSNode(svmTBSNode        **aTBSNode,
                               smiTableSpaceAttr  *aTBSAttr);

    // TBSNode를 해제한다.
    static IDE_RC destroyTBSNode(svmTBSNode *aTBSNode);

    // Volatile TBS를 초기화한다.
    static IDE_RC initTBS(svmTBSNode *aTBSNode);

    // Volatile TBS를 해제한다.
    static IDE_RC finiTBS(svmTBSNode *aTBSNode);

    // Base Page ( 0번 Page ) 에 Latch를 건다
    static IDE_RC lockBasePage(svmTBSNode * aTBSNode);
    // Base Page ( 0번 Page ) 에서 Latch를 푼다.
    static IDE_RC unlockBasePage(svmTBSNode * aTBSNode);

    /* -----------------------
     * persistent  memory
     * manipulations
     * ----------------------*/
    
    // 여러개의 Expand Chunk를 추가하여 데이터베이스를 확장한다.
    static IDE_RC allocNewExpandChunks( svmTBSNode *  aTBSNode,
                                        UInt          aExpandChunkCount );

    
    // 특정 페이지 범위만큼 데이터베이스를 확장한다.
    static IDE_RC allocNewExpandChunk( svmTBSNode *  aTBSNode,
                                       scPageID      aNewChunkFirstPID,
                                       scPageID      aNewChunkLastPID);

#if 0  // not used 
    // DB로부터 하나의 Page를 할당받는다.
    static IDE_RC allocatePersPage (void       *  aTrans,
                                    scSpaceID     aSpaceID,
                                    void      **  aAllocatedPage);
    
#endif    
    // DB로부터 Page를 여러개 할당받아 트랜잭션에게 Free Page를 제공한다.
    // aHeadPage부터 aTailPage까지
    // Page Header의 Prev/Next포인터로 연결해준다.
    static IDE_RC allocatePersPageList (void        *aTrans,
                                        scSpaceID    aSpaceID,
                                        UInt         aPageCount,
                                        void       **aHeadPage,
                                        void       **aTailPage,
                                        UInt        *aAllocPageCnt );
#if 0
    // 하나의 Page를 데이터베이스로 반납한다
    static IDE_RC freePersPage (void     *   aTrans,
                                scSpaceID    aSpaceID,
                                void     *   aToBeFreePage );
#endif
    // 여러개의 Page를 한꺼번에 데이터베이스로 반납한다.
    // aHeadPage부터 aTailPage까지
    // Page Header의 Prev/Next포인터로 연결되어 있어야 한다.
    static IDE_RC freePersPageList (void        *aTrans,
                                    scSpaceID    aSpaceID,
                                    void        *aHeadPage,
                                    void        *aTailPage);
    
    /* -----------------------
     * [*] retrieval of 
     *     the memory manager
     *     informations
     * ----------------------*/
    // 특정 Page의 PCH를 가져온다.

    // 특정 Page에 S래치를 획득한다. ( 현재는 X래치로 구현되어 있다 )
    static IDE_RC            holdPageSLatch(scSpaceID aSpaceID,
                                            scPageID  aPageID);
    // 특정 Page에 X래치를 획득한다. 
    static IDE_RC            holdPageXLatch(scSpaceID aSpaceID,
                                            scPageID  aPageID);
    // 특정 Page에서 래치를 풀어준다.
    static IDE_RC            releasePageLatch(scSpaceID aSpaceID,
                                              scPageID  aPageID);

    static IDE_RC allocPageAlignedPtr(UInt a_nSize, void**, void**);

    static UInt   getDBMaxPageCount(svmTBSNode * aTBSNode)
    {
        return (UInt)aTBSNode->mDBMaxPageCount;
    }

    static void   getTableSpaceAttr(svmTBSNode * aTBSNode,
                                    smiTableSpaceAttr * aTBSAttr)
    {
        IDE_DASSERT( aTBSNode != NULL );
        IDE_DASSERT( aTBSAttr != NULL );
        
        idlOS::memcpy(aTBSAttr,
                      &aTBSNode->mTBSAttr,
                      ID_SIZEOF(smiTableSpaceAttr));
        aTBSAttr->mType = aTBSNode->mHeader.mType;
        aTBSAttr->mTBSStateOnLA = aTBSNode->mHeader.mState;
    }

    static UInt getTBSAttrFlag( svmTBSNode * aTBSNode )
    {
        IDE_DASSERT( aTBSNode != NULL );

        return aTBSNode->mTBSAttr.mAttrFlag;
    }

    static void setTBSAttrFlag( svmTBSNode * aTBSNode,
                                UInt         aAttrFlag )
    {
        IDE_DASSERT( aTBSNode != NULL );

        aTBSNode->mTBSAttr.mAttrFlag = aAttrFlag;
    }
    
    // 데이터베이스 크기를 이용하여 데이터베이스 Page수를 구한다.
    static ULong calculateDbPageCount(ULong aDbSize, ULong aChunkPageCount);

    /***********************************************************
     * svmTableSpace 에서 호출하는 함수들
     ***********************************************************/
    // Tablespace에서 사용할 Page 메모리 풀을 초기화한다.
    static IDE_RC initializePagePool( svmTBSNode * aTBSNode );

    // Tablespace의 Meta Page를 초기화하고 Free Page들을 생성한다.
    static IDE_RC createTBSPages( svmTBSNode      * aTBSNode,
                                  SChar           * aDBName,
                                  scPageID          aCreatePageCount);

};

#if 0
/* TRUE만 리턴하고 있어서 함수삭제함. */

// Page 수가 Valid한 값인지 체크한다.
inline idBool
svmManager::isValidPageCount( svmTBSNode * /* aTBSNode*/,
                              vULong       /*aPageCount*/ )
{
/*
    IDE_ASSERT( mHighLimitPAGE > 0 );
    return ( aPageCount <= mHighLimitPAGE ) ?
             ID_TRUE : ID_FALSE ;
*/
    return ID_TRUE;
}
#endif



#endif // _O_SVM_MANAGER_H_
