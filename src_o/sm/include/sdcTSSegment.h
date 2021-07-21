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
 * $Id: sdcTSSegment.h 89495 2020-12-14 05:19:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 Transaction Status Slot Segment의 헤더파일입니다.
 *
 * # 개념
 *
 *   DRDB의 MVCC과 관련하여 트랜잭션 상태를 관리하기 위한
 *   Transaction Status Slot을 저장하기 위한 세그먼트
 *
 **********************************************************************/

# ifndef _O_SDC_TSSEGMENT_H_
# define _O_SDC_TSSEGMENT_H_ 1

# include <sdcDef.h>

struct sdcTXSegEntry;

class sdcTSSegment
{
public:

    static IDE_RC create( idvSQL          * aStatistics,
                          sdrMtxStartInfo * aStartInfo,
                          scPageID        * aTSSegPID );

    IDE_RC initialize( idvSQL        * aStatistics,
                       sdcTXSegEntry * aEntry,
                       scPageID        aTSSegPID );

    IDE_RC destroy();

    IDE_RC resetUndoSegments( idvSQL  * aStatistics );

    IDE_RC bindTSS( idvSQL            * aStatistics,
                    sdrMtxStartInfo   * aStartInfo );

    static IDE_RC unbindTSS4Commit( idvSQL  * aStatistics,
                                    sdSID     aTSSlotSID,
                                    smSCN   * aSCN );

    static IDE_RC unbindTSS4Abort( idvSQL  * aStatistics,
                                   sdrMtx  * aMtx,
                                   sdSID     aTSSlotSID,
                                   smSCN   * aInitSCN );

    static IDE_RC getCommitSCN( idvSQL      * aStatistics,
                                void        * aTrans,
                                sdSID         aTSSlotSID,
                                smSCN       * aFstDskViewSCN,
                                smSCN         aStmtViewSCN,
                                smTID       * aTID4Wait,
                                smSCN       * aCommitSCN );

    UInt   getAlignSlotSize() { return mAlignSlotSize; }

    inline scPageID getSegPID() { return mTSSegDesc.mSegHandle.mSegPID; }
    inline scPageID getCurAllocExtDir() { return SD_MAKE_PID(mCurAllocExtRID); }
    inline sdpSegHandle * getSegHandle() {
        return sdpSegDescMgr::getSegHandle(&mTSSegDesc); };

    inline void setCurAllocInfo( sdRID     aCurAllocExtRID,
                                 scPageID  aFstPIDOfCurAllocExt,
                                 scPageID  aCurAllocPID );

    static IDE_RC abortCurAllocInfo( void * aUndoSegment );

    inline scPageID getFstPIDOfCurAllocExt() { return mFstPIDOfCurAllocExt; }

    static IDE_RC logAndInitPage( sdrMtx    * aMtx,
                                  UChar     * aPagePtr,
                                  smTID       aTransID,
                                  smSCN       aFstDskViewSCN );

    IDE_RC markSCN4ReCycle( idvSQL   * aStatistics,
                            scSpaceID  aSpaceID,
                            scPageID   aSegPID,
                            sdRID      aFstExtRID,
                            smSCN    * aCSCNorASCN );

    IDE_RC build4SegmentPerfV( void                * aHeader,
                               iduFixedTableMemory * aMemory );

    IDE_RC build4RecordPerfV( UInt                  aSegSeq,
                              void                * aHeader,
                              iduFixedTableMemory * aMemory );

    /* TASK-4007 [SM] PBT를 위한 기능 추가
     * TSS Dump할 수 있는 기능 추가*/
    static IDE_RC dump( UChar *aPage ,
                        SChar *aOutBuf ,
                        UInt   aOutSize );

    static inline void initPage( UChar    * aPagePtr,
                                 smTID      aTransID,
                                 smSCN      aFstDskViewSCN );

private:

    IDE_RC allocNewPage( idvSQL  * aStatistics,
                         sdrMtx  * aMtx,
                         smTID     aTransID,
                         smSCN     aTransBSCN,
                         UChar  ** aNewPagePtr );

    inline IDE_RC getCurPID4Update( idvSQL    * aStatistics,
                                    sdrMtx    * aMtx,
                                    UChar    ** aCurPagePtr );

    inline IDE_RC getCurPID4Read( idvSQL    * aStatistics,
                                  sdrMtx    * aMtx,
                                  UChar    ** aCurPagePtr );

public:

    UInt             mSlotSize;            /* TSS 슬롯의 바이트 크기 */
    UInt             mAlignSlotSize;       /* TSS 슬롯의 정렬된 바이트 크기 */
    sdcTXSegEntry *  mEntryPtr;            /* 자신이 소속된 트랜잭션 세그먼트 엔트리 포인터 */

