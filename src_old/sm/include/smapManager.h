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
 * $Id: smapManager.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMA_MANAGER_H_
# define _O_SMA_MANAGER_H_ 1

#include <smaRefineDB.h>

/* ------------------------------------------------
 *  Parallel RefineDB Thread Manager
 * ----------------------------------------------*/

class smapManager
{
private:
    static iduMutex           mMutex;

public:
    static iduMemPool         mJobPool;
    static smapRebuildJobItem mJobItemHeader;
    
 public:
    static IDE_RC doIt(SInt   aLoaderCnt );

    static IDE_RC addJob(smapRebuildJobItem  ** aJobItem,
                         smcTableHeader       * aTable);

    static IDE_RC lock() { return mMutex.lock( NULL /* idvSQL * */); }
    static IDE_RC unlock() { return mMutex.unlock(); }
    
//    static smapRebuildJobItem* getJob();
    
    static IDE_RC removeJob(smapRebuildJobItem* aJobItem);

    static IDE_RC initializeJobInfo(smapRebuildJobItem * aJobItem, 
                                    smcTableHeader     * aTable);

    static IDE_RC destroyJobInfo(smapRebuildJobItem* aJobItem);
};

#endif /* _O_SMAP_MANAGER_H_ */
