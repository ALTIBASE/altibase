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
 * $Id: acpEnv.h 8132 2009-10-13 10:02:53Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ENV_H_)
#define _O_ACP_ENV_H_

/**
 * @file
 * @ingroup CoreEnv
 * @ingroup CoreSys
 */

#include <acpStr.h>

#if defined(ALTI_CFG_OS_DARWIN)
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#endif

ACP_EXTERN_C_BEGIN


ACP_EXPORT acp_rc_t acpEnvGet(const acp_char_t *aName, acp_char_t **aValue);

ACP_EXPORT acp_rc_t acpEnvSet(const acp_char_t  *aName,
                              const acp_char_t  *aValue,
                              acp_bool_t   aOverwrite);

ACP_EXPORT acp_rc_t acpEnvList(acp_char_t**       aEnv,
                               const acp_size_t   aEnvSize,
                               acp_size_t*        aNoEnv);


ACP_EXTERN_C_END


#endif
