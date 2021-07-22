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
 * $Id: sdnbModule.h 89963 2021-02-09 05:22:10Z justin.kwon $
 **********************************************************************/

#ifndef _O_SDNB_MODULE_H_
# define _O_SDNB_MODULE_H_ 1

# include <idu.h>
# include <smDef.h>
# include <sdpDef.h>
# include <sdpPhyPage.h>
# include <smnDef.h>
# include <sdcDef.h>
# include <sdnDef.h>
# include <sdnbDef.h>

extern smnIndexModule sdnbModule;

typedef enum
{
    SDNB_VISIBILITY_NO,
    SDNB_VISIBILITY_YES,
    SDNB_VISIBILITY_UNKNOWN
} sdnbVisibility;

class sdnbBTree
{
public:
    
    static IDE_RC prepareIteratorMem( smnIndexModule* aModule );

    static IDE_RC releaseIteratorMem( const smnIndexModule* aModule );

    static IDE_RC freeAllNodeList(idvSQL         * aStatistics,
                                  smnIndexHeader * aIndex,
                                  void           * aTrans );
    
    static IDE_RC create( idvSQL             * aStatistics,
                          smcTableHeader     * aTable,
                          smnIndexHeader     * aIndex,
                          smiSegAttr         * aSegAttr,
                          smiSegStorageAttr  * aSegStoAttr,
                          smnInsertFunc      * aInsert,
                          smnIndexHeader    ** aRebuildIndexHeader,
                          ULong                aSmoNo );
    
    /*PROJ-1872 Disk index 저장 구조 최적화
     *런타임 헤더의 인덱스 칼럼 정보를 설정한다. 런타임 헤더 생성시,
     *실시간 AlterDDL로 인해 인덱스 칼럼을 재설정해야 할때 호출된다.*/
    static IDE_RC rebuildIndexColumn (
                          smnIndexHeader * aIndex,
                          smcTableHeader * aTable,
                          void           * aHeader);

    static IDE_RC makeFetchColumnList4Index(
                           void                   *aTableHeader,
                           const sdnbHeader       *aIndexHeader,
                           UShort                  aIndexColCount);
    
    // BUG-25351 : 키생성 및 인덱스 Build에 관한 모듈화 리팩토링이 필요합니다.
    static IDE_RC buildIndex( idvSQL*               aStatistics,
                              void*                 aTrans,
                              smcTableHeader*       aTable,
                              smnIndexHeader*       aIndex,
                              smnGetPageFunc        aGetPageFunc,
                              smnGetRowFunc         aGetRowFunc,
                              SChar*                aNullRow,
                              idBool                aIsNeedValidation,
                              idBool                aIsEnableTable,
                              UInt                  aParallelDegree,
                              UInt                  aBuildFlag,
                              UInt                  aTotalRecCnt );

    static IDE_RC buildDRTopDown(idvSQL               *aStatistics,
                                 void*                 aTrans,
                                 smcTableHeader*       aTable,
                                 smnIndexHeader*       aIndex,
                                 idBool                aIsNeedValidation,
                                 UInt                  aBuildFlag,
                                 UInt                  aParallelDegree );

    static IDE_RC buildDRBottomUp(idvSQL               *aStatistics,
                                  void*                 aTrans,
                                  smcTableHeader*       aTable,
                                  smnIndexHeader*       aIndex,
                                  idBool                aIsNeedValidation,
                                  UInt                  aBuildFlag,
                                  UInt                  aParallelDegree );
    static IDE_RC drop( smnIndexHeader * aIndex );

