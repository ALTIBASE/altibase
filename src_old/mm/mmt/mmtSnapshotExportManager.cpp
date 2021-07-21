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

#include <mmtSnapshotExportManager.h>
#include <mmErrorCode.h>
#include <mmtSessionManager.h>
#include <mmcStatementManager.h>
#include <mmm.h>
#include <mmuProperty.h>
#include <qci.h>

iduMutex                  mmtSnapshotExportManager::mMutex;
mmtSnapshotExportThread * mmtSnapshotExportManager::mThread;
mmtSnapshotExportInfo     mmtSnapshotExportManager::mInfo;

mmtSnapshotExportThread::mmtSnapshotExportThread() : idtBaseThread()
{
}

IDE_RC mmtSnapshotExportManager::initialize()
{
    qciSnapshotCallback sCallback;

    mThread            = NULL;
    mInfo.mSCN         = 0;
    mInfo.mBeginTime   = 0;
    mInfo.mBeginMemUsage = 0;
    mInfo.mBeginDiskUndoUsage = 0;
    mInfo.mCurrentTime = 0;

    sCallback.mSnapshotBeginEnd = mmtSnapshotExportManager::beginEndCallback;

    IDE_TEST( qci::setSnapshotCallback( &sCallback ) != IDE_SUCCESS );

    IDE_TEST_RAISE( mMutex.initialize( (SChar *)"MMT_SNAPSHOT_MANGER_MUTEX",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    return  IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT )
    {
        IDE_SET( ideSetErrorCode( mmERR_FATAL_MUTEX_INIT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtSnapshotExportManager::finalize()
{
    if ( mThread != NULL )
    {
        if ( mThread->isRun() == ID_TRUE )
        {
            mThread->stop();
            mThread->finalize();
        }
        else
        {
            /* Nothing to do */
        }

        ( void )iduMemMgr::free( mThread );
        mThread = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    (void)mMutex.destroy();

    return IDE_SUCCESS;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * alter database begin/end snapshot 구문에서 호출하는 Callback 함수이다.
 *
 * begin 시에 SnapshotExport Thread를 생성하고 End시에 Thread를 종료한다.
 */
IDE_RC mmtSnapshotExportManager::beginEndCallback( idBool aIsBegin )
{
    idBool                    sIsInit   = ID_FALSE;
    idBool                    sIsLocked = ID_FALSE;
    mmtSnapshotExportThread * sThread   = NULL;
    ULong                     sMax      = 0;
    ULong                     sUsed     = 0;

    if ( aIsBegin == ID_TRUE )
    {
        /* Begin 시에 이미 iloader 세션이 있다면 error를 발생시킨다 */
        IDE_TEST_RAISE( mmtSessionManager::existClientAppInfoType( MMC_CLIENT_APP_INFO_TYPE_ILOADER ) == ID_TRUE,
                        ERR_BEGIN_SNAPSHOT_HAVE_ILOADER );

        lock();
        sIsLocked = ID_TRUE;

        if ( mThread != NULL )
        {
            /* Threshold 가 넘어서 스스로 종료 될 경우 mThread 메모리는
             * 제거되지않고 mInfo의 Begin 역시 초기화되지 않았다
             */
            if ( mThread->isRun() == ID_FALSE )
            {
                ( void )iduMemMgr::free( mThread );
                mThread = NULL;

                mInfo.mSCN = 0;
                mInfo.mBeginTime = 0;
                mInfo.mBeginMemUsage = 0;
                mInfo.mBeginDiskUndoUsage = 0;
            }
            else
            {
                /* begin 이 2 번 들어온겨우 error를 발생시킨다 */
                IDE_RAISE( ERR_BEGIN_SNAPSHOT );
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDU_FIT_POINT_RAISE( "mmtSnapshotExportManager::initialize::malloc::sThread",
                             ERR_INSUFFICIENT_MEMORY );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_MMT,
                                           ID_SIZEOF( mmtSnapshotExportThread ),
                                           ( void ** )&sThread,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_INSUFFICIENT_MEMORY );

        mThread = new ( sThread )mmtSnapshotExportThread();

        mThread->initialize();

        sIsInit = ID_TRUE;

        /* 처음 실행시 Threshold를 체크한다. */
        IDE_TEST_RAISE( mThread->checkThreshold( ID_TRUE ) == ID_TRUE, ERR_SNAPSHOT_OVER_THRESHOLD );

        IDE_TEST( mThread->start() != IDE_SUCCESS );
        IDE_TEST( mThread->waitToStart() != IDE_SUCCESS );

        IDE_TEST( mThread->beginSnapshot( &mInfo.mSCN ) != IDE_SUCCESS );

        mInfo.mBeginTime = (SLong)mmtSessionManager::getCurrentTime()->sec();

        /* Memory와 Disk Undo Percentage를 구해서 Info에 설정한다.
         * 이 info 는 x$snapshot에서 활용된다 */
        (void)qciMisc::getMemMaxAndUsedSize( &sMax, &sUsed );

        mInfo.mBeginMemUsage = (UInt)( ( ( sUsed / 1024 ) * 100 ) / ( sMax / 1024 ) );

        IDE_TEST( qciMisc::getDiskUndoMaxAndUsedSize( &sMax, &sUsed )
                  != IDE_SUCCESS );

        mInfo.mBeginDiskUndoUsage = (UInt)( ( ( sUsed / 1024 ) * 100 ) / ( sMax / 1024 ) );

        sIsLocked = ID_FALSE;
        unlock();
    }
    else
    {
        /* alter database end snapshot 시에 iloader세션이 있다면 모두 terminate 한다. */
        mmtSessionManager::terminateAllClientAppInoType( MMC_CLIENT_APP_INFO_TYPE_ILOADER );

        lock();
        sIsLocked = ID_TRUE;

        /* 이미 End 가 되었다면 Error를 발생시킨다 */
        IDE_TEST_RAISE( mThread == NULL, ERR_END_SNAPSHOT );

        if ( mThread->isRun() == ID_TRUE )
        {
            /* Thread가 NULL아 아니고 mRun 이 동작중이라면 Thread를 종료하고
             * Info를 초기화한다 */
            IDE_TEST( mThread->endSnapshot() != IDE_SUCCESS );

            mThread->stop();
            mThread->finalize();

            ( void )iduMemMgr::free( mThread );
            mThread = NULL;

            mInfo.mSCN = 0;
            mInfo.mBeginTime = 0;
            mInfo.mBeginMemUsage = 0;
            mInfo.mBeginDiskUndoUsage = 0;
        }
        else
        {
            /* Threshold가 넘어서 End되었다면 Thread memory 를 해재하고
             * mInfo를 초기화한 뒤에 error를 발생시킨다*/
            ( void )iduMemMgr::free( mThread );
            mThread = NULL;

            mInfo.mSCN = 0;
            mInfo.mBeginTime = 0;
            mInfo.mBeginMemUsage = 0;
            mInfo.mBeginDiskUndoUsage = 0;

            IDE_RAISE( ERR_END_SNAPSHOT );
        }

        sIsLocked = ID_FALSE;
        unlock();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BEGIN_SNAPSHOT_HAVE_ILOADER )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INVALID_BEGIN_SNAPSHOT ) );
    }
    IDE_EXCEPTION( ERR_BEGIN_SNAPSHOT )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_BEGIN_SNAPSHOT ) );
    }
    IDE_EXCEPTION( ERR_END_SNAPSHOT )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_END_SNAPSHOT ) );
    }
    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION( ERR_SNAPSHOT_OVER_THRESHOLD )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INVALID_SNAPSHOT_THRESHOLD ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsInit == ID_TRUE )
    {
        if ( mThread->isStarted() == ID_TRUE )
        {
            mThread->stop();
            mThread->finalize();
        }
        else
        {
            /* Nothing to do */
        }

        ( void )iduMemMgr::free( mThread );
        mThread = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsLocked == ID_TRUE )
    {
        unlock();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * 현제 Snapshot 이 begin 되었는지 아닌지 반환한다.
 */
idBool mmtSnapshotExportManager::isBeginSnapshot( void )
{
    idBool sIsBegin = ID_FALSE;

    if ( mThread != NULL )
    {
        if ( mThread->isBegin() == ID_TRUE )
        {
            sIsBegin = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return sIsBegin;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * begin 시에 설정한 SCN을 가져온다.
 */
IDE_RC mmtSnapshotExportManager::getSnapshotSCN( smSCN * aSCN )
{
    lock();

    IDE_TEST_RAISE( mThread == NULL, ERR_INVALID_WORKING );

    IDE_TEST( mThread->getSCN( aSCN ) != IDE_SUCCESS );

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_WORKING )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INVALID_WORKING_SNAPSHOT ) );
    }
    IDE_EXCEPTION_END;

    unlock();

    return IDE_FAILURE;
}


