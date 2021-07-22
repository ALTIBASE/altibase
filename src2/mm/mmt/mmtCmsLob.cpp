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

#include <cm.h>
#include <qci.h>
#include <mmErrorCode.h>
#include <mmcLob.h>
#include <mmcSession.h>
#include <mmtServiceThread.h>

#define MMT_LOB_PIECE_SIZE   32000

static IDE_RC answerLobGetSizeResult(cmiProtocolContext *aProtocolContext,
                                     smLobLocator        aLocatorID,
                                     UInt                aLobSize,
                                     idBool              aIsNullLob)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    switch (aProtocolContext->mProtocol.mOpID)
    {
        /* PROJ-2728 Sharding LOB
         *   LobGetSizeV3 Protocol: NullLob 여부를 표시하는 1 byte 추가 */
        case CMP_OP_DB_LobGetSizeV3:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 14);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_LobGetSizeV3Result);
            CMI_WR8(aProtocolContext, &aLocatorID);
            CMI_WR4(aProtocolContext, &aLobSize);
            CMI_WR1(aProtocolContext, aIsNullLob);
            break;

        case CMP_OP_DB_LobGetSize:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 13);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_LobGetSizeResult);
            CMI_WR8(aProtocolContext, &aLocatorID);
            CMI_WR4(aProtocolContext, &aLobSize);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerLobCharLengthResult(cmiProtocolContext *aProtocolContext,
                                        smLobLocator        aLocatorID,
                                        UInt                aCharLength)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 13);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobCharLengthResult);
    CMI_WR8(aProtocolContext, &aLocatorID);
    CMI_WR4(aProtocolContext, &aCharLength);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    return IDE_FAILURE;
}

static IDE_RC answerLobGetResult(cmiProtocolContext *aProtocolContext,
                                 smLobLocator        aLocatorID,
                                 UInt                aOffset,
                                 UChar              *aData,
                                 UInt                aDataSize)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 17 + aDataSize);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobGetResult);
    CMI_WR8(aProtocolContext, &aLocatorID);
    CMI_WR4(aProtocolContext, &aOffset);
    CMI_WR4(aProtocolContext, &aDataSize);
    CMI_WCP(aProtocolContext, aData, aDataSize);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerLobGetBytePosCharLenResult(
                   cmiProtocolContext *aProtocolContext,
                   smLobLocator        aLocatorID,
                   UInt                aByteOffset,
                   UInt                aCharLength,
                   UChar              *aData,
                   UInt                aDataSize)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 21 + aDataSize);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobGetBytePosCharLenResult);
    CMI_WR8(aProtocolContext, &aLocatorID);
    CMI_WR4(aProtocolContext, &aByteOffset);
    CMI_WR4(aProtocolContext, &aCharLength);
    CMI_WR4(aProtocolContext, &aDataSize);
    CMI_WCP(aProtocolContext, aData, aDataSize);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerLobBytePosResult(cmiProtocolContext *aProtocolContext,
                                     smLobLocator        aLocatorID,
                                     UInt                aByteOffset)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 11);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobBytePosResult);
    CMI_WR8(aProtocolContext, &aLocatorID);
    CMI_WR4(aProtocolContext, &aByteOffset);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerLobPutBeginResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobPutBeginResult);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerLobPutEndResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobPutEndResult);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerLobFreeResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobFreeResult);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerLobFreeAllResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobFreeAllResult);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::lobGetSizeProtocol(cmiProtocolContext *aProtocolContext,
                                            cmiProtocol        *aProtocol,
                                            void               *aSessionOwner,
                                            void               *aUserContext)
{
    mmcTask            *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread   *sThread = (mmtServiceThread *)aUserContext;
    mmcSession         *sSession;
    UInt                sLobSize;
    idBool              sIsNullLob;

    ULong       sLocatorID;

    switch (aProtocolContext->mProtocol.mOpID)  /* BUG-48028 */
    {
        case CMP_OP_DB_LobGetSizeV3:
        case CMP_OP_DB_LobGetSize:
            /* PROJ-2160 CM 타입제거
            모두 읽은 다음에 프로토콜을 처리해야 한다. */
            CMI_RD8(aProtocolContext, &sLocatorID);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(sSession->getStatSQL(),
                                   sLocatorID,
                                   &sLobSize,
                                   &sIsNullLob) != IDE_SUCCESS);

    return answerLobGetSizeResult(aProtocolContext, sLocatorID, sLobSize, sIsNullLob);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      aProtocol->mOpID,
                                      0,
                                      sSession);
}

