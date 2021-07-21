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
 * $Id: sdpsfFindPage.h 25697 2008-04-18 07:27:36Z bskim $
 *
 * 본 파일은 Freelist Managed Segment에서 가용공간 탐색 연산의 STATIC
 * 인터페이스를 관리한다.
 *
 ***********************************************************************/

# ifndef _O_SDPSF_FREE_PIDLIST_H_
# define _O_SDPSF_FREE_PIDLIST_H_ 1

# include <sdpDef.h>

class sdpsfFreePIDList
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC walkAndUnlink( idvSQL               *aStatistics,
                                 sdrMtx               *aMtx,
                                 scSpaceID             aSpaceID,
                                 sdpsfSegHdr          *aSegHdr,
                                 UInt                  aRecordSize,
                                 idBool                aNeedKeySlot,
                                 UChar               **aPagePtr,
                                 UChar                *aCTSlotIdx );

    static IDE_RC addPage( sdrMtx             * aMtx,
                           sdpsfSegHdr        * aSegHdr,
                           UChar              * aPagePtr );

    static IDE_RC removePage( idvSQL             * aStatistics,
                              sdrMtx             * aMtx,
                              sdpsfSegHdr        * aSegHdr,
                              UChar              * aPrvPagePtr,
                              UChar              * aTgtPagePtr );

    static IDE_RC buildRecord4Dump( idvSQL              * /*aStatistics*/,
                                    void                * aHeader,
                                    void                * aDumpObj,
                                    iduFixedTableMemory * aMemory );
};


#endif // _O_SDPSF_FREE_PIDLIST_H_
