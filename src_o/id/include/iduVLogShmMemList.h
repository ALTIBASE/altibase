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
#if !defined(_O_IDU_VLOG_MEMLIST_H_)
#define _O_IDU_VLOG_MEMLIST_H_ 1

#include <idrDef.h>

//TODO

typedef enum iduVLogShmMemListType
{
    IDU_VLOG_TYPE_SHM_MEMLIST_NONE,

    IDU_VLOG_TYPE_SHM_MEMLIST_INITIALIZE,

    IDU_VLOG_TYPE_SHM_MEMLIST_LINK_WITH_AFTER,
    IDU_VLOG_TYPE_SHM_MEMLIST_LINK_ONLY_BEFORE,
    IDU_VLOG_TYPE_SHM_MEMLIST_UNLINK_WITH_AFTER,
    IDU_VLOG_TYPE_SHM_MEMLIST_UNLINK_ONLY_BEFORE,

    IDU_VLOG_TYPE_SHM_MEMLIST_ALLOC,
    IDU_VLOG_TYPE_SHM_MEMLIST_ALLOC_MYCHUNK,
    IDU_VLOG_TYPE_SHM_MEMLIST_MEMFREE,

    IDU_VLOG_TYPE_SHM_MEMLIST_FREECHUNKCNT,
    IDU_VLOG_TYPE_SHM_MEMLIST_PARTCHUNKCNT,
    IDU_VLOG_TYPE_SHM_MEMLIST_FULLCHUNKCNT,
    IDU_VLOG_TYPE_SHM_MEMLIST_FREESLOTCNT,

    IDU_VLOG_TYPE_SHM_MEMLIST_MAX

} iduVLogShmMemListType;

