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
 * $Id: sdrMiniTrans.h 89495 2020-12-14 05:19:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 mini-transaction에 대한 헤더 파일이다.
 *
 * # 개념
 *
 * mini-transaction이란 특정 단위작업에 대한 page fix와 변경 redo
 * 로그들을 저장해 두었다가 mini-transaction의 commit연산을 수행시
 * 한꺼번에 저장했던 로그를 로그파일에 write 하고, fix되었던 page를
 * unfix시켜서 actomic을 보장하기 위한 객체이다.
 * 이하 mini-transaction을 mtx라고 한다
 *
 * # 설명
 *
 * mtx을 시작한 이후에 얻는 모든 버퍼 프레임과 버퍼에 대한
 * latch는 mtx에 push된다.
 * 만약 한 단위의 작업 중 중간에 죽는다면 그 작업의 어떤 내용도
 * 로그에 반영되지 않으며, mtx commit시에만 로그에 그 내용이 반영된다.
 * mtx commit시에만 버퍼들이 unfix되므로 mtx 중간에 페이지에 대한 어떠한
 * 변경도 디스크에 반영되지 않는다.
 * 실제 로그에 write한 이후가 되어야 디스크에 변경한 페이지가 반영될 수 있다.
 * 미니트랜잭션이 start후 commit하는 동안의 행위는 atomic 하다고
 * 할 수 있다. 이를 위해 mtx 내부에서는 로그 버퍼를 유지한다.
 *
 * mtr start -> commit 되기 까지 mtx는 다음의 작업을 수행한다.
 *    - 오브젝트와 latch모드를 저장
 *    - 버퍼 프레임 fix
 *    - 로그를 로그버퍼에  저장
 *
 * commit되면 이와 같은 작업을 한다.
 *    - 저장된 오브젝트와 latch 모드를 해제
 *    - 사용된 버퍼 프레임 unfix
 *    - 로그버퍼를 실제 log에 write
 *
 * 1) mtx 구성 요소
 *      __________
 *      | sdrMtx |_______________________________
 *      |________|                               |
 *     ______|_____________             mtx stack(latch stack)
 *     |                  |               _______|______
 *     | dynamic          |               |_latch item_|
 *     | log buffer       |               |____________|
 *     |                  |               |____________|
 *     |                  |               |____________|
 *     |__________________|               |____________|
 *
 *
 *    - sdrMtx
 *    mtx의 모든 정보를 저장한다. atomic을 보장해야 하는
 *    작업을 위해서 mtx를 시작한다. 다음과 같은 구성요소가 있다.
 *
 *    1) log buffer
 *    dynamic 버퍼(smuDynArray)로 mtx start 이후에 연산에 의해 발생하는
 *    로그를 기록한다. mtx commit 시에 실제 로그파일버퍼에 write 한다.
 *
 *    2) sdrMtxStackInfo(Latch Stack)
 *    latch item을 쌓아둔다. 이 item들은 commit시에 모두 해제된다.
 *
 *    - sdrMtxLatchItem
 *    latchStack에 저장되는 구조이다. 오브젝트와 latch 모드로 구성됨
 *    오브젝트로는 BCB, 래치, 뮤텍스가 될 수 있다.
 *
 * 2) mtx에 저장되는 DRDB의 로그의 구조
 *
 *  __header(16bytes)_  _________body_____
 * /__________________\/__________________\
 * | type | RID | len | log-specipic body |
 * |______|_____|_____|___________________|
 *
 * mtx start 이후 이러한 로그가 한개 이상 반복되어 commit 될 때까지
 * 쌓인다. mtx commit 시에 로그 버퍼는 로그 관리자에 전달된다.
 *
 * 3) 로그 관리자가 기록하는 실제 로그의 구조
 *
 *  ______________ header__________________  ____body_________  tail
 * /_______________________________________\/_______________ _\/_____\
 * | type | TxID | flag | previous undo LSN | 여러개의 Mtx Log |      |
 * |______|______|______|___________________|__________________|______|
 *
 * 실제 로그화일에 쓰이는 로그는 header와 여러 개의 DRDB 로그로 구성된
 * body, 그리고 tail로 구성된다.
 * 로그 header의 정보를 구성하기 위해서는 type과 TxID가 필요하며 이를
 * mtx start시 받아서 commit시 로그에 write할 때 전달한다.
 *
 **********************************************************************/

#ifndef _O_SDR_MINI_TRANS_H_
#define _O_SDR_MINI_TRANS_H_ 1

#include <sdrDef.h>

class sdrMiniTrans
{
public:

    /* mtx 초기화 및 시작 */
    static IDE_RC begin( idvSQL*       aStatistics,
                         sdrMtx*       aMtx,
                         void*         aTrans,
                         sdrMtxLogMode aLogMode,
                         idBool        aUndoable,
                         UInt          aDLogAttr );


    static IDE_RC begin( idvSQL*          aStatistics,
                         sdrMtx*          aMtx,
                         sdrMtxStartInfo* aStartInfo,
                         idBool           aUndoable,
                         UInt             aDLogAttr );


