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
#if !defined(_O_IDU_VLOG_HASH_H_)
#define _O_IDU_VLOG_HASH_H_ 1

#include <idrDef.h>


typedef enum iduVLogShmHashType
{
    IDU_VLOG_TYPE_SHM_HASH_NONE,
    IDU_VLOG_TYPE_SHM_HASH_CUTNODE,
    IDU_VLOG_TYPE_SHM_HASH_INSERT_NOLATCH,
    IDU_VLOG_TYPE_SHM_HASH_UPDATE_ADDR_BUCKET_LIST,
    IDU_VLOG_TYPE_SHM_HASH_MAX
} iduVLogShmHashType;

class iduVLogShmHash
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    //TODO kj0225.park
    static IDE_RC writeNTAInsertNodeNoLatch( iduShmTxInfo       * aShmTxInfo,
                                             idrLSN               sNTALsn,
                                             iduLtShmHashBase   * aBase,
                                             iduStShmHashBucket * aBucket,
                                             void               * aKeyPtr,
                                             UInt                 aKeyLength );

    static IDE_RC undo_SHM_HASH_INSERT_NOLATCH( idvSQL       * /*aStatistics*/,
                                                iduShmTxInfo * aShmTxInfo,
                                                idrLSN         /*aNTALsn*/,
                                                UShort         aSize,
                                                SChar        * aImage );

    static IDE_RC writeCutNode( iduShmTxInfo     * aShmTxInfo,
                                iduStShmHashBase * aStHashBase );

    static IDE_RC undo_SHM_HASH_CUTNODE( idvSQL       * /*aStatistics*/,
                                         iduShmTxInfo * aShmTxInfo,
                                         idrLSN         /*aNTALsn*/,
                                         UShort         aSize,
                                         SChar        * aImage );

    static IDE_RC writeClose( iduShmTxInfo      * aShmTxInfo,
                              iduStShmHashBase  * aStHashBase );

    static IDE_RC undo_SHM_HASH_CLOSE( idvSQL         * /*aStatistics*/,
                                       iduShmTxInfo   * aShmTxInfo,
                                       idrLSN           /*aNTALsn*/,
                                       UShort           aSize,
                                       SChar          * aImage );
    static IDE_RC writeUpdateAddrBucketList( iduShmTxInfo     * aShmTxInfo,
                                             iduStShmHashBase * aStHashBase );

    static IDE_RC undo_SHM_HASH_UPDATE_ADDR_BUCKET_LIST( idvSQL       * /*aStatistics*/,
                                                         iduShmTxInfo * /*aShmTxInfo*/,
                                                         idrLSN         /*aNTALsn*/,
                                                         UShort         /*aSize*/,
                                                         SChar        * aImage );

public:
    static idrUndoFunc mArrUndoFunction[IDU_VLOG_TYPE_SHM_HASH_MAX];
    static SChar       mStrVLogType[IDU_VLOG_TYPE_SHM_HASH_MAX+1][100];
};

#endif /* _O_IDU_VLOG_HASH_H_ */
