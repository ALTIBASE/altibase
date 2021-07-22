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
 * $Id: sdptbVerifyAndDump.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * Bitmap based TBS를 verify하고 dump하기 위한  루틴들이다.
 ***********************************************************************/

# ifndef _O_SDPTB_VERIFY_AND_DUMP_H_
# define _O_SDPTB_VERIFY_AND_DUMP_H_ 1

class sdptbVerifyAndDump
{
public:
    static IDE_RC initialize(){ return IDE_SUCCESS; }
    static IDE_RC destroy(){ return IDE_SUCCESS; }

    static IDE_RC dump( scSpaceID   aSpaceID,
                        UInt        aDumpFlag );

    static void printStructureSizes(void);

    static void printBitmapOfLG( sdptbLGHdr *aPagePtr ,
                                 SChar      *aOutBuf ,
                                 UInt        aOutSize );

    static IDE_RC dumpGGHdr( UChar *aPage ,
                             SChar *aOutBuf ,
                             UInt   aOutSize );

    static IDE_RC dumpLGHdr( UChar *aPage ,
                             SChar *aOutBuf ,
                             UInt   aOutSize );

    static IDE_RC printLGsCore( sdrMtx     *aMtx,
                                scSpaceID   aSpaceID,
                                sdptbGGHdr *aGGHdr,
                                UInt        aType );

    static void printAllocLGs( sdrMtx      *   aMtx,
                               scSpaceID       aSpaceID,
                               sdptbGGHdr  *   aGGHdr );

    static void printDeallocLGs( sdrMtx      * aMtx,
                                 scSpaceID     aSpaceID,
                                 sdptbGGHdr  * aGGHdr );

    static IDE_RC verify( idvSQL   * aStatistics,
                          scSpaceID  aSpaceID,
                          UInt       aFlag );

    static IDE_RC verifyGG( idvSQL    * aStatistics,
                            scSpaceID   aSpaceID,
                            UInt        aGGID );

    static IDE_RC verifyLG( idvSQL      * aStatistics,
                            scSpaceID     aSpaceID,
                            sdptbGGHdr  * aGGHdr,
                            UInt          aLGID,
                            sdptbLGType   aType,
                            UInt        * aFreeCnt,
                            idBool      * aSuccess );
};


#endif  // _O_SDPTB_VERIFY_AND_DUMP_H_
