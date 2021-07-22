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
 * $Id: qmoPartition.h 14871 2006-01-13 01:39:23Z mhjeong $
 *
 * Description :
 *     파티션드 테이블 관련 최적화 처리.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_PARTITION_H_
#define _O_QMO_PARTITION_H_ 1

#include <qc.h>
#include <smiDef.h>
#include <qmnDef.h>

typedef struct qmmValueNode qmmValueNode;
typedef struct qmnChildren qmnChildren;

class qmoPartition
{
public:

    static IDE_RC optimizeInto(
        qcStatement  * aStatement,
        qmsTableRef  * aTableRef );

    /* BUG-47614 partition table insert시 optimize 성능 개선 */
    static IDE_RC optimizeIntoForInsert( qcStatement  * aStatement,
                                         qmsTableRef  * aTableRef,
                                         idBool         aIsConst );

    static SInt compareRangePartition(
        qcmColumn          * aKeyColumns,
        qmsPartCondValList * aComp1,
        qmsPartCondValList * aComp2 );

    /* BUG-46065 support range using hash */
    static SInt compareRangeUsingHashPartition( qmsPartCondValList * aComp1,
                                                qmsPartCondValList * aComp2 );

    static idBool compareListPartition(
        qcmColumn          * aKeyColumn,
        qmsPartCondValList * aPartKeyCondVal,
        void               * aValue );

    static IDE_RC partitionFilteringWithRow(
        qmsTableRef          * aTableRef,
        smiValue             * aValues,
        qmsPartitionRef     ** aSelectedPartitionRef );

    static IDE_RC makePartitions(
        qcStatement  * aStatement,
        qmsTableRef  * aTableRef );

    static IDE_RC getPartOrder(
        qcmColumn          * aPartKeyColumns,
        UInt                 aPartCount,
        qmsPartCondValList * aPartCondVal,
        UInt               * aPartOrder );

    static IDE_RC mergePartitionRef(
        qmsTableRef     * aTableRef,
        qmsPartitionRef * aPartitionRef );

    static IDE_RC minusPartitionRef(
        qmsTableRef     * aTableRef1,
        qmsTableRef     * aTableRef2 );

    static IDE_RC partitionPruningWithKeyRange(
        qcStatement * aStatement,
        qmsTableRef * aTableRef,
        smiRange    * aPartKeyRange );

    static IDE_RC partitionFilteringWithPartitionFilter(
        qcStatement            * aStatement,
        qmnRangeSortedChildren * aRangeSortedChildrenArray,
        UInt                   * aRangeIntersectCountArray,
        UInt                     aSelectedPartitionCount,
        UInt                     aPartitionCount,
        qmnChildren            * aChildren,
        qcmPartitionMethod       aPartitionMethod,
        smiRange               * aPartFilter,
        qmnChildren           ** aSelectedChildrenArea,
        UInt                   * aSelectedChildrenCount );

    static IDE_RC rangePartitionFilteringWithValues(
        qmsTableRef        * aTableRef,
        qmsPartCondValList * aPartCondVal,
        qmsPartitionRef   ** aSelectedPartitionRef );

    /* BUG-46065 support range using hash */
    static IDE_RC rangeUsingHashPartitionFilteringWithValues( qmsTableRef        * aTableRef,
                                                              qmsPartCondValList * aPartCondVal,
                                                              qmsPartitionRef   ** aSelectedPartitionRef );

    static IDE_RC listPartitionFilteringWithValue(
        qmsTableRef        * aTableRef,
        void               * aValue,
        qmsPartitionRef   ** aSelectedPartitionRef );

    static IDE_RC hashPartitionFilteringWithPartOrder(
        qmsTableRef      * aTableRef,
        UInt               aPartOrder,
        qmsPartitionRef ** aSelectedPartitionRef );

    static IDE_RC isIntersectPartKeyColumn(
        qcmColumn * aPartKeyColumns,
        qcmColumn * aUptColumns,
        idBool    * aIsIntersect );

