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
 * $Id: smrLogMgr.h 88191 2020-07-27 03:08:54Z mason.lee $
 *
 * Description :
 *
 * 로그관리자 헤더 파일 입니다. 
 *
 * 로그는 한 방향으로 계속 자라나는, Durable한 저장공간이다.
 * 하지만 Durable한 매체로 보편적으로 사용하는 Disk의 경우,
 * 그 용량이 한정되어 있어서, 로그를 무한정 자라나게 허용할 수는 없다.
 *
 * 그래서 여러개의 물리적인 로그파일들을 논리적으로 하나의 로그로
 * 사용할 수 있도록 구현을 하는데,
 * 이 때 필요한 물리적인 로그파일을 smrLogFile 로 표현한다.
 * 여러개의 로그파일들의 리스트를 관리하는 역할을 smrLogFileMgr이 담당하고,
 * 로그파일의 Durable한 속성을 충족시키는 역할을 smrLFThread가 담당한다.
 * 여러개의 물리적인 로그파일들을
 * 하나의 논리적인 로그로 추상화 하는 역할을 smrLogMgr가 담당한다.
 *
 * # 기능
 * 1. 로그레코드 타입별 로깅
 * 2. Logging 레벨 설정
 * 3. Durability 레벨 및 Logging 레벨에 따라 로깅 처리
 * 4. 로그파일 전환
 * 5. Synced Last LSN 및 Last LSN 관리
 *
 * #  관리하는 Thread들
 *
 * 1. 로그파일 Prepare 쓰레드 - smrLogFileMgr
 *    STARTUP  :startupLogPrepareThread
 *    SHUTDOWN :shutdown
 *
 * 2. 로그파일 Sync 쓰레드 - smrLFThread
 *    STARTUP  :initialize
 *    SHUTDOWN :shutdown
 *
 * 3. 로그파일 Archive 쓰레드 - smrArchThread
 *    STARTUP  :startupLogArchiveThread
 *    SHUTDOWN :shutdown
 *
 *    사용자가 명시적으로 시작시킬 때 :
 *              startupLogArchiveThread
 *
 *    사용자가 명시적으로 중지시킬 때 :
 *              shutdownLogArchiveThread
 * 
 **********************************************************************/

#ifndef _O_SMR_LOG_MGR_H_
#define _O_SMR_LOG_MGR_H_ 1

#include <idu.h>
#include <idp.h>
#include <iduMemoryHandle.h>

#include <sctDef.h>
#include <smrDef.h>
#include <smrLogFile.h>
#include <smrLogFileMgr.h>
#include <smrLFThread.h>
#include <smrArchThread.h>
#include <smrLogHeadI.h>
#include <smrCompResPool.h>
#include <smrLogComp.h>             /* BUG-35392 */
#include <smrUCSNChkThread.h>       /* BUG-35392 */


#define SMR_CHECK_LSN_UPDATE_GROUP   (32) // FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1 인 경우 dummyLog 가 발생할수 있음.
                                          // 32개 trans을 모아서 로그를 기록중인 트랜잭션의 수를 기록한다. 
                                          // Group Count=(기록중인 트랜잭션의 수)가 0인 Group은 검사하지 않고 skip. 
 

struct smuDynArrayBase;

class smrLogMgr
{
public:

    static IDE_RC initialize();
    static IDE_RC destroy();
    
    static IDE_RC initializeStatic(); 
    static IDE_RC destroyStatic();

    // createDB 시 호출 
    // 로그파일들을 최초로 생성한다.
    static IDE_RC create();
    // 로그 파일 매니저가 지닌 쓰레드들을 중지 
    static IDE_RC shutdown();

/***********************************************************************
 *
 * Description : 로그타입으로 로그의 메모리/디스크 관련여부를 반환한다.
 *
 * [IN] aLogType : 로그타입
 *
 ***********************************************************************/
    static inline idBool isDiskLogType( smrLogType aLogType )
    {
        if ( (aLogType == SMR_DLT_REDOONLY)       ||
             (aLogType == SMR_DLT_UNDOABLE)       ||
             (aLogType == SMR_DLT_NTA)            ||
             (aLogType == SMR_DLT_REF_NTA)        ||
             (aLogType == SMR_DLT_COMPENSATION)   ||
             (aLogType == SMR_LT_DSKTRANS_COMMIT) ||
             (aLogType == SMR_LT_DSKTRANS_ABORT) )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    };

    // MRDB의 operation NTA 로그 기록
    static IDE_RC writeNTALogRec(idvSQL*   aStatistics,
                                 void*      aTrans,
                                 smLSN*     aLSN,
                                 scSpaceID  aSpaceID = 0,
                                 smrOPType  aOPType = SMR_OP_NULL,
                                 vULong     aData1 = 0,
                                 vULong     aData2 = 0);

    static IDE_RC writeCMPSLogRec( idvSQL*       aStatistics,
                                   void*         aTrans,
                                   smrLogType    aType,
                                   smLSN*        aPrvUndoLSN,
                                   smrUpdateLog* aUpdateLog,
                                   SChar*        aBeforeImagePtr );

    // savepoint 설정 로그 기록
    static IDE_RC writeSetSvpLog( idvSQL*   aStatistics,
                                  void*         aTrans,
                                  const SChar*  aSVPName );

    // savepoint 해제 로그 기록
    static IDE_RC writeAbortSvpLog( idvSQL*   aStatistics,
                                    void*          aTrans,
                                    const SChar*   aSVPName );

    // DB 파일 추가에 관련된 로그
    static IDE_RC writeDbFileChangeLog( idvSQL*   aStatistics,
                                        void * aTrans,
                                        SInt   aDBFileNo,
                                        SInt   aCurDBFileCount,
                                        SInt   aAddedDBFileCount );

