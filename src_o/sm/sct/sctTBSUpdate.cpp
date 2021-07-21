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
 * $Id: sctTBSUpdate.cpp 23652 2007-10-01 23:20:28Z bskim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <sct.h>
#include <sctReq.h>
#include <sctTBSUpdate.h>

/*
    Tablespace의 Attribute Flag변경에 대한 로그를 분석한다.

    [IN]  aValueSize     - Log Image 의 크기 
    [IN]  aValuePtr      - Log Image
    [OUT] aAutoExtMode   - Auto extent mode
 */
IDE_RC sctTBSUpdate::getAlterAttrFlagImage( UInt       aValueSize,
                                            SChar    * aValuePtr,
                                            UInt     * aAttrFlag )
{
    IDE_DASSERT( aValuePtr != NULL );
    IDE_DASSERT( aValueSize == (UInt)( ID_SIZEOF(*aAttrFlag) ) );
    IDE_DASSERT( aAttrFlag  != NULL );

    ACP_UNUSED( aValueSize );
    
    idlOS::memcpy(aAttrFlag, aValuePtr, ID_SIZEOF(*aAttrFlag));
    aValuePtr += ID_SIZEOF(*aAttrFlag);

    return IDE_SUCCESS;
}



/*
    Tablespace의 Attribute Flag변경에 대한 로그의 Redo수행

    [ 로그 구조 ]
    After Image   --------------------------------------------
      UInt                aAfterAttrFlag
    
    [ ALTER_TBS_AUTO_EXTEND 의 REDO 처리 ]
      (r-010) TBSNode.AttrFlag := AfterImage.AttrFlag
*/
IDE_RC sctTBSUpdate::redo_SCT_UPDATE_ALTER_ATTR_FLAG(
                       idvSQL*              /*aStatistics*/, 
                       void*                aTrans,
                       smLSN                /* aCurLSN */,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               /* aIsRestart */ )
{

    sctTableSpaceNode  * sSpaceNode;
    UInt                 sAttrFlag;
    
    IDE_DASSERT( aTrans != NULL );
    ACP_UNUSED( aTrans );
    
    // aValueSize, aValuePtr 에 대한 인자 DASSERTION은
    // getAlterAttrFlagImage 에서 실시.
    IDE_TEST( getAlterAttrFlagImage( aValueSize,
                                       aValuePtr,
                                       & sAttrFlag ) != IDE_SUCCESS );

    sSpaceNode = sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    if ( sSpaceNode != NULL )
    {
        // Tablespace Attribute변경한다
        sctTableSpaceMgr::setTBSAttrFlag( sSpaceNode,
                                          sAttrFlag );
    }
    else
    {
        // 이미 Drop된 Tablespace인 경우 
        // nothing to do ...
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*
    Tablespace의 Attribute Flag변경에 대한 로그의 Undo수행

    [ 로그 구조 ]
    After Image   --------------------------------------------
      UInt                aBeforeAttrFlag
    
    [ ALTER_TBS_AUTO_EXTEND 의 REDO 처리 ]
      (r-010) TBSNode.AttrFlag := AfterImage.AttrFlag
*/
IDE_RC sctTBSUpdate::undo_SCT_UPDATE_ALTER_ATTR_FLAG(
                       idvSQL*              /*aStatistics*/, 
                       void*                aTrans,
                       smLSN                /* aCurLSN */,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               aIsRestart )
{

    sctTableSpaceNode  * sSpaceNode;
    UInt                 sAttrFlag;
    
    IDE_DASSERT( aTrans != NULL );
    ACP_UNUSED( aTrans );
    
    // aValueSize, aValuePtr 에 대한 인자 DASSERTION은
    // getAlterAttrFlagImage 에서 실시.
    IDE_TEST( getAlterAttrFlagImage( aValueSize,
                                       aValuePtr,
                                       & sAttrFlag ) != IDE_SUCCESS );

    sSpaceNode = sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    if ( sSpaceNode != NULL )
    {
        // Tablespace Attribute변경한다
        sctTableSpaceMgr::setTBSAttrFlag( sSpaceNode,
                                          sAttrFlag );

        if (aIsRestart == ID_FALSE)
        {
            // Log Anchor에 flush.
            IDE_TEST( smLayerCallback::updateTBSNodeAndFlush( (sctTableSpaceNode*)sSpaceNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // RESTART시에는 Loganchor를 flush하지 않는다.
        }
    }
    else
    {
        // 이미 Drop된 Tablespace인 경우 
        // nothing to do ...
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