    /* ------------------------------------------------
     * !!] mtx로 획득한 latchitem을 release하며, 작성된
     * 로그를 로그파일에 기록하고, 모든 resource를 해제한다
     * ----------------------------------------------*/
    static IDE_RC commit( sdrMtx * aMtx,
                          UInt     aContType = 0,      /* in */
                          smLSN  * aEndLSN   = NULL,   /* in */
                          UInt     aRedoType = 0 );    /* in */

    static IDE_RC setDirtyPage(void*    aMtx,
                               UChar*   aPagePtr );

    /* 일반 페이지임에도 log를 안남길때 설정. (ex: Index Bottom-up Build) */
    static void setNologgingPersistent( sdrMtx* aMtx );

    static void makeStartInfo( sdrMtx* aMtx,
                               sdrMtxStartInfo * aStartInfo );

    /* 특정 연산에 대한 NTA 로그 설정 */
    static void setNTA( sdrMtx*   aMtx,
                        scSpaceID aSpaceID,
                        UInt      aOpType,
                        smLSN*    aPPrevLSN,
                        ULong   * aArrData,
                        UInt      aDataCount );

    static void setRefNTA( sdrMtx*   aMtx,
                           scSpaceID aSpaceID,
                           UInt      aOpType,
                           smLSN*    aPPrevLSN );

    static void setNullNTA( sdrMtx   * aMtx,
                            scSpaceID  aSpaceID,
                            smLSN*     aPPrevLSN );

    static void setCLR(sdrMtx* aMtx,
                       smLSN*  aPPrevLSN);

    /* Rollback하더라도 Undo를 안해도 됨 */
    static void setIgnoreMtxUndo( sdrMtx* aMtx )
    {
        /* BUG-32579 The MiniTransaction commit should not be used in
         * exception handling area.
         * 
         * 부분적으로 로그가 쓰여진 상황에서는 MtxUndo가 가능한 상황
         * 이 아니다. */ 
        IDE_ASSERT( aMtx->mRemainLogRecSize == 0 );

        aMtx->mFlag &= ~SDR_MTX_IGNORE_UNDO_MASK;
        aMtx->mFlag |= SDR_MTX_IGNORE_UNDO_ON;
    }

    /* DML관련 redo/undo 로그 위치를 기록한다 */
    static void setRefOffset(sdrMtx* aMtx,
                             smOID   aTableOID = SM_NULL_OID);

    static void* getTrans( sdrMtx *aMtx );
    static void* getMtxTrans( void *aMtx );

    static void* getStatSQL( void * aMtx ) { return (void*)((sdrMtx*)aMtx)->mStatSQL; }

    /* mtx의 로깅모드를 반환 */
    static sdrMtxLogMode getLogMode(sdrMtx*        aMtx);

    static sdrMtxLoggingType getLoggingType(sdrMtx*        aMtx);

    static UInt getDLogAttr(sdrMtx *aMtx);

    /* page 변경부분에 대한 로그만 기록한다. */
    static IDE_RC writeLogRec(sdrMtx*      aMtx,
                              UChar*       aWritePtr,
                              void*        aValue,
                              UInt         aLength,
                              sdrLogType   aLogType );

    static IDE_RC writeLogRec(sdrMtx*      aMtx,
                              scGRID       aWriteGRID,
                              void*        aValue,
                              UInt         aLength,
                              sdrLogType   aLogType );



    /* page에 대한 변경과 함께 SDR_*_BYTES 타입 로그를
       mtx 로그 버퍼에 기록한다. */
    static IDE_RC writeNBytes(void*           aMtx,
                              UChar*          aDest,
                              void*           aValue,
                              UInt            aLogType);

    /* 현재 mtx 로그 버퍼의  offset에서
       특정 길이의 value를 write */
    static IDE_RC write(sdrMtx*  aMtx,
                        void*    aValue,
                        UInt     aLength );

    /* latch item을 mtx 스택에 push */
    static IDE_RC push(void*            aMtx,
                       void*            aObject,
                       UInt             aLatchMode );

    static IDE_RC pushPage(void*            aMtx,
                           void*            aObject,
                           UInt             aLatchMode );

    /* BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합니다. 
     *
     * Free하려는 Extent들은 Mini-Transaction Commit 이후에 Cache에 추가되어야 
     * 합니다. Mini-Transaction이 Commit이전에 삽입되면, Commit이전에 재활용
     * 되어버립니다.*/
    static IDE_RC addPendingJob(void              * aMtx,
                                idBool              aIsCommitJob,
                                sdrMtxPendingFunc   aPendingFunc,
                                void              * aData );

    static IDE_RC releaseLatchToSP( sdrMtx       *aMtx,
                                    sdrSavePoint *aSP );

    static IDE_RC rollback( sdrMtx       *aMtx );

    static void setSavePoint( sdrMtx       *aMtx,
                              sdrSavePoint *aSP );

    /* PROJ-2162 RestartRiskReduction
     * X특정 스택에 특정 page ID에 해당하는 BCB가 존재할 경우,
     * 해당 page frame에 대한 포인터를 반환 */
    static UChar * getPagePtrFromPageID( sdrMtx     * aMtx,
                                         scSpaceID    aSpaceID,
                                         scPageID     aPageID );

