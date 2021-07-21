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
 * $Id: qmoMultiNonPlan.h 90438 2021-04-02 08:20:57Z ahra.cho $
 *
 * Description :
 *     Plan Generator
 *
 *     Multi-child Non-materialized Plan을 생성하기 위한 관리자이다.
 *
 *     다음과 같은 Plan Node의 생성을 관리한다.
 *         - MultiBUNI 노드
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_MULTI_CHILD_NON_MATER_H_
#define _O_QMO_MULTI_CHILD_NON_MATER_H_ 1

#include <qc.h>
#include <qmoDef.h>
#include <qmn.h>
#include <qmgDef.h>
#include <qmoDependency.h>
#include <qmoPredicate.h>
#include <qmoNormalForm.h>
#include <qmoSubquery.h>

//------------------------------
// MULTI_BUNI노드의 dependency를 호출을 위한 flag
//------------------------------
#define QMO_MULTI_BUNI_DEPENDENCY                       \
    ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_FALSE     |    \
      QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE        |    \
      QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE      |    \
      QMO_DEPENDENCY_STEP2_SETNODE_TRUE            |    \
      QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE   |    \
      QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE )

//------------------------------
// PCRD노드의 dependency를 호출을 위한 flag
//------------------------------
#define QMO_PCRD_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE     |     \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_TRUE        |     \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE     |     \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE          |     \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE  |     \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE )

//------------------------------
// MRGE노드의 dependency를 호출을 위한 flag
//------------------------------
#define QMO_MRGE_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_FALSE    |     \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE       |     \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE     |     \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE          |     \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE  |     \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE )

//------------------------------
// MTIT노드의 dependency를 호출을 위한 flag
//------------------------------
#define QMO_MTIT_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_FALSE     |    \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE        |    \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE      |    \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE           |    \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE   |    \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE )

//------------------------------
// qmoPCRDInfo.flag
//------------------------------
#define QMO_PCRD_INFO_INITIALIZE               (0x00000000)

#define QMO_PCRD_INFO_NOTNULL_KEYRANGE_MASK    (0x00000001)
#define QMO_PCRD_INFO_NOTNULL_KEYRANGE_FALSE   (0x00000000)
#define QMO_PCRD_INFO_NOTNULL_KEYRANGE_TRUE    (0x00000001)

// PROJ-1502 PARTITIONED DISK TABLE
// partition-coordinator 노드를 위한 입력 정보
typedef struct qmoPCRDInfo
{
    UInt                flag;
    qmoPredicate      * predicate;
    qmoPredicate      * constantPredicate;
    qcmIndex          * index;
    qmgPreservedOrder * preservedOrder;
    idBool              forceIndexScan;
    qtcNode           * nnfFilter;
    qtcNode           * partKeyRange;
    qmoPredicate      * partFilterPredicate;
    UInt                selectedPartitionCount;
    qmgChildren       * childrenGraph;
    const void        * mPrePruningPartHandle;  // BUG-48800 partition Table Handle
    smSCN               mPrePruningPartSCN;     // BUG-48800 partition Table SCN
} qmoPCRDInfo;

// PROJ-2205 rownum in DML
// mrege operator 노드를 위한 입력 정보
typedef struct qmoMRGEInfo
{
    qmsTableRef       * tableRef;
    
    qcStatement       * selectSourceStatement;
    qcStatement       * selectTargetStatement;
    
    qcStatement       * updateStatement;
    qcStatement       * deleteStatement;
    qcStatement       * insertStatement;
    qcStatement       * insertNoRowsStatement;
    
    qtcNode           * whereForInsert;
    
    UInt                resetPlanFlagStartIndex;
    UInt                resetPlanFlagEndIndex;
    UInt                resetExecInfoStartIndex;
    UInt                resetExecInfoEndIndex;
    
} qmoMRGEInfo;

//---------------------------------------------------
// Multi-Child Non-Materialized Plan을 관리하기 위한 함수
//---------------------------------------------------

class qmoMultiNonPlan
{
public:

    // MultiBUNI 노드의 생성
    static IDE_RC    initMultiBUNI( qcStatement  * aStatement ,
                                    qmsQuerySet  * aQuerySet ,
                                    qmnPlan      * aParent,
                                    qmnPlan     ** aPlan );

    static IDE_RC    makeMultiBUNI( qcStatement  * aStatement ,
                                    qmsQuerySet  * aQuerySet ,
                                    qmgChildren  * aChildrenGraph,
                                    qmnPlan      * aPlan );

    // PCRD 노드의 생성
    static IDE_RC    initPCRD( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               qmnPlan      * aParent,
                               qmnPlan     ** aPlan );

    static IDE_RC    makePCRD( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               qmsFrom      * aFrom ,
                               qmoPCRDInfo  * aPCRDInfo,
                               qmnPlan      * aPlan );

    // PROJ-1071 Parallel query
    // PPCRD 노드의 생성
    static IDE_RC initPPCRD( qcStatement  * aStatement ,
                             qmsQuerySet  * aQuerySet,
                             qmnPlan      * aParent,
                             qmnPlan     ** aPlan );

    static IDE_RC makePPCRD( qcStatement  * aStatement ,
                             qmsQuerySet  * aQuerySet ,
                             qmsFrom      * aFrom ,
                             qmoPCRDInfo  * aPCRDInfo,
                             qmnPlan      * aPlan );

    // PROJ-2205 rownum in DML
    // MRGE 노드의 생성
    static IDE_RC    initMRGE( qcStatement  * aStatement ,
                               qmnPlan     ** aPlan );
    
    static IDE_RC    makeMRGE( qcStatement  * aStatement ,
                               qmoMRGEInfo  * aMRGEInfo,
                               qmgChildren  * aChildrenGraph,
                               qmnPlan      * aPlan );
    
    // BUG-36596 multi-table insert
    static IDE_RC    initMTIT( qcStatement  * aStatement ,
                               qmnPlan     ** aPlan );
    
    static IDE_RC    makeMTIT( qcStatement  * aStatement ,
                               qmgChildren  * aChildrenGraph,
                               qmnPlan      * aPlan );
    
private:

    static IDE_RC    processPredicate( qcStatement   * aStatement ,
                                       qmsQuerySet   * aQuerySet ,
                                       qmsFrom       * aFrom,
                                       qmoPredicate  * aPredicate ,
                                       qmoPredicate  * aConstantPredicate ,
                                       qtcNode      ** aConstantFilter ,
                                       qtcNode      ** aSubqueryFilter ,
                                       qtcNode      ** aPartitionFilter ,
                                       qtcNode      ** aRemain );

    static IDE_RC    checkSimplePCRD( qcStatement * aStatement,
                                      qmncPCRD    * aPCRD );
};

#endif /* _O_QMO_MULTI_CHILD_NON_MATER_H_ */
