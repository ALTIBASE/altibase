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
 * $Id: sdiLob.cpp 88285 2020-08-05 05:49:37Z bethy $
 **********************************************************************/

#include <sdDef.h>
#include <sdi.h>
#include <sdiLob.h>
#include <sdl.h>

IDE_RC sdiLob::open()
{
    return IDE_SUCCESS;
}

IDE_RC sdiLob::close( idvSQL        * aStatistics,
                      void          * aTrans,
                      smLobViewEnv  * aLobViewEnv )
{
    sdiConnectInfo   *sConnectInfo = NULL;
    sdlRemoteStmt    *sRemoteStmt  = NULL;
    smShardLobCursor *sShardLobCursor = aLobViewEnv->mShardLobCursor;

    (void) enter( aStatistics,
                  aTrans,
                  sShardLobCursor,
                  &sConnectInfo,
                  &sRemoteStmt );

    IDE_TEST( sdl::freeLob(
                  sConnectInfo,
                  sRemoteStmt,
                  sShardLobCursor->mRemoteLobLocator,
                  &(sConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
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
IDE_RC sdiLob::read( idvSQL        * aStatistics,
                     void          * aTrans,
                     smLobViewEnv  * aLobViewEnv,
                     UInt            aOffset,
                     UInt            aMount,
                     UChar         * aPiece,
                     UInt          * aReadLength )
{
    sdiConnectInfo   *sConnectInfo = NULL;
    sdlRemoteStmt    *sRemoteStmt  = NULL;
    smShardLobCursor *sShardLobCursor;
    SShort            sTargetCType;

    sShardLobCursor = aLobViewEnv->mShardLobCursor;

    (void) enter( aStatistics,
                  aTrans,
                  sShardLobCursor,
                  &sConnectInfo,
                  &sRemoteStmt );

    if ( sShardLobCursor->mLobLocatorType == MTD_CLOB_LOCATOR_ID )
    {
        sTargetCType = MTD_CHAR_ID;
    }
    else
    {
        sTargetCType = (SShort) MTD_BINARY_ID;
    }
    IDE_TEST( sdl::getLob(
                  sConnectInfo,
                  sRemoteStmt,
                  sShardLobCursor->mLobLocatorType,
                  sShardLobCursor->mRemoteLobLocator,
                  aOffset,
                  aMount,
                  sTargetCType,
                  (void *) aPiece,
                  aMount,
                  aReadLength,
                  &(sConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

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
IDE_RC sdiLob::write( idvSQL       * aStatistics,
                      void         * aTrans,
                      smLobViewEnv * aLobViewEnv,
                      smLobLocator   aLobLocator,
                      UInt           aOffset,
                      UInt           aPieceLen,
                      UChar        * aPiece,
                      idBool        /* aIsFromAPI */,
                      UInt          /* aContType */ )
{
    SD_UNUSED( aLobLocator );
    SD_UNUSED( aOffset );

    sdiConnectInfo   *sConnectInfo = NULL;
    sdlRemoteStmt    *sRemoteStmt  = NULL;
    smShardLobCursor *sShardLobCursor;
    SShort            sTargetCType;

    sShardLobCursor = aLobViewEnv->mShardLobCursor;

    (void) enter( aStatistics,
                  aTrans,
                  sShardLobCursor,
                  &sConnectInfo,
                  &sRemoteStmt );

    if ( sShardLobCursor->mLobLocatorType == MTD_CLOB_LOCATOR_ID )
    {
        sTargetCType = MTD_CHAR_ID;
    }
    else
    {
        sTargetCType = (SShort) MTD_BINARY_ID;
    }
    IDE_TEST( sdl::lobWrite(
                  sConnectInfo,
                  sRemoteStmt,
                  sShardLobCursor->mLobLocatorType,
                  sShardLobCursor->mRemoteLobLocator,
                  sTargetCType,
                  (void *) aPiece,
                  aPieceLen,
                  &(sConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 이 코드는 추 후 필요한 기능으로 판단되어 구현되었습니다.
 * 사용전에 테스트가 필요합니다.
 */
IDE_RC sdiLob::erase( idvSQL       * aStatistics,
                      void         * aTrans,
                      smLobViewEnv * aLobViewEnv,
                      smLobLocator   aLobLocator,
                      UInt           aOffset,
                      UInt           aPieceLen )
{
    SD_UNUSED( aStatistics );
    SD_UNUSED( aTrans );
    SD_UNUSED( aLobViewEnv );
    SD_UNUSED( aLobLocator );
    SD_UNUSED( aOffset );
    SD_UNUSED( aPieceLen );

    return IDE_SUCCESS;
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
IDE_RC sdiLob::prepare4Write( idvSQL*       aStatistics,
                              void*         aTrans,
                              smLobViewEnv* aLobViewEnv,
                              smLobLocator  aLobLocator,
                              UInt          aOffset,
                              UInt          aNewSize )
{
    SD_UNUSED( aLobLocator );

    sdiConnectInfo   *sConnectInfo = NULL;
    sdlRemoteStmt    *sRemoteStmt  = NULL;
    smShardLobCursor *sShardLobCursor;

    sShardLobCursor = aLobViewEnv->mShardLobCursor;

    (void) enter( aStatistics,
                  aTrans,
                  sShardLobCursor,
                  &sConnectInfo,
                  &sRemoteStmt );

    IDE_TEST( sdl::lobPrepare4Write(
                  sConnectInfo,
                  sRemoteStmt,
                  sShardLobCursor->mLobLocatorType,
                  sShardLobCursor->mRemoteLobLocator,
                  aOffset,
                  aNewSize,
                  &(sConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

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
IDE_RC sdiLob::finishWrite( idvSQL       * aStatistics,
                            void         * aTrans,
                            smLobViewEnv * aLobViewEnv,
                            smLobLocator   aLobLocator )
{
    SD_UNUSED( aLobLocator );

    sdiConnectInfo   *sConnectInfo = NULL;
    sdlRemoteStmt    *sRemoteStmt  = NULL;
    smShardLobCursor *sShardLobCursor;

    sShardLobCursor = aLobViewEnv->mShardLobCursor;

    (void) enter( aStatistics,
                  aTrans,
                  sShardLobCursor,
                  &sConnectInfo,
                  &sRemoteStmt );

    IDE_TEST( sdl::lobFinishWrite(
                  sConnectInfo,
                  sRemoteStmt,
                  sShardLobCursor->mLobLocatorType,
                  sShardLobCursor->mRemoteLobLocator,
                  &(sConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

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
IDE_RC sdiLob::getLobInfo( idvSQL*        aStatistics,
                           void*          aTrans,
                           smLobViewEnv*  aLobViewEnv,
                           UInt*          aLobLen,
                           UInt*          aLobMode,
                           idBool*        aIsNullLob )
{
    sdiConnectInfo   *sConnectInfo = NULL;
    sdlRemoteStmt    *sRemoteStmt  = NULL;
    smShardLobCursor *sShardLobCursor;

    sShardLobCursor = aLobViewEnv->mShardLobCursor;

    (void) enter( aStatistics,
                  aTrans,
                  sShardLobCursor,
                  &sConnectInfo,
                  &sRemoteStmt );

    if (sShardLobCursor->mRemoteLobLocator == MTD_LOCATOR_NULL )
    {
        *aLobLen = 0;
        *aIsNullLob = ID_TRUE;
    }
    else
    {
        *aIsNullLob = ID_FALSE;

        IDE_TEST( sdl::getLobLength(
                      sConnectInfo,
                      sRemoteStmt,
                      sShardLobCursor->mLobLocatorType,
                      sShardLobCursor->mRemoteLobLocator,
                      aLobLen,
                      aIsNullLob,
                      &(sConnectInfo->mLinkFailure) )
                  != IDE_SUCCESS );
    }

    if ( aLobMode != NULL )
    {
        *aLobMode = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiLob::trim( idvSQL       * aStatistics,
                     void         * aTrans,
                     smLobViewEnv * aLobViewEnv,
                     smLobLocator   aLobLocator,
                     UInt           aOffset )
{
    SD_UNUSED( aLobLocator );

    sdiConnectInfo   *sConnectInfo = NULL;
    sdlRemoteStmt    *sRemoteStmt  = NULL;
    smShardLobCursor *sShardLobCursor;

    sShardLobCursor = aLobViewEnv->mShardLobCursor;

    (void) enter( aStatistics,
                  aTrans,
                  sShardLobCursor,
                  &sConnectInfo,
                  &sRemoteStmt );

    IDE_TEST( sdl::trimLob(
                  sConnectInfo,
                  sRemoteStmt,
                  sShardLobCursor->mLobLocatorType,
                  sShardLobCursor->mRemoteLobLocator,
                  aOffset,
                  &(sConnectInfo->mLinkFailure) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiLob::writeLog4CursorOpen( idvSQL       * aStatistics,
                                    void         * aTrans,
                                    smLobLocator   aLobLocator,
                                    smLobViewEnv * aLobViewEnv )
{
    SD_UNUSED( aStatistics );
    SD_UNUSED( aTrans );
    SD_UNUSED( aLobLocator );
    SD_UNUSED( aLobViewEnv );

    return IDE_SUCCESS;
}

IDE_RC sdiLob::put( sdiConnectInfo   *aConnectInfo,
                    sdlRemoteStmt    *aRemoteStmt,
                    UInt              aLocatorType,
                    smLobLocator      aLobLocator,
                    void             *aValue,
                    SLong             aLength )
{
    SShort    sSourceCType;

    if ( aLocatorType == MTD_CLOB_LOCATOR_ID )
    {
        sSourceCType = MTD_CHAR_ID;
    }
    else
    {
        sSourceCType = (SShort) MTD_BINARY_ID;
    }

    if ( aLength == 0 )
    {
        IDE_TEST( sdl::putEmptyLob(
                      aConnectInfo,
                      aRemoteStmt,
                      aLocatorType,
                      aLobLocator,
                      sSourceCType,
                      aValue,
                      &(aConnectInfo->mLinkFailure) )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdl::putLob(
                      aConnectInfo,
                      aRemoteStmt,
                      aLocatorType,
                      aLobLocator,
                      sSourceCType,
                      aValue,
                      aLength,
                      &(aConnectInfo->mLinkFailure) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
