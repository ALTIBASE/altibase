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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer
 **********************************************************************/

#ifndef  _O_SDS_BUFFERMGR_H_
#define  _O_SDS_BUFFERMGR_H_  1

#include <idl.h>
#include <idu.h>

#include <smDef.h>
#include <smiDef.h>
#include <smuProperty.h>

#include <sdbBCB.h>
#include <sdbMPRKey.h>
#include <sdbBCBHash.h>
#include <sdbFlushMgr.h>
#include <sdsBufferMgrStat.h>

typedef struct sdsDiscardPageObj
{
    /* discard 조건 검사 함수 */
    sdsFiltFunc              mFilter;
    /* 통계정보 */
    idvSQL                  *mStatistics;
    /* mFilter수행에 필요한 환경 */
    void                    *mEnv;
}sdsDiscardPageObj;

typedef struct sdsPageOutObj
{
    /* page out 조건 검사 함수 */
    sdsFiltFunc              mFilter;
    /* mFilter수행에 필요한 환경 */
    void                    *mEnv;
    /* flush를 해야하는 BCB들을 모아둔 큐 */
    smuQueueMgr              mQueueForFlush;
    /* make free를 해야하는 BCB들을 모아둔 큐 */
    smuQueueMgr              mQueueForMakeFree;
}sdsPageOutObj;

typedef struct sdsFlushObj
{
    /* flush 조건 검사 함수 */
    sdsFiltFunc              mFilter;
    /* mFilter수행에 필요한 환경 */
    void                    *mEnv;
    /* BCB를 모아두는 큐 */
    smuQueueMgr              mQueue;
}sdsFlushObj;

typedef struct sdsBCBRange
{
    // mSpaceID에 속하면서
    // mStartPID 보다 크거나 같고,
    // mEndPID보다 작거나 같은 모든 pid를 범위에 속하도록한다.
    scSpaceID   mSpaceID;
    scPageID    mStartPID;
    scPageID    mEndPID;
}sdsBCBRange;


typedef iduLatch sdsHashChainsLatchHandle;


class sdsBufferMgr
{
public:
    static IDE_RC initialize( void );

    static IDE_RC destroy( void );

    static IDE_RC finalize( void );

    static void insertBCB( idvSQL     * aStatistics,
                           scSpaceID    aSpaceID, 
                           scPageID     aPageID,
                           sdsBCB     * aSBCB );

    static IDE_RC makeFreeBCB( idvSQL     * aStatistics,
                               sdsBCB     * aSBCB );

    static IDE_RC makeFreeBCBForce( idvSQL     * aStatistics,
                                    sdsBCB     * aSBCB );

    static IDE_RC moveUpPage( idvSQL      * aStatistics,
                              sdsBCB     ** aSBCB,      
                              sdbBCB      * aBCB,
                              idBool      * aIsCorruptRead, 
                              idBool      * aIsCorruptPage );  
    
    static IDE_RC moveDownPage( idvSQL     * aStatistics,
                                sdbBCB     * aBCB,
                                UChar      * aBuffer,
                                ULong        aExtentIndex,
                                UInt         aFrameIndex,
                                sdsBCB    ** aSBCB );

    static idBool filterAllBCBs( void * aSBCB,  void * aObj );
#if 0 //not used. 
    static idBool filterTBSBCBs( void * aSBCB,  void * aObj );
#endif
    static idBool filterRangeBCBs( void * aSBCB,  void * aObj );

    static IDE_RC discardPagesInRange( idvSQL    *aStatistics,
                                       scSpaceID  aSpaceID,
                                       scPageID   aStartPID,
                                       scPageID   aEndPID );
#if 0 //not used.
    static IDE_RC discardAllPages( idvSQL     *aStatistics );
#endif
    static IDE_RC pageOutAll( idvSQL        * aStatistics );
#if 0 //not used.
    static IDE_RC pageOutTBS( idvSQL        * aStatistics,
                              scSpaceID       aSpaceID);
#endif
    static IDE_RC pageOutInRange( idvSQL      * aStatistics,
                                  scSpaceID     aSpaceID,
                                  scPageID      aStartPID,
                                  scPageID      aEndPID ) ;

    static IDE_RC flushPagesInRange( idvSQL       * aStatistics,
                                     scSpaceID      aSpaceID,
                                     scPageID       aStartPID,
                                     scPageID       aEndPID);

    static IDE_RC flushDirtyPagesInCPList( idvSQL *aStatistics, 
                                           idBool aFlushAll );

    static IDE_RC moveUpbySinglePage( idvSQL     * aStatistics,
                                      sdsBCB    ** aSBCB, 
                                      sdbBCB     * aBCB,
                                      idBool     * aIsCorruptRead );

