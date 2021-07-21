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
 * $Id: sdcUndoSegment.h 90259 2021-03-19 01:22:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 undo segment에 대한 header 파일이다
 *
 * # 개념
 *
 *   undo record는 DRDB의 MVCC, garbage collecting, 트랜잭션 rollback
 *   처리에 필요한 undo record를 저장하기 위한 segment 자료구조이다.
 *
 **********************************************************************/

#ifndef _O_SDC_UNDO_SEGMENT_H_
#define _O_SDC_UNDO_SEGMENT_H_ 1

#include <smr.h>

class sdcUndoSegment
{
public:

    IDE_RC initialize( idvSQL        * aStatistics,
                       sdcTXSegEntry * aEntry,
                       scPageID        aUDSegPID );

    IDE_RC destroy();

    static IDE_RC create( idvSQL          * aStatistics,
                          sdrMtxStartInfo * aStartInfo,
                          scPageID        * aUDSegPID );

    IDE_RC addInsertRowPieceUndoRec(
                        idvSQL                      *aStatistics,
                        sdrMtxStartInfo             *aStartInfo,
                        smOID                        aTableOID,
                        scGRID                       aInsRecGRID,
                        const sdcRowPieceInsertInfo *aInsertInfo,
                        idBool                       aIsUndoRec4HeadRowPiece );

    IDE_RC addUpdateRowPieceUndoRec(
                        idvSQL                      *aStatistics,
                        sdrMtxStartInfo             *aStartInfo,
                        smOID                        aTableOID,
                        const UChar                 *aUptRecPtr,
                        scGRID                       aUptRecGRID,
                        const sdcRowPieceUpdateInfo *aUpdateInfo,
                        idBool                       aReplicate,
                        sdSID                       *aUndoSID );

    IDE_RC addOverwriteRowPieceUndoRec(
                        idvSQL                         *aStatistics,
                        sdrMtxStartInfo                *aStartInfo,
                        smOID                           aTableOID,
                        const UChar                    *aUptRecPtr,
                        scGRID                          aUptRecGRID,
                        const sdcRowPieceOverwriteInfo *aOverwriteInfo,
                        idBool                          aReplicate,
                        sdSID                          *aUndoSID );

    IDE_RC addChangeRowPieceLinkUndoRec(
                        idvSQL                      *aStatistics,
                        sdrMtxStartInfo             *aStartInfo,
                        smOID                        aTableOID,
                        const UChar                 *aUptRecPtr,
                        scGRID                       aUptRecGRID,
                        idBool                       aIsTruncateNextLink,
                        sdSID                       *aUndoSID );

    IDE_RC addDeleteFstColumnPieceUndoRec(
                            idvSQL                      *aStatistics,
                            sdrMtxStartInfo             *aStartInfo,
                            smOID                        aTableOID,
                            const UChar                 *aUptRecPtr,
                            scGRID                       aUptRecGRID,
                            const sdcRowPieceUpdateInfo *aUpdateInfo,
                            idBool                       aReplicate,
                            sdSID                       *aUndoSID );

    IDE_RC addDeleteRowPieceUndoRec(
                            idvSQL                         *aStatistics,
                            sdrMtxStartInfo                *aStartInfo,
                            smOID                           aTableOID,
                            UChar                          *aDelRecPtr,
                            scGRID                          aDelRecGRID,
                            idBool                          aIsDelete4Upt,
                            idBool                          aReplicate,
                            const sdcRowPieceUpdateInfo    *aUpdateInfo,
                            sdSID                          *aUndoSID );

    IDE_RC addLockRowUndoRec(
                            idvSQL                      *aStatistics,
                            sdrMtxStartInfo             *aStartInfo,
                            smOID                        aTableOID,
                            const UChar                 *aRecPtr,
                            scGRID                       aRecGRID,
                            idBool                       aIsExplicitLock,
                            sdSID                       *aUndoSID );
#if 0
    IDE_RC addIndexCTSlotUndoRec(
                            idvSQL                      *aStatistics,
                            sdrMtxStartInfo             *aStartInfo,
                            smOID                        aTableOID,
                            const UChar                 *aRecPtr,
                            UShort                       aRecSize,
                            sdSID                       *aUndoSID );
#endif

