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
 * $Id: sddDiskMgr.cpp 91197 2021-07-12 01:15:29Z emlee $
 *
 * Description :
 *
 * 본 파일은 디스크관리자에 대한 구현파일이다.
 *
 * 디스크관리자는 버퍼관리자와 함께 resource layer에 속한다. 대부분
 * 버퍼관리자가 디스크관리자를 호출하며, I/O 완료시에는 버퍼 관리자에게
 * 완료 사실을 알려줄 필요가 있다.
 *
 * # 개념
 *
 * 테이블스페이스 정보, 각 테이블스페이스의 데이타 화일에 대한 정보,
 * 데이타 화일이 I/O에 관련된 정보를 관리한다.
 *
 * # 구성요소
 *
 * 1. sddDiskMgr
 *   tablespace의 hash와 각종 리스트를 유지하며, 전체 I/O에 대한
 *   정보를 관리한다.
 *
 * 2. sddTableSpaceNode
 *   tablespace 노드에 관련된 정보를 관리한다.
 *
 * 3. sddDataFileNode
 *   하나의 datafile 노드에 대한 정보를 관리한다.
 *
 * - sddDiskMgr
 *                 ____________
 *          _______|Hash Table|
 *          |      |__________|
 *  ________|_____ ___________________  ___________________
 *  |sddDiskMgr  |_|sddTableSpaceNode|--|sddTableSpaceNode|
 *  |____________| |_________________|  |_________________|
 *          |      _________________  _________________
 *          |______|sddDataFileNode|__|sddDataFileNode|
 *        LRU-list |_______________|  |_______________|
 *
 * - sddTableSpaceNode
 *  ___________________ __________________  _________________
 *  |sddTableSpaceNode|_|sddDataFileNode |__|sddDataFileNode|
 *  |_________________| |________________|  |_______________|
 *
 *  - sddDataFileNode LRU-list
 * 최대 열 수 있는 datafile의 개수가 정해져 있으므로, 오픈된 datafile의
 * 리스트를 LRU로 유지한다. 리스트의 마지막이 가장 오래전에 오픈된
 * datafile이다.
 *
 *  - sddTableSpaceNode List
 * 현재 데이타베이스에서 사용중인 모든 테이블스페이스에 대한 리스트이다.
 * 시스템 테이블스페이스, 유저 테이블스페이스를 포함한다.
 *
 *  - sddTableSpaceNode Hash
 * 모든 tablespace를 접근하기 위한 해쉬구조
 *
 * # 디스크관리자의 Mutex
 *
 * 디스크관리자의 mutex를 가지며 list와 hash에 대한 접근을 보호한다.
 * tablespace는 mutex를 가지지 않는다.
 *
 * # 개선 가능 사항
 * sddTableSpaceNode와 sddDataFileNode에 mutex를 두어 sddDiskMgr의
 * contention을 줄인다.(??)
 *
 * # 관련 property
 *
 * - SMU_MAX_OPEN_FILE_COUNT  : 최대 오픈할 수 있는 file 개수
 * - SMU_OPEN_DATAFILE_WAIT_INTERVAL : file을 오픈하기 위해 대기하는 시간
 *
 **********************************************************************/

#include <smu.h>
#include <smErrorCode.h>
#include <smiMain.h>
#include <sdd.h>
#include <sddReq.h>
#include <sctTableSpaceMgr.h>
#include <smriChangeTrackingMgr.h>
#include <smriBackupInfoMgr.h>

iduMutex       sddDiskMgr::mMutexVFL;         // VictimFileList용 Mutex
iduMutex       sddDiskMgr::mMutexOPC;         // OpenFileCount 용 Mutex
iduMutex       sddDiskMgr::mGlobalPageCountCheckMutex;
smuList        sddDiskMgr::mVictimFileList;   // Close 대상 datafile 노드의 리스트
UInt           sddDiskMgr::mOpenFileCount;    // 오픈된 datafile 노드 개수
UInt           sddDiskMgr::mVictimListNodeCount;    // 오픈된 datafile 노드 개수
UInt           sddDiskMgr::mMaxDataFilePageCount;  // 모든 datafile의 maxsize
UInt           sddDiskMgr::mWaitThr4Open;     // datafile 오픈대기 thread 개수

// 최초 page write시에 저장되는 checksum
UInt           sddDiskMgr::mInitialCheckSum;

idBool         sddDiskMgr::mEnableWriteToOfflineDBF;
idBool         sddDiskMgr::mIsDirectIO;

/***********************************************************************
 * Description : 디스크관리자 초기화
 *
 * 시스템 startup시에 호출되어, 디스크관리자를 초기화한다.
 *
 * + 2nd. code design
 *   - 디스크관리자의 mutex 생성 및 초기화
 *   - tablespace 노드를 위한 HASH를 생성
 *   - tablespace 리스트 및 tablespace 노드 개수 초기화한다.
 *   - datafile LRU 리스트 및 LRU datafile 노드 개수 초기화한다.
 *   - max open datafile을 설정한다.
 *   - max system datafile size를 설정한다.
 *   - next tablespace ID를 초기화한다.
 *   - open datafile CV를 초기화한다.
 *   - open datafile를 위한 대기플래그를 ID_FALSE를 한다.
 **********************************************************************/
