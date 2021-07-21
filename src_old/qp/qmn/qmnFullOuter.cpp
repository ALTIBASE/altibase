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
 * $Id: qmnFullOuter.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     FOJN(Full Outer JoiN) Node
 *
 *     관계형 모델에서 Full Outer Join를 수행하는 Plan Node 이다.
 *     다양한 Join Method들은 하위 노드의 형태에 따라 결정된다.
 *  
 *     다음과 같은 기능을 위해 사용된다.
 *         - Nested Loop Join 계열
 *         - Sort-based Join 계열
 *         - Hash-based Join 계열
 *
 *     Full Outer Join은 크게 다음과 같은 두 단계로 수행된다.
 *         - Left Outer Join Phase
 *             : Left Outer Join과 동일하나, 만족하는 Right Row에 대하여
 *               Hit Flag을 Setting하여 다음 Phase에 대한 처리를 준비한다.
 *         - Right Outer Join Phase
 *             : Right Row중 Hit 되지 않은 Row만 골라 Left에 대한
 *               Null Padding을 수행한다.
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
#include <qmnSort.h>
#include <qmnHash.h>
#include <qmnFullOuter.h>
#include <qcg.h>


IDE_RC 
qmnFOJN::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    FOJN 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncFOJN * sCodePlan = (qmncFOJN *) aPlan;
    qmndFOJN * sDataPlan = 
        (qmndFOJN *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    sDataPlan->doIt = qmnFOJN::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_FOJN_INIT_DONE_MASK)
         == QMND_FOJN_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //------------------------------------------------
    // Child Plan의 초기화
    //------------------------------------------------

    IDE_TEST( aPlan->left->init( aTemplate, 
                                 aPlan->left ) != IDE_SUCCESS);

    /*
     * PROJ-2402 Parallel Table Scan
     * parallel scan 의 경우
     * 필요한쪽에 먼저 thread 를 분배하기 위해
     * 오른쪽에 PRLQ, HASH(또는 SORT) 가 있을때
     * 오른쪽을 먼저 init 한다.
     */
    if (((aPlan->right->flag & QMN_PLAN_PRLQ_EXIST_MASK) ==
         QMN_PLAN_PRLQ_EXIST_TRUE) &&
        ((aPlan->right->flag & QMN_PLAN_MTR_EXIST_MASK) ==
         QMN_PLAN_MTR_EXIST_TRUE))
    {
        IDE_TEST(aPlan->right->init(aTemplate,
                                    aPlan->right)
                 != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    //------------------------------------------------
    // 수행 함수 결정
    //------------------------------------------------

    sDataPlan->doIt = qmnFOJN::doItLeftHitPhase;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC 
qmnFOJN::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    FOJN의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //  qmncFOJN * sCodePlan = (qmncFOJN *) aPlan;
    qmndFOJN * sDataPlan = 
        (qmndFOJN *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnFOJN::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    Null Padding을 수행한다.
 *
 * Implementation :
 *    별도의 Null Row를 가지지 않으며,
 *    Child에 대한 Null Padding을 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncFOJN * sCodePlan = (qmncFOJN *) aPlan;
    // qmndFOJN * sDataPlan = 
    //    (qmndFOJN *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_FOJN_INIT_DONE_MASK)
         == QMND_FOJN_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }

    //------------------------------------------------
    // Child 에 대한 Null Padding
    //------------------------------------------------
    
    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    IDE_TEST( aPlan->right->padNull( aTemplate, aPlan->right )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnFOJN::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *     수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncFOJN * sCodePlan = (qmncFOJN*) aPlan;
    qmndFOJN * sDataPlan = 
       (qmndFOJN*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    ULong  i;

    //----------------------------
    // Display 위치 결정
    //----------------------------
    
    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //----------------------------
    // FOJN 노드 표시
    //----------------------------
    
    iduVarStringAppend( aString,
                        "FULL-OUTER-JOIN ( " );

    //----------------------------
    // Join Method 출력
    //----------------------------
    qmn::printJoinMethod( aString, sCodePlan->plan.flag );

    //----------------------------
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    //----------------------------
    // Predicate 정보 표시
    //----------------------------
    if ( sCodePlan->filter != NULL )
    {
        if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
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
                                              sCodePlan->filter)
                != IDE_SUCCESS);
        }

        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->filter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
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

    IDE_TEST( aPlan->right->printPlan( aTemplate,
                                       aPlan->right,
                                       aDepth + 1,
                                       aString,
                                       aMode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC 
qmnFOJN::doItDefault( qcTemplate * /* aTemplate */,
                      qmnPlan    * /* aPlan */,
                      qmcRowFlag * /* aFlag */)
{
/***********************************************************************
 *
 * Description :
 *    이 함수가 수행되면 안됨.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT(0);

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnFOJN::doItLeftHitPhase( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Left Outer Join 처리 과정으로 새로운 Left Row에 대한 처리
 *
 * Implementation :
 *    LOJN과 달리 만족하는 Right Row 존재 시 Hit Flag을 설정한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::doItLeftHitPhase"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncFOJN * sCodePlan = (qmncFOJN *) aPlan;
    qmndFOJN * sDataPlan = 
        (qmndFOJN *) (aTemplate->tmplate.data + aPlan->offset);

    idBool sJudge;
    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;

    //------------------------------------
    // Read Left Row
    //------------------------------------
    
    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
              != IDE_SUCCESS );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //------------------------------------
        // Read Right Row
        //------------------------------------

        // To Fix PR-9822
        // Right는 Left가 변할 때마다 초기화해 주어야 한다.
        IDE_TEST( aPlan->right->init( aTemplate, 
                                      aPlan->right ) != IDE_SUCCESS);
        
        IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, & sFlag )
                  != IDE_SUCCESS );

        //------------------------------------
        // Filter 조건을 만족할 때까지 반복
        //------------------------------------
        
        while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            if ( sCodePlan->filter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge, sCodePlan->filter, aTemplate ) 
                          != IDE_SUCCESS );
            }
            else
            {
                sJudge = ID_TRUE;
            }

            if ( sJudge == ID_TRUE )
            {
                break;
            }
            else
            {
                IDE_TEST( aPlan->right->doIt( aTemplate, 
                                              aPlan->right, 
                                              & sFlag ) != IDE_SUCCESS );
            }
        }

        //------------------------------------
        // Right Row가 있을 경우 Hit Flag 설정
        // Right Row가 없을 경우 Null Padding
        //------------------------------------
        
        if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( sDataPlan->setHitFlag( aTemplate, aPlan->right )
                      != IDE_SUCCESS );
            
            sDataPlan->doIt = qmnFOJN::doItRightHitPhase;
        }
        else
        {
            IDE_TEST( aPlan->right->padNull( aTemplate, aPlan->right )
                      != IDE_SUCCESS );
            sDataPlan->doIt = qmnFOJN::doItLeftHitPhase;
        }
    }
    else
    {
        // 더 이상 결과가 없을 경우 Right Outer Join으로 변경
        IDE_TEST( doItFirstNonHitPhase( aTemplate, aPlan, aFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnFOJN::doItRightHitPhase( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Left Outer Join 처리 과정으로 새로운 Right Row에 대한 처리
 *
 * Implementation :
 *    LOJN과 달리 만족하는 Right Row 존재 시 Hit Flag을 설정한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::doItRightHitPhase"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncFOJN * sCodePlan = (qmncFOJN *) aPlan;
    qmndFOJN * sDataPlan = 
        (qmndFOJN *) (aTemplate->tmplate.data + aPlan->offset);

    idBool sJudge;
    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;

    //------------------------------------
    // Read Right Row
    //------------------------------------
    
    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, & sFlag )
              != IDE_SUCCESS );

    //------------------------------------
    // Filter 조건을 만족할 때까지 반복
    //------------------------------------
    
    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        if ( sCodePlan->filter != NULL )
        {
            IDE_TEST( qtc::judge( & sJudge, sCodePlan->filter, aTemplate ) 
                      != IDE_SUCCESS );
        }
        else
        {
            sJudge = ID_TRUE;
        }
        
        if ( sJudge == ID_TRUE )
        {
            break;
        }
        else
        {
            IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, & sFlag )
                      != IDE_SUCCESS );
        }
    }

    //------------------------------------
    // Right Row가 있을 경우 Hit Flag 설정
    //------------------------------------
    
    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( sDataPlan->setHitFlag( aTemplate, aPlan->right )
                  != IDE_SUCCESS );
        
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;
    }
    else
    {
        // 질의 결과가 없을 경우 새로운 Left Row를 이용한 처리
        IDE_TEST( qmnFOJN::doItLeftHitPhase( aTemplate, aPlan, aFlag ) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnFOJN::doItFirstNonHitPhase( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Right Outer Join 처리 과정으로 최초 수행 함수
 *
 * Implementation :
 *    Child Plan의 검색 모드를 변경한다.
 *    Hit Flag이 없는 Child Row를 획득하게 된다.
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::doItFirstNonHitPhase"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncFOJN * sCodePlan = (qmncFOJN *) aPlan;
    qmndFOJN * sDataPlan = 
        (qmndFOJN *) (aTemplate->tmplate.data + aPlan->offset);

    // To Fix PR-9822
    // Right는 Left가 없을 경우도 초기화해 주어야 한다.
    IDE_TEST( aPlan->right->init( aTemplate, 
                                  aPlan->right ) != IDE_SUCCESS);
    
    //------------------------------------
    // Right에 대하여 Non-Hit 검색 모드로 변경
    //------------------------------------
    
    if ( ( sCodePlan->flag & QMNC_FOJN_RIGHT_CHILD_MASK )
         == QMNC_FOJN_RIGHT_CHILD_HASH )
    {
        qmnHASH::setNonHitSearch( aTemplate, aPlan->right );
    }
    else
    {
        qmnSORT::setNonHitSearch( aTemplate, aPlan->right );
    }

    //------------------------------------
    // Read Right Row
    //------------------------------------
    
    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, aFlag )
              != IDE_SUCCESS );
    
    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        // Left Row에 대한 Null Padding
        IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
                  != IDE_SUCCESS );
        sDataPlan->doIt = qmnFOJN::doItNextNonHitPhase;
    }
    else
    {
        // 결과 없음
        sDataPlan->doIt = qmnFOJN::doItLeftHitPhase;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnFOJN::doItNextNonHitPhase( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Right Outer Join 처리 과정으로 최초 수행 함수
 *
 * Implementation :
 *    Hit Flag이 없는 Child Row를 획득하게 된다.
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::doItNextNonHitPhase"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncFOJN * sCodePlan = (qmncFOJN *) aPlan;
    qmndFOJN * sDataPlan = 
        (qmndFOJN *) (aTemplate->tmplate.data + aPlan->offset);

    //------------------------------------
    // Read Right Row
    //------------------------------------
    
    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, aFlag )
              != IDE_SUCCESS );
    
    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
                  != IDE_SUCCESS );
    }
    else
    {
        // 결과 없음
        sDataPlan->doIt = qmnFOJN::doItLeftHitPhase;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnFOJN::firstInit( qmncFOJN   * aCodePlan,
                    qmndFOJN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Data영역에 대한 초기화를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnFOJN::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // Hit Flag 함수 포인터 결정
    //---------------------------------

    if( (aCodePlan->flag & QMNC_FOJN_RIGHT_CHILD_MASK )
        == QMNC_FOJN_RIGHT_CHILD_HASH )
    {
        IDE_DASSERT( aCodePlan->plan.right->type == QMN_HASH );
        aDataPlan->setHitFlag = qmnHASH::setHitFlag;
    }
    else
    {
        IDE_DASSERT( aCodePlan->plan.right->type == QMN_SORT );
        aDataPlan->setHitFlag = qmnSORT::setHitFlag;
    }

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_FOJN_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_FOJN_INIT_DONE_TRUE;

    return IDE_SUCCESS;

#undef IDE_FN
}

