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
 * $$Id:$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102  The Fast Secondary Buffer
 * file I/O 및 데이타가를 저장되는 물리적인 공간을 관리 
 **********************************************************************/

#include <idu.h>
#include <idl.h>

#include <smDef.h>
#include <smErrorCode.h>
#include <smiDef.h>
#include <sddDef.h>
#include <sdbDef.h>
#include <smrDef.h>
#include <smu.h>
#include <sds.h>
#include <sdsReq.h>

/***********************************************************************
 * Description : 
 ***********************************************************************/
IDE_RC sdsFile::initialize( UInt aGroupCnt, 
                            UInt aPageCnt )
{
    initializeStatic( aGroupCnt, aPageCnt );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
IDE_RC sdsFile::destroy()
{ 
    if( mFileNode != NULL )
    {
        close( NULL /*aStatistics*/ );

        destroyStatic();
    }
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
IDE_RC sdsFile::initializeStatic( UInt aGroupCnt, 
                                  UInt aPageCnt )
{
    mSBufferDirName = (SChar *)smuProperty::getSBufferDirectoryName();
    mFrameCntInExt = smuProperty::getBufferIOBufferSize();
    /* Secondary Buffer File을 구성하는 Group 의 수 */
    mGroupCnt = aGroupCnt;    
    /* Group 을 구성하는 page의 수 */
    mPageCntInGrp   = aPageCnt;
    /* Group 당 하나씩 존재하는 MetaTable을 구성하는 page 수 */
    mPageCntInMetaTable = mGroupCnt * SDS_META_TABLE_PAGE_COUNT;

    IDE_TEST( mFileMutex.initialize(
                        (SChar*)"SECONDARY_BUFFER_FILE_MUTEX",
                        IDU_MUTEX_KIND_POSIX,
                        IDV_WAIT_INDEX_LATCH_FREE_OTHERS )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
IDE_RC sdsFile::destroyStatic()
{
    IDE_TEST( freeFileNode() != IDE_SUCCESS );
    IDE_TEST( mFileMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 노드 할당
   aFilePageCnt - [IN]  생성 요청하는 파일 크기  
                        Secondary Buffer Size + MetaTable 크기
   aFileName    - [IN]  생성 요청하는 이름 
   aFileNode    - [OUT] 생성한 노드를 반환
 ************************************************************************/
IDE_RC sdsFile::initFileNode( ULong           aFilePageCnt, 
                              SChar         * aFileName, 
                              sdsFileNode  ** aFileNode )
{
    if( aFileNode == NULL )
    {
        IDE_TEST( makeValidABSPath( aFileName ) != IDE_SUCCESS );
    }

    IDU_FIT_POINT_RAISE( "sdsFile::initFileNode::malloc1", 
                          ERR_INSUFFICIENT_MEMORY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       ID_SIZEOF(sdsFileNode),
                                       (void**)&mFileNode ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );

    idlOS::memset( mFileNode, 0x00, ID_SIZEOF(sdsFileNode) );

    mFileNode->mFileHdr.mSmVersion  = smVersionID;
    SM_LSN_INIT( mFileNode->mFileHdr.mRedoLSN );
    SM_LSN_INIT( mFileNode->mFileHdr.mCreateLSN );
    mFileNode->mFileHdr.mFrameCntInExt      = SDS_FILEHDR_IOB_MAX;
    mFileNode->mFileHdr.mLstMetaTableSeqNum = SDS_FILEHDR_SEQ_MAX; 
 
    mFileNode->mState    = SMI_FILE_ONLINE;
    mFileNode->mPageCnt  = aFilePageCnt;
    idlOS::strcpy( mFileNode->mName, aFileName );
    mFileNode->mIsOpened = ID_FALSE;

    if( aFileNode != NULL )
    {
        *aFileNode = mFileNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
} 

/************************************************************************
 * Description : 노드 삭제 
 ************************************************************************/
IDE_RC sdsFile::freeFileNode()
{
    IDE_TEST( mFileNode->mFile.destroy() != IDE_SUCCESS );
    IDE_TEST( iduMemMgr::free( mFileNode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*******************************************************************************
 * Description :
   1. valid 한 node 생성 및 검사 
   2. Valid 한 파일없으면 생성 
 ******************************************************************************/
IDE_RC sdsFile::identify( idvSQL  * aStatistics )
{
    sdsFileNode * sFileNode;
    idBool        sIsValidFileExist = ID_FALSE;
    idBool        sIsCreate         = ID_FALSE;

    getFileNode( &sFileNode );

    IDE_TEST( makeSureValidationNode( aStatistics,
                                      sFileNode, 
                                      &sIsValidFileExist,
                                      &sIsCreate )
              != IDE_SUCCESS );

    if( sIsValidFileExist != ID_TRUE )
    {
        IDE_TEST( create( aStatistics, sIsCreate ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 실제 파일을 생성한다. 
 **********************************************************************/
IDE_RC sdsFile::create( idvSQL * aStatistics, idBool aIsCreate )
{
    sdsFileNode   * sFileNode;
    smLSN           sEndLSN;
    smLSN           sCreateLSN;
    ULong           sFileSize    = 0;
    ULong           sCurrPageCnt = 0;

    getFileNode( &sFileNode );
    IDE_ASSERT( sFileNode != NULL );

    sFileNode->mState = SMI_FILE_ONLINE;

    /* 파일 관리자가 시작되지 않아서 로그 앵커에서 바로 LSN을 읽어온다
       restart중 호출된다. */
    smLayerCallback::getEndLSN( &sEndLSN );
    SM_GET_LSN( sCreateLSN, sEndLSN );

    setFileHdr( &(sFileNode->mFileHdr),
                smLayerCallback::getDiskRedoLSN(),
                &sCreateLSN,
                &mFrameCntInExt,         
                NULL );  /* Sequence Num */

    IDE_TEST( sFileNode->mFile.initialize( 
                         IDU_MEM_SM_SDS,
                         smuProperty::getMaxOpenFDCount4File(),
                         IDU_FIO_STAT_ON,
                         IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FILEIO )
              != IDE_SUCCESS );

    IDE_TEST( sFileNode->mFile.setFileName(sFileNode->mName) != IDE_SUCCESS );

    /* 같은 이름이 있는지 검사 없으면. 만들고 있으면 재활용 */
    if( idf::access( sFileNode->mName, F_OK ) == 0 )
    {
        IDE_TEST( open( aStatistics ) != IDE_SUCCESS );

        IDE_TEST( sFileNode->mFile.getFileSize(&sFileSize) != IDE_SUCCESS );

        sCurrPageCnt = (ULong)((sFileSize - SM_DBFILE_METAHDR_PAGE_SIZE) / 
                       SD_PAGE_SIZE);

        if( sCurrPageCnt > sFileNode->mPageCnt )
        {
            IDE_TEST( sFileNode->mFile.truncate(
                        SDD_CALC_PAGEOFFSET(sFileNode->mPageCnt) )
                    != IDE_SUCCESS );
        }
    }
    else 
    {
        IDE_TEST( sFileNode->mFile.create() != IDE_SUCCESS );

        IDE_TEST( open( aStatistics ) != IDE_SUCCESS );
    }

    IDE_TEST( writeEmptyPages( aStatistics, sFileNode ) != IDE_SUCCESS );
    
    IDE_TEST( writeFileHdr( aStatistics, 
                            sFileNode )
            != IDE_SUCCESS );

    if( aIsCreate == ID_TRUE )
    {
        IDE_ASSERT( smLayerCallback::addSBufferNodeAndFlush( sFileNode )
                    == IDE_SUCCESS );
    }
    else 
    {
        IDE_ASSERT( smLayerCallback::updateSBufferNodeAndFlush( sFileNode )
                    == IDE_SUCCESS );
    } 

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( smERR_ABORT_CannotCreateSecondaryBuffer ) );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그앵커를 읽어서 node 구성 
 **********************************************************************/
IDE_RC sdsFile::load( smiSBufferFileAttr     * aFileAttr,
                      UInt                     aAnchorOffset )
{
    sdsFileNode  * sFileNode;
    UInt           sIOBSize;

    IDE_DASSERT( aFileAttr != NULL );

    IDE_TEST( initFileNode( aFileAttr->mPageCnt,
                            aFileAttr->mName,  
                            &sFileNode )
              != IDE_SUCCESS );
    /* 로드 앵커에서 읽을수 있는 값을 읽어 채움 */
    sFileNode->mState = aFileAttr->mState;
    sFileNode->mFileHdr.mSmVersion = 
        smLayerCallback::getSmVersionFromLogAnchor();
    sFileNode->mAnchorOffset = aAnchorOffset;

    sIOBSize = smuProperty::getBufferIOBufferSize();

    /* 로드앵커의값과 프러퍼티 값으로 노드를 구성하여 
       identify과정중에 fileHdr와 와 비교 한다 */
    setFileHdr( &(sFileNode->mFileHdr),
                smLayerCallback::getDiskRedoLSN(),
                &aFileAttr->mCreateLSN,
                &sIOBSize,/* FrameCntInExt */
                NULL );   /* Sequence Num  */

    IDE_TEST( sFileNode->mFile.initialize( 
                IDU_MEM_SM_SDS,
                smuProperty::getMaxOpenFDCount4File(),
                IDU_FIO_STAT_ON,
                IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FILEIO )
            != IDE_SUCCESS );

    IDE_TEST( sFileNode->mFile.setFileName(sFileNode->mName) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Second Buffer File을 오픈한다.
 **********************************************************************/
IDE_RC sdsFile::open( idvSQL * /*aStatistics*/ )
{
    sdsFileNode  * sFileNode;

    getFileNode( &sFileNode );

    if( sFileNode->mIsOpened == ID_FALSE )
    {
        IDE_TEST( sFileNode->mFile.open( ((smuProperty::getIOType() == 0) ?
                                         ID_FALSE : ID_TRUE))   /* DIRECT_IO */
                  != IDE_SUCCESS );

        sFileNode->mIsOpened = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : close한다.
 **********************************************************************/
IDE_RC sdsFile::close( idvSQL *  /*aStatistics*/ )
{
    sdsFileNode     * sFileNode;

    getFileNode( &sFileNode );

    if( sFileNode->mIsOpened == ID_TRUE )
    {
        sync();
        
        IDE_ASSERT( sFileNode->mFile.close() == IDE_SUCCESS );

        sFileNode->mIsOpened = ID_FALSE;
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : pageoffset부터 readsize만큼 data 판독
 **********************************************************************/
IDE_RC sdsFile::read( idvSQL      * aStatistics,
                      UInt          aFrameID,
                      ULong         aWhere,
                      ULong         aPageCount,
                      UChar       * aBuffer )

{
    sdsFileNode  * sFileNode;
    ULong          sWhere; 
    ULong          sReadByteSize = SD_PAGE_SIZE * aPageCount;

    getFileNode( &sFileNode );

    sWhere = aWhere+SM_DBFILE_METAHDR_PAGE_SIZE;

    IDE_TEST( sFileNode->mFile.read( aStatistics,
                                     sWhere,
                                     (void*)aBuffer,
                                     sReadByteSize,
                                     NULL )  // setEmergency
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Read Page From Data File Failure\n"
                 "               FrameID       = %"ID_UINT32_FMT"",
                 aFrameID );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  pageoffset부터 writesize만큼 data 기록
 **********************************************************************/
IDE_RC sdsFile::write( idvSQL      * aStatistics,
                       UInt          aFrameID,
                       ULong         aWhere,
                       ULong         aPageCount,
                       UChar       * aBuffer )
{
    sdsFileNode  * sFileNode;
    ULong          sWhere; 
    ULong          sWriteByteSize = SD_PAGE_SIZE * aPageCount; 

    getFileNode( &sFileNode);

    sWhere = aWhere+SM_DBFILE_METAHDR_PAGE_SIZE;

    IDE_TEST( sFileNode->mFile.writeUntilSuccess( 
                                 aStatistics,
                                 sWhere,
                                 (void*)aBuffer,
                                 sWriteByteSize,
                                 smLayerCallback::setEmergency ) /* setEmergency */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Write Page To Data File Failure\n"
                 "              PageID       = %"ID_UINT32_FMT"",
                 aFrameID );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : sync한다.
 **********************************************************************/
IDE_RC sdsFile::sync()
{
    sdsFileNode  * sFileNode;

    getFileNode( &sFileNode );

    IDE_TEST( sFileNode->mFile.syncUntilSuccess( smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : checkpoint 시점에 FileHdr의 redoLSN값을 갱신한다.  
 ***********************************************************************/
IDE_RC sdsFile::syncAllSB( idvSQL  * aStatistics )
{
    IDE_TEST( open( aStatistics ) != IDE_SUCCESS );

    IDE_TEST( setAndWriteFileHdr( aStatistics,
                smLayerCallback::getDiskRedoLSN(),  /* aRedoLSN    */
                NULL,                               /* mCreateLSN  */
                NULL,                               /* aFrameCount */
                NULL )                              /* aSeqNum     */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : file create 시 파일 초기화  
 ***********************************************************************/
IDE_RC sdsFile::writeEmptyPages( idvSQL         * aStatistics,
                                 sdsFileNode    * aFileNode )
{
    UChar     * sBufferPtr;
    UChar     * sAlignedBuffer;
    ULong       sUnitCnt;
    ULong       sUnitPageCnt;
    ULong       sUnitNum;
    ULong       sPageCnt;
    ULong       sWriteOffset;
    SInt        sAllocated = 0;
    ULong       i;
    UChar     * sCurPos;
    UInt        sInitialCheckSum;  // 최초 page write시에 저장되는 checksum

    IDE_DASSERT( aFileNode != NULL );

    sUnitPageCnt = smuProperty::getDataFileWriteUnitSize();
    
    sPageCnt = (aFileNode->mPageCnt);

    if( sUnitPageCnt > sPageCnt )
    {
        /* 기록하려는 데이터파일의 크기가 쓰기 단위보다 작으면
         * 쓰기 단위는 데이터파일 크기가 된다. 
         */
        sUnitPageCnt = sPageCnt;
    }
    sUnitNum = sPageCnt / sUnitPageCnt;

    /* CheckSum 계산 */
    for( i = 0, sInitialCheckSum = 0; i < SD_PAGE_SIZE - ID_SIZEOF(UInt); i++ )
    {
        sInitialCheckSum = smuUtility::foldUIntPair(sInitialCheckSum, 0);
    }

    /* 1.버퍼 공간을 할당받는다. */
    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SDS,
                                           sUnitPageCnt * SD_PAGE_SIZE,
                                           (void**)&sBufferPtr,
                                           (void**)&sAlignedBuffer ) 
              != IDE_SUCCESS );
    sAllocated = 1;

    /* 2.버퍼 공간을 초기화한다. */
    /* 2-1. 먼저 첫번째 페이지를 초기화한다.
     */
    idlOS::memset( sAlignedBuffer, 0x00, sUnitPageCnt * SD_PAGE_SIZE );
    idlOS::memcpy( sAlignedBuffer, &sInitialCheckSum, ID_SIZEOF(UInt) );

    /* 2-2. 두번째 페이지부터는 첫번째 페이지를 복사한다. */
    for (i = 1, sCurPos = sAlignedBuffer + SD_PAGE_SIZE;
         i < sUnitPageCnt;
         i++, sCurPos += SD_PAGE_SIZE)
    {
        idlOS::memcpy( sCurPos, sAlignedBuffer, SD_PAGE_SIZE );
    }

    /* 2-3.sUnitPageCnt 단위로 파일에 버퍼를 기록한다 */
    sWriteOffset = SM_DBFILE_METAHDR_PAGE_OFFSET;
    
    for( sUnitCnt = 0; sUnitCnt < sUnitNum ; sUnitCnt++ )
    {
        /* BUG-18138 */
        IDE_TEST( aFileNode->mFile.write( aStatistics,
                                          sWriteOffset,
                                          sAlignedBuffer,
                                          sUnitPageCnt * SD_PAGE_SIZE )
                  != IDE_SUCCESS);

        sWriteOffset += sUnitPageCnt * SD_PAGE_SIZE;

        /* BUG-12754 : session 연결이 끊겼는지 검사함 */
        IDE_TEST( iduCheckSessionEvent(aStatistics) != IDE_SUCCESS );
    }

    /* 2-4.sUnitPageCnt 배수로 초기화 하고 남은 크기 + 1(file header 공간) 을  기록한다. */
    IDE_TEST( aFileNode->mFile.write( 
                       aStatistics,
                       sWriteOffset,
                       sAlignedBuffer,
                       (sPageCnt - (sUnitPageCnt * sUnitCnt) + 1) * SD_PAGE_SIZE )
              != IDE_SUCCESS);

    sAllocated = 0;
    IDE_TEST( iduMemMgr::free( sBufferPtr ) != IDE_SUCCESS );
    sBufferPtr     = NULL;
    sAlignedBuffer = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if( sAllocated == 1 )
        {
            IDE_ASSERT( iduMemMgr::free(sBufferPtr) == IDE_SUCCESS );
            sBufferPtr     = NULL;
            sAlignedBuffer = NULL;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : File Header를 기록한다.
 ***********************************************************************/
void sdsFile::getFileAttr( sdsFileNode          * aFileNode,
                           smiSBufferFileAttr   * aFileAttr )
{
    sdsFileHdr  * sFileHdr;

    IDE_DASSERT( aFileNode != NULL );
    IDE_DASSERT( aFileAttr != NULL );

    sFileHdr = &(aFileNode->mFileHdr);

    aFileAttr->mAttrType = SMI_SBUFFER_ATTR;
    idlOS::strcpy(aFileAttr->mName, aFileNode->mName);
    aFileAttr->mNameLength = idlOS::strlen(aFileNode->mName);
    aFileAttr->mPageCnt = aFileNode->mPageCnt;
    aFileAttr->mState = aFileNode->mState;

    SM_GET_LSN( aFileAttr->mCreateLSN, sFileHdr->mCreateLSN );

    return;
}

/***********************************************************************
 * Description :  주어진 SBuffer File의 FileHdr를 읽어 일부 정보를 반환한다.
 * aLstMetaTableSeqNum [OUT] - 저장된 마지막 Meta Seq Num
 * aRecoveryLSN        [OUT] - 
 * ********************************************************************/
IDE_RC sdsFile::readHdrInfo( idvSQL * aStatistics,
                             UInt   * aLstMetaTableSeqNum,
                             smLSN  * aRecoveryLSN )
{
    SChar       * sHeadPageBuffPtr = NULL;
    SChar       * sHeadPageBuff    = NULL;
    sdsFileNode * sFileNode;
    sdsFileHdr    sFileHdr;

    getFileNode( &sFileNode );
    /* 헤더 페이지를 읽어올 버퍼 생성 */
    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SDS,
                                           SD_PAGE_SIZE,
                                           (void**)&sHeadPageBuffPtr,
                                           (void**)&sHeadPageBuff )
              != IDE_SUCCESS );

    /* File에 대한 IO는 항상 DirectIO를 고려해서 Buffer, Size, Offset을
     * DirectIO Page 크기로 Align시킨다. */
    IDE_TEST( sFileNode->mFile.read( aStatistics,
                                     SM_DBFILE_METAHDR_PAGE_OFFSET,
                                     sHeadPageBuff,
                                     SD_PAGE_SIZE,
                                     NULL /*RealReadSize*/ )
              != IDE_SUCCESS );

    idlOS::memcpy( &sFileHdr, sHeadPageBuff, ID_SIZEOF(sdsFileHdr) );

    *aLstMetaTableSeqNum = sFileHdr.mLstMetaTableSeqNum;
    SM_GET_LSN( *aRecoveryLSN, sFileHdr.mRedoLSN );

    IDE_TEST( iduMemMgr::free( sHeadPageBuffPtr ) != IDE_SUCCESS );
    sHeadPageBuffPtr = NULL;
    sHeadPageBuff = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Read Database File Header Failure\n"
                 "              File Name   = %s\n"
                 "              File Opened = %s",
                 sFileNode->mName,
                 (sFileNode->mIsOpened ? "Open" : "Close") );

    if( sHeadPageBuffPtr != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( sHeadPageBuffPtr ) == IDE_SUCCESS );
        sHeadPageBuffPtr = NULL;
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  주어진 SBuffer File의 FileHdr를 읽는다.
 * aFileHdr [OUT] - 
 * ********************************************************************/
IDE_RC sdsFile::readFileHdr( idvSQL     * aStatistics,
                             sdsFileHdr * aFileHdr )
{
    SChar       * sHeadPageBuffPtr = NULL;
    SChar       * sHeadPageBuff    = NULL;
    sdsFileNode * sFileNode;

    IDE_DASSERT( aFileHdr != NULL );

    getFileNode( &sFileNode );
    /* 헤더 페이지를 읽어올 버퍼 생성 */
    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SDS,
                                           SD_PAGE_SIZE,
                                           (void**)&sHeadPageBuffPtr,
                                           (void**)&sHeadPageBuff )
              != IDE_SUCCESS );

    /* File에 대한 IO는 항상 DirectIO를 고려해서 Buffer, Size, Offset을
     * DirectIO Page 크기로 Align시킨다. */
    IDE_TEST( sFileNode->mFile.read( aStatistics,
                                     SM_DBFILE_METAHDR_PAGE_OFFSET,
                                     sHeadPageBuff,
                                     SD_PAGE_SIZE,
                                     NULL /*RealReadSize*/ )
              != IDE_SUCCESS );

    idlOS::memcpy( aFileHdr, sHeadPageBuff, ID_SIZEOF(sdsFileHdr) );

    IDE_TEST( iduMemMgr::free( sHeadPageBuffPtr ) != IDE_SUCCESS );
    sHeadPageBuffPtr = NULL;
    sHeadPageBuff = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Read Database File Header Failure\n"
                 "              File Name   = %s\n"
                 "              File Opened = %s",
                 sFileNode->mName,
                 (sFileNode->mIsOpened ? "Open" : "Close") );

    if( sHeadPageBuffPtr != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( sHeadPageBuffPtr ) == IDE_SUCCESS );
        sHeadPageBuffPtr = NULL;
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  File Header를 기록한다.
 * aFileNode   - [IN] File node
 **********************************************************************/
IDE_RC sdsFile::writeFileHdr( idvSQL        * aStatistics,
                              sdsFileNode   * aFileNode )
{
    SChar *sHeadPageBuffPtr = NULL;
    SChar *sHeadPageBuff = NULL;

    IDE_DASSERT( aFileNode != NULL );

    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SDS,
                                           SD_PAGE_SIZE,
                                           (void**)&sHeadPageBuffPtr,
                                           (void**)&sHeadPageBuff )
              != IDE_SUCCESS );

    idlOS::memset( sHeadPageBuff, 0x00, SD_PAGE_SIZE );
    idlOS::memcpy( sHeadPageBuff,
                   &( aFileNode->mFileHdr ),
                   ID_SIZEOF( sdsFileHdr ) );

    IDE_TEST( aFileNode->mFile.writeUntilSuccess( aStatistics,
                                                  SM_SBFILE_METAHDR_PAGE_OFFSET,
                                                  sHeadPageBuff,
                                                  SD_PAGE_SIZE,
                                                  smLayerCallback::setEmergency ) 
              != IDE_SUCCESS); 

    IDE_TEST( aFileNode->mFile.syncUntilSuccess( smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::free( sHeadPageBuffPtr ) != IDE_SUCCESS );
    sHeadPageBuffPtr = NULL;
    sHeadPageBuff = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sHeadPageBuffPtr != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( sHeadPageBuffPtr ) == IDE_SUCCESS );
        sHeadPageBuffPtr = NULL;
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : HEADER에 체크포인트 정보를 기록한다.
 * aRedoSN     [IN] 미디어복구에 필요한 Redo SN
 * aCreateLSN  [IN]  
 * aFrameCnt   [IN]
 * aSeqNum     [IN]
 **********************************************************************/
void sdsFile::setFileHdr( sdsFileHdr    * aFileHdr,
                          smLSN         * aRedoLSN,
                          smLSN         * aCreateLSN,
                          UInt          * aFrameCnt,
                          UInt          * aSeqNum )
{
    IDE_ASSERT( aFileHdr != NULL );

    if( aRedoLSN != NULL )
    {
        SM_GET_LSN( aFileHdr->mRedoLSN, *aRedoLSN );
    }
    if( aCreateLSN != NULL )
    {
        SM_GET_LSN( aFileHdr->mCreateLSN, *aCreateLSN );
    }
    if( aFrameCnt != NULL )
    {
        aFileHdr->mFrameCntInExt = *aFrameCnt;
    } 
    if( aSeqNum != NULL )
    {
        aFileHdr->mLstMetaTableSeqNum = *aSeqNum;
    }
    return;
}

/***********************************************************************
 * Description : Header 기록및 파일에 쓰기
 **********************************************************************/
IDE_RC sdsFile::setAndWriteFileHdr( idvSQL * aStatistics,
                                    smLSN  * aRedoLSN,
                                    smLSN  * aCreateLSN,
                                    UInt   * aFrameCount,
                                    UInt   * aSeqNum )
{
    sdsFileNode * sFileNode;
    sdsFileHdr  * sFileHdr;

    getFileNode( &sFileNode );

    sFileHdr = &( sFileNode->mFileHdr );

    setFileHdr( sFileHdr, 
                aRedoLSN, 
                aCreateLSN, 
                aFrameCount, 
                aSeqNum );
    
    IDE_TEST( writeFileHdr( aStatistics,
                            sFileNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 미디어 리커버리를 수행한뒤 RedoLSN 을 당겨주며
                 MetaTableSeqNum 를 초기화를 시켜 준다  
                 MetaTableSeqNum 이 초기화 되어있으면 secondary Buffer를 
                 사용하지 않은것으로 이해하고 BCB를 rebuild 하지 않는다.
 *  aResetLogsLSN  [IN] - 불완전복구시 설정되는 ResetLogsLSN
 *                        완전복구 시에는 NULL
 * ********************************************************************/
IDE_RC sdsFile::repairFailureSBufferHdr( idvSQL * aStatistics, 
                                         smLSN  * aResetLogsLSN )
{
    sdsFileNode  * sFileNode = NULL;
    sdsFileHdr   * sFileHdr  = NULL;

    getFileNode( &sFileNode );

    IDE_TEST_CONT( sFileNode == NULL, SKIP_SUCCESS );

    sFileHdr = &( sFileNode->mFileHdr );

    if( aResetLogsLSN != NULL )
    {
        SM_GET_LSN( sFileHdr->mRedoLSN, *aResetLogsLSN );
    }

    sFileHdr->mLstMetaTableSeqNum = SDS_FILEHDR_SEQ_MAX;

    IDE_TEST( open( aStatistics ) != IDE_SUCCESS );

    IDE_TEST( writeFileHdr( aStatistics,
                            sFileNode )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 사용가능한 파일인지 검사  
 *  aFileNode           - [IN]  Loganchor에서 읽은 FileNode
 *  aCompatibleFrameCnt - [OUT] 
 **********************************************************************/
IDE_RC sdsFile::checkValidationHdr( idvSQL       * aStatistics,
                                    sdsFileNode  * aFileNode,
                                    idBool       * aCompatibleFrameCnt )
{
    UInt              sDBVer;
    UInt              sFileVer;
    sdsFileHdr      * sFileHdrbyNode;
    sdsFileHdr        sFileHdrbyFile;

    IDE_DASSERT( aFileNode       != NULL );

    sFileHdrbyNode = &(aFileNode->mFileHdr);

    IDE_TEST( open( aStatistics ) != IDE_SUCCESS );

    IDE_TEST( readFileHdr( NULL,  &sFileHdrbyFile ) != IDE_SUCCESS );

    IDE_TEST( close( aStatistics ) != IDE_SUCCESS );

    IDE_TEST_RAISE( checkValuesOfHdr( &sFileHdrbyFile ) 
                    != IDE_SUCCESS,
                    ERR_INVALID_HDR );

    sDBVer   = smVersionID & SM_CHECK_VERSION_MASK;
    sFileVer = sFileHdrbyFile.mSmVersion & SM_CHECK_VERSION_MASK;

    /* [1] SM VERSION 확인
     * 데이타파일과 서버 바이너리의 호환성을 검사한다. 
     */
    IDE_TEST_RAISE( sDBVer != sFileVer, ERR_INVALID_HDR );

    /* [2] CREATE SN 확인 
     * 데이타파일의 Create SN은 Loganchor의 Create SN과 동일해야한다.
     * 다르면 파일명만 동일한 완전히 다른 데이타파일이다. 
     */
    IDE_TEST_RAISE( smLayerCallback::isLSNEQ( &(sFileHdrbyFile.mCreateLSN),
                                              &(sFileHdrbyNode->mCreateLSN) ) 
                    != ID_TRUE,
                    ERR_INVALID_HDR );

    /* [3] Redo LSN 확인
     * Loganchor의 RedoLSN이   
     * 파일의 redoLSN보다(체크포인시에 결정된 DRDB Redo LSN)
     * 더 작거나 같아야 한다. 
     */  
    IDE_TEST_RAISE( smLayerCallback::isLSNLTE( &(sFileHdrbyNode->mRedoLSN),
                                               &(sFileHdrbyFile.mRedoLSN) ) 
                    != ID_TRUE,
                    ERR_INVALID_HDR );

    /* [4] IOB확인 
     * IOB는 Property이므로 바뀔수 있으므로 
     * 다르면 죽이지 않고 파일을 재생성 한다.
     */
    if( sFileHdrbyNode->mFrameCntInExt != sFileHdrbyFile.mFrameCntInExt )
    {
        *aCompatibleFrameCnt = ID_FALSE;
    } 


    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_HDR );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidSecondaryBufferHdr) );

        ideLog::log( IDE_SERVER_0,
                     "Find Invalid Secondary Buffer File Header \n"
                     "Secondary Buffer File Header Hdr \n"
                     "Binary DB Version : < %"ID_xINT32_FMT".%"ID_xINT32_FMT".%"ID_xINT32_FMT" >\n"
                     "Create LSN        : <%"ID_UINT32_FMT",%"ID_UINT32_FMT">\n"
                     "Redo LSN          : <%"ID_UINT32_FMT",%"ID_UINT32_FMT">\n"
                     "MetaData Num      : < %"ID_UINT32_FMT">\n"
                     "Last MetaTable Sequence Number : < %"ID_UINT32_FMT" > \n"
                     "Loganchor \n"
                     "Create LSN        : <%"ID_UINT32_FMT",%"ID_UINT32_FMT">\n"
                     "Redo LSN          : <%"ID_UINT32_FMT",%"ID_UINT32_FMT">\n",
                     ( ( sFileHdrbyFile.mSmVersion & SM_MAJOR_VERSION_MASK ) >> 24 ),
                     ( ( sFileHdrbyFile.mSmVersion & SM_MINOR_VERSION_MASK ) >> 16 ),
                     ( sFileHdrbyFile.mSmVersion  & SM_PATCH_VERSION_MASK),
                     sFileHdrbyFile.mCreateLSN.mFileNo,
                     sFileHdrbyFile.mCreateLSN.mOffset,
                     sFileHdrbyFile.mRedoLSN.mFileNo,
                     sFileHdrbyFile.mRedoLSN.mOffset,
                     sFileHdrbyFile.mFrameCntInExt,
                     sFileHdrbyFile.mLstMetaTableSeqNum,
                     sFileHdrbyNode->mCreateLSN.mFileNo,
                     sFileHdrbyNode->mCreateLSN.mOffset,
                     sFileHdrbyNode->mRedoLSN.mFileNo,
                     sFileHdrbyNode->mRedoLSN.mOffset );

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  HEADERD의 유효성을 검사한다.
 * aFileHdr       [IN] sdsFileHdr 타입의 포인터
 **********************************************************************/
IDE_RC sdsFile::checkValuesOfHdr( sdsFileHdr*  aFileHdr )
{
    smLSN sCmpLSN;

    IDE_DASSERT( aFileHdr != NULL );

    SM_LSN_INIT( sCmpLSN );

    IDE_TEST( smLayerCallback::isLSNEQ( &aFileHdr->mRedoLSN, &sCmpLSN ) 
              == ID_TRUE );

    IDE_TEST( smLayerCallback::isLSNEQ( &aFileHdr->mCreateLSN, &sCmpLSN )
               == ID_TRUE );

    SM_LSN_MAX( sCmpLSN );
    IDE_TEST( smLayerCallback::isLSNEQ( &aFileHdr->mRedoLSN, &sCmpLSN ) 
              == ID_TRUE );

    IDE_TEST( smLayerCallback::isLSNEQ( &aFileHdr->mCreateLSN, &sCmpLSN ) 
              == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 상대경로 입력시 절대경로를 만든다
                 BUGBUG
                 sctTableSpaceMgr::makeValidABSPath 참고
                 동일한 기능은 합치는 등 중복된 부분을 줄이는 작업 필요!!!!

  aValidName   - [IN/OUT] 상대경로를 받아서 절대경로로 변경하여 반환
 **********************************************************************/
IDE_RC sdsFile::makeValidABSPath( SChar  * aValidName )
{
#if !defined(VC_WINCE) && !defined(SYMBIAN)
    UInt    i;
    SChar*  sPtr;

    DIR*    sDir;
    UInt    sDirLength;
    UInt    sNameLength;
    SChar   sFilePath[SM_MAX_FILE_NAME];

    SChar*  sPath = NULL;

    IDE_ASSERT( aValidName  != NULL );

    // fix BUG-15502
    IDE_TEST_RAISE( idlOS::strlen(aValidName) == 0,
                    ERR_FILENAME_IS_NULL_STRING );

    sPath = sFilePath;
    sNameLength = idlOS::strlen( aValidName );

    /* ------------------------------------------------
     * datafile 이름에 대한 시스템 예약어 검사
     * ----------------------------------------------*/
#if defined(VC_WIN32)
    SInt  sIterator;
    for ( sIterator = 0; aValidName[sIterator] != '\0'; sIterator++ ) {
        if ( aValidName[sIterator] == '/' ) {
             aValidName[sIterator] = IDL_FILE_SEPARATOR;
        }
    }
#endif

    sPtr = idlOS::strrchr(aValidName, IDL_FILE_SEPARATOR);
    if ( sPtr == NULL )
    {
        sPtr = aValidName; // datafile 명만 존재
    }
    else
    {
        // Do Nothing...
    }

    sPtr = idlOS::strchr(aValidName, IDL_FILE_SEPARATOR);
#ifndef VC_WIN32
    if (sPtr != &aValidName[0])
#else
    /* BUG-38278 invalid datafile path at windows server
     * 윈도우즈 환경에서 '/' 나 '\' 로 시작되는
     * 경로 입력은 오류로 처리한다. */
    IDE_TEST_RAISE( sPtr == &aValidName[0], ERR_INVALID_FILEPATH_ABS );

    if ((aValidName[1] == ':' && sPtr != &aValidName[2]) ||
        (aValidName[1] != ':' && sPtr != &aValidName[0]))
#endif
    {
        /* ------------------------------------------------
         * 상대경로(relative-path)인 경우
         * Disk TBS이면 default disk db dir을
         * Memory TBS이면 home dir ($ALTIBASE_HOME)을
         * 붙여서 절대경로(absolute-path)로 만든다.
         * ----------------------------------------------*/
        sDirLength = idlOS::strlen(aValidName) +
            idlOS::strlen(idp::getHomeDir());

        IDE_TEST_RAISE( sDirLength + 1 > SM_MAX_FILE_NAME,
                        ERR_TOO_LONG_FILEPATH );

        idlOS::snprintf(sPath, SM_MAX_FILE_NAME,
                "%s%c%s",
                idp::getHomeDir(),
                IDL_FILE_SEPARATOR,
                aValidName);

#if defined(VC_WIN32)
    for ( sIterator = 0; sPath[sIterator] != '\0'; sIterator++ ) {
        if ( sPath[sIterator] == '/' ) {
             sPath[sIterator] = IDL_FILE_SEPARATOR;
        }
    }
#endif
        idlOS::strcpy(aValidName, sPath);
        sNameLength = idlOS::strlen(aValidName);

        sPtr = idlOS::strchr(aValidName, IDL_FILE_SEPARATOR);
#ifndef VC_WIN32
        IDE_TEST_RAISE( sPtr != &aValidName[0], ERR_INVALID_FILEPATH_ABS );
#else
        IDE_TEST_RAISE( (aValidName[1] == ':' && sPtr != &aValidName[2]) ||
        (aValidName[1] != ':' && sPtr != &aValidName[0]), ERR_INVALID_FILEPATH_ABS );
#endif
    }

    /* ------------------------------------------------
     * 영문자, 숫자 + '/'는 허용하고 그외 문자는 허용하지 않는다.
     * (절대경로임)
     * ----------------------------------------------*/
    for (i = 0; i < sNameLength; i++)
    {
        if (smuUtility::isAlNum(aValidName[i]) != ID_TRUE)
        {
            /* BUG-16283: Windows에서 Altibase Home이 '(', ')' 가 들어갈
               경우 DB 생성시 오류가 발생합니다. */
            IDE_TEST_RAISE( (aValidName[i] != IDL_FILE_SEPARATOR) &&
                            (aValidName[i] != '-') &&
                            (aValidName[i] != '_') &&
                            (aValidName[i] != '.') &&
                            (aValidName[i] != ':') &&
                            (aValidName[i] != '(') &&
                            (aValidName[i] != ')') &&
                            (aValidName[i] != ' ')
                            ,
                            ERR_INVALID_FILEPATH_KEYWORD );

            if (aValidName[i] == '.')
            {
                if ((i + 1) != sNameLength)
                {
                    IDE_TEST_RAISE( aValidName[i+1] == '.',
                                    ERR_INVALID_FILEPATH_KEYWORD );
                    IDE_TEST_RAISE( aValidName[i+1] == IDL_FILE_SEPARATOR,
                                    ERR_INVALID_FILEPATH_KEYWORD );
                }
            }
        }
    }

    // [BUG-29812] IDL_FILE_SEPARATOR가 한개도 없다면 절대경로가 아니다.
    IDE_TEST_RAISE( (sPtr = idlOS::strrchr(aValidName, IDL_FILE_SEPARATOR) )
                     == NULL,
                     ERR_INVALID_FILEPATH_ABS );

    // [BUG-29812] dir이 존재하는 확인한다.
    idlOS::strncpy(sPath, aValidName, SM_MAX_FILE_NAME);

    sDirLength = sPtr - aValidName;

    sDirLength = (sDirLength == 0) ? 1 : sDirLength;
    sPath[sDirLength] = '\0';

    // fix BUG-19977
    IDE_TEST_RAISE( idf::access(sPath, F_OK) != 0,
                    ERROR_NOT_EXIST_PATH );
    IDE_TEST_RAISE( idf::access(sPath, R_OK) != 0,
                    ERROR_NO_READ_PERM_PATH );
    IDE_TEST_RAISE( idf::access(sPath, W_OK) != 0,
                    ERROR_NO_WRITE_PERM_PATH );
    IDE_TEST_RAISE( idf::access(sPath, X_OK) != 0,
                    ERROR_NO_EXECUTE_PERM_PATH );

    sDir = idf::opendir(sPath);
    IDE_TEST_RAISE( sDir == NULL, 
                    ERR_OPEN_DIR ); /* BUGBUG - ERROR MSG */
    (void)idf::closedir(sDir);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FILENAME_IS_NULL_STRING );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_FileNameIsNullString));
    }
    IDE_EXCEPTION( ERROR_NOT_EXIST_PATH );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath, sPath));
    }
    IDE_EXCEPTION( ERROR_NO_READ_PERM_PATH );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoReadPermFile, sPath));
    }
    IDE_EXCEPTION( ERROR_NO_WRITE_PERM_PATH );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoWritePermFile, sPath));
    }
    IDE_EXCEPTION( ERROR_NO_EXECUTE_PERM_PATH );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExecutePermFile, sPath));
    }
    IDE_EXCEPTION( ERR_INVALID_FILEPATH_ABS );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFilePathABS));
    }
    IDE_EXCEPTION( ERR_INVALID_FILEPATH_KEYWORD );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFilePathKeyWord));
    }
    IDE_EXCEPTION( ERR_OPEN_DIR );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotDir));
    }
    IDE_EXCEPTION( ERR_TOO_LONG_FILEPATH );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TooLongFilePath,
                                sPath,
                                smuProperty::getDefaultDiskDBDir()));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#else
    // Windows CE에서는 파일의 절대경로가 C:로 시작하지 않는다.
    return IDE_SUCCESS;
#endif
}