IDE_RC sddDiskMgr::initialize( UInt  aMaxFilePageCnt )
{
    UInt  i;

    IDE_DASSERT( aMaxFilePageCnt > 0 );

    mMaxDataFilePageCount = aMaxFilePageCnt;

    /* 오픈된 datafile 노드의 BASE 리스트 및 개수 */
    SMU_LIST_INIT_BASE(&mVictimFileList);
    mOpenFileCount       = 0;
    mVictimListNodeCount = 0;
    //PRJ-1149.

    mWaitThr4Open = 0;

    for(i = 0, mInitialCheckSum = 0; i < SD_PAGE_SIZE - ID_SIZEOF(UInt); i++)
    {
        mInitialCheckSum = smuUtility::foldUIntPair(mInitialCheckSum, 0);
    }

    // BUG-17158
    mEnableWriteToOfflineDBF = ID_FALSE;

    /* BUG-47398 AIX 에서 iduMemMgr::free4malign() 함수에서 비정상 종료
     * Direct IO 유무 설정 */
    mIsDirectIO = ( smuProperty::getIOType() == 0 ) ? ID_FALSE : ID_TRUE;

    IDE_TEST( mMutexVFL.initialize( (SChar*)"CLOSE_VICTIM_FILE_LIST_MUTEX",
                                    IDU_MUTEX_KIND_NATIVE,
                                    IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    IDE_TEST( mMutexOPC.initialize( (SChar*)"OPEN_FILE_COUNT_MUTEX",
                                    IDU_MUTEX_KIND_NATIVE,
                                    IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    IDE_TEST( mGlobalPageCountCheckMutex.initialize(
                  (SChar*)"SDD_GLOBAL_PAGE_COUNT_CHECK_MUTEX",
                  IDU_MUTEX_KIND_POSIX,
                  IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Space Cache 설정 */
void  sddDiskMgr::setSpaceCache( scSpaceID  aSpaceID,
                                 void     * aSpaceCache )
{
    sddTableSpaceNode*  sSpaceNode;

    sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeIncludingDropped( aSpaceID );

    setSpaceCache( sSpaceNode,
                   aSpaceCache );
    return;
}

/* Space Cache 설정 */
void  sddDiskMgr::setSpaceCache( sddTableSpaceNode * aSpaceNode,
                                 void              * aSpaceCache )
{
    aSpaceNode->mSpaceCache = aSpaceCache;

    return;
}

/* Space Cache 반환 */
sdptbSpaceCache * sddDiskMgr::getSpaceCache( scSpaceID  aSpaceID )
{
    sddTableSpaceNode*  sSpaceNode;

    sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeIncludingDropped( aSpaceID );

    IDE_ASSERT( sSpaceNode != NULL );
    IDE_ASSERT( sSpaceNode->mHeader.mID == aSpaceID );

    return getSpaceCache( sSpaceNode );
}


/***********************************************************************
 * Description : 디스크관리자 해제
 *
 * 시스템 shutdown시에 호출되어, 디스크관리자를 해제한다.
 *
 * + 2nd. code design
 *   - 디스크관리자의 mutex 획득
 *   - while( 오픈된 datafile 노드 LRU 리스트의 모든 datafile 노드에 대해 )
 *     {
 *         datafile을 close 한다. -> closeDataFile
 *     }
 *   - while( 모든 tablespace에 대하여 )
 *     {
 *       각 tablespace를 destroy한다 -> sddTableSpace::destroy(노드만)
 *       테이블 스페이스 메모리를 해제한다.
 *     }
 *   - 디스크관리자 mutex 해제
 *   - open datafile CV를 destroy한다.
 *   - 디스크관리자의 mutex를 destroy한다.
 *   - tablespace 노드를 위한 hash를 destroy한다. 
 **********************************************************************/
IDE_RC sddDiskMgr::destroy()
{
    UInt                i,j;
    smuList           * sNode;
    smuList           * sBaseNode;
    sddDataFileNode   * sFileNode;
    sddTableSpaceNode * sSpaceNode;
    // 현 시점에 모든 file node가 사용중이지 않다면 모두 node list 안에 있다.
    // 동시성을 확인할 필요는 없어보인다.
    /* ------------------------------------------------
     * LRU 리스트를 순회하면서 오픈된 datafile들을 모두 닫는다.
     * ----------------------------------------------*/
    sBaseNode = &mVictimFileList;

    for(sNode = SMU_LIST_GET_FIRST(sBaseNode);
        sNode != sBaseNode;
        sNode = SMU_LIST_GET_FIRST(sBaseNode) )
    {
        sFileNode = (sddDataFileNode*)sNode->mData;

        if ( sFileNode->mIsOpened == ID_TRUE )
        {
            IDE_TEST( closeDataFile( NULL, sFileNode ) != IDE_SUCCESS );
        }
        else
        {
            removeNodeList( NULL,
                            sFileNode );
        }
    }

    // 0이 되어야 하지만 혹시 있다면 전체 순회 정리 시도한다.
    if ( mOpenFileCount > 0 )
    {
        for( i = 0 ; i < sctTableSpaceMgr::getNewTableSpaceID() ; i++ )
        {
            sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::getSpaceNodeBySpaceID( i );

            if( sSpaceNode == NULL )
            {
                continue;
            }

            for ( j = 0; j < sSpaceNode->mNewFileID ; j++ )
            {
                sFileNode = sSpaceNode->mFileNodeArr[j] ;

                if( sFileNode == NULL )
                {
                    continue;
                }
                if ( sFileNode->mIsOpened == ID_TRUE )
                {
                    IDE_TEST( closeDataFile( NULL, sFileNode ) != IDE_SUCCESS );
                }
            }
        }
        IDE_DASSERT( mOpenFileCount == 0 );
    }
    IDE_TEST( mMutexOPC.destroy() != IDE_SUCCESS );
    IDE_TEST( mMutexVFL.destroy() != IDE_SUCCESS );
    IDE_TEST( mGlobalPageCountCheckMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * TBS의 자료구조를 참조하여 동일한 파일명이 존재하는지를 알아낸다.
 * BUG-18044를 fix하기위하여 작성되어졌다.
 *
 * [IN]aSpaceNode         : Space Node
 * [IN]aDataFileAttr      : TBS에 추가할 파일들에 대한 정보를 저장하고있음.
 * [IN]aDataFileAttrCount : aDataFileAttr의 요소갯수
 * [OUT]aExistFileName    : TBS에 동일한 파일명이 존재한다면 그 파일명 리턴함.
 *                          (에러메시지 출력용으로 사용됨)
 * [OUT]aNameExist        : 동일한 파일명이 존재하는가?
 *
 */
IDE_RC sddDiskMgr::validateDataFileName(
                                     sddTableSpaceNode  * aSpaceNode,
                                     smiDataFileAttr   ** aDataFileAttr,
                                     UInt                 aDataFileAttrCount,
                                     SChar             ** aExistFileName,
                                     idBool            *  aNameExist )
{
    UInt                i,k;
    sddDataFileNode *   sFileNode=NULL;
    SChar               sNewFileName[SMI_MAX_DATAFILE_NAME_LEN];
    UInt                sNameLength;

    IDE_ASSERT( aSpaceNode          != NULL );
    IDE_ASSERT( aDataFileAttr       != NULL );
    IDE_ASSERT( aDataFileAttrCount  > 0 );
    IDE_ASSERT( aExistFileName      != NULL );
    IDE_ASSERT( aNameExist          != NULL );

    *aNameExist = ID_FALSE;

    for( i=0; i < aDataFileAttrCount; i++)
    {
        idlOS::strcpy( sNewFileName, (*aDataFileAttr[i]).mName );

        sNameLength = idlOS::strlen(sNewFileName);

        IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                            ID_TRUE,
                                            sNewFileName,
                                            &sNameLength,
                                            SMI_TBS_DISK) != IDE_SUCCESS );

        for (k=0; k < aSpaceNode->mNewFileID ; k++ )
        {
            sFileNode = aSpaceNode->mFileNodeArr[k] ;

            if( sFileNode == NULL )
            {
                continue;
            }

            if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
            {
                continue;
            }

            /*
             * 동일한 파일명이 TBS에 존재하는지 검사
             */
            if (idlOS::strcmp(sFileNode->mName, sNewFileName) == 0)
            {
                *aNameExist = ID_TRUE;
                *aExistFileName = sFileNode->mName;
                IDE_CONT( return_anyway );
            }
        }
    }

    IDE_EXCEPTION_CONT( return_anyway );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : tablespace 생성 및 초기화 (노드포함)
 *
 * 디스크관리자에 새로운 tabespace 노드를 만들고 초기화 한다.
 * 해당 노드에 datafile 노드 및 실제 datafile를 생성한다.
 *
 * CREATE TABLESPACE 구문이 실행될 때 sdpTableSpace::create에서
 * 호출되어 meta (0/1/2번) page 초기화 등을 수행할 것이다.
 *
 * [!!] 디스크관리자 mutex 해제하기 전에 로그앵커를 FLUSH 한다.
 * 
 * tablespace ID가 재사용되었는지 여부를 알기 위해서는
 * create 시점의 system SCN을 tablespace에 저장할 필요가 있다.
 *
 * + 2nd. code design
 *   - tablespace 노드를 위한 메모리를 할당한다.
 *   - tablespace 노드를 초기화한다. -> sddTableSpace::initialize
 *   - 디스크관리자 mutex 획득
 *   - tablespace ID를 할당하고, 속성에 설정한다.
 *   - tablespace 노드에 datafile의 노드를 만들고, 실제 datafile을
 *     create한다. -> sddTableSpace::createDataFiles
 *   - HASH에 추가한다.
 *   - tablespace 노드를 tablespace 리스트에 추가한다.
 *   - 로그앵커 FLUSH -> smrLogMgr::updateLogAnchorForTableSpace
 *   - 디스크관리자 mutex 해제
 **********************************************************************/
IDE_RC sddDiskMgr::createTableSpace( idvSQL             * aStatistics,
                                     void               * aTrans,
                                     smiTableSpaceAttr  * aTableSpaceAttr,
                                     smiDataFileAttr   ** aDataFileAttr,
                                     UInt                 aDataFileAttrCount,
                                     smiTouchMode         aTouchMode )
{
    UInt                sChkptBlockState    = 0;
    UInt                sStateTbs;
    UInt                sAllocState;
    sctPendingOp      * sPendingOp;
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    UInt                i;
    idBool              sFileExist;
    UInt                sNameLength;
    SChar               sValidName[ SM_MAX_FILE_NAME ];

    IDE_DASSERT( aTrans             != NULL );
    IDE_DASSERT( aTableSpaceAttr    != NULL );
    IDE_DASSERT( aTableSpaceAttr->mAttrType == SMI_TBS_ATTR );
    IDE_DASSERT( aDataFileAttr      != NULL );
    IDE_DASSERT( aDataFileAttrCount != 0 );
    IDE_DASSERT( (aTouchMode == SMI_ALL_TOUCH ) ||
                 (aTouchMode == SMI_EACH_BYMODE) );

    sStateTbs         = 0;
    sAllocState       = 0;
    sSpaceNode        = NULL;

    /* tbs의 dbf 생성간의 mutex 획득 */
    IDE_TEST( sctTableSpaceMgr::lockForCrtTBS() != IDE_SUCCESS );
    sStateTbs = 1;

    for (i = 0; i < aDataFileAttrCount ; i++ )
    {
        IDE_TEST( existDataFile( aDataFileAttr[i]->mName, &sFileExist ) != IDE_SUCCESS );

        if( sFileExist == ID_TRUE )
        {
            sNameLength = 0;

            idlOS::strncpy( (SChar*)sValidName, 
                            aDataFileAttr[i]->mName, 
                            SM_MAX_FILE_NAME );
            sNameLength = idlOS::strlen(sValidName);

            IDE_TEST( sctTableSpaceMgr::makeValidABSPath( ID_TRUE,
                                                          (SChar *)sValidName,
                                                          &sNameLength,
                                                          SMI_TBS_DISK )
                      != IDE_SUCCESS );

            IDE_RAISE( error_already_used_datafile );
        }
        else 
        {
            /* file not exist : nothing to do */
        }
    }

    /* sddDiskMgr_createTableSpace_malloc_SpaceNode.tc */
    IDU_FIT_POINT("sddDiskMgr::createTableSpace::malloc::SpaceNode");
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SDD,
                                ID_SIZEOF(sddTableSpaceNode),
                                (void **)&sSpaceNode,
                                IDU_MEM_FORCE) != IDE_SUCCESS );
    sAllocState = 1;
    idlOS::memset(sSpaceNode, 0x00, ID_SIZEOF(sddTableSpaceNode));

    //PRJ-1149.
    aTableSpaceAttr->mDiskAttr.mNewFileID = 0;

    IDE_TEST( sctTableSpaceMgr::allocNewTableSpaceID(&aTableSpaceAttr->mID)
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::initialize(sSpaceNode, aTableSpaceAttr)
              != IDE_SUCCESS );
    sAllocState = 2;

    // FOR BUG-9640
    // SMR_DLT_FILEOPER : SCT_UPDATE_DRDB_CREATE_TBS
    // after image : tablespace attribute
    IDE_TEST( smLayerCallback::writeDiskTBSCreateDrop(
                                                  aStatistics,
                                                  aTrans,
                                                  SCT_UPDATE_DRDB_CREATE_TBS,
                                                  sSpaceNode->mHeader.mID,
                                                  aTableSpaceAttr,
                                                  NULL ) != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "2.PROJ-1548@sddDiskMgr::createTableSpace", err_ART );

    // PROJ-2133 incremental backup
    // DataFileDescSlot이 할당되어 checkpoint가 발생하면 CTBody가 flush되어
    // 파일에 반영된다. 하지만 loganchor에 DataFileDescSlotID가 저장되기 전에
    // 서버가 죽는다면 할당된 DataFileDescSlot은 사용할수 없게된다.
    //
    // 즉, logAnchor에 DataFileDescSlotID가 저장되기 전까지 CTBody가 flush 되면
    // 안된다.
    if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
    {
        IDE_TEST( smLayerCallback::blockCheckpoint() != IDE_SUCCESS ); 
        sChkptBlockState = 1; 
    }


    IDE_TEST( sddTableSpace::createDataFiles(
                                   aStatistics,
                                   aTrans,
                                   sSpaceNode,
                                   aDataFileAttr,
                                   aDataFileAttrCount,
                                   aTouchMode,
                                   mMaxDataFilePageCount) // loganchor offset
              != IDE_SUCCESS );
    sAllocState = 3;

    IDU_FIT_POINT_RAISE( "3.PROJ-1548@sddDiskMgr::createTableSpace", err_ART );

    for (i=0; i < sSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = sSpaceNode->mFileNodeArr[i] ;

        IDE_ASSERT( sFileNode != NULL);

        IDE_TEST( sddDataFile::addPendingOperation(
                                              aTrans,
                                              sFileNode,
                                              ID_TRUE, /* commit시에 동작 */
                                              SCT_POP_CREATE_DBF,
                                              &sPendingOp )
                  != IDE_SUCCESS );

        sPendingOp->mTouchMode = aTouchMode;

        IDU_FIT_POINT_RAISE( "4.PROJ-1548@sddDiskMgr::createTableSpace", err_ART );

        // PRJ-1548 User Memory Tablespace
        // TBS Node에 X 잠금을 획득하기 때문에 DBF Node에 X 잠금을
        // 획득할 필요가 없다.
    }

    // PRJ-1548 User Memory Tablespace
    // 시스템 테이블스페이스의 경우 테이블스페이스 노드에
    // 잠금을 획득할 필요가 없다.

    if ( sctTableSpaceMgr::isSystemTableSpace( sSpaceNode->mHeader.mID )
         != ID_TRUE )
    {
        // addTableSpaceNode하게 되면 다른 연산이 TBS Node를 먼저 보기 때문이고
        // 생성중인 TBS 에 대해서 먼저 잠금을 획득할 수 있게 된다.
        // 이를 방지하기 위해서 TBS List에 추가되기 전에 먼저 잠금을 획득한다.
        // 이와 함께, CREATE 중인 TBS Node는 검색이 안되도록 처리할 필요가 있다.
        // mutex와 2PL간의 lock coupling 시에 deadlock이 발생할수 있다.
        // 그러나, 예외적으로 새롭게 생성되는 객체와의 coupling은
        // deadlock을 발생시키지 않는다.

        IDE_TEST( smLayerCallback::lockItem(
                         aTrans,
                         sSpaceNode->mHeader.mLockItem4TBS,
                         ID_FALSE,       /* intent */
                         ID_TRUE,        /* exclusive */
                         ID_ULONG_MAX,   /* lock wait micro sec. */
                         NULL,           /* locked */
                         NULL )          /* to be unlocked by transaction */
                  != IDE_SUCCESS );
    }

    /* 동일한 tablespace명이 존재하는지 검사한다. */
    // BUG-26695 TBS Node가 없는것이 정상이므로 없을 경우 오류 메시지를 반환하지 않도록 수정
    IDE_TEST_RAISE( sctTableSpaceMgr::checkExistSpaceNodeByName(
                                          sSpaceNode->mHeader.mName ) == ID_TRUE,
                    err_already_exist_tablespace_name );


    /* To Fix BUG-23701 [SD] Create Disk Tablespace시 SpaceNode의 상태값을
     * 설정하여 mSpaceNodeArr 에 추가하여야 함. */
    sSpaceNode->mHeader.mState = SMI_TBS_CREATING | SMI_TBS_ONLINE;

    // TBS List에 추가하면서 TBS Count 증가시킴
    sctTableSpaceMgr::addTableSpaceNode( (sctTableSpaceNode*) sSpaceNode );
    sAllocState = 4;

    /* tablespace 생성간의 mutex 해제 */
    sStateTbs = 0;
    IDE_TEST( sctTableSpaceMgr::unlockForCrtTBS() != IDE_SUCCESS );

    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                    aTrans,
                                    sSpaceNode->mHeader.mID,
                                    ID_TRUE, /* commit시에 동작 */
                                    SCT_POP_CREATE_TBS)
              != IDE_SUCCESS );

    IDU_FIT_POINT( "2.PROJ-1552@sddDiskMgr::createTableSpace" );
    
    IDU_FIT_POINT_RAISE( "5.PROJ-1548@sddDiskMgr::createTableSpace", err_ART );

    // PRJ-1548 User Memory Tablespace
    // 새로운 TBS Node와 DBF Node들을 Loganchor에 추가한다.
    IDE_ASSERT( smLayerCallback::addTBSNodeAndFlush( (sctTableSpaceNode*)sSpaceNode )
                == IDE_SUCCESS );

    IDU_FIT_POINT( "3.PROJ-1552@sddDiskMgr::createTableSpace" );

    IDU_FIT_POINT_RAISE( "6.PROJ-1548@sddDiskMgr::createTableSpace", err_ART );

    for( i = 0 ; i < sSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = sSpaceNode->mFileNodeArr[i] ;

        IDE_ASSERT( sFileNode != NULL );

        // BUGBUG
        // 새로운 DBF Node들을 Loganchor에 추가한다.
        IDE_ASSERT( smLayerCallback::addDBFNodeAndFlush( sSpaceNode,
                                                         sFileNode )
                    == IDE_SUCCESS );

        IDU_FIT_POINT_RAISE( "7.PROJ-1548@sddDiskMgr::createTableSpace", err_ART );
    }

    //PROJ-2133 incremental backup
    if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
    {
        sChkptBlockState = 0; 
        IDE_TEST( smLayerCallback::unblockCheckpoint() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( err_ART );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ART));
    }
#endif
    IDE_EXCEPTION( error_already_used_datafile );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_UseFileInOtherTBS, sValidName));
    }
    IDE_EXCEPTION( err_already_exist_tablespace_name )
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AlreadyExistTableSpaceName,
                                sSpaceNode->mHeader.mName));
    }

    IDE_EXCEPTION_END;

    if ( sStateTbs != 0 )
    {
        IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS() == IDE_SUCCESS );
    }

    if( sChkptBlockState == 1)
    {
        IDE_ASSERT( smLayerCallback::unblockCheckpoint() == IDE_SUCCESS );
    }

    /* Tablespace이름이 이미 있거나 데이터 파일이 이미 있거나
       혹은 다른 이유에 의해 Exception이 발생하면
       sAllocState의 상태별로 자원 해제 처리를 해줘야 한다.
       BUG-18176 */

    switch (sAllocState)
    {
        case 3:
            /* sAllocState가 3이라면 createDataFiles() 호출 이후이기 때문에
               데이터 파일부터 모두 제거해야 한다. */
            IDE_ASSERT(sddTableSpace::removeAllDataFiles(NULL,  /* idvSQL* */
                                                         NULL,  /* void *  */
                                                         sSpaceNode,
                                                         SMI_ALL_NOTOUCH,
                                                         ID_FALSE)
                       == IDE_SUCCESS);

        case 2:
            /* sAllocState가 2라면 sSpaceNode에 대해 initialize된 상태이기 때문에
               destory()를 호출해서 자원을 해제해야 한다. */
            IDE_ASSERT(sddTableSpace::destroy(sSpaceNode) == IDE_SUCCESS);

        case 1:
            IDE_ASSERT(iduMemMgr::free(sSpaceNode) == IDE_SUCCESS);
            break;

        default:
            /* sAllocState가 4이면 sSpaceNode를 추가한 상태이기 때문에
               undo시 모두 해제가 되므로 여기서 해주지 않는다. */
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : tablespace 생성 및 초기화
 *  PROJ-1923 ALTIBASE HDB Disaster Recovery
 *
 * 디스크관리자에 새로운 tablespace 노드를 만들고 초기화 한다.
 *
 * CREATE TABLESPACE 구문이 실행될 때 sdpTableSpace::create에서
 * 호출되어 meta (0/1/2번) page 초기화 등을 수행할 것이다.
 *
 * [!!] 디스크관리자 mutex 해제하기 전에 로그앵커를 FLUSH 한다.
 *
 * tablespace ID가 재사용되었는지 여부를 알기 위해서는
 * create 시점의 system SCN을 tablespace에 저장할 필요가 있다.
 *
 **********************************************************************/
IDE_RC sddDiskMgr::createTableSpace4Redo( void               * aTrans,
                                          smiTableSpaceAttr  * aTableSpaceAttr )
{
    sddTableSpaceNode * sSpaceNode;
    UInt                sAllocState     = 0;
    scSpaceID           sNewSpaceID     = 0;

    IDE_DASSERT( aTrans             != NULL );
    IDE_DASSERT( aTableSpaceAttr    != NULL );
    IDE_DASSERT( aTableSpaceAttr->mAttrType == SMI_TBS_ATTR );

    /* sddDiskMgr_createTableSpace4Redo_malloc_SpaceNode.tc */
    IDU_FIT_POINT("sddDiskMgr::createTableSpace4Redo::malloc::SpaceNode");
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SDD,
                                ID_SIZEOF(sddTableSpaceNode),
                                (void **)&sSpaceNode,
                                IDU_MEM_FORCE) != IDE_SUCCESS );
    sAllocState = 1;
    idlOS::memset(sSpaceNode, 0x00, ID_SIZEOF(sddTableSpaceNode));

    //PRJ-1149
    aTableSpaceAttr->mDiskAttr.mNewFileID = 0;

    /* 현재의 mNewTableSpaceID를 가져와 로그의 값과 검증해 보고 실제로 사용 함 */
    sNewSpaceID = sctTableSpaceMgr::getNewTableSpaceID();

    IDE_TEST( sNewSpaceID != aTableSpaceAttr->mID );

    IDE_TEST( sctTableSpaceMgr::allocNewTableSpaceID(&aTableSpaceAttr->mID)
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::initialize(sSpaceNode, aTableSpaceAttr)
              != IDE_SUCCESS );
    sAllocState = 2;

    /* 동일한 tablespace명이 존재하는지 검사한다. */
    // BUG-26695 TBS Node가 없는것이 정상이므로 없을 경우
    // 오류 메시지를 반환하지 않도록 수정
    IDE_TEST_RAISE( sctTableSpaceMgr::checkExistSpaceNodeByName( sSpaceNode->mHeader.mName ) == ID_TRUE, 
                    err_already_exist_tablespace_name );

    /* To Fix BUG-23701 [SD] Create Disk Tablespace시 SpaceNode의 상태값을
     * 설정하여 mSpaceNodeArr 에 추가하여야 함. */
    sSpaceNode->mHeader.mState = SMI_TBS_CREATING | SMI_TBS_ONLINE;

    /* TBS List에 추가하면서 TBS Count 증가시킴 */
    sctTableSpaceMgr::addTableSpaceNode( (sctTableSpaceNode *) sSpaceNode );
    sAllocState = 3;

    IDE_TEST( sctTableSpaceMgr::addPendingOperation( aTrans,
                                                     sSpaceNode->mHeader.mID, 
                                                     ID_TRUE, // commit시에 동작
                                                     SCT_POP_CREATE_TBS)
              != IDE_SUCCESS );

    // PRJ-1548 User Memory Tablespace
    // 새로운 TBS Node와 DBF Node들을 Loganchor에 추가한다.
    IDE_ASSERT( smLayerCallback::addTBSNodeAndFlush( (sctTableSpaceNode *)sSpaceNode )
                == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_already_exist_tablespace_name )
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AlreadyExistTableSpaceName,
                                sSpaceNode->mHeader.mName));
    }

    IDE_EXCEPTION_END;

    switch (sAllocState)
    {
        case 2:
            /* sAllocState가 2라면 sSpaceNode에 대해 initialize된 상태이기 때문에
             * destory()를 호출해서 자원을 해제해야 한다. */
            IDE_ASSERT(sddTableSpace::destroy(sSpaceNode) == IDE_SUCCESS);

        case 1:
            IDE_ASSERT(iduMemMgr::free(sSpaceNode) == IDE_SUCCESS);
            break;

        default:
            /* sAllocState가 3이면 sSpaceNode를 추가한 상태이기 때문에
             * undo시 모두 해제가 되므로 여기서 해주지 않는다. */
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : DBF 생성 및 초기화
 *  PROJ-1923 ALTIBASE HDB Disaster Recovery
 *
 * 디스크관리자에서 해당 TBS에 datafile 노드 및 실제 datafile를 생성한다.
 *
 **********************************************************************/
IDE_RC sddDiskMgr::createDataFile4Redo( void              * aTrans,
                                        smLSN               aCurLSN,
                                        scSpaceID           aSpaceID,
                                        smiDataFileAttr   * aDataFileAttr )
{
    sddTableSpaceNode * sSpaceNode      = NULL;
    SChar             * sExistFileName  = NULL;
    sddDataFileNode   * sFileNode       = NULL;
    sctPendingOp      * sPendingOp      = NULL;
    idBool              sNameExist      = ID_FALSE;
    sdFileID            sNewFileIDSave  = 0;
    smiTableSpaceAttr   sDummySpaceAttr;
    sddTableSpaceNode   sDummySpaceNode;
    idBool              sFileExist      = ID_FALSE;
    SInt                rc              = 0;
    UInt                i               = 0;
    UInt                sNameLength     = 0;
    UInt                sChkptBlockState    = 0;
    SChar               sValidName[ SM_MAX_FILE_NAME ];

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceID ) == ID_TRUE );
    IDE_DASSERT( aDataFileAttr != NULL );

    /* ===================================
     * [1] dummy tablespace 생성 및 초기화
     * =================================== */
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sSpaceNode)
              != IDE_SUCCESS );

    IDE_TEST( validateDataFileName( sSpaceNode,
                                    &aDataFileAttr,
                                    1, // aDataFileAttrCount
                                    &sExistFileName, //에러메시지 출력용
                                    &sNameExist ) != IDE_SUCCESS);

    IDE_TEST_RAISE( sNameExist == ID_TRUE,
                    error_the_filename_is_in_use );

    // PRJ-1149.
    IDE_TEST_RAISE( SMI_TBS_IS_BACKUP(sSpaceNode->mHeader.mState),
                    error_forbidden_op_while_backup);

    // 현재의 mNewFileID를 백업한 후 1 증가 시킨다.
    // 이유는, DBF 생성 후에 백업한 mNewFieID에서부터
    // +1 한 현재의 mNewFileID까지 후처리 한다.
    sNewFileIDSave          = sSpaceNode->mNewFileID ;
    sSpaceNode->mNewFileID += 1; // aDataFileAttrCount;

    idlOS::memset( &sDummySpaceAttr, 0x00, ID_SIZEOF(smiTableSpaceAttr));
    idlOS::memset( &sDummySpaceNode, 0x00, ID_SIZEOF(sddTableSpaceNode));
    sDummySpaceAttr.mID         = aSpaceID;
    sDummySpaceAttr.mAttrType   = SMI_TBS_ATTR;

    IDE_TEST( sddTableSpace::initialize(&sDummySpaceNode,
                                        &sDummySpaceAttr) != IDE_SUCCESS );

    sDummySpaceNode.mNewFileID = sSpaceNode->mNewFileID - 1; //aDataFileAttrCount;

    /* PROJ-2133 incremental backup
     * DataFileDescSlot이 할당되어 checkpoint가 발생하면 CTBody가 flush되어
     * 파일에 반영된다. 하지만 loganchor에 DataFileDescSlotID가 저장되기 전에
     * 서버가 죽는다면 할당된 DataFileDescSlot은 사용할수 없게된다.
     *
     * 즉, logAnchor에 DataFileDescSlotID가 저장되기 전까지 CTBody가 flush 되면
     * 안된다. */
    if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
    {
        IDE_TEST( smLayerCallback::blockCheckpoint() != IDE_SUCCESS );
        sChkptBlockState = 1;
    }

    // 동일 파일 이름이 있는지 검사한다.
    // 있다면 지우고 재생성 한다.
    // 왜냐하면 Log Anchor에는 기록이 안된 상태로 redo를 시작하였으므로,
    // 기존의 DBF는 믿을 수 없는 파일이다.
    IDE_TEST( existDataFile( aDataFileAttr->mName, &sFileExist ) != IDE_SUCCESS );

    if( sFileExist == ID_TRUE )
    {
        // log Anchor에 파일 노드는 없는 상황에서 넘어 왔으므로, 파일만 있음
        idlOS::strncpy( (SChar *)sValidName,
                        aDataFileAttr->mName,
                        SM_MAX_FILE_NAME );
        sNameLength = idlOS::strlen(sValidName);

        IDE_TEST( sctTableSpaceMgr::makeValidABSPath( ID_TRUE,
                                                      (SChar *)sValidName,
                                                      &sNameLength,
                                                      SMI_TBS_DISK )
                  != IDE_SUCCESS );

        // 파일 삭제
        rc = idf::unlink(sValidName);

        IDE_TEST_RAISE( rc != 0 , err_file_unlink );
    }
    else
    {
        /* do nothing */
    }

    /* ===================================
     * [2] 물리적 데이타 화일 생성
     * =================================== */
    IDE_TEST( sddTableSpace::createDataFile4Redo( NULL,
                                                  aTrans,
                                                  &sDummySpaceNode,
                                                  aDataFileAttr,
                                                  aCurLSN,
                                                  SMI_EACH_BYMODE,
                                                  mMaxDataFilePageCount)
              != IDE_SUCCESS );

    for (i = sNewFileIDSave; i < sSpaceNode->mNewFileID; i++ )
    {
        sFileNode = sDummySpaceNode.mFileNodeArr[i] ;

        IDE_ASSERT( sFileNode != NULL);
    }

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void **)&sSpaceNode)
              != IDE_SUCCESS );

    /* ====================================================================
     * [3] 데이타 화일들을 자신이 속한 테이블 스페이스의 화일 리스트에 등록
     * ==================================================================== */

    // fix BUG-16116 [PROJ-1548] sddDiskMgr::createDataFiles()
    // 에러시 DBF Node에 대한 Memory 해제
    // 다음 For loop 안에서 EXCEPTION이 발생하게 되면 DBF Node
    // 메모리 해제에 대한 문제가 있으므로 exception 발생코드는
    // 코딩하지 않는다.
    // 이 이후에 Exception이 발생한다면, 트랜잭션 rollback에 의해서
    // 자동으로 해제될 것이다.

    for( i = sNewFileIDSave ; i < sSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = sDummySpaceNode.mFileNodeArr[i] ;

        IDE_ASSERT( sFileNode != NULL);

        sFileNode->mState |= SMI_FILE_CREATING;
        sFileNode->mState |= SMI_FILE_ONLINE;

        sddTableSpace::addDataFileNode(sSpaceNode, sFileNode);
    }

    for( i = sNewFileIDSave ; i < sSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = sSpaceNode->mFileNodeArr[i] ;

        IDE_ASSERT( sFileNode != NULL);

        IDE_TEST( sddDataFile::addPendingOperation( aTrans,
                                                    sFileNode,
                                                    ID_TRUE, /* commit시에 동작 */
                                                    SCT_POP_CREATE_DBF,
                                                    &sPendingOp )
                  != IDE_SUCCESS );

        sPendingOp->mTouchMode = SMI_EACH_BYMODE; // aTouchMode;

        /* [4] 로그 앵커 변경
         * 시스템 가동시 로그 앵커의 내용을 이용하여 데이타 화일 리스트를 구성하며,
         * 이 경우에는 로그 앵커를 재변경하지 않는다. */
        IDE_ASSERT( smLayerCallback::addDBFNodeAndFlush( sSpaceNode,
                                                         sFileNode )
                    == IDE_SUCCESS );
    }

    // PROJ-2133 incremental backup
    if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
    {
        sChkptBlockState = 0;
        IDE_TEST( smLayerCallback::unblockCheckpoint() != IDE_SUCCESS );
    }

    IDE_TEST( sddTableSpace::destroy( &sDummySpaceNode ) != IDE_SUCCESS );

    // PRJ-1548 User Memory Tablespace
    // 새로운 TBS Node와 DBF Node들을 Loganchor에 추가한다.
    IDE_ASSERT( smLayerCallback::updateTBSNodeAndFlush( (sctTableSpaceNode *)sSpaceNode )
                == IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION(error_forbidden_op_while_backup);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_forbiddenOpWhileBackup,
                                "ADD DATAFILE"));
    }
    IDE_EXCEPTION( error_the_filename_is_in_use )
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_UseFileInTheTBS,
                                 sExistFileName,
                                 ((sctTableSpaceNode *)sSpaceNode)->mName));
    }
    IDE_EXCEPTION(err_file_unlink);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_FileDelete, sValidName));
    }
    IDE_EXCEPTION_END;

    if( sChkptBlockState != 0 )
    {
        IDE_ASSERT( smLayerCallback::unblockCheckpoint()
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그앵커에 의한 tablespace 노드 생성 및 초기화
 *
 * 로그앵커에 저장된 tablespace 정보를 이용하여 디스크관리자에 하나의
 * tabespace 노드를 생성하고 초기화한다. 로그앵커 관리자 초기화시에 호출되어
 * tablespace 노드만 생성하여 HASH 및 tablespace 노드 리스트에 추가하지만,
 * 실제 화일은 create하지 않는다.
 *
 * + 2nd. code design
 *   - tablespace 노드를 위한 메모리를 할당한다.
 *   - tablespace 노드를 초기화한다. -> sddTableSpace::initialize
 *   - 디스크관리자의 mutex 획득
 *   - 디스크관리자의 HASH에 추가한다.
 *   - tablespace 노드 리스트에 추가한다.
 *   - tablespace 노드 중 space ID가 가장 큰 ID를 구해서
 *     SMU_MAX_TABLESPACE_ID와 비교하여 mNewTableSpaceID를
 *     설정한다.
 *   - 디스크관리자의 mutex 해제
 **********************************************************************/
IDE_RC sddDiskMgr::loadTableSpaceNode( idvSQL*           /* aStatistics*/,
                                       smiTableSpaceAttr* aTableSpaceAttr,
                                       UInt               aAnchorOffset )
{
    sddTableSpaceNode*   sSpaceNode;
    UInt                 sState = 0;

    IDE_DASSERT( aTableSpaceAttr != NULL );
    IDE_DASSERT( aTableSpaceAttr->mAttrType == SMI_TBS_ATTR );

    sSpaceNode = NULL;

    /* sddDiskMgr_loadTableSpaceNode_calloc_SpaceNode.tc */
    IDU_FIT_POINT("sddDiskMgr::loadTableSpaceNode::calloc::SpaceNode");
    IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SDD,
                                1,
                                ID_SIZEOF(sddTableSpaceNode),
                                (void**)&sSpaceNode,
                                IDU_MEM_FORCE) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sddTableSpace::initialize( sSpaceNode,
                                         aTableSpaceAttr) != IDE_SUCCESS );

    // PRJ-1548 User Memory Tablespace
    sSpaceNode->mAnchorOffset = aAnchorOffset;

    sctTableSpaceMgr::addTableSpaceNode( (sctTableSpaceNode*)sSpaceNode );
    sState = 0;
    /* 한번 SpaceNode Array에 세팅 된 SpaceNode는 Service 중에는 제거 할 수 없다.*/

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free( sSpaceNode ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  tablespace 제거 (노드 제거만 혹은 노드 및 파일 제거)
 *
 * tablespace를 제거하며, drop 모드에 따라서 datafile까지 제거할 수도 있다.
 *
 * - DDL문인 DROP TABLESPACE의 수행시 호출된다. 이는 해당 tablespace가
 *   OFFLINE이어야만 가능하다.
 * - OFFLINE 모드로 tablespace가 바뀔 때, 이미 datafile은 close 되어 있다.
 * - 이 함수는 SDP 단에서 호출되며, 이 작업후 로그앵커 FLUSH 작업이 수행된다.
 *   이 작업을 위해 오픈 datafile LRU 리스트에 대한 HASH를 따로 두지는 않는다.
 *
 * + 2nd. code design
 *   - 디스크관리자 mutex 획득한다.
 *   - HASH에서 tablespace 노드를 찾는다.
 *   - if( 검색한 tablespace 노드가 ONLINE이면 )
 *     {
 *        디스크관리자 mutex 해제;
 *        return fail;
 *     }
 *   - tablespace 노드를 리스트에서 제거한다.
 *   - tablespace 노드를 HASH에서 제거한다.
 *   - 로그앵커 FLUSH -> smrLogMgr::updateLogAnchorForTableSpace
 *   - 디스크관리자 mutex 해제한다.
 *   - tablespace 노드를 destroy 한다 -> sddTableSpace::destroy(aMode)
 *   - tablespace 노드의 메모리를 해제한다.
 **********************************************************************/
IDE_RC sddDiskMgr::removeTableSpace( idvSQL            *aStatistics,
                                     void*              aTrans,
                                     scSpaceID          aTableSpaceID,
                                     smiTouchMode       aTouchMode )
{
    UInt                sState = 0;
    sddTableSpaceNode*  sSpaceNode;
    sctPendingOp     *  sPendingOp;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID ) == ID_TRUE );
    IDE_DASSERT( aTouchMode != SMI_EACH_BYMODE );

    // Tablespace Backup과 동시성 확인 해야 한다.
    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aTableSpaceID,
                                                               (void**)&sSpaceNode )
              != IDE_SUCCESS );
    sState = 1;

    // backup 중인 테이블 스페이스는 drop 할수 없다
    IDE_TEST_RAISE( SMI_TBS_IS_BACKUP(sSpaceNode->mHeader.mState), 
                    error_forbidden_op_while_backup );

    // dbf 제거는 commit 이후시점으로 tx가 직접 unlink하도록 처리한다.
    // SMR_DLT_FILEOPER : SCT_UPDATE_DRDB_DROP_DBF
    // before image : tablespace attribute
    IDE_TEST( sddTableSpace::removeAllDataFiles(aStatistics,
                                                aTrans,
                                                sSpaceNode,
                                                aTouchMode,
                                                ID_TRUE) /* GhostMark */
              != IDE_SUCCESS );

    // FOR BUG-9640
    // SMR_DLT_FILEOPER : SCT_UPDATE_DRDB_DROP_TBS
    // before image : tablespace attribute
    IDE_TEST( smLayerCallback::writeDiskTBSCreateDrop(
                                                  aStatistics,
                                                  aTrans,
                                                  SCT_UPDATE_DRDB_DROP_TBS,
                                                  sSpaceNode->mHeader.mID,
                                                  NULL, /* PROJ-1923 */
                                                  NULL ) != IDE_SUCCESS );

    /* ------------------------------------------------
     * !! CHECK RECOVERY POINT
     * case) dbf node만 제거되고, anchor는 flush안된 경우
     * redo시 tbs node  제거하고, undo시 before tbs image로
     * 다시 생성한다.
     * ----------------------------------------------*/
    IDU_FIT_POINT( "1.PROJ-1552@sddDiskMgr::removeTableSpace" );

    sSpaceNode->mHeader.mState |= SMI_TBS_DROPPING;

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    IDE_TEST( sctTableSpaceMgr::addPendingOperation( aTrans,
                                                     aTableSpaceID,
                                                     ID_TRUE, /* commit시에 동작 */
                                                     SCT_POP_DROP_TBS,
                                                     &sPendingOp )
              != IDE_SUCCESS );

    /* BUG-29941 - SDP 모듈에 메모리 누수가 존재합니다.
     * Commit Pending 연산 수행중 Space Cache에 할당된 메모리를 해제하도록
     * PendingOpFunc을 등록한다. */
    sPendingOp->mPendingOpFunc = smLayerCallback::freeSpaceCacheCommitPending;

    // DROP TABLESPACE 중간에는 로그앵커 플러쉬를 하지 않는다.

    IDU_FIT_POINT( "1.TASK-1842@sddDiskMgr::removeTableSpace" );

    IDU_FIT_POINT( "2.PROJ-1552@sddDiskMgr::removeTableSpace" );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_forbidden_op_while_backup);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_forbiddenOpWhileBackup,
                                "DROP TABLESPACE"));
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : page 판독 (1)
 *
 * 해당 tablespace의 해당 page를 disk로부터 READ 한다.
 * Read는 이미 TableSpaceLock을 잡은 경우여서 SpaceNode Mutex를 잡을 필요가 없다.
 *
 * - 2nd. code design
 *   + 디스크관리자 mutex 획득
 *   + 디스크관리자의 HASH에서 tablespace 노드를 찾는다.
 *   + 해당 page가 포함된 datafile 노드를 찾는다
 *     -> sddTableSpace::getDataFileNodeByPageID
 *   + Read I/O를 준비한다. -> prepareIO
 *   + 디스크관리자 mutex 해제
 *   + Read I/O를 수행한다.-> sddDataFile::read
 *   + 디스크관리자 mutex 획득
 *   + Read I/O 완료를 수행한다. -> completeIO
 *   + 디스크관리자 mutex 해제
 **********************************************************************/
