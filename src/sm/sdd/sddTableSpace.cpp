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
 * $Id: sddTableSpace.cpp 91197 2021-07-12 01:15:29Z emlee $
 *
 * Description :
 *
 * 본 파일은 디스크관리자의 tablespace 노드에 대한 구현파일이다.
 *
 * 1. tablespace의 생성
 *
 *   - 디스크관리자의 mutex 획득
 *   - sddDiskMgr::createTableSpace
 *     : 디스크관리자에 의해 tablespace 노드 메모리 할당한다.
 *     + sddTableSpace::initialize 초기화
 *       : tablespace 정보 설정
 *     + sddTableSpace::createDataFile
 *       : datafile 생성 및 초기화, datafile 리스트에 추가
 *   - 디스크관리자의 datafile 오픈과 동시에 datafile 리스트 LRU에도 추가
 *   - 디스크관리자의 mutex 풀기
 *
 * 2. tablespace의 로드
 *
 *   - 로그앵커로부터 tablespace 정보를 sddTableSpaceAttr로 읽음
 *   - 디스크관리자의 mutex 획득
 *   - 디스크관리자에 의해 tablespace 노드를 생성
 *   - sddTableSpaceNode::initialize 초기화
 *   - 디스크관리자의 createDataFileNode에 의해
 *     sddTableSpaceNode::attachDataFile을 반복호출
 *     (sddTableSpaceNode는 datafile 노드를 생성한 후, datafile 리스트에 추가)
 *   - 디스크관리자의 datafile오픈과 동시에 datafile LRU 리스트에도 추가
 *   - 디스크매니저의 mutex 풀기
 *
 * 3. tablespace의 제거
 *
 *   - 디스크관리자의 mutex 획득
 *   - sddDiskMgr::removeTableSpace
 *     : 디스크관리자에서 해당 테이블스페이스의 datafile(s)들에 대하여
 *       close 함과 동시에 datafile LRU 리스트에서 제거 후 tablespace 노드 제거
 *     + tablespace의 drop모드에 따라서 sddTableSpace::closeDataFile 또는
 *       sddTableSpace::dropDataFile 반복수행
 *     + sddTableSpace::destory를 수행하여 datafile 리스트에
 *       대한 메모리 해제
 *   - 디스크매니저에 의해 TableSpace Node 제거
 *   - 디스크매니저의 mutex 풀기
 *
 *
 * 4. tablespace의 변경
 *   - datafile 추가 및 제거(?)
 *   - tablespace의 운영중 변경이 가능한 속성 변경
 *
 *
 **********************************************************************/
#include <idl.h> // for win32

#include <sddReq.h>
#include <smErrorCode.h>
#include <sddTableSpace.h>
#include <sddDataFile.h>
#include <sddDiskMgr.h>
#include <sddReq.h>
#include <sctTableSpaceMgr.h>
#include <smriChangeTrackingMgr.h>

/***********************************************************************
 * Description : tablespace 노드 초기화
 *
 * tablespace 노드에 다음과 같은 여러가지 속성을 설정한다.
 * - tablespace명, tablespace ID, tablespace 타입, max page 개수,
 * online/offline 여부, datafile 개수등의 정보를 설정한다.
 *
 * + 2nd. code design
 *   - tablespace ID 설정
 *   - tablespace 타입
 *   - tablespace 명
 *   - tablespace의 online/offline 여부
 *   - tablespace가 소유한 datafile의 개수
 *   - 소유한 datafile의 연결 리스트
 *   - tableSpace에 포함된 모든 page의 개수
 *   - tablespace의 연결 리스트
 ***********************************************************************/
IDE_RC sddTableSpace::initialize( sddTableSpaceNode   * aSpaceNode,
                                  smiTableSpaceAttr   * aSpaceAttr )
{
    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aSpaceAttr != NULL );
    IDE_DASSERT( aSpaceAttr->mAttrType == SMI_TBS_ATTR );
    IDE_DASSERT( aSpaceNode->mHeader.mLockItem4TBS == NULL );

    // Tablespace Node 초기화 ( Disk/Memory 공통 )
    IDE_TEST( sctTableSpaceMgr::initializeTBSNode( & aSpaceNode->mHeader,
                                                   aSpaceAttr )
              != IDE_SUCCESS );

    // 속성 플래그 초기화
    aSpaceNode->mAttrFlag = aSpaceAttr->mAttrFlag;

    //tablespace가 소유한 datafile 배열초기화및 개수 설정.
    idlOS::memset( aSpaceNode->mFileNodeArr,
                   0x00,
                   ID_SIZEOF(aSpaceNode->mFileNodeArr) );
    aSpaceNode->mDataFileCount = 0;

    /* tableSpace에 포함된 모든 page의 개수 */
    aSpaceNode->mTotalPageCount  = 0;
    aSpaceNode->mNewFileID       = aSpaceAttr->mDiskAttr.mNewFileID;

    aSpaceNode->mExtPageCount = aSpaceAttr->mDiskAttr.mExtPageCount;

    /* 인자로 전달된 Extent 관리방식을 설정한다. */
    aSpaceNode->mExtMgmtType = aSpaceAttr->mDiskAttr.mExtMgmtType;

    /* 인자로 전달된 Segment 관리방식을 설정한다. */
    aSpaceNode->mSegMgmtType = aSpaceAttr->mDiskAttr.mSegMgmtType;

    /* Space Cache는 이후 Page Layer 초기화시 할당되어 설정된다. */
    aSpaceNode->mSpaceCache = NULL;

    aSpaceNode->mMaxSmoNoForOffline = 0;

    // PRJ-1548 User Memory Tablespace
    aSpaceNode->mAnchorOffset = SCT_UNSAVED_ATTRIBUTE_OFFSET;

    // fix BUG-17456 Disk Tablespace online이후 update 발생시 index 무한루프
    SM_LSN_INIT( aSpaceNode->mOnlineTBSLSN4Idx );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : tablespace 노드의 모든 자원 해제
 *
 * 모드에 따라 datafile 리스트를 제거하고 그외 할당되었던
 * 메모리를 해제한다. 이 함수가 호출되기전에 디스크관리자의
 * mutex가 획득된 상태여야 한다.
 *
 * + 2nd. code design
 *   - tablespace 노드의 모든 datafile 노드를 모드에 따라
 *     제거한다.
 *   - tablespace 이름에 할당되었던 메모리를 해제한다.
 ***********************************************************************/
