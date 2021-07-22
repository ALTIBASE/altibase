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
#include <ulnDesc.h>

ACI_RC ulnDescCreate(ulnObject         *aParentObject,
                     ulnDesc          **aOutputDesc,
                     ulnDescType        aDescType,
                     ulnDescAllocType   aAllocType)
{
    ULN_FLAG(sNeedDestroyMemory);
    ULN_FLAG(sNeedDestroyDiagHeader);

    ulnDesc      *sDesc;
    uluMemory    *sMemory;
    uluChunkPool *sPool;

    acp_sint16_t  sInitialState;

    ACE_ASSERT(aDescType > ULN_DESC_TYPE_NODESC && aDescType < ULN_DESC_TYPE_MAX);

    sPool = aParentObject->mPool;

    /*
     * 메모리 인스턴스 생성. 청크 풀은 상위 오브젝트의 것을 사용.
     */

    ACI_TEST(uluMemoryCreate(sPool, &sMemory) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedDestroyMemory);

    /*
     * uluDesc 인스턴스 생성.
     */

    ACI_TEST(sMemory->mOp->mMalloc(sMemory,
                                   (void **)&sDesc,
                                   ACI_SIZEOF(ulnDesc)) != ACI_SUCCESS);

    /*
     * ODBC 3.0 스펙에 따르면,
     * Explicit Descriptor는 사용자가 SQLAllocHandle()을 이용해서 할당하는
     * 디스크립터이다.
     * Explicit 디스크립터는 반드시 DBC에 할당되어야 한다.
     */

    if (aAllocType == ULN_DESC_ALLOCTYPE_EXPLICIT)
    {
        /*
         * BUGBUG : parent object 가 dbc 가 아니면 invalid handle 을 내어 줘야 한다.
         */
        sInitialState = ULN_S_D1e;
    }
    else
    {
        sInitialState = ULN_S_D1i;
    }

    ulnObjectInitialize(&sDesc->mObj,
                        ULN_OBJ_TYPE_DESC,
                        aDescType,
                        sInitialState,
                        aParentObject->mPool,
                        sMemory);

    /*
     * 상위 구조와 Lock 을 공유한다. 상위 구조는 Dbc 가 될 수도, Stmt 가 될 수도 있다.
     */

    sDesc->mObj.mLock = aParentObject->mLock;

    ACI_TEST(ulnCreateDiagHeader((ulnObject *)sDesc, aParentObject->mDiagHeader.mPool)
             != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedDestroyDiagHeader);

    /*
     * DescRecArray 를 생성한다.
     * 일단, 사이즈 계산하기가 귀찮으니, 100 개의 element 를 기본 단위로 하자.
     */

    sDesc->mDescRecArraySize = 50;
    ACI_TEST(acpMemAlloc((void**)&sDesc->mDescRecArray,
                         ACI_SIZEOF(ulnDescRec *) * sDesc->mDescRecArraySize)
             != ACP_RC_SUCCESS);
    acpMemSet(sDesc->mDescRecArray, 0, sDesc->mDescRecArraySize * ACI_SIZEOF(ulnDescRec *));

    /*
     * SP 갯수가 초과할 경우 에러가 남.
     */

    ACI_TEST(sMemory->mOp->mMarkSP(sMemory) != ACI_SUCCESS);
    sDesc->mInitialSPIndex = sMemory->mOp->mGetCurrentSPIndex(sMemory);

    /*
     * 만들어진 descriptor 를 리턴
     */

    *aOutputDesc = sDesc;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedDestroyDiagHeader)
    {
        ulnDestroyDiagHeader(&sDesc->mObj.mDiagHeader, ACP_FALSE);
    }

    ULN_IS_FLAG_UP(sNeedDestroyMemory)
    {
        sMemory->mOp->mDestroyMyself(sMemory);
    }

    return ACI_FAILURE;
}

