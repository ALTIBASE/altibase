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
* $Id: smmTBSChkptPath.cpp 19201 2006-11-30 00:54:40Z kmkim $
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
#include <smmTBSChkptPath.h>
#include <smmTBSMultiPhase.h>

/*
  생성자 (아무것도 안함)
*/
smmTBSChkptPath::smmTBSChkptPath()
{

}


/*
  PRJ-1548 User Memory TableSpace 개념도입 
  Checkpoint Path Attribute에 Checkpoint Path를 설정한다.
    
  [IN] aCPathAttr - Checkpoint Path Attribute
  [IN] aChkptPath - Checkpoint Path 문자열 
*/
IDE_RC smmTBSChkptPath::setChkptPath( smiChkptPathAttr * aCPathAttr,
                                    SChar            * aChkptPath )
{
#if defined(VC_WIN32)
    sctTableSpaceMgr::adjustFileSeparator( aChkptPath );
#endif

    idlOS::strncpy( aCPathAttr->mChkptPath,
                    aChkptPath,
                    SMI_MAX_CHKPT_PATH_NAME_LEN );
    
    aCPathAttr->mChkptPath[SMI_MAX_CHKPT_PATH_NAME_LEN] = '\0';
    
    return IDE_SUCCESS ;
}


/*
    Checkpoint Path Attribute를 초기화한다.
    
    [IN] aCPathAttr - 초기화할 Checkpoint Path Attribute
    [IN] aSpaceID   - 초기화할 Checkpoint Path Attribute가
                      속하는 Tablespace의 ID
    [IN] aChkptPath - 초기화할 Checkpoint Path Attribute가 지닐 Path
 */
IDE_RC smmTBSChkptPath::initializeChkptPathAttr(
                          smiChkptPathAttr * aCPathAttr,
                          scSpaceID          aSpaceID,
                          SChar            * aChkptPath )
{

    aCPathAttr->mAttrType   = SMI_CHKPTPATH_ATTR;
    aCPathAttr->mSpaceID    = aSpaceID;
    
    IDE_TEST( setChkptPath( aCPathAttr, aChkptPath ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
    Checkpoint Path Node를 초기화한다.
    
    [IN] aCPathNode - 초기화할 Checkpoint Path Node
    [IN] aSpaceID   - 초기화할 Checkpoint Path Attribute가 속하는
                      Tablespace의 ID
    [IN] aChkptPath - 초기화할 Checkpoint Path Attribute가 지닐 Path
    
 */
IDE_RC smmTBSChkptPath::initializeChkptPathNode(
                          smmChkptPathNode * aCPathNode,
                          scSpaceID          aSpaceID,
                          SChar            * aChkptPath )
{

    IDE_TEST( initializeChkptPathAttr( & aCPathNode->mChkptPathAttr,
                                       aSpaceID,
                                       aChkptPath )
              != IDE_SUCCESS );

    aCPathNode->mAnchorOffset = SCT_UNSAVED_ATTRIBUTE_OFFSET;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
     Checkpoint Path Node를 파괴한다.
  
    [IN] aCPathNode - 파괴할 Checkpoint Path Node
 */
IDE_RC smmTBSChkptPath::destroyChkptPathNode(smmChkptPathNode * aCPathNode )
{
    // 아무 것도 할 것이 없다.
    // 단지 Checkpoint Path의 이름을 destroy되었다고 저장해둔다.

    IDE_TEST( setChkptPath( & aCPathNode->mChkptPathAttr,
                            (SChar*)"destroyedTBS" )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

    

/*
    Checkpoint Path Node를 할당하고 초기화한다.

    [IN]  aSpaceID   - Checkpoint Path를 지니게 될 Tablespace의 ID
    [IN]  aChkptPath - Checkpoint Path ( 디렉토리 경로 문자열 )
    [OUT] aCPathNode - 생
 */
IDE_RC smmTBSChkptPath::makeChkptPathNode( scSpaceID           aSpaceID,
                                           SChar             * aChkptPath,
                                           smmChkptPathNode ** aCPathNode )
{
    UInt                sState = 0;
    smmChkptPathNode  * sCPathNode;
    
    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace( aSpaceID ) == ID_TRUE );
    IDE_DASSERT( aChkptPath != NULL );

    /* smmTBSChkptPath_makeChkptPathNode_calloc_CPathNode.tc */
    IDU_FIT_POINT("smmTBSChkptPath::makeChkptPathNode::calloc::CPathNode");
    IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SMM,
                                1,
                                ID_SIZEOF(smmChkptPathNode),
                                (void**)&(sCPathNode))
              != IDE_SUCCESS);
    sState = 1;
    
    IDE_TEST( initializeChkptPathNode( sCPathNode,
                                       aSpaceID,
                                       aChkptPath )
              != IDE_SUCCESS );

    *aCPathNode = sCPathNode;
    sState = 0;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1 :
            IDE_ASSERT( iduMemMgr::free( sCPathNode ) == IDE_SUCCESS );
            sCPathNode = NULL;
            break;
            
        default:
            break;
    }
    
    IDE_POP();
    
    
    return IDE_FAILURE;
}




/*
 * Checkpoint Path Node를 특정 Tablespace에 추가한다. 
 *
 * aTBSNode       [IN] - Checkpoint Path Node가 추가될 Tablespace
 * aChkptPathNode [IN] - 추가할 Checkpoint Path Node
 */
IDE_RC smmTBSChkptPath::addChkptPathNode( smmTBSNode       * aTBSNode,
                                        smmChkptPathNode * aChkptPathNode )
{
    UInt        sState  = 0;
    smuList   * sNewNode;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aChkptPathNode != NULL );
    
    
    IDE_DASSERT( aTBSNode->mHeader.mID ==
                 aChkptPathNode->mChkptPathAttr.mSpaceID );
    
    /* smmTBSChkptPath_addChkptPathNode_calloc_NewNode.tc */
    IDU_FIT_POINT("smmTBSChkptPath::addChkptPathNode::calloc::NewNode");
    IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMM,
                               1,
                               ID_SIZEOF(smuList),
                               (void**)&(sNewNode))
             != IDE_SUCCESS);
    sState = 1;

    sNewNode->mData = aChkptPathNode;
    
    SMU_LIST_ADD_LAST( & aTBSNode->mChkptPathBase,
                       sNewNode );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
 
    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sNewNode ) == IDE_SUCCESS );
            sNewNode = NULL;
        default:
            break;
    }
   
    return IDE_FAILURE;
}

