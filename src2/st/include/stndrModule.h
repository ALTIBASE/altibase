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
 * $Id: stndrModule.h 89495 2020-12-14 05:19:22Z emlee $ 
 **********************************************************************/

#ifndef _O_STNDR_MODULE_H_
#define _O_STNDR_MODULE_H_  1

#include <idu.h>
#include <stndrDef.h>

extern smnIndexModule stndrModule;

class stndrRTree
{
    
public:

    /* ------------------------------------------------
     * callback function for TTL
     * ----------------------------------------------*/
    
    static IDE_RC softKeyStamping( sdrMtx        * aMtx,
                                   sdpPhyPageHdr * aNode,
                                   UChar           aCTSlotNum,
                                   UChar         * aContext );
    
    static IDE_RC hardKeyStamping( idvSQL        * aStatistics,
                                   sdrMtx        * aMtx,
                                   sdpPhyPageHdr * aNode,
                                   UChar           aCTSlotNum,
                                   UChar         * aContext,
                                   idBool        * aSuccess );

    /* ------------------------------------------------
     * smnIndexModule
     * ----------------------------------------------*/
    
    static IDE_RC prepareIteratorMem( smnIndexModule* aModule );

    static IDE_RC releaseIteratorMem( const smnIndexModule* aModule );
    
    static IDE_RC create( idvSQL            * aStatistics,
                          smcTableHeader    * aTable,
                          smnIndexHeader    * aIndex,
                          smiSegAttr        * aSegAttr,
                          smiSegStorageAttr * aSegStoAttr,
                          smnInsertFunc     * aInsert,
                          smnIndexHeader   ** aRebuildIndexHeader,
                          ULong               aSmoNo );

    static IDE_RC drop( smnIndexHeader  * aIndex );

    static IDE_RC lockRow( stndrIterator* aIterator );

    static IDE_RC deleteKey( idvSQL      * aStatistics,
                             void        * aTrans,
                             void        * aIndex,
                             SChar       * aKeyValue,
                             smiIterator * aIterator,
                             sdSID         aRowSID );

    static IDE_RC insertKeyRollback( idvSQL * aStatistics,
                                     void   * aMtx,
                                     void   * aIndex,
                                     SChar  * aKeyValue,
                                     sdSID    aRowSID,
                                     UChar  * aRollbackContext,
                                     idBool   aIsDupKey );
    
    static IDE_RC deleteKeyRollback( idvSQL * aStatistics,
                                     void   * aMtx,
                                     void   * aIndex,
                                     SChar  * aKeyValue,
                                     sdSID    aRowSID,
                                     UChar  * aRollbackContext );

    static IDE_RC aging( idvSQL         * aStatistics,
                         void           * aTrans,
                         smcTableHeader * aHeader,
                         smnIndexHeader * aIndex );

    static IDE_RC init( stndrIterator        * aIterator,
                        void                 * aTrans,
                        smcTableHeader       * aTable,
                        smnIndexHeader       * aIndex,
                        void                 * aDumpObject,
                        const smiRange       * aKeyRange,
                        const smiRange       * aKeyFilter,
                        const smiCallBack    * aRowFilter,
                        UInt                   aFlag,
                        smSCN                  aSCN,
                        smSCN                  aInfinite,
                        idBool                 aUntouchable,
                        smiCursorProperties  * aProperties,
                        const smSeekFunc    ** aSeekFunc,
                        smiStatement         * aStatement );

    static IDE_RC dest( stndrIterator * /*aIterator*/ );

    static IDE_RC freeAllNodeList( idvSQL         * aStatistics,
                                   smnIndexHeader * aIndex,
                                   void           * aTrans );

    static IDE_RC getPositionNA( stndrIterator    * aIterator,
                                 smiCursorPosInfo * aPosInfo );
    
    static IDE_RC setPositionNA( stndrIterator    * aIterator,
                                 smiCursorPosInfo * aPosInfo );


    static IDE_RC allocIterator( void ** aIteratorMem );
    
    static IDE_RC freeIterator( void * aIteratorMem );
    
    static IDE_RC buildIndex( idvSQL            * aStatistics,
                              void              * aTrans,
                              smcTableHeader    * aTable,
                              smnIndexHeader    * aIndex,
                              smnGetPageFunc      aGetPageFunc,
                              smnGetRowFunc       aGetRowFunc,
                              SChar             * aNullRow,
                              idBool              aIsNeedValidation,
                              idBool              aIsEnableTable,
                              UInt                aParallelDegree,
                              UInt                aBuildFlag,
                              UInt                aTotalRecCnt );


    static void getSmoNo( void  * aIndex,
                          ULong * aSmoNo );

    static IDE_RC increaseSmoNo( idvSQL         * aStatistics,
                                 stndrHeader    * aIndex,
                                 ULong          * aSmoNo );

