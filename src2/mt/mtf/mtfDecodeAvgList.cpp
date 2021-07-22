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

extern mtfModule mtfDecodeAvgList;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfDecodeAvgListFunctionName[1] = {
    { NULL, 15, (void*)"DECODE_AVG_LIST" }
};

static IDE_RC mtfDecodeAvgListInitialize( void );

static IDE_RC mtfDecodeAvgListFinalize( void );

static IDE_RC mtfDecodeAvgListEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

mtfModule mtfDecodeAvgList = {
    2|MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDecodeAvgListFunctionName,
    NULL,
    mtfDecodeAvgListInitialize,
    mtfDecodeAvgListFinalize,
    mtfDecodeAvgListEstimate
};

static IDE_RC mtfDecodeAvgListEstimateFloat( mtcNode*     aNode,
                                             mtcTemplate* aTemplate,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             mtcCallBack* aCallBack );

IDE_RC mtfDecodeAvgListInitializeFloat( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListAggregateFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListFinalizeFloat( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListCalculateFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeAvgListExecuteFloat = {
    mtfDecodeAvgListInitializeFloat,
    mtfDecodeAvgListAggregateFloat,
    mtf::calculateNA,
    mtfDecodeAvgListFinalizeFloat,
    mtfDecodeAvgListCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfDecodeAvgListEstimateDouble( mtcNode*     aNode,
                                              mtcTemplate* aTemplate,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              mtcCallBack* aCallBack );

IDE_RC mtfDecodeAvgListInitializeDouble( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListAggregateDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListFinalizeDouble( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListCalculateDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeAvgListExecuteDouble = {
    mtfDecodeAvgListInitializeDouble,
    mtfDecodeAvgListAggregateDouble,
    mtf::calculateNA,
    mtfDecodeAvgListFinalizeDouble,
    mtfDecodeAvgListCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfDecodeAvgListEstimateBigint( mtcNode*     aNode,
                                              mtcTemplate* aTemplate,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              mtcCallBack* aCallBack );

IDE_RC mtfDecodeAvgListInitializeBigint( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListAggregateBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListFinalizeBigint( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgListCalculateBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeAvgListExecuteBigint = {
    mtfDecodeAvgListInitializeBigint,
    mtfDecodeAvgListAggregateBigint,
    mtf::calculateNA,
    mtfDecodeAvgListFinalizeBigint,
    mtfDecodeAvgListCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

typedef struct mtfDecodeAvgSortedValue
{
    mtcColumn   * column;
    void        * value;
    SInt          idx;
} mtfDecodeAvgSortedValue;

typedef struct mtfDecodeAvgListCalculateInfo
{
    SInt              sSearchCount;
    mtfDecodeAvgSortedValue  * sSearchValue;  // sorted value

} mtfDecodeAvgListCalculateInfo;

typedef struct mtfDecodeAvgInfo
{
    ULong            sSumCount;
    mtdBigintType    sBigintSum;

} mtfDecodeAvgInfo;

typedef struct mtfDecodeAvgListInfo
{
    // 첫번째 인자
    mtcExecute     * sAvgColumnExecute;
    mtcNode        * sAvgColumnNode;

    // 두번째 인자
    mtcExecute     * sExprExecute;
    mtcNode        * sExprNode;

    // return 인자
    mtcColumn      * sReturnColumn;
    void           * sReturnValue;
    mtcStack       * sReturnStack;
    SInt             sReturnCount;

    // 임시변수
    mtfDecodeAvgInfo sAvgInfo[1];  // MTC_NODE_ARGUMENT_COUNT_MAXIMUM까지 확장할 수 있음

} mtfDecodeAvgListInfo;

extern "C" SInt
compareDecodeAvgSortedValue( const void* aElem1, const void* aElem2 )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    mtfDecodeAvgSortedValue  * sElem1 = (mtfDecodeAvgSortedValue*)aElem1;
    mtfDecodeAvgSortedValue  * sElem2 = (mtfDecodeAvgSortedValue*)aElem2;
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

static mtfSubModule mtfDecodeAvgListEstimates[3] = {
    { mtfDecodeAvgListEstimates+1, mtfDecodeAvgListEstimateFloat  },
    { mtfDecodeAvgListEstimates+2, mtfDecodeAvgListEstimateDouble },
    { NULL,                        mtfDecodeAvgListEstimateBigint }
};

static mtfSubModule** mtfTable = NULL;

IDE_RC mtfDecodeAvgListInitialize( void )
{
    return mtf::initializeTemplate( &mtfTable, mtfDecodeAvgListEstimates, mtfXX );
}

IDE_RC mtfDecodeAvgListFinalize( void )
{
    return mtf::finalizeTemplate( &mtfTable );
}

IDE_RC mtfDecodeAvgListEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListEstimate"
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

IDE_RC mtfDecodeAvgListEstimateFloat( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt,
                                      mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListEstimateFloat"
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

    mtfDecodeAvgListCalculateInfo * sCalculateInfo;
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
                // decode_Avg_list( i1, i2, 1 )
        
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
                // decode_Avg_list( i1, i2, (1,2,3,...) )
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
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteFloat;

                // Avg 결과를 저장함
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdFloat,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
        
                // Avg info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgListInfo);
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteFloat;

                // Avg 결과를 저장할 컬럼정보
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcColumn),
                                            (void**)&sMtcColumn )
                          != IDE_SUCCESS );
                
                // Avg 결과를 저장함
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
                
                // Avg info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgListInfo) +
                    ID_SIZEOF(mtfDecodeAvgInfo) * ( sListCount - 1 );
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
                // mtfDecodeAvgListCalculateInfo 저장할 공간을 할당
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtfDecodeAvgListCalculateInfo),
                                            (void**) & sCalculateInfo )
                          != IDE_SUCCESS );
                
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = sCalculateInfo;
            
                if ( aStack[3].column->module != &mtdList )
                {
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeAvgSortedValue),
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn),
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    // decode_Avg_list( i1, i2, 1 )
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
                    // decode_Avg_list( i1, i2, (1,2,3,...) )
                    sCalculateInfo->sSearchCount = aStack[3].column->precision;
                
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeAvgSortedValue) * sCalculateInfo->sSearchCount,
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
                                  ID_SIZEOF(mtfDecodeAvgSortedValue),
                                  compareDecodeAvgSortedValue );
                
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
        
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteFloat;

        // Avg 결과를 저장함
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdFloat,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
        
        // Avg info는 필요없다.
        IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                         & mtdBinary,
                                         1,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    if ( sFence == 3 )
    {
        // decode_Avg_list(i1, i2, (1,2,3))과 같이 세번째 인자가 상수인 경우
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
                              "mtfDecodeAvgListEstimateFloat",
                              "invalid list count" ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgListInitializeFloat( mtcNode*     aNode,
                                        mtcStack*,
                                        SInt,
                                        void*,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListInitializeFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtcNode               * sArgNode[2];
    mtdBinaryType         * sValue;
    mtfDecodeAvgListInfo  * sInfo;
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
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_TEST_RAISE( sInfo == NULL, ERR_LIST_INFO );

    //-----------------------------
    // Avg info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // Avg column 설정
    sInfo->sAvgColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sAvgColumnNode    = sArgNode[0];

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
    // Avg 결과를 초기화
    //-----------------------------
    
    if ( sInfo->sReturnStack == NULL )
    {
        IDE_DASSERT( sInfo->sReturnColumn->module == &mtdFloat );
        
        sFloat = (mtdNumericType*)(sInfo->sReturnValue);
        sFloat->length       = 1;
        sFloat->signExponent = 0x80;
        
        sInfo->sAvgInfo[0].sSumCount = 0;
        sInfo->sAvgInfo[0].sBigintSum = 0;
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            IDE_DASSERT( sInfo->sReturnStack[sCount].column->module == &mtdFloat );
            
            sFloat = (mtdNumericType*)(sInfo->sReturnStack[sCount].value);
            sFloat->length       = 1;
            sFloat->signExponent = 0x80;
            
            sInfo->sAvgInfo[sCount].sSumCount = 0;
            sInfo->sAvgInfo[sCount].sBigintSum = 0;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LIST_INFO );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeAvgListInitializeFloat",
                              "invalid list info" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgListAggregateFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListAggregateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule          * sModule;
    const mtcColumn          * sColumn;
    mtdNumericType           * sFloatAvg;
    mtdNumericType           * sFloatArgument;
    UChar                      sFloatAvgBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType           * sFloatAvgClone;
    mtdBinaryType            * sValue;
    mtfDecodeAvgListInfo     * sInfo;
    mtfDecodeAvgSortedValue    sExprValue;
    mtfDecodeAvgSortedValue  * sFound;
    UInt                       sFence;

    mtfDecodeAvgListCalculateInfo * sCalculateInfo = (mtfDecodeAvgListCalculateInfo*) aInfo;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

    /* BUG-39635 Codesonar warning */
    sExprValue.column     = NULL;
    sExprValue.value      = NULL;
    sExprValue.idx        = 0;

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
        
        sFound = (mtfDecodeAvgSortedValue*)
            idlOS::bsearch( & sExprValue,
                            sCalculateInfo->sSearchValue,
                            sCalculateInfo->sSearchCount,
                            ID_SIZEOF(mtfDecodeAvgSortedValue),
                            compareDecodeAvgSortedValue );
    }
    else
    {
        // null만 아니면 됨
        sFound = & sExprValue;
    }

    if ( sFound != NULL )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sAvgColumnExecute->calculate( sInfo->sAvgColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sAvgColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sAvgColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sAvgColumnNode,
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
                sFloatAvg      = (mtdNumericType*)sInfo->sReturnValue;
                sFloatArgument = (mtdNumericType*)aStack[0].value;
                sFloatAvgClone = (mtdNumericType*)sFloatAvgBuff;
                idlOS::memcpy( sFloatAvgClone, sFloatAvg, sFloatAvg->length + 1 );
                
                IDE_TEST( mtc::addFloat( sFloatAvg,
                                         MTD_FLOAT_PRECISION_MAXIMUM,
                                         sFloatAvgClone,
                                         sFloatArgument )
                          != IDE_SUCCESS );

                sInfo->sAvgInfo[0].sSumCount++;
            }
            else
            {
                IDE_DASSERT( sFound->idx < sInfo->sReturnCount );

                sFloatAvg      = (mtdNumericType*)(sInfo->sReturnStack[sFound->idx].value);
                sFloatArgument = (mtdNumericType*)aStack[0].value;
                sFloatAvgClone = (mtdNumericType*)sFloatAvgBuff;
                idlOS::memcpy( sFloatAvgClone, sFloatAvg, sFloatAvg->length + 1 );
                
                IDE_TEST( mtc::addFloat( sFloatAvg,
                                         MTD_FLOAT_PRECISION_MAXIMUM,
                                         sFloatAvgClone,
                                         sFloatArgument )
                          != IDE_SUCCESS );
                
                sInfo->sAvgInfo[sFound->idx].sSumCount++;
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

IDE_RC mtfDecodeAvgListFinalizeFloat( mtcNode*     aNode,
                                      mtcStack*,
                                      SInt,
                                      void*,
                                      mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListFinalizeFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtdBinaryType         * sValue;
    mtfDecodeAvgListInfo  * sInfo;
    SInt                    sCount;
    mtdNumericType        * sFloatResult;
    UChar                   sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType        * sFloatSum;
    UChar                   sFloatCountBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType        * sFloatCount;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

    if ( sInfo->sReturnStack == NULL )
    {
        if ( sInfo->sAvgInfo[0].sSumCount == 0 )
        {
            mtdFloat.null( sInfo->sReturnColumn,
                           sInfo->sReturnValue );
        }
        else
        {
            // sum
            sFloatResult = (mtdNumericType*)sInfo->sReturnValue;
            sFloatSum = (mtdNumericType*)sFloatSumBuff;
            idlOS::memcpy( sFloatSum, sFloatResult, sFloatResult->length + 1 );

            // count
            sFloatCount = (mtdNumericType*)sFloatCountBuff;
            mtc::makeNumeric( sFloatCount, (SLong)sInfo->sAvgInfo[0].sSumCount );

            IDE_TEST( mtc::divideFloat( sFloatResult,
                                        MTD_FLOAT_PRECISION_MAXIMUM,
                                        sFloatSum,
                                        sFloatCount )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            if ( sInfo->sAvgInfo[sCount].sSumCount == 0 )
            {
                mtdFloat.null( sInfo->sReturnStack[sCount].column,
                               sInfo->sReturnStack[sCount].value );
            }
            else
            {
                // sum
                sFloatResult = (mtdNumericType*)(sInfo->sReturnStack[sCount].value);
                sFloatSum = (mtdNumericType*)sFloatSumBuff;
                idlOS::memcpy( sFloatSum, sFloatResult, sFloatResult->length + 1 );
                
                // count
                sFloatCount = (mtdNumericType*)sFloatCountBuff;
                mtc::makeNumeric( sFloatCount, (SLong)sInfo->sAvgInfo[sCount].sSumCount );
                
                IDE_TEST( mtc::divideFloat( sFloatResult,
                                            MTD_FLOAT_PRECISION_MAXIMUM,
                                            sFloatSum,
                                            sFloatCount )
                          != IDE_SUCCESS );
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeAvgListCalculateFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt,
                                       void*,
                                       mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListCalculateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;

#undef IDE_FN
}

/* ZONE: DOUBLE */

IDE_RC mtfDecodeAvgListEstimateDouble( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt,
                                       mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListEstimateDouble"
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

    mtfDecodeAvgListCalculateInfo * sCalculateInfo;
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
                // decode_Avg_list( i1, i2, 1 )
        
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
                // decode_Avg_list( i1, i2, (1,2,3,...) )
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
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteDouble;

                // Avg 결과를 저장함
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdDouble,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
        
                // Avg info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgListInfo);
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteDouble;

                // Avg 결과를 저장할 컬럼정보
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcColumn),
                                            (void**)&sMtcColumn )
                          != IDE_SUCCESS );
        
                // Avg 결과를 저장함
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
        
                // Avg info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgListInfo) +
                    ID_SIZEOF(mtfDecodeAvgInfo) * ( sListCount - 1 );
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
                // mtfDecodeAvgListCalculateInfo 저장할 공간을 할당
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtfDecodeAvgListCalculateInfo),
                                            (void**) & sCalculateInfo )
                          != IDE_SUCCESS );
                
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = sCalculateInfo;
            
                if ( aStack[3].column->module != &mtdList )
                {
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeAvgSortedValue),
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn),
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    // decode_Avg_list( i1, i2, 1 )
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
                    // decode_Avg_list( i1, i2, (1,2,3,...) )
                    sCalculateInfo->sSearchCount = aStack[3].column->precision;
                
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeAvgSortedValue) * sCalculateInfo->sSearchCount,
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
                                  ID_SIZEOF(mtfDecodeAvgSortedValue),
                                  compareDecodeAvgSortedValue );
                
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
        
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteDouble;

        // Avg 결과를 저장함
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdDouble,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
        
        // Avg info는 필요없다.
        IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                         & mtdBinary,
                                         1,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    if ( sFence == 3 )
    {
        // decode_Avg_list(i1, i2, (1,2,3))과 같이 세번째 인자가 상수인 경우
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
                              "mtfDecodeAvgListEstimateDouble",
                              "invalid list count" ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeAvgListInitializeDouble( mtcNode*     aNode,
                                         mtcStack*,
                                         SInt,
                                         void*,
                                         mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListInitializeDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtcNode               * sArgNode[2];
    mtdBinaryType         * sValue = NULL;
    mtfDecodeAvgListInfo  * sInfo  = NULL;
    mtcStack              * sTempStack = NULL;
    mtdDoubleType         * sTempValue = NULL;
    SInt                    sCount;
    UInt                    sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_TEST_RAISE( sInfo == NULL, ERR_LIST_INFO );

    //-----------------------------
    // Avg info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // Avg column 설정
    sInfo->sAvgColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sAvgColumnNode    = sArgNode[0];

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
    // Avg 결과를 초기화
    //-----------------------------
    
    if ( sInfo->sReturnStack == NULL )
    {
        IDE_DASSERT( sInfo->sReturnColumn->module == &mtdDouble );
        
        *(mtdDoubleType*)(sInfo->sReturnValue) = 0;
        
        sInfo->sAvgInfo[0].sSumCount = 0;
        sInfo->sAvgInfo[0].sBigintSum = 0;
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            IDE_DASSERT( sInfo->sReturnStack[sCount].column->module == &mtdDouble );
            
            *(mtdDoubleType*)(sInfo->sReturnStack[sCount].value) = 0;
            
            sInfo->sAvgInfo[sCount].sSumCount = 0;
            sInfo->sAvgInfo[sCount].sBigintSum = 0;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LIST_INFO );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeAvgListInitializeDouble",
                              "invalid list info" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;  

#undef IDE_FN
}

IDE_RC mtfDecodeAvgListAggregateDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListAggregateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule          * sModule;
    const mtcColumn          * sColumn;
    mtdBinaryType            * sValue;
    mtfDecodeAvgListInfo     * sInfo;
    mtfDecodeAvgSortedValue    sExprValue;
    mtfDecodeAvgSortedValue  * sFound;
    UInt                       sFence;

    mtfDecodeAvgListCalculateInfo * sCalculateInfo = (mtfDecodeAvgListCalculateInfo*) aInfo;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

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
        
        sFound = (mtfDecodeAvgSortedValue*)
            idlOS::bsearch( & sExprValue,
                            sCalculateInfo->sSearchValue,
                            sCalculateInfo->sSearchCount,
                            ID_SIZEOF(mtfDecodeAvgSortedValue),
                            compareDecodeAvgSortedValue );
    }
    else
    {
        // null만 아니면 됨
        sFound = & sExprValue;
    }

    if ( sFound != NULL )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sAvgColumnExecute->calculate( sInfo->sAvgColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sAvgColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sAvgColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sAvgColumnNode,
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

                sInfo->sAvgInfo[0].sSumCount++;
            }
            else
            {
                IDE_DASSERT( sFound->idx < sInfo->sReturnCount );
                
                *(mtdDoubleType*)(sInfo->sReturnStack[sFound->idx].value) +=
                    *(mtdDoubleType*) aStack[0].value;
                
                sInfo->sAvgInfo[sFound->idx].sSumCount++;
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

IDE_RC mtfDecodeAvgListFinalizeDouble( mtcNode*     aNode,
                                       mtcStack*,
                                       SInt,
                                       void*,
                                       mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListFinalizeDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtdBinaryType         * sValue;
    mtfDecodeAvgListInfo  * sInfo;
    SInt                    sCount;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

    if ( sInfo->sReturnStack == NULL )
    {
        if ( sInfo->sAvgInfo[0].sSumCount == 0 )
        {
            mtdDouble.null( sInfo->sReturnColumn,
                            sInfo->sReturnValue );
        }
        else
        {
            *(mtdDoubleType*)(sInfo->sReturnValue) /= sInfo->sAvgInfo[0].sSumCount;
        }
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            if ( sInfo->sAvgInfo[sCount].sSumCount == 0 )
            {
                mtdDouble.null( sInfo->sReturnStack[sCount].column,
                                sInfo->sReturnStack[sCount].value );
            }
            else
            {
                *(mtdDoubleType*)(sInfo->sReturnStack[sCount].value) /=
                    sInfo->sAvgInfo[sCount].sSumCount;
            }
        }
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgListCalculateDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt,
                                        void*,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListCalculateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    return IDE_SUCCESS;
#undef IDE_FN
}

/* ZONE: BIGINT */

IDE_RC mtfDecodeAvgListEstimateBigint( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt,
                                       mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListEstimateBigint"
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

    mtfDecodeAvgListCalculateInfo * sCalculateInfo;
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
                // decode_Avg_list( i1, i2, 1 )
        
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
                // decode_Avg_list( i1, i2, (1,2,3,...) )
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
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteBigint;

                // Avg 결과를 저장함
                // bigint avg의 결과는 double이 아니라 float type임
                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 & mtdFloat,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );
        
                // Avg info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgListInfo);
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteBigint;

                // Avg 결과를 저장할 컬럼정보
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcColumn),
                                            (void**)&sMtcColumn )
                          != IDE_SUCCESS );
        
                // Avg 결과를 저장함
                // bigint avg의 결과는 double이 아니라 float type임
                IDE_TEST( mtc::initializeColumn( sMtcColumn,
                                                 & mtdFloat,
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
        
                // Avg info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgListInfo) +
                    ID_SIZEOF(mtfDecodeAvgInfo) * ( sListCount - 1 );
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
                // mtfDecodeAvgListCalculateInfo 저장할 공간을 할당
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtfDecodeAvgListCalculateInfo),
                                            (void**) & sCalculateInfo )
                          != IDE_SUCCESS );
                
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = sCalculateInfo;
            
                if ( aStack[3].column->module != &mtdList )
                {
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeAvgSortedValue),
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn),
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    // decode_Avg_list( i1, i2, 1 )
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
                    // decode_Avg_list( i1, i2, (1,2,3,...) )
                    sCalculateInfo->sSearchCount = aStack[3].column->precision;
                
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeAvgSortedValue) * sCalculateInfo->sSearchCount,
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
                                  ID_SIZEOF(mtfDecodeAvgSortedValue),
                                  compareDecodeAvgSortedValue );
                
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
        
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgListExecuteBigint;

        // Avg 결과를 저장함
        // bigint avg의 결과는 double이 아니라 float type임
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdFloat,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
        
        // Avg info는 필요없다.
        IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                         & mtdBinary,
                                         1,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    if ( sFence == 3 )
    {
        // decode_Avg_list(i1, i2, (1,2,3))과 같이 세번째 인자가 상수인 경우
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
                              "mtfDecodeAvgListEstimateBigint",
                              "invalid list count" ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeAvgListInitializeBigint( mtcNode*     aNode,
                                         mtcStack*,
                                         SInt,
                                         void*,
                                         mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListInitializeBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtcNode               * sArgNode[2];
    mtdBinaryType         * sValue;
    mtfDecodeAvgListInfo  * sInfo;
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
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_TEST_RAISE( sInfo == NULL, ERR_LIST_INFO );

    //-----------------------------
    // Avg info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // Avg column 설정
    sInfo->sAvgColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sAvgColumnNode    = sArgNode[0];

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
        
        IDE_DASSERT( (UChar*) sTempValue ==
                     (UChar*) sInfo->sReturnValue + sInfo->sReturnColumn->column.size );
    }
    
    //-----------------------------
    // Avg 결과를 초기화
    //-----------------------------
    
    if ( sInfo->sReturnStack == NULL )
    {
        IDE_DASSERT( sInfo->sReturnColumn->module == &mtdFloat );
        
        sFloat = (mtdNumericType*)(sInfo->sReturnValue);
        sFloat->length       = 1;
        sFloat->signExponent = 0x80;
        
        sInfo->sAvgInfo[0].sSumCount = 0;
        sInfo->sAvgInfo[0].sBigintSum = 0;
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            IDE_DASSERT( sInfo->sReturnStack[sCount].column->module == &mtdFloat );
            
            sFloat = (mtdNumericType*)(sInfo->sReturnStack[sCount].value);
            sFloat->length       = 1;
            sFloat->signExponent = 0x80;
            
            sInfo->sAvgInfo[sCount].sSumCount = 0;
            sInfo->sAvgInfo[sCount].sBigintSum = 0;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LIST_INFO );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeAvgListInitializeBigint",
                              "invalid list info" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgListAggregateBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListAggregateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule          * sModule;
    const mtcColumn          * sColumn;
    mtdBinaryType            * sValue;
    mtfDecodeAvgListInfo     * sInfo;
    mtfDecodeAvgSortedValue    sExprValue;
    mtfDecodeAvgSortedValue  * sFound;
    UInt                       sFence;

    mtfDecodeAvgListCalculateInfo * sCalculateInfo = (mtfDecodeAvgListCalculateInfo*) aInfo;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

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
        
        sFound = (mtfDecodeAvgSortedValue*)
            idlOS::bsearch( & sExprValue,
                            sCalculateInfo->sSearchValue,
                            sCalculateInfo->sSearchCount,
                            ID_SIZEOF(mtfDecodeAvgSortedValue),
                            compareDecodeAvgSortedValue );
    }
    else
    {
        // null만 아니면 됨
        sFound = & sExprValue;
    }

    if ( sFound != NULL )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sAvgColumnExecute->calculate( sInfo->sAvgColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sAvgColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sAvgColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sAvgColumnNode,
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
                sInfo->sAvgInfo[0].sBigintSum +=
                    *(mtdBigintType*) aStack[0].value;

                sInfo->sAvgInfo[0].sSumCount++;
            }
            else
            {
                IDE_DASSERT( sFound->idx < sInfo->sReturnCount );

                sInfo->sAvgInfo[sFound->idx].sBigintSum +=
                    *(mtdBigintType*) aStack[0].value;
                
                sInfo->sAvgInfo[sFound->idx].sSumCount++;
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

IDE_RC mtfDecodeAvgListFinalizeBigint( mtcNode*     aNode,
                                       mtcStack*,
                                       SInt,
                                       void*,
                                       mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListFinalizeBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtdBinaryType         * sValue;
    mtfDecodeAvgListInfo  * sInfo;
    SInt                    sCount;
    mtdNumericType        * sFloatSum;
    mtdNumericType        * sFloatCount;
    UChar                   sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                   sFloatCountBuff[MTD_FLOAT_SIZE_MAXIMUM];

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgListInfo*)(sValue->mValue);

    if ( sInfo->sReturnStack == NULL )
    {
        if ( sInfo->sAvgInfo[0].sSumCount == 0 )
        {
            mtdFloat.null( sInfo->sReturnColumn,
                           sInfo->sReturnValue );
        }
        else
        {
            sFloatSum = (mtdNumericType*)sFloatSumBuff;
            mtc::makeNumeric( sFloatSum, (SLong)(sInfo->sAvgInfo[0].sBigintSum) );
            sFloatCount = (mtdNumericType*)sFloatCountBuff;
            mtc::makeNumeric( sFloatCount, (SLong)(sInfo->sAvgInfo[0].sSumCount) );

            IDE_TEST( mtc::divideFloat( (mtdNumericType*)(sInfo->sReturnValue),
                                        MTD_FLOAT_PRECISION_MAXIMUM,
                                        sFloatSum,
                                        sFloatCount )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        for ( sCount = 0; sCount < sInfo->sReturnCount; sCount++ )
        {
            if ( sInfo->sAvgInfo[sCount].sSumCount == 0 )
            {
                mtdFloat.null( sInfo->sReturnStack[sCount].column,
                               sInfo->sReturnStack[sCount].value );
            }
            else
            {
                sFloatSum = (mtdNumericType*)sFloatSumBuff;
                mtc::makeNumeric( sFloatSum, (SLong)(sInfo->sAvgInfo[sCount].sBigintSum) );
                sFloatCount = (mtdNumericType*)sFloatCountBuff;
                mtc::makeNumeric( sFloatCount, (SLong)sInfo->sAvgInfo[sCount].sSumCount );
                
                IDE_TEST( mtc::divideFloat( (mtdNumericType*)(sInfo->sReturnStack[sCount].value),
                                            MTD_FLOAT_PRECISION_MAXIMUM,
                                            sFloatSum,
                                            sFloatCount )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeAvgListCalculateBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt,
                                        void*,
                                        mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgListCalculateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    return IDE_SUCCESS;
    
#undef IDE_FN
}
