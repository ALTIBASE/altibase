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
 * $Id: sdbBufferMgr.cpp 17358 2006-07-31 03:12:47Z bskim $
 **********************************************************************/

#ifndef _O_STNMR_MODULE_H_
# define _O_STNMR_MODULE_H_ 1

# include <smnDef.h>
# include <stnmrDef.h>

extern smnIndexModule stnmrModule;
extern smnIndexModule stnmdModule;

class stnmrRTree
{
public:
    
    static IDE_RC prepareIteratorMem( const smnIndexModule* aModule );

    static IDE_RC releaseIteratorMem( const smnIndexModule* aModule );

    static IDE_RC prepareFreeNodeMem( const smnIndexModule* aModule );

    static IDE_RC releaseFreeNodeMem( const smnIndexModule* aModule );

    static IDE_RC freeAllNodeList( idvSQL*         aStatistics,
                                   smnIndexHeader* aIndex,
                                   void*           aTrans ); 

    static IDE_RC freeNode( void* aNodes );
        
    static IDE_RC create( idvSQL*              aStatistics,
                          smcTableHeader*      aTable,
                          smnIndexHeader*      aIndex,
                          smiSegAttr*          /*aSegAttr*/,
                          smiSegStorageAttr*   /*aSegStoAttr*/,
                          smnInsertFunc*       aInsert,
                          smnIndexHeader**     aRebuildIndexHeader,
                          ULong                aSmoNo );

    static IDE_RC buildIndex( idvSQL*               aStatistics,
                              void*                 aTrans,
                              smcTableHeader*       aTable,
                              smnIndexHeader*       aIndex,
                              smnGetPageFunc        aGetPageFunc,
                              smnGetRowFunc         aGetRowFunc,
                              SChar*                aNullPtr,
                              idBool                aIsNeedValidation,
                              idBool                aIsEnableTable,
                              UInt                  aParallelDegree,
                              UInt                  aBuildFlag,
                              UInt                  aTotalRecCnt );

    static IDE_RC drop( smnIndexHeader * aIndex );