    static void getVirtualRootNode( stndrHeader          * aIndex,
                                    stndrVirtualRootNode * aVRootNode );

    static void setVirtualRootNode( stndrHeader  * aIndex,
                                    scPageID       aRootNode,
                                    ULong          aSmoNo );
    
    static IDE_RC makeKeyValueFromRow(
                                idvSQL                  * aStatistics,
                                sdrMtx                  * aMtx,
                                sdrSavePoint            * aSP,
                                void                    * aTrans,
                                void                    * aTableHeader,
                                const smnIndexHeader    * aIndex,
                                const UChar             * aRow,
                                sdbPageReadMode           aPageReadMode,
                                scSpaceID                 aTableSpaceID,
                                smFetchVersion            aFetchVersion,
                                sdRID                     aTssRID,
                                const smSCN             * aSCN,
                                const smSCN             * aInfiniteSCN,
                                UChar                   * aDestBuf,
                                idBool                  * aIsRowDeleted,
                                idBool                  * aIsPageLatchReleased );

    static IDE_RC makeSmiValueListInFetch(
                                const smiColumn             * aIndexColumn,
                                UInt                          aCopyOffset,
                                const smiValue              * aColumnValue,
                                void                        * aIndexInfo );

    static IDE_RC makeKeyValueFromSmiValueList(
                                const smnIndexHeader    * aIndex,
                                const smiValue          * aSmiValueList,
                                UChar                   * aDestBuf );


    static IDE_RC rebuildIndexColumn( smnIndexHeader    * aIndex,
                                      smcTableHeader    * aTable,
                                      void              * aHeader );

    
    /* ------------------------------------------------
     * Seek Fucntion
     * ----------------------------------------------*/
    
    static IDE_RC NA( void ); 

    static IDE_RC beforeFirst( stndrIterator     * aIterator,
                               const smSeekFunc ** aSeekFunc );


    static IDE_RC beforeFirstW( stndrIterator       * aIterator,
                                const smSeekFunc   ** aSeekFunc );

    static IDE_RC beforeFirstRR( stndrIterator      *  aIterator,
                                 const smSeekFunc  ** aSeekFunc );

    static IDE_RC fetchNext( stndrIterator  *  a_pIterator,
                             const void    ** aRow );

    
    /* ------------------------------------------------
     * R-Tree Implementation Function
     * ----------------------------------------------*/
    
    static IDE_RC allocPage( idvSQL         * aStatistics,
                             stndrStatistic * aIndexStat,
                             stndrHeader    * aIndex,
                             sdrMtx         * aMtx,
                             UChar         ** aNewPage );
    
    static IDE_RC getPage( idvSQL           * aStatistics,
                           stndrPageStat    * aPageStat,
                           scSpaceID          aSpaceID,
                           scPageID           aPageID,
                           sdbLatchMode       aLatchMode,
                           sdbWaitMode        aWaitMode,
                           void             * aMtx,
                           UChar           ** aRetPagePtr,
                           idBool           * aTrySuccess );
    
    static IDE_RC freePage( idvSQL          * aStatistics,
                            stndrStatistic  * aIndexStat,
                            stndrHeader     * aIndex,
                            sdrMtx          * aMtx,
                            UChar           * aFreePage );
    
    static IDE_RC insertKey( idvSQL * aStatistics,
                             void   * aTrans,
                             void   * aTable,
                             void   * aIndex,
                             smSCN    aInfiniteSCN,
                             SChar  * aKeyValue,
                             SChar  * /* aNullRow */,
                             idBool   aUniqueCheck,
                             smSCN    aStmtViewSCN,
                             void   * aRowSID,
                             SChar ** aExistUniqueRow,
                             ULong    aInsertWaitTime,
                             idBool   aForbiddenToRetry );

    static IDE_RC softKeyStamping( stndrHeader      * aIndex,
                                   sdrMtx           * aMtx,
                                   sdpPhyPageHdr    * aNode,
                                   UChar              aCTSlotNum );
    
    static IDE_RC hardKeyStamping( idvSQL           * aStatistics,
                                   stndrHeader      * aIndex,
                                   sdrMtx           * aMtx,
                                   sdpPhyPageHdr    * aNode,
                                   UChar              aCTSlotNum,
                                   idBool           * aSuccess );
    
    static IDE_RC makeFetchColumnList4Index(
                                void        * aTableHeader,
                                stndrHeader * aIndexHeader );

    static IDE_RC setConsistent( smnIndexHeader * aIndex,
                                 idBool      aIsConsistent );
    
    static IDE_RC buildDRTopDown( idvSQL            * aStatistics,
                                  void              * aTrans,
                                  smcTableHeader    * aTable,
                                  smnIndexHeader    * aIndex,
                                  idBool              aIsNeedValidation,
                                  UInt                aBuildFlag,
                                  UInt                aParallelDegree );

