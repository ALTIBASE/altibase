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
#include <ulnLob.h>
#include <ulnConvChar.h>
#include <ulnConvWChar.h>

/*
 * ========================================
 *
 * ULN LOB BUFFER
 *
 * ========================================
 */

/*
 * -------------------------
 * ulnLobBuffer : Initialize
 * -------------------------
 */

static ACI_RC ulnLobBufferInitializeFILE(ulnLobBuffer *aLobBuffer,
                                         acp_uint8_t  *aFileNamePtr,
                                         acp_uint32_t  aFileOption)
{
    aLobBuffer->mObject.mFile.mFileName           = (acp_char_t *)aFileNamePtr;
    aLobBuffer->mObject.mFile.mFileOption         = aFileOption;
    aLobBuffer->mObject.mFile.mFileHandle.mHandle = ULN_INVALID_HANDLE;
    aLobBuffer->mObject.mFile.mFileSize           = ACP_SINT64_LITERAL(0);
    aLobBuffer->mObject.mFile.mTempBuffer         = NULL;

    return ACI_SUCCESS;
}

static ACI_RC ulnLobBufferInitializeMEMORY(ulnLobBuffer *aLobBuffer,
                                           acp_uint8_t  *aBufferPtr,
                                           acp_uint32_t  aBufferSize)
{
    aLobBuffer->mObject.mMemory.mBuffer        = aBufferPtr;
    aLobBuffer->mObject.mMemory.mBufferSize    = aBufferSize;
    aLobBuffer->mObject.mMemory.mCurrentOffset = 0;

    return ACI_SUCCESS;
}

/*
 * ----------------------
 * ulnLobBuffer : Prepare
 * ----------------------
 */

static ACI_RC ulnLobBufferPrepareMEMORY(ulnFnContext *aFnContext,
                                        ulnLobBuffer *aLobBuffer)
{
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aLobBuffer);

    /*
     * 메모리 버퍼는 반드시 사용자의 바인드메모리이기 때문에 아무것도 해서는 안된다.
     */

    return ACI_SUCCESS;
}

static ACI_RC ulnLobBufferPrepareFILE(ulnFnContext *aFnContext, ulnLobBuffer *aLobBuffer)
{
    ULN_FLAG(sNeedCloseFile);

    acp_offset_t  sFileSize   = 0;
    acp_file_t   *sFileHandle = &aLobBuffer->mObject.mFile.mFileHandle;
    acp_stat_t    sStat;

    /*
     * 파일 오픈
     */
    switch (aLobBuffer->mObject.mFile.mFileOption)
    {
        case SQL_FILE_READ:
            ACI_TEST_RAISE(acpFileOpen(sFileHandle,
                                       aLobBuffer->mObject.mFile.mFileName,
                                       ACP_O_RDONLY,
                                       0) != ACP_RC_SUCCESS,
                           LABEL_FILE_OPEN_ERR);

            ULN_FLAG_UP(sNeedCloseFile);

            /*
             * 파일 사이즈 얻기
             */
            ACI_TEST_RAISE(acpFileStat(sFileHandle, &sStat) != ACP_RC_SUCCESS,
                           LABEL_GET_FILESIZE_ERR);

            sFileSize = sStat.mSize;

            /* BUG-47416 LOB 최대사이즈는 4,294,967,295 bytes (4GB-1byte), 32bit 환경을 위해 캐스팅 하자. */
            ACI_TEST_RAISE((acp_uint64_t)sFileSize > (acp_uint64_t)ACP_UINT32_MAX, LABEL_FILESIZE_TOO_BIG);
            aLobBuffer->mObject.mFile.mFileSize = sFileSize;

            /*
             * 파일에서부터 lob 데이터를 읽어서 전송할 때에는 임시로 쓸 메모리가 필요하다.
             * 파일에 수신한 lob 데이터를 쓸 경우에는 임시 메모리가 필요 없다.
             */
            ACI_TEST_RAISE(aLobBuffer->mObject.mFile.mTempBuffer != NULL, LABEL_MEM_MAN_ERR);

            break;

        case SQL_FILE_CREATE:
            /*
             * Access Mode : 0600 :
             */
            ACI_TEST_RAISE(
                acpFileOpen(sFileHandle,
                            aLobBuffer->mObject.mFile.mFileName,
                            ACP_O_WRONLY | ACP_O_CREAT | ACP_O_EXCL,
                            ACP_S_IRUSR | ACP_S_IWUSR) != ACP_RC_SUCCESS,
                LABEL_FILE_OPEN_ERR_EXIST);

            ULN_FLAG_UP(sNeedCloseFile);

            break;

        case SQL_FILE_OVERWRITE:
            /*
             * BUGBUG : O_CREAT 와 O_EXCL 이 함께 명시되어 있을 경우의 open은 파일이 존재하는지
             *          테스트를 하고 파일을 생성하는 두개의 동작을 atomic 하게 처리하게 된다.
             *          그러나 문제는 이 두 옵션이 함께 설정되어 있으면 파일이 존재할 경우
             *          에러가 리턴된다.
             *
             *          그런데, 아래의 함수에서 O_EXCL 을 추가하자니, 존재하는 파일을 truncate
             *          시키고 덮어 쓰는 게 안되고(에러가 남), O_EXCL 을 빼자니 atomic 한
             *          동작이 되지 않아서 걱정이다.
             *
             *          일단은 파일 존재여부 테스트에서 존재하지 않는것을 판단하고 그 사이에
             *          다른 프로게스가 파일을 같은 이름으로 만들어 버리는 상황이 발생할
             *          가능성은 0 에 근접한다고 보고 그냥 O_EXCL 빼고 만들어버렸다.
             */
            ACI_TEST_RAISE(
                acpFileOpen(sFileHandle,
                            aLobBuffer->mObject.mFile.mFileName,
                            ACP_O_WRONLY | ACP_O_TRUNC | ACP_O_CREAT,
                            ACP_S_IRUSR | ACP_S_IWUSR) != ACP_RC_SUCCESS,
                LABEL_FILE_OPEN_ERR);

            ULN_FLAG_UP(sNeedCloseFile);

            break;

        case SQL_FILE_APPEND:
            ACI_TEST_RAISE(acpFileOpen(sFileHandle,
                                       aLobBuffer->mObject.mFile.mFileName,
                                       ACP_O_WRONLY | ACP_O_APPEND,
                                       0) != ACP_RC_SUCCESS,
                           LABEL_FILE_OPEN_ERR);

            ULN_FLAG_UP(sNeedCloseFile);

            break;

        default:
            ACI_RAISE(LABEL_UNKNOWN_FILE_OPTION);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_FILESIZE_TOO_BIG)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         SQL_COLUMN_NUMBER_UNKNOWN,
                         ulERR_ABORT_FILE_SIZE_TOO_BIG,
                         aLobBuffer->mObject.mFile.mFileName);
    }

    ACI_EXCEPTION(LABEL_UNKNOWN_FILE_OPTION)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         SQL_COLUMN_NUMBER_UNKNOWN,
                         ulERR_ABORT_UNKNOWN_FILE_OPTION,
                         aLobBuffer->mObject.mFile.mFileOption);
    }

    ACI_EXCEPTION(LABEL_FILE_OPEN_ERR)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         SQL_COLUMN_NUMBER_UNKNOWN,
                         ulERR_ABORT_FILE_OPEN,
                         aLobBuffer->mObject.mFile.mFileName);
    }

    ACI_EXCEPTION(LABEL_FILE_OPEN_ERR_EXIST)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         SQL_COLUMN_NUMBER_UNKNOWN,
                         ulERR_ABORT_FILE_EXIST,
                         aLobBuffer->mObject.mFile.mFileName);
    }

    ACI_EXCEPTION(LABEL_GET_FILESIZE_ERR)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         SQL_COLUMN_NUMBER_UNKNOWN,
                         ulERR_ABORT_GET_FILE_SIZE,
                         aLobBuffer->mObject.mFile.mFileName);
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         SQL_COLUMN_NUMBER_UNKNOWN,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnLobBufferPrepareFILE");
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedCloseFile)
    {
        /*
         * 파일 닫기
         */
        if (acpFileClose(sFileHandle) != ACP_RC_SUCCESS)
        {
            ulnErrorExtended(aFnContext,
                             SQL_ROW_NUMBER_UNKNOWN,
                             SQL_COLUMN_NUMBER_UNKNOWN,
                             ulERR_ABORT_FILE_CLOSE,
                             aLobBuffer->mObject.mFile.mFileName);
        }
    }
    return ACI_FAILURE;
}

/*
 * --------------------------------------------------
 * ulnLobBuffer : Data In to local buffer from server
 * --------------------------------------------------
 */

/* 
 * PROJ-2047 Strengthening LOB - Partial Converting
 */
static ACI_RC ulnLobBufferDataInDumpAsCHAR(cmtVariable  *aCmVariable,
                                           acp_uint32_t  aOffset,
                                           acp_uint32_t  aReceivedDataSize,
                                           acp_uint8_t  *aReceivedData,
                                           void         *aContext)
{
    /*
     * 이 함수는 ulncBLOB_CHAR 에서 호출된다.
     * 변환을 한다. 필요한 버퍼의 크기는 가져온 데이터크기 * 2 + 1 이다.
     */

    ulnFnContext *sFnContext = (ulnFnContext *)aContext;
    ulnLob       *sLob       = (ulnLob *)sFnContext->mArgs;
    ulnLobBuffer *sLobBuffer = sLob->mBuffer;

    acp_uint32_t          sLengthConverted;

    ACP_UNUSED(aCmVariable);
    ACP_UNUSED(aOffset);

    /*
     * Note : 주의!!!!! mBuffer 가 NULL 일 수도 있다.
     */

    sLengthConverted
        = ulnConvDumpAsChar(sLobBuffer->mObject.mMemory.mBuffer + (sLob->mSizeRetrieved * 2),
                            sLobBuffer->mObject.mMemory.mBufferSize - (sLob->mSizeRetrieved * 2),
                            aReceivedData,
                            aReceivedDataSize);

    /*
     * 위의 함수 ulnConvDumpAsChar 가 리턴하는 것은 언제나 짝수이며,
     * 항상 null termination 을 위한 여지를 남겨 둔 길이를 리턴한다.
     *
     * 즉, 
     *
     *      *(destination buffer + sLengthConverted) = 0;
     *
     *  과 같이 널 터미네이션을 안전하게 할 수 있다.
     *
     * 그리고, sLob 의 mSizeRetrieved 의 단위는 실제로 서버에서 수신된 데이터의 바이트단위 길이.
     */

    sLob->mSizeRetrieved += sLengthConverted / 2;

    return ACI_SUCCESS;
}

