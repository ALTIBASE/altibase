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
 *
 * Spatio-Temporal Date 조작 함수
 *
 ***********************************************************************/

#include <ulsDateFunc.h>

/*----------------------------------------------------------------*
 *  External Interfaces
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   DATE 값를 획득한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACSRETURN ulsGetDate( ulsHandle    * aHandle,
                      mtdDateType  * aDateValue,
                      acp_sint32_t * aYear,
                      acp_sint32_t * aMonth,
                      acp_sint32_t * aDay,
                      acp_sint32_t * aHour,
                      acp_sint32_t * aMin,
                      acp_sint32_t * aSec,
                      acp_sint32_t * aMicSec )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST_RAISE( aDateValue == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aYear == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aMonth == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aDay == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aHour == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aMin == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aSec == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aMicSec == NULL, ERR_NULL_PARAMETER );

    /*------------------------------*/
    /* Get Date Value*/
    /*------------------------------*/
    
    *aYear = (acp_sint32_t) mtdDateInterfaceYear( aDateValue );
    *aMonth = (acp_sint32_t) mtdDateInterfaceMonth( aDateValue );
    *aDay = (acp_sint32_t) mtdDateInterfaceDay( aDateValue );
    *aHour = (acp_sint32_t) mtdDateInterfaceHour( aDateValue );
    *aMin = (acp_sint32_t) mtdDateInterfaceMinute( aDateValue );
    *aSec = (acp_sint32_t) mtdDateInterfaceSecond( aDateValue );
    *aMicSec = (acp_sint32_t) mtdDateInterfaceMicroSecond( aDateValue );
    
    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    
    ACI_EXCEPTION_END;

    return ACS_ERROR;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   DATE 값을 설정한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACSRETURN ulsSetDate( ulsHandle   * aHandle,
                      mtdDateType * aDateValue,
                      acp_sint32_t  aYear,
                      acp_sint32_t  aMonth,
                      acp_sint32_t  aDay,
                      acp_sint32_t  aHour,
                      acp_sint32_t  aMin,
                      acp_sint32_t  aSec,
                      acp_sint32_t  aMicSec )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );
    
    ACI_TEST_RAISE( aDateValue == NULL, ERR_NULL_PARAMETER );

    /*------------------------------*/
    /* Make Date Value*/
    /*------------------------------*/
    
    ACI_TEST( mtdDateInterfaceMakeDate( aDateValue,
                                          (acp_sint16_t) aYear,
                                          (acp_uint8_t)  aMonth,
                                          (acp_uint8_t)  aDay,
                                          (acp_uint8_t)  aHour,
                                          (acp_uint8_t)  aMin,
                                          (acp_uint8_t)  aSec,
                                          (acp_uint32_t) aMicSec)
              != ACI_SUCCESS );
    
    return ACS_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_HANDLE );
    {
        return ACS_INVALID_HANDLE;
    }
    
    ACI_EXCEPTION( ERR_NULL_PARAMETER );
    {
        ulsSetErrorCode( aHandle, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    
    ACI_EXCEPTION_END;

    return ACS_ERROR;
}
   
/*----------------------------------------------------------------*
 *  Internal Interfaces
 *----------------------------------------------------------------*/
