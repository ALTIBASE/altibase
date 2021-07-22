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
 * $Id: mtvClob2ClobLocator.cpp 13146 2005-08-12 09:20:06Z sungminee $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvClob2ClobLocator;

extern mtdModule mtdClob;
extern mtdModule mtdClobLocator;

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Clob2ClobLocator( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

mtvModule mtvClob2ClobLocator = {
    &mtdClobLocator,
    &mtdClob,
    MTV_COST_DEFAULT,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Clob2ClobLocator,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdClobLocator,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Clob2ClobLocator( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt,
                                      void*,
                                      mtcTemplate* aTemplate )
{
    mtdClobLocatorType   sLocator;
    MTC_CURSOR_PTR       sCursor;
    idBool               sFound;
    void               * sRow;
    scGRID               sGRID;
    UInt                 sInfo = 0;

    UShort              sOrgTableID;
    mtcColumn         * sOrgLobColumn;
    
    // convert4Server는 value형만을 처리할 수 있어 aNode가 NULL이다.
    IDE_TEST_RAISE( aNode == NULL, ERR_CONVERSION_NOT_APPLICABLE );
    
    // LOB Column인지 확인한다.
    IDE_TEST_RAISE( aTemplate->isBaseTable( aTemplate, aNode->baseTable ) != ID_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );

    // PROJ-1362
    // Lob Locator를 얻는데 필요한 커서정보를 가져온다.
    IDE_TEST( aTemplate->getOpenedCursor( aTemplate,
                                          aNode->baseTable,
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
        SC_COPY_GRID( aTemplate->rows[sOrgTableID].rid, sGRID );

        sInfo |= MTC_LOB_LOCATOR_CLOSE_TRUE;

        sOrgLobColumn = aTemplate->rows[sOrgTableID].columns
            + aNode->baseColumn;

        IDE_ASSERT_MSG( sOrgLobColumn->module->id == MTD_CLOB_ID,
                        "sOrgLobColumn->module->id : %"ID_UINT32_FMT"\n",
                        sOrgLobColumn->module->id );

        // BUG-43780
        if ( (sOrgLobColumn->column.flag & SMI_COLUMN_STORAGE_MASK)
             == SMI_COLUMN_STORAGE_MEMORY )
        {
            IDE_TEST( mtc::openLobCursorWithRow( sCursor,
                                                 sRow,
                                                 & sOrgLobColumn->column,
                                                 sInfo,
                                                 SMI_LOB_READ_MODE,
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
                                                  SMI_LOB_READ_MODE,
                                                  & sLocator )
                      != IDE_SUCCESS );
        }
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
 
