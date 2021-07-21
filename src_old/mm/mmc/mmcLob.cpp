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

#include <iduHashUtil.h>
#include <qci.h>
#include <mmcLob.h>


iduMemPool mmcLob::mPool;


IDE_RC mmcLob::initialize()
{
    IDE_TEST(mPool.initialize(IDU_MEM_MMC,
                              (SChar *)"MMC_LOB_LOCATOR_POOL",
                              ID_SCALABILITY_SYS,
                              ID_SIZEOF(mmcLobLocator),
                              16,
                              IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                              ID_TRUE,							/* UseMutex */
                              IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                              ID_FALSE,							/* ForcePooling */
                              ID_TRUE,							/* GarbageCollection */
                              ID_TRUE,                          /* HWCacheLine */
                              IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcLob::finalize()
{
    IDE_TEST(mPool.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcLob::alloc(mmcLobLocator **aLobLocator, UInt aStatementID, ULong aLocatorID)
{
    IDU_FIT_POINT( "mmcLob::alloc::alloc::LobLocator" );

    IDE_TEST(mPool.alloc((void **)aLobLocator) != IDE_SUCCESS);

    (*aLobLocator)->mLocatorID   = aLocatorID;
    (*aLobLocator)->mStatementID = aStatementID;

    IDU_LIST_INIT_OBJ(&(*aLobLocator)->mListNode, *aLobLocator);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcLob::free(idvSQL        *aStatistics,
                    mmcLobLocator *aLobLocator)
{
    mmcLobLocator *sLobLocator;
    iduListNode   *sIterator;
    iduListNode   *sNodeNext;
    IDE_RC         sRet = IDE_SUCCESS;

    IDU_LIST_ITERATE_SAFE(&aLobLocator->mListNode, sIterator, sNodeNext)
    {
        sLobLocator = (mmcLobLocator *)sIterator->mObj;

        if (qciMisc::lobFinalize(aStatistics, sLobLocator->mLocatorID) != IDE_SUCCESS)
        {
            sRet = IDE_FAILURE;
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST(mPool.memfree(sLobLocator) != IDE_SUCCESS);
    }

    if (qciMisc::lobFinalize(aStatistics, aLobLocator->mLocatorID) != IDE_SUCCESS)
    {
        sRet = IDE_FAILURE;
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST(mPool.memfree(aLobLocator) != IDE_SUCCESS);

    return sRet;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcLob::addLocator(mmcLobLocator *aLobLocator, UInt aStatementID, ULong aLocatorID)
{
    mmcLobLocator *sLobLocator;

    IDE_TEST(alloc(&sLobLocator, aStatementID, aLocatorID) != IDE_SUCCESS);

    IDU_LIST_ADD_LAST(&aLobLocator->mListNode, &sLobLocator->mListNode);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
IDE_RC mmcLob::beginWrite(idvSQL        *aStatistics, 
                          mmcLobLocator *aLobLocator, 
                          UInt           aOffset, 
                          UInt           aSize)
{
    mmcLobLocator *sLobLocator;
    iduListNode   *sIterator;

    IDE_TEST(qciMisc::lobPrepare4Write(
                         aStatistics, 
                         aLobLocator->mLocatorID, 
                         aOffset,
                         aSize) != IDE_SUCCESS);

    IDU_LIST_ITERATE(&aLobLocator->mListNode, sIterator)
    {
        sLobLocator = (mmcLobLocator *)sIterator->mObj;

        IDE_TEST(qciMisc::lobPrepare4Write(
                             aStatistics, 
                             sLobLocator->mLocatorID, 
                             aOffset, 
                             aSize) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* PROJ-2047 Strengthening LOB - Removed aOffset */
IDE_RC mmcLob::write(idvSQL        *aStatistics,
                     mmcLobLocator *aLobLocator,
                     UInt           aSize,
                     UChar         *aData)
{
    mmcLobLocator *sLobLocator;
    iduListNode   *sIterator;

    IDE_TEST(qciMisc::lobWrite(aStatistics,
                               aLobLocator->mLocatorID,
                               aSize,
                               aData) != IDE_SUCCESS);

    IDU_LIST_ITERATE(&aLobLocator->mListNode, sIterator)
    {
        sLobLocator = (mmcLobLocator *)sIterator->mObj;

        IDE_TEST(qciMisc::lobWrite(aStatistics,
                                   sLobLocator->mLocatorID,
                                   aSize,
                                   aData) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcLob::endWrite(idvSQL *aStatistics, mmcLobLocator *aLobLocator)
{
    mmcLobLocator *sLobLocator;
    iduListNode   *sIterator;

    IDE_TEST(qciMisc::lobFinishWrite(aStatistics,
                                     aLobLocator->mLocatorID) != IDE_SUCCESS);

    IDU_LIST_ITERATE(&aLobLocator->mListNode, sIterator)
    {
        sLobLocator = (mmcLobLocator *)sIterator->mObj;

        IDE_TEST(qciMisc::lobFinishWrite(aStatistics,
                                         sLobLocator->mLocatorID) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

UInt mmcLob::hashFunc(void *aKey)
{
    UInt sHigh = *(ULong *)aKey >> 32;
    UInt sLow  = *(ULong *)aKey;

    return iduHashUtil::hashUInt(sHigh) + iduHashUtil::hashUInt(sLow);
}

SInt mmcLob::compFunc(void *aLhs, void *aRhs)
{
    if (*(ULong *)aLhs > *(ULong *)aRhs)
    {
        return 1;
    }
    else if (*(ULong *)aLhs < *(ULong *)aRhs) // fix BUG-21500 (UInt*) -> (ULong*)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
