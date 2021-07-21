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
 * $Id: smnbModule.h 89495 2020-12-14 05:19:22Z emlee $
 **********************************************************************/

#ifndef _O_SMNB_MODULE_H_
# define _O_SMNB_MODULE_H_ 1

# include <smnDef.h>
# include <smDef.h>
# include <smnbDef.h>
# include <idu.h>

extern smnIndexModule smnbModule;

class smnbBTree
{
public:

    /*************************************************/
    /* PUBLIC FUNCTIONS                              */
    /*************************************************/

    static IDE_RC prepareIteratorMem( const smnIndexModule* aModule );

    static IDE_RC releaseIteratorMem( const smnIndexModule* aModule );

    static IDE_RC prepareFreeNodeMem( const smnIndexModule* aModule );

    static IDE_RC releaseFreeNodeMem( const smnIndexModule* aModule );

    static IDE_RC freeAllNodeList( idvSQL         * aStatistics,
                                   smnIndexHeader * aIndex,
                                   void           * aTrans );
    
    static IDE_RC freeNode( smnbNode* aNodes );
    
    static IDE_RC create( idvSQL             * aStatistics,
                          smcTableHeader     * aTable,
                          smnIndexHeader     * aIndex,
                          smiSegAttr         * /*aSegAttr*/,
                          smiSegStorageAttr  * /*aSegStoAttr*/,
                          smnInsertFunc      * aInsert,
                          smnIndexHeader    ** aRebuildIndexHeader,
                          ULong                aSmoNo );

    static IDE_RC buildIndex( idvSQL*               aStatistics,
                              void*                 aTrans,
                              smcTableHeader*       aTable,
                              smnIndexHeader*       aIndex,
                              smnGetPageFunc        aGetPageFunc,
                              smnGetRowFunc         aGetRowFunc,
                              SChar*                aNullPtr,
                              idBool                aIsNeedValidation,
                              idBool                aIsPersistent,
                              UInt                  aParallelDegree,
                              UInt                  aBuildFlag,
                              UInt                  aTotalRecCnt );

    static IDE_RC drop( smnIndexHeader * aIndex );

    static IDE_RC init( smnbIterator        * aIterator,
                        void                * aTrans,
                        smcTableHeader      * aTable,
                        smnIndexHeader      * aIndex,
                        void                * aDumpObject,
                        const smiRange      * aKeyRange,
                        const smiRange      * aKeyFilter,
                        const smiCallBack   * aRowFilter,
                        UInt                  aFlag,
                        smSCN                 aSCN,
                        smSCN                 aInfinite,
                        idBool                aUntouchable,
                        smiCursorProperties * aProperties,
                        const smSeekFunc   ** aSeekFunc,
                        smiStatement        * aStatement );

    static IDE_RC dest( smnbIterator * /*aIterator*/ );

    static IDE_RC isRowUnique( void          * aTrans,
                               smnbStatistic * aIndexStat,
                               smSCN           aStmtViewSCN,
                               void          * aRow,
                               smTID         * aTid,
                               SChar        ** aExistUniqueRow,
                               idBool          aForbiddenToRetry );

    static SInt compareRows( smnbStatistic      * aIndexStat,
                             const smnbColumn   * aColumns,
                             const smnbColumn   * aFence,
                             const void         * aRow1,
                             const void         * aRow2 );
    
    static SInt compareRowsAndPtr2( smnbStatistic      * aIndexStat,
                                    const smnbColumn   * aColumns,
                                    const smnbColumn   * aFence,
                                    SChar              * aRow1,
                                    SChar              * aRow2,
                                    idBool             * aIsEqual );

    static SInt compareKeys( smnbStatistic      * aIndexStat,
                             const smnbColumn   * aColumns,
                             const smnbColumn   * aFence,
                             void               * aKey1,
                             SChar              * aRow1,
                             UInt                 aPartialKeySize,
                             SChar              * aRow2,
                             idBool             * aIsSuccess );

    static idBool isNullColumn( smnbColumn * aColumn,
                                smiColumn  * aSmiColumn,
                                SChar      * aRow );
    
    /* BUG-30074 disk table의 unique index에서 NULL key 삭제 시/
    *           non-unique index에서 deleted key 추가 시
    *           Cardinality의 정확성이 많이 떨어집니다.
    * Key 전체가 Null인지 확인한다. 모두 Null이어야 한다. */
    static idBool isNullKey( smnbHeader * aIndex,
                             SChar      * aRow );

    static IDE_RC insertRowUnique( idvSQL           * aStatistics,
                                   void             * aTrans,
                                   void             * aTable,
                                   void             * aIndex,
                                   smSCN              aInfiniteSCN,
                                   SChar            * aRow,
                                   SChar            * aNull,
                                   idBool             aUniqueCheck,
                                   smSCN              aStmtViewSCN,
                                   void             * aRowSID,
                                   SChar           ** aExistUniqueRow,
                                   ULong              aInsertWaitTime,
                                   idBool             aForbiddenToRetry );

    static IDE_RC insertRow( idvSQL          * aStatistics,
                             void            * aTrans,
                             void            * aTable,
                             void            * aIndex,
                             smSCN             aInfiniteSCN,
                             SChar           * aRow,
                             SChar           * aNull,
                             idBool            aUniqueCheck,
                             smSCN             aStmtViewSCN,
                             void            * aRowSID,
                             SChar          ** aExistUniqueRow,
                             ULong             aInsertWaitTime,
                             idBool            aForbiddenToRetry );

    static void merge(smnbINode* a_pLeftNode,
                      smnbINode* a_pRightNode);
    
    static IDE_RC deleteNA( idvSQL*           aStatistics,
                            void*             aTrans,
                            void*             aIndex,
                            SChar*            aRow,
                            smiIterator *     aIterator,
                            sdRID             aRowRID);

    static IDE_RC freeSlot( void    * aIndex,
                            SChar   * aRow,
                            idBool    aIgnoreNotFoundKey,
                            idBool  * aIsExistFreeKey );

    static IDE_RC freeSlotByLeafFullScan( smnbHeader   * aIndexHeader,
                                          SChar        * aRow );

    /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure of
     * index aging. */
    static IDE_RC checkExistKey( void   * aIndex,
                                 SChar  * aRow );

    /* TASK-4990 changing the method of collecting index statistics
     * 수동 통계정보 수집 기능 */
    static IDE_RC gatherStat( idvSQL         * aStatistics,
                              void           * aTrans,
                              SFloat           aPercentage,
                              SInt             aDegree,
                              smcTableHeader * aHeader,
                              void           * aTotalTableArg,
                              smnIndexHeader * aIndex,
                              void           * aStats,
                              idBool           aDynamicMode );

