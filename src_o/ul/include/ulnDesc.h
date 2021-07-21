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

#ifndef _O_ULN_DESC_H_
#define _O_ULN_DESC_H_ 1

#include <ulnConfig.h>
#include <ulnPDContext.h>

typedef enum ulnDescAllocType
{
    ULN_DESC_ALLOCTYPE_IMPLICIT = SQL_DESC_ALLOC_AUTO,
    ULN_DESC_ALLOCTYPE_EXPLICIT = SQL_DESC_ALLOC_USER
} ulnDescAllocType;

/*
 * ulnStmtAssociatedWithDesc
 *
 * SQLFreeHandle() 함수로 디스크립터를 해제할 때
 * 그 디스크립터를 APD 나 ARD 로 사용하던 statement 의 APD 혹은 ARD 를
 * 원복해 줘야 한다.
 * 그런데, 디스크립터는 여러개의 statement 의 APD, ARD 로 사용될 수 있다.
 */
struct ulnStmtAssociatedWithDesc
{
    acp_list_node_t  mList;
    ulnStmt         *mStmt;
};

/*
 * ulnDescHeader
 *  - 디스크립터 자체에 대한 정보를 가진다.
 *  - ODBC 스펙에는 디스크립터 헤더라고 표현된다.
 */
typedef struct ulnDescHeader
{
    /*
     * BUGBUG:
     * 이렇게 분리해 봐야 별 볼일 없는데, 헷갈리기만 하고.
     * 그냥 ulnDesc 안으로 집어넣을까?
     */
    ulnDescAllocType  mAllocType;             /* SQL_DESC_ALLOC_TYPE.
                                                 Explicit 인지 Implicit 인지의 여부.
                                                 ULN_DESC_ALLOCTYPE_IMPLICIT(SQL_DESC_ALLOC_AUTO)
                                                 ULN_DESC_ALLOCTYPE_IMPLICIT(SQL_DESC_ALLOC_USER) */

    ulvULen          *mRowsProcessedPtr;      /* SQL_DESC_ROWS_PROCESSED_PTR */
    acp_uint16_t     *mArrayStatusPtr;        /* SQL_DESC_ARRAY_STATUS_PTR */
    // fix BUG-20745 BIND_OFFSET_PTR 지원
    ulvULen          *mBindOffsetPtr;         /* SQL_DESC_BIND_OFFSET_PTR */

    acp_uint32_t      mArraySize;             /* SQL_DESC_ARRAY_SIZE.
                                                 Array Binding 시 Array 의 갯수 */

    acp_uint32_t      mBindType;              /* SQL_DESC_BIND_TYPE
                                                 SQL_BIND_BY_COLUMN 혹은 row 갯수. */

    /*
     * 사용자가 바인드한 컬럼 혹은 파라미터의 인덱스 정보를 관리하기 위한 멤버들.
     *
     * 아래의 멤버들에 값을 세팅하는 일이 발생하는 함수들 :
     *      0. ulnDescInitialize()
     *      1. ulnDescAddDescRec()         : mHighestIndex, mLowestIndex
     *      2. ulnDescRemoveDescRec()      : mHighestIndex, mLowestIndex
     */
    acp_uint16_t     mHighestBoundIndex;     /* SQL_DESC_COUNT */
} ulnDescHeader;

struct ulnDesc
{
    ulnObject      mObj;
    ulnObject     *mParentObject;

    ulnDescHeader  mHeader;

    acp_uint32_t   mInitialSPIndex;        /* 최초에 Desc 가 생성되었을 때 찍은 SP 의 인덱스.
                                              모든 바인딩 정보를 없앨려면 이 시점으로
                                              freetoSP 하면 된다. */

    acp_list_t     mAssociatedStmtList;    /* ulnStmtAssociatedWithDesc 들의 리스트이다. */

    acp_uint32_t   mDescRecArraySize;
    ulnDescRec   **mDescRecArray;
    acp_list_t     mDescRecList;           /* ulnDescRec 들의 리스트 */
    acp_uint16_t   mDescRecCount;          /* mDescRecList 에 있는 ulnDescRec 의 갯수 */