IDE_RC sddTableSpace::destroy( sddTableSpaceNode* aSpaceNode )
{

    IDE_TEST( iduMemMgr::free(aSpaceNode->mHeader.mName) != IDE_SUCCESS );

    if( aSpaceNode->mHeader.mLockItem4TBS != NULL )
    {
        IDE_TEST( smLayerCallback::destroyLockItem( aSpaceNode->mHeader.mLockItem4TBS )
                  != IDE_SUCCESS );
        IDE_TEST( smLayerCallback::freeLockItem( aSpaceNode->mHeader.mLockItem4TBS )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : datafile 노드 및 datafile 생성(여러개)
 *
 * - DDL문에 의해 tablespace의 datafile 노드뿐만 아니라 대응되는
 * datafile들도 생성한다. 또한, 이 함수가 수행되는 동안에는 아직까지
 * 디스크관리자에 tablespace가 등록되지 않은 상황이므로,
 * mutex를 획득할 필요도 없다.
 *
 * - 로그앵커에 의한 초기화시 datafile 노드만 생성
 * 로그앵커로부터 초기화 정보를 읽어 tablespace의 한개의 datafile
 * 노드만 생성한다. 이는 로그앵커에 저장되어 있는 datafile 노드
 * 정보에 해당하는 datafile들은 이미 존재하기 때문이다.
 *
 * - 하나의 datafile create시 해당 이름의 datafile이 이미
 * 존재하는지 검사해야 한다.
 *
 * + 2nd. code design
 *   - for ( attribute 개수 만큼 )
 *     {
 *        datafile 노드를 위한 메모리 할당한다.
 *        datafile 노드를 초기화한다.
 *        실제 datafile을 생성한다.
 *        base 노드에 datafile 리스트에 datafile 노드를 추가한다.
 *     }
 ***********************************************************************/
IDE_RC sddTableSpace::createDataFiles(
                            idvSQL             * aStatistics,
                            void               * aTrans,
                            sddTableSpaceNode  * aSpaceNode,
                            smiDataFileAttr   ** aFileAttr,
                            UInt                 aFileAttrCnt,
                            smiTouchMode         aTouchMode,
                            UInt                 aMaxDataFilePageCount )
{
    UInt                i       = 0;
    UInt                j       = 0;
    UInt                sState  = 0;
    sddDataFileNode   * sFileNode      = NULL;
    sddDataFileNode  ** sFileNodeArray = NULL;
    smLSN               sCreateLSN;
    smLSN               sMustRedoToLSN;
    idBool              sCheckPathVal;
    idBool            * sFileArray          = NULL;
    idBool              sDataFileDescState  = ID_FALSE;
    SInt                sCurFileNodeState   = 0;
    smiDataFileDescSlotID     * sSlotID     = NULL;

    IDE_DASSERT( aFileAttr      != NULL );
    IDE_DASSERT( aFileAttrCnt    > 0 );

    // 디스크관리자의 mMaxDataFileSize
    IDE_DASSERT( aMaxDataFilePageCount  > 0 );
    IDE_DASSERT( aSpaceNode             != NULL );
    IDE_DASSERT( (aTouchMode == SMI_ALL_NOTOUCH) ||
                 (aTouchMode == SMI_EACH_BYMODE) );

    SM_LSN_INIT( sCreateLSN );
    SM_LSN_INIT( sMustRedoToLSN );

    /* sddTableSpace_createDataFiles_calloc_FileNodeArray.tc */
    IDU_FIT_POINT("sddTableSpace::createDataFiles::calloc::FileNodeArray");
    IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SDD,
                                aFileAttrCnt,
                                ID_SIZEOF(sddDataFileNode *),
                                (void**)&sFileNodeArray,
                                IDU_MEM_FORCE)
              != IDE_SUCCESS );
    sState = 1;

    /* sddTableSpace_createDataFiles_calloc_FileArray.tc */
    IDU_FIT_POINT("sddTableSpace::createDataFiles::calloc::FileArray");
    IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SDD,
                                aFileAttrCnt,
                                ID_SIZEOF(idBool),
                                (void**)&sFileArray,
                                IDU_MEM_FORCE)
              != IDE_SUCCESS );
    sState = 2;

    for (i = 0; i < aFileAttrCnt; i++)
    {
        sCurFileNodeState = 0;

        /* ====================================
         * [1] 데이타 화일 노드 할당
         * ==================================== */

        /* sddTableSpace_createDataFiles_malloc_FileNode.tc */
        IDU_FIT_POINT("sddTableSpace::createDataFiles::malloc::FileNode");
        IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SDD,
                                    ID_SIZEOF(sddDataFileNode),
                                    (void**)&sFileNode,
                                    IDU_MEM_FORCE) != IDE_SUCCESS );
        sCurFileNodeState = 1;

        idlOS::memset(sFileNode, 0x00, ID_SIZEOF(sddDataFileNode));

        sFileNodeArray[i] = sFileNode;

        sCheckPathVal = (aTouchMode == SMI_ALL_NOTOUCH) ?
                        ID_FALSE: ID_TRUE;

        /* ====================================
         * [2] 데이타 화일 ID 부여
         * ==================================== */
        if (aTouchMode != SMI_ALL_NOTOUCH)
        {
            aFileAttr[i]->mID = getNewFileID( aSpaceNode );
        }

        /* ====================================
         * [3] 데이타 화일 초기화
         * ==================================== */
        IDE_TEST( sddDataFile::initialize(aSpaceNode->mHeader.mID,
                                          sFileNode,
                                          aFileAttr[i],
                                          aMaxDataFilePageCount,
                                          sCheckPathVal)
                  != IDE_SUCCESS );
        sCurFileNodeState = 2;

        /* ====================================
         * [4] 물리적 데이타 화일 생성/reuse
         * ==================================== */

        switch(aTouchMode)
        {
            case SMI_ALL_NOTOUCH :

                ///////////////////////////////////////////////////
                // 서버구동시에 Loganchor로부터 디스크관리자 초기화
                ///////////////////////////////////////////////////

                IDE_ASSERT( aTrans == NULL );

                // 서버 바이너리 버전을 데이타파일헤더에 설정한다.
                sFileNode->mDBFileHdr.mSmVersion =
                    smLayerCallback::getSmVersionIDFromLogAnchor();

                // DiskRedoLSN, DiskCreateLSN을
                // 데이타파일헤더에 설정한다.
                sddDataFile::setDBFHdr( &(sFileNode->mDBFileHdr),
                                        sctTableSpaceMgr::getDiskRedoLSN(),
                                        &aFileAttr[i]->mCreateLSN,
                                        &sMustRedoToLSN,
                                        &aFileAttr[i]->mDataFileDescSlotID );
                break;

            // for create tablespace, add datafile - reuse phase
            case SMI_EACH_BYMODE :

                ///////////////////////////////////////////////////
                // 데이타파일 생성 과정
                ///////////////////////////////////////////////////

                IDE_ASSERT( aTrans != NULL );

                if ( sFileNode->mCreateMode == SMI_DATAFILE_CREATE )
                {
                    IDE_TEST_RAISE( idf::access( sFileNode->mName, F_OK ) == 0,
                                    error_already_exist_datafile );
                }

                IDE_TEST( smLayerCallback::writeLogCreateDBF( aStatistics,
                                                              aTrans,
                                                              aSpaceNode->mHeader.mID,
                                                              sFileNode,
                                                              aTouchMode,
                                                              aFileAttr[ i ], /* PROJ-1923 */
                                                              &sCreateLSN )
                          != IDE_SUCCESS );

                sFileNode->mState = SMI_FILE_CREATING | SMI_FILE_ONLINE;

                // !!CHECK RECOVERY POINT

                // 서버 바이너리 버전을 데이타파일헤더에 설정한다.
                sFileNode->mDBFileHdr.mSmVersion = smVersionID;

                //PROJ-2133 incremental backup 
                //change tracking파일에 데이터파일등록
                // temptablespace 에 대해서는 등록할 필요가 없습니다.
                if ( ( smLayerCallback::isCTMgrEnabled() == ID_TRUE ) &&
                     ( sctTableSpaceMgr::isTempTableSpace( aSpaceNode )
                       != ID_TRUE ) )
                {
                    IDE_TEST( smriChangeTrackingMgr::addDataFile2CTFile( 
                                                      aSpaceNode->mHeader.mID,
                                                      aFileAttr[i]->mID,
                                                      SMRI_CT_DISK_TBS,
                                                      &sSlotID )
                              != IDE_SUCCESS );

                    sDataFileDescState = ID_TRUE;
                }
                else
                {
                    //nothing to do
                }

                // 최근 수행된 체크포인트정보를 얻어서
                // 데이타파일헤더에 설정한다.
                // DiskRedoLSN, DiskCreateLSN을
                // 데이타파일헤더에 설정한다.
                // CreateLSN은 SCT_UPDATE_DRDB_CREATE_DBF 로그의 LSN
                // 파일헤더에 설정한다.
                sddDataFile::setDBFHdr( &(sFileNode->mDBFileHdr),
                                        sctTableSpaceMgr::getDiskRedoLSN(),
                                        &sCreateLSN,
                                        &sMustRedoToLSN,
                                        sSlotID );

                //PRJ-1149 , 파일 생성 시점의 LSN기록을 한다.
                if(sFileNode->mCreateMode == SMI_DATAFILE_REUSE)
                {
                    /* BUG-32950 when creating tablespace with reuse clause,
                     *           can not check the same file.
                     * 이미 동일 이름을 갖는 데이터 파일을 추가했는지
                     * 확인한다.
                     * 0번째 부터 i - 1 번째까지 i와 비교한다. */
                    for ( j = 0; j < i; j++ )
                    {
                        IDE_TEST_RAISE(
                                idlOS::strcmp( sFileNodeArray[j]->mName,
                                               sFileNodeArray[i]->mName ) == 0,
                                error_already_exist_datafile );
                    }

                    IDE_TEST( sddDataFile::reuse( aStatistics,
                                                  sFileNode )
                              != IDE_SUCCESS );


                }
                else
                {
                    IDE_TEST( sddDataFile::create( aStatistics,
                                                   sFileNode )
                              != IDE_SUCCESS );
                    sFileArray[i] = ID_TRUE;
                }
                break;
            default :
                IDE_ASSERT(0);
                break;
        }
        /* ====================================
         * [5] 데이타 화일 노드 리스트에 연결
         * ==================================== */
        addDataFileNode(aSpaceNode, sFileNode);
    }

    /*BUG-16197: Memory가 샘*/
    sState = 1;
    IDE_TEST( iduMemMgr::free( sFileArray ) != IDE_SUCCESS );
    sFileArray = NULL;

    sState = 0;
    IDE_TEST( iduMemMgr::free( sFileNodeArray ) != IDE_SUCCESS );
    sFileNodeArray = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_exist_datafile );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AlreadyExistFile, sFileNode->mName));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch (sState)
        {
            case 2:
                for( j = 0; j < aFileAttrCnt; j++ )
                {
                    if( sFileNodeArray[j] != NULL )
                    {
                        if( (sFileArray != NULL) && (sFileArray[j] == ID_TRUE) )
                        {
                            /*
                             * BUG-26215 
                             *  [SD]create tablespace시 마지막 파일이 autoextend on으로
                             *  설정되고 크기를 property의 max값보다 크게 입력하면
                             *  윈도우에서 비정상 종료할수 있음.
                             */
                            if( idf::access( sFileNodeArray[j]->mName, F_OK ) == 0 )
                            {
                                (void)idf::unlink(sFileNodeArray[j]->mName);
                            }
                        }

                        /* BUG-18176
                           위에서 생성한 DataFileNode에 대해
                           sddDataFile::initialize()에서 할당한 자원을 해제해줘야 한다.
                           i는 할당한 횟수이기 때문에
                           i보다 작을 땐 무조건 해제해야 하고
                           i번째에 대해서는 sCurFileNodeState로 할당 유무를 판단한다. */
                        if (j < i)
                        {
                            IDE_ASSERT( sddDataFile::destroy( sFileNodeArray[j] ) == IDE_SUCCESS );
                            IDE_ASSERT( iduMemMgr::free( sFileNodeArray[j] ) == IDE_SUCCESS );
                        }
                        else if (j == i)
                        {
                            switch (sCurFileNodeState)
                            {
                                case 2:
                                    IDE_ASSERT( sddDataFile::destroy( sFileNodeArray[j] ) == IDE_SUCCESS );
                                case 1:
                                    IDE_ASSERT( iduMemMgr::free( sFileNodeArray[j] ) == IDE_SUCCESS );
                                case 0:
                                    break;
                                default:
                                    IDE_ASSERT(0);
                                    break;
                            }
                        }
                        else
                        {
                            /* j 가 i보다 크다면 더 루프를 돌 필요가 없다. */
                            break;
                        }
                    }
                }
                IDE_ASSERT( iduMemMgr::free(sFileArray) == IDE_SUCCESS );
            case 1:
                IDE_ASSERT( iduMemMgr::free(sFileNodeArray) == IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    IDE_POP();

    if( sDataFileDescState == ID_TRUE )
    {
        IDE_ASSERT( smriChangeTrackingMgr::deleteDataFileFromCTFile( sSlotID ) 
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-1923 ALTIBASE HDB Disaster Recovery
 * Description : datafile 노드 및 datafile 생성(redo only)
 *
 * - restart recovery 에서 redo 중이므로, mutex를 획득할 필요도 없다.
 *
 * - 로그앵커에 의한 초기화시 datafile 노드만 생성
 * 로그앵커로부터 초기화 정보를 읽어 tablespace의 한개의 datafile
 * 노드만 생성한다. 이는 로그앵커에 저장되어 있는 datafile 노드
 * 정보에 해당하는 datafile들은 이미 존재하기 때문이다.
 *
 * - 하나의 datafile create시 해당 이름의 datafile이 이미
 * 존재하는지 검사해야 한다.
 *
 * + 2nd. code design
 *   - for ( attribute 개수 만큼 )
 *     {
 *        datafile 노드를 위한 메모리 할당한다.
 *        datafile 노드를 초기화한다.
 *        실제 datafile을 생성한다.
 *        base 노드에 datafile 리스트에 datafile 노드를 추가한다.
 *     }
 ***********************************************************************/
IDE_RC sddTableSpace::createDataFile4Redo(
                            idvSQL            * aStatistics,
                            void              * aTrans,
                            sddTableSpaceNode * aSpaceNode,
                            smiDataFileAttr   * aFileAttr,
                            smLSN               aCurLSN,
                            smiTouchMode        aTouchMode,
                            UInt                aMaxDataFilePageCount )
{
    UInt                    i       = 0;
    UInt                    j       = 0;
    UInt                    sState  = 0;
    sddDataFileNode       * sFileNode       = NULL;
    sddDataFileNode      ** sFileNodeArray  = NULL;
    smLSN                   sCreateLSN;
    smLSN                   sMustRedoToLSN;
    idBool                  sCheckPathVal;
    idBool                * sFileArray          = NULL;
    idBool                  sDataFileDescState  = ID_FALSE;
    SInt                    sCurFileNodeState   = 0;
    smiDataFileDescSlotID * sSlotID             = NULL;
    smFileID                sNewFileID          = 0;

    IDE_DASSERT( aFileAttr      != NULL );

    // 디스크관리자의 mMaxDataFileSize
    IDE_DASSERT( aMaxDataFilePageCount  > 0 );
    IDE_DASSERT( aSpaceNode             != NULL );
    IDE_DASSERT( (aTouchMode == SMI_ALL_NOTOUCH) ||
                 (aTouchMode == SMI_EACH_BYMODE) );

    SM_LSN_INIT( sCreateLSN );
    SM_LSN_INIT( sMustRedoToLSN );

    /* sddTableSpace_createDataFile4Redo_calloc_FileNodeArray.tc */
    IDU_FIT_POINT("sddTableSpace::createDataFile4Redo::calloc::FileNodeArray");
    IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SDD,
                                1,
                                ID_SIZEOF(sddDataFileNode *),
                                (void**)&sFileNodeArray,
                                IDU_MEM_FORCE)
              != IDE_SUCCESS );
    sState = 1;

    /* sddTableSpace_createDataFile4Redo_calloc_FileArray.tc */
    IDU_FIT_POINT("sddTableSpace::createDataFile4Redo::calloc::FileArray");
    IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SDD,
                                1,
                                ID_SIZEOF(idBool),
                                (void**)&sFileArray,
                                IDU_MEM_FORCE)
              != IDE_SUCCESS );
    sState = 2;

    sCurFileNodeState = 0;

    /* ====================================
     * [1] 데이타 화일 노드 할당
     * ==================================== */
    /* sddTableSpace_createDataFile4Redo_malloc_FileNode.tc */
    IDU_FIT_POINT("sddTableSpace::createDataFile4Redo::malloc::FileNode");
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SDD,
                                ID_SIZEOF(sddDataFileNode),
                                (void**)&sFileNode,
                                IDU_MEM_FORCE) != IDE_SUCCESS );
    sCurFileNodeState = 1;

    idlOS::memset(sFileNode, 0x00, ID_SIZEOF(sddDataFileNode));

    sFileNodeArray[i] = sFileNode;

    sCheckPathVal = (aTouchMode == SMI_ALL_NOTOUCH) ?
        ID_FALSE: ID_TRUE;

    /* ====================================
     * [2] 데이타 화일 ID 부여
     * ==================================== */
    sNewFileID = aSpaceNode->mNewFileID;

    IDE_TEST( sNewFileID != aFileAttr->mID );
    ideLog::log( IDE_SM_0,
                 "redo createDataFile, FileID in Spacenode : %"ID_UINT32_FMT""
                 ", FileID in Log : %"ID_UINT32_FMT" ",
                 sNewFileID, aFileAttr->mID );

    if (aTouchMode != SMI_ALL_NOTOUCH)
    {
        aFileAttr->mID = getNewFileID( aSpaceNode );
    }

    /* ====================================
     * [3] 데이타 화일 초기화
     * ==================================== */
    IDE_TEST( sddDataFile::initialize(aSpaceNode->mHeader.mID,
                                      sFileNode,
                                      aFileAttr,
                                      aMaxDataFilePageCount,
                                      sCheckPathVal)
              != IDE_SUCCESS );
    sCurFileNodeState = 2;

    /* ====================================
     * [4] 물리적 데이타 화일 생성/reuse
     * ==================================== */

    switch(aTouchMode)
    {
        case SMI_ALL_NOTOUCH :

            ///////////////////////////////////////////////////
            // 서버구동시에 Loganchor로부터 디스크관리자 초기화
            ///////////////////////////////////////////////////

            IDE_ASSERT( aTrans == NULL );

            // 서버 바이너리 버전을 데이타파일헤더에 설정한다.
            sFileNode->mDBFileHdr.mSmVersion =
                smLayerCallback::getSmVersionIDFromLogAnchor();

            // DiskRedoLSN, DiskCreateLSN을
            // 데이타파일헤더에 설정한다.
            sddDataFile::setDBFHdr( &(sFileNode->mDBFileHdr),
                                    sctTableSpaceMgr::getDiskRedoLSN(),
                                    &aFileAttr->mCreateLSN,
                                    &sMustRedoToLSN,
                                    &aFileAttr->mDataFileDescSlotID );
            break;

            // for create tablespace, add datafile - reuse phase
        case SMI_EACH_BYMODE :

            ///////////////////////////////////////////////////
            // 데이타파일 생성 과정
            ///////////////////////////////////////////////////

            IDE_ASSERT( aTrans != NULL );

            if(sFileNode->mCreateMode == SMI_DATAFILE_CREATE)
            {
                IDE_TEST_RAISE( idf::access( sFileNode->mName, F_OK) == 0,
                                error_already_exist_datafile );
            }

            SM_GET_LSN( sCreateLSN, aCurLSN );
            SM_GET_LSN( aFileAttr->mCreateLSN, aCurLSN );

            sFileNode->mState = SMI_FILE_CREATING | SMI_FILE_ONLINE;

            // 서버 바이너리 버전을 데이타파일헤더에 설정한다.
            sFileNode->mDBFileHdr.mSmVersion = smVersionID;

            //PROJ-2133 incremental backup 
            //change tracking파일에 데이터파일등록
            if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
            {
                IDE_TEST( smriChangeTrackingMgr::addDataFile2CTFile( 
                                                    aSpaceNode->mHeader.mID,
                                                    aFileAttr->mID,
                                                    SMRI_CT_DISK_TBS,
                                                    &sSlotID )
                          != IDE_SUCCESS );

                sDataFileDescState = ID_TRUE;
            }
            else
            {
                //nothing to do
            }

            // 최근 수행된 체크포인트정보를 얻어서
            // 데이타파일헤더에 설정한다.
            // DiskRedoLSN, DiskCreateLSN을
            // 데이타파일헤더에 설정한다.
            // CreateLSN은 SCT_UPDATE_DRDB_CREATE_DBF 로그의 LSN
            // 파일헤더에 설정한다.
            sddDataFile::setDBFHdr( &(sFileNode->mDBFileHdr),
                                    sctTableSpaceMgr::getDiskRedoLSN(),
                                    &sCreateLSN,
                                    &sMustRedoToLSN,
                                    sSlotID );

            //PRJ-1149 , 파일 생성 시점의 LSN기록을 한다.
            if(sFileNode->mCreateMode == SMI_DATAFILE_REUSE)
            {
                /* BUG-32950 when creating tablespace with reuse clause,
                 *           can not check the same file.
                 * 이미 동일 이름을 갖는 데이터 파일을 추가했는지
                 * 확인한다.
                 * 0번째 부터 i - 1 번째까지 i와 비교한다. */
                for ( j = 0; j < i; j++ )
                {
                    IDE_TEST_RAISE(
                        idlOS::strcmp( sFileNodeArray[j]->mName,
                                       sFileNodeArray[i]->mName ) == 0,
                        error_already_exist_datafile );
                }

                IDE_TEST( sddDataFile::reuse( aStatistics,
                                              sFileNode )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( sddDataFile::create( aStatistics,
                                               sFileNode )
                          != IDE_SUCCESS );
                sFileArray[i] = ID_TRUE;
            }
            break;
        default :
            IDE_ASSERT(0);
            break;
    }
    /* ====================================
     * [5] 데이타 화일 노드 리스트에 연결
     * ==================================== */
    addDataFileNode(aSpaceNode, sFileNode);

    /*BUG-16197: Memory가 샘*/
    sState = 1;
    IDE_TEST( iduMemMgr::free( sFileArray ) != IDE_SUCCESS );
    sFileArray = NULL;

    sState = 0;
    IDE_TEST( iduMemMgr::free( sFileNodeArray ) != IDE_SUCCESS );
    sFileNodeArray = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_exist_datafile );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AlreadyExistFile, sFileNode->mName));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch (sState)
        {
            case 2:
                if( sFileNodeArray[0] != NULL )
                {
                    if( (sFileArray != NULL) && (sFileArray[0] == ID_TRUE) )
                    {
                        /*
                         * BUG-26215 
                         *  [SD]create tablespace시 마지막 파일이 autoextend on으로
                         *  설정되고 크기를 property의 max값보다 크게 입력하면
                         *  윈도우에서 비정상 종료할수 있음.
                         */
                        if( idf::access( sFileNodeArray[0]->mName, F_OK ) == 0 )
                        {
                            (void)idf::unlink(sFileNodeArray[0]->mName);
                        }
                    }

                    /* BUG-18176
                       위에서 생성한 DataFileNode에 대해
                       sddDataFile::initialize()에서 할당한 자원을 해제해줘야 한다.
                       i는 할당한 횟수이기 때문에
                       i보다 작을 땐 무조건 해제해야 하고
                       i번째에 대해서는 sCurFileNodeState로 할당 유무를 판단한다. */
                    if (0 < i)
                    {
                        IDE_ASSERT( sddDataFile::destroy( sFileNodeArray[0] ) == IDE_SUCCESS );
                        IDE_ASSERT( iduMemMgr::free( sFileNodeArray[0] ) == IDE_SUCCESS );
                    }
                    else if (0 == i)
                    {
                        switch (sCurFileNodeState)
                        {
                            case 2:
                                IDE_ASSERT( sddDataFile::destroy( sFileNodeArray[0] ) == IDE_SUCCESS );
                            case 1:
                                IDE_ASSERT( iduMemMgr::free( sFileNodeArray[0] ) == IDE_SUCCESS );
                            case 0:
                                break;
                            default:
                                IDE_ASSERT(0);
                                break;
                        }
                    }
                    else
                    {
                        /* 0 가 i보다 크다면 더 루프를 돌 필요가 없다. */
                        break;
                    }
                }
                IDE_ASSERT( iduMemMgr::free(sFileArray) == IDE_SUCCESS );
            case 1:
                IDE_ASSERT( iduMemMgr::free(sFileNodeArray) == IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    IDE_POP();

    if( sDataFileDescState == ID_TRUE )
    {
        IDE_ASSERT( smriChangeTrackingMgr::deleteDataFileFromCTFile( sSlotID ) 
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : datafile 연결 리스트에 노드 추가
 *
 * tablespace 노드에서 유지하는 datafile 연결 리스트에
 * 마지막에 datafile 노드을 추가한다.
 ***********************************************************************/
void sddTableSpace::addDataFileNode( sddTableSpaceNode*  aSpaceNode,
                                     sddDataFileNode*    aFileNode )
{
    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aFileNode != NULL );

    aSpaceNode->mFileNodeArr[ aFileNode->mID ] = aFileNode ;
}



void sddTableSpace::removeMarkDataFileNode( sddDataFileNode * aFileNode )
{
    IDE_ASSERT( aFileNode != NULL );

    aFileNode->mState |= SMI_FILE_DROPPING;

    return;
}

/***********************************************************************
 * Description : 모드에 따라 하나의 datafile 노드 및 datafile을 제거
 *
 * + 2nd. code design
 *   - if ( SMI_ALL_TOUCH  이면 )
 *     {
 *         datafile를 삭제한다. -> sddDataFile::delete
 *     }
 *   - touch 모드에 따라 datafile 노드를 datafile 연결리스트에서 제거
 *     -> removeDataFileNode
 *   - datafile 노드를 해제한다.
 *
 * + 데이타 화일의 삭제는 not-used 상태의 화일에 대해서만 가능하므로
 *   (1) 삭제 대상이 되는 화일은 open되어 있지 않고
 *   (2) 버퍼에 fix된 페이지도 없음을 보장한다.
 ***********************************************************************/
IDE_RC sddTableSpace::removeDataFile( idvSQL              * aStatistics,
                                      void                * aTrans,
                                      sddTableSpaceNode   * aSpaceNode,
                                      sddDataFileNode     * aFileNode,
                                      smiTouchMode          aTouchMode,
                                      idBool                aDoGhostMark )
{
    sctPendingOp      * sPendingOp;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aFileNode  != NULL );
    IDE_DASSERT( aTouchMode == SMI_ALL_NOTOUCH ||
                 aTouchMode == SMI_ALL_TOUCH   ||
                 aTouchMode == SMI_EACH_BYMODE );

    /* ====================================
     * [1] 데이타 화일 삭제에 관한 로깅
     *     서버 종료시에는 로깅하지 않음
     * ==================================== */
    if ( aDoGhostMark == ID_TRUE )
    {
        IDE_ASSERT( aTrans != NULL );

        IDE_TEST( smLayerCallback::writeLogDropDBF( aStatistics,
                                                    aTrans,
                                                    aSpaceNode->mHeader.mID,
                                                    aFileNode,
                                                    aTouchMode,
                                                    NULL ) 
                   != IDE_SUCCESS );
  

        /* ====================================
         * [2] 데이타 화일 리스트에서 논리적으로 삭제
         * ==================================== */
        IDE_TEST( sddDataFile::addPendingOperation( aTrans,
                                                    aFileNode,
                                                    ID_TRUE, /* commit시 동작 */
                                                    SCT_POP_DROP_DBF,
                                                    &sPendingOp )
                  != IDE_SUCCESS );


        sPendingOp->mPendingOpFunc  = sddDiskMgr::removeFilePending;
        sPendingOp->mPendingOpParam = (void*)aFileNode;
        sPendingOp->mTouchMode      = aTouchMode;

        removeMarkDataFileNode( aFileNode );
    }
    else
    {
        IDE_TEST( sddDiskMgr::closeDataFile( aStatistics,
                                             aFileNode ) != IDE_SUCCESS );

        IDE_ASSERT( aTouchMode == SMI_ALL_NOTOUCH );
        IDE_ASSERT( sddDataFile::destroy(aFileNode) == IDE_SUCCESS );

        IDE_ASSERT( iduMemMgr::free(aFileNode) == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 모드에 따라 tablespace 노드의 모든
 *               datafile 노드 및 datafile을 제거
 *
 * + 2nd. code design
 *   - while(모든 datafile 리스트에 대해)
 *     {
 *        if ( SMI_ALL_TOUCH 이면 )
 *        {
 *            datafile를 삭제한다. -> sddDataFile::delete
 *        }
 *        datafile 노드를 datafile 연결리스트에서 제거 -> removeDataFileNode
 *        datafile 노드를 해제한다.
 *        datafile 노드의 메모리를 해제한다.
 *     }
 ***********************************************************************/
IDE_RC sddTableSpace::removeAllDataFiles( idvSQL*             aStatistics,
                                          void*               aTrans,
                                          sddTableSpaceNode*  aSpaceNode,
                                          smiTouchMode        aTouchMode,
                                          idBool              aDoGhostMark)
{
    sddDataFileNode *sFileNode;
    UInt i;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aTouchMode == SMI_ALL_NOTOUCH ||
                 aTouchMode == SMI_ALL_TOUCH   ||
                 aTouchMode == SMI_EACH_BYMODE );

    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL)
        {
            continue;
        }

        if( aDoGhostMark == ID_TRUE )
        {
            if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
            {
                continue;
            }
        }
        else // shutdown
        {
            IDE_DASSERT( aTrans == NULL );
        }

        IDE_TEST( removeDataFile(aStatistics,
                                 aTrans,
                                 aSpaceNode,
                                 sFileNode,
                                 aTouchMode,
                                 aDoGhostMark ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 * Description : datafile 노드의 remove 가능여부 확인
 *
 * tablespace의 removeDataFile에서 호출되는 함수로써 제거하고자 하는 datafile
 * 노드의  on/offline 여부와 상관없이 usedpagelimit을 포함한 노드의 이후
 * 노드라면, 제거가 가능하다.
 ***********************************************************************/
IDE_RC sddTableSpace::canRemoveDataFileNodeByName(
                                        sddTableSpaceNode* aSpaceNode,
                                        SChar*             aDataFileName,
                                        scPageID           aUsedPageLimit,
                                        sddDataFileNode**  aFileNode )
{

    idBool           sFound;
    sddDataFileNode* sFileNode;
    UInt i ;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aDataFileName != NULL );
    IDE_DASSERT( aFileNode != NULL );

    sFound     = ID_FALSE;
    *aFileNode = NULL;

    for (i = 0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL)
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ))
        {
            continue;
        }

        if (idlOS::strcmp(sFileNode->mName, aDataFileName) == 0)
        {
            sFound = ID_TRUE;

            /* ------------------------------------------------
             * aUsedPageLimit을 포함한 datafile 노드 이후의
             * datafile 노드인 경우는 성공, 그렇지 않다면 NULL 반환
             * ----------------------------------------------*/
            if ( sFileNode->mID > SD_MAKE_FID( aUsedPageLimit ))
            {
                *aFileNode = sFileNode;
            }
            else
            {
                *aFileNode = NULL;
            }
            break;
        }
    }

     /* 주어진 datafile 이름이 없음 */
    IDE_TEST_RAISE( sFound != ID_TRUE, error_not_found_filenode );
    IDE_TEST_RAISE( *aFileNode == NULL, error_cannot_remove_filenode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNode,
                                aDataFileName));
    }
    IDE_EXCEPTION( error_cannot_remove_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CannotRemoveDataFileNode,
                                aDataFileName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/*
 해당 페이지를 포함하는 datafile 노드 반환

 [IN]  aSpaceNode  - 테이블스페이스 노드
 [IN]  aPageID     - 검색할 페이지ID
 [OUT] aFileNode   - 페이지를 포함하는 데이타파일 노드 
                     유효하지 않은 경우 NULL 반환
 [IN]  aFatal      - 에러타입 : FATAL 또는 ABORT
*/
IDE_RC sddTableSpace::getDataFileNodeByPageID(
                                        sddTableSpaceNode  * aSpaceNode,
                                        scPageID             aPageID,
                                        sddDataFileNode   ** aFileNode,
                                        idBool               aFatal )
{
    idBool           sFound;
    sddDataFileNode* sFileNode;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aFileNode != NULL );

    sFileNode = (sddDataFileNode*)aSpaceNode->mFileNodeArr[ SD_MAKE_FID(aPageID)];

    if( sFileNode != NULL )
    {
        if ( ( sFileNode->mCurrSize > SD_MAKE_FPID( aPageID ) ) &&
             SMI_FILE_STATE_IS_NOT_DROPPED( sFileNode->mState ) )
        {
            *aFileNode = sFileNode;
            sFound     = ID_TRUE;
        }
        else
        {
            *aFileNode = NULL;
            sFound     = ID_FALSE;
        }
    }
    else
    {
        *aFileNode = NULL;
        sFound     = ID_FALSE;
    }

    if( aFatal == ID_TRUE )
    {
        IDE_TEST_RAISE( sFound != ID_TRUE, error_fatal_not_found_filenode );
    }
    else
    {
        IDU_FIT_POINT_RAISE( "BUG-49107@sddTableSpace::getDataFileNodeByPageID::NotFound",error_abort_not_found_filenode );
        IDE_TEST_RAISE( sFound != ID_TRUE, error_abort_not_found_filenode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_fatal_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_NotFoundDataFile,
                                aPageID,
                                aSpaceNode->mHeader.mID,
                                aSpaceNode->mHeader.mType));
    }
    IDE_EXCEPTION( error_abort_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFile,
                                aPageID,
                                aSpaceNode->mHeader.mID,
                                aSpaceNode->mHeader.mType));
    }
    IDE_EXCEPTION_END;

    *aFileNode = NULL;

    return IDE_FAILURE;
}

