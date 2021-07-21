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
 * $Id: $
 ******************************************************************************/

#if !defined(_O_ACL_LFMEMPOOL_H_)
#define _O_ACL_LFMEMPOOL_H_

#include <acp.h>
#include <aclSafeList.h>

#define ACL_LOCKFREE_MEMPOOL_FULLCHUNK      acpByteOrderTON8(0xFFFFFFFFFFFFFFFF)
#define ACL_LOCKFREE_MEMPOOL_ONELSB         acpByteOrderTON8(0x0000000000000001)
#define ACL_LOCKFREE_MEMPOOL_EMPTYCHUNK     acpByteOrderTON8(0x8000000000000000)
#define ACL_LOCKFREE_MEMPOOL_INACTIVECHUNK  ACP_UINT64_LITERAL(0x0)

typedef struct aclLFMemPoolFuncs {
    acp_rc_t (*mChunkAlloc)();
    acp_rc_t (*mChunkFree)();
} aclLFMemPoolFuncs;

typedef enum aclLFMemPoolTypes {
    ACL_LOCKFREE_MEMPOOL_UNINITIALIZED = 0,
    ACL_LOCKFREE_MEMPOOL_HEAP,
    ACL_LOCKFREE_MEMPOOL_SHMEM,
    ACL_LOCKFREE_MEMPOOL_MMAP
} aclLFMemPoolTypes;

typedef struct acl_lockfree_segment_t {
    acp_key_t             mKey;
    acp_shm_t         mShm;
    acp_mmap_t        mMmap;
} acl_lockfree_segment_t;

typedef struct acl_lockfree_mempool_t {
    aclLFMemPoolTypes     mType;
    acp_size_t            mBlockSize;
    acl_safelist_t        mList;
    acl_safelist_t        mSegmentList;
    acp_uint32_t          mEmptyHighLimit;
    acp_uint32_t          mEmptyLowLimit;
    acp_uint32_t          mEmptyCount;
    acp_uint32_t          mSegmentChunks;
    aclLFMemPoolFuncs     mFuncs;
} acl_lockfree_mempool_t;

typedef enum aclLFMemPoolAllocResults {
    ACL_LOCKFREE_MEMPOOL_NONE = 0,
    ACL_LOCKFREE_MEMPOOL_WAS_FULL,
    ACL_LOCKFREE_MEMPOOL_WAS_EMPTY,
    ACL_LOCKFREE_MEMPOOL_FULL,
    ACL_LOCKFREE_MEMPOOL_EMPTY
} aclLFMemPoolAllocResults;

#endif //_O_ACL_LFMEMPOOL_H_