/*
    Checkpoint Path Node와 Checkpoint Path 문자열을 비교하여
    같은 Checkpoint Path인지 체크한다.

    [IN] aCPathNode - 비교할 Checkpoint Path Node
    [IN] aChkptPath - 비교할 Checkpoint Path 문자열
 */
idBool smmTBSChkptPath::isSameChkptPath(smmChkptPathNode * aCPathNode,
                                      SChar            * aChkptPath)
{
    idBool sIsSame;

#if defined(VC_WIN32)
    sctTableSpaceMgr::adjustFileSeparator( aCPathNode->mChkptPathAttr.mChkptPath );
    sctTableSpaceMgr::adjustFileSeparator( aChkptPath );
#endif

    if ( idlOS::strcmp( aCPathNode->mChkptPathAttr.mChkptPath,
                        aChkptPath ) == 0 )
    {
        sIsSame = ID_TRUE;
    }
    else
    {
        sIsSame = ID_FALSE;
    }

    return sIsSame;
    
}


/*
 * 특정 Tablespace에서 특정 Checkpoint Path Node를 찾는다.
 *
 * aTBSNode       [IN] - Checkpoint Path Node를 찾을 Tablespace
 * aChkptPath     [IN] - 찾고자 하는 Checkpoint Path 문자열
 * aChkptPathNode [IN] - 찾아낸 Checkpoint Path Node
 */  
IDE_RC smmTBSChkptPath::findChkptPathNode( smmTBSNode        * aTBSNode,
                                         SChar             * aChkptPath,
                                         smmChkptPathNode ** aChkptPathNode )
{
    smmChkptPathNode * sCPathNode;
    smuList          * sListNode;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aChkptPath != NULL );
    IDE_DASSERT( aChkptPathNode != NULL );
    
    * aChkptPathNode = NULL ;
    
    sListNode = SMU_LIST_GET_FIRST( & aTBSNode->mChkptPathBase );

    while ( sListNode != & aTBSNode->mChkptPathBase )
    {
        sCPathNode = (smmChkptPathNode*)  sListNode->mData;
        
        if ( isSameChkptPath( sCPathNode,
                              aChkptPath ) == ID_TRUE )
        {
            * aChkptPathNode = sCPathNode;
            break;
        }
            
        sListNode = SMU_LIST_GET_NEXT( sListNode );
    }

    return IDE_SUCCESS;

}


/*
 * Checkpoint Path Node를 특정 Tablespace에서 제거한다.
 *
 * aTBSNode       [IN] - Checkpoint Path Node가 제거될 Tablespace
 * aChkptPathNode [IN] - 제거할 Checkpoint Path Node
 */  
