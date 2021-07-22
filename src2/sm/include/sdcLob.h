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

#ifndef _O_SDC_LOB_H_
#define _O_SDC_LOB_H_ 1

#include <idl.h>
#include <smDef.h>
#include <sdcDef.h>
#include <sdr.h>
#include <smrDef.h>
#include <smxDef.h>

class sdcLob
{
    
public:
    
    /* --------------------------------------------------
     *  smLobModule Functions
     * -------------------------------------------------*/
    
    static IDE_RC open();

    static IDE_RC close( idvSQL*        aStatistics,
                         void         * aTrans,
                         smLobViewEnv * aLobViewEnv );

    static IDE_RC read( idvSQL       * aStatistics,
                        void         * aTrans,
                        smLobViewEnv * aLobViewEnv,
                        UInt           aOffset,
                        UInt           aMount,
                        UChar        * aPiece,
                        UInt         * aReadLength );

    static IDE_RC write( idvSQL         * aStatistics,
                         void           * aTrans,
                         smLobViewEnv   * aLobViewEnv,
                         smLobLocator     aLobLocator,
                         UInt             aOffset,
                         UInt             aPieceLen,
                         UChar          * aPieceVal,
                         idBool           aIsFromAPI,
                         UInt             aContType );

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

    static IDE_RC prepare4Write( idvSQL         * aStatistics,
                                 void           * aTrans,
                                 smLobViewEnv   * aLobViewEnv,
                                 smLobLocator     aLobLocator,
                                 UInt             aWriteOffset,
                                 UInt             aWriteSize );

    static IDE_RC finishWrite( idvSQL       * aStatistics,
                               void         * aTrans,
                               smLobViewEnv * aLobViewEnv,
                               smLobLocator   aLobLocator );

    static IDE_RC getLobInfo( idvSQL        * aStatistics,
                              void          * aTrans,
                              smLobViewEnv  * aLobViewEnv,
                              UInt          * aLobLen,
                              UInt          * aLobMode,
                              idBool        * aIsNullLob );

    // for replication
    static IDE_RC writeLog4CursorOpen( idvSQL       * aStatistics,
                                       void         * aTrans,
                                       smLobLocator   aLobLocator,
                                       smLobViewEnv * aLobViewEnv );


    /* --------------------------------------------------
     *  Functions
     * -------------------------------------------------*/
    
    static void initLobDesc( sdcLobDesc * aLobDesc );

    static IDE_RC initLobMetaPage( UChar        * aMetaPtr,
                                   smiColumn    * aColumn,
                                   sdrMtx       * aMtx );

    static IDE_RC removeLob( idvSQL             * aStatistics,     
                             void               * aTrans,
                             sdcLobDesc         * aLobDesc,
                             const smiColumn    * aColumn );

    static UInt getDiskLobDescSize();

    static void initLobViewEnv( smLobViewEnv * aLobViewEnv );

    static IDE_RC copyLobColData( const UInt    /*aColumnSize*/,
                                  const void    * aDestValue,
                                  UInt            aWriteOffset,
                                  UInt            aSrcLength,
                                  const void    * aSrcValue );

    static IDE_RC initLobColBuffer( sdcLobColBuffer * aLobColBuf,
                                    UInt              aLength,
                                    sdcColInOutMode   aInOutMode,
                                    idBool            aIsNullLob );

    static IDE_RC finalLobColBuffer( sdcLobColBuffer * aLobColBuf );

    static ULong getLobLengthFromLobDesc( const sdcLobDesc * aLobDesc );

    static IDE_RC getLobLength( sdcLobColBuffer * aLobColBuf,
                                ULong           * aLength );

    static IDE_RC readLobColBuf( idvSQL        * aStatistics,
                                 void          * aTrans,
                                 smLobViewEnv  * aLobViewEnv );
    
    static IDE_RC updateLobCol( idvSQL          * aStatistics,
                                void            * aTrans,
                                smLobViewEnv    * aLobViewEnv );

    static IDE_RC adjustLobVersion( smLobViewEnv * aLobViewEnv );

    static UChar* getLobDataLayerStartPtr( UChar * aPage );


    static IDE_RC appendLeafNodeRollback( idvSQL    * aStatistics,
                                          sdrMtx    * aMtx,
                                          scSpaceID   aSpaceID,
                                          scPageID    aRootNodePID,
                                          scPageID    aLeafNodePID );

