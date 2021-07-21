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

#ifndef _O_SDB_DPATH_BUFFER_MGR_H_
#define _O_SDB_DPATH_BUFFER_MGR_H_ 1

#include <sdbDef.h>

class sdbDPathBufferMgr
{
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC createPage( idvSQL            * aStatistics,
                              void              * aTrans,
                              scSpaceID           aSpaceID,
                              scPageID            aPageID,
                              idBool              aIsLogging,
                              UChar            ** aPagePtr );

    static IDE_RC setDirtyPage( void* aPagePtr );

    static IDE_RC flushAllPage( idvSQL            * aStatistics,
                                sdbDPathBuffInfo  * aDPathBuffInfo );

    static IDE_RC cancelAll( sdbDPathBuffInfo  * aDPathBuffInfo );

    static IDE_RC flushBCBInList( idvSQL             * aStatistics,
                                  sdbDPathBuffInfo   * aDPathBCBInfo,
                                  sdbDPathBulkIOInfo * aDPathBulkIOInfo,
                                  smuList            * aBaseNode,
                                  scSpaceID          * aNeedSyncSpaceID);

    static IDE_RC initDPathBuffInfo( sdbDPathBuffInfo *aDPathBuffInfo );
    static IDE_RC destDPathBuffInfo( sdbDPathBuffInfo *aDPathBuffInfo );

    static IDE_RC initBulkIOInfo( sdbDPathBulkIOInfo *aDPathBulkIOInfo );
    static IDE_RC destBulkIOInfo( sdbDPathBulkIOInfo *aDPathBulkIOInfo );

    static IDE_RC writePagesByBulkIO( idvSQL             *aStatistics,
                                      sdbDPathBuffInfo   *aDPathBuffInfo,
                                      sdbDPathBulkIOInfo *aDPathBulkIOInfo,
                                      scSpaceID          *aPrevSpaceID );

    static void setMaxDPathBuffPageCnt( UInt aBuffPageCnt ) { mMaxBuffPageCnt = aBuffPageCnt; } ;

    static IDE_RC dumpDPathBuffInfo( sdbDPathBuffInfo *aDPathBuffInfo );
    static IDE_RC dumpDPathBulkIOInfo( sdbDPathBulkIOInfo *aDPathBulkIOInfo );
    static IDE_RC dumpDPathBCB( sdbDPathBCB *aDPathBCB );

private:
    static IDE_RC flushBCBInDPathInfo(
                                idvSQL             * aStatistics,
                                sdbDPathBuffInfo   * aDPathBCBInfo,
                                sdbDPathBulkIOInfo * aDPathBulkIOInfo );

    static IDE_RC freeBCBInDPathInfo( sdbDPathBuffInfo   * aDPathBCBInfo );
    static IDE_RC freeBCBInList( smuList             * aBaseNode );

    static IDE_RC createFlushThread( sdbDPathBuffInfo* aDPathBCBInfo );
    static IDE_RC destFlushThread( sdbDPathBuffInfo* aDPathBCBInfo );

    static IDE_RC addNode4BulkIO( sdbDPathBulkIOInfo *aDPathBulkIOInfo,
                                  smuList            *aFlushNode);
    static IDE_RC allocBuffPage( idvSQL              *aStatistics,
                                 sdbDPathBuffInfo    *aDPathBuffInfo,
                                 void               **aPage );
    static IDE_RC freeBuffPage( void* aPage );

    static idBool isNeedBulkIO( sdbDPathBulkIOInfo *aDPathBulkIOInfo,
                                sdbDPathBCB        *aBCB );

private:
    static iduMemPool mBCBPool;
    static iduMemPool mPageBuffPool;
    static iduMemPool mFThreadPool;

    /* For Buffer Management */
    static iduMutex   mBuffMtx;
    /* Direct Path Buffer Pool이 가질수 있는 최대 버퍼 페이지 갯수 */
    static UInt       mMaxBuffPageCnt;
    /* 현재 Direct Path Buffer Pool에서 할당된 페이지의 갯수 */
    static UInt       mAllocBuffPageCnt;
};

#endif /* _O_SDB_DPATH_BUFFER_MGR_H_ */
