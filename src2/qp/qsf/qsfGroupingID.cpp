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
 * $Id: qsfGroupingID.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * GROUPING_ID ( Expression,... )
 *  Group By 에 해당하는  Expression
 *  GROUP By ROLLUP, CUBE 구문이 항상 나와야한다.
 *  sSFWGH->groupingDataAddr 의 Pseudo Column에 Rollup이나 Cube의
 *  포인터가 있다. 이를 통해서 참조 데이터를 얻어서 계산한다.
 ***********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <mtc.h>
#include <qmnRollup.h>
#include <qmnCube.h>

extern mtdModule mtdBigint;

#define GROUPING_ID_ARG_COUNT   60

static mtcName qsfGroupingIDFunctionName[1] = {
        { NULL, 11, (void *)"GROUPING_ID" }
};

static IDE_RC qsfGroupingIDEstimate( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcCallBack * aCallback );

IDE_RC qsfGroupingIDCalculate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

mtfModule qsfGroupingIDModule = {
    1 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTON_GROUPING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,
    qsfGroupingIDFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfGroupingIDEstimate
};

typedef struct qsfGroupingIDInfo
{
    void * address;
    SInt   location[GROUPING_ID_ARG_COUNT];
} qsfGroupingIDInfo;

IDE_RC qsfGroupingIDInitialize(  mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC qsfGroupingIDFinalize(  mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate );

IDE_RC qsfGroupingIDAggregate(  mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

const mtcExecute qsfExecute = {
    qsfGroupingIDInitialize,
    qsfGroupingIDAggregate,
    mtf::calculateNA,
    qsfGroupingIDFinalize,
    qsfGroupingIDCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC qsfGroupingIDInitializeColumn( mtcNode     * aNode,
                                             mtcTemplate * aTemplate,
                                             mtcStack    * aStack )
{
    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;
    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                              NULL;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC qsfGroupingIDMakeInfo( mtcNode     * aNode,
                                     mtcTemplate * aTemplate,
                                     qmsSFWGH    * aSFWGH,
                                     mtcCallBack * aCallBack )
{
    qcStatement       * sStatement    = NULL;
    mtcNode           * sTmp          = NULL;
    qtcNode           * sListNode     = NULL;
    qmsConcatElement  * sGroup        = NULL;
    qmsConcatElement  * sSubGroup     = NULL;
    idBool              sIsTrue;
    idBool              sIsTrueTmp;
    SInt                sLocation     = -3;
    SInt                i             = 0;
    SInt                sIndex        = 0;
    qsfGroupingIDInfo   * sGroupingIDInfo = NULL;

    sStatement = ((qcTemplate*)aTemplate)->stmt;

    IDU_FIT_POINT( "qsfGroupingID::qsfGroupingIDMakeInfo::alloc::GroupingIDInfo" );
    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                ID_SIZEOF( qsfGroupingIDInfo ),
                                (void**) &sGroupingIDInfo )
              != IDE_SUCCESS );

    sGroupingIDInfo->address = ( void * )&aSFWGH->groupingInfoAddr;

    for ( sTmp = aNode->arguments;
          sTmp != NULL;
          sTmp = sTmp->next, sIndex++ )
    {
        IDE_TEST_RAISE( ( sTmp->lflag & MTC_NODE_OPERATOR_MASK )
                        == MTC_NODE_OPERATOR_LIST,
                        ERR_NOT_APPLICABLE_DATA_TYPE );

        sIsTrue    = ID_FALSE;
        sIsTrueTmp = ID_FALSE;
        // GROUPING_ID()는 다수의 인자가 올 수 있음
        // PROJ-2415 Grouping Sets Clause를 진행 하면서 발견 된 버그 처리
        sLocation = -3;

        for ( sGroup = aSFWGH->group; sGroup != NULL; sGroup = sGroup->next )
        {
            if ( sGroup->type == QMS_GROUPBY_NORMAL )
            {
                IDE_TEST(qtc::isEquivalentExpression( sStatement,
                                                      sGroup->arithmeticOrList,
                                                      ( qtcNode * )sTmp,
                                                      &sIsTrueTmp )
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
                                                       ( qtcNode * )sTmp,
                                                       & sIsTrueTmp )
                          != IDE_SUCCESS );
                if ( ( sIsTrueTmp == ID_TRUE ) && ( sIsTrue == ID_FALSE ) )
                {
                    sIsTrueTmp = ID_TRUE;
                    
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
                for ( sSubGroup = sGroup->arguments, i = 0;
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
                                                                  (qtcNode *)sTmp,
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
                                                              (qtcNode *)sTmp,
                                                              &sIsTrueTmp )
                                 != IDE_SUCCESS);
                    }
                    if ( sIsTrueTmp == ID_TRUE )
                    {
                        sIsTrue = ID_TRUE;
                        sLocation  = i;
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

        sGroupingIDInfo->location[sIndex] = sLocation;
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;
    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                              (void *)sGroupingIDInfo;
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOW_CLAUSE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GROUP_EXPRESSION ) );
    }
    IDE_EXCEPTION( ERR_NOT_APPLICABLE_DATA_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_TARGET,"" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC qsfGroupingIDEstimate( mtcNode     * aNode,
                                     mtcTemplate * aTemplate,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     mtcCallBack * aCallBack )
{
    qtcCallBackInfo   * sCallBackInfo = NULL;
    qmsSFWGH          * sSFWGH        = NULL;
    UInt                sFence        = 0;


    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    IDE_TEST_RAISE( ( sFence > GROUPING_ID_ARG_COUNT ) || ( sFence < 1 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    sSFWGH        = sCallBackInfo->SFWGH;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );
    IDE_TEST_RAISE( sSFWGH        == NULL, ERR_NO_GROUP );
    IDE_TEST_RAISE( sSFWGH->group == NULL, ERR_NO_GROUP );

    switch ( sSFWGH->validatePhase )
    {
    case QMS_VALIDATE_TARGET:
        IDE_TEST( qsfGroupingIDInitializeColumn( aNode, aTemplate, aStack )
                  != IDE_SUCCESS );
        break;
    case QMS_VALIDATE_GROUPBY:
        IDE_TEST( qsfGroupingIDMakeInfo( aNode, aTemplate , sSFWGH, aCallBack )
                  != IDE_SUCCESS );
        break;
    case QMS_VALIDATE_HAVING:
        IDE_TEST( qsfGroupingIDInitializeColumn( aNode, aTemplate, aStack )
                  != IDE_SUCCESS );
        IDE_TEST( qsfGroupingIDMakeInfo( aNode, aTemplate , sSFWGH, aCallBack )
                  != IDE_SUCCESS );
        break;
    default:
        IDE_RAISE( ERR_NOT_ALLOW_CLAUSE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
       IDE_SET( ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW ) );
    }
    IDE_EXCEPTION( ERR_NO_GROUP )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NEED_GROUP_BY ) );
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_CLAUSE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GROUP_EXPRESSION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfGroupingIDFinalize( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate )
{
    qtcNode           ** sTmp            = NULL;
    qmnGrouping       ** sTmp2           = NULL;
    qmnGrouping        * sGrouping       = NULL;
    qmnRollGrouping    * sRollGrouping   = NULL;
    qmnCubeGrouping    * sCubeGrouping   = NULL;
    qsfGroupingIDInfo  * sGroupingIDInfo = NULL;
    SInt                 sCount          = 0;
    SLong                sValue          = 0;
    SLong                sValueBit       = 0;
    SInt                 sIndex          = 0;
    UShort               sCubeGroup      = 0;
    UShort               sMask           = 0;
    UInt                 sGroupIndex     = 0;
    SInt                 sSubIndex       = 0;

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    IDE_TEST_RAISE( aInfo == NULL, ERR_INTERNAL );

    sGroupingIDInfo = (qsfGroupingIDInfo *)aInfo;

    sTmp = (qtcNode **)(sGroupingIDInfo->address);

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void *)( (UChar *)aTemplate->rows[aNode->table].row +
                                     aStack->column->column.offset );

    if ( *sTmp == NULL )
    {
        sCount = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

        for ( sIndex = sCount - 1, sValueBit = 0x1;
              sIndex >= 0;
              sIndex--, sValueBit <<= 1 )
        {
            if ( sGroupingIDInfo->location[ sIndex ] == -2 )
            {
                // QMS_GROUPBY_NORMAL 일 경우
                // Nothing to do.                
            }
            else if ( sGroupingIDInfo->location[ sIndex ] == -1 )
            {
                // Grouping Sets Transform 에 의해 소멸 된 Group은 location이
                sValue |= sValueBit;
            }
            else
            {
                IDE_RAISE( ERR_INTERNAL );
            }
        }
    }
    else
    {
        sTmp2 = (qmnGrouping **)(aTemplate->rows[(*sTmp)->node.table].row);

        if ( *sTmp2 == NULL )
        {
            /* Nothing to do */
        }
        else
        {
            sGrouping   = *sTmp2;
            sCount = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

            switch( sGrouping->type )
            {
                case QMS_GROUPBY_ROLLUP:
                    sRollGrouping = ( qmnRollGrouping * )sGrouping;
                    for ( sIndex = sCount - 1, sValueBit = 0x1;
                          sIndex >= 0 ;
                          sIndex--, sValueBit <<= 1 )
                    {
                        if ( sGroupingIDInfo->location[ sIndex ] == -2 )
                        {
                            // QMS_GROUPBY_NORMAL Type의 Group
                            // Nothing to do.
                        }
                        else if ( sGroupingIDInfo->location[ sIndex ] == -1 )
                        {
                            // Grouping Sets Transform에 의해 소멸 된 Group
                            sValue |= sValueBit;
                        }
                        else
                        {
                            if ( ( *sRollGrouping->info.index ) >=
                                 ( sRollGrouping->count - sGroupingIDInfo->location[ sIndex ] ) )
                            {
                                sValue |= sValueBit;
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        }
                    }
                    break;
                case QMS_GROUPBY_CUBE:
                    sCubeGrouping = ( qmnCubeGrouping * )sGrouping;
                    sSubIndex = *( sCubeGrouping->info.index );
                    for ( sIndex = sCount - 1, sValueBit = 0x1;
                          sIndex >= 0;
                          sIndex--, sValueBit <<= 1 )
                    {
                        if ( sGroupingIDInfo->location[ sIndex ] == -2 )
                        {
                            // QMS_GROUPBY_NORMAL Type의 Group
                            // Nothing to do.
                        }
                        else if ( sGroupingIDInfo->location[sIndex] == -1 )
                        {
                            // Grouping Sets Transform에 의해 소멸 된 Group
                            sValue |= sValueBit;
                        }
                        else
                        {
                            if ( sSubIndex <= 0 )
                            {
                                sValue |= sValueBit;
                            }
                            else
                            {
                                sGroupIndex = sCubeGrouping->subIndexMap[ sSubIndex - 1 ];
                                sCubeGroup  = sCubeGrouping->groups[sGroupIndex] &
                                    ~QMND_CUBE_GROUP_DONE_MASK;
                                sMask = 0x1 << sGroupingIDInfo->location[sIndex];
                                if ( ( sCubeGroup & sMask ) == 0 )
                                {
                                    sValue |= sValueBit;
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
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

    *(mtdBigintType *)aStack[0].value  = sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsfGroupingIDCalculate",
                                  "The pointer is NULL" ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfGroupingIDInitialize(  mtcNode     * ,
                                 mtcStack    * ,
                                 SInt          ,
                                 void*         ,
                                 mtcTemplate *  )
{
    return IDE_SUCCESS;
}

IDE_RC qsfGroupingIDAggregate(  mtcNode     * ,
                               mtcStack    * ,
                               SInt          ,
                               void*         ,
                               mtcTemplate *  )
{
    return IDE_SUCCESS;
}

IDE_RC qsfGroupingIDCalculate(  mtcNode     * aNode,
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
