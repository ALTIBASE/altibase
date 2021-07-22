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
 * $Id: qmoParallelPlan.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <qmnPRLQ.h>
#include <qmnPSCRD.h>
#include <qmoParallelPlan.h>
#include <qmoNormalForm.h>
#include <qmoOneMtrPlan.h>
#include <qmo.h>

IDE_RC qmoParallelPlan::initPRLQ( qcStatement * aStatement,
                                  qmnPlan     * aParent,
                                  qmnPlan    ** aPlan )
{
    UInt      sDataNodeOffset;
    qmncPRLQ* sPRLQ;

    IDU_FIT_POINT_FATAL( "qmoParallelPlan::initPRLQ::__FT__" );

    IDE_DASSERT( aStatement != NULL );

    IDU_FIT_POINT( "qmoParallelPlan::initPRLQ::alloc",
                   idERR_ABORT_InsufficientMemory);
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncPRLQ ),
                                               (void**)& sPRLQ )
              != IDE_SUCCESS);

    QMO_INIT_PLAN_NODE( sPRLQ,
                        QC_SHARED_TMPLATE(aStatement),
                        QMN_PRLQ,
                        qmnPRLQ,
                        qmndPRLQ,
                        sDataNodeOffset );

    IDE_TEST( qmc::copyResultDesc( aStatement,
                                   aParent->resultDesc,
                                   & sPRLQ->plan.resultDesc )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan*)sPRLQ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoParallelPlan::makePRLQ( qcStatement * aStatement,
                                  qmsQuerySet * aQuerySet,
                                  UInt          aParallelType,
                                  qmnPlan     * aChildPlan,
                                  qmnPlan     * aPlan )
{
    qmncPRLQ    * sPRLQ = (qmncPRLQ*)aPlan;
    UInt          sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoParallelPlan::makePRLQ::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    aPlan->offset   = QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset + ID_SIZEOF(qmndPRLQ) );

    sPRLQ->plan.left = aChildPlan;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sPRLQ->plan.flag = QMN_PLAN_FLAG_CLEAR;

    if ( aChildPlan != NULL )
    {
        sPRLQ->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);
    }
    else
    {
        // Nothing to do
    }

    /*
     * PROJ-2402 Parallel Table Scan
     * parallel type 세분화
     */
    switch ( aParallelType )
    {
        case QMO_MAKEPRLQ_PARALLEL_TYPE_SCAN:
            sPRLQ->mParallelType = QMNC_PRLQ_PARALLEL_TYPE_SCAN;
            break;
        case QMO_MAKEPRLQ_PARALLEL_TYPE_PARTITION:
            sPRLQ->mParallelType = QMNC_PRLQ_PARALLEL_TYPE_PARTITION;
            break;
        default:
            IDE_FT_ERROR(0);
    }

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    if ( aChildPlan != NULL )
    {
        IDE_TEST( qmc::filterResultDesc( aStatement,
                                         & sPRLQ->plan.resultDesc,
                                         & aChildPlan->depInfo,
                                         & aQuerySet->depInfo )
                  != IDE_SUCCESS );
    }
    else

    {
        // Multi-children 을 가지는 노드에서 사용되는 경우이다.
        // Execution 시에 하위 노드가 정해지므로 filter 할 수 없다.
        // Nothing to do
    }

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sPRLQ->plan,
                                            QMO_PRLQ_DEPENDENCY,
                                            0,
                                            NULL,
                                            0,
                                            NULL,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    if ( aChildPlan != NULL )
    {
        sPRLQ->plan.mParallelDegree = aChildPlan->mParallelDegree;
    }
    else
    {
        sPRLQ->plan.mParallelDegree = 1;
    }

    sPRLQ->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sPRLQ->plan.flag |= QMN_PLAN_PRLQ_EXIST_TRUE;
    if ( sPRLQ->plan.left != NULL )
    {
        sPRLQ->plan.flag |= (sPRLQ->plan.left->flag & QMN_PLAN_MTR_EXIST_MASK);
    }
    else
    {
        sPRLQ->plan.flag |= QMN_PLAN_MTR_EXIST_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * ------------------------------------------------------------------
 * PROJ-2402 Parallel Table Scan
 * ------------------------------------------------------------------
 * initPSCRD()
 * makePSCRD()
 * ------------------------------------------------------------------
 */

IDE_RC qmoParallelPlan::initPSCRD(qcStatement  * aStatement,
                                  qmsQuerySet  * aQuerySet,
                                  qmnPlan      * aParent,
                                  qmnPlan     ** aPlan)
{
    qmncPSCRD* sPSCRD;
    UInt       sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoParallelPlan::initPSCRD::__FT__" );

    IDE_DASSERT(aStatement != NULL);

    IDU_FIT_POINT( "qmoParallelPlan::initPSCRD::alloc",
                   idERR_ABORT_InsufficientMemory);
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncPSCRD ),
                                               (void**)& sPSCRD )
              != IDE_SUCCESS);

    QMO_INIT_PLAN_NODE(sPSCRD,
                       QC_SHARED_TMPLATE(aStatement),
                       QMN_PSCRD,
                       qmnPSCRD,
                       qmndPSCRD,
                       sDataNodeOffset);

    if ( aParent != NULL )
    {
        IDE_TEST( qmc::pushResultDesc( aStatement,
                                       aQuerySet,
                                       ID_FALSE,
                                       aParent->resultDesc,
                                       & sPSCRD->plan.resultDesc)
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT(0);
    }

    *aPlan = (qmnPlan*)sPSCRD;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoParallelPlan::makePSCRD(qcStatement * aStatement,
                                  qmsQuerySet * aQuerySet,
                                  qmsFrom     * aFrom,
                                  qmoPSCRDInfo* aPSCRDInfo,
                                  qmnPlan     * aPlan)
{
    qmncPSCRD * sPSCRD;
    UInt        sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoParallelPlan::makePSCRD::__FT__" );

    sPSCRD = (qmncPSCRD*)aPlan;

    aPlan->offset   = QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8(aPlan->offset + ID_SIZEOF(qmndPPCRD));

    sPSCRD->mTableRef    = aFrom->tableRef;
    sPSCRD->mTupleRowID  = aFrom->tableRef->table;
    sPSCRD->mTableHandle = aFrom->tableRef->tableInfo->tableHandle;
    sPSCRD->mTableSCN    = aFrom->tableRef->tableSCN;

    /*
     * --------------------------------------------------------------
     * flag
     * --------------------------------------------------------------
     */
    sPSCRD->plan.flag = QMN_PLAN_FLAG_CLEAR;

    if ((QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPSCRD->mTupleRowID].lflag &
         MTC_TUPLE_STORAGE_MASK) ==
        MTC_TUPLE_STORAGE_MEMORY)
    {
        sPSCRD->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
        sPSCRD->plan.flag |= QMN_PLAN_STORAGE_MEMORY;
    }
    else
    {
        sPSCRD->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
        sPSCRD->plan.flag |= QMN_PLAN_STORAGE_DISK;
    }

    /*
     * --------------------------------------------------------------
     * display
     * --------------------------------------------------------------
     */
    IDE_TEST( qmg::setDisplayInfo( aFrom,
                                   &( sPSCRD->mTableOwnerName ),
                                   &( sPSCRD->mTableName ),
                                   &( sPSCRD->mAliasName ) )
              != IDE_SUCCESS );

    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    /*
     * --------------------------------------------------------------
     * constant filter
     * --------------------------------------------------------------
     */
    IDE_TEST( processPredicate( aStatement,
                                aPSCRDInfo->mConstantPredicate,
                                & sPSCRD->mConstantFilter )
              != IDE_SUCCESS );

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPSCRD->mConstantFilter,
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );

    IDU_FIT_POINT("qmoParallelPlan::makePSCRD::setDependency");
    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sPSCRD->plan,
                                            QMO_PSCRD_DEPENDENCY,
                                            sPSCRD->mTupleRowID,
                                            NULL,
                                            1,
                                            & sPSCRD->mConstantFilter,
                                            0,
                                            NULL)
              != IDE_SUCCESS );

    sPSCRD->plan.mParallelDegree = aFrom->tableRef->mParallelDegree;

    IDE_DASSERT(sPSCRD->plan.childrenPRLQ != NULL)
    sPSCRD->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sPSCRD->plan.flag |= QMN_PLAN_PRLQ_EXIST_TRUE;
    sPSCRD->plan.flag |= QMN_PLAN_MTR_EXIST_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoParallelPlan::processPredicate(qcStatement  * aStatement,
                                         qmoPredicate * aConstantPredicate,
                                         qtcNode     ** aConstantFilter)
{
    qtcNode* sNode;
    qtcNode* sOptimizedNode;

    IDU_FIT_POINT_FATAL( "qmoParallelPlan::processPredicate::__FT__" );

    sNode          = NULL;
    sOptimizedNode = NULL;

    if ( aConstantPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          aConstantPredicate,
                                          & sNode )
                  != IDE_SUCCESS );

        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        *aConstantFilter = sOptimizedNode;
    }
    else
    {
        /* nothing to do */
    }

    *aConstantFilter = sOptimizedNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoParallelPlan::copyGRAG( qcStatement * aStatement,
                                  qmsQuerySet * aQuerySet,
                                  UShort        aDestTable,
                                  qmnPlan     * aOrgPlan,
                                  qmnPlan    ** aDstPlan )
{
    qmncGRAG          * sGRAG;
    qcTemplate        * sTemplate = QC_SHARED_TMPLATE(aStatement);

    IDU_FIT_POINT_FATAL( "qmoParallelPlan::copyGRAG::__FT__" );

    IDE_DASSERT( aOrgPlan->type == QMN_GRAG );

    //----------------------------------
    // plan을 복사한다.
    //----------------------------------
    IDU_FIT_POINT( "qmoParallelPlan::copyGRAG::alloc::GRAG" );
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncGRAG ),
                                               (void **)& sGRAG )
              != IDE_SUCCESS );

    idlOS::memcpy( sGRAG, aOrgPlan, ID_SIZEOF( qmncGRAG ) );


    //----------------------------------
    // result desc 를 복사한다.
    //----------------------------------
    sGRAG->plan.resultDesc = NULL;

    IDE_TEST( qmc::copyResultDesc( aStatement,
                                   aOrgPlan->resultDesc,
                                   & sGRAG->plan.resultDesc )
              != IDE_SUCCESS );

    //----------------------------------
    // initGRAG 의 작업을 단순화 시킨다.
    //----------------------------------
    sGRAG->plan.offset           = sTemplate->tmplate.dataSize;
    sGRAG->planID                = sTemplate->planCount;

    sTemplate->tmplate.dataSize += idlOS::align8(ID_SIZEOF(qmndGRAG));
    sTemplate->planCount        += 1;

    // makeGRAG qmoDependency::setDependency 함수가 제대로 설정하지 못한다.
    // 아래와 같이 설정하면 ERR_INVALID_DEPENDENCY 가 발생하지 않는다.
    // 실패하는 원인으로는 파티션 테이블의 tuple 값이 서로 다르기 때문이다.
    // makeSCAN 에서는 ERR_INVALID_DEPENDENCY 를 체크하지 않기때문에 문제가 없었다.
    sGRAG->plan.dependency = aDestTable;
    sGRAG->depTupleRowID   = aDestTable;

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sGRAG->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    *aDstPlan = (qmnPlan*)sGRAG;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
