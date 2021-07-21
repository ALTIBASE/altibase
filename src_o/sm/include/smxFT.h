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
 * $Id: smxFT.h 37671 2010-01-08 02:28:34Z linkedlist $
 **********************************************************************/

#ifndef _O_SMX_FT_H_
#define _O_SMX_FT_H_ 1

#include <idu.h>
#include <smxTrans.h>
#include <smxTransFreeList.h>
#include <smxMinSCNBuild.h>


class smxFT
{
//For Operation
public:
    static void   initializeFixedTableArea();

    // X$TRANSACTION_MANAGER
    static IDE_RC buildRecordForTxMgr(idvSQL              * /*aStatistics*/,
                                      void                *aHeader,
                                      void                *aDumpObj,
                                      iduFixedTableMemory *aMemory);

    /* X$TIME_SCN */
    static IDE_RC buildRecordForTimeSCNList( idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory * aMemory );

    /* X$PENDING_WAIT */
    static IDE_RC buildRecordForPendingWait( idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory * aMemory );

    // X$TRANSACTIONS
    static IDE_RC buildRecordForTxList(idvSQL              * /*aStatistics*/,
                                       void                *aHeader,
                                       void                *aDumpObj,
                                       iduFixedTableMemory *aMemory);
    // X$TXPENDING
    static IDE_RC buildRecordForTxPending(idvSQL              * /*aStatistics*/,
                                          void                *aHeader,
                                          void                *aDumpObj,
                                          iduFixedTableMemory *aMemory);
  
    // X$ACTIVE_TXSEGS
    static IDE_RC buildRecordForActiveTXSEGS( idvSQL              * /*aStatistics*/,
                                              void                *aHeader,
                                              void                *aDumpObj,
                                              iduFixedTableMemory *aMemory);

    // X$LEGACY_TRANSACTIONS
    static IDE_RC buildRecordForLegacyTxList(idvSQL              * /*aStatistics*/,
                                             void                *aHeader,
                                             void                *aDumpObj,
                                             iduFixedTableMemory *aMemory);
};

#endif