    static IDE_RC NA( void );

    static IDE_RC beforeFirst( smnbIterator*       aIterator,
                               const smSeekFunc**  aSeekFunc );
    //BUG-48230: beforeFirst for Queue
    static IDE_RC beforeFirstInternalQ( smnbIterator* a_pIterator );
    static IDE_RC beforeFirstQ( void*               aIterator);

    static IDE_RC afterLast( smnbIterator*       aIterator,
                             const smSeekFunc**  aSeekFunc );
    
    static IDE_RC beforeFirstU( smnbIterator*       aIterator,
                                const smSeekFunc**  aSeekFunc );
    
    //BUG-48230: afterLast for Queue
    static IDE_RC afterLastQ( void*               aIterator);

    static IDE_RC afterLastU( smnbIterator*       aIterator,
                              const smSeekFunc**  aSeekFunc );
    
    static IDE_RC beforeFirstR( smnbIterator*       aIterator,
                                const smSeekFunc**  aSeekFunc );

    static IDE_RC afterLastR( smnbIterator*       aIterator,
                              const smSeekFunc**  aSeekFunc );
    
    static IDE_RC fetchNext( smnbIterator  * a_pIterator,
                             const void   ** aRow );

    //BUG-48230: fetchNext for Queue
    static IDE_RC fetchNextQ( smnbIterator  * a_pIterator,
                              const void   ** aRow );

    static IDE_RC fetchPrev( smnbIterator* aIterator,
                             const void**  aRow );

    static IDE_RC fetchNextU( smnbIterator* aIterator,
                              const void**  aRow );
    
    static IDE_RC fetchPrevU( smnbIterator* aIterator,
                              const void**  aRow );
    

    static IDE_RC retraverse( idvSQL*       aStatistics,
                              smnbIterator* aIterator,
                              const void**  /*aRow*/ );

    static inline IDE_RC initLeafNode( smnbLNode  * a_pNode,
                                       smnbHeader * aIndexHeader,
                                       IDU_LATCH    a_latch );
    static inline void initInternalNode( smnbINode *a_pNode,
                                         smnbHeader * aIndexHeader,
                                         IDU_LATCH a_latch );

    static inline void lockTree( smnbHeader * a_pIndex );
    static inline void unlockTree( smnbHeader * a_pIndex );

    /* For Make Disk Image When server is stopped normally */
    static IDE_RC makeDiskImage(smnIndexHeader* a_pIndex,
                                smnIndexFile*   a_pIndexFile);
    static IDE_RC makeNodeListFromDiskImage(smcTableHeader *a_pTable,
                                            smnIndexHeader *a_pIndex);

    static IDE_RC getPosition( smnbIterator *     aIterator,
                               smiCursorPosInfo * aPosInfo );
    
    static IDE_RC setPosition( smnbIterator     * aIterator,
                               smiCursorPosInfo * aPosInfo );

    static IDE_RC allocIterator( void ** aIteratorMem );
    
    static IDE_RC freeIterator( void * aIteratorMem);

    // To Fix BUG-15670
    // Row Pointer를 이용하여 Key Column의 실제 Value Ptr 획득
    // To Fix BUG-24449
    // 키 길이를 읽을때는 MT 함수를 이용해야 한다
    static IDE_RC getKeyValueAndSize( SChar         * aRowPtr,
                                      smnbColumn    * aIndexColumn,
                                      void          * aKeyValue,
                                      UShort        * aKeySize );

    static void logIndexNode( smnIndexHeader * aIndex,
                              smnbNode       * aNode );
    static void logIndexHeader( smnIndexHeader * aHeader );
    static void dumpIndexHeader( smnbHeader * aHeader,
                                 SChar      * aOutBuf,
                                 UInt         aOutSize );

    static IDE_RC makeKeyFromRow( smnbHeader   * aIndex,
                                  SChar        * aRow,
                                  void         * aKey );

    static inline void setInternalSlot( smnbINode   * aNode,
                                        SShort        aIdx,
                                        smnbNode    * aChildPtr,
                                        SChar       * aRowPtr,
                                        void        * aKey );

    static inline void getInternalSlot( smnbNode   ** aChildPtr,
                                        SChar      ** aRowPtr,
                                        void       ** aKey,
                                        smnbINode   * aNode,
                                        SShort        aIdx );

    static inline void setLeafSlot( smnbLNode   * aNode,
                                    SShort        aIdx,
                                    SChar       * aRowPtr,
                                    void        * aKey );

    static inline void getLeafSlot( SChar      ** aRowPtr,
                                    void       ** aKey,
                                    smnbLNode   * aNode,
                                    SShort        aIdx );

    static inline void copyLeafSlots( smnbLNode * aDest,
                                      SShort      aDestStartIdx,
                                      smnbLNode * aSrc,
                                      SShort      aSrcStartIdx,
                                      SShort      aSrcEndIdx );

    static void setNodeSplitRate( void ); /* BUG-40509 */

    static UInt getNodeSplitRate( void ); /* BUG-40509 */

    /* PROJ-2614 Memory Index Reorganization */
    static IDE_RC keyReorganization( smnbHeader * aIndex );

private:

    /*************************************************/
    /* PRIVATE FUNCTIONS                             */
    /*************************************************/

    static SInt compareRowsAndPtr( smnbStatistic    * aIndexStat,
                                   const smnbColumn * aColumns,
                                   const smnbColumn * aFence,
                                   const void       * aRow1,
                                   const void       * aRow2 );
    
    static SInt compareKeyVarColumn( smnbColumn * aColumn,
                                     void       * aKey,
                                     UInt         aPartialKeySize,
                                     SChar      * aRow );

    static SInt compareVarColumn( const smnbColumn * aColumn,
                                  const void       * aRow1,
                                  const void       * aRow2 );   

    static idBool isVarNull( smnbColumn * aColumn,
                             smiColumn  * aSmiColumn,
                             SChar      * aRow );
 
    static UInt getMaxKeyValueSize( smnIndexHeader *aIndexHeader );

    // BUG-19249
    static inline UInt getKeySize( UInt aKeyValueSize );

    static IDE_RC insertIntoInternalNode( smnbHeader  * aHeader,
                                          smnbINode   * a_pINode,
                                          void        * a_pRow,
                                          void        * aKey,
                                          void        * a_pChild );

    /* BUG-47206 : findKeyInLeaf(), findKeyInInternal() 함수를 통합한다. */
    static IDE_RC findKeyInNode( smnbHeader   * aHeader,
                                 smnbNode     * aNode,
                                 SInt           aMinimum,
                                 SInt           aMaximum,
                                 SChar        * aRow,
                                 SInt         * aSlot );

