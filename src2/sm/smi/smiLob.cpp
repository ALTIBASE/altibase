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

/***********************************************************************
 * PROJ-1362 Large Record & Internal LOB support
 **********************************************************************/
#include <smiLob.h>
#include <smiStatement.h>
#include <smiTrans.h>
#include <smx.h>
#include <sdc.h>

const UShort smiLob::mLobPieceSize = IDL_MAX(SM_PAGE_SIZE,SD_PAGE_SIZE);

/* PROJ-2728 Sharding LOB */
smLobModule sdiLobModule;

IDE_RC smiLob::setShardLobModule(
                smLobOpenFunc                   aOpen,
                smLobReadFunc                   aRead,
                smLobWriteFunc                  aWrite,
                smLobEraseFunc                  aErase,
                smLobTrimFunc                   aTrim,
                smLobPrepare4WriteFunc          aPrepare4Write,
                smLobFinishWriteFunc            aFinishWrite,
                smLobGetLobInfoFunc             aGetLobInfo,
                smLobWriteLog4LobCursorOpen     aWriteLog4CursorOpen,
                smLobCloseFunc                  aClose )
{
    sdiLobModule.mOpen                   = aOpen;
    sdiLobModule.mRead                   = aRead;
    sdiLobModule.mWrite                  = aWrite;
    sdiLobModule.mErase                  = aErase;
    sdiLobModule.mTrim                   = aTrim;
    sdiLobModule.mPrepare4Write          = aPrepare4Write;
    sdiLobModule.mFinishWrite            = aFinishWrite;
    sdiLobModule.mGetLobInfo             = aGetLobInfo;
    sdiLobModule.mWriteLog4LobCursorOpen = aWriteLog4CursorOpen;
    sdiLobModule.mClose                  = aClose;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : This function opens Shard LOB Cursor.
 * PROJ-2728 Sharding LOB
 *
 *  aSmiTrans         - [IN]  smiTrans
 *  aMmStmtId         - [IN]  mmcStatement ID
 *  aRemoteStmtId     - [IN]  Remote statement ID
 *  aNodeId           - [IN]  Data Node ID
 *  aLobLocatorType   - [IN]  CLOB or BLOB locator
 *  aRemoteLobLocator - [IN]  node#에서 받아온lob locator
 *  aInfo             - [IN]  not null 제약등 QP에서 사용함.
 *  aMode             - [IN]  LOB Cursor Open Mode
 *  aShardLobLocator  - [OUT] shard lob locator
 **********************************************************************/
IDE_RC smiLob::openShardLobCursor( smiTrans        * aSmiTrans,
                                   UInt              aMmSessId,
                                   UInt              aMmStmtId,
                                   UInt              aRemoteStmtId,
                                   UInt              aNodeId,
                                   SShort            aLobLocatorType,
                                   smLobLocator      aRemoteLobLocator,
                                   UInt              aInfo,
                                   smiLobCursorMode  aMode,
                                   smLobLocator*     aShardLobLocator )
{
    smxTrans  * sTrans;

    sTrans    = (smxTrans *)(aSmiTrans->getTrans());
    IDE_ASSERT( sTrans != NULL );

    IDE_TEST( sTrans->openShardLobCursor(
                                 NULL,
                                 aMmSessId,
                                 aMmStmtId,
                                 aRemoteStmtId,
                                 aNodeId,
                                 aLobLocatorType,
                                 aRemoteLobLocator,
                                 aInfo,
                                 aMode,
                                 aShardLobLocator )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace
 * volatile tablespace는 memory tablespace 와 유사하므로,
 * 아래 함수를 공유한다. */
/***********************************************************************
 * Description : Memory LobCursor를 생성하고, 트랜잭션에 등록한다.
 * Implementation : 자신의 TID와 LobCursor등록 시점에서 부여한
 *                  LobCursorID를 합성한 LobLocator를
 *                  output parameter로  전달한다.
 *
 *    aTableCursor - [IN]  Table Cursor
 *    aRow         - [IN]  memory row
 *    aLobColumn   - [IN]  Lob Column의 Column정보
 *    aInfo        - [IN]  not null 제약등 QP에서 사용함.
 *    aMode        - [IN]  LOB Cursor Open Mode
 *    aLobLocator  - [OUT] Lob Locator반환
 **********************************************************************/
IDE_RC smiLob::openLobCursorWithRow( smiTableCursor   * aTableCursor,
                                     void             * aRow,
                                     const smiColumn  * aLobColumn,
                                     UInt               aInfo,
                                     smiLobCursorMode   aMode,
                                     smLobLocator     * aLobLocator )
{
    smiTrans  * sSmiTrans;
    smxTrans  * sTrans;

    IDE_ASSERT( aTableCursor != NULL );
    // CodeSonar::Null Pointer Dereference (9)
    IDE_ASSERT( aTableCursor->mStatement != NULL );

    sSmiTrans = aTableCursor->mStatement->getTrans();
    sTrans    = (smxTrans *)(sSmiTrans->getTrans());
    IDE_ASSERT( sTrans != NULL );

    if(aMode == SMI_LOB_TABLE_CURSOR_MODE)
    {
        aMode = (aTableCursor->mUntouchable == ID_TRUE)
                ? SMI_LOB_READ_MODE : SMI_LOB_READ_WRITE_MODE;
    }

    IDE_TEST_RAISE((aTableCursor->mUntouchable == ID_TRUE) &&
                   (aMode == SMI_LOB_READ_WRITE_MODE),
                   err_invalid_mode);

    IDU_FIT_POINT( "smiLob::openLobCursorWithRow::openLobCursor" );

    IDE_TEST( sTrans->openLobCursor( aTableCursor->mStatement->mStatistics,
                                     aTableCursor->mTable,
                                     aMode,
                                     aTableCursor->mSCN,
                                     aTableCursor->mInfinite,
                                     aRow,
                                     aLobColumn,
                                     aInfo,
                                     aLobLocator )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_mode );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALIDE_LOB_CURSOR_MODE ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Disk LobCursor를 생성하고, 트랜잭션에 등록한다.
 * Implementation : 자신의 TID와 LobCursor등록시점에서 부여한
 *                  LobCursorID를 합성한 LobLocator를
 *                  output parameter로  전달한다.
 *
 *  aTableCursor   - [IN]  table cursor
 *  aGRID          - [IN]  global row id
 *  aLobColumn     - [IN]  lob column 정보
 *  aInfo          - [IN]  not null 제약등 QP에서 사용함.
 *  aMode          - [IN]  LOB Cursor Open Mode
 *  aLobLocator    - [OUT] lob locator
 **********************************************************************/
IDE_RC smiLob::openLobCursorWithGRID(smiTableCursor   * aTableCursor,
                                     scGRID             aGRID,
                                     smiColumn        * aLobColumn,
                                     UInt               aInfo,
                                     smiLobCursorMode   aMode,
                                     smLobLocator     * aLobLocator)
{
    smxTrans* sTrans;

    IDE_ASSERT( aTableCursor != NULL );
    // CodeSonar::Null Pointer Dereference (9)
    IDE_ASSERT( aTableCursor->mStatement != NULL );

    sTrans = (smxTrans*)( aTableCursor->mStatement->getTrans()->getTrans() );
    IDE_ASSERT( sTrans != NULL );

    if( aMode == SMI_LOB_TABLE_CURSOR_MODE )
    {
        aMode = (aTableCursor->mUntouchable == ID_TRUE)
            ? SMI_LOB_READ_MODE : SMI_LOB_READ_WRITE_MODE;
    }

    IDE_TEST_RAISE((aTableCursor->mUntouchable == ID_TRUE) &&
                   (aMode == SMI_LOB_READ_WRITE_MODE),
                   err_invalid_mode);

    /*
     * BUG-27495  outer join에 의해 생성된 lob column 을 select시 오류
     */
    if( SC_GRID_IS_VIRTUAL_NULL_GRID_FOR_LOB( aGRID ) )
    {
        *aLobLocator = SMI_NULL_LOCATOR;
        IDE_CONT( err_virtual_null_grid );
    }

    IDE_TEST( sTrans->openLobCursor( aTableCursor->mStatement->mStatistics,
                                     aTableCursor->mTable,
                                     aMode,
                                     aTableCursor->mSCN,
                                     aTableCursor->mInfinite4DiskLob,
                                     aGRID,
                                     aLobColumn,
                                     aInfo,
                                     aLobLocator ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( err_virtual_null_grid );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_mode );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALIDE_LOB_CURSOR_MODE ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LobCursor를 close시킨다.
 * Implementation: 1> LobLocator에 실려있는 트랜잭션ID를 구한다.
 *                 2> 트랜잭션 ID를 이용하여 트랜잭션 객체를 구한다.
 *                 3> 해당 트랜잭션에서 LobCursorID에 해당하는
 *                    LobCursor를 닫는다.
 *
 *   aLobLocator - [IN] close할 Lob Cursor를 가리키는 Lob Locator
 **********************************************************************/
IDE_RC smiLob::closeLobCursor( idvSQL*      aStatistics,
                               smLobLocator aLobLocator )
{
    smxTrans *    sTrans;
    smLobCursorID sLobCursorID;
    smTID         sTID;

    sLobCursorID = SMI_MAKE_LOB_CURSORID( aLobLocator );
    sTID = SMI_MAKE_LOB_TRANSID( aLobLocator );

    IDE_TEST_RAISE( SMI_IS_NULL_LOCATOR(aLobLocator) == ID_TRUE,
                    err_wrong_locator );

    sTrans = smxTransMgr::getTransByTID( sTID );
    IDE_TEST_RAISE( sTrans->mTransID != sTID, transaction_end );

    IDE_TEST( sTrans->closeLobCursor( aStatistics,
                                      sLobCursorID,
                                      SMI_IS_SHARD_LOB_LOCATOR( aLobLocator) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( transaction_end );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CannotSpanTransByLobLocator,
                                  sTID ));
    }
    IDE_EXCEPTION( err_wrong_locator )
    {
        IDE_SET( ideSetErrorCode(smERR_IGNORE_CAN_NOT_USE_THIS_LOCATOR) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiLob::closeAllLobCursors( idvSQL       * aStatistics,
                                   smLobLocator   aLobLocator,
                                   UInt           aInfo )
{
    smxTrans *    sTrans;
    smTID         sTID;

    sTID = SMI_MAKE_LOB_TRANSID( aLobLocator );

    IDE_TEST_RAISE( SMI_IS_NULL_LOCATOR(aLobLocator) == ID_TRUE,
                    err_wrong_locator );

    sTrans = smxTransMgr::getTransByTID( sTID );

    IDE_TEST_RAISE( sTrans->mTransID != sTID, transaction_end );

    if ( aInfo == SMI_LOB_LOCATOR_CLIENT_FALSE )
    {
        IDE_TEST( sTrans->closeAllLobCursorsWithRPLog() != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sTrans->closeAllLobCursors(
                        aStatistics,
                        aInfo,
                        SMI_IS_SHARD_LOB_LOCATOR(aLobLocator) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( transaction_end );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CannotSpanTransByLobLocator,
                                  sTID ));
    }
    IDE_EXCEPTION( err_wrong_locator )
    {
        IDE_SET( ideSetErrorCode(smERR_IGNORE_CAN_NOT_USE_THIS_LOCATOR) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Locator가 가르키는 Lob Cursor가 열려 있는지 확인한다.
 *
 * Implementation: aLobLocator가 가르키는 트랜잭션이 이미 끝났거나,
 *                 aLobLocator가 가르키는 LobCursor가 close된 경우
 *                 ID_FALSE를 return한다.
 *
 *   aLobLocator - [IN] Lob cursor
 **********************************************************************/
idBool smiLob::isOpen( smLobLocator aLobLocator )
{
    smxTrans      * sTrans;
    smLobCursor   * sLobCursor;
    smLobCursorID   sLobCursorID;
    smTID           sTID    = SMI_MAKE_LOB_TRANSID( aLobLocator );
    idBool          sIsOpen = ID_TRUE;

    sTrans = smxTransMgr::getTransByTID( sTID );

    if( sTrans->mTransID != sTID )
    {
        sIsOpen = ID_FALSE;
    }
    else
    {
        sLobCursorID = SMI_MAKE_LOB_CURSORID( aLobLocator );

        if( sTrans->getLobCursor( sLobCursorID,
                                  &sLobCursor,
                                  SMI_IS_SHARD_LOB_LOCATOR( aLobLocator) )
            != IDE_SUCCESS )
        {
            sIsOpen = ID_FALSE;
        }
        if( sLobCursor == NULL )
        {
            sIsOpen = ID_FALSE;
        }
    }
    
    return sIsOpen;
}

/***********************************************************************
 * Description : LobLocator를 이용하여 lob value를 특정 aOffset에서
 *               aMount만큼 읽는다.
 *
 *    aStatistics - [IN]  통계 정보
 *    aLobLocator - [IN]  읽을 대상을 가리키는 Lob Locator
 *    aOffset     - [IN]  Lob data 에서 읽을 offset
 *    aMount      - [IN]  읽을 Lob data 크기
 *    aPiece      - [OUT] 읽은 Lob data를 반환할 버퍼
 *    aReadLength - [OUT] 읽은 Lob data 크기
 **********************************************************************/
IDE_RC smiLob::read( idvSQL      * aStatistics,
                     smLobLocator  aLobLocator,
                     UInt          aOffset,
                     UInt          aMount,
                     UChar       * aPiece,
                     UInt        * aReadLength )
{
    smxTrans      * sTrans;
    smLobCursor   * sLobCursor;

    IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                    &sLobCursor,
                                    &sTrans )
              != IDE_SUCCESS );

    //BUG-48212: Lob 탐색할 때, 타임아웃될 수 있도록 체크 코드 추가
    IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );

    IDE_TEST( sLobCursor->mModule->mRead( aStatistics,
                                          sTrans,
                                          &(sLobCursor->mLobViewEnv),
                                          aOffset,
                                          aMount,
                                          aPiece,
                                          aReadLength )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LobLocator가 가르키는 lob의 길이를 return한다.
 *
 *     aLobLocator - [IN]  크기를 확인하려는 Lob의 Locator
 *     aLobLen     - [OUT] Lob data의 크기를 반환한다.
 *     aIsNullLob  - [OUT] Null 여부를 반환
 **********************************************************************/
IDE_RC smiLob::getLength( idvSQL        * aStatistics,
                          smLobLocator    aLobLocator,
                          SLong         * aLobLen,
                          idBool        * aIsNullLob )
{
    smxTrans      * sTrans;
    smLobCursor   * sLobCursor;
    idBool          sIsNullLob;
    UInt            sLobLen;

    /* BUG-41293 */
    if ( SMI_IS_NULL_LOCATOR(aLobLocator) == ID_TRUE )
    {
        sLobLen = 0;
        sIsNullLob = ID_TRUE;
    }
    else
    {
        IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                        &sLobCursor,
                                        &sTrans )
                  != IDE_SUCCESS );

        IDE_TEST( sLobCursor->mModule->mGetLobInfo(aStatistics,
                                                   sTrans,
                                                   &(sLobCursor->mLobViewEnv),
                                                   &sLobLen,
                                                   NULL /* In Out Mode */,
                                                   &sIsNullLob)
                  != IDE_SUCCESS);
    }

    *aLobLen = sLobLen;
    *aIsNullLob = sIsNullLob;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob을 aOffset에서 시작하여 aPieceLen 만큼,
 *               aPiece내용으로 부분 갱신하는 함수.
 *
 *     aStatistics - [IN] 통계 자료
 *     aLobLocator - [IN] 갱신할 Lob을 가리키는 Lob Locator
 *     aOffset     - [IN] 갱신할 위치 Offset
 *     aPieceLen   - [IN] 덮어쓰려는 Lob Data의 크기
 *     aPiece      - [IN] 덮어쓰려는 Lob Data
 **********************************************************************/
IDE_RC smiLob::write( idvSQL       * aStatistics,
                      smLobLocator   aLobLocator,
                      UInt           aPieceLen,
                      UChar        * aPiece )
{
    smxTrans      * sTrans;
    smLobCursor   * sLobCursor   = NULL;

    IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                    &sLobCursor,
                                    &sTrans )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_MODE) ||
                    (sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_LAST_VERSION_MODE),
                    canNotModify );

    IDE_TEST_RAISE( sLobCursor->mLobViewEnv.mWriteError == ID_TRUE,
                    writePrepareProtocolError );

    IDE_TEST_RAISE(
        (sLobCursor->mLobViewEnv.mWritePhase != SM_LOB_WRITE_PHASE_PREPARE) &&
        (sLobCursor->mLobViewEnv.mWritePhase != SM_LOB_WRITE_PHASE_WRITE),
        writePrepareProtocolError );

    sLobCursor->mLobViewEnv.mWritePhase = SM_LOB_WRITE_PHASE_WRITE;

    IDE_TEST( sLobCursor->mModule->mWrite(
                  aStatistics,
                  sTrans,
                  &(sLobCursor->mLobViewEnv),
                  aLobLocator,
                  sLobCursor->mLobViewEnv.mWriteOffset,
                  aPieceLen,
                  aPiece,
                  ID_TRUE /* aIsFromAPI */,
                  SMR_CT_END)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( canNotModify );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CanNotModifyLob));
    }
    IDE_EXCEPTION( writePrepareProtocolError );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_PrePareWriteProtocol ));
    }
    IDE_EXCEPTION_END;

    if( sLobCursor != NULL )
    {
        sLobCursor->mLobViewEnv.mWriteError = ID_TRUE;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob을 aOffset에서 시작하여 aSize 만큼,
 *               지우는 함수.
 *               이 함수는 추후 필요할 수 있는 기능이므로 구현됨.
 *               실제 사용전에 테스트가 필요함.
 *
 *     aStatistics - [IN] 통계 자료
 *     aLobLocator - [IN] 지우려는 Lob을 가리키는 Lob Locator
 *     aOffset     - [IN] 지우려는 위치 Offset
 *     aPieceLen   - [IN] 지우려는 Lob Data의 크기
 **********************************************************************/
IDE_RC smiLob::erase( idvSQL       * aStatistics,
                      smLobLocator   aLobLocator,
                      ULong          aOffset,
                      ULong          aSize )
{
    smxTrans      * sTrans;
    smLobCursor   * sLobCursor;

    IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                    &sLobCursor,
                                    &sTrans )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_MODE,
                    canNotModify );

    IDE_TEST( sLobCursor->mModule->mErase(aStatistics,
                                          sTrans,
                                          &(sLobCursor->mLobViewEnv),
                                          aLobLocator,
                                          aOffset,
                                          aSize)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( canNotModify )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_CanNotModifyLob) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob을 aOffset에서 시작하여 이후 데이터를 잘라내는 함수.
 *
 *     aStatistics - [IN] 통계 자료
 *     aLobLocator - [IN] Trim할 Lob을 가리키는 Lob Locator
 *     aOffset     - [IN] Trim 시작 위치 Offset
 **********************************************************************/
IDE_RC smiLob::trim( idvSQL       * aStatistics,
                     smLobLocator   aLobLocator,
                     UInt           aOffset )
{
    smxTrans      * sTrans;
    smLobCursor   * sLobCursor;

    IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                    &sLobCursor,
                                    &sTrans )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_MODE,
                    canNotModify );

    IDE_TEST( sLobCursor->mModule->mTrim(aStatistics,
                                         sTrans,
                                         &(sLobCursor->mLobViewEnv),
                                         aLobLocator,
                                         aOffset)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( canNotModify )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_CanNotModifyLob) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  LOB 데이터에 갱신 시작 offset을 설정한다.
 *
 * Implementation:
 *
 *     aStatistics - [IN] 통계정보
 *     aLobLocator - [IN] Lob Locator
 *     aOffset     - [IN] 작업을 시작할 offset
 *     aOldSize    - [IN] 기존의 Lob data의 크기
 *     aNewSize    - [IN] 변경 후의 Lob data의 크기
 **********************************************************************/
IDE_RC smiLob::prepare4Write( idvSQL*      aStatistics,
                              smLobLocator aLobLocator,
                              UInt         aOffset,
                              UInt         aNewSize )
{
    smxTrans      * sTrans;
    smLobCursor   * sLobCursor   = NULL;

    IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                    &sLobCursor,
                                    &sTrans )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_MODE,
                    canNotModify );

    IDE_TEST_RAISE( sLobCursor->mLobViewEnv.mWriteError == ID_TRUE,
                    writePrepareProtocolError );

    IDE_TEST_RAISE(
        sLobCursor->mLobViewEnv.mWritePhase == SM_LOB_WRITE_PHASE_PREPARE,
        writePrepareProtocolError );

    sLobCursor->mLobViewEnv.mWritePhase = SM_LOB_WRITE_PHASE_PREPARE;

    IDE_TEST( sLobCursor->mModule->mPrepare4Write(
                  aStatistics,
                  sTrans,
                  &(sLobCursor->mLobViewEnv),
                  aLobLocator,
                  aOffset,
                  aNewSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( canNotModify );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CanNotModifyLob ) );
    }
    IDE_EXCEPTION( writePrepareProtocolError );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_PrePareWriteProtocol ));
    }
    IDE_EXCEPTION_END;

    if( sLobCursor != NULL )
    {
        sLobCursor->mLobViewEnv.mWriteError = ID_TRUE;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : source Lob을  destionation Lob으로  deep copy하는 함수.
 *    1. source Lob, destination lob cursor를 locate시킨다.
 *    2. destionation Lob을 먼저 지운다
 *    3. destination lob에 source lob을 복사하기 전에
 *       source lob의 길이로 resize를 한다.
 *    4. lob piece복사를 수행한다.
 *
 *    aStatistics - [IN] 통계정보
 *    aDestLob    - [IN] 복사 대상 Lob의 Lob Locator
 *    aSrcLob     - [IN] 복사 원본 Lob의 Lob Locator
 **********************************************************************/
IDE_RC smiLob::copy(idvSQL*      aStatistics,
                    smLobLocator aDestLob,
                    smLobLocator aSrcLob)
{
    smxTrans     * sTrans;
    smTID          sTID;
    smLobCursor  * sSrcLobCursorPtr;
    smLobCursor    sSrcLobCursor4Read;
    smLobCursorID  sSrcLobCursorID;
    smLobCursor  * sDestLobCursor;
    smLobCursorID  sDestLobCursorID;
    UInt           sSrcLobLen;
    UInt           sDestLobLen;
    UInt           sReadBytes;
    UInt           sReadLength;
    UInt           i;
    UChar          sLobPiece[smiLob::mLobPieceSize];
    idBool         sSrcIsNullLob;
    idBool         sDestIsNullLob;

    IDE_TEST_RAISE( (SMI_IS_NULL_LOCATOR(aDestLob) == ID_TRUE) ||
                    (SMI_IS_NULL_LOCATOR(aSrcLob)  == ID_TRUE),
                    err_wrong_locator );

    /* src lob */
    sTID = SMI_MAKE_LOB_TRANSID( aSrcLob );
    IDE_TEST_RAISE( sTID != SMI_MAKE_LOB_TRANSID( aDestLob ),
                    missMatchedLobTransID);

    sTrans =  smxTransMgr::getTransByTID(sTID);
    IDE_TEST_RAISE( sTrans->mTransID != sTID, transaction_end );

    sSrcLobCursorID = SMI_MAKE_LOB_CURSORID( aSrcLob );
    IDE_TEST( sTrans->getLobCursor( sSrcLobCursorID,
                                    &sSrcLobCursorPtr,
                                    SMI_IS_SHARD_LOB_LOCATOR(aSrcLob) )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sSrcLobCursorPtr == NULL, srcLobCursor_closed );

    /* update t1 set c1=c1 질의의 경우,
     * lobdesc를 먼저 초기화 한후,
     * lob copy 연산을 수행한다.
     * 이때 src lob cursor는 초기화 되기 전
     * lobdesc 정보를 읽어와야 한다.
     * 그런데 openmode가 READ_WRITE_MODE이면
     * 자신이 갱신한 최신 버전을 읽게 되므로
     * 초기화된 lobdesc 정보를 읽게 된다.
     * 그래서 src lob cursor 정보를 지역변수에 복사한 후,
     * openmode를 READ_MODE로 변경한다. */
    idlOS::memcpy( &sSrcLobCursor4Read,
                   sSrcLobCursorPtr,
                   ID_SIZEOF(smLobCursor) );
    
    sSrcLobCursor4Read.mLobViewEnv.mOpenMode = SMI_LOB_READ_MODE;

    /* desc lob */
    sDestLobCursorID = SMI_MAKE_LOB_CURSORID( aDestLob );
    IDE_TEST( sTrans->getLobCursor(sDestLobCursorID,
                                   &sDestLobCursor,
                                   SMI_IS_SHARD_LOB_LOCATOR(aDestLob))
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sDestLobCursor == NULL, destLobCursor_closed );

    IDE_TEST_RAISE( sDestLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_MODE,
                    canNotModify );

    IDE_TEST(iduCheckSessionEvent(aStatistics)
             != IDE_SUCCESS);

    /* get LOB info */
    IDE_TEST(sDestLobCursor->mModule->mGetLobInfo(
                 aStatistics,
                 sTrans,
                 &(sDestLobCursor->mLobViewEnv),
                 &sDestLobLen,
                 NULL /* Lob In Out Mode */,
                 &sDestIsNullLob)
             != IDE_SUCCESS );

    IDE_TEST( sSrcLobCursor4Read.mModule->mGetLobInfo(
                  aStatistics,
                  sTrans,
                  &(sSrcLobCursor4Read.mLobViewEnv),
                  &sSrcLobLen,
                  NULL /* Lob In Out Mode */,
                  &sSrcIsNullLob)
              != IDE_SUCCESS );

    /* copy */
    if( sSrcIsNullLob != ID_TRUE )
    {
        IDE_TEST( sDestLobCursor->mModule->mPrepare4Write(
                      aStatistics,
                      sTrans,
                      &(sDestLobCursor->mLobViewEnv),
                      aDestLob,
                      (ULong)0,    // Write Offset
                      sSrcLobLen)  // New Size
                  != IDE_SUCCESS );

        sDestLobCursor->mLobViewEnv.mWritePhase = SM_LOB_WRITE_PHASE_COPY;

        if( sSrcLobLen > 0 )
        {
            // write non empty
            sReadBytes = smiLob::mLobPieceSize;
            
            for( i = 0; i < sSrcLobLen; i += smiLob::mLobPieceSize )
            {
                // LOB_PIECE단위로 읽다가 Lob의 잔여부분을 읽을때 처리.
                if( ( i + smiLob::mLobPieceSize ) > sSrcLobLen )
                {
                    sReadBytes = sSrcLobLen - i;
                }

                IDE_TEST(iduCheckSessionEvent(aStatistics)
                         != IDE_SUCCESS);

                IDE_TEST( sSrcLobCursor4Read.mModule->mRead(
                              aStatistics,
                              sTrans,
                              &(sSrcLobCursor4Read.mLobViewEnv),
                              i, /* Read Offset */
                              sReadBytes,
                              sLobPiece,
                              &sReadLength ) != IDE_SUCCESS );
            
                IDE_TEST( sDestLobCursor->mModule->mWrite(
                              aStatistics,
                              sTrans,
                              &(sDestLobCursor->mLobViewEnv),
                              aDestLob,
                              i, /* Write Offset */
                              sReadLength,
                              sLobPiece,
                              ID_TRUE /* aIsFromAPI */,
                              SMR_CT_END) != IDE_SUCCESS );
            }
        }
        else
        {
            IDE_ERROR( sSrcLobLen == 0 );
            
            // write empty
            IDE_TEST( sDestLobCursor->mModule->mWrite(
                          aStatistics,
                          sTrans,
                          &(sDestLobCursor->mLobViewEnv),
                          aDestLob,
                          0, // Write Offset
                          0, // Write Size
                          sLobPiece,
                          ID_TRUE /* aIsFromAPI*/,
                          SMR_CT_END) != IDE_SUCCESS );
        }

        IDE_TEST( finishWrite(aStatistics,
                              aDestLob)
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( transaction_end );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CannotSpanTransByLobLocator,
                                  sTID ));
    }
    IDE_EXCEPTION( srcLobCursor_closed );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_LobCursorClosed,
                                  sTID,
                                  sSrcLobCursorID ));
    }
    IDE_EXCEPTION( destLobCursor_closed );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_LobCursorClosed,
                                  sTID,
                                  sDestLobCursorID ));
    }
    IDE_EXCEPTION( canNotModify );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CanNotModifyLob ));
    }

    IDE_EXCEPTION( missMatchedLobTransID );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_MissMatchedLobTransID,
                                  sTID,
                                  SMI_MAKE_LOB_TRANSID( aDestLob ) ));
    }
    IDE_EXCEPTION( err_wrong_locator )
    {
        IDE_SET( ideSetErrorCode(smERR_IGNORE_CAN_NOT_USE_THIS_LOCATOR) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Column의 Write를 완료하고 완료 Log를 기록한다.
 *     1) prepare4Write
 *     2) write ...
 *        write ...
 *        write ...
 *    3)  finishWrite
 *
 *    aStatistics - [IN] 통계 자료
 *    aLobLocator - [IN] write 를 완료 할 Lob의 Locator
 **********************************************************************/
