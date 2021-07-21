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

#include <smcLob.h>
#include <smcReq.h>
#include <smr.h>
#include <smErrorCode.h>
#include <smcLobUpdate.h>

smLobModule smcLobModule =
{
    smcLob::open,
    smcLob::read,
    smcLob::write,
    smcLob::erase,
    smcLob::trim,
    smcLob::prepare4Write,
    smcLob::finishWrite,
    smcLob::getLobInfo,
    smcLobUpdate::writeLog4CursorOpen,
    smcLob::close
};

smcLobStatistics smcLob::mMemLobStatistics;

IDE_RC smcLob::open()
{
    mMemLobStatistics.mOpen++;

    return IDE_SUCCESS;
}

IDE_RC smcLob::close( idvSQL*        aStatistics,
                      void         * aTrans,
                      smLobViewEnv * aLobViewEnv )
{
    ACP_UNUSED( aStatistics );
    ACP_UNUSED( aTrans );
    ACP_UNUSED( aLobViewEnv );

    mMemLobStatistics.mClose++;

    return IDE_SUCCESS;
}

void smcLob::initializeFixedTableArea()
{
    mMemLobStatistics.mOpen            = 0;
    mMemLobStatistics.mRead            = 0;
    mMemLobStatistics.mWrite           = 0;
    mMemLobStatistics.mPrepareForWrite = 0;
    mMemLobStatistics.mFinishWrite     = 0;
    mMemLobStatistics.mGetLobInfo      = 0;
    mMemLobStatistics.mClose           = 0;
}

/**********************************************************************
 * lobCursor가 가르키고 있는 메모리 LOB 데이타를 읽어온다.
 *
 * aTrans      [IN]  작업하는 트랜잭션 객체
 * aLobViewEnv [IN]  작업하려는 LobViewEnv 객체
 * aOffset     [IN]  읽어오려는 Lob 데이타의 위치
 * aMount      [IN]  읽어오려는 piece의 크기
 * aPiece      [OUT] 반환하려는 Lob 데이타 piece 포인터
 **********************************************************************/
