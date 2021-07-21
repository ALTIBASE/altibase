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

#ifndef _O_SDA_H_
#define _O_SDA_H_ 1

#include <sdi.h>
#include <qci.h>

typedef enum sdaQtcNodeType
{
    SDA_NONE           = 0,
    SDA_KEY_COLUMN     = 1,
    SDA_SUB_KEY_COLUMN = 2,
    SDA_HOST_VAR       = 3,
    SDA_CONSTANT_VALUE = 4,
    SDA_OR             = 5,
    SDA_EQUAL          = 6,
    SDA_EQUALANY       = 7,
    SDA_SHARD_KEY_FUNC = 8,

} sdaQtcNodeType;

typedef struct sdaSubqueryAnalysis
{
    struct sdiShardAnalysis * mShardAnalysis;
    sdaSubqueryAnalysis     * mNext;

} sdaSubqueryAnalysis;

/* PROJ-2646 shard analyzer enhancement */

typedef struct sdaValueList
{
    qtcNode      * mKeyColumn;
    sdiValueInfo * mValue;
    sdaValueList * mNext;

} sdaValueList;

typedef struct sdaCNFList
{
    qtcNode    * mCNF;
    sdaCNFList * mNext;

} sdaCNFList;

typedef struct sdaFrom
{
    UShort                mTupleId;

    UInt                  mKeyCount;
    UShort              * mKey;

    UShort                mValuePtrCount;
    sdiValueInfo       ** mValuePtrArray;

    sdiTableInfo        * mTableInfo;
    sdiShardInfo          mShardInfo;

    idBool                mIsShardQuery;
    sdiAnalysisFlag       mAnalysisFlag;

    idBool                mIsJoined;
    idBool                mIsNullPadding;
    idBool                mIsAntiJoinInner;

    sdaFrom             * mNext;

} sdaFrom;

#define SDA_INIT_SHARD_FROM( info )                           \
{                                                             \
    (info)->mTupleId                      = 0;                \
    (info)->mKeyCount                     = 0;                \
    (info)->mKey                          = NULL;             \
    (info)->mValuePtrCount                = 0;                \
    (info)->mValuePtrArray                = NULL;             \
    (info)->mShardInfo.mKeyDataType       = ID_UINT_MAX;      \
    (info)->mShardInfo.mDefaultNodeId     = SDI_NODE_NULL_ID; \
    (info)->mShardInfo.mSplitMethod       = SDI_SPLIT_NONE;   \
    (info)->mShardInfo.mRangeInfo         = NULL;             \
    (info)->mIsShardQuery                 = ID_FALSE;         \
    (info)->mIsJoined                     = ID_FALSE;         \
    (info)->mIsNullPadding                = ID_FALSE;         \
    (info)->mIsAntiJoinInner              = ID_FALSE;         \
    (info)->mNext                         = NULL;             \
    (info)->mTableInfo                    = NULL;             \
    SDI_INIT_ANALYSIS_FLAG( ( info )->mAnalysisFlag );        \
}

class sda
{
public:

    static IDE_RC checkStmtTypeBeforeAnalysis( qcStatement * aStatement );
    // PROJ-2727
    static IDE_RC checkDCLStmt( qcStatement * aStatement );
    
    static IDE_RC analyze( qcStatement * aStatement,
                           ULong         aSMN );

    static idBool findRangeInfo( sdiRangeInfo * aRangeInfo,
                                 UInt           aNodeId );

    /* PROJ-2655 Composite shard key */
    static IDE_RC getRangeIndexByValue( qcTemplate     * aTemplate,
                                        mtcTuple       * aShardKeyTuple,
                                        sdiAnalyzeInfo * aShardAnalysis,
                                        UShort           aValueIndex,
                                        sdiValueInfo   * aValue,
                                        sdiRangeIndex  * aRangeIndex,
                                        UShort         * aRangeIndexCount,
                                        idBool         * aHasDefaultNode,
                                        idBool           aIsSubKey );

    static IDE_RC getRangeIndexFromHash( sdiRangeInfo    * aRangeInfo,
                                         UShort            aValueIndex,
                                         UInt              aHashValue,
                                         sdiRangeIndex   * aRangeIdxList,
                                         UShort          * aRangeIdxCount,
                                         sdiSplitMethod    aPriorSplitMethod,
                                         UInt              aPriorKeyDataType,
                                         idBool          * aHasDefaultNode,
                                         idBool            aIsSubKey );

