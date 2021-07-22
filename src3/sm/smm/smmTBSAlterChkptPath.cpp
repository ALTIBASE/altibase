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
* $Id: smmTBSAlterChkptPath.cpp 19201 2006-11-30 00:54:40Z kmkim $
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
#include <smmTBSAlterChkptPath.h>
#include <smmTBSMultiPhase.h>

/*
  생성자 (아무것도 안함)
*/
smmTBSAlterChkptPath::smmTBSAlterChkptPath()
{

}


/*
    새로운 Checkpoint Path를 추가한다.

    aTrans     [IN] Transaction
    aSpaceID   [IN] Tablespace의 ID
    aChkptPath [IN] Tablespace에 추가할 Checkpoint Path

    [ 알고리즘 ]
      (010) CheckpointPathNode 할당
      (020) TBSNode 에 CheckpointPathNode 추가
      (030) Log Anchor Flush

    [ Checkpoint Path add도중 서버 사망하면? ]
      - 여러 벌의 Log Anchor중 일부가 부분 Flush되어
        inconsistent한 상태가 된다.
      - 서버 재기동시 consistent한 최신 Log Anchor를
        읽게 되므로 문제되지 않음.

    [ 에러처리 ]
      (e-010) CONTROL단계가 아니면 에러
      (e-020) 이미 Tablespace에 존재하는 Checkpoint Path이면 에러 
      (e-030) NewPath가 없는 디렉토리이면 에러
      (e-040) NewPath가 디렉토리가 아닌 파일이면 에러
      (e-050) NewPath에 read/write/execute permission이 없으면 에러

    [ 기타 ]
      - 기존의 Checkpoint에서 새로 추가된 Checkpoint Path로의 
        Checkpoint Image File 재분배를 하지 않는다.
      - Checkpoint Image가 갑자기 없어지면 DBA는 당황할 것이다.
      - Checkpoint Image File 재분배에 대한 Undo처리가 복잡하다.
      - 재분배가 필요한 경우 altibase 내리고 DBA가 직접처리
    
    [ 주의 ]
      여기에 인자로 넘어오는 Checkpoint Path 메모리는
      함수 호출 종료후 해제되기 때문에 그대로 사용하면 안된다.
      
*/
IDE_RC smmTBSAlterChkptPath::alterTBSaddChkptPath( scSpaceID          aSpaceID,
                                            SChar            * aChkptPath )
{
    smmTBSNode       * sTBSNode;
    smmChkptPathNode * sCPathNode = NULL;

    IDE_DASSERT( aChkptPath != NULL );
        
    //////////////////////////////////////////////////////////////////
    // (e-010) CONTROL단계가 아니면 에러
    // => smiTableSpace에서 이미 처리

    // Tablespace의 TBSNode를 얻어온다.
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(
                  aSpaceID,
                  (void**) & sTBSNode )
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////
    // (e-020) 이미 Tablespace에 존재하는 Checkpoint Path이면 에러
    IDE_TEST( smmTBSChkptPath::findChkptPathNode( sTBSNode,
                                                  aChkptPath,
                                                  & sCPathNode )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sCPathNode != NULL, err_chkpt_path_already_exists );
        
    //////////////////////////////////////////////////////////////////
    // (e-030) NewPath가 없는 디렉토리이면 에러
    // (e-040) NewPath가 디렉토리가 아닌 파일이면 에러
    // (e-050) NewPath에 read/write/execute permission이 없으면 에러
    IDE_TEST( smmTBSChkptPath::checkAccess2ChkptPath( aChkptPath )
              != IDE_SUCCESS );
    
    //////////////////////////////////////////////////////////////////
    // (010) CheckpointPathNode 할당
    IDE_TEST( smmTBSChkptPath::makeChkptPathNode( aSpaceID,
                                                  aChkptPath,
                                                  & sCPathNode )
              != IDE_SUCCESS );
    
    // (020) TBSNode 에 CheckpointPathNode 추가
    IDE_TEST( smmTBSChkptPath::addChkptPathNode( sTBSNode,
                                                 sCPathNode )
              != IDE_SUCCESS );
    
    //////////////////////////////////////////////////////////////////
    // (030) Log Anchor Flush
    IDE_TEST( smLayerCallback::addChkptPathNodeAndFlush( sCPathNode )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_chkpt_path_already_exists );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_ALREADY_EXISTS,
                                aChkptPath));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*
    기존의 Checkpoint Path를 변경한다.
    
    aTrans         [IN] Transaction
    aSpaceID       [IN] Tablespace의 ID
    aOrgChkptPath  [IN] 기존 Checkpoint Path
    aNewChkptPath  [IN] 새 Checkpoint Path

    [ 알고리즘 ]
      (010) CheckpointPathNode 검색 
      (020) CheckpointPath 변경 
      (030) Log Anchor Flush

    [ Checkpoint Path rename도중 서버 사망하면? ]
      - smmTBSAlterChkptPath::addChkptPath 참고
    
    [ 에러처리 ]
      (e-010) CONTROL단계가 아니면 에러
      (e-020) sNewChkptPath가 Tablespace에 이미 존재하면 에러 
      (e-030) aOrgChkptPath 가 aSpaceID에 해당하는 Tablespace에 없으면 에러
      (e-040) NewPath가 없는 디렉토리이면 에러
      (e-050) NewPath가 디렉토리가 아닌 파일이면 에러
      (e-060) NewPath에 read/write/execute permission이 없으면 에러

    [ 기타 ]
      - Checkpoint Image를 OldPath에서 NewPath로
        이동하는 처리는 하지 않는다. ( DBA가 직접 처리해야 함 )
      
    [ 주의 ]
      여기에 인자로 넘어오는 smiChkptPathAttr의 메모리는
      함수 호출 종료후 해제될 수 있으므로 그대로 사용하면 안된다.
      smiChkptPathAttr을 새로 할당하여 복사해야 한다.
