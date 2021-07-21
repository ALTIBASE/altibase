/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/
#if !defined(_O_IDU_VLOG_SHMMGR_H_)
#define _O_IDU_VLOG_SHMMGR_H_ 1

#include <idrDef.h>

typedef enum iduVLogShmMgrType
{
    IDU_VLOG_TYPE_SHM_MGR_NONE,
    IDU_VLOG_TYPE_SHM_MGR_REGISTER_NEW_SEG,
    IDU_VLOG_TYPE_SHM_MGR_REGISTER_OLD_SEG,
    IDU_VLOG_TYPE_SHM_MGR_ALLOC_SEG,
    IDU_VLOG_TYPE_SHM_MGR_INSERT_FREE_BLOCK,
    IDU_VLOG_TYPE_SHM_MGR_REMOVE_FREE_BLOCK,
    IDU_VLOG_TYPE_SHM_MGR_SPLIT_BLOCK,
    IDU_VLOG_TYPE_SHM_MGR_SET_BLOCK_PREV,
    IDU_VLOG_TYPE_SHM_MGR_SET_BLOCK_SIZE,
    IDU_VLOG_TYPE_SHM_MGR_ALLOC_MEM,
    IDU_VLOG_TYPE_SHM_MGR_MAX
} iduVLogShmMgrType;

class iduVLogShmMgr
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC writeRegisterNewSeg( iduShmTxInfo * aShmTxInfo,
                                       ULong          aSegCnt,
                                       ULong          aFstFreeSegInfoIdx );

    static IDE_RC undo_SHM_MGR_REGISTER_NEW_SEG( idvSQL       * aStatistics,
                                                 iduShmTxInfo * aShmTxInfo,
                                                 idrLSN       /*aNTALsn*/,
                                                 UShort         aSize,
                                                 SChar        * aImage );

    static IDE_RC writeRegisterOldSeg( iduShmTxInfo * aShmTxInfo,
                                       ULong          aSegID,
                                       ULong          aAttachSegCnt );

    static IDE_RC undo_SHM_MGR_REGISTER_OLD_SEG( idvSQL       * aStatistics,
                                                 iduShmTxInfo * aShmTxInfo,
                                                 idrLSN       /*aNTALsn*/,
                                                 UShort         aSize,
                                                 SChar        * aImage );

    static IDE_RC writeAllocSeg( iduShmTxInfo * aShmTxInfo,
                                 iduSCH      *  aSCH );

    static IDE_RC undo_SHM_MGR_REGISTER_ALLOC_SEG( idvSQL       * aStatistics,
                                                   iduShmTxInfo * /*aShmTxInfo*/,
                                                   idrLSN         /*aNTALsn*/,
                                                   UShort           aSize,
                                                   SChar          * aImage );


    static IDE_RC writeInsertFreeBlock( iduShmTxInfo * aShmTxInfo,
                                        UInt           aFstLvlIdx,
                                        UInt           aSndLvlIdx,
                                        iduShmBlockHeader * aBlockHdr );

    static IDE_RC undo_SHM_MGR_INSERT_FREE_BLOCK( idvSQL       * aStatistics,
                                                  iduShmTxInfo * /*aShmTxInfo*/,
                                                  idrLSN         /*aNTALsn*/,
                                                  UShort         aSize,
                                                  SChar        * aImage );

    static IDE_RC writeRemoveFreeBlock( iduShmTxInfo      * aShmTxInfo,
                                        iduShmBlockHeader * aBlockHdr,
                                        UInt                aFstLvlIdx,
                                        UInt                aSndLvlIdx );

    static IDE_RC undo_SHM_MGR_REMOVE_FREE_BLOCK( idvSQL       * aStatistics,
                                                  iduShmTxInfo * /*aShmTxInfo*/,
                                                  idrLSN         /*aNTALsn*/,
                                                  UShort           aSize,
                                                  SChar          * aImage );

    static IDE_RC writeSplitBlock( iduShmTxInfo      * aShmTxInfo,
                                   iduShmBlockHeader * aBlockHdr );

    static IDE_RC undo_SHM_MGR_SPLIT_BLOCK( idvSQL       * aStatistics,
                                            iduShmTxInfo * /*aShmTxInfo*/,
                                            idrLSN         /*aNTALsn*/,
                                            UShort         /*aSize*/,
                                            SChar          * aImage );

    static IDE_RC writeSetBlockPrev( iduShmTxInfo      * aShmTxInfo,
                                     iduShmBlockHeader * aBlockHdr );

    static IDE_RC undo_SHM_MGR_SET_BLOCK_PREV( idvSQL       * aStatistics,
                                               iduShmTxInfo * /*aShmTxInfo*/,
                                               idrLSN         /*aNTALsn*/,
                                               UShort         /*aSize*/,
                                               SChar          * aImage );

    static IDE_RC writeSetBlockSize( iduShmTxInfo      * aShmTxInfo,
                                     iduShmBlockHeader * aBlockHdr );

    static IDE_RC undo_SHM_MGR_SET_BLOCK_SIZE( idvSQL       * aStatistics,
                                               iduShmTxInfo * /*aShmTxInfo*/,
                                               idrLSN         /*aNTALsn*/,
                                               UShort         /*aSize*/,
                                               SChar        * aImage );

    static IDE_RC writeAllocMem( iduShmTxInfo      * aShmTxInfo,
                                 idrLSN              aPrvLSN,
                                 iduShmBlockHeader * aBlockHdr );

    static IDE_RC undo_SHM_MGR_ALLOC_MEM( idvSQL       * aStatistics,
                                          iduShmTxInfo * aShmTxInfo,
                                          idrLSN        /*aNTALsn*/,
                                          UShort        /*aSize*/,
                                          SChar        * aImage );
public:
    static idrUndoFunc mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_MAX];
    static SChar       mStrVLogType[IDU_VLOG_TYPE_SHM_MGR_MAX+1][100];
};

#endif /* _O_IDU_VLOG_SHMMGR_H_ */
