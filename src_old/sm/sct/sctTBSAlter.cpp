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
* $Id: sctTBSAlter.cpp 19201 2006-11-30 00:54:40Z kmkim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smu.h>
#include <sctReq.h>
#include <sctTableSpaceMgr.h>
#include <sctTBSAlter.h>

/*
  생성자 (아무것도 안함)
*/
sctTBSAlter::sctTBSAlter()
{

}

/*
    Tablespace Attrbute Flag의 변경 ( ex> ALTER TABLESPACE LOG COMPRESS ON )
    
    aTrans      [IN] Transaction
    aSpaceID    [IN] Tablespace의 ID
    aAttrFlagMask             [IN] 변경할 Attribute Flag의 MASK
    aAttrFlagNewValue         [IN] 변경할 Attribute Flag의 새로운 값
    
    [ 알고리즘 ]
      (010) lock TBSNode in X
      (030) 로깅실시 => ALTER_TBS_ATTR_FLAG
      (040) ATTR FLAG를 변경 
      (050) Tablespace Node를 Log Anchor에 Flush!
      
    [ ALTER_TBS_ATTR_FLAG 의 REDO 처리 ]
      (r-010) TBSNode.AttrFlag := AfterImage.AttrFlag

    [ ALTER_TBS_ATTR_FLAG 의 UNDO 처리 ]
      (u-010) 로깅실시 -> CLR ( ALTER_TBS_ATTR_FLAG )
      (u-020) TBSNode.AttrFlag := BeforeImage.AttrFlag
*/
IDE_RC sctTBSAlter::alterTBSAttrFlag(void      * aTrans,
                                     scSpaceID   aTableSpaceID,
                                     UInt        aAttrFlagMask, 
                                     UInt        aAttrFlagNewValue )
{
    sctTableSpaceNode * sSpaceNode;
    UInt                sBeforeAttrFlag;
    UInt                sAfterAttrFlag;

    IDE_DASSERT( aTrans != NULL );

    ///////////////////////////////////////////////////////////////////////////
    // Tablespace ID로부터 Node를 가져온다    
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**)&sSpaceNode )
                  != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    // (010) lock TBSNode in X
    //       DROPPED,DISCARD,OFFLINE 인 경우 여기에서 에러가 발생한다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode( 
                                   aTrans,
                                   sSpaceNode,
                                   ID_FALSE,   /* intent */
                                   ID_TRUE,    /* exclusive */
                                   SCT_VAL_DDL_DML) /* validation */
              != IDE_SUCCESS );

    // Tablespace Attribute변경을 위해 Attribute Pointer를 가져온다
    sBeforeAttrFlag = sctTableSpaceMgr::getTBSAttrFlag( sSpaceNode );

    ///////////////////////////////////////////////////////////////////////////
    // (e-010) 이미 해당 Attribute가 설정되어 있는 경우 에러 
    IDE_TEST( checkErrorOnAttrFlag( sSpaceNode,
                                    sBeforeAttrFlag,
                                    aAttrFlagMask,
                                    aAttrFlagNewValue )
              != IDE_SUCCESS );

    // 새로운 AttrFlag := MASK비트 끄고 NewValue비트 킨다
    sAfterAttrFlag   = ( sBeforeAttrFlag & ~aAttrFlagMask ) |
                       aAttrFlagNewValue ;
    
    ///////////////////////////////////////////////////////////////////////////
    // (030) 로깅실시 => ALTER_TBS_ATTR_FLAG
    IDE_TEST( smLayerCallback::writeTBSAlterAttrFlag ( aTrans,
                                                       aTableSpaceID,
                                                       /* Before Image */
                                                       sBeforeAttrFlag,
                                                       /* After Image */
                                                       sAfterAttrFlag )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    // (040) AttrFlag 변경
    sctTableSpaceMgr::setTBSAttrFlag( sSpaceNode ,
                                      sAfterAttrFlag );

    ///////////////////////////////////////////////////////////////////////////
    // (050) Tablespace Node를 Log Anchor에 Flush!
    IDE_TEST( smLayerCallback::updateTBSNodeAndFlush( sSpaceNode ) 
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    // (010)에서 획득한 Tablespace X Lock은 UNDO완료후 자동으로 풀게된다
    // 여기서 별도 처리할 필요 없음
    
    return IDE_FAILURE;
}

/*
    Tablespace Attrbute Flag의 변경에 대한 에러처리 
    aSpaceNode                [IN] Tablespace의 Node
    aCurrentAttrFlag          [IN] 현재 Tablespace에 설정된 Attribute Flag값
    aAttrFlagMask             [IN] 변경할 Attribute Flag의 MASK
    aAttrFlagNewValue         [IN] 변경할 Attribute Flag의 새로운 값
    
    [ 에러처리 ]
      (e-010) Volatile Tablespace에 대해
              로그 압축 여부를 변경하려 하면 에러 
      (e-020) 이미 Attrubute Flag가 설정되어 있으면 에러
      
    [ 선결조건 ]
      aTBSNode에 해당하는 Tablespace에 X락이 잡혀있는 상태여야 한다.
*/
IDE_RC sctTBSAlter::checkErrorOnAttrFlag( sctTableSpaceNode * aSpaceNode,
                                          UInt        aCurrentAttrFlag,
                                          UInt        aAttrFlagMask, 
                                          UInt        aAttrFlagNewValue)
{
    IDE_DASSERT( aSpaceNode != NULL );

    ///////////////////////////////////////////////////////////////////////////
    //  (e-010) Volatile Tablespace에 대해
    //          로그 압축 하도록 설정하면 에러 
    if ( sctTableSpaceMgr::isVolatileTableSpace( aSpaceNode->mID ) == ID_TRUE )
    {
        if ( (aAttrFlagMask & SMI_TBS_ATTR_LOG_COMPRESS_MASK) != 0 )
        {
            if ( (aAttrFlagNewValue & SMI_TBS_ATTR_LOG_COMPRESS_MASK )
                 == SMI_TBS_ATTR_LOG_COMPRESS_TRUE )
            {
                IDE_RAISE( err_volatile_log_compress );
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // (e-020) 이미 Attribute Flag가 설정되어 있으면 에러
    IDE_TEST_RAISE( ( aCurrentAttrFlag & aAttrFlagMask )
                    == aAttrFlagNewValue,
                    error_already_set_attr_flag);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_set_attr_flag );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_TBS_ATTR_FLAG_ALREADY_SET ));
    }
    IDE_EXCEPTION( err_volatile_log_compress );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_UNABLE_TO_COMPRESS_VOLATILE_TBS_LOG));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