    /* PROJ-2047 Strengthening LOB */
    IDE_RC addUpdateLobLeafKeyUndoRec(
                            idvSQL           *aStatistics,
                            sdrMtxStartInfo*  aStartInfo,
                            smOID             aTableOID,
                            UChar*            aKey,
                            sdSID            *aUndoSID );

    /* TASK-4007 [SM] PBT를 위한 기능 추가 - dumpddf정상화
     * Undo 페이지 Dump */
    static IDE_RC dump( UChar *aPage ,
                        SChar *aOutBuf ,
                        UInt   aOutSize );

    inline scPageID getSegPID() { return mUDSegDesc.mSegHandle.mSegPID; }
    inline scPageID getCurAllocExtDir() { return SD_MAKE_PID(mCurAllocExtRID); }
    inline sdpSegHandle * getSegHandle() {
        return sdpSegDescMgr::getSegHandle(&mUDSegDesc); };

    inline void setCurAllocInfo( sdRID     aCurAllocExtRID,
                                 scPageID  aFstPIDOfCurAllocExt,
                                 scPageID  aCurAllocPID );

    static IDE_RC abortCurAllocInfo( void * aUndoSegment );

    inline scPageID getFstPIDOfCurAllocExt() 
                      { return mFstPIDOfCurAllocExt; }

    static IDE_RC logAndInitPage( sdrMtx    * aMtx,
                                  UChar     * aPagePtr );

    IDE_RC markSCN4ReCycle( idvSQL   * aStatistics,
                            scSpaceID  aSpaceID,
                            scPageID   aSegPID,
                            sdRID      aFstExtRID,
                            sdRID      aLstExtRID,
                            smSCN    * aCSCNorASCN );

    /* BUG-31055 Can not reuse undo pages immediately after it is used to 
     *           aborted transaction */
    IDE_RC shrinkExts( idvSQL            * aStatistics,
                       sdrMtxStartInfo   * aStartInfo,
                       scSpaceID           aSpaceID,
                       scPageID            aSegPID,
                       sdRID               aFstExtRID,
                       sdRID               aLstExtRID );

    IDE_RC build4SegmentPerfV( void                * aHeader,
                               iduFixedTableMemory * aMemory );

    IDE_RC build4RecordPerfV( UInt                  aSegSeq,
                              void                * aHeader,
                              iduFixedTableMemory * aMemory );

    static inline IDE_RC initPage( UChar  * aPagePtr );

    static UShort getUndoRecSize( UChar    * aSlotDirStartPtr,
                                  scSlotNum  aSlotNum );

    IDE_RC prepareNewPage( idvSQL  * aStatistics,
                           sdrMtx  * aMtx );

private:

    IDE_RC allocNewPage( idvSQL  * aStatistics,
                         sdrMtx  * aMtx,
                         UChar  ** aNewPagePtr );
#if 0  //not used
    IDE_RC stealFromOfflineEntry( idvSQL          * aStatistics,
                                  sdrMtxStartInfo * aStartInfo,
                                  smSCN           * aOldestTransBSCN );
#endif
    /* 어떤 타입이든 undo record를 작성한 후 이 함수가 호출되어
       redo log를 mtx에 버퍼링한다. */
    IDE_RC makeRedoLogOfUndoRec(UChar*        aUndoRecPtr,
                                UShort        aLength,
                                smOID         aTableOID,
                                sdrMtx*       aMtx);

    /* undo record를 기록할 undo record ptr를 반환 */
    IDE_RC allocSlot( idvSQL           * aStatistics,
                      void             * aTrans,
                      UShort             aUndoRecSize,
                      idBool             aIsAllocSlotEntry,
                      sdrMtx           * aMtx,
                      UChar           ** aSlotPtr,
                      sdSID            * aUndoSID );

    inline IDE_RC getCurPID4Update( idvSQL    * aStatistics,
                                    sdrMtx    * aMtx,
                                    UChar    ** aCurPagePtr );

    inline IDE_RC getCurPID4Read( idvSQL    * aStatistics,
                                  sdrMtx    * aMtx,
                                  UChar    ** aCurPagePtr );

private:

    ULong            mFreeUndoPageCnt;   /* Free된 UndoPage 개수 */
    ULong            mCreateUndoPageCnt; /* 생성된 UndoPage 개수 */
    sdcTXSegEntry *  mEntryPtr;          /* 자신이 소속된 트랜잭션 세그먼트 엔트리 포인터 */

    sdpSegmentDesc   mUDSegDesc;           /* Segment 기술자 */
    sdRID            mCurAllocExtRID;      /* 현재 사용중인 ExtDesc의 RID */
    scPageID         mFstPIDOfCurAllocExt; /* 현재 사용중인 ExtDesc의 첫번째 PID */
    scPageID         mCurAllocPID;             /* 현재 사용중인 Undo 페이지의 PID */
    smSCN            mFstDskViewSCNofCurTrans; /* 현재 사용중인 트랜잭션의
                                                * 첫번째 Dsk Stmt ViewSCN */

    /* 위 Cache에 대해 Mtx Rollback이 일어났을때 복구하기 위한 백업본 */
    sdRID            mCurAllocExtRID4MtxRollback;
    scPageID         mFstPIDOfCurAllocExt4MtxRollback;
    scPageID         mCurAllocPID4MtxRollback;
};

/***********************************************************************
 * Description : Undo Page 초기화
 **********************************************************************/
inline IDE_RC sdcUndoSegment::initPage( UChar  * aPagePtr )
{
    scOffset     sOffset;
    scSlotNum    sAllocSlotNum;

    IDE_ASSERT( aPagePtr != NULL );

    sdpSlotDirectory::init( (sdpPhyPageHdr*)aPagePtr );

    sOffset = (scOffset)(sdpPhyPage::getPageFooterStartPtr( aPagePtr ) -
                         aPagePtr);

    // 첫번째 SlotEntry는 Footer의 Offset을 저장한다.
    // 이후에 SlotEntry(i) - SlotEntry(i-1)로
    // UndoRec의 길이를 구할수 있다.
    IDE_ERROR( sdpSlotDirectory::alloc( (sdpPhyPageHdr*)aPagePtr,
                                        sOffset,
                                        &sAllocSlotNum )
               == IDE_SUCCESS );

    IDE_ASSERT( sAllocSlotNum == 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 현재 사용중인 UndoPage에 X-latch 획득후 반환
 *
 * aStatistics [IN]  - 통계정보
 * aMtx        [IN]  - Mtx 포인터
 * aCurPagePtr [OUT] - 현재 사용중인 Undo 페이지 포인터
 **
 * ********************************************************************/
inline IDE_RC sdcUndoSegment::getCurPID4Update( idvSQL    * aStatistics,
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
 * Description : 현재 사용중인 UndoPage에 S-latch 획득후 반환
 *
 * aStatistics [IN]  - 통계정보
 * aMtx        [IN]  - Mtx 포인터
 * aCurPagePtr [OUT] - 현재 사용중인 Undo 페이지 포인터
 *
 **********************************************************************/
inline IDE_RC sdcUndoSegment::getCurPID4Read( idvSQL    * aStatistics,
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
 * Description : 현재 UndoPage 할당정보 설정
 *
 * [IN] aCurAllocExtRID      - 현재 사용중인 ExtDesc RID
 * [IN] aFstPIDOfCurAllocExt - 현재 사용중인 ExtDesc의 첫번째 PID
 * [IN] aCurAllocExtRID      - 현재 사용중인 페이지의 PID
 *
 **********************************************************************/
inline void sdcUndoSegment::setCurAllocInfo( sdRID     aCurAllocExtRID,
                                             scPageID  aFstPIDOfCurAllocExt,
                                             scPageID  aCurAllocPID )
{
    /* Mtx Rollback이 일어났을때 복구하기 위한 백업본 */
    mCurAllocExtRID4MtxRollback      = mCurAllocExtRID;
    mFstPIDOfCurAllocExt4MtxRollback = mFstPIDOfCurAllocExt;
    mCurAllocPID4MtxRollback         = mCurAllocPID;

    mCurAllocExtRID      = aCurAllocExtRID;
    mFstPIDOfCurAllocExt = aFstPIDOfCurAllocExt;
    mCurAllocPID         = aCurAllocPID;
}

#endif // _O_SDC_UNDO_SEGMENT_H_
