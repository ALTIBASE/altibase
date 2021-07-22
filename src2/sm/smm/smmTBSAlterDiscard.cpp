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
* $Id: smmTBSAlterDiscard.cpp 19201 2006-11-30 00:54:40Z kmkim $
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
#include <smmTBSAlterDiscard.h>
#include <smmTBSMultiPhase.h>

/*
  생성자 (아무것도 안함)
*/
smmTBSAlterDiscard::smmTBSAlterDiscard()
{

}


/*
    Tablespace를 DISCARDED상태로 바꾸고, Loganchor에 Flush한다.

    Discard의 정의 :
       - 더 이상 사용할 수 없는 Tablespace
       - 오직 Drop만이 가능

    사용예 :
       - Disk가 완전히 맛가서 더이상 사용불가할 때
         해당 Tablespace만 discard하고 나머지 Tablespace만이라도
         운영하고 싶을때, CONTROL단계에서 Tablespace를 DISCARD한다.

     동시성제어 :
       - CONTROL단계에서만 호출되기 때문에, sctTableSpaceMgr에
         Mutex를 잡을 필요가 없다.
 */
IDE_RC smmTBSAlterDiscard::alterTBSdiscard( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST_RAISE ( SMI_TBS_IS_DISCARDED( aTBSNode->mHeader.mState),
                     error_already_discarded );

    // Discard Tablespace에 해당되는 다단계 초기화 단계인
    // MEDIA단계로 전이한다.
    //
    // => ONLINE Tablespace를 Discard하는 경우 PAGE 단계를 해제해야함
    IDE_TEST( smmTBSMultiPhase::finiToMediaPhase( aTBSNode->mHeader.mState,
                                                  aTBSNode )
              != IDE_SUCCESS );
    
    aTBSNode->mHeader.mState = SMI_TBS_DISCARDED;

    IDE_TEST( smLayerCallback::updateTBSNodeAndFlush( & aTBSNode->mHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_discarded );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TBS_ALREADY_DISCARDED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