/*
 검색할 페이지를 포함하는 datafile 노드 반환(No에러버전)

 [IN]  aSpaceNode  - 테이블스페이스 노드
 [IN]  aPageID     - 검색할 페이지ID
 [OUT] aFileNode   - 페이지를 포함하는 데이타파일 노드
                     유효하지 않은 경우 NULL 반환
*/
void sddTableSpace::getDataFileNodeByPageIDWithoutException(
                                     sddTableSpaceNode* aSpaceNode,
                                     scPageID           aPageID,
                                     sddDataFileNode**  aFileNode)
{
    sddDataFileNode* sFileNode;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aFileNode != NULL);

    sFileNode=
       (sddDataFileNode*)aSpaceNode->mFileNodeArr[ SD_MAKE_FID(aPageID)];

    /* BUG-35190 - [SM] during recover dwfile in media recovery phase,
     *             it can access a page in area that is not expanded.
     *
     * 미디어리커버리 과정중 DW file 복구를 시도할때,
     * DW file이 백업 시점보다 최신인 경우 datafile에 없는 페이지에 접근할 수 있다.
     * 이 경우 없어도 문제가 되지 않고, 실제로도 datafile 노드에 페이지가 존재하지 않으므로
     * NULL을 리턴해준다. */
    if( sFileNode != NULL )
    {
        if ( ( sFileNode->mCurrSize > SD_MAKE_FPID( aPageID ) ) &&
             SMI_FILE_STATE_IS_NOT_DROPPED( sFileNode->mState ) )
        {
            *aFileNode = sFileNode;
        }
        else
        {
            *aFileNode = NULL;
        }
    }
    else
    {
        *aFileNode = NULL;
    }

    return;
}

