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
 * $Id: sddDataFile.cpp 86490 2020-01-02 05:59:08Z et16 $
 *
 * Description :
 *
 * 본 파일은 디스크관리자의 datafile 노드에 대한 구현파일이다.
 *
 * # 개념
 *
 * 하나의 데이타베이스 화일에 대한 정보를 관리한다.
 *
 * # 데이타베이스 화일의 크기
 *     - 테이블 스페이스의 크기를 늘릴때, 2가지의 방법이 있다.
 *       이 방법은 사용자가 테이블스페이스를 생성할 때 지정가능하다.
 *
 *        1. 화일의 크기를 늘리기
 *           - AUTOEXTEND ON NEXT SIZE를
 *             포함하면 공간이 모자랄 때 SIZE 만큼 추가된다.
 *        2. 또 하나의 화일을 추가하기
 *           - AUTOEXTEND OFF를
 *             포함하면 자동으로 공간을 늘리지 않는다. 대신 사용자가
 *             명시적으로 ALTER TABLESPACE 구문을 이용하여 공간을 늘릴 수
 *             있다.
 *     - system tablespace의 데이타화일이 지정된 크기를 넘을때 그 화일의
 *       크기는 더 이상 증가할 수 없다. 일부 시스템에서는 화일의 크기가
 *       특정 크기(예 2G) 이상이면, 성능이 급격히 떨어질 수 있다.
 *       따라서 system tablespace의 경우 이 크기 이상으로 화일을 늘리지
 *       않고, 새로운 화일을 추가한다.
 *     - 이 값은 altibase cofig file에서 설정 가능하다.
 *     - 사용자의 tablespace의 데이타화일에 대해서는 이러한 제한을 두지 않으며,
 *       이런 현상이 있을 경우 사용자가 처리해야 한다.
 *     - 4.1.1.1 Tablespace User Interface 문서를 참조
 *
 * # 데이타베이스 화일의 Naming
 *     - 4.1.1.1 Tablespace User Interface 문서를 참조
 *     - Databae의 데이타화일 기본 저장 디렉토리 안에
 *       "system, "undo", "temp"로 시작하는 화일 이름이 올 수 없다.
 *
 * # 디렉토리
 *     - 데이타 화일이 저장되는 디렉토리는 config 화일에 지정된다.
 *     - SQL 문에서 데이타 화일은 이름은 상대경로 혹은 절대 경로로
 *       지정될 수 있다.
 *     - 상대경로로 지정된 후, 절대경로로 modify를 시도했을 때,
 *       이를 verify 할 수 있어야 한다. 이를 위해 모든 경로를
 *       절대 경로로 변경할 것이다.
 *     - 데이타화일의 이름은 경로+이름이다.
 *     - 특수문자가 들어갔는 지 확인하는 과정이 필요하다.
 *     - 이에 대한 코드가 sddDiskMgr::makeValidDataFilePath에서 구현되어야
 *       할 것임.
 **********************************************************************/

#include <idu.h>
#include <idl.h>
#include <ide.h>
#include <smu.h>
#include <smErrorCode.h>
#include <sddReq.h>
#include <sddDiskMgr.h>
#include <sddDataFile.h>
#include <sctTableSpaceMgr.h>
#include <smriChangeTrackingMgr.h>

/***********************************************************************
 * Description : datafile 노드를 초기화한다.
 **********************************************************************/
IDE_RC sddDataFile::initialize(scSpaceID         aTableSpaceID,
                               sddDataFileNode*  aDataFileNode,
                               smiDataFileAttr*  aDataFileAttr,
                               UInt              aMaxDataFilePageCount,
                               idBool            aCheckPathValidate)
{
    SChar   sMutexName[128];
    UInt    sState = 0;

    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( aDataFileAttr != NULL );
    IDE_DASSERT( aMaxDataFilePageCount > 0 );
    IDE_DASSERT( aDataFileAttr->mAttrType == SMI_DBF_ATTR );

    aDataFileNode->mSpaceID      = aTableSpaceID;
    aDataFileNode->mID           = aDataFileAttr->mID;
    aDataFileNode->mState        = aDataFileAttr->mState;

    /* datafile 노드의 여러가지 속성 설정 */
    aDataFileNode->mIsAutoExtend = aDataFileAttr->mIsAutoExtend;
    aDataFileNode->mCreateMode   = aDataFileAttr->mCreateMode;
    aDataFileNode->mInitSize     = aDataFileAttr->mInitSize;
    aDataFileNode->mCurrSize     = aDataFileAttr->mCurrSize;
    aDataFileNode->mIsOpened     = ID_FALSE;
    aDataFileNode->mIOCount      = 0;
    aDataFileNode->mIsModified   = ID_FALSE;

    IDE_TEST( iduFile::allocBuff4DirectIO(
                                  IDU_MEM_SM_SMM,
                                  SM_PAGE_SIZE,
                                  (void**)&( aDataFileNode->mPageBuffPtr ),
                                  (void**)&( aDataFileNode->mAlignedPageBuff) )
             != IDE_SUCCESS);
    sState = 1;

    // BUG-17415 autoextend off일 경우 nextsize, maxsize는 0을 유지한다.
    // smiDataFileAttr의 nextsize, maxsize는 그대로 둔다.
    if (aDataFileNode->mIsAutoExtend == ID_TRUE)
    {
        aDataFileNode->mNextSize = aDataFileAttr->mNextSize;
        aDataFileNode->mMaxSize  = aDataFileAttr->mMaxSize;
    }
    else
    {
        aDataFileNode->mNextSize = 0;
        aDataFileNode->mMaxSize = 0;
    }

    idlOS::memset( &(aDataFileNode->mDBFileHdr),
                   0x00,
                   ID_SIZEOF(sddDataFileHdr) );

    //PROJ-2133 incremental backup
    aDataFileNode->mDBFileHdr.mDataFileDescSlotID.mBlockID = 
                                        SMRI_CT_INVALID_BLOCK_ID; 
    aDataFileNode->mDBFileHdr.mDataFileDescSlotID.mSlotIdx = 
                                        SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;

    /* ------------------------------------------------
     * datafile 크기 validation 확인
     * ----------------------------------------------*/
    IDE_TEST_RAISE( aDataFileNode->mInitSize > aMaxDataFilePageCount,
                    error_invalid_filesize );
    IDE_TEST_RAISE( aDataFileNode->mCurrSize > aMaxDataFilePageCount,
                    error_invalid_filesize );
    IDE_TEST_RAISE( aDataFileNode->mMaxSize  > aMaxDataFilePageCount,
                    error_invalid_filesize );

    IDE_TEST_RAISE( (aDataFileNode->mIsAutoExtend == ID_TRUE) &&
                    ((aDataFileNode->mCurrSize > aDataFileNode->mMaxSize) ||
                     (aDataFileNode->mInitSize > aDataFileNode->mCurrSize)),
                    error_invalid_filesize );

    /* ------------------------------------------------
     * path에 대한 영문자, 숫자와 '/'만 허용하며,
     * 상대경로를 절대경로 변경하여 저장한다.
     * ----------------------------------------------*/

    if( aCheckPathValidate == ID_TRUE)
    {
        IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                    ID_TRUE,
                                    aDataFileAttr->mName,
                                    &aDataFileAttr->mNameLength,
                                    SMI_TBS_DISK)
                  != IDE_SUCCESS );
    }

    /* datafile 이름 */
    aDataFileNode->mName = NULL;

    /* TC/FIT/Limit/sm/sddDataFile_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sddDataFile::initialize::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(
                          IDU_MEM_SM_SDD,
                          aDataFileAttr->mNameLength + 1,
                          (void**)&(aDataFileNode->mName)) != IDE_SUCCESS,
                   insufficient_memory );

    idlOS::memset(aDataFileNode->mName, 0x00, aDataFileAttr->mNameLength + 1);
    idlOS::strcpy(aDataFileNode->mName, aDataFileAttr->mName);
    sState = 2;

    IDE_TEST( aDataFileNode->mFile.initialize( IDU_MEM_SM_SDD,
                                               smuProperty::getMaxOpenFDCount4File(),
                                               IDU_FIO_STAT_ON,
                                               IDV_WAIT_INDEX_LATCH_FREE_DRDB_FILEIO )
              != IDE_SUCCESS );

    IDE_TEST( aDataFileNode->mFile.setFileName(aDataFileNode->mName)
              != IDE_SUCCESS );

    /* datafile LRU 리스트의 NODE 리스트 초기화 */
    SMU_LIST_INIT_NODE(&aDataFileNode->mNode4LRUList);
    aDataFileNode->mNode4LRUList.mData = (void*)(aDataFileNode);

    // PRJ-1548 User Memory Tablespace
    aDataFileNode->mAnchorOffset = SCT_UNSAVED_ATTRIBUTE_OFFSET;
    
    idlOS::sprintf( sMutexName, "DATAFILE_NODE_MUTEX_%"ID_UINT32_FMT"_%"ID_UINT32_FMT"",
                    aTableSpaceID,
                    aDataFileNode->mID );

    IDE_TEST( aDataFileNode->mMutex.initialize( sMutexName,
                                                IDU_MUTEX_KIND_NATIVE,
                                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_filesize );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFileSizeOnLogAnchor,
                                aDataFileAttr->mName,
                                aDataFileNode->mInitSize,
                                aDataFileNode->mCurrSize,
                                aDataFileNode->mMaxSize,
                                (ULong)aMaxDataFilePageCount));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_PUSH();
        switch( sState )
        {
            case 2:
                IDE_ASSERT( iduMemMgr::free( aDataFileNode->mName )
                            == IDE_SUCCESS );
            case 1:
                IDE_ASSERT( iduMemMgr::free( aDataFileNode->mPageBuffPtr )
                            == IDE_SUCCESS );
            default:
                break;
        }
        IDE_POP();
        aDataFileNode->mName = NULL;
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : datafile 노드를 해제한다.
 **********************************************************************/
