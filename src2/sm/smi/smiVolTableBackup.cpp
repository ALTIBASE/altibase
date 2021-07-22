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
 
#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <svm.h>
#include <svcDef.h>
#include <svcRecord.h>
#include <smcTable.h>
#include <smi.h>
#include <smiVolTableBackup.h>
#include <smxTrans.h>
#include <svcLob.h>

/* PROJ-2174 Supporting LOB in the volatile tablespace */
#define SMI_MAX_LOB_BACKUP_BUFFER_SIZE (1024*1024)

/***********************************************************************
 * Description : aTable에 대해 Backup 또는 Restore를 준비한다.
 *
 * aTable    - [IN] File에 대한 Table Header
 ***********************************************************************/
IDE_RC smiVolTableBackup::initialize(const void* aTable, SChar * aBackupFileName)
{
    SChar sFileName[SM_MAX_FILE_NAME];

    IDE_DASSERT(aTable != NULL);

    mTableHeader = (smcTableHeader*)((UChar*)aTable + SMP_SLOT_HEADER_SIZE);

    idlOS::snprintf(sFileName,
                    SM_MAX_FILE_NAME,
                    "%s%"ID_vULONG_FMT"%s",
                    smcTable::getBackupDir(), smcTable::getTableOID(mTableHeader),
                    SM_TABLE_BACKUP_EXT);

    IDE_TEST(mFile.initialize(sFileName)
             != IDE_SUCCESS);

    /* BUG-34262 
     * When alter table add/modify/drop column, backup file cannot be
     * deleted. 
     */
    if( aBackupFileName != NULL )
    {
        idlOS::strncpy( aBackupFileName, sFileName, SM_MAX_FILE_NAME );
    } 

    mBuffer       = NULL;
    mOffset       = 0;
    mBufferSize   = 0;
    mArrColumn    = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 할당된 자원을 반납한다.
 *
 ***********************************************************************/
IDE_RC smiVolTableBackup::destroy()
{
    IDE_ASSERT(mBuffer == NULL);
    IDE_TEST(mFile.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
/***********************************************************************
 * Description : aTableOID에 해당하는 Table을 디스크로 내린다.
 *               Filename은 다음과 같이 된다.
 *                - Filename: smcTable::getBackupDir()/smcTable::getTableOID(aTable)
 *                - EXT: SM_TABLE_BACKUP_EXT
 *
 *               File Format
 *                - File Header(smcBackupFileHeader)
 *                - Column List
 *                  Column Cnt (UInt)
 *                  column1 .. column n (smiColumn)
 *                - Data
 *                   row들(column1, column2, column3)
 *                    - LOB Column은 파일에 기록시 뒤 몰아서 기록한다.
 *                      ex) c1: int, c2: lob, c3: lob, c4:string
 *                          c1, c4, c2, c3순으로 백업파일에 기록.
 *
 *                    - Fixed: Data
 *                    - LOB, VAR: Length(UInt), Data
 *
 *                - File Tail(smcBackupFileTail)
 *
 * aStatement    - [IN] Statement
 * aTable        - [IN] Table Header
 * aStatistics   - [IN] 통계정보
 ***********************************************************************/
IDE_RC smiVolTableBackup::backup(smiStatement   * aStatement,
                                 const void     * aTable,
                                 idvSQL         * aStatistics)
{
    smiTableCursor      sCursor;
    UInt                sState      = 0;
    UInt                sCheckCount;
    SChar             * sCurRow;
    smiCursorProperties sCursorProperties;
    smxTrans          * sTrans;
    /* BUG-34262 [SM] When alter table add/modify/drop column, backup file
     * cannot be deleted. 
     * table을 backup하고 mTableHeader->mNullOID를 SM_NULL_OID변경하고 예외
     * 발생시 이전 OID로 복원해야 합니다. 하지만 남는 로그가 redo only여서
     * undo되지않아 mTableHeader->mNullOID이 엉뚱한 값이들어있는 상태가 됩니다.
     * 코드를 삭제하는 대신에 주석처리 합니다.
    scPageID            sCatPageID;
    */
    scGRID              sRowGRID;
    UInt                sRowSize;
    UInt                sRowMaxSize = 0;
    SChar             * sNullRowPtr;
    smiColumn        ** sLobColumnList;
    UInt                sLobColumnCnt; 

    // To fix BUG-14818
    sCursor.initialize();

    SMI_CURSOR_PROP_INIT( &sCursorProperties, aStatistics, NULL );
    sCursorProperties.mLockWaitMicroSec = 0;
    sCursorProperties.mIsUndoLogging    = ID_FALSE;

    sTrans = (smxTrans*)aStatement->mTrans->getTrans();
    /* BUG-34262 [SM] When alter table add/modify/drop column, backup file
     * cannot be deleted. 
     * table을 backup하고 mTableHeader->mNullOID를 SM_NULL_OID변경하고 예외
     * 발생시 이전 OID로 복원해야 합니다. 하지만 남는 로그가 redo only여서
     * undo되지않아 mTableHeader->mNullOID이 엉뚱한 값이들어있는 상태가 됩니다.
     * 코드를 삭제하는 대신에 주석처리 합니다.
 
    sCatPageID = SM_MAKE_PID( mTableHeader->mSelfOID );
     */
    
    sState  = 1;    /* 백업 파일 준비 */

    IDE_TEST_RAISE( smcTable::waitForFileDelete( sTrans->mStatistics,  
                                                 mFile.getFileName() )
                    != IDE_SUCCESS,
                    err_wait_for_file_delete ); 

    /* create backup file and write backup file header */
    IDE_TEST( prepareAccessFile(ID_TRUE/*Write*/) != IDE_SUCCESS );
    
    sState = 2;     /* Lob Column List 생성 */

    /* PROJ-2174 Supporting LOB in the volatile tablespace */
    IDE_TEST( smcTable::makeLobColumnList( mTableHeader,
                                           &sLobColumnList,
                                           &sLobColumnCnt )
              != IDE_SUCCESS );
    
    sState = 3;     /* 백업 파일 header 및 컬럼 정보 쓰기, 커서 open */

    IDE_TEST( appendBackupFileHeader() != IDE_SUCCESS );

    // Table의 smiColumn List정보를 파일에 기록한다.
    IDE_TEST( appendTableColumnInfo() != IDE_SUCCESS );

    // BUGBUG, A4.
    // key filer등 기타 pointer등을 default로 구해서 setting해야 한다.
    IDE_TEST(sCursor.open( aStatement,
                           aTable,
                           NULL,
                           smiGetRowSCN(aTable),
                           NULL,
                           smiGetDefaultKeyRange(),
                           smiGetDefaultKeyRange(),
                           smiGetDefaultFilter(),
                           SMI_LOCK_READ|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                           SMI_SELECT_CURSOR,
                           &sCursorProperties )
             != IDE_SUCCESS);
    
    sState = 4;     /* 실제 data 백업 */

    /* 정상적인 table이면 mNullOID는 반드시 존재해야 한다. */
    IDE_ASSERT( mTableHeader->mNullOID != SM_NULL_OID );

    IDE_ASSERT( smmManager::getOIDPtr( mTableHeader->mSpaceID,
                                       mTableHeader->mNullOID,
                                       (void**)&sNullRowPtr )
                == IDE_SUCCESS );

    // Append NULL Row
    /* PROJ-2174 Supporting LOB in the volatile tablespce */
    IDE_TEST( appendRow( &sCursor,
                         sLobColumnList,
                         sLobColumnCnt,
                         sNullRowPtr,
                         &sRowSize )
             != IDE_SUCCESS );

    if( sRowSize > sRowMaxSize )
    {
        sRowMaxSize = sRowSize;
    }

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow ((const void**)&sCurRow,
                               &sRowGRID,
                               SMI_FIND_NEXT )
              != IDE_SUCCESS );

    sCheckCount = 0;
    while( sCurRow != NULL )
    {
        IDE_ASSERT( sNullRowPtr != sCurRow );

        /* PROJ-2174 Supporting LOB in the volatile tablespace */
        IDE_TEST( appendRow( &sCursor,
                             sLobColumnList,
                             sLobColumnCnt,
                             sCurRow,
                             &sRowSize ) != IDE_SUCCESS );

        if( sRowSize > sRowMaxSize )
        {
            sRowMaxSize = sRowSize;
        }

        IDE_TEST( sCursor.readRow( (const void**)&sCurRow,
                                   &sRowGRID,
                                   SMI_FIND_NEXT )
                 != IDE_SUCCESS);

        sCheckCount++;


        if( sCheckCount == 100 )
        {
            IDE_TEST( iduCheckSessionEvent( sTrans->mStatistics )
                     != IDE_SUCCESS );
            sCheckCount = 0;
        }
    }

    sState = 3;     /* 커서 close, 백업 파일 tail 쓰기 */
   
    IDE_TEST( sCursor.close() != IDE_SUCCESS );
 
    IDE_TEST( appendBackupFileTail( mTableHeader->mSelfOID,
                                    mOffset + ID_SIZEOF( smcBackupFileTail ),
                                    sRowMaxSize ) != IDE_SUCCESS );

    sState = 2;     /* Lob Column List 파괴 */
    
    IDE_TEST( smcTable::destLobColumnList( sLobColumnList )
             != IDE_SUCCESS );

    sState = 1;     /* 백업 파일 Close */

    IDE_TEST( finishAccessFile() != IDE_SUCCESS );

    /* BUG-34262 [SM] When alter table add/modify/drop column, backup file
     * cannot be deleted. 
     * table을 backup하고 mTableHeader->mNullOID를 SM_NULL_OID변경하고 예외
     * 발생시 이전 OID로 복원해야 합니다. 하지만 남는 로그가 redo only여서
     * undo되지않아 mTableHeader->mNullOID이 엉뚱한 값이들어있는 상태가 됩니다.
     * 코드를 삭제하는 대신에 주석처리 합니다.
 
    IDE_TEST( smrUpdate::setNullRow( NULL, // idvSQL
                                     sTrans,
                                     SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                     SM_MAKE_PID( mTableHeader->mSelfOID ),
                                     SM_MAKE_OFFSET( mTableHeader->mSelfOID ) + SMP_SLOT_HEADER_SIZE,
                                     SM_NULL_OID )
             != IDE_SUCCESS );

    mTableHeader->mNullOID = SM_NULL_OID;

    sState = 0;
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                   SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                   sCatPageID )
              != IDE_SUCCESS );
    */
    //====================================================================
    // To Fix BUG-13924
    //====================================================================
    ideLog::log( SM_TRC_LOG_LEVEL_INTERFACE,
                 SM_TRC_INTERFACE_FILE_CREATE,
                 mFile.getFileName() );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_wait_for_file_delete )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_UnableToExecuteAlterTable,
                                 smcTable::getTableOID(mTableHeader)) );
    }
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        switch( sState )
        {
            case 4:     /* 커서 close */
                IDE_ASSERT( sCursor.close() == IDE_SUCCESS );
            case 3:     /* Lob Column List 파괴 */
                IDE_ASSERT( smcTable::destLobColumnList( sLobColumnList )
                            == IDE_SUCCESS );
            case 2:     /* 백업 파일 close */
                IDE_ASSERT( finishAccessFile() == IDE_SUCCESS );

                if( idf::access(mFile.getFileName(), F_OK) == 0 )
                {
                    if( idf::unlink( mFile.getFileName() ) != 0 )
                    {
                        ideLog::log( SM_TRC_LOG_LEVEL_INTERFACE,
                                     SM_TRC_INTERFACE_FILE_UNLINK_ERROR,
                                     mFile.getFileName(), errno );
                    }
                    else
                    {
                        /* nothing to do */
                    }
                }
                else
                {
                    /* nothing to do */
                }

            case 1:
                ;
            /* BUG-34262 [SM] When alter table add/modify/drop column, backup file
             * cannot be deleted.
             * setNullRow와 동일한 이유로 주석처리 됨
                IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(
                                SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                sCatPageID )
                            == IDE_SUCCESS );
            */
        }
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aDstTable에 aTableOID에 해당하는 BackupFile을 읽어서
 *               insert한다.
 *
 * aStatement       - [IN] Statement
 * aDstTable        - [IN] Restore Target Table에 대한 Table Handle
 * aTableOID        - [IN] Source Table OID
 * aSrcColumnList   - [IN] 원본 Table의 Column List
 * aBothColumnList  - [IN] Restore Target Table에 대한 Column List
 * aNewRow          - [IN] Restore Target Table에 대한 Value List
 * aIsUndo          - [IN] Undo시 호출되면 ID_TRUE, 아니면 ID_FALSE
 ***********************************************************************/