IDE_RC smmTBSChkptPath::removeChkptPathNode(
                          smmTBSNode       * aTBSNode,
                          smmChkptPathNode * aChkptPathNode )
{
    smmChkptPathNode * sCPathNode;
    smuList          * sListNode;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aChkptPathNode != NULL );
    
    sListNode = SMU_LIST_GET_FIRST( & aTBSNode->mChkptPathBase );

    // 최소한 하나의 Checkpoint Path가 있어야 한다.
    IDE_DASSERT( sListNode != & aTBSNode->mChkptPathBase );

    while ( sListNode !=  & aTBSNode->mChkptPathBase )
    {
        sCPathNode = (smmChkptPathNode*)  sListNode->mData;
        
        if (  sCPathNode == aChkptPathNode )
        {

            IDE_TEST( destroyChkptPathNode( sCPathNode ) != IDE_SUCCESS );

            // Chkpt Path List 에서 Node의 제거도중
            // Checkpoint Path Count 감소하기 전에 에러가 발생해서는 안된다
            IDE_ASSERT( iduMemMgr::free( sCPathNode ) == IDE_SUCCESS );
            
            SMU_LIST_DELETE( sListNode );

            IDE_ASSERT( iduMemMgr::free( sListNode ) == IDE_SUCCESS );

            break;
        }
            
        sListNode = SMU_LIST_GET_NEXT( sListNode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*
 * Checkpoint Path Node의 Checkpoint Path를 변경한다.
 *
 * aChkptPathNode [IN] - 변경할 Checkpoint Path Node
 * aChkptPath     [IN] - 새로 설정할 Checkpoint Path 문자열 
 */  
IDE_RC smmTBSChkptPath::renameChkptPathNode(
                          smmChkptPathNode * aChkptPathNode,
                          SChar            * aChkptPath )
{
    IDE_DASSERT( aChkptPathNode != NULL );
    IDE_DASSERT( aChkptPath != NULL );

   IDE_TEST(  setChkptPath( & aChkptPathNode->mChkptPathAttr,
                            aChkptPath )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*
 * Checkpoint Path Node의 수를 리턴한다.
 *
 * [IN]  aTBSNode        - Checkpoint Path의 수를 Count할 Tablespace
 * [OUT] aChkptPathCount - Checkpoint Path의 수 
 */  
IDE_RC smmTBSChkptPath::getChkptPathNodeCount( smmTBSNode * aTBSNode,
                                             UInt       * aChkptPathCount )
{
    smuList  * sListNode;
    UInt       sNodeCount = 0;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aChkptPathCount != NULL );
    
    sListNode = SMU_LIST_GET_FIRST( & aTBSNode->mChkptPathBase );

    while ( sListNode != & aTBSNode->mChkptPathBase )
    {
        sNodeCount ++;
        sListNode = SMU_LIST_GET_NEXT( sListNode );
    }

    * aChkptPathCount = sNodeCount;

    return IDE_SUCCESS;

}

/*
   N번째 Checkpoint Path Node를 리턴한다.
   
   [IN]  aTBSNode   - Checkpoint Path를 검색할 Tablespace Node
   [IN]  aIndex     - 몇번째 Checkpoint Path를 리턴할지 (0-based index)
   [OUT] aCPathNode - 검색한 Checkpoint Path Node
 */
IDE_RC smmTBSChkptPath::getChkptPathNodeByIndex(
                          smmTBSNode        * aTBSNode,
                          UInt                aIndex,
                          smmChkptPathNode ** aCPathNode )
{
    smuList  * sListNode;
    UInt       sNodeIndex = 0;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aCPathNode != NULL );
    
    *aCPathNode = NULL;
    
    sListNode = SMU_LIST_GET_FIRST( & aTBSNode->mChkptPathBase );

    while ( sListNode != & aTBSNode->mChkptPathBase )
    {
        if ( sNodeIndex == aIndex )
        {
            *aCPathNode = (smmChkptPathNode*) sListNode->mData;
            break;
        }
        sListNode = SMU_LIST_GET_NEXT( sListNode );
        sNodeIndex ++;
    }
    
    return IDE_SUCCESS;
}



/*
 * 하나 혹은 그 이상의 Checkpoint Path를 TBSNode에서 제거한다.
 *
 * Checkpoint Path가 Tablespace에 존재하는 경우에만 제거한다.
 *
 * aTBSNode       [IN] - Checkpoint Path가 제거될 Tabelspace의 Node
 * aChkptPathList [IN] - 제거할 Checkpoint Path의 List
 */
IDE_RC smmTBSChkptPath::removeChkptPathNodesIfExist(
                          smmTBSNode           * aTBSNode,
                          smiChkptPathAttrList * aChkptPathList )
{
    SChar                * sChkptPath;
    smiChkptPathAttrList * sCPAttrList;
    smmChkptPathNode     * sCPathNode = NULL;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aChkptPathList != NULL );
    
    for ( sCPAttrList = aChkptPathList;
          sCPAttrList != NULL;
          sCPAttrList = sCPAttrList->mNext )
    {
        sChkptPath = sCPAttrList->mCPathAttr.mChkptPath;

        //////////////////////////////////////////////////////////////////
        // (e-020) 이미 Tablespace에 존재하는 Checkpoint Path가 있으면 에러
        IDE_TEST( findChkptPathNode( aTBSNode,
                                     sChkptPath,
                                     & sCPathNode )
                  != IDE_SUCCESS );

        // Checkpoint Path가 존재하는 경우에만 제거 
        if ( sCPathNode != NULL )
        {
            IDE_TEST( removeChkptPathNode( aTBSNode, sCPathNode )
                      != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
        
    return IDE_FAILURE;
}



/*
    Checkpoint Path의 접근 가능여부 체크 

    [IN] aChkptPath - 접근 가능여부를 체크할 Checkpoint Path
    
    [ 알고리즘 ]
      (010) NewPath가 없는 디렉토리이면 에러
      (020) NewPath가 디렉토리가 아닌 파일이면 에러
      (030) NewPath에 read/write/execute permission이 없으면 에러
 */
IDE_RC smmTBSChkptPath::checkAccess2ChkptPath( SChar * aChkptPath )
{
    DIR * sDir;
    IDE_DASSERT( aChkptPath != NULL );

    ////////////////////////////////////////////////////////////////////
    // (010) NewPath가 없는 디렉토리이면 에러
    IDE_TEST_RAISE( idf::access( aChkptPath, F_OK ) != 0,
                    err_path_not_found );

    ////////////////////////////////////////////////////////////////////
    // (020) NewPath가 디렉토리가 아닌 파일이면 에러
    sDir = idf::opendir( aChkptPath );
    IDE_TEST_RAISE( sDir == NULL, err_path_is_not_directory );
    (void)idf::closedir(sDir);
    
    ////////////////////////////////////////////////////////////////////
    // (030) NewPath에 read/write/execute permission이 없으면 에러
    IDE_TEST_RAISE( idf::access( aChkptPath, R_OK ) != 0,
                    err_path_no_read_perm );

    IDE_TEST_RAISE( idf::access( aChkptPath, W_OK ) != 0,
                    err_path_no_write_perm );
    
    IDE_TEST_RAISE( idf::access( aChkptPath, X_OK ) != 0,
                    err_path_no_exec_perm );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_path_not_found);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_NOT_EXIST,
                                aChkptPath));
    }
    IDE_EXCEPTION(err_path_is_not_directory);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_NOT_A_DIRECTORY,
                                aChkptPath));
    }
    IDE_EXCEPTION(err_path_no_read_perm);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_NO_READ_PERMISSION,
                                aChkptPath));
    }
    IDE_EXCEPTION(err_path_no_write_perm);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_NO_WRITE_PERMISSION,
                                aChkptPath));
    }
    IDE_EXCEPTION(err_path_no_exec_perm);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_NO_EXEC_PERMISSION,
                                aChkptPath));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
 * Tablespace의 다단계 해제중 Media Phase를 내릴때 호출된다
 *
 * aTBSNode [IN] - 모든 Checkpoint Path Node를 Free할 Tablespace
 */  