    /* ------------------------------------------------
     * DRDB의 로그를 로그파일에 기록
     * writeDiskLogRec 함수에서 호출되는 함수
     * - 로깅할 로그의 포인터를 받는것 대신 smuDynBuffer
     * 포인터를 받아서 처리
     * - 로그의 BeginLSN과 EndLSN을 리턴함
     * ----------------------------------------------*/
    static IDE_RC writeDiskLogRec( idvSQL           *aStatistics,
                                   void             *aTrans,
                                   smLSN            *aPrvLSN,
                                   smuDynArrayBase  *aMtxLogBuffer,
                                   UInt              aWriteOption,
                                   UInt              aLogAttr,
                                   UInt              aContType,
                                   UInt              aRefOffset,
                                   smOID             aTableOID,
                                   UInt              aRedoType,
                                   smLSN*            aBeginLSN,
                                   smLSN*            aEndLSN );

    static IDE_RC writeDiskNTALogRec( idvSQL*           aStatistics,
                                      void*             aTrans,
                                      smuDynArrayBase*  aMtxLogBuffer,
                                      UInt              aWriteOption,
                                      UInt              aOPType,
                                      smLSN*            aPPrevLSN,
                                      scSpaceID         aSpaceID,
                                      ULong           * aArrData,
                                      UInt              aDataCount,
                                      smLSN*            aBeginLSN,
                                      smLSN*            aEndLSN,
                                      smOID             aTableOID );

    static IDE_RC writeDiskRefNTALogRec( idvSQL*           aStatistics,
                                         void*             aTrans,
                                         smuDynArrayBase*  aMtxLogBuffer,
                                         UInt              aWriteOption,
                                         UInt              aOPType,
                                         UInt              aRefOffset, 
                                         smLSN*            aPPrevLSN,
                                         scSpaceID         aSpaceID,
                                         smLSN*            aBeginLSN,
                                         smLSN*            aEndLSN,
                                         smOID             aTableOID );

    static IDE_RC writeDiskDummyLogRec( idvSQL*           aStatistics,
                                        smuDynArrayBase*  aMtxLogBuffer,
                                        UInt              aWriteOption,
                                        UInt              aContType,
                                        UInt              aRedoType,
                                        smOID             aTableOID,
                                        smLSN*            aBeginLSN,
                                        smLSN*            aEndLSN );

    static IDE_RC writeDiskCMPSLogRec( idvSQL*           aStatistics,
                                       void*             aTrans,
                                       smuDynArrayBase*  aMtxLogBuffer,
                                       UInt              aWriteOption,
                                       smLSN*            aPrevLSN,
                                       smLSN*            aBeginLSN,
                                       smLSN*            aEndLSN,
                                       smOID             aTableOID );

    static IDE_RC writeCMPSLogRec4TBSUpt( idvSQL*           aStatistics,
                                          void*             aTrans,
                                          smLSN*            aPrevLSN,
                                          smrTBSUptLog*     aUpdateLog,
                                          SChar*            aBeforeImagePtr );

    /* BUG-9640 */
    static IDE_RC writeTBSUptLogRec( idvSQL*           aStatistics,
                                     void*             aTrans,
                                     smuDynArrayBase*  aLogBuffer,
                                     scSpaceID         aSpaceID,
                                     UInt              aFileID,
                                     sctUpdateType     aTBSUptType,
                                     UInt              aAImgSize,
                                     UInt              aBImgSize,
                                     smLSN*            aBeginLSN );

    /* prj-1149*/
    static IDE_RC writeDiskPILogRec( idvSQL*           aStatistics,
                                     UChar*            aBuffer,
                                     scGRID            aPageGRID );

    // PROJ-1362 Internal LOB
    static IDE_RC writeLobCursorOpenLogRec(idvSQL*         aStatistics,
                                            void*           aTrans,
                                            smrLobOpType    aLobOp,
                                            smOID           aTable,
                                            UInt            aLobColID,
                                            smLobLocator    aLobLocator,
                                            const void*     aPKInfo );

    static IDE_RC writeLobCursorCloseLogRec( idvSQL*        aStatistics,
                                             void*          aTrans,
                                             smLobLocator   aLobLocator,
                                             smOID          aTableOID );

    static IDE_RC writeLobPrepare4WriteLogRec(idvSQL*       aStatistics,
                                              void*         aTrans,
                                              smLobLocator  aLobLocator,
                                              UInt          aOffset,
                                              UInt          aOldSize,
                                              UInt          aNewSize,
                                              smOID         aTableOID );

    static IDE_RC writeLobFinish2WriteLogRec( idvSQL*       aStatistics,
                                              void*         aTrans,
                                              smLobLocator  aLobLocator,
                                              smOID         aTableOID );

    // PROJ-2047 Strengthening LOB
    static IDE_RC writeLobEraseLogRec( idvSQL       * aStatistics,
                                       void         * aTrans,
                                       smLobLocator   aLobLocator,
                                       ULong          aOffset,
                                       ULong          aSize,
                                       smOID          aTableOID );

    static IDE_RC writeLobTrimLogRec( idvSQL        * aStatistics,
                                      void*           aTrans,
                                      smLobLocator    aLobLocator,
                                      ULong           aOffset,
                                      smOID           aTableOID );

    // PROJ-1665
    // Direct-Path Insert가 수행된 Page 전체를 logging 하는 함수
    static IDE_RC writeDPathPageLogRec( idvSQL * aStatistics,
                                        UChar  * aBuffer,
                                        scGRID   aPageGRID,
                                        smLSN  * aEndLSN );

    // PROJ-1867 write Page Image Log
    static IDE_RC writePageImgLogRec( idvSQL     * aStatistics,
                                      UChar      * aBuffer,
                                      scGRID       aPageGRID,
                                      sdrLogType   aLogType,
                                      smLSN      * aEndLSN );

    static IDE_RC writePageConsistentLogRec( idvSQL        * aStatistics,
                                             scSpaceID       aSpaceID,
                                             scPageID        aPageID,
                                             UChar           aIsPageConsistent );

    // 로그파일에 로그 기록
    static IDE_RC writeLog( idvSQL  * aStatistics,
                            void    * aTrans,
                            SChar   * aStrLog,
                            smLSN   * aPPrvLSN,
                            smLSN   * aBeginLSN,
                            smLSN   * aEndLSN,
                            smOID     aTableOID );

