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
 * $Id: smrFT.cpp 36981 2009-11-26 08:31:03Z mycomman $
 *
 * Description :
 *
 * 본 파일은 백업 관리자에 대한 구현파일이다.
 *
 **********************************************************************/

#include <smErrorCode.h>
#include <smDef.h>
#include <smm.h>
#include <smr.h>
#include <smrReq.h>
#include <sdd.h>
#include <sct.h>
#include <smi.h>


/*********************************************************
 * Description: buildRecordForStableMemDataFiles
  - 메모리 테이블 스페이스의 stable DB 데이타 파일 목록을
    보여주는 performance view 의 build 함수.

 *********************************************************/
IDE_RC
smrFT::buildRecordForStableMemDataFiles(idvSQL	    * /* aStatistics */,
                                        void        *aHeader,
                                        void        * /* aDumpObj */,
                                        iduFixedTableMemory *aMemory)
{
    UInt                    sDiskMemDBFileCount = 0;
    UInt                    sWhichDB;
    UInt                    i;
    UInt                    sState      = 0;
    smmDatabaseFile       * sDatabaseFile;
    smuList                 sDataFileLstBase;
    smuList               * sNode;
    smuList               * sNextNode;
    smmTBSNode            * sCurTBS;
    smrStableMemDataFile  * sStableMemDataFile;

    /* 예외에서 사용하기 때문에 인자검사 이전에 초기화 되어야 함 */
    SMU_LIST_INIT_BASE(&sDataFileLstBase);

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurTBS != NULL )
    {
        /* BUG-44816: X$STABLE_MEM_DATAFILES를 만드려고 시도 할 때,
         * 변경중인 TBS를 접근해서 세그멘테이션 폴트를 발생시킬 수 있다.
         * ( ONLINE, OFFLINE 변경은 TBS노드를 해제하거나 데이터 파일을
         * 삭제 하지 않으므로 고려하지 않음.)  
         */
        if( ( sctTableSpaceMgr::isMemTableSpace( sCurTBS ) != ID_TRUE ) ||
            ( sCurTBS->mRestoreType == SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET ) ||
            ( SMI_TBS_IS_INCOMPLETE( sCurTBS->mHeader.mState ) ))
        {
            sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
            continue;
        }
        sWhichDB = sCurTBS->mTBSAttr.mMemAttr.mCurrentDB;

        /* ------------------------------------------------
         * [1] stable한 memory database file 목록을 만든다.
         * ----------------------------------------------*/
        /*
         * BUG-24163 smrBackupMgr::buildRecordForStableMemDataFiles에서
         *  DB파일목록을 만들때 파일갯수보다 한개더 검사합니다.
         */
        for(i = 0,sDiskMemDBFileCount=0 ;
            i < sCurTBS->mMemBase->mDBFileCount[sWhichDB];
            i++)
        {
            if( smmManager::getDBFile(sCurTBS,
                                      sWhichDB,
                                      i,
                                      SMM_GETDBFILEOP_SEARCH_FILE,
                                      &sDatabaseFile)
                != IDE_SUCCESS)
            {

                ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                            SM_TRC_MRECOVERY_BACKUP_DBFILE_MISMATCH,
                            sWhichDB,
                            i);

                // memory 상에는 존재하나, 실제 disk 에 없는 경우임.
                // skip 한다.
                continue;
            }

            IDE_ERROR(sDatabaseFile != NULL);

            /* TC/Server/LimitEnv/Bugs/BUG-24163/BUG-24163_1.sql */
            /* IDU_FIT_POINT_RAISE( "smrFT::buildRecordForStableMemDataFiles::malloc",
                                    insufficient_memory ); */
            /* [TODO] immediate로 변경할것. */

            IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SMR,
                                        ID_SIZEOF(smrStableMemDataFile),
                                        (void**)&sStableMemDataFile,
                                        IDU_MEM_FORCE)
                      != IDE_SUCCESS );
            sState = 1;

            SMU_LIST_INIT_NODE(&(sStableMemDataFile->mDataFileNameLst));
            sStableMemDataFile->mSpaceID        = sCurTBS->mHeader.mID;
            sStableMemDataFile->mDataFileName   = sDatabaseFile->getFileName();
            sStableMemDataFile->mDataFileNameLst.mData  = sStableMemDataFile;

            SMU_LIST_ADD_LAST(&sDataFileLstBase,
                              &(sStableMemDataFile->mDataFileNameLst));
            sDiskMemDBFileCount++;
        }

        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    /* ------------------------------------------------
     * [2] stable한 memory database file fixed table record set을
     * 만든다.
     * ----------------------------------------------*/

    // [3] build record.
    for(sNode = SMU_LIST_GET_FIRST(&sDataFileLstBase), i =0;
        sNode != &sDataFileLstBase;
        sNode = SMU_LIST_GET_NEXT(sNode),i++)
    {
        sStableMemDataFile=(smrStableMemDataFile*)sNode->mData;
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)sStableMemDataFile )
             != IDE_SUCCESS);
    }

    // de-alloc memory.
    for(sNode = SMU_LIST_GET_FIRST(&sDataFileLstBase);
        sNode != &sDataFileLstBase;
        sNode = sNextNode )
    {
        sNextNode = SMU_LIST_GET_NEXT(sNode);
        SMU_LIST_DELETE(sNode);
        IDE_ASSERT(iduMemMgr::free(sNode->mData) == IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    //IDE_EXCEPTION( insufficient_memory );
    //{
    //    IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    //}
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            if( sDiskMemDBFileCount != 0 )
            {
                // de-alloc memory.
                for(sNode = SMU_LIST_GET_FIRST(&sDataFileLstBase);
                    sNode != &sDataFileLstBase;
                    sNode = sNextNode )
                {
                    sNextNode = SMU_LIST_GET_NEXT(sNode);
                    SMU_LIST_DELETE(sNode);
                    IDE_ASSERT(iduMemMgr::free(sNode->mData) == IDE_SUCCESS);
                } // end of for
            }
            else
            {
                IDE_ASSERT( iduMemMgr::free( sStableMemDataFile ) == IDE_SUCCESS );
                sStableMemDataFile = NULL;
            }
        default:
            break;
    }

    return IDE_FAILURE;
}