/***********************************************************************
 * Description : 해당 파일 ID를 가지는 datafile 노드 반환
 *
 * tablespace 노드의 datafile 노드 연결 리스트에서 주어진 파일ID에
 * 해당하는 datafile 노드를 반환한다.
 ***********************************************************************/
IDE_RC sddTableSpace::getDataFileNodeByID( sddTableSpaceNode * aSpaceNode,
                                           UInt                aID,
                                           sddDataFileNode  ** aFileNode )
{

    sddDataFileNode* sFileNode;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aFileNode != NULL );

    sFileNode = (sddDataFileNode*)aSpaceNode->mFileNodeArr[ aID ];

    if( ( sFileNode == NULL ) ||
        ( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) ) ||
        ( aSpaceNode->mNewFileID <= aID ) )
    {
        IDE_RAISE( error_not_found_filenode );
    }

    *aFileNode = sFileNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNodeByID,
                                aID));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
  PRJ-1548 User Memory Tablespace
  데이타파일이 존재하지 않으면 Null을 반환한다

  sctTableSpaceMgr::lock()을 획득하고 호출되어야 한다.
*/
void sddTableSpace::getDataFileNodeByIDWithoutException( 
                                                 sddTableSpaceNode  * aSpaceNode,
                                                 UInt                aFileID,
                                                 sddDataFileNode  ** aFileNode)
{
    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aFileNode != NULL );

    *aFileNode = (sddDataFileNode*)aSpaceNode->mFileNodeArr[ aFileID];

    return;
}