    static void getMinRecoveryLSN( idvSQL * aStatistics,
                                   smLSN  * aRet );

    static IDE_RC identify( idvSQL * aStatistics );
    /* sdsMeta */
    static inline IDE_RC restart( idBool aNeedRecovery );

    static inline ULong getFrameOffset ( UInt aIndex );

    static inline ULong getTableOffset ( UInt aSeq );

    /* sdsBufferArea */
    static inline idBool getTargetFlushExtentIndex( UInt *aExtIndex );
    
    static inline idBool hasExtentMovedownDone( void );
    /* sdsFile */
    static inline IDE_RC loadFileAttr( smiSBufferFileAttr * aFileAttr,
                                       UInt                 aOffset );

    static inline void getFileAttr( sdsFileNode        * aFileNode,
                                    smiSBufferFileAttr * aFileAttr );

    static inline void getFileNode( sdsFileNode  **aFileNode );

    static inline void makeMetaTable( idvSQL * aStatistics,  
                                      UInt aExtentIndex );


    static inline void dumpMetaTable(  const UChar  * aPage,
                                       SChar        * aOutBuf, 
                                       UInt           aOutSize );

    static inline IDE_RC syncAllSB( idvSQL * aStatistics ); 

    static inline IDE_RC repairFailureSBufferHdr( idvSQL * aStatistics,
                                                   smLSN  * aResetLogsLSN );

    static inline sdbCPListSet * getCPListSet( void);

    static inline sdbBCBHash * getSBufferHashTable( void );

    static inline sdsFile * getSBufferFile( void );

    static inline sdsMeta * getSBufferMeta( void );

    static inline sdsBufferArea * getSBufferArea( void );
    
    static inline sdsBufferMgrStat *getSBufferStat( void );

    static inline UInt getSBufferType( void );

    static inline IDE_RC findBCB( idvSQL     * aStatistics,
                                  scSpaceID    aSpaceID, 
                                  scPageID     aPageID,
                                  sdsBCB    ** aSBCB );

    static inline IDE_RC pageOutBCB( idvSQL     * aStatistics,
                                     sdsBCB     * aSBCB );

    static IDE_RC removeBCB( idvSQL     * aStatistics,
                             sdsBCB     * aSBCB );

    /* 서비스 상태를 설정/얻어오기 위한 함수 */
    static inline idBool isIdentifiable( void );
    static inline idBool isServiceable( void );
    static inline void setUnserviceable( void );
    static inline void setServiceable( void );
    static inline void setIdentifiable( void );

    /* 통계정보를 위한 함수 */
    static inline void applyGetPages( void );
    static inline void applyVictimWaits( void );
    static inline UInt getPageSize( void );
    static inline UInt getIOBSize( void );

private:

    static void getVictim( idvSQL     * aStatistics,
                           UInt         aIndex,
                           sdsBCB    ** aSBCB );

    static IDE_RC discardNoWaitModeFunc( sdsBCB    * aSBCB,
                                         void      * aObj );
    static IDE_RC discardPages( idvSQL        * aStatistics,
                                sdsFiltFunc     aFilter,
                                void          * aFiltAgr );

    static IDE_RC makePageOutTargetQueueFunc( sdsBCB  * aSBCB,
                                              void    * aObj );

    static IDE_RC makeFlushTargetQueueFunc( sdsBCB  * aSBCB,
                                            void    * aObj );
   
    static IDE_RC pageOut( idvSQL       * aStatistics,
                           sdsFiltFunc    aFilter,
                           void         * aFiltAgr );

    static IDE_RC flushPages( idvSQL            * aStatistics,
                              sdsFiltFunc         aFilter,
                              void              * aFiltAgr);

    static void setFrameInfoAfterReadPage( sdsBCB  * aSBCB, 
                                           sdbBCB  * aBCB,
                                           idBool    aChkOnlineTBS );
#ifdef DEBUG
    static IDE_RC validate( sdbBCB  * aBCB ); 
#endif
public :

private:
    /* SD_BUFFER_SIZE */
    static UInt                 mPageSize;

    static UInt                 mFrameCntInExtent;  
    /* Secondary Buffer가 사용가능한가 */
    static sdsSBufferServieStage mServiceStage;
    /* cache type */
    static UInt                 mCacheType;

    /* sdbCPListSet을 따로 가진다. */
    static sdbCPListSet         mCPListSet;

    static sdbBCBHash           mHashTable;

    static sdsBufferArea        mSBufferArea;

    static sdsFile              mSBufferFile;

