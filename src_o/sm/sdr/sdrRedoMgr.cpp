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
 * $Id: sdrRedoMgr.cpp 91197 2021-07-12 01:15:29Z emlee $
 *
 * Description :
 *
 * DRDB 복구과정에서의 재수행 관리자에 대한 구현파일이다.
 *
 **********************************************************************/

#include <sdb.h>
#include <smu.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smiDef.h>
#include <smx.h>
#include <sct.h>

#include <sdrDef.h>
#include <sdrRedoMgr.h>
#include <sdrMiniTrans.h>
#include <sdrUpdate.h>
#include <sdrCorruptPageMgr.h>
#include <sdrReq.h>
#include <sdsFile.h>
#include <sdsBufferArea.h>
#include <sdsMeta.h>
#include <sdsBufferMgr.h>


iduMutex       sdrRedoMgr::mMutex;
smuHashBase    sdrRedoMgr::mHash;
UInt           sdrRedoMgr::mHashTableSize;
UInt           sdrRedoMgr::mApplyPageCount;
smiRecoverType sdrRedoMgr::mRecvType;     /* restart, complete, incomplete */

/*  for media recovery */
smuHashBase    sdrRedoMgr::mRecvFileHash;
UInt           sdrRedoMgr::mNoApplyPageCnt;
UInt           sdrRedoMgr::mRecvFileCnt; /* 복구할 파일 개수 */

// for properties
UInt           sdrRedoMgr::mMaxRedoHeapSize;    /* maximum heap memory */

/* PROJ-2118 BUG Reporting - debug info */
scSpaceID       sdrRedoMgr::mCurSpaceID = SC_NULL_SPACEID;
scPageID        sdrRedoMgr::mCurPageID  = SM_SPECIAL_PID;
UChar*          sdrRedoMgr::mCurPagePtr = NULL;
sdrRedoLogData* sdrRedoMgr::mRedoLogPtr = NULL;

/* BUG-48275 recovery 에서 잘못된 오류 메시지가 출력 됩니다. */
smLSN           sdrRedoMgr::mLstDRDBPageUpdateLSN   = { 0, 0 };
scPageID        sdrRedoMgr::mLstUpdatedPageID = SD_NULL_PID;
scSpaceID       sdrRedoMgr::mLstUpdatedSpaceID = SC_NULL_SPACEID;

/***********************************************************************
 * Description : 재수행 관리자의 초기화 함수
 *
 * restart recovery의 redoall pass과정에서 호출된다. aMaxPageCount는
 * 버퍼관리자의 버퍼풀에 fix할 수 있는 최대 page 개수이며, 이는 redo 로그의
 * 해당 page에 대하여 fix할 수 있는 최대개수를 나타낸다. 이 값에 도달하면,
 * hash에 저장된 redo 로그들을 페이지에 반영한 후, 버퍼풀을 flush하고, hash를
 * 비우며, 그 후 계속해서 redo 로그를 다시 hash에 저장한다.
 *
 * + 2nd. code design
 *   - mutex를 초기화한다.
 *   - 해시 테이블를 초기화한다.
 **********************************************************************/
IDE_RC sdrRedoMgr::initialize(UInt             aHashTableSize,
                              smiRecoverType   aRecvType)
{

    IDE_DASSERT(aHashTableSize > 0);

    IDE_TEST_RAISE( mMutex.initialize((SChar*)"redo manager mutex",
                                      IDU_MUTEX_KIND_NATIVE,
                                      IDV_WAIT_INDEX_NULL)
                    != IDE_SUCCESS,
                    error_mutex_init );

    mHashTableSize = aHashTableSize;
    mRecvType      = aRecvType;

    // HashKey : <space, pageID, unused>
    IDE_TEST( smuHash::initialize(&mHash,
                                  1,
                                  mHashTableSize,    /* aBucketCount */
                                  ID_SIZEOF(scGRID),  /* aKeyLength */
                                  ID_FALSE,          /* aUseLatch */
                                  genHashValueFunc,
                                  compareFunc) != IDE_SUCCESS );

    if (getRecvType() != SMI_RECOVER_RESTART)
    {
        // HashKey : <space, fileID, unused>
        IDE_TEST( smuHash::initialize(&mRecvFileHash,
                                      1,
                                      mHashTableSize,   /* aBucketCount */
                                      ID_SIZEOF(scGRID), /* aKeyLength */
                                      ID_FALSE,         /* aUseLatch */
                                      genHashValueFunc,
                                      compareFunc) != IDE_SUCCESS );
    }

    mApplyPageCount  = 0;
    mNoApplyPageCnt  = 0;
    mRecvFileCnt     = 0;
    SM_SET_LSN( mLstDRDBPageUpdateLSN, 0, 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_mutex_init );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 재수행 관리자의 해제 함수
 *
 * restart recovery의 redoall pass과정에서 모든 디스크 redo log들을
 * DB 페이지에 반영한 후 다음 호출된다.
 *
 * + 2nd. code design
 *   - 해시 테이블을 해제한다.
 *   - mutex를 해제한다.
 **********************************************************************/
IDE_RC sdrRedoMgr::destroy()
{
    // BUG-28709 Media recovery시 disk에 media recovery하지 않으면
    //           mApplyPageCount가 0이 아니고,
    //           mApplyPageCount - mNoApplyPageCnt 가 0 이어야 함
    IDE_DASSERT( (mApplyPageCount - mNoApplyPageCnt) == 0 );

    if (getRecvType() != SMI_RECOVER_RESTART)
    {
        IDE_DASSERT( mRecvFileCnt == 0 );
        IDE_TEST( smuHash::destroy(&mRecvFileHash) != IDE_SUCCESS );
    }

    IDE_TEST( smuHash::destroy(&mHash) != IDE_SUCCESS );

    IDE_TEST_RAISE( mMutex.destroy() != IDE_SUCCESS, error_mutex_destroy );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_mutex_destroy );
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : BUGBUG - 7983 MRDB runtime memory에 대한 redo
 *
 * 로그관리자의 restart recovery의 재수행 단계에서 runtime memory에 대한
 * redo가 필요한 drdb 타입의 로그를 판독 호출된다.
 **********************************************************************/
void sdrRedoMgr::redoRuntimeMRDB( void   * aTrans,
                                  SChar  * aLogRec )
{

    scGRID      sGRID;
    sdrLogType  sLogType;
    UInt        sValueLen;
    SChar*      sValue;

    IDE_DASSERT( aTrans  != NULL );
    IDE_DASSERT( aLogRec != NULL );

    parseRedoLogHdr( aLogRec,
                     &sLogType,
                     &sGRID,
                     &sValueLen );

    if (sValueLen != 0)
    {
        sValue  = aLogRec + ID_SIZEOF(sdrLogHdr);
    }
    else
    {
        sValue = NULL;
    }

    applyLogRecToRuntimeMRDB(aTrans, sLogType, sValue, sValueLen);
}

/***********************************************************************
 * Description : BUGBUG - 7983 runtime memory에 대한 redo
 *
 * redoRuntimeMRDB, applyHashedLogRec 함수에서 호출된다.
 **********************************************************************/
void sdrRedoMgr::applyLogRecToRuntimeMRDB( void*      aTrans,
                                           sdrLogType aLogType,
                                           SChar*     aValue,
                                           UInt       aValueLen )
{
    sdRID       sExtRID4TSS;
    sdSID       sTSSlotSID;
    SChar     * sValuePtr;
    smxTrans  * sTrans;
    UInt        sTXSegID;

    IDE_DASSERT( aTrans != NULL );

    sTrans    = (smxTrans*)aTrans;
    sValuePtr = aValue;

    switch(aLogType)
    {
        case SDR_SDC_BIND_TSS :

            IDE_ASSERT( aValue != NULL );
            IDE_ASSERT( (ID_SIZEOF(sdSID) +
                         ID_SIZEOF(sdRID) +
                         ID_SIZEOF(smTID) +
                        ID_SIZEOF(UInt) ) == aValueLen );

            idlOS::memcpy(&sTSSlotSID, sValuePtr, ID_SIZEOF(sdSID));
            sValuePtr += ID_SIZEOF(sdSID);

            idlOS::memcpy(&sExtRID4TSS, sValuePtr, ID_SIZEOF(sdRID));
            sValuePtr += ID_SIZEOF(sdRID);
            sValuePtr += ID_SIZEOF(smTID);

            idlOS::memcpy(&sTXSegID, sValuePtr, ID_SIZEOF(UInt));

            IDE_ASSERT( sTrans->mTXSegEntry == NULL );
            sdcTXSegMgr::tryAllocEntryByIdx( sTXSegID, &(sTrans->mTXSegEntry) );
            IDE_ASSERT( sTrans->mTXSegEntry != NULL );

            sTrans->setTSSAllocPos( sExtRID4TSS, sTSSlotSID );
            break;

        default:
            break;
    }

    return;

}

 /***********************************************************************
 * Description : LogBuffer에서 DRDB용 RedoLog를 분리해서 List로 구성함
 *
 * aTransID       - [IN]  Log를 만든 TID
 * aLogBuffer     - [IN]  원본 Log
 * aLogBufferSize - [IN]  원본 Log가 들어있는 Buffer의 크기
 * aBeginLSN      - [IN]  원본 Log의 LSN
 * aEndLSN        - [IN]  원본 Log의 LSN의 끝위치
 * aLogDataList   - [OUT] Parsing된 결과 ( sdrRedoLogData )
 ***********************************************************************/
IDE_RC sdrRedoMgr::generateRedoLogDataList( smTID             aTransID,
                                            SChar           * aLogBuffer,
                                            UInt              aLogBufferSize,
                                            smLSN           * aBeginLSN,
                                            smLSN           * aEndLSN,
                                            void           ** aLogDataList )
{
    scGRID              sGRID;
    sdrLogType          sLogType;
    SChar             * sLogRec;
    SChar             * sValue;
    UInt                sParseLen   = 0;
    UInt                sValueLen;
    sdrRedoLogData    * sData   = NULL;
    sdrRedoLogData    * sHead   = NULL;
    UInt                sState  = 0;

    IDE_DASSERT( aLogBuffer != NULL );
    IDE_DASSERT( aLogBufferSize > 0 );

    (*aLogDataList) = NULL;

    while (sParseLen < aLogBufferSize)
    {
        sLogRec = aLogBuffer + sParseLen;

        parseRedoLogHdr( sLogRec,
                         &sLogType,
                         &sGRID,
                         &sValueLen );

        if (sValueLen != 0)
        {
            sValue  = sLogRec + ID_SIZEOF(sdrLogHdr);
        }
        else
        {
            sValue = NULL;
        }

        // Redo Skip할 TableSpace 인지 체크한다.
        // 1) Restart Recovery
        // DROPPED, DISCARDED, OFFLINE
        // 2) Media Recovery 의 경우
        // DROPPED와 DISCARDED

        /* TableSpace에 대한 체크이기 때문에, RestartRecovery가 아니라도 함*/
        if ( smLayerCallback::isSkipRedo( SC_MAKE_SPACE(sGRID), ID_FALSE )
             == ID_FALSE )
        {
            /* TC/FIT/Limit/sm/sdr/sdrRedoMgr_addRedoLogToHashTable_malloc2.sql */
            IDU_FIT_POINT_RAISE( "sdrRedoMgr::addRedoLogToHashTable::malloc2",
                                  insufficient_memory );
			
            IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SDR,
                                              ID_SIZEOF(sdrRedoLogData),
                                              (void**)&sData) != IDE_SUCCESS,
                            insufficient_memory );
            sState = 1;

            sData->mType        = sLogType;
            sData->mSpaceID     = SC_MAKE_SPACE(sGRID);
            sData->mPageID      = SC_MAKE_PID(sGRID);
            sData->mValue       = sValue;
            sData->mValueLength = sValueLen;
            sData->mTransID     = aTransID;
            sData->mBeginLSN    = *aBeginLSN;
            sData->mEndLSN      = *aEndLSN;

            if ( SC_GRID_IS_WITH_SLOTNUM(sGRID) )
            {
                sData->mOffset      = SC_NULL_OFFSET;
                sData->mSlotNum     = SC_MAKE_SLOTNUM(sGRID);
            }
            else
            {
                sData->mOffset      = SC_MAKE_OFFSET(sGRID);
                sData->mSlotNum     = SC_NULL_SLOTNUM;
            }

            SMU_LIST_INIT_BASE( &sData->mNode4RedoLog );
            sData->mNode4RedoLog.mData = (void*)sData;

            if ( sHead == NULL )
            {
                sHead           = sData;
                (*aLogDataList) = (void*)sData;
            }
            else
            {
                SMU_LIST_GET_PREV(&sData->mNode4RedoLog) = NULL;
                SMU_LIST_GET_NEXT(&sData->mNode4RedoLog) = NULL;
                SMU_LIST_ADD_LAST(&sHead->mNode4RedoLog, &sData->mNode4RedoLog);
            }
        }
        else
        {
            /*do nothing*/
        }

        sParseLen += ID_SIZEOF(sdrLogHdr) + sValueLen;
    }

    /* Mtx가 잘못 기록되면 이 길이가 달라진다. 따라서 잘못된 로그인데,
     * 그렇다고 Assert로 처리하면 구동 방법이 없다. */
    IDE_ERROR_MSG( sParseLen == aLogBufferSize,
                   "sParseLen      : %"ID_UINT32_FMT"\n"
                   "aLogBufferSize : %"ID_UINT32_FMT"\n"
                   "mFileNo        : %"ID_UINT32_FMT"\n"
                   "mOffset        : %"ID_UINT32_FMT"\n",
                   sParseLen,
                   aLogBufferSize,
                   aBeginLSN->mFileNo,
                   aBeginLSN->mOffset );  

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sData ) == IDE_SUCCESS );
            sData = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-2162 RestartRiskReduction
 * Description : RedoLogList를 Free함
 *
 * aLogDataList - [IN/OUT] Free할 대상
 ***********************************************************************/
