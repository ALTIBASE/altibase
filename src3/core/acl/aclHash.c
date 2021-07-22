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
 
/*******************************************************************************
 * $Id: aclHash.c 7014 2009-08-18 01:17:30Z djin $
 ******************************************************************************/

#include <acpAlign.h>
#include <aclHash.h>

#define ACL_HASH_SIZEOF_RECORD ACP_ALIGN8(sizeof(aclHashRecord))


typedef struct aclHashBucket
{
    acp_uint32_t     mRecordCount;    /* record count                       */
    acp_list_t       mRecordList;     /* record list                        */
} aclHashBucket;

typedef struct aclHashRecord
{
    acp_list_node_t  mRecordListNode; /* node for record list               */
    void            *mValue;          /* pointer to the value of the record */
                                      /* and, followed by the key data      */
} aclHashRecord;


ACP_INLINE acp_rc_t aclHashFindRecord(acl_hash_table_t  *aHashTable,
                                      aclHashBucket     *aBucket,
                                      void              *aKey,
                                      aclHashRecord    **aRecord)
{
    aclHashRecord   *sRecord = NULL;
    void            *sRecordKey = NULL;
    acp_list_node_t *sIterator = NULL;

    ACP_LIST_ITERATE(&aBucket->mRecordList, sIterator)
    {
        sRecord    = sIterator->mObj;
        sRecordKey = (void *)((acp_char_t *)sRecord + ACL_HASH_SIZEOF_RECORD);

        if (0 ==
            (*aHashTable->mCompFunc)(
                sRecordKey, aKey, aHashTable->mKeyLength
                )
           )
        {
            *aRecord = sRecord;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* do nothing */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_INLINE acp_rc_t aclHashFindInternal(acl_hash_table_t  *aHashTable,
                                        aclHashBucket     *aBucket,
                                        void              *aKey,
                                        void             **aValue)
{
    aclHashRecord *sRecord = NULL;
    acp_rc_t       sRC;

    sRC = aclHashFindRecord(aHashTable, aBucket, aKey, &sRecord);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        *aValue = sRecord->mValue;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

ACP_INLINE acp_rc_t aclHashAddInternal(acl_hash_table_t *aHashTable,
                                       aclHashBucket    *aBucket,
                                       void             *aKey,
                                       void             *aValue)
{
    aclHashRecord *sRecord = NULL;
    acp_rc_t       sRC;

    sRC = aclHashFindRecord(aHashTable, aBucket, aKey, &sRecord);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        return ACP_RC_EEXIST;
    }
    else
    {
        /* do nothing */
    }

    sRC = aclMemPoolAlloc(&aHashTable->mRecordPool, (void **)&sRecord);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sRecord->mValue = aValue;

    acpMemCpy((acp_char_t *)sRecord + ACL_HASH_SIZEOF_RECORD,
              aKey,
              aHashTable->mKeyLength);

    acpListInitObj(&sRecord->mRecordListNode, sRecord);

    acpListAppendNode(&aBucket->mRecordList, &sRecord->mRecordListNode);

    aBucket->mRecordCount++;

    (void)acpAtomicInc32(&(aHashTable->mTotalRecord));

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t aclHashRemoveInternal(acl_hash_table_t  *aHashTable,
                                          aclHashBucket     *aBucket,
                                          void              *aKey,
                                          void             **aValue)
{
    aclHashRecord *sRecord = NULL;
    acp_rc_t       sRC;

    sRC = aclHashFindRecord(aHashTable, aBucket, aKey, &sRecord);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    *aValue = sRecord->mValue;

    aBucket->mRecordCount--;

    (void)acpAtomicDec32(&(aHashTable->mTotalRecord));

    acpListDeleteNode(&sRecord->mRecordListNode);

    aclMemPoolFree(&aHashTable->mRecordPool, sRecord);

    return ACP_RC_SUCCESS;
}

ACP_INLINE void aclHashTraverseFindNext(acl_hash_traverse_t *aHashTraverse)
{
    acl_hash_table_t *sHashTable = aHashTraverse->mHashTable;
    acp_list_node_t  *sNode  = aHashTraverse->mNextRecordListNode->mNext;
    acp_uint32_t      sIndex = aHashTraverse->mBucketIndex;

    while (sNode->mObj == NULL)
    {
        sIndex++;

        if (sIndex >= sHashTable->mBucketCount)
        {
            sNode = NULL;
            break;
        }
        else
        {
            sNode = sHashTable->mBucket[sIndex].mRecordList.mNext;
        }
    }

    aHashTraverse->mNextRecordListNode = sNode;
    aHashTraverse->mBucketIndex        = sIndex;
}


/**
 * creates a hash table
 * @param aHashTable pointer to the hash table
 * @param aBucketCount number of buckets (hash table slots)
 * @param aKeyLength length of key data
 * @param aHashFunc hash function
 * @param aCompFunc compare function
 * @param aNeedLock #ACP_TRUE for multithread safe, otherwise #ACP_FALSE
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if @a aBucketCount is zero.
 *
 * it returns #ACP_RC_EINVAL
 * if @a aKeyLength is equal to zero or greater than #ACP_SINT32_MAX.
 */
ACP_EXPORT acp_rc_t aclHashCreate(acl_hash_table_t     *aHashTable,
                                  acp_uint32_t          aBucketCount,
                                  acp_size_t            aKeyLength,
                                  acl_hash_func_t      *aHashFunc,
                                  acl_hash_comp_func_t *aCompFunc,
                                  acp_bool_t            aNeedLock)
{
    acp_rc_t     sRC;
    acp_uint32_t i;

    if ((aKeyLength == 0) || (aKeyLength > ACP_SINT32_MAX))
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    aHashTable->mHashFunc    = aHashFunc;
    aHashTable->mCompFunc    = aCompFunc;
    aHashTable->mKeyLength   = (acp_uint32_t)aKeyLength;
    aHashTable->mBucketCount = aBucketCount;
    aHashTable->mNeedLock    = aNeedLock;
    aHashTable->mTotalRecord = 0;

    sRC = aclMemPoolCreate(&aHashTable->mRecordPool,
                           ACL_HASH_SIZEOF_RECORD + aKeyLength,
                           64,
                           0);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sRC = acpMemAlloc((void **)&aHashTable->mBucket,
                      sizeof(aclHashBucket) * aBucketCount);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        aclMemPoolDestroy(&aHashTable->mRecordPool);

        return sRC;
    }
    else
    {
        /* do nothing */
    }

    if (aNeedLock == ACP_TRUE)
    {
        sRC = acpThrRwlockCreate(&aHashTable->mLock, 0);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            acpMemFree(aHashTable->mBucket);
            aclMemPoolDestroy(&aHashTable->mRecordPool);

            return sRC;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    for (i = 0; i < aBucketCount; i++)
    {
        aHashTable->mBucket[i].mRecordCount = 0;

        acpListInit(&aHashTable->mBucket[i].mRecordList);
    }

    return ACP_RC_SUCCESS;
}

/**
 * destroys a hash table
 * @param aHashTable pointer to the hash table
 */
ACP_EXPORT void aclHashDestroy(acl_hash_table_t *aHashTable)
{
    aclHashRecord   *sRecord = NULL;
    acp_list_node_t *sIterator = NULL;
    acp_list_node_t *sNextNode = NULL;
    acp_uint32_t     i;

    for (i = 0; i < aHashTable->mBucketCount; i++)
    {
        ACP_LIST_ITERATE_SAFE(&aHashTable->mBucket[i].mRecordList,
                              sIterator,
                              sNextNode)
        {
            sRecord = sIterator->mObj;

            acpListDeleteNode(&sRecord->mRecordListNode);

            aclMemPoolFree(&aHashTable->mRecordPool, sRecord);
        }
    }

    if (aHashTable->mNeedLock == ACP_TRUE)
    {
        (void)acpThrRwlockDestroy(&aHashTable->mLock);
    }
    else
    {
        /* do nothing */
    }

    acpMemFree(aHashTable->mBucket);
    aclMemPoolDestroy(&aHashTable->mRecordPool);
}

/**
 * finds a value for key from the hash table
 * @param aHashTable pointer to the hash table
 * @param aKey key data
 * @param aValue pointer to a variable to store value pointer
 * @return result code
 *
 * it returns #ACP_RC_ENOENT if not found
 */
ACP_EXPORT acp_rc_t aclHashFind(acl_hash_table_t  *aHashTable,
                                void              *aKey,
                                void             **aValue)
{
    acp_uint32_t sIndex;
    acp_rc_t     sRC;

    sIndex =
        (*aHashTable->mHashFunc)(aKey, aHashTable->mKeyLength)
        % aHashTable->mBucketCount;

    if (aHashTable->mNeedLock == ACP_TRUE)
    {
        sRC = acpThrRwlockLockRead(&aHashTable->mLock);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = aclHashFindInternal(aHashTable,
                                      &aHashTable->mBucket[sIndex],
                                      aKey,
                                      aValue);

            (void)acpThrRwlockUnlock(&aHashTable->mLock);
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        sRC = aclHashFindInternal(aHashTable,
                                  &aHashTable->mBucket[sIndex],
                                  aKey,
                                  aValue);
    }

    return sRC;
}

/**
 * adds a value for key into the hash table
 * @param aHashTable pointer to the hash table
 * @param aKey key data
 * @param aValue value pointer to add
 * @return result code
 *
 * it retuns #ACP_RC_EEXIST if it already exist
 */
ACP_EXPORT acp_rc_t aclHashAdd(acl_hash_table_t *aHashTable,
                               void             *aKey,
                               void             *aValue)
{
    acp_uint32_t sIndex;
    acp_rc_t     sRC;

    sIndex =
        (*aHashTable->mHashFunc)(aKey, aHashTable->mKeyLength)
        % aHashTable->mBucketCount;

    if (aHashTable->mNeedLock == ACP_TRUE)
    {
        sRC = acpThrRwlockLockWrite(&aHashTable->mLock);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = aclHashAddInternal(aHashTable,
                                     &aHashTable->mBucket[sIndex],
                                     aKey,
                                     aValue);

            (void)acpThrRwlockUnlock(&aHashTable->mLock);
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        sRC = aclHashAddInternal(aHashTable,
                                 &aHashTable->mBucket[sIndex],
                                 aKey,
                                 aValue);
    }

    return sRC;
}

/**
 * removes a value for key from the hash table
 * @param aHashTable pointer to the hash table
 * @param aKey key data
 * @param aValue pointer to a variable to store value pointer
 * @return result code
 */
ACP_EXPORT acp_rc_t aclHashRemove(acl_hash_table_t  *aHashTable,
                                  void              *aKey,
                                  void             **aValue)
{
    acp_uint32_t sIndex;
    acp_rc_t     sRC;

    sIndex =
        (*aHashTable->mHashFunc)(aKey, aHashTable->mKeyLength)
        % aHashTable->mBucketCount;

    if (aHashTable->mNeedLock == ACP_TRUE)
    {
        sRC = acpThrRwlockLockWrite(&aHashTable->mLock);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = aclHashRemoveInternal(aHashTable,
                                        &aHashTable->mBucket[sIndex],
                                        aKey,
                                        aValue);

            (void)acpThrRwlockUnlock(&aHashTable->mLock);
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        sRC = aclHashRemoveInternal(aHashTable,
                                    &aHashTable->mBucket[sIndex],
                                    aKey,
                                    aValue);
    }

    return sRC;
}

ACP_EXPORT void aclHashStat(acl_hash_table_t      *aHashTable,
                            acl_hash_table_stat_t *aStat)
{
    acp_uint32_t i;
    acp_uint32_t sHitDist = 0;
    acp_uint32_t sHitMax  = 0;
    acp_uint32_t sHitMin  = ACP_UINT32_MAX;

    aStat->mRecordCount = acpAtomicGet32(&(aHashTable->mTotalRecord));

    for (i = 0; i < aHashTable->mBucketCount; i++)
    {
        if (aHashTable->mBucket[i].mRecordCount > 0)
        {
            sHitDist++;

            if (aHashTable->mBucket[i].mRecordCount < sHitMin)
            {
                sHitMin = aHashTable->mBucket[i].mRecordCount;
            }
            else
            {
                /* do nothing */
            }

            if (aHashTable->mBucket[i].mRecordCount > sHitMax)
            {
                sHitMax = aHashTable->mBucket[i].mRecordCount;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    if (sHitMin == ACP_UINT32_MAX)
    {
        sHitMin = 0;
    }
    else
    {
        /* do nothing */
    }

    aStat->mDistribution = 100 * sHitDist / aHashTable->mBucketCount;
    aStat->mDeviation    = sHitMax - sHitMin;
}

/**
 * opens a hash table traverse descriptor
 * @param aHashTraverse pointer to the hash table traverse descriptor
 * @param aHashTable pointer to the hash table to traverse
 * @param aIsCut #ACP_TRUE if cutting traverse
 * (traverse each values and remove them), otherwise #ACP_FALSE
 * @return result code
 *
 * it locks if the hash table needs lock to access.
 * so, until the traverse descriptor closed,
 * any other update access to the hash table will be blocked.
 *
 * when @a aIsCut is #ACP_FALSE,
 * other lookup access can be perfomed successfully
 * because it only locks a read lock for hash table.
 */
ACP_EXPORT acp_rc_t aclHashTraverseOpen(acl_hash_traverse_t *aHashTraverse,
                                        acl_hash_table_t    *aHashTable,
                                        acp_bool_t           aIsCut)
{
    acp_rc_t sRC;

    aHashTraverse->mHashTable          = aHashTable;
    aHashTraverse->mNextRecordListNode = &aHashTable->mBucket[0].mRecordList;
    aHashTraverse->mBucketIndex        = 0;
    aHashTraverse->mIsCut              = aIsCut;

    if (aHashTable->mNeedLock == ACP_TRUE)
    {
        if (aIsCut == ACP_TRUE)
        {
            sRC = acpThrRwlockLockWrite(&aHashTable->mLock);
        }
        else
        {
            sRC = acpThrRwlockLockRead(&aHashTable->mLock);
        }

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    aclHashTraverseFindNext(aHashTraverse);

    return ACP_RC_SUCCESS;
}

/**
 * closes a hash table traverse descriptor
 * @param aHashTraverse pointer to the hash table traverse descriptor
 */
ACP_EXPORT void aclHashTraverseClose(acl_hash_traverse_t *aHashTraverse)
{
    /*
     * unlock if needed
     */
    if (aHashTraverse->mHashTable->mNeedLock == ACP_TRUE)
    {
        (void)acpThrRwlockUnlock(&aHashTraverse->mHashTable->mLock);
    }
    else
    {
        /* do nothing */
    }

    /*
     * nullify
     */
    aHashTraverse->mHashTable          = NULL;
    aHashTraverse->mNextRecordListNode = NULL;
}

/**
 * obtains the next value in the hash table traverse descriptor
 * @param aHashTraverse pointer to the hash table traverse descriptor
 * @param aValue pointer to a variable to store value pointer
 * @return result code
 *
 * it returns #ACP_RC_EOF if there is no more value in the hash table.
 *
 * if the @a aHashTraverse is opened as cutting traverse,
 * the returned value will be removed from the hash table
 * before the function returns.
 */
ACP_EXPORT acp_rc_t aclHashTraverseNext(acl_hash_traverse_t  *aHashTraverse,
                                        void                **aValue)
{
    acl_hash_table_t *sHashTable = NULL;
    aclHashBucket    *sBucket = NULL;
    aclHashRecord    *sRecord = NULL;

    /*
     * return EOF if no more record
     */
    if (aHashTraverse->mNextRecordListNode == NULL)
    {
        return ACP_RC_EOF;
    }
    else
    {
        /* do nothing */
    }

    /*
     * get current record
     */
    sHashTable = aHashTraverse->mHashTable;
    sBucket    = &sHashTable->mBucket[aHashTraverse->mBucketIndex];
    sRecord    = aHashTraverse->mNextRecordListNode->mObj;

    /*
     * find next record list node
     */
    aclHashTraverseFindNext(aHashTraverse);

    /*
     * get value from current record
     */
    *aValue = sRecord->mValue;

    /*
     * remove record if cutting traverse
     */
    if (aHashTraverse->mIsCut == ACP_TRUE)
    {
        sBucket->mRecordCount--;

        (void)acpAtomicDec32(&(sHashTable->mTotalRecord));

        acpListDeleteNode(&sRecord->mRecordListNode);

        aclMemPoolFree(&sHashTable->mRecordPool, sRecord);
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}
