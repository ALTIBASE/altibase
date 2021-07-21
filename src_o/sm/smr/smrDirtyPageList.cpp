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
 * $Id: smrDirtyPageList.cpp 90522 2021-04-09 01:29:20Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smm.h>
#include <sct.h>
#include <smr.h>
#include <smrReq.h>

#define SMR_INIT_DIRTYPAGE_CNT (1000)

extern "C" SInt
smrComparePageGRID( const void* aElem1, const void* aElem2 )
{

    scSpaceID sSpaceID1 = SC_MAKE_SPACE(*(scGRID*)aElem1);
    scSpaceID sSpaceID2 = SC_MAKE_SPACE(*(scGRID*)aElem2);
    scPageID  sPageID1 = SC_MAKE_PID(*(scGRID*)aElem1);
    scPageID  sPageID2 = SC_MAKE_PID(*(scGRID*)aElem2);

    if ( sSpaceID1 > sSpaceID2 )
    {
        return 1;
    }
    else
    {
        if ( sSpaceID1 < sSpaceID2 )
        {
            return -1;
        }
    }
    
    if ( sPageID1 > sPageID2 )
    {
        return 1;
    }
    else
    {
        if ( sPageID1 < sPageID2 )
        {
            return -1;
        }
    }

    return 0;

}


smrDirtyPageList::smrDirtyPageList()
{

}

smrDirtyPageList::~smrDirtyPageList()
{

}

/*  Dirty Page관리자를 초기화한다.
  
    [IN] aSpaceID - 이 Dirty Page관리자가 관리할 Page들이 속한
                    Tablespace의 ID
 */
