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
 * $Id: qmnProject.cpp 90609 2021-04-15 06:36:26Z ahra.cho $
 *
 * Description :
 *     PROJ(PROJection) Node
 *
 *     관계형 모델에서 projection을 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmx.h>
#include <qmnProject.h>
#include <qcg.h>
#include <qmcThr.h>
#include <mtd.h>
#include <qmnViewMaterialize.h>
#include <qsxEnv.h>

IDE_RC
qmnPROJ::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    PROJ 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::init"));

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    qmndPROJ * sDataPlan =
        (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnPROJ::doItDefault;

    //------------------------------------------------
    // 최초 초기화 수행 여부 판단
    //------------------------------------------------

    if ( ( *sDataPlan->flag & QMND_PROJ_INIT_DONE_MASK )
         == QMND_PROJ_INIT_DONE_FALSE )
    {
        // 최초 초기화 수행
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // To Fix PR-8836
    // 하위 노드에서 LEVEL Column을 참조할 수 있기 때문에
    // Child Plan을 초기화하기 전에 LEVEL Pseudo Column을 초기화하여야 함.
    // LEVEL Pseudo Column의 초기화
    IDE_TEST( initLevel( aTemplate, sCodePlan )
              != IDE_SUCCESS );

    //------------------------------------------------
    // Child Plan의 초기화
    //------------------------------------------------

    IDE_TEST( aPlan->left->init( aTemplate,
                                 aPlan->left ) != IDE_SUCCESS);

    // PROJ-2462 Result Cache
    if ( ( sCodePlan->flag & QMNC_PROJ_TOP_RESULT_CACHE_MASK )
         == QMNC_PROJ_TOP_RESULT_CACHE_TRUE )
    {
        IDE_DASSERT( sCodePlan->plan.left->type == QMN_VMTR );

        IDE_TEST( qmnPROJ::getVMTRInfo( aTemplate,
                                        sCodePlan,
                                        sDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    //------------------------------------------------
    // 가변 Data 의 초기화
    //------------------------------------------------

    // Limit 시작 개수의 초기화
    sDataPlan->limitCurrent = 1;

    // 최초 doIt()이 수행되지 않았음을 표기
    *sDataPlan->flag &= ~QMND_PROJ_FIRST_DONE_MASK;
    *sDataPlan->flag |= QMND_PROJ_FIRST_DONE_FALSE;

    //------------------------------------------------
    // 수행 함수의 결정
    //------------------------------------------------

    IDE_TEST( setDoItFunction( sCodePlan, sDataPlan )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnPROJ::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    PROJ 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    - Child Plan을 수행
 *    - Record가 존재하는 경우
 *        - Sequence 값 설정
 *        - 지정된 수행 함수를 실행
 *    - Record가 존재하지 않는 경우
 *        - Indexable MIN-MAX에 대한 처리
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    qmndPROJ * sDataPlan =
        (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    //-----------------------------------
    // Child Plan을 수행함
    //-----------------------------------

    // To fix PR-3921
    if ( sDataPlan->limitCurrent == sDataPlan->limitEnd )
    {
        // 주어진 Limit 조건에 다다른 경우
        *aFlag = QMC_ROW_DATA_NONE;
        sDataPlan->limitCurrent = 1;
    }
    else
    {
        // To Fix PR-6907
        // 수행 도중에 Limit에 의해 종료된 후
        // 다시 수행되는 경우라면 초기화를 다시 수행하여야 한다.
        if( sDataPlan->limitCurrent == 1 &&
            sDataPlan->limitEnd != 0 &&
            ( *sDataPlan->flag & QMND_PROJ_FIRST_DONE_MASK )
            == QMND_PROJ_FIRST_DONE_TRUE )
        {
            IDE_TEST( qmnPROJ::init( aTemplate, aPlan ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        /*
         * for loop clause
         * 매 첫 record의 doIt 시에만 child를 doIt한다.
         * 결과적으로 loop clause 에 의해 복제되는 record는
         * 앞서 수행된 동일한 child의 의 doIt에 대한 결과에 대해
         * projection만 별도로 수행하는 형태가 된다.
         */
        if ( sDataPlan->loopCount > 0 )
        {
            if ( sDataPlan->loopCurrent == 0 )
            {
                // PROJ-2462 Result Cache
                if ( ( sCodePlan->flag & QMNC_PROJ_TOP_RESULT_CACHE_MASK )
                     == QMNC_PROJ_TOP_RESULT_CACHE_FALSE )
                {
                    // doIt left child
                    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    // PROJ-2462 ResultCache
                    // Top Result Cache로 사용된경우 VMTR로 부터
                    // record 를 하나 가져온다.
                    IDE_TEST( doItVMTR( aTemplate, aPlan, aFlag )
                                        != IDE_SUCCESS );
                }
            }
            else
            {
                *aFlag = QMC_ROW_DATA_EXIST;
            }
        }
        else
        {
            *aFlag = QMC_ROW_DATA_NONE;
        }
    }

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        //-----------------------------------
        // Child의 결과가 존재하는 경우
        //-----------------------------------

        IDE_TEST( readSequence( aTemplate,
                                sCodePlan,
                                sDataPlan ) != IDE_SUCCESS );

        // loop_level 설정
        setLoopCurrent( aTemplate, aPlan );
        
        // 한번은 수행되었음을 표시
        *sDataPlan->flag &= ~QMND_PROJ_FIRST_DONE_MASK;
        *sDataPlan->flag |= QMND_PROJ_FIRST_DONE_TRUE;

        // PROJ의 지정된 함수를 수행함.
        IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------
        // Child의 결과가 존재하지 않는 경우
        //-----------------------------------

        if ( ( ( sCodePlan->flag & QMNC_PROJ_MINMAX_MASK )
               == QMNC_PROJ_MINMAX_TRUE ) &&
             ( (*sDataPlan->flag & QMND_PROJ_FIRST_DONE_MASK )
               == QMND_PROJ_FIRST_DONE_FALSE ) )
        {
            // Indexable MIN-MAX 최적화가 적용되고,
            // Child로부터 어떠한 결과도 얻지 못했다면,
            // NULL값을 주어야 한다.

            // 한번은 수행되었음을 표시
            *sDataPlan->flag &= ~QMND_PROJ_FIRST_DONE_MASK;
            *sDataPlan->flag |= QMND_PROJ_FIRST_DONE_TRUE;

            // Child를 모두 Null Padding시킴
            IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
                      != IDE_SUCCESS );

            // PROJ의 지정된 함수를 수행함.
            IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag )
                      != IDE_SUCCESS );

            // Data가 존재함을 표시
            *aFlag &= ~QMC_ROW_DATA_MASK;
            *aFlag |= QMC_ROW_DATA_EXIST;
        }
        else
        {
            // nothing to do
        }

        // fix PR-2367
        // sDataPlan->doIt = qmnPROJ::doItDefault;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnPROJ::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    PROJ 에 해당하는 Null Row를 획득한다.
 *    일반적인 경우, Child Plan에 대한 Null Padding으로 충분하지만,
 *    Outer Column Reference가 존재하는 경우에는 이에 대한 Null Padding을
 *    별도로 수행해 주어야 한다.
 *
 * Implementation :
 *    Child Plan에 대한 Null Padding 없이 이미 구성한 Null Row를
 *    Stack에 구성하여 준다.
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::padNull"));

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    qmndPROJ * sDataPlan =
        (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    UInt       i;
    mtcStack * sStack;
    SInt       sRemain;

    // 적합성 검사
    IDE_ASSERT( (sCodePlan->flag & QMNC_PROJ_TOP_MASK)
                == QMNC_PROJ_TOP_FALSE );

    // 초기화가 수행되지 않은 경우 초기화를 수행
    if ( ( aTemplate->planFlag[sCodePlan->planID] & QMND_PROJ_INIT_DONE_MASK )
         == QMND_PROJ_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //fix BUG-17872
    if( sDataPlan->nullRow == NULL )
    {
        // Row의 최대 Size 계산
        IDE_TEST( getMaxRowSize( aTemplate,
                                 sCodePlan,
                                 &sDataPlan->rowSize ) != IDE_SUCCESS );

        // Null Padding을 위한 Null Row 생성
        IDE_TEST( makeNullRow( aTemplate, sCodePlan, sDataPlan )
                  != IDE_SUCCESS );
    }

    // fix BUG-9283
    // 하위 노드들에 대한 null padding 수행
    // keyRange를 구성할 경우, stack에 쌓인 값이 아닌
    // value node 정보를 참조하게 되므로.
    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    // 이미 구성한 Null Row를 Stack에 구성한다.
    for ( sStack  = aTemplate->tmplate.stack,
              sRemain = aTemplate->tmplate.stackRemain,
              i = 0;
          i < sCodePlan->targetCount;
          sStack++, sRemain--, i++ )
    {
        IDE_TEST_RAISE( sRemain < 1, ERR_STACK_OVERFLOW );

        sStack->column = & sDataPlan->nullColumn[i];
        sStack->value = (void *)mtc::value( & sDataPlan->nullColumn[i],
                                            sDataPlan->nullRow,
                                            MTD_OFFSET_USE );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnPROJ::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    PROJ 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::printPlan"));

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    qmndPROJ * sDataPlan =
        (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    ULong i;
    UInt  j;

    qmcAttrDesc * sItrAttr;

    //------------------------------------------------------
    // 시작 정보의 출력
    //------------------------------------------------------

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //------------------------------------------------------
    // PROJ Target 정보의 출력
    //------------------------------------------------------

    IDE_TEST( printTargetInfo( aTemplate,
                               sCodePlan,
                               aString ) != IDE_SUCCESS );

    //----------------------------
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    //----------------------------
    // PROJ-1473 target info 
    //----------------------------

    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }

        iduVarStringAppend( aString,
                            "[ TARGET INFO ]\n" );

        for( sItrAttr = sCodePlan->plan.resultDesc, j = 0;
             sItrAttr != NULL;
             sItrAttr = sItrAttr->next, j++ )
        {
            for ( i = 0; i < aDepth; i++ )
            {
                iduVarStringAppend( aString,
                                    " " );
            }

            if( sItrAttr->expr->node.arguments == NULL )
            {
                iduVarStringAppendFormat(
                    aString,
                    "sTargetColumn[%"ID_UINT32_FMT"] : [%"ID_INT32_FMT", %"ID_INT32_FMT"],"
                    "sTargetColumn->arg[X, X]\n",
                    j, 
                    (SInt)sItrAttr->expr->node.table,
                    (SInt)sItrAttr->expr->node.column
                    );
            }
            else
            {
                iduVarStringAppendFormat(
                    aString,
                    "sTargetColumn[%"ID_UINT32_FMT"] : [%"ID_INT32_FMT", %"ID_INT32_FMT"],"
                    "sTargetColumn->arg[%"ID_UINT32_FMT"] : [%"ID_INT32_FMT", %"ID_INT32_FMT"]\n",
                    j, 
                    (SInt)sItrAttr->expr->node.table,
                    (SInt)sItrAttr->expr->node.column,
                    j,
                    (SInt)sItrAttr->expr->node.arguments->table,
                    (SInt)sItrAttr->expr->node.arguments->column
                    );
            }
        }
    }    
    
    //------------------------------------------------------
    // Target 내부의 Subquery 정보 출력
    //------------------------------------------------------

    for (sItrAttr = sCodePlan->plan.resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next)
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sItrAttr->expr,
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

    //------------------------------------------------------
    // Child Plan 정보의 출력
    //------------------------------------------------------

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
qmnPROJ::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnPROJ::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::doItDefault"));

    IDE_DASSERT(0);

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnPROJ::doItProject( qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * /* aFlag */)
{
/***********************************************************************
 *
 * Description :
 *    PROJ 가 non-top projection일 때 다음 함수가 수행된다.
 *
 * Implementation :
 *    Target을 모두 수행하면서 Stack에 그 정보가 설정되도록 한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::doItProject"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::doItProject"));

    qmncPROJ    * sCodePlan = (qmncPROJ*) aPlan;
    mtcStack    * sStack;
    SInt          sRemain;
    qmcAttrDesc * sItrAttr;
    qtcNode     * sNode;

    // Stack 정보 저장
    sStack  = aTemplate->tmplate.stack;
    sRemain = aTemplate->tmplate.stackRemain;

    for ( sItrAttr = sCodePlan->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next,
              aTemplate->tmplate.stack++,
              aTemplate->tmplate.stackRemain-- )
    {
        if ( ( sItrAttr->flag & QMC_ATTR_USELESS_RESULT_MASK ) != QMC_ATTR_USELESS_RESULT_TRUE )
        {
            // Column이건 Expression이건 다음 함수 호출을 통해
            // Stack에 Column정보와 Value정보를 설정하게 된다.
            IDE_TEST( qtc::calculate( sItrAttr->expr, aTemplate )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( aTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW );

            /* PROJ-2469 Optimize View Materialization
             * 사용하지 않는 Column에 대해서는 calculate 하지 않는다.
             * Subquery와 같이 Indirect Node일 경우, Argument를 순회하여
             * 실제 Column이 있는 Node를 찾는다.
             */
            for ( sNode = sItrAttr->expr;
                  ( sNode->node.lflag & MTC_NODE_INDIRECT_MASK ) == MTC_NODE_INDIRECT_TRUE;
                  sNode = ( qtcNode* )sNode->node.arguments )
            {
                // Nothing to do.
            }

            // stack의 column을 해당 column으로 지정 해 준다.
            aTemplate->tmplate.stack->column = QTC_TMPL_COLUMN( aTemplate, sNode );

            // stack의 value를 해당 column의 staticNull로 지정 해준다.
            if ( aTemplate->tmplate.stack->column->module->staticNull == NULL )
            {
                // list type등과 같이 staticNull이 정의되지 않은 type인 경우 Calculate 한다. */
                IDE_TEST( qtc::calculate( sItrAttr->expr, aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                aTemplate->tmplate.stack->value =
                    aTemplate->tmplate.stack->column->module->staticNull;
            }
        }
    }
    
    // Stack정보 복원
    aTemplate->tmplate.stack       = sStack;
    aTemplate->tmplate.stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    aTemplate->tmplate.stack       = sStack;
    aTemplate->tmplate.stackRemain = sRemain;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnPROJ::doItTopProject( qcTemplate * aTemplate,
                         qmnPlan    * aPlan,
                         qmcRowFlag * /* aFlag */)
{
/***********************************************************************
 *
 * Description :
 *    PROJ 가 top projection일 때 다음 함수가 수행된다.
 *
 * Implementation :
 *    Target을 모두 수행하면서 그 실제 크기를 얻어 통신 버퍼를
 *    최대한 절약하도록 한다.  예를 들어, Variable Column의
 *    실제 데이타의 공간 크기만큼만 이용함을 고려한다.
 *    Top Projection의 최상위 qtcNode는 모두 Assign노드이며,
 *    해당 Node 수행시 자연스럽게 통신 버퍼에 기록된다.
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::doItTopProject"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::doItTopProject"));

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    qmndPROJ * sDataPlan =
        (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    mtcColumn   * sColumn;
    SChar       * sValue;
    mtcStack    * sStack;
    SInt          sRemain;
    qmcAttrDesc * sItrAttr;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    SLong       sLobSize = 0;
    UInt        sLobCacheThreshold = 0;
    idBool      sIsNullLob;

    sLobCacheThreshold = QCG_GET_LOB_CACHE_THRESHOLD( aTemplate->stmt );

    sDataPlan->tupleOffset = 0;

    // Stack 정보 저장
    sStack  = aTemplate->tmplate.stack;
    sRemain = aTemplate->tmplate.stackRemain;

    // 각 Target에 대한 Projection 수행
    for ( sItrAttr = sCodePlan->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next,
          aTemplate->tmplate.stack++,
          aTemplate->tmplate.stackRemain-- )
    {
        //fix BUG-17713
        //sNode는 더이상 Assign Node가 아니다.
        //Assign Node임을 가정한 코드는 모두 제거한다.

        // Node를 수행한다.
        IDE_TEST( qtc::calculate( sItrAttr->expr, aTemplate )
                  != IDE_SUCCESS );

        //fix BUG-17713
        // sNode가 PASS 노드 이면 sNode->dstColumn->module->actualSize는 NULL
        // 이다. 따라서 실제 stack에 저장된 column, value의 값을 사용한다.
        sColumn = aTemplate->tmplate.stack->column;
        sValue = (SChar *)aTemplate->tmplate.stack->value;

        // 실제 데이터의 길이만큼만 offset을 계산한다.
        sDataPlan->tupleOffset +=
            sColumn->module->actualSize( sColumn,
                                         sValue );

        /* PROJ-2160
           tupleOffset은 row의 사이즈를 의미한다.
           LOB_LOCATOR_ID 의 경우 size(4) + locator(8) 로 전송되므로
           4만큼 더해주어야 한다. */
        if ( (sColumn->module->id == MTD_BLOB_LOCATOR_ID) ||
             (sColumn->module->id == MTD_CLOB_LOCATOR_ID) )
        {
            /* 
             * PROJ-2047 Strengthening LOB - LOBCACHE
             * 
             * LOBSize(8)와 HasData(1)를 더해주어야 한다.
             */
            sDataPlan->tupleOffset += ID_SIZEOF(ULong);
            sDataPlan->tupleOffset += ID_SIZEOF(UChar);

            if (sLobCacheThreshold > 0)
            {
                sLobSize = 0;
                if ( *(smLobLocator*)sValue != MTD_LOCATOR_NULL )
                {
                    (void)smiLob::getLength( QC_STATISTICS(aTemplate->stmt),
                                             *(smLobLocator*)sValue,
                                             &sLobSize,
                                             &sIsNullLob );

                    /* 임계치내에 해당하면 LOBData 사이즈도 더해 준다 */
                    if (sLobSize <= sLobCacheThreshold)
                    {
                        sDataPlan->tupleOffset += sLobSize;
                    }
                    else
                    {
                        /* Nothing */
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing */
            }
        }
        else
        {
            // nothing todo
        }
    }

    // Stack정보 복원
    aTemplate->tmplate.stack       = sStack;
    aTemplate->tmplate.stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // fix BUG-33660
    aTemplate->tmplate.stack       = sStack;
    aTemplate->tmplate.stackRemain = sRemain;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnPROJ::doItWithLimit( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag)
{
/***********************************************************************
 *
 * Description :
 *    Limit과 함께 사용될 때 호출되는 함수이다.
 *    doItProject(), doItTopProject()와 함께 사용되며,
 *    Limitation에 대한 처리만 담당한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::doItWithLimit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::doItWithLimit"));

    // qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    qmndPROJ * sDataPlan =
        (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    for ( ;
          sDataPlan->limitCurrent < sDataPlan->limitStart;
          sDataPlan->limitCurrent++ )
    {
        // Limitation 범위에 들지 않는다.
        // 따라서 Projection없이 Child를 수행하기만 한다.
        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
                  != IDE_SUCCESS );

        if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
        {
            break;
        }
    }

    if ( sDataPlan->limitCurrent >= sDataPlan->limitStart &&
         sDataPlan->limitCurrent < sDataPlan->limitEnd )
    {
        // Limitation 범위 안에 있는 경우
        // Projection을 수행한다.
        // doItProject() 또는 doItTopProject() 함수가 연결되어 있다.
        IDE_TEST( sDataPlan->limitExec( aTemplate,
                                        aPlan,
                                        aFlag ) != IDE_SUCCESS );

        // Limit값 증가
        sDataPlan->limitCurrent++;
    }
    else
    {
        // Limitation 범위를 벗어난 경우
        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnPROJ::getRowSize( qcTemplate * aTemplate,
                     qmnPlan    * aPlan,
                     UInt       * aSize )
{
/***********************************************************************
 *
 * Description :
 *    Target을 구성하는 Column들의 최대 Size를 획득한다.
 *    Communication Buffer가 충분한지를 확인하기 위해
 *    사용한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::getRowSize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::getRowSize"));

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    // qmndPROJ * sDataPlan =
    //     (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    // To Fix PR-7988
    // Prepare Protocol인 경우 ::init()없이 해당 함수를
    // 호출하게 된다.  따라서, 항상 계산해 주어야 한다.
    IDE_TEST( getMaxRowSize ( aTemplate, sCodePlan, aSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnPROJ::getCodeTargetPtr( qmnPlan    * aPlan,
                           qmsTarget ** aTarget)
{
/***********************************************************************
 *
 * Description :
 *    Top Projection일 경우에만 사용되며,
 *    MM 단에서
 *    Client의 Target Column정보를 구성하기 위하여 사용한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::getCodeTargetPtr"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::getCodeTargetPtr"));

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    // qmndPROJ * sDataPlan =
    //     (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    *aTarget = sCodePlan->myTarget;

    return IDE_SUCCESS;

#undef IDE_FN
}

ULong
qmnPROJ::getActualSize( qcTemplate * aTemplate,
                        qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    하나의 Target Row완료 후 실제 처리한 크기를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::getActualSize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::getActualSize"));

    qmndPROJ * sDataPlan =
        (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->tupleOffset;

#undef IDE_FN
}

IDE_RC
qmnPROJ::firstInit( qcTemplate * aTemplate,
                    qmncPROJ   * aCodePlan,
                    qmndPROJ   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    PROJ node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *    - Data 영역의 주요 멤버에 대한 초기화를 수행
 *
 ***********************************************************************/

    ULong sCount;
    UInt  sReserveCnt = 0;

    //--------------------------------
    // 적합성 검사
    //--------------------------------

    IDE_ASSERT( aCodePlan->myTarget != NULL );
    IDE_ASSERT( aCodePlan->myTargetOffset > 0 );

    //--------------------------------
    // PROJ 고유 정보의 초기화
    //--------------------------------

    // Tuple Set정보의 초기화
    // Top Projection일 경우에만 유효하며,
    // plan.myTuple은 통신 버퍼내의 메모리 영역을 사용하게 된다.
    // ===>
    // PROJ-1461
    // cm에서는 통신 버퍼내의 메모리 영역을 직접 사용할 수 없으므로,
    // plan.myTuple에 fetch된 데이타를 저장할 메모리 공간을 할당 받는다.
    // mm에서 qci::fetchColumn시 하나의 레코드 전체에 대해서
    // 데이타를 fetch 하는 것이 아니라,
    // 하나의 컬럼에 대해서 fetch하기때문에,
    // PROJ 노드에서 레코드 단위로 데이타를 저장하고 있어야 한다.
    aDataPlan->plan.myTuple = & aTemplate->tmplate.
        rows[aCodePlan->myTarget->targetColumn->node.table];

    //--------------------------------
    // Limitation 관련 정보의 초기화
    //--------------------------------
    if( aCodePlan->limit != NULL )
    {
        IDE_TEST( qmsLimitI::getStartValue(
                      aTemplate,
                      aCodePlan->limit,
                      &aDataPlan->limitStart )
                  != IDE_SUCCESS );

        IDE_TEST( qmsLimitI::getCountValue(
                      aTemplate,
                      aCodePlan->limit,
                      &sCount )
                  != IDE_SUCCESS );

        if ( aDataPlan->limitStart > 0 )
        {
            aDataPlan->limitEnd = aDataPlan->limitStart + sCount;
        }
        else
        {
            aDataPlan->limitEnd = 0;
        }
    }
    else
    {
        aDataPlan->limitStart = 1;
        aDataPlan->limitEnd = 0;
    }

    // 적합성 검사
    if ( aDataPlan->limitEnd > 0 )
    {
        IDE_ASSERT( (aCodePlan->flag & QMNC_PROJ_LIMIT_MASK)
                    == QMNC_PROJ_LIMIT_TRUE );
    }

    //--------------------------------
    // Loop 관련 정보의 초기화
    //--------------------------------
    if ( aCodePlan->loopNode != NULL )
    {
        IDE_TEST( qtc::getLoopCount( aTemplate,
                                     aCodePlan->loopNode,
                                     & aDataPlan->loopCount )
                  != IDE_SUCCESS );
        aDataPlan->loopCurrent = 0;

        if ( aCodePlan->loopLevel != NULL )
        {
            aDataPlan->loopCurrentPtr = (SLong*)
                aTemplate->tmplate.rows[aCodePlan->loopLevel->node.table].row;
        }
        else
        {
            aDataPlan->loopCurrentPtr = NULL;
        }
    }
    else
    {
        aDataPlan->loopCount   = 1;
        aDataPlan->loopCurrent = 0;
        aDataPlan->loopCurrentPtr = NULL;
    }
        
    //---------------------------------
    // Null Row 관련 정보의 초기화
    //---------------------------------

    aDataPlan->nullRow = NULL;
    aDataPlan->nullColumn = NULL;

    /* BUG-48776
     * One row return unkown 계열의 scalar subquery result backup 정보의 초기화
     */
    aDataPlan->mKeepValue = NULL;
    aDataPlan->mKeepColumn = NULL;

    /*
     * PROJ-1071 Parallel Query
     * thread reserve
     */
    if ( ((aCodePlan->plan.flag & QMN_PLAN_PRLQ_EXIST_MASK) ==
         QMN_PLAN_PRLQ_EXIST_TRUE) &&
         ((aCodePlan->flag & QMNC_PROJ_QUERYSET_TOP_MASK) ==
          QMNC_PROJ_QUERYSET_TOP_TRUE) &&
         // BUG-41279
         // Prevent parallel execution while executing 'select for update' clause.
         ((aTemplate->stmt->spxEnv->mFlag & QSX_ENV_DURING_SELECT_FOR_UPDATE) !=
          QSX_ENV_DURING_SELECT_FOR_UPDATE) )
    {
        IDE_TEST(qcg::reservePrlThr(aCodePlan->plan.mParallelDegree,
                                    &sReserveCnt)
                 != IDE_SUCCESS);

        if (sReserveCnt >= 1)
        {
            IDE_TEST(qmcThrObjCreate(aTemplate->stmt->mThrMgr,
                                     sReserveCnt)
                     != IDE_SUCCESS);
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_PROJ_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_PROJ_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sReserveCnt > 0)
    {
        (void)qcg::releasePrlThr(sReserveCnt);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC
qmnPROJ::getMaxRowSize( qcTemplate * aTemplate,
                        qmncPROJ   * aCodePlan,
                        UInt       * aSize )
{
/***********************************************************************
 *
 * Description :
 *    Target Row의 최대 Size를 계산한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt          sSize;
    qtcNode     * sNode;
    mtcColumn   * sMtcColumn;
    qmcAttrDesc * sItrAttr;

    sSize = 0;

    for ( sItrAttr = aCodePlan->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next )
    {
        // 실제 Target Column을 획득
        sNode = (qtcNode*)mtf::convertedNode(&sItrAttr->expr->node,
                                             &aTemplate->tmplate );
        sMtcColumn = QTC_TMPL_COLUMN(aTemplate, sNode);

        sSize = idlOS::align(sSize, sMtcColumn->module->align);
        sSize += sMtcColumn->column.size;
    }

    // 적합성 검사
    IDE_ASSERT(sSize > 0);

    *aSize = sSize;

    return IDE_SUCCESS;
}

IDE_RC
qmnPROJ::makeNullRow( qcTemplate * aTemplate,
                      qmncPROJ   * aCodePlan,
                      qmndPROJ   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    SELECT 구문 수행의 결과가 없을 경우,
 *    Null Padding을 호출할 수 있다.
 *    이 때, SELECT target에 Outer Column Reference가 존재한다면,
 *    이에 대한 Null Value는 PROJ 노드에서 제공하여야 한다.
 *    Ex) UPDATE T1 SET i1 = ( SELECT T1.i2 FROM T2 LIMIT 1 );
 *                                    ^^^^^
 *
 * Implementation :
 *    최초 한 번만 수행한다.
 *    - Null Row를 위한 공간 확보
 *    - Null Column을 위한 공간 확보
 *    - 각 Null Column의 정보를 구성하고 Null Value생성
 ***********************************************************************/

#define IDE_FN "qmnPROJ::makeNullRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnPROJ::makeNullRow"));

    UInt        i;
    ULong       sTupleOffset = 0;

    qmsTarget * sTarget;
    qtcNode   * sNode;
    mtcColumn * sColumn;

    // Null Row를 위한 공간 확보
    IDU_FIT_POINT( "qmnPROJ::makeNullRow::cralloc::nullRow",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aTemplate->stmt->qmxMem->cralloc( aDataPlan->rowSize,
                                                (void**) & aDataPlan->nullRow )
              != IDE_SUCCESS);

    // Null Column을 위한 공간 확보
    IDU_FIT_POINT( "qmnPROJ::makeNullRow::alloc::nullColumn",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aTemplate->stmt->qmxMem->alloc( aCodePlan->targetCount * ID_SIZEOF(mtcColumn),
                                              (void**) & aDataPlan->nullColumn )
              != IDE_SUCCESS);

    for ( sTarget = aCodePlan->myTarget, i = 0;
          sTarget != NULL;
          sTarget = sTarget->next, i++ )
    {
        //-------------------------------------------
        // 각 Null Column에 대하여 Null Value를 생성
        //-------------------------------------------

        // 실제 Target Column정보를 획득
        sNode = (qtcNode*)mtf::convertedNode( & sTarget->targetColumn->node,
                                              & aTemplate->tmplate );
        sColumn = & aTemplate->tmplate.rows[sNode->node.table].
            columns[sNode->node.column];

        // Column 정보 복사
        // Variable Column의 Null Value 획득을 위하여
        // 새로 생성한 NULL Column을 모두 Fixed로 지정한다.
        mtc::copyColumn( & aDataPlan->nullColumn[i],
                         sColumn );
        
        // BUG-38494
        // Compressed Column 역시 값 자체가 저장되므로
        // Compressed 속성을 삭제한다
        aDataPlan->nullColumn[i].column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
        aDataPlan->nullColumn[i].column.flag |= SMI_COLUMN_COMPRESSION_FALSE;

        // Null Column이 offset 재설정
        sTupleOffset =
            idlOS::align( sTupleOffset, sColumn->module->align );

        aDataPlan->nullColumn[i].column.offset = sTupleOffset;

        // To Fix PR-8005
        // Null Value 생성
        aDataPlan->nullColumn[i].module->null(
            & aDataPlan->nullColumn[i],
            (void*) ( (SChar*) aDataPlan->nullRow + sTupleOffset) );

        // Offset증가
        // fix BUG-31822
        sTupleOffset += sColumn->column.size;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnPROJ::initLevel( qcTemplate * aTemplate,
                    qmncPROJ   * aCodePlan )
{
/***********************************************************************
 *
 * Description :
 *    LEVEL pseudo column의 값 초기화
 *
 * Implementation :
 *    LEVEL pseudo column이 존재할 경우 이 값을 초기화한다.
 *
 ***********************************************************************/

    qtcNode  * sLevel   = aCodePlan->level;
    qtcNode  * sIsLeaf  = aCodePlan->isLeaf;

    if (sLevel != NULL)
    {
        *(mtdBigintType *)(aTemplate->tmplate.rows[sLevel->node.table].row) = 0;
    }
    else
    {
        /* Nothing to do */
    }

    if (sIsLeaf != NULL)
    {
        *(mtdBigintType *)(aTemplate->tmplate.rows[sIsLeaf->node.table].row) = 0;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

}

IDE_RC
qmnPROJ::setDoItFunction( qmncPROJ   * aCodePlan,
                          qmndPROJ   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Top Projection의 여부와 Limit의 존재 여부에 따라
 *    수행 함수를 결정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( ( aCodePlan->flag & QMNC_PROJ_TOP_RESULT_CACHE_MASK )
         == QMNC_PROJ_TOP_RESULT_CACHE_FALSE )
    {
        switch ( ( aCodePlan->flag &
                 ( QMNC_PROJ_TOP_MASK| QMNC_PROJ_LIMIT_MASK ) ) )
        {
            case (QMNC_PROJ_TOP_TRUE | QMNC_PROJ_LIMIT_TRUE):
                {
                    aDataPlan->doIt = qmnPROJ::doItWithLimit;
                    aDataPlan->limitExec = qmnPROJ::doItTopProject;
                    break;
                }
            case (QMNC_PROJ_TOP_TRUE | QMNC_PROJ_LIMIT_FALSE):
                {
                    aDataPlan->doIt = qmnPROJ::doItTopProject;
                    break;
                }
            case (QMNC_PROJ_TOP_FALSE| QMNC_PROJ_LIMIT_TRUE):
                {
                    aDataPlan->doIt = qmnPROJ::doItWithLimit;
                    aDataPlan->limitExec = qmnPROJ::doItProject;
                    break;
                }
            case (QMNC_PROJ_TOP_FALSE | QMNC_PROJ_LIMIT_FALSE):
                {
                    aDataPlan->doIt = qmnPROJ::doItProject;
                    break;
                }
            default:
                {
                    IDE_DASSERT( 0 );
                    break;
                }
        }
    }
    else
    {
        aDataPlan->doIt = qmnPROJ::doItTopProject;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmnPROJ::readSequence( qcTemplate * aTemplate,
                       qmncPROJ   * aCodePlan,
                       qmndPROJ   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    필요한 경우, Sequence의 next value값을 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/
#define IDE_FN "qmnPROJ::readSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // SEQUENCE : Not to split doItFunctions any more
    // This is Better...
    if (aCodePlan->nextValSeqs != NULL)
    {
        if ( ( *aDataPlan->flag & QMND_PROJ_FIRST_DONE_MASK )
             == QMND_PROJ_FIRST_DONE_FALSE )
        {
            IDE_TEST(qmx::addSessionSeqCaches(aTemplate->stmt,
                                              aTemplate->stmt->myPlan->parseTree)
                     != IDE_SUCCESS);
        }
        else
        {
            // Nothing To Do
        }

        IDE_TEST(qmx::readSequenceNextVals(
                     aTemplate->stmt, aCodePlan->nextValSeqs)
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

void qmnPROJ::setLoopCurrent( qcTemplate * aTemplate,
                              qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *     loop current 값을 증가시키고 loop level pseudo column 값을 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;
    qmndPROJ * sDataPlan =
        (qmndPROJ*) (aTemplate->tmplate.data + aPlan->offset);

    if ( sCodePlan->loopNode != NULL )
    {
        sDataPlan->loopCurrent++;

        if ( sDataPlan->loopCurrentPtr != NULL )
        {
            *sDataPlan->loopCurrentPtr = sDataPlan->loopCurrent;
        }
        else
        {
            // Nothing to do.
        }
        
        if ( sDataPlan->loopCurrent >= sDataPlan->loopCount )
        {
            sDataPlan->loopCurrent = 0;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC
qmnPROJ::printTargetInfo( qcTemplate   * aTemplate,
                          qmncPROJ     * aCodePlan,
                          iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *     Target 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnPROJ::printTargetInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sCount = 0;
    UInt        sRowSize;
    qmsTarget * sNode;

    // To Fix PR-8271
    // explain plan = only; 인 경우
    // Data영역의 정보가 없음.  별도로 다시 계산함
    for ( sNode = aCodePlan->myTarget; sNode != NULL;
          sNode = sNode->next )
    {
        sCount++;
    }

    // Data영역의 정보가 없을 수 있음. 별도로 다시 계산함
    IDE_TEST( getMaxRowSize( aTemplate, aCodePlan, & sRowSize )
              != IDE_SUCCESS );

    // PROJ 정보의 출력
    iduVarStringAppendFormat( aString,
                              "PROJECT ( COLUMN_COUNT: %"ID_UINT32_FMT", "
                              "TUPLE_SIZE: %"ID_UINT32_FMT,
                              sCount,
                              sRowSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

UInt qmnPROJ::getTargetCount( qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description : PROJ-1075 project node의 target count를 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/
#define IDE_FN "qmnPROJ::getTargetCount"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qmncPROJ * sCodePlan = (qmncPROJ*) aPlan;

    return sCodePlan->targetCount;

#undef IDE_FN
}

IDE_RC qmnPROJ::getVMTRInfo( qcTemplate * aTemplate,
                             qmncPROJ   * aCodePlan,
                             qmndPROJ   * aDataPlan )
{
    void * sTableHandle = NULL;
    void * sIndexHandle = NULL;

    IDE_TEST( qmnVMTR::getNullRowSize( aTemplate,
                                       aCodePlan->plan.left,
                                       &aDataPlan->nullRowSize )
              != IDE_SUCCESS );

    IDE_DASSERT( aDataPlan->nullRowSize > 0 );

    IDE_TEST( qmnVMTR::getNullRowMemory( aTemplate,
                                         aCodePlan->plan.left,
                                         &aDataPlan->nullRow )
              != IDE_SUCCESS );

    IDE_TEST( qmnVMTR::getCursorInfo( aTemplate,
                                      aCodePlan->plan.left,
                                      &sTableHandle,
                                      &sIndexHandle,
                                      &aDataPlan->memSortMgr,
                                      &aDataPlan->memSortRecord )
              != IDE_SUCCESS );
    IDE_TEST( qmcMemSort::getNumOfElements( aDataPlan->memSortMgr,
                                            &aDataPlan->recordCnt )
              != IDE_SUCCESS );

    aDataPlan->recordPos = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPROJ::setTupleSet( qcTemplate * aTemplate,
                             qmncPROJ   * aCodePlan,
                             qmndPROJ   * aDataPlan )
{
    qmdMtrNode * sNode       = NULL;
    mtcTuple   * sVMTRTuple  = NULL;
    mtcColumn  * sMyColumn   = NULL;
    mtcColumn  * sVMTRColumn = NULL;
    UInt         i           = 0;
    
    // PROJ-2362 memory temp 저장 효율성 개선
    // VSCN tuple 복원
    if ( aDataPlan->memSortRecord != NULL )
    {
        for ( sNode = aDataPlan->memSortRecord;
              sNode != NULL;
              sNode = sNode->next )
        {
            IDE_TEST( sNode->func.setTuple( aTemplate,
                                            sNode,
                                            aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }

        IDE_TEST( qmnVMTR::getTuple( aTemplate,
                                     aCodePlan->plan.left,
                                     & sVMTRTuple )
                  != IDE_SUCCESS );
        
        sMyColumn = aDataPlan->plan.myTuple->columns;
        sVMTRColumn = sVMTRTuple->columns;
        for ( i = 0; i < sVMTRTuple->columnCount; i++, sMyColumn++, sVMTRColumn++ )
        {
            if ( SMI_COLUMN_TYPE_IS_TEMP( sMyColumn->column.flag ) == ID_TRUE )
            {
                IDE_DASSERT( sVMTRColumn->module->actualSize(
                                 sVMTRColumn,
                                 sVMTRColumn->column.value )
                             <= sVMTRColumn->column.size );
                
                idlOS::memcpy( (SChar*)sMyColumn->column.value,
                               (SChar*)sVMTRColumn->column.value,
                               sVMTRColumn->module->actualSize(
                                   sVMTRColumn,
                                   sVMTRColumn->column.value ) );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPROJ::doItVMTR( qcTemplate * aTemplate,
                          qmnPlan    * aPlan,
                          qmcRowFlag * aFlag )
{
    qmncPROJ    * sCodePlan = (qmncPROJ*) aPlan;
    qmndPROJ    * sDataPlan = ( qmndPROJ * )( aTemplate->tmplate.data + aPlan->offset );

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    if ( sDataPlan->recordPos < sDataPlan->recordCnt )
    {
        IDE_TEST( qmcMemSort::getElement( sDataPlan->memSortMgr,
                                          sDataPlan->recordPos,
                                          &sDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
        IDE_DASSERT( sDataPlan->plan.myTuple->row != NULL );

        IDE_TEST( setTupleSet( aTemplate, sCodePlan, sDataPlan )
                  != IDE_SUCCESS );

        sDataPlan->recordPos++;
        sDataPlan->plan.myTuple->modify++;

        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

