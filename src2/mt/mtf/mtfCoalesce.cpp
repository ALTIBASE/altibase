/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtfCoalesce.cpp 86354 2019-11-13 23:41:36Z donovan.seo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfCoalesce;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdDate;
extern mtdModule mtdFloat;
extern mtdModule mtdInterval;
extern mtdModule mtdDouble;
extern mtdModule mtdVarchar;
extern mtdModule mtdNchar;
extern mtdModule mtdNvarchar;
extern mtdModule mtdBigint;

static mtcName mtfCoalesceName[1] = {
    { NULL,  8, ( void * )"COALESCE" }
};

static IDE_RC mtfCoalesceEstimate( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcCallBack * aCallBack );

mtfModule mtfCoalesce = {
    1 | MTC_NODE_OPERATOR_FUNCTION | MTC_NODE_EAT_NULL_TRUE,
    ~( MTC_NODE_INDEX_MASK ),
    1.0, /* default selectivity ( 비교 연산자가 아님 ) */
    mtfCoalesceName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfCoalesceEstimate
};

IDE_RC mtfCoalesceCalculate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCoalesceCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfCoalesceEstimate( mtcNode     * aNode,
                            mtcTemplate * aTemplate,
                            mtcStack    * aStack,
                            SInt          /* aRemain */,
                            mtcCallBack * aCallBack )
{
    UInt              sCount;
    UInt              i;
    UInt              sGroups[MTD_GROUP_MAXIMUM];
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    idBool            sDouble   = ID_TRUE;
    idBool            sNvarchar = ID_FALSE;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_ARGUMENT_NOT_APPLICABLE );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sCount = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    for ( i = 0; i < sCount; i++ )
    {
        sModules[i] = &mtdBoolean;
    }
    for ( i = 0; i < MTD_GROUP_MAXIMUM; i++ )
    {
        sGroups[i] = 0;
    }
    for ( i = 1; i <= sCount; i++ )
    {
        sGroups[(aStack[i].column->module->flag & MTD_GROUP_MASK)] = 1;
    }

    if( sGroups[MTD_GROUP_NUMBER] != 0 )
    {
        sModules[1] = &mtdFloat;

        for ( i = 1; i <= sCount; i++ )
        {
            if( ( sDouble == ID_TRUE ) &&
                ( aStack[i].column->module->id == mtdDouble.id ) )
            {
                /* Nothing to do */
            }
            else
            {
                sDouble = ID_FALSE;
                break;
            }
        }

        if ( sDouble == ID_TRUE )
        {
            sModules[1] = &mtdDouble;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else if ( sGroups[MTD_GROUP_DATE] != 0)
    {
        sModules[1] = &mtdDate;
    }
    else if ( sGroups[MTD_GROUP_INTERVAL] != 0 )
    {
        sModules[1] = &mtdInterval;
    }
    else if ( sGroups[MTD_GROUP_TEXT] != 0 )
    {
        for ( i = 1; i <= sCount; i++ )
        {
            if ( ( aStack[i].column->module->id == mtdNvarchar.id ) ||
                 ( aStack[i].column->module->id == mtdNchar.id ) )
            {
                sNvarchar = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        if ( sNvarchar == ID_TRUE )
        {
            sModules[1] = &mtdNvarchar;
        }
        else
        {
            sModules[1] = &mtdVarchar;
        }
    }
    else
    {
        sModules[1] = aStack[1].column->module;
        IDE_TEST_RAISE( sModules[1] == &mtdList, ERR_CONVERSION_NOT_APPLICABLE );
    }

    for ( i = 2; i <= sCount; i++ )
    {
        sModules[i] = sModules[1];
    }

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        &sModules[1] )
              != IDE_SUCCESS );

    if ( ( aStack[1].column->module->flag & MTD_NON_LENGTH_MASK )
           == MTD_NON_LENGTH_TYPE )
    {
        for ( i = 2; i <= sCount; i++ )
        {
            IDE_TEST_RAISE( !mtc::isSameType( aStack[1].column,
                                              aStack[i].column ),
                            ERR_INVALID_PRECISION );
        }
    }
    else
    {
        /* Nothing to do */
    }

    mtc::initializeColumn( aStack[0].column, aStack[1].column );

    for ( i = 2; i <= sCount; i++ )
    {
        if ( aStack[0].column->column.size < aStack[i].column->column.size )
        {
            mtc::initializeColumn( aStack[0].column, aStack[i].column );
        }
        else
        {
            /* Nothing to do */
        }
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE  )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_INVALID_PRECISION )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCoalesceCalculate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * /* aInfo */,
                             mtcTemplate * aTemplate )
{
    mtcNode   * sNode;
    mtcExecute* sExecute;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );


    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*)aTemplate->rows[aNode->table].row
                                + aStack->column->column.offset );

    for( sNode = aNode->arguments; sNode != NULL; sNode = sNode->next )
    {
        sExecute = MTC_TUPLE_EXECUTE(&aTemplate->rows[sNode->table], sNode);

        IDE_TEST( sExecute->calculate( sNode,
                                       aStack + 1,
                                       aRemain - 1,
                                       sExecute->calculateInfo,
                                       aTemplate )
                  != IDE_SUCCESS );
        if( sNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sNode,
                                             aStack + 1,
                                             aRemain - 1,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        if ( aStack[1].column->module->isNull( aStack[1].column,
                                               aStack[1].value )
              == ID_FALSE )
        {
            idlOS::memcpy( aStack[0].value,
                           aStack[1].value,
                           aStack[1].column->module->actualSize( aStack[1].column,
                                                                 aStack[1].value ) );
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sNode == NULL )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
