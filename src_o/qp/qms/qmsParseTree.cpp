/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

#include <qmsParseTree.h>
#include <qtc.h>

/*
 * BUG-42296
 * startValue 와 countValue 를 가져오는 interface 분리
 */

IDE_RC qmsLimitI::getStartValue(qcTemplate* aTemplate,
                                qmsLimit  * aLimit,
                                ULong     * aResultValue)
{
    mtdBigintType sValue;
    qmsLimitValue sLimitValue;

    IDU_FIT_POINT_FATAL( "qmsLimitI::getStartValue::__FT__" );

    sLimitValue = aLimit->start;

    if ( sLimitValue.hostBindNode != NULL )
    {
        IDE_TEST(getPrimitiveValue(aTemplate, sLimitValue, &sValue)
                 != IDE_SUCCESS);

        /* BUG-36580 supported TOP */
        // To fix BUG-12557 limit절에 호스트변수 사용
        // 호스트변수 사용시, 값이 0보다 커야 한다.
        IDE_TEST_RAISE( sValue <= 0, limit_bound_error );

        *aResultValue = sValue;
    }
    else
    {
        *aResultValue = sLimitValue.constant;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( limit_bound_error );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_LIMIT_VALUE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmsLimitI::getCountValue(qcTemplate* aTemplate,
                                qmsLimit  * aLimit,
                                ULong     * aResultValue)
{
    mtdBigintType sValue;
    qmsLimitValue sLimitValue;

    sLimitValue = aLimit->count;

    if ( sLimitValue.hostBindNode != NULL )
    {
        IDE_TEST(getPrimitiveValue(aTemplate, sLimitValue, &sValue)
                 != IDE_SUCCESS);

        /* BUG-36580 supported TOP */
        // To fix BUG-12557 limit절에 호스트변수 사용
        // 호스트변수 사용시, 값이 0보다 커야 한다.
        IDE_TEST_RAISE( sValue < 0, limit_bound_error );

        *aResultValue = sValue;
    }
    else
    {
        *aResultValue = sLimitValue.constant;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( limit_bound_error );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_LIMIT_VALUE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmsLimitI::getPrimitiveValue(qcTemplate   * aTemplate,
                                    qmsLimitValue  aLimitValue,
                                    mtdBigintType* aResultValue)
{
    qtcNode         * sNode;
    mtcStack        * sStack;
    const mtcColumn * sColumn;
    void            * sValue;

    IDU_FIT_POINT_FATAL( "qmsLimitI::getPrimitiveValue::__FT__" );

    sStack = aTemplate->tmplate.stack;
    sNode  = aLimitValue.hostBindNode;

    // BUG-16055
    IDE_TEST( qtc::calculate( sNode, aTemplate )
              != IDE_SUCCESS );

    sColumn = sStack->column;
    sValue  = sStack->value;

    IDE_TEST_RAISE( sColumn->module->id != MTD_BIGINT_ID,
                    limit_type_error );

    *aResultValue = *(mtdBigintType*)sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( limit_type_error );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_LIMIT_VALUE_TYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