/***********************************************************************
 * Description : 해당 파일명을 가지는 datafile 노드 반환
 *
 * tablespace 노드의 datafile 노드 연결 리스트에서 해당 파일명에
 * 해당하는 datafile 노드를 반환한다.
 ***********************************************************************/
IDE_RC sddTableSpace::getDataFileNodeByName( sddTableSpaceNode*  aSpaceNode,
                                             SChar*              aFileName,
                                             sddDataFileNode**   aFileNode)
{
    sddDataFileNode * sFileNode;
    UInt              i;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aFileName != NULL );
    IDE_DASSERT( aFileNode != NULL );

    *aFileNode = NULL;

    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
        {
            continue;
        }

        if (idlOS::strcmp(sFileNode->mName, aFileName) == 0)
        {
            *aFileNode = sFileNode;
            break; // found
        }
    }

    IDE_TEST_RAISE( *aFileNode == NULL, error_not_found_filenode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNode,
                                aFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * aCurFileID:현재까지 검색한 File ID
 *             (이함수는 이 파일아이디다음의 "유효한"File Node를 알아낸다.)
***********************************************************************/
void sddTableSpace::getNextFileNode( sddTableSpaceNode* aSpaceNode,
                                     sdFileID           aCurFileID,
                                     sddDataFileNode**  aFileNode )
{
    UInt              i;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aFileNode != NULL );
    IDE_ASSERT( aCurFileID < SD_MAX_FID_COUNT );

    *aFileNode = NULL ;

    for (i = (aCurFileID+1) ; i < aSpaceNode->mNewFileID ; i++ )
    {
        if( aSpaceNode->mFileNodeArr[i] != NULL )
        {
            if( SMI_FILE_STATE_IS_NOT_DROPPED( aSpaceNode->mFileNodeArr[i]->mState ) )
            {
                 *aFileNode = aSpaceNode->mFileNodeArr[i] ;
                 break;
            }
        }
    }

    return;
}

