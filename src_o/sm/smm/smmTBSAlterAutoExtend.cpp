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
* $Id: smmTBSAlterAutoExtend.cpp 19201 2006-11-30 00:54:40Z kmkim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smu.h>
#include <smm.h>
#include <smmReq.h>
#include <sctTableSpaceMgr.h>
#include <smmTBSAlterAutoExtend.h>
#include <smmTBSMultiPhase.h>

/*
  생성자 (아무것도 안함)
*/
smmTBSAlterAutoExtend::smmTBSAlterAutoExtend()
{

}



/*
    ALTER TABLESPACE AUTOEXTEND ... 의 실행 실시 
    
    aTrans      [IN] Transaction
    aSpaceID    [IN] Tablespace의 ID
    aAutoExtend [IN] 사용자가 지정한 Auto Extend 여부
                     ID_TRUE => Auto Extend 실시
    aNextSize   [IN] 사용자가 지정한 NEXT (자동확장)크기 ( byte단위 )
                     지정하지 않은 경우 0
    aMaxSize    [IN] 사용자가 지정한 MAX (최대)크기 ( byte단위 )
                     지정하지 않은 경우 0
                     UNLIMITTED로 지정한 경우 ID_ULONG_MAX

    [ 요구사항 ]
      - Auto Extend On/Off시 NEXT와 MAXSIZE 의 설정 ( Oracle과 동일 )
        - Off로 변경시 NEXT와 MAXSIZE를 0으로 설정
        - On으로 변경시 NEXT와 MAXSIZE를 시스템 기본값으로 설정
          - 사용자가 NEXT와 MAXSIZE를 별도 지정한 경우, 지정한 값으로 설정
    
    [ 알고리즘 ]
      (010) lock TBSNode in X
      (020) NextPageCount, MaxPageCount 계산 
      (030) 로깅실시 => ALTER_TBS_AUTO_EXTEND
      (040) AutoExtendMode, NextSize, MaxSize 변경 
      (050) Tablespace Node를 Log Anchor에 Flush!
      
    [ ALTER_TBS_AUTO_EXTEND 의 REDO 처리 ]
      (r-010) TBSNode.AutoExtend := AfterImage.AutoExtend
      (r-020) TBSNode.NextSize   := AfterImage.NextSize
      (r-030) TBSNode.MaxSize    := AfterImage.MaxSize

    [ ALTER_TBS_AUTO_EXTEND 의 UNDO 처리 ]
      (u-010) 로깅실시 -> CLR ( ALTER_TBS_AUTO_EXTEND )
      (u-020) TBSNode.AutoExtend := BeforeImage.AutoExtend
      (u-030) TBSNode.NextSize   := BeforeImage.NextSize
      (u-040) TBSNode.MaxSize    := BeforeImage.MaxSize
*/
IDE_RC smmTBSAlterAutoExtend::alterTBSsetAutoExtend( void      * aTrans,
                                                     scSpaceID   aTableSpaceID,
                                                     idBool      aAutoExtendMode,
                                                     ULong       aNextSize,
                                                     ULong       aMaxSize )
{
    smmTBSNode        * sTBSNode;
    smiTableSpaceAttr * sTBSAttr;
    
    scPageID            sNextPageCount;
    scPageID            sMaxPageCount;

    IDE_DASSERT( aTrans != NULL );

    ///////////////////////////////////////////////////////////////////////////
    // Tablespace ID로부터 Node를 가져온다
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sTBSNode)
                  != IDE_SUCCESS );

    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace( sTBSNode ) == ID_TRUE );

    sTBSAttr = & sTBSNode->mTBSAttr;

    ///////////////////////////////////////////////////////////////////////////
    // (010) lock TBSNode in X
    //       DROPPED,DISCARD,OFFLINE 인 경우 여기에서 에러가 발생한다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode( 
                                   aTrans,
                                   & sTBSNode->mHeader,
                                   ID_FALSE,   /* intent */
                                   ID_TRUE,    /* exclusive */
                                   SCT_VAL_DDL_DML) /* validation */
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    // (e-010) 이미 AUTOEXTEND == ON  인데 또다시 ON설정시 에러
    // (e-020) 이미 AUTOEXTEND == OFF 인데 또다시 OFF설정시 에러
    // (e-030) NextSize가 EXPAND_CHUNK_PAGE_COUNT*SM_PAGE_SIZE로
    //         나누어 떨어지지 않으면 에러
    // (e-040) Tablespace의 현재크기 > MAXSIZE 이면 에러
    IDE_TEST( checkErrorOnAutoExtendAttrs( sTBSNode,
                                           aAutoExtendMode,
                                           aNextSize,
                                           aMaxSize )
              != IDE_SUCCESS );


    ///////////////////////////////////////////////////////////////////////////
    // (020)   NextPageCount, MaxPageCount 계산 
    IDE_TEST( calcAutoExtendAttrs( sTBSNode,
                                   aAutoExtendMode,
                                   aNextSize,
                                   aMaxSize,
                                   & sNextPageCount,
                                   & sMaxPageCount )
              != IDE_SUCCESS );
    

    ///////////////////////////////////////////////////////////////////////////
    // (030) 로깅실시 => ALTER_TBS_AUTO_EXTEND
    IDE_TEST( smLayerCallback::writeMemoryTBSAlterAutoExtend ( NULL, /* idvSQL* */
                                                               aTrans,
                                                               aTableSpaceID,
                                                               /* Before Image */
                                                               sTBSAttr->mMemAttr.mIsAutoExtend,
                                                               sTBSAttr->mMemAttr.mNextPageCount,
                                                               sTBSAttr->mMemAttr.mMaxPageCount,
                                                               /* After Image */
                                                               aAutoExtendMode,
                                                               sNextPageCount,
                                                               sMaxPageCount )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    // (040) AutoExtendMode, NextSize, MaxSize 변경 
    sTBSAttr->mMemAttr.mIsAutoExtend  = aAutoExtendMode;
    sTBSAttr->mMemAttr.mNextPageCount = sNextPageCount;
    sTBSAttr->mMemAttr.mMaxPageCount  = sMaxPageCount;


    ///////////////////////////////////////////////////////////////////////////
    // (050) Tablespace Node를 Log Anchor에 Flush!
    IDE_TEST( smLayerCallback::updateTBSNodeAndFlush( & sTBSNode->mHeader ) 
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    // (010)에서 획득한 Tablespace X Lock은 UNDO완료후 자동으로 풀게된다
    // 여기서 별도 처리할 필요 없음

    return IDE_FAILURE;
}

/*
    ALTER TABLESPACE AUTOEXTEND ... 에 대한 에러처리

    [ 에러처리 ]
      (e-010) TBSNode.state 가 DROPPED이면 에러
      (e-020) TBSNode.state 가 OFFLINE이면 에러 
      (e-050) NextSize가 EXPAND_CHUNK_PAGE_COUNT*SM_PAGE_SIZE로
              나누어 떨어지지 않으면 에러
      (e-060) Tablespace의 현재크기 > MAXSIZE 이면 에러
      
    [ 선결조건 ]
      aTBSNode에 해당하는 Tablespace에 X락이 잡혀있는 상태여야 한다.
*/
IDE_RC smmTBSAlterAutoExtend::checkErrorOnAutoExtendAttrs( smmTBSNode * aTBSNode,
                                                   idBool       aAutoExtendMode,
                                                   ULong        aNextSize,
                                                   ULong        aMaxSize )
{
    scPageID            sChunkPageCount;
    ULong               sCurrTBSSize;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aTBSNode->mMemBase != NULL );
    
    ///////////////////////////////////////////////////////////////////////////
    // (e-010) TBSNode.state 가 DROPPED이면 에러 
    IDE_TEST_RAISE( SMI_TBS_IS_DROPPED(aTBSNode->mHeader.mState),
                    error_dropped_tbs );

    ///////////////////////////////////////////////////////////////////////////
    // (e-020) TBSNode.state 가 OFFLINE이면 에러 
    IDE_TEST_RAISE( SMI_TBS_IS_OFFLINE(aTBSNode->mHeader.mState),
                    error_offline_tbs );
    
    sChunkPageCount = smmDatabase::getExpandChunkPageCnt( aTBSNode->mMemBase );

    if ( aNextSize != ID_ULONG_MAX ) // 사용자가 Next Size를 지정한 경우 
    {
        ////////////////////////////////////////////////////////////////////////
        // (e-050) NextSize가 EXPAND_CHUNK_PAGE_COUNT*SM_PAGE_SIZE로
        //         나누어 떨어지지 않으면 에러
        IDE_TEST_RAISE( (aNextSize % ( sChunkPageCount * SM_PAGE_SIZE )) != 0,
                        error_alter_tbs_nextsize_not_aligned_to_chunk_size );
    }

    if ( ( aMaxSize != ID_ULONG_MAX ) && // 사용자가 Max Size를 지정한 경우 
         ( aMaxSize != 0 ) )             // MAXSIZE UNLIMITTED가 아닌 경우 
    {
        ////////////////////////////////////////////////////////////////////////
        // (e-060) Tablespace의 현재크기 > MAXSIZE 이면 에러
        sCurrTBSSize = smmDatabase::getAllocPersPageCount( aTBSNode->mMemBase )
                       * SM_PAGE_SIZE ;

        // META PAGE의 수는 MAXSIZE산정시 제외한다.
        // 이유 : Tablespace생성시 사용자가 지정한 초기 SIZE를
        //         META PAGE를 포함하지 않는 순수 데이터 Page 수를
        //         계산하는데 사용했기 때문.
        //
        //         초기 SIZE와 MAXSIZE를 같은 정책을 써야
        //         다음과 같은 Tablespace생성 구만에서 에러가 나지 않는다 
        //
        //         CREATE MEMORY TABLESPACE SIZE 8M MAXSIZE 8M
        sCurrTBSSize -= SMM_DATABASE_META_PAGE_CNT * SM_PAGE_SIZE ;
        
        IDE_TEST_RAISE( (aAutoExtendMode == ID_TRUE) &&
                        ( sCurrTBSSize > aMaxSize),
                        error_maxsize_lessthan_current_size );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_alter_tbs_nextsize_not_aligned_to_chunk_size )
    {
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_ALTER_TBS_NEXTSIZE_NOT_ALIGNED_TO_CHUNK_SIZE,
                    /* K byte 단위의 Expand Chunk크기 */
                    (ULong) ( (ULong)sChunkPageCount * SM_PAGE_SIZE / 1024)
                    ));
    }
    IDE_EXCEPTION( error_maxsize_lessthan_current_size );
    {
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_ALTER_TBS_MAXSIZE_LESSTHAN_CURRENT_SIZE,
                    /* K byte 단위의 Current Tablespace Size */
                    (ULong) ( sCurrTBSSize / 1024 )
                    ));
    }
    IDE_EXCEPTION( error_dropped_tbs );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ALTER_TBS_AT_DROPPED_TBS));
    }
    IDE_EXCEPTION( error_offline_tbs );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ALTER_TBS_AT_OFFLINE_TBS));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

