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

#ifndef _O_SMC_LOB_H_
#define _O_SMC_LOB_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smc.h>
#include <smp.h>
#include <smm.h>

extern smLobModule smcLobModule;

class smcLob
{
public:
    static IDE_RC open();
    
    static IDE_RC close( idvSQL*        aStatistics,
                         void         * aTrans,
                         smLobViewEnv * aLobViewEnv );
    
    static IDE_RC read(idvSQL*       aStatistics,
                       void*         aTrans,
                       smLobViewEnv* aLobViewEnv,
                       UInt          aOffset,
                       UInt          aMount,
                       UChar*        aPiece,
                       UInt*         aReadLength);
    
    static IDE_RC write(idvSQL       * /* aStatistics */,
                        void         * aTrans,
                        smLobViewEnv * aLobViewEnv,
                        smLobLocator   aLobLocator,
                        UInt           aOffset,
                        UInt           aPieceLen,
                        UChar        * aPiece,
                        idBool         /* aIsFromAPI */,
                        UInt           /* aContType */); // for disk lob repl

    static IDE_RC erase( idvSQL       * aStatistics,
                         void         * aTrans,
                         smLobViewEnv * aLobViewEnv,
                         smLobLocator   aLobLocator,
                         UInt           aOffset,
                         UInt           aPieceLen );

    static IDE_RC trim( idvSQL       * aStatistics,
                        void         * aTrans,
                        smLobViewEnv * aLobViewEnv,
                        smLobLocator   aLobLocator,
                        UInt           aOffset );
    
    static IDE_RC writeInternal(void*            aTrans,
                                smcTableHeader*  aTable,
                                UChar*           aFixedRowPtr,
                                const smiColumn* aLobColumn,
                                UInt             aOffset,
                                UInt             aPieceLen,
                                UChar*           aPiece,
                                idBool           aIsWriteLog,
                                idBool           aIsReplSenderSend,
                                smLobLocator     aLobLocator);

    static IDE_RC prepare4Write(idvSQL*       aStatistics,
                                void*         aTrans,
                                smLobViewEnv* aLobViewEnv,
                                smLobLocator  aLobLocator,
                                UInt          aOffset,
                                UInt          aNewSize);

    static IDE_RC finishWrite( idvSQL       * aStatistics,
                               void         * aTrans,
                               smLobViewEnv * aLobViewEnv,
                               smLobLocator   aLobLocator );

    static IDE_RC reserveSpaceInternalAndLogging(
                                        void*               aTrans,
                                        smcTableHeader*     aTable,
                                        SChar*              aRow,
                                        const smiColumn*    aLobColumn,
                                        UInt                aOffset,
                                        UInt                aNewSize,
                                        UInt                aAddOIDFlag);
    
    static IDE_RC reserveSpaceInternal(void*            aTrans,
                                       smcTableHeader*  aTable,
                                       const smiColumn* aLobColumn,
                                       ULong            aLobVersion,
                                       smcLobDesc*      aLobDesc,
                                       UInt             aOffset,
                                       UInt             aNewSize,
                                       UInt             aAddOIDFlag,
                                       idBool           aIsFullWrite);

    static IDE_RC rebuildLPCH(smOID       aTableOID,
                              smiColumn **aArrLobColumn,
                              UInt        aLobColumnCnt,
                              SChar      *aFixedRow);

    static IDE_RC getLobInfo(idvSQL*        aStatistics,
                             void*          aTrans,
                             smLobViewEnv*  aLobViewEnv,
                             UInt*          aLobLen,
                             UInt*          aLobMode,
                             idBool*        aIsNullLob);

    static void initializeFixedTableArea();

    static smcLobStatistics mMemLobStatistics;
    
private:
    static IDE_RC allocLPCH(void*           aTrans,
                            smcTableHeader* aTable,
                            smcLobDesc*     aLobDesc,
                            smcLobDesc*     aOldLobDesc,
                            scSpaceID       aLobSpaceID,
                            UInt            aNewLPCHCnt,
                            UInt            aAddOIDFlag,
                            idBool          aIsFullWrite);

    static IDE_RC allocPiece(void*           aTrans,
                             smcTableHeader* aTable,
                             ULong           aLobVersion,
                             smcLobDesc*     aLobDesc,
                             smcLobDesc*     aOldLobDesc,
                             scSpaceID       aLobSpaceID,
                             UInt            aOffset,
                             UInt            aNewSize,
                             UInt            aNewLobLen,
                             UInt            aAddOIDFlag,
                             idBool          aIsFullWrite);

