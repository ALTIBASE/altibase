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
 * $Id: smlFT.h 36756 2009-11-16 11:58:29Z et16 $
 **********************************************************************/

#ifndef _O_SML_FT_H_
#define _O_SML_FT_H_ 1

#include <idu.h>
#include <smErrorCode.h>
#include <smiDef.h>
#include <smlDef.h>
#include <smu.h>
#include <smlLockMgr.h>
#include <iddRBTree.h>

class smlFT
{
private:
    // TBL,TBS,DBF에 LockNode를 FixedTable의 Record로 생성한다. 
    static IDE_RC getLockItemNodes( void                 *aHeader,
                                    iduFixedTableMemory *aMemory,
                                    smlLockItem         *aLockItem );
    static IDE_RC getLockNodesFromTrans( idvSQL              *aStatistics,
                                         void                *aHeader,
                                         iduFixedTableMemory *aMemory,
                                         smiLockItemType      aLockTableType,
                                         iddRBTree           *aLockItemTree );

public:
    static SInt compLockItemID( const void* aLeft, const void* aRight );

    // X$LOCK
    static  IDE_RC  buildRecordForLockTBL(idvSQL      *aStatistics,
                                          void        *aHeader,
                                          void        *aDumpObj,
                                          iduFixedTableMemory *aMemory);

    // X$LOCK_TABLESPACE
    static  IDE_RC  buildRecordForLockTBS(idvSQL      *aStatistics,
                                          void        *aHeader,
                                          void        *aDumpObj,
                                          iduFixedTableMemory *aMemory);

    // X$LOCK_MODE
    static IDE_RC  buildRecordForLockMode(idvSQL      *aStatistics,
                                          void        *aHeader,
                                          void        *aDumpObj,
                                          iduFixedTableMemory *aMemory);

    // X$LOCK_WAIT
    static IDE_RC  buildRecordForLockWait(idvSQL      *aStatistics,
                                          void        *aHeader,
                                          void        *aDumpObj,
                                          iduFixedTableMemory *aMemory);

    /* X$LOCK_RECORD */
    static IDE_RC  buildRecordForLockRecord( idvSQL              * aStatistics,
                                             void                * aHeader,
                                             void                * aDumpObj,
                                             iduFixedTableMemory * aMemory );
    /* X$DIST_LOCK_WAIT */
    static IDE_RC  buildRecordForDistLockWait( idvSQL              * aStatistics,
                                               void                * aHeader,
                                               void                * aDumpObj,
                                               iduFixedTableMemory * aMemory );
};

#endif
