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

#ifndef  _O_SDS_BUFFER_MGR_STAT_H_
#define  _O_SDS_BUFFER_MGR_STAT_H_ 1

#include <idl.h>
#include <idu.h>
#include <idv.h>
#include <smDef.h>
#include <sdbDef.h>
#include <sdpDef.h>
#include <sdsDef.h>
#include <sdbBCB.h>
#include <sdsBufferArea.h>

class sdsBufferMgrStat
{
public:
    inline void initialize( sdsBufferArea * aBufferArea, 
                            sdbBCBHash    * aHashTable,
                            sdbCPListSet  * aCPListSet );

    inline void applyGetPages( void );

    inline void applyReadPages( ULong  aChecksumTime, 
                                ULong  aReadTime );

    inline void applyWritePages( ULong  aChecksumTime, 
                                 ULong  aWriteTime );

    inline void applyMultiReadPages( ULong  aChecksumTime,
                                     ULong  aReadTime,
                                     ULong  aReadPageCount );

    inline void applyVictimWaits();

    inline void applyBeforeMultiReadPages( idvSQL *aStatistics );
    inline void applyAfterMultiReadPages( idvSQL *aStatistics );

    inline void applyBeforeSingleReadPage( idvSQL *aStatistics );
    inline void applyAfterSingleReadPage( idvSQL *aStatistics );

    inline void applyBeforeSingleWritePage( idvSQL *aStatistics );
    inline void applyAfterSingleWritePage( idvSQL *aStatistics );
    inline void buildRecord( void );
    
public:
    inline sdsBufferMgrStatData  *getStatData( void );
private:
    inline SDouble getHitRatio( ULong aGetFixPages, ULong aGetPages );
public:

private:
    sdsBufferArea  * mBufferArea; 
    sdbBCBHash     * mHashTable;
    sdbCPListSet   * mCPListSet;

    /* BufferMgr Statistic 전체의 자료구조. */ 
    sdsBufferMgrStatData  mBufferStatistics;
};

/***********************************************************************
 * Description : 생성자
 ***********************************************************************/
