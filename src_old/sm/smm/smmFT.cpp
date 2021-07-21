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
 

/***********************************************************************
* $Id: smmFT.cpp 37235 2009-12-09 01:56:06Z cgkim $
**********************************************************************/
#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smiFixedTable.h>
#include <sctTableSpaceMgr.h>
#include <smm.h>
#include <smmFT.h>
#include <smErrorCode.h>
#include <smc.h>
#include <smDef.h>
#include <smu.h>
#include <smuUtility.h>
#include <smmReq.h>
#include <smmDef.h>
#include <smmFPLManager.h>
#include <smmExpandChunk.h>


/* ------------------------------------------------
 *  Fixed Table Define for MemBase
 * ----------------------------------------------*/

static iduFixedTableColDesc gMemBaseTableColDesc[] =
{
    {
        (SChar*)"SPACE_ID",
        offsetof(smmMemBaseFT, mSpaceID),
        IDU_FT_SIZEOF(smmMemBaseFT, mSpaceID),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"DB_NAME",
        offsetof(smmMemBaseFT, mMemBase.mDBname),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mDBname),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"PRODUCT_SIGNATURE",
        offsetof(smmMemBaseFT, mMemBase.mProductSignature),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mProductSignature),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"DB_SIGNATURE",
        offsetof(smmMemBaseFT, mMemBase.mDBFileSignature),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mDBFileSignature),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"VERSION_ID",
        offsetof(smmMemBaseFT, mMemBase.mVersionID),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mVersionID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"COMPILE_BIT",
        offsetof(smmMemBaseFT, mMemBase.mCompileBit),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mCompileBit),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"ENDIAN",
        offsetof(smmMemBaseFT, mMemBase.mBigEndian),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mBigEndian),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"LOGFILE_SIZE",
        offsetof(smmMemBaseFT, mMemBase.mLogSize),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mLogSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"MEM_DBFILE_PAGE_COUNT",
        offsetof(smmMemBaseFT, mMemBase.mDBFilePageCount),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mDBFilePageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"TX_TBL_SIZE",
        offsetof(smmMemBaseFT, mMemBase.mTxTBLSize),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mTxTBLSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"MEM_DBFILE_COUNT_0",
        offsetof(smmMemBaseFT, mMemBase.mDBFileCount),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"MEM_DBFILE_COUNT_1",
        offsetof(smmMemBaseFT, mMemBase.mDBFileCount) + ID_SIZEOF(UInt),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"MEM_TIMESTAMP",
        offsetof(smmMemBaseFT, mMemBase.mTimestamp),
        64,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertTIMESTAMP,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"MEM_ALLOC_PAGE_COUNT",
        offsetof(smmMemBaseFT, mMemBase.mAllocPersPageCount),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mAllocPersPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"DURABLE_SYSTEM_SCN",
        offsetof(smmMemBaseFT, mMemBase.mSystemSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"NLS_CHARACTERSET",
        offsetof(smmMemBaseFT, mMemBase.mDBCharSet),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mDBCharSet),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"NLS_NCHAR_CHARACTERSET",
        offsetof(smmMemBaseFT, mMemBase.mNationalCharSet),
        IDU_FT_SIZEOF(smmMemBaseFT, mMemBase.mNationalCharSet),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


