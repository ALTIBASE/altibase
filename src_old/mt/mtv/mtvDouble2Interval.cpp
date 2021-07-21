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
 * $Id: mtvDouble2Interval.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvDouble2Interval;

extern mtdModule mtdInterval;
extern mtdModule mtdDouble;

extern mtxModule mtxFromDoubleTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Double2Interval( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtvModule mtvDouble2Interval = {
    &mtdInterval,
    &mtdDouble,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY|
    MTV_COST_ERROR_PENALTY|MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Double2Interval,
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
        = mtxFromDoubleTo.mGetExecute( mtdInterval.id, mtdInterval.id );

    //IDE_TEST( mtdInterval.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInterval,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Double2Interval( mtcNode*,
                                     mtcStack*    aStack,
                                     SInt,
                                     void*,
                                     mtcTemplate* )
{
    mtdDoubleType    sDouble;
    mtdIntervalType* sInterval;
    SDouble          sIntegralPart;
    SDouble          sFractionalPart;

    if( mtdDouble.isNull( aStack[1].column,
                          aStack[1].value ) == ID_TRUE )
    {
        mtdInterval.null( aStack[0].column,
                          aStack[0].value );
    }
    else
    {
        sDouble   = *(mtdDoubleType*)aStack[1].value;
        sInterval = (mtdIntervalType*)aStack[0].value;
        sDouble  *= 864e2;
        IDE_TEST_RAISE( mtdDouble.isNull( mtdDouble.column, &sDouble )
                        == ID_TRUE, ERR_VALUE_OVERFLOW );

        sFractionalPart = modf( sDouble, &sIntegralPart );
        IDE_TEST_RAISE( sIntegralPart > 9e18 || sIntegralPart < -9e18,
                        ERR_VALUE_OVERFLOW );
        sInterval->second      = (SLong)sIntegralPart;

        sFractionalPart = modf( sFractionalPart*1e6, &sIntegralPart );
        sInterval->microsecond = (SLong)sIntegralPart;

        // BUG-40967
        // 오차 보정 (반올림)
        if ( sFractionalPart >= 0.5 )
        {
            sInterval->microsecond++;
            
            if ( sInterval->microsecond == 1000000 )
            {
                sInterval->second++;
                sInterval->microsecond = 0;
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( sFractionalPart <= -0.5 )
        {
            sInterval->microsecond--;
            
            if ( sInterval->microsecond == -1000000 )
            {
                sInterval->second--;
                sInterval->microsecond = 0;
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
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