/***********************************************************************
 * Description : autoextend 모드를 설정할 datafile 노드를 찾는다.
 *
 * 주어진 autoextend 속성을 설정할 수 있는 datafile 노드를
 * 검색한다.
 * - 설정하고자하는 autoextend 속성이 ON인경우
 *   datafile 이름을 가진 datafile 노드를 검색한 다음,
 *   해당 노드의 aUsedPageLimit를 포함하지 않는 이전 노드라면 [NULL]
 *   포함하면 [해당 datafile 노드] 이후 노드라면 [해당 datafile 노드]
 *   를 반환한다.
 *   !! autoextend 속성은 tablespace 중에 1개이하의 datafile 노드
 *   에만 설정이 가능하다.
 * - 설정하고자하는 autoextend 속성이 OFF인경우
 *   datafile 이름을 가진 datafile 노드를 검색해서 반환한다.
 *
 ***********************************************************************/
IDE_RC sddTableSpace::getDataFileNodeByAutoExtendMode(
                         sddTableSpaceNode* aSpaceNode,
                         SChar*             aDataFileName,
                         idBool             aAutoExtendMode,
                         scPageID           aUsedPageLimit,
                         sddDataFileNode**  aFileNode )
{

    UInt             i;
    idBool           sFound;
    sddDataFileNode* sFileNode;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aDataFileName != NULL );
    IDE_DASSERT( aFileNode != NULL );

    sFound         = ID_FALSE;
    *aFileNode = NULL;

    for (i = 0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState) ||
            SMI_FILE_STATE_IS_CREATING( sFileNode->mState ) )
        {
            continue;
        }

        if (idlOS::strcmp(sFileNode->mName, aDataFileName) == 0)
        {
            if (aAutoExtendMode == ID_TRUE)
            {
                /* ------------------------------------------------
                 * aUsedPageLimit을 포함한 datafile 노드 이거나 그 이후의
                 * datafile 노드인 경우는 성공
                 * 그렇지 않다면 NULL 반환
                 * ----------------------------------------------*/
                if (sFileNode->mID >= SD_MAKE_FID(aUsedPageLimit) )
                {
                    sFound = ID_TRUE;
                    *aFileNode = sFileNode;
                }
                else
                {
                    /* Fix BUG-16962. */
                    IDE_RAISE( error_used_up_filenode );
                }
            }
            else
            {
                sFound = ID_TRUE;
                *aFileNode = sFileNode;
            }
            break;
        }
    }

     /* 주어진 datafile 이름이 없음 */
    IDE_TEST_RAISE( sFound != ID_TRUE, error_not_found_filenode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_used_up_filenode );
    {
        IDE_SET(
             ideSetErrorCode(smERR_ABORT_AUTOEXT_ON_UNALLOWED_FOR_USED_UP_FILE,
                             aDataFileName)
               );
    }
    IDE_EXCEPTION( error_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNode,
                                aDataFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : tablespace 노드의 정보를 출력
 ***********************************************************************/
IDE_RC sddTableSpace::getPageRangeByName( sddTableSpaceNode* aSpaceNode,
                                          SChar*             aDataFileName,
                                          sddDataFileNode**  aFileNode,
                                          scPageID*          aFstPageID,
                                          scPageID*          aLstPageID )
{
    idBool           sFound;
    sddDataFileNode* sFileNode;
    UInt i;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aDataFileName != NULL );
    IDE_DASSERT( aFileNode != NULL );

    sFound         = ID_FALSE;
    *aFileNode = NULL;
    *aFstPageID    = 0;
    *aLstPageID    = 0;

    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
        {
            continue;
        }

        if (idlOS::strcmp(sFileNode->mName, aDataFileName) == 0)
        {
            sFound         = ID_TRUE;
            *aFileNode = sFileNode;
            *aLstPageID    = *aFstPageID + (sFileNode->mCurrSize - 1);
            break; // found
        }

        *aFstPageID += sFileNode->mCurrSize;
    }

    IDE_TEST_RAISE( sFound != ID_TRUE, error_not_found_filenode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNode,
                                aDataFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
    데이타파일노드를 검색하여 페이지 구간을 얻는다
*/
IDE_RC sddTableSpace::getPageRangeInFileByID(
                            sddTableSpaceNode * aSpaceNode,
                            UInt                aFileID,
                            scPageID          * aFstPageID,
                            scPageID          * aLstPageID )
{
    sddDataFileNode* sFileNode = NULL;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aFstPageID != NULL );
    IDE_DASSERT( aLstPageID != NULL );

    sFileNode = aSpaceNode->mFileNodeArr[ aFileID ] ;

    IDE_TEST_RAISE( sFileNode == NULL, error_not_found_filenode );

    *aFstPageID = SD_CREATE_PID( aFileID,0);

    *aLstPageID = SD_CREATE_PID( aFileID, sFileNode->mCurrSize -1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNodeByID,
                                aFileID));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  PRJ-1548 User Memory Tablespace

  기능 : 테이블스페이스 노드에 저장된 총 페이지 개수 반환

   sctTableSpaceMgr::lock()을 획득한 상태로 호출되어야 한다.