    static sdrMtxLatchItem * existObject( sdrMtxStackInfo       * aLatchStack,
                                          void                  * aObject,
                                          sdrMtxItemSourceType    aType );

    static smLSN getEndLSN( void *aMtx );

    static idBool isLogWritten( void     *aMtx );
    static idBool isModeLogging( void     *aMtx );
    static idBool isModeNoLogging( void     *aMtx );

    static idBool isEmpty(sdrMtx *aMtx);

    static idBool isNologgingPersistent( void *aMtx );

    /* writeLogRec함수를 통해 SubLog의 크기를 설정해놓고 그것보다 작은량의
     * Log밖에 쓰지 못했으면, Mtx는 깨진 것이다. */
    static idBool isRemainLogRec( void *aMtx );

    // for debug
    static void dump( sdrMtx *aMtx,
                      sdrMtxDumpMode   aDumpMode = SDR_MTX_DUMP_NORMAL );

    static void dumpHex( sdrMtx *aMtx );

    static idBool validate( sdrMtx    *aMtx );


// Koo Wrapper
static IDE_RC unlock(void* aLatch, UInt, void*) { return (((iduLatch*)aLatch)->unlock());}

static IDE_RC dump(void*) { return IDE_SUCCESS; }


private:

    /* 초기화 */
    static IDE_RC initialize( idvSQL*       aStatistics,
                              sdrMtx*       aMtx,
                              void*         aTrans,
                              sdrMtxLogMode aLogMode,
                              idBool        aUndoable,
                              UInt          aDLogAttr );


    static IDE_RC destroy( sdrMtx *aMtx );

    /* 로그 header에 type과 RID를 설정한다.*/
    static IDE_RC initLogRec( sdrMtx*        aMtx,
                              scGRID         aWriteGRID,
                              UInt           aLength,
                              sdrLogType     aType);


    /* Tablespace에 Log Compression을 하지 않도록 설정된 경우
       로그 압축을 하지 않도록 설정
    */
    static IDE_RC checkTBSLogCompAttr( sdrMtx*      aMtx,
                                       scSpaceID    aSpaceID );

    static IDE_RC writeUndoLog( sdrMtx    *aMtx,
                                UChar     *aWritePtr,
                                UInt       aLength,
                                sdrLogType aType );


    /* 로그 기록을 위한 mtx 로그 버퍼 초기화 */
    static IDE_RC initLogBuffer(sdrMtx* aMtx);

    static IDE_RC destroyLogBuffer(sdrMtx* aMtx);

    /* BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합니다. 
     *
     * Free하려는 Extent들은 Mini-Transaction Commit 이후에 Cache에 추가되어야 
     * 합니다. Mini-Transaction이 Commit이전에 삽입되면, Commit이전에 재활용
     * 되어버립니다.*/
    static IDE_RC executePendingJob(void   * aMtx,
                                    idBool   aDoCommitJob );

    static IDE_RC destroyPendingJob( void * aMtx);

    /*  mtx 스택의 한 item의 release 작업을 수행한다. */
    static IDE_RC releaseLatchItem(sdrMtx*           aMtx,
                                   sdrMtxLatchItem*  aItem);

    static IDE_RC releaseLatchItem4Rollback(sdrMtx          *aMtx,
                                            sdrMtxLatchItem *aItem);

    /* PROJ-2162 RestartRiskReduction
     * OnlineDRDBRedo기능을 통해 Page를 복구할 필요가 있는지 검증한다. */
    static idBool needMtxRollback( sdrMtx * aMtx );

    /* PROJ-2162 RestartRiskReduction
     * Mtx Commit마다 DRDB Redo기능을 검증함 */
    static IDE_RC validateDRDBRedo( sdrMtx          * aMtx,
                                    sdrMtxStackInfo * aMtxStack,
                                    smuDynArrayBase * aLogBuffer );

    static IDE_RC validateDRDBRedoInternal( 
        idvSQL          * aStatistics,
        sdrMtx          * aMtx,
        sdrMtxStackInfo * aMtxStack,
        ULong           * aSPIDArray,
        UInt            * aSPIDCount,
        scSpaceID         aSpaceID,
        scPageID          aPageID );

private:
    // 각 mtx stack item에 type에 따라 적용할 함수와
    // item에 대한 정보를 가진다.
    static sdrMtxLatchItemApplyInfo
                  mItemVector[SDR_MTX_RELEASE_FUNC_NUM];

#if defined(DEBUG)
    /* PROJ-2162 RestartRiskReduction
     * Mtx Rollback 테스트를 위한 임시 seq.
     * __SM_MTX_ROLLBACK_TEST 값이 설정되면 (0이 아니면) 이 값이
     * MtxCommit시마다 증가한다. 그리고 위 Property값에 도달하면
     * 강제로 Rollback을 시도한다. */
    static UInt mMtxRollbackTestSeq;
#endif
};


#endif // _O_SDR_MINI_TRANS_H_