IDE_RC smrDirtyPageList::initialize( scSpaceID aSpaceID )
{
    mSpaceID                   = aSpaceID;
    
    mDirtyPageCnt              = 0;
    mFstPCH.m_pnxtDirtyPCH     = &mFstPCH;
    mFstPCH.m_pprvDirtyPCH     = &mFstPCH;

    mMaxDirtyPageCnt           = SMR_INIT_DIRTYPAGE_CNT;
    mArrPageGRID               = NULL;

    /* TC/FIT/Limit/sm/smr/smrDirtyPageList_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "smrDirtyPageList::initialize::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SMR,
                                ID_SIZEOF(scGRID) * mMaxDirtyPageCnt,
                                (void**)&mArrPageGRID) != IDE_SUCCESS,
                    insufficient_memory );
    
    /*
     * BUG-34125 Posix mutex must be used for cond_timedwait(), cond_wait().
     */
    IDE_TEST( mMutex.initialize((SChar*)"DIRTY_PAGE_LIST_MUTEX",
                               IDU_MUTEX_KIND_POSIX,
                               IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );
    
    IDE_TEST_RAISE(mCV.initialize((SChar *)"DIRTY_PAGE_LIST_COND")
                   != IDE_SUCCESS, err_cond_var_init);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_var_init );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrDirtyPageList::destroy()
{

    if(mArrPageGRID != NULL)
    {
        IDE_TEST( iduMemMgr::free((void*)mArrPageGRID)
                  != IDE_SUCCESS );
        mArrPageGRID = NULL;
    }

    IDE_TEST_RAISE(mCV.destroy() != IDE_SUCCESS, err_cond_var_destroy);
    
    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_destroy);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * 중복된 PID가 없는지 체크한다.
 *
 * return [OUT] 중복된 PID가 없으면 ID_TRUE
 */
idBool smrDirtyPageList::isAllPageUnique()
{
    UInt  i, j;

    idBool sIsUnique = ID_TRUE;
    
    for( i = 0; i < mDirtyPageCnt; i++ )
    {
        for ( j = 0; j < mDirtyPageCnt; j++ )
        {
            if ( i < j )
            {
                if( SC_GRID_IS_EQUAL(mArrPageGRID[i], mArrPageGRID[j]) )
                {
                    sIsUnique = ID_FALSE;
                    IDE_RAISE(finish);
                }
            }
        }
    }

    /* BUG-40385 sResult 값에 따라 Failure 리턴일 수 있으므로,
     * 위에 IDE_TEST_RAISE -> IDE_TEST_CONT 로 변환하지 않는다. */
    IDE_EXCEPTION_CONT(finish);
    
    return sIsUnique ;
}


/*
    Dirty Page ID들을 소팅하고 Page ID가 작은것부터 로깅한다.

    [ 로깅하는 이유 ]
      Dirty Page Flush는 Unstable DB에 수행하게 되는데,
      Dirty Page Flush도중 서버가 사망하게 되면,
      Unstable DB의 일부 Page가 잘못된 Page 이미지를 가지게 된다.

      이를 원복하기 위해 Restart Redo시 Dirty Page ID를 Log로부터 읽어서
      Stable DB로부터 Unstable DB로 해당 Page들을 복구하게 된다.

    [동시성 제어]
       TBSNode.SyncMutex를 잡는다.
       - Tablespace를 Drop/Offline시키려는 Tx와의 동시성 제어를 위함
 */
IDE_RC smrDirtyPageList::writePIDLogs()
{
    scGRID*              sArrPageGRID;
    ULong                sBuffer[SM_PAGE_SIZE / ID_SIZEOF(ULong)];
    scSpaceID            sSpaceID;
    scPageID             sPageID;
    smmPCH*              sCurPCHPtr = NULL;
    UInt                 sDirtyStat;
    vULong               sDPageCnt = 0;
    vULong               sTotalCnt;

    // 저장될 scGRID의 수보다 버퍼의 크기가 작아서는 안된다.
    IDE_DASSERT( SM_PAGE_SIZE >=
                 ((MAX_PAGE_INFO * ID_SIZEOF(scGRID)) + SMR_DEF_LOG_SIZE) );

    sArrPageGRID  = (scGRID*)( ((smrLogHead*)sBuffer) + 1 );

    if(mDirtyPageCnt != 0)
    {
        sTotalCnt = 0;

        /*
          데이타베이스 페이지가 Physical Disk에 페이지 ID순서로 순차적으로 Wirte되도록
          페이지 ID로 Sorting.. 이렇게 함으로써 Restart Reload시 순차적으로 Loading된다.
        */

        idlOS::qsort((void*)mArrPageGRID,
                     mDirtyPageCnt,
                     ID_SIZEOF(scGRID),
                     smrComparePageGRID);

        while(sTotalCnt != mDirtyPageCnt)
        {
            sSpaceID = SC_MAKE_SPACE(mArrPageGRID[sTotalCnt]);
            sPageID = SC_MAKE_PID(mArrPageGRID[sTotalCnt]);

            sCurPCHPtr = smmManager::getPCH( sSpaceID, sPageID );

            sDirtyStat = sCurPCHPtr->m_dirtyStat & SMM_PCH_DIRTY_STAT_MASK;
            IDE_ASSERT(sDirtyStat != SMM_PCH_DIRTY_STAT_INIT);

            if (sDirtyStat != SMM_PCH_DIRTY_STAT_INIT)
            {
                SC_COPY_GRID(mArrPageGRID[sTotalCnt], sArrPageGRID[sDPageCnt]);
                sDPageCnt++;

                if(sDPageCnt >= MAX_PAGE_INFO)
                {
                    IDE_TEST( writePIDLogRec( (SChar*) sBuffer, sDPageCnt )
                              != IDE_SUCCESS );

                    sDPageCnt = 0;
                }
            }

            sTotalCnt++;
        }

        if(sDPageCnt != 0)
        {

            IDE_TEST( writePIDLogRec( (SChar*) sBuffer, sDPageCnt )
                      != IDE_SUCCESS );

            sDPageCnt = 0;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Page ID Array가 기록된 Log Buffer를 Log Record로 기록한다.

    [IN] aLogBuffer - 로그버퍼
                        Head, Tail : 초기화 안된상태
                        Body : Dirty Page GRID가 기록되어 있음

    [IN] aDirtyPageCount - 로그 버퍼의 Body에 기록된 Page의 갯수 
 */
IDE_RC smrDirtyPageList::writePIDLogRec(SChar * aLogBuffer,
                                        UInt    aDirtyPageCount)
{
    smrLogHead * sLogHeadPtr = (smrLogHead*)aLogBuffer;

    IDE_DASSERT( aLogBuffer != NULL );
    IDE_DASSERT( aDirtyPageCount != 0 );

    smrLogHeadI::setType(sLogHeadPtr, SMR_LT_DIRTY_PAGE);
    smrLogHeadI::setTransID(sLogHeadPtr, SM_NULL_TID);
    smrLogHeadI::setSize(sLogHeadPtr,
                         ID_SIZEOF(smrLogHead) +
                         ID_SIZEOF(scGRID) * aDirtyPageCount +
                         ID_SIZEOF(smrLogTail));

    smrLogHeadI::setPrevLSN(sLogHeadPtr,
                            ID_UINT_MAX,
                            ID_UINT_MAX);

    smrLogHeadI::setFlag(sLogHeadPtr, SMR_LOG_TYPE_NORMAL);

    smrLogHeadI::copyTail( aLogBuffer + smrLogHeadI::getSize(sLogHeadPtr)
                                      - ID_SIZEOF(smrLogTail),
                           sLogHeadPtr);

    IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                   NULL,
                                   (SChar*)aLogBuffer,
                                   NULL,  // Previous LSN Ptr
                                   NULL,  // Log LSN Ptr
                                   NULL,  // End LSN Ptr
                                   SM_NULL_OID )
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
     Dirty Page들을 Checkpoint Image에 Write한다.

     [ Latch간 Deadlock회피를 위한 Latch잡는 순서 ]
       1. sctTableSpaceMgr::lock()           // TBS LIST
       2. sctTableSpaceMgr::latchSyncMutex() // TBS NODE
       3. smmPCH.mMutex.lock()               // PAGE

     [동시성 제어]
        TBSNode.SyncMutex를 잡은채로 이 함수를 호출해야 한다.
        - Tablespace를 Drop/Offline시키려는 Tx와의 동시성 제어를 위함
 */
IDE_RC smrDirtyPageList::writeDirtyPages(
                         smmTBSNode                * aTBSNode,
                         smmGetFlushTargetDBNoFunc   aGetFlushTargetDBNoFunc,
                         idBool                      aIsFinalWrite,
                         UInt                        aTotalCnt,
                         UInt                      * aWriteCnt,
                         UInt                      * aRemoveCnt,
                         ULong                     * aWaitTime,
                         ULong                     * aSyncTime )
{
    SInt                 sWhichDB;
    scSpaceID            sSpaceID;
    scPageID             sPageID;
    smmPCH*              sCurPCHPtr = NULL;
    smPCSlot           * sPCHSlot = NULL;
    UInt                 sDirtyPageCnt;
    UInt                 sWriteCnt  = 0;
    UInt                 sRemoveCnt = 0;
    ULong                sWaitTime  = 0;  /* microsecond */
    ULong                sSyncTime  = 0;  /* microsecond */
    UInt                 sDirtyStat;
    UInt                 sStage     = 0;
    PDL_Time_Value       sTV;
    PDL_Time_Value       sAddTV;
    IDE_RC               sRC;
    UInt                 sWritePageCnt = 0;
    UInt                 sSleepSec;
    UInt                 sSleepUSec;
    UInt                 sDPMutexState;
    UInt                 sPropSyncPageCount;
    idvTime              sBeginTime;
    /* LSN을 바탕으로 Sync한 Log의 크기를 알 수 있다. */
    idvTime              sEndTime;

    
    IDE_DASSERT( aTBSNode   != NULL );
    IDE_DASSERT( aWriteCnt  != NULL );
    IDE_DASSERT( aRemoveCnt != NULL );
    IDE_DASSERT( aWaitTime  != NULL );
    IDE_DASSERT( aSyncTime  != NULL );
    IDE_DASSERT( aGetFlushTargetDBNoFunc != NULL );
    
    sDPMutexState     = 0;
    sSleepSec  = 0;
    sSleepUSec = 0;
    sPropSyncPageCount = smuProperty::getChkptBulkSyncPageCount();

    sDirtyPageCnt = mDirtyPageCnt;

    // 새로 Flush할 Ping Pong DB계산 
    // 1. 서버 운영중에는 
    // smmManager::getNxtStableDB
    // 2. 미디어 복구시에는 
    // smmManager::getCurrentDB
    sWhichDB = aGetFlushTargetDBNoFunc( aTBSNode );
    
    while(sWriteCnt != sDirtyPageCnt)
    {
        sSpaceID = SC_MAKE_SPACE(mArrPageGRID[sWriteCnt]);
        IDE_ASSERT( sSpaceID == mSpaceID );
        
        sPageID = SC_MAKE_PID(mArrPageGRID[sWriteCnt]);

        sPCHSlot = smmManager::getPCHSlot(sSpaceID, sPageID);
        sCurPCHPtr = (smmPCH*)sPCHSlot->mPCH;

        sDirtyStat = sCurPCHPtr->m_dirtyStat & SMM_PCH_DIRTY_STAT_MASK;
        IDE_ASSERT( sDirtyStat != SMM_PCH_DIRTY_STAT_INIT );
            
        // Free Page의 페이지 메모리를  반납하려는 Thread와
        // Dirty Page Flush하려는 Thread간의 동시성 제어
        
        IDE_TEST( sCurPCHPtr->mMutex.lock( NULL /* idvSQL* */ )
                  != IDE_SUCCESS );
        sStage = 1;
        
        {
            
            /* --------------------------------------------
             * 1. dirty flag turn off
             * -------------------------------------------- */
            sCurPCHPtr->m_dirty = ID_FALSE;
            
            // m_dirty 플래그가 ID_TRUE이면 smmDirtyPageMgr에 insDirtyPage를
            // 호출하여도 Dirty Page로 추가되지 않는다.
            // 만약 m_dirty 플래그가 ID_TRUE인 채로 Page가 Flush되면,
            // Page가 Flush되는 도중에 Dirty Page추가 시도한 것은
            // Dirty Page로 추가조차 되지 않게 된다.
            // 그러므로 아래 작업(Page를 디스크로 Flush )이 수행되기 전에
            // 반드시 m_dirty 가 ID_FALSE로 세팅되어야 한다.
            IDL_MEM_BARRIER;
            
            // 페이지 메모리가 남아있는 경우에만 Flush 실시.
            if ( sPCHSlot->mPagePtr != NULL )
            {
                IDE_TEST( writePageImage( aTBSNode,
                                          sWhichDB,
                                          sPageID ) != IDE_SUCCESS );
                
            }

            /* FIT/ART/sm/Bugs/BUG-13894/BUG-13894.sql */
            IDU_FIT_POINT( "1.BUG-13894@smrDirtyPageList::writeDirtyPages" );
            
            // To Fix BUG-9366
            sWritePageCnt++;
            
            if ((smuProperty::getChkptBulkWritePageCount() == sWritePageCnt) &&
                (aIsFinalWrite == ID_FALSE))
            {
                sSleepSec  = smuProperty::getChkptBulkWriteSleepSec();
                sSleepUSec = smuProperty::getChkptBulkWriteSleepUSec();
                
                if ((sSleepSec != 0) || (sSleepUSec != 0))
                {
                    /* BUG-32670    [sm-disk-resource] add IO Stat information 
                     * for analyzing storage performance.
                     * Microsecond로 변환해 총 WaitTime을 계산함. */
                    sWaitTime += sSleepSec * 1000 * 1000 + sSleepUSec;
                    sTV.set(idlOS::time(NULL),0);
                    
                    sAddTV.set((sSleepSec + (sSleepUSec/1000000)),
                               (sSleepUSec%1000000));
                    
                    sTV += sAddTV;
                    
                    IDE_TEST( lock() != IDE_SUCCESS );
                    sDPMutexState = 1;
                    
                    while ( 1 )
                    {
                        sDPMutexState = 0;
                        
                        sRC = mCV.timedwait(&mMutex, &sTV);
                        sDPMutexState = 1;
                        
                        if ( sRC != IDE_SUCCESS )
                        {
                            IDE_TEST_RAISE(mCV.isTimedOut() != ID_TRUE, err_cond_wait);
                            
                            errno = 0;
                            break;
                        }
                    }
                    
                    sDPMutexState = 0;
                    IDE_TEST( unlock() != IDE_SUCCESS );
                }
                sWritePageCnt = 0;
            }
            
        }
        
        sStage = 0;
        IDE_TEST( sCurPCHPtr->mMutex.unlock() != IDE_SUCCESS );
        
        if(sDirtyStat == SMM_PCH_DIRTY_STAT_REMOVE)
        {
            remove( sCurPCHPtr );
            
            SC_MAKE_GRID(mArrPageGRID[sWriteCnt],
                         SC_MAX_SPACE_COUNT,
                         SC_MAX_PAGE_COUNT,
                         SC_MAX_OFFSET);
            
            sRemoveCnt++;
        }
        else
        {
            sCurPCHPtr->m_dirtyStat = SMM_PCH_DIRTY_STAT_REMOVE;
        }
        sWriteCnt++;

        // 프로퍼티에 지정된 Sync Page 개수만큼 Write Page가 
        // 되면 Sync를 수행한다.
        // 단, 프로퍼티 값이 0이면 여기에서 DB Sync하지 않고
        // 모든 page를 모두 write하고 checkpoint 마지막에 한번만 Sync한다.
        if( (sPropSyncPageCount > 0) &&
            (((aTotalCnt + sWriteCnt) % sPropSyncPageCount) == 0) )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_MRECOVERY_CHKP_FLUSH_SYNC_BEGIN, 
                         sPropSyncPageCount, sWriteCnt + aTotalCnt );

            /* BUG-32670    [sm-disk-resource] add IO Stat information 
             * for analyzing storage performance.
             * Sync하는데 걸린 시간을 계산함. */
            IDV_TIME_GET(&sBeginTime);
            IDE_TEST( smmManager::syncDB(
                         SCT_SS_SKIP_CHECKPOINT, 
                         ID_FALSE /* synclatch 획득할 필요없음 */) 
                      != IDE_SUCCESS );

            IDV_TIME_GET(&sEndTime);
            sSyncTime +=  IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_MRECOVERY_CHKP_FLUSH_SYNC_END );
        }
    } // while(sWriteCnt != sDirtyPageCnt)

    IDE_ASSERT(sWriteCnt == sDirtyPageCnt);
    
    *aWriteCnt  = sWriteCnt;
    *aRemoveCnt = sRemoveCnt;
    *aWaitTime  = sWaitTime;
    *aSyncTime  = sSyncTime;

    idlOS::qsort((void*)mArrPageGRID,
                 sDirtyPageCnt,
                 ID_SIZEOF(scGRID),
                 smrComparePageGRID);

    // 중복된 PID가 없음을 확인한다.
    IDE_DASSERT( isAllPageUnique() == ID_TRUE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_wait );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }

    IDE_EXCEPTION_END;

    if (sDPMutexState != 0)
    {
        (void)unlock();
    }

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1 :
                IDE_ASSERT( sCurPCHPtr->mMutex.unlock()
                            == IDE_SUCCESS );

                break;
            default:
                break;
        }
    }
    IDE_POP();
        
    return IDE_FAILURE;
    
    
}

