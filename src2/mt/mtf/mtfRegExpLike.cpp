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
 * $Id: mtfRegExpLike.cpp 47933 2011-06-30 02:01:37Z et16 $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtfRegExp.h>

extern mtfModule mtfRegExpLike;
extern mtfModule mtfNotRegExpLike;

extern mtdModule mtdChar;
extern mtdModule mtdBoolean;
extern mtdModule mtdVarchar;
extern mtdModule mtdBinary;

static mtcName mtfRegExpLikeFunctionName[1] = {
    { NULL, 11, (void*)"REGEXP_LIKE" }
};

static IDE_RC mtfRegExpLikeEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );   

mtfModule mtfRegExpLike = {
    2|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_COMPARISON_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0/3.0,  // TODO : default selectivity 
    mtfRegExpLikeFunctionName,
    &mtfNotRegExpLike,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRegExpLikeEstimate
};

IDE_RC mtfRegExpLikeCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

IDE_RC mtfRegExpLikeCalculateFast( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRegExpLikeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,  
    mtk::extractRangeNA  
};

IDE_RC mtfCompileExpression( mtcNode            * aPatternNode,
                             mtcTemplate        * aTemplate,
                             mtfRegExpression  ** aCompiledExpression,
                             mtcCallBack        * aCallBack );