IDE_RC sdrRedoMgr::freeRedoLogDataList( void  * aLogDataList )
{
    sdrRedoLogData   * sData;
    sdrRedoLogData   * sNext;

    sData = (sdrRedoLogData*) aLogDataList;
    while( sData != NULL )
    {
        sNext = (sdrRedoLogData*)SMU_LIST_GET_NEXT( &(sData->mNode4RedoLog) )->mData;
        if ( sNext == sData )
        {
            sNext = NULL;
        }
        SMU_LIST_DELETE( &sData->mNode4RedoLog );
        IDE_TEST( iduMemMgr::free( sData ) != IDE_SUCCESS );
        sData = sNext;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그에서 drdb 로그를 추출하여 파싱하고 Hasing함
 *
 * PROJ-2162 RestartRiskReduction
 * OnlineREDO를 통해 DRDB Page를 새로 생성함 
 *
 * aStatistics       - [IN]     dummy 통계정보
 * aSpaceID          - [IN]     Redo할 Page의 TablespaceID
 * aPageID           - [IN]     Redo할 Page의 PageID
 * aReadFromDisk     - [IN]     Disk의 File로부터 Page를 읽어서 올릴 것인가?
 * aOnlineTBSLSN4Idx - [OUT]    TBS Online/Offline시 Index SMO 값 설정에 쓰임
 * aOutPagePtr       - [IN/OUT] aReadFromDisk가 FALSE일 경우 페이지가 읽혀서 
 *                              와야하고, 그게 아니더라도 Align된 Page라도 
 *                              와야 한다.
 * aSuccess          - [OUT]    성공여부
 ***********************************************************************/
IDE_RC sdrRedoMgr::generatePageUsingOnlineRedo( idvSQL    * aStatistics,
                                                scSpaceID   aSpaceID,
                                                scPageID    aPageID,
                                                idBool      aReadFromDisk,
                                                UChar     * aOutPagePtr,
                                                idBool    * aSuccess )
{
    smLSN                    sCurLSN;
    smLSN                    sLstLSN;
    smLSN                    sLastModifyLSN;
    smrLogFile             * sLogFilePtr = NULL;
    smrLogHead               sLogHead;
    SChar                  * sLogPtr;
    idBool                   sIsValid;
    UInt                     sLogSizeAtDisk = 0;
    smLSN                    sBeginLSN;
    smLSN                    sEndLSN;
    sdrRedoLogData         * sRedoLogDataList;
    iduReusedMemoryHandle  * sDecompBufferHandle;
    SChar                  * sRedoLogBuffer;
    UInt                     sRedoLogSize;
    sdrMtx                   sMtx;
    idBool                   sSuccess = ID_FALSE;
    UInt                     sState = 0;  
    /* PROJ-2102 */
    smLSN                    sCurSecondaryLSN;

    IDE_ERROR( aOutPagePtr != NULL );
    IDE_ERROR( aSuccess    != NULL );

    if ( aReadFromDisk == ID_TRUE )
    {
        IDE_TEST( sddDiskMgr::read( aStatistics,
                                    aSpaceID,
                                    aPageID,
                                    aOutPagePtr )
                  != IDE_SUCCESS );

        /* Page가 초기화되지 않거나 Inconsistent하다면 검증하지 않는다. */
        IDE_TEST_CONT( ( sdpPhyPage::checkAndSetPageCorrupted( aSpaceID, aOutPagePtr )
                          == ID_TRUE ),
                        SKIP );
    }

    /* sdrRedoMgr_generatePageUsingOnlineRedo_malloc_DecompBufferHandle.tc */
    IDU_FIT_POINT("sdrRedoMgr::generatePageUsingOnlineRedo::malloc::DecompBufferHandle");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SDR,
                                 ID_SIZEOF( iduGrowingMemoryHandle ),
                                 (void**)&sDecompBufferHandle,
                                 IDU_MEM_FORCE)
             != IDE_SUCCESS );
    sState = 1;

    /* 로그 압축해제용 버퍼 핸들의 초기화
     * Hash하여 보관할 것이 아니기 때문에, Reuse로 바로바로 사용함 */
    IDE_TEST( sDecompBufferHandle->initialize( IDU_MEM_SM_SDR )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sdrMiniTrans::begin( aStatistics, /* idvSQL* */
                                   &sMtx,
                                   (void*)NULL,
                                   SDR_MTX_NOLOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 3;

    /* BUG-42785
     * MinRecoveryLSN을 획득한 후에 Checkpoint가 수행될 경우
     * MinRecoveryLSN이 변경되며 LogFile이 지워질수 있다.
     * 현재 OnlineDRDBRedo를 수행 중임을 알려
     * 로그파일이 지워지지 않도록 하여야 한다. */
    IDE_TEST( smrRecoveryMgr::incOnlineDRDBRedoCnt() != IDE_SUCCESS );
    sState = 4;

    /* MinRecoveryLSN과 Page의 LastModifyLSN 중 큰 값을 선택한다.
     * - MinRecoveryLSN < PageLSN : 그냥 PageLSN보다 큰 Log만 반영한다.
     * - MinRecoveryLSN > PageLSN :
     *   이 PageLSN은 '자신을 갱신시킨 Log의 다음 LSN'이다.
     *   즉 이 page에 대한 다음 갱신 Log를 가리키는 것이 아니기 때문에,
     *   MinRecoveryLSN을 참고하여 반영한다.
     * 여기서 PageRecoveryLSN을 선택하면 안된다. Flush하기 위해
     * IOB로 옮긴 순간 여기서 Page를 Read해버리는 문제가 발생할 수
     * 있기 때문이다. */
    sdbBufferMgr::getMinRecoveryLSN( aStatistics,
                                     &sCurLSN );
    /* MinRecoveryLSN은 PBuffer와 Secondary Buffer를 비교해 더 작은것으로 선택 */
    sdsBufferMgr::getMinRecoveryLSN( aStatistics,
                                     &sCurSecondaryLSN );
    if ( smrCompareLSN::isGT( &sCurLSN, &sCurSecondaryLSN ) == ID_TRUE )
    {
        SM_GET_LSN( sCurLSN, sCurSecondaryLSN );
    }

    sLastModifyLSN = smLayerCallback::getPageLSN( (UChar*)aOutPagePtr );
    if ( smrCompareLSN::isLT( &sCurLSN, &sLastModifyLSN ) == ID_TRUE )
    {
        SM_GET_LSN( sCurLSN, sLastModifyLSN );
    }

    smrLogMgr::getLstLSN( &sLstLSN );

    IDU_FIT_POINT( "3.BUG-42785@sdrRedoMgr::generatePageUsingOnlineRedo::wakeup" );
    IDU_FIT_POINT( "4.BUG-42785@sdrRedoMgr::generatePageUsingOnlineRedo::sleep" );

    while( smrCompareLSN::isLT( &sCurLSN , &sLstLSN ) == ID_TRUE )
    {
        sCurLSN.mOffset += sLogSizeAtDisk;
        IDE_TEST( smrLogMgr::readLog( sDecompBufferHandle,
                                      &sCurLSN,
                                      ID_FALSE, /* Close Log File  */
                                      &sLogFilePtr,
                                      &sLogHead,
                                      &sLogPtr,
                                      &sIsValid,         /* Is Valid */
                                      &sLogSizeAtDisk )   /* Log Size */
                 != IDE_SUCCESS );

        if ( sIsValid == ID_FALSE )
        {
            /* Done. */
            break;
        }

        IDE_ASSERT( smrCompareLSN::isLTE(&sLastModifyLSN,
                                         &sCurLSN) == ID_TRUE );

        if ( smrLogMgr::isDiskLogType( sLogHead.mType ) == ID_TRUE )
        {
            SM_GET_LSN(sBeginLSN, sCurLSN);
            SM_GET_LSN(sEndLSN, sCurLSN);
            sEndLSN.mOffset += sLogSizeAtDisk;

            smrRecoveryMgr::getDRDBRedoBuffer( smrLogHeadI::getType(&sLogHead),
                                               smrLogHeadI::getSize(&sLogHead),
                                               sLogPtr,
                                               &sRedoLogSize,
                                               &sRedoLogBuffer );

            /* 할거 없음 */
            if ( sRedoLogSize == 0 )
            {
                continue;
            }
            IDE_TEST( generateRedoLogDataList( smrLogHeadI::getTransID(&sLogHead),
                                               sRedoLogBuffer,
                                               sRedoLogSize,
                                               &sBeginLSN,
                                               &sEndLSN,
                                               (void**)&sRedoLogDataList )
                      != IDE_SUCCESS );

            /* Redo해야할 Tablespace가 Drop되는등 하여
             * 의미없는 Log일 수 있음. 왜냐하면 이 Page
             * 와 관련된 로그만이 아닌, 모든 로그를
             * 읽어오기 때문. */
            if ( sRedoLogDataList != NULL )
            {
                IDE_TEST( applyListedLogRec( &sMtx,
                                             aSpaceID,
                                             aPageID,
                                             aOutPagePtr,
                                             sEndLSN,
                                             sRedoLogDataList )
                          != IDE_SUCCESS );

                IDE_TEST( freeRedoLogDataList( (void*)sRedoLogDataList ) 
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* LogFile EndLog일 경우,
             * 일반적인 Recovery와 달리 LogFile이 확실히 존재하는 경우에
             * 불리우는 것이기 때문에, 다음 Logfile을 열어서 확인하는
             * smrRecoveryMgr::redo_FILE_END 같은 작업이 필요 없다. */
            if ( sLogHead.mType == SMR_LT_FILE_END )
            {
                sCurLSN.mFileNo++;
                sCurLSN.mOffset = 0;
                sLogSizeAtDisk  = 0;
            }
        }
    }

    /* BUG-42785 OnlineDRDBRedo가 완료되었으므로
     * Count를 줄여준다. */
    sState = 3;
    IDE_TEST( smrRecoveryMgr::decOnlineDRDBRedoCnt() != IDE_SUCCESS );

    sSuccess = ID_TRUE;

    sState = 2;
    IDE_TEST( sdrMiniTrans::commit(&sMtx ) != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sDecompBufferHandle->destroy() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sDecompBufferHandle ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP );

    (*aSuccess) = sSuccess;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0, 
                 "SpaceID  :%"ID_UINT32_FMT"\n"
                 "PageID   :%"ID_UINT32_FMT"\n",
                 aSpaceID,
                 aPageID );

    switch( sState )
    {
        case 4:
            IDE_ASSERT( smrRecoveryMgr::decOnlineDRDBRedoCnt() == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( sDecompBufferHandle->destroy() == IDE_SUCCESS );
        case 1:
            (void) iduMemMgr::free( sDecompBufferHandle );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC sdrRedoMgr::applyListedLogRec( sdrMtx         * aMtx,
                                      scSpaceID        aSpaceID,
                                      scPageID         aPageID,
                                      UChar          * aPagePtr,
                                      smLSN            aLSN,
                                      sdrRedoLogData * aLogDataList )
{
    sdrRedoLogData * sData;
    sdpPhyPageHdr  * sPhyPageHdr;

    sPhyPageHdr = (sdpPhyPageHdr*)aPagePtr;
    sData       = aLogDataList;
    do // List를 순회함.
    {
        // 이 page에 대한 Log만 찾아서 적용함.
        if ( ( sData->mSpaceID == aSpaceID ) &&
             ( sData->mPageID  == aPageID )  &&
            /* 갱신없이 Logging만 하는 것이기 때문에, OnlineRedo에서는
             * 반응하면 안됨 */
             ( sData->mType != SDR_SDC_SET_INITSCN_TO_TSS ) )
        {
            IDE_TEST( applyLogRec( aMtx,
                                   aPagePtr,
                                   sData )
                      != IDE_SUCCESS );

            SM_GET_LSN( sPhyPageHdr->mFrameHdr.mPageLSN, aLSN );
        }

        sData = (sdrRedoLogData*)SMU_LIST_GET_NEXT( &(sData->mNode4RedoLog) )->mData;
    }
    while( sData != aLogDataList );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그에서 drdb 로그를 추출하여 파싱하고 Hasing함
 *
 * smrLogMgr에서 호출되거나, redo 로그를 hash에 저장하는 과정에서
 * 호출되기도 한다.
 * hash에서 bucket을 차례대로 판독하면서 bucket에 있는 모든 redo 로그들을
 * 해당 tablespace의 page에 반영한다. 이때, mtx를 이용하여 no-logging모드로
 * 처리한다. 적용후 버퍼관리자의 flush list의 모든 BCB를 flush 시킨다.
 *
 * + 2nd. code design
 *   - hash를 traverse 하기 위해 open한다.
 *   - hash로부터 첫번째 노드를 잘라온다.
 *   - while( 노드가 NULL이 아닐때까지 )
 *     {
 *        - 노드 상태를 recovery 시작상태로 설정한다.
 *        - mtx를 begin한다.
 *        - 노드의 space ID, page ID를 이용하여 버퍼관리자로부터 페이지를
 *          얻는다. (X-LATCH)
 *        - for ( redo 로그 데이타 개수만큼 )
 *          {
 *              노드가 가진 redo 로그데이타  리스트를 하나씩 페이지에 반영한다.
 *              반영된 redo 로그데이타 를 메모리 해제한다.
 *              노드의 redo 로그 데이타 개수를 감소시킨다.
 *          }
 *        - mtx를 commit한다.
 *        - hash로부터 다음 노드를 잘라온다.
 *        - 이미 반영된 노드를 메모리해제 한다.
 *        - hash의 적용할 노드 개수를 감소시킨다.
 *     }
 *     - hash를 close한다.
 *     - 적용할 노드 개수가 0인지 확인한다.
 *     - 버퍼관리자의 flush를 요청한다.
 ***********************************************************************/
IDE_RC sdrRedoMgr::applyHashedLogRec(idvSQL * aStatistics)
{

    idBool                      sTry;
    sdrMtx                      sMtx;
    UChar*                      sPagePtr;
    smuList*                    sBase;
    smuList*                    sList;
    smuList*                    sNextList;
    sdrRedoHashNode*            sNode;
    sdrRedoLogData*             sData;
    smLSN                       sLastModifyLSN;
    smLSN                       sMaxApplyLSN;
    smLSN                       sInitLSN;
    UInt                        sState = 0;
    idBool                      sApplied;
    idBool                      sSkipRedo;
    idBool                      sIsOverWriteLog;
    idBool                      sIsCorruptPage;
    UInt                        sHashState;
    smiTableSpaceAttr           sTBSAttr;
    UInt                        sIBChunkID;
    sddTableSpaceNode         * sSpaceNode;
    sddDataFileNode           * sDataFileNode = NULL;
    smiDataFileDescSlotID     * sDataFileDescSlotID;
    smiDataFileDescSlotID     * sAllocDataFileDescSlotID;
    sdFileID                    sDataFileID;
    idBool                      sResult; 

    sPagePtr = NULL;
    sNode    = NULL;
    sData    = NULL;
    sHashState = 0;

    SM_LSN_INIT(sInitLSN);
    SM_LSN_INIT(sLastModifyLSN);

    // Redo Log가 저장된 Hash Table Open
    IDE_TEST( smuHash::open(&mHash) != IDE_SUCCESS );
    sHashState = 1;

    IDE_TEST( smuHash::cutNode(&mHash, (void **)&sNode) != IDE_SUCCESS );

    sdrCorruptPageMgr::allowReadCorruptPage();

    while(sNode != NULL)
    {
        SM_LSN_INIT(sMaxApplyLSN);
        sApplied = ID_FALSE;

        if (sNode->mState == SDR_RECV_INVALID)
        {
            IDE_TEST( iduMemMgr::free( sNode ) != IDE_SUCCESS );
            mApplyPageCount--;

            if (getRecvType() != SMI_RECOVER_RESTART)
            {
                mNoApplyPageCnt--;
            }

            IDE_TEST( smuHash::cutNode( &mHash, (void **)&sNode ) != IDE_SUCCESS );

            continue;
        }

        IDE_DASSERT(sNode->mState == SDR_RECV_NOT_START);

        sNode->mState = SDR_RECV_START;

        if (sNode->mRecvItem != NULL)
        {
            sNode->mRecvItem->mState = SDR_RECV_START;
        }

        IDE_TEST( sdrMiniTrans::begin(aStatistics, /* idvSQL* */
                                      &sMtx,
                                      (void*)NULL,
                                      SDR_MTX_NOLOGGING,
                                      ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                      SM_DLOG_ATTR_DEFAULT)
                  != IDE_SUCCESS );
        sState = 1;

        //---------------------
        // Redo Page 를 구함
        //---------------------
        sSkipRedo = ID_FALSE;
        sIsOverWriteLog = ID_FALSE;
        sIsCorruptPage = ID_FALSE;

        if ( sdbBufferMgr::getPageByPID(aStatistics,
                                        sNode->mSpaceID,
                                        sNode->mPageID,
                                        SDB_X_LATCH,
                                        SDB_WAIT_NORMAL,
                                        SDB_SINGLE_PAGE_READ,
                                        &sMtx,
                                        &sPagePtr,
                                        &sTry,
                                        &sIsCorruptPage ) != IDE_SUCCESS )
        {
            switch( ideGetErrorCode() )
            {
                case smERR_ABORT_PageCorrupted :

                    IDE_TEST( sdrCorruptPageMgr::addCorruptPage( sNode->mSpaceID,
                                                                 sNode->mPageID )
                              != IDE_SUCCESS );

                case smERR_ABORT_NotFoundTableSpaceNode :
                case smERR_ABORT_NotFoundDataFile :
                    // To fix BUG-14949
                    // 테이블 스페이스는 존재하지만 데이터파일이
                    // 없는 경우가 있기 때문에 아래와 같은 두가지
                    // 조건으로 검사해야만 한다.
                    sSkipRedo = ID_TRUE;

                    ideClearError();

                    ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                                SM_TRC_DRECOVER_WARNNING1,
                                sNode->mSpaceID);

                    break;
                default :
                    IDE_ASSERT(ID_FALSE); // 다른에러는 용납되지 않는다.
                    break;
            }
        }
        else
        {
            /* PROJ-2118 Bug Reporting
             * Fatal시 기록하기 위해 Redo 대상 Page를 설정 */
            mCurSpaceID = sNode->mSpaceID;
            mCurPageID  = sNode->mPageID;
            mCurPagePtr = sPagePtr;
        }

        if ( sIsCorruptPage == ID_TRUE )
        {
            // PROJ-1867
            // Corrupted page를 읽은 경우이다.
            //
            // Page에 적용해야 할 첫 Redo Log가 Page Image
            // Log가 아니라면 Corrupted Page Hash에 등록한다.
            //
            // 반대로 Page에 적용해야 할 첫 Redo Log가 Page
            // Img Log이면 Corrupted Page Hash 에 있다면 제거하고
            // Redo Log를 적용해서 Page Image Log로 덮어쓴다.
            //
            // log hash과정에서 PILog를 만나면 해당 Page의
            // 이전 Redo Log를 hash에서 모두 지우기 때문에
            // Page의 첫 Redo Log만 확인하면 된다.

            sList = SMU_LIST_GET_FIRST( &sNode->mRedoLogList );
            sData = (sdrRedoLogData*)sList->mData;

            if ( sdrCorruptPageMgr::isOverwriteLog( sData->mType )
                 == ID_TRUE )
            {
                // Hash상의 log 중 Overwrite log가 있다.
                // sIsOverWriteLog를 셋팅하고 만약
                // Corrupt Page Hash에 있다면 제거한다.

                sctTableSpaceMgr::getTBSAttrByID( aStatistics,
                                                  sNode->mSpaceID,
                                                  &sTBSAttr );
                ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                             SM_TRC_DRECOVER_OVERWRITE_CORRUPTED_PAGE,
                             sTBSAttr.mName,
                             sNode->mPageID,
                             sData->mType );

                sIsOverWriteLog = ID_TRUE;

                IDE_TEST( sdrCorruptPageMgr::delCorruptPage( sNode->mSpaceID,
                                                             sNode->mPageID )
                          != IDE_SUCCESS );
            }
            else
            {
                // Hash상의 log 중 Overwrite log가 없다.
                // Corrupt Page Hash에 추가한다.

                IDE_TEST( sdrCorruptPageMgr::addCorruptPage( sNode->mSpaceID,
                                                             sNode->mPageID )
                          != IDE_SUCCESS );

                sSkipRedo = ID_TRUE;
            }
        }

        if ( sSkipRedo == ID_TRUE )
        {
            IDE_TEST( clearRedoLogList( sNode ) != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( sdrMiniTrans::rollback(&sMtx) != IDE_SUCCESS );

            sNode->mState = SDR_RECV_FINISHED;

            IDE_TEST( iduMemMgr::free( sNode ) != IDE_SUCCESS );
            mApplyPageCount--;

            IDE_TEST( smuHash::cutNode( &mHash, (void **)&sNode )
                      != IDE_SUCCESS );

            continue;
        }
        else
        {
            // redo 수행
        }

        //---------------------
        // Redo 수행
        //---------------------

        sLastModifyLSN = smLayerCallback::getPageLSN( sPagePtr );
        sBase = &sNode->mRedoLogList;

        for ( sList = SMU_LIST_GET_FIRST(sBase);
              sList != sBase;
              sList = sNextList)
        {
            sData = (sdrRedoLogData*)sList->mData;
            mRedoLogPtr = sData;

            /* ------------------------------------------------
             * Redo : PAGE의 update LSN이 로그의 LSN보다 작을 경우
             * ----------------------------------------------*/

            if ( ( smLayerCallback::isLSNLT( &sLastModifyLSN,
                                             &sData->mEndLSN ) == ID_TRUE ) ||
                 ( smLayerCallback::isLSNEQ( &sLastModifyLSN,
                                             &sInitLSN ) == ID_TRUE ) ||
                 ( sIsOverWriteLog == ID_TRUE ) )
            {
                //------------------------------------------------
                //  PAGE의 update LSN이 로그의 LSN보다 작을 경우,
                //  Redo 수행
                //------------------------------------------------

                sApplied = ID_TRUE;

                IDE_TEST( applyLogRec( &sMtx,
                                       sPagePtr,
                                       sData ) != IDE_SUCCESS );
            }
            else
            {
                // PAGE의 update LSN이 로그의 LSN보다 크거나 같을 경우,
                // 이미 로그의 내용이 반영되어있음을 의미

                /* PROJ-2162 PageLSN을 바탕으로 최신 UpdateLSN 갱신
                 * BUG-48275 recovery 에서 잘못된 오류 메시지가 출력 됩니다.
                 * 추가 정보 기록 */
                 if ( smrCompareLSN::isLT( &mLstDRDBPageUpdateLSN,
                                           &sLastModifyLSN ) == ID_TRUE )
                {
                    SM_GET_LSN( mLstDRDBPageUpdateLSN, sLastModifyLSN );
                    mLstUpdatedSpaceID = mCurSpaceID;
                    mLstUpdatedPageID  = mCurPageID;
                }
            }


            /* ------------------------------------------------
             * redo 수행(nologging 모드)시 page에 반영할 sMaxApplyLSN을
             * mtx에 설정한다.(force)
             * ----------------------------------------------*/
            SM_SET_LSN(sMaxApplyLSN,
                       sData->mEndLSN.mFileNo,
                       sData->mEndLSN.mOffset);

            sNextList = SMU_LIST_GET_NEXT(sList);

            SMU_LIST_DELETE(sList);

            mRedoLogPtr = NULL;
            IDE_TEST( iduMemMgr::free(sData) != IDE_SUCCESS );
            sNode->mRedoLogCount--;

            if (sNode->mRecvItem != NULL)
            {
                IDE_DASSERT(getRecvType() != SMI_RECOVER_RESTART);

                sNode->mRecvItem->mApplyRedoLogCnt--;
            }
        }

        //PROJ-2133 incremental backup
        if ( ( smrRecoveryMgr::isCTMgrEnabled() == ID_TRUE ) && 
             ( getRecvType() == SMI_RECOVER_RESTART ) )
        {
            IDE_TEST_CONT( sctTableSpaceMgr::isTempTableSpace(sNode->mSpaceID) == ID_TRUE,
                           skip_change_tracking );

            IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( 
                                                    sNode->mSpaceID, 
                                                    (void**)&sSpaceNode )
                      != IDE_SUCCESS );

            sDataFileID   = SD_MAKE_FID(sNode->mPageID);
            sDataFileNode = sSpaceNode->mFileNodeArr[ sDataFileID ];

            IDE_ERROR( sDataFileNode != NULL );

            sDataFileDescSlotID = 
                            &sDataFileNode->mDBFileHdr.mDataFileDescSlotID;
        
            IDE_TEST_CONT( smriChangeTrackingMgr::isAllocDataFileDescSlotID( 
                                    sDataFileDescSlotID ) != ID_TRUE,
                            skip_change_tracking );

            IDE_TEST( smriChangeTrackingMgr::isNeedAllocDataFileDescSlot( 
                                                    sDataFileDescSlotID,
                                                    sNode->mSpaceID,
                                                    sDataFileID,
                                                    &sResult )
                  != IDE_SUCCESS );

            if ( sResult == ID_TRUE )
            {
                IDE_TEST( smriChangeTrackingMgr::addDataFile2CTFile( 
                                                  sNode->mSpaceID,
                                                  sDataFileID,
                                                  SMRI_CT_DISK_TBS,
                                                  &sAllocDataFileDescSlotID )
                          != IDE_SUCCESS );

                sddDataFile::setDBFHdr(
                            &(sDataFileNode->mDBFileHdr),
                            NULL, // DiskRedoLSN
                            NULL, // DiskCreateLSN
                            NULL, // MustRedoLSN
                            sAllocDataFileDescSlotID);

                IDE_TEST( smrRecoveryMgr::updateDBFNodeToAnchor( 
                                                        sDataFileNode )
                          != IDE_SUCCESS );
            }
            
            sIBChunkID = smriChangeTrackingMgr::calcIBChunkID4DiskPage(
                                                            sNode->mPageID );

            IDE_TEST( smriChangeTrackingMgr::changeTracking( 
                                                 sDataFileNode,
                                                 NULL, //smmDatabaseFile
                                                 sIBChunkID ) 
                      != IDE_SUCCESS );

            IDE_EXCEPTION_CONT( skip_change_tracking );

        }
        else
        {
            //nothing to do
        }

        mCurSpaceID = SC_NULL_SPACEID;
        mCurPageID  = SM_SPECIAL_PID;
        mCurPagePtr = NULL;

        sState = 0;
        if (sApplied == ID_TRUE )
        {
            IDE_ASSERT( ( smLayerCallback::isLSNLT( &sLastModifyLSN, &sMaxApplyLSN ) == ID_TRUE ) ||
                        ( smLayerCallback::isLSNEQ( &sLastModifyLSN, &sInitLSN ) == ID_TRUE )     ||
                        ( sIsOverWriteLog == ID_TRUE ) );

            IDE_TEST( sdrMiniTrans::commit(&sMtx,
                                           (UInt)0, // SMR_CT_END을 의미
                                           &sMaxApplyLSN) != IDE_SUCCESS );
        }
        else
        {
            IDE_ASSERT( (!( ( smLayerCallback::isLSNLT( &sLastModifyLSN, &sMaxApplyLSN ) == ID_TRUE ) ||
                            ( smLayerCallback::isLSNEQ( &sLastModifyLSN, &sInitLSN ) == ID_TRUE ) ))  ||
                        ( sIsOverWriteLog == ID_TRUE ) );

            IDE_TEST( sdrMiniTrans::rollback(&sMtx) != IDE_SUCCESS );
        }

        sNode->mState = SDR_RECV_FINISHED;

        IDE_TEST( iduMemMgr::free( sNode ) != IDE_SUCCESS );
        mApplyPageCount--;

        IDE_TEST( smuHash::cutNode(&mHash, (void **)&sNode) != IDE_SUCCESS );
    }

    // Redo Log가 저장된 Hash Table Close
    sHashState = 0;
    IDE_TEST( smuHash::close(&mHash) != IDE_SUCCESS );

    IDE_DASSERT( mApplyPageCount == 0 ); // 모두 적용되어야 한다.

    IDE_TEST( sdsBufferMgr::flushDirtyPagesInCPList( aStatistics,
                                                     ID_TRUE ) // FLUSH ALL
              != IDE_SUCCESS );


    IDE_TEST( sdbBufferMgr::flushDirtyPagesInCPList( aStatistics,
                                                     ID_TRUE ) // FLUSH ALL
              != IDE_SUCCESS );

    // BUG-45598: 리스타트 리커버리 할 때 에러 페이지 처리 정책을 프로퍼티에 맞게 설정
    sdrCorruptPageMgr::setPageErrPolicyByProp();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // PROJ-2118 BUG Reporting - init Debug Info
    mCurSpaceID = SC_NULL_SPACEID;
    mCurPageID  = SM_SPECIAL_PID;
    mCurPagePtr = NULL;
    mRedoLogPtr = NULL;

    IDE_PUSH();
    {
        if ( sState != 0 )
        {
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        }

        if ( sHashState != 0 )
        {
            IDE_ASSERT( smuHash::close(&mHash) == IDE_SUCCESS );
        }
    }
    IDE_POP();

    sdrCorruptPageMgr::fatalReadCorruptPage();

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description : redo 로그의 parsing
 *
 * 유효한 로그인지 검사하고, disk 로그의 타입, RID, value길이를 반환한다.
 *
 * aStatistics - [IN]  통계정보
 * aLogRec     - [IN]  로그레코드 포인터
 * aLogType    - [OUT] 로그타입
 * aLogGRID    - [OUT] 변경된 페이지의 GRID
 * aValueLen   - [OUT] 로그슬롯의 길이
 *
 ***********************************************************************/
void sdrRedoMgr::parseRedoLogHdr( SChar      * aLogRec,
                                  sdrLogType * aLogType,
                                  scGRID     * aLogGRID,
                                  UInt       * aValueLen )
{
    sdrLogHdr sLogHdr;

    IDE_DASSERT( aLogRec   != NULL );
    IDE_DASSERT( aLogType  != NULL );
    IDE_DASSERT( aLogGRID  != NULL );
    IDE_DASSERT( aValueLen != NULL );

    /* 로그가 align 되어 기록되지 않았기 때문에 memcpy를 이용 */
    idlOS::memcpy(&sLogHdr, aLogRec, ID_SIZEOF(sdrLogHdr));

    IDE_ASSERT( validateLogRec( &sLogHdr ) == ID_TRUE );

    *aLogType    = sLogHdr.mType;
    *aLogGRID    = sLogHdr.mGRID;
    *aValueLen   = sLogHdr.mLength;
}

/***********************************************************************
 *
 * Description : 디스크 로그 슬롯의 유효성 검증
 *
 * aLogHdr - [IN] 디스크 로그 슬롯 헤더
 *
 ***********************************************************************/
idBool sdrRedoMgr::validateLogRec( sdrLogHdr * aLogHdr )
{
    idBool      sIsValid = ID_FALSE;

    IDE_TEST_CONT(
        sctTableSpaceMgr::isSystemMemTableSpace(
            SC_MAKE_SPACE(aLogHdr->mGRID) ) == ID_TRUE,
            ERR_INVALID_LOGREC );

    if ( SC_GRID_IS_WITH_SLOTNUM(aLogHdr->mGRID) )
    {
        IDE_TEST_CONT( SC_MAKE_SLOTNUM(aLogHdr->mGRID) >
                            (SD_PAGE_SIZE / ID_SIZEOF(sdpSlotEntry)),
                        ERR_INVALID_LOGREC );
    }
    else /* OFFSET 값을 갖는 GRID일 때 */
    {
        IDE_TEST_CONT( SC_MAKE_OFFSET(aLogHdr->mGRID) > SD_PAGE_SIZE,
                        ERR_INVALID_LOGREC );
    }

    switch( aLogHdr->mType )
    {
        case SDR_SDP_1BYTE :
        case SDR_SDP_2BYTE :
        case SDR_SDP_4BYTE :
        case SDR_SDP_8BYTE :
        case SDR_SDP_BINARY:
        case SDR_SDP_PAGE_CONSISTENT :
        case SDR_SDP_INIT_PHYSICAL_PAGE :
        case SDR_SDP_INIT_LOGICAL_HDR :
        case SDR_SDP_INIT_SLOT_DIRECTORY :
        case SDR_SDP_FREE_SLOT:
        case SDR_SDP_FREE_SLOT_FOR_SID:
        case SDR_SDP_RESTORE_FREESPACE_CREDIT:
        case SDR_SDP_RESET_PAGE :
        case SDR_SDP_WRITE_PAGEIMG :
        case SDR_SDP_WRITE_DPATH_INS_PAGE : // PROJ-1665

        case SDR_SDPST_INIT_SEGHDR:
        case SDR_SDPST_INIT_BMP:
        case SDR_SDPST_INIT_LFBMP:
        case SDR_SDPST_INIT_EXTDIR:
        case SDR_SDPST_ADD_RANGESLOT:
        case SDR_SDPST_ADD_SLOTS:
        case SDR_SDPST_ADD_EXTDESC:
        case SDR_SDPST_ADD_EXT_TO_SEGHDR:
        case SDR_SDPST_UPDATE_WM:
        case SDR_SDPST_UPDATE_MFNL:
        case SDR_SDPST_UPDATE_LFBMP_4DPATH:
        case SDR_SDPST_UPDATE_PBS:

        case SDR_SDPSC_INIT_SEGHDR :
        case SDR_SDPSC_INIT_EXTDIR :
        case SDR_SDPSC_ADD_EXTDESC_TO_EXTDIR :

        case SDR_SDPTB_INIT_LGHDR_PAGE :
        case SDR_SDPTB_ALLOC_IN_LG:
        case SDR_SDPTB_FREE_IN_LG :

        case SDR_SDC_INSERT_ROW_PIECE :
        case SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE :
        case SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO :
        case SDR_SDC_UPDATE_ROW_PIECE :
        case SDR_SDC_OVERWRITE_ROW_PIECE :
        case SDR_SDC_CHANGE_ROW_PIECE_LINK :
        case SDR_SDC_DELETE_FIRST_COLUMN_PIECE :
        case SDR_SDC_ADD_FIRST_COLUMN_PIECE :
        case SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE :
        case SDR_SDC_DELETE_ROW_PIECE :
        case SDR_SDC_LOCK_ROW :
        case SDR_SDC_PK_LOG:

        case SDR_SDC_INIT_CTL :
        case SDR_SDC_EXTEND_CTL :
        case SDR_SDC_BIND_CTS :
        case SDR_SDC_UNBIND_CTS :
        case SDR_SDC_BIND_ROW :
        case SDR_SDC_UNBIND_ROW :
        case SDR_SDC_ROW_TIMESTAMPING:
        case SDR_SDC_DATA_SELFAGING:

        case SDR_SDC_BIND_TSS :
        case SDR_SDC_UNBIND_TSS :
        case SDR_SDC_SET_INITSCN_TO_TSS:
        case SDR_SDC_INIT_TSS_PAGE :
        case SDR_SDC_INIT_UNDO_PAGE :
        case SDR_SDC_INSERT_UNDO_REC :

        case SDR_SDN_INSERT_INDEX_KEY :
        case SDR_SDN_FREE_INDEX_KEY :
        case SDR_SDN_INSERT_UNIQUE_KEY :
        case SDR_SDN_INSERT_DUP_KEY :
        case SDR_SDN_DELETE_KEY_WITH_NTA :
        case SDR_SDN_FREE_KEYS :
        case SDR_SDN_COMPACT_INDEX_PAGE :
        case SDR_SDN_KEY_STAMPING :
        case SDR_SDN_INIT_CTL :
        case SDR_SDN_EXTEND_CTL :
        case SDR_SDN_FREE_CTS :

        /*
         * PROJ-2047 Strengthening LOB
         */
        case SDR_SDC_LOB_UPDATE_LOBDESC:
        case SDR_SDC_LOB_INSERT_INTERNAL_KEY:
        case SDR_SDC_LOB_INSERT_LEAF_KEY:
        case SDR_SDC_LOB_UPDATE_LEAF_KEY:
        case SDR_SDC_LOB_OVERWRITE_LEAF_KEY:
        case SDR_SDC_LOB_FREE_INTERNAL_KEY:
        case SDR_SDC_LOB_FREE_LEAF_KEY:
        case SDR_SDC_LOB_WRITE_PIECE:
        case SDR_SDC_LOB_WRITE_PIECE4DML:
        case SDR_SDC_LOB_WRITE_PIECE_PREV:
        case SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST:

        /*
         * PROJ-1591: Disk Spatial Index
         */
        case SDR_STNDR_INSERT_INDEX_KEY :
        case SDR_STNDR_UPDATE_INDEX_KEY :
        case SDR_STNDR_FREE_INDEX_KEY :
        case SDR_STNDR_INSERT_KEY :
        case SDR_STNDR_DELETE_KEY_WITH_NTA :
        case SDR_STNDR_FREE_KEYS :
        case SDR_STNDR_COMPACT_INDEX_PAGE :
        case SDR_STNDR_KEY_STAMPING :

            sIsValid = ID_TRUE;
            break;

        default:
            break;
    }

    IDE_EXCEPTION_CONT( ERR_INVALID_LOGREC );

    return sIsValid;
}


/***********************************************************************
 *
 * Description : hash에 저장된 로그들을 모두 db에 반영
 *
 * 유효한 로그 타입인지 검사하고, mtx와 page offset이 NULL이 아니면
 * 해당 로그 타입의 redo 함수를 호출하여 로그의 redo를 수행한다.
 *
 * aStatistics - [IN] statistics
 * aMtx        - [IN] Mtx 포인터
 * aPagePtr    - [IN] 페이지 포이터
 * aData       - [IN] sdrRedoLogData 포인터
 *
 ***********************************************************************/
IDE_RC sdrRedoMgr::applyLogRec( sdrMtx         * aMtx,
                                UChar          * aPagePtr,
                                sdrRedoLogData * aLogData )
{
    sdrRedoInfo  sRedoInfo;
    smrRTOI      sRTOI;
    idBool       sConsistency = ID_FALSE;

    IDE_ASSERT( aMtx      != NULL );
    IDE_ASSERT( aPagePtr  != NULL );
    IDE_ASSERT( aLogData  != NULL );

    sRedoInfo.mLogType = aLogData->mType;

    // PROJ-1705
    // DATA PAGE는 SID(Slot ID)를 사용하기 때문에 mSlotNum 필드를 사용한다.
    sRedoInfo.mSlotNum = aLogData->mSlotNum;

    smrRecoveryMgr::prepareRTOI( NULL, /*smrLog */
                                 NULL, /*smrLogHead*/
                                 &aLogData->mBeginLSN,
                                 aLogData,
                                 aPagePtr, 
                                 ID_TRUE, /* aIsRedo */
                                 &sRTOI );

    smrRecoveryMgr::checkObjectConsistency( &sRTOI, 
                                            &sConsistency );

    /* Redo 해도 되는 바른 객체이다 */
    if ( sConsistency == ID_TRUE )
    {
        if ( sdrUpdate::doRedoFunction( aLogData->mValue,
                                        aLogData->mValueLength,
                                        aPagePtr + aLogData->mOffset,
                                        &sRedoInfo,
                                        aMtx )
            != IDE_SUCCESS )
        {
            sConsistency = ID_FALSE;
        }
    }

    /* 어떤 이유에서건, Redo가 실패했다. */
    if ( sConsistency == ID_FALSE )
    {
        IDE_TEST( smrRecoveryMgr::startupFailure( &sRTOI,
                                                  ID_TRUE ) // isRedo
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 해시테이블에 redo log를 저장
 *
 * parsing된 hash에 space ID,page ID 따라 해시테이블에 저장한다.
 * redo 로그를 저장하다보면 관련된 page개수가 버퍼관리자의
 * MaxPageCount 에 도달하면 이미 hash에 저장된 redo 로그들을 모두
 * DB 페이지에 반영하여, 다른 page를 fix 할 수 있도록 한다.
 *
 * + 2nd. code design
 *   - Hash에서 해당 rid를 갖는 노드를 검색한다.
 *   - if ( 노드를 검색하지 못하면 )
 *     {
 *        - 만약 적용할 노드가 max page count와 동일하다면
 *          지금까지 저장된 redo 로그들을 모두 db 페이지에 적용시키고
 *          hash를 초기화한다.
 *        - sdrRedoHashNode타입의 노드를 할당한다.
 *        - 노드상태를 설정한다.
 *        - 노드의 space id를 설정한다.
 *        - 노드의 page id를 설정한다.
 *        - 노드의 redo 로그 리스트를 초기화한다.
 *        - hash에 노드를 추가한다.
 *        - 적용할 노드 개수를 증가시킨다.
 *        - media recovery 과정의 경우는
 *          2차 hash에서 복구파일노드를 검색한다.
 *          : 존재한다면, 1차hash노드에 2차 hash노드 포인터를 연결한다.
 *          : 존재하지 않는다면 로그를 매달지 않는다.
 *     }
 *     else
 *     {
 *        - 검색된 노드의 space id와 page id가 정확한지 검사한다.
 *     }
 *
 *   - 2차 hash노드가 존재한다면, 해당 로그가 복구대상인지
 *     검사한다. ==> filterRecvRedoLog()
 *   - RECV타입이 RESTART 이거나 filter를 통과한 로그에 대해서는 다음
 *     작업을 계속 진행한다.
 *   - sdrRedoLogData타입의 데이타노드를 할당한다.
 *   - 데이타노드에 redo 로그 타입을 설정한다.
 *   - 데이타노드에 offset을 설정한다.
 *   - 데이타노드에 value를 설정한다.
 *   - 데이타노드에 value 길이를 설정한다.
 *   - 노드의 redo 로그 리스트에 끝에 데이타 노드를 추가한다.
 ***********************************************************************/
IDE_RC sdrRedoMgr::addRedoLogToHashTable( void * aLogDataList )
{

    UInt                 sState = 0;
    sdrRedoHashNode*     sNode;
    sdrRedoLogData*      sData;
    sdrRedoLogData*      sNext;
    scGRID               sGRID;
    idBool               sIsRedo;
    scGRID               sRecvFileKey;
    sdFileID             sFileID;
    sdrRecvFileHashNode* sRecvFileNode;

    sNode   = NULL;
    sData   = (sdrRedoLogData*) aLogDataList;
    sIsRedo = ID_FALSE;

    while( sData != NULL )
    {
        sState = 0;
        /* 다음 Node를 얻어내고, 현재 Node를 때어냄 */
        sNext = (sdrRedoLogData*)SMU_LIST_GET_NEXT( 
                                   &(sData->mNode4RedoLog) )->mData;
        /* circurlarList이기 때문에, 스스로가 스스로를 가리키면 List의
         * 마지막이라는 뜻. */
        if ( sNext == sData )
        {
            sNext = NULL;
        }

        SMU_LIST_DELETE( &sData->mNode4RedoLog );

        /* Hash Table에서 SpaceID와 PageID만을 key로 사용하므로 offset은
         * NULL로 설정한다. */
        SC_MAKE_GRID( sGRID, sData->mSpaceID, sData->mPageID, SC_NULL_OFFSET );
        IDE_DASSERT( sData->mType >= SDR_SDP_1BYTE ||
                     sData->mType <= SDR_SDC_INSERT_UNDO_REC );
        IDE_DASSERT( SC_GRID_IS_NOT_NULL(sGRID) );


        IDE_TEST( smuHash::findNode(&mHash,
                                    &sGRID,
                                    (void **)&sNode) != IDE_SUCCESS );

        if (sNode == NULL) // not found
        {
            /* FIT/Limit/sm/sdr/sdrRedoMgr_addRedoLogToHashTable_malloc1.sql */
            IDU_FIT_POINT_RAISE( "sdrRedoMgr::addRedoLogToHashTable::malloc1",
                                  insufficient_memory );

            IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SDR,
                                              ID_SIZEOF(sdrRedoHashNode),
                                              (void**)&sNode) != IDE_SUCCESS,
                            insufficient_memory );
            sState = 1;
            idlOS::memset(sNode, 0x00, ID_SIZEOF(sdrRedoHashNode));

            sNode->mState = SDR_RECV_NOT_START;

            sNode->mSpaceID = SC_MAKE_SPACE(sGRID);
            sNode->mPageID  = SC_MAKE_PID(sGRID);

            SMU_LIST_INIT_BASE(&sNode->mRedoLogList);
            sNode->mRedoLogCount = 0;
            sNode->mRecvItem     = NULL;


            /* ------------------------------------------------
             * MEDIA RECOVERY 수행 해당 복구파일을 2차 Hash에서
             * 검색하여 존재하지 않으면, 1차 HashNode의 2차 HashNode
             * 포인터를 NULL로 초기화하고 존재한다면 2차 HashNode
             * 포인터를 설정한 후, 2차 Hash노드의 복구 범위안에
             * 해당 로그가 포함되는지를 판단한다.
             * ----------------------------------------------*/
            if (getRecvType() != SMI_RECOVER_RESTART)
            {
                sFileID = SD_MAKE_FID( sNode->mPageID );

                SC_MAKE_GRID(sRecvFileKey, sNode->mSpaceID, sFileID, 0);

                // 2차 hash에서 해당 로그가 복구대상 파일인지 검사
                IDE_TEST( smuHash::findNode(&mRecvFileHash,
                                            &sRecvFileKey,
                                            (void **)&sRecvFileNode) != IDE_SUCCESS );

                if (sRecvFileNode != NULL)
                {
                    IDE_ASSERT(sRecvFileNode->mFileID == sFileID);

                    if ( (!(sRecvFileNode->mFstPageID <= sNode->mPageID) &&
                           (sRecvFileNode->mLstPageID >= sNode->mPageID)) )
                    {

                        ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                                    SM_TRC_DRECOVER_REDO_LOG_HASHING_FATAL1,
                                    sRecvFileNode->mFileNode->mName,
                                    sNode->mSpaceID,
                                    sNode->mPageID);

                        ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                                    SM_TRC_DRECOVER_REDO_LOG_HASHING_FATAL2,
                                    sRecvFileNode->mFstPageID,
                                    sRecvFileNode->mLstPageID);

                        IDE_ASSERT(0);
                    }

                    sNode->mRecvItem = sRecvFileNode;
                }
                else
                {
                    sNode->mState = SDR_RECV_INVALID;
                }
            }
        }
        else
        {
            IDE_DASSERT( SC_MAKE_SPACE(sGRID) == sNode->mSpaceID );
            IDE_DASSERT( SC_MAKE_PID(sGRID) == sNode->mPageID );

            // PROJ-1867 Page Img Log나 Page Init Log의 경우
            // 이전의 Log가 필요 없으므로 제거한다.
            if ( sdrCorruptPageMgr::isOverwriteLog( sData->mType )
                == ID_TRUE )
            {
                IDE_TEST( clearRedoLogList( sNode ) != IDE_SUCCESS );
            }
        }

        if ( sNode->mRecvItem != NULL )
        {
            IDE_DASSERT( getRecvType() != SMI_RECOVER_RESTART );

            (void)filterRecvRedoLog( sNode->mRecvItem,
                                     &sData->mBeginLSN,
                                     &sIsRedo );

            // To fix BUG-14883
            if ( sIsRedo == ID_TRUE )
            {
                sNode->mRecvItem->mApplyRedoLogCnt++;
            }
        }

        if ( ( getRecvType() == SMI_RECOVER_RESTART ) ||
             ( (getRecvType() != SMI_RECOVER_RESTART) && (sIsRedo == ID_TRUE ) ) )
        {
            /* BUG-40107 Media Reocvery 시 Recv LSN 까지만 복구해야 하기 때문에 Node는 생성되었지만 
             * 해당 로그가 Skip되어 Log List에 로그가 없어 FATAL 발생하는 경우가 있습니다.
             * Node를 생성했을 경우 해당 로그의 Skip여부를 판별한 후 insert 하도록 변경합니다 */
            if ( sState != 0 )
            {
                IDE_TEST( smuHash::insertNode( &mHash,
                                               &sGRID,
                                               sNode ) != IDE_SUCCESS );
                mApplyPageCount++;
                if ( sNode->mState == SDR_RECV_INVALID )
                {
                    mNoApplyPageCnt++;
                }

            }

            SMU_LIST_GET_PREV(&sData->mNode4RedoLog) = NULL;
            SMU_LIST_GET_NEXT(&sData->mNode4RedoLog) = NULL;
            sData->mNode4RedoLog.mData = (void*)sData;

            SMU_LIST_ADD_LAST(&sNode->mRedoLogList, &sData->mNode4RedoLog);
            sNode->mRedoLogCount++;
        }
        else
        {
            /* BUG-40107 Node를 생성하였으나 Skip대상 Log인 경우 
             * 생성한 Node를 해제해 주도록 변경합니다. */
            if ( sState != 0 )
            {
                IDE_TEST( iduMemMgr::free( sNode ) != IDE_SUCCESS );
                sState = 0;
                sNode  = NULL;
            }
        }

        sData = sNext;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)iduMemMgr::free(sNode);
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : redoHashNode의 RedoLogList상의 log data들을 모두 제거한다.
 *
 * aRedoHashNode - [IN] RedoLogList에서 log data들을 제거할 RedoHashNode
 **********************************************************************/
IDE_RC sdrRedoMgr::clearRedoLogList( sdrRedoHashNode*  aRedoHashNode )
{
    smuList*        sBase;
    smuList*        sList;
    smuList*        sNextList;
    sdrRedoLogData* sData;

    IDE_ASSERT( aRedoHashNode != NULL );

    sBase = &aRedoHashNode->mRedoLogList;

    for ( sList = SMU_LIST_GET_FIRST(sBase);
          sList != sBase;
          sList = sNextList)
    {
        sData = (sdrRedoLogData*)sList->mData;

        sNextList = SMU_LIST_GET_NEXT(sList);

        SMU_LIST_DELETE(sList);
        IDE_TEST( iduMemMgr::free(sData) != IDE_SUCCESS );
        aRedoHashNode->mRedoLogCount--;

        if ( aRedoHashNode->mRecvItem != NULL )
        {
            IDE_DASSERT(getRecvType() != SMI_RECOVER_RESTART);
            aRedoHashNode->mRecvItem->mApplyRedoLogCnt--;
        }
    }
    IDE_ASSERT( aRedoHashNode->mRedoLogCount == 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 복구 hash노드에 해당하는 redo 로그인지 검사
 ***********************************************************************/
IDE_RC sdrRedoMgr::filterRecvRedoLog(sdrRecvFileHashNode* aHashNode,
                                     smLSN*               aBeginLSN,
                                     idBool*              aIsRedo)
{

    IDE_DASSERT(aHashNode != NULL);
    IDE_DASSERT(aBeginLSN != NULL);
    IDE_DASSERT(aIsRedo   != NULL);
    IDE_DASSERT(getRecvType() != SMI_RECOVER_RESTART);

    *aIsRedo = ID_FALSE;

    if ( smLayerCallback::isLSNGTE( &aHashNode->mToLSN, aBeginLSN )
         == ID_TRUE )
    {
        if ( smLayerCallback::isLSNGTE( aBeginLSN, &aHashNode->mFromLSN )
             == ID_TRUE )
        {
            *aIsRedo = ID_TRUE;
        }
    }

    return IDE_SUCCESS;

}

/*
   파일을 모두 복구하고 hash로부터 RecvFileHashNode를 모두
   해제한다.

   [IN] aResetLogsLSN - 불완전복구시 파일헤더에 설정할 ResetLogsLSN
*/
IDE_RC sdrRedoMgr::repairFailureDBFHdr( smLSN*    aResetLogsLSN )
{
    SChar                 sMsgBuf[ SM_MAX_FILE_NAME ];
    sdrRecvFileHashNode*  sHashNode;

    IDE_ASSERT( getRecvType() != SMI_RECOVER_RESTART );

    IDE_TEST( smuHash::open(&mRecvFileHash) != IDE_SUCCESS );

    IDE_TEST( smuHash::cutNode( &mRecvFileHash,
                                (void **)&sHashNode )
              != IDE_SUCCESS );

    while ( sHashNode != NULL )
    {
        IDE_DASSERT( sHashNode->mApplyRedoLogCnt == 0 );

        IDE_ASSERT( sHashNode->mState != SDR_RECV_FINISHED );

        if ( aResetLogsLSN != NULL)
        {
            IDE_ASSERT( getRecvType() != SMI_RECOVER_COMPLETE );

            IDE_ASSERT( (sHashNode->mToLSN.mFileNo == ID_UINT_MAX) &&
                        (sHashNode->mToLSN.mOffset == ID_UINT_MAX) );

            IDE_ASSERT( (aResetLogsLSN->mFileNo != ID_UINT_MAX) &&
                        (aResetLogsLSN->mOffset != ID_UINT_MAX) );

            SM_GET_LSN( sHashNode->mFileNode->mDBFileHdr.mRedoLSN,
                        *aResetLogsLSN );
        }

        IDE_TEST( sddDiskMgr::writeDBFHdr( NULL, /* idvSQL* */
                                           sHashNode->mFileNode )
                != IDE_SUCCESS );

        idlOS::snprintf( sMsgBuf,
                SM_MAX_FILE_NAME,
                "  \t\tWriting to ..[%s.DBF<ID:%"ID_UINT32_FMT">]",
                sHashNode->mSpaceName,
                sHashNode->mFileID );

        IDE_CALLBACK_SEND_MSG(sMsgBuf);

        sHashNode->mState = SDR_RECV_FINISHED;

        IDE_TEST( iduMemMgr::free( sHashNode ) != IDE_SUCCESS );
        mRecvFileCnt--;

        IDE_TEST( smuHash::cutNode(&mRecvFileHash, (void **)&sHashNode)
                 != IDE_SUCCESS );
    }

    IDE_TEST( smuHash::close(&mRecvFileHash) != IDE_SUCCESS );

    IDE_DASSERT( mRecvFileCnt == 0 ); // 모두 적용되어야 한다.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   파일을 hash로부터 RecvFileHashNode를 모두 해제한다.
*/
IDE_RC sdrRedoMgr::removeAllRecvDBFHashNodes()
{
    sdrRecvFileHashNode*  sHashNode;

    IDE_ASSERT( getRecvType() != SMI_RECOVER_RESTART );

    IDE_TEST( smuHash::open(&mRecvFileHash) != IDE_SUCCESS );

    IDE_TEST( smuHash::cutNode( &mRecvFileHash,
                                (void **)&sHashNode )
              != IDE_SUCCESS );

    while ( sHashNode != NULL )
    {
        IDE_TEST( iduMemMgr::free( sHashNode ) != IDE_SUCCESS );
        mRecvFileCnt--;

        IDE_TEST( smuHash::cutNode( &mRecvFileHash, (void **)&sHashNode )
                  != IDE_SUCCESS );
    }

    IDE_TEST( smuHash::close(&mRecvFileHash) != IDE_SUCCESS );

    // 모두 적용되어야 한다.
    IDE_DASSERT( mRecvFileCnt == 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 복구할 datafile을 분석하여 RecvFileHash(2차 해시)에 삽입
 *
 * + 설계
 *   - 파일명에 대해서 디스크 관리자로부터 DBF노드를 구한다.
 *     : 해당 파일명이 존재하지 않는다면 복구대상 파일이 아니다.
 *       -> IDE_FAILURE 반환 (smERR_ABORT_NoRecvDataFile)
 *
 *   - 검색된 spaceID를 가지고 파일명에 대한 유효성 검사를 수행
 *     : 유효하지 않으면, 복구대상 파일이 아니다.
 *       -> IDE_FAILURE 반환 (smERR_ABORT_NoRecvDataFile)
 *
 *   - 해당 파일의 file Header의 smVersion을 비교하고,
 *     파일헤더를 판독한다.
 *     : 버전이 다르다면
 *       -> IDE_FAILURE 반환 (smERR_ABORT_NoRecvDataFile)
 *
 *   - 판독된 파일헤더와 DBF노드로 부터 복구범위를 결정한다.
 *     : DBF 노드의 OldestLSN이 파일헤더의 OldestLSN보다 크거나 같다면
 *       복구할 필요가 없는 파일이며, SKIP한다.
 *       -> 복구대상파일아님 하지만, IDE_SUCCESS 반환
 *
 *     : 파일을 새로생성한 경우는 createLSN부터 DBF노드의
 *       OldestLSN까지를 복구 범위로 결정하고, 그렇지 않은 경우라면,
 *       DBF노드의 OldestLSN까지 복구범위로 결정한다.
 *
 *   - 복구 범위의 최대 최소 LSN을 구한다.
 *     : 한번의 로그스캔으로 모든 복구대상 파일을 복구하기 위해서이다.
 *
 *   aDBFileHdr   - [IN]  DBFile 에서 읽은 DBFileHeader
 *   aFileName    - [IN]  DBFile명
 *   aRedoFromLSN - [OUT] media recovery redo start LSN
 *   aRedoToLSN   - [OUT] media recovery redo end LSN
 **********************************************************************/
IDE_RC sdrRedoMgr::addRecvFileToHash( sddDataFileHdr*     aDBFileHdr,
                                      SChar*              aFileName,
                                      smLSN*              aRedoFromLSN,
                                      smLSN*              aRedoToLSN )
{
    UInt                  sState = 0;
    scSpaceID             sSpaceID;
    smLSN                 sInitLSN;
    smLSN                 sFromLSN;
    smLSN                 sToLSN;
    scPageID              sFstPageID = SC_NULL_PID;
    scPageID              sLstPageID = SC_NULL_PID;
    scGRID                sHashKey;
    sddDataFileNode*      sFileNode;
    sdrRecvFileHashNode*  sHashNode;
    SChar*                sSpaceName = NULL;

    IDE_DASSERT(aDBFileHdr    != NULL);
    IDE_DASSERT(aFileName     != NULL);
    IDE_DASSERT(aRedoToLSN    != NULL);
    IDE_DASSERT(aRedoFromLSN  != NULL);
    IDE_DASSERT(getRecvType() != SMI_RECOVER_RESTART);

    SM_LSN_INIT( sInitLSN );

    // [0] 기존 경로에 데이타파일이 이미 존재하는지 검사
    IDE_TEST_RAISE( idf::access( aFileName, F_OK ) != 0,
                    err_does_not_exist_datafile );


    // [1] 본 함수 호출이전에 aFileName에 대해 이미 파일이
    // 존재한다는 것을 확인하였고, 절대경로임을 보장한다.
    // 데이타파일의 페이지 범위... 등을 구한다.

    sctTableSpaceMgr::getDataFileNodeByName( aFileName,
                                             &sFileNode,
                                             &sSpaceID,
                                             &sFstPageID,
                                             &sLstPageID,
                                             &sSpaceName );

    IDE_ASSERT( sFileNode != NULL );
    IDE_DASSERT( sFileNode->mSpaceID == sSpaceID );

    // BUG-24250   PRJ-1867/MustRedoToLSN.sql Diff
    // 완전 혹은 불완전 복구 후, FileNode의 FileHdr를 DBFile의
    // FileHdr에 덮어쓰면 DBFile의 MustRedoToLSN을 읽어버리게 됩니다.
    // MustRedoToLSN을 유지하기 위해 DBFile의 MustRedoToLSN을
    // DBFileNode에 복사해두도록 합니다.
    sFileNode->mDBFileHdr.mMustRedoToLSN = aDBFileHdr->mMustRedoToLSN;

    // [2] TO REDO LSN 결정하기
    if ( mRecvType == SMI_RECOVER_COMPLETE )
    {
        // loganchor의 DBFileHdr의 DiskRedoLSN
        sToLSN = sFileNode->mDBFileHdr.mRedoLSN;

        // BUG-24250
        // MustRedoToLSN 까지의 진행 유무는 Restart Recovery에서 검증합니다.
        // 완전복구에서는 LogAnchor의 RedoLSN까지만 복구하면 되므로
        // MustRodoToLSN을 ToLSN에 반영하는 코드를 제거합니다.
    }
    else
    {
        // 불완전 복구일경우 할수 있는데까지
        // 미디어 복구를 진행한다.
        IDE_ASSERT((mRecvType == SMI_RECOVER_UNTILCANCEL) ||
                   (mRecvType == SMI_RECOVER_UNTILTIME));
        SM_LSN_MAX(sToLSN);
    }

    /*
      [3] FROM REDOLSN 결정하기
      만약 파일헤더의 REDOLSN이 INITLSN이라면 EMPTY
      데이타파일이다.
    */
    if ( smLayerCallback::isLSNEQ( &aDBFileHdr->mRedoLSN,
                                   &sInitLSN ) == ID_TRUE )
    {
        // 불완전복구 요구시 EMPTY 파일이 존재한다면
        // 완전복구를 수행하여야하기 때문에 에러처리한다.
        IDE_TEST_RAISE( ( mRecvType == SMI_RECOVER_UNTILTIME ) ||
                        ( mRecvType == SMI_RECOVER_UNTILCANCEL ),
                        err_incomplete_media_recovery);

        // EMPTY 데이타파일일 경우에는 CREATELSN
        // 부터 미디어복구를 진행한다.

        sFromLSN = sFileNode->mDBFileHdr.mCreateLSN;
    }
    else
    {
        // 완전복구 또는 불완전 복구에서의
        // FROM REDOLSN은 파일헤더의 REDO LSN부터
        // 진행한다.
        sFromLSN = aDBFileHdr->mRedoLSN;
    }

    /* BUG-19272: 잘못된 파일로 완전 복구 시도시 error가 나지않고 core가
     * 발생함.
     *
     * smERR_ABORT_Invalid_DataFile_Create_LSN 가 발생하도록 수정함.
     * */
    // 미디어복구에서는
    // FROM REDOLSN < TO REDOLSN의 조건을 만족해야한다.
    IDE_TEST_RAISE( smLayerCallback::isLSNGT( &sToLSN, &sFromLSN ) == ID_FALSE,
                    err_datafile_invalid_create_lsn );


    // 2차 hashkey는 GRID가 아니지만 GRID 구조를
    // 사용하여 구현하였다.
    SC_MAKE_GRID( sHashKey,
                  sFileNode->mSpaceID,
                  sFileNode->mID,
                  0 );

    IDE_TEST( smuHash::findNode(&mRecvFileHash,
                                &sHashKey,
                                (void **)&sHashNode) != IDE_SUCCESS );

    // 동일한 파일은 한번만 등록되어야 한다.
    IDE_ASSERT( sHashNode == NULL );

    /* TC/FIT/Limit/sm/sdr/sdrRedoMgr::addRecvFileToHash::malloc.sql */
    IDU_FIT_POINT_RAISE( "sdrRedoMgr::addRecvFileToHash::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SDR,
                                ID_SIZEOF(sdrRecvFileHashNode),
                                (void**)&sHashNode) != IDE_SUCCESS,
                    insufficient_memory );

    sState = 1;
    idlOS::memset(sHashNode, 0x00, ID_SIZEOF(sdrRecvFileHashNode));

    sHashNode->mState           = SDR_RECV_NOT_START;
    sHashNode->mSpaceID         = sFileNode->mSpaceID;
    sHashNode->mFileID          = sFileNode->mID;
    sHashNode->mFstPageID       = sFstPageID;
    sHashNode->mLstPageID       = sLstPageID;
    sHashNode->mFromLSN         = sFromLSN;
    sHashNode->mToLSN           = sToLSN;
    sHashNode->mApplyRedoLogCnt = 0;
    sHashNode->mFileNode        = sFileNode;
    sHashNode->mSpaceName       = sSpaceName;

    IDE_TEST( smuHash::insertNode(&mRecvFileHash,
                                  &sHashKey,
                                  sHashNode) != IDE_SUCCESS );
    mRecvFileCnt++;

    *aRedoFromLSN = sFromLSN;
    *aRedoToLSN   = sToLSN;

    ideLog::log(SM_TRC_LOG_LEVEL_DRECOV,
                SM_TRC_DRECOVER_ADD_DATA_FILE1,
                sHashNode->mFileNode->mName,
                sHashNode->mSpaceID,
                sHashNode->mFileID);


    ideLog::log(SM_TRC_LOG_LEVEL_DRECOV,
                SM_TRC_DRECOVER_ADD_DATA_FILE2,
                sHashNode->mFstPageID,
                sHashNode->mLstPageID);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_does_not_exist_datafile );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileByPath, aFileName ) );
    }
    IDE_EXCEPTION( err_incomplete_media_recovery );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NeedMediaRecovery ) );
    }
    IDE_EXCEPTION( err_datafile_invalid_create_lsn );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidDataFileCreateLSN,
                                  sFromLSN.mFileNo,
                                  sFromLSN.mOffset,
                                  aFileName,
                                  sToLSN.mFileNo,
                                  sToLSN.mOffset ) );
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if (sState != 0)
        {
            IDE_ASSERT( iduMemMgr::free(sHashNode) == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;

}


/***********************************************************************
 * Description : redo log의 rid를 이용한 hash value를 생성
 *
 * space id와 page id를 적절히 변환하여 정수를 만들어 리턴한다.
 * 이 함수는 redo log에 대한 hash function으로 사용된다. hash key는 RID이다.
 **********************************************************************/
UInt sdrRedoMgr::genHashValueFunc(void* aGRID)
{

    IDE_DASSERT( aGRID != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isSystemMemTableSpace( SC_MAKE_SPACE(*(scGRID*)aGRID) )
                 == ID_FALSE );

    return ((UInt)SC_MAKE_SPACE(*(scGRID*)aGRID) +
            (UInt)SC_MAKE_PID(*(scGRID*)aGRID));

}

/***********************************************************************
 * Description : hash-key 비교함수
 *
 * 2개의 RID가 같은지 비교한다. 같으면 0을 리턴한다.
 * 이 함수는 redo log에 대한 hash compare function으로 사용된다.
 **********************************************************************/
SInt sdrRedoMgr::compareFunc(void*  aLhs,
                             void*  aRhs )
{

    IDE_DASSERT( aLhs != NULL );
    IDE_DASSERT( aRhs != NULL );

    scGRID  *sLhs = (scGRID *)aLhs;
    scGRID  *sRhs = (scGRID *)aRhs;

    return ( ( SC_MAKE_SPACE( *sLhs ) == SC_MAKE_SPACE( *sRhs ) ) &&
             ( SC_MAKE_PID( *sLhs )  == SC_MAKE_PID( *sRhs ) ) ? 0 : -1 );

}

/**********************************************************************
 * Description : PROJ-2118 BUG Reporting
 *               Server Fatal 시점에 Signal Handler 가 호출할
 *               Debugging 정보 기록함수
 *
 *               이미 altibase_dump.log 에 lock을 잡고 들어오므로
 *               lock을 잡지않는 trace 기록 함수들을 사용해야 한다.
 *
 **********************************************************************/
void sdrRedoMgr::writeDebugInfo()
{
    if (( mCurSpaceID != SC_NULL_SPACEID ) &&
        ( mCurPageID  != SM_SPECIAL_PID ))
    {
        ideLog::log( IDE_DUMP_0,
                     "[ Cur Disk Page ] ( "
                     "SpaceID : %"ID_UINT32_FMT", "
                     "PageID  : %"ID_UINT32_FMT" )\n",
                     mCurSpaceID,
                     mCurPageID );

        if ( mCurPagePtr != NULL )
        {
            (void)sdpPhyPage::tracePage( IDE_DUMP_0,
                                         mCurPagePtr,
                                         "[ Page In Buffer ]" );

        }

        (void)sddDiskMgr::tracePageInFile( IDE_DUMP_0,
                                           mCurSpaceID,
                                           mCurPageID,
                                           "[ Page In File ]" );
        if ( mRedoLogPtr != NULL )
        {
            ideLog::log( IDE_DUMP_0,
                         "[ Disk Redo Log Info ]\n"
                         "Log Type : %"ID_UINT32_FMT"\n"
                         "Offset   : %"ID_UINT32_FMT"\n"
                         "SlotNum  : %"ID_UINT32_FMT"\n"
                         "Length   : %"ID_UINT32_FMT"\n"
                         "TransID  : %"ID_UINT32_FMT"\n"
                         "BeginLSN : [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n"
                         "EndLSN   : [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                         mRedoLogPtr->mType,
                         mRedoLogPtr->mOffset,
                         mRedoLogPtr->mSlotNum,
                         mRedoLogPtr->mValueLength,
                         mRedoLogPtr->mTransID,
                         mRedoLogPtr->mBeginLSN.mFileNo,
                         mRedoLogPtr->mBeginLSN.mOffset,
                         mRedoLogPtr->mEndLSN.mFileNo,
                         mRedoLogPtr->mEndLSN.mOffset );

            ideLog::logMem( IDE_DUMP_0,
                            (UChar*)mRedoLogPtr,
                            ID_SIZEOF( sdrRedoLogData ),
                            "[ Disk Log Head ]" );

            ideLog::logMem( IDE_DUMP_0,
                            (UChar*)mRedoLogPtr->mValue,
                            mRedoLogPtr->mValueLength,
                            "[ Disk Log Body ]" );
        }
    }
}
