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
 * $Id: qmoOneMtrPlan.h 82330 2018-02-23 00:32:59Z donovan.seo $
 *
 * Description :
 *     Plan Generator
 *
 *     One-child Materialized Plan을 생성하기 위한 관리자이다.
 *
 *     다음과 같은 Plan Node의 생성을 관리한다.
 *         - SORT 노드
 *         - HASH 노드
 *         - GRAG 노드
 *         - HSDS 노드
 *         - LMST 노드
 *         - VMTR 노드
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_ONE_CHILD_MATER_H_
#define _O_QMO_ONE_CHILD_MATER_H_ 1

#include <qc.h>
#include <qmoDef.h>
#include <qmn.h>
#include <qmgDef.h>
#include <qmoDependency.h>
#include <qmoPredicate.h>
#include <qmoNormalForm.h>
#include <qmoSubquery.h>
#include <qmoOneNonPlan.h>
//---------------------------------------------------
// One-Child Meterialized Plan을 관리하기 위한 자료 구조
//---------------------------------------------------

//----------------------------------------
//SORT 노드의 dependency 호출을 위한 flag
//----------------------------------------
#define QMO_SORT_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_TRUE   )

//----------------------------------------
//HASH 노드의 dependency 호출을 위한 flag
//----------------------------------------
#define QMO_HASH_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_TRUE   )

//----------------------------------------
//GRAG 노드의 dependency 호출을 위한 flag
//----------------------------------------
#define QMO_GRAG_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE  )
//----------------------------------------
//HSDS 노드의 dependency 호출을 위한 flag
//----------------------------------------
#define QMO_HSDS_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE  )

//----------------------------------------
//VMTR 노드의 dependency 호출을 위한 flag
//----------------------------------------
#define QMO_LMST_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE  )

//----------------------------------------
//VMTR 노드의 dependency 호출을 위한 flag
//----------------------------------------
#define QMO_VMTR_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE  )

//----------------------------------------
//WNST 노드의 dependency 호출을 위한 flag
//----------------------------------------
#define QMO_WNST_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE  )

/* Proj-1715 CNBY노드의 dependency flag */
#define QMO_CNBY_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE     |  \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_TRUE        |  \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE     |  \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE          |  \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE  |  \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE )

/* PROJ-1715 CMTR노드의 dependency flag */
#define QMO_CMTR_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE  )

/* PROJ-2509 CNBY JOIN 노드의 dependency flag */
#define QMO_CNBY_DEPENDENCY_FOR_JOIN ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE | \
                                       QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE | \
                                       QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE | \
                                       QMO_DEPENDENCY_STEP2_SETNODE_FALSE | \
                                       QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE | \
                                       QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE )

/* PROJ-1353 ROLLUP dependency flag */
#define QMO_ROLL_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_TRUE  )

/* PROJ-1353 CUBE dependency flag */
#define QMO_CUBE_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE       |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE            |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_TRUE  )

//------------------------------
// makeWNST()함수에 필요한 flag
//------------------------------

// WNST 노드 생성시 BaseTable 저장하는 방법
#define QMO_MAKEWNST_BASETABLE_MASK              (0x0000000F)
#define QMO_MAKEWNST_BASETABLE_DEPENDENCY        (0x00000000)
#define QMO_MAKEWNST_BASETABLE_TARGET            (0x00000001)
#define QMO_MAKEWNST_BASETABLE_GRAG              (0x00000002)
#define QMO_MAKEWNST_BASETABLE_HSDS              (0x00000003)

//Temp Table의 생성 종류
#define QMO_MAKEWNST_TEMP_TABLE_MASK             (0x00000010)
#define QMO_MAKEWNST_MEMORY_TEMP_TABLE           (0x00000000)
#define QMO_MAKEWNST_DISK_TEMP_TABLE             (0x00000010)

// Preserved Order
#define QMO_MAKEWNST_PRESERVED_ORDER_MASK        (0x00000020)
#define QMO_MAKEWNST_PRESERVED_ORDER_TRUE        (0x00000000)
#define QMO_MAKEWNST_PRESERVED_ORDER_FALSE       (0x00000020)

