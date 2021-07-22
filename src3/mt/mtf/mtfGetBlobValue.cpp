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

extern mtfModule mtfGetBlobValue;

static mtcName mtfGetBlobValueFunctionName[1] = {
    { NULL, 15, (void*)"_GET_BLOB_VALUE" }
};

static IDE_RC mtfGetBlobValueEstimate( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       mtcCallBack* aCallBack );

mtfModule mtfGetBlobValue = {
    1|MTC_NODE_OPERATOR_FUNCTION|
    MTC_NODE_PRINT_FMT_MISC,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // TODO : default selectivity
    mtfGetBlobValueFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfGetBlobValueEstimate
};

IDE_RC mtfGetBlobValueCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfGetBlobValueCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfGetBlobValueEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         /* aRemain */,
                                mtcCallBack* /* aCallBack */)
{
    extern mtdModule mtdBlob;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBlob,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aStack[1].column->module->id != MTD_BLOB_ID,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfGetBlobValueCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
    mtdBlobLocatorType   sLocator;
    MTC_CURSOR_PTR       sCursor;
    idBool               sFound;
    void               * sRow;
    scGRID               sGRID;
    UInt                 sInfo = 0;
    UShort               sOrgTableID;
    mtcColumn          * sOrgLobColumn;

    mtdBlobType        * sBlobValue;
    UInt                 sLobLength;
    UInt                 sReadLength;
    idBool               sIsNull;
    idBool               sOpened = ID_FALSE;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    // PROJ-1362
    // Lob Locator를 얻는데 필요한 커서정보를 가져온다.
    IDE_TEST( aTemplate->getOpenedCursor( aTemplate,
                                          aNode->arguments->table,
                                          & sCursor,
                                          & sOrgTableID,
                                          & sFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sFound != ID_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
    sRow = aTemplate->rows[sOrgTableID].row;
    SC_COPY_GRID( aTemplate->rows[sOrgTableID].rid, sGRID );
    
    sOrgLobColumn = aTemplate->rows[sOrgTableID].columns + aNode->arguments->column;

    // BUG-40130
    sInfo |= MTC_LOB_LOCATOR_CLOSE_TRUE;

    // BUG-43780
    if ( (sOrgLobColumn->column.flag & SMI_COLUMN_STORAGE_MASK)
         == SMI_COLUMN_STORAGE_MEMORY )
    {
        IDE_TEST( mtc::openLobCursorWithRow( sCursor,
                                             sRow,
                                             & sOrgLobColumn->column,
                                             sInfo,
                                             SMI_LOB_TABLE_CURSOR_MODE,
                                             & sLocator )
                  != IDE_SUCCESS );
    }
    else
    {
        // BUG-16318
        IDE_TEST_RAISE( SC_GRID_IS_NULL(sGRID), ERR_NOT_APPLICABLE );
        //fix BUG-19687
        IDE_TEST( mtc::openLobCursorWithGRID( sCursor,
                                              sGRID,
                                              & sOrgLobColumn->column,
                                              sInfo,
                                              SMI_LOB_TABLE_CURSOR_MODE,
                                              & sLocator )
                  != IDE_SUCCESS );
    }
    sOpened = ID_TRUE;

    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLobLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );

    if ( sIsNull == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        IDE_TEST_RAISE( (UInt)aStack[0].column->precision < sLobLength,
                        ERR_CONVERT );

        sBlobValue = (mtdBlobType*)aStack[0].value;

        IDE_TEST( mtc::readLob( mtc::getStatistics( aTemplate ), /* idvSQL* */
                                sLocator,
                                0,
                                sLobLength,
                                sBlobValue->value,
                                & sReadLength )
                  != IDE_SUCCESS );

        sBlobValue->length = (SLong)sReadLength;
    }

    sOpened = ID_FALSE;
    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_CONVERT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;
    
    if ( sOpened == ID_TRUE )
    {
        (void) aTemplate->closeLobLocator( sLocator );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}
 