    static IDE_RC getWritePageOffsetAndSize( UInt     aOffset,
                                             UInt     aPieceLen,
                                             UInt   * aWriteOffset,
                                             UInt   * aWriteSize );

    static IDE_RC getReadPageOffsetAndSize( UChar  * aPage,
                                            UInt     aOffset,
                                            UInt     aAmount,
                                            UInt   * aReadOffset,
                                            UInt   * aReadSize );


    /* --------------------------------------------------
     *  CLR Functions
     * -------------------------------------------------*/

    static IDE_RC writeLobInsertLeafKeyCLR( sdrMtx * aMtx,
                                            UChar  * aLeafKey,
                                            SShort   aLeafKeySeq );

    static IDE_RC writeLobUpdateLeafKeyCLR( sdrMtx     * aMtx,
                                            UChar      * aKeyPtr,
                                            SShort       aLeafKeySeq,
                                            sdcLobLKey * aLKey );

    static IDE_RC writeLobOverwriteLeafKeyCLR( sdrMtx     * aMtx,
                                               UChar      * aKeyPtr,
                                               SShort       aLeafKeySeq,
                                               sdcLobLKey * aOldLKey,
                                               sdcLobLKey * aNewLKey );

    /* --------------------------------------------------
     *  Stack Functions
     * -------------------------------------------------*/

    static void initStack( sdcLobStack * aStack );

    static IDE_RC pushStack( sdcLobStack * aStack,
                             scPageID      aPageID );

    static IDE_RC popStack( sdcLobStack * aStack,
                            scPageID    * aPageID );

    static SInt getPosStack( sdcLobStack * aStack );

    
    /* --------------------------------------------------
     *  Fixed Table Define for X$DISK_LOB_STATISTICS
     * -------------------------------------------------*/
    
    static void initializeFixedTableArea();

    
    /* --------------------------------------------------
     *  Dump Functions
     * -------------------------------------------------*/

    static IDE_RC dumpLobDataPageHdr( UChar * aPage,
                                      SChar * aOutBuf,
                                      UInt    aOutSize );

    static IDE_RC dumpLobDataPageBody( UChar    * aPage,
                                       SChar    * aOutBuf,
                                       UInt       aOutSize );

    static IDE_RC dumpLobMeta( UChar    * aPage,
                               SChar    * aOutBuf,
                               UInt       aOutSize );

private:

    /* --------------------------------------------------
     *  Internal Functions
     * -------------------------------------------------*/

    static IDE_RC readInternal( idvSQL       * aStatistics,
                                smLobViewEnv * aLobViewEnv,
                                UInt           aOffset,
                                UInt           aAmount,
                                UChar        * aPiece,
                                UInt         * aReadLength );

    static IDE_RC readBuffer( smLobViewEnv      * aLobViewEnv,
                              sdcLobColBuffer   * aLobColBuf,
                              UInt              * aOffset,
                              UInt                aAmount,
                              UChar             * aPiece,
                              UInt              * aReadLength );

    static IDE_RC readDirect( idvSQL        * aStatistics,
                              smLobViewEnv  * aLobViewEnv,
                              sdcLobDesc    * aLobDesc,
                              UInt          * aOffset,
                              UInt          * aAmount,
                              UChar        ** aPiece,
                              UInt          * aReadLength );

    static IDE_RC readIndex( idvSQL        * aStatistics,
                             smLobViewEnv  * aLobViewEnv,
                             sdcLobDesc    * aLobDesc,
                             UInt          * aOffset,
                             UInt          * aAmount,
                             UChar        ** aPiece,
                             UInt          * aReadLength );

    static IDE_RC getValidVersionLKey( idvSQL      * aStatistics,
                                       ULong         aLobVersion,
                                       sdcLobLKey  * aLKey,
                                       sdcLobLKey  * aValidLKey,
                                       idBool      * aIsFound );

    static IDE_RC makeOldVersionLKey( idvSQL       * aStatistics,
                                      sdSID          aUndoSID,
                                      sdcLobLKey   * aOldLKey );

    static IDE_RC readPage( idvSQL      * aStatistics,
                            smiColumn   * aColumn,
                            scPageID      aPID,
                            UInt        * aOffset,
                            UInt        * aAmount,
                            UChar      ** aPiece,
                            UInt        * aReadLength );

