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
#if !defined(_O_IDR_VLOG_UPDATE_H_)
#define _O_IDR_VLOG_UPDATE_H_ 1

#include <idrDef.h>

typedef enum idrIDLogType
{
    IDR_VLOG_TYPE_NONE,
    IDR_VLOG_TYPE_PHYSICAL,
    IDR_VLOG_TYPE_DUMMY_NTA,
    IDR_VLOG_TYPE_MAX
} idrIDLogType;

class idrVLogUpdate
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC undo_VLOG_TYPE_PHYSICAL( idvSQL       * /* aStatistics */,
                                           iduShmTxInfo * aShmTxInfo,
                                           idrLSN         /* aNTALsn */,
                                           UShort         aSize,
                                           SChar        * aImage );

    static IDE_RC undo_VLOG_TYPE_DUMMY_NTA( idvSQL       * /* aStatistics */,
                                            iduShmTxInfo * aShmTxInfo,
                                            idrLSN         aNTALsn,
                                            UShort         aSize,
                                            SChar        * aImage );

public:
    static idrUndoFunc mArrUndoFunction[IDR_VLOG_TYPE_MAX];
    static SChar       mStrVLogType[IDR_VLOG_TYPE_MAX+1][100];
};

#endif /* _O_IDR_VLOG_UPDATE_H_ */
