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
 * $Id: qmnView.cpp 86122 2019-09-04 07:20:21Z donovan.seo $
 *
 * Description :
 *     VIEW(VIEW) Node
 *
 *     관계형 모델에서 가상 Table을 표현하기 위한 Node이다..
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnView.h>

IDE_RC 
qmnVIEW::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    VIEW 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVIEW::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncVIEW * sCodePlan = (qmncVIEW*) aPlan;
    qmndVIEW * sDataPlan = 
        (qmndVIEW*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnVIEW::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_VIEW_INIT_DONE_MASK)
         == QMND_VIEW_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }

    // init left child
    IDE_TEST( aPlan->left->init( aTemplate, 
                                 aPlan->left ) 
              != IDE_SUCCESS);

    // set doIt function
    sDataPlan->doIt = qmnVIEW::doItProject;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC 
qmnVIEW::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    VIEW의 고유 기능을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVIEW::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndVIEW * sDataPlan = 
        (qmndVIEW*) (aTemplate->tmplate.data + aPlan->offset);

    // BUG-42277
    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );
    
    // doIt left child
    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag ) 
              != IDE_SUCCESS );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    sDataPlan->plan.myTuple->lflag &= ~MTC_TUPLE_VIEW_PADNULL_MASK;
    sDataPlan->plan.myTuple->lflag |= MTC_TUPLE_VIEW_PADNULL_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnVIEW::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *     VIEW의 Tuple Set에 Null Padding을 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVIEW::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncVIEW * sCodePlan = (qmncVIEW*) aPlan;
    qmndVIEW * sDataPlan = 
        (qmndVIEW*) (aTemplate->tmplate.data + aPlan->offset);

    mtcColumn * sColumn;
    mtcNode   * sNode;
    void      * sValueTemp;

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_VIEW_INIT_DONE_MASK)
         == QMND_VIEW_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }

    for ( sNode = & sCodePlan->myNode->node,
              sColumn = sDataPlan->plan.myTuple->columns; 
          sNode != NULL; 
          sNode = sNode->next, sColumn++ )
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        sValueTemp = (void *) mtc::value( sColumn,
                                          sDataPlan->plan.myTuple->row,
                                          MTD_OFFSET_USE );
        
        // To Fix PR-8005
        sColumn->module->null( (const mtcColumn *) sColumn, 
                               sValueTemp );
    }

    sDataPlan->plan.myTuple->lflag &= ~MTC_TUPLE_VIEW_PADNULL_MASK;
    sDataPlan->plan.myTuple->lflag |= MTC_TUPLE_VIEW_PADNULL_TRUE;

    // Null Padding도 record가 변한 것임
    sDataPlan->plan.myTuple->modify++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnVIEW::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    VIEW 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVIEW::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SChar      sNameBuffer[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    qmncVIEW * sCodePlan = (qmncVIEW*) aPlan;
    qmndVIEW * sDataPlan = 
        (qmndVIEW*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    ULong      i;
    UInt       j;
    qtcNode  * sNode;        

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //----------------------------
    // View 이름 정보 출력
    //----------------------------

    iduVarStringAppend( aString,
                        "VIEW ( " );

    if ( sCodePlan->viewName.name != NULL &&
         sCodePlan->viewName.size != QC_POS_EMPTY_SIZE )
    {
        if ( ( sCodePlan->viewOwnerName.name != NULL ) &&
             ( sCodePlan->viewOwnerName.size > 0 ) )
        {
            iduVarStringAppend( aString,
                                sCodePlan->viewOwnerName.name );
            iduVarStringAppend( aString,
                                "." );
        }
        else
        {
            // Nothing to do.
        }
        
        if ( sCodePlan->viewName.size <= QC_MAX_OBJECT_NAME_LEN )
        {
            idlOS::memcpy( sNameBuffer,
                           sCodePlan->viewName.name,
                           sCodePlan->viewName.size );
            sNameBuffer[sCodePlan->viewName.size] = '\0';

            iduVarStringAppend( aString,
                                sNameBuffer );
        }
        else
        {
            // Nothing to do.
        }

        // View의 Alias 이름 출력
        if ( sCodePlan->aliasName.name != NULL &&
             sCodePlan->aliasName.size != QC_POS_EMPTY_SIZE &&
             sCodePlan->aliasName.name != sCodePlan->viewName.name )
        {
            // View 이름 정보와 Alias 이름 정보가 다를 경우
            // (alias name)
            iduVarStringAppend( aString,
                                " " );

            if ( sCodePlan->aliasName.size <= QC_MAX_OBJECT_NAME_LEN )
            {
                idlOS::memcpy( sNameBuffer,
                               sCodePlan->aliasName.name,
                               sCodePlan->aliasName.size );
                sNameBuffer[sCodePlan->aliasName.size] = '\0';

                iduVarStringAppend( aString,
                                    sNameBuffer );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Alias 이름 정보가 없거나 Table 이름 정보가 동일한 경우
            // Nothing To Do 
        }

        iduVarStringAppend( aString,
                            ", " );
    }
    else
    {
        // Nothing To Do 
    }

    
    //----------------------------
    // Access 정보 출력
    //----------------------------
    
    if ( aMode == QMN_DISPLAY_ALL )
    {
        if ( (*sDataPlan->flag & QMND_VIEW_INIT_DONE_MASK)
             == QMND_VIEW_INIT_DONE_TRUE )
        {
            iduVarStringAppendFormat( aString,
                                      "ACCESS: %"ID_UINT32_FMT,
                                      sDataPlan->plan.myTuple->modify );
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "ACCESS: 0" );
        }
    }
    else
    {
        iduVarStringAppendFormat( aString,
                                  "ACCESS: ??" );
    }

    //----------------------------
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    //----------------------------
    // PROJ-1473 mtrNode info 
    //----------------------------

    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }

        iduVarStringAppend( aString,
                            "[ MTRNODE INFO ]\n" );    
    
        for( sNode = sCodePlan->myNode, j = 0;
             sNode != NULL;
             sNode = (qtcNode*)(sNode->node.next), j++ )
        {
            for ( i = 0; i < aDepth; i++ )
            {
                iduVarStringAppend( aString,
                                    " " );
            }

            if( sNode->node.arguments == NULL )
            {
                iduVarStringAppendFormat(
                    aString,
                    "sNode[%"ID_UINT32_FMT"] : [%"ID_INT32_FMT", %"ID_INT32_FMT"] \n",
                    j, 
                    (SInt)sNode->node.table,
                    (SInt)sNode->node.column
                    );
            }
            else
            {
                iduVarStringAppendFormat(
                    aString,
                    "sNode[%"ID_UINT32_FMT"] : [%"ID_INT32_FMT", %"ID_INT32_FMT"] \n",
                    "sNode[%"ID_UINT32_FMT"]->arguments : [%"ID_INT32_FMT", %"ID_INT32_FMT"] \n",
                    j,
                    (SInt)sNode->node.table,
                    (SInt)sNode->node.column,
                    j,
                    (SInt)sNode->node.arguments->table,
                    (SInt)sNode->node.arguments->column
                    );
            }        
        }    
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
qmnVIEW::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnVIEW::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnVIEW::doItProject(  qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * /* aFlag */)
{
/***********************************************************************
 *
 * Description :
 *    하위 PROJ 노드의 결과를 Tuple Set에 구성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVIEW::doItProject"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncVIEW * sCodePlan = (qmncVIEW*) aPlan;
    
    qmndVIEW * sDataPlan = 
        (qmndVIEW*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( setTupleRow( aTemplate, sCodePlan, sDataPlan ) != IDE_SUCCESS );
              
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnVIEW::firstInit( qcTemplate * aTemplate,
                    qmncVIEW   * aCodePlan,
                    qmndVIEW   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    VIEW node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVIEW::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduMemory * sMemory;

    sMemory = aTemplate->stmt->qmxMem;

    //---------------------------------
    // VIEW 고유 정보의 설정
    //---------------------------------
    
    aDataPlan->plan.myTuple = 
        & aTemplate->tmplate.rows[aCodePlan->myNode->node.table];

    // 적합성 검사
    IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_VIEW_MASK)
                 == MTC_TUPLE_VIEW_TRUE );
    IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                 == MTC_TUPLE_STORAGE_MEMORY );
    IDE_DASSERT( aDataPlan->plan.myTuple->rowOffset > 0 );

    //---------------------------------
    // Tuple을 위한 공간 할당
    //---------------------------------

    aDataPlan->plan.myTuple->row = NULL;
    
    IDU_FIT_POINT( "qmnVIEW::firstInit::alloc::myTupleRow",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( sMemory->alloc( aDataPlan->plan.myTuple->rowOffset,
                              (void**)&(aDataPlan->plan.myTuple->row) )
              != IDE_SUCCESS);
    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_VIEW_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_VIEW_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC 
qmnVIEW::setTupleRow( qcTemplate * aTemplate,
                      qmncVIEW   * aCodePlan,
                      qmndVIEW   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Stack에 쌓인 정보를 이용하여 View Record를 구성함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVIEW::setTupleRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode   * sNode;
    mtcColumn * sColumn;
    mtcStack  * sStack;
    SInt        sRemain;

    sStack = aTemplate->tmplate.stack;
    sRemain = aTemplate->tmplate.stackRemain;

    for ( sNode = aCodePlan->myNode, sColumn = aDataPlan->plan.myTuple->columns; 
          sNode != NULL; 
          sNode = (qtcNode*) sNode->node.next,
              sColumn++,
              aTemplate->tmplate.stack++,
              aTemplate->tmplate.stackRemain-- )
    {
        IDE_TEST_RAISE(aTemplate->tmplate.stackRemain < 1,
                       ERR_STACK_OVERFLOW);

        idlOS::memcpy( 
            (SChar*) aDataPlan->plan.myTuple->row + sColumn->column.offset,
            (SChar*) aTemplate->tmplate.stack->value,
            aTemplate->tmplate.stack->column->module->actualSize(
                aTemplate->tmplate.stack->column,
                aTemplate->tmplate.stack->value ) );
    }

    aTemplate->tmplate.stack = sStack;
    aTemplate->tmplate.stackRemain = sRemain;
    
    aDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;
    
    aTemplate->tmplate.stack = sStack;
    aTemplate->tmplate.stackRemain = sRemain;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

