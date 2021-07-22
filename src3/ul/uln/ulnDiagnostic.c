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
#include <ulnDiagnostic.h>
#include <sdErrorCodeClient.h>

static ACI_RC ulnInitializeDiagHeader(ulnDiagHeader *aHeader)
{
    acpListInit(&aHeader->mDiagRecList);

    aHeader->mNumber              = 0;
    aHeader->mAllocedDiagRecCount = 0;
    aHeader->mCursorRowCount      = 0;
    aHeader->mReturnCode          = SQL_SUCCESS;
    aHeader->mRowCount            = 0;

#ifdef COMPILE_SHARDCLI
    /* TASK-7218 Multi-Error Handling 2nd */
    aHeader->mIsAllTheSame         = ACP_TRUE;
    aHeader->mMultiErrorMessageLen = 0;
#endif

    return ACI_SUCCESS;
}

static ACI_RC ulnDiagRecInitialize(ulnDiagHeader *aHeader, ulnDiagRec *aRec, acp_bool_t aIsStatic)
{
    /*
     * BUGBUG: 아래의 필드들의 값을 어떻게 정할까.
     */
    acpListInitObj(&aRec->mList, aRec);

    aRec->mServerName      = (acp_char_t *)"";
    aRec->mConnectionName  = (acp_char_t *)"";

    aRec->mRowNumber       = SQL_NO_ROW_NUMBER;
    aRec->mColumnNumber    = SQL_COLUMN_NUMBER_UNKNOWN;

    aRec->mNativeErrorCode = 0;

    aRec->mSubClassOrigin  = (acp_char_t *)"";
    aRec->mClassOrigin     = (acp_char_t *)"";

    aRec->mHeader          = aHeader;
    aRec->mIsStatic        = aIsStatic;

    if (aIsStatic == ACP_TRUE)
    {
        aRec->mMessageText = aHeader->mStaticMessage;
    }
    else
    {
        aRec->mMessageText = (acp_char_t *)"";
    }

    acpSnprintf(aRec->mSQLSTATE, 6, "00000");

    aRec->mNodeId          = 0; /* TASK-7218 */

    return ACI_SUCCESS;
}