    static IDE_RC buildDRBottomUp( idvSQL           * aStatistics,
                                   void             * aTrans,
                                   smcTableHeader   * aTable,
                                   smnIndexHeader   * aIndex,
                                   idBool             aIsNeedValidation,
                                   UInt               aBuildFlag,
                                   UInt               aParallelDegree );

    static IDE_RC initMeta( UChar * aMetaPtr,
                            UInt    aBuildFlag,
                            void  * aMtx );
    
    static IDE_RC buildMeta( idvSQL * aStatistics,
                             void   * aTrans,
                             void   * aIndex );

    static IDE_RC setIndexMetaInfo( idvSQL          * aStatistics,
                                    stndrHeader     * aIndex,
                                    stndrStatistic  * aIndexStat,
                                    sdrMtx          * aMtx,
                                    scPageID        * aRootPID,
                                    scPageID        * aFreeNodeHead,
                                    ULong           * aFreeNodeCnt,
                                    idBool          * aIsCreatedWithLogging,
                                    idBool          * aIsConsistent,
                                    idBool          * aIsCreatedWithForce,
                                    smLSN           * aNologgingCompletionLSN,
                                    UShort          * aConvexhullPointNum );
    
    static IDE_RC backupRuntimeHeader( sdrMtx      * aMtx,
                                       stndrHeader * aIndex );
    static IDE_RC restoreRuntimeHeader( void      * aIndex );

    static IDE_RC preparePages( idvSQL      * aStatistics,
                                stndrHeader * aIndex,
                                sdrMtx      * aMtx,
                                idBool      * aMtxStart,
                                UInt          aNeedPageCnt );

    static IDE_RC fixPage( idvSQL           * aStatistics,
                           stndrPageStat    * aPageStat,
                           scSpaceID          aSpaceID,
                           scPageID           aPageID,
                           UChar           ** aRetPagePtr,
                           idBool           * aTrySuccess );
    
    static IDE_RC unfixPage( idvSQL * aStatistics,
                             UChar  * aPagePtr );

    static IDE_RC setFreeNodeInfo( idvSQL           * aStatistics,
                                   stndrHeader      * aIndex,
                                   stndrStatistic   * aIndexStat,
                                   sdrMtx           * aMtx,
                                   scPageID           aFreeNodeHead,
                                   ULong              aFreeNodeCnt,
                                   smSCN            * aFreeNodeSCN );


    static IDE_RC allocCTS( idvSQL              * aStatistics,
                            stndrHeader         * aIndex,
                            sdrMtx              * aMtx,
                            sdpPhyPageHdr       * aNode,
                            UChar               * aCTSlotNum,
                            sdnCallbackFuncs    * aCallbackFunc,
                            UChar               * aContext,
                            SShort              * aKeySeq );

    static IDE_RC adjustKeyPosition( sdpPhyPageHdr    * aNode,
                                     SShort           * aKeyPosition );

    static IDE_RC compactPage( sdrMtx           * aMtx,
                               sdpPhyPageHdr    * aPage,
                               idBool             aIsLogging );

    static IDE_RC compactPageInternal( sdpPhyPageHdr * aPage );

    static IDE_RC compactPageLeaf( sdpPhyPageHdr * aPage );

    static IDE_RC initializeNodeHdr( sdrMtx         * aMtx,
                                     sdpPhyPageHdr  * aNode,
                                     UShort           aHeight,
                                     idBool           aIsLogging );
    
    static IDE_RC makeNewRootNode( idvSQL           * aStatistics,
                                   stndrStatistic   * aIndexStat,
                                   sdrMtx           * aMtx,
                                   idBool           * aMtxStart,
                                   stndrHeader      * aIndex,
                                   smSCN            * aInfiniteSCN,
                                   stndrKeyInfo     * aLeftKeyInfo,
                                   sdpPhyPageHdr    * aLeftNode,
                                   stndrKeyInfo     * aRightKeyInfo,
                                   UShort             aKeyValueLen,
                                   sdpPhyPageHdr   ** aNewChildPage,
                                   UInt             * aAllocPageCount );
    
    static UShort getKeyValueLength();

    static UShort getKeyLength( UChar  * aKey,
                                idBool   aIsLeaf );

    static IDE_RC insertKeyIntoLeafNode( sdrMtx                * aMtx,
                                         stndrHeader           * aIndex,
                                         smSCN                 * aInfiniteSCN,
                                         sdpPhyPageHdr         * aLeafNode,
                                         SShort                * aLeafKeySeq,
                                         stndrKeyInfo          * aKeyInfo,
                                         UChar                   aCTSlotNum,
                                         idBool                * aIsSuccess );

