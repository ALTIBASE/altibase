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
#include <ulnDescRec.h>
#include <ulnPDContext.h>

ACI_RC ulnDescRecInitialize(ulnDesc *aParentDesc, ulnDescRec *aRecord, acp_uint16_t aIndex)
{
    aRecord->mIndex         = aIndex;
    aRecord->mParentDesc    = aParentDesc;

    acpListInit(&aRecord->mList);

    /*
     * Meta 정보 초기화
     */
    ulnMetaInitialize(&aRecord->mMeta);

    ulnPDContextCreate(&aRecord->mPDContext);

    /*
     * ODBC 표준 속성들
     */
    aRecord->mInOutType          = ULN_PARAM_INOUT_TYPE_MAX;
    aRecord->mDisplaySize        = 0;

    /*
     * BUGBUG : M$ ODBC 에는 정의되어 있지만, 현재 uln 에서는 사용되고 있지 않은 속성.
     *          어떻게 해야 할런지도 잘 모르겠다.
     */
    aRecord->mAutoUniqueValue   = 0;
    aRecord->mCaseSensitive     = 0;

    aRecord->mDataPtr           = NULL;
    aRecord->mIndicatorPtr      = NULL;
    aRecord->mOctetLengthPtr    = NULL;
    aRecord->mFileOptionsPtr    = NULL;

    aRecord->mDisplayName[0]    = '\0';
    aRecord->mColumnName[0]     = '\0';
    aRecord->mBaseColumnName[0] = '\0';
    aRecord->mTableName[0]      = '\0';
    aRecord->mBaseTableName[0]  = '\0';
    aRecord->mSchemaName[0]     = '\0';
    aRecord->mCatalogName[0]    = '\0';

    aRecord->mTypeName          = (acp_char_t *)"";
    aRecord->mLocalTypeName     = (acp_char_t *)"";
    aRecord->mLiteralPrefix     = (acp_char_t *)"";
    aRecord->mLiteralSuffix     = (acp_char_t *)"";

    aRecord->mRowVer            = 0;
    aRecord->mSearchable        = 0;
    aRecord->mUnnamed           = 0;
    aRecord->mUnsigned          = SQL_FALSE;    // 알티베이스는 모든 숫자가 signed 이다.
    aRecord->mUpdatable         = SQL_ATTR_READONLY;

    /* PROJ-2616 */
    aRecord->mMaxByteSize       = 0;

    /* PROJ-2739 */
    aRecord->mShardLocator      = NULL;
    
    return ACI_SUCCESS;
}

void ulnDescRecInitOutParamBuffer(ulnDescRec *aDescRec)
{
    aDescRec->mOutParamBufferSize = 0;
    aDescRec->mOutParamBuffer     = NULL;
}

void ulnDescRecFinalizeOutParamBuffer(ulnDescRec *aDescRec)
{
    aDescRec->mOutParamBufferSize = 0;

    if (aDescRec->mOutParamBuffer != NULL)
    {
        acpMemFree(aDescRec->mOutParamBuffer);
    }

    aDescRec->mOutParamBuffer = NULL;
}

