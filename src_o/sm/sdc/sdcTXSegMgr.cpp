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
 * $Id: sdcTXSegMgr.cpp 88545 2020-09-10 09:14:02Z emlee $
 **********************************************************************/

#include <idl.h>
#include <smuProperty.h>
#include <smx.h>
#include <smrRecoveryMgr.h>

#include <sdcDef.h>
#include <sdcReq.h>
#include <sdcTXSegMgr.h>
#include <sdcTXSegFreeList.h>
#include <sdsFile.h>
#include <sdsBufferArea.h>
#include <sdsMeta.h>
#include <sdsBufferMgr.h>
#include <sdptbSpaceDDL.h>

sdcTXSegFreeList     * sdcTXSegMgr::mArrFreeList;
sdcTXSegEntry        * sdcTXSegMgr::mArrEntry;
UInt                   sdcTXSegMgr::mTotEntryCnt;
UInt                   sdcTXSegMgr::mFreeListCnt;
UInt                   sdcTXSegMgr::mCurFreeListIdx;
UInt                   sdcTXSegMgr::mCurEntryIdx4Steal;
idBool                 sdcTXSegMgr::mIsAttachSegment;

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 개수 보정
 *
 * 트랜잭션 세그먼트 관리자를 초기화하기 전에 호출하여 사용자 프로퍼티에 지정된
 * 개수를 보정하고, FreeList 개수를 결정한다.
 * 또한, 운영중 참고하게될 사용자 프로퍼티 정보를 보정된 값으로 바꿔치기한다.
 * 물론 프로퍼티 파일에서 변경되는 것은 아니다.
 *
 * aEntryCnt        - [IN]  보정되기전의 트랜잭션 엔트리 개수 ( 보통 프로퍼티로부터 읽어옴 )
 * aAdjustEntryCnt  - [OUT] 보정된 트랜잭션 세그먼트 엔트리 개수
 *
 ***********************************************************************/