    /* 압축된 로그파일의 경우 압축을 풀어서 레코드를 읽어온다. */
    static IDE_RC readLog( iduMemoryHandle    * aDecompBufferHandle,
                           smLSN              * aLSN,
                           idBool               aIsCloseLogFile,
                           smrLogFile        ** aLogFile,
                           smrLogHead         * aLogHeadPtr,
                           SChar             ** aLogPtr,
                           idBool             * aIsValid,
                           UInt               * aLogSizeAtDisk);
    
    /* 압축된 로그파일의 경우 압축을 풀지않고 반환한다. */
    static IDE_RC readLog4RP( smLSN              * aLSN,
                              idBool               aIsCloseLogFile,
                              smrLogFile        ** aLogFile,
                              smrLogHead         * aLogHeadPtr,
                              SChar             ** aLogPtr,
                              idBool             * aIsValid,
                              UInt               * aLogSizeAtDisk );

    // 특정 LSN의 log record와 해당 log record가 속한 로그 파일을 리턴한다.
    static IDE_RC readLogInternal( iduMemoryHandle  * aDecompBufferHandle,
                                   smLSN            * aLSN,
                                   idBool             aIsCloseLogFile,
                                   smrLogFile      ** aLogFile,
                                   smrLogHead       * aLogHead,
                                   SChar           ** aLogPtr,
                                   UInt             * aLogSizeAtDisk );

    static IDE_RC readLogInternal4RP( smLSN            * aLSN,
                                      idBool             aIsCloseLogFile,
                                      smrLogFile      ** aLogFile,
                                      smrLogHead       * aLogHead,
                                      SChar           ** aLogPtr,
                                      UInt             * aLogSizeAtDisk );

    // 특정 로그파일의 첫번째 로그의 Head를 읽는다.
    static IDE_RC readFirstLogHeadFromDisk( UInt   aFileNo,
                                            smrLogHead *aLogHead );
    
    /*
      모든 LogFile을 조사해서 aMinLSN보다 작은 LSN을 가지는 로그를
      첫번째로 가지는 LogFile No를 구해서 aNeedFirstFileNo에 넣어준다.
    */
    static IDE_RC getFirstNeedLFN( smLSN          aMinLSN,
                                   const UInt     aFirstFileNo,
                                   const UInt     aEndFileNo,
                                   UInt         * aNeedFirstFileNo );

    // 마지막 LSN까지 Sync한다.
    static IDE_RC syncToLstLSN( smrSyncByWho   aWhoSyncLog );

    // 이 Log File Group의 로그 파일들이 저장되는 경로를 리턴
    static inline const SChar * getLogPath()
    {
        return mLogPath;
    };

    // 이 Log File Group의 로그들이 아카이브될 경로를 리턴
    static inline const SChar * getArchivePath()
    {
        return mArchivePath;
    };

    static inline smrLogFileMgr &getLogFileMgr()
    {
        return mLogFileMgr;
    };

    static inline smrLFThread &getLFThread()
    {
        return mLFThread;
    };

    static inline smrArchThread &getArchiveThread()
    {
        return mArchiveThread;
    };

    static inline UInt getCurOffset()
    {
        return mCurLogFile->mOffset;
    }
   
    /* 로그파일의 끝에 대한 동시성 제어를 위한 lock()/unlock() */
    static inline IDE_RC lock() 
    { 
        return mMutex.lock( NULL ); 
    }
    static inline IDE_RC unlock() 
    { 
        return mMutex.unlock(); 
    }

    /* LogMgr이 initialize 되었는지 확인한다. */
    static inline idBool isAvailable()
    {
        return mAvailable;
    }

public:
    /********************  Group Commit 관련 ********************/
    // Active Transaction중 Update Transaction의 수를 리턴한다.
    static inline UInt getUpdateTxCount()
    {
        return mUpdateTxCount ;
    };

    // Active Transaction중 Update Transaction의 수를
    // 하나 증가시킨다.
    // 반드시 mMutex를 잡은 상태(lock()호출)에서 호출되어야 한다.
    static  inline void incUpdateTxCount()
    {
        IDE_DASSERT( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY );

        idCore::acpAtomicInc32( &mUpdateTxCount );
        // Update Transaction 수가 시스템의 최대 Transaction수 보다 많을 수 없다.
        IDE_DASSERT( mUpdateTxCount <= smuProperty::getTransTblSize() );
    };

    // Active Transaction중 Update Transaction의 수를
    // 하나 감소시킨다.
    // 반드시 mMutex를 잡은 상태(lock()호출)에서 호출되어야 한다.
    static inline void decUpdateTxCount()
    {
        IDE_DASSERT( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY );

        IDE_DASSERT ( mUpdateTxCount > 0 );

        idCore::acpAtomicDec32( &mUpdateTxCount ); 
    };

public:
    /******************** 로그 파일 매니저 ********************/
    // 로그파일을 강제로 switch시키고 archiving 
    static IDE_RC switchLogFileByForce();
    // interval에 의한 checkpoint 수행후 switch count 초기화 
    static IDE_RC clearLogSwitchCount();
    
     /***********************************************************************
     * Description : startupLogPrepareThread 와 동일한 작업 
                     단 createDB때 호출, LSN is 0
     **********************************************************************/
    static inline IDE_RC startupLogPrepareThread()
    {
        smLSN  sLstLSN;
        SM_LSN_INIT( sLstLSN );
 
        IDE_TEST( mLogFileMgr.startAndWait( &sLstLSN,
                                            0,        /* aLstFileNo */
                                            ID_FALSE, /* aIsRecovery */  
                                            &mCurLogFile )
                  != IDE_SUCCESS );

        return IDE_SUCCESS;
        IDE_EXCEPTION_END;
        return IDE_FAILURE;
    };

