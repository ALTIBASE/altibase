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
 * $Id: qmnFilter.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     FILT(FILTer) Node
 *
 *     관계형 모델에서 selection을 수행하는 Plan Node 이다.
 *     SCAN 노드와 달리 Storage Manager에 직접 접근하지 않고,
 *     이미 selection된 record에 대한 selection을 수행한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmoUtil.h>
#include <qmnFilter.h>
#include <qcg.h>

IDE_RC
qmnFILT::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    FILT 노드의 초기화
 *
 * Implementation :
 *    - 최초 초기화가 되지 않은 경우 최초 초기화 수행
 *    - Child Plan에 대한 초기화
 *    - Constant Filter의 수행 결과 검사
 *    - Constant Filter의 결과에 따른 수행 함수 결정
 *
 ***********************************************************************/

    qmncFILT * sCodePlan = (qmncFILT*) aPlan;
    qmndFILT * sDataPlan =
        (qmndFILT*) (aTemplate->tmplate.data + aPlan->offset);

    idBool sJudge;

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnFILT::doItDefault;

    //------------------------------------------------
    // 최초 초기화 수행 여부 판단
    //------------------------------------------------

    if ( (*sDataPlan->flag & QMND_FILT_INIT_DONE_MASK)
         == QMND_FILT_INIT_DONE_FALSE )
    {
        // 최초 초기화 수행
        IDE_TEST( firstInit(sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //------------------------------------------------
    // Constant Filter를 수행
    //------------------------------------------------

    if ( sCodePlan->constantFilter != NULL )
    {
        IDE_TEST( qtc::judge( & sJudge,
                              sCodePlan->constantFilter,
                              aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        sJudge = ID_TRUE;
    }

    //------------------------------------------------
    // Constant Filter에 따른 수행 함수 결정
    //------------------------------------------------

    /*
     * BUG-29551
     * constant filter 가 false 이면 하위 plan 을 수행 안할것이므로
     * init 조차 하지 않는다
     */
    if (sJudge == ID_TRUE)
    {
        /* Child Plan의 초기화 */
        IDE_TEST(aPlan->left->init(aTemplate, aPlan->left) != IDE_SUCCESS);

        /* Constant Filter를 만족하는 경우 일반 수행 함수를 설정 */
        sDataPlan->doIt = qmnFILT::doItFirst;
    }
    else
    {
        //-------------------------------------------
        // Constant Filter를 만족하지 않는 경우
        // - 항상 조건을 만족하지 않으므로
        //   어떠한 결과도 리턴하지 않는 함수를 결정한다.
        //-------------------------------------------
        sDataPlan->doIt = qmnFILT::doItAllFalse;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnFILT::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    FILT의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnFILT::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnFILT::doIt"));

    qmndFILT * sDataPlan =
        (qmndFILT*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnFILT::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    FILT 노드는 별도의 null row를 가지지 않으며,
 *    Child에 대하여 padNull()을 호출한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFILT::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnFILT::padNull"));

    qmncFILT * sCodePlan = (qmncFILT*) aPlan;
    // qmndFILT * sDataPlan =
    //     (qmndFILT*) (aTemplate->tmplate.data + aPlan->offset);

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_FILT_INIT_DONE_MASK)
         == QMND_FILT_INIT_DONE_FALSE )
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnFILT::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    FILT 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFILT::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnFILT::printPlan"));

    qmncFILT * sCodePlan = (qmncFILT*) aPlan;
    qmndFILT * sDataPlan =
        (qmndFILT*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    UInt  i;

    //----------------------------
    // Display 위치 결정
    //----------------------------

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //----------------------------
    // FILT 노드 표시
    //----------------------------

    iduVarStringAppend( aString,
                        "FILTER\n" );

    //----------------------------
    // Predicate 정보의 상세 출력
    //----------------------------

    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {
        IDE_TEST( printPredicateInfo( aTemplate,
                                      sCodePlan,
                                      aDepth,
                                      aString ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------
    // Subquery 정보의 출력
    // Subquery는 다음과 같은 predicate에만 존재할 수 있다.
    //     1. Constant Filter
    //     2. Normal Filter
    //----------------------------

    // To Fix PR-8030
    // Constant Filter의 Subquery 정보 출력
    if ( sCodePlan->constantFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->constantFilter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // Normal Filter의 Subquery 정보 출력
    if ( sCodePlan->filter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->filter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
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
    // Child Plan의 정보 출력
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
qmnFILT::doItDefault( qcTemplate * /* aTemplate */,
                      qmnPlan    * /* aPlan */,
                      qmcRowFlag * /* Flag */ )
{
/***********************************************************************
 *
 * Description :
 *    이 함수가 수행되면 안됨.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFILT::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnFILT::doItDefault"));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnFILT::doItAllFalse( qcTemplate * /* aTemplate */,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Filter 조건을 만족하는 Record가 하나도 없는 경우 사용
 *
 *    Constant Filter 검사후에 결정되는 함수로 절대 만족하는
 *    Record가 존재하지 않는다.
 *
 * Implementation :
 *    항상 record 없음을 리턴한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnFILT::doItAllFalse"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnFILT::doItAllFalse"));

    qmncFILT * sCodePlan = (qmncFILT*) aPlan;

    // 적합성 검사
    IDE_DASSERT( sCodePlan->constantFilter != NULL );

    // 데이터 없음을 Setting
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= QMC_ROW_DATA_NONE;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnFILT::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    FILT의 수행 함수
 *    Child를 수행하고 Record가 있을 경우 조건을 검사한다.
 *    조건을 만족할 때끼지 이를 반복한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFILT::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncFILT * sCodePlan = (qmncFILT*) aPlan;
    // qmndFILT * sDataPlan =
    //     (qmndFILT*) (aTemplate->tmplate.data + aPlan->offset);

    idBool sJudge = ID_FALSE;

    while ( sJudge == ID_FALSE )
    {
        // Child를 수행
        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
                  != IDE_SUCCESS );

        if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
        {
            // Record가 없는 경우
            break;
        }
        else
        {
            // Record가 있을 경우 조건 검사.

            // fix BUG-10417
            // sCodePlan->filter가 NULL인 경우의 고려가 필요하며,
            // sCodePlan->filter == NULL 이면, sJudge = ID_TRUE로 설정.
            if( sCodePlan->filter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge, sCodePlan->filter, aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                sJudge = ID_TRUE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnFILT::firstInit( qmncFILT   * aCodePlan,
                    qmndFILT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    FILT node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFILT::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnFILT::firstInit"));

    // 적합성 검사
    IDE_DASSERT( aCodePlan->constantFilter != NULL ||
                 aCodePlan->filter != NULL );

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_FILT_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_FILT_INIT_DONE_TRUE;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnFILT::printPredicateInfo( qcTemplate   * aTemplate,
                             qmncFILT     * aCodePlan,
                             ULong          aDepth,
                             iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *    Predicate의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFILT::printPredicateInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;

    // Constant Filter 출력
    if (aCodePlan->constantFilter != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ CONSTANT FILTER ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->constantFilter)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }


    // Normal Filter 출력
    if (aCodePlan->filter != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ FILTER ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->filter)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