*/
IDE_RC smmTBSAlterChkptPath::alterTBSrenameChkptPath( scSpaceID      aSpaceID,
                                               SChar        * aOrgChkptPath,
                                               SChar        * aNewChkptPath )
{
    smmTBSNode       * sTBSNode;
    smmChkptPathNode * sCPathNode = NULL;
    
    IDE_DASSERT( aOrgChkptPath != NULL );
    IDE_DASSERT( aNewChkptPath != NULL );

    //////////////////////////////////////////////////////////////////
    // (e-010) CONTROL단계가 아니면 에러
    // => smiTableSpace에서 이미 처리

    // Memory Dictionary Tablespace의 TBSNode를 얻어온다.
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(
                  aSpaceID,
                  (void**) & sTBSNode )
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////
    // (e-020) sNewChkptPath가 Tablespace에 이미 존재하면 에러 
    IDE_TEST( smmTBSChkptPath::findChkptPathNode( sTBSNode,
                                                  aNewChkptPath,
                                                  & sCPathNode )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sCPathNode != NULL, err_chkpt_path_node_already_exist );
    
    //////////////////////////////////////////////////////////////////////
    // (010) CheckpointPathNode 검색
    IDE_TEST( smmTBSChkptPath::findChkptPathNode( sTBSNode,
                                                  aOrgChkptPath,
                                                  & sCPathNode )
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////
    // (e-030) aOrgChkptPath 가 aSpaceID에 해당하는 Tablespace에 없으면 에러
    IDE_TEST_RAISE( sCPathNode == NULL, err_chkpt_path_node_not_found );

    //////////////////////////////////////////////////////////////////////
    // (e-040) NewPath가 없는 디렉토리이면 에러
    // (e-050) NewPath가 디렉토리가 아닌 파일이면 에러
    // (e-060) NewPath에 read/write/execute permission이 없으면 에러
    IDE_TEST( smmTBSChkptPath::checkAccess2ChkptPath( aNewChkptPath )
              != IDE_SUCCESS );
    
    //////////////////////////////////////////////////////////////////////
    // (020) CheckpointPath 변경
    IDE_TEST( smmTBSChkptPath::renameChkptPathNode( sCPathNode, aNewChkptPath )
              != IDE_SUCCESS );
    
    //////////////////////////////////////////////////////////////////////
    // (030) Log Anchor Flush
    IDE_TEST( smLayerCallback::updateChkptPathAndFlush( sCPathNode )
              != IDE_SUCCESS );
          
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_chkpt_path_node_already_exist );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_ALREADY_EXISTS,
                                aNewChkptPath));
    }
    IDE_EXCEPTION( err_chkpt_path_node_not_found );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_NODE_NOT_EXIST,
                                aOrgChkptPath));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
    기존의 Checkpoint Path를 제거한다.

    aTrans     [IN] Transaction
    aSpaceID   [IN] Tablespace의 ID
    aChkptPath [IN] Tablespace에서 제거할 Checkpoint Path

    [ 알고리즘 ]
      (010) CheckpointPathNode 검색 
      (020) CheckpointPath 제거 
      (030) Log Anchor Flush
    

    [ Checkpoint Path drop도중 서버 사망하면? ]
      - smmTBSAlterChkptPath::addChkptPath 참고

    [ 에러처리 ]
      (e-010) CONTROL단계가 아니면 에러 
      (e-020) OldPath가 해당 TBSNode에 CheckpointPath로 없으면 에러
      (e-030) 해당 TBS의 오직 하나뿐인 Checkpoint Path를
              DROP시도 하면 에러
      (e-040) DROP하려는 경로의 Checkpoint Image를 다른 유효한 경로로 옮겼는지 확인.
              DROP하려는 경로에 남아 있으면 에러

    [ 기타 ]
      - Checkpoint Image를 Drop한 Path에서
        Drop되지 않은 Path로 이동하는 처리는 하지 않는다. 
       ( DBA가 직접 처리해야 함 )
    
    주의 --
      여기에 인자로 넘어오는 smiChkptPathAttr의 메모리는
      함수 호출 종료후 해제될 수 있으므로 그대로 사용하면 안된다.
      smiChkptPathAttr을 새로 할당하여 복사해야 한다.