IDE_RC sddDiskMgr::read( idvSQL     * aStatistics,
                         scSpaceID    aTableSpaceID,
                         scPageID     aPageID,
                         UChar      * aBuffer )
{
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_DASSERT( sctTableSpaceMgr::isSystemMemTableSpace( aTableSpaceID )
                 == ID_FALSE );
    IDE_DASSERT( aBuffer != NULL );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode )
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::getDataFileNodeByPageID( sSpaceNode,
                                                      aPageID,
                                                      &sFileNode,
                                                      ID_FALSE ) // aFatal : could be failed
              != IDE_SUCCESS );

    IDE_ASSERT(sFileNode != NULL);

    IDE_TEST( readPageFromDataFile( aStatistics,
                                    sFileNode,
                                    aPageID,
                                    1, /* Page Count */
                                    aBuffer )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aFileNode가 가리키는 파일에서 aPageID를 읽는다.
 *
 * PRJ-1149관련
 * 데이타 파일이 backup아닌 정상 상태일때 데이타 파일로부터
 * page를 읽어온다.
 *
 * aStatistics - [IN] 통계정보
 * aFileNode   - [IN] 파일노드
 * aPageID     - [IN] PageID
 * aPageCnt    - [IN] Page Count
 * aBuffer     - [IN] Page내용이 들어있는 Buffer
 **********************************************************************/
IDE_RC  sddDiskMgr::readPageFromDataFile( idvSQL           * aStatistics,
                                          sddDataFileNode  * aFileNode,
                                          scPageID           aPageID,
                                          ULong              aPageCnt,
                                          UChar            * aBuffer )
{
    idBool sPreparedIO = ID_FALSE;

    IDE_TEST( prepareIO( aStatistics,
                         aFileNode ) != IDE_SUCCESS );
    sPreparedIO = ID_TRUE;
 
    IDE_TEST( sddDataFile::read( aStatistics,
                                 aFileNode,
                                 SD_MAKE_FOFFSET( aPageID ),
                                 SD_PAGE_SIZE * aPageCnt,
                                 aBuffer )
              != IDE_SUCCESS);

    sPreparedIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aFileNode,
                          SDD_IO_READ ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Read Page From Data File Failure\n"
                 "               Tablespace ID = %"ID_UINT32_FMT"\n"
                 "               File ID       = %"ID_UINT32_FMT"\n"
                 "               IO Count      = %"ID_UINT32_FMT"\n"
                 "               File Opened   = %s",
                 aFileNode->mSpaceID,
                 aFileNode->mID,
                 aFileNode->mIOCount,
                 (aFileNode->mIsOpened ? "Open" : "Close") );

    if( sPreparedIO == ID_TRUE )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aFileNode,
                                SDD_IO_READ )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : aFileNode가 가리키는 파일에서 aPageID를 읽는다.
 *
 * PRJ-1149관련
 * 데이타 파일이 backup아닌 정상 상태일때 데이타 파일로부터
 * page를 읽어온다.
 *
 * aStatistics - [IN] 통계정보
 * aFileNode   - [IN] 파일노드
 * aSpaceID    - [IN] TableSpaceID
 * aPageID     - [IN] PageID
 * aPageCnt    - [IN] Page Count
 * aBuffer     - [IN] Page내용이 들어있는 Buffer
 **********************************************************************/
IDE_RC  sddDiskMgr::readvPageFromDataFile( idvSQL           * aStatistics,
                                           sddDataFileNode  * aFileNode,
                                           scPageID           aPageID,
                                           iduFileIOVec     & aVec )
{
    idBool sPreparedIO = ID_FALSE;

    IDE_TEST( prepareIO( aStatistics,
                         aFileNode ) != IDE_SUCCESS );
    sPreparedIO = ID_TRUE;

    IDE_TEST( sddDataFile::readv( aStatistics,
                                  aFileNode,
                                  SD_MAKE_FOFFSET( aPageID ),
                                  aVec)
              != IDE_SUCCESS);

    sPreparedIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aFileNode,
                          SDD_IO_READ ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Read Page From Data File Failure\n"
                 "               Tablespace ID = %"ID_UINT32_FMT"\n"
                 "               File ID       = %"ID_UINT32_FMT"\n"
                 "               IO Count      = %"ID_UINT32_FMT"\n"
                 "               File Opened   = %s",
                 aFileNode->mSpaceID,
                 aFileNode->mID,
                 aFileNode->mIOCount,
                 (aFileNode->mIsOpened ? "Open" : "Close") );


    if( sPreparedIO == ID_TRUE )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aFileNode,
                                SDD_IO_READ )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : page 판독 (2)
 *
 * 해당 tablespace의 해당 page부터 pagecount만큼 disk로부터 READ 한다.
 *
 * - 2nd. code design
 *   + 디스크관리자 mutex 획득
 *   + 디스크관리자의 HASH에서 tablespace 노드를 찾는다.
 *   + frompid 부터 topid까지 모두 읽어들일때까지 루프를 돈다.
 *     + 해당 page가 포함된 datafile 노드를 찾는다
 *        -> sddTableSpace::getDataFileNodeByPageID
 *     + Read I/O를 준비한다. -> prepareIO
 *     + 디스크관리자 mutex 해제
 *     + Read I/O를 수행한다.-> sddDataFile::read
 *     + 디스크관리자의 mutex 획득한다.
 *     + Read I/O 완료를 수행한다. -> completeIO
 *   + 디스크관리자의 mutex 해제한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] TableSpaceID
 * aFstPageID  - [IN] PageID
 * aBuffer     - [IN] Page내용이 들어있는 Buffer
 * aFatal      - [IN] 예외발생시 동작 : FATAL 또는 ABORT
 **********************************************************************/
IDE_RC sddDiskMgr::read( idvSQL      * aStatistics,
                         scSpaceID     aTableSpaceID,
                         scPageID      aFstPageID,
                         ULong         aPageCount,
                         UChar       * aBuffer,
                         idBool        aFatal )
{
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_DASSERT( sctTableSpaceMgr::isSystemMemTableSpace( aTableSpaceID )
                 == ID_FALSE );
    IDE_DASSERT( aBuffer != NULL );

    sFileNode = NULL;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode )
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::getDataFileNodeByPageID( sSpaceNode,
                                                      aFstPageID,
                                                      &sFileNode,
                                                      aFatal )
              != IDE_SUCCESS );

    IDE_TEST( readPageFromDataFile( aStatistics,
                                    sFileNode,
                                    aFstPageID,
                                    aPageCount,
                                    aBuffer )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : page 판독 (2)
 *
 * 해당 tablespace의 해당 page부터 pagecount만큼 disk로부터 READ 한다.
 *
 * - 2nd. code design
 *   + 디스크관리자 mutex 획득
 *   + 디스크관리자의 HASH에서 tablespace 노드를 찾는다.
 *   + frompid 부터 topid까지 모두 읽어들일때까지 루프를 돈다.
 *     + 해당 page가 포함된 datafile 노드를 찾는다
 *        -> sddTableSpace::getDataFileNodeByPageID
 *     + Read I/O를 준비한다. -> prepareIO
 *     + 디스크관리자 mutex 해제
 *     + Read I/O를 수행한다.-> sddDataFile::read
 *     + 디스크관리자의 mutex 획득한다.
 *     + Read I/O 완료를 수행한다. -> completeIO
 *   + 디스크관리자의 mutex 해제한다.
 *
 * aStatistics - [IN] 통계정보
 * aFileNode   - [IN] 파일노드
 * aSpaceID    - [IN] TableSpaceID
 * aFstPageID  - [IN] PageID
 * aBuffer     - [IN] Page내용이 들어있는 Buffer
 **********************************************************************/
IDE_RC sddDiskMgr::readv( idvSQL       * aStatistics,
                          scSpaceID      aTableSpaceID,
                          scPageID       aFstPageID,
                          iduFileIOVec & aVec )
{
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_DASSERT( sctTableSpaceMgr::isSystemMemTableSpace( aTableSpaceID )
                 == ID_FALSE );

    sFileNode = NULL;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode )
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::getDataFileNodeByPageID( sSpaceNode,
                                                      aFstPageID,
                                                      &sFileNode,
                                                      ID_TRUE ) /* aFatal */
                                                        
              != IDE_SUCCESS );

    IDE_TEST( sddDiskMgr::readvPageFromDataFile( aStatistics,
                                                 sFileNode,
                                                 aFstPageID,
                                                 aVec )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : page 기록 (1)
 *
 * 해당 tablespace의 해당 page를 disk에 WRITE 한다.
 *
 * - 2nd. code design
 *   + 디스크관리자 mutex 획득
 *   + 디스크관리자의 HASH에서 tablespace 노드를 찾는다.
 *   + 해당 page가 포함된 datafile 노드를 찾는다
 *     -> sddTableSpace::getDataFileNodeByPageID
 *   + Write I/O를 준비한다. -> prepareIO
 *   + 디스크관리자 mutex 해제
 *   + Write I/O를 수행한다.-> sddDataFile::write
 *   + 디스크관리자 mutex 획득
 *   + Write I/O 완료를 수행한다. -> completeIO
 *   + 디스크관리자 mutex 해제
 **********************************************************************/
IDE_RC sddDiskMgr::write( idvSQL    * aStatistics,
                          scSpaceID   aTableSpaceID,
                          scPageID    aPageID,
                          UChar     * aBuffer )
{
    sddTableSpaceNode *sSpaceNode;
    sddDataFileNode   *sFileNode;

    IDE_DASSERT( sctTableSpaceMgr::isSystemMemTableSpace( aTableSpaceID )
                 == ID_FALSE );
    IDE_DASSERT( aBuffer != NULL );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode )
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::getDataFileNodeByPageID( sSpaceNode,
                                                      aPageID,
                                                      &sFileNode,
                                                      ID_TRUE ) /* aFatal */
              != IDE_SUCCESS );

    // 일반적으로는 Offline TBS에 Write를 요청하지 않는다.
    // Offline전에 Buffer를 미리 정리한다.
    // 그러므로 lock을 잡고 보지 않아도 된다.
    if ( SMI_FILE_STATE_IS_OFFLINE( sFileNode->mState ) )
    {
        // Restart Recovery 또는 Media Recovery 시에는
        // Offline TBS에 대해 write가 발생할 수 있다.

        // 다음 과정에서 접근이 가능하다.
        // 1) Media Recovery  (open/read/write)
        // : Loganchor에는 Offline 상태이지만,
        //   backup 본들은 Online 상태인 경우 복구는 해야한다.

        // 2) RestartRecovery (open/read/write)
        // : Loganchor에 Online 상태이지만,
        //   Redo하다가 최종적으로는 Offline상태로 되는 경우

        // 3) checkpoint시 Write DBF Hdr (Open/Write)

        // 4) identify 시 read DBF Hdr (Open/Read)

        IDE_TEST_RAISE( isEnableWriteToOfflineDBF() == ID_FALSE ,
                        err_access_to_offline_datafile );
        // 운영중에는 Abort 처리되어야 한다.
    }

    IDE_TEST( writePage2DataFile( aStatistics,
                                  sFileNode,
                                  aPageID,
                                  aBuffer )
              != IDE_SUCCESS);

    /* 
     * PROJ-2133 incremental backup
     * changeTracking은 페이지를 데이터파일에 write한 후에 수행되야만 한다.
     * 그렇지 않을 경우, changeTracking함수 내에서 DataFileDescSlot의 tracking상태가
     * deactive에서 active로 바꿜경우 flush되는 페이지에 대한 changeTracking이
     * 수행되지 않을 수 있다.
     *
     * 즉, changeTracking은 페이지를 데이터파일에 write한 후 수행함으로인해,
     * DataFileDescslot의 상태가 deactive에서 active바뀌는경우 해당 페이지의
     * 변경 사항은 level0 백업에 포함된다.
     */
    if ( ( sctTableSpaceMgr::isTempTableSpace( sSpaceNode ) != ID_TRUE ) &&
         ( smLayerCallback::isCTMgrEnabled() == ID_TRUE ) )
    {
        IDE_TEST( smriChangeTrackingMgr::changeTracking4WriteDiskPage( sFileNode,
                                                                       aPageID ) 
                  != IDE_SUCCESS );
    }
    else
    {
        //nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_access_to_offline_datafile );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_MustBeDataFileOnlineMode,
                                  sFileNode->mID ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 * aFstPID ~ aFstPID + aPageCnt 까지의 페이지를 aFileNode가 가리키는 파일
 * 에 기록한다.
 *
 * PRJ-1149관련
 * 데이타 파일이 backup이 아닌 정상상태일때
 * 데이타파일에 dirty page를 write한다.
 *
 * aStatistics - [IN] 통계 정보
 * aFileNode   - [IN] FileNode
 * aFstPID     - [IN] 첫번째 PageID
 * aPageCnt    - [IN] Write Page Count
 * aBuffer     - [IN] Page내용을 가진 Buffer
 **********************************************************************/
IDE_RC sddDiskMgr::writeMultiPage2DataFile(idvSQL           *  aStatistics,
                                           sddDataFileNode  *  aFileNode,
                                           scPageID            aFstPID,
                                           ULong               aPageCnt,
                                           UChar            *  aBuffer )
{
    idBool sPreparedIO = ID_FALSE;

    IDE_DASSERT( aFileNode != NULL );
    IDE_DASSERT( aBuffer != NULL );

    IDE_TEST( prepareIO( aStatistics,
                         aFileNode ) != IDE_SUCCESS );
    sPreparedIO = ID_TRUE;

    IDE_TEST( sddDataFile::write( aStatistics,
                                  aFileNode,
                                  SD_MAKE_FOFFSET( aFstPID ),
                                  aPageCnt * SD_PAGE_SIZE ,
                                  aBuffer,
                                  smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    sPreparedIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aFileNode,
                          SDD_IO_WRITE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Write Page To Data File Failure\n"
                 "              Tablespace ID = %"ID_UINT32_FMT"\n"
                 "              File ID       = %"ID_UINT32_FMT"\n"
                 "              IO Count      = %"ID_UINT32_FMT"\n"
                 "              File Opened   = %s",
                 aFileNode->mSpaceID,
                 aFileNode->mID,
                 aFileNode->mIOCount,
                 (aFileNode->mIsOpened ? "Open" : "Close") );

    if( sPreparedIO == ID_TRUE )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aFileNode,
                                SDD_IO_READ )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 * aFstPID ~ aFstPID + aPageCnt 까지의 페이지를 aFileNode가 가리키는 파일
 * 에 기록한다.
 *
 * PRJ-1149관련
 * 데이타 파일이 backup이 아닌 정상상태일때
 * 데이타파일에 dirty page를 write한다.
 *
 * aStatistics - [IN] 통계 정보
 * aFileNode   - [IN] FileNode
 * aFstPID     - [IN] 첫번째 PageID
 * aPageCnt    - [IN] Write Page Count
 * aBuffer     - [IN] Page내용을 가진 Buffer
 **********************************************************************/
IDE_RC sddDiskMgr::writevMultiPage2DataFile( idvSQL           * aStatistics,
                                             sddDataFileNode  * aFileNode,
                                             scPageID           aFstPID,
                                             iduFileIOVec     & aVec )
{
    idBool sPreparedIO = ID_FALSE;

    IDE_DASSERT( aFileNode != NULL );

    IDE_TEST( prepareIO( aStatistics,
                         aFileNode ) != IDE_SUCCESS );
    sPreparedIO = ID_TRUE;

    IDE_TEST( sddDataFile::writev( aStatistics,
                                   aFileNode,
                                   SD_MAKE_FOFFSET( aFstPID ),
                                   aVec,
                                   smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    sPreparedIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aFileNode,
                          SDD_IO_WRITE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Write Page To Data File Failure\n"
                 "              Tablespace ID = %"ID_UINT32_FMT"\n"
                 "              File ID       = %"ID_UINT32_FMT"\n"
                 "              IO Count      = %"ID_UINT32_FMT"\n"
                 "              File Opened   = %s",
                 aFileNode->mSpaceID,
                 aFileNode->mID,
                 aFileNode->mIOCount,
                 (aFileNode->mIsOpened ? "Open" : "Close") );

    if( sPreparedIO == ID_TRUE )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aFileNode,
                                SDD_IO_READ )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}


/***********************************************************************
 * Description :
 * aFstPID ~ aFstPID + aPageCnt 까지의 페이지를 aFileNode가 가리키는 파일
 * 에 기록한다.
 *
 * PRJ-1149관련
 * 데이타 파일이 backup이 아닌 정상상태일때
 * 데이타파일에 dirty page를 write한다.
 *
 * aStatistics - [IN] 통계 정보
 * aFileNode   - [IN] FileNode
 * aFstPID     - [IN] 첫번째 PageID
 * aBuffer     - [IN] Page내용을 가진 Buffer
 **********************************************************************/
IDE_RC sddDiskMgr::writePage2DataFile(idvSQL            * aStatistics,
                                      sddDataFileNode   * aFileNode,
                                      scPageID            aPageID,
                                      UChar             * aBuffer )
{
    idBool sPreparedIO = ID_FALSE;

    IDE_DASSERT( aFileNode != NULL );
    IDE_DASSERT( aBuffer != NULL );

    IDE_TEST( prepareIO( aStatistics,
                         aFileNode ) != IDE_SUCCESS );
    sPreparedIO = ID_TRUE;

    IDE_TEST( sddDataFile::write( aStatistics,
                                  aFileNode,
                                  SD_MAKE_FOFFSET( aPageID ),
                                  SD_PAGE_SIZE ,
                                  aBuffer,
                                  smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    sPreparedIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aFileNode,
                          SDD_IO_WRITE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Write Page To Data File Failure\n"
                 "              Tablespace ID = %"ID_UINT32_FMT"\n"
                 "              File ID       = %"ID_UINT32_FMT"\n"
                 "              IO Count      = %"ID_UINT32_FMT"\n"
                 "              File Opened   = %s",
                 aFileNode->mSpaceID,
                 aFileNode->mID,
                 aFileNode->mIOCount,
                 (aFileNode->mIsOpened ? "Open" : "Close") );

    if ( sPreparedIO == ID_TRUE )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aFileNode,
                                SDD_IO_READ )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : sddDiskMgr::abortBackupTableSpace
 * PRJ-1149관련
 * server shutdown시에 하나의 DISK tablespace라도
 * backup begin 되어 있는 경우(end backup을 호출하지 않은경우)
 * page image 로그를 모두 데이타베이스에 반영하여야 하고,
 * tablespace 상태를 online으로 변경해주어야 한다.
 **********************************************************************/
void sddDiskMgr::abortBackupAllTableSpace( idvSQL*  aStatistics )
{

    sddTableSpaceNode* sCurrSpaceNode;

    sCurrSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurrSpaceNode != NULL )
    {
        IDE_ASSERT( (sCurrSpaceNode->mHeader.mState & SMI_TBS_DROPPED)
                    != SMI_TBS_DROPPED );

        if ( sctTableSpaceMgr::isDiskTableSpace( sCurrSpaceNode ) == ID_TRUE )
        {
            if ( SMI_TBS_IS_BACKUP(sCurrSpaceNode->mHeader.mState) )
            {
                abortBackupTableSpace( aStatistics,
                                       sCurrSpaceNode );
            }
        }

        sCurrSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mHeader.mID );
    }
}

/***********************************************************************
 * Description : sddDiskMgr::abortBackupTableSpace
 * veritas 연동 관련 alter tablespace begin backup에서 abort를 처리함.
 * SpaceNode Lock으로 보호 되어야 한다.
 **********************************************************************/
void sddDiskMgr::abortBackupTableSpace( idvSQL            * aStatistics,
                                        sddTableSpaceNode * aSpaceNode )
{
    sddDataFileNode*      sDataFileNode;
    UInt                  i;

    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sDataFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sDataFileNode == NULL )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sDataFileNode->mState ) )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_BACKUP( sDataFileNode->mState ) )
        {
            completeFileBackup( aStatistics,
                                aSpaceNode,
                                sDataFileNode) ;
        }
    }

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );

    aSpaceNode->mHeader.mState &= ~SMI_TBS_BACKUP;

    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
}

/***********************************************************************
 * Description : sddDiskMgr::completeFileBackup
 * - 데이타 파일 backup(즉 copy)이 종료됨에따라. DataFileNode의 상태를
 * 변경한다.
 *
 *   aStatistics      - [IN] 통계정보
 *   aDataFileNode    - [IN] 방금 backup한 DBFile의 DataFileNode
 **********************************************************************/
void sddDiskMgr::completeFileBackup( idvSQL*             aStatistics,
                                     sddTableSpaceNode * aSpaceNode,
                                     sddDataFileNode*    aDataFileNode )
{
    IDE_ASSERT( aDataFileNode    != NULL );

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );

    IDE_DASSERT( SMI_FILE_STATE_IS_BACKUP( aDataFileNode->mState ) );
    IDE_DASSERT( SMI_FILE_STATE_IS_NOT_DROPPED( aDataFileNode->mState ) );

    aDataFileNode->mState &= ~SMI_FILE_BACKUP;

    sctTableSpaceMgr::wakeup4Backup( aSpaceNode );

    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
}

/***********************************************************************
 * Description : sddDiskMgr::prepareFileBackup
 * PRJ-1149관련
 - 데이타 파일 노드의 상태를 변경한다.
  : online-> backup begin

  - File의 State를 변경 하므로 SpaceNode Mutex를 잡아야 한다.

  - 그동안 write한 것이 반영되도록 sync를 한번 해준다.
    꼭 Sync를 해 주어야 하는 것은 아니다.
    
    Open 되어 있지 않으면 그냥 넘어간다.
  
  - FileNode의 mState 는 SpaceMutex의 보호를 받아야 한다.

 **********************************************************************/
IDE_RC  sddDiskMgr::prepareFileBackup( idvSQL*            aStatistics,
                                       sddTableSpaceNode* aSpaceNode,
                                       sddDataFileNode  * aDataFileNode )
{
    UInt      sState = 0;

    sddDataFile::lockFileNode( aStatistics,
                               aDataFileNode );
    sState = 1;

    if (( aDataFileNode->mIsOpened == ID_TRUE) &&
        ( aDataFileNode->mIsModified == ID_TRUE ))
    {
        aDataFileNode->mIsModified = ID_FALSE;
        aDataFileNode->mIOCount++;

        sState = 0;
        sddDataFile::unlockFileNode( aDataFileNode );

        IDE_TEST_RAISE( sddDataFile::sync( aDataFileNode,
                                           smLayerCallback::setEmergency ) != IDE_SUCCESS,
                        error_sync_datafile);

        sddDataFile::lockFileNode( aStatistics,
                                   aDataFileNode );
        sState = 1;

        aDataFileNode->mIOCount--;
    }

    sState = 0;
    sddDataFile::unlockFileNode( aDataFileNode );

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );

    aDataFileNode->mState |= SMI_FILE_BACKUP ;

    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(error_sync_datafile);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_DISK_FILE_SYNC_ERROR);
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sddDataFile::unlockFileNode( aDataFileNode );
    }

    return IDE_FAILURE;

}

/*
  테이블스페이스의 DBF 노드들의 백업완료작업을 수행한다.


*/
IDE_RC sddDiskMgr::endBackupDiskTBS( idvSQL            * aStatistics,
                                     sddTableSpaceNode * aSpaceNode )
{
    sddDataFileNode*    sDataFileNode;
    UInt                i;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE );

    // alter tablespace  backup begin A를 하고나서,
    // alter tablespace  backup end B를 하는 경우를 막기위함이다.
    IDE_TEST_RAISE( (aSpaceNode->mHeader.mState & SMI_TBS_BACKUP)
                    != SMI_TBS_BACKUP,
                    error_not_begin_backup);

    //테이블 스페이스가 backup상태이기때문에
    // 데이타 파일 배열은 그대로 유지 된다.
    /* ------------------------------------------------
     * 테이블 스페이스의 데이타 파일에서
     * page image logging반영.
     * ----------------------------------------------*/

    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sDataFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sDataFileNode == NULL )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sDataFileNode->mState ) )
        {
            continue;
        }

        completeFileBackup( aStatistics,
                            aSpaceNode,
                            sDataFileNode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_begin_backup );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotBeginBackup,
                                  aSpaceNode->mHeader.mID ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : page 기록 (2)
 *
 * 해당 tablespace의 해당 page부터 pagecount만큼 disk로부터 write 한다.
 *
 * - 2nd. code design
 *   + 디스크관리자 mutex 획득
 *   + 디스크관리자의 HASH에서 tablespace 노드를 찾는다.
 *   + frompid 부터 topid까지 모두 기록할때까지 루프를 돈다.
 *     + 해당 page가 포함된 datafile 노드를 찾는다
 *        -> sddTableSpace::getDataFileNodeByPageID
 *     + Write I/O를 준비한다. -> prepareIO
 *     + 디스크관리자 mutex 해제
 *     + Write I/O를 수행한다.-> sddDataFile::write
 *     + 디스크관리자의 mutex 획득한다.
 *     + Write I/O 완료를 수행한다. -> completeIO
 *   + 디스크관리자의 mutex 해제한다.
 *
 * aStatistics   - [IN] 통계정보
 * aTableSpaceID - [IN] TableSpaceID
 * aFstPID       - [IN] First PID
 * aPageCount    - [IN] Write할 Page Count
 * aBuffer       - [IN] Write할 Page 내용이 들어있는 Buffer
 **********************************************************************/