     /***********************************************************************
     * Description : Prepare 쓰레드를 Start시킴.
     *               aLstLSN에 해당하는 파일을 
                     mCurLogFile로 설정하고 LstLSN으로 설정함. 
                      
     * aLstLSN    - [IN] Lst LSN
     * aLstFileNo - [IN] 마지막으로 생성한 로그파일 번호
     * aIsRecovery- [IN] recovery 여부 
     **********************************************************************/
    static inline IDE_RC startupLogPrepareThread( smLSN   * aLstLSN, 
                                                  UInt      aLstFileNo, 
                                                  idBool    aIsRecovery )
    {
        IDE_TEST( mLogFileMgr.startAndWait( aLstLSN,
                                            aLstFileNo,
                                            aIsRecovery,
                                            &mCurLogFile )
                  != IDE_SUCCESS );

        IDE_DASSERT ( mCurLogFile->mOffset == aLstLSN->mOffset );
        IDE_DASSERT ( mCurLogFile->mFileNo == aLstLSN->mFileNo );

        return IDE_SUCCESS;
        IDE_EXCEPTION_END;
        return IDE_FAILURE;
    };

    /***********************************************************************
     * Description : aFileNo가 가리키는 LogFile을 Open한다.
     *               aLogFilePtr에 Open된 Logfile Pointer를 Setting해준다.
     *
     * aFileNo     - [IN] open할 LogFile No
     * aIsWrite    - [IN] open할 logfile에 대해 write를 한다면 ID_TRUE,
     *                    아니면 ID_FALSE
     * aLogFilePtr - [OUT] open된 logfile를 가리킨다.
     **********************************************************************/
    static inline IDE_RC openLogFile( UInt           aFileNo,
                                      idBool         aIsWrite,
                                      smrLogFile   **aLogFilePtr )
    {
        IDE_TEST( mLogFileMgr.open( aFileNo,
                                    aIsWrite,
                                    aLogFilePtr )
                  != IDE_SUCCESS);
        
        return IDE_SUCCESS;
        IDE_EXCEPTION_END;
        return IDE_FAILURE;
    };

    /* aLogFile을 Close한다. */
    static inline IDE_RC closeLogFile(smrLogFile *aLogFile)
    {
        IDE_DASSERT( aLogFile != NULL );

        IDE_TEST( mLogFileMgr.close( aLogFile ) != IDE_SUCCESS );

        return IDE_SUCCESS;
        IDE_EXCEPTION_END;
        return IDE_FAILURE;
    }

    // aLSN이 가리키는 로그파일의 첫번째 Log 의 Head를 읽는다
    static IDE_RC readFirstLogHead( smLSN      *aLSN,
                                    smrLogHead *aLogHead);

public:
    /******************** Archive 쓰레드 ********************/
    // Archive 쓰레드를 startup시킨다.
    static inline IDE_RC startupLogArchiveThread()
    {
        IDE_TEST( mArchiveThread.startThread() != IDE_SUCCESS );

        return IDE_SUCCESS;
        IDE_EXCEPTION_END;
        return IDE_FAILURE;
    }

    // Archive 쓰레드를 shutdown시킨다.
    static inline IDE_RC shutdownLogArchiveThread()
    {
        IDE_TEST( mArchiveThread.shutdown() != IDE_SUCCESS );
        return IDE_SUCCESS;
        IDE_EXCEPTION_END;
        return IDE_FAILURE;
    }

public:
    /******************** Log Flush Thread ********************/
    // Log Flush Thread가 aLSNToSync에 지정된 LSN까지 Sync수행.
    static IDE_RC syncLFThread( smrSyncByWho    aWhoSync,
                                smLSN         * aLSNToSync );

    // sync된 log file의 첫 번째 로그 중 가장 작은 값을 가져온다.
    static IDE_RC getSyncedMinFirstLogLSN( smLSN *aLSN );

    /* aLSNToSync까지 로그가 sync되었음을 보장한다.
     *  
     * 버퍼 관리자에 의해 호출되며, 기본적인 동작은
     * noWaitForLogSync 와 같다.
     */
    static inline IDE_RC sync4BufferFlush( smLSN        * aLSNToSync,
                                           UInt         * aSyncedLFCnt )
    {

        IDE_TEST( mLFThread.syncOrWait4SyncLogToLSN( SMR_LOG_SYNC_BY_BFT,
                                                     aLSNToSync->mFileNo,
                                                     aLSNToSync->mOffset, 
                                                     aSyncedLFCnt )
                  != IDE_SUCCESS);

        return IDE_SUCCESS;
        IDE_EXCEPTION_END;
        return IDE_FAILURE;
    }

    static void writeDebugInfo();

public:
    /********************  FAST UNLOCK LOG ALLOC MUTEX ********************/
    /* BUG-35392 
     * Dummy Log를 포함하지 않는 마지막 로그 레코드의 LSN */
    static inline void getUncompletedLstWriteLSN( smLSN   * aLSN )
    {
        IDE_DASSERT( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE );
        smrLSN4Union  sLstWriteLSN;

        sLstWriteLSN.mSync = mUncompletedLSN.mLstWriteLSN.mSync;

        SM_GET_LSN( *aLSN,
                    sLstWriteLSN.mLSN );
    };

#if 0
    /* BUG-35392 */
    static inline void getLstWriteLogLSN( smLSN   * aLstWriteLSN )
    {
        if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
        {
            /* Dummy Log를 포함하지 않는 마지막 로그 레코드의 LSN */
            (void)getUncompletedLstWriteLSN( aLstWriteLSN );
        }
        else
        {
            /*  마지막으로 기록한 로그 레코드의 LSN */
            getLstWriteLSN( aLstWriteLSN );
        }
    };
#endif

    /* BUG-35392 
     * Dummy Log를 포함하지 않는 마지막 로그 레코드의 Last offset 받아온다. */
    static inline void getUncompletedLstLSN( smLSN   * aUncompletedLSN )
    {
        smrLSN4Union sLstLSN;

        IDE_DASSERT( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE );

        sLstLSN.mSync = mUncompletedLSN.mLstLSN.mSync;

        SM_GET_LSN( *aUncompletedLSN,
                    sLstLSN.mLSN );
    };

