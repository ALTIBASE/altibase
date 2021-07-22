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


typedef struct mmtLobPutContextQCI
{
    smLobLocator mLocator;
} mmtLobPutContextQCI;

typedef struct mmtLobPutContextMMC
{
    mmcLobLocator *mLocator;
} mmtLobPutContextMMC;

/* PROJ-2047 Strengthening LOB - Removed aOffset */
static IDE_RC lobPutCallbackQCI(cmtVariable * /*aVariable*/,
                                UInt        /*aOffset*/,
                                UInt         aSize,
                                UChar       *aData,
                                void        *aContext)
{
    mmtLobPutContextQCI *sContext = (mmtLobPutContextQCI *)aContext;

    IDE_TEST(qciMisc::lobWrite(NULL, /* idvSQL* */
                               sContext->mLocator,
                               aSize,
                               aData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* PROJ-2047 Strengthening LOB - Removed aOffset */
static IDE_RC lobPutCallbackMMC(cmtVariable * /*aVariable*/,
                                UInt        /*aOffset*/,
                                UInt         aSize,
                                UChar       *aData,
                                void        *aContext)
{
    mmtLobPutContextMMC *sContext = (mmtLobPutContextMMC *)aContext;

    IDE_TEST(mmcLob::write(NULL, /* idvSQL* */
                           sContext->mLocator,
                           aSize,
                           aData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerLobGetSizeResult(cmiProtocolContext *aProtocolContext,
                                     smLobLocator        aLocatorID,
                                     UInt                aLobSize)
{
    cmiProtocol               sProtocol;
    cmpArgDBLobGetSizeResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobGetSizeResult);

    sArg->mLocatorID = aLocatorID;
    sArg->mSize      = aLobSize;

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerLobCharLengthResult(cmiProtocolContext *aProtocolContext,
                                        smLobLocator        aLocatorID,
                                        UInt                aCharLength)
{
    cmiProtocol                  sProtocol;
    cmpArgDBLobCharLengthResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobCharLengthResult);

    sArg->mLocatorID = aLocatorID;
    sArg->mLength    = aCharLength;

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerLobGetResult(cmiProtocolContext *aProtocolContext,
                                 smLobLocator        aLocatorID,
                                 UInt                aOffset,
                                 UChar              *aData,
                                 UInt                aDataSize)
{
    cmiProtocol           sProtocol;
    cmpArgDBLobGetResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobGetResult);

    sArg->mLocatorID = aLocatorID;
    sArg->mOffset    = aOffset;

    IDE_TEST(cmtVariableSetData(&sArg->mData, aData, aDataSize) != IDE_SUCCESS);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
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
    cmiProtocol                         sProtocol;
    cmpArgDBLobGetBytePosCharLenResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobGetBytePosCharLenResult);

    sArg->mLocatorID  = aLocatorID;
    sArg->mOffset     = aByteOffset;
    sArg->mCharLength = aCharLength;

    IDE_TEST(cmtVariableSetData(&sArg->mData, aData, aDataSize) != IDE_SUCCESS);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerLobBytePosResult(cmiProtocolContext *aProtocolContext,
                                     smLobLocator        aLocatorID,
                                     UInt                aByteOffset)
{
    cmiProtocol               sProtocol;
    cmpArgDBLobBytePosResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobBytePosResult);

    sArg->mLocatorID  = aLocatorID;
    sArg->mByteOffset = aByteOffset;

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerLobPutBeginResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol                sProtocol;
    cmpArgDBLobPutBeginResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobPutBeginResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerLobPutEndResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol              sProtocol;
    cmpArgDBLobPutEndResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobPutEndResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerLobFreeResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol            sProtocol;
    cmpArgDBLobFreeResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobFreeResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerLobFreeAllResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol               sProtocol;
    cmpArgDBLobFreeAllResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, LobFreeAllResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::lobGetSizeProtocolA5(cmiProtocolContext *aProtocolContext,
                                            cmiProtocol        *aProtocol,
                                            void               *aSessionOwner,
                                            void               *aUserContext)
{
    cmpArgDBLobGetSizeA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetSize);
    mmcTask            *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread   *sThread = (mmtServiceThread *)aUserContext;
    mmcSession         *sSession;
    UInt                sLobSize;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(NULL, /* idvSQL* */
                                   sArg->mLocatorID,
                                   &sLobSize) != IDE_SUCCESS);

    return answerLobGetSizeResult(aProtocolContext, sArg->mLocatorID, sLobSize);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobGetSize),
                                      0);
}

IDE_RC mmtServiceThread::lobCharLengthProtocolA5(cmiProtocolContext *aProtocolContext,
                                               cmiProtocol        *aProtocol,
                                               void               *aSessionOwner,
                                               void               *aUserContext)
{
    cmpArgDBLobCharLengthA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobCharLength);
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

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(NULL, /* idvSQL* */
                                   sArg->mLocatorID,
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
                                       sArg->mLocatorID,
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

    return answerLobCharLengthResult(aProtocolContext, sArg->mLocatorID, sCharLength);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobCharLength),
                                      0);
}

