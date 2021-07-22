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
 
/*******************************************************************************
 * $Id: 
 ******************************************************************************/

#if !defined(_O_ACV_H_) 
#define _O_ACV_H_  

#include <acp.h> 
#include <aclLic.h>

ACP_EXTERN_C_BEGIN 

/* 512 Data can be stored */
#define ACV_DATASIZE (512) 

/* Default license request file */
#define ACV_LICREQ_FILENAME ACP_STR_CONST("licreq.dat")

/* Kind of Operating systems */
#define ACV_OS_LINUX        "LINUX"
#define ACV_OS_SOLARIS      "SOLARIS"
#define ACV_OS_HPUX         "HPUX"
#define ACV_OS_AIX          "AIX"
#define ACV_OS_TRU64        "TRU64"
#define ACV_OS_WINDOWS      "WINDOWS"

/* For Windows and Linux, Mac address is Hardware ID
 * It must be multiple within one machine */
#if defined(ALTI_CFG_OS_WINDOWS) || defined(ALTI_CFG_OS_LINUX)
#define ACV_COMPARE_MAC ACP_TRUE
#else
#define ACV_COMPARE_MAC ACP_FALSE
#endif

#define ACV_LINE_LENGTH 128

typedef struct acv_license_request_t
{
    /* OS Type */
    acp_char_t mOSType[ACV_LINE_LENGTH];
    /* ID */
    acp_char_t mID[ACV_LINE_LENGTH];
    /* Product */
    acp_char_t mProduct[ACV_LINE_LENGTH];
    /* Expiry Date */
    acp_char_t mExpire[ACV_LINE_LENGTH];
    /* Request File Path */
    acp_char_t mRequest[ACV_LINE_LENGTH];
    /* License File Path */
    acp_char_t mLicense[ACV_LINE_LENGTH];

    /* Whether ID contains '-' or ':' with Mac Address */
    acp_bool_t mIDisRaw;
    /* Silent mode */
    acp_bool_t mIsSilent;
} acv_license_request_t;

/* Drop Enter with Input
 * When from stdin, Every input tails with Enter */
ACP_INLINE void acvDropEnter(acp_char_t* aLine, acp_uint32_t aLen)
{
    acp_uint32_t i;
    for(i = 0; i < aLen; i++)
    {
        if('\n' == aLine[i] || '\r' == aLine[i])
        {
            aLine[i] = ACP_CSTR_TERM;
            break;
        }
        else
        {
            /* Loop */
        }
    }
}

#define ACV_DATE_LENGTH 8
#define ACV_LICENSE_REQUEST_INIT \
{{0, }, {0, }, {0, }, {0, }, {0, }, {0, }, ACP_FALSE, ACP_FALSE}

#endif