IDE_RC sddDiskMgr::writeMultiPage( idvSQL      * aStatistics,
                                   scSpaceID     aTableSpaceID,
                                   scPageID      aFstPID,
                                   ULong         aPageCount,
                                   UChar       * aBuffer )
{
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID ) == ID_TRUE );
    IDE_DASSERT( aBuffer != NULL );

    sFileNode      = NULL;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode)
                  != IDE_SUCCESS );


    IDE_TEST( sddTableSpace::getDataFileNodeByPageID(sSpaceNode,
                                                     aFstPID,
                                                     &sFileNode,
                                                     ID_TRUE ) /* aFatal */
              != IDE_SUCCESS );

    IDE_TEST( writeMultiPage2DataFile( aStatistics,
                                       sFileNode,
                                       aFstPID,
                                       aPageCount,
                                       aBuffer )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * Description : page 기록 (2)
 *
 * 해당 tablespace의 해당 page부터 pagecount만큼 disk로부터 write 한다.
 *
 * - 2nd. code design
 *   + 디스크관리자 mutex 획득
 *   + 디스크관리자의 HASH에서 tablespace 노드를 찾는다.
 *   + frompid 부터 topid까지 모두 기록할때까지 루프를 돈다.
 *     + 해당 page가 포함된 datafile 노드를 찾는다
 *        -> sddTableSpace::getDataFileNodeByPageID
 *     + Write I/O를 준비한다. -> prepareIO
 *     + 디스크관리자 mutex 해제
 *     + Write I/O를 수행한다.-> sddDataFile::write
 *     + 디스크관리자의 mutex 획득한다.
 *     + Write I/O 완료를 수행한다. -> completeIO
 *   + 디스크관리자의 mutex 해제한다.
 *
 * aStatistics   - [IN] 통계정보
 * aTableSpaceID - [IN] TableSpaceID
 * aFstPID       - [IN] First PID
 * aPageCount    - [IN] Write할 Page Count
 * aBuffer       - [IN] Write할 Page 내용이 들어있는 Buffer
 **********************************************************************/
IDE_RC sddDiskMgr::writevMultiPage( idvSQL      * aStatistics,
                                    scSpaceID     aTableSpaceID,
                                    scPageID      aFstPID,
                                    iduFileIOVec& aVec )
{
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID )
                 == ID_TRUE );

    sFileNode = NULL;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode)
                  != IDE_SUCCESS );


    IDE_TEST( sddTableSpace::getDataFileNodeByPageID(sSpaceNode,
                                                     aFstPID,
                                                     &sFileNode,
                                                     ID_TRUE ) /* aFatal */
              != IDE_SUCCESS );

    IDE_TEST( writevMultiPage2DataFile( aStatistics,
                                        sFileNode,
                                        aFstPID,
                                        aVec )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : datafile에 대한 I/O를 하기 전에 해당 datafile 노드 오픈
 *
 * read, write 전에 IOCount를 증가하고,
 * 만약 open 되어 있지 않다면 open한다. 
 **********************************************************************/
IDE_RC sddDiskMgr::prepareIO( idvSQL          * aStatistics,
                              sddDataFileNode * aDataFileNode )
{
    IDE_DASSERT( aDataFileNode != NULL );

    // BUG-17158
    // Offline TBS에 대해서 Prepare IO를 막지 않는다.
    // 왜냐하면 Read와 Write를 상황에 따라 허용해야 하기 때문이다.

    sddDataFile::prepareIO( aStatistics,
                            aDataFileNode );

    if ( aDataFileNode->mIsOpened == ID_FALSE )
    {
        return openDataFile( aStatistics,
                             aDataFileNode );
    }

    if ( aDataFileNode->mNode4LRUList.mNext != NULL )
    {
        // IOCount == 0 으로 VictimList에 연결되어 있는 상태에서
        // 내가 IOCount++ 하여 1로 처음 변경한 경우이다.
        // Victim List에서 떼어낸다.
        removeNodeList( aStatistics,
                        aDataFileNode );
    }
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : datafile에 대한 I/O 완료후 datafile 노드 설정
 *
 * IO Count 감소 및 DataFile 변경 유무 갱신한다.
 * 만약 FD 가 부족한 상황이고, close 가능하면, close File 한다.
 *
 **********************************************************************/
IDE_RC sddDiskMgr::completeIO( idvSQL          * aStatistics,
                               sddDataFileNode * aDataFileNode,
                               sddIOMode         aIOMode )
{
    IDE_DASSERT( aDataFileNode != NULL );

    sddDataFile::completeIO( aStatistics,
                             aDataFileNode,
                             aIOMode );

    if ( sddDataFile::getIOCount( aDataFileNode ) == 0 )
    {
        if (( idCore::acpAtomicGet32( &mWaitThr4Open ) > 0 ) ||
            ( mOpenFileCount >= smuProperty::getMaxOpenDataFileCount() ))
        {
            IDE_TEST( closeDataFile( aStatistics,
                                     aDataFileNode ) != IDE_SUCCESS );
        }
        else
        {
            if ( aDataFileNode->mNode4LRUList.mNext == NULL )
            {
                // 이 사이에 0이 아닐수도 있다.
                // 0이 아닐 수 있어도 일단 추가한다.
                // 0인데 List에 없는 경우만 아니면 된다.
                addNodeList( aStatistics,
                             aDataFileNode );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  테이블스페이스의 Dirty된 데이타파일을 Sync한다. (WRAPPER-FUNCTION)

  [IN] aSpaceID : Sync할 디스크 테이블스페이스 ID
*/
IDE_RC sddDiskMgr::syncTBSInNormal( idvSQL *   aStatistics,
                                    scSpaceID  aSpaceID )
{
    idBool             sIsLocked = ID_FALSE;
    sctTableSpaceNode* sSpaceNode;

    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aSpaceID,
                                                               (void**)&sSpaceNode )
              != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    IDE_TEST( sddTableSpace::doActSyncTBSInNormal( aStatistics,
                                                   sSpaceNode,
                                                   NULL )
              != IDE_SUCCESS );

    sIsLocked = ID_FALSE;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if ( sIsLocked == ID_TRUE )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    return IDE_FAILURE;
}

/*
 * Dirty 되었으면 Sync한다. drop file 등을 고려하지 않는다.
 */
IDE_RC sddDiskMgr::syncFile( idvSQL *   aStatistics,
                             scSpaceID  aSpaceID,
                             sdFileID   aFileID )
{
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    idBool              sPreparedIO = ID_FALSE;
    idBool              sIsLocked   = ID_FALSE;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sSpaceNode )
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::getDataFileNodeByID( sSpaceNode,
                                                  aFileID,
                                                  &sFileNode )
              != IDE_SUCCESS );

    sddDataFile::lockFileNode( aStatistics,
                               sFileNode );
    sIsLocked = ID_TRUE;

    // 1. close되었다. close 하기 전에 sync 하였을 것이다.
    // 2. Is Modified 가 False다, 다른 누군가가 Sync 하였다.
    if (( sFileNode->mIsOpened   == ID_TRUE ) &&
        ( sFileNode->mIsModified == ID_TRUE ))
    {
        // 다른 이의 sync를 막기 위해 modified를 먼저 변경한다.
        sFileNode->mIsModified = ID_FALSE;
        sFileNode->mIOCount++;
        sPreparedIO = ID_TRUE;

        sIsLocked = ID_FALSE;
        sddDataFile::unlockFileNode( sFileNode );
        

        IDE_TEST( sddDataFile::sync( sFileNode,
                                     smLayerCallback::setEmergency ) != IDE_SUCCESS );

        // 반드시 DiskMgr로 completeIO 하여야 Victim List에 연결 된다.
        sPreparedIO = ID_FALSE;
        completeIO( aStatistics,
                    sFileNode,
                    SDD_IO_READ );
    }
    else
    {
        sIsLocked = ID_FALSE;
        sddDataFile::unlockFileNode( sFileNode );        
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        sddDataFile::unlockFileNode( sFileNode ); 
    }

    if ( sPreparedIO == ID_TRUE )
    {
        completeIO( aStatistics,
                    sFileNode,
                    SDD_IO_READ );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * 테이블스페이스의 데이타파일 메타헤더에 체크포인트정보를 갱신한다.
 * 본 함수에 호출되기전에 TBS Mgr Latch는 획득된 상태이다.
 *
 *   aSpaceNode - [IN] Sync할 TBS Node
 *   aActionArg - [IN] NULL
 **********************************************************************/
IDE_RC sddDiskMgr::doActUpdateAllDBFHdrInChkpt(
                                        idvSQL            * aStatistics,
                                        sctTableSpaceNode * aSpaceNode,
                                        void              * aActionArg )
{
    sddTableSpaceNode*  sSpaceNode;
    sddDataFileNode*    sFileNode;
    smLSN               sInitLSN;
    UInt                i;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aActionArg == NULL );
    ACP_UNUSED( aActionArg );

    SM_LSN_INIT( sInitLSN );

    while ( 1 )
    {
        if ( (sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) != ID_TRUE) ||
             (sctTableSpaceMgr::isTempTableSpace( aSpaceNode ) == ID_TRUE) )
        {
            // 디스크테이블스페이스가 아닌경우 갱신하지 않음.
            // 임시 테이블스페이스의 경우 갱신하지 않음.
            break;
        }

        if ( sctTableSpaceMgr::hasState( aSpaceNode,
                                         SCT_SS_SKIP_UPDATE_DBFHDR )
             == ID_TRUE )
        {
            // 메모리테이블스페이스가 DROPPED/DISCARDED 경우
            // 갱신하지 않음.
            break;
        }

        sSpaceNode = (sddTableSpaceNode*)aSpaceNode;

        for (i=0; i < sSpaceNode->mNewFileID ; i++ )
        {
            sFileNode = sSpaceNode->mFileNodeArr[i] ;

            if( sFileNode == NULL )
            {
                continue;
            }

            // PRJ-1548 User Memory Tablespace
            // 트랜잭션 Pending 연산에서 DBF Node가 DROPPED로 설정된 후에
            // TBS List Latch를 해제하면,
            // DBF Sync에서 DBF Node의 상태를 확인하여 sync를 무시하도록
            // 처리할 수 있다.
            if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
            {
                continue;
            }

            // sync할 dbf 노드에 checkpoint 정보 설정
            // DiskCreateLSN은 설정할 필요없다.
            sddDataFile::setDBFHdr(
                            &(sFileNode->mDBFileHdr),
                            sctTableSpaceMgr::getDiskRedoLSN(),
                            NULL ,  // DiskCreateLSN
                            &sInitLSN,
                            NULL ); //DataFileDescSlotID

            IDE_ASSERT( sddDataFile::checkValuesOfDBFHdr(
                                     &(sFileNode->mDBFileHdr) )
                        == IDE_SUCCESS );

            IDE_ASSERT( writeDBFHdr( aStatistics,
                                     sFileNode ) == IDE_SUCCESS );
        }

        break;
    }

    return IDE_SUCCESS;
}


/*
  디스크 테이블스페이스 노드들의 모든 Dirty된 데이타파일 노드들을 Sync
  하거나, 체크포인트 연산중에 디스크 데이타파일의 메타헤더에 체크포인트
  정보를 갱신한다.

  [IN] aSyncType : 운영중이거나 체크포인트연산에 따른 Sync타입

*/
IDE_RC sddDiskMgr::syncAllTBS( idvSQL  *   aStatistics,
                               sddSyncType aSyncType )
{
    if ( aSyncType == SDD_SYNC_NORMAL)
    {
        IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                      aStatistics,
                      sddTableSpace::doActSyncTBSInNormal,
                      NULL, /* Action Argument*/
                      SCT_ACT_MODE_LATCH ) != IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( aSyncType == SDD_SYNC_CHKPT );

        IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                      aStatistics,
                      doActUpdateAllDBFHdrInChkpt,
                      NULL, /* Action Argument*/
                      SCT_ACT_MODE_LATCH ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  서버구동시에 모든 디스크 테이블스페이스의 DBFile들의
  미디어 복구 필요 여부를 체크한다.
*/
IDE_RC sddDiskMgr::identifyDBFilesOfAllTBS( idvSQL *    aStatistics,
                                            idBool      aIsOnCheckPoint )
{
    sctActIdentifyDBArgs sIdentifyDBArgs;

    sIdentifyDBArgs.mIsValidDBF  = ID_TRUE;
    sIdentifyDBArgs.mIsFileExist = ID_TRUE;

    if ( aIsOnCheckPoint == ID_FALSE )
    {
        // 서버구동시
        IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                      aStatistics,
                      doActIdentifyAllDBFiles,
                      &sIdentifyDBArgs, /* Action Argument*/
                      SCT_ACT_MODE_NONE ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIdentifyDBArgs.mIsFileExist != ID_TRUE,
                        err_file_not_found );

        IDE_TEST_RAISE( sIdentifyDBArgs.mIsValidDBF != ID_TRUE,
                        err_need_media_recovery );
    }
    else
    {
        // 체크포인트과정에서 런타임헤더가 파일에
        // 제대로 기록이 되었는지 데이타파일을 다시 읽어서
        // 검증한다.
        IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                      aStatistics,
                      doActIdentifyAllDBFiles,
                      &sIdentifyDBArgs, /* Action Argument*/
                      SCT_ACT_MODE_LATCH ) != IDE_SUCCESS );

        IDE_ASSERT( sIdentifyDBArgs.mIsValidDBF  == ID_TRUE );
        IDE_ASSERT( sIdentifyDBArgs.mIsFileExist == ID_TRUE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_found );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistFile ) );
    }
    IDE_EXCEPTION( err_need_media_recovery );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NeedMediaRecovery ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : datafile 생성 (노드 생성 포함) 또는 로그앵커에
 *               의한 datafile 노드 생성
 *
 * - NOMAL 모드 : 하나의 datafile을 create, open 후, tablespace 노드의
 * datafile 리스트에 추가한다. 단, datafile을 create할 때는 화일을
 * 오픈하지 않을 것이다.
 * - FILE 모드 : 하나의 datafile 노드를 tablespace 노드에 추가하고,
 * datafile을 실제 create하지는 않는다. 로그앵커에 저장된 datafile 노드
 *  정보를 초기화할때 호출되며, datafile 노드 추가후에 file을 오픈하지 않는다.
 *
 * + 2nd. code design
 *   - 임시적인 dummy tablespace 노드를 선언하고 초기화한다.
 *   - dummy tablespace 노드에 datafile들을 생성하고 리스트를 구성한다.
 *     -> sddTableSpace::createDataFiles
 *   - 디스크관리자의 mutex 획득한다.
 *   - HASH에서 해당 tablespace 노드를 찾는다.
 *   - dummy tablespace 노드의 datafile 노드 리스트를
 *     실제 tablespace 노드의 datafile 노드 리스트에 추가한다.
 *     ->sddTableSpace::addDataFileNode
 *   - if ( aMode != SMI_EACH_BYMODE )
 *     {
 *        로그앵커 FLUSH
 *     }
 *   - 디스크관리자의 mutex 해제한다.
 **********************************************************************/
IDE_RC sddDiskMgr::createDataFiles( idvSQL           * aStatistics,
                                    void             * aTrans,
                                    scSpaceID          aTableSpaceID,
                                    smiDataFileAttr ** aDataFileAttr,
                                    UInt               aDataFileAttrCount,
                                    smiTouchMode       aTouchMode )
{
    UInt                sState              = 0;
    UInt                sChkptBlockState    = 0;
    UInt                sTBSState;
    smiTableSpaceAttr   sDummySpaceAttr;
    sddTableSpaceNode   sDummySpaceNode;
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    UInt                i;
    sctPendingOp      * sPendingOp;
    sdFileID            sNewFileIDStart;
    sdFileID            sNewFileIDEnd;
    idBool              sNameExist      = ID_FALSE;
    SChar             * sExistFileName  = NULL;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID ) == ID_TRUE );
    IDE_DASSERT( aDataFileAttr != NULL );
    IDE_DASSERT( (aTouchMode == SMI_ALL_NOTOUCH) ||
                 (aTouchMode == SMI_EACH_BYMODE) );

    sTBSState  = 0;

    // fix BUG-16467 for avoid deadlock MutexForTBS->mMutex
    IDE_TEST( sctTableSpaceMgr::lockForCrtTBS() != IDE_SUCCESS );
    sTBSState = 1;

    /* ===================================
     * [1] dummy tablespace 생성 및 초기화
     * =================================== */

    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aTableSpaceID,
                                                               (void**)&sSpaceNode )
                  != IDE_SUCCESS );
    sState = 1;

    /*
     * BUG-18044
     *
     * 동일한 데이터파일명이 동일한 disk tbs에 add하는것이
     * 성공하는 문제
     */
    IDE_TEST( validateDataFileName( sSpaceNode,
                                    aDataFileAttr,
                                    aDataFileAttrCount,
                                    &sExistFileName,//에러메시지 출력용
                                    &sNameExist ) != IDE_SUCCESS);

    IDE_TEST_RAISE( sNameExist == ID_TRUE,
                    error_the_filename_is_in_use );

    // PRJ-1149.
    IDE_TEST_RAISE( SMI_TBS_IS_BACKUP(sSpaceNode->mHeader.mState),
                   error_forbidden_op_while_backup);

    // 새로운 파일들을 만들수 있는지 체크한다.
    if((sSpaceNode->mNewFileID + aDataFileAttrCount) >= SD_MAX_FID_COUNT )
    {
        IDE_RAISE( error_can_not_add_datafile_node );
    }

    sNewFileIDStart = sSpaceNode->mNewFileID ;
    sSpaceNode->mNewFileID += aDataFileAttrCount;
    sNewFileIDEnd   = sSpaceNode->mNewFileID;

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    idlOS::memset(&sDummySpaceAttr, 0x00, ID_SIZEOF(smiTableSpaceAttr));
    idlOS::memset(&sDummySpaceNode, 0x00, ID_SIZEOF(sddTableSpaceNode));
    sDummySpaceAttr.mID = aTableSpaceID;
    sDummySpaceAttr.mAttrType = SMI_TBS_ATTR;

    IDE_TEST( sddTableSpace::initialize(&sDummySpaceNode,
                                        &sDummySpaceAttr) != IDE_SUCCESS );

    sDummySpaceNode.mNewFileID = sNewFileIDStart ;

    // PROJ-2133 incremental backup
    // DataFileDescSlot이 할당되어 checkpoint가 발생하면 CTBody가 flush되어
    // 파일에 반영된다. 하지만 loganchor에 DataFileDescSlotID가 저장되기 전에
    // 서버가 죽는다면 할당된 DataFileDescSlot은 사용할수 없게된다.
    //
    // 즉, logAnchor에 DataFileDescSlotID가 저장되기 전까지 CTBody가 flush 되면
    // 안된다.
    if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
    {
        IDE_TEST( smLayerCallback::blockCheckpoint() != IDE_SUCCESS ); 
        sChkptBlockState = 1; 
    }

    /* ===================================
     * [2] 물리적 데이타 화일 생성
     * =================================== */
    IDE_TEST( sddTableSpace::createDataFiles( aStatistics,
                                              aTrans,
                                              &sDummySpaceNode,
                                              aDataFileAttr,
                                              aDataFileAttrCount,
                                              aTouchMode,
                                              mMaxDataFilePageCount )
              != IDE_SUCCESS );

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     sSpaceNode );
    sState = 1;
    /* ====================================================================
     * [3] 데이타 화일들을 자신이 속한 테이블 스페이스의 화일 리스트에 등록
     * ==================================================================== */

    // fix BUG-16116 [PROJ-1548] sddDiskMgr::createDataFiles()
    // 에러시 DBF Node에 대한 Memory 해제
    // 다음 For loop 안에서 EXCEPTION이 발생하게 되면 DBF Node
    // 메모리 해제에 대한 문제가 있으므로 exception 발생코드는
    // 코딩하지 않는다.
    // 이 이후에 Exception이 발생한다면, 트랜잭션 rollback에 의해서
    // 자동으로 해제될 것이다.

    for (i = sNewFileIDStart ; i < sNewFileIDEnd ; i++ )
    {
        sFileNode = sDummySpaceNode.mFileNodeArr[i] ;

        IDE_ASSERT( sFileNode != NULL);

        sFileNode->mState |= SMI_FILE_CREATING;
        sFileNode->mState |= SMI_FILE_ONLINE;

        sddTableSpace::addDataFileNode( sSpaceNode, sFileNode );
    }

    for ( i = sNewFileIDStart ; i < sNewFileIDEnd ; i++ )
    {
        sFileNode = sSpaceNode->mFileNodeArr[i] ;

        IDE_ASSERT( sFileNode != NULL);

        IDE_TEST( sddDataFile::addPendingOperation(
                    aTrans,
                    sFileNode,
                    ID_TRUE, /* commit시에 동작 */
                    SCT_POP_CREATE_DBF,
                    &sPendingOp )
                != IDE_SUCCESS );

        sPendingOp->mTouchMode = aTouchMode;

        /*
          [4] 로그 앵커 변경
              시스템 가동시 로그 앵커의 내용을 이용하여
              데이타 화일 리스트를 구성하며,
              이 경우에는 로그 앵커를 재변경하지 않는다.
        */
        if ( aTouchMode != SMI_ALL_NOTOUCH )
        {
            IDU_FIT_POINT( "1.PROJ-1552@sddDiskMgr::createDataFiles" );

            IDE_ASSERT( smLayerCallback::addDBFNodeAndFlush( sSpaceNode,
                                                             sFileNode )
                        == IDE_SUCCESS );

            IDU_FIT_POINT( "2.PROJ-1552@sddDiskMgr::createDataFiles" );
        }
        else
        {
            // nothing to do...
        }
    }

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    //PROJ-2133 incremental backup
    if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
    {
        sChkptBlockState = 0; 
        IDE_TEST( smLayerCallback::unblockCheckpoint() != IDE_SUCCESS ); 
    }

    sTBSState = 0;
    IDE_TEST( sctTableSpaceMgr::unlockForCrtTBS() != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::destroy( &sDummySpaceNode ) != IDE_SUCCESS );

    IDU_FIT_POINT( "1.BUG-31608@sddDiskMgr::createDataFiles" );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_forbidden_op_while_backup);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_forbiddenOpWhileBackup,
                                "ADD DATAFILE"));
    }

    IDE_EXCEPTION( error_can_not_add_datafile_node );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CANNOT_ADD_DataFile ));
    }

    IDE_EXCEPTION( error_the_filename_is_in_use )
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_UseFileInTheTBS,
                                 sExistFileName,
                                 ((sctTableSpaceNode *)sSpaceNode)->mName));
    }

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    if( sChkptBlockState != 0 )
    {
        IDE_ASSERT( smLayerCallback::unblockCheckpoint() 
                    == IDE_SUCCESS ); 
    }

    if ( sTBSState != 0 )
    {
        IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS()
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그앵커로부터 datafile 노드 생성
 **********************************************************************/
IDE_RC sddDiskMgr::loadDataFileNode( idvSQL          * aStatistics,
                                     smiDataFileAttr * aDataFileAttr,
                                     UInt              aAnchorOffset  )
{

    UInt               sState = 0;
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_DASSERT( aDataFileAttr != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aDataFileAttr->mSpaceID )
                 == ID_TRUE );

    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aDataFileAttr->mSpaceID,
                                                               (void**)&sSpaceNode )
                  != IDE_SUCCESS );
    sState = 1;

    IDE_DASSERT( sSpaceNode->mHeader.mID == aDataFileAttr->mSpaceID );

    /* ===================================
     * [2] 데이타 화일 노드 생성
     * =================================== */
    IDE_TEST( sddTableSpace::createDataFiles( aStatistics, // statistics
                                              NULL, // transaction
                                              sSpaceNode,
                                              &aDataFileAttr,
                                              1,
                                              SMI_ALL_NOTOUCH,
                                              mMaxDataFilePageCount )
              != IDE_SUCCESS );

    // 할당된 DBF 노드를 검색한다.
    IDE_TEST( sddTableSpace::getDataFileNodeByID(
                                sSpaceNode,
                                aDataFileAttr->mID,
                                &sFileNode ) != IDE_SUCCESS );

    // Loganchor 오프셋을 설정한다.
    sFileNode->mAnchorOffset = aAnchorOffset;

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    return IDE_SUCCESS;


    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : datafile 열기 및 LRU 리스트에 datafile 노드 추가
 *
 * 해당 datafile 노드의 file을 open한다.
 * Open FileNode Count 가 부족 할 때 에는
 * 다른 오픈된 FileNode를 Close 하고 재시도한다.
 *
 **********************************************************************/