    static IDE_RC getRangeIndexFromRange( sdiRangeInfo    * aRangeInfo,
                                          const mtdModule * aKeyModule,
                                          UShort            aValueIndex,
                                          const void      * aValue,
                                          sdiRangeIndex   * aRangeIdxList,
                                          UShort          * aRangeIdxCount,
                                          sdiSplitMethod    aPriorSplitMethod,
                                          UInt              aPriorKeyDataType,
                                          idBool          * aHasDefaultNode,
                                          idBool            aIsSubKey );

    static IDE_RC getRangeIndexFromList( sdiRangeInfo    * aRangeInfo,
                                         const mtdModule * aKeyModule,
                                         UShort            aValueIndex,
                                         const void      * aValue,
                                         sdiRangeIndex   * aRangeIdxList,
                                         UShort          * aRangeIdxCount,
                                         idBool          * aHasDefaultNode,
                                         idBool            aIsSubKey );

    static IDE_RC checkValuesSame( qcTemplate   * aTemplate,
                                   mtcTuple     * aShardKeyTuple,
                                   UInt           aKeyDataType,
                                   sdiValueInfo * aValue1,
                                   sdiValueInfo * aValue2,
                                   idBool       * aIsSame );

    /* BUG-45899 */
    static void setNonShardQueryReason( sdiPrintInfo * aPrintInfo,
                                        UShort         aReason );

    static void setPrintInfoFromAnalyzeInfo( sdiPrintInfo   * aPrintInfo,
                                             sdiAnalyzeInfo * aAnalyzeInfo );

    static void setPrintInfoFromPrintInfo( sdiPrintInfo * aDst,
                                           sdiPrintInfo * aSrc );

    static IDE_RC allocAndSetShardHostValueInfo( qcStatement      * aStatement,
                                                 UInt               aBindParamId,
                                                 sdiValueInfo    ** aValueInfo );

    static IDE_RC allocAndSetShardConstValueInfo( qcStatement      * aStatement,
                                                  UInt               aKeyDataType,
                                                  const void       * aKeyValue,
                                                  sdiValueInfo    ** aValueInfo );

    static UInt calculateValueInfoSize( UInt         aKeyDataType,
                                        const void * aConstValue );

    static IDE_RC allocAndCopyValue( qcStatement      * aStatement,
                                     sdiValueInfo     * aFromValueInfo,
                                     sdiValueInfo    ** aToValueInfo );

    /* TASK-7219 Shard Transformer Refactoring */
    /* BUG-47903 이후, 전 노드 수행 Non-Shard Query 를 고려해서 수정함 */
    static IDE_RC isShardQuery( sdiAnalysisFlag * aAnalysisFlag,
                                idBool          * aIsShardQuery );

    static void setNonShardQueryReasonErr( sdiAnalysisFlag * aAnalysisFlag,
                                           UShort          * aErrIdx );

    static IDE_RC mergeAnalysisFlag( sdiAnalysisFlag * aFlag1,
                                     sdiAnalysisFlag * aFlag2,
                                     UShort            aType );

    static IDE_RC mergeFlag( idBool * aFlag1,
                             idBool * aFlag2,
                             UInt     aIdx );

    static IDE_RC allocAnalysis( qcStatement       * aStatement,
                                 sdiShardAnalysis ** aAnalysis );

    static IDE_RC getParseTreeAnalysis( qcParseTree       * aParseTree,
                                        sdiShardAnalysis ** aAnalysis );

    static IDE_RC setParseTreeAnalysis( qcParseTree      * aParseTree,
                                        sdiShardAnalysis * aAnalysis );

    static IDE_RC makeAndSetAnalyzeInfo( qcStatement      * aStatement,
                                         sdiShardAnalysis * aAnalysis,
                                         sdiAnalyzeInfo  ** aAnalyzeInfo );

    static IDE_RC preAnalyzeQuerySet( qcStatement * aStatement,
                                      qmsQuerySet * aQuerySet,
                                      ULong         aSMN );

private:

    static IDE_RC analyzeSelect( qcStatement             * aStatement,
                                 ULong                     aSMN );
    
    static IDE_RC analyzeSelectCore( qcStatement * aStatement,
                                     ULong         aSMN,
                                     sdiKeyInfo  * aOutRefInfo,
                                     idBool        aIsSubKey );

    static IDE_RC normalizePredicate( qcStatement * aStatement,
                                      qmsQuerySet * aQuerySet,
                                      qtcNode    ** aPredicate );