    /* BUG-47206 : findRowInLeaf(), findRowInInternal() 함수를 통합한다. */
    static void findRowInNode( smnbHeader   * aHeader,
                               smnbNode     * aNode,
                               SInt           aMinimum,
                               SInt           aMaximum,
                               SChar        * aRow,
                               SInt         * aSlot );

    /* BUG-47206 : findSlotInLeaf(), findSotInInternal() 함수를 통합한다. */
    static IDE_RC findSlotInNode( smnbHeader  * aHeader,
                                  smnbNode    * aNode,
                                  void        * aRow,
                                  SInt        * aSlot );

    static IDE_RC insertIntoLeafNode( smnbHeader      * aHeader,
                                      smnbLNode       * aLeafNode,
                                      SChar           * aRow,
                                      SInt              aSlotPos );

    static IDE_RC updateStat4Insert( smnIndexHeader  * aPersIndex,
                                     smnbHeader      * aIndex,
                                     smnbLNode       * aLeafNode,
                                     SInt              aSlotPos, 
                                     SChar           * aRow,
                                     idBool            aIsUniqueIndex );

    static SChar* getPrvLNodeLstRow( smnbLNode * a_pLeafNode );
    static SChar* getNxtLNodeFstRow( smnbLNode * a_pLeafNode );

    /* BUG-47206 : findFirstSlotInLeaf(), findFirstSlotInInternal() 함수를 통합한다. */
    static inline void findFirstSlotInNode( smnbHeader          * aHeader,
                                            const smiCallBack   * aCallBack,
                                            const smnbNode      * aNode,
                                            SInt                * aSlot );

    static IDE_RC findLast( smnbHeader       * aIndexHeader,
                            const smiCallBack* a_pCallBack,
                            SInt*              a_pDepth,
                            smnbStack*         a_pStack,
                            SInt*              a_pSlot );
    
    static IDE_RC afterLastInternal( smnbIterator* a_pIterator );
    
    static inline void findLastSlotInInternal( smnbHeader          * aHeader,
                                               const smiCallBack   * aCallBack,
                                               const smnbINode     * aNode,
                                               SInt                  aMinimum,
                                               SInt                  aMaximum,
                                               SInt                * aSlot );

    static inline void findLastSlotInLeaf( smnbHeader          * aHeader,
                                           const smiCallBack   * aCallBack,
                                           const smnbLNode     * aNode,
                                           SInt                  aMinimum,
                                           SInt                  aMaximum,
                                           SInt                * aSlot );

    static IDE_RC findLstLeaf( smnbHeader        * aIndexHeader,
                               const smiCallBack * a_pCallBack,
                               SInt              * a_pDepth,
                               smnbStack         * a_pStack,
                               smnbLNode        ** a_pLNode );

    static IDE_RC splitInternalNode( smnbHeader      * a_pIndexHeader,
                                     void            * a_pRow,
                                     void            * aKey,
                                     void            * a_pChild,
                                     SInt              a_nSlotPos,
                                     smnbINode       * a_pINode,
                                     smnbINode       * a_pNewINode,
                                     void           ** a_pSepKeyRow,
                                     void           ** aSepKey );

    static IDE_RC splitLeafNode( smnbHeader       * aIndexHeader,
                                 SInt               aSlotPos,
                                 smnbLNode        * aLeafNode,
                                 smnbLNode        * aNewLeafNode,
                                 smnbLNode       ** aTargetLeaf,
                                 SInt             * aTargetSeq );

    /* BUG-47206
       동일한 기능을 수행하던 findFirst(=findFstLeaf) 함수와 findPosition 함수를
       findPosition 으로 통합한다. */
    static IDE_RC findPosition( smnbHeader         * aIndexHeader,
                                const smiCallBack  * aCallBack,
                                SChar              * aRow,
                                SInt               * aDepth,
                                smnbStack          * aStack,
                                smnbLNode         ** aLNode);

    static IDE_RC checkUniqueness( smnIndexHeader   * aIndexHeader,
                                   void             * a_pTrans,
                                   smnbStatistic    * aIndexStat,
                                   const smnbColumn * a_pColumns,
                                   const smnbColumn * a_pFence,
                                   smnbLNode        * a_pLeafNode,
                                   SInt               a_nSlotPos,
                                   SChar            * a_pRow,
                                   smSCN              aStmtViewSCN,
                                   idBool             sIsTreeLatched,
                                   smTID            * a_pTransID,
                                   idBool           * sIsRetraverse,
                                   SChar           ** aExistUniqueRow,
                                   SChar           ** a_diffRow,
                                   SInt             * a_diffSlotPos,
                                   idBool             aForbiddenToRetry );

    static void deleteSlotInLeafNode( smnbLNode*        a_pLeafNode,
                                      SChar*            a_pRow,
                                      SInt*             a_pSlotPos );
    
    static IDE_RC updateStat4Delete( smnIndexHeader  * aPersIndex,
                                     smnbHeader      * aIndex,
                                     smnbLNode       * a_pLeafNode,
                                     SChar           * a_pRow,
                                     SInt            * a_pSlotPos,
                                     idBool            aIsUniqueIndex);

    static IDE_RC deleteSlotInInternalNode( smnbINode*        a_pInternalNode,
                                            SInt              a_nSlotPos );

    static IDE_RC rebuildMinMaxStat( smnIndexHeader * aPersistentIndexHeader,
                                     smnbHeader     * aRuntimeIndexHeader,
                                     idBool           aMinStat );

    static IDE_RC analyzeNode( smnIndexHeader * aPersistentIndexHeader,
                               smnbHeader     * aRuntimeIndexHeader,
                               smnbLNode      * aTargetNode,
                               SLong          * aClusteringFactor,
                               SLong          * aNumDist,
                               SLong          * aKeyCount,
                               SLong          * aMetaSpace,
                               SLong          * aUsedSpace,
                               SLong          * aAgableSpace, 
                               SLong          * aFreeSpace );

    static void getNextNode4Stat( smnbHeader     * aIdxHdr,
                                  idBool           aBackTraverse,
                                  idBool         * aNodeLatched,
                                  UInt           * aIndexHeight,
                                  smnbLNode     ** aCurNode );

    static IDE_RC beforeFirstInternal( smnbIterator* a_pIterator );
    
    static IDE_RC getNextNode( smnbIterator * aIterator,
                               idBool       * aIsRestart );

    static IDE_RC getPrevNode( smnbIterator   * aIterator,
                               idBool         * aIsRestart );

    static IDE_RC fetchNextR( smnbIterator* aIterator );

    static inline IDE_RC destroyNode( smnbNode * a_pNode );