IDE_RC sddDiskMgr::openDataFile( idvSQL          * aStatistics,
                                 sddDataFileNode * aDataFileNode )
{
    UInt    sLockList = 0;
    UInt    sLockNode = 0;
    UInt    sOpenFileCountInc = 0;
    idBool  sIsDirectIO = isDirectIO() ;

    if ( sIsDirectIO == ID_TRUE )
    {
        /* BUG-47398 AIX 에서 iduMemMgr::free4malign() 함수에서 비정상 종료
         * Temp Table은 DirectIO를 사용하지 않는다. */
        if ( sctTableSpaceMgr::isTempTableSpace( aDataFileNode->mSpaceID ) == ID_TRUE )
        {
            sIsDirectIO = ID_FALSE;
        }
    }
    
    while( aDataFileNode->mIsOpened == ID_FALSE )
    {
        // Open File Count가 Property값을 넘어서는지 확인하기 위해 Mutex를 잡는다.
        // Count를 줄이는 시점에는 Lock을 잡지 않는다.
        lockIncOpenFileCount( aStatistics );
        sLockList = 1;

        if ( mOpenFileCount < smuProperty::getMaxOpenDataFileCount() )
        {
            // 먼저 count를 올린다.
            // Max 값 때문에 Lock안에서 해야 한다.
            // Dec 때문에 Atomic 안에서 해야 한다.
            idCore::acpAtomicInc32( &mOpenFileCount );
            sOpenFileCountInc = 1;

            sLockList = 0;
            unlockIncOpenFileCount();

            sddDataFile::lockFileNode( aStatistics,
                                       aDataFileNode );
            sLockNode = 1;

            // Open은 동시에 들어 올 수 있다. File Node Mute로 보호한다.
            // Lock을 잡고도 확인해본다.
            if ( aDataFileNode->mIsOpened == ID_FALSE )
            {
                IDE_TEST( sddDataFile::open( aDataFileNode,
                                             sIsDirectIO ) != IDE_SUCCESS );
                sOpenFileCountInc = 0;

                sLockNode = 0 ;
                sddDataFile::unlockFileNode( aDataFileNode );
            }
            else
            {
                sLockNode = 0 ;
                sddDataFile::unlockFileNode( aDataFileNode );

                sOpenFileCountInc = 0;
                idCore::acpAtomicDec32( &mOpenFileCount );
            }
        }
        else
        {
            sLockList = 0;
            unlockIncOpenFileCount( );

            // 갯수가 줄어들거나 aDataFile 을 다른이가 open했으면 반환
            IDE_TEST( closeDataFileInList( aStatistics,
                                           aDataFileNode ) != IDE_SUCCESS );
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLockList == 1 )
    {
        unlockIncOpenFileCount();
    }

    if( sLockNode == 1 )
    {
        sddDataFile::unlockFileNode( aDataFileNode );
    }

    if( sOpenFileCountInc == 1 )
    {
        idCore::acpAtomicDec32( &mOpenFileCount );        
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : datafile 닫기 및 LRU 리스트로부터 datafile 노드 제거
 *               Lock을 잡는다.
 **********************************************************************/
IDE_RC sddDiskMgr::closeDataFile( idvSQL          * aStatistics,
                                  sddDataFileNode * aDataFileNode )
{
    UInt sNodeLock = 0;

    IDE_DASSERT( aDataFileNode != NULL );

    sddDataFile::lockFileNode( aStatistics,
                               aDataFileNode );
    sNodeLock = 1;

    IDE_TEST( closeDataFileInternal( aStatistics,
                                     aDataFileNode) != IDE_SUCCESS );

    sNodeLock = 0 ;
    sddDataFile::unlockFileNode( aDataFileNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sNodeLock == 1 )
    {
        sddDataFile::unlockFileNode( aDataFileNode );   
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : datafile 닫기 및 LRU 리스트로부터 datafile 노드 제거
 *               File Lock을 잡은 상태로 호출된다.
 **********************************************************************/
IDE_RC sddDiskMgr::closeDataFileInternal( idvSQL          * aStatistics,
                                          sddDataFileNode * aDataFileNode )
{
    if (( aDataFileNode->mIsOpened == ID_TRUE ) &&
        ( aDataFileNode->mIOCount == 0 ))
    {
        IDE_TEST( sddDataFile::close( aDataFileNode ) != IDE_SUCCESS );

        // OpenFileCount를 증가하기 위해서는 Lock을 잡아야 하지만
        // 줄이는데는 Lock을 잡지 않는다.
        idCore::acpAtomicDec32( &mOpenFileCount );

        removeNodeList( aStatistics,
                        aDataFileNode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * Description : 모드에 따라 datafile 제거 ( 노드 제거 포함 )
 *
 * DDL인 alter tablespace add datafile 또는 drop datafile의 연산을 수행할
 * 때 호출된다.
 *
 * - SMI_ALL_NOTOUCH 모드 : 해당 datafile 노드만 제거한다.
 * - SMI_ALL_TOUCH 모드 : 해당 datafile 노드 및 실제 datafile을 삭제한다.
 *
 * + 2nd. code design
 *   - 디스크관리자 mutex 획득
 *   - HASH에서 해당 tablespace 노드를 찾는다. 못찾은 경우 에러를 리턴한다.
 *
 *   - tablespace 노드의 datafile 리스트에서 해당 datafile을 제거한다.
 *   - 로그앵커 FLUSH
 *   - 디스크관리자의 mutex 해제한다.
 **********************************************************************/
IDE_RC sddDiskMgr::removeDataFileFEBT( idvSQL             * aStatistics,
                                       void               * aTrans,
                                       sddTableSpaceNode  * aSpaceNode,
                                       sddDataFileNode    * aFileNode,
                                       smiTouchMode         aTouchMode )
{
    UInt        sState  = 0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTouchMode != SMI_EACH_BYMODE );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE );

    IDU_FIT_POINT( "1.BUG-31608@sddDiskMgr::removeDataFileFEBT" );

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );
    sState = 1;

    IDE_TEST_RAISE( SMI_TBS_IS_BACKUP(aSpaceNode->mHeader.mState),
                   error_forbidden_op_while_backup);

    // PROJ-1548
    // 데이타파일 노드에 대한 DROP 연산이 TBS Node (X) -> TBS META PAGE에 (S)
    // 잠금을 획득하기 때문에 DROP DBF 연산과 동시에 수행될 수 없다.
    // 다음과 같은 조건을 체크해보아야 한다.
    // DBF Node가 DROPPED 상태라면 Exception 처리한다.
    IDE_TEST_RAISE( SMI_FILE_STATE_IS_DROPPED( aFileNode->mState ),
                    error_not_found_datafile_node );

    /* ==================================================
     * 물리적 데이타 화일 삭제
     * removeDataFile은 on/offline에 상관없이 usedpagelimit을
     * 포함한 노드의 이후 노드에서만 가능해야한다.
     * ================================================== */
    IDE_TEST( sddTableSpace::removeDataFile(aStatistics,
                                            aTrans,
                                            aSpaceNode,
                                            aFileNode,
                                            aTouchMode,
                                            ID_TRUE /* aDoGhostMark */)
              != IDE_SUCCESS );

    IDE_ASSERT( sddTableSpace::getTotalPageCount(aSpaceNode) > 0 );

    // PROJ-1548
    // DROP DBF 중간에는 Loganchor를 플러쉬하지 않는다.

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_datafile_node );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_NotFoundDataFileNode,
                                 aFileNode->mName ));
    }
    IDE_EXCEPTION(error_forbidden_op_while_backup);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_forbiddenOpWhileBackup,
                                 "DROP DATAFILE" ));
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    }

    return IDE_FAILURE;

}
/******************************************************************************
 * PRJ-1548 User Memory Tablespace
 *
 * DROP DBF 에 대한 Pending 연산을 수행한다.
 *
 * [IN] aSpaceNode : TBS Node
 * [IN] aPendingOp : Pending 구조체
 *****************************************************************************/
IDE_RC sddDiskMgr::removeFilePending(
                            idvSQL            * aStatistics,
                            sctTableSpaceNode * aSpaceNode,
                            sctPendingOp      * aPendingOp  )
{
    UInt                    sState  = 0;
    sddDataFileNode       * sFileNode;
    scPageID                sFstPID;
    scPageID                sLstPID;
    smiDataFileDescSlotID   sDataFileDescSlotID;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aPendingOp != NULL );
    IDE_ASSERT( aPendingOp->mPendingOpParam != NULL );

    sFileNode  = (sddDataFileNode *)aPendingOp->mPendingOpParam;

    /* PRJ-1548 User Memory Tablespace
     * DBF Node의 상태가 파일삭제보다 loganchor에 선반영되는 것은 다음을 보장한다.
     *
     * " loganchor에 DROPPED 상태인 경우는 반드시 파일이 존재하지 않는다 "
     *
     * RESTART RECOVERY를 수해하는 경우에 COMMIT PENDING 연산을 지원해서
     * ONLINE | DROPPING 에 있는 완료된 트랜잭션이 있었을 경우에 -> DROPPED로
     * 처리해주어야 하며, 파일도 존재한다면 함께 삭제해야한다.
     * 하지만, DROPPED 상태는 loganchor에 저장되지 않기때문에
     * 서버구동시 초기화되지 않으며, 이와 관련된 로그들은 모두 무시되지만,
     * DROP 연산에 대한 pending연산을 수행하기 위해 pending 연산 처리를 하여야
     * 한다. */

    /* DROP TBS Node시에 TBS Node에 X 잠금을 획득하기 때문에 파일
     * 접근을 할수 없는 상태이고, 해당 파일들을 close하고 buffer 상에서 관련 page를
     * 모두 무효화시키기 때문에 파일이 open되어 있을 수는 없지만,
     * sync checkpoint 발생으로 인해 open이 되어 있을 수 있다 */

    /* fix BUG-13125 DROP TABLESPACE시에 관련 페이지들을 Invalid시켜야 한다.
     * 버퍼상에 적재된 관련 페이지를 무효화 시킨다. */
    IDE_TEST( getPageRangeInFileByID( aStatistics,
                                      sFileNode->mSpaceID,
                                      sFileNode->mID,
                                      &sFstPID,
                                      &sLstPID ) != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::discardPagesInRangeInSBuffer(
                  aStatistics,
                  sFileNode->mSpaceID,
                  sFstPID,
                  sLstPID )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::discardPagesInRange(
                  aStatistics,
                  sFileNode->mSpaceID,
                  sFstPID,
                  sLstPID )
              != IDE_SUCCESS );

    /* BUG-24129: Pending연산으로 File삭제 연산을 완료후 Buffer Flusher가 IOB
     *            에 복사된 삭제된 File의 Page를 내리는 경우 발생
     *
     * File 삭제 Pending연산시 BufferPool에 있는 File에 관련된 모든 페이지를
     * Buffer에서 제거하지만 Flusher의 IOB에 이미 복사된 BCB는 제거하지 못한다.
     * 하여 BufferPool에서 File에 관련된 모든 페이지를 제거후 Flusher에 복사된
     * BCB가 존재할지도 모르기때문에 Flusher가 작업중인 Flush작업이 한번 종료
     * 될때까지 여기서 대기한다. */
    smLayerCallback::wait4AllFlusher2Do1JobDoneInSBuffer();

    smLayerCallback::wait4AllFlusher2Do1JobDone();

retry:
    // Dead Lock 방지를 위해 SpaceNode -> FileNode 순으로 Mutex를 잡아야 한다.
    // FileNode->mState를 갱신하기 위해 SpaceNodeMutex를 잡아야 하는데
    // FileNode Mutex를 잡기 전에 미리 잡는다.
    sctTableSpaceMgr::lockSpaceNode( aStatistics ,
                                     aSpaceNode );
    sState = 1;

    sddDataFile::lockFileNode( aStatistics,
                               sFileNode );
    sState = 2;

    if ( sFileNode->mIOCount != 0 )
    {
        sState = 1;
        sddDataFile::unlockFileNode( sFileNode );

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );

        /* checkpoint sync 연산중이므로 대기하다 재시도 한다. */
        idlOS::sleep(1);

        goto retry;
    }
    else
    {
        /* DROP commit중인 연산에 대해서 생각할 수 있는 상황은
         * checkpoint sync 연산으로만 파일이 오픈될수 있다.
         * checkpoint sync 연산중이 아니므로 CLOSE 한다. */
    }

    IDE_TEST( closeDataFileInternal( aStatistics,
                                     sFileNode ) != IDE_SUCCESS );

    sFileNode->mState = SMI_FILE_DROPPED;

    sState = 1;
    sddDataFile::unlockFileNode( sFileNode );

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    /* BUG-24086: [SD] Restart시에도 File이나 TBS에 대한 상태가 바뀌었을 경우
     * LogAnchor에 상태를 반영해야 한다.
     *
     * Restart Recovery시에는 updateDBFNodeAndFlush하지 않던것을 하도록 변경.
     * */
    if ( sFileNode->mAnchorOffset != SCT_UNSAVED_ATTRIBUTE_OFFSET )
    {
        IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                    == IDE_SUCCESS );
    }

    lockGlobalPageCountCheckMutex( aStatistics );
    sState = 3;

    if ( aPendingOp->mTouchMode == SMI_ALL_TOUCH )
    {
        if( idf::unlink((SChar *)(sFileNode->mName)) != 0 )
        {
            ideLog::log(SM_TRC_LOG_LEVEL_TRANS,
                        SM_TRC_TRANS_DBFILE_UNLINK_ERROR,
                        (SChar *)(sFileNode->mName), errno);
        }
        else
        {
            //======================================================
            // To Fix BUG-13924
            //======================================================
            ideLog::log(SM_TRC_LOG_LEVEL_TRANS,
                        SM_TRC_TRANS_DBFILE_UNLINK,
                        (SChar*)sFileNode->mName );
        }
    }
    else // SMI_ALL_NOTOUCH, SMI_EACH_BYMODE
    {
        if( aPendingOp->mTouchMode == SMI_EACH_BYMODE )
        {
            if( sFileNode->mCreateMode == SMI_DATAFILE_CREATE )
            {
                if( idf::unlink((SChar *)(sFileNode->mName)) != 0 )
                {
                    ideLog::log(SM_TRC_LOG_LEVEL_TRANS,
                                SM_TRC_TRANS_DBFILE_UNLINK_ERROR,
                                (SChar *)(sFileNode->mName), errno);
                }
                else
                {
                    //======================================================
                    // To Fix BUG-13924
                    //======================================================
                    ideLog::log(SM_TRC_LOG_LEVEL_TRANS,
                                SM_TRC_TRANS_DBFILE_UNLINK,
                                (SChar*)sFileNode->mName );
                }
            }
            else //SMI_DATAFILE_REUSE
            {
                /* nothing to do */
            }
        }
        else // SMI_ALL_NOTOUCH
        {
            /* nothing to do */
        }
    }

    /* PROJ-2133 incremental backup */
    if ( ( smLayerCallback::isCTMgrEnabled() == ID_TRUE) &&
         ( sctTableSpaceMgr::isTempTableSpace( sFileNode->mSpaceID ) != ID_TRUE ) )
    {
        if( sFileNode->mAnchorOffset != SCT_UNSAVED_ATTRIBUTE_OFFSET )
        {
            /* logAnchor에 DataFileDescSlotID가 저장된 상황 처리 */
            IDE_TEST( smLayerCallback::getDataFileDescSlotIDFromLogAncho4DBF(
                            sFileNode->mAnchorOffset,
                            &sDataFileDescSlotID )
                      != IDE_SUCCESS );

            /* DataFileDescSlot이 이미 할당해제 된경우 */
            IDE_TEST_CONT( smriChangeTrackingMgr::isAllocDataFileDescSlotID( 
                            &sDataFileDescSlotID ) != ID_TRUE,
                      already_dealloc_datafiledesc_slot );
 
            /* DataFileDescSlot이 이미 할당해제 되었으나 loganchor에는
             * DataFileDescSlotID가 남아있는경우 확인 */
            IDE_TEST_CONT( smriChangeTrackingMgr::isValidDataFileDescSlot4Disk(
                            sFileNode, &sDataFileDescSlotID ) != ID_TRUE,
                            already_reuse_datafiledesc_slot); 
        }
        else
        {
            /* logAnchor에 DataFileDescSlotID가 저장되지 않은상황 처리
             * 서버 비정상종료가 아닌 rollback시점에만 해당되는 상황이다. */

            /* DataFileDescSlot이 이미 할당해제 된경우 */
            IDE_TEST_CONT( smriChangeTrackingMgr::isAllocDataFileDescSlotID(
                                &sFileNode->mDBFileHdr.mDataFileDescSlotID )
                          != ID_TRUE,
                          already_dealloc_datafiledesc_slot );
            
            sDataFileDescSlotID.mBlockID = 
                            sFileNode->mDBFileHdr.mDataFileDescSlotID.mBlockID;
            sDataFileDescSlotID.mSlotIdx = 
                            sFileNode->mDBFileHdr.mDataFileDescSlotID.mSlotIdx;
        }

        IDE_TEST( smriChangeTrackingMgr::deleteDataFileFromCTFile( 
                        &sDataFileDescSlotID ) != IDE_SUCCESS );

        IDE_EXCEPTION_CONT( already_reuse_datafiledesc_slot );

        sFileNode->mDBFileHdr.mDataFileDescSlotID.mBlockID = 
                                            SMRI_CT_INVALID_BLOCK_ID;
        sFileNode->mDBFileHdr.mDataFileDescSlotID.mSlotIdx = 
                                            SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;

        if( sFileNode->mAnchorOffset != SCT_UNSAVED_ATTRIBUTE_OFFSET )
        {
            IDE_TEST( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                      != IDE_SUCCESS );    
        }
        else
        {
            /* nothing to do */
        }

        IDE_EXCEPTION_CONT( already_dealloc_datafiledesc_slot );
    }
    else
    {
        /* nothing to do */
    }

    sState = 0;
    unlockGlobalPageCountCheckMutex();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            unlockGlobalPageCountCheckMutex();
            break;
        case 2:
            sddDataFile::unlockFileNode( sFileNode );
        case 1:
            sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/******************************************************************************
 *  - PROJ-1671 Bitmap-based Tablespace And Segment Space Management
 *
 *  NextSize만큼 늘릴수있는 유효한 파일을 찾는다.
 *  [IN ]
 *  [OUT]  sFileNode
 *****************************************************************************/
void sddDiskMgr::getExtendableSmallestFileNode( sddTableSpaceNode  * aSpaceNode,
                                                sddDataFileNode   ** aFileNode )
{

    UInt                i;
    UInt                sMinSize= ID_UINT_MAX;
    sddDataFileNode   * sMinSizeFileNode = NULL;
    sddDataFileNode   * sFileNodeTemp = NULL;
    UInt                sTempSize;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aFileNode != NULL );

    //XXX5 sort 사용
    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNodeTemp = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNodeTemp == NULL )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sFileNodeTemp->mState ) )
        {
            continue;
        }

        if(sFileNodeTemp->mIsAutoExtend == ID_TRUE)
        {
            /*
             * curr크기가 가장작은 파일중 next를 더했을때  max를 넘지 않는 파일
             * 을 찾는다.
             */
            sTempSize = sFileNodeTemp->mCurrSize ;

            if( sMinSize > sTempSize )
            {
                if( (sTempSize + sFileNodeTemp->mNextSize)
                    <= sFileNodeTemp->mMaxSize)
                {
                    sMinSizeFileNode = sFileNodeTemp;
                    sMinSize =sFileNodeTemp->mCurrSize;
                }

            }
        }

    }

    *aFileNode = sMinSizeFileNode;
}


/*
 *  - sdptb를 위한 extendDataFile이다.
 *
 *
 *  - PROJ-1671 Bitmap-based Tablespace And Segment Space Management
 */
