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
 * $Id$
 *
 * Description :
 *
 * - incremental chunk change tracking manager
 *
 **********************************************************************/

#include <sddDef.h>
#include <sct.h>
#include <smu.h>
#include <smi.h>
#include <smr.h>
#include <smrReq.h>

smriCTFileHdrBlock  smriChangeTrackingMgr::mCTFileHdr;
iduFile             smriChangeTrackingMgr::mFile;
smriCTBody        * smriChangeTrackingMgr::mCTBodyPtr[SMRI_CT_MAX_CT_BODY_CNT];
smriCTBody        * smriChangeTrackingMgr::mCTBodyFlushBuffer = NULL;
smriCTHdrState      smriChangeTrackingMgr::mCTHdrState;
iduMutex            smriChangeTrackingMgr::mMutex;
UInt                smriChangeTrackingMgr::mBmpBlockBitmapSize;
UInt                smriChangeTrackingMgr::mCTBodyExtCnt;
UInt                smriChangeTrackingMgr::mCTExtMapSize;
UInt                smriChangeTrackingMgr::mCTBodySize;
UInt                smriChangeTrackingMgr::mCTBodyBlockCnt;
UInt                smriChangeTrackingMgr::mCTBodyBmpExtCnt;
UInt                smriChangeTrackingMgr::mBmpBlockBitCnt;
UInt                smriChangeTrackingMgr::mBmpExtBitCnt;

iduFile           * smriChangeTrackingMgr::mSrcFile;
iduFile           * smriChangeTrackingMgr::mDestFile;
SChar             * smriChangeTrackingMgr::mIBChunkBuffer;
ULong               smriChangeTrackingMgr::mCopySize;
smriCTTBSType       smriChangeTrackingMgr::mTBSType;
UInt                smriChangeTrackingMgr::mIBChunkCnt;
UInt                smriChangeTrackingMgr::mIBChunkID;
UInt                smriChangeTrackingMgr::mFileNo;
scPageID            smriChangeTrackingMgr::mSplitFilePageCount;
UInt                smriChangeTrackingMgr::mCurrChangeTrackingThreadCnt; // change tracking을 수행중인 thread의 수