    static IDE_RC getQtcNodeTypeWithShardFrom( qcStatement         * aStatement,
                                               sdaFrom             * aShardFrom,
                                               qtcNode             * aNode,
                                               sdaQtcNodeType      * aQtcNodeType );

    static IDE_RC getQtcNodeTypeWithKeyInfo( qcStatement         * aStatement,
                                             sdiKeyInfo          * aKeyInfo,
                                             qtcNode             * aNode,
                                             sdaQtcNodeType      * aQtcNodeType,
                                             sdiKeyInfo         ** aKeyInfoRef );

    static IDE_RC getKeyValueID4InsertValue( qcStatement     * aStatement,
                                             ULong             aSMN,
                                             qmsTableRef     * aInsertTableRef,
                                             qcmColumn       * aInsertColumns,
                                             qmmMultiRows    * aInsertRows,
                                             sdiKeyInfo      * aKeyInfo,
                                             idBool          * aIsShardQuery,
                                             sdiAnalysisFlag * aAnalysisFlag,
                                             idBool            aIsSubKey );

    static IDE_RC getKeyValueID4ProcArguments( qcStatement     * aStatement,
                                               ULong             aSMN,
                                               qsProcParseTree * aPlanTree,
                                               qsExecParseTree * aParseTree,
                                               sdiKeyInfo      * aKeyInfo,
                                               sdiAnalysisFlag * aAnalysisFlag,
                                               idBool            aIsSubKey );

    static IDE_RC setAnalysisCommonInfo( qcStatement      * aStatement,
                                         sdiShardAnalysis * aAnalysis,
                                         sdiAnalyzeInfo   * aAnalysisResult );

    static IDE_RC setAnalysisCombinedInfo( sdiShardAnalysis * aAnalysis,
                                           sdiAnalyzeInfo   * aAnalysisResult,
                                           sdiAnalysisFlag  * aAnalysisFlag );

    static IDE_RC setAnalysisIndividualInfo( qcStatement      * aStatement,
                                             sdiShardAnalysis * aAnalysis,
                                             UShort           * aValuePtrCount,
                                             sdiValueInfo*   ** aValuePtrArray,
                                             sdiSplitMethod   * aSplitMethod,
                                             UInt             * aKeyDataType );

    static IDE_RC subqueryAnalysis4NodeTree( qcStatement          * aStatement,
                                             ULong                  aSMN,
                                             qtcNode              * aNode,
                                             sdiKeyInfo           * aOutRefInfo,
                                             idBool                 aIsSubKey,
                                             sdaSubqueryAnalysis ** aSubqueryAnalysis );

    static IDE_RC traverseOverColumn4SubqueryAnalysis( qcStatement          * aStatement,
                                                       ULong                  aSMN,
                                                       qtcOverColumn        * aOverColumn,
                                                       sdiKeyInfo           * aOutRefInfo,
                                                       idBool                 aIsSubKey,
                                                       sdaSubqueryAnalysis ** aSubqueryAnalysis );

    static IDE_RC traversePredForCheckingShardKeyJoin( qcStatement * aStatement,
                                                       qtcNode     * aCompareNode,
                                                       qtcNode     * aLeftNode,
                                                       qtcNode     * aRightNode,
                                                       sdiKeyInfo  * aOutRefInfo,
                                                       idBool        aIsSubKey );

    static IDE_RC checkSubqueryAndTheOtherSideNode( qcStatement * aStatement,
                                                    qtcNode     * aSubquery,
                                                    qtcNode     * aOuterColumn,
                                                    sdiKeyInfo  * aOutRefInfo,
                                                    idBool        aIsSubKey );

    static IDE_RC resetSubqueryAnalysisReasonForTargetLink( sdiShardAnalysis  * aSubQAnalysis,
                                                            sdiShardInfo      * aShardInfo,
                                                            UShort              aShardKeyColumnIdx,
                                                            idBool              aIsSubKey,
                                                            sdiKeyInfo       ** aKeyInfoRet );

    static IDE_RC setShardInfo4Subquery( qcStatement         * aStatement,
                                         sdiShardAnalysis    * aQuerySetAnalysis,
                                         sdaSubqueryAnalysis * aSubqueryAnalysis,
                                         idBool                aIsSelect,
                                         idBool                aIsSubKey,
                                         sdiAnalysisFlag     * aAnalysisFlag );

    static idBool isSameRangesForCloneAndSolo( sdiRangeInfo * aRangeInfo1,
                                               sdiRangeInfo * aRangeInfo2 );

    static IDE_RC subqueryAnalysis4ParseTree( qcStatement      * aStatement,
                                              ULong              aSMN,
                                              qmsParseTree     * aParseTree,
                                              idBool             aIsSubKey );

    static IDE_RC subqueryAnalysis4SFWGH( qcStatement      * aStatement,
                                          ULong              aSMN,
                                          qmsSFWGH         * aSFWGH,
                                          sdiShardAnalysis * aQuerySetAnalysis,
                                          idBool             aIsSubKey );

    static IDE_RC subqueryAnalysis4FromTree( qcStatement          * aStatement,
                                             ULong                  aSMN,
                                             qmsFrom              * aFrom,
                                             sdiKeyInfo           * aOutRefInfo,
                                             idBool                 aIsSubKey,
                                             sdaSubqueryAnalysis ** aSubqueryAnalysis );

    /* PROJ-2646 shard analyzer enhancement */

    static IDE_RC analyzeParseTree( qcStatement * aStatement,
                                    ULong         aSMN,
                                    sdiKeyInfo  * aOutRefInfo,
                                    idBool       aIsSubKey );

    static IDE_RC analyzeQuerySet( qcStatement  * aStatement,
                                   ULong          aSMN,
                                   qmsQuerySet  * aQuerySet,
                                   sdiKeyInfo   * aOutRefInfo,
                                   idBool         aIsSubKey );

    static IDE_RC analyzeSFWGH( qcStatement * aStatement,
                                ULong         aSMN,
                                qmsQuerySet * aQuerySet,
                                sdiKeyInfo  * aOutRefInfo,
                                idBool        aIsSubKey );

    static IDE_RC analyzeSFWGHCore( qcStatement      * aStatement,
                                    ULong              aSMN,
                                    qmsQuerySet      * aQuerySet,
                                    sdiShardAnalysis * aQuerySetAnalysis,
                                    qtcNode          * aShardPredicate,
                                    sdiKeyInfo       * aOutRefInfo,
                                    idBool             aIsSubKey );

    static IDE_RC analyzeFrom( qcStatement       * aStatement,
                               ULong               aSMN,
                               qmsFrom           * aFrom,
                               sdiShardAnalysis  * aQuerySetAnalysis,
                               sdaFrom          ** aShardFromInfo,
                               idBool              aIsSubKey );

    static IDE_RC checkTableRef( qcStatement      * aStatement,
                                 qmsTableRef      * aTableRef,
                                 sdiShardAnalysis * aQuerySetAnalysis );

    static IDE_RC makeShardFromInfo4Table( qcStatement       * aStatement,
                                           ULong               aSMN,
                                           qmsFrom           * aFrom,
                                           sdiShardAnalysis  * aQuerySetAnalysis,
                                           sdaFrom          ** aShardFromInfo,
                                           idBool              aIsSubKey );

    static IDE_RC makeShardFromInfo4View( qcStatement       * aStatement,
                                          qmsFrom           * aFrom,
                                          sdiShardAnalysis  * aQuerySetAnalysis,
                                          sdiShardAnalysis  * aViewAnalysis,
                                          sdaFrom          ** aShardFromInfo,
                                          idBool              aIsSubKey );

    static IDE_RC analyzePredicate( qcStatement     * aStatement,
                                    qtcNode         * aCNF,
                                    sdaCNFList      * aCNFOnCondition,
                                    sdaFrom         * aShardFromInfo,
                                    idBool            aIsOneNodeSQL,
                                    sdiKeyInfo      * aOutRefInfo,
                                    sdiKeyInfo     ** aKeyInfo,
                                    sdiAnalysisFlag * aAnalysisFlag,
                                    idBool            aIsSubKey );

    static IDE_RC makeKeyInfoFromJoin( qcStatement  * aStatement,
                                       qtcNode      * aCNF,
                                       sdaFrom      * aShardFroInfo,
                                       sdiKeyInfo  ** aKeyInfo,
                                       idBool         aIsSubKey );

    static IDE_RC getShardJoinTupleColumn( qcStatement * aStatement,
                                           sdaFrom     * aShardFromInfo,
                                           qtcNode     * aCNF,
                                           idBool      * aIsFound,
                                           UShort      * aLeftTuple,
                                           UShort      * aLeftColumn,
                                           UShort      * aRightTuple,
                                           UShort      * aRightColumn );