/*********************************************************
 * Description: archive log list 수행
 * 로그모드에 관한 정보와 데이터베이스 archive 상태에 대한
 * 다음과같은 정보를 제공한다.
 *
 * - Database log mode
 *   : 데이타베이스 archivelog 모드 출력
 * - Archive Thread Activated
 *   : archivelog thread가 활성화상태인지 비활성화상태인지 출력
 * - Archive destination directory
 *   : archive log directory 출력
 * - Oldest online log sequence
 *   : 삭제되지 않은 online logfile 중 가장 오랜된 logfile 번호 출력
 * - Next log sequence to archive
 *   : archive log list중 다음 archive 할 logfile 번호 출력
 * - Current log sequence
 *   : 현재 로그파일 번호 출력
 *********************************************************/
IDE_RC smrFT::buildRecordForArchiveInfo(idvSQL              * /*aStatistics*/,
                                        void        *aHeader,
                                        void        * /* aDumpObj */,
                                        iduFixedTableMemory *aMemory)
{

    smrArchiveInfo     sArchiveInfo;
    idBool             sEmptyArchLFLst;
    UInt               sLstDeleteFileNo;
    UInt               sCurLogFileNo;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    idlOS::memset(&sArchiveInfo,0x00, ID_SIZEOF(smrArchiveInfo));

    smrRecoveryMgr::getLogAnchorMgr()->getLstDeleteLogFileNo( &sLstDeleteFileNo );

    smrLogMgr::getLogFileMgr().getCurLogFileNo( &sCurLogFileNo );

    /*
       Archive Info를 생성하고 Fixed Table Record를 구성한다.
    */
    sArchiveInfo.mArchiveMode= smrRecoveryMgr::getArchiveMode();
    sArchiveInfo.mArchThrRunning = smrLogMgr::getArchiveThread().isStarted();

    sArchiveInfo.mArchDest = smuProperty::getArchiveDirPath();
    sArchiveInfo.mOldestActiveLogFile = sLstDeleteFileNo;
    sArchiveInfo.mCurrentLogFile = sCurLogFileNo;

    if ( sArchiveInfo.mArchiveMode == SMI_LOG_ARCHIVE)
    {
        IDE_TEST( smrLogMgr::getArchiveThread().
                  getArchLFLstInfo( &(sArchiveInfo.mNextLogFile2Archive), &sEmptyArchLFLst )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }

    // build record for fixed table.
    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        (void *) &(sArchiveInfo))
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static iduFixedTableColDesc  gArchiveTableColDesc[]=
{

    {
        (SChar*)"ARCHIVE_MODE",
        offsetof(smrArchiveInfo, mArchiveMode),
        IDU_FT_SIZEOF(smrArchiveInfo,mArchiveMode),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },


    {
        (SChar*)"ARCHIVE_THR_RUNNING",
        offsetof(smrArchiveInfo,mArchThrRunning),
        IDU_FT_SIZEOF(smrArchiveInfo,mArchThrRunning),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"ARCHIVE_DEST",
        offsetof(smrArchiveInfo,mArchDest),
        IDU_FT_MAX_PATH_LEN,
        IDU_FT_TYPE_VARCHAR|IDU_FT_TYPE_POINTER ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"NEXTLOGFILE_TO_ARCH",
        offsetof(smrArchiveInfo,mNextLogFile2Archive),
        IDU_FT_SIZEOF(smrArchiveInfo,mNextLogFile2Archive) ,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"OLDEST_ACTIVE_LOGFILE",
        offsetof(smrArchiveInfo,mOldestActiveLogFile),
        IDU_FT_SIZEOF(smrArchiveInfo,mOldestActiveLogFile),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"CURRENT_LOGFILE",
        offsetof(smrArchiveInfo,mCurrentLogFile),
        IDU_FT_SIZEOF(smrArchiveInfo,mCurrentLogFile),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

// X$ARCHIVE fixed table def
iduFixedTableDesc  gArchiveTableDesc=
{
    (SChar *)"X$ARCHIVE",
    smrFT::buildRecordForArchiveInfo,
    gArchiveTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


static iduFixedTableColDesc  gStableMemDataFileTableColDesc[]=
{
    {
        (SChar*)"SPACE_ID",
        offsetof(smrStableMemDataFile,mSpaceID),
        IDU_FT_SIZEOF(smrStableMemDataFile,mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"MEM_DATA_FILE",
        offsetof(smrStableMemDataFile,mDataFileName),
        ID_MAX_FILE_NAME,
        IDU_FT_TYPE_VARCHAR|IDU_FT_TYPE_POINTER ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};



// X$STABLE_MEM_DATAFILES fixed table def
iduFixedTableDesc  gStableMemDataFileTableDesc=
{
    (SChar *)"X$STABLE_MEM_DATAFILES",
    smrFT::buildRecordForStableMemDataFiles,
    gStableMemDataFileTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/***********************************************************************
 * Description : V$LFG를 위한 Record를 빌드한다.aRecordBuffer에 aRecordCount
 *               만큼의 레코드를 만들어 저장한다. 그리고 레코드의 갯수를 aRecordCount
 *               에 저장한다.
 *
 * aHeader           - [IN]  Fixed Table Desc
 * aRecordBuffer     - [OUT] Record가 저장될 버퍼
 * aRecordCount      - [OUT] aRecordBuffer에 저장된 레코드의 갯수
 ***********************************************************************/
IDE_RC smrFT::buildRecordOfLFGForFixedTable( idvSQL              * /*aStatistics*/,
                                             void                *  aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory *aMemory )
{
    smrLFGInfo        sPerfLFG;
    smrLogAnchor    * sLogAnchorPtr;
    UInt              sDelLogCnt = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sLogAnchorPtr = smrRecoveryMgr::getLogAnchorMgr()->mLogAnchor;
    IDE_ERROR( sLogAnchorPtr != NULL );

    sPerfLFG.mCurWriteLFNo = smrLogMgr::getLogFileMgr().getCurWriteLFNo();

    /* BUG-19271 control단계에서 v$lfg, x$lfg를 조회하면 DB 비정상 종료함
     *
     * Control단계는 Redo전이기때문에 마지막 Log의 Offset을 알 수 없습니다.
     * */
    if ( smiGetStartupPhase() == SMI_STARTUP_CONTROL )
    {
        sPerfLFG.mCurOffset = 0;
    }
    else
    {
        sPerfLFG.mCurOffset = smrLogMgr::getCurOffset();
    }

    sPerfLFG.mLFOpenCnt       = smrLogMgr::getLogFileMgr().getLFOpenCnt();

    sPerfLFG.mLFPrepareCnt    = smrLogMgr::getLogFileMgr().getLFPrepareCnt();
    sPerfLFG.mLFPrepareWaitCnt= smrLogMgr::getLogFileMgr().getLFPrepareWaitCnt();
    sPerfLFG.mLstPrepareLFNo  = smrLogMgr::getLogFileMgr().getLstPrepareLFNo();

    sPerfLFG.mEndLSN          = sLogAnchorPtr->mMemEndLSN;
    
    //BUG-46266: FIRST_DELETED_LOGFILE, LAST_DELETED_LOGFILE 컬럼 값을 바르게 출력 
    sDelLogCnt = sLogAnchorPtr->mFstDeleteFileNo - sLogAnchorPtr->mLstDeleteFileNo;

    sPerfLFG.mFstDeleteFileNo =
        ( sLogAnchorPtr->mFstDeleteFileNo == 0 ) ?
        0 : ( sLogAnchorPtr->mLstDeleteFileNo - sDelLogCnt );
    sPerfLFG.mLstDeleteFileNo =
        ( sLogAnchorPtr->mLstDeleteFileNo == 0 ) ?
        0 : ( sLogAnchorPtr->mLstDeleteFileNo - 1 );

    sPerfLFG.mResetLSN        = sLogAnchorPtr->mResetLSN;

    sPerfLFG.mUpdateTxCount = smrLogMgr::getUpdateTxCount();

    sPerfLFG.mGCWaitCount        = smrLogMgr::getLFThread().getGroupCommitWaitCount();
    sPerfLFG.mGCAlreadySyncCount = smrLogMgr::getLFThread().getGroupCommitAlreadySyncCount();
    sPerfLFG.mGCRealSyncCount    = smrLogMgr::getLFThread().getGroupCommitRealSyncCount();

    // build record for fixed table.
    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *) &sPerfLFG )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gLFGTableColDesc[]=
{

    {
        (SChar*)"CUR_WRITE_LF_NO",
        offsetof(smrLFGInfo, mCurWriteLFNo),
        IDU_FT_SIZEOF(smrLFGInfo, mCurWriteLFNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"CUR_WRITE_LF_OFFSET",
        offsetof(smrLFGInfo, mCurOffset),
        IDU_FT_SIZEOF(smrLFGInfo, mCurOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"LF_OPEN_COUNT",
        offsetof(smrLFGInfo, mLFOpenCnt),
        IDU_FT_SIZEOF(smrLFGInfo, mLFOpenCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"LF_PREPARE_COUNT",
        offsetof(smrLFGInfo, mLFPrepareCnt),
        IDU_FT_SIZEOF(smrLFGInfo, mLFPrepareCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"LF_PREPARE_WAIT_COUNT",
        offsetof(smrLFGInfo, mLFPrepareWaitCnt),
        IDU_FT_SIZEOF(smrLFGInfo, mLFPrepareWaitCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"LST_PREPARE_LF_NO",
        offsetof(smrLFGInfo, mLstPrepareLFNo),
        IDU_FT_SIZEOF(smrLFGInfo, mLstPrepareLFNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"END_LSN_FILE_NO",
        offsetof(smrLFGInfo,mEndLSN) + offsetof(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN,mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"END_LSN_OFFSET",
        offsetof(smrLFGInfo,mEndLSN) + offsetof(smLSN,mOffset),
        IDU_FT_SIZEOF(smLSN,mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"FIRST_DELETED_LOGFILE",
        offsetof(smrLFGInfo,mFstDeleteFileNo) ,
        IDU_FT_SIZEOF(smrLFGInfo,mFstDeleteFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"LAST_DELETED_LOGFILE",
        offsetof(smrLFGInfo,mLstDeleteFileNo) ,
        IDU_FT_SIZEOF(smrLFGInfo,mLstDeleteFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
   
    {
        (SChar*)"RESET_LSN_FILE_NO",
        offsetof(smrLFGInfo,mResetLSN) + offsetof(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"RESET_LSN_OFFSET",
        offsetof(smrLFGInfo,mResetLSN) + offsetof(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN,mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },

    {
        (SChar*)"UPDATE_TX_COUNT",
        offsetof(smrLFGInfo, mUpdateTxCount),
        IDU_FT_SIZEOF(smrLFGInfo, mUpdateTxCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"GC_WAIT_COUNT",
        offsetof(smrLFGInfo, mGCWaitCount),
        IDU_FT_SIZEOF(smrLFGInfo, mGCWaitCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"GC_ALREADY_SYNC_COUNT",
        offsetof(smrLFGInfo, mGCAlreadySyncCount),
        IDU_FT_SIZEOF(smrLFGInfo, mGCAlreadySyncCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"GC_REAL_SYNC_COUNT",
        offsetof(smrLFGInfo, mGCRealSyncCount),
        IDU_FT_SIZEOF(smrLFGInfo, mGCRealSyncCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

// X$LFG fixed table def
iduFixedTableDesc  gLFGTableDesc=
{
    (SChar *)"X$LFG",
    smrFT::buildRecordOfLFGForFixedTable,
    gLFGTableColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//for fixed table for X$LOG
IDE_RC smrFT::buildRecordForLogAnchor(idvSQL              * /*aStatistics*/,
                                      void*  aHeader,
                                      void*  /* aDumpObj */,
                                      iduFixedTableMemory *aMemory)
{
    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_ERROR( smrRecoveryMgr::getLogAnchorMgr() != NULL );

    // build record for fixed table.
   IDE_TEST(iduFixedTable::buildRecord(
                aHeader,
                aMemory,
                (void *)(smrRecoveryMgr::getLogAnchorMgr())->mLogAnchor)
             != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gLogAnchorColDesc[]=
{
    {
        (SChar*)"BEGIN_CHKPT_FILE_NO",
        offsetof(smrLogAnchor,mBeginChkptLSN) + offsetof(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"BEGIN_CHKPT_FILE_OFFSET",
        offsetof(smrLogAnchor,mBeginChkptLSN) + offsetof(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN,  mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"END_CHKPT_FILE_NO",
        offsetof(smrLogAnchor,mEndChkptLSN) + offsetof(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"END_CHKPT_FILE_OFFSET",
        offsetof(smrLogAnchor,mEndChkptLSN) + offsetof(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN,  mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"SERVER_STATUS",
        offsetof(smrLogAnchor,mServerStatus) ,
        IDU_FT_SIZEOF(smrLogAnchor,mServerStatus),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"OLDEST_LOGFILE_NO",
        offsetof(smrLogAnchor,mDiskRedoLSN) + offsetof(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"OLDEST_LOGFILE_OFFSET",
        offsetof(smrLogAnchor,mDiskRedoLSN) + offsetof(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN,  mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"ARCHIVE_MODE",
        offsetof(smrLogAnchor,mArchiveMode) ,
        IDU_FT_SIZEOF(smrLogAnchor,mArchiveMode),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TRANSACTION_SEGMENT_COUNT",
        offsetof(smrLogAnchor,mTXSEGEntryCnt) ,
        IDU_FT_SIZEOF(smrLogAnchor,mTXSEGEntryCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NEW_TABLESPACE_ID",
        offsetof(smrLogAnchor,mNewTableSpaceID) ,
        IDU_FT_SIZEOF(smrLogAnchor,mNewTableSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

// X$LOG fixed table def
iduFixedTableDesc  gLogAnchorDesc=
{
    (SChar *)"X$LOG",
    smrFT::buildRecordForLogAnchor,
    gLogAnchorColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//for fixed table for X$RECOVERY_FAIL_OBJ
IDE_RC smrFT::buildRecordForRecvFailObj(idvSQL              * /*aStatistics*/,
                                        void*  aHeader,
                                        void*  /* aDumpObj */,
                                        iduFixedTableMemory *aMemory)
{
    smrRTOI    * sCursor;
    smrRTOI4FT   sRTOI4FT;
    SChar        sRTOIType[5][9]={ "NONE",
                                   "TABLE",
                                   "INDEX",
                                   "MEMPAGE",
                                   "DISKPAGE" };
    SChar        sRTOICause[6][9]={ "NONE",
                                    "OBJECT",
                                    "REDO",
                                    "UNDO",
                                    "REFINE",
                                    "PROPERTY" };

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCursor = smrRecoveryMgr::getIOLHead()->mNext;
    while( sCursor != NULL )
    {
        switch( sCursor->mType )
        {
        case SMR_RTOI_TYPE_TABLE:
        case SMR_RTOI_TYPE_INDEX:
        case SMR_RTOI_TYPE_MEMPAGE:
        case SMR_RTOI_TYPE_DISKPAGE:
            idlOS::strncpy( sRTOI4FT.mType, 
                            sRTOIType[ sCursor->mType ], 
                            9 );
            break;
        default:
            idlOS::snprintf( sRTOI4FT.mType,
                             9,
                             "%"ID_UINT32_FMT,
                             sCursor->mType );
            break;
        }

        switch( sCursor->mCause )
        {
        case SMR_RTOI_CAUSE_OBJECT:   /* 객체(Table,index,Page)등이 이상함*/
        case SMR_RTOI_CAUSE_REDO:     /* RedoRecovery에서 실패했음 */
        case SMR_RTOI_CAUSE_UNDO:     /* UndoRecovery에서 실패했음 */
        case SMR_RTOI_CAUSE_REFINE:   /* Refine에서 실패했음 */
        case SMR_RTOI_CAUSE_PROPERTY: /* Property에 의해 강제로 제외됨  */
            idlOS::strncpy( sRTOI4FT.mCause, 
                            sRTOICause[ sCursor->mCause ], 
                            9 );
            break;
        default:
            idlOS::snprintf( sRTOI4FT.mCause,
                             9,
                             "%"ID_UINT32_FMT,
                             sCursor->mCause );
            break;
        }

        switch( sCursor->mType )
        {
        case SMR_RTOI_TYPE_TABLE:
            sRTOI4FT.mData1  = sCursor->mTableOID;
            sRTOI4FT.mData2  = 0;
            break;
        case SMR_RTOI_TYPE_INDEX:
            sRTOI4FT.mData1  = sCursor->mTableOID;
            sRTOI4FT.mData2  = sCursor->mIndexID;
            break;
        case SMR_RTOI_TYPE_MEMPAGE:
        case SMR_RTOI_TYPE_DISKPAGE:
            sRTOI4FT.mData1  = sCursor->mGRID.mSpaceID;
            sRTOI4FT.mData2  = sCursor->mGRID.mPageID;
            break;
        default:
            sRTOI4FT.mData1 = 0;
            sRTOI4FT.mData2 = 0;
            break;
        }

        // build record for fixed table.
        IDE_TEST(iduFixedTable::buildRecord(
                aHeader,
                aMemory,
                (void *)&sRTOI4FT)
            != IDE_SUCCESS);
        sCursor = sCursor->mNext;
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gRecvFailObjColDesc[]=
{
    {
        (SChar*)"TYPE",
        offsetof(smrRTOI4FT,mType),
        IDU_FT_SIZEOF(smrRTOI4FT,mType),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"CAUSE",
        offsetof(smrRTOI4FT,mCause),
        IDU_FT_SIZEOF(smrRTOI4FT,mCause),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"DATA1",
        offsetof(smrRTOI4FT,mData1),
        IDU_FT_SIZEOF(smrRTOI4FT,mData1),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"DATA2",
        offsetof(smrRTOI4FT,mData2),
        IDU_FT_SIZEOF(smrRTOI4FT,mData2),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

// X$RECOVERY_FAIL_OBJ fixed table def
iduFixedTableDesc  gRecvFailObjDesc=
{
    (SChar *)"X$RECOVERY_FAIL_OBJ",
    smrFT::buildRecordForRecvFailObj,
    gRecvFailObjColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