    static IDE_RC insertLKey( sdrMtx        * aMtx,
                              stndrHeader   * aIndex,
                              sdpPhyPageHdr * aNode,
                              SShort          aKeySeq,
                              UChar           aCTSlotNum,
                              smSCN         * aInfiniteSCN,
                              UChar           aTxBoundType,
                              stndrKeyInfo  * aKeyInfo,
                              UShort          aKeyValueLen,
                              idBool          aIsLoggableSlot,
                              UShort        * aKeyOffset );
    
    static IDE_RC canInsertKey( idvSQL               * aStatistics,
                                sdrMtx               * aMtx,
                                stndrHeader          * aIndex,
                                stndrStatistic       * aIndexStat,
                                stndrKeyInfo         * aKeyInfo,
                                sdpPhyPageHdr        * aLeafNode,
                                SShort               * aLeafKeySeq,
                                UChar                * aCTSlotNum,
                                stndrCallbackContext * sContext );
        
    static IDE_RC chooseLeafNode( idvSQL            * aStatistics,
                                  stndrStatistic    * aIndexStat,
                                  sdrMtx            * aMtx,
                                  stndrPathStack    * aStack,
                                  stndrHeader       * aIndexHeader,
                                  stndrKeyInfo      * aKeyInfo,
                                  sdpPhyPageHdr    ** aLeafNode,
                                  SShort            * aLeafKeySeq );
    

    static IDE_RC canAllocLeafKey( sdrMtx           * aMtx,
                                   stndrHeader      * aIndex,
                                   sdpPhyPageHdr    * aNode,
                                   UInt               aSaveSize,
                                   SShort           * aKeySeq );
    
    static IDE_RC canAllocInternalKey( sdrMtx           * aMtx,
                                       stndrHeader      * aIndex,
                                       sdpPhyPageHdr    * aNode,
                                       UInt               aSaveSize,
                                       idBool             aExecCompact,
                                       idBool             aIsLogging );

    static IDE_RC selfAging( stndrHeader    * aIndex,
                             sdrMtx         * aMtx,
                             sdpPhyPageHdr  * aNode,
                             smSCN          * aOldestSCN,
                             UChar          * aAgedCount );

    
    static IDE_RC insertLeafKeyWithTBT( sdrMtx                  * aMtx,
                                        stndrHeader             * aIndex,
                                        UChar                     aCTSlotNum,
                                        smSCN                   * aInfiniteSCN,
                                        sdpPhyPageHdr           * aLeafNode,
                                        stndrKeyInfo            * aKeyInfo,
                                        UShort                    aKeyValueLen,
                                        SShort                    aKeySeq );

    
    static IDE_RC insertLeafKeyWithTBK( sdrMtx          * aMtx,
                                        stndrHeader     * aIndex,
                                        smSCN           * aInfiniteSCN,
                                        sdpPhyPageHdr   * aLeafNode,
                                        stndrKeyInfo    * aKeyInfo,
                                        UShort            aKeyValueLen,
                                        SShort            aKeySeq );
    
    static IDE_RC findBestInternalKey( stndrKeyInfo   * aKeyInfo,
                                       stndrHeader    * aIndex,
                                       sdpPhyPageHdr  * aNode,
                                       SShort         * aKeySeq,
                                       scPageID       * aChildPID,
                                       SDouble        * aDelta,
                                       ULong            aIndexSmoNo,
                                       idBool         * aIsRetry );

    static void findBestLeafKey( sdpPhyPageHdr  * aNode,
                                 SShort         * aKeySeq,
                                 ULong            aIndexSmoNo,
                                 idBool         * aIsRetry );

    static IDE_RC getLeafNodeDelta( stndrHeader     * aIndex,
                                    sdpPhyPageHdr   * aNode,
                                    stndrKeyInfo    * aKeyInfo,
                                    SDouble         * aDelta,
                                    idBool          * aIsSuccess );

    static IDE_RC adjustNodeMBR( stndrHeader   * aIndex,
                                 sdpPhyPageHdr * aNode,
                                 stdMBR        * aInsertMBR,
                                 SShort          aUpdateKeySeq,
                                 stdMBR        * aUpdateMBR,
                                 SShort          aDeleteKeySeq,
                                 stdMBR        * aNodeMBR );

    static IDE_RC adjustINodeMBR( stndrHeader   * aIndex,
                                  sdpPhyPageHdr * aNode,
                                  stdMBR        * aInsertMBR,
                                  SShort          aUpdateKeySeq,
                                  stdMBR        * aUpdateMBR,
                                  SShort          aDeleteKeySeq,
                                  stdMBR        * aNodeMBR );

    static IDE_RC adjustLNodeMBR( stndrHeader   * aIndex,
                                  sdpPhyPageHdr * aNode,
                                  stdMBR        * aInsertMBR,
                                  SShort          aUpdateKeySeq,
                                  stdMBR        * aUpdateMBR,
                                  SShort          aDeleteKeySeq,
                                  stdMBR        * aNodeMBR );

