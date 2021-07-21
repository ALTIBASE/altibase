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
 * $Id: acpProc.c 2164 2008-04-21 12:31:46Z shsuh $
 ******************************************************************************/

#include <acpConfig.h>


#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

#include <acpProc-WINDOWS.c>

#else

#include <acpProc-UNIX.c>

#endif
