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
#include <ulnLobCache.h>
#include <ulnConv.h>

#define HASH_PRIME_NUM        (31)
#define MEM_AREA_MULTIPLE     (64)

/**
 *  ulnLobCache
 */
struct ulnLobCache
{
    acl_hash_table_t        mHash;

    acl_mem_area_t          mMemArea;
    acl_mem_area_snapshot_t mMemAreaSnapShot;

    ulnLobCacheErr          mLobCacheErrNo;
};

/**
 *  ulnLobCacheNode
 *
 *  LOB DATA는 ulnCache에 저장되어 있다.
 */
typedef struct ulnLobCacheNode {
    acp_uint64_t  mLocatorID;
    acp_uint8_t  *mData;
    acp_uint32_t  mLength;
    acp_bool_t    mIsNull; // PROJ-2728
} ulnLobCacheNode;

/**
 *  ulnLobCacheCreate
 */
ACI_RC ulnLobCacheCreate(ulnLobCache **aLobCache)
{
    acp_rc_t sRC;
    ulnLobCache *sLobCache = NULL;

    ULN_FLAG(sNeedFreeLobCache);
    ULN_FLAG(sNeedDestroyHash);
    ULN_FLAG(sNeedDestroyMemArea);

    ACI_TEST(aLobCache == NULL);

    sRC = acpMemAlloc((void**)&sLobCache, ACI_SIZEOF(ulnLobCache));
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));
    ULN_FLAG_UP(sNeedFreeLobCache);

    /* Hash 생성 */
    sRC = aclHashCreate(&sLobCache->mHash, HASH_PRIME_NUM,
                        ACI_SIZEOF(acp_uint64_t),
                        aclHashHashInt64, aclHashCompInt64, ACP_FALSE);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));
    ULN_FLAG_UP(sNeedDestroyHash);

    /*
     * aclMemAreaCreate()에서는 실제 Chunk의 사이즈만 설정하고
     * 실제 할당은 aclMemAreaAlloc()에서 한다.
     */
    aclMemAreaCreate(&sLobCache->mMemArea,
                     ACI_SIZEOF(ulnLobCacheNode) * MEM_AREA_MULTIPLE);
    aclMemAreaGetSnapshot(&sLobCache->mMemArea,
                          &sLobCache->mMemAreaSnapShot);
    ULN_FLAG_UP(sNeedDestroyMemArea);

    sLobCache->mLobCacheErrNo = LOB_CACHE_NO_ERR;

    *aLobCache = sLobCache;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedDestroyMemArea)
    {
        aclMemAreaDestroy(&sLobCache->mMemArea);
    }
    ULN_IS_FLAG_UP(sNeedDestroyHash)
    {
        aclHashDestroy(&sLobCache->mHash);
    }
    ULN_IS_FLAG_UP(sNeedFreeLobCache)
    {
        acpMemFree(sLobCache);
        sLobCache = NULL;
    }

    *aLobCache = NULL;

    return ACI_FAILURE;
}

/**
 *  ulnLobCacheReInitialize
 */
ACI_RC ulnLobCacheReInitialize(ulnLobCache *aLobCache)
{
    acp_rc_t sRC;

    ACI_TEST_RAISE(aLobCache == NULL, NO_NEED_WORK);

    /* Hash를 재생성 하자 */
    aclHashDestroy(&aLobCache->mHash);

    sRC = aclHashCreate(&aLobCache->mHash, HASH_PRIME_NUM,
                        ACI_SIZEOF(acp_uint64_t),
                        aclHashHashInt64, aclHashCompInt64, ACP_FALSE);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));

    /* MemArea를 처음 위치로 돌리자 */
    aclMemAreaFreeToSnapshot(&aLobCache->mMemArea,
                             &aLobCache->mMemAreaSnapShot);

    aLobCache->mLobCacheErrNo = LOB_CACHE_NO_ERR;

    ACI_EXCEPTION_CONT(NO_NEED_WORK);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 *  ulnLobCacheDestroy
 */
void ulnLobCacheDestroy(ulnLobCache **aLobCache)
{
    ACI_TEST(aLobCache == NULL);
    ACI_TEST(*aLobCache == NULL);

    /* Destroy MemArea */
    aclMemAreaFreeAll(&(*aLobCache)->mMemArea);
    aclMemAreaDestroy(&(*aLobCache)->mMemArea);

    /* Destroy Hash */
    aclHashDestroy(&(*aLobCache)->mHash);

    /* Free LobCache */
    acpMemFree(*aLobCache);
    *aLobCache = NULL;

    ACI_EXCEPTION_END;

    return;
}

ulnLobCacheErr ulnLobCacheGetErr(ulnLobCache  *aLobCache)
{
    /* BUG-36966 */
    ulnLobCacheErr sLobCacheErr = LOB_CACHE_NO_ERR;

    if (aLobCache != NULL)
    {
        sLobCacheErr = aLobCache->mLobCacheErrNo;
    }
    else
    {
        /* Nothing */
    }

    return sLobCacheErr;
}

/**
 *  ulnLobCacheAdd
 */