    static inline void lockNode( smnbLNode * a_pLeafNode );
    static inline void unlockNode( smnbLNode * a_pLeafNodne );
    
    static IDU_LATCH getLatchValueOfINode( volatile smnbINode* aNodePtr );

    static IDU_LATCH getLatchValueOfLNode( volatile smnbLNode* aNodePtr );

    /* TASK-4990 changing the method of collecting index statistics */
    static IDE_RC setMinMaxStat( smnIndexHeader * aCommonHeader,
                                 SChar          * aRowPtr,
                                 smnbColumn     * aIndexColumn,
                                 idBool           aIsMinStat );

    static inline idBool needToUpdateStat( idBool aIsMemTBS );

    static IDE_RC dumpIndexNode( smnIndexHeader * aIndex,
                                 smnbNode       * aNode,
                                 SChar          * aOutBuf,
                                 UInt             aOutSize );

    static IDE_RC prepareNodes( smnbHeader    * aIndexHeader,
                                smnbStack     * aStack,
                                SInt            aDepth,
                                smnbNode     ** aNewNodeList,
                                UInt          * aNewNodeCount );

    static void getPreparedNode( smnbNode    ** aNodeList,
                                 UInt         * aNodeCount,
                                 void        ** aNode );

    static void setIndexProperties();

    static inline void copyInternalSlots( smnbINode * aDest,
                                          SShort      aDestStartIdx,
                                          smnbINode * aSrc,
                                          SShort      aSrcStartIdx,
                                          SShort      aSrcEndIdx );
    static inline void shiftInternalSlots( smnbINode    * aNode,
                                           SShort         aStartIdx,
                                           SShort         aEndIdx,
                                           SShort         aShift );
    static inline void shiftLeafSlots( smnbLNode    * aNode,
                                       SShort         aStartIdx,
                                       SShort         aEndIdx,
                                       SShort         aShift );

    /* BUG-47206 : findFirstRowInLeaf(), findFirstRowInInternal() 함수를 통합한다. */
    static void findFirstRowInNode( const smiCallBack   * aCallBack,
                                    const smnbNode      * aNode,
                                    SInt                  aMinimum,
                                    SInt                  aMaximum,
                                    SInt                * aSlot );

    /* BUG-47206 : findFirstKeyInLeaf(), findFirstKeyInInternal() 함수를 통합한다. */
    static void findFirstKeyInNode( smnbHeader          * aHeader,
                                    const smiCallBack   * aCallBack,
                                    const smnbNode      * aNode,
                                    SInt                  aMinimum,
                                    SInt                  aMaximum,
                                    SInt                * aSlot );

    static void findLastRowInInternal( const smiCallBack   * aCallBack,
                                       const smnbINode     * aNode,
                                       SInt                  aMinimum,
                                       SInt                  aMaximum,
                                       SInt                * aSlot );
    static void findLastKeyInInternal( smnbHeader          * aHeader,
                                       const smiCallBack   * aCallBack,
                                       const smnbINode     * aNode,
                                       SInt                  aMinimum,
                                       SInt                  aMaximum,
                                       SInt                * aSlot );
    static void findLastRowInLeaf( const smiCallBack   * aCallBack,
                                   const smnbLNode     * aNode,
                                   SInt                  aMinimum,
                                   SInt                  aMaximum,
                                   SInt                * aSlot );
    static void findLastKeyInLeaf( smnbHeader          * aHeader,
                                   const smiCallBack   * aCallBack,
                                   const smnbLNode     * aNode,
                                   SInt                  aMinimum,
                                   SInt                  aMaximum,
                                   SInt                * aSlot );

    static IDE_RC setDirectKeyIndex( smnbHeader         * aHeader,
                                     smnIndexHeader     * aIndex,
                                     const smiColumn    * aColumn,
                                     UInt                 aMtdAlign );

    /* BUG-47206 : isFullKeyInLeafSlot(), isFullKeyInInternalSlot() 함수를 통합한다. */
    static inline idBool isFullKey( smnbHeader      * aIndex,
                                    const smnbNode  * aNode,
                                    SShort            aIdx );

    static iduMemPool   mIteratorPool;

    /* PROJ-2613 Key Redistibution in MRDB Index */
    static inline idBool checkEnableKeyRedistribution( smnbHeader      * aIndex,
                                                       const smnbLNode * aLNode,
                                                       const smnbINode * aINode );
    
    static inline SInt calcKeyRedistributionPosition( smnbHeader    * aIndex,
                                                      SInt            aCurSlotCount,
                                                      SInt            aNxtSlotCount );

    static IDE_RC keyRedistribute( smnbHeader * aIndex,
                                   smnbLNode  * aCurNode,
                                   smnbLNode  * aNxtNode,
                                   SInt         aKeyRedistributeCount );

    static IDE_RC keyRedistributionPropagate( smnbHeader * aIndex,
                                              smnbINode  * aINode,
                                              smnbLNode  * aLNode,
                                              SChar      * aOldRowPtr );
    /* PROJ-2613 end */

    /* BUG-44043, BUG-46428 */
    static inline void findNextSlotInLeaf( smnbHeader       * index,
                                           const smnbLNode  * aNode,
                                           SInt               aMinimum,
                                           SInt               aMaximum,
                                           SChar            * aSearchRow,
                                           SInt             * aSlot );

    static inline idBool checkEnableReorgInNode( smnbLNode * aLNode,
                                                 smnbINode * aINode,
                                                 SShort      aSlotMaxCount );

    static void setInconsistentIndex( smnIndexHeader * aIndex );

    static IDE_RC makeRootNode( smnIndexHeader * aIndex,
                                SChar          * aRow,
                                idBool           aIsUniqueIndex );

    static IDE_RC processNodeSplit( smnIndexHeader * aIndex,
                                    smnbStack      * aStack,
                                    SInt             aDepth,
                                    smnbLNode      * aCurLNode,
                                    SChar          * aRow,
                                    SInt             aSlotPos,
                                    idBool           aIsUniqueIndex );

    static IDE_RC processKeyRedistribution( smnIndexHeader * aIndex, 
                                            smnbINode      * aCurINode,
                                            smnbLNode      * aCurLNode,
                                            idBool         * aIsTreeLatched,
                                            idBool         * aIsNodeLatched );

    static void dumpNodeLink( smnbLNode * aLNode );

public:
    static smmSlotList  mSmnbNodePool;
    static void*        mSmnbFreeNodeList;
    static UInt         mNodeSize;
    static UInt         mNodeSplitRate; /* BUG-40509 */

private:
    static UInt         mIteratorSize;
};

// BUG-19249
inline UInt smnbBTree::getKeySize( UInt aKeyValueSize )
{
    return idlOS::align8( aKeyValueSize + ID_SIZEOF(SChar*));
}