IDE_RC mmtServiceThread::lobGetProtocolA5(cmiProtocolContext *aProtocolContext,
                                        cmiProtocol        *aProtocol,
                                        void               *aSessionOwner,
                                        void               *aUserContext)
{
    UChar             sBuffer[MMT_LOB_PIECE_SIZE];
    cmpArgDBLobGetA5   *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGet);
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    UInt              sOffset;
    UInt              sRemainSize;
    UInt              sPieceSize;
    UInt              sLobSize;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(NULL, /* idvSQL* */
                                   sArg->mLocatorID,
                                   &sLobSize) != IDE_SUCCESS);

    /* BUG-32194 [sm-disk-collection] The server does not check LOB offset
     * and LOB amounts 
     * mOffset, mSize등의 값은 ID_UINT_MAX (4GB)를 넘어서면 안되며,
     * 두 값의 합 역시 ID_UINT_MAX는 물론 Lob의 크기를 넘어서도 안된다. */
    IDE_TEST_RAISE( ( ( (ULong) sArg->mOffset )
                    + ( (ULong) sArg->mSize   ) )
                    > sLobSize , InvalidRange );

    sOffset     = sArg->mOffset;
    sRemainSize = sArg->mSize;

    do
    {
        sPieceSize = IDL_MIN(MMT_LOB_PIECE_SIZE, sRemainSize);
        IDE_DASSERT(sPieceSize <= MMT_LOB_PIECE_SIZE);

        IDE_TEST(qciMisc::lobRead(sSession->getStatSQL(), /* idvSQL* */
                                  sArg->mLocatorID,
                                  sOffset,
                                  sPieceSize,
                                  sBuffer) != IDE_SUCCESS);

        IDE_TEST(answerLobGetResult(aProtocolContext,
                                    sArg->mLocatorID,
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

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobGet),
                                      0);
}

IDE_RC mmtServiceThread::lobGetBytePosCharLenProtocolA5(
                           cmiProtocolContext *aProtocolContext,
                           cmiProtocol        *aProtocol,
                           void               *aSessionOwner,
                           void               *aUserContext)
{
    UChar             sBuffer[MMT_LOB_PIECE_SIZE];
    cmpArgDBLobGetBytePosCharLenA5
                     *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetBytePosCharLen);
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

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(NULL, /* idvSQL* */
                                   sArg->mLocatorID,
                                   &sLobLength) != IDE_SUCCESS);

    sOffset          = sArg->mOffset; // byte offset
    sRemainCharCount = sArg->mSize;   // character length
    
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
                                   sArg->mLocatorID,
                                   sOffset,
                                   sPieceSize,
                                   sRemainCharCount,
                                   sBuffer,
                                   sLanguage,
                                   &sReadByteLength,
                                   &sReadCharLength) != IDE_SUCCESS);

        IDE_TEST(answerLobGetBytePosCharLenResult(aProtocolContext,
                                     sArg->mLocatorID,
                                     sOffset,
                                     sReadCharLength,
                                     sBuffer,
                                     sReadByteLength) != IDE_SUCCESS);

        sOffset          += sReadByteLength;
        sRemainCharCount -= sReadCharLength;

    } while ((sRemainCharCount > 0) && (sOffset < sLobLength));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobGetBytePosCharLen),
                                      0);
}