IDE_RC mmtServiceThread::lobCharLengthProtocol(cmiProtocolContext *aProtocolContext,
                                               cmiProtocol        *,
                                               void               *aSessionOwner,
                                               void               *aUserContext)
{
    mmcTask               *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread      *sThread = (mmtServiceThread *)aUserContext;
    mmcSession            *sSession;
    UChar                  sBuffer[MMT_LOB_PIECE_SIZE];
    UInt                   sRemainLength;
    UInt                   sByteOffset = 0;
    UInt                   sPieceSize;
    UInt                   sReadByteLength;
    UInt                   sReadCharLength;
    UInt                   sCharLength = 0;
    mtlModule             *sLanguage;

    ULong       sLocatorID;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(sSession->getStatSQL(),
                                   sLocatorID,
                                   &sRemainLength) != IDE_SUCCESS);

    // fix BUG-22225
    // CLOB 데이터가 없을 경우에는 (NULL or EMPTY)
    // CLOB을 읽지 않고 바로 0을 반환한다.
    if (sRemainLength > 0)
    {
        //fix BUG-27378 Code-Sonar UMR, failure 될때 return 값을 무시하면
        //sLanaguate가 unIntialize memory가 된다.
        IDE_TEST( qciMisc::getLanguage(smiGetDBCharSet(), &sLanguage) != IDE_SUCCESS);

        do
        {
            sPieceSize = IDL_MIN(MMT_LOB_PIECE_SIZE, sRemainLength);
            IDE_DASSERT(sPieceSize <= MMT_LOB_PIECE_SIZE);

            IDE_TEST(qciMisc::clobRead(sSession->getStatSQL(),
                                       sLocatorID,
                                       sByteOffset,
                                       sPieceSize,
                                       ID_UINT_MAX,
                                       sBuffer,
                                       sLanguage,
                                       &sReadByteLength,
                                       &sReadCharLength) != IDE_SUCCESS);

            sByteOffset      += sReadByteLength;
            sRemainLength    -= sReadByteLength;
            sCharLength      += sReadCharLength;

        } while (sRemainLength > 0);
    }

    return answerLobCharLengthResult(aProtocolContext, sLocatorID, sCharLength);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobCharLength),
                                      0,
                                      sSession);
}

IDE_RC mmtServiceThread::lobGetProtocol(cmiProtocolContext *aProtocolContext,
                                        cmiProtocol        *,
                                        void               *aSessionOwner,
                                        void               *aUserContext)
{
    UChar             sBuffer[MMT_LOB_PIECE_SIZE];
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    UInt              sPieceSize;
    UInt              sLobSize;

    ULong       sLocatorID;
    UInt        sOffset;
    UInt        sRemainSize;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);
    CMI_RD4(aProtocolContext, &sOffset);
    CMI_RD4(aProtocolContext, &sRemainSize);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(sSession->getStatSQL(),
                                   sLocatorID,
                                   &sLobSize) != IDE_SUCCESS);

    /* PROJ-2728 Sharding LOB
     *   JDBC(lobGetBytePosCharLenProtocol) -> coordinator를 통해서
     *   여기에 오는 경우 sRemainSize가 항상 32K 이므로 조정 필요
     */
    if ( sSession->isShardCoordinatorSession() == ID_TRUE )
    {
        if ( sOffset > sLobSize )
        {
            sRemainSize = 0;
        }
        else
        {
            sRemainSize = IDL_MIN(sRemainSize, sLobSize - sOffset);
        }
    }

    /* BUG-32194 [sm-disk-collection] The server does not check LOB offset
     * and LOB amounts 
     * mOffset, mSize등의 값은 ID_UINT_MAX (4GB)를 넘어서면 안되며,
     * 두 값의 합 역시 ID_UINT_MAX는 물론 Lob의 크기를 넘어서도 안된다. */
    IDE_TEST_RAISE( ( ( (ULong) sOffset )
                    + ( (ULong) sRemainSize   ) )
                    > sLobSize , InvalidRange );

    do
    {
        sPieceSize = IDL_MIN(MMT_LOB_PIECE_SIZE, sRemainSize);
        IDE_DASSERT(sPieceSize <= MMT_LOB_PIECE_SIZE);

        IDE_TEST(qciMisc::lobRead(sSession->getStatSQL(), /* idvSQL* */
                                  sLocatorID,
                                  sOffset,
                                  sPieceSize,
                                  sBuffer) != IDE_SUCCESS);

        IDE_TEST(answerLobGetResult(aProtocolContext,
                                    sLocatorID,
                                    sOffset,
                                    sBuffer,
                                    sPieceSize) != IDE_SUCCESS);

        sOffset     += sPieceSize;
        sRemainSize -= sPieceSize;

    } while (sRemainSize > 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidRange);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_LOB_RANGE));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobGet),
                                      0,
                                      sSession);
}