/*
   Page Image를 Checkpoint Image에 기록한다.
   
   [IN] aTBSNode - Page가 속한 Tablespace
   [IN] aUnstableDBNum - 0 or 1 => Page Image를 기록할 Ping Pong번호 
   [IN] aPageID  - 기록하려는 PageID
 */
IDE_RC smrDirtyPageList::writePageImage( smmTBSNode * aTBSNode,
                                         SInt         aUnstableDBNum,
                                         scPageID     aPageID )
{
    UInt                        sDBFileNo;
    smmDatabaseFile           * sDBFilePtr;
    smmChkptImageHdr            sChkptImageHdr;
    smiDataFileDescSlotID     * sDataFileDescSlotID;
    smiDataFileDescSlotID     * sAllocDataFileDescSlotID;
    idBool                      sResult;
    smmChkptImageAttr           sChkptImageAttr; 
    SInt                        sCurrentDB;
    smmDatabaseFile           * sCurrentDBFilePtr;
    


    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( ( aUnstableDBNum == 0 ) || ( aUnstableDBNum == 1 ));
    
    /* --------------------------------------------
     * (010) get memory object of that database file
     * -------------------------------------------- */
    sDBFileNo = smmManager::getDbFileNo(aTBSNode, aPageID);
    
    IDE_TEST( smmManager::getDBFile( aTBSNode,
                                     aUnstableDBNum,
                                     sDBFileNo,
                                     SMM_GETDBFILEOP_NONE,
                                     &sDBFilePtr )
              != IDE_SUCCESS );
    
    /* ------------------------------------------------
     * (020) if the db file doesn't create, create and open
     * ------------------------------------------------ */

    if( smmManager::getCreateDBFileOnDisk( aTBSNode,
                                           aUnstableDBNum,
                                           sDBFileNo ) == ID_FALSE )
    {
        // BUG-46574 Checkpoint Image 생성도중에 비정상 종료 혹은 디스크 full 등으로 
        // 헤더만 쓰고 정상적인 이미지를 내려쓰지 못하면 
        // 디스크에는 파일이 존재하지만 TBSNode및 로그앵커에는 없는 파일이 됨
        // 오류를 반환하지 않고 삭제 후 재생성한다.
        if ( sDBFilePtr->isOpen() == ID_TRUE )
        {
            (void)sDBFilePtr->close();
        }
        (void)idf::unlink( sDBFilePtr->getFileName() );
        
        if ( sDBFileNo > aTBSNode->mLstCreatedDBFile )
        {
            aTBSNode->mLstCreatedDBFile = sDBFileNo;
        }

        // 파일이 생성되면서 DBF Hdr가 기록된다. 
        IDE_TEST( sDBFilePtr->createDbFile( aTBSNode,
                                            aUnstableDBNum,
                                            sDBFileNo,
                                            0/* DB File Header만 기록*/)
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_ASSERT( sDBFilePtr->isOpen() == ID_TRUE );
    }

    /* PROJ-2133 Incremental backup */
    if( smrRecoveryMgr::isCTMgrEnabled() == ID_TRUE )
    {
        sDBFilePtr->getChkptImageHdr( &sChkptImageHdr );

        sDataFileDescSlotID = &sChkptImageHdr.mDataFileDescSlotID;

        IDE_TEST_CONT( smriChangeTrackingMgr::isAllocDataFileDescSlotID(
                                                        sDataFileDescSlotID ) 
                       != ID_TRUE, skip_change_tracking );

        IDE_TEST( smriChangeTrackingMgr::isNeedAllocDataFileDescSlot( 
                                                          sDataFileDescSlotID,
                                                          sChkptImageHdr.mSpaceID,
                                                          sDBFileNo,
                                                          &sResult )
                  != IDE_SUCCESS );

        if( sResult == ID_TRUE )
        {
            IDE_TEST( smriChangeTrackingMgr::addDataFile2CTFile( 
                                                          sChkptImageHdr.mSpaceID,
                                                          sDBFileNo,
                                                          SMRI_CT_MEM_TBS,
                                                          &sAllocDataFileDescSlotID )
                      != IDE_SUCCESS );

            sDBFilePtr->setChkptImageHdr( NULL, // aMemRedoLSN
                                          NULL, // aMemCreateLSN
                                          NULL, // spaceID
                                          NULL, // smVersion
                                          sAllocDataFileDescSlotID );

            sCurrentDB = smmManager::getCurrentDB(aTBSNode);

            /* chkptImage에 대한 DataFileDescSlot의 재할당은 restart
             * recovery시에만 수행된다. restart recovery시 checkpoint는
             * nextStableDB에 수행되고 media recovery시 checkpoint는 currentDB에
             * 수행된다. 즉,sCurrentDB와 aUnstableDBNum가 달라야한다.
             */
            IDE_DASSERT( sCurrentDB != aUnstableDBNum );

            IDE_TEST( smmManager::getDBFile( aTBSNode,
                                             sCurrentDB,
                                             sDBFileNo,
                                             SMM_GETDBFILEOP_NONE,
                                             &sCurrentDBFilePtr )
                      != IDE_SUCCESS );

            sCurrentDBFilePtr->setChkptImageHdr( NULL, // aMemRedoLSN
                                                 NULL, // aMemCreateLSN
                                                 NULL, // spaceID
                                                 NULL, // smVersion
                                                 sAllocDataFileDescSlotID );

            sDBFilePtr->getChkptImageAttr( aTBSNode, &sChkptImageAttr ); 

            IDE_ASSERT( smrRecoveryMgr::updateChkptImageAttrToAnchor(
                                               &(aTBSNode->mCrtDBFileInfo[ sDBFileNo ]),
                                               &sChkptImageAttr )
                        == IDE_SUCCESS );
        }
        
        IDE_EXCEPTION_CONT( skip_change_tracking );
    }

    /* --------------------------------------------
     * (030) write dirty page to database file
     * -------------------------------------------- */
    IDE_TEST( writePageNormal( aTBSNode,
                               sDBFilePtr,
                               aPageID ) != IDE_SUCCESS );

    return IDE_SUCCESS ;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

IDE_RC smrDirtyPageList::writePageNormal( smmTBSNode       * aTBSNode,
                                          smmDatabaseFile  * aDBFilePtr,
                                          scPageID           aPID)
{
    UInt                 sState = 0;
    SInt                 sSystemErrno;

    while(1)
    {
        /* ------------------------------------------------
         *  errno를 초기화 한다. 에러를 판단하기 위해서임.
         * ----------------------------------------------*/
        if( aPID == 0 )
        {
            IDE_TEST( smmManager::lockBasePage(aTBSNode) != IDE_SUCCESS );
            sState = 1;
        }

        errno = 0;
        if ( aDBFilePtr->writePage(aTBSNode, aPID) == IDE_SUCCESS )
        {
            if ( sState == 1 )
            {
                sState = 0;
                IDE_TEST( smmManager::unlockBasePage(aTBSNode)
                          != IDE_SUCCESS );
            }

            break;
        }

        if ( sState == 1 )
        {
            sState = 0;
            IDE_TEST( smmManager::unlockBasePage(aTBSNode) != IDE_SUCCESS );
        }

        sSystemErrno = ideGetSystemErrno();

        /* ------------------------------------------------
         *  Write에서 에러가 발생한 상황임
         * ----------------------------------------------*/
        IDE_TEST( (sSystemErrno != 0) && (sSystemErrno != ENOSPC) );

        while ( idlVA::getDiskFreeSpace(aDBFilePtr->getDir())
                < (SLong)SM_PAGE_SIZE )
        {
            smLayerCallback::setEmergency( ID_TRUE );
            if ( smrRecoveryMgr::isFinish() == ID_TRUE )
            {
                ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                            SM_TRC_MRECOVERY_DIRTY_PAGE_LIST_FATAL1);
                IDE_ASSERT(0);
            }
            else
            {
                ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                            SM_TRC_MRECOVERY_DIRTY_PAGE_LIST_WARNNING);
                idlOS::sleep(2);
            }
        }

        smLayerCallback::setEmergency( ID_FALSE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState == 1)
    {
        IDE_ASSERT( smmManager::unlockBasePage(aTBSNode) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

void smrDirtyPageList::removeAll( idBool aIsForce )
{

    smmPCH*  sLstPCHPtr;
    smmPCH*  sCurPCHPtr;

    sCurPCHPtr = mFstPCH.m_pnxtDirtyPCH;

    while( sCurPCHPtr != &mFstPCH )
    {
        if ( aIsForce == ID_FALSE )
        {
            // 일반적인 경우에는 제거되는 경우 PCH의 상태가 REMOVE 여야한다. 
            IDE_ASSERT((sCurPCHPtr->m_dirtyStat & SMM_PCH_DIRTY_STAT_MASK)
                       == SMM_PCH_DIRTY_STAT_REMOVE);
        }
        else
        {
            // 강제로 제거하는 경우에는 검사하지 않는다. 
        }

        sCurPCHPtr->m_dirty = ID_FALSE;
        sCurPCHPtr->m_dirtyStat = SMM_PCH_DIRTY_STAT_INIT;

        sLstPCHPtr = sCurPCHPtr;

        sCurPCHPtr->m_pnxtDirtyPCH->m_pprvDirtyPCH
            = sCurPCHPtr->m_pprvDirtyPCH;
        sCurPCHPtr->m_pprvDirtyPCH->m_pnxtDirtyPCH
            = sCurPCHPtr->m_pnxtDirtyPCH;
        sCurPCHPtr = sCurPCHPtr->m_pnxtDirtyPCH;

        sLstPCHPtr->m_pnxtDirtyPCH = NULL;
        sLstPCHPtr->m_pprvDirtyPCH = NULL;

        mDirtyPageCnt--;
    }

    IDE_ASSERT(mDirtyPageCnt == 0);
    IDE_ASSERT(mFstPCH.m_pnxtDirtyPCH == &mFstPCH);
    IDE_ASSERT(mFstPCH.m_pprvDirtyPCH == &mFstPCH);

    return;
}

/*
   SMM Dirty Page Mgr로부터 Dirty Page들을 가져온다
   
   [IN]  aSmmDPMgr - SMM Dirty Page Mgr
   [OUT] aNewCnt   - 새로 추가된 Dirty Page 수
   [OUT] aDupCnt   - 기존에 존재하였던 Dirty Page 수

   [동시성제어]
      TBS.SyncMutex가 잡힌 채로 이 함수가 호출되어야 한다.
      
      이유 : TBS.SyncMutex가 잡힌동안 TBS가 DROP/OFFLINE으로 전이되지
             않음을 보장할 수 있기 때문
             smrDPListMgr::writeDirtyPageAction 주석 참고 
 */
IDE_RC smrDirtyPageList::moveDirtyPagesFrom( smmDirtyPageMgr * aSmmDPMgr,
                                             UInt            * aNewCnt,
                                             UInt            * aDupCnt)
{
    UInt               i;
    UInt               sDirtyPageListCount;
    smmDirtyPageList * sDirtyPageList;

    smPCSlot         * sPCHSlot;
    smmPCH           * sPCH;
    idBool             sIsPCHLocked = ID_FALSE;
    idBool             sIsCount = ID_TRUE;
    UInt               sDirtyStat;
    UInt               sNewCnt    = 0;
    UInt               sDupCnt    = 0;
    
    IDE_DASSERT( aSmmDPMgr != NULL );
    IDE_DASSERT( aNewCnt   != NULL );
    IDE_DASSERT( aDupCnt   != NULL );
    
    sDirtyPageListCount = aSmmDPMgr->getDirtyPageListCount();
    for(i = 0; i < sDirtyPageListCount; i++)
    {
        sDirtyPageList = aSmmDPMgr->getDirtyPageList(i);
        IDE_TEST( sDirtyPageList->open() != IDE_SUCCESS );

        while(1)
        {
            IDE_TEST( sDirtyPageList->read( &sPCHSlot ) != IDE_SUCCESS );
            if( sPCHSlot == NULL )
            {
                break;
            }
            sPCH = (smmPCH*)sPCHSlot->mPCH;
            IDE_ASSERT(sPCH != NULL);

            if (((sPCH->m_dirtyStat & SMM_PCH_DIRTY_STAT_MASK)
                 == SMM_PCH_DIRTY_STAT_FLUSHDUP) ||
                ((sPCH->m_dirtyStat & SMM_PCH_DIRTY_STAT_MASK)
                 == SMM_PCH_DIRTY_STAT_FLUSH))
            {
                sIsCount = ID_FALSE;
            }
            else
            {
                sIsCount = ID_TRUE;
            }
            
            // Free Page 메모리가 언제든지 반납되어 m_page 가 NULL일 수가 있다.
            // 이미 반납된 메모리라면 Dirty시킬 필요가 없다.
            // 주의 ! 코드가 변경되어 lock/unlock 사이에서
            //       에러 발생하게 되면 stage처리 해야함
            IDE_TEST( sPCH->mMutex.lock( NULL /* idvSQL* */ )
                      != IDE_SUCCESS );
            
            sIsPCHLocked = ID_TRUE;

            if ( sPCHSlot->mPagePtr != NULL ) // Dirty되었다가 이미 Free된 Page
            {
//                IDE_ASSERT( smLayerCallback::getPersPageID(sPCH->m_page) <
//                            smmManager::getDBMaxPageCount() );

                add( sPCH,
                     smLayerCallback::getPersPageID( sPCHSlot->mPagePtr ) );
            }
            
            sIsPCHLocked = ID_FALSE;
            IDE_TEST( sPCH->mMutex.unlock() != IDE_SUCCESS );
            
            sDirtyStat = sPCH->m_dirtyStat & SMM_PCH_DIRTY_STAT_MASK;

            if ((sIsCount == ID_TRUE) && (sDirtyStat == SMM_PCH_DIRTY_STAT_FLUSHDUP))
            {
                sDupCnt++;
            }
            else if (sDirtyStat == SMM_PCH_DIRTY_STAT_FLUSH)
            {
                sNewCnt++;
            }
        }

        IDE_TEST( sDirtyPageList->close() != IDE_SUCCESS );
    }

    *aNewCnt = sNewCnt;
    *aDupCnt = sDupCnt;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    IDE_PUSH();
    {
        if ( sIsPCHLocked == ID_TRUE )
        {
            IDE_ASSERT( sPCH->mMutex.unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();
    
    return IDE_FAILURE;
}

