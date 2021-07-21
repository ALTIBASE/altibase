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
 **********************************************************************/

#include <smErrorCode.h>
#include <sdr.h>
#include <smr.h>
#include <smx.h>
#include <sdcReq.h>
#include <sdcRow.h>
#include <sdcLob.h>


const sdcLobDesc sdcLob::mEmptyLobDesc =
{
    0,                        // mLobVersion
    SDC_LOB_DESC_NULL_FALSE,  // mLobDescFlag
    0,                        // mLastPageSize
    0,                        // mLastPageSeq
    0,                        // mDirectCnt
    SD_NULL_PID,              // mRootNodePID
    {
        SD_NULL_PID,          // mDirect[0]
        SD_NULL_PID,          // mDirect[1]
        SD_NULL_PID,          // mDirect[2]
        SD_NULL_PID,          // mDirect[3]
    }
};

typedef struct sdcLobStatistics
{
    ULong mOpen;
    ULong mRead;
    ULong mWrite;
    ULong mErase;
    ULong mTrim;
    ULong mPrepareForWrite;
    ULong mFinishWrite;
    ULong mGetLobInfo;
    ULong mClose;
} sdcLobStatistics;

static sdcLobStatistics gDiskLobStatistics;

smLobModule sdcLobModule =
{
    sdcLob::open,
    sdcLob::read,
    sdcLob::write,
    sdcLob::erase,
    sdcLob::trim,
    sdcLob::prepare4Write,
    sdcLob::finishWrite,
    sdcLob::getLobInfo,
    sdcLob::writeLog4CursorOpen,
    sdcLob::close
};

IDE_RC sdcLob::open()
{
    gDiskLobStatistics.mOpen++;

    return IDE_SUCCESS;
}

