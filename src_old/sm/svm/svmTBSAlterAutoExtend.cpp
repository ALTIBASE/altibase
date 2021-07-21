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
#include <smDef.h>
#include <smErrorCode.h>
#include <smu.h>
#include <svmReq.h>
#include <sctTableSpaceMgr.h>
#include <svmDatabase.h>
#include <svmManager.h>
#include <svmTBSAlterAutoExtend.h>

/*
  생성자 (아무것도 안함)
*/
svmTBSAlterAutoExtend::svmTBSAlterAutoExtend()
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
IDE_RC svmTBSAlterAutoExtend::alterTBSsetAutoExtend(void      * aTrans,
                                            scSpaceID   aTableSpaceID,
                                            idBool      aAutoExtendMode,
                                            ULong       aNextSize,
                                            ULong       aMaxSize )
{
    svmTBSNode        * sTBSNode;
    smiTableSpaceAttr * sTBSAttr;
    
    scPageID            sNextPageCount;
    scPageID            sMaxPageCount;
    
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isVolatileTableSpace( aTableSpaceID ) == ID_TRUE );

    ///////////////////////////////////////////////////////////////////////////
    // Tablespace ID로부터 Node를 가져온다
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sTBSNode )
                  != IDE_SUCCESS );

    sTBSAttr = & sTBSNode->mTBSAttr;

    ///////////////////////////////////////////////////////////////////////////
    // (010) lock TBSNode in X
    
    // BUGBUG-1548 check for lock timeout
    IDE_TEST( sctTableSpaceMgr::lockTBSNode(
                                   aTrans,
                                   &sTBSNode->mHeader,
                                   ID_FALSE,   /* intent */
                                   ID_TRUE,    /* exclusive */
                                   ID_ULONG_MAX, /* lock wait micro sec */
                                   SCT_VAL_DDL_DML,
                                   NULL,       /* is locked */
                                   NULL ) /* lockslot */
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    // (e-010) TBSNode.state 가 DROPPED이면 에러 
    // (e-020) TBSNode.state 가 OFFLINE이면 에러 
    // (e-030) 이미 AUTOEXTEND == ON  인데 또다시 ON설정시 에러
    // (e-040) 이미 AUTOEXTEND == OFF 인데 또다시 OFF설정시 에러
    // (e-050) NextSize가 EXPAND_CHUNK_PAGE_COUNT*SM_PAGE_SIZE로
    //         나누어 떨어지지 않으면 에러
    // (e-060) Tablespace의 현재크기 > MAXSIZE 이면 에러
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
    IDE_TEST( smLayerCallback::writeVolatileTBSAlterAutoExtend ( NULL, /* idvSQL* */
                                                                 aTrans,
                                                                 aTableSpaceID,
                                                                 /* Before Image */
                                                                 sTBSAttr->mVolAttr.mIsAutoExtend,
                                                                 sTBSAttr->mVolAttr.mNextPageCount,
                                                                 sTBSAttr->mVolAttr.mMaxPageCount,
                                                                 /* After Image */
                                                                 aAutoExtendMode,
                                                                 sNextPageCount,
                                                                 sMaxPageCount )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    // (040) AutoExtendMode, NextSize, MaxSize 변경 
    sTBSAttr->mVolAttr.mIsAutoExtend  = aAutoExtendMode;
    sTBSAttr->mVolAttr.mNextPageCount = sNextPageCount;
    sTBSAttr->mVolAttr.mMaxPageCount  = sMaxPageCount;
        
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
IDE_RC svmTBSAlterAutoExtend::checkErrorOnAutoExtendAttrs( svmTBSNode * aTBSNode,
                                                   idBool       aAutoExtendMode,
                                                   ULong        aNextSize,
                                                   ULong        aMaxSize )
{
    scPageID            sChunkPageCount;
    ULong               sCurrTBSSize;

    IDE_DASSERT( aTBSNode != NULL );
        
    ///////////////////////////////////////////////////////////////////////////
    // (e-010) TBSNode.state 가 DROPPED이면 에러 
    IDE_TEST_RAISE( SMI_TBS_IS_DROPPED(aTBSNode->mHeader.mState),
                    error_dropped_tbs );

    ///////////////////////////////////////////////////////////////////////////
    // (e-020) TBSNode.state 가 OFFLINE이면 에러 
    IDE_TEST_RAISE( SMI_TBS_IS_OFFLINE(aTBSNode->mHeader.mState),
                    error_offline_tbs );
    
    sChunkPageCount = svmDatabase::getExpandChunkPageCnt( &aTBSNode->mMemBase );

    if ( aNextSize != ID_ULONG_MAX ) // 사용자가 Next Size를 지정한 경우
    {
        ////////////////////////////////////////////////////////////////////////
        // (e-050) NextSize가 EXPAND_CHUNK_PAGE_COUNT*SM_PAGE_SIZE로
        //         나누어 떨어지지 않으면 에러
        IDE_TEST_RAISE( (aNextSize % ( sChunkPageCount * SM_PAGE_SIZE )) != 0,
                        error_alter_tbs_nextsize_not_aligned_to_chunk_size );
    }

    if ( (aMaxSize != ID_ULONG_MAX) && // 사용자가 Max Size를 지정한 경우
         (aMaxSize != 0 ) )            // MAXSIZE UNLIMITED가 아닌 경우
    {
        ////////////////////////////////////////////////////////////////////////
        // (e-060) Tablespace의 현재크기 > MAXSIZE 이면 에러
        sCurrTBSSize = svmDatabase::getAllocPersPageCount( &aTBSNode->mMemBase )
                       * SM_PAGE_SIZE ;

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
IDE_RC svmTBSAlterAutoExtend::calcAutoExtendAttrs(
                          svmTBSNode * aTBSNode,
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
        if ( aNextSize == ID_ULONG_MAX ) // 사용자가 Next Size를 지정하지 않은 경우
        {
            // 기본값 => EXPAND CHUNK의 크기 
            sNextPageCount =
                svmDatabase::getExpandChunkPageCnt( &aTBSNode->mMemBase );
        }
        else                  // 사용자가 Next Size를 지정한 경우 
        {
            sNextPageCount = aNextSize / SM_PAGE_SIZE ;
        }
        
        
        // 사용자가 Max Size를 지정하지 않은 경우  또는 UNLIMITED로 지정한 경우
        if ( (aMaxSize == ID_ULONG_MAX) || (aMaxSize == 0) )
        {
            // 기본값 => UNLIMITTED
            sMaxPageCount = SM_MAX_PID + 1;
        }
        else
        {
            sMaxPageCount = aMaxSize / SM_PAGE_SIZE ;
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