    static sdsMeta              mMeta;
    /* Buffer Pool 관련 통계정보 */
    static sdsBufferMgrStat     mStatistics;
};

sdbCPListSet* sdsBufferMgr::getCPListSet()
{
    return &mCPListSet;
}

sdbBCBHash* sdsBufferMgr::getSBufferHashTable()
{
    return &mHashTable;
}

sdsFile* sdsBufferMgr::getSBufferFile()
{
    return &mSBufferFile;
}

sdsMeta* sdsBufferMgr::getSBufferMeta()
{
    return &mMeta;
}

sdsBufferArea* sdsBufferMgr::getSBufferArea()
{
    return &mSBufferArea;
}

sdsBufferMgrStat* sdsBufferMgr::getSBufferStat()
{
    return &mStatistics; 
}

UInt sdsBufferMgr::getSBufferType()
{
    return mCacheType;
}

idBool sdsBufferMgr::isIdentifiable()
{
    idBool sIsIdentifiable = ID_FALSE;

    if( mServiceStage == SDS_IDENTIFIABLE )
    {
        sIsIdentifiable = ID_TRUE;
    }

    return  sIsIdentifiable;
}

idBool sdsBufferMgr::isServiceable()
{
    idBool sIsServiceable = ID_FALSE;

    if( mServiceStage == SDS_SERICEABLE )
    {
        sIsServiceable = ID_TRUE;
    }

    return  sIsServiceable;
}

void sdsBufferMgr::setUnserviceable()
{
    mServiceStage = SDS_UNSERVICEABLE;     
}

void sdsBufferMgr::setServiceable()
{
    mServiceStage = SDS_SERICEABLE;     
}

void sdsBufferMgr::setIdentifiable()
{
    mServiceStage = SDS_IDENTIFIABLE;    

}
/* 통계정보를 위한 함수 */
void sdsBufferMgr::applyGetPages()
{
    mStatistics.applyGetPages(); 
}

void sdsBufferMgr::applyVictimWaits()
{
    mStatistics.applyVictimWaits(); 
}

UInt sdsBufferMgr::getPageSize()
{
    return mPageSize;
}

UInt sdsBufferMgr::getIOBSize()
{
    return mFrameCntInExtent;
}

/****************************************************************
 * Description : 
   aSpaceID   - [IN] : SpaceID
   aPageID    - [IN] : PageID
   aSBCB      - [OUT]: 해쉬에서 검색된 Secondary Buffer BCB
 ****************************************************************/
IDE_RC sdsBufferMgr::findBCB( idvSQL     * aStatistics,
                              scSpaceID    aSpaceID, 
                              scPageID     aPageID,
                              sdsBCB    ** aSBCB )
{
    sdsHashChainsLatchHandle  * sHashChainsHandle = NULL;
 
    sHashChainsHandle = mHashTable.lockHashChainsSLatch( aStatistics,
                                                         aSpaceID,
                                                         aPageID );

    IDE_TEST( mHashTable.findBCB( aSpaceID, aPageID, (void**)aSBCB )
              != IDE_SUCCESS ); 

    mHashTable.unlockHashChainsLatch( sHashChainsHandle );
    sHashChainsHandle = NULL;   

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sHashChainsHandle != NULL )
    {
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/****************************************************************
 * Description:BCB 를 free 상태로 만든다. 
 *  flush는 해야 하므로 Dirty는 변경하지 않음.
 *  aSBCB        [IN] : free 상태로 바꿀 BCB
 ****************************************************************/
IDE_RC sdsBufferMgr::pageOutBCB( idvSQL     * aStatistics,
                                 sdsBCB     * aSBCB )
{
    sdsHashChainsLatchHandle  * sHashChainsHandle = NULL;
    scSpaceID     sSpaceID; 
    scPageID      sPageID; 
    UInt          sState = 0;

    IDE_ASSERT( aSBCB != NULL );

    sSpaceID = aSBCB->mSpaceID; 
    sPageID  = aSBCB->mPageID; 
    
    sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                         sSpaceID,
                                                         sPageID );
    sState = 1;
    aSBCB->lockBCBMutex( aStatistics );
    sState = 2;

    /* 넘겨받은 BCB와 lock 잡은 BCB가 같아야 한다. */
    IDE_TEST_CONT( (( sSpaceID != aSBCB->mSpaceID ) ||
                    ( sPageID  != aSBCB->mPageID ) ) ,
                   SKIP_SUCCESS );

    switch( aSBCB->mState )
    {
        case SDS_SBCB_CLEAN:
            mHashTable.removeBCB( aSBCB );
            aSBCB->setFree();
            break;

        case SDS_SBCB_INIOB:
            aSBCB->mState = SDS_SBCB_OLD;
            mHashTable.removeBCB( aSBCB );
            break;  

        case SDS_SBCB_DIRTY:
            /* Secondary Buffer Flusher가 pageOut수행 */ 
            break;

        case SDS_SBCB_OLD:
        case SDS_SBCB_FREE:
            /* nothing to do */
            break;

        default:
            IDE_RAISE( ERROR_INVALID_BCD );
            break;
    }

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    sState = 1;
    aSBCB->unlockBCBMutex();
    sState = 0;
    mHashTable.unlockHashChainsLatch( sHashChainsHandle );
    sHashChainsHandle = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_INVALID_BCD );
    {
        ideLog::log( IDE_ERR_0,
                     "invalid bcd to pageOut \n"
                     "sSpaseID:%u\n"
                     "sPageID :%u\n"
                     "ID      :%u\n"
                     "spaseID :%u\n"
                     "pageID  :%u\n"
                     "state   :%u\n"
                     "CPListNo:%u\n"
                     "HashTableNo:%u\n",
                     sSpaceID,
                     sPageID,
                     aSBCB->mSBCBID,
                     aSBCB->mSpaceID,
                     aSBCB->mPageID,
                     aSBCB->mState,
                     aSBCB->mCPListNo,
                     aSBCB->mHashBucketNo );
        IDE_DASSERT(0);
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            aSBCB->unlockBCBMutex();
        case 1:
            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;
        break;
    }
    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 ****************************************************************/
