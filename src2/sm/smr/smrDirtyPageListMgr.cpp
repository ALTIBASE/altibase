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
 * $Id: smrDPListMgr.cpp 19996 2007-01-18 13:00:36Z bskim $
 **********************************************************************/

#include <smErrorCode.h>
#include <smm.h>
#include <smu.h>
#include <smr.h>
#include <sctTableSpaceMgr.h>

iduHash smrDPListMgr::mMgrHash;

/*
    Tablespace별 Dirty Page관리자를 관리하는 smrDPListMgr을 초기화
 */
IDE_RC smrDPListMgr::initializeStatic()
{
    IDE_TEST( mMgrHash.initialize( IDU_MEM_SM_SMR,
                                   256, /* aInitFreeBucketCount */
                                   256) /* aHashTableSpace */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Tablespace별 Dirty Page관리자를 관리하는 smrDPListMgr을 파괴
 */
IDE_RC smrDPListMgr::destroyStatic()
{

    // 모든 TBS에 대해 Dirty Page관리자 제거
    IDE_TEST( destroyAllMgrs() != IDE_SUCCESS );

    IDE_TEST( mMgrHash.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    각각의 Hash Element에 대해 호출될 Visitor함수

    Shutdown시에 호출되며 mMgrHash에 잔존하는 TBS에 대해 수행된다.
 */
IDE_RC smrDPListMgr::destoyingVisitor( vULong   aKey,
                                       void   * aData,
                                       void   * /*aVisitorArg*/ )
{
    scSpaceID          sSpaceID = (scSpaceID) aKey;
    smrDirtyPageList * sDPMgr   = (smrDirtyPageList *) aData;

    IDE_DASSERT( sDPMgr != NULL );

    // DROP/DISCARD/OFFLINE되지 않은 Tablespace라면 ?
    if ( sctTableSpaceMgr::isOnlineTBS( sSpaceID ) == ID_TRUE )
    {
        // BUGBUG-1548 - smrRecoveryMgr::destroy시에
        //               Checkpoint가 2번 완료되면 더이상 Dirty Page가
        //               있을 수가 없다.
        //               removeAll대신 Dirty Page가 없음을
        //               ASSERT로 확인하도록 수정 (검증필요)
        //

        // Dirty Page의 PCH에 접근하여 Dirty상태가 아닌 상태로 전이해준다.
        sDPMgr->removeAll(ID_FALSE); // return void
    }
    else
    {
        // DROP/DISCARD/OFFLINE 된 Tablespace의
        // PCH는 이미 Free되어 없는 상태이다.
        // 아무런 처리도 필요하지 않다.
    }

    // Dirt Page List를 파괴하고
    // Hash에서 제거한다. ( Visiting도중 제거가 가능하다 )
    IDE_TEST( removeDPList( aKey ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Hashtable에 등록된 모든 smrDirtyPageList를 destroy한다.
 */
IDE_RC smrDPListMgr::destroyAllMgrs()
{

    IDE_TEST( mMgrHash.traverse( destoyingVisitor,
                                 NULL /* Visitor Arg */ )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    특정 Tablespace의 Dirty Page관리자에 Dirty Page를 추가한다.
 */
IDE_RC smrDPListMgr::add(scSpaceID aSpaceID,
                         smmPCH*   aPCHPtr,
                         scPageID  aPageID)
{
    smrDirtyPageList * sDPList ;

    IDE_DASSERT( aPCHPtr != NULL );

    IDE_TEST( findDPList( aSpaceID, & sDPList ) != IDE_SUCCESS );

    if ( sDPList == NULL )
    {
        // Tablespace의 Dirty Page관리자가 없을 경우 새로 만들어준다.
        IDE_TEST( createDPList( aSpaceID ) != IDE_SUCCESS );

        IDE_TEST( findDPList( aSpaceID, & sDPList ) != IDE_SUCCESS );

        IDE_ASSERT( sDPList != NULL );
    }

    sDPList->add( aPCHPtr, aPageID ); // return void

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * smrDPListMgr::flushDP 를 위한 Action인자
 */
typedef struct smrFlushDPArg
{
    ULong                       mTotalCnt;
    ULong                       mRemoveCnt;
    ULong                       mTotalWaitTime;  /* microsecond */
    ULong                       mTotalSyncTime;  /* microsecond */
    /* Shutdown직전의 마지막 Checkpoint인지 여부*/
    idBool                      mIsFinalWrite;
    smmGetFlushTargetDBNoFunc   mGetFlushTargetDBNoFunc;
    sctStateSet                 mStateSet;
} smrFlushDPArg;


/*
   smrDirtyPageList::writeDirtyPage를 수행하는 Action함수

   [ OFFLINE이거나 DROPPED인 TBS => Dirty Page Flush하지 않음 ]

      Tablespace를 Drop하는 경우 Checkpoint Image File을
      지울 수도 있다.  Checkpoint에서는 지워지는 와중에 있는
      (혹은 이미 지워진) Checkpoint Image에 대해
      Dirty Page를 Flush하지 않는다.

      Tablespace를 Offline이나 Drop하는경우
      PCH및 PCH Array가 중간에 해제될 수도 있다.
        => PCH / PCH Array는 Dirty Page Flush하는데 필수 데이터구조이다.

   [ 동시성 제어 ]

      TBS상태가 DROP이나 OFFLINE으로 전이되지 않도록
      Dirty Page를 Checkpoint Image로 기록하는 동안
      TBS.SyncMutex를 획득한다.

      - 참고 : TBS상태를 DROP이나 OFFLINE으로 전이시키는 Tx는
                TBS.SyncMutex를 잡는다.

 */
IDE_RC smrDPListMgr::writeDirtyPageAction( idvSQL            * /* aStatistics */,
                                           sctTableSpaceNode * aSpaceNode,
                                           void * aActionArg )
{
    UInt               sStage = 0;
    UInt               sTotalCnt;
    UInt               sRemoveCnt;
    ULong              sWaitTime;
    ULong              sSyncTime;

    smrFlushDPArg    * sActionArg = (smrFlushDPArg * ) aActionArg;

    smrDirtyPageList * sDPList;

    IDE_DASSERT( aSpaceNode != NULL );

    if ( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE )
    {
        // TBS상태가 DROP이나 OFFLINE으로 전이되지 않도록 보장
        IDE_TEST( sctTableSpaceMgr::latchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
        sStage = 1;

        if ( sctTableSpaceMgr::hasState(
                                   aSpaceNode,
                                   sActionArg->mStateSet ) == ID_TRUE )
        {
            /* 상태를 만족하지 않는 경우 */
            switch ( sActionArg->mStateSet )
            {
                case SCT_SS_SKIP_CHECKPOINT:
                    // Chkpt시에는
                    // DROP/DISCARD/OFFLINE된 Tablespace라면
                    // 해당 Tablespace의 Dirty Page관리자를 파괴, 제거한다.
                    // (참고. removeDPList는 PCH및 Dirty Page를
                    // 건드리지 않는다. )
                    IDE_TEST( removeDPList( aSpaceNode->mID ) != IDE_SUCCESS );
                    break;
                case SCT_SS_UNABLE_MEDIA_RECOVERY:
                    // DROP/DISCARD된 Tablespace는 제외한다.
                    IDE_TEST( removeDPList( aSpaceNode->mID ) != IDE_SUCCESS );
                    break;
                default:
                    IDE_ASSERT( 0 );
                    break;
            }
        }
        else
        {
            // 상태를 만족하는 경우
            switch ( sActionArg->mStateSet )
            {
                case SCT_SS_SKIP_CHECKPOINT:
                    // TableSpace가 ONLINE상의 경우 Flush 수행
                    break;
                case SCT_SS_UNABLE_MEDIA_RECOVERY:
                    // TableSpace가 ONLINE/OFFLINE상태이므로 Flush 수행
                    break;
                default:
                    IDE_ASSERT( 0 );
                    break;
            }

            // SMR Dirty Page List 객체 얻어오기
            IDE_TEST( findDPList( aSpaceNode->mID,
                                  & sDPList ) != IDE_SUCCESS );

            if ( sDPList != NULL )
            {
                // FLush실시
                IDE_TEST( sDPList->writeDirtyPages(
                            (smmTBSNode*) aSpaceNode,
                            sActionArg->mGetFlushTargetDBNoFunc,
                            sActionArg->mIsFinalWrite,
                            sActionArg->mTotalCnt,
                            &sTotalCnt,
                            &sRemoveCnt,
                            &sWaitTime,
                            &sSyncTime )
                        != IDE_SUCCESS );

                sActionArg->mTotalCnt      += sTotalCnt;
                sActionArg->mRemoveCnt     += sRemoveCnt;
                sActionArg->mTotalWaitTime += sWaitTime;
                sActionArg->mTotalSyncTime += sSyncTime;
            }
            else
            {
                // TBS에 해당하는 SMR Dirty Page List가 없다
                // => Flush할 Dirty Page 가 없는 것이므로 아무것도 하지 않는다.
            }
        }

        sStage = 0;
        IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1:
                IDE_ASSERT( sctTableSpaceMgr::unlatchSyncMutex(
                                aSpaceNode ) == IDE_SUCCESS );
            default:
                break;
        }
    }
    IDE_POP();
    return IDE_FAILURE;
}




/*
   smrDirtyPageList::writePIDLogs를 수행하는 Action함수

   [ OFFLINE이거나 DROPPED인 TBS => Dirty Page Flush하지 않음 ]
       writeDirtyPageAction 와 동일

   [ 동시성 제어 ]
       writeDirtyPageAction 와 동일

 */
IDE_RC smrDPListMgr::writePIDLogAction( idvSQL*             /* aStatistics */,
                                        sctTableSpaceNode * aSpaceNode,
                                        void * /* aActionArg */ )
{
    UInt               sStage = 0;

    smrDirtyPageList * sDPList;

    IDE_DASSERT( aSpaceNode != NULL );

    if ( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE )
    {
        // TBS상태가 DROP이나 OFFLINE으로 전이되지 않도록 보장
        IDE_TEST( sctTableSpaceMgr::latchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
        sStage = 1;

        // DROP/DISCARD/OFFLINE된 Tablespace라면 ?
        if ( sctTableSpaceMgr::hasState( aSpaceNode, SCT_SS_SKIP_CHECKPOINT )
             == ID_TRUE )
        {
            // DROP/DISCARD/OFFLINE된 Tablespace라면
            // 아무런 처리도 수행하지 않는다.
        }
        else
        {
            // SMR Dirty Page List 객체 얻어오기
            IDE_TEST( findDPList( aSpaceNode->mID,
                                  & sDPList ) != IDE_SUCCESS );
            if ( sDPList != NULL )
            {
                // Page Write실시
                IDE_TEST( sDPList->writePIDLogs() != IDE_SUCCESS );
            }
            else
            {
                // TBS에 해당하는 SMR Dirty Page List가 없는 상황
                // Dirty Page가 없는 상황이므로 Page ID로깅도 하지 않는다.
            }
        }

        sStage = 0;
        IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1:
                IDE_ASSERT( sctTableSpaceMgr::unlatchSyncMutex(
                                aSpaceNode ) == IDE_SUCCESS );
            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}



/*
    모든 Tablespace의 Dirty Page에 대해
    PID를 로깅하고 Page Image를 Checkpoint Image에 기록

    [OUT] aTotalCnt  - Flush 실시한 DIRTY PAGE 개수
    [OUT] aRemoveCnt - Dirty Page List에서 제거된 Page의 수
    [IN]  aOption    - Dirty Page기록옵션
                       ex> PID로깅 안하기 (Media Recovery시 적용)

    [알고리즘]
       (010) 모든 TBS에 대해 Dirty Page ID를 0번 LFG에 로깅
       (020) Page ID Log가 기록된 DISK(0번) LFG를 FLUSH
       (030) 모든 TBS에 대해 Dirty Page를 Checkpoint Image에 기록
 */
IDE_RC smrDPListMgr::writeDirtyPages4AllTBS(
                     sctStateSet                 aStateSet,
                     ULong                     * aTotalCnt,
                     ULong                     * aRemoveCnt,
                     ULong                     * aWaitTime,
                     ULong                     * aSyncTime,
                     smmGetFlushTargetDBNoFunc   aGetFlushTargetDBNoFunc,
                     smrWriteDPOption            aWriteOption )
{
    smLSN         sSyncLstLSN;
    smrFlushDPArg sActArg;

    /* BUG-39925 [PROJ-2506] Insure++ Warning
     * - 변수 초기화가 필요합니다.
     */
    idlOS::memset( &sActArg, 0, ID_SIZEOF( sActArg ) );

    sActArg.mGetFlushTargetDBNoFunc = aGetFlushTargetDBNoFunc;
    sActArg.mStateSet               = aStateSet;

    IDE_DASSERT( aTotalCnt               != NULL );
    IDE_DASSERT( aRemoveCnt              != NULL );
    IDE_DASSERT( aWaitTime               != NULL );
    IDE_DASSERT( aSyncTime               != NULL );
    IDE_DASSERT( aGetFlushTargetDBNoFunc != NULL );

    IDE_TEST(smmDatabase::checkMembaseIsValid() != IDE_SUCCESS);

    // Checkpoint 직전의 마지막 dirty page write인지?
    if ( ( aWriteOption & SMR_WDP_FINAL_WRITE ) != SMR_WDP_FINAL_WRITE )
    {
        /* BUG-28554 [SM] CHECKPOINT_BULK_WRITE_SLEEP_[U]SEC이 제대로 
         * 동작하지 않습니다.
         * FINAL_WRITE가 아닌 경우, 즉 Shutdown시의 Checkpoint가 아니면
         * 이하 mIsFinalWriteFlag가 False로 설정되어야 CHECKPOINT_BULK-
         * _WRITE_SLEEP 이 제대로 동작합니다. */
        sActArg.mIsFinalWrite = ID_FALSE;
    }
    else
    {
        sActArg.mIsFinalWrite = ID_TRUE;
    }

    if ( ( aWriteOption & SMR_WDP_NO_PID_LOGGING ) != SMR_WDP_NO_PID_LOGGING )
    {
        /////////////////////////////////////////////////////////////////
        // (010) 모든 TBS에 대해 Dirty Page ID를 0번 LFG에 로깅
        IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                      writePIDLogAction,
                                                      (void*) & sActArg,
                                                      SCT_ACT_MODE_NONE )
                  != IDE_SUCCESS );


        /////////////////////////////////////////////////////////////////
        // (020) Page ID Log가 기록된 DISK(0번) LFG를 FLUSH
        smrLogMgr::getLstLSN(&sSyncLstLSN) ;
        IDE_TEST(smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_CKP,
                                                &sSyncLstLSN )
                 != IDE_SUCCESS);
    }

    /////////////////////////////////////////////////////////////////
    // (030) 모든 TBS에 대해 Dirty Page를 Checkpoint Image에 기록
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  writeDirtyPageAction,
                                                  (void*) & sActArg,
                                                  SCT_ACT_MODE_NONE )
              != IDE_SUCCESS );

    *aTotalCnt  = (ULong) sActArg.mTotalCnt;
    *aRemoveCnt = (ULong) sActArg.mRemoveCnt;
    *aWaitTime  = (ULong) sActArg.mTotalWaitTime;
    *aSyncTime  = (ULong) sActArg.mTotalSyncTime;

    IDE_TEST(smmDatabase::checkMembaseIsValid() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * getTotalDirtyPageCnt 를 위한 Action인자
 */
typedef struct smrCountDPArg
{
    ULong mTotalCnt;
} smrCountDPArg;


/*
 * getTotalDirtyPageCnt 를 위한 Action함수
 */
IDE_RC smrDPListMgr::countDPAction( idvSQL*             /*aStatistics */,
                                    sctTableSpaceNode * aSpaceNode,
                                    void * aActionArg )
{
    UInt                sStage = 0;
    smrDirtyPageList  * sDPList ;

    smrCountDPArg * sActionArg = (smrCountDPArg*) aActionArg;

    if ( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE )
    {
        // TBS상태가 DROP이나 OFFLINE으로 전이되지 않도록 보장
        IDE_TEST( sctTableSpaceMgr::latchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
        sStage = 1;

        // DROP/DISCARD/OFFLINE되지 않은 Tablespace라면 ?
        if ( sctTableSpaceMgr::isOnlineTBS( aSpaceNode->mID ) == ID_TRUE )
        {
            IDE_TEST( findDPList( aSpaceNode->mID,
                                  & sDPList ) != IDE_SUCCESS );

            if ( sDPList != NULL )
            {
                sActionArg->mTotalCnt += sDPList->getDirtyPageCnt();
            }
            else
            {
                // TBS에 해당하는 SMR Dirty Page List가 없다는 것은
                // 아직 Dirty Page가 하나도 없는 경우이다
            }
        }
        else
        {
            // DROP/DISCARD/OFFLINE된 Tablespace라면 ?

            // Dirty Page를 세지 않는다.
        }

        sStage = 0;
        IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1:
                IDE_ASSERT( sctTableSpaceMgr::unlatchSyncMutex(
                                aSpaceNode ) == IDE_SUCCESS );
            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;

}

/*
   모든 Tablespace의 Dirty Page수를 계산
    [OUT] aDirtyPageCount - Dirty Page수
*/
IDE_RC smrDPListMgr::getTotalDirtyPageCnt( ULong * aDirtyPageCount)
{
    smrCountDPArg sActArg;

    IDE_DASSERT( aDirtyPageCount != NULL );

    sActArg.mTotalCnt = 0;

    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  countDPAction,
                                                  (void*) & sActArg,
                                                  SCT_ACT_MODE_NONE )
              != IDE_SUCCESS );

    *aDirtyPageCount = sActArg.mTotalCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    특정 Tablespace의 Dirty Page수를 계산한다.

    [IN]  aTBSID          - Dirty Page수를 계산할 Tablespace ID
    [OUT] aDirtyPageCount - Dirty Page수
*/
IDE_RC smrDPListMgr::getDirtyPageCountOfTBS( scSpaceID   aTBSID,
                                             UInt      * aDirtyPageCount )
{
    smrDirtyPageList * sDPList ;

    IDE_DASSERT( aDirtyPageCount != NULL );

    IDE_TEST( findDPList( aTBSID, & sDPList ) != IDE_SUCCESS );
    if ( sDPList != NULL )
    {
        *aDirtyPageCount = sDPList->getDirtyPageCnt();
    }
    else
    {
        // aTBSID의 Dirty Page들을 저장하는 SMR Dirty Page List 가 없다.
        // => Dirty Page갯수는 0개
        *aDirtyPageCount = 0;
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    특정 Tablespace를 위한 Dirty Page관리자를 생성한다.

    [IN] aSpaceID - 생성하고자 하는 Dirty Page관리자가 속한 Tablespace의 ID
 */
IDE_RC smrDPListMgr::createDPList(scSpaceID aSpaceID )
{
    smrDirtyPageList  * sDPList;
    UInt                sState  = 0;

    /* TC/FIT/Limit/sm/smr/smrDPListMgr_createDPList_malloc.sql */
    IDU_FIT_POINT_RAISE( "smrDPListMgr::createDPList::malloc", 
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMR,
                               ID_SIZEOF(smrDirtyPageList),
                               (void**) & sDPList ) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 1;

    new ( sDPList ) smrDirtyPageList;

    IDE_TEST( sDPList->initialize( aSpaceID ) != IDE_SUCCESS);

    IDE_TEST( mMgrHash.insert( aSpaceID, sDPList ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sDPList ) == IDE_SUCCESS );
            sDPList = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/*
   특정 Tablespace를 위한 Dirty Page관리자를 찾아낸다.
   찾지 못한 경우 NULL이 리턴된다.

   [IN]  aSpaceID - 찾고자 하는 DirtyPage관리자가 속한 Tablespace의 ID
   [OUT] aDPList   - 찾아낸 Dirty Page 관리자
*/

IDE_RC smrDPListMgr::findDPList( scSpaceID           aSpaceID,
                                 smrDirtyPageList ** aDPList )
{
    *aDPList = (smrDirtyPageList * ) mMgrHash.search( aSpaceID );

    return IDE_SUCCESS;
}

/*
    특정 Tablespace의 Dirty Page관리자를 제거한다.

    [IN] aSpaceID - 제거하고자 하는 Dirty Page관리자가 속한 Tablespace ID
 */
IDE_RC smrDPListMgr::removeDPList( scSpaceID aSpaceID )
{
    smrDirtyPageList * sDPList;

    sDPList = (smrDirtyPageList *) mMgrHash.search( aSpaceID );

    // SMR Dirty Page List가 존재할 경우
    if ( sDPList != NULL )
    {
        // 제거 실시
        IDE_TEST( sDPList->destroy() != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free( sDPList ) != IDE_SUCCESS );

        IDE_TEST( mMgrHash.remove( aSpaceID ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    특정 Tablespace의 Dirty Page를 SMM=>SMR로 이동한다.

    [IN] aSpaceID - Dirty Page를 이동하고자 하는 Tablespace의 ID
    [OUT] aNewCnt - 새로 추가된 Dirty Page 수
    [OUT] aDupCnt - 기존에 존재하였던 Dirty Page 수
 */
IDE_RC smrDPListMgr::moveDirtyPages4TBS( scSpaceID   aSpaceID,
                                         UInt      * aNewCnt,
                                         UInt      * aDupCnt )
{
    smmDirtyPageMgr  * sSmmDPMgr;
    smrDirtyPageList * sSmrDPList;


    IDE_TEST( smmDirtyPageMgr::findDPMgr( aSpaceID, & sSmmDPMgr )
              != IDE_SUCCESS );

    // SMM Dirty Page Mgr는 TBS가 초기화되면서 함께 초기화 된다.
    // 이 함수는 ONLINE상태인 Tablespace에 대해 호출되기 때문에
    // SMM Dirty Page Mgr이 존재해야 한다.
    IDE_ASSERT( sSmmDPMgr != NULL );

    IDE_TEST( findDPList( aSpaceID, &sSmrDPList ) != IDE_SUCCESS );
    if( sSmrDPList == NULL )
    {
        // 아직 한번도 SMM => SMR로 Dirty Page이동이 되지 않은 경우
        // Tablespace의 Dirty Page관리자가 없을 경우 새로 만들어준다.
        IDE_TEST( createDPList( aSpaceID ) != IDE_SUCCESS );

        IDE_TEST( findDPList( aSpaceID, & sSmrDPList ) != IDE_SUCCESS );

        IDE_ASSERT( sSmrDPList != NULL );
    }

    IDE_TEST( sSmrDPList->moveDirtyPagesFrom( sSmmDPMgr,
                                              aNewCnt,
                                              aDupCnt ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * moveDirtyPages4AllTBS 를 위한 Action인자
 */
typedef struct smrMoveDPArg
{
    sctStateSet mStateSet;
    ULong       mNewDPCount;
    ULong       mDupDPCount;
} smrMoveDPArg;

/*
 * moveDirtyPages4AllTBS 를 위한 Action함수
 */
IDE_RC smrDPListMgr::moveDPAction( idvSQL*            /*  aStatistics */,
                                   sctTableSpaceNode * aSpaceNode,
                                   void * aActionArg )
{
    UInt               sStage = 0;
    scPageID           sNewDPCount;
    scPageID           sDupDPCount;

    smrMoveDPArg     * sActionArg = (smrMoveDPArg *) aActionArg ;

    if ( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE )
    {
        // TBS상태가 DROP이나 OFFLINE으로 전이되지 않도록 보장
        IDE_TEST( sctTableSpaceMgr::latchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
        sStage = 1;

        if ( sctTableSpaceMgr::hasState(
                               aSpaceNode,
                               sActionArg->mStateSet) == ID_TRUE )
        {
            switch ( sActionArg->mStateSet )
            {
                case SCT_SS_SKIP_CHECKPOINT:
                    // Chkpt시에는 DROP/DISCARD/OFFLINE된 Tablespace라면
                    // 아무것도 하지 않는다.
                    break;
                case SCT_SS_UNABLE_MEDIA_RECOVERY:
                    // DROP/DISCARD된 Tablespace는 제외한다.
                    break;
                default:
                    IDE_ASSERT( 0 );
                    break;
            }
        }
        else
        {
            IDE_TEST( moveDirtyPages4TBS( aSpaceNode->mID,
                                          & sNewDPCount,
                                          & sDupDPCount )
                    != IDE_SUCCESS );

            sActionArg->mNewDPCount += sNewDPCount;
            sActionArg->mDupDPCount += sDupDPCount;
        }

        sStage = 0;
        IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1:
                IDE_ASSERT( sctTableSpaceMgr::unlatchSyncMutex(
                                aSpaceNode ) == IDE_SUCCESS );
            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}



/*
    모든 Tablespace의 Dirty Page를 SMM=>SMR로 이동한다.

    [IN]  aIsOnChkpt - Chkpt 또는 Media Recovery
    [OUT] aNewCnt    - 새로 추가된 Dirty Page 수
    [OUT] aDupCnt    - 기존에 존재하였던 Dirty Page 수
 */
IDE_RC smrDPListMgr::moveDirtyPages4AllTBS( sctStateSet aStateSet,
                                            ULong *     aNewCnt,
                                            ULong *     aDupCnt )
{

    smrMoveDPArg sActionArg;

    IDE_DASSERT( aNewCnt != NULL );
    IDE_DASSERT( aDupCnt != NULL );

    sActionArg.mStateSet   = aStateSet;
    sActionArg.mNewDPCount = 0;
    sActionArg.mDupDPCount = 0;

    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  moveDPAction,
                                                  (void*) & sActionArg,
                                                  SCT_ACT_MODE_NONE )
              != IDE_SUCCESS );

    *aNewCnt = sActionArg.mNewDPCount;
    *aDupCnt = sActionArg.mDupDPCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
    모든 Tablespace의 Dirty Page를 discard시킨다.
    미디어복구에서만 호출된다.
*/
IDE_RC smrDPListMgr::discardDirtyPages4AllTBS()
{
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  discardDPAction,
                                                  (void*)NULL,
                                                  SCT_ACT_MODE_NONE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * discardDirtyPages4AllTBS 를 위한 Action함수
 */
IDE_RC smrDPListMgr::discardDPAction( idvSQL            * /* aStatistics */,
                                      sctTableSpaceNode * aSpaceNode,
                                      void              * /* aActionArg */ )
{
    IDE_DASSERT( aSpaceNode != NULL );

    if ( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE )
    {
        // ONLINE/OFFLINE된 테이블스페이스에 대해서만 Media Recovery가
        // 가능하므로 다음 상태에 ASSERT 처리한다.
        if ( sctTableSpaceMgr::hasState( aSpaceNode,
                                         SCT_SS_UNABLE_MEDIA_RECOVERY )
             == ID_TRUE )
        {
            // 미디어복구가 불가능한 TBS의 경우는
            // discard할 dirty page가 존재하지 않는다.
        }
        else
        {
            IDE_TEST( discardDirtyPages4TBS( aSpaceNode->mID )
                    != IDE_SUCCESS );
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    특정 Tablespace의 강제로 Dirty Page를 제거한다.

    [IN] aSpaceID - Dirty Page를 제거하고자 하는 Tablespace의 ID
 */
IDE_RC smrDPListMgr::discardDirtyPages4TBS( scSpaceID   aSpaceID )
{
    smrDirtyPageList * sSmrDPList;

    IDE_TEST( findDPList( aSpaceID, &sSmrDPList ) != IDE_SUCCESS );

    if( sSmrDPList != NULL )
    {
        // SMR에 dirty page가 존재하는 경우
        sSmrDPList->removeAll( ID_TRUE );  // aIsForce
    }
    else
    {
        // SMR에 dirty page가 존재하지 않는 경우
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
