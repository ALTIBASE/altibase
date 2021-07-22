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
 * $Id: mtfRankWithinGroup.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfRankWithinGroup;

extern mtdModule mtdBigint;

static mtcName mtfRankWGFunctionName[ 1 ] = {
    { NULL, 17, (void*)"RANK_WITHIN_GROUP" }
};

static IDE_RC mtfRankWGEstimate( mtcNode     *  aNode,
                                 mtcTemplate *  aTemplate,
                                 mtcStack    *  aStack,
                                 SInt           aRemain,
                                 mtcCallBack *  aCallBack );

mtfModule mtfRankWithinGroup = {
    1 |
    MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfRankWGFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRankWGEstimate
};

IDE_RC mtfRankWGInitialize( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

IDE_RC mtfRankWGAggregate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

IDE_RC mtfRankWGMerge( mtcNode     * aNode,
                       mtcStack    * aStack,
                       SInt          aRemain,
                       void        * aInfo,
                       mtcTemplate * aTemplate );

IDE_RC mtfRankWGFinalize( mtcNode      * aNode,
                          mtcStack     * aStack,
                          SInt           aRemain,
                          void         * aInfo,
                          mtcTemplate  * aTemplate );

IDE_RC mtfRankWGCalculate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfRankWGInitialize,
    mtfRankWGAggregate,
    mtfRankWGMerge,
    mtfRankWGFinalize,
    mtfRankWGCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRankWGEstimate( mtcNode     * aNode,
                          mtcTemplate * aTemplate,
                          mtcStack    * aStack,
                          SInt,
                          mtcCallBack * aCallBack )
{
    const mtdModule *   sModules[ MTC_NODE_ARGUMENT_COUNT_MAXIMUM ];
    const mtdModule *   sRepModule;
    mtcNode         *   sNode = NULL;
    UInt                sCountTotal;
    UInt                sCountWG;
    UInt                sIdx;
    UInt                sIdxWG;
    UInt                sIdxRank;

    // within group argument
    IDE_TEST_RAISE( aNode->funcArguments == NULL,
                    ERR_WITHIN_GORUP_MISSING_WITHIN_GROUP );

    // 총 인자 수
    sCountTotal = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    IDE_TEST_RAISE( (sCountTotal < 2) || ( (sCountTotal % 2) != 0 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // within group() 인자 수 계산
    sCountWG = 0;
    for ( sNode = aNode->funcArguments;
          sNode != NULL;
          sNode = sNode->next )
    {
        sCountWG++;
    }

    // rank() 의 인자 수 == within group() 의 인자 수.
    IDE_TEST_RAISE( (sCountWG * 2) != sCountTotal,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sIdxRank = 1;
    sIdxWG   = sCountWG + 1;
    for ( sIdx = 0;
          sIdx < sCountWG;
          sIdx++, sIdxWG++, sIdxRank++ )
    {
        // Rank(..) 와 Within Group(..) 의 각각 대응되는 인자의 대표타입을 구하여 설정한다.
        if ( aStack[ sIdxRank ].column->module->id !=
             aStack[ sIdxWG   ].column->module->id )
        {
            // 다른 모듈이면 대표 타입 모듈을 구한다.
            IDE_TEST( mtf::getComparisonModule(
                           &sRepModule,
                           aStack[ sIdxRank ].column->module->no,
                           aStack[ sIdxWG   ].column->module->no )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sRepModule == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );
        }
        else
        {
            sRepModule = aStack[ sIdxRank ].column->module;
        }

        // 대소 비교 가능 타입인지 확인한다.
        IDE_TEST_RAISE( mtf::isGreaterLessValidType( sRepModule )
                        != ID_TRUE,
                        ERR_CONVERSION_NOT_APPLICABLE );

        sModules[ sIdxWG   - 1 ] = sRepModule;
        sModules[ sIdxRank - 1 ] = sRepModule;
    }

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[ 0 ].column =
            aTemplate->rows[ aNode->table ].columns + aNode->column;

    aTemplate->rows[ aNode->table ].execute[ aNode->column ]
            = mtfExecute;

    // result
    IDE_TEST( mtc::initializeColumn( aStack[ 0 ].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_WITHIN_GORUP_MISSING_WITHIN_GROUP )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_MISSING_WITHIN_GROUP ));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRankWGInitialize( mtcNode     * aNode,
                            mtcStack    *,
                            SInt,
                            void        *,
                            mtcTemplate * aTemplate )
{
    mtcColumn * sColumn = NULL;
    UChar     * sRow    = NULL;

    sColumn = aTemplate->rows[ aNode->table ].columns + aNode->column;
    sRow    = ( UChar * )aTemplate->rows[ aNode->table ].row;
    
    *( mtdBigintType * )( sRow + sColumn[ 0 ].column.offset ) = 1;
    
    return IDE_SUCCESS;
}

IDE_RC mtfRankWGAggregate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        *,
                           mtcTemplate * aTemplate )
{
    const mtdModule *   sModule;
    mtcNode         *   sNodeWG = NULL;
    mtcColumn       *   sColumn = NULL;
    UChar           *   sRow = NULL;
    mtdBigintType   *   sResult = NULL;
    UInt                sIdxRank;
    UInt                sIdxWG;
    UInt                sCount;
    mtdValueInfo        sValInfo1;
    mtdValueInfo        sValInfo2;
    SInt                sCompare;
    ULong               sOrder;
    ULong               sNullsOpt;
    idBool              sVal1IsNull;
    idBool              sVal2IsNull;

    sColumn  = aTemplate->rows[ aNode->table ].columns + aNode->column;
    sRow     = (UChar *)aTemplate->rows[ aNode->table ].row;
    sResult  = (mtdBigintType *)( sRow + sColumn[ 0 ].column.offset );

    sCount = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );


    sNodeWG  = aNode->funcArguments;

    for ( sIdxWG = (sCount / 2) + 1, sIdxRank = 1;
          sIdxWG <= sCount;
          sIdxWG++, sIdxRank++, sNodeWG = sNodeWG->next )
    {
        sValInfo1.column = aStack[ sIdxRank ].column;
        sValInfo1.value  = aStack[ sIdxRank ].value;
        sValInfo1.flag   = MTD_OFFSET_USELESS;

        sValInfo2.column = aStack[ sIdxWG ].column;
        sValInfo2.value  = aStack[ sIdxWG ].value;
        sValInfo2.flag   = MTD_OFFSET_USELESS;

        sModule = aStack[ sIdxWG ].column->module;

        sVal1IsNull = sModule->isNull( sValInfo1.column,
                                       sValInfo1.value );

        sVal2IsNull = sModule->isNull( sValInfo2.column,
                                       sValInfo2.value );

        sOrder = ( ( sNodeWG->lflag & MTC_NODE_WITHIN_GROUP_ORDER_MASK ) ==
                   MTC_NODE_WITHIN_GROUP_ORDER_DESC )
                 ? MTD_COMPARE_DESCENDING
                 : MTD_COMPARE_ASCENDING;

        sNullsOpt = ( sNodeWG->lflag & MTC_NODE_WITHIN_GROUP_NULLS_MASK );

        if ( ( sVal1IsNull == ID_TRUE ) && ( sVal2IsNull == ID_TRUE ) )
        {
            // continue;
        }
        else if ( ( sVal1IsNull == ID_TRUE ) && ( sVal2IsNull == ID_FALSE ) )
        {
            if ( sNullsOpt == MTC_NODE_WITHIN_GROUP_NULLS_FIRST )
            {
                // Nothing to do
            }
            else
            {
                IDE_TEST_RAISE( *sResult == MTD_BIGINT_MAXIMUM,
                                ERR_VALUE_OVERFLOW );
                *sResult += 1;
            }
            break;
        }
        else if ( ( sVal1IsNull == ID_FALSE ) && ( sVal2IsNull == ID_TRUE ) )
        {
            if ( sNullsOpt == MTC_NODE_WITHIN_GROUP_NULLS_FIRST )
            {
                IDE_TEST_RAISE( *sResult == MTD_BIGINT_MAXIMUM,
                                ERR_VALUE_OVERFLOW );
                *sResult += 1;
            }
            else
            {
                // Nothing to do
            }
            break;
        }
        else // ( sValue1IsNull == ID_FALSE && sValue2IsNull == ID_FALSE )
        {
            sCompare = sModule->
                       logicalCompare[ sOrder ]( &sValInfo1,
                                                 &sValInfo2 );
            if ( sCompare > 0 )
            {
                // asc  : sValInfo1 > sValInfo2
                // desc : sValInfo1 < sValInfo2

                IDE_TEST_RAISE( *sResult == MTD_BIGINT_MAXIMUM,
                                ERR_VALUE_OVERFLOW );
                *sResult += 1;
                break;
            }
            else if ( sCompare < 0 )
            {
                // asc  : sValInfo1 < sValInfo2
                // desc : sValInfo1 > sValInfo2
                break;
            }
            else // ( sCompare == 0 )
            {
                // Nothing to to
            }
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRankWGMerge( mtcNode     * aNode,
                       mtcStack    *,
                       SInt,
                       void        * aInfo,
                       mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    UChar           * sDstRow  = NULL;
    UChar           * sSrcRow  = NULL;
    mtdBigintType   * sDstRank = NULL;
    mtdBigintType   * sSrcRank = NULL;


    sDstRow = ( UChar * )aTemplate->rows[ aNode->table ].row;
    sSrcRow = ( UChar * )aInfo;
    sColumn = aTemplate->rows[ aNode->table ].columns + aNode->column;

    sDstRank = ( mtdBigintType * )( sDstRow + sColumn[ 0 ].column.offset );
    sSrcRank = ( mtdBigintType * )( sSrcRow + sColumn[ 0 ].column.offset );

    IDE_TEST_RAISE( ( MTD_BIGINT_MAXIMUM - *sDstRank ) < ( *sSrcRank - 1 ),
                    ERR_VALUE_OVERFLOW );

    *sDstRank += ( *sSrcRank - 1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRankWGFinalize( mtcNode     *,
                          mtcStack    *,
                          SInt,
                          void        *,
                          mtcTemplate * )
{
    return IDE_SUCCESS;
}

IDE_RC mtfRankWGCalculate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt,
                           void        *,
                           mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[ aNode->table ].columns + aNode->column;
    aStack->value  = ( void * )( ( UChar * )aTemplate->rows[ aNode->table ].row
                                            + aStack->column->column.offset );
    
    return IDE_SUCCESS;
}
