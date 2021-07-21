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
* $Id: smmManager.cpp 90522 2021-04-09 01:29:20Z emlee $
**********************************************************************/


/* ------------------------------------------------


   Initialize

   구성요소
   mDBDir - DB_DIR_COUNT개수의 array

    -----------
   | dbs0      |
    -----------
   | dbs1      |
    -----------
   | dbs2      |
    -----------
   | dbs3      |
    -----------
   | dbs4      |
    -----------
   | dbs5      |
   ------------
   | dbs6      |
    -----------
   | dbs7      |
    -----------

    mDBFile
    화일이 생길수 있는 최대 개수만큼 미리 databaseFile을 alloc한다.
    처음에는 전부 다 사용되지 않을 수 있다.

    --------------
   | mDBFile ptr  |
    --------------
       |
       |
    -----------      -------------
   | pingpong0 | -  | databaeFile |
    -----------      -------------
       |                  |
       |             -------------
       |            | databaeFile |
       |             -------------
       |                  |
       |                  ...
    -----------
   | pingpong1 | - ...
    -----------




 * ----------------------------------------------*/

#include <smiFixedTable.h>
#include <sctTableSpaceMgr.h>
#include <smErrorCode.h>
#include <smc.h>
#include <smDef.h>
#include <smu.h>
#include <smm.h>
#include <smmReq.h>
#include <smpVarPageList.h>
#include <smiMain.h>
#include <svmExpandChunk.h>

/* ------------------------------------------------
 * [] global variable
 * ----------------------------------------------*/

void                   *smmManager::m_catTableHeader = NULL;
void                   *smmManager::m_catTempTableHeader = NULL;
smPCArr                 smmManager::mPCArray[SC_MAX_SPACE_ARRAY_SIZE]; // BUG-48513
smmGetPersPagePtrFunc   smmManager::mGetPersPagePtrFunc = NULL;

smmManager::smmManager()
{
}

/* smmManager를 초기화 한다.
 *
 * 데이터베이스가 이미 존재하는 경우, 데이터베이스의 MemBase로부터
 * 데이터베이스 정보를 읽어와서 그 정보를 토대로 smmManager를 초기화한다.
 *
 * 아직 데이터베이스가 존재하지 않는 경우, smmManager의 초기화는
 * 일부만 되며, 나머지는 createDB시에 initializeWithDBInfo를 호출하여
 * 초기화된다.
 *
 * aOp [IN] 데이터베이스 생성중인지, 일반 Startup된 경우인지 여부
 *
 */
IDE_RC smmManager::initializeStatic()
{
    idlOS::memset( mPCArray, 0, ID_SIZEOF(mPCArray) );

    IDE_TEST( smmFixedMemoryMgr::initializeStatic() != IDE_SUCCESS );

    // To Fix BUG-14185
     // Free Page List 관리자 초기화
    IDE_TEST( smmFPLManager::initializeStatic( ) != IDE_SUCCESS );

    IDE_TEST( smmDatabase::initialize() != IDE_SUCCESS );

    m_catTableHeader        = NULL;
    m_catTempTableHeader    = NULL;

    IDE_TEST( smLayerCallback::prepareIdxFreePages() != IDE_SUCCESS );

    smpVarPageList::initAllocArray();

    smLayerCallback::setSmmCallbacksInSmx();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
/* DB파일 크기가 OS의 파일크기 제한에 걸리지는 않는지 체크한다.
 *
 */
IDE_RC smmManager::checkOSFileSize( vULong aDBFileSize )
{
#if !defined(WRS_VXWORKS)
    struct rlimit  limit;
    /* ------------------------------------------------------------------
     *  [4] Log(DB) File Size 검사
     * -----------------------------------------------------------------*/
    IDE_TEST_RAISE(idlOS::getrlimit(RLIMIT_FSIZE, &limit) != 0,
                   getrlimit_error);

    IDE_TEST_RAISE( aDBFileSize -1 > limit.rlim_cur ,
                    check_OSFileLimit);


    return IDE_SUCCESS;

    IDE_EXCEPTION(check_OSFileLimit);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_OSFileSizeLimit_ERROR,
                                (ULong) aDBFileSize ));
    }
    IDE_EXCEPTION(getrlimit_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_GETLIMIT_ERROR));
    }
    IDE_EXCEPTION_END;


    return IDE_FAILURE;
#else
    return IDE_SUCCESS;
#endif

}

SInt
smmManager::getCurrentDB(smmTBSNode * aTBSNode)
{
    IDE_DASSERT( (aTBSNode->mTBSAttr.mMemAttr.mCurrentDB == 0) ||
                 (aTBSNode->mTBSAttr.mMemAttr.mCurrentDB == 1) );

    return aTBSNode->mTBSAttr.mMemAttr.mCurrentDB;
}

SInt
smmManager::getNxtStableDB( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( (aTBSNode->mTBSAttr.mMemAttr.mCurrentDB == 0) ||
                 (aTBSNode->mTBSAttr.mMemAttr.mCurrentDB == 1) );

    return (aTBSNode->mTBSAttr.mMemAttr.mCurrentDB + 1) % SM_PINGPONG_COUNT;
}



/* 사용자가 생성하려는 데이터베이스 크기에 근접하는
 * 데이터베이스 생성을 위해 생성할 Page 수를 계산한다.
 *
 * 사용자가 지정한 크기와 정확히 일치하는 데이터베이스를 생성할 수 없는
 * 이유는, 하나의 데이터베이스는 항상 Expand Chunk크기의 배수로
 * 생성되기 때문이다.
 *
 * aDbSize         [IN] 생성하려는 데이터베이스 크기
 * aChunkPageCount [IN] 하나의 Expand Chunk가 지니는 Page의 수
 *
 */
ULong smmManager::calculateDbPageCount( ULong aDbSize, ULong aChunkPageCount )
{
    vULong sCalculPageCount;
    vULong sRequestedPageCount;

    // aDbSize 가 0으로 들어와도 죽어서는 안된다.
    // MM단에서 이 함수 호출후에 에러처리하고있기 때문
    IDE_DASSERT( aChunkPageCount > 0  );

    sRequestedPageCount = aDbSize  / SM_PAGE_SIZE;

    // Expand Chunk Page수의 배수가 되도록 설정.
    // BUG-15288 단 Max DB SIZE를 넘을 수 없다.
    sCalculPageCount =
        aChunkPageCount * (sRequestedPageCount / aChunkPageCount);

    // 0번 페이지를 저장하는 META PAGE수를 더하지 않는다.
    // smmTBSCreate::createTBS에서 aInitSize가 Meta Page를 포함하지 않은
    // 크기를 받아서 여기에 META PAGE수를 더하기 때문이다.


    return sCalculPageCount ;
}

/*  DB File 객체들과 관련 데이터 구조들을 초기화한다.
 *
 *  [IN] aTBSNode - Tablespace의  Node
 * aChunkPageCount  [IN] 하나의 Expand Chunk가 지니는 Page의 수
 */
IDE_RC smmManager::initDBFileObjects(smmTBSNode *       aTBSNode,
                                     UInt               aDBFilePageCount)
{
    UInt       i;
    UInt       j;
    ULong      sRemain;
    UInt       sChunkPageCount = smuProperty::getExpandChunkPageCount();
    scPageID   sFstPageID = 0;
    scPageID   sLstPageID = 0;
    scPageID   sPageCountPerFile;

    aTBSNode->mDBMaxPageCount =
        calculateDbPageCount( smuProperty::getMaxDBSize(), sChunkPageCount);

    sRemain = aTBSNode->mDBMaxPageCount % aDBFilePageCount;

    aTBSNode->mHighLimitFile = aTBSNode->mDBMaxPageCount / aDBFilePageCount;

    if ( sRemain > 0 )
    {
        aTBSNode->mHighLimitFile += 1;
    }

    for ( i=0 ; i < SMM_PINGPONG_COUNT ; i++ )
    {
        /* smmManager_initDBFileObjects_calloc_DBFile.tc */
        IDU_FIT_POINT("smmManager::initDBFileObjects::calloc::DBFile");
        IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMM,
                                   (SInt)aTBSNode->mHighLimitFile,
                                   ID_SIZEOF(smmDatabaseFile *),
                                   (void**)&(aTBSNode->mDBFile[i]))
                 != IDE_SUCCESS);
    }

    // fix BUG-17343 loganchor에 Stable/Unstable Chkpt Image에
    // 대한 생성 정보를 저장
    /* smmManager_initDBFileObjects_calloc_CrtDBFileInfo.tc */
    IDU_FIT_POINT("smmManager::initDBFileObjects::calloc::CrtDBFileInfo");
    IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SMM,
                    (SInt)aTBSNode->mHighLimitFile,
                    ID_SIZEOF(smmCrtDBFileInfo),
                    (void**)&(aTBSNode->mCrtDBFileInfo))
                  != IDE_SUCCESS );

    initCrtDBFileInfo( aTBSNode );

    // 최대 생길수 있는 화일까지 databaseFile객체를 생성해둠
    for (i = 0; i< aTBSNode->mHighLimitFile; i++)
    {
        for (j = 0; j < SMM_PINGPONG_COUNT; j++)
        {
            /* TC/FIT/Limit/sm/smm/smmManager_initDBFileObjects_malloc.sql */
            IDU_FIT_POINT_RAISE( "smmManager::initDBFileObjects::malloc",
                                  insufficient_memory );

            IDE_TEST_RAISE(iduMemMgr::malloc(
                                    IDU_MEM_SM_SMM,
                                    ID_SIZEOF(smmDatabaseFile),
                                    (void**)&(aTBSNode->mDBFile[ j ][ i ])) != IDE_SUCCESS,
                           insufficient_memory );

            sPageCountPerFile =
                smmManager::getPageCountPerFile( aTBSNode,
                                                 i );
            sLstPageID = sFstPageID + sPageCountPerFile - 1;

            IDE_TEST( createDBFileObject( aTBSNode,
                                          j,   // PINGPONG 번호
                                          i,   // 데이타베이스파일번호
                                          sFstPageID,
                                          sLstPageID,
                                          (smmDatabaseFile*) aTBSNode->mDBFile[ j ][ i ] )
                      != IDE_SUCCESS );
        }

        // 첫번째 PageID를 계산한다.
        sFstPageID += sPageCountPerFile;
    }


    // BUGBUG-1548 Lock정보 초기화는 어디에서 수행?

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/**
    DB File 객체를 생성한다.

    [IN] aTBSNode     - DB File객체를 생성할 Tablespace Node
    [IN] aPingPongNum - DB File의 핑퐁 번호
    [IN] aFileNum     - DB File의 파일 번호
    [IN] aFstPageID   - DB File이 저장할 Page 범위 (시작)
    [IN] aLstPageID   - DB File이 저장할 Page 범위 (끝)
    [IN/OUT] aDBFileMemory - DB File 객체의 메모리를 넘겨받아
                             해당 메모리에 DB File 객체를 초기화한다.
 */
