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
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtvModule mtvClob2Varchar;

extern mtdModule mtdVarchar;
extern mtdModule mtdClob;

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Clob2Varchar( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

mtvModule mtvClob2Varchar = {
    &mtdVarchar,
    &mtdClob,
    MTV_COST_DEFAULT|MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Clob2Varchar,
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
    SInt  sPrecision;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    /* BUG-36429 LOB Column에 대해서는 최대 Precision을 할당한다. */
    if ( aStack[1].column->precision != 0 )
    {
        sPrecision = IDL_MIN( aStack[1].column->precision,
                              MTD_VARCHAR_PRECISION_MAXIMUM );
    }
    else
    {
        sPrecision = MTD_VARCHAR_PRECISION_MAXIMUM;
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarchar,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Clob2Varchar( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    mtdClobLocatorType   sLocator;
    MTC_CURSOR_PTR       sCursor;
    idBool               sFound;
    idBool               sOpened = ID_FALSE;
    void               * sRow;
    scGRID               sGRID;
    UInt                 sInfo = 0;
    UShort               sOrgTableID;
    mtcColumn          * sOrgLobColumn;

    idBool               sIsClobColumn = ID_FALSE;
    mtdClobType        * sClobValue;
    mtdCharType        * sVarcharValue;
    UInt                 sLobLength;
    UInt                 sReadLength;
    idBool               sIsNull;

    const mtlModule    * sLanguage;
    idBool               sTruncated = ID_FALSE;
    UInt                 sSize;

    sLanguage = aStack[0].column->language;

    // convert4Server는 value형만을 처리할 수 있어 aNode가 NULL이다.
    if ( aNode != NULL )
    {
        if ( aTemplate->isBaseTable( aTemplate, aNode->baseTable ) == ID_TRUE )
        {
            // clob column인 경우
            sIsClobColumn = ID_TRUE;
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

    if ( sIsClobColumn == ID_TRUE )
    {
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
            /* BUG-48006 partition table nvl2 function error */
            if ( ( aTemplate->rows[sOrgTableID].lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
                 == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
            {
                sIsNull = ID_TRUE;
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

                sOpened = ID_TRUE;
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

                sOpened = ID_TRUE;
            }

            IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                                & sIsNull,
                                                & sLobLength,
                                                mtc::getStatistics(aTemplate) )
                      != IDE_SUCCESS );
        }

        /* BUG-47693 CLOB 컬럼에 empty 데이터가 있는 경우 조회 결과가 틀림 */
        if ( ( sIsNull == ID_TRUE ) ||
             ( sLobLength <= MTD_LOB_EMPTY_LENGTH ) )
        {
            mtdVarchar.null( aStack[0].column,
                             aStack[0].value );
        }
        else
        {
            // BUG-38842
            // clob to varchar conversion시 지정한 길이만큼만 변환한다.
            if ( MTU_CLOB_TO_VARCHAR_PRECISION < sLobLength )
            {
                sLobLength = MTU_CLOB_TO_VARCHAR_PRECISION;

                // 문자가 짤릴 수 있다.
                sTruncated = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            IDE_TEST_RAISE( (UInt)aStack[0].column->precision < sLobLength,
                            ERR_CONVERT );

            sVarcharValue = (mtdCharType*)aStack[0].value;

            IDE_TEST( mtc::readLob( mtc::getStatistics( aTemplate ), /* idvSQL* */
                                    sLocator,
                                    0,
                                    sLobLength,
                                    sVarcharValue->value,
                                    & sReadLength )
                      != IDE_SUCCESS );

            if ( sTruncated == ID_FALSE )
            {
                sVarcharValue->length = (UShort)sReadLength;
            }
            else
            {
                IDE_TEST( mtf::truncIncompletedString(
                              sVarcharValue->value,
                              sReadLength,
                              & sSize,
                              sLanguage )
                          != IDE_SUCCESS );

                sVarcharValue->length = (UShort)sSize;
            }
        }

        if ( sOpened == ID_TRUE )
        {
            sOpened = ID_FALSE;
            IDE_TEST( aTemplate->closeLobLocator( sLocator )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sClobValue = (mtdClobType *)aStack[1].value;
        
        if ( (MTD_LOB_IS_NULL( sClobValue->length ) == ID_TRUE) ||
             (sClobValue->length == MTD_LOB_EMPTY_LENGTH) )
        {
            mtdVarchar.null( aStack[0].column,
                             aStack[0].value );
        }
        else
        {
            sLobLength = sClobValue->length;
            
            // BUG-38842
            // clob to varchar conversion시 지정한 길이만큼만 변환한다.
            if ( MTU_CLOB_TO_VARCHAR_PRECISION < sLobLength )
            {
                sLobLength = MTU_CLOB_TO_VARCHAR_PRECISION;

                // 문자가 짤릴 수 있다.
                sTruncated = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            IDE_TEST_RAISE( (SLong)aStack[0].column->precision < sLobLength,
                            ERR_CONVERT );

            sVarcharValue = (mtdCharType *)aStack[0].value;

            idlOS::memcpy( sVarcharValue->value,
                           sClobValue->value,
                           sLobLength );

            if ( sTruncated == ID_FALSE )
            {
                sVarcharValue->length = (UShort)sLobLength;
            }
            else
            {
                IDE_TEST( mtf::truncIncompletedString(
                              sVarcharValue->value,
                              sLobLength,
                              & sSize,
                              sLanguage )
                          != IDE_SUCCESS );

                sVarcharValue->length = (UShort)sSize;
            }
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
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
 