IDE_RC sdcLob::close( idvSQL*        aStatistics,
                      void         * aTrans,
                      smLobViewEnv * aLobViewEnv )
{
    ACP_UNUSED( aStatistics );
    ACP_UNUSED( aTrans );
    ACP_UNUSED( aLobViewEnv );

    gDiskLobStatistics.mClose++;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : 자신의 view에 맞는 LOB Value를 읽는 함수.
 *
 *   aStatistics - [IN]  통계정보
 *   aTrans      - [IN]  Transaction
 *   aLobViewEnv - [IN]  자신이 봐야 할 LOB에 대한 정보
 *   aOffset     - [IN]  부분 read시 읽을 Offset
 *   aAmount     - [IN]  read요청 크기
 *   aPiece      - [OUT] 읽은 LOB Data를 담을 버퍼
 *   aReadLength - [OUT] 읽은 LOB Data의 크기
 ***********************************************************************/
IDE_RC sdcLob::read( idvSQL       * aStatistics,
                     void         * /*aTrans*/,
                     smLobViewEnv * aLobViewEnv,
                     UInt           aOffset,
                     UInt           aAmount,
                     UChar        * aPiece,
                     UInt         * aReadLength )
{
    ULong   sLobLength = 0;
    UInt    sRealAmount;

    IDE_ERROR( aLobViewEnv != NULL );
    IDE_ERROR( aPiece      != NULL );
    IDE_ERROR( aReadLength != NULL );

    /* check size */
    IDE_TEST( getLobLength((sdcLobColBuffer*)aLobViewEnv->mLobColBuf,
                           &sLobLength)
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aOffset >= sLobLength, error_max_lob_size );

    /* adjust read amount size */
    if( (aOffset + aAmount) > sLobLength )
    {
        sRealAmount = sLobLength - aOffset;
    }
    else
    {
        sRealAmount = aAmount;
    }

    IDE_TEST( readInternal(aStatistics,
                           aLobViewEnv,
                           aOffset,
                           sRealAmount,
                           aPiece,
                           aReadLength)
              != IDE_SUCCESS );
    
    gDiskLobStatistics.mRead++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_max_lob_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_MaxLobErrorSize ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  lob piece에서 value를 write하는 함수.
 *                Write할 크기가 0인 경우 MM에서 호출되지 않는다.
 * Implementation:
 *
 *    aStatistics - [IN] 통계정보
 *    aTrans      - [IN] Transaction
 *    aLobViewEnv - [IN] 자신이 봐야할 LOB에대한 정보
 *    aLobLocator - [IN] Lob Locator
 *    aOffset     - [IN] 부분 write시 write할 offset
 *    aAmount     - [IN] wrtie할 data 크기
 *    aPieceVal   - [IN] wrtie할 data가 있는 buffer
 *    aFromAPI    - [IN] ODBC등의 API에 의한 호출과 쿼리에 의한 호출을 구분
 *    aContType   - [IN] drdb의 로그가 여러 로그에 걸쳐 저장이 되었는지
 *                       단일로그인지를 판단 할 수 있는 type
 **********************************************************************/
IDE_RC sdcLob::write( idvSQL       * aStatistics,
                      void         * aTrans,
                      smLobViewEnv * aLobViewEnv,
                      smLobLocator   aLobLocator,
                      UInt           aOffset,
                      UInt           aPieceLen,
                      UChar        * aPieceVal,
                      idBool         aIsFromAPI,
                      UInt           aContType )
{
    UInt              sNeedPageCnt;
    ULong             sLobLength = 0;
    sdcLobColBuffer * sLobColBuf;

    sLobColBuf = (sdcLobColBuffer*)aLobViewEnv->mLobColBuf;

    IDE_TEST( getLobLength(sLobColBuf,
                           &sLobLength)
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aOffset > sLobLength, error_max_lob_size );

    if( aPieceLen > 0 )
    {
        sNeedPageCnt = getNeedPageCount(sLobColBuf,
                                        &aLobViewEnv->mLobCol,
                                        aOffset,
                                        aPieceLen);
    
        IDE_TEST( agingPages(aStatistics,
                             aTrans,
                             &aLobViewEnv->mLobCol,
                             sNeedPageCnt)
                  != IDE_SUCCESS );

        IDE_TEST( writeInternal(aStatistics,
                                aTrans,
                                aLobLocator,
                                aLobViewEnv,
                                aPieceLen,
                                aPieceVal,
                                SDC_LOB_WTYPE_WRITE,
                                aIsFromAPI,
                                (smrContType)aContType)
                  != IDE_SUCCESS );
    }

    incLastPageInfo( (sdcLobColBuffer*)aLobViewEnv->mLobColBuf,
                     aOffset,
                     aPieceLen );

    aLobViewEnv->mWriteOffset += aPieceLen;
    aLobViewEnv->mIsWritten = ID_TRUE;

    /* Becase the value is written, the LOB column value is not null. */
    sLobColBuf->mIsNullLob = ID_FALSE;

    gDiskLobStatistics.mWrite++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_max_lob_size )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_MaxLobErrorSize) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : Write 시작 offset을 설정한다.
 *
 *    aStatistics     - [IN] 통계 정보
 *    aTrans          - [IN] Transaction
 *    aLobViewEnv     - [IN] 자신이 봐야 할 LOB에대한 정보
 *    aLobLocator     - [IN] Lob Locator
 *    aWriteOffset    - [IN] Write할 Offset
 *    aWriteSize      - [IN] Write할 Size(미사용)
 **********************************************************************/
IDE_RC sdcLob::prepare4Write( idvSQL       * aStatistics,
                              void         * aTrans,
                              smLobViewEnv * aLobViewEnv,
                              smLobLocator   aLobLocator,
                              UInt           aWriteOffset,
                              UInt           aWriteSize )
{
    ULong sLobLength = 0;

    IDE_ERROR( aLobViewEnv != NULL );
    
    /* check size */
    IDE_TEST( getLobLength((sdcLobColBuffer*)aLobViewEnv->mLobColBuf,
                           &sLobLength)
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aWriteOffset > sLobLength, error_max_lob_size );

    /* set offset */
    aLobViewEnv->mWriteOffset = aWriteOffset;

    aLobViewEnv->mIsWritten = ID_FALSE;

    /* write prepare log for RP */
    if ( smLayerCallback::needReplicate( (smcTableHeader*)aLobViewEnv->mTable,
                                         aTrans )
         == ID_TRUE )
    {
        IDE_TEST( smrLogMgr::writeLobPrepare4WriteLogRec(
                      aStatistics,
                      aTrans,
                      aLobLocator,
                      aWriteOffset,
                      aWriteSize,
                      aWriteSize,
                      ((smcTableHeader*)aLobViewEnv->mTable)->mSelfOID )
                  != IDE_SUCCESS );
    }

    gDiskLobStatistics.mPrepareForWrite++;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( error_max_lob_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_MaxLobErrorSize ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Write가 종료되었다. Lob Cursor
 *               Buffer의 내용을 Row에 Update하고,
 *               Replication Log를 남긴다.
 *
 *    aStatistics - [IN] 통계 정보
 *    aTrans      - [IN] Transaction
 *    aLobViewEnv - [IN] 자신이 봐야 할 LOB에대한 정보
 *    aLobLocator - [IN] Lob Locator
 **********************************************************************/
IDE_RC sdcLob::finishWrite( idvSQL          * aStatistics,
                            void            * aTrans,
                            smLobViewEnv    * aLobViewEnv,
                            smLobLocator      aLobLocator )
{
    /*
     * update Lob Column
     */

    if( aLobViewEnv->mIsWritten == ID_TRUE )
    {
        IDE_TEST( updateLobCol(aStatistics,
                               aTrans,
                               aLobViewEnv)
                  != IDE_SUCCESS);
    }
    
    /*
     * write RP Log
     */
    
    IDE_ASSERT( aLobViewEnv->mOpenMode == SMI_LOB_READ_WRITE_MODE );

    if ( smLayerCallback::needReplicate( (smcTableHeader*)aLobViewEnv->mTable,
                                         aTrans ) == ID_TRUE )
    {
        IDE_TEST( smrLogMgr::writeLobFinish2WriteLogRec(
                      aStatistics,
                      aTrans,
                      aLobLocator,
                      ( (smcTableHeader*)aLobViewEnv->mTable )->mSelfOID )
                  != IDE_SUCCESS);
    }

    aLobViewEnv->mIsWritten = ID_FALSE;

    gDiskLobStatistics.mFinishWrite++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : lobCursor가 가르키고 있는 LOB의 길이를 return한다.
 * Implementation:
 *
 *   aStatistics - [IN]  통계정보
 *   aTrans      - [IN]  Transaction
 *   aLobViewEnv - [IN]  자신이 봐야할 LOB에대한 정보
 *   aLobLen     - [OUT] LOB의 길이
 *   aLobMode    - [OUT] LOB의 저장 모드 (In/Out)
 *   aIsNullLob  - [OUT] LOB의 Null 여부
 **********************************************************************/
IDE_RC sdcLob::getLobInfo( idvSQL        * aStatistics,
                           void          * aTrans,
                           smLobViewEnv  * aLobViewEnv,
                           UInt          * aLobLen,
                           UInt          * aLobMode,
                           idBool        * aIsNullLob )
{
    ULong             sLobLength;
    sdcLobColBuffer * sLobColBuf;

    IDE_ASSERT( aLobViewEnv             != NULL );
    IDE_ASSERT( aLobViewEnv->mLobColBuf != NULL );

    sLobColBuf = (sdcLobColBuffer*)aLobViewEnv->mLobColBuf;

    *aLobLen = 0;

    //fix BUG-19687
    if( sLobColBuf->mBuffer == NULL )
    {
        IDE_TEST( readLobColBuf(aStatistics,
                                aTrans,
                                aLobViewEnv)
                  != IDE_SUCCESS );
    }
    
    if( sLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB )
    {
        sLobLength = getLobLengthFromLobDesc( (sdcLobDesc*)(sLobColBuf->mBuffer) );
    }
    else
    {
        // BUG-27572 CodeSonar::Uninitialized Variable (6)
        IDE_ERROR( sLobColBuf->mInOutMode == SDC_COLUMN_IN_MODE );

        sLobLength = sLobColBuf->mLength;
    }

    *aIsNullLob = sLobColBuf->mIsNullLob;
    *aLobLen = sLobLength;

    if( aLobMode != NULL )
    {
        *aLobMode = ( sLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB ) ?
            SMI_COLUMN_MODE_OUT : SMI_COLUMN_MODE_IN ;
    }

    gDiskLobStatistics.mGetLobInfo++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : replication 관련 로그 함수.
 * Implementation:
 *
 *   aStatistics - [IN] 통계정보
 *   aTrans      - [IN] Transaction
 *   aLobLocator - [IN] LOB Locator
 *   aLobViewEnv - [IN] 자신이 봐야할 LOB에대한 정보
 **********************************************************************/
IDE_RC sdcLob::writeLog4CursorOpen( idvSQL       * aStatistics,
                                    void         * aTrans,
                                    smLobLocator   aLobLocator,
                                    smLobViewEnv * aLobViewEnv )
{
    sdrMtx       sMtx;
    scSpaceID    sTableSpaceID;
    sdSID        sRowSID;
    UChar       *sSlot;
    sdcPKInfo    sPKInfo;
    UInt         sState = 0;
    idBool       sDummy;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE, /* MtxUndoable ( PROJ-2162 ) */
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    sTableSpaceID = SC_MAKE_SPACE(aLobViewEnv->mGRID);
    sRowSID       = SD_MAKE_SID( SC_MAKE_PID(aLobViewEnv->mGRID),
                                 SC_MAKE_SLOTNUM(aLobViewEnv->mGRID) );

    IDE_TEST( sdbBufferMgr::getPageBySID( aStatistics,
                                          sTableSpaceID,
                                          sRowSID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          &sMtx,
                                          (UChar**)&sSlot,
                                          &sDummy )
              != IDE_SUCCESS );

    IDE_TEST( sdcRow::getPKInfo( aStatistics,
                                 aTrans,
                                 sTableSpaceID,
                                 aLobViewEnv->mTable,
                                 sSlot,
                                 sRowSID,
                                 &sPKInfo )
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeLobCursorOpenLogRec(
                              aStatistics,
                              aTrans,
                              SMR_DISK_LOB_CURSOR_OPEN,
                              smLayerCallback::getTableOID( aLobViewEnv->mTable ),
                              aLobViewEnv->mLobCol.id,
                              aLobLocator,
                              &sPKInfo )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST ( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aOffset으로 부터 aAmount 크기 만큼을 읽는다.
 *               aAmount보다 적게 읽을 수 있다.
 **********************************************************************/
IDE_RC sdcLob::readInternal( idvSQL       * aStatistics,
                             smLobViewEnv * aLobViewEnv,
                             UInt           aOffset,
                             UInt           aAmount,
                             UChar        * aPiece,
                             UInt         * aReadLength )
{
    sdcLobColBuffer * sLobColBuf;
    sdcLobDesc      * sLobDesc;
    UInt              sOffset;
    UInt              sAmount;
    UChar           * sPiece;
    UInt              sReadLength;

    IDE_ERROR( aLobViewEnv != NULL );
    IDE_ERROR( aLobViewEnv->mLobColBuf != NULL );
    IDE_ERROR( aPiece != NULL );
    IDE_ERROR( aReadLength != NULL );

    sOffset     = aOffset;
    sAmount     = aAmount;
    sPiece      = aPiece;
    sReadLength = 0;

    sLobColBuf = (sdcLobColBuffer*)aLobViewEnv->mLobColBuf;

    if( sLobColBuf->mInOutMode == SDC_COLUMN_IN_MODE )
    {
        IDE_TEST( readBuffer(aLobViewEnv,
                             sLobColBuf,
                             &sOffset,
                             aAmount,
                             aPiece,
                             &sReadLength)
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_ERROR( sLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB );
        IDE_ERROR( sLobColBuf->mBuffer != NULL );
        IDE_ERROR( sLobColBuf->mLength == ID_SIZEOF(sdcLobDesc) );

        sLobDesc = (sdcLobDesc*)sLobColBuf->mBuffer;

        IDE_TEST( readDirect(aStatistics,
                             aLobViewEnv,
                             sLobDesc,
                             &sOffset,
                             &sAmount,
                             &sPiece,
                             &sReadLength)
                  != IDE_SUCCESS );

        IDE_TEST( readIndex(aStatistics,
                            aLobViewEnv,
                            sLobDesc,
                            &sOffset,
                            &sAmount,
                            &sPiece,
                            &sReadLength)
                  != IDE_SUCCESS );
    }

    *aReadLength = sReadLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aOffset으로 부터 aAmount 크기 만큼을 읽는다.
 *               Lob Column Buffer로 부터 읽는다.(In Mode LOB)
 **********************************************************************/
IDE_RC sdcLob::readBuffer( smLobViewEnv     * aLobViewEnv,
                           sdcLobColBuffer  * aLobColBuf,
                           UInt             * aOffset,
                           UInt               aAmount,
                           UChar            * aPiece,
                           UInt             * aReadLength )
{
    UInt    sLen;

    IDE_ERROR( aLobColBuf != NULL );
    IDE_ERROR( aPiece != NULL );
    IDE_ERROR( aReadLength != NULL );

    if( ((*aOffset) + aAmount) > aLobColBuf->mLength )
    {
        sLen = aLobColBuf->mLength - (*aOffset);
    }
    else
    {
        sLen = aAmount;
    }

    idlOS::memcpy( aPiece, aLobColBuf->mBuffer + (*aOffset), sLen );

    *aOffset += sLen;
    *aReadLength = sLen;

    aLobViewEnv->mLastReadOffset = (*aOffset);
    aLobViewEnv->mLastReadLeafNodePID = SD_NULL_PID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aOffset으로 부터 aAmount 크기 만큼을 읽는다.
 *               Direct Page로 부터 읽는다.(Out Mode LOB)
 **********************************************************************/
IDE_RC sdcLob::readDirect( idvSQL        * aStatistics,
                           smLobViewEnv  * aLobViewEnv,
                           sdcLobDesc    * aLobDesc,
                           UInt          * aOffset,
                           UInt          * aAmount,
                           UChar        ** aPiece,
                           UInt          * aReadLength )
{
    scPageID    sPID;
    UInt        sPageSeq;
    UInt        i;

    IDE_ERROR( aLobViewEnv != NULL );
    IDE_ERROR( aLobDesc    != NULL );
    IDE_ERROR( aOffset     != NULL );
    IDE_ERROR( aAmount     != NULL );
    IDE_ERROR( aPiece      != NULL );
    IDE_ERROR( aReadLength != NULL );

    sPageSeq = getPageSeq( *aOffset );

    IDE_TEST_CONT( sPageSeq >= SDC_LOB_MAX_DIRECT_PAGE_CNT,
                    SKIP_READ_DIRECT );

    IDE_ERROR( aLobDesc->mDirectCnt <= SDC_LOB_MAX_DIRECT_PAGE_CNT );
    
    for( i = sPageSeq; i < aLobDesc->mDirectCnt; i++ )
    {
        if( *aAmount == 0 )
        {
            break;
        }
        
        sPID = aLobDesc->mDirect[i];

        IDE_TEST( readPage(aStatistics,
                           &aLobViewEnv->mLobCol,
                           sPID,
                           aOffset,
                           aAmount,
                           aPiece,
                           aReadLength)
                  != IDE_SUCCESS );
    }

    aLobViewEnv->mLastReadOffset = (*aOffset);
    aLobViewEnv->mLastReadLeafNodePID = SD_NULL_PID;
    
    IDE_EXCEPTION_CONT( SKIP_READ_DIRECT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aOffset으로 부터 aAmount 크기 만큼을 읽는다.
 *               Lob Index로 부터 읽는다.(Out Mode LOB)
 **********************************************************************/
IDE_RC sdcLob::readIndex( idvSQL        * aStatistics,
                          smLobViewEnv  * aLobViewEnv,
                          sdcLobDesc    * aLobDesc,
                          UInt          * aOffset,
                          UInt          * aAmount,
                          UChar        ** aPiece,
                          UInt          * aReadLength )
{
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobLKey      * sLKeyArray;
    sdcLobLKey      * sLKey;
    sdcLobLKey        sValidLKey;
    scPageID          sPID;
    scPageID          sLeafNodePID;
    SShort            sLeafKeySeq;
    SShort            sLeafKeyEntrySeq;
    UShort            sLeafKeyCount;
    UChar           * sLeafPage;
    UInt              sPageSeq;
    idBool            sIsFound;
    UInt              i;
    UInt              j;
    UInt              sState = 0;

    IDE_ERROR( aLobViewEnv != NULL );
    IDE_ERROR( aLobDesc    != NULL );
    IDE_ERROR( aOffset     != NULL );
    IDE_ERROR( aAmount     != NULL );
    IDE_ERROR( aPiece      != NULL );
    IDE_ERROR( aReadLength != NULL );

    while( (*aAmount) > 0 )
    {
        if( *aAmount == 0 )
        {
            break;
        }
        
        sPageSeq = getPageSeq( *aOffset );

        if( (aLobViewEnv->mLastReadOffset  == (*aOffset)) &&
            (aLobViewEnv->mLastReadLeafNodePID != SD_NULL_PID) )
        {
            // sequential search
            IDE_TEST( traverseSequential(aStatistics,
                                         &aLobViewEnv->mLobCol,
                                         aLobViewEnv->mLastReadLeafNodePID,
                                         sPageSeq,
                                         &sLeafNodePID,
                                         &sLeafKeySeq,
                                         &sLeafKeyEntrySeq,
                                         &sLeafKeyCount)
                      != IDE_SUCCESS );
        }
        else
        {
            // tree search
            IDE_TEST( traverse(aStatistics,
                               &aLobViewEnv->mLobCol,
                               aLobDesc->mRootNodePID,
                               sPageSeq,
                               &sLeafNodePID,
                               &sLeafKeySeq,
                               &sLeafKeyEntrySeq,
                               &sLeafKeyCount)
                      != IDE_SUCCESS );
        }

        if( (sLeafNodePID == SD_NULL_PID) ||
            (sLeafKeySeq  >= sLeafKeyCount) )
        {
            break;
        }

        IDE_TEST( sdbBufferMgr::getPageByPID(
                      aStatistics,
                      aLobViewEnv->mLobCol.colSeg.mSpaceID,
                      sLeafNodePID,
                      SDB_S_LATCH,
                      SDB_WAIT_NORMAL,
                      SDB_SINGLE_PAGE_READ,
                      NULL, /* aMtx */
                      &sLeafPage,
                      NULL,
                      NULL)
                  != IDE_SUCCESS );
        sState = 1;

        sNodeHdr   = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sLeafPage);
        sLKeyArray = (sdcLobLKey*)getLobDataLayerStartPtr(sLeafPage);

        /* key loop */
        for( i = sLeafKeySeq; i < sNodeHdr->mKeyCnt; i++ )
        {
            if( *aAmount == 0 )
            {
                break;
            }
            
            sLKey = &sLKeyArray[i];

            IDE_TEST( getValidVersionLKey(aStatistics,
                                          aLobDesc->mLobVersion,
                                          sLKey,
                                          &sValidLKey,
                                          &sIsFound)
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sIsFound != ID_TRUE,
                            err_get_valide_version_key );

            /* entry loop */
            for( j = sLeafKeyEntrySeq; j < sValidLKey.mEntryCnt; j++ )
            {
                if( *aAmount == 0 )
                {
                    break;
                }

                sPID = sValidLKey.mEntry[j];

                IDE_TEST( readPage(aStatistics,
                                   &aLobViewEnv->mLobCol,
                                   sPID,
                                   aOffset,
                                   aAmount,
                                   aPiece,
                                   aReadLength)
                          != IDE_SUCCESS );
            }

            sLeafKeyEntrySeq = 0;
        } // for

        aLobViewEnv->mLastReadOffset  = *aOffset;
        aLobViewEnv->mLastReadLeafNodePID = sLeafNodePID;

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics, sLeafPage)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_get_valide_version_key )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, 
                                 "fail to get valid version key") );
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, sLeafPage)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 유효한 버전의 Leaf Key를 읽는다.
 **********************************************************************/
IDE_RC sdcLob::getValidVersionLKey( idvSQL      * aStatistics,
                                    ULong         aLobVersion,
                                    sdcLobLKey  * aLKey,
                                    sdcLobLKey  * aValidLKey,
                                    idBool      * aIsFound )
{
    sdcLobLKey  sOldLKey;
    sdSID       sUndoSID;
    idBool      sIsFound = ID_FALSE;

    IDE_ERROR( aLKey      != NULL );
    IDE_ERROR( aValidLKey != NULL );
    IDE_ERROR( aIsFound   != NULL );

    idlOS::memcpy( &sOldLKey, aLKey, ID_SIZEOF(sdcLobLKey) );

    while( 1 )
    {
        if( sOldLKey.mLobVersion <= aLobVersion )
        {
            sIsFound = ID_TRUE;
            break;
        }

        sUndoSID = sOldLKey.mUndoSID;

        if( sUndoSID == SD_NULL_SID )
        {
            break;
        }
        
        IDE_TEST( makeOldVersionLKey(aStatistics,
                                     sUndoSID,
                                     &sOldLKey)
                  != IDE_SUCCESS );
    }

    if( sIsFound == ID_TRUE )
    {
        idlOS::memcpy( aValidLKey, &sOldLKey, ID_SIZEOF(sdcLobLKey) );
    }

    *aIsFound = sIsFound;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Undo Record로 Leaf Key를 가져온다.
 **********************************************************************/
IDE_RC sdcLob::makeOldVersionLKey( idvSQL       * aStatistics,
                                   sdSID          aUndoSID,
                                   sdcLobLKey   * aOldLKey )
{
    sdcUndoRecType    sType;
    UChar           * sUndoRecHdr;
    UChar           * sUndoRecBody;
    idBool            sDummy;
    UInt              sState = 0;

    IDE_ERROR( aUndoSID != SD_NULL_SID );
    IDE_ERROR( aOldLKey != NULL );
    
    IDE_TEST( sdbBufferMgr::getPageBySID(
                                  aStatistics,
                                  SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                  aUndoSID,
                                  SDB_S_LATCH,
                                  SDB_WAIT_NORMAL,
                                  NULL, /* aMtx */
                                  &sUndoRecHdr,
                                  &sDummy ) 
              != IDE_SUCCESS );
    sState = 1;

    sUndoRecBody = sdcUndoRecord::getUndoRecBodyStartPtr(sUndoRecHdr);

    SDC_GET_UNDOREC_HDR_1B_FIELD( sUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &sType );

    IDE_ERROR( sType == SDC_UNDO_UPDATE_LOB_LEAF_KEY );

    idlOS::memcpy( aOldLKey, sUndoRecBody, ID_SIZEOF(sdcLobLKey) );
    
    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage(aStatistics, sUndoRecHdr)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, sUndoRecHdr)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPID 페이지로 부터 값을 읽는다.
 **********************************************************************/
IDE_RC sdcLob::readPage( idvSQL      * aStatistics,
                         smiColumn   * aColumn,
                         scPageID      aPID,
                         UInt        * aOffset,
                         UInt        * aAmount,
                         UChar      ** aPiece,
                         UInt        * aReadLength )
{
    UInt      sState = 0;
    UChar   * sPage = NULL;  
    UInt      sReadOffset;
    UInt      sReadSize;
    UChar   * sLobDataLayerStartPtr = NULL;

    IDE_ERROR( aColumn     != NULL );
    IDE_ERROR( aOffset     != NULL );
    IDE_ERROR( aAmount     != NULL );
    IDE_ERROR( aPiece      != NULL );
    IDE_ERROR( aReadLength != NULL );
    
    if( aPID == SD_NULL_PID )
    {
        IDE_TEST( getReadPageOffsetAndSize(NULL, /* aPage */
                                           *aOffset,
                                           *aAmount,
                                           &sReadOffset,
                                           &sReadSize)
                  != IDE_SUCCESS );

        switch( aColumn->colType )
        {
            case SMI_LOB_COLUMN_TYPE_BLOB:
                idlOS::memset( *aPiece, 0x00, sReadSize );
                break;
                    
            case SMI_LOB_COLUMN_TYPE_CLOB:
                idlOS::memset( *aPiece, ' ', sReadSize );
                break;
                    
            default:
                IDE_ERROR( 0 );
                break;
        }
    }
    else
    {
        IDE_TEST( sdbBufferMgr::getPageByPID(
                      aStatistics,
                      aColumn->colSeg.mSpaceID,
                      aPID,
                      SDB_S_LATCH,
                      SDB_WAIT_NORMAL,
                      SDB_SINGLE_PAGE_READ,
                      NULL, /* aMtx */
                      &sPage,
                      NULL,
                      NULL)
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( getReadPageOffsetAndSize(sPage,
                                           *aOffset,
                                           *aAmount,
                                           &sReadOffset,
                                           &sReadSize)
                  != IDE_SUCCESS );

        sLobDataLayerStartPtr = getLobDataLayerStartPtr(sPage);

        idlOS::memcpy( *aPiece,
                       sLobDataLayerStartPtr + sReadOffset,
                       sReadSize );
        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics, sPage)
                  != IDE_SUCCESS );
    }

    *aOffset += sReadSize;
    *aAmount -= sReadSize;
    *aPiece  += sReadSize;
            
    *aReadLength += sReadSize;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, sPage)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Last Page 정보를 갱신(증가)시킨다.
 **********************************************************************/
void sdcLob::incLastPageInfo( sdcLobColBuffer   * aLobColBuf,
                              UInt                aOffset,
                              UInt                aPieceLen )
{
    sdcLobDesc  * sLobDesc;
    ULong         sOldSize;
    ULong         sNewSize;

    IDE_ASSERT( aLobColBuf != NULL );

    if( aLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB )
    {
        sLobDesc = (sdcLobDesc*)aLobColBuf->mBuffer;
        
        sOldSize =
            (sLobDesc->mLastPageSeq * SDC_LOB_PAGE_BODY_SIZE) +
            sLobDesc->mLastPageSize;

        sNewSize = aOffset + aPieceLen;

        if( sNewSize > sOldSize )
        {
            sLobDesc->mLastPageSeq  = sNewSize / SDC_LOB_PAGE_BODY_SIZE;
            sLobDesc->mLastPageSize = sNewSize % SDC_LOB_PAGE_BODY_SIZE;

            /* The case of last page size 0 should be considered. */
            if( (sLobDesc->mLastPageSeq   > 0) &&
                (sLobDesc->mLastPageSize == 0) )
            {
                sLobDesc->mLastPageSeq -= 1;
                sLobDesc->mLastPageSize = SDC_LOB_PAGE_BODY_SIZE;
            }
        }
    }

    return;
}

/***********************************************************************
 * Description : Last Page 정보를 갱신(감소)시킨다.
 **********************************************************************/
void sdcLob::decLastPageInfo( sdcLobColBuffer   * aLobColBuf,
                              UInt                aOffset )
{
    sdcLobDesc  * sLobDesc;
    ULong         sOldSize;
    ULong         sNewSize;

    IDE_ASSERT( aLobColBuf != NULL );

    if( aLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB )
    {
        sLobDesc = (sdcLobDesc*)aLobColBuf->mBuffer;
        
        sOldSize =
            (sLobDesc->mLastPageSeq * SDC_LOB_PAGE_BODY_SIZE) +
            sLobDesc->mLastPageSize;

        sNewSize = aOffset;

        if( sNewSize < sOldSize )
        {
            sLobDesc->mLastPageSeq  = sNewSize / SDC_LOB_PAGE_BODY_SIZE;
            sLobDesc->mLastPageSize = sNewSize % SDC_LOB_PAGE_BODY_SIZE;

            if( (sLobDesc->mLastPageSeq > 0 ) &&
                (sLobDesc->mLastPageSize == 0) )
            {
                sLobDesc->mLastPageSeq -= 1;
                sLobDesc->mLastPageSize = SDC_LOB_PAGE_BODY_SIZE;
            }
        }
    }

    return;
}

/***********************************************************************
 * Description : LOB에 대한 erase를 수행한다.
 *               추후 필요할 것으로 예상되는 기능으로 추가되었다.
 *               사용하기 위해서는 테스트가 필요하다.
 **********************************************************************/
IDE_RC sdcLob::erase( idvSQL       * aStatistics,
                      void         * aTrans,
                      smLobViewEnv * aLobViewEnv,
                      smLobLocator   aLobLocator,
                      UInt           aOffset,
                      UInt           aPieceLen )
{
    UInt              sRealPieceLen;
    ULong             sLobLength   = 0;
    UInt              sNeedPageCnt = 2;

    /*
     * check size
     */
    
    IDE_TEST( getLobLength((sdcLobColBuffer*)aLobViewEnv->mLobColBuf,
                           &sLobLength)
              != IDE_SUCCESS );

    IDE_TEST_RAISE( 
        ( (aOffset + aPieceLen) > ID_UINT_MAX ) ||
        ( (aOffset >= sLobLength) ),
        error_max_lob_size );

    if( (aOffset + aPieceLen) > sLobLength )
    {
        sRealPieceLen = sLobLength - aOffset;
    }
    else
    {
        sRealPieceLen = aPieceLen;
    }

    /*
     * set offset
     */
    
    aLobViewEnv->mWriteOffset = aOffset;

    /* erase */
    IDE_TEST( agingPages(aStatistics,
                         aTrans,
                         &aLobViewEnv->mLobCol,
                         sNeedPageCnt)
              != IDE_SUCCESS );

    IDE_TEST( writeInternal(aStatistics,
                            aTrans,
                            aLobLocator,
                            aLobViewEnv,
                            sRealPieceLen,
                            NULL, /* aPieceVal */
                            SDC_LOB_WTYPE_ERASE,
                            ID_TRUE /* aIsFromAPI */,
                            SMR_CT_END)
              != IDE_SUCCESS );

    /*
     * update Lob Column
     */
    
    IDE_TEST( updateLobCol(aStatistics,
                           aTrans,
                           aLobViewEnv)
              != IDE_SUCCESS);

    /*
     * write erase log for RP
     */
    
    if ( smLayerCallback::needReplicate( (smcTableHeader*)aLobViewEnv->mTable,
                                         aTrans )
         == ID_TRUE )
    {
        IDE_TEST( smrLogMgr::writeLobEraseLogRec(
                      aStatistics,
                      aTrans,
                      aLobLocator,
                      aOffset,
                      aPieceLen,
                      ((smcTableHeader*)aLobViewEnv->mTable)->mSelfOID )
                  != IDE_SUCCESS );
    }

    gDiskLobStatistics.mErase++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_max_lob_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_MaxLobErrorSize ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LOB에 대한 trim을 수행한다.
 **********************************************************************/
IDE_RC sdcLob::trim( idvSQL       * aStatistics,
                     void         * aTrans,
                     smLobViewEnv * aLobViewEnv,
                     smLobLocator   aLobLocator,
                     UInt           aOffset )
{
    UInt              sNeedPageCnt = 1;
    ULong             sLobLength   = 0;
    UInt              sTrimLen;
    UChar             sDummyVal;

    /*
     * check size
     */
    
    IDE_TEST( getLobLength((sdcLobColBuffer*)aLobViewEnv->mLobColBuf,
                           &sLobLength)
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aOffset >= sLobLength, error_max_lob_size );

    sTrimLen = sLobLength - aOffset;

    /*
     * set offset
     */

    aLobViewEnv->mWriteOffset = aOffset;

    /*
     * erase
     */
    
    IDE_TEST( agingPages(aStatistics,
                         aTrans,
                         &aLobViewEnv->mLobCol,
                         sNeedPageCnt)
              != IDE_SUCCESS );

    IDE_TEST( writeInternal(aStatistics,
                            aTrans,
                            aLobLocator,
                            aLobViewEnv,
                            sTrimLen,
                            &sDummyVal,
                            SDC_LOB_WTYPE_TRIM,
                            ID_TRUE /* aIsFromAPI */,
                            SMR_CT_END)
              != IDE_SUCCESS );

    /*
     * increase last page seq & size
     */
    
    decLastPageInfo( (sdcLobColBuffer*)aLobViewEnv->mLobColBuf,
                     aOffset );

    /*
     * update Lob Column
     */
    
    IDE_TEST( updateLobCol(aStatistics,
                           aTrans,
                           aLobViewEnv)
              != IDE_SUCCESS);

    /*
     * write erase log for RP
     */
    
    if ( smLayerCallback::needReplicate( (smcTableHeader*)aLobViewEnv->mTable,
                                         aTrans )
         == ID_TRUE )
    {
        IDE_TEST( smrLogMgr::writeLobTrimLogRec(
                      aStatistics,
                      aTrans,
                      aLobLocator,
                      aOffset,
                      ( (smcTableHeader*)aLobViewEnv->mTable )->mSelfOID )
                  != IDE_SUCCESS );
    }

    gDiskLobStatistics.mTrim++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_max_lob_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_MaxLobErrorSize ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LOB에 모드 변환 타입을 반환한다.
 **********************************************************************/
sdcLobChangeType sdcLob::getLobChangeType( sdcLobColBuffer * aLobColBuf,
                                           smiColumn       * aColumn,
                                           UInt              aOffset, 
                                           UInt              aPieceLen )
{
    sdcLobChangeType sCType;
    sdcColInOutMode  sColumnMode;
    ULong            sLength;

    sColumnMode = aLobColBuf->mInOutMode;

    switch( sColumnMode )
    {
        case SDC_COLUMN_IN_MODE:
            sLength = aOffset + aPieceLen;
            if( sLength > aColumn->vcInOutBaseSize )
            {
                sCType = SDC_LOB_IN_TO_OUT;
            }
            else
            {
                sCType = SDC_LOB_IN_TO_IN;
            }
            break;

        case SDC_COLUMN_OUT_MODE_LOB:
            sCType = SDC_LOB_OUT_TO_OUT;
            break;

        default:
            IDE_ASSERT( 0 );
            break;
    }

    return sCType;
}

/***********************************************************************
 * Description : aPieceVal를 Write한다.
 **********************************************************************/
IDE_RC sdcLob::writeInternal( idvSQL            * aStatistics,
                              void              * aTrans,
                              smLobLocator        aLobLocator,
                              smLobViewEnv      * aLobViewEnv,
                              UInt                aPieceLen,
                              UChar             * aPieceVal,
                              sdcLobWriteType     aWriteType,
                              idBool              aIsFromAPI,
                              smrContType         aContType )
{
    sdcLobChangeType sCType;

    IDE_ERROR( aLobViewEnv != NULL );

    sCType = getLobChangeType( (sdcLobColBuffer*)aLobViewEnv->mLobColBuf,
                               &aLobViewEnv->mLobCol,
                               aLobViewEnv->mWriteOffset,
                               aPieceLen );

    switch( sCType )
    {
        case SDC_LOB_IN_TO_IN:
            IDE_TEST( writeInToIn(aLobViewEnv,
                                  aPieceVal,
                                  aPieceLen,
                                  aWriteType)
                      != IDE_SUCCESS );
            break;
            
        case SDC_LOB_IN_TO_OUT:
            IDE_TEST( writeInToOut(aStatistics,
                                   aTrans,
                                   aLobViewEnv,
                                   aLobLocator,
                                   aPieceVal,
                                   aPieceLen,
                                   aWriteType,
                                   aIsFromAPI,
                                   aContType)
                      != IDE_SUCCESS );
            break;
            
        case SDC_LOB_OUT_TO_OUT:
            IDE_TEST( writeOutToOut(aStatistics,
                                    aTrans,
                                    aLobViewEnv,
                                    aLobLocator,
                                    aPieceVal,
                                    aPieceLen,
                                    aWriteType,
                                    aIsFromAPI,
                                    aContType)
                      != IDE_SUCCESS );
            break;
            
        default:
            IDE_ERROR( 0 );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 변환 타입이 in to in 경우에 대한 Write한다.
 **********************************************************************/
IDE_RC sdcLob::writeInToIn( smLobViewEnv    * aLobViewEnv,
                            UChar           * aPieceVal,
                            UInt              aPieceLen,
                            sdcLobWriteType   aWriteType )
{
    IDE_ERROR( aLobViewEnv  != NULL );
    IDE_ERROR( aPieceVal    != NULL );

    IDE_TEST( writeBuffer(aLobViewEnv,
                          aPieceVal,
                          aPieceLen,
                          aWriteType)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 변환 타입이 in to out 경우에 대한 Write한다.
 **********************************************************************/
IDE_RC sdcLob::writeInToOut( idvSQL          * aStatistics,
                             void            * aTrans,
                             smLobViewEnv    * aLobViewEnv,
                             smLobLocator      aLobLocator,
                             UChar           * aPieceVal,
                             UInt              aPieceLen,
                             sdcLobWriteType   aWriteType,
                             idBool            aIsFromAPI,
                             smrContType       aContType )
{
    sdcLobColBuffer * sLobColBuf;
    sdcLobColBuffer   sNewLobColBuf;
    UInt              sOffset;
    UChar           * sPieceVal;
    UInt              sPieceLen;
    UInt              sState = 0;

    sLobColBuf = (sdcLobColBuffer *)aLobViewEnv->mLobColBuf;

    IDE_ERROR( aIsFromAPI == ID_TRUE );
    IDE_ERROR( aPieceVal != NULL );
    IDE_ERROR( sLobColBuf != NULL );
    IDE_ERROR( sLobColBuf->mInOutMode == SDC_COLUMN_IN_MODE );

    IDE_TEST( sdcLob::initLobColBuffer(
                  &sNewLobColBuf,
                  ID_SIZEOF(sdcLobDesc),
                  SDC_COLUMN_OUT_MODE_LOB,
                  ID_FALSE) /* aIsNullLob */
              != IDE_SUCCESS );
    sState = 1;

    initLobDesc( (sdcLobDesc*)sNewLobColBuf.mBuffer );
    
    /*
     * 1. write previous value
     */
    
    sOffset   = 0;
    sPieceVal = sLobColBuf->mBuffer;
    sPieceLen = sLobColBuf->mLength;

    if( sPieceLen > 0 )
    {
        IDE_TEST( writeDirect(aStatistics,
                              aTrans,
                              aLobViewEnv,
                              aLobLocator,
                              &sNewLobColBuf,
                              &sOffset,
                              &sPieceVal,
                              &sPieceLen,
                              sPieceLen,
                              aWriteType,
                              ID_TRUE /* aIsPrevVal */,
                              aIsFromAPI,
                              SMR_CT_CONTINUE)
                  != IDE_SUCCESS );
    }

    /* The previous in-mode values must be stored in direct way. */
    IDE_ERROR( sPieceLen == 0 );

    /*
     * 2. write new value
     */
    
    sOffset   = aLobViewEnv->mWriteOffset;
    sPieceVal = aPieceVal;
    sPieceLen = aPieceLen;

    if( sPieceLen > 0 )
    {
        IDE_TEST( writeDirect(aStatistics,
                              aTrans,
                              aLobViewEnv,
                              aLobLocator,
                              &sNewLobColBuf,
                              &sOffset,
                              &sPieceVal,
                              &sPieceLen,
                              aPieceLen,
                              aWriteType,
                              ID_FALSE /* aIsPrevVal */,
                              aIsFromAPI,
                              aContType)
                  != IDE_SUCCESS );
    }

    if( sPieceLen > 0 )
    {
        IDE_TEST( writeIndex(aStatistics,
                             aTrans,
                             aLobViewEnv,
                             aLobLocator,
                             &sNewLobColBuf,
                             &sOffset,
                             &sPieceVal,
                             &sPieceLen,
                             aPieceLen,
                             aWriteType,
                             ID_FALSE /* aIsPrevVal*/,
                             aIsFromAPI,
                             aContType)
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdcLob::finalLobColBuffer(sLobColBuf) 
              != IDE_SUCCESS );

    *sLobColBuf = sNewLobColBuf;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdcLob::finalLobColBuffer(&sNewLobColBuf) 
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 변환 타입이 out to out 경우에 대한 Write한다.
 **********************************************************************/
IDE_RC sdcLob::writeOutToOut( idvSQL          * aStatistics,
                              void            * aTrans,
                              smLobViewEnv    * aLobViewEnv,
                              smLobLocator      aLobLocator,
                              UChar           * aPieceVal,
                              UInt              aPieceLen,
                              sdcLobWriteType   aWriteType,
                              idBool            aIsFromAPI,
                              smrContType       aContType )
{
    sdcLobColBuffer * sLobColBuf;
    UInt              sOffset;
    UChar           * sPieceVal;
    UInt              sPieceLen;

    sLobColBuf = (sdcLobColBuffer*)aLobViewEnv->mLobColBuf;

    IDE_ASSERT( sLobColBuf != NULL );
    IDE_ASSERT( sLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB );

    sOffset   = aLobViewEnv->mWriteOffset;
    sPieceVal = aPieceVal;
    sPieceLen = aPieceLen;

    if( sPieceLen > 0 )
    {
        IDE_TEST( writeDirect(aStatistics,
                              aTrans,
                              aLobViewEnv,
                              aLobLocator,
                              sLobColBuf,
                              &sOffset,
                              &sPieceVal,
                              &sPieceLen,
                              aPieceLen,
                              aWriteType,
                              ID_FALSE /* aIsPrevVal */,
                              aIsFromAPI,
                              aContType)
                  != IDE_SUCCESS );
    }

    if( sPieceLen > 0 )
    {
        IDE_TEST( writeIndex(aStatistics,
                             aTrans,
                             aLobViewEnv,
                             aLobLocator,
                             sLobColBuf,
                             &sOffset,
                             &sPieceVal,
                             &sPieceLen,
                             aPieceLen,
                             aWriteType,
                             ID_FALSE /* aIsPrevVal */,
                             aIsFromAPI,
                             aContType)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LOB Index 대한 Write를 수행한다.
 **********************************************************************/
IDE_RC sdcLob::writeIndex( idvSQL           * aStatistics,
                           void             * aTrans,
                           smLobViewEnv     * aLobViewEnv,
                           smLobLocator       aLobLocator,
                           sdcLobColBuffer  * aLobColBuf,
                           UInt             * aOffset,
                           UChar           ** aPieceVal,
                           UInt             * aPieceLen,
                           UInt               aAmount,
                           sdcLobWriteType    aWriteType,
                           idBool             aIsPrevVal,
                           idBool             aIsFromAPI,
                           smrContType        aContType )
{
    sdcLobDesc  * sLobDesc;
    scPageID      sPageSeq;
    scPageID      sLeafNodePID;
    SShort        sLeafKeySeq;
    SShort        sLeafKeyEntrySeq;
    UShort        sLeafKeyCount;
    idBool        sIsAppended = ID_FALSE;

    IDE_ERROR( aLobViewEnv  != NULL );
    IDE_ERROR( aLobColBuf   != NULL );
    IDE_ERROR( aOffset      != NULL );

    IDE_ERROR( aLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB );
    IDE_ERROR( aLobColBuf->mLength    == ID_SIZEOF(sdcLobDesc) );
    
    sLobDesc = (sdcLobDesc*)aLobColBuf->mBuffer;
    
    while( (*aPieceLen) > 0 )
    {
        sPageSeq = getPageSeq( *aOffset );

        if( (aLobViewEnv->mLastWriteOffset == (*aOffset)) &&
            (aLobViewEnv->mLastWriteLeafNodePID != SD_NULL_PID) )
        {
            // sequential search
            IDE_TEST( traverseSequential(aStatistics,
                                         &aLobViewEnv->mLobCol,
                                         aLobViewEnv->mLastWriteLeafNodePID,
                                         sPageSeq,
                                         &sLeafNodePID,
                                         &sLeafKeySeq,
                                         &sLeafKeyEntrySeq,
                                         &sLeafKeyCount)
                      != IDE_SUCCESS );
        }
        else
        {
            // tree search
            IDE_TEST( traverse(aStatistics,
                               &aLobViewEnv->mLobCol,
                               sLobDesc->mRootNodePID,
                               sPageSeq,
                               &sLeafNodePID,
                               &sLeafKeySeq,
                               &sLeafKeyEntrySeq,
                               &sLeafKeyCount)
                      != IDE_SUCCESS );
        }

        if( sLeafNodePID == SD_NULL_PID )
        {
            IDE_ERROR( sIsAppended == ID_FALSE );
            
            IDE_TEST( appendLeafNode(aStatistics,
                                     aTrans,
                                     &aLobViewEnv->mLobCol,
                                     aLobViewEnv->mLobVersion,
                                     &sLobDesc->mRootNodePID)
                      != IDE_SUCCESS );
            
            sIsAppended = ID_TRUE;
            continue;
        }
        
        while( sLeafKeySeq < getMaxLeafKeyCount() )
        {
            if( (*aPieceLen) == 0 )
            {
                break;
            }

            if( sLeafKeySeq < sLeafKeyCount )
            {
                IDE_TEST( updateKey(aStatistics,
                                    aTrans,
                                    aLobViewEnv->mTable,
                                    &aLobViewEnv->mLobCol,
                                    aLobLocator,
                                    aLobViewEnv->mLobVersion,
                                    sLeafNodePID,
                                    sLeafKeySeq,
                                    aOffset,
                                    aPieceVal,
                                    aPieceLen,
                                    aAmount,
                                    aWriteType,
                                    aIsPrevVal,
                                    aIsFromAPI,
                                    aContType)
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( insertKey(aStatistics,
                                    aTrans,
                                    aLobViewEnv->mTable,
                                    &aLobViewEnv->mLobCol,
                                    aLobLocator,
                                    aLobViewEnv->mLobVersion,
                                    sLeafNodePID,
                                    sLeafKeySeq,
                                    aOffset,
                                    aPieceVal,
                                    aPieceLen,
                                    aAmount,
                                    aIsPrevVal,
                                    aIsFromAPI,
                                    aContType)
                          != IDE_SUCCESS );
            }

            sLeafKeySeq++;
        }

        aLobViewEnv->mLastWriteOffset = *aOffset;
        aLobViewEnv->mLastWriteLeafNodePID = sLeafNodePID;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Node를 Append 한다.
 **********************************************************************/
IDE_RC sdcLob::appendLeafNode( idvSQL       * aStatistics,
                               void         * aTrans,
                               smiColumn    * aColumn,
                               ULong          aLobVersion,
                               scPageID     * aRootNodePID )
{
    sdrMtx            sMtx;
    smLSN             sNTA;
    ULong             sNTAData[2];
    sdcLobStack       sStack;
    sdcLobNodeHdr   * sLeafNodeHdr;
    sdpPhyPageHdr   * sLeafNode;
    sdpPhyPageHdr   * sNewLeafNode;
    scPageID          sPID = SC_NULL_PID;
    UInt              sState = 0;

    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aRootNodePID != NULL );

    IDE_TEST( sdrMiniTrans::begin(
                  aStatistics,
                  &sMtx,
                  aTrans,
                  SDR_MTX_LOGGING,
                  ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                  SM_DLOG_ATTR_DEFAULT)
              != IDE_SUCCESS );
    sState = 1;

    if( *aRootNodePID == SD_NULL_PID )
    {
        IDE_TEST( makeNewRootNode(aStatistics,
                                  &sMtx,
                                  aColumn,
                                  aLobVersion,
                                  NULL, /* aNode */
                                  NULL, /* aNewNode */
                                  aRootNodePID)
                  != IDE_SUCCESS );
    }
    else
    {
        initStack( &sStack );

        IDE_TEST( findRightMostLeafNode(aStatistics,
                                        aColumn->colSeg.mSpaceID,
                                        *aRootNodePID,
                                        &sStack)
                  != IDE_SUCCESS );

        IDE_TEST( popStack(&sStack, &sPID) != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                             aColumn->colSpace,
                                             sPID,
                                             SDB_X_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_SINGLE_PAGE_READ,
                                             &sMtx,
                                             (UChar**)&sLeafNode,
                                             NULL,
                                             NULL)
                  != IDE_SUCCESS );

        sLeafNodeHdr = (sdcLobNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr((UChar*)sLeafNode);

        IDE_TEST( allocNewPage(aStatistics,
                               &sMtx,
                               aColumn,
                               SDP_PAGE_LOB_INDEX,
                               (UChar**)&sNewLeafNode)
                  != IDE_SUCCESS );

        IDE_TEST( initializeNodeHdr(&sMtx,
                                    sNewLeafNode,
                                    0, /* aHeight */
                                    sLeafNodeHdr->mNodeSeq + 1,
                                    aLobVersion)
                  != IDE_SUCCESS );

        IDE_TEST( propagateKey(aStatistics,
                               &sMtx,
                               aColumn,
                               aLobVersion,
                               &sStack,
                               (UChar*)sLeafNode,
                               (UChar*)sNewLeafNode,
                               aRootNodePID)
                  != IDE_SUCCESS );
 
        IDE_TEST( sdpDblPIDList::setPrvOfNode(
                      &sNewLeafNode->mListNode,
                      sdpPhyPage::getPageID((UChar*)sLeafNode),
                      &sMtx)
                  != IDE_SUCCESS );

        IDE_TEST( sdpDblPIDList::setNxtOfNode(
                      &sNewLeafNode->mListNode,
                      sLeafNode->mListNode.mNext,
                      &sMtx)
                  != IDE_SUCCESS );

        IDE_TEST( sdpDblPIDList::setNxtOfNode(
                      &sLeafNode->mListNode,
                      sdpPhyPage::getPageID((UChar*)sNewLeafNode),
                      &sMtx)
                  != IDE_SUCCESS );

        /*
         * write logical log
         */

        /*
         * allocPage에 대한 undo는 segment가 처리한다. 따라서 alloc
         * page에 대한 undo가 skip되지 않도록 NTA를 여기서 찍는다.
         */ 

        sNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

        sNTAData[0] = *aRootNodePID;
        sNTAData[1] = sdpPhyPage::getPageID((UChar*)sNewLeafNode);

        sdrMiniTrans::setNTA(&sMtx,
                             aColumn->colSeg.mSpaceID,
                             SDR_OP_SDC_LOB_APPEND_LEAFNODE,
                             &sNTA,
                             sNTAData,
                             2 /* DataCount */);
    }

    sState = 0;

    IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Node를 Append 한 것에 대한 Logical Undo를 수행한다.
 **********************************************************************/
IDE_RC sdcLob::appendLeafNodeRollback( idvSQL       * aStatistics,
                                       sdrMtx       * aMtx,
                                       scSpaceID      aSpaceID,
                                       scPageID       aRootNodePID,
                                       scPageID       aLeafNodePID )
{
    sdcLobStack       sStack;
    sdcLobNodeHdr   * sNodeHdr;
    sdpPhyPageHdr   * sNode;
    sdpPhyPageHdr   * sPrvNode;
    scPageID          sPrvPID;
    scPageID          sNxtPID;
    scPageID          sPID = SC_NULL_PID;
    UShort            sKeyCnt;

    IDE_ERROR( aMtx         != NULL );
    IDE_ERROR( aRootNodePID != SD_NULL_PID );
    
    initStack( &sStack );

    IDE_TEST( findRightMostLeafNode(aStatistics,
                                    aSpaceID,
                                    aRootNodePID,
                                    &sStack)
              != IDE_SUCCESS );

    /*
     * rollback leaf node
     */
    
    IDE_TEST( popStack(&sStack, &sPID) != IDE_SUCCESS );

    IDE_ERROR( sPID == aLeafNodePID );

    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aSpaceID,
                                         sPID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         aMtx,
                                         (UChar**)&sNode,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS );

    sNodeHdr = (sdcLobNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)sNode);

    IDE_ERROR( sNode->mPageType == SDP_PAGE_LOB_INDEX );
    IDE_ERROR( sNodeHdr->mHeight == 0 );

    sPrvPID = sdpDblPIDList::getPrvOfNode(&sNode->mListNode);
    sNxtPID = sdpDblPIDList::getNxtOfNode(&sNode->mListNode);

    IDE_ERROR( sPrvPID != SD_NULL_PID );
    IDE_ERROR( sNxtPID == SD_NULL_PID );

    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aSpaceID,
                                         sPrvPID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         aMtx,
                                         (UChar**)&sPrvNode,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS );

    IDE_TEST( sdpDblPIDList::setNxtOfNode(
                  &sPrvNode->mListNode,
                  sNxtPID,
                  aMtx)
              != IDE_SUCCESS );

    /*
     * rollback internal node
     */

    while( getPosStack(&sStack) >= 0 )
    {
        IDE_TEST( popStack(&sStack, &sPID) != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                             aSpaceID,
                                             sPID,
                                             SDB_X_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_SINGLE_PAGE_READ,
                                             aMtx,
                                             (UChar**)&sNode,
                                             NULL,
                                             NULL)
                  != IDE_SUCCESS );

        sNodeHdr = (sdcLobNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr((UChar*)sNode);

        IDE_ERROR( sNodeHdr->mHeight > 0 );
        IDE_ERROR( sNodeHdr->mKeyCnt > 0 );

        sKeyCnt = sNodeHdr->mKeyCnt - 1;

        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&sNodeHdr->mKeyCnt,
                                            (void*)&sKeyCnt,
                                            ID_SIZEOF(sKeyCnt))
                  != IDE_SUCCESS );

        if( sNodeHdr->mKeyCnt > 0 )
        {
            break;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 새로운 Root Node를 생성한다.
 **********************************************************************/
IDE_RC sdcLob::makeNewRootNode( idvSQL      * aStatistics,
                                sdrMtx      * aMtx,
                                smiColumn   * aColumn,
                                ULong         aLobVersion,
                                UChar       * aNode,
                                UChar       * aNewNode,
                                scPageID    * aRootNodePID )
{
    sdcLobNodeHdr   * sNodeHdr;
    UChar           * sRootNode;

    IDE_ERROR( aMtx         != NULL );
    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aRootNodePID != NULL );

    IDE_TEST( allocNewPage(aStatistics,
                           aMtx,
                           aColumn,
                           SDP_PAGE_LOB_INDEX,
                           &sRootNode)
              != IDE_SUCCESS );

    if( aNode == NULL )
    {
        IDE_ERROR( aNewNode == NULL );
        
        IDE_TEST( initializeNodeHdr(aMtx,
                                    (sdpPhyPageHdr*)sRootNode,
                                    0, /* aHeight */
                                    0, /* aNodeSeq */
                                    aLobVersion)
                  != IDE_SUCCESS );
    }
    else
    {
        sNodeHdr = (sdcLobNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr(aNode);

        IDE_TEST( initializeNodeHdr(aMtx,
                                    (sdpPhyPageHdr*)sRootNode,
                                    sNodeHdr->mHeight + 1, /* aHeight */
                                    0, /* aNodeSeq */
                                    aLobVersion)
                  != IDE_SUCCESS );

        IDE_TEST( insertInternalKey(aMtx,
                                    sRootNode,
                                    sdpPhyPage::getPageID(aNode))
                  != IDE_SUCCESS );

        IDE_TEST( insertInternalKey(aMtx,
                                    sRootNode,
                                    sdpPhyPage::getPageID(aNewNode))
                  != IDE_SUCCESS );
    }

    *aRootNodePID = sdpPhyPage::getPageID(sRootNode);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 새로운 Internal Key를 삽입한다.
 **********************************************************************/
IDE_RC sdcLob::insertInternalKey( sdrMtx    * aMtx,
                                  UChar     * aNode,
                                  scPageID    aPageID )
{
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobIKey      * sIKeyArray;
    sdcLobIKey      * sIKey;
    SShort            sKeySeq;

    IDE_ERROR( aMtx  != NULL );
    IDE_ERROR( aNode != NULL );

    sNodeHdr   = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aNode);
    sIKeyArray = (sdcLobIKey*)getLobDataLayerStartPtr(aNode);

    IDE_ERROR( sNodeHdr->mKeyCnt < getMaxInternalKeyCount() );

    sKeySeq       = sNodeHdr->mKeyCnt;
    sIKey         = &sIKeyArray[sKeySeq];
    sIKey->mChild = aPageID;

    sNodeHdr->mKeyCnt++;

    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)sIKey,
                  (void*)NULL,
                  ID_SIZEOF(SShort) + ID_SIZEOF(sdcLobIKey),
                  SDR_SDC_LOB_INSERT_INTERNAL_KEY)
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(aMtx,
                                  (void*)&sKeySeq,
                                  ID_SIZEOF(sKeySeq))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(aMtx,
                                  (void*)sIKey,
                                  ID_SIZEOF(sdcLobIKey))
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Internal Node의 공간 존재 유무를 반환한다.
 **********************************************************************/
idBool sdcLob::isInternalNodeFull( UChar * sNode )
{
    sdcLobNodeHdr * sNodeHdr;
    idBool          sRet;

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sNode);

    if( sNodeHdr->mKeyCnt >= getMaxInternalKeyCount() )
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

/***********************************************************************
 * Description : Key를 propagate 한다.
 **********************************************************************/
IDE_RC sdcLob::propagateKey( idvSQL         * aStatistics,
                             sdrMtx         * aMtx,
                             smiColumn      * aColumn,
                             ULong            aLobVersion,
                             sdcLobStack    * aStack,
                             UChar          * aNode,
                             UChar          * aNewNode,
                             scPageID       * aRootNodePID )
{
    scPageID          sPID;
    sdpPhyPageHdr   * sINode;
    sdpPhyPageHdr   * sNewINode;
    sdcLobNodeHdr   * sINodeHdr;

    IDE_ERROR( aMtx         != NULL );
    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aStack       != NULL );
    IDE_ERROR( aNode        != NULL );
    IDE_ERROR( aNewNode     != NULL );
    IDE_ERROR( aRootNodePID != NULL );
    
    if( getPosStack(aStack) < 0 )
    {
        IDE_TEST( makeNewRootNode(aStatistics,
                                  aMtx,
                                  aColumn,
                                  aLobVersion,
                                  aNode,
                                  aNewNode,
                                  aRootNodePID)
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( popStack(aStack, &sPID) != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                             aColumn->colSpace,
                                             sPID,
                                             SDB_X_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_SINGLE_PAGE_READ,
                                             aMtx,
                                             (UChar**)&sINode,
                                             NULL,
                                             NULL)
                  != IDE_SUCCESS );

        if( isInternalNodeFull((UChar*)sINode) == ID_TRUE )
        {
            sINodeHdr = (sdcLobNodeHdr*)
                sdpPhyPage::getLogicalHdrStartPtr((UChar*)sINode);
        
            IDE_TEST( allocNewPage(aStatistics,
                                   aMtx,
                                   aColumn,
                                   SDP_PAGE_LOB_INDEX,
                                   (UChar**)&sNewINode)
                      != IDE_SUCCESS );

            IDE_TEST( initializeNodeHdr(aMtx,
                                        sNewINode,
                                        sINodeHdr->mHeight, /* aHeight */
                                        sINodeHdr->mNodeSeq + 1,
                                        aLobVersion)
                      != IDE_SUCCESS );

            IDE_TEST( insertInternalKey(
                          aMtx,
                          (UChar*)sNewINode,
                          sdpPhyPage::getPageID(aNewNode))
                      != IDE_SUCCESS );

            IDE_TEST( propagateKey( aStatistics,
                                    aMtx,
                                    aColumn,
                                    aLobVersion,
                                    aStack,
                                    (UChar*)sINode,
                                    (UChar*)sNewINode,
                                    aRootNodePID )
                      != IDE_SUCCESS );

            IDE_TEST( sdpDblPIDList::setPrvOfNode(
                          &(sNewINode->mListNode),
                          sdpPhyPage::getPageID((UChar*)sINode),
                          aMtx)
                      != IDE_SUCCESS );

            IDE_TEST( sdpDblPIDList::setNxtOfNode(
                          &(sNewINode->mListNode),
                          sINode->mListNode.mNext,
                          aMtx)
                      != IDE_SUCCESS );

            IDE_TEST( sdpDblPIDList::setNxtOfNode(
                          &(sINode->mListNode),
                          sdpPhyPage::getPageID((UChar*)sNewINode),
                          aMtx)
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( insertInternalKey(
                          aMtx,
                          (UChar*)sINode,
                          sdpPhyPage::getPageID((UChar*)aNewNode))
                      != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 맨 오른쪽 Leaf Node를 탐색한다.
 **********************************************************************/
IDE_RC sdcLob::findRightMostLeafNode( idvSQL        * aStatistics,
                                      scSpaceID       aLobColSpaceID,
                                      scPageID        aRootNodePID,
                                      sdcLobStack   * aStack )
{
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobIKey      * sIKeyArray;
    scPageID          sPID;
    UChar           * sPage;
    UInt              sState = 0;

    IDE_ERROR( aRootNodePID != SD_NULL_PID );
    IDE_ERROR( aStack       != NULL );

    sPID = aRootNodePID;

    IDE_TEST( sdbBufferMgr::getPageByPID(
                  aStatistics,
                  aLobColSpaceID,
                  sPID,
                  SDB_S_LATCH,
                  SDB_WAIT_NORMAL,
                  SDB_SINGLE_PAGE_READ,
                  NULL, /* aMtx */
                  &sPage,
                  NULL,
                  NULL)
              != IDE_SUCCESS );
    sState = 1;

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sPage);

    IDE_TEST( pushStack( aStack, sPID ) != IDE_SUCCESS );

    while( sNodeHdr->mHeight > 0 )
    {
        sIKeyArray = (sdcLobIKey*)getLobDataLayerStartPtr(sPage);

        IDE_ERROR( sNodeHdr->mKeyCnt > 0 );    
        sPID = sIKeyArray[sNodeHdr->mKeyCnt - 1].mChild;
        
        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics, sPage)
                  != IDE_SUCCESS );

        IDE_ERROR( sPID != SD_NULL_PID );

        IDE_TEST( sdbBufferMgr::getPageByPID(
                      aStatistics,
                      aLobColSpaceID,
                      sPID,
                      SDB_S_LATCH,
                      SDB_WAIT_NORMAL,
                      SDB_SINGLE_PAGE_READ,
                      NULL, /* aMtx */
                      &sPage,
                      NULL,
                      NULL)
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( pushStack( aStack, sPID ) != IDE_SUCCESS );

        sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sPage);
    }

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage(aStatistics, sPage)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, sPage)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Key를 update한다.
 **********************************************************************/
IDE_RC sdcLob::updateKey( idvSQL            * aStatistics,
                          void              * aTrans,
                          void              * aTable,
                          smiColumn         * aColumn,
                          smLobLocator        aLobLocator,
                          ULong               aLobVersion,
                          scPageID            aLeafNodePID,
                          SShort              aLeafKeySeq,
                          UInt              * aOffset,
                          UChar            ** aPieceVal,
                          UInt              * aPieceLen,
                          UInt                aAmount,
                          sdcLobWriteType     aWriteType,
                          idBool              aIsPrevVal,
                          idBool              aIsFromAPI,
                          smrContType         aContType )
{
    sdcLobLKey sLKey;

    IDE_TEST( makeUpdateLKey(aStatistics,
                             aTrans,
                             aTable,
                             aColumn,
                             aLobLocator,
                             aLobVersion,
                             aOffset,
                             aPieceVal,
                             aPieceLen,
                             aAmount,
                             aLeafNodePID,
                             aLeafKeySeq,
                             aWriteType,
                             &sLKey,
                             aIsPrevVal,
                             aIsFromAPI,
                             aContType)
              != IDE_SUCCESS );

    IDE_TEST( writeUpdateLKey(aStatistics,
                              aTrans,
                              aTable,
                              aColumn,
                              aLeafNodePID,
                              aLeafKeySeq,
                              &sLKey)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Key를 생성한다.
 **********************************************************************/
IDE_RC sdcLob::makeUpdateLKey( idvSQL         * aStatistics,
                               void           * aTrans,
                               void           * aTable,
                               smiColumn      * aColumn,
                               smLobLocator     aLobLocator,
                               ULong            aLobVersion,
                               UInt           * aOffset,
                               UChar         ** aPieceVal,
                               UInt           * aPieceLen,
                               UInt             aAmount,
                               scPageID         aLeafNodePID,
                               SShort           aLeafKeySeq,
                               sdcLobWriteType  aWriteType,
                               sdcLobLKey     * aLKey,
                               idBool           aIsPrevVal,
                               idBool           aIsFromAPI,
                               smrContType      aContType )
{
    SShort            sStartEntrySeq;
    sdcLobLKey        sOldLKey;
    sdcLobLKey      * sKeyArray;
    UChar           * sLeafNode;
    UInt              i;
    UInt              sState = 0;

    IDE_ERROR( aColumn   != NULL );
    IDE_ERROR( aOffset   != NULL );
    IDE_ERROR( aPieceVal != NULL );
    IDE_ERROR( aPieceLen != NULL );
    IDE_ERROR( aLKey     != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aColumn->colSpace,
                                         aLeafNodePID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         NULL, /* aMtx */
                                         &sLeafNode,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS );
    sState = 1;

    sKeyArray = (sdcLobLKey*)getLobDataLayerStartPtr(sLeafNode);
    *aLKey    = sOldLKey = sKeyArray[aLeafKeySeq];

    aLKey->mLobVersion = aLobVersion;

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage(aStatistics, sLeafNode)
              != IDE_SUCCESS );

    /*
     * make new leaf key
     */
    
    sStartEntrySeq = getEntrySeq(*aOffset);

    for( i = sStartEntrySeq; i < SDC_LOB_MAX_ENTRY_CNT; i++ )
    {
        if( *aPieceLen == 0 )
        {
            break;
        }

        switch(aWriteType)
        {
            case SDC_LOB_WTYPE_WRITE:
                
                IDE_TEST( writeEntry(aStatistics,
                                     aTrans,
                                     aTable,
                                     aColumn,
                                     aLobLocator,
                                     aLobVersion,
                                     aOffset,
                                     aPieceVal,
                                     aPieceLen,
                                     aAmount,
                                     i,
                                     aLKey,
                                     aIsPrevVal,
                                     aIsFromAPI,
                                     aContType)
                          != IDE_SUCCESS );
                break;
                
            case SDC_LOB_WTYPE_ERASE:
                
                IDE_TEST( eraseEntry(aStatistics,
                                     aTrans,
                                     aColumn,
                                     aLobVersion,
                                     aOffset,
                                     aPieceLen,
                                     i,
                                     aLKey)
                          != IDE_SUCCESS );
                break;
                
            case SDC_LOB_WTYPE_TRIM:
                
                IDE_TEST( trimEntry(aStatistics,
                                    aTrans,
                                    aColumn,
                                    aLobVersion,
                                    aOffset,
                                    aPieceLen,
                                    i,
                                    aLKey)
                          != IDE_SUCCESS );
                break;
                
            default:
                IDE_ERROR( 0 );
                break;
        }
    }

    IDE_TEST( removeOldPages(aStatistics,
                             aTrans,
                             aColumn,
                             &sOldLKey,
                             aLKey)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, sLeafNode)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Key를 Leaf Node에 삽입한다.
 **********************************************************************/
IDE_RC sdcLob::writeUpdateLKey( idvSQL      * aStatistics,
                                void        * aTrans,
                                void        * aTable,
                                smiColumn   * aColumn,
                                scPageID      aLeafNodePID,
                                SShort        aLeafKeySeq,
                                sdcLobLKey  * aLKey )
{
    sdrMtx            sMtx;
    sdSID             sUndoSID;
    sdrMtxStartInfo   sStartInfo;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobLKey      * sKeyArray;
    sdcLobLKey        sOldLKey;
    UChar           * sLeafNode;
    UInt              sState = 0;

    IDE_ERROR( aColumn != NULL );
    IDE_ERROR( aLKey    != NULL );

    IDE_TEST( sdrMiniTrans::begin(
                  aStatistics,
                  &sMtx,
                  aTrans,
                  SDR_MTX_LOGGING,
                  ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                  SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_UNDOABLE)
              != IDE_SUCCESS );
    sState = 1;

    /*
     * write key
     */
    
    IDE_TEST( sdbBufferMgr::getPageByPID(
                  aStatistics,
                  aColumn->colSpace,
                  aLeafNodePID,
                  SDB_X_LATCH,
                  SDB_WAIT_NORMAL,
                  SDB_SINGLE_PAGE_READ,
                  &sMtx,
                  &sLeafNode,
                  NULL,
                  NULL)
              != IDE_SUCCESS );

    sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sLeafNode);
    sKeyArray = (sdcLobLKey*)getLobDataLayerStartPtr((UChar*)sNodeHdr);

    IDE_ERROR( sNodeHdr->mKeyCnt > aLeafKeySeq );

    sOldLKey = sKeyArray[aLeafKeySeq];

    if( sOldLKey.mLobVersion == aLKey->mLobVersion )
    {
        idlOS::memcpy( &sKeyArray[aLeafKeySeq],
                       aLKey,
                       ID_SIZEOF(sdcLobLKey) );

        /* write log */
        IDE_TEST( sdrMiniTrans::writeLogRec(
                      &sMtx,
                      (UChar*)&sKeyArray[aLeafKeySeq],
                      (void*)NULL,
                      ID_SIZEOF(SShort) +
                      ID_SIZEOF(sdcLobLKey) +
                      ID_SIZEOF(sdcLobLKey),
                      SDR_SDC_LOB_OVERWRITE_LEAF_KEY )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( &sMtx,
                                       (void*)&aLeafKeySeq,
                                       ID_SIZEOF(aLeafKeySeq) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( &sMtx,
                                       (void*)&sOldLKey,
                                       ID_SIZEOF(sdcLobLKey) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( &sMtx,
                                       (void*)aLKey,
                                       ID_SIZEOF(sdcLobLKey) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* add undo record */
        sdrMiniTrans::makeStartInfo( &sMtx, &sStartInfo );

        IDE_TEST( smxTrans::allocTXSegEntry( aStatistics, &sStartInfo )
                  != IDE_SUCCESS );        

        IDE_TEST(
            smxTrans::getUDSegPtr(
                (smxTrans*)(sStartInfo.mTrans))->addUpdateLobLeafKeyUndoRec(
                    aStatistics,
                    &sStartInfo,
                    smLayerCallback::getTableOID( aTable ),
                    (UChar*)&sOldLKey,
                    &sUndoSID )
            != IDE_SUCCESS );

        aLKey->mUndoSID = sUndoSID;

        idlOS::memcpy( &sKeyArray[aLeafKeySeq],
                       aLKey,
                       ID_SIZEOF(sdcLobLKey) );

        /* write log */
        IDE_TEST( sdrMiniTrans::writeLogRec(
                      &sMtx,
                      (UChar*)&sKeyArray[aLeafKeySeq],
                      (void*)NULL,
                      ID_SIZEOF(SShort) +
                      ID_SIZEOF(sdcLobLKey),
                      SDR_SDC_LOB_UPDATE_LEAF_KEY )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( &sMtx,
                                       (void*)&aLeafKeySeq,
                                       ID_SIZEOF(aLeafKeySeq) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( &sMtx,
                                       (void*)aLKey,
                                       ID_SIZEOF(sdcLobLKey) )
                  != IDE_SUCCESS );
    }
    
    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Key를 특정 Entry 페이지를 Write한다.
 **********************************************************************/
IDE_RC sdcLob::writeEntry( idvSQL        * aStatistics,
                           void          * aTrans,
                           void          * aTable,
                           smiColumn     * aColumn,
                           smLobLocator    aLobLocator,
                           ULong           aLobVersion,
                           UInt          * aOffset,
                           UChar        ** aPieceVal,
                           UInt          * aPieceLen,
                           UInt            aAmount,
                           UInt            aEntrySeq,
                           sdcLobLKey    * aLKey,
                           idBool          aIsPrevVal,
                           idBool          aIsFromAPI,
                           smrContType     aContType )
{
    scPageID    sPID;
    scPageID    sNewPID;

    IDE_ERROR( aColumn   != NULL );
    IDE_ERROR( aOffset   != NULL );
    IDE_ERROR( aPieceVal != NULL );
    IDE_ERROR( aPieceLen != NULL );
    IDE_ERROR( aLKey     != NULL );

    IDE_ERROR( aEntrySeq < SDC_LOB_MAX_ENTRY_CNT );

    sPID = aLKey->mEntry[aEntrySeq];

    IDE_TEST( writePage(aStatistics,
                        aTrans,
                        aTable,
                        aColumn,
                        aLobLocator,
                        aLobVersion,
                        aOffset,
                        aPieceVal,
                        aPieceLen,
                        aAmount,
                        sPID,
                        &sNewPID,
                        aIsPrevVal,
                        aIsFromAPI,
                        aContType)
              != IDE_SUCCESS );

    aLKey->mEntry[aEntrySeq] = sNewPID;

    if( aEntrySeq >= aLKey->mEntryCnt )
    {
        aLKey->mEntryCnt++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Key를 특정 Entry 페이지를 erase한다.
 **********************************************************************/
IDE_RC sdcLob::eraseEntry( idvSQL        * aStatistics,
                           void          * aTrans,
                           smiColumn     * aColumn,
                           ULong           aLobVersion,
                           UInt          * aOffset,
                           UInt          * aPieceLen,
                           UInt            aEntrySeq,
                           sdcLobLKey    * aLKey )
{
    scPageID    sPID;
    scPageID    sNewPID;

    IDE_ERROR( aOffset   != NULL );
    IDE_ERROR( aPieceLen != NULL );
    IDE_ERROR( aLKey     != NULL );
    IDE_ERROR( aEntrySeq < SDC_LOB_MAX_ENTRY_CNT );
    IDE_ERROR( aEntrySeq < aLKey->mEntryCnt );

    sPID = aLKey->mEntry[aEntrySeq];

    IDE_TEST( erasePage(aStatistics,
                        aTrans,
                        aColumn,
                        aLobVersion,
                        aOffset,
                        aPieceLen,
                        sPID,
                        &sNewPID)
              != IDE_SUCCESS );

    aLKey->mEntry[aEntrySeq] = sNewPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Key를 특정 Entry 페이지를 trim한다.
 **********************************************************************/
IDE_RC sdcLob::trimEntry( idvSQL        * aStatistics,
                          void          * aTrans,
                          smiColumn     * aColumn,
                          ULong           aLobVersion,
                          UInt          * aOffset,
                          UInt          * aPieceLen,
                          UInt            aEntrySeq,
                          sdcLobLKey    * aLKey )
{
    scPageID    sPID;
    scPageID    sNewPID;

    IDE_ERROR( aColumn   != NULL );
    IDE_ERROR( aOffset   != NULL );
    IDE_ERROR( aPieceLen != NULL );
    IDE_ERROR( aLKey     != NULL );
    IDE_ERROR( aEntrySeq < SDC_LOB_MAX_ENTRY_CNT );

    sPID = aLKey->mEntry[aEntrySeq];

    IDE_TEST( trimPage(aStatistics,
                       aTrans,
                       aColumn,
                       aLobVersion,
                       aOffset,
                       aPieceLen,
                       sPID,
                       &sNewPID)
              != IDE_SUCCESS );

    aLKey->mEntry[aEntrySeq] = sNewPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 페이지를 trim한다.
 **********************************************************************/
IDE_RC sdcLob::trimPage( idvSQL      * aStatistics,
                         void        * aTrans,
                         smiColumn   * aColumn,
                         ULong         aLobVersion,
                         UInt        * aOffset,
                         UInt        * aPieceLen,
                         scPageID      aPageID,
                         scPageID    * aNewPageID )
{
    sdrMtx          sMtx;
    sdcLobNodeHdr * sNodeHdr;
    UChar         * sPage;
    UChar         * sNewPage;
    UInt            sWriteOffset;
    UInt            sWriteSize;
    UInt            sState = 0;

    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aOffset      != NULL );
    IDE_ERROR( aPieceLen    != NULL );
    IDE_ERROR( aNewPageID   != NULL );

    IDE_TEST( getWritePageOffsetAndSize(*aOffset,
                                        *aPieceLen,
                                        &sWriteOffset,
                                        &sWriteSize)
              != IDE_SUCCESS );

    if( aPageID == SD_NULL_PID )
    {
        sPage = NULL;
    }
    else
    {
        IDE_TEST( sdrMiniTrans::begin(
                      aStatistics,
                      &sMtx,
                      aTrans,
                      SDR_MTX_LOGGING,
                      ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                      SM_DLOG_ATTR_DEFAULT)
                  != IDE_SUCCESS );
        sState = 1;
        
        IDE_TEST( sdbBufferMgr::getPageByPID(
                      aStatistics,
                      aColumn->colSeg.mSpaceID,
                      aPageID,
                      SDB_X_LATCH,
                      SDB_WAIT_NORMAL,
                      SDB_SINGLE_PAGE_READ,
                      &sMtx, /* aMtx */
                      &sPage,
                      NULL,
                      NULL)
                  != IDE_SUCCESS );

        sNodeHdr = (sdcLobNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr(sPage);

        if( isFullPageWrite(sNodeHdr, aOffset, aPieceLen) == ID_TRUE )
        {
            sPage = NULL;
        }
        else
        {
            if( sNodeHdr->mLobVersion != aLobVersion )
            {
                IDE_TEST( allocNewPage(aStatistics,
                                       &sMtx,
                                       aColumn,
                                       SDP_PAGE_LOB_DATA,
                                       &sNewPage)
                          != IDE_SUCCESS );

                IDE_TEST( initializeNodeHdr(&sMtx,
                                            (sdpPhyPageHdr*)sNewPage,
                                            0, /* aHeight */
                                            0, /* aNodeSeq */
                                            aLobVersion)
                          != IDE_SUCCESS );

                IDE_TEST( copyAndTrimPage(&sMtx,
                                          sNewPage,
                                          sPage,
                                          sWriteOffset)
                          != IDE_SUCCESS );

                sPage = sNewPage;
            }
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }

    *aOffset   += sWriteSize;
    *aPieceLen -= sWriteSize;

    if( sPage == NULL )
    {
        *aNewPageID = SD_NULL_PID;
    }
    else
    {
        *aNewPageID = sdpPhyPage::getPageID(sPage);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Key를 삽입한다.
 **********************************************************************/
IDE_RC sdcLob::insertKey( idvSQL        * aStatistics,
                          void          * aTrans,
                          void          * aTable,
                          smiColumn     * aColumn,
                          smLobLocator    aLobLocator,
                          ULong           aLobVersion,
                          scPageID        aLeafNodePID,
                          SShort          aLeafKeySeq,
                          UInt          * aOffset,
                          UChar        ** aPieceVal,
                          UInt          * aPieceLen,
                          UInt            aAmount,
                          idBool          aIsPrevVal,
                          idBool          aIsFromAPI,
                          smrContType     aContType )
{
    sdcLobLKey   sLKey;

    IDE_ERROR( aColumn   != NULL );
    IDE_ERROR( aOffset   != NULL );
    IDE_ERROR( aPieceVal != NULL );
    IDE_ERROR( aPieceLen != NULL );

    IDE_TEST( makeInsertLKey(aStatistics,
                             aTrans,
                             aTable,
                             aColumn,
                             aLobLocator,
                             aLobVersion,
                             aOffset,
                             aPieceVal,
                             aPieceLen,
                             aAmount,
                             &sLKey,
                             aIsPrevVal,
                             aIsFromAPI,
                             aContType)
              != IDE_SUCCESS );

    IDE_TEST( writeInsertLKey(aStatistics,
                              aTrans,
                              aColumn,
                              aLeafNodePID,
                              aLeafKeySeq,
                              &sLKey)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Insert를 위한 Leaf Key를 생성한다.
 **********************************************************************/
IDE_RC sdcLob::makeInsertLKey( idvSQL         * aStatistics,
                               void           * aTrans,
                               void           * aTable,
                               smiColumn      * aColumn,
                               smLobLocator     aLobLocator,
                               ULong            aLobVersion,
                               UInt           * aOffset,
                               UChar         ** aPieceVal,
                               UInt           * aPieceLen,
                               UInt             aAmount,
                               sdcLobLKey     * aKey,
                               idBool           aIsPrevVal,
                               idBool           aIsFromAPI,
                               smrContType      aContType )
{
    sdrMtx    sMtx;
    UChar   * sPage;
    scPageID  sPageID;
    UInt      i;
    UInt      sState = 0;

    IDE_ERROR( aColumn   != NULL );
    IDE_ERROR( aOffset   != NULL );
    IDE_ERROR( aPieceVal != NULL );
    IDE_ERROR( aPieceLen != NULL );
    IDE_ERROR( aKey      != NULL );

    /* initialize leaf key */
    aKey->mLobVersion = aLobVersion;
    aKey->mEntryCnt   = 0;
    aKey->mUndoSID    = SD_NULL_SID;

    for( i = 0; i < SDC_LOB_MAX_ENTRY_CNT; i++ )
    {
        aKey->mEntry[i] = SD_NULL_PID;
    }

    /* alloc page & write */
    for( i = 0; i < SDC_LOB_MAX_ENTRY_CNT; i++ )
    {
        if( *aPieceLen == 0 )
        {
            break;
        }

        IDE_TEST( sdrMiniTrans::begin(aStatistics,
                                      &sMtx,
                                      aTrans,
                                      SDR_MTX_LOGGING,
                                      ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                                      SM_DLOG_ATTR_DEFAULT)
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( allocNewPage(aStatistics,
                               &sMtx,
                               aColumn,
                               SDP_PAGE_LOB_DATA,
                               &sPage)
                  != IDE_SUCCESS );

        IDE_TEST( initializeNodeHdr(&sMtx,
                                    (sdpPhyPageHdr*)sPage,
                                    0, /* aHeight */
                                    0, /* aNodeSeq */
                                    aLobVersion)
                  != IDE_SUCCESS );

        sPageID = sdpPhyPage::getPageID(sPage);

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE)
                  != IDE_SUCCESS );

        IDE_TEST( writeLobPiece(aStatistics,
                                aTrans,
                                aTable,
                                aColumn,
                                aLobLocator,
                                sPageID,
                                aOffset,
                                aPieceVal,
                                aPieceLen,
                                aAmount,
                                aIsPrevVal,
                                aIsFromAPI,
                                aContType)
                  != IDE_SUCCESS );

        aKey->mEntryCnt++;
        aKey->mEntry[i] = sdpPhyPage::getPageID(sPage);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Insert를 위한 Leaf Key를 Leaf Node에 삽입한다.
 **********************************************************************/
IDE_RC sdcLob::writeInsertLKey( idvSQL      * aStatistics,
                                void        * aTrans,
                                smiColumn   * aColumn,
                                scPageID      aLeafNodePID,
                                SShort        aLeafKeySeq,
                                sdcLobLKey  * aKey )
{
    sdrMtx            sMtx;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobLKey      * sLKeyArray;
    UChar           * sLeafNode;
    UInt              sState = 0;

    IDE_ERROR( aColumn     != NULL );
    IDE_ERROR( aKey        != NULL );
    IDE_ERROR( aLeafKeySeq >= 0 );

    IDE_TEST( sdrMiniTrans::begin(aStatistics,
                                  &sMtx,
                                  aTrans,
                                  SDR_MTX_LOGGING,
                                  ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                                  SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_UNDOABLE)
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aColumn->colSpace,
                                         aLeafNodePID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         &sMtx,
                                         &sLeafNode,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS );

    sNodeHdr   = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sLeafNode);
    sLKeyArray = (sdcLobLKey*)getLobDataLayerStartPtr((UChar*)sNodeHdr);

    IDE_ERROR( sNodeHdr->mKeyCnt == aLeafKeySeq );
    IDE_ERROR( sNodeHdr->mKeyCnt < getMaxLeafKeyCount() );

    sNodeHdr->mKeyCnt++;

    idlOS::memcpy( &sLKeyArray[aLeafKeySeq], aKey, ID_SIZEOF(sdcLobLKey) );

    IDE_TEST( sdrMiniTrans::writeLogRec( &sMtx,
                                         (UChar*)&sLKeyArray[aLeafKeySeq],
                                         (void*)NULL,
                                         ID_SIZEOF(SShort) + ID_SIZEOF(sdcLobLKey),
                                         SDR_SDC_LOB_INSERT_LEAF_KEY )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (void*)&aLeafKeySeq,
                                   ID_SIZEOF(SShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (void*)aKey,
                                   ID_SIZEOF(sdcLobLKey) )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE)
              != IDE_SUCCESS );
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aOffset으로부터 Page Sequence를 반환한다.
 **********************************************************************/
UInt sdcLob::getPageSeq( UInt aOffset )
{
    return aOffset / SDC_LOB_PAGE_BODY_SIZE;
}

/***********************************************************************
 * Description : aOffset으로부터 Entry Sequence를 반환한다.
 **********************************************************************/
SShort sdcLob::getEntrySeq( UInt aOffset )
{
    UInt    sPageSeq;
    SShort  sEntrySeq = -1;

    sPageSeq = aOffset / SDC_LOB_PAGE_BODY_SIZE;

    if( sPageSeq >= SDC_LOB_MAX_DIRECT_PAGE_CNT )
    {
        sEntrySeq =
            (sPageSeq - SDC_LOB_MAX_DIRECT_PAGE_CNT) % SDC_LOB_MAX_ENTRY_CNT;
    }

    return sEntrySeq;
}

/***********************************************************************
 * Description : Lob Data Layer 시작 주소를 반환한다.
 **********************************************************************/
UChar* sdcLob::getLobDataLayerStartPtr( UChar * aPage )
{
    IDE_ASSERT( aPage != NULL );

    return sdpPhyPage::getSlotDirStartPtr(aPage);
}

/***********************************************************************
 * Description : Max Leaf Key Count를 반환한다.
 **********************************************************************/
UShort sdcLob::getMaxLeafKeyCount()
{
    return (SDC_LOB_PAGE_BODY_SIZE / ID_SIZEOF(sdcLobLKey));
}

/***********************************************************************
 * Description : Max Internal Key Count를 반환한다.
 **********************************************************************/
UShort sdcLob::getMaxInternalKeyCount()
{
    return (SDC_LOB_PAGE_BODY_SIZE / ID_SIZEOF(sdcLobIKey));
}

/***********************************************************************
 * Description : aPageSeq에 해당하는 Internal Key를 반환한다.
 **********************************************************************/
IDE_RC sdcLob::findInternalKey( sdcLobNodeHdr * aNodeHdr,
                                UInt            aPageSeq,
                                sdcLobIKey    * aIKey )
{
    sdcLobIKey  * sKeyArray;
    UInt          sNodePageSeq;
    UInt          sKeySeq;
    UInt          sInterval;

    IDE_ERROR( aNodeHdr != NULL );
    IDE_ERROR( aIKey    != NULL );
    IDE_ERROR( aNodeHdr->mHeight > 0 );

    SDC_LOB_SET_IKEY(aIKey, SD_NULL_PID);

    sInterval =
        (UInt)(idlOS::pow( getMaxInternalKeyCount(), (aNodeHdr->mHeight - 1) ) *
               ( getMaxLeafKeyCount() * SDC_LOB_MAX_ENTRY_CNT ));

    sNodePageSeq =
        SDC_LOB_MAX_DIRECT_PAGE_CNT +
        (aNodeHdr->mNodeSeq * getMaxInternalKeyCount() * sInterval);

    IDE_ERROR( aPageSeq >= sNodePageSeq );

    sKeySeq = (aPageSeq - sNodePageSeq) / sInterval;
    
    if( sKeySeq >= getMaxInternalKeyCount() )
    {
        SDC_LOB_SET_IKEY(aIKey, SD_NULL_PID);
    }
    else
    {
        if( sKeySeq < aNodeHdr->mKeyCnt )
        {
            sKeyArray = (sdcLobIKey*)getLobDataLayerStartPtr((UChar*)aNodeHdr);
            SDC_LOB_SET_IKEY(aIKey, sKeyArray[sKeySeq].mChild);

        }
        else
        {
            SDC_LOB_SET_IKEY(aIKey, SD_NULL_PID);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPageSeq에 해당하는 Leaf Key를 반환한다.
 **********************************************************************/
IDE_RC sdcLob::findLeafKey( sdcLobNodeHdr * aNodeHdr,
                            UInt            aPageSeq,
                            SShort        * aKeySeq,
                            SShort        * aEntrySeq,
                            UShort        * aKeyCount )
{
    UInt    sInterval;
    UInt    sNodePageSeq;
    SShort  sKeySeq;
    SShort  sEntrySeq;

    IDE_ERROR( aNodeHdr  != NULL );
    IDE_ERROR( aKeySeq   != NULL );
    IDE_ERROR( aEntrySeq != NULL );
    IDE_ERROR( aNodeHdr->mHeight == 0 );

    *aKeySeq   = SDC_LOB_INVALID_KEY_SEQ;
    *aEntrySeq = SDC_LOB_INVALID_ENTRY_SEQ;

    sInterval = SDC_LOB_MAX_ENTRY_CNT;

    sNodePageSeq =
        SDC_LOB_MAX_DIRECT_PAGE_CNT +
        (aNodeHdr->mNodeSeq * getMaxLeafKeyCount() * SDC_LOB_MAX_ENTRY_CNT);

    IDE_ERROR( aPageSeq >= sNodePageSeq );

    sKeySeq   = (aPageSeq - sNodePageSeq) / sInterval;
    if( sKeySeq < getMaxLeafKeyCount() )
    {
        sEntrySeq = (aPageSeq - sNodePageSeq) % sInterval;
        IDE_ERROR( sEntrySeq < SDC_LOB_MAX_ENTRY_CNT );

        *aKeySeq   = sKeySeq;
        *aEntrySeq = sEntrySeq;
        *aKeyCount = aNodeHdr->mKeyCnt;
    }
    else
    {
        *aKeySeq   = SDC_LOB_INVALID_KEY_SEQ;
        *aEntrySeq = SDC_LOB_INVALID_ENTRY_SEQ;
        *aKeyCount = aNodeHdr->mKeyCnt;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPageSeq에 해당하는 Leaf Key를 탐색한다.
 **********************************************************************/
IDE_RC sdcLob::traverse( idvSQL     * aStatistics,
                         smiColumn  * aColumn,
                         scPageID     aRootNodePID,
                         UInt         aPageSeq,
                         scPageID   * aLeafNodePID,
                         SShort     * aLeafKeySeq,
                         SShort     * aLeafKeyEntrySeq,
                         UShort     * aLeafKeyCount )
{
    sdcLobNodeHdr   * sNodeHdr;
    scPageID          sPID;
    sdpPhyPageHdr   * sPage;
    sdcLobIKey        sIKey;
    SShort            sKeySeq;
    SShort            sEntrySeq;
    UShort            sKeyCount;
    UInt              sState = 0;


    IDE_ERROR( aColumn          != NULL );
    IDE_ERROR( aLeafNodePID     != NULL );
    IDE_ERROR( aLeafKeySeq      != NULL );
    IDE_ERROR( aLeafKeyEntrySeq != NULL );
    IDE_ERROR( aLeafKeyCount    != NULL );

    *aLeafNodePID     = SD_NULL_PID;
    *aLeafKeySeq      = SDC_LOB_INVALID_KEY_SEQ;
    *aLeafKeyEntrySeq = SDC_LOB_INVALID_ENTRY_SEQ;
    
    sPID = aRootNodePID;

    IDE_TEST_CONT( sPID == SD_NULL_PID, SKIP_TRAVERSE );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aColumn->colSeg.mSpaceID,
                                          sPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          NULL, /* aMtx */
                                          (UChar**)&sPage,
                                          NULL,
                                          NULL)
              != IDE_SUCCESS );
    sState = 1;

    IDE_ERROR( sPage->mPageType == SDP_PAGE_LOB_INDEX );

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)sPage);

    while( sNodeHdr->mHeight > 0 )
    {
        IDE_TEST( findInternalKey(sNodeHdr,
                                  aPageSeq,
                                  &sIKey)
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics, (UChar*)sPage)
                  != IDE_SUCCESS );

        sPID = sIKey.mChild;
        
        if( sPID == SD_NULL_PID )
        {
            break;
        }

        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aColumn->colSeg.mSpaceID,
                                              sPID,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL, /* aMtx */
                                              (UChar**)&sPage,
                                              NULL,
                                              NULL)
                  != IDE_SUCCESS );
        sState = 1;

        IDE_ERROR( sPage->mPageType == SDP_PAGE_LOB_INDEX );

        sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)sPage);
    }

    /* if the leaf node is found. */
    if( sPID != SD_NULL_PID )
    {
        IDE_TEST( findLeafKey(sNodeHdr,
                              aPageSeq,
                              &sKeySeq,
                              &sEntrySeq,
                              &sKeyCount)
                  != IDE_SUCCESS );

        if( sKeySeq != SDC_LOB_INVALID_KEY_SEQ )
        {
            *aLeafNodePID     = sPID;
            *aLeafKeySeq      = sKeySeq;
            *aLeafKeyEntrySeq = sEntrySeq;
            *aLeafKeyCount    = sKeyCount;
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics, (UChar*)sPage)
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_TRAVERSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, (UChar*)sPage)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPageSeq에 해당하는 Leaf Key를 탐색한다.
 *               Leaf Node의 Link를 따라가면서 Sequential Scan한다.
 **********************************************************************/
IDE_RC sdcLob::traverseSequential( idvSQL     * aStatistics,
                                   smiColumn  * aColumn,
                                   scPageID     aNxtLeafNodePID,
                                   UInt         aPageSeq,
                                   scPageID   * aLeafNodePID,
                                   SShort     * aLeafKeySeq,
                                   SShort     * aLeafKeyEntrySeq,
                                   UShort     * aLeafKeyCount )
{
    sdcLobNodeHdr   * sNodeHdr;
    scPageID          sPID;
    sdpPhyPageHdr   * sPage;
    SShort            sKeySeq;
    SShort            sEntrySeq;
    UShort            sKeyCount;
    UInt              sMaxSearchCnt = 2;
    UInt              sState = 0;
    UInt              i;

    IDE_ERROR( aColumn          != NULL );
    IDE_ERROR( aLeafNodePID     != NULL );
    IDE_ERROR( aLeafKeySeq      != NULL );
    IDE_ERROR( aLeafKeyEntrySeq != NULL );
    IDE_ERROR( aLeafKeyCount    != NULL );

    *aLeafNodePID     = SD_NULL_PID;
    *aLeafKeySeq      = SDC_LOB_INVALID_KEY_SEQ;
    *aLeafKeyEntrySeq = SDC_LOB_INVALID_ENTRY_SEQ;
    
    sPID = aNxtLeafNodePID;

    IDE_TEST_CONT( sPID == SD_NULL_PID, SKIP_TRAVERSE );

    for( i = 0; i < sMaxSearchCnt; i++ )
    {
        if( sPID == SD_NULL_PID )
        {
            break;
        }
        
        IDE_TEST( sdbBufferMgr::getPageByPID(
                      aStatistics,
                      aColumn->colSeg.mSpaceID,
                      sPID,
                      SDB_S_LATCH,
                      SDB_WAIT_NORMAL,
                      SDB_SINGLE_PAGE_READ,
                      NULL, /* aMtx */
                      (UChar**)&sPage,
                      NULL,
                      NULL)
                  != IDE_SUCCESS );
        sState = 1;

        sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)sPage);

        IDE_ERROR( sNodeHdr->mHeight == 0 );
        IDE_ERROR( sPage->mPageType == SDP_PAGE_LOB_INDEX );

        IDE_TEST( findLeafKey(sNodeHdr,
                              aPageSeq,
                              &sKeySeq,
                              &sEntrySeq,
                              &sKeyCount)
                  != IDE_SUCCESS );

        if( sKeySeq != SDC_LOB_INVALID_KEY_SEQ )
        {
            *aLeafNodePID     = sPID;
            *aLeafKeySeq      = sKeySeq;
            *aLeafKeyEntrySeq = sEntrySeq;
            *aLeafKeyCount    = sKeyCount;

            sState = 0;
            IDE_TEST( sdbBufferMgr::releasePage(aStatistics, (UChar*)sPage)
                      != IDE_SUCCESS );
            break;
        }

        sPID = sdpPhyPage::getNxtPIDOfDblList(sPage);

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics, (UChar*)sPage)
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_TRAVERSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, (UChar*)sPage)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Page에 Write 한다.
 **********************************************************************/
IDE_RC sdcLob::writeDirect( idvSQL           * aStatistics,
                            void             * aTrans,
                            smLobViewEnv     * aLobViewEnv,
                            smLobLocator       aLobLocator,
                            sdcLobColBuffer  * aLobColBuf,
                            UInt             * aOffset,
                            UChar           ** aPieceVal,
                            UInt             * aPieceLen,
                            UInt               aAmount,
                            sdcLobWriteType    aWriteType,
                            idBool             aIsPrevVal,
                            idBool             aIsFromAPI,
                            smrContType        aContType )
{
    sdcLobDesc  sOldLobDesc;
    sdcLobDesc  sNewLobDesc;
    scPageID    sPID;
    scPageID    sNewPID;
    UInt        sFrom;
    UInt        sDirectIdx;

    IDE_ERROR( aLobViewEnv  != NULL );
    IDE_ERROR( aLobColBuf   != NULL );
    IDE_ERROR( aOffset      != NULL );
    IDE_ERROR( aPieceVal    != NULL );
    IDE_ERROR( aPieceLen    != NULL );
    
    IDE_ERROR( aLobColBuf->mLength    == ID_SIZEOF(sdcLobDesc) );
    IDE_ERROR( aLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB );

    sFrom = (*aOffset) / SDC_LOB_PAGE_BODY_SIZE;

    IDE_TEST_CONT( sFrom >= SDC_LOB_MAX_DIRECT_PAGE_CNT,
                    SKIP_WRITE_DIRECT );

    idlOS::memcpy( &sOldLobDesc,
                   aLobColBuf->mBuffer,
                   ID_SIZEOF(sdcLobDesc) );
    
    idlOS::memcpy( &sNewLobDesc,
                   aLobColBuf->mBuffer,
                   ID_SIZEOF(sdcLobDesc) );

    for( sDirectIdx = sFrom; sDirectIdx < SDC_LOB_MAX_DIRECT_PAGE_CNT; sDirectIdx++ )
    {
        if( *aPieceLen == 0 )
        {
            break;
        }
        
        sPID = sNewLobDesc.mDirect[sDirectIdx];

        switch(aWriteType)
        {
            case SDC_LOB_WTYPE_WRITE:
                
                IDE_TEST( writePage(aStatistics,
                                    aTrans,
                                    aLobViewEnv->mTable,
                                    &aLobViewEnv->mLobCol,
                                    aLobLocator,
                                    aLobViewEnv->mLobVersion,
                                    aOffset,
                                    aPieceVal,
                                    aPieceLen,
                                    aAmount,
                                    sPID,
                                    &sNewPID,
                                    aIsPrevVal,
                                    aIsFromAPI,
                                    aContType)
                          != IDE_SUCCESS );

                sNewLobDesc.mDirect[sDirectIdx] = sNewPID;

                if( (sDirectIdx + 1) > sNewLobDesc.mDirectCnt )
                {
                    sNewLobDesc.mDirectCnt = sDirectIdx + 1;
                }

                break;
                
            case SDC_LOB_WTYPE_ERASE:
                
                if( sDirectIdx < sNewLobDesc.mDirectCnt )
                {
                    IDE_TEST( erasePage(aStatistics,
                                        aTrans,
                                        &aLobViewEnv->mLobCol,
                                        aLobViewEnv->mLobVersion,
                                        aOffset,
                                        aPieceLen,
                                        sPID,
                                        &sNewPID)
                              != IDE_SUCCESS );

                    sNewLobDesc.mDirect[sDirectIdx] = sNewPID;
                }

                break;
                
            case SDC_LOB_WTYPE_TRIM:

                if( sDirectIdx < sNewLobDesc.mDirectCnt )
                {
                    IDE_TEST( trimPage(aStatistics,
                                       aTrans,
                                       &aLobViewEnv->mLobCol,
                                       aLobViewEnv->mLobVersion,
                                       aOffset,
                                       aPieceLen,
                                       sPID,
                                       &sNewPID)
                              != IDE_SUCCESS );
                
                    sNewLobDesc.mDirect[sDirectIdx] = sNewPID;
                }

                break;
                
            default:
                
                IDE_ERROR( 0 );
                break;
        }
    }

    idlOS::memcpy( aLobColBuf->mBuffer,
                   &sNewLobDesc,
                   ID_SIZEOF(sdcLobDesc) );

    aLobViewEnv->mLastWriteOffset      = *aOffset;
    aLobViewEnv->mLastWriteLeafNodePID = SD_NULL_PID;

    /* add pages to agable list */
    IDE_TEST( removeOldPages(aStatistics,
                             aTrans,
                             &aLobViewEnv->mLobCol,
                             &sOldLobDesc,
                             &sNewLobDesc)
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_WRITE_DIRECT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page Offset와 Page에서 Write할 크기를 반환한다.
 **********************************************************************/
IDE_RC sdcLob::getWritePageOffsetAndSize( UInt     aOffset,
                                          UInt     aPieceLen,
                                          UInt   * aWriteOffset,
                                          UInt   * aWriteSize )
{
    *aWriteOffset = aOffset % SDC_LOB_PAGE_BODY_SIZE;

    if( aPieceLen > (SDC_LOB_PAGE_BODY_SIZE - *aWriteOffset) )
    {
        *aWriteSize = (SDC_LOB_PAGE_BODY_SIZE - *aWriteOffset);
    }
    else
    {
        *aWriteSize = aPieceLen;
    }

    IDE_ERROR( (*aWriteOffset + *aWriteSize) <= SDC_LOB_PAGE_BODY_SIZE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page Offset와 Page에서 Read할 크기를 반환한다.
 **********************************************************************/
IDE_RC sdcLob::getReadPageOffsetAndSize( UChar  * aPage,
                                         UInt     aOffset,
                                         UInt     aAmount,
                                         UInt   * aReadOffset,
                                         UInt   * aReadSize )
{
    sdcLobNodeHdr   * sNodeHdr;
    UShort            sStoreSize;

    if( aPage == NULL )
    {
        sStoreSize = SDC_LOB_PAGE_BODY_SIZE;
    }
    else
    {
        sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aPage);
        sStoreSize = sNodeHdr->mStoreSize;
    }

    IDE_ERROR( sStoreSize <= SDC_LOB_PAGE_BODY_SIZE );

    *aReadOffset = aOffset % SDC_LOB_PAGE_BODY_SIZE;

    if( aPage != NULL )
    {
        IDE_ERROR( *aReadOffset < sNodeHdr->mStoreSize );
    }

    if( aAmount > (sStoreSize - *aReadOffset) )
    {
        *aReadSize = (sStoreSize - *aReadOffset);
    }
    else
    {
        *aReadSize = aAmount;
    }

    IDE_ERROR( (*aReadOffset + *aReadSize) <= SDC_LOB_PAGE_BODY_SIZE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page를 erase한다.
 **********************************************************************/
IDE_RC sdcLob::erasePage( idvSQL      * aStatistics,
                          void        * aTrans,
                          smiColumn   * aColumn,
                          ULong         aLobVersion,
                          UInt        * aOffset,
                          UInt        * aPieceLen,
                          scPageID      aPageID,
                          scPageID    * aNewPageID )
{
    sdrMtx          sMtx;
    sdcLobNodeHdr * sNodeHdr = NULL;
    UChar         * sPage    = NULL;
    UChar         * sNewPage = NULL;
    UInt            sState   = 0;

    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aOffset      != NULL );
    IDE_ERROR( aPieceLen    != NULL );
    IDE_ERROR( aNewPageID   != NULL );

    IDE_TEST( sdrMiniTrans::begin(
                  aStatistics,
                  &sMtx,
                  aTrans,
                  SDR_MTX_LOGGING,
                  ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                  SM_DLOG_ATTR_DEFAULT)
              != IDE_SUCCESS );
    sState = 1;

    if( aPageID == SD_NULL_PID )
    {
        /* nothing to do */
        sPage = NULL;
    }
    else
    {
        IDE_TEST( sdbBufferMgr::getPageByPID(
                      aStatistics,
                      aColumn->colSeg.mSpaceID,
                      aPageID,
                      SDB_X_LATCH,
                      SDB_WAIT_NORMAL,
                      SDB_SINGLE_PAGE_READ,
                      &sMtx, /* aMtx */
                      &sPage,
                      NULL,
                      NULL)
                  != IDE_SUCCESS );

        sNodeHdr = (sdcLobNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr(sPage);

        if( isFullPageWrite(sNodeHdr, aOffset, aPieceLen) == ID_TRUE )
        {
            sPage = NULL;
        }
        else
        {
            if( sNodeHdr->mLobVersion != aLobVersion )
            {
                IDE_TEST( allocNewPage(aStatistics,
                                       &sMtx,
                                       aColumn,
                                       SDP_PAGE_LOB_DATA,
                                       &sNewPage)
                          != IDE_SUCCESS );

                IDE_TEST( initializeNodeHdr(&sMtx,
                                            (sdpPhyPageHdr*)sNewPage,
                                            0, /* aHeight */
                                            0, /* aNodeSeq */
                                            aLobVersion)
                          != IDE_SUCCESS );

                IDE_TEST( copyPage(&sMtx,
                                   sNewPage,
                                   sPage)
                          != IDE_SUCCESS );

                sPage = sNewPage;
            }
        }
    }

    IDE_TEST( eraseLobPiece(&sMtx,
                            aColumn,
                            sPage,
                            aOffset,
                            aPieceLen)
              != IDE_SUCCESS );

    if( sPage == NULL )
    {
        *aNewPageID = SD_NULL_PID;
    }
    else
    {
        *aNewPageID = sdpPhyPage::getPageID(sPage);
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page의 모든 데이터가 Write되는 여부를 반환한다.
 **********************************************************************/
idBool sdcLob::isFullPageWrite( sdcLobNodeHdr   * aNodeHdr,
                                UInt            * aOffset,
                                UInt            * aPieceLen )
{
    idBool  sRet;
    ULong   sWriteOffset;

    sWriteOffset = (*aOffset) % SDC_LOB_PAGE_BODY_SIZE;

    if( sWriteOffset == 0 )
    {
        if( (*aPieceLen) >= aNodeHdr->mStoreSize )
        {
            sRet = ID_TRUE;
        }
        else
        {
            sRet = ID_FALSE;
        }
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

/***********************************************************************
 * Description : Page를 Write한다.
 **********************************************************************/
IDE_RC sdcLob::writePage( idvSQL        * aStatistics,
                          void          * aTrans,
                          void          * aTable,
                          smiColumn     * aColumn,
                          smLobLocator    aLobLocator,
                          ULong           aLobVersion,
                          UInt          * aOffset,
                          UChar        ** aPieceVal,
                          UInt          * aPieceLen,
                          UInt            aAmount,
                          scPageID        aPageID,
                          scPageID      * aNewPageID,
                          idBool          aIsPrevVal,
                          idBool          aIsFromAPI,
                          smrContType     aContType )
{
    sdrMtx          sMtx;
    sdcLobNodeHdr * sNodeHdr;
    UChar         * sPage;
    scPageID        sWritePageID;
    UChar         * sNewPage;
    UInt            sPageState = 0;
    UInt            sMtxState = 0;

    IDE_ERROR( aColumn    != NULL );
    IDE_ERROR( aOffset    != NULL );
    IDE_ERROR( aPieceVal  != NULL );
    IDE_ERROR( aPieceLen  != NULL );
    IDE_ERROR( aNewPageID != NULL );
    
    if( aPageID == SD_NULL_PID )
    {
        IDE_TEST( sdrMiniTrans::begin(
                      aStatistics,
                      &sMtx,
                      aTrans,
                      SDR_MTX_LOGGING,
                      ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                      SM_DLOG_ATTR_DEFAULT)
                  != IDE_SUCCESS );
        sMtxState = 1;

        IDE_TEST( allocNewPage(aStatistics,
                               &sMtx,
                               aColumn,
                               SDP_PAGE_LOB_DATA,
                               &sNewPage)
                  != IDE_SUCCESS );

        IDE_TEST( initializeNodeHdr(&sMtx,
                                    (sdpPhyPageHdr*)sNewPage,
                                    0, /* aHeight */
                                    0, /* aNodeSeq */
                                    aLobVersion)
                  != IDE_SUCCESS );

        sWritePageID = sdpPhyPage::getPageID(sNewPage);

        sMtxState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE)
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdbBufferMgr::getPageByPID(
                      aStatistics,
                      aColumn->colSeg.mSpaceID,
                      aPageID,
                      SDB_S_LATCH,
                      SDB_WAIT_NORMAL,
                      SDB_SINGLE_PAGE_READ,
                      NULL, /* aMtx */
                      &sPage,
                      NULL,
                      NULL)
                  != IDE_SUCCESS );
        sPageState = 1;

        sNodeHdr = (sdcLobNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr(sPage);

        if( sNodeHdr->mLobVersion != aLobVersion )
        {
            IDE_TEST( sdrMiniTrans::begin(
                          aStatistics,
                          &sMtx,
                          aTrans,
                          SDR_MTX_LOGGING,
                          ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                          SM_DLOG_ATTR_DEFAULT)
                      != IDE_SUCCESS );
            sMtxState = 1;

            IDE_TEST( allocNewPage(aStatistics,
                                   &sMtx,
                                   aColumn,
                                   SDP_PAGE_LOB_DATA,
                                   &sNewPage)
                      != IDE_SUCCESS );

            IDE_TEST( initializeNodeHdr(&sMtx,
                                        (sdpPhyPageHdr*)sNewPage,
                                        0, /* aHeight */
                                        0, /* aNodeSeq */
                                        aLobVersion)
                      != IDE_SUCCESS );

            if( isFullPageWrite(sNodeHdr, aOffset, aPieceLen) != ID_TRUE )
            {
                IDE_TEST( copyPage(&sMtx, sNewPage, sPage)
                          != IDE_SUCCESS );
            }

            sWritePageID = sdpPhyPage::getPageID(sNewPage);

            sMtxState = 0;
            IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE)
                      != IDE_SUCCESS );
        }
        else
        {
            sWritePageID = sdpPhyPage::getPageID(sPage);
        }

        sPageState = 0;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics,
                                            sPage)
                  != IDE_SUCCESS );
    }

    IDE_TEST( writeLobPiece(aStatistics,
                            aTrans,
                            aTable,
                            aColumn,
                            aLobLocator,
                            sWritePageID,
                            aOffset,
                            aPieceVal,
                            aPieceLen,
                            aAmount,
                            aIsPrevVal,
                            aIsFromAPI,
                            aContType)
              != IDE_SUCCESS );

    *aNewPageID = sWritePageID;
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sPageState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics,
                                              sPage)
                    == IDE_SUCCESS );
    }

    if( sMtxState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Old Version의 Direct Page를 AgableList에 연결한다.
 **********************************************************************/
IDE_RC sdcLob::removeOldPages( idvSQL       * aStatistics,
                               void         * aTrans,
                               smiColumn    * aColumn,
                               sdcLobDesc   * aOldLobDesc,
                               sdcLobDesc   * aNewLobDesc )
{
    UInt i;

    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aOldLobDesc  != NULL );
    IDE_ERROR( aNewLobDesc  != NULL );

    for( i = 0; i < aOldLobDesc->mDirectCnt; i++ )
    {
        if( aOldLobDesc->mDirect[i] == SD_NULL_PID )
        {
            continue;
        }

        if( aOldLobDesc->mDirect[i] == aNewLobDesc->mDirect[i] )
        {
            continue;
        }

        IDE_TEST( addPage2AgingList(aStatistics,
                                    aTrans,
                                    aColumn,
                                    aOldLobDesc->mDirect[i])
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Old Version의 Entry Page를 AgableList에 연결한다.
 **********************************************************************/
IDE_RC sdcLob::removeOldPages( idvSQL       * aStatistics,
                               void         * aTrans,
                               smiColumn    * aColumn,
                               sdcLobLKey   * aOldLKey,
                               sdcLobLKey   * aNewLKey )
{
    UInt i;

    for( i = 0; i < aOldLKey->mEntryCnt; i++ )
    {
        if( aOldLKey->mEntry[i] == SD_NULL_PID )
        {
            continue;
        }

        if( aOldLKey->mEntry[i] == aNewLKey->mEntry[i] )
        {
            continue;
        }

        IDE_TEST( addPage2AgingList(aStatistics,
                                    aTrans,
                                    aColumn,
                                    aOldLKey->mEntry[i])
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page를 AgableList에 연결한다.
 **********************************************************************/
IDE_RC sdcLob::addPage2AgingList( idvSQL      * aStatistics,
                                   void        * aTrans,
                                   smiColumn   * aColumn,
                                   scPageID      aOldPageID )
{
    sdrMtx                sMtx;
    smLSN                 sNTA;
    sdpSegMgmtOp        * sSegMgmtOp;
    scPageID              sSegPID;
    scPageID              sMetaPID;
    scGRID                sMetaGRID;
    sdcLobMeta          * sMeta;
    sdcLobNodeHdr       * sNodeHdr;
    sdpPhyPageHdr       * sOldPage;
    UChar               * sMetaPage;
    UInt                  sState = 0;

    IDE_ERROR( aColumn    != NULL );
    IDE_ERROR( aOldPageID != SD_NULL_PID );

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( aColumn );
    sSegPID    = sdpSegDescMgr::getSegPID( aColumn );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    sNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

    IDE_TEST( sdrMiniTrans::begin(aStatistics,
                                  &sMtx,
                                  aTrans,
                                  SDR_MTX_LOGGING,
                                  ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                                  SM_DLOG_ATTR_DEFAULT)
              != IDE_SUCCESS );
    sState = 1;

    /*
     * get Meta page (X-Latch)
     */
    
    IDE_TEST( sSegMgmtOp->mGetMetaPID(aStatistics,
                                      aColumn->colSpace,
                                      sSegPID,
                                      0, /* Seg Meta PID Array Index */
                                      &sMetaPID)
              != IDE_SUCCESS );

    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aColumn->colSpace,
                                         sMetaPID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         &sMtx,
                                         (UChar**)&sMetaPage,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS);

    sMeta = (sdcLobMeta*)sdpPhyPage::getLogicalHdrStartPtr(sMetaPage);

    /*
     * get old page
     */

    IDE_ERROR( aOldPageID != sMetaPID );
    
    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aColumn->colSpace,
                                         aOldPageID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         &sMtx,
                                         (UChar**)&sOldPage,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS);

    sNodeHdr = (sdcLobNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)sOldPage);

    sNodeHdr->mTSSlotSID     = smxTrans::getTSSlotSID(aTrans);
    sNodeHdr->mFstDskViewSCN = smxTrans::getFstDskViewSCN(aTrans);

    IDE_ERROR( sNodeHdr->mLobPageState != SDC_LOB_AGING_LIST );
    sNodeHdr->mLobPageState = SDC_LOB_AGING_LIST;

    IDE_TEST( sdpDblPIDList::insertTailNode(
                  aStatistics,
                  &(sMeta->mAgingListBase),
                  &(sOldPage->mListNode),
                  &sMtx)
              != IDE_SUCCESS );

    SC_MAKE_GRID( sMetaGRID,
                  aColumn->colSpace,
                  sMetaPID,
                  0 );

    /*
     * write log (NTA)
     */
    
    sdrMiniTrans::setRefOffset( &sMtx, SM_OID_NULL );

    IDE_TEST( sdrMiniTrans::writeLogRec( &sMtx,
                                         (UChar*)sOldPage,
                                         (void*)NULL,
                                         ID_SIZEOF(sdSID) +
                                         ID_SIZEOF(smSCN) +
                                         ID_SIZEOF(scGRID),
                                         SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (UChar*)&sNodeHdr->mTSSlotSID,
                                   ID_SIZEOF(sNodeHdr->mTSSlotSID) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (UChar*)&sNodeHdr->mFstDskViewSCN,
                                   ID_SIZEOF(sNodeHdr->mFstDskViewSCN) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   &sMetaGRID,
                                   ID_SIZEOF(scGRID) )
              != IDE_SUCCESS );

    sdrMiniTrans::setRefNTA( &sMtx,
                             aColumn->colSpace,
                             SDR_OP_SDC_LOB_ADD_PAGE_TO_AGINGLIST,
                             &sNTA );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page를 Copy한다.
 **********************************************************************/
IDE_RC sdcLob::copyPage( sdrMtx * aMtx,
                         UChar  * aDstPage,
                         UChar  * aSrcPage )
{
    sdcLobNodeHdr   * sDstNodeHdr;
    sdcLobNodeHdr   * sSrcNodeHdr;
    UChar           * sDstLobDataLayerStartPtr;
    UChar           * sSrcLobDataLayerStartPtr;

    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR( aDstPage != NULL );
    IDE_ERROR( aSrcPage != NULL );

    sSrcNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aSrcPage);
    sDstNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aDstPage);

    sDstLobDataLayerStartPtr = getLobDataLayerStartPtr(aDstPage);
    sSrcLobDataLayerStartPtr = getLobDataLayerStartPtr(aSrcPage);

    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  sDstLobDataLayerStartPtr,
                  sSrcLobDataLayerStartPtr,
                  sSrcNodeHdr->mStoreSize,
                  SDR_SDP_BINARY)
              != IDE_SUCCESS );

    idlOS::memcpy( sDstLobDataLayerStartPtr,
                   sSrcLobDataLayerStartPtr,
                   sSrcNodeHdr->mStoreSize );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sDstNodeHdr->mStoreSize,
                  (void*)&sSrcNodeHdr->mStoreSize,
                  ID_SIZEOF(sSrcNodeHdr->mStoreSize) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page를 Copy & Trim한다.
 **********************************************************************/
IDE_RC sdcLob::copyAndTrimPage( sdrMtx * aMtx,
                                UChar  * aDstPage,
                                UChar  * aSrcPage,
                                UInt     aOffset )
{
    sdcLobNodeHdr   * sDstNodeHdr;
    sdcLobNodeHdr   * sSrcNodeHdr;
    UChar           * sDstLobDataLayerStartPtr;
    UChar           * sSrcLobDataLayerStartPtr;
    UShort            sStoreSize;

    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR( aDstPage != NULL );
    IDE_ERROR( aSrcPage != NULL );

    IDE_ERROR( aOffset < SDC_LOB_PAGE_BODY_SIZE );

    sSrcNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aSrcPage);
    sDstNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aDstPage);

    sDstLobDataLayerStartPtr = getLobDataLayerStartPtr(aDstPage);
    sSrcLobDataLayerStartPtr = getLobDataLayerStartPtr(aSrcPage);

    IDE_ERROR( aOffset < sSrcNodeHdr->mStoreSize );
    sStoreSize = aOffset;

    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  sDstLobDataLayerStartPtr,
                  sSrcLobDataLayerStartPtr,
                  sStoreSize,
                  SDR_SDP_BINARY)
              != IDE_SUCCESS );

    idlOS::memcpy( sDstLobDataLayerStartPtr,
                   sSrcLobDataLayerStartPtr,
                   sStoreSize );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sDstNodeHdr->mStoreSize,
                  (void*)&sStoreSize,
                  ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page에 대한 erase를 수행한다.
 **********************************************************************/
IDE_RC sdcLob::eraseLobPiece( sdrMtx    * aMtx,
                              smiColumn * aColumn,
                              UChar     * aPage,
                              UInt      * aOffset,
                              UInt      * aPieceLen )
{
    ULong   sTmpBuf[SD_PAGE_SIZE];
    UInt    sWriteOffset;
    UInt    sWriteSize;

    IDE_ERROR( aMtx      != NULL );
    IDE_ERROR( aColumn   != NULL );
    IDE_ERROR( aPage     != NULL );
    IDE_ERROR( aOffset   != NULL );
    IDE_ERROR( aPieceLen != NULL );

    IDE_TEST( getWritePageOffsetAndSize(*aOffset,
                                        *aPieceLen,
                                        &sWriteOffset,
                                        &sWriteSize)
              != IDE_SUCCESS );

    if( aColumn->colType == SMI_LOB_COLUMN_TYPE_BLOB )
    {
        idlOS::memset( sTmpBuf, 0x00, ID_SIZEOF(sTmpBuf) );
    }
    else
    {
        idlOS::memset( sTmpBuf, ' ', ID_SIZEOF(sTmpBuf) );
    }
        
    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  getLobDataLayerStartPtr(aPage) + sWriteOffset,
                  sTmpBuf,
                  sWriteSize,
                  SDR_SDP_BINARY)
              != IDE_SUCCESS );

    /*
     * update piece infomation
     */
    
    *aPieceLen -= sWriteSize;
    *aOffset   += sWriteSize;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page에 대한 write를 수행한다.
 **********************************************************************/
IDE_RC sdcLob::writeLobPiece( idvSQL        * aStatistics,
                              void          * aTrans,
                              void          * aTable,
                              smiColumn     * aColumn,
                              smLobLocator    aLobLocator,
                              scPageID        aPageID,
                              UInt          * aOffset,
                              UChar        ** aPieceVal,
                              UInt          * aPieceLen,
                              UInt            aAmount,
                              idBool          aIsPrevVal,
                              idBool          aIsFromAPI,
                              smrContType     aContType )
{
    sdrMtx            sMtx;
    UChar           * sPage;
    sdcLobNodeHdr   * sNodeHdr;
    UInt              sWriteOffset;
    UInt              sWriteSize;
    UChar           * sWriteValue;
    UChar           * sLobDataLayerStartPtr;
    smrContType       sLobContType;
    UInt              sState = 0;

    IDE_ERROR( aOffset   != NULL );
    IDE_ERROR( aPieceVal != NULL );
    IDE_ERROR( aPieceLen != NULL );

    IDE_TEST( sdrMiniTrans::begin(
                  aStatistics,
                  &sMtx,
                  aTrans,
                  SDR_MTX_LOGGING,
                  ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                  SM_DLOG_ATTR_DEFAULT)
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aColumn->colSpace,
                                         aPageID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         &sMtx,
                                         &sPage,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS );

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sPage);
    sLobDataLayerStartPtr = getLobDataLayerStartPtr(sPage);

    /*
     * write data
     */

    sWriteValue  = *aPieceVal;

    IDE_TEST( getWritePageOffsetAndSize( *aOffset,
                                         *aPieceLen,
                                         &sWriteOffset,
                                         &sWriteSize )
              != IDE_SUCCESS );

    if( sWriteSize > 0 )
    {
        idlOS::memcpy( sLobDataLayerStartPtr + sWriteOffset,
                       sWriteValue,
                       sWriteSize );

        if( (sWriteOffset + sWriteSize) > sNodeHdr->mStoreSize )
        {
            sNodeHdr->mStoreSize = (sWriteOffset + sWriteSize);
        }
    }

    /*
     * write log
     */

    if( aIsFromAPI == ID_TRUE )
    {
        IDE_TEST( writeLobWritePieceRedoLog(&sMtx,
                                            aTable,
                                            sLobDataLayerStartPtr,
                                            aIsPrevVal,
                                            *aOffset,
                                            aAmount,
                                            sWriteValue,
                                            sWriteSize,
                                            sWriteOffset,
                                            aLobLocator)
                  != IDE_SUCCESS );

        sLobContType = SMR_CT_END;
    }
    else
    {
        IDE_ERROR( aIsPrevVal == ID_FALSE );

        IDE_TEST( writeLobWritePiece4DMLRedoLog(&sMtx,
                                                aTable,
                                                aColumn,
                                                sLobDataLayerStartPtr,
                                                aAmount,
                                                sWriteValue,
                                                sWriteSize,
                                                sWriteOffset)
                  != IDE_SUCCESS );

        /*
         * Condition for sLobContType == SMR_CT_END.
         * 
         * 1. called by SQL (aIsFromAPI == ID_FALSE)
         * 2. whole value is written. ((*aPieceLen - sWriteSize) == 0)
         * 3. last LOB column (aContType == SMR_CT_END)
         */
        
        if( ((*aPieceLen - sWriteSize) == 0) &&
            (aContType  == SMR_CT_END) )
        {
            sLobContType = SMR_CT_END;
        }
        else
        {
            sLobContType = SMR_CT_CONTINUE;
        }
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx, sLobContType)
              != IDE_SUCCESS );

    /*
     * update piece information
     */
    
    *aPieceVal += sWriteSize;
    *aPieceLen -= sWriteSize;
    *aOffset   += sWriteSize;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page를 할당한다.
 **********************************************************************/
IDE_RC sdcLob::allocNewPage( idvSQL         * aStatistics,
                             sdrMtx         * aMtx,
                             smiColumn      * aColumn,
                             sdpPageType      aPageType,
                             UChar         ** aPagePtr )
{
    sdpSegMgmtOp  * sSegMgmtOp;
    sdcLobNodeHdr * sNodeHdr;

    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR( aColumn  != NULL );
    IDE_ERROR( aPagePtr != NULL );

    *aPagePtr  = NULL;
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( aColumn->colSeg.mSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST( sSegMgmtOp->mAllocNewPage(
                  aStatistics,
                  aMtx,
                  aColumn->colSeg.mSpaceID,
                  sdpSegDescMgr::getSegHandle(aColumn),
                  aPageType,
                  aPagePtr)
              != IDE_SUCCESS );

    IDE_TEST( sdpPhyPage::logAndInitLogicalHdr(
                  (sdpPhyPageHdr*)*aPagePtr,
                  ID_SIZEOF(sdcLobNodeHdr),
                  aMtx,
                  (UChar**)&sNodeHdr)
              != IDE_SUCCESS );

    IDE_TEST( sdpSlotDirectory::logAndInit(
                  (sdpPhyPageHdr*)*aPagePtr,
                  aMtx)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Logical Node를 초기화한다.
 **********************************************************************/
IDE_RC sdcLob::initializeNodeHdr( sdrMtx        * aMtx,
                                  sdpPhyPageHdr * aPage,
                                  UShort          aHeight,
                                  UInt            aNodeSeq,
                                  ULong           aLobVersion )
{
    sdcLobNodeHdr   * sNodeHdr;
    UShort            sKeyCnt = 0;
    UShort            sStoreSize = 0;
    UInt              sLobPageState = SDC_LOB_USED;
    sdSID             sTSSlotSID = SD_NULL_SID;
    smSCN             sFstDskViewSCN;

    IDE_ERROR( aMtx  != NULL );
    IDE_ERROR( aPage != NULL );

    SM_SET_SCN_INFINITE( &sFstDskViewSCN );

    sNodeHdr = (sdcLobNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)aPage);

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&sNodeHdr->mHeight,
                                        (void*)&aHeight,
                                        ID_SIZEOF(aHeight))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&sNodeHdr->mKeyCnt,
                                        (void*)&sKeyCnt,
                                        ID_SIZEOF(sKeyCnt))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&sNodeHdr->mStoreSize,
                                        (void*)&sStoreSize,
                                        ID_SIZEOF(sStoreSize))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&sNodeHdr->mNodeSeq,
                                        (void*)&aNodeSeq,
                                        ID_SIZEOF(aNodeSeq))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&sNodeHdr->mLobPageState,
                                        (void*)&sLobPageState,
                                        ID_SIZEOF(sLobPageState))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&sNodeHdr->mLobVersion,
                                        (void*)&aLobVersion,
                                        ID_SIZEOF(aLobVersion))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&sNodeHdr->mTSSlotSID,
                                        (void*)&sTSSlotSID,
                                        ID_SIZEOF(sTSSlotSID))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&sNodeHdr->mFstDskViewSCN,
                                        (void*)&sFstDskViewSCN,
                                        ID_SIZEOF(sFstDskViewSCN))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aNeedPageCnt 만큼의 Page를 Aging 시도한다.
 **********************************************************************/
IDE_RC sdcLob::agingPages( idvSQL    * aStatistics,
                           void      * aTrans,
                           smiColumn * aColumn,
                           UInt        aNeedPageCnt )
{
    sdrMtx            sMtx;
    sdpSegCCache    * sSegCache;
    sdcLobMeta      * sMeta;
    scPageID          sMetaPID;
    scPageID          sFreePageID;
    UChar           * sMetaPage = NULL;
    UInt              sFreeNodeCnt = 0;
    UInt              sTotalFreeNodeCnt = 0;
    UInt              sState = 0;

    IDE_ERROR( aColumn != NULL );

    sSegCache = (sdpSegCCache*)(((sdpSegmentDesc*)aColumn->descSeg)->mSegHandle.mCache);
    sMetaPID  = sSegCache->mMetaPID;

    while( 1 )
    {
        sTotalFreeNodeCnt += sFreeNodeCnt;

        if( sTotalFreeNodeCnt >= aNeedPageCnt )
        {
            break;
        }

        IDE_TEST( sdrMiniTrans::begin(
                      aStatistics,
                      &sMtx,
                      aTrans,
                      SDR_MTX_LOGGING,
                      ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                      SM_DLOG_ATTR_DEFAULT)
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByPID(
                      aStatistics,
                      aColumn->colSeg.mSpaceID,
                      sMetaPID,
                      SDB_X_LATCH,
                      SDB_WAIT_NORMAL,
                      SDB_SINGLE_PAGE_READ,
                      &sMtx,
                      &sMetaPage,
                      NULL, /* aTrySuccess */
                      NULL /* aIsCommitJob */)
                  != IDE_SUCCESS );

        sMeta = (sdcLobMeta*)sdpPhyPage::getLogicalHdrStartPtr(sMetaPage);
        if( sdpDblPIDList::getNodeCnt(&sMeta->mAgingListBase) <= 0 )
        {
            sState = 0;
            IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE)
                      != IDE_SUCCESS );
            break;
        }

        sFreePageID = sdpDblPIDList::getListHeadNode( &sMeta->mAgingListBase );

        IDE_ERROR( sFreePageID != sMetaPID );
        
        IDE_TEST( agingNode(aStatistics,
                            &sMtx,
                            aColumn,
                            sMeta,
                            sFreePageID,
                            &sFreeNodeCnt)
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE)
                  != IDE_SUCCESS );
        
        if( sFreeNodeCnt == 0 )
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }
        
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page를 Aging 한다.
 **********************************************************************/
IDE_RC sdcLob::agingNode( idvSQL     * aStatistics,
                          sdrMtx     * aMtx,
                          smiColumn  * aColumn, 
                          sdcLobMeta * aMeta,
                          scPageID     aFreePageID,
                          UInt       * aFreeNodeCnt )
{
    idBool  sIsFreeable;
    UShort  sPageType;

    IDE_ERROR( aMtx         != NULL );
    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aMtx         != NULL );
    IDE_ERROR( aFreeNodeCnt != NULL );
    
    *aFreeNodeCnt = 0;

    IDE_TEST( getNodeInfo(aStatistics,
                          aColumn,
                          aFreePageID,
                          &sIsFreeable,
                          &sPageType)
              != IDE_SUCCESS );

    IDE_TEST_CONT( sIsFreeable != ID_TRUE, SKIP_FREE_NODE );

    if( sPageType == SDP_PAGE_LOB_DATA )
    {
        IDE_TEST( agingDataNode(aStatistics,
                                aMtx,
                                aColumn,
                                aMeta,
                                aFreePageID,
                                aFreeNodeCnt)
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( agingIndexNode(aStatistics,
                                 aMtx,
                                 aColumn,
                                 aMeta,
                                 aFreePageID,
                                 aFreeNodeCnt)
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_FREE_NODE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Data Page를 Aging 한다.
 **********************************************************************/
IDE_RC sdcLob::agingDataNode( idvSQL     * aStatistics,
                              sdrMtx     * aMtx,
                              smiColumn  * aColumn,
                              sdcLobMeta * aMeta,
                              scPageID     aFreePageID,
                              UInt       * aFreeNodeCnt )
{
    sdpPhyPageHdr   * sFreePage = NULL;

    IDE_ERROR( aMtx         != NULL );
    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aMeta        != NULL );
    IDE_ERROR( aFreeNodeCnt != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID(
                  aStatistics,
                  aColumn->colSeg.mSpaceID,
                  aFreePageID,
                  SDB_X_LATCH,
                  SDB_WAIT_NORMAL,
                  SDB_SINGLE_PAGE_READ,
                  aMtx, /* aMtx */
                  (UChar**)&sFreePage,
                  NULL,
                  NULL )
              != IDE_SUCCESS );

    IDE_ERROR( sFreePage->mPageType == SDP_PAGE_LOB_DATA );

    IDE_TEST( sdpDblPIDList::removeNode( aStatistics,
                                         &aMeta->mAgingListBase,
                                         &sFreePage->mListNode,
                                         aMtx)
              != IDE_SUCCESS );

    IDE_TEST( sdpSegDescMgr::getSegMgmtOp(aColumn)->mFreePage(
                  aStatistics,
                  aMtx,
                  aColumn->colSeg.mSpaceID,
                  sdpSegDescMgr::getSegHandle(aColumn),
                  (UChar*)sFreePage)
              != IDE_SUCCESS );

    (*aFreeNodeCnt) += 1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Index Page를 Aging 한다.
 **********************************************************************/
IDE_RC sdcLob::agingIndexNode( idvSQL     * aStatistics,
                               sdrMtx     * aMtx,
                               smiColumn  * aColumn,
                               sdcLobMeta * aMeta,
                               scPageID     aFreePageID,
                               UInt       * aFreeNodeCnt )
{
    sdcLobStack sStack;
    scPageID    sRootNodePID;

    IDE_ERROR( aMtx         != NULL );
    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aMeta        != NULL );
    IDE_ERROR( aFreePageID  != SD_NULL_PID );
    IDE_ERROR( aFreeNodeCnt != NULL );

    sRootNodePID = aFreePageID;

    initStack( &sStack );

    IDE_TEST( findRightMostLeafNode(aStatistics,
                                    aColumn->colSeg.mSpaceID,
                                    sRootNodePID,
                                    &sStack)
              != IDE_SUCCESS );

    IDE_TEST( deleteLeafKey(aStatistics,
                            aMtx,
                            aColumn,
                            aMeta,
                            &sStack,
                            sRootNodePID,
                            aFreeNodeCnt)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Key를 삭제한다.
 **********************************************************************/
IDE_RC sdcLob::deleteLeafKey( idvSQL        * aStatistics,
                              sdrMtx        * aMtx,
                              smiColumn     * aColumn,
                              sdcLobMeta    * aMeta,
                              sdcLobStack   * aStack,
                              scPageID        aRootNodePID,
                              UInt          * aFreeNodeCnt )
{
    sdrMtxStartInfo   sStartInfo;
    sdpPhyPageHdr   * sLeafNode;
    sdcLobNodeHdr   * sLeafNodeHdr;
    sdcLobLKey      * sLKeyArray;
    sdcLobLKey      * sLKey;
    scPageID          sLeafNodePID;
    UChar           * sDataNode;
    SShort            sKeySeq;
    UInt              sState = 0;
    UInt              i;
    sdrMtx            sMtx;

    IDE_ERROR( aMtx         != NULL );
    IDE_ERROR( aColumn      != NULL );
    IDE_ERROR( aMeta        != NULL );
    IDE_ERROR( aStack       != NULL );
    IDE_ERROR( aFreeNodeCnt != NULL );
    IDE_ERROR( aRootNodePID != SD_NULL_PID );

    IDE_ERROR( getPosStack(aStack) >= 0 );
    
    IDE_TEST( popStack(aStack, &sLeafNodePID) != IDE_SUCCESS );

    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aColumn->colSpace,
                                         sLeafNodePID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         aMtx,
                                         (UChar**)&sLeafNode,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS );

    sLeafNodeHdr = (sdcLobNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)sLeafNode);

    IDE_ERROR( sLeafNode->mPageType == SDP_PAGE_LOB_INDEX );
    
    IDE_ERROR( sLeafNodeHdr->mHeight == 0 );
    IDE_ERROR( sLeafNodeHdr->mKeyCnt > 0 );

    /*
     * free key
     */

    sKeySeq    = sLeafNodeHdr->mKeyCnt - 1;
    sLKeyArray = (sdcLobLKey*)getLobDataLayerStartPtr((UChar*)sLeafNode);
    sLKey      = &sLKeyArray[sKeySeq];

    sdrMiniTrans::makeStartInfo( aMtx,
                                 &sStartInfo );
        
    for( i = 0; i < sLKey->mEntryCnt; i++ )
    {
        if( sLKey->mEntry[i] == SD_NULL_PID )
        {
            continue;
        }
        /* BUG-47735 lob aging 에서 deadlock 발생
         * 여러 Lob Page를 Free하는 과정 중에 LOB Segment Root page에 대한 latch를 풀지 않아서
         * alloc 받는 transaction과 segment page 간의 latch dead lock 발생
         * 매번 mtx commit 해 주도록 수정합니다. */
        IDE_TEST( sdrMiniTrans::begin(
                      aStatistics,
                      &sMtx,
                      &sStartInfo,
                      ID_TRUE, /*MtxUndoable(PROJ-2162)*/
                      SM_DLOG_ATTR_DEFAULT)
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                             aColumn->colSpace,
                                             sLKey->mEntry[i],
                                             SDB_X_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_SINGLE_PAGE_READ,
                                             &sMtx,
                                             &sDataNode,
                                             NULL,
                                             NULL)
                  != IDE_SUCCESS );

        IDE_TEST( sdpSegDescMgr::getSegMgmtOp(aColumn)->mFreePage(
                      aStatistics,
                      &sMtx,
                      aColumn->colSeg.mSpaceID,
                      sdpSegDescMgr::getSegHandle(aColumn),
                      sDataNode)
                  != IDE_SUCCESS );

        sLKey->mEntry[i] = SD_NULL_PID;

        /* BUG-47735 */
        IDE_TEST( sdrMiniTrans::writeNBytes(
                      &sMtx,
                      (UChar*)&sLKey->mEntry[i],
                      (void*)&sLKey->mEntry[i],
                      ID_SIZEOF( scPageID ) )
                  != IDE_SUCCESS );

        /* BUG-47735 */
        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx,
                                        SMR_CT_CONTINUE ) != IDE_SUCCESS );

        (*aFreeNodeCnt) += 1;
    }
    
    sLeafNodeHdr->mKeyCnt--;
    
    /*
     * wrie free key log
     */
    
    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)sLKey,
                  (void*)&sKeySeq,
                  ID_SIZEOF(sKeySeq),
                  SDR_SDC_LOB_FREE_LEAF_KEY)
              != IDE_SUCCESS );

    /*
     * remove leaf node
     */

    if( sLeafNodeHdr->mKeyCnt == 0 )
    {
        /*
         * If the parent node is existed, the internal key should be
         * deleted from parent node.
         */
        IDE_TEST( deleteInternalKey(aStatistics,
                                    aMtx,
                                    aColumn,
                                    aMeta,
                                    aStack,
                                    aRootNodePID,
                                    sLeafNodePID,
                                    aFreeNodeCnt)
                  != IDE_SUCCESS );
                                    
        if( sdpPhyPage::getPageID((UChar*)sLeafNode) == aRootNodePID )
        {
            IDE_TEST( sdpDblPIDList::removeNode(
                          aStatistics,
                          &aMeta->mAgingListBase,
                          &sLeafNode->mListNode,
                          aMtx)
                      != IDE_SUCCESS );
        }

        IDE_TEST( sdpSegDescMgr::getSegMgmtOp(aColumn)->mFreePage(
                      aStatistics,
                      aMtx,
                      aColumn->colSeg.mSpaceID,
                      sdpSegDescMgr::getSegHandle(aColumn),
                      (UChar*)sLeafNode)
                  != IDE_SUCCESS );

        (*aFreeNodeCnt) += 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        /* BUG-47735 */
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Internal Key를 삭제한다.
 **********************************************************************/
IDE_RC sdcLob::deleteInternalKey( idvSQL        * aStatistics,
                                  sdrMtx        * aMtx,
                                  smiColumn     * aColumn,
                                  sdcLobMeta    * aMeta,
                                  sdcLobStack   * aStack,
                                  scPageID        aRootNodePID,
                                  scPageID        aChildNodePID,
                                  UInt          * aFreeNodeCnt )
{
    sdpPhyPageHdr   * sNode;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobIKey      * sIKeyArray;
    sdcLobIKey      * sIKey;
    scPageID          sInternalNodePID;
    SShort            sKeySeq;

    IDE_ERROR( aMtx          != NULL );
    IDE_ERROR( aColumn       != NULL );
    IDE_ERROR( aMeta         != NULL );
    IDE_ERROR( aStack        != NULL );
    IDE_ERROR( aRootNodePID  != SD_NULL_PID );
    IDE_ERROR( aChildNodePID != SD_NULL_PID );
    IDE_ERROR( aFreeNodeCnt  != NULL );

    IDE_TEST_CONT( getPosStack(aStack) < 0, SKIP_DELETE_IKEY );

    IDE_TEST( popStack(aStack, &sInternalNodePID) != IDE_SUCCESS );
    
    IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                         aColumn->colSpace,
                                         sInternalNodePID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         aMtx,
                                         (UChar**)&sNode,
                                         NULL,
                                         NULL)
              != IDE_SUCCESS );

    sNodeHdr = (sdcLobNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)sNode);

    IDE_ERROR( sNode->mPageType == SDP_PAGE_LOB_INDEX );
    IDE_ERROR( sNodeHdr->mHeight > 0 );
    IDE_ERROR( sNodeHdr->mKeyCnt > 0 );

    /*
     * free key
     */

    sKeySeq    = sNodeHdr->mKeyCnt - 1;
    sIKeyArray = (sdcLobIKey*)getLobDataLayerStartPtr((UChar*)sNode);
    sIKey      = &sIKeyArray[sKeySeq];

    IDE_ERROR( sIKey->mChild == aChildNodePID );
    
    sIKey->mChild = SD_NULL_PID;
    sNodeHdr->mKeyCnt--;
    
    /*
     * write free key log
     */
    
    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)sIKey,
                  (void*)&sKeySeq,
                  ID_SIZEOF(sKeySeq),
                  SDR_SDC_LOB_FREE_INTERNAL_KEY)
              != IDE_SUCCESS );

    /*
     * remove internal node
     */

    if( sNodeHdr->mKeyCnt == 0 )
    {
        /*
         * If the parent node is existed, the internal key should be
         * deleted from parent node.
         */
        IDE_TEST( deleteInternalKey(aStatistics,
                                    aMtx,
                                    aColumn,
                                    aMeta,
                                    aStack,
                                    aRootNodePID,
                                    sInternalNodePID,
                                    aFreeNodeCnt)
                  != IDE_SUCCESS );

        if( sdpPhyPage::getPageID((UChar*)sNode) == aRootNodePID )
        {
            IDE_TEST( sdpDblPIDList::removeNode(
                          aStatistics,
                          &aMeta->mAgingListBase,
                          &sNode->mListNode,
                          aMtx)
                      != IDE_SUCCESS );
        }

        IDE_TEST( sdpSegDescMgr::getSegMgmtOp(aColumn)->mFreePage(
                      aStatistics,
                      aMtx,
                      aColumn->colSeg.mSpaceID,
                      sdpSegDescMgr::getSegHandle(aColumn),
                      (UChar*)sNode)
                  != IDE_SUCCESS );

        (*aFreeNodeCnt) += 1;
    }

    IDE_EXCEPTION_CONT( SKIP_DELETE_IKEY );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page Type 과 Aging 가능 여부를 반환한다.
 **********************************************************************/
IDE_RC sdcLob::getNodeInfo( idvSQL      * aStatistics,
                            smiColumn   * aColumn,
                            scPageID      aPageID,
                            idBool      * aIsFreeable,
                            UShort      * aPageType )
{
    sdcLobNodeHdr   * sNodeHdr;
    smSCN             sCommitSCN;
    smSCN             sSysMinDskViewSCN;
    UChar           * sPage;
    UInt              sState = 0;

    IDE_ERROR( aColumn     != NULL );
    IDE_ERROR( aIsFreeable != NULL );
    IDE_ERROR( aPageType   != NULL );

    *aIsFreeable = ID_FALSE;
    SM_INIT_SCN( &sCommitSCN );
    
    /*
     * to check if this node is freeable.
     */
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aColumn->colSeg.mSpaceID,
                                          aPageID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          NULL, /* aMtx */
                                          &sPage,
                                          NULL,
                                          NULL )
              != IDE_SUCCESS );
    sState = 1;

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sPage);

    SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

    if( SM_SCN_IS_LT(&sNodeHdr->mFstDskViewSCN, &sSysMinDskViewSCN) == ID_TRUE )
    {
        IDE_TEST( getCommitSCN( aStatistics,
                                sNodeHdr->mTSSlotSID,
                                &sNodeHdr->mFstDskViewSCN,
                                &sCommitSCN )
                  != IDE_SUCCESS );

        /* BUG-25702
         * statement 단위로 miminum disk viewscn을 관리하게 되면서
         * SysMinDskViewSCN과 sCommitSCN의 비교만으로는
         * 트랜잭션이 commit되었는지를 판단할 수 없게 되었다.
         * 아직 트랜잭션이 commit하지 않았더라도 sSysMinDskViewSCN이
         * sCommitSCN보다 큰 경우가 있을 수 있다.
         * 그래서 트랜잭션이 commit 되었는지 여부를 판단하기 위해서
         * SM_SCN_IS_VIEWSCN() 조건검사를 추가한다. */
        if( (SM_SCN_IS_VIEWSCN(sCommitSCN) != ID_TRUE) &&
            (SM_SCN_IS_LT(&sCommitSCN, &sSysMinDskViewSCN) == ID_TRUE) )
        {
            *aIsFreeable = ID_TRUE;
        }
        else
        {
            *aIsFreeable = ID_FALSE;
        }
    }
    else
    {
        *aIsFreeable = ID_FALSE;
    }

    *aPageType = ((sdpPhyPageHdr*)sPage)->mPageType;

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage(aStatistics, sPage)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, sPage)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Commit SCN을 구한다.
 **********************************************************************/
IDE_RC sdcLob::getCommitSCN( idvSQL   * aStatistics,
                             sdSID      aTSSlotSID,
                             smSCN    * aFstDskViewSCN,
                             smSCN    * aCommitSCN )
{
    smTID   sDummyTID4Wait;
    smSCN   sCommitSCN;

    IDE_TEST( sdcTSSegment::getCommitSCN(aStatistics,
                                         NULL,        /* aTrans */
                                         aTSSlotSID,
                                         aFstDskViewSCN,
                                         SM_SCN_INIT, /* aStmtViewSCN */
                                         &sDummyTID4Wait,
                                         &sCommitSCN)
              != IDE_SUCCESS );

    if( SM_SCN_IS_NOT_INFINITE(sCommitSCN) )
    {
        *aCommitSCN = sCommitSCN;
    }
    else
    {
        *aCommitSCN = *aFstDskViewSCN;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Column Buffer에 Write한다.(In-Mode)
 **********************************************************************/
IDE_RC sdcLob::writeBuffer( smLobViewEnv    * aLobViewEnv,
                            UChar           * aPieceVal,
                            UInt              aPieceLen,
                            sdcLobWriteType   aWriteType )
{
    ULong             sNewLength = 0;
    sdcLobColBuffer   sOldLobColBuf;
    sdcLobColBuffer   sNewLobColBuf;
    sdcLobColBuffer * sLobColBuf;
    UInt              sOffset;
    UInt              sState = 0;

    IDE_ERROR( aLobViewEnv  != NULL );
    IDE_ERROR( aPieceVal    != NULL );

    sOffset    = aLobViewEnv->mWriteOffset;
    sLobColBuf = (sdcLobColBuffer *)aLobViewEnv->mLobColBuf;

    switch( aWriteType )
    {
        case SDC_LOB_WTYPE_WRITE:
            sNewLength = sOffset + aPieceLen;

            if( sNewLength > sLobColBuf->mLength )
            {
                sOldLobColBuf = *sLobColBuf;

                IDE_TEST( initLobColBuffer(
                              &sNewLobColBuf,
                              sNewLength,
                              SDC_COLUMN_IN_MODE,
                              ID_FALSE)
                          != IDE_SUCCESS );
                sState = 1;

                idlOS::memcpy( sNewLobColBuf.mBuffer,
                               sOldLobColBuf.mBuffer,
                               sOldLobColBuf.mLength );

                IDE_TEST( finalLobColBuffer(&sOldLobColBuf)
                          != IDE_SUCCESS );
                
                *sLobColBuf = sNewLobColBuf;
                sState = 0;
            }

            idlOS::memcpy( sLobColBuf->mBuffer + sOffset,
                           aPieceVal,
                           aPieceLen );
            break;

        case SDC_LOB_WTYPE_ERASE:
            sNewLength = sOffset + aPieceLen;

            IDE_ERROR( sNewLength <= sLobColBuf->mLength );

            if( aLobViewEnv->mLobCol.colType == SMI_LOB_COLUMN_TYPE_BLOB )
            {
                idlOS::memset( sLobColBuf->mBuffer + sOffset,
                               0x00,
                               aPieceLen );
            }
            else
            {
                idlOS::memset( sLobColBuf->mBuffer + sOffset,
                               ' ',
                               aPieceLen );
            }
            
            break;

        case SDC_LOB_WTYPE_TRIM:
            sNewLength = sOffset;

            IDE_ERROR( sNewLength <= sLobColBuf->mLength );

            if( sNewLength != sLobColBuf->mLength )
            {
                sOldLobColBuf = *sLobColBuf;

                IDE_TEST( initLobColBuffer(
                              &sNewLobColBuf,
                              sNewLength,
                              SDC_COLUMN_IN_MODE,
                              ID_FALSE)
                          != IDE_SUCCESS );
                sState = 1;

                idlOS::memcpy( sNewLobColBuf.mBuffer,
                               sOldLobColBuf.mBuffer,
                               sNewLength );

                IDE_TEST( finalLobColBuffer(&sOldLobColBuf)
                          != IDE_SUCCESS );

                *sLobColBuf = sNewLobColBuf;
                sState = 0;
            }
            break;

        default:
            IDE_ERROR( 0 );
            break;
    }

    aLobViewEnv->mLastWriteOffset = sOffset + aPieceLen;
    aLobViewEnv->mLastWriteLeafNodePID = SD_NULL_PID;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( finalLobColBuffer(&sNewLobColBuf) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Desc를 초기화한다.
 **********************************************************************/
void sdcLob::initLobDesc( sdcLobDesc * aLobDesc )
{
    UInt i;

    aLobDesc->mLobVersion   = 0;
    aLobDesc->mLobDescFlag  = SDC_LOB_DESC_NULL_FALSE;
    aLobDesc->mLastPageSize = 0;
    aLobDesc->mLastPageSeq  = 0;
    aLobDesc->mDirectCnt    = 0;
    aLobDesc->mRootNodePID  = SD_NULL_PID;

    for( i = 0; i < SDC_LOB_MAX_DIRECT_PAGE_CNT; i++ )
    {
        aLobDesc->mDirect[i] = SD_NULL_PID;
    }
}

/***********************************************************************
 * Description : LOB Meta Page를 초기화 한다.(mFreeList 초기화)
 *
 *   aMetaPtr - [IN] 초기화 할 LOB Meta Page의 Pointer
 *   aMtx     - [IN] 미니 트랜잭션
 **********************************************************************/
IDE_RC sdcLob::initLobMetaPage( UChar       * aMetaPtr,
                                smiColumn   * aColumn,
                                sdrMtx      * aMtx )
{
    sdcLobMeta  * sLobMeta;
    idBool        sLogging = ID_TRUE;
    idBool        sBuffer = ID_TRUE;
    UInt          sColumnID;

    sLobMeta = (sdcLobMeta*)( aMetaPtr + ID_SIZEOF( sdpPhyPageHdr ) );

    // logging
    if( (aColumn->flag & SMI_COLUMN_LOGGING_MASK)
        == SMI_COLUMN_LOGGING )
    {
        sLogging = ID_TRUE;
    }
    else
    {
        sLogging = ID_FALSE;
    }

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sLobMeta->mLogging,
                                         (void*)&sLogging,
                                         ID_SIZEOF(sLogging) )
              != IDE_SUCCESS );

    // buffer
    if( (aColumn->flag & SMI_COLUMN_USE_BUFFER_MASK)
        == SMI_COLUMN_USE_BUFFER )
    {
        sBuffer = ID_TRUE;
    }
    else
    {
        sBuffer = ID_FALSE;
    }

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sLobMeta->mBuffer,
                                         (void*)&sBuffer,
                                         ID_SIZEOF(sBuffer) )
              != IDE_SUCCESS );

    // Column ID
    sColumnID = aColumn->id;

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sLobMeta->mColumnID,
                                         (void*)&sColumnID,
                                         ID_SIZEOF(sColumnID) )
              != IDE_SUCCESS );

    // Agable List
    IDE_TEST( sdpDblPIDList::initBaseNode( &(sLobMeta->mAgingListBase),
                                           aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob 컬럼에 대한 페이지들을 Agable List에 연결한다.
 **********************************************************************/
IDE_RC sdcLob::removeLob( idvSQL            * aStatistics,     
                          void              * aTrans,
                          sdcLobDesc        * aLobDesc,
                          const smiColumn   * aColumn )
{
    UInt i;
    
    /* remove direct pages */
    for( i = 0; i < aLobDesc->mDirectCnt; i++ )
    {
        IDE_TEST( addPage2AgingList(aStatistics,
                                    aTrans,
                                    (smiColumn*)aColumn,
                                    aLobDesc->mDirect[i])
                  != IDE_SUCCESS );
    }

    /* remove index pages */
    if( aLobDesc->mRootNodePID != SD_NULL_PID )
    {
        IDE_TEST( addPage2AgingList(aStatistics,
                                    aTrans,
                                    (smiColumn*)aColumn,
                                    aLobDesc->mRootNodePID)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Desc 크기를 반환한다.
 **********************************************************************/
UInt sdcLob::getDiskLobDescSize()
{
    return ID_SIZEOF(sdcLobDesc);
}

/***********************************************************************
 * Description :  DiskLob을 위한 LobViewEnv 초기화
 *
 * aLobViewEnv - [OUT] 초기화할 aLobViewEnv 포인터
 **********************************************************************/
void sdcLob::initLobViewEnv( smLobViewEnv * aLobViewEnv )
{
    IDE_ASSERT( aLobViewEnv != NULL );

    aLobViewEnv->mTable = NULL;

    SC_MAKE_NULL_GRID( aLobViewEnv->mGRID );
    idlOS::memset( &aLobViewEnv->mLobCol, 0x00, ID_SIZEOF(smiColumn));

    aLobViewEnv->mTID = SM_NULL_TID;

    SM_INIT_SCN(&aLobViewEnv->mSCN);
    SM_INIT_SCN(&aLobViewEnv->mInfinite);

    aLobViewEnv->mOpenMode = SMI_LOB_READ_MODE;
    
    aLobViewEnv->mWriteOffset = 0;
    aLobViewEnv->mWritePhase = SM_LOB_WRITE_PHASE_NONE;
    aLobViewEnv->mWriteError = ID_FALSE;

    aLobViewEnv->mIsWritten = ID_FALSE;

    aLobViewEnv->mLastWriteOffset = 0;
    aLobViewEnv->mLastWriteLeafNodePID = SD_NULL_PID;

    aLobViewEnv->mLastReadOffset = 0;
    aLobViewEnv->mLastReadLeafNodePID = SD_NULL_PID;

    aLobViewEnv->mLobVersion = 0;
    aLobViewEnv->mColSeqInRowPiece = 0;
    aLobViewEnv->mLobColBuf = NULL;

    return;
}

/***********************************************************************
 * Description : Fetch할 때 넘겨줄 Column복사 함수
 *               LOB Descriptor를 가져올 용도
 *
 *    aColumn      - [IN] 사용하지 않음
 *    aDestValue   - [OUT] 복사한 것을 담을 곳
 *    aWriteOffset - [IN] 사용하지 않음
 *    aSrcLength   - [IN] 복사할 길이 sdcLobDesc와 같아야 한다.
 *    aSrcValue    - [IN] Page내 LobDescriptor의 Pointer
 **********************************************************************/
IDE_RC sdcLob::copyLobColData( const UInt   /*aColumnSize*/,
                               const void   * aDestValue,
                               UInt           aWriteOffset,
                               UInt           aSrcLength,
                               const void   * aSrcValue )
{
    smiValue * sLobColValue = (smiValue*)aDestValue;

    if( (aSrcValue == NULL) && (aSrcLength == 0) )
    {
        IDE_ASSERT( aWriteOffset == 0 );
        
        // NULL 데이타
        sLobColValue->value = NULL;
        sLobColValue->length = 0;
    }
    else
    {
        IDE_ASSERT( aSrcValue  != NULL );
        IDE_ASSERT( aDestValue != NULL );

        IDE_ASSERT( aWriteOffset + aSrcLength <= SDC_LOB_MAX_IN_MODE_SIZE );

        sLobColValue->length = (UShort)(aWriteOffset + aSrcLength);

        if( sLobColValue->length > 0 )
        {
            IDE_ASSERT( sLobColValue->value != NULL );
            
            idlOS::memcpy( (UChar*)sLobColValue->value + aWriteOffset,
                           aSrcValue,
                           aSrcLength );
        }
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Lob Column Buffer를 초기화한다.
 **********************************************************************/
IDE_RC sdcLob::initLobColBuffer( sdcLobColBuffer * aLobColBuf,
                                 UInt              aLength,
                                 sdcColInOutMode   aInOutMode,
                                 idBool            aIsNullLob )
{
    IDE_ERROR( aLength <= SDC_LOB_MAX_IN_MODE_SIZE );
    IDE_ERROR( aLobColBuf != NULL );

    aLobColBuf->mBuffer = NULL;
    
    if( aLength > 0 )
    {
        /* sdcLob_initLobColBuffer_malloc_Buffer.tc */
        IDU_FIT_POINT_RAISE("sdcLob::initLobColBuffer::malloc::Buffer",insufficient_memory);
        IDE_TEST_RAISE( iduMemMgr::malloc(
                            IDU_MEM_SM_SDC,
                            (ULong)aLength * ID_SIZEOF(UChar),
                            (void**)&(aLobColBuf->mBuffer))
                        != IDE_SUCCESS,
                        insufficient_memory );
    }
    
    aLobColBuf->mLength    = aLength;
    aLobColBuf->mInOutMode = aInOutMode;
    aLobColBuf->mIsNullLob = aIsNullLob;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Column Buffer를 해제한다.
 **********************************************************************/
IDE_RC sdcLob::finalLobColBuffer( sdcLobColBuffer * aLobColBuf )
{
    IDE_ERROR( aLobColBuf != NULL );
    
    if( aLobColBuf->mBuffer != NULL )
    {
        IDE_ASSERT( aLobColBuf->mLength > 0 );
            
        IDE_TEST( iduMemMgr::free(aLobColBuf->mBuffer)
                  != IDE_SUCCESS );
    }
    
    aLobColBuf->mBuffer    = NULL;
    aLobColBuf->mLength    = 0;
    aLobColBuf->mInOutMode = SDC_COLUMN_IN_MODE;
    aLobColBuf->mIsNullLob = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LOB Data의 크기를 LOB Descriptor 로부터 얻는다.
 *
 * aLobDesc - [IN] LOB Descriptor
 **********************************************************************/
ULong sdcLob::getLobLengthFromLobDesc( const sdcLobDesc* aLobDesc )
{
    ULong sLength = 0;

    IDE_ASSERT( aLobDesc != NULL );

    sLength =
        aLobDesc->mLastPageSize +
        (aLobDesc->mLastPageSeq * SDC_LOB_PAGE_BODY_SIZE);

    return sLength;
}

/***********************************************************************
 * Description : Lob Length를 반환한다.
 **********************************************************************/
IDE_RC sdcLob::getLobLength( sdcLobColBuffer  * aLobColBuf,
                             ULong            * aLength)
{
    sdcLobDesc  * sLobDesc;
    ULong         sLength  = 0;

    IDE_ERROR( aLobColBuf != NULL );

    if( aLobColBuf->mInOutMode == SDC_COLUMN_IN_MODE )
    {
        sLength = aLobColBuf->mLength;
    }
    else
    {
        IDE_ERROR( aLobColBuf->mLength == ID_SIZEOF(sdcLobDesc) );
        
        sLobDesc = (sdcLobDesc*)aLobColBuf->mBuffer;
        sLength = getLobLengthFromLobDesc(sLobDesc);
    }

    *aLength = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Column을 Lob Column Buffer에 읽어 들인다.
 **********************************************************************/
IDE_RC sdcLob::readLobColBuf( idvSQL       * aStatistics,
                              void         * aTrans,
                              smLobViewEnv * aLobViewEnv )
{
    UChar               * sRow;
    idBool                sTrySuccess;
    smiColumn             sColumn;
    smiValue              sLobColValue;
    smiFetchColumnList    sFetchColumnList;
    sdcLobInfo4Fetch      sLobInfo4Fetch;
    sdcLobColBuffer     * sLobColBuf;
    idBool                sIsRowDeleted;
    idBool                sIsPageLatchReleased = ID_TRUE;
    UChar                 sLobColData[SDC_LOB_MAX_IN_MODE_SIZE];
    smFetchVersion        sFetchVersion;
    UInt                  sState = 0;

    IDE_ERROR( aLobViewEnv != NULL );
    IDE_ERROR( aLobViewEnv->mLobColBuf != NULL );

    sLobColBuf = (sdcLobColBuffer*)aLobViewEnv->mLobColBuf;

    /* skip reading LOB column if the column was read. */
    IDE_TEST_CONT( sLobColBuf->mBuffer != NULL,
                    SKIP_READ_COL );

    /*
     * read Lob Column Value
     */
    
    IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                           aLobViewEnv->mGRID,
                                           SDB_S_LATCH,
                                           SDB_WAIT_NORMAL,
                                           NULL, /* aMtx */
                                           (UChar**)&sRow,
                                           &sTrySuccess )
              != IDE_SUCCESS );
    sIsPageLatchReleased = ID_FALSE;

    idlOS::memcpy( (void*)&sColumn,
                   (void*)&aLobViewEnv->mLobCol,
                   ID_SIZEOF(smiColumn) );
    sColumn.offset = 0;

    sFetchColumnList.column         = &sColumn;
    sFetchColumnList.columnSeq      = SDC_GET_COLUMN_SEQ( &sColumn );
    sFetchColumnList.copyDiskColumn = (void*)sdcLob::copyLobColData;
    sFetchColumnList.next           = NULL;

    sLobColValue.value  = sLobColData;
    sLobColValue.length = 0;

    sLobInfo4Fetch.mOpenMode = aLobViewEnv->mOpenMode;

    /* BUG-43093 lob size가 0일 경우 mInOutMode가 초기화되지 않는다. */
    sLobInfo4Fetch.mInOutMode = SDC_COLUMN_IN_MODE;

    if( aLobViewEnv->mOpenMode == SMI_LOB_READ_LAST_VERSION_MODE )
    {
        // DEAD Code 인듯.. 설정하는곳은 없지만.. 그냥 두자. 
        IDE_DASSERT(0);

        sFetchVersion = SMI_FETCH_VERSION_LAST;
    }
    else
    {
        sFetchVersion = SMI_FETCH_VERSION_CONSISTENT;
    }
    
    IDE_TEST( sdcRow::fetch( aStatistics,
                             NULL, /* aMtx */
                             NULL, /* aSP */
                             aTrans,
                             SC_MAKE_SPACE(aLobViewEnv->mGRID),
                             sRow,
                             ID_TRUE, /* aIsPersSlot */
                             SDB_SINGLE_PAGE_READ,
                             &sFetchColumnList,
                             sFetchVersion,
                             smxTrans::getTSSlotSID(aTrans),
                             &(aLobViewEnv->mSCN),
                             &(aLobViewEnv->mInfinite),
                             NULL, /* aIndexInfo4Fetch */
                             &sLobInfo4Fetch,
                             ((smcTableHeader*)aLobViewEnv->mTable)->mRowTemplate,
                             (UChar*)&sLobColValue,
                             &sIsRowDeleted,
                             &sIsPageLatchReleased )
              != IDE_SUCCESS );

    if( sIsPageLatchReleased == ID_FALSE )
    {
        sIsPageLatchReleased = ID_TRUE;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics,
                                            sRow)
                  != IDE_SUCCESS );
    }

    /*
     * alloc & set Lob Column Buffer
     */
    
    IDE_TEST( sdcLob::initLobColBuffer(
                  sLobColBuf,
                  sLobColValue.length,
                  sLobInfo4Fetch.mInOutMode,
                  SDC_IS_NULL(&sLobColValue))
              != IDE_SUCCESS );
    sState = 1;

    if( sLobColValue.length > 0 )
    {
        idlOS::memcpy( sLobColBuf->mBuffer, 
                       sLobColValue.value, 
                       sLobColValue.length );
    }
    
    IDE_EXCEPTION_CONT( SKIP_READ_COL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if( sIsPageLatchReleased == ID_FALSE )
    {
        sIsPageLatchReleased = ID_TRUE;
        IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics, sRow)
                    == IDE_SUCCESS );
    }

    if( sState == 1 )
    {
        IDE_ASSERT( sdcLob::finalLobColBuffer(sLobColBuf) 
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Column Buffer의 값으로 Lob Column을 update한다.
 **********************************************************************/
IDE_RC sdcLob::updateLobCol( idvSQL         * aStatistics,
                             void           * aTrans,
                             smLobViewEnv   * aLobViewEnv )
{
    smiValue          sValue;
    smiColumnList     sColumn;
    smxTableInfo    * sTableInfoPtr;
    sdcLobColBuffer * sLobColBuf;
    sdcColInOutMode   sValueMode;
    sdcLobDesc      * sLobDesc;
    UInt              sDummy;

    sLobColBuf = (sdcLobColBuffer*)aLobViewEnv->mLobColBuf;
    sValueMode = sLobColBuf->mInOutMode;

    if( sLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB )
    {
        IDE_ERROR( sLobColBuf->mLength == ID_SIZEOF(sdcLobDesc) );

        sLobDesc = (sdcLobDesc *)sLobColBuf->mBuffer;
        sLobDesc->mLobVersion = aLobViewEnv->mLobVersion;
    }
    else
    {
        /* nothing to do */
    }

    if( sLobColBuf->mIsNullLob == ID_TRUE )
    {
        sValue.length = 0;
        sValue.value  = NULL;
    }
    else
    {
        if( (sLobColBuf->mBuffer == NULL) && (sLobColBuf->mLength == 0) )
        {
            // empty
            sValue.length = 0;
            sValue.value  = &sDummy;
        }
        else
        {
            sValue.length = sLobColBuf->mLength;
            sValue.value  = sLobColBuf->mBuffer;
        }
    }
    
    sColumn.next   = NULL;
    sColumn.column = &(aLobViewEnv->mLobCol);

    IDE_TEST( ((smxTrans*)aTrans)->mTableInfoMgr.getTableInfo(
                  smLayerCallback::getTableOID( aLobViewEnv->mTable ),
                  &sTableInfoPtr,
                  ID_FALSE ) // IsSearch
              != IDE_SUCCESS );

    IDE_TEST( sdcRow::update( aStatistics,
                              aTrans,
                              aLobViewEnv->mSCN,
                              (void*)sTableInfoPtr,
                              aLobViewEnv->mTable,
                              NULL,     /*aOldRow*/
                              aLobViewEnv->mGRID,
                              NULL,     /*aRetRow*/
                              NULL,     /*aRetUpdateSlotGSID*/
                              &sColumn,
                              &sValue,
                              NULL,     /*aDMLRetryInfo*/
                              aLobViewEnv->mInfinite,
                              &sValueMode,
                              NULL,     /*aModifyIdxBit*/
                              ID_FALSE )/* aForbiddenToRetry */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Version을 갱신한다.
 **********************************************************************/
IDE_RC sdcLob::adjustLobVersion( smLobViewEnv * aLobViewEnv )
{
    sdcLobColBuffer * sLobColBuf;
    sdcLobDesc      * sLobDesc;

    IDE_ERROR( aLobViewEnv != NULL );
    
    sLobColBuf = (sdcLobColBuffer *)aLobViewEnv->mLobColBuf;

    if( sLobColBuf->mInOutMode == SDC_COLUMN_OUT_MODE_LOB )
    {
        IDE_ASSERT( sLobColBuf->mLength == ID_SIZEOF(sdcLobDesc) );
        
        sLobDesc = (sdcLobDesc *)sLobColBuf->mBuffer;

        IDE_TEST_RAISE( sLobDesc->mLobVersion == ID_ULONG_MAX,
                        error_version_overflow );

        if( aLobViewEnv->mOpenMode == SMI_LOB_READ_WRITE_MODE )
        {
            aLobViewEnv->mLobVersion = sLobDesc->mLobVersion + 1;
        }
        else
        {
            IDE_ERROR( aLobViewEnv->mOpenMode == SMI_LOB_READ_MODE );
            
            aLobViewEnv->mLobVersion = sLobDesc->mLobVersion;
        }
    }
    else
    {
        aLobViewEnv->mLobVersion = 1;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_version_overflow )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, 
                                 "adjustLobVersion:version overflow") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ================================================
 *
 *    Stack Functions
 *
 * ================================================ */

void sdcLob::initStack( sdcLobStack * aStack )
{
    aStack->mPos = -1;
    
    idlOS::memset( aStack->mStack, 0x00, ID_SIZEOF(aStack->mStack) );
}


IDE_RC sdcLob::pushStack( sdcLobStack * aStack,
                          scPageID      aPageID )
{
    IDE_ERROR( aStack->mPos < (SDC_LOB_STACK_SIZE - 1) );
    
    aStack->mPos++;

    aStack->mStack[aStack->mPos] = aPageID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdcLob::popStack( sdcLobStack * aStack,
                         scPageID    * aPageID )
{
    IDE_ERROR( aStack->mPos >= 0 );
    
    *aPageID = aStack->mStack[aStack->mPos];

    aStack->mPos--;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SInt sdcLob::getPosStack( sdcLobStack * aStack )
{
    return aStack->mPos;
}

/* ================================================
 *
 *    Logging Functions
 *
 * ================================================ */

IDE_RC sdcLob::writeLobWritePieceRedoLog( sdrMtx        * aMtx,
                                          void          * aTable,
                                          UChar         * aLobDataLayerStartPtr,
                                          idBool          aIsPrevVal,
                                          UInt            aOffset,
                                          UInt            aAmount,
                                          UChar         * aWriteValue,
                                          UInt            aWriteSize,
                                          UInt            aWriteOffset,
                                          smLobLocator    aLobLocator )
{
    UInt        sRedoLogSize;
    sdrLogType  sLogType;

    sRedoLogSize = (ID_SIZEOF(UInt) +           /* write size */
                    aWriteSize +                /* write value, */
                    ID_SIZEOF(smLobLocator) +   /* lob locator */
                    ID_SIZEOF(UInt) +           /* offset */
                    ID_SIZEOF(UInt));           /* amount */

    if( aIsPrevVal == ID_TRUE )
    {
        sLogType = SDR_SDC_LOB_WRITE_PIECE_PREV;
    }
    else
    {
        sLogType = SDR_SDC_LOB_WRITE_PIECE;
    }

    /* This funcation must be called for replication */
    sdrMiniTrans::setRefOffset( aMtx,
                                smLayerCallback::getTableOID( aTable ) );

    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  aLobDataLayerStartPtr + aWriteOffset,
                  NULL,
                  sRedoLogSize,
                  sLogType)
              != IDE_SUCCESS);

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aWriteSize,
                                   ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
    
    /*
     * fix BUG-15799, replication에 위하여
     * lob을 SQL구문으로 null로 update하는 것을 표현하기 위함.
     */
    
    if( aWriteSize > 0 )
    {
        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       aWriteValue,
                                       aWriteSize )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aLobLocator,
                                   ID_SIZEOF(aLobLocator) )
              != IDE_SUCCESS);

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aOffset,
                                   ID_SIZEOF(UInt) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aAmount,
                                   ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdcLob::writeLobWritePiece4DMLRedoLog( sdrMtx    * aMtx,
                                              void      * aTable,
                                              smiColumn * aColumn,
                                              UChar     * aLobDataLayerStartPtr,
                                              UInt        aAmount,
                                              UChar     * aWriteValue,
                                              UInt        aWriteSize,
                                              UInt        aWriteOffset )
{
    UInt        sRedoLogSize;
    sdrLogType  sLogType;

    sRedoLogSize = (ID_SIZEOF(UInt) + /* write size */
                    aWriteSize +      /* write value */
                    ID_SIZEOF(UInt) + /* column id */
                    ID_SIZEOF(UInt)); /* amount */

    sLogType = SDR_SDC_LOB_WRITE_PIECE4DML;

    /* This funcation must be called for replication */
    sdrMiniTrans::setRefOffset( aMtx,
                                smLayerCallback::getTableOID(aTable) );

    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  aLobDataLayerStartPtr + aWriteOffset,
                  NULL,
                  sRedoLogSize,
                  sLogType)
              != IDE_SUCCESS);

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aWriteSize,
                                   ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
    
    /*
     * fix BUG-15799, replication에 위하여
     * lob을 SQL구문으로 null로 update하는 것을 표현하기 위함.
     */
    
    if( aWriteSize > 0 )
    {
        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       aWriteValue,
                                       aWriteSize )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &(aColumn->id),
                                   ID_SIZEOF(UInt) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aAmount,
                                   ID_SIZEOF(UInt) )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ================================================
 *
 *    CLR Functions
 *
 * ================================================ */
IDE_RC sdcLob::writeLobInsertLeafKeyCLR( sdrMtx * aMtx,
                                         UChar  * aLeafKey,
                                         SShort   aLeafKeySeq )
{
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR( aLeafKey != NULL );

    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)aLeafKey,
                  (void*)&aLeafKeySeq,
                  ID_SIZEOF(aLeafKeySeq),
                  SDR_SDC_LOB_FREE_LEAF_KEY)
              != IDE_SUCCESS );
  
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdcLob::writeLobUpdateLeafKeyCLR( sdrMtx     * aMtx,
                                         UChar      * aKeyPtr,
                                         SShort       aLeafKeySeq,
                                         sdcLobLKey * aLKey )
{
    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)aKeyPtr,
                  (void*)NULL,
                  ID_SIZEOF(SShort) + ID_SIZEOF(sdcLobLKey),
                  SDR_SDC_LOB_UPDATE_LEAF_KEY )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&aLeafKeySeq,
                                   ID_SIZEOF(aLeafKeySeq) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)aLKey,
                                   ID_SIZEOF(sdcLobLKey) )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdcLob::writeLobOverwriteLeafKeyCLR( sdrMtx     * aMtx,
                                            UChar      * aKeyPtr,
                                            SShort       aLeafKeySeq,
                                            sdcLobLKey * aOldLKey,
                                            sdcLobLKey * aNewLKey )
{
    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)aKeyPtr,
                  (void*)NULL,
                  ID_SIZEOF(SShort) +
                  ID_SIZEOF(sdcLobLKey) +
                  ID_SIZEOF(sdcLobLKey),
                  SDR_SDC_LOB_OVERWRITE_LEAF_KEY )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&aLeafKeySeq,
                                   ID_SIZEOF(aLeafKeySeq) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)aOldLKey,
                                   ID_SIZEOF(sdcLobLKey) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)aNewLKey,
                                   ID_SIZEOF(sdcLobLKey) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt sdcLob::getNeedPageCount( sdcLobColBuffer  * aLobColBuf,
                               smiColumn        * aColumn,                               
                               UInt               aOffset,
                               UInt               aPieceLen )
{
    UInt sNeedPageCnt = 0;
    UInt sLobLength = 0;
    sdcLobChangeType sType;
    
    sType = getLobChangeType( aLobColBuf,
                              aColumn,
                              aOffset,
                              aPieceLen );

    switch( sType )
    {
        case SDC_LOB_IN_TO_IN:
            sNeedPageCnt = 0;
            break;
            
        case SDC_LOB_IN_TO_OUT:
            sLobLength = 
                aLobColBuf->mLength > (aOffset + aPieceLen) ?
                aLobColBuf->mLength : (aOffset + aPieceLen);

            sNeedPageCnt =
                (sLobLength + SDC_LOB_PAGE_BODY_SIZE - 1) / SDC_LOB_PAGE_BODY_SIZE; 
            break;
            
        case SDC_LOB_OUT_TO_OUT:
            sNeedPageCnt =
                (aPieceLen + SDC_LOB_PAGE_BODY_SIZE - 1) / SDC_LOB_PAGE_BODY_SIZE; 
            break;
            
        default:
            IDE_ASSERT( 0 );
            break;
    }    

    return sNeedPageCnt;
}

/* ================================================
 *
 *    Dump Functions
 *
 * ================================================ */

/***********************************************************************
 * Description: Lob Data Page의 Logical Header 를 Dump
 ***********************************************************************/
IDE_RC sdcLob::dumpLobDataPageHdr( UChar * aPage,
                                   SChar * aOutBuf,
                                   UInt    aOutSize )
{
    sdcLobNodeHdr   * sNodeHdr;
    SChar             sStrFstDskViewSCN[ SM_SCN_STRING_LENGTH + 1];
#ifndef COMPILE_64BIT
    ULong             sSCN;
#endif

    IDE_ERROR( aPage != NULL );
    IDE_ERROR( sdpPhyPage::getPageStartPtr(aPage) == aPage );

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( aPage );

    idlOS::memset( sStrFstDskViewSCN, 0x00, ID_SIZEOF(sStrFstDskViewSCN) );
    
#ifdef COMPILE_64BIT
    idlOS::sprintf( (SChar*)sStrFstDskViewSCN,
                    "%"ID_XINT64_FMT,
                    sNodeHdr->mFstDskViewSCN );
#else
    sSCN  = (ULong)sNodeHdr->mFstDskViewSCN.mHigh << 32;
    sSCN |= (ULong)sNodeHdr->mFstDskViewSCN.mLow;
    idlOS::snprintf( (SChar*)sStrFstDskViewSCN,
                     SM_SCN_STRING_LENGTH,
                     "%"ID_XINT64_FMT,
                     sSCN );
#endif

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "------ Disk LOB Data Page Header Begin -----\n" );

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "LOB Logical Header\n"
                         " - Height               : %"ID_UINT32_FMT"\n"
                         " - Key Count            : %"ID_UINT32_FMT"\n"
                         " - Store Size           : %"ID_UINT32_FMT"\n"
                         " - Node Sequence        : %"ID_UINT32_FMT"\n"
                         " - Lob Page State       : %"ID_UINT32_FMT"\n"
                         
                         " - Lob Version          : %"ID_UINT64_FMT"\n"
                         " - TSSlot ID            : %"ID_UINT64_FMT"\n"
                         " - FstDskViewSCN        : %s\n",
                         sNodeHdr->mHeight,
                         sNodeHdr->mKeyCnt,
                         sNodeHdr->mStoreSize,
                         sNodeHdr->mNodeSeq,
                         sNodeHdr->mLobPageState,
                         sNodeHdr->mLobVersion,
                         sNodeHdr->mTSSlotSID,
                         sStrFstDskViewSCN );

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "------- Disk LOB Data Page Header End ------\n" );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: Lob Data Page에 저장된 Data를 Dump한다.
 * BUG-29385 Lob Data, Lob Inode등의 Lob Page PBT 기능이 필요합니다.
 ***********************************************************************/
IDE_RC sdcLob::dumpLobDataPageBody( UChar *aPage ,
                                    SChar *aOutBuf ,
                                    UInt   aOutSize )
{
    SChar     sTempBuf[IDE_DUMP_DEST_LIMIT] = {0,};
    UChar   * sLobDataStartPtr;

    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( sdpPhyPage::getPageStartPtr(aPage) == aPage );

    sLobDataStartPtr = getLobDataLayerStartPtr( aPage );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "------------ Disk LOB Data Begin -----------\n" );

    /* page를 Hexa code로 dump하여 출력한다. */
    if( ideLog::ideMemToHexStr( sLobDataStartPtr,
                                SDC_LOB_PAGE_BODY_SIZE,
                                IDE_DUMP_FORMAT_NORMAL,
                                sTempBuf,
                                IDE_DUMP_DEST_LIMIT )
        == IDE_SUCCESS )
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%s\n",
                             sTempBuf);
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "------------- Disk Lob Data End ------------\n" );
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description: Lob Meta Page를 Dump한다.
 * BUG-29385 Lob Data, Lob Inode등의 Lob Page PBT 기능이 필요합니다.
 ***********************************************************************/
IDE_RC sdcLob::dumpLobMeta( UChar *aPage ,
                            SChar *aOutBuf ,
                            UInt   aOutSize )
{
    sdcLobMeta*   sLobMeta;

    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( sdpPhyPage::getPageStartPtr(aPage) == aPage );

    sLobMeta = (sdcLobMeta*)sdpPhyPage::getLogicalHdrStartPtr( aPage );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "----------- Disk LOB Meta Page Begin ----------\n" );

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "LOB Meta\n"
                         " - Logging             : %s\n"
                         " - Buffer              : %s\n"
                         " - Column ID           : %"ID_UINT64_FMT"\n"
                         " - AgingList Node Cnt  : %"ID_UINT64_FMT"\n"
                         " - AgingList Prev Page : %"ID_UINT32_FMT"\n"
                         " - AgingList Next Page : %"ID_UINT32_FMT"\n",
                         sLobMeta->mLogging == ID_TRUE ? "True" : "False",
                         sLobMeta->mBuffer  == ID_TRUE ? "True" : "False",
                         sLobMeta->mColumnID,
                         sLobMeta->mAgingListBase.mNodeCnt,
                         sLobMeta->mAgingListBase.mBase.mPrev,
                         sLobMeta->mAgingListBase.mBase.mNext );

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "------------ Disk LOB Meta Page End -----------\n" );

    return IDE_SUCCESS;
}

/* ---------------------------------------------------
 *  Fixed Table Define for X$DISK_LOB_STATISTICS
 * -------------------------------------------------*/

void sdcLob::initializeFixedTableArea()
{
    gDiskLobStatistics.mOpen            = 0;
    gDiskLobStatistics.mRead            = 0;
    gDiskLobStatistics.mWrite           = 0;
    gDiskLobStatistics.mErase           = 0;
    gDiskLobStatistics.mTrim            = 0;
    gDiskLobStatistics.mPrepareForWrite = 0;
    gDiskLobStatistics.mFinishWrite     = 0;
    gDiskLobStatistics.mGetLobInfo      = 0;
    gDiskLobStatistics.mClose           = 0;
}

static iduFixedTableColDesc gDiskLobStatisticsColDesc[] =
{
    {
        (SChar*)"OPEN_COUNT",
        offsetof(sdcLobStatistics, mOpen),
        IDU_FT_SIZEOF(sdcLobStatistics, mOpen),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"READ_COUNT",
        offsetof(sdcLobStatistics, mRead),
        IDU_FT_SIZEOF(sdcLobStatistics, mRead),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"WRITE_COUNT",
        offsetof(sdcLobStatistics, mWrite),
        IDU_FT_SIZEOF(sdcLobStatistics, mWrite),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
        {
        (SChar*)"ERASE_COUNT",
        offsetof(sdcLobStatistics, mErase),
        IDU_FT_SIZEOF(sdcLobStatistics, mErase),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TRIM_COUNT",
        offsetof(sdcLobStatistics, mTrim),
        IDU_FT_SIZEOF(sdcLobStatistics, mTrim),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"PREPARE_FOR_WRITE_COUNT",
        offsetof(sdcLobStatistics, mPrepareForWrite),
        IDU_FT_SIZEOF(sdcLobStatistics, mPrepareForWrite),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"FINISH_WRITE_COUNT",
        offsetof(sdcLobStatistics, mFinishWrite),
        IDU_FT_SIZEOF(sdcLobStatistics, mFinishWrite),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"GET_LOBINFO_COUNT",
        offsetof(sdcLobStatistics, mGetLobInfo),
        IDU_FT_SIZEOF(sdcLobStatistics, mGetLobInfo),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"CLOSE_COUNT",
        offsetof(sdcLobStatistics, mClose),
        IDU_FT_SIZEOF(sdcLobStatistics, mClose),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

static IDE_RC buildRecordForDiskLobStatistics(
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * /* aDumpObj */,
    iduFixedTableMemory * aMemory )
{
    IDE_TEST(iduFixedTable::buildRecord(
                 aHeader,
                 aMemory,
                 (void *) &gDiskLobStatistics)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gDiskLobStatisticsTableDesc =
{
    (SChar *)"X$DISK_LOB_STATISTICS",
    buildRecordForDiskLobStatistics,
    gDiskLobStatisticsColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
