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
 * $Id: smxTableInfoMgr.cpp 91252 2021-07-19 00:47:57Z emlee $
 **********************************************************************/

# include <smErrorCode.h>
# include <smp.h>
# include <smx.h>
# include <smr.h>
# include <sdp.h>

/***********************************************************************
 * Description : Table Info Manager 전역 초기화
 *
 * 트랜잭션마다 테이블정보관리자를 가지고 있으며, 서버구동과정의 트랜잭션
 * 전역초기화시에 테이블정보관리자도 함께 초기화된다.
 *
 * - 운영시 메모리할당을 적게하기 위해 SMX_TABLEINFO_CACHE_COUNT만큼
 *   미리 smxTableInfo 객체를 할당하며, smxTableInfo의 freelist를 관리한다.
 * - Hash를 사용하여 table OID를 입력하여 smxTableInfo 객체를 검색가능하게 한다.
 * - Hash에 삽입된 smxTableInfo 객체들에 대하여 추가적으로 smxTableInfo
 *   객체 참조 리스트를 구성한다. 객체 참조 리스트는 table OID 순으로 정렬이
 *   되어 있다.
 ***********************************************************************/
IDE_RC smxTableInfoMgr::initialize()
{
    UInt          i;
    smxTableInfo *sCurTableInfo;

    mCacheCount = SMX_TABLEINFO_CACHE_COUNT;
    mFreeTableInfoCount = mCacheCount;

    IDE_TEST( mHash.initialize( IDU_MEM_SM_TABLE_INFO,
                                SMX_TABLEINFO_CACHE_COUNT,
                                SMX_TABLEINFO_HASHTABLE_SIZE ) 
              != IDE_SUCCESS );

    mTableInfoFreeList.mPrevPtr = &mTableInfoFreeList;
    mTableInfoFreeList.mNextPtr = &mTableInfoFreeList;
    mTableInfoFreeList.mRecordCnt = ID_ULONG_MAX;

    /* TMS 세그먼트를 위한 가용공간 탐색 시작 Data 페이지의 PID */
    mTableInfoFreeList.mHintDataPID = SD_NULL_PID;

    mTableInfoList.mPrevPtr = &mTableInfoList;
    mTableInfoList.mNextPtr = &mTableInfoList;
    mTableInfoList.mRecordCnt = ID_ULONG_MAX;

    for ( i = 0 ; i < mCacheCount ; i++ )
    {
        /* TC/FIT/Limit/sm/smx/smxTableInfoMgr_initialize_malloc.sql */
        IDU_FIT_POINT_RAISE( "smxTableInfoMgr::initialize::malloc",
                              insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_TABLE_INFO,
                                           ID_SIZEOF(smxTableInfo),
                                           (void**)&sCurTableInfo) 
                        != IDE_SUCCESS,
                        insufficient_memory );

        initTableInfo(sCurTableInfo);
        addTableInfoToFreeList(sCurTableInfo);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table Info Manager 전역 해제
 *
 * 서버종료과정의 트랜잭션 전역해제시에 테이블정보관리자도 함께 해제된다.
 * - Hash 및 미리 할당되어 있던 smxTableInfo 객체들을 모두 정리한다.
 ***********************************************************************/
IDE_RC smxTableInfoMgr::destroy()
{
    smxTableInfo *sCurTableInfo;
    smxTableInfo *sNxtTableInfo;

    IDE_TEST( init() != IDE_SUCCESS );

    sCurTableInfo = mTableInfoFreeList.mNextPtr;

    while( sCurTableInfo != &mTableInfoFreeList )
    {
        sNxtTableInfo = sCurTableInfo->mNextPtr;

        removeTableInfoFromFreeList(sCurTableInfo);

        IDE_TEST( iduMemMgr::free(sCurTableInfo) != IDE_SUCCESS );

        sCurTableInfo = sNxtTableInfo;
    }

    mFreeTableInfoCount = 0;

    IDE_TEST( mHash.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table Info Manager 전역 해제
 *
 * 서버종료과정의 트랜잭션 전역해제시에 테이블정보관리자도 함께 해제된다.
 * - Hash 및 미리 할당되어 있던 smxTableInfo 객체들을 모두 정리한다.
 ***********************************************************************/
IDE_RC smxTableInfoMgr::init()
{
    smxTableInfo   *sCurTableInfo;
    smxTableInfo   *sNxtTableInfo;

    sCurTableInfo = mTableInfoList.mNextPtr;

    // Update Table Record And Unlock Mutex.
    while ( sCurTableInfo != &mTableInfoList )
    {
        sNxtTableInfo = sCurTableInfo->mNextPtr;

        IDE_TEST( mHash.remove( sCurTableInfo->mTableOID )
                  != IDE_SUCCESS );

        removeTableInfoFromList( sCurTableInfo );
        IDE_TEST( freeTableInfo( sCurTableInfo ) != IDE_SUCCESS );

        sCurTableInfo = sNxtTableInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : smxTableInfo 객체를 검색하여 반환한다.
 *
 * 테이블 Cursor open 또는 트랜잭션 철회과정에서 호출되면, 검색하고자하는 테이블의
 * OID를 입력하여 해당 테이블의 smxTableInfo 객체를 반환한다.
 ***********************************************************************/
IDE_RC smxTableInfoMgr::getTableInfo( smOID          aTableOID,
                                      smxTableInfo **aTableInfoPtr,
                                      idBool         aIsSearch )
{
    smxTableInfo   *sTableInfoPtr;

    IDE_DASSERT( aTableOID  != SM_NULL_OID );
    IDE_DASSERT( aTableInfoPtr != NULL );

    sTableInfoPtr = NULL;
    sTableInfoPtr = (smxTableInfo*)(mHash.search(aTableOID));

    if ( ( sTableInfoPtr == NULL ) && ( aIsSearch == ID_FALSE ) )
    {
        IDE_TEST( allocTableInfo(&sTableInfoPtr) != IDE_SUCCESS );

        sTableInfoPtr->mTableOID    = aTableOID;

        sTableInfoPtr->mDirtyPageID = ID_UINT_MAX;
        
        IDE_TEST( mHash.insert(aTableOID, sTableInfoPtr) != IDE_SUCCESS );

        addTableInfoToList(sTableInfoPtr);
    }

    *aTableInfoPtr = sTableInfoPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 모든  smxTableInfo 객체 중 TableHeader  가 존재하면
 * ModifyCount를 증가시켜준다.
 ***********************************************************************/
void smxTableInfoMgr::addTablesModifyCount( void )
{

    smxTableInfo   * sTableInfo = NULL;
    smcTableHeader * sTableHeader = NULL;

    sTableInfo = mTableInfoList.mNextPtr;

    while ( sTableInfo != &mTableInfoList )
    {
        IDE_ASSERT( smcTable::getTableHeaderFromOID( sTableInfo->mTableOID,
                                                     (void**)&sTableHeader )
                     == IDE_SUCCESS );

        smcTable::addModifyCount( sTableHeader );
        sTableInfo = sTableInfo->mNextPtr;
    }
}

/***********************************************************************
 * Description : 트랜잭션 commit시 트랜잭션이 참조한 테이블들에 대해서
 *               max rows를 검사한다.
 *
 * 테이블정보관리자의 smxTableInfo 객체 참조 리스트(order by table OID)를 순회하면서
 * 참조된 테이블들의 page list entry의 mutex를 잡는다.
 *
 * - 메모리테이블은 row count가 증가된 테이블에 대해서만 mutex를 잡는다.
 * - disk 테이블은 모두 잡는다.
 * - deadlock을 방지하기 위해 한방향으로 mutex를 잡는다.
 ***********************************************************************/

IDE_RC smxTableInfoMgr::requestAllEntryForCheckMaxRow()
{
    smxTableInfo   *sCurTableInfoPtr;
    smxTableInfo   *sNxtTableInfoPtr;
    smcTableHeader *sCurTableHeaderPtr;
    ULong           sTotalRecord;
    UInt            i, j;
    UInt            sTableType;

    i = 0;
    sTotalRecord = 0;

    sCurTableInfoPtr = mTableInfoList.mNextPtr;

    // Check And Lock Mutex.
    while ( sCurTableInfoPtr != &mTableInfoList )
    {
        i++;
        sNxtTableInfoPtr = sCurTableInfoPtr->mNextPtr;

        IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurTableInfoPtr->mTableOID,
                                                     (void**)&sCurTableHeaderPtr )
                     == IDE_SUCCESS );
       
        sTableType = SMI_GET_TABLE_TYPE( sCurTableHeaderPtr );

        /* BUG-47368: 이미 ID_ULONG_MAX 값이면 체크 할 필요 없다. 이 경우에는
         * 락을 잡지 않고, 이 다음에 호출되는 다음의 함수들 
         * releaseEntryAndUpdateMemTableInfoForIns(), 
         * updateMemTableInfoForDe|()에서 Atomic 연산처리 한다.  
         */  
        if ( ( sCurTableHeaderPtr->mMaxRow == ID_ULONG_MAX ) &&
             ( sTableType != SMI_TABLE_DISK ) )    
        {
            sCurTableInfoPtr = sNxtTableInfoPtr;
            continue;
        }

        if ( ( sTableType == SMI_TABLE_META )   ||
             ( sTableType == SMI_TABLE_MEMORY ) ||
             ( sTableType == SMI_TABLE_FIXED ) )
        {
            if ( sCurTableInfoPtr->mRecordCnt > 0 )
            {
                IDE_TEST_RAISE(
                    sCurTableHeaderPtr->mFixed.mMRDB.mRuntimeEntry->mMutex.lock(NULL /* idvSQL* */)
                               != IDE_SUCCESS, err_mutex_lock);

                sTotalRecord = sCurTableInfoPtr->mRecordCnt +
                    sCurTableHeaderPtr->mFixed.mMRDB.mRuntimeEntry->mInsRecCnt;
            }
        }
        else if ( sTableType == SMI_TABLE_VOLATILE )
        {
            if ( sCurTableInfoPtr->mRecordCnt > 0 )
            {
                IDE_TEST_RAISE(
                    sCurTableHeaderPtr->mFixed.mVRDB.mRuntimeEntry->mMutex.lock(NULL /* idvSQL* */)
                               != IDE_SUCCESS, err_mutex_lock);
                sTotalRecord = sCurTableInfoPtr->mRecordCnt +
                               sCurTableHeaderPtr->mFixed.mVRDB.mRuntimeEntry->mInsRecCnt;
            }
        }
        else if ( sTableType == SMI_TABLE_DISK )
        {
            if ( sCurTableInfoPtr->mRecordCnt != 0 )
            {
                IDE_TEST_RAISE( (sCurTableHeaderPtr->mFixed.mDRDB.mMutex)->lock(NULL /* idvSQL* */)
                                != IDE_SUCCESS, err_mutex_lock );

                if ( sCurTableInfoPtr->mRecordCnt > 0 )
                {
                    sTotalRecord = sCurTableInfoPtr->mRecordCnt +
                                   sCurTableHeaderPtr->mFixed.mDRDB.mRecCnt;
                }
            }
        }
        else
        {
            IDE_ASSERT(0);
        }

        IDE_TEST_RAISE( sCurTableHeaderPtr->mMaxRow < sTotalRecord,
                        err_exceed_maxrow );

        sTotalRecord = 0;
        sCurTableInfoPtr = sNxtTableInfoPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_mutex_lock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(err_exceed_maxrow);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ExceedMaxRows));
    }
    IDE_EXCEPTION_END;

    j = 0;
    sCurTableInfoPtr = mTableInfoList.mNextPtr;

    while ( j != i )
    {
        j++;
        sNxtTableInfoPtr = sCurTableInfoPtr->mNextPtr;
        IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurTableInfoPtr->mTableOID,
                                                     (void**)&sCurTableHeaderPtr )
                    == IDE_SUCCESS );
        sTableType = SMI_GET_TABLE_TYPE( sCurTableHeaderPtr );

        if ( ( sTableType == SMI_TABLE_MEMORY ) ||
             ( sTableType == SMI_TABLE_META )   ||
             ( sTableType == SMI_TABLE_FIXED ) )
        {
            if ( sCurTableInfoPtr->mRecordCnt > 0 )
            {
                (void)(sCurTableHeaderPtr->mFixed.mMRDB.mRuntimeEntry->mMutex.unlock());
            }
        }
        else if ( sTableType == SMI_TABLE_VOLATILE )
        {
            if ( sCurTableInfoPtr->mRecordCnt > 0 )
            {
                (void)(sCurTableHeaderPtr->mFixed.mVRDB.mRuntimeEntry->mMutex.unlock());
            }
        }
        else if ( sTableType == SMI_TABLE_DISK )
        {
            if ( sCurTableInfoPtr->mRecordCnt != 0 )
            {
                (void)(sCurTableHeaderPtr->mFixed.mDRDB.mMutex)->unlock();
            }
        }
        else
        {
            IDE_ASSERT(0);
        }

        sCurTableInfoPtr = sNxtTableInfoPtr;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 메모리 테이블의 증가된 row 개수를 갱신하고
 * entry mutex를 release한다.
 ***********************************************************************/

IDE_RC smxTableInfoMgr::releaseEntryAndUpdateMemTableInfoForIns()
{
    smxTableInfo   *sCurTableInfoPtr;
    smxTableInfo   *sNxtTableInfoPtr;
    smcTableHeader *sCurTableHeaderPtr;
    UInt            sTableType;

    sCurTableInfoPtr = mTableInfoList.mNextPtr;

    while ( sCurTableInfoPtr != &mTableInfoList )
    {
        sNxtTableInfoPtr = sCurTableInfoPtr->mNextPtr;

        IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurTableInfoPtr->mTableOID,
                                                     (void**)&sCurTableHeaderPtr )
                    == IDE_SUCCESS );
        sTableType = SMI_GET_TABLE_TYPE( sCurTableHeaderPtr );

        if ( ( sTableType == SMI_TABLE_MEMORY ) ||
             ( sTableType == SMI_TABLE_META )   ||
             ( sTableType == SMI_TABLE_FIXED ) )
        {
            /* TASK-4990 changing the method of collecting index statistics 
             * 갱신된 Row 개수를 누적시킴 */
            /* BUG-47368: 다음의 연산을 atomic으로 변경 할 경우 SINT_MAX 에
             * 맞추어 초기화 할 필요는 없어 보임. X$DBMS_STATS 전용.
             */
            if ( sCurTableInfoPtr->mRecordCnt < 0 )
            {
                (void)idCore::acpAtomicAdd64 ( &(sCurTableHeaderPtr->mStat.mNumRowChange), 
                                               ( -1 * sCurTableInfoPtr->mRecordCnt ) );
            }
            else
            {
                (void)idCore::acpAtomicAdd64 ( &(sCurTableHeaderPtr->mStat.mNumRowChange), 
                                               sCurTableInfoPtr->mRecordCnt );
            }
            
            if ( sCurTableInfoPtr->mRecordCnt > 0 )
            {
                // BUG-47368: ForInsert()
                (void)idCore::acpAtomicAdd64( &(sCurTableHeaderPtr->mFixed.mMRDB.mRuntimeEntry->mInsRecCnt),
                                              sCurTableInfoPtr->mRecordCnt );

                if ( sCurTableHeaderPtr->mMaxRow != ID_ULONG_MAX ) 
                {
                    IDE_TEST_RAISE(
                        sCurTableHeaderPtr->mFixed.mMRDB.mRuntimeEntry->mMutex.unlock()
                        != IDE_SUCCESS, err_mutex_unlock);
                }
                
                sCurTableInfoPtr->mRecordCnt = 0;
            }
        }
        else if ( sTableType == SMI_TABLE_VOLATILE )
        {
            if ( sCurTableInfoPtr->mRecordCnt > 0 )
            {
                // BUG-47368: ForInsert() 
                (void)idCore::acpAtomicAdd64( &(sCurTableHeaderPtr->mFixed.mVRDB.mRuntimeEntry->mInsRecCnt),
                                              sCurTableInfoPtr->mRecordCnt );

                if ( sCurTableHeaderPtr->mMaxRow != ID_ULONG_MAX )
                {
                    IDE_TEST_RAISE(
                        sCurTableHeaderPtr->mFixed.mVRDB.mRuntimeEntry->mMutex.unlock()
                        != IDE_SUCCESS, err_mutex_unlock);
                }
                
                sCurTableInfoPtr->mRecordCnt = 0;
            }
        }

        sCurTableInfoPtr = sNxtTableInfoPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_mutex_unlock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Description : 디스크 테이블의 증감된 row 개수를 갱신과 함께 commit하고
 * entry mutex를 release한다.
 ***********************************************************************/
IDE_RC smxTableInfoMgr::releaseEntryAndUpdateDiskTableInfoWithCommit(
                                                   idvSQL    * aStatistics,
                                                   smxTrans  * aTransPtr,
                                                   smSCN       aCommitSCN,
                                                   smLSN     * aEndLSN )
{
    smxTableInfo      *sCurTableInfoPtr;
    smxTableInfo      *sNxtTableInfoPtr;
    smcTableHeader    *sCurTableHeaderPtr;
    smrTransCommitLog  sCommitLog;
    smrTransCommitLog *sCommitLogHead;
    scPageID           sPageID;
    scOffset           sOffset;
    ULong              sRecordCount;
    sdrMtx             sMtx;
    UInt               sState = 0;
    sdcTSS            *sTSSlotPtr;
    idBool             sTrySuccess;
    UInt               sDskRedoSize;
    UInt               sLogSize;
    smSCN              sInitSCN;
    smuDynArrayBase   *sDynArrayPtr;
    sdSID              sTSSlotSID = SD_NULL_SID;

    aTransPtr->initLogBuffer();
 
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   (void*)aTransPtr,
                                   SDR_MTX_LOGGING,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT |
                                   SM_DLOG_ATTR_TRANS_LOGBUFF )
              != IDE_SUCCESS );
    sState = 1;

    sTSSlotSID = smxTrans::getTSSlotSID( aTransPtr );

    if ( sTSSlotSID != SD_NULL_SID )
    {
        aTransPtr->initCommitLog( &sCommitLog, SMR_LT_DSKTRANS_COMMIT, aCommitSCN  );
    }
    else
    {
        /* bindTSS 에서 실패하면
         * mTXSegEntry != NULL 이면서 sTSSlotSID 는 SD_NULL_SID 인 상태가 된다 */

        aTransPtr->initCommitLog( &sCommitLog, SMR_LT_MEMTRANS_COMMIT, aCommitSCN );
    }

    /* BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
     * Transaction CommitLog의 logHead를 기록한다. */
    IDE_TEST( aTransPtr->writeLogToBuffer( &sCommitLog,
                                           SMR_LOGREC_SIZE(smrTransCommitLog) ) 
              != IDE_SUCCESS );

    sCurTableInfoPtr = mTableInfoList.mNextPtr;

    while ( sCurTableInfoPtr != &mTableInfoList )
    {
        sNxtTableInfoPtr = sCurTableInfoPtr->mNextPtr;

        IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurTableInfoPtr->mTableOID,
                                                     (void**)&sCurTableHeaderPtr )
                    == IDE_SUCCESS );

        if ( ( SMI_TABLE_TYPE_IS_DISK( sCurTableHeaderPtr ) == ID_TRUE ) &&
             ( sCurTableInfoPtr->mRecordCnt != 0 ) )
        {
            /* TASK-4990 changing the method of collecting index statistics 
             * 갱신된 Row 개수를 누적시킴 */
            if ( sCurTableInfoPtr->mRecordCnt < 0 )
            {
                sCurTableHeaderPtr->mStat.mNumRowChange -=
                                                sCurTableInfoPtr->mRecordCnt;
            }
            else
            {
                sCurTableHeaderPtr->mStat.mNumRowChange +=
                                                sCurTableInfoPtr->mRecordCnt;
            }
            /* 초기화 */
            if ( sCurTableHeaderPtr->mStat.mNumRowChange > ID_SINT_MAX )
            {
                sCurTableHeaderPtr->mStat.mNumRowChange = 0;
            }
            
            sRecordCount = sCurTableHeaderPtr->mFixed.mDRDB.mRecCnt +
                sCurTableInfoPtr->mRecordCnt;
            
            sPageID = SM_MAKE_PID(sCurTableHeaderPtr->mSelfOID);
            sOffset = SM_MAKE_OFFSET(sCurTableHeaderPtr->mSelfOID)
                      + SMP_SLOT_HEADER_SIZE;

            IDE_TEST( aTransPtr->writeLogToBuffer( &sPageID,
                                                   ID_SIZEOF( scPageID ) )
                       != IDE_SUCCESS );

            IDE_TEST( aTransPtr->writeLogToBuffer( &sOffset,
                                                   ID_SIZEOF( scOffset ) )
                      != IDE_SUCCESS );

            IDE_TEST( aTransPtr->writeLogToBuffer( &sRecordCount,
                                                   ID_SIZEOF( ULong ) )
                      != IDE_SUCCESS );

            sCurTableInfoPtr->mDirtyPageID = sPageID;
        }

        sCurTableInfoPtr = sNxtTableInfoPtr;
    }

    if ( sTSSlotSID != SD_NULL_SID )
    {
        IDE_TEST( sdbBufferMgr::getPageBySID( 
                                   aStatistics,
                                   SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                   sTSSlotSID,
                                   SDB_X_LATCH,
                                   SDB_WAIT_NORMAL,
                                   &sMtx,
                                   (UChar**)&sTSSlotPtr,
                                   &sTrySuccess ) != IDE_SUCCESS );

        SM_INIT_SCN( &sInitSCN );

        IDE_TEST( sdcTSSlot::setCommitSCN( &sMtx,
                                           sTSSlotPtr,
                                           &sInitSCN,
                                           ID_FALSE ) // aDoUpdate
                  != IDE_SUCCESS );

        // 지금까지 작성된 Mtx내의 Log Buffer를 트랜잭션에 로딩한다.
        sDynArrayPtr = &(sMtx.mLogBuffer);
        sDskRedoSize = smuDynArray::getSize( sDynArrayPtr );

        IDE_TEST( aTransPtr->writeLogToBufferUsingDynArr( sDynArrayPtr,
                                                          sDskRedoSize )
                  != IDE_SUCCESS );
    }
    else
    {
        // Memory 트랜잭션 혹은 Parallel DPath 의 경우 Main Thread는 
        // TSS를 할당하지 않는다.
        sDskRedoSize = 0;
    }

    /* BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
     * Transaction CommitLog의 logTail을 기록한다. */
    IDE_TEST( aTransPtr->writeLogToBuffer( &sCommitLog.mHead.mType,
                                           ID_SIZEOF( smrLogType ) ) 
              != IDE_SUCCESS );

    /* BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
     * Transaction log buffer에서 CommitLog의 size와 diskRedoSize를 설정한다. */
    sCommitLogHead = (smrTransCommitLog*)( aTransPtr->getLogBuffer() );
    sLogSize       = aTransPtr->getLogSize();

    smrLogHeadI::setSize( &sCommitLogHead->mHead, sLogSize );
    sCommitLogHead->mDskRedoSize = sDskRedoSize;

    IDU_FIT_POINT( "1.BUG-15608@smxTableInfoMgr::releaseEntryAndUpdateDiskTableInfoWithCommit" );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx,
                                    SMR_CT_END,
                                    aEndLSN,
                                    SMR_RT_DISKONLY )
              != IDE_SUCCESS );

    sCurTableInfoPtr = mTableInfoList.mNextPtr;

    while ( sCurTableInfoPtr != &mTableInfoList )
    {
        sNxtTableInfoPtr = sCurTableInfoPtr->mNextPtr;

        IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurTableInfoPtr->mTableOID,
                                                     (void**)&sCurTableHeaderPtr )
                    == IDE_SUCCESS );

        if ( ( SMI_TABLE_TYPE_IS_DISK( sCurTableHeaderPtr ) == ID_TRUE ) &&
             ( sCurTableInfoPtr->mRecordCnt != 0 ) )
        {
            /* BUG-15608 : Commit시 TableHeader의 Record갯수에 대해
             * Logging시 WAL을 지키지 않습니다. Commit Log를 기록후에 Table
             * Header의 Record Count를 갱신한다. */

            sCurTableHeaderPtr->mFixed.mDRDB.mRecCnt += sCurTableInfoPtr->mRecordCnt;

            sCurTableInfoPtr->mRecordCnt = 0;

            IDE_TEST_RAISE( (sCurTableHeaderPtr->mFixed.mDRDB.mMutex)->unlock()
                            != IDE_SUCCESS, err_mutex_unlock );

            if (sCurTableInfoPtr->mDirtyPageID != ID_UINT_MAX)
            {
                IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                              sCurTableInfoPtr->mDirtyPageID )
                          != IDE_SUCCESS );
            }
        }

        sCurTableInfoPtr = sNxtTableInfoPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_mutex_unlock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    /* BUG-32576 [sm_transaction] The server does not unlock PageListMutex
     * if the commit log writing occurs an exception. */
    sCurTableInfoPtr = mTableInfoList.mNextPtr;
    while ( sCurTableInfoPtr != &mTableInfoList )
    {
        sNxtTableInfoPtr = sCurTableInfoPtr->mNextPtr;

        IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurTableInfoPtr->mTableOID,
                                                     (void**)&sCurTableHeaderPtr )
                    == IDE_SUCCESS );

        if ( SMI_TABLE_TYPE_IS_DISK( sCurTableHeaderPtr ) &&
             ( sCurTableInfoPtr->mRecordCnt != 0 ) ) 
        {
            sCurTableInfoPtr->mRecordCnt = 0;

            IDE_TEST_RAISE( (sCurTableHeaderPtr->mFixed.mDRDB.mMutex)->unlock()
                            != IDE_SUCCESS, err_mutex_unlock );
        }

        sCurTableInfoPtr = sNxtTableInfoPtr;
    }

    return IDE_FAILURE;
}

void smxTableInfoMgr::updateMemTableInfoForDel()
{
    smxTableInfo   *sCurTableInfoPtr;
    smxTableInfo   *sNxtTableInfoPtr;
    smcTableHeader *sCurTableHeaderPtr;
    UInt            sTableType;

    sCurTableInfoPtr = mTableInfoList.mNextPtr;

    while ( sCurTableInfoPtr != &mTableInfoList )
    {
        sNxtTableInfoPtr = sCurTableInfoPtr->mNextPtr;

        if ( sCurTableInfoPtr->mRecordCnt < 0 )
        {
            IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurTableInfoPtr->mTableOID,
                                                         (void**)&sCurTableHeaderPtr )
                        == IDE_SUCCESS );

            sTableType = SMI_GET_TABLE_TYPE( sCurTableHeaderPtr );

            if ( ( sTableType == SMI_TABLE_MEMORY ) ||
                 ( sTableType == SMI_TABLE_META )   ||
                 ( sTableType == SMI_TABLE_FIXED ) )
            {
                // BUG-47368: ForDelete() 
                (void)idCore::acpAtomicAdd64( &(sCurTableHeaderPtr->mFixed.mMRDB.mRuntimeEntry->mInsRecCnt),
                                sCurTableInfoPtr->mRecordCnt );

                sCurTableInfoPtr->mRecordCnt = 0;
            }
            else if (sTableType == SMI_TABLE_VOLATILE)
            {
                // BUG-47368: ForDelete()
                (void)idCore::acpAtomicAdd64( &(sCurTableHeaderPtr->mFixed.mVRDB.mRuntimeEntry->mInsRecCnt),
                                sCurTableInfoPtr->mRecordCnt );

                sCurTableInfoPtr->mRecordCnt = 0;
            }
        }

        sCurTableInfoPtr = sNxtTableInfoPtr;
    }
}

/*******************************************************************************
 * Description: DPath INSERT를 수행한 TX가 commit 할 때, 각 table info에 대해
 *      필요한 작업들을 처리한다.
 *
 *      1. DPath INSERT를 수행한 table이 index를 가지고 있으면 inconsistent
 *          플래그를 설정한다.
 *      2. DPath INSERT를 수행한 table에 nologging 옵션이 설정되어 있으면
 *          table header에 inconsistent 플래그를 설정하는 redo 로그를 남긴다.
 ******************************************************************************/
IDE_RC smxTableInfoMgr::processAtDPathInsCommit()
{
    smxTableInfo    * sCurTableInfo;
    smxTableInfo    * sNxtTableInfo;
    smcTableHeader  * sTableHeader;
    UInt              sTableType;

    sCurTableInfo = mTableInfoList.mNextPtr;

    while( sCurTableInfo != &mTableInfoList )
    {
        sNxtTableInfo = sCurTableInfo->mNextPtr;

        if ( sCurTableInfo->mExistDPathIns == ID_TRUE )
        {
            IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurTableInfo->mTableOID,
                                                         (void**)&sTableHeader )
                        == IDE_SUCCESS );

            sTableType = (sTableHeader->mFlag & SMI_TABLE_TYPE_MASK);

            IDE_ERROR( sTableType == SMI_TABLE_DISK );

            IDE_TEST( smcTable::setAllIndexInconsistency( sTableHeader )
                      != IDE_SUCCESS );

            if ( smcTable::isLoggingMode( sTableHeader ) == ID_FALSE )
            {
                //------------------------------------------------------------
                // NOLOGGING으로 Direct-Path INSERT가 수행되는 경우,
                // Redo Log가 없기 때문에 Media Recovery 시에
                // Direct-Path INSERT가 수행된 page에는 아무것도 기록되지
                // 않는다. 이때, Table의 상태는 consistent 하지 않다.
                // 따라서 이를 detect 하기 위하여
                // Direct-Path INSER가 NOLOGGING으로 처음 수행된 경우,
                // table header에 inconsistent flag를 설정하는 로그를 로깅한다.
                //------------------------------------------------------------
                IDE_TEST( smrUpdate::setInconsistencyAtTableHead(
                                                    sTableHeader,
                                                    ID_TRUE ) // media recovery 시에만 redo 수행함
                          != IDE_SUCCESS );
            }
            else
            {
                // LOGGING MODE로 Direct-Path INSERT가 수행되는 경우
            }
        }

        sCurTableInfo = sNxtTableInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : smxTableInfo 객체를 메모리 할당한다.
 ***********************************************************************/
