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
 * $Id: sdcDPathInsertMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 ******************************************************************************/

#ifndef  _O_SDC_DPATH_INSERT_MGR_H_
#define  _O_SDC_DPATH_INSERT_MGR_H_  1

#include <sdcDef.h>
#include <sdpDef.h>

class sdcDPathInsertMgr
{
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC allocDPathEntry( void **aDPathEntry );

    static IDE_RC destDPathEntry( void *aDPathEntry );

    static IDE_RC allocDPathSegInfo( idvSQL   * aStatistics,
                                     void     * aTrans,
                                     smOID      aTableOID,
                                     void    ** aDPathSegInfo );

    static IDE_RC setDirtyLastAllocPage( void *aDPathSegInfo );

    static IDE_RC commit( idvSQL  * aStatistics,
                          void    * aTrans,
                          void    * aDPathEntry );

    static IDE_RC abort( void * aDPathEntry );

    static void* getDPathBuffInfo( void *aTrans );
    static void* getDPathInfo( void *aTrans );
    static void* getDPathSegInfo( void   * aTrans,
                                  smOID    aTableOID );

    static IDE_RC getDPathStat( sdcDPathStat *aDPathStat );

    static IDE_RC dumpDPathEntry( sdcDPathEntry *aDPathEntry );
    static IDE_RC dumpDPathEntry( void *aTrans );

private:
    // sdpDPathEntry의 Slot을 TRANSACTION_TABLE_SIZE 만큼 만들어 둔다.
    static iduMemPool       mDPathEntryPool;

    // X$DIRECT_PATH_INSERT
    static sdcDPathStat     mDPathStat;
    static iduMutex         mStatMtx;
};

#endif
