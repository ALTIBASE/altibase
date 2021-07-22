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
 * $Id: qmnAggregation.cpp 85261 2019-04-17 01:26:11Z andrew.shin $
 *
 * Description :
 *     AGGR(AGGRegation) Node
 *
 *     관계형 모델에서 다음과 같은 기능을 수행하는 Plan Node 이다.
 *
 *         - Sort-based Grouping
 *         - Distinct Aggregation
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnAggregation.h>

IDE_RC
qmnAGGR::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    AGGR 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncAGGR * sCodePlan = (qmncAGGR *) aPlan;
    qmndAGGR * sDataPlan =
        (qmndAGGR *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnAGGR::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_AGGR_INIT_DONE_MASK)
         == QMND_AGGR_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    sDataPlan->mtrRowIdx = 0;

    // init left child
    IDE_TEST( aPlan->left->init( aTemplate,
                                 aPlan->left ) != IDE_SUCCESS);

    if ( (sCodePlan->flag & QMNC_AGGR_GROUPED_MASK )
         == QMNC_AGGR_GROUPED_TRUE )
    {
        sDataPlan->doIt = qmnAGGR::doItGroupAggregation;
    }
    else
    {
        sDataPlan->doIt = qmnAGGR::doItAggregation;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnAGGR::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    AGGR의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndAGGR * sDataPlan =
        (qmndAGGR *) (aTemplate->tmplate.data + aPlan->offset);

    // 저장 위치를 변경한다.
    sDataPlan->mtrRowIdx++;
    sDataPlan->plan.myTuple->row = sDataPlan->mtrRow[sDataPlan->mtrRowIdx % 2];

    // 해당 함수를 수행한다.
    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    Child에 대하여 padNull()을 호출하고
 *    AGGR 노드의 null row를 setting한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncAGGR * sCodePlan = (qmncAGGR *) aPlan;
    qmndAGGR * sDataPlan =
        (qmndAGGR *) (aTemplate->tmplate.data + aPlan->offset);

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_AGGR_INIT_DONE_MASK)
         == QMND_AGGR_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }

    // Child Plan에 대하여 Null Padding수행
    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    // AGGR 노드의 Null Row설정
    sDataPlan->plan.myTuple->row = sDataPlan->nullRow;

    // Null Padding도 record가 변한 것임
    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    AGGR 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncAGGR * sCodePlan = (qmncAGGR*) aPlan;
    qmndAGGR * sDataPlan =
        (qmndAGGR*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    UInt  sMtrRowIdx = 0;
    ULong i;

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    if ( aMode == QMN_DISPLAY_ALL )
    {
        if ( (*sDataPlan->flag & QMND_AGGR_INIT_DONE_MASK)
             == QMND_AGGR_INIT_DONE_TRUE )
        {
            /* PROJ-2448 Subquery caching */
            sMtrRowIdx = (sDataPlan->mtrRowIdx > 0)? sDataPlan->mtrRowIdx - 1: sDataPlan->mtrRowIdx;

            if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
            {
                iduVarStringAppendFormat( aString,
                                          "AGGREGATION ( "
                                          "ITEM_SIZE: %"ID_UINT32_FMT", "
                                          "GROUP_COUNT: %"ID_UINT32_FMT,
                                          sDataPlan->mtrRowSize,
                                          sMtrRowIdx );
            }
            else
            {
                // BUG-29209
                // ITEM_SIZE 정보 보여주지 않음
                iduVarStringAppendFormat( aString,
                                          "AGGREGATION ( "
                                          "ITEM_SIZE: BLOCKED, "
                                          "GROUP_COUNT: %"ID_UINT32_FMT,
                                          sMtrRowIdx );
            }
        }
        else
        {
            iduVarStringAppend( aString,
                                "AGGREGATION "
                                "( ITEM_SIZE: 0, GROUP_COUNT: 0" );
        }
    }
    else
    {
        iduVarStringAppend( aString,
                            "AGGREGATION "
                            "( ITEM_SIZE: ??, GROUP_COUNT: ??" );
    }

    //----------------------------
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    //----------------------------
    // PROJ-1473 mtrNode info 
    //----------------------------

    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        qmn::printMTRinfo( aString,
                           aDepth,
                           sCodePlan->myNode,
                           "myNode",
                           sCodePlan->myNode->dstNode->node.table,
                           ID_USHORT_MAX,
                           ID_USHORT_MAX);

        qmn::printMTRinfo( aString,
                           aDepth,
                           sCodePlan->distNode,
                           "distNode",
                           sCodePlan->myNode->dstNode->node.table,
                           ID_USHORT_MAX,
                           ID_USHORT_MAX);
    }

    //----------------------------
    // Operator별 결과 정보 출력
    //----------------------------
    if ( QCU_TRCLOG_RESULT_DESC == 1 )
    {
        IDE_TEST( qmn::printResult( aTemplate,
                                    aDepth,
                                    aString,
                                    sCodePlan->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------
    // Child Plan 정보 출력
    //----------------------------

    IDE_TEST( aPlan->left->printPlan( aTemplate,
                                      aPlan->left,
                                      aDepth + 1,
                                      aString,
                                      aMode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::doItDefault( qcTemplate * /* aTemplate */,
                      qmnPlan    * /* aPlan */,
                      qmcRowFlag * /* aFlag */ )
{
/***********************************************************************
 *
 * Description :
 *    이 함수가 수행되면 안됨.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::doItAggregation( qcTemplate * aTemplate,
                          qmnPlan    * aPlan,
                          qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     One-Group Aggregation을 수행
 *
 * Implementation :
 *     Child가 없을 때까지 반복 수행하여 그 결과를 Return한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::doItAggregation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndAGGR * sDataPlan =
        (qmndAGGR *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;

    // doIt left child
    IDE_TEST( aPlan->left->doIt( aTemplate,
                                 aPlan->left,
                                 & sFlag ) != IDE_SUCCESS);

    // 초기화 수행
    IDE_TEST( clearDistNode( sDataPlan )
              != IDE_SUCCESS);

    IDE_TEST( initAggregation(aTemplate, sDataPlan)
              != IDE_SUCCESS);

    // Record가 있을 때까지 Aggregation 수행
    while( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( execAggregation( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        // doIt left child
        IDE_TEST( aPlan->left->doIt( aTemplate,
                                     aPlan->left,
                                     & sFlag ) != IDE_SUCCESS);
    }

    // Aggregation을 마무리
    IDE_TEST( finiAggregation( aTemplate, sDataPlan )
              != IDE_SUCCESS );

    /* BUG-46906 */
    IDE_TEST( setTupleSet( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

    *aFlag = QMC_ROW_DATA_EXIST;

    // 최종적으로 Data 없음을 리턴하기 위함
    sDataPlan->doIt = qmnAGGR::doItLast;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::doItGroupAggregation( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Multi-Group Aggregatino을 최초 수행
 *
 * Implementation :
 *     동일 Group인 동안 Child를 반복 수행하여 Aggregation을 한다.
 *     다른 Group이 올라온 경우
 *        새로운 Group에 대한 초기화를 수행
 *        현재 Group에 대한 마무리를 수행하고 리턴
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::doItGroupAggregation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndAGGR        * sDataPlan =
        (qmndAGGR *) (aTemplate->tmplate.data + aPlan->offset);
    qmcRowFlag        sFlag     = QMC_ROW_INITIALIZE;

    //------------------------------------
    // 최초 Group에 대한 Aggregation 수행
    //------------------------------------

    IDE_TEST( aPlan->left->doIt( aTemplate,
                                 aPlan->left,
                                 & sFlag ) != IDE_SUCCESS);

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //--------------------------------
        // 최초 Group에 대한 초기화
        //--------------------------------

        IDE_TEST( clearDistNode( sDataPlan )
                  != IDE_SUCCESS);

        IDE_TEST( initAggregation(aTemplate, sDataPlan)
                  != IDE_SUCCESS);

        IDE_TEST( execAggregation( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        // 동일 Group에 대한 반복 수행
        IDE_TEST( aPlan->left->doIt( aTemplate,
                                     aPlan->left,
                                     & sFlag ) != IDE_SUCCESS);

        while( (sFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_SAME )
        {
            IDE_TEST( execAggregation( aTemplate, sDataPlan )
                      != IDE_SUCCESS );

            IDE_TEST( aPlan->left->doIt( aTemplate,
                                         aPlan->left,
                                         & sFlag ) != IDE_SUCCESS);
        }

        if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
        {
            //--------------------------------
            // Record 가 더이상 없는 경우
            //--------------------------------

            sDataPlan->doIt = qmnAGGR::doItLast;

            /* BUG-46906 */
            // 현재 Group에 대한 마무리 수행
            IDE_TEST( finiAggregation( aTemplate, sDataPlan )
                      != IDE_SUCCESS );
        }
        else
        {
            /* BUG-46906 */
            // 현재 Group에 대한 마무리 수행
            IDE_TEST( finiAggregation( aTemplate, sDataPlan )
                      != IDE_SUCCESS );

            //--------------------------------
            // 다른 Group이 올라온 경우
            //--------------------------------

            // 새로운 Group에 대한 처리
            IDE_TEST( setNewGroup( aTemplate, sDataPlan )
                      != IDE_SUCCESS );

            // 현재 Group의 위치로 돌리고 다음 수행 함수를 설정
            sDataPlan->plan.myTuple->row =
                sDataPlan->mtrRow[sDataPlan->mtrRowIdx % 2];

            sDataPlan->doIt = qmnAGGR::doItNext;
        }

        /* BUG-46906 */
        IDE_TEST( setTupleSet( aTemplate, sDataPlan )
                   != IDE_SUCCESS );

        *aFlag = QMC_ROW_DATA_EXIST;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Multi-Group Aggregation의 다음 수행
 *
 * Implementation :
 *    현재 Group 정보로 Tuple Set을 구성
 *    동일 Group일동안 반복하여 Aggregation을 수행
 *    다른 Group이 올라온 경우
 *        새로운 Group에 대한 초기화를 수행
 *        현재 Group에 대한 마무리를 수행하고 리턴
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndAGGR * sDataPlan =
        (qmndAGGR *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;

    // To fix PR-4355
    // 호출 시점에는 이전 Group에 대한 Tuple Set 정보가 설정되어 있다.
    // 따라서, 현재 Group에 대한 Tuple Set정보로 변경한다.
    IDE_TEST( setTupleSet( aTemplate, sDataPlan )
              != IDE_SUCCESS );

    //-----------------------------
    // 동일 Group에 대한 반복 수행
    //-----------------------------

    IDE_TEST( aPlan->left->doIt( aTemplate,
                                 aPlan->left,
                                 & sFlag ) != IDE_SUCCESS);

    while( (sFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_SAME )
    {
        IDE_TEST( execAggregation( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        IDE_TEST( aPlan->left->doIt( aTemplate,
                                     aPlan->left,
                                     & sFlag ) != IDE_SUCCESS);
    }

    //-----------------------------
    // 현재 Group에 대한 마무리
    //-----------------------------

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        // 더 이상 Record가 없는 경우
        sDataPlan->doIt = qmnAGGR::doItLast;

        /* BUG-46906 */
        // 현재 Group에 대한 마무리 수행
        IDE_TEST( finiAggregation( aTemplate, sDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        /* BUG-46906 */
        // 현재 Group에 대한 마무리 수행
        IDE_TEST( finiAggregation( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        // 다른 Group이 존재하는 경우
        // 다른 Group에 대한 초기화를 수행한다.

        // 새로운 Group에 대한 처리
        IDE_TEST( setNewGroup( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        // 현재 Group의 위치 설정
        sDataPlan->plan.myTuple->row = sDataPlan->mtrRow[sDataPlan->mtrRowIdx % 2];
    }

    /* BUG-46906 */
    IDE_TEST( setTupleSet( aTemplate, sDataPlan )
              != IDE_SUCCESS );

    *aFlag = QMC_ROW_DATA_EXIST;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::doItLast( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Record 없음을 리턴한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::doItLast"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncAGGR * sCodePlan = (qmncAGGR *) aPlan;
    qmndAGGR * sDataPlan =
        (qmndAGGR *) (aTemplate->tmplate.data + aPlan->offset);

    // set that no data found
    *aFlag = QMC_ROW_DATA_NONE;

    // BUG-44041 저장위치를 조정합니다.
    sDataPlan->plan.myTuple->row = sDataPlan->mtrRow[(sDataPlan->mtrRowIdx-1) % 2];

    if ( (sCodePlan->flag & QMNC_AGGR_GROUPED_MASK )
         == QMNC_AGGR_GROUPED_TRUE )
    {
        sDataPlan->doIt = qmnAGGR::doItGroupAggregation;
    }
    else
    {
        sDataPlan->doIt = qmnAGGR::doItAggregation;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}


IDE_RC
qmnAGGR::firstInit( qcTemplate * aTemplate,
                    qmncAGGR   * aCodePlan,
                    qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    AGGR node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    //---------------------------------
    // AGGR 고유 정보의 초기화
    //---------------------------------

    // 1. 저장 Column의 초기화
    // 2. Distinct Column정보의 초기화
    // 3. Aggregation Column 정보의 초기화
    // 4. Grouping Column 정보의 위치 지정
    // 5. Tuple 위치 지정

    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    if ( aCodePlan->distNode != NULL )
    {
        IDE_TEST( initDistNode( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        aDataPlan->distNodeCnt = 0;
        aDataPlan->distNode = NULL;
    }

    if ( aDataPlan->aggrNodeCnt > 0 )
    {
        IDE_TEST( initAggrNode( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        aDataPlan->aggrNode = NULL;
    }

    if ( (aCodePlan->flag & QMNC_AGGR_GROUPED_MASK)
         == QMNC_AGGR_GROUPED_TRUE )
    {
        IDE_TEST( initGroupNode( aCodePlan, aDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        aDataPlan->groupNode = NULL;
    }

    aDataPlan->plan.myTuple = aDataPlan->mtrNode->dstTuple;

    //---------------------------------
    // 서로 다른 Group을 위한 자료 구조의 초기화
    //---------------------------------

    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    IDE_TEST( allocMtrRow( aTemplate, aDataPlan )
              != IDE_SUCCESS );

    IDE_TEST( makeNullRow( aTemplate, aDataPlan )
              != IDE_SUCCESS );

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_AGGR_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_AGGR_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::initMtrNode( qcTemplate * aTemplate,
                      qmncAGGR   * aCodePlan,
                      qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column의 정보를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::initMtrNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcMtrNode * sNode;
    UShort       i;    

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aCodePlan->mtrNodeOffset > 0 );

    // Distinct Column을 Disk Temp Table을 사용하던 Memory Temp Table을
    // 사용하던, Row 저장을 위한 Tuple Set의 정보는 Memory Storage여야 한다.
    // Distinct Column을  Disk에 저장할 경우, Plan의 flag정보는 DISK
    // 각 Distinct Column을 위한 Tuple Set의 Storage 역시 Disk Type이 된다.
    IDE_DASSERT(
        ( aTemplate->tmplate.rows[aCodePlan->myNode->dstNode->node.table].lflag
          & MTC_TUPLE_STORAGE_MASK ) == MTC_TUPLE_STORAGE_MEMORY );

    //----------------------------------
    // Aggregation 영역의 구분
    //----------------------------------

    for( i = 0, sNode = aCodePlan->myNode;
         i < aCodePlan->baseTableCount;
         sNode = sNode->next, i++ )
    {
        // Nothing To Do 
    }    

    for ( sNode = sNode,
              aDataPlan->aggrNodeCnt  = 0;
          sNode != NULL;
          sNode = sNode->next )
    {
        if ( ( sNode->flag & QMC_MTR_GROUPING_MASK )
             == QMC_MTR_GROUPING_FALSE )
        {
            // aggregation node should not use converted source node
            //   e.g) SUM( MIN(I1) )
            //        MIN(I1)'s converted node is not aggregation node
            aDataPlan->aggrNodeCnt++;
        }
        else
        {
            break;
        }
    }

    //---------------------------------
    // 저장 Column의 초기화
    //---------------------------------

    aDataPlan->mtrNode =
        (qmdMtrNode*) (aTemplate->tmplate.data + aCodePlan->mtrNodeOffset);

    // 저장 Column의 연결 정보 생성
    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->mtrNode ) != IDE_SUCCESS );

    // 저장 Column의 초기화
    // Aggregation의 저장 시 Conversion값을 저장해서는 안됨
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                (UShort)aCodePlan->baseTableCount + aDataPlan->aggrNodeCnt )
              != IDE_SUCCESS );

    // 저장 Column의 offset을 재조정.
    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  0 ) // 별도의 header가 필요 없음
              != IDE_SUCCESS );

    // Row Size의 계산
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::initDistNode( qcTemplate * aTemplate,
                       qmncAGGR   * aCodePlan,
                       qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Distinct Argument를 관리하기 위한 Distinct 정보를 설정한다.
 *
 * Implementation :
 *    다른 저장 Column과 달리 Distinct Argument Column정보는
 *    서로 간의 연결 정보를 유지하지 않는다.
 *    이는 각 Column정보는 별도의 Tuple을 사용하며, 서로 간의 연관
 *    관계를 갖지 않을 뿐더러 Hash Temp Table의 수정 없이 쉽게
 *    사용하기 위해서이다.
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::initDistNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcMtrNode  * sCNode;
    qmdDistNode * sDistNode;
    UInt          sFlag;
    UInt          sHeaderSize;
    UInt          i;

    // 적합성 검사.
    IDE_DASSERT( aCodePlan->distNodeOffset > 0 );

    //------------------------------------------------------
    // Distinct 저장 Column의 기본 정보 구성
    // Distinct Node는 개별적으로 저장 공간을 갖고 처리되며,
    // 따라서 Distinct Node간에 연결 정보를 생성하지 않는다.
    //------------------------------------------------------

    aDataPlan->distNode =
        (qmdDistNode *) (aTemplate->tmplate.data + aCodePlan->distNodeOffset);

    for( sCNode = aCodePlan->distNode, sDistNode = aDataPlan->distNode,
             aDataPlan->distNodeCnt = 0;
         sCNode != NULL;
         sCNode = sCNode->next, sDistNode++,
             aDataPlan->distNodeCnt++ )
    {
        sDistNode->myNode = sCNode;
        sDistNode->srcNode = NULL;
        sDistNode->next = NULL;
    }

    //------------------------------------------------------------
    // [Hash Temp Table을 위한 정보 정의]
    // AGGR 노드의 Row와 달리 Distinct Column은 저장 매체가
    // Memory 또는 Disk일 수 있다.  이 정보는 plan.flag을 이용하여
    // 판별하며, 해당 distinct column을 저장하기 위한 Tuple Set또한
    // 동일한 저장 매체를 사용하고 있어야 한다.
    // 이에 대한 적합성 검사는 Hash Temp Table에서 검사하게 된다.
    //------------------------------------------------------------

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sFlag =
            QMCD_HASH_TMP_STORAGE_MEMORY |
            QMCD_HASH_TMP_DISTINCT_TRUE | QMCD_HASH_TMP_PRIMARY_TRUE;

        sHeaderSize = QMC_MEMHASH_TEMPHEADER_SIZE;
    }
    else
    {
        sFlag =
            QMCD_HASH_TMP_STORAGE_DISK |
            QMCD_HASH_TMP_DISTINCT_TRUE | QMCD_HASH_TMP_PRIMARY_TRUE;

        sHeaderSize = QMC_DISKHASH_TEMPHEADER_SIZE;
    }

    // PROJ-2553
    // DISTINCT Hashing은 Bucket List Hashing 방법을 써야 한다.
    sFlag &= ~QMCD_HASH_TMP_HASHING_TYPE;
    sFlag |= QMCD_HASH_TMP_HASHING_BUCKET;

    //----------------------------------------------------------
    // 개별 Distinct 저장 Column의 초기화
    //----------------------------------------------------------

    for ( i = 0, sDistNode = aDataPlan->distNode;
          i < aDataPlan->distNodeCnt;
          i++, sDistNode++ )
    {
        //---------------------------------------------------
        // 1. Dist Column의 구성 정보 초기화
        // 2. Dist Column의 offset재조정
        // 3. Disk Temp Table을 사용하는 경우 memory 공간을 할당받으며,
        //    Dist Node는 이 정보를 계속 유지하여야 한다.
        //    Memory Temp Table을 사용하는 경우 별도의 공간을 할당 받지
        //    않는다.
        // 4. Dist Column을 위한 Hash Temp Table을 초기화한다.
        //---------------------------------------------------

        IDE_TEST( qmc::initMtrNode( aTemplate,
                                    (qmdMtrNode*) sDistNode,
                                    0 )
                  != IDE_SUCCESS );

        IDE_TEST( qmc::refineOffsets( (qmdMtrNode*) sDistNode,
                                      sHeaderSize )
                  != IDE_SUCCESS );

        IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                   & aTemplate->tmplate,
                                   sDistNode->dstNode->node.table )
                  != IDE_SUCCESS );

        // Disk Temp Table을 사용하는 경우라면
        // 이 공간을 잃지 않도록 해야 한다.
        sDistNode->mtrRow = sDistNode->dstTuple->row;
        sDistNode->isDistinct = ID_TRUE;

        IDE_TEST( qmcHashTemp::init( & sDistNode->hashMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     (qmdMtrNode*) sDistNode,  // 저장 대상
                                     (qmdMtrNode*) sDistNode,  // 비교 대상
                                     NULL,
                                     sDistNode->myNode->bucketCnt,
                                     sFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::initAggrNode( qcTemplate * aTemplate,
                       qmncAGGR   * aCodePlan,
                       qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column의 초기화
 *
 * Implementation :
 *    Aggregation Column을 초기화하고,
 *    Distinct Aggregation인 경우 해당 Distinct Node를 찾아 연결한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::initAggrNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;

    qmdAggrNode * sAggrNode;
    qmdDistNode * sDistNode;

    //-----------------------------------------------
    // 적합성 검사
    //-----------------------------------------------

    IDE_DASSERT( aCodePlan->aggrNodeOffset > 0 );

    aDataPlan->aggrNode =
        (qmdAggrNode*) (aTemplate->tmplate.data + aCodePlan->aggrNodeOffset);

    //-----------------------------------------------
    // Aggregation Node의 연결 정보를 설정하고 초기화
    //-----------------------------------------------

    IDE_TEST( linkAggrNode( aCodePlan,
                            aDataPlan ) != IDE_SUCCESS );

    // aggregation node should not use converted source node
    //   e.g) SUM( MIN(I1) )
    //        MIN(I1)'s converted node is not aggregation node
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                (qmdMtrNode*) aDataPlan->aggrNode,
                                (UShort)aDataPlan->aggrNodeCnt )
              != IDE_SUCCESS );

    // Aggregation Column 의 offset을 재조정
    IDE_TEST( qmc::refineOffsets( (qmdMtrNode*) aDataPlan->aggrNode,
                                  0 ) // 별도의 header가 필요 없음
              != IDE_SUCCESS );

    //-----------------------------------------------
    // Distinct Aggregation의 경우 해당 Distinct Node를
    // 찾아 연결한다.
    //-----------------------------------------------

    for ( sAggrNode = aDataPlan->aggrNode;
          sAggrNode != NULL;
          sAggrNode = sAggrNode->next )
    {
        if ( sAggrNode->myNode->myDist != NULL )
        {
            // Distinct Aggregation인 경우
            for ( i = 0, sDistNode = aDataPlan->distNode;
                  i < aDataPlan->distNodeCnt;
                  i++, sDistNode++ )
            {
                if ( sDistNode->myNode == sAggrNode->myNode->myDist )
                {
                    sAggrNode->myDist = sDistNode;
                    break;
                }
            }
        }
        else
        {
            // 일반 Aggregation인 경우
            sAggrNode->myDist = NULL;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::linkAggrNode( qmncAGGR   * aCodePlan,
                       qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Code Materialized Node 영역으로부터 Aggregation Node영역만을
 *    추출하여 연결한다.
 *
 * Implementation :
 *    Code 영역의 Materialized Node의 구성은 다음과 같다.
 *
 *    <----- Aggregation 영역 ---->|<----- Grouping 영역 ------>
 *    [SUM]----->[AVG]----->[MAX]----->[i1]----->[i2]
 *
 *    Aggregation영역의 끝은 GROUPING 영역의 시작 전까지가 된다.
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::linkAggrNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    qmcMtrNode  * sNode;
    qmdAggrNode * sAggrNode;
    qmdAggrNode * sPrevNode;

    for( i = 0, sNode = aCodePlan->myNode;
         i < aCodePlan->baseTableCount;
         sNode = sNode->next, i++ )
    {
        // Nothing To Do 
    }

    for ( i = 0,
              sNode = sNode,
              sAggrNode = aDataPlan->aggrNode,
              sPrevNode = NULL;
          i < aDataPlan->aggrNodeCnt;
          i++, sNode = sNode->next, sAggrNode++ )
    {
        sAggrNode->myNode = sNode;
        sAggrNode->srcNode = NULL;
        sAggrNode->next = NULL;

        if ( sPrevNode != NULL )
        {
            sPrevNode->next = sAggrNode;
        }

        sPrevNode = sAggrNode;
    }

    return IDE_SUCCESS;

    // IDE_EXCEPTION_END;

    // return IDE_FAIULRE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::initGroupNode( qmncAGGR   * aCodePlan,
                        qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::initGroupNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    qmdMtrNode * sMtrNode;

    for ( i = 0, sMtrNode = aDataPlan->mtrNode;
          i < aCodePlan->baseTableCount + aDataPlan->aggrNodeCnt;
          i++, sMtrNode = sMtrNode->next ) ;

    aDataPlan->groupNode = sMtrNode;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnAGGR::allocMtrRow( qcTemplate * aTemplate,
                      qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::allocMtrRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduMemory * sMemory;

    sMemory = aTemplate->stmt->qmxMem;

    //-------------------------------------------
    // 두 Row의 저장를 위한 공간 할당
    //-------------------------------------------

    IDU_FIT_POINT_RAISE( "qmnAGGR::allocMtrRow::cralloc::DataPlan_mtrRow0",
                          err_mem_alloc );
    IDE_TEST( sMemory->cralloc( aDataPlan->mtrRowSize,
                                (void**)&(aDataPlan->mtrRow[0]))
              != IDE_SUCCESS);
    IDE_TEST_RAISE( aDataPlan->mtrRow[0] == NULL, err_mem_alloc );

    IDU_FIT_POINT_RAISE( "qmnAGGR::allocMtrRow::cralloc::DataPlan_mtrRow1", 
                          err_mem_alloc );
    IDE_TEST( sMemory->cralloc( aDataPlan->mtrRowSize,
                                (void**)&(aDataPlan->mtrRow[1]))
              != IDE_SUCCESS);
    IDE_TEST_RAISE( aDataPlan->mtrRow[1] == NULL, err_mem_alloc );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_mem_alloc );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MEMORY_ALLOCATION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::makeNullRow(qcTemplate * aTemplate,
                     qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::makeNullRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;
    iduMemory  * sMemory;

    sMemory = aTemplate->stmt->qmxMem;

    //-------------------------------------------
    // Null Row를 위한 공간 할당
    //-------------------------------------------

    IDU_FIT_POINT_RAISE( "qmnAGGR::makeNullRow::cralloc::DataPlan_nullRow", 
                          err_mem_alloc );
    IDE_TEST( sMemory->cralloc( aDataPlan->mtrRowSize,
                                (void**)&(aDataPlan->nullRow))
              != IDE_SUCCESS);
    IDE_TEST_RAISE( aDataPlan->nullRow == NULL, err_mem_alloc );

    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        //-----------------------------------------------
        // 실제 값을 저장하는 Column에 대해서만
        // NULL Value를 생성한다.
        //-----------------------------------------------

        sNode->func.makeNull( sNode,
                              aDataPlan->nullRow );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_mem_alloc );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MEMORY_ALLOCATION));
    }
    IDE_EXCEPTION_END;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnAGGR::clearDistNode( qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Distinct Column의 구분을 위해 생성한
 *    Temp Table을 Clear한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::clearDistNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    qmdDistNode * sDistNode;

    for ( i = 0, sDistNode = aDataPlan->distNode;
          i < aDataPlan->distNodeCnt;
          i++, sDistNode++ )
    {
        IDE_TEST( qmcHashTemp::clear( & sDistNode->hashMgr )
                  != IDE_SUCCESS );
        sDistNode->mtrRow = sDistNode->dstTuple->row;
        sDistNode->isDistinct = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}



IDE_RC
qmnAGGR::initAggregation( qcTemplate * aTemplate,
                          qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Row를 구성한다.
 *    Aggregation Column은 초기화하고, Group Column을 구성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::initAggregation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdAggrNode * sNode;

    for ( sNode = aDataPlan->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( qtc::initialize( sNode->dstNode, aTemplate )
                  != IDE_SUCCESS );
    }

    IDE_TEST( setGroupColumns( aTemplate, aDataPlan )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::setGroupColumns( qcTemplate * aTemplate,
                          qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Grouping Column에 대한 저장을 수행
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::setGroupColumns"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    // idBool sExist;
    // sExist = ID_TRUE;

    for ( sNode = aDataPlan->groupNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST(
            sNode->func.setMtr( aTemplate,
                                sNode,
                                aDataPlan->mtrRow[aDataPlan->mtrRowIdx % 2] )
            != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::execAggregation( qcTemplate * aTemplate,
                          qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation을 수행한다.
 *
 * Implementation :
 *    Distinct Column에 대하여 수행하고,
 *    Distinct Aggregation인 경우 Distinction 여부에 따라,
 *    Aggregation 수행 여부를 판단한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::execAggregation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdAggrNode * sAggrNode;

    // BUG-42277
    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    // set distinct column and insert to hash(DISTINCT)
    IDE_TEST( setDistMtrColumns( aTemplate, aDataPlan )
              != IDE_SUCCESS );

    for ( sAggrNode = aDataPlan->aggrNode;
          sAggrNode != NULL;
          sAggrNode = sAggrNode->next )
    {
        if ( sAggrNode->myDist == NULL )
        {
            // Non Distinct Aggregation인 경우
            IDE_TEST( qtc::aggregate( sAggrNode->dstNode, aTemplate )
                      != IDE_SUCCESS );
        }
        else
        {
            // Distinct Aggregation인 경우
            if ( sAggrNode->myDist->isDistinct == ID_TRUE )
            {
                // Distinct Argument인 경우
                IDE_TEST( qtc::aggregate( sAggrNode->dstNode, aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                // Non-Distinct Argument인 경우
                // Aggregation을 수행하지 않는다.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::setDistMtrColumns( qcTemplate * aTemplate,
                            qmndAGGR * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Distinct Column을 구성한다.
 *
 * Implementation :
 *     Memory 공간을 할당 받고, Distinct Column을 구성
 *     Hash Temp Table에 삽입을 시도한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::setDistMtrColumns"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    qmdDistNode * sDistNode;

    for ( i = 0, sDistNode = aDataPlan->distNode;
          i < aDataPlan->distNodeCnt;
          i++, sDistNode++ )
    {
        if ( sDistNode->isDistinct == ID_TRUE )
        {
            // 새로운 메모리 공간을 할당
            // Memory Temp Table인 경우에만 새로운 공간을 할당받는다.
            IDE_TEST( qmcHashTemp::alloc( & sDistNode->hashMgr,
                                          & sDistNode->mtrRow )
                      != IDE_SUCCESS );

            sDistNode->dstTuple->row = sDistNode->mtrRow;
        }
        else
        {
            // To Fix PR-8556
            // 이전 메모리를 그대로 사용할 수 있는 경우
            sDistNode->mtrRow = sDistNode->dstTuple->row;
        }

        // Distinct Column을 구성
        IDE_TEST( sDistNode->func.setMtr( aTemplate,
                                          (qmdMtrNode*) sDistNode,
                                          sDistNode->mtrRow ) != IDE_SUCCESS );

        // Hash Temp Table에 삽입
        // Is Distinct의 결과로 삽입 성공 여부를 판단할 수 있다.
        IDE_TEST( qmcHashTemp::addDistRow( & sDistNode->hashMgr,
                                           & sDistNode->mtrRow,
                                           & sDistNode->isDistinct )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::finiAggregation( qcTemplate * aTemplate,
                          qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation을 마무리하고 결과를 Tuple Set에 Setting한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::finiAggregation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdAggrNode * sAggrNode;

    for ( sAggrNode = aDataPlan->aggrNode;
          sAggrNode != NULL;
          sAggrNode = sAggrNode->next )
    {
        IDE_TEST( qtc::finalize( sAggrNode->dstNode, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnAGGR::setTupleSet( qcTemplate * aTemplate,
                      qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    수행 결과를 Tuple Set에 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::setTupleSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST(
            sNode->func.setTuple( aTemplate,
                                  sNode,
                                  aDataPlan->mtrRow[aDataPlan->mtrRowIdx % 2] )
            != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnAGGR::setNewGroup( qcTemplate * aTemplate,
                      qmndAGGR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Aggregation 수행 중 새로운 그룹에 대한 초기화
 *
 * Implementation :
 *     새로운 Group에 대한 공간을 지정하고 초기화 수행
 *     이전  Group의 위치로 이동
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::setNewGroup"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // 새로운 Group의 위치로 이동
    aDataPlan->mtrRowIdx++;
    aDataPlan->plan.myTuple->row =
        aDataPlan->mtrRow[aDataPlan->mtrRowIdx % 2];

    // 새로운 Group에 대한 초기화 및 Aggregation 수행
    IDE_TEST( clearDistNode( aDataPlan )
              != IDE_SUCCESS);

    IDE_TEST( initAggregation(aTemplate, aDataPlan)
              != IDE_SUCCESS);

    IDE_TEST( execAggregation( aTemplate, aDataPlan )
              != IDE_SUCCESS );

    // 이전 Group으로 원복
    aDataPlan->mtrRowIdx--;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