ACI_RC ulnCreateDiagHeader(ulnObject *aParentObject, uluChunkPool *aSrcPool)
{
    ulnDiagHeader   *sDiagHeader;
    uluChunkPool    *sPool = NULL;

    uluMemory       *sMemory;

    if (aSrcPool == NULL)
    {
        sPool = uluChunkPoolCreate(ULN_SIZE_OF_CHUNK_IN_DIAGHEADER,
                                   ULN_NUMBER_OF_SP_IN_DIAGHEADER,
                                   1);

        ACI_TEST(sPool == NULL);
    }
    else
    {
        /*
         * 소스 풀이 지정되었을 경우에는 지정한 풀을 청크 풀로 사용한다.
         * 즉, 모든 메모리를 상위 객체에 있는 DiagHeader 의 청크 풀로부터
         * 할당받을 수 있게 된다.
         */
        sPool = aSrcPool;
    }

    /*
     * uluMemory 생성
     */

    ACI_TEST_RAISE(uluMemoryCreate(sPool, &sMemory) != ACI_SUCCESS,
                   LABEL_MEMORY_CREATE_FAIL);

    /*
     * Note : ulnDiagHeader 는 ulnObject 에 static 하게 들어있다.
     */

    sDiagHeader                = &aParentObject->mDiagHeader;
    sDiagHeader->mMemory       = sMemory;
    sDiagHeader->mPool         = sPool;
    sDiagHeader->mParentObject = aParentObject;

    ulnInitializeDiagHeader(sDiagHeader);
    ulnDiagRecInitialize(sDiagHeader, &sDiagHeader->mStaticDiagRec, ACP_TRUE);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEMORY_CREATE_FAIL)
    {
        if (aSrcPool == NULL)
        {
            sPool->mOp->mDestroyMyself(sPool);
        }
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnDestroyDiagHeader(ulnDiagHeader *aDiagHeader, acp_bool_t aDestroyChunkPool)
{
    ACI_TEST(aDiagHeader->mMemory == NULL);
    ACI_TEST(aDiagHeader->mPool == NULL);

    /*
     * 청크풀을 파괴할 필요가 있으면 파괴한다.
     */
    if (aDestroyChunkPool == ACP_TRUE)
    {
        /*
         * 혹여라도 자식들이 있으면 없애지 않고 HY013을 낸다.
         */
        ACI_TEST(aDiagHeader->mPool->mOp->mGetRefCnt(aDiagHeader->mPool) == 0);

        /*
         * 메모리 인스턴스 파괴 :
         * HY013 을 내면 그걸 매달 Diagnostic 구조체가 있어야 한다.
         * 무조건 메모리 인스턴스부터 없애보 볼 일이 아니므로 이 위치로 옮김.
         */
        aDiagHeader->mMemory->mOp->mDestroyMyself(aDiagHeader->mMemory);

        /*
         * 청크 풀 파괴
         */
        aDiagHeader->mPool->mOp->mDestroyMyself(aDiagHeader->mPool);
    }
    else
    {
        /*
         * 메모리 인스턴스만 파괴
         */
        aDiagHeader->mMemory->mOp->mDestroyMyself(aDiagHeader->mMemory);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulnDiagRecCreate(ulnDiagHeader *aHeader, ulnDiagRec **aDiagRec)
{
    uluMemory  *sMemory;
    ulnDiagRec *sDiagRec;

    sMemory = aHeader->mMemory;

    /*
     * 레코드를 위한 메모리를 헤더의 uluMemory를 이용해 할당
     */
    if (sMemory->mOp->mMalloc(sMemory, (void **)&sDiagRec, ACI_SIZEOF(ulnDiagRec)) == ACI_SUCCESS)
    {
        // BUG-21791 diag record를 할당할 때 증가시킨다.
        aHeader->mAllocedDiagRecCount++;
    }
    else
    {
        sDiagRec = &(aHeader->mStaticDiagRec);
    }

    /*
     * Diagnostic Record 를 초기화한다.
     */
    /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
    if ( sDiagRec != NULL )
    {
        ulnDiagRecInitialize(aHeader, sDiagRec, ACP_FALSE);
    }

    *aDiagRec = sDiagRec;

    return;
}

ACI_RC ulnDiagRecDestroy(ulnDiagRec *aRec)
{
    ACP_UNUSED(aRec);

    return ACI_SUCCESS;
}

void ulnDiagHeaderAddDiagRec(ulnDiagHeader *aDiagHeader, ulnDiagRec *aDiagRec)
{
    /*
     * BUGBUG : rank 에 맞게 sort를 하고 인덱스를 집어넣어 주어야 한다.
     */

    if (aDiagRec->mNativeErrorCode == ulERR_FATAL_MEMORY_ALLOC_ERROR ||
        aDiagRec->mNativeErrorCode == ulERR_FATAL_MEMORY_MANAGEMENT_ERROR ||
        aDiagRec->mNativeErrorCode == sdERR_ABORT_SHARD_MULTIPLE_ERRORS )
    {
        /*
         * 메모리 부족 에러는 맨 앞에 놓인다.
         * Multi-Error는 맨 앞에 놓인다.
         */
        acpListPrependNode(&(aDiagHeader->mDiagRecList), (acp_list_node_t *)aDiagRec);
    }
    else
    {
        acpListAppendNode(&(aDiagHeader->mDiagRecList), (acp_list_node_t *)aDiagRec);
    }

    aDiagHeader->mNumber++;
}

void ulnDiagHeaderRemoveDiagRec(ulnDiagHeader *aDiagHeader, ulnDiagRec *aDiagRec)
{
    if (aDiagHeader->mNumber > 0)
    {
        acpListDeleteNode((acp_list_node_t *)aDiagRec);
        aDiagHeader->mNumber--;
    }
}

ACI_RC ulnClearDiagnosticInfoFromObject(ulnObject *aObject)
{
    //fix BUG-18001
    if(aObject->mDiagHeader.mAllocedDiagRecCount > 0)
    {
        // BUG-21791
        aObject->mDiagHeader.mAllocedDiagRecCount = 0;

        ACI_TEST(aObject->mDiagHeader.mMemory->mOp->mFreeToSP(aObject->mDiagHeader.mMemory, 0)
                 != ACI_SUCCESS);

    }

    ulnInitializeDiagHeader(&(aObject->mDiagHeader));

    aObject->mSqlErrorRecordNumber = 1;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnGetDiagRecFromObject(ulnObject *aObject, ulnDiagRec **aDiagRec, acp_sint32_t aRecNumber)
{
    acp_sint32_t  i;
    acp_list_t   *sIterator;
    ulnDiagRec   *sDiagRec;

    ACE_ASSERT(aDiagRec != NULL);

    ACI_TEST(aRecNumber > aObject->mDiagHeader.mNumber);

    sDiagRec = NULL;

    /*
     * Note: DiagRec 는 1번부터 시작된다.
     */
    i = 1;

    ACP_LIST_ITERATE(&(aObject->mDiagHeader.mDiagRecList), sIterator)
    {
        if (i == aRecNumber)
        {
            sDiagRec = (ulnDiagRec *)sIterator;
            break;
        }

        i++;
    }

    /*
     * 발견 안되었다는 것은 심각한 에러가 있다는 이야기인데...
     * BUGBUG: 죽어야 할까?
     */
    ACI_TEST(sDiagRec == NULL);

    *aDiagRec = sDiagRec;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    *aDiagRec = NULL;

    return ACI_FAILURE;
}

ulnDiagHeader *ulnGetDiagHeaderFromObject(ulnObject *aObject)
{
    return &aObject->mDiagHeader;
}

/*
 * Diagnostic Header 의 필드들을 읽어오거나 설정하는 함수들.
 */

ACI_RC ulnDiagGetReturnCode(ulnDiagHeader *aDiagHeader, SQLRETURN *aReturnCode)
{
    *aReturnCode = aDiagHeader->mReturnCode;

    return ACI_SUCCESS;
}

ACI_RC ulnDiagGetCursorRowCount(ulnDiagHeader *aDiagHeader, acp_sint32_t *aCursorRowcount)
{
    if (aCursorRowcount != NULL)
    {
        *aCursorRowcount = aDiagHeader->mCursorRowCount;
    }

    return ACI_SUCCESS;
}

ACI_RC ulnDiagGetDynamicFunctionCode(ulnDiagHeader *aDiagHeader, acp_sint32_t *aFunctionCode)
{
    ACP_UNUSED(aDiagHeader);

    if (aFunctionCode != NULL)
    {
        /*
         * BUGBUG
         */

        *aFunctionCode = SQL_DIAG_UNKNOWN_STATEMENT;
    }

    return ACI_SUCCESS;
}

ACI_RC ulnDiagGetDynamicFunction(ulnDiagHeader *aDiagHeader,const acp_char_t **aFunctionName)
{
    ACP_UNUSED(aDiagHeader);

    if (aFunctionName != NULL)
    {
        /*
         * BUGBUG
         */

        *aFunctionName = "";
    }

    return ACI_SUCCESS;
}

ACI_RC ulnDiagGetNumber(ulnDiagHeader *aDiagHeader, acp_sint32_t *aNumberOfStatusRecords)
{
    if (aNumberOfStatusRecords != NULL)
    {
        *aNumberOfStatusRecords = aDiagHeader->mNumber;
    }

    return ACI_SUCCESS;
}

/*
 * Diagnostic Record 의 필드들의 값을 읽어오는 함수들
 */

void ulnDiagRecSetMessageText(ulnDiagRec *aDiagRec, acp_char_t *aMessageText)
{
    acp_uint32_t  sSizeMessageText;
    uluMemory    *sMemory;

    if (aDiagRec->mIsStatic == ACP_TRUE)
    {
        acpSnprintf(aDiagRec->mMessageText,
                        ULN_DIAG_CONTINGENCY_BUFFER_SIZE,
                        "%s",
                        aMessageText);
    }
    else
    {
        sMemory = aDiagRec->mHeader->mMemory;

        // fix BUG-30387
        // 서버의 에러 메세지 최대 길이와 상관없이 에러 메세지 설정
        sSizeMessageText = acpCStrLen(aMessageText, ACP_SINT32_MAX) + 1;

        if (sMemory->mOp->mMalloc(sMemory,
                                  (void **)(&aDiagRec->mMessageText),
                                  sSizeMessageText) != ACI_SUCCESS)
        {
            aDiagRec->mMessageText = (acp_char_t *)"";
        }
        else
        {
            acpSnprintf(aDiagRec->mMessageText, sSizeMessageText, "%s", aMessageText);
        }
    }

    return;
}

ACI_RC ulnDiagRecGetMessageText(ulnDiagRec   *aDiagRec,
                                acp_char_t   *aMessageText,
                                acp_sint16_t  aBufferSize,
                                acp_sint16_t *aActualLength)
{
    acp_sint16_t sActualLength;

    if (aActualLength != NULL)
    {
        //BUG-28623 [CodeSonar]Ignored Return Value
        sActualLength = acpCStrLen(aDiagRec->mMessageText, ACP_SINT32_MAX);
        *aActualLength = sActualLength;
    }

    if ((aMessageText != NULL) && (aBufferSize > 0))
    {
        acpSnprintf(aMessageText, aBufferSize, "%s", aDiagRec->mMessageText);
    }

    return ACI_SUCCESS;
}

void ulnDiagRecSetSqlState(ulnDiagRec *aDiagRec, acp_char_t *aSqlState)
{
    /* BUG-35242 ALTI-PCM-002 Coding Convention Violation in UL module */
    acpMemCpy( aDiagRec->mSQLSTATE, aSqlState, 5 );
    aDiagRec->mSQLSTATE[5] = '\0';
}

void ulnDiagRecGetSqlState(ulnDiagRec *aDiagRec, acp_char_t **aSqlState)
{
    if (aSqlState != NULL)
    {
        *aSqlState = aDiagRec->mSQLSTATE;
    }
}

void ulnDiagRecSetNativeErrorCode(ulnDiagRec *aDiagRec, acp_uint32_t aNativeErrorCode)
{
    aDiagRec->mNativeErrorCode = aNativeErrorCode;
}

acp_uint32_t ulnDiagRecGetNativeErrorCode(ulnDiagRec *aDiagRec)
{
    return aDiagRec->mNativeErrorCode;
}

ACI_RC ulnDiagGetClassOrigin(ulnDiagRec *aDiagRec, acp_char_t **aClassOrigin)
{
    ACE_ASSERT(aDiagRec != NULL);
    // ACE_ASSERT(*aClassOrigin != NULL);
    ACE_ASSERT(aClassOrigin != NULL);

    *aClassOrigin = aDiagRec->mClassOrigin;

    return ACI_SUCCESS;
}

ACI_RC ulnDiagGetConnectionName(ulnDiagRec *aDiagRec, acp_char_t **aConnectionName)
{
    ACE_ASSERT(aDiagRec != NULL);
    // ACE_ASSERT(*aConnectionName != NULL);
    ACE_ASSERT(aConnectionName != NULL);

    *aConnectionName = aDiagRec->mConnectionName;

    return ACI_SUCCESS;
}

ACI_RC ulnDiagGetNative(ulnDiagRec *aDiagRec, acp_sint32_t *aNative)
{
    ACE_ASSERT(aDiagRec != NULL);
    ACE_ASSERT(aNative != NULL);

    *aNative = aDiagRec->mNativeErrorCode;

    return ACI_SUCCESS;
}

acp_sint32_t ulnDiagRecGetRowNumber(ulnDiagRec *aDiagRec)
{
    return aDiagRec->mRowNumber;
}

void ulnDiagRecSetRowNumber(ulnDiagRec *aDiagRec, acp_sint32_t aRowNumber)
{
    aDiagRec->mRowNumber = aRowNumber;
}

acp_sint32_t ulnDiagRecGetColumnNumber(ulnDiagRec *aDiagRec)
{
    return aDiagRec->mColumnNumber;
}

void ulnDiagRecSetColumnNumber(ulnDiagRec *aDiagRec, acp_sint32_t aColumnNumber)
{
    aDiagRec->mColumnNumber = aColumnNumber;
}

ACI_RC ulnDiagGetServerName(ulnDiagRec *aDiagRec, acp_char_t **aServerName)
{
    ACE_ASSERT(aDiagRec != NULL);
    // ACE_ASSERT(*aServerName != NULL);
    ACE_ASSERT(aServerName != NULL);

    *aServerName = aDiagRec->mServerName;

    return ACI_SUCCESS;
}

ACI_RC ulnDiagGetSubClassOrigin(ulnDiagRec *aDiagRec, acp_char_t **aSubClassOrigin)
{
    ACE_ASSERT(aDiagRec != NULL);
    // ACE_ASSERT(*aSubClassOrigin != NULL);
    ACE_ASSERT(aSubClassOrigin != NULL);

    *aSubClassOrigin = aDiagRec->mSubClassOrigin;

    return ACI_SUCCESS;
}

/**
 * ulnDiagGetDiagIdentifierClass.
 *
 * Diagnostic Identifier 가 header 용인지 record 용인지를 판별한다.
 *
 * @param[in] aDiagIdentifier
 * @param[out] aClass
 *  - ULN_DIAG_IDENTIFIER_CLASS_UNKNOWN : 알 수 없는 Identifier
 *  - ULN_DIAG_IDENTIFIER_CLASS_HEADER : 헤더필드
 *      - SQL_DIAG_CURSOR_ROW_COUNT
 *      - SQL_DIAG_DYNAMIC_FUNCTION
 *      - SQL_DIAG_DYNAMIC_FUNCTION_CODE
 *      - SQL_DIAG_NUMBER
 *      - SQL_DIAG_RETURNCODE
 *      - SQL_DIAG_ROW_COUNT
 *  - ULN_DIAG_IDENTIFIER_CLASS_RECORD : 레코드필드
 *      - SQL_DIAG_CLASS_ORIGIN
 *      - SQL_DIAG_COLUMN_NUMBER
 *      - SQL_DIAG_CONNECTION_NAME
 *      - SQL_DIAG_MESSAGE_TEXT
 *      - SQL_DIAG_NATIVE
 *      - SQL_DIAG_ROW_NUMBER
 *      - SQL_DIAG_SERVER_NAME
 *      - SQL_DIAG_SQLSTATE
 *      - SQL_DIAG_SUBCLASS_ORIGIN
 * @return
 *  - ACI_SUCCESS
 *  - ACI_FAILURE
 *    정의되지 않은 id 가 들어왔을 때.
 *    이때는 aClass 가 가리키는 포인터에 ULN_DIAG_IDENTIFIER_CLASS_UNKNOWN 이 들어간다.
 */
ACI_RC ulnDiagGetDiagIdentifierClass(acp_sint16_t aDiagIdentifier, ulnDiagIdentifierClass *aClass)
{
    switch(aDiagIdentifier)
    {
        case SQL_DIAG_CURSOR_ROW_COUNT:
        case SQL_DIAG_DYNAMIC_FUNCTION:
        case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
        case SQL_DIAG_NUMBER:
        case SQL_DIAG_RETURNCODE:
        case SQL_DIAG_ROW_COUNT:
            *aClass = ULN_DIAG_IDENTIFIER_CLASS_HEADER;
            break;

        case SQL_DIAG_CLASS_ORIGIN:
        case SQL_DIAG_COLUMN_NUMBER:
        case SQL_DIAG_CONNECTION_NAME:
        case SQL_DIAG_MESSAGE_TEXT:
        case SQL_DIAG_NATIVE:
        case SQL_DIAG_ROW_NUMBER:
        case SQL_DIAG_SERVER_NAME:
        case SQL_DIAG_SQLSTATE:
        case SQL_DIAG_SUBCLASS_ORIGIN:
            *aClass = ULN_DIAG_IDENTIFIER_CLASS_RECORD;
            break;

        default:
            *aClass = ULN_DIAG_IDENTIFIER_CLASS_UNKNOWN;
            ACI_TEST(1);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-1381 Fetch Across Commit */

/**
 * DiagRec을 옮긴다.
 *
 * @param[in] aObjectTo   옮길 DiagRec을 넣을     Object Handle
 * @param[in] aObjectFrom 옮길 DiagRec을 갖고있는 Object Handle
 */
void ulnDiagRecMoveAll(ulnObject *aObjectTo, ulnObject *aObjectFrom)
{
    ulnDiagRec *sDiagRecFrom;
    ulnDiagRec *sDiagRecTo;

    while (ulnGetDiagRecFromObject(aObjectFrom, &sDiagRecFrom, 1) == ACI_SUCCESS)
    {
        ulnDiagRecCreate(&(aObjectTo->mDiagHeader), &sDiagRecTo);
        ulnDiagRecSetMessageText(sDiagRecTo, sDiagRecFrom->mMessageText);
        ulnDiagRecSetSqlState(sDiagRecTo, sDiagRecFrom->mSQLSTATE);
        ulnDiagRecSetNativeErrorCode(sDiagRecTo, sDiagRecFrom->mNativeErrorCode);
        ulnDiagRecSetRowNumber(sDiagRecTo, sDiagRecFrom->mRowNumber);
        ulnDiagRecSetColumnNumber(sDiagRecTo, sDiagRecFrom->mColumnNumber);
        ulnDiagRecSetNodeId(sDiagRecTo, sDiagRecFrom->mNodeId);

        ulnDiagHeaderAddDiagRec(sDiagRecTo->mHeader, sDiagRecTo);
        ulnDiagHeaderRemoveDiagRec(sDiagRecFrom->mHeader, sDiagRecFrom);
        ulnDiagRecDestroy(sDiagRecFrom);
    }
}

/**
 * DiagRec을 모두 지운다.
 *
 * @param[in] aObject DiagRec을 지울 Object Handle
 */
void ulnDiagRecRemoveAll(ulnObject *aObject)
{
    ulnDiagRec *sDiagRec;

    while (ulnGetDiagRecFromObject(aObject, &sDiagRec, 1) == ACI_SUCCESS)
    {
        ulnDiagHeaderRemoveDiagRec(sDiagRec->mHeader, sDiagRec);
        ulnDiagRecDestroy(sDiagRec);
    }
}

/* BUG-48216 */
void ulnDiagRecSoftMoveAll(ulnObject *aObjectTo, ulnObject *aObjectFrom)
{
    ulnDiagRec *sDiagRec = NULL;

    while (ulnGetDiagRecFromObject(aObjectFrom, &sDiagRec, 1) == ACI_SUCCESS)
    {
        ulnDiagHeaderRemoveDiagRec(&aObjectFrom->mDiagHeader, sDiagRec);
        sDiagRec->mHeader = &aObjectTo->mDiagHeader;
        ulnDiagHeaderAddDiagRec(&aObjectTo->mDiagHeader, sDiagRec);
    }
}

/* TASK-7218 */
void ulnDiagRecSetNodeId(ulnDiagRec *aDiagRec, acp_uint32_t aNodeId)
{
    aDiagRec->mNodeId = aNodeId;
}

acp_uint32_t ulnDiagRecGetNodeId(ulnDiagRec *aDiagRec)
{
    return aDiagRec->mNodeId;
}