    /* BUG-35392
     * logfile에 저장된 Log의 마지막 Offset을 받아온다.
     */
    static inline void getLstLogOffset( smLSN  * aValidLSN )
    {
        if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
        {
            /* Dummy Log를 포함하지 않는 마지막 로그 레코드의 Last offset */
            getUncompletedLstLSN( aValidLSN );
        }
        else
        {
            /* 마지막으로 기록한 로그 레코드의 Last Offset을 반환 */
            getLstLSN( aValidLSN );
        }
    };

   /* 지정된 LSN 까지 sync가 완료되기를 대기한다. */
    static void waitLogSyncToLSN( smLSN  * aLSNToSync,
                                  UInt     aSyncWaitMin,
                                  UInt     aSyncWaitMax );

    static void rebuildMinUCSN();

    // Dummy 로그를 기록한다. 
    static IDE_RC writeDummyLog();
 
    /***********************************************************************
     * Description : 마지막으로 기록한 로그 레코드의 "Last Offset"을 설정한다. 
     *               읽는 것은 동시에 가능하나, 동시에 쓰는것은 안된다.
     *               allocMutex로 보호하고 기록한다.
     *               allocMutex로 보호되기에 항상 더 큰 값이 와야 한다.
     *
     *   logfile0 에 20까지 로그가 기록되었다면 
     *   +---------------------------------------------
     *   + [FileNo, offset] | .....   [0,10] | [0,20] |        
     *   +----------------------------------------------
     *                                      (A)      (B) 
     *   (A) : mLstWriteLSN    (B) : mLstLSN
     *   이 함수는  (B) 를 설정함 
     *
     *   aFileNo  - [IN] Log File Number
     *   aOffset  - [IN] Log를 마지막으로 기록한 로그레코드의 "Last Offset"
     ***********************************************************************/
    static inline void setLstLSN( UInt   aFileNo,
                                  UInt   aOffset )
    {
        /* BUG-32137 [sm-disk-recovery] The setDirty operation in DRDB causes 
         * contention of LOG_ALLOCATION_MUTEX.
         * 무조건 smrUncompletedLogInfo를 이용해 설정한다.
         * 64Bit일 경우 Atomic하게 수행되기 때문에 상관없고,
         * 32Bit 는 지원하지 않는다!.*/
        smrLSN4Union sLstLSN;

        ID_SERIAL_BEGIN( SM_SET_LSN( sLstLSN.mLSN,
                                     aFileNo,
                                     aOffset ) );        

        ID_SERIAL_END( mLstLSN.mSync = sLstLSN.mSync );
        
    }

    /***********************************************************************
     * Description : 마지막으로 기록한 로그 레코드의 "Last Offset"을 반환한다.  
     *               flush 등의 이유로 마지막 LSN을 구해야 할때 사용 
     *   aLstLSN - [OUT] 마지막 LSN의 "Last Offset"
     ***********************************************************************/
    static inline void getLstLSN( smLSN  * aLSN )
    {
        smrLSN4Union sLstLSN ;

        ID_SERIAL_BEGIN( sLstLSN.mSync = mLstLSN.mSync );

        ID_SERIAL_END( SM_GET_LSN( *aLSN,
                                    sLstLSN.mLSN ) );
    }

    /***********************************************************************
     * Description : 마지막으로 기록한 로그 레코드의 LSN을 설정한다
     *               DR, RP 등에서 마지막 기록한 로그 레코드를 찾기 위해 사용 
     *
     *   logfile0 에 20까지 로그가 기록되었다면 
     *   +---------------------------------------------
     *   + [FileNo, offset] | .....   [0,10] | [0,20] |        
     *   +----------------------------------------------
     *                                      (A)      (B) 
     *   (A) : mLstWriteLSN    (B) : mLstLSN
     *   이 함수는 (A) 를 설정함
     *
     *   aLstLSN  - [IN]  Log를 마지막으로 기록한 로그레코드의 LSN
     ***********************************************************************/
    static inline void setLstWriteLSN( smLSN  aLSN )
    {
         /* 무조건 smrUncompletedLogInfo를 이용해 설정한다.
         * 64Bit일 경우 Atomic하게 수행되기 때문에 상관없고,
         * 32Bit 는 지원하지 않는다!.*/
        smrLSN4Union sLstWriteLSN;

        ID_SERIAL_BEGIN( SM_SET_LSN( sLstWriteLSN.mLSN,
                                     aLSN.mFileNo,
                                     aLSN.mOffset ) );        

        ID_SERIAL_END( mLstWriteLSN.mSync = sLstWriteLSN.mSync );
    }

    /***********************************************************************
     * Description : 마지막으로 기록한 로그 레코드의 LSN을 가져온다  
     *
     *   aLstLSN - [OUT] 마지막으로 기록한 로그 레코드의 LSN
     ***********************************************************************/
    static inline void getLstWriteLSN( smLSN *aLSN )
    {
        smrLSN4Union sLstWriteLSN;

        ID_SERIAL_BEGIN( sLstWriteLSN.mSync = mLstWriteLSN.mSync );

        ID_SERIAL_END( SM_GET_LSN( *aLSN,
                                   sLstWriteLSN.mLSN ) );
    }

public: // for request function

    // Disk로그의 Log 압축 여부를 결정한다
    static IDE_RC decideLogComp( UInt aDiskLogWriteOption,
                                 smrLogHead * aLogHead );

    /* SMR_OP_NULL 타입의 NTA 로그 기록 */
    static IDE_RC writeNullNTALogRec( idvSQL* aStatistics,
                                      void*   aTrans,
                                      smLSN*  aLSN );

    /* SMR_OP_SMM_PERS_LIST_ALLOC 타입의 NTA 로그 기록 */
    static IDE_RC writeAllocPersListNTALogRec( idvSQL*    aStatistics,
                                               void     * aTrans,
                                               smLSN    * aLSN,
                                               scSpaceID  aSpaceID,
                                               scPageID   aFirstPID,
                                               scPageID   aLastPID );

    static IDE_RC writeCreateTbsNTALogRec( idvSQL*    aStatistics,
                                           void     * aTrans,
                                           smLSN    * aLSN,
                                           scSpaceID  aSpaceID);

