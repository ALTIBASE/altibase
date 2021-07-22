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
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>

extern mtfModule mtfInlist;
extern mtfModule mtfNotInlist;
extern mtdModule mtdVarchar;
extern mtdModule mtdChar;
extern mtdModule mtdFloat;
extern mtdModule mtdNumeric;
extern mtdModule mtdList;

static mtcName mtfInlistFunctionName[1] = {
    { NULL, 6, (void*)"INLIST" }
};

static IDE_RC mtfInlistEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfInlist = {
    1|MTC_NODE_OPERATOR_EQUAL|MTC_NODE_COMPARISON_TRUE|
        MTC_NODE_INDEX_ARGUMENT_LEFT|
        MTC_NODE_PRINT_FMT_PREFIX_PA,
    ~0,
    1.0/3.0,  // TODO : default selectivity
    mtfInlistFunctionName,
    &mtfNotInlist,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfInlistEstimate
};

IDE_RC mtfInlistTokenize( const mtdModule * aModule,
                          mtcColumn       * aValueColumn,
                          const void      * aValue,
                          UInt              aCount,
                          UInt              aMaxLength,
                          mtcInlistInfo   * aInlistInfo,
                          mtkRangeInfo    * aInfo );

/* BUG-47690 Inlist 복합 Index fatal */
IDE_RC mtfInlistGetCountAndMaxLength( const void * aValue,
                                      UInt       * aCount,
                                      UInt       * aMaxLength );

IDE_RC mtfInlistEstimateRange( mtcNode*,
                               mtcTemplate*,
                               UInt,
                               UInt*    aSize );


IDE_RC mtfInlistExtractRange( mtcNode*       aNode,
                              mtcTemplate*   aTemplate,
                              mtkRangeInfo * aInfo,
                              smiRange*      aRange );

IDE_RC mtfInlistCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfInlistCalculate,
    NULL,
    mtx::calculateNA,
    mtfInlistEstimateRange,
    mtfInlistExtractRange
};

