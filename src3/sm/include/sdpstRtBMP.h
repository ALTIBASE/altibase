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
 * $Id: sdpstBMP.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 BMP의 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_RT_BMP_H_
# define _O_SDPST_RT_BMP_H_ 1

# include <sdrDef.h>
# include <sdpDef.h>
# include <sdpPhyPage.h>

# include <sdpstDef.h>
# include <sdpstFindPage.h>
# include <sdpstBMP.h>

class sdpstRtBMP
{
public:
/* not used
    static IDE_RC updateFN(sdrMtx              * aMtx,
                           sdpstBMPHdr         * aBMPHdr,
                           SShort                aFmSlotIdx,
                           SShort                aToSlotIdx,
                           sdpstMFNL             aFmMFNL,
                           sdpstMFNL             aToMFNL );
*/
    static IDE_RC rescanItHint( idvSQL          * aStatistics,
                                scSpaceID         aSpaceID,
                                scPageID          aSegPID,
                                sdpstSearchType   aSearchType,
                                sdpstSegCache   * aSegCache,
                                sdpstStack      * aStack );

    static void findFreeItBMP( sdpstBMPHdr       * aBMPHdr,
                                 SShort              aSlotNoInParent,
                                 sdpstMFNL           aTargetMFNL,
                                 sdpstWM           * aHWM,
                                 scPageID          * aNxtItBMP,
                                 SShort            * aFreeSlotIdx );

    static IDE_RC findFstFreeSlot( idvSQL             * aStatistics,
                                   scSpaceID            aSpaceID,
                                   sdpstBMPHdr        * aBMPHdr,
                                   scPageID             aCurRtBMP,
                                   SShort               aCurIdx,
                                   sdpstSearchType      aSearchType,
                                   SShort               aRtBMPIdx,
                                   sdpstSegCache      * aSegCache,
                                   sdpstStack         * aStack,
                                   idBool             * aIsFound );

    static IDE_RC forwardItHint( idvSQL           * aStatistics,
                                 sdrMtx           * aMtx,
                                 scSpaceID          aSpaceID,
                                 sdpstSegCache    * aSegCache,
                                 sdpstStack       * aRevStack,
                                 sdpstSearchType    aSearchType );

    static IDE_RC setNxtRtBMP( idvSQL          * aStatistics,
                               sdrMtx          * aMtx,
                               scSpaceID         aSpaceID,
                               scPageID          aLstRtBMP,
                               scPageID          aNewRtBMP );

    static inline SShort getNxtFree( sdpstBMPHdr  * aBMPHdr,
                                     SShort         aSlotNo );

    static inline scPageID getNxtRtBMP( sdpstBMPHdr *aRtBMPHdr );

    static sdpstBMPOps  mVirtBMPOps;
    static sdpstBMPOps  mRtBMPOps;
};

/***********************************************************************
 * Description : SlotIdx부터 이후로 다음 free를 검색한다.
 ***********************************************************************/
inline SShort sdpstRtBMP::getNxtFree( sdpstBMPHdr  * aBMPHdr,
                                      SShort         aSlotNo )
{
    UShort             sCurSlotNo;
    SShort             sSlotIdx;
    sdpstBMPSlot     * sItSlot;

    IDE_ASSERT( aBMPHdr != NULL );
    IDE_ASSERT( aSlotNo != SDPST_INVALID_SLOTNO );

    sSlotIdx = SDPST_INVALID_SLOTNO;
    sItSlot  = sdpstBMP::getMapPtr( aBMPHdr );

    for( sCurSlotNo = aSlotNo; sCurSlotNo < aBMPHdr->mSlotCnt; sCurSlotNo++ )
    {
        if ( sItSlot[sCurSlotNo].mMFNL != SDPST_MFNL_FUL  )
        {
            sSlotIdx = (SShort)sCurSlotNo;
            break; // found it !!
        }
    }
    return sSlotIdx;
}

inline scPageID sdpstRtBMP::getNxtRtBMP( sdpstBMPHdr *aRtBMPHdr )
{
    IDE_ASSERT(aRtBMPHdr != NULL);
    return aRtBMPHdr->mNxtRtBMP;
}

#endif // _O_SDPST_RT_BMP_H_