    // partition용 update column list를 생성
    static IDE_RC makePartUpdateColumnList(
        qcStatement      * aStatement,
        qmsPartitionRef  * aPartitionRef,
        smiColumnList    * aColumnList,
        smiColumnList   ** aPartColumnList );

    static IDE_RC sortPartitionRef(
        qmnRangeSortedChildren * aRangeSortedChildrenArray,
        UInt                     aPartitionCount );

    /* BUG-46065 support range using hash */
    static IDE_RC sortRangeHashPartitionRef(
        qmnRangeSortedChildren * aRangeSortedChildrenArray,
        UInt                     aPartitionCount );

private:
    static SInt mCompareCondValType[QMS_PARTKEYCONDVAL_TYPE_COUNT][QMS_PARTKEYCONDVAL_TYPE_COUNT];


    static IDE_RC getPartCondValues(
        qcStatement        * aStatement,
        qcmColumn          * aPartKeyColumns,
        qcmColumn          * aColumnList,
        qmmValueNode       * aValueList,
        qmsPartCondValList * aPartCondVal );

    static IDE_RC getPartCondValuesFromRow(
        qcmColumn          * aKeyColumns,
        smiValue           * aValues,
        qmsPartCondValList * aPartCondVal );

    static idBool isIntersectRangePartition(
        qmsPartCondValList * aMinPartCondVal,
        qmsPartCondValList * aMaxPartCondVal,
        smiRange           * aPartKeyRange );

    static idBool isIntersectListPartition(
        qmsPartCondValList * aPartCondVal,
        smiRange           * aPartKeyRange );

    static IDE_RC makeHashKeyFromPartKeyRange(
        qcStatement * aStatement,
        iduMemory   * aMemory,
        smiRange    * aPartKeyRange,
        idBool      * aIsValidHashVal );

    static IDE_RC makeHashKeyFromPartKeyRange(
        qcStatement   * aStatement,
        iduVarMemList * aMemory,
        smiRange      * aPartKeyRange,
        idBool        * aIsValidHashVal );

    static IDE_RC rangePartitionPruningWithKeyRange(
        qmsTableRef * aTableRef,
        smiRange    * aPartKeyRange );

    static IDE_RC listPartitionPruningWithKeyRange(
        qmsTableRef * aTableRef,
        smiRange    * aPartKeyRange );

    static IDE_RC hashPartitionPruningWithKeyRange(
        qcStatement * aStatement,
        qmsTableRef * aTableRef,
        smiRange    * aPartKeyRange );

    static IDE_RC rangePartitionFilteringWithPartitionFilter(
        qmnRangeSortedChildren * aRangeSortedChildrenArray,
        UInt                   * aRangeIntersectCountArray,
        UInt                     aPartCount,
        smiRange               * aPartFilter,
        qmnChildren           ** aSelectedChildrenArea,
        UInt                   * aSelectedChildrenCount );

    static IDE_RC listPartitionFilteringWithPartitionFilter(
        qmnChildren  * aChildren,
        smiRange     * aPartFilter,
        qmnChildren ** aSelectedChildrenArea,
        UInt         * aSelectedChildrenCount );

    static IDE_RC hashPartitionFilteringWithPartitionFilter(
        qcStatement  * aStatement,
        UInt           aPartitionCount,
        qmnChildren  * aChildren,
        smiRange     * aPartFilter,
        qmnChildren ** aSelectedChildrenArea,
        UInt         * aSelectedChildrenCount );

    static SInt comparePartMinRangeMax( qmsPartCondValList * aMinPartCondVal,
                                        smiRange           * aPartKeyRange );
    
    static SInt comparePartMinRangeMin( qmsPartCondValList * aMinPartCondVal,
                                        smiRange           * aPartKeyRange );
    
    static SInt comparePartMaxRangeMin( qmsPartCondValList * aMaxPartCondVal,
                                        smiRange           * aPartKeyRange );
    
    static SInt comparePartMaxRangeMax( qmsPartCondValList * aMaxPartCondVal,
                                        smiRange           * aPartKeyRange );
    
    static SInt comparePartGT( qmsPartCondValList * aMinPartCondVal,
                               qmsPartCondValList * aMaxPartCondVal,
                               smiRange           * aPartKeyRange );
    