    // Table/Index/Sequence의
    // Create/Alter/Drop DDL에 대해 Query String을 로깅한다.
    static IDE_RC writeDDLStmtTextLog( idvSQL         * aStatistics,
                                       void           * aTrans,
                                       smrDDLStmtMeta * aDDLStmtMeta,
                                       SChar          * aStmtText,
                                       SInt             aStmtTextLen );

private:
    static IDE_RC writeLobOpLogRec( idvSQL*           aStatistics,
                                    void*             aTrans,
                                    smLobLocator      aLobLocator,
                                    smrLobOpType      aLobOp,
                                    smOID             aTableOID );
    
    static UInt   getMaxLogOffset() { return mMaxLogOffset; };

    // File Begin Log를 구성한다.
    static void initializeFileBeginLog
                           ( smrFileBeginLog * aFileBeginLog );
    // File End Log를 구성한다.
    static void initializeFileEndLog
                           ( smrFileEndLog * aFileEndLog );

    // SMR_LT_FILE_BEGIN 로그를 기록한다.
    static void writeFileBeginLog();
    
    // SMR_LT_FILE_END 로그를 기록한다.
    static void writeFileEndLog();

    // 압축 리소스를 가져온다
    static IDE_RC allocCompRes( void        * aTrans,
                                smrCompRes ** aCompRes );

    // 압축 리소스를 반납한다.
    static IDE_RC freeCompRes( void       * aTrans,
                               smrCompRes * aCompRes );

    // 압축하지 않은 원본로그를 Replication Log Buffer로 복사
    static void copyLogToReplBuffer( idvSQL * aStatistics,
                                     SChar  * aRawLog,
                                     UInt     aRawLogSize,
                                     smLSN    aLSN );

    // Log의 끝단 Mutex를 잡은 상태로 로그 기록 
    static IDE_RC lockAndWriteLog( idvSQL   * aStatistics,
                                   void     * aTrans,
                                   SChar    * aRawOrCompLog,
                                   UInt       aRawOrCompLogSize,
                                   SChar    * aRawLog4Repl,
                                   UInt       aRawLogSize4Repl,
                                   smLSN    * aBeginLSN,
                                   smLSN    * aEndLSN,
                                   idBool   * aIsLogFileSwitched );

    // Log의 끝단 Mutex를 잡은 상태로 로그 기록 
    static IDE_RC lockAndWriteLog4FastUnlock( idvSQL   * aStatistics,
                                              void     * aTrans,
                                              SChar    * aRawOrCompLog,
                                              UInt       aRawOrCompLogSize,
                                              SChar    * aRawLog4Repl,
                                              UInt       aRawLogSize4Repl,
                                              smLSN    * aBeginLSN,
                                              smLSN    * aEndLSN,
                                              idBool   * aIsLogFileSwitched );


    // 로그 기록전에 수행할 작업 처리
    static void onBeforeWriteLog( void     * aTrans,
                                  SChar    * aStrLog,
                                  smLSN    * aPPrvLSN );
    

    
    // 로그 기록후에 수행할 작업들 처리
    static void onAfterWriteLog( idvSQL     * aStatistics,
                                 void       * aTrans,
                                 smrLogHead * aLogHead,
                                 smLSN        aLSN,
                                 UInt         aWrittenLogSize );

    // 로그의 압축이 가능할 경우 압축 실시
    static IDE_RC tryLogCompression( smrCompRes         * aCompRes,
                                     SChar              * aRawLog,
                                     UInt                 aRawLogSize,
                                     SChar             ** aLogToWrite,
                                     UInt               * aLogSizeToWrite,
                                     smOID                aTableOID );

    
 
    /* 로그 header의 previous undo LSN을 설정한다.
     * writeLog 에서 사용한다.  
     */
    static void setLogHdrPrevLSN( void*       aTrans, 
                                  smrLogHead* aLogHead,
                                  smLSN*      aPPrvLSN );
    
    /* 로그 기록할 공간 확보
     * writeLog 에서 사용한다.
     *
     * aLogSize           - [IN]  새로 기록하려는 로그 레코드의 크기
     * aIsLogFileSwitched - [OUT] aLogSize만큼 기록할만한 공간을 확보하던 중에
     *                            로그파일 Switch가 발생했는지의 여부
     */
    static IDE_RC reserveLogSpace( UInt     aLogSize,
                                   idBool * aIsLogFileSwitched );


    // 버퍼에 기록된 로그의 smrLogTail의 판독하여,
    // 로그의 validation 검사를 한다.
    static void validateLogRec( SChar * aStrLog );

    /* Transaction의 Fst, Lst Log LSN을 갱신한다. */
    static void updateTransLSNInfo( idvSQL * aStatistics,
                                    void   * aTrans,
                                    smLSN  * aLSN,
                                    UInt     aLogSize );

//    // Check LogDir Exist
    static IDE_RC checkLogDirExist();

    /******************************************************************************
     * 압축/비압축 로그의 Head에 SN을 세팅한다.
     *
     * [IN] aRawOrCompLog - 압축/비압축 로그
     * [IN] aLogSN - 로그에 기록할 SN
     *****************************************************************************/

    // 압축/비압축 로그의 Head에 SN을 세팅한다.
    static inline void setLogLSN( SChar  * aRawOrCompLog,
                                  smLSN    aLogLSN )
    {
        if ( smrLogComp::isCompressedLog( aRawOrCompLog ) == ID_TRUE )
        {
            smrLogComp::setLogLSN( aRawOrCompLog, aLogLSN );
        }
        else
        {
            /* LSN값을 로그 헤더에 세팅한다. */
            smrLogHeadI::setLSN( (smrLogHead*)aRawOrCompLog, aLogLSN );
        }
    }

