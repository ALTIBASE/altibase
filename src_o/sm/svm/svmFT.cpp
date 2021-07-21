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
* $Id: svmFT.cpp 37235 2009-12-09 01:56:06Z elcarim $
**********************************************************************/

#include <idl.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <svmFPLManager.h>
#include <svmReq.h>
#include <svmDef.h>
#include <svmManager.h>
#include <svmFT.h>
#include <sctTableSpaceMgr.h>

/*----------------------------------------
 * D$VOL_TBS_PCH
 *----------------------------------------- */

/* TASK-4007 [SM] PBT를 위한 기능 추가
 * PCH를 Dump할 수 있는 기능 추가 */

static iduFixedTableColDesc gDumpVolTBSPCHColDesc[] =
{
    {
        (SChar*)"SPACEID",
        IDU_FT_OFFSETOF(svmVolTBSPCHDump, mSpaceID),
        IDU_FT_SIZEOF(svmVolTBSPCHDump, mSpaceID),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MY_PAGEID",
        IDU_FT_OFFSETOF(svmVolTBSPCHDump, mMyPageID),
        IDU_FT_SIZEOF(svmVolTBSPCHDump, mMyPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE",
        IDU_FT_OFFSETOF(svmVolTBSPCHDump, mPage),
        IDU_FT_SIZEOF(svmVolTBSPCHDump, mPage),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"NEXT_SCAN_PID",
        IDU_FT_OFFSETOF(svmVolTBSPCHDump, mNxtScanPID),
        IDU_FT_SIZEOF(svmVolTBSPCHDump, mNxtScanPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PREV_SCAN_PID",
        IDU_FT_OFFSETOF(svmVolTBSPCHDump, mPrvScanPID),
        IDU_FT_SIZEOF(svmVolTBSPCHDump, mPrvScanPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MODIFYSEQ_FOR_SCAN",
        IDU_FT_OFFSETOF(svmVolTBSPCHDump, mModifySeqForScan),
        IDU_FT_SIZEOF(svmVolTBSPCHDump, mModifySeqForScan),
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

// D$VOL_TBS_PCH
// 한 TBS의 모든 PCH를 Dump한다.
static IDE_RC buildRecordVolTBSPCHDump(idvSQL              * /*aStatistics*/,
                                       void                *aHeader,
                                       void                *aDumpObj,
                                       iduFixedTableMemory *aMemory)
{
    scSpaceID               sSpaceID;
    svmPCH                * sPCH;
    UInt                    i;
    UInt                    sDBMaxPageCount;
    svmTBSNode            * sTBSNode;
    svmVolTBSPCHDump        sVolTBSPCHDump;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    /* BUG-28678  [SM] qmsDumpObjList::mObjInfo에 설정될 메모리 주소는 
     * 반드시 공간을 할당해서 설정해야합니다. 
     * 
     * aDumpObj는 Pointer로 데이터가 오기 때문에 값을 가져와야 합니다. */
    sSpaceID  = *( (scSpaceID*)aDumpObj );

    // VOL_TABLESPACE가 맞는지 검사한다.
    IDE_ASSERT( sctTableSpaceMgr::isVolatileTableSpace( sSpaceID ) == ID_TRUE );


    sTBSNode = (svmTBSNode*)sctTableSpaceMgr::getSpaceNodeBySpaceID( sSpaceID );


    sDBMaxPageCount = sTBSNode->mDBMaxPageCount;

    for( i = 0 ; i < sDBMaxPageCount; i++ )
    {
        if( smmManager::isValidPageID( sSpaceID, i ) != ID_TRUE )
        {
            continue;
        }

        sPCH = (svmPCH*)smmManager::getPCH( sSpaceID, i );
        if( sPCH == NULL )
        {
            continue;
        }

        sVolTBSPCHDump.mSpaceID          = sSpaceID;
        sVolTBSPCHDump.mMyPageID         = i;
        sVolTBSPCHDump.mPage             = (vULong)smmManager::getPagePtr( sSpaceID, i );
        sVolTBSPCHDump.mNxtScanPID       = sPCH->mNxtScanPID;
        sVolTBSPCHDump.mPrvScanPID       = sPCH->mPrvScanPID;
        sVolTBSPCHDump.mModifySeqForScan = sPCH->mModifySeqForScan;

        IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                             aMemory,
                                             (void *)&sVolTBSPCHDump)
                 != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gDumpVolTBSPCHTableDesc =
{
    (SChar *)"D$VOL_TBS_PCH",
    buildRecordVolTBSPCHDump,
    gDumpVolTBSPCHColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc gVolTablespaceDescColDesc[] =
{
    {
        (SChar*)"SPACE_ID",
        offsetof(svmPerfTBSDesc, mSpaceID),
        IDU_FT_SIZEOF(svmPerfTBSDesc, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SPACE_NAME",
        offsetof(svmPerfTBSDesc, mSpaceName),
        IDU_FT_SIZEOF(svmPerfTBSDesc, mSpaceName)-1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SPACE_STATUS",
        offsetof(svmPerfTBSDesc, mSpaceStatus),
        IDU_FT_SIZEOF(svmPerfTBSDesc, mSpaceStatus),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INIT_SIZE",
        offsetof(svmPerfTBSDesc, mInitSize),
        IDU_FT_SIZEOF(svmPerfTBSDesc, mInitSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"AUTOEXTEND_MODE",
        offsetof(svmPerfTBSDesc, mAutoExtendMode),
        IDU_FT_SIZEOF(svmPerfTBSDesc, mAutoExtendMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_SIZE",
        offsetof(svmPerfTBSDesc, mNextSize),
        IDU_FT_SIZEOF(svmPerfTBSDesc, mNextSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_SIZE",
        offsetof(svmPerfTBSDesc, mMaxSize),
        IDU_FT_SIZEOF(svmPerfTBSDesc, mMaxSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CURRENT_SIZE",
        offsetof(svmPerfTBSDesc, mCurrentSize),
        IDU_FT_SIZEOF(svmPerfTBSDesc, mCurrentSize),
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

/* Tablespace Node로부터 X$VOL_TABLESPACE_DESC의 구조체 구성
 */
   
IDE_RC constructTBSDesc( svmTBSNode     * aTBSNode,
                         svmPerfTBSDesc * aTBSDesc  )
{
    IDE_ASSERT( aTBSNode != NULL );
    IDE_ASSERT( aTBSDesc != NULL );
    
    smiVolTableSpaceAttr * sVolAttr = & aTBSNode->mTBSAttr.mVolAttr;

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
    aTBSDesc->mInitSize            = 
        sVolAttr->mInitPageCount * SM_PAGE_SIZE ;
    aTBSDesc->mAutoExtendMode      =
        (sVolAttr->mIsAutoExtend == ID_TRUE) ? (1) : (0) ;
    aTBSDesc->mNextSize  =
        sVolAttr->mNextPageCount * SM_PAGE_SIZE ;
    aTBSDesc->mMaxSize   =
        sVolAttr->mMaxPageCount * SM_PAGE_SIZE;

    aTBSDesc->mCurrentSize     =
        aTBSNode->mMemBase.mExpandChunkPageCnt *
        aTBSNode->mMemBase.mCurrentExpandChunkCnt;
    aTBSDesc->mCurrentSize    *= SM_PAGE_SIZE ;

    sctTableSpaceMgr::unlockSpaceNode( aTBSNode  );

    return IDE_SUCCESS;

    // 에러처리 해야할 경우 lock/unlock에 대한 상태처리 필요
}

/*
     X$VOL_TABLESPACE_DESC 의 레코드를 구축한다.
 */

IDE_RC buildRecordForVolTablespaceDesc(
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                * , // aDumpObj
    iduFixedTableMemory *aMemory)
{
    svmTBSNode *    sCurTBS;
    svmPerfTBSDesc  sTBSDesc;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurTBS = (svmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();
    IDE_ASSERT(sCurTBS != NULL);

    while( sCurTBS != NULL )
    {
        if( sctTableSpaceMgr::isVolatileTableSpace(sCurTBS->mHeader.mID) != ID_TRUE ) 
        {
            sCurTBS = (svmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
            continue;
        }

        IDE_TEST( constructTBSDesc( sCurTBS,
                                    & sTBSDesc )
                  != IDE_SUCCESS);
        
        IDE_TEST(iduFixedTable::buildRecord(
                     aHeader,
                     aMemory,
                     (void *) &sTBSDesc )
                 != IDE_SUCCESS);

        // Drop된 Tablespace는 SKIP한다
        sCurTBS = (svmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gVolTablespaceDescTableDesc =
{
    (SChar *)"X$VOL_TABLESPACE_DESC",
    buildRecordForVolTablespaceDesc,
    gVolTablespaceDescColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  Fixed Table Define for X$VOL_TABLESPACE_FREE_PAGE_LIST
 * ----------------------------------------------*/

static iduFixedTableColDesc gVolTBSFreePageListTableColDesc[] =
{
    {
        (SChar*)"SPACE_ID",
        offsetof(svmPerfVolTBSFreePageList, mSpaceID),
        IDU_FT_SIZEOF(svmPerfVolTBSFreePageList, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RESOURCE_GROUP_ID",
        offsetof(svmPerfVolTBSFreePageList, mResourceGroupID),
        IDU_FT_SIZEOF(svmPerfVolTBSFreePageList, mResourceGroupID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FIRST_FREE_PAGE_ID",
        offsetof(svmPerfVolTBSFreePageList, mFirstFreePageID),
        IDU_FT_SIZEOF(svmPerfVolTBSFreePageList, mFirstFreePageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_PAGE_COUNT",
        offsetof(svmPerfVolTBSFreePageList, mFreePageCount),
        IDU_FT_SIZEOF(svmPerfVolTBSFreePageList, mFreePageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RESERVED_PAGE_COUNT",
        offsetof(svmPerfVolTBSFreePageList, mReservedPageCount),
        IDU_FT_SIZEOF(svmPerfVolTBSFreePageList, mReservedPageCount),
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
 * X$VOL_TABLESPACE_FREE_PAGE_LIST Performance View 의 레코드를 만들어낸다.
 */
IDE_RC buildRecordForVolTBSFreePageList(
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                * /* aDumpObj */,
    iduFixedTableMemory *aMemory)
{
    svmTBSNode *    sCurTBS;
    ULong           sNeedRecCount;
    UInt            i;
    svmPerfVolTBSFreePageList sPerfVolTBSFreeList;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurTBS = (svmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();
    IDE_ASSERT(sCurTBS != NULL);

    while( sCurTBS != NULL )
    {
        if( sctTableSpaceMgr::isVolatileTableSpace(sCurTBS->mHeader.mID) != ID_TRUE )
        {
            sCurTBS = (svmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
            continue;
        }

        sNeedRecCount = sCurTBS->mMemBase.mFreePageListCount;

        for (i = 0; i < sNeedRecCount; i++)
        {
            sPerfVolTBSFreeList.mSpaceID         = (UInt)sCurTBS->mTBSAttr.mID;
            sPerfVolTBSFreeList.mResourceGroupID = i;
            sPerfVolTBSFreeList.mFirstFreePageID =
                sCurTBS->mMemBase.mFreePageLists[i].mFirstFreePageID;
            sPerfVolTBSFreeList.mFreePageCount   =
                sCurTBS->mMemBase.mFreePageLists[i].mFreePageCount;

            /* BUG-31881 예약된 사용불가 페이지의 개수를 출력함 */
            IDE_TEST( svmFPLManager::getUnusablePageCount(
                    & sCurTBS->mArrPageReservation[i],
                    NULL, // Transaction
                    &(sPerfVolTBSFreeList.mReservedPageCount) )
                    == IDE_FAILURE );

            IDE_TEST(iduFixedTable::buildRecord(
                         aHeader,
                         aMemory,
                         (void *) &sPerfVolTBSFreeList )
                     != IDE_SUCCESS);
        }
        sCurTBS = (svmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gVolTBSFreePageListTableDesc =
{
    (SChar *)"X$VOL_TABLESPACE_FREE_PAGE_LIST",
    buildRecordForVolTBSFreePageList,
    gVolTBSFreePageListTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