    static SInt comparePartGE( qmsPartCondValList * aMinPartCondVal,
                               qmsPartCondValList * aMaxPartCondVal,
                               smiRange           * aPartKeyRange );
    
    static SInt comparePartLT( qmsPartCondValList * aMinPartCondVal,
                               qmsPartCondValList * aMaxPartCondVal,
                               smiRange           * aPartKeyRange );
    
    static SInt comparePartLE( qmsPartCondValList * aMinPartCondVal,
                               qmsPartCondValList * aMaxPartCondVal,
                               smiRange           * aPartKeyRange );

    static void partitionFilterGT( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                   UInt                   * aRangeIntersectCountArray,
                                   smiRange               * aPartKeyRange,
                                   UInt                     aPartCount );
    
    static void partitionFilterGE( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                   UInt                   * aRangeIntersectCountArray,
                                   smiRange               * aPartKeyRange,
                                   UInt                     aPartCount );
    
    static void partitionFilterLT( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                   UInt                   * aRangeIntersectCountArray,
                                   smiRange               * aPartKeyRange,
                                   UInt                     aPartCount );
    
    static void partitionFilterLE( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                   UInt                   * aRangeIntersectCountArray,
                                   smiRange               * aPartKeyRange,
                                   UInt                     aPartCount );

    /* BUG-46065 support range using hash */
    static IDE_RC isIntersectRangeUsingHashPartition( qmsPartCondValList * aMinPartCondVal,
                                                      qmsPartCondValList * aMaxPartCondVal,
                                                      UInt                 aMinHashVal,
                                                      UInt                 aMaxHashVal,
                                                      idBool             * aIsIntersect );

    static IDE_RC rangeUsinghashPartitionPruningWithKeyRange( qcStatement * aStatement,
                                                              qmsTableRef * aTableRef,
                                                              smiRange    * aPartKeyRange );

    static IDE_RC makeHashKeyForRangePruning( qcStatement        * aStatement,
                                              mtkRangeCallBack   * aData,
                                              UInt               * aHashValue );

    static IDE_RC rangeUsingHashFilteringWithPartitionFilter( qcStatement            * aStatement,
                                                              qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                                              UInt                   * aRangeIntersectCountArray,
                                                              UInt                     aPartCount,
                                                              smiRange               * aPartFilter,
                                                              qmnChildren           ** aSelectedChildrenArea,
                                                              UInt                   * aSelectedChildrenCount );

    static SInt comparePartMinRangeMaxUsingHash( qmsPartCondValList * aMinPartCondVal,
                                                 UInt                 aMaxHashVal );

    static SInt comparePartMinRangeMinUsingHash( qmsPartCondValList * aMinPartCondVal,
                                                 UInt                 aMinHashVal );

    static SInt comparePartMaxRangeMinUsingHash( qmsPartCondValList * aMaxPartCondVal,
                                                 UInt                 aMinHashVal );

    static SInt comparePartMaxRangeMaxUsingHash( qmsPartCondValList * aMaxPartCondVal,
                                                 UInt                 aMaxHashVal );

    static SInt comparePartGEUsingHash( qmsPartCondValList * aMinPartCondVal,
                                        qmsPartCondValList * aMaxPartCondVal,
                                        UInt                 aMinHashVal );

    static SInt comparePartLEUsingHash( qmsPartCondValList * aMinPartCondVal,
                                        qmsPartCondValList * aMaxPartCondVal,
                                        UInt                 aMaxHashVal );

    static void partitionFilterGEUsingHash( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                            UInt                   * aRangeIntersectCountArray,
                                            UInt                     aMinHashVal,
                                            UInt                     aPartCount );

    static void partitionFilterLEUsingHash( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                            UInt                   * aRangeIntersectCountArray,
                                            UInt                     aMaxHashVal,
                                            UInt                     aPartCount );

    static IDE_RC makeHashKeyForRangeFilter( qcStatement        * aStatement,
                                             mtkRangeCallBack   * aData,
                                             UInt               * aHashValue );
};

#endif /* _O_QMO_PARTITION_H_ */