IDE_RC smiLob::finishWrite(idvSQL*      aStatistics,
                           smLobLocator aLobLocator)
{
    smxTrans       * sTrans;
    smLobCursor    * sLobCursor   = NULL;

    IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                    &sLobCursor,
                                    &sTrans )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_MODE,
                    canNotModify );

    IDE_TEST_RAISE( sLobCursor->mLobViewEnv.mWriteError == ID_TRUE,
                    writeFinishProtocolError );

    IDE_TEST_RAISE(
        (sLobCursor->mLobViewEnv.mWritePhase != SM_LOB_WRITE_PHASE_PREPARE) &&
        (sLobCursor->mLobViewEnv.mWritePhase != SM_LOB_WRITE_PHASE_WRITE) &&
        (sLobCursor->mLobViewEnv.mWritePhase != SM_LOB_WRITE_PHASE_COPY),
        writeFinishProtocolError );

    sLobCursor->mLobViewEnv.mWritePhase = SM_LOB_WRITE_PHASE_FINISH;

    IDE_TEST( sLobCursor->mModule->mFinishWrite(
                  aStatistics,
                  sTrans,
                  &(sLobCursor->mLobViewEnv),
                  aLobLocator )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( canNotModify );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CanNotModifyLob ) );
    }
    IDE_EXCEPTION(writeFinishProtocolError);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_FinishWriteProtocol));
    }
    IDE_EXCEPTION_END;

    if( sLobCursor != NULL )
    {
        sLobCursor->mLobViewEnv.mWriteError = ID_TRUE;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : not null 제약등 QP에서 사용하는 정보를 반환
 *
 *    aLobLocator - [IN]  정보를 확인할 Lob의 Locator
 *    aInfo       - [OUT] not null 제약등 QP에서 사용함.
 **********************************************************************/
IDE_RC smiLob::getInfo(smLobLocator aLobLocator,
                       UInt*        aInfo)
{
    smxTrans*     sTrans;
    smLobCursor*  sLobCursor;

    IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                    &sLobCursor,
                                    &sTrans )
              != IDE_SUCCESS );
    
    *aInfo = sLobCursor->mInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : not null 제약등 QP에서 사용하는 정보를 반환
 *
 *    aLobLocator - [IN]  정보를 확인할 Lob의 Locator
 *    aInfo       - [OUT] not null 제약등 QP에서 사용함.
                          단, getInfo와는 달리 포인터를 리턴한다.
 **********************************************************************/