IDE_RC sddDataFile::destroy( sddDataFileNode* aDataFileNode )
{

    IDE_DASSERT( aDataFileNode != NULL );

    IDE_TEST( iduMemMgr::free( aDataFileNode->mPageBuffPtr )
              != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::free( aDataFileNode->mName ) != IDE_SUCCESS );

    IDE_TEST( aDataFileNode->mFile.destroy() != IDE_SUCCESS );

    IDE_TEST( aDataFileNode->mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 새로운 datafile을 create한다.
 datafile Layout는 다음과 같다.
 ------------------------------------------------------------
|  |             |               |               |           |
 ------------------------------------------------------------
 ^            |                                       |
 |            -----------------------------------------
 file heaer     data pages

 **********************************************************************/
IDE_RC sddDataFile::create( idvSQL          * aStatistics,
                            sddDataFileNode * aDataFileNode,
                            sddDataFileHdr  * aDBFileHdr )
{
    UInt   sState = 0;

    IDE_DASSERT( aDataFileNode != NULL );

    /* ===========================================================
     * [1] disk 레벨에서 동일한 이름을 가진 파일이 존재하는지 검사
     * =========================================================== */
    IDE_TEST_RAISE( idf::access(aDataFileNode->mName, F_OK) == 0,
                    error_already_exist_datafile );

    /* ===========================================================
     * [2] 화일 생성 및 오픈
     * =========================================================== */
    IDE_TEST( aDataFileNode->mFile.createUntilSuccess(
                smLayerCallback::setEmergency )
              != IDE_SUCCESS );
    sState = 1;

    /* BUG-47398 AIX 에서 iduMemMgr::free4malign() 함수에서 비정상 종료
     * 바로 close 하므로 DirectIO 여부는 중요하지는 않다, property를 따른다.*/
    IDE_TEST( open( aDataFileNode,
                    sddDiskMgr::isDirectIO() ) != IDE_SUCCESS );
    sState = 2;

    /* ===========================================================
     * [3] PRJ-1149, 화일헤더 쓰기.
     * =========================================================== */
    // BUG-17415 writing 단위 만큼씩 기록함.
    IDE_TEST( writeNewPages(aStatistics, aDataFileNode) != IDE_SUCCESS );

    IDE_TEST( writeDBFileHdr( aStatistics,
                              aDataFileNode,
                              aDBFileHdr )
              != IDE_SUCCESS );

    /* ===========================================================
     * [4] 데이타 화일 sync 및 close
     * =========================================================== */
    sState = 0;
    IDE_TEST( close( aDataFileNode ) != IDE_SUCCESS );

    //==========================================================================
    // To Fix BUG-13924
    //==========================================================================

    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_DISK_FILE_CREATE,
                (aDataFileNode->mName == NULL ) ? "" : aDataFileNode->mName);

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_exist_datafile );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AlreadyExistFile, aDataFileNode->mName));
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        switch (sState)
        {
            case 2:
                IDE_ASSERT( close( aDataFileNode ) == IDE_SUCCESS );

            case 1:
                IDE_ASSERT( idf::unlink( aDataFileNode->mName )
                            == IDE_SUCCESS );
            default:
                break;
        }
        IDE_POP();
    }

    return IDE_FAILURE;

}

