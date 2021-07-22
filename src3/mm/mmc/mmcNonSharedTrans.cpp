/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <mmcNonSharedTrans.h>
#include <mmcTrans.h>
#include <mmcSession.h>
#include <sdi.h>

iduMemPool mmcNonSharedTrans::mPool;

IDE_RC mmcNonSharedTrans::initialize()
{
    UInt sTransSize;

    sTransSize = ID_SIZEOF(mmcTransObj);

    IDE_TEST( mPool.initialize( IDU_MEM_MMC,
                                (SChar *)"MMC_NON_SHARED_TRANS_POOL",
                                ID_SCALABILITY_SYS,
                                sTransSize,
                                4,
                                IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                ID_TRUE,							/* UseMutex */
                                IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                ID_FALSE,							/* ForcePooling */
                                ID_TRUE,							/* GarbageCollection */
                                ID_TRUE,                            /* HWCacheLine */
                                IDU_MEMPOOL_TYPE_LEGACY             /* mempool type*/) 
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcNonSharedTrans::finalize()
{
    IDE_TEST( mPool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcNonSharedTrans::allocTrans( mmcTransObj ** aTrans )
{
    mmcTransObj * sTransOut = NULL;

    IDE_TEST( mPool.alloc( (void **)&sTransOut ) != IDE_SUCCESS);

    initTransInfo( sTransOut );

    *aTrans = sTransOut;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcNonSharedTrans::freeTrans( mmcTransObj ** aTrans )
{
    finiTransInfo( *aTrans );

    IDE_TEST( mPool.memfree( *aTrans )
              != IDE_SUCCESS );

    *aTrans = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mmcNonSharedTrans::initTransInfo( mmcTransObj * aTransObj )
{
    aTransObj->mShareInfo = NULL;
}

void mmcNonSharedTrans::finiTransInfo( mmcTransObj * aTransObj )
{
    aTransObj->mShareInfo = NULL;
}