    sdpSegmentDesc   mTSSegDesc;           /* Segment 기술자 */
    sdRID            mCurAllocExtRID;      /* 현재 혹은 마지막 사용한 ExtDesc의 RID */
    scPageID         mFstPIDOfCurAllocExt; /* 현재 혹은 마지막 사용한 ExtDesc의 첫번째 PageID */
    scPageID         mCurAllocPID;         /* 현재 혹은 마지막 사용한 TSS 페이지의 PID */

    /* 위 Cache에 대해 Mtx Rollback이 일어났을때 복구하기 위한 백업본 */
    sdRID            mCurAllocExtRID4MtxRollback;
    scPageID         mFstPIDOfCurAllocExt4MtxRollback;
    scPageID         mCurAllocPID4MtxRollback;
};



/***********************************************************************
 *
 * Description : TSS Page의 CntlHdr 초기화
 *
 * aPagePtr       - [IN] 페이지 포인터
 * aTransID       - [IN] 페이지를 할당한 트랜잭션 ID
 * aFstDskViewSCN - [IN] 페이지를 할당한 트랜잭션의 Begin SCN
 *
 **********************************************************************/
void sdcTSSegment::initPage( UChar  * aPagePtr,
                             smTID    aTransID,
                             smSCN    aFstDskViewSCN )
{
    sdcTSSPageCntlHdr   * sCntlHdr;

    IDE_ASSERT( aPagePtr != NULL );

    sCntlHdr =
        (sdcTSSPageCntlHdr*)
        sdpPhyPage::initLogicalHdr( (sdpPhyPageHdr*)aPagePtr,
                                    ID_SIZEOF( sdcTSSPageCntlHdr ) );

    sCntlHdr->mTransID  = aTransID;
    SM_SET_SCN( &sCntlHdr->mFstDskViewSCN, &aFstDskViewSCN );

    sdpSlotDirectory::init( (sdpPhyPageHdr*)aPagePtr );
}

/***********************************************************************
 *
 * Description : TSS Page에 X-Latch 획득 반환
 *
 * aStatistics [IN]  - 통계정보
 * aMtx        [IN]  - Mtx 포인터
 * aCurPagePtr [OUT] - 현재 사용중인 TSS 페이지 포인터
 *
 **********************************************************************/
inline IDE_RC sdcTSSegment::getCurPID4Update( idvSQL    * aStatistics,
                                              sdrMtx    * aMtx,
                                              UChar    ** aCurPagePtr )
{
    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aCurPagePtr != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          mCurAllocPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          aCurPagePtr,
                                          NULL,
                                          NULL )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : TSS Page에 S-Latch 획득 반환
 *
 * aStatistics [IN]  - 통계정보
 * aMtx        [IN]  - Mtx 포인터
 * aCurPagePtr [OUT] - 현재 사용중인 TSS 페이지 포인터
 *
 **********************************************************************/
inline IDE_RC sdcTSSegment::getCurPID4Read( idvSQL    * aStatistics,
                                            sdrMtx    * aMtx,
                                            UChar    ** aCurPagePtr )
{
    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aCurPagePtr != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          mCurAllocPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          aCurPagePtr,
                                          NULL,
                                          NULL )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 현재 할당한 TSS 페이지의 정보 설정
 *
 * [IN] aCurAllocExtRID      - 현재 사용중인 ExtDesc RID
 * [IN] aFstPIDOfCurAllocExt - 현재 사용중인 ExtDesc의 첫번째 PID
 * [IN] aCurAllocExtRID      - 현재 사용중인 페이지의 PID
 *
 **********************************************************************/
inline void sdcTSSegment::setCurAllocInfo( sdRID     aCurAllocExtRID,
                                           scPageID  aFstPIDOfCurAllocExt,
                                           scPageID  aCurAllocPID )
{
    /* Mtx Rollback이 일어났을때 복구하기 위한 백업본 */
    mCurAllocExtRID4MtxRollback      = mCurAllocExtRID;
    mFstPIDOfCurAllocExt4MtxRollback = mFstPIDOfCurAllocExt;
    mCurAllocPID4MtxRollback         = mCurAllocPID;

    mCurAllocExtRID            = aCurAllocExtRID;
    mFstPIDOfCurAllocExt       = aFstPIDOfCurAllocExt;
    mCurAllocPID               = aCurAllocPID;
}

# endif // _O_SDC_TSSEGMENT_H_
