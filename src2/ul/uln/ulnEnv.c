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
#include <ulnEnv.h>

/**
 * ulnCreateEnv.
 *
 * 함수가 하는 일
 *  - ENV 를 위한 uluChunkPool 인스턴스 생성
 *  - ENV 를 위한 uluMemory 인스턴스 생성
 *  - ENV 를 위한 Diagnostic Header 인스턴스 생성
 *  - ENV 의 Diagnostic Header 초기화
 *  - ENV 의 mObj 초기화
 *
 * @return
 *  - ACI_SUCCESS
 *  - ACI_FAILURE
 *    메모리 문제로 실패한 것임.
 *    할당하려고 시도했던 모든 메모리를 clear 한 후에 리턴하므로 에러만 처크해서
 *    사용자에게 바로 리턴해도 됨.
 */
ACI_RC ulnEnvCreate(ulnEnv **aOutputEnv)
{
    ULN_FLAG(sNeedFinalize);
    ULN_FLAG(sNeedDestroyPool);
    ULN_FLAG(sNeedDestroyMemory);
    ULN_FLAG(sNeedDestroyLock);

    ulnEnv          *sEnv;

    uluChunkPool    *sPool;
    uluMemory       *sMemory;

    acp_thr_mutex_t *sLock = NULL;

    /*
     * ulnInitialize
     */
    ACI_TEST(ulnInitialize() != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinalize);

    /*
     * 메모리 할당을 미리 해 둔다.
     * 이를 위해서 Chunk Pool 을 생성하고, Memory 도 생성한 후
     * Env 핸들을 생성된 메모리에다가 둔다.
     */
    sPool = uluChunkPoolCreate(ULN_SIZE_OF_CHUNK_IN_ENV, ULN_NUMBER_OF_SP_IN_ENV, 2);
    ACI_TEST(sPool == NULL);
    ULN_FLAG_UP(sNeedDestroyPool);

    ACI_TEST(uluMemoryCreate(sPool, &sMemory) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedDestroyMemory);

    ACI_TEST(sMemory->mOp->mMalloc(sMemory, (void **)(&sEnv), ACI_SIZEOF(ulnEnv)) != ACI_SUCCESS);
    ACI_TEST(sMemory->mOp->mMarkSP(sMemory) != ACI_SUCCESS);

    /*
     * ulnEnv 의 Object 부분을 초기화 한다.
     *
     * 최초 할당되면 상태를 Allocated로 둔다.
     * 따라서 상태 전이가 불필요하다.
     */
    ulnObjectInitialize((ulnObject *)sEnv,
                        ULN_OBJ_TYPE_ENV,
                        ULN_DESC_TYPE_NODESC,
                        ULN_S_E1,
                        sPool,
                        sMemory);

    /*
     * Lock 구조체 생성 및 초기화
     */
    ACI_TEST(uluLockCreate(&sLock) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedDestroyLock);

    ACI_TEST(acpThrMutexCreate(sLock, ACP_THR_MUTEX_DEFAULT) != ACP_RC_SUCCESS);

    /*
     * CreateDiagHeader에 실패했을 경우,
     * LABEL_MALLOC_FAIL_ENV 레이블로 점프해서 메모리 정리하는 것으로 충분한다
     */
    ACI_TEST(ulnCreateDiagHeader((ulnObject *)sEnv, NULL) != ACI_SUCCESS);

    sEnv->mObj.mLock = sLock;
    *aOutputEnv      = sEnv;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedDestroyLock)
    {
        acpThrMutexDestroy(sLock);
        uluLockDestroy(sLock);
    }

    ULN_IS_FLAG_UP(sNeedDestroyMemory)
    {
        sMemory->mOp->mDestroyMyself(sMemory);
    }

    ULN_IS_FLAG_UP(sNeedDestroyPool)
    {
        sPool->mOp->mDestroyMyself(sPool);
    }

    ULN_IS_FLAG_UP(sNeedFinalize)
    {
        ulnFinalize();
    }

    return ACI_FAILURE;
}

/**
 * ulnEnvDestroy.
 *
 * @param[in] aEnv
 *  파괴할 ENV 를 가리키는 포인터
 * @return
 *  - ACI_SUCCESS
 *    성공
 *  - ACI_FAILURE
 *    실패.
 *    호출자는 HY013 을 사용자에게 줘야 한다.
 */