IDE_RC mtfRegExpLikeEstimate( mtcNode     * aNode,
                              mtcTemplate * aTemplate,
                              mtcStack    * aStack,
                              SInt         /*aRemain*/,
                              mtcCallBack * aCallBack )
{
    mtfRegExpression  * sCompiledExpression;
    const mtdModule   * sModules[2];
    mtcNode           * sPatternNode;
    UInt                sPrecision;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( (aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK) != 2 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sModules[0] = &mtdVarchar;
    sModules[1] = sModules[0];
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    /* regexp_like의 결과를 저장함 */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    /* regexp_like의 compiled pattern을 저장함 */
    sPrecision = MTF_REG_EXPRESSION_SIZE( aStack[2].column->precision );
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );
    
    if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK )
         == MTC_NODE_REESTIMATE_FALSE )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }
    else
    {
        sPatternNode = mtf::convertedNode( aNode->arguments->next,
                                           aTemplate );

        if( ( sPatternNode == aNode->arguments->next )
            &&
            ( ( aTemplate->rows[sPatternNode->table].lflag & MTC_TUPLE_TYPE_MASK )
              == MTC_TUPLE_TYPE_CONSTANT ) )
        {
            IDE_TEST ( mtfCompileExpression( sPatternNode,
                                             aTemplate,
                                             &sCompiledExpression,
                                             aCallBack )
                       != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                mtfRegExpLikeCalculateFast;
            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                sCompiledExpression;

            // 더이상 사용하지 않음
            IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                             & mtdBinary,
                                             1,
                                             0,
                                             0 )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next ) == ID_TRUE )
         &&
         ( ( ( aTemplate->rows[aNode->arguments->next->table].lflag
               & MTC_TUPLE_TYPE_MASK )
             == MTC_TUPLE_TYPE_CONSTANT ) ||
           ( ( aTemplate->rows[aNode->arguments->next->table].lflag
               & MTC_TUPLE_TYPE_MASK )
             == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            
        // BUG-38070 undef type으로 re-estimate하지 않는다.
        if ( ( aTemplate->variableRow != ID_USHORT_MAX ) &&
             ( ( aNode->lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) )
        {
            if ( aTemplate->rows[aTemplate->variableRow].
                 columns->module->id == MTD_UNDEF_ID )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
    }

    /* BUG-44740 mtfRegExpression 재사용을 위해 Tuple Row를 초기화한다. */
    aTemplate->rows[aNode->table].lflag &= ~MTC_TUPLE_ROW_MEMSET_MASK;
    aTemplate->rows[aNode->table].lflag |= MTC_TUPLE_ROW_MEMSET_TRUE;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfCompileExpression( mtcNode            * aPatternNode,
                             mtcTemplate        * aTemplate,
                             mtfRegExpression  ** aCompiledExpression,
                             mtcCallBack        * aCallBack )
{
    mtcColumn          * sPatternColumn;
    const mtdCharType  * sPatternValue;
    UShort               sPatternLength;
    UChar              * sPattern;
    mtfRegExpression   * sCompiledExpression;

    sPatternColumn = &(aTemplate->rows[aPatternNode->table].
                       columns[aPatternNode->column]);

    sPatternValue = (const mtdCharType *)
        mtd::valueForModule(
            (smiColumn*)&(sPatternColumn->column),
            aTemplate->rows[aPatternNode->table].row,
            MTD_OFFSET_USE,
            mtdChar.staticNull );

    sPattern       = (UChar*) sPatternValue->value;
    sPatternLength = sPatternValue->length;

    IDE_TEST_RAISE( sPatternLength > MTF_REGEXP_MAX_PATTERN_LENGTH, ERR_LONG_PATTERN );

    if ( sPatternLength > 0 )
    {
        IDE_TEST( aCallBack->alloc( aCallBack->info,
                                    MTF_REG_EXPRESSION_SIZE( sPatternLength ),
                                    (void**) &sCompiledExpression )
                  != IDE_SUCCESS );

        IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                         (const SChar *)sPattern,
                                         sPatternLength )
                  != IDE_SUCCESS );

        *aCompiledExpression = sCompiledExpression;
    }
    else
    {
        *aCompiledExpression = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LONG_PATTERN )
    {
        IDE_SET(ideSetErrorCode( mtERR_ABORT_REGEXP_LONG_PATTERN ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mtfRegExpLikeCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
 /***********************************************************************
 *
 * Description : RegExpLike의 Calculate 수행
 *               단일 바이트 문자(ASCII) 처리 부분
 *
 * Implementation :
 *    ex ) WHERE REGEXP_LIKE (dname, '^de[:ALPHA:]*_dep$')
 *
 *    aStack[0] : 결과
 *    aStack[1] : 읽어온 스트링
 *               ( eg. 'develop_dep'와 같은 dname 칼럼값)
 *    aStack[2] : 패턴 스트링
 *               ( eg. '^de[:ALPHA:]*_dep$'와 같은 패턴 일치 검사 조건값 )
 *    
 ***********************************************************************/ 

    const mtdModule        * sModule;
    const mtcColumn        * sColumn;
    mtdBinaryType          * sTempValue;
    mtdCharType            * sVarchar;
    UChar                  * sSearch;

    UChar                  * sPattern;
    UShort                   sPatternLength;
    UInt                     sSearchLength;

    mtfRegExpression       * sCompiledExpression;
    
    const SChar *sBeginStr;
    const SChar *sEndStr;
            
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sModule   = aStack[1].column->module;
    
    if( ( sModule->isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE ) ||
        ( sModule->isNull( aStack[2].column,
                           aStack[2].value ) == ID_TRUE ) )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_NULL;
    }
    else
    {        
        sVarchar        = (mtdCharType*)aStack[1].value;
        sSearch         = sVarchar->value;
        sSearchLength   = sVarchar->length;
        
        sVarchar        = (mtdCharType*)aStack[2].value;
        sPattern        = sVarchar->value;
        sPatternLength  = sVarchar->length;

        IDE_TEST_RAISE( sPatternLength > MTF_REGEXP_MAX_PATTERN_LENGTH, ERR_LONG_PATTERN );

        if ( sPatternLength == 0 )
        {
            /* NULL pattern */
            *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_NULL;
        }
        else
        {
            sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
            sTempValue      = (mtdBinaryType*)
                ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);
            sCompiledExpression = (mtfRegExpression*)(sTempValue->mValue);
            
            if ( sPatternLength != sCompiledExpression->patternLen )
            {
                IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                                 (const SChar *)sPattern,
                                                 sPatternLength )
                          != IDE_SUCCESS );
            
            }
            else
            {
                if ( idlOS::memcmp( sPattern,
                                    sCompiledExpression->patternBuf,
                                    sPatternLength ) != 0 )
                {
                    IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                                     (const SChar *)sPattern,
                                                     sPatternLength )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
            }
        
            if ( mtfRegExp::search( sCompiledExpression,
                                    (const SChar *)sSearch,
                                    sSearchLength,
                                    &sBeginStr,
                                    &sEndStr ) == ID_TRUE )
            {
                /* match */
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
            }
            else
            {
                /* No match */
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
            }
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_LONG_PATTERN );
    IDE_SET(ideSetErrorCode( mtERR_ABORT_REGEXP_LONG_PATTERN ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRegExpLikeCalculateFast( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : RegExpLike의 Calculate 수행
 *               단일 바이트 문자(ASCII) 처리 부분
 *
 * Implementation :
 *    ex ) WHERE REGEXP_LIKE (dname, '^de[:ALPHA:]*_dep$')
 *
 *    aStack[0] : 결과
 *    aStack[1] : 읽어온 스트링
 *               ( eg. 'develop_dep'와 같은 dname 칼럼값)
 *    aStack[2] : 패턴 스트링
 *               ( eg. '^de[:ALPHA:]*_dep$'와 같은 패턴 일치 검사 조건값 )
 *
 ***********************************************************************/
    
    const mtdModule        * sModule;
    mtdCharType            * sVarchar;
    UChar                  * sSearch;
    UInt                     sSearchLength;
    mtfRegExpression       * sCompiledExpression   = NULL;

    const SChar *sBeginStr;
    const SChar *sEndStr;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                    aStack,
                                    aRemain,
                                    aInfo,
                                    aTemplate )
             != IDE_SUCCESS );
    
    sModule = aStack[1].column->module;
    
    if( ( sModule->isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE ) ||
        ( sModule->isNull( aStack[2].column,
                           aStack[2].value ) == ID_TRUE ) )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_NULL;
    }
    else
    {
        sVarchar        = (mtdCharType*)aStack[1].value;
        sSearch         = sVarchar->value;
        sSearchLength   = sVarchar->length;
       
        IDE_DASSERT( aInfo != NULL );
        sCompiledExpression = (mtfRegExpression*)aInfo;

        if ( sCompiledExpression->patternLen == 0 )
        {
            /* NULL pattern */
            *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_NULL;
        }
        else
        {
            if ( mtfRegExp::search( sCompiledExpression,
                                    (const SChar *)sSearch,
                                    sSearchLength,
                                    &sBeginStr,
                                    &sEndStr ) == ID_TRUE )
            {
                /* match */
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
            }
            else
            {
                /* No match */
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
            }
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