static IDE_RC buildRecordForMemBase(idvSQL              * /*aStatistics*/,
                                    void        *aHeader,
                                    void        * /* aDumpObj */,
                                    iduFixedTableMemory *aMemory)
{
    smmTBSNode * sCurTBS;
    smmMemBase * sMemBase;
    void       * sIndexValues[1];
    smmMemBaseFT sMemBaseFT;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurTBS != NULL )
    {
        /* BUG-47948 tablespace가 drop되면 membase 가 null이 될 수 있습니다.
         * membase page는 pool 에 반납 되므로 memory 공간이 free 되는 것은 아닙니다.
         * memmbase가 nul인지 확인을 해도 사용하려는 타이밍에 null로 변할 수 있으니
         * pointer를 미리 받아서 null인지 확인 한 후에 사용합니다.
         * 상황에 따라서 pool 에서 page가 Free 될 수도 있습니다만,
         * drop tablespace가 자주 발생하는 건이 아니므로만 이정도로 방어 합니다.*/
        sMemBase = sCurTBS->mMemBase;

        if (( sctTableSpaceMgr::isMemTableSpace( sCurTBS->mHeader.mID ) == ID_TRUE ) &&
            ( sCurTBS->mRestoreType != SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET ) &&
            ( sMemBase != NULL ) &&
            ( SMI_TBS_IS_COMPLETE( sCurTBS->mHeader.mState ) ))
        {
            /* BUG-43006 FixedTable Indexing Filter
             * Indexing Filter를 사용해서 전체 Record를 생성하지않고
             * 부분만 생성해 Filtering 한다.
             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
             * 해당하는 값을 순서대로 넣어주어야 한다.
             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
             * 어 주어야한다.
             */
            sIndexValues[0] = &sCurTBS->mHeader.mID;
            if ( iduFixedTable::checkKeyRange( aMemory,
                                               gMemBaseTableColDesc,
                                               sIndexValues )
                 == ID_FALSE )
            {
                sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
                continue;
            }
            else
            {
                /* Nothing to do */
            }

            sMemBaseFT.mSpaceID = sCurTBS->mHeader.mID;
            idlOS::memcpy( &sMemBaseFT.mMemBase,
                           sMemBase,
                           ID_SIZEOF( sMemBaseFT.mMemBase ) );

            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void *) & sMemBaseFT )
                     != IDE_SUCCESS);
        }

        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gMemBaseTableDesc =
{
    (SChar *)"X$MEMBASE",
    buildRecordForMemBase,
    gMemBaseTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc gMemBaseMgrTableColDesc[] =
{
    {
        (SChar*)"SPACE_ID",
        offsetof(smmTBSStatistics, mSpaceID),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RESTORE_TYPE",
        offsetof(smmTBSStatistics, mRestoreType),
        ID_SIZEOF(smmDBRestoreType),
        IDU_FT_TYPE_UBIGINT | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"CURRENT_DB",
        offsetof(smmTBSStatistics, m_currentDB),
        IDU_FT_SIZEOF_INTEGER,
        IDU_FT_TYPE_INTEGER | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"LAST_SYSTEM_SCN",
        offsetof(smmTBSStatistics, m_lstSystemSCN),
        29,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"HIGH_LIMIT_PAGE",
        offsetof(smmTBSStatistics, mDBMaxPageCount),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"PAGE_COUNT_PER_FILE",
        offsetof(smmTBSStatistics, mPageCountPerFile),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"PAGE_COUNT_IN_DISK",
        offsetof(smmTBSStatistics, m_nDBPageCountInDisk),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"MAX_ACCESS_FILE_SIZE",
        offsetof(smmTBSStatistics,mMaxAccessFileSize),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"PAGE_SIZE",
        offsetof(smmTBSStatistics, mPageSize),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


static IDE_RC buildRecordForMemBaseMgr(idvSQL              * /*aStatistics*/,
                                       void        *aHeader,
                                       void        * /* aDumpObj */,
                                       iduFixedTableMemory *aMemory)
{
    smmTBSNode *     sCurTBS;
    smmMemBase *     sMemBase;
    smmTBSStatistics sStat;
    void           * sIndexValues[1];

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurTBS != NULL )
    {
        /* BUG-47948 tablespace가 drop되면 membase 가 null이 될 수 있습니다.
         * membase page는 pool 에 반납 되므로 memory 공간이 free 되는 것은 아닙니다.
         * memmbase가 nul인지 확인을 해도 사용하려는 타이밍에 null로 변할 수 있으니
         * pointer를 미리 받아서 null인지 확인 한 후에 사용합니다.
         * 상황에 따라서 pool 에서 page가 Free 될 수도 있습니다만,
         * drop tablespace가 자주 발생하는 건이 아니므로만 이정도로 방어 합니다.*/
        sMemBase = sCurTBS->mMemBase;

        if (( sctTableSpaceMgr::isMemTableSpace( sCurTBS->mHeader.mID ) == ID_TRUE ) &&
            ( sCurTBS->mRestoreType != SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET ) &&
            ( sMemBase != NULL ) &&
            ( SMI_TBS_IS_COMPLETE( sCurTBS->mHeader.mState ) ))
        {
            /* BUG-43006 FixedTable Indexing Filter
             * Indexing Filter를 사용해서 전체 Record를 생성하지않고
             * 부분만 생성해 Filtering 한다.
             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
             * 해당하는 값을 순서대로 넣어주어야 한다.
             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
             * 어 주어야한다.
             */
            sIndexValues[0] = &sCurTBS->mHeader.mID;
            if ( iduFixedTable::checkKeyRange( aMemory,
                                               gMemBaseMgrTableColDesc,
                                               sIndexValues )
                 == ID_FALSE )
            {
                sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
                continue;
            }
            else
            {
                /* Nothing to do */
            }
            sStat.mSpaceID             = (UInt)sCurTBS->mHeader.mID;
            sStat.mRestoreType         = &sCurTBS->mRestoreType;
            sStat.m_currentDB          = &sCurTBS->mTBSAttr.mMemAttr.mCurrentDB;
            sStat.m_lstSystemSCN       = &smmDatabase::mLstSystemSCN;
            sStat.mDBMaxPageCount      = &sCurTBS->mDBMaxPageCount;
            sStat.mHighLimitFile       = &sCurTBS->mHighLimitFile;
            sStat.mPageCountPerFile    = (ULong)sMemBase->mDBFilePageCount;
            sStat.m_nDBPageCountInDisk = &sCurTBS->mDBPageCountInDisk;
            sStat.mMaxAccessFileSize   = ID_SIZEOF(PDL_OFF_T);
            sStat.mPageSize            = SM_PAGE_SIZE;

            IDE_TEST(iduFixedTable::buildRecord(
                         aHeader,
                         aMemory,
                         (void *) &sStat) != IDE_SUCCESS);
        }
        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gMemBaseMgrTableDesc =
{
    (SChar *)"X$MEMBASEMGR",
    buildRecordForMemBaseMgr,
    gMemBaseMgrTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*
 * [BUG-24430] X$DB_FREE_PAGE_LIST의 mFreePageCount 컬럼 출력에 문제가 있습니다.
 * "vULong mFreePageCount"를 "ULong mFreePageCount"로 변경함.
 */

/* ------------------------------------------------
 *  Fixed Table Define for DbFreePageList
 * ----------------------------------------------*/

static iduFixedTableColDesc gMemTBSFreePageListTableColDesc[] =
{
    {
        (SChar*)"SPACE_ID",
        offsetof(smmPerfMemTBSFreePageList, mSpaceID),
        IDU_FT_SIZEOF(smmPerfMemTBSFreePageList, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RESOURCE_GROUP_ID",
        offsetof(smmPerfMemTBSFreePageList, mResourceGroupID),
        IDU_FT_SIZEOF(smmPerfMemTBSFreePageList, mResourceGroupID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FIRST_FREE_PAGE_ID",
        offsetof(smmPerfMemTBSFreePageList, mFirstFreePageID),
        IDU_FT_SIZEOF(smmPerfMemTBSFreePageList, mFirstFreePageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_PAGE_COUNT",
        offsetof(smmPerfMemTBSFreePageList, mFreePageCount),
        IDU_FT_SIZEOF(smmPerfMemTBSFreePageList, mFreePageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RESERVED_PAGE_COUNT",
        offsetof(smmPerfMemTBSFreePageList, mReservedPageCount),
        IDU_FT_SIZEOF(smmPerfMemTBSFreePageList, mReservedPageCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

/*
 * X$MEM_TABLESPACE_FREE_PAGE_LIST Performance View 의 레코드를 만들어낸다.
 */
IDE_RC buildRecordForMemTBSFreePageList(
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                * /* aDumpObj */,
    iduFixedTableMemory *aMemory)
{
    smmTBSNode *    sCurTBS;
    smmMemBase *    sMemBase;
    ULong           sNeedRecCount;
    UInt            i;
    smmPerfMemTBSFreePageList sPerfMemTBSFreeList;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();
    IDE_ASSERT(sCurTBS != NULL);

    sNeedRecCount = sCurTBS->mMemBase->mFreePageListCount;
    while( sCurTBS != NULL )
    {
        /* BUG-47948 tablespace가 drop되면 membase 가 null이 될 수 있습니다.
         * membase page는 pool 에 반납 되므로 memory 공간이 free 되는 것은 아닙니다.
         * memmbase가 nul인지 확인을 해도 사용하려는 타이밍에 null로 변할 수 있으니
         * pointer를 미리 받아서 null인지 확인 한 후에 사용합니다.
         * 상황에 따라서 pool 에서 page가 Free 될 수도 있습니다만,
         * drop tablespace가 자주 발생하는 건이 아니므로만 이정도로 방어 합니다.*/
        sMemBase = sCurTBS->mMemBase;

        if (( sctTableSpaceMgr::isMemTableSpace( sCurTBS->mHeader.mID ) == ID_TRUE ) &&
            ( sCurTBS->mRestoreType != SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET ) &&
            ( sMemBase != NULL ) &&
            ( SMI_TBS_IS_COMPLETE( sCurTBS->mHeader.mState ) ))
        {
            for (i = 0; i < sNeedRecCount; i++)
            {
                sPerfMemTBSFreeList.mSpaceID         = (UInt)sCurTBS->mTBSAttr.mID;
                sPerfMemTBSFreeList.mResourceGroupID = i;
                sPerfMemTBSFreeList.mFirstFreePageID =
                    sMemBase->mFreePageLists[i].mFirstFreePageID;
                sPerfMemTBSFreeList.mFreePageCount   =
                    sMemBase->mFreePageLists[i].mFreePageCount;
                /* BUG-31881 예약된 사용불가 페이지의 개수를 출력함 */
                IDE_TEST( smmFPLManager::getUnusablePageCount(
                              & sCurTBS->mArrPageReservation[i],
                              NULL, // Transaction
                              &(sPerfMemTBSFreeList.mReservedPageCount) )
                          == IDE_FAILURE );

                IDE_TEST(iduFixedTable::buildRecord(
                             aHeader,
                             aMemory,
                             (void *) &sPerfMemTBSFreeList )
                         != IDE_SUCCESS);
            }
        }
        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gMemTBSFreePageListTableDesc =
{
    (SChar *)"X$MEM_TABLESPACE_FREE_PAGE_LIST",
    buildRecordForMemTBSFreePageList,
    gMemTBSFreePageListTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



/*----------------------------------------
 * D$MEM_TBS_PCH
 *----------------------------------------- */

/* TASK-4007 [SM] PBT를 위한 기능 추가
 * PCH를 Dump할 수 있는 기능 추가 */
static iduFixedTableColDesc gDumpMemTBSPCHColDesc[] =
{
    {
        (SChar*)"SPACEID",
        IDU_FT_OFFSETOF(smmMemTBSPCHDump, mSpaceID),
        IDU_FT_SIZEOF(smmMemTBSPCHDump, mSpaceID),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MY_PAGEID",
        IDU_FT_OFFSETOF(smmMemTBSPCHDump, mMyPageID),
        IDU_FT_SIZEOF(smmMemTBSPCHDump, mMyPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE",
        IDU_FT_OFFSETOF(smmMemTBSPCHDump, mPage),
        IDU_FT_SIZEOF(smmMemTBSPCHDump, mPage),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"DIRTY",
        IDU_FT_OFFSETOF(smmMemTBSPCHDump, mDirty),
        IDU_FT_SIZEOF(smmMemTBSPCHDump, mDirty),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"DIRTY_STAT",
        IDU_FT_OFFSETOF(smmMemTBSPCHDump, mDirtyStat),
        IDU_FT_SIZEOF(smmMemTBSPCHDump, mDirtyStat),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"NEXT_SCAN_PID",
        IDU_FT_OFFSETOF(smmMemTBSPCHDump, mNxtScanPID),
        IDU_FT_SIZEOF(smmMemTBSPCHDump, mNxtScanPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PREV_SCAN_PID",
        IDU_FT_OFFSETOF(smmMemTBSPCHDump, mPrvScanPID),
        IDU_FT_SIZEOF(smmMemTBSPCHDump, mPrvScanPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MODIFYSEQ_FOR_SCAN",
        IDU_FT_OFFSETOF(smmMemTBSPCHDump, mModifySeqForScan),
        IDU_FT_SIZEOF(smmMemTBSPCHDump, mModifySeqForScan),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

// D$MEM_TBS_PCH
// 한 TBS의 모든 PCH를 Dump한다.
static IDE_RC buildRecordMemTBSPCHDump(idvSQL              * /*aStatistics*/,
                                       void                *aHeader,
                                       void                *aDumpObj,
                                       iduFixedTableMemory *aMemory)
{
    scSpaceID               sTBSID;
    UInt                    sLocked = ID_FALSE;
    smmPCH                * sPCH;
    UInt                    i;
    UInt                    sDBMaxPageCount;
    smmTBSNode            * sTBSNode;
    smmMemTBSPCHDump        sMemTBSPCHDump;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    /* BUG-28678  [SM] qmsDumpObjList::mObjInfo에 설정될 메모리 주소는
     * 반드시 공간을 할당해서 설정해야합니다.
     *
     * aDumpObj는 Pointer로 데이터가 오기 때문에 값을 가져와야 합니다. */
    sTBSID  = *( (scSpaceID*)aDumpObj );

    //MEM_TABLESPACE가 맞는지 검사한다.
    IDE_ASSERT( sctTableSpaceMgr::isMemTableSpace( sTBSID ) == ID_TRUE );

    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::getSpaceNodeBySpaceID( sTBSID );

    sDBMaxPageCount = sTBSNode->mDBMaxPageCount;

    for( i = 0 ; i < sDBMaxPageCount; i++ )
    {
        if( smmManager::isValidPageID( sTBSID, i ) != ID_TRUE )
        {
            continue;
        }

        sPCH = smmManager::getPCH( sTBSID, i );
        if( sPCH == NULL )
        {
            continue;
        }

        IDE_TEST( sPCH->mMutex.lock( NULL )
             != IDE_SUCCESS );
        sLocked = ID_TRUE;

        sMemTBSPCHDump.mSpaceID          = sPCH->mSpaceID;
        sMemTBSPCHDump.mMyPageID         = i;
        sMemTBSPCHDump.mPage             = (vULong)smmManager::getPagePtr( sTBSID, i );
        sMemTBSPCHDump.mDirty            = (sPCH->m_dirty == ID_TRUE) ? 'T' : 'F' ;
        sMemTBSPCHDump.mDirtyStat        = sPCH->m_dirtyStat;
        sMemTBSPCHDump.mNxtScanPID       = sPCH->mNxtScanPID;
        sMemTBSPCHDump.mPrvScanPID       = sPCH->mPrvScanPID;
        sMemTBSPCHDump.mModifySeqForScan = sPCH->mModifySeqForScan;

        sLocked = ID_FALSE;
        IDE_TEST( sPCH->mMutex.unlock() != IDE_SUCCESS );

        IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                             aMemory,
                                             (void *)&sMemTBSPCHDump)
                 != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( sPCH->mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gDumpMemTBSPCHTableDesc =
{
    (SChar *)"D$MEM_TBS_PCH",
    buildRecordMemTBSPCHDump,
    gDumpMemTBSPCHColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc gMemTablespaceDescColDesc[] =
{
    {
        (SChar*)"SPACE_ID",
        offsetof(smmPerfTBSDesc, mSpaceID),
        IDU_FT_SIZEOF(smmPerfTBSDesc, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SPACE_NAME",
        offsetof(smmPerfTBSDesc, mSpaceName),
        IDU_FT_SIZEOF(smmPerfTBSDesc, mSpaceName)-1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SPACE_STATUS",
        offsetof(smmPerfTBSDesc, mSpaceStatus),
        IDU_FT_SIZEOF(smmPerfTBSDesc, mSpaceStatus),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SPACE_SHM_KEY",
        offsetof(smmPerfTBSDesc, mSpaceShmKey),
        IDU_FT_SIZEOF(smmPerfTBSDesc, mSpaceShmKey),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"AUTOEXTEND_MODE",
        offsetof(smmPerfTBSDesc, mAutoExtendMode),
        IDU_FT_SIZEOF(smmPerfTBSDesc, mAutoExtendMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"AUTOEXTEND_NEXTSIZE",
        offsetof(smmPerfTBSDesc, mAutoExtendNextSize),
        IDU_FT_SIZEOF(smmPerfTBSDesc, mAutoExtendNextSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAXSIZE",
        offsetof(smmPerfTBSDesc, mMaxSize),
        IDU_FT_SIZEOF(smmPerfTBSDesc, mMaxSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CURRENT_SIZE",
        offsetof(smmPerfTBSDesc, mCurrentSize),
        IDU_FT_SIZEOF(smmPerfTBSDesc, mCurrentSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

/* Tablespace Node로부터 X$MEM_TABLESPACE_DESC의 구조체 구성
 */

IDE_RC constructTBSDesc( smmTBSNode     * aTBSNode,
                         smmPerfTBSDesc * aTBSDesc  )
{
    IDE_ASSERT( aTBSNode != NULL );
    IDE_ASSERT( aTBSDesc != NULL );

    smiMemTableSpaceAttr * sMemAttr = & aTBSNode->mTBSAttr.mMemAttr;
    smmMemBase *           sMemBase;

    // Tablespace의 Performance View구축중
    // Offline, Drop으로의 상태전이를 막기 위함
    sctTableSpaceMgr::lockSpaceNode( NULL /* idvSQL * */,
                                     aTBSNode );

    aTBSDesc->mSpaceID             = aTBSNode->mHeader.mID;
    idlOS::strncpy( aTBSDesc->mSpaceName,
                    aTBSNode->mTBSAttr.mName,
                    SMI_MAX_TABLESPACE_NAME_LEN );
    aTBSDesc->mSpaceName[ SMI_MAX_TABLESPACE_NAME_LEN ] = '\0';

    aTBSDesc->mSpaceStatus         = aTBSNode->mHeader.mState ;
    aTBSDesc->mSpaceShmKey         = (UInt) sMemAttr->mShmKey ;
    aTBSDesc->mAutoExtendMode      =
        (sMemAttr->mIsAutoExtend == ID_TRUE) ? (1) : (0) ;
    aTBSDesc->mAutoExtendNextSize  =
        sMemAttr->mNextPageCount * SM_PAGE_SIZE ;
    aTBSDesc->mMaxSize   =
        sMemAttr->mMaxPageCount * SM_PAGE_SIZE;

    /* BUG-47948 tablespace가 drop되면 membase 가 null이 될 수 있습니다.
     * membase page는 pool 에 반납 되므로 memory 공간이 free 되는 것은 아닙니다.
     * memmbase가 nul인지 확인을 해도 사용하려는 타이밍에 null로 변할 수 있으니
     * pointer를 미리 받아서 null인지 확인 한 후에 사용합니다.
     * 상황에 따라서 pool 에서 page가 Free 될 수도 있습니다만,
     * drop tablespace가 자주 발생하는 건이 아니므로만 이정도로 방어 합니다.*/
    sMemBase = aTBSNode->mMemBase;
    if ( sMemBase != NULL )
    {
        aTBSDesc->mCurrentSize     =
            sMemBase->mExpandChunkPageCnt *
            sMemBase->mCurrentExpandChunkCnt;
        aTBSDesc->mCurrentSize    *= SM_PAGE_SIZE ;
    }
    else // aTBSDesc->mMemBase == NULL
         // ==> Tablespace가 아직 Prepare/Restore되지 않은 상태
    {
        aTBSDesc->mCurrentSize     = 0;
    }

    sctTableSpaceMgr::unlockSpaceNode( aTBSNode );

    return IDE_SUCCESS;

    // 에러처리 해야할 경우 lock/unlock에 대한 상태처리 필요
}


/*
     X$MEM_TABLESPACE_DESC 의 레코드를 구축한다.
 */

IDE_RC buildRecordForMemTablespaceDesc(
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                * , // aDumpObj
    iduFixedTableMemory *aMemory)
{
    smmTBSNode *    sCurTBS;
    smmPerfTBSDesc  sTBSDesc;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();
    IDE_ASSERT(sCurTBS != NULL);

    while( sCurTBS != NULL )
    {
        if ( sctTableSpaceMgr::isMemTableSpace( sCurTBS ) == ID_TRUE )
        {
            IDE_TEST( constructTBSDesc( sCurTBS,
                                        & sTBSDesc )
                      != IDE_SUCCESS);

            IDE_TEST(iduFixedTable::buildRecord(
                         aHeader,
                         aMemory,
                         (void *) &sTBSDesc )
                     != IDE_SUCCESS);
        }

        // Drop된 Tablespace는 SKIP한다
        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gMemTablespaceDescTableDesc =
{
    (SChar *)"X$MEM_TABLESPACE_DESC",
    buildRecordForMemTablespaceDesc,
    gMemTablespaceDescColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc gCheckpointPathColDesc[] =
{
    {
        (SChar*)"SPACE_ID",
        offsetof(smmPerfCheckpointPath, mSpaceID),
        IDU_FT_SIZEOF(smmPerfCheckpointPath, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CHECKPOINT_PATH",
        offsetof(smmPerfCheckpointPath, mCheckpointPath),
        IDU_FT_SIZEOF(smmPerfCheckpointPath, mCheckpointPath)-1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


/*
  특정 Tablespace의 Checkpoint Path를 이용하여
  X$MEM_TABLESPACE_CHECKPOINT_PATHS의 레코드를 구축한다.
*/
IDE_RC buildRecordForCheckpointPathOfTBS( void                * aHeader,
                                          iduFixedTableMemory * aMemory,
                                          smmTBSNode          * aTBSNode )
{
    smmChkptPathNode      * sCPathNode;
    smuList               * sListNode;
    smmPerfCheckpointPath   sPerfCheckpointPath;
    UInt                    sState = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );
    IDE_ERROR( aTBSNode != NULL );

    // Tablespace의 Performance View구축중
    // Offline, Drop으로의 상태전이를 막기 위함
    sctTableSpaceMgr::lockSpaceNode( NULL /* idvSQL * */,
                                     aTBSNode );
    sState = 1;

    sListNode = SMU_LIST_GET_FIRST( & aTBSNode->mChkptPathBase );

    while ( sListNode != & aTBSNode->mChkptPathBase )
    {
        sCPathNode = (smmChkptPathNode*)  sListNode->mData;

        sPerfCheckpointPath.mSpaceID = aTBSNode->mHeader.mID;
        idlOS::strncpy( sPerfCheckpointPath.mCheckpointPath,
                        sCPathNode->mChkptPathAttr.mChkptPath,
                        SMI_MAX_CHKPT_PATH_NAME_LEN );

        sPerfCheckpointPath.mCheckpointPath[ SMI_MAX_CHKPT_PATH_NAME_LEN ] =
            '\0';

        IDE_TEST(iduFixedTable::buildRecord(
                     aHeader,
                     aMemory,
                     (void *) &sPerfCheckpointPath )
                 != IDE_SUCCESS);

        sListNode = SMU_LIST_GET_NEXT( sListNode );
    }

    sState = 0;
    sctTableSpaceMgr::unlockSpaceNode( aTBSNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch(sState)
    {
        case 1:
            sctTableSpaceMgr::unlockSpaceNode( aTBSNode );
            break;
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;

}

/*
  X$MEM_TABLESPACE_CHECKPOINT_PATHS 의 레코드를 구축한다.
*/
IDE_RC buildRecordForCheckpointPath(
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                * /* aDumpObj */,
    iduFixedTableMemory *aMemory)
{
    smmTBSNode *    sCurTBS;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();
    IDE_ASSERT(sCurTBS != NULL);

    while( sCurTBS != NULL )
    {
        if ( sctTableSpaceMgr::isMemTableSpace( sCurTBS ) == ID_TRUE )
        {
            IDE_TEST( buildRecordForCheckpointPathOfTBS( aHeader,
                                                         aMemory,
                                                         sCurTBS )
                      != IDE_SUCCESS );
        }

        // Drop된 Tablespace는 SKIP한다
        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gMemTablespaceCheckpointPathsTableDesc =
{
    (SChar *)"X$MEM_TABLESPACE_CHECKPOINT_PATHS",
    buildRecordForCheckpointPath,
    gCheckpointPathColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



static iduFixedTableColDesc gMemTablespaceStatusDescColDesc[] =
{
    {
        (SChar*)"STATUS",
        offsetof(smmPerfTBSStatusDesc, mStatus),
        IDU_FT_SIZEOF(smmPerfTBSStatusDesc, mStatus),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATUS_DESC",
        offsetof(smmPerfTBSStatusDesc, mStatusDesc),
        IDU_FT_SIZEOF(smmPerfTBSStatusDesc, mStatusDesc)-1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

/*
    X$MEM_TABLESPACE_STATUS_DESC의 레코드를 구축한다.

*/
IDE_RC buildRecordForMemTablespaceStatusDesc(
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                * /* aDumpObj */,
    iduFixedTableMemory *aMemory)
{
    smmPerfTBSStatusDesc sPerfStatusDesc;
    smmTBSStatusDesc     sStatusDesc[] =
        {
            { (UInt)SMI_TBS_OFFLINE,              (SChar*)"OFFLINE" },
            { (UInt)SMI_TBS_ONLINE,               (SChar*)"ONLINE" },
            { (UInt)SMI_TBS_DISCARDED,            (SChar*)"DISCARDED" },
            { (UInt)SMI_TBS_DROPPED,              (SChar*)"DROPPED" },
            { (UInt)SMI_TBS_BACKUP,               (SChar*)"BACKUP" },
            { (UInt)SMI_TBS_CREATING,             (SChar*)"CREATING" },
            { (UInt)SMI_TBS_DROPPING,             (SChar*)"DROPPING" },
            { (UInt)SMI_TBS_DROP_PENDING,         (SChar*)"DROP_PENDING" },
            { (UInt)SMI_TBS_SWITCHING_TO_OFFLINE, (SChar*)"SWITCHING_TO_OFFLINE" },
            { (UInt)SMI_TBS_SWITCHING_TO_ONLINE,  (SChar*)"SWITCHING_TO_ONLINE" },
            { (UInt)SMI_TBS_BLOCK_BACKUP,         (SChar*)"BLOCK_BACKUP" },
            { 0, NULL }
        };
    UInt            i;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    for (i=0; sStatusDesc[i].mStatusDesc != NULL; i++ )
    {

        sPerfStatusDesc.mStatus = sStatusDesc[i].mStatus;
        idlOS::strncpy( sPerfStatusDesc.mStatusDesc,
                        sStatusDesc[i].mStatusDesc,
                        SMM_MAX_TBS_STATUS_DESC_LEN );
        sPerfStatusDesc.mStatusDesc[ SMM_MAX_TBS_STATUS_DESC_LEN ] = '\0';

        IDE_TEST(iduFixedTable::buildRecord(
                     aHeader,
                     aMemory,
                     (void *) & sPerfStatusDesc )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gMemTablespaceStatusDescTableDesc =
{
    (SChar *)"X$MEM_TABLESPACE_STATUS_DESC",
    buildRecordForMemTablespaceStatusDesc,
    gMemTablespaceStatusDescColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*----------------------------------------
 * D$MEM_DB_PERSPAGEHDR
 *----------------------------------------- */

/* PROJ-2162 RestartRiskReduction
 * PERSPAGEHEADER를 DUMP할 수 있는 기능 추가 */
static iduFixedTableColDesc gDumpMemDBPersPageHdrColDesc[] =
{
    {
        (SChar*)"SELF",
        IDU_FT_OFFSETOF(smpPersPageHeader, mSelfPageID),
        IDU_FT_SIZEOF(smpPersPageHeader, mSelfPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PREV",
        IDU_FT_OFFSETOF(smpPersPageHeader, mPrevPageID),
        IDU_FT_SIZEOF(smpPersPageHeader, mPrevPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"NEXT",
        IDU_FT_OFFSETOF(smpPersPageHeader, mNextPageID),
        IDU_FT_SIZEOF(smpPersPageHeader, mNextPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TYPE",
        IDU_FT_OFFSETOF(smpPersPageHeader, mType),
        IDU_FT_SIZEOF(smpPersPageHeader, mType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TABLE_OID",
        IDU_FT_OFFSETOF(smpPersPageHeader, mTableOID),
        IDU_FT_SIZEOF(smpPersPageHeader, mTableOID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"ALLOC_LIST_ID",
        IDU_FT_OFFSETOF(smpPersPageHeader, mAllocListID),
        IDU_FT_SIZEOF(smpPersPageHeader, mAllocListID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

// D$MEM_DB_PERSPAGEHDR
// 특정 Page의 Header를 Dump한다.
static IDE_RC buildRecordMemDBPersPageHdrDump(idvSQL              * /*aStatistics*/,
                                              void                *aHeader,
                                              void                *aDumpObj,
                                              iduFixedTableMemory *aMemory)
{
    smpPersPageHeader        * sPersPageHeader;
    scGRID                   * sGRID = NULL;
    scSpaceID                  sSpaceID;
    scPageID                   sPageID;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sGRID      = (scGRID*) aDumpObj;
    sSpaceID   = sGRID->mSpaceID;
    sPageID    = sGRID->mPageID;

    //MEM_TABLESPACE가 맞는지 검사한다.
    IDE_ASSERT( sctTableSpaceMgr::isMemTableSpace( sSpaceID ) == ID_TRUE );

    sPersPageHeader = (smpPersPageHeader*) smmManager::getPagePtr( sSpaceID, sPageID ) ;

    IDE_TEST_RAISE( sPersPageHeader == NULL , ERR_EMPTY_OBJECT );
    

    IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                         aMemory,
                                         (void *)sPersPageHeader)
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gDumpMemDBPersPageHdrDesc =
{
    (SChar *)"D$MEM_DB_PERSPAGEHDR",
    buildRecordMemDBPersPageHdrDump,
    gDumpMemDBPersPageHdrColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