IDE_RC sddDataFile::writeNewPages(idvSQL          *aStatistics,
                                  sddDataFileNode *aDataFileNode)
{
    UChar *sBufferPtr;
    UChar *sAlignedBuffer;
    UChar *sCurPos;
    ULong  i;
    ULong  sUnitSize;
    ULong  sWriteOffset;
    SInt   sAllocated = 0;

    IDE_DASSERT( aDataFileNode != NULL );

    sUnitSize = smuProperty::getDataFileWriteUnitSize();

    if (sUnitSize > aDataFileNode->mInitSize)
    {
        // 기록하려는 데이터파일의 크기가 쓰기 단위보다 작으면
        // 쓰기 단위는 데이터파일 크기가 된다.
        sUnitSize = aDataFileNode->mInitSize;
    }

    // 버퍼 공간을 할당받는다.
    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SDD,
                                           sUnitSize * SD_PAGE_SIZE,
                                           (void**)&sBufferPtr,
                                           (void**)&sAlignedBuffer ) != IDE_SUCCESS );
    sAllocated = 1;

    // 버퍼 공간을 초기화한다.
    //   1. 먼저 첫번째 페이지를 초기화한다.
    idlOS::memset(sAlignedBuffer, 0x00, SD_PAGE_SIZE);
    idlOS::memcpy(sAlignedBuffer, &(sddDiskMgr::mInitialCheckSum), ID_SIZEOF(UInt));

    //   2. 두번째 페이지부터는 첫번째 페이지를 복사한다.
    for (i = 1, sCurPos = sAlignedBuffer + SD_PAGE_SIZE;
         i < sUnitSize;
         i++, sCurPos += SD_PAGE_SIZE)
    {
        idlOS::memcpy(sCurPos, sAlignedBuffer, SD_PAGE_SIZE);
    }

    // 데이터 파일에 버퍼를 기록한다.
    //   1. sUnitSize 단위로 기록한다.
    for ( i = 0, sWriteOffset = SM_DBFILE_METAHDR_PAGE_OFFSET;
          i < aDataFileNode->mInitSize / sUnitSize;
          i++ )
    {
        /* BUG-18138 테이블스페이스 생성하다가 디스크 풀 상황이 되면
         * 해당 세션이 정리가 되지 않습니다.
         *
         * 이전에 Disk IO가 성공할때까지 대기해서 문제가 생겼었습니다.
         * 바로 에러가 발생하면 에러처리하도록 수정하였습니다. */
        IDE_TEST( aDataFileNode->mFile.write(
                                      aStatistics,
                                      sWriteOffset,
                                      sAlignedBuffer,
                                      sUnitSize * SD_PAGE_SIZE )
                  != IDE_SUCCESS);

        sWriteOffset += sUnitSize * SD_PAGE_SIZE;

        IDU_FIT_POINT( "1.BUG-18138@sddDataFile::writeNewPages" );

        IDU_FIT_POINT( "1.BUG-12754@sddDataFile::writeNewPages" );

        // BUG-12754 : session 연결이 끊겼는지 검사함
        IDE_TEST(iduCheckSessionEvent(aStatistics) != IDE_SUCCESS);
    }

    //   2. aDataFileNode->mInitSize - sUnitSize * i + 1 크기만큼 기록한다.
    //      여기서 1은 datafile header이다.
    IDE_TEST( aDataFileNode->mFile.write(
                         aStatistics,
                         sWriteOffset,
                         sAlignedBuffer,
                         (aDataFileNode->mInitSize - sUnitSize * i + 1)
                         * SD_PAGE_SIZE )
              != IDE_SUCCESS );

    sAllocated = 0;
    IDE_TEST(iduMemMgr::free(sBufferPtr) != IDE_SUCCESS);
    sBufferPtr = NULL;
    sAlignedBuffer = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if (sAllocated == 1)
        {
            IDE_ASSERT(iduMemMgr::free(sBufferPtr) == IDE_SUCCESS);
            sBufferPtr = NULL;
            sAlignedBuffer = NULL;

        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 해당 datafile을 reuse한다.
 **********************************************************************/
IDE_RC sddDataFile::reuse( idvSQL          * aStatistics,
                           sddDataFileNode * aDataFileNode )
{
    UInt            sState = 0;
    ULong           sCurrSize;
    ULong           sFileSize;

    IDE_DASSERT( aDataFileNode != NULL );

    sFileSize = 0;

    // must be fullpath
    IDE_TEST( aDataFileNode->mFile.setFileName(aDataFileNode->mName) != IDE_SUCCESS );

    /* BUGBUG - ERROR MSG - datafile이 존재하면 reuse 그렇지 않으면 생성 */
    if ( idf::access(aDataFileNode->mName, F_OK) == 0 )
    {
        /* BUG-47398 AIX 에서 iduMemMgr::free4malign() 함수에서 비정상 종료
         * 바로 close 하므로 DirectIO 여부는 중요하지는 않다, property를 따른다.*/
        IDE_TEST( open( aDataFileNode,
                        sddDiskMgr::isDirectIO() ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( aDataFileNode->mFile.getFileSize(&sFileSize) != IDE_SUCCESS );

        sCurrSize = (ULong)((sFileSize - SM_DBFILE_METAHDR_PAGE_SIZE) / SD_PAGE_SIZE);

        if ( sCurrSize > aDataFileNode->mInitSize )
        {
            IDE_TEST( aDataFileNode->mFile.truncate( SDD_CALC_PAGEOFFSET(aDataFileNode->mInitSize) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }

        /* ===========================================================
         * 데이타 화일의 실제 크기만큼 초기화
         * =========================================================== */
        // BUG-17415 writing 단위 만큼씩 기록함.
        IDE_TEST( writeNewPages(aStatistics, aDataFileNode) != IDE_SUCCESS );

        /* ===========================================================
         * PRJ-1149, 화일헤더 쓰기.
         * =========================================================== */

        IDE_TEST( writeDBFileHdr( aStatistics,
                                  aDataFileNode,
                                  &(aDataFileNode->mDBFileHdr) )
                  != IDE_SUCCESS );

        /* ===========================================================
         * 데이타 화일 sync 및 close
         * =========================================================== */
        sState = 0;
        IDE_TEST( close( aDataFileNode ) != IDE_SUCCESS );
    }
    else
    {
        aDataFileNode->mCreateMode = SMI_DATAFILE_CREATE;

        IDE_TEST( create(aStatistics, aDataFileNode) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        (void)close( aDataFileNode );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : datafile을 extend 시킨다.
 *
 * aStatistics   - [IN] 통계 정보
 * aDataFileNode - [IN] Datafile Node
 * aExtendSize   - [IN] 확장하고자 하는 크기로 Page Count이다.
 **********************************************************************/
IDE_RC sddDataFile::extend( idvSQL          * aStatistics,
                            sddDataFileNode * aDataFileNode,
                            ULong             aExtendSize )
{
    ULong  i;
    ULong  sWriteOffset;
    ULong  sExtendWriteOffset4CT;
    UChar* sBufferPtr;
    UChar* sAlignedBuffer;
    UInt   sWriteUnitSize;
    ULong  sRestSize;
    ULong  sExtendByte;
    UInt   sExtendedPageID;
    UInt   sIBChunkID;
    UInt   sExtendedPageCnt;
    UInt   sTotalExtendedPageCnt;
    UInt   sState = 0;

    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( aDataFileNode->mIsOpened == ID_TRUE );

    sExtendByte    = aExtendSize * SD_PAGE_SIZE;
    sWriteUnitSize = smuProperty::getDataFileWriteUnitSize() * SD_PAGE_SIZE;

    if( sExtendByte < sWriteUnitSize )
    {
        sWriteUnitSize = sExtendByte;
    }

    IDE_TEST( iduFile::allocBuff4DirectIO(
                  IDU_MEM_SM_SDD,
                  sWriteUnitSize,
                  (void**)&sBufferPtr,
                  (void**)&sAlignedBuffer )
              != IDE_SUCCESS );
    sState = 1;

    // 버퍼 공간을 초기화한다.
    //   1. 먼저 첫번째 페이지를 초기화한다.
    idlOS::memset(sAlignedBuffer, 0x00, SD_PAGE_SIZE);
    idlOS::memcpy(sAlignedBuffer, &(sddDiskMgr::mInitialCheckSum), ID_SIZEOF(UInt));

    sWriteOffset            = SDD_CALC_PAGEOFFSET(aDataFileNode->mCurrSize);
    sExtendWriteOffset4CT   = sWriteOffset;

    //   2. 두번째 페이지부터는 첫번째 페이지를 복사한다.
    for( i = SD_PAGE_SIZE; i < sWriteUnitSize; i += SD_PAGE_SIZE )
    {
        idlOS::memcpy( sAlignedBuffer + i, sAlignedBuffer, SD_PAGE_SIZE );
    }

    // 데이터 파일에 버퍼를 기록한다.
    //   1. sUnitSize 단위로 기록한다.
    for ( i = 0;
          i < ( sExtendByte / sWriteUnitSize );
          i++ )
    {
        IDU_FIT_POINT( "1.BUG-12754@sddDataFile::extend" );

        /* BUG-18138 테이블스페이스 생성하다가 디스크 풀 상황이 되면
         * 해당 세션이 정리가 되지 않습니다.
         *
         * 이전에 Disk IO가 성공할때까지 대기해서 문제가 생겼었습니다.
         * 바로 에러가 발생하면 에러처리하도록 수정하였습니다. */
        IDE_TEST( aDataFileNode->mFile.write(
                      aStatistics,
                      sWriteOffset,
                      sAlignedBuffer,
                      sWriteUnitSize )
                  != IDE_SUCCESS);

        sWriteOffset += sWriteUnitSize;

        IDU_FIT_POINT( "1.BUG-18138@sddDataFile::extend" );

        // BUG-12754 : session 연결이 끊겼는지 검사함
        IDE_TEST(iduCheckSessionEvent(aStatistics) != IDE_SUCCESS);
    }

    sRestSize = sExtendByte - ( sWriteUnitSize * i );

    if( sRestSize != 0 )
    {
        //   2. sExtendByte - sUnitSize * i 크기만큼 기록한다.
        IDE_TEST(aDataFileNode->mFile.write(
                     aStatistics,
                     sWriteOffset,
                     sAlignedBuffer,
                     sRestSize )
                 != IDE_SUCCESS);
    }

    IDE_TEST( aDataFileNode->mFile.syncUntilSuccess(
                  smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    // PROJ-2133 Incremental backup
    // temptablespace 에 대해서는 change tracking 할 필요가 없습니다.
    if ( ( smLayerCallback::isCTMgrEnabled() == ID_TRUE ) &&
         ( sctTableSpaceMgr::isTempTableSpace( aDataFileNode->mSpaceID ) != ID_TRUE ) )
    {
        sTotalExtendedPageCnt = ( sExtendByte / SD_PAGE_SIZE );

        for( sExtendedPageCnt = 0; 
             sExtendedPageCnt <  sTotalExtendedPageCnt;
             sExtendedPageCnt++,
             sExtendWriteOffset4CT += SD_PAGE_SIZE )
        {
            sExtendedPageID = (sExtendWriteOffset4CT - SM_DBFILE_METAHDR_PAGE_SIZE) 
                              / SD_PAGE_SIZE;

            sIBChunkID = smriChangeTrackingMgr::calcIBChunkID4DiskPage( 
                                                            sExtendedPageID );

            IDE_TEST( smriChangeTrackingMgr::changeTracking(
                            aDataFileNode,
                            NULL,
                            sIBChunkID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* nothing to do */
    }

    //==========================================================================
    // To Fix BUG-13924
    //==========================================================================
    ideLog::log( SM_TRC_LOG_LEVEL_DISK,
                 SM_TRC_DISK_FILE_EXTEND,
                 sWriteOffset - sExtendByte,
                 sWriteOffset,
                 (aDataFileNode->mName == NULL) ? "" : aDataFileNode->mName);

    sState = 0;
    IDE_TEST( iduMemMgr::free( sBufferPtr )
              != IDE_SUCCESS );

    sBufferPtr     = NULL;
    sAlignedBuffer = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( sBufferPtr ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 하나의 datafile을 오픈한다.
 *
 * + 2nd. code design
 *   - datafile을 open한다.
 *   - datafile 노드에 open 모드로 설정
 *
 * BUG-47398 AIX 에서 iduMemMgr::free4malign() 함수에서 비정상 종료
 *           Direct IO 유무 호출 함수에서 결정한다.
 **********************************************************************/
IDE_RC sddDataFile::open( sddDataFileNode* aDataFileNode,
                          idBool           aIsDirectIO )
{
    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( aDataFileNode->mIsOpened != ID_TRUE );

    IDU_FIT_POINT( "BUG-46596@sddDataFile::open::exceptionTest");
    IDE_TEST( aDataFileNode->mFile.openUntilSuccess( aIsDirectIO ) != IDE_SUCCESS );

    aDataFileNode->mIsOpened = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : datafile을 close한다.
 *
 * + 2nd. code design
 *   - datafile을 닫는다.
 *   - datafile 노드에 close 모드를 설정한다.
 **********************************************************************/
IDE_RC sddDataFile::close( sddDataFileNode  *aDataFileNode )
{
    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( isOpened(aDataFileNode) == ID_TRUE );
    IDE_DASSERT( getIOCount(aDataFileNode) == 0 );

    if ( aDataFileNode->mIsModified == ID_TRUE )
    {
        IDE_TEST( sync( aDataFileNode,
                        smLayerCallback::setEmergency ) != IDE_SUCCESS );
        aDataFileNode->mIsModified = ID_FALSE;
    }

    // XXX 순서를 변경한 이유가 뭘까.. 
    aDataFileNode->mIsOpened = ID_FALSE;

    IDE_TEST( aDataFileNode->mFile.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : datafile을 축소한다.(truncate)
 **********************************************************************/
IDE_RC sddDataFile::truncate( sddDataFileNode*  aDataFileNode,
                              ULong             aNewDataFileSize )
{

    ULong sOldFileSize = 0;

    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( aDataFileNode->mIsOpened == ID_TRUE );

    // To Fix BUG-13924
    IDE_TEST( aDataFileNode->mFile.getFileSize( &sOldFileSize ) != IDE_SUCCESS );

    //BUGBUG.
    IDE_TEST( aDataFileNode->mFile.truncate( SDD_CALC_PAGEOFFSET(aNewDataFileSize) )
              != IDE_SUCCESS );

    IDE_TEST( aDataFileNode->mFile.syncUntilSuccess(
                  smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    //====================================================================
    // To Fix BUG-13924
    //====================================================================

    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_DISK_FILE_TRUNCATE,
                sOldFileSize,
                SDD_CALC_PAGEOFFSET(aNewDataFileSize),
                (aDataFileNode->mName == NULL) ? "" : aDataFileNode->mName);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


}


/***********************************************************************
 * Description : 실제 datafile의 삭제는 트랜잭션 커밋 직전에 수행됨.
 **********************************************************************/
IDE_RC sddDataFile::addPendingOperation( void             *aTrans,
                                         sddDataFileNode  *aDataFileNode,
                                         idBool            aIsCommit,
                                         sctPendingOpType  aPendingOpType,
                                         sctPendingOp     **aPendingOp )
{
    sctPendingOp *sPendingOp;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aDataFileNode != NULL );

    IDE_TEST( sctTableSpaceMgr::addPendingOperation( aTrans,
                                                     aDataFileNode->mSpaceID,
                                                     aIsCommit,
                                                     aPendingOpType,
                                                     & sPendingOp )
              != IDE_SUCCESS );

    sPendingOp->mFileID        = aDataFileNode->mID;

    if ( aPendingOp != NULL )
    {
        *aPendingOp = sPendingOp;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Description : datafile노드의 autoextend를 모드 설정
 *               autoexnted on/off, next size, maxsize는 이 함수를
 *               통해서 바꿔야만 한다.
 *               autoextend on/off에 따라 next size, maxsize의
 *               consistent한 값이 있다.
 **********************************************************************/
void sddDataFile::setAutoExtendProp( sddDataFileNode* aDataFileNode,
                                     idBool           aAutoExtendMode,
                                     ULong            aNextSize,
                                     ULong            aMaxSize )
{
    IDE_DASSERT( aDataFileNode != NULL );

    if ( aAutoExtendMode == ID_FALSE )
    {
        // autoextend off일 경우
        // nextsize와 maxsize는 모두 0 이어야 한다.
        // 사용자의 오류는 코드상 있을 수 없어야 하므로
        // assert로 보장한다.
        IDE_ASSERT(aNextSize == 0);
        IDE_ASSERT(aMaxSize == 0);
    }

    //====================================================================
    // To Fix BUG-13924
    //====================================================================

    ideLog::log( SM_TRC_LOG_LEVEL_DISK,
                 SM_TRC_DISK_FILE_EXTENDMODE,
                 aDataFileNode->mIsAutoExtend,
                 aAutoExtendMode,
                 (aDataFileNode->mName == NULL) ? "" : aDataFileNode->mName );

    aDataFileNode->mIsAutoExtend = aAutoExtendMode;
    aDataFileNode->mNextSize = aNextSize;
    aDataFileNode->mMaxSize = aMaxSize;
}

/***********************************************************************
 * Description : datafile 노드의 currsize를 set한다.
 **********************************************************************/
void sddDataFile::setCurrSize( sddDataFileNode*  aDataFileNode,
                               ULong             aSize )
{


    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( aSize <= SD_MAX_PAGE_COUNT);

    //====================================================================
    // To Fix BUG-13924
    //====================================================================

    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_DISK_FILE_CURRSIZE,
                aDataFileNode->mCurrSize,
                aSize,
                (aDataFileNode->mName == NULL) ? "" : aDataFileNode->mName);

    aDataFileNode->mCurrSize = aSize;

    return;

}

/***********************************************************************
 * Description : datafile 노드의 initsize를 set한다.
 * truncate할 때만 변한다.
 **********************************************************************/
void sddDataFile::setInitSize( sddDataFileNode*  aDataFileNode,
                               ULong             aSize )
{

    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( aSize <= SD_MAX_PAGE_COUNT );

    //====================================================================
    // To Fix BUG-13924
    //====================================================================

    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_DISK_FILE_INITSIZE,
                aDataFileNode->mInitSize,
                aSize,
                (aDataFileNode->mName == NULL) ? "" : aDataFileNode->mName);

    aDataFileNode->mInitSize = aSize;

}

/***********************************************************************
 * Description : datafile 노드의 새로운 datafile 이름 설정
 *
 * # 단, MEDIA RECOVERY과정에서는 새로운 데이타 파일생성을 위하여,
 *   기존 파일 노드의 이름만 갱신한다.
 *   - alter database create datafile 또는
 *     alter database rename file을 수행할때 불림.
 **********************************************************************/
IDE_RC sddDataFile::setDataFileName( sddDataFileNode* aDataFileNode,
                                     SChar*           aNewName,
                                     idBool           aCheckAccess )
{

    SChar* sOldFileName;
    UInt   sStrLen;
    UInt   sState = 0;

    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( aNewName != NULL );

    sOldFileName = aDataFileNode->mName;

    if (aCheckAccess == ID_TRUE)
    {
        IDE_TEST_RAISE( idf::access(aNewName, F_OK) != 0,
                        error_not_exist_file );
        IDE_TEST_RAISE( idf::access(aNewName, R_OK) != 0,
                        error_no_read_perm_file );
        IDE_TEST_RAISE( idf::access(aNewName, W_OK) != 0,
                        error_no_write_perm_file );
    }

    sStrLen = idlOS::strlen(aNewName);

    /* sddDataFile_setDataFileName_malloc_Name.tc */
    IDU_FIT_POINT("sddDataFile::setDataFileName::malloc::Name");
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_SM_SDD,
                                sStrLen + 1,
                                (void**)&(aDataFileNode->mName),
                                IDU_MEM_FORCE) != IDE_SUCCESS );
    sState = 1;

    idlOS::memset(aDataFileNode->mName, 0x00, sStrLen);
    idlOS::strcpy(aDataFileNode->mName, aNewName);


    /* BUG-31401 [sm-disk-recovery] When changing a Data file name, 
     *           does not refresh FD of DataFileNode.
     * 파일을 완전히 닫아야지만, 과거에 사용하던 FD를 닫을 수 있습니다.
     * 그렇지 않으면, 리네임 했음에도 과거의 파일을 접근합니다. */
    if( aDataFileNode->mIsOpened == ID_TRUE )
    {
        IDE_TEST( aDataFileNode->mFile.close() != IDE_SUCCESS );
        sState = 2;
    }

    IDE_TEST( aDataFileNode->mFile.setFileName(aDataFileNode->mName) != IDE_SUCCESS );

    if( sState == 2 )
    {
        IDE_TEST( aDataFileNode->mFile.open(
                ((smuProperty::getIOType() == 0) ?
                 ID_FALSE : ID_TRUE)) != IDE_SUCCESS );
        sState = 1;
    }

 
    //====================================================================
    // To Fix BUG-13924
    //====================================================================

    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_DISK_FILE_RENAME,
                (sOldFileName == NULL) ? "" : sOldFileName, aNewName);

    if ( sOldFileName != NULL )
    {
        IDE_TEST( iduMemMgr::free(sOldFileName) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_exist_file );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileByPath, aNewName ) );
    }
    IDE_EXCEPTION( error_no_read_perm_file );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NoReadPermFile, aNewName ) );
    }
    IDE_EXCEPTION( error_no_write_perm_file );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NoWritePermFile, aNewName ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( aDataFileNode->mFile.open(
                    ((smuProperty::getIOType() == 0) ?
                     ID_FALSE : ID_TRUE)) == IDE_SUCCESS );

            /* BUG-33353 
             * 예외상황에서 할당받은 메모리를 해제 하지 않고 있었음. */
        case 1:
            (void)iduMemMgr::free( aDataFileNode->mName );

        default:
            break;
    }

    return IDE_FAILURE;
}
/***********************************************************************
 *   데이타파일 HEADER에 체크포인트 정보를 기록한다.
 *
 *  [IN] aDataFileNode  : DBF Node 포인트
 *  [IN] aRedoSN        : 미디어복구에 필요한 Disk Redo SN
 *  [IN] aDiskCreateLSN : 미디어복구에 필요한 Disk Create LSN
 *  [IN] aMustRedoToLSN : 미디어복구에 필요한 Must Redo To LSN
 *  [IN] aDataFileDescSlotID : change tracking을 위한 CT파일의 slotID
 **********************************************************************/
void sddDataFile::setDBFHdr( sddDataFileHdr*            aFileMetaHdr,
                             smLSN*                     aRedoLSN,
                             smLSN*                     aCreateLSN,
                             smLSN*                     aMustRedoToLSN,
                             smiDataFileDescSlotID*     aDataFileDescSlotID )
{
    IDE_DASSERT( aFileMetaHdr != NULL );

    if ( aRedoLSN != NULL )
    {
        SM_GET_LSN( aFileMetaHdr->mRedoLSN, *aRedoLSN );
    }

    if ( aCreateLSN != NULL )
    {
        SM_GET_LSN( aFileMetaHdr->mCreateLSN, *aCreateLSN );
    }

    if ( aMustRedoToLSN != NULL )
    {
        SM_GET_LSN( aFileMetaHdr->mMustRedoToLSN, *aMustRedoToLSN );
    }

    //PROJ-2133 incremental backup
    if ( aDataFileDescSlotID != NULL )
    {
        aFileMetaHdr->mDataFileDescSlotID.mBlockID = 
                                    aDataFileDescSlotID->mBlockID;

        aFileMetaHdr->mDataFileDescSlotID.mSlotIdx = 
                                    aDataFileDescSlotID->mSlotIdx;
    }

    return;
}

/***********************************************************************
 * Description : PRJ-1149  media recovery가 필요한지 검사
 * binary db version이 불일치하거나 create lsn이 값이 잘못된 경우
 * invalid file이며, oldest lst이 불일치하는 경우는 media recovery가
 * 필요한 경우로 판단한다.
 *
 *    aFileNode       - [IN]  Loganchor에서 읽은 FileNode
 *    aDBFileHdr      - [IN]  DBFile에서 읽은 DBFileHdr
 *    aIsMediaFailure - [OUT] MediaRecovery가 필요한지를 반환한다.
 **********************************************************************/
IDE_RC sddDataFile::checkValidationDBFHdr(
                         sddDataFileNode  * aFileNode,
                         sddDataFileHdr   * aDBFileHdr,
                         idBool           * aIsMediaFailure )
{
    UInt              sDBVer    = 0;
    UInt              sCtlVer   = 0;
    UInt              sFileVer  = 0;
    sddDataFileHdr*   sDBFileHdrOfNode;

    IDE_DASSERT( aFileNode       != NULL );
    IDE_DASSERT( aDBFileHdr      != NULL );
    IDE_DASSERT( aIsMediaFailure != NULL );

    sDBFileHdrOfNode = &(aFileNode->mDBFileHdr);

#ifdef DEBUG
    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_MRECOVERY_CHECK_DB_SID_FID,
                aFileNode->mSpaceID,
                aFileNode->mID);

    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_MRECOVERY_CHECK_DISK_RLSN,
                sDBFileHdrOfNode->mRedoLSN.mFileNo,
                sDBFileHdrOfNode->mRedoLSN.mOffset,
                aDBFileHdr->mRedoLSN.mFileNo,
                aDBFileHdr->mRedoLSN.mOffset);

    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_MRECOVERY_CHECK_DISK_CLSN,
                sDBFileHdrOfNode->mCreateLSN.mFileNo,
                sDBFileHdrOfNode->mCreateLSN.mOffset,
                aDBFileHdr->mCreateLSN.mFileNo,
                aDBFileHdr->mCreateLSN.mOffset);
#endif

    IDE_TEST_RAISE( checkValuesOfDBFHdr( sDBFileHdrOfNode )
                    != IDE_SUCCESS,
                    err_invalid_hdr );

    sDBVer   = smVersionID & SM_CHECK_VERSION_MASK;
    sCtlVer  = sDBFileHdrOfNode->mSmVersion & SM_CHECK_VERSION_MASK;
    sFileVer = aDBFileHdr->mSmVersion & SM_CHECK_VERSION_MASK;

    // [1] SM VERSION 확인
    // 데이타파일과 서버 바이너리의 호환성을 검사한다.
    IDE_TEST_RAISE(sDBVer != sCtlVer, err_invalid_hdr);
    IDE_TEST_RAISE(sDBVer != sFileVer, err_invalid_hdr);

    // [2] CREATE SN 확인
    // 데이타파일의 Create SN은 Loganchor의 Create SN과 동일해야한다.
    // 다르면 파일명만 동일한 완전히 다른 데이타파일이다.
    // CREATE LSNN
    IDE_TEST_RAISE( smLayerCallback::isLSNEQ( &(sDBFileHdrOfNode->mCreateLSN),
                                              &(aDBFileHdr->mCreateLSN) )
                    != ID_TRUE, err_invalid_hdr );

    // [3] Redo LSN 확인
    if ( smLayerCallback::isLSNLTE(
                &sDBFileHdrOfNode->mRedoLSN,
                &aDBFileHdr->mRedoLSN) == ID_TRUE )
    {
        // 데이타 파일의 REDO SN보다 Loganchor의 Redo SN이 더 작거나
        // 같아야 한다.

        *aIsMediaFailure = ID_FALSE; // 미디어 복구 불필요

#ifdef DEBUG
        ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                    SM_TRC_MRECOVERY_CHECK_DB_SUCCESS);
#endif
    }
    else
    {
        // 데이타 파일의 REDO SN보다 Loganchor의 Redo SN이 더 크면
        // 미디어 복구가 필요하다.

        *aIsMediaFailure = ID_TRUE; // 미디어 복구 필요

#ifdef DEBUG
        ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                    SM_TRC_MRECOVERY_CHECK_NEED_MEDIARECOV);
#endif
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_hdr );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFileHdr,
                                aFileNode->mSpaceID,
                                aFileNode->mID));

        /* BUG-33353 */
        ideLog::log(IDE_SM_0,
                    SM_TRC_MRECOVERY_CHECK_DB_SID_FID,
                    aFileNode->mSpaceID,
                    aFileNode->mID);

        ideLog::log(IDE_SM_0,
                    SM_TRC_MRECOVERY_CHECK_DISK_RLSN,
                    sDBFileHdrOfNode->mRedoLSN.mFileNo,
                    sDBFileHdrOfNode->mRedoLSN.mOffset,
                    aDBFileHdr->mRedoLSN.mFileNo,
                    aDBFileHdr->mRedoLSN.mOffset);

        ideLog::log(IDE_SM_0,
                    SM_TRC_MRECOVERY_CHECK_DISK_CLSN,
                    sDBFileHdrOfNode->mCreateLSN.mFileNo,
                    sDBFileHdrOfNode->mCreateLSN.mOffset,
                    aDBFileHdr->mCreateLSN.mFileNo,
                    aDBFileHdr->mCreateLSN.mOffset);

        ideLog::log( IDE_SM_0,
                     "DBVer = %u"
                     ", CtrVer = %u"
                     ", FileVer = %u",
                     sDBVer, sCtlVer, sFileVer );
    }
    IDE_EXCEPTION_END;

    ideLog::log(SM_TRC_LOG_LEVEL_DISK,
                SM_TRC_MRECOVERY_CHECK_DB_FAILURE);

    return IDE_FAILURE;
}

/*
   데이타파일 HEADERD의 유효성을 검사한다.

   [IN] aDBFileHdr : sddDataFileHdr 타입의 포인터
*/
IDE_RC sddDataFile::checkValuesOfDBFHdr(
                          sddDataFileHdr*  aDBFileHdr )
{
    smLSN sCmpLSN;

    IDE_DASSERT( aDBFileHdr != NULL );

    // REDO LSN과 CREATE LSN은 0이거나 ULONG_MAX 값을 가질 수 없다.
    SM_LSN_INIT(sCmpLSN);
    IDE_TEST( smLayerCallback::isLSNEQ( &aDBFileHdr->mRedoLSN,
                                        &sCmpLSN ) == ID_TRUE );

    IDE_TEST( smLayerCallback::isLSNEQ( &aDBFileHdr->mCreateLSN,
                                        &sCmpLSN ) == ID_TRUE );

    SM_LSN_MAX(sCmpLSN);
    IDE_TEST( smLayerCallback::isLSNEQ( &aDBFileHdr->mRedoLSN,
                                        &sCmpLSN ) == ID_TRUE );

    IDE_TEST( smLayerCallback::isLSNEQ( &aDBFileHdr->mCreateLSN,
                                        &sCmpLSN ) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

void sddDataFile::getDataFileAttr( sddDataFileNode*   aDataFileNode,
                                   smiDataFileAttr*   aDataFileAttr )
{
    sddDataFileHdr * sFileMetaHdr;

    IDE_DASSERT( aDataFileNode != NULL );
    IDE_DASSERT( aDataFileAttr != NULL );

    sFileMetaHdr = &(aDataFileNode->mDBFileHdr);

    // PRJ-1548 User Memory Tablespace
    aDataFileAttr->mAttrType  = SMI_DBF_ATTR;
    aDataFileAttr->mSpaceID   = aDataFileNode->mSpaceID;
    aDataFileAttr->mID        = aDataFileNode->mID;

    idlOS::strcpy(aDataFileAttr->mName, aDataFileNode->mName);
    aDataFileAttr->mNameLength = idlOS::strlen(aDataFileNode->mName);

    aDataFileAttr->mIsAutoExtend = aDataFileNode->mIsAutoExtend;

    aDataFileAttr->mState        = aDataFileNode->mState;
    aDataFileAttr->mMaxSize      = aDataFileNode->mMaxSize;
    aDataFileAttr->mNextSize     = aDataFileNode->mNextSize;
    aDataFileAttr->mCurrSize     = aDataFileNode->mCurrSize;
    aDataFileAttr->mInitSize     = aDataFileNode->mInitSize;
    aDataFileAttr->mCreateMode   = aDataFileNode->mCreateMode;

    //PROJ-2133 incremental backup
    if ( ( smLayerCallback::isCTMgrEnabled() == ID_TRUE ) || 
         ( smLayerCallback::isCreatingCTFile() == ID_TRUE ) )
    {
        aDataFileAttr->mDataFileDescSlotID.mBlockID = 
                        aDataFileNode->mDBFileHdr.mDataFileDescSlotID.mBlockID;
        aDataFileAttr->mDataFileDescSlotID.mSlotIdx = 
                        aDataFileNode->mDBFileHdr.mDataFileDescSlotID.mSlotIdx;
    }
    else
    {
        aDataFileAttr->mDataFileDescSlotID.mBlockID = 
                                    SMRI_CT_INVALID_BLOCK_ID;
        aDataFileAttr->mDataFileDescSlotID.mSlotIdx = 
                                    SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;
    }
    //PRJ-1149 backup file header copy added.
    SM_GET_LSN( aDataFileAttr->mCreateLSN,
                sFileMetaHdr->mCreateLSN );
    return;
}

/*
 * Disk Data File Header를 기록한다.
 *
 * aDataFileNode - [IN] Data file node
 * aDBFileHdr    - [IN] 디스크 데이타파일의 메타헤더
 *
 */
IDE_RC sddDataFile::writeDBFileHdr( idvSQL          * aStatistics,
                                    sddDataFileNode * aDataFileNode,
                                    sddDataFileHdr  * aDBFileHdr )
{
    SChar *sHeadPageBuffPtr = NULL;
    SChar *sHeadPageBuff = NULL;

    /* fix BUG-17825 Data File Header기록중 Process Failure시
     *               Startup안될 수 있음
     *
     * 모든 파일시스템에서는 Block( 512 Bytes )에 대한 Atomic IO를 수행한다.
     * Write하고자 하는 자료가 Block 경계에 걸리지만 않는다면 안전하게 내려가
     * 거나 아예 내려가지 않기 때문에 자료가 깨질가능성이 없다.
     * sddDataFileHdr는 데이타파일의 0 offset에 저장되고 크기가 512바이트 이하
     * 이기 때문에  Block 경계에 걸쳐서 포함되지 않으므로 Atomic하게
     * Disk에 내려갈 수 있다.
     *
     * 만약 512 Bytes 넘는다면, Double Write 형식으로 수정되어야 한다. */
    
    /* PROJ-2133 incremental backup
     * 파일헤더에 backupinfo가 추가됨으로인해 512bytes가 넘게 되나 백업파일에만
     * 존재하는 정보로 일반적인 데이터파일에는 사용되지않는다.
     * 따라서 backup info정보 크기를 datafileHdr에서 빼서 512byte가 넘는지 검사한다.
     */
    IDE_ASSERT( (ID_SIZEOF( sddDataFileHdr ) - ID_SIZEOF( smriBISlot )) < 512 );

    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SDD,
                                           SD_PAGE_SIZE,
                                           (void**)&sHeadPageBuffPtr,
                                           (void**)&sHeadPageBuff )
              != IDE_SUCCESS );

    if ( aDBFileHdr != NULL )
    {
        idlOS::memcpy( sHeadPageBuff, aDBFileHdr, ID_SIZEOF( sddDataFileHdr ) );
    }
    else
    {
        idlOS::memcpy( sHeadPageBuff,
                       &( aDataFileNode->mDBFileHdr ),
                       ID_SIZEOF( sddDataFileHdr ) );
    }

    aDataFileNode->mFile.writeUntilSuccess(
                                    aStatistics,
                                    SM_DBFILE_METAHDR_PAGE_OFFSET,
                                    sHeadPageBuff,
                                    SM_DBFILE_METAHDR_PAGE_SIZE,
                                    smLayerCallback::setEmergency );

    IDE_TEST( iduMemMgr::free( sHeadPageBuffPtr ) != IDE_SUCCESS );
    sHeadPageBuffPtr = NULL;
    sHeadPageBuff = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    if( sHeadPageBuffPtr != NULL )
    {
        IDE_PUSH();
        IDE_ASSERT( iduMemMgr::free( sHeadPageBuffPtr ) == IDE_SUCCESS );
        IDE_POP();

        sHeadPageBuffPtr = NULL;
    }
    return IDE_FAILURE;
}


/***********************************************************************
 * 임의의 DBFile에 DiskDataFileHeader를 기록한다.
 *
 * aBBFilePath - [IN] DB file의 경로
 * aDBFileHdr  - [IN] 디스크 데이타파일의 메타헤더
 **********************************************************************/
IDE_RC sddDataFile::writeDBFileHdrByPath( SChar           * aDBFilePath,
                                          sddDataFileHdr  * aDBFileHdr )
{
    iduFile sFile;

    IDE_ASSERT( aDBFilePath != NULL );
    IDE_ASSERT( aDBFileHdr  != NULL );

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SDD,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    IDE_TEST( sFile.setFileName( aDBFilePath ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sFile.open() != IDE_SUCCESS , err_file_not_found);

    IDE_TEST_RAISE( sFile.write( NULL, /*idvSQL*/
                                 SM_DBFILE_METAHDR_PAGE_OFFSET,
                                 aDBFileHdr,
                                 ID_SIZEOF(sddDataFileHdr) )
                    != IDE_SUCCESS, err_dbfilehdr_write_failure );

    IDE_TEST( sFile.close()   != IDE_SUCCESS );
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_found );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileByPath,
                                  aDBFilePath ) );
    }
    IDE_EXCEPTION( err_dbfilehdr_write_failure );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_Datafile_Header_Write_Failure,
                                  aDBFilePath ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#if 0 //not used