*/
ULong sddTableSpace::getTotalPageCount( sddTableSpaceNode* aSpaceNode )
{
    IDE_DASSERT( aSpaceNode != NULL );

    return aSpaceNode->mTotalPageCount;
}

/*
  PRJ-1548 User Memory Tablespace

   기능 : 테이블스페이스 노드에 저장된 총 DBF 개수 반환

   sctTableSpaceMgr::lock()을 획득한 상태로 호출되어야 한다.
*/
UInt sddTableSpace::getTotalFileCount( sddTableSpaceNode* aSpaceNode )
{
    IDE_DASSERT( aSpaceNode != NULL );

    return aSpaceNode->mDataFileCount;
}


/*
   기능 : tablespace 속성을 반환한다.
*/
void sddTableSpace::getTableSpaceAttr(sddTableSpaceNode* aSpaceNode,
                                      smiTableSpaceAttr* aSpaceAttr)
{

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aSpaceAttr != NULL );

    aSpaceAttr->mAttrType  = SMI_TBS_ATTR;
    idlOS::strcpy(aSpaceAttr->mName, aSpaceNode->mHeader.mName);
    aSpaceAttr->mNameLength = idlOS::strlen(aSpaceNode->mHeader.mName);

    aSpaceAttr->mID                        = aSpaceNode->mHeader.mID;
    aSpaceAttr->mDiskAttr.mNewFileID       = aSpaceNode->mNewFileID;
    aSpaceAttr->mDiskAttr.mExtMgmtType     = aSpaceNode->mExtMgmtType;
    aSpaceAttr->mDiskAttr.mSegMgmtType     = aSpaceNode->mSegMgmtType;
    aSpaceAttr->mType                      = aSpaceNode->mHeader.mType;
    aSpaceAttr->mTBSStateOnLA              = aSpaceNode->mHeader.mState;
    aSpaceAttr->mDiskAttr.mExtPageCount    = aSpaceNode->mExtPageCount;
    aSpaceAttr->mAttrFlag                  = aSpaceNode->mAttrFlag;

    return;

}

/*
   기능 : tablespace 정보를 출력한다.
*/
IDE_RC sddTableSpace::dumpTableSpaceNode( sddTableSpaceNode* aSpaceNode )
{
    SChar  sTbsStateBuff[500 + 1];

    IDE_ERROR( aSpaceNode != NULL );

    idlOS::printf("[TBS-BEGIN]\n");
    idlOS::printf(" ID\t: %"ID_UINT32_FMT"\n", aSpaceNode->mHeader.mID );
    idlOS::printf(" Type\t: %"ID_UINT32_FMT"\n", aSpaceNode->mHeader.mType );
    idlOS::printf(" Name\t: %s\n", aSpaceNode->mHeader.mName );

    // klocwork SM
    idlOS::memset( sTbsStateBuff, 0x00, 500 + 1 );

    if( SMI_TBS_IS_OFFLINE(aSpaceNode->mHeader.mState))
    {
        idlOS::strncat(sTbsStateBuff,"OFFLINE | ", 500);
    }
    if( SMI_TBS_IS_ONLINE(aSpaceNode->mHeader.mState) )
    {
        idlOS::strncat(sTbsStateBuff,"ONLINE | ", 500);
    }
    if( SMI_TBS_IS_CREATING(aSpaceNode->mHeader.mState) )
    {
        idlOS::strncat(sTbsStateBuff,"CREATING | ", 500);
    }
    if( SMI_TBS_IS_DROPPING(aSpaceNode->mHeader.mState) )
    {
        idlOS::strncat(sTbsStateBuff,"DROPPING | ", 500);
    }
    if( SMI_TBS_IS_DROPPED(aSpaceNode->mHeader.mState) )
    {
        idlOS::strncat(sTbsStateBuff,"DROPPED | ", 500);
    }
    if( SMI_TBS_IS_BACKUP(aSpaceNode->mHeader.mState) )
    {
        idlOS::strncat(sTbsStateBuff,"BACKUP | ", 500);
    }

    sTbsStateBuff[idlOS::strlen(sTbsStateBuff) - 2] = 0;

    idlOS::printf(" STATE\t: %s\n", sTbsStateBuff);
    idlOS::printf(" Datafile Node Count : %"ID_UINT32_FMT"\n",
                  aSpaceNode->mDataFileCount );
    idlOS::printf(" Total Page Count : %"ID_UINT64_FMT"\n",
                  aSpaceNode->mTotalPageCount );
    idlOS::printf("[TBS-BEGIN]\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#if 0 //not used
/*
  기능 : tablespace 노드의 datafile 노드 리스트를 출력
*/
void sddTableSpace::dumpDataFileList( sddTableSpaceNode* aSpaceNode )
{

    UInt i;
    sddDataFileNode*     sFileNode;

    IDE_DASSERT ( aSpaceNode != NULL );

    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL )
        {
            continue;
        }

        (void)sddDataFile::dumpDataFileNode( sFileNode );
    }

}
#endif
/*
  기능 : 새로운 DBF ID를 반환하고 +1 증가시킨다.
*/
sdFileID sddTableSpace::getNewFileID( sddTableSpaceNode * aSpaceNode )
{
    IDE_DASSERT( aSpaceNode != NULL );

    return (aSpaceNode->mNewFileID)++;
}


/*
  PRJ-1548 User Memory Tablespace

  서버구동시 복구이후에 테이블스페이스의
  DataFileCount와 TotalPageCount를 계산하여 설정한다.

  본 프로젝트부터 sddTableSpaceNode의 mDataFileCount와
  mTotalPageCount는 RUNTIME 정보로 취급하므로
  서버구동시 복구이후에 정보를 보정해주어야 한다.
*/

void sddTableSpace::calculateFileSizeOfTBS( sddTableSpaceNode * aSpaceNode )
{
    UInt                 sFileCount = 0;
    ULong                sTotalPageCount = 0;
    sddDataFileNode*     sFileNode;
    UInt                 i;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE );

    for ( i = 0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL )
        {
            continue;
        }

        IDE_DASSERT( SMI_FILE_STATE_IS_NOT_CREATING( sFileNode->mState ) );
        IDE_DASSERT( SMI_FILE_STATE_IS_NOT_DROPPING( sFileNode->mState ) );
        IDE_DASSERT( SMI_FILE_STATE_IS_NOT_RESIZING( sFileNode->mState ) );

        if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
        {
            continue;
        }

        sFileCount++;
        sTotalPageCount += sFileNode->mCurrSize;
    }

    aSpaceNode->mDataFileCount = sFileCount;
    aSpaceNode->mTotalPageCount = sTotalPageCount;
}