IDE_RC smriChangeTrackingMgr::initializeStatic()
{
    UInt    i;
    idBool  sFileState   = ID_FALSE;
    idBool  sMutexState  = ID_FALSE;
    idBool  sBufferState = ID_FALSE;

    /*change tracking파일 헤더*/
    idlOS::memset( &mCTFileHdr, 0x00, SMRI_CT_BLOCK_SIZE );

    mCTHdrState = SMRI_CT_MGR_STATE_NORMAL;

    /*CT body에 대한 pointer*/
    for( i = 0; i < SMRI_CT_MAX_CT_BODY_CNT; i++ )
    {
        mCTBodyPtr[i] = NULL;
    }
    
    /* BUG-40716
     * CTBody를 flush할때 사용할 buffer를 미리 할당하여
     * checkpoint시에 CTBody의 메모리 할당 실패로 인해 서버가 죽는 경우 방지
     */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                 1,
                                 ID_SIZEOF( smriCTBody ),
                                 (void**)&mCTBodyFlushBuffer )
              != IDE_SUCCESS );
    sBufferState = ID_TRUE;

    IDE_TEST( mFile.initialize( IDU_MEM_SM_SMR,
                                1,
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sFileState = ID_TRUE;

    IDE_TEST( mMutex.initialize( (SChar*)"CHANGE_TRACKING_MGR_MUTEX",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sMutexState = ID_TRUE;
    
    mSrcFile            = NULL;
    mDestFile           = NULL;
    mIBChunkBuffer      = NULL;
    mCopySize           = 0;
    mTBSType            = SMRI_CT_NONE;
    mIBChunkCnt         = 0;
    mIBChunkID          = ID_UINT_MAX;
    mBmpBlockBitmapSize = smuProperty::getBmpBlockBitmapSize();
    mCTBodyExtCnt       = smuProperty::getCTBodyExtCnt();
    mCTBodyBmpExtCnt    = mCTBodyExtCnt - 2; /* meta ext와 empty ext를 뺀다 */
    mCTExtMapSize       = mCTBodyBmpExtCnt;
    mCTBodySize         = (mCTBodyExtCnt 
                          * SMRI_CT_EXT_BLOCK_CNT 
                          * SMRI_CT_BLOCK_SIZE );
    mCTBodyBlockCnt     = (mCTBodySize / SMRI_CT_BLOCK_SIZE);
    
    /*BmpBlock이 가지는 bitmap의 bit 수*/
    mBmpBlockBitCnt     = (mBmpBlockBitmapSize * SMRI_CT_BIT_CNT);
    /*BmpExt가 가지는 bitmap의 bit 수*/
    mBmpExtBitCnt       = (mBmpBlockBitCnt 
                          * SMRI_CT_EXT_BLOCK_CNT_EXCEPT_META_BLOCK);

    //PROJ-2133 incremental backup
    mCurrChangeTrackingThreadCnt = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
   
    if ( sBufferState == ID_TRUE )
    {
        IDE_ASSERT( iduMemMgr::free( mCTBodyFlushBuffer ) == IDE_SUCCESS );
        mCTBodyFlushBuffer = NULL;
    }

    if( sFileState == ID_TRUE )
    {
        IDE_ASSERT( mFile.destroy() == IDE_SUCCESS );
    }

    if( sMutexState == ID_TRUE )
    {
        IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC smriChangeTrackingMgr::destroyStatic()
{
    idBool  sFileState  = ID_TRUE;
    idBool  sMutexState = ID_TRUE;

    /* BUG-40716 */
    if ( mCTBodyFlushBuffer != NULL )
    {
        IDE_TEST( iduMemMgr::free( mCTBodyFlushBuffer ) != IDE_SUCCESS );
        mCTBodyFlushBuffer = NULL;
    }
     
    IDE_TEST( mFile.destroy() != IDE_SUCCESS );
    sFileState = ID_FALSE;

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );
    sMutexState = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
   
    if ( mCTBodyFlushBuffer != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( mCTBodyFlushBuffer ) == IDE_SUCCESS );
        mCTBodyFlushBuffer = NULL;
    }

    if( sFileState == ID_TRUE )
    {
        IDE_ASSERT( mFile.destroy() == IDE_SUCCESS );
    }
    
    if( sMutexState == ID_TRUE )
    {
        IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * CT(change tracking)파일을 생성한다.
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createCTFile()
{
    smriCTBody        * sCTBody;
    SChar               sFileName[ SM_MAX_FILE_NAME ] = "\0";
    smriCTMgrState      sCTMgrState;
    idBool              sCTBodyState = ID_FALSE;
    smLSN               sFlushLSN;
    UInt                sState = 0;
   
    IDE_ERROR( ID_SIZEOF( smriCTFileHdrBlock )      == SMRI_CT_BLOCK_SIZE );
    IDE_ERROR( ID_SIZEOF( smriCTExtMapBlock )       == SMRI_CT_BLOCK_SIZE );
    IDE_ERROR( ID_SIZEOF( smriCTBmpExtHdrBlock )    == SMRI_CT_BLOCK_SIZE );
    IDE_ERROR( ID_SIZEOF( smriCTBmpBlock )          == SMRI_CT_BLOCK_SIZE );
    IDE_ERROR( ID_SIZEOF( smriCTDataFileDescBlock ) == SMRI_CT_BLOCK_SIZE );

    IDE_TEST( lockCTMgr() != IDE_SUCCESS );
    sState = 1;

    /* change tarcking 기능이 활성화 되어있는지 확인 */
    IDE_TEST_RAISE( smrRecoveryMgr::isCTMgrEnabled() == ID_TRUE, 
                    error_already_enabled_change_tracking );

    /* 파일 이름 설정 */
    idlOS::snprintf( sFileName,                     
                     SM_MAX_FILE_NAME,              
                     "%s%c%s",
                     smuProperty::getDBDir(0),      
                     IDL_FILE_SEPARATOR,            
                     SMRI_CT_FILE_NAME );

    /* 
     * change tracking  파일 생성중 서버 비정상종료 상황
     * 생성중인 파일이 존재하면 삭제하고 새로 만든다
     * */
    if( smrRecoveryMgr::isCreatingCTFile() == ID_TRUE )
    {
        if( idf::access( sFileName, F_OK ) == 0 )
        {
            IDE_TEST( idf::unlink( sFileName ) != IDE_SUCCESS );
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

    /* logAnchor에 CTFile이름과 파일을 생성중이라는 정보를 남김 */
    SM_LSN_INIT( sFlushLSN );
    sCTMgrState = SMRI_CT_MGR_FILE_CREATING;
    IDE_TEST( smrRecoveryMgr::updateCTFileAttrToLogAnchor( sFileName,
                                                           &sCTMgrState,
                                                           NULL ) //LastFlushLSN
              != IDE_SUCCESS );
    

    /* CTMgrHdr 정보 초기화 */
    mCTHdrState  = SMRI_CT_MGR_STATE_NORMAL;
    IDE_TEST( createFileHdrBlock( &mCTFileHdr ) !=IDE_SUCCESS );

    IDE_TEST( mFile.setFileName( sFileName ) != IDE_SUCCESS );

    IDE_TEST( mFile.create() != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( mFile.open() != IDE_SUCCESS );
    sState = 3;

    /*CTFileHdr checksum 계산후 파일로 write */
    setBlockCheckSum( &mCTFileHdr.mBlockHdr );

    IDE_TEST( mFile.write( NULL, //aStatistics
                           SMRI_CT_HDR_OFFSET,
                           &mCTFileHdr,
                           SMRI_CT_BLOCK_SIZE )
              != IDE_SUCCESS );

    sState = 2;
    IDE_TEST( mFile.close() != IDE_SUCCESS );

    /* CTFile Body생성 */
    IDE_TEST( extend( ID_FALSE, /* 확장하고 파일에 기록을 할지 안할지 판단*/
                      &sCTBody ) != IDE_SUCCESS );
    sCTBodyState = ID_TRUE;

    /* 기존에 존재하는 모든 데이터파일을 CTFile에 등록*/
    IDE_TEST( addAllExistingDataFile2CTFile( NULL /*aStatistics*/ ) 
              != IDE_SUCCESS );

    IDE_TEST( flush() != IDE_SUCCESS );

    /* 
     * change tracking파일이 생성 완료되고 change tracking manager사 활성화 됨을
     * 기록으로 남김
     */
    sCTMgrState = SMRI_CT_MGR_ENABLED;
    IDE_TEST( smrRecoveryMgr::updateCTFileAttrToLogAnchor( NULL,  //FileName
                                                           &sCTMgrState,
                                                           NULL ) //LastFlushLSN
              != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( unlockCTMgr() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_enabled_change_tracking );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ChangeTrackingState));
    }

    IDE_EXCEPTION_END;

    if( sCTBodyState == ID_TRUE )
    {
        IDE_ASSERT( unloadCTBody( sCTBody ) == IDE_SUCCESS );
    }

    switch( sState )
    {
        case 3:
            IDE_ASSERT( mFile.close() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( idf::unlink( sFileName ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( unlockCTMgr() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * CT(chang tracking)파일 body를 초기화 한다.
 * 
 * aCTBody    - [IN] changeTracking body
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createCTBody( smriCTBody * aCTBody )
{
    UInt                    sBlockIdx;
    UInt                    sBlockID;
    smriCTBmpExtHdrBlock  * sBmpExtHdrBlock;
    void                  * sBlock;
    UInt                    sBlockIdx4Exception = 0;

    IDE_DASSERT( aCTBody != NULL );
    
    /* CTbody의 첫번째 block의 ID를 구한다.*/
    sBlockID = mCTFileHdr.mCTBodyCNT * mCTBodyBlockCnt;

    for( sBlockIdx = 0; 
         sBlockIdx < mCTBodyBlockCnt; 
         sBlockIdx++, sBlockID++ )
    {
        sBlock = &aCTBody->mBlock[ sBlockIdx ];

        /* Meta Extent 초기화 */
        if( sBlockIdx < SMRI_CT_EXT_BLOCK_CNT )
        {
            if( ( sBlockIdx % SMRI_CT_EXT_BLOCK_CNT ) == 0 )
            {
                IDE_TEST( createExtMapBlock( 
                            (smriCTExtMapBlock *) sBlock,
                            sBlockID )
                        != IDE_SUCCESS );

            }
            else
            {
                IDE_TEST( createDataFileDescBlock(
                            (smriCTDataFileDescBlock *)sBlock,
                            sBlockID )
                        != IDE_SUCCESS );

            }
        }

        /*BmpExt 초기화*/
        if( (sBlockIdx >= SMRI_CT_EXT_BLOCK_CNT) &&
            (sBlockIdx < mCTBodyBlockCnt) )
        {
            /*BmpExtHdr블럭 초기화*/
            if( ( sBlockIdx % SMRI_CT_EXT_BLOCK_CNT ) == 0 )
            {
                IDE_TEST( createBmpExtHdrBlock( 
                           (smriCTBmpExtHdrBlock *)sBlock,
                           sBlockID )
                          != IDE_SUCCESS );
            }
            else /*Bmp블럭 초기화*/
            {
                IDE_TEST( createBmpBlock(
                           (smriCTBmpBlock *)sBlock,
                           sBlockID )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    for( sBlockIdx4Exception = 0; 
         sBlockIdx4Exception < sBlockIdx;
         sBlockIdx4Exception++ )
    {
        IDE_ASSERT( 
            aCTBody->mBlock[ sBlockIdx4Exception ].mBlockHdr.mMutex.destroy() 
            == IDE_SUCCESS );
    
        if( ( sBlockIdx4Exception != SMRI_CT_EXT_MAP_BLOCK_IDX ) &&
            ( ( sBlockIdx4Exception % SMRI_CT_EXT_BLOCK_CNT ) == 0 ) )
        {
            sBmpExtHdrBlock = 
                (smriCTBmpExtHdrBlock*)&aCTBody->mBlock[ sBlockIdx4Exception ];

            IDE_ASSERT( sBmpExtHdrBlock->mLatch.destroy(  ) 
                        == IDE_SUCCESS );
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * CT(change tracking)파일헤더를 초기화 한다.
 * 
 * aCTFileHdrBlock  - [IN] CT파일 헤더 블럭
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createFileHdrBlock( 
                                    smriCTFileHdrBlock * aCTFileHdrBlock )
{
    UInt    sState = 0;
    SChar * sDBName; 

    IDE_DASSERT( aCTFileHdrBlock != NULL );

    IDE_TEST( createBlockHdr( &aCTFileHdrBlock->mBlockHdr,
                              SMRI_CT_FILE_HDR_BLOCK,
                              SMRI_CT_FILE_HDR_BLOCK_ID )
              != IDE_SUCCESS );

    aCTFileHdrBlock->mCTBodyCNT     = 0;
    aCTFileHdrBlock->mIBChunkSize   = 
                                smuProperty::getIncrementalBackupChunkSize();

    SM_LSN_INIT( aCTFileHdrBlock->mLastFlushLSN );

    IDE_TEST(aCTFileHdrBlock->mFlushCV.initialize((SChar *)"CT_BODY_FLUSH_COND") 
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST(aCTFileHdrBlock->mExtendCV.initialize((SChar *)"CT_BODY_EXTEND_COND") 
              != IDE_SUCCESS );
    sState = 2;

    sDBName = smmDatabase::getDBName();

    idlOS::strncpy( aCTFileHdrBlock->mDBName, sDBName, SM_MAX_DB_NAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( aCTFileHdrBlock->mExtendCV.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( aCTFileHdrBlock->mFlushCV.destroy() == IDE_SUCCESS );
        default:
            break;
    
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * ExtMap블럭을 초기화 한다.
 * 
 * aExtMapBlock - [IN] ExtMap Block의 Ptr
 * aBlockID     - [IN] 블럭 ID
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createExtMapBlock( 
                                   smriCTExtMapBlock * aExtMapBlock,
                                   UInt                aBlockID )
{
    IDE_DASSERT( aExtMapBlock != NULL );
    IDE_DASSERT( aBlockID     != SMRI_CT_INVALID_BLOCK_ID );

    IDE_TEST( createBlockHdr( &aExtMapBlock->mBlockHdr,
                              SMRI_CT_EXT_MAP_BLOCK,
                              aBlockID )
              != IDE_SUCCESS );

    aExtMapBlock->mCTBodyID = aBlockID / mCTBodyBlockCnt;

    idlOS::memset( aExtMapBlock->mExtentMap,
                   0,
                   mCTExtMapSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * dataFileDesc블럭을 초기화 한다.
 * 
 * adataFileDescBlock   - [IN] dataFileDesc block의 Ptr
 * aBlockID             - [IN] 블럭 ID
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createDataFileDescBlock(
                        smriCTDataFileDescBlock * aDataFileDescBlock,
                        UInt                      aBlockID )
{
    UInt    sSlotIdx;

    IDE_DASSERT( aDataFileDescBlock  != NULL );
    IDE_DASSERT( aBlockID            != SMRI_CT_INVALID_BLOCK_ID );

    IDE_TEST( createBlockHdr( &aDataFileDescBlock->mBlockHdr,
                              SMRI_CT_DATAFILE_DESC_BLOCK,
                              aBlockID )
              != IDE_SUCCESS );

    aDataFileDescBlock->mAllocSlotFlag = 0;
    
    for( sSlotIdx = 0; sSlotIdx < SMRI_CT_DATAFILE_DESC_SLOT_CNT; sSlotIdx++ )
    {
        IDE_TEST( createDataFileDescSlot( 
                                &aDataFileDescBlock->mSlot[ sSlotIdx ],
                                aBlockID,
                                sSlotIdx )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * DataFileDesc slot을 초기화 한다.
 * 
 * aDataFileDescSlot    - [IN] DataFiledescSlot의 Ptr
 * aBlockID             - [IN] 블럭 ID
 * aSlotIdx             - [IN] Slot의 index번호
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createDataFileDescSlot(
                        smriCTDataFileDescSlot * aDataFileDescSlot,
                        UInt                     aBlockID,
                        UInt                     aSlotIdx )
{
    UInt    sBmpExtListIdx;

    IDE_DASSERT( aDataFileDescSlot != NULL );
    IDE_DASSERT( aBlockID          != SMRI_CT_INVALID_BLOCK_ID );
    IDE_DASSERT( aSlotIdx          != SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX );

    aDataFileDescSlot->mSlotID.mBlockID     = aBlockID;
    aDataFileDescSlot->mSlotID.mSlotIdx     = aSlotIdx;
    aDataFileDescSlot->mTrackingState       = SMRI_CT_TRACKING_DEACTIVE;
    aDataFileDescSlot->mTBSType             = SMRI_CT_NONE;
    aDataFileDescSlot->mPageSize            = 0;
    aDataFileDescSlot->mBmpExtCnt           = 0;
    aDataFileDescSlot->mCurTrackingListID   = SMRI_CT_DIFFERENTIAL_LIST0;
    aDataFileDescSlot->mFileID              = 0;
    aDataFileDescSlot->mSpaceID             = SC_NULL_SPACEID;

    for( sBmpExtListIdx = 0; 
         sBmpExtListIdx < SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT; 
         sBmpExtListIdx++ ) 
    {

        idlOS::memset( 
              &aDataFileDescSlot->mBmpExtList[ sBmpExtListIdx ].mBmpExtListHint,
              SMRI_CT_INVALID_BLOCK_ID,
              ID_SIZEOF( UInt ) * SMRI_CT_BMP_EXT_LIST_HINT_CNT );

        aDataFileDescSlot->mBmpExtList[ sBmpExtListIdx ].mSetBitCount = 0;
    }


    return IDE_SUCCESS;
}

/***********************************************************************
 * BmpExtHdrBlock을 초기화 한다.
 * 
 * aBmpExtHdrBlock  - [IN] BmpExtHdrBlock의 Ptr
 * aBlockID         - [IN] 블럭 ID
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createBmpExtHdrBlock( 
                                        smriCTBmpExtHdrBlock * aBmpExtHdrBlock,
                                        UInt                   aBlockID )
{
    SChar   sLatchName[128];
    UInt    sState = 0;

    IDE_DASSERT( aBmpExtHdrBlock != NULL );
    IDE_DASSERT( aBlockID        != SMRI_CT_INVALID_BLOCK_ID );
    
    IDE_TEST( createBlockHdr( &aBmpExtHdrBlock->mBlockHdr,
                              SMRI_CT_BMP_EXT_HDR_BLOCK,
                              aBlockID )
              != IDE_SUCCESS );

    aBmpExtHdrBlock->mPrevBmpExtHdrBlockID        = SMRI_CT_INVALID_BLOCK_ID;
    aBmpExtHdrBlock->mNextBmpExtHdrBlockID        = SMRI_CT_INVALID_BLOCK_ID;
    aBmpExtHdrBlock->mCumBmpExtHdrBlockID         = SMRI_CT_INVALID_BLOCK_ID;
    aBmpExtHdrBlock->mType                        = SMRI_CT_FREE_BMP_EXT;
    aBmpExtHdrBlock->mDataFileDescSlotID.mBlockID = SMRI_CT_INVALID_BLOCK_ID;
    aBmpExtHdrBlock->mDataFileDescSlotID.mSlotIdx = 
                                        SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;
    aBmpExtHdrBlock->mBmpExtSeq                   = 0;
    SM_LSN_INIT(aBmpExtHdrBlock->mFlushLSN);
    
    idlOS::snprintf( sLatchName,
                     ID_SIZEOF(sLatchName),
                     "BMP_EXT_HDR_LATCH_%"ID_UINT32_FMT,
                     aBlockID % SMRI_CT_EXT_BLOCK_CNT );

    IDE_TEST( aBmpExtHdrBlock->mLatch.initialize( sLatchName )
              != IDE_SUCCESS );
    sState = 1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if( sState == 1 )
    {
        IDE_ASSERT( aBmpExtHdrBlock->mLatch.destroy( ) 
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * BmpBlock을 초기화 한다.
 * 
 * aBmpBlock  - [IN] BmpBlock의 Ptr
 * aBlockID   - [IN] 블럭 ID
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createBmpBlock( 
                                smriCTBmpBlock * aBmpBlock,
                                UInt             aBlockID )
{
    IDE_DASSERT( aBmpBlock   != NULL );
    IDE_DASSERT( aBlockID    != SMRI_CT_INVALID_BLOCK_ID );
    
    IDE_TEST( createBlockHdr( &aBmpBlock->mBlockHdr,
                              SMRI_CT_BMP_BLOCK,
                              aBlockID )
              != IDE_SUCCESS );

    idlOS::memset( aBmpBlock->mBitmap, 
                   0, 
                   mBmpBlockBitmapSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * 블럭헤더를 초기화 한다.
 * 
 * aBlockHdr    - [IN] 블럭헤더
 * aBlockType   - [IN] 블럭종류
 * aBlockID     - [IN] 블럭 ID
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::createBlockHdr( 
                                  smriCTBlockHdr    * aBlockHdr,
                                  smriCTBlockType     aBlockType,
                                  UInt                aBlockID )
{
    SChar   sMutexName[128];
    UInt    sState = 0;

    idlOS::snprintf( sMutexName,
                     ID_SIZEOF(sMutexName),
                     "CT_BLOCK_MUTEX_%"ID_UINT32_FMT,
                     aBlockID );

    /*
     * BUG-34125 Posix mutex must be used for cond_timedwait(), cond_wait().
     */
    IDE_TEST( aBlockHdr->mMutex.initialize( sMutexName,
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    aBlockHdr->mCheckSum        = 0;
    aBlockHdr->mBlockType       = aBlockType;
    aBlockHdr->mBlockID         = aBlockID;
    aBlockHdr->mExtHdrBlockID   = aBlockID & SMRI_CT_EXT_HDR_ID_MASK;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
 
    if(sState == 1 )
    {
        IDE_ASSERT( aBlockHdr->mMutex.destroy() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * 이미 존재하는 모든 DataFile들을 CT파일에 등록한다.
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::addAllExistingDataFile2CTFile( 
                                                    idvSQL * aStatistics )
{
    UInt    sState = 0;

    // XXX 왜 필요하지? 일단 놔두자
    IDE_TEST( sctTableSpaceMgr::lockForCrtTBS() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( aStatistics,
                                                  addExistingDataFile2CTFile,
                                                  NULL,  //aActionArg
                                                  SCT_ACT_MODE_LATCH )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sctTableSpaceMgr::unlockForCrtTBS() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * 이미 존재하는 모든 DataFile들을 CT파일에 등록한다.
 * 
 * aStatistics  - [IN] 
 * aSpaceNode   - [IN]  tableSpace Node
 * aActionAtg   - [IN]  action함수에 전달될 인자
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::addExistingDataFile2CTFile( 
                                        idvSQL                    * aStatistics, 
                                        sctTableSpaceNode         * aSpaceNode, 
                                        void                      */* aActionArg*/ )
{
    UInt                        i;
    smiDataFileDescSlotID     * sSlotID;
    smmDatabaseFile           * sDatabaseFile0;
    smmDatabaseFile           * sDatabaseFile1;
    sddDataFileNode           * sDataFileNode = NULL;
    sddTableSpaceNode         * sDiskSpaceNode;
    smmTBSNode                * sMemSpaceNode;
    smmChkptImageAttr           sChkptImageAttr;

    IDE_DASSERT( aSpaceNode != NULL );

    if( sctTableSpaceMgr::isMemTableSpace( aSpaceNode->mID ) == ID_TRUE )
    {
        sMemSpaceNode = (smmTBSNode *)aSpaceNode;        

        for( i = 0; i <= sMemSpaceNode->mLstCreatedDBFile; i++ )
        {
            IDE_TEST( smmManager::openAndGetDBFile( sMemSpaceNode,
                                          0,
                                          i,
                                          &sDatabaseFile0 )
                      != IDE_SUCCESS );

            IDE_TEST( smmManager::openAndGetDBFile( sMemSpaceNode,
                                          1,
                                          i,
                                          &sDatabaseFile1 )
                      != IDE_SUCCESS );

            /* DataFileDescSlot 할당 */
            IDE_TEST( addDataFile2CTFile( aSpaceNode->mID,
                                          i,
                                          SMRI_CT_MEM_TBS,
                                          &sSlotID )
                      != IDE_SUCCESS );
    
            /* ChkptInageHdr에 slotID 저장 */
            sDatabaseFile0->setChkptImageHdr( NULL, //MemRedoLSN
                                              NULL, //MemCreateLSN
                                              NULL, //SpaceID
                                              NULL, //SmVersion
                                              sSlotID ); 

            IDE_TEST( sDatabaseFile0->flushDBFileHdr() != IDE_SUCCESS );

            /* ChkptInageHdr에 slotID 저장 */
            sDatabaseFile1->setChkptImageHdr( NULL, //MemRedoLSN
                                              NULL, //MemCreateLSN
                                              NULL, //SpaceID
                                              NULL, //SmVersion
                                              sSlotID );

            IDE_TEST( sDatabaseFile1->flushDBFileHdr() != IDE_SUCCESS );

            sDatabaseFile0->getChkptImageAttr( sMemSpaceNode, 
                                               &sChkptImageAttr );
        
            /* logAnchor에 slotID저장 */
            IDE_ASSERT( smrRecoveryMgr::updateChkptImageAttrToAnchor( 
                                            &(sMemSpaceNode->mCrtDBFileInfo[i]),
                                            &sChkptImageAttr )
                        == IDE_SUCCESS );
            
        }
    }
    else 
    {
        if( ( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE ) &&
            ( sctTableSpaceMgr::isTempTableSpace( aSpaceNode ) != ID_TRUE ) )
        {
            sDiskSpaceNode = (sddTableSpaceNode *)aSpaceNode;
            
            for( i = 0; i < sDiskSpaceNode->mNewFileID; i++ )
            {
                sDataFileNode = sDiskSpaceNode->mFileNodeArr[i];
 
                if( sDataFileNode == NULL)     
                {
                    continue;
                }
 
                if( SMI_FILE_STATE_IS_DROPPED( sDataFileNode->mState ) )
                {
                    continue;
                }
 
                /* DataFileDescSlot 할당 */
                IDE_TEST( addDataFile2CTFile( aSpaceNode->mID,
                                              i,
                                              SMRI_CT_DISK_TBS,
                                              &sSlotID )
                          != IDE_SUCCESS );
                
                /* DataFileHdr에 slotID 저장 */
                sDataFileNode->mDBFileHdr.mDataFileDescSlotID.mBlockID 
                                                            = sSlotID->mBlockID;
                sDataFileNode->mDBFileHdr.mDataFileDescSlotID.mSlotIdx 
                                                            = sSlotID->mSlotIdx;
 
                IDE_TEST( sddDiskMgr::writeDBFHdr( aStatistics,
                                                   sDataFileNode ) 
                          != IDE_SUCCESS );
 
                /* logAnchor에 slotID저장 */
                IDE_ASSERT( smrRecoveryMgr::updateDBFNodeToAnchor( 
                                                        sDataFileNode )
                            == IDE_SUCCESS );
            }
        }
        else
        {
            /* nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * CT(change tracking)파일을 삭재한다.
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::removeCTFile()
{
    SChar               sNULLFileName[ SM_MAX_FILE_NAME ] = "\0";
    SChar               sFileName[ SM_MAX_FILE_NAME ];
    smriCTMgrState      sCTMgrState;
    smLSN               sFlushLSN;
    smrLogAnchorMgr     sAnchorMgr4ProcessPhase;
    smrLogAnchorMgr   * sAnchorMgr;
    UInt                sCTBodyCnt;
    UInt                sMutexState     = 0;
    UInt                sMutexlockState = 0;
    UInt                sState          = 0;
    idBool              sLockState      = ID_FALSE;
    idBool              sIsCTMgrEnable = ID_FALSE;
    
    IDE_TEST( lockCTMgr() != IDE_SUCCESS );
    sLockState = ID_TRUE;

    if( smiGetStartupPhase() == SMI_STARTUP_PROCESS )
    {
        IDE_TEST( sAnchorMgr4ProcessPhase.initialize4ProcessPhase() 
                  != IDE_SUCCESS ); 
        sState = 1;

        sAnchorMgr = &sAnchorMgr4ProcessPhase;

        IDE_TEST_RAISE(sAnchorMgr->getCTMgrState() != SMRI_CT_MGR_ENABLED,
                       err_change_traking_state); 

        idlOS::strncpy( sFileName, 
                        sAnchorMgr->getCTFileName(), 
                        SM_MAX_FILE_NAME );

        /* logAnchor에 change tracking파일 상태 갱신 */
        SM_LSN_INIT( sFlushLSN );
        sCTMgrState = SMRI_CT_MGR_DISABLED;
        IDE_TEST( sAnchorMgr->updateCTFileAttr( sNULLFileName,
                                                &sCTMgrState,                  
                                                &sFlushLSN )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sAnchorMgr->destroy() != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE(smrRecoveryMgr::isCTMgrEnabled() != ID_TRUE,
                       err_change_traking_state);
        sIsCTMgrEnable = ID_TRUE;
        sMutexState = 1;
        
        IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/) 
                  != IDE_SUCCESS ); 
        sMutexlockState = 1;

        IDE_TEST( wait4FlushAndExtend() != IDE_SUCCESS );

        sAnchorMgr = smrRecoveryMgr::getLogAnchorMgr();
 
        /* logAnchor에 change tracking파일 상태 갱신 */
        SM_LSN_INIT( sFlushLSN );
        sCTMgrState = SMRI_CT_MGR_DISABLED;
        IDE_TEST( sAnchorMgr->updateCTFileAttr( sNULLFileName,
                                                &sCTMgrState,                  
                                                &sFlushLSN )
                  != IDE_SUCCESS );
        
        sMutexlockState = 0;
        IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() 
                  != IDE_SUCCESS ); 
    
        idlOS::strncpy( sFileName, 
                        mFile.getFileName(), 
                        SM_MAX_FILE_NAME );

        // changeTracking을 수행중인 thread가 있다면 완료될때 까지 대기한다.
        while( getCurrChangeTrackingThreadCnt() != 0 )
        {
            idlOS::sleep(1);
        } 
 
        for( sCTBodyCnt = 0; sCTBodyCnt < mCTFileHdr.mCTBodyCNT; sCTBodyCnt++ )
        {
            IDE_TEST( unloadCTBody( mCTBodyPtr[ sCTBodyCnt ] ) != IDE_SUCCESS );
        }

        sMutexState = 0;
        IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.destroy() != IDE_SUCCESS );
 
        idlOS::memset( &mCTFileHdr, 0, SMRI_CT_BLOCK_SIZE );
    }

    /* 파일을 삭제 한다. */
    if( idf::access( sFileName, F_OK ) == 0 )
    {
        IDE_TEST( idf::unlink( sFileName ) != IDE_SUCCESS );
    }

    sLockState = ID_FALSE;
    IDE_TEST( unlockCTMgr() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_change_traking_state );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ChangeTrackingState));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( sAnchorMgr->destroy() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    if( sMutexlockState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS ); 
    }

    switch( sMutexState )
    {
        case 1:
            if( sIsCTMgrEnable == ID_TRUE )
            {
                IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.destroy() 
                            == IDE_SUCCESS );
            }
        case 0:
            idlOS::memset( &mCTFileHdr, 0, SMRI_CT_BLOCK_SIZE );
        default:
            break;
    }
    
    if( sLockState == ID_TRUE )
    {
        IDE_ASSERT( unlockCTMgr() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * ct(change tracking)manager를 초기화한다.
 * startup control 단계에서 호출된다.
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::begin()
{
    SChar             * sFileName;
    SChar               sMutexName[128] = "\0";
    smLSN               sLastFlushLSN;
    UInt                sLoadCTBodyCNT  = 0;
    ULong               sCTBodyOffset;
    UInt                sState          = 0;
    UInt                sCTBodyIdx      = 0;

    mCTHdrState  = SMRI_CT_MGR_STATE_NORMAL;

    /* logAnchor에서 파일이름을 가져온다. */
    sFileName = smrRecoveryMgr::getCTFileName();

    IDE_TEST_RAISE( idf::access( sFileName, F_OK ) != 0,
                    error_not_exist_change_tracking_file );

    IDE_TEST( mFile.setFileName( sFileName ) != IDE_SUCCESS );

    IDE_TEST( mFile.open() != IDE_SUCCESS );
    sState = 1;

    /* change tracking파일의 헤더블럭을 읽어옴 */
    IDE_TEST( mFile.read( NULL,  //aStatistics
                          SMRI_CT_HDR_OFFSET,
                          (void *)&mCTFileHdr,
                          SMRI_CT_BLOCK_SIZE,
                          NULL ) //ReadSize
              != IDE_SUCCESS );

    IDE_TEST( checkBlockCheckSum( &mCTFileHdr.mBlockHdr ) != IDE_SUCCESS );

    sLastFlushLSN = smrRecoveryMgr::getCTFileLastFlushLSNFromLogAnchor();

    /* 파일헤더에 저장된 LSN과 logAnchor에 저장된 LSN을 비교하여 valid한
     * change tracking파일인지 검사
     */
    IDE_TEST_RAISE( smrCompareLSN::isEQ( &mCTFileHdr.mLastFlushLSN, 
                                         &sLastFlushLSN ) != ID_TRUE,
                    error_invalid_change_tracking_file );
    
    idlOS::snprintf( sMutexName,
                     ID_SIZEOF(sMutexName),
                     "CT_BLOCK_MUTEX_%"ID_UINT32_FMT,
                     SMRI_CT_INVALID_BLOCK_ID );
    
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.initialize( sMutexName,
                                                      IDU_MUTEX_KIND_POSIX,
                                                      IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST(mCTFileHdr.mFlushCV.initialize((SChar *)"CT_BODY_FLUSH_COND") != IDE_SUCCESS);
    sState = 3;

    IDE_TEST(mCTFileHdr.mExtendCV.initialize((SChar *)"CT_BODY_EXTEND_COND") != IDE_SUCCESS);
    sState = 4;
    
    idlOS::memset( &mCTBodyPtr, 0x00, SMRI_CT_MAX_CT_BODY_CNT );

    /* 
     * CTBody를 메모리로 읽어 온다.
     * 메모리 할당, checksum검사, mutex, latch초기화
     */
    for( sLoadCTBodyCNT = 0, sCTBodyOffset = SMRI_CT_BLOCK_SIZE; 
         sLoadCTBodyCNT < mCTFileHdr.mCTBodyCNT; 
         sLoadCTBodyCNT++, sCTBodyOffset += mCTBodySize )
    {
        IDE_TEST( loadCTBody( sCTBodyOffset, sLoadCTBodyCNT ) != IDE_SUCCESS );
    }

    IDE_TEST( validateCTFile() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( mFile.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_exist_change_tracking_file );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileByPath, sFileName ) );
    }

    IDE_EXCEPTION( error_invalid_change_tracking_file );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidChangeTrackingFile,sFileName ) );
    }
    IDE_EXCEPTION_END;

    for( sCTBodyIdx = 0; 
         sCTBodyIdx < sLoadCTBodyCNT ; 
         sCTBodyIdx++ )
    {
        IDE_ASSERT( unloadCTBody( mCTBodyPtr[ sCTBodyIdx ] ) 
                    == IDE_SUCCESS );
    }

    switch( sState )
    {
        case 4:
            IDE_ASSERT( mCTFileHdr.mExtendCV.destroy() == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( mCTFileHdr.mFlushCV.destroy() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.destroy() 
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mFile.close() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * ct(change tracking)Body를 초기화한다.
 *
 * aCTBodyOffset - [IN] 읽어올 body의 파일내의 offset
 * aCTBodyIdx    - [IN] 읽어올 bpdy의 idx
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::loadCTBody( ULong aCTBodyOffset, UInt aCTBodyIdx )
{
    smriCTBody    * sCTBody;
    UInt            sState = 0;

    /* smriChangeTrackingMgr_loadCTBody_calloc_CTBody.tc */
    IDU_FIT_POINT("smriChangeTrackingMgr::loadCTBody::calloc::CTBody");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                 1,
                                 ID_SIZEOF( smriCTBody ),
                                 (void**)&sCTBody )
              != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( mFile.read( NULL,  //aStatistics
                          aCTBodyOffset,
                          (void *)sCTBody,
                          mCTBodySize,
                          NULL ) //ReadSize
              != IDE_SUCCESS );

    sCTBody->mBlock = (smriCTBmpBlock *)sCTBody;
    
    IDE_TEST_RAISE( checkCTBodyCheckSum( sCTBody, mCTFileHdr.mLastFlushLSN )
                    != IDE_SUCCESS,
                    error_invalid_change_tracking_file );
    
    IDE_TEST( initBlockMutex( sCTBody ) != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( initBmpExtLatch( sCTBody ) != IDE_SUCCESS );
    sState = 3;

    mCTBodyPtr[ aCTBodyIdx ]  = sCTBody;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( error_invalid_change_tracking_file );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidChangeTrackingFile, 
                mFile.getFileName()));
    }

    IDE_EXCEPTION_END;
    
    switch( sState )
    {
        case 3:
            IDE_ASSERT( destroyBmpExtLatch( sCTBody ) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( destroyBlockMutex( sCTBody ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( sCTBody ) == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * ct(change tracking)파일의 body를 검사한다.
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::validateCTFile()
{
    UInt                        sAllocSlotFlag;
    UInt                        sCTBodyIdx;
    UInt                        sSlotIdx;
    UInt                        sDataFileDescBlockIdx;
    smriCTBody                * sCTBody;
    smriCTDataFileDescBlock   * sDataFileDescBlockArea;
    smriCTDataFileDescBlock   * sDataFileDescBlock;
    smriCTDataFileDescSlot    * sSlotArea;
    smriCTDataFileDescSlot    * sSlot;

    for( sCTBodyIdx = 0; sCTBodyIdx < mCTFileHdr.mCTBodyCNT; sCTBodyIdx++ )
    {
        sCTBody = mCTBodyPtr[ sCTBodyIdx ];

        IDE_DASSERT( sCTBody != NULL );

        sDataFileDescBlockArea = sCTBody->mMetaExtent.mDataFileDescBlock;
        
        for( sDataFileDescBlockIdx = 0; 
             sDataFileDescBlockIdx < SMRI_CT_DATAFILE_DESC_BLOCK_CNT; 
             sDataFileDescBlockIdx++ )
        {
            sDataFileDescBlock = &sDataFileDescBlockArea[ sDataFileDescBlockIdx ];

            IDE_ERROR( sDataFileDescBlock != NULL );

            sSlotArea = sDataFileDescBlock->mSlot;

            for( sSlotIdx = 0; 
                 sSlotIdx < SMRI_CT_DATAFILE_DESC_SLOT_CNT; 
                 sSlotIdx++ )
            {
                sAllocSlotFlag = (1 << sSlotIdx);
                if( (sDataFileDescBlock->mAllocSlotFlag & sAllocSlotFlag) != 0 )
                {
                    sSlot = &sSlotArea[ sSlotIdx ];

                    IDE_ERROR( sSlot->mBmpExtCnt % 
                               SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT == 0 );

                    IDE_ERROR( validateBmpExtList( 
                                    sSlot->mBmpExtList,
                                    (sSlot->mBmpExtCnt / 
                                     SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT) ) 
                               == IDE_SUCCESS ); 
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * ct(change tracking)파일의 DataFileDesc slot에
 * 할당된 BmpExtList를 검사한다.
 * 
 * aBmpExtList      - [IN] BmpExtList
 * aBmpExtListLen   - [IN] BmpExtList의 길이
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::validateBmpExtList( 
                                        smriCTBmpExtList * aBmpExtList,
                                        UInt               aBmpExtListLen )
{
    smriCTBmpExtHdrBlock    * sBmpExtHdrBlock;
    smriCTBmpExtList        * sBmpExtList;
    UInt                      sFirstBmpExtBlockID;
    UInt                      sBmpExtListIdx;
    UInt                      sBmpExtListLen = 0;

    IDE_DASSERT( aBmpExtList != NULL );

    for( sBmpExtListIdx = 0; 
         sBmpExtListIdx < SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT; 
         sBmpExtListIdx++ )
    {
        sBmpExtListLen      = 0;
        sBmpExtList         = &aBmpExtList[ sBmpExtListIdx ];
        sFirstBmpExtBlockID = sBmpExtList->mBmpExtListHint[0];

        IDE_ERROR( sFirstBmpExtBlockID != SMRI_CT_INVALID_BLOCK_ID );

        getBlock( sFirstBmpExtBlockID, (void**)&sBmpExtHdrBlock );

        sBmpExtListLen++;

        while( sFirstBmpExtBlockID != sBmpExtHdrBlock->mNextBmpExtHdrBlockID )
        {
            IDE_ERROR( ( sBmpExtHdrBlock->mNextBmpExtHdrBlockID != 
                         SMRI_CT_INVALID_BLOCK_ID ) ||
                       ( sBmpExtHdrBlock->mPrevBmpExtHdrBlockID != 
                         SMRI_CT_INVALID_BLOCK_ID ) );

            getBlock( sBmpExtHdrBlock->mNextBmpExtHdrBlockID, 
                                (void**)&sBmpExtHdrBlock );

            sBmpExtListLen++;

            if( sBmpExtListLen < SMRI_CT_BMP_EXT_LIST_HINT_CNT )
            {
                IDE_ERROR( sBmpExtHdrBlock->mBlockHdr.mBlockID == 
                           sBmpExtList->mBmpExtListHint[ sBmpExtListLen - 1 ] );
            }
        }

        IDE_ERROR( sBmpExtListLen == aBmpExtListLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * CT(change tracking)manager를 파괴한다.
 * 서버 종료시에 호출된다.
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::end()
{
    UInt            sCTBodyCnt;
    UInt            sState = 1;
    
    // changeTracking을 수행중인 thread가 있다면 완료될때 까지 대기한다.
    while( getCurrChangeTrackingThreadCnt() != 0 )
    {
        idlOS::sleep(1);
    } 

    for( sCTBodyCnt = 0; sCTBodyCnt < mCTFileHdr.mCTBodyCNT; sCTBodyCnt++ )
    {
        IDE_TEST( unloadCTBody( mCTBodyPtr[ sCTBodyCnt ] ) != IDE_SUCCESS );
    }
     
    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.destroy() != IDE_SUCCESS );

    idlOS::memset( &mCTFileHdr, 0, SMRI_CT_BLOCK_SIZE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.destroy() == IDE_SUCCESS );
        case 0:
            idlOS::memset( &mCTFileHdr, 0, SMRI_CT_BLOCK_SIZE );
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * CT(change tracking)body를 파괴한다.
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::unloadCTBody( smriCTBody * aCTBody )
{
    UInt    sState;

    IDE_DASSERT( aCTBody != NULL );

    sState = 2;
    IDE_TEST( destroyBlockMutex( aCTBody ) != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( destroyBmpExtLatch( aCTBody ) != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( iduMemMgr::free( aCTBody ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( destroyBmpExtLatch( aCTBody ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( aCTBody ) == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Block mutex를 초기화 한다.
 * 
 * aCTBody  - [IN] CTBody의 ptr
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::initBlockMutex( smriCTBody * aCTBody )
{
    UInt                sBlockIdx;
    UInt                sBlockIdx4Exception;
    smriCTBmpBlock    * sBlock;
    SChar               sMutexName[128];

    IDE_DASSERT( aCTBody != NULL );
    
    for( sBlockIdx = 0; sBlockIdx < mCTBodyBlockCnt; sBlockIdx++ )
    {
        sBlock = &aCTBody->mBlock[ sBlockIdx ];

        idlOS::snprintf( sMutexName,
                         ID_SIZEOF(sMutexName),
                         "CT_BLOCK_MUTEX_%"ID_UINT32_FMT,
                         sBlock->mBlockHdr.mBlockID );

        IDE_TEST( sBlock->mBlockHdr.mMutex.initialize( 
                                            sMutexName,
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    for( sBlockIdx4Exception = 0; 
         sBlockIdx4Exception < sBlockIdx; 
         sBlockIdx4Exception++ )
    {
        sBlock = &aCTBody->mBlock[ sBlockIdx4Exception ];
        IDE_ASSERT( sBlock->mBlockHdr.mMutex.destroy() == IDE_SUCCESS );
    }    

    return IDE_FAILURE;
}

/***********************************************************************
 * Block mutex를 파괴한다.
 * 
 * aCTBody  - [IN] CTBody의 ptr
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::destroyBlockMutex( smriCTBody * aCTBody )
{
    UInt                sBlockIdx;
    smriCTBmpBlock    * sBlock;
    
    IDE_DASSERT( aCTBody != NULL );

    for( sBlockIdx = 0; sBlockIdx < mCTBodyBlockCnt; sBlockIdx++ )
    {
        sBlock = &aCTBody->mBlock[ sBlockIdx ];
        IDE_TEST( sBlock->mBlockHdr.mMutex.destroy() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * BmpExtHdr block의 latch를 초기화 한다.
 * 
 * aCTBody  - [IN] CTBody의 Ptr
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::initBmpExtLatch( smriCTBody * aCTBody )
{
    UInt            sBmpExtIdx;
    smriCTBmpExt  * sBmpExt;
    SChar           sLatchName[128];
    UInt            sState = 0;
    UInt            sBmpExtIdx4Exception = 0;
    
    IDE_DASSERT( aCTBody != NULL );

    for( sBmpExtIdx = 0; sBmpExtIdx < mCTBodyBmpExtCnt; sBmpExtIdx++ )
    {
        sBmpExt = &aCTBody->mBmpExtent[ sBmpExtIdx ];

        idlOS::snprintf( 
                sLatchName,
                ID_SIZEOF(sLatchName),
                "BMP_EXT_HDR_LATCH_%"ID_UINT32_FMT,
                sBmpExt->mBmpExtHdrBlock.mBlockHdr.mBlockID 
                % SMRI_CT_EXT_BLOCK_CNT );

        IDE_TEST( sBmpExt->mBmpExtHdrBlock.mLatch.initialize( 
                                        sLatchName )
                  != IDE_SUCCESS );
        sState++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    for( sBmpExtIdx4Exception = 0;
         sBmpExtIdx4Exception < sBmpExtIdx; 
         sBmpExtIdx4Exception++ )
    {
        sBmpExt = &aCTBody->mBmpExtent[ sBmpExtIdx4Exception ];
        IDE_ASSERT( sBmpExt->mBmpExtHdrBlock.mLatch.destroy(  ) 
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * BmpExtHdr block의 latch를 파괴 한다.
 * 
 * aCTBody  - [IN] CTBody의 Ptr
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::destroyBmpExtLatch( smriCTBody * aCTBody )
{
    UInt            sBmpExtIdx;
    smriCTBmpExt  * sBmpExt;
    
    IDE_DASSERT( aCTBody != NULL );

    for( sBmpExtIdx = 0; sBmpExtIdx < mCTBodyBmpExtCnt; sBmpExtIdx++ )
    {
        sBmpExt = &aCTBody->mBmpExtent[ sBmpExtIdx ];

        IDE_TEST( sBmpExt->mBmpExtHdrBlock.mLatch.destroy(  )
                    != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

/***********************************************************************
 * CT파일 flush
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::flush()
{
    smLSN   sFlushLSN;
    UInt    sState = 0;

    ideLog::log( IDE_SM_0,
                 "===================================\n"
                 " flush change tracking file [start]\n"
                 "===================================\n" );

    IDE_TEST( startFlush() != IDE_SUCCESS );
    sState = 1;


    IDE_TEST( mFile.open() != IDE_SUCCESS );
    sState = 2;

    /* CT파일을 디스크로 기록할 때의 LSN 값을 loganchor와
     * CTFileHdr에 저장한다.*/
    smrLogMgr::getLstLSN( &sFlushLSN );
    IDE_TEST( smrRecoveryMgr::updateCTFileAttrToLogAnchor( NULL, //FileName
                                                           NULL, //CTMgrState
                                                           &sFlushLSN )
              != IDE_SUCCESS );


    SM_GET_LSN( mCTFileHdr.mLastFlushLSN, sFlushLSN );

    IDE_TEST( flushCTBody( sFlushLSN ) != IDE_SUCCESS );


    IDE_TEST( flushCTFileHdr() != IDE_SUCCESS );


    IDE_TEST( mFile.sync() != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( mFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( completeFlush() != IDE_SUCCESS );

    ideLog::log( IDE_SM_0,
                 "======================================\n"
                 " flush change tracking file [complete]\n"
                 "======================================\n" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( mFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( completeFlush() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
} 

/***********************************************************************
 * CT파일을 flush하기위한 준비 작업
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::startFlush()
{
    UInt    sState = 0;

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/) 
              != IDE_SUCCESS );
    sState = 1;

    /* BUG-44834 특정 장비에서 sprious wakeup 현상이 발생하므로 
                 wakeup 후에도 다시 확인 하도록 while문으로 체크한다.*/
    while ( mCTHdrState == SMRI_CT_MGR_STATE_EXTEND )
    {
        IDE_TEST_RAISE(mCTFileHdr.mExtendCV.wait(&(mCTFileHdr.mBlockHdr.mMutex))
                       != IDE_SUCCESS, error_cond_wait );
    }

    while ( mCTHdrState == SMRI_CT_MGR_STATE_FLUSH )
    {
        IDE_TEST_RAISE(mCTFileHdr.mFlushCV.wait(&(mCTFileHdr.mBlockHdr.mMutex))
                       != IDE_SUCCESS, error_cond_wait );
    }

    IDE_ASSERT( mCTHdrState == SMRI_CT_MGR_STATE_NORMAL );

    mCTHdrState = SMRI_CT_MGR_STATE_FLUSH;
    
    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
} 

/***********************************************************************
 * CT파일 flush 완료 작업
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::completeFlush()
{
    UInt    sState = 0;

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_ASSERT( mCTHdrState != SMRI_CT_MGR_STATE_NORMAL );

    mCTHdrState = SMRI_CT_MGR_STATE_NORMAL;

    IDE_TEST_RAISE(mCTFileHdr.mFlushCV.broadcast() != IDE_SUCCESS,
                   error_cond_signal );

    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
} 

/***********************************************************************
 * CTFileHdr flush
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::flushCTFileHdr()
{
    setBlockCheckSum( &mCTFileHdr.mBlockHdr );

    IDE_TEST( mFile.write( NULL, //aStatistics
                           SMRI_CT_HDR_OFFSET,
                           &mCTFileHdr,
                           SMRI_CT_BLOCK_SIZE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * CTBody flush
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::flushCTBody( smLSN aFlushLSN )
{
    smriCTBody    * sCTBody;
    UInt            sCTBodyWriteOffset;
    UInt            sCTBodyIdx;

    IDE_ASSERT( mCTBodyFlushBuffer != NULL );

    for( sCTBodyIdx = 0; sCTBodyIdx < mCTFileHdr.mCTBodyCNT; sCTBodyIdx++ )
    {
        sCTBody = mCTBodyPtr[ sCTBodyIdx ];

        idlOS::memcpy( mCTBodyFlushBuffer, sCTBody, ID_SIZEOF( smriCTBody ) );
 
        setCTBodyCheckSum( mCTBodyFlushBuffer, aFlushLSN );
 
        sCTBodyWriteOffset = ( sCTBodyIdx * mCTBodySize ) 
                             + SMRI_CT_BLOCK_SIZE;
 
        IDE_ERROR( sCTBodyIdx == sCTBody->mMetaExtent.mExtMapBlock.mCTBodyID );
 
        IDE_TEST( mFile.write( NULL, //aStatistics
                               sCTBodyWriteOffset,
                               mCTBodyFlushBuffer, 
                               mCTBodySize ) 
                  != IDE_SUCCESS );
        
        idlOS::memset( mCTBodyFlushBuffer, 0x00, ID_SIZEOF( smriCTBody ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * CT파일 extend
 * 
 * aFlushBody - [IN] extend완료후 파일로 내릴것인 지 판단
 * aCTBody    - [OUT] extend된 CTBody의 ptr
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::extend( idBool        aFlushBody, 
                                      smriCTBody ** aCTBody )
{
    idBool          sIsNeedExtend;
    smriCTBody    * sCTBody = NULL;
    UInt            sCTBodyIdx;
    smLSN           sFlushLSN;
    UInt            sState      = 0;
    UInt            sFileState  = 0;

    IDE_DASSERT( aCTBody != NULL );

    ideLog::log( IDE_SM_0,
                 "====================================\n"
                 " extend change tracking file [start]\n"
                 "====================================\n" );

    IDE_TEST( startExtend( &sIsNeedExtend ) != IDE_SUCCESS );
    sState = 1;


    IDE_TEST_CONT( sIsNeedExtend == ID_FALSE, skip_extend );

    /* smriChangeTrackingMgr_extend_calloc_CTBody.tc */
    IDU_FIT_POINT("smriChangeTrackingMgr::extend::calloc::CTBody");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                 1,
                                 ID_SIZEOF( smriCTBody ),
                                 (void**)&sCTBody )
                  != IDE_SUCCESS );
    sState = 2;

    sCTBody->mBlock = (smriCTBmpBlock *)sCTBody;

    IDE_TEST( createCTBody( sCTBody ) != IDE_SUCCESS );

    mCTFileHdr.mCTBodyCNT++;
    
    sCTBodyIdx = mCTFileHdr.mCTBodyCNT - 1;

    mCTBodyPtr[ sCTBodyIdx ] = sCTBody;

    if( aFlushBody == ID_TRUE )
    {
        IDE_TEST( mFile.open() != IDE_SUCCESS );
        sFileState = 1;

        smrLogMgr::getLstLSN( &sFlushLSN );

        SM_GET_LSN( mCTFileHdr.mLastFlushLSN, sFlushLSN );
 
        IDE_TEST( smrRecoveryMgr::updateCTFileAttrToLogAnchor( NULL,//FileName
                                                               NULL,//CTMgrState
                                                               &sFlushLSN )
              != IDE_SUCCESS );

        IDE_TEST( flushCTBody( sFlushLSN ) != IDE_SUCCESS );
        
        IDE_TEST( flushCTFileHdr() != IDE_SUCCESS );

        IDE_TEST( mFile.sync() != IDE_SUCCESS );

        sFileState = 0;
        IDE_TEST( mFile.close() != IDE_SUCCESS );
    }
    else
    {
        //noting to do
    }

    IDE_EXCEPTION_CONT(skip_extend);


    sState = 0;
    IDE_TEST( completeExtend() != IDE_SUCCESS );


    ideLog::log( IDE_SM_0,
                 "=======================================\n"
                 " extend change tracking file [complete]\n"
                 "=======================================\n" );

    *aCTBody = sCTBody; 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( iduMemMgr::free( sCTBody ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( completeExtend() == IDE_SUCCESS );
        default:
            break;
    }

    if( sFileState == 1 )
    {
        IDE_ASSERT( mFile.close() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
} 

/***********************************************************************
 * CT파일을 Extend하기위한 준비 작업
 * 
 * aIsNeedExtend    - [OUT] extend가 필요한지 여부
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::startExtend( idBool * aIsNeedExtend )
{
    UInt    sState = 0;

    IDE_DASSERT( aIsNeedExtend   != NULL );

    *aIsNeedExtend = ID_TRUE;

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/ ) 
              != IDE_SUCCESS );
    sState = 1;

    /* BUG-44834 특정 장비에서 sprious wakeup 현상이 발생하므로 
                 wakeup 후에도 다시 확인 하도록 while문으로 체크한다.*/
    while ( mCTHdrState == SMRI_CT_MGR_STATE_EXTEND )
    {
        IDE_TEST_RAISE(mCTFileHdr.mExtendCV.wait(&(mCTFileHdr.mBlockHdr.mMutex))
                       != IDE_SUCCESS, error_cond_wait );
        *aIsNeedExtend = ID_FALSE;
    }

    while ( mCTHdrState == SMRI_CT_MGR_STATE_FLUSH )
    {
        IDE_TEST_RAISE(mCTFileHdr.mFlushCV.wait(&(mCTFileHdr.mBlockHdr.mMutex))
                       != IDE_SUCCESS, error_cond_wait);
    }

    IDE_ASSERT( mCTHdrState == SMRI_CT_MGR_STATE_NORMAL );

    mCTHdrState = SMRI_CT_MGR_STATE_EXTEND;

    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
} 

/***********************************************************************
 * CT파일 Extend 완료 작업
 * 
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::completeExtend()
{
    UInt    sState = 0;

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/ ) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_ASSERT( mCTHdrState != SMRI_CT_MGR_STATE_NORMAL );

    mCTHdrState = SMRI_CT_MGR_STATE_NORMAL;

    IDE_TEST_RAISE( mCTFileHdr.mExtendCV.broadcast() != IDE_SUCCESS,
                    error_cond_signal );

    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
} 

/***********************************************************************
 * CT파일의 flush와 extend가 완료되기를 대기함
 * 
 * 이 함수를 호출하기 위해서는 CTFileHdr의 mutex를 획득한 상태이여만 한다.
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::wait4FlushAndExtend()
{
    /* BUG-44834 특정 장비에서 sprious wakeup 현상이 발생하므로 
                 wakeup 후에도 다시 확인 하도록 while문으로 체크한다.*/
    while ( mCTHdrState == SMRI_CT_MGR_STATE_FLUSH )
    {
        IDE_TEST_RAISE( mCTFileHdr.mFlushCV.wait(&(mCTFileHdr.mBlockHdr.mMutex))
                        != IDE_SUCCESS, error_cond_wait );
    }

    while ( mCTHdrState == SMRI_CT_MGR_STATE_EXTEND )
    {
        IDE_TEST_RAISE(mCTFileHdr.mExtendCV.wait(&(mCTFileHdr.mBlockHdr.mMutex))
                       != IDE_SUCCESS, error_cond_wait );
    }

    IDE_ERROR( mCTHdrState == SMRI_CT_MGR_STATE_NORMAL );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * 데이터파일을 CT파일에 등록
 * 함수 실패시 change tracking파일 invalid
 *
 * aSpaceID     - [IN]  데이터파일의 테이블스페이스 ID
 * aDataFileID  - [IN]  데이터파일의 ID
 * aPageSize    - [IN]  데이터파일의 페이지크기
 * aSlotID      - [OUT] 할당된 slot의 ID
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::addDataFile2CTFile( 
                                      scSpaceID                aSpaceID, 
                                      UInt                     aDataFileID,
                                      smriCTTBSType            aTBSType,
                                      smiDataFileDescSlotID ** aSlotID )
{
    smriCTDataFileDescSlot    * sSlot;
    smriCTBmpExt              * sDummyBmpExt;
    UInt                        sState = 0;

    IDE_DASSERT( aSlotID != NULL );

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/) 
              != IDE_SUCCESS ); 
    sState = 1;

    IDE_TEST( wait4FlushAndExtend() != IDE_SUCCESS );

    /* 추가되는 데이터파일에 DataFileDescSlot을 할당한다. */
    IDE_TEST( allocDataFileDescSlot( &sSlot ) !=IDE_SUCCESS );

    /* DataFileDescSlot에 BmpExt를 추가한다. */
    IDE_TEST( addBmpExt2DataFileDescSlot( sSlot,
                                          &sDummyBmpExt ) 
              != IDE_SUCCESS );

    /* DataFileDescSlot 정보 설정 */
    sSlot->mSpaceID        = aSpaceID;

    if( aTBSType == SMRI_CT_MEM_TBS )
    {
        sSlot->mTBSType     = SMRI_CT_MEM_TBS;
        sSlot->mPageSize    = SM_PAGE_SIZE;
        sSlot->mFileID      = (UInt)aDataFileID;
    }
    else
    { 
        if( aTBSType == SMRI_CT_DISK_TBS )
        {
            sSlot->mTBSType     = SMRI_CT_DISK_TBS;
            sSlot->mPageSize    = SD_PAGE_SIZE;
            sSlot->mFileID      = (UInt)aDataFileID;
        }
        else
        {
            IDE_DASSERT(0);
            /* nothing to do */
        }
    }

    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS ); 

    *aSlotID = &sSlot->mSlotID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * 사용중이지 않은 DataFile Desc slot을 가져온다.
 * 한수가 호출되기 전에 CTFileHdr의 mutex가 잡힌 상태여야 한다.
 * DataFileDescSlot의 할당은 CTFileHdr의 mutex를 잡고 진행 되기 때문에
 * DataFileDescBlock의 mutex를 획들할 필요가 없다.
 * 
 * aSlot    - [OUT] 할당받은 DataFile Desc slot의 ptr
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::allocDataFileDescSlot( 
                                        smriCTDataFileDescSlot ** aSlot )
{
    smriCTDataFileDescBlock * sDataFileDescBlock;
    smriCTBody              * sCTBody;
    UInt                      sCTBodyIdx;
    UInt                      sDataFileDescBlockIdx;
    UInt                      sSlotIdx = SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;

    IDE_DASSERT( aSlot != NULL );

    IDE_ERROR( 0 < mCTFileHdr.mCTBodyCNT );

    for( sCTBodyIdx = 0; sCTBodyIdx < mCTFileHdr.mCTBodyCNT; sCTBodyIdx++ )
    {
        sCTBody = mCTBodyPtr[ sCTBodyIdx ];

        for( sDataFileDescBlockIdx = 0; 
             sDataFileDescBlockIdx < SMRI_CT_DATAFILE_DESC_BLOCK_CNT; 
             sDataFileDescBlockIdx++ )
        {
            sDataFileDescBlock = 
                    &sCTBody->mMetaExtent.mDataFileDescBlock[ sDataFileDescBlockIdx ];
 
            if( SMRI_CT_CAN_ALLOC_DATAFILE_DESC_SLOT( 
                            sDataFileDescBlock->mAllocSlotFlag ) == ID_TRUE )
            {
                getFreeSlotIdx( sDataFileDescBlock->mAllocSlotFlag, &sSlotIdx );
 
                IDE_TEST_RAISE( sSlotIdx == 
                                SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX,
                                error_invalid_change_tracking_file );

                sDataFileDescBlock->mAllocSlotFlag |= ( 1 << sSlotIdx );
 
                IDE_CONT( datafile_descriptor_slot_is_allocated );
            }
            else
            {
                /* nothing to do */
            }
        }
    }

    IDE_EXCEPTION_CONT(datafile_descriptor_slot_is_allocated);

    *aSlot = &sDataFileDescBlock->mSlot[ sSlotIdx ];
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_change_tracking_file );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidChangeTrackingFile, 
                mFile.getFileName() ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * 사용중이지 않은 DataFile Desc slot idx를 가져온다
 * 
 * aAllocSlotFlag   - [IN] 할당하려는 slot이 존재하는 DataFileDesc 블럭
 * aSlotIdx         - [OUT] 할당 하려는 DataFileDesc slot의 index
 * 
 **********************************************************************/
void smriChangeTrackingMgr::getFreeSlotIdx( UInt aAllocSlotFlag, 
                                            UInt * aSlotIdx )
{
    UInt    sSlotIdx;

    IDE_DASSERT( aAllocSlotFlag != SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX )
    IDE_DASSERT( aSlotIdx       != NULL );

    if( (aAllocSlotFlag & SMRI_CT_DATAFILE_DESC_SLOT_FLAG_FIRST  ) == 0 )
    {
        sSlotIdx = 0;
    }
    else
    {
        if( (aAllocSlotFlag & SMRI_CT_DATAFILE_DESC_SLOT_FLAG_SECOND  ) == 0 )
        {
            sSlotIdx = 1;
        }
        else
        {
            if( (aAllocSlotFlag & SMRI_CT_DATAFILE_DESC_SLOT_FLAG_THIRD  ) == 0 )
            {
                sSlotIdx = 2;
            }
            else
            {
                sSlotIdx = SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;
            }
        }
    }

    *aSlotIdx = sSlotIdx;

    return;
}

/***********************************************************************
 * DataFileDesc slot에 BmpExt를 추가한다.
 * 
 * aSlot            - [IN] BmpExt를 바인드 할 slot
 * aAllocCurBmpExt  - [OUT] 새로 추가된 BmpExt중CurTrackingListID에 해당하는
 *                          BmpExt를 전달한다.
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::addBmpExt2DataFileDescSlot( 
                                    smriCTDataFileDescSlot * aSlot,
                                    smriCTBmpExt          ** aAllocCurBmpExt )
{
    UInt                    i;
    smriCTBmpExtHdrBlock  * sBmpExtHdrBlock;
    smriCTBmpExt          * sBmpExt[ SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT ];

    IDE_DASSERT( aSlot           != NULL );
    IDE_DASSERT( aAllocCurBmpExt != NULL );
    
    for( i = 0; i < SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT; i++ )
    {
        IDE_TEST( allocBmpExt( &sBmpExt[i] ) != IDE_SUCCESS );

        sBmpExtHdrBlock                     = &sBmpExt[i]->mBmpExtHdrBlock;
        sBmpExtHdrBlock->mType              = (smriCTBmpExtType)i;
        sBmpExtHdrBlock->mDataFileDescSlotID.mBlockID   
                                            = aSlot->mSlotID.mBlockID;
        sBmpExtHdrBlock->mDataFileDescSlotID.mSlotIdx   
                                            = aSlot->mSlotID.mSlotIdx;
        
        addBmpExt2List( &aSlot->mBmpExtList[i], sBmpExt[i] );

        aSlot->mBmpExtCnt++;
    }
    
    sBmpExt[SMRI_CT_DIFFERENTIAL_BMP_EXT_0]->mBmpExtHdrBlock.mCumBmpExtHdrBlockID = 
                sBmpExt[SMRI_CT_CUMULATIVE_BMP_EXT]->mBmpExtHdrBlock.mBlockHdr.mBlockID;

    sBmpExt[SMRI_CT_DIFFERENTIAL_BMP_EXT_1]->mBmpExtHdrBlock.mCumBmpExtHdrBlockID = 
                sBmpExt[SMRI_CT_CUMULATIVE_BMP_EXT]->mBmpExtHdrBlock.mBlockHdr.mBlockID;

    *aAllocCurBmpExt = sBmpExt[ aSlot->mCurTrackingListID ];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * DataFileDesc slot에 BmpExt를 할당한다.
 * 
 * aAllocBmpExt   - [OUT] 할당된 BmpExt 반환
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::allocBmpExt( smriCTBmpExt ** aAllocBmpExt )
{
    UInt                sExtMapIdx;
    UInt                sAllocBmpExtIdx;
    UInt                sCTBodyIdx;
    smriCTExtMapBlock * sExtMapBlock;
    smriCTBody        * sCTBody;
    smriCTBody        * sDummyCTBody;
    UInt                sScanStartCTBodyIdx = 0;
    idBool              sResult             = ID_FALSE;
    idBool              sMutexState         = ID_TRUE;

    IDE_DASSERT( aAllocBmpExt != NULL );

    while(1)
    {
        for( sCTBodyIdx = sScanStartCTBodyIdx; 
             sCTBodyIdx < mCTFileHdr.mCTBodyCNT; 
             sCTBodyIdx++ )
        {
            sCTBody = mCTBodyPtr[ sCTBodyIdx ];
 
            sExtMapBlock = &sCTBody->mMetaExtent.mExtMapBlock;
        
            for( sExtMapIdx = 0; sExtMapIdx < mCTExtMapSize; sExtMapIdx++)
            {
                if( sExtMapBlock->mExtentMap[ sExtMapIdx ] == SMRI_CT_EXT_FREE )
                {
                    sExtMapBlock->mExtentMap[ sExtMapIdx ] = SMRI_CT_EXT_USE;
                    sAllocBmpExtIdx                        = sExtMapIdx;
                    sResult                                = ID_TRUE;
                    IDE_CONT( bitmap_extent_is_allocated );
                }
                else
                {
                    /* nothing to do */
                }
            }
        }
 
        /* BmpExt가 부족한 상황 CTBody를 확장한다. */
        if ( sResult == ID_FALSE )
        {
            IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS );
            sMutexState = ID_FALSE;
 
            IDE_TEST( extend( ID_TRUE, &sDummyCTBody ) != IDE_SUCCESS );
 
            
            sMutexState = ID_TRUE;
            IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/ ) 
                      != IDE_SUCCESS );
 
            sScanStartCTBodyIdx = mCTFileHdr.mCTBodyCNT - 1; 
        }
        else 
        {
            /* BmpExt할당에 성공함
             * sResult == ID_TRUE
             */
            break;
        }
    }   

    IDE_EXCEPTION_CONT(bitmap_extent_is_allocated);

    IDE_DASSERT( sResult == ID_TRUE );

    *aAllocBmpExt = &sCTBody->mBmpExtent[ sAllocBmpExtIdx ];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if( sMutexState == ID_FALSE )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.lock( NULL ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * BmpExtList에 BmpExt를 add한다.
 * 
 * aBaseBmpExt  - [IN] BmpExtList의 헤더 BmpExt
 * aNewBmpExt   - [IN] BmpExtList에 추가할 BmpExt
 *
 **********************************************************************/
void smriChangeTrackingMgr::addBmpExt2List( 
                                    smriCTBmpExtList       * aBmpExtList,
                                    smriCTBmpExt           * aNewBmpExt )
{
    UInt                   sBaseBmpExtHdrBlockID;
    smriCTBmpExtHdrBlock * sBaseBmpExtHdrBlock;
    smriCTBmpExtHdrBlock * sNewBmpExtHdrBlock;
    smriCTBmpExtHdrBlock * sBasePrevBmpExtHdrBlock;
    
    IDE_DASSERT( aBmpExtList != NULL );
    IDE_DASSERT( aNewBmpExt  != NULL );
    
    sBaseBmpExtHdrBlockID   = aBmpExtList->mBmpExtListHint[0];
    sNewBmpExtHdrBlock      = &aNewBmpExt->mBmpExtHdrBlock;

    if( sBaseBmpExtHdrBlockID != SMRI_CT_INVALID_BLOCK_ID )
    {
        getBlock( sBaseBmpExtHdrBlockID, (void**)&sBaseBmpExtHdrBlock );

        sNewBmpExtHdrBlock->mNextBmpExtHdrBlockID = 
                                    sBaseBmpExtHdrBlock->mBlockHdr.mBlockID;
        sNewBmpExtHdrBlock->mPrevBmpExtHdrBlockID = 
                                    sBaseBmpExtHdrBlock->mPrevBmpExtHdrBlockID;

        getBlock( sBaseBmpExtHdrBlock->mPrevBmpExtHdrBlockID, 
                  (void**)&sBasePrevBmpExtHdrBlock );

        sBasePrevBmpExtHdrBlock->mNextBmpExtHdrBlockID = 
                                    sNewBmpExtHdrBlock->mBlockHdr.mBlockID;
        sBaseBmpExtHdrBlock->mPrevBmpExtHdrBlockID = 
                                    sNewBmpExtHdrBlock->mBlockHdr.mBlockID;

        sNewBmpExtHdrBlock->mBmpExtSeq = 
                                    sBasePrevBmpExtHdrBlock->mBmpExtSeq + 1;

        if( sNewBmpExtHdrBlock->mBmpExtSeq < SMRI_CT_BMP_EXT_LIST_HINT_CNT )
        {
            aBmpExtList->mBmpExtListHint[ sNewBmpExtHdrBlock->mBmpExtSeq ] = 
                                    sNewBmpExtHdrBlock->mBlockHdr.mBlockID;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        sNewBmpExtHdrBlock->mNextBmpExtHdrBlockID =
                                     sNewBmpExtHdrBlock->mBlockHdr.mBlockID;
        sNewBmpExtHdrBlock->mPrevBmpExtHdrBlockID =
                                     sNewBmpExtHdrBlock->mBlockHdr.mBlockID;
        sNewBmpExtHdrBlock->mBmpExtSeq    = 0;
        aBmpExtList->mBmpExtListHint[ 0 ] = 
                                    sNewBmpExtHdrBlock->mBlockHdr.mBlockID;

    }
        
    return;
}

/***********************************************************************
 * DataFileDesc slot을 반환한다.
 * 함수 실패시 change tracking파일 invalid
 *
 * aSlotID  - [IN] 할당해제하려는 slot의 ID
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::deleteDataFileFromCTFile( 
                                    smiDataFileDescSlotID * aSlotID )
{
    smriCTDataFileDescSlot    * sSlot; 
    UInt                        sState = 0;

    IDE_DASSERT( aSlotID != NULL );

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/ ) 
              != IDE_SUCCESS ); 
    sState = 1;

    IDE_TEST( wait4FlushAndExtend() != IDE_SUCCESS );

    getDataFileDescSlot( aSlotID, &sSlot );

    /* slot에 할당된 모든 BmpExt를 제거한다. */
    IDE_TEST( deleteBmpExtFromDataFileDescSlot( sSlot ) != IDE_SUCCESS );

    /* DataFileDescSlot 정보 초기화및 할당 해제 */
    IDE_TEST( deallocDataFileDescSlot( sSlot ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS ); 
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * slot에 할당된 BmpExtList를 초기화한다.
 * 
 * aBmpExtList  - [IN] slot에서 반환하려는 BmpExt들의 List
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::deleteBmpExtFromDataFileDescSlot( 
                                            smriCTDataFileDescSlot * aSlot )
{
    smriCTBmpExtHdrBlock  * sBmpExtHdrBlock;
    smriCTBmpExtList      * sBmpExtList;
    UInt                    sBmpExtListLen;
    UInt                    sBmpExtHintIdx;
    UInt                    sDeleteBmpExtCnt;
    UInt                    i;

    IDE_DASSERT( aSlot != NULL );

    sBmpExtListLen = aSlot->mBmpExtCnt / SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT;

    for( i = 0; i < SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT; i++ )
    {
        sBmpExtList = &aSlot->mBmpExtList[i];

        for( sDeleteBmpExtCnt = 0; 
             sDeleteBmpExtCnt < sBmpExtListLen; 
             sDeleteBmpExtCnt++ )
        {
            IDE_TEST( deleteBmpExtFromList( sBmpExtList, &sBmpExtHdrBlock ) 
                      != IDE_SUCCESS );

            aSlot->mBmpExtCnt--;

            IDE_TEST( deallocBmpExt( sBmpExtHdrBlock ) != IDE_SUCCESS );
        }
    
        for( sBmpExtHintIdx = 0; 
             sBmpExtHintIdx < SMRI_CT_BMP_EXT_LIST_HINT_CNT; 
             sBmpExtHintIdx++ )
        {
            IDE_DASSERT( sBmpExtList->mBmpExtListHint[ sBmpExtHintIdx ] 
                         == SMRI_CT_INVALID_BLOCK_ID );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * BmpExtList로부터 list의 마지막 BmpExt를 삭제한다.
 * 
 * aBmpExtList      - [IN] BmpExt를 삭제하려는 List
 * aBmpExtHdrBlock  - [OUT] List로부터 삭제되 BmpExt의 BmpExtHdrBlock
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::deleteBmpExtFromList( 
                                    smriCTBmpExtList      * aBmpExtList,
                                    smriCTBmpExtHdrBlock ** aBmpExtHdrBlock )
{
    UInt                   sBaseBmpExtHdrBlockID;
    UInt                   sLastBmpExtHdrBlockID;
    UInt                   sLastPrevBmpExtHdrBlockID;
    smriCTBmpExtHdrBlock * sBaseBmpExtHdrBlock;
    smriCTBmpExtHdrBlock * sLastBmpExtHdrBlock;
    smriCTBmpExtHdrBlock * sLastPrevBmpExtHdrBlock;

    IDE_DASSERT( aBmpExtList         != NULL );
    IDE_DASSERT( aBmpExtHdrBlock     != NULL );

    sBaseBmpExtHdrBlockID = aBmpExtList->mBmpExtListHint[0];

    IDE_ERROR( sBaseBmpExtHdrBlockID != SMRI_CT_INVALID_BLOCK_ID );
    
    getBlock( sBaseBmpExtHdrBlockID, (void**)&sBaseBmpExtHdrBlock );

    sLastBmpExtHdrBlockID = sBaseBmpExtHdrBlock->mPrevBmpExtHdrBlockID;

    if( sBaseBmpExtHdrBlockID != sLastBmpExtHdrBlockID )
    {
        getBlock( sLastBmpExtHdrBlockID, (void**)&sLastBmpExtHdrBlock );

        sLastPrevBmpExtHdrBlockID = sLastBmpExtHdrBlock->mPrevBmpExtHdrBlockID;
        getBlock( sLastPrevBmpExtHdrBlockID, (void**)&sLastPrevBmpExtHdrBlock );

        sBaseBmpExtHdrBlock->mPrevBmpExtHdrBlockID = sLastPrevBmpExtHdrBlockID;
        sLastPrevBmpExtHdrBlock->mNextBmpExtHdrBlockID = sBaseBmpExtHdrBlockID;
        
    }
    else
    {
        /* 
         * aBmpExtList에 BmpExt가 1개 남아있는상황, 별도의 list연산을 수행할
         * 필요가 없다
         */
        sLastBmpExtHdrBlock = sBaseBmpExtHdrBlock;
    }

    if( sLastBmpExtHdrBlock->mBmpExtSeq < SMRI_CT_BMP_EXT_LIST_HINT_CNT )
    {
        IDE_ERROR( aBmpExtList->mBmpExtListHint[sLastBmpExtHdrBlock->mBmpExtSeq] 
                     != SMRI_CT_INVALID_BLOCK_ID );

        aBmpExtList->mBmpExtListHint[sLastBmpExtHdrBlock->mBmpExtSeq] = 
                                      SMRI_CT_INVALID_BLOCK_ID;
    }

    *aBmpExtHdrBlock = sLastBmpExtHdrBlock;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * BmpExt를 할당 해제한다.
 * 
 * aBmpExtHdrBlock  - [IN] 할당해제하려는 BmpExtHdrBlock
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::deallocBmpExt( 
                                smriCTBmpExtHdrBlock  * aBmpExtHdrBlock )
{

    UInt                    sCTBodyIdx;
    UInt                    sBmpExtIdx;
    UInt                    sBmpBlockIdx;
    smriCTBody            * sCTBody;
    smriCTBmpExt          * sBmpExt;
    smriCTBmpBlock        * sBmpBlock;
    smriCTExtMapBlock     * sExtMapBlock;

    IDE_DASSERT( aBmpExtHdrBlock != NULL );    

    /* BmpExt가 속한 CTBody의 Idx를 구한다. */
    sCTBodyIdx = aBmpExtHdrBlock->mBlockHdr.mBlockID / mCTBodyBlockCnt;

    /* CTBody내에서 BmpExt의 Idx값을 구한다. */
    sBmpExtIdx = ( aBmpExtHdrBlock->mBlockHdr.mBlockID 
                   - (sCTBodyIdx * mCTBodyBlockCnt) ) 
                   / SMRI_CT_EXT_BLOCK_CNT;

    sCTBody         = mCTBodyPtr[ sCTBodyIdx ];
    sExtMapBlock    = &sCTBody->mMetaExtent.mExtMapBlock;

    sBmpExt = &sCTBody->mBmpExtent[ sBmpExtIdx - 1 ];

    IDE_ERROR( sBmpExt->mBmpExtHdrBlock.mBlockHdr.mBlockID == 
                 aBmpExtHdrBlock->mBlockHdr.mBlockID );
    
    /* BmpExt의 정보를 초기화 한다. */
    sBmpExt->mBmpExtHdrBlock.mNextBmpExtHdrBlockID = SMRI_CT_INVALID_BLOCK_ID;
    sBmpExt->mBmpExtHdrBlock.mPrevBmpExtHdrBlockID = SMRI_CT_INVALID_BLOCK_ID;
    sBmpExt->mBmpExtHdrBlock.mCumBmpExtHdrBlockID  = SMRI_CT_INVALID_BLOCK_ID;
    sBmpExt->mBmpExtHdrBlock.mBmpExtSeq            = 0;
    sBmpExt->mBmpExtHdrBlock.mType                 = SMRI_CT_FREE_BMP_EXT;

    sBmpExt->mBmpExtHdrBlock.mDataFileDescSlotID.mBlockID = 
                                        SMRI_CT_INVALID_BLOCK_ID;
    sBmpExt->mBmpExtHdrBlock.mDataFileDescSlotID.mSlotIdx = 
                                        SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;
    
    /*BmpBlock의 bitmap정보를 0으로 초기화한다. */
    for( sBmpBlockIdx = 0; 
         sBmpBlockIdx < SMRI_CT_EXT_BLOCK_CNT_EXCEPT_META_BLOCK; 
         sBmpBlockIdx++ )
    {
        sBmpBlock = &sBmpExt->mBmpBlock[ sBmpBlockIdx ];
        idlOS::memset( &sBmpBlock->mBitmap, 0, mBmpBlockBitmapSize );
    }

    IDE_ERROR( sExtMapBlock->mExtentMap[ sBmpExtIdx - 1 ] == SMRI_CT_EXT_USE );

    /* extent map에서 해당 BmpExt의 할당을 해제한다. */
    sExtMapBlock->mExtentMap[ sBmpExtIdx - 1 ] = SMRI_CT_EXT_FREE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * 할당해제되는 BmpExt를 초기화한다.
 * 
 * aSlot              - [IN] 할당 해제되는 DataFileDescSlot
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::deallocDataFileDescSlot( 
                                        smriCTDataFileDescSlot  * aSlot )
{
    smriCTDataFileDescBlock * sDataFileDescBlock;
    UInt                      sAllocSlotFlag = 0x1;

    IDE_DASSERT( aSlot != NULL );

    getBlock( aSlot->mSlotID.mBlockID, (void**)&sDataFileDescBlock ); 

    IDE_ERROR( aSlot->mSlotID.mBlockID == 
               sDataFileDescBlock->mBlockHdr.mBlockID );
    
    aSlot->mBmpExtCnt           = 0;
    aSlot->mSpaceID             = SC_NULL_SPACEID;
    aSlot->mFileID              = 0;
    aSlot->mPageSize            = 0;
    aSlot->mTBSType             = SMRI_CT_NONE;
    aSlot->mTrackingState       = SMRI_CT_TRACKING_DEACTIVE;
    aSlot->mCurTrackingListID   = 0;

    IDE_ERROR( ( sDataFileDescBlock->mAllocSlotFlag & 
               ( sAllocSlotFlag << aSlot->mSlotID.mSlotIdx ) ) != 0 );

    sDataFileDescBlock->mAllocSlotFlag &=
                             ~(sAllocSlotFlag << aSlot->mSlotID.mSlotIdx );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * differential bakcup변경 정보를 추적중인 BmpExtList를 switch한다.
 * 함수 실패시 changeTracking파일 invaild
 * aSlot  - [IN] backup을 수행하려는 datafile의 slot
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::switchBmpExtListID( 
                                        smriCTDataFileDescSlot * aSlot )
{
    UInt    sState = 0;
    UShort  sLockListID;

    IDE_DASSERT( aSlot != NULL );

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/ ) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( wait4FlushAndExtend() != IDE_SUCCESS );

    sLockListID = aSlot->mCurTrackingListID;

    IDE_TEST( lockBmpExtHdrLatchX( aSlot, sLockListID ) != IDE_SUCCESS );

    aSlot->mCurTrackingListID = ( aSlot->mCurTrackingListID + 1 ) % 2;

    IDE_TEST( unlockBmpExtHdrLatchX( aSlot, sLockListID ) != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * DataFile Desc slot에 할당된 모든 BmpExtHdr의 latch를 획득한다.
 * 
 * aSlot  - [IN] backup을 수행하려는 datafile의 slot
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::lockBmpExtHdrLatchX( 
                                    smriCTDataFileDescSlot * aSlot,
                                    UShort                   aLockListID )
{
    UInt                    sFirstBmpExtHdrBlockID;
    UInt                    sBmpExtHdrBlockID;
    smriCTBmpExtHdrBlock  * sBmpExtHdrBlock;
    UInt                    i = 0;
    UInt                    sBmpExtCnt = 0;

    IDE_DASSERT( aSlot != NULL );

    sFirstBmpExtHdrBlockID = 
                aSlot->mBmpExtList[ aLockListID ].mBmpExtListHint[0];

    IDE_ERROR( sFirstBmpExtHdrBlockID != SMRI_CT_INVALID_BLOCK_ID );

    sBmpExtHdrBlockID = sFirstBmpExtHdrBlockID;

    do
    {
        getBlock( sBmpExtHdrBlockID, 
                            (void**)&sBmpExtHdrBlock );

        IDE_TEST( sBmpExtHdrBlock->mLatch.lockWrite( NULL,  //aStatistics
                                       NULL ) //aWeArgs
                  != IDE_SUCCESS );

        sBmpExtCnt++;

        sBmpExtHdrBlockID = sBmpExtHdrBlock->mNextBmpExtHdrBlockID;

    }while( sFirstBmpExtHdrBlockID != sBmpExtHdrBlockID );

    IDE_ERROR( sBmpExtCnt == ( aSlot->mBmpExtCnt 
                             / SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sBmpExtHdrBlockID = sFirstBmpExtHdrBlockID;

    for( i = 0 ; i < sBmpExtCnt; i++ )
    {
        getBlock( sBmpExtHdrBlockID, 
                  (void**)&sBmpExtHdrBlock );

        IDE_ASSERT( sBmpExtHdrBlock->mLatch.unlock( ) 
                    == IDE_SUCCESS );
        
        sBmpExtHdrBlockID = sBmpExtHdrBlock->mNextBmpExtHdrBlockID;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * DataFile Desc slot에 할당된 모든 BmpExtHdr의 latch를 해제한다.
 * 
 * aSlot  - [IN] backup을 수행하려는 datafile의 slot
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::unlockBmpExtHdrLatchX( 
                                        smriCTDataFileDescSlot * aSlot,
                                        UShort                   aLockListID )
{
    UInt                    sFirstBmpExtHdrBlockID;
    UInt                    sBmpExtHdrBlockID;
    smriCTBmpExtHdrBlock  * sBmpExtHdrBlock;
    UInt                    sBmpExtCnt = 0;

    IDE_DASSERT( aSlot != NULL );

    sFirstBmpExtHdrBlockID = 
                aSlot->mBmpExtList[ aLockListID ].mBmpExtListHint[0];

    IDE_ERROR( sFirstBmpExtHdrBlockID != SMRI_CT_INVALID_BLOCK_ID );

    sBmpExtHdrBlockID = sFirstBmpExtHdrBlockID;

    do
    {
        getBlock( sBmpExtHdrBlockID, 
                            (void**)&sBmpExtHdrBlock );

        IDE_TEST( sBmpExtHdrBlock->mLatch.unlock( )
                  != IDE_SUCCESS );

        sBmpExtCnt++;

        sBmpExtHdrBlockID = sBmpExtHdrBlock->mNextBmpExtHdrBlockID;

    }while( sFirstBmpExtHdrBlockID != sBmpExtHdrBlockID );

    IDE_ERROR( sBmpExtCnt == ( aSlot->mBmpExtCnt 
                                 / SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sBmpExtCnt != 0 )
    {
        for( ; 
             sBmpExtCnt < ( aSlot->mBmpExtCnt 
                            / SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT ); 
             sBmpExtCnt++ )
        {
            getBlock( sBmpExtHdrBlock->mNextBmpExtHdrBlockID, 
                      (void**)&sBmpExtHdrBlock );
 
            IDE_ASSERT( sBmpExtHdrBlock->mLatch.unlock(  ) 
                        == IDE_SUCCESS );
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Write Page 한 정보를 기록한다.
 * 
 * aDataFileNode - [IN] 해당 DataFile
 * aPageID       - [IN] 해당 PageID
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::changeTracking4WriteDiskPage( sddDataFileNode * aDataFileNode,
                                                            scPageID          aPageID )
{
    UInt  sIBChunkID;
    
    (void)idCore::acpAtomicInc32( &mCurrChangeTrackingThreadCnt );

    sIBChunkID = calcIBChunkID4DiskPage( aPageID );

    IDE_TEST( changeTracking( aDataFileNode,
                              NULL,
                              sIBChunkID ) 
              != IDE_SUCCESS );

    (void)idCore::acpAtomicDec32( &mCurrChangeTrackingThreadCnt );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * IBChunk변화를 추적한다.
 * 함수 실패시 change tracking파일 invalid
 * sddDataFileNode  - [IN] 추적하려는 datafile의 node
 * aDatabaseFile    - [IN] 추적하려는 database file
 * aIBChunkID       - [IN] 추적하려는 IBChunkID
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::changeTracking( 
                                  sddDataFileNode * aDataFileNode,
                                  smmDatabaseFile * aDatabaseFile,
                                  UInt              aIBChunkID )
{
    smriCTDataFileDescSlot    * sSlot;
    smiDataFileDescSlotID     * sDataFileDescSlotID;
    smriCTBmpExt              * sCurBmpExt;
    smriCTState                 sTrackingState;
    smmChkptImageHdr            sChkptImageHdr;
    scSpaceID                   sSpaceID;
    UInt                        sFileID;
    UInt                        sBeforeTrackingListID;
    UInt                        sAfterTrackingListID;
    UInt                        sState = 0;

    /* 메모리/디스크 데이터파일을 구분하고 필요한 정보를 얻는다 */
    if( aDataFileNode != NULL )
    {
        IDE_ERROR( aDatabaseFile == NULL );

        sDataFileDescSlotID = &aDataFileNode->mDBFileHdr.mDataFileDescSlotID;
        sSpaceID            = aDataFileNode->mSpaceID;
        sFileID             = (UInt)(aDataFileNode->mID);
    }
    else
    {
        IDE_ERROR( aDataFileNode == NULL );
        IDE_ERROR( aDatabaseFile != NULL );

        aDatabaseFile->getChkptImageHdr( &sChkptImageHdr );
        sDataFileDescSlotID = &sChkptImageHdr.mDataFileDescSlotID;
        sFileID             = aDatabaseFile->getFileNum();
        sSpaceID            = aDatabaseFile->getSpaceID();
    }
    
    IDE_ERROR_RAISE( (sDataFileDescSlotID->mBlockID 
                                     != SMRI_CT_INVALID_BLOCK_ID) ||
                     (sDataFileDescSlotID->mSlotIdx 
                                     != SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX),
                    error_invalid_change_tracking_file );

    while(1)
    {
        /* DataFileDescSlot을 가져온다. */
        getDataFileDescSlot( sDataFileDescSlotID, &sSlot );
 
        IDE_ERROR( (sSpaceID == sSlot->mSpaceID) && (sFileID == sSlot->mFileID) );
 
        /* DataFileDescSlot이 change tracking을 할수 있는 상태인지 확인한다. */
        sTrackingState = 
                    (smriCTState)idCore::acpAtomicGet32( &sSlot->mTrackingState );
 
        IDE_TEST_CONT( sTrackingState != SMRI_CT_TRACKING_ACTIVE, 
                        skip_change_tracking );

        sBeforeTrackingListID = sSlot->mCurTrackingListID;

 
        /* 전달된 IBChunkID에 해당하는 BmpExt를 가져온다 */
        IDE_TEST( getCurBmpExt( sSlot, aIBChunkID, &sCurBmpExt ) 
                  != IDE_SUCCESS );
 
        /* 
         * change tracking중 bitmap이 switch되는 것을
         * 방지하기위해 가져온 BmpExt에 read lock을 건다.
         */
        IDE_TEST( sCurBmpExt->mBmpExtHdrBlock.mLatch.lockRead( NULL,  //aStatistics
                                      NULL ) //aWeArgs
                  != IDE_SUCCESS );
        sState = 1;
 
        /* 
         * readlock을 걸기전에 bitmap switch가 되었는지 확인하기위해
         * 다시 DataFileDescSlot을 가져온다.
         */
        getDataFileDescSlot( sDataFileDescSlotID, &sSlot ); 
 
        IDE_ERROR( (sSpaceID == sSlot->mSpaceID) && (sFileID == sSlot->mFileID) );
 
        sAfterTrackingListID = sSlot->mCurTrackingListID;
 
        /* BmpExt에 readlock을 잡기전에 bitmap switch가 되었는지 확인 */
        if( sBeforeTrackingListID != sAfterTrackingListID )
        {
            sState = 0;
            IDE_TEST( sCurBmpExt->mBmpExtHdrBlock.mLatch.unlock( ) 
                      != IDE_SUCCESS );
        }
        else
        {
            /* bitmap이 swit되지 않았다. 제대로된 CurBmpExt를 가져온 상태 */
            break;
        }
    }

    /* bit를 set한다. */
    IDE_TEST( calcBitPositionAndSet( sCurBmpExt, aIBChunkID, sSlot ) != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( sCurBmpExt->mBmpExtHdrBlock.mLatch.unlock(  ) 
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_change_tracking );


    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_change_tracking_file );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidChangeTrackingFile, 
                mFile.getFileName()));
    }
    IDE_EXCEPTION_END;
    
    if( sState == 1 )
    {
        IDE_ASSERT( sCurBmpExt->mBmpExtHdrBlock.mLatch.unlock(  ) 
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * IBChunkID가 속하는 현재 추적중인 differential BmpExt를 가져온다.
 * 
 * aSlot        - [IN] 추적하려는 datafile의 DataFileDesc slot
 * aIBChunkID   - [IN] 추적하려는 IBChunkID
 * aCurBmpExt   - [OUT] IBChunkID가 속하는 BmpExt
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::getCurBmpExt(       
                        smriCTDataFileDescSlot   * aSlot,
                        UInt                       aIBChunkID,
                        smriCTBmpExt            ** aCurBmpExt )
{
    UInt                sBmpExtSeq;
    smriCTBmpExt      * sCurBmpExt;
    smriCTBmpExt      * sAllocCurBmpExt;
    UInt                sState = 0;

    IDE_DASSERT( aSlot       != NULL );
    IDE_DASSERT( aCurBmpExt  != NULL );

    /* IBChunkID가 속한 BmpExt가 List에서 몇번째 BmpExt인지 구한다. */
    sBmpExtSeq = aIBChunkID / mBmpExtBitCnt;


    /* BmpExtSeq에 해당하는 BmpExt를 가져온다. */
    IDE_TEST( getCurBmpExtInternal( aSlot, sBmpExtSeq, &sCurBmpExt ) 
              != IDE_SUCCESS );

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/ ) 
              != IDE_SUCCESS );
    sState = 1;

    while(1)
    {
        if( sCurBmpExt == NULL )
        {
            IDE_TEST( wait4FlushAndExtend() != IDE_SUCCESS );
 
            /* 
             * CTHdr의 mutext를 잡은 후에 다른쓰레드에의해 BmpExt가 새로 할당
             * 되었는지 확인한다.
             */
             IDE_TEST( getCurBmpExtInternal( aSlot, sBmpExtSeq, &sCurBmpExt ) 
                       != IDE_SUCCESS );
 
            if( sCurBmpExt != NULL )
            {
                /* IBChunkID가 속한 BmpExt를 찾은경우 */
            }
            else
            {
                /* 
                 * IBChunkID가 속한 BmpExt를 못 찾은경우 DataFileDescSlot에 새로운
                 * BmpExt를 할당한다.
                 */
                IDE_TEST( addBmpExt2DataFileDescSlot( aSlot, 
                                                      &sAllocCurBmpExt ) 
                          != IDE_SUCCESS );
 
                sCurBmpExt = sAllocCurBmpExt;

                if( sBmpExtSeq == sCurBmpExt->mBmpExtHdrBlock.mBmpExtSeq )
                {
                    /* 제대로된 BmpExtSeq를 가진 BmpExt를 할당 받은 경우 */
                    break;
                }
                else
                {
                    if( sBmpExtSeq > sCurBmpExt->mBmpExtHdrBlock.mBmpExtSeq )
                    {
                        /* BmpExt를 할당받았지만 IBCunkID가 속한 BmpExt를 할당받지 못한경우.
                         * 여러개의 BmpExt를 할당해야하는경우 다시 처음으로 돌아가
                         * BmpExt를 할당받는다.
                         */
                        sCurBmpExt = NULL;
                    }
                    else
                    {
                        /* sBmpExtSeq보다 새로할당받은 BmpExt의 mBmpExtSeq이 클 수 없다. */
                        IDE_ERROR(0);
                    }
                }
            }
        }
        else
        {
            break;
        }
    }
 
    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS );

    IDE_DASSERT( sBmpExtSeq == sCurBmpExt->mBmpExtHdrBlock.mBmpExtSeq );
    
    *aCurBmpExt = sCurBmpExt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    switch( sState )
    {
        case 1:
            IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
        default:
            break;
    }   
    
    return IDE_FAILURE;
}

/***********************************************************************
 * aBmpExtSeq에 해당하는 현재 추적중인 differential BmpExt를 가져온다.
 * 
 * aSlot        - [IN] 추적하려는 datafile의 DataFileDesc slot
 * aBmpExtSeq   - [IN] 찾으려는 BmpExt의 sequence 번호
 * aCurBmpExt   - [OUT] aBmpExtSeq에 해당하는 BmpExt
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::getCurBmpExtInternal( 
                                        smriCTDataFileDescSlot   * aSlot,
                                        UInt                       aBmpExtSeq,
                                        smriCTBmpExt            ** aCurBmpExt )
{
    UInt            sBmpExtBlockID;
    UInt            sFirstBmpExtBlockID;
    smriCTBmpExt  * sCurBmpExt;

    IDE_DASSERT( aSlot       != NULL );
    IDE_DASSERT( aCurBmpExt  != NULL );

    /* 
     * IBChunkID가 속한 BmpExt가 SMRI_CT_BMP_EXT_LIST_HINT_CNT이내에 있다면
     * mBmpExtListHint에서 바로 가져온다. 그렇지 않은경우 BmpExtList를 탐색해
     * 찾는다.
     */
    if( aBmpExtSeq < SMRI_CT_BMP_EXT_LIST_HINT_CNT )
    {
        sBmpExtBlockID = 
                    aSlot->mBmpExtList[aSlot->mCurTrackingListID]
                                .mBmpExtListHint[aBmpExtSeq];

        if( sBmpExtBlockID == SMRI_CT_INVALID_BLOCK_ID )
        {
            sCurBmpExt = NULL;
        }
        else
        {
            getBlock( sBmpExtBlockID, (void**)&sCurBmpExt );
            
            IDE_ERROR( sCurBmpExt->mBmpExtHdrBlock.mBmpExtSeq == aBmpExtSeq );
        }
    }
    else
    {
        sFirstBmpExtBlockID = 
        aSlot->mBmpExtList[ aSlot->mCurTrackingListID ].mBmpExtListHint[0];

        IDE_ERROR( sFirstBmpExtBlockID != SMRI_CT_INVALID_BLOCK_ID );

        getBlock( sFirstBmpExtBlockID, (void**)&sCurBmpExt );
        
        while(1)
        {
            if( aBmpExtSeq == sCurBmpExt->mBmpExtHdrBlock.mBmpExtSeq )
            {
                break;
            }

            getBlock( sCurBmpExt->mBmpExtHdrBlock.mNextBmpExtHdrBlockID,
                      (void**)&sCurBmpExt );

            if( sCurBmpExt->mBmpExtHdrBlock.mBmpExtSeq == 0 )
            {
                sCurBmpExt = NULL;
                break;
            }
        }
    }

    *aCurBmpExt = sCurBmpExt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * IBChunkID에 해당하는 bit의 위치를 구하고 bit를 set한다.
 * 
 * aCurBmpExt   - [IN] bit를 set할 BmpExt
 * aIBChunkID   - [IN] bit의 위치를 구할 IBChunkID
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::calcBitPositionAndSet( 
                                            smriCTBmpExt           * aCurBmpExt,
                                            UInt                     aIBChunkID,
                                            smriCTDataFileDescSlot * aSlot )
{
    smriCTBmpExt  * sCumBmpExt;
    UInt            sRelativeIBChunkID;
    UInt            sBmpBlockIdx;
    UInt            sBitOffset;
    UInt            sByteIdx;
    UInt            sBitPosition;

    IDE_DASSERT( aCurBmpExt != NULL );

    sRelativeIBChunkID  = aIBChunkID 
                          - ( mBmpExtBitCnt 
                          * aCurBmpExt->mBmpExtHdrBlock.mBmpExtSeq );

    sBmpBlockIdx        = sRelativeIBChunkID / mBmpBlockBitCnt;

    sBitOffset          = sRelativeIBChunkID 
                          - ( mBmpBlockBitCnt
                          * sBmpBlockIdx );
    
    sByteIdx        = sBitOffset / 8;
    sBitPosition    = sBitOffset % 8;

    IDE_ERROR( sBitOffset < mBmpBlockBitCnt );
    
    /* Differential Bitmap */
    IDE_TEST( setBit( aCurBmpExt, 
                      sBmpBlockIdx, 
                      sByteIdx, 
                      sBitPosition,
                      &aSlot->mBmpExtList[ aSlot->mCurTrackingListID ] )
              != IDE_SUCCESS ); 

    getBlock( aCurBmpExt->mBmpExtHdrBlock.mCumBmpExtHdrBlockID, 
                        (void**)&sCumBmpExt );

    IDU_FIT_POINT("smriChangeTrackingMgr::calcBitPositionAndSet::wait");

    /* Cumulative Bitmap */
    IDE_TEST( setBit( sCumBmpExt, 
                      sBmpBlockIdx, 
                      sByteIdx, 
                      sBitPosition,
                      &aSlot->mBmpExtList[ SMRI_CT_CUMULATIVE_LIST ] ) 
              != IDE_SUCCESS ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * IBChunk의 bit를 set한다.
 * 
 * aBmpExt          - [IN] bit를 set할 BmpExt
 * aBmpBlockIdx     - [IN] bit가 속한 BmpBlock의 Idx
 * aByteIdx         - [IN] bit가 속한 byte의 Idx
 * aBitPosition     - [IN] byte내에서의 bit위치
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::setBit( smriCTBmpExt     * aBmpExt,
                                      UInt               aBmpBlockIdx,
                                      UInt               aByteIdx,
                                      UInt               aBitPosition,
                                      smriCTBmpExtList * aBmpExtList )
{
    smriCTBmpBlock        * sBmpBlock;
    SChar                 * sTargetByte;
    SChar                   sTempByte = 0;
    UInt                    sState    = 0;

    IDE_DASSERT( aBmpExt != NULL );

    sBmpBlock       = &aBmpExt->mBmpBlock[ aBmpBlockIdx ];
    sTargetByte     = &sBmpBlock->mBitmap[ aByteIdx ];

    sTempByte |= (1 << aBitPosition);

    /* bit가 set되어있지 않은경우에만 bit를 set한다. */
    if( ( *sTargetByte & sTempByte ) == 0 )
    {
        IDE_TEST( sBmpBlock->mBlockHdr.mMutex.lock( NULL ) != IDE_SUCCESS );
        sState = 1;

        *sTargetByte |= sTempByte;
        (void)idCore::acpAtomicInc32( &aBmpExtList->mSetBitCount );

        sState = 0;
        IDE_TEST( sBmpBlock->mBlockHdr.mMutex.unlock() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sBmpBlock->mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
}


/*********************************************************************** 
 * Bimap을 스캔하고 IBchunk를 복사한다.
 *
 * aDataFielDescSlot    - [IN] bitmap을 검색할 DataFileDescSlot
 * aBackupInfo          - [IN] 어떤 backup을 수행할지에대한 정보
 * aSrcFile             - [IN] backup을 수행할 원본 데이터파일
 * aDestFile            - [IN] 생성될 backup파일
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::makeLevel1BackupFile( 
                              smriCTDataFileDescSlot * aDataFileDescSlot, 
                              smriBISlot             * aBackupInfo,
                              iduFile                * aSrcFile,
                              iduFile                * aDestFile,
                              smriCTTBSType            aTBSType,
                              scSpaceID                aSpaceID,
                              UInt                     aFileNo )
{
    smriCTBmpExt  * sBmpExt;
    UInt            sIBChunkSize;
    UInt            sBmpExtListLen;
    UInt            sBmpExtHdrBlockID;
    UInt            sFirstBmpExtHdrBlockID;
    UInt            sListID;
    smmTBSNode    * sTBSNode;
    UInt            sBmpExtCnt = 0;
    UInt            sState = 0;

    IDE_DASSERT( aDataFileDescSlot     != NULL );
    IDE_DASSERT( aBackupInfo           != NULL );
    IDE_DASSERT( aSrcFile              != NULL );
    IDE_DASSERT( aDestFile             != NULL );

    /* 
     * Differetion backup인지 cumulative backup인지 판단하여 해당하는 BmpExt를
     * 가져온다.
     */
    if( (aBackupInfo->mBackupType & SMI_BACKUP_TYPE_DIFFERENTIAL) != 0 )
    {
        IDE_ERROR( (aBackupInfo->mBackupType & SMI_BACKUP_TYPE_CUMULATIVE) 
                    == 0 );

        getFirstDifBmpExt( aDataFileDescSlot, &sFirstBmpExtHdrBlockID );

        sListID = ( aDataFileDescSlot->mCurTrackingListID + 1 ) % 2;
    }
    else
    {
        IDE_ERROR( (aBackupInfo->mBackupType & SMI_BACKUP_TYPE_DIFFERENTIAL) 
                    == 0 );

        IDE_ERROR( (aBackupInfo->mBackupType & SMI_BACKUP_TYPE_CUMULATIVE) 
                    != 0 );

        getFirstCumBmpExt( aDataFileDescSlot, &sFirstBmpExtHdrBlockID );
        sListID = SMRI_CT_CUMULATIVE_LIST; 
    }

    IDE_ERROR( sFirstBmpExtHdrBlockID != SMRI_CT_INVALID_BLOCK_ID );

    getIBChunkSize( &sIBChunkSize );
    mSrcFile        = aSrcFile; 
    mDestFile       = aDestFile; 
    mCopySize       = aDataFileDescSlot->mPageSize * sIBChunkSize;
    mTBSType        = aDataFileDescSlot->mTBSType;
    mIBChunkCnt     = 0;
    mIBChunkID      = 0;

    mFileNo         = aFileNo;

    IDE_ERROR( mTBSType == aTBSType );

    /* MEM TBS일 경우 split된 두번째 파일의 pageid를 구하기 위해
     * mSplitFilePageCount값을 구한다. */
    if( aTBSType == SMRI_CT_MEM_TBS )
    {
        IDE_ERROR( sctTableSpaceMgr::isMemTableSpace( aSpaceID ) == ID_TRUE );
        IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID , (void**)&sTBSNode )
                  != IDE_SUCCESS );
        mSplitFilePageCount = sTBSNode->mTBSAttr.mMemAttr.mSplitFilePageCount;
    }
    else
    {
        mSplitFilePageCount = 0;
    }


    /* IBChunk를 복사할 버퍼를 할당 받는다. */
    /* smriChangeTrackingMgr_makeLevel1BackupFile_calloc_IBChunkBuffer.tc */
    IDU_FIT_POINT("smriChangeTrackingMgr::makeLevel1BackupFile::calloc::IBChunkBuffer");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                 1,
                                 mCopySize,
                                 (void**)&mIBChunkBuffer )
              != IDE_SUCCESS );              
    sState = 1;

    IDE_ERROR( aDataFileDescSlot->mBmpExtCnt % 
                        SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT == 0 );
    
    /* BmpExtList의 길이를 구한다 */
    sBmpExtListLen = aDataFileDescSlot->mBmpExtCnt 
                     / SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT;

    sBmpExtHdrBlockID = sFirstBmpExtHdrBlockID;

    for( sBmpExtCnt = 0; sBmpExtCnt < sBmpExtListLen; sBmpExtCnt++ )
    {
        getBlock( sBmpExtHdrBlockID, (void **)&sBmpExt ); 

        /* 한 BmpExt에 대한 bitmap을 검색한다. */
        IDE_TEST( makeLevel1BackupFilePerEachBmpExt( 
                                    sBmpExt, 
                                    sBmpExt->mBmpExtHdrBlock.mBmpExtSeq )
                  != IDE_SUCCESS ); 

        sBmpExtHdrBlockID = sBmpExt->mBmpExtHdrBlock.mNextBmpExtHdrBlockID;
    }

    IDE_ERROR( sBmpExtHdrBlockID == sFirstBmpExtHdrBlockID );
    
    if( mIBChunkCnt != aDataFileDescSlot->mBmpExtList[ sListID ].mSetBitCount )
    {
        ideLog::log( IDE_DUMP_0,
                "==================================================================\n"
                " smriChangeTrackingMgr::makeLevel1BackupFile\n"
                "==================================================================\n"
                "IBChunkCnt             : %u\n"
                "SetBitCount            : %u\n"
                "ListID                 : %u\n"
                "==================================================================\n",
                mIBChunkCnt,
                aDataFileDescSlot->mBmpExtList[ sListID ].mSetBitCount,
                sListID );

        IDE_TEST(1);
    }

    IDE_TEST( mDestFile->syncUntilSuccess( smLayerCallback::setEmergency )
              != IDE_SUCCESS );
    aBackupInfo->mIBChunkCNT  = mIBChunkCnt;
    aBackupInfo->mIBChunkSize = sIBChunkSize;

    sState = 0;
    IDE_TEST( iduMemMgr::free( mIBChunkBuffer ) != IDE_SUCCESS );              

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if( sState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free( mIBChunkBuffer ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * 각 BmpExt의 bit를 확인한다.
 *
 * aBmpExt      - [IN] 검색할 BmpExt
 * aBmpExtSeq   - [IN] BmpExtList에서의 BmpExt의 위치
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::makeLevel1BackupFilePerEachBmpExt( 
                                                     smriCTBmpExt * aBmpExt, 
                                                     UInt           aBmpExtSeq )
{
    UInt                sBmpBlockIdx;
    smriCTBmpBlock    * sBmpBlock;

    IDE_DASSERT( aBmpExt     != NULL );

    for( sBmpBlockIdx = 0; 
         sBmpBlockIdx < SMRI_CT_EXT_BLOCK_CNT_EXCEPT_META_BLOCK; 
         sBmpBlockIdx++ )
    {
        sBmpBlock = &aBmpExt->mBmpBlock[ sBmpBlockIdx ];
        /* 한 BmpExtBlock을 검색한다. */
        IDE_TEST( makeLevel1BackupFilePerEachBmpBlock( sBmpBlock, 
                                                       aBmpExtSeq, 
                                                       sBmpBlockIdx )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * 각 BmpBlock의 bit를 확인한다.
 *
 * aBmpBlock    - [IN] 검색할 BmpBlock
 * aBmpExtSeq   - [IN] BmpExtList에서의 BmpExt의 위치
 * aBmpBlockIdx - [IN] BmpExt에서의 BmpBlock의 위치
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::makeLevel1BackupFilePerEachBmpBlock(
                                         smriCTBmpBlock   * aBmpBlock, 
                                         UInt               aBmpExtSeq, 
                                         UInt               aBmpBlockIdx )
{
    UInt        sBmpByteIdx;
    SChar     * sBmpByte;

    IDE_DASSERT( aBmpBlock != NULL );

    for( sBmpByteIdx = 0; 
         sBmpByteIdx < mBmpBlockBitmapSize;
         sBmpByteIdx++ )
    {
        sBmpByte = &aBmpBlock->mBitmap[ sBmpByteIdx ];

        /* 한 BmpByte를 검색한다. */
        IDE_TEST( makeLevel1BackupFilePerEachBmpByte( sBmpByte, 
                                                      aBmpExtSeq, 
                                                      aBmpBlockIdx, 
                                                      sBmpByteIdx )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************** 
 * 각 BmpByte의 bit를 확인하고 bit에 해당하는 IBChunk를 복사한다.
 *
 * aBmpBlock    - [IN] 검색할 BmpBlock
 * aBmpExtSeq   - [IN] BmpExtList에서의 BmpExt의 위치
 * aBmpBlockIdx - [IN] BmpExt에서의 BmpBlock의 위치
 * aBmpByteIdx  - [IN] BmpBlock에서의 BmpByte의위치
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::makeLevel1BackupFilePerEachBmpByte( 
                                                      SChar * aBmpByte,
                                                      UInt    aBmpExtSeq,
                                                      UInt    aBmpBlockIdx,
                                                      UInt    aBmpByteIdx )
{
    UInt        sBmpBitIdx;
    UInt        sTempByte;
    UInt        sIBChunkID;

    IDE_DASSERT( aBmpByte    != NULL );

    for( sBmpBitIdx = 0; sBmpBitIdx < SMRI_CT_BIT_CNT ; sBmpBitIdx++ )
    {
        sTempByte = 0;
        sTempByte = ( 1 << sBmpBitIdx );
        
        /* BmpByte에서 bit가 1로 set된 위치를 찾는다. */
        if( ( *aBmpByte & sTempByte ) != 0 )
        {
            sIBChunkID = ( aBmpExtSeq * mBmpExtBitCnt )
                         + ( aBmpBlockIdx * mBmpBlockBitCnt )
                         + ( aBmpByteIdx * SMRI_CT_BIT_CNT )
                         + sBmpBitIdx;

            IDE_ASSERT( sIBChunkID == mIBChunkID );

            /* 1로 set된 IBChunk를 backup파일로 복사 한다. */
            IDE_TEST( copyIBChunk( sIBChunkID ) != IDE_SUCCESS );

            mIBChunkCnt++;
        }
        mIBChunkID++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************** 
 * IBChunk를 백업한다.
 * PROJ-2133 
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::copyIBChunk( UInt aIBChunkID )
{
    size_t              sReadOffset;
    size_t              sWriteOffset;
    ULong               sDestFileSize = 0;
    size_t              sReadSize;
    UInt                sIBChunkSize;
    UInt                i;
    UInt                sValidationPageCnt;
    sdpPhyPageHdr     * sDiskPageHdr;
    sdpPhyPageHdr     * sDiskPageHdr4Validation;
    smpPersPageHeader * sMemPageHdr;
    smpPersPageHeader * sMemPageHdr4Validation;
    scPageID            sPageIDFromIBChunkID;
    scPageID            sPageIDFromReadOffset;
    idBool              sIsValid = ID_FALSE;

    /* 원본파일에서 IBChunk를 읽어올 위치를 구한다. */
    sReadOffset = SM_DBFILE_METAHDR_PAGE_SIZE;
    sReadOffset += aIBChunkID * mCopySize;

    /* 백업파일에서 IBChunk를 쓸 위치를 구한다. append olny */
    IDE_TEST( mDestFile->getFileSize( &sDestFileSize ) != IDE_SUCCESS );

    if( sDestFileSize <= SM_DBFILE_METAHDR_PAGE_SIZE )
    {
        sWriteOffset = SM_DBFILE_METAHDR_PAGE_SIZE;
    }
    else
    {
        sWriteOffset = sDestFileSize;
    }

    IDE_TEST( mSrcFile->read( NULL,  //aStatistics
                              sReadOffset, 
                              (void*)mIBChunkBuffer, 
                              mCopySize, 
                              &sReadSize ) 
              != IDE_SUCCESS );

    IDE_ERROR( sReadSize != 0 );

    getIBChunkSize( &sIBChunkSize );

    if( mTBSType == SMRI_CT_DISK_TBS )
    {
        IDE_ASSERT( (sReadSize % SD_PAGE_SIZE) == 0 );

        sDiskPageHdr = ( sdpPhyPageHdr * )mIBChunkBuffer;

        sPageIDFromIBChunkID = aIBChunkID * sIBChunkSize;

        sValidationPageCnt = sReadSize / SD_PAGE_SIZE;
        for( i = 0; i < sValidationPageCnt; i++ )
        {
            sDiskPageHdr4Validation = (sdpPhyPageHdr *)&mIBChunkBuffer[i * SD_PAGE_SIZE];

            if( SD_MAKE_FPID(sDiskPageHdr4Validation->mPageID) == sPageIDFromIBChunkID + i )
            {
                sIsValid = ID_TRUE;
            }
            else
            {
                IDE_ERROR( sDiskPageHdr4Validation->mPageType == SDP_PAGE_UNFORMAT );

                /* unformat page에는 pageid가 세팅되어 있지 않다. 계산하여 구한
                 * pageid를 넣는다.*/
                sDiskPageHdr4Validation->mPageID = sPageIDFromIBChunkID + i;
            }
        }

        sPageIDFromReadOffset = (sReadOffset - SM_DBFILE_METAHDR_PAGE_SIZE)
                                / SD_PAGE_SIZE ;

        IDE_DASSERT( sPageIDFromReadOffset == sPageIDFromIBChunkID );
        IDE_ERROR( (sIsValid == ID_TRUE) || (sPageIDFromReadOffset == sPageIDFromIBChunkID) );

        if( SD_MAKE_FPID(sDiskPageHdr->mPageID) != sPageIDFromIBChunkID )
        {
            ideLog::log( IDE_DUMP_0,
                         "==================================================================\n"
                         " Storage Manager Dump Info for Copy Incremental Backup Chunk(Disk)\n"
                         "==================================================================\n"
                         "pageType                : %u\n"
                         "pageID                  : %u\n"
                         "IBChunkID * IBChunkSize : %u\n"
                         "srcFileName             : %s\n"
                         "destFileName            : %s\n"
                         "==================================================================\n",
                         sDiskPageHdr->mPageType,
                         SD_MAKE_FPID(sDiskPageHdr->mPageID),
                         (ULong)aIBChunkID * sIBChunkSize,
                         mSrcFile->getFileName(),
                         mDestFile->getFileName());
        
            IDE_ASSERT(0)
        }
    }
    else
    {
        if( mTBSType == SMRI_CT_MEM_TBS )
        {
            IDE_ASSERT( (sReadSize % SM_PAGE_SIZE) == 0 );

            sMemPageHdr = (smpPersPageHeader *)mIBChunkBuffer;

            if( mFileNo == 0)
            {
                sPageIDFromIBChunkID = aIBChunkID * sIBChunkSize;

                sPageIDFromReadOffset = (sReadOffset - SM_DBFILE_METAHDR_PAGE_SIZE)
                                        / SM_PAGE_SIZE ;
            }
            else
            {
                /* chkpt의 두번째 파일부터는 첫번째 pageid가 0이 아닌
                 * mSplitFilePageCount + 1의 값을 가진다. */
                sPageIDFromIBChunkID = (aIBChunkID * sIBChunkSize)
                                       + ( mFileNo * mSplitFilePageCount )
                                       + 1;

                sPageIDFromReadOffset = ((sReadOffset - SM_DBFILE_METAHDR_PAGE_SIZE)
                                        / SM_PAGE_SIZE)
                                        + ( mFileNo * mSplitFilePageCount )
                                        + 1;
            }

            IDE_DASSERT( sPageIDFromReadOffset == sPageIDFromIBChunkID );

            sValidationPageCnt = sReadSize / SM_PAGE_SIZE;
            for( i = 0; i < sValidationPageCnt; i ++ )
            {
                sMemPageHdr4Validation = (smpPersPageHeader *)&mIBChunkBuffer[i * SM_PAGE_SIZE];

                if( sMemPageHdr4Validation->mSelfPageID == sPageIDFromIBChunkID + i )
                {
                    sIsValid = ID_TRUE;
                }
                else
                {
                    IDE_ERROR( ( SMP_GET_PERS_PAGE_TYPE( sMemPageHdr4Validation) != SMP_PAGETYPE_FIX ) &&
                               ( SMP_GET_PERS_PAGE_TYPE( sMemPageHdr4Validation) != SMP_PAGETYPE_VAR ) );

                    /* unformat page에는 pageid가 세팅되어 있지 않다. 계산하여 구한
                     * pageid를 넣는다.*/
                    sMemPageHdr4Validation->mSelfPageID = sPageIDFromIBChunkID + i;
                }
            }

            IDE_ERROR( (sIsValid == ID_TRUE) || (sPageIDFromReadOffset == sPageIDFromIBChunkID) );

            if( sMemPageHdr->mSelfPageID != sPageIDFromIBChunkID )
            {
                ideLog::log( IDE_DUMP_0,
                             "====================================================================\n"
                             " Storage Manager Dump Info for Copy Incremental Backup Chunk(Memory)\n"
                             "====================================================================\n"
                             "pageID                  : %u\n"
                             "IBChunkID * IBChunkSize : %u\n"
                             "srcFileName             : %s\n"
                             "destFileName            : %s\n"
                             "====================================================================\n",
                             sMemPageHdr->mSelfPageID,
                             (ULong)aIBChunkID * sIBChunkSize,
                             mSrcFile->getFileName(),
                             mDestFile->getFileName());
            
                IDE_ASSERT(0)
            }
        }
        else
        {
            IDE_ASSERT(0);
        }
    }

    IDE_TEST( mDestFile->write( NULL, //aStatistics
                                sWriteOffset,
                                mIBChunkBuffer,
                                sReadSize ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * incremental Backup을 수행한다.
 * 
 * DataFileDescSlot - [IN] 백업을 수행하려는 데이터파일의 DataFileDescSlot
 * SrcFile          - [IN] 백업대상이 되는 파일
 * DestFile         - [IN] 생성되는 백업파일
 * BackupInfo       - [IN/OUT] 백업수행중 필요한 정보와 백업수행 결과 정보 전달
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::performIncrementalBackup(
                                    smriCTDataFileDescSlot * aDataFileDescSlot,
                                    iduFile                * aSrcFile,
                                    iduFile                * aDestFile,
                                    smriCTTBSType            aTBSType,
                                    scSpaceID                aSpaceID,
                                    UInt                     aFileNo,
                                    smriBISlot             * aBackupInfo )
{
    UInt    sNewBmpExtListID;

    /* switch할 BmpExtListID를 구한다. */
    sNewBmpExtListID = (aDataFileDescSlot->mCurTrackingListID + 1) % 2;

    /* sNewBmpExtList의 bitmap들을 0으로 초기화한다. */
    IDE_TEST( initBmpExtListBlocks( aDataFileDescSlot, sNewBmpExtListID )
              != IDE_SUCCESS );

    /* BmpExtList를 switch한다. */
    IDE_TEST( switchBmpExtListID( aDataFileDescSlot ) != IDE_SUCCESS );

    /* bitmap을 스캔하고 IBChunk를 백업파일로 복사한다. */
    IDE_TEST( makeLevel1BackupFile( aDataFileDescSlot,
                                    aBackupInfo,
                                    aSrcFile,
                                    aDestFile,
                                    aTBSType,
                                    aSpaceID,
                                    aFileNo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * 블럭을 가져온다.
 * 
 * DataFileDescSlotID - [IN] 가져올 블럭의 ID
 * aSpaceID           - [IN] tablespace ID
 * aDataFileNum       - [IN] dataFile Num( or ID)
 * aResult            - [OUT] 블럭을 전달할 out인자
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::isNeedAllocDataFileDescSlot( 
                                    smiDataFileDescSlotID * aDataFileDescSlotID,
                                    scSpaceID               aSpaceID,
                                    UInt                    aDataFileNum, 
                                    idBool                * aResult )
{
    smriCTDataFileDescBlock * sDataFileDescBlock;
    smriCTDataFileDescSlot  * sDataFileDescSlot;
    UInt                      sDataFileDescSlotIdx = 0; 
    idBool                    sNeedDataFileDescSlotAlloc;

    getBlock( aDataFileDescSlotID->mBlockID,
              (void**)&sDataFileDescBlock );

    sDataFileDescSlot = 
        &sDataFileDescBlock->mSlot[ aDataFileDescSlotID->mSlotIdx ];
    
    sDataFileDescSlotIdx |= ( 1 << aDataFileDescSlotID->mSlotIdx );

    /* loganchor및 데이터파일 헤더에 DataFileDescSlotID가 저장되어있지만
     * CTFile이 flush되지 않아 DataFileDescSlotID가 할당되지 않은 상태
     */
    if( (sDataFileDescBlock->mAllocSlotFlag & 
         sDataFileDescSlotIdx) == 0 )
    {
        sNeedDataFileDescSlotAlloc = ID_TRUE;
    }
    else
    {
        if( (aSpaceID != sDataFileDescSlot->mSpaceID) &&
            (aDataFileNum != sDataFileDescSlot->mFileID) )
        {
            /* DataFileDescSlot이 다른 데이터파일에 의해 이미 사용중인
             * 경우는 발생할수 없음
             */
            IDE_ERROR_MSG( 0, 
                           "SpaceID                    : %"ID_UINT32_FMT"\n"
                           "FileID                     : %"ID_UINT32_FMT"\n"
                           "DataFileDescSlot->mSpaceID : %"ID_UINT32_FMT"\n"
                           "DataFileDescSlot->mFileID  : %"ID_UINT32_FMT"\n",
                           aSpaceID,
                           aDataFileNum,
                           sDataFileDescSlot->mSpaceID,
                           sDataFileDescSlot->mFileID );
        }
        else
        {
            /* DataFileDescSlot이 제대로 할당되어있는 상태 */
            sNeedDataFileDescSlotAlloc = ID_FALSE;
        }
    }

    *aResult = sNeedDataFileDescSlotAlloc;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * DataFile desc Slot을 가져온다.
 * 
 * aSlotID - [IN] 가져올 Slot의 ID
 * aSlot   - [OUT] Slot을 전달할 out인자
 *
 *
 **********************************************************************/
void smriChangeTrackingMgr::getDataFileDescSlot( 
                                     smiDataFileDescSlotID    * aSlotID,
                                     smriCTDataFileDescSlot  ** aSlot )
{
    smriCTDataFileDescBlock   * sDataFileDescSlotBlock;

    IDE_DASSERT( aSlotID != NULL );
    IDE_DASSERT( aSlot   != NULL );

    getBlock( aSlotID->mBlockID, (void**)&sDataFileDescSlotBlock );

    *aSlot = &sDataFileDescSlotBlock->mSlot[ aSlotID->mSlotIdx ];

    return;
}

/***********************************************************************
 * 블럭을 가져온다.
 * 
 * aBlockID - [IN] 가져올 블럭의 ID
 * aBlock   - [OUT] 블럭을 전달할 out인자
 *
 **********************************************************************/
void smriChangeTrackingMgr::getBlock( UInt aBlockID, void ** aBlock )
{
    UInt            sCTBodyIdx;
    UInt            sBlockIdx;
    smriCTBody    * sCTBody;

    IDE_DASSERT( aBlock != NULL );

    sCTBodyIdx  = aBlockID / mCTBodyBlockCnt;
    sCTBody     = mCTBodyPtr[ sCTBodyIdx ] ;
    sBlockIdx   = aBlockID - ( sCTBodyIdx * mCTBodyBlockCnt );

    *aBlock = (void *)&sCTBody->mBlock[ sBlockIdx ];

    return;
}

/***********************************************************************
 * NewBmpExtList의 첫번째 블럭 ID를 가져온다.
 * 
 * aSlot            - [IN] 가져올 블럭이있는 slot
 * aBmpExtBlockID   - [OUT] 블럭을 전달할 out인자
 *
 **********************************************************************/
void smriChangeTrackingMgr::getFirstDifBmpExt( 
                                   smriCTDataFileDescSlot   * aSlot,
                                   UInt                     * aBmpExtBlockID)
{
    UInt    sNewBmpExtListID;
    
    IDE_DASSERT( aSlot           != NULL );
    IDE_DASSERT( aBmpExtBlockID  != NULL );

    sNewBmpExtListID = ( aSlot->mCurTrackingListID + 1 ) % 2;

    *aBmpExtBlockID = 
                aSlot->mBmpExtList[ sNewBmpExtListID ].mBmpExtListHint[0];

    return;
}

/***********************************************************************
 * NewBmpExtList의 첫번째 블럭 ID를 가져온다.
 * 
 * aSlot            - [IN] 가져올 블럭이있는 slot
 * aBmpExtBlockID   - [OUT] 블럭을 전달할 out인자
 *
 **********************************************************************/
void smriChangeTrackingMgr::getFirstCumBmpExt( 
                                    smriCTDataFileDescSlot   * aSlot,
                                    UInt                     * aBmpExtBlockID)
{

    IDE_DASSERT( aSlot           != NULL );
    IDE_DASSERT( aBmpExtBlockID  != NULL );

    *aBmpExtBlockID = 
                aSlot->mBmpExtList[SMRI_CT_CUMULATIVE_LIST].mBmpExtListHint[0];

    return;
}

/***********************************************************************
 * BmpExtList에 속한 모든 Bmp block들을 초기화 한다.
 *
 * aBmpExtList - [IN] 초기화할 BmpExt의 List
 *
 *
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::initBmpExtListBlocks( 
                                    smriCTDataFileDescSlot * aDataFileDescSlot,
                                    UInt                     aBmpExtListID )
{
    smriCTBmpExt      * sBmpExt;
    smriCTBmpBlock    * sBmpBlock;
    smriCTBmpExtList  * sBmpExtList;
    UInt                sFirstBmpExtBlockID;
    UInt                sBmpBlockIdx;
    UInt                sBmpExtCnt;
    UInt                sBmpExtListLen;
    UInt                sState = 0;

    IDE_DASSERT( aDataFileDescSlot != NULL );
    IDE_DASSERT( aBmpExtListID < SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT );

    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.lock( NULL /*aStatistics*/ ) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( wait4FlushAndExtend() != IDE_SUCCESS );

    IDE_ERROR( aDataFileDescSlot->mBmpExtCnt % 
                        SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT == 0 );
    
    /* BmpExtList의 길이를 구한다 */
    sBmpExtListLen = aDataFileDescSlot->mBmpExtCnt 
                     / SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT;

    sBmpExtList = &aDataFileDescSlot->mBmpExtList[ aBmpExtListID ];

    sFirstBmpExtBlockID = sBmpExtList->mBmpExtListHint[0];

    getBlock( sFirstBmpExtBlockID, (void**)&sBmpExt );

    for( sBmpExtCnt = 0; sBmpExtCnt < sBmpExtListLen; sBmpExtCnt++ )
    {
        for( sBmpBlockIdx = 0; 
             sBmpBlockIdx < SMRI_CT_EXT_BLOCK_CNT_EXCEPT_META_BLOCK; 
             sBmpBlockIdx++ )
        {
            sBmpBlock = &sBmpExt->mBmpBlock[ sBmpBlockIdx ];
            idlOS::memset( sBmpBlock->mBitmap,
                           0,
                           mBmpBlockBitmapSize );
        }

        getBlock( sBmpExt->mBmpExtHdrBlock.mNextBmpExtHdrBlockID, 
                            (void**)&sBmpExt );
    }

    IDE_ERROR( sBmpExtListLen == sBmpExtCnt );

    IDU_FIT_POINT("smriChangeTrackingMgr::initBmpExtListBlocks::wait");

    sBmpExtList->mSetBitCount = 0;

    sState = 0;
    IDE_TEST( mCTFileHdr.mBlockHdr.mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( mCTFileHdr.mBlockHdr.mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * 블럭의 checksum을 검사한다.
 * 
 * aBlockHdr    - [IN] CheckSum을 계산할 블럭의 헤더
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::checkBlockCheckSum( smriCTBlockHdr * aBlockHdr )
{

    UChar * sCheckSumStartPtr;
    UInt    sCheckSumValue;

    IDE_DASSERT( aBlockHdr != NULL );

    sCheckSumStartPtr = (UChar *)( &aBlockHdr->mCheckSum ) +
                        ID_SIZEOF( aBlockHdr->mCheckSum );

    sCheckSumValue = smuUtility::foldBinary( 
                                    sCheckSumStartPtr,
                                    SMRI_CT_BLOCK_SIZE -
                                    ID_SIZEOF( aBlockHdr->mCheckSum ) );

    IDE_TEST_RAISE( sCheckSumValue != aBlockHdr->mCheckSum, 
                    error_invalid_change_tracking_file );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_change_tracking_file );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidChangeTrackingFile, 
                mFile.getFileName()));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * 블럭의 checksum값을 구해 저장한다.
 * 
 * aBlockHdr    - [IN] CheckSum을 계산할 블럭의 헤더
 * 
 **********************************************************************/
void smriChangeTrackingMgr::setBlockCheckSum( smriCTBlockHdr * aBlockHdr )
{

    UChar * sCheckSumStartPtr;
    UInt    sCheckSumValue;

    IDE_DASSERT( aBlockHdr != NULL );

    sCheckSumStartPtr = (UChar *)( &aBlockHdr->mCheckSum ) 
                        + ID_SIZEOF( aBlockHdr->mCheckSum );

    sCheckSumValue = smuUtility::foldBinary( 
                                    sCheckSumStartPtr,
                                    SMRI_CT_BLOCK_SIZE -
                                    ID_SIZEOF( aBlockHdr->mCheckSum ) );
    aBlockHdr->mCheckSum = sCheckSumValue;

    return;
}

/***********************************************************************
 * CTBody의 checksum값을 검사한다.
 * 
 * aCTBody    - [IN] CheckSum을 검사할 CTBody의 ptr
 * aFlushLSN  - [IN] CT파일 헤더에 저장된 FlushLSN
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::checkCTBodyCheckSum( smriCTBody * aCTBody, 
                                                   smLSN        aFlushLSN )
{
    UInt                sExtIdx;
    smriCTMetaExt     * sMetaExt;
    smriCTBmpExt      * sBmpExt;

    IDE_DASSERT( aCTBody != NULL );

    sMetaExt = &aCTBody->mMetaExtent;

    IDE_TEST( smrCompareLSN::isEQ( &sMetaExt->mExtMapBlock.mFlushLSN, 
                                   &aFlushLSN) 
              != ID_TRUE );

    IDE_TEST( checkExtCheckSum( &sMetaExt->mExtMapBlock.mBlockHdr ) 
              != IDE_SUCCESS );

    for( sExtIdx = 0; sExtIdx < mCTBodyBmpExtCnt; sExtIdx++ )
    {
        sBmpExt = &aCTBody->mBmpExtent[ sExtIdx ];

        IDE_TEST( smrCompareLSN::isEQ( &sBmpExt->mBmpExtHdrBlock.mFlushLSN, 
                                       &aFlushLSN) 
                  != ID_TRUE );

        IDE_TEST( checkExtCheckSum( &sBmpExt->mBmpExtHdrBlock.mBlockHdr ) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * CTBody의 checksum값을 계산하고 저장한다.
 * 
 * aCTBody    - [IN] CheckSum을 계산할 CTBody의 ptr
 * aFlushLSN  - [IN] BmpExtHdrBlock에 저장될 FlushLSN
 * 
 **********************************************************************/
void smriChangeTrackingMgr::setCTBodyCheckSum( smriCTBody * aCTBody, 
                                               smLSN        aFlushLSN )
{
    UInt                sExtIdx;
    smriCTMetaExt     * sMetaExt;
    smriCTBmpExt      * sBmpExt;

    IDE_DASSERT( aCTBody != NULL );

    sMetaExt = &aCTBody->mMetaExtent;

    SM_GET_LSN( sMetaExt->mExtMapBlock.mFlushLSN, aFlushLSN );

    setExtCheckSum( &sMetaExt->mExtMapBlock.mBlockHdr );

    for( sExtIdx = 0; sExtIdx < mCTBodyBmpExtCnt; sExtIdx++ )
    {
        sBmpExt = &aCTBody->mBmpExtent[ sExtIdx ];

        SM_GET_LSN( sBmpExt->mBmpExtHdrBlock.mFlushLSN, aFlushLSN );

        setExtCheckSum( &sBmpExt->mBmpExtHdrBlock.mBlockHdr );
    }

    return;
}

/***********************************************************************
 * Ext의 checksum값을 검사한다.
 * 
 * aBlockHdr    - [IN] CheckSum을 계산할 BmpExtHdrBlock
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::checkExtCheckSum( smriCTBlockHdr * aBlockHdr )
{
    UChar * sCheckSumStartPtr;
    UInt    sCheckSumValue;

    IDE_DASSERT( aBlockHdr != NULL );
    IDE_DASSERT( aBlockHdr->mBlockID % SMRI_CT_EXT_BLOCK_CNT == 0 );

    sCheckSumStartPtr = (UChar *)( &aBlockHdr->mCheckSum ) 
                        + ID_SIZEOF( aBlockHdr->mCheckSum );

    sCheckSumValue = smuUtility::foldBinary( 
                            sCheckSumStartPtr,
                            SMRI_CT_EXT_SIZE -
                            ID_SIZEOF( aBlockHdr->mCheckSum ) );

    IDE_TEST( aBlockHdr->mCheckSum != sCheckSumValue );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Ext의 checksum값을 구해 저장한다.
 * 
 * aBlockHdr    - [IN] CheckSum을 계산할 BmpExtHdrBlock
 * 
 **********************************************************************/
void smriChangeTrackingMgr::setExtCheckSum( smriCTBlockHdr * aBlockHdr )
{
    UChar * sCheckSumStartPtr;
    UInt    sCheckSumValue;

    IDE_DASSERT( aBlockHdr != NULL );
    IDE_DASSERT( aBlockHdr->mBlockID % SMRI_CT_EXT_BLOCK_CNT == 0 );

    sCheckSumStartPtr = (UChar *)( &aBlockHdr->mCheckSum ) 
                        + ID_SIZEOF( aBlockHdr->mCheckSum );

    sCheckSumValue = smuUtility::foldBinary( 
                            sCheckSumStartPtr,
                            SMRI_CT_EXT_SIZE -
                            ID_SIZEOF( aBlockHdr->mCheckSum ) );

    aBlockHdr->mCheckSum = sCheckSumValue;

    return;
}

/***********************************************************************
 * MemBase에 저장된 DBName과 CTFile에 저장된 DBName을 비교한다.
 * 
 * aDBName    - [IN] MemBase에 저장된 DBName
 * 
 **********************************************************************/
IDE_RC smriChangeTrackingMgr::checkDBName( SChar * aDBName )
{
    IDE_DASSERT( aDBName != NULL );

    IDE_TEST_RAISE( idlOS::strcmp( mCTFileHdr.mDBName, 
                                   (const char *)aDBName ) != 0,
                    error_invalid_change_tracking_file );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_change_tracking_file );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidChangeTrackingFile, 
                mFile.getFileName()));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