/*
    Next Page Count 와 Max Page Count를 계산한다.
    
    aMemBase    [IN] Tablespace의 0번 Page에 존재하는 Membase
    aAutoExtend [IN] 사용자가 지정한 Auto Extend 여부
                     ID_TRUE => Auto Extend 실시
    aNextSize   [IN] 사용자가 지정한 NEXT (자동확장)크기 ( byte단위 )
                     지정하지 않은 경우 0
    aMaxSize    [IN] 사용자가 지정한 MAX (최대)크기 ( byte단위 )
                     지정하지 않은 경우 0
                     UNLIMITTED로 지정한 경우 ID_ULONG_MAX

    aNextPageCount [OUT] Tablespace의 자동확장 단위 ( SM Page 갯수 )
    aMaxPageCount  [OUT] Tablespace의 최대 크기      ( SM Page 갯수 )


    [ 알고리즘 ]
    
      if AUTO EXTEND == ON
         (010) NewNextSize := DEFAULT Next Size 혹은 사용자가 지정한 값
         (020) NewMaxSize  := DEFAULT Max Size  혹은 사용자가 지정한 값
      else // AUTO EXTEND OFF
         (030) NewNextSize := 0
         (040) NewMaxSize  := 0
      fi
    
    

    [ 선결조건 ]
      aTBSNode에 해당하는 Tablespace에 X락이 잡혀있는 상태여야 한다.
 */
IDE_RC smmTBSAlterAutoExtend::calcAutoExtendAttrs(
                          smmTBSNode * aTBSNode,
                          idBool       aAutoExtendMode,
                          ULong        aNextSize,
                          ULong        aMaxSize,
                          scPageID   * aNextPageCount,
                          scPageID   * aMaxPageCount )
{
    scPageID sNextPageCount;
    scPageID sMaxPageCount;
    
    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aNextPageCount != NULL );
    IDE_DASSERT( aMaxPageCount != NULL );
    
    if ( aAutoExtendMode == ID_TRUE ) /* Auto Extend On */
    {
        // Oracle과 동일하게 처리
        // Auto Extend On시 Next/Max Size를 지정하지 않았다면
        // 시스템의 기본값을 사용

        // 사용자가 Next Size를 지정하지 않은 경우
        if ( aNextSize == ID_ULONG_MAX ) 
        {
            // 기본값 => EXPAND CHUNK의 크기 
            sNextPageCount =
                smmDatabase::getExpandChunkPageCnt( aTBSNode->mMemBase );
        }
        else                  // 사용자가 Next Size를 지정한 경우 
        {
            sNextPageCount = aNextSize / SM_PAGE_SIZE ;
        }
        
        // 사용자가 Max Size를 지정하지 않은 경우
        if ( aMaxSize == ID_ULONG_MAX ) 
        {
            // 기본값 => UNLIMITTED
            sMaxPageCount = SM_MAX_PID + 1;
        }
        else
        {
            if ( aMaxSize == 0 ) // MAXSIZE UNLIMITTED인 경우 
            {
                // 최대 PAGE COUNT로 지정 
                sMaxPageCount = SM_MAX_PID + 1;
            }
            else                 // 사용자가 Max Size를 지정한 경우 
            {
                sMaxPageCount = aMaxSize / SM_PAGE_SIZE ;
            }
        }
    } 
    else /* Auto Extend Off */
    {
        // Oracle과 동일하게 처리
        // Auto Extend Off시 Next/Max Size를 0으로 초기화 
        sNextPageCount = 0;
        sMaxPageCount = 0;
    }
    
    *aNextPageCount = sNextPageCount;
    *aMaxPageCount = sMaxPageCount;

    return IDE_SUCCESS;

}

