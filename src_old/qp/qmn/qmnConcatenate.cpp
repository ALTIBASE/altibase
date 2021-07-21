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
 * $Id: qmnConcatenate.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     BUNI(Bag Union) Node
 *
 *     관계형 모델에서 Bag Union을 수행하는 Plan Node 이다.
 *  
 *     다음과 같은 기능을 위해 사용된다.
 *         - Bag Union
 *         - Set Union
 *
 *     Left Child에 대한 Data와 Right Child에 대한 Data를
 *     모두 리턴한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnConcatenate.h>


IDE_RC 
qmnCONC::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    CONC 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCONC::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncCONC * sCodePlan = (qmncCONC *) aPlan;
    qmndCONC * sDataPlan = 
        (qmndCONC *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    
    sDataPlan->doIt = qmnCONC::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_CONC_INIT_DONE_MASK)
         == QMND_CONC_INIT_DONE_FALSE )
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

    IDE_TEST( aPlan->left->init( aTemplate, 
                                 aPlan->left ) != IDE_SUCCESS);

    // Right Child에 대한 초기화는 필요 시점에 수행한다.
    
    //------------------------------------------------
    // 수행 함수 결정
    //------------------------------------------------

    sDataPlan->doIt = qmnCONC::doItLeft;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC 
qmnCONC::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    CONC의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnCONC::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncCONC * sCodePlan = (qmncCONC *) aPlan;
    qmndCONC * sDataPlan = 
        (qmndCONC*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnCONC::padNull( qcTemplate * aTemplate,
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
 *    일반 Two-Child Plan과 달리 Left와 Right가 동일한 구조를
 *    가지므로 Left에 대한 Null Padding만을 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnCONC::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncCONC * sCodePlan = (qmncCONC *) aPlan;
    // qmndCONC * sDataPlan = 
    //     (qmndCONC *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_CONC_INIT_DONE_MASK)
         == QMND_CONC_INIT_DONE_FALSE )
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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnCONC::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *     수행 정보를 출력한다
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCONC::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncCONC * sCodePlan = (qmncCONC*) aPlan;
    qmndCONC * sDataPlan = 
        (qmndCONC*) (aTemplate->tmplate.data + aPlan->offset);
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
    // CONC 노드 표시
    //----------------------------

    iduVarStringAppend( aString,
                        "CONCATENATION \n" );

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
qmnCONC::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnCONC::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnCONC::doItLeft( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Left Child로부터 Row를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCONC::doItLeft"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncCONC * sCodePlan = (qmncCONC*) aPlan;
    qmndCONC * sDataPlan = 
        (qmndCONC*) (aTemplate->tmplate.data + aPlan->offset);

    //----------------------------
    // Read Left Row
    //----------------------------
    
    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag ) 
              != IDE_SUCCESS );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        // Row가 존재하지 않을 경우 Right Child에 대한 처리를 수행
        
        sDataPlan->doIt = qmnCONC::doItRight;
        IDE_TEST( aPlan->right->init( aTemplate, aPlan->right ) 
                  != IDE_SUCCESS );
        
        IDE_TEST( qmnCONC::doItRight( aTemplate, aPlan, aFlag ) 
                  != IDE_SUCCESS );
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
qmnCONC::doItRight( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Right Child로부터 Row를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCONC::doItRight"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncCONC * sCodePlan = (qmncCONC*) aPlan;
    qmndCONC * sDataPlan = 
        (qmndCONC*) (aTemplate->tmplate.data + aPlan->offset);

    //----------------------------
    // Read Right Row
    //----------------------------

    IDE_TEST( aPlan->right->doIt( aTemplate, aPlan->right, aFlag ) 
              != IDE_SUCCESS );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        sDataPlan->doIt = qmnCONC::doItLeft;

        *aFlag = QMC_ROW_DATA_NONE;
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
qmnCONC::firstInit( qmndCONC   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Data 영역에 대한 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCONC::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_CONC_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_CONC_INIT_DONE_TRUE;
    
    return IDE_SUCCESS;
    
#undef IDE_FN
}

