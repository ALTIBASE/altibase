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
 * $Id: qsfGrouping.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * GROUPING ( Expression )
 *  Group By 에 해당하는  Expression
 *  GROUP By ROLLUP, CUBE 구문이 항상 나와야한다.
 *  sSFWGH->groupingDataAddr 의 Pseudo Column에 Rollup이나 Cube의
 *  포인터가 있다. 이를 통해서 참조 데이터를 얻어서 계산한다.
 ***********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <qmv.h>
#include <mtc.h>
#include <qmnRollup.h>
#include <qmnCube.h>

extern mtdModule mtdInteger;

static mtcName qsfGroupingFunctionName[1] = {
        { NULL, 8, (void *)"GROUPING" }
};

static IDE_RC qsfGroupingEstimate( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcCallBack * aCallback );

IDE_RC qsfGroupingCalculate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

mtfModule qsfGroupingModule = {
    1 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTON_GROUPING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,
    qsfGroupingFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfGroupingEstimate
};


typedef struct qsfGroupingInfo
{
    void * address;
    SInt   location;
} qsfGroupingInfo;

IDE_RC qsfGroupingInitialize(  mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate );

IDE_RC qsfGroupingFinalize(  mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

IDE_RC qsfGroupingAggregate(  mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate );

const mtcExecute qsfExecute = {
    qsfGroupingInitialize,
    qsfGroupingAggregate,
    mtf::calculateNA,
    qsfGroupingFinalize,
    qsfGroupingCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


static IDE_RC qsfGroupingInitializeColumn( mtcNode     * aNode,
                                           mtcTemplate * aTemplate,
                                           mtcStack    * aStack )
{
    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;
    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                              NULL;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC qsfGroupingMakeInfo( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   qmsSFWGH    * aSFWGH,
                                   mtcCallBack * aCallBack )
{
    qcStatement       * sStatement    = NULL;
    qtcNode           * sTmp          = NULL;
    qtcNode           * sListNode     = NULL;
    qmsConcatElement  * sGroup        = NULL;
    qmsConcatElement  * sSubGroup     = NULL;
    idBool              sIsTrue       = ID_FALSE;
    idBool              sIsTrueTmp    = ID_FALSE;
    SInt                sLocation     = -3;
    SInt                i             = 0;
    qsfGroupingInfo   * sGroupingInfo = NULL;

    sTmp = (qtcNode *)aNode->arguments;
    sStatement = ((qcTemplate*)aTemplate)->stmt;

    IDE_TEST_RAISE( ( sTmp->node.lflag & MTC_NODE_OPERATOR_MASK )
                    == MTC_NODE_OPERATOR_LIST,
                    ERR_NOT_APPLICABLE_DATA_TYPE );

    for ( sGroup = aSFWGH->group; sGroup != NULL; sGroup = sGroup->next )
    {
        if ( sGroup->type == QMS_GROUPBY_NORMAL )
        {
            IDE_TEST(qtc::isEquivalentExpression( sStatement,
                                                  sGroup->arithmeticOrList,
                                                  sTmp,
                                                  &sIsTrueTmp)
                     != IDE_SUCCESS);
            if ( sIsTrueTmp == ID_TRUE )
            {
                sIsTrue = ID_TRUE;
                sLocation = -2;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else if ( sGroup->type == QMS_GROUPBY_NULL )
        {
            /* PROJ-2415 Grouping Sets Clause */
            IDE_TEST( qtc::isEquivalentExpression( sStatement,
                                                   sGroup->arithmeticOrList,
                                                   sTmp,
                                                   &sIsTrueTmp )
                      != IDE_SUCCESS );
            
            if ( ( sIsTrueTmp == ID_TRUE ) && ( sIsTrue == ID_FALSE ) )
            {
                sIsTrue = ID_TRUE;
                // Grouping Sets Transform 에 의해 소멸 될 Group은 location이 -1 로 세팅 한다.
                // QMS_GROUPBY_NORMAL Type의 Group이
                // 존재 할 경우 QMS_GROUPBY_NULL보다 우선하기 때문에 break 하지 않는다.                    
                sLocation = -1;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            for ( sSubGroup = sGroup->arguments;
                  sSubGroup != NULL;
                  sSubGroup = sSubGroup->next, i++ )
            {
                if ( ( sSubGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    for ( sListNode = (qtcNode *)sSubGroup->arithmeticOrList->node.arguments;
                          sListNode != NULL;
                          sListNode = (qtcNode *)sListNode->node.next )
                    {
                        IDE_TEST(qtc::isEquivalentExpression( sStatement,
                                                              sListNode,
                                                              sTmp,
                                                              &sIsTrueTmp )
                                 != IDE_SUCCESS);
                        if ( sIsTrueTmp == ID_TRUE )
                        {
                            sIsTrue = ID_TRUE;
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                }
                else
                {
                    IDE_TEST(qtc::isEquivalentExpression( sStatement,
                                                          sSubGroup->arithmeticOrList,
                                                          sTmp,
                                                          &sIsTrueTmp)
                             != IDE_SUCCESS);
                }

                if ( sIsTrueTmp == ID_TRUE )
                {
                    sIsTrue = ID_TRUE;
                    sLocation = i;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }    
    }

    IDE_TEST_RAISE ( sLocation < -2, ERR_NOT_ALLOW_CLAUSE )

    IDU_FIT_POINT( "qsfGrouping::qsfGroupingMakeInfo::alloc::GroupingInfo" );
    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                ID_SIZEOF( qsfGroupingInfo ),
                                (void**) &sGroupingInfo )
              != IDE_SUCCESS );

    sGroupingInfo->address = ( void * )&aSFWGH->groupingInfoAddr;
    sGroupingInfo->location = sLocation;

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;
    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                              (void *)sGroupingInfo;
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOW_CLAUSE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GROUP_EXPRESSION ));
    }
    IDE_EXCEPTION( ERR_NOT_APPLICABLE_DATA_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_TARGET,"" ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC qsfGroupingEstimate( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcCallBack * aCallBack )
{
    qtcCallBackInfo   * sCallBackInfo = NULL;
    qmsSFWGH          * sSFWGH        = NULL;
    UInt                sFence        = 0;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    IDE_TEST_RAISE( sFence != 1, ERR_INVALID_FUNCTION_ARGUMENT );

    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    sSFWGH        = sCallBackInfo->SFWGH;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );
    IDE_TEST_RAISE( sSFWGH        == NULL, ERR_NO_GROUP );
    IDE_TEST_RAISE( sSFWGH->group == NULL, ERR_NO_GROUP );

    switch ( sSFWGH->validatePhase )
    {
    case QMS_VALIDATE_TARGET:
        IDE_TEST( qsfGroupingInitializeColumn( aNode, aTemplate, aStack )
                  != IDE_SUCCESS );
        break;
    case QMS_VALIDATE_GROUPBY:
        IDE_TEST( qsfGroupingMakeInfo( aNode, aTemplate , sSFWGH, aCallBack )
                  != IDE_SUCCESS );
        break;
    case QMS_VALIDATE_HAVING:
        IDE_TEST( qsfGroupingInitializeColumn( aNode, aTemplate, aStack )
                  != IDE_SUCCESS );
        IDE_TEST( qsfGroupingMakeInfo( aNode, aTemplate , sSFWGH, aCallBack )
                  != IDE_SUCCESS );
        break;
    default:
        IDE_RAISE( ERR_NOT_ALLOW_CLAUSE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
       IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_NO_GROUP )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NEED_GROUP_BY ));
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_CLAUSE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GROUP_EXPRESSION ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfGroupingFinalize( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate )
{
    qtcNode         ** sTmp          = NULL;
    qmnGrouping     ** sTmp2         = NULL;
    qmnGrouping      * sGrouping     = NULL;
    qmnRollGrouping  * sRollGrouping = NULL;
    qmnCubeGrouping  * sCubeGrouping = NULL;
    UShort             sMask         = 0;
    UShort             sCubeGroup    = 0;
    UInt               sGroupIndex   = 0;
    SInt               sSubIndex     = 0;

    qsfGroupingInfo  * sGroupingInfo = NULL;

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    IDE_TEST_RAISE( aInfo == NULL, ERR_INTERNAL );

    sGroupingInfo = (qsfGroupingInfo *)aInfo;

    sTmp = (qtcNode **)(sGroupingInfo->address);

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void *)( (UChar *)aTemplate->rows[aNode->table].row +
                                     aStack->column->column.offset );

    if ( *sTmp == NULL )
    {
        if ( sGroupingInfo->location == -2 )
        {
            // QMS_GROUPBY_NORMAL 일 경우
            *( mtdIntegerType *)aStack[0].value = 0;
        }
        else if ( sGroupingInfo->location == -1 )
        {    
            // Grouping Sets Transform에 의해 소멸 된 Group
            *( mtdIntegerType *)aStack[0].value = 1;
        }
        else
        {
            IDE_RAISE( ERR_INTERNAL );
        }
    }
    else
    {
        sTmp2 = (qmnGrouping **)(aTemplate->rows[(*sTmp)->node.table].row);

        if ( *sTmp2 == NULL )
        {
            *(mtdIntegerType *)aStack[0].value = 0;
        }
        else
        {
            sGrouping   = *sTmp2;
            switch( sGrouping->type )
            {
                case QMS_GROUPBY_ROLLUP:
                    sRollGrouping = ( qmnRollGrouping * )sGrouping;

                    if ( sGroupingInfo->location == -1 )
                    {
                        // Grouping Sets Transform에 의해 소멸 된 Group
                        *( mtdIntegerType *)aStack[0].value = 1;
                    }
                    else if ( sGroupingInfo->location == -2 )
                    {
                        // QMS_GROUPBY_NORMAL Type의 Group
                        *( mtdIntegerType *)aStack[0].value = 0;
                    }
                    else
                    {
                        // 일반적인 ROLLUP의 Group Expression을 Grouping()의 인자로 가질 경우
                        if ( ( *sRollGrouping->info.index ) >=
                             ( sRollGrouping->count - sGroupingInfo->location ) )
                        {
                            *( mtdIntegerType *)aStack[0].value = 1;
                        }
                        else
                        {
                            *( mtdIntegerType *)aStack[0].value = 0;
                        }
                    }
                    break;
                case QMS_GROUPBY_CUBE:
                    sCubeGrouping = ( qmnCubeGrouping * )sGrouping;
                    sSubIndex = *( sCubeGrouping->info.index );
                    if ( sSubIndex <= 0 )
                    {
                        *(mtdIntegerType *)aStack[0].value = 1;
                    }
                    else
                    {
                        sGroupIndex = sCubeGrouping->subIndexMap[sSubIndex - 1];
                        sCubeGroup = sCubeGrouping->groups[sGroupIndex] &
                                     ~QMND_CUBE_GROUP_DONE_MASK;
                        sMask = 0x1 << ( sGroupingInfo->location );

                        if ( sGroupingInfo->location == -1 )
                        {
                            // Grouping Sets Transform에 의해 소멸 된 Group
                            *( mtdIntegerType *)aStack[0].value = 1;
                        }
                        else if ( sGroupingInfo->location == -2 )
                        {
                            // QMS_GROUPBY_NORMAL Type의 Group
                            *( mtdIntegerType *)aStack[0].value = 0;
                        }
                        else
                        {
                            if ( ( sCubeGroup & sMask ) == 0 )
                            {
                                *( mtdIntegerType *)aStack[0].value = 1;
                            }
                            else
                            {
                                *( mtdIntegerType *)aStack[0].value = 0;
                            }
                        }
                    }
                    break;
                default:
                    IDE_RAISE( ERR_INTERNAL );
                    break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ) );
    }

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsfGroupingCalculate",
                                  "The pointer is NULL" ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfGroupingInitialize(  mtcNode     * ,
                               mtcStack    * ,
                               SInt          ,
                               void*         ,
                               mtcTemplate *  )
{
    return IDE_SUCCESS;
}

IDE_RC qsfGroupingAggregate(  mtcNode     * ,
                             mtcStack    * ,
                             SInt          ,
                             void*         ,
                             mtcTemplate *  )
{
    return IDE_SUCCESS;
}

IDE_RC qsfGroupingCalculate(  mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          ,
                              void        * ,
                              mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;
}
