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
 * $Id: qmoParallelPlan.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QMO_PARALLEL_PLAN_H_
#define _O_QMO_PARALLEL_PLAN_H_ 1

#include <qc.h>
#include <qmoDependency.h>

// PRLQ 노드의 dependency를 호출을 위한 flag
#define QMO_PRLQ_DEPENDENCY ( QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE      |   \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_FALSE        |   \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE      |   \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE           |   \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE   |   \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE )
/*
                              QMO_DEPENDENCY_STEP2_DEP_WITH_MATERIALIZATION|   \
*/

/* PSCRD노드의 dependency를 호출을 위한 flag */
#define QMO_PSCRD_DEPENDENCY (QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE     |    \
                              QMO_DEPENDENCY_STEP2_BASE_TABLE_TRUE        |    \
                              QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE     |    \
                              QMO_DEPENDENCY_STEP2_SETNODE_FALSE          |    \
                              QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_FALSE  |    \
                              QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_FALSE)

#define QMO_MAKEPRLQ_PARALLEL_TYPE_SCAN      (1)
#define QMO_MAKEPRLQ_PARALLEL_TYPE_PARTITION (2)

typedef struct qmoPSCRDInfo
{
    qmoPredicate* mConstantPredicate;
} qmoPSCRDInfo;

class qmoParallelPlan
{
public:
    static IDE_RC initPRLQ( qcStatement * aStatement,
                            qmnPlan     * aParent,
                            qmnPlan    ** aPlan );

    static IDE_RC makePRLQ( qcStatement * aStatement,
                            qmsQuerySet * aQuerySet,
                            UInt          aParallelType,
                            qmnPlan     * aChildPlan,
                            qmnPlan     * aPlan );

    /*
     * PROJ-2402 Parallel Table Scan
     */
    static IDE_RC initPSCRD(qcStatement * aStatement,
                            qmsQuerySet * aQuerySet,
                            qmnPlan     * aParent,
                            qmnPlan    ** aPlan);

    static IDE_RC makePSCRD(qcStatement * aStatement,
                            qmsQuerySet * aQuerySet,
                            qmsFrom     * aFrom,
                            qmoPSCRDInfo* aPSCRDInfo,
                            qmnPlan     * aPlan);

    static IDE_RC copyGRAG(qcStatement * aStatement,
                           qmsQuerySet * aQuerySet,
                           UShort        aDestTable,
                           qmnPlan     * aOrgPlan,
                           qmnPlan    ** aDstPlan);

    static IDE_RC processPredicate(qcStatement  * aStatement,
                                   qmoPredicate * aConstantPredicate,
                                   qtcNode     ** aConstantFilter);
}; 

#endif /* _O_QMO_PARALLEL_PLAN_H_ */
