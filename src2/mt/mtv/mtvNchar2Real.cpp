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
 * $Id: mtvNchar2Real.cpp 47933 2011-06-30 02:01:37Z et16 $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvNchar2Real;

extern mtdModule mtdReal;
extern mtdModule mtdNchar;

extern mtxModule mtxFromNcharTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Nchar2Real( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

mtvModule mtvNchar2Real = {
    &mtdReal,
    &mtdNchar,
    MTV_COST_DEFAULT | MTV_COST_ERROR_PENALTY | MTV_COST_LOSS_PENALTY * 2 | MTV_COST_GROUP_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Nchar2Real,
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

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxFromNcharTo.mGetExecute( mtdReal.id, mtdReal.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdReal,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Nchar2Real( mtcNode*,
                               mtcStack*    aStack,
                               SInt,
                               void*,
                               mtcTemplate* )
{
    mtdDoubleType sDouble;

    if( mtdNchar.isNull( aStack[1].column,
                         aStack[1].value ) == ID_TRUE )
    {
        mtdReal.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        IDE_TEST( mtdNcharInterface::toChar( aStack,
                                             (const mtlModule *) mtl::mNationalCharSet,
                                             (const mtlModule *) mtl::mDBCharSet,
                                             (mtdNcharType*)aStack[1].value,
                                             (mtdCharType*)aStack[0].value )
                  != IDE_SUCCESS );

        IDE_TEST( mtv::character2NativeR( aStack ) != IDE_SUCCESS );

        sDouble = *((mtdDoubleType*) aStack[0].value);
        *(mtdRealType*) aStack[0].value = (mtdRealType) sDouble;

        IDE_TEST_RAISE( mtdReal.isNull( aStack[0].column,
                                        (mtdRealType*) aStack[0].value,
                                        MTD_OFFSET_USELESS )
                        == ID_TRUE, ERR_VALUE_OVERFLOW );

        if( ( idlOS::fabs(*(mtdRealType*)aStack[0].value) < MTD_REAL_MINIMUM ) &&
            ( *(mtdRealType*)aStack[0].value != 0 ) )
        {
            *(mtdRealType*)aStack[0].value = 0;
        }
        else
        {
            // Nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