/* PROJ-2433 */
#define SMNB_DEFAULT_MAX_KEY_SIZE() \
    ( smuProperty::getMemBtreeDefaultMaxKeySize() );

/* PROJ-2433 
 *
 * internal node를 초기화한다
 *
 * - direct key index를 사용하더라도 항상 row pointer 가 있어야한다
 *   : non-unique index에서 동일 key가 여러개 insert되면, row 포인터 주소 순서로 정렬해 저장한다.
 *   : uninque index라면 생략할수있겠지만, 어차피 index 크기를 결정하는것은 leaf node 이다.
 */
inline void smnbBTree::initInternalNode( smnbINode  * a_pNode,
                                         smnbHeader * aIndexHeader,
                                         IDU_LATCH    a_latch )
{
    idlOS::memset( a_pNode,
                   0,
                   mNodeSize );

    a_pNode->prev           = NULL;
    a_pNode->next           = NULL;
    SM_SET_SCN_CONST( &(a_pNode->scn), 0 );
    a_pNode->freeNodeList   = NULL;
    a_pNode->latch          = a_latch;
    a_pNode->flag           = SMNB_NODE_TYPE_INTERNAL;
    a_pNode->prevSPtr       = NULL;
    a_pNode->nextSPtr       = NULL;
    a_pNode->sequence       = 0;
    a_pNode->mKeySize       = aIndexHeader->mKeySize;
    a_pNode->mMaxSlotCount  = aIndexHeader->mINodeMaxSlotCount;
    a_pNode->mSlotCount     = 0;

    /* 항상 row poniter 존재 */
    a_pNode->mRowPtrs = (SChar **)&(a_pNode->mChildPtrs[a_pNode->mMaxSlotCount]);

    if ( a_pNode->mKeySize == 0 )
    {
        a_pNode->mKeys = NULL;
    }
    else /* KEY INDEX */
    {
        a_pNode->mKeys = (void  **)&(a_pNode->mRowPtrs[a_pNode->mMaxSlotCount]); 
    }
}

/* PROJ-2433 
 *
 * leaf node를 초기화한다
 */