/******************************************************************************
 * Description : Node가 없으면 생성한다.
                 Node가 있으면 valid한 파일이 존재하는지 확인  
 ******************************************************************************/
IDE_RC sdsFile::makeSureValidationNode( idvSQL         * aStatistics,
                                        sdsFileNode    * aFileNode,
                                        idBool         * aIsValidFileExist,
                                        idBool         * aIsCreate )
{
    SChar           sDir[SMI_MAX_SBUFFER_FILE_NAME_LEN];
    ULong           sFilePageCnt;
    idBool          sCompatibleFrameCnt = ID_TRUE;

    IDE_ASSERT( ( idlOS::strcmp(mSBufferDirName, "") != 0 ) &&
                ( mPageCntInGrp != 0 ) );
 
    /* 파일이름 설정 */
    idlOS::memset( sDir, 0x00, ID_SIZEOF(sDir) );
    idlOS::snprintf( sDir, SMI_MAX_SBUFFER_FILE_NAME_LEN,
                     "%s%c%s.sbf", 
                     mSBufferDirName, 
                     IDL_FILE_SEPARATOR,
                     SDS_SECONDARY_BUFFER_FILE_NAME_PREFIX );

    /* 파일 사이즈는 Secondary Buffer Size + MetaTable 크기 */
    sFilePageCnt = mPageCntInGrp + mPageCntInMetaTable;

    if( aFileNode == NULL ) 
    {
        IDE_TEST( initFileNode( sFilePageCnt, sDir, NULL ) != IDE_SUCCESS );
        *aIsCreate = ID_TRUE; 
    }
    else
    {
        if( (idlOS::strncmp(aFileNode->mName,sDir,idlOS::strlen(sDir)) == 0 )  &&
            (idlOS::strlen(sDir) == idlOS::strlen(aFileNode->mName) ) &&
            (aFileNode->mPageCnt == sFilePageCnt) ) 
        {
            if( idf::access( aFileNode->mName, F_OK) == 0 )
            {        
               IDE_TEST( checkValidationHdr( aStatistics, 
                                             aFileNode,
                                             &sCompatibleFrameCnt ) 
                         != IDE_SUCCESS );

               if( sCompatibleFrameCnt == ID_TRUE )
               {
                   *aIsValidFileExist = ID_TRUE;
               }
            }
            else 
            {   /* 속성은 그대로 이나 파일이 없거나 
                   mFrameCntInExt 가 변한상황이라면 
                   파일을 새로 생성해야 한다. */ 
                aFileNode->mState = SMI_FILE_CREATING;
            }
        } 
        else 
        {   
            /* 경로가 변경이 되었을 경우 : */ 
            if( (idlOS::strncmp(aFileNode->mName,sDir,idlOS::strlen(sDir)) != 0 ) ||
                (idlOS::strlen(sDir) != idlOS::strlen(aFileNode->mName) ) )
            {
                IDE_TEST( makeValidABSPath( sDir ) != IDE_SUCCESS );

                idlOS::strcpy( aFileNode->mName, sDir );
            }
            /* 파일 크기가 바뀐 경우 :  */
            if( aFileNode->mPageCnt != sFilePageCnt )
            {
                aFileNode->mPageCnt = sFilePageCnt;
            }
            aFileNode->mState = SMI_FILE_CREATING;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