    static IDE_RC isShardEquivalentExpression( qcStatement * aStatement,
                                               qtcNode     * aNode1,
                                               qtcNode     * aNode2,
                                               idBool      * aIsTrue );

    static IDE_RC getShardFromInfo( sdaFrom     * aShardFromInfo,
                                    UShort        aTuple,
                                    UShort        aColumn,
                                    sdaFrom    ** aRetShardFrom );

    static IDE_RC makeKeyInfo( qcStatement * aStatement,
                               sdaFrom     * aMyShardFrom,
                               sdaFrom     * aLinkShardFrom,
                               idBool        aIsSubKey,
                               sdiKeyInfo ** aKeyInfo );

    static IDE_RC getKeyInfoForAddingKeyList( sdaFrom     * aShardFrom,
                                              sdiKeyInfo  * aKeyInfo,
                                              idBool        aIsSubKey,
                                              sdiKeyInfo ** aRetKeyInfo );

    static IDE_RC isSameShardHashInfo( sdiShardInfo * aShardInfo1,
                                       sdiShardInfo * aShardInfo2,
                                       idBool         aIsSubKey,
                                       idBool       * aOutResult );

    static IDE_RC isSameShardRangeInfo( sdiShardInfo * aShardInfo1,
                                        sdiShardInfo * aShardInfo2,
                                        idBool         aIsSubKey,
                                        idBool       * aOutResult );

    static IDE_RC isSameShardInfo( sdiShardInfo * aShardInfo1,
                                   sdiShardInfo * aShardInfo2,
                                   idBool         aIsSubKey,
                                   idBool       * aIsSame );

    static IDE_RC addKeyList( qcStatement * aStatement,
                              sdaFrom     * aFrom,
                              sdiKeyInfo ** aKeyInfo,
                              sdiKeyInfo  * aKeyInfoForAdding );

    static IDE_RC makeKeyInfoFromNoJoin( qcStatement * aStatement,
                                         sdaFrom     * aShardFromInfo,
                                         idBool        aIsSubKey,
                                         sdiKeyInfo ** aKeyInfo );

    static IDE_RC checkOutRefShardJoin( qcStatement     * aStatement,
                                        qtcNode         * aCNF,
                                        sdaCNFList      * sCNFOnCondition,
                                        sdiKeyInfo      * aKeyInfo,
                                        sdiKeyInfo      * aOutRefInfo,
                                        sdiAnalysisFlag * aAnalysisFlag,
                                        idBool            aIsSubKey );

    static IDE_RC findShardJoinWithOutRefKey( qcStatement     * aStatement,
                                              sdiKeyInfo      * aKeyInfo,
                                              qtcNode         * aCNFOr,
                                              sdiKeyInfo      * aOutRefInfo,
                                              sdiAnalysisFlag * aAnalysisFlag,
                                              idBool            aIsSubKey,
                                              idBool          * aIsOutRefJoinFound );

    static IDE_RC makeValueInfo( qcStatement * aStatement,
                                 qtcNode     * aCNF,
                                 sdiKeyInfo  * aKeyInfo );

    static IDE_RC getShardValue( qcStatement       * aStatement,
                                 qtcNode           * aCNF,
                                 sdiKeyInfo        * aKeyInfo,
                                 sdaValueList     ** aValueList );

    static IDE_RC isShardCondition( qcStatement  * aStatement,
                                    qtcNode      * aCNF,
                                    sdiKeyInfo   * aKeyInfo,
                                    idBool       * aIsShardCond );

    static IDE_RC addValueListOnKeyInfo( qcStatement  * aStatement,
                                         sdiKeyInfo   * aKeyInfo,
                                         sdaValueList * aValueList );

    static IDE_RC analyzeTarget( qcStatement * aStatement,
                                 qmsQuerySet * aQuerySet,
                                 sdiKeyInfo  * aKeyInfo );

    static IDE_RC getKeyInfoForAddingKeyTarget( qtcNode     * aTargetColumn,
                                                sdiKeyInfo  * aKeyInfo,
                                                sdiKeyInfo ** aKeyInfoForAdding );

    static IDE_RC setKeyTarget( qcStatement * aStatement,
                                sdiKeyInfo  * aKeyInfoForAdding,
                                UShort        aTargetOrder );

    static IDE_RC setShardInfoWithKeyInfo( qcStatement  * aStatement,
                                           sdiKeyInfo   * aKeyInfo,
                                           sdiShardInfo * aShardInfo,
                                           idBool         aIsSubKey );

    static IDE_RC getRangeIntersection( qcStatement   * aStatement,
                                        sdiRangeInfo  * aRangeInfo1,
                                        sdiRangeInfo  * aRangeInfo2,
                                        sdiRangeInfo ** aOutRangeInfo ); /* TASK-7219 Shard Transformer Refactoring */

    static void copyShardInfoFromShardInfo( sdiShardInfo * aTo,
                                            sdiShardInfo * aFrom );

    static void copyShardInfoFromObjectInfo( sdiShardInfo  * aTo,
                                             sdiObjectInfo * aFrom,
                                             idBool          aIsSubKey );

    static IDE_RC allocAndMergeValues( qcStatement         * aStatement,
                                       sdiValueInfo       ** aSrcValueInfoPtrArray1,
                                       UShort                aSrcValueInfoPtrCount1,
                                       sdiValueInfo       ** aSrcValueInfoPtrArray2,
                                       UShort                aSrcValueInfoPtrCount2,
                                       sdiValueInfo *     ** aNewValueInfoPtrArray,
                                       UShort              * aNewValueInfoPtrCount );

    static IDE_RC andRangeInfo( qcStatement   * aStatement,
                                sdiRangeInfo  * aRangeInfo1,
                                sdiRangeInfo  * aRangeInfo2,
                                sdiRangeInfo ** aAndRangeInfo );

    static idBool isSubRangeInfo( sdiRangeInfo * aRangeInfo,
                                  sdiRangeInfo * aSubRangeInfo,
                                  UInt           aSubRangeDefaultNode );

