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

# ifndef _O_SDPSF_UFMT_PIDLIST_H_
# define _O_SDPSF_UFMT_PIDLIST_H_ 1

# include <sdpDef.h>
# include <sdpSglPIDList.h>

class sdpsfUFmtPIDList
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC add2Head( sdrMtx             * aMtx,
                            sdpsfSegHdr        * aSegHdr,
                            UChar              * aPagePtr );

    static IDE_RC removeAtHead( idvSQL             * aStatistics,
                                sdrMtx             * aRmvMtx,
                                sdrMtx             * aCrtMtx,
                                scSpaceID            aSpaceID,
                                sdpsfSegHdr        * aSegHdr,
                                sdpPageType          aPageType,
                                scPageID           * aAllocPID,
                                UChar             ** aPagePtr );

    static IDE_RC buildRecord4Dump( idvSQL              * /*aStatistics*/,
                                    void                * aHeader,
                                    void                * aDumpObj,
                                    iduFixedTableMemory * aMemory );

    inline static ULong getPageCnt( sdpsfSegHdr * aSegHdr );

private:
    static scPageID getFstPageID( sdpsfSegHdr * aSegHdr );

};

inline ULong sdpsfUFmtPIDList::getPageCnt( sdpsfSegHdr  * aSegHdr )
{
    return sdpSglPIDList::getNodeCnt( &( aSegHdr->mUFmtPIDList ) );

}

#endif // _O_SDPSF_UFMT_PIDLIST_H_
