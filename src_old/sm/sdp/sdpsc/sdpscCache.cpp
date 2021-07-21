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
 *
 * $Id: sdpscCache.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment의 Segment Runtime Cache에 대한
 * STATIC 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <sdpDef.h>

# include <sdpscCache.h>
# include <sdpscSH.h>

# include <sdpSegment.h>


/***********************************************************************
 *
 * Description : [ INTERFACE ] Segment Cache 초기화
 *
 * aSegHandle  - [IN] 세그먼트 핸들 포인터
 * aSpaceID    - [IN] 테이블스페이스 ID
 * aSegType    - [IN] 세그먼트 타입
 * aTableOID   - [IN] 세그먼트의 소유 객체의 OID (객체가 Table인 경우만)
 *
 ***********************************************************************/
IDE_RC sdpscCache::initialize( sdpSegHandle * aSegHandle,
                               scSpaceID      aSpaceID,
                               sdpSegType     aSegType,
                               smOID          aTableOID )
{
    sdpSegInfo          sSegInfo;
    sdpscSegCache     * sSegCache;
    UInt                sState  = 0;

    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aTableOID  == SM_NULL_OID );

    //TC/Server/LimitEnv/Bugs/BUG-24163/BUG-24163_2.sql
    //IDU_LIMITPOINT("sdpscCache::initialize::malloc");
    //[TODO] immediate로 변경할것.

    IDE_ASSERT( iduMemMgr::malloc( IDU_MEM_SM_SDP,
                                   ID_SIZEOF(sdpscSegCache),
                                   (void **)&(sSegCache),
                                   IDU_MEM_FORCE )
                == IDE_SUCCESS );
    sState  = 1;

    sdpSegment::initCommonCache( &sSegCache->mCommon,
                                 aSegType,
                                 0,  /* aPageCntInExt */
                                 aTableOID,
                                 SM_NULL_INDEX_ID);

    if ( aSegHandle->mSegPID != SD_NULL_PID )
    {
        IDE_TEST( sdpscSegHdr::getSegInfo( NULL, /* aStatistics */
                                           aSpaceID,
                                           aSegHandle->mSegPID,
                                           NULL, /* aTableHeader */
                                           &sSegInfo )
                  != IDE_SUCCESS );

        IDE_ASSERT( sSegInfo.mType == aSegType );

        sSegCache->mCommon.mPageCntInExt   = sSegInfo.mPageCntInExt;
        sSegCache->mCommon.mSegSizeByBytes =
            ((sSegInfo.mExtCnt * sSegInfo.mPageCntInExt )
             * SD_PAGE_SIZE);
    }

    aSegHandle->mCache       = (void*)sSegCache;
    aSegHandle->mSpaceID     = aSpaceID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( sSegCache ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SegCacheInfo를 얻어준다.
 *
 * aStatistics   - [IN] 통계 정보
 * aSegHandle    - [IN] Segment Handle
 *
 * aSegCacheInfo - [OUT] SegCacheInfo가 설정된다.
 *
 ***********************************************************************/
IDE_RC sdpscCache::getSegCacheInfo( idvSQL          * /*aStatistics*/,
                                    sdpSegHandle    * /*aSegHandle*/,
                                    sdpSegCacheInfo * aSegCacheInfo )
{
    IDE_ASSERT( aSegCacheInfo != NULL );

    aSegCacheInfo->mUseLstAllocPageHint = ID_FALSE;
    aSegCacheInfo->mLstAllocPID         = SD_NULL_PID;
    aSegCacheInfo->mLstAllocSeqNo       = 0;

    return IDE_SUCCESS;
}

IDE_RC sdpscCache::setLstAllocPage( idvSQL          * /*aStatistics*/,
                                    sdpSegHandle    * /*aSegHandle*/,
                                    scPageID          /*aLstAllocPID*/,
                                    ULong             /*aLstAllocSeqNo*/ )
{
    /* TMS가 아니면 아무것도 하지 않는다. */
    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description : [ INTERFACE ] Segment Cache 해제
 *
 * aSegHandle  - [IN] 세그먼트 핸들 포인터
 *
 ***********************************************************************/
IDE_RC sdpscCache::destroy( sdpSegHandle * aSegHandle )
{
    IDE_ASSERT( aSegHandle != NULL );

    // Segment Cache를 메모리 해제한다.
    IDE_ASSERT( iduMemMgr::free( aSegHandle->mCache ) == IDE_SUCCESS );
    aSegHandle->mCache = NULL;

    return IDE_SUCCESS;
}
