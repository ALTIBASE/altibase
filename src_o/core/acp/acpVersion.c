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

#include <acpVersion.h>

/*
 * Major version
 */
ACP_EXPORT acp_uint32_t acpVersionMajor(void)
{
    return ALTICORE_MAJOR_VERSION;
}

/*
 * Minor version
 */
ACP_EXPORT acp_uint32_t acpVersionMinor(void)
{
    return ALTICORE_MINOR_VERSION;
}

/*
 * Term version
 */
ACP_EXPORT acp_uint32_t acpVersionTerm(void)
{
    return ALTICORE_TERM_VERSION;
}

/*
 * Patch version
 */
ACP_EXPORT acp_uint32_t acpVersionPatch(void)
{
    return ALTICORE_PATCH_VERSION;
}