IDE_RC smiVolTableBackup::restore(void                  * aTrans,
                                  const void            * aDstTable,
                                  smOID                   aTableOID,
                                  smiColumnList         * aSrcColumnList,
                                  smiColumnList         * aBothColumnList,
                                  smiValue              * aNewRow,
                                  idBool                  aIsUndo,
                                  smiAlterTableCallBack * aCallBack)
{
    smcTableHeader            * sDstTableHeader;
    UInt                        sState      = 0;
    UInt                        sRowCount   = 0;
    UInt                        sCheckCount = 0;
    smxTrans                  * sTrans = (smxTrans*)aTrans;
    smSCN                       sSCN;
    scGRID                      sDummyGRID;
    smxTableInfo              * sTableInfo  = NULL;
    SChar*                      sInsertedRowPtr;
    smcBackupFileTail           sBackupFileTail;
    ULong                       sFenceSize;
    smiTableBackupColumnInfo  * sArrColumnInfo;
    smiTableBackupColumnInfo  * sArrLobColumnInfo;
    UInt                        sLobColumnCnt;
    UInt                        sColumnCnt;
    UInt                        sAddOIDFlag = 0;

    /* BUG-31881 [sm-mem-resource] When executing alter table in MRDB 
     * and using space by other transaction, 
     * The server can not do restart recovery. */
    IDU_FIT_POINT( "1.BUG-31881@smiVolTableBackup::restore" );

    SM_INIT_SCN( &sSCN );

    sDstTableHeader  = (smcTableHeader*)( (UChar*)aDstTable + SMP_SLOT_HEADER_SIZE );

    if ( aIsUndo == ID_TRUE )
    {
        sAddOIDFlag = SM_FLAG_TABLE_RESTORE_UNDO;
    }
    else
    {
        sAddOIDFlag = SM_FLAG_TABLE_RESTORE;
    }

    IDE_TEST( prepareAccessFile( ID_FALSE/*READ*/ ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( getBackupFileTail( &sBackupFileTail ) != IDE_SUCCESS );

    IDE_TEST( checkBackupFileIsValid( aTableOID, &sBackupFileTail )
              != IDE_SUCCESS );

    mOffset = ID_SIZEOF( smcBackupFileHeader );

    /* 기록된 Column정보는 Skip한다. */
    IDE_TEST( skipColumnInfo() != IDE_SUCCESS );

    /* Max Row크기로 버퍼를 할당한다.*/
    IDE_TEST( initBuffer( sBackupFileTail.mMaxRowSize )
              != IDE_SUCCESS );
    sState = 2;

    /* PROJ-2174 Supporting LOB in the volatile tablespace*/
    IDE_TEST( makeColumnList4Res(aDstTable,
                                 aSrcColumnList,
                                 aBothColumnList,
                                 &sArrColumnInfo,
                                 &sColumnCnt,
                                 &sArrLobColumnInfo,
                                 &sLobColumnCnt )
              != IDE_SUCCESS );
    sState = 3;

    if ( aIsUndo == ID_TRUE )
    {
        /* Undo시에는 Record Count를 이 함수에서 원복시켜준다. 때문에 Insert시에
           Table Info를 넘길 필요가 없다. */

        /* Undo 시에는 Null Row를 이미 Free했기 때문에 다시 만들어
           주어야 한다.*/
        IDE_ASSERT(aSrcColumnList == aBothColumnList);

        /* PROJ-2174 Supporting LOB in the volatile tablespace*/
        IDE_TEST( readNullRowAndSet( sTrans,
                                     sDstTableHeader,
                                     sArrColumnInfo,
                                     sColumnCnt,
                                     sArrLobColumnInfo,
                                     sLobColumnCnt,
                                     aNewRow,
                                     aCallBack )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sTrans->getTableInfo( sDstTableHeader->mSelfOID, &sTableInfo )
                  != IDE_SUCCESS );
        /* smcTable::createTable에서 Target Table생성시
           Null Row를 생성했다. 따라서 Skip한다.*/
        IDE_TEST( skipNullRow( aSrcColumnList )
                  != IDE_SUCCESS );
    }
    
    sFenceSize = sBackupFileTail.mSize - ID_SIZEOF(smcBackupFileTail);

    while(mOffset < sFenceSize)
    {
        if ( aCallBack != NULL )
        {
            IDE_TEST( aCallBack->initializeConvert( aCallBack->info )
                      != IDE_SUCCESS );
        }
        
        IDE_TEST( readRowFromBackupFile( sArrColumnInfo,
                                         sColumnCnt,
                                         aNewRow,
                                         aCallBack )
                 != IDE_SUCCESS);

        /* PROJ-1090 Function-based Index */
        if ( aCallBack != NULL )
        {
            IDE_TEST( aCallBack->calculateValue( aNewRow,
                                                 aCallBack->info )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( svcRecord::insertVersion( NULL,
                                            sTrans,
                                            sTableInfo,
                                            sDstTableHeader,
                                            sSCN,
                                            &sInsertedRowPtr,
                                            &sDummyGRID,
                                            aNewRow,
                                            sAddOIDFlag )
                  != IDE_SUCCESS );
        /* 실제 Row 삽입 후 Count */
        sRowCount++;

        /* PROJ-2174 Supporting LOB in the volatile tablespace */
        /* LOB Column 삽입 */
        IDE_TEST( insertLobRow( sTrans,
                                sDstTableHeader,
                                sArrLobColumnInfo,
                                sLobColumnCnt,
                                sInsertedRowPtr,
                                sAddOIDFlag )
                  != IDE_SUCCESS );

        if ( aCallBack != NULL )
        {
            // BUG-42920 DDL print data move progress
            IDE_TEST( aCallBack->printProgressLog( aCallBack->info,
                                                   ID_FALSE )
                      != IDE_SUCCESS );

            IDE_TEST( aCallBack->finalizeConvert( aCallBack->info )
                      != IDE_SUCCESS );
        }
        
        sCheckCount++;


        if( sCheckCount == 100 )
        {
            IDE_TEST( iduCheckSessionEvent( sTrans->mStatistics )
                      != IDE_SUCCESS );
            sCheckCount = 0;
        }
    } // end of while

    // BUG-42920 DDL print data move progress
    if ( aCallBack != NULL )
    {
        IDE_TEST( aCallBack->printProgressLog( aCallBack->info,
                                               ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    sState = 2;
    IDE_TEST( destColumnList4Res( sArrColumnInfo,
                                  sArrLobColumnInfo)
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( destBuffer() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( finishAccessFile() != IDE_SUCCESS );

    if( aIsUndo == ID_TRUE )
    {
        /* BUG-18021: Record Count가 0이 아닌 Table이 Alter Table Add Column을 하다
         * 가 실패하게 되면 Record Count가 0이 됩니다.
         *
         * Rollback시에 Alter Table Add Column과 같은 경우 Table의 Record 갯수가
         * 0개로 되어 있다. 왜냐하면 Drop Table을 수행했기 때문이다. 그리고 위
         * Insert Version시 Record의 갯수는 올라가지 않는다. 때문에 이 함수가
         * Undo시에 호출이 되었다면 테이블의 레코드 갯수를 보정해야 한다. */
        IDE_TEST( smcTable::setRecordCount( sDstTableHeader,
                                            sRowCount )
                  != IDE_SUCCESS );
    }

    /* BUG-16841: [AT-F5-2]  ART sequential test중 ART /Server/mm3/
     * Statement/Rebuild/PrepareRebuild/TruncateTable/
     * truncateTable.sql에서 crash
     * restart recovery시 undo시에 refine시에 빌드되는 temporal index헤더를
     * 접근하는 경우가 발생하여 restart안되었음. restart시에
     * 이런 정보에 접근하는 것을 방지해야 한다.*/
    if( smrRecoveryMgr::isRestart() == ID_FALSE )
    {
        // Index를 삭제한다.
        IDE_TEST( smnManager::dropIndexes( sDstTableHeader )
                  != IDE_SUCCESS );

        // Restore된 Table에 대해서 Index를 재구성한다.
        // PROJ-1629
        IDE_TEST( smnManager::createIndexes( 
                    NULL,
                    sTrans,
                    sDstTableHeader,
                    ID_FALSE, /* aIsRestartRebuild */
                    ID_FALSE,
                    NULL, /* smiSegAttr */
                    NULL) /* smiSegStorageAttr * */
                != IDE_SUCCESS );
    }

    /* BUG-42411 */
    IDE_ERROR( svrLogMgr::removeLogHereafter( sTrans->getVolatileLogEnv(),
                                              SVR_LSN_BEFORE_FIRST )
                == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-30457  memory volatile 테이블에 대한 alter시
     * 공간 부족으로 rollback이 부족한 상황이면 비정상
     * 종료합니다.
     * pageList를 모두 반환했으므로, 이후 대상 Table에
     * 이루어진 Insert Record에 대한 Undo는 필요 없습니다.
     * 이때 Memory Table과 달리, Volatile Table은 Vol 로그를 모두
     * 날려야 합니다.
     * Vol은 로그를 따로 관리하며, 여기에는 DML로그만 기록합니다.
     * 또한 Vol로그는 Mem로그와 독립적으로 기록하고, Mem로그를 모두
     * 수행한 후에 Vol로그만 따로 기록합니다.
     *
     * 따라서 이전 CreateTable시 삽입한 NullRow에 대한 Rollback이
     * CreateTable DDL문과 함께 같이 NTARollback되지 않습니다.
     * 따라서 모든 Vol DML을 날리기 위해 전부 지워야 합니다. */

    /* BUG-30813 [VAL] Volatile Table Restore에 실패하면
     * Volatile Log를 모두 제거해야 합니다.
     *
     * Resotre에 실패하면 무조건 Volatile Log를 제거해야 합니다.
     * Null Row를 삽입하지 않았고, Insert 한 row가 없어도
     * Volatile Log가 존재하여 svrRecoveryMgr::undo에서
     * Volatile Log를 읽어서 svcRecordUndo의 undoInsert를
     * 호출합니다. (valgrind bug로 발생)
     *
     * 따라서 Restore에 실패하면 Volatile Log를
     * 모두 제거해야 합니다. */
    IDE_PUSH();

    IDE_ASSERT( svrLogMgr::removeLogHereafter( sTrans->getVolatileLogEnv(),
                                               SVR_LSN_BEFORE_FIRST )
                == IDE_SUCCESS );

    IDE_POP();

    if(sState != 0)
    {
        IDE_PUSH();
        switch(sState)
        {
            case 3:
                IDE_ASSERT( destColumnList4Res( sArrColumnInfo,
                                                sArrLobColumnInfo )
                            == IDE_SUCCESS );
            case 2:
                IDE_ASSERT( destBuffer() == IDE_SUCCESS );
            case 1:
                IDE_ASSERT( finishAccessFile() == IDE_SUCCESS );
            default:
                break;
        }
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BackupFile로 부터 데이타를 읽어서 Value List를 구성한다.
 *
 * aSrcColumnList   - [IN] Source Table의 Column List
 * aDstColumnList   - [IN] Target Table의 Column List
 * aValue           - [IN] Value List
 * aCallBack        - [IN] Type Conversion CallBack
 ***********************************************************************/
IDE_RC smiVolTableBackup::readRowFromBackupFile(
    smiTableBackupColumnInfo * aArrColumnInfo,
    UInt                       aColumnCnt,
    smiValue                 * aArrValue,
    smiAlterTableCallBack    * aCallBack )
{
    UInt            sColumnOffset;
    smiValue*       sValue;

    smiTableBackupColumnInfo *sColumnInfo;
    smiTableBackupColumnInfo *sFence;

    sColumnOffset  = 0;

    sValue      = aArrValue;
    sColumnInfo = aArrColumnInfo;
    sFence      = sColumnInfo + aColumnCnt;

    for( ; sColumnInfo < sFence; sColumnInfo++ )
    {
        if( sColumnInfo->mIsSkip == ID_FALSE )
        {
            IDE_TEST( readColumnFromBackupFile( sColumnInfo->mSrcColumn,
                                                sValue,
                                                &sColumnOffset )
                      != IDE_SUCCESS);

            if ( aCallBack != NULL )
            {
                // PROJ-1877
                // src column과 dest column의 schema가 달리질 수 있으므로
                // 변환이 필요하다.
                IDE_TEST( aCallBack->convertValue( NULL, /* aStatistics */
                                                   sColumnInfo->mSrcColumn,
                                                   sColumnInfo->mDestColumn,
                                                   sValue,
                                                   aCallBack->info )
                          != IDE_SUCCESS );
            }
            
            sValue++;
        }
        else
        {
            IDE_TEST( skipReadColumn( sColumnInfo->mSrcColumn )
                      != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Backup file로 부터 aColumn이 지정하는 Column을 읽어서
 *               aValue에 저장한다.
 *
 * aColumn        - [IN] Column Pointer
 * aValue         - [IN] Value Pointer
 * aColumnOffset  - [INOUT] 1. IN: mBuffer에 이번 Column이 저장될 위치
 *                          2. OUT: mBuffer에 다음 Column이 저정될 위치
 ***********************************************************************/
IDE_RC smiVolTableBackup::readColumnFromBackupFile(const smiColumn *aColumn,
                                                   smiValue        *aValue,
                                                   UInt            *aColumnOffset)
{
    SInt sColumnType = aColumn->flag & SMI_COLUMN_TYPE_MASK;

    IDE_ASSERT( *aColumnOffset <= mBufferSize );

    aValue->value = mBuffer + *aColumnOffset;

    switch( sColumnType )
    {
        /* PROJ-2174 Supporting LOB in the volatile tablespace */
        case SMI_COLUMN_TYPE_LOB:
            /* LOB Cursor로 별도로 저장하기 때문에 Insert시
             * Empty LOB을 넣는다.*/
            aValue->length = 0;
            aValue->value = NULL;
            break;

        case SMI_COLUMN_TYPE_VARIABLE:
        case SMI_COLUMN_TYPE_VARIABLE_LARGE:
            IDE_TEST(mFile.read(mOffset, &(aValue->length), ID_SIZEOF(UInt))
                     != IDE_SUCCESS);

            mOffset += ID_SIZEOF(UInt);

            if( aValue->length == 0 )
            {
                aValue->value = NULL;
            }
            else
            {
                /* BUG-25640
                 * Variable 또는 Lob 칼럼만 존재하며 값이 Null일 경우,
                 * Row의 Value만 저장하는 mBuffer 또한 Null일 수 있다. */
                IDE_DASSERT( mBuffer != NULL );

                mFile.read(mOffset,
                           (void*)(aValue->value),
                           aValue->length);

                mOffset += aValue->length;
            }
            break;

        case SMI_COLUMN_TYPE_FIXED:
            /* BUG-25640
             * Variable 또는 Lob 칼럼만 존재하며 값이 Null일 경우,
             * Row의 Value만 저장하는 mBuffer 또한 Null일 수 있다. */
            IDE_DASSERT( mBuffer != NULL );

            aValue->length = aColumn->size;

            IDE_TEST(mFile.read(mOffset,
                                (void*)(aValue->value),
                                aColumn->size)
                     != IDE_SUCCESS);

            mOffset += aColumn->size;
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    /* BUG-25121
     * Restore시 백업파일로부터 Row를 읽습니다. 이때 칼럼별로 하나씩 읽습니다.
     * 각 칼럼은 향후 QP에게 전달될 가능성이 있기 때문에 Align에 맞지 않으면
     * Sigbus error가 납니다.
     *
     * 따라서 각 칼럼별로 8byte Align을 맞춰줍니다. 어차피 읽어드린 하나의
     * Row가 잠시 저장될 공간이기 때문에 넉넉하게 8Byte align을 맞춰도
     * 낭비가 아닙니다. */
    *aColumnOffset += idlOS::align8( aValue->length );

    IDE_ASSERT( *aColumnOffset <= mBufferSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Backup File에서 aColumn이 가리키는 데이타는 Skip한다.
 *
 * aColumn - [IN] Column Pointer
 ***********************************************************************/
IDE_RC smiVolTableBackup::skipReadColumn(const smiColumn * aColumn)
{
    SInt sColumnType = aColumn->flag & SMI_COLUMN_TYPE_MASK;
    UInt sLength;

    IDE_DASSERT( aColumn != NULL );

    switch( sColumnType )
    {
        /* PROJ-2174 Supporting LOB in the volatile tablespace */
        case SMI_COLUMN_TYPE_LOB:
            break;

        case SMI_COLUMN_TYPE_VARIABLE:
        case SMI_COLUMN_TYPE_VARIABLE_LARGE:
            IDE_TEST(mFile.read(mOffset, &sLength, ID_SIZEOF(UInt))
                     != IDE_SUCCESS);

            mOffset += ID_SIZEOF(UInt);
            mOffset += sLength;

            break;

        case SMI_COLUMN_TYPE_FIXED:
            mOffset += aColumn->size;
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
/***********************************************************************
 * Description : Backup File에서 aColumn이 가리키는 데이타는 Skip한다.
 *
 * aColumn - [IN] Column Pointer
 ***********************************************************************/
IDE_RC smiVolTableBackup::skipReadLobColumn(const smiColumn * aColumn)
{
    SInt sColumnType = aColumn->flag & SMI_COLUMN_TYPE_MASK;
    UInt sLength;
    UInt sLobFlag;

    IDE_DASSERT( aColumn != NULL );

    IDE_ASSERT( sColumnType == SMI_COLUMN_TYPE_LOB );
    IDE_TEST(mFile.read(mOffset, &sLength, ID_SIZEOF(UInt))
             != IDE_SUCCESS);
    mOffset += ID_SIZEOF(UInt);

    IDE_TEST(mFile.read(mOffset, &sLobFlag, ID_SIZEOF(UInt))
             != IDE_SUCCESS);
    mOffset += ID_SIZEOF(UInt);
    
    mOffset += sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
/***********************************************************************
 * Description : aRowPtr에 있는 LOB Column을 aCursor가 가리키는
 *               Table에 삽입한다.
 *
 * aArrLobColumnInfo - [IN] 백업파일에서 읽어야 할 Lob Column정보
 * aLobColumnCnt     - [IN] 백업파일에서 읽어야 할 Lob Column 갯수
 * aRowPtr           - Insert할 Row Pointer.
 * aAddOIDFlag       - [IN] OID LIST에 추가할지 여부
 ***********************************************************************/
IDE_RC smiVolTableBackup::insertLobRow(
                            void                          * aTrans,
                            void                          * aHeader,
                            smiTableBackupColumnInfo      * aArrLobColumnInfo,
                            UInt                            aLobColumnCnt,
                            SChar                         * aRowPtr,
                            UInt                            aAddOIDFlag )
{
    UInt            sLobLen;
    UInt            sLobFlag;
    UInt            sRemainLobLen;
    UInt            sPieceSize;
    UInt            sOffset;
    smcLobDesc*     sCurLobDesc;
    
    smiTableBackupColumnInfo * sColumnInfo;
    smiTableBackupColumnInfo * sFence;

    IDE_DASSERT( aTrans  != NULL );
    IDE_DASSERT( aRowPtr != NULL );

    sColumnInfo = aArrLobColumnInfo;
    sFence      = sColumnInfo + aLobColumnCnt;

    for( sColumnInfo = aArrLobColumnInfo; sColumnInfo < sFence; sColumnInfo++)
    {
        if( sColumnInfo->mIsSkip == ID_TRUE )
        {
            IDE_TEST(skipReadLobColumn(sColumnInfo->mSrcColumn)
                     != IDE_SUCCESS);

        }
        else
        {
            IDE_TEST(mFile.read(mOffset, &sLobLen, ID_SIZEOF(UInt))
                     != IDE_SUCCESS);
            mOffset += ID_SIZEOF(UInt);

            IDE_TEST(mFile.read(mOffset, &sLobFlag, ID_SIZEOF(UInt))
                     != IDE_SUCCESS);
            mOffset += ID_SIZEOF(UInt);

            sRemainLobLen = sLobLen;

            if( (sLobFlag & SM_VCDESC_NULL_LOB_MASK) !=
                SM_VCDESC_NULL_LOB_TRUE )
            {
                /* BUG-25640
                 * Variable 또는 Lob 칼럼만 존재하며 값이
                 * Null일 경우, Row의 Value만 저장하는
                 * mBuffer 또한 Null일 수 있다.
                 */
                IDE_DASSERT( mBuffer != NULL );
                
                IDE_TEST( svcLob::reserveSpaceInternalAndLogging(
                                                      aTrans,
                                                      (smcTableHeader*)aHeader,
                                                      aRowPtr,
                                                      sColumnInfo->mDestColumn,
                                                      0,  /* aOffset */ 
                                                      sRemainLobLen, /*aNewSize*/
                                                      aAddOIDFlag )
                          != IDE_SUCCESS );

                sPieceSize = mBufferSize;
                sOffset    = 0;

                while(sRemainLobLen > 0)
                {
                    if(sRemainLobLen < sPieceSize)
                    {
                        sPieceSize = sRemainLobLen;
                    }

                    IDE_TEST(mFile.read(mOffset, mBuffer, sPieceSize)
                             != IDE_SUCCESS);
                    mOffset += sPieceSize;


                    IDE_TEST( svcLob::writeInternal( (UChar*)aRowPtr,
                                                     sColumnInfo->mDestColumn,
                                                     sOffset,
                                                     sPieceSize,
                                                     mBuffer )
                              != IDE_SUCCESS );

                    sOffset += sPieceSize;

                    IDE_ASSERT(sRemainLobLen - sPieceSize < sRemainLobLen);
                    sRemainLobLen -= sPieceSize;
                }
            }
            else
            {
                sCurLobDesc = smcRecord::getLOBDesc( sColumnInfo->mDestColumn,
                                                     aRowPtr );

                IDE_ERROR( (sCurLobDesc->flag & SM_VCDESC_NULL_LOB_TRUE)
                           == SM_VCDESC_NULL_LOB_TRUE );
                
                IDE_ERROR( sRemainLobLen == 0 );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BackupFile에 File Header를 mOffset이 가리키는 위치에
 *               Write한다.
 *
 ***********************************************************************/
IDE_RC smiVolTableBackup::appendBackupFileHeader()
{
    smmMemBase            * sMemBase;
    smcBackupFileHeader     sFileHeader;

    /* BUG-16233: [Valgrind] Syscall param pwrite64(buf) points to
       uninitialised byte(s) [smcTableBackupFile.cpp:142]: smcBackupFileHeader에
       모든 Member가 초기화되지 않습니다. */
    idlOS::memset(&sFileHeader, 0, ID_SIZEOF(smcBackupFileHeader));

    sMemBase = smmDatabase::getDicMemBase();

    /*
        주의. Tablespace마다 달라질 수 있는 필드를
        이 함수에서 사용해서는 안된다.
     */

    // fix BUG-10824
    idlOS::memset(&sFileHeader, 0x00, ID_SIZEOF(smcBackupFileHeader));

    // BUG-20048
    idlOS::snprintf(sFileHeader.mType,
                    SM_TABLE_BACKUP_TYPE_SIZE,
                    SM_VOLTABLE_BACKUP_STR);

    idlOS::memcpy(sFileHeader.mProductSignature,
                  sMemBase->mProductSignature,
                  IDU_SYSTEM_INFO_LENGTH);

    idlOS::memcpy(sFileHeader.mDbname,
                  sMemBase->mDBname,
                  SM_MAX_DB_NAME);

    idlOS::memcpy((void*)&(sFileHeader.mVersionID),
                  (void*)&(sMemBase->mVersionID),
                  ID_SIZEOF(UInt));

    idlOS::memcpy((void*)&(sFileHeader.mCompileBit),
                  (void*)&(sMemBase->mCompileBit),
                  ID_SIZEOF(UInt));

    idlOS::memcpy((void*)&(sFileHeader.mBigEndian),
                  (void*)&(sMemBase->mBigEndian),
                  ID_SIZEOF(idBool));

    idlOS::memcpy((void*)&(sFileHeader.mLogSize),
                  (void*)&(sMemBase->mLogSize),
                  ID_SIZEOF(vULong));

    idlOS::memcpy((void*)&(sFileHeader.mTableOID),
                  (void*)&(mTableHeader->mSelfOID),
                  ID_SIZEOF(smOID));

    IDE_TEST(writeToBackupFile((UChar*)&sFileHeader, ID_SIZEOF(smcBackupFileHeader))
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BackupFile에 File Tail을 mOffset이 가리키는 위치에
 *               Write한다.
 *
 ***********************************************************************/
IDE_RC smiVolTableBackup::appendBackupFileTail( smOID   aTableOID,
                                                ULong   aFileSize,
                                                UInt    aRowMaxSize )
{
    smcBackupFileTail sFileTail;

    /* BUG-16233: [Valgrind] Syscall param pwrite64(buf) points to
       uninitialised byte(s) [smcTableBackupFile.cpp:142]: smcBackupFileTail에
       모든 Member가 초기화되지 않습니다. */
    idlOS::memset(&sFileTail, 0, ID_SIZEOF(smcBackupFileTail));

    idlOS::memcpy(&(sFileTail.mTableOID), &aTableOID, ID_SIZEOF(smOID));
    idlOS::memcpy(&(sFileTail.mSize), &aFileSize, ID_SIZEOF(ULong));
    idlOS::memcpy(&(sFileTail.mMaxRowSize), &aRowMaxSize, ID_SIZEOF(UInt));

    IDE_TEST(writeToBackupFile(&sFileTail, ID_SIZEOF(smcBackupFileTail))
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
/***********************************************************************
 * Description : BackupFile에 mOffset이 가리키는 위치에 aRowPtr를 Write한다.
 *
 *     * Row Format
 *       Fixed Column: Data
 *       VAR, LOB    : Length(UInt) | Data
 *
 * aTableCursor - [IN]: Backup할 table에 대해 open된 table cursor
 * aRowPtr      - [IN]: Write할 row pointer
 * aRowSize     - [OUT]: File에 Write한 크기
 ***********************************************************************/
IDE_RC smiVolTableBackup::appendRow(smiTableCursor * aTableCursor,
                                    smiColumn     ** aArrLobColumn,
                                    UInt             aLobColumnCnt,
                                    SChar          * aRowPtr,
                                    UInt           * aRowSize)
{
    UInt        sColumnCnt;
    UInt        sColumnLen;
    UInt        i;
    SInt        sColumnType;
    UInt        sRowSize    = 0;
    SChar       sVarColumnData[SMP_VC_PIECE_MAX_SIZE];
    UInt        sRemainedReadSize;
    UInt        sReadSize;
    SChar*      sColumnPtr;
    UInt        sReadPos;
    UChar*      sLobBuffer      = NULL;
    idBool      sLobBufferAlloc = ID_FALSE;
    UInt        sLobBackupBufferSize;
    smcLobDesc* sLOBDesc;

    const smiColumn * sCurColumn;

    IDE_DASSERT( aTableCursor != NULL );
    IDE_DASSERT( aRowPtr      != NULL );
    IDE_DASSERT( aRowSize     != NULL );

    sColumnCnt = mTableHeader->mColumnCount;

    for( i = 0; i < sColumnCnt; i++ )
    {
        sCurColumn    = smcTable::getColumn( mTableHeader, i );
        sColumnLen    = svcRecord::getColumnLen( sCurColumn, aRowPtr );
        sColumnType = sCurColumn->flag & SMI_COLUMN_TYPE_MASK;

        switch( sColumnType )
        {
            case SMI_COLUMN_TYPE_VARIABLE:
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                /* BUG-25121
                 * sRowSize는 순수하게 Row의 최대 크기를 알아내기 위해
                 * 사용됩니다. Row의 최대 크기를 파일에 저장하고,
                 * 이후 Restore할때 Row 임시 버퍼의 크기로 사용됩니다. */
                sRowSize += idlOS::align8( sColumnLen );

                /* Column Len : Data */
                IDE_TEST( writeToBackupFile(&sColumnLen, ID_SIZEOF(UInt))
                          != IDE_SUCCESS );

                sRemainedReadSize   = sColumnLen;
                sReadPos            = 0;

                // To fix BUG-22745
                while( sRemainedReadSize > 0 )
                {
                    sReadSize = SMP_VC_PIECE_MAX_SIZE;

                    if( sRemainedReadSize < SMP_VC_PIECE_MAX_SIZE )
                    {
                        sReadSize = sRemainedReadSize;
                    }

                    sColumnPtr = svcRecord::getVarRow( aRowPtr,
                                                       sCurColumn,
                                                       sReadPos,
                                                       &sReadSize,
                                                       (SChar*)sVarColumnData,
                                                       ID_FALSE );

                    //Codesonar
                    IDE_ASSERT( sColumnPtr != NULL );

                    IDE_TEST( writeToBackupFile( sColumnPtr, sReadSize )
                              != IDE_SUCCESS );

                    sRemainedReadSize -= sReadSize;
                    sReadPos          += sReadSize;
                }
                break;

            case SMI_COLUMN_TYPE_FIXED:
                /* BUG-25121
                 * sRowSize는 순수하게 Row의 최대 크기를 알아내기 위해
                 * 사용됩니다. Row의 최대 크기를 파일에 저장하고,
                 * 이후 Restore할때 Row 임시 버퍼의 크기로 사용됩니다. */
                sRowSize += idlOS::align8( sColumnLen );

                /* Data */
                IDE_TEST( writeToBackupFile( aRowPtr + sCurColumn->offset, sColumnLen )
                          != IDE_SUCCESS );
                break;

            default:
                IDE_ASSERT(sColumnType == SMI_COLUMN_TYPE_LOB);
                break;
        }
    }// for

    /* PROJ-2174 Supporting LOB in the volatile tablespace */
    if (aLobColumnCnt > 0)
    {
        /* BUG-25711
         * lob컬럼을 backup하기 위해 버퍼가 필요하다.
         * 메모리를 최대한 요율적으로 할당하기 위해
         * 모든 lob column들의 length를 얻어 가장 큰 컬럼에 맞게
         * 할당한다.
         * 단, lob column중 SMI_MAX_LOB_BACKUP_BUFFER_SIZE보다
         * 큰게 있으면 버퍼를 SMI_MAX_LOB_BACKUP_SIZE 만큼
         * 할당한다. */

        sLobBackupBufferSize = 0;
        for(i = 0; i < aLobColumnCnt; i++)
        {
            sCurColumn    = aArrLobColumn[i];
            sColumnLen    = svcRecord::getColumnLen(sCurColumn, aRowPtr);

            if (sColumnLen >= SMI_MAX_LOB_BACKUP_BUFFER_SIZE)
            {
                sLobBackupBufferSize = SMI_MAX_LOB_BACKUP_BUFFER_SIZE;
                break;
            }

            if (sLobBackupBufferSize < sColumnLen)
            {
                sLobBackupBufferSize = sColumnLen;
            }
        }

        /* lob 컬럼의 길이가 앞에서 계산한 lob을 제외한 row
         * 길이보다 길다. 이런경우 restore 할 때 lob restore시
         * 충분한 공간 확보를 위해 sRowSize를 재설정한다.
         * 혹은 lob을 제외한 컬럼이 하나도 없으면 sRowSize는
         * 0 이 될 것인데, 그러면 lob컬럼을 restore 할 메모리를
         * 할당할지 못하기 때문에 sRowSize를 재설정해야 한다.*/
        if (sLobBackupBufferSize > sRowSize)
        {
            sRowSize = sLobBackupBufferSize;
        }

        /* lob column length 가 0 일 수 있다.
         * 이 경우엔 메모리를 할당하지 않는다.
         * appendLobColumn() 함수내에서는 이에 대해 고려가
         * 되어 있다. */
        if (sLobBackupBufferSize > 0)
        {
            /* smiVolTableBackup_appendRow_malloc_LobBuffer.tc */
            IDU_FIT_POINT("smiVolTableBackup::appendRow::malloc::LobBuffer");
            IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SMC,
                                       sLobBackupBufferSize,
                                       (void**)&sLobBuffer,
                                       IDU_MEM_FORCE)
                     != IDE_SUCCESS);
            sLobBufferAlloc = ID_TRUE;
        }

        for(i = 0; i < aLobColumnCnt; i++)
        {
            sCurColumn    = aArrLobColumn[i];
            sColumnLen    = svcRecord::getColumnLen(sCurColumn, aRowPtr);
            sLOBDesc      = smcRecord::getLOBDesc(sCurColumn, aRowPtr);

            /* LOB Column의 Length가 무지하게 클 수 있다.
             * 때문에 특정크기단위로 여러번 데이터를 읽어서
             * 처리해야 한다. */
            if((sCurColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB)
            {
                IDE_TEST(appendLobColumn(NULL, /* idvSQL* */
                                         aTableCursor,
                                         sLobBuffer,
                                         sLobBackupBufferSize,
                                         sCurColumn,
                                         sColumnLen,
                                         sLOBDesc->flag,
                                         aRowPtr)
                         != IDE_SUCCESS);
            }
        }// for

        if (sLobBufferAlloc == ID_TRUE)
        {
            sLobBufferAlloc = ID_FALSE;
            IDE_TEST(iduMemMgr::free(sLobBuffer) != IDE_SUCCESS);
        }

    }//if

    *aRowSize = sRowSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sLobBufferAlloc == ID_TRUE)
    {
        IDE_ASSERT(iduMemMgr::free(sLobBuffer) == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
/***********************************************************************
 * Description : Backup file에 aRow에서 aLobColumn이 가리키는 데이타를
 *               기록한다.
 *      Format: Length(UInt) | Data
 *
 * aTableCursor - [IN] Table Cursor
 * aLobColumn   - [IN] LOB Column정º??
 * aLobLen      - [IN] LOB Column길이
 * aRow         - [IN] LOB Column을 가지는 Row
 ***********************************************************************/
IDE_RC smiVolTableBackup::appendLobColumn(idvSQL          * aStatistics,
                                          smiTableCursor  * aTableCursor,
                                          UChar           * aBuffer,
                                          UInt              aBufferSize,
                                          const smiColumn * aLobColumn,
                                          UInt              aLobLen,
                                          UInt              aLobFlag,
                                          SChar           * aRow)
{
    smLobLocator    sLOBLocator;
    UInt            sLOBInfo    = 0;
    UInt            sOffset     = 0;
    UInt            sPieceSize;
    UInt            sReadLength;

    IDE_TEST( writeToBackupFile(&aLobLen, ID_SIZEOF(UInt))
              != IDE_SUCCESS );

    IDE_TEST( writeToBackupFile(&aLobFlag, ID_SIZEOF(UInt))
              != IDE_SUCCESS );

    IDE_ASSERT(smiLob::openLobCursorWithRow(aTableCursor,
                                            aRow,
                                            aLobColumn,
                                            sLOBInfo,
                                            SMI_LOB_READ_MODE,
                                            &sLOBLocator)
               == IDE_SUCCESS);

    sPieceSize = aBufferSize;

    while(aLobLen > 0)
    {
        IDE_ASSERT(aBuffer != NULL);

        if(aLobLen < sPieceSize)
        {
            sPieceSize = aLobLen;
        }

        IDE_ASSERT(smiLob::read(aStatistics,
                                sLOBLocator,
                                sOffset,
                                sPieceSize,
                                aBuffer,
                                &sReadLength)
                   == IDE_SUCCESS);

        IDE_TEST( writeToBackupFile(aBuffer, sReadLength)
                  != IDE_SUCCESS );

        sOffset += sPieceSize;
        aLobLen -= sPieceSize;
    }

    IDE_TEST(smiLob::closeLobCursor( aStatistics, sLOBLocator )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aNeedSpace공간이 생길때까지 기다린다.
 *
 * aNeedSpace - [IN] 필요한 디스크 공간(Byte)
 ***********************************************************************/
IDE_RC smiVolTableBackup::waitForSpace(UInt aNeedSpace)
{
    SInt sSystemErrno;

    sSystemErrno = ideGetSystemErrno();

    IDE_TEST(sSystemErrno != 0 && sSystemErrno != ENOSPC);

    while (idlVA::getDiskFreeSpace(smcTable::getBackupDir()) < aNeedSpace)
    {
        gSmiGlobalCallBackList.setEmergencyFunc(SMI_EMERGENCY_DB_SET);
        if (smrRecoveryMgr::isFinish() == ID_TRUE)
        {
            ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                        SM_TRC_INTERFACE_WAITFOR_FATAL1);
            idlOS::exit(0);
        }
        else
        {
            IDE_WARNING(SM_TRC_LOG_LEVEL_WARNNING,
                        SM_TRC_INTERFACE_WAITFOR_FATAL2);
            idlOS::sleep(2);
        }
        gSmiGlobalCallBackList.clrEmergencyFunc(SMI_EMERGENCY_DB_CLR);
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : aTableOID 에 해당하는 백업 파일이 올바른지 Check한다.
 *
 * aTableOID - [IN] Table OID
 * aFileTail - [IN] File Tail
 ***********************************************************************/
IDE_RC smiVolTableBackup::checkBackupFileIsValid(smOID               aTableOID,
                                                 smcBackupFileTail * aFileTail)
{
    smcBackupFileHeader   sFileHeader;
    smmMemBase          * sMemBase;
    ULong                 sFileSize = 0;

    sMemBase = smmDatabase::getDicMemBase();

    IDE_TEST(mFile.read(0, (void*)&sFileHeader, ID_SIZEOF(smcBackupFileHeader))
             != IDE_SUCCESS);

    // BUG-20048
    IDE_TEST_RAISE(0 != idlOS::strncmp(sFileHeader.mType,
                                       SM_VOLTABLE_BACKUP_STR,
                                       SM_TABLE_BACKUP_TYPE_SIZE),
                   err_invalid_backupfile);

    IDE_TEST_RAISE(idlOS::memcmp(sFileHeader.mProductSignature, sMemBase->mProductSignature,
                                 IDU_SYSTEM_INFO_LENGTH) != 0, err_invalid_backupfile);

    IDE_TEST_RAISE(idlOS::memcmp(sFileHeader.mDbname,
                                 sMemBase->mDBname,
                                 SM_MAX_DB_NAME) != 0,
                   err_invalid_backupfile);

    IDE_TEST_RAISE(sFileHeader.mVersionID != sMemBase->mVersionID,
                   err_invalid_backupfile);

    IDE_TEST_RAISE(sFileHeader.mCompileBit != sMemBase->mCompileBit,
                   err_invalid_backupfile);

    IDE_TEST_RAISE(sFileHeader.mBigEndian != sMemBase->mBigEndian,
                   err_invalid_backupfile);

    IDE_TEST_RAISE(aFileTail->mTableOID != aTableOID,
                   err_invalid_backupfile);

    IDE_TEST(mFile.getFileSize(&sFileSize) != IDE_SUCCESS);

    IDE_TEST_RAISE(aFileTail->mSize != sFileSize,
                   err_invalid_backupfile);

    IDE_TEST_RAISE(aFileTail->mTableOID != aTableOID,
                   err_invalid_backupfile);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_backupfile);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidBackupFile, mFile.getFileName()));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 백업 파일의 Tail을 가져온다.
 *
 * aBackupFileTail - [OUT] 백업파일 Tail
 ***********************************************************************/
IDE_RC smiVolTableBackup::getBackupFileTail(smcBackupFileTail * aBackupFileTail)
{
    ULong sFileSize = 0;

    IDE_TEST(mFile.getFileSize(&sFileSize) != IDE_SUCCESS);
    IDE_TEST(mFile.read(sFileSize - ID_SIZEOF(smcBackupFileTail),
                        (void *)aBackupFileTail,
                        ID_SIZEOF(smcBackupFileTail))
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
/***********************************************************************
 * Description : 백업 파일에서 Null Row를 읽어서 aTableHeader가
 *               가리키는 테이블에 삽입하고 NULL Row로 Setting한다.
 *               주의 : UNDO시에만 사용되는 함수이다.
 *
 * aTrans       - [IN] Transaction Pointer
 * aTableInfo   - [IN] Table Info
 * aTableHeader - [IN] Table Header
 * aColumnList  - [IN] Column List
 * aValueList   - [IN] Value List
 ***********************************************************************/
IDE_RC smiVolTableBackup::readNullRowAndSet( void                     * aTrans,
                                             void                     * aTableHeader,
                                             smiTableBackupColumnInfo * aArrColumnInfo,
                                             UInt                       aColumnCnt,
                                             smiTableBackupColumnInfo * aArrLobColumnInfo,
                                             UInt                       aLobColumnCnt,
                                             smiValue                 * aValueList,
                                             smiAlterTableCallBack    * aCallBack )
{
    smSCN   sNullRowSCN;
    smOID   sNullRowOID;
    SChar * sInsertedRowPtr = NULL;

    smcTableHeader * sTableHeader = (smcTableHeader*)aTableHeader;
     
    SM_SET_SCN_NULL_ROW( &sNullRowSCN );

    if ( aCallBack != NULL )
    {
        IDE_TEST( aCallBack->initializeConvert( aCallBack->info )
                  != IDE_SUCCESS );
    }
        
    IDE_TEST(readRowFromBackupFile(aArrColumnInfo,
                                   aColumnCnt,
                                   aValueList,
                                   aCallBack )
             != IDE_SUCCESS);

    IDE_TEST( svcRecord::makeNullRow( aTrans,
                                      sTableHeader,
                                      sNullRowSCN,
                                      aValueList,
                                      SM_FLAG_MAKE_NULLROW_UNDO,
                                      &sNullRowOID )
              != IDE_SUCCESS );

    /* PROJ-2174 Supporting LOB in the volatile tablespace */
    IDE_TEST( smmManager::getOIDPtr( sTableHeader->mSpaceID,
                                     sNullRowOID,
                                     (void**)&sInsertedRowPtr)
              != IDE_SUCCESS );

    /* LOB Column 삽입 */
    IDE_TEST(insertLobRow(aTrans,
                          sTableHeader,
                          aArrLobColumnInfo,
                          aLobColumnCnt,
                          sInsertedRowPtr,
                          SM_FLAG_MAKE_NULLROW_UNDO)
            != IDE_SUCCESS);

    /*첫번째 Row는 Null Row이다.*/
    IDE_TEST( smcTable::setNullRow(aTrans,
                                   sTableHeader,
                                   SMI_GET_TABLE_TYPE( sTableHeader ),
                                   &sNullRowOID)
              != IDE_SUCCESS );

    if ( aCallBack != NULL )
    {
        IDE_TEST( aCallBack->finalizeConvert( aCallBack->info )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Backup File에서 NULL값을 읽지 않고 Skip하게 한다.
 *
 * aColumnList - [IN] Table의 Column List
 ***********************************************************************/
IDE_RC smiVolTableBackup::skipNullRow(smiColumnList * aColumnList)
{
    SInt            sColumnType;
    smiColumnList * sCurColumnList = NULL;
    UInt            sColLen;
    UInt            sLobFlag;

    IDE_DASSERT( aColumnList != NULL );

    sCurColumnList  = aColumnList;

    while(sCurColumnList != NULL)
    {
        sColumnType =
            sCurColumnList->column->flag & SMI_COLUMN_TYPE_MASK;

        switch( sColumnType )
        {
            case SMI_COLUMN_TYPE_VARIABLE:
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                IDE_TEST(mFile.read(mOffset, &sColLen, ID_SIZEOF(UInt))
                         != IDE_SUCCESS);

                mOffset += ID_SIZEOF(UInt);
                mOffset += sColLen;
                break;

            case SMI_COLUMN_TYPE_FIXED:
                mOffset += sCurColumnList->column->size;
                break;

            default:
                IDE_ASSERT(sColumnType == SMI_COLUMN_TYPE_LOB);
                break;
        }

        sCurColumnList = sCurColumnList->next;
    }

    sCurColumnList  = aColumnList;

    while(sCurColumnList != NULL)
    {
        sColumnType =
            sCurColumnList->column->flag & SMI_COLUMN_TYPE_MASK;

        /* PROJ-2174 Supporting LOB in the volatile tablespace */
        if( sColumnType == SMI_COLUMN_TYPE_LOB )
        {
            IDE_TEST(mFile.read(mOffset, &sColLen, ID_SIZEOF(UInt))
                     != IDE_SUCCESS );
            mOffset += ID_SIZEOF(UInt);
                       
            IDE_TEST(mFile.read(mOffset, &sLobFlag, ID_SIZEOF(UInt))
                     != IDE_SUCCESS);
            mOffset += ID_SIZEOF(UInt);

            mOffset += sColLen;
            /* Caution!!: LOB Column의 Null값은 Length가 0이다. 하지만 그것은
             * MT가 정하는 것이므로 나중에 바뀔수 있다. 일단은 정확성 Check를
             * 위해서 ASSERT를 건다. Null값이 바뀌면 이부분을 수정해야 한다.*/
            IDE_ASSERT(sColLen == 0);
        }

        sCurColumnList = sCurColumnList->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
/***********************************************************************
 * Description : Column List에 할당된 메모리를 반납한다.
 *
 * aSrcColumnList    - [IN] Source Table Column List
 * aBothColumnList   - [IN] Source Table과 Destination Table에 공통된 Source Table Column List
 * aArrColumnInfo    - [OUT] 백업파일에서 읽어야 할 Column정보
 * aColumnCnt        - [OUT] 컬럼 갯수
 *
 * aArrLobColumnInfo - [OUT] 백업파일에서 읽어야 할 Lob Column정보
 * aLobColumnCnt     - [OUT] Lob 컬럼 갯수
 ***********************************************************************/
IDE_RC smiVolTableBackup::makeColumnList4Res(const void                * aDstTable,
                                             smiColumnList             * aSrcColumnList,
                                             smiColumnList             * aBothColumnList,
                                             smiTableBackupColumnInfo ** aArrColumnInfo,
                                             UInt                      * aColumnCnt,
                                             smiTableBackupColumnInfo ** aArrLobColumnInfo,
                                             UInt                      * aLobColumnCnt)

{
    smiTableBackupColumnInfo  * sArrColumnInfo      = NULL;
    smiTableBackupColumnInfo  * sArrLobColumnInfo   = NULL;

    smiColumnList   * sSrcColumnList    = NULL;
    smiColumnList   * sBothColumnList   = NULL;
    const smiColumn * sCurColumn        = NULL;
    SInt              sColumnType;
    UInt              sColumnCnt = smcTable::getColumnCount(mTableHeader);
    UInt              sLobColumnCnt = smcTable::getLobColumnCount(mTableHeader);
    UInt              i, j;
    smcTableHeader  * sDstTableHeader;
    UInt              sColumnIdx        = 0;

    sDstTableHeader  = (smcTableHeader*)( (UChar*)aDstTable + SMP_SLOT_HEADER_SIZE );

    *aColumnCnt     = sColumnCnt;
    *aLobColumnCnt  = sLobColumnCnt;

    *aArrColumnInfo     = NULL;
    *aArrLobColumnInfo  = NULL;

    if( sColumnCnt != 0 )
    {
        /* smiVolTableBackup_makeColumnList4Res_malloc_ArrColumnInfo.tc */
        IDU_FIT_POINT("smiVolTableBackup::makeColumnList4Res::malloc::ArrColumnInfo");
        IDE_TEST(iduMemMgr::malloc(
                     IDU_MEM_SM_SMC,
                     (ULong)sColumnCnt *
                     ID_SIZEOF(smiTableBackupColumnInfo),
                     (void**)aArrColumnInfo,
                     IDU_MEM_FORCE)
                 != IDE_SUCCESS);
        sArrColumnInfo = *aArrColumnInfo;
    }

    if ( sLobColumnCnt != 0 )
    {
        /* smiVolTableBackup_makeColumnList4Res_malloc_ArrLobColumnInfo */
        IDU_FIT_POINT("smiVolTableBackup::makeColumnList4Res::malloc::ArrLobColumnInfo");
        IDE_TEST(iduMemMgr::malloc(
                     IDU_MEM_SM_SMC,
                     (ULong)sLobColumnCnt *
                     ID_SIZEOF(smiTableBackupColumnInfo),
                     (void**)aArrLobColumnInfo,
                     IDU_MEM_FORCE)
                 != IDE_SUCCESS);
        sArrLobColumnInfo = *aArrLobColumnInfo;
    }

    sSrcColumnList  = aSrcColumnList;
    sBothColumnList  = aBothColumnList;

    sCurColumn  = smcTable::getColumn(sDstTableHeader, sColumnIdx);
    sColumnIdx++;

    i = 0;
    j = 0;
    while( sSrcColumnList != NULL )
    {
        sColumnType =
            sSrcColumnList->column->flag & SMI_COLUMN_TYPE_MASK;

        IDE_ASSERT( sArrColumnInfo != NULL );
        
        sArrColumnInfo[i].mIsSkip    = ID_TRUE;
        sArrColumnInfo[i].mSrcColumn = sSrcColumnList->column;

        if( sColumnType == SMI_COLUMN_TYPE_LOB )
        {
            IDE_ASSERT( sArrLobColumnInfo != NULL );

            sArrLobColumnInfo[j].mSrcColumn = sSrcColumnList->column;
            sArrLobColumnInfo[j].mIsSkip    = ID_TRUE;
        }

        if ( sCurColumn != NULL )
        {
            if ( sSrcColumnList->column->id == sBothColumnList->column->id )
            {
                sArrColumnInfo[i].mDestColumn = sCurColumn;
                sArrColumnInfo[i].mIsSkip     = ID_FALSE;

                if(sColumnType == SMI_COLUMN_TYPE_LOB)
                {
                    sArrLobColumnInfo[j].mIsSkip     = ID_FALSE;
                    sArrLobColumnInfo[j].mDestColumn = sCurColumn;
                }

                if( sColumnIdx < smcTable::getColumnCount(sDstTableHeader) )
                {
                    sCurColumn  = smcTable::getColumn(sDstTableHeader, sColumnIdx);
                }
                else
                {
                    sCurColumn = NULL;
                }
                sColumnIdx++;
                sBothColumnList = sBothColumnList->next;
            }
        }

        if(sColumnType == SMI_COLUMN_TYPE_LOB)
        {
            j++;
        }

        i++;
        sSrcColumnList = sSrcColumnList->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( *aArrColumnInfo != NULL )
    {
        IDE_PUSH();
        (void)iduMemMgr::free(*aArrColumnInfo);
        IDE_POP();
    }

    if( *aArrLobColumnInfo != NULL )
    {
        IDE_PUSH();
        (void)iduMemMgr::free(*aArrLobColumnInfo);
        IDE_POP();
    }

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
/***********************************************************************
 * Description : Column List에 할당된 메모리를 반납한다.
 *
 * aArrColumn    - [IN] Column List
 * aArrLobColumn - [IN] Lob Column List
 ***********************************************************************/
IDE_RC smiVolTableBackup::destColumnList4Res( void    * aArrColumn,
                                              void    * aArrLobColumn )
{
    if( aArrColumn != NULL )
    {
        IDE_ASSERT( iduMemMgr::free(aArrColumn)
                    == IDE_SUCCESS );
    }

    if( aArrLobColumn != NULL )
    {
        IDE_ASSERT( iduMemMgr::free(aArrLobColumn)
                    == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Column List를 File Header다음에 기록한다.
 ***********************************************************************/
IDE_RC smiVolTableBackup::appendTableColumnInfo()
{
    UInt  sColumnCnt = smcTable::getColumnCount(mTableHeader);
    UInt  i;
    const smiColumn * sColumn;

    IDE_TEST(writeToBackupFile((UChar*)&sColumnCnt, ID_SIZEOF(UInt))
             != IDE_SUCCESS);

    for( i = 0; i < sColumnCnt; i++ )
    {
        sColumn = smcTable::getColumn( mTableHeader, i );

        IDE_TEST(writeToBackupFile((UChar*)sColumn, ID_SIZEOF(smiColumn))
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Restore시 기록된 Column List를 skip한다.
 ***********************************************************************/
IDE_RC smiVolTableBackup::skipColumnInfo()
{
    UInt sColumnCnt;

    IDE_TEST(mFile.read(mOffset, &sColumnCnt, ID_SIZEOF(UInt))
             != IDE_SUCCESS);
    mOffset += ID_SIZEOF(UInt);

    mOffset += ( sColumnCnt * ID_SIZEOF(smiColumn));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Filename에 해당하는 파일을 Dump한다.
 *
 * aFilename : TableBackupFile을 Dump한다.
 *
 * cf)본 함수는 사용되고 있지 않음.
 ***********************************************************************/
IDE_RC smiVolTableBackup::dump(SChar *aFilename)
{
    ULong       sOffset     = 0;
    UInt        sColumnCnt;
    smiColumn * sColumnList;
    ULong       sFileSize = 0;
    ULong       sFenceSize;
    SInt        sColumnType;
    UInt        i;
    UInt        sSize;
    ULong       sRowCnt;

    smcTableBackupFile sFile;   //Backup File

    IDE_ASSERT(sFile.initialize(aFilename)
               == IDE_SUCCESS);

    IDE_ASSERT(sFile.open(ID_FALSE/*Read*/) == IDE_SUCCESS);

    IDE_ASSERT(sFile.getFileSize(&sFileSize) == IDE_SUCCESS);

    sOffset += ID_SIZEOF(smcBackupFileHeader);

    sFenceSize = sFileSize - ID_SIZEOF(smcBackupFileTail);

    IDE_ASSERT(sFile.read( sOffset, &sColumnCnt, ID_SIZEOF(UInt))
               == IDE_SUCCESS);
    sOffset += ID_SIZEOF(UInt);

    IDE_ASSERT(iduMemMgr::malloc(
                   IDU_MEM_SM_SMI,
                   (ULong)ID_SIZEOF(smiColumn) * sColumnCnt,
                   (void **)&sColumnList,
                   IDU_MEM_FORCE)
               == IDE_SUCCESS);

    IDE_ASSERT(sFile.read( sOffset, sColumnList, (size_t)ID_SIZEOF(smiColumn) * sColumnCnt)
               == IDE_SUCCESS);

    sOffset += (sColumnCnt * ID_SIZEOF(smiColumn));
    sRowCnt = 0;

    while( sOffset < sFenceSize)
    {
        idlOS::printf(" #%"ID_UINT32_FMT":", sRowCnt);
        for(i = 0; i < sColumnCnt; i++)
        {
            sColumnType = sColumnList[i].flag & SMI_COLUMN_TYPE_MASK;
            switch(sColumnType)
            {
                case SMI_COLUMN_TYPE_VARIABLE:
                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    IDE_ASSERT(sFile.read( sOffset, &sSize, ID_SIZEOF(UInt))
                               == IDE_SUCCESS);
                    sOffset += ID_SIZEOF(UInt);

                    idlOS::printf(" VAR:%"ID_UINT32_FMT"", sSize);
                    sOffset += sSize;
                    break;

                case SMI_COLUMN_TYPE_FIXED:
                    idlOS::printf(" Fixed:%"ID_UINT32_FMT"", sColumnList[i].size);
                    sOffset += sColumnList[i].size;
                    break;

                default:
                    break;
            }
        }

        /* PROJ-2174 Supporting LOB in the volatile tablespace */
        for( i = 0 ; i < sColumnCnt ; i++ )
        {
            sColumnType = sColumnList[i].flag & SMI_COLUMN_TYPE_MASK;
            switch(sColumnType)
            {
                case SMI_COLUMN_TYPE_LOB:
                    IDE_ASSERT( sFile.read( sOffset, &sSize, ID_SIZEOF(UInt))
                                == IDE_SUCCESS);
                    sOffset += ID_SIZEOF(UInt);

                    idlOS::printf(" LOB:%"ID_UINT32_FMT"", sSize);
                    sOffset += sSize;

                    break;

                default:
                    break;
            }
        }

        sRowCnt++;

        idlOS::printf("\n");
    }

    IDE_ASSERT(iduMemMgr::free(sColumnList) == IDE_SUCCESS);

    idlOS::fflush(NULL);

    IDE_ASSERT(sFile.close()    == IDE_SUCCESS);
    IDE_ASSERT(sFile.destroy()  == IDE_SUCCESS);

    return IDE_SUCCESS;
}


// 파일의 헤더를 읽고, 알티베이스 백업파일이 맞는지를 검사한다.
IDE_RC smiVolTableBackup::isBackupFile(SChar *aFilename, idBool *aCheck)
{
    smmMemBase*            sMemBase;
    smcTableBackupFile     sFile;   //Backup File
    smcBackupFileHeader    sFileHeader;
    UInt                   sState = 0;
    idBool                 sCheck = ID_TRUE;

    sMemBase = smmDatabase::getDicMemBase();


    IDE_TEST(sFile.initialize(aFilename)
             != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(sFile.open(ID_FALSE/*Read*/) != IDE_SUCCESS);
    sState = 2;

    IDE_TEST(sFile.read(0, (void*)&sFileHeader,
                        ID_SIZEOF(smcBackupFileHeader))
             != IDE_SUCCESS);


    if(0 != idlOS::strncmp(sFileHeader.mType,
                           SM_VOLTABLE_BACKUP_STR,
                           SM_TABLE_BACKUP_TYPE_SIZE))
    {
        sCheck = ID_FALSE;
    }
    else if(0 != idlOS::memcmp(sFileHeader.mProductSignature,
                          sMemBase->mProductSignature,
                          IDU_SYSTEM_INFO_LENGTH))
    {
        sCheck = ID_FALSE;
    }
    else if(0 != idlOS::memcmp(sFileHeader.mDbname,
                          sMemBase->mDBname,
                          SM_MAX_DB_NAME))
    {
        sCheck = ID_FALSE;
    }
    else if(sFileHeader.mVersionID != sMemBase->mVersionID)
    {
        sCheck = ID_FALSE;
    }
    else if(sFileHeader.mCompileBit != sMemBase->mCompileBit)
    {
        sCheck = ID_FALSE;
    }
    else if(sFileHeader.mBigEndian != sMemBase->mBigEndian)
    {
        sCheck = ID_FALSE;
    }

    *aCheck = sCheck;

    sState = 1;
    IDE_TEST(sFile.close() != IDE_SUCCESS);
    sState = 0;
    IDE_TEST(sFile.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aCheck = ID_FALSE;

    switch(sState)
    {
        case 2:
            sFile.close();
        case 1:
            sFile.destroy();
        default:
            break;
    }

    return IDE_FAILURE;

}
