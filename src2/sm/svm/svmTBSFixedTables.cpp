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
* $Id: svmTBSFixedTable.cpp 22277 2007-06-25 01:34:15Z newdaily $
**********************************************************************/


/*
  Tablespace와 관련된 Fixed Table들을 정의한다.
 */

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smiFixedTable.h>
#include <sctTableSpaceMgr.h>
#include <svm.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smu.h>
#include <smuUtility.h>
#include <svmReq.h>
#include <svmDef.h>


/* ------------------------------------------------
 *  Fixed Table Define for X$VOL_TABLESPACE_DESC
 * ----------------------------------------------*/

typedef struct svmPerfTBSDesc
{
    UInt       mSpaceID;
    SChar      mSpaceName[SMI_MAX_TABLESPACE_NAME_LEN + 1];
    UInt       mSpaceStatus;
    UInt       mAutoExtendMode;
    ULong      mInitSize;
    ULong      mNextSize;
    ULong      mMaxSize;
    ULong      mCurrentSize;
} svmPerfTBSDesc ;

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
    IDE_ASSERT( sctTableSpaceMgr::lock(NULL /* idvSQL * */)
                == IDE_SUCCESS );
    
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

    IDE_ASSERT( sctTableSpaceMgr::unlock() == IDE_SUCCESS );

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

    sctTableSpaceMgr::getFirstSpaceNode((void**)&sCurTBS);
    IDE_ASSERT(sCurTBS != NULL);

    while( sCurTBS != NULL )
    {
        if( sctTableSpaceMgr::isVolatileTableSpace(sCurTBS->mHeader.mID) != ID_TRUE ) 
        {
            sctTableSpaceMgr::getNextSpaceNode(sCurTBS, (void**)&sCurTBS );
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
        sctTableSpaceMgr::getNextSpaceNode((void*)sCurTBS, (void**)&sCurTBS);
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
