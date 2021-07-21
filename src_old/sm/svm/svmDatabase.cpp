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
 
#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <svmDef.h>
#include <svmDatabase.h>
#include <smu.h>
#include <smr.h>

/* 데이터베이스 생성시 membase를 초기화한다.
 * aDBName           [IN] 데이터베이스 이름
 * aDbFilePageCount  [IN] 하나의 데이터베이스 파일이 가지는 Page수
 * aChunkPageCount   [IN] 하나의 Expand Chunk당 Page수
 */
IDE_RC svmDatabase::initializeMembase( svmTBSNode * aTBSNode,
                                       SChar *      aDBName,
                                       vULong       aChunkPageCount )
{
    SInt i;

    IDE_ASSERT( aDBName != NULL );
    IDE_ASSERT( idlOS::strlen( aDBName ) < SM_MAX_DB_NAME );
    IDE_ASSERT( aChunkPageCount > 0 );

    idlOS::strncpy( aTBSNode->mMemBase.mDBname,
                    aDBName,
                    SM_MAX_DB_NAME - 1 );
    aTBSNode->mMemBase.mDBname[ SM_MAX_DB_NAME - 1 ] = '\0';

    aTBSNode->mMemBase.mAllocPersPageCount = 0;
    aTBSNode->mMemBase.mExpandChunkPageCnt = aChunkPageCount ;
    aTBSNode->mMemBase.mCurrentExpandChunkCnt = 0;

    // Free Page List를 초기화 한다.
    for ( i = 0 ; i< SVM_MAX_FPL_COUNT ; i++ )
    {
        aTBSNode->mMemBase.mFreePageLists[i].mFirstFreePageID = SM_NULL_PID;
        aTBSNode->mMemBase.mFreePageLists[i].mFreePageCount = 0;
    }

    aTBSNode->mMemBase.mFreePageListCount = SVM_FREE_PAGE_LIST_COUNT;

    return IDE_SUCCESS;
}

/*
 * Expand Chunk관련된 프로퍼티 값들이 제대로 된 값인지 체크한다.
 *
 * 1. 하나의 Chunk안의 데이터 페이지를 여러 Free Page List에 분배할 때,
 *    최소한 한번은 분배가 되는지 체크
 *
 *    만족조건 : Chunk당 데이터페이지수 >= 2 * List당 분배할 Page수 * List 수
 *
 * aChunkDataPageCount [IN] Expand Chunk안의 데이터페이지 수
 *                          ( FLI Page를 제외한 Page의 수 )
 */
IDE_RC svmDatabase::checkExpandChunkProps( svmMemBase * aMemBase )
{
    IDE_DASSERT( aMemBase != NULL );

    // 다중화된 Free Page List수가  createdb시점과 다른 경우
    IDE_TEST_RAISE( aMemBase->mFreePageListCount !=
                    SVM_FREE_PAGE_LIST_COUNT,
                    different_page_list_count );

    // Expand Chunk안의 Page수가 createdb시점과 다른 경우
    IDE_TEST_RAISE( aMemBase->mExpandChunkPageCnt !=
                    smuProperty::getExpandChunkPageCount() ,
                    different_expand_chunk_page_count );

    //  Expand Chunk가 추가될 때
    //  ( 데이터베이스 Chunk가 새로 할당될 때  )
    //  그 안의 Free Page들은 여러개의 다중화된 Free Page List로 분배된다.
    //
    //  이 때, 각각의 Free Page List에 최소한 하나의 Free Page가
    //  분배되어야 하도록 시스템의 아키텍쳐가 설계되어 있다.
    //
    //  만약 Expand Chunk안의 Free Page수가 충분하지 않아서,
    //  PER_LIST_DIST_PAGE_COUNT 개씩 모든 Free Page List에 분배할 수가
    //  없다면 에러를 발생시킨다.
    //
    //  Expand Chunk안의 Free List Info Page를 제외한
    //  데이터페이지만이 Free Page List에 분배되므로, 이 갯수를 체크해야 한다.
    //  그러나, 이러한 모든 내용을 일반 사용자가 이해하기에는 너무 난해하다.
    //
    //  Expand Chunk의 페이지 수는 Free Page List에 두번씩 분배할 수 있을만큼
    //  충분한 크기를 가지도록 강제한다.
    //
    //  조건식 : EXPAND_CHUNK_PAGE_COUNT <=
    //           2 * PER_LIST_DIST_PAGE_COUNT * PAGE_LIST_GROUP_COUNT
   IDE_TEST_RAISE( aMemBase->mExpandChunkPageCnt <
                   ( 2 * SVM_PER_LIST_DIST_PAGE_COUNT * aMemBase->mFreePageListCount ),
                   err_too_many_per_list_page_count );

    return IDE_SUCCESS;

    IDE_EXCEPTION(different_expand_chunk_page_count);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_DIFFERENT_DB_EXPAND_CHUNK_PAGE_COUNT,
                                SVM_FREE_PAGE_LIST_COUNT,
                                aMemBase->mExpandChunkPageCnt ));
    }
    IDE_EXCEPTION(different_page_list_count);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_DIFFERENT_DB_FREE_PAGE_LIST_COUNT,
                                SVM_FREE_PAGE_LIST_COUNT,
                                aMemBase->mFreePageListCount ));
    }
    IDE_EXCEPTION( err_too_many_per_list_page_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TOO_MANY_PER_LIST_PAGE_COUNT_ERROR,
                                (ULong) aMemBase->mExpandChunkPageCnt,
                                (ULong) SVM_PER_LIST_DIST_PAGE_COUNT,
                                (ULong) aMemBase->mFreePageListCount ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

