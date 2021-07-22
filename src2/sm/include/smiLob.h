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

#ifndef _O_SMI_LOB_CURSOR_H_
#define _O_SMI_LOB_CURSOR_H_ 1

#include <smDef.h>
#include <idl.h>
#include <smiTableCursor.h>

class smiLob
{
public:

    static IDE_RC openLobCursorWithRow( smiTableCursor*   aTableCursor,
                                        void*             aRow,
                                        const smiColumn*  aLobColumn,
                                        UInt              aInfo,
                                        smiLobCursorMode  aMode,
                                        smLobLocator*     aLobLocator );
    
    static IDE_RC openLobCursorWithGRID( smiTableCursor*   aTableCursor,
                                         scGRID            aGRID,
                                         smiColumn*        aLobColumn,
                                         UInt              aInfo,
                                         smiLobCursorMode  aMode,
                                         smLobLocator*     aLobLocator );

    static IDE_RC closeLobCursor( idvSQL*      aStatistics,
                                  smLobLocator aLobLocator );

    /* BUG-40427 [sm_resource] Closing cost of a LOB cursor 
     * which is used internally is too much expensive */
    static IDE_RC closeAllLobCursors( idvSQL       * aStatistics,
                                      smLobLocator   aLobLocator,
                                      UInt           aInfo );

    static idBool isOpen( smLobLocator aLobLoc );
    
    static IDE_RC getLength( idvSQL       * aStatistics,
                             smLobLocator   aLobLocator,
                             SLong        * aLobLen,
                             idBool       * aIsNullLob );
    
    static IDE_RC read( idvSQL*      aStatistics,
                        smLobLocator aLobLoc,
                        UInt         aOffset,
                        UInt         aMount,
                        UChar*       aPiece,
                        UInt*        aReadLength );
    
    static IDE_RC write( idvSQL       * aStatistics,
                         smLobLocator   aLobLoc,
                         UInt           aPieceLen,
                         UChar        * aPiece );

    static IDE_RC erase( idvSQL       * aStatistics,
                         smLobLocator   aLobLocator,
                         ULong          aOffset,
                         ULong          aSize );

    static IDE_RC trim( idvSQL       * aStatistics,
                        smLobLocator   aLobLocator,
                        UInt           aOffset );
    
    static IDE_RC prepare4Write( idvSQL       * aStatistics,
                                 smLobLocator   aLobLoc,
                                 UInt           aOffset,
                                 UInt           aNewSize );
    
    static IDE_RC copy( idvSQL*      aStatistics,
                        smLobLocator aDestLob,
                        smLobLocator aSrcLob );
    
    static IDE_RC finishWrite( idvSQL*      aStatistics,
                               smLobLocator aLobLoc );
    
    static IDE_RC getInfo( smLobLocator aLobLocator,
                           UInt* aInfo);

    static IDE_RC getInfoPtr( smLobLocator aLobLocator,
                              UInt**       aInfo );

    /* PROJ-2728 Sharding LOB */
    static IDE_RC setShardLobModule(
                smLobOpenFunc                   aOpen,
                smLobReadFunc                   aRead,
                smLobWriteFunc                  aWrite,
                smLobEraseFunc                  aErase,
                smLobTrimFunc                   aTrim,
                smLobPrepare4WriteFunc          aPrepare4Write,
                smLobFinishWriteFunc            aFinishWrite,
                smLobGetLobInfoFunc             aGetLobInfo,
                smLobWriteLog4LobCursorOpen     aWriteLog4CursorOpen,
                smLobCloseFunc                  aClose );

    static IDE_RC openShardLobCursor( smiTrans        * aSmiTrans,
                                      UInt              aMmSessId,
                                      UInt              aMmStmtId,
                                      UInt              aRemoteStmtId,
                                      UInt              aNodeId,
                                      SShort            aLobLocatorType,
                                      smLobLocator      aRemoteLobLocator,
                                      UInt              aInfo,
                                      smiLobCursorMode  aMode,
                                      smLobLocator*     aShardLobLocator );

private:
    static IDE_RC getLobCursorAndTrans( smLobLocator      aLobLocator,
                                        smLobCursor    ** aLobCursor,
                                        smxTrans       ** aTrans );

public:
    static const UShort mLobPieceSize;
};
#endif