    static IDE_RC getParentNode( idvSQL         * aStatistics,
                                 stndrStatistic * aIndexStat,
                                 sdrMtx         * aMtx,
                                 stndrHeader    * aIndex,
                                 stndrPathStack * aStack,
                                 scPageID         aChildPID,
                                 scPageID       * aParentPID,
                                 sdpPhyPageHdr ** aParentNode,
                                 SShort         * aParentKeySeq,
                                 stdMBR         * aParentKeyMBR,
                                 idBool         * aIsRetry );

    static UInt getMinimumKeyValueLength( smnIndexHeader * aIndexHeader );

    static IDE_RC propagateKeyValue( idvSQL         * aStatistics,
                                     stndrStatistic * aIndexStat,
                                     stndrHeader    * aIndex,
                                     sdrMtx         * aMtx,
                                     stndrPathStack * aStack,
                                     sdpPhyPageHdr  * aChildNode,
                                     stdMBR         * aChildNodeMBR,
                                     idBool         * aIsRetry );

    static void findEmptyNodes( sdrMtx          * aMtx,
                                stndrHeader     * aIndex,
                                sdpPhyPageHdr   * aStartPage,
                                scPageID        * aEmptyNodePID,
                                UInt            * aEmptyNodeCount );
    
    static IDE_RC nodeAging( idvSQL         * aStatistics,
                             void           * aTrans,
                             stndrStatistic * aIndexStat,
                             stndrHeader    * aIndex,
                             UInt             aFreePageCount );
    
    static IDE_RC linkEmptyNodes( idvSQL            * aStatistics,
                                  void              * aTrans,
                                  stndrHeader       * aIndex,
                                  stndrStatistic    * aIndexStat,
                                  scPageID          * aEmptyNodePID );
    
    static IDE_RC linkEmptyNode( idvSQL         * aStatistics,
                                 stndrStatistic * aIndexStat,
                                 stndrHeader    * aIndex,
                                 sdrMtx         * aMtx,
                                 sdpPhyPageHdr  * aNode,
                                 idBool         * aIsSuccess );

    static IDE_RC agingAllTBK( idvSQL           * aStatistics,
                               sdrMtx           * aMtx,
                               sdpPhyPageHdr    * aNode,
                               idBool           * aIsSuccess );

    static idBool isAgableTBK( smSCN    aCommitSCN );

    static IDE_RC freeNode( idvSQL          * aStatistics,
                            void            * aTrans,
                            stndrHeader     * aIndex,
                            scPageID          aFreeNodeID,
                            stndrKeyInfo    * aKeyInfo,
                            stndrStatistic  * aIndexStat,
                            UInt            * aFreePageCount );

    static IDE_RC traverse( idvSQL          * aStatistics,
                            stndrHeader     * aIndex,
                            sdrMtx          * aMtx,
                            smSCN           * aCursorSCN,
                            smSCN           * aFstDiskViewSCN,
                            stndrKeyInfo    * aKeyInfo,
                            scPageID          aFreeNodePID,
                            UInt              aTraverseMode,
                            stndrPathStack  * aStack,
                            stndrStatistic  * aIndexStat,
                            sdpPhyPageHdr  ** aLeafNode,
                            SShort          * aLeafKeySeq );
    
    static IDE_RC findLeafKey( stndrHeader      * aIndex,
                               sdrMtx           * aMtx,
                               smSCN            * aCursorSCN,
                               smSCN            * aFstDiskViewSCN,
                               UInt               aTraverseMode,
                               sdpPhyPageHdr    * aNode,
                               stndrStatistic   * aIndexStat,
                               stndrKeyInfo     * aKeyInfo,
                               SShort           * aKeySeq,
                               idBool           * aIsSuccess );

    static IDE_RC tranLevelVisibility( idvSQL           * aStatistics,
                                       void             * aTrans,
                                       UChar            * aNode,
                                       UChar            * aLeafKey,
                                       smSCN            * aStmtViewSCN,
                                       idBool           * aIsVisible,
                                       idBool           * aIsUnknown );

    static IDE_RC cursorLevelVisibility( stndrLKey  * aLeafKey,
                                         smSCN      * aInfiniteSCN,
                                         idBool     * aIsVisible );

    static IDE_RC deleteInternalKey( idvSQL         * aStatistics,
                                     stndrHeader    * aIndex,
                                     stndrStatistic * aIndexStat,
                                     scPageID         aSegPID,
                                     scPageID         aChildPID,
                                     sdrMtx         * aMtx,
                                     stndrPathStack * aStack,
                                     UInt           * aFreePageCount,
                                     idBool         * aIsRetry );
    
