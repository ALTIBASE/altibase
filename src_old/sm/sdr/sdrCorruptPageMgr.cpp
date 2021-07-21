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
 * $Id:$
 *
 * Description :
 *
 * DRDB 복구과정에서의 Corrupted page 처리에 대한 구현파일이다.
 *
 **********************************************************************/

#include <sdp.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <sdrCorruptPageMgr.h>
#include <sct.h>
#include <sdptbExtent.h>

// Redo 시에 Direct-Path INSERT 수행 시 발생한
// Partial Write Page는 Skip 하기 위하여 필요한 자료 구조
smuHashBase sdrCorruptPageMgr::mCorruptedPages;
UInt        sdrCorruptPageMgr::mHashTableSize;
sdbCorruptPageReadPolicy sdrCorruptPageMgr::mCorruptPageReadPolicy;

/***********************************************************************
 * Description : Corrupt page 관리자의 초기화 함수이다.
 *               addCorruptedPage가 redoAll과정 중에 수행되므로
 *               그 이전에 초기화 되어야 한다.
 *
 *  aHashTableSize - [IN] CorruptedPagesHash의 bucket count
 **********************************************************************/
IDE_RC sdrCorruptPageMgr::initialize( UInt  aHashTableSize )
{
    IDE_DASSERT(aHashTableSize >  0);

    mHashTableSize = aHashTableSize;

    // Corrupted Page들을 저장할 Hash Table
    // HashKey : <space, pageID, unused>
    IDE_TEST( smuHash::initialize( &mCorruptedPages,
                                   1,                 // ConcurrentLevel
                                   mHashTableSize,    // aBucketCount
                                   ID_SIZEOF(scGRID), // aKeyLength
                                   ID_FALSE,          // aUseLatch
                                   genHashValueFunc,
                                   compareFunc ) != IDE_SUCCESS );

    // BUG-45598: 프로퍼티에 맞춰 Corrupt Page 처리 정책을 설정
    setPageErrPolicyByProp();
 
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Corrupt page 관리자의 해제 함수이다.
 **********************************************************************/
IDE_RC sdrCorruptPageMgr::destroy()
{
    IDE_TEST( smuHash::destroy(&mCorruptedPages) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Corrupted pages hash에 corrupted page정보를 수집한다.
 *
 *  aSpaceID - [IN] tablespace ID
 *  aPageID  - [IN] corrupted page ID
 ***********************************************************************/
IDE_RC sdrCorruptPageMgr::addCorruptPage( scSpaceID aSpaceID,
                                          scPageID  aPageID )
{
    scGRID                  sLogGRID;
    sdrCorruptPageHashNode* sNode;
    UInt                    sState = 0;
    smiTableSpaceAttr       sTBSAttr;

    SC_MAKE_GRID( sLogGRID, aSpaceID, aPageID, 0 );

    IDE_TEST( smuHash::findNode( &mCorruptedPages,
                                 &sLogGRID,
                                 (void**)&sNode )
              != IDE_SUCCESS );

    if ( sNode == NULL )
    {
        //------------------------------------------
        // corrupted page에 등록되지 않은 page이면
        // 이를 mCorruptedPages에 등록
        //------------------------------------------

        /* sdrCorruptPageMgr_addCorruptPage_malloc_Node.tc */
        IDU_FIT_POINT("sdrCorruptPageMgr::addCorruptPage::malloc::Node");
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SDR,
                                     ID_SIZEOF( sdrCorruptPageHashNode ),
                                     (void**)&sNode )
                  != IDE_SUCCESS );

        sState = 1;

        sNode->mSpaceID = aSpaceID;
        sNode->mPageID  = aPageID;

        sctTableSpaceMgr::getTBSAttrByID( NULL,
                                          aSpaceID,
                                          &sTBSAttr );
        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     SM_TRC_DRECOVER_ADD_CORRUPTED_PAGE,
                     sTBSAttr.mName,
                     aPageID );

        IDE_TEST( smuHash::insertNode( &mCorruptedPages,
                                       &sLogGRID,
                                       sNode )
                  != IDE_SUCCESS );
    }
    else
    {
        // 이미 corruptedPages에 등록되어 있다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_ASSERT( iduMemMgr::free(sNode) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Corrupted pages hash에 corrupted page가 있다면 삭제한다.
 *
 *  aSpaceID - [IN] tablespace ID
 *  aPageID  - [IN] corrupted page ID
 ***********************************************************************/
IDE_RC sdrCorruptPageMgr::delCorruptPage( scSpaceID aSpaceID,
                                          scPageID  aPageID )
{
    scGRID                  sLogGRID;
    sdrCorruptPageHashNode* sNode = NULL;
    smiTableSpaceAttr       sTBSAttr;

    SC_MAKE_GRID( sLogGRID, aSpaceID, aPageID, 0 );

    IDE_TEST( smuHash::findNode( &mCorruptedPages,
                                 &sLogGRID,
                                 (void**)&sNode )
              != IDE_SUCCESS );

    if ( sNode != NULL )
    {
        IDE_TEST( smuHash::deleteNode( &mCorruptedPages,
                                       &sLogGRID,
                                       (void**)&sNode )
                  != IDE_SUCCESS );

        sctTableSpaceMgr::getTBSAttrByID( NULL,
                                          aSpaceID,
                                          &sTBSAttr );
        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     SM_TRC_DRECOVER_DEL_CORRUPTED_PAGE,
                     sTBSAttr.mName,
                     aPageID );

        IDE_ASSERT( iduMemMgr::free(sNode) == IDE_SUCCESS );
    }
    else
    {
        // 없다면 상관없음
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 * Description : Corrupted Page들의 상태를 검사한다.
 *
 *    Corrupted pages hash 에 포함된 page들이 속한 Extent의 상태가
 *    Free인지 모두 확인한다. Free extent가 아닌경우 Corrupted page의
 *    PID 및 extent의 PID를 boot log에 기록하고 서버를 종료한다.
 *
 **********************************************************************/
IDE_RC sdrCorruptPageMgr::checkCorruptedPages()
{
    sdrCorruptPageHashNode * sNode;
    UInt                     sState = 0;
    idBool                   sExistCorruptPage;
    idBool                   sIsFreeExt;
    smiTableSpaceAttr        sTBSAttr;
    scSpaceID                sSpaceID = SC_NULL_SPACEID ;
    scPageID                 sPageID  = SC_NULL_PID;

    mCorruptPageReadPolicy = SDB_CORRUPTED_PAGE_READ_ABORT;

    IDE_TEST( smuHash::open( &mCorruptedPages) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smuHash::cutNode( &mCorruptedPages, (void **)&sNode )
              != IDE_SUCCESS );

    sExistCorruptPage = ID_FALSE;

    while( sNode != NULL )
    {
        sIsFreeExt = ID_FALSE ;

        // 해당 extent가 현재 free 인지 확인한다.
        IDE_TEST( sdptbExtent::isFreeExtPage( NULL /*idvSQL*/,
                                              sNode->mSpaceID,
                                              sNode->mPageID,
                                              &sIsFreeExt )
                  != IDE_SUCCESS );

        if( sIsFreeExt == ID_TRUE )
        {
            // page가 속한 extent가 free이다
            sctTableSpaceMgr::getTBSAttrByID( NULL,
                                              sNode->mSpaceID,
                                              &sTBSAttr );
            ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                         SM_TRC_DRECOVER_DEL_CORRUPTED_PAGE_NOTALLOC,
                         sTBSAttr.mName,
                         sNode->mPageID );
        }
        else
        {
            // page가 속한 extent가 free가 아니다.
            sctTableSpaceMgr::getTBSAttrByID( NULL,
                                              sNode->mSpaceID,
                                              &sTBSAttr );
            ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                         SM_TRC_DRECOVER_PAGE_IS_CORRUPTED,
                         sTBSAttr.mName,
                         sNode->mPageID );

            sSpaceID = sNode->mSpaceID;
            sPageID  = sNode->mPageID;
            
            sExistCorruptPage = ID_TRUE;
        }

        IDE_TEST( iduMemMgr::free( sNode ) != IDE_SUCCESS);

        IDE_TEST( smuHash::cutNode( &mCorruptedPages, (void **)&sNode )
                 != IDE_SUCCESS);
    }

    if( ( smuProperty::getCorruptPageErrPolicy()
          & SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL )
        == SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL )
    {
        // corrupt page가 발견 되었을 시에 서버를 종료할 것인지를
        // 정하는 property, 이 Property가 꺼져 있을 경우 corrupt
        // page를 발견 하더라도 GG,LG Hdr가 corrupt 된 경우를
        // 제외하고는 서버를 종료하지 않는다. default : 0 (skip)

        IDE_TEST_RAISE( sExistCorruptPage == ID_TRUE,
                        page_corruption_fatal_error );
    }

    sState = 0;
    IDE_TEST( smuHash::close( &mCorruptedPages ) != IDE_SUCCESS );
    
    // BUG-45598: 프로퍼티에 맞춰 Corrupt Page 처리 정책을 재 설정
    setPageErrPolicyByProp();

    return IDE_SUCCESS;

    IDE_EXCEPTION( page_corruption_fatal_error );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_PageCorrupted,
                                  sSpaceID,
                                  sPageID ));
    }
    IDE_EXCEPTION_END;

    if( sState > 0 )
    {
        IDE_ASSERT( smuHash::close( &mCorruptedPages )
                    == IDE_SUCCESS );
    }

    setPageErrPolicyByProp();

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : redo log의 rid를 이용한 hash value를 생성
 *
 * space id와 page id를 적절히 변환하여 정수를 만들어 리턴한다.
 * 이 함수는 corrupted page에 대한 hash function으로 사용된다.
 * hash key는 GRID이다.
 *
 * aGRID - [IN] <spaceid, pageid, 0>로 corrupt page에 대한 grid이다.
 **********************************************************************/
