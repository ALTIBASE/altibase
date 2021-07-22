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
 * $Id: qmoDef.h 84125 2018-10-11 04:38:30Z jake.jang $
 *
 * Description :
 *     모든 Optimizer 관련 상수 및 자료 구조를 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_DEF_H_
#define _O_QMO_DEF_H_ 1

/***********************************************************************
 * plan node 생성시 초기화에 필요한 macro
 ***********************************************************************/

#define QMO_INIT_PLAN_NODE( _node_ ,                                                \
                            _template ,                                             \
                            _type ,                                                 \
                            _qmnplan_ ,                                             \
                            _qmndPlan_ ,                                            \
                            _offset )                                               \
{                                                                                   \
    _node_->plan.type            = _type;                                           \
    _node_->planID               = _template->planCount;                            \
    (_template->planCount)++;                                                       \
    _offset                      = _template->tmplate.dataSize;                     \
    _offset                      = idlOS::align8( _offset + ID_SIZEOF(_qmndPlan_)); \
    _node_->plan.offset          = _template->tmplate.dataSize;                     \
    _node_->plan.qmgOuput        = 0;                                               \
    _node_->plan.qmgAllCost      = 0;                                               \
    _node_->plan.init            = _qmnplan_::init;                                 \
    _node_->plan.doIt            = _qmnplan_::doIt;                                 \
    _node_->plan.padNull         = _qmnplan_::padNull;                              \
    _node_->plan.printPlan       = _qmnplan_::printPlan;                            \
    _node_->plan.left            = NULL;                                            \
    _node_->plan.right           = NULL;                                            \
    _node_->plan.children        = NULL;                                            \
    _node_->plan.dependency      = ID_UINT_MAX;                                     \
    _node_->plan.outerDependency = ID_UINT_MAX;                                     \
    _node_->plan.resultDesc      = NULL;                                            \
    _node_->plan.mParallelDegree = 0;                                               \
}

/***********************************************************************
 * Distinct Aggregation의 bucketCnt를 위한 자료 구조
 ***********************************************************************/

typedef struct qmoDistAggArg
{
    qtcNode       * aggArg;
    UInt            bucketCnt;
    qmoDistAggArg * next;

} qmoDistAggArg;

struct qmgSELT;
struct qmoPredicate;
struct qmncScanMethod;
struct qmncSCAN;

#define QMO_SCAN_METHOD_UNSELECTED    ID_UINT_MAX
#define QMO_RANGE_USING_HASH_MAX_VALUE (1000)
#define QMO_RANGE_USING_HASH_PRED_MAX  (128)

typedef struct qmoScanDecisionFactor
{
    qmgSELT               * baseGraph;
    qmnPlan               * basePlan;
    qmoPredicate          * predicate;
    UInt                    accessMethodsOffset;
    UInt                    selectedMethodOffset;
    UInt                    candidateCount;
    qmncScanMethod        * candidate;
    qmoScanDecisionFactor * next;
} qmoScanDecisionFactor;

// PROJ-2205 rownum in DML
// update type
typedef enum qmoUpdateType
{
    QMO_UPDATE_NORMAL = 0,        // 일반 테이블의 update
    QMO_UPDATE_ROWMOVEMENT,       // row movement가 발생하면 delete-insert로 처리하는 update
    QMO_UPDATE_CHECK_ROWMOVEMENT, // row movement가 발생하면 에러내는 update
    QMO_UPDATE_NO_ROWMOVEMENT     // row movement가 일어나지 않는 update
} qmnUpdateType;

// PROJ-2205 rownum in DML
// merge DML의 children index
typedef enum qmoMergeChildrenIndex
{
    QMO_MERGE_SELECT_SOURCE_IDX = 0,
    QMO_MERGE_SELECT_TARGET_IDX,
    QMO_MERGE_UPDATE_IDX,
    QMO_MERGE_DELETE_IDX,    
    QMO_MERGE_INSERT_IDX,
    QMO_MERGE_INSERT_NOROWS_IDX,
    QMO_MERGE_IDX_MAX
} qmoMergeChildrenIndex;

#endif /* _O_QMO_DEF_H_ */

