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
 * Spatio-Temporal Environment Handle 관련 함수
 *
 ***********************************************************************/

#include <ulsEnvHandle.h>

/*----------------------------------------------------------------*
 *  External Interfaces
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Environment Handle을 초기화한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACSRETURN ulsAllocEnv( ulsHandle ** aHandle )
{
    ulsHandle * sHandle = NULL;

    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACI_TEST( aHandle == NULL );

    /*------------------------------*/
    /* Initialize Environment*/
    /*------------------------------*/
    ACI_TEST( acpMemAlloc((void**)&sHandle, ACI_SIZEOF(ulsHandle)) != ACP_RC_SUCCESS );

    acpMemSet( sHandle, 0x00, ACI_SIZEOF(ulsHandle) );

    ACI_TEST( ulsInitEnv( sHandle ) != ACI_SUCCESS );
    
    *aHandle = sHandle;
    
    return ACS_SUCCESS;

    ACI_EXCEPTION_END;

    *aHandle = NULL;

    if ( sHandle != NULL )
    {
        acpMemFree( sHandle );
    }
    else
    {
        /* do nothing */
    }
    
    return ACS_INVALID_HANDLE;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Environment Handle을 초기화한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACSRETURN ulsFreeEnv( ulsHandle * aHandle )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACI_TEST( ulsCheckEnv( aHandle ) != ACI_SUCCESS );

    /*------------------------------*/
    /* Free Environment*/
    /*------------------------------*/
    
    aHandle->mInit = ULS_HANDLE_INVALID;
    
    acpMemFree( aHandle );
    
    return ACS_SUCCESS;

    ACI_EXCEPTION_END;

    return ACS_INVALID_HANDLE;
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Environment Handle 에 존재하는 Error 정보를 획득한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACSRETURN ulsGetError( ulsHandle    * aHandle,
                       acp_uint32_t * aErrorCode,
                       acp_char_t  ** aErrorMessage,
                       acp_sint16_t * aErrorMessageLength )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/
    
    ACI_TEST_RAISE( ulsCheckEnv( aHandle ) != ACI_SUCCESS,
                    ERR_INVALID_HANDLE );

    ACI_TEST_RAISE( aErrorCode == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aErrorMessage == NULL, ERR_NULL_PARAMETER );
    ACI_TEST_RAISE( aErrorMessageLength == NULL, ERR_NULL_PARAMETER );
    
    /*------------------------------*/
    /* Get Error Information*/
    /*------------------------------*/

    *aErrorCode = ulsGetErrorCode( & aHandle->mErrorMgr );
    *aErrorMessage = (acp_char_t*) ulsGetErrorMsg( & aHandle->mErrorMgr );
    *aErrorMessageLength = acpCStrLen( *aErrorMessage, ACP_SINT32_MAX );
    
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

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Handle을 초기화한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACI_RC  ulsInitEnv( ulsHandle * aHandle )
{
    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACE_DASSERT( aHandle != NULL );

    /*------------------------------*/
    /* Initialize Environment*/
    /*------------------------------*/
    
    aHandle->mInit = ULS_HANDLE_INITIALIZED;

    ulsClearError( & aHandle->mErrorMgr );
    
    return ACI_SUCCESS;

    /* ACI_EXCEPTION_END;*/

    /* return ACI_FAILURE;*/
}

/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Handle의 유효성을 검사한다.
 *
 * Implementation:
 *
 *---------------------------------------------------------------*/

ACI_RC  ulsCheckEnv( ulsHandle * aHandle )
{
    /*------------------------------*/
    /* Environment Validation*/
    /*------------------------------*/
    
    ACI_TEST( aHandle == NULL );

    ACI_TEST( aHandle->mInit != ULS_HANDLE_INITIALIZED );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


/*----------------------------------------------------------------*
 *
 * Description:
 *
 *   Handle에 Error Code를 셋팅한다. 
 *
 * Implementation:
 *
 *   copy from ulnErrorMgr.cpp
 * 
 *---------------------------------------------------------------*/

void ulsSetErrorCode( ulsHandle * aHandle, acp_uint32_t aErrorCode, ... )
{
    va_list         sArgs;

    /*------------------------------*/
    /* Parameter Validation*/
    /*------------------------------*/

    ACE_DASSERT( aHandle != NULL );

    /*------------------------------*/
    /* Set Error Code*/
    /*------------------------------*/
    
    va_start(sArgs, aErrorCode);
    ulsErrorSetError( & aHandle->mErrorMgr, aErrorCode, sArgs);
    va_end(sArgs);
    
}