IDE_RC  sdcTXSegMgr::adjustEntryCount( UInt    aEntryCnt,
                                       UInt  * aAdjustEntryCnt )
{
    SChar sBuffer[ sdcTXSegMgr::CONV_BUFF_SIZE ]="";

    if ( aEntryCnt > 1 )
    {
        mFreeListCnt = IDL_MIN(smuUtility::getPowerofTwo( ID_SCALABILITY_CPU ), 512);
        mTotEntryCnt = IDL_MIN(smuUtility::getPowerofTwo( aEntryCnt ), 512);
    }
    else
    {
        // For Test
        mFreeListCnt = 1;
        mTotEntryCnt = 2;
    }

    if ( mTotEntryCnt < mFreeListCnt )
    {
        /* BUG-47681 undo tablespace full */
        mFreeListCnt =  mTotEntryCnt;
    }

    idlOS::snprintf( sBuffer, sdcTXSegMgr::CONV_BUFF_SIZE,
                     "%"ID_UINT32_FMT"", mTotEntryCnt );

    // PROJ-2446 bugbug
    IDE_TEST( idp::update( NULL, "TRANSACTION_SEGMENT_COUNT", sBuffer, 0 )
              != IDE_SUCCESS );

    if ( aAdjustEntryCnt != NULL )
    {
        *aAdjustEntryCnt = mTotEntryCnt;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 *
 * Description : Undo TBS에 초기 트랜잭션 세그먼트를 생성한다.
 *
 * CreateDB 과정에서 Undo TBS에 TSS세그먼트와 Undo 세그먼트를
 * TRANSACTION_SEGMENT_COUNT만큼 각각 생성한다.
 *
 * aStatistics - [IN] 통계정보
 * aTrans      - [IN] 트랜잭션 포인터
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::createSegs( idvSQL   * aStatistics,
                                void     * aTrans )
{
    sdrMtxStartInfo    sStartInfo;

    IDE_ASSERT( aTrans != NULL );

    sStartInfo.mTrans   = aTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    IDE_TEST( createTSSegs( aStatistics, &sStartInfo )
              != IDE_SUCCESS );

    IDE_TEST( createUDSegs( aStatistics, &sStartInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트들을 리셋
 *
 * Undo TBS를 리셋하여( HWM는 리셋하지 않는다 ) TSSEG와 UDSEG를
 * 다시 생성하고 초기화하며, System Reused SCN을 초기화 한다.
 *
 * aStatistics - [IN] 통계정보
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::resetSegs( idvSQL* aStatistics )
{
    void * sTrans;

    IDE_ASSERT( smLayerCallback::allocTrans( &sTrans ) == IDE_SUCCESS );

    IDE_ASSERT( smLayerCallback::beginTrans( sTrans,
                                             (SMI_TRANSACTION_REPL_DEFAULT |
                                              SMI_COMMIT_WRITE_NOWAIT),
                                             NULL )
                == IDE_SUCCESS );

    IDE_TEST( sdpTableSpace::resetTBS( aStatistics,
                                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       sTrans )
              != IDE_SUCCESS );

    /*
     * resetTBS 수행시 발생한 pending작업을 완료하기 위해서는 반드시
     * createSegs이전에 Commit되어야 한다.
     */
    IDE_ASSERT( smLayerCallback::commitTrans( sTrans ) == IDE_SUCCESS );

    IDE_ASSERT( smLayerCallback::beginTrans( sTrans,
                                             (SMI_TRANSACTION_REPL_DEFAULT |
                                              SMI_COMMIT_WRITE_NOWAIT),
                                             NULL )
                == IDE_SUCCESS );

    IDE_TEST( createSegs( aStatistics, sTrans ) != IDE_SUCCESS );

    IDE_ASSERT( smLayerCallback::commitTrans( sTrans ) == IDE_SUCCESS );

    IDE_ASSERT( smLayerCallback::freeTrans( sTrans ) == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 관리자 초기화
 *
 * Mutex, 트랜잭션 세그먼트 엔트리, FreeList를 초기화 한다.
 * 초기화 시점은 Restart 과정이 완료된 이후에 초기화된다.
 * Restart 과정에서는 트랜잭션 세그먼트 엔트리를 사용하지 않으며,
 * 이후 과정에서 엔트리 개수가 변경될 수도 있기 때문에 초기화시점을
 * 이후 과정으로 한다.
 *
 * 트랜잭션 세그먼트에 해당하는 UDS/TSS 세그먼트를 Attach하여 초기화하기도한다.
 * 예를들어, Create Database 과정에서는 Segment 생성이후에 초기화되므로 Attach
 * 하며, Restart Recovery 이전에는 복구전이기 때문에 Segment를 Attach해서는
 * 안된다.
 *
 * aIsAttachSegment - [IN] 트랜잭션 세그먼트 관리자 초기화시 세그먼트 Attach 여부
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::initialize( idBool   aIsAttachSegment )
{

    UInt i;
    UInt sFreeEntryCnt;
    UInt sFstFreeEntry;
    UInt sLstFreeEntry;

    mArrEntry         = NULL;

    IDE_ASSERT( iduMemMgr::malloc( IDU_MEM_SM_TRANSACTION_SEGMENT_TABLE,
                                   (ULong)ID_SIZEOF(sdcTXSegEntry) * mTotEntryCnt,
                                   (void**)&mArrEntry)
                == IDE_SUCCESS );

    mArrFreeList      = NULL;
    IDE_ASSERT( iduMemMgr::malloc( IDU_MEM_SM_TRANSACTION_SEGMENT_TABLE,
                                   (ULong)ID_SIZEOF(sdcTXSegFreeList) * mFreeListCnt,
                                   (void**)&mArrFreeList )
                == IDE_SUCCESS );

    for ( i = 0; i < mTotEntryCnt; i++ )
    {
        initEntry( &mArrEntry[i], i /* aEntryIdx */ );
    }

    sFreeEntryCnt = mTotEntryCnt/mFreeListCnt;
    for ( i = 0; i < mFreeListCnt; i++ )
    {
        (void)new ( mArrFreeList + i ) sdcTXSegFreeList;

        sFstFreeEntry = i * sFreeEntryCnt;
        sLstFreeEntry = (i + 1) * sFreeEntryCnt -1;

        IDE_ASSERT( mArrFreeList[i].initialize( mArrEntry,
                                                i,
                                                sFstFreeEntry,
                                                sLstFreeEntry )
                    == IDE_SUCCESS );
    }

    if ( aIsAttachSegment == ID_TRUE )
    {
        for ( i = 0; i < mTotEntryCnt; i++ )
        {
            IDE_ASSERT( attachSegToEntry( &mArrEntry[i],
                                          i /* aEntryIdx */ )
                        == IDE_SUCCESS );
        }
    }
    mIsAttachSegment = aIsAttachSegment;

    mCurFreeListIdx    = 0;
    mCurEntryIdx4Steal = 0;

    return IDE_SUCCESS;
}
/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 리빌딩
 *
 * 트랜잭션 세그먼트는 Prepare 트랜잭션이 존재하지 않을때는
 * 리셋하지만, 존재하면 리셋하지 못하고, 초기화해서 이전종료
 * 시점까지 사용하던 각 세그먼트들을 계속 사용해야한다.
 *
 * 서버 구동시 이전 종료전의 트랜잭션 세그먼트 개수와 다르게
 * 설정된 경우 트랜잭션 세그먼트들을 지정된 개수로 재생성하고
 * 초기화한다. 이때도 prepare 트랜잭션이 존재하는 경우에는
 * 리셋할 수 없으므로 에러를 출력하고 서버구동을 실패하도록 처리한다.
 *
 * 서버구동과정에서 Reset 트랜잭션 세그먼트 이후에 비정상 종료를
 * 대비하여 EntryCnt가 변경되어야 하는 경우 Reset 하기전에 반드시
 * Buffer Pool 및 Log들을 모두 Flush 한후 checkpoint를 수행한다.
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::rebuild()
{
    UInt     i;
    UInt     sPropEntryCnt;
    UInt     sNewAdjustEntryCnt;

    if ( smxTransMgr::getPreparedTransCnt() == 0 )
    {
        ideLog::log(IDE_SERVER_0,
           "              Reset Undo Tablespace ...");

        IDE_ASSERT( destroy() == IDE_SUCCESS );

        sPropEntryCnt = smuProperty::getTXSEGEntryCnt();

        IDE_ASSERT( adjustEntryCount( sPropEntryCnt,
                                      &sNewAdjustEntryCnt )
                    == IDE_SUCCESS );

        /* Entry 개수 체크만 해서 다른 경우만 flush 수행 */
        if ( isModifiedEntryCnt( sNewAdjustEntryCnt ) == ID_TRUE )
        {
            IDE_TEST( sdsBufferMgr::flushDirtyPagesInCPList(
                                        NULL,      /* aStatistics */
                                        ID_TRUE )  /* flushAll */
                      != IDE_SUCCESS );

            IDE_TEST( sdbBufferMgr::flushDirtyPagesInCPList(
                                        NULL,      /* aStatistics */
                                        ID_TRUE )  /* flushAll */
                      != IDE_SUCCESS );

            IDE_TEST( smrRecoveryMgr::checkpoint(
                                        NULL /* aStatistics */,
                                        SMR_CHECKPOINT_BY_SYSTEM,
                                        SMR_CHKPT_TYPE_BOTH )
                      != IDE_SUCCESS );
        }

        IDE_TEST( resetSegs( NULL /* idvSQL */ ) != IDE_SUCCESS );

        IDE_ASSERT( smrRecoveryMgr::updateTXSEGEntryCnt(
                                          sNewAdjustEntryCnt ) == IDE_SUCCESS );

        IDE_TEST( initialize( ID_TRUE /* aIsAttachSegment */ )
                  != IDE_SUCCESS );
    }
    else
    {
        ideLog::log(IDE_SERVER_0,
           "              Attach Undo Tablespace ...");
        /* prepare 트랜잭션이 있는 경우에는 이미 ON_SEG로 초기화된
         * 트랜잭션 세그먼트 관리자를 그대로 사용한다.
         * FreeList 및 Entry 상태도 모두 복원되어 있다 */
        for ( i = 0; i < mTotEntryCnt; i++ )
        {
            IDE_ASSERT( attachSegToEntry( &mArrEntry[i],
                                          i /* aEntryIdx */ )
                        == IDE_SUCCESS );
        }
        mIsAttachSegment = ID_TRUE;

        // BUG-27024 prepare trans 들의 FstDskViewSCN중 가장 작은
        // 값으로 prepare trans 들의 Oldest View SCN을 설정함
        smxTransMgr::rebuildPrepareTransOldestSCN();

   }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 관리자 해제
 *
 * Mutex, 트랜잭션 세그먼트 엔트리, FreeList를 해제한다.
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::destroy()
{
    UInt      i;

    for( i = 0; i < mTotEntryCnt; i++ )
    {
        finiEntry( &mArrEntry[i] );
    }

    for(i = 0; i < mFreeListCnt; i++)
    {
        IDE_ASSERT( mArrFreeList[i].destroy() == IDE_SUCCESS );
    }

    IDE_ASSERT( iduMemMgr::free( mArrEntry )    == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::free( mArrFreeList ) == IDE_SUCCESS);

    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 초기화
 *
 * 트랜잭션 엔트리 초기화는 각 세그먼트가 모두 생성된 상태에서만 가능하다.
 * 생성된 세그먼트로부터 SegHdr PID와 할당할 수 있는 PID를 받아서
 * 엔트리에 저장한다.
 *
 * aEntry         - [IN] 트랜잭션 세그먼트 엔트리 Pointer
 * aEntryIdx      - [IN] 트랜잭션 세그먼트 엔트리 순번
 *
 ***********************************************************************/
void sdcTXSegMgr::initEntry( sdcTXSegEntry  * aEntry,
                             UInt             aEntryIdx )
{
    IDE_ASSERT( aEntry    != NULL );
    IDE_ASSERT( aEntryIdx != ID_UINT_MAX );

    aEntry->mEntryIdx       = aEntryIdx;
    aEntry->mStatus         = SDC_TXSEG_OFFLINE;
    aEntry->mFreeList       = NULL;

    SMU_LIST_INIT_NODE( &aEntry->mListNode );
    aEntry->mListNode.mData = (void*)aEntry;

    /*
     * BUG-23649 Undo TBS의 균형적인 공간관리를 위한 Steal 정책 구현
     *
     * Steal 연산에서 Expired된 ExtDir 존재하는지를
     * 실제 Next ExtDir에 가보지 않아도 (I/O) 없이 확인할 수 있다.
     */
    SM_MAX_SCN( &aEntry->mMaxCommitSCN );
}


/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리에 Segment 적재
 *
 * 트랜잭션 세그먼트에 Segment의 할당정보를 적재한다.
 *
 * aEntry         - [IN] 트랜잭션 세그먼트 엔트리 Pointer
 * aEntryIdx      - [IN] 트랜잭션 세그먼트 엔트리 순번
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::attachSegToEntry( sdcTXSegEntry  * aEntry,
                                      UInt             aEntryIdx )
{
    scPageID       sSegHdrPID;

    IDE_ASSERT( sdptbSpaceDDL::getTSSPID(
                    NULL, /* aStatistics */
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                    aEntryIdx,
                    &sSegHdrPID ) == IDE_SUCCESS );

    IDE_ASSERT( aEntry->mTSSegmt.initialize( NULL, /* aStatistics */
                                             aEntry,
                                             sSegHdrPID )
                == IDE_SUCCESS );

    IDE_ASSERT( sdptbSpaceDDL::getUDSPID(
                    NULL, /* aStatistics */
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                    aEntryIdx,
                    &sSegHdrPID ) == IDE_SUCCESS );

    IDE_ASSERT( aEntry->mUDSegmt.initialize( NULL, /* aStatistics */
                                             aEntry,
                                             sSegHdrPID )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 해제
 *
 * 트랜잭션 세그먼트의 정보를 모두 초기화 한다.
 *
 * aEntry         - [IN] 트랜잭션 세그먼트 엔트리 Pointer
 * aEntryIdx      - [IN] 트랜잭션 세그먼트 엔트리 순번
 *
 ***********************************************************************/
void sdcTXSegMgr::finiEntry( sdcTXSegEntry * aEntry )
{
    IDE_ASSERT( aEntry != NULL );

    SMU_LIST_DELETE( &aEntry->mListNode );
    SMU_LIST_INIT_NODE( &aEntry->mListNode );
    aEntry->mListNode.mData = NULL;

    aEntry->mFreeList       = NULL;

    if ( mIsAttachSegment == ID_TRUE )
    {
        IDE_ASSERT( aEntry->mTSSegmt.destroy() == IDE_SUCCESS );
        IDE_ASSERT( aEntry->mUDSegmt.destroy() == IDE_SUCCESS );
    }
    aEntry->mStatus        = SDC_TXSEG_OFFLINE;
    aEntry->mEntryIdx      = ID_UINT_MAX;

    SM_MAX_SCN( &aEntry->mMaxCommitSCN );
}


/***********************************************************************
 *
 * Description : Steal 연산시 Expired된 Entry 할당
 *
 * 오프라인 Entry중에서 Entry의 Max CommitSCN과 OldestTransBSCN을
 * 비교해보고 Expired되었는지 확인한 한후, Entry를 할당한다.
 *
 * aStatistics      - [IN]  통계정보
 * aStartEntryIdx   - [IN]  할당할 Entry Idx
 * aSegType         - [IN]  Segment Type
 * aOldestTransBSCN - [IN]  가장 오래전에 시작한 Statement SCN
 * aEntry           - [OUT] 트랜잭션 세그먼트 엔트리 포인터
 *
 ***********************************************************************/
void sdcTXSegMgr::tryAllocExpiredEntry( UInt             aStartEntryIdx,
                                        sdpSegType       aSegType,
                                        smSCN          * aOldestTransBSCN,
                                        sdcTXSegEntry ** aEntry )
{
    sdcTXSegEntry  * sEntry;
    UInt             sCurEntryIdx;

    sEntry       = NULL;
    sCurEntryIdx = aStartEntryIdx;

    while( 1 )
    {
        sdcTXSegFreeList::tryAllocExpiredEntryByIdx( sCurEntryIdx,
                                                     aSegType,
                                                     aOldestTransBSCN,
                                                     &sEntry );

        if ( sEntry != NULL )
        {
            break;
        }

        sCurEntryIdx++;

        if( sCurEntryIdx >= mTotEntryCnt )
        {
            sCurEntryIdx = 0;
        }

        if( sCurEntryIdx == aStartEntryIdx )
        {
            break;
        }
    }

    if ( sEntry != NULL )
    {
        // 트랜잭션 세그먼트 엔트리 초기화시에 모두 설정되어 있어야 한다.
        IDE_ASSERT( sEntry->mEntryIdx != ID_UINT_MAX );
        IDE_ASSERT( sEntry->mStatus   == SDC_TXSEG_ONLINE );
    }

    *aEntry = sEntry;
}

/***********************************************************************
 *
 * Description : Entry Idx에 해당하는 Entry가 오프라인이면 할당
 *
 * (a) Steal
 *
 * (b) Restart Recovery시 Active Entry 바인딩
 *
 * Restart Recovery 과정에서는 비정상종료 이전의 할당되었던 Entry ID가 반드시
 * 바인딩될 수 있도록 TRANSACTION_SEGMENT_COUNT 프로퍼티가 비정상종료 이전으로
 * 보장되어야 하므로, loganchor에 저장된 개수로 초기화를 하며, Restart Recovery
 * 이후에 변경된 TRANSACTION_SEGMENT_COUNT를 적용할 수 있다.
 *
 *
 * aEntryIdx     - [IN]  탐색 시작 Entry Idx
 * aEntry        - [OUT] 트랜잭션 세그먼트 엔트리 포인터
 *
 ***********************************************************************/
void sdcTXSegMgr::tryAllocEntryByIdx( UInt             aEntryIdx,
                                      sdcTXSegEntry ** aEntry )
{
    sdcTXSegEntry  * sEntry = NULL;

    IDE_ASSERT( mTotEntryCnt > aEntryIdx );

    sdcTXSegFreeList::tryAllocEntryByIdx( aEntryIdx,
                                          &sEntry );

    if ( sEntry != NULL )
    {
        // 트랜잭션 세그먼트 엔트리 초기화시에 모두 설정되어 있어야 한다.
        IDE_ASSERT( sEntry->mEntryIdx != ID_UINT_MAX );
        IDE_ASSERT( sEntry->mStatus   == SDC_TXSEG_ONLINE );
    }

    *aEntry = sEntry;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 할당
 *
 * 트랜잭션 세그먼트 엔트리를 할당할 때까지 FreeList를 선택하여 할당 시도한다.
 * 만약 모든 엔트리가 ONLINE 상태라면 TXSEG_ALLOC_WAIT_TIME 만큼 대기한 후
 * 다시 시도한다.
 *
 * aStatistics - [IN]  통계정보
 * aStartInfo  - [IN] Mini Transaction Start Info
 * aEntry      - [OUT] 트랜잭션 세그먼트 엔트리 포인터
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::allocEntry( idvSQL                   * aStatistics,
                                sdrMtxStartInfo          * aStartInfo,
                                sdcTXSegEntry           ** aEntry )
{

    UInt             sCurFreeListIdx;
    UInt             sStartFreeListIdx;
    PDL_Time_Value   sWaitTime;
    idvWeArgs        sWeArgs;
    /* BUG-40266 TRANSACTION_SEGMENT_COUNT 에 도달했는지 trc로그에 남긴다. */
    idBool           sAddTXSegEntryFull = ID_FALSE;
    smTID            sTransID = SM_NULL_TID;

    sStartFreeListIdx = (mCurFreeListIdx++ % mFreeListCnt);
    sCurFreeListIdx   = sStartFreeListIdx;
    *aEntry           = NULL;
    
    IDU_FIT_POINT( "BUG-45401@sdcTXSegMgr::allocEntry::iduCheckSessionEvent" );

    while(1)
    {
        mArrFreeList[sCurFreeListIdx].allocEntry( aEntry );

        if ( *aEntry != NULL )
        {
            break;
        }

        sCurFreeListIdx++;

        if( sCurFreeListIdx >= mFreeListCnt )
        {
            sCurFreeListIdx = 0;
        }

        if( sCurFreeListIdx == sStartFreeListIdx )
        {
            IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );

            IDV_WEARGS_SET(
                &sWeArgs,
                IDV_WAIT_INDEX_ENQ_ALLOCATE_TXSEG_ENTRY,
                0, /* WaitParam1 */
                0, /* WaitParam2 */
                0  /* WaitParam3 */ );

            IDV_BEGIN_WAIT_EVENT( aStatistics, &sWeArgs );

            if( sAddTXSegEntryFull == ID_FALSE )
            {  
                ideLog::log( IDE_SERVER_0,
                             " TRANSACTION_SEGMENT_COUNT is full !!\n"
                             " Current TRANSACTION_SEGMENT_COUNT is %"ID_UINT32_FMT"(Entry count:%"ID_UINT32_FMT")\n"
                             " Please check TRANSACTION_SEGMENT_COUNT\n",
                             smuProperty::getTXSEGEntryCnt(),
                             mTotEntryCnt );
                
                sTransID = smxTrans::getTransID(aStartInfo->mTrans);
                ideLog::log( IDE_SERVER_0,
                             " Transaction (TID:%"ID_UINT32_FMT") is waiting for TXSegs allocation.",
                             sTransID );

                /* trc 는 한번만 남김 */
                sAddTXSegEntryFull = ID_TRUE;
            }
            else 
            {
                /* nothing to do */
            }

            sWaitTime.set( 0, smuProperty::getTXSegAllocWaitTime() );
            idlOS::sleep( sWaitTime );

            IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
        }
    }

    // 트랜잭션 세그먼트 엔트리 초기화시에 모두 설정되어 있어야 한다.
    IDE_ASSERT( (*aEntry)->mEntryIdx != ID_UINT_MAX );
    IDE_ASSERT( (*aEntry)->mStatus   == SDC_TXSEG_ONLINE );
     
    /* 대기하던 Trans 가 잘 나갔는지 확인 */
    if (sAddTXSegEntryFull == ID_TRUE )
    {
        IDE_DASSERT ( sTransID != SM_NULL_TID );
        ideLog::log( IDE_SERVER_0,
                     " Transaction (TID:%"ID_UINT32_FMT") acquired TXSegs allocation.\n",
                     sTransID );
    }
    else 
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* 대기하던 Trans 가 잘 나갔는지 확인 */
    if (sAddTXSegEntryFull == ID_TRUE )
    {
        IDE_DASSERT ( sTransID != SM_NULL_TID );
        ideLog::log( IDE_SERVER_0,
                     " TXSegs allocation failed (TID:%"ID_UINT32_FMT").\n",
                     sTransID );
    }
    else 
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 할당
 *
 * BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
 * transaction에 특정 segment entry를 binding하는 기능 추가
 *
 * aEntryID    - [IN]  할당받을 트랜잭션 세그먼트 Entry ID
 * aEntry      - [OUT] 트랜잭션 세그먼트 엔트리 포인터
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::allocEntryBySegEntryID( UInt             aEntryID,
                                            sdcTXSegEntry ** aEntry )
{
    UInt             sFreeListIdx;

    IDE_TEST_RAISE( mTotEntryCnt <= aEntryID, err_WRONG_ENTRY_ID );

    // entry ID에 해당하는 segment entry가 연결된 free list를 찾음
    sFreeListIdx = aEntryID / (mTotEntryCnt/mFreeListCnt);
    *aEntry      = NULL;

    mArrFreeList[sFreeListIdx].allocEntryByEntryID( aEntryID, aEntry );

    IDE_TEST_RAISE( *aEntry == NULL, err_NOT_AVAILABLE_ENTRY );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_WRONG_ENTRY_ID );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_WRONG_ENTRY_ID, aEntryID, mTotEntryCnt ) );
    }
    IDE_EXCEPTION( err_NOT_AVAILABLE_ENTRY );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_AVAILABLE_ENTRY, aEntryID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : TSS( Transaction Slot Segment)를 생성한다. 기본적으로
 *               TRANSACTION_SEGMENT_COUNT 갯수만큼의 TSS를 생성한다.
 *               각각의 TSS는 기본적으로 Free TS List를 가진다.
 *               단 첫번째 생성되는 System TSS는 Commit TS List를 가진다.
 *
 * aStatistics   - [IN] 통계 정보
 * aStartInfo    - [IN] Mini Transaction Start Info
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::createTSSegs( idvSQL          * aStatistics,
                                  sdrMtxStartInfo * aStartInfo )
{
    UInt              i;
    scPageID          sTSSegPID;
    sdrMtx            sMtx;

    for( i = 0; i < mTotEntryCnt ; i++ )
    {
        IDE_TEST( sdcTSSegment::create( aStatistics,
                                        aStartInfo,
                                        &sTSSegPID ) != IDE_SUCCESS );

        IDE_ASSERT( sdrMiniTrans::begin( aStatistics,
                                         &sMtx,
                                         aStartInfo,
                                         ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                         SM_DLOG_ATTR_DEFAULT )
                    == IDE_SUCCESS );

        IDE_ASSERT( sdptbSpaceDDL::setTSSPID( aStatistics,
                                              &sMtx,
                                              SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                              i,
                                              sTSSegPID )
                    == IDE_SUCCESS );

        IDE_ASSERT( sdrMiniTrans::commit( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Undo Segment들을 SDP_MAX_UDS_CNT만큼 생성하고 생성된
 *               UDS를 SegPID를 TBS의 Header에 설정한다.
 *
 * aStatistics - [IN] 통계 정보
 * aStartInfo  - [IN] Mini Transaction 시작 정보
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::createUDSegs( idvSQL          * aStatistics,
                                  sdrMtxStartInfo * aStartInfo )
{
    UInt              i;
    scPageID          sUDSegPID;
    sdrMtx            sMtx;

    for( i = 0; i < mTotEntryCnt; i++ )
    {
        IDE_TEST( sdcUndoSegment::create( aStatistics,
                                          aStartInfo,
                                          &sUDSegPID ) != IDE_SUCCESS );

        IDE_ASSERT( sdrMiniTrans::begin( aStatistics,
                                         &sMtx,
                                         aStartInfo,
                                         ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                         SM_DLOG_ATTR_DEFAULT )
                    == IDE_SUCCESS );

        IDE_ASSERT( sdptbSpaceDDL::setUDSPID( aStatistics,
                                              &sMtx,
                                              SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                              i,
                                              sUDSegPID )
                    == IDE_SUCCESS );

        IDE_ASSERT( sdrMiniTrans::commit( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : TSS 세그먼트 PID 여부 반환
 *
 ***********************************************************************/
idBool sdcTXSegMgr::isTSSegPID( scPageID aSegPID )
{
    UInt i;

    for( i = 0; i < mTotEntryCnt; i++ )
    {
        if( mArrEntry[i].mTSSegmt.getSegPID() == aSegPID )
        {
            return ID_TRUE;
        }
    }

    return ID_FALSE;
}

/***********************************************************************
 *
 * Description : Undo 세그먼트 PID 여부 반환
 *
 ***********************************************************************/
idBool sdcTXSegMgr::isUDSegPID( scPageID aSegPID )
{
    UInt i;

    for( i = 0; i < mTotEntryCnt; i++ )
    {
        if( mArrEntry[i].mUDSegmt.getSegPID() == aSegPID )
        {
            return ID_TRUE;
        }
    }

    return ID_FALSE;
}

/***********************************************************************
 *
 * Description : 트랜잭션 Commit/Abort한 후에 TSS와 UDS로부터 사용한 Extent Dir.
 *               페이지에 CSCN/ASCN 설정
 *
 * 트랜잭션 커밋 과정 중에 일부이며, 트랜잭션이 사용한 Extent Dir.페이지에 Last CSCN을
 * No-Logging으로 갱신하여 다른 트랜잭션이 재사용여부를 판단할 수 있게 한다.
 *
 * No-Logging으로 처리한 것은 서버가 비정상 종료되면 GSCN보다
 * 작은 값으로 설정되어 있을 것이고, 무조건 재사용가능하게 판단할 수 있을 것이기 때문이다.
 *
 * aStatistics - [IN] 통계정보
 * aEntry      - [IN] 커밋하는 트랜잭션의 트랜잭션 세그먼트 엔트리 포인터
 * aCommitSCN  - [IN] 트랜잭션의 CommitSCN 혹은 AbortSCN(GSCN)
 *
 ***********************************************************************/
IDE_RC sdcTXSegMgr::markSCN( idvSQL        * aStatistics,
                             sdcTXSegEntry * aEntry,
                             smSCN         * aCSCNorASCN )
{
    scPageID         sTSSegPID;
    scPageID         sUDSegPID;
    sdcTSSegment   * sTSSegPtr;
    sdcUndoSegment * sUDSegPtr;

    IDE_ASSERT( aEntry->mExtRID4TSS != SD_NULL_RID );
    IDE_ASSERT( aCSCNorASCN         != NULL );
    IDE_ASSERT( SM_SCN_IS_NOT_INIT( *aCSCNorASCN ) );

    sTSSegPtr = getTSSegPtr( aEntry );
    sTSSegPID = sTSSegPtr->getSegPID();

    IDE_TEST( sTSSegPtr->markSCN4ReCycle(
                    aStatistics,
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                    sTSSegPID,
                    aEntry->mExtRID4TSS,
                    aCSCNorASCN ) != IDE_SUCCESS );

    if ( aEntry->mFstExtRID4UDS != SD_NULL_RID )
    {
        sUDSegPtr = getUDSegPtr( aEntry );
        sUDSegPID = sUDSegPtr->getSegPID();

        IDE_TEST( sUDSegPtr->markSCN4ReCycle(
                               aStatistics,
                               SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                               sUDSegPID,
                               aEntry->mFstExtRID4UDS,
                               aEntry->mLstExtRID4UDS,
                               aCSCNorASCN ) != IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( aEntry->mLstExtRID4UDS == SD_NULL_RID );
    }

    /*
     * 트랜잭션 세그먼트는 한번에 한 트랜잭션만 사용하므로,
     * 가장 마지막에 사용한 트랜잭션 CSCN이 가장 크다.
     */
    SM_SET_SCN( &aEntry->mMaxCommitSCN, aCSCNorASCN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-31055 Can not reuse undo pages immediately after it is used to 
 * aborted transaction 
 * 즉시 재활용 할 수 있도록, ED들을 Shrink한다. */
IDE_RC sdcTXSegMgr::shrinkExts( idvSQL        * aStatistics,
                                void          * aTrans,
                                sdcTXSegEntry * aEntry )
{
    scPageID         sUDSegPID;
    sdcUndoSegment * sUDSegPtr;
    sdrMtxStartInfo  sStartInfo;

    IDE_ASSERT( aTrans != NULL );

    sStartInfo.mTrans   = aTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;


    if ( aEntry->mFstExtRID4UDS != SD_NULL_RID )
    {
        /* Undo만 shrink해준다. TxSeg는 abort중인 row가 볼 수
         * 있으므로 shrink 해서는 안된다. */
        sUDSegPtr = getUDSegPtr( aEntry );
        sUDSegPID = sUDSegPtr->getSegPID();

        IDE_TEST( sUDSegPtr->shrinkExts( aStatistics,
                                         &sStartInfo,
                                         SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                         sUDSegPID,
                                         aEntry->mFstExtRID4UDS,
                                         aEntry->mLstExtRID4UDS ) 
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( aEntry->mLstExtRID4UDS == SD_NULL_RID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#if 0
/******************************************************************************
 *
 * Description : 할당된 트랜잭션 세그먼트 엔트리 개수 반환
 *
 ******************************************************************************/
SInt sdcTXSegMgr::getOnlineEntryCount()
{
    UInt i;
    SInt sTotOnlineEntryCnt = 0;

    for ( i = 0 ; i < mFreeListCnt; i++ )
    {
        sTotOnlineEntryCnt += mArrFreeList[i].getAllocEntryCount();
    }

    IDE_ASSERT( sTotOnlineEntryCnt >= 0 );

    return sTotOnlineEntryCnt;
}
#endif

/******************************************************************************
 *
 * Description : TRANSACTION_SEGMENT_ENTRY_COUNT 프로퍼티가 변경되었는지 체크한다.
 *
 ******************************************************************************/
idBool sdcTXSegMgr::isModifiedEntryCnt( UInt  aNewAdjustEntryCnt )
{
    UInt  sPrvAdjustEntryCnt;

    sPrvAdjustEntryCnt = smrRecoveryMgr::getTXSEGEntryCnt();

    if ( sPrvAdjustEntryCnt == aNewAdjustEntryCnt )
    {
        return ID_FALSE;
    }

    return ID_TRUE;
}

/******************************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리로부터 가용한 공간을 뺏어온다.
 *
 * Optimisitics 방식으로 한번 체크해본 이후이기 때문에 MaxSCN은 확인해 볼 필요없다.
 * 다만, 직접 NxtExtDir를 가보는 것이 최상이다. 무조건 True를 반환한다.
 * Pessmistics의 재수행은 모든 엔트리에 대해서 수행해본다.
 *
 * aStatistics       - [IN] 통계정보
 * aStartInfo        - [IN] Mtx 시작정보
 * aFromSegType      - [IN] From 세그먼트 타입
 * aToSegType        - [IN] To 세그먼트 타입
 * aToEntry          - [IN] To 트랜잭션 세그먼트 엔트리 포인터
 * aSysMinDskViewSCN - [IN] Active 트랜잭션이 가진 Statment 중에서
 *                          가장 오래전에 시작한 Statement의 SCN
 * aTrySuccess       - [OUT] Steal 성공여부
 *
 ******************************************************************************/
IDE_RC sdcTXSegMgr::tryStealFreeExtsFromOtherEntry(
                                            idvSQL          * aStatistics,
                                            sdrMtxStartInfo * aStartInfo,
                                            sdpSegType        aFromSegType,
                                            sdpSegType        aToSegType,
                                            sdcTXSegEntry   * aToEntry,
                                            smSCN           * aSysMinDskViewSCN,
                                            idBool          * aTrySuccess )
{
    sdcTXSegEntry  * sFrEntry;
    UInt             sStartIdx;
    UInt             sCurEntryIdx;
    UInt             sState = 0;
    idBool           sTrySuccess;
    UInt             sPropRetryStealCnt;
    UInt             sRetryCnt;

    IDE_ASSERT( aStartInfo        != NULL );
    IDE_ASSERT( aToEntry          != NULL );
    IDE_ASSERT( aSysMinDskViewSCN != NULL );

    *aTrySuccess = ID_FALSE;
    sTrySuccess  = ID_FALSE;
    sStartIdx    = (mCurEntryIdx4Steal++ % mTotEntryCnt);
    sCurEntryIdx = sStartIdx;

    sPropRetryStealCnt = smuProperty::getRetryStealCount();

    IDE_TEST_CONT( sPropRetryStealCnt == 0, CONT_FINISH_STEAL );
    
    /* 1.target segType에서 ExpiredEntry가 있는지 확인 */
    tryAllocExpiredEntry( sCurEntryIdx,
                          aFromSegType,
                          aSysMinDskViewSCN,
                          &sFrEntry );
    
    /* 2. (1)이 성공했으면 sFrEntry를 Steal 시도 */
    if ( sFrEntry != NULL )
    {
        sState = 1;
        IDE_TEST( tryStealFreeExts( aStatistics,
                                    aStartInfo,
                                    aFromSegType,
                                    aToSegType,
                                    sFrEntry,
                                    aToEntry,
                                    &sTrySuccess ) 
                  != IDE_SUCCESS );

        sState = 0;
        freeEntry( sFrEntry,
                   ID_FALSE /* aMoveToFirst */ );

        IDE_TEST_CONT( sTrySuccess == ID_TRUE, CONT_FINISH_STEAL );
    }

    sRetryCnt = 0;

    /* 3. target segType에서 각 entry 를 순회하면서 Steal 시도 */
    while( 1 )
    {
        sFrEntry = NULL;

        tryAllocEntryByIdx( sCurEntryIdx, &sFrEntry );

        if ( sFrEntry != NULL )
        {
            sState = 1;
            IDE_TEST( tryStealFreeExts( aStatistics,
                                        aStartInfo,
                                        aFromSegType,
                                        aToSegType,
                                        sFrEntry,
                                        aToEntry,
                                        &sTrySuccess ) 
                      != IDE_SUCCESS );

            sState = 0;
            freeEntry( sFrEntry,
                       ID_FALSE /* aMoveToFirst */ );

            IDE_TEST_CONT( sTrySuccess == ID_TRUE, CONT_FINISH_STEAL );
        }

        sCurEntryIdx++;

        if ( sCurEntryIdx >= mTotEntryCnt )
        {
            sCurEntryIdx = 0;
        }

        sRetryCnt++;

        if ( (sCurEntryIdx == sStartIdx) || (sRetryCnt >= sPropRetryStealCnt) )
        {
            break;
        }
    }
    
    IDE_EXCEPTION_CONT( CONT_FINISH_STEAL );

    *aTrySuccess = sTrySuccess;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        sdcTXSegMgr::freeEntry( sFrEntry,
                                ID_FALSE /* aMoveToFirst */ );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/******************************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리로부터 가용한 공간을 뺏어온다.
 *
 * Optimisitics 방식으로 한번 체크해본 이후이기 때문에 MaxSCN은 확인해 볼 필요없다.
 * 다만, 직접 NxtExtDir를 가보는 것이 최상이다. 무조건 True를 반환한다.
 * Pessmistics의 재수행은 모든 엔트리에 대해서 수행해본다.
 *
 * aStatistics      - [IN] 통계정보
 * aStartInfo       - [IN] Mtx 시작정보
 * aFromSegType     - [IN] From 세그먼트 타입
 * aToSegType       - [IN] To 세그먼트 타입
 * aFrEntry         - [IN] From 트랜잭션 세그먼트 엔트리 포인터
 * aToEntry         - [IN] To 트랜잭션 세그먼트 엔트리 포인터
 *
 ******************************************************************************/
IDE_RC sdcTXSegMgr::tryStealFreeExts( idvSQL          * aStatistics,
                                      sdrMtxStartInfo * aStartInfo,
                                      sdpSegType        aFromSegType,
                                      sdpSegType        aToSegType,
                                      sdcTXSegEntry   * aFrEntry,
                                      sdcTXSegEntry   * aToEntry,
                                      idBool          * aTrySuccess )
{
    sdpSegHandle   * sFrSegHandle;
    scPageID         sFrSegPID;
    scPageID         sFrCurExtDirPID;
    sdpSegHandle   * sToSegHandle;
    scPageID         sToSegPID;
    scPageID         sToCurExtDirPID;
    sdpSegMgmtOp   * sSegMgmtOp;

    switch (aFromSegType)
    {
        case SDP_SEG_TYPE_UNDO:
            sFrSegPID       = aFrEntry->mUDSegmt.getSegPID();
            sFrCurExtDirPID = aFrEntry->mUDSegmt.getCurAllocExtDir();
            sFrSegHandle    = aFrEntry->mUDSegmt.getSegHandle();
            break;

        case SDP_SEG_TYPE_TSS:
            sFrSegPID       = aFrEntry->mTSSegmt.getSegPID();
            sFrCurExtDirPID = aFrEntry->mTSSegmt.getCurAllocExtDir();
            sFrSegHandle    = aFrEntry->mTSSegmt.getSegHandle();
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    switch (aToSegType)
    {
        case SDP_SEG_TYPE_UNDO:
            sToSegPID       = aToEntry->mUDSegmt.getSegPID();
            sToCurExtDirPID = aToEntry->mUDSegmt.getCurAllocExtDir();
            sToSegHandle    = aToEntry->mUDSegmt.getSegHandle();
            break;

        case SDP_SEG_TYPE_TSS:
            sToSegPID       = aToEntry->mTSSegmt.getSegPID();
            sToCurExtDirPID = aToEntry->mTSSegmt.getCurAllocExtDir();
            sToSegHandle    = aToEntry->mTSSegmt.getSegHandle();
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST( sSegMgmtOp->mTryStealExts( aStatistics,
                                         aStartInfo,
                                         SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                         sFrSegHandle,
                                         sFrSegPID,
                                         sFrCurExtDirPID,
                                         sToSegHandle,
                                         sToSegPID,
                                         sToCurExtDirPID,
                                         aTrySuccess )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aTrySuccess = ID_FALSE;

    return IDE_FAILURE;
}
