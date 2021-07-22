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
 
/*****************************************************************************
 * $Id:
 ****************************************************************************/

/* **********************************************************************
 *   $Id: 
 *   NAME
 *     iddDef.h - definitions
 *
 *   DESCRIPTION
 *
 *   PUBLIC FUNCTION(S)
 *
 *   PRIVATE FUNCTION(S)
 *
 *   NOTES
 *
 *   MODIFIED   (MM/DD/YY)
 ********************************************************************** */

#ifndef O_IDD_DEF_H
#define O_IDD_DEF_H   1

#include <acp.h>
#include <idl.h>
#include <ideErrorMgr.h>
#include <idu.h>

typedef SInt (*iddCompFunc)(const void*, const void*);
typedef SInt (*iddHashFunc)(const void*);

typedef struct iddRBHashStat
{
    SChar                   mName[32];
    UInt                    mBucketNo;
    ULong                   mKeyLength;
    ULong                   mCount;
    ULong                   mSearchCount;
    ULong                   mInsertLeft;
    ULong                   mInsertRight;
} iddRBHashStat;

#endif /* O_IDD_DEF_H */

