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

#ifndef _O_CMB_POOL_CLIENT_H_
#define _O_CMB_POOL_CLIENT_H_ 1


enum {
    CMB_POOL_IMPL_NONE = 0,
    CMB_POOL_IMPL_LOCAL,
    CMB_POOL_IMPL_IPC,
    CMB_POOL_IMPL_IPCDA,
    CMB_POOL_IMPL_MAX
};


typedef struct cmbPool
{
    acp_uint16_t      mBlockSize;
    struct cmbPoolOP *mOp;
} cmbPool;


struct cmbPoolOP
{
    acp_char_t   *mName;

    ACI_RC (*mInitialize)(cmbPool *aPool);
    ACI_RC (*mFinalize)(cmbPool *aPool);

    ACI_RC (*mAllocBlock)(cmbPool *aPool, cmbBlock **aBlock);
    ACI_RC (*mFreeBlock)(cmbPool *aPool, cmbBlock *aBlock);
};


ACI_RC cmbPoolAlloc(cmbPool **aPool, acp_uint8_t aImpl, acp_uint16_t aBlockSize, acp_uint32_t aBlockCount);
ACI_RC cmbPoolFree(cmbPool *aPool);

ACI_RC cmbPoolSetSharedPool(cmbPool *aPool, acp_uint8_t aImpl);
ACI_RC cmbPoolGetSharedPool(cmbPool **aPool, acp_uint8_t aImpl);


#endif
