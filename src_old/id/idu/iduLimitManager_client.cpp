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
 * $Id: iduLimitManager.cpp 39643 2010-05-28 11:30:20Z djin $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idp.h>
#include <iduLimitManager.h>

#if defined(ALTIBASE_LIMIT_CHECK)

limitPointFunc     iduLimitManager::mLimitPoint     = NULL;
limitPointDoneFunc iduLimitManager::mLimitPointDone = NULL;
PDL_SHLIB_HANDLE   iduLimitManager::mLimitDl        = PDL_SHLIB_INVALID_HANDLE;

IDE_RC iduLimitManager::loadFunctions()
{
    return IDE_SUCCESS;
}
    
SInt iduLimitManager::limitPoint( SChar *, SChar * )
{
    return 0;
}

void iduLimitManager::limitPointDone()
{
    return;
}

#endif