IDE_RC mmtServiceThread::lobGetCharPosCharLenProtocolA5(
                             cmiProtocolContext *aProtocolContext,
                             cmiProtocol        *aProtocol,
                             void               *aSessionOwner,
                             void               *aUserContext)
{
    UChar             sBuffer[MMT_LOB_PIECE_SIZE];
    cmpArgDBLobGetCharPosCharLenA5 
                     *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetCharPosCharLen);
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

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(qciMisc::lobGetLength(NULL, /* idvSQL* */
                                   sArg->mLocatorID,
                                   &sLobLength) != IDE_SUCCESS);

    //fix BUG-27378 Code-Sonar UMR, failure 될때 return 값을 무시하면
    //sLanaguate가 unIntialize memory가 된다.
    IDE_TEST( qciMisc::getLanguage(smiGetDBCharSet(), &sLanguage) != IDE_SUCCESS);

    if (sArg->mOffset > 0)
    {
        // 읽을 위치가 처음이 아닌 경우 offset만큼 문자를 skip 한다.

        sOffset          = 0;
        sRemainCharCount = sArg->mOffset;
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
                                       sArg->mLocatorID,
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

    sRemainCharCount = sArg->mSize;   // character length

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
                                   sArg->mLocatorID,
                                   sOffset,
                                   sPieceSize,
                                   sRemainCharCount,
                                   sBuffer,
                                   sLanguage,
                                   &sReadByteLength,
                                   &sReadCharLength) != IDE_SUCCESS);

        IDE_TEST(answerLobGetBytePosCharLenResult(aProtocolContext,
                                                  sArg->mLocatorID,
                                                  sOffset,
                                                  sReadCharLength,
                                                  sBuffer,
                                                  sReadByteLength) != IDE_SUCCESS);

        sOffset          += sReadByteLength;
        sRemainCharCount -= sReadCharLength;

    } while ((sRemainCharCount > 0) && (sOffset < sLobLength));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobGetCharPosCharLen),
                                      0);
}

IDE_RC mmtServiceThread::lobBytePosProtocolA5(cmiProtocolContext *aProtocolContext,
                                            cmiProtocol        *aProtocol,
                                            void               *aSessionOwner,
                                            void               *aUserContext)
{
    UChar             sBuffer[MMT_LOB_PIECE_SIZE];
    cmpArgDBLobBytePosA5 *sArg  = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobBytePos);
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

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    sByteOffset = 0;
    if (sArg->mCharOffset > 0)
    {
        //fix BUG-27378 Code-Sonar UMR, failure 될때 return 값을 무시하면
        //sLanaguate가 unIntialize memory가 된다.
        IDE_TEST( qciMisc::getLanguage(smiGetDBCharSet(), &sLanguage) != IDE_SUCCESS);
        sRemainCharCount = sArg->mCharOffset;
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
                                       sArg->mLocatorID,
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
                                    sArg->mLocatorID,
                                    sByteOffset) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobBytePos),
                                      0);
}

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
IDE_RC mmtServiceThread::lobPutBeginProtocolA5(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *aProtocol,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    cmpArgDBLobPutBeginA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPutBegin);
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmcLobLocator       *sLobLocator;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(sSession->findLobLocator(&sLobLocator, sArg->mLocatorID) != IDE_SUCCESS);

    if (sLobLocator == NULL)
    {
        IDE_TEST(qciMisc::lobPrepare4Write(sSession->getStatSQL(),
                                           sArg->mLocatorID,
                                           sArg->mOffset,
                                           sArg->mNewSize) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(mmcLob::beginWrite(sSession->getStatSQL(),
                                    sLobLocator,
                                    sArg->mOffset,
                                    sArg->mNewSize) != IDE_SUCCESS);
    }

    return answerLobPutBeginResult(aProtocolContext);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobPutBegin),
                                      0);
}

/* PROJ-2047 Strengthening LOB - Removed aOffset */
IDE_RC mmtServiceThread::lobPutProtocolA5(cmiProtocolContext *aProtocolContext,
                                        cmiProtocol        *aProtocol,
                                        void               *aSessionOwner,
                                        void               *aUserContext)
{
    cmpArgDBLobPutA5      *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPut);
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmcLobLocator       *sLobLocator;
    mmtLobPutContextQCI  sContextQCI;
    mmtLobPutContextMMC  sContextMMC;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(sSession->findLobLocator(&sLobLocator, sArg->mLocatorID) != IDE_SUCCESS);

    if (sLobLocator == NULL)
    {
        sContextQCI.mLocator = sArg->mLocatorID;

        IDE_TEST(cmtVariableGetDataWithCallback(&sArg->mData,
                                                lobPutCallbackQCI,
                                                (void *)&sContextQCI) != IDE_SUCCESS);
    }
    else
    {
        sContextMMC.mLocator = sLobLocator;

        IDE_TEST(cmtVariableGetDataWithCallback(&sArg->mData,
                                                lobPutCallbackMMC,
                                                (void *)&sContextMMC) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobPut),
                                      0);
}

