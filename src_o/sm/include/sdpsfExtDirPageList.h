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
 * $Id$
 **********************************************************************/

#ifndef _O_SDPSF_EXTDIR_PAGELIST_H_
#define _O_SDPSF_EXTDIR_PAGELIST_H_ 1

#include <sdr.h>
#include <sdpsfExtDirPage.h>
#include <sdpDef.h>

class sdpsfExtDirPageList
{
public:
    static IDE_RC initialize( sdrMtx      * aMtx,
                              sdpsfSegHdr * aSegHdr );
    static IDE_RC destroy();

    static IDE_RC alloc();

    static IDE_RC allocPage( idvSQL              * aStatistics,
                             scSpaceID             aSpaceID,
                             sdpsfSegHdr         * aTbsHdr,
                             sdrMtx              * aMtx );

    static IDE_RC addExtDesc( idvSQL              * aStatistics,
                              sdrMtx              * aMtx,
                              scSpaceID             aSpaceID,
                              sdpsfSegHdr         * aSegHdr,
                              scPageID              aFstExtPID,
                              sdpsfExtDirCntlHdr ** aExtDirPageCntlHdr,
                              sdRID               * aNewExtRID,
                              sdpsfExtDesc       ** aNewExtDescPtr );

    static IDE_RC addNewExtDirPage2Tail( idvSQL       * aStatistics,
                                         sdrMtx       * aMtx,
                                         scSpaceID      aSpaceID,
                                         sdpsfSegHdr  * aSegHdr,
                                         UChar        * aExtDirPagePtr );

    static IDE_RC addPage2Tail( idvSQL               * aStatistics,
                                sdrMtx               * aMtx,
                                sdpsfSegHdr          * aSegHdr,
                                sdpsfExtDirCntlHdr   * aExtDirPageHdr );

    static IDE_RC unlinkPage( idvSQL               * aStatistics,
                              sdrMtx               * aMtx,
                              sdpsfSegHdr          * aSegHdr,
                              sdpsfExtDirCntlHdr   * aExtDirCntlHdr );

    static IDE_RC getLstExtDirPage4Update( idvSQL               * aStatistics,
                                           sdrMtx               * aMtx,
                                           scSpaceID              aSpaceID,
                                           sdpsfSegHdr          * aSegHdr,
                                           UChar               ** aExtDirPagePtr,
                                           sdpsfExtDirCntlHdr  ** aExtDirCntlHdr );

    static IDE_RC dump( sdpsfSegHdr * aTbsHdr );

/* inline function */
public:
    static inline idBool isNeedNewExtDirPage( sdpsfSegHdr * aSegHdr );
};

inline idBool sdpsfExtDirPageList::isNeedNewExtDirPage( sdpsfSegHdr * aSegHdr )
{
    ULong   sTotExtCnt         = aSegHdr->mTotExtCnt;
    UShort  sMaxExtCntInSegHdr = aSegHdr->mExtDirCntlHdr.mMaxExtDescCnt;
    idBool  sIsFull;

    sIsFull = ID_TRUE;

    if( sTotExtCnt > sMaxExtCntInSegHdr )
    {
        sTotExtCnt -= sMaxExtCntInSegHdr;

        if( sTotExtCnt % aSegHdr->mMaxExtCntInExtDirPage == 0 )
        {
            sIsFull = ID_TRUE;
        }
        else
        {
            sIsFull = ID_FALSE;
        }
    }
    else
    {
        if( sTotExtCnt < sMaxExtCntInSegHdr )
        {
            sIsFull = ID_FALSE;
        }
        else
        {
            sIsFull = ID_TRUE;
        }
    }

    return sIsFull;
}

#endif // _O_SDPTL_EXTDIR_PAGELIST_H_