ACI_RC ulnLobCacheAdd(ulnLobCache  *aLobCache,
                      acp_uint64_t  aLocatorID,
                      acp_uint8_t  *aValue,
                      acp_uint32_t  aLength,
                      acp_bool_t    aIsNull)
{
    acp_rc_t sRC;

    ulnLobCacheNode *sNewNode = NULL;
    ulnLobCacheNode *sGetNode = NULL;

    ACI_TEST_RAISE(aLobCache == NULL, NO_NEED_WORK);

    /* BUG-36966 aValue가 NULL이면 길이만 저장된다. */
    sRC = aclMemAreaAlloc(&aLobCache->mMemArea,
                          (void **)&sNewNode,
                          ACI_SIZEOF(ulnLobCacheNode));
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));

    sNewNode->mLocatorID = aLocatorID;
    sNewNode->mLength    = aLength;
    sNewNode->mData      = aValue;
    sNewNode->mIsNull    = aIsNull;

    sRC = aclHashAdd(&aLobCache->mHash, &aLocatorID, sNewNode);

    /*
     * 충돌이 일어나면 오버라이트 해 버리자.
     * LOB LOCATOR가 충돌이 일어난다는 건 2^32만큼 순환 했다는건데
     * 이런일이 가능할까~~~
     */
    if (ACP_RC_IS_EEXIST(sRC))
    {
        sRC = aclHashRemove(&aLobCache->mHash, &aLocatorID, (void **)&sGetNode);
        ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));

        sRC = aclHashAdd(&aLobCache->mHash, &aLocatorID, sNewNode);
        ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));
    }
    else
    {
        ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));
    }

    ACI_EXCEPTION_CONT(NO_NEED_WORK);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 *  ulnLobCacheRemove
 */
ACI_RC ulnLobCacheRemove(ulnLobCache  *aLobCache,
                         acp_uint64_t  aLocatorID)
{
    acp_rc_t sRC;
    ulnLobCacheNode *sGetNode = NULL;

    ACI_TEST_RAISE(aLobCache == NULL, NO_NEED_WORK);

    sRC = aclHashRemove(&aLobCache->mHash, &aLocatorID, (void **)&sGetNode);
    /* ENOENT가 발생해도 SUCCESS를 리턴하자 */
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC) && ACP_RC_NOT_ENOENT(sRC));

    ACI_EXCEPTION_CONT(NO_NEED_WORK);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 *  ulnLobCacheGetLob
 */
ACI_RC ulnLobCacheGetLob(ulnLobCache  *aLobCache,
                         acp_uint64_t  aLocatorID,
                         acp_uint32_t  aFromPos,
                         acp_uint32_t  aForLength,
                         ulnFnContext *aContext)
{
    acp_rc_t sRC;
    ACI_RC   sRCACI;

    ulnLobCacheNode *sGetNode = NULL;
    ulnLobBuffer *sLobBuffer = NULL;

    aLobCache->mLobCacheErrNo = LOB_CACHE_NO_ERR;

    ACI_TEST(aLobCache == NULL);
    ACI_TEST(aContext == NULL);

    sLobBuffer = ((ulnLob *)aContext->mArgs)->mBuffer;
    ACI_TEST(sLobBuffer == NULL);

    sRC = aclHashFind(&aLobCache->mHash, &aLocatorID, (void **)&sGetNode);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));

    /* Data가 없다면 ACI_FAILURE */
    ACI_TEST(sGetNode->mData == NULL);

    /* LOB Range를 넘어가면 ACI_FAILURE */
    ACI_TEST_RAISE((acp_uint64_t)aFromPos + (acp_uint64_t)aForLength >
                   (acp_uint64_t)sGetNode->mLength,
                   LABEL_INVALID_LOB_RANGE);

    /*
     * ulnLobBufferDataInFILE()
     * ulnLobBufferDataInBINARY()
     * ulnLobBufferDataInCHAR()
     * ulnLobBufferDataInWCHAR()
     *
     * LobBuffer의 타입에 따라 위의 함수가 호출되며
     * 모두 1, 2번째 파라메터가 ACP_UNUSED()이다.
     */
    sRCACI = sLobBuffer->mOp->mDataIn(NULL,
                                      0,
                                      aForLength,
                                      sGetNode->mData + aFromPos,
                                      aContext);
    ACI_TEST(sRCACI);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LOB_RANGE)
    {
        aLobCache->mLobCacheErrNo = LOB_CACHE_INVALID_RANGE;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 *  ulnLobCacheGetLobLength
 */
ACI_RC ulnLobCacheGetLobLength(ulnLobCache  *aLobCache,
                               acp_uint64_t  aLocatorID,
                               acp_uint32_t *aLength,
                               acp_bool_t   *aIsNull)
{
    acp_rc_t sRC;

    ulnLobCacheNode *sGetNode = NULL;

    ACI_TEST(aLobCache == NULL);
    ACI_TEST(aLength == NULL);

    sRC = aclHashFind(&aLobCache->mHash, &aLocatorID, (void **)&sGetNode);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));

    *aLength = sGetNode->mLength;
    *aIsNull = sGetNode->mIsNull;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
