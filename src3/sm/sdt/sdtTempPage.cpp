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
 * $Id $
 **********************************************************************/


#include <sdtTempPage.h>

SChar sdtTempPage::mPageName[ SDT_TEMP_PAGETYPE_MAX ][ SMI_TT_STR_SIZE ] = {
    "INIT",
    "INMEMORYGROUP",
    "SORTEDRUN",
    "LNODE",
    "INODE",
    "INDEX_EXTRA",
    "HASHROWS",  /* dump Sort Page에서는 사용되지 않음 */
    "SUBHASH"    /* dump Sort Page에서는 사용되지 않음 */
};

/**************************************************************************
 * Description :
 * Page를 sdtTempPage 형태로 초기화한다.
 *
 * <IN>
 * aPagePtr       - 초기화할 대상 Page
 * aType          - Page의 Type
 * aPrev          - Page의 이전 PID
 * aNext          - Page의 다음 PID
 ***************************************************************************/
void sdtTempPage::init( sdtSortPageHdr  * aPageHdr,
                        sdtTempPageType   aType,
                        scPageID          aPrev,
                        scPageID          aSelf,
                        scPageID          aNext )
{
    IDE_DASSERT( SD_PAGE_SIZE -1 <= SDT_TEMP_FREEOFFSET_BITMASK );

    aPageHdr->mTypeAndFreeOffset = ( aType << SDT_TEMP_TYPE_SHIFT ) | ( SD_PAGE_SIZE - 1 );

    IDE_DASSERT( getType( aPageHdr ) == aType );
    IDE_DASSERT( getFreeOffset( aPageHdr ) == SD_PAGE_SIZE - 1 );

    aPageHdr->mPrevPID   = aPrev;
    aPageHdr->mSelfPID   = aSelf;
    aPageHdr->mNextPID   = aNext;
    aPageHdr->mSlotCount = 0;

    return;
}

void sdtTempPage::dumpTempPage( void  * aPagePtr,
                                SChar * aOutBuf,
                                UInt    aOutSize )
{
    sdtSortPageHdr  * sHdr = (sdtSortPageHdr*)aPagePtr;
    UInt              sSlotValue;
    UInt              sSize;
    sdtTempPageType   sType;
    UInt              i;

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "\nDUMP TEMP PAGE:\n" );
    sSize = idlOS::strlen( aOutBuf );
    IDE_TEST( ideLog::ideMemToHexStr( (UChar*)aPagePtr,
                                      SD_PAGE_SIZE,
                                      IDE_DUMP_FORMAT_FULL,
                                      aOutBuf + sSize,
                                      aOutSize - sSize )
              != IDE_SUCCESS );

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "\n\nDUMP TEMP PAGE HEADER:\n"
                               "mTypeAndFreeOffset : %"ID_UINT32_FMT"\n"
                               "mPrevPID           : %"ID_UINT32_FMT"\n"
                               "mSelfPID           : %"ID_UINT32_FMT"\n"
                               "mNextPID           : %"ID_UINT32_FMT"\n"
                               "mSlotCount         : %"ID_UINT32_FMT"\n",
                               sHdr->mTypeAndFreeOffset,
                               sHdr->mPrevPID,
                               sHdr->mSelfPID,
                               sHdr->mNextPID,
                               sHdr->mSlotCount );

    sType = getType( sHdr );
    if ( ( SDT_TEMP_PAGETYPE_INIT <= sType ) &&
         ( sType < SDT_TEMP_PAGETYPE_MAX ) )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "mType              : %s\n",
                                   mPageName[ sType ] );
    }

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "\nSlot Array :" );

    for( i = 0 ; i < sHdr->mSlotCount; i ++ )
    {
        if ( ( i % 16 ) == 0 )
        {
            (void)idlVA::appendFormat( aOutBuf,
                                       aOutSize,
                                       "\n[%4"ID_UINT32_FMT"] ",
                                       i );
        }

        sSlotValue = getSlotOffset( (UChar*)aPagePtr, i );
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "%6"ID_UINT32_FMT, sSlotValue );
    }
    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n");

    return;

    IDE_EXCEPTION_END;

    return;
}

/***************************************************************************
 * Description : Sort Temp Page의 Header를 dump한다.
 *
 * aPagePtr   - [IN] dump 대상 Sort Temp Page
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtTempPage::dumpWAPageHeaders( void     * aWASegment,
                                     SChar    * aOutBuf,
                                     UInt       aOutSize )
{
    sdtSortSegHdr  * sWASegment = (sdtSortSegHdr*)aWASegment;
    SChar          * sTypeNamePtr;
    SChar            sInvalidName[] = "INVALID";
    sdtSortPageHdr * sPagePtr;
    sdtTempPageType  sType;
    sdtWCB         * sWCBPtr;
    sdtWCB         * sEndWCBPtr;
    scPageID         sWPageID;

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "TEMP PAGE HEADERS:\n"
                               "%10s %16s %10s %16s %10s %10s %10s %10s %10s\n",
                               "WPID",
                               "TAFO", /*TypeAndFreeOffset*/
                               "TYPE",
                               "TYPENAME",
                               "FREEOFF",
                               "PREVPID",
                               "SELFPID",
                               "NEXTPID",
                               "SLOTCNT" );

    sWCBPtr    = &(sWASegment->mWCBMap[0]);
    sEndWCBPtr = sWCBPtr + (sWASegment->mMaxWAExtentCount * SDT_WAEXTENT_PAGECOUNT);
    sWPageID   = 0;

    while( sWCBPtr < sEndWCBPtr )
    {
        sPagePtr = (sdtSortPageHdr *)sWCBPtr->mWAPagePtr;

        sWCBPtr++;

        if ( sPagePtr == NULL )
        {
            sWPageID++;
            continue;
        }

        if ( ( sPagePtr->mTypeAndFreeOffset == 0 ) &&
             ( sPagePtr->mPrevPID == 0 ) &&
             ( sPagePtr->mNextPID == 0 ) &&
             ( sPagePtr->mSlotCount  == 0 ) )
        {
            sWPageID++;
            continue;
        }

        sType = getType( sPagePtr );
        if ( ( SDT_TEMP_PAGETYPE_INIT <= sType ) &&
             ( sType < SDT_TEMP_PAGETYPE_MAX ) )
        {
            sTypeNamePtr = mPageName[ sType ];
        }
        else
        {
            sTypeNamePtr = sInvalidName;
        }

        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "%10"ID_UINT32_FMT
                                   " %16"ID_UINT32_FMT
                                   " %10"ID_UINT32_FMT
                                   " %16s"
                                   " %10"ID_UINT32_FMT
                                   " %10"ID_UINT32_FMT
                                   " %10"ID_UINT32_FMT
                                   " %10"ID_UINT32_FMT
                                   " %10"ID_UINT32_FMT"\n",
                                   sWPageID,
                                   sPagePtr->mTypeAndFreeOffset,
                                   getType( sPagePtr ),
                                   sTypeNamePtr,
                                   getFreeOffset( sPagePtr ),
                                   sPagePtr->mPrevPID,
                                   sPagePtr->mSelfPID,
                                   sPagePtr->mNextPID,
                                   sPagePtr->mSlotCount );
        sWPageID++;
    }

    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n" );

    return;
}