IDE_RC mmtServiceThread::lobGetBytePosCharLenProtocol(
                           cmiProtocolContext *aProtocolContext,
                           cmiProtocol        *,
                           void               *aSessionOwner,
                           void               *aUserContext)
{
    UChar             sBuffer[MMT_LOB_PIECE_SIZE];
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    UInt              sPieceSize;
    mtlModule        *sLanguage;
    UInt              sReadByteLength;
    UInt              sReadCharLength;
    UInt              sLobLength;
    SInt              sTempPrecision;

    ULong       sLocatorID;
    UInt        sOffset;
    UInt        sRemainCharCount;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);
    CMI_RD4(aProtocolContext, &sOffset);
    CMI_RD4(aProtocolContext, &sRemainCharCount);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(sSession->getStatSQL(),
                                   sLocatorID,
                                   &sLobLength) != IDE_SUCCESS);

    //fix BUG-27378 Code-Sonar UMR, failure 될때 return 값을 무시하면
    //sLanaguate가 unIntialize memory가 된다.
    IDE_TEST( qciMisc::getLanguage(smiGetDBCharSet(), &sLanguage) != IDE_SUCCESS);

    do
    {
        // BUG-21509
        // sRemainCharCount가 값이 클 경우 maxPrecision 값이 음수일 수도 있다.
        // 음수일 경우 sPieceSize는 MMT_LOB_PIECE_SIZE이면 된다.
        sTempPrecision = sLanguage->maxPrecision(sRemainCharCount);
        if (sTempPrecision < 0)
        {
            sPieceSize = MMT_LOB_PIECE_SIZE;
        }
        else
        {
            sPieceSize = IDL_MIN(MMT_LOB_PIECE_SIZE, sTempPrecision);
        }

        IDE_DASSERT(sPieceSize <= MMT_LOB_PIECE_SIZE);

        IDE_TEST(qciMisc::clobRead(sSession->getStatSQL(),
                                   sLocatorID,
                                   sOffset,
                                   sPieceSize,
                                   sRemainCharCount,
                                   sBuffer,
                                   sLanguage,
                                   &sReadByteLength,
                                   &sReadCharLength) != IDE_SUCCESS);

        IDE_TEST(answerLobGetBytePosCharLenResult(aProtocolContext,
                                     sLocatorID,
                                     sOffset,
                                     sReadCharLength,
                                     sBuffer,
                                     sReadByteLength) != IDE_SUCCESS);

        sOffset          += sReadByteLength;
        sRemainCharCount -= sReadCharLength;

    } while ((sRemainCharCount > 0) && (sOffset < sLobLength));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobGetBytePosCharLen),
                                      0,
                                      sSession);
}