// BUG-37277
// 상위에 order by가 있고 하위에 group by가 있는 window절
#define QMO_MAKEWNST_ORDERBY_GROUPBY_MASK        (0x00000040)
#define QMO_MAKEWNST_ORDERBY_GROUPBY_FALSE       (0x00000000)
#define QMO_MAKEWNST_ORDERBY_GROUPBY_TRUE        (0x00000040)

/* BUG-40354 pushed rank */
/* pushed rank 함수가 적용되었음을 표시 */
#define QMO_MAKEWNST_PUSHED_RANK_MASK            (0x00000080)
#define QMO_MAKEWNST_PUSHED_RANK_FALSE           (0x00000000)
#define QMO_MAKEWNST_PUSHED_RANK_TRUE            (0x00000080)

//------------------------------
//makeSORT()함수에 필요한 flag
//------------------------------

//SORT노드의 용도 구분
#define QMO_MAKESORT_METHOD_MASK                 (0x0000000F)
#define QMO_MAKESORT_ORDERBY                     (0x00000000)
#define QMO_MAKESORT_SORT_BASED_GROUPING         (0x00000001)
#define QMO_MAKESORT_SORT_BASED_DISTINCTION      (0x00000002)
#define QMO_MAKESORT_SORT_BASED_JOIN             (0x00000003)
#define QMO_MAKESORT_SORT_MERGE_JOIN             (0x00000004)
#define QMO_MAKESORT_STORE_AND_SEARCH            (0x00000005)
#define QMO_MAKESORT_STORE                       (0x00000006)

//JOIN아래의 생성 위치 구분
#define QMO_MAKESORT_POSITION_MASK               (0x00000010)
#define QMO_MAKESORT_POSITION_LEFT               (0x00000000)
#define QMO_MAKESORT_POSITION_RIGHT              (0x00000010)

//Temp Table의 생성 종류
#define QMO_MAKESORT_TEMP_TABLE_MASK             (0x00000020)
#define QMO_MAKESORT_MEMORY_TEMP_TABLE           (0x00000000)
#define QMO_MAKESORT_DISK_TEMP_TABLE             (0x00000020)

//Preserverd Order의 존재 유무
#define QMO_MAKESORT_PRESERVED_ORDER_MASK        (0x00000040)
#define QMO_MAKESORT_PRESERVED_FALSE             (0x00000000)
#define QMO_MAKESORT_PRESERVED_TRUE              (0x00000040)

// PROJ-1353 Temp에 value를 쌓는 Store용 Sort
#define QMO_MAKESORT_VALUE_TEMP_MASK             (0x00000080)
#define QMO_MAKESORT_VALUE_TEMP_FALSE            (0x00000000)
#define QMO_MAKESORT_VALUE_TEMP_TRUE             (0x00000080)

//SORT노드 생성시 BaseTable 저장하는 방법
#define QMO_MAKESORT_BASETABLE_MASK              (0x00000F00)
#define QMO_MAKESORT_BASETABLE_DEPENDENCY        (0x00000000)
#define QMO_MAKESORT_BASETABLE_TARGET            (0x00000100)
#define QMO_MAKESORT_BASETABLE_GRAG              (0x00000200)
#define QMO_MAKESORT_BASETABLE_HSDS              (0x00000300)
#define QMO_MAKESORT_BASETABLE_WNST              (0x00000400)

/* BUG-37281 */
#define QMO_MAKESORT_GROUP_EXT_VALUE_MASK        (0x00001000)
#define QMO_MAKESORT_GROUP_EXT_VALUE_FALSE       (0x00000000)
#define QMO_MAKESORT_GROUP_EXT_VALUE_TRUE        (0x00001000)

//------------------------------
//makeHASH()함수에 필요한 flag
//------------------------------

//HASH노드의 용도 구분
#define QMO_MAKEHASH_METHOD_MASK                 (0x00000001)
#define QMO_MAKEHASH_HASH_BASED_JOIN             (0x00000000)
#define QMO_MAKEHASH_STORE_AND_SEARCH            (0x00000001)

//JOIN아래의 생성 위치 구분
#define QMO_MAKEHASH_POSITION_MASK               (0x00000002)
#define QMO_MAKEHASH_POSITION_LEFT               (0x00000000)
#define QMO_MAKEHASH_POSITION_RIGHT              (0x00000002)

