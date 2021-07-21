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
 * $Id: smiMain.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <iduFixedTable.h>
#include <iduMemPoolMgr.h>
#include <smErrorCode.h>
#include <smm.h>
#include <svm.h>
#include <sdd.h>
#include <sdb.h>
#include <smr.h>
#include <sdp.h>
#include <smc.h>
#include <sdc.h>
#include <smn.h>
#include <sml.h>
#include <sm2x.h>
#include <sma.h>
#include <smi.h>
#include <smu.h>
#include <sct.h>
#include <scp.h>
#include <sds.h>
#include <smx.h>
#ifdef ALTIBASE_ENABLE_SMARTSSD
#include <sdm_mgnt_public.h>
#endif

/* SM에서 사용할 콜백함수들 */
smiGlobalCallBackList gSmiGlobalCallBackList;

/* The NULL GRID */
scGRID gScNullGRID = { SC_NULL_SPACEID, SC_NULL_OFFSET, SC_NULL_PID };

static IDE_RC smiCreateMemoryTableSpaces( SChar         * aDBName,
                                          UInt            aCreatePageCount,
                                          SChar         * aDBCharSet,
                                          SChar         * aNationalCharSet);

static IDE_RC smiCreateDiskTableSpaces();

static ULong gDBFileMaxPageCntOfDRDB;

/*
 * TASK-6198 Samsung Smart SSD GC control
 */
#ifdef ALTIBASE_ENABLE_SMARTSSD
sdm_handle_t * gLogSDMHandle;
#endif

/********************************************************************
 * Description : MEM_MAX_DB_SIZE가 EXPAND_CHUNK_PAGE_COUNT보다 큰지
 *               검사한다.
 ********************************************************************/
IDE_RC smiCheckMemMaxDBSize()
{
    IDE_TEST( smuProperty::getExpandChunkPageCount() * SM_PAGE_SIZE >
              smuProperty::getMaxDBSize() )

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( smERR_FATAL_INVALID_MEM_MAX_DB_SIZE,
                              smuProperty::getMaxDBSize(),
                              smuProperty::getExpandChunkPageCount() ) );

    return IDE_FAILURE;
}

/* 사용자가 지정한 데이터베이스 크기를 토대로
 * 실제로 생성할 데이터베이스 크기를 계산한다.
 *
 * 하나의 데이터베이스 파일이 여러개의 Expand Chunk로 구성되기 때문에,
 * 사용자가 지정한 데이터베이스 크기와 정확히 일치하지 않는
 * 크기로 데이터베이스가 생성될 수 있기 때문에 이 함수가 필요하다.
 *
 * aUserDbCreatePageCount [IN] 사용자가 지정한 초기 데이터 베이스의 Page수
 * aDbCreatePageCount     [OUT] 시스템이 계산한 초기 데이터 베이스의 Page수
 */
IDE_RC smiCalculateDBSize( UInt   aUserDbCreatePageCount,
                           UInt * aDbCreatePageCount )
{
    UInt sChunkPageCount = smuProperty::getExpandChunkPageCount() ;

#ifdef DEBUG    
    ULong  sCalculateDbPageCount;
#endif

    IDE_ASSERT( sChunkPageCount > 0 );

#ifdef DEBUG    
    sCalculateDbPageCount = smmManager::calculateDbPageCount( 
                               aUserDbCreatePageCount * (ULong)SM_PAGE_SIZE,
                               sChunkPageCount);
    IDE_DASSERT( (ULong)SC_MAX_PAGE_COUNT >= sCalculateDbPageCount );  
#endif

    // BUG-15288
    // create시 page count는 chunk page count로 align하지 않고
    // smmManager를 통해서 구한다.
    *aDbCreatePageCount = smmManager::calculateDbPageCount(
                              aUserDbCreatePageCount * (ULong)SM_PAGE_SIZE,
                              sChunkPageCount );

    return IDE_SUCCESS ;
}


/* 하나의 데이터베이스 파일이 지니는 Page의 수를 리턴한다
 *
 * aDBFilePageCount [IN] 하나의 데이터베이스 파일이 지니는 Page의 수
 */
IDE_RC smiGetDBFilePageCount( scSpaceID aSpaceID, UInt * aDBFilePageCount)
{
    smmTBSNode * sTBSNode;
    UInt         sDBFilePageCount;
    UInt         sChunkPageCount ;

    IDE_DASSERT( aDBFilePageCount != NULL );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(aSpaceID,
                                                       (void**)&sTBSNode)
              != IDE_SUCCESS );
    IDE_ASSERT( sTBSNode != NULL);
    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace(aSpaceID) == ID_TRUE );

    IDE_TEST( smmManager::readMemBaseInfo( sTBSNode,
                                           & sDBFilePageCount,
                                           & sChunkPageCount )
              != IDE_SUCCESS );

    * aDBFilePageCount = sDBFilePageCount ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* 데이터베이스가 생성할 수 있는 최대 Page수를 계산
 *
 */
UInt smiGetMaxDBPageCount()
{
    UInt   sChunkPageCount;
    ULong  sMaxDbSize;

#ifdef DEBUG    
    ULong  sCalculateDbPageCount;
#endif

    sMaxDbSize      = smuProperty::getMaxDBSize();
    sChunkPageCount = smuProperty::getExpandChunkPageCount();

    IDE_ASSERT( sChunkPageCount > 0 );
#ifdef DEBUG    
    sCalculateDbPageCount = smmManager::calculateDbPageCount( sMaxDbSize,
                                                              sChunkPageCount);
    IDE_DASSERT( (ULong)SC_MAX_PAGE_COUNT >= sCalculateDbPageCount );  
#endif

    return smmManager::calculateDbPageCount( sMaxDbSize,
                                             sChunkPageCount);
}

/* 데이터베이스가 생성할 수 있는 최소 Page 수를 계산
 * 최소한 expand_chunk_page_count보단 커야 한다.
 */
UInt smiGetMinDBPageCount()
{
    return smuProperty::getExpandChunkPageCount();
}

/*
 * 데이터베이스를 생성한다.
 * createdb 에서 부른다.
 *
 * aDBName          [IN] 데이터베이스 이름
 * aCreatePageCount [IN] 생성할 데이터베이스가 가질 Page의 수
 *                       Membase가 기록되는 Meta Page(0번 Page)의 수는
 *                       포함되지 않는다.
 * aDBCharSet       [IN] 데이터베이스 캐릭터 셋
 * aNationalCharSet [IN] 내셔널 캐릭터 셋
 * aArchiveLog      [IN] 아카이브 로그 모드
 */
