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
 *
 * Description :
 * 
 * 본 파일은 Disk R-Tree Index 관련 redo/undo 함수에 대한 헤더파일이다.
 *
 **********************************************************************/

#ifndef _O_STNDR_UPDATE_H_
#define _O_STNDR_UPDATE_H_ 1



#include <smDef.h>
#include <stndrDef.h>

class stndrUpdate
{
    
public:
    
    /* type:  SDR_STNDR_INSERT_INDEX_KEY */
    static IDE_RC redo_SDR_STNDR_INSERT_INDEX_KEY( SChar       * aLogPtr,
                                                   UInt          aSize,
                                                   UChar       * aRecPtr,
                                                   sdrRedoInfo * /* aRedoInfo */,
                                                   sdrMtx      * /* aMtx */ );

    /* type: redo_SDR_STNDR_UPDATE_INDEX_KEY */
    static IDE_RC redo_SDR_STNDR_UPDATE_INDEX_KEY( SChar        * aLogPtr,
                                                   UInt           aSize,
                                                   UChar        * aRecPtr,
                                                   sdrRedoInfo  * /*aRedoInfo*/,
                                                   sdrMtx       * /* aMtx */ );

    /* type:  SDR_STNDR_FREE_INDEX_KEY */
    static IDE_RC redo_SDR_STNDR_FREE_INDEX_KEY( SChar       * aData,
                                                 UInt          aLength,
                                                 UChar       * aPagePtr,
                                                 sdrRedoInfo * /* aRedoInfo */,
                                                 sdrMtx      * aMtx );
    
    /* type:  SDR_STNDR_INSERT_KEY */
    static IDE_RC redo_SDR_STNDR_INSERT_KEY( SChar       * aLogPtr,
                                             UInt          aSize,
                                             UChar       * aRecPtr,
                                             sdrRedoInfo * /* aRedoInfo */,
                                             sdrMtx      * /* aMtx */ );
    
    static IDE_RC undo_SDR_STNDR_INSERT_KEY( idvSQL   * aStatistics,
                                             void     * aTrans,
                                             sdrMtx   * aMtx,
                                             scGRID     /*aGRID*/,
                                             SChar    * aLogPtr,
                                             UInt       aSize );

    /* type:  SDR_STNDR_DELETE_KEY_WITH_NTA*/
    static IDE_RC redo_SDR_STNDR_DELETE_KEY_WITH_NTA( SChar       * aData,
                                                      UInt          aLength,
                                                      UChar       * aPagePtr,
                                                      sdrRedoInfo * /* aRedoInfo */,
                                                      sdrMtx      * aMtx );
    
    static IDE_RC undo_SDR_STNDR_DELETE_KEY_WITH_NTA( idvSQL   * aStatistics,
                                                      void     * aTrans,
                                                      sdrMtx   * aMtx,
                                                      scGRID     /*aGRID*/,
                                                      SChar    * aLogPtr,
                                                      UInt       aSize );
    
    /* type:  SDR_STNDR_FREE_KEYS */
    static IDE_RC redo_SDR_STNDR_FREE_KEYS( SChar       * aData,
                                            UInt          aLength,
                                            UChar       * aPagePtr,
                                            sdrRedoInfo * /* aRedoInfo */,
                                            sdrMtx      * aMtx );
    
    /* type:  SDR_STNDR_COMPACT_INDEX_PAGE */
    static IDE_RC redo_SDR_STNDR_COMPACT_INDEX_PAGE( SChar       * aLogPtr,
                                                     UInt          aSize,
                                                     UChar       * aRecPtr,
                                                     sdrRedoInfo * /* aRedoInfo */,
                                                     sdrMtx      * /* aMtx */ );

    /* type:  SDR_STNDR_MAKE_CHAINED_KEYS */
    static IDE_RC redo_SDR_STNDR_MAKE_CHAINED_KEYS( SChar       * aLogPtr,
                                                    UInt          aSize,
                                                    UChar       * aRecPtr,
                                                    sdrRedoInfo * /* aRedoInfo */,
                                                    sdrMtx      * /* aMtx */ );
    
    /* type:  SDR_STNDR_MAKE_UNCHAINED_KEYS */
    static IDE_RC redo_SDR_STNDR_MAKE_UNCHAINED_KEYS( SChar       * aLogPtr,
                                                      UInt          aSize,
                                                      UChar       * aRecPtr,
                                                      sdrRedoInfo * /* aRedoInfo */,
                                                      sdrMtx      * /* aMtx */ );
    
    /* type:  SDR_STNDR_KEY_STAMPING */
    static IDE_RC redo_SDR_STNDR_KEY_STAMPING( SChar       * aLogPtr,
                                               UInt          aSize,
                                               UChar       * aRecPtr,
                                               sdrRedoInfo * /* aRedoInfo */,
                                               sdrMtx      * /* aMtx */ );
};



#endif // _O_STNDR_UPDATE_H_


