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
 *
 * $Id: sdpstCache.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Segment Runtime Cache에 대한
 * 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_CACHE_H_
# define _O_SDPST_CACHE_H_ 1

# include <sdpstDef.h>

class sdpstCache
{
public:

    static IDE_RC initialize( sdpSegHandle * aSegHandle,
                              scSpaceID      aSpaceID,
                              sdpSegType     aSegType,
                              smOID          aObjectID,
                              UInt           aIndexID );

    static IDE_RC destroy( sdpSegHandle * aSegHandle );

    static void allocHintPageArray( sdpstSegCache * aSegCache );

    static void clearItHint( void * aSegCache );

    static IDE_RC prepareExtendExtOrWait( idvSQL            * aStatistics,
                                       sdpstSegCache     * aSegCache,
                                       idBool            * aDoExtendExt );

    static IDE_RC completeExtendExtAndWakeUp( idvSQL        * aStatistics,
                                           sdpstSegCache * aSegCache );

    static inline smOID getTableOID( sdpstSegCache   * aSegCache );

    static void setItHintIfLT( idvSQL            * aStatistics,
                               sdpstSegCache     * aSegCache,
                               sdpstSearchType     aSearchType,
                               sdpstStack        * aStack,
                               idBool            * aItHintFlag );

    static void setItHintIfGT( idvSQL            * aStatistics,
                               sdpstSegCache     * aSegCache,
                               sdpstSearchType     aSearchType,
                               sdpstStack        * aStack );

    static void copyHWM( idvSQL            * aStatistics,
                        sdpstSegCache     * aSegCache,
                        sdpstWM           * aWM );

    static void copyItHint( idvSQL            * aStatistics,
                            sdpstSegCache     * aSegCache,
                            sdpstSearchType     aSearchType,
                            sdpstStack        * aHintStack );
    static void initItHint( idvSQL            * aStatistics,
                            sdpstSegCache     * aSegCache,
                            scPageID            aRtBMP,
                            scPageID            aItBMP );


    static inline void setUpdateHint4Page( sdpstSegCache * aSegCache,
                                           idBool          aItHintFlag );

    static inline void setUpdateHint4Slot( sdpstSegCache * aSegCache,
                                           idBool          aItHintFlag );

    static sdpstStack getMinimumItHint( idvSQL        * aStatistics,
                                        sdpstSegCache * aSegCache );

    static inline idBool needToUpdateItHint( sdpstSegCache * aSegCache,
                                         sdpstSearchType aSearchType );

    static void getHintPosInfo( idvSQL          * aStatistics,
                                void            * aSegCache,
                                sdpHintPosInfo  * aHintPosInfo );
   
    static IDE_RC getFmtPageCnt( idvSQL        * aStatistics,
                                 scSpaceID       aSpaceID,
                                 sdpSegHandle  * aSegHandle,
                                 ULong         * aFmtPageCnt );

    static IDE_RC getSegCacheInfo( idvSQL             * aStatistics,
                                   sdpSegHandle       * aSegHandle,
                                   sdpSegCacheInfo    * aSegCacheInfo );

    static void getHintDataPage( idvSQL            * aStatistics,
                                 sdpstSegCache     * aSegCache,
                                 scPageID          * aHintDataPID );

    static void setHintDataPage( idvSQL            * aStatistics,
                                 sdpstSegCache     * aSegCache,
                                 scPageID            aHintDataPID );

    static void dump( sdpstSegCache * aSegCache );

    static IDE_RC setLstAllocPage4AllocPage( idvSQL          * aStatistics,
                                             sdpSegHandle    * aSegHandle,
                                             scPageID          aLstAllocPID,
                                             ULong             aLstAllocSeqNo );

    static IDE_RC setLstAllocPage4DPath( idvSQL          * aStatistics,
                                         sdpSegHandle    * aSegHandle,
                                         scPageID          aLstAllocPID,
                                         ULong             aLstAllocSeqNo );

    static IDE_RC setLstAllocPage( idvSQL          * aStatistics,
                                   sdpSegHandle    * aSegHandle,
                                   scPageID          aLstAllocPID,
                                   ULong             aLstAllocSeqNo );

    static inline void setMetaPageID( void       * aSegCache,
                                      scPageID     aMetaPageID );
    static inline scPageID getMetaPageID( void   * aSegCache );


    static inline void lockHWM4Read( idvSQL          * aStatistics,
                                    sdpstSegCache   * aSegCache );

    static inline void lockHWM4Write( idvSQL          * aStatistics,
                                     sdpstSegCache   * aSegCache );

    static inline void unlockHWM( sdpstSegCache   * aSegCache );

    static inline void lockLstAllocPage( idvSQL          * aStatistics,
                                         sdpstSegCache   * aSegCache );

    static inline void unlockLstAllocPage( sdpstSegCache   * aSegCache );

private:
    static inline idBool isOnExtend( sdpstSegCache * aSegCache );

    static inline void  lockExtendExt( idvSQL         * aStatistics,
                                    sdpstSegCache  * aSegCache );

    static inline void  unlockExtendExt( sdpstSegCache  * aSegCache );


    static inline void lockSlotHint4Read( idvSQL        * aStatistics,
                                          sdpstSegCache * aSegCache );