    static IDE_RC init( sdnbIterator        * aIterator,
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

    static IDE_RC dest( sdnbIterator* /*aIterator*/ );

    static IDE_RC traverse( idvSQL         * aStatistics,
                            sdnbHeader     * aIndex,
                            sdrMtx         * aMtx,
                            sdnbKeyInfo    * aKeyInfo,
                            sdnbStack      * aStack,
                            sdnbStatistic  * aIndexStat,
                            idBool           aIsPessimistic,
                            sdpPhyPageHdr ** aLeafNode,
                            SShort         * aLeafKeySeq ,
                            sdrSavePoint   * aLeafSP,  
                            idBool         * aIsSameKey,
                            SLong          * aTotalTraverseLength );

    static IDE_RC findValidStackDepth( idvSQL          *aStatistics,
                                       sdnbStatistic *  aIndexStat,
                                       sdnbHeader *     aIndex,
                                       sdnbStack *      aStack,
                                       ULong *          aSmoNo );
    
    static IDE_RC compareInternalKey ( sdpPhyPageHdr * aNode,
                                       sdnbStatistic * aIndexStat,
                                       SShort          aSrcSeq,
                                       SShort          aDestSeq,
                                       sdnbColumn    * aColumns,
                                       sdnbColumn    * aColumnFence,
                                       idBool        * aIsSameValue,
                                       SInt          * aResult );

    static IDE_RC validateInternal( sdpPhyPageHdr   * aNode,
                                    sdnbStatistic   * aIndexStat,
                                    sdnbColumn      * aColumns,
                                    sdnbColumn      * aColumnFence );

    static IDE_RC findInternalKey ( sdnbHeader           * aIndex,
                                    sdpPhyPageHdr        * aNode,
                                    sdnbStatistic        * aIndexStat,
                                    sdnbColumn           * aColumns,
                                    sdnbColumn           * aColumnFence,
                                    sdnbConvertedKeyInfo * aConvertedKeyInfo,
                                    ULong                  aIndexSmoNo,
                                    ULong                * aNodeSmoNo,
                                    scPageID             * aChildPID,
                                    SShort               * aKeyMapSeq,
                                    idBool               * aRetry,
                                    idBool               * aIsSameValueInSiblingNodes,
                                    scPageID             * aNextChildPID,
                                    UShort               * aNextSlotSize );

    static IDE_RC compareLeafKey (idvSQL        * aStatistics,
                                  sdpPhyPageHdr * aNode,
                                  sdnbStatistic * aIndexStat,
                                  sdnbHeader    * aIndex,
                                  SShort          aSrcSeq,
                                  SShort          aDestSeq,
                                  sdnbColumn    * aColumns,
                                  sdnbColumn    * aColumnFence,
                                  idBool        * aIsSameValue,
                                  SInt          * aResult );

    static IDE_RC validateNodeSpace( sdnbHeader    * aIndex,
                                     sdpPhyPageHdr * aNode,
                                     idBool          aIsLeaf );
    
    static IDE_RC validateLeaf(idvSQL *          aStatistics,
                               sdpPhyPageHdr *   aNode,
                               sdnbStatistic *   aIndexStat,
                               sdnbHeader *      aIndex,
                               sdnbColumn *      aColumns,
                               sdnbColumn *      aColumnFence);

    static IDE_RC findLeafKey ( sdpPhyPageHdr        * aNode,
                                sdnbStatistic        * aIndexStat, 
                                sdnbColumn           * aColumns,
                                sdnbColumn           * aColumnFence,
                                sdnbConvertedKeyInfo * aConvertedKeyInfo,
                                ULong                  aIndexSmoNo,
                                ULong                * aNodeSmoNo,
                                SShort               * aKeyMapSeq,
                                idBool               * aRetry,
                                idBool               * aIsSameKey );
    
    static SInt compareKeyAndVRow( sdnbStatistic    * aIndexStat,
                                   const sdnbColumn * aColumns,
                                   const sdnbColumn * aColumnFence,
                                   sdnbKeyInfo      * aKeyInfo, 
                                   sdnbKeyInfo      * aVRowInfo );

    //일대다 비교일 경우, 변환된 키와 일반 키를 비교한다.
    static SInt compareConvertedKeyAndKey( sdnbStatistic        * aIndexStat,
                                           const sdnbColumn     * aColumns,
                                           const sdnbColumn     * aColumnFence,
                                           sdnbConvertedKeyInfo * aConvertedKeyInfo,
                                           sdnbKeyInfo          * aKeyInfo, 
                                           UInt                   aCompareFlag,
                                           idBool               * aIsSameValue );
    
    static SInt compareKeyAndKey( sdnbStatistic    * aIndexStat,
                                  const sdnbColumn * aColumns,
                                  const sdnbColumn * aColumnFence,
                                  sdnbKeyInfo      * aKey1Info,
                                  sdnbKeyInfo      * aKey2Info,
                                  UInt               aCompareFlag,
                                  idBool           * aIsSameValue );

    /* Proj-1872 Disk index 저장 구조 최적화
     * Index Key구조를 고려하여 Null을 체크해야 한다. */
    static idBool isNullColumn( sdnbColumn       * aIndexColumn,
                                UChar            * aKeyPtr );

    /* BUG-30074 disk table의 unique index에서 NULL key 삭제 시/
     *           non-unique index에서 deleted key 추가 시
     *           cardinality의 정확성이 많이 떨어집니다.
     * Key 전체가 Null인지 확인한다. */
    static idBool isNullKey( sdnbHeader       * aIndexHeader,
                             UChar            * aKeyPtr );

    /* Proj-1872 Disk index 저장 구조 최적화
     * keyValue를 String으로 변환하는 함수. Dump에서 사용한다. */
    static IDE_RC columnValue2String ( sdnbColumn       * aIndexColumn,
                                       UChar              aTargetColLenInfo,
                                       UChar            * aKeyPtr,
                                       UChar            * aText,
                                       UInt             * aTextLen,
                                       IDE_RC           * aRet );

    /* Proj-1872 Disk index 저장 구조 최적화
     * 키 길이를 계산하기 위해 사용하는 ColLenInfoList를 생성한다. */
    static void   makeColLenInfoList ( const sdnbColumn     * aColumns,
                                       const sdnbColumn     * aColumnFence,
                                       sdnbColLenInfoList   * aColLenInfoList );
                               
    static IDE_RC setIndexMetaInfo( idvSQL *          aStatistics,
                                    sdnbHeader *      aIndex,
                                    sdnbStatistic *   aIndexStat,
                                    sdrMtx *          aMtx,
                                    scPageID *        aRootPID,
                                    scPageID *        aMinPID,
                                    scPageID *        aMaxPID,
                                    scPageID *        aFreeNodeHead,
                                    ULong *           aFreeNodeCnt,
                                    idBool *          aIsCreatedWithLogging,
                                    idBool *          aIsConsistent,
                                    idBool *          aIsCreatedWithForce,
                                    smLSN *           aNologgingCompletionLSN );
    
    static IDE_RC backupRuntimeHeader( sdrMtx     * aMtx,
                                       sdnbHeader * aIndex );
    static IDE_RC restoreRuntimeHeader( void * aIndex );

    static IDE_RC setIndexEmptyNodeInfo( idvSQL *          aStatistics,
                                         sdnbHeader *      aIndex,
                                         sdnbStatistic *   aIndexStat,
                                         sdrMtx *          aMtx,
                                         scPageID *        aEmptyNodeHead,
                                         scPageID *        aEmptyNodeTail );

    // PROJ-1628
    static idBool isLastChildLeaf(sdnbStack * aStack);
    static idBool isLastChildInternal(sdnbStack * aStack);
    static IDE_RC propagateKeyInternalNode(idvSQL *         aStatistics,
                                           sdnbStatistic *  aIndexStat,
                                           sdrMtx *         aMtx,
                                           idBool         * aMtxStart,
                                           smnIndexHeader * aPersIndex,
                                           sdnbHeader *     aIndex,
                                           sdnbStack *      aStack,
                                           sdnbKeyInfo *    aKeyInfo,
                                           UShort *         aKeyValueLen,
                                           UInt             aMode,
                                           sdpPhyPageHdr ** aNewChildPage,
                                           UInt           * aAllocPageCount );
    static IDE_RC processLeafFull(idvSQL           * aStatistics,
                                  sdnbStatistic    * aIndexStat,
                                  sdrMtx           * aMtx,
                                  idBool           * aMtxStart,
                                  smnIndexHeader   * aPersIndex,
                                  sdnbHeader       * aIndex,
                                  sdnbStack        * aStack,
                                  sdpPhyPageHdr    * aNode,
                                  sdnbKeyInfo      * aKeyInfo,
                                  UShort             aKeySize,
                                  SShort             aInsertSeq,
                                  sdpPhyPageHdr   ** aTargetNode,
                                  SShort           * aTargetSlotSeq,
                                  UInt             * aAllocPageCount,
                                  idBool             aIsDeleteLeafKey );
    
    static IDE_RC processInternalFull(idvSQL           * aStatistics,
                                      sdnbStatistic    * aIndexStat,
                                      sdrMtx           * aMtx,
                                      idBool           * aMtxStart,
                                      smnIndexHeader   * aPersIndex,
                                      sdnbHeader       * aIndex,
                                      sdnbStack        * aStack,
                                      sdnbKeyInfo      * aKeyInfo,
                                      UShort           * aKeySize,
                                      SShort             aKeyMapSeq,
                                      UInt               aMode,
                                      sdpPhyPageHdr    * aNode,
                                      sdpPhyPageHdr   ** aNewChildNode,
                                      UInt             * aAllocPageCount);
    
    static IDE_RC redistributeKeyLeaf(idvSQL           * aStatistics,
                                      sdnbStatistic    * aIndexStat,
                                      sdrMtx           * aMtx,
                                      idBool           * aMtxStart, 
                                      smnIndexHeader   * aPersIndex,
                                      sdnbHeader       * aIndex,
                                      sdnbStack        * aStack,
                                      sdpPhyPageHdr    * aNode,
                                      sdnbKeyInfo      * aKeyInfo,
                                      UShort             aKeySize,
                                      SShort             aInsertSeq,
                                      sdpPhyPageHdr   ** aTargetNode,
                                      SShort           * aTargetSlotSeq,
                                      idBool           * aCanRedistribute,
                                      UInt             * aAllocPageCount);
    
    static IDE_RC redistributeKeyInternal(idvSQL *         aStatistics,
                                          sdnbStatistic *  aIndexStat,
                                          sdrMtx *         aMtx,
                                          idBool *         aMtxStart,
                                          smnIndexHeader * aPersIndex,
                                          sdnbHeader *     aIndex,
                                          sdnbStack *      aStack,
                                          sdnbKeyInfo *    aKeyInfo,
                                          UShort *         aKeyValueLen,
                                          SShort           aKeyMapSeq,
                                          UInt             aMode,
                                          idBool *         aCanRedistribute,
                                          sdpPhyPageHdr  * aNode,
                                          sdpPhyPageHdr ** aNewChildPage,
                                          UInt           * aAllocPageCount);
    static IDE_RC makeKeyArray(sdnbHeader *       aIndex,
                               sdnbStack *        aStack,
                               UShort *           aKeySize,
                               SShort             aInsertSeq,
                               UInt               aMode,
                               idBool             aIsLeaf,
                               sdpPhyPageHdr *    aNode,
                               sdpPhyPageHdr *    aNextNode,
                               SShort *           aAllKeyCount,
                               sdnbKeyArray **    aKeyArray);
    static IDE_RC findRedistributeKeyPoint(sdnbHeader *       aIndex,
                                           sdnbStack *        aStack,
                                           UShort *           aKeySize,
                                           SShort             aInsertSeq,
                                           UInt               aMode,
                                           sdpPhyPageHdr *    aNode,
                                           sdpPhyPageHdr *    aNextNode,
                                           idBool             aIsLeaf,
                                           SShort *           aPoint,
                                           idBool *           aCanRedistribute);
    static IDE_RC splitInternalNode(idvSQL *         aStatistics,
                                    sdnbStatistic *  aIndexStat,
                                    sdrMtx *         aMtx,
                                    idBool *         aMtxStart,
                                    smnIndexHeader * aPersIndex,
                                    sdnbHeader *     aIndex,
                                    sdnbStack *      aStack,
                                    sdnbKeyInfo *    aKeyInfo,
                                    UShort *         aKeyValueLen,
                                    UInt             aMode,
                                    UShort           aPCTFree,
                                    sdpPhyPageHdr *  aPage,
                                    sdpPhyPageHdr ** aNewChildPage,
                                    UInt           * aAllocPageCount);

    static IDE_RC canAllocLeafKey (sdrMtx         * aMtx,
                                   sdnbHeader     * aIndex,
                                   sdpPhyPageHdr  * aPageHdr,
                                   UInt             aSaveSize,
                                   idBool           aIsPessimistic,
                                   SShort         * aKeyMapSeq);

    static IDE_RC canAllocInternalKey (sdrMtx *         aMtx,
                                       sdnbHeader *     aIndex,
                                       sdpPhyPageHdr *  aPageHdr,
                                       UInt             aSaveSize,
                                       idBool           aExecCompact,
                                       idBool           aIsLogging);

    static IDE_RC getKeyInfoFromPIDAndKey( idvSQL *         aStatistics,
                                           sdnbStatistic *  aIndexStat,
                                           sdnbHeader *     aIndex,
                                           scPageID         aPID,
                                           UShort           aSeq,
                                           idBool           aIsLeaf,
                                           sdnbKeyInfo *    aKeyInfo,
                                           UShort *         aKeyValueLen,
                                           scPageID *       aRightChild,
                                           UChar *          aKeyValueBuf );
    static IDE_RC getKeyInfoFromSlot(sdnbHeader *    aIndex,
                                     sdpPhyPageHdr * aPage,
                                     UShort          aSeq,
                                     idBool          aIsLeaf,
                                     sdnbKeyInfo *   aKeyInfo,
                                     UShort *        aKeyValueLen,
                                     scPageID *      aRightChild);
    static UInt calcPageFreeSize( sdpPhyPageHdr * aPageHdr );
    static IDE_RC writeLogFreeKeys(sdrMtx        *aMtx,
                                   UChar         *aPagePtr,
                                   sdnbHeader    *aIndex,
                                   UShort         aFromSeq,
                                   UShort         aToSeq);

    static IDE_RC adjustKeyPosition( sdpPhyPageHdr  * aPage,
                                     SShort         * aKeyPosition );

    /* BUG-30639 Disk Index에서 Internal Node를
     * Min/Max Node라고 인식하여 사망합니다. */
    static IDE_RC rebuildMinStat( idvSQL         * aStatistics,
                                  void           * aTrans,
                                  smnIndexHeader * aPersIndex,
                                  sdnbHeader     * aIndex );

    static IDE_RC rebuildMaxStat( idvSQL         * aStatistics,
                                  void           * aTrans,
                                  smnIndexHeader * aPersIndex,
                                  sdnbHeader     * aIndex );

    static IDE_RC adjustMinStat( idvSQL         * aStatistics,
                                 sdnbStatistic  * aIndexStat,
                                 smnIndexHeader * aPersIndex,
                                 sdnbHeader     * aIndex,
                                 sdrMtx         * aMtx,
                                 sdpPhyPageHdr  * aOrgNode,
                                 sdpPhyPageHdr  * aNewNode,
                                 idBool           aIsLoggingMeta,
                                 UInt             aStatFlag );
    
    static IDE_RC adjustMaxStat( idvSQL         * aStatistics,
                                 sdnbStatistic  * aIndexStat,
                                 smnIndexHeader * aPersIndex,
                                 sdnbHeader     * aIndex,
                                 sdrMtx         * aMtx,
                                 sdpPhyPageHdr  * aOrgNode,
                                 sdpPhyPageHdr  * aNewNode,
                                 idBool           aIsLoggingMeta,
                                 UInt             aStatFlag );
    
    static IDE_RC adjustMaxNode( idvSQL        * aStatistics,
                                 sdnbStatistic * aIndexStat,
                                 sdnbHeader    * aIndex,
                                 sdrMtx        * aMtx,
                                 sdnbKeyInfo   * aKeyInfo,
                                 sdpPhyPageHdr * aOrgNode,
                                 sdpPhyPageHdr * aNewNode,
                                 sdpPhyPageHdr * aTargetNode,
                                 SShort          aTargetSlotSeq );
    
    static IDE_RC adjustNumDistStat( smnIndexHeader * aPersIndex,
                                     sdnbHeader     * aIndex,
                                     sdrMtx         * aMtx,
                                     sdpPhyPageHdr  * aLeafNode,
                                     SShort           aSlotSeq,
                                     SShort           aAddCount );

    // BUG-32313: The values of DRDB index cardinality converge on 0
    static IDE_RC adjustNumDistStatByDistributeKeys( smnIndexHeader * aPersIndex,
                                                     sdnbHeader     * aIndex,
                                                     sdrMtx         * aMtx,
                                                     sdnbKeyInfo    * aPropagateKeyInfo,
                                                     sdpPhyPageHdr  * aLeafNode,
                                                     UShort           aMoveStartSeq,
                                                     UInt             aMode );

    /* PROJ-1872 Disk index 저장 구조 최적화
     * Column을 분석하여, Column Value의 길이와 Column Header의 길이
     * Column의 길이, Column Value의 포인터를 반환한다 */
    static UShort getColumnLength( UChar        aTargetColLenInfo,
                                   UChar       *aColumnPtr,
                                   UInt        *aColumnHeaderLen,
                                   UInt        *aColumnValueLen,
                                   const void **aColumnValuePtr);

    /* PROJ-1872 Disk index 저장 구조 최적화
     * Index Key의 한 칼럼 값을 smiValue형태로 변환한다 */
    static UShort column2SmiValue( UChar             * aTargetColLenInfo,
                                   UChar             * aColumnPtr,
                                   UInt              * aLength,
                                   const void       ** aValue );

    /* PROJ-1872 Disk index 저장 구조 최적화
     * getKeyValueLength를 이용해 Key의 크기를 알아낸다 */
    static UShort getKeyLength( sdnbColLenInfoList * aColLenInfoList,
                                UChar              * aKey,
                                idBool               aIsLeaf );

    /* PROJ-1872 Disk index 저장 구조 최적화
     * keyValue의 크기를 알아낸다 */
    static UShort getKeyValueLength( sdnbColLenInfoList * aColLenInfoList,
                                     UChar              * aKeyValue);

    //Compare등을 위해 KeyValue를 smiValueList의 형태로 변경한다.
    static void makeSmiValueListFromKeyValue( sdnbColLenInfoList * aColLenInfoList,
                                              UChar              * aKey,
                                              smiValue           * aSmiValue );
    
    /* PROJ-1872 Disk index 저장 구조 최적화
     * 제대로된 FetchColumnList가 왔는지 확인합니다.. */
    static IDE_RC checkFetchColumnList(
                                    sdnbHeader         * aIndex,
                                    smiFetchColumnList * aFetchColumnList,
                                    idBool             * aFullIndexScan );

    /* PROJ-1872 Disk index 저장 구조 최적화
     * Fetch시, RowFilter 및 CursorLevelVisibility가 가능한 VRow를 생성합니다. */
    static IDE_RC makeVRowFromRow( idvSQL             * aStatistics,
                                   sdrMtx             * aMtx,
                                   sdrSavePoint       * aSP,
                                   void               * aTrans,
                                   void               * aTable,
                                   scSpaceID            aTableSpaceID,
                                   const UChar        * aRow,
                                   sdbPageReadMode      aPageReadMode,
                                   smiFetchColumnList * aFetchColumnList,
                                   smFetchVersion       aFetchVersion,
                                   sdRID                aTssRID,
                                   const smSCN        * aSCN,
                                   const smSCN        * aInfiniteSCN,
                                   UChar              * aDestBuf,
                                   idBool             * aIsRowDeleted,
                                   idBool             * aIsPageLatchReleased );

    /* PROJ-1872 Disk index 저장 구조 최적화
     * mFetchColumnListToMakeKeyValue를 이용해 Row로부터 KeyValue를 만들어냅니다. */
    static IDE_RC makeKeyValueFromRow(
                           idvSQL                 * aStatistics,
                           sdrMtx                 * aMtx,
                           sdrSavePoint           * aSP,
                           void                   * aTrans,
                           void                   * aTableHeader,
                           const smnIndexHeader   * aIndex,
                           const UChar            * aRow,
                           sdbPageReadMode          aPageReadMode,
                           scSpaceID                aTableSpaceID,
                           smFetchVersion           aFetchVersion,
                           sdRID                    aTssRID,
                           const smSCN            * aSCN,
                           const smSCN            * aInfiniteSCN,
                           UChar                  * aDestBuf,
                           idBool                 * aIsRowDeleted,
                           idBool                 * aIsPageLatchReleased);

    static IDE_RC makeKeyValueFromSmiValueList(
                           const smnIndexHeader   * aIndex,
                           const smiValue         * aSmiValueList,
                           UChar                  * aDestBuf );

    static IDE_RC makeSmiValueListInFetch(
                           const smiColumn        * aIndexColumn,
                           UInt                     aCopyOffset,
                           const smiValue         * aColumnValue,
                           void                   * aIndexInfo );

    static IDE_RC findSplitPoint( sdnbHeader    * aIndex,
                                  sdpPhyPageHdr * aNode,
                                  UInt            aHeight,
                                  SShort          aNewSlotPos,
                                  UShort          aNewSlotSize,
                                  UShort          aPCTFree,
                                  UInt            aMode,
                                  idBool          aIsDeleteLeafKey,
                                  UShort        * aSplitPoint);
    
    static IDE_RC insertKeyIntoLeafNode(idvSQL         * aStatistics,
                                        sdnbStatistic  * aIndexStat,
                                        sdrMtx         * aMtx,
                                        smnIndexHeader * aPersIndex,
                                        sdnbHeader     * aIndex,
                                        sdpPhyPageHdr  * aLeafPage,
                                        SShort         * aKeyMapSeq,
                                        sdnbKeyInfo    * aKeyInfo,
                                        idBool           aIsPessimistic,
                                        idBool         * aIsSuccess );
    
    static IDE_RC deleteKeyFromLeafNode(idvSQL        * aStatistics,
                                        sdnbStatistic * aIndexStat,
                                        sdrMtx        * aMtx,
                                        sdnbHeader    * aIndex,
                                        sdpPhyPageHdr * aLeafPage,
                                        SShort        * aLeafKeySeq ,
                                        idBool          aIsPessimistic,
                                        idBool        * aIsSuccess );
    
    static void copyKey( UChar *      aSrcKey,
                         UChar *      aDstKey,
                         UShort       aKeySize );
    
    static IDE_RC compactPage( sdnbHeader *    aIndex,
                               sdrMtx *        aMtx,
                               sdpPhyPageHdr * aPage,
                               idBool          aIsLogging );
    
    static IDE_RC compactPage( sdrMtx        * aMtx,
                               sdpPhyPageHdr * aPage,
                               UChar         * aCallbackContext );
    
    static IDE_RC compactPageLeaf( sdnbHeader         * aIndex,
                                   sdrMtx             * aMtx,
                                   sdpPhyPageHdr      * aPage,
                                   sdnbColLenInfoList * aColLenInfoList);

    static IDE_RC compactPageInternal( sdnbHeader         * aIndex,
                                       sdpPhyPageHdr      * aPage,
                                       sdnbColLenInfoList * aColLenInfoList);

    static IDE_RC insertLKey ( sdrMtx          * aMtx,
                               sdnbHeader      * aIndex,
                               sdpPhyPageHdr   * aNode,
                               SShort            aSlotSeq,
                               UChar             aCTSlotNum,
                               UChar             aTxBoundType,
                               sdnbKeyInfo     * aKeyInfo,
                               UShort            aKeyValueSize,
                               idBool            aIsLoggableSlot,
                               scOffset        * aKeyOffset );
    
    static IDE_RC makeNewRootNode( idvSQL *          aStatistics,
                                   sdnbStatistic *   aIndexStat,
                                   sdrMtx *          aMtx,
                                   idBool *          aMtxStart, 
                                   smnIndexHeader  * aPersIndex,
                                   sdnbHeader *      aIndex,
                                   sdnbStack *       aStack,
                                   sdnbKeyInfo *     aKeyInfo,
                                   UShort            aKeyValueLength,
                                   sdpPhyPageHdr **  aNewChildPage,
                                   UInt            * aAllocPageCount);

    static IDE_RC moveSlots( sdrMtx *          aMtx,
                             sdnbHeader *      aIndex,
                             sdpPhyPageHdr *   aSrcNode,
                             SInt              aHeight,
                             SShort            aFromSeq,
                             SShort            aToSeq,
                             sdpPhyPageHdr *   aDstNode );
    
    static IDE_RC insertIKey( idvSQL          * aStatistics,
                              sdrMtx          * aMtx,
                              sdnbHeader      * aIndex,
                              sdpPhyPageHdr   * aNode,
                              SShort            aSlotSeq,
                              sdnbKeyInfo     * aKeyInfo,
                              UShort            aKeyValueLen,
                              scPageID          aRightChildPID,
                              idBool            aIsNeedLogging );

    static IDE_RC deleteIKey( idvSQL *          aStatistics,
                              sdrMtx *          aMtx,
                              sdnbHeader *      aIndex,
                              sdpPhyPageHdr *   aNode,
                              SShort            aSlotSeq,
                              idBool            aIsNeedLogging,
                              scPageID          aChildPID);
    
    static IDE_RC updateAndInsertIKey( idvSQL *          aStatistics,
                                       sdrMtx *          aMtx,
                                       sdnbHeader *      aIndex,
                                       sdpPhyPageHdr *   aNode,
                                       SShort            aSlotSeq,
                                       sdnbKeyInfo *     aKeyInfo,
                                       UShort *          aKeyValueLen,
                                       scPageID          aRighChildPID,
                                       UInt              aMode,
                                       idBool            aIsNeedLogging);

    static IDE_RC splitLeafNode( idvSQL           * aStatistics,
                                 sdnbStatistic    * aIndexStat,
                                 sdrMtx           * aMtx,
                                 idBool           * aMtxStart,
                                 smnIndexHeader   * aPersIndex,
                                 sdnbHeader       * aIndex,
                                 sdnbStack        * aStack,
                                 sdpPhyPageHdr    * aNode,
                                 sdnbKeyInfo      * aKeyInfo,
                                 UShort             aKeySize,
                                 SShort             aInsertSeq,
                                 UShort             aPCTFree,
                                 sdpPhyPageHdr   ** aTargetNode,
                                 SShort           * aTargetSlotSeq,
                                 UInt             * aAllocPageCount,
                                 idBool             aIsDeleteLeafKey );

    static IDE_RC initializeNodeHdr(sdrMtx *         aMtx,
                                    sdpPhyPageHdr *  aPage,
                                    scPageID         aLeftmostChild,
                                    UShort           aHeight,
                                    idBool           aIsLogging);
    
    static IDE_RC checkUniqueness(idvSQL           * aStatistics,
                                  void             * aTable,
                                  void             * aIndex,
                                  sdnbStatistic    * aIndexStat,
                                  sdrMtx           * aMtx,
                                  sdnbHeader       * aIndexHeader,
                                  smSCN              aStmtViewSCN,
                                  smSCN              aInfiniteSCN,
                                  sdnbKeyInfo      * aKeyInfo,
                                  sdpPhyPageHdr    * aStartNode,
                                  SShort             aSlotSeq,
                                  sdcUpdateState   * aRetFlag,
                                  smTID            * aWaitTID);

    // BUG-15553
    // 유니크 검사를 위해 같은 값(value)를 갖는 최소 키를 찾기 위해
    // 역방향으로 노드를 순회한다.
    static IDE_RC backwardScanNode( idvSQL         * aStatistics,
                                    sdnbStatistic  * aIndexStat,
                                    sdrMtx         * aMtx,
                                    sdnbHeader     * aIndex,
                                    scPageID         aStartPID,
                                    sdnbKeyInfo    * aKeyInfo,
                                    sdpPhyPageHdr ** aDestNode );

    // BUG-16702
    // 유니크 검사를 위해 같은 값(value)를 갖는 첫번째 슬롯을 찾기 위해
    // 역방향으로 슬롯을 순회한다.
    static IDE_RC backwardScanSlot( sdnbHeader     * aIndex,
                                    sdnbStatistic  * aIndexStat,                
                                    sdnbKeyInfo    * aKeyInfo,
                                    sdpPhyPageHdr  * aLeafNode,
                                    SShort           aStartSlotSeq,
                                    SShort         * aDestSlotSeq );
    
    // BUG-15106
    // unique index 일때, insert position을 찾음
    static IDE_RC findInsertPos4Unique( idvSQL*          aStatistics,
                                        void           * aTrans,
                                        void           * aTable,
                                        void           * aIndex,
                                        sdnbStatistic  * aIndexStat,
                                        smSCN            aStmtViewSCN,
                                        smSCN            aInfiniteSCN,
                                        sdrMtx         * aMtx,
                                        idBool         * aMtxStart,
                                        sdnbStack      * aStack,
                                        sdnbHeader     * aIndexHeader,
                                        sdnbKeyInfo    * aKeyInfo,
                                        idBool         * aIsPessimistic,
                                        sdpPhyPageHdr ** aLeafNode,
                                        SShort         * aLeafKeySeq ,
                                        idBool         * aIsSameKey,
                                        idBool         * aIsRetry,
                                        idBool         * aIsRetraverse,
                                        ULong          * aIdxSmoNo,
                                        ULong            aInsertWaitTime,
                                        idBool           aForbiddenToRetry,
                                        SLong          * aTotalTraverseLength );

    // BUG-15106
    // non unique index 일때, insert position을 찾음
    static IDE_RC findInsertPos4NonUnique( idvSQL*          aStatistics,
                                           sdnbStatistic  * aIndexStat,
                                           sdrMtx         * aMtx,
                                           idBool         * aMtxStart,
                                           sdnbStack      * aStack,
                                           sdnbHeader     * aIndexHeader,
                                           sdnbKeyInfo    * aKeyInfo,
                                           idBool         * aIsPessimistic,
                                           sdpPhyPageHdr ** aLeafNode,
                                           SShort         * aLeafKeySeq ,
                                           idBool         * aIsSameKey,
                                           idBool         * aIsRetry,
                                           SLong          * aTotalTraverseLength );
    
    static IDE_RC insertKeyUnique( idvSQL          * aStatistics,
                                   void            * aTrans,
                                   void            * aTable,
                                   void            * aIndex,
                                   smSCN             aInfiniteSCN,
                                   SChar           * aKey,
                                   SChar           * aNull,
                                   idBool            aUniqueCheck,
                                   smSCN             aStmtViewSCN,
                                   void            * aRowSID,
                                   SChar          ** aExistUniqueRow,
                                   ULong             aInsertWaitTime,
                                   idBool            aForbiddenToRetry );

    static IDE_RC insertKey( idvSQL          * aStatistics,
                             void            * aTrans,
                             void            * aTable,
                             void            * aIndex,
                             smSCN             aInfiniteSCN,
                             SChar           * aKeyValue,
                             SChar           * aNull,
                             idBool            aUniqueCheck,
                             smSCN             aStmtViewSCN,
                             void            * aRowSID,
                             SChar          ** aExistUniqueRow,
                             ULong             aInsertWaitTime,
                             idBool            aForbiddenToRetry );

    static IDE_RC validatePath( idvSQL*          aStatistics,
                                scSpaceID        aIndexTSID,
                                sdnbStatistic *  aIndexStat,
                                sdnbStack *      aStack,
                                ULong            aSmoNo );

    static IDE_RC deleteInternalKey (idvSQL        * aStatistics,
                                     sdnbHeader    * aIndex,
                                     sdnbStatistic * aIndexStat,
                                     scPageID        aSegPID,
                                     sdrMtx        * aMtx,
                                     sdnbStack     * aStack,
                                     UInt          * aFreePageCount );
    
    static IDE_RC unlinkNode( sdrMtx *        aMtx,
                              sdnbHeader *    aIndex,
                              sdpPhyPageHdr * aNode,
                              ULong           aSmoNo );
    
    static IDE_RC unsetRootNode( idvSQL *        aStatistics,
                                 sdrMtx *        aMtx,
                                 sdnbHeader *    aIndex,
                                 sdnbStatistic * aIndexStat );
    
    static IDE_RC deleteKey( idvSQL       * aStatistics,
                             void         * aTrans,
                             void         * aIndex,
                             SChar        * aKey,
                             smiIterator  * /*aIterator*/,
                             sdRID          aRowRID );

    static IDE_RC linkEmptyNode( idvSQL        * aStatistics,
                                 sdnbStatistic * aIndexStat,
                                 sdnbHeader    * aIndex,
                                 sdrMtx        * aMtx,
                                 sdpPhyPageHdr * aNode,
                                 idBool        * aIsSuccess );
    
    static IDE_RC unlinkEmptyNode( idvSQL        * aStatistics,
                                   sdnbStatistic * aIndexStat,
                                   sdnbHeader    * aIndex,
                                   sdrMtx        * aMtx,
                                   sdpPhyPageHdr * aNode,
                                   UChar           aNodeState );
    
    static IDE_RC aging( idvSQL         * aStatistics,
                         void           * aTrans,
                         smcTableHeader * aHeader,
                         smnIndexHeader * aIndex );

    static IDE_RC NA( void ); 

    static IDE_RC beforeFirstInternal( sdnbIterator* a_pIterator );
    
    static IDE_RC beforeFirst( sdnbIterator*       aIterator,
                               const smSeekFunc** aSeekFunc );

    static IDE_RC findFirst( sdnbIterator * aIterator );
    
    static IDE_RC findFirstInternalKey( const smiCallBack * aCallBack,
                                        sdnbHeader        * aIndex,
                                        sdpPhyPageHdr     * aNode,
                                        sdnbStatistic     * aIndexStat,
                                        scPageID          * aChildNode,
                                        ULong               aIndexSmoNo,
                                        ULong             * aNodeSmoNo,
                                        idBool            * aRetry );

    static IDE_RC findFirstLeafKey( const smiCallBack  * aCallBack,
                                    sdnbHeader         * aIndex,
                                    UChar              * aPagePtr,
                                    SShort               aMinimum,
                                    SShort               aMaximum,
                                    SShort             * aSlot );

    static IDE_RC makeRowCacheForward( sdnbIterator   * aIterator,
                                       UChar          * aPagePtr,
                                       SShort           aStartSlotSeq );
    
    static IDE_RC findSlotAtALeafNode( sdpPhyPageHdr * aNode,
                                       sdnbHeader    * aIndex,
                                       sdnbKeyInfo   * aKeyInfo,
                                       SShort        * aKeyMapSeq,
                                       idBool        * aFound );
    
    static IDE_RC makeNextRowCacheForward(sdnbIterator * aIterator,
                                          sdnbHeader   * aIndex );

    static IDE_RC makeNextRowCacheBackward(sdnbIterator * aIterator,
                                           sdnbHeader   * aIndex );
    
    static IDE_RC afterLast( sdnbIterator*       aIterator,
                             const smSeekFunc** aSeekFunc );
    
    static IDE_RC afterLastInternal( sdnbIterator* aIterator );
    
    static IDE_RC findLast( sdnbIterator * aIterator );

    static IDE_RC findLastInternalKey ( const smiCallBack * aCallBack,
                                        sdnbHeader *        aIndex,
                                        sdpPhyPageHdr *     aNode,
                                        sdnbStatistic *     aIndexStat, 
                                        scPageID *          aChildNode,
                                        ULong               aIndexSmoNo,
                                        ULong *             aNodeSmoNo,
                                        idBool *            aRetry );
    
    static IDE_RC findLastLeafKey ( const smiCallBack* aCallBack,
                                    sdnbHeader *       aIndex,
                                    UChar *            aPagePtr,
                                    SShort             aMinimum,
                                    SShort             aMaximum,
                                    SShort*            aSlot );
    
    static IDE_RC makeRowCacheBackward( sdnbIterator *  aIterator,
                                        UChar  *        aPagePtr,
                                        SShort          aStartSlotSeq );
    
    static IDE_RC beforeFirstW( sdnbIterator*       aIterator,
                                const smSeekFunc** aSeekFunc );

    static IDE_RC afterLastW( sdnbIterator*       aIterator,
                              const smSeekFunc** aSeekFunc );
    
    static IDE_RC beforeFirstRR( sdnbIterator*       aIterator,
                                 const smSeekFunc** aSeekFunc );

    static IDE_RC afterLastRR( sdnbIterator*       aIterator,
                               const smSeekFunc** aSeekFunc );
    
    static IDE_RC restoreCursorPosition( const smnIndexHeader   *aIndexHeader,
                                         sdrMtx                 *aMtx,
                                         sdnbIterator           *aIterator,
                                         idBool                  aSmoOccurred,
                                         sdpPhyPageHdr         **aNode,
                                         SShort                 *aSlotSeq);
    
    static IDE_RC isVisibleKey( void          * aIndex,
                                sdnbStatistic * aIndexStat,
                                UChar         * aRow,
                                UChar         * aKey,
                                idBool        * aIsVisible );
    
    static IDE_RC fetchNext( sdnbIterator* a_pIterator,
                             const void**  aRow );
    
    static IDE_RC fetchPrev( sdnbIterator* aIterator,
                             const void**  aRow );
    
    static IDE_RC lockAllRows4RR( sdnbIterator* aIterator );

    static IDE_RC retraverse(sdnbIterator* aIterator,
                             const void**  /*aRow*/);

    static IDE_RC getPosition( sdnbIterator *     aIterator,
                               smiCursorPosInfo * aPosInfo );
    
    static IDE_RC setPosition( sdnbIterator *     aIterator,
                               smiCursorPosInfo * aPosInfo );

    static SInt checkSMO( sdnbStatistic * aIndexStat,
                          sdpPhyPageHdr * aNode,
                          ULong           aIndexSmoNo,
                          ULong *         aNodeSmoNo );

    static IDE_RC allocIterator( void ** aIteratorMem );
    
    static IDE_RC freeIterator( void * aIteratorMem);

    static UShort getMaxKeySize();

    static IDE_RC verifyIndexIntegrity( idvSQL*  aStatistics,
                                        void  *  aIndex );
    
    static IDE_RC initMeta( UChar * aMetaPtr,
                            UInt    aBuildFlag,
                            void  * aMtx );
    
    static IDE_RC buildMeta( idvSQL*       aStatistics,
                             void *        aTrans,
                             void *        aIndex );

    static void getSmoNo( void * aIndex, ULong * aSmoNo );
    
    static void increaseSmoNo( sdnbHeader*    aIndex );

    // To Fix BUG-15670
    // Key Map Sequence No를 이용하여 Key Column의 실제 Value Ptr 획득한 후
    // 인덱스 헤더에 MinMax 통계치를 설정함
    static IDE_RC setMinMaxValue( sdnbHeader     * aIndex,
                                  UChar          * aIndexPageNode,
                                  SShort           aKeyMapSeq,
                                  UChar          * aTargetPtr );

    static UInt getMinimumKeyValueSize( smnIndexHeader *aIndexHeader );
    
    static UInt getInsertableMaxKeyCnt( UInt sKeyValueSize );

    static IDE_RC lockRow( sdnbIterator* aIterator );

    // To fix BUG-21925
    static IDE_RC setConsistent( smnIndexHeader * aIndex,
                                 idBool           aIsConsistent );

    static IDE_RC selfAging( idvSQL        * aStatistics,
                             sdnbHeader    * aIndex,
                             sdrMtx        * aMtx,
                             sdpPhyPageHdr * aPage,
                             UChar         * aAgedCount );
    
    static IDE_RC allocCTS( idvSQL             * aStatistics,
                            sdnbHeader         * aIndex,
                            sdrMtx             * aMtx,
                            sdpPhyPageHdr      * aPage,
                            UChar              * aCTSlotNum,
                            sdnCallbackFuncs   * aCallbackFunc,
                            UChar              * aContext,
                            SShort             * aKeyMapSeq );
    
    static IDE_RC softKeyStamping( sdnbHeader    * aIndex,
                                   sdrMtx        * aMtx,
                                   sdpPhyPageHdr * aPage,
                                   UChar           aCTSlotNum );

    static IDE_RC softKeyStamping( sdrMtx        * aMtx,
                                   sdpPhyPageHdr * aPage,
                                   UChar           aCTSlotNum,
                                   UChar         * aContext );

    static IDE_RC hardKeyStamping( idvSQL        * aStatistics,
                                   sdrMtx        * aMtx,
                                   sdpPhyPageHdr * aPage,
                                   UChar           aCTSlotNum,
                                   UChar         * aContext,
                                   idBool        * aSuccess );
    
    static IDE_RC hardKeyStamping( idvSQL        * aStatistics,
                                   sdnbHeader    * aIndex,
                                   sdrMtx        * aMtx,
                                   sdpPhyPageHdr * aPage,
                                   UChar           aCTSlotNum,
                                   idBool        * aSuccess );
    
    static IDE_RC insertKeyRollback( idvSQL  * aStatistics,
                                     void    * aMtx,
                                     void    * aIndex,
                                     SChar   * aKeyValue,
                                     sdSID     aRowSID,
                                     UChar   * aRollbackContext,
                                     idBool    aIsDupKey );
    
    static IDE_RC deleteKeyRollback( idvSQL  * aStatistics,
                                     void    * aMtx,
                                     void    * aIndex,
                                     SChar   * aKeyValue,
                                     sdSID     aRowSID,
                                     UChar   * aRollbackContext );
    
    static IDE_RC isDuplicateKey( sdnbStatistic * aIndexStat,
                                  sdnbHeader    * aIndex,
                                  sdpPhyPageHdr * aLeafPage,
                                  SShort          aKeyMapSeq,
                                  sdnbKeyInfo   * aKeyInfo,
                                  idBool        * aIsDupKey );

    static IDE_RC removeDuplicateKey( idvSQL        * aStatistics,
                                      sdrMtx        * aMtx,
                                      sdnbHeader    * aIndex,
                                      sdpPhyPageHdr * aLeafPage,
                                      SShort          aKeyMapSeq,
                                      idBool        * aIsSuccess );

    static IDE_RC makeDuplicateKey( sdrMtx        * aMtx,
                                    sdnbHeader    * aIndex,
                                    UChar           aCTSlotNum,
                                    sdpPhyPageHdr * aLeafPage,
                                    SShort          aKeyMapSeq,
                                    UChar           aTxBoundType );
    
    static idBool isMyTransaction( void   * aTrans,
                                   smSCN    aBeginSCN,
                                   sdSID    aTSSlotSID );
    
    static idBool isAgableTBK( smSCN    aCommitSCN );
    
    static IDE_RC getCommitSCNFromTBK( idvSQL        * aStatistics,
                                       void          * aTrans,
                                       sdpPhyPageHdr * aPage,
                                       sdnbLKeyEx    * aLeafKeyEx,
                                       idBool          aIsLimit,
                                       smSCN           aStmtViewSCN,
                                       smSCN         * aCommitSCN );
    
    static IDE_RC agingAllTBK( idvSQL        * aStatistics,
                               sdnbHeader    * aIndex,
                               sdrMtx        * aMtx,
                               sdpPhyPageHdr * aPage,
                               idBool        * aIsSuccess );
    
    static IDE_RC cursorLevelVisibility( void          * aIndex,
                                         sdnbStatistic * aIndexStat,
                                         UChar         * aVRow,
                                         UChar         * aKey,
                                         idBool        * aIsVisible );
    
    static IDE_RC transLevelVisibility( idvSQL         * aStatistics,
                                        void           * aTrans, 
                                        sdnbHeader     * aIndex,
                                        UChar          * aNode,
                                        UChar          * aLeafKey ,
                                        smSCN          * aStmtViewSCN,
                                        sdnbVisibility * aVisibility );
    
    static IDE_RC uniqueVisibility( idvSQL         * aStatistics,
                                    void           * aTrans,
                                    sdnbHeader     * aIndex,
                                    UChar          * aNode,
                                    UChar          * aLeafKey,
                                    smSCN          * aStmtViewSCN,
                                    sdnbVisibility * aVisibility );
    
    static IDE_RC freeNode(idvSQL          * aStatistics,
                           void            * aTrans,
                           smnIndexHeader  * aPersIndex,
                           sdnbHeader      * aIndex,
                           scPageID          aPageID,
                           sdnbKeyInfo     * aKeyInfo,
                           sdnbStatistic   * aIndexStat,
                           UInt            * aFreePageCount );
    
    static IDE_RC nodeAging( idvSQL         * aStatistics,
                             void           * aTrans,
                             sdnbStatistic  * aIndexStat, 
                             smnIndexHeader * aPersIndex,
                             sdnbHeader     * aIndex,
                             UInt             aFreePageCount );

    static IDE_RC allocPage( idvSQL          * aStatistics,
                             sdnbStatistic   * aIndexStat,
                             sdnbHeader      * aIndex,
                             sdrMtx          * aMtx,
                             UChar          ** aNewPage );
    
    static IDE_RC freePage( idvSQL          * aStatistics,
                            sdnbStatistic   * aIndexStat,
                            sdnbHeader      * aIndex,
                            sdrMtx          * aMtx,
                            UChar           * aFreePage );
    
    static IDE_RC preparePages( idvSQL      * aStatistics,
                                sdnbHeader  * aIndex,
                                sdrMtx      * aMtx,
                                idBool      * aMtxStart,
                                UInt          aNeedPageCnt );
    
    static IDE_RC setFreeNodeInfo( idvSQL         * aStatistics,
                                   sdnbHeader     * aIndex,
                                   sdnbStatistic  * aIndexStat,
                                   sdrMtx         * aMtx,
                                   scPageID         aFreeNodeHead,
                                   ULong            aFreeNodeCnt );
    
    static IDE_RC insertLeafKeyWithTBT( sdrMtx               * aMtx,
                                        sdnbHeader           * aIndex,
                                        UChar                  aCTSlotNum,
                                        sdpPhyPageHdr        * aLeafPage,
                                        sdnbKeyInfo          * aKeyInfo,
                                        UShort                 aKeyValueSize,
                                        idBool                 aIsDupKey,
                                        SShort                 aKeyMapSeq );
    
    static IDE_RC insertLeafKeyWithTBK( sdrMtx               * aMtx,
                                        sdnbHeader           * aIndex,
                                        sdpPhyPageHdr        * aLeafPage,
                                        sdnbKeyInfo          * aKeyInfo,
                                        UShort                 aKeyValueSize,
                                        idBool                 aIsDupKey,
                                        SShort                 aKeyMapSeq );
    
    static IDE_RC deleteLeafKeyWithTBK( idvSQL        * aStatistics,
                                        sdrMtx        * aMtx,
                                        sdnbHeader    * aIndex,
                                        sdpPhyPageHdr * aLeafPage,
                                        SShort        * aLeafKeySeq ,
                                        idBool          aIsPessimistic,
                                        idBool        * aIsSuccess );
    
    static IDE_RC deleteLeafKeyWithTBT( sdrMtx               * aMtx,
                                        sdnbHeader           * aIndex,
                                        UChar                  aCTSlotNum,
                                        sdpPhyPageHdr        * aLeafPage,
                                        SShort                 aLeafKeySeq  );

    static inline idBool needToUpdateStat();
    

private:
    static iduMemPool mIteratorPool;

    static IDE_RC fetchRowCache( sdnbIterator  * aIterator,
                                 const void   ** aRow,
                                 idBool        * aNeedMoreCache );
    
    static IDE_RC verifyLeafKeyOrder( sdnbHeader    * aIndex,
                                      sdpPhyPageHdr * aNode,
                                      sdnbIKey      * aLeftParentKey,
                                      sdnbIKey      * aRightParentKey, 
                                      idBool        * aIsOK );
    
    static IDE_RC verifyInternalKeyOrder( sdnbHeader    * aIndex,
                                          sdpPhyPageHdr * aNode,
                                          sdnbIKey      * aLeftParentKey,
                                          sdnbIKey      * aRightParentKey,
                                          idBool        * aIsOK );

    static idBool verifyPrefixPos( sdnbHeader    * aIndex,
                                   sdpPhyPageHdr * aNode,
                                   scPageID        aNewChildPID,
                                   SShort          aKeyMapSeq,
                                   sdnbKeyInfo   * aKeyInfo );

    static IDE_RC isSameRowAndKey( sdnbHeader    * aIndex,
                                   sdnbStatistic * aIndexStat,
                                   sdnbKeyInfo   * aRowInfo,
                                   sdpPhyPageHdr * aLeafPage,
                                   SShort          aLeafKeySeq,
                                   idBool        * aIsSameKey );
    
    static void findEmptyNodes( sdrMtx        * aMtx,
                                sdnbHeader    * aIndex,
                                sdpPhyPageHdr * aStartPage,
                                scPageID      * aEmptyNodePID,
                                UInt          * aEmptyNodeCount );
    
    static IDE_RC linkEmptyNodes( idvSQL         * aStatistics,
                                  void           * aTrans,
                                  sdnbHeader     * aIndex,
                                  sdnbStatistic  * aIndexStat,
                                  scPageID       * aEmptyNodePID );
    
    static IDE_RC findTargetKeyForDupKey( sdrMtx         * aMtx,
                                          sdnbHeader     * aIndex,
                                          sdnbStatistic  * aIndexStat,
                                          sdnbKeyInfo    * aKeyInfo,
                                          sdpPhyPageHdr ** aTargetPage,
                                          SShort         * aTargetSlotSeq );
    
    static void findTargetKeyForDeleteKey( sdrMtx         * aMtx,
                                           sdnbHeader     * aIndex,
                                           sdpPhyPageHdr ** aTargetPage,
                                           SShort         * aTargetSlotSeq );

    // Key Map Sequence No를 이용하여 Key Pointer획득
    static IDE_RC getKeyPtr( UChar         * aIndexPageNode,
                             SShort          aKeyMapSeq,
                             UChar        ** aKeyPtr );
                             
    static IDE_RC getPage( idvSQL             *aStatistics,
                           sdnbPageStat       *aPageStat,
                           scSpaceID           aSpaceID,
                           scPageID            aPageID,
                           sdbLatchMode        aLatchMode,
                           sdbWaitMode         aWaitMode,
                           void               *aMtx,
                           UChar             **aRetPagePtr,
                           idBool             *aTrySuccess );
    
    static IDE_RC fixPage( idvSQL            * aStatistics,
                           sdnbPageStat      * aPageStat,
                           scSpaceID           aSpaceID,
                           scPageID            aPageID,
                           UChar            ** aRetPagePtr,
                           idBool            * aTrySuccess );

     static IDE_RC fixPageByRID( idvSQL            * aStatistics,
                                 sdnbPageStat      * aPageStat,
                                 scSpaceID           aSpaceID,
                                 sdRID               aRID,
                                 UChar            ** aRetPagePtr,
                                 idBool            * aTrySuccess );


    static IDE_RC unfixPage( idvSQL* aStatistics,
                             UChar * aPagePtr );
    
    static void initStack( sdnbStack *aStack );

public:
    /* PROJ-2162 RestartRiskReduction
     * 인덱스 Dump함수들을 한곳에 모아둠 */
    /* TASK-4007 [SM] PBT를 위한 기능 추가 - dumpddf정상화
     * 인덱스 페이지의 NodeHdr 및 MetaPage Dump*/
    static IDE_RC dump       ( UChar *sPage ,
                               SChar *aOutBuf ,
                               UInt   aOutSize );
    static IDE_RC dumpMeta   ( UChar *sPage ,
                               SChar *aOutBuf ,
                               UInt   aOutSize );
    static IDE_RC dumpNodeHdr( UChar *sPage ,
                               SChar *aOutBuf ,
                               UInt   aOutSize );
    
    // BUG-28711 SM PBT 보강
    static void dumpIndexNode( sdpPhyPageHdr * aNode );

    // BUG-30605: Deleted/Dead된 키도 Min/Max로 포함시킵니다
    static idBool isIgnoredKey4MinMaxStat( sdnbLKey   * aKey,
                                           sdnbColumn * aColumn );

    // BUG-32313: The values of DRDB index cardinality converge on 0
    static idBool isIgnoredKey4NumDistStat( sdnbHeader * aIdxHdr,
                                            sdnbLKey   * aKey );

    static idBool isPrimaryKey( sdnbHeader * aIdxHdr );

    static IDE_RC findSameValueKey( sdnbHeader    * aIndex,
                                    sdpPhyPageHdr * aLeafNode,
                                    SInt            aSlotSeq,
                                    sdnbKeyInfo   * aKeyInfo,
                                    UInt            aDirect,
                                    sdnbLKey     ** aFindSameKey );

    static IDE_RC gatherStat( idvSQL         * aStatistics,
                              void           * aTrans,
                              SFloat           aPercentage,
                              SInt             aDegree,
                              smcTableHeader * aHeader,
                              void           * aTotalTableArg,
                              smnIndexHeader * aIndex,
                              void           * aStats,
                              idBool           aDynamicMode );

    static IDE_RC gatherIndexHeight( idvSQL        * aStatistics,
                                     void          * aTrans,
                                     sdnbHeader    * aIndex,
                                     SLong         * aIndexHeight );

    static void gatherStatParallel( void   * aJob );

    static IDE_RC analyzeNode4GatherStat( sdnbStatArgument  * aStatArgument,
                                          sdnbHeader        * aIdxHdr,
                                          sdpPhyPageHdr     * aNode );

    /* BUG-31845 [sm-disk-index] Debugging information is needed for 
     * PBT when fail to check visibility using DRDB Index.
     * 검증용 Dump 코드 추가 */
    static void dumpHeadersAndIteratorToSMTrc( 
                                        smnIndexHeader * aCommonHeader,
                                        sdnbHeader     * aRuntimeHeader,
                                        sdnbIterator   * aIterator );

    static IDE_RC dumpRuntimeHeader( sdnbHeader * aHeader,
                                     SChar      * aOutBuf,
                                     UInt         aOutSize );

    static IDE_RC dumpIterator( sdnbIterator * aIterator,
                                SChar        * aOutBuf,
                                UInt           aOutSize );

    static IDE_RC dumpStack( sdnbStack    * aStack,
                             SChar        * aOutBuf,
                             UInt           aOutSize );

    static IDE_RC dumpKeyInfo( sdnbKeyInfo        * aKeyInfo,
                               sdnbColLenInfoList * aColLenInfoList,
                               SChar              * aOutBuf,
                               UInt                 aOutSize );

    // To fix BUG-16798
    static IDE_RC traverse4VerifyIndexIntegrity( 
                                        idvSQL     * aStatistics,
                                        sdnbHeader * aIndex,
                                        scPageID     aPageID,
                                        sdnbIKey   * aLeftParentKey,
                                        sdnbIKey   * aRightParentKey,
                                        scPageID   * aPrevLeafPID,
                                        scPageID   * aNextPIDofPrevLeafNode);

    static IDE_RC verifyLeafIntegrity( idvSQL     * aStatistics,
                                       sdnbHeader * aIndex,
                                       scPageID     aPageID,
                                       sdnbIKey   * aLeftParentKey,
                                       sdnbIKey   * aRightParentKey,
                                       scPageID   * aPrevLeafPID,
                                       scPageID   * aNextPIDofPrevLeafNode );

static IDE_RC allTBKStamping( idvSQL        * aStatistics,
                              sdnbHeader    * aIndex,
                              sdrMtx        * aMtx,
                              sdpPhyPageHdr * aPage,
                              UShort        * aStampingCount );

static IDE_RC checkTBKStamping( idvSQL        * aStatistics,
                                sdpPhyPageHdr * aPage,
                                sdnbLKey      * aLeafKey,
                                idBool        * aIsCreateCSCN,
                                idBool        * aIsLimitCSCN );

};

inline void sdnbBTree::initStack( sdnbStack *aStack )
{
    aStack->mIndexDepth = -1;
    aStack->mCurrentDepth = -1;
    aStack->mIsSameValueInSiblingNodes = ID_FALSE;
    aStack->mStack[0].mNextNode = SD_NULL_PID;
}

inline void sdnbBTree::dumpIndexNode( sdpPhyPageHdr *aNode )
{
    if( aNode == NULL )
    {
        ideLog::log( IDE_DUMP_0, "\
===================================================\n\
            PID : NULL\n");
    }
    else
    {
        ideLog::log( IDE_DUMP_0, "\
===================================================\n\
            PID : %u              \n",
                    aNode->mPageID );

        ideLog::logMem( IDE_DUMP_0, (UChar *)aNode, SD_PAGE_SIZE );
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnbBTree::isIgnoredKey4NumDistStat           *
 * ------------------------------------------------------------------*
 * BUG-32313: The values of DRDB index cardinality converge on 0     *
 *                                                                   *
 * 인덱스 통계정보 갱신시 무시될 키 여부를 반환한다. DELETED, DEAD   *
 * 상태 키들은 삭제된 키들이기 때문에 통계 정보 갱신시 무시된다.     *
 *                                                                   *
 * Min, Max 정보 갱신시 첫번째 키 컬럼이 Null 이면 무시된다. Min,    *
 * Max는 첫번째 컬럼만을 고려하기 때문이다.                          *
 *********************************************************************/
inline idBool sdnbBTree::isIgnoredKey4NumDistStat( sdnbHeader * aIdxHdr,
                                                   sdnbLKey   * aKey )
{
    idBool    sIsIgnored = ID_FALSE;
    UChar   * sKeyValue  = NULL;

    IDE_ASSERT( aKey != NULL );

    if( (SDNB_GET_STATE(aKey) == SDNB_KEY_DELETED) ||
        (SDNB_GET_STATE(aKey) == SDNB_KEY_DEAD) )
    {
        sIsIgnored = ID_TRUE;
    }

    if( sIsIgnored == ID_FALSE )
    {
        sKeyValue = SDNB_LKEY_KEY_PTR(aKey);
        
        if( isNullKey(aIdxHdr, sKeyValue) == ID_TRUE )
        {
            sIsIgnored = ID_TRUE;
        }
    }

    return sIsIgnored;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnbBTree::isIgnoredKey4MinMaxStat         *
 * ------------------------------------------------------------------*
 * BUG-30605 Deleted/Dead된 키도 Min/Max로 포함시킵니다.             *
 *                                                                   *
 * Min, Max 정보는 delete, dead 키도 포함하여 계산된다.              *
 *                                                                   *
 * Min, Max 정보 갱신시 첫번째 키 컬럼이 Null 이면 무시된다. Min,    *
 * Max는 첫번째 컬럼만을 고려하기 때문이다.                          *
 *********************************************************************/
inline idBool sdnbBTree::isIgnoredKey4MinMaxStat( sdnbLKey   * aKey,
                                                  sdnbColumn * aColumn )
{
    idBool    sIsIgnored = ID_FALSE;
    UChar   * sKeyValue  = NULL;

    IDE_ASSERT( aKey    != NULL );
    IDE_ASSERT( aColumn != NULL );

    sKeyValue = SDNB_LKEY_KEY_PTR(aKey);
        
    if( isNullColumn(aColumn, sKeyValue) == ID_TRUE )
    {
        sIsIgnored = ID_TRUE;
    }

    return sIsIgnored;
}

inline idBool sdnbBTree::isPrimaryKey( sdnbHeader * aIdxHdr )
{
    idBool sIsPrimaryKey = ID_FALSE;
    
    if( (aIdxHdr->mIsUnique  == ID_TRUE) &&
        (aIdxHdr->mIsNotNull == ID_TRUE) )
    {
        sIsPrimaryKey = ID_TRUE;
    }

    return sIsPrimaryKey;
}

#endif /* _O_SDNB_MODULE_H_ */