//Temp Table의 생성 종류
#define QMO_MAKEHASH_TEMP_TABLE_MASK             (0x00000004)
#define QMO_MAKEHASH_MEMORY_TEMP_TABLE           (0x00000000)
#define QMO_MAKEHASH_DISK_TEMP_TABLE             (0x00000004)

//Store and Search로 사용될 경우 not null 검사여부
#define QMO_MAKEHASH_NOTNULLCHECK_MASK           (0x00000008)
#define QMO_MAKEHASH_NOTNULLCHECK_FALSE          (0x00000000)
#define QMO_MAKEHASH_NOTNULLCHECK_TRUE           (0x00000008)


//------------------------------
//makeGRAG()함수에 필요한 flag
//------------------------------

#define QMO_MAKEGRAG_TEMP_TABLE_MASK             (0x00000001)
#define QMO_MAKEGRAG_MEMORY_TEMP_TABLE           (0x00000000)
#define QMO_MAKEGRAG_DISK_TEMP_TABLE             (0x00000001)

// PROJ-2444
#define QMO_MAKEGRAG_PARALLEL_STEP_MASK          (0x000000F0)
#define QMO_MAKEGRAG_NOPARALLEL                  (0x00000000)
#define QMO_MAKEGRAG_PARALLEL_STEP_AGGR          (0x00000010)
#define QMO_MAKEGRAG_PARALLEL_STEP_MERGE         (0x00000020)

//------------------------------
//makeHSDS()함수에 필요한 flag
//------------------------------

//HSDS노드의 용도 구분
#define QMO_MAKEHSDS_METHOD_MASK                 (0x0000000F)
#define QMO_MAKEHSDS_HASH_BASED_DISTINCTION      (0x00000000)
#define QMO_MAKEHSDS_SET_UNION                   (0x00000001)
#define QMO_MAKEHSDS_IN_SUBQUERY_KEYRANGE        (0x00000002)

//Temp Table의 생성 종류
#define QMO_MAKEHSDS_TEMP_TABLE_MASK             (0x00000010)
#define QMO_MAKEHSDS_MEMORY_TEMP_TABLE           (0x00000000)
#define QMO_MAKEHSDS_DISK_TEMP_TABLE             (0x00000010)


//------------------------------
//makeLMST()함수에 필요한 flag
//------------------------------

//LMST노드의 용도 구분
#define QMO_MAKELMST_METHOD_MASK                 (0x00000001)
#define QMO_MAKELMST_LIMIT_ORDERBY               (0x00000000)
#define QMO_MAKELMST_STORE_AND_SEARCH            (0x00000001)

//SORT노드 생성시 BaseTable 저장하는 방법
#define QMO_MAKELMST_BASETABLE_MASK          QMO_MAKESORT_BASETABLE_MASK
#define QMO_MAKELMST_BASETABLE_DEPENDENCY    QMO_MAKESORT_BASETABLE_DEPENDENCY
#define QMO_MAKELMST_BASETABLE_TARGET        QMO_MAKESORT_BASETABLE_TARGET
#define QMO_MAKELMST_BASETABLE_GRAG          QMO_MAKESORT_BASETABLE_GRAG
#define QMO_MAKELMST_BASETABLE_HSDS          QMO_MAKESORT_BASETABLE_HSDS
#define QMO_MAKELMST_BASETABLE_WNST          QMO_MAKESORT_BASETABLE_WNST

//------------------------------
//makeVMTR()함수에 필요한 flag
//------------------------------

//Temp Table의 생성 종류
#define QMO_MAKEVMTR_TEMP_TABLE_MASK             (0x00000010)
#define QMO_MAKEVMTR_MEMORY_TEMP_TABLE           (0x00000000)
#define QMO_MAKEVMTR_DISK_TEMP_TABLE             (0x00000010)

// PROJ-2462 ResultCache
#define QMO_MAKEVMTR_TOP_RESULT_CACHE_MASK       (0x00000020)
#define QMO_MAKEVMTR_TOP_RESULT_CACHE_FALSE      (0x00000000)
#define QMO_MAKEVMTR_TOP_RESULT_CACHE_TRUE       (0x00000020)