static ACI_RC ulnLobBufferDataInDumpAsWCHAR(cmtVariable  *aCmVariable,
                                            acp_uint32_t  aOffset,
                                            acp_uint32_t  aReceivedDataSize,
                                            acp_uint8_t  *aReceivedData,
                                            void         *aContext)
{
    /*
     * 이 함수는 ulncBLOB_WCHAR 에서 호출된다.
     * 변환을 한다. 필요한 버퍼의 크기는 가져온 데이터크기 * 4 + 1 이다.
     */

    ulnFnContext *sFnContext = (ulnFnContext *)aContext;
    ulnLob       *sLob       = (ulnLob *)sFnContext->mArgs;
    ulnLobBuffer *sLobBuffer = sLob->mBuffer;

    acp_uint32_t          sLengthConverted;

    ACP_UNUSED(aCmVariable);
    ACP_UNUSED(aOffset);

    /*
     * Note : 주의!!!!! mBuffer 가 NULL 일 수도 있다.
     */

    sLengthConverted
        = ulnConvDumpAsWChar(sLobBuffer->mObject.mMemory.mBuffer + (sLob->mSizeRetrieved * 2),
                             sLobBuffer->mObject.mMemory.mBufferSize - (sLob->mSizeRetrieved * 2),
                             aReceivedData,
                             aReceivedDataSize);

    /*
     * 위의 함수 ulnConvDumpAsChar 가 리턴하는 것은 언제나 짝수이며,
     * 항상 null termination 을 위한 여지를 남겨 둔 길이를 리턴한다.
     *
     * 즉,
     *
     *      *(destination buffer + sLengthConverted) = 0;
     *
     *  과 같이 널 터미네이션을 안전하게 할 수 있다.
     *
     * 그리고, sLob 의 mSizeRetrieved 의 단위는 실제로 서버에서 수신된 데이터의 바이트단위 길이.
     */

    sLob->mSizeRetrieved += (sLengthConverted / ACI_SIZEOF(ulWChar)) / 2;

    return ACI_SUCCESS;
}

static ACI_RC ulnLobBufferDataInBINARY(cmtVariable  *aCmVariable,
                                       acp_uint32_t  aOffset,
                                       acp_uint32_t  aReceivedDataSize,
                                       acp_uint8_t  *aReceivedData,
                                       void         *aContext)
{
    /*
     * 이 함수는 ulncBLOB_BINARY, ulnc_CLOB_BINARY, ulnc_CLOB_CHAR 에서 호출된다.
     * 변환 없이 그대로 복사한다.
     */

    ulnFnContext *sFnContext = (ulnFnContext *)aContext;
    ulnLob       *sLob       = (ulnLob *)sFnContext->mArgs;
    ulnLobBuffer *sLobBuffer = sLob->mBuffer;

    acp_uint32_t  sBufferSize;
    acp_uint8_t  *sTarget;
    acp_uint32_t  sSizeToCopy;

    ACP_UNUSED(aCmVariable);
    ACP_UNUSED(aOffset);

    sBufferSize = sLobBuffer->mOp->mGetSize(sLobBuffer);

    if (sLob->mSizeRetrieved + aReceivedDataSize > sBufferSize)
    {
        /*
         * 수신된 데이터가 사용자 버퍼 경계를 지난다.
         *
         * 01004 는 conversion 함수를 호출한 함수에서
         * length needed, length written 변수와 데이터 타입을 보고서 결정한다.
         */
        sSizeToCopy = sBufferSize - sLob->mSizeRetrieved;
    }
    else
    {
        /*
         * 수신된 데이터가 사용자 버퍼 경계 안쪽에 들어갈 수 있는 크기이다.
         */
        sSizeToCopy = aReceivedDataSize;
    }

    if (sSizeToCopy > 0)
    {
        /*
         * Note : 주의!!!!! mBuffer 가 NULL 일 수도 있다.
         */
        sTarget = sLob->mBuffer->mObject.mMemory.mBuffer + sLob->mSizeRetrieved;

        acpMemCpy(sTarget, aReceivedData, sSizeToCopy);

        sLob->mSizeRetrieved += sSizeToCopy;
    }

    return ACI_SUCCESS;
}

static ACI_RC ulnLobBufferDataInFILE(cmtVariable * aCmVariable,
                                     acp_uint32_t  aOffset,
                                     acp_uint32_t  aReceivedDataSize,
                                     acp_uint8_t  *aReceivedData,
                                     void         *aContext)
{
    acp_size_t    sWrittenSize = 0;
    ulnFnContext *sFnContext   = (ulnFnContext *)aContext;
    ulnLob       *sLob         = (ulnLob *)sFnContext->mArgs;

    ACP_UNUSED(aCmVariable);
    ACP_UNUSED(aOffset);

    ACI_TEST_RAISE(sLob->mBuffer->mObject.mFile.mFileHandle.mHandle == 
                   ULN_INVALID_HANDLE,
                   LABEL_MEM_MAN_ERR);

    ACI_TEST_RAISE(acpFileWrite(&sLob->mBuffer->mObject.mFile.mFileHandle,
                                aReceivedData,
                                aReceivedDataSize,
                                &sWrittenSize) != ACP_RC_SUCCESS,
                   LABEL_FILE_WRITE_ERR);

    ACI_TEST_RAISE(sWrittenSize != (acp_size_t)aReceivedDataSize, LABEL_FILE_WRITE_ERR);

    sLob->mSizeRetrieved += aReceivedDataSize;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_FILE_WRITE_ERR)
    {
        ulnError(sFnContext, ulERR_ABORT_LOB_FILE_WRITE_ERR);
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnLobBufferFileFinalize");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* 
 * PROJ-2047 Strengthening LOB - Partial Converting
 */
static ACI_RC ulnLobBufferDataInCHAR(cmtVariable  *aCmVariable,
                                     acp_uint32_t  aOffset,
                                     acp_uint32_t  aReceivedDataSize,
                                     acp_uint8_t  *aReceivedData,
                                     void         *aContext)
{
    ulnFnContext *sFnContext = (ulnFnContext *)aContext;
    ulnLob       *sLob       = (ulnLob *)sFnContext->mArgs;
    ulnLobBuffer *sLobBuffer = sLob->mBuffer;

    ulnDbc       *sDbc = NULL;
    acp_uint32_t *sAppBufferOffset = &sLobBuffer->mObject.mMemory.mCurrentOffset;
    acp_uint8_t  *sAppBuffer       = sLobBuffer->mObject.mMemory.mBuffer +
                                     sLobBuffer->mObject.mMemory.mCurrentOffset;
    acp_uint8_t  *sAppBufferCurPtr     = sAppBuffer;
    acp_uint32_t  sAppBufferRemainSize = sLobBuffer->mObject.mMemory.mBufferSize -
                                         sLobBuffer->mObject.mMemory.mCurrentOffset;

    acp_uint8_t   sBuffer[ULN_MAX_CHARSIZE] = {0, };
    acp_uint8_t   sBufferOffset = 0;

    acp_uint32_t  sSrcMaxPrecision  = 0;
    acp_uint32_t  sDestMaxPrecision = 0;
    acp_sint32_t  sRemainSize       = 0;

    mtlModule          *sSrcCharSet  = NULL;
    mtlModule          *sDestCharSet = NULL;
    aciConvCharSetList  sSrcCharSetIndex;
    aciConvCharSetList  sDestCharSetIndex;
    ACI_RC              sRC = ACI_FAILURE;

    ulnCharSet   *sCharSet = &sLobBuffer->mCharSet;

    /* 이전에 수신된 데이터 끝부분 + 새로 수신된 데이터의 시작 포인트 */
    acp_uint8_t  *sMergeDataPrePtr = NULL;
    acp_uint8_t  *sMergeDataPtr    = aReceivedData - sCharSet->mRemainSrcLen;
    acp_uint8_t  *sMergeDataCurPtr = aReceivedData - sCharSet->mRemainSrcLen;
    acp_uint32_t  sMergeDataRemainSize = aReceivedDataSize +
                                         sCharSet->mRemainSrcLen;
    acp_uint8_t  *sMergeDataFencePtr  = sMergeDataCurPtr + sMergeDataRemainSize;

    sLob->mGetSize -= aReceivedDataSize;

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACP_UNUSED(aCmVariable);
    ACP_UNUSED(aOffset);

    sSrcCharSet       = sDbc->mCharsetLangModule;
    sDestCharSet      = sDbc->mClientCharsetLangModule;
    sSrcCharSetIndex  = mtlGetIdnCharSet(sSrcCharSet);
    sDestCharSetIndex = mtlGetIdnCharSet(sDestCharSet);
    sSrcMaxPrecision  = sSrcCharSet->maxPrecision(1);
    sDestMaxPrecision = sDestCharSet->maxPrecision(1);

    /* 
     * Partial Converting
     *
     * 이전에 수신된 데이터 끝부분과 새로 수신된 데이터를 합쳐서 인코딩 한다.
     * 이전 데이터 끝부분은 새로 수신된 데이터의 헤더 영역에 복사되며
     * 이미 헤더는 읽혀졌기 때문에 훼손이 되어도 문제가 없다.
     *
     * 참고로 이전에 수신된 데이터 끝부분은 4byte 이하이고 헤더 사이즈는 16byte이다.
     */
    if (sCharSet->mRemainSrcLen > 0)
    {
        acpMemCpy(sMergeDataCurPtr, 
                  sCharSet->mRemainSrc,
                  sCharSet->mRemainSrcLen);
        sCharSet->mRemainSrcLen = 0;
    }
    else
    {
        /* Nothing */
    }

    ACI_TEST_RAISE(sAppBufferRemainSize == 0, NO_NEED_WORK);
    ACI_TEST_RAISE(sAppBufferRemainSize < sDestMaxPrecision,
                   LABEL_MAY_NOT_ENOUGH_APP_BUFFER)

    while (sMergeDataCurPtr < sMergeDataFencePtr &&
           sAppBufferRemainSize > 0)
    {
        sRemainSize = sDestMaxPrecision;

        ACI_TEST_RAISE(aciConvConvertCharSet(sSrcCharSetIndex,
                                             sDestCharSetIndex,
                                             sMergeDataCurPtr,
                                             sMergeDataRemainSize,
                                             sAppBufferCurPtr,
                                             &sRemainSize,
                                             -1)
                       != ACI_SUCCESS, LABEL_CONVERT_CHARSET_FAILED);

        sMergeDataPrePtr = sMergeDataCurPtr;
        (void)sSrcCharSet->nextCharPtr(&sMergeDataCurPtr, sMergeDataFencePtr);
        sMergeDataRemainSize -= (sMergeDataCurPtr - sMergeDataPrePtr);

        sAppBufferCurPtr     += (sDestMaxPrecision - sRemainSize);
        sAppBufferRemainSize -= (sDestMaxPrecision - sRemainSize);

        /* Partial인 경우 */
        if (sLob->mGetSize != 0 && sMergeDataRemainSize < sSrcMaxPrecision)
        {
            sCharSet->mRemainSrcLen = sMergeDataRemainSize;
            acpMemCpy(sCharSet->mRemainSrc,
                      sMergeDataCurPtr,
                      sMergeDataRemainSize);
            break;
        }
        else if (sAppBufferRemainSize < sDestMaxPrecision)
        {
            ACI_RAISE(LABEL_MAY_NOT_ENOUGH_APP_BUFFER);
        }
        else
        {
            /* Nothing */
        }
    } /* while (sMergeDataCurPtr < sMergeDataFencePtr... */

    sLob->mSizeRetrieved    += (sMergeDataCurPtr - sMergeDataPtr);
    sCharSet->mConvedSrcLen += (sMergeDataCurPtr - sMergeDataPtr);

    sCharSet->mCopiedDesLen += (sAppBufferCurPtr - sAppBuffer);
    sCharSet->mDestLen      += (sAppBufferCurPtr - sAppBuffer);
    *(sAppBufferOffset)     += (sAppBufferCurPtr - sAppBuffer);

    ACI_EXCEPTION_CONT(NO_NEED_WORK);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_MAY_NOT_ENOUGH_APP_BUFFER)
    {
        /* 
         * mDestLen에는 잘린 글자의 전체 사이즈까지 포함해야 한다.
         * aLength->mNeeded에 이 값이 복사된다.
         */
        sCharSet->mDestLen += (sAppBufferCurPtr - sAppBuffer);

        while (sMergeDataCurPtr < sMergeDataFencePtr)
        {
            sRemainSize = sDestMaxPrecision;

            ACI_TEST_RAISE(aciConvConvertCharSet(sSrcCharSetIndex,
                                                 sDestCharSetIndex,
                                                 sMergeDataCurPtr,
                                                 sMergeDataRemainSize,
                                                 sBuffer + sBufferOffset,
                                                 &sRemainSize,
                                                 -1)
                           != ACI_SUCCESS, LABEL_CONVERT_CHARSET_FAILED_SBUFFER);

            sMergeDataPrePtr = sMergeDataCurPtr;
            (void)sSrcCharSet->nextCharPtr(&sMergeDataCurPtr, sMergeDataFencePtr);
            sMergeDataRemainSize -= (sMergeDataCurPtr - sMergeDataPrePtr);

            sBufferOffset += (sDestMaxPrecision - sRemainSize);

            if (sBufferOffset > sAppBufferRemainSize)
            {
                break;
            }
            else
            {
                /* Nothing */
            }
        }

        /* 
         * "랍" KSC5601(B6 F8), UTF-8(EB 9E 8D)
         *
         * App Buffer가 2바이트 남은 경우 EB 9E는 App Buffer에 복사되고
         * 8D는 mRemainTextLen에 복사 되어 다음에 사용자가 LOB 데이터를
         * 요청할 때 전달해 준다. Binding을 하거나 SQLGetData()를 쓰는
         * 경우 글자가 잘려도 데이터를 버퍼에 꽉 채워줘야 한다.
         *
         * GDPosition은 "랍" 다음 글자를 가리킨다. 
         */
        if (sBufferOffset > sAppBufferRemainSize)
        {
            sCharSet->mRemainTextLen = sBufferOffset -
                                       sAppBufferRemainSize;
            acpMemCpy(sCharSet->mRemainText,
                      sBuffer + sAppBufferRemainSize,
                      sCharSet->mRemainTextLen);

            acpMemCpy(sAppBufferCurPtr,
                      sBuffer,
                      sAppBufferRemainSize);

            sAppBufferCurPtr += sAppBufferRemainSize;
        }
        else
        {
            acpMemCpy(sAppBufferCurPtr,
                      sBuffer,
                      sBufferOffset);

            sAppBufferCurPtr += sBufferOffset;
        }

        sCharSet->mDestLen      += sBufferOffset;
        sLob->mSizeRetrieved    += (sMergeDataCurPtr - sMergeDataPtr);
        sCharSet->mConvedSrcLen += (sMergeDataCurPtr - sMergeDataPtr);

        sCharSet->mCopiedDesLen += (sAppBufferCurPtr - sAppBuffer);
        *(sAppBufferOffset)     += (sAppBufferCurPtr - sAppBuffer);

        sRC = ACI_SUCCESS;
    }
    ACI_EXCEPTION(LABEL_CONVERT_CHARSET_FAILED)
    {
        ulnError(sFnContext,
                 ulERR_ABORT_CONVERT_CHARSET_FAILED,
                 sSrcCharSet,
                 sDestCharSet,
                 sMergeDataFencePtr - sMergeDataCurPtr,
                 sAppBufferCurPtr - sLobBuffer->mObject.mMemory.mBuffer);
    }
    ACI_EXCEPTION(LABEL_CONVERT_CHARSET_FAILED_SBUFFER)
    {
        ulnError(sFnContext,
                 ulERR_ABORT_CONVERT_CHARSET_FAILED,
                 sSrcCharSet,
                 sDestCharSet,
                 sMergeDataFencePtr - sMergeDataCurPtr,
                 -1);
    }

    ACI_EXCEPTION_END;

    return sRC;
}

