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
 * $Id: sdcTSSlot.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 * 본 파일은 transaction status slot에 대한 구현파일입니다.
 *
 **********************************************************************/

#include <smDef.h>
#include <sdr.h>
#include <sdp.h>
#include <sdcTSSlot.h>
#include <sdcReq.h>


/***********************************************************************
 * Description : TSS 초기화 및 SDR_SDC_BIND_TSS 로깅
 *
 * [ 인자 ]
 *
 **********************************************************************/
IDE_RC sdcTSSlot::logAndInit( sdrMtx    * aMtx,
                              sdcTSS    * aSlotPtr,
                              sdSID       aTSSlotSID,
                              sdRID       aExtRID4TSS,
                              smTID       aTransID,
                              UInt        aTXSegEntryID )
{
    UInt        sWriteSize;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aSlotPtr       != NULL );

    init( aSlotPtr, aTransID );

    if( sdrMiniTrans::getLogMode(aMtx) == SDR_MTX_LOGGING )
    {
        sdrMiniTrans::setRefOffset(aMtx);

        sWriteSize = ID_SIZEOF(sdSID) + ID_SIZEOF(sdRID) +
                     ID_SIZEOF(smTID) + ID_SIZEOF(UInt);

        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)aSlotPtr,
                                             NULL,
                                             sWriteSize,
                                             SDR_SDC_BIND_TSS )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       &aTSSlotSID,
                                       ID_SIZEOF(sdSID) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       &aExtRID4TSS,
                                       ID_SIZEOF(sdRID) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       &aTransID,
                                       ID_SIZEOF(smTID) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       &aTXSegEntryID,
                                       ID_SIZEOF(UInt) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  Commit SCN 설정
 **********************************************************************/
IDE_RC sdcTSSlot::setCommitSCN( sdrMtx    * aMtx,
                                sdcTSS    * aSlotPtr,
                                smSCN     * aCommitSCN,
                                idBool      aDoUpdate )
{
    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSlotPtr   != NULL );
    IDE_ASSERT( aCommitSCN != NULL );
    IDE_ASSERT( SM_SCN_IS_NOT_INFINITE(*aCommitSCN) );

    if ( aDoUpdate == ID_TRUE )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes(
                     aMtx,
                     (UChar*)&aSlotPtr->mCommitSCN,
                     aCommitSCN,
                     SDR_SDP_8BYTE ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdrMiniTrans::writeLogRec(
                      aMtx,
                      (UChar*)aSlotPtr,
                      NULL, // value
                      0,    // valueSize
                      SDR_SDC_SET_INITSCN_TO_TSS )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : TSS 상태설정
 **********************************************************************/
IDE_RC sdcTSSlot::setState( sdrMtx     * aMtx,
                            sdcTSS     * aSlotPtr,
                            sdcTSState   aState )
{

    IDE_ASSERT( aSlotPtr != NULL );
    IDE_ASSERT( aMtx     != NULL );

    return sdrMiniTrans::writeNBytes( aMtx,
                                      (UChar*)&aSlotPtr->mState,
                                      &aState,
                                      SDR_SDP_4BYTE );
}