inline IDE_RC smnbBTree::initLeafNode( smnbLNode  * a_pNode,
                                       smnbHeader * aIndexHeader,
                                       IDU_LATCH    a_latch )
{
    SChar   sBuf[IDU_MUTEX_NAME_LEN];

    // To fix BUG-17337
    idlOS::memset( a_pNode,
                   0,
                   mNodeSize );

    a_pNode->prev        = NULL;
    a_pNode->next        = NULL;
    SM_SET_SCN_CONST( &(a_pNode->scn), 0 );
    a_pNode->freeNodeList= NULL;
    a_pNode->latch       = a_latch;
    a_pNode->flag        = SMNB_NODE_TYPE_LEAF;
    a_pNode->prevSPtr    = NULL;
    a_pNode->nextSPtr    = NULL;

    /* init Node Latch */
    idlOS::snprintf( sBuf, IDU_MUTEX_NAME_LEN, "SMNB_MUTEX_NODE_%"ID_UINT64_FMT,
                     (ULong)a_pNode );
    IDE_TEST( a_pNode->nodeLatch.initialize( sBuf,
                                             IDU_MUTEX_KIND_POSIX,
                                             IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    a_pNode->sequence      = 0;
    a_pNode->mKeySize      = aIndexHeader->mKeySize;
    a_pNode->mMaxSlotCount = aIndexHeader->mLNodeMaxSlotCount;
    a_pNode->mSlotCount    = 0;

    a_pNode->mRowPtrs = (SChar **)( ((SChar *)a_pNode) + ID_SIZEOF( smnbLNode ) );

    if ( a_pNode->mKeySize == 0 )
    {
        a_pNode->mKeys = NULL;
    }
    else /* DIRECT KEY INDEX */
    {
        a_pNode->mKeys = (void **)&(a_pNode->mRowPtrs[a_pNode->mMaxSlotCount]);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


inline IDE_RC smnbBTree::destroyNode( smnbNode * a_pNode )
{
    smnbLNode   * s_pLeafNode;

    IDE_DASSERT( a_pNode != NULL );

    if ( SMNB_IS_LEAF_NODE( a_pNode ) )
    {
        s_pLeafNode = (smnbLNode*)a_pNode;
        IDE_TEST( s_pLeafNode->nodeLatch.destroy() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

inline void smnbBTree::lockTree( smnbHeader * a_pIndex )
{
    IDE_DASSERT( a_pIndex != NULL );
    (void)a_pIndex->mTreeMutex.lock( NULL /* idvSQL */ ); /* always IDE_SUCCESS return */
}

inline void smnbBTree::unlockTree( smnbHeader * a_pIndex )
{
    IDE_DASSERT( a_pIndex != NULL );
    (void)a_pIndex->mTreeMutex.unlock(); /* always IDE_SUCCESS return */
}

inline void smnbBTree::lockNode( smnbLNode * a_pLeafNode )
{
    IDE_DASSERT( a_pLeafNode != NULL );
    IDE_DASSERT( SMNB_IS_LEAF_NODE( a_pLeafNode ) );
    (void)a_pLeafNode->nodeLatch.lock( NULL /* idvSQL */ ); /* always IDE_SUCCESS return */
}

inline void smnbBTree::unlockNode( smnbLNode * a_pLeafNode )
{
    IDE_DASSERT( a_pLeafNode != NULL );
    IDE_DASSERT( SMNB_IS_LEAF_NODE( a_pLeafNode ) );
    (void)a_pLeafNode->nodeLatch.unlock(); /* always IDE_SUCCESS return */
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::setInternalSlot                 *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * INTERNAL NODE의 aIdx번째 SLOT에
 * child pointer, row pointer, direct key를 세팅한다.
 *
 * - direct key는 memcpy가 이루어지는것에 주의
 *
 * aNode     - [IN]  INTERNAL NODE
 * aIdx      - [IN]  slot index
 * aChildPtr - [IN]  child pointer
 * aRowPtr   - [IN]  row pointer
 * aKey      - [IN]  direct key pointer
 *********************************************************************/
inline void smnbBTree::setInternalSlot( smnbINode   * aNode,
                                        SShort        aIdx,
                                        smnbNode    * aChildPtr,
                                        SChar       * aRowPtr,
                                        void        * aKey )
{
    IDE_DASSERT( aIdx >= 0 );

    aNode->mChildPtrs[aIdx] = aChildPtr;
    aNode->mRowPtrs  [aIdx] = aRowPtr;

    if ( SMNB_IS_DIRECTKEY_IN_NODE( aNode ) == ID_TRUE )
    { 
        if ( aKey != NULL )
        {
            idlOS::memcpy( SMNB_GET_KEY_PTR( aNode, aIdx ),
                           aKey,
                           aNode->mKeySize ); 
        }
        else
        { 
            idlOS::memset( SMNB_GET_KEY_PTR( aNode, aIdx ),
                           0,
                           aNode->mKeySize ); 
        }
    }
    else
    {
        /* nothing todo */
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::getInternalSlot                 *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * INTERNAL NODE의 aIdx번째 SLOT의
 * child pointer, row pointer, direct key pointer를 가져온다.
 *
 * aChildPtr - [OUT] child pointer
 * aRowPtr   - [OUT] row pointer
 * aKey      - [OUT] direct key pointer
 * aNode     - [IN]  INTERNAL NODE
 * aIdx      - [IN]  slot index
 *********************************************************************/
inline void smnbBTree::getInternalSlot( smnbNode ** aChildPtr,
                                        SChar    ** aRowPtr,
                                        void     ** aKey,
                                        smnbINode * aNode,
                                        SShort      aIdx )
{
    if ( aChildPtr != NULL )
    {
        *aChildPtr = aNode->mChildPtrs[aIdx];
    }
    else
    {
        /* nothing todo */
    }

    if ( aRowPtr != NULL )
    {
        *aRowPtr = aNode->mRowPtrs[aIdx];
    }
    else
    {
        /* nothing todo */
    }

    if ( aKey != NULL )
    {
        if ( SMNB_IS_DIRECTKEY_IN_NODE( aNode ) == ID_TRUE )
        {
            *aKey = SMNB_GET_KEY_PTR( aNode, aIdx );
        }
        else
        {
            *aKey = NULL; /* 값이설정안되어있는경우NULL반환*/
        }
    }
    else
    {
        /* nothing todo */
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::copyInternalSlots               *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * INTERNAL NODE 내의 특정범위의 slot들을 다른 INTERNAL NODE 로 복사한다.
 *
 * - aSrc의 aSrcSTartIdx부터 aSrcEndIdx까지의 slot들을
 *   aDest의 aDestStartIdex로 복사한다.
 * - aSrc, aDest는 다른 NODE 이어야 한다.
 *
 * aDest         - [IN] 대상 INTERNAL NODE
 * aDestStartIdx - [IN] 대상 시작 slot index
 * aSrc          - [IN] 원본 INTERNAL NODE
 * aSrcStartIdx  - [IN] 복사할 시작 slot index
 * aSrcEndIdx    - [IN] 복사할 마지막 slot index
 *********************************************************************/
inline void smnbBTree::copyInternalSlots( smnbINode * aDest,
                                          SShort      aDestStartIdx,
                                          smnbINode * aSrc,
                                          SShort      aSrcStartIdx,
                                          SShort      aSrcEndIdx )
{
    IDE_DASSERT( aSrcStartIdx <= aSrcEndIdx );

    idlOS::memcpy( &aDest->mChildPtrs[aDestStartIdx],
                   &aSrc ->mChildPtrs[aSrcStartIdx],
                   ID_SIZEOF( aDest->mChildPtrs[0] ) * ( aSrcEndIdx - aSrcStartIdx + 1 ) );

    idlOS::memcpy( &aDest->mRowPtrs[aDestStartIdx],
                   &aSrc ->mRowPtrs[aSrcStartIdx],
                   ID_SIZEOF( aDest->mRowPtrs[0] ) * ( aSrcEndIdx - aSrcStartIdx + 1 ) );

    if ( SMNB_IS_DIRECTKEY_IN_NODE( aDest ) == ID_TRUE )
    {
        idlOS::memcpy( SMNB_GET_KEY_PTR( aDest, aDestStartIdx ),
                       SMNB_GET_KEY_PTR( aSrc, aSrcStartIdx ),
                       aDest->mKeySize * ( aSrcEndIdx - aSrcStartIdx + 1 ) );
    }
    else
    {
        /* nothing todo */
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::shiftInternalSlots              *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * INTERNAL NODE 내에서 특정범위의 slot들을 shift 시킨다.
 *
 * - aShift가 양수이면 slot들을 오른쪽으로 shift 하고,
 *   aShift가 음수이면 slot들을 왼쪽으로 shift 한다.
 *
 * aNode      - [IN] INTERNAL NODE
 * aStartIdx  - [IN] shift 시킬 시작 slot index
 * aEndIdx    - [IN] shift 시킬 마지막 slot index
 * aShift     - [IN] shift값
 *********************************************************************/
inline void smnbBTree::shiftInternalSlots( smnbINode * aNode,
                                           SShort      aStartIdx,
                                           SShort      aEndIdx,
                                           SShort      aShift )
{
    SInt i = 0;

    IDE_DASSERT( aStartIdx <= aEndIdx );

    /* BUG-41787
     * 기존에는 index node의 slot 이동시 memmove() 함수를 사용하였는데
     * slot 이동중에 pointer를 참조하면 완성되지 않은 주소값을 읽어서 segment fault가 발생할수있음.
     * 그래서, 아래와 같이 for문을 사용하여 slot을 하나씩 이동하도록 수정하였음.
     *
     * direct key의 경우는 주소값이 아니므로, 문제가 되지 않음 */

    if ( aShift > 0 ) 
    {
        for ( i = aEndIdx;
              i >= aStartIdx;
              i-- )
        {
            aNode->mChildPtrs[ i + aShift ] =  aNode->mChildPtrs[ i ];
            aNode->mRowPtrs[ i + aShift ] =  aNode->mRowPtrs[ i ];
        }
    }
    else
    {
        /* nothing todo */
    }

    if ( aShift < 0)
    {
        for ( i = aStartIdx;
              i <= aEndIdx;
              i++ )
        {
            aNode->mChildPtrs[ i + aShift ] =  aNode->mChildPtrs[ i ];
            aNode->mRowPtrs[ i + aShift ] =  aNode->mRowPtrs[ i ];
        }
    }
    else
    {
        /* nothing todo */
    }

    /* Shift direct key */
    if ( SMNB_IS_DIRECTKEY_IN_NODE( aNode ) == ID_TRUE )
    {
        idlOS::memmove( ((SChar *)aNode->mKeys) + ( aNode->mKeySize * ( aStartIdx + aShift ) ),
                        ((SChar *)aNode->mKeys) + ( aNode->mKeySize * ( aStartIdx ) ),
                        aNode->mKeySize * ( aEndIdx - aStartIdx + 1 ) );
    }
    else
    {
        /* nothing todo */
    }

}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::setLeafSlot                     *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * LEAF NODE의 aIdx번째 SLOT에
 * row pointer, direct key를 세팅한다.
 *
 * - direct key는 memcpy가 이루어지는것에 주의
 *
 * aNode     - [IN]  LEAF NODE
 * aIdx      - [IN]  slot index
 * aRowPtr   - [IN]  row pointer
 * aKey      - [IN]  direct key pointer
 *********************************************************************/
inline void smnbBTree::setLeafSlot( smnbLNode * aNode,
                                    SShort      aIdx,
                                    SChar     * aRowPtr,
                                    void      * aKey )
{
    IDE_DASSERT( aIdx >= 0 );

    aNode->mRowPtrs[aIdx] = aRowPtr;

    if ( SMNB_IS_DIRECTKEY_IN_NODE( aNode ) == ID_TRUE )
    {
        if ( aKey != NULL)
        {
            idlOS::memcpy( SMNB_GET_KEY_PTR( aNode, aIdx ),
                           aKey,
                           aNode->mKeySize ); 
        }
        else
        { 
            idlOS::memset( SMNB_GET_KEY_PTR( aNode, aIdx ),
                           0,
                           aNode->mKeySize ); 
        }
    }
    else
    {
        /* nothing todo */
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::getLeafSlot                     *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * LEAF NODE의 aIdx번째 SLOT의
 * row pointer, direct key pointer를 가져온다.
 *
 * aRowPtr   - [OUT] row pointer
 * aKey      - [OUT] direct key pointer
 * aNode     - [IN]  LEAF NODE
 * aIdx      - [IN]  slot index
 *********************************************************************/
inline void smnbBTree::getLeafSlot( SChar       ** aRowPtr,
                                    void        ** aKey,
                                    smnbLNode   *  aNode,
                                    SShort         aIdx )
{
    if ( aRowPtr != NULL )
    {
        *aRowPtr = aNode->mRowPtrs[aIdx];
    }
    else
    {
        /* nothing todo */
    }

    if ( aKey != NULL )
    {
        if ( SMNB_IS_DIRECTKEY_IN_NODE( aNode ) == ID_TRUE )
        {
            *aKey = SMNB_GET_KEY_PTR( aNode, aIdx );
        }
        else
        {
            *aKey = NULL; /* 값이설정안되어있는경우NULL반환*/
        }
    }
    else
    {
        /* nothing todo */
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::copyLeafSlots                   *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * LEAF NODE 내의 특정범위의 slot들을 다른 LEAF NODE 로 복사한다.
 *
 * - aSrc의 aSrcSTartIdx부터 aSrcEndIdx까지의 slot들을
 *   aDest의 aDestStartIdex로 복사한다.
 * - aSrc, aDest는 다른 NODE 이어야 한다.
 *
 * aDest         - [IN] 대상 LEAF NODE
 * aDestStartIdx - [IN] 대상 시작 slot index
 * aSrc          - [IN] 원본 LEAF NODE
 * aSrcStartIdx  - [IN] 복사할 시작 slot index
 * aSrcEndIdx    - [IN] 복사할 마지막 slot index
 *********************************************************************/
inline void smnbBTree::copyLeafSlots( smnbLNode * aDest,
                                      SShort      aDestStartIdx,
                                      smnbLNode * aSrc,
                                      SShort      aSrcStartIdx,
                                      SShort      aSrcEndIdx )
{
    IDE_DASSERT( aSrcStartIdx <= aSrcEndIdx );

    idlOS::memcpy( &aDest->mRowPtrs[aDestStartIdx],
                   &aSrc ->mRowPtrs[aSrcStartIdx],
                   ID_SIZEOF( aDest->mRowPtrs[0] ) * ( aSrcEndIdx - aSrcStartIdx + 1 ) );

    if ( SMNB_IS_DIRECTKEY_IN_NODE( aDest ) == ID_TRUE )
    {
        idlOS::memcpy( SMNB_GET_KEY_PTR( aDest, aDestStartIdx ),
                       SMNB_GET_KEY_PTR( aSrc, aSrcStartIdx ),
                       aDest->mKeySize * ( aSrcEndIdx - aSrcStartIdx + 1 ) );
    }
    else
    {
        /* nothing todo */
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::shiftLeafSlots                  *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * LEAF NODE 내에서 특정범위의 slot들을 shift 시킨다.
 *
 * - aShift가 양수이면 slot들을 오른쪽으로 shift 하고,
 *   aShift가 음수이면 slot들을 왼쪽으로 shift 한다.
 *
 * aNode      - [IN] LEAF NODE
 * aStartIdx  - [IN] shift 시킬 시작 slot index
 * aEndIdx    - [IN] shift 시킬 마지막 slot index
 * aShift     - [IN] shift값
 *********************************************************************/
inline void smnbBTree::shiftLeafSlots( smnbLNode * aNode,
                                       SShort      aStartIdx,
                                       SShort      aEndIdx,
                                       SShort      aShift )
{
    SInt i = 0;

    IDE_DASSERT( aStartIdx <= aEndIdx );

    /* BUG-41787
     * 기존에는 index node의 slot 이동시 memmove() 함수를 사용하였는데
     * slot 이동중에 pointer를 참조하면 완성되지 않은 주소값을 읽어서 segment fault가 발생할수있음.
     * 그래서, 아래와 같이 for문을 사용하여 slot을 하나씩 이동하도록 수정하였음.
     *
     * direct key의 경우는 주소값이 아니므로, 문제가 되지 않음 */

    if ( aShift > 0 ) 
    {
        for ( i = aEndIdx;
              i >= aStartIdx;
              i-- )
        {
            aNode->mRowPtrs[ i + aShift ] = aNode->mRowPtrs[ i ];
        }
    }
    else
    {
        /* nothing todo */
    }

    if ( aShift < 0)
    {
        for ( i = aStartIdx;
              i <= aEndIdx;
              i++ )
        {
            aNode->mRowPtrs[ i + aShift ] = aNode->mRowPtrs[ i ];
        }
    }
    else
    {
        /* nothing todo */
    }

    /* Shift direct key */
    if ( SMNB_IS_DIRECTKEY_IN_NODE( aNode ) == ID_TRUE )
    {
        idlOS::memmove( ((SChar *)aNode->mKeys) + ( aNode->mKeySize * ( aStartIdx + aShift ) ),
                        ((SChar *)aNode->mKeys) + ( aNode->mKeySize * ( aStartIdx ) ),
                        aNode->mKeySize * ( aEndIdx - aStartIdx + 1 ) );
    }
    else
    {
        /* nothing todo */
    }
}

/* BUG-40509 Change Memory Index Node Split Rate */
inline void smnbBTree::setNodeSplitRate()
{
    mNodeSplitRate = smuProperty::getMemoryIndexUnbalancedSplitRate();
}
inline UInt smnbBTree::getNodeSplitRate()
{
    return mNodeSplitRate;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::calcKeyRedistributionPosition   *
 * ------------------------------------------------------------------*
 * PROJ-2613 Key Redistribution in MRDB Index
 * 키 재분배가 수행될지 여부를 판단한다.
 * 수행 여부를 판단하는 기준은 다음과 같다.
 *  1. MEM_INDEX_KEY_REDISTRIBUTION이 켜져있음(1)
 *  2. 해당 인덱스가 키 재분배 기능을 사용하도록 세팅되어 있음
 *  3. 키 재분배가 수행할 노드의 이웃 노드가 존재
 *  4. 이웃 노드가 키 재분배가 수행할 노드와 부모 노드가 같음
 *  5. 이웃 노드의 빈 slot이 충분히 존재
 * 위의 다섯가지 조건을 모두 만족할 경우에만 키 재분배를 수행한다.
 *
 * aIndex    - [IN] Index Header
 * aLNode    - [IN] 키 재분배를 수행할지 판단해야 하는 leaf node
 * aINode    - [IN] 키 재분배를 수행할 leaf node의 부모 노드
 *********************************************************************/
inline idBool smnbBTree::checkEnableKeyRedistribution( smnbHeader      * aIndex,
                                                       const smnbLNode * aLNode,
                                                       const smnbINode * aINode )
{
    idBool  sRet = ID_FALSE;

    if ( aINode != NULL )
    {
        if ( smuProperty::getMemIndexKeyRedistribute() == ID_TRUE )
        {
            if ( ( (smnbLNode*)aINode->mChildPtrs[( aINode->mSlotCount ) - 1] ) != aLNode )
            {
                if ( aLNode->nextSPtr != NULL )
                {
                    /* 이웃 노드에 충분한 공간이 있는지 판단하는 기준은
                     * MEM_INDEX_KEY_REDISTRIBUTION_STANDARD_RATE 프로퍼티를 사용한다. */
                    if ( ( aLNode->nextSPtr->mSlotCount ) <
                         ( ( SMNB_LEAF_SLOT_MAX_COUNT( aIndex ) *
                             smuProperty::getMemIndexKeyRedistributionStandardRate() / 100 ) ) )
                    {
                        sRet = ID_TRUE;
                    }
                    else
                    {
                        /* 이웃 노드에 충분한 공간이 없는 경우 키 재분배를 수행하지 않는다. */
                    }
                }
                else
                {
                    /* 이웃 노드가 없는 경우는 키 재분배를 수행할 수 없다. */
                }
            }
            else
            {
                /* 해당 노드가 부모노드의 마지막 자식일 경우에는 키 재분배를 수행하지 않는다. */
            }
        }
        else
        {
            /* 프로퍼티가 꺼져 있다면 키 재분배를 수행하지 않는다. */
        }
    }
    else
    {
        /* 루트 노드에 값이 삽입 될 경우 해당 노드의 부모 노드가 존재하지 않는다.
         * 이 경우는 이웃 노드도 없기 때문에 키 재분배를 수행하지 않는다. */
    }
    return sRet;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::calcKeyRedistributionPosition   *
 * ------------------------------------------------------------------*
 * PROJ-2613 Key Redistribution in MRDB Index
 * 해당 노드 내 키 재분배가 시작될 위치를 계산한다.
 * 원래 이웃 노드로 이동이 시작될 위치를 계산후 리턴해야 하나
 * 코드의 간편성을 위해 이동이 시작될 위치가 아닌, 이동 시킬 slot의 수를 리턴하도록 한다.
 *
 * aIndex         - [IN] Index Header
 * aCurSlotCount  - [IN] 키 재분배가 수행될 노드(src node)의 slot count
 * aNxtSlotCount  - [IN] 키 재분배로 이동될 노드(dest node)의 slot count
 *********************************************************************/
inline SInt smnbBTree::calcKeyRedistributionPosition( smnbHeader    * aIndex,
                                                      SInt            aCurSlotCount,
                                                      SInt            aNxtSlotCount )
{
    SInt    sStartPosition = 0;

    sStartPosition = ( aCurSlotCount + aNxtSlotCount + 1 ) / 2;

    return (SInt)( SMNB_LEAF_SLOT_MAX_COUNT( aIndex ) - sStartPosition );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::checkEnableReorgInNode          *
 * ------------------------------------------------------------------*
 * PROJ-2614 Memory Index Reorganization
 * 두 노드간 통합 가능 여부를 확인하여 리턴한다.
 *
 * aLNode         - [IN] 통합 가능을 확인하기 위한 leaf node
 * aINode         - [IN] 통합 대상 leaf node의 부모노드
 * aSlotMaxCount  - [IN] 노드가 최대 가질수 있는 slot의 수
 *********************************************************************/
inline idBool smnbBTree::checkEnableReorgInNode( smnbLNode * aLNode,
                                                 smnbINode * aINode,
                                                 SShort      aSlotMaxCount )
{
    idBool  sEnableReorg = ID_FALSE;
    smnbLNode   * sCurLNode = aLNode;
    smnbLNode   * sNxtLNode = (smnbLNode*)aLNode->nextSPtr;

    IDE_ERROR( aLNode != NULL );
    IDE_ERROR( aINode != NULL );

    if ( sNxtLNode != NULL )
    {
        if ( sNxtLNode->nextSPtr != NULL )
        {
            if ( ( (smnbLNode*)aINode->mChildPtrs[ ( aINode->mSlotCount ) - 1 ] ) != aLNode )
            {
                if ( ( sCurLNode->mSlotCount + sNxtLNode->mSlotCount ) < aSlotMaxCount )
                {
                    sEnableReorg = ID_TRUE;
                }
                else
                {
                    /* 두 노드를 합쳤을 때 하나의 노드에 들어가지 않는다면
                     * 통합을 수행할 수 없다. */
                }
            }
            else
            {
                /* 현재 노드가 부모 노드의 마지막 자식이라면 다음 노드와
                 * 부모가 다르므로 통합을 수행할 수 없다. */
            }
        }
        else
        {
            /* fetchNext가 트리의 마지막 노드에서 대기 중일 경우 문제가 발생할 수 있으므로
             * 트리의 마지막 노드에 대해서는 통합을 수행하지 않는다.*/
        }
    }
    else
    {
        /* 이후 노드가 없거나 트리의 마지막 노드라면 통합을 수행할 수 없다. */
    }
    IDE_EXCEPTION_END;

    return sEnableReorg;
}
#endif /* _O_SMNB_MODULE_H_ */
