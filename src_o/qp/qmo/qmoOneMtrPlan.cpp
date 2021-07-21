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
 * $Id: qmoOneMtrPlan.cpp 91133 2021-07-05 01:24:06Z donovan.seo $
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
 *         - WNST 노드
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <qmo.h>
#include <qmoOneMtrPlan.h>
#include <qmsParseTree.h>
#include <qtcDef.h>
#include <qmv.h>

extern mtfModule mtfList;
extern mtfModule mtfLag;
extern mtfModule mtfLagIgnoreNulls;
extern mtfModule mtfLead;
extern mtfModule mtfLeadIgnoreNulls;
extern mtfModule mtfNtile;
extern mtfModule mtfGetBlobLocator;
extern mtfModule mtfGetClobLocator;
extern mtfModule mtfListagg;        /* BUG-46906 */
extern mtfModule mtfPercentileDisc; /* BUG-46922 */
extern mtfModule mtfPercentileCont; /* BUG-46922 */

// ORDER BY절과 같이 sorting 대상 column이 별도로 지정되어있는 경우
IDE_RC
qmoOneMtrPlan::initSORT( qcStatement     * aStatement ,
                         qmsQuerySet     * aQuerySet ,
                         UInt              aFlag,
                         qmsSortColumns  * aOrderBy ,
                         qmnPlan         * aParent,
                         qmnPlan        ** aPlan )
{
/***********************************************************************
 *
 * Description : SORT 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncSORT의 할당 및 초기화
 *     + 메인 작업
 *         - 검색 방법 및 저장 방식 flag 세팅
 *         - SORT노드의 컬럼 구성
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *     - 컬럼의 구성은 다음과 같이 4가지로 한다. 이에따라 인터페이스를
 *       구분해주도록 한다. (private으로 처리)
 *         - ORDER BY로 사용되는 경우 (baseTable + orderby정보)
 *         - ORDER BY의 예외상황 (하위에 GRAG , HSDS일경우
 *                                단 memory base은 저장한다.)
 *         - SORT-BASED GROUPING (baseTable + grouping 정보)
 *         - SORT-BASED DISTINCTION (baseTable + target 정보)
 *         - SORT-BASED JOIN (joinPredicate로 컬럼구성)
 *         - SORT MERGE JOIN (joinPredicate로 컬럼구성, sequential search)
 *         - STORE AND SEARCH (하위 VIEW의 정보)
 *
 ***********************************************************************/

    qmncSORT          * sSORT;
    qmsSortColumns    * sSortColumn;
    qmcAttrDesc       * sItrAttr;
    qtcNode           * sNode;
    UInt                sDataNodeOffset;
    UInt                sColumnCount;
    UInt                sFlag = 0;
    SInt                i;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initSORT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncSORT의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF(qmncSORT) ,
                                               (void **)& sSORT )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sSORT ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_SORT ,
                        qmnSORT ,
                        qmndSORT,
                        sDataNodeOffset );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sSORT->flag  = QMN_PLAN_FLAG_CLEAR;

    sSORT->flag &= ~QMNC_SORT_SEARCH_MASK;
    sSORT->flag |= QMNC_SORT_SEARCH_SEQUENTIAL;

    sSORT->range = NULL;

    *aPlan = (qmnPlan *)sSORT;

    // Sorting key임을 표시한다.
    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    // Order by절의 attribute들을 추가한다.
    for ( sSortColumn = aOrderBy, sColumnCount = 0;
          sSortColumn != NULL;
          sSortColumn = sSortColumn->next, sColumnCount++ )
    {
        // Sorting 방향을 표시한다.
        if ( sSortColumn->isDESC == ID_FALSE )
        {
            sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_ORDER_ASC;
        }
        else
        {
            sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_ORDER_DESC;
        }

        /* PROJ-2435 order by nulls first/last */
        if ( sSortColumn->nullsOption != QMS_NULLS_NONE )
        {
            if ( sSortColumn->nullsOption == QMS_NULLS_FIRST )
            {
                sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
                sFlag |= QMC_ATTR_SORT_NULLS_ORDER_FIRST;
            }
            else
            {
                sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
                sFlag |= QMC_ATTR_SORT_NULLS_ORDER_LAST;
            }
        }
        else
        {
            sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_NULLS_ORDER_NONE;
        }

        /* BUG-36826 A rollup or cube occured wrong result using order by count_i3 */
        if ( ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK ) == QMO_MAKESORT_VALUE_TEMP_TRUE ) ||
             ( ( aFlag & QMO_MAKESORT_GROUP_EXT_VALUE_MASK ) == QMO_MAKESORT_GROUP_EXT_VALUE_TRUE ) )
        {
            for ( i = 1, sItrAttr = aParent->resultDesc;
                  i < sSortColumn->targetPosition;
                  i++ )
            {
                sItrAttr = sItrAttr->next;
            }

            if ( sItrAttr != NULL )
            {
                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                &sSORT->plan.resultDesc,
                                                sItrAttr->expr,
                                                sFlag,
                                                QMC_APPEND_EXPRESSION_TRUE,
                                                ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            &sSORT->plan.resultDesc,
                                            sSortColumn->sortColumn,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
    }

    for ( sItrAttr = aParent->resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next )
    {
        /* BUG-37146 together with rollup order by the results are wrong.
         * Value Temp의 상위에 SortTemp가 있을 경우 모든 SortTemp는 value temp를
         * 참조해야한다. 따라서 상위 Plan의 exression을 모두 sort에 추가하고 추후
         * pushResultDesc 함수에서 passNode를 만들도록 유도한다.
         */
        if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK ) ==
               QMO_MAKESORT_VALUE_TEMP_TRUE )
        {
            if ( ( sItrAttr->expr->lflag & QTC_NODE_AGGREGATE_MASK ) ==
                 QTC_NODE_AGGREGATE_EXIST )
            {
                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                &sSORT->plan.resultDesc,
                                                sItrAttr->expr,
                                                QMC_ATTR_SEALED_TRUE,
                                                QMC_APPEND_EXPRESSION_TRUE,
                                                ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
        /* BUG-35265 wrong result desc connect_by_root, sys_connect_by_path */
        if ( ( sItrAttr->expr->node.lflag & MTC_NODE_FUNCTION_CONNECT_BY_MASK )
               == MTC_NODE_FUNCTION_CONNECT_BY_TRUE )
        {
            /* 복사해서 넣지 않으면 상위 Parent 가 바뀔때 잘못된 Tuple을 가르키게된다 */
            IDU_FIT_POINT( "qmoOneMtrPlan::initSORT::alloc::Node",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                       (void**)&sNode )
                      != IDE_SUCCESS );
            idlOS::memcpy( sNode, sItrAttr->expr, ID_SIZEOF( qtcNode ) );

            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            &sSORT->plan.resultDesc,
                                            sNode,
                                            QMC_ATTR_SEALED_TRUE,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do*/
        }
    }

    // ORDER BY 절 외 참조되는 attribute들을 추가한다.
    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ( aParent->type == QMN_PROJ ? ID_TRUE : ID_FALSE ),
                                   aParent->resultDesc,
                                   & sSORT->plan.resultDesc )
              != IDE_SUCCESS );

    // BUG-36997
    // select distinct i1, i2+1 from t1 order by i1;
    // distinct 를 처리할때 i2+1 을 연산해서 저장해 놓는다.
    // 따라서 PASS 노드를 생성해주어야 한다.
    IDE_TEST( qmc::makeReferenceResult( aStatement,
                                        ( aParent->type == QMN_PROJ ? ID_TRUE : ID_FALSE ),
                                        aParent->resultDesc,
                                        sSORT->plan.resultDesc )
              != IDE_SUCCESS );

    // ORDER BY절에서 참조되었다는 flag를 더 이상 물려받지 않도록 해제한다.
    for( sItrAttr = sSORT->plan.resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        sItrAttr->flag &= ~QMC_ATTR_ORDER_BY_MASK;
        sItrAttr->flag |= QMC_ATTR_ORDER_BY_FALSE;
    }

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sSORT->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// 단순히 일시적인 결과의 저장이나, grouping/distinction과 같이 대상 전체에 sorting이 필요한 경우
IDE_RC
qmoOneMtrPlan::initSORT( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
    qmncSORT          * sSORT;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initSORT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncSORT의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncSORT ),
                                               (void **)& sSORT )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sSORT ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_SORT ,
                        qmnSORT ,
                        qmndSORT,
                        sDataNodeOffset );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sSORT->flag  = QMN_PLAN_FLAG_CLEAR;

    sSORT->flag &= ~QMNC_SORT_SEARCH_MASK;
    sSORT->flag |= QMNC_SORT_SEARCH_SEQUENTIAL;

    sSORT->range = NULL;

    IDE_TEST( qmc::copyResultDesc( aStatement,
                                   aParent->resultDesc,
                                   &sSORT->plan.resultDesc )
              != IDE_SUCCESS );

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sSORT->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sSORT;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// Sort/merge join을 위해 predicate에 포함된 attribute로 sorting이 필요한 경우
IDE_RC
qmoOneMtrPlan::initSORT( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qtcNode      * aRange,
                         idBool         aIsLeftSort,
                         idBool         aIsRangeSearch,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
    qmncSORT          * sSORT;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initSORT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncSORT의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncSORT ),
                                               (void **)& sSORT )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sSORT ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_SORT ,
                        qmnSORT ,
                        qmndSORT,
                        sDataNodeOffset );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sSORT->flag  = QMN_PLAN_FLAG_CLEAR;

    if( aIsRangeSearch == ID_TRUE )
    {
        sSORT->flag &= ~QMNC_SORT_SEARCH_MASK;
        sSORT->flag |= QMNC_SORT_SEARCH_RANGE;
        sSORT->range = aRange;
    }
    else
    {
        sSORT->flag &= ~QMNC_SORT_SEARCH_MASK;
        sSORT->flag |= QMNC_SORT_SEARCH_SEQUENTIAL;
        sSORT->range = NULL;
    }

    if( aRange != NULL )
    {
        IDE_TEST( appendJoinPredicate( aStatement,
                                       aQuerySet,
                                       aRange,
                                       aIsLeftSort,
                                       ID_FALSE, // allowDuplicateAttr?
                                       &sSORT->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sSORT->plan.resultDesc )
              != IDE_SUCCESS );

    // PROJ-2462 Result Cache
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sSORT->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sSORT;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeSORT( qcStatement       * aStatement ,
                         qmsQuerySet       * aQuerySet ,
                         UInt                aFlag,
                         qmgPreservedOrder * aChildPreservedOrder ,
                         qmnPlan           * aChildPlan ,
                         SDouble             aStoreRowCount,
                         qmnPlan           * aPlan )
{
    qmncSORT          * sSORT = (qmncSORT *)aPlan;
    UInt                sDataNodeOffset;
    qmcMtrNode        * sMtrNode[2];
    qtcNode           * sPredicate[2];

    UShort              sTupleID;
    UShort              sColumnCount  = 0;

    qmcMtrNode        * sNewMtrNode   = NULL;
    qmcMtrNode        * sFirstMtrNode = NULL;
    qmcMtrNode        * sLastMtrNode  = NULL;

    mtcTemplate       * sMtcTemplate;
    qmcAttrDesc       * sItrAttr;
    UShort              sKeyCount     = 0;

    UShort       sOrgTable  = ID_USHORT_MAX;
    UShort       sOrgColumn = ID_USHORT_MAX;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeSORT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildPlan != NULL );
    IDE_DASSERT( aPlan != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndSORT));

    sSORT->mtrNodeOffset = sDataNodeOffset;
    sSORT->storeRowCount = aStoreRowCount;

    sSORT->plan.left = aChildPlan;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sSORT->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sSORT->myNode = NULL;

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    //----------------------------------
    // 튜플의 할당
    //----------------------------------
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    // To Fix PR-8493
    // GROUP BY 컬럼의 대체 여부를 결정하기 위해서는
    // Tuple의 저장 매체 정보를 미리 기록하고 있어야 한다.
    if( (aFlag & QMO_MAKESORT_TEMP_TABLE_MASK) ==
        QMO_MAKESORT_MEMORY_TEMP_TABLE )
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    // To Fix BUG-7730
    // Preserved Order가 보장되는 경우,
    // Sorting 없이 하위에서 올라오는 순서대로 저장하기 위함
    if ( ( aFlag & QMO_MAKESORT_PRESERVED_ORDER_MASK ) ==
         QMO_MAKESORT_PRESERVED_TRUE )
    {
        sSORT->flag &= ~QMNC_SORT_STORE_MASK;
        sSORT->flag |= QMNC_SORT_STORE_PRESERVE;
    }
    else
    {
        sSORT->flag &= ~QMNC_SORT_STORE_MASK;
        sSORT->flag |= QMNC_SORT_STORE_ONLY;
    }

    if( sSORT->range != NULL )
    {
        sSORT->flag &= ~QMNC_SORT_SEARCH_MASK;
        sSORT->flag |= QMNC_SORT_SEARCH_RANGE;
    }

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sSORT->plan.resultDesc,
                                     &aChildPlan->depInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    // BUGBUG
    // PROJ-2179 Full store nested join시 다음과 같은 구문의 경우 result descriptor가 비어있을 수 있다.
    // SELECT r.a FROM r FULL OUTER JOIN s ON r.a > 1 AND s.a = 3;
    // 따라서 각 row마다 1로 채워져있는 temp table을 생성한다.
    // 원칙적으로 이 경우 full stored nested join을 수행해서는 안된다.
    if( aPlan->resultDesc == NULL )
    {
        IDE_TEST( qmg::makeDummyMtrNode( aStatement,
                                         aQuerySet,
                                         sTupleID,
                                         &sColumnCount,
                                         &sNewMtrNode )
                  != IDE_SUCCESS );

        sFirstMtrNode = sNewMtrNode;
        sLastMtrNode  = sNewMtrNode;
    }
    else
    {
        // Nothing to do.
    }

    // Sorting key가 아닌 경우
    IDE_TEST( makeNonKeyAttrsMtrNodes( aStatement,
                                       aQuerySet,
                                       sSORT->plan.resultDesc,
                                       sTupleID,
                                       &sFirstMtrNode,
                                       &sLastMtrNode,
                                       &sColumnCount )
              != IDE_SUCCESS );

    sSORT->baseTableCount = sColumnCount;

    for( sItrAttr = sSORT->plan.resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        // Sorting key 인 경우
        if( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            sOrgTable  = sItrAttr->expr->node.table;
            sOrgColumn = sItrAttr->expr->node.column;

            // Sorting이 필요함을 표시
            sSORT->flag &= ~QMNC_SORT_STORE_MASK;
            sSORT->flag |= QMNC_SORT_STORE_SORTING;

            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ( ( sItrAttr->flag & QMC_ATTR_CONVERSION_MASK )
                                                == QMC_ATTR_CONVERSION_TRUE ? ID_TRUE : ID_FALSE),
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            // BUG-43088 setColumnLocate 호출 위치변경으로 인한
            // TC/Server/qp4/Bugs/PR-13286/PR-13286.sql diff 를 복구하기 위해
            // ex) SELECT /*+ USE_ONE_PASS_SORT( D2, D1 ) */ d1.i1 FROM D1, D2
            //      WHERE D1.I1 > D2.I1 AND D1.I1 < D2.I1 + 10;
            //            ^^^^^ column locate 변경이 누락
            //   * sSORT->range
            //     AND
            //     |
            //     OR
            //     |
            //     < ------------------- >
            //     | (1)                 | (2)
            //     d1.i1 - +             d1.i1 - d2.i1
            //     (2,0)   |             (2,0)->(4,0) 으로 변경 누락
            //             d2.i1 - 10
            // sSORT->range 를 순회하여 sItrAttr->expr 의 원본과 동일한 노드를 찾아
            // sNewMtrNode.dstNode 즉 변경된 값 sItrAttr->expr(table,colomn)으로 set
            if ( sSORT->range != NULL )
            {
                IDE_TEST( qmg::resetDupNodeToMtrNode( aStatement,
                                                      sOrgTable,
                                                      sOrgColumn,
                                                      sItrAttr->expr,
                                                      sSORT->range )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            sNewMtrNode->flag &= ~QMC_MTR_SORT_NEED_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_NEED_TRUE;

            // BUG-37993
            // qmgJoin::makePreservedOrder
            // Merge Join의 경우 ASCENDING만이 가능하다.
            if ( (aFlag & QMO_MAKESORT_METHOD_MASK) == QMO_MAKESORT_SORT_MERGE_JOIN )
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
            }
            else
            {
                if ( ( sItrAttr->flag & QMC_ATTR_SORT_ORDER_MASK )
                        == QMC_ATTR_SORT_ORDER_ASC )
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
                }
                else
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_DESCENDING;
                }

                /* PROJ-2435 order by nulls first/last */
                if ( ( sItrAttr->flag & QMC_ATTR_SORT_NULLS_ORDER_MASK )
                     != QMC_ATTR_SORT_NULLS_ORDER_NONE )
                {
                    if ( ( sItrAttr->flag & QMC_ATTR_SORT_NULLS_ORDER_MASK )
                         == QMC_ATTR_SORT_NULLS_ORDER_FIRST )
                    {
                        sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                        sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_FIRST;
                    }
                    else
                    {
                        sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                        sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_LAST;
                    }
                }
                else
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_NONE;
                }

                // Preserved order 설정
                if( aChildPreservedOrder != NULL )
                {
                    IDE_TEST( qmg::setDirection4SortColumn( aChildPreservedOrder,
                                                            sKeyCount,
                                                            &( sNewMtrNode->flag ) )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
            }

            if( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            sKeyCount++;
        }
        else
        {
            // Nothing to do.
        }
    }

    sSORT->myNode = sFirstMtrNode;

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------
    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sTupleID ,
                                           sColumnCount )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    //GRAPH에서 지정한 저장매체를 사용한다.
    if( (aFlag & QMO_MAKESORT_TEMP_TABLE_MASK) ==
        QMO_MAKESORT_MEMORY_TEMP_TABLE )
    {
        sSORT->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sSORT->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sSORT->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sSORT->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    //----------------------------------
    // mtcColumn , mtcExecute 정보의 구축
    //----------------------------------
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sSORT->myNode )
              != IDE_SUCCESS );

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    for( sNewMtrNode = sSORT->myNode , sColumnCount = 0 ;
         sNewMtrNode != NULL;
         sNewMtrNode = sNewMtrNode->next , sColumnCount++ ) ;

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    //----------------------------------
    //dependency 처리 및 subquery 처리
    //----------------------------------

    sPredicate[0] = sSORT->range;
    sMtrNode[0]  = sSORT->myNode;

    //----------------------------------
    // PROJ-1437
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate[0],
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            &sSORT->plan ,
                                            QMO_SORT_DEPENDENCY,
                                            sTupleID ,
                                            NULL ,
                                            1 ,
                                            sPredicate ,
                                            1 ,
                                            sMtrNode )
              != IDE_SUCCESS );

    // BUG-27526
    IDE_TEST_RAISE( sSORT->plan.dependency == ID_UINT_MAX,
                    ERR_INVALID_DEPENDENCY );

    sSORT->depTupleRowID = (UShort)sSORT->plan.dependency;

    //----------------------------------
    // PROJ-1473 column locate 지정.
    //----------------------------------

    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sSORT->myNode )
              != IDE_SUCCESS );

    //----------------------------------
    // PROJ-2179 calculate가 필요한 node들의 결과 위치를 설정
    //----------------------------------

    IDE_TEST( qmg::setCalcLocate( aStatement,
                                  sSORT->myNode )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sSORT->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sSORT->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sSORT->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);
    sSORT->plan.flag |= QMN_PLAN_MTR_EXIST_TRUE;

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sSORT->planID,
                               sSORT->plan.flag,
                               sMtcTemplate->rows[sTupleID].lflag,
                               sSORT->myNode,
                               &sSORT->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeSORT",
                                  "Invalid dependency" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::initHASH( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qtcNode      * aHashFilter,
                         idBool         aIsLeftHash,
                         idBool         aDistinction,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : HASH 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncHASH의 할당 및 초기화
 *     + 메인 작업
 *         - 검색 방법 및 CHECK_NOTNULL , DISTINCT의 여부 flag 세팅
 *         - HASH노드의 컬럼 구성
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *     - 컬럼의 구성은 다음과 같이 2가지로 한다. 이에따라 인터페이스를
 *       구분해주도록 한다. (private으로 처리)
 *         - HASH-BASED JOIN (joinPredicate로 컬럼구성)
 *         - STORE AND SEARCH (하위 VIEW의 정보)
 *
 *     - filter와 filterConst 생성시 passNode생성에 유의한다.
 *
 ***********************************************************************/

    qmncHASH          * sHASH;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initHASH::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncHASH의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncHASH ),
                                               (void **)& sHASH )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sHASH ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_HASH ,
                        qmnHASH ,
                        qmndHASH,
                        sDataNodeOffset );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sHASH->flag  = QMN_PLAN_FLAG_CLEAR;
    if( aDistinction == ID_TRUE )
    {
        sHASH->flag  &= ~QMNC_HASH_STORE_MASK;
        sHASH->flag  |= QMNC_HASH_STORE_DISTINCT;
    }
    else
    {
        sHASH->flag  &= ~QMNC_HASH_STORE_MASK;
        sHASH->flag  |= QMNC_HASH_STORE_HASHING;
    }

    if( aIsLeftHash == ID_FALSE )
    {
        sHASH->flag &= ~QMNC_HASH_SEARCH_MASK;
        sHASH->flag |= QMNC_HASH_SEARCH_RANGE;
    }
    else
    {
        sHASH->flag &= ~QMNC_HASH_SEARCH_MASK;
        sHASH->flag |= QMNC_HASH_SEARCH_SEQUENTIAL;
    }

    if( aHashFilter != NULL )
    {
        IDE_TEST( appendJoinPredicate( aStatement,
                                       aQuerySet,
                                       aHashFilter,
                                       aIsLeftHash,
                                       ID_TRUE,  // allowDuplicateAttr?
                                       &sHASH->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sHASH->plan.resultDesc )
              != IDE_SUCCESS );

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sHASH->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sHASH;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// Store and search 시
IDE_RC
qmoOneMtrPlan::initHASH( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
    qmncHASH          * sHASH;
    UInt                sDataNodeOffset;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initHASH::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncHASH의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncHASH ) ,
                                               (void **)& sHASH )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sHASH ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_HASH ,
                        qmnHASH ,
                        qmndHASH,
                        sDataNodeOffset );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sHASH->flag  = QMN_PLAN_FLAG_CLEAR;
    sHASH->flag  &= ~QMNC_HASH_STORE_MASK;
    sHASH->flag  |= QMNC_HASH_STORE_DISTINCT;

    sHASH->flag  &= ~QMNC_HASH_SEARCH_MASK;
    sHASH->flag  |= QMNC_HASH_SEARCH_STORE_SEARCH;

    IDE_TEST( qmc::copyResultDesc( aStatement,
                                   aParent->resultDesc,
                                   &sHASH->plan.resultDesc )
              != IDE_SUCCESS );

    for( sItrAttr = sHASH->plan.resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        sItrAttr->flag &= ~QMC_ATTR_KEY_MASK;
        sItrAttr->flag |= QMC_ATTR_KEY_TRUE;
    }

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sHASH->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sHASH;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeHASH( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         UInt           aFlag ,
                         UInt           aTempTableCount ,
                         UInt           aBucketCount ,
                         qtcNode      * aFilter ,
                         qmnPlan      * aChildPlan ,
                         qmnPlan      * aPlan,
                         idBool         aAllAttrToKey )
{
    qmncHASH          * sHASH = (qmncHASH *)aPlan;

    UInt                sDataNodeOffset;
    qmcMtrNode        * sMtrNode[2];
    qtcNode           * sPredicate[2];
    qtcNode           * sNode;
    qtcNode           * sOptimizedNode;

    UShort              sTupleID;
    UShort              sColumnCount  = 0;

    qmcMtrNode        * sNewMtrNode;
    qmcMtrNode        * sLastMtrNode  = NULL;
    qmcMtrNode        * sFirstMtrNode = NULL;

    mtcTemplate       * sMtcTemplate;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeHASH::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildPlan != NULL );
    IDE_DASSERT( aPlan != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndHASH));

    sHASH->mtrNodeOffset = sDataNodeOffset;

    sHASH->plan.left = aChildPlan;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sHASH->plan.flag = QMN_PLAN_FLAG_CLEAR;

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    sHASH->bucketCnt     = aBucketCount;
    sHASH->tempTableCnt  = aTempTableCount;

    //----------------------------------
    // 튜플의 할당
    //----------------------------------
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    // To Fix PR-8493
    // 컬럼의 대체 여부를 결정하기 위해서는
    // Tuple의 저장 매체 정보를 미리 기록하고 있어야 한다.
    if( (aFlag & QMO_MAKEHASH_TEMP_TABLE_MASK) ==
        QMO_MAKEHASH_MEMORY_TEMP_TABLE )
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sHASH->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     &( aQuerySet->depInfo ) )
              != IDE_SUCCESS );

    /* PROJ-2385
     * Inverse Index NL에서의 Left Distinct HASH는
     * Filtering 된 모든 Attribute를 Distinct Key로 사용해야 한다.
     * 그렇지 않을 경우 Distinct 결과가 줄어들어 JOIN 연산 결과가 줄어든다. */
    if ( aAllAttrToKey == ID_TRUE )
    {
        for ( sItrAttr = sHASH->plan.resultDesc;
              sItrAttr != NULL;
              sItrAttr = sItrAttr->next )
        {
            sItrAttr->flag &= ~QMC_ATTR_KEY_MASK;
            sItrAttr->flag |= QMC_ATTR_KEY_TRUE;
        }
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    // myNode의 구성
    //----------------------------------

    // Hash key가 아닌 경우
    IDE_TEST( makeNonKeyAttrsMtrNodes( aStatement,
                                       aQuerySet,
                                       sHASH->plan.resultDesc,
                                       sTupleID,
                                       & sFirstMtrNode,
                                       & sLastMtrNode,
                                       & sColumnCount )
              != IDE_SUCCESS );

    sHASH->baseTableCount = sColumnCount;

    for( sItrAttr = sHASH->plan.resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        // Hash key인 경우
        if( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ( ( sItrAttr->flag & QMC_ATTR_CONVERSION_MASK )
                                                == QMC_ATTR_CONVERSION_TRUE ? ID_TRUE : ID_FALSE),
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_HASH_NEED_MASK;
            sNewMtrNode->flag |= QMC_MTR_HASH_NEED_TRUE;

            if( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
        }
    }

    sHASH->myNode = sFirstMtrNode;

    switch( aFlag & QMO_MAKEHASH_METHOD_MASK )
    {
        //----------------------------------
        // Hash-based JOIN으로 사용되는 경우
        //----------------------------------
        case QMO_MAKEHASH_HASH_BASED_JOIN:
        {
            switch( aFlag & QMO_MAKEHASH_POSITION_MASK )
            {
                case QMO_MAKEHASH_POSITION_LEFT:
                {
                    /* PROJ-2385
                     * LEFT HASH인 경우, Hashing Method를 수정할 필요 없다.
                     * - Inverse Index NL => STORE_DISTINCT
                     * - Two-Pass HASH    => STORE_HASHING
                     */ 

                    sHASH->filter      = NULL;
                    sHASH->filterConst = NULL;

                    sHASH->flag &= ~QMNC_HASH_SEARCH_MASK;
                    sHASH->flag |= QMNC_HASH_SEARCH_SEQUENTIAL;
                    break;
                }
                case QMO_MAKEHASH_POSITION_RIGHT:
                {
                    // RIGHT HASH인 경우, Hashing Method를 STORE_HASHING으로 고정
                    sHASH->flag &= ~QMNC_HASH_STORE_MASK;
                    sHASH->flag |= QMNC_HASH_STORE_HASHING;

                    //검색 방식의 선택
                    sHASH->flag  &= ~QMNC_HASH_SEARCH_MASK;
                    sHASH->flag  |= QMNC_HASH_SEARCH_RANGE;

                    IDE_TEST( makeFilterConstFromPred( aStatement ,
                                                       aQuerySet,
                                                       sTupleID,
                                                       aFilter ,
                                                       &( sHASH->filterConst ) )
                              != IDE_SUCCESS );

                    // BUG-17921
                    IDE_TEST( qmoNormalForm::optimizeForm( aFilter,
                                                           & sOptimizedNode )
                              != IDE_SUCCESS );

                    sHASH->filter = sOptimizedNode;

                    break;
                }
                default:
                {
                    IDE_DASSERT(0);
                    break;
                }
            }
            break;
        }
        case QMO_MAKEHASH_STORE_AND_SEARCH:
        {
            //----------------------------------
            // Store and Search로 사용되는 경우
            //----------------------------------

            //not null 검사 여부
            if( (aFlag & QMO_MAKEHASH_NOTNULLCHECK_MASK ) ==
                QMO_MAKEHASH_NOTNULLCHECK_TRUE )
            {
                sHASH->flag  &= ~QMNC_HASH_NULL_CHECK_MASK;
                sHASH->flag  |= QMNC_HASH_NULL_CHECK_TRUE;
            }
            else
            {
                sHASH->flag  &= ~QMNC_HASH_NULL_CHECK_MASK;
                sHASH->flag  |= QMNC_HASH_NULL_CHECK_FALSE;
            }

            //Filter의 재구성
            IDE_TEST( makeFilterINSubquery( aStatement ,
                                            aQuerySet ,
                                            sTupleID,
                                            aFilter ,
                                            & sNode )
                      != IDE_SUCCESS );

            IDE_TEST( makeFilterConstFromPred( aStatement ,
                                               aQuerySet,
                                               sTupleID,
                                               sNode ,
                                               &(sHASH->filterConst) )
                      != IDE_SUCCESS );

            // BUG-17921
            IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                                   & sOptimizedNode )
                      != IDE_SUCCESS );

            sHASH->filter = sOptimizedNode;

            break;
        }
        default:
        {
            IDE_DASSERT(0);
            break;
        }
    }

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------
    IDE_TEST( qtc::allocIntermediateTuple( aStatement ,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sTupleID ,
                                           sColumnCount )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    //GRAPH에서 지정한 저장매체를 사용한다.
    if( (aFlag & QMO_MAKEHASH_TEMP_TABLE_MASK) ==
        QMO_MAKEHASH_MEMORY_TEMP_TABLE )
    {
        sHASH->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sHASH->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sHASH->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sHASH->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    //----------------------------------
    // mtcColumn , mtcExecute 정보의 구축
    //----------------------------------
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sHASH->myNode )
              != IDE_SUCCESS );

    //----------------------------------
    // PROJ-1473 column locate 지정.
    //----------------------------------

    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sHASH->myNode )
              != IDE_SUCCESS );

    //----------------------------------
    // PROJ-2179 calculate가 필요한 node들의 결과 위치를 설정
    //----------------------------------

    IDE_TEST( qmg::setCalcLocate( aStatement,
                                  sHASH->myNode )
              != IDE_SUCCESS );

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    for (sNewMtrNode = sHASH->myNode , sColumnCount = 0 ;
         sNewMtrNode != NULL;
         sNewMtrNode = sNewMtrNode->next , sColumnCount++ ) ;

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    //----------------------------------
    //dependency 처리 및 subquery 처리
    //----------------------------------

    sPredicate[0] = sHASH->filter;
    sMtrNode[0]  = sHASH->myNode;

    //----------------------------------
    // PROJ-1437
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate[0],
                                       sTupleID,
                                       ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sHASH->plan ,
                                            QMO_HASH_DEPENDENCY,
                                            sTupleID ,
                                            NULL ,
                                            1 ,
                                            sPredicate ,
                                            1 ,
                                            sMtrNode )
              != IDE_SUCCESS );

    // BUG-27526
    IDE_TEST_RAISE( sHASH->plan.dependency == ID_UINT_MAX,
                    ERR_INVALID_DEPENDENCY );

    sHASH->depTupleRowID = (UShort)sHASH->plan.dependency;

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sHASH->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sHASH->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sHASH->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);
    sHASH->plan.flag |= QMN_PLAN_MTR_EXIST_TRUE;

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sHASH->planID,
                               sHASH->plan.flag,
                               sMtcTemplate->rows[sTupleID].lflag,
                               sHASH->myNode,
                               &sHASH->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeHASH",
                                  "Invalid dependency" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::initGRAG( qcStatement       * aStatement ,
                         qmsQuerySet       * aQuerySet ,
                         qmsAggNode        * aAggrNode ,
                         qmsConcatElement  * aGroupColumn,
                         qmnPlan           * aParent,
                         qmnPlan          ** aPlan )
{
/***********************************************************************
 *
 * Description : GRAG 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncGRAG의 할당 및 초기화
 *     + 메인 작업
 *         - GRAG노드의 컬럼 구성 (aggregation + grouping정보)
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *     - aggregation은 연산이지만, stack에 달아주기때문에 passNode를
 *       생성하지 않는다. 그러나 grouping컬럼은 passNode를 생성하고
 *       이정보가 order by , having에 쓰일수 있으므로 srcNode를 복사
 *       하여 컬럼을 구성하고, passNode로 대체 한다.
 *
 ***********************************************************************/

    qmncGRAG          * sGRAG;
    UInt                sDataNodeOffset;
    UInt                sFlag = 0;
    qmsConcatElement  * sGroupColumn;
    qmsAggNode        * sAggrNode;
    qtcNode           * sNode;
    qmcAttrDesc       * sResultDesc;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initGRAG::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncGRAG의 할당 및 초기화
    IDU_FIT_POINT( "qmoOneMtrPlan::initGRAG::alloc::GRAG",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncGRAG ),
                                               (void **)& sGRAG )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sGRAG ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_GRAG ,
                        qmnGRAG ,
                        qmndGRAG,
                        sDataNodeOffset );

    sGRAG->plan.readyIt         = qmnGRAG::readyIt;

    *aPlan = (qmnPlan *)sGRAG;

    // BUG-41565
    // AGGR 함수에 컨버젼이 달려있으면 결과가 틀려집니다.
    // 상위 플랜에서 컨버젼이 있으면 qtcNode 를 새로 생성하기 때문에
    // 상위 플랜의 result desc 의 것을 추가해 주어야 같은 qtcNode 를 공유할수 있다.
    if ( aParent->type != QMN_GRAG )
    {
        // 상위 플랜이 GRAG 이면 추가하지 않는다. 잘못된 AGGR 을 추가하게됨
        // select max(count(i1)), sum(i1) from t1 group by i1; 일때
        // GRAG1 -> max(count(i1)), sum(i1)
        // GRAG2 -> count(i1) 가 처리된다.
        for( sResultDesc = aParent->resultDesc;
             sResultDesc != NULL;
             sResultDesc = sResultDesc->next )
        {
            // BUG-43288 외부참조 AGGR 함수는 추가하지 않는다.
            if ( ( qtc::haveDependencies( &aQuerySet->outerDepInfo ) == ID_TRUE ) &&
                 ( qtc::dependencyContains( &aQuerySet->outerDepInfo,
                                            &sResultDesc->expr->depInfo ) == ID_TRUE ) )
            {
                continue;
            }
            else
            {
                // Nothing To Do
            }

            // nested aggr x, over x
            if ( (sResultDesc->expr->overClause == NULL) &&
                 (QTC_IS_AGGREGATE( sResultDesc->expr ) == ID_TRUE ) &&
                 (QTC_HAVE_AGGREGATE2( sResultDesc->expr ) == ID_FALSE ) )
            {
                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                &sGRAG->plan.resultDesc,
                                                sResultDesc->expr,
                                                0,
                                                QMC_APPEND_EXPRESSION_TRUE,
                                                ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else
    {
        // Nothing To Do
    }

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    for( sGroupColumn = aGroupColumn;
         sGroupColumn != NULL;
         sGroupColumn = sGroupColumn->next )
    {
        IDE_TEST( qmc::appendAttribute( aStatement,
                                        aQuerySet,
                                        & sGRAG->plan.resultDesc,
                                        sGroupColumn->arithmeticOrList,
                                        sFlag,
                                        QMC_APPEND_EXPRESSION_TRUE | QMC_APPEND_ALLOW_CONST_TRUE,
                                        ID_FALSE )
                  != IDE_SUCCESS );
    }

    for( sAggrNode = aAggrNode;
         sAggrNode != NULL;
         sAggrNode = sAggrNode->next )
    {
        sNode = sAggrNode->aggr;

        /* BUG-46906 BUG-46922 */
        IDE_TEST_RAISE( ( sNode->node.module == & mtfListagg ) ||
                        ( sNode->node.module == & mtfPercentileDisc ) ||
                        ( sNode->node.module == & mtfPercentileCont ),
                        ERR_UNEXPECTED );

        while( sNode->node.module == &qtc::passModule )
        {
            // Aggregate function을 having/order by절에 참조하는 경우 pass node가 생성된다.
            sNode = (qtcNode *)sNode->node.arguments;
        }

        IDE_TEST( qmc::appendAttribute( aStatement,
                                        aQuerySet,
                                        & sGRAG->plan.resultDesc,
                                        sNode,
                                        0,
                                        QMC_APPEND_EXPRESSION_TRUE,
                                        ID_FALSE )
                  != IDE_SUCCESS );
    }

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sGRAG->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::initGRAG",
                                  "mathematics temp is not support a group aggregate plan" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeGRAG( qcStatement      * aStatement ,
                         qmsQuerySet      * aQuerySet ,
                         UInt               aFlag,
                         qmsConcatElement * aGroupColumn,
                         UInt               aBucketCount ,
                         qmnPlan          * aChildPlan ,
                         qmnPlan          * aPlan )
{
    qmncGRAG          * sGRAG = (qmncGRAG *)aPlan;

    UInt                sDataNodeOffset;

    qmcMtrNode        * sMtrNode[2];

    UShort              sTupleID;
    UShort              sColumnCount = 0;

    qmcMtrNode        * sNewMtrNode;
    qmcMtrNode        * sFirstMtrNode = NULL;
    qmcMtrNode        * sLastMtrNode = NULL;

    UShort              sAggrNodeCount = 0;
    UShort              sMtrNodeCount;
    mtcTemplate       * sMtcTemplate;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeGRAG::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndGRAG));

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    sGRAG->plan.left = aChildPlan;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sGRAG->flag      = QMN_PLAN_FLAG_CLEAR;
    sGRAG->plan.flag = QMN_PLAN_FLAG_CLEAR;

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    //----------------------------------
    // 튜플의 할당
    //----------------------------------
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    // To Fix PR-8493
    // GROUP BY 컬럼의 대체 여부를 결정하기 위해서는
    // Tuple의 저장 매체 정보를 미리 기록하고 있어야 한다.
    if( (aFlag & QMO_MAKEGRAG_TEMP_TABLE_MASK) ==
        QMO_MAKEGRAG_MEMORY_TEMP_TABLE )
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

        // BUG-23689  group by가 없는 aggregation 수행시, 저장컬럼처리 오류
        // 질의가 디스크템프테이블로 처리되던중,
        // group by가 없는 aggregation에 대한 처리시 예외적으로 메모리템프테이블사용하고,
        // 상위노드는 다시 디스크템프테이블로 처리됨.
        // 이 경우 디스크컬럼이 상위 노드로 전달될 수 있도록 처리해야 함.
        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            if( ( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
                  == QMN_PLAN_STORAGE_DISK ) &&
                ( aGroupColumn == NULL ) )
            {
                sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
                sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    // PROJ-2444 parallel aggregation
    // parallel aggregation 은 2단계로 수행된다.
    // 이를 구분하는 flag 를 설정한다.
    if ( (aFlag & QMO_MAKEGRAG_PARALLEL_STEP_MASK) ==
                  QMO_MAKEGRAG_PARALLEL_STEP_AGGR )
    {
        sGRAG->flag &= ~QMNC_GRAG_PARALLEL_STEP_MASK;
        sGRAG->flag |= QMNC_GRAG_PARALLEL_STEP_AGGR;
        sGRAG->plan.flag &= ~QMN_PLAN_GARG_PARALLEL_MASK;
        sGRAG->plan.flag |= QMN_PLAN_GARG_PARALLEL_TRUE;
    }
    else if ( (aFlag & QMO_MAKEGRAG_PARALLEL_STEP_MASK) ==
                       QMO_MAKEGRAG_PARALLEL_STEP_MERGE )
    {
        sGRAG->flag &= ~QMNC_GRAG_PARALLEL_STEP_MASK;
        sGRAG->flag |= QMNC_GRAG_PARALLEL_STEP_MERGE;
        sGRAG->plan.flag &= ~QMN_PLAN_GARG_PARALLEL_MASK;
        sGRAG->plan.flag |= QMN_PLAN_GARG_PARALLEL_TRUE;
    }
    else
    {
        sGRAG->flag &= ~QMNC_GRAG_PARALLEL_STEP_MASK;
        sGRAG->plan.flag &= ~QMN_PLAN_GARG_PARALLEL_MASK;
        sGRAG->plan.flag |= QMN_PLAN_GARG_PARALLEL_FALSE;
    }

    //----------------------------------
    // myNode의 구성
    // - for aggregation
    //----------------------------------
    sGRAG->myNode = NULL;

    //----------------------------------
    // PROJ-1473
    // aggrNode에 대한 mtrNode 구성전에
    // 질의에 사용된 컬럼에 대한 저장
    //----------------------------------

    sGRAG->baseTableCount = 0;

    for( sItrAttr = aPlan->resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        // Grouping key가 아닌 경우(aggregation)
        if( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_FALSE )
        {
            sAggrNodeCount++;

            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_CALCULATE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // group by에는 TEMP_VAR_TYPE를 사용하지 않는다.
            sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;

            if( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
        }
    }

    for( sItrAttr = aPlan->resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        // Grouping key인 경우
        if( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;

            sNewMtrNode->flag &= ~QMC_MTR_GROUPING_MASK;
            sNewMtrNode->flag |= QMC_MTR_GROUPING_TRUE;

            sNewMtrNode->flag &= ~QMC_MTR_MTR_PLAN_MASK;
            sNewMtrNode->flag |= QMC_MTR_MTR_PLAN_TRUE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // group by에는 TEMP_VAR_TYPE를 사용하지 않는다.
            sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;

            if( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
        }
    }

    sGRAG->myNode = sFirstMtrNode;

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------
    IDE_TEST( qtc::allocIntermediateTuple( aStatement ,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sTupleID ,
                                           sColumnCount )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    //GRAPH에서 지정한 저장매체를 사용한다.
    if( (aFlag & QMO_MAKEGRAG_TEMP_TABLE_MASK) ==
        QMO_MAKEGRAG_MEMORY_TEMP_TABLE )
    {
        sGRAG->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sGRAG->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

        // BUG-23689  group by가 없는 aggregation 수행시, 저장컬럼처리 오류
        // 질의가 디스크템프테이블로 처리되던중,
        // group by가 없는 aggregation에 대한 처리시 예외적으로 메모리템프테이블사용하고,
        // 상위노드는 다시 디스크템프테이블로 처리됨.
        // 이 경우 디스크컬럼이 상위 노드로 전달될 수 있도록 처리해야 함.
        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {        
            if( ( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
                  == QMN_PLAN_STORAGE_DISK ) &&
                ( aGroupColumn == NULL ) )
            {            
                sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
                sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;                
            }
            else
            {
                // Nothing To Do 
            }
        }
        else
        {
            // Nothing To Do 
        }
    }
    else
    {
        sGRAG->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sGRAG->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    //----------------------------------
    // mtcColumn , mtcExecute 정보의 구축
    //----------------------------------
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sGRAG->myNode )
              != IDE_SUCCESS);

    // PROJ-2444 parallel aggregation
    // qmg::setColumnLocate 함수에서 템플릿의 columnLocate 를 변경한다.
    // GRAG 플랜을 여러번 생성하는 과정에서
    // 변경된 columnLocate를 참조하면 잘못된 플랜이 생성된다.
    if ( (aFlag & QMO_MAKEGRAG_PARALLEL_STEP_MASK) !=
                  QMO_MAKEGRAG_PARALLEL_STEP_AGGR )
    {
        //----------------------------------
        // PROJ-1473 column locate 지정.
        //----------------------------------

        IDE_TEST( qmg::setColumnLocate( aStatement,
                                        sGRAG->myNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    //----------------------------------
    // PROJ-2179 calculate가 필요한 node들의 결과 위치를 설정
    //----------------------------------

    IDE_TEST( qmg::setCalcLocate( aStatement,
                                  sGRAG->myNode )
              != IDE_SUCCESS );

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    // 저장 Column의 data 영역 지정
    sGRAG->mtrNodeOffset = sDataNodeOffset;

    for( sNewMtrNode = sGRAG->myNode , sMtrNodeCount = 0 ;
         sNewMtrNode != NULL ;
         sNewMtrNode = sNewMtrNode->next , sMtrNodeCount++ ) ;

    sDataNodeOffset += sMtrNodeCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    // Aggregation Column 의 data 영역 지정
    sGRAG->aggrNodeOffset = sDataNodeOffset;

    // Data 영역 Size 조정
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sAggrNodeCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    //----------------------------------
    //dependency 처리 및 subquery 처리
    //----------------------------------

    sMtrNode[0]  = sGRAG->myNode;
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sGRAG->plan ,
                                            QMO_GRAG_DEPENDENCY,
                                            sTupleID ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            1 ,
                                            sMtrNode )
              != IDE_SUCCESS );

    // BUG-27526
    IDE_TEST_RAISE( sGRAG->plan.dependency == ID_UINT_MAX,
                    ERR_INVALID_DEPENDENCY );

    sGRAG->depTupleRowID = (UShort)sGRAG->plan.dependency;

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sGRAG->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sGRAG->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sGRAG->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);
    sGRAG->plan.flag |= QMN_PLAN_MTR_EXIST_TRUE;

    // PROJ-2444
    if ( (aFlag & QMO_MAKEGRAG_PARALLEL_STEP_MASK) ==
                  QMO_MAKEGRAG_PARALLEL_STEP_AGGR )
    {
        if ( aBucketCount == 1 )
        {
            sGRAG->bucketCnt = aBucketCount;
        }
        else
        {
            sGRAG->bucketCnt = IDL_MAX( (aBucketCount / sGRAG->plan.mParallelDegree), 1024 );
        }
    }
    else
    {
        sGRAG->bucketCnt     = aBucketCount;
    }

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sGRAG->planID,
                               sGRAG->plan.flag,
                               sMtcTemplate->rows[sTupleID].lflag,
                               sGRAG->myNode,
                               &sGRAG->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeGRAG",
                                  "Invalid dependency" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::initHSDS( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         idBool         aExplicitDistinct,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : HSDS 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncHSDS의 할당 및 초기화
 *     + 메인 작업
 *         - HSDS노드의 사용구분
 *         - HSDS노드의 컬럼 구성
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *     - HSDS노드의 컬럼은 baseTable을 생성하지 않는다.
 *     - 컬럼의 구성은 다음과 같이 3가지로 한다. 이에따라 인터페이스를
 *       구분해주도록 한다. (private으로 처리)
 *         - HASH-BASED DISTINCTION (target정보 이용)
 *         - SET-UNION으로 쓰이는 경우 (하위의 VIEW노드 이용)
 *         - IN연산자의 SUBQUERY KEYRANGE (하위의 VIEW노드 이용)
 *     - HSDS 설계문서에는 반영되지 않았지만, HASH-BASED DISTINCTION으로
 *       이용될 시에는 target의 정보를 이용하고나서는 생성된 dstNode 또는
 *       passNode를 target에 다시 달아주어야 상위의 SORT노드 등에서 바로
 *       이용이 가능하다.
 *
 ***********************************************************************/

    qmncHSDS          * sHSDS;
    UInt                sDataNodeOffset;
    UInt                sFlag = 0;
    UInt                sOption = 0;
    idBool              sAppend;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initHSDS::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncHSDS의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncHSDS ),
                                               (void **)& sHSDS )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sHSDS ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_HSDS ,
                        qmnHSDS ,
                        qmndHSDS,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sHSDS;

    sOption &= ~QMC_APPEND_EXPRESSION_MASK;
    sOption |= QMC_APPEND_EXPRESSION_TRUE;

    sOption &= ~QMC_APPEND_ALLOW_CONST_MASK;
    sOption |= QMC_APPEND_ALLOW_CONST_TRUE;

    sOption &= ~QMC_APPEND_ALLOW_DUP_MASK;
    sOption |= QMC_APPEND_ALLOW_DUP_FALSE;

    if( aExplicitDistinct == ID_TRUE )
    {
        // SELECT DISTINCT절을 사용한 경우
        for( sItrAttr = aParent->resultDesc;
             sItrAttr != NULL;
             sItrAttr = sItrAttr->next )
        {
            if( ( sItrAttr->flag & QMC_ATTR_DISTINCT_MASK )
                == QMC_ATTR_DISTINCT_TRUE )
            {
                sAppend = ID_TRUE;
            }
            else
            {
                sAppend = ID_FALSE;
            }

            if( sAppend == ID_TRUE )
            {
                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                & sHSDS->plan.resultDesc,
                                                sItrAttr->expr,
                                                sFlag,
                                                sOption,
                                                ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }

        // DISTINCT절에서 참조되었다는 flag를 더 이상 물려받지 않도록 해제한다.
        for( sItrAttr = sHSDS->plan.resultDesc;
             sItrAttr != NULL;
             sItrAttr = sItrAttr->next )
        {
            sItrAttr->flag &= ~QMC_ATTR_DISTINCT_MASK;
            sItrAttr->flag |= QMC_ATTR_DISTINCT_FALSE;
        }

        IDE_TEST( qmc::makeReferenceResult( aStatement,
                                            ( aParent->type == QMN_PROJ ? ID_TRUE : ID_FALSE ),
                                            aParent->resultDesc,
                                            sHSDS->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // IN-SUBQUERY KEYRANGE 또는 UNION절 사용시
        // 하위에 VIEW가 존재하므로 상위 PROJECTION의 결과를 복사한다.
        IDE_TEST( qmc::copyResultDesc( aStatement,
                                       aParent->resultDesc,
                                       & sHSDS->plan.resultDesc )
                  != IDE_SUCCESS );
    }

    IDE_DASSERT( sHSDS->plan.resultDesc != NULL );

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sHSDS->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeHSDS( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         UInt           aFlag ,
                         UInt           aBucketCount ,
                         qmnPlan      * aChildPlan ,
                         qmnPlan      * aPlan )
{
    qmncHSDS          * sHSDS = (qmncHSDS *)aPlan;
    UInt                sDataNodeOffset;
    qmcMtrNode        * sMtrNode[2];

    UShort              sTupleID;
    UShort              sColumnCount;

    qmcMtrNode        * sNewMtrNode;
    qmcMtrNode        * sLastMtrNode = NULL;

    mtcTemplate       * sMtcTemplate;

    qmcMtrNode        * sFirstMtrNode = NULL;

    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeHSDS::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndHSDS));

    sHSDS->plan.left = aChildPlan;

    sHSDS->mtrNodeOffset = sDataNodeOffset;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sHSDS->flag      = QMN_PLAN_FLAG_CLEAR;
    sHSDS->plan.flag = QMN_PLAN_FLAG_CLEAR;

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    sHSDS->bucketCnt     = aBucketCount;
    sHSDS->myNode        = NULL;

    //----------------------------------
    // 튜플의 할당
    //----------------------------------
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    // To Fix PR-8493
    // 컬럼의 대체 여부를 결정하기 위해서는
    // Tuple의 저장 매체 정보를 미리 기록하고 있어야 한다.
    if( (aFlag & QMO_MAKEHSDS_TEMP_TABLE_MASK) ==
        QMO_MAKEHSDS_MEMORY_TEMP_TABLE )
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    //bug-7405 fixed
    if( QC_SHARED_TMPLATE(aStatement)->stmt == aStatement )
    {
        // 최상위 Query에서 사용되는 경우
        sHSDS->flag  &= ~QMNC_HSDS_IN_TOP_MASK;
        sHSDS->flag  |= QMNC_HSDS_IN_TOP_TRUE;

        // BUG-31997: When using temporary tables by RID, RID refers
        // to the invalid row.
        sHSDS->plan.flag  &= ~QMN_PLAN_TEMP_FIXED_RID_MASK;
        sHSDS->plan.flag  |= QMN_PLAN_TEMP_FIXED_RID_TRUE;
    }
    else
    {
        // 최상위 Query가 아닌경우
        sHSDS->flag  &= ~QMNC_HSDS_IN_TOP_MASK;
        sHSDS->flag  |= QMNC_HSDS_IN_TOP_FALSE;

        // BUG-31997: When using temporary tables by RID, RID refers
        // to the invalid row.
        sHSDS->plan.flag  &= ~QMN_PLAN_TEMP_FIXED_RID_MASK;
        sHSDS->plan.flag  |= QMN_PLAN_TEMP_FIXED_RID_FALSE;
    }

    //----------------------------------
    // PROJ-1473
    // mtrNode 구성전에
    // 질의에 사용된 컬럼에 대한 저장
    //----------------------------------

    //----------------------------------
    // 1473TODO
    // 메모리테이블에 대한 베이스테이블생성제거필요
    // 즉, 질의에 사용된 컬럼정보만 저장 필요.
    //----------------------------------

    sHSDS->baseTableCount = 0;

    sColumnCount = 0;

    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                          aQuerySet,
                                          sItrAttr->expr,
                                          ID_FALSE,
                                          sTupleID,
                                          0,
                                          & sColumnCount,
                                          & sNewMtrNode )
                  != IDE_SUCCESS );

        sNewMtrNode->flag &= ~QMC_MTR_HASH_NEED_MASK;
        sNewMtrNode->flag |= QMC_MTR_HASH_NEED_TRUE;

        if( sFirstMtrNode == NULL )
        {
            sFirstMtrNode       = sNewMtrNode;
            sLastMtrNode        = sNewMtrNode;
        }
        else
        {
            sLastMtrNode->next  = sNewMtrNode;
            sLastMtrNode        = sNewMtrNode;
        }
    }

    sHSDS->myNode = sFirstMtrNode;

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------
    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sTupleID ,
                                           sColumnCount )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    //GRAPH에서 지정한 저장매체를 사용한다.
    if( (aFlag & QMO_MAKEHSDS_TEMP_TABLE_MASK) ==
        QMO_MAKEHSDS_MEMORY_TEMP_TABLE )
    {
        sHSDS->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sHSDS->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sHSDS->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sHSDS->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    //----------------------------------
    // mtcColumn , mtcExecute 정보의 구축
    //----------------------------------
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sHSDS->myNode )
              != IDE_SUCCESS);

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    for (sNewMtrNode = sHSDS->myNode , sColumnCount = 0 ;
         sNewMtrNode != NULL;
         sNewMtrNode = sNewMtrNode->next , sColumnCount++ ) ;
    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    //----------------------------------
    //dependency 처리 및 subquery 처리
    //----------------------------------

    sMtrNode[0]  = sHSDS->myNode;
    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sHSDS->plan,
                                            QMO_HSDS_DEPENDENCY,
                                            sTupleID,
                                            NULL,
                                            0,
                                            NULL,
                                            1,
                                            sMtrNode )
                 != IDE_SUCCESS );

    // BUG-27526
    IDE_TEST_RAISE( sHSDS->plan.dependency == ID_UINT_MAX,
                    ERR_INVALID_DEPENDENCY );

    sHSDS->depTupleRowID = (UShort)sHSDS->plan.dependency;

    //----------------------------------
    // PROJ-1473 column locate 지정.
    //----------------------------------

    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sHSDS->myNode )
              != IDE_SUCCESS );

    //----------------------------------
    // PROJ-2179 calculate가 필요한 node들의 결과 위치를 설정
    //----------------------------------

    IDE_TEST( qmg::setCalcLocate( aStatement,
                                  sHSDS->myNode )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sHSDS->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sHSDS->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sHSDS->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sHSDS->planID,
                               sHSDS->plan.flag,
                               sMtcTemplate->rows[sTupleID].lflag,
                               sHSDS->myNode,
                               &sHSDS->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeHSDS",
                                  "Invalid dependency" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::initLMST( qcStatement    * aStatement ,
                         qmsQuerySet    * aQuerySet ,
                         UInt             aFlag ,
                         qmsSortColumns * aOrderBy ,
                         qmnPlan        * aParent ,
                         qmnPlan       ** aPlan )
{
/***********************************************************************
 *
 * Description : LMST 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncLMST의 할당 및 초기화
 *     + 메인 작업
 *         - LMST노드의 사용구분
 *         - LMST노드의 컬럼 구성
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *     - 컬럼의 구성은 다음과 같이 3가지로 한다. 이에따라 인터페이스를
 *       구분해주도록 한다. (private으로 처리)
 *         - LIMIT ORDER BY로 쓰이는 경우 (orderBy정보 이용)
 *         - STORE AND SEARCH (하위 VIEW의 정보)
 *
 ***********************************************************************/

    qmncLMST          * sLMST;
    qmsSortColumns    * sSortColumn;
    qmcAttrDesc       * sItrAttr;
    qtcNode           * sNode;
    UInt                sDataNodeOffset;
    UInt                sColumnCount;
    UInt                sFlag = 0;
    SInt                i;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initLMST::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParent != NULL );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncLMST ),
                                               (void **)& sLMST )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sLMST ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_LMST ,
                        qmnLMST ,
                        qmndLMST,
                        sDataNodeOffset );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sLMST->flag      = QMN_PLAN_FLAG_CLEAR;

    // Sorting이 필요함을 표시
    sLMST->flag &= ~QMNC_LMST_USE_MASK;
    sLMST->flag |= QMNC_LMST_USE_ORDERBY;

    *aPlan = (qmnPlan *)sLMST;

    // Sorting key임을 표시한다.
    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    // Order by절의 attribute들을 추가한다.
    for( sSortColumn = aOrderBy, sColumnCount = 0;
         sSortColumn != NULL;
         sSortColumn = sSortColumn->next, sColumnCount++ )
    {
        // Sorting 방향을 표시한다.
        if( sSortColumn->isDESC == ID_FALSE )
        {
            sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_ORDER_ASC;
        }
        else
        {
            sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_ORDER_DESC;
        }

        /* PROJ-2435 order by nulls first/last */
        if ( sSortColumn->nullsOption != QMS_NULLS_NONE )
        {
            if ( sSortColumn->nullsOption == QMS_NULLS_FIRST )
            {
                sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
                sFlag |= QMC_ATTR_SORT_NULLS_ORDER_FIRST;
            }
            else
            {
                sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
                sFlag |= QMC_ATTR_SORT_NULLS_ORDER_LAST;
            }
        }
        else
        {
            sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_NULLS_ORDER_NONE;
        }

        /* BUG-36826 A rollup or cube occured wrong result using order by count_i3 */
        if ( ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK ) == QMO_MAKESORT_VALUE_TEMP_TRUE ) ||
             ( ( aFlag & QMO_MAKESORT_GROUP_EXT_VALUE_MASK ) == QMO_MAKESORT_GROUP_EXT_VALUE_TRUE ) )
        {
            for ( i = 1, sItrAttr = aParent->resultDesc;
                  i < sSortColumn->targetPosition;
                  i++ )
            {
                sItrAttr = sItrAttr->next;
            }

            if ( sItrAttr != NULL )
            {
                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                & sLMST->plan.resultDesc,
                                                sItrAttr->expr,
                                                sFlag,
                                                QMC_APPEND_EXPRESSION_TRUE,
                                                ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sLMST->plan.resultDesc,
                                            sSortColumn->sortColumn,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
    }

    // ORDER BY 절 외 참조되는 attribute들을 추가한다.
    for ( sItrAttr = aParent->resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next )
    {
        /* BUG-37146 together with rollup order by the results are wrong.
         * Value Temp의 상위에 SortTemp가 있을 경우 모든 SortTemp는 value temp를
         * 참조해야한다. 따라서 상위 Plan의 exression을 모두 sort에 추가하고 추후
         * pushResultDesc 함수에서 passNode를 만들도록 유도한다.
         */
        if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK ) ==
             QMO_MAKESORT_VALUE_TEMP_TRUE )
        {
            if ( ( sItrAttr->expr->lflag & QTC_NODE_AGGREGATE_MASK ) ==
                 QTC_NODE_AGGREGATE_EXIST )
            {
                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                & sLMST->plan.resultDesc,
                                                sItrAttr->expr,
                                                QMC_ATTR_SEALED_TRUE,
                                                QMC_APPEND_EXPRESSION_TRUE,
                                                ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        /* BUG-35265 wrong result desc connect_by_root, sys_connect_by_path */
        if ( ( sItrAttr->expr->node.lflag & MTC_NODE_FUNCTION_CONNECT_BY_MASK )
             == MTC_NODE_FUNCTION_CONNECT_BY_TRUE )
        {
            /* 복사해서 넣지 않으면 상위 Parent 가 바뀔때 잘못된 Tuple을 가르키게된다 */
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                       (void**)&sNode )
                      != IDE_SUCCESS );
            idlOS::memcpy( sNode, sItrAttr->expr, ID_SIZEOF( qtcNode ) );

            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sLMST->plan.resultDesc,
                                            sNode,
                                            QMC_ATTR_SEALED_TRUE,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do*/
        }
    }

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ( aParent->type == QMN_PROJ ? ID_TRUE : ID_FALSE ),
                                   aParent->resultDesc,
                                   &sLMST->plan.resultDesc )
              != IDE_SUCCESS );

    // BUG-36997
    // select distinct i1, i2+1 from t1 order by i1;
    // distinct 를 처리할때 i2+1 을 연산해서 저장해 놓는다.
    // 따라서 PASS 노드를 생성해주어야 한다.
    IDE_TEST( qmc::makeReferenceResult( aStatement,
                                        ( aParent->type == QMN_PROJ ? ID_TRUE : ID_FALSE ),
                                        aParent->resultDesc,
                                        sLMST->plan.resultDesc )
              != IDE_SUCCESS );

    // ORDER BY절에서 참조되었다는 flag를 더 이상 물려받지 않도록 해제한다.
    for ( sItrAttr  = sLMST->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        sItrAttr->flag &= ~QMC_ATTR_ORDER_BY_MASK;
        sItrAttr->flag |= QMC_ATTR_ORDER_BY_FALSE;
    }

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sLMST->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::initLMST( qcStatement    * aStatement ,
                         qmsQuerySet    * aQuerySet ,
                         qmnPlan        * aParent ,
                         qmnPlan       ** aPlan )
{
    qmncLMST          * sLMST;
    UInt                sDataNodeOffset;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initLMST::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncLMST ),
                                               (void **)& sLMST )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sLMST ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_LMST ,
                        qmnLMST ,
                        qmndLMST,
                        sDataNodeOffset );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sLMST->flag = QMN_PLAN_FLAG_CLEAR;
    sLMST->flag &= ~QMNC_LMST_USE_MASK;
    sLMST->flag |= QMNC_LMST_USE_STORESEARCH;

    *aPlan = (qmnPlan *)sLMST;

    IDE_TEST( qmc::copyResultDesc( aStatement,
                                   aParent->resultDesc,
                                   & sLMST->plan.resultDesc )
              != IDE_SUCCESS );

    for( sItrAttr = sLMST->plan.resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        sItrAttr->flag &= ~QMC_ATTR_KEY_MASK;
        sItrAttr->flag |= QMC_ATTR_KEY_TRUE;

        sItrAttr->flag &= ~QMC_ATTR_SORT_ORDER_MASK;
        sItrAttr->flag |= QMC_ATTR_SORT_ORDER_ASC;
    }

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sLMST->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeLMST( qcStatement    * aStatement ,
                         qmsQuerySet    * aQuerySet ,
                         ULong            aLimitRowCount ,
                         qmnPlan        * aChildPlan ,
                         qmnPlan        * aPlan )
{
    qmncLMST          * sLMST = (qmncLMST*)aPlan;
    UInt                sDataNodeOffset;
    qmcMtrNode        * sMtrNode[2];

    UShort              sTupleID;
    UShort              sColumnCount = 0;
    qmcMtrNode        * sNewMtrNode = NULL;
    qmcMtrNode        * sFirstMtrNode = NULL;
    qmcMtrNode        * sLastMtrNode = NULL;

    mtcTemplate       * sMtcTemplate;

    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeLMST::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildPlan != NULL );
    IDE_DASSERT( aPlan != NULL );

    //qmncLMST의 할당 및 초기화
    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndLMST));

    sLMST->plan.left = aChildPlan;

    sLMST->mtrNodeOffset = sDataNodeOffset;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sLMST->plan.flag = QMN_PLAN_FLAG_CLEAR;

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    sLMST->limitCnt      = aLimitRowCount;

    //----------------------------------
    // 튜플의 할당
    //----------------------------------
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

    if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
    {
        if( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
            == QMN_PLAN_STORAGE_DISK )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sLMST->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     &( aQuerySet->depInfo ) )
              != IDE_SUCCESS );

    // Sorting key가 아닌 경우
    IDE_TEST( makeNonKeyAttrsMtrNodes( aStatement,
                                       aQuerySet,
                                       sLMST->plan.resultDesc,
                                       sTupleID,
                                       & sFirstMtrNode,
                                       & sLastMtrNode,
                                       & sColumnCount )
              != IDE_SUCCESS );

    sLMST->baseTableCount = sColumnCount;

    // PROJ-2362 memory temp 저장 효율성 개선
    // limit sort에는 TEMP_VAR_TYPE를 사용하지 않는다.
    for( sNewMtrNode = sFirstMtrNode;
         sNewMtrNode != NULL;
         sNewMtrNode = sNewMtrNode->next )
    {
        sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
        sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;
    }

    for( sItrAttr = aPlan->resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        // Sorting key 인 경우
        if( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              &sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_SORT_NEED_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_NEED_TRUE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // limit sort에는 TEMP_VAR_TYPE를 사용하지 않는다.
            sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;

            if( ( sItrAttr->flag & QMC_ATTR_SORT_ORDER_MASK ) == QMC_ATTR_SORT_ORDER_ASC )
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
            }
            else
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_DESCENDING;
            }

            /* PROJ-2435 order by nulls first/last */
            if ( ( sItrAttr->flag & QMC_ATTR_SORT_NULLS_ORDER_MASK )
                 != QMC_ATTR_SORT_NULLS_ORDER_NONE )
            {
                if ( ( sItrAttr->flag & QMC_ATTR_SORT_NULLS_ORDER_MASK )
                     == QMC_ATTR_SORT_NULLS_ORDER_FIRST )
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_FIRST;
                }
                else
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_LAST;
                }
            }
            else
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_NONE;
            }

            if( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    sLMST->myNode = sFirstMtrNode;

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------
    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sTupleID ,
                                           sColumnCount )
              != IDE_SUCCESS);

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    //LMST는 memory로 저장매체를 사용한다.
    sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    sLMST->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
    sLMST->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;

    if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
    {
        if( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
            == QMN_PLAN_STORAGE_DISK )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------
    // mtcColumn , mtcExecute 정보의 구축
    //----------------------------------
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sLMST->myNode )
              != IDE_SUCCESS);

    //----------------------------------
    // PROJ-1473 column locate 지정.
    //----------------------------------

    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sLMST->myNode )
              != IDE_SUCCESS );

    //----------------------------------
    // PROJ-2179 calculate가 필요한 node들의 결과 위치를 설정
    //----------------------------------

    IDE_TEST( qmg::setCalcLocate( aStatement,
                                  sLMST->myNode )
              != IDE_SUCCESS );

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    for ( sNewMtrNode  = sLMST->myNode , sColumnCount = 0 ;
          sNewMtrNode != NULL;
          sNewMtrNode  = sNewMtrNode->next , sColumnCount++ ) ;

    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    //----------------------------------
    //dependency 처리 및 subquery 처리
    //----------------------------------

    sMtrNode[0]  = sLMST->myNode;
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sLMST->plan ,
                                            QMO_LMST_DEPENDENCY,
                                            sTupleID ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            1 ,
                                            sMtrNode )
              != IDE_SUCCESS );

    // BUG-27526
    IDE_TEST_RAISE( sLMST->plan.dependency == ID_UINT_MAX,
                    ERR_INVALID_DEPENDENCY );

    sLMST->depTupleRowID = (UShort)sLMST->plan.dependency;

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sLMST->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sLMST->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sLMST->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sLMST->planID,
                               sLMST->plan.flag,
                               sMtcTemplate->rows[sTupleID].lflag,
                               sLMST->myNode,
                               &sLMST->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeLMST",
                                  "Invalid dependency" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::initVMTR( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmnPlan      * aParent ,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : VMTR 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncVMTR의 할당 및 초기화
 *     + 메인 작업
 *         - VMTR노드의 컬럼 구성(하위 VIEW의 정보)
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 ***********************************************************************/

    qmncVMTR          * sVMTR;
    UInt                sDataNodeOffset;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initVMTR::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncVMTR의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncVMTR ),
                                               (void **)& sVMTR )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sVMTR ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_VMTR ,
                        qmnVMTR ,
                        qmndVMTR,
                        sDataNodeOffset );

    IDE_TEST( qmc::createResultFromQuerySet( aStatement,
                                             aQuerySet,
                                             & sVMTR->plan.resultDesc )
              != IDE_SUCCESS );

    /* BUG-49095 */
    for ( sItrAttr  = sVMTR->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        IDE_TEST_RAISE( ( sItrAttr->expr->lflag & QTC_NODE_COLUMN_RID_MASK ) == QTC_NODE_COLUMN_RID_EXIST,
                        ERR_PROWID_NOT_SUPPORTED );
    }

    if ( aParent != NULL )
    {
        /* PROJ-2462 Result Cache
         * Top Result Cache의 경우 VMTR의 상위에 항상 * Projection이 생성된다.
         */
        if ( aParent->type == QMN_PROJ )
        {
            IDE_TEST( qmo::initResultCacheStack( aStatement,
                                                 aQuerySet,
                                                 sVMTR->planID,
                                                 ID_TRUE,
                                                 ID_TRUE )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmo::initResultCacheStack( aStatement,
                                                 aQuerySet,
                                                 sVMTR->planID,
                                                 ID_FALSE,
                                                 ID_TRUE )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_TEST( qmo::initResultCacheStack( aStatement,
                                             aQuerySet,
                                             sVMTR->planID,
                                             ID_FALSE,
                                             ID_TRUE )
                  != IDE_SUCCESS );
    }

    *aPlan = (qmnPlan *)sVMTR;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_PROWID_NOT_SUPPORTED)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeVMTR( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         UInt           aFlag ,
                         qmnPlan      * aChildPlan ,
                         qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : VMTR 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncVMTR의 할당 및 초기화
 *     + 메인 작업
 *         - VMTR노드의 컬럼 구성(하위 VIEW의 정보)
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 ***********************************************************************/

    qmncVMTR          * sVMTR = (qmncVMTR*)aPlan;
    UInt                sDataNodeOffset;
    qmcMtrNode        * sMtrNode[2];

    UShort              sTupleID;
    UShort              sColumnCount = 0;
    qtcNode           * sNode;
    qmcMtrNode        * sNewMtrNode;
    qmcMtrNode        * sLastMtrNode = NULL;

    mtcTemplate       * sMtcTemplate;

    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeVMTR::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildPlan != NULL );
    IDE_DASSERT( aChildPlan->type == QMN_VIEW );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndVMTR));

    sVMTR->plan.left        = aChildPlan;

    sVMTR->mtrNodeOffset = sDataNodeOffset;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sVMTR->flag      = QMN_PLAN_FLAG_CLEAR;
    sVMTR->plan.flag = QMN_PLAN_FLAG_CLEAR;

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    sVMTR->myNode    = NULL;

    //----------------------------------
    // 튜플의 할당
    //----------------------------------

    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    // To Fix PR-8493
    // 컬럼의 대체 여부를 결정하기 위해서는
    // Tuple의 저장 매체 정보를 미리 기록하고 있어야 한다.
    if ( (aFlag & QMO_MAKEVMTR_TEMP_TABLE_MASK) ==
         QMO_MAKEVMTR_MEMORY_TEMP_TABLE )
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;
    }

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sVMTR->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     &( aQuerySet->depInfo ) )
                 != IDE_SUCCESS );

    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qtcNode, & sNode )
                  != IDE_SUCCESS);

        idlOS::memcpy( sNode,
                       sItrAttr->expr,
                       ID_SIZEOF(qtcNode) );

        IDE_TEST( qmg::makeColumnMtrNode( aStatement ,
                                          aQuerySet ,
                                          sItrAttr->expr,
                                          ID_FALSE,
                                          sTupleID ,
                                          0,
                                          & sColumnCount ,
                                          & sNewMtrNode )
                  != IDE_SUCCESS );

        if ( ( sItrAttr->flag & QMC_ATTR_USELESS_RESULT_MASK ) == QMC_ATTR_USELESS_RESULT_TRUE )
        {
            // PROJ-2469 Optimize View Materialization
            // 상위에서 사용하지 않는 MtrNode에 대해서 flag표시한다.
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |=  QMC_MTR_TYPE_USELESS_COLUMN;
        }
        else
        {
            //VMTR에서 생성된 qmcMtrNode이므로 복사하도록 한다.
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;
        }
        
        // PROJ-2179
        sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
        sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE;

        // 상위에서 temp table의 값을 참조하도록 변경된 위치를 설정한다.
        sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
        sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;

        //connect
        if( sVMTR->myNode == NULL )
        {
            sVMTR->myNode = sNewMtrNode;
            sLastMtrNode  = sNewMtrNode;
        }
        else
        {
            sLastMtrNode->next = sNewMtrNode;
            sLastMtrNode       = sNewMtrNode;
        }
    }

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------
    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                           sTupleID ,
                                           sColumnCount )
              != IDE_SUCCESS);

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    //GRAPH에서 지정한 저장매체를 사용한다.
    if ( (aFlag & QMO_MAKEVMTR_TEMP_TABLE_MASK) ==
         QMO_MAKEVMTR_MEMORY_TEMP_TABLE )
    {
        sVMTR->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sVMTR->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sVMTR->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sVMTR->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;
    }

    //----------------------------------
    // mtcColumn , mtcExecute 정보의 구축
    //----------------------------------
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sVMTR->myNode )
              != IDE_SUCCESS);

    //----------------------------------
    // PROJ-1473 column locate 지정.
    //----------------------------------

    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sVMTR->myNode )
              != IDE_SUCCESS );

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    //----------------------------------
    //dependency 처리 및 subquery 처리
    //----------------------------------

    sMtrNode[0]  = sVMTR->myNode;
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sVMTR->plan ,
                                            QMO_VMTR_DEPENDENCY,
                                            sTupleID ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            1 ,
                                            sMtrNode )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sVMTR->plan.dependency == ID_UINT_MAX,
                    ERR_INVALID_DEPENDENCY );

    sVMTR->depTupleID = ( UShort )sVMTR->plan.dependency;

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sVMTR->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sVMTR->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sVMTR->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    /* PROJ-2462 Result Cache */
    if ( ( aFlag & QMO_MAKEVMTR_TOP_RESULT_CACHE_MASK )
         == QMO_MAKEVMTR_TOP_RESULT_CACHE_TRUE )
    {
        qmo::makeResultCacheStack( aStatement,
                                   aQuerySet,
                                   sVMTR->planID,
                                   sVMTR->plan.flag,
                                   sMtcTemplate->rows[sTupleID].lflag,
                                   sVMTR->myNode,
                                   &sVMTR->componentInfo,
                                   ID_TRUE );
    }
    else
    {
        qmo::makeResultCacheStack( aStatement,
                                   aQuerySet,
                                   sVMTR->planID,
                                   sVMTR->plan.flag,
                                   sMtcTemplate->rows[sTupleID].lflag,
                                   sVMTR->myNode,
                                   &sVMTR->componentInfo,
                                   ID_FALSE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeVMTR",
                                  "Invalid dependency" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::initWNST( qcStatement        * aStatement,
                         qmsQuerySet        * aQuerySet,
                         UInt                 aSortKeyCount,
                         qmsAnalyticFunc   ** aAnalyticFuncListPtrArr,
                         UInt                 aFlag,
                         qmnPlan            * aParent,
                         qmnPlan           ** aPlan )
{
/***********************************************************************
 *
 * Description : WNST 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncWNST의 할당 및 초기화
 *     + 메인 작업
 *         - 검색 방법 및 저장 방식 flag 세팅
 *         - WNST노드의 컬럼 구성
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 *    Graph에서 넘겨주는 analytic function list ptr array 형태
 *    ex ) SELECT i,
 *                sum(i1) over ( partition by i1 ),                 ==> a1
 *                sum(i2) over ( partition by i1,i2 ),              ==> a2
 *                sum(i2) over ( partition by i1 ),                 ==> a3
 *                sum(i3) over ( partition by i3 ),                 ==> a4
 *                sum(i2) over ( partition by i1 order by i2 )      ==> a5
 *                sum(i2) over ( partition by i1 order by i2 desc ) ==> a6
 *                sum(i3) over ( partition by i1 order by i2 )      ==> a7
 *         FROM t1;
 *
 *       < Sorting Key Pointer Array >
 *        +-------------------+
 *        |    |    |    |    |
 *        +-|----|-----|----|-+
 *          |    |     |    |
 *          |    |     |   (i1)-(i2 desc)
 *          |    |     |
 *          |   (i3)  (i1)-(i2 asc)
 *          |
 *        (i1)-(i2)
 *
 *       < Analytic Function Pointer Array >
 *        +-------------------+
 *        |    |    |    |    |
 *        +-|----|----|----|--+
 *          |    |    |    |
 *          |    |    |   (a6)
 *          |    |    |
 *          |    |   (a5)-(a7)
 *          |   (a4)
 *          |
 *        (a1)-(a2)-(a3)
 *
 * < 이 함수 수행 결과로 생기는 WNST plan >
 * +--------+
 * | WNST   |
 * +--------+
 * | myNode |-->(baseTable/ -(i1)-(i2)-(i3)-(i2 asc)-(i2 desc)-(sum(i1))-(sum(i2))-(sum(i2))-(sum(i3))-(sum(i2))-(sum(i2))-(sum(i3))
 * |        |   baseColumn)
 * |        |
 * |        |
 * |        |   +---------------+
 * | wndNode|-->| wndNode       |
 * |        |   +---------------+
 * |        |   | overColumnNode|-->(i1,i2)
 * |        |   | aggrNode      |-->(sum(i1))-(sum(i2))-(sum(i2))
 * |        |   | aggrResultNode|-->(sum(i1))-(sum(i2))-(sum(i2))
 * |        |   | next ---+     |
 * |        |   +--------/------+
 * |        |   +-------V-------+
 * |        |   | wndNode       |
 * |        |   +---------------+
 * |        |   | overColumnNode|-->(i3)
 * |        |   | aggrNode      |-->(sum(i3))
 * |        |   | aggrResultNode|-->(sum(i3))
 * |        |   | next          |
 * |        |   +--------/------+
 * |        |   +-------V-------+
 * |        |   | wndNode       |
 * |        |   +---------------+
 * |        |   | overColumnNode|-->(i1,i2 asc)
 * |        |   | aggrNode      |-->(sum(i2))-(sum(i3))
 * |        |   | aggrResultNode|-->(sum(i2))-(sum(i3))
 * |        |   | next          |
 * |        |   +--------/------+
 * |        |   +-------V-------+
 * |        |   | wndNode       |
 * |        |   +---------------+
 * |        |   | overColumnNode|-->(i1,i2 desc)
 * |        |   | aggrNode      |-->(sum(i2))
 * |        |   | aggrResultNode|-->(sum(i2))
 * |        |   | next          |
 * |        |   +---------------+
 * |        |
 * |        |
 * |sortNode|-->(i1,i2)-(i3)-(i1,i2 asc)-(i1,i2 desc)
 * |        |
 * |distNode|-->(distinct i1)
 * +--------+
 *
 ***********************************************************************/

    qmncWNST          * sWNST;
    qmsAnalyticFunc   * sAnalyticFunc;
    UInt                sDataNodeOffset;
    UInt                sFlag = 0;
    qtcOverColumn     * sOverColumn;
    qtcNode           * sNode;
    UInt                i;
    qmcAttrDesc       * sItrAttr;
    mtcNode           * sArg;
    qtcNode           * sAnalyticArg;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initWNST::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncWNST의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmncWNST) ,
                                            (void **)&sWNST )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sWNST,
                        QC_SHARED_TMPLATE(aStatement),
                        QMN_WNST,
                        qmnWNST,
                        qmndWNST,
                        sDataNodeOffset );

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    // Over 절의 attribute들을 key attribute로 추가
    for ( i = 0; i < aSortKeyCount; i++ )
    {
        // sorting key array에서 i번째 sorting key는
        // analytic function list pointer array에서 i번째에 있는
        // anlaytic function list들의 sorting key 이므로
        // 이중에서 동일 key count를 가지는 partition by column을 찾으면
        // 된다.
        for ( sAnalyticFunc = aAnalyticFuncListPtrArr[i];
              sAnalyticFunc != NULL;
              sAnalyticFunc = sAnalyticFunc->next )
        {
            for ( sOverColumn = sAnalyticFunc->analyticFuncNode->overClause->overColumn;
                  sOverColumn != NULL;
                  sOverColumn = sOverColumn->next )
            {
                if ( ( sOverColumn->flag & QTC_OVER_COLUMN_MASK )
                     == QTC_OVER_COLUMN_ORDER_BY )
                {
                    sFlag &= ~QMC_ATTR_ANALYTIC_SORT_MASK;
                    sFlag |= QMC_ATTR_ANALYTIC_SORT_TRUE;
                }
                else
                {
                    sFlag &= ~QMC_ATTR_ANALYTIC_SORT_MASK;
                    sFlag |= QMC_ATTR_ANALYTIC_SORT_FALSE;
                }

                if ( ( sOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK )
                     == QTC_OVER_COLUMN_ORDER_ASC )
                {
                    sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
                    sFlag |= QMC_ATTR_SORT_ORDER_ASC;
                }
                else
                {
                    sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
                    sFlag |= QMC_ATTR_SORT_ORDER_DESC;
                }

                /* PROJ-243 Order by Nulls first/last */
                if ( ( sOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                     == QTC_OVER_COLUMN_NULLS_ORDER_NONE )
                {
                    sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
                    sFlag |= QMC_ATTR_SORT_NULLS_ORDER_NONE;
                }
                else
                {
                    if ( ( sOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                         == QTC_OVER_COLUMN_NULLS_ORDER_FIRST )
                    {
                        sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
                        sFlag |= QMC_ATTR_SORT_NULLS_ORDER_FIRST;
                    }
                    else
                    {
                        sFlag &= ~QMC_ATTR_SORT_NULLS_ORDER_MASK;
                        sFlag |= QMC_ATTR_SORT_NULLS_ORDER_LAST;
                    }
                }

                // BUG-34966 OVER절의 column들에 pass node가 설정될 수 있다.
                IDE_TEST( qmc::duplicateGroupExpression( aStatement,
                                                         (qtcNode*)sOverColumn->node )
                          != IDE_SUCCESS );

                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                & sWNST->plan.resultDesc,
                                                sOverColumn->node,
                                                sFlag,
                                                QMC_APPEND_EXPRESSION_TRUE |
                                                QMC_APPEND_CHECK_ANALYTIC_TRUE |
                                                QMC_APPEND_ALLOW_CONST_TRUE,
                                                ID_FALSE )
                          != IDE_SUCCESS );

                /* BUG-37672 the result of windowing query is not correct. */
                if ( ( ( sOverColumn->node->lflag & QTC_NODE_AGGREGATE_MASK ) ==
                       QTC_NODE_AGGREGATE_ABSENT ) &&
                     ( sAnalyticFunc->analyticFuncNode->node.arguments != NULL ) )
                {
                    sAnalyticArg = (qtcNode *)sAnalyticFunc->analyticFuncNode->node.arguments;
                    if ( ( sAnalyticArg->node.arguments != NULL ) ||
                         ( sAnalyticArg->node.next != NULL ) )
                    {
                        /* 서로 같은 테이블인지를 체크 */
                        if ( qtc::dependencyContains( &sAnalyticArg->depInfo,
                                                      &sOverColumn->node->depInfo )
                             == ID_TRUE )
                        {
                            for ( sArg = sOverColumn->node->node.arguments;
                                  sArg != NULL;
                                  sArg = sArg->next )
                            {
                                IDE_TEST( qmc::appendAttribute( aStatement,
                                                                aQuerySet,
                                                                & sWNST->plan.resultDesc,
                                                                (qtcNode *)sArg,
                                                                0,
                                                                0,
                                                                ID_FALSE )
                                          != IDE_SUCCESS );
                            }
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }

    sFlag = 0;
    sFlag &= ~QMC_ATTR_ANALYTIC_FUNC_MASK;
    sFlag |= QMC_ATTR_ANALYTIC_FUNC_TRUE;

    // Analytic function을 attribute로 추가
    for( sAnalyticFunc = aQuerySet->analyticFuncList;
         sAnalyticFunc != NULL;
         sAnalyticFunc = sAnalyticFunc->next )
    {
        sNode = sAnalyticFunc->analyticFuncNode;

        // Argument도 attribute로 추가한다.
        if( sNode->node.arguments != NULL )
        {
            // PROJ-2527 WITHIN GROUP AGGR
            // WITHIN GROUP의 node도 resultDesc등록 해야 한다.
            for ( sArg = sNode->node.arguments;
                  sArg != NULL;
                  sArg = sArg->next )
            {
                // BUG-34966 Argument가 GROUP BY절의 column이면서 ORDER BY절에서도 사용된 경우
                //           argument를 별도로 복사하여 analytic function에서 참조한다.
                IDE_TEST( qmc::duplicateGroupExpression( aStatement,
                                                         (qtcNode*)sArg )
                          != IDE_SUCCESS );

                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                & sWNST->plan.resultDesc,
                                                (qtcNode *)sArg,
                                                0,
                                                0,
                                                ID_FALSE )
                          != IDE_SUCCESS );
            }

            // Aggregate function의 argument로 pass node가 설정된 경우 fast execution을
            // 수행해서는 안되므로 다시 estimation한다.
            // BUGBUG: SELECT clause보다 GROUP BY clause를 먼저 validation해야 한다.
            IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                     (qtcNode*)sNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
            // COUNT(*) 의 경우 argument가 NULL이다.
        }

        // BUG-44046 recursive with이면 conversion node를 유지 한다.
        // parent에 conversion node가 오는 경우가 있다.
        if ( ( ( aQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
               == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) ||
             ( ( aQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
               == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) )
        {
            sFlag &= ~QMC_ATTR_CONVERSION_MASK;
            sFlag |= QMC_ATTR_CONVERSION_TRUE;

            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sWNST->plan.resultDesc,
                                            sNode,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sWNST->plan.resultDesc,
                                            sNode,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
    }

    if( aParent != NULL )
    {
        // BUG-37277
        if ( ( aFlag & QMO_MAKEWNST_ORDERBY_GROUPBY_MASK ) ==
             QMO_MAKEWNST_ORDERBY_GROUPBY_TRUE )
        {
            for ( sItrAttr = aParent->resultDesc;
                  sItrAttr != NULL;
                  sItrAttr = sItrAttr->next )
            {
                if ( ( sItrAttr->expr->lflag & QTC_NODE_ANAL_FUNC_MASK ) ==
                     QTC_NODE_ANAL_FUNC_ABSENT )
                {
                    sItrAttr->flag &= ~QMC_ATTR_SEALED_MASK;
                    sItrAttr->flag |= QMC_ATTR_SEALED_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( qmc::pushResultDesc( aStatement,
                                       aQuerySet,
                                       ( aParent->type == QMN_PROJ ? ID_TRUE : ID_FALSE ),
                                       aParent->resultDesc,
                                       & sWNST->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sWNST->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sWNST;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeWNST( qcStatement          * aStatement,
                         qmsQuerySet          * aQuerySet,
                         UInt                   aFlag,
                         qmoDistAggArg        * aDistAggArg,
                         UInt                   aSortKeyCount,
                         qmgPreservedOrder   ** aSortKeyPtrArr,
                         qmsAnalyticFunc     ** aAnalFuncListPtrArr,
                         qmnPlan              * aChildPlan,
                         SDouble                aStoreRowCount,
                         qmnPlan              * aPlan )
{
    qmncWNST          * sWNST = (qmncWNST *)aPlan;
    UInt                sDataNodeOffset;
    qmcMtrNode        * sMtrNode[2];
    mtcTemplate       * sMtcTemplate;

    qmcWndNode       ** sWndNodePtrArr;
    UInt                sAnalFuncListPtrCount;
    qmcMtrNode       ** sSortMtrNodePtrArr;

    UShort              sTupleID;
    UShort              sColumnCount;
    UShort              sAggrTupleID;
    UShort              sAggrColumnCount;

    qmcWndNode        * sCurWndNode;
    qmcMtrNode        * sWndAggrNode;
    qmcMtrNode        * sCurAggrNode;

    qmcMtrNode        * sNewAggrNode;

    UInt                sWndNodeCount;
    UShort              sDistNodeCount;
    UInt                sAggrNodeCount;
    UInt                sSortNodeCount;

    UInt                sWndOverColumnNodeCount;
    UInt                sWndAggrNodeCount;
    UInt                sWndAggrResultNodeCount;

    qmcMtrNode        * sFirstAnalResultFuncMtrNode;
    qmcMtrNode        * sCurAnalResultFuncMtrNode;

    qmsAnalyticFunc   * sCurAnalyticFunc;
    UInt                i;

    UInt                sCurOffset;

    qtcOverColumn     * sOverColumn;
    mtcNode           * sNextNode;
    mtcNode           * sConversion;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeWNST::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    // 기본 초기화
    sMtcTemplate      = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    sColumnCount      = 0;
    sAggrColumnCount  = 0;

    sWndNodeCount     = 0;
    sDistNodeCount    = 0;
    sAggrNodeCount    = 0;
    sSortNodeCount    = 0;

    sWndOverColumnNodeCount = 0;
    sWndAggrNodeCount = 0;
    sWndAggrResultNodeCount = 0;
    sFirstAnalResultFuncMtrNode = NULL;
    sCurAnalResultFuncMtrNode = NULL;

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndWNST));

    sWNST->plan.left = aChildPlan;

    // Flag 정보 설정
    sWNST->flag      = QMN_PLAN_FLAG_CLEAR;
    sWNST->plan.flag = QMN_PLAN_FLAG_CLEAR;

    if ( aSortKeyCount == 0 )
    {
        // sort key가 없는 경우, sorting 할 필요없음
        sWNST->flag &= ~QMNC_WNST_STORE_MASK;
        sWNST->flag |= QMNC_WNST_STORE_ONLY;
    }
    else
    {
        if ( ( aFlag & QMO_MAKEWNST_PRESERVED_ORDER_MASK ) ==
             QMO_MAKEWNST_PRESERVED_ORDER_TRUE )
        {
            /* BUG-40354 pushed rank */
            if ( ( aFlag & QMO_MAKEWNST_PUSHED_RANK_MASK ) ==
                 QMO_MAKEWNST_PUSHED_RANK_TRUE )
            {
                sWNST->flag &= ~QMNC_WNST_STORE_MASK;
                sWNST->flag |= QMNC_WNST_STORE_LIMIT_PRESERVED_ORDER;
            }
            else
            {
                sWNST->flag &= ~QMNC_WNST_STORE_MASK;
                sWNST->flag |= QMNC_WNST_STORE_PRESERVED_ORDER;
            }
        }
        else
        {
            /* BUG-40354 pushed rank */
            if ( ( aFlag & QMO_MAKEWNST_PUSHED_RANK_MASK ) ==
                 QMO_MAKEWNST_PUSHED_RANK_TRUE )
            {
                sWNST->flag &= ~QMNC_WNST_STORE_MASK;
                sWNST->flag |= QMNC_WNST_STORE_LIMIT_SORTING;
            }
            else
            {
                sWNST->flag &= ~QMNC_WNST_STORE_MASK;
                sWNST->flag |= QMNC_WNST_STORE_SORTING;
            }
        }
    }

    sWNST->myNode        = NULL;
    sWNST->distNode      = NULL;
    sWNST->aggrNode      = NULL;
    sWNST->storeRowCount = aStoreRowCount;

    sWNST->sortKeyCnt = aSortKeyCount;

    //--------------------------
    // wndNode 생성 및 초기화
    //--------------------------
    if ( aSortKeyCount == 0 )
    {
        // Sort Key가 없으면
        // Analytic Function Ptr Array의 0번째 위치에
        // 모든 analytic function들이 list로 연결되어 있음
        sAnalFuncListPtrCount = 1;
    }
    else
    {
        // Sort Key가 있으면
        // Sort Key 개수만큼 analytic function ptr array가 존재하고
        // Sort Key에 대응되는 위치에 동일 sort key를 가지는
        // analytic function list들의 pointer를 가지고 있음
        sAnalFuncListPtrCount = aSortKeyCount;
    }

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( (ID_SIZEOF(qmcWndNode*) *
                                              sAnalFuncListPtrCount),
                                             (void**) &sWndNodePtrArr )
              != IDE_SUCCESS);

    for ( i = 0; i < sAnalFuncListPtrCount; i ++ )
    {
        sWndNodePtrArr[i] = NULL;
    }
    sWNST->wndNode = sWndNodePtrArr;

    //--------------------------
    // sortNode 생성 및 초기화
    //--------------------------
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ( ID_SIZEOF(qmcMtrNode*) *
                                               sAnalFuncListPtrCount ),
                                             (void**) &sSortMtrNodePtrArr )
              != IDE_SUCCESS );

    for ( i = 0; i < sAnalFuncListPtrCount; i ++ )
    {
        sSortMtrNodePtrArr[i] = NULL;
    }
    sWNST->sortNode = sSortMtrNodePtrArr;

    // over절이 비어있어도 sort key count는 1이 된다.
    sWNST->sortKeyCnt = sAnalFuncListPtrCount;

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sWNST->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     &( aQuerySet->depInfo ) )
              != IDE_SUCCESS );

    //----------------------------------
    // 튜플의 할당
    //----------------------------------

    // Analytic Function Result를 도출하기 위하여
    // 필요한 중간 결과를 저장하기 위한 tuple 할당 받음
    IDE_TEST( qtc::nextTable( & sAggrTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    // temp table을 위한 tuple을 할당받음
    IDE_TEST( qtc::nextTable(  &sTupleID ,
                               aStatement ,
                               NULL,
                               ID_TRUE,
                               MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    //----------------------------------
    // Tuple의 저장 매체 정보 설정
    //----------------------------------
    if( (aFlag & QMO_MAKEWNST_TEMP_TABLE_MASK) ==
        QMO_MAKEWNST_MEMORY_TEMP_TABLE )
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    sMtcTemplate->rows[sAggrTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sAggrTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

    if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
    {
        if( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
            == QMN_PLAN_STORAGE_DISK )
        {
            sMtcTemplate->rows[sAggrTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sAggrTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    //GRAPH에서 지정한 저장매체를 사용한다.
    if( (aFlag & QMO_MAKEWNST_TEMP_TABLE_MASK) ==
        QMO_MAKEWNST_MEMORY_TEMP_TABLE )
    {
        sWNST->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sWNST->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
    }
    else
    {
        sWNST->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sWNST->plan.flag  |= QMN_PLAN_STORAGE_DISK;
    }

    //----------------------------------
    // myNode 구성
    //     아래와 같이 구성된다.
    //     [Base Table] + [Columns] + [Analytic Function]
    //----------------------------------

    IDE_TEST( makeMyNodeOfWNST( aStatement,
                                aQuerySet,
                                aPlan,
                                aAnalFuncListPtrArr,
                                sAnalFuncListPtrCount,
                                sTupleID,
                                sWNST,
                                & sColumnCount,
                                & sFirstAnalResultFuncMtrNode )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sFirstAnalResultFuncMtrNode == NULL,
                    ERR_MTR_NODE_NOT_FOUND );

    //----------------------------------
    // Temp Table에 대한 Tuple column의 할당
    //----------------------------------

    // temp table의 tuple column 할당
    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sTupleID ,
                                           sColumnCount )
        != IDE_SUCCESS);

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    //GRAPH에서 지정한 저장매체를 사용한다.
    if( (aFlag & QMO_MAKEWNST_TEMP_TABLE_MASK) ==
        QMO_MAKEWNST_MEMORY_TEMP_TABLE )
    {
        sWNST->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sWNST->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sWNST->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sWNST->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if ( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    for ( i = 0; i < sAnalFuncListPtrCount; i++ )
    {
        for ( sCurAnalyticFunc  = aAnalFuncListPtrArr[i];
              sCurAnalyticFunc != NULL ;
              sCurAnalyticFunc  = sCurAnalyticFunc->next)
         {
             // analytic function의 argument
             IDE_TEST( qmg::changeColumnLocate( aStatement,
                                                aQuerySet,
                                                (qtcNode*)sCurAnalyticFunc->analyticFuncNode->node.arguments,
                                                sTupleID,
                                                ID_TRUE ) // aNext
                       != IDE_SUCCESS );

             // analytic function의 partition by column들
             for ( sOverColumn  =
                       sCurAnalyticFunc->analyticFuncNode->overClause->overColumn;
                   sOverColumn != NULL;
                   sOverColumn  = sOverColumn->next )
             {
                 IDE_TEST( qmg::changeColumnLocate( aStatement,
                                                    aQuerySet,
                                                    sOverColumn->node,
                                                    sTupleID,
                                                    ID_FALSE ) // aNext
                           != IDE_SUCCESS );
             }
         }
    }

    // mtcColumn , mtcExecute 정보의 구축
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sWNST->myNode )
              != IDE_SUCCESS);

    // column locate 지정
    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sWNST->myNode )
              != IDE_SUCCESS );

    //----------------------------------
    // wndNode, distNode, aggrNode, sortNode 생성
    //----------------------------------
    sCurAnalResultFuncMtrNode = sFirstAnalResultFuncMtrNode;

    for ( i = 0; i < sAnalFuncListPtrCount; i++ )
    {
        for ( sCurAnalyticFunc  = aAnalFuncListPtrArr[i];
              sCurAnalyticFunc != NULL ;
              sCurAnalyticFunc  = sCurAnalyticFunc->next)
        {
            //----------------------------------
            // wndNode 생성
            //----------------------------------

            // 동일 Partition By를 가지는 wndNode 존재하는지 검사
            sCurWndNode = NULL;
            IDE_TEST( existSameWndNode( aStatement,
                                        sTupleID,
                                        sWndNodePtrArr[i],
                                        sCurAnalyticFunc->analyticFuncNode,
                                        & sCurWndNode )
                      != IDE_SUCCESS );

            if ( sCurWndNode == NULL )
            {
                // 존재하지 않을 경우, wndNode 생성
                IDE_TEST( makeWndNode( aStatement,
                                       sTupleID,
                                       sWNST->myNode,
                                       sCurAnalyticFunc->analyticFuncNode,
                                       & sWndOverColumnNodeCount,
                                       & sCurWndNode )
                          != IDE_SUCCESS );
                sWndNodeCount++;

                // 새로운 wndNode를 sWNST의 wndNode에 앞 부분에 연결
                sCurWndNode->next = sWndNodePtrArr[i];
                sWndNodePtrArr[i] = sCurWndNode;
            }
            else
            {
                // nothing to do
            }

            // wndNode에 aggrNode 추가
            IDE_TEST( addAggrNodeToWndNode( aStatement,
                                            aQuerySet,
                                            sCurAnalyticFunc,
                                            sAggrTupleID,
                                            & sAggrColumnCount,
                                            sCurWndNode,
                                            & sWndAggrNode )
                      != IDE_SUCCESS );
            sWndAggrNodeCount++;

            // aggrNode에 대하여 distinct 검사
            // distinct node 추가
            if ( ( sCurAnalyticFunc->analyticFuncNode->node.lflag &
                   MTC_NODE_DISTINCT_MASK )
                 == MTC_NODE_DISTINCT_FALSE )
            {
                // Distinction이 존재하지 않을 경우
                sWndAggrNode->flag &= ~QMC_MTR_DIST_AGGR_MASK;
                sWndAggrNode->flag |= QMC_MTR_DIST_AGGR_FALSE;
                sWndAggrNode->myDist = NULL;
            }
            else
            {
                IDE_TEST_RAISE( (sCurWndNode->execMethod != QMC_WND_EXEC_PARTITION_UPDATE) &&
                                (sCurWndNode->execMethod != QMC_WND_EXEC_AGGR_UPDATE),
                                ERR_INVALID_ORDERBY );

                // Distinction이 존재하는 경우
                sWndAggrNode->flag &= ~QMC_MTR_DIST_AGGR_MASK;
                sWndAggrNode->flag |= QMC_MTR_DIST_AGGR_TRUE;

                //----------------------------------
                // distNode 생성
                //----------------------------------

                IDE_TEST( qmg::makeDistNode(aStatement,
                                            aQuerySet,
                                            sWNST->plan.flag,
                                            aChildPlan,
                                            sAggrTupleID,
                                            aDistAggArg,
                                            sCurAnalyticFunc->analyticFuncNode,
                                            sWndAggrNode,
                                            & sWNST->distNode,
                                            & sDistNodeCount )
                          != IDE_SUCCESS );
            }

            // wndNode에 aggrResultNode 추가
            IDE_TEST( addAggrResultNodeToWndNode( aStatement,
                                                  sCurAnalResultFuncMtrNode,
                                                  sCurWndNode )
                      != IDE_SUCCESS );
            sWndAggrResultNodeCount++;

            sCurAnalResultFuncMtrNode = sCurAnalResultFuncMtrNode->next;

            //----------------------------------
            // aggrNode 생성
            //----------------------------------

            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmcMtrNode ),
                                                     (void**)& sNewAggrNode)
                      != IDE_SUCCESS);

            *sNewAggrNode = *sWndAggrNode;

            sNewAggrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
            sNewAggrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_FALSE;

            sNewAggrNode->next = NULL;

            sAggrNodeCount++;

            if ( sWNST->aggrNode == NULL )
            {
                sWNST->aggrNode = sNewAggrNode;
            }
            else
            {
                for ( sCurAggrNode = sWNST->aggrNode;
                      sCurAggrNode->next != NULL;
                      sCurAggrNode = sCurAggrNode->next ) ;

                sCurAggrNode->next = sNewAggrNode;
            }
        }

        //----------------------------------
        // sortNode의 생성
        //----------------------------------

        IDE_TEST( qmg::makeSortMtrNode( aStatement,
                                        sTupleID,
                                        aSortKeyPtrArr[i],
                                        aAnalFuncListPtrArr[i],
                                        sWNST->myNode,
                                        & sSortMtrNodePtrArr[i],
                                        & sSortNodeCount )
                  != IDE_SUCCESS );
    }

    //----------------------------------
    // Aggregation을 위해 할당받은 Tuple column의 할당
    //----------------------------------

    // Analytic Function Result를 도출하기 위하여
    // 필요한 중간 결과를 저장하기 위한 aggregation의 tuple column 할당
    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sAggrTupleID ,
                                           sAggrColumnCount )
              != IDE_SUCCESS);

    // mtcColumn , mtcExecute 정보의 구축
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sWNST->aggrNode )
              != IDE_SUCCESS);

    // column locate 지정
    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sWNST->aggrNode )
              != IDE_SUCCESS );

    //----------------------------------
    // PROJ-2179 calculate가 필요한 node들의 결과 위치를 설정
    //----------------------------------

    IDE_TEST( qmg::setCalcLocate( aStatement,
                                  sWNST->myNode )
              != IDE_SUCCESS );

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    //----------------------------------
    // Data Offset 설정
    //----------------------------------

    // mtrNodeOffset 설정
    sWNST->mtrNodeOffset = sDataNodeOffset;

    // 다음 노드가 저장될 시작 지점
    sCurOffset = sDataNodeOffset +
        idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sColumnCount;

    // distNodeOffset
    sWNST->distNodeOffset = sCurOffset;
    sCurOffset += idlOS::align8(ID_SIZEOF(qmdDistNode)) * sDistNodeCount;

    // aggrNodeOffset
    sWNST->aggrNodeOffset = sCurOffset;
    sCurOffset += idlOS::align8(ID_SIZEOF(qmdAggrNode)) * sAggrNodeCount;

    // sortNodeOffset
    // Pointer를 위한 공간 + sort node를 위한 공간
    sWNST->sortNodeOffset = sCurOffset;
    sCurOffset +=
        ( idlOS::align8(ID_SIZEOF(qmdMtrNode*)) * sAnalFuncListPtrCount ) +
        ( idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sSortNodeCount );

    // wndNodeOffset
    // wndNodeOffset + partition By Node offset + aggr node offset +
    // aggr result node offset
    sWNST->wndNodeOffset = sCurOffset;
    sCurOffset +=
        ( idlOS::align8(ID_SIZEOF(qmdWndNode*)) * sAnalFuncListPtrCount ) +
        ( idlOS::align8(ID_SIZEOF(qmdWndNode)) * sWndNodeCount ) +
        ( idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sWndOverColumnNodeCount ) +
        ( idlOS::align8(ID_SIZEOF(qmdAggrNode)) * sWndAggrNodeCount ) +
        ( idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sWndAggrResultNodeCount );

    // Sort Manager Offset
    if ( (aFlag & QMO_MAKEWNST_TEMP_TABLE_MASK) ==
         QMO_MAKEWNST_MEMORY_TEMP_TABLE )
    {
        sWNST->sortMgrOffset = sCurOffset;
        sCurOffset += idlOS::align8(ID_SIZEOF(qmcdSortTemp));
    }
    else
    {
        sWNST->sortMgrOffset = sCurOffset;
        sCurOffset +=
            idlOS::align8(ID_SIZEOF(qmcdSortTemp)) * sAnalFuncListPtrCount;
    }

    // data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sCurOffset;

    //----------------------------------
    // dependency 처리 및 subquery 처리
    //----------------------------------
    sMtrNode[0] = sWNST->myNode;

    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sWNST->plan,
                                            QMO_WNST_DEPENDENCY,
                                            sTupleID,
                                            NULL, // Target
                                            0,
                                            NULL, // Predicate
                                            1,
                                            sMtrNode )
              != IDE_SUCCESS );

    // BUG-27526
    IDE_TEST_RAISE( sWNST->plan.dependency == ID_UINT_MAX,
                       ERR_INVALID_DEPENDENCY );

    sWNST->depTupleRowID = (UShort)sWNST->plan.dependency;

    //----------------------------------
    // aggrNode의 dst를 aggrResultNode의 src로 변경
    //----------------------------------
    sCurAnalResultFuncMtrNode = sFirstAnalResultFuncMtrNode;
    for ( sCurAggrNode = sWNST->aggrNode;
          sCurAggrNode != NULL;
          sCurAggrNode = sCurAggrNode->next )
    {
        idlOS::memcpy( sCurAnalResultFuncMtrNode->srcNode,
                       sCurAggrNode->dstNode,
                       ID_SIZEOF(qtcNode) );

        sCurAnalResultFuncMtrNode = sCurAnalResultFuncMtrNode->next;
    }

    //----------------------------------
    // aggrResultNode의 dst를 analytic function node에 저장
    // ( temp table에 저장된 값을 target 또는 order by에서 읽을수 있게
    //   하기 위함 )
    //----------------------------------

    sCurAnalResultFuncMtrNode = sFirstAnalResultFuncMtrNode;

    for ( i = 0; i < sAnalFuncListPtrCount; i++ )
    {
        for( sCurAnalyticFunc  = aAnalFuncListPtrArr[i];
             sCurAnalyticFunc != NULL ;
             sCurAnalyticFunc  = sCurAnalyticFunc->next)
         {
             sNextNode = sCurAnalyticFunc->analyticFuncNode->node.next;
             sConversion = sCurAnalyticFunc->analyticFuncNode->node.conversion;

             // aggrResultNode의 dst 변경
             idlOS::memcpy( sCurAnalyticFunc->analyticFuncNode,
                            sCurAnalResultFuncMtrNode->dstNode,
                            ID_SIZEOF(qtcNode) );

             // BUG-21912 : aggregation 결과에 대한 conversion을
             //             다시 연결해주어야 함
             sCurAnalyticFunc->analyticFuncNode->node.conversion =
                 sConversion;

             sCurAnalyticFunc->analyticFuncNode->node.next = sNextNode;

             sCurAnalResultFuncMtrNode = sCurAnalResultFuncMtrNode->next;
         }
    }

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sWNST->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sWNST->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sWNST->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sWNST->planID,
                               sWNST->plan.flag,
                               sMtcTemplate->rows[sTupleID].lflag,
                               sWNST->myNode,
                               & sWNST->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MTR_NODE_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeWNST",
                                  "Materialized node error" ));
    }
    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeWNST",
                                  "Invalid dependency" ));
    }
    IDE_EXCEPTION( ERR_INVALID_ORDERBY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_POSITION_IN_ORDERBY ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOneMtrPlan::makeFilterINSubquery( qcStatement  * aStatement ,
                                     qmsQuerySet  * aQuerySet ,
                                     UShort         aTupleID,
                                     qtcNode      * aInFilter ,
                                     qtcNode     ** aChangedFilter )
{
/***********************************************************************
 *
 * Description : IN subquery에서 필요한 filter를 재구성해준다.
 *
 * Implementation :
 *     - IN은  "="로
 *     - LIST는 각각 element에 대해서 "=" 연산을 취해주도록한다.
 *       모든 연산은 다시 AND로 묶여야 한다.
 *     - passNode는 target으로 부터 알아낸다. HASH노드에서 미리 세팅
 *
 * ex)
 *      IN                                     =
 *      |                                      |
 *      i1  -  sub                            i1  -  passNode
 *
 *                        ------->
 *      IN                                    AND
 *      |                                      |
 *     LIST -  sub                             =    -    =
 *      |                                      |         |
 *    (i1,i2)                                 i1 - p    i2 - p
 *
 ***********************************************************************/

    qcNamePosition      sNullPosition;

    qtcNode           * sStartNode;
    qtcNode           * sNewNode[2];
    qtcNode           * sLastNode = NULL;
    qtcNode           * sFirstNode = NULL;
    qtcNode           * sArgNode;

    qtcNode           * sArg1;
    qtcNode           * sArg2;

    qmsTarget         * sTarget;

    extern mtfModule    mtfList;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeFilterINSubquery::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );

    IDE_TEST_RAISE( aInFilter == NULL, ERR_EMPTY_FILTER );

    // To Fix PR-8109
    // 모든 Key Range는 DNF로 구성되어 있어야 함.
    IDE_TEST_RAISE( ( aInFilter->node.lflag & MTC_NODE_OPERATOR_MASK )
                    != MTC_NODE_OPERATOR_OR,
                    ERR_INVALID_FILTER );

    IDE_TEST_RAISE( aInFilter->node.arguments == NULL,
                    ERR_INVALID_FILTER );
    IDE_TEST_RAISE( ( aInFilter->node.arguments->lflag & MTC_NODE_OPERATOR_MASK )
                    != MTC_NODE_OPERATOR_AND,
                    ERR_INVALID_FILTER );

    // PROJ-1473
    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       aInFilter,
                                       aTupleID,
                                       ID_TRUE )
              != IDE_SUCCESS );

    //----------------------------------
    // Filter의 처리
    // - IN , NOT IN의 하위가 List라면
    //     - 리스트의 개수만큼  , (= , !=)을 생성하고
    //       passNode로 연결한후 , AND로 묶어준다.
    //
    // - LIST가 아니라면
    //     - (= , !=)을 생성하고, passNode로 연결
    //
    // - passNode는 HASH노드에서 생성후 Target에 미리 세팅
    //----------------------------------

    // To Fix PR-8109
    // IN Node를 찾는다.
    IDE_FT_ASSERT( aInFilter->node.arguments != NULL );

    sStartNode   = (qtcNode *) aInFilter->node.arguments->arguments;

    //연산자 노드의 argumnent가 LIST인지 아닌지를 판별
    sArgNode = (qtcNode*)sStartNode->node.arguments;
    if ( sArgNode->node.module == &mtfList )
    {
        //List인 경우
        sArgNode = (qtcNode*)sArgNode->node.arguments;
    }
    else
    {
        //List가 아닌경우
        //nothing to do
    }

    //Target에 변경해주어야 할 passNode가 있기 때문이다.
    for ( sTarget   = aQuerySet->target ;
          sArgNode != NULL && sTarget != NULL ;
          sArgNode  = (qtcNode*)sArgNode->node.next , sTarget = sTarget->next )
    {
        // "=" operator를 생성해준다.
        SET_EMPTY_POSITION( sNullPosition );
        IDE_TEST( qtc::makeNode( aStatement ,
                                 sNewNode ,
                                 &sNullPosition ,
                                 (const UChar*)"=" ,
                                 1 )
                  != IDE_SUCCESS );

        sNewNode[0]->indexArgument = 1;

        // Argument 복사
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                 (void **) & sArg1 )
                  != IDE_SUCCESS );
        idlOS::memcpy( sArg1, sArgNode, ID_SIZEOF(qtcNode) );

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                 (void **) & sArg2 )
                  != IDE_SUCCESS );
        idlOS::memcpy( sArg2, sTarget->targetColumn, ID_SIZEOF(qtcNode) );

        // 연결 관계 구성
        sNewNode[0]->node.arguments = (mtcNode*)sArg1;
        sArg1->node.next = (mtcNode*) sArg2;
        sArg2->node.next = NULL;

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sNewNode[0] )
                  != IDE_SUCCESS );

        //connect
        if( sFirstNode == NULL )
        {
            sFirstNode = sNewNode[0];
            sLastNode = sNewNode[0];
        }
        else
        {
            sLastNode->node.next = (mtcNode*)sNewNode[0];
            sLastNode = sNewNode[0];
        }
    }

    //1개 이상의 연산자 노드가 생성될수도 있으므로 AND로 연결해준다.
    SET_EMPTY_POSITION( sNullPosition );
    IDE_TEST( qtc::makeNode( aStatement,
                             sNewNode,
                             & sNullPosition,
                             (const UChar*)"AND",
                             3 )
              != IDE_SUCCESS );

    sNewNode[0]->node.arguments = (mtcNode*)sFirstNode;

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sNewNode[0] )
              != IDE_SUCCESS );

    // To Fix PR-8109
    // DNF로 구성하기 위하여 OR 노드를 생성하여 연결한다.
    sFirstNode = sNewNode[0];
    IDE_TEST( qtc::makeNode( aStatement,
                             sNewNode,
                             & sNullPosition,
                             (const UChar*)"OR",
                             2 )
              != IDE_SUCCESS );

    sNewNode[0]->node.arguments = (mtcNode*)sFirstNode;

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sNewNode[0] )
              != IDE_SUCCESS );

    *aChangedFilter = sNewNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EMPTY_FILTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeFilterINSubquery",
                                  "Filter is empty" ));
    }
    IDE_EXCEPTION( ERR_INVALID_FILTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeFilterINSubquery",
                                  "Filter is invalid" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeFilterConstFromPred( qcStatement  * aStatement ,
                                        qmsQuerySet  * aQuerySet,
                                        UShort         aTupleID,
                                        qtcNode      * aFilter ,
                                        qtcNode     ** aFilterConst )
{
/***********************************************************************
 *
 * Description : 주어진 filter로 부터 filterConst를 구성한다.
 *
 * Implementation :
 *
 *   To Fix PR-8109
 *
 *   Join Key Range는 아래와 같이 항상 DNF로 구성된다.
 *
 *    OR
 *    |
 *    AND
 *    |
 *    =      ->       =
 *
 ***********************************************************************/

    qtcNode           * sOperatorNode;

    qtcNode           * sNewNode;
    qtcNode           * sLastNode = NULL;
    qtcNode           * sFirstNode = NULL;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeFilterConstFromPred::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_TEST_RAISE( aFilter == NULL, ERR_EMPTY_FILTER );

    // To Fix PR-8109
    // 모든 Key Range는 DNF로 구성되어 있어야 함.

    IDE_TEST_RAISE( ( aFilter->node.lflag & MTC_NODE_OPERATOR_MASK )
                    != MTC_NODE_OPERATOR_OR,
                    ERR_INVALID_FILTER );

    IDE_TEST_RAISE( aFilter->node.arguments == NULL,
                    ERR_INVALID_FILTER );
    IDE_TEST_RAISE( ( aFilter->node.arguments->lflag & MTC_NODE_OPERATOR_MASK )
                    != MTC_NODE_OPERATOR_AND,
                    ERR_INVALID_FILTER );

    // To Fix PR-8242
    // DNF로 변형된 Join Key Range는
    // AND 노드가 하나만이 존재할 수 있다.
    IDE_TEST_RAISE( aFilter->node.arguments->next != NULL,
                    ERR_INVALID_FILTER );

    //----------------------------------
    // AND 하위로부터 각 연산자에 대하여 컬럼 구성
    //----------------------------------

    for ( sOperatorNode  = (qtcNode *) aFilter->node.arguments->arguments;
          sOperatorNode != NULL ;
          sOperatorNode  = (qtcNode*) sOperatorNode->node.next )
    {
        IDE_TEST( makeFilterConstFromNode( aStatement ,
                                           aQuerySet,
                                           aTupleID,
                                           sOperatorNode ,
                                           & sNewNode )
                  != IDE_SUCCESS );

        sNewNode->node.next = NULL;

        //connect
        if( sFirstNode == NULL )
        {
            sFirstNode = sNewNode;
            sLastNode = sNewNode;
        }
        else
        {
            sLastNode->node.next = (mtcNode*)sNewNode;
            sLastNode = sNewNode;
        }
    }

    *aFilterConst = sFirstNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EMPTY_FILTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeFilterConstFromPred",
                                  "Filter is empty" ));
    }
    IDE_EXCEPTION( ERR_INVALID_FILTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeFilterConstFromPred",
                                  "Filter is invalid" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeFilterConstFromNode( qcStatement  * aStatement ,
                                        qmsQuerySet  * aQuerySet,
                                        UShort         aTupleID,
                                        qtcNode      * aOperatorNode ,
                                        qtcNode     ** aNewNode )
{
/***********************************************************************
 *
 * Description : 주어진 Operator 노드로 부터 컬럼을 찾아 filterConst가 될
 *               노드를 복사해서 준다
 *
 * Implementation :
 *
 *     - indexArgument에 해당하는 노드를 저장해야 할 경우
 *
 *       = (indexArgument = 1인경우)
 *       |
 *       i1    -    i2
 *      (복사)
 *
 ***********************************************************************/

    qtcNode           * sNode;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeFilterConstFromNode::__FT__" );

    //----------------------------------
    // indexArgument에 해당하지 않는 컬럼을 저장한다
    //----------------------------------
    if( aOperatorNode->indexArgument == 0 )
    {
        sNode = (qtcNode*)aOperatorNode->node.arguments->next;
    }
    else
    {
        sNode = (qtcNode*)aOperatorNode->node.arguments;
    }

    //----------------------------------
    // sNode에 해당 하는 컬럼을 복사해서 사용한다.
    //----------------------------------
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qtcNode,
                            aNewNode )
              != IDE_SUCCESS);

    // validation시 설정된 위치정보를 참조해야 할 새로운 위치정보로 변경한다.
    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sNode,
                                       aTupleID,
                                       ID_FALSE )
                 != IDE_SUCCESS );

    idlOS::memcpy( *aNewNode , sNode , ID_SIZEOF(qtcNode) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::existSameWndNode( qcStatement    * aStatement,
                                 UShort           aTupleID,
                                 qmcWndNode     * aWndNode,
                                 qtcNode        * aAnalticFuncNode,
                                 qmcWndNode    ** aSameWndNode )
{
/***********************************************************************
 *
 * Description : partition by가 동일한 wnd node가 존재하는지 검사
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcWndNode        * sCurWndNode;
    qmcMtrNode        * sCurOverColumnNode;
    qtcOverColumn     * sCurOverColumn;
    qtcWindow         * sWindow;
    UShort              sTable;
    UShort              sColumn;
    idBool              sExistSameWndNode;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::existSameWndNode::__FT__" );

    sWindow = aAnalticFuncNode->overClause->window;

     // 동일 Partition By Column을 가지는 Wnd Node를 찾음
    for ( sCurWndNode = aWndNode;
          sCurWndNode != NULL;
          sCurWndNode = sCurWndNode->next )
    {
        /* PROJ-1804 Lag, Lead관련 Function 은 새로 추가 */
        if ( ( aAnalticFuncNode->node.module == &mtfLag ) ||
             ( aAnalticFuncNode->node.module == &mtfLagIgnoreNulls ) ||
             ( aAnalticFuncNode->node.module == &mtfLead ) ||
             ( aAnalticFuncNode->node.module == &mtfLeadIgnoreNulls ) ||
             ( aAnalticFuncNode->node.module == &mtfNtile ) )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
        /* PROJ-1805 window clause는 일단 무조건 새로 추가 */
        if ( sWindow != NULL )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
        sExistSameWndNode = ID_TRUE;
        for ( sCurOverColumnNode = aWndNode->overColumnNode,
                  sCurOverColumn = aAnalticFuncNode->overClause->overColumn;
              ( sCurOverColumnNode != NULL ) &&
                  ( sCurOverColumn != NULL );
              sCurOverColumnNode = sCurOverColumnNode->next,
                  sCurOverColumn = sCurOverColumn->next )
        {
            // 칼럼의 현재 (table, column) 정보를 찾음
            IDE_TEST( qmg::findColumnLocate( aStatement,
                                             aTupleID,
                                             sCurOverColumn->node->node.table,
                                             sCurOverColumn->node->node.column,
                                             & sTable,
                                             & sColumn )
                      != IDE_SUCCESS );

            if ( ( sCurOverColumnNode->srcNode->node.table == sTable )
                 &&
                 ( sCurOverColumnNode->srcNode->node.column == sColumn ) )
            {
                // BUG-33663 Ranking Function
                // partition by column끼리 비교하고 order by 컬럼끼리 비교한다.
                if ( (sCurOverColumnNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                     == QMC_MTR_SORT_ORDER_FIXED_TRUE )
                {
                    if ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                         == QTC_OVER_COLUMN_ORDER_BY )
                    {
                        if ( ( (sCurOverColumnNode->flag & QMC_MTR_SORT_ORDER_MASK)
                               == QMC_MTR_SORT_ASCENDING )
                             &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                               == QTC_OVER_COLUMN_ORDER_ASC ) )
                        {
                            // 동일한 경우, 다음 칼럼도 동일한지 계속 검사
                        }
                        else
                        {
                            sExistSameWndNode = ID_FALSE;
                            break;
                        }

                        if ( ( (sCurOverColumnNode->flag & QMC_MTR_SORT_ORDER_MASK)
                               == QMC_MTR_SORT_DESCENDING )
                             &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                               == QTC_OVER_COLUMN_ORDER_DESC ) )
                        {
                            // 동일한 경우, 다음 칼럼도 동일한지 계속 검사
                        }
                        else
                        {
                            sExistSameWndNode = ID_FALSE;
                            break;
                        }
                    }
                    else
                    {
                        sExistSameWndNode = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    if ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                         == QTC_OVER_COLUMN_NORMAL )
                    {
                        // 동일한 경우, 다음 칼럼도 동일한지 계속 검사
                    }
                    else
                    {
                        sExistSameWndNode = ID_FALSE;
                        break;
                    }
                }
            }
            else
            {
                sExistSameWndNode = ID_FALSE;
                break;
            }
        }

        if ( sExistSameWndNode == ID_TRUE )
        {
            if ( ( sCurOverColumnNode == NULL ) && ( sCurOverColumn == NULL ) )
            {
                *aSameWndNode = sCurWndNode;
                break;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::addAggrNodeToWndNode( qcStatement       * aStatement,
                                     qmsQuerySet       * aQuerySet,
                                     qmsAnalyticFunc   * aAnalyticFunc,
                                     UShort              aAggrTupleID,
                                     UShort            * aAggrColumnCount,
                                     qmcWndNode        * aWndNode,
                                     qmcMtrNode       ** aNewAggrNode )
{
/***********************************************************************
 *
 * Description : wndNode에 aggrNode를 추가함
 *
 * Implementation :
 *
 *    ex ) SELECT SUM(i1) OVER ( PARTITION BY i2 ) FROM t1;
 *         위 질의일때 아래 그림에서 aggrNode를 추가하는 함수
 *
 *     myNode-->(baseTable/baseColumn)->(i1)->(i2)->(sum(i1))
 *
 *               +---------------+
 *     wndNode-->| WndNode       |
 *               +---------------+
 *               | overColumnNode|-->(i2)
 *               | aggrNode      |-->(sum(i1))
 *               | aggrResultNode|
 *               | next          |
 *               +---------------+
 *
 ***********************************************************************/

    qtcNode    * sSrcNode;
    qmcMtrNode * sNewMtrNode;
    qmcMtrNode * sLastAggrNode;
    mtcNode    * sArgs;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::addAggrNodeToWndNode::__FT__" );

    //----------------------------
    // aggrNode의 src 생성
    //----------------------------

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                               (void**)&( sSrcNode ) )
              != IDE_SUCCESS );

    // analytic function 정보 복사
    *(sSrcNode) = *(aAnalyticFunc->analyticFuncNode);

    //----------------------------
    // aggrNode 생성 및 dst 구성
    //----------------------------

    sNewMtrNode = NULL;

    // PROJ-2179
    // PROJ-2527 WITHIN GROUP AGGR
    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       (qtcNode*)sSrcNode->node.arguments,
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                      aQuerySet,
                                      sSrcNode,
                                      ID_FALSE,
                                      aAggrTupleID,
                                      0,
                                      aAggrColumnCount,
                                      & sNewMtrNode )
              != IDE_SUCCESS );

    // PROJ-2179
    // Aggregate function의 인자들의 위치가 더이상 변경되어서는 안된다.
    for( sArgs = sSrcNode->node.arguments;
         sArgs != NULL;
         sArgs = sArgs->next )
    {
        sArgs->lflag &= ~MTC_NODE_COLUMN_LOCATE_CHANGE_MASK;
        sArgs->lflag |= MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE;
    }

    // 무조건 생성되어야 함
    IDE_DASSERT( sNewMtrNode != NULL );

    //----------------------------
    // 연결
    //----------------------------

    if ( aWndNode->aggrNode == NULL )
    {
        aWndNode->aggrNode = sNewMtrNode;
    }
    else
    {
        for ( sLastAggrNode = aWndNode->aggrNode;
              sLastAggrNode->next != NULL;
              sLastAggrNode = sLastAggrNode->next ) ;

        sLastAggrNode->next = sNewMtrNode;
    }

    *aNewAggrNode   = sNewMtrNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::addAggrResultNodeToWndNode( qcStatement * aStatement,
                                           qmcMtrNode  * aAnalResultFuncMtrNode,
                                           qmcWndNode  * aWndNode )
{
/***********************************************************************
 *
 * Description : wndNode에 resultAggrNode를 추가함
 *
 * Implementation :
 *
 *    ex ) SELECT SUM(i1) OVER ( PARTITION BY i2 ) FROM t1;
 *         위 질의일때 아래 그림에서
 *         wndNode의 aggrResultNode를 추가하는 함수
 *         wndNode의 aggrResultNode는 wndNode의 aggrNode의 결과가
 *         최종적으로 저장되는 곳에 대한 정보이며
 *         myNode의 aggrResultNode와 동일하다.
 *
 *     myNode-->(baseTable/baseColumn)->(i1)->(i2)->(sum(i1))
 *                                                     |
 *               +---------------+                     |
 *     wndNode-->| wndNode       |                     |
 *               +---------------+                     |
 *               | overColumnNode|-->(i2)              |
 *               | aggrNode      |-->(sum(i1))         |
 *               | aggrResultNode|-->(sum(i1)) --------+ 서로 동일
 *               | next          |
 *               +---------------+
 *
 ***********************************************************************/

    qmcMtrNode * sNewMtrNode;
    qmcMtrNode * sLastAggrResultNode;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::addAggrResultNodeToWndNode::__FT__" );

    // materialize node 생성
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmcMtrNode ),
                                               (void**)& sNewMtrNode )
              != IDE_SUCCESS);

    // result function node 복사
    *sNewMtrNode = *aAnalResultFuncMtrNode;
    sNewMtrNode->next = NULL;

    if ( aWndNode->aggrResultNode == NULL )
    {
        aWndNode->aggrResultNode = sNewMtrNode;
    }
    else
    {
        for ( sLastAggrResultNode = aWndNode->aggrResultNode;
              sLastAggrResultNode->next != NULL;
              sLastAggrResultNode = sLastAggrResultNode->next ) ;
        sLastAggrResultNode->next = sNewMtrNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeWndNode( qcStatement       * aStatement,
                            UShort              aTupleID,
                            qmcMtrNode        * aMtrNode,
                            qtcNode           * aAnalticFuncNode,
                            UInt              * aOverColumnNodeCount,
                            qmcWndNode       ** aNewWndNode )
{
/***********************************************************************
 *
 * Description : wnd node에 aggr node를 추가함
 *
 * Implementation :
 *    ex ) SELECT SUM(i1) OVER ( PARTITION BY i2 ORDER BY i3 ) FROM t1;
 *         위 질의일때 아래 그림에서 wndNode를 생성하는 함수
 *
 *     myNode-->(baseTable/baseColumn)->(i1)->(i2)->(i3 asc)-(sum(i1))
 *
 *               +---------------+
 *     wndNode-->| wndNode       |
 *               +---------------+
 *               | overColumnNode|-->(i2)-(i3 asc)
 *               | aggrNode      |
 *               | aggrResultNode|
 *               | next          |
 *               +---------------+
 *
 *
 ***********************************************************************/

    qtcOverColumn     * sCurOverColumn;
    qmcMtrNode        * sCurMtrNode;
    qmcWndNode        * sNewWndNode;
    qmcMtrNode        * sSameMtrNode;
    qmcMtrNode        * sNewMtrNode;
    qmcMtrNode        * sFirstOverColumnNode;
    qmcMtrNode        * sLastOverColumnNode;
    qtcWindow         * sWindow;
    mtcNode           * sNode;
    UShort              sTable;
    UShort              sColumn;
    idBool              sExistPartitionByColumn;
    idBool              sExistOrderByColumn;
    qmcWndExecMethod    sExecMethod;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeWndNode::__FT__" );

    // 초기화
    sSameMtrNode            = NULL;
    sFirstOverColumnNode    = NULL;
    sLastOverColumnNode     = NULL;
    sExistPartitionByColumn = ID_FALSE;
    sExistOrderByColumn     = ID_FALSE;
    sExecMethod             = QMC_WND_EXEC_NONE;

    //-----------------------------
    // Wnd Node 생성
    //-----------------------------

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmcWndNode ),
                                               (void**) & sNewWndNode )
              != IDE_SUCCESS );

    sWindow = aAnalticFuncNode->overClause->window;
    //-----------------------------
    // overColumnNode 생성
    //-----------------------------

    for ( sCurOverColumn  = aAnalticFuncNode->overClause->overColumn;
          sCurOverColumn != NULL;
          sCurOverColumn  = sCurOverColumn->next )
    {
        *aOverColumnNodeCount = *aOverColumnNodeCount + 1;

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmcMtrNode ),
                                                   (void**)& sNewMtrNode )
                  != IDE_SUCCESS);

        // BUG-34966 Pass node 일 수 있으므로 실제 값의 위치를 설정한다.
        sNode = &sCurOverColumn->node->node;

        while( sNode->module == &qtc::passModule )
        {
            sNode = sNode->arguments;
        }

        // 칼럼의 현재 (table, column) 정보를 찾음
        IDE_TEST( qmg::findColumnLocate( aStatement,
                                         aTupleID,
                                         sNode->table,
                                         sNode->column,
                                         & sTable,
                                         & sColumn )
                  != IDE_SUCCESS );

        for ( sCurMtrNode  = aMtrNode;
              sCurMtrNode != NULL;
              sCurMtrNode  = sCurMtrNode->next )
        {
            if ( ( ( sCurMtrNode->srcNode->node.table == sTable )
                   &&
                   ( sCurMtrNode->srcNode->node.column == sColumn ) ) ||
                ( ( sCurMtrNode->dstNode->node.table == sTable )
                   &&
                   ( sCurMtrNode->dstNode->node.column == sColumn ) ) )
            {
                if ( (sCurMtrNode->flag & QMC_MTR_BASETABLE_MASK)
                     != QMC_MTR_BASETABLE_TRUE )
                {
                    // table을 표현하기 위한 column이 아닌 경우

                    // BUG-33663 Ranking Function
                    // mtr node가 partition column인지 order column인지 구분
                    if ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                         == QTC_OVER_COLUMN_ORDER_BY )
                    {
                        sExistOrderByColumn = ID_TRUE;

                        if ( (sCurMtrNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                             == QMC_MTR_SORT_ORDER_FIXED_TRUE )
                        {
                            if ( ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                                   == QTC_OVER_COLUMN_ORDER_ASC )
                                 &&
                                 ( (sCurMtrNode->flag & QMC_MTR_SORT_ORDER_MASK)
                                   == QMC_MTR_SORT_ASCENDING ) )
                            {
                                // BUG-42145 Nulls Option 이 다른 경우도
                                // 체크해야한다.
                                if ( ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                       == QTC_OVER_COLUMN_NULLS_ORDER_NONE ) &&
                                     ( ( sCurMtrNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                       == QMC_MTR_SORT_NULLS_ORDER_NONE ) )
                                {
                                    sSameMtrNode = sCurMtrNode;
                                    break;
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
                                if ( ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                       == QTC_OVER_COLUMN_NULLS_ORDER_FIRST ) &&
                                     ( ( sCurMtrNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                       == QMC_MTR_SORT_NULLS_ORDER_FIRST ) )
                                {
                                    sSameMtrNode = sCurMtrNode;
                                    break;
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
                                if ( ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                       == QTC_OVER_COLUMN_NULLS_ORDER_LAST ) &&
                                     ( ( sCurMtrNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                       == QMC_MTR_SORT_NULLS_ORDER_LAST ) )
                                {
                                    sSameMtrNode = sCurMtrNode;
                                    break;
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
                            }
                            else
                            {
                                // Noting to do.
                            }

                            if ( ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                                   == QTC_OVER_COLUMN_ORDER_DESC )
                                 &&
                                 ( (sCurMtrNode->flag & QMC_MTR_SORT_ORDER_MASK)
                                   == QMC_MTR_SORT_DESCENDING ) )
                            {
                                // BUG-42145 Nulls Option 이 다른 경우도
                                // 체크해야한다.
                                if ( ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                       == QTC_OVER_COLUMN_NULLS_ORDER_NONE ) &&
                                     ( ( sCurMtrNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                       == QMC_MTR_SORT_NULLS_ORDER_NONE ) )
                                {
                                    sSameMtrNode = sCurMtrNode;
                                    break;
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
                                if ( ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                       == QTC_OVER_COLUMN_NULLS_ORDER_FIRST ) &&
                                     ( ( sCurMtrNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                       == QMC_MTR_SORT_NULLS_ORDER_FIRST ) )
                                {
                                    sSameMtrNode = sCurMtrNode;
                                    break;
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
                                if ( ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                       == QTC_OVER_COLUMN_NULLS_ORDER_LAST ) &&
                                     ( ( sCurMtrNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                       == QMC_MTR_SORT_NULLS_ORDER_LAST ) )
                                {
                                    sSameMtrNode = sCurMtrNode;
                                    break;
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
                            }
                            else
                            {
                                // Noting to do.
                            }
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        sExistPartitionByColumn = ID_TRUE;

                        if ( (sCurMtrNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                             == QMC_MTR_SORT_ORDER_FIXED_FALSE )
                        {
                            sSameMtrNode = sCurMtrNode;
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    // table을 표현하기 위한 column인 경우
                    // 다른 칼럼임에도 불구하고 같을 수 있음
                }
            }
            else
            {
                // nothing to
            }
        }

        // Analytic Function을 위한 column의 materialize node는
        // 이미 qmg::makeColumn4Analytic()에 의해 추가됨
        IDE_TEST_RAISE( sSameMtrNode == NULL, ERR_INVALID_NODE );

        // mtr노드 복사
        idlOS::memcpy( sNewMtrNode,
                       sSameMtrNode,
                       ID_SIZEOF(qmcMtrNode) );

        sNewMtrNode->next = NULL;

        if ( sFirstOverColumnNode == NULL )
        {
            sFirstOverColumnNode = sNewMtrNode;
            sLastOverColumnNode = sNewMtrNode;
        }
        else
        {
            sLastOverColumnNode->next = sNewMtrNode;
            sLastOverColumnNode       = sLastOverColumnNode->next;
        }
    }

    // BUG-33663 Ranking Function
    // window node의 동작방식을 결정한다.
    if ( (sExistPartitionByColumn == ID_TRUE) && (sExistOrderByColumn == ID_TRUE) )
    {
        if ( sWindow == NULL )
        {
            if ( ( aAnalticFuncNode->node.module == &mtfLag ) ||
                 ( aAnalticFuncNode->node.module == &mtfLagIgnoreNulls ) )
            {
                sExecMethod = QMC_WND_EXEC_PARTITION_ORDER_UPDATE_LAG;
            }
            else if ( ( aAnalticFuncNode->node.module == &mtfLead ) ||
                      ( aAnalticFuncNode->node.module == &mtfLeadIgnoreNulls ) )
            {
                sExecMethod = QMC_WND_EXEC_PARTITION_ORDER_UPDATE_LEAD;
            }
            /* BUG43086 support ntile analytic function */
            else if ( aAnalticFuncNode->node.module == &mtfNtile )
            {
                sExecMethod = QMC_WND_EXEC_PARTITION_ORDER_UPDATE_NTILE;
            }
            else
            {
                // partition & order by aggregation and update
                sExecMethod = QMC_WND_EXEC_PARTITION_ORDER_UPDATE;
            }
        }
        else
        {
            sExecMethod = QMC_WND_EXEC_PARTITION_ORDER_WINDOW_UPDATE;
        }
    }
    else
    {
        if ( sExistPartitionByColumn == ID_TRUE )
        {
            // partition by aggregation and update
            sExecMethod = QMC_WND_EXEC_PARTITION_UPDATE;
        }
        else
        {
            if ( sExistOrderByColumn == ID_TRUE )
            {
                if ( sWindow == NULL )
                {
                    if ( ( aAnalticFuncNode->node.module == &mtfLag ) ||
                         ( aAnalticFuncNode->node.module == &mtfLagIgnoreNulls ) )
                    {
                        sExecMethod = QMC_WND_EXEC_ORDER_UPDATE_LAG;
                    }
                    else if ( ( aAnalticFuncNode->node.module == &mtfLead ) ||
                              ( aAnalticFuncNode->node.module == &mtfLeadIgnoreNulls ) )
                    {
                        sExecMethod = QMC_WND_EXEC_ORDER_UPDATE_LEAD;
                    }
                    /* BUG43086 support ntile analytic function */
                    else if ( aAnalticFuncNode->node.module == &mtfNtile )
                    {
                        sExecMethod = QMC_WND_EXEC_ORDER_UPDATE_NTILE;
                    }
                    else
                    {
                        // order by aggregation and update
                        sExecMethod = QMC_WND_EXEC_ORDER_UPDATE;
                    }
                }
                else
                {
                    sExecMethod = QMC_WND_EXEC_ORDER_WINDOW_UPDATE;
                }
            }
            else
            {
                // aggregation and update
                sExecMethod = QMC_WND_EXEC_AGGR_UPDATE;
            }
        }
    }

    if ( sWindow != NULL )
    {
        sNewWndNode->window.rowsOrRange = sWindow->rowsOrRange;
        if ( sWindow->isBetween == ID_TRUE )
        {
            sNewWndNode->window.startOpt = sWindow->start->option;
            if ( sWindow->start->value != NULL )
            {
                sNewWndNode->window.startValue = *sWindow->start->value;
            }
            else
            {
                sNewWndNode->window.startValue.number = 0;
                sNewWndNode->window.startValue.type   = QTC_OVER_WINDOW_VALUE_TYPE_NUMBER;
            }

            sNewWndNode->window.endOpt = sWindow->end->option;
            if ( sWindow->end->value != NULL )
            {
                sNewWndNode->window.endValue = *sWindow->end->value;
            }
            else
            {
                sNewWndNode->window.endValue.number = 0;
                sNewWndNode->window.endValue.type   = QTC_OVER_WINDOW_VALUE_TYPE_NUMBER;
            }
        }
        else
        {
            sNewWndNode->window.startOpt = sWindow->start->option;
            if ( sWindow->start->value != NULL )
            {
                sNewWndNode->window.startValue = *sWindow->start->value;
            }
            else
            {
                sNewWndNode->window.startValue.number = 0;
                sNewWndNode->window.startValue.type   = QTC_OVER_WINDOW_VALUE_TYPE_NUMBER;
            }
            sNewWndNode->window.endOpt = QTC_OVER_WINODW_OPT_CURRENT_ROW;
            sNewWndNode->window.endValue.number = 0;
            sNewWndNode->window.endValue.type   = QTC_OVER_WINDOW_VALUE_TYPE_NUMBER;
        }
    }
    else
    {
        sNewWndNode->window.rowsOrRange = QTC_OVER_WINDOW_NONE;
    }

    sNewWndNode->overColumnNode = sFirstOverColumnNode;
    sNewWndNode->aggrNode       = NULL;
    sNewWndNode->aggrResultNode = NULL;
    sNewWndNode->execMethod     = sExecMethod;
    sNewWndNode->next           = NULL;

    *aNewWndNode = sNewWndNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeWndNode",
                                  "Invalid node" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeMyNodeOfWNST( qcStatement      * aStatement,
                                 qmsQuerySet      * aQuerySet,
                                 qmnPlan          * aPlan,
                                 qmsAnalyticFunc ** aAnalFuncListPtrArr,
                                 UInt               aAnalFuncListPtrCnt,
                                 UShort             aTupleID,
                                 qmncWNST         * aWNST,
                                 UShort           * aColumnCountOfMyNode,
                                 qmcMtrNode      ** aFirstAnalResultFuncMtrNode)
{
/***********************************************************************
 *
 * Description : WNST의 myNode 생성
 *
 * Implementation :
 *     WNST의 myNode는 temp table에 저장될 칼럼 정보로써
 *     아래와 같은 칼럼 정보들로 구성된다.
 *
 *     [Base Table] + [Columns] + [Analytic Function]
 *
 *     < output >
 *         aMyNode        : WNST plan의 myNode(materialize node) 구성
 *         baseTableCount : myNode중 baseTableCount
 *         aColumnCount   : myNode 전체 칼럼 개수
 *         aFirstAnalResultFuncMtrNode : myNode의 analytic function
 *                                       result 저장될 칼럼들 중 첫번째
 *
 ***********************************************************************/

    qmcMtrNode      * sMyNode;
    UShort            sColumnCount;
    qmcMtrNode      * sLastMtrNode;
    qmsAnalyticFunc * sCurAnalyticFunc;
    qmcMtrNode      * sFirstAnalResultFuncMtrNode;

    UInt              i;
    qmcMtrNode      * sNewMtrNode = NULL;
    qmcAttrDesc     * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeMyNodeOfWNST::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    // 기본 초기화
    sFirstAnalResultFuncMtrNode = NULL;
    sMyNode = NULL;

    //-----------------------------------------------
    // 1. Base Table 생성
    //-----------------------------------------------

    sColumnCount = 0;
    sLastMtrNode = NULL;

    // Sorting key가 아닌 경우
    IDE_TEST( makeNonKeyAttrsMtrNodes( aStatement,
                                       aQuerySet,
                                       aPlan->resultDesc,
                                       aTupleID,
                                       & sMyNode,
                                       & sLastMtrNode,
                                       & sColumnCount )
              != IDE_SUCCESS );

    aWNST->baseTableCount  = sColumnCount;

    //-----------------------------------------------
    // 2. Column들의 생성
    //    Analytic Function의 argument column과
    //    Partition By column
    //    ( myNode를 보고 중복되는 칼럼이 없는 경우에만
    //      last mtr node의 next에 연결한다. )
    //-----------------------------------------------
    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        // Sorting key 인 경우
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              aTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_SORT_NEED_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_NEED_TRUE;

            if ( ( sItrAttr->flag & QMC_ATTR_ANALYTIC_SORT_MASK ) == QMC_ATTR_ANALYTIC_SORT_TRUE )
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_FIXED_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_ORDER_FIXED_TRUE;
            }
            else
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_FIXED_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_ORDER_FIXED_FALSE;
            }

            if ( ( sItrAttr->flag & QMC_ATTR_SORT_ORDER_MASK )
                  == QMC_ATTR_SORT_ORDER_ASC )
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
            }
            else
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_DESCENDING;
            }

            /* PROJ-2435 Order by Nulls first/last */
            if ( ( sItrAttr->flag & QMC_ATTR_SORT_NULLS_ORDER_MASK )
                  == QMC_ATTR_SORT_NULLS_ORDER_NONE )
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_NONE;
            }
            else
            {
                if ( ( sItrAttr->flag & QMC_ATTR_SORT_NULLS_ORDER_MASK )
                      == QMC_ATTR_SORT_NULLS_ORDER_FIRST )
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_FIRST;
                }
                else
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_LAST;
                }
            }

            if( sMyNode == NULL )
            {
                sMyNode             = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    //-----------------------------------------------
    // 3. Analytic Function 결과가 저장될 칼럼을 생성하여
    //    myNode에 연결
    //-----------------------------------------------

    for ( i = 0;
          i < aAnalFuncListPtrCnt;
          i++ )
    {
        for ( sCurAnalyticFunc  = aAnalFuncListPtrArr[i];
              sCurAnalyticFunc != NULL ;
              sCurAnalyticFunc  = sCurAnalyticFunc->next)
         {
             IDE_TEST( qmg::makeAnalFuncResultMtrNode( aStatement,
                                                       sCurAnalyticFunc,
                                                       aTupleID,
                                                       & sColumnCount,
                                                       & sMyNode,
                                                       & sLastMtrNode )
                       != IDE_SUCCESS );

             if ( sFirstAnalResultFuncMtrNode == NULL )
             {
                 sFirstAnalResultFuncMtrNode = sLastMtrNode;
             }
             else
             {
                 // nothing to do
             }
         }
    }

    aWNST->myNode = sMyNode;
    *aColumnCountOfMyNode = sColumnCount;
    *aFirstAnalResultFuncMtrNode = sFirstAnalResultFuncMtrNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneMtrPlan::findPriorColumns( qcStatement * aStatement,
                                        qtcNode     * aNode,
                                        qtcNode     * aFirst )
{
    qtcNode   * sNode      = NULL;
    qtcNode   * sPrev      = NULL;
    qtcNode   * sTmp       = NULL;
    idBool      sIsSame    = ID_FALSE;
    mtcColumn * sMtcColumn = NULL;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::findPriorColumns::__FT__" );

    if ( ( ( aNode->lflag & QTC_NODE_PRIOR_MASK) ==
           QTC_NODE_PRIOR_EXIST ) &&
           ( aNode->node.module == &qtc::columnModule ))
    {
        sPrev = (qtcNode *)aFirst->node.next;
        for ( sNode  = sPrev;
              sNode != NULL;
              sNode  = (qtcNode *) sNode->node.next )
        {
            if ( sNode->node.column == aNode->node.column )
            {
                sIsSame = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
            sPrev = sNode;
        }

        if ( sIsSame == ID_FALSE )
        {
            sMtcColumn = QTC_TMPL_COLUMN( QC_SHARED_TMPLATE( aStatement ),
                                          aNode );

            IDE_TEST_RAISE( ( sMtcColumn->module->id == MTD_CLOB_ID ) ||
                            ( sMtcColumn->module->id == MTD_CLOB_LOCATOR_ID ) ||
                            ( sMtcColumn->module->id == MTD_BLOB_ID ) ||
                            ( sMtcColumn->module->id == MTD_BLOB_LOCATOR_ID ),
                            ERR_NOT_SUPPORT_PRIOR_LOB );

            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                    qtcNode,
                                    & sTmp )
                      != IDE_SUCCESS );
            idlOS::memcpy( sTmp, aNode, sizeof( qtcNode ) );
            sTmp->node.arguments = NULL;
            sTmp->node.next      = NULL;

            if ( sPrev == NULL )
            {
                aFirst->node.next = (mtcNode *)sTmp;
            }
            else
            {
                sPrev->node.next = (mtcNode *)sTmp;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.arguments != NULL )
    {
        IDE_TEST( findPriorColumns( aStatement,
                                    (qtcNode *)aNode->node.arguments,
                                    aFirst )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.next != NULL )
    {
        IDE_TEST( findPriorColumns( aStatement,
                                    (qtcNode *)aNode->node.next,
                                    aFirst )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_PRIOR_LOB )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOW_PRIOR_LOB ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneMtrPlan::findPriorPredAndSortNode( qcStatement  * aStatement,
                                                qtcNode      * aNode,
                                                qtcNode     ** aSortNode,
                                                qtcNode     ** aPriorPred,
                                                UShort         aFromTable,
                                                idBool       * aFind )
{
    qtcNode * sOperator = NULL;
    qtcNode * sNode1    = NULL;
    qtcNode * sNode2    = NULL;
    qtcNode * sTmpNode  = NULL;
    qtcNode * sTmpNode1 = NULL;
    qtcNode * sTmpNode2 = NULL;
    UInt      sCount    = 0;
    idBool    sFind     = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::findPriorPredAndSortNode::__FT__" );

    if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK ) ==
         MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sOperator = (qtcNode *)aNode->node.arguments;
        if ( sOperator != NULL && *aFind == ID_FALSE )
        {
            IDE_TEST( findPriorPredAndSortNode( aStatement,
                                                sOperator,
                                                aSortNode,
                                                aPriorPred,
                                                aFromTable,
                                                aFind )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
        sOperator = (qtcNode *)aNode->node.next;

        if ( sOperator != NULL && *aFind == ID_FALSE )
        {
            IDE_TEST( findPriorPredAndSortNode( aStatement,
                                                sOperator,
                                                aSortNode,
                                                aPriorPred,
                                                aFromTable,
                                                aFind )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        sOperator = aNode;
    }

    for ( ; sOperator != NULL ; sOperator = ( qtcNode * )sOperator->node.next )
    {
        sFind = ID_FALSE;
        if ( *aFind == ID_TRUE )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sOperator->node.lflag & MTC_NODE_INDEX_JOINABLE_MASK ) !=
              MTC_NODE_INDEX_JOINABLE_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        sNode1 = ( qtcNode * )sOperator->node.arguments;

        if( sNode1 != NULL )
        {
            sNode2 = ( qtcNode * )sNode1->node.next;
        }
        else
        {
            sNode2 = NULL;
        }

        if ( (sNode1 != NULL) && (sNode2 != NULL) )
        {
            if ( ( (sNode1->lflag & QTC_NODE_PRIOR_MASK) ==
                   QTC_NODE_PRIOR_EXIST ) &&
                   ( sNode1->node.module == &qtc::columnModule ))
            {
                sCount++;
            }
            else
            {
                *aSortNode = sNode1;
            }
            if ( ( (sNode2->lflag & QTC_NODE_PRIOR_MASK) ==
                   QTC_NODE_PRIOR_EXIST ) &&
                   ( sNode2->node.module == &qtc::columnModule ))
            {
                sCount++;
            }
            else
            {
                *aSortNode = sNode2;
            }

            /*
             * 1. sortNode는 컬럼이어야 한다.
             * 2. 컬럼에 conversion이 없어야 한다.
             * 3. 컬럼 dependency는 from table이어야 한다.
             */
            if ( ( sCount == 1 ) &&
                 ( ( *aSortNode )->node.module == &qtc::columnModule ) &&
                 ( ( *aSortNode )->node.conversion == NULL ) &&
                 ( ( *aSortNode )->node.table == aFromTable ) )
            {
                sFind = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }

            if ( sFind == ID_TRUE )
            {
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                        qtcNode,
                                        & sTmpNode )
                          != IDE_SUCCESS );
                idlOS::memcpy( sTmpNode, *aSortNode, sizeof( qtcNode ) );
                sTmpNode->node.next = NULL;
                *aSortNode = sTmpNode;

                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                        qtcNode,
                                        & sTmpNode )
                          != IDE_SUCCESS );
                idlOS::memcpy( sTmpNode, sOperator, sizeof( qtcNode ) );
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                        qtcNode,
                                        & sTmpNode1 )
                          != IDE_SUCCESS );
                idlOS::memcpy( sTmpNode1, sNode1, sizeof( qtcNode ) );
                sTmpNode->node.arguments = ( mtcNode * )sTmpNode1;

                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                        qtcNode,
                                        & sTmpNode2 )
                          != IDE_SUCCESS );
                idlOS::memcpy( sTmpNode2, sNode2, sizeof( qtcNode ) );
                sTmpNode1->node.next = ( mtcNode * )sTmpNode2;
                *aPriorPred          = sTmpNode;
                *aFind               = ID_TRUE;
                break;
            }
            else
            {
                *aSortNode = NULL;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneMtrPlan::processStartWithPredicate( qcStatement * aStatement,
                                                 qmsQuerySet * aQuerySet,
                                                 qmncCNBY    * aCNBY,
                                                 qmoLeafInfo * aStartWith )
{
    qmncScanMethod sMethod;
    idBool         sIsSubQuery = ID_FALSE;
    idBool         sFind       = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::processStartWithPredicate::__FT__" );

    /* Start With Predicate 의 처리 */
    IDE_TEST( qmoOneNonPlan::processPredicate( aStatement,
                                               aQuerySet,
                                               aStartWith->predicate,
                                               aStartWith->constantPredicate,
                                               aStartWith->ridPredicate,
                                               aStartWith->index,
                                               aCNBY->myRowID,
                                               & sMethod.constantFilter,
                                               & sMethod.filter,
                                               & sMethod.lobFilter,
                                               & sMethod.subqueryFilter,
                                               & sMethod.varKeyRange,
                                               & sMethod.varKeyFilter,
                                               & sMethod.varKeyRange4Filter,
                                               & sMethod.varKeyFilter4Filter,
                                               & sMethod.fixKeyRange,
                                               & sMethod.fixKeyFilter,
                                               & sMethod.fixKeyRange4Print,
                                               & sMethod.fixKeyFilter4Print,
                                               & sMethod.ridRange,
                                               & sIsSubQuery )
              != IDE_SUCCESS );

    IDE_TEST( qmoOneNonPlan::postProcessScanMethod( aStatement,
                                                    & sMethod,
                                                    & sFind )
              != IDE_SUCCESS );

    if ( sMethod.constantFilter != NULL )
    {
        aCNBY->startWithConstant = sMethod.constantFilter;
    }
    else
    {
        aCNBY->startWithConstant = NULL;
    }

    if ( sMethod.filter != NULL )
    {
        aCNBY->startWithFilter = sMethod.filter;
    }
    else
    {
        aCNBY->startWithFilter = NULL;
    }

    if ( sMethod.subqueryFilter != NULL )
    {
        aCNBY->startWithSubquery = sMethod.subqueryFilter;
    }
    else
    {
        aCNBY->startWithSubquery = NULL;
    }

    if ( aStartWith->nnfFilter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement,
                                                    aStartWith->nnfFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    aCNBY->startWithNNF = aStartWith->nnfFilter;

    /* Hierarchy를 항상 inline view로 만들어 사용하기 때문에 lobFilter 가 나올수 없다 */
    IDE_TEST_RAISE ( sMethod.lobFilter != NULL, ERR_INTERNAL )

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qmoOneMtrPlan::processStartWithPredicate",
                                "lobFilter is not NULL"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Connect By 구문의 Predicate를 처리한다.
 *
 *  1. level Filter를 처리한다.
 *  2. processPredicate 를 통해서 Filter와 ConstantFilter를 얻는다.
 *     나머지는 사용되지 않는다.
 *  3. ConstantFilter를 지정한다.
 *  4. Filter가 있다면 Prior 가 1개 붙은 노드를 찾아 Prior노드와 Sort Node를 지정한다.
 *     이때 KeryRange 노드도 복사된다.
 *  5. makeCNBYMtrNode를 통해 baseMTR 즉 CMTR Materialize의 SortNode를 지정한다.
 *     이를 통해서 baseMTR을 Sort하게된다. 이는 레벨이 1인 Row에 대해 Order Siblgins By
 *     를 지원하기 위해서이다.
 *  6. Prior가 1개인 노드를 찾으면 이를 통해 KeyRange를 생성하고 따로 Sort Table에
 *     쌓게된다. 이를 위해 TupleID를 할당 받는다.
 *  7. Order Siblings by를 위한 Composite MtrNode 생성
 *  8. CNF의 형태의 Prior SortNode를 DNF의 형태로 변환
 *
 */
IDE_RC qmoOneMtrPlan::processConnectByPredicate( qcStatement    * aStatement,
                                                 qmsQuerySet    * aQuerySet,
                                                 qmsFrom        * aFrom,
                                                 qmncCNBY       * aCNBY,
                                                 qmoLeafInfo    * aConnectBy )
{
    qmncScanMethod sMethod;
    idBool         sIsSubQuery    = ID_FALSE;
    idBool         sFind          = ID_FALSE;
    qtcNode      * sNode          = NULL;
    qtcNode      * sSortNode      = NULL;
    qtcNode      * sPriorPred     = NULL;
    qtcNode      * sOptimizedNode = NULL;
    qmcMtrNode   * sMtrNode       = NULL;
    qmcMtrNode   * sTmp           = NULL;
    UInt           sCurOffset     = 0;
    qtcNode        sPriorTmp;
    qmoPredicate * sPred          = NULL;
    qmoPredicate * sPredMore      = NULL;

    qmcAttrDesc  * sItrAttr;
    qmcMtrNode   * sFirstMtrNode  = NULL;
    qmcMtrNode   * sLastMtrNode   = NULL;
    UShort         sColumnCount   = 0;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::processConnectByPredicate::__FT__" );

    aCNBY->levelFilter       = NULL;
    aCNBY->rownumFilter      = NULL;
    aCNBY->connectByKeyRange = NULL;
    aCNBY->connectByFilter   = NULL;
    aCNBY->priorNode         = NULL;

    /* 1. Level Filter 의 처리 */
    if ( aConnectBy->levelPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          aConnectBy->levelPredicate,
                                          & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement,
                                           aQuerySet,
                                           sNode )
                  != IDE_SUCCESS );

        /* BUG-17921 */
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                                & sOptimizedNode )
                   != IDE_SUCCESS );
        aCNBY->levelFilter = sOptimizedNode;

        IDE_TEST( qtc::optimizeHostConstExpression( aStatement,
                                                    aCNBY->levelFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-39434 The connect by need rownum pseudo column.
     * 1-2 Rownum Filter 의 처리
     */
    if ( aConnectBy->connectByRownumPred != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          aConnectBy->connectByRownumPred,
                                          & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement,
                                           aQuerySet,
                                           sNode )
                  != IDE_SUCCESS );

        /* BUG-17921 */
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );
        aCNBY->rownumFilter = sOptimizedNode;

        IDE_TEST( qtc::optimizeHostConstExpression( aStatement,
                                                    aCNBY->rownumFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2641 Hierarchy Query Index
     * Connectby predicate에서 prior Node를 찾아서
     * Error가 나는 상황을 체크해야한다.
     */
    for ( sPred = aConnectBy->predicate; sPred != NULL; sPred = sPred->next )
    {
        for ( sPredMore = sPred;
              sPredMore != NULL;
              sPredMore = sPredMore->more )
        {
            sPriorTmp.node.next = NULL;
            IDE_TEST( findPriorColumns( aStatement,
                                        sPredMore->node,
                                        & sPriorTmp )
                      != IDE_SUCCESS );
            /* BUG-44759 processPredicae을 수행하기전에 priorNode
             * 가 있어야한다.
             */
            aCNBY->priorNode = (qtcNode *)sPriorTmp.node.next;
        }
    }

    /* 2. Connect By Predicate 의 처리 */
    IDE_TEST( qmoOneNonPlan::processPredicate( aStatement,
                                               aQuerySet,
                                               aConnectBy->predicate,
                                               aConnectBy->constantPredicate,
                                               aConnectBy->ridPredicate,
                                               aConnectBy->index,
                                               aCNBY->myRowID,
                                               & sMethod.constantFilter,
                                               & sMethod.filter,
                                               & sMethod.lobFilter,
                                               & sMethod.subqueryFilter,
                                               & sMethod.varKeyRange,
                                               & sMethod.varKeyFilter,
                                               & sMethod.varKeyRange4Filter,
                                               & sMethod.varKeyFilter4Filter,
                                               & sMethod.fixKeyRange,
                                               & sMethod.fixKeyFilter,
                                               & sMethod.fixKeyRange4Print,
                                               & sMethod.fixKeyFilter4Print,
                                               & sMethod.ridRange,
                                               & sIsSubQuery )
              != IDE_SUCCESS );

    /* 3. constantFilter 지정 */
    if ( sMethod.constantFilter != NULL )
    {
        aCNBY->connectByConstant = sMethod.constantFilter;
    }
    else
    {
        aCNBY->connectByConstant = NULL;
    }

    /* BUG-39401 need subquery for connect by clause */
    if ( sMethod.subqueryFilter != NULL )
    {
        aCNBY->connectBySubquery = sMethod.subqueryFilter;
    }
    else
    {
        aCNBY->connectBySubquery = NULL;
    }

    if ( ( sMethod.varKeyRange == NULL ) &&
         ( sMethod.varKeyFilter == NULL ) &&
         ( sMethod.varKeyRange4Filter == NULL ) &&
         ( sMethod.varKeyFilter4Filter == NULL ) &&
         ( sMethod.fixKeyRange == NULL ) &&
         ( sMethod.fixKeyFilter == NULL ) )
    {
        aCNBY->mIndex = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST_RAISE( sMethod.lobFilter != NULL, ERR_NOT_SUPPORT_LOB_FILTER );

    if ( sMethod.filter != NULL )
    {
        aCNBY->connectByFilter = sMethod.filter;

        /* 4-1. find Prior column list */
        sPriorTmp.node.next = NULL;
        IDE_TEST( findPriorColumns( aStatement,
                                    sMethod.filter,
                                    & sPriorTmp )
                  != IDE_SUCCESS );
        aCNBY->priorNode = (qtcNode *)sPriorTmp.node.next;

        /* 4-2. find Prior predicate and Sort node */
        IDE_TEST( findPriorPredAndSortNode( aStatement,
                                            sMethod.filter,
                                            & sSortNode,
                                            & sPriorPred,
                                            aFrom->tableRef->table,
                                            & sFind )
                  != IDE_SUCCESS );

        /* 5. baseSort Node 지정 */
        for ( sItrAttr  = aCNBY->plan.resultDesc;
              sItrAttr != NULL;
              sItrAttr  = sItrAttr->next )
        {
            if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
            {
                IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                                  aQuerySet,
                                                  sItrAttr->expr,
                                                  ID_FALSE,
                                                  aCNBY->baseRowID,
                                                  0,
                                                  & sColumnCount,
                                                  & sMtrNode )
                          != IDE_SUCCESS );

                sMtrNode->srcNode->node.table = aCNBY->baseRowID;

                sMtrNode->flag &= ~QMC_MTR_SORT_NEED_MASK;
                sMtrNode->flag |= QMC_MTR_SORT_NEED_TRUE;

                if ( ( sItrAttr->flag & QMC_ATTR_SORT_ORDER_MASK )
                     == QMC_ATTR_SORT_ORDER_ASC )
                {
                    sMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                    sMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
                }
                else
                {
                    sMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                    sMtrNode->flag |= QMC_MTR_SORT_DESCENDING;
                }

                if( sFirstMtrNode == NULL )
                {
                    sFirstMtrNode       = sMtrNode;
                    sLastMtrNode        = sMtrNode;
                }
                else
                {
                    sLastMtrNode->next  = sMtrNode;
                    sLastMtrNode        = sMtrNode;
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        aCNBY->baseSortNode = sFirstMtrNode;

        if ( ( sFind == ID_TRUE ) &&
             ( aCNBY->mIndex == NULL ) )
        {
            IDE_TEST( makeSortNodeOfCNBY( aStatement,
                                          aQuerySet,
                                          aCNBY,
                                          sSortNode )
                      != IDE_SUCCESS );

            /* CNF sPriorPredicate 를 DNF 형태로 변환 */
            IDE_TEST( qmoNormalForm::normalizeDNF( aStatement,
                                                   sPriorPred,
                                                   & aCNBY->connectByKeyRange)
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        /* fix BUG-19074 Host 변수를 포함한 Constant Expression 의 최적화 */
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement,
                                                    aCNBY->connectByFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    aCNBY->mVarKeyRange         = sMethod.varKeyRange;
    aCNBY->mVarKeyFilter        = sMethod.varKeyFilter;
    aCNBY->mVarKeyRange4Filter  = sMethod.varKeyRange4Filter;
    aCNBY->mVarKeyFilter4Filter = sMethod.varKeyFilter4Filter;
    aCNBY->mFixKeyRange         = sMethod.fixKeyRange;
    aCNBY->mFixKeyFilter        = sMethod.fixKeyFilter;
    aCNBY->mFixKeyRange4Print   = sMethod.fixKeyRange4Print;
    aCNBY->mFixKeyFilter4Print  = sMethod.fixKeyFilter4Print;

    sColumnCount = 0;
    for ( sTmp = aCNBY->sortNode; sTmp != NULL ; sTmp = sTmp->next )
    {
        sColumnCount++;
    }

    sCurOffset = aCNBY->mtrNodeOffset;

    // 다음 노드가 저장될 시작 지점
    sCurOffset += idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sColumnCount;

    sColumnCount = 0;
    for ( sTmp = aCNBY->baseSortNode; sTmp != NULL ; sTmp = sTmp->next )
    {
        sColumnCount++;
    }

    aCNBY->baseSortOffset = sCurOffset;
    sCurOffset = sCurOffset + idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sColumnCount;

    aCNBY->sortMTROffset = sCurOffset;
    sCurOffset += idlOS::align8(ID_SIZEOF(qmcdSortTemp));

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sCurOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_LOB_FILTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMO_NOT_ALLOWED_LOB_FILTER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneMtrPlan::makeSortNodeOfCNBY( qcStatement * aStatement,
                                          qmsQuerySet * aQuerySet,
                                          qmncCNBY    * aCNBY,
                                          qtcNode     * aSortNode )
{
    UShort         sSortID       = 0;
    UShort         sColumnCount  = 0;
    UInt           sFlag         = 0;
    mtcTemplate  * sMtcTemplate  = NULL;
    qmcMtrNode   * sMtrNode      = NULL;
    qmcMtrNode   * sFirstMtrNode = NULL;
    qmcMtrNode   * sLastMtrNode  = NULL;
    qmcAttrDesc  * sAttr         = NULL;
    qmcAttrDesc  * sItrAttr      = NULL;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeSortNodeOfCNBY::__FT__" );

    sMtcTemplate = &QC_SHARED_TMPLATE(aStatement)->tmplate;

    /* 6. KeyRange를 생성해서 Sort할 튜풀 생성 */

    // BUG-36472 (table tuple->intermediate tuple)
    IDE_TEST( qtc::nextTable( & sSortID,
                              aStatement,
                              NULL,
                              ID_FALSE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );
    sMtcTemplate->rows[sSortID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sSortID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

    /* 7. Order Siblings by 노드 처리 */
    sFirstMtrNode = NULL;
    sLastMtrNode = NULL;
    sColumnCount = 0;

    IDE_TEST( qmc::findAttribute( aStatement,
                                  aCNBY->plan.resultDesc,
                                  aSortNode,
                                  ID_FALSE,
                                  & sAttr )
              != IDE_SUCCESS );

    if ( sAttr != NULL )
    {
        // PROJ-2179
        // Sort node는 materialize node의 가장 처음에 항상 위치하므로
        // 만약 siblings에 포함된 경우 key flag를 해제한다.
        sAttr->flag &= ~QMC_ATTR_KEY_MASK;
        sAttr->flag |= QMC_ATTR_KEY_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    aSortNode->node.table = aCNBY->baseRowID;

    if ( ( ( aCNBY->flag & QMNC_CNBY_CHILD_VIEW_MASK )
            == QMNC_CNBY_CHILD_VIEW_FALSE ) &&
         ( ( aCNBY->flag & QMNC_CNBY_JOIN_MASK )
           == QMNC_CNBY_JOIN_FALSE ) )
    {
        if ( ( aCNBY->plan.flag & QMN_PLAN_STORAGE_MASK )
             == QMN_PLAN_STORAGE_DISK )
        {
            sFlag &= ~QMG_MAKECOLUMN_MTR_NODE_NOT_CHANGE_COLUMN_MASK;
            sFlag |= QMG_MAKECOLUMN_MTR_NODE_NOT_CHANGE_COLUMN_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( qmg::makeBaseTableMtrNode( aStatement,
                                             aSortNode,
                                             sSortID,
                                             & sColumnCount,
                                             & sMtrNode )
                  != IDE_SUCCESS );
        sFirstMtrNode = sMtrNode;
        sLastMtrNode = sMtrNode;
    }
    else
    {
        /* Nothing to do */
    }
    IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                      aQuerySet,
                                      aSortNode,
                                      ID_FALSE,
                                      sSortID,
                                      sFlag,
                                      & sColumnCount,
                                      & sMtrNode )
              != IDE_SUCCESS );

    if ( sFirstMtrNode == NULL )
    {
        sFirstMtrNode = sMtrNode;
        sLastMtrNode = sMtrNode;
    }
    else
    {
        sLastMtrNode->next  = sMtrNode;
        sLastMtrNode        = sMtrNode;
    }

    for ( sItrAttr  = aCNBY->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sSortID,
                                              0,
                                              & sColumnCount,
                                              & sMtrNode )
                      != IDE_SUCCESS );

            sMtrNode->srcNode->node.table = aCNBY->baseRowID;

            if ( ( sItrAttr->flag & QMC_ATTR_SORT_ORDER_MASK )
                    == QMC_ATTR_SORT_ORDER_ASC )
            {
                sMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
            }
            else
            {
                sMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sMtrNode->flag |= QMC_MTR_SORT_DESCENDING;
            }

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sMtrNode;
                sLastMtrNode        = sMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sMtrNode;
                sLastMtrNode        = sMtrNode;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    aCNBY->sortNode = sFirstMtrNode;

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------

    // BUG-36472 (table tuple->intermediate tuple)
    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sSortID ,
                                           sColumnCount )
              != IDE_SUCCESS);

    sMtcTemplate->rows[sSortID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sSortID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sSortID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sSortID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    aCNBY->sortRowID = sSortID;
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement,
                                         aCNBY->sortNode )
              != IDE_SUCCESS);

    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    aCNBY->sortNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Pseduo Column의 Tuple ID 세팅.
 *
 */
IDE_RC qmoOneMtrPlan::setPseudoColumnRowID( qtcNode * aNode, UShort * aRowID )
{
    qtcNode * sNode = aNode;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::setPseudoColumnRowID::__FT__" );

    if ( sNode != NULL )
    {
        /*
         * BUG-17949
         * Group By Level 인 경우 SFWGH->level 에 passNode가 달려있다.
         */
        if ( sNode->node.module == &qtc::passModule )
        {
            sNode = ( qtcNode * )sNode->node.arguments;
        }
        else
        {
            /* Nothing to do */
        }

        *aRowID = sNode->node.table;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;
}

/**
 * Make CNBY Plan
 *
 *  Hierarchy 쿼리에서 사용될 CNBY Plan을 생성한다.
 *  이 Plan에서 StartWith와 ConnectBy와 Order Siblings By 구문을 모두 처리한다.
 *
 *  현제 CNBY Plan은 View가 CMTR로 Materialize된 Data를 가지고 Hierarhcy를 처리한다.
 *
 *  한개의 Table일 경우도 이를 View로 Transformaion 해서 처리한다.
 *  이는 qmvHierTransform에서 처리한다.
 *        select * from t1 connect by prior id = pid;
 *    --> select * from (select * from t1 ) t1 connect by prior id = pid;
 */
IDE_RC qmoOneMtrPlan::initCNBY( qcStatement    * aStatement,
                                qmsQuerySet    * aQuerySet,
                                qmoLeafInfo    * aLeafInfo,
                                qmsSortColumns * aSiblings,
                                qmnPlan        * aParent,
                                qmnPlan       ** aPlan )
{
    qmncCNBY       * sCNBY;
    UInt             sDataNodeOffset;
    UInt             sFlag = 0;
    qtcNode        * sNode;
    qmsSortColumns * sSibling;
    qmcAttrDesc    * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initCNBY::__FT__" );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncCNBY ),
                                               (void **)& sCNBY )
              != IDE_SUCCESS );

    idlOS::memset( sCNBY, 0x00, ID_SIZEOF(qmncCNBY));
    QMO_INIT_PLAN_NODE( sCNBY,
                        QC_SHARED_TMPLATE( aStatement),
                        QMN_CNBY,
                        qmnCNBY,
                        qmndCNBY,
                        sDataNodeOffset );

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    for ( sSibling  = aSiblings;
          sSibling != NULL;
          sSibling  = sSibling->next )
    {
        if ( aSiblings->isDESC == ID_FALSE )
        {
            sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_ORDER_ASC;
        }
        else
        {
            sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_ORDER_DESC;
        }

        IDE_TEST( qmc::appendAttribute( aStatement,
                                        aQuerySet,
                                        & sCNBY->plan.resultDesc,
                                        sSibling->sortColumn,
                                        sFlag,
                                        QMC_APPEND_EXPRESSION_TRUE,
                                        ID_FALSE )
                  != IDE_SUCCESS );
    }

    // START WITH절 추가
    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    & sCNBY->plan.resultDesc,
                                    aLeafInfo[0].predicate )
              != IDE_SUCCESS );

    // CONNECT BY절 추가
    // PROJ-2469 Optimize View Materialization
    // BUG FIX : aLeafInfo[0] -> aLeafInfo[1]로 변경
    //                           CONNECT BY 의 Predicate을 등록하지 않고 START WITH만 두 번
    //                           등록하고 있었음.
    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    & sCNBY->plan.resultDesc,
                                    aLeafInfo[1].predicate )
              != IDE_SUCCESS );

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   & sCNBY->plan.resultDesc )
              != IDE_SUCCESS );

    for ( sItrAttr  = sCNBY->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( ( sItrAttr->expr->lflag & QTC_NODE_LEVEL_MASK ) == QTC_NODE_LEVEL_EXIST ) ||
             ( ( sItrAttr->expr->lflag & QTC_NODE_ISLEAF_MASK ) == QTC_NODE_ISLEAF_EXIST ) )
        {
            sItrAttr->flag &= ~QMC_ATTR_TERMINAL_MASK;
            sItrAttr->flag |= QMC_ATTR_TERMINAL_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        if ( ( sItrAttr->expr->lflag & QTC_NODE_PRIOR_MASK ) == QTC_NODE_PRIOR_EXIST )
        {
            sItrAttr->flag &= ~QMC_ATTR_TERMINAL_MASK;
            sItrAttr->flag |= QMC_ATTR_TERMINAL_TRUE;

            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                       (void**)&sNode )
                      != IDE_SUCCESS );

            idlOS::memcpy( sNode, sItrAttr->expr, ID_SIZEOF( qtcNode ) );

            sNode->lflag &= ~QTC_NODE_PRIOR_MASK;
            sNode->lflag |= QTC_NODE_PRIOR_ABSENT;

            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sCNBY->plan.resultDesc,
                                            sNode,
                                            0,
                                            0,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    *aPlan = (qmnPlan *)sCNBY;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneMtrPlan::makeCNBY( qcStatement    * aStatement,
                                qmsQuerySet    * aQuerySet,
                                qmsFrom        * aFrom,
                                qmoLeafInfo    * aStartWith,
                                qmoLeafInfo    * aConnectBy,
                                qmnPlan        * aChildPlan,
                                qmnPlan        * aPlan )
{
    mtcTemplate  * sMtcTemplate    = NULL;
    qmncCNBY     * sCNBY = (qmncCNBY *)aPlan;
    UShort         sPriorID        = 0;
    UShort         sChildTupleID   = 0;
    UShort         sTupleID        = 0;
    UInt           sDataNodeOffset = 0;
    UInt           i               = 0;
    qtcNode      * sPredicate[9];

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeCNBY::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aFrom      != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8(aPlan->offset + ID_SIZEOF(qmndCNBY));

    sCNBY->plan.left     = aChildPlan;
    sCNBY->myRowID       = aFrom->tableRef->table;
    sTupleID             = sCNBY->myRowID;
    sCNBY->mtrNodeOffset = sDataNodeOffset;

    sCNBY->flag             = QMN_PLAN_FLAG_CLEAR;
    sCNBY->plan.flag        = QMN_PLAN_FLAG_CLEAR;
    sCNBY->plan.flag       |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //loop을 찾을 것인지에 대한 결정
    if( (aQuerySet->SFWGH->hierarchy->flag & QMS_HIERARCHY_IGNORE_LOOP_MASK) ==
         QMS_HIERARCHY_IGNORE_LOOP_TRUE )
    {
        sCNBY->flag &= ~QMNC_CNBY_IGNORE_LOOP_MASK;
        sCNBY->flag |= QMNC_CNBY_IGNORE_LOOP_TRUE;
    }
    else
    {
        sCNBY->flag &= ~QMNC_CNBY_IGNORE_LOOP_MASK;
        sCNBY->flag |= QMNC_CNBY_IGNORE_LOOP_FALSE;
    }

    sCNBY->flag &= ~QMNC_CNBY_CHILD_VIEW_MASK;
    sCNBY->flag |= QMNC_CNBY_CHILD_VIEW_FALSE;

    if ( aFrom->tableRef->view != NULL )
    {
        sCNBY->flag &= ~QMNC_CNBY_CHILD_VIEW_MASK;
        sCNBY->flag |= QMNC_CNBY_CHILD_VIEW_TRUE;

        sChildTupleID = ((qmncCMTR *)aChildPlan)->myNode->dstNode->node.table;
        sCNBY->baseRowID = sChildTupleID;
        sCNBY->mIndex     = NULL;

        // Tuple.flag의 세팅
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_TYPE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_TYPE_TABLE;

        // To Fix PR-8385
        // VSCN이 생성되는 경우에는 in-line view라 하더라도
        // 일반테이블로 처리하여야 한다. 따라서 하위에 view라고 세팅된것을
        // false로 설정한다.
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_VIEW_MASK;
        sMtcTemplate->rows[sTupleID].lflag |=  MTC_TUPLE_VIEW_FALSE;

        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;

        sMtcTemplate->rows[sTupleID].lflag
            |=  (sMtcTemplate->rows[sChildTupleID].lflag & MTC_TUPLE_STORAGE_MASK);

        if ( ( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
             &&
             ( ( sMtcTemplate->rows[sTupleID].lflag & MTC_TUPLE_STORAGE_MASK )
               == MTC_TUPLE_STORAGE_DISK ) )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_RID;
        }
        IDE_TEST( qtc::nextTable( & sPriorID,
                                  aStatement,
                                  aFrom->tableRef->tableInfo,
                                  ID_FALSE,
                                  MTC_COLUMN_NOTNULL_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        sCNBY->baseRowID    = aFrom->tableRef->table;
        sCNBY->mIndex       = aConnectBy->index;
        sCNBY->mTableHandle = aFrom->tableRef->tableInfo->tableHandle;
        sCNBY->mTableSCN    = aFrom->tableRef->tableSCN;

        if ( ( ( aFrom->tableRef->tableInfo->tableOwnerID == QC_SYSTEM_USER_ID ) &&
               ( idlOS::strMatch( aFrom->tableRef->tableInfo->name,
                                  idlOS::strlen(aFrom->tableRef->tableInfo->name),
                                  "SYS_DUMMY_",
                                  10 ) == 0 ) ) ||
             ( idlOS::strMatch( aFrom->tableRef->tableInfo->name,
                                idlOS::strlen(aFrom->tableRef->tableInfo->name),
                                "X$DUAL",
                                6 ) == 0 ) )
        {
            sCNBY->flag &= ~QMNC_CNBY_FROM_DUAL_MASK;
            sCNBY->flag |= QMNC_CNBY_FROM_DUAL_TRUE;
        }
        else
        {
            sCNBY->flag &= ~QMNC_CNBY_FROM_DUAL_MASK;
            sCNBY->flag |= QMNC_CNBY_FROM_DUAL_FALSE;
        }
        IDE_TEST( qtc::nextTable( & sPriorID,
                                  aStatement,
                                  aFrom->tableRef->tableInfo,
                                  smiIsDiskTable(sCNBY->mTableHandle),
                                  MTC_COLUMN_NOTNULL_TRUE )
                  != IDE_SUCCESS );
    }

    /* BUG-44382 clone tuple 성능개선 */
    // 복사가 필요함
    qtc::setTupleColumnFlag( &(sMtcTemplate->rows[sPriorID]),
                             ID_TRUE,
                             ID_FALSE );

    aQuerySet->SFWGH->hierarchy->originalTable = sCNBY->myRowID;
    aQuerySet->SFWGH->hierarchy->priorTable    = sPriorID;
    sCNBY->priorRowID                          = sPriorID;

    /* BUG-48300 */
    if ( aQuerySet->SFWGH->level != NULL )
    {
        IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->level,
                                        & sCNBY->levelRowID )
                  != IDE_SUCCESS );
        sCNBY->flag &= ~QMNC_CNBY_LEVEL_MASK;
        sCNBY->flag |= QMNC_CNBY_LEVEL_TRUE;
    }

    /* BUG-48300 */
    if ( aQuerySet->SFWGH->isLeaf != NULL )
    {
        IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->isLeaf,
                                        & sCNBY->isLeafRowID )
                  != IDE_SUCCESS );
        sCNBY->flag &= ~QMNC_CNBY_ISLEAF_MASK;
        sCNBY->flag |= QMNC_CNBY_ISLEAF_TRUE;
    }
    /* BUG-48300 */
    if ( aQuerySet->SFWGH->cnbyStackAddr != NULL )
    {
        IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->cnbyStackAddr,
                                        & sCNBY->stackRowID )
                  != IDE_SUCCESS );
        sCNBY->flag &= ~QMNC_CNBY_FUNCTION_MASK;
        sCNBY->flag |= QMNC_CNBY_FUNCTION_TRUE;
    }

    IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->rownum,
                                    & sCNBY->rownumRowID )
              != IDE_SUCCESS );

    if ( aFrom->tableRef->view != NULL )
    {
        sMtcTemplate->rows[sPriorID].lflag &= ~MTC_TUPLE_ROW_ALLOCATE_MASK;
        sMtcTemplate->rows[sPriorID].lflag |= MTC_TUPLE_ROW_ALLOCATE_FALSE;
    }
    else
    {
        idlOS::memcpy( &QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPriorID],
                       &QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sCNBY->myRowID],
                       ID_SIZEOF(mtcTuple) );
    }

    /* PROJ-1473 */
    IDE_TEST( qtc::allocAndInitColumnLocateInTuple( QC_QMP_MEM( aStatement ),
                                                    &( QC_SHARED_TMPLATE( aStatement )->tmplate ),
                                                    sPriorID )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sCNBY->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     &( aQuerySet->depInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( processStartWithPredicate( aStatement,
                                         aQuerySet,
                                         sCNBY,
                                         aStartWith )
              != IDE_SUCCESS );
    IDE_TEST( processConnectByPredicate( aStatement,
                                         aQuerySet,
                                         aFrom,
                                         sCNBY,
                                         aConnectBy )
              != IDE_SUCCESS );


    /* 마무리 작업1 */
    sPredicate[0] = sCNBY->startWithConstant;
    sPredicate[1] = sCNBY->startWithFilter;
    sPredicate[2] = sCNBY->startWithSubquery;
    sPredicate[3] = sCNBY->startWithNNF;
    sPredicate[4] = sCNBY->connectByConstant;
    sPredicate[5] = sCNBY->connectByFilter;
    /* fix BUG-26770
     * connect by LEVEL+to_date(:D1,'YYYYMMDD') <= to_date(:D2,'YYYYMMDD')+1;
     * 질의수행시 서버비정상종료
     * level filter에 대한 처리가 없어,
     * level filter에 포한된 host변수를 등록하지 못해 bindParamInfo 설정시, 비정상종료함.
     */
    sPredicate[6] = sCNBY->levelFilter;

    /* BUG-39041 The connect by clause is not support subquery. */
    sPredicate[7] = sCNBY->connectBySubquery;

    /* BUG-39434 The connect by need rownum pseudo column. */
    sPredicate[8] = sCNBY->rownumFilter;

    for ( i = 0;
          i < 9;
          i++ )
    {
        IDE_TEST( qmg::changeColumnLocate( aStatement,
                                           aQuerySet,
                                           sPredicate[i],
                                           ID_USHORT_MAX,
                                           ID_TRUE )
                  != IDE_SUCCESS );
    }

    /* dependency 처리 및 subquery의 처리 */
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sCNBY->plan ,
                                            QMO_CNBY_DEPENDENCY,
                                            sCNBY->myRowID ,
                                            NULL,
                                            9,
                                            sPredicate,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sCNBY->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sCNBY->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sCNBY->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneMtrPlan::initCMTR( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet,
                                qmnPlan     ** aPlan )
{
    qmncCMTR          * sCMTR;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initCMTR::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncCMTR의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncCMTR ),
                                               (void **)& sCMTR )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sCMTR ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_CMTR ,
                        qmnCMTR ,
                        qmndCMTR,
                        sDataNodeOffset );

    IDE_TEST( qmc::createResultFromQuerySet( aStatement,
                                             aQuerySet,
                                             & sCMTR->plan.resultDesc )
              != IDE_SUCCESS );

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sCMTR->planID,
                                         ID_FALSE,
                                         ID_TRUE )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sCMTR;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * make CMTR
 *
 *  CNBY Plan을 상위로 두고 하위 노드를 QMN_VIEW만 지원하 Plan노드를 생성한다.
 *  View 에 해당하는 모든 데이터를 이 CMTR에 쌓아놓고 CNBY가 처리한다.
 *
 *  CMTR은 Memory Temp Table만 지원한다.
 */
