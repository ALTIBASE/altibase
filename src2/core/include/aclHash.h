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
 * $Id: aclHash.h 7482 2009-09-08 06:20:11Z djin $
 ******************************************************************************/

#if !defined(_O_ACL_HASH_H_)
#define _O_ACL_HASH_H_

/**
 * @file
 * @ingroup CoreCollection
 *
 * chained hash table
 */

#include <acpAtomic.h>
#include <acpThrRwlock.h>
#include <acpList.h>
#include <aclMemPool.h>


ACP_EXTERN_C_BEGIN


/**
 * hash function
 */
typedef acp_uint32_t acl_hash_func_t(const void *aKey, acp_size_t aKeyLength);

/**
 * compare function
 */
typedef acp_sint32_t acl_hash_comp_func_t(
    const void* aKey1,
    const void* aKey2,
    acp_size_t  aKeyLength);

/**
 * hash table
 */
typedef struct acl_hash_table_t
{
    acp_thr_rwlock_t      mLock;         /* lock for multithread safe      */
    acl_mem_pool_t        mRecordPool;   /* memory pool for records        */

    acl_hash_func_t      *mHashFunc;     /* hash function                  */
    acl_hash_comp_func_t *mCompFunc;     /* compare function               */

    acp_uint32_t          mKeyLength;    /* length of key data             */
    acp_uint32_t          mBucketCount;  /* bucket count                   */
    acp_bool_t            mNeedLock;     /* use lock for multithread safe  */
    volatile acp_sint32_t mTotalRecord;  /* total record count             */

    struct aclHashBucket *mBucket;       /* bucket array                   */
} acl_hash_table_t;

/**
 * traverse descriptor for hash table
 */
typedef struct acl_hash_traverse_t
{
    acl_hash_table_t *mHashTable;
    acp_list_node_t  *mNextRecordListNode;
    acp_uint32_t      mBucketIndex;
    acp_bool_t        mIsCut;
} acl_hash_traverse_t;

/*
 * hash table statistics information
 */
typedef struct acl_hash_table_stat_t
{
    acp_uint32_t mRecordCount;  /* all record count                       */
    acp_uint32_t mDistribution; /* bucket distribution rate in percentage */
    acp_uint32_t mDeviation;    /* bucket deviation (max - min)           */
} acl_hash_table_stat_t;

/**
 * get the total record count
 *
 * @param aHashTable pointer to the hash table
 *        to get the total record count from
 *
 * @return count of records in @a aHashTable
 */
ACP_INLINE acp_uint32_t aclHashGetTotalRecordCount(acl_hash_table_t *aHashTable)
{
    return acpAtomicGet32(&(aHashTable->mTotalRecord));
}

/**
 * check hash table is empty or not.
 *
 * @param aHashTable pointer to the hash table to check whether it is empty
 *
 * @return #ACP_TRUE when @a aHashTable is empty, #ACP_FALSE otherwise
 *
 */
ACP_INLINE acp_bool_t aclHashIsEmpty(acl_hash_table_t *aHashTable)
{
    return aclHashGetTotalRecordCount(aHashTable) == 0 ? ACP_TRUE : ACP_FALSE;
}

/*
 * hash table functions
 */
ACP_EXPORT acp_rc_t aclHashCreate(acl_hash_table_t     *aHashTable,
                                  acp_uint32_t          aBucketCount,
                                  acp_size_t            aKeyLength,
                                  acl_hash_func_t      *aHashFunc,
                                  acl_hash_comp_func_t *aCompFunc,
                                  acp_bool_t            aNeedLock);
ACP_EXPORT void     aclHashDestroy(acl_hash_table_t *aHashTable);

ACP_EXPORT acp_rc_t aclHashFind(acl_hash_table_t  *aHashTable,
                                void              *aKey,
                                void             **aValue);

ACP_EXPORT acp_rc_t aclHashAdd(acl_hash_table_t *aHashTable,
                               void             *aKey,
                               void             *aValue);

ACP_EXPORT acp_rc_t aclHashRemove(acl_hash_table_t  *aHashTable,
                                  void              *aKey,
                                  void             **aValue);

ACP_EXPORT void aclHashStat(acl_hash_table_t      *aHashTable,
                            acl_hash_table_stat_t *aStat);

/*
 * hash table traverse functions
 */
ACP_EXPORT acp_rc_t aclHashTraverseOpen(acl_hash_traverse_t *aHashTraverse,
                                        acl_hash_table_t    *aHashTable,
                                        acp_bool_t           aIsCut);
ACP_EXPORT void     aclHashTraverseClose(acl_hash_traverse_t *aHashTraverse);
ACP_EXPORT acp_rc_t aclHashTraverseNext(acl_hash_traverse_t  *aHashTraverse,
                                        void                **aValue);


/* ------------------------------------------------
 *  Hash Functions
 * ----------------------------------------------*/

#define ACL_HASH_MAX_CSTRING_LENGTH           256

/*
 * builtin hash & compare functions
 */
ACP_EXPORT acp_uint32_t aclHashHashInt32(
    const void *aKey,
    acp_size_t aKeyLength);
ACP_EXPORT acp_sint32_t aclHashCompInt32(
    const void *aKey1,
    const void *aKey2,
    acp_size_t aKeyLength);

ACP_EXPORT acp_uint32_t aclHashHashInt64(
    const void *aKey,
    acp_size_t aKeyLength);
ACP_EXPORT acp_sint32_t aclHashCompInt64(
    const void *aKey1,
    const void *aKey2,
    acp_size_t aKeyLength);

ACP_EXPORT acp_uint32_t aclHashHashCString(
    const void *aKey,
    acp_size_t aKeyLength);
ACP_EXPORT acp_sint32_t aclHashCompCString(
    const void *aKey1,
    const void *aKey2,
    acp_size_t aKeyLength);

ACP_EXPORT acp_uint32_t aclHashHashString(
    const void *aKey,
    acp_size_t aKeyLength);
ACP_EXPORT acp_sint32_t aclHashCompString(
    const void *aKey1,
    const void *aKey2,
    acp_size_t aKeyLength);

/*
 * builtin hash support functions
 */
ACP_EXPORT acp_uint32_t aclHashHashBinaryWithLen(const acp_uint8_t *aPtr,
                                                 acp_size_t         aLen);
ACP_EXPORT acp_uint32_t aclHashHashCStringWithLen(const acp_char_t *aPtr,
                                                  acp_size_t        aLen);


ACP_EXTERN_C_END


#endif
