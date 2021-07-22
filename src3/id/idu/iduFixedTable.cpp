/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFixedTable.cpp 74637 2016-03-07 06:01:43Z donovan.seo $
 **********************************************************************/

#include <idl.h>
#include <iduMemMgr.h>
#include <iduFixedTable.h>

iduFixedTableDesc              iduFixedTable::mTableHead; // Dummy
iduFixedTableDesc             *iduFixedTable::mTableTail = &mTableHead;
UInt                           iduFixedTable::mTableCount = 0;
UInt                           iduFixedTable::mColumnCount = 0;
iduFixedTableAllocRecordBuffer iduFixedTable::mAllocCallback;
iduFixedTableBuildRecord       iduFixedTable::mBuildCallback;
iduFixedTableCheckKeyRange     iduFixedTable::mCheckKeyRangeCallback;

iduFixedTableRegistBroker::iduFixedTableRegistBroker(iduFixedTableDesc *aDesc)
{
    iduFixedTable::registFixedTable(aDesc);
}

void   iduFixedTable::initialize( void )
{
    mTableTail     = &mTableHead;
    mTableCount    = 0;
    mColumnCount   = 0;
    mAllocCallback = (iduFixedTableAllocRecordBuffer)NULL;
    mBuildCallback = (iduFixedTableBuildRecord)NULL;
    mCheckKeyRangeCallback = (iduFixedTableCheckKeyRange)NULL;
}

void   iduFixedTable::finalize( void )
{
    mTableTail     = &mTableHead;
    mTableCount    = 0;
    mColumnCount   = 0;
    mAllocCallback = (iduFixedTableAllocRecordBuffer)NULL;
    mBuildCallback = (iduFixedTableBuildRecord)NULL;
    mCheckKeyRangeCallback = (iduFixedTableCheckKeyRange)NULL;
}

void   iduFixedTable::registFixedTable(iduFixedTableDesc *aDesc)
{
    iduFixedTableColDesc *sColDesc;

    // Table 객체 갯수 증가
    mTableCount++;

    // Column 객체 갯수 연산
    for (sColDesc = aDesc->mColDesc;
         sColDesc->mName != NULL;
         sColDesc++)
    {
        mColumnCount++;
    }

    // single list로 유지한다.

    mTableTail->mNext = aDesc;
    mTableTail        = aDesc;
    aDesc->mNext      =  NULL;
}

void   iduFixedTable::setCallback(iduFixedTableAllocRecordBuffer aAlloc,
                                  iduFixedTableBuildRecord       aBuild,
                                  iduFixedTableCheckKeyRange     aCheckKeyRange )
{
    mAllocCallback = aAlloc;
    mBuildCallback = aBuild;
    mCheckKeyRangeCallback = aCheckKeyRange;
}

//=============================================================================
// iduFixedTable Memory 관리자

IDE_RC iduFixedTableMemory::initialize( iduMemory * aMemory )
{
    mBeginRecord    = NULL;
    mCurrRecord     = NULL;
    mBeforeRecord   = NULL;
    mContext        = NULL;

    /* BUG-42639 Monitoring query
     * x$와 v$를 사용하는 Fixed Table의 Memory는
     * QMX Memory 를 인자로 받아서 처리하도록 한다.
     */
    if ( aMemory == NULL )
    {
        mMemory.init(IDU_MEM_SM_FIXED_TABLE);
        mMemoryPtr         = &mMemory;
        mUseExternalMemory = ID_FALSE;
    }
    else
    {
        mMemoryPtr         = aMemory;
        mUseExternalMemory = ID_TRUE;
    }
    return IDE_SUCCESS;
}

// BUG-41560
IDE_RC iduFixedTableMemory::restartInit()
{
    mBeginRecord  = NULL;
    mCurrRecord   = NULL;
    mBeforeRecord = NULL;
    mContext      = NULL;

    return IDE_SUCCESS;
}

IDE_RC  iduFixedTableMemory::destroy()
{
    mMemoryPtr->destroy();
    return IDE_SUCCESS;
}

