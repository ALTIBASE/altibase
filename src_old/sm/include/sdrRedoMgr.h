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
 * $Id: sdrRedoMgr.h 89217 2020-11-11 12:31:50Z et16 $
 *
 * Description :
 *
 * 본 파일은 DRDB 복구과정에서의 재수행 관리자에 대한 헤더 파일이다.
 *
 * # 개념
 *
 * DRDB에 대한 restart recovery 과정중 redoAll 과정을 효율적으로
 * 처리하기 위한 자료구조이다.
 *
 * - redoAll 과정에서는 판독된 DRDB관련 redo 로그들을 파싱하여 적당한
 *   tablespace의 page에 반영해야 한다.
 *   (physiological 로깅)
 *
 * - 동일한 tablespace의 같은 page에 해당하는 redo 로그를
 *   모아서 한꺼번에 반영하는 것이 buffer 매니저의 I/O를 줄일 수 있는
 *   장점을 가지게 된다.
 *   (hash table 사용으로 가능)
 *
 * # 재수행 관리자 구조
 *
 *     재수행 관리자
 *     ________________       ______
 *     |_______________| -----|_____| 로그파싱버퍼
 *
 *
 *    restart recovery   1차 Hash
 *                       smuHash  sdrRedoRecvNode
 *                     ______|____  _________ __________ __________
 *                     |_________|--|(0,5)  |-| (2,10) |-| (3,25) |
 *                     |_________|  |_______| |________| |__*_____|
 *                     |_________|      |          |
 *                     |_________|      O          O sdrHashLogData
 *                     |_________|      O          O
 *                     |_________|                 O
 *                     |_________|--ㅁ-ㅁ
 *                     |_________|
 *                     |_________|--ㅁ-ㅁ-ㅁ-ㅁ
 *
 *    media recovery     2차 hash (spaceID, fileID)
 *                       smuHash  sdrRecvFileHashNode
 *                     ____|______  _________ _________
 *                     |_________|--| (1,0) |-| (2,0) |
 *                     |_________|  |from_to| |from_to|
 *                     |_________|
 *                     |_________|
 *                     |_________|
 *                     |_________|
 *                     |_________|--ㅁ-ㅁ
 *                     |_________|
 *                     |_________|--ㅁ-ㅁ-ㅁ-ㅁ
 *
 *  - 재수행 관리자
 *    DRDB에 대한 redoAll과정을 관리하는 자료구조
 *    redoAll 단계에 필요한 각종 정보 들을 포함한다.
 *
 *  - 1 차 Hash : 리두로그 저장
 *    restart recover 및 media recovery시 사용하는 hash이며, hashkey는 (space ID, page ID) 이다
 *
 *  - 2 차 Hash : 복구파일정보 저장
 *    media recovery시 사용하는 hash이며, hashkey는 (space ID, file ID) 이다.
 *
 *  - sdrRedoHashNode
 *    hash 버켓에 chain으로 연결된 자료구조이며,특정 (space ID, pageID)
 *    에 대한 redo log의 연결리스트를 포함한다.
 *
 *  - sdrRedoLogData
 *    redo 로그 정보와 실제 로그레코드를 포함한다.
 *
 *  - sdrRecvFileHashNode
 *    복구 대상의 데이타파일에 대한 복구정보를 가진다. 1차 해쉬의
 *    HashNode가 해당 노드의 포이터를 유지하여, Media recovery를 수행
 *    한다.
 **********************************************************************/

#ifndef _O_SDR_REDO_MGR_H_
#define _O_SDR_REDO_MGR_H_ 1

#include <smu.h>
#include <sdd.h>
#include <sdrDef.h>

class sdrRedoMgr
{
public:

    /* mutex, 해시테이블등을 초기화하는 함수 */
    static IDE_RC initialize(UInt            aHashTableSize,
                             smiRecoverType  aRecvType);

    /* 재수행 관리자의 해제 */
    static IDE_RC destroy();

    /* PROJ-2162 RestartRiskReduction
     * RedoLogBody를 순회하며 sdrRedoLogDataList를 만들어 반환함 */
    static IDE_RC generateRedoLogDataList( smTID             aTransID,
                                           SChar           * aLogBuffer,
                                           UInt              aLogBufferSize,
                                           smLSN           * aBeginLSN,
                                           smLSN           * aEndLSN,
                                           void           ** aLogDataList );

    /* RedoLogList를 Free함 */
    static IDE_RC freeRedoLogDataList( void            * aLogDataList );

    /* PROJ-2162 RestartRiskReduction
     * OnlineREDO를 통해 DRDB Page를 새로 생성함 */
    static IDE_RC generatePageUsingOnlineRedo( idvSQL     * aStatistics,
                                               scSpaceID    aSpaceID,
                                               scPageID     aPageID,
                                               idBool       aReadByDisk,
                                               UChar      * aOutPagePtr,
                                               idBool     * aSuccess );

    static IDE_RC addRedoLogToHashTable( void * aLogDataList );

    /* 해시 테이블에 저장된 redo log를
       적당한 페이지에 반영 */
    static IDE_RC applyHashedLogRec(idvSQL * aStatistics);

