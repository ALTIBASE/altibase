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
 * $Id: qmnLeftOuter.cpp 90785 2021-05-06 07:26:22Z hykim $
 *
 * Description :
 *     LOJN(Left Outer JoiN) Node
 *
 *     관계형 모델에서 Left Outer Join를 수행하는 Plan Node 이다.
 *     다양한 Join Method들은 하위 노드의 형태에 따라 결정된다.
 *  
 *     다음과 같은 기능을 위해 사용된다.
 *         - Nested Loop Join 계열
 *         - Sort-based Join 계열
 *         - Hash-based Join 계열
 *         - Full Outer Join의 Anti-Outer 최적화 적용시
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
#include <qmnHash.h>
#include <qmnLeftOuter.h>
#include <qcg.h>


IDE_RC 
qmnLOJN::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    LOJN 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnLOJN::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncLOJN * sCodePlan = (qmncLOJN *) aPlan;
    qmndLOJN * sDataPlan = 
        (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnLOJN::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_LOJN_INIT_DONE_MASK)
         == QMND_LOJN_INIT_DONE_FALSE )
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
    if ( ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
         == QMN_PLAN_JOIN_METHOD_INVERSE_HASH )
    {
        sDataPlan->doIt = qmnLOJN::doItInverseLeft;
    }
    else
    {
        sDataPlan->doIt = qmnLOJN::doItLeft;
    }

    // PROJ-2750
    sDataPlan->mSkipRightCnt = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnLOJN::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    LOJN의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnLOJN::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncLOJN * sCodePlan = (qmncLOJN *) aPlan;
    qmndLOJN * sDataPlan = 
        (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC 
qmnLOJN::padNull( qcTemplate * aTemplate,
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

#define IDE_FN "qmnLOJN::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncLOJN * sCodePlan = (qmncLOJN *) aPlan;
    // qmndLOJN * sDataPlan = 
    //     (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_LOJN_INIT_DONE_MASK)
         == QMND_LOJN_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
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
qmnLOJN::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnLOJN::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncLOJN * sCodePlan = (qmncLOJN*) aPlan;
    qmndLOJN * sDataPlan = 
       (qmndLOJN*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    ULong i;

    //----------------------------
    // Display 위치 결정
    //----------------------------
    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //----------------------------
    // LOJN 노드 표시
    //----------------------------
    iduVarStringAppend( aString,
                        "LEFT-OUTER-JOIN" );

    /* PROJ-2339 */
    if ( ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
        == QMN_PLAN_JOIN_METHOD_INVERSE_HASH )
    {
        iduVarStringAppend( aString, " INVERSE ( " ); // add 'INVERSE' keyword
    }
    else
    {
        iduVarStringAppend( aString, " ( " );
    }

    //----------------------------
    // Join Method 출력
    //----------------------------
    qmn::printJoinMethod( aString, sCodePlan->plan.flag );

    //----------------------------
    // PROJ-2750 Skip Right Count 출력
    //----------------------------
    printSkipRightCnt( aTemplate,
                       aString,
                       sCodePlan->flag,
                       sDataPlan->mSkipRightCnt,
                       aMode );

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
qmnLOJN::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnLOJN::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT(0);
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnLOJN::doItLeft( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     새로운 Left Row에 대한 처리
 *
 * Implementation :
 *     JOIN과 달리 Left에 대응하는 Right Row가 존재하지 않을 경우
 *     Null Padding하여 결과를 구성한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnLOJN::doItLeft"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncLOJN * sCodePlan = (qmncLOJN *) aPlan;
    qmndLOJN * sDataPlan =
        (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sFlag = QMC_ROW_DATA_NONE;
    idBool sJudge;

    //------------------------------------
    // Read Left Row
    //------------------------------------

    // PROJ-2750
    *aFlag &= ~QMC_ROW_NULL_PADDING_MASK;
    *aFlag |=  QMC_ROW_NULL_PADDING_FALSE;

    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
              != IDE_SUCCESS );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // PROJ-2750
        if ( ( ( sCodePlan->flag & QMNC_LOJN_SKIP_RIGHT_COND_MASK ) == QMNC_LOJN_SKIP_RIGHT_COND_TRUE ) &&
             ( ( *aFlag & QMC_ROW_NULL_PADDING_MASK ) == QMC_ROW_NULL_PADDING_TRUE ) )
        {
            IDE_TEST( aPlan->right->init( aTemplate, aPlan->right ) != IDE_SUCCESS );
            IDE_TEST( aPlan->right->padNull( aTemplate, aPlan->right ) != IDE_SUCCESS );
            sDataPlan->mSkipRightCnt++;
            sDataPlan->doIt = qmnLOJN::doItLeft;
        }
        else
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
            // Right Row가 없을 경우 Null Padding
            //------------------------------------

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                // PROJ-2750
                *aFlag |= QMC_ROW_NULL_PADDING_FALSE;
                sDataPlan->doIt = qmnLOJN::doItRight;
            }
            else
            {
                IDE_TEST( aPlan->right->padNull( aTemplate, aPlan->right )
                          != IDE_SUCCESS );

                // PROJ-2750
                *aFlag |= QMC_ROW_NULL_PADDING_TRUE;
                sDataPlan->doIt = qmnLOJN::doItLeft;
            }
        }
    }
    else
    {
        // 더 이상 결과가 없음
        // PROJ-2750
        *aFlag |= QMC_ROW_NULL_PADDING_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnLOJN::doItRight( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     새로운 Right Row에 대한 처리
 *
 * Implementation :
 *     Filter 조건을 만족할 때까지 반복 수행
 *
 ***********************************************************************/

#define IDE_FN "qmnLOJN::doItRight"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncLOJN * sCodePlan = (qmncLOJN *) aPlan;
    qmndLOJN * sDataPlan = 
        (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

    idBool sJudge;
    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;

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

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;
    }
    else
    {
        // 질의 결과가 없을 경우 새로운 Left Row를 이용한 처리
        IDE_TEST( qmnLOJN::doItLeft( aTemplate, aPlan, aFlag )
                  != IDE_SUCCESS );
        if ( (*aFlag & QMC_ROW_DATA_MASK ) != QMC_ROW_DATA_EXIST )
        {
            sDataPlan->doIt = qmnLOJN::doItLeft;
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qmnLOJN::doItInverseLeft( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     새로운 Left Row에 대한 처리
 *
 * Implementation :
 *     JOIN과 달리 Left에 대응하는 Right Row가 존재하지 않을 경우
 *     Null Padding하여 결과를 구성한다.
 *
 ***********************************************************************/

    qmncLOJN * sCodePlan = (qmncLOJN *) aPlan;
    qmndLOJN * sDataPlan = 
        (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sLeftFlag = QMC_ROW_DATA_NONE;
    qmcRowFlag sRightFlag = QMC_ROW_DATA_NONE;
    idBool sJudge;

    //------------------------------------
    // Left Row를 모두 탐색
    //------------------------------------
    
    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
              != IDE_SUCCESS );

    while ( (sLeftFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //------------------------------------
        // Read Right Row
        //------------------------------------

        // To Fix PR-9822
        // Right는 Left가 변할 때마다 초기화해 주어야 한다.
        IDE_TEST( aPlan->right->init( aTemplate, 
                                      aPlan->right ) != IDE_SUCCESS);
        
        IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, & sRightFlag )
                  != IDE_SUCCESS );

        //------------------------------------
        // Filter 조건을 만족할 때까지 반복
        //------------------------------------
        
        while ( (sRightFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
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
                                              & sRightFlag ) != IDE_SUCCESS );
            }
        }

        //------------------------------------
        // Return this Row / Search Next Left Row
        //------------------------------------
        
        if ( (sRightFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( sDataPlan->setHitFlag( aTemplate, aPlan->right )
                      != IDE_SUCCESS );

            sDataPlan->doIt = qmnLOJN::doItInverseRight;

            *aFlag &= ~QMC_ROW_DATA_MASK;
            *aFlag |= QMC_ROW_DATA_EXIST;

            break;
        }
        else
        {
            // Matched right record is not found.
            // Search the hashtable with next left one.
            IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
                      != IDE_SUCCESS );

        }
    }

    // Non-Hit Phase
    if ( (sLeftFlag & QMC_ROW_DATA_MASK) != QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmnLOJN::doItInverseNonHitFirst( aTemplate, aPlan, aFlag ) 
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do : return this record
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnLOJN::doItInverseRight( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan,
                                  qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Right Row를 계속 처리
 *
 * Implementation :
 *     Right Row가 더 존재하면 Join 결과로 구성한다.
 *
 ***********************************************************************/

    qmncLOJN * sCodePlan = (qmncLOJN *) aPlan;
    qmndLOJN * sDataPlan = 
        (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

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
        IDE_TEST( qmnLOJN::doItInverseLeft( aTemplate, aPlan, aFlag ) 
                  != IDE_SUCCESS );

        if ( (*aFlag & QMC_ROW_DATA_MASK ) != QMC_ROW_DATA_EXIST )
        {
            // It goes to endpoint.
            sDataPlan->doIt = qmnLOJN::doItInverseLeft;
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

IDE_RC qmnLOJN::doItInverseNonHitFirst( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan,
                                        qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Left Outer Inverse Join 후처리 과정으로 최초 수행 함수
 *
 * Implementation :
 *    Child Plan의 검색 모드를 변경한다.
 *    Hit Flag이 없는 Child Row를 획득하게 된다.
 *
 ***********************************************************************/

    qmndLOJN * sDataPlan = 
        (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

    // To Fix PR-9822
    // Right는 Left가 없을 경우도 초기화해 주어야 한다.
    IDE_TEST( aPlan->right->init( aTemplate, 
                                  aPlan->right ) != IDE_SUCCESS);
    
    //------------------------------------
    // Right에 대하여 Non-Hit 검색 모드로 변경
    //------------------------------------
    qmnHASH::setNonHitSearch( aTemplate, aPlan->right );

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
        sDataPlan->doIt = qmnLOJN::doItInverseNonHitNext;

        // PROJ-2750
        *aFlag |= QMC_ROW_NULL_PADDING_TRUE;
    }
    else
    {
        // 결과 없음
        sDataPlan->doIt = qmnLOJN::doItInverseLeft;

        // PROJ-2750
        *aFlag |= QMC_ROW_NULL_PADDING_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmnLOJN::doItInverseNonHitNext( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Left Outer Inverse Join 후처리 과정으로 다음 수행 함수
 *
 * Implementation :
 *    Child Plan의 검색 모드를 변경한다.
 *    Hit Flag이 없는 Child Row를 획득하게 된다.
 *
 ***********************************************************************/

    qmndLOJN * sDataPlan = 
        (qmndLOJN *) (aTemplate->tmplate.data + aPlan->offset);

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

        // PROJ-2750
        *aFlag |= QMC_ROW_NULL_PADDING_TRUE;
    }
    else
    {
        // 결과 없음
        sDataPlan->doIt = qmnLOJN::doItInverseLeft;

        // PROJ-2750
        *aFlag |= QMC_ROW_NULL_PADDING_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC 
qmnLOJN::firstInit( qmncLOJN   * aCodePlan,
                    qmndLOJN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Data 영역에 대한 초기화
 * Implementation :
 *
 ***********************************************************************/

    if ( ( aCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
        == QMN_PLAN_JOIN_METHOD_INVERSE_HASH )
    {   
        IDE_DASSERT( aCodePlan->plan.right->type == QMN_HASH );
        aDataPlan->setHitFlag = qmnHASH::setHitFlag;
    }   
    else
    {
        // Nothing to do.   
    } 

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_LOJN_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_LOJN_INIT_DONE_TRUE;

    return IDE_SUCCESS;
}

void qmnLOJN::printSkipRightCnt( qcTemplate   * aTemplate,
                                 iduVarString * aString,
                                 UInt           aFlag,
                                 UInt           aSkipRightCnt,
                                 qmnDisplay     aMode )
{
/***********************************************************************
 * Description : PROJ-2750 Skip Right Count 를 출력한다.
 ***********************************************************************/

    if ( ( aMode == QMN_DISPLAY_ALL ) &&
         ( QCG_GET_SESSION_TRCLOG_DETAIL_INFORMATION( aTemplate->stmt ) == 1 ) &&
         ( ( aFlag & QMNC_LOJN_SKIP_RIGHT_COND_MASK ) == QMNC_LOJN_SKIP_RIGHT_COND_TRUE ) &&
         ( aSkipRightCnt > 0 ) )
    {
        iduVarStringAppendFormat( aString, ", SKIP RIGHT COUNT: %"ID_INT32_FMT"", aSkipRightCnt );
    }
}