IDE_RC sddDiskMgr::extendDataFileFEBT(
                         idvSQL              * aStatistics,
                         void                * aTrans,
                         scSpaceID             aTableSpaceID,
                         sddDataFileNode     * aFileNode )
{
    UInt                sTransStatus = 0;
    sddTableSpaceNode * sSpaceNode;
    UInt                sState       = 0;
    UInt                sPreparedIO  = 0;
    ULong               sExtendSize;
    smLSN               sLsnNTA;

    IDE_DASSERT( aFileNode != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID ) == ID_TRUE );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode )
                  != IDE_SUCCESS );

    // PRJ-1548 User Memory Tablespace
    // DDL이 모두 TBS Node에 (X) 잠금을 획득하기 때문에, DML과 경합할 수 없다.
    // 예를들어, ADD DATAFILE과 DROP DATAFILE, RESIZE DATAFILE과 같은 연산과
    // 자동확장연산이 동시에 발생할 수 없다.
    // 단, Backup 연산은 DML과 동시에 발생할 수 있다.

    IDU_FIT_POINT( "1.PROJ-1548@sddDiskMgr::extendDataFileFEBT" );

    sctTableSpaceMgr::lockSpaceNode( aStatistics ,
                                     sSpaceNode );
    sState = 1;

    /* BUG-32218
     * wait4Backup() 가 데이터 파일의 백업이 끝날때까지
     * 무한 대기 */
    // fix BUG-11337.
    // 데이타 파일이 백업중이면 완료할때까지 대기 한다.
    while ( SMI_FILE_STATE_IS_BACKUP( aFileNode->mState ) )
    {
        sctTableSpaceMgr::wait4Backup( sSpaceNode );

        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );
    }

    /* 확장시킬 file node는 반드시 autoextend on 모드이어야 한다. */
    IDE_DASSERT(aFileNode->mIsAutoExtend == ID_TRUE);

    if ((aFileNode->mCurrSize + aFileNode->mNextSize)
        <= aFileNode->mMaxSize )
    {
        sExtendSize = aFileNode->mNextSize;
    }
    else
    {
        sExtendSize = aFileNode->mMaxSize - aFileNode->mCurrSize;
    }

    IDE_TEST( prepareIO( aStatistics,
                         aFileNode ) != IDE_SUCCESS );
    sPreparedIO = 1;

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    if (aTrans != NULL)
    {
        // PRJ-1548 User Memory Tablespace
        // 현재 사용중인 DBF Node는 DROPPED 상태일수 없다.
        IDE_ASSERT( SMI_FILE_STATE_IS_NOT_DROPPED( aFileNode->mState ) );

        sLsnNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

        IDE_TEST( smLayerCallback::writeLogExtendDBF( aStatistics,
                                                      aTrans,
                                                      aTableSpaceID,
                                                      aFileNode,
                                                      aFileNode->mCurrSize + sExtendSize,
                                                      NULL )
                  != IDE_SUCCESS );

        sTransStatus = 1;

        // BUG-17465
        // autoextend mode가 바뀌면 로깅을 해야 한다.
        /*
         * BUG-22571 파일의 크기가 커질수없는 상황에서도 v$datafiles의 autoextend mode값이
         *           on인 경우가 있습니다.
         */
        if ( (aFileNode->mCurrSize + sExtendSize + aFileNode->mNextSize) > 
             aFileNode->mMaxSize )
        {
            IDE_TEST( smLayerCallback::writeLogSetAutoExtDBF( aStatistics,
                                                              aTrans,
                                                              aTableSpaceID,
                                                              aFileNode,
                                                              ID_FALSE,
                                                              0,
                                                              0,
                                                              NULL )
                      != IDE_SUCCESS );
        }
        // !!CHECK RECOVERY POINT
        IDU_FIT_POINT( "1.PROJ-1552@sddDiskMgr::extendDataFileFEBT" );
    }

    IDE_TEST( sddDataFile::extend(aStatistics, aFileNode, sExtendSize)
              != IDE_SUCCESS );

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     sSpaceNode );
    sState = 1;

    sddDataFile::setCurrSize(aFileNode, (aFileNode->mCurrSize + sExtendSize));

    if ( aFileNode->mCurrSize == aFileNode->mMaxSize )
    {
        sddDataFile::setAutoExtendProp(aFileNode, ID_FALSE, 0, 0);
    }

    /* 노드 정보 재설정 */
    sSpaceNode->mTotalPageCount += sExtendSize;

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    sPreparedIO = 0;
    IDE_TEST( completeIO( aStatistics,
                          aFileNode,
                          SDD_IO_WRITE ) != IDE_SUCCESS );

    // PRJ-1548 User Memory Tablespace
    // 자동확장 수행하였다면 NTA 로깅과 DBF Node에 잠금을 해제한다.

    IDU_FIT_POINT( "2.PROJ-1552@sddDiskMgr::extendDataFileFEBT" );

    IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( aFileNode )
                == IDE_SUCCESS );

    IDU_FIT_POINT( "3.PROJ-1552@sddDiskMgr::extendDataFileFEBT" );

    if ( aTrans != NULL )
    {
        IDE_TEST( smLayerCallback::writeNTAForExtendDBF( aStatistics,
                                                         aTrans,
                                                         &sLsnNTA )
                  != IDE_SUCCESS );

        // !!CHECK RECOVERY POINT
        IDU_FIT_POINT( "4.PROJ-1552@sddDiskMgr::extendDataFileFEBT" );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sPreparedIO != 0 )
    {
        //  CompleteIO는 Lock을 잡지 않아도 됨
        IDE_ASSERT( completeIO( aStatistics,
                                aFileNode,
                                SDD_IO_WRITE ) == IDE_SUCCESS );
    }

    if ( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    if (sTransStatus != 0)
    {
        IDE_ASSERT( smLayerCallback::undoTrans4LayerCall(
                        aStatistics,
                        aTrans,
                        &sLsnNTA )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : tablespace의 datafile 크기 resize
 *
 * datafile의 resize 변경 연산의 수행에 의해 datafile의 크기를 축소하거나
 * 확장한다.
 *
 * next size는 extent의 크기의 배수가 될 것이다. 이 작업은 write가 끝날때까지 다른
 * 스레드가 접근할수 없도록 mutex를 잡는다. 그렇지 않으면 여러개가 한꺼번에 화일을
 * 확장하려는 시도를 할 수가 있다.
 *
 * + 2nd. code design
 *   - 디스크관리자의 mutex 획득한다.
 *   * while
 *   * 디스크관리자의 HASH에서 tablespace 노드를 찾는다.
 *   * PID를 알면 파일을 얻을수있다.
 *     if( datafile 노드가 이미 원하는 크기 이상이다. )
 *     {
 *         디스크관리자의 mutex 해제한다.
 *         return fail;
 *     }
 *     datafile 노드가 online이면, datafile 노드의 max사이즈에
 *     제한을 받고, offline이면 mMaxDataFileSize에 제한을 받는다.
 *     다음 current size 를 구한다.
 *     확장 size를 구한다.
 *   - write I/O를 준비한다. -> prepareIO
 *   - SD_PAGE_SIZE의 null buffer를 생성한다.
 *   - datafile 크기를 확장크기만큼 루프를 돌면서 SD_PAGE_SIZE
 *     단위로 늘린다. -> write
 *   - datafile 노드를 sync한다.
 *   - datafile 노드의 정보 재설정 -> sddDataFile::setSize
 *   - tablespace 노드의 크기를 재설정
 *   - 로그앵커 FLUSH
 *   - I/O 작업을 완료한다. -> completeIO
 *   - 디스크관리자의 mutex 해제한다.
 **********************************************************************/

IDE_RC sddDiskMgr::alterResizeFEBT(
                   idvSQL          * aStatistics,
                   void             * aTrans,
                   scSpaceID          aTableSpaceID,
                   SChar            * aDataFileName, /*이미 valid함*/
                   scPageID           aHWM,
                   ULong              aSizeWanted,
                   sddDataFileNode  * aFileNode)
{

    UInt               sState = 0;
    ULong              sExtendSize;
    idBool             sExtendFlag;
    UInt               sPreparedIO;
    SLong              sResizePageSize;
    sctPendingOp      *sPendingOp;
    sddTableSpaceNode* sSpaceNode;

    //현재 파일의 크기는 aFileNode->mCurrSize 로 알수있다.

    IDE_DASSERT( aTrans        != NULL );
    IDE_DASSERT( aDataFileName != NULL );
    IDE_DASSERT( aSizeWanted > 0 );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID ) == ID_TRUE );
    IDE_DASSERT( aFileNode != NULL);

    sPreparedIO     = 0;
    sResizePageSize = 0;

    IDU_FIT_POINT( "1.BUG-32218@sddDiskMgr::alterResizeFEBT" );

    
    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aTableSpaceID,
                                                               (void**)&sSpaceNode )
              != IDE_SUCCESS );
    sState = 1;

    /* resize는 현재 화일의 크기와 요구하는 크기가 다른 경우에만 수행된다.*/
    if( aFileNode->mCurrSize != aSizeWanted )
    {
        /* BUG-32218
         * wait4Backup() 가 데이터 파일의 백업이 끝날때까지
         * 무한 대기 */
        // fix BUG-11337.
        // 데이타 파일이 백업중이면 완료할때까지 대기 한다.
        while ( SMI_FILE_STATE_IS_BACKUP( aFileNode->mState ) )
        {
            sctTableSpaceMgr::wait4Backup( sSpaceNode );

            IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );
        }

        // PRJ-1548 User Memory Tablespace
        // DBF Node에 대해 Resize하려면 TBS에 (X) 잠금이 획득되어 있다.
        // 데이타파일 노드에 대한 DROP 연산이 TBS Node에 (X) ->TBS META PAGE에 (S)
        // 잠금을 획득하기 때문에 RESIZE 연산과 동시에 수행될 수 없다.
        // 같은 조건을 체크해보아야 한다.
        // DBF Node가 DROPPED 상태라면 Exception 처리한다.
        IDE_TEST_RAISE( SMI_FILE_STATE_IS_DROPPED( aFileNode->mState ),
                        error_not_found_datafile_node );

        /*
         * 만약 HWM보다 더 작은 크기로의 축소를 요청한다면 에러처리해야한다.
         * 이 함수의 앞부분에서 먼저 처리를 하기 때문에 여기서는 assert한다.
         */
        IDE_ASSERT( aSizeWanted > SD_MAKE_FPID( aHWM ) );

        /* ==================================================
         * 화일 확장의 경우 에러 체크
         * 화일 확장으로 인해 화일의 최대 크기에 도달하는 경우
         * auto extend mode를 off로 설정함
         * =================================================== */

        // BUG-17415 autoextend on일 경우에만 maxsize를 체크한다.
        if (aFileNode->mIsAutoExtend == ID_TRUE)
        {
            IDE_ASSERT( aFileNode->mMaxSize <= mMaxDataFilePageCount );

            // BUG-29566 데이터 파일의 크기를 32G 를 초과하여 지정해도
            //           에러를 출력하지 않습니다.
            // Max Size가 이미 검증되어 있는 값이므로 Autoextend on에서는
            // Max Size보다 작은지만 검사하면 된다.
            IDE_TEST_RAISE( aSizeWanted > aFileNode->mMaxSize,
                            error_invalid_extend_filesize_maxsize );
        }

        // 요청한 File Size가 Maximum File Size를 넘는지 비교한다..
        // Autoextend off 시에는 비교할 Max Size가 없으므로 직접비교해야한다.
        if( mMaxDataFilePageCount < SD_MAX_FPID_COUNT )
        {
            // Os Limit 이 32G이하인 경우
            IDE_TEST_RAISE( aSizeWanted > mMaxDataFilePageCount,
                            error_invalid_extend_filesize_oslimit );
        }
        else
        {
            IDE_TEST_RAISE( aSizeWanted > SD_MAX_FPID_COUNT,
                            error_invalid_extend_filesize_limit );
        }

        /* ------------------------------------------------
         * 원하는 page 개수가 datafile 노드의 currsize보다
         * 큰 경우는 확장을 하고, 그렇지 않은 경우는 축소를
         * 한다. 축소할 경우 init size도 함께 변한다.
         * ----------------------------------------------*/
        /* t: 확장, f: 축소 */

        if (aSizeWanted > aFileNode->mCurrSize) //확장
        {
           sExtendSize = aSizeWanted - aFileNode->mCurrSize;
           sExtendFlag = ID_TRUE;
        }
        else                                    //축소
        {
           sExtendSize = aFileNode->mCurrSize - aSizeWanted;
           sExtendFlag = ID_FALSE;
        }

        IDE_ASSERT( sExtendSize != 0 );

        /* ------------------------------------------------
         * 위에서 계산된 datafile 크기를 참고하여 축소 또는
         * 확장 처리한다.
         * ----------------------------------------------*/
        IDE_TEST( prepareIO( aStatistics,
                             aFileNode ) != IDE_SUCCESS );
        sPreparedIO = 1;
        
        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

        if (sExtendFlag == ID_TRUE) // 확장
        {
            if ( aTrans != NULL )
            {
                IDE_TEST( smLayerCallback::writeLogExtendDBF(
                              aStatistics,
                              aTrans,
                              aTableSpaceID,
                              aFileNode,
                              aFileNode->mCurrSize + sExtendSize,
                              NULL ) != IDE_SUCCESS );

            }

            sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                             sSpaceNode );
            sState = 1;
            // 현재 Resize 진행하는 파일에 대해서 CREATING 상태를
            // Oring 해주고 Pending으로 처리한다.
            aFileNode->mState |= SMI_FILE_RESIZING;

            sState = 0;
            sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

            sResizePageSize = sExtendSize;

            IDE_TEST( sddDataFile::addPendingOperation(
                          aTrans,
                          aFileNode,
                          ID_TRUE, /* commit시에 동작 */
                          SCT_POP_ALTER_DBF_RESIZE,
                          &sPendingOp ) != IDE_SUCCESS );

            sPendingOp->mResizePageSize = sResizePageSize;

            IDE_TEST( sddDataFile::extend(aStatistics, aFileNode, sExtendSize)
                      != IDE_SUCCESS );

            // BUG-17465
            // maxsize에 도달하면 autoextend mode가 on에서 off로 바뀌는데
            // 이에 대해 로깅해야 한다.
            if ( (aFileNode->mIsAutoExtend == ID_TRUE) &&
                 ((aFileNode->mCurrSize + sExtendSize + aFileNode->mNextSize) > 
                     aFileNode->mMaxSize) )
            {
                IDE_TEST( smLayerCallback::writeLogSetAutoExtDBF(
                              aStatistics,
                              aTrans,
                              aTableSpaceID,
                              aFileNode,
                              ID_FALSE,
                              0,
                              0,
                              NULL ) != IDE_SUCCESS );

            }
        }
        else // 축소
        {
            if ( aTrans != NULL )
            {
                IDE_TEST( smLayerCallback::writeLogShrinkDBF(
                                             aStatistics,
                                             aTrans,
                                             aTableSpaceID,
                                             aFileNode,
                                             aSizeWanted,
                                             aFileNode->mCurrSize - sExtendSize,
                                             NULL )
                          != IDE_SUCCESS );

            }

            sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                             sSpaceNode );
            sState = 1;
            // 현재 Resize 진행하는 파일에 대해서 CREATING 상태를
            // Oring 해주고 Pending으로 처리한다.
            aFileNode->mState |= SMI_FILE_RESIZING;

            sState = 0;
            sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

            // 실제 Resizing 페이지 개수를 저장한다. (Runtime 정보)
            sResizePageSize = -sExtendSize;

            IDE_TEST( sddDataFile::addPendingOperation(
                          aTrans,
                          aFileNode,
                          ID_TRUE, /* commit시에 동작 */
                          SCT_POP_ALTER_DBF_RESIZE,
                          &sPendingOp ) != IDE_SUCCESS );

            sPendingOp->mPendingOpFunc    = sddDiskMgr::shrinkFilePending;
            sPendingOp->mPendingOpParam   = (void*)aFileNode;
            sPendingOp->mResizePageSize   = sResizePageSize;
            sPendingOp->mResizeSizeWanted = aSizeWanted;
        }

        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        sState = 1;

        /* 노드 정보 재설정 */
        if (sExtendFlag == ID_TRUE) //확장
        {
            sddDataFile::setCurrSize(
                         aFileNode,
                         (aFileNode->mCurrSize + sExtendSize));

            if ( aFileNode->mIsAutoExtend == ID_TRUE )
            {
                if ( aFileNode->mCurrSize == aFileNode->mMaxSize )
                {
                    sddDataFile::setAutoExtendProp(aFileNode,
                                                   ID_FALSE,
                                                   0,
                                                   0);
                }
            }
        }
        else
        {
            // Shrink 연산은 Pending으로 처리된다.
        }

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

        sPreparedIO = 0;
        IDE_TEST( completeIO( aStatistics,
                              aFileNode,
                              SDD_IO_WRITE ) != IDE_SUCCESS );

        IDU_FIT_POINT( "3.PROJ-1552@sddDiskMgr::alterResizeFEBT" );

        /* loganchor flush */
        IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( aFileNode )
                    == IDE_SUCCESS );

        IDU_FIT_POINT( "4.PROJ-1552@sddDiskMgr::alterResizeFEBT" );
    }
    else
    {
        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_datafile_node );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNode,
                                aDataFileName));
    }
    IDE_EXCEPTION( error_invalid_extend_filesize_limit );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_InvalidExtendFileSize,
                                 aSizeWanted,
                                 (ULong)SD_MAX_FPID_COUNT) );
    }
    IDE_EXCEPTION( error_invalid_extend_filesize_oslimit );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidExtendFileSizeOSLimit,
                                aSizeWanted,
                                (ULong)mMaxDataFilePageCount));
    }
    IDE_EXCEPTION( error_invalid_extend_filesize_maxsize );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidExtendFileSizeMaxSize,
                                aSizeWanted,
                                aFileNode->mMaxSize));
    }

    IDE_EXCEPTION_END;

    if ( sPreparedIO != 0 )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aFileNode,
                                SDD_IO_WRITE ) == IDE_SUCCESS );
    }

    if (sState != 0)
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 오픈된 datafile 중에서 사용하지 않는 datafile을 close한다.
 *               사용되지 않는것으로 추정되는 FileNode가 VictimFileList에 모여 있고,
 *               이를 탐색하여 정말로 사용하지 않는 Node를 close한다.
 *
 * 누군가 내가 사용 하여야 할 FileNode를 대신 Open하였거나,
 * OpenFileCount가 Max Count 이하로 내려가면 빠져나온다.
 *
 **********************************************************************/
IDE_RC sddDiskMgr::closeDataFileInList( idvSQL          * aStatistics,
                                        sddDataFileNode * aDataFileNode )
{
    UInt              sState = 1;
    sddDataFileNode*  sFileNode;
    smuList*          sNode;
    smuList*          sBaseNode;

    while( ( aDataFileNode->mIsOpened == ID_FALSE ) &&
           ( mOpenFileCount >= smuProperty::getMaxOpenDataFileCount() ))
    {
        if ( mVictimListNodeCount > 0 )
        {
            lockCloseVictimList( aStatistics );
            sState = 1;

            sBaseNode = &mVictimFileList;

            for ( sNode =  SMU_LIST_GET_LAST( sBaseNode );
                  sNode != sBaseNode;
                  sNode =  SMU_LIST_GET_PREV( sNode ) )
            {
                sFileNode = (sddDataFileNode*)sNode->mData;

                /* found */
                // IO Count가 0인 녀석들의 list이지만,
                // 순간적으로 0이 아닐수도 있다.
                if ( sFileNode->mIOCount == 0 )
                {
                    // List에서 연결을 해제한다.
                    // 이후 closeDataFile에서 성공 할 수도 있고 실패 할 수도 있다.
                    // 성공했다면, close된것
                    // 실패 하였다면,
                    // 1. 다른이가 close한것이다.
                    // 2. 누가 다시 재활용 하였다. IOCount >= 1
                    // 어느쪽이던 List에는 다시 연결 할 필요가 없다.
                    // 미리 연결을 끊지 않으면, 같은 파일을 서로 Close하려고 시도하게 된다.
                    SMU_LIST_DELETE( &sFileNode->mNode4LRUList );
                    sFileNode->mNode4LRUList.mNext = NULL;
                    sFileNode->mNode4LRUList.mPrev = NULL;

                    break;
                }
            }

            sState = 0;
            unlockCloseVictimList();

            if ( sNode != sBaseNode )
            {
                IDE_TEST( closeDataFile( aStatistics,
                                         sFileNode ) != IDE_SUCCESS );
                continue;
            }
        }

        idCore::acpAtomicInc32( &mWaitThr4Open );

        idlOS::sleep(0);

        idCore::acpAtomicDec32( &mWaitThr4Open );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        unlockCloseVictimList();
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : datafile 노드의 autoextend 속성 변경
 *
 * fix BUG-15387
 * 데이타파일 사용상태가 NotUsed/Used/InUsed가 있을 수 있는데
 * Used 상태만 아닌 데이타파일에 대해서는 각각 autoextend 모드를
 * On/Off 할 수 있다.
 *
 * [ 인자 ]
 *
 * [IN]  aTrans          - 트랜잭션 객체
 * [IN]  aTableSpaceID   - 해당 Tablespace ID
 * [IN]  aDataFileName   - 데이타파일 경로
 * [IN]  aAutoExtendMode - 변경하고자하는 Autoextend 모드 ( On/Off )
 * [IN]  aHWM            - high water mark
 * [IN]  aNextSize       - Autoextend On 시 Next Size  ( page size )
 * [IN]  aMaxSize        - Autoextend On 시 Max Size   ( page size )
 * [OUT] aValidDataFileName - 데이타파일 절대경로

 **********************************************************************/
IDE_RC sddDiskMgr::alterAutoExtendFEBT(
                        idvSQL            * aStatistics,
                        void              * aTrans,
                        sddTableSpaceNode * aSpaceNode,
                        SChar             * aDataFileName,
                        sddDataFileNode   * aFileNode,
                        idBool              aAutoExtendMode,
                        ULong               aNextSize,
                        ULong               aMaxSize)
{
    UInt sState = 0;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE );
    IDE_DASSERT( aDataFileName != NULL );
    IDE_DASSERT( aFileNode != NULL );

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );
    sState = 1;

    /*
     * 속성 변경이 가능한 경우인지를 검사한다
     *
     *     1. autoextend on으로 변경할 경우 maxsize는 currsize보다
     *        커야 한다.
     *     2. autoextend on으로 변경할 경우 maxsize는 OS file limit
     *        보다 작아야 한다.
     */

    /* Check maxsize validation */
    IDE_TEST_RAISE( (aAutoExtendMode == ID_TRUE) &&
                    (aFileNode->mCurrSize >= aMaxSize),
                    error_invalid_autoextend_filesize );

    IDE_TEST_RAISE( aMaxSize > getMaxDataFileSize(),
                    error_max_exceed_maxfilesize );

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );

    /* ------------------------------------------------
     * 1. 로깅
     * ----------------------------------------------*/
    // To Fix BUG-13924

    // autoextend off일 경우 nextsize, maxsize는 0이 된다.
    if (aAutoExtendMode == ID_FALSE)
    {
        aNextSize = 0;
        aMaxSize = 0;
    }

    if ( aTrans != NULL )
    {
        // PROJ-1548
        // 데이타파일 노드에 대한 DROP 연산이 TBS Node에 (X) 잠금을 획득하고 있다.
        // 그렇기때문에 AUTOEXTEND MODE 연산이 동시에 수행될 수 없다.
        // 다음과 같은 조건을 체크해보아야 한다.
        // DBF Node가 DROPPED 상태라면 Exception 처리한다.
        IDE_TEST_RAISE( SMI_FILE_STATE_IS_DROPPED( aFileNode->mState ),
                        error_not_found_datafile_node );

        IDE_TEST( smLayerCallback::writeLogSetAutoExtDBF( aStatistics,
                                                          aTrans,
                                                          aSpaceNode->mHeader.mID,
                                                          aFileNode,
                                                          aAutoExtendMode,
                                                          aNextSize,
                                                          aMaxSize,
                                                          NULL )
                  != IDE_SUCCESS );

    }

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );
    sState = 1;

    /* -------------------------------------------------------
     * 2. 검색된 datafile 노드의 autoextend 모드를 변경한다.
     * ----------------------------------------------------- */
    sddDataFile::setAutoExtendProp(aFileNode,
                                   aAutoExtendMode,
                                   aNextSize,
                                   aMaxSize);

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );

    /* -------------------------------------------------------
     * 3. 로그 앵커를 플러쉬한다.
     * ----------------------------------------------------- */
    IDU_FIT_POINT( "2.PROJ-1552@sddDiskMgr::alterAutoExtendFEBT" );

    IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( aFileNode )
                == IDE_SUCCESS );

    IDU_FIT_POINT( "3.PROJ-1552@sddDiskMgr::alterAutoExtendFEBT" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_datafile_node );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNode,
                                aDataFileName));
    }
    IDE_EXCEPTION(error_invalid_autoextend_filesize);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidAutoExtFileSize,
                                aMaxSize,
                                aFileNode->mCurrSize));
    }
    IDE_EXCEPTION(error_max_exceed_maxfilesize);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_MaxExceedMaxFileSize,
                                aMaxSize,
                                (ULong)getMaxDataFileSize()));
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    }

    return IDE_FAILURE;

}
/***********************************************************************
 * Description :  datafile 노드의 datafile 명 속성 변경
 * BUG-10474에 의해서 datafile rename은 startup control 단계에서
 * tablespace가 offlin인경우에만 가능하다.
 **********************************************************************/
IDE_RC sddDiskMgr::alterDataFileName( idvSQL*     aStatistics,
                                      scSpaceID   aTableSpaceID,
                                      SChar*      aOldFileName,
                                      SChar*      aNewFileName )
{

    UInt               sState = 0;
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;
    SChar              sValidNewDataFileName[SM_MAX_FILE_NAME + 1];
    SChar              sValidOldDataFileName[SM_MAX_FILE_NAME + 1];
    UInt               sNameLen;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID ) == ID_TRUE );
    IDE_DASSERT( aNewFileName != NULL );
    IDE_DASSERT( aOldFileName != NULL );

    sNameLen = 0;

    idlOS::memset(sValidNewDataFileName, 0x00, SM_MAX_FILE_NAME + 1);
    idlOS::strncpy(sValidNewDataFileName, aNewFileName, SM_MAX_FILE_NAME);
    sNameLen = idlOS::strlen(sValidNewDataFileName);

    IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                ID_TRUE,
                                sValidNewDataFileName,
                                &sNameLen,
                                SMI_TBS_DISK)
              != IDE_SUCCESS );

    idlOS::memset(sValidOldDataFileName, 0x00, SM_MAX_FILE_NAME + 1);
    idlOS::strncpy(sValidOldDataFileName, aOldFileName, SM_MAX_FILE_NAME);
    sNameLen = idlOS::strlen(sValidOldDataFileName);

    IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                ID_FALSE,
                                sValidOldDataFileName,
                                &sNameLen,
                                SMI_TBS_DISK)
              != IDE_SUCCESS );

    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aTableSpaceID,
                                                               (void**)&sSpaceNode )
                  != IDE_SUCCESS );
    sState = 1;


    IDE_TEST( sddTableSpace::getDataFileNodeByName(sSpaceNode,
                                                   sValidOldDataFileName,
                                                   &sFileNode)
            != IDE_SUCCESS );

    IDE_TEST( sddDataFile::setDataFileName(sFileNode, sValidNewDataFileName)
            != IDE_SUCCESS );

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    /* ------------------------------------------------
     * disable recovery check point
     * "alterDataFileName#before update loganchor"
     * startup control 과정에서만 수행되는 command이기
     * 때문에 recovery 대상이 아닙니다. loganchor에 반영이
     * 되고 crash가 되었다면 반영이 된것입니다.
     * ----------------------------------------------*/

    /* loganchor flush */
    IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;

}

/**********************************************************************
 * Description : 해당 tablespace 노드의 total page 개수를 반환
 **********************************************************************/
IDE_RC sddDiskMgr::getTotalPageCountOfTBS( idvSQL  *  aStatistics,
                                           scSpaceID  aTableSpaceID,
                                           ULong*     aTotalPageCount )
{

    UInt               sState = 0;
    sddTableSpaceNode* sSpaceNode;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID ) == ID_TRUE );
    IDE_DASSERT( aTotalPageCount != NULL );

    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aTableSpaceID,
                                                               (void**)&sSpaceNode )
                  != IDE_SUCCESS );
    sState = 1;

    *aTotalPageCount = sddTableSpace::getTotalPageCount( sSpaceNode );

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;

}

/**********************************************************************
 *  fix BUG-13646 tablespace의 page count와 extent크기를 얻는다.
 **********************************************************************/
IDE_RC sddDiskMgr::getExtentAnTotalPageCnt( idvSQL  *  aStatistics,
                                            scSpaceID  aTableSpaceID,
                                            UInt*      aExtentPageCount,
                                            ULong*     aTotalPageCount )
{

    UInt               sState = 0;
    sddTableSpaceNode* sSpaceNode;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID ) == ID_TRUE );
    IDE_DASSERT( aTotalPageCount != NULL );

    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aTableSpaceID,
                                                               (void**)&sSpaceNode )
                  != IDE_SUCCESS );
    sState = 1;

    *aTotalPageCount  = sddTableSpace::getTotalPageCount(sSpaceNode);
    *aExtentPageCount = sSpaceNode->mExtPageCount;

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * Description :
 **********************************************************************/