    static void incLastPageInfo( sdcLobColBuffer   * aLobColBuf,
                                 UInt                aOffset,
                                 UInt                aPieceLen );

    static void decLastPageInfo( sdcLobColBuffer   * aLobColBuf,
                                 UInt                aOffset );

    static sdcLobChangeType getLobChangeType( sdcLobColBuffer * aLobColBuf,
                                              smiColumn       * aColumn,
                                              UInt              aOffset, 
                                              UInt              aPieceLen );

    static IDE_RC writeInternal( idvSQL            * aStatistics,
                                 void              * aTrans,
                                 smLobLocator        aLobLocator,
                                 smLobViewEnv      * aLobViewEnv,
                                 UInt                aPieceLen,
                                 UChar             * aPieceVal,
                                 sdcLobWriteType     aWriteType,
                                 idBool              aIsFromAPI,
                                 smrContType         aContType );

    static IDE_RC writeInToIn( smLobViewEnv    * aLobViewEnv,
                               UChar           * aPieceVal,
                               UInt              aPieceLen,
                               sdcLobWriteType   aWriteType );

    static IDE_RC writeInToOut( idvSQL          * aStatistics,
                                void            * aTrans,
                                smLobViewEnv    * aLobViewEnv,
                                smLobLocator      aLobLocator,
                                UChar           * aPieceVal,
                                UInt              aPieceLen,
                                sdcLobWriteType   aWriteType,
                                idBool            aIsFromAPI,
                                smrContType       aContType );

    static IDE_RC writeOutToOut( idvSQL         * aStatistics,
                                 void           * aTrans,
                                 smLobViewEnv   * aLobViewEnv,
                                 smLobLocator     aLobLocator,
                                 UChar          * aPieceVal,
                                 UInt             aPieceLen,
                                 sdcLobWriteType  aWriteType,
                                 idBool           aIsFromAPI,
                                 smrContType      aContType );

    static IDE_RC writeIndex( idvSQL           * aStatistics,
                              void             * aTrans,
                              smLobViewEnv     * aLobViewEnv,
                              smLobLocator       aLobLocator,
                              sdcLobColBuffer  * aLobColBuf,
                              UInt             * aOffset,
                              UChar           ** aPieceVal,
                              UInt             * aPieceLen,
                              UInt               aAmount,
                              sdcLobWriteType    aWriteType,
                              idBool             aIsPrevVal,
                              idBool             aIsFromAPI,
                              smrContType        aContType );

    static IDE_RC appendLeafNode( idvSQL       * aStatistics,
                                  void         * aTrans,
                                  smiColumn    * aColumn,
                                  ULong          aLobVersion,
                                  scPageID     * aRootNodePID );


    static IDE_RC makeNewRootNode( idvSQL      * aStatistics,
                                   sdrMtx      * aMtx,
                                   smiColumn   * aColumn,
                                   ULong         aLobVersion,
                                   UChar       * aNode,
                                   UChar       * aNewNode,
                                   scPageID    * aRootNodePID );

    static IDE_RC insertInternalKey( sdrMtx    * aMtx,
                                     UChar     * aNode,
                                     scPageID    aPageID );

    static IDE_RC propagateKey( idvSQL         * aStatistics,
                                sdrMtx         * aMtx,
                                smiColumn      * aColumn,
                                ULong            aLobVersion,
                                sdcLobStack    * aStack,
                                UChar          * sNode,
                                UChar          * sNewNode,
                                scPageID       * aRootNodePID );

    static IDE_RC findRightMostLeafNode( idvSQL        * aStatistics,
                                         scSpaceID       aLobColSpaceID,
                                         scPageID        aRootNodePID,
                                         sdcLobStack   * aStack );

    static IDE_RC updateKey( idvSQL            * aStatistics,
                             void              * aTrans,
                             void              * aTable,
                             smiColumn         * aColumn,
                             smLobLocator        aLobLocator,
                             ULong               aLobVersion,
                             scPageID            aLeafNodePID,
                             SShort              aLeafKeySeq,
                             UInt              * aOffset,
                             UChar            ** aPieceVal,
                             UInt              * aPieceLen,
                             UInt                aAmount,
                             sdcLobWriteType     aWriteType,
                             idBool              aIsPrevVal,
                             idBool              aIsFromAPI,
                             smrContType         aContType );

