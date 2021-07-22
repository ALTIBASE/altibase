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
 * $Id: sdnUpdate.h 88414 2020-08-25 04:45:02Z justin.kwon $
 *
 * Description :
 * 
 * 본 파일은 index 관련 redo 함수에 대한 헤더파일이다.
 *
 **********************************************************************/

#ifndef _O_SDN_UPDATE_H_
#define _O_SDN_UPDATE_H_ 1

#include <smDef.h>
#include <sdnbDef.h>

class sdnUpdate
{
    
public:
    
    /* type:  SDR_SDN_INSERT_INDEX_KEY */
    static IDE_RC redo_SDR_SDN_INSERT_INDEX_KEY( SChar       * aLogPtr,
                                                 UInt          aSize,
                                                 UChar       * aRecPtr,
                                                 sdrRedoInfo * /* aRedoInfo */,
                                                 sdrMtx      * /* aMtx */ );

    /* type:  SDR_SDN_INSERT_UNIQUE_KEY */
    static IDE_RC redo_SDR_SDN_INSERT_UNIQUE_KEY( SChar       * aLogPtr,
                                                  UInt          aSize,
                                                  UChar       * aRecPtr,
                                                  sdrRedoInfo * /* aRedoInfo */,
                                                  sdrMtx      * /* aMtx */ );
    
    static IDE_RC undo_SDR_SDN_INSERT_UNIQUE_KEY( idvSQL   * aStatistics,
                                                  void     * aTrans,
                                                  sdrMtx   * aMtx,
                                                  scGRID     aGRID,
                                                  SChar    * aLogPtr,
                                                  UInt       aSize );

    /* type:  SDR_SDN_INSERT_DUP_KEY */
    static IDE_RC redo_SDR_SDN_INSERT_DUP_KEY( SChar       * aLogPtr,
                                               UInt          aSize,
                                               UChar       * aRecPtr,
                                               sdrRedoInfo * /* aRedoInfo */,
                                               sdrMtx      * /* aMtx */ );
    
    static IDE_RC undo_SDR_SDN_INSERT_DUP_KEY( idvSQL   * aStatistics,
                                               void     * aTrans,
                                               sdrMtx   * aMtx,
                                               scGRID     aGRID,
                                               SChar    * aLogPtr,
                                               UInt       aSize );

    /* type:  SDR_SDN_FREE_INDEX_KEY */
    static IDE_RC redo_SDR_SDN_FREE_INDEX_KEY( SChar       * aData,
                                               UInt          aLength,
                                               UChar       * aPagePtr,
                                               sdrRedoInfo * /* aRedoInfo */,
                                               sdrMtx      * aMtx );
    
    /* type:  SDR_SDN_DELETE_KEY_WITH_NTA*/
    static IDE_RC redo_SDR_SDN_DELETE_KEY_WITH_NTA( SChar       * aData,
                                                    UInt          aLength,
                                                    UChar       * aPagePtr,
                                                    sdrRedoInfo * /* aRedoInfo */,
                                                    sdrMtx      * aMtx );
    
    static IDE_RC undo_SDR_SDN_DELETE_KEY_WITH_NTA( idvSQL   * aStatistics,
                                                    void     * aTrans,
                                                    sdrMtx   * aMtx,
                                                    scGRID     aGRID,
                                                    SChar    * aLogPtr,
                                                    UInt       aSize );
    
    /* type:  SDR_SDN_FREE_KEYS */
    static IDE_RC redo_SDR_SDN_FREE_KEYS( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /* aRedoInfo */,
                                          sdrMtx      * aMtx );
    
    /* type:  SDR_SDN_COMPACT_INDEX_PAGE */
    static IDE_RC redo_SDR_SDN_COMPACT_INDEX_PAGE( SChar       * aLogPtr,
                                                   UInt          aSize,
                                                   UChar       * aRecPtr,
                                                   sdrRedoInfo * /* aRedoInfo */,
                                                   sdrMtx      * /* aMtx */ );

    /* type:  SDR_SDN_KEY_STAMPING */
    static IDE_RC redo_SDR_SDN_KEY_STAMPING( SChar       * aLogPtr,
                                             UInt          aSize,
                                             UChar       * aRecPtr,
                                             sdrRedoInfo * /* aRedoInfo */,
                                             sdrMtx      * /* aMtx */ );
    
    /* type:  SDR_SDN_INIT_CTL */
    static IDE_RC redo_SDR_SDN_INIT_CTL( SChar       * aLogPtr,
                                         UInt          aSize,
                                         UChar       * aRecPtr,
                                         sdrRedoInfo * /* aRedoInfo */,
                                         sdrMtx      * /* aMtx */ );

    /* type:  SDR_SDN_EXTEND_CTL */
    static IDE_RC redo_SDR_SDN_EXTEND_CTL( SChar       * aLogPtr,
                                           UInt          aSize,
                                           UChar       * aRecPtr,
                                           sdrRedoInfo * /* aRedoInfo */,
                                           sdrMtx      * /* aMtx */ );

    /* type:  SDR_SDN_FREE_CTS */
    static IDE_RC redo_SDR_SDN_FREE_CTS( SChar       * aLogPtr,
                                         UInt          aSize,
                                         UChar       * aRecPtr,
                                         sdrRedoInfo * /* aRedoInfo */,
                                         sdrMtx      * /* aMtx */ );

    static IDE_RC getIndexInfoToVerify( SChar     * aLogPtr,
                                        smOID     * aTableOID,
                                        smOID     * aIndexOID,
                                        scSpaceID * aSpaceID );


private:

    static void getRollbackContext( sdrLogType            aLogType,
                                    SChar               * aLogPtr,
                                    sdnbRollbackContext * aContext);

    static void getRollbackContextEx( sdrLogType              aLogType,
                                      SChar                 * aLogPtr,
                                      sdnbRollbackContextEx * aContextEx );

};

#endif // _O_SDN_UPDATE_H_