/* 
 * PROJ-2047 Strengthening LOB - Partial Converting
 */
static ACI_RC ulnLobBufferDataInWCHAR(cmtVariable  *aCmVariable,
                                      acp_uint32_t  aOffset,
                                      acp_uint32_t  aReceivedDataSize,
                                      acp_uint8_t  *aReceivedData,
                                      void         *aContext)
{
    ulnFnContext *sFnContext = (ulnFnContext *)aContext;
    ulnLob       *sLob       = (ulnLob *)sFnContext->mArgs;
    ulnLobBuffer *sLobBuffer = sLob->mBuffer;

    ulnDbc       *sDbc = NULL;
    acp_uint32_t *sAppBufferOffset = &sLobBuffer->mObject.mMemory.mCurrentOffset;
    acp_uint8_t  *sAppBuffer       = sLobBuffer->mObject.mMemory.mBuffer + 
                                     sLobBuffer->mObject.mMemory.mCurrentOffset;
    acp_uint8_t  *sAppBufferCurPtr = sAppBuffer;
    acp_uint32_t  sAppBufferRemainSize = sLobBuffer->mObject.mMemory.mBufferSize -
                                         sLobBuffer->mObject.mMemory.mCurrentOffset;

    acp_uint32_t  sSrcMaxPrecision  = 0;
    acp_uint32_t  sDestMaxPrecision = 0;
    acp_sint32_t  sRemainSize       = 0;
    acp_uint8_t   sTempChar         = '\0';

    mtlModule          *sSrcCharSet  = NULL;
    mtlModule          *sDestCharSet = NULL;
    aciConvCharSetList  sSrcCharSetIndex;
    aciConvCharSetList  sDestCharSetIndex;
    ACI_RC              sRC = ACI_FAILURE;

    ulnCharSet   *sCharSet = &sLobBuffer->mCharSet;

    /* 이전에 수신된 데이터 끝부분 + 새로 수신된 데이터의 시작 포인트 */
    acp_uint8_t  *sMergeDataPrePtr = NULL;
    acp_uint8_t  *sMergeDataPtr    = aReceivedData - sCharSet->mRemainSrcLen;
    acp_uint8_t  *sMergeDataCurPtr = aReceivedData - sCharSet->mRemainSrcLen;
    acp_uint32_t  sMergeDataRemainSize = aReceivedDataSize +
                                         sCharSet->mRemainSrcLen;
    acp_uint8_t  *sMergeDataFencePtr  = sMergeDataCurPtr + sMergeDataRemainSize;

    sLob->mGetSize -= aReceivedDataSize;

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACP_UNUSED(aCmVariable);
    ACP_UNUSED(aOffset);

    sSrcCharSet       = sDbc->mCharsetLangModule;
    sDestCharSet      = sDbc->mWcharCharsetLangModule;
    sSrcCharSetIndex  = mtlGetIdnCharSet(sSrcCharSet);
    sDestCharSetIndex = mtlGetIdnCharSet(sDestCharSet);
    sSrcMaxPrecision  = sSrcCharSet->maxPrecision(1);
    sDestMaxPrecision = sDestCharSet->maxPrecision(1);

    /* 
     * Partial Converting
     *
     * 이전에 수신된 데이터 끝부분과 새로 수신된 데이터를 합쳐서 인코딩 한다.
     * 이전 데이터 끝부분은 새로 수신된 데이터의 헤더 영역에 복사되며
     * 이미 헤더는 읽혀졌기 때문에 훼손이 되어도 문제가 없다.
     *
     * 참고로 이전에 수신된 데이터 끝부분은 4byte 이하이고 헤더 사이즈는 16byte이다.
     */
    if (sCharSet->mRemainSrcLen > 0)
    {
        acpMemCpy(sMergeDataCurPtr, 
                  sCharSet->mRemainSrc,
                  sCharSet->mRemainSrcLen);
        sCharSet->mRemainSrcLen = 0;
    }
    else
    {
        /* Nothing */
    }

    ACI_TEST_RAISE(sAppBufferRemainSize == 0, NO_NEED_WORK);

    while (sMergeDataCurPtr < sMergeDataFencePtr &&
           sAppBufferRemainSize > 0)
    {
        sRemainSize = sDestMaxPrecision;

        sRC = aciConvConvertCharSet(sSrcCharSetIndex,
                                    sDestCharSetIndex,
                                    sMergeDataCurPtr,
                                    sMergeDataRemainSize,
                                    sAppBufferCurPtr,
                                    &sRemainSize,
                                    -1);

        ACI_TEST_RAISE(sRC != ACI_SUCCESS, LABEL_CONVERT_CHARSET_FAILED);

#ifndef ENDIAN_IS_BIG_ENDIAN
        sTempChar = sAppBufferCurPtr[0];
        sAppBufferCurPtr[0] = sAppBufferCurPtr[1];
        sAppBufferCurPtr[1] = sTempChar;
#endif

        sMergeDataPrePtr = sMergeDataCurPtr;
        (void)sSrcCharSet->nextCharPtr(&sMergeDataCurPtr, sMergeDataFencePtr);

        sMergeDataRemainSize -= (sMergeDataCurPtr - sMergeDataPrePtr);
        sAppBufferCurPtr     += (sDestMaxPrecision - sRemainSize);
        sAppBufferRemainSize -= (sDestMaxPrecision - sRemainSize);

        /* Partial인 경우 */
        if (sLob->mGetSize != 0 && sMergeDataRemainSize < sSrcMaxPrecision)
        {
            sCharSet->mRemainSrcLen = sMergeDataRemainSize;
            acpMemCpy(sCharSet->mRemainSrc,
                      sMergeDataCurPtr,
                      sMergeDataRemainSize);

            break;
        }
        else
        {
            /* Nothing */
        }
    }

    sCharSet->mCopiedDesLen += (sAppBufferCurPtr - sAppBuffer);
    sCharSet->mDestLen      += (sAppBufferCurPtr - sAppBuffer);
    (*sAppBufferOffset)     += (sAppBufferCurPtr - sAppBuffer);

    sLob->mSizeRetrieved    += (sMergeDataCurPtr - sMergeDataPtr);
    sCharSet->mConvedSrcLen += (sMergeDataCurPtr - sMergeDataPtr);

    ACI_EXCEPTION_CONT(NO_NEED_WORK);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_CONVERT_CHARSET_FAILED)
    {
        ulnError(sFnContext,
                 ulERR_ABORT_CONVERT_CHARSET_FAILED,
                 sSrcCharSet,
                 sDestCharSet,
                 sMergeDataFencePtr - sMergeDataCurPtr,
                 sAppBufferCurPtr - sLobBuffer->mObject.mMemory.mBuffer);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


/*
 * ---------------------------------------------------
 * ulnLobBuffer : Data Out from local buffer to server
 * ---------------------------------------------------
 */

/*
 * Note : 주의사항!! 주의사항!!!
 *
 *                     |--- cmpArgDBLobPut::mOffset = A + B
 *                     |           : DB 에 저장된 LOB 의 시작을 0 으로 했을때의
 *                     |             offset. 즉, 절대 offset
 *                     |
 *                     |  +-- cmpArgDBLobPut::mData
 *                     |  |
 *           |<-- B -->|<-+-->|  DATA : 한번의 cmpArgDBLobPut 으로 보내는 Data
 *           |         |      |
 *           +---------+------+-------------------+
 *           |  새 로  | DATA |  쓰는 데이터 전체 | 업데이트 하고자 하는 Data
 *           +---------+------+-------------------+
 * |<-- A -->|                             ______/
 * |         |                       _____/
 * |         |               _______/
 * | start   |        ______/
 * | offset->|       /
 * +---------+------+------------------------
 * | DB 에 저장되어 있는 LOB 데이터
 * +---------+------+------------------------
 *           |      |
 *           |<---->|
 *              Update 하려고 하는 구간
 */

/* PROJ-2047 Strengthening LOB - Removed aOffset */
static ACI_RC ulnLobAppendCore(ulnFnContext *aFnContext,
                               ulnPtContext *aPtContext,
                               ulnLob       *aLob,
                               acp_uint8_t  *aBuffer,
                               acp_uint32_t  aSize)
{
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx     = &(aPtContext->mCmiPtContext);
    acp_uint64_t         sLobLocatorVal;
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState          = 0;

    ACE_ASSERT(aSize <= ULN_LOB_PIECE_SIZE);

    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_OPENED, LABEL_LOB_NOT_OPENED);

    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));
    sPacket.mOpID = CMP_OP_DB_LobPut;

    CMI_WRITE_CHECK(sCtx, 13 + aSize);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_LobPut);
    CMI_WR8(sCtx, &sLobLocatorVal);
    CMI_WR4(sCtx, &aSize);          // size
    CMI_WCP(sCtx, aBuffer, aSize);

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    /*
     * ulnLob 의 사이즈를 추가시킨 만큼 증가시킨다.
     */
    aLob->mSize += aSize;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_LOB_NOT_OPENED)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_NOT_OPENED, "LobAppend");
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

