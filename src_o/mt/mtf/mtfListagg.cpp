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
 * $Id: mtfListagg.cpp 85458 2019-05-16 07:13:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfListagg;

extern mtdModule mtdBoolean;
extern mtdModule mtdVarchar;
extern mtdModule mtdBinary;

#define MTF_LISTAGG_COLUMN_MAX     (32)

typedef struct mtfListaggFuncData
{
    SLong               rowSize;       // value column + key column의 rowSize.
    UInt                resultLength;  // max check를 위해.

    // key column
    mtcColumn         * keyColumn[MTF_LISTAGG_COLUMN_MAX];    // key column.
    UInt                keyIndex[MTF_LISTAGG_COLUMN_MAX];     // stack 위치.
    UInt                keyOffset[MTF_LISTAGG_COLUMN_MAX];    // key column의 offset.
    idBool              keyOrderDesc[MTF_LISTAGG_COLUMN_MAX]; // key column direction.
    UInt                keyCount;                             // key column 개수.

    // value column
    mtcColumn         * valueColumn;
    UInt                valueIndex;
    UInt                valueOffset;

    // finialize의 sorting 
    SChar            ** index;  // data ( value ) 의 pointer.
    SChar             * data;   // value.
    UInt                idx;    // value index.
    
} mtfListaggFuncData;

typedef struct mtfListaggSortStack
{
    SLong low;    // QuickSort Partition의 Low 값
    SLong high;   // QuickSort Partition의 High 값
} mtfListaggSortStack;

#define MTF_LISTAGG_SWAP( elem1, elem2 )  \
    { sTemp = elem1; elem1 = elem2; elem2 = sTemp; }

#define MTF_LISTAGG_GET( aListaggFuncData, aIndex, aElement )  \
    *(aElement) = (void**)((aListaggFuncData)->index + (aIndex));
    
static IDE_RC makeListaggFuncData( mtcTemplate           * aTemplate,
                                   mtfFuncDataBasicInfo  * aFuncDataInfo,
                                   mtcNode               * aNode,
                                   mtfListaggFuncData   ** aListaggFuncData );

static IDE_RC allocListaggFuncDataBuffer( mtcTemplate           * aTemplate,
                                          mtfFuncDataBasicInfo  * aFuncDataInfo,
                                          mtcStack              * aStack,
                                          mtfListaggFuncData    * aListaggFuncData );

static IDE_RC copyToListaggFuncDataBuffer( mtfFuncDataBasicInfo * aFuncDataInfo,
                                           mtcStack             * aStack,
                                           mtfListaggFuncData   * aListaggFuncData );

static IDE_RC sortListaggFuncDataBuffer( mtcTemplate          * aTemplate,
                                         mtfFuncDataBasicInfo * aFuncDataInfo,
                                         mtfListaggFuncData   * aListaggFuncData );

static IDE_RC partitionListaggFuncDataBuffer( mtcTemplate         * aTemplate,
                                              mtfListaggFuncData  * aListaggFuncData,
                                              SLong                 aLow,
                                              SLong                 aHigh,
                                              SLong               * aPartition,
                                              idBool              * aAllEqual );

static SInt compareListaggFuncDataBuffer( mtcTemplate         * aTemplate,
                                          mtfListaggFuncData  * aListaggFuncData,
                                          const void          * aElem1,
                                          const void          * aElem2 );

static mtcName mtfListaggFunctionName[1] = {
    { NULL, 7, (void*)"LISTAGG" }
};

static IDE_RC mtfListaggEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

mtfModule mtfListagg = {
    3 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfListaggFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfListaggEstimate
};