void sdsBufferMgrStat::initialize( sdsBufferArea * aBufferArea, 
                                   sdbBCBHash    * aHashTable,
                                   sdbCPListSet  * aCPListSet )
{
    mBufferArea = aBufferArea;
    mHashTable  = aHashTable;
    mCPListSet  = aCPListSet;    

    mBufferStatistics.mBufferPages = 0;
    mBufferStatistics.mHashBucketCount = 0;
    mBufferStatistics.mHashChainLatchCount = 0;
    mBufferStatistics.mCheckpointListCount = 0;
    mBufferStatistics.mHashPages = 0;
    mBufferStatistics.mMovedoweIngPages = 0;
    mBufferStatistics.mMovedownDonePages = 0;
    mBufferStatistics.mFlushIngPages = 0;
    mBufferStatistics.mFlushDonePages = 0;
    mBufferStatistics.mCheckpointListPages = 0;
    mBufferStatistics.mGetPages = 0;
    mBufferStatistics.mReadPages = 0;
    mBufferStatistics.mReadTime = 0;
    mBufferStatistics.mReadChecksumTime = 0;
    mBufferStatistics.mWritePages = 0;
    mBufferStatistics.mWriteTime = 0;
    mBufferStatistics.mWriteChecksumTime = 0;
    mBufferStatistics.mHitRatio = 0;
    mBufferStatistics.mVictimWaits = 0;
    mBufferStatistics.mMPRChecksumTime = 0;
    mBufferStatistics.mMPRReadTime = 0;
    mBufferStatistics.mMPRReadPages = 0;
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
sdsBufferMgrStatData  * sdsBufferMgrStat::getStatData()
{
    return &mBufferStatistics;
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdsBufferMgrStat::applyBeforeMultiReadPages( idvSQL *aStatistics )
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_MULTI_PAGE_READ,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_BEGIN_WAIT_EVENT( aStatistics, &sWeArgs );
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdsBufferMgrStat::applyAfterMultiReadPages( idvSQL *aStatistics )
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_MULTI_PAGE_READ,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdsBufferMgrStat::applyBeforeSingleReadPage( idvSQL *aStatistics )
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_SINGLE_PAGE_READ,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_BEGIN_WAIT_EVENT( aStatistics, &sWeArgs );
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdsBufferMgrStat::applyAfterSingleReadPage( idvSQL *aStatistics )
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_SINGLE_PAGE_READ,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdsBufferMgrStat::applyBeforeSingleWritePage( idvSQL *aStatistics )
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_SINGLE_PAGE_WRITE,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_BEGIN_WAIT_EVENT( aStatistics, &sWeArgs );
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdsBufferMgrStat::applyAfterSingleWritePage( idvSQL *aStatistics )
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_SINGLE_PAGE_WRITE,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
}

/* BUG-21307: VS6.0에서 Compile Error발생.
 *
 * ULong이 double로 casting시 win32에서 에러 발생 */
/***********************************************************************
 * Description : 
 ***********************************************************************/
SDouble sdsBufferMgrStat::getHitRatio( ULong aGetPages,
                                       ULong aReadPages )
{
    SDouble sGetPages = UINT64_TO_DOUBLE( aGetPages );
    SDouble sReadPage = UINT64_TO_DOUBLE( aReadPages );

    return ( sReadPage / sGetPages ) * 100.0;
}

/***********************************************************************
 * Description : Get Page 통계정보를 갱신한다.
 ***********************************************************************/
void sdsBufferMgrStat::applyGetPages()
{
    mBufferStatistics.mGetPages++;
}

/***********************************************************************
 * Description :  Read Page 통계정보를 갱신한다.
 ***********************************************************************/
void sdsBufferMgrStat::applyReadPages( ULong  aChecksumTime, 
                                       ULong  aReadTime )
{
    mBufferStatistics.mReadPages++;
    mBufferStatistics.mReadTime += aReadTime;
    mBufferStatistics.mReadChecksumTime += aChecksumTime;
}

/***********************************************************************
 * Description : Write Page 통계정보를 갱신한다.
 ***********************************************************************/
void sdsBufferMgrStat::applyWritePages( ULong  aChecksumTime, 
                                        ULong  aWriteTime )
{
    mBufferStatistics.mWritePages++;
    mBufferStatistics.mWriteTime += aWriteTime;
    mBufferStatistics.mWriteChecksumTime += aChecksumTime;
}

/***********************************************************************
 * Description : Multi Read Page 통계정보를 갱신한다.
 ***********************************************************************/
void sdsBufferMgrStat::applyMultiReadPages( ULong  aChecksumTime,
                                            ULong  aReadTime,
                                            ULong  aReadPageCount )
{
    mBufferStatistics.mMPRReadPages += aReadPageCount;
    mBufferStatistics.mMPRReadTime  += aReadTime;
    mBufferStatistics.mMPRChecksumTime += aChecksumTime;
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdsBufferMgrStat::applyVictimWaits()
{
    mBufferStatistics.mVictimWaits++;
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdsBufferMgrStat::buildRecord()
{
    UInt sMovedoweIngPages  = 0;
    UInt sMovedownDonePages = 0;
    UInt sFlushIngPages     = 0; 
    UInt sFlushDonePages    = 0; 

    mBufferStatistics.mBufferPages         = mBufferArea->getBCBCount();

    mBufferStatistics.mHashBucketCount     = mHashTable->getBucketCount();
    mBufferStatistics.mHashChainLatchCount = mHashTable->getLatchCount();
    mBufferStatistics.mHashPages           = mHashTable->getBCBCount();

    mBufferArea->getExtentCnt( &sMovedoweIngPages,
                               &sMovedownDonePages,
                               &sFlushIngPages,
                               &sFlushDonePages );

    mBufferStatistics.mMovedoweIngPages    = sMovedoweIngPages;
    mBufferStatistics.mMovedownDonePages   = sMovedownDonePages;
    mBufferStatistics.mFlushIngPages       = sFlushIngPages;
    mBufferStatistics.mFlushDonePages      = sFlushDonePages;
    
    mBufferStatistics.mCheckpointListPages = mCPListSet->getTotalBCBCnt();
    mBufferStatistics.mCheckpointListCount = mCPListSet->getListCount();

    if( mBufferStatistics.mGetPages == 0 )
    {
        mBufferStatistics.mHitRatio = 0;
    }
    else
    {
        mBufferStatistics.mHitRatio = getHitRatio( mBufferStatistics.mGetPages, 
                                                   mBufferStatistics.mReadPages );
    }
}

#endif  // _O_SDS_BUFFER_MGR_STAT_H_                 