    static IDE_RC makeUpdateLKey( idvSQL         * aStatistics,
                                  void           * aTrans,
                                  void           * aTable,
                                  smiColumn      * aColumn,
                                  smLobLocator     aLobLocator,
                                  ULong            aLobVersion,
                                  UInt           * aOffset,
                                  UChar         ** aPieceVal,
                                  UInt           * aPieceLen,
                                  UInt             aAmount,
                                  scPageID         aLeafNodePID,
                                  SShort           aLeafKeySeq,
                                  sdcLobWriteType  aWriteType,
                                  sdcLobLKey     * aLKey,
                                  idBool           aIsPrevVal,
                                  idBool           aIsFromAPI,
                                  smrContType      aContType );

    static IDE_RC writeEntry( idvSQL        * aStatistics,
                              void          * aTrans,
                              void          * aTable,
                              smiColumn     * aColumn,
                              smLobLocator    aLobLocator,
                              ULong           aLobVersion,
                              UInt          * aOffset,
                              UChar        ** aPieceVal,
                              UInt          * aPieceLen,
                              UInt            aAmount,
                              UInt            aEntrySeq,
                              sdcLobLKey    * aLKey,
                              idBool          aIsPrevVal,
                              idBool          aIsFromAPI,
                              smrContType     aContType );

    static IDE_RC eraseEntry( idvSQL        * aStatistics,
                              void          * aTrans,
                              smiColumn     * aColumn,
                              ULong           aLobVersion,
                              UInt          * aOffset,
                              UInt          * aPieceLen,
                              UInt            aEntrySeq,
                              sdcLobLKey    * aLKey );

    static IDE_RC trimEntry( idvSQL        * aStatistics,
                             void          * aTrans,
                             smiColumn     * aColumn,
                             ULong           aLobVersion,
                             UInt          * aOffset,
                             UInt          * aPieceLen,
                             UInt            aEntrySeq,
                             sdcLobLKey    * aLKey );

    static IDE_RC writeUpdateLKey( idvSQL      * aStatistics,
                                   void        * aTrans,
                                   void        * aTable,
                                   smiColumn   * aColumn, 
                                   scPageID      aLeafNode,
                                   SShort        aLeafKeySeq,
                                   sdcLobLKey  * aLKey );

    static IDE_RC insertKey( idvSQL         * aStatistics,
                             void           * aTrans,
                             void           * aTable,
                             smiColumn      * aColumn,
                             smLobLocator     aLobLocator,
                             ULong            aLobVersion,
                             scPageID         aLeafNodePID,
                             SShort           aLeafKeySeq,
                             UInt           * aOffset,
                             UChar         ** aPieceVal,
                             UInt           * aPieceLen,
                             UInt             aAmount,
                             idBool           aIsPrevVal,
                             idBool           aIsFromAPI,
                             smrContType      aContType );

    static IDE_RC writeInsertLKey( idvSQL       * aStatistics,
                                   void         * aTrans,
                                   smiColumn    * aColumn,
                                   scPageID       aLeafNodePID,
                                   SShort         aLeafKeySeq,
                                   sdcLobLKey   * aKey );

    static IDE_RC makeInsertLKey( idvSQL         * aStatistics,
                                  void           * aTrans,
                                  void           * aTable,
                                  smiColumn      * aColumn,
                                  smLobLocator     aLobLocator,
                                  ULong            aLobVersion,
                                  UInt           * aOffset,
                                  UChar         ** aPieceVal,
                                  UInt           * aPieceLen,
                                  UInt             aAmount,
                                  sdcLobLKey     * aKey,
                                  idBool           aIsPrevVal,
                                  idBool           aIsFromAPI,
                                  smrContType      aContType );

    static UInt getPageSeq( UInt aOffset );

    static SShort getEntrySeq( UInt aOffset );

    static UShort getMaxLeafKeyCount();

    static UShort getMaxInternalKeyCount();

    static IDE_RC findInternalKey( sdcLobNodeHdr * aNodeHdr,
                                   UInt            aPageSeq,
                                   sdcLobIKey    * aIKey );

    static IDE_RC findLeafKey( sdcLobNodeHdr * aNodeHdr,
                               UInt            aPageSeq,
                               SShort        * aKeySeq,
                               SShort        * aEntrySeq,
                               UShort        * aKeyCount );