    /******************************************************************************
     * 압축/비압축 로그의 Head에 MAGIC값을 세팅한다.
     *
     * [IN] aRawOrCompLog - 압축/비압축 로그
     * [IN] aLSN - 로그의 LSN
     *****************************************************************************/
    static inline void setLogMagic( SChar * aRawOrCompLog,
                                    smLSN * aLSN )
    {
        smMagic sLogMagicValue = smrLogFile::makeMagicNumber( aLSN->mFileNo,
                                                              aLSN->mOffset );

        if ( smrLogComp::isCompressedLog( aRawOrCompLog ) == ID_TRUE )
        {
            smrLogComp::setLogMagic( aRawOrCompLog,
                                     sLogMagicValue );
        }
        else
        {
            /* 나중에 로그를 읽을때 Log의 Validity check를 위해 로그가 기록되는
             * 파일번호와 로그레코드의 파일내 Offset을 이용하여
             * Magic Number를 생성해둔다. */
            smrLogHeadI::setMagic( (smrLogHead *)aRawOrCompLog,
                                   sLogMagicValue );
        }
    }


private:
    /********************  Group Commit 관련 ********************/
    // Transaction이 로그를 기록할 때 
    // Update Transaction의 수를 증가시켜야 하는지 체크한다.
    static inline void checkIncreaseUpdateTxCount( void       * aTrans );

    // Transaction이 로그를 기록할 때 
    // Update Transaction의 수를 감소시켜야 하는지 체크한다.
    static inline void checkDecreaseUpdateTxCount( void       * aTrans,
                                                   smrLogHead * aLogHead );
    
private:
    /********************  FAST UNLOCK LOG ALLOC MUTEX ********************/

    inline static void incCount( UInt aSlotID )
    {
        UInt sIdx = aSlotID / SMR_CHECK_LSN_UPDATE_GROUP;

        idCore::acpAtomicInc32( &mFstChkLSNUpdateCnt[sIdx] );
    }

    inline static void decCount( UInt aSlotID )
    {
        UInt sIdx = aSlotID / SMR_CHECK_LSN_UPDATE_GROUP;
        
        idCore::acpAtomicDec32( &mFstChkLSNUpdateCnt[sIdx] );
    }

    /* BUG-35392 
     * 더미로그를 포함하지 않는 LstlSN, LstWriteLSN을 구하기 위해 사용
     * 해당 더미로그가 작성되기 전 LstlSN, LstWriteLSN 을 저장한다. */
    static inline void setFstCheckLSN( UInt aSlotID )
    {
        smrUncompletedLogInfo     * sFstChkLSN;

        IDE_DASSERT( aSlotID < mFstChkLSNArrSize );

        sFstChkLSN = &mFstChkLSNArr[ aSlotID ];

        IDE_DASSERT( SM_IS_SYNC_LSN_MAX( sFstChkLSN->mLstLSN.mSync ) );
        IDE_DASSERT( SM_IS_SYNC_LSN_MAX( sFstChkLSN->mLstWriteLSN.mSync ) );

        sFstChkLSN->mLstLSN.mSync      = mLstLSN.mSync;
        sFstChkLSN->mLstWriteLSN.mSync = mLstWriteLSN.mSync;
        
        incCount( aSlotID );
    }

    /* BUG-35392 */
    static inline void unsetFstCheckLSN( UInt aSlotID )
    {
        static smrUncompletedLogInfo       sMaxSyncLSN;
        smrUncompletedLogInfo            * sFstChkLSN;

        IDE_DASSERT( aSlotID < mFstChkLSNArrSize );
        
        decCount( aSlotID );

        SM_SYNC_LSN_MAX( sMaxSyncLSN.mLstLSN.mSync );
        SM_SYNC_LSN_MAX( sMaxSyncLSN.mLstWriteLSN.mSync );

        sFstChkLSN = &mFstChkLSNArr[ aSlotID ];

        IDE_DASSERT( !(SM_IS_SYNC_LSN_MAX( sFstChkLSN->mLstLSN.mSync )) );
        IDE_DASSERT( !(SM_IS_SYNC_LSN_MAX( sFstChkLSN->mLstWriteLSN.mSync )) );

        sFstChkLSN->mLstLSN.mSync      = sMaxSyncLSN.mLstLSN.mSync;
        sFstChkLSN->mLstWriteLSN.mSync = sMaxSyncLSN.mLstWriteLSN.mSync;
    }

private:
/******************** 로그 파일 매니저 ********************/
    // 로그파일이 Switch될 때마다 불리운다.
    // 로그파일 Switch Count를 1 증가시키고
    // 체크포인트를 수행해야 할 지의 여부를 결정한다.
    static IDE_RC onLogFileSwitched();

    static inline IDE_RC lockLogSwitchCount()
    { 
        return mLogSwitchCountMutex.lock( NULL ); 
    }
    static inline IDE_RC unlockLogSwitchCount()
    { 
        return mLogSwitchCountMutex.unlock(); 
    }

private:

    //For Logging Mode
    static iduMutex           mMtxLoggingMode;
    static UInt               mMaxLogOffset;

    /* Transaction이 NULL로 들어오는 경우에 사용되는
       로그 압축 리소스 풀
       
       Pool 접근시 Mutex잡는 구간이 짧기 때문에
       Contention은 무시할 수 있는 정도이다.
    */
    static smrCompResPool       mCompResPool;
    
  
    // 로그파일의 끝에 Write시 하나의 쓰레드만 접근이 가능하다.
    // 로그파일의 끝에 대한 동시성 제어를 위한 Mutex
    static iduMutex             mMutex;

    // 이 로그파일 그룹안의 open된 로그파일들을 관리하는
    // 로그파일 관리자
    // 로그파일을 사용하기 전에 미리 준비해 두는 prepare 쓰레드이기도 하다.
    static smrLogFileMgr        mLogFileMgr;

    // 이 로그파일 그룹에 속한 로그파일들의 Flush를 담당하는
    // 로그파일 Flush 쓰레드
    static smrLFThread          mLFThread;

    // 이 로그파일 그룹에 속한 로그파일들을 아카이브 시키는
    // 로그파일 아카이브 쓰레드
    static smrArchThread        mArchiveThread;

    // 로그 파일들이 저장될 로그 디렉토리
    static const SChar        * mLogPath ;

