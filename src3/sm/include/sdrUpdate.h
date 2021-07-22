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
 * $Id: sdrUpdate.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 * 본 파일은 DRDB의 redo/undo function map에 대한 헤더파일이다.
 *
 **********************************************************************/

#ifndef _O_SDR_UPDATE_H_
#define _O_SDR_UPDATE_H_ 1

#include <smDef.h>
#include <sdd.h>
#include <sdrDef.h>

class sdrUpdate
{
public:

    /* 초기화 */
    static void initialize();

    /* dummy function  */
    static void destroy();

    /* BUG-25279 Btree for spatial과 Disk Btree의 자료구조 및 로깅 분리 
     * Btree for Spatial과 일반 Disk Btree간의 로깅 분리에 따라
     * Recovery시에 st layer에 매달린 redo/undo/nta undo 함수를 호출해야 한다.
     * 이를 위해 외부적으로 Undo Redo NTA Undo 함수를 매달 수 있도록 한다. */
    static void appendExternalUndoFunction( UInt                aUndoMapID,
                                            sdrDiskUndoFunction aDiskUndoFunction );

    static void appendExternalRedoFunction( UInt                aRedoMapID,
                                            sdrDiskRedoFunction aDiskRedoFunction );

    static void appendExternalRefNTAUndoFunction( UInt                      aNTARefUndoMapID,
                                                  sdrDiskRefNTAUndoFunction aDiskRefNTAUndoFunction );

    /* DRDB 로그의 undo 수행 */
    static IDE_RC doUndoFunction( idvSQL      * aStatistics,
                                  smTID         aTransID,
                                  smOID         aOID,
                                  SChar       * aData,
                                  smLSN       * aPrevLSN );
    
    /* DRDB 로그의 redo 처리 */
    static IDE_RC doRedoFunction( SChar       * aValue,
                                  UInt          aValueLen,
                                  UChar       * aPageOffset,
                                  sdrRedoInfo * aRedoInfo,
                                  sdrMtx      * aMtx );
    
    /* DRDB NTA로그의 operational undo 처리 */
    static IDE_RC doNTAUndoFunction( idvSQL      * aStatistics,
                                     void        * aTrans,
                                     UInt          aOPType,
                                     scSpaceID     aSpaceID,
                                     smLSN       * aPrevLSN,
                                     ULong       * aArrData,
                                     UInt          aDataCount );
    
    /* DRDB Index NTA로그의 operational undo 처리 */
    static IDE_RC doRefNTAUndoFunction( idvSQL      * aStatistics,
                                        void        * aTrans,
                                        UInt          aOPType,
                                        smLSN       * aPrevLSN,
                                        SChar       * aRefData );

private:
  
      static  IDE_RC redoNA( SChar       * aData,
                             UInt          aLength,
                             UChar       * aPagePtr,
                             sdrRedoInfo * aRedoInfo,
                             sdrMtx      * aMtx );
  
};

extern sdrDiskRedoFunction gSdrDiskRedoFunction[SM_MAX_RECFUNCMAP_SIZE];
extern sdrDiskUndoFunction gSdrDiskUndoFunction[SM_MAX_RECFUNCMAP_SIZE];

#endif // _O_SDR_UPDATE_H_


