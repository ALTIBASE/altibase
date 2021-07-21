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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * PROJ-1362 Large Record & Internal LOB support
 **********************************************************************/

#ifndef _O_SMC_LOB_UPDATE_H_
#define _O_SMC_LOB_UPDATE_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smcDef.h>

class smcLobUpdate
{
public:

    // replication related log.
    static IDE_RC writeLog4CursorOpen(idvSQL*        aStatistics,
                                      void*          aTrans,
                                      smLobLocator   aLobLocator,
                                      smLobViewEnv*  aLobViewEnv);

    // lob write log.
    static IDE_RC writeLog4LobWrite(void*           aTrans,
                                    smcTableHeader* aTable,
                                    smLobLocator    aLobLocator,
                                    scSpaceID       aLobSpaceID,
                                    scPageID        aPageID,
                                    UShort          aPageOffset,
                                    UShort          aLobOffset,
                                    UShort          aPieceLen,
                                    UChar*          aPiece,
                                    idBool          aIsReplSenderSend);

    // redo, undo (SMR_SMC_PERS_WRITE_LOB_PIECE)
    static IDE_RC redo_SMC_PERS_WRITE_LOB_PIECE(smTID      aTID,
                                                scSpaceID  aSpaceID,
                                                scPageID   aPID,
                                                scOffset   aOffset,
                                                vULong     aData,
                                                SChar     *aImage,
                                                SInt       aSize,
                                                UInt       aFlag);
};

#endif
