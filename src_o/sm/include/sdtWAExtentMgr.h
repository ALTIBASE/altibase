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
 * $Id
 *
 * Sort 및 Hash를 하는 공간으로, 최대한 동시성 처리 없이 동작하도록 구현된다.
 * 따라서 SortArea, HashArea를 아우르는 의미로 사용된다. 즉
 * SortArea, HashArea는WorkArea에 속한다.
 *
 * 원래 PrivateWorkArea를 생각했으나, 첫번째 글자인 P가 여러기지로 사용되기
 * 때문에(예-Page); WorkArea로 변경하였다.
 * 일반적으로 약자 및 Prefix로 WA를 사용하지만, 몇가지 예외가 있다.
 * WCB - BCB와 역할이 같기 때문에, 의미상 WACB보다 WCB로 수정하였다.
 * WPID - WAPID는 좀 길어서 WPID로 줄였다.
 *
 **********************************************************************/

#ifndef _O_SDT_WA_EXTENT_MGR_H_
#define _O_SDT_WA_EXTENT_MGR_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smiDef.h>
#include <sdtDef.h>
#include <iduLatch.h>
#include <smuMemStack.h>

class sdtWAExtentMgr
{
public:
    /******************************************************************
     * WorkArea
     ******************************************************************/
    static IDE_RC initializeStatic();

    static IDE_RC prepareCachedFreeNExts( idvSQL  * aStatistics );
    static IDE_RC resizeWAExtentPool( ULong aNewSize );
    static IDE_RC destroyStatic();

    static UInt getFreeWAExtentCount()
    {
        return mFreeExtentPool.getTotItemCnt();
    };
    static UInt getWAExtentCount() { return mWAExtentCount; }
    static UInt getMaxWAExtentCount() { return mMaxWAExtentCount; }

    static void updateMaxWAExtentCount()
    {
        SInt sCurUsedExtentCount = (SInt)getWAExtentCount() - (SInt)getFreeWAExtentCount();

        if ( sCurUsedExtentCount > (SInt)getMaxWAExtentCount() )
        {
            mMaxWAExtentCount = (UInt)sCurUsedExtentCount;
        }
    };
    static ULong getInitTotalWASize()
    {
        ULong sInitTotalWASize = smuProperty::getInitTotalWASize();
        ULong sMaxTotalWASize  = smuProperty::getMaxTotalWASize();

        /* 둘 중 작은값으로 반환한다.
         * 변경 될 수 있으므로 변수로 받아서 사용한다.
         * 비교와 반환 만 같으면 된다. (비교시에 작았던 값이 return전에 크게 변경 되어 반환하면 안된다.)
         * 변수로 받은 후에 return 하기 전에 property값이 변경 되는 것은 상관 없다. */
        return ( sInitTotalWASize < sMaxTotalWASize) ? sInitTotalWASize : sMaxTotalWASize;
    }
    static void lock()
    {
        (void)mMutex.lock( NULL );
    }
    static void unlock()
    {
        (void) mMutex.unlock();
    }

    static UInt calcWAExtentCount( ULong aWorkAreaSize )
    {
        return (UInt)(( aWorkAreaSize + SDT_WAEXTENT_SIZE -1 ) / SDT_WAEXTENT_SIZE );
    }

    /******************************************************************
     * Segment
     ******************************************************************/
    static IDE_RC initWAExtents( idvSQL            * aStatistics,
                                 smiTempTableStats * aStatsPtr,
                                 sdtWAExtentInfo   * aWAExtentInfo,
                                 UInt                aInitWAExtentCount );

    static IDE_RC allocWAExtents( sdtWAExtentInfo  * aWAExtentInfo,
                                  UInt               aGetWAExtentCount );

    static IDE_RC memAllocWAExtent( sdtWAExtentInfo * aWAExtentInfo );

    static IDE_RC freeWAExtents( sdtWAExtentInfo * aWAExtentInfo );
    static IDE_RC freeAllNExtents( scSpaceID           aSpaceID,
                                   sdtNExtFstPIDList * aNExtFstPIDList );
    static IDE_RC allocFreeNExtent( idvSQL            * aStatistics,
                                    smiTempTableStats * aStatsPtr,
                                    scSpaceID           aSpaceID,
                                    sdtNExtFstPIDList * aNExtFstPIDList );

public:
    /***********************************************************
     * FixedTable용 함수들 ( for X$TEMPINFO )
     ***********************************************************/
    static IDE_RC buildTempInfoRecord( void                * aHeader,
                                       iduFixedTableMemory * aMemory );

    static void dumpNExtentList( sdtNExtFstPIDList & aNExtentList,
                                 SChar             * aOutBuf,
                                 UInt                aOutSize );

private:
    static UInt                  mWAExtentCount;
    static UInt                  mMaxWAExtentCount;
    static iduMutex              mMutex;
    static iduStackMgr           mFreeExtentPool;    /*빈 Extent를 관리하는 곳 */
    static iduMemPool            mNExtentArrPool;    /*빈 NExtMgr을 관리하는 곳 */
};

#endif //_O_SDT_WA_EXTENT_MGR
