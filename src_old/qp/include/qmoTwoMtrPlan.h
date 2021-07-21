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
 * $Id: qmoTwoMtrPlan.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Plan Generator
 *
 *     Two-child Materialized Plan을 생성하기 위한 관리자이다.
 *
 *     다음과 같은 Plan Node의 생성을 관리한다.
 *         - SITS 노드
 *         - SDIF 노드
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_TWO_CHILD_MATER_H_
#define _O_QMO_TWO_CHILD_MATER_H_ 1

#include <qc.h>
#include <qmoDef.h>
#include <qmn.h>
#include <qmgDef.h>
#include <qmoDependency.h>
#include <qmoPredicate.h>
#include <qmoNormalForm.h>
#include <qmoSubquery.h>

//------------------------------
//SITS노드의 dependency를 호출을 위한 flag
//------------------------------
#define QMO_SITS_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_FALSE      | \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         | \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION | \
                              QMO_DEPENDENCY_STEP2_SETNODE_TRUE             | \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     | \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE  )


//------------------------------
//SDIF노드의 dependency를 호출을 위한 flag
//------------------------------
#define QMO_SDIF_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_FALSE      | \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE         | \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION | \
                              QMO_DEPENDENCY_STEP2_SETNODE_TRUE             | \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE     | \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE  )

//---------------------------------------------------
// Two-Child Meterialized Plan을 관리하기 위한 자료 구조
//---------------------------------------------------

//------------------------------
//makeSITS()함수에 필요한 flag
//------------------------------

#define QMO_MAKESITS_TEMP_TABLE_MASK             (0x00000001)
#define QMO_MAKESITS_MEMORY_TEMP_TABLE           (0x00000000)
#define QMO_MAKESITS_DISK_TEMP_TABLE             (0x00000001)

//------------------------------
//makeSDIF()함수에 필요한 flag
//------------------------------

#define QMO_MAKESDIF_TEMP_TABLE_MASK             (0x00000001)
#define QMO_MAKESDIF_MEMORY_TEMP_TABLE           (0x00000000)
#define QMO_MAKESDIF_DISK_TEMP_TABLE             (0x00000001)

//---------------------------------------------------
// Two-Child Materialized Plan을 관리하기 위한 함수
//---------------------------------------------------

class qmoTwoMtrPlan
{
public:

    // SITS 노드의 생성
    static IDE_RC    initSITS( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               qmnPlan      * aParent,
                               qmnPlan     ** aPlan );

    static IDE_RC    makeSITS( qcStatement  * aStatement ,
                               qmsQuerySet  * aQuerySet ,
                               UInt           aFlag,
                               UInt           aBucketCount ,
                               qmnPlan      * aLeftChild ,
                               qmnPlan      * aRightChild ,
                               qmnPlan      * aPlan );

    // SDIF 노드의 생성
    static IDE_RC    initSDIF( qcStatement  * aStatement,
                               qmsQuerySet  * aQuerySet ,
                               qmnPlan      * aParent,
                               qmnPlan     ** aPlan );

    static IDE_RC    makeSDIF( qcStatement  * aStatement,
                               qmsQuerySet  * aQuerySet ,
                               UInt           aFlag,
                               UInt           aBucketCount ,
                               qmnPlan      * aLeftChild ,
                               qmnPlan      * aRightChild ,
                               qmnPlan      * aPlan );
  
private:

};

#endif /* _O_QMO_TWO_CHILD_MATER_H_ */

