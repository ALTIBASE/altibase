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
 * $Id:$
 ***********************************************************************/

# ifndef _O_SDPSF_FINDPAGE_H_
# define _O_SDPSF_FINDPAGE_H_ 1

# include <sdr.h>

# include <sdpReq.h>
# include <sdpsfDef.h>

class sdpsfFindPage
{
public:
    static IDE_RC findFreePage( idvSQL           *aStatistics,
                                sdrMtx           *aMtx,
                                scSpaceID         aSpaceID,
                                sdpSegHandle     *aSegHandle,
                                void             *aTableInfo,
                                sdpPageType       aPageType,
                                UInt              aRecordSize,
                                idBool            aNeedKeySlot,
                                UChar           **aPagePtr,
                                UChar            *aCTSlotIdx );

    static IDE_RC checkHintPID4Insert( idvSQL           *aStatistics,
                                       sdrMtx           *aMtx,
                                       scSpaceID         aSpaceID,
                                       void             *aTableInfo,
                                       UInt              aRecordSize,
                                       idBool            aNeedKeySlot,
                                       UChar           **aPagePtr,
                                       UChar            *aCTSlotIdx );

    static IDE_RC updatePageState( idvSQL           * aStatistics,
                                   sdrMtx           * aMtx,
                                   scSpaceID          aSpaceID,
                                   sdpSegHandle     * aSegHandle,
                                   UChar            * aDataPagePtr );

    static IDE_RC checkSizeAndAllocCTS(
                                   idvSQL          * aStatistics,
                                   sdrMtxStartInfo * aStartInfo,
                                   UChar           * aPagePtr,
                                   UInt              aRowSize,
                                   idBool            aNeedKeySlot,
                                   idBool          * aCanAllocSlot,
                                   idBool          * aRemFrmList,
                                   UChar           * aCTSlotIdx );

private:

    static inline idBool isPageInsertable( sdpPhyPageHdr *aPageHdr,
                                           UInt           aPctUsed );

    static inline idBool isPageUpdateOnly( sdpPhyPageHdr *aPageHdr,
                                           UInt           aPctFree );

};

/***********************************************************************
 *
 * Description :
 *  Page 상태가 Insertable인지를 Check한다.
 *
 *  aPageHdr - [IN] physical page header
 *  aPctUsed - [IN] PCTUSED
 *
 **********************************************************************/
inline idBool sdpsfFindPage::isPageInsertable( sdpPhyPageHdr *aPageHdr,
                                               UInt           aPctUsed )
{
    UInt   sUsedSize;
    idBool sIsInsertable;
    UInt   sPctUsedSize;

    IDE_DASSERT( aPageHdr != NULL );
    /* PCTUSED는 0~99사이의 값을 가져야 한다. */
    IDE_DASSERT( aPctUsed < 100 );

    sIsInsertable = ID_FALSE;

    if( sdpPhyPage::canMakeSlotEntry( aPageHdr ) == ID_TRUE )
    {
        sUsedSize = SD_PAGE_SIZE -
            sdpPhyPage::getTotalFreeSize( aPageHdr )    -
            smLayerCallback::getTotAgingSize( aPageHdr ) +
            ID_SIZEOF(sdpSlotEntry);

        sPctUsedSize = ( SD_PAGE_SIZE * aPctUsed )  / 100 ;

        if( sUsedSize < sPctUsedSize )
        {
            /* Page의 공간 사용량이 PCTUSED보다 작아지면,
             * 다시 insert할 수 있다. */
            sIsInsertable = ID_TRUE;
        }
    }

    return sIsInsertable;
}

/***********************************************************************
 *
 * Description :
 *  Page 상태가 Update Only인지를 Check한다.
 *
 *  aPageHdr - [IN] physical page header
 *  aPctFree - [IN] PCTFREE
 *
 **********************************************************************/
inline idBool sdpsfFindPage::isPageUpdateOnly( sdpPhyPageHdr *aPageHdr,
                                               UInt           aPctFree )
{
    UInt   sFreeSize;
    idBool sIsUpdateOnly;
    UInt   sPctFreeSize;

    IDE_DASSERT( aPageHdr != NULL );
    /* PCTFREE는 0~99사이의 값을 가져야 한다. */
    IDE_DASSERT( aPctFree < 100 );

    sIsUpdateOnly = ID_FALSE;

    if( sdpPhyPage::canMakeSlotEntry( aPageHdr ) == ID_TRUE )
    {
        sFreeSize = sdpPhyPage::getTotalFreeSize( aPageHdr ) +
                    smLayerCallback::getTotAgingSize( aPageHdr );

        sPctFreeSize = ( SD_PAGE_SIZE * aPctFree )  / 100 ;

        if( sFreeSize < sPctFreeSize )
        {
            /* 페이지의 가용공간이 PCTFREE보다 작게되면,
             * 이 페이지에 새로운 row를 insert할 수 없다. */
            sIsUpdateOnly = ID_TRUE;
        }
    }

    return sIsUpdateOnly;
}

#endif /* _O_SDPSF_FINDPAGE_H_ */