IDE_RC mtfInlistEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt      /* aRemain */,
                          mtcCallBack* aCallBack )
{
    extern mtdModule mtdBoolean;
    mtcNode* sNode;
    ULong    sLflag;

    const mtdModule* sTarget;
    const mtdModule* sModules[2];

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if ( ( aCallBack->flag & MTC_ESTIMATE_ARGUMENTS_MASK ) ==
         MTC_ESTIMATE_ARGUMENTS_ENABLE )
    {
        for ( sNode  = aNode->arguments, sLflag = MTC_NODE_INDEX_UNUSABLE;
              sNode != NULL;
              sNode  = sNode->next )
        {
            if ( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) ==
                 MTC_NODE_COMPARISON_TRUE )
            {
                sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
            }
            sLflag |= sNode->lflag & MTC_NODE_INDEX_MASK;
        }

        aNode->lflag &= ~(MTC_NODE_INDEX_MASK);
        aNode->lflag |= sLflag;

        /* list type is not supported */
        IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                        ( aStack[2].column->module == &mtdList ),
                        ERR_INVALID_FUNCTION_ARGUMENT );

        if ( aStack[2].column->module == &mtdChar )
        {
            IDE_TEST( mtf::getComparisonModule( &sTarget,
                                                aStack[1].column->module->no,
                                                mtdChar.no )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sTarget == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );

            //fix BUG-17610
            if ( ( aNode->lflag & MTC_NODE_EQUI_VALID_SKIP_MASK ) !=
                 MTC_NODE_EQUI_VALID_SKIP_TRUE )
            {
                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget )
                                != ID_TRUE, ERR_CONVERSION_NOT_APPLICABLE );
            }

            // column conversion 을 붙인다.
            sModules[0] = sTarget;
            sModules[1] = &mtdChar;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( aStack[2].column->module == &mtdVarchar )
            {
                IDE_TEST( mtf::getComparisonModule( &sTarget,
                                                    aStack[1].column->module->no,
                                                    mtdVarchar.no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );

                //fix BUG-17610
                if ( (aNode->lflag & MTC_NODE_EQUI_VALID_SKIP_MASK) !=
                     MTC_NODE_EQUI_VALID_SKIP_TRUE )
                {
                    // To Fix PR-15208
                    IDE_TEST_RAISE( mtf::isEquiValidType( sTarget )
                                    != ID_TRUE, ERR_CONVERSION_NOT_APPLICABLE );
                }

                // column conversion 을 붙인다.
                sModules[0] = sTarget;
                sModules[1] = &mtdVarchar;

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            }
            else
            {
                /*char varchar 가 아닌 경우 강제로 varchar 로 conversion */
                IDE_TEST( mtf::getComparisonModule( &sTarget,
                                                    aStack[1].column->module->no,
                                                    mtdVarchar.no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );

                //fix BUG-17610
                if ( (aNode->lflag & MTC_NODE_EQUI_VALID_SKIP_MASK) !=
                     MTC_NODE_EQUI_VALID_SKIP_TRUE )
                {
                    // To Fix PR-15208
                    IDE_TEST_RAISE( mtf::isEquiValidType( sTarget )
                                    != ID_TRUE, ERR_CONVERSION_NOT_APPLICABLE );
                }

                // column conversion 을 붙인다.
                sModules[0] = sTarget;
                sModules[1] = &mtdVarchar;

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            }
        }
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfInlistEstimateRange( mtcNode*,
                               mtcTemplate*,
                               UInt,
                               UInt*    aSize )
{
    /* (smiRange + RangeCallback*2 )*1000 + ID_SIZEOF(mtcInlistInfo)  */
    /* BUG-47690 Inlist 복합 Index fatal */
    *aSize =  ( ( ( ID_SIZEOF(smiRange) + ID_SIZEOF(mtkRangeCallBack) * 2 ) )
              * MTC_INLIST_KEYRANGE_COUNT_MAX )  + ID_SIZEOF(mtcInlistInfo) ;

    return IDE_SUCCESS;
}


IDE_RC mtfInlistExtractRange( mtcNode*       aNode,
                              mtcTemplate*   aTemplate,
                              mtkRangeInfo * aInfo,
                              smiRange*      aRange )
{
    mtcNode*          sIndexNode;
    mtcNode*          sValueNode;
    mtkRangeCallBack* sMinimumCallBack;
    mtkRangeCallBack* sMaximumCallBack;
    mtcColumn*        sValueColumn;
    mtcColumn*        sIndexColumn;
    void*             sValue;

    smiRange        * sCurRange;
    smiRange        * sPrevRange = NULL;
    mtcInlistInfo   * sInlistInfo;
    UInt              i;
    UInt              sCount = 0;
    UInt              sMaxLength = 0;
    mtcColumn *       sSrcValueColumn;
    const void *      sSrcValue;

    IDE_TEST_RAISE( aInfo->argument >= 2, ERR_INVALID_FUNCTION_ARGUMENT );

    /* index left or right */
    if ( aInfo->argument == 0 )
    {
        sIndexNode = aNode->arguments;
        sValueNode = sIndexNode->next;
    }
    else
    {
        sValueNode = aNode->arguments;
        sIndexNode = sValueNode->next;
    }

    sValueNode = mtf::convertedNode( sValueNode, aTemplate );
    sIndexNode = mtf::convertedNode( sIndexNode, aTemplate );

    sIndexColumn = aTemplate->rows[sIndexNode->table].columns
        + sIndexNode->column;

    /* tokenize source value and column */
    sSrcValueColumn = aTemplate->rows[sValueNode->table].columns
        + sValueNode->column;

    sSrcValue = mtd::valueForModule( (smiColumn*)sSrcValueColumn,
                                     aTemplate->rows[sValueNode->table].row,
                                     MTD_OFFSET_USE,
                                     sSrcValueColumn->module->staticNull );

    /* BUG-47690 Inlist 복합 Index fatal */
    IDE_TEST( mtfInlistGetCountAndMaxLength( sSrcValue,
                                             &sCount,
                                             &sMaxLength )
              != IDE_SUCCESS );

    /* BUG-47690 Inlist 실제 구한 토큰 갯수의 Range 뒤로 Inlistinfo의 address를
     * 지정하고 InlistInfo까지 포함한 메모리 크기를 useOffset에 지정한다
     */
    sInlistInfo = (mtcInlistInfo *) ( (UChar *)aRange +
                  ( (ID_SIZEOF(smiRange) + ID_SIZEOF(mtkRangeCallBack) * 2 ) * sCount ) );

    aInfo->useOffset = ((ID_SIZEOF(smiRange) + ID_SIZEOF(mtkRangeCallBack) * 2 ) * sCount ) + ID_SIZEOF(mtcInlistInfo);

    IDE_TEST( mtfInlistTokenize( sIndexColumn->module,
                                 sSrcValueColumn,
                                 sSrcValue,
                                 sCount,
                                 sMaxLength,
                                 sInlistInfo,
                                 aInfo )
              != IDE_SUCCESS );

    sCurRange = aRange;

    for ( i = 0; i < sInlistInfo->count; i++ )
    {
        sMinimumCallBack = (mtkRangeCallBack*)( sCurRange + 1 );
        sMaximumCallBack = sMinimumCallBack + 1;

        sCurRange->prev           = sPrevRange;
        sCurRange->next           = (smiRange*)( sMaximumCallBack + 1 );
        sCurRange->minimum.data   = sMinimumCallBack;
        sCurRange->maximum.data   = sMaximumCallBack;
        sMinimumCallBack->next = NULL;
        sMaximumCallBack->next = NULL;
        sMinimumCallBack->flag = 0;
        sMaximumCallBack->flag = 0;

        sValueColumn = &sInlistInfo->valueDesc;

        sValue = (void*)mtd::valueForModule( (smiColumn*)sValueColumn,
                                             sInlistInfo->valueArray[i],
                                             MTD_OFFSET_USE,
                                             sValueColumn->module->staticNull );

        if ( sValueColumn->module->isNull( sValueColumn,
                                           sValue ) == ID_TRUE )
        {
            if ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ||
                 aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL )
            {
                // mtd type의 column value에 대한 range callback
                sCurRange->minimum.callback     = mtk::rangeCallBackGT4Mtd;
                sCurRange->maximum.callback     = mtk::rangeCallBackLT4Mtd;
            }
            else
            {
                if ( ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_MTDVAL ) ||
                     ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_STOREDVAL ) )
                {
                    /* MTD_COMPARE_STOREDVAL_MTDVAL
                       stored type의 column value에 대한 range callback */
                    sCurRange->minimum.callback     = mtk::rangeCallBackGT4Stored;
                    sCurRange->maximum.callback     = mtk::rangeCallBackLT4Stored;
                }
                else
                {
                    /* PROJ-2433 */
                    sCurRange->minimum.callback     = mtk::rangeCallBackGT4IndexKey;
                    sCurRange->maximum.callback     = mtk::rangeCallBackLT4IndexKey;
                }
            }

            sMinimumCallBack->compare    = mtk::compareMinimumLimit;
            sMinimumCallBack->columnIdx  = aInfo->columnIdx;
            //sMinimumCallBack->columnDesc = NULL;
            //sMinimumCallBack->valueDesc  = NULL;
            sMinimumCallBack->value      = NULL;

            sMaximumCallBack->compare    = mtk::compareMinimumLimit;
            sMaximumCallBack->columnIdx  = aInfo->columnIdx;
            //sMaximumCallBack->columnDesc = NULL;
            //sMaximumCallBack->valueDesc  = NULL;
            sMaximumCallBack->value      = NULL;
        }
        else
        {
            //---------------------------
            // RangeCallBack 설정
            //---------------------------

            if ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ||
                 aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL )
            {
                // mtd type의 column value에 대한 range callback
                sCurRange->minimum.callback     = mtk::rangeCallBackGE4Mtd;
                sCurRange->maximum.callback     = mtk::rangeCallBackLE4Mtd;
            }
            else
            {
                if ( ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_MTDVAL ) ||
                     ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_STOREDVAL ) )
                {
                    /* MTD_COMPARE_STOREDVAL_MTDVAL
                       stored type의 column value에 대한 range callback */
                    sCurRange->minimum.callback     = mtk::rangeCallBackGE4Stored;
                    sCurRange->maximum.callback     = mtk::rangeCallBackLE4Stored;
                }
                else
                {
                    /* PROJ-2433 */
                    sCurRange->minimum.callback     = mtk::rangeCallBackGE4IndexKey;
                    sCurRange->maximum.callback     = mtk::rangeCallBackLE4IndexKey;
                }
            }

            //----------------------------------------------
            // MinimumCallBack & MaximumCallBack 정보 설정
            //----------------------------------------------

            sMinimumCallBack->columnIdx  = aInfo->columnIdx;
            sMinimumCallBack->columnDesc = *aInfo->column;
            sMinimumCallBack->valueDesc  = *sValueColumn;

            sMinimumCallBack->value      = sValue;

            sMaximumCallBack->columnIdx  = aInfo->columnIdx;
            sMaximumCallBack->columnDesc = *aInfo->column;
            sMaximumCallBack->valueDesc  = *sValueColumn;

            sMaximumCallBack->value      = sValue;

            // PROJ-1364
            if ( aInfo->isSameGroupType == ID_FALSE )
            {
                sMinimumCallBack->flag &= ~MTK_COMPARE_SAMEGROUP_MASK;
                sMinimumCallBack->flag |= MTK_COMPARE_SAMEGROUP_FALSE;

                sMaximumCallBack->flag &= ~MTK_COMPARE_SAMEGROUP_MASK;
                sMaximumCallBack->flag |= MTK_COMPARE_SAMEGROUP_FALSE;

                if ( aInfo->direction == MTD_COMPARE_ASCENDING )
                {
                    sMinimumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                    sMinimumCallBack->flag |= MTK_COMPARE_DIRECTION_ASC;

                    sMaximumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                    sMaximumCallBack->flag |= MTK_COMPARE_DIRECTION_ASC;
                }
                else
                {
                    sMinimumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                    sMinimumCallBack->flag |= MTK_COMPARE_DIRECTION_DESC;

                    sMaximumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                    sMaximumCallBack->flag |= MTK_COMPARE_DIRECTION_DESC;
                }

                sMinimumCallBack->compare    =
                    aInfo->column->module->keyCompare[aInfo->compValueType]
                                                     [aInfo->direction];

                sMaximumCallBack->compare    =
                    aInfo->column->module->keyCompare[aInfo->compValueType]
                                                     [aInfo->direction];
            }
            else
            {
                sMinimumCallBack->flag &= ~MTK_COMPARE_SAMEGROUP_MASK;
                sMinimumCallBack->flag |= MTK_COMPARE_SAMEGROUP_TRUE;

                sMaximumCallBack->flag &= ~MTK_COMPARE_SAMEGROUP_MASK;
                sMaximumCallBack->flag |= MTK_COMPARE_SAMEGROUP_TRUE;

                if ( aInfo->direction == MTD_COMPARE_ASCENDING )
                {
                    sMinimumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                    sMinimumCallBack->flag |= MTK_COMPARE_DIRECTION_ASC;

                    sMaximumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                    sMaximumCallBack->flag |= MTK_COMPARE_DIRECTION_ASC;
                }
                else
                {
                    sMinimumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                    sMinimumCallBack->flag |= MTK_COMPARE_DIRECTION_DESC;

                    sMaximumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                    sMaximumCallBack->flag |= MTK_COMPARE_DIRECTION_DESC;
                }

                sMinimumCallBack->compare = sMaximumCallBack->compare =
                    mtd::findCompareFunc( aInfo->column,
                                          sValueColumn,
                                          aInfo->compValueType,
                                          aInfo->direction );

            }
        }

        sPrevRange = sCurRange;
        sCurRange = sCurRange->next;
    }

    if ( sPrevRange != NULL )
    {
        sPrevRange->next = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfInlistCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
    const mtdModule   * sModule;
    mtdValueInfo        sValueInfo1;
    mtdValueInfo        sValueInfo2;
    mtdBooleanType      sValue;
    mtcColumn           sTokenValueColumn;
    ULong               sTokenValueBuf[MTC_INLIST_ELEMRNT_LENGTH_MAX / 8 + 1];
    void              * sTokenValue;
    const mtdCharType * sSrcValue;
    UInt                sIndex;
    UInt                sTokenLength;
    UInt                sOffset;
    UInt                sTokenMaxLength;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /* list type is not supported */
    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                    ( aStack[2].column->module == &mtdList ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sValue = MTD_BOOLEAN_FALSE;

    sTokenValue = (void *)sTokenValueBuf;

    sSrcValue = (const mtdCharType *)aStack[2].value;

    /* get max token length */
    sIndex          = 0;
    sTokenLength    = 0;
    sTokenMaxLength = 0;

    while ( sIndex < sSrcValue->length )
    {
        if ( sSrcValue->value[sIndex] == ',' )
        {
            sTokenMaxLength = IDL_MAX( sTokenMaxLength, sTokenLength );
            sTokenLength = 0;
        }
        else
        {
            sTokenLength++;
        }

        sIndex++;
    }
    sTokenMaxLength = IDL_MAX( sTokenMaxLength, sTokenLength );
    IDE_TEST_RAISE( sTokenLength > MTC_INLIST_ELEMRNT_LENGTH_MAX, ERR_INVALID_LENGTH );

    /* clolumn init */
    sModule = aStack[1].column->module;
    if ( ( sModule == &mtdFloat ) || ( sModule == &mtdNumeric ) )
    {
        IDE_TEST( mtc::initializeColumn( &sTokenValueColumn,
                                         & mtdFloat,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

    }
    else if ( sModule == &mtdChar )
    {
        IDE_TEST( mtc::initializeColumn( &sTokenValueColumn,
                                         & mtdChar,
                                         1,
                                         sTokenMaxLength,
                                         0 )
                  != IDE_SUCCESS );
    }
    else if ( sModule == &mtdVarchar )
    {
        IDE_TEST( mtc::initializeColumn( &sTokenValueColumn,
                                         & mtdVarchar,
                                         1,
                                         sTokenMaxLength,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_RAISE( ERR_CONVERSION_NOT_APPLICABLE );
    }

    sIndex       = 0;
    sTokenLength = 0;
    sOffset      = 0;
    while ( sIndex < sSrcValue->length )
    {
        if ( sSrcValue->value[sIndex] != ',' )
        {

            sTokenLength++;
        }
        else
        {
            if ( ( sModule == &mtdFloat ) || ( sModule == &mtdNumeric ) )
            {
                IDE_TEST( mtc::makeNumeric(
                              (mtdNumericType*) sTokenValue,
                              MTD_FLOAT_MANTISSA_MAXIMUM,
                              (const UChar*)sSrcValue->value + sOffset,
                              sTokenLength )
                          != IDE_SUCCESS );
            }
            else
            {
                /* space padding */
                idlOS::memcpy( ((mtdCharType *)sTokenValue)->value,
                               (const UChar*)sSrcValue->value + sOffset,
                               sTokenLength );
                if ( sModule == &mtdChar )
                {
                    idlOS::memset( ((mtdCharType *)sTokenValue)->value + sTokenLength ,
                                   ' ',
                                   sTokenMaxLength - sTokenLength );
                    ((mtdCharType *)sTokenValue)->length = sTokenMaxLength; /* space padding */
                }
                else
                {
                    ((mtdCharType *)sTokenValue)->length = sTokenLength;
                }

            }

            /* logical compare toeknized value and aStack[1] */
            if ( ( sModule->isNull( aStack[1].column,
                                    aStack[1].value ) == ID_TRUE ) ||
                ( sModule->isNull( &sTokenValueColumn,
                                   sTokenValue ) == ID_TRUE ) )
            {
                sValue = MTD_BOOLEAN_NULL;
            }
            else
            {
                sValueInfo1.column = aStack[1].column;
                sValueInfo1.value  = aStack[1].value;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = &sTokenValueColumn;
                sValueInfo2.value  = sTokenValue;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                     &sValueInfo2 ) == 0 )
                {
                    sValue = MTD_BOOLEAN_TRUE;
                    break;
                }
                else
                {
                    sValue = MTD_BOOLEAN_FALSE;
                }
            }

            sOffset = sIndex + 1;
            sTokenLength = 0;
        }
        sIndex++;
    } /* while */

    /* 마지막 , 또는 ','가 없는 경우 */
    if ( ( sIndex == sSrcValue->length ) && ( sValue != MTD_BOOLEAN_TRUE ) )
    {
        if ( ( sModule == &mtdFloat ) || ( sModule == &mtdNumeric ) )
        {
            IDE_TEST( mtc::makeNumeric(
                          (mtdNumericType*) sTokenValue,
                          MTD_FLOAT_MANTISSA_MAXIMUM,
                          (const UChar*)sSrcValue->value + sOffset,
                          sTokenLength )
                      != IDE_SUCCESS );
        }
        else
        {
            idlOS::memcpy( ((mtdCharType *)sTokenValue)->value,
                           (const UChar*)sSrcValue->value + sOffset,
                           sTokenLength );
            if ( sModule == &mtdChar)
            {
                idlOS::memset( ((mtdCharType *)sTokenValue)->value + sTokenLength ,
                               ' ',
                               sTokenMaxLength - sTokenLength );
                ((mtdCharType *)sTokenValue)->length = sTokenMaxLength; /* space padding */
            }
            else
            {
                ((mtdCharType *)sTokenValue)->length = sTokenLength;
            }
        }

        /* logical compare toeknized value and aStack[1] */
        if ( ( sModule->isNull( aStack[1].column,
                                aStack[1].value ) == ID_TRUE ) ||
            ( sModule->isNull( &sTokenValueColumn,
                               sTokenValue ) == ID_TRUE ) )
        {
            sValue = MTD_BOOLEAN_NULL;
        }
        else
        {
            sValueInfo1.column = aStack[1].column;
            sValueInfo1.value  = aStack[1].value;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = &sTokenValueColumn;
            sValueInfo2.value  = sTokenValue;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                 &sValueInfo2 ) == 0 )
            {
                sValue = MTD_BOOLEAN_TRUE;
            }
            else
            {
                sValue = MTD_BOOLEAN_FALSE;
            }
        }
     }

    *(mtdBooleanType*)aStack[0].value = sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALIDATE_INVALID_LENGTH ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-47690 Inlist 복합 Index fatal */
IDE_RC mtfInlistGetCountAndMaxLength( const void * aValue,
                                      UInt       * aCount,
                                      UInt       * aMaxLength )
{
    const mtdCharType * sSrcValue;
    UInt                sIndex = 0;
    UInt                sCommaCount = 0;
    UInt                sTokenLength = 0;
    UInt                sTokenMaxLength = 0;

    /* comma 개수를 세고 가장 큰 토큰의 길이를 얻는다.*/
    sSrcValue = (const mtdCharType *)aValue;

    while ( sIndex < sSrcValue->length )
    {
        if ( sSrcValue->value[sIndex] == ',' )
        {
            sCommaCount++;
            sTokenMaxLength = IDL_MAX( sTokenMaxLength, sTokenLength );
            sTokenLength = 0;
        }
        else
        {
            sTokenLength++;
        }

        sIndex++;
    }

    sTokenMaxLength = IDL_MAX( sTokenMaxLength, sTokenLength );

    IDE_TEST_RAISE( sTokenMaxLength > MTC_INLIST_ELEMRNT_LENGTH_MAX, ERR_INVALID_LENGTH );
    *aMaxLength = sTokenMaxLength;

    *aCount = sCommaCount + 1;
    IDE_TEST_RAISE( *aCount > MTC_INLIST_ELEMENT_COUNT_MAX, ERR_INVALID_VALUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALIDATE_INVALID_LENGTH ) );
    }
    IDE_EXCEPTION( ERR_INVALID_VALUE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALIDATE_INVALID_VALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfInlistTokenize( const mtdModule * aModule,
                          mtcColumn       * /* aValueColumn */,
                          const void      * aValue,
                          UInt              aCount,
                          UInt              aMaxLength,
                          mtcInlistInfo   * aInlistInfo,
                          mtkRangeInfo    * aInfo )
{
    const mtdCharType * sSrcValue;
    UInt                sIndex;
    UInt                sOffset;
    void              * sTokenValue;
    UInt                sTokenLength;
    UInt                sTokenIdx;
    UInt                sTokenMaxLength;
    UInt                sValueOffset;
    mtdValueInfo        sValueInfo1;
    mtdValueInfo        sValueInfo2;
    idBool              sIsDuplicate;
    SInt                sCompare;
    UInt                i;
    UInt                j;

    sSrcValue = (const mtdCharType *)aValue;

    aInlistInfo->count = aCount;
    sTokenMaxLength = aMaxLength;

    /* clolumn init */
    if ( ( aModule == &mtdFloat ) || ( aModule == &mtdNumeric ) )
    {
        IDE_TEST( mtc::initializeColumn( &aInlistInfo->valueDesc,
                                         & mtdFloat,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

    }
    else if ( aModule == &mtdChar )
    {
        IDE_TEST( mtc::initializeColumn( &aInlistInfo->valueDesc,
                                         & mtdChar,
                                         1,
                                         sTokenMaxLength,
                                         0 )
                  != IDE_SUCCESS );
        /* space padding */
        idlOS::memset( (void *)aInlistInfo->valueBuf,
                       ' ',
                       ID_SIZEOF(aInlistInfo->valueBuf) );
    }
    else if ( aModule == &mtdVarchar )
    {
        IDE_TEST( mtc::initializeColumn( &aInlistInfo->valueDesc,
                                         & mtdVarchar,
                                         1,
                                         sTokenMaxLength,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_RAISE( ERR_CONVERSION_NOT_APPLICABLE );
    }

    sOffset      = 0;
    sTokenLength = 0;
    sTokenIdx    = 0;
    sIndex       = 0;
    sValueOffset = 0;

    while ( sIndex < sSrcValue->length )
    {
        if ( sSrcValue->value[sIndex] != ',' )
        {
            sTokenLength++;
        }
        else
        {
            if ( ( aModule == &mtdFloat ) || ( aModule == &mtdNumeric ) )
            {
                sTokenValue = (mtdNumericType*)( (UChar *)aInlistInfo->valueBuf + sValueOffset );
                IDE_TEST( mtc::makeNumeric(
                              (mtdNumericType*) sTokenValue,
                              MTD_FLOAT_MANTISSA_MAXIMUM,
                              (const UChar*)sSrcValue->value + sOffset,
                              sTokenLength )
                          != IDE_SUCCESS );
            }
            else
            {
                /* char, varchar */
                sTokenValue = (mtdCharType *)( (UChar *)aInlistInfo->valueBuf + sValueOffset );
                idlOS::memcpy( ((mtdCharType *)sTokenValue)->value,
                               (const UChar*)sSrcValue->value + sOffset,
                               sTokenLength );

                if( aModule == &mtdChar)
                {
                    ((mtdCharType *)sTokenValue)->length = sTokenMaxLength; /* space padding */
                }
                else
                {
                    ((mtdCharType *)sTokenValue)->length = sTokenLength;
                }
            }

            sOffset = sIndex + 1;
            sTokenLength = 0;

            /* BUG-43803 중복제거와 정렬 */
            sIsDuplicate = ID_FALSE;
            for ( i = 0; i < sTokenIdx; i++ )
            {
                sValueInfo1.column = &aInlistInfo->valueDesc;
                sValueInfo1.value  = aInlistInfo->valueArray[i];
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = &aInlistInfo->valueDesc;
                sValueInfo2.value  = sTokenValue;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                sCompare = aModule->logicalCompare[aInfo->direction]( &sValueInfo1,
                                                                      &sValueInfo2 );

                if ( sCompare == 0 )
                {
                    sIsDuplicate = ID_TRUE;
                    break;
                }
                else if ( sCompare > 0 )
                {
                    /* 여기에 삽입 */
                    break;
                }
                else
                {
                    /* nothing to do */
                }
            }

            if ( sIsDuplicate == ID_FALSE )
            {
                /* i번째에 삽입한다 */
                for ( j = sTokenIdx; j > i; j-- )
                {
                    aInlistInfo->valueArray[j] = aInlistInfo->valueArray[j - 1];
                }
                aInlistInfo->valueArray[j] = sTokenValue;
                sTokenIdx++;

                if ( ( aModule == &mtdFloat ) || ( aModule == &mtdNumeric ) )
                {
                    sValueOffset += idlOS::align(
                        ((mtdNumericType*)sTokenValue)->length + mtdFloat.headerSize(),
                        mtdFloat.align );
                }
                else
                {
                    /* char, varchar align은 같다 */
                    sValueOffset += idlOS::align(
                        ((mtdCharType*)sTokenValue)->length + mtdChar.headerSize(),
                        mtdChar.align );
                }
            }
            else
            {
                /* nothing to do */
            }
        }

        sIndex++;
    }

    /* 마지막 , 또는 ','가 없는 경우 */
    if ( ( aModule == &mtdFloat ) || ( aModule == &mtdNumeric ) )
    {
        sTokenValue = (mtdNumericType*)( (UChar *)aInlistInfo->valueBuf + sValueOffset );
        IDE_TEST( mtc::makeNumeric(
                      (mtdNumericType*) sTokenValue,
                      MTD_FLOAT_MANTISSA_MAXIMUM,
                      (const UChar*)sSrcValue->value + sOffset,
                      sTokenLength )
                  != IDE_SUCCESS );
    }
    else
    {
        /* char, varchar */
        sTokenValue = (mtdCharType *)( (UChar *)aInlistInfo->valueBuf + sValueOffset );
        idlOS::memcpy( ((mtdCharType *)sTokenValue)->value,
                       (const UChar*)sSrcValue->value + sOffset,
                       sTokenLength);

        if ( aModule == &mtdChar )
        {
            ((mtdCharType *)sTokenValue)->length = sTokenMaxLength; /* space padding */
        }
        else
        {
            ((mtdCharType *)sTokenValue)->length = sTokenLength;
        }
    }

    sIsDuplicate = ID_FALSE;
    for ( i = 0; i < sTokenIdx; i++ )
    {
        sValueInfo1.column = &aInlistInfo->valueDesc;
        sValueInfo1.value  = aInlistInfo->valueArray[i];
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = &aInlistInfo->valueDesc;
        sValueInfo2.value  = sTokenValue;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sCompare = aModule->logicalCompare[aInfo->direction]( &sValueInfo1,
                                                              &sValueInfo2 );
        if ( sCompare == 0 )
        {
            sIsDuplicate = ID_TRUE;
            break;
        }
        else if ( sCompare > 0 )
        {
            /* 여기에 삽입 */
            break;
        }
        else
        {
            /* nothing to do */
        }
    }

    if ( sIsDuplicate == ID_FALSE )
    {
        /* i번째에 삽입한다 */
        for ( j = sTokenIdx; j > i; j-- )
        {
            aInlistInfo->valueArray[j] = aInlistInfo->valueArray[j - 1];
        }
        aInlistInfo->valueArray[j] = sTokenValue;

        aInlistInfo->count = sTokenIdx + 1;
    }
    else
    {
        aInlistInfo->count = sTokenIdx;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