/* PROJ-1353 ROLLUP, CUBE */
#define QMO_MAX_CUBE_COUNT                       (15)
#define QMO_MAX_CUBE_COLUMN_COUNT                (60)

//---------------------------------------------------
// One-Child Materialized Plan을 관리하기 위한 함수
//---------------------------------------------------

class qmoOneMtrPlan
{
public:
    // SORT 노드의 생성
    static IDE_RC    initSORT( qcStatement     * aStatement ,
                               qmsQuerySet     * aQuerySet ,
                               UInt              aFlag ,
                               qmsSortColumns  * aOrderBy ,
                               qmnPlan         * aParent,
                               qmnPlan        ** aPlan );

    static IDE_RC    initSORT( qcStatement  * aStatement ,
                               qmsQuerySet  * /*aQuerySet*/ ,
                               qmnPlan      * aParent,
                               qmnPlan     ** aPlan );

    static IDE_RC    initSORT( qcStatement  * aStatement ,
                               qmsQuerySet  * /*aQuerySet*/ ,
                               qtcNode      * aRange,
                               idBool         aIsLeftSort,
                               idBool         aIsRangeSearch,
                               qmnPlan      * aParent,
                               qmnPlan     ** aPlan );

    static IDE_RC    makeSORT( qcStatement       * aStatement ,
                               qmsQuerySet       * aQuerySet ,
                               UInt                aFlag,
                               qmgPreservedOrder * aChildPreservedOrder ,
                               qmnPlan           * aChildPlan ,
                               SDouble             aStoreRowCount,
                               qmnPlan           * aPlan );

    // HASH 노드의 생성
    static IDE_RC    initHASH( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               qtcNode      * aHashFilter,
                               idBool         aIsLeftHash,
                               idBool         aDistinction,
                               qmnPlan      * aParent,
                               qmnPlan     ** aPlan );

    static IDE_RC    initHASH( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               qmnPlan      * aParent,
                               qmnPlan     ** aPlan );

    // PROJ-2385
    // aAllAttrToKey : Filtering이 끝난 모든 Attr를 HashKey로 만들지 여부
    static IDE_RC    makeHASH( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               UInt           aFlag ,
                               UInt           aTempTableCount ,
                               UInt           aBucketCount ,
                               qtcNode      * aFilter ,
                               qmnPlan      * aChildPlan ,
                               qmnPlan      * aPlan,
                               idBool         aAllAttrToKey );

    // GRAG 노드의 생성
    static IDE_RC    initGRAG( qcStatement       * aStatement ,
                               qmsQuerySet       * aQuerySet ,
                               qmsAggNode        * aAggrNode ,
                               qmsConcatElement  * aGroupColumn,
                               qmnPlan           * aParent,
                               qmnPlan          ** aPlan );

    static IDE_RC    makeGRAG( qcStatement      * aStatement ,
                               qmsQuerySet      * aQuerySet ,
                               UInt               aFlag,
                               qmsConcatElement * aGroupColumn,
                               UInt               aBucketCount ,
                               qmnPlan          * aChildPlan ,
                               qmnPlan          * aPlan );

    // HSDS 노드의 생성
    static IDE_RC    initHSDS( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               idBool         aExplicitDistinct,
                               qmnPlan      * aParent,
                               qmnPlan     ** aPlan );

    static IDE_RC    makeHSDS( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               UInt           aFlag ,
                               UInt           aBucketCount ,
                               qmnPlan      * aChildPlan ,
                               qmnPlan      * aPlan );

    // LMST 노드의 생성
    static IDE_RC    initLMST( qcStatement    * aStatement ,
                               qmsQuerySet    * aQuerySet ,
                               UInt             aFlag ,
                               qmsSortColumns * aOrderBy ,
                               qmnPlan        * aParentPlan ,
                               qmnPlan       ** aPlan );

    static IDE_RC    initLMST( qcStatement    * aStatement ,
                               qmsQuerySet    * aQuerySet ,
                               qmnPlan        * aParentPlan ,
                               qmnPlan       ** aPlan );

    static IDE_RC    makeLMST( qcStatement    * aStatement ,
                               qmsQuerySet    * aQuerySet ,
                               ULong            aLimitRowCount ,
                               qmnPlan        * aChildPlan ,
                               qmnPlan        * aPlan );