IDE_RC smmManager::createDBFileObject( smmTBSNode       * aTBSNode,
                                       UInt               aPingPongNum,
                                       UInt               aFileNum,
                                       scPageID           aFstPageID,
                                       scPageID           aLstPageID,
                                       smmDatabaseFile  * aDBFileObj )
{
    SChar   sDBFileName[SM_MAX_FILE_NAME];
    SChar * sDBFileDir;
    idBool  sFound;


    aDBFileObj = new ( aDBFileObj ) smmDatabaseFile;

    IDE_TEST( aDBFileObj->initialize(
                  aTBSNode->mHeader.mID,
                  aPingPongNum,
                  aFileNum,
                  &aFstPageID,
                  &aLstPageID )
              != IDE_SUCCESS);

    sFound = smmDatabaseFile::findDBFile( aTBSNode,
                                          aPingPongNum,
                                          aFileNum,
                                          (SChar*)sDBFileName,
                                          &sDBFileDir);
    if ( sFound == ID_TRUE )
    {
        // To Fix BUG-17997 [메모리TBS] DISCARD된 Tablespace를
        //                  DROP시에 Checkpoint Image 지워지지 않음
        //
        // MEDIA단계 초기화시에 DB File명과 패스를 세팅한다.
        aDBFileObj->setFileName(sDBFileName);
        aDBFileObj->setDir(sDBFileDir);
    }
    else
    {
        // Discard된 Tablespace의 경우 DB File이 존재하지 않을 수도 있다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* DB File객체와 관련 자료구조를 해제한다.

   aTBSNode [IN] Tablespace의 Node

   참고 : 이 함수는 initDBFileObjects에서 초기화한 내용을 파괴한다.
 */
IDE_RC smmManager::finiDBFileObjects( smmTBSNode * aTBSNode )
{
    UInt   i;
    UInt   j;

    IDE_DASSERT( aTBSNode != NULL );

    // initDBFileObjects가 호출된 상태인지 검사.
    // 다음 Field들이 0이 아닌 값이어야 한다.
    IDE_ASSERT( aTBSNode->mHighLimitFile != 0);
    IDE_ASSERT( aTBSNode->mDBMaxPageCount != 0);


    /* ------------------------------------------------
     *  Close all db file & memory free
     * ----------------------------------------------*/
    for (i = 0; i < aTBSNode->mHighLimitFile; i++)
    {
        for (j = 0; j < SMM_PINGPONG_COUNT; j++)
        {
            IDE_ASSERT( aTBSNode->mDBFile[j][i] != NULL );

            IDE_TEST(
                ((smmDatabaseFile*)aTBSNode->mDBFile[j][i])->destroy()
                != IDE_SUCCESS);


            IDE_TEST(iduMemMgr::free(aTBSNode->mDBFile[j][i])
                     != IDE_SUCCESS);

            aTBSNode->mDBFile[j][i] = NULL ;
        }
    }

    for (i = 0; i < SMM_PINGPONG_COUNT; i++)
    {
        IDE_ASSERT( aTBSNode->mDBFile[i] != NULL);


        IDE_TEST(iduMemMgr::free(aTBSNode->mDBFile[i])
                 != IDE_SUCCESS);

        aTBSNode->mDBFile[i] = NULL;
    }


    IDE_ASSERT( aTBSNode->mCrtDBFileInfo!= NULL );

    IDE_TEST(iduMemMgr::free(aTBSNode->mCrtDBFileInfo)
             != IDE_SUCCESS);

    aTBSNode->mCrtDBFileInfo= NULL;

    // initDBFileObjects에서 초기화했던 Field들을 0으로 세팅
    aTBSNode->mHighLimitFile = 0;
    aTBSNode->mDBMaxPageCount = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smmManager::destroyStatic()
{

    smLayerCallback::unsetSmmCallbacksInSmx();

    /* ------------------------------------------------
     *  release all index free page list
     * ----------------------------------------------*/
    IDE_TEST( smLayerCallback::releaseIdxFreePages() != IDE_SUCCESS );

    IDE_TEST( smmDatabase::destroy() != IDE_SUCCESS );

    IDE_TEST( smmFPLManager::destroyStatic() != IDE_SUCCESS );

    IDE_TEST( smmFixedMemoryMgr::destroyStatic() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Tablespace를 위한 Page관리 시스템 해제

    aTBSNode [IN] 해제하려는 Tablespace의 Node

 */
IDE_RC smmManager::finiPageSystem(smmTBSNode * aTBSNode)
{
    IDE_DASSERT( aTBSNode != NULL );

    // To Fix BUG-14185
    // Free Page List 관리자 해제
    IDE_TEST( smmFPLManager::destroy( aTBSNode ) != IDE_SUCCESS );


    // Expand Chunk 관리자 해제
    IDE_TEST( smmExpandChunk::destroy( aTBSNode ) != IDE_SUCCESS );

    // BUGBUG-1548
    // Index Memory Pool을 TBS마다 따로 두고 Index메모리를 해제해야함

    // 해당 Tablespace를 위한 Dirty Page관리자를 제거
    IDE_TEST( smmDirtyPageMgr::removeDPMgr( aTBSNode )
                != IDE_SUCCESS );

    // Tablespace의 Page 메모리 반납
    IDE_TEST( freeAllPageMemAndPCH( aTBSNode ) != IDE_SUCCESS );

    // Page Memory Pool 제거
    IDE_TEST(destroyPagePool( aTBSNode ) != IDE_SUCCESS );

    // PCH Memory Pool 제거
    IDE_TEST(aTBSNode->mPCHMemPool.destroy() != IDE_SUCCESS);

    // PCH Array제거
    IDE_TEST( freePCHArray( aTBSNode->mHeader.mID ) != IDE_SUCCESS );
    
    // BUG-19384 : table space offline과 동시에 V$TABLESPACES가
    // 조회될 때 mem base가 null로 설정되지 않아서 비정상 종료됨
    IDU_FIT_POINT( "1.BUG-19384@smmManager::finiPageSystem" );

    // BUG-19299 : tablespace의 mem base를 null로 설정
    aTBSNode->mMemBase = NULL;

    // 아직 Restore되지 않은 상태로 설정한다
    aTBSNode->mRestoreType = SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET;

    // TBS가 DROP되어도 Lock Item은 그대로 둔다
    // 현재 TBS의 Lock을 기다리고 있는 Tx들이 있을 수 있기 때문
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Tablespace안의 모든 Page Memory/PCH Entry를 해제한다.

    [IN] aTBSNode - Tablespace의 Node
 */
IDE_RC smmManager::freeAllPageMemAndPCH(smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    idBool sFreePageMemory;

    switch( aTBSNode->mRestoreType )
    {
        case SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET :
            // Media Recovery를 위해 PAGE단계까지 초기화 하였으나,
            // prepare/restore되지 않은 tablespace.
            //
            // 아직 prepare/restore되이 않았으므로
            // 해제할 page memory가 없다.
            sFreePageMemory = ID_FALSE;
            break;

        case SMM_DB_RESTORE_TYPE_SHM_CREATE :
        case SMM_DB_RESTORE_TYPE_SHM_ATTACH :
            // Drop/Offline의 Pending으로 호출된 경우
            // 호출된 경우 공유메모리 자체를 제거한다
            if ( sctTableSpaceMgr::hasState(
                     & aTBSNode->mHeader,
                     SCT_SS_FREE_SHM_PAGE_ON_DESTROY )
                 == ID_TRUE )
            {
                // Tablespace의 Page Memory를 해제한다.
                sFreePageMemory = ID_TRUE;
            }
            else
            {
                // Shutdown시 ONLINE Tablespace의 경우
                sFreePageMemory = ID_FALSE;
            }
            break;

        case SMM_DB_RESTORE_TYPE_DYNAMIC :
            // 일반 메모리의 경우 무조건 Page memory를 해제
            sFreePageMemory = ID_TRUE;
            break;
        default :
            IDE_ASSERT(0);
    }

    IDE_TEST(freeAll(aTBSNode, sFreePageMemory)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    Tablespace를 위한 매체(DB File) 관리 시스템 해제

    aTBSNode [IN] 해제하려는 Tablespace의 Node

 */
IDE_RC smmManager::finiMediaSystem(smmTBSNode * aTBSNode)
{
    IDE_DASSERT( aTBSNode != NULL );

    // Tablespace는 아직 prepare/restore되지 않은 상태여야한다.
    IDE_ASSERT( aTBSNode->mRestoreType
                == SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET );

//    IDE_TEST( freeMetaPage( aTBSNode ) != IDE_SUCCESS );

    IDE_TEST( finiDBFileObjects( aTBSNode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Tablespace Node를 파괴한다.

    aTBSNode [IN] 해제할 Tablespace
 */
IDE_RC smmManager::finiMemTBSNode(smmTBSNode * aTBSNode)
{
    IDE_DASSERT(aTBSNode != NULL );

    // Tablespace는 아직 prepare/restore되지 않은 상태여야한다.
    IDE_ASSERT( aTBSNode->mRestoreType
                == SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET );

    // Disk, Memory Tablespace 공통 파괴자 호출
    // - Lock정보를 포함한 TBSNode의 모든 정보를 파괴
    IDE_TEST( sctTableSpaceMgr::destroyTBSNode( & aTBSNode->mHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    Tablespace의 모든 Checkpoint Image File들을 지워준다.

    만약 파일이 존재하지 않을 경우 무시하고 넘어간다.

    [IN] aTBSNode           - Tablespace Node
    [IN] aRemoveImageFiles  - Checkpoint Image File들을 지워야 할 지 여부
 */


IDE_RC smmManager::closeAndRemoveChkptImages(smmTBSNode * aTBSNode,
                                             idBool       aRemoveImageFiles )
{
    smmDatabaseFile           * sDBFilePtr;
    UInt                        sWhichDB;
    UInt                        i;
    scSpaceID                   sSpaceID;


    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace( aTBSNode->mHeader.mID ) == ID_TRUE );

    sSpaceID = aTBSNode->mHeader.mID;

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,"[TBSID:%d-%s] closeAndRemove.. \n", sSpaceID, aTBSNode->mHeader.mName );


    for (sWhichDB = 0; sWhichDB < SMM_PINGPONG_COUNT; sWhichDB++)
    {
        for (i = 0; i <= aTBSNode->mLstCreatedDBFile; i++)
        {

            IDE_TEST( getDBFile( aTBSNode,
                                 sWhichDB,
                                 i, // DB File No
                                 SMM_GETDBFILEOP_NONE,
                                 &sDBFilePtr )
                      != IDE_SUCCESS );

            IDE_TEST( sDBFilePtr->closeAndRemoveDbFile( sSpaceID,
                                                        aRemoveImageFiles,
                                                        aTBSNode )
                      != IDE_SUCCESS );
        } // for ( i )
    } // for ( sWhichDB )


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    Membase가 들어있는 Meta Page(0번)을 Flush한다.

    Create Tablespace중에 호출된다.
    Meta Page정보가 Disk에 내려가 있어야 Restore DB가 가능하기 때문이다.
    자세한 이유는 smmTableSpace::createTableSpace의 (주5)를 참고

    [IN] aTBSNode - 0번 Page를 Flush할 Tablespace Node
    [IN] aWhichDB - 0번, 1번 Checkpoint Image중 어디에 Flush할지?
 */
IDE_RC smmManager::flushTBSMetaPage(smmTBSNode *   aTBSNode,
                                    UInt           aWhichDB)
{
    smmDatabaseFile * sFirstDBFilePtr ;

    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( smmManager::getDBFile(
                              aTBSNode,
                              aWhichDB,
                              0, // DB File No
                              SMM_GETDBFILEOP_NONE,
                              &sFirstDBFilePtr )
              != IDE_SUCCESS );

    // 이미 0번째 DB파일이 이미 Create되고 Open된 상태여야 한다.
    IDE_ASSERT( sFirstDBFilePtr->isOpen() == ID_TRUE );

    IDE_TEST( sFirstDBFilePtr->writePage( aTBSNode, SMM_MEMBASE_PAGEID )
              != IDE_SUCCESS );

    IDE_TEST( sFirstDBFilePtr->syncUntilSuccess() != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* 0번 Page를 디스크로부터 읽어서 MemBase로부터
 * 다음과 같은 정보들을 읽어온다.
 *
 * aDbFilePageCount [OUT] 하나의 데이터베이스 파일이 지니는 Page의 수
 * aChunkPageCount  [OUT] 하나의 Expand Chunk가 지니는 Page의 수
 */
IDE_RC smmManager::readMemBaseInfo(smmTBSNode * aTBSNode,
                                   UInt       * aDbFilePageCount,
                                   UInt       * aChunkPageCount )
{
    IDE_ASSERT( aTBSNode != NULL );
    IDE_ASSERT( aDbFilePageCount != NULL );
    IDE_ASSERT( aChunkPageCount != NULL );

    smmMemBase sMemBase;

    IDE_TEST( smmManager::readMemBaseFromFile( aTBSNode,
                                               &sMemBase )
              != IDE_SUCCESS );

    * aDbFilePageCount = sMemBase.mDBFilePageCount ;
    * aChunkPageCount  = sMemBase.mExpandChunkPageCnt ;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/* 0번 Page를 디스크로부터 읽어서 MemBase를 복사한다.
 *
 * [IN] aTBSNode - Membase를 읽어올 테이블스페이스의 노드
 * [OUT] aMemBase - Disk로부터 읽은 Membase가 복사될 메모리공간
 *
 */
IDE_RC smmManager::readMemBaseFromFile(smmTBSNode *   aTBSNode,
                                       smmMemBase *   aMemBase )
{
    IDE_ASSERT( aTBSNode != NULL );
    IDE_ASSERT( aMemBase != NULL );

    SChar           * sBasePage    = NULL;
    SChar           * sBasePagePtr = NULL;
    smmDatabaseFile   sDBFile0;
    SChar             sDBFileName[SM_MAX_FILE_NAME];
    SChar           * sDBFileDir;
    idBool            sFound ;
    UInt              sStage = 0;

    /* BUG-22188: DataFile에서 IO수행시 Disk Sector나 Page Size에 Align된
     *            시작주소 Buffer를 할당해야 합니다. */
    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SMM,
                                           SM_PAGE_SIZE,
                                           (void**)&sBasePagePtr,
                                           (void**)&sBasePage )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sDBFile0.initialize(
                       aTBSNode->mHeader.mID,
                       0,      // PingPongNum.
                       0)      // FileNum
            != IDE_SUCCESS );
    sStage = 2;

    sFound =  smmDatabaseFile::findDBFile( aTBSNode,
                                           0, // PING-PONG
                                           0, // DB-File-No
                                           (SChar*)sDBFileName,
                                           &sDBFileDir);
    IDE_TEST_RAISE( sFound != ID_TRUE,
                    file_exist_error );

    sDBFile0.setFileName(sDBFileName);
    sDBFile0.setDir(sDBFileDir);

    IDE_TEST(sDBFile0.open() != IDE_SUCCESS);
    sStage = 3;


    IDE_TEST(sDBFile0.readPage( aTBSNode, SMM_MEMBASE_PAGEID, (UChar*)sBasePage )
             != IDE_SUCCESS);

    idlOS::memcpy( aMemBase,
                   ( smmMemBase * )((UChar*)sBasePage + SMM_MEMBASE_OFFSET ),
                   ID_SIZEOF(*aMemBase));

    sStage = 2;
    IDE_TEST(sDBFile0.close() != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sDBFile0.destroy() != IDE_SUCCESS);

    sStage = 0;
    IDE_TEST( iduMemMgr::free( sBasePagePtr ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION(file_exist_error);
    {
        SChar sErrorFile[SM_MAX_FILE_NAME];

        idlOS::snprintf((SChar*)sErrorFile, SM_MAX_FILE_NAME, "%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT"",
                aTBSNode->mTBSAttr.mName,
                0,
                0);

        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistFile,
                                (SChar*)sErrorFile));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sStage)
    {
        case 3:
            IDE_ASSERT( sDBFile0.close() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( sDBFile0.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( sBasePagePtr ) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}



/*
 * 데이터 베이스의 Base Page (Page#0)를 가리키는 포인터들을 세팅한다.
 * 이 함수는 createdb시에도 불릴 수 있으며,
 * aBasePage에 데이터가 초기화되어 있지 않을 수도 있다.
 *
 * aBasePage     [IN] Base Page의 주소
 *
 */
IDE_RC smmManager::setupCatalogPointers( smmTBSNode * aTBSNode,
                                         UChar *      aBasePage )
{
    IDE_DASSERT( aBasePage != NULL );
    IDE_DASSERT( aTBSNode != NULL );
    IDE_ASSERT( aTBSNode->mHeader.mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC );

    smmDatabase::setDicMemBase((smmMemBase *)(aBasePage + SMM_MEMBASE_OFFSET));

    // table을 위한 지정 catalog table header
    m_catTableHeader = (void *)
                       ( aBasePage +
                         SMM_CAT_TABLE_OFFSET +
                         smLayerCallback::getSlotSize() );

    // temp table을 위한 지정 catalog table header
    m_catTempTableHeader = (void *)
                           ( aBasePage +
                             smLayerCallback::getCatTempTableOffset() +
                             smLayerCallback::getSlotSize() );


    return IDE_SUCCESS;
}

/*     
 * BUG-34530 
 * SYS_TBS_MEM_DIC테이블스페이스 메모리가 해제되더라도
 * DicMemBase포인터가 NULL로 초기화 되지 않습니다.
 *
 * Media recovery시 복구 완료된 memory tablespace해제시
 * DicMemBase와 catalog table header를 NULL로 초기화함 
 */
void smmManager::clearCatalogPointers()
{
    smmDatabase::setDicMemBase((smmMemBase *)NULL);

    // table을 위한 catalog table header
    m_catTableHeader = (void *)NULL;

    // temp table을 위한 catalog table header
    m_catTempTableHeader = (void *)NULL;
}

/*
 * 데이터 베이스의 Base Page (Page#0)를 가리키는 포인터들을 세팅한다.
 * 이 함수는 createdb시에도 불릴 수 있으며,
 * aBasePage에 데이터가 초기화되어 있지 않을 수도 있다.
 *
 * aBasePage     [IN] Base Page의 주소
 *
 */
IDE_RC smmManager::setupMemBasePointer( smmTBSNode * aTBSNode,
                                        UChar *      aBasePage )
{
    IDE_DASSERT( aBasePage != NULL );

    aTBSNode->mMemBase        = (smmMemBase *)(aBasePage + SMM_MEMBASE_OFFSET);

    return IDE_SUCCESS;
}


/*
 * 데이터 베이스의 Base Page (Page#0) 와 관련한 정보를 설정한다.
 * 이와 관련된 정보로는  MemBase와 Catalog Table정보가 있다.
 *
 * aBasePage     [IN] Base Page의 주소
 *
 */
IDE_RC smmManager::setupBasePageInfo( smmTBSNode * aTBSNode,
                                      UChar *      aBasePage )
{
    IDE_DASSERT( aBasePage != NULL );

    if( aTBSNode->mHeader.mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC )
    {
        setupCatalogPointers( aTBSNode, aBasePage );
    }
    setupMemBasePointer( aTBSNode, aBasePage );

    IDE_TEST(smmDatabase::checkVersion(aTBSNode->mMemBase) != IDE_SUCCESS);

    IDE_TEST_RAISE(smmDatabase::getAllocPersPageCount(aTBSNode->mMemBase) >
                   aTBSNode->mDBMaxPageCount,
                   page_size_overflow_error);


    // Expand Chunk 관리자 초기화
    IDE_TEST( smmExpandChunk::setChunkPageCnt(aTBSNode,
                  smmDatabase::getExpandChunkPageCnt( aTBSNode->mMemBase ) )
              != IDE_SUCCESS );

    // Expand Chunk와 관련된 Property 값 체크
    IDE_TEST( smmDatabase::checkExpandChunkProps(aTBSNode->mMemBase)
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION(page_size_overflow_error);
    {
        IDE_SET(ideSetErrorCode(
                    smERR_FATAL_Overflow_DB_Size,
                    (ULong)aTBSNode->mDBMaxPageCount,
                    (vULong)smmDatabase::getAllocPersPageCount(aTBSNode->mMemBase)));
    }
    IDE_EXCEPTION_END;


    return IDE_FAILURE;

}

/***********************************************************************
 * Description : Tablespace의 초기 페이지 갯수에 해당하는
 *               Database File들을 생성한다.
 *
 *               Create Tablespace시에만 호출된다.
 *
 *               각 Data File생성 직전에 Logging하여
 *               Create Tablespace의 Undo시 해당 File을 지우도록 한다.
 *
 *               0번 DB File은 DB File Header + Membase Page + Database Page.
 *               0번이외 DB File은 DB File Header + Database Page.
 *               따라서 생성되는 DB File의 최대 크기는
 *               0번 DB File: aDBPageCnt+2 Page
 *               0번이외 DB File: aDBPageCnt+1 Page 이다.
 *
 * [IN] aTrans - Create Tablespace를 수행하는 Transaction
 * [IN] aTBSNode - 생성중인 Tablespace의 Node
 * [IN] aTBSName - 생성될 Tablespace의 이름으로서
 *                  DB File생성시 DB File Name으로 사용한다.
 * [IN] aDBPageCnt - Tablespace의 초기 페이지 갯수.
 *                   Membase가 기록되는 Meta Page 수도 포함한다.
 **********************************************************************/
IDE_RC smmManager::createDBFile( void       * aTrans,
                                 smmTBSNode * aTBSNode,
                                 SChar      * aTBSName,
                                 vULong       aDBPageCnt,
                                 idBool       aIsNeedLogging )

{
    SChar    sCreateDBDir[SM_MAX_FILE_NAME];
    UInt     i;
    UInt     sDBFileNo = 0;
    UInt     sPageCnt;
    vULong   sDBFileSize;
    scPageID sPageCountPerFile;
    smmDatabaseFile * sDBFile ;

    IDE_DASSERT( aTBSName != NULL );
    IDE_DASSERT( aTBSName[0] != '\0' );

    IDE_TEST( smmDatabaseFile::makeDBDirForCreate( aTBSNode,
                                                   sDBFileNo,
                                                   (SChar*)sCreateDBDir )
              != IDE_SUCCESS );

    // BUG-29607 Create Tablespace에서, 동일 이름의 File들이 CP Path경로에
    //           존재하는지를 앞으로 생성할 File의 Name까지 감안해서 검사한다.
    IDE_TEST( smmDatabaseFile::chkExistDBFileByNode( aTBSNode )
              != IDE_SUCCESS );

    for ( i = 0; i < SMM_PINGPONG_COUNT; i++ )
    {
        sPageCnt = aDBPageCnt;
        sDBFileNo = 0;

        while(sPageCnt != 0)
        {
            sDBFile = (smmDatabaseFile*) aTBSNode->mDBFile[i][sDBFileNo];

            IDE_TEST(sDBFile->setFileName( sCreateDBDir,
                                           aTBSName,
                                           i,
                                           sDBFileNo) != IDE_SUCCESS);

            IDE_TEST_RAISE( sDBFile->exist() == ID_TRUE,  exist_file_error );

            // 이 파일에 기록할 수 있는 Page의 수
            // 0번 파일의 경우 membase가 기록되는 Metapage수까지 포함된 Page수
            sPageCountPerFile = smmManager::getPageCountPerFile( aTBSNode,
                                                                 sDBFileNo );


            // 마지막 파일이 아닌 경우
            if( sPageCnt > sPageCountPerFile )
            {
                sDBFileSize = sPageCountPerFile * SM_PAGE_SIZE;
                sPageCnt   -= sPageCountPerFile;
            }
            else
            {
                /* 마지막 DB파일 생성 */
                sDBFileSize = sPageCnt * SM_PAGE_SIZE;
                sPageCnt    = 0;
            }

            /* DB File Header의 크기만큼 더해준다*/
            sDBFileSize += SM_DBFILE_METAHDR_PAGE_SIZE;

            if( aIsNeedLogging == ID_TRUE )
            {
                // Create DB File에 대한 로그기록
                // - redo시 : do nothing
                // - undo시 : DB File Close & remove
                IDE_TEST( smLayerCallback::writeMemoryDBFileCreate( NULL, /* idvSQL* */
                                                                    aTrans,
                                                                    aTBSNode->mHeader.mID,
                                                                    i,
                                                                    sDBFileNo )
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothign */
            }

            IDE_TEST( sDBFile->createDbFile( aTBSNode,
                                             i,
                                             sDBFileNo,
                                             sDBFileSize) != IDE_SUCCESS);

            // create tablespace 에 의해 파일이 생성되는 경우에도
            // mLstCreatedDBFile을 계산해준다.
            if ( sDBFileNo > aTBSNode->mLstCreatedDBFile )
            {
                aTBSNode->mLstCreatedDBFile = sDBFileNo;
            }

            sDBFileNo++;
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION(exist_file_error);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AlreadyExistFile,
                                  sDBFile->getFileName() ));
    }
    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/* --------------------------------------------------------------------------
 *   SECTION : Create & Load DB
 * -------------------------------------------------------------------------*/
/*
 * 공유메모리 풀을 초기화한다.
 *
 * aTBSNode [IN] 공유메모리 풀을 초기화할 테이블 스페이스 노드
 */
IDE_RC smmManager::initializeShmMemPool( smmTBSNode *  aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( smmFixedMemoryMgr::initialize(aTBSNode) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;

}

/******************************************************************************
 * PROJ-1923 ALTIBASE HDB Disaster Recovery
 *****************************************************************************/
IDE_RC smmManager::createTBSPages4Redo( smmTBSNode   * aTBSNode,
                                        void         * aTrans,
                                        SChar        * aDBName,
                                        UInt           aDBFilePageCount,
                                        UInt           aCreatePageCount,
                                        SChar        * aDBCharSet,
                                        SChar        * aNationalCharSet )
{
    UInt        sState          = 0;
    scPageID    sTotalPageCount;
    vULong      sNewChunks;
    UInt        sChunkPageCount = smuProperty::getExpandChunkPageCount();

    IDE_DASSERT( aTBSNode   != NULL );
    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aDBName    != NULL );
    IDE_DASSERT( aDBFilePageCount   > 0 );
    IDE_DASSERT( aCreatePageCount   > 0 );

    // PROJ-1579 NCHAR
    IDE_DASSERT( aDBCharSet         != NULL );
    IDE_DASSERT( aNationalCharSet   != NULL );

    // 데이터베이스 파일의 크기가 expand chunk크기의 배수인지 확인.
    IDE_ASSERT( aDBFilePageCount % sChunkPageCount == 0 );

    (void)invalidate(aTBSNode); // db를 inconsistency 상태로 설정
    {
        ////////////////////////////////////////////////////////////////
        // (010) 0번 Meta Page (Membase 존재)를 초기화
        // 0번 Page할당
        IDE_TEST( createTBSMetaPage( aTBSNode,
                                     aTrans,
                                     aDBName,
                                     aDBFilePageCount,
                                     aDBCharSet,
                                     aNationalCharSet,
                                     ID_FALSE /* aIsNeedLogging */ )
                  != IDE_SUCCESS );

        ///////////////////////////////////////////////////////////////
        // (020) Expand Chunk관리자 초기화
        IDE_TEST( smmExpandChunk::setChunkPageCnt( aTBSNode,
                                                   sChunkPageCount )
                  != IDE_SUCCESS );

        // Expand Chunk와 관련된 Property 값 체크
        IDE_TEST( smmDatabase::checkExpandChunkProps(aTBSNode->mMemBase)
                  != IDE_SUCCESS );

        //////////////////////////////////////////////////////////////////////
        // (030) 초기 Tablespace크기만큼 Tablespace 확장(Expand Chunk 할당)

        // 생성할 데이터베이스 Page수를 토대로 새로 할당할 Expand Chunk 의 수를 결정
        sNewChunks = smmExpandChunk::getExpandChunkCount(
                                    aTBSNode,
                                    aCreatePageCount - SMM_DATABASE_META_PAGE_CNT );

        // 시스템에서 오직 하나의 Tablespace만이
        // Chunk확장을 하도록 하는 Mutex
        // => 두 개의 Tablespace가 동시에 Chunk확장하는 상황에서는
        //    모든 Tablespace의 할당한 Page 크기가 MEM_MAX_DB_SIZE보다
        //    작은 지 검사할 수 없기 때문
        IDE_TEST( smmFPLManager::lockGlobalPageCountCheckMutex()
                  != IDE_SUCCESS );
        sState = 1;

        /* BUG- 35443 Add Property for Excepting SYS_TBS_MEM_DIC size from
         * MEM_MAX_DB_SIZE */
        if( smuProperty::getSeparateDicTBSSizeEnable() == ID_TRUE )
        {
            if( aTBSNode->mHeader.mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC )
            {
                IDE_TEST( smmFPLManager::getDicTBSPageCount( &sTotalPageCount ) 
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( smmFPLManager::getTotalPageCountExceptDicTBS( 
                                                            &sTotalPageCount )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            IDE_TEST( smmFPLManager::getTotalPageCount4AllTBS( 
                                                            &sTotalPageCount ) 
                      != IDE_SUCCESS );
        }

        // Tablespace를 확장후 Database의 모든 Tablespace에 대해
        // 할당된 Page수의 총합이 MEM_MAX_DB_SIZE 프로퍼티에 허용된
        // 크기보다 더 크면 에러
        IDE_TEST_RAISE( ( sTotalPageCount + ( sNewChunks * sChunkPageCount ) ) >
                        ( smuProperty::getMaxDBSize() / SM_PAGE_SIZE ),
                        error_unable_to_create_cuz_mem_max_db_size );

        sState = 0;
        IDE_TEST( smmFPLManager::unlockGlobalPageCountCheckMutex()
                  != IDE_SUCCESS );

        IDE_ASSERT( aTBSNode->mMemBase->mAllocPersPageCount <=
                    aTBSNode->mDBMaxPageCount );

        if( aTBSNode->mHeader.mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC )
        {
            // Service 상태로 상태 전이
            smmDatabase::makeMembaseBackup();
        }

    } // 괄호 안에서는 aTBSNode가 invalid
    (void)validate(aTBSNode); // db를 consistency 상태로 되돌림.

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_unable_to_create_cuz_mem_max_db_size );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_UNABLE_TO_CREATE_CUZ_MEM_MAX_DB_SIZE,
                     aTBSNode->mHeader.mName,
                     (ULong) ( ( aCreatePageCount * SM_PAGE_SIZE) / 1024),
                     (ULong) (smuProperty::getMaxDBSize()/1024),
                     (ULong) ( (sTotalPageCount * SM_PAGE_SIZE ) / 1024 )
                ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sState )
        {
            case 1:

                IDE_ASSERT( smmFPLManager::unlockGlobalPageCountCheckMutex()
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
   Tablespace의 Meta Page를 초기화하고 Free Page들을 생성한다.

   Chunk확장에 대한 로깅을 실시한다.

   aTrans           [IN]
   aDBName          [IN] 데이터베이스 이름
   aCreatePageCount [IN] 생성할 데이터베이스가 가질 Page의 수
                         Membase가 기록되는 데이터베이스
                         Meta Page 수도 포함한다.
                         이 값은 smiMain::smiCalculateDBSize()를 통해서
                         구해진 값이어야 하고 smiGetMaxDBPageCount()보다
                         작은 값임을 확인한 후의 값이어야 한다.
   aDbFilePageCount [IN] 하나의 데이터베이스 파일이 가질 Page의 수
   aChunkPageCount  [IN] 하나의 Expand Chunk가 가질 Page의 수


   [ 알고리즘 ]
   - (010) 0번 Meta Page (Membase 존재)를 초기화
   - (020) Expand Chunk관리자 초기화
   - (030) 초기 Tablespace크기만큼 Tablespace 확장(Expand Chunk 할당)
 */
IDE_RC smmManager::createTBSPages( smmTBSNode   * aTBSNode,
                                   void         * aTrans,
                                   SChar        * aDBName,
                                   UInt           aDBFilePageCount,
                                   UInt           aCreatePageCount,
                                   SChar        * aDBCharSet,
                                   SChar        * aNationalCharSet )

{
    UInt        sState          = 0;
    scPageID    sTotalPageCount;
    vULong      sNewChunks;
    UInt        sChunkPageCount = smuProperty::getExpandChunkPageCount();

    IDE_DASSERT( aTBSNode   != NULL );
    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aDBName    != NULL );
    IDE_DASSERT( aDBFilePageCount   > 0 );
    IDE_DASSERT( aCreatePageCount   > 0 );

    // PROJ-1579 NCHAR
    IDE_DASSERT( aDBCharSet != NULL );
    IDE_DASSERT( aNationalCharSet != NULL );


    // 데이터베이스 파일의 크기가 expand chunk크기의 배수인지 확인.
    IDE_ASSERT( aDBFilePageCount % sChunkPageCount == 0 );

    (void)invalidate(aTBSNode); // db를 inconsistency 상태로 설정
    {

        ////////////////////////////////////////////////////////////////
        // (010) 0번 Meta Page (Membase 존재)를 초기화
        // 0번 Page할당
        IDE_TEST( createTBSMetaPage( aTBSNode,
                                     aTrans,
                                     aDBName,
                                     aDBFilePageCount,
                                     aDBCharSet,
                                     aNationalCharSet,
                                     ID_TRUE /* aIsNeedLogging */ )
                  != IDE_SUCCESS );


        ///////////////////////////////////////////////////////////////
        // (020) Expand Chunk관리자 초기화
        IDE_TEST( smmExpandChunk::setChunkPageCnt( aTBSNode,
                                                   sChunkPageCount )
                  != IDE_SUCCESS );

        // Expand Chunk와 관련된 Property 값 체크
        IDE_TEST( smmDatabase::checkExpandChunkProps(aTBSNode->mMemBase)
                  != IDE_SUCCESS );


        //////////////////////////////////////////////////////////////////////
        // (030) 초기 Tablespace크기만큼 Tablespace 확장(Expand Chunk 할당)

        // 생성할 데이터베이스 Page수를 토대로 새로 할당할 Expand Chunk 의 수를 결정
        sNewChunks = smmExpandChunk::getExpandChunkCount(
                                        aTBSNode,
                                        aCreatePageCount - SMM_DATABASE_META_PAGE_CNT );


        // 시스템에서 오직 하나의 Tablespace만이
        // Chunk확장을 하도록 하는 Mutex
        // => 두 개의 Tablespace가 동시에 Chunk확장하는 상황에서는
        //    모든 Tablespace의 할당한 Page 크기가 MEM_MAX_DB_SIZE보다
        //    작은 지 검사할 수 없기 때문
        IDE_TEST( smmFPLManager::lockGlobalPageCountCheckMutex()
                  != IDE_SUCCESS );
        sState = 1;

        /*
         * BUG- 35443 Add Property for Excepting SYS_TBS_MEM_DIC size from
         * MEM_MAX_DB_SIZE
         */
        if( smuProperty::getSeparateDicTBSSizeEnable() == ID_TRUE )
        {
            if( aTBSNode->mHeader.mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC )
            {
                IDE_TEST( smmFPLManager::getDicTBSPageCount( &sTotalPageCount ) 
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( smmFPLManager::getTotalPageCountExceptDicTBS( 
                                                            &sTotalPageCount )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            IDE_TEST( smmFPLManager::getTotalPageCount4AllTBS( 
                                                            &sTotalPageCount ) 
                      != IDE_SUCCESS );
        }

        // Tablespace를 확장후 Database의 모든 Tablespace에 대해
        // 할당된 Page수의 총합이 MEM_MAX_DB_SIZE 프로퍼티에 허용된
        // 크기보다 더 크면 에러
        IDE_TEST_RAISE( ( sTotalPageCount + ( sNewChunks * sChunkPageCount ) ) >
                        ( smuProperty::getMaxDBSize() / SM_PAGE_SIZE ),
                        error_unable_to_create_cuz_mem_max_db_size );

        // 트랜잭션을 NULL로 넘겨서 로깅을 하지 않도록 한다.
        // 최대 Page수를 넘어서는지 이 속에서 체크한다.
        IDE_TEST( allocNewExpandChunks( aTBSNode,
                                        aTrans, // 로깅한다.
                                        sNewChunks )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( smmFPLManager::unlockGlobalPageCountCheckMutex()
                  != IDE_SUCCESS );


        IDE_ASSERT( aTBSNode->mMemBase->mAllocPersPageCount <=
                    aTBSNode->mDBMaxPageCount );

        if( aTBSNode->mHeader.mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC )
        {
            // Service 상태로 상태 전이
            smmDatabase::makeMembaseBackup();
        }

    }
    (void)validate(aTBSNode); // db를 consistency 상태로 되돌림.



    return IDE_SUCCESS;

    IDE_EXCEPTION( error_unable_to_create_cuz_mem_max_db_size );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_UNABLE_TO_CREATE_CUZ_MEM_MAX_DB_SIZE,
                     aTBSNode->mHeader.mName,
                     (ULong) ( ( aCreatePageCount * SM_PAGE_SIZE) / 1024),
                     (ULong) (smuProperty::getMaxDBSize()/1024),
                     (ULong) ( (sTotalPageCount * SM_PAGE_SIZE ) / 1024 )
                ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sState )
        {
            case 1:

                IDE_ASSERT( smmFPLManager::unlockGlobalPageCountCheckMutex()
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
   Tablespace의 Meta Page ( 0번 Page)를 초기화한다.

   이 함수는 Create Tablespace도중 Tablespace의 Meta 정보를 지니는
   0번 Page를 초기화한다.

   aDBName          [IN] 데이터베이스 이름
   aCreatePageCount [IN] 생성할 데이터베이스가 가질 Page의 수
                         Membase가 기록되는 데이터베이스 Meta Page 수도
                         포함한다.
   aDbFilePageCount [IN] 하나의 데이터베이스 파일이 가질 Page의 수
   aChunkPageCount  [IN] 하나의 Expand Chunk가 가질 Page의 수
    IDE_DASSERT( aTBSAttr->mAttrType == SMI_TBS_ATTR );

   [ 알고리즘 ]
   (010) 0번 Page의 주소 획득
   (020) 0번 Page 의 PID및 Page Type설정
   (030) Membase및 Catalog Table의 메모리 영역을 설정한다.
   (040) Membase의 내용을 초기화한다.
   (050) MemBase 전체를 로깅한다.
   (060) 0번 Page를 DirtyPage로 등록
 */
IDE_RC smmManager::createTBSMetaPage( smmTBSNode  * aTBSNode,
                                      void        * aTrans,
                                      SChar       * aDBName,
                                      scPageID      aDBFilePageCount,
                                      SChar       * aDBCharSet,
                                      SChar       * aNationalCharSet,
                                      idBool        aIsNeedLogging /* PROJ-1923 */ )
{
    void      * sBasePage ;
    UInt        sChunkPageCount = smuProperty::getExpandChunkPageCount();

    scSpaceID   sSpaceID = aTBSNode->mHeader.mID;

    IDE_DASSERT( aTBSNode   != NULL );
    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aDBName    != NULL );
    IDE_DASSERT( aDBFilePageCount   > 0 );

    // PROJ-1579 NCHAR
    IDE_DASSERT( aDBCharSet         != NULL );
    IDE_DASSERT( aNationalCharSet   != NULL );

    IDE_TEST(fillPCHEntry( aTBSNode, SMM_MEMBASE_PAGEID ) != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////
    // (010) 0번 Page의 주소 획득
    sBasePage = mPCArray[ sSpaceID ].mPC[SMM_MEMBASE_PAGEID].mPagePtr;

    // Meta Page를 Disk로 내릴때 UMR(Uninitialized Memory Read) 발생하지 않도록 memset실시
    idlOS::memset( sBasePage, 0x0, SM_PAGE_SIZE );

    // 디버깅 모드라면 일부러 Garbage로 초기화 해서 에러를 초기에 검출
#ifdef DEBUG_SMM_FILL_GARBAGE_PAGE
    idlOS::memset( sBasePage, 0x43, SM_PAGE_SIZE );
#endif


    ///////////////////////////////////////////////////////////////////////
    // (020) 0번 Page 의 PID및 Page Type설정
    //
    // (020)-1 로깅실시
    if( aIsNeedLogging == ID_TRUE )
    {
        IDE_TEST( smLayerCallback::updateLinkAtPersPage( NULL, /* idvSQL* */
                                                         aTrans,
                                                         sSpaceID,
                                                         SMM_MEMBASE_PAGEID,// Page ID
                                                         SM_NULL_PID,   // Before PREV_PID
                                                         SM_NULL_PID,   // Before NEXT_PID
                                                         SM_NULL_PID,   // After PREV_PID
                                                         SM_NULL_PID )  // After PREV_PID
                  != IDE_SUCCESS );
    }
    else
    {
        // do nothing
    }


    // (020)-2 Page에 PID및 Page Type변경 실시
    smLayerCallback::linkPersPage( sBasePage,
                                   SMM_MEMBASE_PAGEID,
                                   SM_NULL_PID,
                                   SM_NULL_PID );


    ///////////////////////////////////////////////////////////////////////
    // (030) Membase및 Catalog Table의 메모리 영역을 설정한다.
    if( aTBSNode->mHeader.mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC )
    {
        setupCatalogPointers( aTBSNode, (UChar*)sBasePage );
    }


    IDE_TEST( setupMemBasePointer( aTBSNode,
                                   (UChar*) sBasePage )
              // version check 금지!
              != IDE_SUCCESS);



    ///////////////////////////////////////////////////////////////////////
    // (040) Membase의 내용을 초기화한다.
    IDE_TEST( smmDatabase::initializeMembase(
                                      aTBSNode,
                                      aDBName,
                                      aDBFilePageCount,
                                      sChunkPageCount,
                                      aDBCharSet,
                                      aNationalCharSet )
              != IDE_SUCCESS );


    ///////////////////////////////////////////////////////////////////////
    // (050) MemBase 전체를 로깅한다.
    if( aIsNeedLogging == ID_TRUE )
    {
        IDE_TEST( smLayerCallback::setMemBaseInfo( NULL, /* idvSQL* */
                                                   aTrans,
                                                   sSpaceID,
                                                   aTBSNode->mMemBase )
                  != IDE_SUCCESS );
    }
    else
    {
        // do nothing
    }


    ///////////////////////////////////////////////////////////////////////
    // (060) 0번 Page를 DirtyPage로 등록
    IDE_TEST( smmDirtyPageMgr::insDirtyPage( sSpaceID, SMM_MEMBASE_PAGEID )
              != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   Memory Tablespace Node의 필드를 초기화한다.

   [IN] aTBSNode - Tablespace의  Node
   [IN] aTBSAttr - Tablespace의  Attribute
 */
IDE_RC smmManager::initMemTBSNode( smmTBSNode         * aTBSNode,
                                   smiTableSpaceAttr  * aTBSAttr )
{
    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aTBSAttr != NULL );

    // (10) Memory / Disk Tablespace 공통 초기화
    IDE_TEST( sctTableSpaceMgr::initializeTBSNode( & aTBSNode->mHeader,
                                                   aTBSAttr )
              != IDE_SUCCESS );

    // (20) Tablespace Attribute복사
    idlOS::memcpy( &(aTBSNode->mTBSAttr),
                   aTBSAttr,
                   ID_SIZEOF(smiTableSpaceAttr) );

    // (30) Checkpoint Path List 초기화
    SMU_LIST_INIT_BASE( & aTBSNode->mChkptPathBase );

    // (50) Anchor Offset초기화
    aTBSNode->mAnchorOffset = SCT_UNSAVED_ATTRIBUTE_OFFSET;

    // (60) mRestoreType초기화 := 아직 Restore되지 않은상태.
    aTBSNode->mRestoreType = SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET;

    // (70) Page/File 제한을 0으로 설정 (MEDIA단계에서 초기화됨)
    aTBSNode->mDBMaxPageCount = 0;
    aTBSNode->mHighLimitFile = 0;

    // Dirty Page 관리자를 NULL로 초기화 ( PAGE단계에서 초기화됨 )
    aTBSNode->mDirtyPageMgr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*  Tablespace를 위한 매체(DB File) 관리 시스템 초기화

   [IN] aTBSNode - Tablespace의  Node
*/
IDE_RC smmManager::initMediaSystem( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    // 아직 prepare/restore가 된 상태가 아니어야 한다.
    IDE_ASSERT( aTBSNode->mRestoreType ==
                SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET );


    scPageID     sDbFilePageCount;

    // Tablespace Attribute에 기록된 DB File의 PAGE수를 토대로
    // 하나의 DB File크기가 OS의 파일 크기 제한에 걸리는지 비교한다.
    sDbFilePageCount = aTBSNode->mTBSAttr.mMemAttr.mSplitFilePageCount;

    IDE_TEST( checkOSFileSize( sDbFilePageCount * SM_PAGE_SIZE )
              != IDE_SUCCESS );

    IDE_TEST( initDBFileObjects( aTBSNode, sDbFilePageCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/********************************************************************************
 * memory, volatile 용 PCH Array 를 할당한다.
 * 
 ********************************************************************************/
IDE_RC smmManager::allocPCHArray( scSpaceID aSpaceID,
                                  UInt      aMaxPageCount )
{
    // PCH Array초기화
    IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMM,
                               aMaxPageCount,
                               ID_SIZEOF(smPCSlot),
                               (void**)&mPCArray[aSpaceID].mPC)
             != IDE_SUCCESS);

    mPCArray[ aSpaceID ].mMaxPageCount = aMaxPageCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/********************************************************************************
 * memory, volatile 용 PCH Array 를 해제한다.
 * 
 ********************************************************************************/
IDE_RC smmManager::freePCHArray( scSpaceID aSpaceID )
{
    IDE_TEST( iduMemMgr::free( mPCArray[ aSpaceID ].mPC ) != IDE_SUCCESS );
    mPCArray[ aSpaceID ].mMaxPageCount = 0;
    mPCArray[ aSpaceID ].mPC           = NULL;
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*  Tablespace를 위한 Page관리 시스템 초기화

    [IN] aTBSNode    - 초기화할 Tablespace Node

    참고 - initMediaSystem이 호출된 상태에서만 이 함수가 정상동작한다.

    참고 - initPageSystem은 prepare/restore를 위한 자료구조를 준비한다.
            page memory pool관리자는 prepare/restore시에 결정된다.
            이 함수에서는 page memory pool관리자를 초기화하지 않는다.
 */
IDE_RC smmManager::initPageSystem( smmTBSNode        * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    // 아직 prepare/restore가 된 상태가 아니어야 한다.
    IDE_ASSERT( aTBSNode->mRestoreType ==
                SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET );

    // initMediaSystem에 호출되었는지 검사.
    // initMediaSystem의 initDBFileObjects에서 이를 세팅한다.
    IDE_ASSERT( aTBSNode->mDBMaxPageCount != 0 );
    IDE_ASSERT( aTBSNode->mHighLimitFile != 0 );

    // Dirty Page관리자를 초기화
    IDE_TEST( smmDirtyPageMgr::createDPMgr( aTBSNode )
              != IDE_SUCCESS );

    // Free Page List관리자 초기화
    IDE_TEST( smmFPLManager::initialize( aTBSNode ) != IDE_SUCCESS );

    // Tablespace확장 ChunK관리자 초기화
    IDE_TEST( smmExpandChunk::initialize( aTBSNode ) != IDE_SUCCESS );

    /* smmManager_initPageSystem_calloc_PCHArray.tc */
    IDU_FIT_POINT("smmManager::initPageSystem::calloc::PCHArray");
    IDE_TEST( allocPCHArray( aTBSNode->mHeader.mID,
                             aTBSNode->mDBMaxPageCount ) != IDE_SUCCESS );

    // PCH Memory Pool초기화
    IDE_TEST(aTBSNode->mPCHMemPool.initialize(
                 IDU_MEM_SM_SMM,
                 (SChar*)"PCH_MEM_POOL",
                 1,    // 다중화 하지 않는다.
                 ID_SIZEOF(smmPCH),
                 1024, // 한번에 1024개의 PCH저장할 수 있는 크기로 메모리를 확장한다.
                 IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                 ID_TRUE,							/* UseMutex */
                 IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                 ID_FALSE,							/* ForcePooling */
                 ID_TRUE,							/* GarbageCollection */
                 ID_TRUE,                           /* HWCacheLine */
                 IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/) 
             != IDE_SUCCESS);			


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/* 여러개의 Expand Chunk를 추가하여 데이터베이스를 확장한다.
 *
 *
 *
 * aTrans            [IN] 데이터베이스를 확장하고자 하는 트랜잭션
 *                        CreateDB 혹은 logical redo중인경우 NULL로 들어온다.
 * aExpandChunkCount [IN] 확장하고자 하는 Expand Chunk의 수
 */
IDE_RC smmManager::allocNewExpandChunks( smmTBSNode   * aTBSNode,
                                         void         * aTrans,
                                         UInt           aExpandChunkCount )
{
    UInt        i;
    scPageID    sChunkFirstPID;
    scPageID    sChunkLastPID;

    // 이 함수는 Normal Processing때에만 불리우므로
    // Expand Chunk확장 후 데이터베이스 Page수가
    // 최대 Page갯수를 넘어서는지 체크한다.
    //
    // mDBMaxPageCount는 MAXSIZE에 해당하는 Page Count를 지닌다.
    // 그런데, 다음과 같이 사용자가 MAXSIZE와 INITSIZE를
    // 같게 지정하는 경우 MAXSIZE제한에 걸려서
    // Tablespace생성을 하지 못하는 문제가 발생한다.
    //
    // CREATE MEMORY TABLESPACE MEM_TBS SIZE 16M
    // AUTOEXTEND ON NEXT 8M MAXSIZE 16M;
    //
    // 위의 경우 초기크기 16M, 확장크기 16M이지만,
    // 32K(META PAGE크기) + 16M > 16M 여서 확장이 되지 않게 된다.
    // 할당된 Page수에서 METAPAGE수를 빼고 MAXSIZE체크를 해야
    // MAXSIZE와
    IDE_TEST_RAISE( aTBSNode->mMemBase->mAllocPersPageCount
                    - SMM_DATABASE_META_PAGE_CNT
                    + aExpandChunkCount * aTBSNode->mMemBase->mExpandChunkPageCnt
                    > aTBSNode->mDBMaxPageCount ,
                    max_page_error);

    // Expand Chunk를 데이터베이스에 추가하여 데이터베이스를 확장한다.
    for( i = 0 ; i < aExpandChunkCount ; i++ )
    {
        // 새로 추가할 Chunk의 첫번째 Page ID를 계산한다.
        // 지금까지 할당한 모든 Chunk의 총 Page수가 새 Chunk의 첫번째 Page ID가 된다.
        sChunkFirstPID = aTBSNode->mMemBase->mCurrentExpandChunkCnt *
                         aTBSNode->mMemBase->mExpandChunkPageCnt +
                         SMM_DATABASE_META_PAGE_CNT ;
        // 새로 추가할 Chunk의 마지막 Page ID를 계산한다.
        sChunkLastPID  = sChunkFirstPID + aTBSNode->mMemBase->mExpandChunkPageCnt - 1;

        IDE_TEST( allocNewExpandChunk( aTBSNode,
                                       aTrans,
                                       sChunkFirstPID,
                                       sChunkLastPID )
                  != IDE_SUCCESS );
    } // end of for


    return IDE_SUCCESS;

    IDE_EXCEPTION(max_page_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TooManyPage,
                                (ULong)aTBSNode->mDBMaxPageCount ));
    }
    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/* 새로 할당된 Expand Chunk안에 속하는 Page들의 PCH Entry를 할당한다.
 * Chunk안의 Free List Info Page의 Page Memory도 할당한다.
 *
 * 주의! 1. Alloc Chunk는 Logical Redo대상이므로, Physical 로깅하지 않는다.
 *          Chunk내의 Free Page들에 대해서는 Page Memory를 할당하지 않음
 *
 * aNewChunkFirstPID [IN] Chunk안의 첫번째 Page
 * aNewChunkLastPID  [IN] Chunk안의 마지막 Page
 */
IDE_RC smmManager::fillPCHEntry4AllocChunk(smmTBSNode * aTBSNode,
                                           scPageID     aNewChunkFirstPID,
                                           scPageID     aNewChunkLastPID )
{
    UInt       sFLIPageCnt = 0 ;
    scSpaceID  sSpaceID;
    scPageID   sPID = 0 ;

    sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_DASSERT( isValidPageID( sSpaceID, aNewChunkFirstPID )
                 == ID_TRUE );
    IDE_DASSERT( isValidPageID( sSpaceID, aNewChunkLastPID )
                 == ID_TRUE );

    for ( sPID = aNewChunkFirstPID ;
          sPID <= aNewChunkLastPID ;
          sPID ++ )
    {
        // Restart Recovery 시에 들어왔다면,
        // DB가 이미 로딩된 상태이므로, PCH가 이미 할당되어 있을 수 있다.

        if ( mPCArray[sSpaceID].mPC[sPID].mPCH == NULL )
        {
            // PCH Entry를 할당한다.
            IDE_TEST( allocPCHEntry( aTBSNode, sPID ) != IDE_SUCCESS );
        }

        // BUG-47487: FLI 페이지 할당 위치  
        // Free List Info Page의 Page 메모리를 할당한다.
        if ( sFLIPageCnt < smmExpandChunk::getChunkFLIPageCnt(aTBSNode) )
        {
            sFLIPageCnt ++ ;

            // Restart Recovery중에는 해당 Page메모리가 이미 할당되어
            // 있을 수 있다.
            //
            // allocAndLinkPageMemory 에서 이를 고려한다.
            // 자세한 내용은 allocPageMemory의 주석을 참고

            IDE_TEST( allocAndLinkPageMemory( aTBSNode,
                                              NULL, // 로깅하지 않음
                                              sPID,          // PID
                                              SM_NULL_PID,   // prev PID
                                              SM_NULL_PID )  // next PID
                      != IDE_SUCCESS );
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}



/* 특정 페이지 범위만큼 데이터베이스를 확장한다.
 *
 * aTrans 가 NULL로 들어오면, 이 때는 Restart Redo에서 불리운 것으로
 * Normal Processing때와 비교했을 때 다음과 같은 점에서 다르게 작동한다.
 *    1. Logging 할 필요가 없음
 *    2. 할당할 Page Count의 영역을 검사할 필요가 없음
 *       by gamestar 2001/05/24
 *
 * 모든 Free Page List에 대해 Latch가 잡히지 않은 채로 이 함수가 호출된다.
 *
 * aTrans            [IN] 데이터베이스를 확장하려는 트랜잭션
 *                        CreateDB 혹은 logical redo중인경우 NULL로 들어온다.
 * aNewChunkFirstPID [IN] 확장할 데이터베이스 Expand Chunk의 첫번째 Page ID
 * aNewChunkFirstPID [IN] 확장할 데이터베이스 Expand Chunk의 마지막 Page ID
 */
IDE_RC smmManager::allocNewExpandChunk(smmTBSNode * aTBSNode,
                                       void       * aTrans,
                                       scPageID     aNewChunkFirstPID,
                                       scPageID     aNewChunkLastPID )
{
    UInt            sStage      = 0;
    UInt            sNewDBFileCount;
    smLSN           sCreateLSN;
    smLSN           sMemCreateLSN;
    UInt            sArrFreeListCount;
    smmPageList     sArrFreeList[ SMM_MAX_FPL_COUNT ];

    IDE_DASSERT( aTBSNode != NULL );

    // BUGBUG kmkim 에러처리로 변경. 사용자가 프로퍼티 잘못 바꾸었을때
    // 바로 알려줄 수 있도록.
    // 지금은 사용자가 PAGE수 제한을 줄인후 Restart Recovery하면 여기서 죽게된다.
    IDE_ASSERT( aNewChunkFirstPID < aTBSNode->mDBMaxPageCount );
    IDE_ASSERT( aNewChunkLastPID  < aTBSNode->mDBMaxPageCount );

    // 모든 Free Page List의 Latch획득
    IDE_TEST( smmFPLManager::lockAllFPLs(aTBSNode) != IDE_SUCCESS );
    sStage = 1;


    if ( aTrans != NULL )
    {
        // RunTime 시
        IDE_TEST( smLayerCallback::allocExpandChunkAtMembase(
                      NULL, /* idvSQL* */
                      aTrans,
                      aTBSNode->mTBSAttr.mID,
                      // BEFORE : Logical Redo 하기 전의 이미지로 활용.
                      aTBSNode->mMemBase,
                      ID_UINT_MAX, /* ID_UINT_MAX로 설정하여 여러 Page List에 골고루 분배되도록함 */
                      aNewChunkFirstPID, // 새 Chunk시작 PID
                      aNewChunkLastPID,  // 새 Chunk끝 PID
                      &sCreateLSN )      // CreateLSN
                  != IDE_SUCCESS );


        // 파일이 생성되어야 하는 경우
        // Last LSN을 얻는다.
        smLayerCallback::getLstLSN( &sMemCreateLSN );

        // Create LSN 설정 현재 구하게 되는 ArrCreateLSN 은
        // 보정됨을 보장하지 않는다.
        // 파일이 생성되는 (Checkpoint)시 보정하여 파일에
        // 최종적으로 기록된다.
        SM_GET_LSN( sMemCreateLSN, sCreateLSN );
    }
    else
    {
        // Restart Recovery
        // 운영중에 파일을 생성하지 못하여 메모리 상으로 파일이
        // 늘어난 경우 redo하면서 CreateLSN Set을 구성해주어야 한다.

        /* 파일이 생성되어야 하는 경우 LstCheckLSN을 얻는다.
         *
         * RedoLSNMgr로부터 얻는 CreateLSN 은
         * Next LSN(read 해야할)이거나, invalid한 LSN일 수 있다.
         * RedoLSNMgr 를통해서는 반드시 보정되어야 할필요가 없는데
         * 이유는 다음 순서대로 Scan하면서 얻는 로그이기 때문이다.
         *
         * 이렇게 구해진 CreateLSN을 기준으로 미디어 복구에 이용한다.
         */
        // 해당 로그를 읽을 때는 아직 RedoInfo 의
        // 로그 offset을 증가시키기 전으로 해당 ALLOC_EXPAND_CHUNK
        // 로그에 해당하는 LSN을 반환한다.
        SM_GET_LSN( sMemCreateLSN ,
                    smLayerCallback::getLstCheckLogLSN() );
    }

    // Chunk가 새로 할당된 후 DB File이 몇개가 더 생겨야 하는지 계산
    IDE_TEST( calcNewDBFileCount( aTBSNode,
                                  aNewChunkFirstPID,
                                  aNewChunkLastPID,
                                  & sNewDBFileCount )
              != IDE_SUCCESS );

    if ( sNewDBFileCount > 0 )
    {
        // 파일이 생성되어야 하는 경우
        // 하나의 Chunk는 하나의 파일내에 포함되므로
        // 파일이 생성되어야 하는 경우에는 오직 1개만 생성가능하다.
        IDE_ASSERT( sNewDBFileCount == 1 );

        // PRJ-1548 User Memory TableSpace
        // 미디어복구를 위한 CreateLSN을 데이타파일의 런타임헤더에 설정
        IDE_TEST( smmTBSMediaRecovery::setCreateLSN4NewDBFiles(
                                 aTBSNode,
                                 &sMemCreateLSN ) != IDE_SUCCESS );
    }
    else
    {
        // 파일이 생성되지 않고, CHUNK만 확장된 경우
    }


    // 하나의 Expand Chunk에 속하는 Page들의 PCH Entry들을 구성한다.
    IDE_TEST( fillPCHEntry4AllocChunk( aTBSNode,
                                       aNewChunkFirstPID,
                                       aNewChunkLastPID )
              != IDE_SUCCESS );


    IDE_ASSERT( aTBSNode->mMemBase != NULL );

    sArrFreeListCount = aTBSNode->mMemBase->mFreePageListCount;

    // Logical Redo 될 것이므로 Physical Update( Next Free Page ID 세팅)
    // 에 대한 로깅을 하지 않음.
    IDE_TEST( smmFPLManager::distributeFreePages(
                  aTBSNode,
                  // Chunk내의 첫번째 Free Page
                  // Chunk의 앞부분은 Free List Info Page들이 존재하므로,
                  // Free List Info Page만큼 건너뛰어야 Free Page가 나온다.
                  aNewChunkFirstPID +
                  smmExpandChunk::getChunkFLIPageCnt(aTBSNode),
                  // Chunk내의 마지막 Free Page
                  aNewChunkLastPID,
                  ID_TRUE, // set next free page, PRJ-1548
                  sArrFreeListCount,
                  sArrFreeList  )
              != IDE_SUCCESS );
    sStage = 2;


    // 주의! smmUpdate::redo_SMMMEMBASE_ALLOC_EXPANDCHUNK 에서
    // membase 멤버들을 Logical Redo하기 전의 값으로 세팅해놓고 이 루틴으로
    // 들어와야 한다.

    // 지금까지 데이터베이스에 할당된 총 페이지수 변경
    aTBSNode->mMemBase->mAllocPersPageCount = aNewChunkLastPID + 1;
    aTBSNode->mMemBase->mCurrentExpandChunkCnt ++ ;


    // DB File 수 변경
    aTBSNode->mMemBase->mDBFileCount[0]    += sNewDBFileCount;
    aTBSNode->mMemBase->mDBFileCount[1]    += sNewDBFileCount;

    // Logical Redo할 것이므로 Phyical Update에 대해 로깅하지 않는다.
    IDE_TEST( smmFPLManager::appendPageLists2FPLs(
                                          aTBSNode,
                                          sArrFreeList,
                                          ID_TRUE, // aSetFreeListOfMembase
                                          ID_TRUE )// aSetNextFreePageOfFPL
              != IDE_SUCCESS );


    sStage = 1;
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(aTBSNode->mTBSAttr.mID,
                                            SMM_MEMBASE_PAGEID) != IDE_SUCCESS);




    sStage = 0;
    IDE_TEST( smmFPLManager::unlockAllFPLs(aTBSNode) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    switch (sStage)
    {
        case 2:
        IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(aTBSNode->mTBSAttr.mID,
                                                  SMM_MEMBASE_PAGEID) == IDE_SUCCESS);
        case 1:
        IDE_ASSERT( smmFPLManager::unlockAllFPLs(aTBSNode) == IDE_SUCCESS );

        default:
            break;
    } 
    //BUG-15508 TASK-2000 mmdb module 교육중 발견한 버그 리스트
    //그중 SBUG-1, smmManager::allocNewExpandChunk실패시 FATAL로 처리
    //이함수는 Logical redo를 하는 부분으로 undo를 하지 않기 때문에 abort가 되면 안된다.
    //차라리 죽이고, 다시 recovery하는 것이 좋다.
    IDE_SET( ideSetErrorCode(
                 smERR_FATAL_ALLOC_NEW_EXPAND_CHUNK));


    return IDE_FAILURE;
}


/*
 * 데이터베이스 확장에 따라 새로운 Expand Chunk가 할당됨에 따라
 * 새로 생겨나게 되는 DB파일의 수를 계산한다.
 *
 * aChunkFirstPID [IN] Expand Chunk의 첫번째 Page ID
 * aChunkLastPID  [IN] Expand Chunk의 마지막 Page ID
 */
IDE_RC smmManager::calcNewDBFileCount( smmTBSNode * aTBSNode,
                                       scPageID     aChunkFirstPID,
                                       scPageID     aChunkLastPID,
                                       UInt       * aNewDBFileCount)
{
    UInt    sFirstFileNo    = 0;
    UInt    sLastFileNo     = 0;

    IDE_DASSERT( isValidPageID( aTBSNode->mTBSAttr.mID, aChunkFirstPID )
                 == ID_TRUE );
    IDE_DASSERT( isValidPageID( aTBSNode->mTBSAttr.mID, aChunkLastPID )
                 == ID_TRUE );

    // 할당받은 페이지들이 속할 데이타 화일의 생성
    // membase 변경 및 로깅만 수행: 실제 화일의 생성은 checkpoint시 수행됨
    sFirstFileNo = getDbFileNo( aTBSNode, aChunkFirstPID );
    sLastFileNo  = getDbFileNo( aTBSNode, aChunkLastPID );

    // sFirstFileNo가 지금의 마지막 DB파일이라면
    if(aTBSNode->mMemBase->mDBFileCount[aTBSNode->mTBSAttr.mMemAttr.mCurrentDB]
       == ( sFirstFileNo + 1))
    {
        // 새로 생성할 DB파일은 바로 그 다음 파일
        sFirstFileNo ++;
    }

    // 새로 생성할 DB파일 수
    *aNewDBFileCount = sLastFileNo - sFirstFileNo + 1;


    return IDE_SUCCESS;
}

/*
 * Page들의 메모리를 할당한다.
 *
 * FLI Page에 Next Free Page ID로 링크된 페이지들에 대해
 * PCH안의 Page 메모리를 할당하고 Page Header의 Prev/Next포인터를 연결한다.
 *
 * Free List Info Page안의 Next Free Page ID를 기반으로
 * PCH의 Page들을 Page Header의 Prev/Next링크로 연결한다.
 *
 * Free List Info Page에 특별한 값을 설정하여 할당된 페이지임을 표시한다.
 *
 * Free Page가 새로 할당되기 전에 불리우는 루틴으로, 기존 Free Page들의
 * PCH및 Page 메모리를 할당한다.
 *
 * aTrans     [IN] Page를 할당받고자 하는 트랜잭션
 * aHeadPID   [IN] 연결하고자 하는 첫번째 Free Page
 * aTailPID   [IN] 연결하고자 하는 마지막 Free Page
 * aPageCount [OUT] 연결된 총 페이지 수
 */
IDE_RC smmManager::allocFreePageMemoryList( smmTBSNode * aTBSNode,
                                            void       * aTrans,
                                            scPageID     aHeadPID,
                                            scPageID     aTailPID,
                                            vULong     * aPageCount )
{
    scPageID   sPrevPID = SM_NULL_PID;
    scPageID   sNextPID;
    scPageID   sPID;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( isValidPageID( aTBSNode->mTBSAttr.mID, aHeadPID )
                 == ID_TRUE );
    IDE_DASSERT( isValidPageID( aTBSNode->mTBSAttr.mID, aTailPID )
                 == ID_TRUE );
    IDE_DASSERT( aPageCount != NULL );

    vULong   sProcessedPageCnt = 0;

    // BUGBUG kmkim 각 Page에 Latch를 걸어야 하는지 고민 필요.
    // 살짝쿵 생각해본 결과로는 Latch잡을 필요 없음..

    // sHeadPage부터 sTailPage사이의 모든 Page에 대해
    sPID = aHeadPID;
    while ( sPID != SM_NULL_PID )
    {
        // Next Page ID 결정
        if ( sPID == aTailPID )
        {
            // 마지막으로 link될 Page라면 다음 Page는 NULL
            sNextPID = SM_NULL_PID ;
        }
        else
        {
            // 마지막이 아니라면 다음 Page Free Page ID를 얻어온다.
            IDE_TEST( smmExpandChunk::getNextFreePage( aTBSNode,
                                                       sPID,
                                                       & sNextPID )
                      != IDE_SUCCESS );
        }

        // Free Page이더라도 PCH는 할당되어 있어야 한다.
        IDE_ASSERT( mPCArray[aTBSNode->mTBSAttr.mID].mPC[sPID].mPCH != NULL );

        // To Fix BUG-15107 Checkpoint시 Dirty Page처리도중
        //                  쓰레기 페이지 만남
        // => Page메모리 할당과 초기화를 mMutex로 묶어서
        //    Checkpoint시에 할당만 되고 초기화되지 않은 메모리를
        //    보는 일이 없도록 한다.
        //
        // 페이지 메모리를 할당하고 초기화
        IDE_TEST( allocAndLinkPageMemory( aTBSNode,
                                          aTrans, // 로깅실시
                                          sPID,
                                          sPrevPID,
                                          sNextPID ) != IDE_SUCCESS );

        // 테이블에 할당되었다는 의미로
        // Page의 Next Free Page로 특별한 값을 기록해둔다.
        // 서버 기동시 Page가 테이블에 할당된 Page인지,
        // Free Page인지 여부를 결정하기 위해 사용된다.
        IDE_TEST( smmExpandChunk::logAndSetNextFreePage(
                      aTBSNode,
                      aTrans, // 로깅실시
                      sPID,
                      SMM_FLI_ALLOCATED_PID )
                  != IDE_SUCCESS );

        sProcessedPageCnt ++ ;


        sPrevPID = sPID ;

        // sPID 가 aTailPID일 때,
        // 여기에서 sPID가 SM_NULL_PID 로 설정되어 loop 종료
        sPID = sNextPID ;
    }


    * aPageCount = sProcessedPageCnt;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/* PCH의 Page속의 Page Header의 Prev/Next포인터를 기반으로
 * FLI Page에 Next Free Page ID를 설정한다.
 *
 * 테이블에 할당되었던 Page가 Free Page로 반납되기 전에 전에
 * 불리우는 루틴이다.
 *
 * aTrans     [IN] 연결하고자 하는 트랜잭션
 * aHeadPage  [IN] 연결하고자 하는 첫번째 Free Page
 * aTailPage  [IN] 연결하고자 하는 마지막 Free Page
 * aPageCount [OUT] 연결된 총 페이지 수
 */
IDE_RC smmManager::linkFreePageList( smmTBSNode * aTBSNode,
                                     void       * aTrans,
                                     void       * aHeadPage,
                                     void       * aTailPage,
                                     vULong     * aPageCount )
{
    scPageID    sPID, sTailPID, sNextPID;
    vULong      sProcessedPageCnt = 0;
    void      * sPagePtr;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aHeadPage != NULL );
    IDE_DASSERT( aTailPage != NULL );
    IDE_DASSERT( aPageCount != NULL );

    sPID = smLayerCallback::getPersPageID( aHeadPage );
    sTailPID = smLayerCallback::getPersPageID( aTailPage );
    // sHeadPage부터 sTailPage사이의 모든 Page에 대해

    do
    {
        if ( sPID == sTailPID ) // 마지막 페이지인 경우
        {
            sNextPID = SM_NULL_PID ;
        }
        else  // 마지막 페이지가 아닌 경우
        {
            // Free List Info Page에 Next Free Page ID를 기록한다.
            IDE_ASSERT( smmManager::getPersPagePtr( aTBSNode->mTBSAttr.mID, 
                                                    sPID,
                                                    &sPagePtr )
                        == IDE_SUCCESS );
            sNextPID = smLayerCallback::getNextPersPageID( sPagePtr );
        }

        // Free List Info Page에 Next Free Page ID 세팅
        IDE_TEST( smmExpandChunk::logAndSetNextFreePage( aTBSNode,
                                                         aTrans,  // 로깅실시
                                                         sPID,
                                                         sNextPID )
                  != IDE_SUCCESS );

        sProcessedPageCnt ++ ;

        sPID = sNextPID ;
    } while ( sPID != SM_NULL_PID );

    * aPageCount = sProcessedPageCnt;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}


/* PCH의 Page속의 Page Header의 Prev/Next포인터를 기반으로
 * PCH 의 Page 메모리를 반납한다.
 *
 * 테이블에 할당되었던 Page가 Free Page로 반납된 후에
 * 불리우는 루틴으로, Page들의 PCH및 Page 메모리를 해제한다.
 *
 * aHeadPage  [IN] 연결하고자 하는 첫번째 Free Page
 * aTailPage  [IN] 연결하고자 하는 마지막 Free Page
 * aPageCount [OUT] 연결된 총 페이지 수
 */
IDE_RC smmManager::freeFreePageMemoryList( smmTBSNode * aTBSNode,
                                           void       * aHeadPage,
                                           void       * aTailPage,
                                           vULong     * aPageCount )
{
    scPageID    sPID, sTailPID, sNextPID;
    vULong      sProcessedPageCnt = 0;
    void      * sPagePtr;

    IDE_DASSERT( aHeadPage != NULL );
    IDE_DASSERT( aTailPage != NULL );
    IDE_DASSERT( aPageCount != NULL );

    sPID = smLayerCallback::getPersPageID( aHeadPage );
    sTailPID = smLayerCallback::getPersPageID( aTailPage );
    // sHeadPage부터 sTailPage사이의 모든 Page에 대해

    do
    {
        if ( sPID == sTailPID ) // 마지막 페이지인 경우
        {
            sNextPID = SM_NULL_PID ;
        }
        else  // 마지막 페이지가 아닌 경우
        {
            // Free List Info Page에 Next Free Page ID를 기록한다.
            IDE_ASSERT( smmManager::getPersPagePtr( aTBSNode->mTBSAttr.mID, 
                                                    sPID,
                                                    &sPagePtr )
                        == IDE_SUCCESS );
            sNextPID = smLayerCallback::getNextPersPageID( sPagePtr );
        }

        IDE_TEST( freePageMemory( aTBSNode, sPID ) != IDE_SUCCESS );

        sProcessedPageCnt ++ ;

        sPID = sNextPID ;
    } while ( sPID != SM_NULL_PID );

    * aPageCount = sProcessedPageCnt;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}




/** DB로부터 Page를 여러개 할당받는다.
 *
 * 여러개의 Page를 동시에 할당받으면 DB Page할당 횟수를 줄일 수 있으며,
 * 이를 통해 DB Free Page List 로의 동시성을 향상시킬 수 있다.
 *
 * 여러 Page들을 서로 연결하기 위해 aHeadPage부터 aTailPage까지
 * Page Header의 Prev/Next포인터로 연결해준다.
 *
 * 주의 ! 이 함수에서 NTA로그 찍은 후 실패하면, Logical Undo되어
 * freePersPageList가 호출된다. 그러므로 aHeadPage부터 aTailPage까지
 * Page 메모리안의 Next 링크가 깨져서는 안된다.
 *
 * aTrans     [IN] 페이지를 할당받을 트랜잭션 객체
 * aPageCount [IN] 할당받을 페이지의 수
 * aHeadPage  [OUT] 할당받은 페이지 중 첫번째 페이지
 * aTailPage  [OUT] 할당받은 페이지 중 마지막 페이지
 */
IDE_RC smmManager::allocatePersPageList (void      *  aTrans,
                                         scSpaceID    aSpaceID,
                                         UInt         aPageCount,
                                         void     **  aHeadPage,
                                         void     **  aTailPage,
                                         UInt      *  aAllocPageCnt )
{

    scPageID  sHeadPID = SM_NULL_PID;
    scPageID  sTailPID = SM_NULL_PID;
    vULong    sLinkedPageCount;
    smLSN     sNTALSN ;
    UInt      sPageListID;
    UInt      sStage = 0;
    UInt      sState = 0;
    smmTBSNode * sTBSNode;
    UInt      sPageCount = 0;
    UInt      sTotalPageCount = 0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPageCount != 0 );
    IDE_DASSERT( aHeadPage != NULL );
    IDE_DASSERT( aTailPage != NULL );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sTBSNode)
              != IDE_SUCCESS );

    sNTALSN = smLayerCallback::getLstUndoNxtLSN( aTrans );

    // 여러개의 Free Page List중 하나를 선택한다.
    smLayerCallback::allocRSGroupID( aTrans, &sPageListID );

    if( aPageCount != 1 )
    {
        /* BUG-46861 TABLE_ALLOC_PAGE_COUNT 프로퍼티를 사용해 다수의 페이지를 할당할 경우 다수의 페이지 할당으로
           할당된 페이지가 MEM_MAX_DB_SIZE를 넘어서지 않도록 주의해야 한다.
           또한, TBS lock을 잡지 않고 현재 사용중인 DB의 크기를 체크하기 때문에
           현재 사용중인 공간+새로 할당한 공간이 최대 사용 가능 공간에 도달하지 않아도
           그에 근접할 경우(1개 Chunk이하의 공간이 차이날 경우)에는 기존처럼 1개씩 할당한다. */

        IDE_TEST( smmFPLManager::lockGlobalPageCountCheckMutex() != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( smmFPLManager::getTotalPageCount4AllTBS( & sTotalPageCount ) != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( smmFPLManager::unlockGlobalPageCountCheckMutex() != IDE_SUCCESS );

        if( ( smiGetStartupPhase() == SMI_STARTUP_SERVICE ) &&
                ( ( sTotalPageCount + aPageCount ) < 
                  ( ( smuProperty::getMaxDBSize() / SM_PAGE_SIZE ) - sTBSNode->mMemBase->mExpandChunkPageCnt ) ) )
        {
            sPageCount = aPageCount;
        }
        else
        {
            /* 할당한 공간+할당할 공간이 MEM_MAX_DB_SIZE에 근접하거나 서버 시작 중에는 기존처럼 하나만 할당한다. */
            sPageCount = 1;
        }
    }
    else
    {
        sPageCount = 1;
    }

    // sPageListID에 해당하는 Free Page List에 최소한 aPageCount개의
    // Free Page가 존재함을 보장하면서 latch를 획득한다.
    //
    // aPageCount만큼 Free Page List에 있음을 보장하기 위해서
    //
    // 1. Free Page List간에 Free Page들을 이동시킬 수 있다. => Physical 로깅
    // 2. Expand Chunk를 할당할 수 있다.
    //     => Chunk할당을 Logical 로깅.
    //       -> Recovery시 smmManager::allocNewExpandChunk호출하여 Logical Redo
    IDE_TEST( smmFPLManager::lockListAndPreparePages( sTBSNode,
                                                      aTrans,
                                                      (smmFPLNo)sPageListID,
                                                      sPageCount )
              != IDE_SUCCESS );
    sStage = 1;

    // 트랜잭션이 사용하는 Free Page List에서 Free Page들을 떼어낸다.
    // DB Free Page List에 대한 로깅이 여기에서 이루어진다.
    IDE_TEST( smmFPLManager::removeFreePagesFromList( sTBSNode,
                                                      aTrans,
                                                      (smmFPLNo)sPageListID,
                                                      sPageCount,
                                                      & sHeadPID,
                                                      & sTailPID )
              != IDE_SUCCESS );

    // Head부터 Tail까지 모든 Page에 대해
    // Page Header의 Prev/Next 링크를 서로 연결시킨다.
    IDE_TEST( allocFreePageMemoryList ( sTBSNode,
                                        aTrans,
                                        sHeadPID,
                                        sTailPID,
                                        & sLinkedPageCount )
              != IDE_SUCCESS );

    IDE_ASSERT( sLinkedPageCount == sPageCount );

    IDE_ASSERT( smmManager::getPersPagePtr( sTBSNode->mTBSAttr.mID, 
                                            sHeadPID,
                                            aHeadPage )
                == IDE_SUCCESS );
    IDE_ASSERT( smmManager::getPersPagePtr( sTBSNode->mTBSAttr.mID, 
                                            sTailPID,
                                            aTailPage )
                == IDE_SUCCESS );

    // write NTA
    IDE_TEST( smLayerCallback::writeAllocPersListNTALogRec( NULL, /* idvSQL* */
                                                            aTrans,
                                                            & sNTALSN,
                                                            sTBSNode->mTBSAttr.mID,
                                                            sHeadPID,
                                                            sTailPID )
              != IDE_SUCCESS );



    // 페이지를 할당받은 Free Page List의 Latch를 풀어준다.
    sStage = 0;
    IDE_TEST( smmFPLManager::unlockFreePageList( sTBSNode,
                                                 (smmFPLNo)sPageListID )
              != IDE_SUCCESS );

    *aAllocPageCnt = sPageCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sStage )
    {
        case 1:
            // Fatal 오류가 아니라면, NTA 록백
            IDE_ASSERT( smLayerCallback::undoTrans( NULL, /* idvSQL* */
                                                    aTrans,
                                                    &sNTALSN )
                        == IDE_SUCCESS );

            IDE_ASSERT( smmFPLManager::unlockFreePageList(
                            sTBSNode,
                            (smmFPLNo)sPageListID ) == IDE_SUCCESS );
            break;
        default:
            break;
    }

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smmFPLManager::unlockGlobalPageCountCheckMutex()
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    IDE_POP();


    return IDE_FAILURE;
}


/*
 * 여러개의 Page를 한꺼번에 데이터베이스로 반납한다.
 *
 * aHeadPage부터 aTailPage까지
 * Page Header의 Prev/Next포인터로 연결되어 있어야 한다.
 *
 * 현재 Free Page수가 가장 작은 Free Page List 에 Page를 Free 한다.
 *
 * aTrans    [IN] Page를 반납하려는 트랜잭션
 * aHeadPage [IN] 반납할 첫번째 Page
 * aHeadPage [IN] 반납할 마지막 Page
 * aNTALSN   [IN] NTA로그를 찍을 때 기록할 NTA시작 LSN
 *
 */
IDE_RC smmManager::freePersPageList (void       * aTrans,
                                     scSpaceID    aSpaceID,
                                     void       * aHeadPage,
                                     void       * aTailPage,
                                     smLSN      * aNTALSN )
{
    UInt            sPageListID;
    vULong          sLinkedPageCount;
    vULong          sFreePageCount;
    scPageID        sHeadPID;
    scPageID        sTailPID;
    UInt            sStage      = 0;
    idBool          sIsInNTA    = ID_TRUE;
    smmTBSNode    * sTBSNode;

    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aHeadPage  != NULL );
    IDE_DASSERT( aTailPage  != NULL );
    IDE_DASSERT( aNTALSN    != NULL );

    IDE_TEST(sctTableSpaceMgr::findSpaceNodeBySpaceID(aSpaceID,
                                                      (void**)&sTBSNode)
             != IDE_SUCCESS);

    sHeadPID = smLayerCallback::getPersPageID( aHeadPage );
    sTailPID = smLayerCallback::getPersPageID( aTailPage );

    // Free List Info Page안에 Free Page들의 Link를 기록한다.
    IDE_TEST( linkFreePageList( sTBSNode,
                                aTrans,
                                aHeadPage,
                                aTailPage,
                                & sLinkedPageCount )
              != IDE_SUCCESS );

    // Free Page를 반납한다.
    // 이렇게 해도 allocFreePage에서 Page가장 많은 Free Page List에서
    // 절반을 떼어오는 루틴때문에 Free Page List간의 밸런싱이 된다.

    // 여러개의 Free Page List중 하나를 선택한다.
    smLayerCallback::allocRSGroupID( aTrans, &sPageListID );

    // Page를 반납할 Free Page List에 Latch획득
    IDE_TEST( smmFPLManager::lockFreePageList(sTBSNode, (smmFPLNo)sPageListID)
              != IDE_SUCCESS );
    sStage = 1;

    // Free Page List 에 Page를 반납한다.
    // DB Free Page List에 대한 로깅이 발생한다.
    IDE_TEST( smmFPLManager::appendFreePagesToList(
                                   sTBSNode,
                                   aTrans,
                                   sPageListID,
                                   sLinkedPageCount,
                                   sHeadPID,
                                   sTailPID,
                                   ID_TRUE, // aSetFreeListOfMembase
                                   ID_TRUE )// aSetNextFreePageOfFPL
              != IDE_SUCCESS );

    // write NTA
    IDE_TEST( smLayerCallback::writeNullNTALogRec( NULL, /* idvSQL* */
                                                   aTrans,
                                                   aNTALSN )
              != IDE_SUCCESS );


    sIsInNTA = ID_FALSE;

    // Page Memory Free를 NTA 종료되고 처리하도록 한다
    //
    // 왜냐하면, Page Memory Free해버리고 나서 NTA실패하면
    // Page메모리 다시 할당받아 DB파일에서 로딩해야 하는데,
    // 그 작업이 만만치 않기 때문이다.
    if ( smLayerCallback::isRestartRecoveryPhase() == ID_FALSE )
    {
        // Restart Recovery중이 아닐때에만 페이지 메모리를 free한다.
        // Restart Recovery중에는 free된 페이지 메모리에 대한
        // Redo/Undo를 할 수 있기 때문이다.

        IDE_TEST( freeFreePageMemoryList( sTBSNode,
                                          aHeadPage,
                                          aTailPage,
                                          & sFreePageCount )
                  != IDE_SUCCESS );

        IDE_ASSERT( sFreePageCount == sLinkedPageCount );
    }

    // 주의! Page 반납 연산 Logging을 하고
    // Page Free를 하기 때문에 Flush를 할 필요가 없다.

    sStage = 0;
    // Free Page List에서 Latch푼다
    IDE_TEST( smmFPLManager::unlockFreePageList( sTBSNode,
                                                 (smmFPLNo)sPageListID )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsInNTA == ID_TRUE )
    {
        sIsInNTA = ID_FALSE;
        // NTA안에서 에러가 발생했다면 이유 불문하고, NTA 롤백
        IDE_ASSERT( smLayerCallback::undoTrans( NULL, /* idvSQL* */
                                                aTrans,
                                                aNTALSN )
                    == IDE_SUCCESS );
    }

    switch( sStage )
    {
        case 1:

            IDE_ASSERT( smmFPLManager::unlockFreePageList(
                            sTBSNode,
                            (smmFPLNo)sPageListID )
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }
    sStage = 0;

    IDE_POP();


    return IDE_FAILURE;
}


/* --------------------------------------------------------------------------
 *  SECTION : Latch Control
 * -------------------------------------------------------------------------*/

/*
 * 특정 Page에 S래치를 획득한다. ( 현재는 X래치로 구현되어 있다 )
 */
IDE_RC
smmManager::holdPageSLatch(scSpaceID aSpaceID,
                           scPageID  aPageID )
{
    smmPCH            * sPCH ;
    smpPersPageHeader * sMemPagePtr;

    IDE_DASSERT( isValidPageID( aSpaceID, aPageID ) == ID_TRUE );

    IDE_TEST( smmManager::getPersPagePtr( aSpaceID,
                                          aPageID,
                                          (void**) &sMemPagePtr )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( SMP_GET_PERS_PAGE_INCONSISTENCY( sMemPagePtr ) 
                    == SMP_PAGEINCONSISTENCY_TRUE,
                    ERR_INCONSISTENT_PAGE );

    sPCH = getPCH( aSpaceID, aPageID );

    IDE_DASSERT( sPCH != NULL );

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_TEST( sPCH->mPageMemLatch.lockRead( NULL,/* idvSQL* */
                                  NULL ) /* sWeArgs*/
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INCONSISTENT_PAGE )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_PAGE,
                                  aSpaceID,
                                  aPageID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * 특정 Page에 X래치를 획득한다.
 */
IDE_RC
smmManager::holdPageXLatch(scSpaceID aSpaceID,
                           scPageID  aPageID)
{
    smmPCH * sPCH;

    IDE_DASSERT( isValidPageID( aSpaceID, aPageID ) == ID_TRUE );

    sPCH = getPCH( aSpaceID, aPageID );

    IDE_DASSERT( sPCH != NULL );

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_TEST( sPCH->mPageMemLatch.lockWrite( NULL,/* idvSQL* */
                                             NULL ) /* sWeArgs*/
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * 특정 Page에서 래치를 풀어준다.
 */
IDE_RC
smmManager::releasePageLatch(scSpaceID aSpaceID,
                             scPageID  aPageID)
{
    smmPCH * sPCH;

    IDE_DASSERT( isValidPageID( aSpaceID, aPageID ) == ID_TRUE );

    sPCH = getPCH( aSpaceID, aPageID );

    IDE_DASSERT( sPCH );

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_TEST( sPCH->mPageMemLatch.unlock( ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    일반 메모리 Page Pool을 초기화한다.

    [IN] aTBSNode - Page Pool을 초기화할 테이블 스페이스
*/
IDE_RC smmManager::initializeDynMemPool(smmTBSNode * aTBSNode)
{
    // BUG-47487: FLI 페이지 MemPool 초기화
    IDE_TEST(aTBSNode->mFLIMemPagePool.initialize(
                IDU_MEM_SM_SMM,
                (SChar*)"TEMP_MEMORY_POOL",
                1, /* MemList Count */
                SM_PAGE_SIZE,
                smuProperty::getTempPageChunkCount(),
                0, /* Cache Size */
                iduProperty::getDirectIOPageSize() /* Align Size */)
        != IDE_SUCCESS);

    /* BUG-16885: Stroage_Memory_Manager영역의 메모리가 비정상적으로
     * 커지는 경우가 발생
     * Free Page List가 여러개이다 보니 다른 리스트에 100M의 Free Page
     * 가 있더라도 자신이 접근하는 리스트에 Memory가 없으면 새로 메모리
     * 를 할당하여 비정상적으로 메모리가 커짐. 그런데 mDynamicMemPagePool
     * 에 병렬로 할당하는 경우는 없기때문에 리스트 갯수를 1로 수정함.
     *
     * 아래 mIndexMemPool도 마찬가지 이유로 리스트 갯수를 1로 수정함.
     */
    IDE_TEST(aTBSNode->mDynamicMemPagePool.initialize(
                 IDU_MEM_SM_SMM,
                 (SChar*)"TEMP_MEMORY_POOL",
                 1, /* MemList Count */
                 SM_PAGE_SIZE,
                 smuProperty::getTempPageChunkCount(),
                 0, /* Cache Size */
                 iduProperty::getDirectIOPageSize() /* Align Size */)
             != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Tablespace에서 사용할 Page 메모리 풀을 초기화한다.

    공유메모리 Key값 존재 여부에 따라
    공유메모리 혹은 일반메모리를 사용하도록 Page Pool을 초기화 한다.
 */
IDE_RC smmManager::initializePagePool( smmTBSNode * aTBSNode )
{

    if ( smuProperty::getShmDBKey() != 0) // 공유메모리 사용시
    {
        // 공유메모리 Page Pool 초기화
        IDE_TEST( initializeShmMemPool( aTBSNode ) != IDE_SUCCESS );

        // 첫번째 공유 메모리 Chunk 생성
        // 크기 : SHM_PAGE_COUNT_PER_KEY 프로퍼티에 지정된 Page수만큼
        // TBSNode에 맨 첫번째 공유메모리 Key가 설정된다.
        IDE_TEST(smmFixedMemoryMgr::createFirstChunk(
                     aTBSNode,
                     smuProperty::getShmPageCountPerKey() )
                 != IDE_SUCCESS);

        aTBSNode->mRestoreType = SMM_DB_RESTORE_TYPE_SHM_CREATE;
    }
    else // 일반메모리 사용시
    {
        IDE_TEST( initializeDynMemPool( aTBSNode ) != IDE_SUCCESS );

        aTBSNode->mRestoreType = SMM_DB_RESTORE_TYPE_DYNAMIC;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Tablespace에 할당된 Page Pool을 파괴한다.

    [ 알고리즘 ]

    if ( 공유메모리 사용 )
       if ( Drop/Offline된 Tablespace )
          (010) 공유메모리 remove ( 시스템에서 제거 )
       else // Shutdown하는 경우
          (020) 공유메모리 detach
       fi
       (030) 공유메모리 Page 관리자 파괴
    else // 일반 메모리 사용
       (040) 일반메모리 Page 관리자 파괴
    fi

    aTBSNode [IN] Page Pool을 파괴할 테이블 스페이스
 */
IDE_RC smmManager::destroyPagePool( smmTBSNode * aTBSNode )
{
    switch( aTBSNode->mRestoreType )
    {
        case SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET :
            // PAGE단계까지 초기화 하였으나,
            // prepare/restore되지 않은 tablespace.
            //
            // 아직 page memory pool이 초기화되지 않은 상태.
            // 아무것도 수행하지 않는다.
            break;

        case SMM_DB_RESTORE_TYPE_SHM_CREATE :
        case SMM_DB_RESTORE_TYPE_SHM_ATTACH :
            // Drop/Offline의 Pending으로 호출된 경우
            // 호출된 경우 공유메모리 자체를 제거한다
            if ( sctTableSpaceMgr::hasState(
                     & aTBSNode->mHeader,
                     SCT_SS_FREE_SHM_PAGE_ON_DESTROY )
                 == ID_TRUE )
            {
                /////////////////////////////////////////////////
                // (010) 공유메모리 remove ( 시스템에서 제거 )
                //
                // 공유 메모리 모드인 경우 공유메모리영역을 제거하고
                // 공유메모리 Key를 재사용 Key로 등록
                IDE_TEST( smmFixedMemoryMgr::remove( aTBSNode )
                          != IDE_SUCCESS );
            }
            else
            {
                /////////////////////////////////////////////////
                // (020) 공유메모리 detach
                //
                // 공유메모리 Detach실시
                IDE_TEST( smmFixedMemoryMgr::detach( aTBSNode )
                          != IDE_SUCCESS );
            }

            /////////////////////////////////////////////////////
            // (030) 공유메모리 Page 관리자 파괴
            //
            // TBSNode에서 공유메모리 관련 정보 해제
            IDE_TEST( smmFixedMemoryMgr::destroy( aTBSNode ) != IDE_SUCCESS );

            break;

        case SMM_DB_RESTORE_TYPE_DYNAMIC :
            ////////////////////////////////////////////////////
            // (040) 일반메모리 Page 관리자 파괴
            IDE_TEST(aTBSNode->mDynamicMemPagePool.destroy() != IDE_SUCCESS);
            // BUG-47487: FLI 페이지 memPool 파괴
            IDE_TEST(aTBSNode->mFLIMemPagePool.destroy() != IDE_SUCCESS);

            break;
        default :
            IDE_ASSERT(0);
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * smmManager::prepareDB를 위한 Action함수
 */
IDE_RC smmManager::prepareTBSAction( idvSQL            * /*aStatistics*/,
                                     sctTableSpaceNode * aTBSNode,
                                     void              * aActionArg )
{
    idBool   sDoIt;

    IDE_DASSERT( aTBSNode != NULL );

    // CONTROL단계에서 DROPPED인 TBS는 loganchor로부터
    // 읽어들이지 않기 때문에
    // PREPARE/RESTORE중에 DROPPED상태인 TBS는 있을 수 없다.
    IDE_ASSERT( ( aTBSNode->mState & SMI_TBS_DROPPED ) != SMI_TBS_DROPPED );

    // Memory Tablespace중 DISCARD,OFFLINE Tablespace를 제외하고
    // PREPARE/RESTORE를 수행한다.
    if(( sctTableSpaceMgr::isMemTableSpace(aTBSNode->mID) == ID_TRUE ) &&
       ( sctTableSpaceMgr::hasState(aTBSNode->mID, SCT_SS_SKIP_PREPARE )
         == ID_FALSE) )
    {
        IDE_DASSERT( ID_SIZEOF(void*) >= ID_SIZEOF(smmPrepareOption) );

        // 미디어복구를 지원하기 위해 Startup Control 단계에서
        // Memory TableSpace 노드 초기화는 Startup Control단계에서
        // LogAnchor 초기화 과정에서 처리된다.

        switch ( (smmPrepareOption)(vULong)aActionArg )
        {
            case SMM_PREPARE_OP_DBIMAGE_NEED_MEDIA_RECOVERY:
            {
                IDE_TEST_RAISE ( smuProperty::getShmDBKey() != 0,
                    error_media_recovery_is_not_support_shm );

                if ( (isMediaFailureTBS( (smmTBSNode*)aTBSNode ) == ID_TRUE) ||
                     (aTBSNode->mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC) )
                {
                    // 미디어복구 플래그가 ID_TRUE로 설정된 데이타파일이
                    // 있을 경우 Prepare를 진행한다.
                    // 또한, 0번 TBS의 경우 DicMemBase 로딩을 위해
                    // 무조건 한다.
                    sDoIt = ID_TRUE;
                }
                else
                {
                   // prepare 하지 않는다.
                   sDoIt = ID_FALSE;
                }
                break;
            }
            default:
            {
                 // 서버구동시
                sDoIt = ID_TRUE;
                break;
            }
        }

        if ( sDoIt == ID_TRUE )
        {
            IDE_TEST( prepareTBS( (smmTBSNode*) aTBSNode,
                        (smmPrepareOption)(vULong) aActionArg )
                    != IDE_SUCCESS );
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_media_recovery_is_not_support_shm );
    {
        IDE_SET( ideSetErrorCode(
                 smERR_ABORT_MEDIA_RECOVERY_IS_NOT_SUPPORT_SHARED_MEMORY) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * 데이터베이스 restore(Disk -> 메모리로 로드 )를 위해 준비한다.
 *
 * aOp          [IN] Prepare 옵션/정보
 */

IDE_RC smmManager::prepareDB ( smmPrepareOption aOp )
{

    IDE_DASSERT( ID_SIZEOF(void*) >= ID_SIZEOF(smmPrepareOption) );

    // 서버 구동시 또는 미디어 복구시
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  prepareTBSAction,
                                                  (void*) aOp,
                                                  SCT_ACT_MODE_NONE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
   Tablespace restore(Disk=>Memory)를 위한 준비작업을 수행한다.

   aTBSNode     [IN] 준비작업을 수행할 Tablespace
   aTempMemBase [IN] 0번 Page를 읽어들일 Buffer
                     이 함수가 종료된 후에도 이 Buffer안의 Membase를
                     포인팅 해야 하기 때문에 이 함수안의 stack변수로
                     잡을 수 없어서 인자로 받는다.
   aOp          [IN] prepare option


   배경지식 -----------------------------------------------------------------
   - Restore 방식의 종류
     - 일반메모리 사용 ( SMM_DB_RESTORE_TYPE_DYNAMIC )
       - 일반 메모리에 Disk=>Memory로 Page로딩하는 I/O를 수행
     - 공유메모리 사용
       - 공유메모리 Attach ( SMM_DB_RESTORE_TYPE_SHM_ATTACH )
         - 현재 공유메모리에 Disk Image가 올라와 있는 상황
         - 공유메모리 Attach만 수행하며,
           Disk=> Memory로의 Page를 로드하는 I/O가 필요없다
       - 공유메모리 Create ( SMM_DB_RESTORE_TYPE_SHM_CREATE )
         - 공유메모리를 새로 생성하고 Disk=>Memory로의
           Page를 로드하는 I/O를 수행

   PROJ-1548 User Memory Tablespace ------------------------------------------

   [ 공유메모리 관련 Design ]

   기존에는 Tablespace가 한개였기 때문에 SHM_DB_KEY라는 프로퍼티에
   기술된 공유메모리 키로부터 시작하여 공유메모리 영역들을
   알티베이스에 Attach하였다.

   Memory Tablespace가 여러개가 됨에 따라, 각각의 Tablespace마다
   이와 같은 시작 공유메모리 Key가 필요하다.

   Tablespace의 시작 공유메모리 Key는 서버를 내렸다가 올렸을 때
   해당 Tablespace 첫번째 공유메모리 영역을 Attach하기 위해
   꼭 필요하다.

   그래서 각 Tablespace 별로 시작 공유메모리 Key를 Durable하게
   저장할 필요가 있다.

   Log Anchor의 Tablespace정보에 각 Tablespace별로 시작 공유메모리 Key를
   저장하도록 한다.

   제약조건
     - 일부 Tablespace만을 공유메모리 모드로 띄울 수 없다
       ( 복잡도를 낮추기 위한 제약이며 추후 추가구현 가능 )
       - 모든 Tablespace를 공유 메모리로 띄우거나,
         모든 Tablespace를 일반 메모리로 띄울 수 있다.
     - 일부 Tablespace만을 Attach하고 나머지를 Create할 수 없다
       - 모든 Tablespace를 공유메모리로부터 Attach하거나,
         모든 Tablespace의 공유메모리를 새로 생성해야 한다.

   자료구조
     - LogAnchor
       - TBSNode
         - TBSAttr
           - ShmKey : 테이블 스페이스의 시작 공유메모리 Key
                      Log Anchor에 Durable하게 저장된다.

   동작
     - 공유메모리 사용을 위해서는 사용자가 SHM_DB_KEY프로퍼티를
       원하는 공유메모리 Key값으로 설정한다.

     - 각 Tablespace별 공유메모리 Key는 시스템이 자동으로 결정한다

   알고리즘 ( restore방식을 결정 )
     - 1. SHM_DB_KEY == 0 이면 일반 메모리를 사용한다.
       - LogAnchor의 ShmKey := 0 설정 ( 추후 모든 TBS에 대해 일괄 Flush )
     - 2. SHM_DB_KEY != 0 이면
        - 2.1 TBSNode.ShmKey 에 해당하는 공유메모리 영역이 존재?
             Attach실시
        - 2.2 공유메모리 영역이 존재하지 않으면
          - SHM_DB_KEY로부터 1씩 감소해가며 공유메모리 영역 신규 생성
 */

IDE_RC smmManager::prepareTBS (smmTBSNode *      aTBSNode,
                               smmPrepareOption  aOp )
{
    // 첫번째 prepare된 TBS의 restore mode
    // 모든 tablespace의 restore mode가 이와 같아야 한다.
    //
    // ( 함수 시작부분 주석에 기술된 제약사항 체크를 위해 사용된다. )
    static smmDBRestoreType sFirstRestoreType = SMM_DB_RESTORE_TYPE_NONE;

    key_t             sTbsShmKey;
    smmShmHeader      sShmHeader;
    idBool            sShmExist;

    IDE_DASSERT( aTBSNode != NULL );

    /* -------------------------------
     * [3] Recovery 이전 callback 설정
     * ----------------------------- */
    if (smuProperty::getShmDBKey() == 0)
    {
        aTBSNode->mRestoreType = SMM_DB_RESTORE_TYPE_DYNAMIC ;
        aTBSNode->mTBSAttr.mMemAttr.mShmKey = 0 ;

        // Restore마친 이후 Restart Redo/Undo다 끝내고
        // Log Anchor에 모든 TBS의 ShmKey가 Flush된다.

        // BUGBUG-1548 정말 Flush하는지 체크할 것

        // 일반 메모리 Page Pool 초기화
        IDE_TEST( initializeDynMemPool(aTBSNode) != IDE_SUCCESS);
    }
    else
    {
        /* ------------------------------------------------
         * [3-1] Fixed Memory Manager Creation
         * ----------------------------------------------*/
        IDE_TEST( initializeShmMemPool(aTBSNode)
                  != IDE_SUCCESS);

        /* ------------------------------------------------
         * [3-2] Exist Check & Set
         * ----------------------------------------------*/
        sTbsShmKey = aTBSNode->mTBSAttr.mMemAttr.mShmKey;

        // 기존에 TBS를 Dynamic Memory로 Restore한 경우 TBSNode의 ShmKey=0
        if ( sTbsShmKey == 0 )
        {
            sShmExist = ID_FALSE;
        }
        else
        {
            IDE_TEST(smmFixedMemoryMgr::checkExist(
                         sTbsShmKey,
                         sShmExist,
                         &sShmHeader) != IDE_SUCCESS);
        }

        if ( sShmExist == ID_FALSE )
        {
            // Just Attach Shared Memory
            aTBSNode->mRestoreType = SMM_DB_RESTORE_TYPE_SHM_CREATE;
        }
        else
        {
            aTBSNode->mRestoreType = SMM_DB_RESTORE_TYPE_SHM_ATTACH;
        }
    }

    if ( (aOp & SMM_PREPARE_OP_DONT_CHECK_RESTORE_TYPE) ==
         SMM_PREPARE_OP_DONT_CHECK_RESTORE_TYPE )
    {
        // 모든 Tablespace가 같은 Restore Type으로 Restore되는지
        // 에러체크를 하지 않음

        // ALTER TABLESPACE ONLINE시에 여기로 들어온다.
        // Startup시에는 Shared Memory Attach되었던 Tablespace가
        // Alter Tablespace Offline시 Shared Memory를 완전히 제거하고
        // Alter Tablespace Online시 Shared Memory Create로 Restore될 수 있기 때문

        // Do Nothing.
    }
    else
    {
        // 제약조건 체크
        if ( sFirstRestoreType == SMM_DB_RESTORE_TYPE_NONE ) // 맨 처음 prepare?
        {
            sFirstRestoreType = aTBSNode->mRestoreType;

            // To Fix BUG-17293 Server Startup시 Tablespace갯수만큼
            //                  Loading메시지가 나옴
            // => 각 Tablespace Loading시마다 Message를 출력하지 않고,
            //    각각의 Server start시 딱 한번만 Loading Message출력.
            printLoadingMessage( sFirstRestoreType );
        }
        else // 두번 째 이후 prepare ?
        {
            // 맨 처음과 항상 같은 Restore모드여야 함
            IDE_TEST_RAISE( aTBSNode->mRestoreType != sFirstRestoreType,
                            error_invalid_shm_region );
        }
    }

    // 공유메모리 Attach의 경우 restore할 필요없이
    // prepare단계에서 attach를 수행한다.
    if ( aTBSNode->mRestoreType == SMM_DB_RESTORE_TYPE_SHM_ATTACH )
    {
        IDE_TEST(restoreAttachSharedDB(aTBSNode,
                                       &sShmHeader,
                                       aOp)
                 != IDE_SUCCESS);
    }
    else
    {
        // 공유메모리 Create나, 일반 메모리모드인 경우
        //
        // Restore시에 실제 Page Memory가 할당/로드되며,
        // Prepare시에는 아무런 처리도 하지 않는다.
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_shm_region );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_INVALID_SHARED_MEMORY_DATABASE_TRIAL_TO_DIFFERENT_RESTORE_MODE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*
    DB File Loading Message출력

    [IN] aRestoreType - DB File Restore Type
 */
void smmManager::printLoadingMessage( smmDBRestoreType aRestoreType )
{
    switch ( aRestoreType )
    {
        case SMM_DB_RESTORE_TYPE_SHM_CREATE :
            IDE_CALLBACK_SEND_SYM("                          : Created Shared Memory Version ");
            break;

        case SMM_DB_RESTORE_TYPE_SHM_ATTACH :
            IDE_CALLBACK_SEND_SYM("                          : Attached Shared Memory Version ");

        case SMM_DB_RESTORE_TYPE_DYNAMIC :
            IDE_CALLBACK_SEND_SYM("                          : Dynamic Memory Version");
            break;

        default:
            IDE_ASSERT(0);

    }

    switch ( aRestoreType )
    {
        // Restore를 하는 경우 Parallel Loading인지,
        // Serial Loading인지 출력
        case SMM_DB_RESTORE_TYPE_SHM_CREATE :
        case SMM_DB_RESTORE_TYPE_DYNAMIC :
            switch(smuProperty::getRestoreMethod())
            {
                case 0:
                    IDE_CALLBACK_SEND_MSG(" => Serial Loading");
                    break;
                case 1:
                    IDE_CALLBACK_SEND_MSG(" => Parallel Loading");
                    break;
                default:
                    IDE_ASSERT(0);
            }
            break;
        default:
            // do nothing
            break;
    }
}

/*
    Alter TBS Online을 위해 Tablespace를 Prepare / Restore 한다.

    [IN] aTBSNode - Restore할 Tablespace의 Node

    [ 에러처리 특이사항 ]
      - 모든 Tablespace가 같은 Restore Type으로 Restore되는지
        에러체크를 하지 않음

      - 이유 :  Startup시에는 Shared Memory Attach되었던 Tablespace가
                Alter Tablespace Offline시 Shared Memory를
                완전히 제거하고 Alter Tablespace Online시
                Shared Memory Create로 Restore될 수 있기 때문
 */
IDE_RC smmManager::prepareAndRestore( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    // 모든 Tablespace가 같은 Restore Type으로 Restore되는지
    // 에러체크를 하지 않음
    IDE_TEST( prepareTBS( aTBSNode,
                          SMM_PREPARE_OP_DONT_CHECK_RESTORE_TYPE )
                  != IDE_SUCCESS );

    // Alter TBS Offline시 경우 공유메모리 자체를 날리기 때문에
    // Alter TBS Online시에는 공유메모리 Attach로 들어올 수가 없다.
    IDE_ASSERT( aTBSNode->mRestoreType != SMM_DB_RESTORE_TYPE_SHM_ATTACH );

    IDE_TEST( restoreTBS( aTBSNode, SMM_RESTORE_OP_NONE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* loganchor의 checkpoint image attribute수를 토대로 계산된
   dbfile 갯수 반환 => restore db시에 사용됨

 [IN] aTBSNode - Tablespace의 Node
 */
UInt smmManager::getRestoreDBFileCount( smmTBSNode      * aTBSNode )
{
    UInt sDBFileCount;

    IDE_DASSERT( aTBSNode != NULL );

    // loganchor(TBSNode)기준으로 loading을 수행한다.
    // +1을 하는 것은 mLstCreateDBFile이 파일 번호이기 때문이다.
    sDBFileCount = aTBSNode->mLstCreatedDBFile + 1;

    return sDBFileCount;
}


/*
 * smmManager::restoreDB를 위한 Action함수
 */
IDE_RC smmManager::restoreTBSAction( idvSQL*             /*aStatistics*/,
                                     sctTableSpaceNode * aTBSNode,
                                     void *              aActionArg )
{
    idBool      sDoIt;
    UInt        sState = 0;
    void       *sPageBuffer;
    void       *sAlignedPageBuffer;

    IDE_DASSERT( aTBSNode != NULL );

    sPageBuffer= NULL;
    sAlignedPageBuffer = NULL;

    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SMM,
                                           SM_PAGE_SIZE,
                                           (void**)&sPageBuffer,
                                           (void**)&sAlignedPageBuffer )
             != IDE_SUCCESS);
    sState = 1;

    // CONTROL단계에서 DROPPED인 TBS는 loganchor로부터
    // 읽어들이지 않기 때문에
    // PREPARE/RESTORE중에 DROPPED상태인 TBS는 있을 수 없다.
    IDE_ASSERT( ( aTBSNode->mState & SMI_TBS_DROPPED ) != SMI_TBS_DROPPED );

    // Memory Tablespace중 DISCARD,OFFLINE Tablespace를 제외하고
    // PREPARE/RESTORE를 수행한다.
    if(( sctTableSpaceMgr::isMemTableSpace(aTBSNode->mID) == ID_TRUE ) &&
       ( sctTableSpaceMgr::hasState(aTBSNode->mID, SCT_SS_SKIP_RESTORE )
         == ID_FALSE) )
    {
        IDE_DASSERT( ID_SIZEOF(void*) >= ID_SIZEOF(smmRestoreOption) );

        switch ( (smmRestoreOption)(vULong)aActionArg )
        {
            case SMM_RESTORE_OP_NONE:
            case SMM_RESTORE_OP_DBIMAGE_NEED_RECOVERY:
            {
                // 정상구동시
                sDoIt = ID_TRUE;
                break;
            }
            case SMM_RESTORE_OP_DBIMAGE_NEED_MEDIA_RECOVERY:
            {
                if ( isMediaFailureTBS( (smmTBSNode*)aTBSNode ) == ID_TRUE )
                {
                    // 미디어복구 플래그가 ID_TRUE로 설정된 데이타파일이
                    // 있을 경우 Restore를 진행한다.
                    sDoIt = ID_TRUE;
                }
                else
                {
                    if( aTBSNode->mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC )
                    {

                        // sys_mem_dic_tbs에 대해서 membase는 미디어복구시
                        // 설정한다. dirty page write시에 check membase를
                        // 수행하는 것을 통과해야한다.
                        IDE_TEST( openFstDBFilesAndSetupMembase(
                                                (smmTBSNode*)aTBSNode,
                                                (smmRestoreOption)(vULong)aActionArg,
                                                (UChar*)sAlignedPageBuffer)
                                  != IDE_SUCCESS );

                        // PCH 엔트리및 Page 메모리를 할당하고 Page내용 복사
                        IDE_TEST( fillPCHEntry( (smmTBSNode*)aTBSNode,
                                                 SMM_MEMBASE_PAGEID, // sPID
                                                 SMM_FILL_PCH_OP_COPY_PAGE,
                                                 (void*)((UChar*)sAlignedPageBuffer))
                                  != IDE_SUCCESS);

                        IDE_TEST(setupBasePageInfo(
                                  (smmTBSNode*)aTBSNode,
                                  (UChar*)mPCArray[((smmTBSNode*)aTBSNode)->mTBSAttr.mID].mPC[SMM_MEMBASE_PAGEID].mPagePtr )
                                  != IDE_SUCCESS);

                        smmDatabase::makeMembaseBackup();
                    }

                   // retore 는 하지 않는다.
                   sDoIt = ID_FALSE;
                }
                break;
            }
            default:
            {
                IDE_ASSERT( 0 );
                break;
            }
        }

        if ( sDoIt == ID_TRUE )
        {
            IDE_TEST( restoreTBS( (smmTBSNode *) aTBSNode,
                                  (smmRestoreOption) (vULong) aActionArg )
                      != IDE_SUCCESS );
        }
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sPageBuffer ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( iduMemMgr::free( sPageBuffer ) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/*
 * 디스크 이미지로부터 데이터베이스 페이지를 로드한다.
 *
 * aOp [IN] Restore 옵션/정보
 */

IDE_RC smmManager::restoreDB ( smmRestoreOption aOp )
{
    IDE_DASSERT( ID_SIZEOF(void*) >= ID_SIZEOF(aOp) );

    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  restoreTBSAction,
                                                  (void*) aOp,
                                                  SCT_ACT_MODE_NONE)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * 테이블스페이스의 첫번째 DB 파일을 모두 오픈하고,
 * Membase 를 설정한다.
 *
 * aTBSNode [IN] TableSpace Node
 * aOp      [IN] Restore 옵션/정보
 */

IDE_RC smmManager::openFstDBFilesAndSetupMembase( smmTBSNode * aTBSNode,
                                                  smmRestoreOption aOp,
                                                  UChar*       aPageBuffer )
{
    UInt              i;
    smmDatabaseFile * sDBFile;
    smmDatabaseFile * sFirstDBFile = NULL;
    UInt              sOpenLoop;

    if ( aOp == SMM_RESTORE_OP_DBIMAGE_NEED_MEDIA_RECOVERY )
    {
        // Media Recovery시에는 Stable 버전만 Open하면 된다.
        i = (UInt)aTBSNode->mTBSAttr.mMemAttr.mCurrentDB; // stable
        sOpenLoop = i+1;
    }
    else
    {
        // Restart시에는 Stable/Unstable 모두 Open한다.
        i =  0;
        sOpenLoop = SMM_PINGPONG_COUNT;
    }
    /* --------------------------
     * [0] First DB File Open
     * ------------------------ */

    for (; i < sOpenLoop ; i++)
    {
        IDE_TEST( getDBFile( aTBSNode,
                             i,
                             0,
                             SMM_GETDBFILEOP_SEARCH_FILE,
                             & sDBFile )
                  != IDE_SUCCESS );

        if( i == (UInt)aTBSNode->mTBSAttr.mMemAttr.mCurrentDB )
        {
            sFirstDBFile = sDBFile;
        }
        
        // LogAnchor에서 초기화될때는 파일을 오픈하지 않으므로,
        // 최초로 Open하는 곳이다.
        if (sDBFile->isOpen() != ID_TRUE )
        {
            IDE_TEST(sDBFile->open() != IDE_SUCCESS);
        }
    }

    // BUG-27456 Klocwork SM (4)
    IDE_ERROR_RAISE (sFirstDBFile != NULL, ERR_NOEXIST_FILE );

    /* ------------------------------------------------
     *  Read catalog page(PageID=0) &
     *  setup mMemBase & m_catTableHeader
     * ----------------------------------------------*/
    if ( aTBSNode->mRestoreType == SMM_DB_RESTORE_TYPE_SHM_ATTACH )
    {
        // Do nothing
        // prepareTBS에서 attach하면서 모두 처리하였다.
    }
    else
    {
        // 공유메모리 Create 또는 일반 메모리모드인 경우
        //
        // 0번 Page를 스택 메모리에 임시적으로 로드하여
        // restore하는 동안에만 사용할
        // Membase와 catalog table header를 설정.

        // restore가 완료된 후에 setBasePageInfo를 다시 호출하여
        // 실제 0번 Page의 주소를 이용하여
        // Membase와 catalog table header를 재설정 해야한다.
        IDE_TEST(sFirstDBFile->readPage(
                                   aTBSNode,
                                   SMM_MEMBASE_PAGEID,
                                   aPageBuffer )
                 != IDE_SUCCESS);

        IDE_TEST(setupBasePageInfo( aTBSNode,
                                    aPageBuffer )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOEXIST_FILE );
    {
        SChar sErrorFile[SM_MAX_FILE_NAME];

        idlOS::snprintf( (SChar*)sErrorFile, SM_MAX_FILE_NAME, "%s-%"ID_UINT32_FMT"-0",
                         aTBSNode->mHeader.mName,
                         i );

        IDE_SET( ideSetErrorCode( smERR_ABORT_NoExistFile,
                                  (SChar*)sErrorFile ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 디스크 이미지로부터 데이터베이스 페이지를 로드한다.
 *
 * aOp [IN] Restore 옵션/정보
 */

IDE_RC smmManager::restoreTBS ( smmTBSNode * aTBSNode, smmRestoreOption aOp )
{
    UInt              sFileIdx;
    SInt              sPingPong;
    idBool            sFound;
    UInt              sDBFileCount;
    SChar             sDBFileName[SM_MAX_FILE_NAME];
    SChar           * sDBFileDir;
    smmDatabaseFile * sDBFile;
    UInt              sState = 0;
    void            * sPageBuffer;
    void            * sAlignedPageBuffer;

    IDE_DASSERT( aTBSNode != NULL );

    sPageBuffer = NULL;
    sAlignedPageBuffer = NULL;

    /*
     * BUG-18828 memory tbs의 restoreTBS 에서 align 되지 않은
     * i/o buffer 를 사용하여 서버 구동실패
     */
    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SMM,
                                           SM_PAGE_SIZE,
                                           (void**)&sPageBuffer,
                                           (void**)&sAlignedPageBuffer )
             != IDE_SUCCESS);
    sState = 1;

    ideLog::log(IDE_SERVER_0,
                "Restoring Tablespace : %s\n",
                aTBSNode->mHeader.mName );

    /* ----------------------------------
     *  [1] open First DBFiles And Setup MemBase
     * -------------------------------- */
    IDE_TEST( openFstDBFilesAndSetupMembase( aTBSNode,
                                             aOp,
                                             (UChar*)sAlignedPageBuffer)
              != IDE_SUCCESS );

    /* ----------------------------------
     *  [2] set total page count in Disk
     * -------------------------------- */
    IDE_TEST( calculatePageCountInDisk(aTBSNode) != IDE_SUCCESS);

    /* ------------------------------------------------
     * [3] All DB File Open
     * ----------------------------------------------*/
    for ( sPingPong = 0 ; sPingPong < SMM_PINGPONG_COUNT; sPingPong++ )
    {
        sDBFileCount = getRestoreDBFileCount( aTBSNode );

        // 파일이 있을 경우에만 Open한다.
        for ( sFileIdx = 1; sFileIdx < sDBFileCount; sFileIdx++ )
        {
            IDE_ASSERT( sFileIdx < aTBSNode->mHighLimitFile );

            if( smmManager::getCreateDBFileOnDisk( aTBSNode,
                                                   sPingPong,
                                                   sFileIdx ) == ID_TRUE )
            {

                sFound =  smmDatabaseFile::findDBFile( aTBSNode,
                                                       sPingPong,
                                                       sFileIdx,
                                                       (SChar*)sDBFileName,
                                                       &sDBFileDir );
                if ( sFound == ID_TRUE )
                {
                    IDE_TEST( openAndGetDBFile( aTBSNode,
                                                sPingPong,
                                                sFileIdx,
                                                & sDBFile )
                              != IDE_SUCCESS )
                }
            }
            else
            {
                /*  디스크에 생성된 기록이 없으면 확인할 필요도 없음. */  
            }
        } //for
    } //for

    /* ------------------------------------------------
     * [2] 실제 DB 로딩
     * ----------------------------------------------*/
    if ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY )
    {
        // log buffer type이 memory 인 경우에는
        // SMM_DB_RESTORE_TYPE_DYNAMIC 만 가능
        IDE_TEST_RAISE( ( aTBSNode->mRestoreType == SMM_DB_RESTORE_TYPE_SHM_CREATE) ||
                        ( aTBSNode->mRestoreType == SMM_DB_RESTORE_TYPE_SHM_ATTACH),
                        err_invalid_database_type );
    }

    (void)invalidate(aTBSNode); // db를 inconsistency 상태로 설정
    {
        ideLog::log(IDE_SERVER_0,
                    "     BEGIN TABLESPACE[%"ID_UINT32_FMT"] RESTORATION\n",
                    aTBSNode->mHeader.mID);

        switch( aTBSNode->mRestoreType )
        {
        case SMM_DB_RESTORE_TYPE_DYNAMIC:
            IDE_TEST( restoreDynamicDB(aTBSNode )
                      != IDE_SUCCESS);
            break;
        case SMM_DB_RESTORE_TYPE_SHM_CREATE:
            IDE_TEST( restoreCreateSharedDB(aTBSNode, aOp ) != IDE_SUCCESS);
            break;
        case SMM_DB_RESTORE_TYPE_SHM_ATTACH:
            // do nothing : prepare시에 이미 Attach완료하였음
            break;
        default:
            IDE_CALLBACK_FATAL("error");
            idlOS::abort();
        }

        ideLog::log(IDE_SERVER_0,
                    "     END TABLESPACE[%"ID_UINT32_FMT"] RESTORATION\n",
                    aTBSNode->mHeader.mID);
    }
    (void)validate(aTBSNode); // db를 consistency 상태로 되돌림.

    // copy membase to backup
    if( aTBSNode->mHeader.mID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC )
    {
        smmDatabase::makeMembaseBackup();
    }

#ifdef DEBUG
    // Recovery가 필요하지 않은 정상 Startup일 경우
    if ( (aOp & SMM_RESTORE_OP_DBIMAGE_NEED_RECOVERY) == 0 )
    {
        // DB가 다 로딩된 상태이므로, Assertion수행
        IDE_DASSERT( smmFPLManager::isAllFPLsValid(aTBSNode) == ID_TRUE );
    }
#endif

    sState = 0;
    IDE_TEST( iduMemMgr::free( sPageBuffer ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_database_type );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_INVALID_SHARED_MEMORY_DATABASE));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( iduMemMgr::free( sPageBuffer ) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}

/*
  PRJ-1548 User Memory Tablespace 개념도입

  미디어복구 플래그가 ID_TRUE로 설정된 데이타파일이 있을 경우
  Media Failure 테이블스페이스이므로 ID_TRUE를 반환한다.

  [IN] aTBSNode - check할 Tablespace의 Node

  [RETURN] ID_TRUE  - media failure 상태
           ID_FALSE - 정상

*/
idBool smmManager::isMediaFailureTBS( smmTBSNode * aTBSNode )
{
    UInt             sFileIdx;
    idBool           sResult;
    smmDatabaseFile *sDatabaseFile;

    IDE_DASSERT( aTBSNode != NULL );

    sResult = ID_FALSE;

    for ( sFileIdx = 0; sFileIdx <= aTBSNode->mLstCreatedDBFile ; sFileIdx ++ )
    {
        IDE_ASSERT(sFileIdx < aTBSNode->mHighLimitFile);

        IDE_ASSERT( getDBFile( aTBSNode,
                               smmManager::getCurrentDB( aTBSNode ),
                               sFileIdx,
                               SMM_GETDBFILEOP_NONE,
                               &sDatabaseFile )
                    == IDE_SUCCESS );

        if ( sDatabaseFile->getIsMediaFailure() == ID_TRUE )
        {
            // Media Failure 상태
            sResult = ID_TRUE;
            break;
        }
        else
        {
            // 정상상태
        }
    }

    return sResult;
}


/*
 * 일반 메모리로 데이터베이스 이미지의 내용을 읽어들인다.
 *
 * aOp [IN] Restore 옵션/정보
 */
IDE_RC smmManager::restoreDynamicDB( smmTBSNode     * aTBSNode)
{

    IDE_ASSERT( aTBSNode->mRestoreType == SMM_DB_RESTORE_TYPE_DYNAMIC );

    switch(smuProperty::getRestoreMethod())
    {
        case 0:
            IDE_TEST(loadSerial2(aTBSNode) != IDE_SUCCESS);
            break;
        case 1:
            IDE_TEST(loadParallel(aTBSNode) != IDE_SUCCESS);
            break;
        default:
            IDE_CALLBACK_FATAL("Can't be here");
    }

    IDE_TEST(setupBasePageInfo(
                 aTBSNode,
                 (UChar*)mPCArray[aTBSNode->mTBSAttr.mID].mPC[SMM_MEMBASE_PAGEID].mPagePtr )
             != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 공유 메모리로 데이터베이스 이미지의 내용을 읽어들인다.
 *
 */
IDE_RC smmManager::restoreCreateSharedDB( smmTBSNode     * aTBSNode,
                                          smmRestoreOption aOp )
{
    scPageID            sNeedPage = 0;
    scPageID            sShmPageChunkCount;
    ULong               sShmChunkSize;

    // 미디어 복구는 Shared Memory Version을 지원하지 않는다.
    // 이미 prepare 과정에서 Error checking을 하였기 때문에
    IDE_ASSERT( aOp != SMM_RESTORE_OP_DBIMAGE_NEED_MEDIA_RECOVERY );
    sShmChunkSize = smuProperty::getShmChunkSize();

    /* ----------------------------------------------------------------------
     * [Section - 1] DB에 사용될 Page 갯수를 얻는다.
     * --------------------------------------------------------------------*/

    /* --------------------------
     * [1-3] 필요한 page 갯수 구하기
     *       free page list count + echo table page count
     * ------------------------ */
    sNeedPage = aTBSNode->mMemBase->mAllocPersPageCount;

    /* -------------------------------------------------------------------
     * [Section - 2] Page 갯수를 기반으로 공유메모리 생성 및  로딩
     * -----------------------------------------------------------------*/

    /* --------------------------
     * [2-1] Shared Memory 생성
     *
     * PR-1561 : [대신증권] shared memory사용시 2GB이상 create할 수 없음
     *
     *  = 공유 메모리 버젼의 DB Startup시에 2G이상의 DB가 존재할 경우
     *    AIX에서 한꺼번에 2G의 공유메모리를 할당받을 수 없다.
     *    따라서, Startup시 지정된 프로퍼티(최대 2G)의 용량단위로
     *    할당받도록 한다.
     *    프로퍼티명 : SMU_STARTUP_SHM_CHUNK_SIZE
     * ------------------------ */

    sShmPageChunkCount = (sShmChunkSize / SM_PAGE_SIZE);

    if (sNeedPage <= sShmPageChunkCount)
    {
        /* ------------------------------------------------
         *  case 1 : 로드할 DB의 크기가 프로퍼티보다 작거나 같음.
         * ----------------------------------------------*/
        // 첫번째 Chunk생성하고 공유메모리 Key를 TBSNode에 설정
        IDE_TEST(smmFixedMemoryMgr::createFirstChunk(
                     aTBSNode,
                     sNeedPage) != IDE_SUCCESS);

        // Restore마친 이후
        // Log Anchor에 모든 TBS의 ShmKey를 Flush해야한다.
        // BUGBUG-1548 정말 Flush하는지 체크할 것
    }
    else
    {
        /* ------------------------------------------------
         *  case 2 : 로드할 DB의 크기가 프로퍼티보다 큼.
         *           so, split DB to shm chunk.
         * ----------------------------------------------*/
        // 첫번째 Chunk생성하고 공유메모리 Key를 Log Anchor에 Flush
        IDE_TEST(smmFixedMemoryMgr::createFirstChunk(
                     aTBSNode,
                     sShmPageChunkCount) != IDE_SUCCESS);
        while(1)
        {
            sNeedPage -= sShmPageChunkCount;
            if (sNeedPage <= sShmPageChunkCount)
            {
                IDE_TEST(smmFixedMemoryMgr::extendShmPage(aTBSNode,
                                                          sNeedPage)
                         != IDE_SUCCESS);
                break;
            }
            else
            {
                IDE_TEST(smmFixedMemoryMgr::extendShmPage(aTBSNode,
                                                          sShmPageChunkCount)
                         != IDE_SUCCESS);
            }
        }
    }

    (void)invalidate(aTBSNode);
    {
        switch(smuProperty::getRestoreMethod())
        {
            case 0:
                IDE_TEST(loadSerial2( aTBSNode ) != IDE_SUCCESS);
                break;
            case 1:
                IDE_TEST(loadParallel( aTBSNode ) != IDE_SUCCESS);
                break;
            default:
                IDE_CALLBACK_FATAL("Can't be here");
        }

        IDE_TEST(setupBasePageInfo(
                     aTBSNode,
                     (UChar*)mPCArray[aTBSNode->mTBSAttr.mID].mPC[SMM_MEMBASE_PAGEID].mPagePtr)
                 != IDE_SUCCESS);
    }
    (void)validate(aTBSNode); // db를 consistency 상태로 되돌림.


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* DB File로부터 테이블에 할당된 Page들을 메모리로 로드한다.
 *
 * 이 함수는 서버 기동시에 디스크 이미지 파일로부터 Page를 메모리로 적재할 때
 * 사용된다.
 *
 * 그리고 DB File이 저장해야 할 Page의 범위를 받아서
 * PCH 엔트리를 할당하고 초기화한다.
 * 또한, DB File로부터 메모리로 로드할 Page의 수를 받아서 Page를 Disk로부터
 * 읽어들인다.
 *
 * 테이블에 할당된 Page의 경우는 DB File에 기록되지만,
 * 한번도 테이블에 할당된 적이 없는 Free Page의 경우는
 * 아예 DB File에 기록조차 되지 않기 때문에,
 * DB File의 크기는 실제 저장해야 하는 Page수를 커버하지 못할 수도 있다.
 *
 * 예를들어, 하나의 DB파일에 1번부터 20번까지 20개의 Page가 기록 가능한데,
 * 이 중 1번부터 10번까지가 테이블에 할당되었고, 11번부터 20번까지는
 * 한번도 사용된적이 없는 Free Page라고 가정해보자.
 *
 * 1번부터 10번까지는 ( 혹은 10번 Page하나라도 ) Disk 이미지에 기록된 적이
 * 있어서 DB File은 10개의 Page를 기록할 수 있는 크기인 320KB까지 크기가
 * 잡히게 된다. ( 하나의 Page는 32KB라고 가정 )
 * 이 함수는 1번부터 10번까지의 Page에 대해서는 PCH엔트리를 할당하고,
 * Page 메모리 또한 할당한 후, Disk로부터 읽은 Page의 내용을 복사한다.
 *
 * 11번부터 20번까지는 한번도 쓰인적이 없는 Free Page라서 Disk이미지에
 * 기록조차 되지 않으며, Free Page라도 PCH 엔트리는 유지해야 하기 때문에,
 * 이 함수는 11번부터 20번까지는 PCH 엔트리를 할당하고 초기화만 한다.
 *
 * 이 예에서 loadDbFile의 인자는 다음과 같다.
 * ( aFileMinPID = 1, aFileMaxPID = 20, aLoadPageCount = 10 )
 *
 * aFileNumber    [IN] DB File 번호 - 0부터 시작한다.
 * aFileMinPID    [IN] DB File에 기록되어야 하는 Page 범위 - 첫번째 Page ID
 * aFileMaxPID    [IN] DB File에 기록되어야 하는 Page 범위 - 마지막 Page ID
 * aLoadPageCount [IN] 첫번째 Page부터 시작하여 메모리로 읽어들일 Page의 수
 */


IDE_RC smmManager::loadDbFile( smmTBSNode *     aTBSNode,
                               UInt             aFileNumber,
                               scPageID         aFileMinPID,
                               scPageID         aFileMaxPID,
                               UInt             aLoadPageCount )
{
#ifdef DEBUG
    scSpaceID        sSpaceID;
#endif
    scPageID         sPID;
    scPageID         sLastLoadedPID;

#ifdef DEBUG
    sSpaceID = aTBSNode->mTBSAttr.mID;
#endif

    IDE_DASSERT( isValidPageID( sSpaceID, aFileMinPID )
                 == ID_TRUE );
    /* Max PID는 File이 가질 수 있는 최대크기 이기때문에 Valid Page가
     * 실패할 수 있다.
    IDE_DASSERT( isValidPageID( sSpaceID, aFileMaxPID )
                 == ID_TRUE );
    */

    IDE_TEST( loadDbPagesFromFile( aTBSNode,
                                   aFileNumber,
                                   aFileMinPID,
                                   aLoadPageCount ) != IDE_SUCCESS );

    // DB 이미지로부터 Load한 Page 중 PID값 가장 큰것
    sLastLoadedPID = aFileMinPID + aLoadPageCount - 1;

    // DISK 이미지에 한번도 기록조차 되지 않은 모든 Page에 대해서
    // PCH Entry할당
    for (sPID = sLastLoadedPID + 1; sPID <= aFileMaxPID; sPID++)
    {
        if ( sPID >= aTBSNode->mDBMaxPageCount )
        {
            // 최대 페이지 범위를 벗어났다면 PCH Entry할당중지
            break;
        }
        else
        {
            // PCH만 할당하고 Page메모리는 할당하지 않는다.
            IDE_TEST( allocPCHEntry( aTBSNode, sPID ) != IDE_SUCCESS );
        }
    }



    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 하나의 DB파일에 속한 모든 Page를 메모리 Page로 로드한다.
 *
 * - 하나의 데이터 파일을 여러 개의 조각(Chunk)로 나누어 로드한다.
 * - 이러한 로드할 조각의 크기는 RESTORE_CHUNK_PAGE_COUNT로 조정 가능하다.
 * - RESTORE_CHUNK_PAGE_COUNT가 너무 크면 메모리 부족으로 인해 Startup이
 *   실패할 수도 있다. ( BUG-15020 참고 )
 * - RESTORE_CHUNK_PAGE_COUNT가 너무 작으면 빈번한 I/O수행으로 인해
 *   Startup시 성능이 저하될 수 있다.
 *
 * aFileNumber    [IN] DB File 번호 - 0부터 시작한다.
 * aFileMinPID    [IN] DB File에 기록되어야 하는 Page 범위 - 첫번째 Page ID
 * aLoadPageCount [IN] 첫번째 Page부터 시작하여 메모리로 읽어들일 Page의 수
 */

IDE_RC smmManager::loadDbPagesFromFile(smmTBSNode *     aTBSNode,
                                       UInt             aFileNumber,
                                       scPageID         aFileMinPID,
                                       ULong            aLoadPageCount )
{
    UInt        sStage = 0;
    void      * sRealBuffer    = NULL; /* 할당받은 청크 메모리 주소 */
    void      * sAlignedBuffer = NULL; /* AIO를 위해 Align한 메모리 주소 */
    SLong       sRemainPageCount;      /* 앞으로 더 로드할 (남은)Page 수 */
    scPageID    sChunkStartPID;        /* 로드할 조각의 첫번째 Page ID */
    UInt        sChunkPageCount;       /* 로드할 조각에 속한 Page의 수 */

    IDE_DASSERT( isValidPageID( aTBSNode->mTBSAttr.mID, aFileMinPID )
                 == ID_TRUE );

    smmDatabaseFile *sDbFile;

    IDE_TEST( openAndGetDBFile( aTBSNode,
                                aTBSNode->mTBSAttr.mMemAttr.mCurrentDB,
                                aFileNumber,
                                &sDbFile )
              != IDE_SUCCESS );

    // Page들을 데이터베이스 파일에서 읽어오는 I/O를 수행한다.
    IDE_TEST(iduFile::allocBuff4DirectIO(
                 IDU_MEM_SM_SMM,
                 smuProperty::getRestoreBufferPageCount()
                 * SM_PAGE_SIZE,
                 (void**)&sRealBuffer,
                 (void**)&sAlignedBuffer)
             != IDE_SUCCESS);
    sStage = 1;


    sChunkStartPID   = aFileMinPID ;
    sRemainPageCount = aLoadPageCount;

    do
    {
        /* smuProperty::getRestoreBufferPageCount() 나 sRemainPageCount중
           작은 페이지 수만큼 로드 */
        sChunkPageCount =
            ( sRemainPageCount < smuProperty::getRestoreBufferPageCount() ) ?
            sRemainPageCount : smuProperty::getRestoreBufferPageCount();

        IDE_TEST( loadDbFileChunk( aTBSNode,
                                   sDbFile,
                                   sAlignedBuffer,
                                   aFileMinPID,
                                   sChunkStartPID,
                                   sChunkPageCount )
                  != IDE_SUCCESS );

        sChunkStartPID   += smuProperty::getRestoreBufferPageCount();
        sRemainPageCount -= smuProperty::getRestoreBufferPageCount();
    } while ( sRemainPageCount > 0 );

    sStage = 0;
    IDE_TEST(iduMemMgr::free(sRealBuffer) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1 :
            IDE_ASSERT( iduMemMgr::free(sRealBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;

}

/* 데이터베이스 파일의 일부 조각(Chunk) 하나를 메모리 페이지로 로드한다.
 *
 *
 * 데이터베이스 페이지를 디스크로부터 메모리로 올리는 작업은
 * Restart Recovery이전에 이루어진다.
 * 특정 Page가 Free Page 인지 여부를 가려서 테이블에 할당된 Page만을
 * 로딩하게 되면, Restart Recovery이전에 특정 Page가 Free Page인지
 * 여부를 알기 위해 Free List Info Page에 접근하게 된다.
 *
 * 이때, Recovery가 되지 않은 상태에서 Free List Info Page에 접근하게
 * 되어, Free List Info Page에 엉뚱한 내용이 기록되어 있을 수 있다,
 * 이로 인해 Free Page가 아닌데도 Free Page로 판단하거나,
 * 테이블에 할당된 Page가  아닌데도 테이블에 할당된 Page로 판단하여
 * 로드해야 할 Page를 로드하지 않거나, 불필요한 Page를 로드하게 되는
 * 문제점이 발생한다.
 *
 * 해결책 : Restart Recovery이전에 Page 로딩시에는 Free Page인지
 *           여부를 알 수가 없다.
 *
 *           우선 Free Page정보가 맞을 것이라고 가정하고
 *           DB File을 로드한다.
 *
 *           Free Page정보가 맞지 않을 경우, 즉, Restart Recovery
 *           중에 Redo/Undo를 위한 Page가 메모리에 없을 경우,
 *           해당 Page를 DB File에서 별도로 로드한다.
 *
 *           Restart Recovery후에 Free Page들에 대해서
 *           Page 메모리를 반납한다.
 *
 * aDbFile         [IN] 로드할 데이터 페이지가 있는 데이터 파일
 * aAlignedBuffer  [IN] 데이터가 로드될 메모리 위치.
 *                      시작주소가 AIO를 위한 경계로 Align되어 있다.
 * aFileMinPID     [IN] DB File에 기록되어야 하는 Page 범위 - 첫번째 Page ID
 * aChunkStartPID  [IN] 로드할 조각(Chunk)의 시작 PID
 * aChunkPageCount [IN] 로드할 조각(Chunk)의 페이지 수
 *
 * 주의! 이 함수에서의 Chunk는 DB파일을 로드할 단위로,
 *        Expand Chunk와 아무런 관련이 없다.
 */

IDE_RC smmManager::loadDbFileChunk(smmTBSNode      * aTBSNode,
                                   smmDatabaseFile * aDbFile,
                                   void            * aAlignedBuffer,
                                   scPageID          aFileMinPID,
                                   scPageID          aChunkStartPID,
                                   ULong             aChunkPageCount )
{
#ifdef DEBUG
    scSpaceID        sSpaceID;
#endif
    scPageID         sPID ;
    size_t           sReadSize;    /* Data File에서 실제 읽은 Page 수 */
    idBool           sIsFreePage ; /* 하나의 Page가 Free Page인지 여부 */
    UInt             sPageOffset ; /* aAlignedBuffer안에서의 Page의 Offset */
    scPageID         sChunkEndPID; /* 하나의 조각(Chunk)의 마지막 PID */

#ifdef DEBUG
    sSpaceID = aTBSNode->mTBSAttr.mID;
#endif
    IDE_DASSERT( aDbFile != NULL );
    IDE_DASSERT( aAlignedBuffer != NULL );
    IDE_DASSERT( isValidPageID(sSpaceID, aChunkStartPID )
                 == ID_TRUE );

#ifndef VC_WIN32
    if (smuProperty::getRestoreAIOCount() > 0 &&
        (aChunkPageCount * SM_PAGE_SIZE) > SMM_MIN_AIO_FILE_SIZE)
    {
        IDE_TEST(aDbFile->readPagesAIO( (aChunkStartPID - aFileMinPID) * SM_PAGE_SIZE,
                                        aAlignedBuffer,
                                        aChunkPageCount * SM_PAGE_SIZE,
                                        &sReadSize,
                                        smuProperty::getRestoreAIOCount()
                                        ) != IDE_SUCCESS);
        if( sReadSize != (aChunkPageCount * SM_PAGE_SIZE) )
        {
            ideLog::log(IDE_SERVER_0, "2. sReadSize(%lld), require(%lld)\n",
                        sReadSize,
                        aChunkPageCount * SM_PAGE_SIZE );
            IDE_RAISE( read_size_error );

//              IDE_TEST_RAISE(sReadSize != (aPageCount * SM_PAGE_SIZE),
//                             read_size_error);
        }

    }
    else
#endif
    {
        IDE_TEST(aDbFile->readPages( (aChunkStartPID - aFileMinPID) * SM_PAGE_SIZE,
                                     aAlignedBuffer,
                                     aChunkPageCount * SM_PAGE_SIZE,
                                     &sReadSize) != IDE_SUCCESS);

        if( sReadSize != (aChunkPageCount * SM_PAGE_SIZE) )
        {
            ideLog::log(IDE_SERVER_0,
                        "1. sReadSize(%llu), require(%llu)\n",
                        (ULong)sReadSize,
                        (ULong)aChunkPageCount * (ULong)(SM_PAGE_SIZE) );
            IDE_RAISE( read_size_error );
        }
    }

    sChunkEndPID = aChunkStartPID + aChunkPageCount - 1;

    // 로드한 모든 Page에 대해
    for ( sPID = aChunkStartPID;
          sPID <= sChunkEndPID;
          sPID ++ )
    {
        sIsFreePage = ID_FALSE ;

        // Page를 Load해야 할지 검사
        IDE_TEST( smmExpandChunk::isFreePageID ( aTBSNode,
                                                 sPID,
                                                 & sIsFreePage )
                  != IDE_SUCCESS );

        // 테이블에 할당되지 않은 데이터베이스 Free Page인 경우
        if ( sIsFreePage == ID_TRUE )
        {
            // PCH만 할당하고 Page메모리는 할당하지 않는다.
            IDE_TEST( allocPCHEntry( aTBSNode, sPID ) != IDE_SUCCESS );
        }
        else // 테이블에 할당된 Page인 경우
        {

            // DB File 안에서 Page의 Offset
            sPageOffset = ( sPID - aChunkStartPID ) * SM_PAGE_SIZE ;

            // Page Offset 으로부터 Page 크기만큼 데이터 복사해도 되는지 체크
            IDE_ASSERT( sPageOffset + SM_PAGE_SIZE - 1 < sReadSize );

            // PCH 엔트리및 Page 메모리를 할당하고 Page내용 복사
            IDE_TEST( fillPCHEntry(aTBSNode,
                                   sPID,
                                   SMM_FILL_PCH_OP_COPY_PAGE,
                                   (UChar *)aAlignedBuffer +
                                   sPageOffset
                                   ) != IDE_SUCCESS);
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION(read_size_error);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                    SM_TRC_MEMORY_LOADING_DATAFILE_FATAL);
        IDE_SET(ideSetErrorCode(smERR_FATAL_SysRead));
    }
    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/*
    [동시성 제어]
       Restart Recovery끝나고 즉시 불리우므로
       여러 Transaction간의 동시성 제어를 고려할 필요가 없다.
 */
void smmManager::setLstCreatedDBFileToAllTBS ( )
{
    smmMemBase   sMemBase;
    smmTBSNode * sCurTBS;
    UInt         sDBFileCount;

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurTBS != NULL )
    {
        if ( sctTableSpaceMgr::isMemTableSpace(sCurTBS->mHeader.mID) == ID_TRUE )
        {

            sDBFileCount = 0;

            if (sCurTBS->mRestoreType == SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET )
            {
                // To Fix BUG-17997 [메모리TBS] DISCARD된 Tablespace를
                //                  DROP시에 Checkpoint Image 지워지지 않음
                //
                // Restore되지 않은 Tablespace라도 DB로부터 Membase를 읽어서
                // DB File갯수를 설정한다.
                if ( smmManager::readMemBaseFromFile( sCurTBS,
                                                      &sMemBase )
                     == IDE_SUCCESS )
                {
                    sDBFileCount =
                        smmDatabase::getDBFileCount(
                            & sMemBase,
                            sCurTBS->mTBSAttr.mMemAttr.mCurrentDB);
                }
                else
                {
                    // Discard된 Tablespace의 경우
                    // 아예 DB파일이 없을 수도 있다.

                    // 이 경우 SKIP한다.
                }
            }
            else
            {
                sDBFileCount =
                    smmDatabase::getDBFileCount(
                        sCurTBS->mMemBase,
                        sCurTBS->mTBSAttr.mMemAttr.mCurrentDB);
            }

            if ( sDBFileCount > 0 )
            {
                // 마지막 생성된 DB파일 번호이므로, DB파일 갯수에서 1감소
                sCurTBS->mLstCreatedDBFile = sDBFileCount - 1;
            }
        } // isMemTableSpace

        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }
}






/*
   데이터베이스 안의 Page중 Free Page에 할당된 메모리를 해제한다

   [동시성 제어]
      Restart Recovery끝나고 즉시 불리우므로
      여러 Transaction간의 동시성 제어를 고려할 필요가 없다.
 */
IDE_RC smmManager::freeAllFreePageMemory()
{
    smmTBSNode * sCurTBS;

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurTBS != NULL )
    {
        if ( ( sctTableSpaceMgr::isMemTableSpace(sCurTBS->mHeader.mID) != ID_TRUE ) ||
             ( sCurTBS->mRestoreType == SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET ) )
        {
            sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
            continue;
        }

        IDE_TEST( freeTBSFreePageMemory( sCurTBS )
                  != IDE_SUCCESS );

        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * 데이터베이스 안의 Page중 Free Page에 할당된 메모리를 해제한다
 *
 * 데이터베이스 Page가 Free Page인지 Allocated Page인지를 Free List Info Page를
 * 보고 알 수 있는데, 이 영역이 Restart Recovery가 끝나야 복구가 완료되므로,
 * Restart Recovery가 완료된 후에 Free Page인지 여부를 분간할 수가 있다.
 *
 * Restart Recovery전에는 Free Page인지, Allocated Page인지 구분하지 않고
 * 무조건 페이지를 디스크로부터 메모리로 로드한다.
 * 그리고 Restart Recovery가 완료되고 나면, 불필요하게 로드된 Free Page들의
 * Page 메모리를 메모리를 반납한다.
 */
IDE_RC smmManager::freeTBSFreePageMemory(smmTBSNode * aTBSNode)
{
    scSpaceID sSpaceID;
    scPageID  sPID ;
    idBool    sIsFreePage;

    IDE_DASSERT( aTBSNode->mMemBase != NULL );

    sSpaceID = aTBSNode->mTBSAttr.mID;

    // BUGBUG kmkim Restart Recovery시점에 불린 것인지 ASSERT걸것.
    for ( sPID = SMM_DATABASE_META_PAGE_CNT;
          sPID < aTBSNode->mMemBase->mAllocPersPageCount;
          sPID ++ )
    {
        IDE_DASSERT( mPCArray[sSpaceID].mPC[ sPID ].mPCH != NULL );

        if ( mPCArray[sSpaceID].mPC[sPID].mPCH != NULL )
        {
            // BUG-31191 Page가 Alloc되어 있는 경우에만 해제한다.
            if ( mPCArray[sSpaceID].mPC[sPID].mPagePtr != NULL )
            {
                IDE_TEST( smmExpandChunk::isFreePageID ( aTBSNode,
                                                         sPID,
                                                         & sIsFreePage )
                          != IDE_SUCCESS );

                // 테이블에 할당되지 않은 데이터베이스 Free Page인 경우
                if ( sIsFreePage == ID_TRUE )
                {
                    // 페이지 메모리를 반납한다.
                    IDE_TEST( freePageMemory( aTBSNode, sPID ) != IDE_SUCCESS );
                }
            }
        }
    }

    // 현재 할당된 Page로부터 DB가 최대 가질 수 있는 Page수까지
    // Loop을 돌며 혹시 Page Memory가 할당된 Page가 있는지 검사한다.
    for ( sPID = aTBSNode->mMemBase->mAllocPersPageCount;
          sPID < aTBSNode->mDBMaxPageCount ;// BUG-15066 MEM_MAX_DB_SIZE 벗어나지 않도록
          sPID ++ )
    {
        if ( mPCArray[sSpaceID].mPC[sPID].mPCH != NULL )
        {
            if ( mPCArray[sSpaceID].mPC[sPID].mPagePtr != NULL )
            {
                // Media Recovery를 Until Cancel로 수행한 후
                // Restart Recovery완료하면
                //
                // mAllocPersPageCount의 위치가 Media Recovery수행 이전보다
                // 더 작아질 수 있다.
                //
                // 이 경우 mAllocPersPageCount ~ mDBMaxPageCount사이에
                // Page메모리가 할당되어 있을 수 있다.
                // 이러한 경우 Page Memory를 반납해 주어야 한다.

                // 페이지 메모리를 반납한다.
                IDE_TEST( freePageMemory( aTBSNode, sPID ) != IDE_SUCCESS );
            }
        }
    }

    // 테이블에 할당된 페이지이면서 메모리가 없는 경우 있는지 체크
    // Free Page이면서 페이지 메모리 설정된 경우 있는지 체크
    IDE_DASSERT( isAllPageMemoryValid(aTBSNode) == ID_TRUE );

    // Restart Recovery가 완료된 상태이므로, Free Page List에 대한 Assertion수행
    IDE_DASSERT( smmFPLManager::isAllFPLsValid(aTBSNode) == ID_TRUE );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/*
 * 테이블에 할당된 페이지이면서 메모리가 없는 경우 있는지 체크
 * Free Page이면서 페이지 메모리 설정된 경우 있는지 체크
 */
idBool smmManager::isAllPageMemoryValid(smmTBSNode * aTBSNode)
{
    void   * sPagePtr;
    idBool    sIsValid = ID_TRUE;
    scSpaceID sSpaceID;
    scPageID  sPID;
    idBool    sIsFreePage;

    IDE_DASSERT( aTBSNode->mMemBase != NULL );

    sSpaceID = aTBSNode->mTBSAttr.mID;

    // 테이블에 할당된 페이지이면서 메모리가 없는 경우 있는지 체크
    // 테이블에 할당된 페이지인지 여부를 Restart Recovery가 끝난후
    // 확인이 가능하며, 이 함수가 Restart Recovery후에
    // 호출되므로, 여기에서 체크를 실시한다.
    for (sPID = 0; sPID < aTBSNode->mDBMaxPageCount; sPID++)
    {
        sPagePtr = mPCArray[sSpaceID].mPC[sPID].mPagePtr;

        if ( mPCArray[sSpaceID].mPC[sPID].mPCH != NULL)
        {
            if ( sPID < aTBSNode->mMemBase->mAllocPersPageCount )
            {
                IDE_ASSERT( smmExpandChunk::isFreePageID (aTBSNode,
                                                          sPID,
                                                          & sIsFreePage )
                            == IDE_SUCCESS );

                if ( sIsFreePage == ID_TRUE )
                {
                    // Free Page인데 페이지 메모리가 있으면 에러
                    if ( sPagePtr != NULL )
                    {
                        sIsValid = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    // Table에 할당된 페이지인제 페이지 메모리가 없으면 에러
                    if ( sPagePtr == NULL )
                    {
                        sIsValid = ID_FALSE;
                        break;
                    }
                }
            }
            else
            {
                // 아직 DB에 할당된 PAGE가 아닌데
                // 페이지 메모리가 있으면 에러
                if ( sPagePtr != NULL )
                {
                    sIsValid = ID_FALSE;
                    break;
                }
            }
        }
    }
    return sIsValid ;
}



/*
 * 디스크상의 데이터베이스 이미지를 메모리로 로드한다 ( 1개의 Thread만 사용 )
 *
 * aCurrentDB [IN] 로드 하려는 Ping-Pong 데이터베이스 ( 0 혹은 1 )
 * aOp [IN] Restore 옵션/정보
 */
IDE_RC smmManager::loadSerial2( smmTBSNode     * aTBSNode)
{
    smmDatabaseFile *s_DbFile;
    ULong            s_nFileSize = 0;
    UInt             sDBFileCount;
    ULong            i;
    scPageID         sPageID = 0;
    vULong           sPageCountPerFile ;
    vULong           sWrittenPageCount;
    UInt             sCurrentDB = aTBSNode->mTBSAttr.mMemAttr.mCurrentDB;

    IDE_DASSERT( sCurrentDB == 0 || sCurrentDB == 1 );


    sDBFileCount = getRestoreDBFileCount( aTBSNode );

    /* ------------------------------------------------
     *  - 선택된 DB에 대한 각 화일을 선택
     * ----------------------------------------------*/
    for (i = 0; i < sDBFileCount; i++)
    {
        // 이 파일에 기록할 수 있는 Page의 수
        sPageCountPerFile = smmManager::getPageCountPerFile( aTBSNode, i );

        // DB 파일이 Disk에 존재한다면?
        if ( smmDatabaseFile::isDBFileOnDisk( aTBSNode, sCurrentDB, i )
             == ID_TRUE )
        {
            IDE_TEST( openAndGetDBFile( aTBSNode,
                                        sCurrentDB,
                                        i,
                                        &s_DbFile )
                      != IDE_SUCCESS );

            // 실제 파일에 기록된 Page의 수를 계산
            IDE_TEST(s_DbFile->getFileSize(&s_nFileSize) != IDE_SUCCESS);

            if ( s_nFileSize > SM_DBFILE_METAHDR_PAGE_SIZE )
            {
                sWrittenPageCount =
                    (s_nFileSize - SM_DBFILE_METAHDR_PAGE_SIZE)
                    / SM_PAGE_SIZE;

                // DB파일로부터 Page를 로드한다.
                // 아직 기록되지는 않았으나 이 파일에 기록될 수 있는 Page들에
                // 대해서는 PCH 엔트리만을 할당하고 초기화한다.
                // 이에 대한 자세한 내용은 loadDbFile의 주석을 참고한다.
                IDE_TEST(loadDbFile(aTBSNode,
                                    i,
                                    sPageID,
                                    sPageID + sPageCountPerFile - 1,
                                    sWrittenPageCount ) != IDE_SUCCESS);
            }
            else
            {
                // To FIX BUG-18630
                // SM_DBFILE_METAHDR_PAGE_SIZE(8K)보다
                // 작은 DB파일이 존재할 경우 Restart Recovery실패함
                //
                // 8K보다 작은 크기의 DB파일에는
                // 데이터 페이지가 기록되지 않은 것이므로
                // DB파일 Restore를 SKIP한다.
            }

        }
        else // DB File이 Disk에 없는 경우
        {
            if ( SMI_TBS_IS_DROPPED(aTBSNode->mHeader.mState) )
            {
                // Tablespace를 Drop한 Transaction이 Commit된 경우 ======
                // => nothing to do
                //
                // Drop Tablespace에 대한 Pending작업 수행도중
                // Server가 사망한 경우이다.
                //
                // Tablespace의 상태는 ( ONLINE | DROP_PENDING ) 이다.
                //
                // 이 경우에는 Drop Tablespace가 Pending작업을 수행하면서
                // Checkpoint Image File의 일부, 혹은 전부를 삭제하였을
                // 수도 있다.
                //
                // 그러므로 File이 없는 경우 Load를 하지 않고 SKIP한다.
                //
                // 참고> 해당 File에 속한 Page에 대해 Redo가 발생할 경우
                //
                //   이 함수에서 DB File Load를 하지 않았으므로
                //   Page Memory가 NULL이다. 하지만 Redo시에
                //   Page Memory가 NULL이면 Page를 할당하고 Page를 초기화
                //   하기 때문에 Redo는 문제없이 수행된다.
                //
                //   이후 DROP TABLESPACE로그를 만나면 Tablespace의
                //   PAGE단계를 해제하기 때문에 해당 Page는
                //   메모리 해제된다.
            }
            else
            {
                // 일반적인 경우 =========================================
                //
                // getRestoreDBFileCount는 확실하게 생성된 DB File의
                // 갯수 만을 리턴한다.
                // => DB File이 없는 상황이 있을 수 없음.
                IDE_ASSERT(0);
            }
        }

        // PROJ-1490
        // DB파일안의 Free Page는 Disk로 내려가지도 않고
        // 메모리로 올라가지도 않는다.
        // 그러므로, DB파일의 크기와 DB파일에 저장되어야 할 Page수와는
        // 아무런 관계가 없다.
        //
        // 각 DB파일이 기록해야할 Page의 수를 계산하여
        // 각 DB파일의 로드 시작 Page ID를 계산한다.
        sPageID += sPageCountPerFile;
    }

    aTBSNode->mStartupPID = sPageID;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * 시스템의 공유메모리에 존재하는 데이터베이스 페이지들을 ATTACH한다.
 *
 * aTBSNode   [IN] Page가 Attach될 Tablespace
 * aShmHeader [IN] 공유메모리 헤더
 * aOp        [IN] Prepare 옵션/정보
 */
IDE_RC smmManager::restoreAttachSharedDB(smmTBSNode       * aTBSNode,
                                         smmShmHeader     * aShmHeader,
                                         smmPrepareOption   aOp )
{
    scSpaceID     sSpaceID;
    scPageID      i;
    smmTempPage * sPageSelf;
    smmTempPage * sCurPage;
    smmSCH      * sSCH;
    UChar       * sBasePage;

    IDE_DASSERT( aShmHeader != NULL );

    IDE_ASSERT( aTBSNode->mRestoreType == SMM_DB_RESTORE_TYPE_SHM_ATTACH );

    sSpaceID = aTBSNode->mTBSAttr.mID;

    /* ------------------------------------------------
     * [1] Base Shared Memory에 대한 설정
     * ----------------------------------------------*/
    IDE_TEST(smmFixedMemoryMgr::attach(aTBSNode, aShmHeader) != IDE_SUCCESS);

    // SHM Page를 따라가면서 [ Reverse Mapping ]
    //
    // Allocated Page만 페이지 메모리를 할당하였으므로,
    // Free Page에 대해서는 fillPCHEntry가 호출되지 않는다.
    sSCH = aTBSNode->mBaseSCH.m_next;
    while(sSCH != NULL)
    {
        sBasePage = (UChar *)sSCH->m_header + SMM_CACHE_ALIGNED_SHM_HEADER_SIZE;

        for (i = 0; i < sSCH->m_header->m_page_count; i++)
        {
            sCurPage  = (smmTempPage *)(sBasePage + (SM_PAGE_SIZE * i));
            sPageSelf = sCurPage->m_header.m_self;

            if ( sPageSelf == (smmTempPage *)SMM_SHM_LOCATION_FREE )
            {
                /* BUG-19583: shmutil이 Shared Memory DB영역을 읽기만 해야하는데
                 * Page Link를 갱신하는 경우가 있습니다.
                 *
                 * Page가 Free일경우 Free List에 페이지를 연결하는 작업을 하는데
                 * Shmutil일 경우에는 이 작업을 하면 안된다. 단지 Read만 해야
                 * 한다.
                 * */
                if( aOp != SMM_PREPARE_OP_DONT_CHECK_DB_SIGNATURE_4SHMUTIL )
                {
                    IDE_TEST(smmFixedMemoryMgr::freeShmPage(
                                                     aTBSNode,
                                                     (smmTempPage *)sCurPage)
                             != IDE_SUCCESS);
                }
            }
            else
            {
                IDE_TEST( fillPCHEntry( aTBSNode,
                                        smLayerCallback::getPersPageID( sCurPage ),
                                        SMM_FILL_PCH_OP_SET_PAGE,
                                        sCurPage )
                          != IDE_SUCCESS );
            }
        }
        sSCH = sSCH->m_next;
    }
    IDE_TEST(setupBasePageInfo(
                 aTBSNode,
                 (UChar *)mPCArray[sSpaceID].mPC[SMM_MEMBASE_PAGEID].mPagePtr)
            != IDE_SUCCESS);

    if ( aOp ==  SMM_PREPARE_OP_DONT_CHECK_DB_SIGNATURE_4SHMUTIL )
    {
        // shmutil이 호출한 경우이다.
        // Disk상에 DB파일이 존재하지 않을 수도 있는 상황이므로
        // DB Signature를 Check하지 않는다
    }
    else
    {
        IDE_TEST(smmFixedMemoryMgr::checkDBSignature(
                     aTBSNode,
                     aTBSNode->mTBSAttr.mMemAttr.mCurrentDB) != IDE_SUCCESS);
    }


    /////////////////////////////////////////////////////////////////////
    // Free Page에 대해서는 PCH Entry조차 만들지 않았다.
    // Free Page들의 PCH Entry를 생성한다.
    //
    // PCH엔트리가 구성되지 않은 모든 Page들에 대해 PCH 엔트리를 구성해준다.
    /////////////////////////////////////////////////////////////////////
/*
    mStartupPID = 0;

    for (i = 0; i < getDbFileCount( aDbNumber ) ; i++ )
    {
        mStartupPID += smmManager::getPageCountPerFile( i );
    }
*/
    // Checkpoint가 발생하지 않으면, 할당된 PAGE임에도 불구하고 DISK
    // 에조차 내려가지 않기 때문에 Disk상의 파일을 토대로 mStartupPID를
    // 계산해내면 안된다.
    aTBSNode->mStartupPID = aTBSNode->mMemBase->mAllocPersPageCount;

    // 모든 데이터베이스 Page에 대해 PCH 엔트리가 구성되도록 한다.
    for ( i = 0;
          i < aTBSNode->mStartupPID ;
          i ++ )
    {
        if ( mPCArray[sSpaceID].mPC[i].mPCH == NULL )
        {
            IDE_TEST( allocPCHEntry( aTBSNode, i ) != IDE_SUCCESS );
        }
    }

    // To Fix BUG-15112
    // Restart Recovery중에 Page Memory가 NULL인 Page에 대한 Redo시
    // 해당 페이지를 그때 그때 필요할 때마다 할당한다.
    // 여기에서 페이지 메모리를 미리 할당해둘 필요가 없다


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



// SyncDB
// 1. 운영중(CheckPoint)일 경우에는 모든 Online Tablespace에 대해서
// Sync를 수행한다.
// 2. 미디어복구중에는 모든 Online/Offline TableSpace 중 미디어 복구
// 대상인 것만 Sync를 수행한다.
//
// [ 인자 ]
// [IN] aSkipStateSet - Sync하지 않을 TBS 상태 집합
// [IN] aSyncLatch    - SyncLatch 획득이 필요한경우
IDE_RC smmManager::syncDB( sctStateSet aSkipStateSet,
                           idBool aSyncLatch )
{
    UInt         sStage = 0;

    sctTableSpaceNode * sSpaceNode;

    sSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while( sSpaceNode != NULL )
    {
        if ( sctTableSpaceMgr::isMemTableSpace( sSpaceNode ) == ID_TRUE )
        {
            if ( aSyncLatch == ID_TRUE )
            {
                // TBS상태가 DROP이나 OFFLINE으로 전이되지 않도록 보장
                IDE_TEST( sctTableSpaceMgr::latchSyncMutex( sSpaceNode )
                          != IDE_SUCCESS );
                sStage = 1;
            }

            if ( ((smmTBSNode*)sSpaceNode)->mRestoreType
                 != SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET )
            {
                // TBS가 Memory에 Loading 된 상태

                if ( sctTableSpaceMgr::hasState(
                            sSpaceNode->mID,
                            aSkipStateSet ) == ID_TRUE )
                {
                    // TBS가 sync할 수 없는 상태
                    // 1. Checkpoint 시 -> DISCARD/DROPPED/OFFLINE
                    // 2. Media Recovery 시 -> DISCARD/DROPPED
                }
                else
                {
                    // TBS가 sync할 수 있는 상태
                    IDE_TEST( syncTBS( (smmTBSNode*) sSpaceNode )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                // TBS가 Memory에 Restore 되지 않은 상태
            }

            if ( aSyncLatch == ID_TRUE )
            {
                sStage = 0;
                IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( sSpaceNode )
                          != IDE_SUCCESS );
            }
        }
        sSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sSpaceNode->mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1:
                IDE_ASSERT( sctTableSpaceMgr::unlatchSyncMutex(sSpaceNode)
                            == IDE_SUCCESS );
            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smmManager::syncTBS(smmTBSNode * aTBSNode)
{
    UInt sDBFileCount;
    UInt i;
    UInt sStableDB = (aTBSNode->mTBSAttr.mMemAttr.mCurrentDB + 1) % SM_PINGPONG_COUNT;

    sDBFileCount = getDbFileCount(aTBSNode, sStableDB);
    for ( i = 0; i < sDBFileCount; i++ )
    {
        if(((smmDatabaseFile*)aTBSNode->mDBFile[sStableDB][i])->isOpen()
           == ID_TRUE)
        {
            IDE_TEST(
                ((smmDatabaseFile*)aTBSNode->mDBFile[sStableDB][i])->syncUntilSuccess()
                     != IDE_SUCCESS);
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  alloc & free PCH + Page
 * ----------------------------------------------*/

IDE_RC smmManager::allocPCHEntry(smmTBSNode *  aTBSNode,
                                 scPageID      aPageID)
{

    SChar         sMutexName[128];
    smmPCH      * sCurPCH;
    scSpaceID     sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_ASSERT( sSpaceID < SC_MAX_SPACE_COUNT );

    IDE_DASSERT( isValidPageID( sSpaceID, aPageID ) == ID_TRUE );

    IDE_ASSERT(mPCArray[sSpaceID].mPC[aPageID].mPCH == NULL);

    /* smmManager_allocPCHEntry_alloc_CurPCH.tc */
    IDU_FIT_POINT("smmManager::allocPCHEntry::alloc::CurPCH");
    IDE_TEST( aTBSNode->mPCHMemPool.alloc((void **)&sCurPCH) != IDE_SUCCESS);
    mPCArray[sSpaceID].mPC[aPageID].mPCH = sCurPCH;

    /* ------------------------------------------------
     * [] mutex 초기화
     * ----------------------------------------------*/

    idlOS::snprintf( sMutexName,
                     128,
                     "SMMPCH_%"ID_UINT32_FMT"_%"ID_UINT32_FMT"_MUTEX",
                     (UInt) sSpaceID,
                     (UInt) aPageID );

    IDE_TEST(sCurPCH->mMutex.initialize( sMutexName,
                                         IDU_MUTEX_KIND_NATIVE,
                                         IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);


    idlOS::snprintf( sMutexName,
                     128,
                     "SMMPCH_%"ID_UINT32_FMT"_%"ID_UINT32_FMT"_PAGE_MEMORY_LATCH",
                     (UInt) sSpaceID,
                     (UInt) aPageID );

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_ASSERT( sCurPCH->mPageMemLatch.initialize(  
                                      sMutexName,
                                      IDU_LATCH_TYPE_NATIVE )
                == IDE_SUCCESS );

    sCurPCH->m_dirty           = ID_FALSE;
    sCurPCH->m_dirtyStat       = SMM_PCH_DIRTY_STAT_INIT;
    sCurPCH->m_pnxtDirtyPCH    = NULL;
    sCurPCH->m_pprvDirtyPCH    = NULL;
    sCurPCH->mNxtScanPID       = SM_NULL_PID;
    sCurPCH->mPrvScanPID       = SM_NULL_PID;
    sCurPCH->mModifySeqForScan = 0;
    sCurPCH->mSpaceID          = sSpaceID;
    mPCArray[sSpaceID].mPC[aPageID].mPagePtr = NULL;

    // smmPCH.mFreePageHeader 초기화
    IDE_TEST( smLayerCallback::initializeFreePageHeader( sSpaceID, aPageID )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}
/* PCH 해제
 *
 * aPID      [IN] PCH를 해제하고자 하는 Page ID
 * aPageFree [IN] PCH뿐만 아니라 그 안의 Page 메모리도 해제할 것인지 여부
 */

IDE_RC smmManager::freePCHEntry(smmTBSNode * aTBSNode,
                                scPageID     aPID,
                                idBool       aPageFree )
{
    smmPCH    *sCurPCH;
    scSpaceID  sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_DASSERT( isValidPageID( sSpaceID, aPID ) == ID_TRUE );

    sCurPCH = (smmPCH*)mPCArray[sSpaceID].mPC[aPID].mPCH;

    IDE_ASSERT(sCurPCH != NULL);

    // smmPCH.mFreePageHeader 해제
    IDE_TEST( smLayerCallback::destroyFreePageHeader( sSpaceID, aPID )
              != IDE_SUCCESS );

    if (aPageFree == ID_TRUE )
    {
        // Free Page라면 Page메모리가 이미 반납되어
        // 메모리를 Free하지 않아도 된다.
        // Page 메모리가 있는 경우에만 메모리를 반납한다.
        if ( mPCArray[sSpaceID].mPC[aPID].mPagePtr != NULL )
        {
            IDE_TEST( freePageMemory( aTBSNode, aPID ) != IDE_SUCCESS );
        }
    }


    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_TEST( sCurPCH->mPageMemLatch.destroy() != IDE_SUCCESS);

    IDE_TEST( sCurPCH->mMutex.destroy() != IDE_SUCCESS );

    // 현재 시스템에 존재하는 Tablespace라면
    // 제거하려는 PCH가 Dirty Page가 아니어야 한다.
    IDE_ASSERT(sCurPCH->m_dirty == ID_FALSE);
    IDE_ASSERT((sCurPCH->m_dirtyStat & SMM_PCH_DIRTY_STAT_MASK)
               == SMM_PCH_DIRTY_STAT_INIT);

    sCurPCH->m_pnxtDirtyPCH    = NULL;
    sCurPCH->m_pprvDirtyPCH    = NULL;
    sCurPCH->mNxtScanPID       = SM_NULL_PID;
    sCurPCH->mPrvScanPID       = SM_NULL_PID;
    sCurPCH->mModifySeqForScan = 0;

    IDE_TEST( aTBSNode->mPCHMemPool.memfree(sCurPCH) != IDE_SUCCESS);

    mPCArray[ sSpaceID ].mPC[aPID].mPCH = NULL;
    mPCArray[ sSpaceID ].mPC[aPID].mPagePtr   = NULL;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
     Dirty Page의 PCH에 기록된 Dirty Flag를 모두 초기화한다.

     [IN] aTBSNode - Dirty Page Flag를 모두 해제할 Tablespace Node
 */
IDE_RC smmManager::clearDirtyFlag4AllPages(smmTBSNode * aTBSNode )
{
    UInt i;
    smmPCH   *sPCH;

    IDE_DASSERT( aTBSNode != NULL );

    for (i = 0; i < aTBSNode->mDBMaxPageCount; i++)
    {
        sPCH = getPCH(aTBSNode->mTBSAttr.mID, i);

        if (sPCH != NULL)
        {
            sPCH->m_dirty = ID_FALSE;
            sPCH->m_dirtyStat = SMM_PCH_DIRTY_STAT_INIT;
        }
    }

    return IDE_SUCCESS;
}

/*
 * 데이터베이스의 PCH, Page Memory를 모두 Free한다.
 *
 * aPageFree [IN] Page Memory를 Free할 지의 여부.
 */
IDE_RC smmManager::freeAll(smmTBSNode * aTBSNode, idBool aPageFree)
{
    scPageID i;

    for (i = 0; i < aTBSNode->mDBMaxPageCount; i++)
    {
        smmPCH *sPCH = getPCH(aTBSNode->mTBSAttr.mID, i);

        if (sPCH != NULL)
        {
            IDE_TEST( freePCHEntry(aTBSNode, i, aPageFree) != IDE_SUCCESS);
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* 데이터베이스 타입에 따라 공유메모리나 일반 메모리를 페이지 메모리로 할당한다
 *
 * aPage [OUT] 할당된 Page 메모리
 */
IDE_RC smmManager::allocDynOrShm( smmTBSNode   * aTBSNode,
                                  void        ** aPageMemHandle,
                                  smmTempPage ** aPage,
                                  idBool         aIsDataPage )
{
    IDE_DASSERT( aPage != NULL );

    *aPageMemHandle = NULL;
    *aPage          = NULL;

    switch( aTBSNode->mRestoreType )
    {
        // 일반 메모리로부터 Page 메모리를 할당
        case SMM_DB_RESTORE_TYPE_DYNAMIC :
            /* smmManager_allocDynOrShm_alloc_Page.tc */
            IDU_FIT_POINT("smmManager::allocDynOrShm::alloc::Page");
            
            // BUG-47487: DATA / FLI 페이지 메모리 alloc 분리
            if ( aIsDataPage == ID_TRUE )
            {
                IDE_TEST( aTBSNode->mDynamicMemPagePool.alloc( aPageMemHandle,
                                                               (void **)aPage )
                          != IDE_SUCCESS);
            }
            else 
            {
                IDE_TEST( aTBSNode->mFLIMemPagePool.alloc( aPageMemHandle,
                                                               (void **)aPage )
                          != IDE_SUCCESS);
            }
            break;

        // 공유 메모리로부터 Page 메모리를 할당
        case SMM_DB_RESTORE_TYPE_SHM_CREATE :
        case SMM_DB_RESTORE_TYPE_SHM_ATTACH :
            IDE_TEST( smmFixedMemoryMgr::allocShmPage( aTBSNode, aPage )
                      != IDE_SUCCESS );
            break;

        // 어떠한 메모리를 사용해야 할 지 알 수 없다.
        case SMM_DB_RESTORE_TYPE_NONE :
        default :
            IDE_ASSERT( 0 );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}


/* 데이터베이스타입에따라 Page메모리를 공유메모리나 일반메모리로 해제한다.
 *
 * aPage [IN] 해제할 Page 메모리
 */
IDE_RC smmManager::freeDynOrShm( smmTBSNode  * aTBSNode,
                                 void        * aPageMemHandle,
                                 smmTempPage * aPage,
                                 idBool        aIsDataPage )
{
    IDE_DASSERT( aPage != NULL );

    switch( aTBSNode->mRestoreType )
    {
        // 일반 메모리에 Page 메모리를 해제
        case SMM_DB_RESTORE_TYPE_DYNAMIC :
            // BUG-47487: DATA / FLI 페이지 메모리 해제 분리 
            if ( aIsDataPage == ID_TRUE )
            {
                IDE_TEST( aTBSNode->mDynamicMemPagePool.memFree( aPageMemHandle )
                          != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST( aTBSNode->mFLIMemPagePool.memFree( aPageMemHandle )
                          != IDE_SUCCESS);
            }
            break;

        // 공반 메모리에 Page 메모리를 해제
        case SMM_DB_RESTORE_TYPE_SHM_CREATE :
        case SMM_DB_RESTORE_TYPE_SHM_ATTACH :
            IDE_TEST( smmFixedMemoryMgr::freeShmPage( aTBSNode, aPage )
                      != IDE_SUCCESS );
            break;

        // 어떠한 메모리를 사용해야 할 지 알 수 없다.
        case SMM_DB_RESTORE_TYPE_NONE :
        default :
            IDE_ASSERT( 0 );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/* 특정 Page의 PCH안의 Page Memory를 할당한다.
 *
 * aPID [IN] Page Memory를 할당할 Page의 ID
 */
IDE_RC smmManager::allocPageMemory( smmTBSNode * aTBSNode, scPageID aPID )
{
    scSpaceID sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_DASSERT( isValidPageID( sSpaceID, aPID ) == ID_TRUE );
    IDE_ASSERT( mPCArray[sSpaceID].mPC[aPID].mPCH != NULL );

    // Recovery중에는 Redo/Undo중 페이지 메모리가 NULL이면
    // 항상 Page메모리를 새로 할당하도록 되어있다.
    //
    // ex1> smrRecoveryMgr::redo의 SMR_LT_DIRTY_PAGE 처리부분에서
    //     SMM_PID_PTR(sArrPageID[i]) => 여기에서 메모리 할당

    // ex2> REDO가 다 끝나고 ALTER TABLE의 Undo시에
    //      Page 메모리 할당을 시도하는데
    //      이 때 free page라도 메모리가 할당되어 있을 수 있다.
    //
    // Recovery가 아닌 경우에만 확인한다.

    // BUG-23146 TC/Recovery/OnlineBackupRec/onlineBackupServerStop.sql 에서
    // 서버가 비정상 종료합니다.
    // Media Recovery에서도 page가 할당되어 있는 경우가 있습니다.
    // Restart Recovery인 경우에만 확인하지 않도록 되어있는 코드를
    // Media Recovery인 경우에도 확인하지 않도록 수정합니다.
    if ( ( smLayerCallback::isRestartRecoveryPhase() == ID_FALSE ) &&
         ( smLayerCallback::isMediaRecoveryPhase() == ID_FALSE ) )
    {
        // Page Memory가 할당되어 있지 않아야 한다.
        IDE_ASSERT( mPCArray[sSpaceID].mPC[aPID].mPagePtr == NULL );
    }

    if ( mPCArray[sSpaceID].mPC[aPID].mPagePtr == NULL )
    {
        // BUG-47487: DATA / FLI 페이지 경우 나누기 alloc 
        if ( smmExpandChunk::isFLIPageID( aTBSNode, aPID ) == ID_TRUE )
        {
            // FLI
            IDE_TEST( allocDynOrShm( aTBSNode,
                                     &((smmPCH*)mPCArray[sSpaceID].mPC[aPID].mPCH)->mPageMemHandle,
                                     (smmTempPage **) & mPCArray[sSpaceID].mPC[aPID].mPagePtr,
                                     ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            // DATA
            IDE_TEST( allocDynOrShm( aTBSNode,
                                     &((smmPCH*)mPCArray[sSpaceID].mPC[aPID].mPCH)->mPageMemHandle,
                                     (smmTempPage **) & mPCArray[sSpaceID].mPC[aPID].mPagePtr )
                      != IDE_SUCCESS );
        }
    }

#ifdef DEBUG_SMM_FILL_GARBAGE_PAGE
    idlOS::memset( sPCH->mPagePtr, 0x43, SM_PAGE_SIZE );
#endif // DEBUG


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}


/*
 * 페이지 메모리를 할당하고, 해당 Page를 초기화한다.
 * 필요한 경우, 페이지 초기화에 대한 로깅을 실시한다
 *
 * To Fix BUG-15107 Checkpoint시 Dirty Page처리도중 쓰레기 페이지 만남
 * => 페이지 메모리할당과 초기화를 묶어서 처리,
 * => smmPCHEntry의 mPageMemMutex로 checkpoint와 page할당tx간의 동시성 제어
 *
 * aTrans   [IN] 페이지 초기화 로그를 기록할 트랜잭션
 *               aTrans == NULL이면 로깅하지 않는다.
 * aPID     [IN] 페이지 메모리를 할당하고 초기화할 페이지 ID
 * aPrevPID [IN] 할당할 페이지의 이전 Page ID
 * aNextPID [IN] 할당할 페이지의 다음 Page ID
 */
IDE_RC smmManager::allocAndLinkPageMemory( smmTBSNode * aTBSNode,
                                           void     *   aTrans,
                                           scPageID     aPID,
                                           scPageID     aPrevPID,
                                           scPageID     aNextPID )
{
    UInt         sStage = 0;
    idBool       sPageIsDirty=ID_FALSE;
    smmPCH     * sPCH   = NULL;
    scSpaceID    sSpaceID = aTBSNode->mTBSAttr.mID;

    // 인자 적합성 검사 ( aTrans는 NULL일 수도 있다. )
    IDE_DASSERT( aPID != SM_NULL_PID );
    IDE_DASSERT( isValidPageID( sSpaceID, aPID ) == ID_TRUE );

#ifdef DEBUG
    if ( aPrevPID != SM_NULL_PID )
    {
        IDE_DASSERT( isValidPageID( sSpaceID, aPrevPID ) == ID_TRUE );
    }

    if ( aNextPID != SM_NULL_PID )
    {
        IDE_DASSERT( isValidPageID( sSpaceID, aNextPID ) == ID_TRUE );
    }
#endif

    sPCH = (smmPCH*)mPCArray[sSpaceID].mPC[aPID].mPCH;

    IDE_ASSERT( sPCH != NULL );

    // smrRecoveryMgr::chkptFlushMemDirtyPages 에서 smrDirtyPageList에
    // add하는 코드와 경쟁하는 페이지 메모리 뮤텍스
    IDE_TEST( sPCH->mMutex.lock( NULL ) != IDE_SUCCESS );
    sStage = 1;

    // Page Memory를 할당한다.
    IDE_TEST( allocPageMemory( aTBSNode, aPID ) != IDE_SUCCESS );

    if ( aTrans != NULL )
    {
        IDE_TEST( smLayerCallback::updateLinkAtPersPage( NULL, /* idvSQL* */
                                                         aTrans,
                                                         sSpaceID,
                                                         aPID,
                                                         SM_NULL_PID,
                                                         SM_NULL_PID,
                                                         aPrevPID,
                                                         aNextPID )
                  != IDE_SUCCESS );

    }


    sPageIsDirty = ID_TRUE;
    smLayerCallback::linkPersPage( mPCArray[sSpaceID].mPC[aPID].mPagePtr,
                                   aPID,
                                   aPrevPID,
                                   aNextPID );

    // 주의! smrRecoveryMgr::chkptFlushMemDirtyPages 에서
    // smmDirtyPageMgr이 지니는 Mutex를 먼저 잡고 PageMemMutex를 잡고 있다.
    // 만약 PageMemMutex를 먼저잡고 smmDirtyPageMgr의 Mutex를 잡으면
    // Dead Lock이 발생한다.
    //
    // Page Memory 할당과 초기화를 하는 동안 Checkpoint가 이 Page를
    // 못보게 하는 것이 mMutex의 용도이므로,
    // insDirtyPage전에 mMutex를 풀어도 관계없다.
    sStage = 0;
    IDE_TEST( sPCH->mMutex.unlock() != IDE_SUCCESS );

    sPageIsDirty = ID_FALSE;
   IDE_TEST( smmDirtyPageMgr::insDirtyPage( sSpaceID, aPID ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1:
                IDE_ASSERT( sPCH->mMutex.unlock() == IDE_SUCCESS );
            default:
                break;
        }

        if ( sPageIsDirty )
        {
            IDE_ASSERT( smmDirtyPageMgr::insDirtyPage( sSpaceID, aPID )
                        == IDE_SUCCESS );
        }
    }
    IDE_POP();


    return IDE_FAILURE;
}

/* 특정 Page의 PCH안의 Page Memory를 해제한다.
 *
 * aPID [IN] Page Memory를 반납할 Page의 ID
 */
IDE_RC smmManager::freePageMemory( smmTBSNode * aTBSNode, scPageID aPID )
{
    smmPCH     * sPCH;
    UInt         sStage = 0;
    scSpaceID    sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_DASSERT( isValidPageID( sSpaceID, aPID ) == ID_TRUE );

    sPCH = (smmPCH*)mPCArray[sSpaceID].mPC[aPID].mPCH;

    IDE_ASSERT( sPCH != NULL );

    // Free될 Page이므로 Dirty Page라고 하더라도 Disk에 내릴 필요는 없지만,
    // 현재 dirty page manager및 checkpoint 루틴이
    // 한번 추가된 Dirty Page를 dirty page list에서 제거하기 힘든 구조이다.
    //
    // Free Page의 Page 메모리는 반납하여 NULL로 세팅해두고
    // Dirty Page List에는 그대로 dirty인 상태로 둔다.
    //
    // 그러면 checkpoint시에 Dirty Page의 Page 메모리가 NULL이면,
    // Dirty Page로 등록되었다가 Free Page로 반납된 Page이므로 무시한다.
    // 나중에 restart recovery시에 해당 Page는 Free Page이므로,
    // Disk에서 읽어오지도 않기 때문에,
    // 이와같이 Dirty Page를 Flush하지 않고 무시하는 것이 가능하다.
    //
    // 주의할 점은, freePageMemory하려는 순간에
    // checkpoint가 진행중일 수 있는 사실이다.
    // sPID가 Dirty Page이어서 Dirty Page의 Page메모리를 Flush하고 있다면
    // Page 메모리를 바로 해제하면 안된다.

    IDE_TEST( sPCH->mMutex.lock( NULL /* idvSQL* */ )
              != IDE_SUCCESS );
    sStage = 1;

    // Page Memory가 할당되어 있어야 한다.
    IDE_ASSERT( mPCArray[sSpaceID].mPC[aPID].mPagePtr != NULL );

    // BUG-47487: DATA / FLI 페이지 경우 나누기 free 
    if ( smmExpandChunk::isFLIPageID( aTBSNode, aPID ) == ID_TRUE )
    {
        //FLI
        IDE_TEST( freeDynOrShm( aTBSNode,
                                sPCH->mPageMemHandle,
                                (smmTempPage*)mPCArray[sSpaceID].mPC[aPID].mPagePtr,
                                ID_FALSE )
                  != IDE_SUCCESS );
    }
    else
    {
        //DATA
        IDE_TEST( freeDynOrShm( aTBSNode,
                                sPCH->mPageMemHandle,
                                (smmTempPage*)mPCArray[sSpaceID].mPC[aPID].mPagePtr )
                  != IDE_SUCCESS );
    }

    mPCArray[sSpaceID].mPC[aPID].mPagePtr = NULL;

    // checkpoint시에 smmDirtyPageList -> smrDirtyPageList 로 Dirty Page를
    // 이동시키는데, 만약 이때 mPagePtr == NULL이면, 이동이 되지 않는다.
    // 그럴 경우, smmDirtyPageList에 추가된 Page이므로,
    // m_dirty == ID_TRUE이지만, smrDirtyPageList에는 추가되지 않은 채로
    // 방치된다.
    //
    // 그리고 해당 Page가 사용되어서 mPagePtr != NULL인 채로 smmDirtyPageList에
    // 넣으려고 해도, m_dirty 가 ID_TRUE여서 Dirty Page로 추가가 되지 않는다.
    //
    // 페이지에 메모리가 해제되어 mPagePtr가 NULL로 되는 시점에서
    // m_dirty 플래그를 ID_FALSE로 세팅하여 추후 페이지에 메모리가 할당되고,
    // smmDirtyPageList에 페이지를 추가하고자 할 때, m_dirty 가 ID_TRUE여서
    // 추가되지 않는 문제점을 미연에 방지한다.
    sPCH->m_dirty = ID_FALSE;

    sStage = 0;
    IDE_TEST( sPCH->mMutex.unlock() != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1:
                IDE_ASSERT( sPCH->mMutex.unlock() == IDE_SUCCESS );
            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}



/*
 * Page의 PCH(Page Control Header)정보를 구성한다.
 *
 * aPID  [IN] PCH Entry를 구성하려고 하는 Page ID
 * aOP   [IN] 페이지 데이터를 복사할지 아니면 메모리 포인터를 세팅할지 여부
 * aPage [IN] 디스크로부터 읽어들인 페이지 데이터
 */
IDE_RC smmManager::fillPCHEntry( smmTBSNode *       aTBSNode,
                                 scPageID           aPID,
                                 smmFillPCHOption   aFillOption,
                                                   /* =SMM_FILL_PCH_OP_NONE*/
                                 void             * aPage /* =NULL */ )
{
    smmPCH     * sPCH;
#ifdef DEBUG
    idBool   sIsFreePage;
#endif
    scSpaceID  sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_DASSERT( isValidPageID( sSpaceID, aPID ) == ID_TRUE );

    sPCH = (smmPCH*)mPCArray[sSpaceID].mPC[aPID].mPCH;

    if ( sPCH == NULL )
    {
        IDE_TEST( allocPCHEntry( aTBSNode, aPID ) != IDE_SUCCESS );
        sPCH = (smmPCH*)mPCArray[sSpaceID].mPC[aPID].mPCH;

        IDE_ASSERT( sPCH != NULL );

        // 페이지 데이터가 NULL이 아니라면 Restore중임을 뜻한다.
        // Restore는 디크스로부터 메모리로 Page를 읽어들이는 과정을 말한다.
        if ( aPage != NULL )
        {
            /* BUG-43789 Restore시 FPLIP에 Allocated 되었다고 되어 있으나
             * 해당하는 Page가 Flush 되지 않은 경우 읽은 페이지가 빈 Page일 수 있다.
             */
            if ( ((smpPersPageHeader*)aPage)->mSelfPageID == aPID )
            {
                switch ( aFillOption )
                {
                    case SMM_FILL_PCH_OP_COPY_PAGE :

                        IDE_TEST( allocPageMemory(aTBSNode, aPID ) != IDE_SUCCESS );
                        idlOS::memcpy( mPCArray[sSpaceID].mPC[aPID].mPagePtr, aPage, SM_PAGE_SIZE );
                        break;
                    case SMM_FILL_PCH_OP_SET_PAGE :
                        mPCArray[sSpaceID].mPC[aPID].mPagePtr = aPage;
                        break;
                        // 페이지 데이터가 NULL이 아닌 경우에는
                        // aFillOption이 항상 COPY_PAGE나 SET_PAGE중
                        // 하나의 경우여야 한다.
                    case SMM_FILL_PCH_OP_NONE :
                    default:
                        IDE_ASSERT(0);
                }
            }
            else
            {
                /* BUG-43789 Page Header의 ID와 요청한 PageID가 다를경우
                 * 해당 Page를 그대로 읽어서는 안된다.
                 * Page Memory를 새로 할당하여 Page Header의 SelfID만을 초기화 하여주면
                 * Recovery 단계에서 정상적인 Page로 복원될 것이다. */
                IDE_TEST( allocPageMemory( aTBSNode, aPID ) != IDE_SUCCESS );

                /* BUG-44136 Memory Page loading 시 flush되지 않은 FLI Page를
                 *           새로 할당 했다면 초기화 해야 합니다.
                 * Meta, FLI Page는 recovery전에 참고하는 경우가 있다.
                 * allocPageMemory는 mempool에서 가져오므로 쓰레기 값이 들어있다.
                 * 최소한 그 쓰레기 값들은 지워 두어야 한다.
                 */
                if( smmExpandChunk::isDataPageID( aTBSNode, aPID ) != ID_TRUE )
                {
                    idlOS::memset(  mPCArray[sSpaceID].mPC[aPID].mPagePtr , 0x0, SM_PAGE_SIZE );
                }

                smLayerCallback::linkPersPage(  mPCArray[sSpaceID].mPC[aPID].mPagePtr ,
                                               aPID,
                                               SM_NULL_PID,
                                               SM_NULL_PID );
            }
        }
        else
        {
            IDE_TEST( allocPageMemory( aTBSNode, aPID ) != IDE_SUCCESS );
        }
    }
    else // sPCH != NULL
    {
        // PCH가 차 있는 경우 Page 데이터를 처리할 수 없다.
        // Page 데이터를 설정하려는 경우에는 항상 PCH가 NULL이어야 함
        IDE_ASSERT( aPage == NULL );

#ifdef DEBUG
        IDE_TEST( smmExpandChunk::isFreePageID (aTBSNode,
                                                aPID,
                                                & sIsFreePage )
                  != IDE_SUCCESS );

        if ( sIsFreePage == ID_FALSE )
        {
            // PCH가 NULL이 아니면서 PCH의 Page가 NULL이라면 서버를 죽인다.
            IDE_ASSERT( mPCArray[sSpaceID].mPC[aPID].mPagePtr != NULL );
        }
#endif
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}



/*
 * 하나의 Page의 데이터가 저장되는 메모리 공간을 PCH Entry에서 가져온다.
 *
 * aPID [IN] 페이지의 ID
 * return Page의 데이터가 저장되는 메모리 공간
 */
/* BUG-32479 [sm-mem-resource] refactoring for handling exceptional case about
 * the SMM_OID_PTR and SMM_PID_PTR macro. */
IDE_RC smmManager::getPersPagePtrErr( scSpaceID    aSpaceID, 
                                      scPageID     aPID,
                                      void      ** aPersPagePtr )
{
    idBool       sIsFreePage ;
    smmTBSNode * sTBSNode = NULL;
    smmPCH     * sPCH;
    svmTBSNode * sVolTBSNode;
    IDE_RC       rc;

    // BUG-14343
    // DB 크기를 줄이기 위해서 free page는 loading 하지 않았다.
    // 때문에 recovery phase에서는 free page 접근시 page 메모리를 할당해 주어야 한다.
    /*
        Free List Info Page에 Free Page로 지정되어 있어서
        Restart Recovery이전에 Page가 Load되지 않았으나,
        Redo도중 해당 Page를 만나는 경우 Disk로부터 Page를 Load할 필요가 없다.

        Redo이전 Load DB시에 Page가 Free Page여서 Load되지 않은 경우 :

        1. 해당 Page가 Redo완료 후에도 Free Page라면
            Redo완료후 해당 Page안의 내용은 쓰레기 값이 들어있어도 상관없으며
           해당 Page의 Header만 온전한 값으로 설정되어 있으면 된다.

        2. 해당 Page가 Redo완료 후에 Alloced Page라면
           Checkpoint시 해당 Page를 alloced page라는 내용을 지닌
           Dirty Page가 Disk에 내려가지 않은 상태이므로
           Free => alloced page로 변경되는 로그가 redo대상에 포함된다.
           Disk로부터 Page를 로드하지 않아도 redo에 의해
           Page Image를 만들어 내게 된다.

           위 두 가지 상황 모두 Page를 Disk로부터 로드할 필요가 없다.
           하지만, Page Memory는 할당되어야 하며, Page Header도
           온전한 값으로 데이터가 설정되어 있어야 한다.

           (ex> Page Header의 mSelf가 설정되어 있어야 해당 Page에 대한
                Redo가 정상 동작함을 보장할 수 있음
                => Redo루틴에서 Page Header의 mSelf를 사용하기 때문 )
    */

    // BUG-31191 isRestart()->isRestartRecoveryPhase()로 변경
    if ( (smLayerCallback::isRestartRecoveryPhase() == ID_TRUE ) ||
         (smLayerCallback::isMediaRecoveryPhase() == ID_TRUE) )
    {
        IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                            (void**)&sTBSNode ) 
                  != IDE_SUCCESS );
        IDE_ERROR( sTBSNode != NULL );
        IDE_ERROR_MSG( mPCArray[aSpaceID].mPC != NULL,
                       "aSpaceID : %"ID_UINT32_FMT,
                       aSpaceID );
        IDE_ERROR_MSG( mPCArray[aSpaceID].mPC[aPID].mPCH != NULL,
                       "aSpaceID : %"ID_UINT32_FMT"\n"
                       "aPID     : %"ID_UINT32_FMT"\n",
                       aSpaceID,
                       aPID );

        if ( mPCArray[aSpaceID].mPC[aPID].mPagePtr == NULL )
        {
            IDE_TEST( allocAndLinkPageMemory( sTBSNode,
                                              NULL, // 로깅하지 않음
                                              aPID,          // PID
                                              SM_NULL_PID,   // prev PID
                                              SM_NULL_PID )  // next PID
                      != IDE_SUCCESS );
        }
    }

    if( isValidPageID( aSpaceID, aPID ) == ID_FALSE )
    {
        ideLog::log( IDE_SERVER_0,
                     SM_TRC_PAGE_PID_INVALID,
                     aSpaceID,
                     aPID,
                     mPCArray[aSpaceID].mMaxPageCount );
        IDE_ERROR( 0 );
    }

    sPCH = (smmPCH*)mPCArray[aSpaceID].mPC[aPID].mPCH;

    if ( sPCH == NULL )
    {
        /* PCH가 아직 할당되지 못했다면, Utility에서 사용하는 것일 확율
         * 이 있음. 따라서 Callback이 있으면, callback으로 호출 시도해봄*/
        if( mGetPersPagePtrFunc != NULL )
        {
            IDE_TEST( mGetPersPagePtrFunc ( aSpaceID,
                                            aPID,
                                            aPersPagePtr ) 
                      != IDE_SUCCESS );

            IDE_CONT( RETURN_SUCCESS );
        }
        else
        {
            /*Noting to do ...*/
        }
 
        IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                            (void**)&sTBSNode )
                  != IDE_SUCCESS );
        IDE_ERROR( sTBSNode != NULL );

        ideLog::log(IDE_ERR_0,
                    "aSpaceID : %"ID_UINT32_FMT"\n"
                    "aPID     : %"ID_UINT32_FMT"\n",
                    aSpaceID,
                    aPID );

        ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                    SM_TRC_MEMORY_PCH_ARRAY_NULL1,
                    (ULong)aPID);

        ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                    SM_TRC_MEMORY_PCH_ARRAY_NULL2,
                    (ULong)mPCArray[aSpaceID].mMaxPageCount );

        if ( sctTableSpaceMgr::isMemTableSpace( sTBSNode ) == ID_TRUE )
        {
            if ( sTBSNode->mMemBase != NULL )
            {
                ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                            SM_TRC_MEMORY_PCH_ARRAY_NULL3,
                            (ULong)sTBSNode->mMemBase->mAllocPersPageCount);
            }

            rc = smmExpandChunk::isFreePageID(sTBSNode, aPID, &sIsFreePage);
        }
        else
        {
            sVolTBSNode = (svmTBSNode*)sTBSNode;

            ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                        SM_TRC_MEMORY_PCH_ARRAY_NULL3,
                        (ULong)sVolTBSNode->mMemBase.mAllocPersPageCount);

            rc = svmExpandChunk::isFreePageID(sVolTBSNode, aPID, &sIsFreePage);
        }

        if ( rc == IDE_SUCCESS )
        {
            if (sIsFreePage == ID_TRUE)
            {
                ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                            SM_TRC_MEMORY_PCH_ARRAY_NULL4,
                            (ULong)aPID);
            }
            else
            {
                ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                            SM_TRC_MEMORY_PCH_ARRAY_NULL5,
                            (ULong)aPID);
            }
        }
        else
        {
            ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                        SM_TRC_MEMORY_PCH_ARRAY_NULL6);
        }

        IDE_ERROR( 0 );     // BUG-41347
    }
    
    /* BUGBUG: by newdaily
     * To Trace BUG-15969 */
    (*aPersPagePtr) = mPCArray[aSpaceID].mPC[aPID].mPagePtr;

    IDE_ERROR_MSG( (*aPersPagePtr) != NULL,
                   "aSpaceID : %"ID_UINT32_FMT"\n"
                   "aPID     : %"ID_UINT32_FMT"\n",
                   aSpaceID,
                   aPID );

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-41149
    // ASSERT를 제거 하고, trc 로그를 추가한다.
    ideLog::logCallStack( IDE_ERR_0 );

    return IDE_FAILURE;
}

/*
 * 데이터베이스 File을 Open하고, 데이터베이스 파일 객체를 리턴한다
 *
 * aStableDB [IN] Ping/Pong DB 지정 ( 0이나 1 )
 * aDBFileNo [IN] 데이터베이스 파일 번호 ( 0 부터 시작 )
 * aDBFile   [OUT] 데이터베이스 파일 객체
 */
IDE_RC smmManager::openAndGetDBFile( smmTBSNode *      aTBSNode,
                                     SInt              aStableDB,
                                     UInt              aDBFileNo,
                                     smmDatabaseFile **aDBFile )
{

    IDE_DASSERT( aStableDB == 0 || aStableDB == 1 );
    IDE_DASSERT( aDBFileNo < aTBSNode->mHighLimitFile );
    IDE_DASSERT( aDBFile != NULL );

    IDE_TEST( getDBFile( aTBSNode,
                         aStableDB,
                         aDBFileNo,
                         SMM_GETDBFILEOP_SEARCH_FILE,
                         aDBFile )
              != IDE_SUCCESS );

    IDE_DASSERT( *aDBFile != NULL );

    if( (*aDBFile)->isOpen() != ID_TRUE )
    {
        IDE_TEST( (*aDBFile)->open() != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * 데이터베이스 파일 객체를 리턴한다.
 * ( 필요하다면 모든 DB 디렉토리에서 DB파일을 찾는다 )
 *
 * aPingPongDBNum [IN] Ping/Pong DB 지정 ( 0이나 1 )
 * aDBFileNo      [IN] 데이터베이스 파일 번호
 * aOp            [IN] getDBFile 옵션
 * aDBFile        [OUT] 데이터베이스 파일 객체
 */
IDE_RC smmManager::getDBFile( smmTBSNode *       aTBSNode,
                              UInt               aPingPongDBNum,
                              UInt               aDBFileNo,
                              smmGetDBFileOption aOp,
                              smmDatabaseFile ** aDBFile )
{

    SChar   sDBFileName[SM_MAX_FILE_NAME];
    SChar  *sDBFileDir;
    idBool  sFound;

    //BUG-27610	CodeSonar::Type Underrun, Overrun (2)
    IDE_ASSERT(  aPingPongDBNum <  SM_DB_DIR_MAX_COUNT );
    IDE_DASSERT( aDBFile != NULL );

    // 모든 MEM_DB_DIR에서 데이터베이스 파일을 찾아야 하는지 여부
    if( aOp == SMM_GETDBFILEOP_SEARCH_FILE )
    {
        sFound =  smmDatabaseFile::findDBFile( aTBSNode,
                                               aPingPongDBNum,
                                               aDBFileNo,
                                               (SChar*)sDBFileName,
                                               &sDBFileDir);

        IDE_TEST_RAISE( sFound != ID_TRUE,
                        file_exist_error );

        ((smmDatabaseFile*)aTBSNode->mDBFile[aPingPongDBNum][aDBFileNo])->setFileName(sDBFileName);
        ((smmDatabaseFile*)aTBSNode->mDBFile[aPingPongDBNum][aDBFileNo])->setDir(sDBFileDir);
    }

    /* BUG-32214 [sm] when server start to check the db file size.
     * cause of buffer overflow 
     * Table의 File 개수가 최대치를 넘어갔음. MEM_MAX_DB_SIZE가 작기 때문. */
    IDE_TEST_RAISE( aDBFileNo >= aTBSNode->mHighLimitFile ,
                    error_invalid_mem_max_db_size );

    *aDBFile = (smmDatabaseFile*)aTBSNode->mDBFile[aPingPongDBNum][aDBFileNo];

    return IDE_SUCCESS;

    IDE_EXCEPTION(file_exist_error);
    {
        SChar sErrorFile[SM_MAX_FILE_NAME];

        idlOS::snprintf( (SChar*)sErrorFile, SM_MAX_FILE_NAME, "%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT"",
                         aTBSNode->mHeader.mName,
                         aPingPongDBNum,
                         aDBFileNo );

        IDE_SET( ideSetErrorCode( smERR_ABORT_NoExistFile,
                                  (SChar*)sErrorFile ) );
    }
    IDE_EXCEPTION( error_invalid_mem_max_db_size );
    {
        SChar sErrorFile[SM_MAX_FILE_NAME];

        idlOS::snprintf( (SChar*)sErrorFile, SM_MAX_FILE_NAME, "%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT"",
                         aTBSNode->mHeader.mName,
                         aPingPongDBNum,
                         aDBFileNo );

        IDE_SET( ideSetErrorCode( smERR_ABORT_DB_FILE_SIZE_EXCEEDS_LIMIT,
                                  (SChar*)sErrorFile ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smmManager::openOrCreateDBFileinRecovery( smmTBSNode * aTBSNode,
                                                 SInt         aDBFileNo,
                                                 idBool     * aIsCreated )
{
    SInt             sCurrentDB;
    smmDatabaseFile *sDBFile;
    IDE_RC           sSuccess;

    IDE_ASSERT( aIsCreated != NULL );

    for ( sCurrentDB=0 ; sCurrentDB<SMM_PINGPONG_COUNT; sCurrentDB++ )
    {
        sSuccess = getDBFile( aTBSNode,
                              sCurrentDB,
                              aDBFileNo,
                              SMM_GETDBFILEOP_SEARCH_FILE,
                              &sDBFile );

        if( sSuccess != IDE_SUCCESS )
        {
            IDE_TEST(((smmDatabaseFile*)aTBSNode->mDBFile[sCurrentDB][aDBFileNo])->createDbFile(
                         aTBSNode,
                         sCurrentDB,
                         aDBFileNo,
                         0/* DB File Header만 기록*/)
                     != IDE_SUCCESS);

            // fix BUG-17513
            // restart recovery완료이후 loganchor resorting과정에서
            // 새로 생성된 Memory DBF에 대한 정보가 저장되지않는 경우 발생

            // create tablespace 에 의해 파일이 생성되는 경우에도
            // mLstCreatedDBFile을 계산해준다.
            if ( (UInt)aDBFileNo > aTBSNode->mLstCreatedDBFile )
            {
                aTBSNode->mLstCreatedDBFile = (UInt)aDBFileNo;
            }
            *aIsCreated = ID_TRUE;
        }
        else
        {
            if (sDBFile->isOpen() != ID_TRUE)
            {
                IDE_TEST(sDBFile->open() != IDE_SUCCESS);
            }
            *aIsCreated = ID_FALSE;
        }
    } // for

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Disk에 존재하는 데이터베이스 Page의 수를 계산한다
 * Performance View에 통계값으로 Reporting할 용도로 사용되므로
 * 정확할 필요는 없다.
 *
 * aCurrentDB [IN] 현재 사용중인 Ping/Pong데이터베이스 중 하나 ( 0 or 1 )
 */
IDE_RC smmManager::calculatePageCountInDisk( smmTBSNode * aTBSNode )
{

    UInt             s_DbFileCount;

    s_DbFileCount = getDbFileCount(aTBSNode,
                                   aTBSNode->mTBSAttr.mMemAttr.mCurrentDB);

    aTBSNode->mDBPageCountInDisk =
        s_DbFileCount * aTBSNode->mMemBase->mDBFilePageCount +
        SMM_DATABASE_META_PAGE_CNT;

    return IDE_SUCCESS;

}

/*
   메모리 데이타파일 생성 Runtime 정보 구조체 초기화
*/
void smmManager::initCrtDBFileInfo( smmTBSNode * aTBSNode )
{
    UInt i;
    UInt j;

    IDE_DASSERT( aTBSNode != NULL );

    for ( i = 0; i < aTBSNode->mHighLimitFile; i ++ )
    {
        for (j = 0; j < SMM_PINGPONG_COUNT; j++)
        {
            (aTBSNode->mCrtDBFileInfo[ i ]).mCreateDBFileOnDisk[ j ]
                = ID_FALSE;
        }

        (aTBSNode->mCrtDBFileInfo[ i ]).mAnchorOffset
            = SCT_UNSAVED_ATTRIBUTE_OFFSET;
    }

    return;
}

/*
   주어진 파일번호에 해당하는 DBF가 하나라도 생성이 되었는지 여부반환

   [ 관련버그 ]
   fix BUG-17343
   loganchor에 Stable/Unstable Chkpt Image에 대한 생성 정보를 저장
*/
idBool smmManager::isCreateDBFileAtLeastOne(
                     idBool    * aCreateDBFileOnDisk )
{
    UInt    i;
    idBool  sIsCreate;

    IDE_DASSERT( aCreateDBFileOnDisk != NULL );

    sIsCreate = ID_FALSE;

    for ( i = 0 ; i < SMM_PINGPONG_COUNT; i++ )
    {
       if ( aCreateDBFileOnDisk[ i ] == ID_TRUE )
       {
           sIsCreate = ID_TRUE;
           break;
       }
    }

    return sIsCreate;
}


IDE_RC smmManager::initSCN()
{
    smmTBSNode * sTBSNode;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  (void**)&sTBSNode ) != IDE_SUCCESS );
    IDE_ASSERT(sTBSNode != NULL);

    SM_ADD_SCN( &(sTBSNode->mMemBase->mSystemSCN),
                smuProperty::getSCNSyncInterval() );

    IDE_TEST( smLayerCallback::setSystemSCN( sTBSNode->mMemBase->mSystemSCN )
              != IDE_SUCCESS );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                             SMM_MEMBASE_PAGEID ) 
              != IDE_SUCCESS );

    SM_SET_SCN(&smmDatabase::mLstSystemSCN, &sTBSNode->mMemBase->mSystemSCN);

    smmDatabase::makeMembaseBackup();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC smmManager::loadParallel(smmTBSNode     * aTBSNode)
{
    SInt            sThrCnt;
    UInt            sState      = 0;
    smmPLoadMgr   * sLoadMgr    = NULL;

    /* TC/FIT/Limit/sm/smm/smmManager_loadParallel_malloc.sql */
    IDU_FIT_POINT_RAISE( "smmManager::loadParallel::malloc",
                          insufficient_memory );
	
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMM,
                               ID_SIZEOF(smmPLoadMgr),
                               (void**)&(sLoadMgr)) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 1;

    sLoadMgr = new (sLoadMgr) smmPLoadMgr();

    sThrCnt = smuProperty::getRestoreThreadCount();

    IDE_TEST(sLoadMgr->initializePloadMgr(aTBSNode,
                                          sThrCnt)
             != IDE_SUCCESS);

    IDE_TEST(sLoadMgr->start() != IDE_SUCCESS);
    IDE_TEST(sLoadMgr->waitToStart() != IDE_SUCCESS);

    IDE_TEST(sLoadMgr->join() != IDE_SUCCESS);

    IDE_TEST(sLoadMgr->destroy() != IDE_SUCCESS);

    /* BUG-40933 thread 한계상황에서 FATAL에러 내지 않도록 수정
     * thread를 하나도 생성하지 못하여 ABORT된 경우에
     * smmPLoadMgr thread join 후 그 결과를 확인하여
     * ABORT에러를 낼 수 있도록 한다. */
    IDE_TEST(sLoadMgr->getResult() == ID_FALSE);

    IDE_TEST(iduMemMgr::free(sLoadMgr) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sLoadMgr ) == IDE_SUCCESS );
            sLoadMgr = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}


// Base Page ( 0번 Page ) 에 Latch를 건다
// 0번 Page를 변경하는 Transaction들이 없음을 보장한다.
IDE_RC smmManager::lockBasePage(smmTBSNode * aTBSNode)
{
    IDE_TEST( smmFPLManager::lockAllocChunkMutex(aTBSNode) != IDE_SUCCESS );

    IDE_TEST( smmFPLManager::lockAllFPLs(aTBSNode) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Fatal Error at smmManager::lockBasePage");

    return IDE_FAILURE;
}

// Base Page ( 0번 Page ) 에서 Latch를 푼다.
// lockBasePage로 잡은 Latch를 모두 해제한다
IDE_RC smmManager::unlockBasePage(smmTBSNode * aTBSNode)
{
    IDE_TEST( smmFPLManager::unlockAllFPLs(aTBSNode) != IDE_SUCCESS);
    IDE_TEST( smmFPLManager::unlockAllocChunkMutex(aTBSNode) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Fatal Error at smmManager::unlockBasePage");

    return IDE_FAILURE;
}




/* 특정 페이지 범위만큼 데이터베이스를 확장한다.
 *
 * aTrans 가 NULL로 들어오면, 이 때는 Restart Redo에서 불리운 것으로
 * Normal Processing때와 비교했을 때 다음과 같은 점에서 다르게 작동한다.
 *    1. Logging 할 필요가 없음
 *    2. 할당할 Page Count의 영역을 검사할 필요가 없음
 *       by gamestar 2001/05/24
 *
 * 모든 Free Page List에 대해 Latch가 잡히지 않은 채로 이 함수가 호출된다.
 *
 * aTrans            [IN] 데이터베이스를 확장하려는 트랜잭션
 *                        CreateDB 혹은 logical redo중인경우 NULL로 들어온다.
 * aNewChunkFirstPID [IN] 확장할 데이터베이스 Expand Chunk의 첫번째 Page ID
 * aNewChunkFirstPID [IN] 확장할 데이터베이스 Expand Chunk의 마지막 Page ID
 */
IDE_RC smmManager::allocNewExpandChunk4MR( smmTBSNode * aTBSNode,
                                           scPageID     aNewChunkFirstPID,
                                           scPageID     aNewChunkLastPID,
                                           idBool       aSetFreeListOfMembase,
                                           idBool       aSetNextFreePageOfFPL )
{
    UInt              sStage;
    UInt              sNewDBFileCount;
    UInt              sArrFreeListCount;
    smmPageList       sArrFreeList[ SMM_MAX_FPL_COUNT ];

    IDE_DASSERT( aTBSNode   != NULL );

    sStage = 0;

    // BUGBUG kmkim 에러처리로 변경. 사용자가 프로퍼티 잘못 바꾸었을때
    // 바로 알려줄 수 있도록.
    // 지금은 사용자가 PAGE수 제한을 줄인후 Restart Recovery하면
    // 여기서 죽게된다.
    IDE_ASSERT( aNewChunkFirstPID < aTBSNode->mDBMaxPageCount );
    IDE_ASSERT( aNewChunkLastPID < aTBSNode->mDBMaxPageCount );

    // 모든 Free Page List의 Latch획득
    IDE_TEST( smmFPLManager::lockAllFPLs( aTBSNode ) != IDE_SUCCESS );
    sStage = 1;

    if ( aSetNextFreePageOfFPL == ID_TRUE )
    {
        // 하나의 Expand Chunk에 속하는 Page들의 PCH Entry들을 구성한다.
        IDE_TEST( fillPCHEntry4AllocChunk( aTBSNode,
                                           aNewChunkFirstPID,
                                           aNewChunkLastPID )
                  != IDE_SUCCESS );
    }
    else
    {
        // Chunk 복구가 필요없는 경우 PCHEntry에 page를 할당할
        // 필요가 없다.
    }

    IDE_ASSERT( aTBSNode->mMemBase != NULL );

    sArrFreeListCount = aTBSNode->mMemBase->mFreePageListCount;

    // Logical Redo 될 것이므로 Physical Update( Next Free Page ID 세팅)
    // 에 대한 로깅을 하지 않음.
    IDE_TEST( smmFPLManager::distributeFreePages(
                    aTBSNode,
                    aNewChunkFirstPID +
                    smmExpandChunk::getChunkFLIPageCnt(aTBSNode),
                    aNewChunkLastPID,
                    aSetNextFreePageOfFPL,
                    sArrFreeListCount,
                    sArrFreeList  )
                != IDE_SUCCESS );
    sStage = 2;

    // 주의! smmUpdate::redo_SMMMEMBASE_ALLOC_EXPANDCHUNK 에서
    // membase 멤버들을 Logical Redo하기 전의 값으로 세팅해놓고 이 루틴으로
    // 들어와야 한다.

    // 지금까지 데이터베이스에 할당된 총 페이지수 변경
    aTBSNode->mMemBase->mAllocPersPageCount = aNewChunkLastPID + 1;
    aTBSNode->mMemBase->mCurrentExpandChunkCnt ++ ;

    // Chunk가 새로 할당된 후 DB File이 몇개가 더 생겨야 하는지 계산
    IDE_TEST( calcNewDBFileCount( aTBSNode,
                                  aNewChunkFirstPID,
                                  aNewChunkLastPID,
                                  & sNewDBFileCount )
            != IDE_SUCCESS );

    // DB File 수 변경
    aTBSNode->mMemBase->mDBFileCount[0]    += sNewDBFileCount;
    aTBSNode->mMemBase->mDBFileCount[1]    += sNewDBFileCount;


    // Logical Redo할 것이므로 Phyical Update에 대해 로깅하지 않는다.
    IDE_TEST( smmFPLManager::appendPageLists2FPLs(
                                      aTBSNode,
                                      sArrFreeList,
                                      aSetFreeListOfMembase,
                                      aSetNextFreePageOfFPL )
              != IDE_SUCCESS );

    if ( aSetFreeListOfMembase == ID_TRUE )
    {
        sStage = 1;
        IDE_TEST( smmDirtyPageMgr::insDirtyPage( aTBSNode->mTBSAttr.mID,
                                                 SMM_MEMBASE_PAGEID)
                  != IDE_SUCCESS);
    }
    else
    {
        // Membase 복구가 필요없는 경우
    }

    sStage = 0;
    IDE_TEST( smmFPLManager::unlockAllFPLs(aTBSNode) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sStage )
    {
        case 2 :
            if ( aSetFreeListOfMembase == ID_TRUE )
            {
                IDE_ASSERT( smmDirtyPageMgr::insDirtyPage( aTBSNode->mTBSAttr.mID,
                                                           SMM_MEMBASE_PAGEID)
                            == IDE_SUCCESS);
            }

        case 1 :
            IDE_ASSERT( smmFPLManager::unlockAllFPLs(aTBSNode) == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}


IDE_RC smmManager::setSystemStatToMemBase( smiSystemStat * aSystemStat )
{
    smmMemBase  * sMemBase;

    sMemBase = smmDatabase::getDicMemBase();

    IDE_DASSERT( sMemBase != NULL );

    sMemBase->mSystemStat.mCreateTV            =   aSystemStat->mCreateTV;
    sMemBase->mSystemStat.mSReadTime           =   aSystemStat->mSReadTime;
    sMemBase->mSystemStat.mMReadTime           =   aSystemStat->mMReadTime;
    sMemBase->mSystemStat.mDBFileMultiPageReadCount
                                    = aSystemStat->mDBFileMultiPageReadCount;
    sMemBase->mSystemStat.mHashTime            =   aSystemStat->mHashTime;
    sMemBase->mSystemStat.mCompareTime         =   aSystemStat->mCompareTime;
    sMemBase->mSystemStat.mStoreTime           =   aSystemStat->mStoreTime;

    IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                             SMM_MEMBASE_PAGEID )
                              != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smmManager::getSystemStatFromMemBase( smiSystemStat * aSystemStat )
{
    smmMemBase  * sMemBase;

    sMemBase = smmDatabase::getDicMemBase();

    IDE_DASSERT( sMemBase != NULL );

    aSystemStat->mCreateTV      =   sMemBase->mSystemStat.mCreateTV;
    aSystemStat->mSReadTime     =   sMemBase->mSystemStat.mSReadTime;
    aSystemStat->mMReadTime     =   sMemBase->mSystemStat.mMReadTime;
    aSystemStat->mDBFileMultiPageReadCount  =
                    sMemBase->mSystemStat.mDBFileMultiPageReadCount;
    aSystemStat->mHashTime      =   sMemBase->mSystemStat.mHashTime;
    aSystemStat->mCompareTime   =   sMemBase->mSystemStat.mCompareTime;
    aSystemStat->mStoreTime     =   sMemBase->mSystemStat.mStoreTime;

    return IDE_SUCCESS;
}
