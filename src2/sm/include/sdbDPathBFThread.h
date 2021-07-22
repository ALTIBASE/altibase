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
 

/*******************************************************************************
 * $Id$
 ******************************************************************************/

#ifndef _O_SDB_DPATH_BFTHREAD_H_
#define _O_SDB_DPATH_BFTHREAD_H_ 1

#include <sdbDef.h>
#include <idtBaseThread.h>

class sdbDPathBFThread : public idtBaseThread
{
public:
    sdbDPathBFThread();
    virtual ~sdbDPathBFThread();

    IDE_RC initialize( sdbDPathBuffInfo * aDPathBCBInfo );
    IDE_RC destroy();

    IDE_RC startThread();
    IDE_RC shutdown();

    virtual void run();

private:
    PDL_Time_Value    mTV;
    idBool            mFinish;

    /* Direct Path Insert시  필요한 정보 */
    sdbDPathBuffInfo*   mDPathBCBInfo;

    /* Direct Path Insert시 Bulk IO를 할때 필요한 정보 */
    sdbDPathBulkIOInfo  mDPathBulkIOInfo;
};

#endif /* _O_SDB_DPATH_BFTHREAD_H_ */
