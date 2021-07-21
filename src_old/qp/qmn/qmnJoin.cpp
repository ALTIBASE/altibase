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
 * $Id: qmnJoin.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     JOIN(JOIN) Node
 *
 *     관계형 모델에서 cartesian product를 수행하는 Plan Node 이다.
 *     다양한 Join Method들은 하위 노드의 형태에 따라 결정된다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Cartesian Product
 *         - Nested Loop Join 계열
 *         - Sort-based Join 계열
 *         - Hash-based Join 계열
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnHash.h>
#include <qmnSort.h>
#include <qmnJoin.h>
#include <qcg.h>
#include <qmoUtil.h>

IDE_RC
qmnJOIN::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    JOIN 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnJOIN::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncJOIN * sCodePlan = (qmncJOIN *) aPlan;
    qmndJOIN * sDataPlan =
        (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    sDataPlan->doIt = qmnJOIN::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_JOIN_INIT_DONE_MASK)
         == QMND_JOIN_INIT_DONE_FALSE )
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

    switch( sCodePlan->flag & QMNC_JOIN_TYPE_MASK )
    {
        case QMNC_JOIN_TYPE_INNER:
            sDataPlan->doIt = qmnJOIN::doItLeft;
            break;
        case QMNC_JOIN_TYPE_SEMI:
            {
                switch ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
                {
                    // BUG-43950 INVERSE_HASH 도 doItInverse 를 사용한다.
                    case QMN_PLAN_JOIN_METHOD_INVERSE_HASH :
                    case QMN_PLAN_JOIN_METHOD_INVERSE_SORT :
                        sDataPlan->doIt = qmnJOIN::doItInverse;
                        break;
                    case QMN_PLAN_JOIN_METHOD_INVERSE_INDEX :
                        sDataPlan->doIt = qmnJOIN::doItLeft;
                        break;
                    default:
                        sDataPlan->doIt = qmnJOIN::doItSemi;
                }
                break;
            }
        case QMNC_JOIN_TYPE_ANTI:
            {
                switch ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
                {
                    case QMN_PLAN_JOIN_METHOD_INVERSE_HASH :
                    case QMN_PLAN_JOIN_METHOD_INVERSE_SORT :
                        sDataPlan->doIt = qmnJOIN::doItInverse;
                        break;
                    default:
                        sDataPlan->doIt = qmnJOIN::doItAnti;
                }
                break;
            }
        default:
            IDE_DASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnJOIN::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    JOIN의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnJOIN::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncJOIN * sCodePlan = (qmncJOIN *) aPlan;
    qmndJOIN * sDataPlan =
        (qmndJOIN*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnJOIN::padNull( qcTemplate * aTemplate,
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

#define IDE_FN "qmnJOIN::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncJOIN * sCodePlan = (qmncJOIN *) aPlan;
    // qmndJOIN * sDataPlan =
    //   (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_JOIN_INIT_DONE_MASK)
         == QMND_JOIN_INIT_DONE_FALSE )
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
qmnJOIN::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *     JOIN의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnJOIN::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncJOIN * sCodePlan = (qmncJOIN*) aPlan;
    qmndJOIN * sDataPlan =
        (qmndJOIN*) (aTemplate->tmplate.data + aPlan->offset);
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
    // JOIN 노드 표시
    //----------------------------

    switch( sCodePlan->flag & QMNC_JOIN_TYPE_MASK )
    {
        case QMNC_JOIN_TYPE_INNER:
            iduVarStringAppend( aString, "JOIN" );
            break;
        case QMNC_JOIN_TYPE_SEMI:
            iduVarStringAppend( aString, "SEMI-JOIN" );
            break;
        case QMNC_JOIN_TYPE_ANTI:
            iduVarStringAppend( aString, "ANTI-JOIN" );
            break;
        default:
            IDE_DASSERT( 0 );
    }

    /* PROJ-2339, 2385 */
    if ( ( ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
          == QMN_PLAN_JOIN_METHOD_INVERSE_INDEX ) ||
         ( ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
          == QMN_PLAN_JOIN_METHOD_INVERSE_HASH ) ||
         ( ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
          == QMN_PLAN_JOIN_METHOD_INVERSE_SORT ) )
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
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    // Filter 정보 출력
    if ( sCodePlan->filter != NULL)
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
        else
        {
            // Nothing To Do
        }

        // Subquery 정보 출력
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->filter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
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
qmnJOIN::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnJOIN::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT(0);

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnJOIN::doItLeft( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Left에서 한건, Right에서 한건 수행
 *
 * Implementation :
 *    - Left에서 한건 Fetch한 후 Right 추출
 *    - 조건에 맞는 Right 없을 경우 다시 Left 추출
 *    - Left가 없을 때까지 반복
 *
 ***********************************************************************/

#define IDE_FN "qmnJOIN::doItLeft"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncJOIN * sCodePlan = (qmncJOIN *) aPlan;
    qmndJOIN * sDataPlan =
        (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sFlag    = QMC_ROW_DATA_NONE;
    idBool     sRetry;

    //-------------------------------------
    // Left와 Right가 모두 있거나,
    // Left가 없을 때까지 반복 수행
    //-------------------------------------

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
                  != IDE_SUCCESS );

        if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            // To Fix PR-9822
            // Right는 Left가 변할 때마다 초기화해 주어야 한다.
            IDE_TEST( aPlan->right->init( aTemplate,
                                          aPlan->right ) != IDE_SUCCESS);

            do
            {
                IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, &sFlag )
                          != IDE_SUCCESS );

                IDE_TEST( checkFilter( aTemplate,
                                       sCodePlan,
                                       sFlag,
                                       &sRetry )
                          != IDE_SUCCESS );
            } while( sRetry == ID_TRUE );

        }
        else
        {
            // end of left child
            break;
        }
    }

    //-------------------------------------
    // 다음 수행 함수 결정
    //-------------------------------------

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // 다음 수행 시 Right만을 우선 검색
        sDataPlan->doIt = qmnJOIN::doItRight;
    }
    else
    {
        // 다음 수행 시 Left를 우선 검색
        sDataPlan->doIt = qmnJOIN::doItLeft;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnJOIN::doItRight( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    기존의 Left는 두고 Right에서 한건 수행
 *
 * Implementation :
 *    - Right를 추출
 *    - 존재하지 않는다면 Left부터 추출하도록 호출
 *
 ***********************************************************************/

#define IDE_FN "qmnJOIN::doItRight"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncJOIN * sCodePlan = (qmncJOIN *) aPlan;
    // qmndJOIN * sDataPlan =
    //     (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);
    idBool sRetry;

    do
    {
        IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, aFlag )
                  != IDE_SUCCESS );

        IDE_TEST( checkFilter( aTemplate,
                               sCodePlan,
                               *aFlag,
                               &sRetry )
                  != IDE_SUCCESS );
    } while( sRetry == ID_TRUE );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // nothing to do
    }
    else
    {
        IDE_TEST( qmnJOIN::doItLeft( aTemplate, aPlan, aFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnJOIN::doItSemi( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
    qmncJOIN   * sCodePlan = (qmncJOIN *) aPlan;
    qmcRowFlag   sLeftFlag  = QMC_ROW_DATA_NONE;
    qmcRowFlag   sRightFlag = QMC_ROW_DATA_NONE;
    idBool       sRetry;

    //-------------------------------------
    // Left와 right가 모두 있거나,
    // 또는 left가 없을 때까지 반복 수행
    //-------------------------------------

    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, &sLeftFlag )
              != IDE_SUCCESS );

    while( ( sLeftFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( aPlan->right->init( aTemplate,
                                      aPlan->right ) != IDE_SUCCESS);

        do
        {
            IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, &sRightFlag )
                      != IDE_SUCCESS );

            IDE_TEST( checkFilter( aTemplate,
                                   sCodePlan,
                                   sRightFlag,
                                   &sRetry )
                      != IDE_SUCCESS );
        } while( sRetry == ID_TRUE );

        if( ( sRightFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            break;
        }
        else
        {
            IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, &sLeftFlag )
                      != IDE_SUCCESS );
        }
    }

    // 최종 결과는 left 결과의 존재 유무
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= ( sLeftFlag & QMC_ROW_DATA_MASK );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnJOIN::doItAnti( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
    qmncJOIN   * sCodePlan = (qmncJOIN *) aPlan;
    qmcRowFlag   sLeftFlag  = QMC_ROW_DATA_NONE;
    qmcRowFlag   sRightFlag = QMC_ROW_DATA_NONE;
    idBool       sRetry;

    //-------------------------------------
    // Left만 있고 right가 없을 때까지,
    // 또는 left가 없을 때까지 반복 수행
    //-------------------------------------

    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, &sLeftFlag )
              != IDE_SUCCESS );

    while( ( sLeftFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( aPlan->right->init( aTemplate,
                                      aPlan->right ) != IDE_SUCCESS);

        do
        {
            IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, &sRightFlag )
                      != IDE_SUCCESS );

            IDE_TEST( checkFilter( aTemplate,
                                   sCodePlan,
                                   sRightFlag,
                                   &sRetry )
                      != IDE_SUCCESS );
        } while( sRetry == ID_TRUE );

        if( ( sRightFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, &sLeftFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            break;
        }
    }

    // 최종 결과는 left 결과의 존재 유무
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= ( sLeftFlag & QMC_ROW_DATA_MASK );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnJOIN::doItInverse( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Semi Join Inverse Sort + Anti Join Inverse (Hash + Sort) 에 대한 처리
 *
 * Implementation :
 *     일치하는 RIGHT Record에 Hit Flag를 작성하고,
 *     Hit/Non-Hit 된 RIGHT Record를 반환한다.
 *
 ***********************************************************************/

    qmncJOIN * sCodePlan = (qmncJOIN *) aPlan;
    qmndJOIN * sDataPlan = 
        (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);

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
        while ( ID_TRUE )
        {
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
            // Search Next Right Row / Return this Row
            //------------------------------------

            if ( (sRightFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( sDataPlan->setHitFlag( aTemplate, aPlan->right )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        }

        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
                  != IDE_SUCCESS );
    }

    IDE_DASSERT( (sLeftFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE );

    switch ( sCodePlan->flag & QMNC_JOIN_TYPE_MASK )
    {
        case QMNC_JOIN_TYPE_SEMI:
            IDE_TEST( qmnJOIN::doItInverseHitFirst( aTemplate, aPlan, aFlag ) 
                      != IDE_SUCCESS );
            break;
        case QMNC_JOIN_TYPE_ANTI:
            IDE_TEST( qmnJOIN::doItInverseNonHitFirst( aTemplate, aPlan, aFlag ) 
                      != IDE_SUCCESS );
            break;
        default:
            IDE_DASSERT(0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnJOIN::doItInverseHitFirst( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan,
                                     qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Semi Join Inverse Sort 에 대한 계속된 처리
 *
 * Implementation :
 *     Hit된 Record를 처음으로 검색한다
 *
 ***********************************************************************/
    qmncJOIN * sCodePlan = (qmncJOIN *) aPlan;
    qmndJOIN * sDataPlan = 
        (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( aPlan->right->init( aTemplate, 
                                  aPlan->right ) != IDE_SUCCESS);
    
    //------------------------------------
    // Right에 대하여 Hit 검색 모드로 변경
    //------------------------------------

    if ( ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
        == QMN_PLAN_JOIN_METHOD_INVERSE_HASH )
    {   
        IDE_DASSERT( sCodePlan->plan.right->type == QMN_HASH );
        qmnHASH::setHitSearch( aTemplate, aPlan->right );
    }   
    else
    {
        IDE_DASSERT( sCodePlan->plan.right->type == QMN_SORT );
        qmnSORT::setHitSearch( aTemplate, aPlan->right );
    }

    //------------------------------------
    // Read Right Row
    //------------------------------------
    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, aFlag )
              != IDE_SUCCESS );
    
    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        sDataPlan->doIt = qmnJOIN::doItInverseHitNext;
    }
    else
    {
        // 결과 없음
        sDataPlan->doIt = qmnJOIN::doItInverse;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmnJOIN::doItInverseHitNext( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan,
                                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Semi Join Inverse Sort 에 대한 계속된 처리
 *
 * Implementation :
 *     Hit된 Record를 계속해서 검색한다
 *
 ***********************************************************************/
    qmndJOIN* sDataPlan = 
        (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);

    //------------------------------------
    // Read Right Row (Continuously)
    //------------------------------------
    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, aFlag )
              != IDE_SUCCESS );
    
    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        // 결과 없음
        sDataPlan->doIt = qmnJOIN::doItInverse;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmnJOIN::doItInverseNonHitFirst( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan,
                                        qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Anti Join Inverse 에 대한 계속된 처리
 *
 * Implementation :
 *     Hit되지 않은 Record를 처음으로 검색한다
 *
 ***********************************************************************/
    qmncJOIN * sCodePlan = (qmncJOIN *) aPlan;
    qmndJOIN * sDataPlan = 
        (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( aPlan->right->init( aTemplate, 
                                  aPlan->right ) != IDE_SUCCESS);
    
    //------------------------------------
    // Right에 대하여 Non-Hit 검색 모드로 변경
    //------------------------------------

    if ( ( sCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
        == QMN_PLAN_JOIN_METHOD_INVERSE_HASH )
    {   
        IDE_DASSERT( sCodePlan->plan.right->type == QMN_HASH );
        qmnHASH::setNonHitSearch( aTemplate, aPlan->right );
    }   
    else
    {
        IDE_DASSERT( sCodePlan->plan.right->type == QMN_SORT );
        qmnSORT::setNonHitSearch( aTemplate, aPlan->right );
    }

    //------------------------------------
    // Read Right Row
    //------------------------------------
    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, aFlag )
              != IDE_SUCCESS );
    
    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        sDataPlan->doIt = qmnJOIN::doItInverseNonHitNext;
    }
    else
    {
        // 결과 없음
        sDataPlan->doIt = qmnJOIN::doItInverse;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmnJOIN::doItInverseNonHitNext( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Anti Join Inverse 에 대한 계속된 처리
 *
 * Implementation :
 *     Hit되지 않은 Record를 계속해서 검색한다
 *
 ***********************************************************************/
    qmndJOIN* sDataPlan = 
        (qmndJOIN *) (aTemplate->tmplate.data + aPlan->offset);

    //------------------------------------
    // Read Right Row (Continuously)
    //------------------------------------
    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, aFlag )
              != IDE_SUCCESS );
    
    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        // 결과 없음
        sDataPlan->doIt = qmnJOIN::doItInverse;
   }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmnJOIN::checkFilter( qcTemplate * aTemplate,
                      qmncJOIN   * aCodePlan,
                      UInt         aRightFlag,
                      idBool     * aRetry )
{
    idBool sJudge;

    if( ( ( aRightFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST ) &&
        ( aCodePlan->filter != NULL ) )
    {
        IDE_TEST( qtc::judge( &sJudge,
                              aCodePlan->filter,
                              aTemplate )
                  != IDE_SUCCESS );

        if( sJudge == ID_FALSE )
        {
            *aRetry = ID_TRUE;
        }
        else
        {
            *aRetry = ID_FALSE;
        }
    }
    else
    {
        *aRetry = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnJOIN::firstInit( qmncJOIN   * aCodePlan,
                    qmndJOIN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    JOIN node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

    //---------------------------------
    // Hit Flag 함수 포인터 결정
    //---------------------------------

    switch ( aCodePlan->plan.flag & QMN_PLAN_JOIN_METHOD_TYPE_MASK )
    {
        case QMN_PLAN_JOIN_METHOD_INVERSE_HASH :
            IDE_DASSERT( aCodePlan->plan.right->type == QMN_HASH );
            aDataPlan->setHitFlag   = qmnHASH::setHitFlag;
            aDataPlan->isHitFlagged = qmnHASH::isHitFlagged;
            break;
        case QMN_PLAN_JOIN_METHOD_INVERSE_SORT :
            IDE_DASSERT( aCodePlan->plan.right->type == QMN_SORT );
            aDataPlan->setHitFlag   = qmnSORT::setHitFlag;
            aDataPlan->isHitFlagged = qmnSORT::isHitFlagged;
            break;
        default :
            // Nothing to do.
            break;
    }
     
    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_JOIN_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_JOIN_INIT_DONE_TRUE;

    return IDE_SUCCESS;
}