ACI_RC ulnEnvDestroy(ulnEnv *aEnv)
{
    ulnObject       *sObject;
    uluMemory       *sMemory;
    uluChunkPool    *sPool;

    /*
     * BUGBUG
     * ulnDestroyDbc() 함수와 ENV 인것만 빼고 완전히 identical 한 코드이다.
     * 버그의 확률을 줄이기 위해서 두개의 코드를 합쳐야 할 필요가 있겠다.
     */

    ACE_ASSERT(ULN_OBJ_GET_TYPE(aEnv) == ULN_OBJ_TYPE_ENV);

    sObject = &aEnv->mObj;
    sPool   =  sObject->mPool;
    sMemory =  sObject->mMemory;

    /* BUG-35332 The socket files can be moved */
    ulnPropertiesDestroy(&aEnv->mProperties);

    /*
     * DiagHeader 에 딸린 메모리 객체를 파괴한다.
     */

    ACI_TEST(ulnDestroyDiagHeader(&sObject->mDiagHeader, ULN_DIAG_HDR_DESTROY_CHUNKPOOL)
             != ACI_SUCCESS);

    /*
     * DESC 를 없애기 직전에 실수에 의한 재사용을 방지하기 위해서 ulnObject 에 표시를 해 둔다.
     * BUG-15894 와 같은 사용자 응용 프로그램에 의한 버그를 방지하기 위해서이다.
     */

    sObject->mType = ULN_OBJ_TYPE_MAX;

    /*
     * 청크풀과 메모리 파괴
     */

    sMemory->mOp->mDestroyMyself(sMemory);
    sPool->mOp->mDestroyMyself(sPool);

    /*
     * Note : Lock 해제는 ulnFreeHandleEnv() 에서 한다.
     */

    /*
     * ulnFinalize
     */
    ACI_TEST(ulnFinalize() != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * ulnInitializeEnv.
 *
 * 함수가 하는 일
 * - ENV 구조체의 필드들의 초기화
 */
ACI_RC ulnEnvInitialize(ulnEnv *aEnv)
{
    aEnv->mDbcCount = 0;
    acpListInit(&aEnv->mDbcList);

    /*
     * Note:
     * MSDN ODBC 3.0 :
     * SQLAllocHandle does not set the SQL_ATTR_ODBC_VERSION
     * environment attribute when it is called to allocate an
     * environment handle;
     * the environment attribute must be set by the application
     * BUGBUG: 2.0이 맞는지, 0 이 맞는지..
     */
    aEnv->mOdbcVersion   = SQL_OV_ODBC2;

    aEnv->mConnPooling   = SQL_CP_OFF;
    aEnv->mConnPoolMatch = SQL_CP_STRICT_MATCH;
    aEnv->mOutputNts     = SQL_TRUE;

    aEnv->mUlnVersion    = 0;

    /* BUG-35332 The socket files can be moved */
    ulnPropertiesCreate(&aEnv->mProperties);

    /* PROJ-2733-DistTxInfo */
    aEnv->mSCN           = 0;

    /*
     * 상태를 E1 - Allocated 상태로 변경한다.
     * BUGBUG:지금 해야 할지 앞에서 했어야 할지 판단 필요.
     */
    ULN_OBJ_SET_STATE(aEnv, ULN_S_E1);

    return ACI_SUCCESS;
}

/**
 * ulnAddDbcToEnv.
 *
 * DBC 구조체를 ENV 의 mDbcList 에 추가한다.
 */
ACI_RC ulnEnvAddDbc(ulnEnv *aEnv, ulnDbc *aDbc)
{
    ACE_ASSERT(aEnv != NULL);
    ACE_ASSERT(aDbc != NULL);

    acpListAppendNode(&(aEnv->mDbcList), (acp_list_t *)aDbc);
    aEnv->mDbcCount++;

    aDbc->mParentEnv = aEnv;

    return ACI_SUCCESS;
}

/**
 * ulnRemoveDbcFromEnv.
 *
 * DBC 구조체를 ENV 의 mDbcList 에서 삭제한다.
 */
ACI_RC ulnEnvRemoveDbc(ulnEnv *aEnv, ulnDbc *aDbc)
{
    ACE_ASSERT(aEnv != NULL);
    ACE_ASSERT(aDbc != NULL);

    ACI_TEST(acpListIsEmpty(&aEnv->mDbcList));
    ACI_TEST(aEnv->mDbcCount == 0);

    acpListDeleteNode((acp_list_node_t *)aDbc);
    aEnv->mDbcCount--;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * ulnGetDbcCountFromEnv.
 *
 * ENV 구조체에 달려 있는 DBC 구조체의 갯수를 읽는다.
 */
acp_uint32_t ulnEnvGetDbcCount(ulnEnv *aEnv)
{
    return aEnv->mDbcCount;
}

ACI_RC ulnEnvSetOutputNts(ulnEnv *aEnv, acp_sint32_t aNts)
{
    aEnv->mOutputNts = aNts;
    return ACI_SUCCESS;
}

ACI_RC ulnEnvGetOutputNts(ulnEnv *aEnv, acp_sint32_t *aNts)
{
    *aNts = aEnv->mOutputNts;
    return ACI_SUCCESS;
}

acp_uint32_t ulnEnvGetOdbcVersion(ulnEnv *aEnv)
{
    return aEnv->mOdbcVersion;
}

ACI_RC ulnEnvSetOdbcVersion(ulnEnv *aEnv, acp_uint32_t aVersion)
{
    switch(aVersion)
    {
        case SQL_OV_ODBC3:
        case SQL_OV_ODBC2:
            aEnv->mOdbcVersion = aVersion;
            break;
        default:
            return ACI_FAILURE;
    }
    return ACI_SUCCESS;
}

ACI_RC ulnEnvSetUlnVersion(ulnEnv *aEnv, acp_uint64_t aVersion)
{
    aEnv->mUlnVersion = aVersion;

    return ACI_SUCCESS;
}

acp_uint64_t  ulnEnvGetUlnVersion(ulnEnv *aEnv)
{
    return aEnv->mUlnVersion;
}