void mmtSnapshotExportThread::initialize()
{
    mRun     = ID_FALSE;
    mIsBegin = ID_FALSE;
}

void mmtSnapshotExportThread::finalize()
{
}

IDE_RC mmtSnapshotExportThread::initializeThread()
{
    return IDE_SUCCESS;
}

void  mmtSnapshotExportThread::finalizeThread()
{
}

void mmtSnapshotExportThread::run()
{
    ULong sCount = 0;
    UInt  sIsOverThreshold = ID_FALSE;

    mRun = ID_TRUE;

    while ( mRun == ID_TRUE )
    {
        if ( mIsBegin == ID_TRUE )
        {
            sIsOverThreshold = ID_FALSE;

            sCount++;

            /* 10초에 한번 모두 Memory와 Disk Undo를 모두 체크한다.
             * Disk Undo Usage 체크는 비용이 매우 많이 들고 오래 걸릴 수 있는
             * 일이고 간단하게 조사할수 없다고 SM으로 부터 의견이 있어서
             * 10초에 한번씩 Check하기로 하였다.
             */
            if ( ( sCount % 10 ) == 0 )
            {
                /* Memory / Disk Undo 모두 Check 한다 */
                sIsOverThreshold = checkThreshold( ID_TRUE );
            }
            else
            {
                /* Memory Check 한다 */
                sIsOverThreshold = checkThreshold( ID_FALSE );
            }

            /* Threshold가 넘었을 경우 iloader Session을 종료하고 endSnapshot
             * 한다*/
            if ( sIsOverThreshold == ID_TRUE )
            {
                mmtSessionManager::terminateAllClientAppInoType( MMC_CLIENT_APP_INFO_TYPE_ILOADER );

                (void)endSnapshot();
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
        idlOS::sleep(1);
    }

    mRun = ID_FALSE;
}

void mmtSnapshotExportThread::stop()
{
    mRun = ID_FALSE;

    IDE_ASSERT( join() == IDE_SUCCESS );
}

idBool mmtSnapshotExportThread::isRun()
{
    return mRun;
}

idBool mmtSnapshotExportThread::isBegin()
{
    return mIsBegin;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * 1. 내부 SCN 저장용 Transaction과 Statement를 begin한다.
 * 2. Inplace update를 하지 않도록 qp에 설정한다.
 */
IDE_RC mmtSnapshotExportThread::beginSnapshot( ULong * aSCN )
{
    smiStatement * sDummySmiStmt = NULL;
    UInt           sStage        = 0;
    UInt           sSmiStmtFlag  = 0;
    ULong          sSCN          = 0;

    IDE_TEST_RAISE( mIsBegin == ID_TRUE, ERR_BEGIN_SNAPSHOT );

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR;

    IDE_TEST( mTrans.initialize() != IDE_SUCCESS );

    sStage++; //1
    IDE_TEST( mTrans.begin( &sDummySmiStmt, NULL )
              != IDE_SUCCESS);

    sStage++; //2
    IDE_TEST( mSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );

    sStage++; //3

    sSCN = SM_SCN_TO_LONG(mSmiStmt.getSCN());

    *aSCN = sSCN;
    ideLog::log( IDE_SERVER_0, "BEGIN SNAPSHOT [TID:%"ID_UINT32_FMT"] [SCN:%"ID_UINT64_FMT"]",
                 mTrans.getTransID(), sSCN );

    qci::setInplaceUpdateDisable( ID_TRUE );
    mIsBegin = ID_TRUE;

    IDL_MEM_BARRIER;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BEGIN_SNAPSHOT )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_BEGIN_SNAPSHOT ) );
    }
    IDE_EXCEPTION_END;

    if ( sStage > 0 )
    {
        switch ( sStage )
        {
            case 3:
                ( void )mSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS );
            case 2:
                ( void )mTrans.commit();
            case 1:
                ( void )mTrans.destroy( NULL );
            default:
                break;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * 1. Inplace update disable 한 부분을 원복한다.
 * 2. transaction과 statement를 종료한다.
 *    이 내부 transaction은 다른 일은 하지 않고 오직 SCN
 *    저장용이기 때문에 항상 SUCCESS 이다.
 */
IDE_RC mmtSnapshotExportThread::endSnapshot( void )
{
    UInt   sStage    = 3;

    IDE_TEST_RAISE( mIsBegin == ID_FALSE, ERR_END_SNAPSHOT );

    qci::setInplaceUpdateDisable( ID_FALSE );

    mIsBegin = ID_FALSE;

    IDL_MEM_BARRIER;

    ideLog::log(IDE_SERVER_0, "END SNAPSHOT");

    sStage--; //2
    IDE_TEST( mSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sStage--; //1
    IDE_TEST( mTrans.commit()
              != IDE_SUCCESS );

    sStage--; //0
    IDE_TEST( mTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_END_SNAPSHOT )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_END_SNAPSHOT ) );
    }
    IDE_EXCEPTION_END;

    if ( sStage > 0 )
    {
        switch ( sStage )
        {
            case 2:
                ( void )mTrans.commit();
            case 1:
                ( void )mTrans.destroy( NULL );
            default:
                break;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * Memory Usage 와 Disk Undo Usage의 Threshold를 check 한다.
 */
idBool mmtSnapshotExportThread::checkThreshold( idBool aCheckAll )
{
    idBool      sIsTrue    = ID_FALSE;
    UInt        sThreshold = 0;
    ULong       sTemp      = 0;
    ULong       sMax       = 0;
    ULong       sUsedSize  = 0;

    qciMisc::getMemMaxAndUsedSize( &sMax, &sUsedSize );
    sThreshold = mmuProperty::getSnapshotMemThreshold();

    sTemp = ( sMax / 100 ) * sThreshold;
    if ( sUsedSize >= sTemp )
    {
        sIsTrue = ID_TRUE;
    }
    else
    {
        if ( aCheckAll == ID_TRUE )
        {
            (void)qciMisc::getDiskUndoMaxAndUsedSize( &sMax, &sUsedSize );
            sThreshold = mmuProperty::getSnapshotDiskUndoThreshold();

            sTemp = ( sMax / 100 ) * sThreshold;
            if ( sUsedSize >= sTemp )
            {
                sIsTrue = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return sIsTrue;
}

IDE_RC mmtSnapshotExportThread::getSCN( smSCN * aSCN )
{
    IDE_TEST_RAISE( ( mRun == ID_FALSE ) || ( mIsBegin == ID_FALSE ), ERR_INVALID_WORKING );

    *aSCN = mSmiStmt.getSCN();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_WORKING )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INVALID_WORKING_SNAPSHOT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtSnapshotExportManager::buildRecordForSnapshot(idvSQL              *,
                                                        void                * aHeader,
                                                        void                *,
                                                        iduFixedTableMemory * aMemory )
{
    ULong sMax   = 0;
    ULong sUsed  = 0;

    lock();
    if ( mThread != NULL )
    {
        /* Threshold가 넘어서 end 된경우 정리해둔다 */
        if ( mThread->isRun() == ID_FALSE )
        {
            mInfo.mSCN = 0;
            mInfo.mBeginTime = 0;
            mInfo.mBeginMemUsage = 0;
            mInfo.mBeginDiskUndoUsage = 0;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }
    unlock();

    mInfo.mCurrentTime = (SLong)mmtSessionManager::getCurrentTime()->sec();

    /* 현재 Memory 와 Disk Undo Percengate를 구해서 mInfo 에 설정후에
     * x$snapshot을 buildRecord 한다 */
    qciMisc::getMemMaxAndUsedSize( &sMax, &sUsed );

    mInfo.mCurrentMemUsage = (UInt)(( ( sUsed / 1024 ) * 100 ) / ( sMax / 1024 ) );

    IDE_TEST( qciMisc::getDiskUndoMaxAndUsedSize( &sMax, &sUsed )
              != IDE_SUCCESS );

    mInfo.mCurrentDiskUndoUsage = (UInt)(( ( sUsed / 1024 ) * 100 ) / ( sMax / 1024 ) );

    IDE_TEST(iduFixedTable::buildRecord(aHeader, aMemory, (void *)&mInfo) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * Fixed Table Definition for SNAPSHOT
 */
static iduFixedTableColDesc gSnapshotExportColDesc[] =
{
    // task
    {
        (SChar *)"SCN",
        offsetof(mmtSnapshotExportInfo, mSCN),
        IDU_FT_SIZEOF(mmtSnapshotExportInfo, mSCN),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"BEGIN_TIME",
        offsetof(mmtSnapshotExportInfo, mBeginTime),
        IDU_FT_SIZEOF(mmtSnapshotExportInfo, mBeginTime),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"BEGIN_MEM_USAGE",
        offsetof(mmtSnapshotExportInfo, mBeginMemUsage),
        IDU_FT_SIZEOF(mmtSnapshotExportInfo, mBeginMemUsage),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"BEGIN_DISK_UNDO_USAGE",
        offsetof(mmtSnapshotExportInfo, mBeginDiskUndoUsage),
        IDU_FT_SIZEOF(mmtSnapshotExportInfo, mBeginDiskUndoUsage),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CURRENT_TIME",
        offsetof(mmtSnapshotExportInfo, mCurrentTime),
        IDU_FT_SIZEOF(mmtSnapshotExportInfo, mCurrentTime),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CURRENT_MEM_USAGE",
        offsetof(mmtSnapshotExportInfo, mCurrentMemUsage),
        IDU_FT_SIZEOF(mmtSnapshotExportInfo, mCurrentMemUsage),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CURRENT_DISK_UNDO_USAGE",
        offsetof(mmtSnapshotExportInfo, mCurrentDiskUndoUsage),
        IDU_FT_SIZEOF(mmtSnapshotExportInfo, mCurrentDiskUndoUsage),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

iduFixedTableDesc gSnapshotExportTableDesc =
{
    (SChar *)"X$SNAPSHOT",
    mmtSnapshotExportManager::buildRecordForSnapshot,
    gSnapshotExportColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

