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
 

/***********************************************************************
 * $Id:$
 **********************************************************************/

#ifndef _O_SDB_DWRECOVERY_MGR_H_
#define _O_SDB_DWRECOVERY_MGR_H_ 1

#include <sdbDef.h>
#include <idu.h>
#include <idv.h>
#include <sddDWFile.h>
#include <sdbFlusher.h>

class sdbDWRecoveryMgr
{
public:
    static IDE_RC recoverCorruptedPages();

private:
    static IDE_RC recoverDWFile(sddDWFile *aDWFile);
};

#endif // _O_SDB_DWRECOVERY_MGR_H_

