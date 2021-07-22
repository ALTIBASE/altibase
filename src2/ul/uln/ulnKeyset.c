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
#include <ulnCursor.h>
#include <ulnKeyset.h>

/*
 * ================================================================
 *
 * Init/Destroy Functions
 *
 * ================================================================
 */

ACI_RC ulnKeysetCreate(ulnKeyset **aKeyset)
{
    acp_uint32_t    sStage;
    ulnKeyset      *sKeyset;

    sStage = 0;

    ACI_TEST( acpMemAlloc( (void**)&sKeyset, ACI_SIZEOF(ulnKeyset) ) != ACP_RC_SUCCESS );
    sStage = 1;

    sKeyset->mKeyCount              = 0;
    sKeyset->mIsFullFilled          = ACP_FALSE;
    sKeyset->mSeqKeyset.mCount      = 0;
    sKeyset->mSeqKeyset.mUsedCount  = 0;
    sKeyset->mSeqKeyset.mBlock      = NULL;

    sKeyset->mParentStmt            = NULL;

    ACI_TEST( acpMemAlloc( (void**)&sKeyset->mChunk,
                           ACI_SIZEOF(acl_mem_area_t) ) != ACP_RC_SUCCESS );

    aclMemAreaCreate(sKeyset->mChunk, 0);

    sStage = 2;

    ACI_TEST( ulnKeysetMarkChunk( sKeyset ) != ACI_SUCCESS );

    /* ulnKeyset를 만들 때, 사용자가 가져간 locator를 기억하기 위한 Hash 생성 */
    ACI_TEST( aclHashCreate( &sKeyset->mHashKeyset,
                             ULN_KEYSET_HASH_BUCKET_COUNT,
                             ULN_KEYSET_MAX_KEY_SIZE,
                             aclHashHashInt64,
                             aclHashCompInt64,
                             ACP_FALSE ) != ACP_RC_SUCCESS );

    *aKeyset = sKeyset;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    /* >> BUG-40316 */
    switch( sStage )
    {
        case 2:
            aclMemAreaDestroy( sKeyset->mChunk );
            acpMemFree( sKeyset->mChunk );
        case 1:
            acpMemFree( sKeyset );
        default:
            break;
    }
    /* << BUG-40316 */

    return ACI_FAILURE;
}

void ulnKeysetFreeAllBlocks(ulnKeyset *aKeyset)
{
    /*
     * RowBlock 을 모조리 free 하므로 캐쉬된 row 도 모조리 사라짐.
     */
    aKeyset->mKeyCount              = 0;
    aKeyset->mIsFullFilled          = ACP_FALSE;
    aKeyset->mSeqKeyset.mUsedCount  = 0;
}

void ulnKeysetDestroy(ulnKeyset *aKeyset)
{
    /*
     * Keyset Chunk 해제
     */
    if( aKeyset->mChunk != NULL )
    {
        aclMemAreaFreeAll(aKeyset->mChunk);
        aclMemAreaDestroy(aKeyset->mChunk);
        acpMemFree( aKeyset->mChunk );
    }

    /*
     * RowBlock 해제
     */
    ulnKeysetFreeAllBlocks(aKeyset);

    /*
     * RowBlockArray 해제
     */
    if (aKeyset->mSeqKeyset.mBlock != NULL)
    {
        acpMemFree(aKeyset->mSeqKeyset.mBlock);
    }

    /* BUG-32474: HashKeyset 해제 */
    aclHashDestroy(&aKeyset->mHashKeyset);

    /*
     * Keyset 해제
     */
    acpMemFree(aKeyset);
}

