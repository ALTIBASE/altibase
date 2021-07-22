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
 * $Id: qmnMergeJoin.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     MGJN(MerGe JoiN) Node
 *
 *     관계형 모델에서 Merge Join 연산을 수행하는 Plan Node 이다.
 *
 *     Join Method중 Merge Join만을 담당하며, Left및 Right Child로는
 *     다음과 같은 Plan Node만이 존재할 수 있다.
 *
 *         - SCAN Node
 *         - SORT Node
 *         - MGJN Node
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcuProperty.h>
#include <qmoUtil.h>
#include <qmnScan.h>
#include <qmnPartitionCoord.h>
#include <qmnSort.h>
#include <qmnMergeJoin.h>
#include <qcg.h>

extern mtfModule mtfLessThan;
extern mtfModule mtfLessEqual;

IDE_RC
qmnMGJN::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    MGJN 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;
    qmndMGJN * sDataPlan =
        (qmndMGJN *) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    sDataPlan->doIt = qmnMGJN::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_MGJN_INIT_DONE_MASK)
         == QMND_MGJN_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
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

    IDE_TEST( aPlan->right->init( aTemplate,
                                  aPlan->right ) != IDE_SUCCESS);

    //------------------------------------------------
    // 저장 Cursor가 없음을 표기
    //------------------------------------------------

    *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
    *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;

    //------------------------------------------------
    // 수행 함수 결정
    //------------------------------------------------

    sDataPlan->doIt = qmnMGJN::doItFirst;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMGJN::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;

    switch( sCodePlan->flag & QMNC_MGJN_TYPE_MASK )
    {
        case QMNC_MGJN_TYPE_INNER:
            IDE_TEST( doItInner( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );
            break;
        case QMNC_MGJN_TYPE_SEMI:
            IDE_TEST( doItSemi( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );
            break;
        case QMNC_MGJN_TYPE_ANTI:
            IDE_TEST( doItAnti( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );
            break;
        default:
            IDE_ERROR( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnMGJN::doItInner( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MGJN의 고유 기능을 수행한다.
 *
 * Implementation :
 *    1.지정된 함수 포인터를 수행한다.
 *    2.row가 존재하는경우 filter조건을 검사한다.
 *    2.1.row가 존재하지 않는 경우 완료.
 *    3.filter조건을 검사한다.
 *    3.1 filter조건이 맞는 경우 완료.
 *    3.2 filter조건이 맞지 않는 경우 1로
 ***********************************************************************/

    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;
    qmndMGJN * sDataPlan =
        (qmndMGJN *) (aTemplate->tmplate.data + aPlan->offset);

    idBool     sJudge = ID_FALSE;

    while( sJudge == ID_FALSE )
    {
        // Merge Join Predicate 조건 검사후 맞는 레코드를 올림
        IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

        // 만약 맞는 레코드가 올라오면
        if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            // 
            // Join Filter 조건 검사
            IDE_TEST( checkJoinFilter( aTemplate,
                                       sCodePlan,
                                       & sJudge )
                      != IDE_SUCCESS );

            // Join Filter조건에 맞으면 sJudge는 TRUE가 되고 루프를 빠져나옴
        }
        else
        {
            // 맞는 결과가 없으면 break
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnMGJN::padNull( qcTemplate * aTemplate,
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

#define IDE_FN "qmnMGJN::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;
    // qmndMGJN * sDataPlan =
    //     (qmndMGJN *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_MGJN_INIT_DONE_MASK)
         == QMND_MGJN_INIT_DONE_FALSE )
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
qmnMGJN::printPlan( qcTemplate   * aTemplate,
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

#define IDE_FN "qmnMGJN::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;
    qmndMGJN * sDataPlan =
        (qmndMGJN *) (aTemplate->tmplate.data + aPlan->offset);
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
    // MGJN 노드 표시
    //----------------------------

    switch( sCodePlan->flag & QMNC_MGJN_TYPE_MASK )
    {
        case QMNC_MGJN_TYPE_INNER:
            (void) iduVarStringAppend( aString, "MERGE-JOIN ( " );
            break;
        case QMNC_MGJN_TYPE_SEMI:
            (void) iduVarStringAppend( aString, "SEMI-MERGE-JOIN ( " );
            break;
        case QMNC_MGJN_TYPE_ANTI:
            (void) iduVarStringAppend( aString, "ANTI-MERGE-JOIN ( " );
            break;
        default:
            IDE_DASSERT( 0 );
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

    //----------------------------
    // Predicate 정보의 상세 출력
    //----------------------------

    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {
        // Key Range 정보 출력
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ VARIABLE KEY ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          sCodePlan->mergeJoinPred )
                 != IDE_SUCCESS);

        // Filter 정보 출력
        if ( sCodePlan->joinFilter != NULL)
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
                                              sCodePlan->joinFilter)
                     != IDE_SUCCESS);
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

    //----------------------------
    // Subquery 정보의 출력
    // Subquery는 다음과 같은 predicate에만 존재할 수 있다.
    //     1. Merge Join Predicate
    //     2. Join Filter
    //----------------------------

    IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                      sCodePlan->mergeJoinPred,
                                      aDepth,
                                      aString,
                                      aMode ) != IDE_SUCCESS );

    if ( sCodePlan->joinFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->joinFilter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------
    // PROJ-1473 mtrNode info 
    //----------------------------

    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        qmn::printMTRinfo( aString,
                           aDepth,
                           sCodePlan->myNode,
                           "myNode",
                           ID_USHORT_MAX,
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
qmnMGJN::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnMGJN::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMGJN::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    최초 수행 함수
 *
 * Implementation :
 *     Left Child 또는 Right Child의 결과가 모두 있는 경우에만 수행
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;
    qmndMGJN * sDataPlan =
        (qmndMGJN *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;

    //----------------------------
    // Left Child의 수행
    //----------------------------

    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sFlag )
              != IDE_SUCCESS );
    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        //----------------------------
        // Right Child의 수행
        //----------------------------

        IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, & sFlag )
                  != IDE_SUCCESS );

        if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
        {
            *aFlag = QMC_ROW_DATA_NONE;
        }
        else
        {
            // Left와 Right 모두 존재하는 경우
            // Merge Join 수행

            IDE_TEST( mergeJoin( aTemplate,
                                 sCodePlan,
                                 sDataPlan,
                                 ID_TRUE,  // Right Data가 존재함
                                 aFlag )
                      != IDE_SUCCESS );
        }
    }

    //----------------------------
    // 결과의 존재 유무에 따른 처리
    //----------------------------

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
        *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;
    }
    else
    {
        sDataPlan->doIt = qmnMGJN::doItNext;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMGJN::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 수행 함수
 *
 * Implementation :
 *    Right를 수행하고 Data 존재 유무에 따른 Merge Join을 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;
    qmndMGJN * sDataPlan =
        (qmndMGJN *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;

    //----------------------------
    // Right Child의 수행
    //----------------------------

    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, & sFlag )
              != IDE_SUCCESS );

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        IDE_TEST( mergeJoin( aTemplate,
                             sCodePlan,
                             sDataPlan,
                             ID_FALSE,  // Right Data가 없음
                             aFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mergeJoin( aTemplate,
                             sCodePlan,
                             sDataPlan,
                             ID_TRUE,  // Right Data가 존재함
                             aFlag )
                  != IDE_SUCCESS );
    }

    //----------------------------
    // 결과의 존재 유무에 따른 처리
    //----------------------------

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
        *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;

        sDataPlan->doIt = qmnMGJN::doItFirst;
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
qmnMGJN::doItSemi( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 수행 함수
 *
 * Implementation :
 *    Semi merge join을 수행한다.
 *
 ***********************************************************************/

    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;
    qmndMGJN * sDataPlan =
        (qmndMGJN *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sLeftFlag  = QMC_ROW_INITIALIZE;
    qmcRowFlag sRightFlag = QMC_ROW_INITIALIZE;
    idBool     sJudge;
    idBool     sFetchRight;

    if( ( *sDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK )
            == QMND_MGJN_CURSOR_STORED_TRUE )
    {
        sFetchRight = ID_FALSE;
    }
    else
    {
        sFetchRight = ID_TRUE;
    }

    //----------------------------
    // Left Child의 수행
    //----------------------------

    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
              != IDE_SUCCESS );

    while ( (sLeftFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        if( ( ( *sDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK )
                  == QMND_MGJN_CURSOR_STORED_TRUE ) &&
              ( sFetchRight == ID_FALSE ) )
        {
            IDE_TEST( qtc::judge( &sJudge,
                                  sCodePlan->storedMergeJoinPred,
                                  aTemplate )
                      != IDE_SUCCESS );

            if( sJudge == ID_TRUE )
            {
                IDE_TEST( restoreRightCursor( aTemplate, sCodePlan, sDataPlan )
                          != IDE_SUCCESS );

                sRightFlag = QMC_ROW_DATA_EXIST;
                sFetchRight = ID_FALSE;
            }
            else
            {
                *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
                *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }

        if( sFetchRight == ID_TRUE )
        {
            sRightFlag = QMC_ROW_DATA_NONE;
            IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, & sRightFlag )
                      != IDE_SUCCESS );

            if ( (sRightFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
            {
                sFetchRight = ID_FALSE;
                IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
                          != IDE_SUCCESS );

                continue;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if( ( sRightFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
            {
                *aFlag = QMC_ROW_DATA_NONE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        IDE_TEST( qtc::judge( &sJudge,
                              sCodePlan->mergeJoinPred,
                              aTemplate )
                  != IDE_SUCCESS );

        if( sJudge == ID_TRUE )
        {
            // mergeJoinPred가 만족되는 첫 번재 right인 경우 cursor 저장
            IDE_TEST( manageCursor( aTemplate, sCodePlan, sDataPlan )
                      != IDE_SUCCESS );

            // joinFilter가 만족되는지 확인
            IDE_TEST( checkJoinFilter( aTemplate, sCodePlan, &sJudge )
                      != IDE_SUCCESS );

            if( sJudge == ID_TRUE )
            {
                *aFlag = QMC_ROW_DATA_EXIST;
                break;
            }
            else
            {
                // joinFilter가 실패하였으므로 다음 right를 fetch
                sFetchRight = ID_TRUE;
            }
        }
        else
        {
            if( sCodePlan->compareLeftRight != NULL )
            {
                // Equi-joni인 경우
                IDE_TEST( qtc::judge( &sJudge,
                                      sCodePlan->compareLeftRight,
                                      aTemplate )
                          != IDE_SUCCESS );

                if( sJudge == ID_TRUE )
                {
                    // L > R
                    sFetchRight = ID_TRUE;
                    *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
                    *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;
                }
                else
                {
                    // L < R
                    sFetchRight = ID_FALSE;
                    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                // Non equi-join인 경우
                // BUG-41632 Semi-Merge-Join gives different result
                // 프리디킷의 형태에 따라서 수행방식이 바뀌어야 한다.
                if ( sCodePlan->mergeJoinPred->indexArgument == 0 )
                {
                    // T2.i1 < T1.i1
                    //      MGJN
                    //     |    |
                    //    T1    T2 인 경우임

                    if( ( sCodePlan->mergeJoinPred->node.module == &mtfLessThan ) ||
                        ( sCodePlan->mergeJoinPred->node.module == &mtfLessEqual ) )
                    {
                        // <, <= 인 경우
                        // Left를 읽어들인다.

                        sFetchRight = ID_FALSE;
                        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
                                  != IDE_SUCCESS );



                    }
                    else
                    {
                        // >, >= 인 경우
                        // Right를 읽어들인다.

                        *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
                        *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;

                        sFetchRight = ID_TRUE;
                    }
                }
                else
                {
                    // T1.i1 < T2.i1
                    //      MGJN
                    //     |    |
                    //    T1    T2 인 경우임

                    if( ( sCodePlan->mergeJoinPred->node.module == &mtfLessThan ) ||
                        ( sCodePlan->mergeJoinPred->node.module == &mtfLessEqual ) )
                    {
                        *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
                        *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;

                        // <, <= 인 경우
                        // Right를 읽어들인다.
                        sFetchRight = ID_TRUE;
                    }
                    else
                    {
                        // >, >= 인 경우
                        // Left를 읽어들인다.
                        sFetchRight = ID_FALSE;
                        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
                                  != IDE_SUCCESS );
                    }
                }
            }
        }
    }

    if( (sLeftFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        // Nothing to do.
    }

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
        *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnMGJN::doItAnti( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 수행 함수
 *
 * Implementation :
 *    Anti merge join을 수행한다.
 *
 ***********************************************************************/

    qmncMGJN * sCodePlan = (qmncMGJN *) aPlan;
    qmndMGJN * sDataPlan =
        (qmndMGJN *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sLeftFlag  = QMC_ROW_INITIALIZE;
    qmcRowFlag sRightFlag = QMC_ROW_INITIALIZE;
    idBool     sJudge;
    idBool     sFetchRight;

    if( ( *sDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK )
            == QMND_MGJN_CURSOR_STORED_TRUE )
    {
        sFetchRight = ID_FALSE;
    }
    else
    {
        sFetchRight = ID_TRUE;
    }

    //----------------------------
    // Left Child의 수행
    //----------------------------

    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
              != IDE_SUCCESS );

    while ( (sLeftFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        if( ( ( *sDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK )
                  == QMND_MGJN_CURSOR_STORED_TRUE ) &&
              ( sFetchRight == ID_FALSE ) )
        {
            IDE_TEST( qtc::judge( &sJudge,
                                  sCodePlan->storedMergeJoinPred,
                                  aTemplate )
                      != IDE_SUCCESS );

            if( sJudge == ID_TRUE )
            {
                IDE_TEST( restoreRightCursor( aTemplate, sCodePlan, sDataPlan )
                          != IDE_SUCCESS );

                sRightFlag = QMC_ROW_DATA_EXIST;
                sFetchRight = ID_FALSE;
            }
            else
            {
                *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
                *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }

        if( sFetchRight == ID_TRUE )
        {
            IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, & sRightFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        if( ( sRightFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
        {
            *aFlag = QMC_ROW_DATA_EXIST;
            break;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( qtc::judge( &sJudge,
                              sCodePlan->mergeJoinPred,
                              aTemplate )
                  != IDE_SUCCESS );

        if( sJudge == ID_TRUE )
        {
            // mergeJoinPred가 만족되는 첫 번재 right인 경우 cursor 저장
            IDE_TEST( manageCursor( aTemplate, sCodePlan, sDataPlan )
                      != IDE_SUCCESS );

            // joinFilter가 만족되는지 확인
            IDE_TEST( checkJoinFilter( aTemplate, sCodePlan, &sJudge )
                      != IDE_SUCCESS );

            if( sJudge == ID_TRUE )
            {
                // Anti join 실패
                // 다음 left를 읽어들인다.
                IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, & sLeftFlag )
                          != IDE_SUCCESS );
                sFetchRight = ID_FALSE;
            }
            else
            {
                // joinFilter가 실패하였으므로 다음 right를 fetch
                sFetchRight = ID_TRUE;
            }
        }
        else
        {
            if( sCodePlan->compareLeftRight != NULL )
            {
                *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
                *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;

                // Equi-joni인 경우
                IDE_TEST( qtc::judge( &sJudge,
                                      sCodePlan->compareLeftRight,
                                      aTemplate )
                          != IDE_SUCCESS );

                if( sJudge == ID_TRUE )
                {
                    // L > R
                    sFetchRight = ID_TRUE;
                }
                else
                {
                    // L < R
                    // Anti join 성공
                    IDE_TEST( manageCursor( aTemplate, sCodePlan, sDataPlan )
                              != IDE_SUCCESS );

                    *aFlag = QMC_ROW_DATA_EXIST;
                    break;
                }
            }
            else
            {
                *sDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
                *sDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;

                // Non equi-join인 경우

                if( ( sCodePlan->mergeJoinPred->node.module == &mtfLessThan ) ||
                    ( sCodePlan->mergeJoinPred->node.module == &mtfLessEqual ) )
                {
                    // <, <= 인 경우
                    // Right를 읽어들인다.
                    sFetchRight = ID_TRUE;
                }
                else
                {
                    // >, >= 인 경우
                    // Anti join 성공
                    IDE_TEST( manageCursor( aTemplate, sCodePlan, sDataPlan )
                              != IDE_SUCCESS );

                    *aFlag = QMC_ROW_DATA_EXIST;
                    break;
                }
            }
        }
    }

    if( (sLeftFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        *aFlag = QMC_ROW_DATA_NONE;
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
qmnMGJN::firstInit( qcTemplate * aTemplate,
                    qmncMGJN   * aCodePlan,
                    qmndMGJN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    MGJN node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduMemory * sMemory;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    switch ( aCodePlan->flag & QMNC_MGJN_LEFT_CHILD_MASK )
    {
        case QMNC_MGJN_LEFT_CHILD_SCAN:
            IDE_DASSERT( aCodePlan->plan.left->type == QMN_SCAN );
            break;
        case QMNC_MGJN_LEFT_CHILD_PCRD:
            IDE_DASSERT( aCodePlan->plan.left->type == QMN_PCRD );
            break;
        case QMNC_MGJN_LEFT_CHILD_SORT:
            IDE_DASSERT( aCodePlan->plan.left->type == QMN_SORT );
            break;
        case QMNC_MGJN_LEFT_CHILD_MGJN:
            IDE_DASSERT( aCodePlan->plan.left->type == QMN_MGJN );
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    switch ( aCodePlan->flag & QMNC_MGJN_RIGHT_CHILD_MASK )
    {
        case QMNC_MGJN_RIGHT_CHILD_SCAN:
            IDE_DASSERT( aCodePlan->plan.right->type == QMN_SCAN );
            break;
        case QMNC_MGJN_RIGHT_CHILD_PCRD:
            IDE_DASSERT( aCodePlan->plan.right->type == QMN_PCRD );
            break;
        case QMNC_MGJN_RIGHT_CHILD_SORT:
            IDE_DASSERT( aCodePlan->plan.right->type == QMN_SORT );
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    IDE_DASSERT( aCodePlan->myNode != NULL );
    IDE_DASSERT( aCodePlan->mergeJoinPred != NULL );
    IDE_DASSERT( aCodePlan->storedMergeJoinPred != NULL );

    //---------------------------------
    // MGJN 고유 정보의 초기화
    //---------------------------------

    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );
    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    // Right Value 저장을 위한 공간 확보
    sMemory = aTemplate->stmt->qmxMem;
    IDE_TEST( sMemory->alloc( aDataPlan->mtrRowSize,
                              (void**) & aDataPlan->mtrNode->dstTuple->row )
              != IDE_SUCCESS);
    IDE_TEST_RAISE( aDataPlan->mtrNode->dstTuple->row == NULL, err_mem_alloc );

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_MGJN_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_MGJN_INIT_DONE_TRUE;

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
qmnMGJN::initMtrNode( qcTemplate * aTemplate,
                      qmncMGJN   * aCodePlan,
                      qmndMGJN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::initMtrNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aCodePlan->myNode->next == NULL );
    IDE_DASSERT( aCodePlan->mtrNodeOffset > 0 );
    IDE_DASSERT(
        ( aTemplate->tmplate.rows[aCodePlan->myNode->dstNode->node.table].lflag
          & MTC_TUPLE_STORAGE_MASK ) == MTC_TUPLE_STORAGE_MEMORY );

    //---------------------------------
    // 저장 Column의 초기화
    //---------------------------------

    // 1.  저장 Column의 연결 정보 생성
    // 2.  저장 Column의 초기화
    // 3.  저장 Column의 offset을 재조정
    // 4.  Row Size의 계산

    aDataPlan->mtrNode =
        (qmdMtrNode*) (aTemplate->tmplate.data + aCodePlan->mtrNodeOffset);

    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->mtrNode ) != IDE_SUCCESS );

    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                0 ) // Base Table을 저장하지 않음
              != IDE_SUCCESS );

    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  0 ) // Temp Table을 사용하지 않음
              != IDE_SUCCESS );

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnMGJN::mergeJoin( qcTemplate * aTemplate,
                    qmncMGJN   * aCodePlan,
                    qmndMGJN   * aDataPlan,
                    idBool       aRightExist,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Merge Join Algorithm을 통해 Row를 구성하여 리턴한다.
 *
 * Implementation :
 *     Merge Join Algorithm의 절차는 대략 다음과 같다.
 *
 *     - Right Row가 존재하지 않는 경우
 *         A.  최초 Stored Merge 조건 검사
 *             - Left Row 획득
 *             - 다음 조건에 따라 처리
 *                 : Left Row가 없는 경우, [결과 없음]
 *                 : Stored Merge 조건을 만족하지 않는 경우, [결과 없음]
 *                 : Stored Merge 조건을 만족하는 경우[결과 있음]
 *
 *     - Right Row가 존재하는 경우
 *         B.  Merge 조건 검사
 *             - 다음 조건에 따라 처리
 *                 : Merge 조건을 만족하지 않는 경우, [C 부터 진행]
 *                 : Merge 조건을 만족하는 경우 [결과 있음]
 *
 *         C. Stored Merge 조건 검사
 *             - 적절한 Left 또는 Right Row 획득
 *             - 다음 조건에 따라 처리
 *                 - Left를 읽고 row가 없는 경우, [결과 없음]
 *                 - 저장 Cursor가 없거나 Stored Merge조건을 만족하지 않는 경우
 *                     - Right Row가 없는 경우, [결과 없음]
 *                     - Right Row가 있는 경우, [B 부터 진행]
 *                 - Stored Merge 조건을 만족하는 경우 [결과 있음]
 *
 *    위와 같은 절차를 추상화하면 다음과 같은 과정으로 수행된다.
 *        - Right Row가 없는 경우
 *            A --> [C --> B --> C --> B]
 *        - Right Row가 있는 경우
 *            B --> [C --> B --> C --> B]
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::mergeJoin"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool sIsLoopMerge;

    if ( aRightExist == ID_FALSE )
    {
        //----------------------------------
        // Right Row가 존재하지 않는 경우
        //----------------------------------

        // A.  최초 Stored Merge 조건을 검사

        IDE_TEST( checkFirstStoredMerge( aTemplate,
                                         aCodePlan,
                                         aDataPlan,
                                         aFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------
        // Right Row가 존재하는 경우
        //-----------------------------------

        // B. Merge Join 조건 검사

        IDE_TEST( checkMerge( aTemplate,
                              aCodePlan,
                              aDataPlan,
                              & sIsLoopMerge,
                              aFlag )
                  != IDE_SUCCESS );

        if ( sIsLoopMerge == ID_TRUE )
        {
            IDE_TEST( loopMerge( aTemplate, aCodePlan, aDataPlan, aFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnMGJN::loopMerge( qcTemplate * aTemplate,
                    qmncMGJN   * aCodePlan,
                    qmndMGJN   * aDataPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     만족하는 결과가 있거나, 아예 없을 때까지 반복 수행
 *     cf) mergeJoin()의 C-->B 의 반복 과정
 *
 * Implementation :
 *     결과가 있을 때까지 다음을 수행
 *         C : Stored Merge Join 조건 검사
 *         B : Merge Join 조건 검사
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::loopMerge"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool     sContinue;

    //-----------------------------------
    // 질의 결과가 있거나, 없음이 보장될 때까지 반복 수행
    //-----------------------------------

    sContinue = ID_TRUE;

    while ( sContinue == ID_TRUE )
    {
        //-----------------------------------
        // C. Stored Merge 조건 검사
        //-----------------------------------

        IDE_TEST( checkStoredMerge( aTemplate,
                                    aCodePlan,
                                    aDataPlan,
                                    & sContinue,
                                    aFlag )
                  != IDE_SUCCESS );

        if ( sContinue == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing To Do
        }

        //-----------------------------------
        // B. Merge Join 조건 검사
        //-----------------------------------

        IDE_TEST( checkMerge( aTemplate,
                              aCodePlan,
                              aDataPlan,
                              & sContinue,
                              aFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnMGJN::checkMerge( qcTemplate * aTemplate,
                     qmncMGJN   * aCodePlan,
                     qmndMGJN   * aDataPlan,
                     idBool     * aContinueNeed,
                     qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Merge 조건을 만족하는 지를 판단하고,
 *    이에 따라 계속 진행할 것인지를 결정한다.
 *
 * Implementation :
 *    계속 진행하는 경우 :
 *        Merge 조건을 만족하지 않는 경우
 *    진행하지 않는 경우 :
 *        모든 조건을 만족하는 경우 : 결과 있음
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::checkMerge"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool sJudge;

    // Merge Join 조건 검사
    IDE_TEST( qtc::judge( & sJudge, aCodePlan->mergeJoinPred, aTemplate )
              != IDE_SUCCESS );

    if ( sJudge == ID_TRUE )
    {
        // 커서의 저장 여부 결정
        IDE_TEST( manageCursor( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );

        *aFlag = QMC_ROW_DATA_EXIST;
        *aContinueNeed = ID_FALSE;
    }
    else
    {
        *aContinueNeed = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMGJN::checkFirstStoredMerge( qcTemplate * aTemplate,
                                qmncMGJN   * aCodePlan,
                                qmndMGJN   * aDataPlan,
                                qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    최초 Right Row 가 없는 경우에 호출되며,
 *    Stored Merge 조건을 검사하여
 *    결과를 리턴할 것인지 Loop Merge가 필요한 지를 판단
 *    cf) mergeJoin()에서 A에 해당하는 내용 참조
 *
 * Implementation :
 *    Left Row를 읽어 Stored Merge 조건을 검사한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::checkFirstStoredMerge"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool sJudge;

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    // 반드시 저장 Cursor가 존재한다.
    IDE_DASSERT( (*aDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK)
                 == QMND_MGJN_CURSOR_STORED_TRUE );

    //-----------------------------------
    // Left Row 획득
    //-----------------------------------

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          aFlag )
              != IDE_SUCCESS );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        // 결과가 더 이상 없는 경우이다.
        // Nothing to do.
    }
    else
    {
        //-----------------------------------
        // Stored Merge Join 조건 검사
        //-----------------------------------

        IDE_TEST( qtc::judge( &sJudge,
                              aCodePlan->storedMergeJoinPred,
                              aTemplate )
                  != IDE_SUCCESS );

        if ( sJudge == ID_TRUE )
        {
            //-----------------------------------
            // Stored Merge Join 조건을 만족하는 경우
            //-----------------------------------

            // Cursor를 복원하여 Right Row를 구성한다.
            IDE_TEST( restoreRightCursor( aTemplate, aCodePlan, aDataPlan )
                      != IDE_SUCCESS );
            
            *aFlag = QMC_ROW_DATA_EXIST;
        }
        else
        {
            //-----------------------------------
            // Stored Merge Join 조건을 만족하지 않는 경우
            //-----------------------------------

            // 더이상 Cursor가 유효하지 않음
            *aDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
            *aDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;

            // 더이상 결과가 존재하지 않음
            *aFlag = QMC_ROW_DATA_NONE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnMGJN::checkStoredMerge( qcTemplate * aTemplate,
                           qmncMGJN   * aCodePlan,
                           qmndMGJN   * aDataPlan,
                           idBool     * aContinueNeed,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Stored Merge 조건과 Filter 조건을 만족하는 지를
 *    판단하고, 이에 따라 계속 진행할 것인지를 결정한다.
 *
 * Implementation :
 *
 *    다음과 같은 과정을 반복한다.
 *       - 새로운 Row를 획득
 *           - Left Row를 읽었는데 없는 경우, [결과 없음]
 *       - 저장 Cursor가 없는 경우
 *           - 새로운 Row가 있다면, [B(Merge 조건)부터 진행]
 *           - 새로운 Row가 없다면, [결과 없음]
 *       - 저장 Cursor가 있는 경우
 *           - Stored Merge조건을 만족하지 않는 경우
 *               - 새로운 Row가 없다면, [결과 없음]
 *               - 새로운 Row가 있다면, [B부터 진행]
 *           - Stored Merge조건은 만족하나, Filter를 만족하지 않는 경우
 *               - [계속 반복]
 *           - Stored Merge조건과 Filter 조건을 만족하는 경우, [결과 있음]
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::checkStoredMerge"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool sJudge;
    idBool sReadLeft;

    while ( 1 )
    {
        //----------------------------------------
        // 새로운 Row를 획득을 통한 지속 여부 검사
        //----------------------------------------

        IDE_TEST(
            readNewRow( aTemplate, aCodePlan, & sReadLeft, aFlag )
            != IDE_SUCCESS );

        //---------------------------------------
        // To Fix PR-8260
        // 다음과 같은 고려가 부족했음.
        // Left Row를 읽은 경우라면 Store Cursor를 검사해야 하지만,
        // Right Row를 읽은 경우라면 Store Cursor를 검사하지 말아야 한다.
        // 따라서, 다음과 같이 정리한다.
        //
        // Left Row를 읽은 경우
        //    - Data가 있는 경우
        //        - Store 조건이 있으면 검사
        //        - Store 조건이 없으면 Return 후 계속 진행
        //    - Data가 없는 경우
        //        - 종료 조건
        // Right Row를 읽은 경우
        //    - Data가 있는 경우
        //        - Return 후 계속 진행
        //    - Data가 없는 경우
        //        - Store 조건이 있으면 Left를 읽은 후 존재하면 검사
        //        - Store 조건이 없으면 종료 조건
        //---------------------------------------

        if ( sReadLeft == ID_TRUE )
        {
            if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                if ( (*aDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK)
                     == QMND_MGJN_CURSOR_STORED_TRUE )
                {
                    // Store Condition이 존재하면 검사한다.
                    // Nothing To Do
                }
                else
                {
                    *aContinueNeed = ID_TRUE;
                    break;
                }
            }
            else
            {
                *aContinueNeed = ID_FALSE;
                break;
            }
        }
        else
        {
            if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                *aContinueNeed = ID_TRUE;
                break;
            }
            else
            {
                if ( (*aDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK)
                     == QMND_MGJN_CURSOR_STORED_TRUE )
                {
                    // Store Condition이 존재하면 Left를 읽은 후 검사한다.
                    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                                          aCodePlan->plan.left,
                                                          aFlag )
                              != IDE_SUCCESS );
                    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                    {
                        // 저장 Cursor로부터 다시 검사한다.
                        // Nothing To Do
                    }
                    else
                    {
                        *aContinueNeed = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    *aContinueNeed = ID_FALSE;
                    break;
                }
            }
        }

        //----------------------------------------
        // Stored Merge 조건을 이용한 검사
        //----------------------------------------

        // 읽은 Row가 left인 경우에만 검사하게 된다.
        IDE_TEST( qtc::judge( &sJudge,
                              aCodePlan->storedMergeJoinPred,
                              aTemplate )
                  != IDE_SUCCESS );

        if ( sJudge == ID_FALSE )
        {
            // Stored Merge Join 조건을 만족하지 않는 경우

            // 더이상 Cursor가 유효하지 않음
            *aDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
            *aDataPlan->flag |= QMND_MGJN_CURSOR_STORED_FALSE;

            if ( sReadLeft == ID_TRUE )
            {
                // Stored Condition이 없기 때문에
                // 읽은 Row의 구성에 맞게 진행하게 된다.
                *aContinueNeed = ID_TRUE;
            }
            else
            {
                // Right를 읽은 경우라면 Right가 Data가 없고
                // Left를 다시 읽은 후에 검사하는 경우이다.
                // 더 이상 진행할 수 없다.
                *aContinueNeed = ID_FALSE;
                // To Fix BUG-8747
                *aFlag = QMC_ROW_DATA_NONE;
            }
            break;
        }
        else
        {
            // Stored Merge Join 조건을 만족하는 경우

            // Cursor를 복원하여 Right Row를 Tuple Set에 복원
            IDE_TEST( restoreRightCursor( aTemplate,
                                          aCodePlan,
                                          aDataPlan )
                      != IDE_SUCCESS );

            // 만족할 경우 결과 리턴
            *aFlag = QMC_ROW_DATA_EXIST;
            *aContinueNeed = ID_FALSE;
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMGJN::manageCursor( qcTemplate * aTemplate,
                       qmncMGJN   * aCodePlan,
                       qmndMGJN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Merge Join Predicate 만족 시 Cursor 관리
 *
 * Implementation :
 *     이미 저장 Cursor가 존재한다면, 별다른 작업을 하지 않는다.
 *     저장 Cursor가 존재하지 않을 경우,
 *         - Cursor 저장
 *         - Right Value 저장
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::manageCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    if ( (*aDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK )
         == QMND_MGJN_CURSOR_STORED_FALSE )
    {
        //------------------------------------
        // Cursor를 저장
        //------------------------------------

        IDE_TEST( storeRightCursor( aTemplate, aCodePlan )
                  != IDE_SUCCESS );

        // To Fix PR-8062
        // Mask 초기화 잘못함.
        *aDataPlan->flag &= ~QMND_MGJN_CURSOR_STORED_MASK;
        *aDataPlan->flag |= QMND_MGJN_CURSOR_STORED_TRUE;

        //------------------------------------
        // Right Value 저장
        //------------------------------------

        for ( sNode = aDataPlan->mtrNode;
              sNode != NULL;
              sNode = sNode->next )
        {
            IDE_TEST( sNode->func.setMtr( aTemplate,
                                          sNode,
                                          sNode->dstTuple->row )
                      != IDE_SUCCESS );
        }
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
qmnMGJN::readNewRow( qcTemplate * aTemplate,
                     qmncMGJN   * aCodePlan,
                     idBool     * aReadLeft,
                     qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     적절한 Left 또는 Right Row를 획득한다.
 *
 * Implementation :
 *     등호 연산자인 경우(Compare 조건이 존재)
 *         - 조건을 만족하지 않으면, Left Read
 *         - 조건을 만족하지 않으면, Right Read
 *     등호 연산자가 아닌 경우(Compare 조건이 없음)
 *         - Left Read
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::readNewRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool sJudge;

    //------------------------------------
    // 읽을 Row의 결정
    //------------------------------------

    if ( aCodePlan->compareLeftRight != NULL )
    {
        IDE_TEST( qtc::judge( &sJudge,
                              aCodePlan->compareLeftRight,
                              aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        sJudge = ID_FALSE;
    }

    //------------------------------------
    // 새로운 Row의 획득
    //------------------------------------

    if ( sJudge == ID_TRUE )
    {
        // Right Row를 읽는다.
        IDE_TEST( aCodePlan->plan.right->doIt( aTemplate,
                                               aCodePlan->plan.right,
                                               aFlag )
                  != IDE_SUCCESS );
        *aReadLeft = ID_FALSE;
    }
    else
    {
        // To Fix PR-8062
        // Left Row를 읽는다.
        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              aFlag )
                  != IDE_SUCCESS );

        *aReadLeft = ID_TRUE;
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}



IDE_RC
qmnMGJN::checkJoinFilter( qcTemplate * aTemplate,
                          qmncMGJN   * aCodePlan,
                          idBool     * aResult )
{
/***********************************************************************
 *
 * Description :
 *     Join Filter의 만족 여부 검사
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::checkJoinFilter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( aCodePlan->joinFilter == NULL )
    {
        *aResult = ID_TRUE;
    }
    else
    {
        IDE_TEST( qtc::judge( aResult, aCodePlan->joinFilter, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMGJN::storeRightCursor( qcTemplate * aTemplate,
                           qmncMGJN   * aCodePlan )
{
/***********************************************************************
 *
 * Description :
 *     Right Child의 Cursor를 저장한다.
 *
 * Implementation :
 *     Child의 종류에 따라 Cursor 저장 함수를 호출한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::storeRightCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //------------------------------------
    // Right Child의 Cursor 저장
    //------------------------------------

    switch ( aCodePlan->flag & QMNC_MGJN_RIGHT_CHILD_MASK )
    {
        case QMNC_MGJN_RIGHT_CHILD_SCAN:
            IDE_TEST( qmnSCAN::storeCursor( aTemplate, aCodePlan->plan.right )
                      != IDE_SUCCESS );
            break;
        case QMNC_MGJN_RIGHT_CHILD_PCRD:
            IDE_TEST( qmnPCRD::storeCursor( aTemplate, aCodePlan->plan.right )
                      != IDE_SUCCESS );
            break;
        case QMNC_MGJN_RIGHT_CHILD_SORT:
            IDE_TEST( qmnSORT::storeCursor( aTemplate, aCodePlan->plan.right )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMGJN::restoreRightCursor( qcTemplate * aTemplate,
                             qmncMGJN   * aCodePlan,
                             qmndMGJN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Right Child의 Cursor를 복원한다.
 *
 * Implementation :
 *     Child의 종류에 따라 Cursor 복원 함수를 호출한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnMGJN::restoreRightCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // To Check PR-11733
    IDE_ASSERT( ( *aDataPlan->flag & QMND_MGJN_CURSOR_STORED_MASK )
                == QMND_MGJN_CURSOR_STORED_TRUE );

    //------------------------------------
    // Right Child의 Cursor 복원
    //------------------------------------

    switch ( aCodePlan->flag & QMNC_MGJN_RIGHT_CHILD_MASK )
    {
        case QMNC_MGJN_RIGHT_CHILD_SCAN:
            IDE_TEST( qmnSCAN::restoreCursor( aTemplate,
                                              aCodePlan->plan.right )
                      != IDE_SUCCESS );
            break;
        case QMNC_MGJN_RIGHT_CHILD_PCRD:
            IDE_TEST( qmnPCRD::restoreCursor( aTemplate,
                                              aCodePlan->plan.right )
                      != IDE_SUCCESS );
            break;
        case QMNC_MGJN_RIGHT_CHILD_SORT:
            IDE_TEST( qmnSORT::restoreCursor( aTemplate,
                                              aCodePlan->plan.right )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
