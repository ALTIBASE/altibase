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

#include <acpFeature.h>
#include <acpCStr.h>
#include <acpMem.h>

ACP_EXPORT acp_rc_t acpFeatureStatus(acp_feature_t aRequest, ...)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    va_list  sArgs;

    acp_bool_t *sResult = NULL;
    acp_char_t *sUserBuf = NULL;

    va_start(sArgs, aRequest);

    switch (aRequest)
    {
        case ACP_FEATURE_TYPE_ATOMIC:
            sResult = va_arg(sArgs, acp_bool_t *);
            *sResult = ACP_FEATURE_NATIVE_ATOMIC;

            sUserBuf = va_arg(sArgs, acp_char_t *);
#if (defined ALTI_CFG_CPU_PARISC)
            /* Atomic operations are not supported on PARISC */
#elif (defined ALTI_CFG_OS_SOLARIS) && \
    (ALTI_CFG_OS_MAJOR == 2) &&  \
    (ALTI_CFG_OS_MINOR <= 9)
            acpMemCpy(sUserBuf, "SIMULATE", acpCStrLen("SIMULATE", 10));
#else
            acpMemCpy(sUserBuf, "NATIVE", acpCStrLen("NATIVE", 10));
#endif
            break;
        case ACP_FEATURE_TYPE_CALLSTACK:
            sResult = va_arg(sArgs, acp_bool_t *);
            *sResult = ACP_FEATURE_CALLSTACK;
            break;
        case ACP_FEATURE_TYPE_IPV6:
            sResult = va_arg(sArgs, acp_bool_t *);
            *sResult = ACP_FEATURE_IPV6;
            break;
        default:
            sRC = ACP_RC_EINVAL;
            break;
    }

    va_end(sArgs);
        
    return sRC;
}

