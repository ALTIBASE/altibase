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
 * $Id: qmcCursor.h 88285 2020-08-05 05:49:37Z bethy $
 **********************************************************************/

#ifndef _O_QMC_CURSOR_H_
#define _O_QMC_CURSOR_H_ 1

#include <mt.h>
#include <qmcMemory.h>

typedef struct qmcOpenedCursor
{
    UShort            tableID;
    smiTableCursor  * cursor;
    qmcOpenedCursor * next;
} qmcOpenedCursor;

class qmcCursor
{
private:
    // table cursor
    qmcMemory       * mMemory;

    qmcOpenedCursor * mTop;
    qmcOpenedCursor * mCurrent;

    // BUG-40427 lob cursor 1개만 보관한다
    smLobLocator      mOpenedLob;

public:
    IDE_RC init( iduMemory * aMemory );

    IDE_RC addOpenedCursor( iduMemory      * aMemory,
                            UShort           aTableID,
                            smiTableCursor * aCursor );
    IDE_RC getOpenedCursor( UShort aTableID, smiTableCursor ** aCursor, idBool * aFound );

    // BUG-40427 최초로 Open한 LOB Cursor인 경우, qmcCursor에 기록
    IDE_RC addOpenedLobCursor( smLobLocator aLocator );

    IDE_RC closeAllCursor( idvSQL * aStatistics );
};

#endif /* _O_QMC_CURSOR_H_ */