    // 아카이브 로그가 저장될 디렉토리
    // Log File Group당 하나의 unique한 아카이브 디렉토리가 필요하다.
    static const SChar        * mArchivePath ;

    // 이 로그파일에 로그 레코드들을 기록해 나간다.
    static smrLogFile         * mCurLogFile;

     /*   logfile0 에 20까지 로그가 기록되었다면 
     *   +---------------------------------------------
     *   + [FileNo, offset] | .....   [0,10] | [0,20] |        
     *   +----------------------------------------------
     *                                      (A)      (B) 
     *   (A) : mLstWriteLSN    (B) : mLstLSN
     */

    // 마지막 LSN
    // 더미를 포함해서 mCurLogFile에서 로그 레코드가 기록된 마지막 offset
    // mCurLogFile에서 다음 로그 레코드를 기록할 위치.
    static smrLSN4Union             mLstLSN;

    // 마지막으로 Write한 LSN값
    // 더미를 포함해서 mCurLogFile에 로그 레코드가 기록된 LSN
    static smrLSN4Union             mLstWriteLSN;  
 
    // 더미를 포함하지 않은 LstLSN, LstWriteLSN
    static smrUncompletedLogInfo    mUncompletedLSN;

    // 모든 로그파일에 맨 처음으로 기록되는 File Begin Log이다.
    // 이 로그레코드의 용도에 대해서는 smrDef.h를 참조한다.
    static smrFileBeginLog          mFileBeginLog;
    
    // 하나의 로그파일을 다 썼을 때 로그파일의 맨 마지막에 기록하는
    // File End Log이다.
    static smrFileEndLog            mFileEndLog;
    
    /********************  FAST UNLOCK LOG ALLOC MUTEX ********************/
    static iduMutex                 mMutex4NullTrans;

    static smrUncompletedLogInfo  * mFstChkLSNArr;

    static UInt                     mFstChkLSNArrSize;
    
    static UInt                   * mFstChkLSNUpdateCnt;

private:
/********************  Group Commit 관련 ********************/
    // Active Transaction중 Update Transaction의 수
    // 이 값이 LFG_GROUP_COMMIT_UPDATE_TX_COUNT 프로퍼티보다 클 때에만
    // 그룹커밋이 동작한다.
    //
    // 이 변수의 동시성 제어는 로그파일 끝단 뮤텍스인 mMutex 으로 처리한다.
    static UInt                 mUpdateTxCount;

/******************** 로그 파일 매니저 ********************/
    // 하나의 로그파일 그룹 안의 로그파일이 Switch될 때마다 1씩 증가시킨다.
    // 이 값이 smuProperty::getChkptIntervalInLog() 만큼 증가하면
    // 체크포인트를 수행하고 값을 0으로 세팅한다.
    static UInt                 mLogSwitchCount;

    // mLogSwitchCount 변수에 대한 동시성 제어를 위한 Mutex
    static iduMutex             mLogSwitchCountMutex;

    // DebugInfo 용
    static idBool               mAvailable;  

    static smrUCSNChkThread     mUCSNChkThread; /* BUG-35392 */
};

/************************************************************************
  PROJ-1527 Log Optimization

  1~8바이트의 데이터 복사는 memcpy보다 byte assign으로 수행하는 것이
  성능이 더 좋다.
  ( 여러개의 독립적인 assign instruction들이 동시 수행되기 때문 )

  로그 버퍼에 데이터를 복사 할 때 다음과 같은 매크로를 사용하도록 한다.
  ( inline함수로 처리하면 로그 버퍼의 주소를 증가시키는 부분 처리를 위해
    SChar ** 를 써야 하는데, 이렇게 될 경우 memcpy보다 더 느려진다.
    이러한 연유로 inline함수를 쓰지 않고 매크로를 사용하였다. )

 ************************************************************************/
#define SMR_LOG_APPEND_1( aDest, aSrc )    \
{                                          \
    IDE_DASSERT( ID_SIZEOF( aSrc ) == 1 ); \
    (aDest)[0] = aSrc;                     \
    (aDest)   += 1;                        \
}

#define SMR_LOG_APPEND_2( aDest, aSrc )   \
{                                         \
    IDE_DASSERT( ID_SIZEOF( aSrc ) == 2 );\
    (aDest)[0] = ((SChar*)(&aSrc))[0]; \
    (aDest)[1] = ((SChar*)(&aSrc))[1]; \
    (aDest)   += 2;                       \
}

#define SMR_LOG_APPEND_4( aDest, aSrc )   \
{                                         \
    IDE_DASSERT( ID_SIZEOF( aSrc ) == 4 );\
    (aDest)[0] = ((SChar*)(&aSrc))[0]; \
    (aDest)[1] = ((SChar*)(&aSrc))[1]; \
    (aDest)[2] = ((SChar*)(&aSrc))[2]; \
    (aDest)[3] = ((SChar*)(&aSrc))[3]; \
    (aDest)   += 4;                       \
}

#define SMR_LOG_APPEND_8( aDest, aSrc )   \
{                                         \
    IDE_DASSERT( ID_SIZEOF( aSrc ) == 8 );\
    (aDest)[0] = ((SChar*)(&aSrc))[0]; \
    (aDest)[1] = ((SChar*)(&aSrc))[1]; \
    (aDest)[2] = ((SChar*)(&aSrc))[2]; \
    (aDest)[3] = ((SChar*)(&aSrc))[3]; \
    (aDest)[4] = ((SChar*)(&aSrc))[4]; \
    (aDest)[5] = ((SChar*)(&aSrc))[5]; \
    (aDest)[6] = ((SChar*)(&aSrc))[6]; \
    (aDest)[7] = ((SChar*)(&aSrc))[7]; \
    (aDest)   += 8;                       \
}


#if defined(COMPILE_64BIT)
#   define SMR_LOG_APPEND_vULONG SMR_LOG_APPEND_8
#else
#   define SMR_LOG_APPEND_vULONG SMR_LOG_APPEND_4
#endif


#endif /* _O_SMR_LOG_MGR_H_ */