    static IDE_RC init( stnmrIterator       * aIterator,
                        void                * aTrans,
                        smcTableHeader      * aTable,
                        smnIndexHeader      * aIndex,
                        void*                 aDumpObject,
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

    static IDE_RC dest( stnmrIterator* aIterator );
    
    static IDE_RC insertRowUnique( idvSQL   * aStatistics,
                                   void     * aTrans,
                                   void     * aTable,
                                   void     * aIndex,
                                   smSCN      aInfiniteSCN,
                                   SChar    * aRow,
                                   SChar    * aNull,
                                   idBool     aUniqueCheck,
                                   smSCN      aStmtViewSCN,
                                   void     * aRowSID,
                                   SChar   ** aExistUniqueRow,
                                   ULong      aInsertWaitTime,
                                   idBool     aForbiddenToRetry );

    static UInt getSlots( stnmrNode* aNode,
                          smmSlot*   aSlots );    
    
    static IDE_RC insertRow( idvSQL         * aStatistics,
                             void           * aTrans,
                             void           * aTable,
                             void           * aIndex,
                             smSCN            aInfiniteSCN,
                             SChar          * aRow,
                             SChar          * aNull,
                             idBool           aUniqueCheck,
                             smSCN            aStmtViewSCN,
                             void           * aRowSID,
                             SChar         ** aExistUniqueRow,
                             ULong            aInsertWaitTime,
                             idBool           aForbiddenToRetry );

    static IDE_RC isRowUnique( void*     aTrans,
                               void*     aRow,
                               smTID*    aTid );
    
    static IDE_RC deleteRowNA( idvSQL*    aStatistics,
                               void*          aTrans,
                               void*          aIndex,
                               SChar*         aRow,
                               smiIterator*   aIterator,
                               sdRID          aRowRID );
    
    static IDE_RC freeSlot( void*          aIndex,
                            SChar*         aRow,
                            idBool         aIgnoreNotFoundKey,
                            idBool       * aIsExistFreeKey );
    
    static IDE_RC NA( void );

    static IDE_RC beforeFirst( stnmrIterator*       aIterator,
                               const smSeekFunc**  aSeekFunc );

    static IDE_RC beforeFirstU( stnmrIterator*       aIterator,
                                const smSeekFunc**  aSeekFunc );

    static IDE_RC beforeFirstR( stnmrIterator*       aIterator,
                                const smSeekFunc**  aSeekFunc );

    static IDE_RC afterLastR( stnmrIterator*       aIterator,
                              const smSeekFunc**  aSeekFunc );
    
    static IDE_RC fetchNext( stnmrIterator* aIterator,
                             const void**   aRow );

    static IDE_RC fetchNextU( stnmrIterator* aIterator,
                              const void**   aRow );
    
    static IDE_RC fetchNextR( stnmrIterator* aIterator );

    static void insertSlot( stnmrNode* aNode,
                            stnmrSlot* aSlot ); 

    static void propagate( stnmrHeader*    aHeader,
                           stnmrStatistic* aIndexStat,
                           stnmrNode*      aNode,
                           UInt            aUpdatePos,
                           stnmrStack*     aStack );
    
    static SInt locateInNode( stnmrNode* aNode,
                              void*      aPtr);
    
    static SInt bestentry( stnmrNode* aNode,
                           stdMBR*    aMbr);
    
    static stnmrNode* chooseLeaf( stnmrHeader*  aHeader,
                                  stdMBR*       aMbr,
                                  stnmrStack*   aStack,
                                  SInt*         aDepth);

    static void pickSeed( stnmrSlot* aSlots,
                          SInt       aCount);

    static SInt pickNext( stnmrSlot* aSlots,
                          SInt       aCount,
                          stdMBR*    aMbr1,
                          stdMBR*    aMbr2);
    

    static stnmrNode* findLeaf( const stnmrHeader* aHeader,
                                stnmrStatistic*    aIndexStat,
                                stnmrStack*        aStack,
                                stdMBR*            aMbr,
                                void*              aPtr,
                                SInt*              aDepth);
    
                          
    static void findNextLeaf( const stnmrHeader*  aHeader,
                              const smiCallBack*  aCallBack,
                              stnmrStack*         aStack,
                              SInt*               aDepth);

    static IDE_RC beforeFirstInternal( stnmrIterator* aIterator );

    static SInt compareRows( const stnmrColumn* aColumns,
                             const stnmrColumn* aFence,
                             const void*        aRow1,
                             const void*        aRow2 );
    
    static SInt compareRowsAndOID( const stnmrColumn* aColumns,
                                   const stnmrColumn* aFence,
                                   SChar*             aRow1,
                                   SChar*             aRow2 );
    
    static SInt compareRows( const stnmrColumn* aColumns,
                             const stnmrColumn* aFence,
                             SChar*             aRow1,
                             SChar*             aRow2,
                             idBool*            aIsEqual);

    static IDE_RC rebuild( void*             aTrans,
                           smnIndexHeader*   aIndex,
                           SChar*            aNull,
                           smnSortStack*     aStack );
    
    static void initNode(stnmrNode *aNode, SInt aFlag, IDU_LATCH aLatch);
    
    static void insertNode(stnmrNode*  aNewNode,
                           stdMBR*     aMbr,
                           void*       aPtr,
                           ULong       aVersion);

    static void split( stnmrHeader*  aHeader,
                       stnmrNode*    aLNode,
                       stnmrNode*    aRNode,
                       UInt          aPos,
                       stdMBR*       aChildMBR,
                       void*         aChildPtr,
                       ULong         aChildVersion,
                       stdMBR*       aInsertMBR,
                       void*         aInsertPtr,
                       ULong         aInsertVersion,
                       stdMBR*       aMbr1,
                       stdMBR*       aMbr2);
    
    static void deleteNode( stnmrNode* aNode,
                            SInt       aPos );
    
    /* For Make Disk Image When server is stopped normally */
    static IDE_RC makeDiskImage( smnIndexHeader* aIndex,
                                 smnIndexFile*   aIndexFile );
    
    static IDU_LATCH getLatchValueOfNode( volatile stnmrNode* aNodePtr );
    
    static IDU_LATCH getLatchValueOfHeader( volatile stnmrHeader* aHeaderPtr );

    static IDE_RC getPositionNA( stnmrIterator*     aIterator,
                                 smiCursorPosInfo*  aPosInfo );
    
    static IDE_RC setPositionNA( stnmrIterator    * aIterator,
                                 smiCursorPosInfo * aPosInfo );

    static IDE_RC allocIterator( void** aIteratorMem );
    
    static IDE_RC freeIterator( void* aIteratorMem );

    static IDE_RC getGeometryHeaderFromRow( stnmrHeader*    aHeader,
                                 const void*          aRow,
                                 UInt                 aFlag,
                                 stdGeometryHeader**  aGeoHeader );

    static IDE_RC buildRecordForMemRTreeHeader(idvSQL              * /*aStatistics*/,
                                               void                * aHeader,
                                               void                * /* aDumpObj */,
                                               iduFixedTableMemory * aMemory);
    
    static IDE_RC buildRecordForMemRTreeNodePool(idvSQL              * /*aStatistics*/,
                                                 void                * aHeader,
                                                 void                * /* aDumpObj */,
                                                 iduFixedTableMemory * aMemory);
    static IDE_RC buildRecordForMemRTreeStat(idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory * aMemory);
    
private:
    static iduMemPool           mIteratorPool;
};

inline void stnmrRTree::initNode( stnmrNode* aNode, 
                                  SInt       aFlag, 
                                  IDU_LATCH  aLatch )
{
    aNode->mPrev      = NULL;
    aNode->mNext      = NULL;
    SM_SET_SCN_CONST(&(aNode->mSCN), 0);
    aNode->mFlag      = aFlag;
    aNode->mFreeNodeList    = NULL;
    aNode->mSlotCount = 0;
    aNode->mLatch     = aLatch;
    aNode->mNextSPtr  = NULL;
    aNode->mVersion   = 0;
    
}


#endif /* _O_STNMR_MODULE_H_ */
