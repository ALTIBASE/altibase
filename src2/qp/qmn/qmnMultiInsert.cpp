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
 * $Id$
 *
 * Description :
 *     Multiple INST(Multi INSerT) Node
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmnMultiInsert.h>
#include <qmnInsert.h>

IDE_RC 
qmnMTIT::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    MTIT 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMTIT::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMTIT * sCodePlan = (qmncMTIT *) aPlan;
    qmndMTIT * sDataPlan = 
        (qmndMTIT *) (aTemplate->tmplate.data + aPlan->offset);

    qmnChildren * sChildren;
    
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
    
    sDataPlan->doIt = qmnMTIT::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_MTIT_INIT_DONE_MASK)
         == QMND_MTIT_INIT_DONE_FALSE )
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

    for ( sChildren = sCodePlan->plan.children;
          sChildren != NULL;
          sChildren = sChildren->next )
    {
        // BUG-45288
        ((qmndINST*) (aTemplate->tmplate.data +
                      sChildren->childPlan->offset))->isAppend =
            ((qmncINST*)sChildren->childPlan)->isAppend;    

        IDE_TEST( sChildren->childPlan->init( aTemplate,
                                              sChildren->childPlan )
                  != IDE_SUCCESS );
    }

    //------------------------------------------------
    // 수행 함수 결정
    //------------------------------------------------

    sDataPlan->doIt = qmnMTIT::doIt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnMTIT::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MTIT의 고유 기능을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMTIT::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMTIT    * sCodePlan = (qmncMTIT *) aPlan;
    qmnChildren * sChildren;
    qmcRowFlag    sFlag = QMC_ROW_INITIALIZE;
    
    //--------------------------------------------------
    // 모든 Child를 수행한다.
    //--------------------------------------------------

    sChildren = sCodePlan->plan.children;

    // 첫번째 결과만 넘긴다.
    IDE_TEST( sChildren->childPlan->doIt( aTemplate,
                                          sChildren->childPlan,
                                          aFlag )
              != IDE_SUCCESS );

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        for ( sChildren = sChildren->next;
              sChildren != NULL;
              sChildren = sChildren->next )
        {
            IDE_TEST( sChildren->childPlan->doIt( aTemplate,
                                                  sChildren->childPlan,
                                                  &sFlag )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnMTIT::padNull( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnMTIT::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMTIT::printPlan( qcTemplate   * aTemplate,
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

#define IDE_FN "qmnMTIT::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMTIT * sCodePlan = (qmncMTIT*) aPlan;
    qmndMTIT * sDataPlan = 
        (qmndMTIT*) (aTemplate->tmplate.data + aPlan->offset);
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
    // MTIT 노드 표시
    //----------------------------
    
    iduVarStringAppend( aString,
                        "MULTIPLE-INSERT\n" );

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
qmnMTIT::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnMTIT::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnMTIT::firstInit( qmndMTIT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Data 영역에 대한 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMTIT::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_MTIT_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_MTIT_INIT_DONE_TRUE;

    return IDE_SUCCESS;

#undef IDE_FN
}
