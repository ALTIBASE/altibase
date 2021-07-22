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
 * $Id: qmnCounter.cpp 20233 2007-08-06 01:58:21Z sungminee $
 *
 * Description :
 *     CNTR(CouNTeR) Node
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
#include <qmnCounter.h>
#include <qcg.h>

IDE_RC 
qmnCNTR::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    CNTR 노드의 초기화
 *
 * Implementation :
 *    - 최초 초기화가 되지 않은 경우 최초 초기화 수행
 *    - Child Plan에 대한 초기화
 *    - Stop Filter의 수행 결과 검사
 *    - Stop Filter의 결과에 따른 수행 함수 결정
 *
 ***********************************************************************/

#define IDE_FN "qmnCNTR::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnCNTR::init"));

    qmncCNTR * sCodePlan = (qmncCNTR*) aPlan;
    qmndCNTR * sDataPlan = 
        (qmndCNTR*) (aTemplate->tmplate.data + aPlan->offset);

    idBool sJudge;
    
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnCNTR::doItDefault;

    //------------------------------------------------
    // 최초 초기화 수행 여부 판단
    //------------------------------------------------

    if ( (*sDataPlan->flag & QMND_CNTR_INIT_DONE_MASK)
         == QMND_CNTR_INIT_DONE_FALSE )
    {
        // 최초 초기화 수행
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //--------------------------------
    // Rownum 초기화
    //--------------------------------

    sDataPlan->rownumValue = 1;
    *(sDataPlan->rownumPtr) = sDataPlan->rownumValue;

    //------------------------------------------------
    // Child Plan의 초기화
    //------------------------------------------------

    IDE_TEST( aPlan->left->init( aTemplate, 
                                 aPlan->left ) 
              != IDE_SUCCESS);

    //------------------------------------------------
    // Stop Filter를 수행
    //------------------------------------------------

    if ( sCodePlan->stopFilter != NULL )
    {
        IDE_TEST( qtc::judge( & sJudge,
                              sCodePlan->stopFilter,
                              aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        sJudge = ID_TRUE;
    }

    //------------------------------------------------
    // Stop Filter에 따른 수행 함수 결정
    //------------------------------------------------
    
    if ( sJudge == ID_TRUE )
    {
        //---------------------------------
        // Stop Filter를 만족하는 경우
        // 일반 수행 함수를 설정
        //---------------------------------

        sDataPlan->doIt = qmnCNTR::doItFirst;
    }
    else
    {
        //-------------------------------------------
        // Stop Filter를 만족하지 않는 경우
        // - 항상 조건을 만족하지 않으므로
        //   어떠한 결과도 리턴하지 않는 함수를 결정한다.
        //-------------------------------------------

        sDataPlan->doIt = qmnCNTR::doItAllFalse;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC 
qmnCNTR::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    CNTR의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnCNTR::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnCNTR::doIt"));

    qmndCNTR * sDataPlan = 
        (qmndCNTR*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnCNTR::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    CNTR 노드는 별도의 null row를 가지지 않으며,
 *    Child에 대하여 padNull()을 호출한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCNTR::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnCNTR::padNull"));

    qmncCNTR * sCodePlan = (qmncCNTR*) aPlan;
    // qmndCNTR * sDataPlan = 
    //     (qmndCNTR*) (aTemplate->tmplate.data + aPlan->offset);

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_CNTR_INIT_DONE_MASK)
         == QMND_CNTR_INIT_DONE_FALSE )
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
qmnCNTR::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    CNTR 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCNTR::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnCNTR::printPlan"));

    qmncCNTR * sCodePlan = (qmncCNTR*) aPlan;
    qmndCNTR * sDataPlan = 
        (qmndCNTR*) (aTemplate->tmplate.data + aPlan->offset);
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
    // CNTR 노드 표시
    //----------------------------

    if (sCodePlan->stopFilter != NULL)
    {
        iduVarStringAppend( aString,
                            "COUNTER STOPKEY\n" );
    }
    else
    {
        iduVarStringAppend( aString,
                            "COUNTER\n" );
    }

    //----------------------------
    // Predicate 정보의 상세 출력
    //----------------------------

    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {
        IDE_TEST( printPredicateInfo( aTemplate,
                                      sCodePlan,
                                      sDataPlan,
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
    //     1. Stop Filter
    //----------------------------

    // Normal Filter의 Subquery 정보 출력
    if ( sCodePlan->stopFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->stopFilter,
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
qmnCNTR::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnCNTR::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnCNTR::doItDefault"));

    IDE_DASSERT( 0 );
    
    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnCNTR::doItAllFalse( qcTemplate * /* aTemplate */,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Filter 조건을 만족하는 Record가 하나도 없는 경우 사용
 *
 *    Stop Filter 검사후에 결정되는 함수로 절대 만족하는
 *    Record가 존재하지 않는다.
 *
 * Implementation :
 *    항상 record 없음을 리턴한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnCNTR::doItAllFalse"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnCNTR::doItAllFalse"));

    qmncCNTR * sCodePlan = (qmncCNTR*) aPlan;

    // 적합성 검사
    IDE_DASSERT( sCodePlan->stopFilter != NULL );

    // 데이터 없음을 Setting
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= QMC_ROW_DATA_NONE;
    
    return IDE_SUCCESS;
    
#undef IDE_FN
}

IDE_RC 
qmnCNTR::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    CNTR의 수행 함수
 *    Child를 수행하고 Record가 있을 때까지 반복한다.
 * 
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCNTR::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncCNTR * sCodePlan = (qmncCNTR*) aPlan;
    qmndCNTR * sDataPlan = 
        (qmndCNTR*) (aTemplate->tmplate.data + aPlan->offset);

    // Child를 수행
    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag ) 
              != IDE_SUCCESS );
        
    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        sDataPlan->doIt = qmnCNTR::doItNext;
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

IDE_RC 
qmnCNTR::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    CNTR의 수행 함수
 *    Child를 수행하고 Record가 있을 때까지 반복한다.
 * 
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCNTR::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncCNTR * sCodePlan = (qmncCNTR*) aPlan;
    qmndCNTR * sDataPlan = 
        (qmndCNTR*) (aTemplate->tmplate.data + aPlan->offset);

    idBool sJudge = ID_FALSE;

    //--------------------------------
    // Rownum 증가
    //--------------------------------
    
    sDataPlan->rownumValue ++;
    *(sDataPlan->rownumPtr) = sDataPlan->rownumValue;
    
    //------------------------------------------------
    // Stop Filter를 수행
    //------------------------------------------------

    if ( sCodePlan->stopFilter != NULL )
    {
        IDE_TEST( qtc::judge( & sJudge,
                              sCodePlan->stopFilter,
                              aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        sJudge = ID_TRUE;
    }

    //------------------------------------------------
    // Stop Filter에 따른 수행 함수 결정
    //------------------------------------------------
    
    if ( sJudge == ID_TRUE )
    {
        // Child를 수행
        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag ) 
                  != IDE_SUCCESS );
    }
    else
    {
        // 만족하는 Record 없음
        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnCNTR::firstInit( qcTemplate * aTemplate,
                    qmncCNTR   * aCodePlan,
                    qmndCNTR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    CNTR node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCNTR::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnCNTR::firstInit"));

    //---------------------------------
    // CNTR 고유 정보의 초기화
    //---------------------------------

    aDataPlan->rownumTuple =
        & aTemplate->tmplate.rows[aCodePlan->rownumRowID];

    // Rownum 위치 설정
    aDataPlan->rownumPtr = (SLong*) aDataPlan->rownumTuple->row;
    aDataPlan->rownumValue = *(aDataPlan->rownumPtr);

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_CNTR_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_CNTR_INIT_DONE_TRUE;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnCNTR::printPredicateInfo( qcTemplate   * aTemplate,
                             qmncCNTR     * aCodePlan,
                             qmndCNTR     * /* aDataPlan */,
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

#define IDE_FN "qmnCNTR::printPredicateInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    
    // Stop Filter 출력
    if (aCodePlan->stopFilter != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ STOPKEY ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->stopFilter)
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

