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
 * $Id: sdcUpdate.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 * 
 * 본 파일은 undo tablespace 관련 redo 함수에 대한 헤더파일이다.
 *
 **********************************************************************/

#ifndef _O_SDC_UPDATE_H_
#define _O_SDC_UPDATE_H_ 1

#include <smDef.h>
#include <sdcDef.h>

class sdcUpdate
{
public:

    /* type:  SDR_SDC_INSERT_UNDO_REC */
    static IDE_RC redo_SDR_SDC_INSERT_UNDO_REC( SChar       * aLogPtr,
                                                UInt          aSize,
                                                UChar       * aRecPtr,
                                                sdrRedoInfo * /*aRedoInfo*/,
                                                sdrMtx      * /* aMtx */ );

    /* type:  SDR_SDC_INIT_TSS */
    static IDE_RC redo_SDR_SDC_INIT_TSS( SChar       * aLogPtr,
                                         UInt          aSize,
                                         UChar       * aRecPtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * aMtx );
    /* type:  SDR_SDC_BIND_TSS */
    static IDE_RC redo_SDR_SDC_BIND_TSS( SChar       * aLogPtr,
                                         UInt          aSize,
                                         UChar       * aRecPtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * aMtx );

    /* type:  SDR_SDC_UNBIND_TSS */
    static IDE_RC redo_SDR_SDC_UNBIND_TSS( SChar       * aLogPtr,
                                           UInt          aSize,
                                           UChar       * aRecPtr,
                                           sdrRedoInfo * /*aRedoInfo*/,
                                           sdrMtx      * aMtx );

    /* type:  SDR_SDC_SET_INITSCN_TO_CTS */
    static IDE_RC redo_SDR_SDC_SET_INITSCN_TO_CTS( SChar       * aLogPtr,
                                                   UInt          aSize,
                                                   UChar       * aRecPtr,
                                                   sdrRedoInfo * /*aRedoInfo*/,
                                                   sdrMtx      * aMtx );
    /* type:  SDR_SDC_INIT_TSS_PAGE */
    static IDE_RC redo_SDR_SDC_INIT_TSS_PAGE( SChar       * aLogPtr,
                                              UInt          aSize,
                                              UChar       * aRecPtr,
                                              sdrRedoInfo * /*aRedoInfo*/,
                                              sdrMtx      * aMtx );
    /* type:  SDR_SDC_INIT_UNDO_PAGE */
    static IDE_RC redo_SDR_SDC_INIT_UNDO_PAGE( SChar       * aLogPtr,
                                               UInt          aSize,
                                               UChar       * aRecPtr,
                                               sdrRedoInfo * /*aRedoInfo*/,
                                               sdrMtx      * aMtx );
    /* PROJ-1704 redo type:  SDR_SDC_BIND_CTS */
    static IDE_RC redo_SDR_SDC_BIND_CTS( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * aRedoInfo,
                                         sdrMtx      * aMtx );

    /* PROJ-1704 redo type:  SDR_SDC_UNBIND_CTS */
    static IDE_RC redo_SDR_SDC_UNBIND_CTS( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * aRedoInfo,
                                           sdrMtx      * aMtx );

    /* PROJ-1704 redo type:  SDR_SDC_BIND_ROW */
    static IDE_RC redo_SDR_SDC_BIND_ROW( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * aRedoInfo,
                                         sdrMtx      * aMtx );

    /* PROJ-1704 redo type:  SDR_SDC_UNBIND_ROW */
    static IDE_RC redo_SDR_SDC_UNBIND_ROW( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * aRedoInfo,
                                           sdrMtx      * aMtx );

    /* PROJ-1704 redo type:  SDR_SDC_ROW_TIMESTAMPING */
    static IDE_RC redo_SDR_SDC_ROW_TIMESTAMPING( SChar       * aData,
                                                 UInt          aLength,
                                                 UChar       * aPagePtr,
                                                 sdrRedoInfo * aRedoInfo,
                                                 sdrMtx      * aMtx );

    static IDE_RC redo_SDR_SDC_DATA_SELFAGING( SChar       * aData,
                                               UInt          aLength,
                                               UChar       * aPagePtr,
                                               sdrRedoInfo * /*aRedoInfo*/,
                                               sdrMtx      * /*aMtx*/ );

    /* PROJ-1704 redo type:  SDR_SDP_INIT_CTL */
    static IDE_RC redo_SDR_SDC_INIT_CTL( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * aRedoInfo,
                                         sdrMtx      * aMtx );

    /* PROJ-1704 redo type:  SDR_SDP_EXTEND_CTL */
    static IDE_RC redo_SDR_SDC_EXTEND_CTL( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * aRedoInfo,
                                           sdrMtx      * aMtx );
};

#endif // _O_SDC_UPDATE_H_