IDE_RC mmtServiceThread::lobPutEndProtocolA5(cmiProtocolContext *aProtocolContext,
                                           cmiProtocol        *aProtocol,
                                           void               *aSessionOwner,
                                           void               *aUserContext)
{
    cmpArgDBLobPutEndA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPutEnd);
    mmcTask           *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread  *sThread = (mmtServiceThread *)aUserContext;
    mmcSession        *sSession;
    mmcLobLocator     *sLobLocator;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(sSession->findLobLocator(&sLobLocator, sArg->mLocatorID) != IDE_SUCCESS);

    if (sLobLocator == NULL)
    {
        IDE_TEST(qciMisc::lobFinishWrite(sSession->getStatSQL(), 
                                         sArg->mLocatorID) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(mmcLob::endWrite(sSession->getStatSQL(),
                                  sLobLocator) != IDE_SUCCESS);
    }

    return answerLobPutEndResult(aProtocolContext);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, LobPutEnd),
                                      0);
}

// fix BUG-19407
IDE_RC mmtServiceThread::lobFreeProtocolA5(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    cmpArgDBLobFreeA5  *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobFree);
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    idBool            sFound;
    IDE_RC            sRc = IDE_SUCCESS;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(sSession->freeLobLocator(sArg->mLocatorID, &sFound) != IDE_SUCCESS);

    if (sFound == ID_FALSE)
    {
        IDE_TEST(qciMisc::lobFinalize(NULL, sArg->mLocatorID) != IDE_SUCCESS);
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
        sRc = sThread->answerErrorResultA5(aProtocolContext,
                                         CMI_PROTOCOL_OPERATION(DB, LobFree),
                                         0);
    }

	/* BUG-19138 */
    return sRc;
}

IDE_RC mmtServiceThread::lobFreeAllProtocolA5(cmiProtocolContext *aProtocolContext,
                                            cmiProtocol        *aProtocol,
                                            void               *aSessionOwner,
                                            void               *aUserContext)
{
    cmpArgDBLobFreeAllA5  *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobFreeAll);
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    IDE_RC               sRc = IDE_SUCCESS;
    cmtAny               sAny;
    UInt                 sCursor = 0;
    UChar               *sCollectionData = NULL;
    UInt                 i;
    ULong                sLocatorID;
    idBool               sFound;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    if( cmtCollectionGetType( &sArg->mListData ) == CMT_ID_VARIABLE )
    {
        IDE_TEST( sSession->allocChunk( cmtCollectionGetSize(&sArg->mListData) )
                  != IDE_SUCCESS );

        sCollectionData = sSession->getChunk();
    
        IDE_TEST( cmtCollectionCopyData( &sArg->mListData, sCollectionData )
                  != IDE_SUCCESS );
    }
    else
    {
        sCollectionData = cmtCollectionGetData( &sArg->mListData );
    }

    // bug-27571: klocwork warnings
    // add null check
    if (sArg->mLocatorCount > 0)
    {
        IDE_TEST_RAISE(sCollectionData == NULL, NullPtrErr);
    }

    for( i = 0; i < sArg->mLocatorCount; i++ )
    {
        IDE_TEST( cmtAnyInitialize( &sAny ) != IDE_SUCCESS );
        IDE_TEST( cmtCollectionReadAnyNext( sCollectionData,
                                            &sCursor,
                                            &sAny )
                  != IDE_SUCCESS );

        IDE_TEST( cmtAnyReadULong( &sAny, &sLocatorID ) != IDE_SUCCESS );
        IDE_TEST( sSession->freeLobLocator(sLocatorID, &sFound) != IDE_SUCCESS );

        if (sFound == ID_FALSE)
        {
            IDE_TEST(qciMisc::lobFinalize(NULL, /* idvSQL* */
                                          sLocatorID) != IDE_SUCCESS);
        }
    }
    
    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPC)
    {
        sRc = answerLobFreeAllResult(aProtocolContext);
    }

    return sRc;

    // bug-27571: klocwork warnings
    // save errmsg to boot.log file
    IDE_EXCEPTION(NullPtrErr);
    {
        ideLog::log(IDE_SERVER_0,
                    "warning: lobFreeAllProtocol: no data received");
    }
    IDE_EXCEPTION_END;

    sRc = IDE_SUCCESS;

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPC)
    {
        sRc = sThread->answerErrorResultA5(aProtocolContext,
                                         CMI_PROTOCOL_OPERATION(DB, LobFreeAll),
                                         0);
    }

	/* BUG-19138 */
    return sRc;
}