ACI_RC ulnKeysetInitialize(ulnKeyset *aKeyset)
{
    aKeyset->mKeyCount              = 0;
    aKeyset->mIsFullFilled          = ACP_FALSE;
    aKeyset->mSeqKeyset.mUsedCount  = 0;

    ACI_TEST( ulnKeysetBackChunkToMark( aKeyset ) != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t ulnKeysetGetKeyCount(ulnKeyset *aKeyset)
{
    return aKeyset->mKeyCount;
}

/**
 * Keyset을 모두 채웠는지 여부를 얻는다.
 *
 * @param[in] aKeyset keyset
 *
 * @return Keyset이 모두 찼으면 ACP_TRUE, 아니면 ACP_FALSE
 */
acp_bool_t ulnKeysetIsFullFilled(ulnKeyset *aKeyset)
{
    return aKeyset->mIsFullFilled;
}

/**
 * Keyset을 모두 채웠는지 여부를 설정한다.
 *
 * @param[in] aKeyset       keyset
 * @param[in] aIsFullFilled Keyset을 모두 채웠는지 여부
 */
void ulnKeysetSetFullFilled(ulnKeyset *aKeyset, acp_bool_t aIsFullFilled)
{
    aKeyset->mIsFullFilled = aIsFullFilled;
}

/*
 * ================================================================
 *
 * Chunk Functions
 *
 * ================================================================
 */

/**
 * 새 Chunk 메모리를 할당한다.
 *
 * @param[in]  aKeyset keyset
 * @param[in]  aSize   alloc size
 * @param[out] aData   buffer pointer
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnKeysetAllocChunk( ulnKeyset     *aKeyset,
                            acp_uint32_t   aSize,
                            acp_uint8_t  **aData )
{
    ACI_TEST( aclMemAreaAlloc( aKeyset->mChunk, (void**)aData, aSize ) != ACP_RC_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Chunk 정리를 위한 초기화 수행
 *
 * @param[in] aKeyset keyset
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnKeysetMarkChunk( ulnKeyset  *aKeyset )
{
    aclMemAreaGetSnapshot(aKeyset->mChunk, &aKeyset->mChunkStatus);

    return ACI_SUCCESS;
}

/**
 * Chunk를 재사용하기위해 정리한다.
 *
 * @param[in] aKeyset keyset
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnKeysetBackChunkToMark( ulnKeyset  *aKeyset )
{
    aclMemAreaFreeToSnapshot(aKeyset->mChunk, &aKeyset->mChunkStatus);

    return ACI_SUCCESS;
}

/*
 * ================================================================
 *
 * Keyset Build Functions
 *
 * ================================================================
 */

/**
 * Key가 Keyset에 있는지 확인한다.
 *
 * @param[in] aKeyset   keyset
 * @param[in] aPosition position
 *
 * @return Key가 있으면 ULN_KEYSET_KEY_EXISTS,
 *         Key가 없으면 ULN_KEYSET_KEY_NOT_EXISTS,
 *         Position이 유효하지 않으면 ULN_KEYSET_INVALID_POSITION
 */
acp_sint32_t ulnKeysetIsKeyExists(ulnKeyset    *aKeyset,
                                  acp_sint64_t  aPosition)
{
    acp_sint32_t sExistsRC;

    if (aPosition <= 0)
    {
        /* Position은 1부터 시작 */
        sExistsRC = ULN_KEYSET_INVALID_POSITION;
    }
    else if (aPosition > ulnKeysetGetKeyCount(aKeyset))
    {
        if (ulnKeysetIsFullFilled(aKeyset) == ACP_TRUE)
        {
            /* ResultSet의 범위를 벗어난 Position */
            sExistsRC = ULN_KEYSET_INVALID_POSITION;
        }
        else
        {
            sExistsRC = ULN_KEYSET_KEY_NOT_EXISTS;
        }
    }
    else /* if ((0 < aPosition) && (aPosition <= aKeyset->mKeyCount)) */
    {
        sExistsRC = ULN_KEYSET_KEY_EXISTS;
    }

    return sExistsRC;
}

/**
 * KSAB를 확장한다.
 *
 * @param[in] aKeyset keyset
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnKeysetExtendKSAB(ulnKeyset *aKeyset)
{
    acp_uint32_t  sNewBlockSize;
    ulnKSAB      *sKSAB = &aKeyset->mSeqKeyset;

    sNewBlockSize = (sKSAB->mCount * ACI_SIZEOF(ulnKey *)) +
                    (ULN_KEYSET_KSAB_UNIT_COUNT * ACI_SIZEOF(ulnKey *));

    ACI_TEST( acpMemRealloc( (void**)&sKSAB->mBlock, sNewBlockSize ) != ACP_RC_SUCCESS );

    sKSAB->mCount += ULN_KEYSET_KSAB_UNIT_COUNT;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * KSAB를 확장해야 하는지 확인
 *
 * @param[in] aKeyset keyset
 *
 * @return 필요하면 ACP_TRUE, 아니면 ACP_FALSE
 */
#define NEED_EXTEND_KSAB( aKeyset ) (\
    ( (aKeyset)->mSeqKeyset.mUsedCount + 1 > (aKeyset)->mSeqKeyset.mCount ) \
    ? ACP_TRUE : ACP_FALSE \
)

/**
 * 새 _PROWID 값을 Keyset 끝에 추가한다.
 *
 * @param[in] aKeyset   keyset
 * @param[in] aKeyValue _PROWID value
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnKeysetAddNewKSA(ulnKeyset *aKeyset)
{
    ulnKey *sNewKSA;

    if( NEED_EXTEND_KSAB(aKeyset) )
    {
        ACI_TEST( ulnKeysetExtendKSAB(aKeyset) != ACI_SUCCESS );
    }

    ACI_TEST( ulnKeysetAllocChunk( aKeyset,
                                   ULN_KEYSET_MAX_KEY_IN_KSA
                                   * ACI_SIZEOF(ulnKey),
                                   (acp_uint8_t**)&sNewKSA ) != ACI_SUCCESS );

    aKeyset->mSeqKeyset.mBlock[aKeyset->mSeqKeyset.mUsedCount] = sNewKSA;
    aKeyset->mSeqKeyset.mUsedCount++;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * 새 KSA가 필요한지 확인
 *
 * @param[in] aKeyset keyset
 *
 * @return 필요하면 ACP_TRUE, 아니면 ACP_FALSE
 */
#define NEED_MORE_KSA( aKeyset ) (\
    ( (((aKeyset)->mKeyCount) / ULN_KEYSET_MAX_KEY_IN_KSA) >= ((aKeyset)->mSeqKeyset.mUsedCount) ) \
    ? ACP_TRUE : ACP_FALSE \
)