    static inline void lockSlotHint4Write( idvSQL        * aStatistics,
                                           sdpstSegCache * aSegCache );

    static inline void unlockSlotHint( sdpstSegCache * aSegCache );

    static inline void lockPageHint4Read( idvSQL        * aStatistics,
                                          sdpstSegCache * aSegCache );

    static inline void lockPageHint4Write( idvSQL        * aStatistics,
                                           sdpstSegCache * aSegCache );

    static inline void unlockPageHint( sdpstSegCache * aSegCache );

};


// SearchType에 따라서 Hint flag를 반환한다.
inline idBool sdpstCache::needToUpdateItHint( sdpstSegCache * aSegCache,
                                          sdpstSearchType aSearchType )
{
    IDE_ASSERT( aSegCache != NULL );
    if ( aSearchType == SDPST_SEARCH_NEWPAGE )
    {
        return aSegCache->mHint4Page.mUpdateHintItBMP;
    }
    else
    {
        return aSegCache->mHint4Slot.mUpdateHintItBMP;
    }
}

inline idBool sdpstCache::isOnExtend( sdpstSegCache * aSegCache )
{
    return aSegCache->mOnExtend;
}

// Extend Extent Mutex 획득
inline void  sdpstCache::lockExtendExt( idvSQL         * aStatistics,
                                     sdpstSegCache  * aSegCache )
{
    IDE_ASSERT( aSegCache->mExtendExt.lock( aStatistics ) == IDE_SUCCESS );
}

// Extend Extent Mutex 해제
inline void  sdpstCache::unlockExtendExt( sdpstSegCache  * aSegCache )
{
    IDE_ASSERT( aSegCache->mExtendExt.unlock() == IDE_SUCCESS );
}

// Table OID를 반환한다.
inline smOID sdpstCache::getTableOID( sdpstSegCache   * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    return aSegCache->mCommon.mTableOID;
}

// free page가 발생하였기 때문에 update hint flag를 설정한다.
inline void sdpstCache::setUpdateHint4Page( sdpstSegCache * aSegCache,
                                            idBool          aItHintFlag )
{
    IDE_ASSERT( aSegCache != NULL );
    aSegCache->mHint4Page.mUpdateHintItBMP = aItHintFlag;
}

// free page가 발생하였기 때문에 Update Hint Flag를 설정한다.
inline void sdpstCache::setUpdateHint4Slot( sdpstSegCache * aSegCache,
                                            idBool          aItHintFlag )
{
    IDE_ASSERT( aSegCache != NULL );
    aSegCache->mHint4Slot.mUpdateHintItBMP = aItHintFlag;
}

inline void sdpstCache::setMetaPageID( void      * aSegCache,
                                       scPageID    aMetaPageID )
{
    ((sdpstSegCache*)aSegCache)->mCommon.mMetaPID = aMetaPageID;
}

inline scPageID sdpstCache::getMetaPageID( void * aSegCache )
{
    return ((sdpstSegCache*)aSegCache)->mCommon.mMetaPID;
}

inline void sdpstCache::lockSlotHint4Read( idvSQL        * aStatistics,
                                           sdpstSegCache * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mHint4Slot.mLatch4Hint).lockRead( aStatistics,
                        NULL );
}

inline void sdpstCache::lockSlotHint4Write( idvSQL        * aStatistics,
                                            sdpstSegCache * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mHint4Slot.mLatch4Hint).lockWrite( aStatistics,
                         NULL );
}

inline void sdpstCache::unlockSlotHint( sdpstSegCache * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mHint4Slot.mLatch4Hint).unlock( );
}

inline void sdpstCache::lockPageHint4Read( idvSQL        * aStatistics,
                                           sdpstSegCache * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mHint4Page.mLatch4Hint).lockRead( aStatistics,
                        NULL );
}

inline void sdpstCache::lockPageHint4Write( idvSQL        * aStatistics,
                                            sdpstSegCache * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mHint4Page.mLatch4Hint).lockWrite( aStatistics,
                         NULL );
}

inline void sdpstCache::unlockPageHint( sdpstSegCache * aSegCache )

{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mHint4Page.mLatch4Hint).unlock( );
}

inline void sdpstCache::lockHWM4Read( idvSQL          * aStatistics,
                                     sdpstSegCache   * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mLatch4WM).lockRead( aStatistics,  NULL );
}

inline void sdpstCache::lockHWM4Write( idvSQL          * aStatistics,
                                      sdpstSegCache   * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mLatch4WM).lockWrite( aStatistics, NULL );
}

inline void sdpstCache::unlockHWM( sdpstSegCache   * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    (aSegCache->mLatch4WM).unlock( );
}

inline void sdpstCache::lockLstAllocPage( idvSQL          * aStatistics,
                                          sdpstSegCache   * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    IDE_ASSERT( aSegCache->mMutex4LstAllocPage.lock( aStatistics )
                == IDE_SUCCESS );
}

inline void sdpstCache::unlockLstAllocPage( sdpstSegCache   * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    IDE_ASSERT( aSegCache->mMutex4LstAllocPage.unlock() == IDE_SUCCESS );
}

#endif // _O_SDPST_CACHE_H_
