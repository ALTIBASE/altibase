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

extern mtfModule mtfDecodeMinList;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfDecodeMinListFunctionName[1] = {
    { NULL, 15, (void*)"DECODE_MIN_LIST" }
};

static IDE_RC mtfDecodeMinListEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

mtfModule mtfDecodeMinList = {
    2|MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDecodeMinListFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDecodeMinListEstimate
};

static IDE_RC mtfDecodeMinListEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

IDE_RC mtfDecodeMinListInitialize( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfDecodeMinListAggregate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfDecodeMinListFinalize( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

IDE_RC mtfDecodeMinListCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeMinListExecute = {
    mtfDecodeMinListInitialize,
    mtfDecodeMinListAggregate,
    mtf::calculateNA,
    mtfDecodeMinListFinalize,
    mtfDecodeMinListCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

typedef struct mtfDecodeMinSortedValue
{
    mtcColumn   * column;
    void        * value;
    SInt          idx;
} mtfDecodeMinSortedValue;

typedef struct mtfDecodeMinListCalculateInfo
{
    SInt                       sSearchCount;
    mtfDecodeMinSortedValue  * sSearchValue;  // sorted value

} mtfDecodeMinListCalculateInfo;

typedef struct mtfDecodeMinListInfo
{
    // 첫번째 인자
    mtcExecute     * sMinColumnExecute;
    mtcNode        * sMinColumnNode;

    // 두번째 인자
    mtcExecute     * sExprExecute;
    mtcNode        * sExprNode;

    // return 인자
    mtcColumn      * sReturnColumn;
    void           * sReturnValue;
    mtcStack       * sReturnStack;
    SInt             sReturnCount;

} mtfDecodeMinListInfo;

extern "C" SInt
compareDecodeMinSortedValue( const void* aElem1, const void* aElem2 )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    mtfDecodeMinSortedValue  * sElem1 = (mtfDecodeMinSortedValue*)aElem1;
    mtfDecodeMinSortedValue  * sElem2 = (mtfDecodeMinSortedValue*)aElem2;
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

IDE_RC mtfDecodeMinListEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt,
                                 mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeMinListEstimate"
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

    mtfDecodeMinListCalculateInfo * sCalculateInfo;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    /* BUG-44109 pivot 구문의 transform 함수인 list 용 decode 함수에서
       잘못된 인자 개수를 사용할 경우 비정상종료합니다.  */
    IDE_TEST_RAISE( ( sFence != 1 ) && ( sFence != 3 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // PROJ-2002 Column Security
    // min함수는 비교만을 수행하므로 min함수 자체는 복호화가
    // 필요하지 않다. 그러나 min함수가 복호화한 값을 리턴하기
    // 위해서는 마지막 min값에 대해 복호화를 수행할 수 도 있지만
    // 이 경우 암호 타입의 임시 변수를 저장할 공간이 필요하고
    // 또 min이 중첩되는 경우도 있으므로 min함수에 보안 타입이
    // 오는 경우 보안 타입으로 리턴한다. 단, 복호화를 위해
    // 인자의 source를 min함수의 source로 설정한다.
    //
    // ex) select _decrypt(min(i1)) from t1;
    //     select _decrypt(max(min(i2))) from t1 group by i1;
    aNode->baseTable = aNode->arguments->baseTable;
    aNode->baseColumn = aNode->arguments->baseColumn;

    IDE_TEST_RAISE( (aStack[1].column->module == &mtdList) ||
                    (aStack[1].column->module == &mtdBoolean),
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
                // decode_min_list( i1, i2, 1 )
        
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
        
                sModules[0] = aStack[1].column->module;
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
                // decode_min_list( i1, i2, (1,2,3,...) )
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
            
                sModules[0] = aStack[1].column->module;
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
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeMinListExecute;

                // Min 결과를 저장함
                // BUG-23102
                // mtcColumn으로 초기화한다.
                mtc::initializeColumn( aStack[0].column, aStack[1].column );
        
                // Min info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeMinListInfo);
                IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                                 & mtdBinary,
                                                 1,
                                                 sBinaryPrecision,
                                                 0 )
                          != IDE_SUCCESS );
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeMinListExecute;

                // Min 결과를 저장할 컬럼정보
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtcColumn),
                                            (void**)&sMtcColumn )
                          != IDE_SUCCESS );
        
                // Min 결과를 저장함
                // BUG-23102
                // mtcColumn으로 초기화한다.
                mtc::initializeColumn( sMtcColumn, aStack[1].column );
                
                // execution용 sListCount개의 stack과 value를 저장할 공간을 설정한다.
                // BUG-42973 module align을 고려하여 list value를 생성한다.
                IDE_TEST( mtc::initializeColumn(
                              aStack[0].column,
                              & mtdList,
                              2,
                              sListCount,
                              idlOS::align( sMtcColumn->column.size,
                                            sMtcColumn->module->align ) * sListCount )
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
        
                // Min info 정보를 mtdBinary에 저장
                sBinaryPrecision = ID_SIZEOF(mtfDecodeMinListInfo);
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
                // mtfDecodeMinListCalculateInfo 저장할 공간을 할당
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF(mtfDecodeMinListCalculateInfo),
                                            (void**) & sCalculateInfo )
                          != IDE_SUCCESS );
                
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = sCalculateInfo;
            
                if ( aStack[3].column->module != &mtdList )
                {
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeMinSortedValue),
                                                (void**) & sCalculateInfo->sSearchValue )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtcColumn),
                                                (void**) & sMtcColumn )
                              != IDE_SUCCESS );
                    
                    // decode_min_list( i1, i2, 1 )
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
                    // decode_min_list( i1, i2, (1,2,3,...) )
                    sCalculateInfo->sSearchCount = aStack[3].column->precision;
                
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfDecodeMinSortedValue) * sCalculateInfo->sSearchCount,
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
                                  ID_SIZEOF(mtfDecodeMinSortedValue),
                                  compareDecodeMinSortedValue );
                
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
        sModules[0] = aStack[1].column->module;
        
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
        
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeMinListExecute;

        // Min 결과를 저장함
        // BUG-23102
        // mtcColumn으로 초기화한다.
        mtc::initializeColumn( aStack[0].column, aStack[1].column );
        
        // Min info는 필요없다.
        IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                         & mtdBinary,
                                         1,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    if ( sFence == 3 )
    {
        // decode_min_list(i1, i2, (1,2,3))과 같이 세번째 인자가 상수인 경우
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
                              "mtfDecodeMinListEstimate",
                              "invalid list count" ) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeMinListInitialize( mtcNode*     aNode,
                                   mtcStack*,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeMinListInitialize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn       * sColumn;
    mtcNode               * sArgNode[2];
    mtdBinaryType         * sValue;
    mtfDecodeMinListInfo  * sInfo;
    mtcStack              * sTempStack;
    UChar                 * sTempValue;
    SInt                    sCount;
    UInt                    sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeMinListInfo*)(sValue->mValue);

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_TEST_RAISE( sInfo == NULL, ERR_LIST_INFO );

    //-----------------------------
    // Min info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // Min column 설정
    sInfo->sMinColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sMinColumnNode    = sArgNode[0];

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
            
            // BUG-42973 module align을 고려하여 list value를 할당한다.
            sTempValue += idlOS::align(
                sInfo->sReturnStack[sCount].column->column.size,
                sTempStack->column->module->align );
        }
        
        IDE_DASSERT( (UChar*) sTempValue ==
                     (UChar*) sInfo->sReturnValue + sInfo->sReturnColumn->column.size );
    }
    
    //-----------------------------
    // Min 결과를 초기화
    //-----------------------------
    
    if ( sInfo->sReturnStack == NULL )
    {
        if ( sInfo->sReturnColumn->module->isNull( sInfo->sReturnColumn,
                                                   sInfo->sReturnValue ) == ID_FALSE )
        {
            sInfo->sReturnColumn->module->null( sInfo->sReturnColumn,
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
            // 성능개선을 위해 group by의 mtrRow를 cralloc하고
            // null이 아닌경우만 수행한다.
            if ( sInfo->sReturnStack[sCount].column->module->isNull(
                     sInfo->sReturnStack[sCount].column,
                     sInfo->sReturnStack[sCount].value ) == ID_FALSE )
            {
                sInfo->sReturnStack[sCount].column->module->null(
                    sInfo->sReturnStack[sCount].column,
                    sInfo->sReturnStack[sCount].value );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LIST_INFO );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfDecodeMinListInitialize",
                              "invalid list info" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeMinListAggregate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeMinListAggregate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule          * sModule;
    const mtcColumn          * sColumn;
    mtdBinaryType            * sValue;
    mtfDecodeMinListInfo     * sInfo;
    mtfDecodeMinSortedValue    sExprValue;
    mtfDecodeMinSortedValue  * sFound;
    UInt                       sFence;
    mtdValueInfo               sValueInfo1;
    mtdValueInfo               sValueInfo2;

    mtfDecodeMinListCalculateInfo * sCalculateInfo = (mtfDecodeMinListCalculateInfo*) aInfo;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeMinListInfo*)(sValue->mValue);

    /* BUG-39635 Codesonar warning */
    sExprValue.column    = NULL;
    sExprValue.value     = NULL;
    sExprValue.idx       = 0;

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
        
        sFound = (mtfDecodeMinSortedValue*)
            idlOS::bsearch( & sExprValue,
                            sCalculateInfo->sSearchValue,
                            sCalculateInfo->sSearchCount,
                            ID_SIZEOF(mtfDecodeMinSortedValue),
                            compareDecodeMinSortedValue );
    }
    else
    {
        // null만 아니면 됨
        sFound = & sExprValue;
    }

    if ( sFound != NULL )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sMinColumnExecute->calculate( sInfo->sMinColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sMinColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sMinColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sMinColumnNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        sModule = aStack[0].column->module;

        if ( sInfo->sReturnStack == NULL )
        {
            sValueInfo1.column = sInfo->sReturnColumn;
            sValueInfo1.value  = sInfo->sReturnValue;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aStack[0].column;
            sValueInfo2.value  = aStack[0].value;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                 &sValueInfo2 ) > 0 )
            {
                idlOS::memcpy( sInfo->sReturnValue,
                               aStack[0].value,
                               sModule->actualSize( aStack[0].column,
                                                    aStack[0].value ) );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            IDE_DASSERT( sFound->idx < sInfo->sReturnCount );
            
            sValueInfo1.column = sInfo->sReturnStack[sFound->idx].column;
            sValueInfo1.value  = sInfo->sReturnStack[sFound->idx].value;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aStack[0].column;
            sValueInfo2.value  = aStack[0].value;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            
            if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                 &sValueInfo2 ) > 0 )
            {
                idlOS::memcpy( sInfo->sReturnStack[sFound->idx].value,
                               aStack[0].value,
                               sModule->actualSize( aStack[0].column,
                                                    aStack[0].value ) );
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

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfDecodeMinListFinalize( mtcNode*,
                                 mtcStack*,
                                 SInt,
                                 void*,
                                 mtcTemplate* )
{
#define IDE_FN "IDE_RC mtfDecodeMinListFinalize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeMinListCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeMinListCalculate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    return IDE_SUCCESS;
    
#undef IDE_FN
}