IDE_RC qmoOneMtrPlan::makeCMTR( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet,
                                UInt           aFlag,
                                qmnPlan      * aChildPlan,
                                qmnPlan      * aPlan )
{
    qmncCMTR        * sCMTR = (qmncCMTR *)aPlan;
    qmcMtrNode      * sNewMtrNode       = NULL;
    qmcMtrNode      * sLastMtrNode      = NULL;
    mtcTemplate     * sMtcTemplate      = NULL;
    UInt              sDataNodeOffset   = 0;
    UShort            sTupleID          = 0;
    UShort            sColumnCount      = 0;
    qmcMtrNode      * sMtrNode[2];
    qmcAttrDesc     * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeCMTR::__FT__" );

    sMtcTemplate = &QC_SHARED_TMPLATE(aStatement)->tmplate;

    IDE_TEST_RAISE( ( aFlag & QMO_MAKEVMTR_TEMP_TABLE_MASK )
                    != QMO_MAKEVMTR_MEMORY_TEMP_TABLE,
                    ERR_NOT_MEMORY_TEMP_TABLE);

    IDE_TEST_RAISE( aChildPlan->type != QMN_VIEW, ERR_NOT_VIEW );

    aPlan->offset    = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndCMTR));

    sCMTR->plan.left     = aChildPlan;
    sCMTR->mtrNodeOffset = sDataNodeOffset;

    sCMTR->flag      = QMN_PLAN_FLAG_CLEAR;
    sCMTR->plan.flag = QMN_PLAN_FLAG_CLEAR;
    sCMTR->myNode    = NULL;

    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_FALSE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sCMTR->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     &( aQuerySet->depInfo ) )
              != IDE_SUCCESS );

    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        IDE_TEST( qmg::makeColumnMtrNode( aStatement ,
                                          aQuerySet ,
                                          sItrAttr->expr,
                                          ID_FALSE,
                                          sTupleID ,
                                          0,
                                          & sColumnCount ,
                                          & sNewMtrNode )
                  != IDE_SUCCESS );

        if ( ( sItrAttr->flag & QMC_ATTR_USELESS_RESULT_MASK ) == QMC_ATTR_USELESS_RESULT_TRUE )
        {
            // PROJ-2469 Optimize View Materialization
            // 상위에서 사용되지 않는 MtrNode에 대해서 flag 처리한다.
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |=  QMC_MTR_TYPE_USELESS_COLUMN;    
        }
        else
        {
            //CMTR에서 생성된 qmcMtrNode이므로 복사하도록 한다.
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;
        }

        // PROJ-2179
        sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
        sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE;

        // 상위에서 temp table의 값을 참조하도록 변경된 위치를 설정한다.
        sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
        sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;

        //connect
        if( sCMTR->myNode == NULL )
        {
            sCMTR->myNode = sNewMtrNode;
            sLastMtrNode  = sNewMtrNode;
        }
        else
        {
            sLastMtrNode->next = sNewMtrNode;
            sLastMtrNode       = sNewMtrNode;
        }
    }

    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                           sTupleID,
                                           sColumnCount )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;
    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    sCMTR->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
    sCMTR->plan.flag |= QMN_PLAN_STORAGE_MEMORY;

    IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sCMTR->myNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setColumnLocate( aStatement, sCMTR->myNode )
              != IDE_SUCCESS );

    QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    sMtrNode[0] = sCMTR->myNode;
    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sCMTR->plan,
                                            QMO_CMTR_DEPENDENCY,
                                            sTupleID,
                                            NULL,
                                            0,
                                            NULL,
                                            1,
                                            sMtrNode )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sCMTR->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sCMTR->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sCMTR->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sCMTR->planID,
                               sCMTR->plan.flag,
                               sMtcTemplate->rows[sTupleID].lflag,
                               sCMTR->myNode,
                               &sCMTR->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_MEMORY_TEMP_TABLE )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qmoOneMtrPlan::makeCMTR",
                                "aFlag is not QMO_MAKEVMTR_MEMORY_TEMP_TABLE"));
    }
    IDE_EXCEPTION( ERR_NOT_VIEW )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qmoOneMtrPlan::makeCMTR",
                                "aChildPlan->type != QMN_VIEW"));
    }
    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::makeNonKeyAttrsMtrNodes( qcStatement  * aStatement,
                                        qmsQuerySet  * aQuerySet,
                                        qmcAttrDesc  * aResultDesc,
                                        UShort         aTupleID,
                                        qmcMtrNode  ** aFirstMtrNode,
                                        qmcMtrNode  ** aLastMtrNode,
                                        UShort       * aMtrNodeCount )
{
/***********************************************************************
 *
 * Description :
 *    Result descriptor에서 key가 아닌 attribute들에 대해서만
 *    materialize node를 생성한다.
 *
 * Implementation :
 *
 **********************************************************************/

    mtcTemplate * sMtcTemplate;
    qmcMtrNode  * sNewMtrNode;
    qmcAttrDesc * sItrAttr;
    ULong         sFlag;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeNonKeyAttrsMtrNodes::__FT__" );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    for ( sItrAttr  = aResultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_FALSE ) &&
               ( ( sItrAttr->flag & QMC_ATTR_ANALYTIC_FUNC_MASK )
                 == QMC_ATTR_ANALYTIC_FUNC_FALSE ) ) ||
             ( ( sMtcTemplate->rows[sItrAttr->expr->node.table].lflag &
                 MTC_TUPLE_TARGET_UPDATE_DELETE_MASK ) /* BUG-39399 remove search key preserved table */
               == MTC_TUPLE_TARGET_UPDATE_DELETE_TRUE ) )
        {
            IDE_DASSERT( sItrAttr->expr->node.module != &qtc::passModule );

            IDE_TEST( qmg::changeColumnLocate( aStatement,
                                               aQuerySet,
                                               sItrAttr->expr,
                                               aTupleID,
                                               ID_FALSE )
                      != IDE_SUCCESS );

            sFlag = sMtcTemplate->rows[sItrAttr->expr->node.table].lflag;

            // src, dst가 모두 disk인 경우에만 value로 materialization이 가능하다.
            if( qmg::getMtrMethod( aStatement,
                                   sItrAttr->expr->node.table,
                                   aTupleID ) == ID_TRUE )
            {
                IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                                  aQuerySet,
                                                  sItrAttr->expr,
                                                  ID_FALSE,
                                                  aTupleID,
                                                  0,
                                                  aMtrNodeCount,
                                                  & sNewMtrNode )
                          != IDE_SUCCESS );
            }
            else
            {
                // Surrogate-key(RID 또는 pointer)를 복사한다.

                // 이미 surrogate-key가 추가되어있는지 확인한다.
                if( qmg::existBaseTable( *aFirstMtrNode,
                                         qmg::getBaseTableType( sFlag ),
                                         sItrAttr->expr->node.table )
                        == ID_TRUE )
                {
                    continue;
                }
                else
                {
                    IDE_TEST( qmg::makeBaseTableMtrNode( aStatement,
                                                         sItrAttr->expr,
                                                         aTupleID,
                                                         aMtrNodeCount,
                                                         & sNewMtrNode )
                              != IDE_SUCCESS );
                }
            }

            if( *aFirstMtrNode == NULL )
            {
                *aFirstMtrNode        = sNewMtrNode;
                *aLastMtrNode         = sNewMtrNode;
            }
            else
            {
                (*aLastMtrNode)->next = sNewMtrNode;
                *aLastMtrNode         = sNewMtrNode;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::appendJoinPredicate( qcStatement  * aStatement,
                                    qmsQuerySet  * aQuerySet,
                                    qtcNode      * aJoinPredicate,
                                    idBool         aIsLeft,
                                    idBool         aAllowDup,
                                    qmcAttrDesc ** aResultDesc )
{
/***********************************************************************
 *
 * Description :
 *    Sort/hash join을 위해 join predicate으로부터 key를 찾아
 *    materialize node를 생성한다.
 *
 * Implementation :
 *
 **********************************************************************/
    UInt                sFlag = 0;
    UInt                sAppendOption = 0;
    qtcNode           * sOperatorNode;
    qtcNode           * sNode;
    qtcNode           * sArg;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::appendJoinPredicate::__FT__" );

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    sFlag &= ~QMC_ATTR_CONVERSION_MASK;
    sFlag |= QMC_ATTR_CONVERSION_TRUE;

    sAppendOption &= ~QMC_APPEND_EXPRESSION_MASK;
    sAppendOption |= QMC_APPEND_EXPRESSION_TRUE;

    if( aAllowDup == ID_TRUE )
    {
        sAppendOption &= ~QMC_APPEND_ALLOW_DUP_MASK;
        sAppendOption |= QMC_APPEND_ALLOW_DUP_TRUE;
    }
    else
    {
        sAppendOption &= ~QMC_APPEND_ALLOW_DUP_MASK;
        sAppendOption |= QMC_APPEND_ALLOW_DUP_FALSE;
    }

    // aJoinPredicate는 DNF로 구성되어있으므로 각각의 AND node들을 순회한다.
    for( sOperatorNode = (qtcNode *)aJoinPredicate->node.arguments->arguments;
         sOperatorNode != NULL;
         sOperatorNode = (qtcNode *)sOperatorNode->node.next )
    {
        sNode = (qtcNode*)sOperatorNode->node.arguments;

        if( aIsLeft == ID_FALSE )
        {
            //----------------------------------
            // indexArgument에 해당하는 컬럼을 저장할 경우
            //----------------------------------

            if( sOperatorNode->indexArgument == 0 )
            {
                // Nothing to do.
            }
            else
            {
                sNode = (qtcNode*)sNode->node.next;
            }
        }
        else
        {
            //----------------------------------
            // indexArgument에 해당하지 않는 컬럼을 저장할 경우
            //----------------------------------

            if ( sOperatorNode->indexArgument == 0 )
            {
                sNode = (qtcNode*)sNode->node.next;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sNode->node.module == &mtfList )
        {
            for ( sArg  = (qtcNode *)sNode->node.arguments;
                  sArg != NULL;
                  sArg  = (qtcNode *)sArg->node.next )
            {
                IDE_TEST( appendJoinColumn( aStatement,
                                            aQuerySet,
                                            sArg,
                                            sFlag,
                                            sAppendOption,
                                            aResultDesc )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            IDE_TEST( appendJoinColumn( aStatement,
                                        aQuerySet,
                                        sNode,
                                        sFlag,
                                        sAppendOption,
                                        aResultDesc )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneMtrPlan::appendJoinColumn( qcStatement  * aStatement,
                                 qmsQuerySet  * aQuerySet,
                                 qtcNode      * aColumnNode,
                                 UInt           aFlag,
                                 UInt           aAppendOption,
                                 qmcAttrDesc ** aResultDesc )
{
    qtcNode * sCopiedNode;
    qtcNode * sPassNode;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::appendJoinColumn::__FT__" );

    if ( ( QMC_NEED_CALC( aColumnNode ) == ID_TRUE ) ||
         ( ( mtf::convertedNode( &aColumnNode->node , NULL) != &aColumnNode->node ) ) )
    {
        IDE_DASSERT( aColumnNode->node.module != &qtc::passModule );

        // Join predicate에 사용된 expression의 경우
        // retrieve 시 expression이 재수행되지 않도록 pass node를 설정한다.

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                   (void **)& sCopiedNode )
                  != IDE_SUCCESS );
        idlOS::memcpy( sCopiedNode, aColumnNode, ID_SIZEOF( qtcNode ) );

        IDE_TEST( qtc::makePassNode( aStatement ,
                                     NULL ,
                                     sCopiedNode ,
                                     & sPassNode )
                  != IDE_SUCCESS );

        sPassNode->node.next = sCopiedNode->node.next;
        idlOS::memcpy( aColumnNode, sPassNode, ID_SIZEOF(qtcNode) );

        aColumnNode = sCopiedNode;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmc::appendAttribute( aStatement,
                                    aQuerySet,
                                    aResultDesc,
                                    aColumnNode,
                                    aFlag,
                                    aAppendOption,
                                    ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1353 makeMemoryValeTemp
 *
 *  Rollup, Cube Plan는 기존 Row를 상위 Plan에 올려 주는 것이 아니라 기존 Row가 Group에
 *  따라 NULL이 된 컬럼이 존재하는 새로운 Row라 할 수 있다.
 *  이때 Memory Table일 경우에는 항상 Pointer 형으로 값을 쌓게 되는 데 ORDER BY 구문일 경우
 *  상위에 SORT PLAN이 생성된다. 이 SORT Plan의 하위에 Rollup 이나 Cube이 존재 하면 메모리
 *  일 경우 Pointer를 쌓으려 할텐데 Rollup이나 Cube는 상위 Plan에서 참조하는 하나의 Row만
 *  존재 하게 될 경우 이 makeValueTemp 함수를 통해 STORE용 Sort Temp를 생성하고
 *  Rollup이나 Cube에서 생성된 ROW를 새로 추가해서 상위에서 이 Row Pointer를 참조 할 수
 *  있도록 해준다.
 */
IDE_RC qmoOneMtrPlan::makeValueTempMtrNode( qcStatement * aStatement,
                                            qmsQuerySet * aQuerySet,
                                            UInt          aFlag,
                                            qmnPlan     * aPlan,
                                            qmcMtrNode ** aFirstNode,
                                            UShort      * aColumnCount )
{
    mtcTemplate * sMtcTemplate = NULL;
    qmcAttrDesc * sItrAttr = NULL;
    qmcMtrNode  * sFirstMtrNode = NULL;
    qmcMtrNode  * sLastMtrNode = NULL;
    qmcMtrNode  * sNewMtrNode = NULL;

    UShort        sTupleID = 0;
    UShort        sCount = 0;
    UShort        sColumnCount = 0;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeValueTempMtrNode::__FT__" );

    sMtcTemplate = &QC_SHARED_TMPLATE( aStatement )->tmplate;

    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    if ( (aFlag & QMO_MAKESORT_TEMP_TABLE_MASK) ==
         QMO_MAKESORT_MEMORY_TEMP_TABLE )
    {
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_DISK;

        if ( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // disk temp를 사용하는 경우 TEMP_VAR_TYPE를 사용하지 않는다.
            if ( ( aPlan->flag & QMN_PLAN_STORAGE_MASK )
                 == QMN_PLAN_STORAGE_DISK )
            {
                sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
                sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;
            }
            else
            {
                // Nothing to do.
            }

            // 상위에서 temp table의 값을 참조하도록 변경된 위치를 설정한다.
            sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
            sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode = sNewMtrNode;
                sLastMtrNode  = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next = sNewMtrNode;
                sLastMtrNode       = sNewMtrNode;
            }
            sCount++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    for ( sItrAttr = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next )
    {
        if ( ( ( sItrAttr->flag & QMC_ATTR_GROUP_BY_EXT_MASK )
               == QMC_ATTR_GROUP_BY_EXT_TRUE ) &&
             ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK )
               == QMC_ATTR_KEY_FALSE ) )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            // BUG-36472
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // disk temp를 사용하는 경우 TEMP_VAR_TYPE를 사용하지 않는다.
            if ( ( aPlan->flag & QMN_PLAN_STORAGE_MASK )
                 == QMN_PLAN_STORAGE_DISK )
            {
                sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
                sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;
            }
            else
            {
                // Nothing to do.
            }

            // 상위에서 temp table의 값을 참조하도록 변경된 위치를 설정한다.
            sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
            sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode = sNewMtrNode;
                sLastMtrNode  = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next = sNewMtrNode;
                sLastMtrNode       = sNewMtrNode;
            }
            sCount++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aFirstNode = sFirstMtrNode;
    *aColumnCount = sCount;

    if ( sFirstMtrNode != NULL )
    {
        IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                               sMtcTemplate,
                                               sTupleID,
                                               sColumnCount )
                  != IDE_SUCCESS );

        /* BUG-36015 distinct sort temp problem */
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

        IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sFirstMtrNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmg::setColumnLocate( aStatement, sFirstMtrNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmg::setCalcLocate( aStatement, sFirstMtrNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1353 initROLL
 *
 *  Rollup에 Result Descript를 추가한다.
 *  GROUP BY 컬럼을 먼저 추가하고 Rollup 컬럼을 추가해서 하위 Plan에서 이 순서로 sort 될 수
 *   있도록 한다.
 *  Aggregation 을 추가한다.
 */
IDE_RC qmoOneMtrPlan::initROLL( qcStatement      * aStatement,
                                qmsQuerySet      * aQuerySet,
                                UInt             * aRollupCount,
                                qmsAggNode       * aAggrNode,
                                qmsConcatElement * aGroupColumn,
                                qmnPlan         ** aPlan )
{
    qmncROLL         * sROLL           = NULL;
    qmsConcatElement * sGroup          = NULL;
    qmsConcatElement * sSubGroup       = NULL;
    qmsAggNode       * sAggrNode       = NULL;
    qtcNode          * sNode           = NULL;
    qtcNode          * sListNode       = NULL;
    UInt               sDataNodeOffset = 0;
    UInt               sFlag           = 0;
    UInt               sRollupCount    = 0;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initROLL::__FT__" );

    IDE_DASSERT( aStatement != NULL );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncROLL ),
                                               (void **)& sROLL )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sROLL,
                        QC_SHARED_TMPLATE( aStatement),
                        QMN_ROLL,
                        qmnROLL,
                        qmndROLL,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sROLL;

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
    sFlag |= QMC_ATTR_SORT_ORDER_ASC;

    for ( sGroup  = aGroupColumn;
          sGroup != NULL;
          sGroup  = sGroup->next )
    {
        if ( sGroup->type == QMS_GROUPBY_NORMAL )
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sROLL->plan.resultDesc,
                                            sGroup->arithmeticOrList,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE |
                                            QMC_APPEND_ALLOW_CONST_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    sFlag &= ~QMC_ATTR_GROUP_BY_EXT_MASK;
    sFlag |= QMC_ATTR_GROUP_BY_EXT_TRUE;
    for ( sGroup  = aGroupColumn;
          sGroup != NULL;
          sGroup  = sGroup->next )
    {
        if ( sGroup->type != QMS_GROUPBY_NORMAL )
        {
            for ( sSubGroup = sGroup->arguments;
                  sSubGroup != NULL;
                  sSubGroup = sSubGroup->next )
            {
                sSubGroup->type = sGroup->type;
                if ( ( sSubGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    for ( sListNode  = (qtcNode *)sSubGroup->arithmeticOrList->node.arguments;
                          sListNode != NULL;
                          sListNode  = (qtcNode *)sListNode->node.next )
                    {
                        IDE_TEST( qmc::appendAttribute( aStatement,
                                                        aQuerySet,
                                                        & sROLL->plan.resultDesc,
                                                        sListNode,
                                                        sFlag,
                                                        QMC_APPEND_EXPRESSION_TRUE |
                                                        QMC_APPEND_ALLOW_DUP_TRUE |
                                                        QMC_APPEND_ALLOW_CONST_TRUE,
                                                        ID_FALSE )
                                  != IDE_SUCCESS );
                    }
                }
                else
                {
                    IDE_TEST( qmc::appendAttribute( aStatement,
                                                    aQuerySet,
                                                    & sROLL->plan.resultDesc,
                                                    sSubGroup->arithmeticOrList,
                                                    sFlag,
                                                    QMC_APPEND_EXPRESSION_TRUE |
                                                    QMC_APPEND_ALLOW_DUP_TRUE |
                                                    QMC_APPEND_ALLOW_CONST_TRUE,
                                                    ID_FALSE )
                              != IDE_SUCCESS );
                }
                ++sRollupCount;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    sFlag = 0;
    sFlag &= ~QMC_ATTR_GROUP_BY_EXT_MASK;
    sFlag |= QMC_ATTR_GROUP_BY_EXT_TRUE;

    for ( sAggrNode  = aAggrNode;
          sAggrNode != NULL;
          sAggrNode  = sAggrNode->next )
    {
        sNode = sAggrNode->aggr;
        while( sNode->node.module == &qtc::passModule )
        {
            sNode = ( qtcNode *)sNode->node.arguments;
        }

        /* BUG-35193  Window function 이 아닌 aggregation 만 처리해야 한다. */
        if( ( QTC_IS_AGGREGATE( sNode ) == ID_TRUE ) &&
            ( sNode->overClause == NULL ) )
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sROLL->plan.resultDesc,
                                            sNode,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE |
                                            QMC_APPEND_ALLOW_DUP_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sROLL->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    *aRollupCount = sRollupCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1353 makeROLL
 *
 *    qmnROLL Plan을 생성한다.
 *    Rollup ( 컬럼 ) 은 ( n + 1 )개의 그룹으로 이루어져 있다.
 *    Sort된 자료들을 한번 읽고 비교하므로써 n+1 개의 Group을 처리한다.
 *
 *  - 하위에 SORT Plan이 오거나 SCAN이 올 수 있다.
 *  - ROLL Plan은 5개의 Tuple과 mtr Node가 존재 할 수 있다.
 *  - mtrNode  - 하위 Plan에서 자료를 읽어 접근하기 위한 NODE
 *  - myNode   - 상위 PLAN에서 보는 NODE이다.
 *  - aggrNode - aggregation MTR NODE
 *  - distNode - aggregation에 distinct가 지정될 때의 NODE
 *  - ValueTempNode - value store용 TEMP NODE
 */
IDE_RC qmoOneMtrPlan::makeROLL( qcStatement      * aStatement,
                                qmsQuerySet      * aQuerySet,
                                UInt               aFlag,
                                UInt               aRollupCount,
                                qmsAggNode       * aAggrNode,
                                qmoDistAggArg    * aDistAggArg,
                                qmsConcatElement * aGroupColumn,
                                qmnPlan          * aChildPlan,
                                qmnPlan          * aPlan )
{
    mtcTemplate      * sMtcTemplate    = NULL;
    qmncROLL         * sROLL           = NULL;
    qmcAttrDesc      * sItrAttr        = NULL;
    qmcMtrNode       * sNewMtrNode     = NULL;
    qmcMtrNode       * sFirstMtrNode   = NULL;
    qmcMtrNode       * sLastMtrNode    = NULL;
    qmsAggNode       * sAggrNode       = NULL;
    qmsConcatElement * sGroupBy;
    qmsConcatElement * sSubGroup;
    qmcMtrNode       * sMtrNode[3];
    qmcMtrNode         sDummyNode;
    qtcNode          * sNode;

    UChar       ** sGroup;
    UChar        * sTemp;
    UShort         sTupleID        = 0;
    UShort         sColumnCount    = 0;
    UShort         sAggrNodeCount  = 0;
    UShort         sDistNodeCount  = 0;
    UShort         sCount          = 0;
    UShort         sMemValueCount  = 0;
    UInt           sDataNodeOffset = 0;
    UInt           i;
    SInt           j;
    UInt           k;
    SInt           sRollupStart    = -1;
    idBool         sIsPartial      = ID_FALSE;
    UInt           sTempTuple      = 0;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeROLL::__FT__" );

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aGroupColumn != NULL );
    IDE_DASSERT( aChildPlan   != NULL );
    IDE_DASSERT( aPlan        != NULL );

    sMtcTemplate    = &QC_SHARED_TMPLATE(aStatement)->tmplate;
    aPlan->offset   = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset + ID_SIZEOF( qmndROLL ));

    sROLL            = ( qmncROLL * )aPlan;
    sROLL->plan.left = aChildPlan;
    sROLL->flag      = QMN_PLAN_FLAG_CLEAR;
    sROLL->plan.flag = QMN_PLAN_FLAG_CLEAR;
    sROLL->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->groupingInfoAddr,
                                    & sROLL->groupingFuncRowID )
              != IDE_SUCCESS );

    if ( ( aFlag & QMO_MAKESORT_PRESERVED_ORDER_MASK )
         == QMO_MAKESORT_PRESERVED_TRUE )
    {
        sROLL->preservedOrderMode = ID_TRUE;
    }
    else
    {
        sROLL->preservedOrderMode = ID_FALSE;
    }

    for ( sAggrNode  = aAggrNode;
          sAggrNode != NULL;
          sAggrNode  = sAggrNode->next )
    {
        sAggrNodeCount++;
        sNode = sAggrNode->aggr;
        if ( sNode->node.arguments != NULL )
        {
            IDE_TEST_RAISE ( ( sNode->node.arguments->lflag & MTC_NODE_OPERATOR_MASK )
                             == MTC_NODE_OPERATOR_SUBQUERY,
                             ERR_NOT_ALLOW_SUBQUERY );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* make mtrNode */
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    sTempTuple = sTupleID;
    if ( ( aFlag & QMO_MAKESORT_TEMP_TABLE_MASK ) ==
         QMO_MAKESORT_MEMORY_TEMP_TABLE )
    {
        sROLL->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sROLL->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

        /* PROJ-2462 Result Cache */
        if ( ( QC_SHARED_TMPLATE( aStatement )->resultCache.flag & QC_RESULT_CACHE_MASK )
             == QC_RESULT_CACHE_ENABLE )
        {
            if ( aQuerySet->SFWGH->hints->resultCacheType
                 != QMO_RESULT_CACHE_NO )
            {
                // Result Cache에서는 항상 Value Temp를 생성하도록 유도한다.
                aFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
                aFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( aQuerySet->SFWGH->hints->resultCacheType
                 == QMO_RESULT_CACHE )
            {
                // Result Cache에서는 항상 Value Temp를 생성하도록 유도한다.
                aFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
                aFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        sROLL->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sROLL->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            if ( ( sItrAttr->flag & QMC_ATTR_GROUP_BY_EXT_MASK )
                 == QMC_ATTR_GROUP_BY_EXT_TRUE )
            {
                if ( sRollupStart == -1 )
                {
                    sRollupStart = sCount;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                sIsPartial = ID_TRUE;
            }

            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
            sNewMtrNode->flag &= ~QMC_MTR_GROUPING_MASK;
            sNewMtrNode->flag |= QMC_MTR_GROUPING_TRUE;
            sNewMtrNode->flag &= ~QMC_MTR_MTR_PLAN_MASK;
            sNewMtrNode->flag |= QMC_MTR_MTR_PLAN_TRUE;

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode = sNewMtrNode;
                sLastMtrNode  = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next = sNewMtrNode;
                sLastMtrNode       = sNewMtrNode;
            }
            sCount++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sDummyNode.next    = NULL;
    sDummyNode.srcNode = NULL;
    sNewMtrNode        = &sDummyNode;
    for ( sAggrNode  = aAggrNode;
          sAggrNode != NULL;
          sAggrNode  = sAggrNode->next )
    {
        sNode = sAggrNode->aggr;
        while( sNode->node.module == &qtc::passModule )
        {
            sNode = (qtcNode *)sNode->node.arguments;
        }

        if ( ( sNode->node.lflag & MTC_NODE_FUNCTON_GROUPING_MASK )
             == MTC_NODE_FUNCTON_GROUPING_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        /* Aggregate function이 아닌 node가 전달되는 경우가 존재한다.*/
        /* BUG-35193  Window function 이 아닌 aggregation 만 처리해야 한다. */
        if ( ( QTC_IS_AGGREGATE( sNode ) == ID_TRUE ) &&
             ( sNode->overClause == NULL ) )
        {
            if ( sNode->node.arguments != NULL )
            {
                IDE_TEST( makeAggrArgumentsMtrNode( aStatement,
                                                    aQuerySet,
                                                    sTupleID,
                                                    & sColumnCount,
                                                    sNode->node.arguments,
                                                    & sNewMtrNode )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    if ( sDummyNode.next != NULL )
    {
        sLastMtrNode->next = sDummyNode.next;
    }
    else
    {
        /* Nothing to do */
    }

    sROLL->mtrNode      = sFirstMtrNode;
    sROLL->mtrNodeCount = sColumnCount;

    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           sMtcTemplate,
                                           sTupleID,
                                           sColumnCount )
              != IDE_SUCCESS );

    if ( ( aFlag & QMO_MAKESORT_TEMP_TABLE_MASK ) ==
         QMO_MAKESORT_MEMORY_TEMP_TABLE )
    {
        sROLL->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sROLL->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sROLL->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sROLL->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if ( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sROLL->mtrNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setColumnLocate( aStatement, sROLL->mtrNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setCalcLocate( aStatement, sROLL->mtrNode )
              != IDE_SUCCESS );

    /* Parital Rollup 인 경우 Rollup이 컬럼의 위치를 지정한다. */
    if ( sIsPartial == ID_TRUE )
    {
        sROLL->partialRollup = sRollupStart;
    }
    else
    {
        sROLL->partialRollup = -1;
    }

    sROLL->groupCount = aRollupCount + 1;

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ( ID_SIZEOF( UShort ) * sROLL->groupCount ),
                                               (void**) & sROLL->elementCount )
              != IDE_SUCCESS );

    sROLL->elementCount[aRollupCount] = 0;

    for ( sGroupBy  = aGroupColumn;
          sGroupBy != NULL;
          sGroupBy  = sGroupBy->next )
    {
        if ( sGroupBy->type == QMS_GROUPBY_ROLLUP )
        {
            for ( sSubGroup = sGroupBy->arguments, i = 0;
                  sSubGroup != NULL;
                  sSubGroup = sSubGroup->next, i++ )
            {
                if ( ( sSubGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    sROLL->elementCount[i] = sSubGroup->arithmeticOrList->node.lflag &
                        MTC_NODE_ARGUMENT_COUNT_MASK;
                }
                else
                {
                    sROLL->elementCount[i] = 1;
                }
            }
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    /**
     * Rollup에서 사용되는 Group의 컬럼의 NULL여부를 표시한다.
     * 만약 Rollup에서 사용된 컬럼이 5 개라면 다음과 같이 표시된다.
     * ([0] 11111
     *  [1] 11110
     *  [2] 11100
     *  [3] 11000
     *  [4] 10000
     *  [5] 00000 )
     *  Rollup은 n+1개의 그룹이 생성되므로 6개의 배열을 가지게 된다.
     */
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( (ID_SIZEOF(UChar*) *
                                              (aRollupCount + 1)),
                                             (void**) &sGroup )
              != IDE_SUCCESS);
    sROLL->mtrGroup = sGroup;
    for ( i = 0; i < aRollupCount + 1; i++ )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->cralloc( (ID_SIZEOF(UChar) *
                                                    sROLL->mtrNodeCount ),
                                                   (void**) &sTemp )
                  != IDE_SUCCESS);
        sROLL->mtrGroup[i] = sTemp;

        for ( j = 0; j < sRollupStart; j++ )
        {
            sROLL->mtrGroup[i][j] = 0x1;
        }

        for ( k = 0; k < aRollupCount; k++ )
        {
            if ( k >= aRollupCount - i )
            {
                break;
            }
            else
            {
                for ( sCount = 0;
                      sCount < sROLL->elementCount[k];
                      sCount++ )
                {
                    sROLL->mtrGroup[i][j] = 0x1;
                    j++;
                }
            }
        }
    }

    /* make myNode */
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

    sFirstMtrNode = NULL;
    sCount        = 0;
    sColumnCount  = 0;
    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );
            if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK )
                 == QMO_MAKESORT_VALUE_TEMP_FALSE )
            {
                // 상위에서 이 table의 값을 참조하도록 변경된 위치를 설정한다.
                sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
                sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;
            }
            else
            {
                /* Nothing to do */
            }

            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // myNode는 temp를 만들지 않는다.
            sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode = sNewMtrNode;
                sLastMtrNode  = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next = sNewMtrNode;
                sLastMtrNode       = sNewMtrNode;
            }
            sCount++;
        }
        else
        {
            /* Nothing to do */
        }
    }
    sROLL->myNode      = sFirstMtrNode;
    sROLL->myNodeCount = sCount;

    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           sMtcTemplate,
                                           sTupleID,
                                           sColumnCount )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;
    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sROLL->myNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setColumnLocate( aStatement, sROLL->myNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setCalcLocate( aStatement, sROLL->myNode )
              != IDE_SUCCESS );

    /* AggrMTR Node 구성 */
    sROLL->aggrNodeCount = sAggrNodeCount;

    if ( sAggrNodeCount > 0 )
    {
        IDE_TEST(  qtc::nextTable( & sTupleID,
                                   aStatement,
                                   NULL,
                                   ID_TRUE,
                                   MTC_COLUMN_NOTNULL_TRUE )
                   != IDE_SUCCESS );

        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

        sROLL->distNode = NULL;
        sFirstMtrNode   = NULL;
        sColumnCount    = 0;
        for ( sItrAttr  = aPlan->resultDesc;
              sItrAttr != NULL;
              sItrAttr  = sItrAttr->next )
        {
            if ( ( (sItrAttr->flag & QMC_ATTR_GROUP_BY_EXT_MASK)
                   == QMC_ATTR_GROUP_BY_EXT_TRUE ) &&
                 ( (sItrAttr->flag & QMC_ATTR_KEY_MASK)
                   == QMC_ATTR_KEY_FALSE ) )
            {
                IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                                  aQuerySet,
                                                  sItrAttr->expr,
                                                  ID_FALSE,
                                                  sTupleID,
                                                  0,
                                                  & sColumnCount,
                                                  & sNewMtrNode )
                          != IDE_SUCCESS );

                if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK )
                     == QMO_MAKESORT_VALUE_TEMP_FALSE )
                {
                    // 상위에서 이 table의 값을 참조하도록 변경된 위치를 설정한다.
                    sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
                    sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;
                }
                else
                {
                    /* Nothing to do */
                }
                sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                sNewMtrNode->flag |= QMC_MTR_TYPE_CALCULATE;

                if ( ( sItrAttr->expr->node.lflag & MTC_NODE_DISTINCT_MASK )
                     == MTC_NODE_DISTINCT_FALSE )
                {
                    sNewMtrNode->flag &= ~QMC_MTR_DIST_AGGR_MASK;
                    sNewMtrNode->flag |= QMC_MTR_DIST_AGGR_FALSE;
                    sNewMtrNode->myDist = NULL;
                }
                else
                {
                    sNewMtrNode->flag &= ~QMC_MTR_DIST_AGGR_MASK;
                    sNewMtrNode->flag |= QMC_MTR_DIST_AGGR_TRUE;
                    sNewMtrNode->flag &= ~QMC_MTR_DIST_DUP_MASK;
                    sNewMtrNode->flag |= QMC_MTR_DIST_DUP_TRUE;

                    IDE_TEST( qmg::makeDistNode( aStatement,
                                                 aQuerySet,
                                                 sROLL->plan.flag,
                                                 aChildPlan,
                                                 sTupleID,
                                                 aDistAggArg,
                                                 sItrAttr->expr,
                                                 sNewMtrNode,
                                                 & sROLL->distNode,
                                                 & sDistNodeCount )
                              != IDE_SUCCESS );
                }
                if ( sFirstMtrNode == NULL )
                {
                    sFirstMtrNode = sNewMtrNode;
                    sLastMtrNode  = sNewMtrNode;
                }
                else
                {
                    sLastMtrNode->next = sNewMtrNode;
                    sLastMtrNode       = sNewMtrNode;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        sROLL->aggrNode      = sFirstMtrNode;
        sROLL->distNodeCount = sDistNodeCount;
        IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                               sMtcTemplate,
                                               sTupleID,
                                               sColumnCount )
                  != IDE_SUCCESS );

        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_FALSE;
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

        IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sROLL->aggrNode )
                  != IDE_SUCCESS );
        if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK )
             == QMO_MAKESORT_VALUE_TEMP_FALSE )
        {
            IDE_TEST( qmg::setColumnLocate( aStatement, sROLL->aggrNode )
                      != IDE_SUCCESS );
            IDE_TEST( qmg::setCalcLocate( aStatement, sROLL->aggrNode )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        sROLL->aggrNode = NULL;
        sROLL->distNode = NULL;
        sROLL->distNodeCount = 0;
    }

    /* momory value temp가 생성될 필요가 있을경우 이를 생성한다. */
    if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK )
         == QMO_MAKESORT_VALUE_TEMP_TRUE )
    {
        sFirstMtrNode  = NULL;
        IDE_TEST( makeValueTempMtrNode( aStatement,
                                        aQuerySet,
                                        aFlag,
                                        aPlan,
                                        & sFirstMtrNode,
                                        & sMemValueCount )
                  != IDE_SUCCESS );
        sROLL->valueTempNode = sFirstMtrNode;
    }
    else
    {
        sROLL->valueTempNode = NULL;
    }

    sROLL->mtrNodeOffset = sDataNodeOffset;
    sROLL->myNodeOffset  = sROLL->mtrNodeOffset +
                   idlOS::align8( ID_SIZEOF(qmdMtrNode) * sROLL->mtrNodeCount );
    sROLL->sortNodeOffset = sROLL->myNodeOffset +
                   idlOS::align8( ID_SIZEOF(qmdMtrNode) * sROLL->myNodeCount );
    sROLL->aggrNodeOffset = sROLL->sortNodeOffset +
               idlOS::align8( ID_SIZEOF(qmdMtrNode) * sROLL->myNodeCount );
    sROLL->valueTempOffset = sROLL->aggrNodeOffset
            + idlOS::align8( ID_SIZEOF(qmdAggrNode) * sROLL->aggrNodeCount );

    if ( sROLL->distNodeCount > 0 )
    {
        sROLL->distNodePtrOffset = sROLL->valueTempOffset +
                idlOS::align8( ID_SIZEOF(qmdMtrNode) * sMemValueCount );
        sROLL->distNodeOffset = sROLL->distNodePtrOffset +
                   idlOS::align8( ID_SIZEOF(qmdDistNode *) * sROLL->groupCount );
        QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize =
                   sROLL->distNodeOffset +
                   idlOS::align8( ID_SIZEOF(qmdDistNode) * sDistNodeCount *
                                  sROLL->groupCount );
    }
    else
    {
        QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize =
                   sROLL->valueTempOffset +
                   idlOS::align8( ID_SIZEOF(qmdMtrNode) * sMemValueCount );
    }

    sMtrNode[0] = sROLL->mtrNode;
    sMtrNode[1] = sROLL->aggrNode;
    sMtrNode[2] = sROLL->distNode;
    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sROLL->plan,
                                            QMO_ROLL_DEPENDENCY,
                                            sTupleID,
                                            NULL,
                                            0,
                                            NULL,
                                            3,
                                            sMtrNode )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sROLL->plan.dependency == ID_UINT_MAX,
                    ERR_INVALID_DEPENDENCY );

    sROLL->depTupleID = (UShort)sROLL->plan.dependency;

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sROLL->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sROLL->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sROLL->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sROLL->planID,
                               sROLL->plan.flag,
                               sMtcTemplate->rows[sTempTuple].lflag,
                               sROLL->valueTempNode,
                               & sROLL->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOW_SUBQUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NOT_ALLOWED_SUBQUERY,
                                  "ROLLUP,CUBE Aggregation" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeROLL",
                                  "Invalid dependency " ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1353 makeAggrArgumentsMtr
 *
 *   Aggregation용 MTR 노드 를 생성하고 생성된 노드의 TupleID를 변경한다.
 *   memory이 일경우는 pointer만
 *   쌓게 되고 Aggregation에서 사용되는 Arguments는 pointer를 원복해서 사용하게 되므로
 *   별 상관이 없게 된다.
 *   하지만 Disk일 경우에 value로 읽어지게 되는데 이때 다음과 같은 경우가 있을 수 있다.
 *
 *   | i1 | i2 | i3 |
 *   위와 같이 sortNode가 구성되면 myNode로 setColumnLocate 로 TupleID로
 *   변경하면 Aggr(i2)의 i2의 dstTuple 역시 myNode의 TupleID로 변경된다.
 *   그렇게 되면 myNode의 i2 언제든지 NULL이 될 수 도 있고 않될 수 도 있다.
 *   그런데 Aggr의 Arguemnts의 i2는 항상 값을 가지고 있어야 한다.
 *   따라서 위와 같은 Disk Sort Temp의 경우
 *   | i1 | i2 | i3 | i2 | 와 같이 구성해서 Aggr의 Arguments의 i2는 뒷쪽의 value
 *   로 지정 해준다. 그래서 Aggregaion의 Arguments의 TupleID를 모두 새로 생성된
 *   TupleID로 바꿔주는 작업을 이 함수에서 하게 된다.
 *
 *   또 no_push_projection 인경우 RID를 쌓게 되는 데 이 때 포인터를 쌓게
 *   해준다.
 */
IDE_RC qmoOneMtrPlan::makeAggrArgumentsMtrNode( qcStatement * aStatement,
                                                qmsQuerySet * aQuerySet,
                                                UShort        aTupleID,
                                                UShort      * aColumnCount,
                                                mtcNode     * aNode,
                                                qmcMtrNode ** aFirstMtrNode )
{
    mtcTemplate * sMtcTemplate;
    qmcMtrNode  * sLastMtrNode;
    qmcMtrNode  * sNewMtrNode;
    ULong         sFlag;
    idBool        sIsConnectByFunc = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeAggrArgumentsMtrNode::__FT__" );

    sMtcTemplate = &QC_SHARED_TMPLATE( aStatement )->tmplate;

    /* BUG-38495 Rollup error with subquery */
    IDE_TEST_CONT( aNode->module == &qtc::subqueryModule, NORMAL_EXIT );

    if ( aNode->module == &qtc::columnModule )
    {
        for ( sLastMtrNode        = *aFirstMtrNode;
              sLastMtrNode->next != NULL;
              sLastMtrNode        = sLastMtrNode->next );

        IDE_TEST( qmg::changeColumnLocate( aStatement,
                                           aQuerySet,
                                           (qtcNode *)aNode,
                                           aTupleID,
                                           ID_FALSE )
                  != IDE_SUCCESS );
        if( qmg::getMtrMethod( aStatement,
                               aNode->table,
                               aTupleID ) == ID_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              (qtcNode *)aNode,
                                              ID_FALSE,
                                              aTupleID,
                                              0,
                                              aColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );
            sLastMtrNode->next = sNewMtrNode;
            sLastMtrNode       = sNewMtrNode;
        }
        else
        {
            if ( sLastMtrNode->srcNode == NULL )
            {
                IDE_TEST( qmg::makeBaseTableMtrNode( aStatement,
                                                     (qtcNode *)aNode,
                                                     aTupleID,
                                                     aColumnCount,
                                                     & sNewMtrNode )
                          != IDE_SUCCESS );
                sLastMtrNode->next = sNewMtrNode;
                sLastMtrNode       = sNewMtrNode;
            }
            else
            {
                sFlag = sMtcTemplate->rows[aNode->table].lflag;
                /* Surrogate-key(RID 또는 pointer)를 복사한다.
                 * 이미 surrogate-key가 추가되어있는지 확인한다.
                 */
                if( qmg::existBaseTable( (*aFirstMtrNode)->next,
                                         qmg::getBaseTableType( sFlag ),
                                         aNode->table )
                    == ID_TRUE )
                {
                    /* Nothing to do */
                }
                else
                {
                    IDE_TEST( qmg::makeBaseTableMtrNode( aStatement,
                                                         (qtcNode *)aNode,
                                                         aTupleID,
                                                         aColumnCount,
                                                         & sNewMtrNode )
                              != IDE_SUCCESS );
                    sLastMtrNode->next = sNewMtrNode;
                    sLastMtrNode       = sNewMtrNode;
                }
            }
        }
        /* BUG-37218 wrong result rollup when rollup use memory sort temp in disk table */
        aNode->lflag &= ~MTC_NODE_COLUMN_LOCATE_CHANGE_MASK;
        aNode->lflag |= MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE;
    }
    else
    {
        /* BUG-39611 support SYS_CONNECT_BY_PATH의 expression arguments
         * CONNECT BY  구문은 ROLLUP보다 먼저 실행되는 구문이고,
         * SYS_CONNECT_BY_PATH와 같은 경우 CONNECT BY 구문 실행시 계산되어야하기
         * 때문에 ROLL UP에서는 이를 쌓아서 처리해야한다. 따라서 아래와 같은
         * MTR NODE를 구성해 주면 ROLLUP에서는 SYS_CONNECT_BY_PATH의 결과가
         * 쌓이고 aggregation에서는 이 결과를 참조하여 Calculate를 수행한다.
         */
        if ( ( aNode->lflag & MTC_NODE_FUNCTION_CONNECT_BY_MASK )
             == MTC_NODE_FUNCTION_CONNECT_BY_TRUE )
        {
            for ( sLastMtrNode        = *aFirstMtrNode;
                  sLastMtrNode->next != NULL;
                  sLastMtrNode        = sLastMtrNode->next );

            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              (qtcNode *)aNode,
                                              ID_FALSE,
                                              aTupleID,
                                              0,
                                              aColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE;

            /* BUG-39611 CONNECT_BY_FUNC가 쌓일 것이므로 valueModuel로 세팅한다. */
            sNewMtrNode->dstNode->node.module = &qtc::valueModule;

            sLastMtrNode->next = sNewMtrNode;
            sLastMtrNode       = sNewMtrNode;

            sIsConnectByFunc = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( aNode->next != NULL )
    {
        IDE_TEST( makeAggrArgumentsMtrNode( aStatement,
                                            aQuerySet,
                                            aTupleID,
                                            aColumnCount,
                                            aNode->next,
                                            aFirstMtrNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( ( aNode->arguments != NULL ) &&
         ( sIsConnectByFunc == ID_FALSE ) )
    {
        IDE_TEST( makeAggrArgumentsMtrNode( aStatement,
                                            aQuerySet,
                                            aTupleID,
                                            aColumnCount,
                                            aNode->arguments,
                                            aFirstMtrNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1353 initCUBE
 *
 *  Cube에 Result Descript를 추가한다.
 *  GROUP BY 컬럼을 먼저 추가하고 Cube 컬럼을 추가한다.
 *  Aggregation 을 추가한다.
 */
IDE_RC qmoOneMtrPlan::initCUBE( qcStatement      * aStatement,
                                qmsQuerySet      * aQuerySet,
                                UInt             * aCubeCount,
                                qmsAggNode       * aAggrNode,
                                qmsConcatElement * aGroupColumn,
                                qmnPlan         ** aPlan )
{
    qmncCUBE         * sCUBE           = NULL;
    qmsConcatElement * sGroup          = NULL;
    qmsConcatElement * sSubGroup       = NULL;
    qmsAggNode       * sAggrNode       = NULL;
    qtcNode          * sNode           = NULL;
    qtcNode          * sListNode       = NULL;
    UInt               sDataNodeOffset = 0;
    UInt               sFlag           = 0;
    UInt               sCubeCount      = 0;
    UInt               sColumnCount    = 0;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::initCUBE::__FT__" );

    IDE_DASSERT( aStatement != NULL );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncCUBE ),
                                               (void **)& sCUBE )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sCUBE,
                        QC_SHARED_TMPLATE( aStatement),
                        QMN_CUBE,
                        qmnCUBE,
                        qmndCUBE,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sCUBE;

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
    sFlag |= QMC_ATTR_SORT_ORDER_ASC;

    for ( sGroup  = aGroupColumn;
          sGroup != NULL;
          sGroup  = sGroup->next )
    {
        if ( sGroup->type == QMS_GROUPBY_NORMAL )
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sCUBE->plan.resultDesc,
                                            sGroup->arithmeticOrList,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE |
                                            QMC_APPEND_ALLOW_CONST_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
            ++sColumnCount;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sFlag &= ~QMC_ATTR_GROUP_BY_EXT_MASK;
    sFlag |= QMC_ATTR_GROUP_BY_EXT_TRUE;

    for ( sGroup = aGroupColumn; sGroup != NULL; sGroup = sGroup->next )
    {
        if ( sGroup->type != QMS_GROUPBY_NORMAL )
        {
            for ( sSubGroup  = sGroup->arguments;
                  sSubGroup != NULL;
                  sSubGroup  = sSubGroup->next )
            {
                sSubGroup->type = sGroup->type;
                if ( ( sSubGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    for ( sListNode  = (qtcNode *)sSubGroup->arithmeticOrList->node.arguments;
                          sListNode != NULL;
                          sListNode  = (qtcNode *)sListNode->node.next )
                    {
                        IDE_TEST( qmc::appendAttribute( aStatement,
                                                        aQuerySet,
                                                        & sCUBE->plan.resultDesc,
                                                        sListNode,
                                                        sFlag,
                                                        QMC_APPEND_EXPRESSION_TRUE |
                                                        QMC_APPEND_ALLOW_DUP_TRUE |
                                                        QMC_APPEND_ALLOW_CONST_TRUE,
                                                        ID_FALSE )
                                  != IDE_SUCCESS );
                        ++sColumnCount;
                    }
                }
                else
                {
                    IDE_TEST( qmc::appendAttribute( aStatement,
                                                    aQuerySet,
                                                    & sCUBE->plan.resultDesc,
                                                    sSubGroup->arithmeticOrList,
                                                    sFlag,
                                                    QMC_APPEND_EXPRESSION_TRUE |
                                                    QMC_APPEND_ALLOW_DUP_TRUE  |
                                                    QMC_APPEND_ALLOW_CONST_TRUE,
                                                    ID_FALSE )
                              != IDE_SUCCESS );
                    ++sColumnCount;
                }
                ++sCubeCount;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    sFlag = 0;
    sFlag &= ~QMC_ATTR_GROUP_BY_EXT_MASK;
    sFlag |= QMC_ATTR_GROUP_BY_EXT_TRUE;

    for ( sAggrNode  = aAggrNode;
          sAggrNode != NULL;
          sAggrNode  = sAggrNode->next )
    {
        sNode = sAggrNode->aggr;
        while( sNode->node.module == &qtc::passModule )
        {
            sNode = ( qtcNode *)sNode->node.arguments;
        }

        /* BUG-35193  Window function 이 아닌 aggregation 만 처리해야 한다. */
        if ( ( QTC_IS_AGGREGATE( sNode ) == ID_TRUE ) &&
             ( sNode->overClause == NULL ) )
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sCUBE->plan.resultDesc,
                                            sNode,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE |
                                            QMC_APPEND_ALLOW_DUP_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_TEST_RAISE( ( sCubeCount > QMO_MAX_CUBE_COUNT ) ||
                    ( sColumnCount > QMO_MAX_CUBE_COLUMN_COUNT ),
                    ERR_INVALID_COUNT );

    /* PROJ-2462 Result Cache */
    IDE_TEST( qmo::initResultCacheStack( aStatement,
                                         aQuerySet,
                                         sCUBE->planID,
                                         ID_FALSE,
                                         ID_FALSE )
              != IDE_SUCCESS );

    *aCubeCount = sCubeCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_COUNT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_MAX_CUBE_COUNT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1353 makeCUBE
 *
 *    CUBE Plan은 하위에 memory Table이면 pointer를 쌓고 disk면 value를 Sort Temp에
 *    쌓아서 Sort를 여러번 수행하는 형식으로 Cube를 수행한다.
 *
 *    Cube( 컬럼 ) 은 2^n 만큼의 그룹수가 나오게 된다. 컬럼이 10개면 2의 10승이므로
 *    1024개의 그룹이 존재 하게 된다. 최대 컬럼 갯수는 15개로 제한한다.
 *    Cube는 ( 2^(n-1) )만큼 sort를 수행하게 된다.
 *
 *  - CUBE Plan은 5개의 Tuple과 mtr Node가 존재 할 수 있다.
 *  - mtrNode  - Sort용도로 사용된다.
 *  - myNode   - 상위 PLAN에서 보는 NODE이다.
 *  - aggrNode - aggregation MTR NODE
 *  - distNode - aggregation에 distinct가 지정될 때의 NODE
 *  - valueTempNode - value store용 TEMP NODE
 */
IDE_RC qmoOneMtrPlan::makeCUBE( qcStatement      * aStatement,
                                qmsQuerySet      * aQuerySet,
                                UInt               aFlag,
                                UInt               aCubeCount,
                                qmsAggNode       * aAggrNode,
                                qmoDistAggArg    * aDistAggArg,
                                qmsConcatElement * aGroupColumn,
                                qmnPlan          * aChildPlan ,
                                qmnPlan          * aPlan )
{
    mtcTemplate      * sMtcTemplate    = NULL;
    qmncCUBE         * sCUBE           = NULL;
    qmcAttrDesc      * sItrAttr        = NULL;
    qmcMtrNode       * sNewMtrNode     = NULL;
    qmcMtrNode       * sFirstMtrNode   = NULL;
    qmcMtrNode       * sLastMtrNode    = NULL;
    qmcMtrNode         sDummyNode;
    qmsAggNode       * sAggrNode       = NULL;
    qmsConcatElement * sGroupBy;
    qmsConcatElement * sSubGroup;
    qmcMtrNode       * sMtrNode[3];
    qtcNode          * sNode;

    UShort         sTupleID        = 0;
    UShort         sColumnCount    = 0;
    UShort         sAggrNodeCount  = 0;
    UShort         sDistNodeCount  = 0;
    UShort         sCount          = 0;
    UShort         sMemValueCount  = 0;
    UInt           sDataNodeOffset = 0;
    SInt           sCubeStart      = -1;
    idBool         sIsPartial      = ID_FALSE;
    UInt           i;
    UInt           sTempTuple      = 0;

    IDU_FIT_POINT_FATAL( "qmoOneMtrPlan::makeCUBE::__FT__" );

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aGroupColumn != NULL );
    IDE_DASSERT( aChildPlan   != NULL );
    IDE_DASSERT( aPlan        != NULL );

    sMtcTemplate    = &QC_SHARED_TMPLATE( aStatement )->tmplate;
    aPlan->offset   = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset + ID_SIZEOF( qmndCUBE ));

    sCUBE            = ( qmncCUBE * )aPlan;
    sCUBE->plan.left = aChildPlan;
    sCUBE->flag      = QMN_PLAN_FLAG_CLEAR;
    sCUBE->plan.flag = QMN_PLAN_FLAG_CLEAR;
    sCUBE->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->groupingInfoAddr,
                                    & sCUBE->groupingFuncRowID )
              != IDE_SUCCESS );

    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    for ( sAggrNode  = aAggrNode;
          sAggrNode != NULL;
          sAggrNode  = sAggrNode->next )
    {
        sAggrNodeCount++;
        sNode = sAggrNode->aggr;
        if ( sNode->node.arguments != NULL )
        {
            IDE_TEST_RAISE ( ( sNode->node.arguments->lflag & MTC_NODE_OPERATOR_MASK )
                             == MTC_NODE_OPERATOR_SUBQUERY,
                             ERR_NOT_ALLOW_SUBQUERY );
        }
        else
        {
            /* Nothing to do */
        }
    }

    sTempTuple = sTupleID;
    if( (aFlag & QMO_MAKESORT_TEMP_TABLE_MASK) ==
        QMO_MAKESORT_MEMORY_TEMP_TABLE )
    {
        sCUBE->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sCUBE->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

        /* PROJ-2462 Result Cache */
        if ( ( QC_SHARED_TMPLATE( aStatement )->resultCache.flag & QC_RESULT_CACHE_MASK )
             == QC_RESULT_CACHE_ENABLE )
        {
            if ( aQuerySet->SFWGH->hints->resultCacheType
                 != QMO_RESULT_CACHE_NO )
            {
                // Result Cache에서는 항상 Value Temp를 생성하도록 유도한다.
                aFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
                aFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( aQuerySet->SFWGH->hints->resultCacheType
                 == QMO_RESULT_CACHE )
            {
                // Result Cache에서는 항상 Value Temp를 생성하도록 유도한다.
                aFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
                aFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        sCUBE->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sCUBE->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    for ( sItrAttr = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next )
    {
        if (( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            if ( ( sItrAttr->flag & QMC_ATTR_GROUP_BY_EXT_MASK )
                 == QMC_ATTR_GROUP_BY_EXT_TRUE )
            {
                if ( sCubeStart == -1 )
                {
                    sCubeStart = sCount;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                sIsPartial = ID_TRUE;
            }

            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
            sNewMtrNode->flag &= ~QMC_MTR_GROUPING_MASK;
            sNewMtrNode->flag |= QMC_MTR_GROUPING_TRUE;
            sNewMtrNode->flag &= ~QMC_MTR_MTR_PLAN_MASK;
            sNewMtrNode->flag |= QMC_MTR_MTR_PLAN_TRUE;

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode = sNewMtrNode;
                sLastMtrNode  = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next = sNewMtrNode;
                sLastMtrNode       = sNewMtrNode;
            }
            sCount++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sDummyNode.next    = NULL;
    sDummyNode.srcNode = NULL;
    sNewMtrNode        = &sDummyNode;
    for ( sAggrNode  = aAggrNode;
          sAggrNode != NULL;
          sAggrNode  = sAggrNode->next )
    {
        sNode = sAggrNode->aggr;
        while ( sNode->node.module == &qtc::passModule )
        {
            sNode = (qtcNode *)sNode->node.arguments;
        }

        if ( ( sNode->node.lflag & MTC_NODE_FUNCTON_GROUPING_MASK )
             == MTC_NODE_FUNCTON_GROUPING_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        /* Aggregate function이 아닌 node가 전달되는 경우가 존재한다. */
        /* BUG-35193  Window function 이 아닌 aggregation 만 처리해야 한다. */
        if ( ( QTC_IS_AGGREGATE( sNode ) == ID_TRUE ) &&
             ( sNode->overClause == NULL ) )
        {
            if ( sNode->node.arguments != NULL )
            {
                IDE_TEST( makeAggrArgumentsMtrNode( aStatement,
                                                    aQuerySet,
                                                    sTupleID,
                                                    & sColumnCount,
                                                    sNode->node.arguments,
                                                    & sNewMtrNode )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    if ( sDummyNode.next != NULL )
    {
        sLastMtrNode->next = sDummyNode.next;
    }
    else
    {
        /* Nothing to do */
    }

    sCUBE->mtrNode      = sFirstMtrNode;
    sCUBE->mtrNodeCount = sColumnCount;

    if ( sIsPartial == ID_TRUE )
    {
        sCUBE->partialCube = sCubeStart;
    }
    else
    {
        sCUBE->partialCube = -1;
    }

    /* Cube의 각 element가 몇개의 컬럼으로 구성되어있는지를 저장한다. */
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ( ID_SIZEOF( UShort ) * aCubeCount ),
                                             (void**) & sCUBE->elementCount )
              != IDE_SUCCESS );

    for ( sGroupBy  = aGroupColumn;
          sGroupBy != NULL;
          sGroupBy  = sGroupBy->next )
    {
        if ( sGroupBy->type == QMS_GROUPBY_CUBE )
        {
            for ( sSubGroup  = sGroupBy->arguments, i = 0;
                  sSubGroup != NULL;
                  sSubGroup  = sSubGroup->next, i++ )
            {
                if ( ( sSubGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    sCUBE->elementCount[i] = sSubGroup->arithmeticOrList->node.lflag &
                        MTC_NODE_ARGUMENT_COUNT_MASK;
                }
                else
                {
                    sCUBE->elementCount[i] = 1;
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           sMtcTemplate,
                                           sTupleID,
                                           sColumnCount )
              != IDE_SUCCESS );

    if( (aFlag & QMO_MAKESORT_TEMP_TABLE_MASK) ==
        QMO_MAKESORT_MEMORY_TEMP_TABLE )
    {
        sCUBE->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sCUBE->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;
    }
    else
    {
        sCUBE->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sCUBE->plan.flag  |= QMN_PLAN_STORAGE_DISK;
        sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_DISK;

        if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
        }
        else
        {
            // Nothing To Do
        }
    }
    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;
    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sCUBE->mtrNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setColumnLocate( aStatement, sCUBE->mtrNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setCalcLocate( aStatement, sCUBE->mtrNode )
              != IDE_SUCCESS );

    /* sCUBE myNode */
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

    sFirstMtrNode = NULL;
    sCount        = 0;
    sColumnCount  = 0;
    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );
            if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK )
                 == QMO_MAKESORT_VALUE_TEMP_FALSE )
            {
                // 상위에서 temp table의 값을 참조하도록 변경된 위치를 설정한다.
                sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
                sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;
            }
            else
            {
                /* Nothing to do */
            }
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // myNode는 temp를 만들지 않는다.
            sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode = sNewMtrNode;
                sLastMtrNode  = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next = sNewMtrNode;
                sLastMtrNode       = sNewMtrNode;
            }
            sCount++;
        }
        else
        {
            /* Nothing to do */
        }
    }
    sCUBE->myNode      = sFirstMtrNode;
    sCUBE->myNodeCount = sCount;

    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           sMtcTemplate,
                                           sTupleID,
                                           sColumnCount )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;
    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sCUBE->myNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setColumnLocate( aStatement, sCUBE->myNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setCalcLocate( aStatement, sCUBE->myNode )
              != IDE_SUCCESS );

    /* aggrNode */
    sCUBE->aggrNodeCount = sAggrNodeCount;
    sCUBE->groupCount    = 0x1 << aCubeCount;
    sCUBE->cubeCount     = aCubeCount;

    if ( sAggrNodeCount > 0 )
    {
        IDE_TEST( qtc::nextTable( & sTupleID,
                                  aStatement,
                                  NULL,
                                  ID_TRUE,
                                  MTC_COLUMN_NOTNULL_TRUE )
                  != IDE_SUCCESS );

         sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
         sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

         sCUBE->distNode = NULL;
         sFirstMtrNode   = NULL;
         sColumnCount    = 0;
         sAggrNode = aAggrNode;
         for ( sItrAttr  = aPlan->resultDesc;
               sItrAttr != NULL;
               sItrAttr  = sItrAttr->next )
         {
             if ( ( ( sItrAttr->flag & QMC_ATTR_GROUP_BY_EXT_MASK)
                    == QMC_ATTR_GROUP_BY_EXT_TRUE ) &&
                  ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK)
                    == QMC_ATTR_KEY_FALSE ) )

             {
                 IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                                   aQuerySet,
                                                   sItrAttr->expr,
                                                   ID_FALSE,
                                                   sTupleID,
                                                   0,
                                                   & sColumnCount,
                                                   & sNewMtrNode )
                           != IDE_SUCCESS );

                 if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK )
                      == QMO_MAKESORT_VALUE_TEMP_FALSE )
                 {
                     sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
                     sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;
                 }
                 else
                 {
                     /* Nothing to do */
                 }

                 sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                 sNewMtrNode->flag |= QMC_MTR_TYPE_CALCULATE;

                 if ( ( sItrAttr->expr->node.lflag & MTC_NODE_DISTINCT_MASK )
                      == MTC_NODE_DISTINCT_FALSE )
                 {
                     sNewMtrNode->flag &= ~QMC_MTR_DIST_AGGR_MASK;
                     sNewMtrNode->flag |= QMC_MTR_DIST_AGGR_FALSE;
                     sNewMtrNode->myDist = NULL;
                 }
                 else
                 {
                     sNewMtrNode->flag &= ~QMC_MTR_DIST_AGGR_MASK;
                     sNewMtrNode->flag |= QMC_MTR_DIST_AGGR_TRUE;
                     sNewMtrNode->flag &= ~QMC_MTR_DIST_DUP_MASK;
                     sNewMtrNode->flag |= QMC_MTR_DIST_DUP_TRUE;

                     IDE_TEST( qmg::makeDistNode( aStatement,
                                                  aQuerySet,
                                                  sCUBE->plan.flag,
                                                  aChildPlan,
                                                  sTupleID,
                                                  aDistAggArg,
                                                  sItrAttr->expr,
                                                  sNewMtrNode,
                                                  & sCUBE->distNode,
                                                  & sDistNodeCount )
                               != IDE_SUCCESS );
                 }
                 if ( sFirstMtrNode == NULL )
                 {
                     sFirstMtrNode = sNewMtrNode;
                     sLastMtrNode  = sNewMtrNode;
                 }
                 else
                 {
                     sLastMtrNode->next = sNewMtrNode;
                     sLastMtrNode       = sNewMtrNode;
                 }
                 sAggrNode = sAggrNode->next;
             }
             else
             {
                 /* Nothing to do */
             }
         }
         sCUBE->aggrNode      = sFirstMtrNode;
         sCUBE->distNodeCount = sDistNodeCount;
         IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                                sMtcTemplate,
                                                sTupleID,
                                                sColumnCount )
                   != IDE_SUCCESS );

         sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
         sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;
         sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
         sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_FALSE;
         sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
         sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

         IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sCUBE->aggrNode )
                   != IDE_SUCCESS );
         if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK )
              == QMO_MAKESORT_VALUE_TEMP_FALSE )
         {
             IDE_TEST( qmg::setColumnLocate( aStatement, sCUBE->aggrNode )
                       != IDE_SUCCESS );
             IDE_TEST( qmg::setCalcLocate( aStatement, sCUBE->aggrNode )
                       != IDE_SUCCESS );
         }
         else
         {
             /* Nothing to do */
         }
    }
    else
    {
        sCUBE->aggrNode = NULL;
        sCUBE->distNode = NULL;
        sCUBE->distNodeCount = 0;
    }

    if ( ( aFlag & QMO_MAKESORT_VALUE_TEMP_MASK )
         == QMO_MAKESORT_VALUE_TEMP_TRUE )
    {
        sFirstMtrNode  = NULL;
        IDE_TEST( makeValueTempMtrNode( aStatement,
                                        aQuerySet,
                                        aFlag,
                                        aPlan,
                                        & sFirstMtrNode,
                                        & sMemValueCount )
                  != IDE_SUCCESS );
        sCUBE->valueTempNode = sFirstMtrNode;
    }
    else
    {
        sCUBE->valueTempNode = NULL;
    }
    sCUBE->mtrNodeOffset = sDataNodeOffset;
    sCUBE->myNodeOffset  = sCUBE->mtrNodeOffset +
        idlOS::align8(ID_SIZEOF(qmdMtrNode) * sCUBE->mtrNodeCount);
    sCUBE->sortNodeOffset = sCUBE->myNodeOffset +
        idlOS::align8(ID_SIZEOF(qmdMtrNode) * sCUBE->myNodeCount);
    sCUBE->aggrNodeOffset = sCUBE->sortNodeOffset +
        idlOS::align8(ID_SIZEOF(qmdMtrNode) * sCUBE->myNodeCount);
    sCUBE->valueTempOffset = sCUBE->aggrNodeOffset +
        idlOS::align8( ID_SIZEOF(qmdAggrNode) * sCUBE->aggrNodeCount );

    if ( sCUBE->distNodeCount > 0 )
    {
        sCUBE->distNodePtrOffset = sCUBE->valueTempOffset +
                idlOS::align8( ID_SIZEOF(qmdMtrNode) * sMemValueCount );
        sCUBE->distNodeOffset = sCUBE->distNodePtrOffset +
                idlOS::align8( ID_SIZEOF(qmdDistNode *) * ( sCUBE->cubeCount + 1 ) );
        QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize =
            sCUBE->distNodeOffset +
            idlOS::align8( ID_SIZEOF(qmdDistNode) * sCUBE->distNodeCount *
            ( sCUBE->cubeCount + 1 ) );
    }
    else
    {
        QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize = sCUBE->valueTempOffset +
                    idlOS::align8( ID_SIZEOF(qmdMtrNode) * sMemValueCount );
    }

    sMtrNode[0] = sCUBE->mtrNode;
    sMtrNode[1] = sCUBE->aggrNode;
    sMtrNode[2] = sCUBE->distNode;
    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sCUBE->plan,
                                            QMO_CUBE_DEPENDENCY,
                                            sTupleID,
                                            NULL,
                                            0,
                                            NULL,
                                            3,
                                            sMtrNode )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sCUBE->plan.dependency == ID_UINT_MAX,
                    ERR_INVALID_DEPENDENCY );

    sCUBE->depTupleID = (UShort)sCUBE->plan.dependency;

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sCUBE->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sCUBE->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sCUBE->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    /* PROJ-2462 Result Cache */
    qmo::makeResultCacheStack( aStatement,
                               aQuerySet,
                               sCUBE->planID,
                               sCUBE->plan.flag,
                               sMtcTemplate->rows[sTempTuple].lflag,
                               sCUBE->valueTempNode,
                               &sCUBE->componentInfo,
                               ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOW_SUBQUERY )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QDB_NOT_ALLOWED_SUBQUERY,
                                 "ROLLUP,CUBE Aggregation" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DEPENDENCY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOneMtrPlan::makeCUBE",
                                  "Invalid dependency " ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneMtrPlan::initCNBYForJoin( qcStatement    * aStatement,
                                       qmsQuerySet    * aQuerySet,
                                       qmoLeafInfo    * aLeafInfo,
                                       qmsSortColumns * aSiblings,
                                       qmnPlan        * aParent,
                                       qmnPlan       ** aPlan )
{
    qmncCNBY       * sCNBY           = NULL;
    UInt             sDataNodeOffset = 0;
    UInt             sFlag           = 0;
    qtcNode        * sNode           = NULL;
    qmsSortColumns * sSibling        = NULL;
    qmcAttrDesc    * sItrAttr        = NULL;
    idBool           sIsTrue         = ID_FALSE;

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncCNBY ),
                                               (void **)& sCNBY )
              != IDE_SUCCESS );

    idlOS::memset( sCNBY, 0x00, ID_SIZEOF(qmncCNBY));
    QMO_INIT_PLAN_NODE( sCNBY,
                        QC_SHARED_TMPLATE( aStatement),
                        QMN_CNBY,
                        qmnCNBY,
                        qmndCNBY,
                        sDataNodeOffset );

    // CONNECT BY절 추가
    // PROJ-2469 Optimize View Materialization
    // BUG FIX : aLeafInfo[0] -> aLeafInfo[1]로 변경
    //                           CONNECT BY 의 Predicate을 등록하지 않고 START WITH만 두 번
    //                           등록하고 있었음.
    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    & sCNBY->plan.resultDesc,
                                    aLeafInfo[1].predicate,
                                    sFlag )
              != IDE_SUCCESS );

    // START WITH절 추가
    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    & sCNBY->plan.resultDesc,
                                    aLeafInfo[0].predicate,
                                    sFlag )
              != IDE_SUCCESS );

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    for ( sSibling  = aSiblings;
          sSibling != NULL;
          sSibling  = sSibling->next )
    {
        if ( aSiblings->isDESC == ID_FALSE )
        {
            sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_ORDER_ASC;
        }
        else
        {
            sFlag &= ~QMC_ATTR_SORT_ORDER_MASK;
            sFlag |= QMC_ATTR_SORT_ORDER_DESC;
        }

        IDE_TEST( qmc::appendAttribute( aStatement,
                                        aQuerySet,
                                        & sCNBY->plan.resultDesc,
                                        sSibling->sortColumn,
                                        sFlag,
                                        QMC_APPEND_EXPRESSION_TRUE,
                                        ID_FALSE )
                  != IDE_SUCCESS );
    }

    /**
     * select connect_by_root(t1.i1) from t1 left outer join t2 on t1.i2 = t2.i2 + 1 
     *  connect by prior t2.i1 = t1.i1 order by t1.i1;
     *  order by 와 connect_by_root or sys_connect_by_path가 같이 사요된경우
     *  sort에서 쌓아서 처리해야하기 때문에 SEALED_TRUE로 expression을 전체를
     *  넣게된다. 하지만 이는 sort에서 사용하기 위해서이지 hierarchy query에서는
     *  아니므로 걸러줘야하기 때문에 아래와 같이 잠시 SEALED_FALSE로 parent를 
     *  변환했다가 다시 원복시킨다.
     */
    for ( sItrAttr = aParent->resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next )
    {
        if ( ( sItrAttr->expr->node.lflag & MTC_NODE_FUNCTION_CONNECT_BY_MASK )
               == MTC_NODE_FUNCTION_CONNECT_BY_TRUE )
        {
            if ( ( sItrAttr->flag & QMC_ATTR_SEALED_MASK )
                 == QMC_ATTR_SEALED_TRUE )
            {
                sIsTrue = ID_TRUE;
                sItrAttr->flag &= ~QMC_ATTR_SEALED_MASK;
                sItrAttr->flag |= QMC_ATTR_SEALED_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   & sCNBY->plan.resultDesc )
              != IDE_SUCCESS );

    if ( sIsTrue == ID_TRUE )
    {
        for ( sItrAttr = aParent->resultDesc;
              sItrAttr != NULL;
              sItrAttr = sItrAttr->next )
        {
            if ( ( sItrAttr->expr->node.lflag & MTC_NODE_FUNCTION_CONNECT_BY_MASK )
                   == MTC_NODE_FUNCTION_CONNECT_BY_TRUE )
            {
                sItrAttr->flag &= ~QMC_ATTR_SEALED_MASK;
                sItrAttr->flag |= QMC_ATTR_SEALED_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }
    sFlag &= ~QMC_ATTR_KEY_MASK;

    for ( sItrAttr  = sCNBY->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( ( sItrAttr->expr->lflag & QTC_NODE_LEVEL_MASK ) == QTC_NODE_LEVEL_EXIST ) ||
             ( ( sItrAttr->expr->lflag & QTC_NODE_ISLEAF_MASK ) == QTC_NODE_ISLEAF_EXIST ) )
        {
            sItrAttr->flag &= ~QMC_ATTR_TERMINAL_MASK;
            sItrAttr->flag |= QMC_ATTR_TERMINAL_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        if ( ( sItrAttr->expr->lflag & QTC_NODE_PRIOR_MASK ) == QTC_NODE_PRIOR_EXIST )
        {
            sItrAttr->flag &= ~QMC_ATTR_TERMINAL_MASK;
            sItrAttr->flag |= QMC_ATTR_TERMINAL_TRUE;

            IDU_FIT_POINT( "qmoOneMtrPlan::initCNBYForJoin::alloc::sNode",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                       (void**)&sNode )
                      != IDE_SUCCESS );

            idlOS::memcpy( sNode, sItrAttr->expr, ID_SIZEOF( qtcNode ) );

            sNode->lflag &= ~QTC_NODE_PRIOR_MASK;
            sNode->lflag |= QTC_NODE_PRIOR_ABSENT;
            sNode->depInfo.depCount = 1;
            sNode->depInfo.depend[0] = sNode->node.table;
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sCNBY->plan.resultDesc,
                                            sNode,
                                            sFlag,
                                            0,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

    }

    *aPlan = (qmnPlan *)sCNBY;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneMtrPlan::makeCNBYForJoin( qcStatement    * aStatement,
                                       qmsQuerySet    * aQuerySet,
                                       qmsSortColumns * aSiblings,
                                       qmoLeafInfo    * aStartWith,
                                       qmoLeafInfo    * aConnectBy,
                                       qmnPlan        * aChildPlan,
                                       qmnPlan        * aPlan )
{
    mtcTemplate  * sMtcTemplate    = NULL;
    qmncCNBY     * sCNBY           = (qmncCNBY *)aPlan;
    UShort         sPriorID        = 0;
    UShort         sColumnCount    = 0;
    UShort         sOrgTable       = 0;
    UShort         sOrgColumn      = 0;
    UInt           sDataNodeOffset = 0;
    UInt           i               = 0;
    UInt           sCurOffset      = 0;
    qmcAttrDesc  * sItrAttr        = NULL;
    qmcMtrNode   * sNewMtrNode     = NULL;
    qmcMtrNode   * sLastMtrNode    = NULL;
    qmcMtrNode   * sLastMtrNode2   = NULL;
    qmoPredicate * sPred           = NULL;
    qmoPredicate * sPredMore       = NULL;
    ULong          sFlag           = 0;
    qtcNode      * sPredicate[9];

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    sMtcTemplate = &QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8(aPlan->offset + ID_SIZEOF(qmndCNBY));

    sCNBY->plan.left     = aChildPlan;
    sCNBY->mtrNodeOffset = sDataNodeOffset;

    sCNBY->flag             = QMN_PLAN_FLAG_CLEAR;
    sCNBY->plan.flag        = QMN_PLAN_FLAG_CLEAR;
    sCNBY->plan.flag       |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //loop을 찾을 것인지에 대한 결정
    if ( ( aQuerySet->SFWGH->hierarchy->flag & QMS_HIERARCHY_IGNORE_LOOP_MASK ) ==
           QMS_HIERARCHY_IGNORE_LOOP_TRUE )
    {
        sCNBY->flag &= ~QMNC_CNBY_IGNORE_LOOP_MASK;
        sCNBY->flag |= QMNC_CNBY_IGNORE_LOOP_TRUE;
    }
    else
    {
        sCNBY->flag &= ~QMNC_CNBY_IGNORE_LOOP_MASK;
        sCNBY->flag |= QMNC_CNBY_IGNORE_LOOP_FALSE;
    }

    sCNBY->flag &= ~QMNC_CNBY_CHILD_VIEW_MASK;
    sCNBY->flag |= QMNC_CNBY_CHILD_VIEW_FALSE;
    sCNBY->flag &= ~QMNC_CNBY_JOIN_MASK;
    sCNBY->flag |= QMNC_CNBY_JOIN_TRUE;

    if ( aSiblings != NULL )
    {
        sCNBY->flag &= ~QMNC_CNBY_SIBLINGS_MASK;
        sCNBY->flag |= QMNC_CNBY_SIBLINGS_TRUE;
    }
    else
    {
        sCNBY->flag &= ~QMNC_CNBY_SIBLINGS_MASK;
        sCNBY->flag |= QMNC_CNBY_SIBLINGS_FALSE;
    }
    IDE_TEST( qtc::nextTable( &sCNBY->myRowID,
                              aStatement,
                              NULL,
                              ID_FALSE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );
    sCNBY->mIndex       = NULL;
    sCNBY->mTableHandle = NULL;
    sCNBY->mTableSCN    = 0;
    sCNBY->baseRowID    = sCNBY->myRowID;

    IDE_TEST( qtc::nextTable( &sPriorID,
                              aStatement,
                              NULL,
                              ID_FALSE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    aQuerySet->SFWGH->hierarchy->priorTable    = sPriorID;
    sCNBY->priorRowID                          = sPriorID;

    /* BUG-48300 */
    if ( aQuerySet->SFWGH->level != NULL )
    {
        IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->level,
                                        & sCNBY->levelRowID )
                  != IDE_SUCCESS );
        sCNBY->flag &= ~QMNC_CNBY_LEVEL_MASK;
        sCNBY->flag |= QMNC_CNBY_LEVEL_TRUE;
    }

    /* BUG-48300 */
    if ( aQuerySet->SFWGH->isLeaf != NULL )
    {
        IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->isLeaf,
                                        & sCNBY->isLeafRowID )
                  != IDE_SUCCESS );
        sCNBY->flag &= ~QMNC_CNBY_ISLEAF_MASK;
        sCNBY->flag |= QMNC_CNBY_ISLEAF_TRUE;
    }
    /* BUG-48300 */
    if ( aQuerySet->SFWGH->cnbyStackAddr != NULL )
    {
        IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->cnbyStackAddr,
                                        & sCNBY->stackRowID )
                  != IDE_SUCCESS );
        sCNBY->flag &= ~QMNC_CNBY_FUNCTION_MASK;
        sCNBY->flag |= QMNC_CNBY_FUNCTION_TRUE;
    }

    IDE_TEST( setPseudoColumnRowID( aQuerySet->SFWGH->rownum,
                                    & sCNBY->rownumRowID )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sCNBY->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     &( aQuerySet->depInfo ) )
              != IDE_SUCCESS );

    for ( sItrAttr  = sCNBY->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( sItrAttr->expr->lflag & QTC_NODE_LOB_COLUMN_MASK ) == QTC_NODE_LOB_COLUMN_EXIST )
        {
            sFlag = sMtcTemplate->rows[sItrAttr->expr->node.table].lflag;

            if ( qmg::existBaseTable( sCNBY->mBaseMtrNode,
                                      qmg::getBaseTableType( sFlag ),
                                      sItrAttr->expr->node.table )
                    == ID_TRUE )
            {
                continue;
            }
            else
            {
                IDE_TEST( qmg::makeBaseTableMtrNode( aStatement,
                                                     sItrAttr->expr,
                                                     sCNBY->myRowID,
                                                     &sColumnCount,
                                                     &sNewMtrNode )
                          != IDE_SUCCESS );

                if ( sCNBY->mBaseMtrNode == NULL )
                {
                    sCNBY->mBaseMtrNode = sNewMtrNode;
                    sLastMtrNode  = sNewMtrNode;
                }
                else
                {
                    sLastMtrNode->next = sNewMtrNode;
                    sLastMtrNode       = sNewMtrNode;
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    for ( sItrAttr  = sCNBY->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( QTC_IS_PSEUDO( sItrAttr->expr ) == ID_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sItrAttr->expr->lflag & QTC_NODE_PRIOR_MASK ) == QTC_NODE_PRIOR_EXIST )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sItrAttr->expr->lflag & QTC_NODE_LOB_COLUMN_MASK ) == QTC_NODE_LOB_COLUMN_EXIST )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }
        sOrgTable  = sItrAttr->expr->node.baseTable;
        sOrgColumn = sItrAttr->expr->node.baseColumn;
        IDE_TEST( qmg::makeColumnMtrNode( aStatement ,
                                          aQuerySet ,
                                          sItrAttr->expr,
                                          ID_FALSE,
                                          sCNBY->myRowID ,
                                          0,
                                          & sColumnCount ,
                                          & sNewMtrNode )
                  != IDE_SUCCESS );

        for ( sPred = aStartWith->predicate; sPred != NULL; sPred = sPred->next )
        {
            for ( sPredMore = sPred; sPredMore != NULL; sPredMore = sPredMore->more )
            {
                changeNodeForCNBYJoin( sPredMore->node,
                                       sOrgTable,
                                       sOrgColumn,
                                       sNewMtrNode->dstNode->node.table,
                                       sNewMtrNode->dstNode->node.column );
            }
        }
        for ( sPred = aConnectBy->predicate; sPred != NULL; sPred = sPred->next )
        {
            for ( sPredMore = sPred; sPredMore != NULL; sPredMore = sPredMore->more )
            {
                changeNodeForCNBYJoin( sPredMore->node,
                                       sOrgTable,
                                       sOrgColumn,
                                       sNewMtrNode->dstNode->node.table,
                                       sNewMtrNode->dstNode->node.column );
            }
        }
        if ( ( sItrAttr->flag & QMC_ATTR_USELESS_RESULT_MASK ) == QMC_ATTR_USELESS_RESULT_TRUE )
        {
            // PROJ-2469 Optimize View Materialization
            // 상위에서 사용되지 않는 MtrNode에 대해서 flag 처리한다.
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_USELESS_COLUMN;
        }
        else
        {
            sFlag = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sNewMtrNode->srcNode->node.table].lflag;

            if ( ( sFlag & MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK )
                   == MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE )
            {
                sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                sNewMtrNode->flag |= QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN;
            }
            else
            {
                //CMTR에서 생성된 qmcMtrNode이므로 복사하도록 한다.
                sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;
            }
        }

        // PROJ-2179
        sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
        sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE;

        // 상위에서 temp table의 값을 참조하도록 변경된 위치를 설정한다.
        sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
        sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;

        //connect
        if ( sCNBY->mBaseMtrNode == NULL )
        {
            sCNBY->mBaseMtrNode = sNewMtrNode;
            sLastMtrNode  = sNewMtrNode;
        }
        else
        {
            sLastMtrNode->next = sNewMtrNode;
            sLastMtrNode       = sNewMtrNode;
        }
    }

    for ( sItrAttr  = sCNBY->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            sOrgTable  = sItrAttr->expr->node.baseTable;
            sOrgColumn = sItrAttr->expr->node.baseColumn;
            IDE_TEST( qmg::makeColumnMtrNode( aStatement ,
                                              aQuerySet ,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sCNBY->myRowID ,
                                              0,
                                              & sColumnCount ,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            if ( ( sItrAttr->flag & QMC_ATTR_USELESS_RESULT_MASK ) == QMC_ATTR_USELESS_RESULT_TRUE )
            {
                // PROJ-2469 Optimize View Materialization
                // 상위에서 사용되지 않는 MtrNode에 대해서 flag 처리한다.
                sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                sNewMtrNode->flag |= QMC_MTR_TYPE_USELESS_COLUMN;
            }
            else
            {
                sFlag = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sNewMtrNode->srcNode->node.table].lflag;
                if ( ( sFlag & MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK )
                       == MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE )
                {
                    sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                    sNewMtrNode->flag |= QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN;
                }
                else
                {
                    //CMTR에서 생성된 qmcMtrNode이므로 복사하도록 한다.
                    sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                    sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;
                }
            }

            for ( sPred = aStartWith->predicate; sPred != NULL; sPred = sPred->next )
            {
                for ( sPredMore = sPred; sPredMore != NULL; sPredMore = sPredMore->more )
                {
                    changeNodeForCNBYJoin( sPredMore->node,
                                           sOrgTable,
                                           sOrgColumn,
                                           sNewMtrNode->dstNode->node.table,
                                           sNewMtrNode->dstNode->node.column );
                }
            }
            for ( sPred = aConnectBy->predicate; sPred != NULL; sPred = sPred->next )
            {
                for ( sPredMore = sPred; sPredMore != NULL; sPredMore = sPredMore->more )
                {
                    changeNodeForCNBYJoin( sPredMore->node,
                                           sOrgTable,
                                           sOrgColumn,
                                           sNewMtrNode->dstNode->node.table,
                                           sNewMtrNode->dstNode->node.column );
                }
            }
            sNewMtrNode->flag &= ~QMC_MTR_SORT_NEED_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_NEED_TRUE;

            // PROJ-2179
            sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
            sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE;

            if ( ( sItrAttr->flag & QMC_ATTR_SORT_ORDER_MASK )
                    == QMC_ATTR_SORT_ORDER_ASC )
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;
            }
            else
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_DESCENDING;
            }

            /* PROJ-2435 order by nulls first/last */
            if ( ( sItrAttr->flag & QMC_ATTR_SORT_NULLS_ORDER_MASK )
                 != QMC_ATTR_SORT_NULLS_ORDER_NONE )
            {
                if ( ( sItrAttr->flag & QMC_ATTR_SORT_NULLS_ORDER_MASK )
                     == QMC_ATTR_SORT_NULLS_ORDER_FIRST )
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_FIRST;
                }
                else
                {
                    sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                    sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_LAST;
                }
            }
            else
            {
                sNewMtrNode->flag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
                sNewMtrNode->flag |= QMC_MTR_SORT_NULLS_ORDER_NONE;
            }
            // 상위에서 temp table의 값을 참조하도록 변경된 위치를 설정한다.
            sItrAttr->expr->node.table  = sNewMtrNode->dstNode->node.table;
            sItrAttr->expr->node.column = sNewMtrNode->dstNode->node.column;

            //connect
            if ( sCNBY->mBaseMtrNode == NULL )
            {
                sCNBY->mBaseMtrNode = sNewMtrNode;
                sLastMtrNode  = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next = sNewMtrNode;
                sLastMtrNode       = sNewMtrNode;
            }
            if ( sCNBY->baseSortNode == NULL )
            {
                sCNBY->baseSortNode = sNewMtrNode;
                sLastMtrNode2  = sNewMtrNode;
            }
            else
            {
                sLastMtrNode2->next = sNewMtrNode;
                sLastMtrNode2       = sNewMtrNode;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    sCurOffset = sCNBY->mtrNodeOffset;

    // 다음 노드가 저장될 시작 지점
    sCurOffset += idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sColumnCount;

    sCNBY->baseSortOffset = sCurOffset;

    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           sMtcTemplate,
                                           sCNBY->myRowID,
                                           sColumnCount )
              != IDE_SUCCESS );

    sMtcTemplate->rows[sCNBY->myRowID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sCNBY->myRowID].lflag |= MTC_TUPLE_PLAN_TRUE;
    sMtcTemplate->rows[sCNBY->myRowID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sCNBY->myRowID].lflag |= MTC_TUPLE_PLAN_MTR_TRUE;

    sCNBY->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
    sCNBY->plan.flag |= QMN_PLAN_STORAGE_MEMORY;

    IDE_TEST( qmg::copyMtcColumnExecute( aStatement, sCNBY->mBaseMtrNode )
              != IDE_SUCCESS );
    IDE_TEST( qmg::setColumnLocate( aStatement, sCNBY->mBaseMtrNode )
              != IDE_SUCCESS );

    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           sMtcTemplate,
                                           sCNBY->priorRowID,
                                           sColumnCount )
              != IDE_SUCCESS );

    for ( i = 0; i < sMtcTemplate->rows[sCNBY->priorRowID].columnCount; i++ )
    {
        idlOS::memcpy( (void *) &(sMtcTemplate->rows[sCNBY->priorRowID].columns[i]),
                       (void *) &(sMtcTemplate->rows[sCNBY->myRowID].columns[i]),
                       ID_SIZEOF(mtcColumn) );

        /* BUG-47750  Connect by 구문에 Variable column 과 join이 함께 사용할 경우 FATAL */
        if ( ( ( sMtcTemplate->rows[sCNBY->priorRowID].columns[i].column.flag & SMI_COLUMN_TYPE_MASK )
               == SMI_COLUMN_TYPE_VARIABLE ) ||
               ( ( sMtcTemplate->rows[sCNBY->priorRowID].columns[i].column.flag & SMI_COLUMN_TYPE_MASK )
               == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
        {
            sMtcTemplate->rows[sCNBY->priorRowID].columns[i].column.flag &= ~SMI_COLUMN_TYPE_MASK;
            sMtcTemplate->rows[sCNBY->priorRowID].columns[i].column.flag |= SMI_COLUMN_TYPE_FIXED;
        }
    }

    IDE_TEST( processStartWithPredicate( aStatement,
                                         aQuerySet,
                                         sCNBY,
                                         aStartWith )
              != IDE_SUCCESS );

    IDE_TEST( processConnectByPredicateForJoin( aStatement,
                                                aQuerySet,
                                                sCNBY,
                                                aConnectBy )
              != IDE_SUCCESS );

    /* 마무리 작업1 */
    sPredicate[0] = sCNBY->startWithConstant;
    sPredicate[1] = sCNBY->startWithFilter;
    sPredicate[2] = sCNBY->startWithSubquery;
    sPredicate[3] = sCNBY->startWithNNF;
    sPredicate[4] = sCNBY->connectByConstant;
    sPredicate[5] = sCNBY->connectByFilter;
    /* fix BUG-26770
     * connect by LEVEL+to_date(:D1,'YYYYMMDD') <= to_date(:D2,'YYYYMMDD')+1;
     * 질의수행시 서버비정상종료
     * level filter에 대한 처리가 없어,
     * level filter에 포한된 host변수를 등록하지 못해 bindParamInfo 설정시, 비정상종료함.
     */
    sPredicate[6] = sCNBY->levelFilter;

    /* BUG-39041 The connect by clause is not support subquery. */
    sPredicate[7] = sCNBY->connectBySubquery;

    /* BUG-39434 The connect by need rownum pseudo column. */
    sPredicate[8] = sCNBY->rownumFilter;

    for ( i = 0;
          i < 9;
          i++ )
    {
        IDE_TEST( qmg::changeColumnLocate( aStatement,
                                           aQuerySet,
                                           sPredicate[i],
                                           ID_USHORT_MAX,
                                           ID_TRUE )
                  != IDE_SUCCESS );
    }

    /* dependency 처리 및 subquery의 처리 */
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sCNBY->plan ,
                                            QMO_CNBY_DEPENDENCY_FOR_JOIN,
                                            sCNBY->myRowID,
                                            NULL,
                                            9,
                                            sPredicate,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sCNBY->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sCNBY->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sCNBY->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2509 Connect By Predicate 처리 */
IDE_RC qmoOneMtrPlan::processConnectByPredicateForJoin( qcStatement    * aStatement,
                                                        qmsQuerySet    * aQuerySet,
                                                        qmncCNBY       * aCNBY,
                                                        qmoLeafInfo    * aConnectBy )
{
    qmncScanMethod sMethod;
    idBool         sIsSubQuery    = ID_FALSE;
    idBool         sFind          = ID_FALSE;
    qtcNode      * sNode          = NULL;
    qtcNode      * sSortNode      = NULL;
    qtcNode      * sPriorPred     = NULL;
    qtcNode      * sOptimizedNode = NULL;
    qmcMtrNode   * sMtrNode       = NULL;
    qmcMtrNode   * sTmp           = NULL;
    UInt           sCurOffset     = 0;
    qtcNode        sPriorTmp;
    UShort         sColumnCount   = 0;
    qmoPredicate * sPred          = NULL;
    qmoPredicate * sPredMore      = NULL;
    qmcMtrNode   * sNewMtrNode    = NULL;

    aCNBY->levelFilter       = NULL;
    aCNBY->rownumFilter      = NULL;
    aCNBY->connectByKeyRange = NULL;
    aCNBY->connectByFilter   = NULL;
    aCNBY->priorNode         = NULL;

    /* 1. Level Filter 의 처리 */
    if ( aConnectBy->levelPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          aConnectBy->levelPredicate,
                                          & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement,
                                           aQuerySet,
                                           sNode )
                  != IDE_SUCCESS );

        /* BUG-17921 */
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                                & sOptimizedNode )
                   != IDE_SUCCESS );
        aCNBY->levelFilter = sOptimizedNode;

        IDE_TEST( qtc::optimizeHostConstExpression( aStatement,
                                                    aCNBY->levelFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-39434 The connect by need rownum pseudo column.
     * 1-2 Rownum Filter 의 처리
     */
    if ( aConnectBy->connectByRownumPred != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          aConnectBy->connectByRownumPred,
                                          & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement,
                                           aQuerySet,
                                           sNode )
                  != IDE_SUCCESS );

        /* BUG-17921 */
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );
        aCNBY->rownumFilter = sOptimizedNode;

        IDE_TEST( qtc::optimizeHostConstExpression( aStatement,
                                                    aCNBY->rownumFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2641 Hierarchy Query Index
     * Connectby predicate에서 prior Node를 찾아서
     * Error가 나는 상황을 체크해야한다.
     */
    for ( sPred = aConnectBy->predicate; sPred != NULL; sPred = sPred->next )
    {
        for ( sPredMore = sPred;
              sPredMore != NULL;
              sPredMore = sPredMore->more )
        {
            sPriorTmp.node.next = NULL;
            IDE_TEST( findPriorColumns( aStatement,
                                        sPredMore->node,
                                        & sPriorTmp )
                      != IDE_SUCCESS );
            /* BUG-44759 processPredicae을 수행하기전에 priorNode
             * 가 있어야한다.
             */
            aCNBY->priorNode = (qtcNode *)sPriorTmp.node.next;

            if ( aCNBY->priorNode != NULL )
            {
                aQuerySet->SFWGH->hierarchy->originalTable = aCNBY->priorNode->node.table;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    IDE_TEST( qmoOneNonPlan::processPredicate( aStatement,
                                               aQuerySet,
                                               aConnectBy->predicate,
                                               aConnectBy->constantPredicate,
                                               aConnectBy->ridPredicate,
                                               aConnectBy->index,
                                               aCNBY->myRowID,
                                               & sMethod.constantFilter,
                                               & sMethod.filter,
                                               & sMethod.lobFilter,
                                               & sMethod.subqueryFilter,
                                               & sMethod.varKeyRange,
                                               & sMethod.varKeyFilter,
                                               & sMethod.varKeyRange4Filter,
                                               & sMethod.varKeyFilter4Filter,
                                               & sMethod.fixKeyRange,
                                               & sMethod.fixKeyFilter,
                                               & sMethod.fixKeyRange4Print,
                                               & sMethod.fixKeyFilter4Print,
                                               & sMethod.ridRange,
                                               & sIsSubQuery )
              != IDE_SUCCESS );

    /* 3. constantFilter 지정 */
    if ( sMethod.constantFilter != NULL )
    {
        aCNBY->connectByConstant = sMethod.constantFilter;
    }
    else
    {
        aCNBY->connectByConstant = NULL;
    }

    /* BUG-39401 need subquery for connect by clause */
    if ( sMethod.subqueryFilter != NULL )
    {
        aCNBY->connectBySubquery = sMethod.subqueryFilter;
    }
    else
    {
        aCNBY->connectBySubquery = NULL;
    }

    if ( ( sMethod.varKeyRange == NULL ) &&
         ( sMethod.varKeyFilter == NULL ) &&
         ( sMethod.varKeyRange4Filter == NULL ) &&
         ( sMethod.varKeyFilter4Filter == NULL ) &&
         ( sMethod.fixKeyRange == NULL ) &&
         ( sMethod.fixKeyFilter == NULL ) )
    {
        aCNBY->mIndex = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST_RAISE( sMethod.lobFilter != NULL, ERR_NOT_SUPPORT_LOB_FILTER );

    if ( sMethod.filter != NULL )
    {
        aCNBY->connectByFilter = sMethod.filter;

        sPriorTmp.node.next = NULL;
        IDE_TEST( findPriorColumns( aStatement,
                                    sMethod.filter,
                                    & sPriorTmp )
                  != IDE_SUCCESS );
        aCNBY->priorNode = (qtcNode *)sPriorTmp.node.next;

        /* 4-2. find Prior predicate and Sort node */
        IDE_TEST( findPriorPredAndSortNode( aStatement,
                                            sMethod.filter,
                                            &sSortNode,
                                            &sPriorPred,
                                            aCNBY->myRowID,
                                            &sFind )
                  != IDE_SUCCESS );

        if ( ( sFind == ID_TRUE ) &&
             ( aCNBY->mIndex == NULL ) )
        {
            if ( ( aCNBY->flag & QMNC_CNBY_SIBLINGS_MASK )
                 == QMNC_CNBY_SIBLINGS_TRUE )
            {
                IDE_TEST( makeSortNodeOfCNBY( aStatement,
                                              aQuerySet,
                                              aCNBY,
                                              sSortNode )
                          != IDE_SUCCESS );
            }
            else
            {
                for ( sMtrNode  = aCNBY->mBaseMtrNode;
                      sMtrNode != NULL;
                      sMtrNode  = sMtrNode->next )
                {
                    if ( ( sMtrNode->dstNode->node.table == sSortNode->node.table ) &&
                         ( sMtrNode->dstNode->node.column == sSortNode->node.column ))
                    {
                        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmcMtrNode, &sNewMtrNode)
                                  != IDE_SUCCESS);
                        *sNewMtrNode = *sMtrNode;
                        aCNBY->baseSortNode = sNewMtrNode;
                        sNewMtrNode->next = NULL;

                        sNewMtrNode->flag &= ~QMC_MTR_SORT_NEED_MASK;
                        sNewMtrNode->flag |= QMC_MTR_SORT_NEED_TRUE;
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            if ( (((qtcNode*)(sPriorPred->node.arguments))->lflag & QTC_NODE_PRIOR_MASK) ==
                 QTC_NODE_PRIOR_EXIST )
            {
                sPriorPred->indexArgument = 1;
            }
            else
            {
                /* Nothing to do */
            }

            /* CNF sPriorPredicate 를 DNF 형태로 변환 */
            IDE_TEST( qmoNormalForm::normalizeDNF( aStatement,
                                                   sPriorPred,
                                                   &aCNBY->connectByKeyRange)
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    sCurOffset = aCNBY->baseSortOffset;
    sColumnCount = 0;
    for ( sTmp = aCNBY->baseSortNode; sTmp != NULL ; sTmp = sTmp->next )
    {
        sColumnCount++;
    }
    sCurOffset += idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sColumnCount;

    aCNBY->mSortNodeOffset = sCurOffset;
    sColumnCount = 0;
    for ( sTmp = aCNBY->sortNode; sTmp != NULL ; sTmp = sTmp->next )
    {
        sColumnCount++;
    }
    // 다음 노드가 저장될 시작 지점
    sCurOffset += idlOS::align8(ID_SIZEOF(qmdMtrNode)) * sColumnCount;

    aCNBY->sortMTROffset = sCurOffset;
    sCurOffset += idlOS::align8(ID_SIZEOF(qmcdSortTemp));
    aCNBY->mBaseSortMTROffset = sCurOffset;
    sCurOffset += idlOS::align8(ID_SIZEOF(qmcdSortTemp));

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sCurOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_LOB_FILTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMO_NOT_ALLOWED_LOB_FILTER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmoOneMtrPlan::changeNodeForCNBYJoin( qtcNode * aNode,
                                           UShort    aSrcTable,
                                           UShort    aSrcColumn,
                                           UShort    aDstTable,
                                           UShort    aDstColumn )
{
    if ( ( aNode->node.module == &qtc::columnModule ) &&
         ( QTC_IS_PSEUDO( aNode ) == ID_FALSE ) )
    {
        if ( ( aNode->node.baseTable == aSrcTable ) &&
             ( aNode->node.baseColumn == aSrcColumn ) )
        {
            aNode->node.table = aDstTable;
            aNode->node.column = aDstColumn;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.arguments != NULL )
    {
        changeNodeForCNBYJoin( (qtcNode *)aNode->node.arguments,
                               aSrcTable,
                               aSrcColumn,
                               aDstTable,
                               aDstColumn );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.next != NULL )
    {
        changeNodeForCNBYJoin( (qtcNode *)aNode->node.next,
                               aSrcTable,
                               aSrcColumn,
                               aDstTable,
                               aDstColumn );
    }
    else
    {
        /* Nothing to do */
    }
}