IDE_RC sddDiskMgr::existDataFile( idvSQL*   aStatistics,
                                  scSpaceID aID,
                                  SChar*    aName,
                                  idBool*   aExist)
{

    UInt               sState = 0;
    UInt               sNameLength;
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;
    SChar              sValidName[SM_MAX_FILE_NAME];

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aID ) == ID_TRUE );
    IDE_DASSERT( aName != NULL );

    sNameLength = 0;

    idlOS::strcpy((SChar*)sValidName, aName);
    sNameLength = idlOS::strlen(sValidName);

    IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                ID_TRUE,
                                (SChar*)sValidName,
                                &sNameLength,
                                SMI_TBS_DISK)
              != IDE_SUCCESS );

    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aID,
                                                               (void**)&sSpaceNode )
                  != IDE_SUCCESS );
    sState = 1;

    /* 상위모듈에서 aExist를 보고 직접 에러를 설정할수 있도록 해야한다. */
    (void)sddTableSpace::getDataFileNodeByName( sSpaceNode,
                                                sValidName,
                                                &sFileNode );

    *aExist = ((sFileNode != NULL) ? ID_TRUE : ID_FALSE );

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * Description :
 **********************************************************************/
IDE_RC sddDiskMgr::existDataFile( SChar   * aName,
                                  idBool  * aExist)
{
    UInt                sNameLength;
    sddDataFileNode   * sFileNode   = NULL;
    scSpaceID           sSpaceID;
    scPageID            sFirstPID;
    scPageID            sLastPID;
    SChar               sValidName[ SM_MAX_FILE_NAME ];

    IDE_DASSERT( aName != NULL );

    sNameLength = 0;

    idlOS::strcpy((SChar *)sValidName, aName);
    sNameLength = idlOS::strlen(sValidName);

    IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                ID_TRUE,
                                (SChar*)sValidName,
                                &sNameLength,
                                SMI_TBS_DISK)
              != IDE_SUCCESS );

    sctTableSpaceMgr::getDataFileNodeByName( sValidName,
                                             &sFileNode,
                                             &sSpaceID,
                                             &sFirstPID,
                                             &sLastPID );

    *aExist = ((sFileNode != NULL) ? ID_TRUE : ID_FALSE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  테이블스페이스와 데이타파일의
  삭제로 인한 페이지의 유효성 여부를 반환한다.

   drop tablespace의 연산처리가 데이타파일을 먼저 DROPPED 하고
   테이블스페이스가 나중에 DROPPED가 되기 때문에 (복구알고리즘)
   커밋 PENDING 연산 순서에 따라 트랜잭션이 커밋되더라도
   하위부터 체크해야하는 경우가 존재한다.

   DELETE 쓰레드가 Index/Table 세그먼트 PENDING 연산에서 데이타파일도
   체크하도록 하여 DROPPED이 된경우 무시하도록 해야한다.

   [IN] aTableSpaceID  -  검사할 테이블스페이스 ID
   [IN] aPageID        -  검사할 페이지 ID
   [IN] aIsValid        - 유효한 페이진인지 여부

   [ 참고 ]
   Disk Aging 시에 Table/Index에 대한 Free Segment 를 해야할 지 여부를
   판단할 때 호출되는데, Latch를 잡고 판단하기 때문에
   즉, DDL에 대한 Aging 처리이기 때문에 빈번하게 호출되지는 않는다.

   [ Call by ]
   smcTable::rebuildDRDBIndexHeader
   sdpPageList::freeIndexSegForEntry
   sdpPageList::freeTableSegForEntry
   sdpPageList::freeLobSeg
   sdpPageList::initEntryAtRuntime
   sdbDW::initEntryAtRuntime

 */
idBool sddDiskMgr::isValidPageID( idvSQL*    aStatistics,
                                  scSpaceID  aTableSpaceID,
                                  scPageID   aPageID )
{
    sddTableSpaceNode* sSpaceNode = NULL;
    sddDataFileNode*   sFileNode;
    idBool             sIsValid = ID_FALSE;

    // drop 된 TBS는 NULL롤 반환된다.
    sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aTableSpaceID );

    if( sSpaceNode != NULL )
    {
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        // fix BUG-17159
        // dropped/discarded tbs에 대해서 invalid하다라고 판단한다.
        if ( sctTableSpaceMgr::hasState(
                               &sSpaceNode->mHeader,
                               SCT_SS_INVALID_DISK_TBS) == ID_FALSE )
        {
            sddTableSpace::getDataFileNodeByPageIDWithoutException(
                                                sSpaceNode,
                                                aPageID,
                                                &sFileNode );

            if( sFileNode != NULL )
            {
                sIsValid = ID_TRUE;
            }
        }
        else
        {
            // Dropped, Discard 된 TBS는 Valid 하지 않다.
        }
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return sIsValid;
}


//***********************************************************************
// For Unit TestCase

/***********************************************************************
 * Description : 해당 datafile 노드를 검색하여 datafile을 오픈한다.
 ***********************************************************************/
IDE_RC sddDiskMgr::openDataFile( idvSQL    * aStatistics,
                                 scSpaceID   aTableSpaceID,
                                 scPageID    aPageID )
{

    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode )
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::getDataFileNodeByPageID( sSpaceNode,
                                                      aPageID,
                                                      &sFileNode,
                                                      ID_TRUE ) /* aFatal */
              != IDE_SUCCESS )

    IDE_TEST( prepareIO( aStatistics,
                         sFileNode ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 해당 datafile 노드를 검색하여 datafile을 close한다.
 ***********************************************************************/
IDE_RC sddDiskMgr::closeDataFile( scSpaceID aTableSpaceID,
                                  scPageID  aPageID )
{
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode)
                  != IDE_SUCCESS );

    IDE_ASSERT( sSpaceNode->mHeader.mID == aTableSpaceID );

    IDE_TEST( sddTableSpace::getDataFileNodeByPageID( sSpaceNode,
                                                      aPageID,
                                                      &sFileNode,
                                                      ID_TRUE ) /* aFatal */
              != IDE_SUCCESS );

    IDE_TEST( closeDataFile( NULL, sFileNode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

#if 0 // not used
/***********************************************************************
 * Description : completeIO를 하여,max open대기상태에서 victim이 되도록 한다.
 * // Unit Test 용
 ***********************************************************************/
IDE_RC sddDiskMgr::completeIO( idvSQL*   aStatistics,
                               scSpaceID aTableSpaceID,
                               scPageID  aPageID )
{
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode)
                  != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::getDataFileNodeByPageID( sSpaceNode,
                                                      aPageID,
                                                      &sFileNode,
                                                      ID_TRUE ) /* aFatal */
              != IDE_SUCCESS );

    IDE_TEST( completeIO( aStatistics,
                          sFileNode,
                          SDD_IO_READ ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

/*
  PRJ-1149,  주어진 데이타 파일의 헤더를 읽는다.

  [IN]  aFileNode       - 데이타파일 노드
  [OUT] aDBFileHdr      - 데이타파일로부터 판독된 파일메타헤더 정보를 저장할 버퍼
  [OUT] aIsMediaFailure - 메타정보를 확인한 후 복구필요여부 반환
*/
IDE_RC sddDiskMgr::checkValidationDBFHdr(
                       idvSQL           * aStatistics,
                        sddDataFileNode  * aFileNode,
                        sddDataFileHdr   * aDBFileHdr,
                        idBool           * aIsMediaFailure )
{
    IDE_DASSERT( aFileNode       != NULL );
    IDE_DASSERT( aDBFileHdr      != NULL );
    IDE_DASSERT( aIsMediaFailure != NULL );

    IDE_TEST( readDBFHdr( aStatistics,
                          aFileNode,
                          aDBFileHdr )
              != IDE_SUCCESS );

    // Loganchor와 파일로부터 판독된 파일메타헤더정보를 비교하여
    // 미디어복구의 필요여부를 반환한다.
    IDE_TEST(sddDataFile::checkValidationDBFHdr( aFileNode,
                                                 aDBFileHdr,
                                                 aIsMediaFailure )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : PROJ-1867  주어진 DB File의 DBFileHdr를 읽는다.
 *
 * [IN]  aFileNode       - 데이타파일 노드
 * [OUT] aDBFileHdr      - 데이타파일로부터 판독된 파일메타헤더 정보를 저장할 버퍼
 * ********************************************************************/
IDE_RC sddDiskMgr::readDBFHdr( idvSQL           * aStatistics,
                               sddDataFileNode  * aFileNode,
                               sddDataFileHdr   * aDBFileHdr )
{
    idBool sPreparedIO = ID_FALSE;

    IDE_DASSERT( aFileNode       != NULL );
    IDE_DASSERT( aDBFileHdr      != NULL );

    IDE_TEST( prepareIO( aStatistics,
                         aFileNode ) != IDE_SUCCESS );
    sPreparedIO = ID_TRUE;

    /* File에 대한 IO는 항상 DirectIO를 고려해서 Buffer, Size, Offset을
     * DirectIO Page 크기로 Align시킨다. */
    IDE_TEST( aFileNode->mFile.read( aStatistics,
                                     SM_DBFILE_METAHDR_PAGE_OFFSET,
                                     aFileNode->mAlignedPageBuff,
                                     SD_PAGE_SIZE,
                                     NULL/*RealReadSize*/ )
              != IDE_SUCCESS );

    idlOS::memcpy( aDBFileHdr,
                   aFileNode->mAlignedPageBuff,
                   ID_SIZEOF(sddDataFileHdr) );

    sPreparedIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aFileNode,
                          SDD_IO_READ ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Read Database File Header Failure\n"
                 "              Tablespace ID = %"ID_UINT32_FMT"\n"
                 "              File ID       = %"ID_UINT32_FMT"\n"
                 "              IO Count      = %"ID_UINT32_FMT"\n"
                 "              File Opened   = %s",
                 aFileNode->mSpaceID,
                 aFileNode->mID,
                 aFileNode->mIOCount,
                 (aFileNode->mIsOpened ? "Open" : "Close") );

    if( sPreparedIO == ID_TRUE )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aFileNode,
                                SDD_IO_READ ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 데이타 파일 해더 갱신.
 * PRJ-1149,  주어진 데이타 파일 노드의 헤더를 이용하여
 * 데이타 파일 갱신을 한다.
 **********************************************************************/
IDE_RC sddDiskMgr::writeDBFHdr( idvSQL         * aStatistics,
                                sddDataFileNode* aDataFileNode )
{
    idBool sPreparedIO = ID_FALSE;

    IDE_DASSERT( aDataFileNode != NULL);
    IDE_DASSERT( sddDataFile::checkValuesOfDBFHdr(
                 &(aDataFileNode->mDBFileHdr))
                 == IDE_SUCCESS );

    // BUG-17158
    // offline TBS라도 DBF Hdr는 Checkpoint시 갱신할 수 있게
    // 하기 위해 설정한다.
    mEnableWriteToOfflineDBF = ID_TRUE;

    IDE_TEST( prepareIO( aStatistics,
                         aDataFileNode ) != IDE_SUCCESS );
    sPreparedIO = ID_TRUE;

    IDU_FIT_POINT( "1.TASK-1842@sddDiskMgr::writeDBFHdr" );

    IDE_TEST( sddDataFile::writeDBFileHdr(
                  aStatistics,
                  aDataFileNode,
                  NULL )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "2.TASK-1842@sddDiskMgr::writeDBFHdr" );

    IDE_TEST(sddDataFile::sync( aDataFileNode,
                                smLayerCallback::setEmergency ) != IDE_SUCCESS);

    sPreparedIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aDataFileNode,
                          SDD_IO_READ ) != IDE_SUCCESS );

    mEnableWriteToOfflineDBF = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Write Database File Header Failure\n"
                 "               Tablespace ID = %"ID_UINT32_FMT"\n"
                 "               File ID       = %"ID_UINT32_FMT"\n"
                 "               IO Count      = %"ID_UINT32_FMT"\n"
                 "               File Opened   = %s",
                 aDataFileNode->mSpaceID,
                 aDataFileNode->mID,
                 aDataFileNode->mIOCount,
                 (aDataFileNode->mIsOpened ? "Open" : "Close") );

    if( sPreparedIO == ID_TRUE )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aDataFileNode,
                                SDD_IO_READ )
                    == IDE_SUCCESS );
    }

    mEnableWriteToOfflineDBF = ID_FALSE;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 데이타 파일 복사
 **********************************************************************/
IDE_RC sddDiskMgr::copyDataFile(idvSQL            * aStatistics,
                                sddTableSpaceNode * aSpaceNode,
                                sddDataFileNode   * aDataFileNode,
                                SChar             * aBackupFilePath )
{
    idBool  sLocked = ID_FALSE;
    idBool  sPrepareIO = ID_FALSE;
    smLSN   sMustRedoToLSN;
    sddDataFileHdr   sDBFileHdr;

    IDE_DASSERT( aDataFileNode   != NULL );
    IDE_DASSERT( aBackupFilePath != NULL );

    /* BUG-41031
     * 파일의 상태가 SMI_FILE_CREATING, SMI_FILE_RESIZING, SMI_FILE_DROPPING일 경우 백업 수행 시에 대기를 하게 된다.
     * 그러나 파일 확장과 backup간에 동시성 제어가 lock이 아닌 상태값으로만 파악하기 때문에
     * 백업이 진행되는 중에 다시 상태 값이 SMI_FILE_CREATING, SMI_FILE_RESIZING, SMI_FILE_DROPPING 상태로 변경될 수 있다.
     * 그러므로 만약 상태 값이 다시 바뀐 상태에서 데이터 파일을 복사할 경우 trace log를 찍도록 한다. */
    if ( SMI_FILE_STATE_IS_DROPPED ( aDataFileNode->mState ) ||
         SMI_FILE_STATE_IS_CREATING( aDataFileNode->mState ) ||
         SMI_FILE_STATE_IS_RESIZING( aDataFileNode->mState ) ||
         SMI_FILE_STATE_IS_DROPPING( aDataFileNode->mState ) )
    {
        ideLog::log( IDE_SM_0,
                     "The data file has an invalid state.\n"
                     "               Tablespace ID = %"ID_UINT32_FMT"\n"
                     "               File ID       = %"ID_UINT32_FMT"\n"
                     "               State         = %08"ID_XINT32_FMT"\n"
                     "               IO Count      = %"ID_UINT32_FMT"\n"
                     "               File Opened   = %s\n",
                     "               File Modified = %s\n",
                     aDataFileNode->mSpaceID,
                     aDataFileNode->mID,
                     aDataFileNode->mState,
                     aDataFileNode->mIOCount,
                     ( aDataFileNode->mIsOpened ? "Open" : "Close" ),
                     ( aDataFileNode->mIsModified ? "Modified" : "Unmodified" ) );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( prepareIO( aStatistics,
                         aDataFileNode ) != IDE_SUCCESS );
    sPrepareIO = ID_TRUE;

    IDE_TEST(aDataFileNode->mFile.copy( aStatistics,
                                        aBackupFilePath )
             != IDE_SUCCESS);

    // Complete 자체는 lock이 필요하지 않지만
    // Complete가 풀려서 File Close 등 다른 작업이 진행되기 전에
    // 먼저 lock을 잡는다.
    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );

    sLocked    = ID_TRUE;
    sPrepareIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aDataFileNode,
                          SDD_IO_READ ) != IDE_SUCCESS );

    // backup한 DBFile의 DataFileHeader의 MustRedoToLSN을 갱신한다.

    smLayerCallback::getLstLSN( &sMustRedoToLSN );

    sDBFileHdr = aDataFileNode->mDBFileHdr;

    sddDataFile::setDBFHdr( &sDBFileHdr,
                            NULL,   // DiskRedoLSN
                            NULL,   // DiskCreateLSN
                            &sMustRedoToLSN,
                            NULL ); //DataFileDescSlotID

    IDE_TEST( sddDataFile::writeDBFileHdrByPath( aBackupFilePath,
                                                 &sDBFileHdr )
              != IDE_SUCCESS );

    sLocked = ID_FALSE;
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sPrepareIO == ID_TRUE  )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aDataFileNode,
                                SDD_IO_READ ) == IDE_SUCCESS );
    }

    if ( sLocked == ID_TRUE )
    {
        sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    }

    return IDE_FAILURE;

}
/***********************************************************************
 * Description : 데이타 파일 incremental backup
 * PROJ-2133
 **********************************************************************/
IDE_RC sddDiskMgr::incrementalBackup(idvSQL                 * aStatistics,
                                     sddTableSpaceNode      * aSpaceNode,
                                     smriCTDataFileDescSlot * aDataFileDescSlot,
                                     sddDataFileNode        * aDataFileNode,
                                     smriBISlot             * aBackupInfo )
{
    idBool              sLocked = ID_FALSE;
    idBool              sPrepareIO = ID_FALSE;
    smLSN               sMustRedoToLSN;
    sddDataFileHdr      sDBFileHdr;
    iduFile           * sSrcFile;
    iduFile             sDestFile;
    ULong               sBackupFileSize = 0;
    ULong               sBackupSize;
    ULong               sIBChunkSizeInByte;
    ULong               sRestIBChunkSizeInByte;
    UInt                sState             = 0;
    idBool              sBackupCreateState = ID_FALSE;

    IDE_DASSERT( aDataFileDescSlot  != NULL );
    IDE_DASSERT( aDataFileNode      != NULL );
    IDE_DASSERT( aBackupInfo        != NULL );

    sSrcFile = &aDataFileNode->mFile;
    
    IDE_TEST( sDestFile.initialize( IDU_MEM_SM_SDD,
                                    1,                             
                                    IDU_FIO_STAT_OFF,              
                                    IDV_WAIT_INDEX_NULL )          
              != IDE_SUCCESS );              
    sState = 1;

    IDE_TEST( sDestFile.setFileName( aBackupInfo->mBackupFileName ) 
              != IDE_SUCCESS );              

    IDE_TEST( sDestFile.create()    != IDE_SUCCESS );
    sBackupCreateState = ID_TRUE;

    IDE_TEST( sDestFile.open()      != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( prepareIO( aStatistics,
                         aDataFileNode ) != IDE_SUCCESS );
    sPrepareIO = ID_TRUE;
 
    /* Incremental backup을 수행한다.*/
    IDE_TEST( smriChangeTrackingMgr::performIncrementalBackup( 
                                                   aDataFileDescSlot,
                                                   sSrcFile,
                                                   &sDestFile,
                                                   SMRI_CT_DISK_TBS,
                                                   SC_NULL_SPACEID, //chkpt img file spaceID-not use in DRDB
                                                   0,               //chkpt img file number-not use in DRDB
                                                   aBackupInfo )
              != IDE_SUCCESS );

    // Complete 자체는 lock이 필요하지 않지만
    // Complete가 풀려서 File Close 등 다른 작업이 진행되기 전에
    // 먼저 lock을 잡는다.
    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );

    sLocked    = ID_TRUE;
    sPrepareIO = ID_FALSE;
    IDE_TEST( completeIO( aStatistics,
                          aDataFileNode,
                          SDD_IO_READ ) != IDE_SUCCESS );

    /* 
     * 생성된 파일의 크기와 복사된 IBChunk의 개수를 비교해 backup된 크기가
     * 동일한지 확인한다
     */
    IDE_TEST( sDestFile.getFileSize( &sBackupFileSize ) != IDE_SUCCESS );

    sIBChunkSizeInByte = (ULong)SD_PAGE_SIZE * aBackupInfo->mIBChunkSize;

    sBackupSize = (ULong)aBackupInfo->mIBChunkCNT * sIBChunkSizeInByte;

    /* 
     * 파일의 확장이나 축소가 IBChunk의 크기만큼 이루지지 않을수 있기때문에
     * 실제 백업된 파일의 크기는 mIBChunkCNT의 배수가 아닐수 있다.
     * 파일의 크기가 mIBChunkCNT의 배수가 되도록 조정해준다.
     */
    if( sBackupFileSize > SM_DBFILE_METAHDR_PAGE_SIZE )
    {
        sBackupFileSize -= SM_DBFILE_METAHDR_PAGE_SIZE;

        sRestIBChunkSizeInByte = (ULong)sBackupFileSize % sIBChunkSizeInByte;

        IDE_ASSERT( sRestIBChunkSizeInByte % SD_PAGE_SIZE == 0 );

        if( sRestIBChunkSizeInByte != 0 )
        {
            sBackupFileSize += sIBChunkSizeInByte - sRestIBChunkSizeInByte;
        }
    }

    IDE_ASSERT( sBackupFileSize == sBackupSize );

    // backup한 DBFile의 DataFileHeader의 MustRedoToLSN을 갱신한다.
    smLayerCallback::getLstLSN( &sMustRedoToLSN );

    sDBFileHdr = aDataFileNode->mDBFileHdr;

    sddDataFile::setDBFHdr( &sDBFileHdr,
                            NULL,   // DiskRedoLSN
                            NULL,   // DiskCreateLSN
                            &sMustRedoToLSN,
                            NULL ); //DataFileDescSlotID

    sLocked = ID_FALSE;
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );

    sState = 1;
    IDE_TEST( sDestFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sDestFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sPrepareIO == ID_TRUE  )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                aDataFileNode,
                                SDD_IO_READ ) == IDE_SUCCESS );
    }

    if ( sLocked == ID_TRUE )
    {
        sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    }

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( sDestFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sDestFile.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    if( sBackupCreateState == ID_TRUE )
    {
        IDE_ASSERT( idf::unlink( aBackupInfo->mBackupFileName ) == 0 );
    }

    return IDE_FAILURE;
}

/*
  PRJ-1548 User Memory Tablespace

  서버구동시 복구이후에 모든 테이블스페이스의
  DataFileCount와 TotalPageCount를 계산하여 설정한다.

  본 프로젝트부터 sddTableSpaceNode의 mDataFileCount와
  mTotalPageCount는 RUNTIME 정보로 취급하므로
  서버구동시 복구이후에 정보를 보정해주어야 한다.
*/
void sddDiskMgr::calculateFileSizeOfAllTBS()
{
    UInt                sSpaceID;
    UInt                sMaxTBSCount;
    sddTableSpaceNode * sSpaceNode;

    sMaxTBSCount = sctTableSpaceMgr::getNewTableSpaceID();

    for( sSpaceID = 0 ; sSpaceID < sMaxTBSCount ; sSpaceID++ )
    {
        sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::getSpaceNodeBySpaceID( sSpaceID );

        if ( sSpaceNode != NULL )
        {
            //BUG-48460: restart 할 때 discard된 테이블 스페이스를 체크해야 함
            if (    SMI_TBS_IS_NOT_DROPPED( sSpaceNode->mHeader.mState )
                 && SMI_TBS_IS_NOT_DISCARDED( sSpaceNode->mHeader.mState ) )
            {
                if ( sctTableSpaceMgr::isDiskTableSpace( sSpaceNode ) == ID_TRUE )
                {
                    sddTableSpace::calculateFileSizeOfTBS( sSpaceNode );
                }
            }
        }
    }

    return;
}

