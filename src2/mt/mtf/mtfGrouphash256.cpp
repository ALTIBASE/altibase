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
 * $Id: mtfGrouphash256.cpp 66435 2014-08-18 03:20:54Z myungsub.shin $
 **********************************************************************/

#include <idsSHA256.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfGrouphash256;

extern mtdModule mtdByte;
extern mtdModule mtdBinary;

static mtcName mtfGrouphash256FunctionName[1] = {
    { NULL, 12, (void*)"GROUPHASH256" }
};

static IDE_RC mtfGrouphash256Estimate( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

mtfModule mtfGrouphash256 = {
    2|MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfGrouphash256FunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfGrouphash256Estimate
};

IDE_RC mtfGrouphash256Initialize( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfGrouphash256Aggregate(   mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

IDE_RC mtfGrouphash256Finalize(   mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfGrouphash256Calculate(  mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtfGrouphash256Initialize,
    mtfGrouphash256Aggregate,
    mtf::calculateNA,
    mtfGrouphash256Finalize,
    mtfGrouphash256Calculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfGrouphash256Estimate( mtcNode     * aNode,
                                mtcTemplate * aTemplate,
                                mtcStack    * aStack,
                                SInt         /*aRemain*/,
                                mtcCallBack */*aCallBack*/ )
{
    SInt              sModuleId;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    sModuleId = (SInt) aStack[1].column->module->id;

    /* 연산 대상인 컬럼 외에는 에러를 출력한다. */
    if ( idlOS::strlen( (SChar*)aNode->arguments->module->names->string ) == 6 )
    {
        if ( idlOS::strncmp( (SChar*)aNode->arguments->module->names->string,
                             (const SChar*)"COLUMN", 6 )
             != 0 )
        {
            IDE_RAISE( ERR_ARGUMENT_NOT_APPICABLE );
        }
        else
        {
            /* 연산 대상 외인 컬럼 타입은 에러를 출력한다. */
            switch ( sModuleId )
            {
                case MTD_BLOB_ID:
                case MTD_BLOB_LOCATOR_ID:
                case MTD_CLOB_ID:
                case MTD_CLOB_LOCATOR_ID:
                case MTD_LIST_ID:
                case MTD_UNDEF_ID:
                case MTD_GEOMETRY_ID:
                    IDE_RAISE( ERR_ARGUMENT_NOT_APPICABLE );
                    break;
                default:
                    break;
            }
        }
    }
    else
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPICABLE );
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdByte,
                                     1,
                                     32,
                                     0 )
              != IDE_SUCCESS );

    /* 해싱 컨텍스트를 생성한다. */
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBinary,
                                     1,
                                     ID_SIZEOF(idsSHA256Context)
                                     + aStack[1].column->column.size,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPICABLE );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfGrouphash256Initialize( mtcNode     * aNode,
                                  mtcStack    */*aStack*/,
                                  SInt         /*aRemain*/,
                                  void        */*aInfo*/,
                                  mtcTemplate * aTemplate )
{
    const mtcColumn  * sColumn;
    mtdBinaryType    * sInfo;
    idsSHA256Context * sContext;

    /* 해싱 컨텍스트을 초기화 한다. */
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column + 1;
    sInfo = (mtdBinaryType*) mtc::value( sColumn,
                                         aTemplate->rows[aNode->table].row,
                                         MTD_OFFSET_USE );

    sContext = (idsSHA256Context*) sInfo->mValue;

    idsSHA256::initializeForGroup( sContext );

    // BUG-43709
    sInfo->mLength = sColumn->precision;

    return IDE_SUCCESS;
}

IDE_RC mtfGrouphash256Aggregate( mtcNode     * aNode,
                                 mtcStack    */*aStack*/,
                                 SInt         /*aRemain*/,
                                 void        */*aInfo*/,
                                 mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : Grouphash256 Aggregate
 *
 * Implementation :
 *
 *  GROUPHASH256( I1 )
 *
 *  한 컬럼을 해싱하는 집약함수를 구현한다.
 *    1. 해싱 함수에 컬럼의 값을 전달한다.
 *    2. 버퍼와 오프셋은 해싱 함수에서 이용한다.
 *
 ***********************************************************************/

    mtcNode          * sArgNode;
    const mtcColumn  * sArgColumn;
    const mtcColumn  * sColumn;
    mtdBinaryType    * sInfo;
    idsSHA256Context * sContext;
    void             * sValue;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column + 1;
    sInfo = (mtdBinaryType*) mtc::value( sColumn,
                                         aTemplate->rows[aNode->table].row,
                                         MTD_OFFSET_USE );

    sContext = (idsSHA256Context*) sInfo->mValue;

    sArgNode = aNode->arguments;
    sArgColumn = aTemplate->rows[sArgNode->table].columns + sArgNode->column;
    sValue = (void*) mtc::value( sArgColumn,
                                 aTemplate->rows[sArgNode->table].row,
                                 MTD_OFFSET_USE );

    if ( sArgColumn->module->isNull( sArgColumn, sValue ) == ID_TRUE )
    {
        sValue = sArgColumn->module->staticNull;
    }
    else
    {
        /* Nothing to do */
    }

    sContext->mMessageSize = sArgColumn->module->actualSize( sArgColumn,
                                                             sValue );

    idlOS::memcpy( (void*) sContext->mMessage, sValue, sContext->mMessageSize );

/* Big-endian */
#ifdef ENDIAN_IS_BIG_ENDIAN
    sArgColumn->module->endian( (void*) sContext->mMessage );
#endif

    idsSHA256::digestForGroup( sContext );

    return IDE_SUCCESS;
}

IDE_RC mtfGrouphash256Finalize( mtcNode     * aNode,
                                mtcStack    */*aStack*/,
                                SInt         /*aRemain*/,
                                void        */*aInfo*/,
                                mtcTemplate * aTemplate )
{
    const mtcColumn  * sColumn;
    mtdByteType      * sHash;
    mtdBinaryType    * sInfo;
    idsSHA256Context * sContext;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sHash = (mtdByteType*) mtc::value( sColumn,
                                       aTemplate->rows[aNode->table].row,
                                       MTD_OFFSET_USE );

    sInfo = (mtdBinaryType*) mtc::value( sColumn + 1,
                                         aTemplate->rows[aNode->table].row,
                                         MTD_OFFSET_USE );

    sContext = (idsSHA256Context*) sInfo->mValue;

    idsSHA256::finalizeForGroup( sHash->value, sContext );

    sHash->length = 32;

    return IDE_SUCCESS;
}

IDE_RC mtfGrouphash256Calculate( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt         /*aRemain*/,
                                 void        */*aInfo*/,
                                 mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*) ( (UChar*) aTemplate->rows[aNode->table].row
                               + aStack->column->column.offset );

    return IDE_SUCCESS;
}