    // VMTR 노드의 생성
    static IDE_RC    initVMTR( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               qmnPlan      * aParent ,
                               qmnPlan     ** aPlan );

    static IDE_RC    makeVMTR( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               UInt           aFlag ,
                               qmnPlan      * aChildPlan ,
                               qmnPlan      * aPlan );

    // WNST 노드의 생성
    static IDE_RC    initWNST( qcStatement        * aStatement,
                               qmsQuerySet        * aQuerySet,
                               UInt                 aSortKeyCount,
                               qmsAnalyticFunc   ** aAnalFuncListPtrArr,
                               UInt                 aFlag,
                               qmnPlan            * aParent,
                               qmnPlan           ** aPlan );

    static IDE_RC    makeWNST( qcStatement          * aStatement,
                               qmsQuerySet          * aQuerySet,
                               UInt                   aFlag,
                               qmoDistAggArg        * aDistAggArg,
                               UInt                   aSortKeyCount,
                               qmgPreservedOrder   ** aSortKeyPtrArr,
                               qmsAnalyticFunc     ** aAnalFuncListPtrArr,
                               qmnPlan              * aChildPlan,
                               SDouble                aStoreRowCount,
                               qmnPlan              * aPlan );

    /* Proj-1715 Hierarchy Node 생성 */
    static IDE_RC    initCNBY( qcStatement    * aStatement,
                               qmsQuerySet    * aQuerySet,
                               qmoLeafInfo    * aLeafInfo,
                               qmsSortColumns * aSiblings,
                               qmnPlan        * aParent,
                               qmnPlan       ** aPlan );

    static IDE_RC    makeCNBY( qcStatement    * aStatement,
                               qmsQuerySet    * aQuerySet,
                               qmsFrom        * aFrom,
                               qmoLeafInfo    * aStartWith,
                               qmoLeafInfo    * aConnectBy,
                               qmnPlan        * aChildPlan,
                               qmnPlan        * aPlan );

    /* PROJ-2509 Hierarchy Query Join*/
    static IDE_RC initCNBYForJoin( qcStatement    * aStatement,
                                   qmsQuerySet    * aQuerySet,
                                   qmoLeafInfo    * aLeafInfo,
                                   qmsSortColumns * aSiblings,
                                   qmnPlan        * aParent,
                                   qmnPlan       ** aPlan );

    /* PROJ-2509 Hierarchy Query Join*/
    static IDE_RC makeCNBYForJoin( qcStatement    * aStatement,
                                   qmsQuerySet    * aQuerySet,
                                   qmsSortColumns * aSiblings,
                                   qmoLeafInfo    * aStartWith,
                                   qmoLeafInfo    * aConnectBy,
                                   qmnPlan        * aChildPlan,
                                   qmnPlan        * aPlan );

    /* Proj-1715 Hierarchy 쿼리에서 사용되는 Materialize 노드 */
    static IDE_RC    initCMTR( qcStatement  * aStatement,
                               qmsQuerySet  * aQuerySet,
                               qmnPlan     ** aPlan );

    static IDE_RC    makeCMTR( qcStatement  * aStatement,
                               qmsQuerySet  * aQuerySet,
                               UInt           aFlag,
                               qmnPlan      * aChildPlan,
                               qmnPlan      * aPlan );

    /* PROJ-1353 Group By Rollup */
    static IDE_RC    initROLL( qcStatement      * aStatement,
                               qmsQuerySet      * aQuerySet,
                               UInt             * aRollupCount,
                               qmsAggNode       * aAggrNode,
                               qmsConcatElement * aGroupColumn,
                               qmnPlan         ** aPlan );
    /* PROJ-1353 Group By Rollup */
    static IDE_RC    makeROLL( qcStatement      * aStatement,
                               qmsQuerySet      * aQuerySet,
                               UInt               aFlag,
                               UInt               aRollupCount,
                               qmsAggNode       * aAggrNode,
                               qmoDistAggArg    * aDistAggArg,
                               qmsConcatElement * aGroupColumn,
                               qmnPlan          * aChildPlan,
                               qmnPlan          * aPlan );