    static IDE_RC unsetRootNode( idvSQL         * aStatistics,
                                 sdrMtx         * aMtx,
                                 stndrHeader    * aIndex,
                                 stndrStatistic * aIndexStat );
    

    static IDE_RC unlinkEmptyNode( idvSQL           * aStatistics,
                                   stndrStatistic   * aIndexStat,
                                   stndrHeader      * aIndex,
                                   sdrMtx           * aMtx,
                                   sdpPhyPageHdr    * aNode,
                                   UChar              aNodeState );

    static IDE_RC setIndexEmptyNodeInfo( idvSQL         * aStatistics,
                                         stndrHeader    * aIndex,
                                         stndrStatistic * aIndexStat,
                                         sdrMtx         * aMtx,
                                         scPageID       * aEmptyNodeHead,
                                         scPageID       * aEmptyNodeTail );

  
    static IDE_RC splitLeafNode( idvSQL         * aStatistics,
                                 stndrStatistic * aIndexStat,
                                 sdrMtx         * aMtx,
                                 idBool         * aMtxStart,
                                 stndrHeader    * aIndex,
                                 smSCN          * aInfiniteSCN,
                                 stndrPathStack * aStack,
                                 sdpPhyPageHdr  * aNode,
                                 stndrKeyInfo   * aKeyInfo,
                                 UShort           aKeyValueLen,
                                 SShort           aKeySeq,
                                 idBool           aIsInsert,
                                 UInt             aSplitMode,
                                 sdpPhyPageHdr ** aTargetNode,
                                 SShort         * aTargetKeySeq,
                                 UInt           * aAllocPageCount,
                                 idBool         * aIsRetry );

    static IDE_RC propagateKeyInternalNode( idvSQL          * aStatistics,
                                            stndrStatistic  * aIndexStat,
                                            sdrMtx          * aMtx,
                                            idBool          * aMtxStart,
                                            stndrHeader     * aIndex,
                                            smSCN           * aInfiniteSCN,
                                            stndrPathStack  * aStack,
                                            UShort            aKeyValue,
                                            stndrKeyInfo    * aUpdateKeyInfo,
                                            sdpPhyPageHdr   * aUpdateChildNode,
                                            stndrKeyInfo    * aInsertKeyInfo,
                                            UInt              aSplitMode,
                                            sdpPhyPageHdr  ** aNewChildNode,
                                            UInt            * aAllocPageCount,
                                            idBool          * aIsRetry );

    static IDE_RC splitInternalNode( idvSQL         * aStatistics,
                                     stndrStatistic * aIndexStat,
                                     sdrMtx         * aMtx,
                                     idBool         * aMtxStart,
                                     stndrHeader    * aIndex,
                                     smSCN          * aInfiniteSCN,
                                     stndrPathStack * aStack,
                                     UShort           aKeyValueLen,
                                     stndrKeyInfo   * aUpdateKeyInfo,
                                     SShort           aUpdateKeySeq,
                                     stndrKeyInfo   * aInsertKeyInfo,
                                     UInt             aSplitMode,
                                     sdpPhyPageHdr  * aNode,
                                     sdpPhyPageHdr ** aNewChildNode,
                                     UInt           * aAllocPageCount,
                                     idBool         * aIsRetry );

    static IDE_RC makeKeyArray( stndrKeyInfo    * aUpdateKeyInfo,
                                SShort            aUpdateKeySeq,
                                stndrKeyInfo    * aInsertKeyInfo,
                                SShort            aInsertKeySeq,
                                sdpPhyPageHdr   * aNode,
                                stndrKeyArray  ** aKeyArray,
                                UShort          * aKeyArrayCnt );
    
    static void adjustKeySeq( stndrKeyArray * aKeyArray,
                              UShort          aKeyArrayCnt,
                              UShort          aSplitPoint,
                              SShort        * aInsertKeySeq,
                              SShort        * aUpdateKeySeq,
                              SShort        * aDeleteKeySeq,
                              idBool        * aInsertKeyOnNewPage,
                              idBool        * aUpdateKeyOnNewPage,
                              idBool        * aDeleteKeyOnNewPage );

    static void makeSplitGroup( stndrHeader     * aIndex,
                                UInt              aSplitMode,
                                stndrKeyArray   * aKeyArray,
                                UShort            aKeyArrayCnt,
                                UShort          * aSplitPoint,
                                stdMBR          * aOldGroupMBR,
                                stdMBR          * aNewGroupMBR );

    static void splitByRStarWay( stndrHeader    * aIndex,
                                 stndrKeyArray  * aKeyArray,
                                 UShort           aKeyArrayCnt,
                                 UShort         * aSplitPoint );
    
    static void splitByRTreeWay( stndrHeader    * aIndex,
                                 stndrKeyArray  * aKeyArray,
                                 UShort           aKeyArrayCnt,
                                 UShort         * aSplitPoint );
    
