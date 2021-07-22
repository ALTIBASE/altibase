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
 

/*****************************************************************************
 * $Id: smuVersion.h 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#ifndef _O_SMU_VERSION_H_
#define _O_SMU_VERSION_H_ 1

#define SM_MAJOR_VERSION_MASK      (0xFF000000)
#define SM_MINOR_VERSION_MASK      (0x00FF0000)
#define SM_PATCH_VERSION_MASK      (0x0000FFFF)

#define SM_CHECK_VERSION_MASK      (0xFFFF0000)

extern const SChar *smVersionString;
extern const UInt smVersionID;

void smuMakeUniqueDBString(SChar *aUnique);
#endif