    static IDE_RC traverse( idvSQL     * aStatistics,
                            smiColumn  * aColumn,
                            scPageID     aRootNodePID,
                            UInt         aPageSeq,
                            scPageID   * aLeafNodePID,
                            SShort     * aLeafKeySeq,
                            SShort     * aLeafKeyEntrySeq,
                            UShort     * aLeafKeyCount );

    static IDE_RC traverseSequential( idvSQL     * aStatistics,
                                      smiColumn  * aColumn,
                                      scPageID     aNxtLeafNodePID,
                                      UInt         aPageSeq,
                                      scPageID   * aLeafNodePID,
                                      SShort     * aLeafKeySeq,
                                      SShort     * aLeafKeyEntrySeq,
                                      UShort     * aLeafKeyCount );
    
    static IDE_RC writeDirect( idvSQL           * aStatistics,
                               void             * aTrans,
                               smLobViewEnv     * aLobViewEnv,
                               smLobLocator       aLobLocator,
                               sdcLobColBuffer  * aLobColBuf,
                               UInt             * aOffset,
                               UChar           ** aPieceVal,
                               UInt             * aPieceLen,
                               UInt               aAmount,
                               sdcLobWriteType    aWriteType,
                               idBool             aIsPrevVal,
                               idBool             aIsFromAPI,
                               smrContType        aContType );
    
    static IDE_RC erasePage( idvSQL      * aStatistics,
                             void        * aTrans,
                             smiColumn   * aColumn,
                             ULong         aLobVersion,
                             UInt        * aOffset,
                             UInt        * aPieceLen,
                             scPageID      aPageID,
                             scPageID    * aNewPageID );

    static IDE_RC trimPage( idvSQL      * aStatistics,
                            void        * aTrans,
                            smiColumn   * aColumn,
                            ULong         aLobVersion,
                            UInt        * aOffset,
                            UInt        * aPieceLen,
                            scPageID      aPageID,
                            scPageID    * aNewPageID );

    static idBool isFullPageWrite( sdcLobNodeHdr    * aNodeHdr,
                                   UInt             * aOffset,
                                   UInt             * aPieceLen );

    static IDE_RC writePage( idvSQL       * aStatistics,
                             void         * aTrans,
                             void         * aTable,
                             smiColumn    * aColumn,
                             smLobLocator   aLobLocator,
                             ULong          aLobVersion,
                             UInt         * aOffset,
                             UChar       ** aPieceVal,
                             UInt         * aPieceLen,
                             UInt           aAmount,
                             scPageID       aPageID,
                             scPageID     * aNewPageID,
                             idBool         aIsPrevVal,
                             idBool         aIsFromAPI,
                             smrContType    aContType );

    static IDE_RC removeOldPages( idvSQL       * aStatistics,
                                  void         * aTrans,
                                  smiColumn    * aColumn,
                                  sdcLobDesc   * aOldLobDesc,
                                  sdcLobDesc   * aNewLobDesc );

    static IDE_RC removeOldPages( idvSQL       * aStatistics,
                                  void         * aTrans,
                                  smiColumn    * aColumn,
                                  sdcLobLKey   * aOldLKey,
                                  sdcLobLKey   * aNewLKey );
    
    static IDE_RC addPage2AgingList( idvSQL      * aStatistics,
                                     void        * aTrans,
                                     smiColumn   * aColumn,
                                     scPageID      aOldPageID );

    static IDE_RC copyPage( sdrMtx * aMtx,
                            UChar  * sDstPage,
                            UChar  * sSrcPage );

    static IDE_RC copyAndTrimPage( sdrMtx * aMtx,
                                   UChar  * aDstPage,
                                   UChar  * aSrcPage,
                                   UInt     aOffset );

    static IDE_RC eraseLobPiece( sdrMtx    * aMtx,
                                 smiColumn * aColumn,
                                 UChar     * aPage,
                                 UInt      * aOffset,
                                 UInt      * aPieceLen );

    static IDE_RC writeLobPiece( idvSQL       * aStatistics,
                                 void         * aTrans,
                                 void         * aTable,
                                 smiColumn    * aColumn,
                                 smLobLocator   aLobLocator,
                                 scPageID       aPageID,
                                 UInt         * aOffset,
                                 UChar       ** aPieceVal,
                                 UInt         * aPieceLen,
                                 UInt           aAmout,
                                 idBool         aIsPrevVal,
                                 idBool         aIsFromAPI,
                                 smrContType    aContType );