UInt sdrCorruptPageMgr::genHashValueFunc( void* aGRID )
{
    IDE_DASSERT( aGRID != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isSystemMemTableSpace( SC_MAKE_SPACE(*(scGRID*)aGRID) )
                 == ID_FALSE );

    return ((UInt)SC_MAKE_SPACE(*(scGRID*)aGRID) +
            (UInt)SC_MAKE_PID(*(scGRID*)aGRID));
}


/***********************************************************************
 * Description : hash-key 비교함수
 *
 * 2개의 GRID가 같은지 비교한다. 같으면 0을 리턴한다.
 * 이 함수는 corrupted page 에 대한 hash compare function으로
 * 사용된다.
 *
 * aLhs - [IN] page의 grid로 <spaceid, pageid, 0>로 구성.
 * aRhs - [IN] page의 grid로 <spaceid, pageid, 0>로 구성.
 **********************************************************************/
SInt sdrCorruptPageMgr::compareFunc( void*  aLhs,
                                     void*  aRhs )
{
    IDE_DASSERT( aLhs != NULL );
    IDE_DASSERT( aRhs != NULL );

    scGRID  *sLhs = (scGRID *)aLhs;
    scGRID  *sRhs = (scGRID *)aRhs;

    return ( ( SC_MAKE_SPACE( *sLhs ) == SC_MAKE_SPACE( *sRhs ) ) &&
             ( SC_MAKE_PID( *sLhs )  == SC_MAKE_PID( *sRhs ) ) ? 0 : -1 );
}