    /* PROJ-2162 RestartRiskReduction
     * List로 연결된 Log들을 모두 반영한다. */
    static IDE_RC applyListedLogRec( sdrMtx         * aMtx,
                                     scSpaceID        aTargetSID,
                                     scPageID         aTargetPID,
                                     UChar          * aPagePtr,
                                     smLSN            aLSN,
                                     sdrRedoLogData * aLogDataList );


    /* BUGBUG - 9640 FILE 연산에 대한 redo */
    static IDE_RC  redoFILEOPER(smTID        aTransID,
                                scSpaceID    aSpaceID,
                                UInt         aFileID,
                                UInt         aFOPType,
                                UInt         aAImgSize,
                                SChar*       aLogRec);

    /* BUGBUG - 7983 MRDB runtime memory에 대한 redo */
    static void redoRuntimeMRDB( void   * aTrans,
                                 SChar  * aLogRec );

    // Redo Log List상의 Redo Log Data를 모두 제거한다.
    static IDE_RC clearRedoLogList( sdrRedoHashNode*  aRedoHashNode );

    /* 복구할 datafile을 분석하여 hash에 삽입 */
    static IDE_RC addRecvFileToHash( sddDataFileHdr*   aDBFileHdr,
                                     SChar*            aFileName,
                                     smLSN*            aRedoFromLSN,
                                     smLSN*            aRedoToLSN );

    /* 복구 hash노드에 해당하는 redo 로그인지 검사 */
    static IDE_RC filterRecvRedoLog( sdrRecvFileHashNode* aHashNode,
                                     smLSN*               aBeginLSN,
                                     idBool*              aIsRedo );

    /* 복구 파일헤더를 갱신하고 Hash Node 제거 */
    static IDE_RC repairFailureDBFHdr( smLSN* aResetLogsLSN );

    /* 복구 파일의 Hash Node를 제거 */
    static IDE_RC removeAllRecvDBFHashNodes();

    static smiRecoverType getRecvType()
        { return mRecvType; }

    static void writeDebugInfo();

    /* PROJ-1923 private -> public 전환 */
    static idBool validateLogRec( sdrLogHdr * aLogHdr );
    /* BUG-48275 recovery 에서 잘못된 오류 메시지가 출력 됩니다. */
    static smLSN     getLastUpdatedPageLSN() { return mLstDRDBPageUpdateLSN; };
    static scSpaceID getLastUpdatedSpaceID() { return mLstUpdatedSpaceID; };
    static scPageID  getLastUpdatedPageID()  { return mLstUpdatedPageID; };

private:

    static IDE_RC lock() { return mMutex.lock( NULL ); }
    static IDE_RC unlock() { return mMutex.unlock(); }

    /* 실제로  mParseBuffer의 저장된 로그를 파싱한 후,
       해시 테이블에 저장 */
    static void parseRedoLogHdr( SChar       * aLogRec,
                                 sdrLogType  * aLogType,
                                 scGRID      * aLogGRID,
                                 UInt        * aValueLen );


    static IDE_RC applyLogRec( sdrMtx         * aMtx,
                               UChar          * aPagePtr,
                               sdrRedoLogData * aLogData );

    /* BUGBUG - 7983 runtime memory에 대한 redo */
    static void applyLogRecToRuntimeMRDB( void*      aTrans,
                                          sdrLogType aLogType,
                                          SChar*     aValue,
                                          UInt       aValueLen );

    /* 해쉬 함수 */
    static UInt genHashValueFunc( void* aRID );

    /* 비교 함수 */
    static SInt compareFunc(void* aLhs, void* aRhs);

private:

    /* hashed log를 페이지에 반영할 경우 처리된 버켓 개수를 변경하는 등의
       해시에 접근을 제어하기 위해 정의 */
    static iduMutex          mMutex;
    static smiRecoverType    mRecvType;

    /* 파싱된 redo log를 저장하는 해시 구조체 (1차 hash)*/
    static smuHashBase   mHash;

    /* 복구할 데이타파일노드를 저장하는 해시 (2차 hash) */
    static smuHashBase   mRecvFileHash;

    /* 해시 크기 */
    static UInt          mHashTableSize;

    static UInt          mApplyPageCount; /* 적용한 Page 개수 */

    /* 1차해시의 hash node중 적용할 필요없는 노드의 개수
       특정 임계치에 다다르면 모두 해제하게 된다 */
    static UInt          mNoApplyPageCnt;
    static UInt          mRecvFileCnt;    /* 복구할 데이타파일 hash node 개수 */

    /* for properties */
    static UInt          mMaxRedoHeapSize;    /* maximum heap memory */

    /* PROJ-2118 BUG Reporting - Debug Info for Fatal */
    static scSpaceID       mCurSpaceID;
    static scPageID        mCurPageID;
    static UChar         * mCurPagePtr;
    static sdrRedoLogData* mRedoLogPtr;
    static smLSN           mLstDRDBPageUpdateLSN;   // DRDB WAL Check에 사용됨
    static sddDataFileNode *mLstUpdatedDataFileNode;
    static scPageID         mLstUpdatedPageID;
    static scSpaceID        mLstUpdatedSpaceID;
};

#endif // _O_SDR_REDO_MGR_H_