    /* PROJ-1353 Group By Cube */
    static IDE_RC    initCUBE( qcStatement      * aStatement,
                               qmsQuerySet      * aQuerySet,
                               UInt             * aCubeCount,
                               qmsAggNode       * aAggrNode,
                               qmsConcatElement * aGroupColumn,
                               qmnPlan         ** aPlan );
    /* PROJ-1353 Group By Cube */
    static IDE_RC    makeCUBE( qcStatement      * aStatement,
                               qmsQuerySet      * aQuerySet,
                               UInt               aFlag,
                               UInt               aCubeCount,
                               qmsAggNode       * aAggrNode,
                               qmoDistAggArg    * aDistAggArg,
                               qmsConcatElement * aGroupColumn,
                               qmnPlan          * aChildPlan ,
                               qmnPlan          * aPlan );
private:


    //-----------------------------------------
    // SORT 노드 생성을 위한 함수
    //-----------------------------------------

    //-----------------------------------------
    // HASH 노드 생성을 위한 함수
    //-----------------------------------------

    //HASH가 IN subquery로 쓰이는 경우 Filter를 새롭게 생성한다.
    static IDE_RC    makeFilterINSubquery( qcStatement  * aStatement ,
                                           qmsQuerySet  * aQuerySet ,
                                           UShort         aTupleID,
                                           qtcNode      * aInFilter ,
                                           qtcNode     ** aChangedFilter );

    // 주어진 filter로 부터 filterConst를 구성한다.
    static IDE_RC    makeFilterConstFromPred(  qcStatement  * aStatement ,
                                               qmsQuerySet  * aQuerySet,
                                               UShort         aTupleID,
                                               qtcNode      * aFilter ,
                                               qtcNode     ** aFilterConst );

    // 주어진 Operator 노드로 부터 컬럼을 찾아
    // filterConst가 될 노드를 복사해서 준다
    static IDE_RC makeFilterConstFromNode( qcStatement  * aStatement ,
                                           qmsQuerySet  * aQuerySet,
                                           UShort         aTupleID,
                                           qtcNode      * aOperatorNode ,
                                           qtcNode     ** aNewNode );

    //-----------------------------------------
    // WNST 노드 생성을 위한 함수 ( PROJ-1762 ) 
    //-----------------------------------------
    
    // 동일 partition by를 가진 wndNode 존재하는지 검사 
    static IDE_RC existSameWndNode( qcStatement       * aStatement,
                                    UShort              aTupleID,
                                    qmcWndNode        * aWndNode,
                                    qtcNode           * aAnalticFuncNode,
                                    qmcWndNode       ** aSameWndNode );

    // wndNode에 aggrNode를 생성하여 add 
    static IDE_RC addAggrNodeToWndNode( qcStatement     * aStatement,
                                        qmsQuerySet     * aQuerySet,
                                        qmsAnalyticFunc * aAnalyticFunc,
                                        UShort            aAggrTupleID,
                                        UShort          * aAggrColumnCount,
                                        qmcWndNode      * aWndNode,
                                        qmcMtrNode     ** aNewAggrNode );

    // wndNode에 aggrResultNode를 생성하여 add
    static IDE_RC
    addAggrResultNodeToWndNode( qcStatement * aStatement,
                                qmcMtrNode  * aAnalResultFuncMtrNode,
                                qmcWndNode  * aWndNode );

    // WndNode를 생성함 
    static IDE_RC makeWndNode( qcStatement       * aStatement,
                               UShort              aTupleID,
                               qmcMtrNode        * aMtrNode,
                               qtcNode           * aAnalticFuncNode,
                               UInt              * aOverColumnNodeCount,
                               qmcWndNode       ** aNewWndNode );

    // WNST의 myNode를 생성 
    static IDE_RC makeMyNodeOfWNST( qcStatement      * aStatement,
                                    qmsQuerySet      * aQuerySet,
                                    qmnPlan          * aPlan,
                                    qmsAnalyticFunc ** aAnalFuncListPtrArr,
                                    UInt               aAnalFuncListPtrCnt,
                                    UShort             aTupleID,
                                    qmncWNST         * aWNST,
                                    UShort           * aColumnCountOfMyNode,
                                    qmcMtrNode      ** aFirstAnalResultFuncMtrNode);
    