IDE_RC smxTableInfoMgr::allocTableInfo( smxTableInfo  ** aTableInfoPtr )
{
    smxTableInfo  * sFreeTableInfo;
    UInt            sState  = 0;

    IDE_DASSERT( aTableInfoPtr != NULL );

    if ( mFreeTableInfoCount != 0 )
    {
        mFreeTableInfoCount--;
        sFreeTableInfo = mTableInfoFreeList.mNextPtr;
        removeTableInfoFromFreeList(sFreeTableInfo);
    }
    else
    {
        /* TC/FIT/Limit/sm/smx/smxTableInfoMgr_allocTableInfo_malloc.sql */
        IDU_FIT_POINT_RAISE( "smxTableInfoMgr::allocTableInfo::malloc",
                              insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_TABLE_INFO,
                                           ID_SIZEOF(smxTableInfo),
                                           (void**)&sFreeTableInfo) != IDE_SUCCESS,
                        insufficient_memory );
        sState = 1;
    }

    initTableInfo(sFreeTableInfo);

    *aTableInfoPtr = sFreeTableInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sFreeTableInfo ) == IDE_SUCCESS );
            sFreeTableInfo = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : smxTableInfo 객체를 메모리 해제한다.
 ***********************************************************************/
IDE_RC smxTableInfoMgr::freeTableInfo( smxTableInfo  * aTableInfo )
{
    if ( mFreeTableInfoCount >= mCacheCount )
    {
        IDE_TEST( iduMemMgr::free(aTableInfo) != IDE_SUCCESS );
    }
    else
    {
        mFreeTableInfoCount++;
        addTableInfoToFreeList(aTableInfo);
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : smxTableInfo 객체의 row count를 1증가시킨다.
 ***********************************************************************/
void  smxTableInfoMgr::incRecCntOfTableInfo( void  * aTableInfoPtr )
{
    IDE_DASSERT( aTableInfoPtr != NULL );

    ((smxTableInfo*)aTableInfoPtr)->mRecordCnt++;

    return;
}

/***********************************************************************
 * Description : smxTableInfo 객체의 row count를 1감소시킨다.
 ***********************************************************************/
void  smxTableInfoMgr::decRecCntOfTableInfo( void  * aTableInfoPtr )
{
    IDE_DASSERT( aTableInfoPtr != NULL );
    ((smxTableInfo*)aTableInfoPtr)->mRecordCnt--;
    return;
}

/***********************************************************************
 * Description : smxTableInfo 객체의 row count를 반환한다.
 ***********************************************************************/
SLong smxTableInfoMgr::getRecCntOfTableInfo( void  * aTableInfoPtr )
{
    IDE_DASSERT( aTableInfoPtr != NULL );
    return (((smxTableInfo*)aTableInfoPtr)->mRecordCnt);
}

/***********************************************************************
 * Description : smxTableInfo 객체의 ExistDPathIns를 설정한다.
 ***********************************************************************/
void smxTableInfoMgr::setExistDPathIns( void  * aTableInfoPtr,
                                        idBool  aExistDPathIns )
{
    IDE_DASSERT( aTableInfoPtr != NULL );
    ((smxTableInfo*)aTableInfoPtr)->mExistDPathIns = aExistDPathIns;
}
