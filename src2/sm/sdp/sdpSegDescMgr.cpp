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
 * $Id: sdpSegDescMgr.cpp 27220 2008-07-23 14:56:22Z newdaily $
 **********************************************************************/

# include <smDef.h>
# include <smuProperty.h>
# include <smErrorCode.h>

# include <sdpDef.h>
# include <sdpSegDescMgr.h>

IDE_RC sdpSegDescMgr::initSegDesc( sdpSegmentDesc  * aSegmentDesc,
                                   scSpaceID         aSpaceID,
                                   scPageID          aSegPID,
                                   sdpSegType        aSegType,
                                   smOID             aTableOID,
                                   UInt              aIndexID )
{
    idBool        sIsExist;
    sdpSegMgmtOp *sSegMgmtOp = NULL;

    IDE_ASSERT( aSegmentDesc != NULL );

    aSegmentDesc->mSegMgmtType = sdpTableSpace::getSegMgmtType( aSpaceID );
    aSegmentDesc->mSegMgmtOp   = getSegMgmtOp(aSegmentDesc->mSegMgmtType);

    aSegmentDesc->mSegHandle.mSpaceID = aSpaceID;
    // Segment RID는 생성전에는 SD_NULL_RID일 수 있다.
    aSegmentDesc->mSegHandle.mSegPID = aSegPID;

    if ( aSegmentDesc->mSegHandle.mSegPID != SD_NULL_PID )
    {
        sIsExist = sddDiskMgr::isValidPageID( NULL,
                                              aSpaceID,
                                              aSegmentDesc->mSegHandle.mSegPID );

        if( sIsExist == ID_FALSE )
        {
            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        SM_TRC_DPAGE_WARNNING1,
                        aSegType,
                        aSegmentDesc->mSegHandle.mSegPID,
                        aSpaceID);
        }
    }
    else
    {
        // Segment생성시 아직 Segment PID가 결정되지 않은 경우
        // Segment PID가 설정되지 않은 경우에는 Segment 초기화를 Skip 한다.
    }

    if( aSegmentDesc->mSegMgmtType != SMI_SEGMENT_MGMT_NULL_TYPE )
    {
        sSegMgmtOp = getSegMgmtOp( aSegmentDesc );

        IDE_ASSERT( sSegMgmtOp != NULL );

        IDE_TEST( sSegMgmtOp->mInitialize(
                      &(aSegmentDesc->mSegHandle),
                      aSpaceID,
                      aSegType,
                      aTableOID,
                      aIndexID ) != IDE_SUCCESS );
    }
    else
    {
        /* Table이 속해있는 TableSpace가 이미 Drop되었을때 */
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpSegDescMgr::destSegDesc( sdpSegmentDesc * aSegmentDesc )
{
    sdpSegMgmtOp *sSegMgmtOp = NULL;
    IDE_ASSERT( aSegmentDesc != NULL );

    if( aSegmentDesc->mSegMgmtType != SMI_SEGMENT_MGMT_NULL_TYPE )
    {
        sSegMgmtOp = getSegMgmtOp( aSegmentDesc );

        IDE_ASSERT( sSegMgmtOp != NULL );

        IDE_TEST( sSegMgmtOp->mDestroy(
                      &(aSegmentDesc->mSegHandle) ) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  segment attribute default값을 설정한다.
 *
 *  aSegmentAttr - [OUT] segment attribute
 *  aSegType     - [IN]  segment type
 *
 ***********************************************************************/
void sdpSegDescMgr::setDefaultSegAttr( smiSegAttr  * aSegmentAttr,
                                       sdpSegType    aSegType )
{
    IDE_ASSERT( aSegmentAttr != NULL );

    aSegmentAttr->mPctFree
        = smuProperty::getDefaultPctFree();
    aSegmentAttr->mPctUsed
        = smuProperty::getDefaultPctUsed();

    if( aSegType == SDP_SEG_TYPE_INDEX )
    {
        aSegmentAttr->mInitTrans
            = smuProperty::getDefaultIndexInitTrans();
        aSegmentAttr->mMaxTrans
            = smuProperty::getDefaultIndexMaxTrans();
    }
    else
    {
        aSegmentAttr->mInitTrans
            = smuProperty::getDefaultTableInitTrans();
        aSegmentAttr->mMaxTrans
            = smuProperty::getDefaultTableMaxTrans();
    }
    return;
}

void sdpSegDescMgr::setDefaultSegStoAttr( smiSegStorageAttr  * aSegmentStoAttr )
{
    IDE_ASSERT( aSegmentStoAttr != NULL );
  
    aSegmentStoAttr->mInitExtCnt
        = smuProperty::getSegStoInitExtCnt();
    aSegmentStoAttr->mNextExtCnt
        = smuProperty::getSegStoNextExtCnt();
    aSegmentStoAttr->mMinExtCnt
        = smuProperty::getSegStoMinExtCnt();
    aSegmentStoAttr->mMaxExtCnt
        = smuProperty::getSegStoMaxExtCnt();
    return;
}

/*******************************************************************************
 * Description : sdpSegmentDesc의 내용을 dump한다.
 *
 * Implementation : mCache에 바인딩되는 객체는 형이 특정되어 있지 않으므로
 *          본 dump 함수에서는 제외한다.
 *
 * Parameters :
 *      aSegDesc    - [IN] dump 할 sdpSegmentDesc
 ******************************************************************************/
IDE_RC sdpSegDescMgr::dumpSegDesc( sdpSegmentDesc* aSegDesc )
{
    if( aSegDesc == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "=============================\n"
                     " Segment Descriptor: NULL\n"
                     "=============================\n" );
    }
    else
    {
        ideLog::log( IDE_SERVER_0,
                     "=============================\n"
                     " Segment Descriptor dump...\n"
                     "=============================\n"
                     "Space ID              : %u\n"
                     "Segment PID           : %u\n"
                     "Init. Extent Count    : %u\n"
                     "Next Extent Count     : %u\n"
                     "Minimum Extent Count  : %u\n"
                     "Maximum Extent Count  : %u\n"
                     "PCTFREE               : %u\n"
                     "PCTUSED               : %u\n"
                     "Init. CTS Count       : %u\n"
                     "Maximum CTS Count     : %u\n"
                     "Segment Mgmt. Type    : %u",
                     aSegDesc->mSegHandle.mSpaceID,
                     aSegDesc->mSegHandle.mSegPID,
                     aSegDesc->mSegHandle.mSegStoAttr.mInitExtCnt,
                     aSegDesc->mSegHandle.mSegStoAttr.mNextExtCnt,
                     aSegDesc->mSegHandle.mSegStoAttr.mMinExtCnt,
                     aSegDesc->mSegHandle.mSegStoAttr.mMaxExtCnt,
                     aSegDesc->mSegHandle.mSegAttr.mPctFree,
                     aSegDesc->mSegHandle.mSegAttr.mPctUsed,
                     aSegDesc->mSegHandle.mSegAttr.mInitTrans,
                     aSegDesc->mSegHandle.mSegAttr.mMaxTrans,
                     aSegDesc->mSegMgmtType );
    }

    return IDE_SUCCESS;
}

