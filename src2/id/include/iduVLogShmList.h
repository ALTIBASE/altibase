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
#if !defined(_O_IDU_VLOG_LIST_H_)
#define _O_IDU_VLOG_LIST_H_ 1

#include <idrDef.h>


typedef enum iduVLogShmListType
{
    IDU_VLOG_TYPE_SHM_LIST_NONE,
    IDU_VLOG_TYPE_SHM_LIST_ADD_BEFORE,
    IDU_VLOG_TYPE_SHM_LIST_ADD_AFTER,
    IDU_VLOG_TYPE_SHM_LIST_ADDLIST_FIRST,
    IDU_VLOG_TYPE_SHM_LIST_CUT_BETWEEN,
    IDU_VLOG_TYPE_SHM_LIST_REMOVE,
    IDU_VLOG_TYPE_SHM_LIST_MAX
} iduVLogShmListType;

class iduVLogShmList
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC writeAddBefore( iduShmTxInfo   * aShmTxInfo,
                                  iduShmListNode * aTarget,
                                  iduShmListNode * aTgtNodePrev,
                                  iduShmListNode * aNode );

    static IDE_RC undo_SHM_LIST_ADD_BEFORE( idvSQL        * /*aStatistics*/,
                                            iduShmTxInfo  * aShmTxInfo,
                                            idrLSN        /*aNTALsn*/,
                                            UShort          aSize,
                                            SChar         * aImage );

    static IDE_RC writeAddAfter( iduShmTxInfo   * aShmTxInfo,
                                 iduShmListNode * aTarget,
                                 iduShmListNode * aTgtNodeNext,
                                 iduShmListNode * aNode );

    static IDE_RC undo_SHM_LIST_ADD_AFTER( idvSQL         * /*aStatistics*/,
                                           iduShmTxInfo   * aShmTxInfo,
                                           idrLSN         /*aNTALsn*/,
                                           UShort           aSize,
                                           SChar          * aImage );

    static IDE_RC writeCutBetween( iduShmTxInfo   * aShmTxInfo,
                                   iduShmListNode * aPrev,
                                   iduShmListNode * aNext );

    static IDE_RC undo_SHM_LIST_CUT_BETWEEN( idvSQL         * /*aStatistics*/,
                                             iduShmTxInfo   * aShmTxInfo,
                                             idrLSN         /*aNTALsn*/,
                                             UShort           aSize,
                                             SChar          * aImage );

    static IDE_RC writeAddListFirst( iduShmTxInfo   * aShmTxInfo,
                                     iduShmListNode * aBase,
                                     iduShmListNode * aBaseNext,
                                     iduShmListNode * aFirst,
                                     iduShmListNode * aLast );

    static IDE_RC undo_SHM_LIST_ADDLIST_FIRST( idvSQL         * /*aStatistics*/,
                                               iduShmTxInfo   * aShmTxInfo,
                                               idrLSN         /*aNTALsn*/,
                                               UShort           aSize,
                                               SChar          * aImage );

    static IDE_RC writeRemove( iduShmTxInfo   * aShmTxInfo,
                               iduShmListNode * aNode,
                               iduShmListNode * aNodePrev,
                               iduShmListNode * aNodeNext );

    static IDE_RC undo_SHM_LIST_REMOVE( idvSQL         * /*aStatistics*/,
                                        iduShmTxInfo   * aShmTxInfo,
                                        idrLSN         /*aNTALsn*/,
                                        UShort           aSize,
                                        SChar          * aImage );

public:
    static idrUndoFunc mArrUndoFunction[IDU_VLOG_TYPE_SHM_LIST_MAX];
    static SChar       mStrVLogType[IDU_VLOG_TYPE_SHM_LIST_MAX+1][100];
};

#endif /* _O_IDU_VLOG_LIST_H_ */