/***********************************************************************
 * Description : datafile 노드의 정보를 출력
 **********************************************************************/
IDE_RC sddDataFile::dumpDataFileNode( sddDataFileNode* aDataFileNode )
{
    sddDataFileHdr*    sFileMetaHdr;
    SChar              sFileStatusBuff[500 + 1];

    IDE_ERROR( aDataFileNode != NULL );

    sFileMetaHdr = &( aDataFileNode->mDBFileHdr );

    idlOS::printf("[DUMPDBF-BEGIN]\n");
    idlOS::printf("[HEADER]OLSN: <%"ID_UINT32_FMT",%"ID_UINT32_FMT",%"ID_UINT32_FMT">\n",
                  sFileMetaHdr->mRedoLSN.mFileNo,
                  sFileMetaHdr->mRedoLSN.mOffset);
    idlOS::printf("[HEADER]CLSN    : <%"ID_UINT32_FMT",%"ID_UINT32_FMT",%"ID_UINT32_FMT">\n",
                  sFileMetaHdr->mCreateLSN.mFileNo,
                  sFileMetaHdr->mCreateLSN.mOffset);

    idlOS::printf("[GENERAL]DBF PATH: %s\n", aDataFileNode->mName );

    idlOS::printf("[GENERAL]Create Mode: %s\n",
                  (aDataFileNode->mCreateMode == SMI_DATAFILE_CREATE)?
                  (SChar*)"create":(SChar*)"reuse");

    idlOS::printf("[GENERAL]Init Size: %"ID_UINT64_FMT"\n", aDataFileNode->mInitSize);
    idlOS::printf("[GENERAL]Curr Size: %"ID_UINT64_FMT"\n", aDataFileNode->mCurrSize);
    idlOS::printf("[GENERAL]AutoExtend Mode: %s\n", (aDataFileNode->mIsAutoExtend == ID_TRUE)?
                  (SChar*)"on":(SChar*)"off");

    // klocwork SM
    idlOS::memset( sFileStatusBuff, 0x00, 500 + 1);

    if( SMI_FILE_STATE_IS_OFFLINE( aDataFileNode->mState) )
    {
        idlOS::strncat(sFileStatusBuff,"OFFLINE | ", 500);
    }
    if( SMI_FILE_STATE_IS_ONLINE( aDataFileNode->mState ) )
    {
        idlOS::strncat(sFileStatusBuff,"ONLINE | ", 500);
    }
    if( SMI_FILE_STATE_IS_CREATING( aDataFileNode->mState ) )
    {
        idlOS::strncat(sFileStatusBuff,"CREATING | ", 500);
    }
    if( SMI_FILE_STATE_IS_DROPPING( aDataFileNode->mState ) )
    {
        idlOS::strncat(sFileStatusBuff,"DROPPING | ", 500);
    }
    if( SMI_FILE_STATE_IS_RESIZING( aDataFileNode->mState ) )
    {
        idlOS::strncat(sFileStatusBuff,"RESIZING | ", 500);
    }
    if( SMI_FILE_STATE_IS_DROPPED( aDataFileNode->mState ) )
    {
        idlOS::strncat(sFileStatusBuff,"DROPPED | ", 500);
    }
    if( SMI_FILE_STATE_IS_BACKUP( aDataFileNode->mState ) )
    {
        idlOS::strncat(sFileStatusBuff,"BACKUP | ", 500);
    }

    sFileStatusBuff[idlOS::strlen(sFileStatusBuff) - 2] = 0;

    idlOS::printf("[GENERAL]STATE: %s\n",sFileStatusBuff);
    idlOS::printf("[GENERAL]Next Size: %"ID_UINT64_FMT"\n", aDataFileNode->mNextSize);
    idlOS::printf("[GENERAL]Max  Size: %"ID_UINT64_FMT"\n", aDataFileNode->mMaxSize);
    idlOS::printf("[GENERAL]Open Mode: %s\n", (aDataFileNode->mIsOpened == ID_TRUE)?
                  (SChar*)"open":(SChar*)"close");
    idlOS::printf("[GENERAL]I/O Count: %"ID_UINT32_FMT"\n", aDataFileNode->mIOCount);
    idlOS::printf("[GENERAL]Modified: %s\n", (aDataFileNode->mIsModified == ID_TRUE)?
                  (SChar*)"yes":(SChar*)"no");
    idlOS::printf("[DUMPDBF-END]\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