/*
  PRJ-1548 User Memory Tablespace

  디스크 테이블스페이스를 모두 백업한다.

  [IN] aTrans : 트랜잭션
  [IN] aBackupDir : 백업 Dest. 디렉토리
*/
IDE_RC sddDiskMgr::backupAllDiskTBS( idvSQL    * aStatistics,
                                     SChar     * aBackupDir )
{
    sctActBackupArgs sActBackupArgs;

    IDE_DASSERT( aBackupDir != NULL );

    sActBackupArgs.mBackupDir           = aBackupDir;
    sActBackupArgs.mIsIncrementalBackup = ID_FALSE;

    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                                aStatistics,
                                sddTableSpace::doActOnlineBackup,
                                (void*)&sActBackupArgs, /* Action Argument*/
                                SCT_ACT_MODE_NONE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  PROJ-2133 increemntal backup

  디스크 테이블스페이스를 모두 백업한다.

  [IN] aTrans : 트랜잭션
  [IN] aBackupDir : 백업 Dest. 디렉토리
*/
IDE_RC sddDiskMgr::incrementalBackupAllDiskTBS( idvSQL     * aStatistics,
                                                smriBISlot * aCommonBackupInfo,
                                                SChar      * aBackupDir )
{
    sctActBackupArgs sActBackupArgs;

    IDE_DASSERT( aBackupDir != NULL );

    sActBackupArgs.mBackupDir           = aBackupDir;
    sActBackupArgs.mCommonBackupInfo    = aCommonBackupInfo;
    sActBackupArgs.mIsIncrementalBackup = ID_TRUE;

    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                                aStatistics,
                                sddTableSpace::doActOnlineBackup,
                                (void*)&sActBackupArgs, /* Action Argument*/
                                SCT_ACT_MODE_NONE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
  테이블스페이스의 Dirty된 데이타파일을 Sync한다.
  본 함수에 호출되기전에 TBS Mgr Latch는 획득된 상태이다.

  [IN] aSpaceNode : 검증할 TBS Node
  [IN] aActionArg : NULL
*/
IDE_RC sddDiskMgr::doActIdentifyAllDBFiles(
                                idvSQL             * aStatistics,
                                sctTableSpaceNode  * aSpaceNode,
                                void               * aActionArg )
{
    idBool                   sNeedRecovery;
    UInt                     i;
    sddTableSpaceNode*       sSpaceNode;
    sddDataFileNode*         sFileNode;
    sddDataFileHdr           sDBFileHdr;
    SChar                    sMsgBuf[ 512 ];
    sctActIdentifyDBArgs   * sIdentifyDBArgs;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aActionArg != NULL );

    idlOS::memset( &sDBFileHdr, 0x00, ID_SIZEOF(sddDataFileHdr) );

    while ( 1 )
    {
        if ( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) != ID_TRUE )
        {
            // 디스크 테이블스페이스가 아닌 경우 체크하지 않는다.
            break;
        }

        if ( sctTableSpaceMgr::hasState( aSpaceNode,
                                         SCT_SS_SKIP_IDENTIFY_DB )
             == ID_TRUE )
        {
            // 테이블스페이스가 DISCARD된 경우 체크하지 않는다.
            // 테이블스페이스가 삭제된 경우 체크하지 않는다.
            break;
        }

        sSpaceNode      = (sddTableSpaceNode*)aSpaceNode;
        sIdentifyDBArgs = (sctActIdentifyDBArgs*)aActionArg;

        for (i=0; i < sSpaceNode->mNewFileID ; i++ )
        {
            sFileNode = sSpaceNode->mFileNodeArr[i] ;

            if( sFileNode == NULL )
            {
                continue;
            }

            if ( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
            {
                // 데이타파일이 삭제된 경우 무시한다.
                continue;
            }

            // PRJ-1548 User Memory Tablespace
            // CREATING 상태 또는 DROPPING 상태의 파일노드는
            // 반드시 파일이 존재해야 한다.
            if ( idf::access( sFileNode->mName, F_OK|R_OK|W_OK ) != 0 )
            {
                // restart 중이면
                // 디스크 임시 테이블스페이스인 경우 다시 만들면 된다.
                // 체크포인트 등 다른 이유로(운영중에) 호출되면I 기존과 동일하게 에러 올림
                if ( ( smiGetStartupPhase() < SMI_STARTUP_SERVICE ) &&
                     ( sctTableSpaceMgr::isTempTableSpace( sSpaceNode ) == ID_TRUE ) )
                {
                    if ( sFileNode->mCurrSize > sFileNode->mInitSize )
                    {
                        sddDataFile::setCurrSize( sFileNode, sFileNode->mInitSize);
                    }
                    else
                    {
                        /* nothing to do */
                    }

                    IDE_TEST( sddDataFile::reuse( aStatistics, sFileNode ) != IDE_SUCCESS );

                    /* loganchor flush */
                    IDE_TEST( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                              != IDE_SUCCESS );

                }
                else
                {
                    sIdentifyDBArgs->mIsFileExist = ID_FALSE;
                    sIdentifyDBArgs->mIsValidDBF  = ID_FALSE;

                    idlOS::snprintf( sMsgBuf, 
                                     SM_MAX_FILE_NAME,
                                     "  [SM-WARNING] CANNOT IDENTIFY DATAFILE\n"
                                     "               [%s-<DBF ID:%"ID_UINT32_FMT">] Datafile Not Found\n",
                                     sSpaceNode->mHeader.mName,
                                     sFileNode->mID );

                    IDE_CALLBACK_SEND_MSG(sMsgBuf);
                }
                continue;
            }

            if ( sctTableSpaceMgr::isTempTableSpace( sSpaceNode ) == ID_TRUE )
            {
                // 디스크 임시 테이블스페이스인 경우 데이타파일이
                // 존재하는지 확인만 한다.
                continue;
            }


            // version, oldest lsn, create lsn 일치하지 않으면
            // media recovery가 필요하다.
            IDE_TEST_RAISE( checkValidationDBFHdr(
                                                aStatistics,
                                                sFileNode,
                                                &sDBFileHdr,
                                                &sNeedRecovery ) != IDE_SUCCESS,
                            err_data_file_header_invalid );

            if ( sNeedRecovery == ID_TRUE )
            {
                idlOS::snprintf( sMsgBuf, 
                                 SM_MAX_FILE_NAME,
                                 "  [SM-WARNING] CANNOT IDENTIFY DATAFILE\n"
                                 "               [%s-<DBF ID:%"ID_UINT32_FMT">] Mismatch Datafile Version\n",
                                 sSpaceNode->mHeader.mName,
                                 sFileNode->mID );
                IDE_CALLBACK_SEND_MSG( sMsgBuf );

                sIdentifyDBArgs->mIsValidDBF  = ID_FALSE;

                continue;
            }
        }

        break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_data_file_header_invalid )
    {
        /* BUG-33353 */
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidDatafileHeader,
                                  sFileNode->mName,
                                  sFileNode->mSpaceID,
                                  sFileNode->mID,
                                  (sFileNode->mDBFileHdr).mRedoLSN.mFileNo,
                                  (sFileNode->mDBFileHdr).mRedoLSN.mOffset,
                                  sDBFileHdr.mRedoLSN.mFileNo,
                                  sDBFileHdr.mRedoLSN.mOffset,
                                  (sFileNode->mDBFileHdr).mCreateLSN.mFileNo,
                                  (sFileNode->mDBFileHdr).mCreateLSN.mOffset,
                                  sDBFileHdr.mCreateLSN.mFileNo,
                                  sDBFileHdr.mCreateLSN.mOffset,
                                  smVersionID & SM_CHECK_VERSION_MASK,
                                  (sFileNode->mDBFileHdr).mSmVersion & SM_CHECK_VERSION_MASK,
                                  sDBFileHdr.mSmVersion & SM_CHECK_VERSION_MASK) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
  디스크 테이블스페이스의 미디어오류가 있는 데이타파일 목록을 만든다.

  [IN]  aSpaceNode     - 테이블스페이스 노드
  [IN]  aRecoveryType  - 미디어복구 타입
  [OUT] aDiskDBFCount  - 미디어오류가 발생한 데이타파일노드 개수
  [OUT] aFromRedoLSN   - 복구시작 Redo LSN
  [OUT] aToRedoLSN     - 복구완료 Redo LSN
*/
IDE_RC sddDiskMgr::makeMediaRecoveryDBFList(
                            idvSQL            * aStatistics,
                            sctTableSpaceNode * aSpaceNode,
                            smiRecoverType      aRecoveryType,
                            UInt              * aFailureDBFCount,
                            smLSN             * aFromRedoLSN,
                            smLSN             * aToRedoLSN )
{
    sddDataFileHdr      sDBFileHdr;
    sddDataFileNode   * sFileNode;
    smLSN               sHashNodeFromLSN;
    smLSN               sHashNodeToLSN;
    idBool              sIsMediaFailure;
    SChar               sMsgBuf[ SM_MAX_FILE_NAME ];
    sddTableSpaceNode * sSpaceNode;
    UInt                i;

    IDE_DASSERT( aSpaceNode       != NULL );
    IDE_DASSERT( aFailureDBFCount != NULL );
    IDE_DASSERT( aFromRedoLSN     != NULL );
    IDE_DASSERT( aToRedoLSN       != NULL );

    idlOS::memset( &sDBFileHdr, 0x00, ID_SIZEOF(sddDataFileHdr) );

    sSpaceNode = (sddTableSpaceNode *)aSpaceNode;

    for (i=0; i < sSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = sSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
        {
            continue;
        }

        /* ------------------------------------------------
         * [1] 데이타파일이 존재하는지 검사
         * ----------------------------------------------*/
        if ( idf::access( sFileNode->mName, F_OK|W_OK|R_OK ) != 0 )
        {
            idlOS::snprintf( sMsgBuf, 
                             SM_MAX_FILE_NAME,
                             "  [SM-WARNING] CANNOT IDENTIFY DATAFILE\n"
                             "               [%s-<DBF ID:%"ID_UINT32_FMT">] DATAFILE NOT FOUND\n",
                             aSpaceNode->mName,
                             sFileNode->mID );
            IDE_CALLBACK_SEND_MSG(sMsgBuf);

            IDE_RAISE( err_file_not_found );
        }

        /* ------------------------------------------------
         * [2] 데이타파일과 파일노드와 바이너리버전을 검사
         * ----------------------------------------------*/
        IDE_TEST_RAISE( checkValidationDBFHdr(
                                            aStatistics,
                                            sFileNode,
                                            &sDBFileHdr,
                                            &sIsMediaFailure) != IDE_SUCCESS,
                        err_data_file_header_invalid );

        if ( sIsMediaFailure == ID_TRUE )
        {
            /*
               미디어 오류가 존재하는 경우

               일치하지 않는 경우는 완전(COMPLETE) 복구를
               수행해야 한다.
            */
            IDE_TEST_RAISE( (aRecoveryType == SMI_RECOVER_UNTILTIME) ||
                            (aRecoveryType == SMI_RECOVER_UNTILCANCEL),
                            err_incomplete_media_recovery);
        }
        else
        {
            /*
              미디어오류가 없는 데이타파일

              불완전복구(INCOMPLETE) 미디어 복구시에는
              백업본을 가지고 재수행을 시작하므로 REDO LSN이
              로그앵커와 일치하지 않는 경우는 존재하지 않는다.

              그렇다고 완전복구시에 모든 데이타파일이
              오류가 있어야 한다는 말은 아니다.
            */
        }

        if ( (aRecoveryType == SMI_RECOVER_COMPLETE) &&
             (sIsMediaFailure != ID_TRUE) )
        {
            // 완전복구시 오류가 없는 데이타파인 경우
            // 미디어 복구가 필요없다.
        }
        else
        {
            // 오류를 복구하고자하는 완전복구 또는
            // 오류가 없는 불완전복구도
            // 모두 미디어 복구를 진행한다.

            // 불완전 복구 :
            // 데이타파일 헤더의 oldest lsn을 검사한 다음,
            // 불일치하는 경우 복구대상파일로 선정한다.

            // 완전복구 :
            // 모든 데이타파일이 복구대상이 된다.

            IDE_TEST( smLayerCallback::addRecvFileToHash(
                                           &sDBFileHdr,
                                           sFileNode->mName,
                                           &sHashNodeFromLSN,
                                           &sHashNodeToLSN )
                      != IDE_SUCCESS );

            if ( smLayerCallback::isLSNGT( aFromRedoLSN,
                                           &sHashNodeFromLSN )
                 == ID_TRUE)
            {
                SM_GET_LSN( *aFromRedoLSN, sHashNodeFromLSN );
            }
            if ( smLayerCallback::isLSNGT( &sHashNodeToLSN,
                                           aToRedoLSN )
                 == ID_TRUE)
            {
                SM_GET_LSN( *aToRedoLSN, sHashNodeToLSN );
            }

            *aFailureDBFCount = *aFailureDBFCount + 1;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_found );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistFile ) );
    }
    IDE_EXCEPTION( err_data_file_header_invalid );
    {
        /* BUG-33353 */
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidDatafileHeader,
                                  sFileNode->mName,
                                  sFileNode->mSpaceID,
                                  sFileNode->mID,
                                  (sFileNode->mDBFileHdr).mRedoLSN.mFileNo,
                                  (sFileNode->mDBFileHdr).mRedoLSN.mOffset,
                                  sDBFileHdr.mRedoLSN.mFileNo,
                                  sDBFileHdr.mRedoLSN.mOffset,
                                  (sFileNode->mDBFileHdr).mCreateLSN.mFileNo,
                                  (sFileNode->mDBFileHdr).mCreateLSN.mOffset,
                                  sDBFileHdr.mCreateLSN.mFileNo,
                                  sDBFileHdr.mCreateLSN.mOffset,
                                  smVersionID & SM_CHECK_VERSION_MASK,
                                  (sFileNode->mDBFileHdr).mSmVersion & SM_CHECK_VERSION_MASK,
                                  sDBFileHdr.mSmVersion & SM_CHECK_VERSION_MASK) );
    }
    IDE_EXCEPTION( err_incomplete_media_recovery );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NeedMediaRecovery ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  테이블 스페이스의 MustRedoToLSN값 중에 가장 큰 값을 반환한다.
 *
 * aSpaceNode     - [IN]  테이블스페이스 노드
 * aMustRedoToLSN - [OUT] 복구완료 Redo LSN
 * aDBFileName    - [OUT] 해당 Must Redo to LSN을 가진 DBFile
 **********************************************************************/
IDE_RC sddDiskMgr::getMustRedoToLSN( idvSQL            * aStatistics,
                                     sctTableSpaceNode * aSpaceNode,
                                     smLSN             * aMustRedoToLSN,
                                     SChar            ** aDBFileName )
{
    sddDataFileHdr      sDBFileHdr;
    sddDataFileNode   * sFileNode;
    sddTableSpaceNode * sSpaceNode;
    smLSN               sMustRedoToLSN;
    SChar             * sDBFileName = NULL;
    UInt                i;

    IDE_DASSERT( aSpaceNode     != NULL );
    IDE_DASSERT( aMustRedoToLSN != NULL );
    IDE_DASSERT( aDBFileName    != NULL );

    *aDBFileName = NULL;
    SM_LSN_INIT( sMustRedoToLSN );

    sSpaceNode = (sddTableSpaceNode *)aSpaceNode;

    for (i=0; i < sSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = sSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
        {
            continue;
        }

        // 데이타파일이 존재하는지 검사
        IDE_TEST_RAISE( idf::access( sFileNode->mName, F_OK|W_OK|R_OK )
                        != 0 , err_file_not_found );

        // DBFileHdr를 DBFile에서 읽어온다.
        IDE_TEST_RAISE( readDBFHdr( aStatistics,
                                    sFileNode,
                                    &sDBFileHdr ) != IDE_SUCCESS,
                        err_dbfilehdr_read_failure );

        if ( smLayerCallback::isLSNGT( &sDBFileHdr.mMustRedoToLSN,
                                       &sMustRedoToLSN )
             == ID_TRUE )
        {
            SM_GET_LSN( sMustRedoToLSN, sDBFileHdr.mMustRedoToLSN );
            sDBFileName = idlOS::strrchr( sFileNode->mName,
                                          IDL_FILE_SEPARATOR );
        }
    }

    SM_GET_LSN( *aMustRedoToLSN, sMustRedoToLSN );
    *aDBFileName = sDBFileName + 1; // '/' 제거하고 반환

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_found );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileByPath,
                                  sFileNode->mFile.getFileName() ) );
    }
    IDE_EXCEPTION( err_dbfilehdr_read_failure );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_Datafile_Header_Read_Failure,
                                  sFileNode->mFile.getFileName() ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
  데이타파일의 페이지 구간을 반환한다 .
*/
IDE_RC  sddDiskMgr::getPageRangeInFileByID( idvSQL          * aStatistics,
                                            scSpaceID         aSpaceID,
                                            UInt              aFileID,
                                            scPageID        * aFstPageID,
                                            scPageID        * aLstPageID )
{
    UInt                sState = 0;
    sddTableSpaceNode * sSpaceNode;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceID ) == ID_TRUE );
    IDE_DASSERT( aFstPageID != NULL );
    IDE_DASSERT( aLstPageID != NULL );

    IDE_TEST( sctTableSpaceMgr::findAndLockSpaceNodeBySpaceID( aStatistics,
                                                               aSpaceID,
                                                               (void**)&sSpaceNode )
                  != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sddTableSpace::getPageRangeInFileByID( sSpaceNode,
                                                     aFileID,
                                                     aFstPageID,
                                                     aLstPageID)
              != IDE_SUCCESS );

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aSpaceNode가 디스크 Tablesapce라면 Tablespace에 속해
 *               있는 모든 Datafile의 Max Open FD Count를 aMaxFDCnt4File
 *               로 변경한다.
 *
 * aSpaceNode     - [IN] Space Node Pointer
 * aMaxFDCnt4File - [IN] Max FD Count
 **********************************************************************/
IDE_RC  sddDiskMgr::setMaxFDCnt4AllDFileOfTBS( sctTableSpaceNode* aSpaceNode,
                                               UInt               aMaxFDCnt4File )
{
    sddTableSpaceNode*       sSpaceNode;
    sddDataFileNode*         sFileNode;
    sdFileID                 sFileID;
    UInt                     sState = 0;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE );

    sctTableSpaceMgr::lockSpaceNode( NULL /* idvSQL* */,
                                     aSpaceNode );
    sState = 1;

    // 1. 테이블스페이스가 DISCARD된 경우 체크하지 않는다.
    // 2. 테이블스페이스가 삭제된 경우 체크하지 않는다.
    IDE_TEST_CONT( sctTableSpaceMgr::hasState( aSpaceNode,
                                                SCT_SS_SKIP_IDENTIFY_DB )
                    == ID_TRUE, err_tbs_skip );

    sSpaceNode = (sddTableSpaceNode*)aSpaceNode;

    for( sFileID = 0; sFileID < sSpaceNode->mNewFileID; sFileID++ )
    {
        sFileNode = sSpaceNode->mFileNodeArr[ sFileID ];

        if( sFileNode == NULL )
        {
            // 완전히 Drop된 경우.
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
        {
            // 데이타파일이 삭제된 경우 무시한다.
            continue;
        }

        IDE_TEST( sddDataFile::setMaxFDCount( sFileNode,
                                              aMaxFDCnt4File )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( err_tbs_skip );
    
    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    }

    return IDE_FAILURE;
}

/*
  Offline TBS에 포함된 Table들의 Runtime Index header들을
  모두 free 한다.

  [ 참고 ]
  Restart Reocvery 과정중 Undo All 완료 직후에 호출된다.

  [ 인자 ]
  [IN] aSpaceNode - TBS 노드

 */
IDE_RC sddDiskMgr::finiOfflineTBSAction( idvSQL            * aStatistics ,
                                         sctTableSpaceNode * aSpaceNode,
                                         void              * /* aActionArg */ )
{
    IDE_DASSERT( aSpaceNode != NULL );

    if ( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE )
    {
        // Tablespace의 상태가 Offline이면
        if (SMI_TBS_IS_OFFLINE(aSpaceNode->mState) )
        {
            // Table의 Runtime Item  및 Runtime Index Header를 제거한다.
            IDE_TEST( smLayerCallback::alterTBSOffline4Tables( aStatistics,
                                                               aSpaceNode->mID )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To do..
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*

  RESIZE DBF 에 대한 Shrink Pending 연산을 수행한다.

  [IN] aSpaceNode : TBS Node
  [IN] aPendingOp : Pending 구조체

*/
IDE_RC sddDiskMgr::shrinkFilePending( idvSQL            * aStatistics,
                                      sctTableSpaceNode * /*aSpaceNode*/,
                                      sctPendingOp      * aPendingOp  )
{
    sddDataFileNode   * sFileNode;
    scPageID            sFstPID;
    scPageID            sLstPID;
    UInt                sPreparedIO = 0;

    IDE_DASSERT( aPendingOp != NULL );
    IDE_DASSERT( aPendingOp->mPendingOpParam != NULL );

    sFileNode  = (sddDataFileNode*)aPendingOp->mPendingOpParam;

    // 버퍼상에 적재된 관련 페이지를 무효화 시킨다.
    // GC가 접근하지 못하도록 한다.
    IDE_TEST( getPageRangeInFileByID( aStatistics,
                                      sFileNode->mSpaceID,
                                      sFileNode->mID,
                                      &sFstPID,
                                      &sLstPID ) != IDE_SUCCESS );

    sFstPID = SD_CREATE_PID( sFileNode->mID, aPendingOp->mResizeSizeWanted );

    IDE_TEST( smLayerCallback::discardPagesInRangeInSBuffer(
                                      aStatistics,
                                      sFileNode->mSpaceID,
                                      sFstPID,
                                      sLstPID )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::discardPagesInRange(
                                      aStatistics,
                                      sFileNode->mSpaceID,
                                      sFstPID,
                                      sLstPID )
              != IDE_SUCCESS );

    IDE_TEST( prepareIO( aStatistics,
                         sFileNode ) != IDE_SUCCESS );
    sPreparedIO = 1;

    IDE_TEST( sddDataFile::truncate( sFileNode,
                                     aPendingOp->mResizeSizeWanted )
              != IDE_SUCCESS );

    sddDataFile::setCurrSize( sFileNode, aPendingOp->mResizeSizeWanted );

    if( aPendingOp->mResizeSizeWanted < sFileNode->mInitSize )
    {
        sddDataFile::setInitSize(sFileNode,
                                 aPendingOp->mResizeSizeWanted);
    }

    sPreparedIO = 0;
    IDE_TEST( completeIO( aStatistics,
                          sFileNode,
                          SDD_IO_WRITE ) != IDE_SUCCESS );

    /* loganchor flush */
    IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sPreparedIO != 0 )
    {
        IDE_ASSERT( completeIO( aStatistics,
                                sFileNode,
                                SDD_IO_WRITE ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------
 * PROJ-2118 BUG Reporting
 *
 * Description : Disk File상의 Page의 내용을 지정한 trace file에 뿌려준다.
 *
 * ----------------------------------------------------------------- */
IDE_RC sddDiskMgr::tracePageInFile( UInt            aChkFlag,
                                    ideLogModule    aModule,
                                    UInt            aLevel,
                                    scSpaceID       aSpaceID,
                                    scPageID        aPageID,
                                    const SChar   * aTitle )
{
    smuAlignBuf  sPagePtr;
    UInt         sState = 0;

    IDE_TEST( smuUtility::allocAlignedBuf( IDU_MEM_SM_SDD,
                                           SD_PAGE_SIZE,
                                           SD_PAGE_SIZE,
                                           &sPagePtr )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( read( NULL,
                    aSpaceID,
                    aPageID,
                    sPagePtr.mAlignPtr )
              != IDE_SUCCESS );

    if( aTitle != NULL )
    {
        (void)smLayerCallback::tracePage( aChkFlag,
                                          aModule,
                                          aLevel,
                                          sPagePtr.mAlignPtr,
                                          "%s ( "
                                          "Space ID : %"ID_UINT32_FMT", "
                                          "Page ID : %"ID_UINT32_FMT" ):",
                                          aTitle,
                                          aSpaceID,
                                          aPageID );
    }
    else
    {
        (void)smLayerCallback::tracePage( aChkFlag,
                                          aModule,
                                          aLevel,
                                          sPagePtr.mAlignPtr,
                                          "Dump Disk Page In Data File ( "
                                          "Space ID : %"ID_UINT32_FMT", "
                                          "Page ID : %"ID_UINT32_FMT" ):",
                                          aSpaceID,
                                          aPageID );
    }

    sState = 0;
    IDE_TEST( smuUtility::freeAlignedBuf( &sPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( smuUtility::freeAlignedBuf( &sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------
 * BUG-48275 recovery 에서 잘못된 오류 메시지가 출력 됩니다. 
 *           pageid 기준으로 filename 알아오는 기능 추가
 * ----------------------------------------------------------------- */
SChar* sddDiskMgr::getFileName( scSpaceID aSpaceID,
                                scPageID  aPageID )
{
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sDataFileNode;
    SChar             * sFileName = NULL;
    UInt                sFileIndex = SD_MAKE_FID(aPageID) ;
        
    if ( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID, 
                                                   (void**)&sSpaceNode )
              == IDE_SUCCESS )
    {
        if ( sSpaceNode != NULL )
        {
            if ( sFileIndex < sSpaceNode->mNewFileID )
            {
                sDataFileNode = sSpaceNode->mFileNodeArr[ sFileIndex ];

                if ( sDataFileNode != NULL )
                {
                    if ( sDataFileNode->mName != NULL )
                    {
                        sFileName = idlOS::strrchr( sDataFileNode->mName,
                                                IDL_FILE_SEPARATOR ) ;
                    }
                }
            }
        }
    }
    return sFileName;
}