IDE_RC mtfListaggInitialize( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

IDE_RC mtfListaggAggregate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

IDE_RC mtfListaggFinalize( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

IDE_RC mtfListaggCalculate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfListaggInitialize,
    mtfListaggAggregate,
    mtf::calculateNA,
    mtfListaggFinalize,
    mtfListaggCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfListaggEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          /* aRemain */,
                           mtcCallBack * aCallBack )
{
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcNode         * sNode;
    mtcNode         * sKeyNode;
    const mtdModule * sModule;
    SInt              sPrecision;
    UInt              sCount;
    UInt              i = 0;
    
    // within group by가 있어야 함
    IDE_TEST_RAISE( aNode->funcArguments == NULL,
                    ERR_WITHIN_GORUP_MISSING_WITHIN_GROUP );

    sCount = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );
    
    // function arguments count = count - key count
    for ( sNode = aNode->funcArguments; sNode != NULL; sNode = sNode->next )
    {
        sCount--;
    }

    IDE_TEST_RAISE( ( sCount != 1 ) && ( sCount != 2 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // 첫번째 인자
    IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[i],
                                                aStack[i + 1].column->module )
              != IDE_SUCCESS );
    i++;

    if ( sCount == 2 )
    {
        // 두번째 인자 (seperator)
        IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[i],
                                                    aStack[i + 1].column->module )
                  != IDE_SUCCESS );
        i++;

        sKeyNode = aNode->arguments->next->next;
    }
    else
    {
        sKeyNode = aNode->arguments->next;
    }

    for ( sNode = sKeyNode; sNode != NULL; sNode = sNode->next, i++ )
    {
        IDE_TEST_RAISE( mtf::isGreaterLessValidType( aStack[i + 1].column->module ) != ID_TRUE,
                        ERR_CONVERSION_NOT_APPLICABLE );

        sModules[i] = aStack[i + 1].column->module;
    }
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     4000,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF(mtfListaggFuncData*),
                                     0 )
              != IDE_SUCCESS );

    if ( sCount == 2 )
    {
        sModule = aStack[2].column->module;
        sPrecision = aStack[2].column->precision;
    }
    else
    {
        sModule = &mtdVarchar;
        sPrecision = 0;
    }
    
    // listagg option
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     sModule,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    // funcData 사용
    aNode->info = aTemplate->funcDataCnt;
    aTemplate->funcDataCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {   
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION( ERR_WITHIN_GORUP_MISSING_WITHIN_GROUP )
    {   
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MISSING_WITHIN_GROUP ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfListaggInitialize( mtcNode     * aNode,
                             mtcStack    * ,
                             SInt          ,
                             void        * ,
                             mtcTemplate * aTemplate )
{
    iduMemory             * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo  * sFuncData;
    mtfListaggFuncData    * sListaggFuncData;
    const mtcColumn       * sColumn;
    mtdBinaryType         * sBinary;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset);
    
    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );

        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfListaggInitialize::alloc::sFuncData",
                             ERR_MEMORY_ALLOCATION );
        IDE_TEST_RAISE( sMemoryMgr->alloc( ID_SIZEOF(mtfFuncDataBasicInfo),
                                           (void**)&sFuncData )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
        
        // function data init
        IDE_TEST( mtf::initializeFuncDataBasicInfo( sFuncData,
                                                    sMemoryMgr )
                  != IDE_SUCCESS );

        // 등록
        aTemplate->funcData[aNode->info] = sFuncData;

        /* BUG-46938 */
        sMemoryMgr = sFuncData->memoryMgr;

        /* BUG-46906 */
        IDE_TEST( sMemoryMgr->getStatus( &( sFuncData->mMemStatus ) )
                  != IDE_SUCCESS);
    }
    else
    {
        sFuncData = aTemplate->funcData[aNode->info];

        /* BUG-46938 */
        sMemoryMgr = sFuncData->memoryMgr;

        /* BUG-46906 */
        IDE_TEST( sMemoryMgr->setStatus( &( sFuncData->mMemStatus ) )
                  != IDE_SUCCESS);
    }

    // make listagg function data
    IDE_TEST( makeListaggFuncData( aTemplate,
                                   sFuncData,
                                   aNode,
                                   & sListaggFuncData )
              != IDE_SUCCESS );

    // set listagg function data
    sBinary->mLength = ID_SIZEOF(mtfListaggFuncData*);
    *((mtfListaggFuncData**)sBinary->mValue) = sListaggFuncData;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sMemoryMgr != NULL )
    {
        mtf::freeFuncDataMemory( sMemoryMgr );

        aTemplate->funcData[aNode->info] = NULL;
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_FAILURE;
}