IDE_RC mmtServiceThread::lobGetCharPosCharLenProtocol(
                             cmiProtocolContext *aProtocolContext,
                             cmiProtocol        *,
                             void               *aSessionOwner,
                             void               *aUserContext)
{
    UChar             sBuffer[MMT_LOB_PIECE_SIZE];
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    UInt              sOffset;
    UInt              sRemainCharCount;
    UInt              sPieceSize;
    mtlModule        *sLanguage;
    UInt              sReadByteLength;
    UInt              sReadCharLength;
    UInt              sLobLength;
    SInt              sTempPrecision;

    ULong       sLocatorID;
    UInt        sReadOffset;
    UInt        sSize;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);
    CMI_RD4(aProtocolContext, &sReadOffset);
    CMI_RD4(aProtocolContext, &sSize);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(sSession->getStatSQL(),
                                   sLocatorID,
                                   &sLobLength) != IDE_SUCCESS);

    //fix BUG-27378 Code-Sonar UMR, failure 될때 return 값을 무시하면
    //sLanaguate가 unIntialize memory가 된다.
    IDE_TEST( qciMisc::getLanguage(smiGetDBCharSet(), &sLanguage) != IDE_SUCCESS);

    if (sReadOffset > 0)
    {
        // 읽을 위치가 처음이 아닌 경우 offset만큼 문자를 skip 한다.

        sOffset          = 0;
        sRemainCharCount = sReadOffset;
        do
        {
            // BUG-21509 
            sTempPrecision = sLanguage->maxPrecision(sRemainCharCount);
            if (sTempPrecision < 0)
            {
                sPieceSize = MMT_LOB_PIECE_SIZE;
            }
            else
            {
                sPieceSize = IDL_MIN(MMT_LOB_PIECE_SIZE, sTempPrecision);
            }
            IDE_DASSERT(sPieceSize <= MMT_LOB_PIECE_SIZE);

            IDE_TEST(qciMisc::clobRead(sSession->getStatSQL(),
                                       sLocatorID,
                                       sOffset,
                                       sPieceSize,
                                       sRemainCharCount,
                                       sBuffer,
                                       sLanguage,
                                       &sReadByteLength,
                                       &sReadCharLength) != IDE_SUCCESS);

            sOffset          += sReadByteLength;
            sRemainCharCount -= sReadCharLength;

        } while ((sRemainCharCount > 0) && (sOffset < sLobLength));
    }
    else
    {
        // To Fix BUG-21182
        sOffset = 0;
    }

    sRemainCharCount = sSize;   // character length

    do
    {
        // BUG-21509
        sTempPrecision = sLanguage->maxPrecision(sRemainCharCount);
        if (sTempPrecision < 0)
        {
            sPieceSize = MMT_LOB_PIECE_SIZE;
        }
        else
        {
            sPieceSize = IDL_MIN(MMT_LOB_PIECE_SIZE, sTempPrecision);
        }
        IDE_DASSERT(sPieceSize <= MMT_LOB_PIECE_SIZE);

        IDE_TEST(qciMisc::clobRead(sSession->getStatSQL(),
                                   sLocatorID,
                                   sOffset,
                                   sPieceSize,
                                   sRemainCharCount,
                                   sBuffer,
                                   sLanguage,
                                   &sReadByteLength,
                                   &sReadCharLength) != IDE_SUCCESS);

        IDE_TEST(answerLobGetBytePosCharLenResult(aProtocolContext,
                                                  sLocatorID,
                                                  sOffset,
                                                  sReadCharLength,
                                                  sBuffer,
                                                  sReadByteLength) != IDE_SUCCESS);

        sOffset          += sReadByteLength;
        sRemainCharCount -= sReadCharLength;

    } while ((sRemainCharCount > 0) && (sOffset < sLobLength));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobGetCharPosCharLen),
                                      0,
                                      sSession);
}