    acp_list_t     mPDContextList;         /* 활성화된 PDContext 들의 리스트 */

    /* BUG-44858 DescRec를 재활용하자 */
    acp_list_t     mFreeDescRecList;       /* Free된 ulnDescRec 들의 리스트 */

    /*
     * PROJ-1697: SQLSetDescField or SQLSetDescRec에서 DescRec을 수정할 경우,
     * 이를 stmt에 반영하기 위함. 만약 SQLCopyDesc가 구현된다면 mStmt는
     * 제거되어야 한다.
     */
    void           *mStmt;
};

/*
 * Function Declarations
 */

ACI_RC ulnDescCreate(ulnObject       *aParentObject,
                     ulnDesc        **aOutputDesc,
                     ulnDescType      aDescType,
                     ulnDescAllocType aAllocType);

ACI_RC ulnDescDestroy(ulnDesc *aDesc);

ACI_RC ulnDescInitialize(ulnDesc *aDesc, ulnObject *aParentObject);
ACI_RC ulnDescInitializeUserPart(ulnDesc *aDesc);

ACI_RC ulnDescRollBackToInitial(ulnDesc *aDesc);

ACI_RC ulnDescAddDescRec(ulnDesc *aDesc, ulnDescRec *aDescRec);
ACI_RC ulnDescRemoveDescRec(ulnDesc    *aDesc,
                            ulnDescRec *aDescRec,
                            acp_bool_t  aPrependToFreeList);

ACI_RC ulnDescAddStmtToAssociatedStmtList(ulnStmt *aStmt, ulnDesc *aDesc);
ACI_RC ulnDescRemoveStmtFromAssociatedStmtList(ulnStmt *aStmt, ulnDesc *aDesc);

ACI_RC ulnDescSaveAssociatedStmtList(ulnDesc      *aDesc,
                                     uluMemory    *aMemory,
                                     acp_list_t   *aTempStmtList,
                                     acp_uint32_t *aTempSP);

ACI_RC ulnDescRecoverAssociatedStmtList(ulnDesc      *aDesc,
                                        uluMemory    *aMemory,
                                        acp_list_t   *aTempStmtList,
                                        acp_uint32_t  aTempSP);

ACI_RC ulnDescCheckConsistency(ulnDesc *aDesc);

ACP_INLINE ulnDescAllocType ulnDescGetAllocType(ulnDesc *aDesc)
{
    return aDesc->mHeader.mAllocType;
}

ACP_INLINE acp_uint32_t ulnDescGetBindType(ulnDesc *aDesc)
{
    return aDesc->mHeader.mBindType;
}

ACP_INLINE void ulnDescSetBindType(ulnDesc *aDesc, acp_uint32_t aType)
{
    aDesc->mHeader.mBindType = aType;
}

// fix BUG-20745 BIND_OFFSET_PTR 지원
ACP_INLINE void ulnDescSetBindOffsetPtr(ulnDesc *aDesc, ulvULen *aOffsetPtr)
{
    aDesc->mHeader.mBindOffsetPtr = aOffsetPtr;
}

ACP_INLINE ulvULen *ulnDescGetBindOffsetPtr(ulnDesc *aDesc)
{
    return aDesc->mHeader.mBindOffsetPtr;
}

ACP_INLINE ulvULen ulnDescGetBindOffsetValue(ulnDesc *aDesc)
{
    ulvULen *sOffsetPtr = NULL;

    /*
     * 만약 사용자가 Bind Offset 을 지정하였다면 그 값을 얻어온다.
     */
    sOffsetPtr = aDesc->mHeader.mBindOffsetPtr;

    if (sOffsetPtr != NULL)
    {
        return *sOffsetPtr;
    }
    else
    {
        return 0;
    }
}

void          ulnDescInitStatusArrayValues(ulnDesc      *aDesc,
                                           acp_uint32_t  aStartIndex,
                                           acp_uint32_t  aArraySize,
                                           acp_uint16_t  aValue);

