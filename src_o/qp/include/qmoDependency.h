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
 * $Id: qmoDependency.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Plan Dependency Manager
 *
 *     각 Plan Node 생성 시 Plan 간의 관계를 고려하여,
 *     해당 Plan의 Dependency를 설정하는 절차를 제공한다.
 *     아래와 같이 총 6단계의 절차를 통해 Plan의 Depenendency가 결정된다.
 *
 *     - 1단계 : Set Tuple ID
 *     - 2단계 : Dependencies 생성
 *     - 3단계 : Table Map 보정
 *     - 4단계 : Dependency 결정
 *     - 5단계 : Dependency 보정
 *     - 6단계 : Dependencies 보정
 *
 *    주의)  각 Plan 노드에서 Subquery를 포함할 경우,
 *           Subquery에 대한 Plan Tree 생성은 2단계와 3단계 사이에서
 *           처리되어야 한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_PLAN_DEPENDENCY_H_
#define _O_QMO_PLAN_DEPENDENCY_H_ 1

#include <qc.h>
#include <qmoCnfMgr.h>
#include <qmoCrtPathMgr.h>

//---------------------------------------------------
// Plan Dependency 를 관리하기 위한 자료 구조
//---------------------------------------------------

// Plan Node 가 처리 해야 하는 dependency 종류
#define QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_MASK        (0x00000010) 
#define QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE        (0x00000000) 
#define QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_FALSE       (0x00000010) 

#define QMO_DEPENDENCY_STEP2_BASE_TABLE_MASK           (0x00000020) 
#define QMO_DEPENDENCY_STEP2_BASE_TABLE_TRUE           (0x00000000) 
#define QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE          (0x00000020) 

#define QMO_DEPENDENCY_STEP2_DEP_MASK                  (0x00000040) 
#define QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE        (0x00000000) 
#define QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION  (0x00000040) 

#define QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_MASK      (0x00000080) 
#define QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE      (0x00000000) 
#define QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE     (0x00000080) 

#define QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_MASK  (0x00000100) 
#define QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_TRUE  (0x00000000) 
#define QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE (0x00000100) 

#define QMO_DEPENDENCY_STEP2_SETNODE_MASK              (0x00000200)
#define QMO_DEPENDENCY_STEP2_SETNODE_FALSE             (0x00000000)
#define QMO_DEPENDENCY_STEP2_SETNODE_TRUE              (0x00000200)

//---------------------------------------------------
// Plan Dependency 를 관리하기 위한 함수
//---------------------------------------------------

class qmoDependency
{
public:
    
    // 해당 Plan의 Dependency, Dependencies, Subquery의 Plan을 생성
    static IDE_RC    setDependency( qcStatement * aStatement,
                                    qmsQuerySet * aQuerySet,
                                    qmnPlan     * aPlan,
                                    UInt          aDependencyFlag,
                                    UShort        aTupleID,
                                    qmsTarget   * aTarget,
                                    UInt          aPredCount,
                                    qtcNode    ** aPredExpr,
                                    UInt          aMtrCount,
                                    qmcMtrNode ** aMtrNode );
    
    // Dependency 설정 1단계
    static IDE_RC    step1setTupleID( qcStatement * aStatement ,
                                      UShort        aTupleID );
    
    // Dependency 설정 2단계
    static IDE_RC    step2makeDependencies( qmnPlan     * aPlan ,
                                            UInt          aDependencyFlag,
                                            UShort        aTupleID ,
                                            qcDepInfo   * aDependencies );
    
    // Dependency 설정 3단계
    static IDE_RC    step3refineTableMap( qcStatement * aStatement ,
                                          qmnPlan     * aPlan ,
                                          qmsQuerySet * aQuerySet ,
                                          UShort        aTupleID );

    // Dependency 설정 4-5단계
    static IDE_RC    step4decideDependency( qcStatement * aStatement ,
                                            qmnPlan     * aPlan ,
                                            qmsQuerySet * aQuerySet );

    // Dependency 설정 6단계
    static IDE_RC    step6refineDependencies( qmnPlan     * aPlan ,
                                              qcDepInfo   * aDependencies );
    
private:

    // Join Order와 Dependencies로 부터 가장 오른쪽 order를 찾기
    static IDE_RC    findRightJoinOrder( qmsSFWGH   * aSFWGH ,
                                         qcDepInfo  * aInputDependencies ,
                                         qcDepInfo  * aOutputDependencies );
    
    
};

#endif /* _O_QMO_PLAN_DEPENDENCY_H_ */