IDE_RC iduFixedTableMemory::allocateRecord(UInt aSize, void **aMem)
{
    IDU_FIXED_TABLE_LOGGING_POINTER();

    IDE_ASSERT(mMemoryPtr->getStatus(&mMemStatus) == IDE_SUCCESS);

    IDE_TEST(mMemoryPtr->alloc(aSize, aMem) != IDE_SUCCESS);

    if (mBeginRecord == NULL)
    {
        mBeginRecord = (UChar *)*aMem;
    }
    mBeforeRecord = mCurrRecord;
    mCurrRecord   = (UChar *)*aMem;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduFixedTableMemory::initRecord(void **aMem)
{
    IDU_FIXED_TABLE_LOGGING_POINTER();

    if (mBeginRecord == NULL)
    {
        mBeginRecord = (UChar *)*aMem;
    }
    mBeforeRecord = mCurrRecord;
    mCurrRecord   = (UChar *)*aMem;

    return IDE_SUCCESS;
}

void   iduFixedTableMemory::freeRecord()
{
    IDU_FIXED_TABLE_ROLLBACK_POINTER();
    IDE_ASSERT(mMemoryPtr->setStatus(&mMemStatus) == IDE_SUCCESS);
}

void   iduFixedTableMemory::resetRecord()
{
    IDU_FIXED_TABLE_ROLLBACK_POINTER();
}

//=============================================================================

IDE_RC iduFixedTable::buildRecordForSelfTable( idvSQL      * /* aStatistics */,
                                               void        * aHeader,
                                               void        * /* aDumpObj */,
                                               iduFixedTableMemory   *aMemory)
{
    UInt               i;
    iduFixedTableDesc *sDesc;
    UInt               sNeedRecCount;

    // [1] Record Count Decided
    sNeedRecCount = mTableCount;

    for (sDesc = iduFixedTable::getTableDescHeader(), i = 0;
         sDesc != NULL && i < sNeedRecCount;
         sDesc = sDesc->mNext, i++)
    {
        /* [2] make one fixed table record */
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)sDesc)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


iduFixedTableColDesc gTableListColDesc[] =
{
    {
        (SChar *)"NAME",
        offsetof(iduFixedTableDesc, mTableName),
        39,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SLOTSIZE",
        offsetof(iduFixedTableDesc, mSlotSize),
        IDU_FT_SIZEOF(iduFixedTableDesc, mSlotSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"COLUMNCOUNT",
        offsetof(iduFixedTableDesc, mColCount),
        IDU_FT_SIZEOF(iduFixedTableDesc, mColCount),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
};


iduFixedTableDesc gTableListTable =
{
    (SChar *)"X$TABLE",
    iduFixedTable::buildRecordForSelfTable,
    gTableListColDesc,
    IDU_STARTUP_INIT,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


//=============================================================================


IDE_RC iduFixedTable::buildRecordForSelfColumn( idvSQL      * /*aStatistics*/,
                                                void        * aHeader,
                                                void        * /* aDumpObj */,
                                                iduFixedTableMemory *aMemory)
{
    UInt                  i;
    UInt                  sNeedRecCount;
    iduFixedTableDesc    *sDesc;
    iduFixedTableColDesc *sColDesc;

    // [1] Record Count Decided
    sNeedRecCount = mColumnCount;

    for (sDesc = iduFixedTable::getTableDescHeader(), i = 0;
         sDesc != NULL && i < sNeedRecCount;
         sDesc = sDesc->mNext)
    {

        for (sColDesc = sDesc->mColDesc;
             sColDesc->mName != NULL &&  i < sNeedRecCount;
             sColDesc++, i++)
        {
            /* [2] make one fixed table record */
            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void *)sColDesc)
                     != IDE_SUCCESS);
        }
    }


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableColDesc gColumnListColDesc[] =
{
    {
        (SChar *)"TABLENAME",
        offsetof(iduFixedTableColDesc, mTableName),
        39,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"COLNAME",
        offsetof(iduFixedTableColDesc, mName),
        39,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"OFFSET",
        offsetof(iduFixedTableColDesc, mColOffset),
        IDU_FT_SIZEOF(iduFixedTableColDesc, mColOffset),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"LENGTH",
        offsetof(iduFixedTableColDesc, mLength),
        IDU_FT_SIZEOF(iduFixedTableColDesc, mLength),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TYPE",
        offsetof(iduFixedTableColDesc, mDataType),
        IDU_FT_SIZEOF(iduFixedTableColDesc, mDataType),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
};


iduFixedTableDesc gColumnListTable =
{
    (SChar *)"X$COLUMN",
    iduFixedTable::buildRecordForSelfColumn,
    gColumnListColDesc,
    IDU_STARTUP_INIT,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
