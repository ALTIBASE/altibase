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
 * $Id: sdpstDPath.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 Direct-Path Insert 연산에 관련된
 * 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_DIRECT_PATH_INSERT_H_
# define _O_SDPST_DIRECT_PATH_INSERT_H_ 1

# include <sdpDef.h>
# include <sdpstDef.h>

class sdpstDPath
{
public:
    static IDE_RC allocNewPage4Append(
                              idvSQL              * aStatistics,
                              sdrMtx              * aMtx,
                              scSpaceID             aSpaceID,
                              sdpSegHandle        * aSegHandle,
                              sdRID                 aPrvAllocExtRID,
                              scPageID              /*aFstPIDOfPrvAllocExt*/,
                              scPageID              aPrvAllocPageID,
                              idBool                aIsLogging,
                              sdpPageType           aPageType,
                              sdRID               * aAllocExtRID,
                              scPageID            * /*aFstPIDOfAllocExt*/,
                              scPageID            * aAllocPID,
                              UChar              ** aAllocPagePtr );

    static IDE_RC  updateWMInfo4DPath(
                              idvSQL           *aStatistics,
                              sdrMtxStartInfo  *aStartInfo,
                              scSpaceID         aSpaceID,
                              sdpSegHandle     *aSegHandle,
                              scPageID          aPrvLstAllocPID,
                              sdRID             aLstAllocExtRID,
                              scPageID        /*aFstPIDOfAllocExtOfFM*/,
                              scPageID          aLstPID,
                              ULong             aAllocPageCnt,
                              idBool            aMergeMultiSeg );

    static IDE_RC updateBMPUntilHWM(
                              idvSQL           * aStatistics,
                              scSpaceID          aSpaceID,
                              scPageID           aSegPID,
                              scPageID           aPrvLstAllocPIDOfFM,
                              scPageID           aLstAllocPIDOfFM,
                              sdrMtxStartInfo  * aStartInfo );

    static IDE_RC updateMFNLToFull4DPath(
                              idvSQL             * aStatistics,
                              sdrMtxStartInfo    * aStartInfo,
                              sdpstBMPHdr        * aBMPHdr,
                              SShort               aFmSlotNo,
                              SShort               aToSlotNo,
                              sdpstMFNL          * aNewMFNL );

    static inline void  calcExtInfo2ExtRID(
                              scPageID             aSegPID,
                              UChar              * aSegPagePtr,
                              scPageID             aExtMapPID,
                              SShort               aSlotNo,
                              sdRID              * aExtRID );

    static inline void  calcExtRID2ExtInfo(
                              scPageID             aSegPID,
                              UChar              * aSegPagePtr,
                              sdRID                aExtRID,
                              scPageID           * aExtMapPID,
                              SShort             * aSlotNo );

    static IDE_RC updateBMPUntilHWMInLfBMP(
                              idvSQL            * aStatistics,
                              sdrMtxStartInfo   * aStartInfo,
                              scSpaceID           aSpaceID,
                              scPageID            /*aSegPID*/,
                              sdpstStack        * aHWMStack,
                              sdpstStack        * aCurStack,
                              SShort            * /*aFmLfSlotNo*/,
                              SShort            * /*aFmItSlotNo*/,
                              sdpstBMPType      * aPrvDepth,
                              idBool            * aIsFinish,
                              sdpstMFNL         * aNewMFNL );

    static IDE_RC updateBMPUntilHWMInRtAndItBMP(
                              idvSQL            * aStatistics,
                              sdrMtxStartInfo   * aStartInfo,
                              scSpaceID           aSpaceID,
                              scPageID            /*aSegPID*/,
                              sdpstStack        * aHWMStack,
                              sdpstStack        * aCurStack,
                              SShort            * aFmSlotNo,
                              SShort            * aFmItSlotNo,
                              sdpstBMPType      * aPrvDepth,
                              idBool            * aIsFinish,
                              sdpstMFNL         * aNewMFNL );


    /* BUG-32164 [sm-disk-page] If Table Segment is expanded to 12gb by
     * Direct-path insert operation, Server fatal
     * RootBMP 개수를 넘어설 정도로 갱신할 경우, VirtualBMP까지 접근함 */
    static IDE_RC updateBMPUntilHWMInVtBMP(
                              idvSQL            * aStatistics,
                              sdrMtxStartInfo   * /*aStartInfo*/,
                              scSpaceID           aSpaceID,
                              scPageID            aSegPID,
                              sdpstStack        * /*aHWMStack*/,
                              sdpstStack        * aCurStack,
                              SShort            * /*aFmSlotNo*/,
                              SShort            * /*aFmItSlotNo*/,
                              sdpstBMPType      * aPrvDepth,
                              idBool            * /*aIsFinish*/,
                              sdpstMFNL         * /*aNewMFNL*/ );

    static IDE_RC  reformatPage4DPath(
                            idvSQL           *aStatistics,
                            sdrMtxStartInfo  *aStartInfo,
                            scSpaceID         aSpaceID,
                            sdpSegHandle     *aSegHandle,
                            sdRID             aLstAllocExtRID,
                            scPageID          aLstPID );
};

inline void  sdpstDPath::calcExtInfo2ExtRID( scPageID             aSegPID,
                                             UChar              * aSegPagePtr,
                                             scPageID             aExtMapPID,
                                             SShort               aSlotNo,
                                             sdRID              * aExtRID )
{
    IDE_ASSERT( aSegPagePtr != NULL );
    IDE_ASSERT( aExtRID != NULL );

    // 현재 HWM의 ExtRID 구한다.
    if ( aSegPID != aExtMapPID )
    {
        *aExtRID = SD_MAKE_RID( aExtMapPID,
                                sdpstExtDir::calcSlotNo2Offset(
                                    NULL,
                                    aSlotNo ) );
    }
    else
    {
        *aExtRID = SD_MAKE_RID( aExtMapPID,
                                sdpstExtDir::calcSlotNo2Offset(
                                    sdpstSH::getExtDirHdr ( aSegPagePtr ),
                                    aSlotNo ) );
    }
    return;
}

inline void  sdpstDPath::calcExtRID2ExtInfo( scPageID             aSegPID,
                                             UChar              * aSegPagePtr,
                                             sdRID                aExtRID,
                                             scPageID           * aExtMapPID,
                                             SShort             * aSlotNo )
{
    IDE_ASSERT( aSegPagePtr != NULL );
    IDE_ASSERT( aExtRID != SD_NULL_RID );
    IDE_ASSERT( aExtMapPID != NULL );
    IDE_ASSERT( aSlotNo != NULL );

    // 현재 HWM의 ExtRID 구한다.
    if ( aSegPID != SD_MAKE_PID( aExtRID ) )
    {
        *aSlotNo = sdpstExtDir::calcOffset2SlotNo(
            NULL,
            SD_MAKE_OFFSET(aExtRID) );
    }
    else
    {
        *aSlotNo = sdpstExtDir::calcOffset2SlotNo(
            sdpstSH::getExtDirHdr( aSegPagePtr ),
            SD_MAKE_OFFSET(aExtRID) );
    }
    *aExtMapPID = SD_MAKE_PID(aExtRID);
    return;
}

#endif // _O_SDPST_DIRECT_PATH_INSERT_H_