IDE_RC smiLob::getInfoPtr(smLobLocator aLobLocator,
                          UInt**       aInfo)
{
    smxTrans*     sTrans;
    smLobCursor*  sLobCursor;

    IDE_TEST( getLobCursorAndTrans( aLobLocator,
                                    &sLobCursor,
                                    &sTrans )
              != IDE_SUCCESS );
    
    *aInfo = &(sLobCursor->mInfo);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Lob Locator로 부터 LobCursor와 Trans 정보를 반환
 *      aLobLocator - [IN] Lob의 Locator
 *      aLobCursor  - [OUT] LobCursor를 반환
 *      aTrans      - [OUT] Trans를 반환
 **********************************************************************/
IDE_RC smiLob::getLobCursorAndTrans( smLobLocator      aLobLocator,
                                     smLobCursor    ** aLobCursor,
                                     smxTrans       ** aTrans )
{
    smxTrans      * sTrans       = NULL;
    smLobCursor   * sLobCursor   = NULL;
    smLobCursorID   sLobCursorID = SMI_MAKE_LOB_CURSORID( aLobLocator );
    smTID           sTID         = SMI_MAKE_LOB_TRANSID( aLobLocator );

    IDE_TEST_RAISE( SMI_IS_NULL_LOCATOR(aLobLocator) == ID_TRUE,
                    err_wrong_locator );

    /* to get trans */
    sTrans = smxTransMgr::getTransByTID(sTID);
    IDE_TEST_RAISE( sTrans->mTransID != sTID, transaction_end );

    /* to get lob cusor */
    IDE_TEST( sTrans->getLobCursor(
                    sLobCursorID,
                    &sLobCursor,
                    SMI_IS_SHARD_LOB_LOCATOR( aLobLocator) )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sLobCursor == NULL, lobCursor_closed );

    *aLobCursor = sLobCursor;
    *aTrans     = sTrans;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_wrong_locator )
    {
        IDE_SET( ideSetErrorCode(smERR_IGNORE_CAN_NOT_USE_THIS_LOCATOR) );
    }
    IDE_EXCEPTION( transaction_end );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CannotSpanTransByLobLocator,
                                  sTID ));
    }
    IDE_EXCEPTION( lobCursor_closed );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_LobCursorClosed,
                                  sTID,
                                  sLobCursorID ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
