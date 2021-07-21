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
 

#ifndef O_SDC_HV_TEMP_RECORD_H_
#define O_SDC_HV_TEMP_RECORD_H_ 1

#include <smDef.h>
#include <smpDef.h>
#include <smcDef.h>
#include <sdcDef.h>
#include <sdr.h>
#include <smiDef.h>

typedef idBool (*sdcHVCompareFunc)(UChar *aRow,
                                   void  *aCompareArg1,
                                   void  *aCompareArg2);

class sdcHVTempRecord
{
public:
    static IDE_RC findRow(idvSQL            *aStatistics,
                          scSpaceID          aTBSID,
                          UInt               aHashValue,
                          smiCallBack       *aFilter,
                          sdpPhyPageHdr     *aTargetPage,
                          UChar            **aRowPtr,
                          UInt              *aRowSize,
                          sdRID             *aRowRID);

    static IDE_RC insertRecord(idvSQL         *aStatistics,
                               void           *aTableHdr,
                               UInt            aHashValue,
                               sdpSegmentDesc *aSegDesc,
                               scSpaceID       aTBSID,
                               const smiValue *aValueList,
                               smiCallBack    *aFilter,
                               sdpPhyPageHdr  *aTargetPage,
                               idBool          aIsUnique,
                               scGRID         *aRowGRID,
                               UInt           *aPropagateHashVal,
                               scPageID       *aPropagatePageID,
                               idBool         *aHashConflict);

    static IDE_RC createPage(idvSQL         *aStatistics,
                             sdrMtx         *aMtx,
                             sdpSegmentDesc *aSegDesc,
                             scSpaceID       aTBSID,
                             UInt            aRowSize,
                             sdpPhyPageHdr  *aCurDataPage,
                             sdpPhyPageHdr **aNewDataPage);

    static IDE_RC update(idvSQL           *aStatistics,
                         void             * ,// aTrans
                         smSCN            ,
                         void             */*aTableInfo*/,
                         void             * aTableHdr,
                         SChar            *,//aOldRow,
                         scGRID             aSlotGRID,
                         SChar            **, //aRetRow,
                         scGRID           * aRetUpdateSlotGRID,
                         const smiColumnList * aColumnList,
                         const smiValue   * aValueList,
                         smSCN            /*aInfinite */,
                         void*            /*aLobInfo4Update*/,
                         UChar           */*aOldRecImage */,
                         UChar           */*aModifyIdxBit */ );

    static IDE_RC validatePage(UChar *aPagePtr);

private:
    static void findPosByHashValue(UInt              aHashValue,
                                   sdcHVDataPageHdr *aPageHdr,
                                   UShort           *aInsertPos,
                                   idBool           *aIsSame,
                                   idBool           *aNeedToSearchNext);

    static IDE_RC findExactRow(idvSQL           *aStatistics,
                               scSpaceID         aTBSID,
                               sdcHVDataPageHdr *aDataPageHdr,
                               UShort            aPos,
                               UInt              aHashValue,
                               smiCallBack      *aFilter,
                               UChar           **aRow,
                               UInt             *aRowSize,
                               sdRID            *aRowRID);

    static void initRecSizeInfo(sdpPhyPageHdr *aPage,
                                UShort         aSize);

    static void insertHashValueAndRow(void             *aTableHdr,
                                      sdcHVDataPageHdr *aPageHdr,
                                      UInt              aHashValue,
                                      UShort            aInsertPos,
                                      const smiValue   *aValueList,
                                      sdRID            *aRowRID);

    static void divideRecords(sdcHVDataPageHdr *aSourcePage,
                              sdcHVDataPageHdr *aTargetPage,
                              UShort            aInsertPos,
                              idBool           *aInsertOnLeft);

    static UInt getMinHashValue(sdcHVDataPageHdr *aPage);

    inline static UShort getRecordSize(sdcHVDataPageHdr *aPageLH)
    {
        return offsetof(sdcHVRecord, mRow) + aPageLH->mRowSize;
    }
};
#endif
