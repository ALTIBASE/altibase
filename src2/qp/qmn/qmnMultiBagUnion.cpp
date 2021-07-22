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
 * $Id: qmnMultiBagUnion.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Multiple BUNI(Multiple Bag Union) Node
 *
 *     관계형 모델에서 Bag Union을 수행하는 Plan Node 이다.
 *  
 *     다음과 같은 기능을 위해 사용된다.
 *         - Multiple Bag Union
 *
 *     Multi Children 에 대한 Data를 모두 리턴한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmnMultiBagUnion.h>

IDE_RC 
qmnMultiBUNI::init( qcTemplate * aTemplate,
                    qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    MultiBUNI 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMultiBUNI::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMultiBUNI * sCodePlan = (qmncMultiBUNI *) aPlan;
    qmndMultiBUNI * sDataPlan = 
        (qmndMultiBUNI *) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan * sChildPlan;
    
    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aPlan->left     == NULL );
    IDE_DASSERT( aPlan->right    == NULL );
    IDE_DASSERT( aPlan->children != NULL );

    //---------------------------------
    // 기본 초기화
    //---------------------------------
    
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    
    sDataPlan->doIt = qmnMultiBUNI::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_MULTI_BUNI_INIT_DONE_MASK)
         == QMND_MULTI_BUNI_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //------------------------------------------------
    // Child Plan의 초기화
    //------------------------------------------------

    sDataPlan->curChild = sCodePlan->plan.children;
    sChildPlan = sDataPlan->curChild->childPlan;
    
    IDE_TEST( sChildPlan->init( aTemplate, sChildPlan )
              != IDE_SUCCESS);

    //------------------------------------------------
    // 수행 함수 결정
    //------------------------------------------------

    sDataPlan->doIt = qmnMultiBUNI::doIt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC 
qmnMultiBUNI::doIt( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MultiBUNI의 고유 기능을 수행한다.
 *
 * Implementation :
 *    현재 Child Plan 를 수행하고, 없을 경우 다음 child plan을 수행
 *
 ***********************************************************************/

#define IDE_FN "qmnMultiBUNI::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndMultiBUNI * sDataPlan = 
        (qmndMultiBUNI*) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan       * sChildPlan;
    
    //--------------------------------------------------
    // Data가 존재할 때까지 모든 Child를 수행한다.
    //--------------------------------------------------

    while ( 1 )
    {
        //----------------------------
        // 현재 Child를 수행
        //----------------------------
        
        sChildPlan = sDataPlan->curChild->childPlan;

        IDE_TEST( sChildPlan->doIt( aTemplate, sChildPlan, aFlag )
                  != IDE_SUCCESS );

        //----------------------------
        // Data 존재 여부에 따른 처리
        //----------------------------
        
        if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            //----------------------------
            // 현재 Child에 Data가 존재하는 경우
            //----------------------------
            break;
        }
        else
        {
            //----------------------------
            // 현재 Child에 Data가 없는 경우
            //----------------------------
            
            sDataPlan->curChild = sDataPlan->curChild->next;

            if ( sDataPlan->curChild == NULL )
            {
                // 모든 Child를 수행한 경우
                break;
            }
            else
            {
                // 다음 Child Plan 에 대한 초기화를 수행한다.
                sChildPlan = sDataPlan->curChild->childPlan;
                IDE_TEST( sChildPlan->init( aTemplate, sChildPlan )
                          != IDE_SUCCESS);
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnMultiBUNI::padNull( qcTemplate * /* aTemplate */,
                       qmnPlan    * /* aPlan */)
{
/***********************************************************************
 *
 * Description :
 *    호출되어서는 안됨.
 *    상위 Node는 반드시 VIEW노드이며,
 *    View는 자신의 Null Row만을 설정하기 때문이다.
 *    
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMultiBUNI::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMultiBUNI::printPlan( qcTemplate   * aTemplate,
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

#define IDE_FN "qmnMultiBUNI::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMultiBUNI * sCodePlan = (qmncMultiBUNI*) aPlan;
    qmndMultiBUNI * sDataPlan = 
       (qmndMultiBUNI*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    ULong i;

    qmnChildren * sChildren;

    //----------------------------
    // Display 위치 결정
    //----------------------------
    
    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //----------------------------
    // MultiBUNI 노드 표시
    //----------------------------
    
    iduVarStringAppend( aString,
                        "BAG-UNION\n" );

    //----------------------------
    // Child Plan의 정보 출력
    //----------------------------

    for ( sChildren = sCodePlan->plan.children;
          sChildren != NULL;
          sChildren = sChildren->next )
    {
        IDE_TEST( sChildren->childPlan->printPlan( aTemplate,
                                                   sChildren->childPlan,
                                                   aDepth + 1,
                                                   aString,
                                                   aMode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC 
qmnMultiBUNI::doItDefault( qcTemplate * /* aTemplate */,
                           qmnPlan    * /* aPlan */,
                           qmcRowFlag * /* aFlag */)
{
/***********************************************************************
 *
 * Description :
 *    호출되어서는 안됨
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMultiBUNI::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnMultiBUNI::firstInit( qmndMultiBUNI   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Data 영역에 대한 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMultiBUNI::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_MULTI_BUNI_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_MULTI_BUNI_INIT_DONE_TRUE;

    return IDE_SUCCESS;

#undef IDE_FN
}