static ACI_RC ulnLobBufferDataOutBINARY(ulnFnContext *aFnContext,
                                        ulnPtContext *aPtContext,
                                        ulnLob       *aLob)
{
    acp_uint32_t sSourceSize          = aLob->mBuffer->mOp->mGetSize(aLob->mBuffer);
    acp_uint32_t sSourceOffsetToWrite = 0;
    acp_uint32_t sRemainingSize       = 0;
    acp_uint32_t sSizeToSend          = 0;

    while (sSourceOffsetToWrite < aLob->mBuffer->mObject.mMemory.mBufferSize)
    {
        sRemainingSize = sSourceSize - sSourceOffsetToWrite;
        sSizeToSend    = ACP_MIN(sRemainingSize, ULN_LOB_PIECE_SIZE);

        ACI_TEST(ulnLobAppendCore(aFnContext,
                                  aPtContext,
                                  aLob,
                                  aLob->mBuffer->mObject.mMemory.mBuffer + sSourceOffsetToWrite,
                                  sSizeToSend) != ACI_SUCCESS);

        sSourceOffsetToWrite += sSizeToSend;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * PROJ-2047 Strengthening LOB - Partial Converting
 *
 * 파일의 내용을 CM 통신 버퍼에 직접 쓰도록 수정
 */
static ACI_RC ulnLobBufferDataOutFILE(ulnFnContext *aFnContext,
                                      ulnPtContext *aPtContext,
                                      ulnLob       *aLob)
{
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx     = &(aPtContext->mCmiPtContext);
    acp_uint64_t         sLobLocatorVal;

    acp_size_t   sFileSizeRetrieved = 0;
    acp_uint32_t sDataSize          = 0;
    acp_uint16_t sCursor            = 0;
    acp_uint16_t sOrgWriteCursor    = CMI_GET_CURSOR(sCtx);
    acp_uint8_t  sState             = 0;
    acp_rc_t     sRet;

    acp_uint8_t *sSizePtr = NULL;

    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_OPENED, LABEL_LOB_NOT_OPENED);

    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));

    do
    {
        sDataSize = ULN_LOB_PIECE_SIZE;

        sPacket.mOpID = CMP_OP_DB_LobPut;

        CMI_WRITE_CHECK(sCtx, 13 + sDataSize);
        sState = 1;
        sCursor = sCtx->mWriteBlock->mCursor;

        CMI_WOP(sCtx, CMP_OP_DB_LobPut);
        CMI_WR8(sCtx, &sLobLocatorVal);

        /* 데이터 크기 위치는 백업해 두자 */
        sSizePtr = sCtx->mWriteBlock->mData + sCtx->mWriteBlock->mCursor;
        CMI_WR4(sCtx, &sDataSize); // size

        sRet = acpFileRead(&aLob->mBuffer->mObject.mFile.mFileHandle,
                           sCtx->mWriteBlock->mData + sCtx->mWriteBlock->mCursor,
                           ULN_LOB_PIECE_SIZE,
                           &sFileSizeRetrieved);

        if (ACP_RC_NOT_SUCCESS(sRet))
        {
            ACI_TEST_RAISE(ACP_RC_NOT_EOF(sRet), LABEL_FILE_READ_ERR);
        }

        if (sFileSizeRetrieved != 0)
        {
            /* 실제 읽어온 크기로 Overwrite */
            sDataSize = sFileSizeRetrieved;
            CM_ENDIAN_ASSIGN4(sSizePtr, &sDataSize);
            sCtx->mWriteBlock->mCursor += sFileSizeRetrieved;

            ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket)
                     != ACI_SUCCESS);

            /*
             * ulnLob 의 사이즈를 추가시킨 만큼 증가시킨다.
             */
            aLob->mSize += sFileSizeRetrieved;
        }
        else
        {
            /* 파일에서 읽은 데이터가 0이면 mCursor의 위치를 되돌리자 */
            sCtx->mWriteBlock->mCursor = sCursor;
        }
    } while (sFileSizeRetrieved != 0 &&
             aLob->mSize < aLob->mBuffer->mObject.mFile.mFileSize );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_FILE_READ_ERR)
    {
        sCtx->mWriteBlock->mCursor = sCursor;

        ulnError(aFnContext, ulERR_ABORT_LOB_FILE_READ_ERR);
    }
    ACI_EXCEPTION(LABEL_LOB_NOT_OPENED)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_NOT_OPENED, "ulnLobBufferDataOutFILE");
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

/* 
 * PROJ-2047 Strengthening LOB - Partial Converting
 */
static ACI_RC ulnLobBufferDataOutCHAR(ulnFnContext *aFnContext,
                                      ulnPtContext *aPtContext,
                                      ulnLob       *aLob)
{
    cmiProtocolContext  *sCtx     = &(aPtContext->mCmiPtContext);
    acp_uint64_t         sLobLocatorVal;

    ulnDbc       *sDbc = NULL;

    acp_uint8_t  *sSrcPrePtr     = NULL;
    acp_uint8_t  *sSrcCurPtr     = aLob->mBuffer->mObject.mMemory.mBuffer;
    acp_uint32_t  sSrcSize       = aLob->mBuffer->mOp->mGetSize(aLob->mBuffer);
    acp_uint8_t  *sSrcFencePtr   = sSrcCurPtr + sSrcSize;
    acp_uint32_t  sSrcRemainSize = sSrcSize;

    acp_uint8_t  *sDestPtr          = NULL;
    acp_uint32_t  sDestOffset       = 0;
    acp_uint32_t  sDestRemainSize   = 0;
    acp_uint32_t  sDestMaxPrecision = 0;
    acp_sint32_t  sRemainSize       = 0;

    mtlModule          *sSrcCharSet  = NULL;
    mtlModule          *sDestCharSet = NULL;
    aciConvCharSetList  sSrcCharSetIndex;
    aciConvCharSetList  sDestCharSetIndex;
    acp_uint8_t        *sSizePtr        = NULL;
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;
    ACI_RC              sRC             = ACI_FAILURE;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_OPENED, LABEL_LOB_NOT_OPENED);

    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));

    sSrcCharSet       = sDbc->mClientCharsetLangModule;
    sDestCharSet      = sDbc->mCharsetLangModule;
    sSrcCharSetIndex  = mtlGetIdnCharSet(sSrcCharSet);
    sDestCharSetIndex = mtlGetIdnCharSet(sDestCharSet);
    sDestMaxPrecision = sDestCharSet->maxPrecision(1);

    while (sSrcRemainSize > 0)
    {
        sDestOffset = 0;
        sDestRemainSize = ULN_LOB_PIECE_SIZE;

        CMI_WRITE_CHECK(sCtx, 13 + sDestRemainSize);
        sState = 1;

        CMI_WOP(sCtx, CMP_OP_DB_LobPut);
        CMI_WR8(sCtx, &sLobLocatorVal);

        /* 데이터 크기 위치는 백업해 두자 */
        sSizePtr = sCtx->mWriteBlock->mData + sCtx->mWriteBlock->mCursor;
        CMI_WR4(sCtx, &sDestRemainSize); // size

        sDestPtr = sCtx->mWriteBlock->mData + sCtx->mWriteBlock->mCursor;

        while (sDestMaxPrecision < sDestRemainSize &&
               sSrcCurPtr < sSrcFencePtr)
        {
            sRemainSize = sDestMaxPrecision;

            sRC = aciConvConvertCharSet(sSrcCharSetIndex,
                                        sDestCharSetIndex,
                                        sSrcCurPtr,
                                        sSrcRemainSize,
                                        sDestPtr + sDestOffset,
                                        &sRemainSize,
                                        -1);

            ACI_TEST_RAISE(sRC != ACI_SUCCESS, LABEL_CONVERT_CHARSET_FAILED);

            sSrcPrePtr = sSrcCurPtr;
            (void)sSrcCharSet->nextCharPtr(&sSrcCurPtr, sSrcFencePtr);
            sSrcRemainSize -= (sSrcCurPtr - sSrcPrePtr);

            sDestOffset     += (sDestMaxPrecision - sRemainSize);
            sDestRemainSize -= (sDestMaxPrecision - sRemainSize);
        }

        /* 실제 읽어온 크기로 Overwrite */
        CM_ENDIAN_ASSIGN4(sSizePtr, &sDestOffset);
        sCtx->mWriteBlock->mCursor += sDestOffset;

        aLob->mSize += sDestOffset;
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_LOB_NOT_OPENED)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_NOT_OPENED, "ulnLobBufferDataOutCHAR");
    }
    ACI_EXCEPTION(LABEL_CONVERT_CHARSET_FAILED)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_CONVERT_CHARSET_FAILED,
                 sSrcCharSet,
                 sDestCharSet,
                 sSrcFencePtr - sSrcCurPtr,
                 sDestOffset);
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

/* 
 * PROJ-2047 Strengthening LOB - Partial Converting
 */
