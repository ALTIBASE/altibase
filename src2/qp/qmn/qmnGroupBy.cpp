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
 * $Id: qmnGroupBy.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     GRBY(GRoup BY) Node
 *
 *     관계형 모델에서 다음과 같은 기능을 수행하는 Plan Node 이다.
 *
 *         - Sort-based Distinction
 *         - Sort-based Grouping
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnGroupBy.h>

IDE_RC
qmnGRBY::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    GRBY 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRBY * sCodePlan = (qmncGRBY *) aPlan;
    qmndGRBY * sDataPlan =
        (qmndGRBY *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnGRBY::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_GRBY_INIT_DONE_MASK)
         == QMND_GRBY_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }

    // init left child
    IDE_TEST( aPlan->left->init( aTemplate,
                                 aPlan->left ) != IDE_SUCCESS);

    sDataPlan->doIt = qmnGRBY::doItFirst;
    sDataPlan->mtrRowIdx = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnGRBY::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    GRBY의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnGRBY::doIt"));

    qmndGRBY * sDataPlan =
        (qmndGRBY*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRBY::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    Child에 대하여 padNull()을 호출하고
 *    GRBY 노드의 null row를 setting한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRBY * sCodePlan = (qmncGRBY *) aPlan;
    qmndGRBY * sDataPlan =
        (qmndGRBY *) (aTemplate->tmplate.data + aPlan->offset);

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_GRBY_INIT_DONE_MASK)
         == QMND_GRBY_INIT_DONE_FALSE )
    {
        // 초기화되지 않은 경우 초기화 수행
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // Child Plan에 대하여 Null Padding수행
    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    // GRBY 노드의 Null Row설정
    sDataPlan->plan.myTuple->row = sDataPlan->nullRow;

    // Null Padding도 record가 변한 것임
    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRBY::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    GRBY 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRBY * sCodePlan = (qmncGRBY*) aPlan;
    qmndGRBY * sDataPlan =
        (qmndGRBY*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    ULong i;

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    iduVarStringAppend( aString,
                        "GROUPING\n" );

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
                           ID_USHORT_MAX );
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
qmnGRBY::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnGRBY::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRBY::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    GRBY 의 최초 수행 함수
 *    Child를 수행하고 용도에 맞는 수행 함수를 결정한다.
 *    최초 선택된 Row는 반드시 상위 노드로 전달되기 때문에
 *    용도에 맞는 처리가 필요없다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRBY * sCodePlan = (qmncGRBY *) aPlan;
    qmndGRBY * sDataPlan =
        (qmndGRBY *) (aTemplate->tmplate.data + aPlan->offset);
    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;

    // Child를 수행
    IDE_TEST( sCodePlan->plan.left->doIt( aTemplate,
                                          sCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // 저장 Row를 생성.
        IDE_TEST( setMtrRow( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        *aFlag = sFlag;

        // 동일 Row가 아님을 표기
        *aFlag &= ~QMC_ROW_GROUP_MASK;
        *aFlag |= QMC_ROW_GROUP_NULL;

        // 상위에서 저장 Row를 사용할 수 있도록 Tuple Set에 설정
        IDE_TEST( setTupleSet( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        // 용도에 맞는 수행 함수를 결정한다.
        switch (sCodePlan->flag & QMNC_GRBY_METHOD_MASK)
        {
            case QMNC_GRBY_METHOD_DISTINCTION :
                {
                    sDataPlan->doIt = qmnGRBY::doItDistinct;
                    break;
                }
            case QMNC_GRBY_METHOD_GROUPING :
                {
                    sDataPlan->doIt = qmnGRBY::doItGroup;
                    break;
                }
            default :
                {
                    IDE_ASSERT( 0 );
                    break;
                }
        }
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }

    // 다음에 저장할 위치 지정
    sDataPlan->mtrRowIdx++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN

}

IDE_RC
qmnGRBY::doItGroup( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Sort-Based Grouping을 처리할 때, 수행되는 함수이다.
 *    동일 Group인지에 대한 판단을 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::doItGroup"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRBY * sCodePlan = (qmncGRBY *) aPlan;
    qmndGRBY * sDataPlan =
        (qmndGRBY *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;

    // Child 수행
    IDE_TEST( sCodePlan->plan.left->doIt( aTemplate,
                                          sCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // 저장 Row를 구성
        IDE_TEST( setMtrRow( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        *aFlag = sFlag;

        // 저장된 두 Row를 비교하여 동일 Group인지를 판단.
        if ( compareRows( sDataPlan ) == 0 )
        {
            *aFlag &= ~QMC_ROW_GROUP_MASK;
            *aFlag |= QMC_ROW_GROUP_SAME;
        }
        else
        {
            *aFlag &= ~QMC_ROW_GROUP_MASK;
            *aFlag |= QMC_ROW_GROUP_NULL;

            // 동일 Group이 아닌 경우
            // 상위에서 저장 Row를 사용할 수 있도록 Tuple Set에 설정
            IDE_TEST( setTupleSet( aTemplate, sDataPlan )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
        sDataPlan->doIt = qmnGRBY::doItFirst;
    }

    sDataPlan->mtrRowIdx++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRBY::doItDistinct( qcTemplate * aTemplate,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Sort-based Distinction을 위한 수행 함수
 *
 * Implementation :
 *    동일 Record인지를 판단하여 다른 Record일 경우에만,
 *    상위 Plan에 전달한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::doItDistinct"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( doItGroup( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    while ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        if ( (*aFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_NULL )
        {
            break;
        }
        IDE_TEST( doItGroup( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRBY::doItLast( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     TODO - A4 Grouping Set Integration
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::doItLast"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncGRBY * sCodePlan = (qmncGRBY *) aPlan;
    qmndGRBY * sDataPlan =
        (qmndGRBY *) (aTemplate->tmplate.data + aPlan->offset);

    // set that no data found
    *aFlag = QMC_ROW_DATA_NONE;

    sDataPlan->doIt = qmnGRBY::doItFirst;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnGRBY::firstInit( qcTemplate * aTemplate,
                    qmncGRBY   * aCodePlan,
                    qmndGRBY   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    GRBY node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // 적합성 검사
    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aCodePlan != NULL );
    IDE_DASSERT( aDataPlan != NULL );

    //---------------------------------
    // GRBY 고유 정보의 초기화
    //---------------------------------

    // 1. 저장 Column의 정보 초기화
    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );

    // 2. Group Column 위치 지정
    IDE_TEST( initGroupNode( aDataPlan )
              != IDE_SUCCESS );

    IDE_ASSERT( aDataPlan->mtrNode != NULL );
    
    // 3. Tuple정보의 설정
    aDataPlan->plan.myTuple = aDataPlan->mtrNode->dstTuple;

    //---------------------------------
    // 동일 Group 판단을 위한 자료 구조의 초기화
    //---------------------------------

    // 1. 저장 Column의 Row Size 계산
    // 적합성 검사
    IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                 == MTC_TUPLE_STORAGE_MEMORY );

    // Tuple의 Row Size 설정
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );

    // Row Size 획득
    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    // 2. 비교를 위한 공간 할당 및 Null Row 생성
    IDE_TEST( allocMtrRow( aTemplate, aDataPlan )
              != IDE_SUCCESS );

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_GRBY_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_GRBY_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRBY::initMtrNode( qcTemplate * aTemplate,
                      qmncGRBY   * aCodePlan,
                      qmndGRBY   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column의 정보를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::initMtrNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // 적합성 검사
    IDE_DASSERT( aCodePlan->mtrNodeOffset > 0 );

    // 저장 Column의 영역 획득
    aDataPlan->mtrNode =
        (qmdMtrNode*) (aTemplate->tmplate.data + aCodePlan->mtrNodeOffset);

    // 저장 Column의 연결 정보 생성
    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->mtrNode ) != IDE_SUCCESS );

    // 저장 Column의 초기화
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                aCodePlan->baseTableCount ) //basetable을관리하지않음
              != IDE_SUCCESS );

    // 저장 Column의 offset을 재조정.
    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  0) // 별도의 Header가 존재하지 않음
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRBY::initGroupNode( qmndGRBY   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column중에서 Group Node의 시작 위치를 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::initGroupNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        if ( (sNode->myNode->flag & QMC_MTR_GROUPING_MASK)
             == QMC_MTR_GROUPING_TRUE )
        {
            break;
        }
    }

    aDataPlan->groupNode = sNode;

    // 적합성 검사.
    IDE_DASSERT( aDataPlan->groupNode != NULL );

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnGRBY::allocMtrRow( qcTemplate * aTemplate,
                      qmndGRBY   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Row를 위한 공간을 할당받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::allocMtrRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;
    iduMemory  * sMemory;

    sMemory = aTemplate->stmt->qmxMem;

    //-------------------------------------------
    // 두 Row의 비교를 위한 공간 할당
    //-------------------------------------------

    IDE_TEST( sMemory->alloc( aDataPlan->mtrRowSize,
                              (void**)&(aDataPlan->mtrRow[0]))
              != IDE_SUCCESS);
    IDE_TEST_RAISE( aDataPlan->mtrRow[0] == NULL, err_mem_alloc );

    IDE_TEST( sMemory->alloc( aDataPlan->mtrRowSize,
                              (void**)&(aDataPlan->mtrRow[1]))
              != IDE_SUCCESS);
    IDE_TEST_RAISE( aDataPlan->mtrRow[1] == NULL, err_mem_alloc );

    //-------------------------------------------
    // Null Row를 위한 공간 할당
    //-------------------------------------------

    IDE_TEST( sMemory->alloc(aDataPlan->mtrRowSize,
                             (void**) & aDataPlan->nullRow )
              != IDE_SUCCESS);
    IDE_TEST_RAISE( aDataPlan->nullRow == NULL, err_mem_alloc );

    *(qmcRowFlag*)aDataPlan->nullRow = QMC_ROW_DATA_EXIST | QMC_ROW_VALUE_NULL;

    //-------------------------------------------
    // Null Row의 생성
    //-------------------------------------------

    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        //-----------------------------------------------
        // 실제 값을 저장하는 Column에 대해서만
        // NULL Value를 생성한다.
        //-----------------------------------------------

        sNode->func.makeNull( sNode, aDataPlan->nullRow );
    }

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
qmnGRBY::setMtrRow( qcTemplate * aTemplate,
                    qmndGRBY   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Row를 구성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::setMtrRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;
    // idBool       sExist;

    // 저장할 위치를 결정
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx % 2];

    // 저장 Row를 구성
    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST(
            sNode->func.setMtr( aTemplate,
                                sNode,
                                aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRBY::setTupleSet( qcTemplate * aTemplate,
                      qmndGRBY   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    상위에서 저장 Row를 사용할 수 있도록 Tuple Set에 설정
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRBY::setTupleSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;
    // idBool       sExist;

    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx % 2];

    // Base Table이 존재하지 않지만, Memory Column을 저장했을 경우의
    // 처리를 위해 Tuple Set을 원복시킨다.
    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setTuple( aTemplate, sNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

SInt
qmnGRBY::compareRows( qmndGRBY   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장한 두 Row간의 동일 여부를 판단한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmdMtrNode  * sNode;

    SInt          sResult = -1;
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    for( sNode = aDataPlan->groupNode;
         sNode != NULL;
         sNode = sNode->next )
    {
        sValueInfo1.value  = sNode->func.getRow( sNode, aDataPlan->mtrRow[0]);
        sValueInfo1.column = (const mtcColumn *)sNode->func.compareColumn;
        sValueInfo1.flag   = MTD_OFFSET_USE;
        
        sValueInfo2.value  = sNode->func.getRow( sNode, aDataPlan->mtrRow[1]);
        sValueInfo2.column = (const mtcColumn *)sNode->func.compareColumn;
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sResult = sNode->func.compare( &sValueInfo1, &sValueInfo2 );
        
        if( sResult != 0 )
        {
            break;
        }
    }

    return sResult;
}