class iduVLogShmMemList
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC writeInitialize( iduShmTxInfo     * aShmTxInfo,
                                   idrLSN             aNTALsn,
                                   iduStShmMemList  * aMemListInfo );

    static IDE_RC undo_SHM_MEMLIST_INITIALIZE( idvSQL       * /*aStatistics*/,
                                               iduShmTxInfo * aShmTxInfo,
                                               idrLSN       /*aNTALsn*/,
                                               UShort         aSize,
                                               SChar        * aImage );

    static IDE_RC writeLinkWithAfter( iduShmTxInfo     * aShmTxInfo,
                                      iduStShmMemChunk * aBefore,
                                      iduStShmMemChunk * aChunk,
                                      iduStShmMemChunk * sAfter );

    static IDE_RC undo_SHM_MEMLIST_LINK_WITH_AFTER( idvSQL       * /*aStatistics*/,
                                                    iduShmTxInfo * aShmTxInfo,
                                                    idrLSN       /*aNTALsn*/,
                                                    UShort         aSize,
                                                    SChar        * aImage );

    static IDE_RC writeLinkOnlyBefore( iduShmTxInfo     * aShmTxInfo,
                                       iduStShmMemChunk * aBefore,
                                       iduStShmMemChunk * aChunk );

    static IDE_RC undo_SHM_MEMLIST_LINK_ONLY_BEFORE( idvSQL       * /*aStatistics*/,
                                                     iduShmTxInfo * aShmTxInfo,
                                                     idrLSN       /*aNTALsn*/,
                                                     UShort         aSize,
                                                     SChar        * aImage );

    static IDE_RC writeUnlinkWithAfter( iduShmTxInfo     * aShmTxInfo,
                                        iduStShmMemChunk * sBefore,
                                        iduStShmMemChunk * sAfter );

    static IDE_RC undo_SHM_MEMLIST_UNLINK_WITH_AFTER( idvSQL       * /*aStatistics*/,
                                                      iduShmTxInfo * aShmTxInfo,
                                                      idrLSN       /*aNTALsn*/,
                                                      UShort         aSize,
                                                      SChar        * aImage );

    static IDE_RC writeUnlinkOnlyBefore( iduShmTxInfo     * aShmTxInfo,
                                         iduStShmMemChunk * sBefore );

    static IDE_RC undo_SHM_MEMLIST_UNLINK_ONLY_BEFORE( idvSQL       * /*aStatistics*/,
                                                       iduShmTxInfo * aShmTxInfo,
                                                       idrLSN       /*aNTALsn*/,
                                                       UShort         aSize,
                                                       SChar        * aImage );

    static IDE_RC writeAllocMyChunk( iduShmTxInfo     * aShmTxInfo,
                                     iduStShmMemChunk * aMyChunk );

    static IDE_RC undo_SHM_MEMLIST_ALLOC_MYCHUNK( idvSQL       * /*aStatistics*/,
                                                  iduShmTxInfo * aShmTxInfo,
                                                  idrLSN       /*aNTALsn*/,
                                                  UShort         aSize,
                                                  SChar        * aImage );

    static IDE_RC writeNTAAlloc(iduShmTxInfo    * aShmTxInfo,
                                idrLSN            aNTALsn,
                                iduStShmMemList * aMemListInfo,
                                idShmAddr         aAddr4Mem  );

    static IDE_RC undo_SHM_MEMLIST_ALLOC( idvSQL       * /*aStatistics*/,
                                          iduShmTxInfo * aShmTxInfo,
                                          idrLSN       /*aNTALsn*/,
                                          UShort         aSize,
                                          SChar        * aImage );

    static IDE_RC writeMemFree( iduShmTxInfo     * aShmTxInfo,
                                iduStShmMemChunk * aCurChunk,
                                iduStShmMemSlot  * sFreeElem );

    static IDE_RC writeFreeChunkCnt( iduShmTxInfo    * aShmTxInfo,
                                     iduStShmMemList * aMemListInfo );

    static IDE_RC writePartChunkCnt( iduShmTxInfo    * aShmTxInfo,
                                     iduStShmMemList * aMemListInfo );

    static IDE_RC writeFullChunkCnt( iduShmTxInfo    * aShmTxInfo,
                                     iduStShmMemList * aMemListInfo );

    static IDE_RC writeFreeSlotCnt( iduShmTxInfo     * aShmTxInfo,
                                    iduStShmMemChunk * aMyChunk );

    static IDE_RC undo_SHM_MEMLIST_MEMFREE( idvSQL         * /*aStatistics*/,
                                            iduShmTxInfo   * aShmTxInfo,
                                            idrLSN           /*aNTALsn*/,
                                            UShort           aSize,
                                            SChar          * aImage );

    static IDE_RC undo_SHM_MEMLIST_FREECHUNKCNT( idvSQL       * /*aStatistics*/,
                                                 iduShmTxInfo * aShmTxInfo,
                                                 idrLSN       /*aNTALsn*/,
                                                 UShort         aSize,
                                                 SChar        * aImage );

    static IDE_RC undo_SHM_MEMLIST_PARTCHUNKCNT( idvSQL       * /*aStatistics*/,
                                                 iduShmTxInfo * aShmTxInfo,
                                                 idrLSN       /*aNTALsn*/,
                                                 UShort         aSize,
                                                 SChar        * aImage );

    static IDE_RC undo_SHM_MEMLIST_FULLCHUNKCNT( idvSQL       * /*aStatistics*/,
                                                 iduShmTxInfo * aShmTxInfo,
                                                 idrLSN       /*aNTALsn*/,
                                                 UShort         aSize,
                                                 SChar        * aImage );

    static IDE_RC undo_SHM_MEMLIST_FREESLOTCNT( idvSQL       * /*aStatistics*/,
                                                iduShmTxInfo * aShmTxInfo,
                                                idrLSN       /*aNTALsn*/,
                                                UShort         aSize,
                                                SChar        * aImage );

public:
    static idrUndoFunc mArrUndoFunction[IDU_VLOG_TYPE_SHM_MEMLIST_MAX];
    static SChar       mStrVLogType[IDU_VLOG_TYPE_SHM_MEMLIST_MAX+1][100];
};

#endif /* _O_IDU_VLOG_MEMLIST_H_ */