    static IDE_RC writeLobWritePieceRedoLog( sdrMtx        * aMtx,
                                             void          * aTable,
                                             UChar         * aLobDataLayerStartPtr,
                                             idBool          aIsPrevVal,
                                             UInt            aOffset,
                                             UInt            aAmount,
                                             UChar         * aWriteValue,
                                             UInt            aWriteSize,
                                             UInt            aWriteOffset,
                                             smLobLocator    aLobLocator );
 
    static IDE_RC writeLobWritePiece4DMLRedoLog( sdrMtx     * aMtx,
                                                 void       * aTable,
                                                 smiColumn  * aColumn,
                                                 UChar      * aLobDataLayerStartPtr,
                                                 UInt         aAmount,
                                                 UChar      * aWriteValue,
                                                 UInt         aWriteSize,
                                                 UInt         aWriteOffset );   

    static IDE_RC allocNewPage( idvSQL          * aStatistics,
                                sdrMtx          * aMtx,
                                smiColumn       * aColumn,
                                sdpPageType       aPageType,
                                UChar          ** aPagePtr );

    static IDE_RC initializeNodeHdr( sdrMtx        * aMtx,
                                     sdpPhyPageHdr * aPage,
                                     UShort          aHeight,
                                     UInt            aNodeSeq,
                                     ULong           aLobVersion );

    static IDE_RC agingPages( idvSQL    * aStatistics,
                              void      * aTrans,
                              smiColumn * aColumn,
                              UInt        aNeedPageCnt );

    static IDE_RC agingNode( idvSQL     * aStatistics,
                             sdrMtx     * aMtx,
                             smiColumn  * aColumn, 
                             sdcLobMeta * sMeta,
                             scPageID     aFreePageID,
                             UInt       * aFreeNodeCnt );

    static IDE_RC agingDataNode( idvSQL     * aStatistics,
                                 sdrMtx     * aMtx,
                                 smiColumn  * aColumn,
                                 sdcLobMeta * aMeta,
                                 scPageID     aFreePageID,
                                 UInt       * aFreeNodeCnt );

    static IDE_RC agingIndexNode( idvSQL     * aStatistics,
                                  sdrMtx     * aMtx,
                                  smiColumn  * aColumn,
                                  sdcLobMeta * aMeta,
                                  scPageID     aFreePageID,
                                  UInt       * aFreeNodeCnt );

    static IDE_RC deleteLeafKey( idvSQL        * aStatistics,
                                 sdrMtx        * aMtx,
                                 smiColumn     * aColumn,
                                 sdcLobMeta    * aMeta, 
                                 sdcLobStack   * aStack,
                                 scPageID        aRootNodePID,
                                 UInt          * aFreeNodeCnt );

    static IDE_RC deleteInternalKey( idvSQL        * aStatistics,
                                     sdrMtx        * aMtx,
                                     smiColumn     * aColumn,
                                     sdcLobMeta    * aMeta,
                                     sdcLobStack   * aStack,
                                     scPageID        aRootNodePID,
                                     scPageID        aChildNodePID,
                                     UInt          * aFreeNodeCnt );

    static IDE_RC getNodeInfo( idvSQL      * aStatistics,
                               smiColumn   * aColumn,
                               scPageID      aPageID,
                               idBool      * aIsFreeable,
                               UShort      * aPageType );

    static IDE_RC getCommitSCN( idvSQL * aStatistics,
                                sdSID    aTSSlotSID,
                                smSCN  * aFstDskViewSCN,
                                smSCN  * aCommitSCN );

    static IDE_RC writeBuffer( smLobViewEnv    * aLobViewEnv,
                               UChar           * aPieceVal,
                               UInt              aPieceLen,
                               sdcLobWriteType   aWriteType );

    static idBool isInternalNodeFull( UChar * sNode );

    static UInt getNeedPageCount( sdcLobColBuffer  * aLobColBuf,
                                  smiColumn        * aColumn,
                                  UInt               aOffset,
                                  UInt               aPieceLen );

public:
    static const sdcLobDesc mEmptyLobDesc;
    
};

#endif