IDE_RC smiCreateDB( SChar         * aDBName,
                    UInt            aCreatePageCount,
                    SChar         * aDBCharSet,
                    SChar         * aNationalCharSet,
                    smiArchiveMode  aArchiveLog )
{
    /* -------------------------
     * [2] create Memory Mgr & init
     * ------------------------*/
    IDE_CALLBACK_SEND_SYM("\tCreating MMDB FILES     ");

    IDE_TEST( smiCreateMemoryTableSpaces( aDBName,
                                          aCreatePageCount,
                                          aDBCharSet,
                                          aNationalCharSet )
              != IDE_SUCCESS );

    IDE_CALLBACK_SEND_MSG("[SUCCESS]\n");

    /* -------------------------
     * [3] create Catalog Table & Index
     * ------------------------*/
    IDE_CALLBACK_SEND_SYM("\tCreating Catalog Tables ");

    IDE_TEST( smcCatalogTable::createCatalogTable()
              != IDE_SUCCESS );

    IDE_CALLBACK_SEND_MSG("[SUCCESS]\n");

    /* FOR A4 : DRDB를 위한 DB Create 작업 수행 */

    IDE_CALLBACK_SEND_SYM("\tCreating DRDB FILES     ");

    IDE_TEST( smiCreateDiskTableSpaces() != IDE_SUCCESS );

    IDE_CALLBACK_SEND_MSG("[SUCCESS]\n");

    /* create database dbname ..[archivelog|noarchivelog] */
    IDE_TEST( smiBackup::alterArchiveMode( aArchiveLog,
                                           ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   Createdb시에 Memory Tablespace들을 생성한다.

   aDBName          [IN] 데이터베이스 이름
   aCreatePageCount [IN] 생성할 데이터베이스가 가질 Page의 수
                         Membase가 기록되는 Meta Page(0번 Page)의 수는
                         포함되지 않는다.
 */
static IDE_RC smiCreateMemoryTableSpaces(
                       SChar         * aDBName,
                       UInt        aCreatePageCount,
                       SChar         * aDBCharSet,
                       SChar         * aNationalCharSet)
{
    UInt               sState = 0;
    smxTrans *         sTrans = NULL;
    SChar              sOutputMsg[256];
    // 시스템이 가질 수 있는 최대 Page 갯수
    UInt               sHighLimitPageCnt;
    UInt               sTotalPageCount;
    UInt               sSysDicPageCount;
    UInt               sSysDataPageCount;

    // SYSTEM DICTIONARY TABLESPACE의 초기 크기
    // -> Tablespace의 최소 크기인 EXPAND_CHUNK_PAGE_COUNT로 설정
    sSysDicPageCount  = smuProperty::getExpandChunkPageCount();

    // SYSTEM DATA TABLESPACE의 초기 크기
    // -> 사용자가 create database구문에 지정한 초기크기인,
    //    aCreatePageCount로 설정
    sSysDataPageCount = aCreatePageCount;

    // 에러체크 실시
    {
        // MEM_MAX_DB_SIZE에 걸리지 않는지 검사
        sHighLimitPageCnt = smiGetMaxDBPageCount();

        sTotalPageCount = sSysDicPageCount + sSysDataPageCount;

        // 사용자가 초기 크기로 지정한 Page가 0개이면 에러
        IDE_TEST_RAISE( aCreatePageCount <= 0, page_range_error);

        // 사용자가 지정한 Page수 + SYSTEM DICTIONARY Tablespace크기가
        // MEM_MAX_DB_SIZE 를 넘어서면 에러
        IDE_TEST_RAISE( sTotalPageCount > sHighLimitPageCnt,
                        page_range_error );

        // BUG-29607 Create DB에서 Memory Tablespace를 생성하기 전
        //           동일 이름의 File이 이미 존재하는지 확인한다.
        //        Create Tablespace에서도 검사 하지만
        //        반환하는 오류의 내용이 다르다.
        IDE_TEST_RAISE( smmDatabaseFile::chkExistDBFileByProp(
                            SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DIC ) != IDE_SUCCESS,
            error_already_exist_datafile );

        IDE_TEST_RAISE( smmDatabaseFile::chkExistDBFileByProp(
                            SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DATA ) != IDE_SUCCESS,
            error_already_exist_datafile );
    }


    IDE_TEST( smxTransMgr::alloc( &sTrans ) != IDE_SUCCESS );
    sState = 1;


    IDE_ASSERT( sTrans->begin( NULL,
                               ( SMI_TRANSACTION_REPL_DEFAULT |
                                 SMI_COMMIT_WRITE_NOWAIT ),
                               SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );
    sState = 2;

    IDE_TEST( smmTBSCreate::createTBS(sTrans,
                                       aDBName,
                                       (SChar*)SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DIC,
                                       SMI_TABLESPACE_ATTRFLAG_SYSTEM_MEMORY_DIC,
                                       SMI_MEMORY_SYSTEM_DICTIONARY,
                                       NULL,   /* Use Default Checkpoint Path*/
                                       0,      /* Use Default Split Size */
                                       // Init Size
                                       sSysDicPageCount * SM_PAGE_SIZE,
                                       ID_TRUE, /* Auto Extend On */
                                       ID_ULONG_MAX,/* Use Default Next Size */
                                       ID_ULONG_MAX,/* Use Default Max Size */
                                       ID_TRUE, /* Online */
                                       aDBCharSet, /* PROJ-1579 NCHAR */
                                       aNationalCharSet, /* PROJ-1579 NCHAR */
                                       NULL    /* No Need to get TBSID */)
              != IDE_SUCCESS );

    IDE_TEST( sTrans->commit() != IDE_SUCCESS );
    sState = 1;


    IDE_ASSERT( sTrans->begin( NULL,
                               ( SMI_TRANSACTION_REPL_DEFAULT |
                                 SMI_COMMIT_WRITE_NOWAIT ),
                               SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );
    sState = 2;
    IDE_TEST( smmTBSCreate::createTBS(
                            sTrans,
                            aDBName,
                            (SChar*)SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DATA,
                            SMI_TABLESPACE_ATTRFLAG_SYSTEM_MEMORY_DATA,
                            SMI_MEMORY_SYSTEM_DATA,
                            NULL,    /* Use Default Checkpoint Path*/
                            0,       /* Use Default Split Size */
                            // Init Size
                            sSysDataPageCount * SM_PAGE_SIZE,
                            ID_TRUE, /* Auto Extend On */
                            ID_ULONG_MAX, /* Use Default Next Size */
                            ID_ULONG_MAX, /* Use Default Max Size */
                            ID_TRUE, /* Online */
                            aDBCharSet, /* PROJ-1579 NCHAR */
                            aNationalCharSet, /* PROJ-1579 NCHAR */
                            NULL     /* No Need to get TBSID */  )
              != IDE_SUCCESS );

    IDE_TEST( sTrans->commit() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smxTransMgr::freeTrans( sTrans ) != IDE_SUCCESS );
    sState = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION(page_range_error);
    {
        idlOS::snprintf(sOutputMsg, ID_SIZEOF(sOutputMsg), "ERROR: \n"
                        "Specify Valid Page Range (1 ~ %"ID_UINT64_FMT") \n",
                        (ULong) sHighLimitPageCnt);
        ideLog::log(IDE_SERVER_0,"%s\n",sOutputMsg);

        IDE_SET(ideSetErrorCode(smERR_ABORT_PAGE_RANGE_ERROR, sHighLimitPageCnt));
    }
    IDE_EXCEPTION( error_already_exist_datafile );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AlreadyExistDBFiles ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sState )
    {
        case 2:
            sTrans->abort( ID_FALSE, /* aIsLegacyTrans */
                           NULL      /* aLegacyTrans */ );
        case 1:
            smxTransMgr::freeTrans( sTrans );
            break;

        default :
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


static IDE_RC smiCreateDiskTableSpaces( )
{
    smiTableSpaceAttr   sTbsAttr;
    smiDataFileAttr     sDFAttr;
    smiDataFileAttr     sSysDFAttr;
    smiDataFileAttr *   sDFAttrPtr;
    SChar               sDir[SMI_MAX_DATAFILE_NAME_LEN];
    SChar *             sDbDir;
    smxTrans *          sTx = NULL;
    ULong               sExtentSize;
    ULong               sInitSize;
    ULong               sMaxSize;
    ULong               sNextSize;
    UInt                sEntryCnt;
    UInt                sAdjustEntryCnt;

    sDbDir = (SChar *)smuProperty::getDefaultDiskDBDir();
    idlOS::strcpy(sDir, sDbDir);

    /* 1. system tablespace
       2. undo  tablespace
       3. temp  tablespace
       순으로 생성한다.

       // system tablespace를 생성.
      sdpTableSpace::createSystemTBS()를 호출
       //  undo tablespace 생성.
      sdpTableSpace::createUndoTBS();
       //  temp tablespace 생성.
       //->temp  테이블 스페이스 이름,
       sdpTableSpace::createTempTBS(.....)
     */

    // ===== create system Table Space =====
    // BUG-27911
    // smuProperty 의 함수를 직접 호출하지 않고
    // smiTableSpace 의 인터페이스 함수를 호출하는 방식으로 변경합니다.
    sExtentSize = smiTableSpace::getSysDataTBSExtentSize();
    sInitSize   = smiTableSpace::getSysDataFileInitSize();
    sMaxSize    = smiTableSpace::getSysDataFileMaxSize();
    sNextSize   = smiTableSpace::getSysDataFileNextSize();

    if( sInitSize > sMaxSize )
    {
        sInitSize = sMaxSize;
    }

    idlOS::memset(&sTbsAttr, 0x00, ID_SIZEOF(smiTableSpaceAttr));
    idlOS::memset(&sSysDFAttr, 0x00, ID_SIZEOF(smiDataFileAttr));

    // PRJ-1548 User Memory Tablespace
    // DISK SYSTEM TBS의 TBS Node Attribute 설정

    sTbsAttr.mAttrType = SMI_TBS_ATTR;
    sTbsAttr.mAttrFlag = SMI_TABLESPACE_ATTRFLAG_SYSTEM_DISK_DATA;
    sTbsAttr.mID = 0;
    idlOS::snprintf(sTbsAttr.mName, SMI_MAX_TABLESPACE_NAME_LEN,
                    "%s", SMI_TABLESPACE_NAME_SYSTEM_DISK_DATA);
    sTbsAttr.mNameLength   = idlOS::strlen(sTbsAttr.mName);
    sTbsAttr.mType         = SMI_DISK_SYSTEM_DATA;
    sTbsAttr.mTBSStateOnLA = SMI_TBS_ONLINE;

    idlOS::snprintf(sSysDFAttr.mName, SMI_MAX_DATAFILE_NAME_LEN,
                    "%s%csystem001.dbf",
                    sDir,
                    IDL_FILE_SEPARATOR);

    // BUG-29607 Create DB에서 Disk Tablespace를 생성하기 전
    //           동일 이름의 File이 이미 존재하는지 확인한다.
    //        Create Tablespace에서도 검사 하지만
    //        반환하는 오류의 내용이 다르다.
    IDE_TEST_RAISE( idf::access( sSysDFAttr.mName, F_OK) == 0,
                    error_already_exist_datafile );

    // PRJ-1548 User Memory Tablespace
    // DISK SYSTEM TBS의 DBF Node Attribute 설정
    sSysDFAttr.mAttrType     = SMI_DBF_ATTR;
    sSysDFAttr.mNameLength   = idlOS::strlen(sSysDFAttr.mName);
    sSysDFAttr.mIsAutoExtend = ID_TRUE;
    sSysDFAttr.mState        = SMI_FILE_ONLINE;

    // BUG-27911
    // alignByPageSize() 는 size 에 대해 잘못된 계산을 하고 있었습니다.
    // smiTableSpace 의 인터페이스 함수를 통해 얻어온값은
    // valide 하기 때문에 이 값을 바로 사용합니다.
    sSysDFAttr.mMaxSize      = sMaxSize  / SD_PAGE_SIZE;
    sSysDFAttr.mNextSize     = sNextSize / SD_PAGE_SIZE;
    sSysDFAttr.mCurrSize     = sInitSize / SD_PAGE_SIZE;
    sSysDFAttr.mInitSize     = sInitSize / SD_PAGE_SIZE;
    // BUG-29607 동일 파일명이 있으면 재사용하지 말고 오류 반환하도록 수정
    sSysDFAttr.mCreateMode   = SMI_DATAFILE_CREATE;

    IDE_TEST( smxTransMgr::alloc( &sTx ) != IDE_SUCCESS );

    IDE_ASSERT( sTx->begin( NULL,
                            ( SMI_TRANSACTION_REPL_NONE |
                              SMI_COMMIT_WRITE_NOWAIT ),
                            SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );

    /* PROJ-1671 Bitmap-base Tablespace And Segment Space Management
     * Create Database 과정에서는 기본 시스템 프로퍼티값을 판독한다 */
    sTbsAttr.mDiskAttr.mSegMgmtType  =
             (smiSegMgmtType)smuProperty::getDefaultSegMgmtType();
    sTbsAttr.mDiskAttr.mExtMgmtType  =  SMI_EXTENT_MGMT_BITMAP_TYPE;
    sTbsAttr.mDiskAttr.mExtPageCount = (UInt)(sExtentSize / SD_PAGE_SIZE);

    sDFAttrPtr = &sSysDFAttr;

    IDE_TEST( sdpTableSpace::createTBS( NULL,// BUGBUG
                                        &sTbsAttr,
                                        &sDFAttrPtr,
                                        1,
                                        sTx)
              != IDE_SUCCESS );

    IDE_TEST( sTx->commit() != IDE_SUCCESS );

    // ===== create undo Table Space =====
    // BUG-27911
    // smuProperty 의 함수를 직접 호출하지 않고
    // smiTableSpace 의 인터페이스 함수를 호출하는 방식으로 변경합니다.
    sExtentSize = smiTableSpace::getSysUndoTBSExtentSize();
    sInitSize   = smiTableSpace::getSysUndoFileInitSize();
    sMaxSize    = smiTableSpace::getSysUndoFileMaxSize();
    sNextSize   = smiTableSpace::getSysUndoFileNextSize();

    if( sInitSize > sMaxSize )
    {
        sInitSize = sMaxSize;
    }

    idlOS::memset(&sTbsAttr, 0x00, ID_SIZEOF(smiTableSpaceAttr));
    idlOS::memset(&sDFAttr, 0x00, ID_SIZEOF(smiDataFileAttr));

    sTbsAttr.mID = 0;

    // PRJ-1548 User Memory Tablespace
    // UNDO TBS의 TBS Node Attribute 설정
    sTbsAttr.mAttrType = SMI_TBS_ATTR;
    sTbsAttr.mAttrFlag = SMI_TABLESPACE_ATTRFLAG_SYSTEM_DISK_UNDO;

    idlOS::snprintf(sTbsAttr.mName,
                    SMI_MAX_TABLESPACE_NAME_LEN,
                    SMI_TABLESPACE_NAME_SYSTEM_DISK_UNDO);

    sTbsAttr.mNameLength =
        idlOS::strlen(sTbsAttr.mName);
    sTbsAttr.mType  = SMI_DISK_SYSTEM_UNDO;
    sTbsAttr.mTBSStateOnLA = SMI_TBS_ONLINE;

    // PRJ-1548 User Memory Tablespace
    // UNDO TBS의 DBF Node Attribute 설정

    sDFAttr.mAttrType  = SMI_DBF_ATTR;

    idlOS::snprintf(sDFAttr.mName, SMI_MAX_DATAFILE_NAME_LEN,
                   "%s%cundo001.dbf",
                   sDir,
                   IDL_FILE_SEPARATOR);

    // BUG-29607 Create DB에서 Disk Tablespace를 생성하기 전
    //           동일 이름의 File이 이미 존재하는지 확인한다.
    //        Create Tablespace에서도 검사 하지만
    //        반환하는 오류의 내용이 다르다.
    IDE_TEST_RAISE( idf::access( sDFAttr.mName, F_OK) == 0,
                    error_already_exist_datafile );

    sDFAttr.mNameLength   = idlOS::strlen(sDFAttr.mName);
    sDFAttr.mIsAutoExtend = ID_TRUE;
    sDFAttr.mState        = SMI_FILE_ONLINE;

    // BUG-27911
    // alignByPageSize() 는 size 에 대해 잘못된 계산을 하고 있었습니다.
    // smiTableSpace 의 인터페이스 함수를 통해 얻어온값은
    // valide 하기 때문에 이 값을 바로 사용합니다.
    sSysDFAttr.mMaxSize   = sMaxSize  / SD_PAGE_SIZE;
    sDFAttr.mMaxSize      = sMaxSize  / SD_PAGE_SIZE;
    sDFAttr.mNextSize     = sNextSize / SD_PAGE_SIZE;
    sDFAttr.mCurrSize     = sInitSize / SD_PAGE_SIZE;
    sDFAttr.mInitSize     = sInitSize / SD_PAGE_SIZE;
    // BUG-29607 동일 파일명이 있으면 재사용하지 말고 오류 반환하도록 수정
    sDFAttr.mCreateMode   = SMI_DATAFILE_CREATE;

    sDFAttrPtr = &sDFAttr;

    IDE_ASSERT( sTx->begin( NULL,
                            ( SMI_TRANSACTION_REPL_NONE |
                              SMI_COMMIT_WRITE_NOWAIT ),
                            SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );

    sTbsAttr.mDiskAttr.mSegMgmtType  = SMI_SEGMENT_MGMT_CIRCULARLIST_TYPE;
    sTbsAttr.mDiskAttr.mExtMgmtType  = SMI_EXTENT_MGMT_BITMAP_TYPE;
    sTbsAttr.mDiskAttr.mExtPageCount = (UInt)(sExtentSize / SD_PAGE_SIZE);
    IDE_TEST( sdpTableSpace::createTBS( NULL,// BUGBUG
                                        &sTbsAttr,
                                        &sDFAttrPtr,
                                        1,
                                        sTx)
              != IDE_SUCCESS );

    /* To Fix BUG-24090 createdb시 undo001.dbf 크기가 이상합니다.
     * 트랜잭션 Commit Pending으로 Add DataFile 연산이 수행되어
     * SpaceCache의 FreenessOfGGs를 setBit하게되어 있는데
     * Segment 생성과 함께 하나의 트랜잭션으로 처리되어 Segment
     * 생성시에는 유효하지 않은 Freeness로 인해 파일 확장이 발생하여
     * 크기가 증가하였다. */
    IDE_TEST( sTx->commit() != IDE_SUCCESS );

    IDE_ASSERT( sTx->begin( NULL,
                            ( SMI_TRANSACTION_REPL_NONE |
                              SMI_COMMIT_WRITE_NOWAIT ),
                            SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );

    /***********************************************************************
     * PROJ-1704 DISK MVCC 리뉴얼
     * 트랜잭션 세그먼트는 TSS Segment와 Undo Segment를 묶어서 일컫는
     * 용어이다. Create Database 과정에서 Undo Tablespace를 생성한 후에
     * 사용자 프로퍼티 TRANSACTION SEGMENT에 명시된 개수를 보정하여
     * TSS Segment 와 Undo Segment를 생성하고, 이를 관리하는 Transaction
     * Segment Manager를 초기화 한다.
     ***********************************************************************/
    sEntryCnt = smuProperty::getTXSEGEntryCnt();

    IDE_TEST( sdcTXSegMgr::adjustEntryCount( sEntryCnt,
                                             &sAdjustEntryCnt )
              != IDE_SUCCESS );

    IDE_TEST( sdcTXSegMgr::createSegs( NULL /* idvSQL */, sTx )
              != IDE_SUCCESS );

    IDE_TEST( sdcTXSegMgr::initialize( ID_TRUE /* aIsAttachSegment*/ )
              != IDE_SUCCESS );

    IDE_TEST( sTx->commit() != IDE_SUCCESS );

    // ===== create temp Table Space =====
    // BUG-27911
    // smuProperty 의 함수를 직접 호출하지 않고
    // smiTableSpace 의 인터페이스 함수를 호출하는 방식으로 변경합니다.    
    sExtentSize = smiTableSpace::getSysTempTBSExtentSize();
    sInitSize   = smiTableSpace::getSysTempFileInitSize();
    sMaxSize    = smiTableSpace::getSysTempFileMaxSize();
    sNextSize   = smiTableSpace::getSysTempFileNextSize();

    if( sInitSize > sMaxSize )
    {
        sInitSize = sMaxSize;
    }

    idlOS::memset(&sTbsAttr, 0x00, ID_SIZEOF(smiTableSpaceAttr));
    idlOS::memset(&sDFAttr, 0x00, ID_SIZEOF(smiDataFileAttr));

    sTbsAttr.mID = 0;

    // PRJ-1548 User Memory Tablespace
    // DISK TEMP TBS의 TBS Node Attribute 설정
    sTbsAttr.mAttrType = SMI_TBS_ATTR;
    sTbsAttr.mAttrFlag = SMI_TABLESPACE_ATTRFLAG_SYSTEM_DISK_TEMP;

    idlOS::snprintf(sTbsAttr.mName,
                    SMI_MAX_TABLESPACE_NAME_LEN,
                    SMI_TABLESPACE_NAME_SYSTEM_DISK_TEMP);

    sTbsAttr.mNameLength = idlOS::strlen(sTbsAttr.mName);

    sTbsAttr.mType  = SMI_DISK_SYSTEM_TEMP;
    sTbsAttr.mTBSStateOnLA = SMI_TBS_ONLINE;

    // PRJ-1548 User Memory Tablespace
    // DISK TEMP TBS의 DBF Node Attribute 설정
    sDFAttr.mAttrType  = SMI_DBF_ATTR;

    idlOS::snprintf(sDFAttr.mName,
                    SMI_MAX_DATAFILE_NAME_LEN,
                    "%s%ctemp001.dbf",
                    sDir,
                    IDL_FILE_SEPARATOR);

    // BUG-29607 Create DB에서 Disk Tablespace를 생성하기 전
    //           동일 이름의 File이 이미 존재하는지 확인한다.
    //        Create Tablespace에서도 검사 하지만
    //        반환하는 오류의 내용이 다르다.
    IDE_TEST_RAISE( idf::access( sDFAttr.mName, F_OK) == 0,
                    error_already_exist_datafile );

    sDFAttr.mNameLength   = idlOS::strlen(sDFAttr.mName);
    sDFAttr.mIsAutoExtend = ID_TRUE;
    sDFAttr.mState        = SMI_FILE_ONLINE;

    // BUG-27911
    // alignByPageSize() 는 size 에 대해 잘못된 계산을 하고 있었습니다.
    // smiTableSpace 의 인터페이스 함수를 통해 얻어온값은
    // valide 하기 때문에 이 값을 바로 사용합니다.
    sDFAttr.mMaxSize      = sMaxSize  / SD_PAGE_SIZE;
    sDFAttr.mNextSize     = sNextSize / SD_PAGE_SIZE;
    sDFAttr.mCurrSize     = sInitSize / SD_PAGE_SIZE;
    sDFAttr.mInitSize     = sInitSize / SD_PAGE_SIZE;
    // BUG-29607 동일 파일명이 있으면 재사용하지 말고 오류 반환하도록 수정
    sDFAttr.mCreateMode   = SMI_DATAFILE_CREATE;

    sDFAttrPtr = &sDFAttr;

    IDE_ASSERT( sTx->begin( NULL,
                            ( SMI_TRANSACTION_REPL_NONE |
                              SMI_COMMIT_WRITE_NOWAIT ),
                            SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );

    sTbsAttr.mDiskAttr.mSegMgmtType  = SMI_SEGMENT_MGMT_FREELIST_TYPE;
    sTbsAttr.mDiskAttr.mExtMgmtType  = SMI_EXTENT_MGMT_BITMAP_TYPE;
    sTbsAttr.mDiskAttr.mExtPageCount = (UInt)(sExtentSize / SD_PAGE_SIZE);

    IDE_TEST( sdpTableSpace::createTBS( NULL,// BUGBUG
                                        &sTbsAttr,
                                        &sDFAttrPtr,
                                        1,
                                        sTx )
              != IDE_SUCCESS );

    IDE_TEST( sTx->commit() != IDE_SUCCESS );

    IDE_TEST( smxTransMgr::freeTrans( sTx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_exist_datafile );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AlreadyExistDBFiles ));
    }
    IDE_EXCEPTION_END;
    if( sTx != NULL )
    {
        sTx->commit();
        smxTransMgr::freeTrans( sTx );
    }

    return IDE_FAILURE;
}

// ======================= for supporting Multi Phase Startup =================
// BUGBUG : 구현 예정
static smiStartupPhase gStartupPhase = SMI_STARTUP_INIT;

/*
 * 구현 Guide:
 * QP/MM으로 부터 임의의 aPhase가 넘어올 수 있음.
 * 그것에 대한 에러 코드를 올려야 함(Not Killed!!)
 *
 * Phase가 증가될 때 현재의 Phase로 부터 명시된 Phase까지 단계를 밟아
 * 초기화를 수행해야 함.
 */

/*
 * 현재의 Phase로 부터 INIT 모드까지 단계적으로 Shutdown을 수행해야 함.
 */


/**************************************************/
/* TSM에서 CreateDB 시에 사용되는 초기화 Callback */
/**************************************************/
IDE_RC smiCreateDBCoreInit(smiGlobalCallBackList *   /*aCallBack*/)
{
    struct rlimit sLimit;

    /* -------------------------
     * [0] Get Properties
     * ------------------------*/
    /* BUG-22201: Disk DataFile의 최대크기가 32G를 넘어서는 파일생성이 성공하고
     * 있습니다.
     *
     * 최대크기가 32G를 넘지 않도록 보정함. ( SD_MAX_FPID_COUNT: 1<<22, 2^21 )
     * */
    IDE_TEST(idlOS::getrlimit( RLIMIT_FSIZE, &sLimit) != 0 );
    gDBFileMaxPageCntOfDRDB = sLimit.rlim_cur / SD_PAGE_SIZE;

    gDBFileMaxPageCntOfDRDB = gDBFileMaxPageCntOfDRDB > SD_MAX_FPID_COUNT ? SD_MAX_FPID_COUNT : gDBFileMaxPageCntOfDRDB;

    ideLog::log(IDE_SERVER_0,"\n");

    /* 1.메모리 테이블스페이스 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] TableSpace Manager");
    IDE_TEST( sctTableSpaceMgr::initialize( ) != IDE_SUCCESS );
    
    /* 2.Dirty Page 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Dirty Page Manager");
    IDE_TEST( smmDirtyPageMgr::initializeStatic() != IDE_SUCCESS );

    /* 3.메모리 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Memory Manager");
    IDE_TEST( smmManager::initializeStatic( ) != IDE_SUCCESS );

    /* 4.테이블스페이스 관리 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Memory Tablespace");
    IDE_TEST( smmTBSStartupShutdown::initializeStatic( ) != IDE_SUCCESS );

    /* 5.디스크 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Disk Manager");
    IDE_TEST( sddDiskMgr::initialize( (UInt)gDBFileMaxPageCntOfDRDB )
              != IDE_SUCCESS );

    /* 6.버퍼 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Buffer Manager");
    IDE_TEST( sdbBufferMgr::initialize() != IDE_SUCCESS );

    /* 0번 파일 생성 : 복구 관리자보다 먼저 생성이 되어 있어야 함. */
    IDE_TEST( smrRecoveryMgr::create() != IDE_SUCCESS );

    /* 7.복구 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Recovery Manager");
    IDE_TEST( smrRecoveryMgr::initialize() != IDE_SUCCESS );

    /* 8.로그 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Log File");
    IDE_TEST( smrLogMgr::initialize() != IDE_SUCCESS );

    /* 9.디스크 테이블스페이스 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Tablespace Manager");
    IDE_TEST( sdpTableSpace::initialize() != IDE_SUCCESS );

    /* 10.백업 관리자 */
    IDE_TEST( smrBackupMgr::initialize() != IDE_SUCCESS );

    /* prepare Thread 시작 */ 
    IDE_TEST( smrLogMgr::startupLogPrepareThread() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Buffer Flusher");
    IDE_TEST( sdbFlushMgr::initialize(smuProperty::getBufferFlusherCnt())
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************/
/* MM에서 CreateDB시에 사용되는 초기화 Callback */
/************************************************/
IDE_RC smiCreateDBMetaInit(smiGlobalCallBackList*    /*aCallBack*/)
{
    /* -------------------------
     * [1] CheckPoint & GC관련 데이터 초기화
     * ------------------------*/

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] CheckPoint Manager");
    IDE_TEST( gSmrChkptThread.initialize() != IDE_SUCCESS );

    /* PROJ-2733
       Ager에서 초기화되지 않은 smxMinSCNBuild::mAgingViewSCN에 접근하지 않도록
       Ager 시작전에 smxMinSCNBuild를 초기화한다. */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Initialize Minimum SCN Builder ");
    IDE_TEST( smxTransMgr::initializeMinSCNBuilder() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Memory Garbage Collector");
    IDE_TEST( smaLogicalAger::initializeStatic() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Delete Manager");
    IDE_TEST( smaDeleteThread::initializeStatic() != IDE_SUCCESS );

    /* -------------------------
     * [2] Index Rebuilding
     * ------------------------*/

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Index Rebuilding");
    IDE_TEST( smnManager::rebuildIndexes() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***************************************************/
/* TSM에서 CreateDB시에 사용되는 Shutdown Callback */
/***************************************************/
IDE_RC smiCreateDBCoreShutdown(smiGlobalCallBackList*    /*aCallBack*/)
{
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Flush All DirtyPages And Checkpoint Database");
    IDE_TEST( smrRecoveryMgr::finalize() != IDE_SUCCESS );

    /* 로그 관리자 작업 종료*/
    IDE_TEST( smrLogMgr::shutdown() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Flush Manager");
    IDE_TEST( sdbFlushMgr::destroy() != IDE_SUCCESS );

    /* 10.백업 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Backup Manager");
    IDE_TEST( smrBackupMgr::destroy() != IDE_SUCCESS );

    /* 9.디스크 테이블스페이스 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Tablespace Manager");
    IDE_TEST( sdpTableSpace::destroy() != IDE_SUCCESS );

    /* 8.로그 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Log Manager");
    IDE_TEST( smrLogMgr::destroy() != IDE_SUCCESS );

    /* 7.복구 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Recovery Manager");
    IDE_TEST( smrRecoveryMgr::destroy() != IDE_SUCCESS );

    /* 6.버퍼 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Buffer Manager");
    IDE_TEST( sdbBufferMgr::destroy() != IDE_SUCCESS );

    /* 5.디스크 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Disk Manager");
    IDE_TEST( sddDiskMgr::destroy() != IDE_SUCCESS );

    /* 4.테이블스페이스  */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Memory Tablespace");
    IDE_TEST( smmTBSStartupShutdown::destroyStatic() != IDE_SUCCESS );

    /* 3.메모리 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Memory Manager");
    IDE_TEST( smmManager::destroyStatic() != IDE_SUCCESS );

    /* 2.Dirty Page 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Dirty Page Manager");
    IDE_TEST( smmDirtyPageMgr::destroyStatic() != IDE_SUCCESS );

    /* 1.메모리 테이블스페이스 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] TableSpace Manager");
    IDE_TEST( sctTableSpaceMgr::destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***************************************************/
/* MM에서 CreateDB시에 사용되는 Shutdown Callback  */
/***************************************************/
IDE_RC smiCreateDBMetaShutdown(smiGlobalCallBackList* /*aCallBack*/)
{
    // Start up시와 반대 순서로 진행

    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Index Storage");
    IDE_TEST( smnManager::destroyIndexes() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Memory Garbage Collector Shutdown");
    IDE_TEST( smaLogicalAger::shutdownAll() != IDE_SUCCESS );

    //BUG-35886 server startup will fail in a test case
    //smaDeleteThread는 smaLogicalAger::destroyStatic() 수행전에 shutdown
    //되야한다.
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Delete Manager Shutdown");
    IDE_TEST( smaDeleteThread::shutdownAll() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Memory Garbage Collector Destroy");
    IDE_TEST( smaLogicalAger::destroyStatic() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Delete Manager Destroy");
    IDE_TEST( smaDeleteThread::destroyStatic() != IDE_SUCCESS );

    /* PROJ-2733
       smxMinSCNBuild를 run하지 않았으므로 destroy만 진행한다. */
    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Minimum SCN Builder");
    IDE_TEST( smxTransMgr::destroyMinSCNBuilder() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] CheckPoint Manager");
    IDE_TEST( gSmrChkptThread.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/****************************************************/
/* Utility 프로그램에서 사용되는 초기화 Callback  */
/* Utility 프로그램은 Process단계를 거치지 않음.  */
/****************************************************/
IDE_RC smiSmUtilInit(smiGlobalCallBackList*    aCallBack)
{
    struct rlimit sLimit;
    UInt          sTransCnt = 0;

    /* BUG-22201: Disk DataFile의 최대크기가 32G를 넘어서는 파일생성이 성공하고
     * 있습니다.
     *
     * 최대크기가 32G를 넘지 않도록 보정함. ( SD_MAX_FPID_COUNT: 2^22 )
     * */
    IDE_TEST(idlOS::getrlimit( RLIMIT_FSIZE, &sLimit) != 0 );
    gDBFileMaxPageCntOfDRDB = sLimit.rlim_cur / SD_PAGE_SIZE;

    gDBFileMaxPageCntOfDRDB = gDBFileMaxPageCntOfDRDB > SD_MAX_FPID_COUNT ? SD_MAX_FPID_COUNT : gDBFileMaxPageCntOfDRDB;

    /* -------------------------
     * [0] Init Idu module
     * ------------------------*/
    IDE_TEST( iduMemMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );
    iduLatch::initializeStatic( IDU_CLIENT_TYPE );
    IDE_TEST( iduCond::initializeStatic() != IDE_SUCCESS );

    /* -------------------------
     * [1] Property Loading
     * ------------------------*/
    IDE_TEST( idp::initialize(NULL, NULL) != IDE_SUCCESS );
    IDE_TEST( iduProperty::load() != IDE_SUCCESS );
    IDE_TEST( smuProperty::load() != IDE_SUCCESS );
    smuProperty::init4Util();

    /* -------------------------
     * [2] Message Loading
     * ------------------------*/
    IDE_TEST_RAISE( smuUtility::loadErrorMsb(idp::getHomeDir(),
                                             (SChar*)"KO16KSC5601")
                    != IDE_SUCCESS, load_error_msb_error );

    /* -------------------------
     * [3] Init CallBack
     * ------------------------*/
    if( aCallBack != NULL )
    {
        gSmiGlobalCallBackList = *aCallBack;
    }

    /* ---------------------------
     * [4] 기본 SM Manager 초기화
     * --------------------------*/
    IDE_TEST( idvManager::initializeStatic() != IDE_SUCCESS );
    IDE_TEST( idvManager::startupService() != IDE_SUCCESS );

    IDE_TEST( smxTransMgr::calibrateTransCount(&sTransCnt)
              != IDE_SUCCESS );

    if( aCallBack != NULL )
    {
        IDE_TEST( smlLockMgr::initialize( sTransCnt,
                                          aCallBack->waitLockFunc,
                                          aCallBack->wakeupLockFunc )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sctTableSpaceMgr::initialize() != IDE_SUCCESS );
    IDE_TEST( smmDirtyPageMgr::initializeStatic() != IDE_SUCCESS );
    IDE_TEST( smmManager::initializeStatic() != IDE_SUCCESS );
    IDE_TEST( sddDiskMgr::initialize( (UInt)gDBFileMaxPageCntOfDRDB ) 
              != IDE_SUCCESS );
    IDE_TEST( sdsBufferMgr::initialize() != IDE_SUCCESS );
    IDE_TEST( sdbBufferMgr::initialize() != IDE_SUCCESS );
    IDE_TEST( smmTBSStartupShutdown::initializeStatic() != IDE_SUCCESS );
    IDE_TEST( svmTBSStartupShutdown::initializeStatic() != IDE_SUCCESS );
    IDE_TEST( smriChangeTrackingMgr::initializeStatic() != IDE_SUCCESS );
    IDE_TEST( smriBackupInfoMgr::initializeStatic() != IDE_SUCCESS );
    IDE_TEST( smrRecoveryMgr::initialize() != IDE_SUCCESS );
    IDE_TEST( scpManager::initializeStatic() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(load_error_msb_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't Load Error Files. \n");
        smuUtility::outputMsg("Check Directory in $ALTIBASE_HOME"IDL_FILE_SEPARATORS"msg. \n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/******************************************************/
/* Utility 프로그램에서 사용되는 Shutdown Callback  */
/******************************************************/
IDE_RC smiSmUtilShutdown(smiGlobalCallBackList* /*aCallBack*/)
{
    /* -----------------------------
     * SM Manager Shutdown
     * ----------------------------*/
    IDE_TEST( scpManager::destroyStatic() != IDE_SUCCESS );
    IDE_TEST( smrRecoveryMgr::finalize() != IDE_SUCCESS );
    IDE_TEST( smrRecoveryMgr::destroy() != IDE_SUCCESS );
    IDE_TEST( smriBackupInfoMgr::destroyStatic() != IDE_SUCCESS ); /* BUG-39938 */
    IDE_TEST( smriChangeTrackingMgr::destroyStatic() != IDE_SUCCESS ); /* BUG-39938 */
    IDE_TEST( svmTBSStartupShutdown::destroyStatic() != IDE_SUCCESS );
    IDE_TEST( smmTBSStartupShutdown::destroyStatic() != IDE_SUCCESS );
    IDE_TEST( sdbBufferMgr::destroy() != IDE_SUCCESS );
    IDE_TEST( sdsBufferMgr::destroy() != IDE_SUCCESS );
    IDE_TEST( sddDiskMgr::destroy() != IDE_SUCCESS );
    IDE_TEST( smmManager::destroyStatic( ) != IDE_SUCCESS );
    IDE_TEST( smmDirtyPageMgr::destroyStatic() != IDE_SUCCESS );
    IDE_TEST( sctTableSpaceMgr::destroy() != IDE_SUCCESS );
    IDE_TEST( smlLockMgr::destroy() != IDE_SUCCESS );

    IDE_TEST( idvManager::shutdownService() != IDE_SUCCESS );
    IDE_TEST( idvManager::destroyStatic() != IDE_SUCCESS );

    /* -----------------------------
     * Properties Shutdown
     * ----------------------------*/
    IDE_TEST( idp::destroy() != IDE_SUCCESS );

    /* -----------------------------
     * Idu Module Shutdown
     * ----------------------------*/
    IDE_TEST( iduCond::destroyStatic() != IDE_SUCCESS );
    (void) iduLatch::destroyStatic();
    IDE_TEST( iduMutexMgr::destroyStatic() != IDE_SUCCESS );
    IDE_TEST( iduMemMgr::destroyStatic() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#ifdef ALTIBASE_ENABLE_SMARTSSD
static IDE_RC smiSDMOpen( const SChar    * aLogDevice,
                          sdm_handle_t  ** aSDMHandle )
{
    sdm_api_version_t   version;

    version.major = API_VERSION_MAJOR_NUMBER;
    version.minor = API_VERSION_MINOR_NUMBER;

    IDE_TEST_RAISE( sdm_open( (const SChar *) aLogDevice, 
                              aSDMHandle,
                              &version ) 
                    != 0, error_sdm_open_failed );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_sdm_open_failed );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_SDMOpenFailed ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC smiSDMClose( sdm_handle_t * aSDMHandle )
{
    IDE_TEST( sdm_close( aSDMHandle ) != 0 )

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif /* ALTIBASE_ENABLE_SMARTSSD */

/******************************************************/
/* Normal Start-Up에서 사용되는 초기화 Callback  */
/******************************************************/

static IDE_RC smiStartupPreProcess(smiGlobalCallBackList*   aCallBack )
{
    UInt   sTransCnt = 0;

    /* -------------------------
     * [1] Property Loading
     * ------------------------*/
    IDE_TEST( smuProperty::load() != IDE_SUCCESS );

    /* -------------------------
     * [1-1] Recovery Test Code Added
     * ------------------------*/

    /* -------------------------
     * [2] Init CallBack
     * ------------------------*/
    gSmiGlobalCallBackList = *aCallBack;

    /* -------------------------
     * [2] SM Manager 초기화
     * ------------------------*/
    IDE_TEST( smxTransMgr::calibrateTransCount(&sTransCnt)
              != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0,"\n");

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Lock Manager");
    IDE_TEST( smlLockMgr::initialize( sTransCnt,
                                      aCallBack->waitLockFunc,
                                      aCallBack->wakeupLockFunc )
              != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Transaction Manager");
    IDE_TEST( smxTransMgr::initialize() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Legacy Transaction Manager");
    IDE_TEST( smxLegacyTransMgr::initializeStatic() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Index Manager");
    IDE_TEST( smnManager::initialize() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] DataPort Manager");
    IDE_TEST( scpManager::initializeStatic() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Direct-Path INSERT Manager");
    IDE_TEST( sdcDPathInsertMgr::initializeStatic()
              != IDE_SUCCESS );

    // PROJ-2118 BUG Reporting
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Set Debug Info Callback");
    iduFatalCallback::setCallback( smrLogMgr::writeDebugInfo );
    iduFatalCallback::setCallback( smrRecoveryMgr::writeDebugInfo );
    iduFatalCallback::setCallback( sdrRedoMgr::writeDebugInfo );

    //PROJ-2133 Incremental backup
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] ChangeTracking Manager");
    IDE_TEST( smriChangeTrackingMgr::initializeStatic() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] BackupInfo Manager");
    IDE_TEST( smriBackupInfoMgr::initializeStatic() != IDE_SUCCESS );

    /*
     * TASK-6198 Samsung Smart SSD GC control
     */
#ifdef ALTIBASE_ENABLE_SMARTSSD
    if ( smuProperty::getSmartSSDLogRunGCEnable() != 0 )
    {
        ideLog::log(IDE_SERVER_0," [SM-PREPARE] Samsung Smart SSD GC control");
        IDE_TEST( smiSDMOpen( smuProperty::getSmartSSDLogDevice(),
                              &gLogSDMHandle ) 
                  != IDE_SUCCESS );
    }
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC smiStartupProcess(smiGlobalCallBackList*   /*aCallBack*/ )
{
    return IDE_SUCCESS;
}

static IDE_RC smiStartupControl(smiGlobalCallBackList* /*aCallBack*/)
{
    struct rlimit sLimit;

    /* -------------------------
     * [0] Get Properties
     * ------------------------*/
    /* BUG-22201: Disk DataFile의 최대크기가 32G를 넘어서는 파일생성이 성공하고
     * 있습니다.
     *
     * 최대크기가 32G를 넘지 않도록 보정함. ( SD_MAX_FPID_COUNT: 2^22 )
     * */
    IDE_TEST(idlOS::getrlimit( RLIMIT_FSIZE, &sLimit) != 0 );
    gDBFileMaxPageCntOfDRDB = sLimit.rlim_cur / SD_PAGE_SIZE;
    gDBFileMaxPageCntOfDRDB = gDBFileMaxPageCntOfDRDB > SD_MAX_FPID_COUNT ? SD_MAX_FPID_COUNT : gDBFileMaxPageCntOfDRDB;

    /* ---------------------------
     * [1] 기본 SM Manager 초기화
     * --------------------------*/

    ideLog::log(IDE_SERVER_0,"\n");

    /* 1.메모리 테이블스페이스 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] TableSpace Manager");
    IDE_TEST( sctTableSpaceMgr::initialize() != IDE_SUCCESS );

    /* 2.Dirty Page 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Dirty Page Manager");
    IDE_TEST( smmDirtyPageMgr::initializeStatic() != IDE_SUCCESS );

    /* 3.메모리 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Memory Manager");
    IDE_TEST( smmManager::initializeStatic() != IDE_SUCCESS );

    /* 4.테이블스페이스 관리 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Memory Tablespace");
    IDE_TEST( smmTBSStartupShutdown::initializeStatic() != IDE_SUCCESS );

    /* PROJ-1594 Volatile TBS */
    ideLog::log( IDE_SERVER_0, " [SM-PREPARE] Volatile Manager" );
    IDE_TEST( svmManager::initializeStatic() != IDE_SUCCESS );

    ideLog::log( IDE_SERVER_0, " [SM-PREPARE] Volatile Tablespace" );
    IDE_TEST( svmTBSStartupShutdown::initializeStatic()
              != IDE_SUCCESS );

    /* 5.디스크 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Disk Manager");
    IDE_TEST( sddDiskMgr::initialize( (UInt)gDBFileMaxPageCntOfDRDB )
              != IDE_SUCCESS );

    /*  PROJ_2102 Fast Secondary Buffer */
    ideLog::log( IDE_SERVER_0," [SM-PREPARE] Secondary Buffer Manager" );
    IDE_TEST( sdsBufferMgr::initialize() != IDE_SUCCESS );

    /* 6.버퍼 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Buffer Manager");
    IDE_TEST( sdbBufferMgr::initialize() != IDE_SUCCESS );

    /* ------------------------------------
     * [2] Recovery 관련 SM Manager 초기화
     * ----------------------------------*/

    /* 7.복구 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Recovery Manager");
    IDE_TEST( smrRecoveryMgr::initialize() != IDE_SUCCESS );

    /* 8.로그 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Log Manager");
    IDE_TEST( smrLogMgr::initialize() != IDE_SUCCESS );

    /* 9. 디스크 테이블스페이스 : 리커버리 문제로 복구 관리자 보다 늦게 초기화 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Tablespace");
    IDE_TEST( sdpTableSpace::initialize() != IDE_SUCCESS );

    /* 10.백업 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Backup Manager");
    IDE_TEST( smrBackupMgr::initialize() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Index Pool");
    IDE_TEST( smcTable::initialize() != IDE_SUCCESS );
    
    /* 11.템프 테이블 관리자 */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] TempTable Manager      ");
    IDE_TEST( smiTempTable::initializeStatic() != IDE_SUCCESS );
    ideLog::log(IDE_SERVER_0,"[SUCCESS]\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC smiStartupMetaFirstHalf( UInt     aActionFlag )
{
    smmTBSNode * sTBSNode;
    smmMemBase   sMemBase;
    UInt         sEntryCnt;
    UInt         sCurEntryCnt;

    // aActionFlag가 SMI_STARTUP_RESETLOGS이면 resetlog를 한다.
    ideLog::log(IDE_SERVER_0,"\n");

    // To fix BUG-22158
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Check DataBase");

    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    IDE_TEST( smmManager::readMemBaseFromFile( sTBSNode,
                                               &sMemBase )
              != IDE_SUCCESS );
    IDE_TEST( smmDatabase::checkVersion( &sMemBase ) != IDE_SUCCESS );

    if ( smrRecoveryMgr::isCTMgrEnabled() == ID_TRUE )
    {
        IDE_TEST( smriChangeTrackingMgr::checkDBName( sMemBase.mDBname )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( smrRecoveryMgr::getBIMgrState() != SMRI_BI_MGR_FILE_REMOVED )
    {
        IDE_TEST( smriBackupInfoMgr::checkDBName( sMemBase.mDBname )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /* BUG-31862 resize transaction table without db migration */
    IDE_DASSERT( smmDatabase::checkTransTblSize( &sMemBase ) == IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Identify Database");

    //[0] Secondary Buffer의 속성 검사 
    IDE_TEST( sdsBufferMgr::identify( NULL /*aStatistics*/ ) != IDE_SUCCESS );

    // [1] 모든 데이타파일들의 미디어오류를 검사한다
    IDE_TEST( smrRecoveryMgr::identifyDatabase( aActionFlag ) != IDE_SUCCESS );

    if ((aActionFlag & SMI_STARTUP_ACTION_MASK) != SMI_STARTUP_RESETLOGS)
    {
        IDE_TEST( smrRecoveryMgr::identifyLogFiles() != IDE_SUCCESS );
    }

    if( smuProperty::getUseDWBuffer() == ID_TRUE )
    {
        IDE_TEST( sdbDWRecoveryMgr::recoverCorruptedPages()
                  != IDE_SUCCESS );
    }

    // DWFile을 참조한 DATA page의 corruption 검사가 끝난 후
    // flusher를 생성, 초기화한다.
    ideLog::log( IDE_SERVER_0," [SM-PREPARE] Secondary Buffer Flusher" );
    IDE_TEST( sdsFlushMgr::initialize( smuProperty::getSBufferFlusherCnt() ) 
              != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Buffer Flusher");
    IDE_TEST( sdbFlushMgr::initialize(smuProperty::getBufferFlusherCnt() )
              != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Transaction Segment Manager");

    sEntryCnt = smrRecoveryMgr::getTXSEGEntryCnt();

    IDE_TEST( sdcTXSegMgr::adjustEntryCount( sEntryCnt, &sCurEntryCnt )
              != IDE_SUCCESS );

    IDE_TEST( sdcTXSegMgr::initialize( ID_FALSE /* aIsAttachSegment */ ) 
              != IDE_SUCCESS );

    /* FOR A4 : Disk Table의 경우 Undo 시에 Index를 참조한다.
                이를 위하여 모든 table의 index는 undo 전에 사용할
                준비가 완료되어 있어야 한다.
                따라서 다음과 같이 restart 순서를 바꾼다.

                redo --> prepare index header --> undo
    */

    /* BUG-38962
     * restart recovery 이전에 MinSCNBuilder를 초기화 한다. */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Initialize Minimum SCN Builder ");
    IDE_TEST( smxTransMgr::initializeMinSCNBuilder() != IDE_SUCCESS );
    ideLog::log(IDE_SERVER_0,"[SUCCESS]\n");

    /* -------------------------
     * [2] restart recovery
     * ------------------------*/
    if ( smrRecoveryMgr::getArchiveMode() == SMI_LOG_ARCHIVE )
    {
        if ( smuProperty::getArchiveThreadAutoStart() != 0 )
        {
            ideLog::log(IDE_SERVER_0," [SM-PREPARE] Archive Log Thread Start...");
            IDE_TEST( smrLogMgr::startupLogArchiveThread()
                      != IDE_SUCCESS );
        }
    }

    if ( (aActionFlag & SMI_STARTUP_ACTION_MASK) == SMI_STARTUP_RESETLOGS )
    {
        IDE_TEST( smrRecoveryMgr::resetLogFiles() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC smiStartupMetaLatterHalf()
{
    UInt         sParallelFactor;

    sParallelFactor = smuProperty::getParallelLoadFactor();

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] CheckPoint Manager");
    IDE_TEST( gSmrChkptThread.initialize() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Rebuild Transaction Segment Entries");
    IDE_TEST( sdcTXSegMgr::rebuild() != IDE_SUCCESS );

    /* -------------------------
     * [3] start system threads
     * ------------------------*/
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Memory Garbage Collector");
    IDE_TEST( smaLogicalAger::initializeStatic() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Delete Manager");
    IDE_TEST( smaDeleteThread::initializeStatic() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] CheckPoint Thread Start");
    IDE_TEST( gSmrChkptThread.startThread() != IDE_SUCCESS );

    /* -------------------------
     * [4] DB Refining
     * ------------------------*/
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Database Refining");
    IDE_TEST( smapManager::doIt( (SInt)(sParallelFactor) ) != IDE_SUCCESS );

    /* PROJ-2733
       smmManager::initSCN()에서
       ( smapManager::doit() 내부에서 호출함)
       AgingViewSCN, AccessSCN 세팅된후에 MinSCNBuilder 쓰레드가 시작되도록 실행순서 변경합니다. */

    /* BUG-38962
     * restart recovery 완료 후 MinSCNBuilder 쓰레드를 run 한다. */
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Minimum SCN Builder ");
    IDE_TEST( smxTransMgr::startupMinSCNBuilder() != IDE_SUCCESS );

    IDE_TEST( smcCatalogTable::doAction4EachTBL( NULL, /* aStatistics */
                                                 smcTable::initRowTemplate,
                                                 NULL )
              != IDE_SUCCESS );

    // remove unused table backups
    IDE_TEST( smcTable::deleteAllTableBackup() != IDE_SUCCESS );

    // For In-Doubt transaction
    IDE_TEST( smrRecoveryMgr::acquireLockForInDoubt() != IDE_SUCCESS );

    // BUG-28819 [SM] REBUILD_MIN_VIEWSCN_INTERVAL_을 0으로 유지하고
    // 서버 restart하면 비정상 종료합니다.
    // refile단계 마지막에 Min View SCN을 rebuild합니다.
    IDE_TEST( smxTransMgr::rebuildMinViewSCN( NULL /*idvSQL*/) != IDE_SUCCESS );

    /* ----------------------------
     * [5] memory index rebuiding
     * --------------------------*/

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Index Rebuilding");
    IDE_TEST( smnManager::rebuildIndexes() != IDE_SUCCESS );

    /* BUG-42724 : server 재시작 후 XA 트래잭션으로 commit/rollback되는 경우 aging할 때
     * 빠진 플래그가 존재하여 죽는 현상이 발생한다. 따라서 인덱스 리빌드 완료 후 XA TX에
     * 의해 insert/update된 레코드의 관련 OID flag들을 추가/수정 한다. */
    IDE_TEST( smxTransMgr::setOIDFlagForInDoubtTrans() != IDE_SUCCESS );
    
    /* ----------------------------
     * [6] resize transaction table size
     * --------------------------*/

    /*BUG-31862 resize transaction table without db migration */
    IDE_TEST( smmDatabase::refineTransTblSize() != IDE_SUCCESS );

    /* ----------------------------
     * [7] StatThread
     * --------------------------*/

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Stat Thread Start...");
    IDE_TEST( smiStatistics::initializeStatic() != IDE_SUCCESS );

    IDE_TEST( sdtWAExtentMgr::prepareCachedFreeNExts( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC smiStartupMeta(smiGlobalCallBackList* /*aCallBack*/,
                             UInt                   aActionFlag)
{
    /* startup Meta */
    IDE_TEST( smiStartupMetaFirstHalf( aActionFlag ) != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Restart Recovery");
    IDE_TEST( smrRecoveryMgr::restart( 
            smuProperty::getEmergencyStartupPolicy() ) != IDE_SUCCESS );

    IDE_TEST( smiStartupMetaLatterHalf() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC smiStartupService(smiGlobalCallBackList* /*aCallBack*/)
{
    if ( iduProperty::getEnableRecTest() == 1 )
    {
        IDE_TEST( smiVerifySM( NULL, SMI_VERIFY_TBS )
                  != IDE_SUCCESS );
    }

    /* service startup 단계에서 volatile table들을 초기화한다. */
    IDE_TEST( smaRefineDB::initAllVolatileTables() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiSetRecStartupEnd()
{
    return IDE_SUCCESS;
}

static IDE_RC smiStartupShutdown(smiGlobalCallBackList* /*aCallBack*/)
{
    UInt           sParallelFactor;

    sParallelFactor = smuProperty::getParallelLoadFactor();

    switch( gStartupPhase )
    {
        case SMI_STARTUP_SHUTDOWN:
            IDE_ASSERT( 0 );
            break;

        case SMI_STARTUP_SERVICE:
        case SMI_STARTUP_META:
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Stat Thread");
            IDE_TEST( smiStatistics::finalizeStatic() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] CheckPoint Manager Thread");
            IDE_TEST( gSmrChkptThread.shutdown() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] CheckPoint Manager");
            IDE_TEST( gSmrChkptThread.destroy() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Memory Garbage Collector Thread");
            IDE_TEST( smaLogicalAger::shutdownAll() != IDE_SUCCESS );

            //BUG-35886 server startup will fail in a test case
            //smaDeleteThread는 smaLogicalAger::destroyStatic() 수행전에 shutdown
            //되야한다.
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Delete Manager Thread");
            IDE_TEST( smaDeleteThread::shutdownAll() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Memory Garbage Collector");
            IDE_TEST( smaLogicalAger::destroyStatic() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Delete Manager");
            IDE_TEST( smaDeleteThread::destroyStatic() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Minimum SCN Builder");
            IDE_TEST( smxTransMgr::shutdownMinSCNBuilder() != IDE_SUCCESS );

            /* BUG-41541 __FORCE_INDEX_PERSISTENCE_MODE의 값이 0인 경우 
             * persistent index 기능을 사용하지 않는다.*/
            if( smuProperty::forceIndexPersistenceMode() != SMN_INDEX_PERSISTENCE_NOUSE )
            {
                /* BUG-34504 - smaLogicalAger에서 index header에 접근하기 때문에,
                 * 각종 thread들을 모두 shutdown 한 후, persistent index들을 쓰고,
                 * 마지막으로 runtime상의 index 들을 파괴한다. */
                ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Write Persistent Indice");
                IDE_TEST( smnpIWManager::doIt((SInt)sParallelFactor) != IDE_SUCCESS );
            }

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Index Storage Area");
            IDE_TEST( smnManager::destroyIndexes() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Transaction Segment Manager");
            IDE_TEST( sdcTXSegMgr::destroy() != IDE_SUCCESS );

            /*
             * BUG-24518 [MDB] Shutdown Phase에서 메모리 테이블 Compaction이
             * 필요합니다.
             */
            if( smuProperty::getTableCompactAtShutdown() == 1 )
            {
                ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Table Compaction");
                IDE_TEST( smaRefineDB::compactTables() != IDE_SUCCESS );
            }

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] MVRDB Table Runtime Information");
            IDE_TEST( smcCatalogTable::finalizeCatalogTable() != IDE_SUCCESS );


            /* BUG-24781 서버종료과정에서 플러쉬관리자가 복구관리자보다
             * 이전에 destroy되어야함. flushForCheckpoint 수행과정중에 LFG에
             * 접근하기 때문에 복구관리자가 해제되면 서버가 비정상종료한다. */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Flush All DirtyPages And Checkpoint Database...");
            IDE_TEST( smrRecoveryMgr::finalize() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Flush Manager");
            IDE_TEST( sdbFlushMgr::destroy() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Secondary buffer Flusher ");
            IDE_TEST( sdsFlushMgr::destroy() != IDE_SUCCESS );
       
            ideLog::log( IDE_SERVER_0," [SM-SHUTDOWN] Flush ALL Secondary Buffer Meta data" );
            IDE_TEST( sdsBufferMgr::finalize() != IDE_SUCCESS );

        case SMI_STARTUP_CONTROL:

            /* 로그 관리자 작업 종료*/
            IDE_TEST( smrLogMgr::shutdown() != IDE_SUCCESS );

            /* 11.템프 테이블 관리자 */ 
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] TempTable Manager");
            IDE_TEST( smiTempTable::destroyStatic() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Index Pool Manager");
            IDE_TEST( smcTable::destroy() != IDE_SUCCESS );
 
            /* 10.백업 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Backup Manager");
            IDE_TEST( smrBackupMgr::destroy() != IDE_SUCCESS );

            /* 9.디스크 테이블스페이스 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Tablespace Manager");
            IDE_TEST( sdpTableSpace::destroy() != IDE_SUCCESS );
            
            /* 8.로그 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Log Manager");
            IDE_TEST( smrLogMgr::destroy() != IDE_SUCCESS );
 
            /* 7.복구 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Recovery Manager");
            IDE_TEST( smrRecoveryMgr::destroy() != IDE_SUCCESS );

            /* 6.버퍼 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Buffer Manager");
            IDE_TEST( sdbBufferMgr::destroy() != IDE_SUCCESS );

            ideLog::log( IDE_SERVER_0," [SM-SHUTDOWN] Secondary Buffer Manager" );
            IDE_TEST( sdsBufferMgr::destroy() != IDE_SUCCESS );

            /* 5.디스크 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Disk Manager");
            IDE_TEST( sddDiskMgr::destroy() != IDE_SUCCESS );

            /* PROJ-1594 Volatile TBS */
            ideLog::log( IDE_SERVER_0, " [SM-SHUTDOWN] Volatile Tablespace Destroy..." );
            IDE_TEST( svmTBSStartupShutdown::destroyStatic() != IDE_SUCCESS );

            ideLog::log( IDE_SERVER_0, " [SM-SHUTDOWN] Volatile Manager" );
            IDE_TEST( svmManager::destroyStatic() != IDE_SUCCESS );
            
            /* 4.테이블스페이스 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Memory Tablespace Destroy...");
            IDE_TEST( smmTBSStartupShutdown::destroyStatic() != IDE_SUCCESS );

            /* 3.메모리 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Memory Manager");
            IDE_TEST( smmManager::destroyStatic() != IDE_SUCCESS );

            /* 2.Dirty Page 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Dirty Page Manager");
            IDE_TEST( smmDirtyPageMgr::destroyStatic() != IDE_SUCCESS );

            /* 1.메모리 테이블스페이스 관리자 */
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] TableSpace Manager");
            IDE_TEST( sctTableSpaceMgr::destroy() != IDE_SUCCESS );

        case SMI_STARTUP_PROCESS:
        case SMI_STARTUP_PRE_PROCESS:
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] BackupInfo Manager");
            IDE_TEST( smriBackupInfoMgr::destroyStatic() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] ChangeTracking Manager");
            IDE_TEST( smriChangeTrackingMgr::destroyStatic() != IDE_SUCCESS );

            // PROJ-2118 BUG Reporting
            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Remove Debug Info Callback");
            iduFatalCallback::unsetCallback( smrLogMgr::writeDebugInfo );
            iduFatalCallback::unsetCallback( smrRecoveryMgr::writeDebugInfo );
            iduFatalCallback::unsetCallback( sdrRedoMgr::writeDebugInfo );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Direct-Path INSERT Manager");
            IDE_TEST( sdcDPathInsertMgr::destroyStatic() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] DataPort");
            IDE_TEST( scpManager::destroyStatic() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Index Manager");
            IDE_TEST( smnManager::destroy() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Legacy Transaction Manager");
            IDE_TEST( smxLegacyTransMgr::destroyStatic() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Transaction Manager");
            IDE_TEST( smxTransMgr::destroy() != IDE_SUCCESS );

            ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Lock Manager");
            IDE_TEST( smlLockMgr::destroy() != IDE_SUCCESS );

            /*
             * TASK-6198 Samsung Smart SSD GC control
             */
#ifdef ALTIBASE_ENABLE_SMARTSSD
            if( smuProperty::getSmartSSDLogRunGCEnable() != 0 )
            {
                ideLog::log(IDE_SERVER_0," [SM-SHUTDOWN] Samsung Smart SSD GC control");
                IDE_TEST( smiSDMClose( gLogSDMHandle ) 
                          != IDE_SUCCESS );
            }
#endif 
            break;

        default:
            IDE_ASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log(IDE_SERVER_0,"[FAILURE]\n");

    return IDE_FAILURE;
}

IDE_RC smiStartup(smiStartupPhase        aPhase,
                  UInt                   aActionFlag,
                  smiGlobalCallBackList* aCallBack)
{
    switch(aPhase)
    {
        case SMI_STARTUP_PRE_PROCESS:  /* for DB Creation    */
            IDE_TEST( smiStartupPreProcess(aCallBack) != IDE_SUCCESS );
            break;
        case SMI_STARTUP_PROCESS:  /* for DB Creation    */
            IDE_TEST( smiStartupProcess(aCallBack) != IDE_SUCCESS );
            break;
        case SMI_STARTUP_CONTROL:  /* for Recovery       */
            IDE_TEST( smiStartupControl(aCallBack) != IDE_SUCCESS );
            break;
        case SMI_STARTUP_META:  /* for upgrade meta   */
            IDE_TEST( smiStartupMeta( aCallBack, aActionFlag ) != IDE_SUCCESS );
            break;
        case SMI_STARTUP_SERVICE:  /* for normal service */
            IDE_TEST( smiStartupService(aCallBack) != IDE_SUCCESS );
            break;
        case SMI_STARTUP_SHUTDOWN:
            IDE_TEST( smiStartupShutdown(aCallBack) != IDE_SUCCESS );
            break;
        default:
            IDE_CALLBACK_FATAL("Invalid Start-Up Phase Request");
    }

    gStartupPhase = aPhase;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

smiStartupPhase smiGetStartupPhase()
{
    return gStartupPhase;
}