/*
  PRJ-1548 User Memory Tablespace

  테이블스페이스의 상태를 설정하고 백업을 수행한다.
  CREATING 중이거나 DROPPING 중인 경우 TBS Mgr Latch를 풀고
  잠시 대기하다가 Latch를 다시 시도한 다음 다시 시도한다.

  본 함수에 호출되기전에 TBS Mgr Latch는 획득된 상태이다.

  [IN] aSpaceNode : 백업할 TBS Node
  [IN] aActionArg : 백업에 필요한 인자
*/

IDE_RC sddTableSpace::doActOnlineBackup( idvSQL            * aStatistics,
                                         sctTableSpaceNode * aSpaceNode,
                                         void              * aActionArg )
{
    sctActBackupArgs * sActBackupArgs  = (sctActBackupArgs*)aActionArg;
    UInt               sState = 0;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aActionArg != NULL );

    if ( (sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE) &&
         (sctTableSpaceMgr::isTempTableSpace( aSpaceNode ) != ID_TRUE) )
    {
        if ( ( (aSpaceNode->mState & SMI_TBS_DROPPED)   != SMI_TBS_DROPPED ) &&
             ( (aSpaceNode->mState & SMI_TBS_DISCARDED) != SMI_TBS_DISCARDED ) )
        {
            /* ------------------------------------------------
             * disk table space backup 상태 변경및
             * 첫번째 데이타파일의 상태를 backup으로 변경한다.
             * ----------------------------------------------*/
            IDE_TEST( sctTableSpaceMgr::startTableSpaceBackup(
                          aStatistics,
                          aSpaceNode ) != IDE_SUCCESS);
            sState = 1;

            // Backup Flag를 설정하고 한번 더 확인한다.
            // Discard는 될 수 없어도 Drop은 될 수도 있다.
            if ( (aSpaceNode->mState & SMI_TBS_DROPPED) != SMI_TBS_DROPPED )
            {
                IDE_DASSERT( SMI_TBS_IS_ONLINE(aSpaceNode->mState) );

                if ( sActBackupArgs->mIsIncrementalBackup == ID_TRUE )
                {
                    IDE_TEST( smLayerCallback::incrementalBackupDiskTBS(
                                  aStatistics,
                                  (sddTableSpaceNode*)aSpaceNode,
                                  sActBackupArgs->mBackupDir,
                                  sActBackupArgs->mCommonBackupInfo )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( smLayerCallback::backupDiskTBS(
                                  aStatistics,
                                  (sddTableSpaceNode*)aSpaceNode,
                                  sActBackupArgs->mBackupDir )
                              != IDE_SUCCESS );
                }
            }

            /* ------------------------------------------------
             * 테이블 스페이스 의 상태를 backup에서 online으로 변경한다.
             * ----------------------------------------------*/
            sState = 0;
            IDE_TEST( sctTableSpaceMgr::endTableSpaceBackup( aStatistics,
                                                             aSpaceNode ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        sctTableSpaceMgr::endTableSpaceBackup( aStatistics,
                                               aSpaceNode ) ;
    }

    return IDE_FAILURE;
}

/*
  테이블스페이스의 Dirty된 데이타파일을 Sync한다.
  본 함수에 호출되기전에 TBS Mgr Latch는 획득된 상태이다.

  [IN] aSpaceNode : Sync할 TBS Node
  [IN] aActionArg : NULL
*/
IDE_RC sddTableSpace::doActSyncTBSInNormal( idvSQL            * aStatistics,
                                            sctTableSpaceNode * aSpaceNode,
                                            void              * /*aActionArg*/ )
{
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    UInt                i;

    IDE_DASSERT( aSpaceNode != NULL );

    while( 1 )
    {
        if( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) != ID_TRUE )
        {
            // Disk Table이 아닌경우 무시한다.
            break;
        }

        if( sctTableSpaceMgr::hasState( aSpaceNode,
                                        SCT_SS_SKIP_SYNC_DISK_TBS )
            == ID_TRUE )
        {
            // 테이블스페이스가 DROPPED/DISCARDED 경우 무시한다.

            // DROP된 TBS의 경우 DROP 과정에서 Buffer의 Page들을
            // 모두 Invalid 시키고 나서 DROPPED 상태로 변경되기
            // 때문에 그 이후에 TBS와 관련된 Page가 Dirty 될
            // 일이 없다.
            // DISCARDED TBS는 Startup Control 단계에서 설정되므로
            // Dirty Page가 발생할 수 없다.
            break;
        }

        sSpaceNode = (sddTableSpaceNode*)aSpaceNode;

        // PRJ-1548 User Memory Tablespace
        // 테이블스페이스가 존재하지 않거나, DROPPED 상태의 경우에
        // sSpaceNode는 NULL이 반환된다.
        //
        // A. 트랜잭션 Pending 연산에 의해서 TBS Node가 DROPPED 상태가 되었다면,
        //    SYNC를 무시하도록 처리한다.
        //   테이블스페이스 노드가 삭제되더라도 바로 free되지 않기 때문이다.
        //
        // B. 존재하지 않는 테이블스페이스의 경우도 그냥 무시한다.

        for ( i=0; i < sSpaceNode->mNewFileID ; i++ )
        {
            sFileNode = sSpaceNode->mFileNodeArr[i] ;

            if( sFileNode == NULL )
            {
                continue;
            }

            // PRJ-1548 User Memory Tablespace
            // 트랜잭션 Pending 연산에 의해서 DBF Node가 DROPPED 상태가
            // 되었다면, SYNC 연산에서 볼수 없다. 상태변경을 sctTableSpaceMgr::lock()
            // 으로 보호한다.

            if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
            {
                continue;
            }

            sddDataFile::lockFileNode( aStatistics,
                                       sFileNode );

            if ( (sFileNode->mIsOpened == ID_TRUE) &&
                 (sFileNode->mIsModified == ID_TRUE) )
            {
                // checkpoint sync는 prepareIO/completeIO로 수행되므로
                // checkpoint sync로 인해서 진입할 수 있다.

                sFileNode->mIsModified = ID_FALSE ;

                /* BUG-24558: [SD] DBF Sync와 Close가 동시에 수행됨
                 *
                 * DBF가 Sync중인지를 idBool로 처리했으나 동시에 Sync연산이
                 * 발생시 먼저 완료된 Sync연산이 Sync Flag를 ID_FALSE로
                 * 하여 두번째 Sync연산이 진행중이지만 File Open List에서
                 * Victim으로 선정되어 Close가 되는 문제가 있음.
                 * 하여 Sync가 여러번 동시에 요청이 될수 있기때문에 연산 시작시
                 * IOCount를 증가시키고 끝나면 감소시키게 하였음.
                 * Sync가 수행중인지는 IOCount 0보다 크면 연산진행중으로 판단 */
                sFileNode->mIOCount++;
                sddDataFile::unlockFileNode( sFileNode );

                /* prepareIO 를 하기전에 space lock을 먼저 unlock해버리면
                 * 그 사이에 file이 drop등이 될 수 있다.*/
                sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
                // PRJ-1548 SM - User Memory TableSpace 개념도입
                //
                // A. mIOCount가 0이 아닐 경우는 close될 수 없다.
                //    (참고 sddDiskMgr::findVictim)
                // B. sync 중일때 DROPPED 상태로 변경될수 없다.
                //    (참고 sctTableSpaceMgr::executePendingOp)
                IDE_ASSERT( sddDataFile::sync( sFileNode,
                                               smLayerCallback::setEmergency ) == IDE_SUCCESS );

                // 반드시 DiskMgr로 completeIO 하여야 Victim List에 연결 된다.
                sddDiskMgr::completeIO( aStatistics,
                                        sFileNode,
                                        SDD_IO_READ );
                /* completeIO 전에 Lock을 잡지 않아도 된다.
                 * prepareIO 는 전에 file 이 drop될 수도 있어서 잡고 있어야 하지만
                 * completeIO는 작업을 전부 이미 다 하였으므로 lock을 잡지 않고 있어도 된다.
                 * prepareIO와 completeIO에는 자체적은 file lock을 잡는다.*/
                sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                                 aSpaceNode );
            }
            else
            {
                sddDataFile::unlockFileNode( sFileNode );
                // DIRTY 파일 노드가 아닌경우
                // Nothing To Do..
            }
        }

        break;
    }

    return IDE_SUCCESS;
}