    static void pickSeed( stndrHeader   * aIndex,
                          stndrKeyArray * aArray,
                          UShort          aArrayCount,
                          stndrKeyArray * aKeySeed0,
                          stndrKeyArray * aKeySeed1,
                          SShort        * aPos );

    static void pickNext( stndrHeader   * aIndex,
                          stndrKeyArray * aArray,
                          stdMBR        * aMBRSeed0,
                          stdMBR        * aMBRSeed1,
                          SShort        * aPos,
                          stndrKeyArray * aKey,
                          UShort        * aSeed );
    
    static IDE_RC moveSlots( sdrMtx         * aMtx,
                             stndrHeader    * aIndex,
                             sdpPhyPageHdr  * aSrcNode,
                             stndrKeyArray  * sKeyArray,
                             UShort           sFromIdx,
                             UShort           sToIdx,
                             sdpPhyPageHdr  * aDstNode );

    static void getSplitInfo( stndrHeader   * aHeaer,
                              stndrKeyArray * aKeyArray,
                              SShort          aKeyArrayCnt,
                              UShort        * aSplitPoint,
                              SDouble       * aSumPerimeter );

    static void getArrayPerimeter( stndrHeader      * aIndex,
                                   stndrKeyArray    * aKeyArray,
                                   UShort             aStartPos,
                                   UShort             aEndPos,
                                   SDouble          * aPerimeter,
                                   stdMBR           * aMBR );

    static IDE_RC insertIKey( sdrMtx        * aMtx,
                              stndrHeader   * aIndex,
                              sdpPhyPageHdr * aNode,
                              SShort          aKeySeq,
                              stndrKeyInfo  * aKeyInfo,
                              UShort          aKeyValueLen,
                              scPageID        aRightChildPID,
                              idBool          aIsNeedLogging );

    static IDE_RC updateIKey( sdrMtx        * aMtx,
                              sdpPhyPageHdr * aNode,
                              SShort          aKeySeq,
                              stdMBR        * aKeyValue,
                              idBool          aLogging );

    static IDE_RC getCommitSCN( idvSQL          * aStatistics,
                                void            * aTrans,
                                sdpPhyPageHdr   * aNode,
                                stndrLKeyEx     * aLeafKeyEx,
                                idBool            aIsLimit,
                                smSCN             aStmtViewSCN,
                                smSCN           * aCommitSCN );

    static idBool isMyTransaction( void     * aTrans,
                                   smSCN      aBeginSCN,
                                   sdSID      aTSSlotSID );
    
    static idBool isMyTransaction( void   * aTrans,
                                   smSCN    aBeginSCN,
                                   sdSID    aTSSlotSID,
                                   smSCN  * aSCN );
    
    static IDE_RC writeLogFreeKeys( sdrMtx          * aMtx,
                                    UChar           * aNode,
                                    stndrKeyArray   * aKeyArray,
                                    UShort            aFromSeq,
                                    UShort            aToSeq );
    
    static void getNodeDelta( stndrHeader   * aIndex,
                              stndrKeyInfo  * aKeyInfo,
                              sdpPhyPageHdr * aNode,
                              SDouble       * aDelta );

    static IDE_RC findValidStackDepth( idvSQL           * aStatistics,
                                       stndrStatistic   * aIndexStat,
                                       stndrHeader      * aIndex,
                                       stndrPathStack   * aStack,
                                       ULong            * aSmoNo );

    static IDE_RC deleteKeyFromLeafNode( idvSQL         * aStatistics,
                                         stndrStatistic * aIndexStat,
                                         sdrMtx         * aMtx,
                                         stndrHeader    * aIndex,
                                         smSCN          * aInfiniteSCN,
                                         sdpPhyPageHdr  * aLeafNode,
                                         SShort         * aLeafKeySeq ,
                                         idBool         * aIsSuccess );

    static IDE_RC deleteLeafKeyWithTBT( sdrMtx          * aMtx,
                                        stndrHeader     * aIndex,
                                        smSCN           * aInfiniteSCN,
                                        UChar             aCTSlotNum,
                                        sdpPhyPageHdr   * aLeafNode,
                                        SShort            aLeafKeySeq );

    static IDE_RC deleteLeafKeyWithTBK( sdrMtx          * aMtx,
                                        stndrHeader     * aIndex,
                                        smSCN           * aInfiniteSCN,
                                        sdpPhyPageHdr   * aLeafNode,
                                        SShort          * aLeafKeySeq ,
                                        idBool          * aIsSuccess );
    
    static IDE_RC beforeFirstInternal( stndrIterator * aIterator );
    
    static IDE_RC findFirst( stndrIterator * aIterator );
    