/***********************************************************************
 * Description : page전체를 Overwrite하는 log인지 확인해 준다.
 * aLogType - [IN] 확인할 log의 logtype
 **********************************************************************/
idBool sdrCorruptPageMgr::isOverwriteLog( sdrLogType aLogType )
{
    idBool sResult = ID_FALSE;

    if( ( smuProperty::getCorruptPageErrPolicy()
          & SDR_CORRUPT_PAGE_ERR_POLICY_OVERWRITE )
        == SDR_CORRUPT_PAGE_ERR_POLICY_OVERWRITE )
    {
        if( ( aLogType == SDR_SDP_WRITE_PAGEIMG ) ||
            ( aLogType == SDR_SDP_WRITE_DPATH_INS_PAGE ) ||
            ( aLogType == SDR_SDP_INIT_PHYSICAL_PAGE) ||
            ( aLogType == SDR_SDP_PAGE_CONSISTENT ) )
        {
            sResult = ID_TRUE;
        }
    }

    return sResult;
}

/***********************************************************************
 * Description : corrupt page를 읽었을 때 서버를 종료시키지 않는다.
 **********************************************************************/
void sdrCorruptPageMgr::allowReadCorruptPage()
{
    if( ( smuProperty::getCorruptPageErrPolicy()
          & SDR_CORRUPT_PAGE_ERR_POLICY_OVERWRITE )
        == SDR_CORRUPT_PAGE_ERR_POLICY_OVERWRITE )
    {
        mCorruptPageReadPolicy = SDB_CORRUPTED_PAGE_READ_PASS;
    }
    else
    {
        mCorruptPageReadPolicy = SDB_CORRUPTED_PAGE_READ_ABORT;
    }
}

/***********************************************************************
 * Description : corrupt page를 읽었을 때 서버를 종료시킨다.
 **********************************************************************/
void sdrCorruptPageMgr::fatalReadCorruptPage()
{
    mCorruptPageReadPolicy = SDB_CORRUPTED_PAGE_READ_FATAL ;
}

/***********************************************************************
 * Description : 운영중 corrupt page를 읽었을 때 정책을 CorruptPageErrPolicy
 *              프로퍼티 값에 따라서 설정한다.(BUG-45598)
 **********************************************************************/
void sdrCorruptPageMgr::setPageErrPolicyByProp()
{
    if ( ( smuProperty::getCorruptPageErrPolicy()
           & SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL )
         == SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL )
    {
        mCorruptPageReadPolicy = SDB_CORRUPTED_PAGE_READ_FATAL;
    }
    else
    {
        mCorruptPageReadPolicy = SDB_CORRUPTED_PAGE_READ_ABORT;
    }
}


