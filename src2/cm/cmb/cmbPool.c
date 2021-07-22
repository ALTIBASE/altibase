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

#include <cmAllClient.h>


typedef struct cmbPoolAllocInfo
{
    ACI_RC     (*mMap)(cmbPool *aPool);
    acp_uint32_t (*mSize)();
} cmbPoolAllocInfo;


static cmbPoolAllocInfo gCmbPoolAllocInfoClient[CMB_POOL_IMPL_MAX] =
{
    {
        NULL,
        NULL
    },
    {
        cmbPoolMapLOCAL,
        cmbPoolSizeLOCAL
    },
    {
#if defined(CM_DISABLE_IPC)
        NULL,
        NULL
#else
        cmbPoolMapIPC,
        cmbPoolSizeIPC
#endif
    },
    {
#if defined(CM_DISABLE_IPCDA)
        NULL,
        NULL
#else
        cmbPoolMapIPCDA,
        cmbPoolSizeIPCDA
#endif
    },
};


static cmbPool *gCmbPoolSharedPoolClient[CMB_POOL_IMPL_MAX] =
{
    NULL,
    NULL,
    NULL,
    NULL
};


ACI_RC cmbPoolAlloc(cmbPool **aPool, acp_uint8_t aImpl, acp_uint16_t aBlockSize, acp_uint32_t aBlockCount)
{
    cmbPoolAllocInfo *sAllocInfo;

    ACP_UNUSED(aBlockCount);

    /*
     * 파라미터 범위 검사
     */
    ACE_ASSERT(aImpl < CMB_POOL_IMPL_MAX);

    /*
     * AllocInfo 획득
     */
    sAllocInfo = &gCmbPoolAllocInfoClient[aImpl];

    ACE_ASSERT(sAllocInfo->mMap  != NULL);
    ACE_ASSERT(sAllocInfo->mSize != NULL);

    /*
     * 메모리 할당
     */
    ACI_TEST(acpMemAlloc((void **)aPool, sAllocInfo->mSize()) != ACP_RC_SUCCESS);

    /*
     * 멤버 초기화
     */
    (*aPool)->mBlockSize = aBlockSize;

    /*
     * 함수 포인터 매핑
     */
    ACI_TEST_RAISE(sAllocInfo->mMap(*aPool) != ACI_SUCCESS, InitializeFail);

    /*
     * 초기화
     */
    ACI_TEST_RAISE((*aPool)->mOp->mInitialize(*aPool) != ACI_SUCCESS, InitializeFail);

    return ACI_SUCCESS;

    ACI_EXCEPTION(InitializeFail)
    {
        acpMemFree(*aPool);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbPoolFree(cmbPool *aPool)
{
    /*
     * 정리
     */
    ACI_TEST(aPool->mOp->mFinalize(aPool) != ACI_SUCCESS);

    /*
     * 메모리 해제
     */
    acpMemFree(aPool);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbPoolSetSharedPool(cmbPool *aPool, acp_uint8_t aImpl)
{
    /*
     * 파라미터 범위 검사
     */
    ACE_ASSERT(aImpl > CMB_POOL_IMPL_NONE);
    ACE_ASSERT(aImpl < CMB_POOL_IMPL_MAX);

    gCmbPoolSharedPoolClient[aImpl] = aPool;

    return ACI_SUCCESS;
}

ACI_RC cmbPoolGetSharedPool(cmbPool **aPool, acp_uint8_t aImpl)
{
    /*
     * 파라미터 범위 검사
     */
    ACE_ASSERT(aImpl < CMB_POOL_IMPL_MAX);

    *aPool = gCmbPoolSharedPoolClient[aImpl];

    ACI_TEST_RAISE(*aPool == NULL, SharedPoolNotExist);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SharedPoolNotExist)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SHARED_POOL_NOT_EXIST));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
