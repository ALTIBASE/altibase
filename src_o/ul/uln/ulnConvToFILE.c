/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnConv.h>
#include <ulnConvToFILE.h>

/* 
 * PROJ-2047 Strengthening LOB - Partial Converting
 */
ACI_RC ulncBLOB_FILE(ulnFnContext  *aFnContext,
                     ulnAppBuffer  *aAppBuffer,
                     ulnColumn     *aColumn,
                     ulnLengthPair *aLength,
                     acp_uint16_t   aRowNumber)
{
    ulnPtContext *sPtContext = (ulnPtContext *)aFnContext->mArgs;

    acp_uint8_t  *sFileName  = NULL;
    acp_uint32_t  sFileOption;

    ulnLob       *sLob;
    ulnLobBuffer  sLobBuffer;

    sLob = (ulnLob *)aColumn->mBuffer;

    /*
     * ulnLobBuffer 준비
     */

    sFileName   = aAppBuffer->mBuffer;
    sFileOption = *(aAppBuffer->mFileOptionPtr);

    ACI_TEST_RAISE(aAppBuffer->mBuffer == NULL, LABEL_MEM_MAN_ERR);

    ACI_TEST_RAISE(ulnLobBufferInitialize(&sLobBuffer,
                                          NULL,
                                          sLob->mType,
                                          ULN_CTYPE_FILE,
                                          sFileName,
                                          sFileOption) != ACI_SUCCESS,
                   LABEL_MEM_MAN_ERR);

    ACI_TEST(sLobBuffer.mOp->mPrepare(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * open LOB 및 get data
     */

    ACI_TEST(sLob->mOp->mOpen(aFnContext, sPtContext, sLob) != ACI_SUCCESS);

    /*
     * ulnLobGetData() 함수가 호출됨.
     */

    ACI_TEST(sLob->mOp->mGetData(aFnContext,
                                 sPtContext,
                                 sLob,
                                 &sLobBuffer,
                                 0,
                                 sLob->mSize) != ACI_SUCCESS);

    /*
     * 읽어와서 쓰여진 LOB 데이터의 사이즈를 사용자에게 반환
     */

    aLength->mWritten = sLob->mSizeRetrieved;
    aLength->mNeeded  = sLob->mSizeRetrieved;

    /*
     * ulnLobBuffer 정리
     */

    ACI_TEST(sLobBuffer.mOp->mFinalize(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * close LOB :
     *      1. scrollable 커서일 때는 커서가 닫힐 때.
     *         ulnCursorClose() 함수에서
     *      2. forward only 일 때에는 캐쉬 미스가 발생했을 때.
     *         ulnFetchFromCache() 함수에서
     *
     *      ulnCacheCloseLobInCurrentContents() 를 호출해서 종료시킴.
     */

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnFetchGetLobColumnData");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncCLOB_FILE(ulnFnContext  *aFnContext,
                     ulnAppBuffer  *aAppBuffer,
                     ulnColumn     *aColumn,
                     ulnLengthPair *aLength,
                     acp_uint16_t   aRowNumber)
{
    return ulncBLOB_FILE(aFnContext,
                         aAppBuffer,
                         aColumn,
                         aLength,
                         aRowNumber);
}