    static IDE_RC findNextLeaf( idvSQL              * aStatistics,
                                stndrStatistic      * aIndexStat,
                                stndrHeader         * aIndex,
                                const smiCallBack   * aCallBack,
                                stndrStack          * aStack,
                                sdpPhyPageHdr      ** aLeafNode,
                                idBool              * aIsRetry );

    static IDE_RC makeRowCache( stndrIterator   * aIterator,
                                UChar           * aNode );

    static IDE_RC fetchRowCache( stndrIterator  * aIterator,
                                 const void    ** aRow,
                                 idBool         * aNeedMoreCache );

    static IDE_RC makeVRowFromRow( stndrHeader          * aIndex,
                                   idvSQL               * aStatistics,
                                   sdrMtx               * aMtx,
                                   sdrSavePoint         * aSP,
                                   void                 * aTrans,
                                   void                 * aTable,
                                   scSpaceID              aTableSpaceID,
                                   const UChar          * aRow,
                                   sdbPageReadMode        aPageReadMode,
                                   smiFetchColumnList   * aFetchColumnList,
                                   smFetchVersion         aFetchVersion,
                                   sdSID                  aTSSlotSID,
                                   const smSCN          * aSCN,
                                   const smSCN          * aInfiniteSCN,
                                   UChar                * aDestBuf,
                                   idBool               * aIsRowDeleted,
                                   idBool               * aIsPageLatchReleased );

    static IDE_RC checkFetchColumnList( stndrHeader         * aIndex,
                                        smiFetchColumnList  * aFetchColumnList );
    
    static IDE_RC makeNextRowCache( stndrIterator   * aIterator,
                                    stndrHeader     * aIndex );

    static IDE_RC lockAllRows4RR( stndrIterator * aIterator );

    static void isIndexableRow( void    * aIndex,
                                SChar   * aRow,
                                idBool  * aIsIndexable );

    static IDE_RC columnValue2String ( UChar * aColumnPtr,
                                       UChar * aText,
                                       UInt  * aTextLen );

    static UShort getColumnLength( UChar        * aColumnPtr,
                                   UInt         * aColumnHeaderLen,
                                   UInt         * aColumnValueLen,
                                   const void  ** aColumnValuePtr );

    static IDE_RC freeKeys( sdrMtx          * aMtx,
                            sdpPhyPageHdr   * aPage,
                            stndrKeyArray   * aKeyArray,
                            UShort            aFromIdx,
                            UShort            aToIdx );

    static IDE_RC freeKeysInternal( sdpPhyPageHdr * aPage,
                                    stndrKeyArray * aKeyArray,
                                    UShort          aFromIdx,
                                    UShort          aToIdx );

    // BUG-29538 split시 TBK count를 조정하지 않고 있습니다.
    static IDE_RC freeKeysLeaf( sdpPhyPageHdr * aPage,
                                stndrKeyArray * aKeyArray,
                                UShort          aFromIdx,
                                UShort          aToIdx,
                                UShort        * aUnlimitedKeyCount,
                                UShort        * aTBKCount );

    static IDE_RC setNodeMBR( sdrMtx         * aMtx,
                              sdpPhyPageHdr  * aNode,
                              stdMBR         * aMBR );
    
    static UShort getNonFragFreeSize( stndrHeader   * aIndex,
                                      sdpPhyPageHdr * aNode );
    
    static UShort getTotalFreeSize( stndrHeader   * aIndex,
                                    sdpPhyPageHdr * aNode );

    
    static void swap( stndrKeyArray * aArray,
                      SInt            i,
                      SInt            j );


    static void quickSort( stndrKeyArray * aArray,
                           SInt            aArraySize,
                           SInt         (* compare)( const void* aLhs,
                                                     const void* aRhs ) );

    static IDE_RC dumpMeta( UChar * aPage,
                            SChar * aOutBuf,
                            UInt    aOutSize );

    static IDE_RC dumpNodeHdr( UChar * aPage,
                               SChar * aOutBuf,
                               UInt    aOutSize );

    static IDE_RC dump( UChar * aPage,
                        SChar * aOutBuf,
                        UInt    aOutSize );

    static void dumpIndexNode( sdpPhyPageHdr *aNode );

private:
    
    static iduMemPool   mIteratorPool;
};

inline void stndrRTree::dumpIndexNode( sdpPhyPageHdr *aNode )
{
    if( aNode == NULL )
    {
        ideLog::log( IDE_SERVER_0, "\
===================================================\n\
            PID : NULL\n");
    }
    else
    {
        ideLog::log( IDE_SERVER_0, "\
===================================================\n\
            PID : %u              \n",
                    aNode->mPageID );

        ideLog::logMem( IDE_SERVER_0, (UChar *)aNode, SD_PAGE_SIZE );
    }
}

#endif /* _O_STNDR_MODULE_H_ */