/**
 * 새 _PROWID 값을 Keyset 끝에 추가한다.
 *
 * @param[in] aKeyset   keyset
 * @param[in] aKeyValue _PROWID value
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnKeysetAddNewKey(ulnKeyset *aKeyset, acp_uint8_t *aKeyValue)
{
    ulnKey *sCurKSA;
    ulnKey *sCurKey;
    acp_sint64_t sNewKeySeq = aKeyset->mKeyCount + 1;
    acp_rc_t sRC;

    // for SeqKeyset

    if( NEED_MORE_KSA(aKeyset) )
    {
        ACI_TEST( ulnKeysetAddNewKSA(aKeyset) != ACI_SUCCESS );
    }

    sCurKSA = aKeyset->mSeqKeyset.mBlock[aKeyset->mKeyCount / ULN_KEYSET_MAX_KEY_IN_KSA];
    sCurKey = &sCurKSA[aKeyset->mKeyCount % ULN_KEYSET_MAX_KEY_IN_KSA];

    acpMemCpy(sCurKey->mKeyValue, aKeyValue, ULN_KEYSET_MAX_KEY_SIZE);
    sCurKey->mSeq = sNewKeySeq;

    // for HashKeyset

    sRC = aclHashAdd(&aKeyset->mHashKeyset, sCurKey->mKeyValue, &(sCurKey->mSeq));
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));

    aKeyset->mKeyCount = sNewKeySeq;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ================================================================
 *
 * Keyset Get Functions
 *
 * ================================================================
 */

/**
 * 지정한 순번의 _PROWID 값을 얻는다.
 *
 * @param[in] aKeyset keyset
 * @param[in] aSeq    seq number. 1부터 시작.
 *
 * @return 지정한 순번의 _PROWID 값. 해당값이 없으면 NULL
 */
acp_uint8_t * ulnKeysetGetKeyValue(ulnKeyset *aKeyset, acp_sint64_t aSeq)
{
    ulnKey *sKSA;
    ulnKey *sKey;

    ACI_TEST((aSeq <= 0) || (aSeq > aKeyset->mKeyCount));
    aSeq = aSeq - 1;

    sKSA = aKeyset->mSeqKeyset.mBlock[aSeq / ULN_KEYSET_MAX_KEY_IN_KSA];
    sKey = &sKSA[aSeq % ULN_KEYSET_MAX_KEY_IN_KSA];

    return sKey->mKeyValue;

    ACI_EXCEPTION_END;

    return NULL;
}

/**
 * _PROWID의 Seq를 얻는다.
 *
 * @param[in] aKeyset   keyset
 * @param[in] aKeyValue _PROWID value
 *
 * @return 순번(1부터 시작). 없으면 0.
 */
acp_sint64_t ulnKeysetGetSeq(ulnKeyset *aKeyset, acp_uint8_t *aKeyValue)
{
    ulnKey *sKey;

    // 방어 코드
    ACE_ASSERT(aKeyset != NULL);
    ACE_ASSERT(aKeyValue != NULL);

    ACI_TEST( aclHashFind( &aKeyset->mHashKeyset,
                           aKeyValue,
                           (void **)&sKey) != ACP_RC_SUCCESS );
    ACE_ASSERT(sKey != NULL);

    return sKey->mSeq;

    ACI_EXCEPTION_END;

    return 0;
}