IDE_RC mtfListaggAggregate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * ,
                            mtcTemplate * aTemplate )
{
    mtfFuncDataBasicInfo  * sFuncData;
    mtfListaggFuncData    * sListaggFuncData;
    const mtcColumn       * sColumn;
    mtdBinaryType         * sBinary;
    mtdCharType           * sSeperator;
    
    IDE_TEST_RAISE( aRemain < 3, ERR_STACK_OVERFLOW );
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                                   sColumn[1].column.offset);
        
        sFuncData = aTemplate->funcData[aNode->info];
        sListaggFuncData = *((mtfListaggFuncData**)sBinary->mValue);
        
        // 첫번째 인자 length concatenation check
        sListaggFuncData->resultLength +=
            ((mtdCharType*)aStack[1].value)->length;

        if ( sColumn[2].precision > 0 )
        {
            sSeperator = (mtdCharType*)((UChar*)aTemplate->rows[aNode->table].row +
                                        sColumn[2].column.offset);
            
            // listagg option
            if ( sListaggFuncData->idx == 0 )
            {
                idlOS::memcpy( sSeperator,
                               aStack[2].value,
                               aStack[2].column->module->actualSize( aStack[2].column,
                                                                     aStack[2].value ) );
            }
            else
            {
                // Nothing To Do
            }
            
            // 두번째 인자 length concatenation check
            sListaggFuncData->resultLength += sSeperator->length;
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST_RAISE( aStack[0].column->precision <
                        (SInt)sListaggFuncData->resultLength,
                        ERR_TOO_LONG_CONCATENATION );

        // alloc data
        if ( sListaggFuncData->idx == 0 )
        {
            IDE_TEST( allocListaggFuncDataBuffer( aTemplate,
                                                  sFuncData,
                                                  aStack,
                                                  sListaggFuncData )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        // copy to data
        IDE_TEST( copyToListaggFuncDataBuffer( sFuncData,
                                               aStack,
                                               sListaggFuncData )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ) );
    }
    IDE_EXCEPTION( ERR_TOO_LONG_CONCATENATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_TOO_LONG_CONCATENATION ));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfListaggFinalize( mtcNode     * aNode,
                           mtcStack    * ,
                           SInt          ,
                           void        * ,
                           mtcTemplate * aTemplate )
{
    mtdCharType           * sResult;
    mtdCharType           * sFirstString;
    mtdCharType           * sSecondString;
    mtfFuncDataBasicInfo  * sFuncDataBasicInfo;
    mtfListaggFuncData    * sListaggFuncData;
    UInt                    sCopySize;
    const mtcColumn       * sColumn;
    mtdBinaryType         * sBinary;
    UInt                    i;

    // function data, list agg function data  얻어온다.
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset);
    
    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sListaggFuncData = *((mtfListaggFuncData**)sBinary->mValue);
    sResult = (mtdCharType*)((UChar *)aTemplate->rows[aNode->table].row +
                             sColumn[0].column.offset);

    // GROUP_SORT 는 두개의 tuple을 번갈아 사용하므로 첫번째 결과 값이 존재
    // 하여 초기화 해야 한다.
    sResult->length = 0;

    if ( sListaggFuncData->idx > 0 )
    {
        // sort
        IDE_TEST( sortListaggFuncDataBuffer( aTemplate,
                                             sFuncDataBasicInfo,
                                             sListaggFuncData )
                  != IDE_SUCCESS );

        // 연산 수행
        for ( i = 0; i < sListaggFuncData->idx; i++ )
        {
            sFirstString = (mtdCharType*)
                (sListaggFuncData->index[i] + sListaggFuncData->valueOffset);
            
            IDE_DASSERT( sFirstString->length > 0 );
            
            if ( ( i > 0 ) && ( sColumn[2].precision > 0 ) )
            {
                // listagg option
                sSecondString = (mtdCharType*)
                    ( (UChar *)aTemplate->rows[aNode->table].row +
                      sColumn[2].column.offset );
                
                IDE_TEST( mtf::copyString(
                              sResult->value + sResult->length,
                              sColumn->precision - sResult->length,
                              & sCopySize,
                              sSecondString->value,
                              sSecondString->length,
                              sColumn->language )
                          != IDE_SUCCESS );
                        
                sResult->length += sCopySize;
            }
            else
            {
                // Nothing to do.
            }
            
            IDE_TEST( mtf::copyString(
                          sResult->value + sResult->length,
                          sColumn->precision - sResult->length,
                          & sCopySize,
                          sFirstString->value,
                          sFirstString->length,
                          sColumn->language )
                      != IDE_SUCCESS );
                            
            sResult->length += sCopySize;
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfListaggCalculate( mtcNode     * aNode,
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

IDE_RC makeListaggFuncData( mtcTemplate           * aTemplate,
                            mtfFuncDataBasicInfo  * aFuncDataInfo,
                            mtcNode               * aNode,
                            mtfListaggFuncData   ** aListaggFuncData )
{
 /***********************************************************************
 *
 * Description : 
 *      keyIndex, keyOrderDesc 정보 설정.
 *
 * Implementation :
 *
 *    select listagg(i1,'/') within group ( order by i2, i3 ) from t1;
 *    
 *    stack[0] = return value
 *    stack[1] = i1
 *    stack[2] = '/'
 *    stack[3] = i2
 *    stack[4] = i3
 *
 *    sIndex = 3 key column의 시작 stack 위치를 나타낸다.
 *    스택의 위치를 keyIndex에 저장 한다.
 *    keyIndex[0] = 3
 *    keyIndex[1] = 4
 ***********************************************************************/

    mtfListaggFuncData  * sListaggFuncData;
    mtcNode             * sNode;
    UInt                  sKeyCount = 0;
    UInt                  sIndex;  // stack index

    /* BUG-46892 */
    IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                               aFuncDataInfo->memoryMgr,
                                               ID_SIZEOF( mtfListaggFuncData ) )
              != IDE_SUCCESS );

    // listagg function date alloc
    IDU_FIT_POINT_RAISE( "makeListaggFuncData::cralloc::sListaggFuncData",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->cralloc(
                        ID_SIZEOF(mtfListaggFuncData),
                        (void**)&sListaggFuncData )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    // init
    sIndex = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) + 1;
    
    // order by key들은 함수의 인자 뒤에 연결되어 있다.
    for ( sNode = aNode->funcArguments;
          sNode != NULL;
          sNode = sNode->next, sIndex-- );
    
    for ( sNode = aNode->funcArguments;
          sNode != NULL;
          sNode = sNode->next, sIndex++ )
    {
        if ( ( sNode->lflag & MTC_NODE_WITHIN_GROUP_ORDER_MASK )
             == MTC_NODE_WITHIN_GROUP_ORDER_DESC )
        {
            sListaggFuncData->keyOrderDesc[sKeyCount] = ID_TRUE;
        }
        else
        {
            sListaggFuncData->keyOrderDesc[sKeyCount] = ID_FALSE;
        }
        
        sListaggFuncData->keyIndex[sKeyCount] = sIndex;
        sKeyCount++;
        
        IDE_TEST_RAISE( sKeyCount >= MTF_LISTAGG_COLUMN_MAX,
                        ERR_WITHIN_GORUP_COLUMN_OVERFLOW );
    }

    IDE_TEST_RAISE( sKeyCount == 0, ERR_INVALID_FUNCTION_ARGUMENT );

    sListaggFuncData->keyCount = sKeyCount;

    // listagg value
    sListaggFuncData->valueIndex = 1;
    
    // return
    *aListaggFuncData = sListaggFuncData;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_WITHIN_GORUP_COLUMN_OVERFLOW )
    {   
        IDE_SET( ideSetErrorCode( mtERR_ABORT_WITHIN_GORUP_COLUMN_OVERFLOW ));
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC allocListaggFuncDataBuffer( mtcTemplate           * aTemplate,
                                   mtfFuncDataBasicInfo  * aFuncDataInfo,
                                   mtcStack              * aStack,
                                   mtfListaggFuncData    * aListaggFuncData )
{    
    mtfFuncDataBasicInfo  * sFuncData = aFuncDataInfo;
    UInt                    sRowSize = 0;
    UInt                    i;

    //---------------------------------------
    // offset, row size 계산
    //---------------------------------------

    // key
    for ( i = 0; i < aListaggFuncData->keyCount; i++ )
    {
        aListaggFuncData->keyColumn[i] = aStack[aListaggFuncData->keyIndex[i]].column;
        sRowSize = idlOS::align( sRowSize,
                                 aListaggFuncData->keyColumn[i]->module->align );
        aListaggFuncData->keyOffset[i] = sRowSize;
            
        sRowSize += aListaggFuncData->keyColumn[i]->column.size;
    }

    // value
    aListaggFuncData->valueColumn = aStack[aListaggFuncData->valueIndex].column;
    sRowSize = idlOS::align( sRowSize,
                             aListaggFuncData->valueColumn->module->align );
    aListaggFuncData->valueOffset = sRowSize;
            
    sRowSize += aListaggFuncData->valueColumn->column.size;

    // rowSize
    aListaggFuncData->rowSize = idlOS::align8( sRowSize );

    //---------------------------------------
    // alloc
    //---------------------------------------

    /* BUG-46892 */
    IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                               aFuncDataInfo->memoryMgr,
                                               ( ID_SIZEOF(void*) * 4000 ) )
              != IDE_SUCCESS );

    // null을 제외하고 최소 1byte를 가정하면 최대 4000개를 저장할 수 있다.
    IDU_FIT_POINT_RAISE( "allocListaggFuncDataBuffer::alloc::index",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( sFuncData->memoryMgr->alloc(
                        ID_SIZEOF(void*) * 4000,
                        (void**)&(aListaggFuncData->index) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    /* BUG-46892 */
    IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                               aFuncDataInfo->memoryMgr,
                                               ( aListaggFuncData->rowSize * 4000 ) )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "allocListaggFuncDataBuffer::alloc::data",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( sFuncData->memoryMgr->alloc(
                        aListaggFuncData->rowSize * 4000,
                        (void**)&(aListaggFuncData->data) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC copyToListaggFuncDataBuffer( mtfFuncDataBasicInfo * /*aFuncDataInfo*/,
                                    mtcStack             * aStack,
                                    mtfListaggFuncData   * aListaggFuncData )
{
    mtcStack  * sStack;
    SChar     * sValue;
    UInt        i;

    IDE_TEST_RAISE( aListaggFuncData->idx >= 4000, ERR_BUFFER_OVERFLOW );
    
    sValue = aListaggFuncData->data + aListaggFuncData->rowSize * aListaggFuncData->idx;
    
    aListaggFuncData->index[aListaggFuncData->idx] = sValue;
    
    for ( i = 0; i < aListaggFuncData->keyCount; i++ )
    {
        sStack = aStack + aListaggFuncData->keyIndex[i];
        
        idlOS::memcpy( sValue + aListaggFuncData->keyOffset[i],
                       sStack->value,
                       sStack->column->module->actualSize( sStack->column,
                                                           sStack->value ) );
    }
    
    sStack = aStack + aListaggFuncData->valueIndex;
    
    idlOS::memcpy( sValue + aListaggFuncData->valueOffset,
                   sStack->value,
                   sStack->column->module->actualSize( sStack->column,
                                                       sStack->value  ) );

    aListaggFuncData->idx++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BUFFER_OVERFLOW )
    {       
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "copyToListaggFuncDataBuffer",
                                  "buffer overflow" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sortListaggFuncDataBuffer( mtcTemplate          * aTemplate,
                                  mtfFuncDataBasicInfo * aFuncDataInfo,
                                  mtfListaggFuncData   * aListaggFuncData )
{
 /***********************************************************************
 *
 * Description : 
 *    지정된 low와 high간의 Data들을 Quick Sorting한다.
 *
 * Implementation :
 *
 *    Stack Overflow가 발생하지 않도록 Stack공간을 마련한다.
 *    Partition을 분할하여 해당 Partition 단위로 Recursive하게
 *    Quick Sorting한다.
 *    자세한 내용은 일반적인 quick sorting algorithm을 참고.
 *
 ***********************************************************************/

    SLong                  sLow;
    SLong                  sHigh;
    SLong                  sPartition;
    mtfListaggSortStack  * sSortStack = NULL;
    mtfListaggSortStack  * sStack = NULL;
    idBool                 sAllEqual;

    /* BUG-46892 */
    IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                               aFuncDataInfo->memoryMgr,
                                               ID_SIZEOF(mtfListaggSortStack) * aListaggFuncData->idx )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "sortListaggFuncDataBuffer::alloc::sSortStack",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->alloc(
                        ID_SIZEOF(mtfListaggSortStack) * aListaggFuncData->idx,
                        (void**)&sSortStack )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    sStack = sSortStack;
    sStack->low = 0;
    sStack->high = aListaggFuncData->idx - 1;
    sStack++;

    sLow = 0;
    sHigh = aListaggFuncData->idx - 1;

    while ( sStack != sSortStack )
    {
        while ( sLow < sHigh )
        {
            sAllEqual = ID_FALSE;
            IDE_TEST( partitionListaggFuncDataBuffer( aTemplate,
                                                      aListaggFuncData,
                                                      sLow,
                                                      sHigh,
                                                      & sPartition,
                                                      & sAllEqual )
                      != IDE_SUCCESS );

            if ( sAllEqual == ID_TRUE )
            {
                break;
            }
            else
            {
                sStack->low = sPartition;
                sStack->high = sHigh;
                sStack++;
                sHigh = sPartition - 1;
            }
        }

        sStack--;
        sLow = sStack->low + 1;
        sHigh = sStack->high;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC partitionListaggFuncDataBuffer( mtcTemplate         * aTemplate,
                                       mtfListaggFuncData  * aListaggFuncData,
                                       SLong                 aLow,
                                       SLong                 aHigh,
                                       SLong               * aPartition,
                                       idBool              * aAllEqual )
{
    void** sPivotRow;
    void** sLowRow;
    void** sHighRow;
    void*  sTemp;

    SInt   sOrder = 0;
    SLong  sLow = aLow;
    SLong  sHigh = aHigh + 1;
    idBool sAllEqual = ID_TRUE;

    if ( aLow < aHigh )
    {
        MTF_LISTAGG_GET( aListaggFuncData, (aLow+aHigh)/2, &sLowRow );
        MTF_LISTAGG_GET( aListaggFuncData, aLow, &sPivotRow );
        MTF_LISTAGG_SWAP( *sPivotRow, *sLowRow );
        
        do
        {
            do
            {
                sLow++;
                if ( sLow < sHigh )
                {
                    MTF_LISTAGG_GET( aListaggFuncData, sLow, &sLowRow );

                    sOrder = compareListaggFuncDataBuffer( aTemplate,
                                                           aListaggFuncData,
                                                           *sPivotRow,
                                                           *sLowRow );

                    if ( sOrder )
                    {
                        sAllEqual = ID_FALSE;
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
            while ( ( sLow < sHigh ) && ( sOrder > 0 ) );

            do
            {
                sHigh--;
                MTF_LISTAGG_GET( aListaggFuncData, sHigh, &sHighRow );

                sOrder = compareListaggFuncDataBuffer( aTemplate,
                                                       aListaggFuncData,
                                                       *sPivotRow,
                                                       *sHighRow );
                if ( sOrder != 0 )
                {
                    sAllEqual = ID_FALSE;
                }
                else
                {
                    // nothing to do
                }
            }
            while ( sOrder < 0 );

            if ( sLow < sHigh )
            {
                MTF_LISTAGG_SWAP( *sLowRow, *sHighRow );
            }
            else
            {
                // nothing to do
            }
        }
        while ( sLow < sHigh );

        MTF_LISTAGG_SWAP( *sPivotRow, *sHighRow );
    }
    else
    {
        // nothing to do
    }

    *aPartition = sHigh;
    *aAllEqual = sAllEqual;

    return IDE_SUCCESS;
}

SInt compareListaggFuncDataBuffer( mtcTemplate         * /*aTemplate*/,
                                   mtfListaggFuncData  * aListaggFuncData,
                                   const void          * aElem1,
                                   const void          * aElem2 )
{
    SInt           sResult = 0;
    mtdValueInfo   sValueInfo1;
    mtdValueInfo   sValueInfo2;
    UInt           i;

    // compare
    for ( i = 0; ( i < aListaggFuncData->keyCount ) && ( sResult == 0 ); i++ )
    {
        sValueInfo1.value  = (void*)((SChar*)aElem1 + aListaggFuncData->keyOffset[i]);
        sValueInfo1.column = (const mtcColumn *)aListaggFuncData->keyColumn[i];
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.value  = (void*)((SChar*)aElem2 + aListaggFuncData->keyOffset[i]);
        sValueInfo2.column = (const mtcColumn *)aListaggFuncData->keyColumn[i];
        sValueInfo2.flag   = MTD_OFFSET_USE;

        if ( aListaggFuncData->keyOrderDesc[i] == ID_FALSE )
        {
            sResult = aListaggFuncData->keyColumn[i]->module->
                logicalCompare[MTD_COMPARE_ASCENDING](
                    &sValueInfo1,
                    &sValueInfo2 );
        }
        else
        {
            sResult = aListaggFuncData->keyColumn[i]->module->
                logicalCompare[MTD_COMPARE_DESCENDING](
                    &sValueInfo1,
                    &sValueInfo2 );
        }
    }

    return sResult;
}