IDE_RC smcLob::read( idvSQL        * /*aStatistics */,
                     void          * aTrans,
                     smLobViewEnv  * aLobViewEnv,
                     UInt            aOffset,
                     UInt            aMount,
                     UChar         * aPiece,
                     UInt          * aReadLength )
{
    smcLobDesc*    sLobDesc;
    smcLPCH*       sTargetLPCH;
    UShort         sPieceOffset;
    UShort         sReadableSize;
    UInt           sReadSize = 0;
    SChar*         sCurFixedRowPtr;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aLobViewEnv != NULL );
    IDE_DASSERT( aPiece != NULL );

    if ( aMount > 0 )
    {
        // fixed row의 읽을 버전을 선택한다.
        IDE_TEST( getViewRowPtr(aTrans,
                                aLobViewEnv,
                                &sCurFixedRowPtr)
                  != IDE_SUCCESS );

        // 대상 lob data를 읽는다.
        sLobDesc = (smcLobDesc *)(sCurFixedRowPtr + aLobViewEnv->mLobCol.offset);

        IDE_TEST_RAISE( aOffset >= sLobDesc->length, range_error );

        if ( (aOffset + aMount) > sLobDesc->length )
        {
            aMount = sLobDesc->length - aOffset;
        }
        else
        {
            /* nothind to do */
        }
        
        *aReadLength = aMount;

        if ( SM_VCDESC_IS_MODE_IN(sLobDesc) )
        {
            idlOS::memcpy(aPiece,
                          (UChar*)sLobDesc + ID_SIZEOF(smVCDescInMode) + aOffset,
                          aMount);
        }
        else
        {
            sTargetLPCH = findPosition(sLobDesc, aOffset);

            sPieceOffset = aOffset % SMP_VC_PIECE_MAX_SIZE;

            while ( sReadSize < aMount )
            {
                sReadableSize = SMP_VC_PIECE_MAX_SIZE - sPieceOffset;
                if ( sReadableSize > (aMount - sReadSize) )
                {
                    sReadableSize = (aMount - sReadSize);
                }
                else
                {
                    /* nothing to do */
                }

                IDE_ERROR( sTargetLPCH->mOID != 0 );
                
                readPiece( sTargetLPCH,
                           sPieceOffset,
                           sReadableSize,
                           aPiece + sReadSize );

                sReadSize += sReadableSize;
                sTargetLPCH++;
                sPieceOffset = 0;
            }
        }
    }

    mMemLobStatistics.mRead++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(range_error);
    {
        // BUG-29212 디스크 Lob의 read시 range check를 하지 않습니다.
        // Read시에는 Start Offset만 확인합니다.
        // End Offset이 LobLength를 넘어가더라도
        // 읽을 수 있는 부분만 읽고 읽은 Read Size를 반환합니다.
        // 오류메시지를 Range오류에서 Offset오류로 변경합니다.
        IDE_SET(ideSetErrorCode( smERR_ABORT_InvalidLobStartOffset,
                                 aOffset,
                                 sLobDesc->length));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * 새로 할당된 공간에 lob 데이타를 기록한다.
 *
 * aTrans      [IN] 작업하는 트랜잭션 객체
 * aLobViewEnv [IN] 작업하려는 LobViewEnv 객체
 * aLobLocator [IN] 작업하려는 Lob Locator
 * aOffset     [IN] 작업하려는 Lob 데이타의 위치
 * aPieceLen   [IN] 새로 입력되는 piece의 크기
 * aPiece      [IN] 새로 입력되는 lob 데이타 piece
 **********************************************************************/
IDE_RC smcLob::write( idvSQL       * /* aStatistics */,
                      void         * aTrans,
                      smLobViewEnv * aLobViewEnv,
                      smLobLocator   aLobLocator,
                      UInt           aOffset,
                      UInt           aPieceLen,
                      UChar        * aPiece,
                      idBool        /* aIsFromAPI */,
                      UInt          /* aContType */)
{
    smSCN          sSCN;
    smTID          sTID;
    idBool         sIsReplSenderSend;
    smpSlotHeader* sCurFixedSlotHeader;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aLobViewEnv != NULL );
    IDE_DASSERT( aPiece != NULL );

    /* 갱신할 Row를 선택한다. 갱신할 Row는 가장 마지막 Row이다. */
    sCurFixedSlotHeader = (smpSlotHeader*)aLobViewEnv->mRow;

    while ( SMP_SLOT_HAS_VALID_NEXT_OID( sCurFixedSlotHeader ) )
    {
        IDE_ASSERT( smmManager::getOIDPtr( 
                              aLobViewEnv->mLobCol.colSpace,
                              SMP_SLOT_GET_NEXT_OID( sCurFixedSlotHeader ),
                              (void**)&sCurFixedSlotHeader )
                    == IDE_SUCCESS );
    }

    // aTrans 트랜잭션이  LobCursor의 memory row 인  mRow에 대하여
    // record lock을 쥔 트랜잭션인지 확인한다.
    SMX_GET_SCN_AND_TID( sCurFixedSlotHeader->mCreateSCN, sSCN, sTID );

    if ( smLayerCallback::getLogTypeFlagOfTrans( aTrans )
         == SMR_LOG_TYPE_NORMAL )
    {
        /* BUG-16003:
         * Sender가 하나의 Row에 대해서 같은 Table Cursor로
         * LOB Cursor를 두개 열면 두개의 LOB Cursor는 같은
         * Infinite SCN을 가진다. 하지만 Receiver단에서는
         * 각기 다른 Table Cursor로 LOB Cursor가 두개열리게
         * 되어서 다른 Infinite SCN을 가지게 되어 Sender에서
         * 성공한 Prepare가 Receiver단에서는 Too Old에러가
         * 발생한다. 위 현상을 방지하기 위해 Normal Transaction
         * 일 경우에만 아래 체크를 수행한다. Replication은 성공한
         * 연산일 경우에만 Log가 기록되기 때문에 아래 Validate는
         * 무시해도 된다.*/

        IDE_ASSERT( SM_SCN_IS_EQ(&sSCN, &(aLobViewEnv->mInfinite)));
    }
    else
    {
        /* nothing to do */
    }

    IDE_ASSERT( sTID == smLayerCallback::getTransID( aTrans ) );

    /*
     * reserve space
     */
    
    IDE_TEST( reserveSpace(aTrans,
                           aLobViewEnv,
                           (SChar*)sCurFixedSlotHeader,
                           aOffset,
                           aPieceLen)
              != IDE_SUCCESS );

    /*
     * for replication.
     */
    
    if ( (aLobViewEnv->mOpenMode == SMI_LOB_READ_WRITE_MODE) &&
         (smcTable::needReplicate(
            (const smcTableHeader*)aLobViewEnv->mTable, aTrans) == ID_TRUE ) )
    {
        sIsReplSenderSend = ID_TRUE;
    }
    else
    {
        sIsReplSenderSend = ID_FALSE;
    }

    IDE_TEST( writeInternal(aTrans,
                            (smcTableHeader*)aLobViewEnv->mTable,
                            (UChar*)sCurFixedSlotHeader,
                            &aLobViewEnv->mLobCol,
                            aOffset,
                            aPieceLen,
                            aPiece,
                            ID_TRUE,
                            sIsReplSenderSend,
                            aLobLocator)
              != IDE_SUCCESS );

    aLobViewEnv->mWriteOffset += aPieceLen;

    mMemLobStatistics.mWrite++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 이 코드는 추 후 필요한 기능으로 판단되어 구현되었습니다.
 * 사용전에 테스트가 필요합니다.
 */
IDE_RC smcLob::erase( idvSQL       * aStatistics,
                      void         * aTrans,
                      smLobViewEnv * aLobViewEnv,
                      smLobLocator   aLobLocator,
                      UInt           aOffset,
                      UInt           aPieceLen )
{
    SChar*      sFixedRowPtr = NULL;
    smcLobDesc* sCurLobDesc  = NULL;
    SChar*      sLobInRowPtr = NULL;
    UChar       sTmpPiece[SMP_VC_PIECE_MAX_SIZE];
    UInt        sTmpPieceLen;
    UInt        sTmpOffset;
    UInt        sEraseLen;
    
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aLobViewEnv != NULL );

    /*
     * to update LOB column
     */

    IDE_TEST( getLastVersion(aStatistics,
                             aTrans,
                             aLobViewEnv,
                             &sFixedRowPtr)
              != IDE_SUCCESS );

    /*
     * to set new LOB version
     */
    
    sLobInRowPtr = sFixedRowPtr + aLobViewEnv->mLobCol.offset;
    sCurLobDesc  = (smcLobDesc*)sLobInRowPtr;

    IDE_TEST_RAISE( (aOffset > sCurLobDesc->length), range_error );

    if( (sCurLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT )
    {
        // LobVersion 은 Out Mode 에서만 유효. 
        aLobViewEnv->mLobVersion = sCurLobDesc->mLobVersion + 1;
        IDE_TEST_RAISE( aLobViewEnv->mLobVersion == ID_ULONG_MAX,
                        error_version_overflow );    
    }
    else
    {
        aLobViewEnv->mLobVersion = 1;
    }

    /*
     * to erase
     */
    sEraseLen = sCurLobDesc->length > (aOffset + aPieceLen) ?
        aPieceLen : (sCurLobDesc->length - aOffset);

    IDE_TEST( reserveSpace(aTrans,
                           aLobViewEnv,
                           sFixedRowPtr,
                           aOffset,
                           sEraseLen)
              != IDE_SUCCESS );

    if ( aLobViewEnv->mLobCol.colType == SMI_LOB_COLUMN_TYPE_BLOB )
    {
        idlOS::memset( sTmpPiece, 0x00, ID_SIZEOF(sTmpPiece) );
    }
    else
    {
        idlOS::memset( sTmpPiece, ' ', ID_SIZEOF(sTmpPiece) );
    }

    sTmpOffset = aOffset;

    while ( sEraseLen > 0 )
    {
        if ( sEraseLen > ID_SIZEOF(sTmpPiece) )
        {
            sTmpPieceLen = ID_SIZEOF(sTmpPiece);
        }
        else
        {
            sTmpPieceLen = sEraseLen;
        }
        
        IDE_TEST( writeInternal(aTrans,
                                (smcTableHeader*)aLobViewEnv->mTable,
                                (UChar*)sFixedRowPtr,
                                &aLobViewEnv->mLobCol,
                                sTmpOffset,
                                sTmpPieceLen,
                                sTmpPiece,
                                ID_TRUE,
                                ID_FALSE, /* aIsReplSenderSend */
                                aLobLocator)
                  != IDE_SUCCESS );

        sEraseLen  -= sTmpPieceLen;
        sTmpOffset += sTmpPieceLen;
    }

    /*
     * to write replication log
     */
    
    if ( (aLobViewEnv->mOpenMode == SMI_LOB_READ_WRITE_MODE) &&
         (smcTable::needReplicate(
            (const smcTableHeader*)aLobViewEnv->mTable, aTrans) == ID_TRUE ) )
    {
        IDE_TEST( smrLogMgr::writeLobEraseLogRec(aStatistics,
                                                 aTrans,
                                                 aLobLocator,
                                                 aOffset,
                                                 aPieceLen,
                                                 ((const smcTableHeader*)aLobViewEnv->mTable)->mSelfOID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    mMemLobStatistics.mErase++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(range_error);
    {
        // BUG-29212 디스크 Lob의 read시 range check를 하지 않습니다.
        // range는 Offset부터 Amount - 1 까지 입니다.
        IDE_SET( ideSetErrorCode(smERR_ABORT_RangeError,
                                 aOffset,
                                 (aOffset + aPieceLen - 1),
                                 0,
                                 (sCurLobDesc->length - 1)) );
    }
    IDE_EXCEPTION( error_version_overflow )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG,
                                 "memory erase:version overflow") );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * 새로 할당된 공간에 lob 데이타를 기록한다.
 *
 * aTrans            [IN] 작업하는 트랜잭션 객체
 * aTable            [IN] 작업하는 테이블 헤더
 * aFixedRowPtr      [IN] lob 데이타를 저장할 fixed row
 * aLobColumn        [IN] lob 데이타를 저장할 column 객체
 * aOffset           [IN] lob 데이타의 저장 위치
 * aPieceLen         [IN] 새로 입력되는 piece의 크기
 * aPiece            [IN] 새로 입력되는 lob 데이타 piece
 * aIsWriteLog       [IN] 로깅 여부
 * aIsReplSenderSend [IN] replication 작동 여부
 * aLobLocator       [IN] lob locator 객체
 **********************************************************************/
IDE_RC smcLob::writeInternal( void             * aTrans,
                              smcTableHeader   * aTable,
                              UChar            * aFixedRowPtr,
                              const smiColumn  * aLobColumn,
                              UInt               aOffset,
                              UInt               aPieceLen,
                              UChar            * aPiece,
                              idBool             aIsWriteLog,
                              idBool             aIsReplSenderSend,
                              smLobLocator       aLobLocator)
{
    smcLobDesc* sLobDesc;
    smOID       sRowOID;
 
    IDE_DASSERT( aFixedRowPtr != NULL );
    IDE_DASSERT( aLobColumn != NULL );
    IDE_DASSERT( aPiece != NULL );

    IDE_TEST_CONT( aPieceLen == 0, SKIP_WRITE );

    sLobDesc = (smcLobDesc*)(aFixedRowPtr + aLobColumn->offset);

    IDE_TEST_RAISE( (aOffset + aPieceLen) > sLobDesc->length,
                    range_error);

    if ( SM_VCDESC_IS_MODE_IN(sLobDesc) )
    {
        sRowOID = SMP_SLOT_GET_OID(aFixedRowPtr);

        if ( aIsWriteLog == ID_TRUE )
        {
            // write log
            IDE_TEST( smcLobUpdate::writeLog4LobWrite(
                                          aTrans,
                                          aTable,
                                          aLobLocator,
                                          aLobColumn->colSpace,
                                          SM_MAKE_PID(sRowOID),
                                          ( SM_MAKE_OFFSET(sRowOID)
                                            + aLobColumn->offset
                                            + ID_SIZEOF(smVCDescInMode)
                                            + aOffset ),
                                          aOffset,
                                          aPieceLen,
                                          aPiece,
                                          aIsReplSenderSend )
                      != IDE_SUCCESS );

        }
        else
        {
            /* nothing to do */
        }

        idlOS::memcpy( (aFixedRowPtr + aLobColumn->offset + ID_SIZEOF(smVCDescInMode) + aOffset),
                       aPiece,
                       aPieceLen);

        /*
         * If aIsWriteLog is ID_FALSE, this function is called by
         * inplace update. Therefore aFixedRowPtr is stack variable
         * address. In the case insDirtyPage must not be called.
         */
        
        if ( aIsWriteLog == ID_TRUE )
        {
            IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                          aLobColumn->colSpace,
                                          SMP_SLOT_GET_PID(aFixedRowPtr) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        IDE_TEST( write4OutMode(aTrans,
                                aTable,
                                sLobDesc,
                                aLobColumn->colSpace,
                                aOffset,
                                aPieceLen,
                                aPiece,
                                aIsWriteLog,
                                aIsReplSenderSend,
                                aLobLocator)
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_WRITE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(range_error);
    {
        // BUG-29212 디스크 Lob의 read시 range check를 하지 않습니다.
        // range는 Offset부터 Amount - 1 까지 입니다.
        IDE_SET(ideSetErrorCode(smERR_ABORT_RangeError,
                                aOffset,
                                (aOffset + aPieceLen - 1),
                                0,
                                (sLobDesc->length - 1)));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * OutMode로 aLobDesc의 Lob Piece에 lob 데이타를 기록한다.
 *
 * aTrans            [IN] 작업하는 트랜잭션 객체
 * aTable            [IN] 작업하는 테이블 헤더
 * aLobDesc          [IN] lob 데이타를 저장할 LobDesc 객체
 * aLobSpaceID       [IN] log 데이타를 저장할 SpaceID
 * aOffset           [IN] lob 데이타의 저장 위치
 * aPieceLen         [IN] 새로 입력되는 piece의 크기
 * aPiece            [IN] 새로 입력되는 lob 데이타 piece
 * aIsWriteLog       [IN] 로깅 여부
 * aIsReplSenderSend [IN] replication 작동 여부
 * aLobLocator       [IN] lob locator 객체
 **********************************************************************/
IDE_RC smcLob::write4OutMode(void*           aTrans,
                             smcTableHeader* aTable,
                             smcLobDesc*     aLobDesc,
                             scSpaceID       aLobSpaceID,
                             UInt            aOffset,
                             UInt            aPieceLen,
                             UChar*          aPiece,
                             idBool          aIsWriteLog,
                             idBool          aIsReplSenderSend,
                             smLobLocator    aLobLocator)
{
    smcLPCH* sTargetLPCH;
    UShort   sOffset;
    UShort   sWritableSize;
    UInt     sWrittenSize = 0;

    IDE_DASSERT( aLobDesc != NULL );
    IDE_DASSERT( aPiece != NULL );
    IDE_DASSERT( aPieceLen > 0 );

    sTargetLPCH = findPosition(aLobDesc, aOffset);

    IDE_ASSERT( sTargetLPCH != NULL );

    sOffset = aOffset % SMP_VC_PIECE_MAX_SIZE;

    // write lob piece
    while ( sWrittenSize < aPieceLen )
    {
        sWritableSize = SMP_VC_PIECE_MAX_SIZE - sOffset;
        if ( sWritableSize > (aPieceLen - sWrittenSize) )
        {
            sWritableSize = aPieceLen - sWrittenSize;
        }
        else
        {
            /* nothing to do */
        }

        // sOffset이 만약 Piece 데이타의 크기에 마지막 위치를
        // Offset으로 지정되었을 경우 sWritableSize으로 된다.
        if ( sWritableSize != 0 )
        {
            if ( aIsWriteLog == ID_TRUE )
            {
                // write log
                IDE_TEST( smcLobUpdate::writeLog4LobWrite(
                              aTrans,
                              aTable,
                              aLobLocator,
                              aLobSpaceID,
                              SM_MAKE_PID(sTargetLPCH->mOID),
                              ( SM_MAKE_OFFSET(sTargetLPCH->mOID)
                                + ID_SIZEOF(smVCPieceHeader) + sOffset ),
                              aOffset + sWrittenSize,
                              sWritableSize,
                              aPiece + sWrittenSize,
                              aIsReplSenderSend )
                          != IDE_SUCCESS );

            }
            else
            {
                /* nothing to do */
            }

            IDE_TEST( writePiece( aLobSpaceID,
                                  sTargetLPCH,
                                  sOffset,
                                  sWritableSize,
                                  aPiece + sWrittenSize )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }

        sWrittenSize += sWritableSize;
        sTargetLPCH++;
        sOffset = 0;
    }

    IDE_ASSERT(sWrittenSize == aPieceLen);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * lob write하기 전에 new version에 대한 시작 Offset을 설정한다.
 *
 * aTrans      [IN] 작업하는 트랜잭션 객체
 * aLobViewEnv [IN] 작업하려는 LobViewEnv 객체
 * aLobLocator [IN] 작업하려는 Lob Locator
 * aOffset     [IN] 작업하려는 Lob 데이타의 위치
 * aNewSize    [IN] 새로 입력되는 데이타의 크기
 **********************************************************************/
IDE_RC smcLob::prepare4Write( idvSQL*       aStatistics,
                              void*         aTrans,
                              smLobViewEnv* aLobViewEnv,
                              smLobLocator  aLobLocator,
                              UInt          aOffset,
                              UInt          aNewSize )
{
    SChar*         sNxtFixedRowPtr = NULL;
    smcLobDesc*    sCurLobDesc  = NULL;
    SChar*         sLobInRowPtr = NULL;
    
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aLobViewEnv != NULL );

    /*
     * to update LOB column
     */

    IDE_TEST( getLastVersion(aStatistics,
                             aTrans,
                             aLobViewEnv,
                             &sNxtFixedRowPtr)
              != IDE_SUCCESS );

    /*
     * to set new LOB version
     */
    
    sLobInRowPtr = sNxtFixedRowPtr + aLobViewEnv->mLobCol.offset;
    sCurLobDesc  = (smcLobDesc*)sLobInRowPtr;

    IDE_TEST_RAISE( (aOffset > sCurLobDesc->length), range_error );

    /*
     * set offset
     */
    
    aLobViewEnv->mWriteOffset = aOffset;

    /*
     * for replication.
     */
    
    if ( (aLobViewEnv->mOpenMode == SMI_LOB_READ_WRITE_MODE) &&
         (smcTable::needReplicate(
            (const smcTableHeader*)aLobViewEnv->mTable, aTrans) == ID_TRUE ) )
    {
        // write log for prepare4Write.
        IDE_TEST( smrLogMgr::writeLobPrepare4WriteLogRec(NULL, /* idvSQL* */
                                                         aTrans,
                                                         aLobLocator,
                                                         aOffset,
                                                         aNewSize,
                                                         aNewSize,
                                                         ((const smcTableHeader*)aLobViewEnv->mTable)->mSelfOID )
                  != IDE_SUCCESS);

    }
    else
    {
        /* nothing to do */
    }

    mMemLobStatistics.mPrepareForWrite++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(range_error);
    {
        // BUG-29212 디스크 Lob의 read시 range check를 하지 않습니다.
        // range는 Offset부터 Amount - 1 까지 입니다.
        IDE_SET(ideSetErrorCode(smERR_ABORT_RangeError,
                                aOffset,
                                (aOffset + aNewSize - 1),
                                0,
                                (sCurLobDesc->length - 1)));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Write가 종료되었다. Replication Log를 남긴다.
 *
 *    aStatistics - [IN] 통계 정보
 *    aTrans      - [IN] Transaction
 *    aLobViewEnv - [IN] 자신이 봐야 할 LOB에대한 정보
 *    aLobLocator - [IN] Lob Locator
 **********************************************************************/
IDE_RC smcLob::finishWrite( idvSQL       * aStatistics,
                            void         * aTrans,
                            smLobViewEnv * aLobViewEnv,
                            smLobLocator   aLobLocator )
{
    IDE_ASSERT( aLobViewEnv->mOpenMode == SMI_LOB_READ_WRITE_MODE );

    if ( smcTable::needReplicate( (smcTableHeader*)(aLobViewEnv->mTable),
                                  aTrans )
         == ID_TRUE )
    {
        IDE_TEST( smrLogMgr::writeLobFinish2WriteLogRec(
                                                  aStatistics,
                                                  aTrans,
                                                  aLobLocator,
                                                  ( (smcTableHeader*)(aLobViewEnv->mTable) )->mSelfOID )
                  != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    mMemLobStatistics.mFinishWrite++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * 내부적으로 prepare4WriteInternal를 호출하고 갱신된 LobDesc에 대해
 * 로그를 기록한다. 이는 prepare4Write에서는 prepare4WriteInternal호출후에
 * 갱신된 영역에 대해서 로그를 기록하지만 smiTableBackup::restore에서는
 * prepare4WriteInternal를 바로 호출하기때문에 로깅을 별도로하는 인터페이스가
 * 필요하다.
 *
 * aTrans      [IN] 작업하는 트랜잭션 객체
 * aTable      [IN] 작업하려는 테이블 헤더
 * aLobColumn  [IN] Lob Column Desc정보
 * aLobDesc    [IN] 작업하려는 Lob Description
 * aOffset     [IN] 작업하려는 Lob 데이타의 위치
 * aNewSize    [IN] 새로 입력되는 데이타의 크기
 * aAddOIDFlag [IN] OID LIST에 추가할지 여부
 **********************************************************************/
IDE_RC smcLob::reserveSpaceInternalAndLogging(
                                            void*               aTrans,
                                            smcTableHeader*     aTable,
                                            SChar*              aRow,
                                            const smiColumn*    aLobColumn,
                                            UInt                aOffset,
                                            UInt                aNewSize,
                                            UInt                aAddOIDFlag)
{
    ULong       sLobColBuf[ SMC_LOB_MAX_IN_ROW_STORE_SIZE/ID_SIZEOF(ULong) ];
    /* BUG-30414  LobColumn 복사시 Stack의 Buffer가
       align 돼지 않아 sigbus가 일어납니다. */
    SChar     * sLobInRowPtr;
    smcLobDesc* sNewLobDesc;
    smcLobDesc* sCurLobDesc;
    scPageID    sPageID = SM_NULL_PID;
    void      * sLobColPagePtr;

    sLobInRowPtr = aRow + aLobColumn->offset;
    sCurLobDesc  = (smcLobDesc*)sLobInRowPtr;
    sPageID = SMP_SLOT_GET_PID( aRow );

    // old image를 복사
    if ( SM_VCDESC_IS_MODE_IN(sCurLobDesc) )
    {
        // BUG-30101 데이타가 In-Mode로 저장되었을 경우 데이타는
        // Fixed영역에 저장되어있다. Lob Column Desc 와 함께 같이 복사해둔다
        idlOS::memcpy( sLobColBuf,
                       sLobInRowPtr,
                       sCurLobDesc->length + ID_SIZEOF(smVCDescInMode) );
    }
    else
    {
        // Out Mode로 저장 되어 있을경우 LOB Desc만 복사
        idlOS::memcpy( sLobColBuf, sLobInRowPtr, ID_SIZEOF(smcLobDesc) );
    }

    // BUG-30036 Memory LOB을 ODBC로 Insert 하다 실패하였을 때,
    // 변경하던 LOB Desc를 수정된 채로 Rollback하지 않고 있습니다. 로 인하여
    // 별도의 Dummy Lob Desc로 Prepare 하고 Log를 먼저 기록 한 후에
    // 변경된 LOB Desc를 Data Page에 반영합니다.
    IDE_TEST( reserveSpaceInternal(aTrans,
                                   aTable,
                                   aLobColumn,
                                   sCurLobDesc->mLobVersion + 1,
                                   (smcLobDesc*)sLobColBuf,
                                   aOffset,
                                   aNewSize,
                                   aAddOIDFlag,
                                   ID_TRUE) /* aIsFullWrite */
              != IDE_SUCCESS );

    IDE_ASSERT( smmManager::getPersPagePtr( aLobColumn->colSpace, 
                                            sPageID,
                                            (void**)&sLobColPagePtr )
                == IDE_SUCCESS );

    IDE_TEST( smrUpdate::physicalUpdate(NULL, /* idvSQL* */
                                        aTrans,
                                        aLobColumn->colSpace,
                                        sPageID,
                                        (SChar*)sLobInRowPtr - (SChar*)sLobColPagePtr,
                                        (SChar*)sLobInRowPtr,
                                        ID_SIZEOF(smcLobDesc),
                                        (SChar*)sLobColBuf,
                                        ID_SIZEOF(smcLobDesc),
                                        NULL,
                                        0)
              != IDE_SUCCESS );

    sNewLobDesc = (smcLobDesc*)sLobColBuf;

    // BUG-30036 변경된 Lob Desc 를 Data Page에 반영
    if ( SM_VCDESC_IS_MODE_IN(sNewLobDesc) )
    {
        idlOS::memcpy( sLobInRowPtr,
                       sLobColBuf,
                       ID_SIZEOF(smVCDescInMode) );
    }
    else
    {
        // Out Mode로 저장 된 경우 Lob Desc만 복사한다.
        idlOS::memcpy( sLobInRowPtr,
                       sLobColBuf,
                       ID_SIZEOF(smcLobDesc) );
    }

    IDE_TEST( smmDirtyPageMgr::insDirtyPage( aLobColumn->colSpace,
                                             sPageID)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * lob write하기 전에 new version에 대한 공간 할당하는 실제 작업을 한다.
 *
 * old [LobDesc]--[LPCH#1][LPCH#2][LPCH#3][LPCH#4][LPCH#5]
 *                  | |      |       |        |       | |
 *           [piece#1]| [piece#2] [piece#3] [piece#4] |[piece#5]
 *                  | |    |                   |      | |
 *                  | |    V                   V      | |
 *                  | |[piece#2'][piece#3'][piece#4'] | |
 *                  | V      |       |          |     V |
 * new [LobDesc]--[LPCH#1'][LPCH#2'][LPCH#3'][LPCH#4'][LPCH#5']
 *
 * aOffset이 piece#2에서 시작해서 aOldSize가 piece#4까지 이고,
 * aNewSize가 piece#4'까지 일때, 변경되지 않는 piece#1,5는 LPCH를
 * 복사(LPCH#1->#1', #5->#5')하여 piece#1,5를 공유하게 되고,
 * 변경되는 piece#2',#3',#4'를 새로 할당받아 LPCH에 연결한다.
 * 이때, 변경 시작과 끝 piece#2,#4에 변경 영역이 아닌 값은 새로
 * 할당 받은 piece#2',#4'에 복사해 준다.
 *
 * aTrans      [IN] 작업하는 트랜잭션 객체
 * aTable      [IN] 작업하려는 테이블 헤더
 * aLobColumn  [IN]
 * aLobDesc    [IN] 작업하려는 Lob Description
 * aOffset     [IN] 작업하려는 Lob 데이타의 위치
 * aNewSize    [IN] 새로 입력되는 데이타의 크기
 * aAddOIDFlag [IN] OID LIST에 추가할지 여부
 **********************************************************************/
IDE_RC smcLob::reserveSpaceInternal( void*               aTrans,
                                     smcTableHeader*     aTable,
                                     const smiColumn*    aLobColumn,
                                     ULong               aLobVersion,
                                     smcLobDesc*         aLobDesc,
                                     UInt                aOffset,
                                     UInt                aNewSize,
                                     UInt                aAddOIDFlag,
                                     idBool              aIsFullWrite )
{
    smcLobDesc* sOldLobDesc;
    UInt        sNewLobLen;
    UInt        sNewLPCHCnt;
    UInt        sBeginIdx;
    UInt        sEndIdx;
    ULong       sLobColumnBuffer[SM_PAGE_SIZE / ID_SIZEOF(ULong)];

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );
    IDE_DASSERT( aLobDesc != NULL );

    IDE_TEST_RAISE( (aOffset > aLobDesc->length), range_error );

    /*
     * to backup LOB Descriptor
     */
    
    sOldLobDesc = (smcLobDesc*)sLobColumnBuffer;

    if ( SM_VCDESC_IS_MODE_IN(aLobDesc) )
    {
        /* 데이타가 In-Mode로 저장되었을 경우 데이타는
         * Fixed영역에 저장되어있다. Lob Column Desc 와 함께 같이 복사해둔다 */
        idlOS::memcpy( (SChar*)sOldLobDesc,
                       (SChar*)aLobDesc,
                       ID_SIZEOF(smVCDescInMode) + aLobDesc->length );
    }
    else
    {
        idlOS::memcpy( (SChar*)sOldLobDesc,
                       (SChar*)aLobDesc,
                       ID_SIZEOF(smcLobDesc) );
    }

    /*
     * to set LOB mode & length
     */

    if ( aIsFullWrite == ID_TRUE )
    {
        IDE_ERROR( aOffset == 0 );
        
        sNewLobLen = aOffset + aNewSize;

        if ( sNewLobLen <= aLobColumn->vcInOutBaseSize )
        {
            if ( ( sOldLobDesc->flag & SM_VCDESC_MODE_MASK ) == SM_VCDESC_MODE_OUT )
            {
                /* BUG-46666 : FULL WRITE에서,
                               out -> in 일때, new값(aLobDesc)를 초기화해야 old값이 aging된다. */
                SMC_LOB_DESC_INIT( aLobDesc );
            }

            aLobDesc->flag = SM_VCDESC_MODE_IN;
        }
        else
        {
            aLobDesc->flag = SM_VCDESC_MODE_OUT;
        }
    }
    else
    {
        /*
         * partial write에서 가능한 모드 변환은 다음 3가지 경우이다.
         * out 모드는 out 모든 될 수 있다.
         *
         *  in  -> in
         *  in  -> out
         *  out -> out
         */
        
        sNewLobLen = (aOffset + aNewSize) > sOldLobDesc->length ?
            (aOffset + aNewSize) : sOldLobDesc->length;

        if ( sNewLobLen <= aLobColumn->vcInOutBaseSize )
        {
            if ( SM_VCDESC_IS_MODE_IN(sOldLobDesc) )
            {
                aLobDesc->flag = SM_VCDESC_MODE_IN;
            }
            else
            {
                aLobDesc->flag = SM_VCDESC_MODE_OUT;
            }
        }
        else
        {
            aLobDesc->flag = SM_VCDESC_MODE_OUT;
        }
    }

    aLobDesc->length = sNewLobLen;

    /*
     * to allocate new version space
     */
    
    if ( (aLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT )
    {
        aLobDesc->mLobVersion = aLobVersion;

        sNewLPCHCnt = getLPCHCntFromLength(sNewLobLen);

        IDE_TEST( allocLPCH(aTrans,
                            aTable,
                            aLobDesc,
                            sOldLobDesc,
                            aLobColumn->colSpace,
                            sNewLPCHCnt,
                            aAddOIDFlag,
                            aIsFullWrite)
                  != IDE_SUCCESS );

        IDE_TEST( allocPiece(aTrans,
                             aTable,
                             aLobVersion,
                             aLobDesc,
                             sOldLobDesc,
                             aLobColumn->colSpace,
                             aOffset,
                             aNewSize,
                             sNewLobLen,
                             aAddOIDFlag,
                             aIsFullWrite)
                  != IDE_SUCCESS );

        /* If the LOB mode is converted from in to out, the previsous
         * value must be copied to new piece. */
        if ( (aIsFullWrite != ID_TRUE) &&
             SM_VCDESC_IS_MODE_IN(sOldLobDesc) )
        {
            if ( sOldLobDesc->length > 0 )
            {
                IDE_TEST( copyPiece(aTrans,
                                    aTable,
                                    aLobColumn->colSpace,
                                    sOldLobDesc,    /* aSrcLobDesc */
                                    0,              /* aSrcOffset */
                                    aLobDesc,       /* aDstLobDesc */
                                    0,              /* aDstOffset */
                                    sOldLobDesc->length)
                          != IDE_SUCCESS );
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
    }
    else
    {
        /* nothing to do */
    }


    /*
     * to remove old version (LPCH & LOB Piece)
     */

    if ( ( (sOldLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT) &&
         ( sOldLobDesc->mLPCHCount > 0 ) )
    { 
        if ( aIsFullWrite == ID_TRUE )
        {
            sBeginIdx = 0;
            sEndIdx = sOldLobDesc->mLPCHCount - 1;
        }
        else
        {
            sBeginIdx = aOffset / SMP_VC_PIECE_MAX_SIZE;

            if ( (aOffset + aNewSize) <= sOldLobDesc->length )
            {
                IDE_ERROR( (aOffset + aNewSize) > 0);
                sEndIdx = (aOffset + aNewSize - 1) / SMP_VC_PIECE_MAX_SIZE;
            }
            else
            {
                sEndIdx = sOldLobDesc->mLPCHCount - 1;
            }
        }

        IDE_TEST( removeOldLPCH(aTrans,
                                aTable,
                                aLobColumn,
                                sOldLobDesc,
                                aLobDesc,
                                sBeginIdx,
                                sEndIdx)
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(range_error);
    {
        // BUG-29212 디스크 Lob의 read시 range check를 하지 않습니다.
        // range는 Offset부터 Amount - 1 까지 입니다.
        IDE_SET(ideSetErrorCode(smERR_ABORT_RangeError,
                                aOffset,
                                (aOffset + aNewSize - 1),
                                0,
                                (aLobDesc->length - 1)));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * 새로운 lob piece를 할당한다.
 *
 * aTrans         [IN] 작업하는 트랜잭션 객체
 * aTable         [IN] 작업하려는 테이블 헤더
 * aLobSpaceID [IN] Lob Column데이터가 기록되는 Tablespace의 ID
 * aFirstLPCH     [IN] 전체 lob 데이터의 시작 LPCH
 * aStartLPCH     [IN] 변경 시작하는 LPCH
 * aNxtPieceOID   [IN] 새로 할당 받을 lob piece의 가장 마지막의 next oid
 * aNewSlotSize   [IN] 새로 할당 받을 slot들 중 마지막 slot의 크기
 * aAddOIDFlag    [IN] OID LIST에 추가할지 여부
 **********************************************************************/
IDE_RC smcLob::allocPiece(void*           aTrans,
                          smcTableHeader* aTable,
                          ULong           aLobVersion,
                          smcLobDesc*     aLobDesc,
                          smcLobDesc*     aOldLobDesc,
                          scSpaceID       aLobSpaceID,
                          UInt            aOffset,
                          UInt            aNewSize,
                          UInt            aNewLobLen,
                          UInt            aAddOIDFlag,
                          idBool          aIsFullWrite)
{
    UInt             i;
    UInt             sBeginIdx;
    UInt             sEndIdx;
    UInt             sLPCHCnt;
    UInt             sOldLPCHCnt;
    UInt             sPieceSize;
    smOID            sNxtPieceOID;
    smOID            sNewPieceOID = SM_NULL_OID;
    SChar*           sNewPiecePtr = NULL;
    smcLPCH*         sTargetLPCH;
    smVCPieceHeader* sCurVCPieceHeader;
    smVCPieceHeader* sSrcVCPieceHeader;

    IDE_ASSERT( aTrans   != NULL );
    IDE_ASSERT( aTable   != NULL );
    IDE_ASSERT( aLobDesc != NULL );
    IDE_ASSERT( aLobDesc->mFirstLPCH != NULL );
    IDE_ASSERT( (aLobDesc->flag & SM_VCDESC_MODE_OUT) == SM_VCDESC_MODE_OUT );

    IDE_ERROR( (aOffset + aNewSize) > 0 );

    sBeginIdx = aOffset / SMP_VC_PIECE_MAX_SIZE;
    sEndIdx   = (aOffset + aNewSize - 1) / SMP_VC_PIECE_MAX_SIZE;

    sLPCHCnt  = aLobDesc->mLPCHCount;

    /*
     * 공간 낭비를 막기위해 mLPCHCnt가 1인 경우에 대해서는 가변 Piece를 할당한다.
     * 1보다 큰 경우 모든 Piece는 SMP_VC_PIECE_MAX_SIZE 크기로 고정이다.
     */
    
    if ( sLPCHCnt <= 1 )
    {
        IDE_ERROR( aNewLobLen <= SMP_VC_PIECE_MAX_SIZE );
        
        sPieceSize = aNewLobLen;
    }
    else
    {
        sPieceSize = SMP_VC_PIECE_MAX_SIZE;
    }

    if ( (aOldLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT )
    {
        sOldLPCHCnt = aOldLobDesc->mLPCHCount;
    }
    else
    {
        sOldLPCHCnt = 0;
    }

    IDE_ERROR( sBeginIdx < aLobDesc->mLPCHCount );
    IDE_ERROR( sEndIdx   < aLobDesc->mLPCHCount );

    /*
     * to allocate piece.
     */

    for ( i = sEndIdx; i >= sBeginIdx; i-- )
    {
        if ( (i + 1) < sLPCHCnt )
        {
            sNxtPieceOID = aLobDesc->mFirstLPCH[i+1].mOID;
        }
        else
        {
            sNxtPieceOID = SM_NULL_OID;
        }

        if ( (i < sOldLPCHCnt) && (aIsFullWrite != ID_TRUE) )
        {
            sSrcVCPieceHeader = (smVCPieceHeader*)(aOldLobDesc->mFirstLPCH[i].mPtr);

            if ( i > 0 )
            {
                IDE_ERROR( sSrcVCPieceHeader->length == SMP_VC_PIECE_MAX_SIZE );
            }
            else
            {
                /* 첫번째 LPCH만 가변 일 수 있다. */
                IDE_ERROR( sSrcVCPieceHeader->length <= SMP_VC_PIECE_MAX_SIZE );
            }

            if ( (aLobVersion != aOldLobDesc->mFirstLPCH[i].mLobVersion) ||
                 (sSrcVCPieceHeader->length < SMP_VC_PIECE_MAX_SIZE) )
            {
                IDE_TEST( smpVarPageList::allocSlot(aTrans,
                                                    aLobSpaceID,
                                                    aTable->mSelfOID,
                                                    aTable->mVar.mMRDB,
                                                    sPieceSize,
                                                    sNxtPieceOID,
                                                    &sNewPieceOID,
                                                    &sNewPiecePtr)
                          != IDE_SUCCESS );

                aLobDesc->mFirstLPCH[i].mLobVersion = aLobVersion;
                aLobDesc->mFirstLPCH[i].mOID        = sNewPieceOID;
                aLobDesc->mFirstLPCH[i].mPtr        = sNewPiecePtr;

                /* 새로 할당한 lob piece에 대하여 version list 추가 */
                if ( SM_INSERT_ADD_OID_IS_OK(aAddOIDFlag) )
                {
                    IDE_TEST( smLayerCallback::addOID( aTrans,
                                                       aTable->mSelfOID,
                                                       sNewPieceOID,
                                                       aLobSpaceID,
                                                       SM_OID_NEW_VARIABLE_SLOT )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* nothing to do */
                }

                if ( i == sBeginIdx )
                {
                    IDE_ERROR( sPieceSize >= sSrcVCPieceHeader->length );
                    
                    IDE_TEST( copyPiece(aTrans,
                                        aTable,
                                        aLobSpaceID,
                                        aOldLobDesc,
                                        (sBeginIdx * SMP_VC_PIECE_MAX_SIZE),
                                        aLobDesc,
                                        (sBeginIdx * SMP_VC_PIECE_MAX_SIZE),
                                        sSrcVCPieceHeader->length)
                              != IDE_SUCCESS );
                }

                if ( i == sEndIdx )
                {
                    IDE_ERROR( sPieceSize >= sSrcVCPieceHeader->length );
                    
                    IDE_TEST( copyPiece(aTrans,
                                        aTable,
                                        aLobSpaceID,
                                        aOldLobDesc,
                                        (sEndIdx * SMP_VC_PIECE_MAX_SIZE),
                                        aLobDesc,
                                        (sEndIdx * SMP_VC_PIECE_MAX_SIZE),
                                        sSrcVCPieceHeader->length)
                              != IDE_SUCCESS );
                }
            }
            else
            {
                sTargetLPCH = aLobDesc->mFirstLPCH + i;
                
                sCurVCPieceHeader =
                            (smVCPieceHeader*)(sTargetLPCH)->mPtr;

                if ( sCurVCPieceHeader->nxtPieceOID != sNxtPieceOID )
                {
                    IDE_TEST( smrUpdate::setNxtOIDAtVarRow(
                                          NULL, /* idvSQL* */
                                          aTrans,
                                          aLobSpaceID,
                                          aTable->mSelfOID,
                                          sTargetLPCH->mOID,
                                          sCurVCPieceHeader->nxtPieceOID,
                                          sNxtPieceOID )
                              != IDE_SUCCESS );

                    sCurVCPieceHeader->nxtPieceOID = sNewPieceOID;

                    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                          aLobSpaceID,
                                          SM_MAKE_PID( sTargetLPCH->mOID ))
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            IDE_TEST( smpVarPageList::allocSlot(aTrans,
                                                aLobSpaceID,
                                                aTable->mSelfOID,
                                                aTable->mVar.mMRDB,
                                                sPieceSize,
                                                sNxtPieceOID,
                                                &sNewPieceOID,
                                                &sNewPiecePtr)
                      != IDE_SUCCESS );

            aLobDesc->mFirstLPCH[i].mLobVersion = aLobVersion;
            aLobDesc->mFirstLPCH[i].mOID        = sNewPieceOID;
            aLobDesc->mFirstLPCH[i].mPtr        = sNewPiecePtr;

            /* 새로 할당한 lob piece에 대하여 version list 추가 */
            if ( SM_INSERT_ADD_OID_IS_OK(aAddOIDFlag) )
            {
                IDE_TEST( smLayerCallback::addOID( aTrans,
                                                   aTable->mSelfOID,
                                                   sNewPieceOID,
                                                   aLobSpaceID,
                                                   SM_OID_NEW_VARIABLE_SLOT )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }

        if( i == 0 )
        {
            break;
        }
    }

    /*
     * to set fstPieceOID
     */
    
    if ( aLobDesc->mLPCHCount > 0 )
    {
        aLobDesc->fstPieceOID = aLobDesc->mFirstLPCH[0].mOID;
    }

    /*
     * to set nxtPieceOID
     */

    if ( sBeginIdx > 0 )
    {
        sNxtPieceOID = aLobDesc->mFirstLPCH[sBeginIdx].mOID;
        
        sTargetLPCH = aLobDesc->mFirstLPCH + (sBeginIdx - 1);

        sCurVCPieceHeader = (smVCPieceHeader*)(sTargetLPCH)->mPtr;

        if ( sCurVCPieceHeader->nxtPieceOID != sNxtPieceOID )
        {
            IDE_TEST( smrUpdate::setNxtOIDAtVarRow(
                                              NULL, /* idvSQL* */
                                              aTrans,
                                              aLobSpaceID,
                                              aTable->mSelfOID,
                                              (sTargetLPCH)->mOID,
                                              sCurVCPieceHeader->nxtPieceOID,
                                              sNxtPieceOID )
                      != IDE_SUCCESS );


            sCurVCPieceHeader->nxtPieceOID = sNxtPieceOID;

            IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                              aLobSpaceID,
                                              SM_MAKE_PID( (sTargetLPCH)->mOID ))
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**********************************************************************
 * 새로 할당받은 lob piece영역에서 변경되지 않는 영역을 복사해 온다.
 *
 * aTrans                [IN] 작업하는 트랜잭션 객체
 * aTable                [IN] 작업하려는 테이블 헤더
 * aSourceLobDesc        [IN] 작업하려는 Lob 데이타의 위치
 * aLobSpaceID           [IN] Lob Piece를 저장하는 SpaceID
 * aSourceOffset         [IN] 변경 작업을 하고자 하는 부분의 크기
 * aDstLobDesc           [IN] 새로 입력되는 데이타의 크기
 * aDstOffset            [IN] 변경 시작하는 LPCH
 * aLength               [IN] aStartLPCH에서 변경되지 않는 크기
 **********************************************************************/
IDE_RC smcLob::copyPiece( void           * aTrans,
                          smcTableHeader * aTable,
                          scSpaceID        aLobSpaceID,
                          smcLobDesc     * aSrcLobDesc,
                          UInt             aSrcOffset,
                          smcLobDesc     * aDstLobDesc,
                          UInt             aDstOffset,
                          UInt             aLength )
{
    UChar*   sSrcPiecePtr;
    smcLPCH* sSrcLPCH;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );
    IDE_DASSERT( aSrcLobDesc != NULL );
    IDE_DASSERT( aDstLobDesc != NULL );

    // source point 획득
    if ( SM_VCDESC_IS_MODE_IN(aSrcLobDesc) )
    {
        sSrcPiecePtr = (UChar*)aSrcLobDesc + ID_SIZEOF(smVCDescInMode) + aSrcOffset;
    }
    else
    {
        sSrcLPCH = findPosition( aSrcLobDesc, aSrcOffset );

        sSrcPiecePtr = (UChar*)(sSrcLPCH->mPtr) + ID_SIZEOF(smVCPieceHeader)
                       + (aSrcOffset % SMP_VC_PIECE_MAX_SIZE);
    }

    // destination에 write
    if ( SM_VCDESC_IS_MODE_IN(aDstLobDesc) )
    {
        idlOS::memcpy( ((SChar*)aDstLobDesc + ID_SIZEOF(smVCDescInMode) + aDstOffset),
                       sSrcPiecePtr,
                       aLength);
    }
    else
    {
        IDE_TEST( write4OutMode(aTrans,
                                aTable,
                                aDstLobDesc,
                                aLobSpaceID,
                                aDstOffset,
                                aLength,
                                sSrcPiecePtr,
                                ID_TRUE,  //aIsWriteLog
                                ID_FALSE, //aIsReplSenderSend
                                0)        //aLobLocator
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * 새로운 LPCH를 할당한다.
 *
 * aTrans         [IN]  작업하는 트랜잭션 객체
 * aTable         [IN]  작업하려는 테이블 헤더
 * aLobDesc       [IN]  작업하려는 Lob Description
 * aLobSpaceID    [IN] Lob Column데이터가 기록되는 Tablespace의 ID
 * aAddOIDFlag    [IN] OID LIST에 추가할지 여부
 **********************************************************************/
IDE_RC smcLob::allocLPCH( void*              aTrans,
                          smcTableHeader*    aTable,
                          smcLobDesc*        aLobDesc,
                          smcLobDesc*        aOldLobDesc,
                          scSpaceID          aLobSpaceID,
                          UInt               aNewLPCHCnt,
                          UInt               aAddOIDFlag,
                          idBool             aIsFullWrite )
{
    smcLPCH   * sNewLPCH    = NULL;
    UInt        sCopyCnt    = 0;
    UInt        sState      = 0;

    if ( aNewLPCHCnt > 0 )
    {
        /* smcLob_allocLPCH_calloc_NewLPCH.tc */
        IDU_FIT_POINT("smcLob::allocLPCH::calloc::NewLPCH");
        IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SMC,
                                    1,
                                    (ULong)ID_SIZEOF(smcLPCH) * aNewLPCHCnt,
                                    (void**)&sNewLPCH)
                  != IDE_SUCCESS );
        sState = 1;
        /*
         * to copy LPCH from old LPCH if the LOB is out-mode.
         */

        if ( aIsFullWrite != ID_TRUE )
        {
            if ( (aOldLobDesc->flag & SM_VCDESC_MODE_MASK)
                 == SM_VCDESC_MODE_OUT )
            {
                if ( aOldLobDesc->mLPCHCount > aNewLPCHCnt )
                {
                    sCopyCnt = aNewLPCHCnt;
                }
                else
                {
                    sCopyCnt = aOldLobDesc->mLPCHCount;
                }

                if ( sCopyCnt > 0 )
                {
                    idlOS::memcpy( sNewLPCH,
                                   aOldLobDesc->mFirstLPCH,
                                   (size_t)ID_SIZEOF(smcLPCH) * sCopyCnt );
                }
            }
        }

        /*
         * to add new LPCH OID (for rollback)
         */

        if ( SM_INSERT_ADD_LPCH_OID_IS_OK(aAddOIDFlag) )
        {
            IDE_TEST( smLayerCallback::addOID( aTrans,
                                               aTable->mSelfOID,
                                               (smOID)sNewLPCH,
                                               aLobSpaceID,
                                               SM_OID_NEW_LPCH )
                      != IDE_SUCCESS );
        }
        else
        {
            /* BUG-42411 add column이 실패로 기존 테이블을 restore할때 할당한 LPCH를
               ager가 지워버리지 않도록 OID를 add 하지 않습니다.
               (undo시 호출되는 restore 에서만 SM_INSERT_ADD_LPCH_OID_NO 설정) */
        }
    }

    aLobDesc->mFirstLPCH = sNewLPCH;
    aLobDesc->mLPCHCount = aNewLPCHCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sNewLPCH ) == IDE_SUCCESS );
            sNewLPCH = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * refine시에 LPCH를 rebuild한다.
 *
 * aTableOID     - [IN] Table의 OID
 * aArrLobColumn - [IN] Lob Column Array
 * aLobColumnCnt - [IN] Lob Column Count
 * aFixedRow     - [IN] 작업하려는 Fixed Row
 **********************************************************************/
IDE_RC smcLob::rebuildLPCH( smOID       /*aTableOID*/,
                            smiColumn **aArrLobColumn,
                            UInt        aLobColumnCnt,
                            SChar*      aFixedRow )
{
    UInt            i;
    smiColumn    ** sCurColumn;
    smiColumn    ** sFence;
    smcLobDesc    * sLobDesc;
    smcLPCH       * sNewLPCH;
    smOID           sCurOID;
    void          * sCurPtr;

    IDE_DASSERT(aFixedRow != NULL);

    sCurColumn = aArrLobColumn;
    sFence     = aArrLobColumn + aLobColumnCnt;

    for ( ; sCurColumn < sFence; sCurColumn++ )
    {
        IDE_ERROR_MSG( ((*sCurColumn)->flag & SMI_COLUMN_TYPE_MASK)
                       == SMI_COLUMN_TYPE_LOB,
                       "Flag : %"ID_UINT32_FMT,
                       (*sCurColumn)->flag );

        sLobDesc = (smcLobDesc*)(aFixedRow + (*sCurColumn)->offset);

        if ( (sLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT )
        {
            /* trim으로 인해 mLPCHCount가 0일 수 있다. */
            if ( sLobDesc->mLPCHCount > 0 )
            {
                /* smcLob_rebuildLPCH_malloc_NewLPCH.tc */
                IDU_FIT_POINT("smcLob::rebuildLPCH::malloc::NewLPCH");
                IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMC,
                                             ID_SIZEOF(smcLPCH) * sLobDesc->mLPCHCount,
                                             (void**)&sNewLPCH )
                          != IDE_SUCCESS );

                sCurOID = sLobDesc->fstPieceOID;

                for ( i = 0 ; i < sLobDesc->mLPCHCount ; i++ )
                {
                    IDE_ERROR_MSG( smmManager::getOIDPtr( (*sCurColumn)->colSpace, 
                                                          sCurOID,
                                                          (void**)&sCurPtr )
                                   == IDE_SUCCESS,
                                   "OID : %"ID_UINT64_FMT,
                                   sCurOID );

                    sNewLPCH[i].mOID = sCurOID;
                    sNewLPCH[i].mPtr = sCurPtr;

                    sCurOID  = ((smVCPieceHeader*)sCurPtr)->nxtPieceOID;
                }

                IDE_ERROR_MSG( sCurOID == SM_NULL_OID,
                               "OID : %"ID_UINT64_FMT,
                               sCurOID );
                IDE_ERROR_MSG( (SMP_VC_PIECE_MAX_SIZE * sLobDesc->mLPCHCount)
                               >= sLobDesc->length,
                               "PieceCount : %"ID_UINT32_FMT
                               "Length     : %"ID_UINT32_FMT,
                               sLobDesc->length,
                               sLobDesc->length );

                sLobDesc->mFirstLPCH = sNewLPCH;
            }
            else
            {
                sLobDesc->mFirstLPCH = NULL;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * lobCursor가 가르키고 있는 메모리 LOB의 길이를 return한다.
 *
 * aTrans      [IN] 작업하는 트랜잭션 객체
 * aLobViewEnv [IN] 작업하려는 LobViewEnv 객체
 * aLobLen     [OUT] LOB 데이타 길이
 * aLobMode    [OUT] LOB 저장 모드 ( In/Out )
 **********************************************************************/
IDE_RC smcLob::getLobInfo( idvSQL*        /*aStatistics*/,
                           void*          aTrans,
                           smLobViewEnv*  aLobViewEnv,
                           UInt*          aLobLen,
                           UInt*          aLobMode,
                           idBool*        aIsNullLob )
{
    smcLobDesc*    sLobDesc;
    SChar*         sCurFixedRowPtr;

    IDE_DASSERT(aLobViewEnv != NULL);
    IDE_DASSERT(aLobLen != NULL);

    IDE_TEST(getViewRowPtr(aTrans,
                           aLobViewEnv,
                           &sCurFixedRowPtr) != IDE_SUCCESS);

    sLobDesc = (smcLobDesc*)(sCurFixedRowPtr
                             + aLobViewEnv->mLobCol.offset);

    *aLobLen = sLobDesc->length;

    if ( (sLobDesc->flag & SM_VCDESC_NULL_LOB_MASK) == SM_VCDESC_NULL_LOB_TRUE )
    {
        *aIsNullLob = ID_TRUE;
    }
    else
    {
        *aIsNullLob = ID_FALSE;
    }

    if ( aLobMode != NULL )
    {
        *aLobMode = sLobDesc->flag & SMI_COLUMN_MODE_MASK ;
    }
    else
    {
        /* nothing to do */
    }

    mMemLobStatistics.mGetLobInfo++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Lob Cursor가 봐야할 Row Pointer를 찾는다.
 *
 * aTrans      [IN] 작업하는 트랜잭션 객체
 * aLobViewEnv [IN] 작업하려는 LobViewEnv 객체
 * aRowPtr     [OUT] 읽거나 Update해야할 Row Pointer
 **********************************************************************/
IDE_RC smcLob::getViewRowPtr( void*         aTrans,
                              smLobViewEnv* aLobViewEnv,
                              SChar**       aRowPtr )
{
    SChar*         sReadFixedRowPtr;

    smpSlotHeader* sCurFixedSlotHeader;
    smpSlotHeader* sNxtFixedSlotHeader;
    smSCN          sSCN;
    smTID          sTID;

    sReadFixedRowPtr    = (SChar*)(aLobViewEnv->mRow);
    sCurFixedSlotHeader = (smpSlotHeader*)sReadFixedRowPtr;

    switch ( aLobViewEnv->mOpenMode )
    {
        case SMI_LOB_READ_WRITE_MODE:
            {
                // fixed row의 읽을 버전을 선택한다.
                while ( SMP_SLOT_HAS_VALID_NEXT_OID( sCurFixedSlotHeader ) )
                {
                    IDE_ASSERT( smmManager::getOIDPtr( 
                                    aLobViewEnv->mLobCol.colSpace,
                                    SMP_SLOT_GET_NEXT_OID(sCurFixedSlotHeader),
                                    (void**)&sNxtFixedSlotHeader )
                                == IDE_SUCCESS );

                    SMX_GET_SCN_AND_TID( sNxtFixedSlotHeader->mCreateSCN, sSCN, sTID );

                    if ( ( SM_SCN_IS_INFINITE( sSCN ) ) &&
                         ( sTID == smLayerCallback::getTransID(aTrans) ) )
                    {
                        if ( SM_SCN_IS_EQ( &sSCN, &(aLobViewEnv->mInfinite) ) )
                        {
                            // 같은 Lob Cursor로 update한 Next Version이라면 보여준다.
                            sReadFixedRowPtr = (SChar*)sNxtFixedSlotHeader;
                        }
                        else
                        {
                            /* nothing to do */
                        }
                    }
                    else
                    {
                        /* nothing to do */
                        /* 다른 TX 가 수정중인 Next Version 은 보지 않는다. */
                    }

                    sCurFixedSlotHeader = sNxtFixedSlotHeader;
                }

                sCurFixedSlotHeader = (smpSlotHeader*)sReadFixedRowPtr;

                IDE_TEST_RAISE( SM_SCN_IS_DELETED(sCurFixedSlotHeader->mCreateSCN),
                                lob_cursor_too_old);                
            }
            break;
            
        case SMI_LOB_READ_LAST_VERSION_MODE:
            {
                // DEAD Code 인듯.. 설정하는곳은 없지만.. 그냥 두자.
                IDE_DASSERT(0);

                // fixed row의 읽을 버전을 선택한다.
                while ( SMP_SLOT_HAS_VALID_NEXT_OID( sCurFixedSlotHeader ) )
                {
                    IDE_ASSERT( smmManager::getOIDPtr( 
                                    aLobViewEnv->mLobCol.colSpace,
                                    SMP_SLOT_GET_NEXT_OID(sCurFixedSlotHeader),
                                    (void**)&sNxtFixedSlotHeader )
                                == IDE_SUCCESS );

                    sCurFixedSlotHeader = sNxtFixedSlotHeader;
                }

                IDE_TEST_RAISE( SM_SCN_IS_DELETED(sCurFixedSlotHeader->mCreateSCN),
                                lob_cursor_too_old);

                sReadFixedRowPtr = (SChar*)sCurFixedSlotHeader;
            }
            break;
            
        default:
            break;
    }

    *aRowPtr = sReadFixedRowPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION(lob_cursor_too_old);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_LobCursorTooOld));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * aRowPtr을 aLobViewEnv가 가리키는 Table에 삽입한다.
 *
 * aTrans      [IN] 작업하는 트랜잭션 객체
 * aLobViewEnv [IN] 작업하려는 LobViewEnv 객체
 * aRowPtr     [IN] Insert해야할 Row Pointer
 **********************************************************************/
IDE_RC smcLob::insertIntoIdx(idvSQL*       aStatistics,
                             void*         aTrans,
                             smLobViewEnv* aLobViewEnv,
                             SChar*        aRowPtr)
{
    UInt             sIndexCnt;
    smnIndexHeader*  sIndexCursor;
    UInt             i;
    idBool           sUniqueCheck = ID_FALSE;
    smcTableHeader*  sTable;
    SChar*           sNullPtr;

    sTable = (smcTableHeader*)aLobViewEnv->mTable;
    sIndexCnt = smcTable::getIndexCount(aLobViewEnv->mTable);
    IDE_ASSERT( smmManager::getOIDPtr( sTable->mSpaceID, 
                                       sTable->mNullOID,
                                       (void**)&sNullPtr )
                == IDE_SUCCESS );

    for ( i = 0 ; i < sIndexCnt; i++ )
    {
        sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( aLobViewEnv->mTable, i);

        if ( (sIndexCursor->mFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE )
        {
            if ( ( (sIndexCursor->mFlag & SMI_INDEX_UNIQUE_MASK) ==
                   SMI_INDEX_UNIQUE_ENABLE ) ||
                 ( (sIndexCursor->mFlag & SMI_INDEX_LOCAL_UNIQUE_MASK) ==
                   SMI_INDEX_LOCAL_UNIQUE_ENABLE) )
            {
                sUniqueCheck = ID_TRUE;
            }
            else
            {
                /* nothing to do */
            }

            IDE_TEST( sIndexCursor->mInsert( aStatistics,
                                             aTrans,
                                             aLobViewEnv->mTable,
                                             sIndexCursor,
                                             aLobViewEnv->mInfinite,
                                             aRowPtr,
                                             sNullPtr,
                                             sUniqueCheck,
                                             aLobViewEnv->mSCN,
                                             NULL,
                                             NULL,
                                             ID_ULONG_MAX, /* aInsertWaitTime */ 
                                             ID_FALSE )    /* aForbiddenToRetry */ 
                      != IDE_SUCCESS );
        }//if
        else
        {
            /* nothing to do */
        }
    }//for i

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Lob Piece를 기록한다.
 **********************************************************************/
IDE_RC smcLob::writePiece( scSpaceID    aLobPieceSpaceID,
                           smcLPCH    * aTargetLPCH,
                           UShort       aOffset,
                           UShort       aMount,
                           UChar      * aBuffer )
{
    idlOS::memcpy( (UChar*)(aTargetLPCH->mPtr) + ID_SIZEOF(smVCPieceHeader) + aOffset,
                   aBuffer,
                   aMount );

    return smmDirtyPageMgr::insDirtyPage(aLobPieceSpaceID,
                                         SM_MAKE_PID(aTargetLPCH->mOID));
}

/**********************************************************************
 * 지정된 길이를 저장하기 위한 LPCH의 Count를 구한한다.
 **********************************************************************/
UInt smcLob::getLPCHCntFromLength( UInt aLength )
{
    UInt sLPCHCnt = 0;

    if ( aLength > 0 )
    {
        sLPCHCnt = aLength / SMP_VC_PIECE_MAX_SIZE;
        
        if ( (aLength % SMP_VC_PIECE_MAX_SIZE) > 0 )
        {
            sLPCHCnt++;
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
    return sLPCHCnt;
}

/**********************************************************************
 * LOB 컬럼을 update 한다.
 **********************************************************************/
IDE_RC smcLob::getLastVersion(idvSQL*          aStatistics,
                              void*            aTrans,
                              smLobViewEnv*    aLobViewEnv,
                              SChar**          aNxtFixedRowPtr)
{
    smSCN          sSCN;
    smTID          sTID;
    SChar*         sNxtFixedRowPtr = NULL;
    smpSlotHeader* sCurFixedSlotHeader = NULL;
    SChar*         sUpdateFixedRowPtr = NULL;
    smpSlotHeader* sNxtFixedSlotHeader = NULL ;
    
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aLobViewEnv != NULL );

    sCurFixedSlotHeader = (smpSlotHeader*)aLobViewEnv->mRow;

    /*
     * to update LOB column
     */ 

    sUpdateFixedRowPtr = NULL;

    IDE_TEST_RAISE( SM_SCN_IS_DELETED(sCurFixedSlotHeader->mCreateSCN),
                    lob_cursor_too_old );
    while (1)
    {
        if ( SM_SCN_IS_FREE_ROW( sCurFixedSlotHeader->mLimitSCN ) )
        {

            if ( /*현재 Update할 Row를 트랜잭션의 다른 Lob Cursor가 보고있다.*/
                 ( smLayerCallback::getMemLobCursorCnt( aTrans,
                                                        aLobViewEnv->mLobCol.id,
                                                        aLobViewEnv->mRow ) 
                   <= 1 ) &&
                 /*현재 Row를 다른 Transaction이 Update한 Row이다.*/
                 ( SM_SCN_IS_EQ( &(sCurFixedSlotHeader->mCreateSCN),
                                 &(aLobViewEnv->mInfinite)) ) )
            {

                // 현재 Update할 Row를 다른 Lob Cursor가 보고있지 않고
                // 현재 Row을 같은 Lob Cursor로 update 했으면 
                // (lob copy할때는)새로 insert된 row를 바로 사용한다.
                sNxtFixedRowPtr = (SChar*)aLobViewEnv->mRow;
                break;
            }
            else
            {
                sUpdateFixedRowPtr = (SChar*)(aLobViewEnv->mRow);
            }
        }
        else
        {
            sNxtFixedSlotHeader = sCurFixedSlotHeader;

            while ( SMP_SLOT_HAS_VALID_NEXT_OID( sNxtFixedSlotHeader ) )
            {
                /* update는 가장 최신 버전에 대해서 수행이 된다.*/
                IDE_ASSERT( smmManager::getOIDPtr( 
                                            aLobViewEnv->mLobCol.colSpace,
                                            SMP_SLOT_GET_NEXT_OID(sNxtFixedSlotHeader),
                                            (void**)&sNxtFixedSlotHeader )
                            == IDE_SUCCESS );
            }

            if ( SM_SCN_IS_FREE_ROW( sNxtFixedSlotHeader->mLimitSCN ) )
            {
                SMX_GET_SCN_AND_TID( sNxtFixedSlotHeader->mCreateSCN, sSCN, sTID );

                /* check whether the record is already modified. */
                IDE_ASSERT( SM_SCN_IS_INFINITE( sSCN ) );

                // 동일 트랜잭션에 의한 다른 작업이 있었다면...
                IDE_ASSERT( sTID == smLayerCallback::getTransID( aTrans ) );

                // 이 LOB Cursor에 의해 Update되기전에 다른 LOB
                // Cursor에 의해 Update가 발생하였다.
                if ( smLayerCallback::getLogTypeFlagOfTrans( aTrans )
                     == SMR_LOG_TYPE_NORMAL )
                {
                    /* BUG-16003:
                     * Sender가 하나의 Row에 대해서 같은 Table Cursor로
                     * LOB Cursor를 두개 열면 두개의 LOB Cursor는 같은
                     * Infinite SCN을 가진다. 하지만 Receiver단에서는
                     * 각기 다른 Table Cursor로 LOB Cursor가 두개열리게
                     * 되어서 다른 Infinite SCN을 가지게 되어 Sender에서
                     * 성공한 Prepare가 Receiver단에서는 Too Old에러가
                     * 발생한다. 위 현상을 방지하기 위해 Normal Transaction
                     * 일 경우에만 아래 체크를 수행한다. Replication은 성공한
                     * 연산일 경우에만 Log가 기록되기 때문에 아래 Validate는
                     * 무시해도 된다.*/
                    IDE_TEST_RAISE( !SM_SCN_IS_EQ(
                                                &sSCN,
                                                &(aLobViewEnv->mInfinite)),
                                    lob_cursor_too_old);
                }
                else
                {
                    /* nothing to do */
                }

                if ( smLayerCallback::getMemLobCursorCnt( aTrans,
                                                          aLobViewEnv->mLobCol.id,
                                                          sNxtFixedSlotHeader )
                     != 0 )
                {
                    sUpdateFixedRowPtr = (SChar*)sNxtFixedSlotHeader;
                }
                else
                {
                    // 같은 Lob Cursor로 update한 Next Version은 공유한다.
                    // BUGBUG - 이때 partial rollback 복잡해짐..
                    sNxtFixedRowPtr = (SChar*)sNxtFixedSlotHeader;
                    break;
                }
            }
            else
            {
                if ( SM_SCN_IS_LOCK_ROW(sNxtFixedSlotHeader->mLimitSCN) )
                {
                    IDE_ASSERT( sNxtFixedSlotHeader == sCurFixedSlotHeader );

                    sUpdateFixedRowPtr = (SChar*)(aLobViewEnv->mRow);
#ifdef DEBUG
                    SMX_GET_SCN_AND_TID( sNxtFixedSlotHeader->mLimitSCN, sSCN, sTID );
                    IDE_ASSERT( sTID == smLayerCallback::getTransID( aTrans ) );
#endif
                }
                else
                {
                    /*
                     * delete row연산이 수행되었다.
                     * 이 경우에는 이미 delete된 row의 mNext에 새로운 버전이 달리게 된다.
                     * 이러한 일이 발생해서는 안된다.
                     */
                    IDE_ASSERT(0);
                }
            }
        }// else

        IDE_ASSERT(sUpdateFixedRowPtr != NULL);

        // Global변수로 선언된 SC_NULL_GRID가 NULL GRID가 맞는지 재확인
        // 메모리 긁을 경우 여기서 ASSERT걸림
        IDE_ASSERT( SC_GRID_IS_NULL( SC_NULL_GRID ) );

        // fixed row에 대한 update version 수행
        IDE_TEST( smcRecord::updateVersionInternal( aTrans,
                                                    aLobViewEnv->mSCN,
                                                    (smcTableHeader*)(aLobViewEnv->mTable),
                                                    sUpdateFixedRowPtr,
                                                    &sNxtFixedRowPtr,
                                                    NULL,                     // aRetSlotRID
                                                    NULL,                     // aColumnList
                                                    NULL,                     // aValueList
                                                    NULL,                     // DML Retry Info
                                                    aLobViewEnv->mInfinite,
                                                    SMC_UPDATE_BY_LOBCURSOR,  // smcUpdateOpt
                                                    ID_FALSE )                // aForbiddenToRetry
                  != IDE_SUCCESS );

        IDE_TEST( insertIntoIdx( aStatistics, /* idvSQL* */
                                 aTrans,
                                 aLobViewEnv,
                                 sNxtFixedRowPtr )
                 != IDE_SUCCESS );
        
        break;
    }

    *aNxtFixedRowPtr = sNxtFixedRowPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION(lob_cursor_too_old);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_LobCursorTooOld));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * write할 공간을 확보한다.
 **********************************************************************/
IDE_RC smcLob::reserveSpace(void*         aTrans,
                            smLobViewEnv* aLobViewEnv,
                            SChar*        aNxtFixedRowPtr,
                            UInt          aOffset,
                            UInt          aNewSize)
{
    scPageID       sPageID;
    smcLobDesc*    sCurLobDesc  = NULL;
    smcLobDesc*    sNewLobDesc  = NULL;
    SChar*         sLobInRowPtr = NULL;
    ULong          sLobColBuf[ SMC_LOB_MAX_IN_ROW_STORE_SIZE/ID_SIZEOF(ULong) ];
    void*          sLobColPagePtr;

    sLobInRowPtr = aNxtFixedRowPtr + aLobViewEnv->mLobCol.offset;
    sCurLobDesc  = (smcLobDesc*)sLobInRowPtr;

    IDE_TEST_RAISE( aOffset > sCurLobDesc->length, range_error );

    // old image를 복사
    if ( SM_VCDESC_IS_MODE_IN(sCurLobDesc) )
    {
        // BUG-30101 데이타가 In-Mode로 저장되었을 경우 데이타는
        // Fixed영역에 저장되어있다. Lob Column Desc 와 함께 같이 복사해둔다
        idlOS::memcpy( sLobColBuf,
                       sLobInRowPtr,
                       sCurLobDesc->length + ID_SIZEOF(smVCDescInMode));
    }
    else
    {
        // Out Mode로 저장 되어 있을경우
        idlOS::memcpy( sLobColBuf, sLobInRowPtr, ID_SIZEOF(smcLobDesc) );
    }

    // new version 할당
    // BUG-30036 Memory LOB을 ODBC로 Insert 하다 실패하였을 때,
    // 변경하던 LOB Desc를 수정된 채로 Rollback하지 않고 있습니다. 로 인하여
    // 별도의 Dummy Lob Desc로 Prepare 하고 Log를 먼저 기록 한 후에
    // 변경된 LOB Desc를 Data Page에 반영합니다.
    IDE_TEST( reserveSpaceInternal( aTrans,
                                    (smcTableHeader*)(aLobViewEnv->mTable),
                                    &aLobViewEnv->mLobCol,
                                    aLobViewEnv->mLobVersion,
                                    (smcLobDesc*)sLobColBuf,
                                    aOffset,
                                    aNewSize,
                                    SM_FLAG_INSERT_LOB,
                                    ID_FALSE /* aIsFullWrite */)
              != IDE_SUCCESS );

    sPageID = SMP_SLOT_GET_PID( aNxtFixedRowPtr );

    IDE_ASSERT( smmManager::getPersPagePtr( aLobViewEnv->mLobCol.colSpace, 
                                            sPageID,
                                            (void**)&sLobColPagePtr )
                == IDE_SUCCESS );

    IDE_TEST( smrUpdate::physicalUpdate( 
                                     NULL, /* idvSQL* */
                                     aTrans,
                                     aLobViewEnv->mLobCol.colSpace,
                                     sPageID,
                                     (scOffset)((SChar*)sLobInRowPtr - (SChar*)sLobColPagePtr),
                                     (SChar *)sLobInRowPtr,  /*aBeforeImage*/
                                     ID_SIZEOF(smcLobDesc), /*aBeforeImageSize*/
                                     (SChar *)sLobColBuf,    /*aAfterImage1*/
                                     ID_SIZEOF(smcLobDesc), /*aAfterImageSize1*/
                                     NULL, /*aAfterImage2 */
                                     0     /* aAfterImageSize2*/)
              != IDE_SUCCESS );

    sNewLobDesc = (smcLobDesc*)sLobColBuf;

    // BUG-30036 변경된 Lob Desc를 Data Page에 반영
    if ( SM_VCDESC_IS_MODE_IN(sNewLobDesc) )
    {
        idlOS::memcpy( sLobInRowPtr,
                       sLobColBuf,
                       ID_SIZEOF(smVCDescInMode) );
    }
    else
    {
        // BUG-30101 Out Mode로 저장 된 경우 Lob Desc만 복사한다.
        idlOS::memcpy( sLobInRowPtr,
                       sLobColBuf,
                       ID_SIZEOF(smcLobDesc) );
    }

    IDE_TEST( smmDirtyPageMgr::insDirtyPage( aLobViewEnv->mLobCol.colSpace,
                                             sPageID)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(range_error);
    {
        // BUG-29212 디스크 Lob의 read시 range check를 하지 않습니다.
        // range는 Offset부터 Amount - 1 까지 입니다.
        IDE_SET(ideSetErrorCode(smERR_ABORT_RangeError,
                                aOffset,
                                (aOffset + aNewSize - 1),
                                0,
                                (sCurLobDesc->length - 1)));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * 공간 trim 한다.
 **********************************************************************/
IDE_RC smcLob::trimSpaceInternal( void*               aTrans,
                                  smcTableHeader*     aTable,
                                  const smiColumn*    aLobColumn,
                                  ULong               aLobVersion,
                                  smcLobDesc*         aLobDesc,
                                  UInt                aOffset,
                                  UInt                aAddOIDFlag )
{
    smcLobDesc* sOldLobDesc;
    UInt        sNewLobLen;
    UInt        sNewLPCHCnt;
    UInt        sBeginIdx;
    UInt        sEndIdx;
    ULong       sLobColumnBuffer[SM_PAGE_SIZE / ID_SIZEOF(ULong)];

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );
    IDE_DASSERT( aLobDesc != NULL );

    IDE_TEST_RAISE( (aOffset >= aLobDesc->length), range_error );

    /*
     * to backup LOB Descriptor
     */
    
    sOldLobDesc = (smcLobDesc*)sLobColumnBuffer;

    if ( SM_VCDESC_IS_MODE_IN(aLobDesc) )
    {
        /* 데이타가 In-Mode로 저장되었을 경우 데이타는
         * Fixed영역에 저장되어있다. Lob Column Desc 와 함께 같이 복사해둔다 */
        idlOS::memcpy( (SChar*)sOldLobDesc,
                       (SChar*)aLobDesc,
                       ID_SIZEOF(smVCDescInMode) + aLobDesc->length );
    }
    else
    {
        idlOS::memcpy( (SChar*)sOldLobDesc,
                       (SChar*)aLobDesc,
                       ID_SIZEOF(smcLobDesc) );
    }

    /*
     * to allocate new version space
     */

    sNewLobLen = aOffset;
    
    if ( (sOldLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT )
    {
        aLobDesc->mLobVersion = aLobVersion;

        sNewLPCHCnt = getLPCHCntFromLength(sNewLobLen);

        IDE_TEST( allocLPCH(aTrans,
                            aTable,
                            aLobDesc,
                            sOldLobDesc,
                            aLobColumn->colSpace,
                            sNewLPCHCnt,
                            aAddOIDFlag,
                            ID_FALSE /* aIsFullWrite */)
                  != IDE_SUCCESS );

        IDE_TEST( trimPiece(aTrans,
                            aTable,
                            aLobVersion,
                            aLobDesc,
                            sOldLobDesc,
                            aLobColumn->colSpace,
                            aOffset,
                            aAddOIDFlag)
                  != IDE_SUCCESS );
    }
    else
    {
        /* nithing to do */
    }

    aLobDesc->length = sNewLobLen;

    /*
     * to remove old version (LPCH & LOB Piece)
     */

    if ( ((sOldLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT) &&
         (sOldLobDesc->mLPCHCount > 0) )
    {

        sBeginIdx = aOffset / SMP_VC_PIECE_MAX_SIZE;
        sEndIdx = sOldLobDesc->mLPCHCount - 1;

        IDE_TEST( removeOldLPCH(aTrans,
                                aTable,
                                aLobColumn,
                                sOldLobDesc,
                                aLobDesc,
                                sBeginIdx,
                                sEndIdx)
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(range_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_RangeError,
                                aOffset,
                                (aLobDesc->length - 1),
                                0,
                                (aLobDesc->length - 1)));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Old Version의 LPCH가 가리키는 Piece 및 LPCH를 삭제한다.
 **********************************************************************/
IDE_RC smcLob::removeOldLPCH(void*               aTrans,
                             smcTableHeader*     aTable,
                             const smiColumn*    aLobColumn,
                             smcLobDesc*         aOldLobDesc,
                             smcLobDesc*         aLobDesc,
                             UInt                aBeginIdx,
                             UInt                aEndIdx)
{
    smcLPCH*    sOldLPCH = NULL;
    smcLPCH*    sNewLPCH = NULL;
    UInt        i;

    IDE_ASSERT( aOldLobDesc->mFirstLPCH != NULL );
    IDE_ASSERT( aOldLobDesc->mLPCHCount > 0 );
    IDE_ASSERT( (aOldLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT );

    if ( aLobDesc->mFirstLPCH != aOldLobDesc->mFirstLPCH )
    {
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aTable->mSelfOID,
                                           (smOID)(aOldLobDesc->mFirstLPCH),
                                           aLobColumn->colSpace,
                                           SM_OID_OLD_LPCH )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */

    }

    for ( i = aBeginIdx; i <= aEndIdx; i++ )
    {
        sOldLPCH = aOldLobDesc->mFirstLPCH + i;

        if ( i < aLobDesc->mLPCHCount )
        {
            sNewLPCH = aLobDesc->mFirstLPCH + i;
        }
        else
        {
            sNewLPCH = NULL;
        }

        if ( sNewLPCH == NULL )
        {
            IDE_TEST( smcRecord::setFreeFlagAtVCPieceHdr(aTrans,
                                                         aTable->mSelfOID,
                                                         aLobColumn->colSpace,
                                                         sOldLPCH->mOID,
                                                         sOldLPCH->mPtr,
                                                         SM_VCPIECE_FREE_OK)
                      != IDE_SUCCESS );
                            
            IDE_TEST( smLayerCallback::addOID( aTrans,
                                               aTable->mSelfOID,
                                               sOldLPCH->mOID,
                                               aLobColumn->colSpace,
                                               SM_OID_OLD_VARIABLE_SLOT )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( sOldLPCH->mOID != sNewLPCH->mOID )
            {
                IDE_TEST( smcRecord::setFreeFlagAtVCPieceHdr(aTrans,
                                                             aTable->mSelfOID,
                                                             aLobColumn->colSpace,
                                                             sOldLPCH->mOID,
                                                             sOldLPCH->mPtr,
                                                             SM_VCPIECE_FREE_OK)
                          != IDE_SUCCESS );
                            
                IDE_TEST( smLayerCallback::addOID( aTrans,
                                                   aTable->mSelfOID,
                                                   sOldLPCH->mOID,
                                                   aLobColumn->colSpace,
                                                   SM_OID_OLD_VARIABLE_SLOT )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
         }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcLob::trimPiece(void*           aTrans,
                         smcTableHeader* aTable,
                         ULong           aLobVersion,
                         smcLobDesc*     aLobDesc,
                         smcLobDesc*     aOldLobDesc,
                         scSpaceID       aLobSpaceID,
                         UInt            aOffset,
                         UInt            aAddOIDFlag)
{
    UInt             sIdx;
    smOID            sNxtPieceOID = SM_NULL_OID;
    smOID            sNewPieceOID = SM_NULL_OID;
    SChar*           sNewPiecePtr = NULL;
    smcLPCH*         sTargetLPCH;
    smVCPieceHeader* sCurVCPieceHeader;
    UInt             sPieceSize;

    IDE_ASSERT( aTrans   != NULL );
    IDE_ASSERT( aTable   != NULL );
    IDE_ASSERT( aLobDesc != NULL );
    IDE_DASSERT( (aOldLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT );

    if ( aLobDesc->mLPCHCount == 0 )
    {
        aLobDesc->fstPieceOID = SM_NULL_OID;
        IDE_CONT( SKIP_TRIM_PIECE );
    }

    sIdx = aOffset / SMP_VC_PIECE_MAX_SIZE;

    IDE_ERROR( (UInt)sIdx < aOldLobDesc->mLPCHCount );

    if ( aLobDesc->mLPCHCount <= 1 )
    {
        IDE_ERROR( aOffset <= SMP_VC_PIECE_MAX_SIZE );
        
        sPieceSize = aOffset;
    }
    else
    {
        sPieceSize = SMP_VC_PIECE_MAX_SIZE;
    }

    /*
     * to allocate piece.
     */

    if ( aLobDesc->mLPCHCount > 0 )
    {
        IDE_ERROR( sIdx < aLobDesc->mLPCHCount );

        sNxtPieceOID = SM_NULL_OID;
        sNewPiecePtr = NULL;
        sNewPieceOID = SM_NULL_OID;

        if ( aLobVersion != aOldLobDesc->mFirstLPCH[sIdx].mLobVersion )
        {
            IDE_TEST( smpVarPageList::allocSlot(aTrans,
                                                aLobSpaceID,
                                                aTable->mSelfOID,
                                                aTable->mVar.mMRDB,
                                                sPieceSize,
                                                sNxtPieceOID,
                                                &sNewPieceOID,
                                                &sNewPiecePtr)
                      != IDE_SUCCESS );

            aLobDesc->mFirstLPCH[sIdx].mLobVersion = aLobVersion;
            aLobDesc->mFirstLPCH[sIdx].mOID        = sNewPieceOID;
            aLobDesc->mFirstLPCH[sIdx].mPtr        = sNewPiecePtr;

            /* 새로 할당한 lob piece에 대하여 version list 추가 */
            if ( SM_INSERT_ADD_OID_IS_OK(aAddOIDFlag) )
            {
                IDE_TEST( smLayerCallback::addOID( aTrans,
                                                   aTable->mSelfOID,
                                                   sNewPieceOID,
                                                   aLobSpaceID,
                                                   SM_OID_NEW_VARIABLE_SLOT )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }

            IDE_TEST( copyPiece(aTrans,
                                aTable,
                                aLobSpaceID,
                                aOldLobDesc,
                                (sIdx * SMP_VC_PIECE_MAX_SIZE),
                                aLobDesc,
                                (sIdx * SMP_VC_PIECE_MAX_SIZE),
                                sPieceSize)
                      != IDE_SUCCESS );
        }
        else
        {
            sTargetLPCH = aLobDesc->mFirstLPCH + sIdx;
                
            sCurVCPieceHeader =
                (smVCPieceHeader*)(sTargetLPCH)->mPtr;

            if ( sCurVCPieceHeader->nxtPieceOID != sNxtPieceOID )
            {
                IDE_TEST( smrUpdate::setNxtOIDAtVarRow(
                                          NULL, /* idvSQL* */
                                          aTrans,
                                          aLobSpaceID,
                                          aTable->mSelfOID,
                                          sTargetLPCH->mOID,
                                          sCurVCPieceHeader->nxtPieceOID,
                                          sNxtPieceOID )
                          != IDE_SUCCESS );

                sCurVCPieceHeader->nxtPieceOID = sNxtPieceOID;

                IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                          aLobSpaceID,
                                          SM_MAKE_PID( sTargetLPCH->mOID ))
                          != IDE_SUCCESS );
            }
        }

        /*
         * to set fstPieceOID
         */
        
        aLobDesc->fstPieceOID = aLobDesc->mFirstLPCH[0].mOID;
    }
    else
    {
        IDE_ERROR( aLobDesc->mFirstLPCH == NULL );
        
        /*
         * to set fstPieceOID
         */
        
        aLobDesc->fstPieceOID = SM_NULL_OID;
    }

    /*
     * to set nxtPieceOID
     */ 

    if ( sIdx > 0 )
    {
        sNxtPieceOID = aLobDesc->mFirstLPCH[sIdx].mOID;

        sTargetLPCH = aLobDesc->mFirstLPCH + (sIdx - 1);

        sCurVCPieceHeader = (smVCPieceHeader*)(sTargetLPCH)->mPtr;

        IDE_TEST( smrUpdate::setNxtOIDAtVarRow( NULL, /* idvSQL* */
                                                aTrans,
                                                aLobSpaceID,
                                                aTable->mSelfOID,
                                                (sTargetLPCH)->mOID,
                                                sCurVCPieceHeader->nxtPieceOID,
                                                sNxtPieceOID )
                  != IDE_SUCCESS );

        sCurVCPieceHeader->nxtPieceOID = sNxtPieceOID;

        IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                      aLobSpaceID,
                      SM_MAKE_PID( (sTargetLPCH)->mOID ))
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_TRIM_PIECE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcLob::trim( idvSQL       * aStatistics,
                     void         * aTrans,
                     smLobViewEnv * aLobViewEnv,
                     smLobLocator   aLobLocator,
                     UInt           aOffset )
{
    SChar*      sFixedRowPtr = NULL;
    smcLobDesc* sCurLobDesc  = NULL;
    SChar*      sLobInRowPtr = NULL;
    smcLobDesc* sNewLobDesc;
    scPageID    sPageID;
    void*       sLobColPagePtr;
    ULong       sLobColBuf[ SMC_LOB_MAX_IN_ROW_STORE_SIZE/ID_SIZEOF(ULong) ];

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aLobViewEnv != NULL );

    /*
     * to update LOB column
     */

    IDE_TEST( getLastVersion(aStatistics,
                             aTrans,
                             aLobViewEnv,
                             &sFixedRowPtr)
              != IDE_SUCCESS );

    /*
     * to set new LOB version
     */
    
    sLobInRowPtr = sFixedRowPtr + aLobViewEnv->mLobCol.offset;
    sCurLobDesc  = (smcLobDesc*)sLobInRowPtr;

    IDE_TEST_RAISE( aOffset >= sCurLobDesc->length, range_error );

    if ( SM_VCDESC_IS_MODE_IN(sCurLobDesc) )
    {
        aLobViewEnv->mLobVersion = 1;
        // BUG-30101 데이타가 In-Mode로 저장되었을 경우 데이타는
        // Fixed영역에 저장되어있다. Lob Column Desc 와 함께 같이 복사해둔다
        idlOS::memcpy( sLobColBuf,
                       sLobInRowPtr,
                       sCurLobDesc->length + ID_SIZEOF(smVCDescInMode));
    }
    else
    {
        // LobVersion 은 Out Mode 에서만 유효하다. 
        aLobViewEnv->mLobVersion = sCurLobDesc->mLobVersion + 1;
        IDE_TEST_RAISE( aLobViewEnv->mLobVersion == ID_ULONG_MAX,
                        error_version_overflow );

        // Out Mode로 저장 되어 있을경우
        idlOS::memcpy( sLobColBuf, sLobInRowPtr, ID_SIZEOF(smcLobDesc) );
    }

    /*
     * to trim
     */

    IDE_TEST( trimSpaceInternal(aTrans,
                                (smcTableHeader*)(aLobViewEnv->mTable),
                                &aLobViewEnv->mLobCol,
                                aLobViewEnv->mLobVersion,
                                (smcLobDesc*)sLobColBuf,
                                aOffset,
                                SM_FLAG_INSERT_LOB)
              != IDE_SUCCESS );

    sPageID = SMP_SLOT_GET_PID( sFixedRowPtr );

    IDE_ASSERT( smmManager::getPersPagePtr( aLobViewEnv->mLobCol.colSpace, 
                                            sPageID,
                                            (void**)&sLobColPagePtr )
                == IDE_SUCCESS );

    IDE_TEST( smrUpdate::physicalUpdate(NULL, /* idvSQL* */
                                        aTrans,
                                        aLobViewEnv->mLobCol.colSpace,
                                        sPageID,
                                        (SChar*)sLobInRowPtr - (SChar*)sLobColPagePtr,
                                        (SChar*)sLobInRowPtr,
                                        ID_SIZEOF(smcLobDesc),
                                        (SChar*)sLobColBuf,
                                        ID_SIZEOF(smcLobDesc),
                                        NULL,
                                        0)
              != IDE_SUCCESS );
    
    sNewLobDesc = (smcLobDesc*)sLobColBuf;

    if ( SM_VCDESC_IS_MODE_IN(sNewLobDesc) )
    {
        idlOS::memcpy( sLobInRowPtr,
                       sLobColBuf,
                       ID_SIZEOF(smVCDescInMode) );
    }
    else
    {
        // BUG-30101 Out Mode로 저장 된 경우 Lob Desc만 복사한다.
        idlOS::memcpy( sLobInRowPtr,
                       sLobColBuf,
                       ID_SIZEOF(smcLobDesc) );
    }
    
    IDE_TEST( smmDirtyPageMgr::insDirtyPage( aLobViewEnv->mLobCol.colSpace,
                                             sPageID )
              != IDE_SUCCESS );

    /*
     * to write replication log
     */
    
    if ( (aLobViewEnv->mOpenMode == SMI_LOB_READ_WRITE_MODE) &&
         (smcTable::needReplicate(
            (const smcTableHeader*)aLobViewEnv->mTable, aTrans) == ID_TRUE ) )
    {
        IDE_TEST( smrLogMgr::writeLobTrimLogRec(
                                              aStatistics,
                                              aTrans,
                                              aLobLocator,
                                              aOffset,
                                              ( (const smcTableHeader*)aLobViewEnv->mTable )->mSelfOID )
                  != IDE_SUCCESS );
    }

    mMemLobStatistics.mTrim++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(range_error);
    {
        // BUG-29212 디스크 Lob의 read시 range check를 하지 않습니다.
        // range는 Offset부터 Amount - 1 까지 입니다.
        IDE_SET(ideSetErrorCode(smERR_ABORT_RangeError,
                                aOffset,
                                (sCurLobDesc->length - 1),
                                0,
                                (sCurLobDesc->length - 1)));
    }
    IDE_EXCEPTION( error_version_overflow )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG,
                                 "memory trim:version overflow") );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