    static IDE_RC setAnalysisFlag4SFWGH( qcStatement     * aStatement,
                                         qmsSFWGH        * aSFWGH,
                                         sdiKeyInfo      * aKeyInfo,
                                         sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4Distinct( qcStatement     * aStatement,
                                            qmsSFWGH        * aSFWGH,
                                            sdiKeyInfo      * aKeyInfo,
                                            sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4GroupBy( qcStatement     * aStatement,
                                           qmsSFWGH        * aSFWGH,
                                           sdiKeyInfo      * aKeyInfo,
                                           sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4WindowFunc( qmsAnalyticFunc * aAnalyticFuncList,
                                              sdiKeyInfo      * aKeyInfo,
                                              sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC analyzeQuerySetAnalysis( qcStatement * aStatement,
                                           qmsQuerySet * aMyQuerySet,
                                           qmsQuerySet * aLeftQuerySet,
                                           qmsQuerySet * aRightQuerySet,
                                           idBool        aIsSubKey );

    static IDE_RC setQuerySetAnalysis( qmsQuerySet      * aQuerySet,
                                       sdiSplitMethod     aLeftQuerySetSplit,
                                       sdiSplitMethod     aRightQuerySetSplit,
                                       sdiShardAnalysis * aSetAnalysis );

    static IDE_RC analyzeSetLeftRight( qcStatement       * aStatement,
                                       sdiShardAnalysis  * aLeftQuerySetAnalysis,
                                       sdiShardAnalysis  * aRightQuerySetAnalysis,
                                       sdiKeyInfo       ** aKeyInfo,
                                       idBool              aIsSubKey );

    static IDE_RC makeKeyInfo4SET( qcStatement * aStatement,
                                   sdiKeyInfo  * aSourceKeyInfo,
                                   sdiKeyInfo ** aKeyInfoList,
                                   idBool        aIsLeft );

    static IDE_RC getKeyInfoForAddingSetKeyList( sdiKeyInfo  * aKeyInfoList,
                                                 sdiKeyInfo  * aSourceKeyInfo,
                                                 sdiKeyInfo ** aKeyInfoForAdding );

    static IDE_RC devideKeyInfo( qcStatement * aStatement,
                                 sdiKeyInfo  * aKeyInfo,
                                 sdiKeyInfo ** aDevidedKeyInfo );

    static IDE_RC mergeKeyInfo4Set( qcStatement * aStatement,
                                    sdiKeyInfo  * aLeftKeyInfo,
                                    sdiKeyInfo  * aRightKeyInfo,
                                    idBool        aIsSubKey,
                                    sdiKeyInfo ** aKeyInfo );

    static IDE_RC mergeKeyInfo( qcStatement * aStatement,
                                sdiKeyInfo  * aLeftKeyInfo,
                                sdiKeyInfo  * aRightKeyInfo,
                                sdiKeyInfo ** aKeyInfo );

    static IDE_RC makeKeyInfo4SetTarget( qcStatement * aStatement,
                                         sdiKeyInfo  * aKeyInfo,
                                         UInt          aTargetPos,
                                         sdiKeyInfo ** aDevidedKeyInfo );

    static IDE_RC getSameKey4Set( sdiKeyInfo  * aKeyInfo,
                                  sdiKeyInfo  * aCurrKeyInfo,
                                  sdiKeyInfo ** aRetKeyInfo );

    static IDE_RC setAnalysisFlag4ParseTree( qmsParseTree * aParseTree,
                                             ULong          aSMN,
                                             idBool         aIsSubKey );

    static IDE_RC appendKeyInfo( qcStatement * aStatement,
                                 sdiKeyInfo  * aKeyInfo,
                                 sdiKeyInfo  * aArgument );

    static IDE_RC mergeSameKeyInfo( qcStatement * aStatement,
                                    sdiKeyInfo  * aKeyInfoTmp,
                                    sdiKeyInfo ** aKeyInfo );

    static IDE_RC analyzeInsert( qcStatement * aStatement,
                                 ULong         aSMN );
    static IDE_RC analyzeUpdate( qcStatement * aStatement,
                                 ULong         aSMN );
    static IDE_RC analyzeDelete( qcStatement * aStatement,
                                 ULong         aSMN );
    static IDE_RC analyzeExecProc( qcStatement * aStatement,
                                   ULong         aSMN );

    static IDE_RC checkUpdate( qcStatement     * aStatement,
                               qmmUptParseTree * aUptParseTree,
                               ULong             aSMN );
    static IDE_RC checkDelete( qcStatement     * aStatement,
                               qmmDelParseTree * aDelParseTree );
    static IDE_RC checkInsert( qcStatement     * aStatement,
                               qmmInsParseTree * aInsParseTree );
    static IDE_RC checkExecProc( qcStatement     * aStatement,
                                 qsExecParseTree * aExecParseTree );

    static IDE_RC analyzeUpdateCore( qcStatement       * aStatement,
                                     ULong               aSMN,
                                     sdiObjectInfo     * aShardObjInfo,
                                     idBool              aIsSubKey );

    static IDE_RC analyzeDeleteCore( qcStatement       * aStatement,
                                     ULong               aSMN,
                                     sdiObjectInfo     * aShardObjInfo,
                                     idBool              aIsSubKey );

    static IDE_RC analyzeInsertCore( qcStatement       * aStatement,
                                     ULong               aSMN,
                                     sdiObjectInfo     * aShardObjInfo,
                                     idBool              aIsSubKey );

    static IDE_RC setInsertValueAnalysis( qcStatement      * aStatement,
                                          UShort             aSMN,
                                          qmmInsParseTree  * aParseTree,
                                          sdiShardAnalysis * aAnalysis,
                                          idBool             aIsSubKey );

    static IDE_RC setInsertSelectAnalysis( qcStatement      * aStatement,
                                           UShort             aSMN,
                                           qmmInsParseTree  * aParseTree,
                                           sdiShardAnalysis * aAnalysis,
                                           idBool             aIsSubKey );

    static IDE_RC analyzeExecProcCore( qcStatement       * aStatement,
                                       ULong               aSMN, /* TASK-7219 Shard Transformer Refactoring */
                                       sdiObjectInfo     * aShardObjInfo,
                                       idBool              aIsSubKey );

    static IDE_RC setDMLCommonAnalysis( qcStatement      * aStatement,
                                        ULong              aSMN, /* TASK-7219 Shard Transformer Refactoring */
                                        sdiObjectInfo    * aShardObjInfo,
                                        qmsTableRef      * aTableRef,
                                        sdiShardAnalysis * aAnalysis,
                                        idBool             aIsSubKey );

    static IDE_RC subqueryAnalysis4DML( qcStatement      * aStatement,
                                        UShort             aSMN,
                                        sdiShardAnalysis * aAnalysis,
                                        idBool             aIsSubKey );

    static IDE_RC getKeyConstValue( qcStatement      * aStatement,
                                    qtcNode          * aNode,
                                    const mtdModule  * aModule,
                                    const void      ** aValue );

    static IDE_RC getShardInfo( qtcNode       * aNode,
                                sdiKeyInfo    * aKeyInfo,
                                sdiShardInfo ** aShardInfo );

    static IDE_RC analyzeAnsiJoin( qcStatement     * aStatement,
                                   qmsQuerySet     * aQuerySet,
                                   qmsFrom         * aFrom,
                                   sdaFrom         * aShardFrom,
                                   sdaCNFList     ** aCNFList,
                                   sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setNullPaddedShardFrom( qmsFrom * aFrom,
                                          sdaFrom * aShardFrom,
                                          idBool  * aIsShardNullPadding );

    static IDE_RC isCloneAndShardExists( qmsFrom * aFrom,
                                         idBool  * aIsCloneExists,
                                         idBool  * aIsShardExists );

    static IDE_RC analyzePredJoin( qcStatement     * aStatement,
                                   qtcNode         * aNode,
                                   sdaFrom         * aShardFromInfo,
                                   sdiAnalysisFlag * aAnalysisFlag,
                                   idBool            aIsSubKey );

    /* PROJ-2655 Composite shard key */
    static IDE_RC setKeyValue( UInt             aKeyDataType,
                               const void     * aConstValue,
                               sdiValueInfo   * aValue );

    static IDE_RC compareSdiValue( UInt            aKeyDataType,
                                   sdiValueInfo  * aValue1,
                                   sdiValueInfo  * aValue2,
                                   SInt          * aResult );
    
    static IDE_RC isOneNodeSQL( sdaFrom      * aShardFrom,
                                sdiKeyInfo   * aKeyInfo,
                                idBool       * aIsOneNodeSQL,
                                UInt         * aDistKeyCount );

    static IDE_RC checkInnerOuterTable( qcStatement     * aStatement,
                                        qtcNode         * aNode,
                                        sdaFrom         * aShardFrom,
                                        sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC checkAntiJoinInner( sdiKeyInfo * aKeyInfo,
                                      UShort       aTable,
                                      UShort       aColumn,
                                      idBool     * aIsAntiJoinInner );

    static IDE_RC addTableInfo( qcStatement      * aStatement,
                                sdiShardAnalysis * aQuerySetAnalysis,
                                sdiTableInfo     * aTableInfo );

    static IDE_RC addTableInfoList( qcStatement      * aStatement,
                                    sdiShardAnalysis * aQuerySetAnalysis,
                                    sdiTableInfoList * aTableInfoList );

    static IDE_RC mergeTableInfoList( qcStatement      * aStatement,
                                      sdiShardAnalysis * aQuerySetAnalysis,
                                      sdiShardAnalysis * aLeftQuerySetAnalysis,
                                      sdiShardAnalysis * aRightQuerySetAnalysis );

    /* BUG-45823 */
    static void increaseAnalyzeCount( qcStatement * aStatement );

    /* TASK-7219 Shard Transformer Refactoring */
    static IDE_RC allocAndSetAnalysis( qcStatement       * aStatement,
                                       qcParseTree       * aParseTree,
                                       qmsQuerySet       * aQuerySet,
                                       idBool              aIsSubKey,
                                       sdiShardAnalysis ** aAnalysis );

    static IDE_RC getShardPredicate( qcStatement * aStatement,
                                     qmsQuerySet * aQuerySet,
                                     idBool        aIsSelect,
                                     idBool        aIsSubKey,
                                     qtcNode    ** aPredicate );

    static IDE_RC checkAndGetShardObjInfo( ULong            aSMN,
                                           sdiAnalysisFlag * aAnalysisFlag,
                                           sdiObjectInfo   * aShardObjList,
                                           idBool            aIsSelect,
                                           idBool            aIsSubKey,
                                           sdiObjectInfo  ** aShardObjInfo );

    static IDE_RC setAnalysisFlag( qmsQuerySet * aQuerySet,
                                   idBool        aIsSubKey );

    static IDE_RC setAnalysisFlag4From( qmsFrom         * aFrom,
                                        sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4TransformAble( sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4Target( qcStatement     * aStatement,
                                          qmsTarget       * aTarget,
                                          sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4OrderBy( qmsSortColumns  * aOrderBy,
                                           sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4GroupPsmBind( qmsConcatElement * aGroupBy,
                                                sdiAnalysisFlag  * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4PsmBind( qtcNode         * aNode,
                                           sdiAnalysisFlag * aAnalysisFlag );

    static IDE_RC setAnalysisFlag4PsmLob( qcStatement     * aStatement,
                                          qtcNode         * aNode,
                                          sdiAnalysisFlag * aAnalysisFlag );
};
#endif /* _O_SDA_H_ */