IDE_RC sdsBufferMgr::restart( idBool aIsRecoveryMode )
{
    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP_SUCCESS );
    
    IDE_TEST( mMeta.buildBCB( NULL/* aStatistics*/, aIsRecoveryMode ) 
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 ****************************************************************/
ULong sdsBufferMgr::getFrameOffset ( UInt aIndex )
{
    return mMeta.getFrameOffset( aIndex );
}

ULong sdsBufferMgr::getTableOffset ( UInt aSeq )
{
    return mMeta.getFrameOffset( aSeq );
}

/****************************************************************
 * Description : Secondary Flusher 가 flush 해야할 Extent가 있는지 확인 
 ****************************************************************/
idBool sdsBufferMgr::getTargetFlushExtentIndex( UInt *aExtIndex )
{
    return mSBufferArea.getTargetFlushExtentIndex( aExtIndex );
}

/****************************************************************
 * Description :
 ****************************************************************/
idBool sdsBufferMgr::hasExtentMovedownDone()
{
    return mSBufferArea.hasExtentMovedownDone();
} 

/****************************************************************
 * Description :
 ****************************************************************/
void sdsBufferMgr::makeMetaTable( idvSQL * aStatistics, UInt aExtentIndex )
{
    mMeta.makeMetaTable( aStatistics,  aExtentIndex );
}

/****************************************************************
 * Description :
 ****************************************************************/
void sdsBufferMgr::dumpMetaTable(  const UChar  * aPage,
                                   SChar        * aOutBuf, 
                                   UInt           aOutSize )
{
    (void)mMeta.dumpMetaTable( aPage, aOutBuf, aOutSize );
}

/****************************************************************
 * Description :
 ****************************************************************/
IDE_RC sdsBufferMgr::loadFileAttr( smiSBufferFileAttr     * aFileAttr,
                                   UInt                     aOffset )
{
    IDE_TEST( mSBufferFile.load( aFileAttr,
                                 aOffset )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 
/****************************************************************
 * Description :
 ****************************************************************/
void sdsBufferMgr::getFileAttr( sdsFileNode           * aFileNode,
                                smiSBufferFileAttr    * aFileAttr )
{
    mSBufferFile.getFileAttr( aFileNode, aFileAttr );
}

void sdsBufferMgr::getFileNode( sdsFileNode  **aFileNode )
{
    mSBufferFile.getFileNode( aFileNode );
}
/****************************************************************
 * Description :
 ****************************************************************/
IDE_RC sdsBufferMgr::syncAllSB( idvSQL * aStatistics )
{
    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP_SUCCESS );
    
    IDE_TEST( mSBufferFile.syncAllSB( aStatistics ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :  
 *  aResetLogsLSN  [IN] - 불완전복구시 설정되는 ResetLogsLSN
 *                        완전복구 시에는 NULL
 ****************************************************************/
IDE_RC sdsBufferMgr::repairFailureSBufferHdr( idvSQL * aStatistics,
                                              smLSN  * aResetLogsLSN )
{
    IDE_TEST( mSBufferFile.repairFailureSBufferHdr( aStatistics, 
                                                    aResetLogsLSN ) 
              != IDE_SUCCESS );
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif
