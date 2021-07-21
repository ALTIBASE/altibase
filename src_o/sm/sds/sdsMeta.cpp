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
 * $Id:$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer 
 * BCB에서 최소한의 정보만 추출해서 frame과 같이 저장한다.
 * Secondary Buffer File 그림과 같은 구조를 가짐
 * Group 내  정보를 이용해 MetaTable을 작성한다. 

   |F|F|F|..|F|F|F|..|F|F|F|..|MetaTable|F|F|F|..|F|F|F|..
   |-Extent-|-Extent-|-Extent-|         |-Extent-|-Extent-|...
   |------     Group   -------|         |------     Group    ---- ...
 **********************************************************************/

#include <smDef.h>
#include <smErrorCode.h>
#include <sdb.h>
#include <sdpPhyPage.h>
#include <sds.h>
#include <sdsReq.h>

/***********************************************************************
 * Description :
 * Meta Table을 쓰기 위해 공간을 확보한다.
 ***********************************************************************/
IDE_RC sdsMeta::initializeStatic( UInt            aGroupCnt,
                                  UInt            aExtCntInGroup,
                                  sdsFile       * aSBufferFile,
                                  sdsBufferArea * aSBufferArea,
                                  sdbBCBHash    * aHashTable )
{
    SInt    sState = 0; 
    UInt    i;

    /* 최대 Group 갯수 */
    mMaxMetaTableCnt = aGroupCnt,
    /* 하나의 Group에 들어가는 Extent의 수 */
    mExtCntInGroup = aExtCntInGroup;
    /* 한 Extent내의 Frame(=meta)의 갯수는 IOB 사이즈와 동일 */
    mFrameCntInExt = smuProperty::getBufferIOBufferSize();

    mSBufferFile = aSBufferFile;
    mSBufferArea = aSBufferArea;
    mHashTable   = aHashTable;

    IDE_ASSERT( SDS_META_TABLE_SIZE > 
                (mExtCntInGroup * mFrameCntInExt * ID_SIZEOF(sdsMetaData)) );

    /* Meta 정보를 저장할 공간을 확보 : 한 Group의 크기: 약 58KB  */
    IDE_TEST_RAISE( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SDS,
                                                 SDS_META_TABLE_SIZE,
                                                 (void**)&mBasePtr,
                                                 (void**)&mBase  )
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sState = 1;

    /* TC/FIT/Limit/sm/sds/sdsMeta_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdsMeta::initialize::malloc1", 
                          ERR_INSUFFICIENT_MEMORY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       (ULong)ID_SIZEOF(sdsMetaData*) * 
                                       mExtCntInGroup,
                                       (void**)&mMetaData ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sState = 2;

    /* TC/FIT/Limit/sm/sds/sdsMeta_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdsMeta::initialize::malloc2", 
                          ERR_INSUFFICIENT_MEMORY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       (ULong)ID_SIZEOF(sdsMetaData) *
                                       mFrameCntInExt *
                                       mExtCntInGroup,
                                       (void**)&mMetaData[0] ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sState = 3;

    for( i = 1; i < mExtCntInGroup ; i++ )
    {
        mMetaData[i] = mMetaData[i-1] + mFrameCntInExt ;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            IDE_ASSERT( iduMemMgr::free( mMetaData[0] ) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( iduMemMgr::free( mMetaData ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( mBasePtr ) == IDE_SUCCESS );
        default:
            break;   
    }
 
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC sdsMeta::destroyStatic()
{
    IDE_ASSERT( iduMemMgr::free( mMetaData[0] ) == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::free( mMetaData ) == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::free( mBasePtr ) == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : smiStartupShutdown 에서 호출됨 
 *  shutdown 시 메타 데이타를 업데이트 함  
 ***********************************************************************/
IDE_RC sdsMeta::finalize( idvSQL * aStatistics )
{
    sdsBCB    * sSBCB;
    UInt        sBCBIndex;
    UInt        sMetaTableIndex;
    UInt        sLastMetaTableNum;
    UInt        i;
    UInt        j;
    
    /* Group을 0~MAX 까지 스캔하여 MetaTable 을 새로 생성함 */ 
    for( sMetaTableIndex = 0 ; sMetaTableIndex < mMaxMetaTableCnt ; sMetaTableIndex++ ) 
    {
        sBCBIndex = sMetaTableIndex * ( mFrameCntInExt * mExtCntInGroup );

        for( i = 0 ; i < mExtCntInGroup ; i++ )
        {
            for( j = 0 ; j < mFrameCntInExt ; j++ )
            {                 
                sSBCB = mSBufferArea->getBCB( sBCBIndex++ );

                mMetaData[i][j].mSpaceID = sSBCB->mSpaceID; 
                mMetaData[i][j].mPageID  = sSBCB->mPageID;
                mMetaData[i][j].mState   = sSBCB->mState;
                mMetaData[i][j].mPageLSN = sSBCB->mPageLSN;
            }
        }

        /* mBase에 복사 하기 */
        (void)copyToMetaTable( sMetaTableIndex );
        /* mBase를 stable에 내려쓰기 */
        IDE_TEST( writeMetaTable( aStatistics, sMetaTableIndex )
                  != IDE_SUCCESS );
    }

    sLastMetaTableNum = mSBufferArea->getLastFlushExtent() / mExtCntInGroup;

    /* File Header 에 Seq 저장 */ 
    IDE_TEST( mSBufferFile->setAndWriteFileHdr( 
                aStatistics,
                NULL,               /* mRedoLSN       */
                NULL,               /* mCreateLSN     */
                NULL,               /* mFrameCntInExt */
                &sLastMetaTableNum )/* mLstGroupNum   */
            != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
   | SEQ | LSN | MetaData(0), ,,,,, MetaData(N) | LSN |
 * aMetaTableSeqNum   - [IN] Group의 번호 
 ***********************************************************************/
IDE_RC sdsMeta::writeMetaTable( idvSQL  * aStatistics, 
                                UInt      aMetaTableSeqNum )
{
    IDE_TEST( mSBufferFile->open( aStatistics ) != IDE_SUCCESS );
   
    IDE_TEST( mSBufferFile->write( aStatistics,
                            0,                                 /* ID */
                            getTableOffset( aMetaTableSeqNum ),
                            SDS_META_TABLE_PAGE_COUNT,         /* page count */
                            mBase ) 
              != IDE_SUCCESS );       

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
   | SEQ | LSN | MetaData(0), ,,,,, MetaData(N) | LSN |
 ***********************************************************************/
IDE_RC sdsMeta::copyToMetaTable( UInt aMetaTableSeqNum )
{
    smLSN    sChkLSN;
    UInt     sMetaDataLen;

    idlOS::memset( mBase , 0x00, SDS_META_TABLE_SIZE );

    SM_GET_LSN( sChkLSN, mMetaData[0][0].mPageLSN );
    /* 1. Seq */   
    idlOS::memcpy( mBase , &aMetaTableSeqNum, ID_SIZEOF(UInt) );
    /* 2. LSN */
    idlOS::memcpy( mBase + ID_SIZEOF(UInt), 
                   &sChkLSN, 
                   ID_SIZEOF(smLSN) );
    /* 3. MetaData */
    sMetaDataLen = ID_SIZEOF(sdsMetaData) * mExtCntInGroup * mFrameCntInExt ;
    
    idlOS::memcpy( mBase + ID_SIZEOF(UInt) + ID_SIZEOF(smLSN), 
                   mMetaData[0], 
                   sMetaDataLen );
    /* 4. LSN */
    idlOS::memcpy( mBase + ID_SIZEOF(UInt) + ID_SIZEOF(smLSN) + sMetaDataLen, 
                   &sChkLSN, 
                   ID_SIZEOF(smLSN) );

    return IDE_SUCCESS;
} 

/***********************************************************************
 * Description :
 * aMetaTableSeqNum  - [IN] 읽어야하는 MetaTable Num 
 * aMeta             - [OUT] MetaData를 복사할 버퍼
 * aIsValidate       - [OUT] MetaTable을 통해 SBCB를 복구해도 되는지  
 ***********************************************************************/
IDE_RC sdsMeta::readAndCheckUpMetaTable( idvSQL         * aStatistics, 
                                         UInt             aMetaTableSeqNum,
                                         sdsMetaData    * aMeta,
                                         idBool         * aIsValidate ) 
{
    UInt      sReadMetaTableSeq;
    smLSN     sFstLSN;    
    smLSN     sLstLSN;    
    UInt      sMetaLen;

    IDE_TEST( mSBufferFile->open( aStatistics ) != IDE_SUCCESS );

    IDE_TEST( mSBufferFile->read( aStatistics,
                                  0,                              /* aFrameID */
                                  getTableOffset( aMetaTableSeqNum ),
                                  SDS_META_TABLE_PAGE_COUNT,      /* page count */
                                  (UChar*)mBase ) 
              != IDE_SUCCESS );       

    /* 1. Seq */
    idlOS::memcpy( &sReadMetaTableSeq, 
                   (UChar*)mBase, 
                   ID_SIZEOF(UInt) );
    /* 2. LSN */
    idlOS::memcpy( &sFstLSN, 
                   (SChar*)(mBase + ID_SIZEOF(UInt)), 
                   ID_SIZEOF(smLSN) );
    /* 3. MetaData */
    sMetaLen = ID_SIZEOF(sdsMetaData) * mExtCntInGroup * mFrameCntInExt ;
    idlOS::memcpy( (SChar*)aMeta, 
                   (SChar*)(mBase + ID_SIZEOF(UInt) + ID_SIZEOF(smLSN)), 
                   sMetaLen );
    /* 4. LSN */
    idlOS::memcpy( &sLstLSN, 
                   (SChar*)(mBase + ID_SIZEOF(UInt) + ID_SIZEOF(smLSN) + sMetaLen),
                   ID_SIZEOF(smLSN) );

    /* checksum이 다르면 skip */
    IDE_TEST_CONT( smLayerCallback::isLSNEQ( &sFstLSN, &sLstLSN ) 
                    == ID_TRUE, 
                    CONT_SKIP );
    
    /* ChecksumLSN과 0번 MetaData의 LSN이 다르면 skip */
    IDE_TEST_CONT( smLayerCallback::isLSNEQ( &sFstLSN, &aMeta[0].mPageLSN ) 
                    == ID_TRUE,
                    CONT_SKIP );

    /* 읽어야 하는 MetaTable 이랑 읽은 MetaTable의 번호가 다르면 skip */
    IDE_TEST_CONT( sReadMetaTableSeq != aMetaTableSeqNum, 
                    CONT_SKIP );

    *aIsValidate = ID_TRUE;

    IDE_EXCEPTION_CONT( CONT_SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * aLstMetaTableSeqNum - [OUT] 마지막에 저장된 MetaTable Seq 번호
 * aRecoveryLSN        - [OUT] RecoveryLSN
 ***********************************************************************/
IDE_RC sdsMeta::readMetaInfo( idvSQL * aStatistics, 
                              UInt   * aLstMetaTableSeqNum,
                              smLSN  * aRecoveryLSN ) 
{
    IDE_TEST( mSBufferFile->open( aStatistics ) != IDE_SUCCESS );

    IDE_TEST( mSBufferFile->readHdrInfo( aStatistics, 
                                         aLstMetaTableSeqNum,
                                         aRecoveryLSN )
              != IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Meta Table을 읽어서 BCB 정보를 재구축한다.
************************************************************************/
IDE_RC sdsMeta::copyToSBCB( idvSQL      * aStatistics,
                            sdsMetaData * aMetaData,
                            UInt          aMetaTableNum,
                            smLSN       * aRecoveryLSN )
{
    SInt        i;
    UInt        sBCBIndex;
    sdsBCB    * sSBCB     = NULL;
    sdsBCB    * sExistBCB = NULL;
    sdbHashChainsLatchHandle  * sHashChainsHandle = NULL;

    /* 해당 MetaTable의 마지막 MetaData부터 읽어 와야 하므로
       그다음 Extent의 첫 MetaData 번호 가져와서 하나 빼기 */        
    sBCBIndex = ( aMetaTableNum+1 ) * mFrameCntInExt * mExtCntInGroup;
    sBCBIndex--;

    i = mFrameCntInExt-1;

    for( ; i >= 0 ; i-- )
    {                   
        sSBCB = mSBufferArea->getBCB( sBCBIndex-- );

        if( ( aMetaData[i].mState == SDS_SBCB_FREE )  ||
            ( aMetaData[i].mState == SDS_SBCB_INIOB ) ||
            ( aMetaData[i].mState == SDS_SBCB_OLD ) )
        {
            continue;
        } 
        else 
        {
            IDE_ASSERT( ( aMetaData[i].mState == SDS_SBCB_CLEAN ) ||
                        ( aMetaData[i].mState == SDS_SBCB_DIRTY ) );
        }

        if ( smLayerCallback::isLSNGT( aRecoveryLSN, 
                                       &aMetaData[i].mPageLSN ) 
             == ID_TRUE) 
        {
            continue; 
        }

        sSBCB->mSpaceID  = aMetaData[i].mSpaceID; 
        sSBCB->mPageID   = aMetaData[i].mPageID;
        sSBCB->mState    = aMetaData[i].mState;
        SM_GET_LSN( sSBCB->mPageLSN, aMetaData[i].mPageLSN );

        /* 설정한 BCB를 hash에 삽입.. */
        sHashChainsHandle = mHashTable->lockHashChainsXLatch( aStatistics,
                                                              sSBCB->mSpaceID,
                                                              sSBCB->mPageID );

        /* Meta Table을 읽어서 Hash를 재구성 할때는 backward로 스캔한다 */
        /* 이미 삽입된 데이타가 존재 한다면 그게 더 최신 데이타 인것임. */
        mHashTable->insertBCB( sSBCB, (void**)&sExistBCB );

        if( sExistBCB != NULL )
        {
            /* 새로 읽은 페이지의 LSN이 존재하는 LSN보다 크면 삽입 */
            if ( smLayerCallback::isLSNGTE( &sSBCB->mPageLSN, 
                                            &sExistBCB->mPageLSN ) 
                 == ID_TRUE) 
            {
                mHashTable->removeBCB ( sExistBCB );
                (void)sExistBCB->setFree();

                mHashTable->insertBCB( sSBCB, (void**)&sExistBCB );
            }
            else 
            {
                (void)sSBCB->setFree();
            }     
        }

        mHashTable->unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 * aLstMetaTableSeqNum - [IN] MetaTable 검색을 통해 BCB를 구축할 첫 Table 번호 
 * aRecoveryLSN        - [IN] RecoveryLSN 보다 작은 pageLSN을 가지는 BCB를 복구할 필요 없음.
 ***********************************************************************/
IDE_RC sdsMeta::buildBCBByMetaTable( idvSQL * aStatistics, 
                                     UInt     aLstMetaTableSeqNum,
                                     smLSN  * aRecoveryLSN )
{
    UInt        i;
    UInt        sTargetMetaTableSeq;
    sdsMetaData sMetaData[SDS_META_DATA_MAX_COUNT_IN_GROUP]; 
    idBool      sIsValidate = ID_FALSE;

    sTargetMetaTableSeq = aLstMetaTableSeqNum;

    for( i = 0 ; i < mMaxMetaTableCnt ; i++ )
    { 
        IDE_TEST( readAndCheckUpMetaTable( aStatistics, 
                                           sTargetMetaTableSeq, 
                                           sMetaData,
                                           &sIsValidate ) 
                  != IDE_SUCCESS );
    
        if( sIsValidate == ID_FALSE )
        {
            /* 정상적인 MetaTable이 아니므로 다음 MetaTable을 검사 한다*/
            continue;
        }
        else
        {
            /* MetaTable을 통한 SBCB 복구를 시작한다. */
            /* nothing to do */
        }
        
        IDE_TEST( copyToSBCB( aStatistics, 
                              sMetaData, 
                              sTargetMetaTableSeq, 
                              aRecoveryLSN ) 
                  != IDE_SUCCESS ); 
        
        if( sTargetMetaTableSeq > 0 )
        {
            sTargetMetaTableSeq--;
        }
        else 
        {
            sTargetMetaTableSeq = mMaxMetaTableCnt-1;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
} 

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC sdsMeta::buildBCB( idvSQL * aStatistics, idBool aIsRecoveryMode )
{
    UInt  sLstMetaTableSeqNum;
    smLSN sRecoveryLSN;

    if( aIsRecoveryMode != ID_TRUE ) /* 정상 종료 */
    { 
        IDE_TEST( readMetaInfo( aStatistics, 
                                &sLstMetaTableSeqNum,
                                &sRecoveryLSN )
                  != IDE_SUCCESS );

        if( sLstMetaTableSeqNum != SDS_FILEHDR_SEQ_MAX )
        {
            IDE_TEST( sLstMetaTableSeqNum > mMaxMetaTableCnt ); 

            IDE_TEST( buildBCBByMetaTable( aStatistics,
                                           sLstMetaTableSeqNum,
                                           &sRecoveryLSN )   
                      != IDE_SUCCESS);
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
         /* 비정상 종료시에는 BCB 재구축을 하지 않음. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : dump 함수. Meta table을 출력한다.
 * aPage     [IN]  - 출력할 페이지 정보 
 * aOutBuf   [OUT] - 페이지 내용을 저장할 버퍼
 * aOutSize  [IN]  - 버퍼의 limit 크기 
 ***********************************************************************/
IDE_RC sdsMeta::dumpMetaTable( const UChar    * aPage,
                               SChar          * aOutBuf, 
                               UInt          /* aOutSize */)
{
    UInt    i;
    sdsMetaData * sMetaData;

    IDE_ASSERT( aPage != NULL );
    IDE_ASSERT( aOutBuf != NULL );
  
    sMetaData = (sdsMetaData*)aPage;
    
    for( i = 0 ; i < SDS_META_DATA_MAX_COUNT_IN_GROUP ; i++ )
    {
        idlOS::sprintf( aOutBuf,
                        "%"ID_UINT32_FMT".\n",
                        i );

        idlOS::sprintf( aOutBuf,
                "Space ID        : %"ID_UINT32_FMT"\n"
                "Page ID         : %"ID_UINT32_FMT"\n",
                sMetaData[i].mSpaceID, 
                sMetaData[i].mPageID );

        switch( sMetaData[i].mState )
        {
            case SDS_SBCB_FREE:
                idlOS::sprintf( aOutBuf,
                                "Page State      : SDS_SBCB_FREE \n" );
                break;

            case  SDS_SBCB_CLEAN:
                idlOS::sprintf( aOutBuf,
                                "Page State      : SDS_SBCB_CLEAN \n" );
                break;

            case SDS_SBCB_DIRTY:
                idlOS::sprintf( aOutBuf,
                                "Page State      : SDS_SBCB_DIRTY \n" );
                break;

            case SDS_SBCB_INIOB:
                idlOS::sprintf( aOutBuf,
                                "Page State      : SDS_SBCB_INOIB \n" );
                break;

            case SDS_SBCB_OLD: 
                idlOS::sprintf( aOutBuf,
                                "Page State      : SDS_SBCB_REDIRTY \n" );
                break;

            default:
                idlOS::sprintf( aOutBuf, 
                                "Unknown State \n" );
                break;
        }

        idlOS::sprintf( aOutBuf,
                "PageLSN.FileNo  : %"ID_UINT32_FMT"\n"
                "PageLSN.Offset  : %"ID_UINT32_FMT"\n",
                sMetaData[i].mPageLSN.mFileNo,
                sMetaData[i].mPageLSN.mOffset );

        idlOS::sprintf( aOutBuf, " \n" );
    }  

    return IDE_SUCCESS;
}