IDE_RC smmTBSChkptPath::freeAllChkptPathNode( smmTBSNode       * aTBSNode )
{
    smmChkptPathNode * sCPathNode;
    smuList          * sListNode;
    smuList          * sNextListNode;

    IDE_DASSERT( aTBSNode != NULL );
    
    sListNode = SMU_LIST_GET_FIRST( & aTBSNode->mChkptPathBase );

    // Create Tablespace의 Undo시에는 Checkpoint Path가 없을 수도 있다
    // 아래 ASSERT를 걸어서는 안된다.
    // IDE_DASSERT( sListNode != & aTBSNode->mChkptPathBase );

    while ( sListNode !=  & aTBSNode->mChkptPathBase )
    {
        sNextListNode = SMU_LIST_GET_NEXT( sListNode );
        
        sCPathNode = (smmChkptPathNode*)  sListNode->mData;
        
        IDE_TEST( destroyChkptPathNode( sCPathNode ) != IDE_SUCCESS );
        
        // Chkpt Path List 에서 Node의 제거도중
        // Checkpoint Path Count 감소하기 전에 에러가 발생해서는 안된다
        IDE_ASSERT( iduMemMgr::free( sCPathNode ) == IDE_SUCCESS );
        
        SMU_LIST_DELETE( sListNode );
        
        IDE_ASSERT( iduMemMgr::free( sListNode ) == IDE_SUCCESS );
        
        sListNode = sNextListNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