ACI_RC ulnDescDestroy(ulnDesc *aDesc)
{
    ACE_ASSERT(aDesc != NULL);
    ACE_ASSERT(ULN_OBJ_GET_TYPE(aDesc) == ULN_OBJ_TYPE_DESC);

    acpMemFree(aDesc->mDescRecArray);
    aDesc->mDescRecArray     = NULL;
    aDesc->mDescRecArraySize = 0;

    /*
     * PutData 중이었던 DescRec 가 있으면 찾아가서 mTempBuffer 를 해제해 준다.
     */
    ulnDescRemoveAllPDContext(aDesc);

    /*
     * DESC 가 가진 DiagHeader 에 딸린 메모리 객체들을 파괴한다.
     */
    ACI_TEST(ulnDestroyDiagHeader(&(aDesc->mObj.mDiagHeader), ULN_DIAG_HDR_NOTOUCH_CHUNKPOOL)
             != ACI_SUCCESS);

    /*
     * DESC 를 없애기 직전에 실수에 의한 재사용을 방지하기 위해서 ulnObject 에 표시를 해 둔다.
     * BUG-15894 와 같은 사용자 응용 프로그램에 의한 버그를 방지하기 위해서이다.
     */
    aDesc->mObj.mType = ULN_OBJ_TYPE_MAX;

    /*
     * DESC 가 소유한 ulumemory 를 파괴한다.
     * DESC 가 가지고 있는 mAssociatedStmtList 가 저장된 곳도 자동으로 파괴됨.
     */
    aDesc->mObj.mMemory->mOp->mDestroyMyself(aDesc->mObj.mMemory);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ulnInitializeDesc
 *
 * ulnDesc 객체의 관리 필드들을 초기화한다.
 * 사용자가 세팅하는 부분인
 *      SQL_DESC_BIND_TYPE          : mBindType
 *      SQL_DESC_ARRAY_SIZE         : mArraySize
 *      SQL_DESC_BIND_OFFSET_PTR    : mBindOffsetPtr
 *      SQL_DESC_ROWS_PROCESSED_PTR : mRowsProcessedPtr
 *      SQL_DESC_ARAY_STATUS_PTR    : mArrayStatusPtr
 * 은 초기화하지 않는다.
 */
ACI_RC ulnDescInitialize(ulnDesc *aDesc, ulnObject *aParentObject)
{
    /*
     * 관리 필드들의 초기화
     */

    acpListInit(&aDesc->mAssociatedStmtList);
    acpListInit(&aDesc->mDescRecList);
    acpListInit(&aDesc->mFreeDescRecList); /* BUG-44858 */

    aDesc->mParentObject = aParentObject;
    aDesc->mDescRecCount = 0;

    // fix BUG-24380
    // Desc의 부모가 Stmt면 Stmt의 포인터를 저장
    if (aParentObject->mType == ULN_OBJ_TYPE_STMT)
    {
        aDesc->mStmt = aParentObject;
    }
    else
    {
        aDesc->mStmt = NULL;
    }

    /*
     * mHeader의 초기화
     */
    if (aDesc->mObj.mState == ULN_S_D1i)
    {
        aDesc->mHeader.mAllocType = ULN_DESC_ALLOCTYPE_IMPLICIT;
    }
    else
    {
        if (aDesc->mObj.mState == ULN_S_D1e)
        {
            aDesc->mHeader.mAllocType = ULN_DESC_ALLOCTYPE_EXPLICIT;
        }
        else
        {
            ACE_ASSERT(0);
        }
    }

    acpListInit(&aDesc->mPDContextList);

    aDesc->mHeader.mHighestBoundIndex = 0;                   /* SQL_DESC_COUNT */

    /*
     * DescRecArray 를 초기화
     *
     * Note : 이 함수는 unbind 시에도 호출되는데, unbind 를 하면, Desc 의 uluMemory 를 초기
     *        상태로 rolback 시켜버린다.
     *        이때, DescRecArray 에 있던 배열들도 모두 해제되고 DescRecArray 도 초기 상태로
     *        돌아가 줘야 한다.
     */
    // memset 을 해야 하나?
    // 아니면, ulnDescGetDescRec() 에서 highest bound index 를 초과하는 index 가 올 경우
    // NULL 을 돌려주도록 했는데, 그것으로 충분한가?
    // uluArrayInitializeToInitial(aDesc->mDescRecArray);
    acpMemSet(aDesc->mDescRecArray, 0, aDesc->mDescRecArraySize * ACI_SIZEOF(ulnDescRec *));

    return ACI_SUCCESS;
}

ACI_RC ulnDescInitializeUserPart(ulnDesc *aDesc)
{
    /*
     * ulnDesc 에서 사용자가 세팅하는 부분인
     *      SQL_DESC_BIND_TYPE          : mBindType
     *      SQL_DESC_ARRAY_SIZE         : mArraySize
     *      SQL_DESC_BIND_OFFSET_PTR    : mBindOffsetPtr
     *      SQL_DESC_ROWS_PROCESSED_PTR : mRowsProcessedPtr
     *      SQL_DESC_ARAY_STATUS_PTR    : mArrayStatusPtr
     * 을 초기화한다.
     *
     * 아래의 함수들에서 ulnDescInitialize() 만 호출해야 하기 때문에
     * 이처럼 따로 빼 두어야 할 필요가 있다 :
     *      Prepare
     *      Unbind
     *      ResetParams
     */

    aDesc->mHeader.mBindType          = SQL_BIND_BY_COLUMN;
    aDesc->mHeader.mArraySize         = 1;
    aDesc->mHeader.mRowsProcessedPtr  = NULL;
    aDesc->mHeader.mBindOffsetPtr     = NULL;
    aDesc->mHeader.mArrayStatusPtr    = NULL;

    return ACI_SUCCESS;
}

/*
 * ulnDescRollBackToInitial
 *
 * 함수가 하는 일 :
 *  - ulnDesc 를 처음 생성된 상태로 되돌린다.
 *  - 바인드를 한다거나 해서 할당된 모든 메모리를 해제한다.
 *    단지, ulnDesc 를 위한 메모리만 남겨두고 해제한다.
 */
ACI_RC ulnDescRollBackToInitial(ulnDesc *aDesc)
{
    acp_uint32_t sInitialSPIndex;

    sInitialSPIndex = aDesc->mInitialSPIndex;

    ACI_TEST(aDesc->mObj.mMemory->mOp->mFreeToSP(aDesc->mObj.mMemory, sInitialSPIndex)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ================================================
 * PutDataContext 리스트를 조작하는 함수들
 * ================================================
 */

void ulnDescAddPDContext(ulnDesc *aDesc, ulnPDContext *aPDContext)
{
    acpListInitObj(&aPDContext->mList, aPDContext);

    acpListAppendNode(&aDesc->mPDContextList, &aPDContext->mList);
}

void ulnDescRemovePDContext(ulnDesc *aDesc, ulnPDContext *aPDContext)
{
    ACP_UNUSED(aDesc);

    acpListDeleteNode(&aPDContext->mList);
}

void ulnDescRemoveAllPDContext(ulnDesc *aDesc)
{
    acp_list_node_t *sIterator;
    acp_list_node_t *sIteratorNext;

    ACP_LIST_ITERATE_SAFE(&aDesc->mPDContextList, sIterator, sIteratorNext)
    {
        /*
         * 혹시라도 PutData() 하다가 에러가 났을 경우 해당 PDContext 의 mBuffer 는 free
         * 되지 않은 채로 남아 잇다.
         * 이것들을 모아서 처리해 준다.
         */
        ((ulnPDContext *)sIterator)->mOp->mFinalize((ulnPDContext *)sIterator);
        ulnDescRemovePDContext(aDesc, (ulnPDContext *)sIterator);
    }
}

/*
 * descriptor record array list 관련 함수들
 *
 * 생각같아서는 가중치 트리라든지, 2, 3 중 인덱스를 이용해서 하고 싶지만,
 * 그렇게 하면, 정작 필요한 것에 비해서 완전 오버다. 일반적으로 100 개 안쪽의
 * 컬럼 혹은 파라미터를 바인드한다고 생각하면, 보통 desc rec array header list 는 하나밖에
 * 생성되지 않는다.
 * 현재까지 보고된 가장 많은 바인드 갯수는 800개가 약간 못미치는 것으로 알고 있다.
 * 800 개면, 헤더 7개이다. 최악의 경우 DescRec 하나를 찾기 위해서 LIST ITERATION 을 6 번 한다.
 * 현재 1K 단위에서 하나의 헤더에는 124 개의 배열 원소가 할당된다. (64비트 플랫폼 기준)
 */

/*
 * Invoking Index 는 아래의 함수의 호출을 유발한 디스크립터 레코드의 인덱스이다.
 *
 * 예를 들어 count 10 의 DescRecArray 가 아래의 그림과 같이 달려 있다고 하자 :
 *
 *      LIST  0 10 * * * * * * * * * * ; start index 0, cnt 10
 *      LIST 20 10 * * * * * * * * * * ; start index 20, cnt 10
 *
 * 이 때 사용자가 ParamNumber 17 번을 바인드하면, 함수호출을 따라 오다가
 * ulnDescAddDescRec() 함수에 와서 이 함수가 aInvokingIndex 에 17 을 가지고 호출되게 된다.
 *
 * 그러면 이 함수는 DescRecArray 를 하나 만드는데, start index 10, cnt 10 인 녀석을
 * 만들어서 리스트에 추가한다.
 * 그러면 리스트는 다음 그림과 같이 된다 :
 *
 *      LIST  0 10 * * * * * * * * * * ; start index 0, cnt 10
 *      LIST 20 10 * * * * * * * * * * ; start index 20, cnt 10
 *      LIST 10 10 * * * * * * * * * * ; start index 10, cnt 10
 */

/*
 * Note : ulnDesc 와 ulnDescRec 와의 관계를 설명하는 그림.
 *
 * +--------------+             +-DescRec-+       +-DescRec-+
 * | mDescRecList |-------------|  mList  |-------|  mList  |---
 * +--------------+             |         |       |         |
 *                              |         |       |         |
 * +-------------------+     +->|         |   +-->|         |
 * | mDescRecArrayList |     |  +---------+   |   +---------+
 * +---|---------------+    /   _____________/
 *     |                   /   /
 * +-DescRecArrayHeader---|-+-|-+---+---+----------------------------------+
 * | mList | ....       | * | * |   |   | . . .                            |
 * +---|--------------------+---+---+---+----------------------------------+
 *     |
 * +---|--------------------+---+---+---+----------------------------------+
 * | mList | ....       |   |   |   | A | . . .                            |
 * +---|--------------------+---+---+---+----------------------------------+
 *     |
 * +---|--------------------+---+---+---+----------------------------------+
 * | mList | ....       |   |   |   |   | . . .                            |
 * +--------------------|---+---+---+---+----------------------------------|
 *                      |                                                  |
 *                      |<------------ mDescRecArrayUnitCount ------------>|
 *
 * Note : 위의 그림에서, uluArrayGetElement() 함수를 호출하면, 이를테면, 위 그림의
 *        A 를 가리키는 포인터를 리턴한다.
 *        즉, (ulnDescRec *) 타입을 가리키는 포인터인 (ulnDescRec **) 를 리턴한다.
 */

ACI_RC ulnDescAddDescRec(ulnDesc *aDesc, ulnDescRec *aDescRec)
{
    acp_uint32_t sSizeDiffrence;
    acp_uint32_t sSizeToExtend;

    /*
     * DescRecArray 에 추가한다.
     * 해당 인덱스에 미리 들어가 있는 것이 있든 없든간에 무작정 덮어써버린다.
     */

    if (aDescRec->mIndex >= aDesc->mDescRecArraySize)
    {
        sSizeDiffrence = aDescRec->mIndex - aDesc->mDescRecArraySize;
        sSizeToExtend  = (sSizeDiffrence / 50 + 1) * 50;

        ACI_TEST(acpMemRealloc((void**)&aDesc->mDescRecArray,
                                ACI_SIZEOF(ulnDescRec *) *
                                (aDesc->mDescRecArraySize + sSizeToExtend))
                 != ACP_RC_SUCCESS);

        acpMemSet(aDesc->mDescRecArray + aDesc->mDescRecArraySize,
                      0,
                      sSizeToExtend * ACI_SIZEOF(ulnDescRec *));

        aDesc->mDescRecArraySize += sSizeToExtend;
    }

    aDesc->mDescRecArray[aDescRec->mIndex] = aDescRec;

    /*
     * DescRecList 에 추가한다.
     *
     * Note : 최근에 추가한 DescRec 일수록 앞에 존재한다
     */
    acpListPrependNode(&(aDesc->mDescRecList), (acp_list_t *)aDescRec);

    /*
     * 카운터 증가
     */
    aDesc->mDescRecCount++;

    /*
     * Note : ulnDesc::mHeader::mCount 는
     *        ODBC 에서 정의하는 의미와 완전히 똑같은 의미로 동작하도록 한다.
     *        ODBC 에서 정의하는 의미는 헤더파일을 참조하라.
     *
     *        HighestBoundIndex 의 이름으로 쓴다.
     */

    if (ulnDescGetHighestBoundIndex(aDesc) < aDescRec->mIndex)
    {
        ulnDescSetHighestBoundIndex(aDesc, aDescRec->mIndex);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnDescRemoveDescRec(ulnDesc    *aDesc,
                            ulnDescRec *aDescRec,
                            acp_bool_t  aPrependToFreeList)
{
    acp_uint16_t sHighestIndex;

    /*
     * BUGBUG : 여기서 unbound 한 DescRec 는 그냥 좀비로 남게 된다.
     *          재활용할 방안을 모색해 보자.
     *
     *          재바인드 시에는 재활용이 되지만, 사용자가 문자적으로 딱 지정해서
     *          "UnBind" 를 하게 되면 방법이 대략 난감하다.
     *
     *          FreeDescRecList 를 하나 만들어?
     *          일단 나중에 생각하자.
     */

    ACI_TEST(aDesc->mDescRecCount == 0);

    /*
     * BUGBUG : 아래의 경우 메모리 매니지 에러가 아니고, 사용자가 언바인드 하면서 실수로
     *          바인드하지도 않은 인덱스를 준 것일 가능성도 있다.
     */

    ACI_TEST(aDescRec->mIndex >= aDesc->mDescRecArraySize);

    /*
     * DescRecArray 의 해당 엔트리에 널 찍어주기
     */

    aDesc->mDescRecArray[aDescRec->mIndex] = NULL;

    /*
     * 리스트에서 삭제
     */

    acpListDeleteNode((acp_list_t *)aDescRec);

    /*
     * Desc 에 달려 있는 DescRec 의 갯수를 나타내는 변수인 mDescRecCount 감소시키기
     */

    aDesc->mDescRecCount--;


    /*
     * Note : ODBC 에서는 컬럼을 unbind 한 후에, 그 descriptor 의 SQL_DESC_COUNT 는
     *        그 stmt 에서 unbind 한 컬럼을 제외한 나머지 컬럼들 중 가장 큰 column number 를
     *        가지는 desc record 의 column number 가 되어야 한다고 규정하고 있다.
     */

    sHighestIndex = ulnDescGetHighestBoundIndex(aDesc);

    if (sHighestIndex == aDescRec->mIndex)
    {
        sHighestIndex = ulnDescFindHighestBoundIndex(aDesc, sHighestIndex);
        ulnDescSetHighestBoundIndex(aDesc, sHighestIndex);
    }

    /* BUG-44858 메모리 재활용을 위해 FreeList에 넣어둔다. */
    if (aPrependToFreeList == ACP_TRUE)
    {
        acpListPrependNode(&aDesc->mFreeDescRecList, (acp_list_node_t *)aDescRec);
    }
    else
    {
        /* A obsolete convention */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ulnDescAddStmtToAssociatedStmtList
 *
 * STMT 를 가리키는 포인터를 DESC 의 mAssociatedStmtList 에 추가한다.
 * SQLSetStmtAttr() 함수 수행중에 호출될 수도 있다.
 */
ACI_RC ulnDescAddStmtToAssociatedStmtList(ulnStmt *aStmt, ulnDesc *aDesc)
{
    ulnStmtAssociatedWithDesc *sItem;

    ACE_ASSERT(aStmt != NULL);
    ACE_ASSERT(aDesc != NULL);

    /*
     * 리스트 Item 을 만들 메모리 확보
     */
    ACI_TEST(aDesc->mObj.mMemory->mOp->mMalloc(aDesc->mObj.mMemory,
                                               (void **)&sItem,
                                               ACI_SIZEOF(ulnStmtAssociatedWithDesc))
             != ACI_SUCCESS);

    sItem->mStmt = aStmt;

    acpListAppendNode(&aDesc->mAssociatedStmtList, &sItem->mList);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ulnDescRemoveStmtFromAssociatedStmtList
 *
 * STMT 를 DESC 의 mAssociatedStmtList 에서 제거한다.
 * 하나만 제거하는 것이 아니라 STMT 가 여러번 출현하면 그 출현하는거 모조리 다 제거한다.
 *
 * @note
 *  호출 포인트
 *      - ulnDestroyStmt() 함수 수행 중에 호출된다.
 *      - DESC 를 STMT 의 Explicit Ard/Apd 로 설정할 때 앞번에
 *        설정된 Explicit Ard/Apd 가 존재할때 호출된다.
 */
ACI_RC ulnDescRemoveStmtFromAssociatedStmtList(ulnStmt *aStmt, ulnDesc *aDesc)
{
    /*
     * BUGBUG : 발견해서 지우면 SUCCESS, 발견 못하면 FAILURE 를 리턴하게 하는것은 어떤가?
     */

    acp_list_node_t *sIterator;
    acp_list_node_t *sIteratorNext;

    ACE_ASSERT(aStmt != NULL);
    ACE_ASSERT(aDesc != NULL);

    ACP_LIST_ITERATE_SAFE(&(aDesc->mAssociatedStmtList), sIterator, sIteratorNext)
    {
        if (((ulnStmtAssociatedWithDesc *)sIterator)->mStmt == aStmt)
        {
            /*
             * BUGBUG : 여기서 할당된 메모리를 해제해 줘야 하나 방법이 묘연하다.
             *          그다지 문제되지 않는 것이, DESC 를 해제하면서 DESC 의 
             *          uluMemory를 파괴하면 자동으로 다 파괴되므로 걱정 안해도 된다.
             */
            acpListDeleteNode(sIterator);

            /*
             * 여러번 바인드 되었어도 모조리 제거하자
             * ---> break 안하고 계속 돌아라.
             */
        }
    }

    return ACI_SUCCESS;
}

/*
 * AssociatedStmtList 를 백업하고 복구하는 함수들.
 *
 * 아래의 두 함수는 ulnFreeStmtResetParams 와 ulnFreeStmtUnbind 에서 호출된다.
 * 예제는 그곳에서 찾을 수 있다.
 *
 * 인자로 받는 aMemory 는 일반적으로 상위 객체의 메모리를 사용하면 되겠다.
 *
 * Note : 아래의 두 함수
 *        ulnDescSaveAssociatedStmtList() 와
 *        ulnDescRecoverAssociatedStmtList() 는 반드시
 *        짝을 이루어서 쓰여야 한다.
 *
 *        또한, 함수의 인자로 사용되는 모든 인자는 동일한 녀석들로 사용하도록 조심해야 한다!!!
 */
ACI_RC ulnDescSaveAssociatedStmtList(ulnDesc      *aDesc,
                                     uluMemory    *aMemory,
                                     acp_list_t   *aTempStmtList,
                                     acp_uint32_t *aTempSP)
{
    acp_list_node_t           *sIterator;
    ulnStmtAssociatedWithDesc *sAssociatedStmt;

    acpListInit(aTempStmtList);

    *aTempSP = aMemory->mOp->mGetCurrentSPIndex(aMemory);
    ACI_TEST(aMemory->mOp->mMarkSP(aMemory) != ACI_SUCCESS);

    ACP_LIST_ITERATE(&aDesc->mAssociatedStmtList, sIterator)
    {
        ACI_TEST(aMemory->mOp->mMalloc(aMemory,
                                       (void **)(&sAssociatedStmt),
                                       ACI_SIZEOF(ulnStmtAssociatedWithDesc)) != ACI_SUCCESS);

        sAssociatedStmt->mStmt = ((ulnStmtAssociatedWithDesc *)sIterator)->mStmt;

        acpListAppendNode(aTempStmtList, (acp_list_t *)sAssociatedStmt);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnDescRecoverAssociatedStmtList(ulnDesc      *aDesc,
                                        uluMemory    *aMemory,
                                        acp_list_t   *aTempStmtList,
                                        acp_uint32_t  aTempSP)
{
    acp_list_node_t *sIterator;

    ACP_LIST_ITERATE(aTempStmtList, sIterator)
    {
        ACI_TEST(ulnDescAddStmtToAssociatedStmtList(
                ((ulnStmtAssociatedWithDesc *)sIterator)->mStmt,
                aDesc) != ACI_SUCCESS);
    }

    /*
     * 앞서 찍었던 temp sp 로 원복한다.
     */
    ACI_TEST(aMemory->mOp->mFreeToSP(aMemory, aTempSP) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ODBC 3.0 의 SQLSetDescRec() 함수 레퍼런스 매뉴얼의 맨 뒷부분에서 설명하고 있는
 * Consistency chech 을 구현한 함수이다.
 *
 * - Consistency check 는 IRD 에 대해서는 수행될 수 없다.
 * - IPD 에 대해서 수행될 경우, IPD 용 field 가 아니더라도 이 체크를 수행하기 위해서
 *   Descriptor 의 필드들을 강제로 세팅할 수도 있다.
 */
ACI_RC ulnDescCheckConsistency(ulnDesc *aDesc)
{
    ACI_TEST(ULN_OBJ_GET_TYPE(aDesc) != ULN_OBJ_TYPE_DESC);
    ACI_TEST(ULN_OBJ_GET_DESC_TYPE(aDesc) == ULN_DESC_TYPE_IRD);

    /*
     * BUGBUG : 함수 구현을 해야 한다.
     */

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static void ulnDescInitStatusArrayValuesCore(ulnDesc      *aDesc,
                                             acp_uint32_t  aStartIndex,
                                             acp_uint32_t  aArraySize,
                                             acp_uint16_t  aValue,
                                             acp_uint16_t *aStatusArrayPtr)
{
    acp_uint32_t sRowNumber;

    /*
     * Note : APD 의 SQL_DESC_ARRAY_STATUS_PTR 은 driver 가 어떻게 행동할지 app 가 지시하는
     *        것이다. 즉, driver 의 입장에서는 input 이다.
     */
    ACE_ASSERT(ULN_OBJ_GET_DESC_TYPE(aDesc) == ULN_DESC_TYPE_IRD ||
               ULN_OBJ_GET_DESC_TYPE(aDesc) == ULN_DESC_TYPE_IPD);

    ACE_ASSERT(aArraySize > 0);

    for (sRowNumber = aStartIndex; sRowNumber < aArraySize; sRowNumber++)
    {
        *(aStatusArrayPtr + sRowNumber) = aValue;
    }
}

/*
 * aArraySize 는 PARAMSET_SIZE 혹은 ROWSET_SIZE 이다.
 * 0 일 수 없고, 1 이상이어야만 한다.
 */
void ulnDescInitStatusArrayValues(ulnDesc      *aDesc,
                                  acp_uint32_t  aStartIndex,
                                  acp_uint32_t  aArraySize,
                                  acp_uint16_t  aValue)
{
    acp_uint16_t *sStatusArrayPtr;

    sStatusArrayPtr = ulnDescGetArrayStatusPtr(aDesc);

    if (sStatusArrayPtr != NULL)
    {
        ulnDescInitStatusArrayValuesCore(aDesc, aStartIndex, aArraySize, aValue, sStatusArrayPtr);
    }
}

/*
 * Descriptor 에 바인드되어 있는 DescRec 들의 인덱스와 관련된 함수들
 *
 *      1. 최대 유효인덱스
 *      2. 최대 인덱스
 *      3. 최소 인덱스
 */

static acp_uint16_t ulnDescFindEstIndex(ulnDesc *aDesc, acp_uint16_t aIndexToStartFrom, acp_sint16_t aIncrement)
{
    acp_uint16_t  i;
    ulnDescRec   *sDescRec = NULL;

    for (i = aIndexToStartFrom; i > 0; i += aIncrement)
    {
        sDescRec= ulnDescGetDescRec(aDesc, i);

        if (sDescRec!= NULL)
        {
            break;
        }
    }

    return i;
}

acp_uint16_t ulnDescFindHighestBoundIndex(ulnDesc *aDesc, acp_uint16_t aCurrentHighestBoundIndex)
{
    return ulnDescFindEstIndex(aDesc, aCurrentHighestBoundIndex, -1);
}