IDE_RC mmtServiceThread::lobBytePosProtocol(cmiProtocolContext *aProtocolContext,
                                            cmiProtocol        *,
                                            void               *aSessionOwner,
                                            void               *aUserContext)
{
    UChar             sBuffer[MMT_LOB_PIECE_SIZE];
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    UInt              sByteOffset;
    UInt              sRemainCharCount;
    UInt              sPieceSize;
    mtlModule        *sLanguage;
    UInt              sReadByteLength;
    UInt              sReadCharLength;
    SInt              sTempPrecision;

    ULong       sLocatorID;
    UInt        sCharOffset;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);
    CMI_RD4(aProtocolContext, &sCharOffset);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    sByteOffset = 0;
    if (sCharOffset > 0)
    {
        //fix BUG-27378 Code-Sonar UMR, failure 될때 return 값을 무시하면
        //sLanaguate가 unIntialize memory가 된다.
        IDE_TEST( qciMisc::getLanguage(smiGetDBCharSet(), &sLanguage) != IDE_SUCCESS);
        sRemainCharCount = sCharOffset;
        do
        {
            // BUG-21509
            sTempPrecision = sLanguage->maxPrecision(sRemainCharCount);
            if (sTempPrecision < 0)
            {
                sPieceSize = MMT_LOB_PIECE_SIZE;
            }
            else
            {
                sPieceSize = IDL_MIN(MMT_LOB_PIECE_SIZE, sTempPrecision);
            }
            IDE_DASSERT(sPieceSize <= MMT_LOB_PIECE_SIZE);

            IDE_TEST(qciMisc::clobRead(sSession->getStatSQL(),
                                       sLocatorID,
                                       sByteOffset,
                                       sPieceSize,
                                       sRemainCharCount,
                                       sBuffer,
                                       sLanguage,
                                       &sReadByteLength,
                                       &sReadCharLength) != IDE_SUCCESS);

            sByteOffset      += sReadByteLength;
            sRemainCharCount -= sReadCharLength;

        } while (sRemainCharCount > 0);
    }

    IDE_TEST(answerLobBytePosResult(aProtocolContext,
                                    sLocatorID,
                                    sByteOffset) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobBytePos),
                                      0,
                                      sSession);
}

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
IDE_RC mmtServiceThread::lobPutBeginProtocol(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmcLobLocator       *sLobLocator;

    ULong       sLocatorID;
    UInt        sOffset;
    UInt        sSize;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);
    CMI_RD4(aProtocolContext, &sOffset);
    CMI_RD4(aProtocolContext, &sSize);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(sSession->findLobLocator(&sLobLocator, sLocatorID) != IDE_SUCCESS);

    if (sLobLocator == NULL)
    {
        IDE_TEST(qciMisc::lobPrepare4Write(sSession->getStatSQL(),
                                           sLocatorID,
                                           sOffset,
                                           sSize) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(mmcLob::beginWrite(sSession->getStatSQL(),
                                    sLobLocator,
                                    sOffset,
                                    sSize) != IDE_SUCCESS);
    }

    return answerLobPutBeginResult(aProtocolContext);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobPutBegin),
                                      0,
                                      sSession);
}

/* PROJ-2047 Strengthening LOB - Removed aOffset */
IDE_RC mmtServiceThread::lobPutProtocol(cmiProtocolContext *aProtocolContext,
                                        cmiProtocol        *,
                                        void               *aSessionOwner,
                                        void               *aUserContext)
{
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmcLobLocator       *sLobLocator;

    ULong       sLocatorID;
    UInt        sSize;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);
    CMI_RD4(aProtocolContext, &sSize);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(sSession->findLobLocator(&sLobLocator, sLocatorID) != IDE_SUCCESS);

    if (sLobLocator == NULL)
    {
        IDE_TEST(qciMisc::lobWrite(sSession->getStatSQL(),
                                   sLocatorID,
                                   sSize,
                                   aProtocolContext->mReadBlock->mData +
                                   aProtocolContext->mReadBlock->mCursor) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(mmcLob::write(sSession->getStatSQL(),
                               sLobLocator,
                               sSize,
                               aProtocolContext->mReadBlock->mData +
                               aProtocolContext->mReadBlock->mCursor) != IDE_SUCCESS);
    }

    CMI_SKIP_READ_BLOCK(aProtocolContext, sSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    CMI_SKIP_READ_BLOCK(aProtocolContext, sSize);

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobPut),
                                      0,
                                      sSession);
}

IDE_RC mmtServiceThread::lobPutEndProtocol(cmiProtocolContext *aProtocolContext,
                                           cmiProtocol        *,
                                           void               *aSessionOwner,
                                           void               *aUserContext)
{
    mmcTask           *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread  *sThread = (mmtServiceThread *)aUserContext;
    mmcSession        *sSession;
    mmcLobLocator     *sLobLocator;

    ULong       sLocatorID;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(sSession->findLobLocator(&sLobLocator, sLocatorID) != IDE_SUCCESS);

    if (sLobLocator == NULL)
    {
        IDE_TEST(qciMisc::lobFinishWrite(sSession->getStatSQL(), 
                                         sLocatorID) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(mmcLob::endWrite(sSession->getStatSQL(),
                                  sLobLocator) != IDE_SUCCESS);
    }

    return answerLobPutEndResult(aProtocolContext);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobPutEnd),
                                      0,
                                      sSession);
}

// fix BUG-19407
IDE_RC mmtServiceThread::lobFreeProtocol(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    idBool            sFound;
    IDE_RC            sRc = IDE_SUCCESS;

    ULong             sLocatorID = 0;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorID);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(sSession->freeLobLocator(sLocatorID, &sFound) != IDE_SUCCESS);

    if (sFound == ID_FALSE)
    {
        IDE_TEST(qciMisc::lobFinalize(sSession->getStatSQL(),
                                      sLocatorID) != IDE_SUCCESS);
    }

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPC)
    {
        sRc = answerLobFreeResult(aProtocolContext);
    }

    return sRc;

    IDE_EXCEPTION_END;

    sRc = IDE_SUCCESS;

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPC)
    {
        sRc = sThread->answerErrorResult(aProtocolContext,
                                         CMI_PROTOCOL_OPERATION(DB, LobFree),
                                         0,
                                         sSession);
    }

	/* BUG-19138 */
    return sRc;
}

IDE_RC mmtServiceThread::lobFreeAllProtocol(cmiProtocolContext *aProtocolContext,
                                            cmiProtocol        *,
                                            void               *aSessionOwner,
                                            void               *aUserContext)
{
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    IDE_RC               sRc = IDE_SUCCESS;
    UInt                 i;
    ULong                sLocatorID;
    idBool               sFound;
    UShort               sOrgCursor;

    ULong          sLocatorCount;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD8(aProtocolContext, &sLocatorCount);

    sOrgCursor = aProtocolContext->mReadBlock->mCursor;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    for( i = 0; i < sLocatorCount; i++ )
    {
        CMI_RD8(aProtocolContext, &sLocatorID);

        IDE_TEST( sSession->freeLobLocator(sLocatorID, &sFound) != IDE_SUCCESS );

        if (sFound == ID_FALSE)
        {
            IDE_TEST(qciMisc::lobFinalize(sSession->getStatSQL(),
                                          sLocatorID) != IDE_SUCCESS);
        }
    }

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPC)
    {
        sRc = answerLobFreeAllResult(aProtocolContext);
    }

    return sRc;

    IDE_EXCEPTION_END;

    sRc = IDE_SUCCESS;

    aProtocolContext->mReadBlock->mCursor = sOrgCursor + (sLocatorCount * 8);

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPC)
    {
        sRc = sThread->answerErrorResult(aProtocolContext,
                                         CMI_PROTOCOL_OPERATION(DB, LobFreeAll),
                                         0,
                                         sSession);
    }

    /* BUG-19138 */
    return sRc;
}

/*
 * PROJ-2047 Strengthening LOB - Added Interfaces
 */
static IDE_RC answerLobTrimResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_LobTrimResult);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

/*
 * PROJ-2047 Strengthening LOB - Added Interfaces
 */
IDE_RC mmtServiceThread::lobTrimProtocol(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    UInt              sLobSize;

    ULong             sLocatorID;
    UInt              sOffset;

    CMI_RD8(aProtocolContext, &sLocatorID);
    CMI_RD4(aProtocolContext, &sOffset);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(sSession->getStatSQL(),
                                   sLocatorID,
                                   &sLobSize) != IDE_SUCCESS);

    IDE_TEST_RAISE((ULong)sOffset >= sLobSize, InvalidRange);

    IDE_TEST(qciMisc::lobTrim(sSession->getStatSQL(), /* idvSQL* */
                              sLocatorID,
                              sOffset) != IDE_SUCCESS);

    IDE_TEST(answerLobTrimResult(aProtocolContext) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidRange);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_LOB_RANGE));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobTrim),
                                      0,
                                      sSession);
}