static ACI_RC ulnLobBufferDataOutWCHAR(ulnFnContext *aFnContext,
                                       ulnPtContext *aPtContext,
                                       ulnLob       *aLob)
{
    cmiProtocolContext  *sCtx     = &(aPtContext->mCmiPtContext);
    acp_uint64_t         sLobLocatorVal;

    ulnDbc       *sDbc = NULL;

    acp_uint8_t  *sSrcPrePtr      = NULL;
    acp_uint8_t  *sSrcCurPtr      = aLob->mBuffer->mObject.mMemory.mBuffer;
    acp_uint32_t  sSrcSize        = aLob->mBuffer->mOp->mGetSize(aLob->mBuffer);
    acp_uint8_t  *sSrcFencePtr    = sSrcCurPtr + sSrcSize;
    acp_uint32_t  sSrcRemainSize  = sSrcSize;
    acp_uint16_t  sOrgWriteCursor = CMI_GET_CURSOR(sCtx);

#ifndef ENDIAN_IS_BIG_ENDIAN
    acp_uint8_t   sSrcWCharBuf[2] = {0, };
#endif

    acp_uint8_t  *sDestPtr          = NULL;
    acp_uint32_t  sDestOffset       = 0;
    acp_uint32_t  sDestRemainSize   = 0;
    acp_uint32_t  sDestMaxPrecision = 0;
    acp_sint32_t  sRemainSize       = 0;

    mtlModule          *sSrcCharSet  = NULL;
    mtlModule          *sDestCharSet = NULL;
    aciConvCharSetList  sSrcCharSetIndex;
    aciConvCharSetList  sDestCharSetIndex;
    acp_uint8_t        *sSizePtr = NULL;
    acp_uint8_t         sState   = 0;
    ACI_RC              sRC = ACI_FAILURE;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_OPENED, LABEL_LOB_NOT_OPENED);

    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));

    sSrcCharSet       = sDbc->mWcharCharsetLangModule;
    sDestCharSet      = sDbc->mCharsetLangModule;
    sSrcCharSetIndex  = mtlGetIdnCharSet(sSrcCharSet);
    sDestCharSetIndex = mtlGetIdnCharSet(sDestCharSet);
    sDestMaxPrecision = sDestCharSet->maxPrecision(1);

    while (sSrcRemainSize > 0)
    {
        sDestOffset = 0;
        sDestRemainSize = ULN_LOB_PIECE_SIZE;

        CMI_WRITE_CHECK(sCtx, 13 + sDestRemainSize);
        sState = 1;

        CMI_WOP(sCtx, CMP_OP_DB_LobPut);
        CMI_WR8(sCtx, &sLobLocatorVal);

        /* 데이터 크기 위치는 백업해 두자 */
        sSizePtr = sCtx->mWriteBlock->mData + sCtx->mWriteBlock->mCursor;
        CMI_WR4(sCtx, &sDestRemainSize); // size

        sDestPtr = sCtx->mWriteBlock->mData + sCtx->mWriteBlock->mCursor;

        while (sDestMaxPrecision < sDestRemainSize &&
               sSrcCurPtr < sSrcFencePtr)
        {
            sRemainSize = sDestMaxPrecision;

#ifndef ENDIAN_IS_BIG_ENDIAN
            /* UTF16 Little Endian이면 Big Endian으로 변환하자 */
            if (sSrcRemainSize >= 2)
            {
                sSrcWCharBuf[0] = sSrcCurPtr[1];
                sSrcWCharBuf[1] = sSrcCurPtr[0];
            }
            else
            {
                /* Nothing */
            }

            sRC = aciConvConvertCharSet(sSrcCharSetIndex,
                                        sDestCharSetIndex,
                                        sSrcWCharBuf,
                                        sSrcRemainSize,
                                        sDestPtr + sDestOffset,
                                        &sRemainSize,
                                        -1);
#else
            sRC = aciConvConvertCharSet(sSrcCharSetIndex,
                                        sDestCharSetIndex,
                                        sSrcCurPtr,
                                        sSrcRemainSize,
                                        sDestPtr + sDestOffset,
                                        &sRemainSize,
                                        -1);
#endif

            ACI_TEST_RAISE(sRC != ACI_SUCCESS, LABEL_CONVERT_CHARSET_FAILED);

            sSrcPrePtr = sSrcCurPtr;
            (void)sSrcCharSet->nextCharPtr(&sSrcCurPtr, sSrcFencePtr);
            sSrcRemainSize -= (sSrcCurPtr - sSrcPrePtr);

            sDestOffset     += (sDestMaxPrecision - sRemainSize);
            sDestRemainSize -= (sDestMaxPrecision - sRemainSize);
        }

        /* 실제 읽어온 크기로 Overwrite */
        CM_ENDIAN_ASSIGN4(sSizePtr, &sDestOffset);
        sCtx->mWriteBlock->mCursor += sDestOffset;

        aLob->mSize += sDestOffset;
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_LOB_NOT_OPENED)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_NOT_OPENED, "ulnLobBufferDataOutWCHAR");
    }
    ACI_EXCEPTION(LABEL_CONVERT_CHARSET_FAILED)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_CONVERT_CHARSET_FAILED,
                 sSrcCharSet,
                 sDestCharSet,
                 sSrcFencePtr - sSrcCurPtr,
                 sDestOffset);
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

/*
 * -----------------------
 * ulnLobBuffer : Finalize
 * -----------------------
 */

static ACI_RC ulnLobBufferFinalizeMEMORY(ulnFnContext *aFnContext,
                                         ulnLobBuffer *aLobBuffer)
{
    ACP_UNUSED(aFnContext);

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    ulnCharSetFinalize(&aLobBuffer->mCharSet);

    return ACI_SUCCESS;
}

static ACI_RC ulnLobBufferFinalizeFILE(ulnFnContext *aFnContext, ulnLobBuffer *aLobBuffer)
{
    /*
     * 임시로 할당했던 메모리 해제
     */

    if (aLobBuffer->mObject.mFile.mTempBuffer != NULL)
    {
        acpMemFree(aLobBuffer->mObject.mFile.mTempBuffer);
        aLobBuffer->mObject.mFile.mTempBuffer = NULL;
    }

    /*
     * 파일 정리
     */

    aLobBuffer->mObject.mFile.mFileSize = ACP_SINT64_LITERAL(0);

    ACI_TEST_RAISE(aLobBuffer->mObject.mFile.mFileHandle.mHandle == 
                   ULN_INVALID_HANDLE, LABEL_MEM_MAN_ERR);
    ACI_TEST_RAISE(
        acpFileClose(&aLobBuffer->mObject.mFile.mFileHandle) != ACP_RC_SUCCESS,
        LABEL_FILE_CLOSE_ERR);

    aLobBuffer->mObject.mFile.mFileHandle.mHandle = ULN_INVALID_HANDLE;

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    ulnCharSetFinalize(&aLobBuffer->mCharSet);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_FILE_CLOSE_ERR)
    {
        ulnError(aFnContext, ulERR_ABORT_FILE_CLOSE, aLobBuffer->mObject.mFile.mFileName);
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnLobBufferFileFinalize");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static acp_uint32_t ulnLobBufferGetSizeMEMORY(ulnLobBuffer *aBuffer)
{
    return aBuffer->mObject.mMemory.mBufferSize;
}

static acp_uint32_t ulnLobBufferGetSizeFILE(ulnLobBuffer *aBuffer)
{
    return aBuffer->mObject.mFile.mFileSize;
}

/*
 * ========================================
 *
 * 외부로 export 되는 함수들
 *
 *      : SQLGetLob
 *      : SQLPutLob
 *      : SQLGetLobLength
 *
 * 등의 함수에서 사용된다.
 *
 * ========================================
 */

ACI_RC ulnLobGetSize(ulnFnContext *aFnContext,
                     ulnPtContext *aPtContext,
                     acp_uint64_t  aLocatorID,
                     acp_uint32_t *aSize,
                     acp_uint16_t *aIsNull)
{
    ulnStmt      *sStmt  = NULL;
    ulnCache     *sCache = NULL;
    ulnLob       *sLob;
    ulnColumn    *sColumn;
    acp_uint32_t  i, j;
    void         *sTempArg;
    ulnDbc       *sDbc;
    ulnRow       *sRow;
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLocatorVal;

    acp_uint32_t  sOffset;
    acp_uint8_t  *sSrc;
    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    acp_uint64_t  sSize;

    /* PROJ-2728 Sharding LOB */
    acp_bool_t    sIsNullLob      = ACP_FALSE;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);


    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    ACI_TEST_RAISE(cmiGetLinkImpl(&aPtContext->mCmiPtContext) == CMI_LINK_IMPL_IPCDA,
                   IPCDANotSupport);

    ACI_TEST_RAISE( aFnContext->mObjType != ULN_OBJ_TYPE_STMT,
                    LABEL_CACHE_MISS );

    sStmt  = aFnContext->mHandle.mStmt;
    sCache = ulnStmtGetCache(sStmt);

    /*
     * Seaching in CACHE
     */
    ACI_TEST_RAISE( ulnCacheHasLob( sCache ) == ACP_FALSE, LABEL_CACHE_MISS );

    /*
     * 0. In Current Position
     */
    for ( i = 1; i <= ulnStmtGetColumnCount(sStmt); i++ )
    {
        sColumn = ulnCacheGetColumn(sCache, i);

        if ( (sColumn->mMtype == ULN_MTYPE_BLOB) ||
             (sColumn->mMtype == ULN_MTYPE_CLOB) )
        {
            sLob = (ulnLob*)sColumn->mBuffer;
            ACI_TEST_RAISE(sLob == NULL, LABEL_FUNCTION_SEQUENCE_ERR);  /* BUG-46889 */

            ULN_GET_LOB_LOCATOR_VALUE(&(sLocatorVal),&(sLob->mLocatorID));
            if ( sLocatorVal == aLocatorID )
            {
                *aSize = sLob->mSize;
                sIsNullLob = sLob->mIsNull;
                ACI_RAISE( LABEL_CACHE_HIT );
            }
        }
    }

    /*
     * PROJ-2047 Strengthening LOB - LOBCACHE
     * 
     * 1. In LOB Cache
     */
    ACI_TEST_RAISE(ulnLobCacheGetLobLength(sStmt->mLobCache,
                                           aLocatorID,
                                           aSize,
                                           &sIsNullLob)
                   == ACI_SUCCESS, LABEL_CACHE_HIT);

    /*
     * 2. In whole CACHE
     */

    // To Fix BUG-20480
    // 논리적 Cursor Position을 이용하여 Cache 내에 존재하는 지를 검사
    // Review : ulnCacheGetRow() 함수를 이용한 물리적 Position 사용도 가능함.
    for ( i = 0; i < (acp_uint32_t)ulnCacheGetRowCount(sCache); i++ )
    {
        sRow = ulnCacheGetRow(sCache, i);
        sOffset = 0;

        for ( j = 1; j <= ulnStmtGetColumnCount(sStmt); j++ )
        {
            sColumn = ulnCacheGetColumn(sCache, j);
            if ( (sColumn->mMtype == ULN_MTYPE_BLOB) ||
                 (sColumn->mMtype == ULN_MTYPE_CLOB) )
            {
                sSrc = sRow->mRow + sOffset;
                CM_ENDIAN_ASSIGN8(&sLocatorVal, sSrc);

                if (sLocatorVal == aLocatorID )
                {
                    sSrc = sRow->mRow + sOffset + 8;

                    /* PROJ-2047 Strengthening LOB - LOBCACHE */
                    CM_ENDIAN_ASSIGN8(&sSize, sSrc);

                    /* PROJ-2728 Sharding LOB */
                    if ( sSize == ACP_ULONG_MAX )
                    {
                        sIsNullLob = ACP_TRUE;
                        *aSize = 0;
                    }
                    else
                    {
                        sIsNullLob = ACP_FALSE;
                        *aSize = sSize;
                    }

                    ACI_RAISE( LABEL_CACHE_HIT );
                }
            }
            // 수신한 원본 데이터에서 다음 column 위치를 구한다.
            ulnDataGetNextColumnOffset(sColumn, sRow->mRow+sOffset, &sOffset);
        }
    }

    ACI_EXCEPTION_CONT( LABEL_CACHE_MISS );

    /*
     * 3. Get LobSize from Server
     */
    ACI_TEST(ulnWriteLobGetSizeREQ(aFnContext,
                                   aPtContext,
                                   aLocatorID) != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(aFnContext,
                              aPtContext) != ACI_SUCCESS);

    sTempArg          = aFnContext->mArgs;
    aFnContext->mArgs = (void *)&sSize;

    ACI_TEST(ulnReadProtocol(aFnContext,
                             aPtContext,
                             sDbc->mConnTimeoutValue) != ACI_SUCCESS);

    /* PROJ-2728 Sharding LOB
     *   ULONG_MAX 값을 NULL로 사용한다 */
    if ( sSize == ACP_ULONG_MAX )
    {
        sIsNullLob = ACP_TRUE;
        sSize = 0;
    }
    else
    {
        sIsNullLob = ACP_FALSE;
    }
    *aSize = sSize;

    aFnContext->mArgs = sTempArg;

    ACI_EXCEPTION_CONT( LABEL_CACHE_HIT );

    /* PROJ-2728 Sharding LOB */
    if ( aIsNull != NULL )
    {
        *aIsNull = sIsNullLob;
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(IPCDANotSupport)
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_UNSUPPORTED_FUNCTION);
    }
    ACI_EXCEPTION(LABEL_FUNCTION_SEQUENCE_ERR)
    {
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnLobGetData(ulnFnContext *aFnContext,
                            ulnPtContext *aPtContext,
                            ulnLob       *aLob,
                            ulnLobBuffer *aLobBuffer,
                            acp_uint32_t  aStartingOffset,
                            acp_uint32_t  aSizeToGet)
{
    void   *sTempArg;
    ulnDbc *sDbc;
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLobLocatorVal;
    acp_uint64_t  sLobLocatorValCache;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    ulnStmt      *sStmt  = NULL;
    ulnCache     *sCache = NULL;
    ulnLob       *sLob;
    ulnColumn    *sColumn;
    acp_uint32_t  i;

    /*
     * 백업
     */
    sTempArg      = aFnContext->mArgs;
    aLob->mBuffer = aLobBuffer;

    /*
     * send LOB GET REQ
     *
     * aStartingOffset :
     *      서버에 저장된 lob 데이터 중에서
     *      가져오고 싶은 데이터의 시작 위치
     */

    aLob->mSizeRetrieved = 0;

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    aLob->mGetSize       = aSizeToGet;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    // fix BUG-18938
    // LOB 데이터의 길이가 0이면 데이터를 요청할 필요가 없다.
    ACI_TEST_RAISE(aSizeToGet == 0, NO_NEED_REQUEST_TO_SERVER);

    ACI_TEST_RAISE(aLobBuffer->mType != ULN_LOB_BUFFER_TYPE_FILE &&
                   aLobBuffer->mOp->mGetSize(aLobBuffer) == 0,
                   NO_NEED_REQUEST_TO_SERVER);

    aFnContext->mArgs = (void *)aLob;

    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal, &(aLob->mLocatorID));                      

    /* PROJ-2728 Sharding LOB
     *   handle이 DBC이면 서버에 요구해야 한다 */
    ACI_TEST_RAISE( aFnContext->mObjType != ULN_OBJ_TYPE_STMT,
                    REQUEST_TO_SERVER );

    sStmt  = aFnContext->mHandle.mStmt;
    sCache = ulnStmtGetCache(sStmt);

    /*
     * Seaching in CACHE
     */
    ACI_TEST_RAISE(ulnCacheHasLob(sCache) == ACP_FALSE, REQUEST_TO_SERVER);

    /*
     * PROJ-2047 Strengthening LOB - LOBCACHE
     */
    for ( i = 1; i <= ulnStmtGetColumnCount(sStmt); i++ )
    {
        sColumn = ulnCacheGetColumn(sCache, i);

        if ( (sColumn->mMtype == ULN_MTYPE_BLOB) ||
             (sColumn->mMtype == ULN_MTYPE_CLOB) )
        {
            sLob = (ulnLob *)sColumn->mBuffer;
            ACI_TEST_RAISE(sLob == NULL, LABEL_FUNCTION_SEQUENCE_ERR);  /* BUG-46889 */

            ULN_GET_LOB_LOCATOR_VALUE(&(sLobLocatorValCache),
                                      &(sLob->mLocatorID));
            if (sLobLocatorValCache == sLobLocatorVal)
            {
                ACI_TEST_RAISE(sLob->mData == NULL, REQUEST_TO_SERVER);

                ACI_TEST_RAISE((acp_uint64_t)aStartingOffset +
                               (acp_uint64_t)aSizeToGet >
                               (acp_uint64_t)sLob->mSize,
                               LABEL_INVALID_LOB_RANGE);

                ACI_TEST_RAISE(
                    aLobBuffer->mOp->mDataIn(NULL,
                                             0,
                                             aSizeToGet,
                                             sLob->mData + aStartingOffset,
                                             aFnContext) != ACI_SUCCESS,
                    REQUEST_TO_SERVER);

                ACI_RAISE(NO_NEED_REQUEST_TO_SERVER);
            }
            else
            {
                /* Nothing */
            }
        }
        else
        {
            /* Nothing */
        }
    }

    /*
     * PROJ-2047 Strengthening LOB - LOBCACHE
     *
     * 서버에 요청하기 전에 LOB CACHE를 확인하자.
     */
    ACI_TEST_RAISE(ulnLobCacheGetLob(sStmt->mLobCache,
                                     sLobLocatorVal,
                                     aStartingOffset,
                                     aSizeToGet,
                                     aFnContext)
                   == ACI_SUCCESS, NO_NEED_REQUEST_TO_SERVER);

    ACI_TEST_RAISE(ulnLobCacheGetErr(sStmt->mLobCache)
                   == LOB_CACHE_INVALID_RANGE, LABEL_INVALID_LOB_RANGE);

    ACI_EXCEPTION_CONT(REQUEST_TO_SERVER);

    /* 서버에 요청하자 */
    ACI_TEST(ulnWriteLobGetREQ(aFnContext,
                               aPtContext,
                               sLobLocatorVal,
                               aStartingOffset,
                               aSizeToGet) != ACI_SUCCESS);
    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    /*
     * receive LOB GET RES : ulnCallbackLobGetResult() 함수에서 계속됨.
     */
    ACI_TEST(ulnReadProtocol(aFnContext,
                             aPtContext,
                             sDbc->mConnTimeoutValue) != ACI_SUCCESS);

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    ACI_EXCEPTION_CONT(NO_NEED_REQUEST_TO_SERVER);

    /*
     * 원복
     */
    aFnContext->mArgs = sTempArg;
    aLob->mBuffer     = NULL;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LOB_RANGE)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_LOB_RANGE);
    }
    ACI_EXCEPTION(LABEL_FUNCTION_SEQUENCE_ERR)
    {
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION_END;

    /*
     * 원복
     */
    aFnContext->mArgs = sTempArg;
    aLob->mBuffer     = NULL;

    return ACI_FAILURE;
}

