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

#if !defined(_O_ACP_VERSION_H_)
#define _O_ACP_VERSION_H_

/**
 * @file
 * @ingroup CoreVersion
 */

#include <acpTypes.h>

ACP_EXTERN_C_BEGIN

#define ACP_VERSION_MAJOR acpVersionMajor()
#define ACP_VERSION_MINOR acpVersionMinor()
#define ACP_VERSION_TERM  acpVersionTerm()
#define ACP_VERSION_PATCH acpVersionPatch()

/**
 * Major version
 * @return major version of Altibaes Core Library
 */
ACP_EXPORT acp_uint32_t acpVersionMajor(void);

/**
 * Minor version
 * @return minor version of Altibaes Core Library
 */
ACP_EXPORT acp_uint32_t acpVersionMinor(void);

/**
 * Term version
 * @return term version of Altibaes Core Library
 */
ACP_EXPORT acp_uint32_t acpVersionTerm(void);

/**
 * Patch version
 * @return patch version of Altibaes Core Library
 */
ACP_EXPORT acp_uint32_t acpVersionPatch(void);

ACP_EXTERN_C_END

#endif
