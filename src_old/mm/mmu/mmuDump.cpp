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

#include <ideErrorMgr.h>
#include <ideLog.h>
#include <idErrorCode.h>
#include <mmuDump.h>

SInt mmuDump::dump(
void   * /*p*/, 
vULong   /*len*/, 
idBool   /*is_relative_base_address*/)
{
#define IDE_FN "mmuDump::dump()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    return IDE_SUCCESS;
#undef IDE_FN    
}

SInt mmuDump::dump(SChar * /*object_name*/)
{
#define IDE_FN "mmuDump::dump()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;
#undef IDE_FN    
}