    /* PROJ-1715 Hierarcy Pseudo 컬럼 RowID Set */
    static IDE_RC setPseudoColumnRowID( qtcNode * aNode, UShort * aRowID );

    /* PROJ-1715 Start with Predicate 처리 */
    static IDE_RC processStartWithPredicate( qcStatement * aStatement,
                                             qmsQuerySet * aQuerySet,
                                             qmncCNBY    * aCNBY,
                                             qmoLeafInfo * aStartWith );

    /* PROJ-1715 Connect By Predicate 처리 */
    static IDE_RC processConnectByPredicate( qcStatement    * aStatement,
                                             qmsQuerySet    * aQuerySet,
                                             qmsFrom        * aFrom,
                                             qmncCNBY       * aCNBY,
                                             qmoLeafInfo    * aConnectBy );

    /* PROJ-1715 */
    static IDE_RC findPriorColumns( qcStatement * aStatement,
                                    qtcNode     * aNode,
                                    qtcNode     * aFirst );

    /* PROJ-1715 Prior 노드와 이와 연결된 Sort Node 찾기 */
    static IDE_RC findPriorPredAndSortNode( qcStatement  * aStatement,
                                            qtcNode      * aNode,
                                            qtcNode     ** aSortNode,
                                            qtcNode     ** aPriorPred,
                                            UShort         aFromTable,
                                            idBool       * aFind );

    static IDE_RC makeSortNodeOfCNBY( qcStatement * aStatement,
                                      qmsQuerySet * aQuerySet,
                                      qmncCNBY    * aCNBY,
                                      qtcNode     * aSortNode );

    /* PROJ-2179 Non-key attribute들에 대하여 mtr node들을 생성 */
    static IDE_RC makeNonKeyAttrsMtrNodes( qcStatement  * aStatement,
                                           qmsQuerySet  * aQuerySet,
                                           qmcAttrDesc  * aResultDesc,
                                           UShort         aTupleID,
                                           qmcMtrNode  ** aFirstMtrNode,
                                           qmcMtrNode  ** aLastMtrNode,
                                           UShort       * aMtrNodeCount );

    /* PROJ-2179 Join predicate으로부터 mtr node들을 생성 */
    static IDE_RC appendJoinPredicate( qcStatement  * aStatement,
                                       qmsQuerySet  * aQuerySet,
                                       qtcNode      * aJoinPredicate,
                                       idBool         aIsLeft,
                                       idBool         aAllowDup,
                                       qmcAttrDesc ** aResultDesc );

    /* PROJ-1353 Temp 테이블에 Value를 쌓기위한 Store용 Sort 생성*/
    static IDE_RC makeValueTempMtrNode( qcStatement * aStatement,
                                        qmsQuerySet * aQuerySet,
                                        UInt          aFlag,
                                        qmnPlan     * aPlan,
                                        qmcMtrNode ** aFirstNode,
                                        UShort      * aColumnCount );

    /* PROJ-1353 Aggregation의 인자에 사용되는 MTR Node 생성 */
    static IDE_RC makeAggrArgumentsMtrNode( qcStatement * aStatement,
                                            qmsQuerySet * aQuerySet,
                                            UShort        aTupleID,
                                            UShort      * aColumnCount,
                                            mtcNode     * aNode,
                                            qmcMtrNode ** aFirstMtrNode );

    static IDE_RC appendJoinColumn( qcStatement  * aStatement,
                                    qmsQuerySet  * aQuerySet,
                                    qtcNode      * aColumnNode,
                                    UInt           aFlag,
                                    UInt           aAppendOption,
                                    qmcAttrDesc ** aResultDesc );

    /* PROJ-2509 Connect By Predicate 처리 */
    static IDE_RC processConnectByPredicateForJoin( qcStatement    * aStatement,
                                                    qmsQuerySet    * aQuerySet,
                                                    qmncCNBY       * aCNBY,
                                                    qmoLeafInfo    * aConnectBy );

    static void changeNodeForCNBYJoin( qtcNode * aNode,
                                       UShort    aSrcTable,
                                       UShort    aSrcColumn,
                                       UShort    aDstTable,
                                       UShort    aDstColumn );
};

#endif /* _O_QMO_ONE_CHILD_MATER_H_ */