ACI_RC ulnLobFreeLocator(ulnFnContext *aFnContext, ulnPtContext *aPtContext, acp_uint64_t aLocator)
{
    ulnDbc  *sDbc;
    ulnStmt *sStmt = NULL;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * send LOB FREE REQ
     */
    ACI_TEST(ulnWriteLobFreeREQ(aFnContext, aPtContext, aLocator) != ACI_SUCCESS);

    if (sDbc->mConnType == ULN_CONNTYPE_IPC)
    {
        /*
         * 패킷 전송
         *
         * BUGBUG : 이거 튜닝을 나중에 하자. free 바깥에서 flush 해 주면 더 빠르다.
         */
        ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

        /*
         * receive LOB FREE RES
         */
        ACI_TEST(ulnReadProtocol(aFnContext,
                                 aPtContext,
                                 sDbc->mConnTimeoutValue) != ACI_SUCCESS);
    }

    /* PROJ-2728 Sharding LOB */
    if ( aFnContext->mObjType == ULN_OBJ_TYPE_STMT )
    {
        sStmt = aFnContext->mHandle.mStmt;

        /* PROJ-2047 Strengthening LOB - LOBCACHE */
        ACI_TEST(ulnLobCacheRemove(sStmt->mLobCache, aLocator) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ========================================
 *
 * ULN LOB
 *
 * ========================================
 */

/*
 * ------------------
 * ulnLob : Get State
 * ------------------
 */

static ulnLobState ulnLobGetState(ulnLob *aLob)
{
    return aLob->mState;
}

/*
 * --------------------
 * ulnLob : Set Locator
 * --------------------
 */

static ACI_RC ulnLobSetLocator(ulnFnContext *aFnContext, ulnLob *aLob, acp_uint64_t aLocatorID)
{
    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_INITIALIZED, LABEL_INVALID_STATE);
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_SET_LOB_LOCATOR(&(aLob->mLocatorID),aLocatorID);    
    aLob->mState     = ULN_LOB_ST_LOCATOR;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_STATE)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_INVALID_STATE, "ulnLobSetLocator");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * -------------
 * ulnLob : Open
 * -------------
 */

static ACI_RC ulnLobOpen(ulnFnContext *aFnContext,
                         ulnPtContext *aPtContext,
                         ulnLob       *aLob)
{
    ACP_UNUSED(aPtContext);

    /*
     * BUGBUG : 이부분에서 ULN_LOB_ST_OPENED 를 없애도 되는 방향으로 수정해야 한다.
     *          일단, getdata 에서 어쩔 수 없이 여러번 open 이 호출되는 구조인 관계로
     *          지금은 그냥 통과하도록 했다.
     */

    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_LOCATOR &&
                   aLob->mState != ULN_LOB_ST_OPENED,
                   LABEL_INVALID_STATE);

    aLob->mState = ULN_LOB_ST_OPENED;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_STATE)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_INVALID_STATE, "ulnLobOpen");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * --------------
 * ulnLob : Close
 * --------------
 */

