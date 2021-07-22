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
 *     DLAY(DeLAY) Node
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qmnDelay.h>
#include <qcg.h>

IDE_RC
qmnDLAY::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    DLAY 노드의 초기화
 *
 * Implementation :
 *    - 최초 초기화가 되지 않은 경우 최초 초기화 수행
 *    - Child Plan에 대한 초기화
 *    - Constant Delay의 수행 결과 검사
 *    - Constant Delay의 결과에 따른 수행 함수 결정
 *
 ***********************************************************************/

    qmncDLAY * sCodePlan = (qmncDLAY*) aPlan;
    qmndDLAY * sDataPlan =
        (qmndDLAY*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnDLAY::doIt;

    return IDE_SUCCESS;
}

IDE_RC
qmnDLAY::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    DLAY의 수행 함수
 *    Child를 수행하고 Record가 있을 경우 조건을 검사한다.
 *    조건을 만족할 때끼지 이를 반복한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnDLAY::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //qmncDLAY * sCodePlan = (qmncDLAY*) aPlan;
    qmndDLAY * sDataPlan =
        (qmndDLAY*) (aTemplate->tmplate.data + aPlan->offset);

    if ( (*sDataPlan->flag & QMND_DLAY_INIT_DONE_MASK)
         == QMND_DLAY_INIT_DONE_FALSE )
    {
        /* Child Plan의 초기화 */
        IDE_TEST( aPlan->left->init( aTemplate, aPlan->left ) != IDE_SUCCESS );
        
        *sDataPlan->flag &= ~QMND_DLAY_INIT_DONE_MASK;
        *sDataPlan->flag |= QMND_DLAY_INIT_DONE_TRUE;
    }
    else
    {
        // Nothing To Do
    }
    
    // Child를 수행
    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnDLAY::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    DLAY 노드는 별도의 null row를 가지지 않으며,
 *    Child에 대하여 padNull()을 호출한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnDLAY::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnDLAY::padNull"));

    qmncDLAY * sCodePlan = (qmncDLAY*) aPlan;

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_DLAY_INIT_DONE_MASK)
         == QMND_DLAY_INIT_DONE_FALSE )
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
qmnDLAY::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    DLAY 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnDLAY::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnDLAY::printPlan"));

    qmncDLAY * sCodePlan = (qmncDLAY*) aPlan;
    qmndDLAY * sDataPlan =
        (qmndDLAY*) (aTemplate->tmplate.data + aPlan->offset);
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
    // DLAY 노드 표시
    //----------------------------

    iduVarStringAppend( aString,
                        "DELAY\n" );

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