ACI_RC ulnDescRecPrepareOutParamBuffer(ulnDescRec *aDescRec, acp_uint32_t aNewSize, ulnMTypeID aMTYPE)
{
    uluMemory *sMemory;
    ulnColumn *sOutParamBuffer = NULL;

    // PROJ-1752 LIST PROTOCOL
    if( aMTYPE == ULN_MTYPE_BLOB_LOCATOR ||
        aMTYPE == ULN_MTYPE_CLOB_LOCATOR )
    {
        aNewSize = ACI_SIZEOF(ulnLob);
    }

    if (aDescRec->mOutParamBufferSize < aNewSize)
    {
        /*
         * BUGBUG : 새로 메모리를 할당할 필요가 있을 경우에는 기존에 할당한 것을 그냥
         *          무시하고 새로 할당한다.
         *          이로 인해 ulnDesc 에 좀비로 남아있는 메모리는 큰 위협이 되지 않으리라
         *          판단하고 일단 무시한다.
         *
         *          차후에 ul 의 메모리 구조를 개편할 때 함께 해결하도록 한다.
         */

        sMemory = aDescRec->mParentDesc->mObj.mMemory;

        ACI_TEST(sMemory->mOp->mMalloc(sMemory,
                                       (void **)&sOutParamBuffer,
                                       ACI_SIZEOF(ulnColumn) + aNewSize) != ACI_SUCCESS);

        aDescRec->mOutParamBuffer     = sOutParamBuffer;
        aDescRec->mOutParamBufferSize = aNewSize;
    }

    /*
     * BUGBUG : aDescRec->mOutParamBuffer 가 null 인지 체크를 해야 하나 말아야 하나...
     */
    ((ulnColumn *)(aDescRec->mOutParamBuffer))->mColumnNumber  = aDescRec->mIndex;
    ((ulnColumn *)(aDescRec->mOutParamBuffer))->mMtype         = aMTYPE;
    ((ulnColumn *)(aDescRec->mOutParamBuffer))->mGDPosition    = 0;
    ((ulnColumn *)(aDescRec->mOutParamBuffer))->mRemainTextLen = 0;
    ((ulnColumn *)(aDescRec->mOutParamBuffer))->mBuffer        = (acp_uint8_t*)(aDescRec->mOutParamBuffer + 1);
    ((ulnColumn *)(aDescRec->mOutParamBuffer))->mBuffer[aDescRec->mOutParamBufferSize] = 0;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulnDescRecInitLobArray(ulnDescRec *aDescRec)
{
    aDescRec->mLobArray = NULL;
}

ACI_RC ulnDescRecCreate(ulnDesc *aParentDesc, ulnDescRec **aDescRec, acp_uint16_t aIndex)
{
    uluMemory   *sParentMemory = aParentDesc->mObj.mMemory;
    ulnDescType  sDescType     = ULN_OBJ_GET_DESC_TYPE(aParentDesc);
    ulnDescRec  *sDescRec      = NULL;

    /* PROJ-1789 Updatable Scrollable Cursor
     * BOOKMARK 바인딩을 할 수 있도록 조건 변경.
     * unsigned이므로 >= 0 조건이 항상 성공이다. 그러므로 무시해도 OK */
    ACP_UNUSED(aIndex);

    /*
     * Desc랍시고 넘겨진 녀석이 정말로 Descriptor이며 유효한 종류의 Descriptor인지 확인
     */

    ACE_ASSERT(ULN_OBJ_GET_TYPE(aParentDesc) == ULN_OBJ_TYPE_DESC);

    ACE_ASSERT(ULN_DESC_TYPE_NODESC < sDescType &&
                                      sDescType < ULN_DESC_TYPE_MAX);

    *aDescRec = NULL;

    /* BUG-44858 mFreeDescRecList를 먼저 확인하자. */
    if (acpListIsEmpty(&aParentDesc->mFreeDescRecList) != ACP_TRUE)
    {
        sDescRec = (ulnDescRec *)acpListGetFirstNode(&aParentDesc->mFreeDescRecList);
        acpListDeleteNode((acp_list_node_t *)sDescRec);
    }
    else
    {
        ACI_TEST(sParentMemory->mOp->mMalloc(sParentMemory,
                                             (void **)&sDescRec,
                                             ACI_SIZEOF(ulnDescRec)) != ACI_SUCCESS);
    }

    *aDescRec = sDescRec;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ulnDescRec::mLobArray 관련 함수들
 */

ulnLob *ulnDescRecGetLobElement(ulnDescRec *aDescRec, acp_uint32_t aRowNumber)
{
    ulnLob *aReturnLob;

    /*
     * Note : ULU_ARRAY_IGNORE 이면 무조건 ACI_SUCCESS 만 리턴한다.
     */
    uluArrayGetElement(aDescRec->mLobArray, aRowNumber, (void **)&aReturnLob, ULU_ARRAY_IGNORE);

    return aReturnLob;
}

ACI_RC ulnDescRecArrangeLobArray(ulnDescRec *aDescRec, acp_uint32_t aArrayCount)
{
    uluMemory *sMemory;

    if (aDescRec->mLobArray == NULL)
    {
        sMemory = aDescRec->mParentDesc->mObj.mMemory;

        ACI_TEST(uluArrayCreate(sMemory,
                                &aDescRec->mLobArray,
                                ACI_SIZEOF(ulnLob),
                                20,
                                NULL) != ACI_SUCCESS);
    }

    /*
     * Array 에 aArrayCount 개의 element 가 들어갈 공간 확보.
     */
    ACI_TEST(uluArrayReserve(aDescRec->mLobArray, aArrayCount) != ACI_SUCCESS);

    /*
     * 모든 내용을 0 으로 초기화
     */
    uluArrayInitializeContent(aDescRec->mLobArray);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * 컬럼이 어떤 종류인지 판단하는 함수들.
 *      1. Memory bound LOB column
 *      2. DATA AT EXEC column
 */

acp_bool_t ulnDescRecIsDataAtExecParam(ulnDescRec *aAppDescRec, acp_uint32_t aRow)
{
    ulvSLen *sUserOctetLengthPtr = NULL;

    sUserOctetLengthPtr = ulnBindCalcUserOctetLengthAddr(aAppDescRec, aRow);

    if (sUserOctetLengthPtr != NULL)
    {
        if (*sUserOctetLengthPtr <= SQL_LEN_DATA_AT_EXEC_OFFSET ||
            *sUserOctetLengthPtr == SQL_DATA_AT_EXEC)
        {
            return ACP_TRUE;
        }
    }

    return ACP_FALSE;
}

/* PROJ-1789 Updatable Scrollable Cursor */

/**
 * DisplayName을 설정한다.
 *
 * @param[in] aRecord         IRD Record
 * @param[in] aDisplayName    DisplayName
 * @param[in] aDisplayNameLen DisplayName 길이
 */
void ulnDescRecSetDisplayName(ulnDescRec *aRecord, acp_char_t *aDisplayName, acp_size_t aDisplayNameLen)
{
    acp_size_t sLen = ACP_MIN(aDisplayNameLen, ACI_SIZEOF(aRecord->mDisplayName) - 1);
    acpMemCpy(aRecord->mDisplayName, aDisplayName, sLen);
    aRecord->mDisplayName[sLen] = '\0';
}

/**
 * ColumnName을 설정한다.
 *
 * @param[in] aRecord        IRD Record
 * @param[in] aColumnName    ColumnName
 * @param[in] aColumnNameLen ColumnName 길이
 */
void ulnDescRecSetColumnName(ulnDescRec *aRecord, acp_char_t *aColumnName, acp_size_t aColumnNameLen)
{
    acp_size_t sLen = ACP_MIN(aColumnNameLen, ACI_SIZEOF(aRecord->mColumnName) - 1);
    acpMemCpy(aRecord->mColumnName, aColumnName, sLen);
    aRecord->mColumnName[sLen] = '\0';

    /* SQL_DESC_UNNAMED */
    if (sLen == 0)
    {
        ulnDescRecSetUnnamed(aRecord, SQL_UNNAMED);
    }
    else
    {
        ulnDescRecSetUnnamed(aRecord, SQL_NAMED);
    }
}

/**
 * BaseColumnName을 설정한다.
 *
 * @param[in] aRecord            IRD Record
 * @param[in] aBaseColumnName    BaseColumnName
 * @param[in] aBaseColumnNameLen BaseColumnName 길이
 */
void ulnDescRecSetBaseColumnName(ulnDescRec *aRecord, acp_char_t *aBaseColumnName, acp_size_t aBaseColumnNameLen)
{
    acp_size_t sLen = ACP_MIN(aBaseColumnNameLen, ACI_SIZEOF(aRecord->mBaseColumnName) - 1);
    acpMemCpy(aRecord->mBaseColumnName, aBaseColumnName, sLen);
    aRecord->mBaseColumnName[sLen] = '\0';
}

/**
 * TableName을 설정한다.
 *
 * @param[in] aRecord       IRD Record
 * @param[in] aTableName    TableName
 * @param[in] aTableNameLen TableName 길이
 */
void ulnDescRecSetTableName(ulnDescRec *aRecord, acp_char_t *aTableName, acp_size_t aTableNameLen)
{
    acp_size_t sLen = ACP_MIN(aTableNameLen, ACI_SIZEOF(aRecord->mTableName) - 1);
    acpMemCpy(aRecord->mTableName, aTableName, sLen);
    aRecord->mTableName[sLen] = '\0';
}

/**
 * BaseTableName을 설정한다.
 *
 * @param[in] aRecord           IRD Record
 * @param[in] aBaseTableName    BaseTableName
 * @param[in] aBaseTableNameLen BaseTableName 길이
 */
void ulnDescRecSetBaseTableName(ulnDescRec *aRecord, acp_char_t *aBaseTableName, acp_size_t aBaseTableNameLen)
{
    acp_size_t sLen = ACP_MIN(aBaseTableNameLen, ACI_SIZEOF(aRecord->mBaseTableName) - 1);
    acpMemCpy(aRecord->mBaseTableName, aBaseTableName, sLen);
    aRecord->mBaseTableName[sLen] = '\0';
}

/**
 * SchemaName(User Name)을 설정한다.
 *
 * @param[in] aRecord        IRD Record
 * @param[in] aSchemaName    SchemaName
 * @param[in] aSchemaNameLen SchemaName 길이
 */
void ulnDescRecSetSchemaName(ulnDescRec *aRecord, acp_char_t *aSchemaName, acp_size_t aSchemaNameLen)
{
    acp_size_t sLen = ACP_MIN(aSchemaNameLen, ACI_SIZEOF(aRecord->mSchemaName) - 1);
    acpMemCpy(aRecord->mSchemaName, aSchemaName, sLen);
    aRecord->mSchemaName[sLen] = '\0';
}

/**
 * CatalogName(DB Name)을 설정한다.
 *
 * @param[in] aRecord         IRD Record
 * @param[in] aCatalogName    CatalogName
 * @param[in] aCatalogNameLen CatalogName 길이
 */
void ulnDescRecSetCatalogName(ulnDescRec *aRecord, acp_char_t *aCatalogName, acp_size_t aCatalogNameLen)
{
    acp_size_t sLen = ACP_MIN(aCatalogNameLen, ACI_SIZEOF(aRecord->mCatalogName) - 1);
    acpMemCpy(aRecord->mCatalogName, aCatalogName, sLen);
    aRecord->mCatalogName[sLen] = '\0';
}