    static IDE_RC trimPiece(void*           aTrans,
                            smcTableHeader* aTable,
                            ULong           aLobVersion,
                            smcLobDesc*     aLobDesc,
                            smcLobDesc*     aOldLobDesc,
                            scSpaceID       aLobSpaceID,
                            UInt            aOffset,
                            UInt            aAddOIDFlag);

    static IDE_RC copyPiece( void            * aTrans,
                             smcTableHeader  * aTable,
                             scSpaceID         aLobSpaceID,
                             smcLobDesc      * aSrcLobDesc,
                             UInt              aSrcOffset,
                             smcLobDesc      * aDstLobDesc,
                             UInt              aDstOffset,
                             UInt              aLength);

    static IDE_RC writePiece( scSpaceID aLobPieceSpaceID,
                              smcLPCH* aTargetLPCH,
                              UShort   aOffset,
                              UShort   aMount,
                              UChar*   aBuffer );

    static IDE_RC write4OutMode(void*           aTrans,
                                smcTableHeader* aTable,
                                smcLobDesc*     aLobDesc,
                                scSpaceID       aLobSpaceID,
                                UInt            aOffset,
                                UInt            aPieceLen,
                                UChar*          aPiece,
                                idBool          aIsWriteLog,
                                idBool          aIsReplSenderSend,
                                smLobLocator    aLobLocator);
    
    static UShort getPieceSize( smcLPCH* aLPCH )
    {
        smVCPieceHeader* sPiece = (smVCPieceHeader*)(aLPCH->mPtr);

        return (sPiece == NULL)?0:sPiece->length;
    }

    static UChar* getPiecePtr( smcLPCH* aSourceLPCH,
                               UShort   aOffset )
    {
        return (UChar*)(aSourceLPCH->mPtr)
            + ID_SIZEOF(smVCPieceHeader) + aOffset;
    }

    static void readPiece( smcLPCH* aSourceLPCH,
                           UShort   aOffset,
                           UShort   aMount,
                           UChar*   aBuffer )
    {
        idlOS::memcpy( aBuffer,
                       getPiecePtr(aSourceLPCH, aOffset),
                       aMount );

        return;
    }

    static smcLPCH* findPosition( smcLobDesc* aLobDesc,
                                  UInt        aOffset );

    static IDE_RC getViewRowPtr(void*         aTrans,
                                smLobViewEnv* aLobViewEnv,
                                SChar**       aRowPtr);

    static IDE_RC insertIntoIdx(idvSQL*       aStatistics,
                                void*         aTrans,
                                smLobViewEnv* aLobViewEnv,
                                SChar*        aRowPtr);

    static UInt getLPCHCntFromLength(UInt aLength);

    
    static IDE_RC getLastVersion(idvSQL*       aStatistics,
                                 void*         aTrans,
                                 smLobViewEnv* aLobViewEnv,
                                 SChar**       aNxtFixedRowPtr);

    static IDE_RC reserveSpace(void*         aTrans,
                               smLobViewEnv* aLobViewEnv,
                               SChar*        aNxtFixedRowPtr,
                               UInt          aOffset,
                               UInt          aNewSize);

    static IDE_RC trimSpaceInternal(void*               aTrans,
                                    smcTableHeader*     aTable,
                                    const smiColumn*    aLobColumn,
                                    ULong               aLobVersion,
                                    smcLobDesc*         aLobDesc,
                                    UInt                aOffset,
                                    UInt                aAddOIDFlag);

    static IDE_RC removeOldLPCH(void*            aTrans,
                                smcTableHeader*  aTable,
                                const smiColumn* aLobColumn,
                                smcLobDesc*      aOldLobDesc,
                                smcLobDesc*      aLobDesc,
                                UInt             aBeginIdx,
                                UInt             aEndIdx);
};

/**********************************************************************
 * aLobDesc에서 aOffset에 해당하는 PCH를 찾는다.
 *
 * aLobDesc      [IN] LOB Descriptor
 * aOffset       [IN] 찾고자하는 위치
 **********************************************************************/
inline smcLPCH* smcLob::findPosition( smcLobDesc* aLobDesc,
                                      UInt        aOffset )
{
    smcLPCH* sCurLPCH;
    
    IDE_DASSERT( aLobDesc != NULL );

    IDE_ASSERT( (aOffset / SMP_VC_PIECE_MAX_SIZE) < aLobDesc->mLPCHCount );

    sCurLPCH = aLobDesc->mFirstLPCH + (aOffset / SMP_VC_PIECE_MAX_SIZE);
    
    return sCurLPCH;
}

#endif
