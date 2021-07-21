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
#if !defined(_O_IDW_VLOG_PMMMGR_H_)
#define _O_IDW_VLOG_PMMMGR_H_ 1

#include <idrDef.h>

typedef enum iduVLogPMMgrType
{
    IDW_VLOG_TYPE_PM_MGR_NONE,
    IDW_VLOG_TYPE_PM_MGR_UPDATE_THREAD_COUNT,
    IDW_VLOG_TYPE_PM_MGR_MAX
} idwVLogPMMgrType;

class idwVLogPMMgr
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC writeThreadCount4Proc( iduShmTxInfo * aShmTxInfo,
                                         idShmAddr      aAddr4Proc,
                                         UInt           aThrCnt );

    static IDE_RC undo_PMMGR_UPDATE_THREAD_COUNT( idvSQL       * /*aStatistics*/,
                                                  iduShmTxInfo * /*aShmTxInfo*/,
                                                  idrLSN        /*aNTALsn*/,
                                                  UShort         aSize,
                                                  SChar        * aImage );

public:
    static idrUndoFunc mArrUndoFunction[IDW_VLOG_TYPE_PM_MGR_MAX];
};

#endif /* _O_IDW_VLOG_SHMMGR_H_ */
