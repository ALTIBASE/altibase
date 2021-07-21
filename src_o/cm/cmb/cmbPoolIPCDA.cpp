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

#include <cmAll.h>

#if !defined(CM_DISABLE_IPCDA)

typedef struct cmbPoolIPCDA
{
    cmbPool    mPool;
    iduMemPool mBlockPool;
} cmbPoolIPCDA;

IDE_RC cmbPoolInitializeIPCDA(cmbPool * /*aPool*/)
{
    /* do nothing. */
    return IDE_SUCCESS;
}

IDE_RC cmbPoolFinalizeIPCDA(cmbPool * /*aPool*/)
{
    /* do nothing. */
    return IDE_SUCCESS;
}

IDE_RC cmbPoolAllocBlockIPCDA(cmbPool * /*aPool*/, cmbBlock ** /*aBlock*/)
{
    /* do nothing. */
    return IDE_SUCCESS;
}

IDE_RC cmbPoolFreeBlockIPCDA(cmbPool * /*aPool*/, cmbBlock * /*aBlock*/)
{
    /* do nothing. */
    return IDE_SUCCESS;
}

struct cmbPoolOP gCmbPoolOpIPCDA =
{
    (SChar *)"IPCDA",

    cmbPoolInitializeIPCDA,
    cmbPoolFinalizeIPCDA,

    cmbPoolAllocBlockIPCDA,
    cmbPoolFreeBlockIPCDA
};

IDE_RC cmbPoolMapIPCDA(cmbPool *aPool)
{
    /*
     * 함수 포인터 세팅
     */
    aPool->mOp = &gCmbPoolOpIPCDA;

    return IDE_SUCCESS;
}

UInt cmbPoolSizeIPCDA()
{
    return ID_SIZEOF(cmbPoolIPCDA);
}

#endif
