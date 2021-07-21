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

extern mtfModule mtfDecodeSumList;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfDecodeSumListFunctionName[1] = {
    { NULL, 15, (void*)"DECODE_SUM_LIST" }
};

static IDE_RC mtfDecodeSumListInitialize( void );

static IDE_RC mtfDecodeSumListFinalize( void );

static IDE_RC mtfDecodeSumListEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

mtfModule mtfDecodeSumList = {
    2|MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDecodeSumListFunctionName,
    NULL,
    mtfDecodeSumListInitialize,
    mtfDecodeSumListFinalize,
    mtfDecodeSumListEstimate
};

static IDE_RC mtfDecodeSumListEstimateFloat( mtcNode*     aNode,
                                             mtcTemplate* aTemplate,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             mtcCallBack* aCallBack );

IDE_RC mtfDecodeSumListInitializeFloat( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListAggregateFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListFinalizeFloat( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListCalculateFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeSumListExecuteFloat = {
    mtfDecodeSumListInitializeFloat,
    mtfDecodeSumListAggregateFloat,
    mtf::calculateNA,
    mtfDecodeSumListFinalizeFloat,
    mtfDecodeSumListCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfDecodeSumListEstimateDouble( mtcNode*     aNode,
                                              mtcTemplate* aTemplate,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              mtcCallBack* aCallBack );

IDE_RC mtfDecodeSumListInitializeDouble( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListAggregateDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListFinalizeDouble( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListCalculateDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeSumListExecuteDouble = {
    mtfDecodeSumListInitializeDouble,
    mtfDecodeSumListAggregateDouble,
    mtf::calculateNA,
    mtfDecodeSumListFinalizeDouble,
    mtfDecodeSumListCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfDecodeSumListEstimateBigint( mtcNode*     aNode,
                                              mtcTemplate* aTemplate,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              mtcCallBack* aCallBack );

IDE_RC mtfDecodeSumListInitializeBigint( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListAggregateBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListFinalizeBigint( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumListCalculateBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeSumListExecuteBigint = {
    mtfDecodeSumListInitializeBigint,
    mtfDecodeSumListAggregateBigint,
    mtf::calculateNA,
    mtfDecodeSumListFinalizeBigint,
    mtfDecodeSumListCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

typedef struct mtfDecodeSumSortedValue
{
    mtcColumn   * column;
    void        * value;
    SInt          idx;
} mtfDecodeSumSortedValue;

typedef struct mtfDecodeSumListCalculateInfo
{
    SInt                       sSearchCount;
    mtfDecodeSumSortedValue  * sSearchValue;  // sorted value

} mtfDecodeSumListCalculateInfo;

typedef struct mtfDecodeSumListInfo
{
    // 첫번째 인자
    mtcExecute     * sSumColumnExecute;
    mtcNode        * sSumColumnNode;

    // 두번째 인자
    mtcExecute     * sExprExecute;
    mtcNode        * sExprNode;

    // return 인자
    mtcColumn      * sReturnColumn;
    void           * sReturnValue;
    mtcStack       * sReturnStack;
    SInt             sReturnCount;
    idBool           sIsNullValue[1];  // MTC_NODE_ARGUMENT_COUNT_MAXIMUM까지 확장할 수 있음

} mtfDecodeSumListInfo;

extern "C" SInt
compareDecodeSumSortedValue( const void* aElem1, const void* aElem2 )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    mtfDecodeSumSortedValue  * sElem1 = (mtfDecodeSumSortedValue*)aElem1;
    mtfDecodeSumSortedValue  * sElem2 = (mtfDecodeSumSortedValue*)aElem2;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    IDE_ASSERT( sElem1 != NULL );
    IDE_ASSERT( sElem2 != NULL );

    sValueInfo1.column = (const mtcColumn *)sElem1->column;
    sValueInfo1.value  = sElem1->value;
    sValueInfo1.flag   = MTD_OFFSET_USELESS;
                
    sValueInfo2.column = (const mtcColumn *)sElem2->column;
    sValueInfo2.value  = sElem2->value;
    sValueInfo2.flag   = MTD_OFFSET_USELESS;

    IDE_DASSERT( sElem1->column->module->no == sElem2->column->module->no );

    return sElem1->column->module->logicalCompare[MTD_COMPARE_ASCENDING]
        ( &sValueInfo1, &sValueInfo2 );
}

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfDecodeSumListEstimates[3] = {
    { mtfDecodeSumListEstimates+1, mtfDecodeSumListEstimateFloat  },
    { mtfDecodeSumListEstimates+2, mtfDecodeSumListEstimateDouble },
    { NULL,                        mtfDecodeSumListEstimateBigint }
};

static mtfSubModule** mtfTable = NULL;

IDE_RC mtfDecodeSumListInitialize( void )
{
    return mtf::initializeTemplate( &mtfTable, mtfDecodeSumListEstimates, mtfXX );
}

IDE_RC mtfDecodeSumListFinalize( void )
{
    return mtf::finalizeTemplate( &mtfTable );
}

IDE_RC mtfDecodeSumListEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeSumListEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtfSubModule* sSubModule;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    // 1 혹은 3개의 인자
    IDE_TEST_RAISE( (sFence != 1) && (sFence != 3),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList,
                    ERR_CONVERSION_NOT_APPLICABLE );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getSubModule1Arg( &sSubModule,
                                     mtfTable,
                                     aStack[1].column->module->no )
              != IDE_SUCCESS );

    IDE_TEST( sSubModule->estimate( aNode,
                                    aTemplate,
                                    aStack,
                                    aRemain,
                                    aCallBack )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/* ZONE: FLOAT */

IDE_RC mtfDecodeSumListEstimateFloat( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt,
                                      mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeSumListEstimateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule * sTarget;
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcColumn       * sMtcColumn;
    mtcStack        * sListStack;
    mtcNode         * sNode;
    idBool            sIsConstValue;
    UInt              sBinaryPrecision;
    SInt              sListCount = 1;
    SInt              sCount;
    UInt              sFence;

    mtfDecodeSumListCalculateInfo * sCalculateInfo;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aStack[2].column->module == &mtdList,
                        ERR_CONVERSION_NOT_APPLICABLE );
        
        if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK )
             == MTC_NODE_REESTIMATE_FALSE )
        {
            if ( aStack[3].column->module != &mtdList )
            {
                // decode_sum_list( i1, i2, 1 )
        
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              aStack[2].column->module->no,
                              aStack[3].column->module->no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );

                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                sModules[0] = & mtdFloat;
                sModules[1] = sTarget;
                sModules[2] = sTarget;
            
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
                // decode_sum_list( i1, i2, (1,2,3,...) )
                //
                // aStack[2] = i2
                // aStack[3] = (1,2,3,...)
                // sListStack = (mtcStack*)aStack[3].value
                // sListStack[0] = 1
                // sListStack[1] = 2
                
                sListStack = (mtcStack*)aStack[3].value;
                sListCount = aStack[3].column->precision;

                /* BUG-40349 sListCount는 2이상이어야 한다. */
                IDE_TEST_RAISE( sListCount < 2, ERR_LIST_COUNT );

                // list의 모든 element가 동일한 type으로 convert되어야 하므로
                // list의 첫번째 element에 맞춘다.
                IDE_TEST_RAISE( sListStack[0].column->module == &mtdList,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              aStack[2].column->module->no,
                              sListStack[0].column->module->no )
                          != IDE_SUCCESS );
            
                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );
                
                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                for ( sCount = 0; sCount < sListCount; sCount++ )
                {
                    IDE_TEST_RAISE( sListStack[sCount].column->module == &mtdList,
                                    ERR_CONVERSION_NOT_APPLICABLE );
                
                    sModules[sCount] = sTarget;
                }
        
                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments->next->next->arguments,
                                                    aTemplate,
                                                    sListStack,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            
                sModules[0] = & mtdFloat;
                sModules[1] = sTarget;
                sModules[2] = & mtdList;
                
                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            }
            
            if ( sListCount == 1 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteFloat;

                // sum 결과를 저장함
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdFloat,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
        
                // sum info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeSumListInfo);
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteFloat;

                // sum 결과를 저장할 컬럼정보
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcColumn),
                                            (void**)&sMtcColumn )
                          != IDE_SUCCESS );
                
                // sum 결과를 저장함
                IDE_TEST( mtc::initializeColumn( sMtcColumn,
                                                 & mtdFloat,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
                
                // execution용 sListCount개의 stack과 float value를 저장할 공간을 설정한다.
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdList,
                                                 2,
                                                 sListCount,
                                                 sMtcColumn->column.size * sListCount )
                          != IDE_SUCCESS );
                
                // estimate용 sListCount개의 stack을 생성한다.
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcStack) * sListCount,
                                            (void**)&(aStack[0].value) )
                          != IDE_SUCCESS);
                
                // list stack을 smiColumn.value에 기록해둔다.
                aStack[0].column->column.value = aStack[0].value;

                sListStack = (mtcStack*)aStack[0].value;
                for ( sCount = 0; sCount < sListCount; sCount++ )
                {
                    sListStack[sCount].column = sMtcColumn;
                    sListStack[sCount].value  = sMtcColumn->module->staticNull;
                }
                
                // sum info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeSumListInfo) +
                    ID_SIZEOF(idBool) * ( sListCount - 1 );
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            if ( aStack[3].column->module != &mtdList )
            {
                sNode = mtf::convertedNode( aNode->arguments->next->next,
                                            aTemplate );
                
                if ( ( sNode == aNode->arguments->next->next )
                     &&
                     ( ( aTemplate->rows[sNode->table].lflag
                         & MTC_TUPLE_TYPE_MASK )
                       == MTC_TUPLE_TYPE_CONSTANT ) )
                {
                    sIsConstValue = ID_TRUE;
                }
                else
                {
                    sIsConstValue = ID_FALSE;
                }
            }
            else
            {
                sIsConstValue = ID_TRUE;
                
                for ( sNode = aNode->arguments->next->next->arguments;
                      sNode != NULL;
                      sNode = sNode->next )
                {    
                    if ( ( mtf::convertedNode( sNode, aTemplate ) == sNode )
                         &&
                         ( ( aTemplate->rows[sNode->table].lflag
                             & MTC_TUPLE_TYPE_MASK )
                           == MTC_TUPLE_TYPE_CONSTANT ) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsConstValue = ID_FALSE;
                        break;
                    }
                }
            }

            if ( sIsConstValue == ID_TRUE )
            {
                // mtfDecodeSumListCalculateInfo 저장할 공간을 할당
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtfDecodeSumListCalculateInfo),
                                            (void**) & sCalculateInfo )
                          != IDE_SUCCESS );
                
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = sCalculateInfo;
            
                if ( aStack[3].column->module != &mtdList )
                {
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeSumSortedValue),
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn),
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    // decode_sum_list( i1, i2, 1 )
                    sNode = aNode->arguments->next->next;

                    sCalculateInfo->sSearchCount = 1;
                    
                    // 상수 tuple은 재할당되므로 복사해서 저장한다.
                    mtc::copyColumn( sMtcColumn,
                                     &(aTemplate->rows[sNode->table].columns[sNode->column]) );
                    
                    sCalculateInfo->sSearchValue[0].column = sMtcColumn;
                    sCalculateInfo->sSearchValue[0].value = (void*)
                        mtc::value( sCalculateInfo->sSearchValue[0].column,
                                    aTemplate->rows[sNode->table].row,
                                    MTD_OFFSET_USE );
                    sCalculateInfo->sSearchValue[0].idx = 0;
                }
                else
                {
                    // decode_sum_list( i1, i2, (1,2,3,...) )
                    sCalculateInfo->sSearchCount = aStack[3].column->precision;
                
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeSumSortedValue) * sCalculateInfo->sSearchCount,
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn) * sCalculateInfo->sSearchCount,
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    sListStack = (mtcStack*)aStack[3].value;
                    sListCount = aStack[3].column->precision;
                    
                    for ( sCount = 0, sNode = aNode->arguments->next->next->arguments;
                          ( sCount < sCalculateInfo->sSearchCount ) && ( sNode != NULL );
                          sCount++, sNode = sNode->next, sMtcColumn++ )
                    {
                        // 모두 동일 type이어야 한다.
                        IDE_DASSERT( sListStack[0].column->module->no ==
                                     sListStack[sCount].column->module->no );
                        
                        // 상수 tuple은 재할당되므로 복사해서 저장한다.
                        mtc::copyColumn( sMtcColumn,
                                         &(aTemplate->rows[sNode->table].columns[sNode->column]) );
                        
                        sCalculateInfo->sSearchValue[sCount].column = sMtcColumn;
                        sCalculateInfo->sSearchValue[sCount].value = (void*)
                            mtc::value( sCalculateInfo->sSearchValue[sCount].column,
                                        aTemplate->rows[sNode->table].row,
                                        MTD_OFFSET_USE );
                        sCalculateInfo->sSearchValue[sCount].idx = sCount;
                    }
                    
                    // sort
                    IDE_DASSERT( sCalculateInfo->sSearchCount > 1 );
        
                    idlOS::qsort( sCalculateInfo->sSearchValue,
                                  sCalculateInfo->sSearchCount,
                                  ID_SIZEOF(mtfDecodeSumSortedValue),
                                  compareDecodeSumSortedValue );
                
                    // 중복이 있어서는 안된다. (bsearch는 한개만 찾아준다.)
                    for ( sCount = 1; sCount < sCalculateInfo->sSearchCount; sCount++ )
                    {
                        sValueInfo1.column = (const mtcColumn *)
                            sCalculateInfo->sSearchValue[sCount - 1].column;
                        sValueInfo1.value  =
                            sCalculateInfo->sSearchValue[sCount - 1].value;
                        sValueInfo1.flag   = MTD_OFFSET_USELESS;
                    
                        sValueInfo2.column = (const mtcColumn *)
                            sCalculateInfo->sSearchValue[sCount].column;
                        sValueInfo2.value  =
                            sCalculateInfo->sSearchValue[sCount].value;
                        sValueInfo2.flag   = MTD_OFFSET_USELESS;
                    
                        IDE_TEST_RAISE( sCalculateInfo->sSearchValue[sCount].column->module->
                                        logicalCompare[MTD_COMPARE_ASCENDING]
                                        ( &sValueInfo1,
                                          &sValueInfo2 )
                                        == 0,
                                        ERR_INVALID_FUNCTION_ARGUMENT );
                    }
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        sModules[0] = & mtdFloat;
        
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
        
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteFloat;

        // sum 결과를 저장함
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdFloat,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
        
        // sum info는 필요없다.
        IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                         & mtdBinary,
                                         1,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    if ( sFence == 3 )
    {
        // decode_sum_list(i1, i2, (1,2,3))과 같이 세번째 인자가 상수인 경우
        if ( sListCount > 1 )
        {
            sIsConstValue = ID_TRUE;
            
            for ( sCount = 0, sNode = aNode->arguments->next->next->arguments;
                  ( sCount < sListCount ) && ( sNode != NULL );
                  sCount++, sNode = sNode->next )
            {
                if ( ( MTC_NODE_IS_DEFINED_VALUE( sNode ) == ID_TRUE )
                     &&
                     ( ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                         == MTC_TUPLE_TYPE_CONSTANT ) ||
                       ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                         == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
                {
                    // Nothing to do.
                }
                else
                {
                    sIsConstValue = ID_FALSE;
                    break;
                }
            }

            if ( sIsConstValue == ID_TRUE )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            }
            else
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
        }
        else
        {
            if ( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next->next ) == ID_TRUE )
                 &&
                 ( ( ( aTemplate->rows[aNode->arguments->next->next->table].lflag
                       & MTC_TUPLE_TYPE_MASK )
                     == MTC_TUPLE_TYPE_CONSTANT ) ||
                   ( ( aTemplate->rows[aNode->arguments->next->next->table].lflag
                       & MTC_TUPLE_TYPE_MASK )
                     == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            }
            else
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
        }
            
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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION( ERR_LIST_COUNT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeSumListEstimateFloat",
                              "invalid list count" ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeSumListInitializeFloat( mtcNode*     aNode,
                                        mtcStack*,
                                        SInt,
                                        void*,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListInitializeFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtcNode               * sArgNode[2];
    mtdBinaryType         * sValue;
    mtfDecodeSumListInfo  * sInfo;
    mtcStack              * sTempStack;
    UChar                 * sTempValue;
    mtdNumericType        * sFloat;
    SInt                    sCount;
    UInt                    sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_TEST_RAISE( sInfo == NULL, ERR_LIST_INFO );

    //-----------------------------
    // sum info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // sum column 설정
    sInfo->sSumColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sSumColumnNode    = sArgNode[0];

    if ( sFence == 3 )
    {
        sArgNode[1] = sArgNode[0]->next;

        // expression column 설정
        sInfo->sExprExecute = aTemplate->rows[sArgNode[1]->table].execute + sArgNode[1]->column;
        sInfo->sExprNode    = sArgNode[1];
    }
    else
    {
        // Nothing to do.
    }
    
    // return column 설정
    sInfo->sReturnColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sInfo->sReturnValue  = (void *)
        ((UChar*) aTemplate->rows[aNode->table].row + sInfo->sReturnColumn->column.offset);
    
    if ( sInfo->sReturnColumn->module != &mtdList )
    {
        sInfo->sReturnStack = NULL;
        sInfo->sReturnCount = 1;
    }
    else
    {
        sInfo->sReturnStack = (mtcStack*)sInfo->sReturnValue;
        sInfo->sReturnCount = sInfo->sReturnColumn->precision;
        
        // stack 초기화
        // (1) estimate때 생성한 column 정보로 초기화
        // (2) value를 실제 값으로 설정
        sTempStack = (mtcStack*) sInfo->sReturnColumn->column.value;
        sTempValue = 
            ( (UChar*)sInfo->sReturnStack + ID_SIZEOF(mtcStack) * sInfo->sReturnCount );
        
        for ( sCount = 0;
              sCount < sInfo->sReturnCount;
              sCount++, sTempStack++ )
        {
            sInfo->sReturnStack[sCount].column = sTempStack->column;
            sInfo->sReturnStack[sCount].value  = sTempValue;

            // BUG-42973 float은 1byte align이어서 고려할 필요가 없다.
            sTempValue += sInfo->sReturnStack[sCount].column->column.size;
        }

        IDE_DASSERT( sTempValue ==
                     (UChar*) sInfo->sReturnValue + sInfo->sReturnColumn->column.size );
    }
    
    //-----------------------------
    // sum 결과를 초기화
    //-----------------------------
    
    if ( sInfo->sReturnStack == NULL )
    {
        IDE_DASSERT( sInfo->sReturnColumn->module == &mtdFloat );
        
        sFloat = (mtdNumericType*)(sInfo->sReturnValue);
        sFloat->length       = 1;
        sFloat->signExponent = 0x80;
        
        sInfo->sIsNullValue[0] = ID_TRUE;
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            IDE_DASSERT( sInfo->sReturnStack[sCount].column->module == &mtdFloat );
            
            sFloat = (mtdNumericType*)(sInfo->sReturnStack[sCount].value);
            sFloat->length       = 1;
            sFloat->signExponent = 0x80;
            
            sInfo->sIsNullValue[sCount] = ID_TRUE;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LIST_INFO );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeSumListInitializeFloat",
                              "invalid list info" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeSumListAggregateFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListAggregateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule          * sModule;
    const mtcColumn          * sColumn;
    mtdNumericType           * sFloatSum;
    mtdNumericType           * sFloatArgument;
    UChar                      sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType           * sFloatSumClone;
    mtdBinaryType            * sValue;
    mtfDecodeSumListInfo     * sInfo;
    mtfDecodeSumSortedValue    sExprValue;
    mtfDecodeSumSortedValue  * sFound;
    UInt                       sFence;

    mtfDecodeSumListCalculateInfo * sCalculateInfo = (mtfDecodeSumListCalculateInfo*) aInfo;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    /* BUG-39635 Codesonar warning */
    sExprValue.column = NULL;
    sExprValue.value  = NULL;
    sExprValue.idx    = 0;
        
    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

        // 세번째 인자는 반드시 상수여야 한다.
        IDE_TEST_RAISE( sCalculateInfo == NULL, ERR_INVALID_FUNCTION_ARGUMENT );
        
        // 두번째 인자
        IDE_TEST( sInfo->sExprExecute->calculate( sInfo->sExprNode,
                                                  aStack,
                                                  aRemain,
                                                  sInfo->sExprExecute->calculateInfo,
                                                  aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sExprNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sExprNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // decode 연산수행
        sExprValue.column = aStack[0].column;
        sExprValue.value  = aStack[0].value;
        sExprValue.idx    = 0;
        
        sFound = (mtfDecodeSumSortedValue*)
            idlOS::bsearch( & sExprValue,
                            sCalculateInfo->sSearchValue,
                            sCalculateInfo->sSearchCount,
                            ID_SIZEOF(mtfDecodeSumSortedValue),
                            compareDecodeSumSortedValue );
    }
    else
    {
        // null만 아니면 됨
        sFound = & sExprValue;
    }

    if ( sFound != NULL )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sSumColumnExecute->calculate( sInfo->sSumColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sSumColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sSumColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSumColumnNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        IDE_DASSERT( aStack[0].column->module == & mtdFloat );

        sModule = aStack[0].column->module;

        if ( sModule->isNull( aStack[0].column,
                              aStack[0].value ) != ID_TRUE )
        {
            if ( sInfo->sReturnStack == NULL )
            {
                sFloatSum      = (mtdNumericType*)sInfo->sReturnValue;
                sFloatArgument = (mtdNumericType*)aStack[0].value;
                sFloatSumClone = (mtdNumericType*)sFloatSumBuff;
                idlOS::memcpy( sFloatSumClone, sFloatSum, sFloatSum->length + 1 );
                
                IDE_TEST( mtc::addFloat( sFloatSum,
                                         MTD_FLOAT_PRECISION_MAXIMUM,
                                         sFloatSumClone,
                                         sFloatArgument )
                          != IDE_SUCCESS );

                if ( sInfo->sIsNullValue[0] == ID_TRUE )
                {
                    sInfo->sIsNullValue[0] = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                IDE_DASSERT( sFound->idx < sInfo->sReturnCount );

                sFloatSum      = (mtdNumericType*)(sInfo->sReturnStack[sFound->idx].value);
                sFloatArgument = (mtdNumericType*)aStack[0].value;
                sFloatSumClone = (mtdNumericType*)sFloatSumBuff;
                idlOS::memcpy( sFloatSumClone, sFloatSum, sFloatSum->length + 1 );
                
                IDE_TEST( mtc::addFloat( sFloatSum,
                                         MTD_FLOAT_PRECISION_MAXIMUM,
                                         sFloatSumClone,
                                         sFloatArgument )
                          != IDE_SUCCESS );
                
                if ( sInfo->sIsNullValue[sFound->idx] == ID_TRUE )
                {
                    sInfo->sIsNullValue[sFound->idx] = ID_FALSE;
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
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeSumListFinalizeFloat( mtcNode*     aNode,
                                      mtcStack*,
                                      SInt,
                                      void*,
                                      mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListFinalizeFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtdBinaryType         * sValue;
    mtfDecodeSumListInfo  * sInfo;
    SInt                    sCount;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    if ( sInfo->sReturnStack == NULL )
    {
        if ( sInfo->sIsNullValue[0] == ID_TRUE )
        {
            mtdFloat.null( sInfo->sReturnColumn,
                           sInfo->sReturnValue );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            if ( sInfo->sIsNullValue[sCount] == ID_TRUE )
            {
                mtdFloat.null( sInfo->sReturnStack[sCount].column,
                               sInfo->sReturnStack[sCount].value );
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeSumListCalculateFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt,
                                       void*,
                                       mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListCalculateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;

#undef IDE_FN
}

/* ZONE: DOUBLE */

IDE_RC mtfDecodeSumListEstimateDouble( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt,
                                       mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeSumListEstimateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule * sTarget;
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcColumn       * sMtcColumn;
    mtcStack        * sListStack;
    mtcNode         * sNode;
    idBool            sIsConstValue;
    UInt              sBinaryPrecision;
    SInt              sListCount = 1;
    SInt              sCount;
    UInt              sFence;

    mtfDecodeSumListCalculateInfo * sCalculateInfo;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aStack[2].column->module == &mtdList,
                        ERR_CONVERSION_NOT_APPLICABLE );
        
        if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK )
             == MTC_NODE_REESTIMATE_FALSE )
        {
            if ( aStack[3].column->module != &mtdList )
            {
                // decode_sum_list( i1, i2, 1 )
        
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              aStack[2].column->module->no,
                              aStack[3].column->module->no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );

                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                sModules[0] = & mtdDouble;
                sModules[1] = sTarget;
                sModules[2] = sTarget;
            
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
                // decode_sum_list( i1, i2, (1,2,3,...) )
                //
                // aStack[2] = i2
                // aStack[3] = (1,2,3,...)
                // sListStack = (mtcStack*)aStack[3].value
                // sListStack[0] = 1
                // sListStack[1] = 2
                
                sListStack = (mtcStack*)aStack[3].value;
                sListCount = aStack[3].column->precision;

                /* BUG-40349 sListCount는 2이상이어야 한다. */
                IDE_TEST_RAISE( sListCount < 2, ERR_LIST_COUNT );

                // list의 모든 element가 동일한 type으로 convert되어야 하므로
                // list의 첫번째 element에 맞춘다.
                IDE_TEST_RAISE( sListStack[0].column->module == &mtdList,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              aStack[2].column->module->no,
                              sListStack[0].column->module->no )
                          != IDE_SUCCESS );
            
                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );
                
                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                for ( sCount = 0; sCount < sListCount; sCount++ )
                {
                    IDE_TEST_RAISE( sListStack[sCount].column->module == &mtdList,
                                    ERR_CONVERSION_NOT_APPLICABLE );
                
                    sModules[sCount] = sTarget;
                }
        
                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments->next->next->arguments,
                                                    aTemplate,
                                                    sListStack,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            
                sModules[0] = & mtdDouble;
                sModules[1] = sTarget;
                sModules[2] = & mtdList;
                
                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            }
            
            if ( sListCount == 1 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteDouble;

                // sum 결과를 저장함
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdDouble,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
        
                // sum info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeSumListInfo);
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteDouble;

                // sum 결과를 저장할 컬럼정보
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcColumn),
                                            (void**)&sMtcColumn )
                          != IDE_SUCCESS );
        
                // sum 결과를 저장함
                IDE_TEST( mtc::initializeColumn( sMtcColumn,
                                                 & mtdDouble,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
        
                // execution용 sListCount개의 stack과 double value를 저장할 공간을 설정한다.
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdList,
                                                 2,
                                                 sListCount,
                                                 sMtcColumn->column.size * sListCount )
                          != IDE_SUCCESS );

                // estimate용 sListCount개의 stack을 생성한다.
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcStack) * sListCount,
                                            (void**)&(aStack[0].value) )
                          != IDE_SUCCESS);

                // list stack을 smiColumn.value에 기록해둔다.
                aStack[0].column->column.value = aStack[0].value;

                sListStack = (mtcStack*)aStack[0].value;
                for ( sCount = 0; sCount < sListCount; sCount++ )
                {
                    sListStack[sCount].column = sMtcColumn;
                    sListStack[sCount].value  = sMtcColumn->module->staticNull;
                }
        
                // sum info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeSumListInfo) +
                    ID_SIZEOF(idBool) * ( sListCount - 1 );
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            if ( aStack[3].column->module != &mtdList )
            {
                sNode = mtf::convertedNode( aNode->arguments->next->next,
                                            aTemplate );
                
                if ( ( sNode == aNode->arguments->next->next )
                     &&
                     ( ( aTemplate->rows[sNode->table].lflag
                         & MTC_TUPLE_TYPE_MASK )
                       == MTC_TUPLE_TYPE_CONSTANT ) )
                {
                    sIsConstValue = ID_TRUE;
                }
                else
                {
                    sIsConstValue = ID_FALSE;
                }
            }
            else
            {
                sIsConstValue = ID_TRUE;
                    
                for ( sNode = aNode->arguments->next->next->arguments;
                      sNode != NULL;
                      sNode = sNode->next )
                {
                    if ( ( mtf::convertedNode( sNode, aTemplate ) == sNode )
                         &&
                         ( ( aTemplate->rows[sNode->table].lflag
                             & MTC_TUPLE_TYPE_MASK )
                           == MTC_TUPLE_TYPE_CONSTANT ) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsConstValue = ID_FALSE;
                        break;
                    }
                }
            }

            if ( sIsConstValue == ID_TRUE )
            {
                // mtfDecodeSumListCalculateInfo 저장할 공간을 할당
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtfDecodeSumListCalculateInfo),
                                            (void**) & sCalculateInfo )
                          != IDE_SUCCESS );
                
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = sCalculateInfo;
            
                if ( aStack[3].column->module != &mtdList )
                {
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeSumSortedValue),
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn),
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    // decode_sum_list( i1, i2, 1 )
                    sNode = aNode->arguments->next->next;

                    sCalculateInfo->sSearchCount = 1;
                    
                    // 상수 tuple은 재할당되므로 복사해서 저장한다.
                    mtc::copyColumn( sMtcColumn,
                                     &(aTemplate->rows[sNode->table].columns[sNode->column]) );
                    
                    sCalculateInfo->sSearchValue[0].column = sMtcColumn;
                    sCalculateInfo->sSearchValue[0].value = (void*)
                        mtc::value( sCalculateInfo->sSearchValue[0].column,
                                    aTemplate->rows[sNode->table].row,
                                    MTD_OFFSET_USE );
                    sCalculateInfo->sSearchValue[0].idx = 0;
                }
                else
                {
                    // decode_sum_list( i1, i2, (1,2,3,...) )
                    sCalculateInfo->sSearchCount = aStack[3].column->precision;
                
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeSumSortedValue) * sCalculateInfo->sSearchCount,
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn) * sCalculateInfo->sSearchCount,
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    sListStack = (mtcStack*)aStack[3].value;
                    sListCount = aStack[3].column->precision;
                    
                    for ( sCount = 0, sNode = aNode->arguments->next->next->arguments;
                          ( sCount < sCalculateInfo->sSearchCount ) && ( sNode != NULL );
                          sCount++, sNode = sNode->next, sMtcColumn++ )
                    {
                        // 모두 동일 type이어야 한다.
                        IDE_DASSERT( sListStack[0].column->module->no ==
                                     sListStack[sCount].column->module->no );
                        
                        // 상수 tuple은 재할당되므로 복사해서 저장한다.
                        mtc::copyColumn( sMtcColumn,
                                         &(aTemplate->rows[sNode->table].columns[sNode->column]) );
                        
                        sCalculateInfo->sSearchValue[sCount].column = sMtcColumn;
                        sCalculateInfo->sSearchValue[sCount].value = (void*)
                            mtc::value( sCalculateInfo->sSearchValue[sCount].column,
                                        aTemplate->rows[sNode->table].row,
                                        MTD_OFFSET_USE );
                        sCalculateInfo->sSearchValue[sCount].idx = sCount;
                    }
                    
                    // sort
                    IDE_DASSERT( sCalculateInfo->sSearchCount > 1 );
        
                    idlOS::qsort( sCalculateInfo->sSearchValue,
                                  sCalculateInfo->sSearchCount,
                                  ID_SIZEOF(mtfDecodeSumSortedValue),
                                  compareDecodeSumSortedValue );
                
                    // 중복이 있어서는 안된다. (bsearch는 한개만 찾아준다.)
                    for ( sCount = 1; sCount < sCalculateInfo->sSearchCount; sCount++ )
                    {
                        sValueInfo1.column = (const mtcColumn *)
                            sCalculateInfo->sSearchValue[sCount - 1].column;
                        sValueInfo1.value  =
                            sCalculateInfo->sSearchValue[sCount - 1].value;
                        sValueInfo1.flag   = MTD_OFFSET_USELESS;
                    
                        sValueInfo2.column = (const mtcColumn *)
                            sCalculateInfo->sSearchValue[sCount].column;
                        sValueInfo2.value  =
                            sCalculateInfo->sSearchValue[sCount].value;
                        sValueInfo2.flag   = MTD_OFFSET_USELESS;
                    
                        IDE_TEST_RAISE( sCalculateInfo->sSearchValue[sCount].column->module->
                                        logicalCompare[MTD_COMPARE_ASCENDING]
                                        ( &sValueInfo1,
                                          &sValueInfo2 )
                                        == 0,
                                        ERR_INVALID_FUNCTION_ARGUMENT );
                    }
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        sModules[0] = & mtdDouble;
        
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
        
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteDouble;

        // sum 결과를 저장함
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdDouble,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
        
        // sum info는 필요없다.
        IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                         & mtdBinary,
                                         1,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    if ( sFence == 3 )
    {
        // decode_sum_list(i1, i2, (1,2,3))과 같이 세번째 인자가 상수인 경우
        if ( sListCount > 1 )
        {
            sIsConstValue = ID_TRUE;
            
            for ( sCount = 0, sNode = aNode->arguments->next->next->arguments;
                  ( sCount < sListCount ) && ( sNode != NULL );
                  sCount++, sNode = sNode->next )
            {
                if ( ( MTC_NODE_IS_DEFINED_VALUE( sNode ) == ID_TRUE )
                     &&
                     ( ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                         == MTC_TUPLE_TYPE_CONSTANT ) ||
                       ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                         == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
                {
                    // Nothing to do.
                }
                else
                {
                    sIsConstValue = ID_FALSE;
                    break;
                }
            }

            if ( sIsConstValue == ID_TRUE )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            }
            else
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
        }
        else
        {
            if ( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next->next ) == ID_TRUE )
                 &&
                 ( ( ( aTemplate->rows[aNode->arguments->next->next->table].lflag
                       & MTC_TUPLE_TYPE_MASK )
                     == MTC_TUPLE_TYPE_CONSTANT ) ||
                   ( ( aTemplate->rows[aNode->arguments->next->next->table].lflag
                       & MTC_TUPLE_TYPE_MASK )
                     == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            }
            else
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
        }
            
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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION( ERR_LIST_COUNT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeSumListEstimateDouble",
                              "invalid list count" ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeSumListInitializeDouble( mtcNode*     aNode,
                                         mtcStack*,
                                         SInt,
                                         void*,
                                         mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListInitializeDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtcNode               * sArgNode[2];
    mtdBinaryType         * sValue;
    mtfDecodeSumListInfo  * sInfo;
    mtcStack              * sTempStack;
    mtdDoubleType         * sTempValue;
    SInt                    sCount;
    UInt                    sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_TEST_RAISE( sInfo == NULL, ERR_LIST_INFO );

    //-----------------------------
    // sum info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // sum column 설정
    sInfo->sSumColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sSumColumnNode    = sArgNode[0];

    if ( sFence == 3 )
    {
        sArgNode[1] = sArgNode[0]->next;

        // expression column 설정
        sInfo->sExprExecute = aTemplate->rows[sArgNode[1]->table].execute + sArgNode[1]->column;
        sInfo->sExprNode    = sArgNode[1];
    }
    else
    {
        // Nothing to do.
    }
    
    // return column 설정
    sInfo->sReturnColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sInfo->sReturnValue  = (void *)
        ((UChar*) aTemplate->rows[aNode->table].row + sInfo->sReturnColumn->column.offset);
    
    if ( sInfo->sReturnColumn->module != &mtdList )
    {
        sInfo->sReturnStack = NULL;
        sInfo->sReturnCount = 1;
    }
    else
    {
        sInfo->sReturnStack = (mtcStack*)sInfo->sReturnValue;
        sInfo->sReturnCount = sInfo->sReturnColumn->precision;
        
        // stack 초기화
        // (1) estimate때 생성한 column 정보로 초기화
        // (2) value를 실제 값으로 설정
        sTempStack = (mtcStack*) sInfo->sReturnColumn->column.value;
        sTempValue = (mtdDoubleType*)
            ( (UChar*)sInfo->sReturnStack + ID_SIZEOF(mtcStack) * sInfo->sReturnCount );
        
        for ( sCount = 0;
              sCount < sInfo->sReturnCount;
              sCount++, sTempStack++, sTempValue++ )
        {
            sInfo->sReturnStack[sCount].column = sTempStack->column;
            sInfo->sReturnStack[sCount].value  = sTempValue;
        }
        
        IDE_DASSERT( (UChar*) sTempValue ==
                     (UChar*) sInfo->sReturnValue + sInfo->sReturnColumn->column.size );
    }
    
    //-----------------------------
    // sum 결과를 초기화
    //-----------------------------
    
    if ( sInfo->sReturnStack == NULL )
    {
        IDE_DASSERT( sInfo->sReturnColumn->module == &mtdDouble );
        
        *(mtdDoubleType*)(sInfo->sReturnValue) = 0;
        sInfo->sIsNullValue[0] = ID_TRUE;
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            IDE_DASSERT( sInfo->sReturnStack[sCount].column->module == &mtdDouble );
            
            *(mtdDoubleType*)(sInfo->sReturnStack[sCount].value) = 0;
            sInfo->sIsNullValue[sCount] = ID_TRUE;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LIST_INFO );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeSumListInitializeDouble",
                              "invalid list info" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeSumListAggregateDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListAggregateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule          * sModule;
    const mtcColumn          * sColumn;
    mtdBinaryType            * sValue;
    mtfDecodeSumListInfo     * sInfo;
    mtfDecodeSumSortedValue    sExprValue;
    mtfDecodeSumSortedValue  * sFound;
    UInt                       sFence;

    mtfDecodeSumListCalculateInfo * sCalculateInfo = (mtfDecodeSumListCalculateInfo*) aInfo;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    /* BUG-39635 Codesonar warning */
    sExprValue.column = NULL;
    sExprValue.value  = NULL;
    sExprValue.idx    = 0;

    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

        // 세번째 인자는 반드시 상수여야 한다.
        IDE_TEST_RAISE( sCalculateInfo == NULL, ERR_INVALID_FUNCTION_ARGUMENT );
        
        // 두번째 인자
        IDE_TEST( sInfo->sExprExecute->calculate( sInfo->sExprNode,
                                                  aStack,
                                                  aRemain,
                                                  sInfo->sExprExecute->calculateInfo,
                                                  aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sExprNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sExprNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // decode 연산수행
        sExprValue.column = aStack[0].column;
        sExprValue.value  = aStack[0].value;
        sExprValue.idx    = 0;
        
        sFound = (mtfDecodeSumSortedValue*)
            idlOS::bsearch( & sExprValue,
                            sCalculateInfo->sSearchValue,
                            sCalculateInfo->sSearchCount,
                            ID_SIZEOF(mtfDecodeSumSortedValue),
                            compareDecodeSumSortedValue );
    }
    else
    {
        // null만 아니면 됨
        sFound = & sExprValue;
    }

    if ( sFound != NULL )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sSumColumnExecute->calculate( sInfo->sSumColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sSumColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sSumColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSumColumnNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        IDE_DASSERT( aStack[0].column->module == & mtdDouble );

        sModule = aStack[0].column->module;

        if ( sModule->isNull( aStack[0].column,
                              aStack[0].value ) != ID_TRUE )
        {
            if ( sInfo->sReturnStack == NULL )
            {
                *(mtdDoubleType*)(sInfo->sReturnValue) +=
                    *(mtdDoubleType*) aStack[0].value;

                if ( sInfo->sIsNullValue[0] == ID_TRUE )
                {
                    sInfo->sIsNullValue[0] = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                IDE_DASSERT( sFound->idx < sInfo->sReturnCount );
                
                *(mtdDoubleType*)(sInfo->sReturnStack[sFound->idx].value) +=
                    *(mtdDoubleType*) aStack[0].value;
                
                if ( sInfo->sIsNullValue[sFound->idx] == ID_TRUE )
                {
                    sInfo->sIsNullValue[sFound->idx] = ID_FALSE;
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
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeSumListFinalizeDouble( mtcNode*     aNode,
                                       mtcStack*,
                                       SInt,
                                       void*,
                                       mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListFinalizeDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtdBinaryType         * sValue;
    mtfDecodeSumListInfo  * sInfo;
    SInt                    sCount;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    if ( sInfo->sReturnStack == NULL )
    {
        if ( sInfo->sIsNullValue[0] == ID_TRUE )
        {
            mtdDouble.null( sInfo->sReturnColumn,
                            sInfo->sReturnValue );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            if ( sInfo->sIsNullValue[sCount] == ID_TRUE )
            {
                mtdDouble.null( sInfo->sReturnStack[sCount].column,
                                sInfo->sReturnStack[sCount].value );
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeSumListCalculateDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt,
                                        void*,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListCalculateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    return IDE_SUCCESS;
#undef IDE_FN
}

/* ZONE: BIGINT */

IDE_RC mtfDecodeSumListEstimateBigint( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt,
                                       mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeSumListEstimateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule * sTarget;
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcColumn       * sMtcColumn;
    mtcStack        * sListStack;
    mtcNode         * sNode;
    idBool            sIsConstValue;
    UInt              sBinaryPrecision;
    SInt              sListCount = 1;
    SInt              sCount;
    UInt              sFence;

    mtfDecodeSumListCalculateInfo * sCalculateInfo;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aStack[2].column->module == &mtdList,
                        ERR_CONVERSION_NOT_APPLICABLE );
        
        if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK )
             == MTC_NODE_REESTIMATE_FALSE )
        {
            if ( aStack[3].column->module != &mtdList )
            {
                // decode_sum_list( i1, i2, 1 )
        
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              aStack[2].column->module->no,
                              aStack[3].column->module->no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );

                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                sModules[0] = & mtdBigint;
                sModules[1] = sTarget;
                sModules[2] = sTarget;
            
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
                // decode_sum_list( i1, i2, (1,2,3,...) )
                //
                // aStack[2] = i2
                // aStack[3] = (1,2,3,...)
                // sListStack = (mtcStack*)aStack[3].value
                // sListStack[0] = 1
                // sListStack[1] = 2
                
                sListStack = (mtcStack*)aStack[3].value;
                sListCount = aStack[3].column->precision;

                /* BUG-40349 sListCount는 2이상이어야 한다. */
                IDE_TEST_RAISE( sListCount < 2, ERR_LIST_COUNT );

                // list의 모든 element가 동일한 type으로 convert되어야 하므로
                // list의 첫번째 element에 맞춘다.
                IDE_TEST_RAISE( sListStack[0].column->module == &mtdList,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              aStack[2].column->module->no,
                              sListStack[0].column->module->no )
                          != IDE_SUCCESS );
            
                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );
                
                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );
        
                for ( sCount = 0; sCount < sListCount; sCount++ )
                {
                    IDE_TEST_RAISE( sListStack[sCount].column->module == &mtdList,
                                    ERR_CONVERSION_NOT_APPLICABLE );
                
                    sModules[sCount] = sTarget;
                }
        
                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments->next->next->arguments,
                                                    aTemplate,
                                                    sListStack,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            
                sModules[0] = & mtdBigint;
                sModules[1] = sTarget;
                sModules[2] = & mtdList;
                
                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            }
            
            if ( sListCount == 1 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteBigint;

                // sum 결과를 저장함
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdBigint,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
        
                // sum info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeSumListInfo);
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteBigint;

                // sum 결과를 저장할 컬럼정보
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcColumn),
                                            (void**)&sMtcColumn )
                          != IDE_SUCCESS );
        
                // sum 결과를 저장함
                IDE_TEST( mtc::initializeColumn( sMtcColumn,
                                                 & mtdBigint,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
        
                // execution용 sListCount개의 stack과 bigint value를 저장할 공간을 설정한다.
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdList,
                                                 2,
                                                 sListCount,
                                                 sMtcColumn->column.size * sListCount )
                          != IDE_SUCCESS );

                // estimate용 sListCount개의 stack을 생성한다.
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcStack) * sListCount,
                                            (void**)&(aStack[0].value) )
                          != IDE_SUCCESS);

                // list stack을 smiColumn.value에 기록해둔다.
                aStack[0].column->column.value = aStack[0].value;

                sListStack = (mtcStack*)aStack[0].value;
                for ( sCount = 0; sCount < sListCount; sCount++ )
                {
                    sListStack[sCount].column = sMtcColumn;
                    sListStack[sCount].value  = sMtcColumn->module->staticNull;
                }
        
                // sum info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeSumListInfo) +
                    ID_SIZEOF(idBool) * ( sListCount - 1 );
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            if ( aStack[3].column->module != &mtdList )
            {
                sNode = mtf::convertedNode( aNode->arguments->next->next,
                                            aTemplate );
                
                if ( ( sNode == aNode->arguments->next->next )
                     &&
                     ( ( aTemplate->rows[sNode->table].lflag
                         & MTC_TUPLE_TYPE_MASK )
                       == MTC_TUPLE_TYPE_CONSTANT ) )
                {
                    sIsConstValue = ID_TRUE;
                }
                else
                {
                    sIsConstValue = ID_FALSE;
                }
            }
            else
            {
                sIsConstValue = ID_TRUE;
                    
                for ( sNode = aNode->arguments->next->next->arguments;
                      sNode != NULL;
                      sNode = sNode->next )
                {
                    if ( ( mtf::convertedNode( sNode, aTemplate ) == sNode )
                         &&
                         ( ( aTemplate->rows[sNode->table].lflag
                             & MTC_TUPLE_TYPE_MASK )
                           == MTC_TUPLE_TYPE_CONSTANT ) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsConstValue = ID_FALSE;
                        break;
                    }
                }
            }

            if ( sIsConstValue == ID_TRUE )
            {
                // mtfDecodeSumListCalculateInfo 저장할 공간을 할당
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtfDecodeSumListCalculateInfo),
                                            (void**) & sCalculateInfo )
                          != IDE_SUCCESS );
                
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = sCalculateInfo;
            
                if ( aStack[3].column->module != &mtdList )
                {
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeSumSortedValue),
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn),
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    // decode_sum_list( i1, i2, 1 )
                    sNode = aNode->arguments->next->next;

                    sCalculateInfo->sSearchCount = 1;
                    
                    // 상수 tuple은 재할당되므로 복사해서 저장한다.
                    mtc::copyColumn( sMtcColumn,
                                     &(aTemplate->rows[sNode->table].columns[sNode->column]) );
                    
                    sCalculateInfo->sSearchValue[0].column = sMtcColumn;
                    sCalculateInfo->sSearchValue[0].value = (void*)
                        mtc::value( sCalculateInfo->sSearchValue[0].column,
                                    aTemplate->rows[sNode->table].row,
                                    MTD_OFFSET_USE );
                    sCalculateInfo->sSearchValue[0].idx = 0;
                }
                else
                {
                    // decode_sum_list( i1, i2, (1,2,3,...) )
                    sCalculateInfo->sSearchCount = aStack[3].column->precision;
                
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeSumSortedValue) * sCalculateInfo->sSearchCount,
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn) * sCalculateInfo->sSearchCount,
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    sListStack = (mtcStack*)aStack[3].value;
                    sListCount = aStack[3].column->precision;
                    
                    for ( sCount = 0, sNode = aNode->arguments->next->next->arguments;
                          ( sCount < sCalculateInfo->sSearchCount ) && ( sNode != NULL );
                          sCount++, sNode = sNode->next, sMtcColumn++ )
                    {
                        // 모두 동일 type이어야 한다.
                        IDE_DASSERT( sListStack[0].column->module->no ==
                                     sListStack[sCount].column->module->no );
                        
                        // 상수 tuple은 재할당되므로 복사해서 저장한다.
                        mtc::copyColumn( sMtcColumn,
                                         &(aTemplate->rows[sNode->table].columns[sNode->column]) );
                        
                        sCalculateInfo->sSearchValue[sCount].column = sMtcColumn;
                        sCalculateInfo->sSearchValue[sCount].value = (void*)
                            mtc::value( sCalculateInfo->sSearchValue[sCount].column,
                                        aTemplate->rows[sNode->table].row,
                                        MTD_OFFSET_USE );
                        sCalculateInfo->sSearchValue[sCount].idx = sCount;
                    }
                    
                    // sort
                    IDE_DASSERT( sCalculateInfo->sSearchCount > 1 );
        
                    idlOS::qsort( sCalculateInfo->sSearchValue,
                                  sCalculateInfo->sSearchCount,
                                  ID_SIZEOF(mtfDecodeSumSortedValue),
                                  compareDecodeSumSortedValue );
                
                    // 중복이 있어서는 안된다. (bsearch는 한개만 찾아준다.)
                    for ( sCount = 1; sCount < sCalculateInfo->sSearchCount; sCount++ )
                    {
                        sValueInfo1.column = (const mtcColumn *)
                            sCalculateInfo->sSearchValue[sCount - 1].column;
                        sValueInfo1.value  =
                            sCalculateInfo->sSearchValue[sCount - 1].value;
                        sValueInfo1.flag   = MTD_OFFSET_USELESS;
                    
                        sValueInfo2.column = (const mtcColumn *)
                            sCalculateInfo->sSearchValue[sCount].column;
                        sValueInfo2.value  =
                            sCalculateInfo->sSearchValue[sCount].value;
                        sValueInfo2.flag   = MTD_OFFSET_USELESS;
                    
                        IDE_TEST_RAISE( sCalculateInfo->sSearchValue[sCount].column->module->
                                        logicalCompare[MTD_COMPARE_ASCENDING]
                                        ( &sValueInfo1,
                                          &sValueInfo2 )
                                        == 0,
                                        ERR_INVALID_FUNCTION_ARGUMENT );
                    }
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        sModules[0] = & mtdBigint;
        
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
        
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumListExecuteBigint;

        // sum 결과를 저장함
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdBigint,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
        
        // sum info는 필요없다.
        IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                         & mtdBinary,
                                         1,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    if ( sFence == 3 )
    {
        // decode_sum_list(i1, i2, (1,2,3))과 같이 세번째 인자가 상수인 경우
        if ( sListCount > 1 )
        {
            sIsConstValue = ID_TRUE;
            
            for ( sCount = 0, sNode = aNode->arguments->next->next->arguments;
                  ( sCount < sListCount ) && ( sNode != NULL );
                  sCount++, sNode = sNode->next )
            {
                if ( ( MTC_NODE_IS_DEFINED_VALUE( sNode ) == ID_TRUE )
                     &&
                     ( ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                         == MTC_TUPLE_TYPE_CONSTANT ) ||
                       ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                         == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
                {
                    // Nothing to do.
                }
                else
                {
                    sIsConstValue = ID_FALSE;
                    break;
                }
            }

            if ( sIsConstValue == ID_TRUE )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            }
            else
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
        }
        else
        {
            if ( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next->next ) == ID_TRUE )
                 &&
                 ( ( ( aTemplate->rows[aNode->arguments->next->next->table].lflag
                       & MTC_TUPLE_TYPE_MASK )
                     == MTC_TUPLE_TYPE_CONSTANT ) ||
                   ( ( aTemplate->rows[aNode->arguments->next->next->table].lflag
                       & MTC_TUPLE_TYPE_MASK )
                     == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            }
            else
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
        }
            
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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION( ERR_LIST_COUNT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeSumListEstimateBigint",
                              "invalid list count" ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeSumListInitializeBigint( mtcNode*     aNode,
                                         mtcStack*,
                                         SInt,
                                         void*,
                                         mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListInitializeBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtcNode               * sArgNode[2];
    mtdBinaryType         * sValue;
    mtfDecodeSumListInfo  * sInfo;
    mtcStack              * sTempStack;
    mtdBigintType         * sTempValue;
    SInt                    sCount;
    UInt                    sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_TEST_RAISE( sInfo == NULL, ERR_LIST_INFO );

    //-----------------------------
    // sum info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // sum column 설정
    sInfo->sSumColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sSumColumnNode    = sArgNode[0];

    if ( sFence == 3 )
    {
        sArgNode[1] = sArgNode[0]->next;

        // expression column 설정
        sInfo->sExprExecute = aTemplate->rows[sArgNode[1]->table].execute + sArgNode[1]->column;
        sInfo->sExprNode    = sArgNode[1];
    }
    else
    {
        // Nothing to do.
    }
    
    // return column 설정
    sInfo->sReturnColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sInfo->sReturnValue  = (void *)
        ((UChar*) aTemplate->rows[aNode->table].row + sInfo->sReturnColumn->column.offset);
    
    if ( sInfo->sReturnColumn->module != &mtdList )
    {
        sInfo->sReturnStack = NULL;
        sInfo->sReturnCount = 1;
    }
    else
    {
        sInfo->sReturnStack = (mtcStack*)sInfo->sReturnValue;
        sInfo->sReturnCount = sInfo->sReturnColumn->precision;
        
        // stack 초기화
        // (1) estimate때 생성한 column 정보로 초기화
        // (2) value를 실제 값으로 설정
        sTempStack = (mtcStack*) sInfo->sReturnColumn->column.value;
        sTempValue = (mtdBigintType*)
            ( (UChar*)sInfo->sReturnStack + ID_SIZEOF(mtcStack) * sInfo->sReturnCount );
        
        for ( sCount = 0;
              sCount < sInfo->sReturnCount;
              sCount++, sTempStack++, sTempValue++ )
        {
            sInfo->sReturnStack[sCount].column = sTempStack->column;
            sInfo->sReturnStack[sCount].value  = sTempValue;
        }
        
        IDE_DASSERT( (UChar*) sTempValue ==
                     (UChar*) sInfo->sReturnValue + sInfo->sReturnColumn->column.size );
    }
    
    //-----------------------------
    // sum 결과를 초기화
    //-----------------------------
    
    if ( sInfo->sReturnStack == NULL )
    {
        IDE_DASSERT( sInfo->sReturnColumn->module == &mtdBigint );
        
        *(mtdBigintType*)(sInfo->sReturnValue) = 0;
        sInfo->sIsNullValue[0] = ID_TRUE;
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            IDE_DASSERT( sInfo->sReturnStack[sCount].column->module == &mtdBigint );
            
            *(mtdBigintType*)(sInfo->sReturnStack[sCount].value) = 0;
            sInfo->sIsNullValue[sCount] = ID_TRUE;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LIST_INFO );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeSumListInitializeBigint",
                              "invalid list info" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeSumListAggregateBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListAggregateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule          * sModule;
    const mtcColumn          * sColumn;
    mtdBinaryType            * sValue;
    mtfDecodeSumListInfo     * sInfo;
    mtfDecodeSumSortedValue    sExprValue;
    mtfDecodeSumSortedValue  * sFound;
    UInt                       sFence;

    mtfDecodeSumListCalculateInfo * sCalculateInfo = (mtfDecodeSumListCalculateInfo*) aInfo;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    /* BUG-39635 Codesonar warning */
    sExprValue.column = NULL;
    sExprValue.value  = NULL;
    sExprValue.idx    = 0;

    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

        // 세번째 인자는 반드시 상수여야 한다.
        IDE_TEST_RAISE( sCalculateInfo == NULL, ERR_INVALID_FUNCTION_ARGUMENT );
        
        // 두번째 인자
        IDE_TEST( sInfo->sExprExecute->calculate( sInfo->sExprNode,
                                                  aStack,
                                                  aRemain,
                                                  sInfo->sExprExecute->calculateInfo,
                                                  aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sExprNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sExprNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // decode 연산수행
        sExprValue.column = aStack[0].column;
        sExprValue.value  = aStack[0].value;
        sExprValue.idx    = 0;
        
        sFound = (mtfDecodeSumSortedValue*)
            idlOS::bsearch( & sExprValue,
                            sCalculateInfo->sSearchValue,
                            sCalculateInfo->sSearchCount,
                            ID_SIZEOF(mtfDecodeSumSortedValue),
                            compareDecodeSumSortedValue );
    }
    else
    {
        // null만 아니면 됨
        sFound = & sExprValue;
    }

    if ( sFound != NULL )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sSumColumnExecute->calculate( sInfo->sSumColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sSumColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sSumColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSumColumnNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        IDE_DASSERT( aStack[0].column->module == & mtdBigint );

        sModule = aStack[0].column->module;

        if ( sModule->isNull( aStack[0].column,
                              aStack[0].value ) != ID_TRUE )
        {
            if ( sInfo->sReturnStack == NULL )
            {
                *(mtdBigintType*)(sInfo->sReturnValue) +=
                    *(mtdBigintType*) aStack[0].value;

                if ( sInfo->sIsNullValue[0] == ID_TRUE )
                {
                    sInfo->sIsNullValue[0] = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                IDE_DASSERT( sFound->idx < sInfo->sReturnCount );
                
                *(mtdBigintType*)(sInfo->sReturnStack[sFound->idx].value) +=
                    *(mtdBigintType*) aStack[0].value;
                
                if ( sInfo->sIsNullValue[sFound->idx] == ID_TRUE )
                {
                    sInfo->sIsNullValue[sFound->idx] = ID_FALSE;
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
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeSumListFinalizeBigint( mtcNode*     aNode,
                                       mtcStack*,
                                       SInt,
                                       void*,
                                       mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListFinalizeBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtdBinaryType         * sValue;
    mtfDecodeSumListInfo  * sInfo;
    SInt                    sCount;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumListInfo*)(sValue->mValue);

    if ( sInfo->sReturnStack == NULL )
    {
        if ( sInfo->sIsNullValue[0] == ID_TRUE )
        {
            mtdBigint.null( sInfo->sReturnColumn,
                            sInfo->sReturnValue );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            if ( sInfo->sIsNullValue[sCount] == ID_TRUE )
            {
                mtdBigint.null( sInfo->sReturnStack[sCount].column,
                                sInfo->sReturnStack[sCount].value );
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeSumListCalculateBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt,
                                        void*,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumListCalculateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    return IDE_SUCCESS;
    
#undef IDE_FN
}