static ACI_RC ulnLobClose(ulnFnContext *aFnContext, ulnPtContext *aPtContext, ulnLob *aLob)
{

    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLobLocatorVal;
    ACI_TEST_RAISE((aLob->mState != ULN_LOB_ST_LOCATOR &&
                    aLob->mState != ULN_LOB_ST_OPENED),
                   LABEL_INVALID_STATE);

    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));
    ACI_TEST(ulnLobFreeLocator(aFnContext, aPtContext, sLobLocatorVal) != ACI_SUCCESS);

    aLob->mState         = ULN_LOB_ST_INITIALIZED;

    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_SET_LOB_LOCATOR(&(aLob->mLocatorID), ACP_SINT64_LITERAL(0));
    aLob->mSizeRetrieved = 0;
    aLob->mSize          = 0;

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    aLob->mGetSize       = 0;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_STATE)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_INVALID_STATE, "ulnLobClose");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ---------------
 * ulnLob : Append
 * ---------------
 */

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
static ACI_RC ulnLobAppendBegin(ulnFnContext *aFnContext,
                                ulnPtContext *aPtContext,
                                ulnLob       *aLob,
                                acp_uint32_t  aSizeToAppend)
{
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLobLocatorVal;
    
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));
    ACI_TEST(ulnWriteLobPutBeginREQ(aFnContext,
                                    aPtContext,
                                    sLobLocatorVal,
                                    aLob->mSize,   /* Offset to Start : LOB 의 맨 끝 */
                                    aSizeToAppend) /* Size to append */
             != ACI_SUCCESS);

    /*
     * Note : Append 하는 것이므로 LOB 사이즈에는 변화가 없다.
     */

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnLobAppendEnd(ulnFnContext *aFnContext, ulnPtContext *aPtContext, ulnLob *aLob)
{
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLobLocatorVal;
    
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));
    ACI_TEST(ulnWriteLobPutEndREQ(aFnContext,
                                  aPtContext,
                                  sLobLocatorVal) != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnLobAppend(ulnFnContext *aFnContext,
                           ulnPtContext *aPtContext,
                           ulnLob       *aLob,
                           ulnLobBuffer *aLobBuffer)
{
    ULN_FLAG(sNeedEndLob);
    ulnDbc *sDbc;

    aLob->mBuffer = aLobBuffer;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_OPENED, LABEL_LOB_NOT_OPENED);

    /*
     * APPEND BEGIN
     */

    ACI_TEST(ulnLobAppendBegin(aFnContext,
                               aPtContext,
                               aLob,
                               aLobBuffer->mOp->mGetSize(aLobBuffer)) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedEndLob);

    /*
     * 데이터 전송
     */

    ACI_TEST(aLobBuffer->mOp->mDataOut(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    /*
     * APPEND END
     */

    ULN_FLAG_DOWN(sNeedEndLob);
    ACI_TEST(ulnLobAppendEnd(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    ACI_TEST(ulnReadProtocol(aFnContext,
                             aPtContext,
                             sDbc->mConnTimeoutValue) != ACI_SUCCESS);

    aLob->mBuffer = NULL;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_LOB_NOT_OPENED)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_NOT_OPENED, "ulnLobAppend");
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedEndLob)
    {
        /*
         * Note : Flush 및 ReadProtocol 은 여기서 안해도 finalize protocol context 에서
         *        알아서 해 준다. 일단 에러가 났으므로 잊어버려도 된다.
         */
        ulnLobAppendEnd(aFnContext, aPtContext, aLob);
    }

    aLob->mBuffer = NULL;

    return ACI_FAILURE;
}

/*
 * ------------------
 * ulnLob : Overwrite
 * ------------------
 */

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
static ACI_RC ulnLobOverWriteBegin(ulnFnContext *aFnContext,
                                   ulnPtContext *aPtContext,
                                   ulnLob       *aLob,
                                   acp_uint32_t  aSizeToOverWrite)
{
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLobLocatorVal;
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));
    ACI_TEST(ulnWriteLobPutBeginREQ(aFnContext,
                                    aPtContext,
                                    sLobLocatorVal,
                                    0,                 /* Offset to Start */
                                    aSizeToOverWrite)  /* OverwriteSize */
             != ACI_SUCCESS);

    /*
     * Note : Overwrite 하는 것이므로 LOB 사이즈를 가상으로 0 으로 초기화시켜야 한다.
     *        ulnLobAppendCore() 함수에서 데이터를 전송하면서
     *        ulnLob::mSize 를 전송한 양만큼 증가시켜서 동기화 시킨다.
     */

    aLob->mSize = 0;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnLobOverWriteEnd(ulnFnContext *aFnContext, ulnPtContext *aPtContext, ulnLob *aLob)
{
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLobLocatorVal;
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));
    ACI_TEST(ulnWriteLobPutEndREQ(aFnContext,
                                  aPtContext,
                                  sLobLocatorVal) != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnLobOverWrite(ulnFnContext *aFnContext,
                              ulnPtContext *aPtContext,
                              ulnLob       *aLob,
                              ulnLobBuffer *aLobBuffer)
{
    ULN_FLAG(sNeedEndLob);
    ulnDbc *sDbc;

    aLob->mBuffer = aLobBuffer;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_OPENED, LABEL_LOB_NOT_OPENED);

    /*
     * OVERWRITE BEGIN
     */

    ACI_TEST(ulnLobOverWriteBegin(aFnContext,
                                  aPtContext,
                                  aLob,
                                  aLobBuffer->mOp->mGetSize(aLobBuffer)) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedEndLob);

    /*
     * 데이터 전송
     */

    ACI_TEST(aLobBuffer->mOp->mDataOut(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    /*
     * OVERWRITE END
     */

    ULN_FLAG_DOWN(sNeedEndLob);
    ACI_TEST(ulnLobOverWriteEnd(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    ACI_TEST(ulnReadProtocol(aFnContext,
                             aPtContext,
                             sDbc->mConnTimeoutValue) != ACI_SUCCESS);

    aLob->mBuffer = NULL;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_LOB_NOT_OPENED)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_NOT_OPENED, "ulnLobOverWrite");
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedEndLob)
    {
        /*
         * Note : Flush 및 ReadProtocol 은 여기서 안해도 finalize protocol context 에서
         *        알아서 해 준다. 일단 에러가 났으므로 잊어버려도 된다.
         */
        ulnLobOverWriteEnd(aFnContext, aPtContext, aLob);
    }

    aLob->mBuffer = NULL;

    return ACI_FAILURE;
}

/*
 * ---------------
 * ulnLob : Update
 * ---------------
 */

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
static ACI_RC ulnLobUpdateBegin(ulnFnContext *aFnContext,
                                ulnPtContext *aPtContext,
                                ulnLob       *aLob,
                                acp_uint32_t  aStartOffset,
                                acp_uint32_t  aNewSize)
{
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLobLocatorVal;
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));

    ACI_TEST(ulnWriteLobPutBeginREQ(aFnContext,
                                    aPtContext,
                                    sLobLocatorVal,
                                    aStartOffset,
                                    aNewSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnLobUpdateEnd(ulnFnContext *aFnContext, ulnPtContext *aPtContext, ulnLob *aLob)
{
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    acp_uint64_t  sLobLocatorVal;
    /* BUG-21932  The ulnLob structure should consider 4 byte alignment for 32bit-GCC */
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));
    
    ACI_TEST(ulnWriteLobPutEndREQ(aFnContext, aPtContext,sLobLocatorVal) != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
static ACI_RC ulnLobUpdate(ulnFnContext *aFnContext,
                           ulnPtContext *aPtContext,
                           ulnLob       *aLob,
                           ulnLobBuffer *aLobBuffer,        /* 업데이트 구간에 새로 채울 데이터 */
                           acp_uint32_t  aStartOffset,      /* 업데이트 시작점 (서버 lob) */
                           acp_uint32_t  aLengthToUpdate)   /* 업데이트 구간의 길이 */
{
    ULN_FLAG(sNeedEndLob);

    ulnDbc       *sDbc;
    acp_uint32_t  sOriginalSize;

    aLob->mBuffer = aLobBuffer;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * Note : 업데이트 되어서 바뀌는 구간에 대한 정보는 update begin 함수에서 전송했으니
     *        나머지는 업데이트를 시작할 구간을 크기로 하는 ulnLob 에다
     *        Append 를 하는 것과 마찬가지이다.
     */
    sOriginalSize = aLob->mSize;
    aLob->mSize   = aStartOffset;

    /*
     * UPDATE BEGIN
     */
    ACI_TEST(ulnLobUpdateBegin(aFnContext,
                               aPtContext,
                               aLob,
                               aStartOffset,
                               aLobBuffer->mOp->mGetSize(aLobBuffer)) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedEndLob);

    /*
     * 데이터 전송
     */

    ACI_TEST(aLobBuffer->mOp->mDataOut(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    /*
     * UPDATE END
     */

    ULN_FLAG_UP(sNeedEndLob);
    ACI_TEST(ulnLobUpdateEnd(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    /*
     * 업데이트 후에 LOB 의 크기를 원복시켜야 한다.
     */

    ACI_TEST(ulnReadProtocol(aFnContext,
                             aPtContext,
                             sDbc->mConnTimeoutValue) != ACI_SUCCESS);

    /*
     * ulnLob::mSize 를 업데이트 한 후의 사이즈로 일치시킴
     */

    aLob->mSize = sOriginalSize - aLengthToUpdate + aLobBuffer->mOp->mGetSize(aLobBuffer);

    aLob->mBuffer = NULL;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedEndLob)
    {
        /*
         * Note : Flush 및 ReadProtocol 은 여기서 안해도 finalize protocol context 에서
         *        알아서 해 준다. 일단 에러가 났으므로 잊어버려도 된다.
         */
        ulnLobUpdateEnd(aFnContext, aPtContext, aLob);
    }

    aLob->mBuffer = NULL;

    return ACI_FAILURE;
}

/* 
 * PROJ-2047 Strengthening LOB - Added Interfaces
 */
static ACI_RC ulnLobTrim(ulnFnContext *aFnContext,
                         ulnPtContext *aPtContext,
                         ulnLob       *aLob,
                         acp_uint32_t  aStartOffset)
{
    acp_uint64_t sLobLocatorVal;
    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));

    ACI_TEST(ulnWriteLobTrimREQ(aFnContext,
                                aPtContext,
                                sLobLocatorVal,
                                aStartOffset) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * PROJ-2728 Sharding LOB
 */
static ACI_RC ulnLobPrepare4Write(ulnFnContext *aFnContext,
                                  ulnPtContext *aPtContext,
                                  ulnLob       *aLob,
                                  acp_uint32_t  aStartOffset,      /* 업데이트 시작점 (서버 lob) */
                                  acp_uint32_t  aNewSize)
{
    ulnDbc       *sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * UPDATE BEGIN
     */
    ACI_TEST(ulnLobUpdateBegin(aFnContext,
                               aPtContext,
                               aLob,
                               aStartOffset,
                               aNewSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    aLob->mBuffer = NULL;

    return ACI_FAILURE;
}

static ACI_RC ulnLobWrite(ulnFnContext *aFnContext,
                          ulnPtContext *aPtContext,
                          ulnLob       *aLob,
                          ulnLobBuffer *aLobBuffer) /* 업데이트 구간에 새로 채울 데이터 */
{
    ulnDbc       *sDbc;

    aLob->mBuffer = aLobBuffer;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * 데이터 전송
     */

    ACI_TEST(aLobBuffer->mOp->mDataOut(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    aLob->mBuffer = NULL;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    aLob->mBuffer = NULL;

    return ACI_FAILURE;
}

static ACI_RC ulnLobFinishWrite(ulnFnContext *aFnContext,
                                ulnPtContext *aPtContext,
                                ulnLob       *aLob)
{
    ulnDbc       *sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnLobUpdateEnd(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    /*
     * 업데이트 후에 LOB 의 크기를 원복시켜야 한다.
     */

    ACI_TEST(ulnReadProtocol(aFnContext,
                             aPtContext,
                             sDbc->mConnTimeoutValue) != ACI_SUCCESS);

    aLob->mBuffer = NULL;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    aLob->mBuffer = NULL;

    return ACI_FAILURE;
}

/*
 * -----------------------------
 * Meaningful callback functions
 * -----------------------------
 */

ACI_RC ulnCallbackLobGetSizeResult(cmiProtocolContext *aProtocolContext,
                                   cmiProtocol        *aProtocol,
                                   void               *aServiceSession,
                                   void               *aUserContext)
{
    ulnFnContext             *sFnContext  = (ulnFnContext *)aUserContext;

    acp_uint64_t        sLobSize64;
    acp_uint64_t        sLocatorID;
    acp_uint32_t        sSize;
    acp_bool_t          sIsNullLob;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD8(aProtocolContext, &sLocatorID);
    CMI_RD4(aProtocolContext, &sSize);
    CMI_RD1(aProtocolContext, sIsNullLob);

    /* PROJ-2728 Sharding LOB
     *   ULONG_MAX 값을 NULL로 사용한다 */
    if ( sIsNullLob == ACP_TRUE )
    {
        sLobSize64 = ACP_ULONG_MAX;
    }
    else
    {
        sLobSize64 = sSize;
    }

    /*
     * BUGBUG: lob locator ID 를 체크해야 .... 할까?
     */

    *(acp_uint64_t *)(sFnContext->mArgs) = sLobSize64;

    return ACI_SUCCESS;
}

ACI_RC ulnCallbackLobGetResult(cmiProtocolContext *aPtContext,
                               cmiProtocol        *aProtocol,
                               void               *aServiceSession,
                               void               *aUserContext)
{
    /*
     * Note : 서버에 아무리 큰 사이즈의 LOB 데이터를 요청하더라고 서버는 데이터를 잘라서
     *        보내준다. LOBGET RES 의 mOffset 은
     *        서버에 있는 전체 LOB 데이터에서 현재 전송된 패킷이 시작되는 절대 위치이다.
     *
     *        즉, 1Mib 데이터를 요구해서 서버는 이를테면, 32Kib 단위로 쪼개서 LOBGET RES 를
     *        여러번 보내준다.
     *
     *        데이터의 사이즈는 cmtVariable 의 size 를 읽으면 알 수 있다.
     */
    ulnFnContext  *sFnContext = (ulnFnContext *)aUserContext;
    ulnLob        *sLob       = (ulnLob *)sFnContext->mArgs;

    acp_uint64_t       sLocatorID;
    acp_uint32_t       sOffset;
    acp_uint32_t       sDataSize;


    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD8(aPtContext, &sLocatorID);
    CMI_RD4(aPtContext, &sOffset);
    CMI_RD4(aPtContext, &sDataSize);

    /*
     * 호출되는 함수 :
     *      ulnLobBufferDataInFILE()
     *      ulnLobBufferDataInBINARY()
     *      ulnLobBufferDataInCHAR()
     *      ulnLobBufferDataInWCHAR()
     *      ulnLobBufferDataInDumpAsCHAR()
     *      ulnLobBufferDataInDumpAsWCHAR()
     */

    ACI_TEST_RAISE(sLob->mBuffer->mOp->mDataIn(NULL,
                                sOffset,
                                sDataSize,
                                aPtContext->mReadBlock->mData +
                                aPtContext->mReadBlock->mCursor,
                                (void *)sFnContext ) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);

    aPtContext->mReadBlock->mCursor += sDataSize;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "CallbackLobGetResult");
    }

    ACI_EXCEPTION_END;

    aPtContext->mReadBlock->mCursor += sDataSize;

    /*
     * Note : 여기에서 ACI_SUCCESS 리턴하는 것은 버그가 아님. 이 함수가 콜백함수이기 때문임.
     */

    return ACI_SUCCESS;
}

/*
 * ------------------------------
 * Meaningless callback functions
 * ------------------------------
 */

ACI_RC ulnCallbackDBLobPutBeginResult(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      void               *aServiceSession,
                                      void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

ACI_RC ulnCallbackDBLobPutEndResult(cmiProtocolContext *aProtocolContext,
                                    cmiProtocol        *aProtocol,
                                    void               *aServiceSession,
                                    void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

ACI_RC ulnCallbackLobFreeResult(cmiProtocolContext *aProtocolContext,
                                cmiProtocol        *aProtocol,
                                void               *aServiceSession,
                                void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

ACI_RC ulnCallbackLobFreeAllResult(cmiProtocolContext *aProtocolContext,
                                   cmiProtocol        *aProtocol,
                                   void               *aServiceSession,
                                   void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

/* PROJ-2047 Strengthening LOB - Added Interfaces */
ACI_RC ulnCallbackLobTrimResult(cmiProtocolContext *aProtocolContext,
                                cmiProtocol        *aProtocol,
                                void               *aServiceSession,
                                void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}


/*
 * ========================================
 *
 * INITIALIZER functions
 *
 * PROJ-2047 Strengthening LOB - Added Interfaces 
 * 
 * BLOB과 CLOB용 Interface를 나눈다.
 *
 * ========================================
 */
static ulnLobBufferOp gUlnLobBufferOp[ULN_LOB_TYPE_MAX][ULN_LOB_BUFFER_TYPE_MAX] =
{
    /* NULL */
    {
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        }
    },

    /* BLOB */
    {
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },

        /*
         * ULN_LOB_BUFFER_TYPE_FILE
         */
        {
            ulnLobBufferInitializeFILE,
            ulnLobBufferPrepareFILE,
            ulnLobBufferDataInFILE,
            ulnLobBufferDataOutFILE,
            ulnLobBufferFinalizeFILE,

            ulnLobBufferGetSizeFILE
        },

        /*
         * ULN_LOB_BUFFER_TYPE_CHAR
         */
        {
            ulnLobBufferInitializeMEMORY,
            ulnLobBufferPrepareMEMORY,
            ulnLobBufferDataInDumpAsCHAR,
            ulnLobBufferDataOutBINARY,
            ulnLobBufferFinalizeMEMORY,

            ulnLobBufferGetSizeMEMORY
        },

        /*
         * ULN_LOB_BUFFER_TYPE_WCHAR
         */
        {
            ulnLobBufferInitializeMEMORY,
            ulnLobBufferPrepareMEMORY,
            ulnLobBufferDataInDumpAsWCHAR,
            ulnLobBufferDataOutBINARY,
            ulnLobBufferFinalizeMEMORY,

            ulnLobBufferGetSizeMEMORY
        },

        /*
         * ULN_LOB_BUFFER_TYPE_BINARY
         */
        {
            ulnLobBufferInitializeMEMORY,
            ulnLobBufferPrepareMEMORY,
            ulnLobBufferDataInBINARY,
            ulnLobBufferDataOutBINARY,
            ulnLobBufferFinalizeMEMORY,

            ulnLobBufferGetSizeMEMORY
        }
    },

    /* CLOB */
    {
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },

        /*
         * ULN_LOB_BUFFER_TYPE_FILE
         */
        {
            ulnLobBufferInitializeFILE,
            ulnLobBufferPrepareFILE,
            ulnLobBufferDataInFILE,
            ulnLobBufferDataOutFILE,
            ulnLobBufferFinalizeFILE,

            ulnLobBufferGetSizeFILE
        },

        /*
         * ULN_LOB_BUFFER_TYPE_CHAR
         */
        {
            ulnLobBufferInitializeMEMORY,
            ulnLobBufferPrepareMEMORY,
            ulnLobBufferDataInCHAR,
            ulnLobBufferDataOutCHAR,
            ulnLobBufferFinalizeMEMORY,

            ulnLobBufferGetSizeMEMORY
        },

        /*
         * ULN_LOB_BUFFER_TYPE_WCHAR
         */
        {
            ulnLobBufferInitializeMEMORY,
            ulnLobBufferPrepareMEMORY,
            ulnLobBufferDataInWCHAR,
            ulnLobBufferDataOutWCHAR,
            ulnLobBufferFinalizeMEMORY,

            ulnLobBufferGetSizeMEMORY
        },

        /*
         * ULN_LOB_BUFFER_TYPE_BINARY
         */
        {
            ulnLobBufferInitializeMEMORY,
            ulnLobBufferPrepareMEMORY,
            ulnLobBufferDataInBINARY,
            ulnLobBufferDataOutBINARY,
            ulnLobBufferFinalizeMEMORY,

            ulnLobBufferGetSizeMEMORY
        }
    }
};


static ulnLobOp gUlnLobOp =
{
    ulnLobGetState,

    ulnLobSetLocator,

    ulnLobOpen,
    ulnLobClose,

    ulnLobOverWrite,
    ulnLobAppend,
    ulnLobUpdate,

    ulnLobGetData,

    /* PROJ-2047 Strengthening LOB - Added Interfaces */
    ulnLobTrim,

    /* PROJ-2728 Sharding LOB */
    ulnLobPrepare4Write,
    ulnLobWrite,
    ulnLobFinishWrite,
};

/*
 * -------------------------
 * ulnLobBuffer : Initialize
 * -------------------------
 */
ACI_RC ulnLobBufferInitialize(ulnLobBuffer *aLobBuffer,
                              ulnDbc       *aDbc,
                              ulnLobType    aLobType,
                              ulnCTypeID    aCTYPE,
                              acp_uint8_t  *aFileNameOrBufferPtr,
                              acp_uint32_t  aFileOptionOrBufferSize)
{
    ulnLobBufferType sLobBufferType;

    switch (aLobType)
    {
        case ULN_LOB_TYPE_BLOB:
        case ULN_LOB_TYPE_CLOB:
            break;

        default:
            ACI_RAISE(LABEL_INVALID_LOB_TYPE);
            break;
    }

    switch (aCTYPE)
    {
        case ULN_CTYPE_FILE:
            sLobBufferType = ULN_LOB_BUFFER_TYPE_FILE;
            break;

        case ULN_CTYPE_CHAR:
            /*
             * PROJ-2047 Strengthening LOB - Partial Converting
             *
             * 서버와 클라이언트캐릭터셋이 동일한 경우에는 BINARY로 셋팅한다.
             */
            if (aDbc != NULL && aLobType == ULN_LOB_TYPE_CLOB)
            {
                if (aDbc->mCharsetLangModule == aDbc->mClientCharsetLangModule)
                {
                    sLobBufferType = ULN_LOB_BUFFER_TYPE_BINARY;
                }
                else
                {
                    sLobBufferType = ULN_LOB_BUFFER_TYPE_CHAR;
                }
            }
            else
            {
                sLobBufferType = ULN_LOB_BUFFER_TYPE_CHAR;
            }

            break;

        case ULN_CTYPE_WCHAR:
            sLobBufferType = ULN_LOB_BUFFER_TYPE_WCHAR;
            break;

        case ULN_CTYPE_BINARY:
            sLobBufferType = ULN_LOB_BUFFER_TYPE_BINARY;
            break;

        default:
            ACI_RAISE(LABEL_INVALID_LOB_BUFFER_TYPE);
            break;
    }

    aLobBuffer->mType = sLobBufferType;
    aLobBuffer->mOp   = &gUlnLobBufferOp[aLobType][sLobBufferType];
    aLobBuffer->mLob  = NULL;

    aLobBuffer->mOp->mInitialize(aLobBuffer, aFileNameOrBufferPtr, aFileOptionOrBufferSize);

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    ulnCharSetInitialize(&aLobBuffer->mCharSet);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LOB_BUFFER_TYPE);

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    ACI_EXCEPTION(LABEL_INVALID_LOB_TYPE);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * -------------------------
 * ulnLob : Initialize
 * -------------------------
 */

ACI_RC ulnLobInitialize(ulnLob *aLob, ulnMTypeID aMTYPE)
{
    switch (aMTYPE)
    {
        case ULN_MTYPE_BLOB:
        case ULN_MTYPE_BLOB_LOCATOR:
            aLob->mType = ULN_LOB_TYPE_BLOB;
            break;

        case ULN_MTYPE_CLOB:
        case ULN_MTYPE_CLOB_LOCATOR:
            aLob->mType = ULN_LOB_TYPE_CLOB;
            break;

        default:
            ACI_RAISE(LABEL_INVALID_LOB_TYPE);
            break;
    }

    aLob->mState          = ULN_LOB_ST_INITIALIZED;
    ULN_SET_LOB_LOCATOR(&(aLob->mLocatorID), ACP_SINT64_LITERAL(0));
    aLob->mSize           = 0;

    aLob->mSizeRetrieved  = 0;

    aLob->mBuffer         = NULL;
    aLob->mOp             = &gUlnLobOp;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    aLob->mData           = NULL;

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    aLob->mGetSize        = 0;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LOB_TYPE);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
