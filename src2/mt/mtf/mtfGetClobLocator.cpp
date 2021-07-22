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
 * $Id: mtfGetClobLocator.cpp 15573 2006-04-15 02:50:33Z mhjeong $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfGetClobLocator;

static mtcName mtfGetClobLocatorFunctionName[1] = {
    { NULL, 17, (void*)"_GET_CLOB_LOCATOR" }
};

static IDE_RC mtfGetClobLocatorEstimate( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

mtfModule mtfGetClobLocator = {
    1|MTC_NODE_OPERATOR_FUNCTION|
        MTC_NODE_PRINT_FMT_MISC,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // TODO : default selectivity
    mtfGetClobLocatorFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfGetClobLocatorEstimate
};

IDE_RC mtfGetClobLocatorCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfGetClobLocatorCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfGetClobLocatorEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         /* aRemain */,
                                  mtcCallBack* /* aCallBack */)
{
    extern mtdModule mtdClobLocator;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdClobLocator,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aStack[1].column->module->id != MTD_CLOB_ID,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( aTemplate->isBaseTable( aTemplate, aNode->arguments->table )
                    == ID_FALSE,
                    ERR_CONVERSION_NOT_APPLICABLE );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfGetClobLocatorCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
    mtdClobLocatorType   sLocator;
    MTC_CURSOR_PTR       sCursor;
    idBool               sFound;
    void               * sRow = NULL;
    scGRID               sGRID;
    UInt                 sInfo = 0;
    UShort               sOrgTableID;
    mtcColumn          * sOrgLobColumn;

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

    if ( sFound == ID_FALSE )
    {
        /* BUG-42173 OUTER JOIN is processed anomaly about partitioned tables, which
         * have difference row counts
         */
        if ( ( aTemplate->rows[sOrgTableID].lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
             == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
        {
            sLocator = MTD_CLOB_LOCATOR_NULL;
        }
        else
        {
            IDE_RAISE( ERR_CONVERSION_NOT_APPLICABLE );
        }
    }
    else
    {
        sRow = aTemplate->rows[sOrgTableID].row;
        SC_COPY_GRID( aTemplate->rows[sOrgTableID].rid,
                      sGRID );
        if ( (aTemplate->rows[aNode->arguments->table].
              columns[aNode->arguments->column].flag &
              MTC_COLUMN_NOTNULL_MASK)
             == MTC_COLUMN_NOTNULL_TRUE )
        {
            sInfo |= MTC_LOB_COLUMN_NOTNULL_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        sOrgLobColumn = aTemplate->rows[sOrgTableID].columns + aNode->arguments->column;

        // BUG-43780
        if ( (sOrgLobColumn->column.flag & SMI_COLUMN_STORAGE_MASK)
             == SMI_COLUMN_STORAGE_MEMORY )
        {
            IDE_TEST_RAISE( sRow == NULL, ERR_NOT_APPLICABLE );

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

        // BUG-40427
        // 최초로 Open된 Lob Locator라면, qmcCursor에 기록한다.
        IDE_TEST( aTemplate->addOpenedLobCursor( aTemplate,
                                                 sLocator )
                  != IDE_SUCCESS );

    }

    *(mtdClobLocatorType*)aStack[0].value = sLocator;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