*/
IDE_RC smmTBSAlterChkptPath::alterTBSdropChkptPath( scSpaceID     aSpaceID,
                                                    SChar       * aChkptPath )
{
    smmTBSNode       * sTBSNode;
    smmChkptPathNode * sCPathNode = NULL;
    UInt               sCPathCount = 0;

    IDE_DASSERT( aChkptPath != NULL );

    //////////////////////////////////////////////////////////////////
    // (e-010) CONTROL단계가 아니면 에러
    // => smiTableSpace에서 이미 처리

    // Memory Dictionary Tablespace의 TBSNode를 얻어온다.
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(
                  aSpaceID,
                  (void**) & sTBSNode )
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////////
    // (010) CheckpointPathNode 검색 
    IDE_TEST( smmTBSChkptPath::findChkptPathNode( sTBSNode,
                                                  aChkptPath,
                                                  & sCPathNode )
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////////
    // (e-020) OldPath가 해당 TBSNode에 CheckpointPath로 없으면 에러
    IDE_TEST_RAISE( sCPathNode == NULL, err_chkpt_path_node_not_found );

    //////////////////////////////////////////////////////////////////////
    // (e-030) 해당 TBS의 오직 하나뿐인 Checkpoint Path를
    //         DROP시도 하면 에러
    IDE_TEST( smmTBSChkptPath::getChkptPathNodeCount( sTBSNode,
                                                      & sCPathCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sCPathCount == 1, err_trial_to_drop_the_last_chkpt_path );

    //////////////////////////////////////////////////////////////////////
    // (e-040)  BUG-28523 DROP하려는 경로의 Checkpoint Image File을
    //          다른 유효한 경로로 옮기지 않고 DROP시도 하면 에러
    IDE_TEST( smmDatabaseFile::checkChkptImgInDropCPath( sTBSNode,
                                                         sCPathNode )
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////////
    // (020) CheckpointPath 제거
    IDE_TEST( smmTBSChkptPath::removeChkptPathNode( sTBSNode, sCPathNode )
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////////
    // (030) Log Anchor Flush
    //
    // 모든 Tablespace의 Log Anchor정보를 새로 구축한다.
    // ( 제거된 Checkpoint Path Node를 제외한
    //   나머지 Checkpoint Path Node만 Log Anchor에 남겨두기 위함 )
    IDE_TEST( smLayerCallback::updateAnchorOfTBS() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_chkpt_path_node_not_found );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_NODE_NOT_EXIST,
                                aChkptPath));
    }
    IDE_EXCEPTION( err_trial_to_drop_the_last_chkpt_path );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_UNABLE_TO_DROP_LAST_CPATH,
                                aChkptPath));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