ACP_INLINE void ulnDescSetArrayStatusPtr(ulnDesc *aDesc, acp_uint16_t *aStatusPtr)
{
    aDesc->mHeader.mArrayStatusPtr = aStatusPtr;
}

ACP_INLINE acp_uint16_t *ulnDescGetArrayStatusPtr(ulnDesc *aDesc)
{
    return aDesc->mHeader.mArrayStatusPtr;
}

ACP_INLINE ulvULen *ulnDescGetRowsProcessedPtr(ulnDesc *aDesc)
{
    return aDesc->mHeader.mRowsProcessedPtr;
}

ACP_INLINE void ulnDescSetRowsProcessedPtr(ulnDesc *aDesc, ulvULen *aProcessedPtr)
{
    aDesc->mHeader.mRowsProcessedPtr = aProcessedPtr;
}

ACP_INLINE void ulnDescSetRowsProcessedPtrValue(ulnDesc *aDesc, ulvULen aValue)
{
    if (aDesc->mHeader.mRowsProcessedPtr != NULL)
    {
        // CASE-21536
        // 표준은 SQLULEN이 맞으나 고객이 사용하는 프로그램이 SQLUINTEGER를 사용하고 있으며,
        // 해당 프로그램은 다른 DB와 연동이 잘 되는 상황이어서, 결국 우리가 임시로 코드 수정을 해 배포함.
        // 코드는 표준에 맞도록 유지되고 있으나 위 고객이 새로운 패키지를 요청하면
        // 아래 코드 주석을 다음과 같이 변경해 컴파일 해야 함.
        // *((acp_uint32_t*)(aDesc->mHeader.mRowsProcessedPtr)) = (acp_uint32_t)aValue;
        *(aDesc->mHeader.mRowsProcessedPtr) = aValue;
    }
}

ACP_INLINE void ulnDescSetArraySize(ulnDesc *aDesc, acp_uint32_t aArraySize)
{
    aDesc->mHeader.mArraySize = aArraySize;
}

ACP_INLINE acp_uint32_t ulnDescGetArraySize(ulnDesc *aDesc)
{
    return aDesc->mHeader.mArraySize;
}

/*
 * Descriptor 에 바인드되어 있는 DescRec 들의 인덱스와 관련된 함수들
 */
acp_uint16_t ulnDescFindHighestBoundIndex(ulnDesc *aDesc, acp_uint16_t aCurrentHighestBoundIndex);

ACP_INLINE void ulnDescSetHighestBoundIndex(ulnDesc *aDesc, acp_uint16_t aIndex)
{
    aDesc->mHeader.mHighestBoundIndex = aIndex;
}

ACP_INLINE acp_uint16_t ulnDescGetHighestBoundIndex(ulnDesc *aDesc)
{
    return aDesc->mHeader.mHighestBoundIndex;
}

ACP_INLINE ulnDescRec *ulnDescGetDescRec(ulnDesc *aDesc, acp_uint16_t aIndex)
{
    if (aIndex > aDesc->mHeader.mHighestBoundIndex )
    {
        return NULL;
    }
    else
    {
        return aDesc->mDescRecArray[aIndex];
    }
}

/*
 * DescRecArray 관련 함수들
 */
ulnDescRec **ulnDescGetDescRecArrayEntry(ulnDesc *aDesc, acp_uint16_t aIndex);

/*
 * =======================
 * PDContext
 * =======================
 */

void ulnDescAddPDContext(ulnDesc *aDesc, ulnPDContext *aPDContext);
void ulnDescRemovePDContext(ulnDesc *aDesc, ulnPDContext *aPDContext);
void ulnDescRemoveAllPDContext(ulnDesc *aDesc);

/*
 * StatusArray 관련 함수들
 */
#if 0
ACI_RC ulnDescSetStatusArrayElementValue(ulnDessc, *aDesc, acp_uint32_t aRowNumber, acp_uint16_t aValue);
ACI_RC ulnDescArrangeStatusArray(ulnDesc *aImpDesc, acp_uint32_t aArrayCount, acp_uint16_t aInitValue);
#endif

#endif /* _O_ULN_DESC_H_ */

