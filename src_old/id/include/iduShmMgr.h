/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDU_SHM_MGR_H_)
#define _O_IDU_SHM_MGR_H_ 1

#include <iduProperty.h>
#include <idTypes.h>
#include <iduMemDefs.h>
#include <iduShmDef.h>
#include <iduVLogShmMgr.h>
#include <iduShmChunkMgr.h>
#include <iduVersion.h>

#ifdef DEBUG
#define MEMORY_ASSERT
#endif

#ifdef MEMORY_ASSERT
/*  Used to detect buffer overflow. */
extern vULong const gFence;
#endif

typedef void (*idmShmWalker)( void* aPtr, UInt aSize, idBool * aPrvBlockState );

class iduShmMgr
{
    friend class iduShmChunkMgr;
    friend class iduVLogShmMgr;
    friend class iduShmDump;

public:
    /* 할당된 Segment Header Array */
    static iduShmHeader      ** mArrSegment;
    static SInt                 mOffsetBitSize4ShmAddr;
    static UInt                 mMaxBlockSize;
    /* Daemon Process가 Startup 된 시간 */
    static struct timeval       mStartUpTime;

private:
    /* System Segment Header */
    static iduShmSSegment     * mSSegment;
    /* Attach된 Segment의 갯수 */
    static ULong                mAttachSegCnt;

    static SInt                 mMSBit4FF[];
    /* Shutdown시 Shm을 어떻게(Destroy or Detach) 처리할 것인가 */
    static idBool               mIsShmAreaFreeAtShutdown;

    /* BUG-40895 Cannot destroy the shared memory segments which is created as other user */
    static uid_t                mUID;

public:
    static IDE_RC initialize( iduShmProcType aProcType );
    static IDE_RC destroy( idBool aIsNormalShutdown );

    static IDE_RC attach( iduShmProcType aProcType );
    static IDE_RC attach4Clean( iduShmProcType aProcType, UInt aShmDBKey );
    static IDE_RC detach();

    static IDE_RC checkShmAndClean( iduShmProcType aProcType,
                                    idBool       * aIsValid );

    static IDE_RC allocMem( idvSQL               * aStatistics,
                            iduShmTxInfo         * aShmTxInfo,
                            iduMemoryClientIndex   aIndex,
                            UInt                   aSize,
                            void                ** aAllocPtr );

    static IDE_RC allocMemWithoutUndo( idvSQL               * aStatistics,
                                       iduShmTxInfo         * aShmTxInfo,
                                       iduMemoryClientIndex   aIndex,
                                       UInt                   aSize,
                                       void                ** aAllocPtr );

    static IDE_RC reallocMem( idvSQL               * aStatistics,
                              iduShmTxInfo         * aShmTxInfo,
                              iduMemoryClientIndex   aIndex,
                              void                 * aPtr,
                              UInt                   aSize,
                              void                ** aAllocPtr );

    static IDE_RC allocAlignMem( idvSQL               * aStatistics,
                                 iduShmTxInfo         * aShmTxInfo,
                                 iduMemoryClientIndex   aIndex,
                                 UInt                   aSize,
                                 UInt                   aAlignSize,
                                 void                ** aAllocPtr );

    static IDE_RC freeMem( idvSQL         * aStatistics,
                           iduShmTxInfo   * aShmTxInfo,
                           idrSVP         * aSavepoint,
                           void           * aPtr );

    static IDE_RC attachSeg( UInt aSegID, idBool aIsCleanPurpose );

    static idShmAddr getAddr( void* aAllocPtr );
    static UInt getBlockSize( void* aDataPtr );
    static iduShmBlockHeader * getBlockPtrByAddr( idShmAddr aBlockAddr );

    static inline void setMetaBlockAddr( iduShmMetaBlockType aMetablockType,
                                         idShmAddr           aMetaBlockAddr );

    static inline idShmAddr getMetaBlockAddr( iduShmMetaBlockType aMetablockType );

    static inline void getStatistics( iduShmStatistics * aShmTxInfo );

    static UInt getOverhead();

    static void checkHeap( void * aShmSSegment );

    static SChar* getPtrOfAddr( idShmAddr aAddr );
    static inline SChar* getPtrOfAddrCheck( idShmAddr aAddr );
    static inline iduShmProcMgrInfo * getProcMgrInfo();

    static inline void setShmCleanModeAtShutdown( iduShmProcInfo * aProcInfo );
    static inline idBool isShmAreaFreeAtShutdown();

    static inline iduShmHeader * getSegShmHeader( UInt aSegID );

    static inline void dump( void * aBlock );
    static void cleanAllShmArea( iduShmProcType aProcType, UInt aShmDBKey );

    static inline ULong getTotalSegSize();
    static inline iduShmSSegment* getSysSeg() { return mSSegment; }

    static IDE_RC validateShmMgr( idvSQL         * aStatistics,
                                  iduShmTxInfo   * aShmTxInfo );
    static UInt getMaxBlockSize();

    static inline void setShmAddrByAOP( idShmAddr   aNewShmAddr,
                                        idShmAddr * aTgtShmAddr )
    {
#if defined(COMPILE_64BIT)
        idCore::acpAtomicSet64( aTgtShmAddr, aNewShmAddr );
#else
        idCore::acpAtomicSet32( aTgtShmAddr, aNewShmAddr );
#endif
    }

    static IDE_RC eraseShmAreaAndPrint();

    static inline void setNormalShutdown( idBool aIsNormalShutdown )
    {
        getSysSeg()->mIsNormalShutdown = aIsNormalShutdown;
    }

private:
    static IDE_RC registerNewSeg( iduShmTxInfo   * aShmTxInfo,
                                  iduSCH         * aNewSCH,
                                  ULong          * aNewFreeSlotIdx );

    static void registerOldSeg( iduSCH * aNewSCH, ULong aSegID );

    static inline IDE_RC initLtArea( ULong aMaxSegCnt );
    static inline IDE_RC destLtArea();

    static IDE_RC createSSegment( iduShmTxInfo   * aShmTxInfo,
                                  UInt             aSize4DataSeg,
                                  ULong            aMaxSegCnt );

    static IDE_RC dropSSegment( idBool aIsCleanPurpose );

    static IDE_RC createDSegment( idvSQL        * aStatistics,
                                  iduShmTxInfo  * aShmTxInfo,
                                  iduShmType      aShmType,
                                  UInt            aChunkSize );

    static IDE_RC dropDSegment( iduShmHeader * aSegHeader );
    static IDE_RC dropAllDSegment();

    static inline SInt getLSBit( SInt aVal );
    static inline SInt getMSBit( SInt aVal );
    static inline idBool isBlockFree( const iduShmBlockHeader *aBlock );
    static inline idBool isBlockUsed( const iduShmBlockHeader *aBlock );
    static inline IDE_RC setBlockFree( iduShmTxInfo      * aShmTxInfo,
                                       iduShmBlockHeader * aBlock );

    static inline IDE_RC setBlockUsed( iduShmTxInfo      * aShmTxInfo,
                                       iduShmBlockHeader * aBlock );

    static inline idBool isPrevBlockFree( const iduShmBlockHeader *aBlock );
    static inline IDE_RC setPrevBlockFree( iduShmTxInfo      * aShmTxInfo,
                                           iduShmBlockHeader * aBlock );

    static inline IDE_RC setPrevBlockUsed( iduShmTxInfo      * aShmTxInfo,
                                           iduShmBlockHeader * aBlock );

    static inline iduShmBlockHeader* getBlockFromDataPtr( const void* aPtr );
    static inline void* getDataPtr4Block( const iduShmBlockHeader *aBlock );
    static inline void* getDataPtr4Offset( const void* aPtr, UInt aOffset );
    static inline iduShmBlockHeader* getPrevBlock( const iduShmBlockHeader *aBlock );
    static inline iduShmBlockHeader* getNextBlock( const iduShmBlockHeader *aBlock );
    static inline IDE_RC linkAndGetBlockNext( iduShmTxInfo            * aShmTxInfo,
                                              const iduShmBlockHeader * aBlock,
                                              iduShmBlockHeader      ** aNxtBlock);
    static inline IDE_RC setBlkAndPrvOfNxtBlkFree( iduShmTxInfo      * aShmTxInfo,
                                                   iduShmBlockHeader * aBlock );
    static inline IDE_RC setBlkUsedAndPrvOfNxtBlkUsed( iduShmTxInfo      * aShmTxInfo,
                                                       iduShmBlockHeader * aBlock );
    static inline UInt alignUp( UInt aSize, UInt aAlignSize );
    static inline UInt alignDown( UInt aSize, UInt aAlignSize );
    static inline SChar* alignUpPtr( const void* aPtr, UInt aAlignSize );
    static inline UInt adjustRequestSize( UInt aSize, UInt aAlignSize );
    static inline void getMatrixIdx4Ins( UInt   aSize,
                                         UInt * aFstLvlIdx,
                                         UInt * aSndLvlIdx );

    static inline void searchMapping( UInt   aSize,
                                      UInt * aFstLvlIdx,
                                      UInt * aSndLvlIdx );

    static inline iduShmBlockHeader* searchSuitableBlock( iduShmSSegment * aShmSSegment,
                                                          UInt           * aFstLvlIdx,
                                                          UInt           * aSndLvlIdx );

    static inline IDE_RC removeFreeBlock( iduShmTxInfo      * aShmTxInfo,
                                          iduShmSSegment    * aShmSSegment,
                                          iduShmBlockHeader * aBlockHdr,
                                          UInt                aFstLvlIdx,
                                          UInt                aSndLvlIdx );

    static inline IDE_RC insertFreeBlock( iduShmTxInfo      * aShmTxInfo,
                                          iduShmSSegment    * aShmSSegment,
                                          iduShmBlockHeader * aBlockHdr,
                                          UInt                aFstLvlIdx,
                                          UInt                aSndLvlIdx );

    static inline IDE_RC removeBlock( iduShmTxInfo      * aShmTxInfo,
                                      iduShmSSegment    * aShmSSegment,
                                      iduShmBlockHeader * aBlockHdr );

    static inline IDE_RC insertBlock( iduShmTxInfo      * aShmTxInfo,
                                      iduShmSSegment    * aShmSSegment,
                                      iduShmBlockHeader * aBlockHdr );

    static inline idBool canSplitBlock( iduShmBlockHeader * aBlockHdr,
                                        UInt                aSize );

    static inline IDE_RC splitBlock( iduShmTxInfo       * aShmTxInfo,
                                     iduShmBlockHeader  * aBlockHdr,
                                     UInt                 aSize,
                                     iduShmBlockHeader ** aSplitBlock );

    static inline IDE_RC absorbBlock( iduShmTxInfo       * aShmTxInfo,
                                      iduShmBlockHeader  * aPrvBlockHdr,
                                      iduShmBlockHeader  * aBlockHdr,
                                      iduShmBlockHeader ** aAbsorbBlock );

    static inline IDE_RC mergePrev( iduShmTxInfo       * aShmTxInfo,
                                    iduShmSSegment     * aShmSSegment,
                                    iduShmBlockHeader  * aBlockHdr,
                                    iduShmBlockHeader ** aMergeBlock );

    static inline idBool validateBlock( iduShmBlockHeader * aBlockHdr );

    static inline IDE_RC mergeNext( iduShmTxInfo       * aShmTxInfo,
                                    iduShmSSegment     * aShmSSegment,
                                    iduShmBlockHeader  * aBlockHdr,
                                    iduShmBlockHeader ** aMergeBlock);

    static inline IDE_RC trimFreeBlock( iduShmTxInfo      * aShmTxInfo,
                                        iduShmSSegment    * aShmSSegment,
                                        iduShmBlockHeader * aBlockHdr,
                                        UInt                aSize );

    static inline IDE_RC trimUsedBlock( iduShmTxInfo      * aShmTxInfo,
                                        iduShmSSegment    * aShmSSegment,
                                        iduShmBlockHeader * aBlockHdr,
                                        UInt                aSize );

    static inline IDE_RC trimFreeLeading( iduShmTxInfo       * aShmTxInfo,
                                          iduShmSSegment     * aShmSSegment,
                                          iduShmBlockHeader  * aBlockHdr,
                                          UInt                 aSize,
                                          iduShmBlockHeader ** aRemainBlockPtr);

    static inline IDE_RC findFree( iduShmTxInfo       * aShmTxInfo,
                                   iduShmSSegment     * aShmSSegment,
                                   UInt                 aSize,
                                   iduShmBlockHeader ** aFreeBlock );

    static inline IDE_RC prepareUsedBlock( iduShmTxInfo      * aShmTxInfo,
                                           iduShmSSegment    * aShmSSegment,
                                           iduShmBlockHeader * aBlockHdr,
                                           UInt                aSize,
                                           void             ** aDataPtr );

    static inline void initShmHeader( key_t           aShmKey,
                                      UInt            aSegID,
                                      UInt            aSegSize,
                                      iduShmType      aShmType,
                                      iduShmHeader  * aShmHeader );

    static inline void initBlockHeader( iduShmBlockHeader  * aBlockHeader,
                                        idShmAddr            aSelfAddr,
                                        UInt                 aSize );

    static inline void initFstBlockHeader( iduShmBlockHeader  * aBlockHeader,
                                           idShmAddr            aSelfAddr,
                                           UInt                 aSize );

    static inline void initSSegment( key_t             aShmKey,
                                     UInt              aSegSize,
                                     UInt              aSysSegSize,
                                     iduShmType        aShmType,
                                     ULong             aMaxSegCnt,
                                     iduShmSSegment  * aShmSSegment );

    static void destSSegment( iduShmSSegment  * aShmSSegment );

    static inline IDE_RC initDataAreaOfSeg( iduShmTxInfo       * aShmTxInfo,
                                            iduShmHeader       * aNewSeg,
                                            iduShmBlockHeader ** aInitBlock );

    static inline void getMaxSegCnt( ULong   aMaxSize,
                                     UInt    aShmChunkSize,
                                     ULong * aMaxCnt4Seg );

    static inline ULong getChunkSize( ULong aReqAllocSize );

    static inline idBool validateFBMatrix();

    static inline void initSegInfo( ULong aSegID );

    static IDE_RC validateShmChunk( iduShmHeader * aShmHeader );

    static IDE_RC validateShmArea();

    static inline UInt getMaxBlockSize4Chunk( UInt aChunkSize );

    static IDE_RC dropAllDSegmentAndPrint( iduShmSSegment  * aSSegment );
    static IDE_RC dropAllSegmentAndPrint( iduShmSSegment  * aSSegment );
};

/*
 * Function gets least significant bit
 */
inline SInt iduShmMgr::getLSBit( SInt aVal )
{
    UInt sDigit;
    UInt sLeastBit = aVal & -aVal;

    sDigit = sLeastBit <= 0xffff ? ( sLeastBit <= 0xff ? 0 : 8 ) :
        ( sLeastBit <= 0xffffff ? 16 : 24 );

    return mMSBit4FF[sLeastBit >> sDigit] + sDigit;
}

/*
 * Function gets most significant bit
 */
inline SInt iduShmMgr::getMSBit( SInt aVal )
{
    UInt sDigit;
    UInt sValue = (UInt)aVal;

    sDigit = sValue <= 0xffff ? (sValue <= 0xff ? 0 : 8) :
        ( sValue <= 0xffffff ? 16 : 24 );

    return mMSBit4FF[sValue >> sDigit] + sDigit;
}

inline idBool iduShmMgr::isBlockFree( const iduShmBlockHeader *aBlock )
{
    return ( ( aBlock->mSize & IDU_SHM_BLOCK_SIZE_FREE_BIT ) != 0 ?
             ID_TRUE : ID_FALSE );
}

inline idBool iduShmMgr::isBlockUsed( const iduShmBlockHeader *aBlock )
{
    return ((aBlock->mSize & IDU_SHM_BLOCK_SIZE_FREE_BIT) != 0 ?
            ID_FALSE : ID_TRUE );
}

inline IDE_RC iduShmMgr::setBlockFree( iduShmTxInfo      * aShmTxInfo,
                                       iduShmBlockHeader * aBlock )
{
    IDE_TEST( iduVLogShmMgr::writeSetBlockSize( aShmTxInfo,
                                                aBlock )
              != IDE_SUCCESS );

    aBlock->mSize |= IDU_SHM_BLOCK_SIZE_FREE_BIT;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::setBlockUsed( iduShmTxInfo      * aShmTxInfo,
                                       iduShmBlockHeader * aBlock )
{
    IDE_TEST( iduVLogShmMgr::writeSetBlockSize( aShmTxInfo,
                                                aBlock )
              != IDE_SUCCESS );

    aBlock->mSize = aBlock->mSize & ~(IDU_SHM_BLOCK_SIZE_FREE_BIT);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline idBool iduShmMgr::isPrevBlockFree( const iduShmBlockHeader *aBlock )
{
    return ((aBlock->mSize & IDU_SHM_BLOCK_SIZE_PREV_FREE_BIT) != 0 ?
            ID_TRUE : ID_FALSE );
}

inline IDE_RC iduShmMgr::setPrevBlockFree( iduShmTxInfo      * aShmTxInfo,
                                           iduShmBlockHeader * aBlock )
{
    IDE_TEST( iduVLogShmMgr::writeSetBlockSize( aShmTxInfo,
                                                aBlock )
              != IDE_SUCCESS );

    aBlock->mSize |= IDU_SHM_BLOCK_SIZE_PREV_FREE_BIT;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::setPrevBlockUsed( iduShmTxInfo      * aShmTxInfo,
                                           iduShmBlockHeader * aBlock )
{
    IDE_TEST( iduVLogShmMgr::writeSetBlockSize( aShmTxInfo,
                                                aBlock )
              != IDE_SUCCESS );

    aBlock->mSize = aBlock->mSize & ~(IDU_SHM_BLOCK_SIZE_PREV_FREE_BIT);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline iduShmBlockHeader* iduShmMgr::getBlockFromDataPtr( const void* aPtr )
{
    return (iduShmBlockHeader*)((SChar*)aPtr - IDU_SHM_BLOCK_START_OFFSET);
}

inline void* iduShmMgr::getDataPtr4Block( const iduShmBlockHeader *aBlock )
{
    return ((SChar*)aBlock) + IDU_SHM_BLOCK_START_OFFSET;
}

inline void* iduShmMgr::getDataPtr4Offset( const void* aPtr, UInt aOffset )
{
    return ((SChar*)aPtr + aOffset);
}

inline iduShmBlockHeader* iduShmMgr::getPrevBlock( const iduShmBlockHeader *aBlock )
{
    iduShmBlockFooter *sPrevFooter = (iduShmBlockFooter*)aBlock - 1;
    return (iduShmBlockHeader*)IDU_SHM_GET_BLOCK_HEADER_PTR( sPrevFooter->mHeaderAddr );
}

inline iduShmBlockHeader* iduShmMgr::getNextBlock( const iduShmBlockHeader *aBlock )
{
    iduShmBlockHeader * sNextBlock = (iduShmBlockHeader*)getDataPtr4Offset(
        getDataPtr4Block( aBlock ),
        IDU_SHM_GET_BLOCK_SIZE( aBlock ) + ID_SIZEOF( iduShmBlockFooter ) );

    IDE_ASSERT( !IDU_SHM_IS_BLOCK_LAST( aBlock ) );

    return sNextBlock;
}

inline IDE_RC iduShmMgr::linkAndGetBlockNext( iduShmTxInfo            * aShmTxInfo,
                                              const iduShmBlockHeader * aBlock,
                                              iduShmBlockHeader      ** aNxtBlock )
{
    iduShmBlockHeader * sNextBlock = getNextBlock( aBlock );
    iduShmBlockFooter * sFooter = (iduShmBlockFooter*)sNextBlock - 1;

    IDE_TEST( iduVLogShmMgr::writeSetBlockPrev( aShmTxInfo,
                                                sNextBlock )
              != IDE_SUCCESS );

    sFooter->mHeaderAddr = aBlock->mAddrSelf;


    *aNxtBlock = sNextBlock;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::setBlkAndPrvOfNxtBlkFree( iduShmTxInfo      * aShmTxInfo,
                                                   iduShmBlockHeader * aBlock )
{
    iduShmBlockHeader * sNextBlock = NULL;

    IDE_TEST( linkAndGetBlockNext( aShmTxInfo, aBlock, &sNextBlock )
              != IDE_SUCCESS );

    IDE_TEST( setPrevBlockFree( aShmTxInfo, sNextBlock ) != IDE_SUCCESS );
    IDE_TEST( setBlockFree( aShmTxInfo, aBlock ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::setBlkUsedAndPrvOfNxtBlkUsed( iduShmTxInfo      * aShmTxInfo,
                                                       iduShmBlockHeader * aBlock )
{
    iduShmBlockHeader * sNextBlock = getNextBlock( aBlock );

    IDE_TEST( setPrevBlockUsed( aShmTxInfo, sNextBlock )
              != IDE_SUCCESS );

    IDE_TEST( setBlockUsed( aShmTxInfo, aBlock )
              != IDE_SUCCESS );

#ifdef MEMORY_ASSERT
    /* The following sets up the fence in the footer of the current
     * block.  freeMem() uses the fence to detect buffer overflow. */
    ( (iduShmBlockFooter*)sNextBlock - 1 )->mFence = gFence;
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline UInt iduShmMgr::alignUp( UInt aSize, UInt aAlignSize )
{
    return (aSize + (aAlignSize - 1)) & ~(aAlignSize - 1);
}

inline UInt iduShmMgr::alignDown( UInt aSize, UInt aAlignSize )
{
    return aSize - (aSize & (aAlignSize - 1));
}

inline SChar* iduShmMgr::alignUpPtr( const void* aPtr, UInt aAlignSize )
{
    void *sAlignedPtr =
        (void*)(((vULong)aPtr + ( aAlignSize - 1 )) & ~( (vULong)aAlignSize - 1 ));

    IDE_ASSERT( ( 0 == ( aAlignSize & ( aAlignSize - 1 )) ) && "Must align to a power of two" );

    return (SChar*)sAlignedPtr;
}

inline UInt iduShmMgr::adjustRequestSize( UInt aSize, UInt aAlignSize )
{
    UInt sAdjustSize = 0;
    UInt sAlignSize  = 0;

    sAlignSize  = alignUp( aSize, aAlignSize );
    sAdjustSize = IDL_MAX( sAlignSize, IDU_SHM_BLOCK_SIZE_MIN );

    return sAdjustSize;
}

inline void iduShmMgr::getMatrixIdx4Ins( UInt   aSize,
                                         UInt * aFstLvlIdx,
                                         UInt * aSndLvlIdx )
{
    UInt  sFstLvlIdx;
    UInt  sSndLvlIdx;

    if( aSize < IDU_SHM_SMALL_BLOCK_SIZE )
    {
        sFstLvlIdx = 0;
        /* SCM_SHM_SMALL_BLOCK_SIZE보다 작은 Block은
         * ( SCM_SHM_SMALL_BLOCK_SIZE / SCM_SHM_SND_LVL_INDEX_COUNT )로 나누어진
         * 1차원의 Mapping Array를 이용한다. */
        sSndLvlIdx = aSize / ( IDU_SHM_SMALL_BLOCK_SIZE / IDU_SHM_SND_LVL_INDEX_COUNT );
    }
    else
    {
        /* SCM_SHM_SMALL_BLOCK_SIZE보다 큰 Block은
         * 2차원 Array를 이용한다. */
        sFstLvlIdx = getMSBit( (SInt)(aSize) );

        sSndLvlIdx = (UInt)
            ((aSize >> (UInt)(sFstLvlIdx - IDU_SHM_SND_LVL_IDX_COUNT_LOG2) ^
              ( 1 << IDU_SHM_SND_LVL_IDX_COUNT_LOG2 ) ));

        sFstLvlIdx -= ( IDU_SHM_FST_LVL_INDEX_SHIFT - 1 );
    }

    *aFstLvlIdx = sFstLvlIdx;
    *aSndLvlIdx = sSndLvlIdx;
}

inline void iduShmMgr::searchMapping( UInt   aSize,
                                      UInt * aFstLvlIdx,
                                      UInt * aSndLvlIdx )
{
    UInt sAlignUpSize;
    UInt sSize;

    sSize = aSize;

    if( sSize > ( 1 << IDU_SHM_SND_LVL_IDX_COUNT_LOG2 ) )
    {
        /* Round-Up Split Policy를 사용한다. 이유는 논문을 참조하기 바란다.*/
        sAlignUpSize = ( 1 << ( getMSBit( aSize ) -
                                IDU_SHM_SND_LVL_IDX_COUNT_LOG2 ) ) - 1;
        sSize += sAlignUpSize;
    }

    getMatrixIdx4Ins( sSize, aFstLvlIdx, aSndLvlIdx );
}

inline iduShmBlockHeader* iduShmMgr::searchSuitableBlock( iduShmSSegment * aShmSSegment,
                                                          UInt           * aFstLvlIdx,
                                                          UInt           * aSndLvlIdx )
{
    UInt  sFstLvlMap;
    UInt  sSndLvlMap;
    UInt  sFstLvlIdx = *aFstLvlIdx;
    UInt  sSndLvlIdx = *aSndLvlIdx;

    sSndLvlMap = aShmSSegment->mSndLvlBitmap[sFstLvlIdx] & ( ~0 << sSndLvlIdx );

    if( sSndLvlMap == 0 )
    {
        sFstLvlMap = aShmSSegment->mFstLvlBitmap & ( ~0 << ( sFstLvlIdx + 1 ));

        if( sFstLvlMap == 0 )
        {
            return NULL;
        }

        sFstLvlIdx = getLSBit( sFstLvlMap );
        sSndLvlMap = aShmSSegment->mSndLvlBitmap[sFstLvlIdx];
    }

    sSndLvlIdx = getLSBit( sSndLvlMap );

    *aFstLvlIdx = sFstLvlIdx;
    *aSndLvlIdx = sSndLvlIdx;

    return (iduShmBlockHeader*)IDU_SHM_GET_BLOCK_HEADER_PTR(
        aShmSSegment->mFBMatrix[ sFstLvlIdx ][ sSndLvlIdx ] );
}

inline IDE_RC iduShmMgr::removeFreeBlock( iduShmTxInfo      * aShmTxInfo,
                                          iduShmSSegment    * aShmSSegment,
                                          iduShmBlockHeader * aBlockHdr,
                                          UInt                aFstLvlIdx,
                                          UInt                aSndLvlIdx )
{
    IDE_ASSERT( aBlockHdr->mFreeList.mAddrPrev != 0 );
    IDE_ASSERT( aBlockHdr->mFreeList.mAddrNext != 0 );

    iduShmBlockHeader *sPrevPtr = (iduShmBlockHeader*)
        IDU_SHM_GET_BLOCK_HEADER_PTR( aBlockHdr->mFreeList.mAddrPrev );

    iduShmBlockHeader *sNextPtr = (iduShmBlockHeader*)
        IDU_SHM_GET_BLOCK_HEADER_PTR( aBlockHdr->mFreeList.mAddrNext );

    IDE_TEST( iduVLogShmMgr::writeRemoveFreeBlock( aShmTxInfo,
                                                   aBlockHdr,
                                                   aFstLvlIdx,
                                                   aSndLvlIdx )
              != IDE_SUCCESS );

    if( sNextPtr != NULL )
    {
        IDE_ASSERT( sNextPtr->mFreeList.mAddrPrev !=
                    aBlockHdr->mFreeList.mAddrPrev );

        sNextPtr->mFreeList.mAddrPrev = aBlockHdr->mFreeList.mAddrPrev;

        IDE_ASSERT( sNextPtr->mFreeList.mAddrPrev != 0 );
        IDE_ASSERT( sNextPtr->mFreeList.mAddrNext != 0 );
    }

    if( sPrevPtr != NULL )
    {
        sPrevPtr->mFreeList.mAddrNext = aBlockHdr->mFreeList.mAddrNext;

        IDE_ASSERT( sPrevPtr->mFreeList.mAddrPrev != 0 );
        IDE_ASSERT( sPrevPtr->mFreeList.mAddrNext != 0 );
    }

    /* If this block is the head of the free list, set new head. */
    if( aShmSSegment->mFBMatrix[aFstLvlIdx][aSndLvlIdx] == aBlockHdr->mAddrSelf )
    {
        if( aBlockHdr->mFreeList.mAddrNext != IDU_SHM_NULL_ADDR )
        {
            IDE_ASSERT( IDU_SHM_GET_ADDR_SEGID( aBlockHdr->mFreeList.mAddrNext ) <
                        mSSegment->mSegCount );
        }

        aShmSSegment->mFBMatrix[aFstLvlIdx][aSndLvlIdx] = aBlockHdr->mFreeList.mAddrNext;

        /* If the new head is null, clear the bitmap. */
        if( sNextPtr == NULL )
        {
            aShmSSegment->mSndLvlBitmap[aFstLvlIdx] &= ~( 1 << aSndLvlIdx );

            /* If the second bitmap is now empty, clear the fl bitmap. */
            if( aShmSSegment->mSndLvlBitmap[aFstLvlIdx] == 0 )
            {
                aShmSSegment->mFstLvlBitmap &= ~(1 << aFstLvlIdx);
            }
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Insert a free block into the free block list. */
inline IDE_RC iduShmMgr::insertFreeBlock( iduShmTxInfo      * aShmTxInfo,
                                          iduShmSSegment    * aShmSSegment,
                                          iduShmBlockHeader * aBlockHdr,
                                          UInt                aFstLvlIdx,
                                          UInt                aSndLvlIdx )
{
    iduShmBlockHeader *sCurBlockPtr =
        (iduShmBlockHeader*)IDU_SHM_GET_BLOCK_HEADER_PTR( aShmSSegment->mFBMatrix[aFstLvlIdx][aSndLvlIdx] );

    IDE_ASSERT( ( aBlockHdr != NULL ) && "Cannot insert a null entry into the free list");

    IDE_TEST( iduVLogShmMgr::writeInsertFreeBlock( aShmTxInfo,
                                                   aFstLvlIdx,
                                                   aSndLvlIdx,
                                                   aBlockHdr )
              != IDE_SUCCESS );

    aBlockHdr->mFreeList.mAddrPrev = IDU_SHM_NULL_ADDR;

    if( sCurBlockPtr == NULL )
    {
        aBlockHdr->mFreeList.mAddrNext = IDU_SHM_NULL_ADDR;
    }
    else
    {
        aBlockHdr->mFreeList.mAddrNext = sCurBlockPtr->mAddrSelf;

        IDE_ASSERT( sCurBlockPtr->mFreeList.mAddrPrev != 0 );
        IDE_ASSERT( sCurBlockPtr->mFreeList.mAddrNext != 0 );

        sCurBlockPtr->mFreeList.mAddrPrev = aBlockHdr->mAddrSelf;

        IDE_ASSERT( sCurBlockPtr->mFreeList.mAddrPrev != 0 );
        IDE_ASSERT( sCurBlockPtr->mFreeList.mAddrNext != 0 );
    }

    IDE_ASSERT( getDataPtr4Block( aBlockHdr )
                == alignUpPtr( getDataPtr4Block(aBlockHdr), IDU_SHM_ALIGN_SIZE ) &&
                "Block not alinged properly");

    IDU_SHM_VALIDATE_ADDR_PTR( aBlockHdr->mAddrSelf -
                               IDU_SHM_ALLOC_BLOCK_HEADER_SIZE, aBlockHdr );

    IDE_ASSERT( IDU_SHM_GET_ADDR_SEGID( aBlockHdr->mAddrSelf ) <
                mSSegment->mSegCount );

    aShmSSegment->mFBMatrix[aFstLvlIdx][aSndLvlIdx] = aBlockHdr->mAddrSelf;
    aShmSSegment->mFstLvlBitmap |= (1 << aFstLvlIdx);
    aShmSSegment->mSndLvlBitmap[aFstLvlIdx] |= (1 << aSndLvlIdx);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Remove a given block from the free list. */
inline IDE_RC iduShmMgr::removeBlock( iduShmTxInfo      * aShmTxInfo,
                                      iduShmSSegment    * aShmSSegment,
                                      iduShmBlockHeader * aBlockHdr )
{
    UInt sFstLvlIdx;
    UInt sSndLvlIdx;

    getMatrixIdx4Ins( IDU_SHM_GET_BLOCK_SIZE( aBlockHdr ),
                      &sFstLvlIdx,
                      &sSndLvlIdx );

    return removeFreeBlock( aShmTxInfo,
                            aShmSSegment,
                            aBlockHdr,
                            sFstLvlIdx,
                            sSndLvlIdx );
}

inline IDE_RC iduShmMgr::insertBlock( iduShmTxInfo      * aShmTxInfo,
                                      iduShmSSegment    * aShmSSegment,
                                      iduShmBlockHeader * aBlockHdr )
{
    UInt sFstLvlIdx;
    UInt sSndLvlIdx;

    getMatrixIdx4Ins( IDU_SHM_GET_BLOCK_SIZE( aBlockHdr ),
                      &sFstLvlIdx,
                      &sSndLvlIdx );

    return insertFreeBlock( aShmTxInfo,
                            aShmSSegment,
                            aBlockHdr,
                            sFstLvlIdx,
                            sSndLvlIdx );
}

inline idBool iduShmMgr::canSplitBlock( iduShmBlockHeader * aBlockHdr,
                                        UInt                aSize )
{
    return ( IDU_SHM_GET_BLOCK_SIZE( aBlockHdr ) >=
             ( ID_SIZEOF(iduShmBlockHeader) + aSize + ID_SIZEOF( iduShmBlockFooter ) ) ) ?
        ID_TRUE : ID_FALSE;
}

inline IDE_RC iduShmMgr::splitBlock( iduShmTxInfo       * aShmTxInfo,
                                     iduShmBlockHeader  * aBlockHdr,
                                     UInt                 aSize,
                                     iduShmBlockHeader ** aSplitBlock )
{
    iduShmBlockHeader * sRemainBlockHdr = (iduShmBlockHeader*)getDataPtr4Offset(
        getDataPtr4Block( aBlockHdr ),
        aSize + ID_SIZEOF( iduShmBlockFooter ) );

    const UInt sRemainSize = IDU_SHM_GET_BLOCK_SIZE( aBlockHdr ) -
        ( aSize + IDU_SHM_BLOCK_OVERHEAD );

    IDE_ASSERT( getDataPtr4Block( sRemainBlockHdr )
                == alignUpPtr( getDataPtr4Block(sRemainBlockHdr ), IDU_SHM_ALIGN_SIZE )
                && "Remaining block not aligned properly");

    IDE_ASSERT( IDU_SHM_GET_BLOCK_SIZE( aBlockHdr )
                == sRemainSize + aSize + IDU_SHM_BLOCK_OVERHEAD );

    IDE_TEST( iduVLogShmMgr::writeSplitBlock( aShmTxInfo,
                                              aBlockHdr )
              != IDE_SUCCESS );

    IDU_SHM_SET_BLOCK_SIZE( sRemainBlockHdr, sRemainSize );
    IDE_ASSERT( IDU_SHM_GET_BLOCK_SIZE( sRemainBlockHdr ) >= IDU_SHM_MIN_BLOCK_SIZE
                && "Block split with invalid size" );

    IDU_SHM_SET_BLOCK_SIZE( aBlockHdr, aSize );

    sRemainBlockHdr->mAddrSelf
        = aBlockHdr->mAddrSelf + aSize + IDU_SHM_BLOCK_OVERHEAD;

    IDE_TEST( setBlkAndPrvOfNxtBlkFree( aShmTxInfo, sRemainBlockHdr )
              != IDE_SUCCESS );

    IDE_DASSERT( IDU_SHM_GET_BLOCK_HEADER_PTR( sRemainBlockHdr->mAddrSelf )
                == (SChar*)sRemainBlockHdr );

    *aSplitBlock = sRemainBlockHdr;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::absorbBlock( iduShmTxInfo       * aShmTxInfo,
                                      iduShmBlockHeader  * aPrvBlockHdr,
                                      iduShmBlockHeader  * aBlockHdr,
                                      iduShmBlockHeader ** aAbsorbBlock )
{
    iduShmBlockHeader * sNxtBlock;

    IDE_ASSERT( !IDU_SHM_IS_BLOCK_LAST(aPrvBlockHdr) && "Previous block can't be last!" );

    IDE_TEST( iduVLogShmMgr::writeSetBlockSize( aShmTxInfo,
                                                aPrvBlockHdr )
              != IDE_SUCCESS );

    aPrvBlockHdr->mSize += IDU_SHM_GET_BLOCK_SIZE( aBlockHdr ) + IDU_SHM_BLOCK_OVERHEAD;

    IDE_TEST( linkAndGetBlockNext( aShmTxInfo, aPrvBlockHdr, &sNxtBlock )
              != IDE_SUCCESS );

    *aAbsorbBlock = aPrvBlockHdr;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aAbsorbBlock = NULL;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::mergePrev( iduShmTxInfo       * aShmTxInfo,
                                    iduShmSSegment     * aShmSSegment,
                                    iduShmBlockHeader  * aBlockHdr,
                                    iduShmBlockHeader ** aMergeBlock)
{
    iduShmBlockHeader *sPrvBlockPtr;
    iduShmBlockHeader *sMergeBlockPtr = aBlockHdr;

    if( isPrevBlockFree( aBlockHdr ) == ID_TRUE )
    {
        sPrvBlockPtr = getPrevBlock( aBlockHdr );

        IDE_ASSERT( sPrvBlockPtr && "Prev physical block can't be null");
        IDE_ASSERT( isBlockFree( sPrvBlockPtr ) && "Prev block is not free though marked as such");

        IDE_TEST( removeBlock( aShmTxInfo, aShmSSegment, sPrvBlockPtr )
                  != IDE_SUCCESS );


        IDE_TEST( absorbBlock( aShmTxInfo, sPrvBlockPtr, aBlockHdr, &sMergeBlockPtr )
                  != IDE_SUCCESS );

    }

    *aMergeBlock = sMergeBlockPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aMergeBlock = NULL;

    return IDE_FAILURE;
}

inline idBool iduShmMgr::validateBlock( iduShmBlockHeader * aBlockHdr )
{
    iduShmBlockHeader *sPrvBlockPtr;

    if( isPrevBlockFree( aBlockHdr ) == ID_TRUE )
    {
        sPrvBlockPtr = getPrevBlock( aBlockHdr );

        IDE_ASSERT( sPrvBlockPtr && "Prev physical block can't be null");
        IDE_ASSERT( isBlockFree( sPrvBlockPtr ) && "Prev block is not free though marked as such");
    }

    return ID_TRUE;
}

inline IDE_RC iduShmMgr::mergeNext( iduShmTxInfo       * aShmTxInfo,
                                    iduShmSSegment     * aShmSSegment,
                                    iduShmBlockHeader  * aBlockHdr,
                                    iduShmBlockHeader ** aMergeBlock )
{
    iduShmBlockHeader *sNxtBlockPtr;
    iduShmBlockHeader *sMergeBlockPtr = aBlockHdr;

    sNxtBlockPtr = getNextBlock( aBlockHdr );

    IDE_ASSERT( sNxtBlockPtr && "Next physical block can't be null");

    if( isBlockFree( sNxtBlockPtr ) == ID_TRUE )
    {
        IDE_ASSERT( !IDU_SHM_IS_BLOCK_LAST(aBlockHdr) && "Previous block can't be last!");

        IDE_TEST( removeBlock( aShmTxInfo, aShmSSegment, sNxtBlockPtr )
                  != IDE_SUCCESS );


        IDE_TEST( absorbBlock( aShmTxInfo, aBlockHdr, sNxtBlockPtr, &sMergeBlockPtr )
                  != IDE_SUCCESS );

    }

    *aMergeBlock = sMergeBlockPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::trimFreeBlock( iduShmTxInfo      * aShmTxInfo,
                                        iduShmSSegment    * aShmSSegment,
                                        iduShmBlockHeader * aBlockHdr,
                                        UInt                aSize )
{
    iduShmBlockHeader *sRemainBlockPtr;
    iduShmBlockHeader *sNxtBlockPtr = NULL;

    IDE_ASSERT( ( isBlockFree(aBlockHdr) == ID_TRUE ) && "Block must be free" );

    if( canSplitBlock( aBlockHdr, aSize ) == ID_TRUE )
    {
        IDE_TEST( splitBlock( aShmTxInfo, aBlockHdr, aSize, &sRemainBlockPtr )
                  != IDE_SUCCESS );

        IDE_TEST( linkAndGetBlockNext( aShmTxInfo, aBlockHdr, &sNxtBlockPtr )
                  != IDE_SUCCESS );

        IDE_ASSERT( sRemainBlockPtr == sNxtBlockPtr );

        IDE_TEST( setPrevBlockFree( aShmTxInfo, sRemainBlockPtr )
                  != IDE_SUCCESS );

        IDE_TEST( insertBlock( aShmTxInfo, aShmSSegment, sRemainBlockPtr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::trimUsedBlock( iduShmTxInfo      * aShmTxInfo,
                                        iduShmSSegment    * aShmSSegment,
                                        iduShmBlockHeader * aBlockHdr,
                                        UInt                aSize )
{
    iduShmBlockHeader *sRemainBlockPtr;

    IDE_ASSERT( ( isBlockFree(aBlockHdr) == ID_FALSE ) && "Block must be used" );

    if( canSplitBlock( aBlockHdr, aSize ) == ID_TRUE )
    {
        /* If the next block is free, we must coalesce. */
        IDE_TEST( splitBlock( aShmTxInfo, aBlockHdr, aSize, &sRemainBlockPtr )
                  != IDE_SUCCESS );

        IDE_TEST( setPrevBlockUsed( aShmTxInfo, sRemainBlockPtr )
                  != IDE_SUCCESS );

        IDE_TEST( mergeNext( aShmTxInfo,
                             aShmSSegment,
                             sRemainBlockPtr,
                             &sRemainBlockPtr )
                  != IDE_SUCCESS );

        IDE_TEST( insertBlock( aShmTxInfo, aShmSSegment, sRemainBlockPtr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::trimFreeLeading( iduShmTxInfo       * aShmTxInfo,
                                          iduShmSSegment     * aShmSSegment,
                                          iduShmBlockHeader  * aBlockHdr,
                                          UInt                 aSize,
                                          iduShmBlockHeader ** aRemainBlockPtr )
{
    iduShmBlockHeader *sNxtBlockPtr;
    iduShmBlockHeader *sRemainBlockPtr = aBlockHdr;

    if( canSplitBlock( aBlockHdr, aSize ) == ID_TRUE )
    {
        IDE_TEST( splitBlock( aShmTxInfo,
                              aBlockHdr,
                              aSize - IDU_SHM_BLOCK_OVERHEAD,
                              &sRemainBlockPtr )
                  != IDE_SUCCESS );

        /* SplitBlock에서 Undo Image가 Logging되기 때문에 Logging이 불필요 */
        IDE_TEST( setPrevBlockFree( aShmTxInfo, sRemainBlockPtr )
                  != IDE_SUCCESS );

        IDE_TEST( linkAndGetBlockNext( aShmTxInfo, aBlockHdr, &sNxtBlockPtr )
                  != IDE_SUCCESS );

        IDE_TEST( insertBlock( aShmTxInfo, aShmSSegment, aBlockHdr )
                  != IDE_SUCCESS );
    }

    *aRemainBlockPtr = sRemainBlockPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aRemainBlockPtr = NULL;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::findFree( iduShmTxInfo       * aShmTxInfo,
                                   iduShmSSegment     * aShmSSegment,
                                   UInt                 aSize,
                                   iduShmBlockHeader ** aFreeBlock)
{
    UInt sFstLvlIdx = 0;
    UInt sSndLvlIdx = 0;
    iduShmBlockHeader * sBlockPtr = NULL;

    IDE_ASSERT( aSize != 0 );

    searchMapping( aSize, &sFstLvlIdx, &sSndLvlIdx );
    sBlockPtr = searchSuitableBlock( aShmSSegment, &sFstLvlIdx, &sSndLvlIdx );

    if( sBlockPtr != NULL )
    {
        IDE_ASSERT( IDU_SHM_GET_BLOCK_SIZE(sBlockPtr) >= aSize );

        IDE_TEST( removeFreeBlock( aShmTxInfo,
                                   aShmSSegment,
                                   sBlockPtr,
                                   sFstLvlIdx,
                                   sSndLvlIdx )
                  != IDE_SUCCESS );
    }

    *aFreeBlock = sBlockPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aFreeBlock = NULL;

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::prepareUsedBlock( iduShmTxInfo      * aShmTxInfo,
                                           iduShmSSegment    * aShmSSegment,
                                           iduShmBlockHeader * aBlockHdr,
                                           UInt                aSize,
                                           void             ** aDataPtr )
{
    void *sDataPtr4Block = NULL;

    if( aBlockHdr != NULL )
    {
        IDE_TEST( trimFreeBlock( aShmTxInfo,
                                 aShmSSegment,
                                 aBlockHdr,
                                 aSize )
                  != IDE_SUCCESS );

        IDE_TEST( setBlkUsedAndPrvOfNxtBlkUsed( aShmTxInfo, aBlockHdr )
                  != IDE_SUCCESS );

        sDataPtr4Block = getDataPtr4Block( aBlockHdr );
    }

    *aDataPtr = sDataPtr4Block;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aDataPtr = NULL;

    return IDE_FAILURE;
}

inline void iduShmMgr::initShmHeader( key_t           aShmKey,
                                      UInt            aSegID,
                                      UInt            aSegSize,
                                      iduShmType      aShmType,
                                      iduShmHeader  * aShmHeader )
{
    idlOS::snprintf( aShmHeader->mSigniture, 128, "%s", IDU_SHM_SIGNATURE );

    aShmHeader->mShmKey      = aShmKey;
    aShmHeader->mState       = IDU_SHM_STATE_INVALID;
    aShmHeader->mType        = aShmType;
    aShmHeader->mSegID       = aSegID;
    aShmHeader->mSize        = aSegSize;
    aShmHeader->mStartUpTime = mStartUpTime;
    aShmHeader->mCreateTime  = idlOS::gettimeofday();

    idlOS::snprintf( aShmHeader->mVersion, 24, "%s", iduShmVersionString );

    aShmHeader->mVersionID = iduShmVersionID;
}

inline void iduShmMgr::initBlockHeader( iduShmBlockHeader  * aBlockHeader,
                                        idShmAddr            aSelfAddr,
                                        UInt                 aSize )
{
    aBlockHeader->mAddrSelf       = aSelfAddr;
    aBlockHeader->mSize           = aSize;
    aBlockHeader->mFreeList.mAddrPrev = IDU_SHM_NULL_ADDR;
    aBlockHeader->mFreeList.mAddrNext = IDU_SHM_NULL_ADDR;
}

inline void iduShmMgr::initFstBlockHeader( iduShmBlockHeader  * aBlockHeader,
                                           idShmAddr            aSelfAddr,
                                           UInt                 aSize )
{
    aBlockHeader->mAddrSelf       = aSelfAddr;
    aBlockHeader->mSize           = aSize;
    aBlockHeader->mFreeList.mAddrPrev = IDU_SHM_NULL_ADDR;
    aBlockHeader->mFreeList.mAddrNext = IDU_SHM_NULL_ADDR;
}

inline void iduShmMgr::getMaxSegCnt( ULong   aMaxSize,
                                     UInt    aShmChunkSize,
                                     ULong * aMaxCnt4Seg )
{
    ULong sShmMaxChunkCnt;
    ULong sMaxSegmentIDCnt;

    /* Shared Memory Chunk 최대갯수를 계산한다. */
    sShmMaxChunkCnt = aMaxSize / aShmChunkSize;

    // For System Segment, Add one.
    sShmMaxChunkCnt++;

    // 2 ^ (32 - mOffsetBitSize4ShmAddr)만큼의 Segment만 표현할 수 있다.
    IDE_ASSERT( ID_SIZEOF(idShmAddr) == ID_SIZEOF(vULong) );
    sMaxSegmentIDCnt = IDU_SHM_GET_ADDR_SEGID( ID_vULONG_MAX ) + 1; // 0부터 시작하므로

    if( sShmMaxChunkCnt > sMaxSegmentIDCnt )
    {
        sShmMaxChunkCnt = sMaxSegmentIDCnt;
    }

    *aMaxCnt4Seg = sShmMaxChunkCnt;
}

inline void iduShmMgr::setMetaBlockAddr( iduShmMetaBlockType aMetablockType,
                                         idShmAddr           aMetaBlockAddr )
{
    mSSegment->mArrMetaBlockAddr[aMetablockType] = aMetaBlockAddr;
}

inline idShmAddr iduShmMgr::getMetaBlockAddr( iduShmMetaBlockType aMetablockType )
{
    return mSSegment->mArrMetaBlockAddr[aMetablockType];
}

inline void iduShmMgr::getStatistics( iduShmStatistics * aShmTxInfo )
{
    *aShmTxInfo = mSSegment->mStatistics;
}

inline IDE_RC iduShmMgr::initLtArea( ULong aMaxSegCnt )
{
    UInt sState = 0;

    mAttachSegCnt = 0;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID_IDU,
                                 aMaxSegCnt,
                                 ID_SIZEOF(iduShmHeader*),
                                 (void**)&mArrSegment )
              != IDE_SUCCESS );
    sState = 1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
    case 1:
        IDE_ASSERT( iduMemMgr::free( mArrSegment ) == IDE_SUCCESS );
        mArrSegment = NULL;
    default:
        break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

inline IDE_RC iduShmMgr::destLtArea()
{
    IDE_ASSERT( iduMemMgr::free( mArrSegment ) == IDE_SUCCESS );
    mArrSegment = NULL;

    return IDE_SUCCESS;
}

inline SChar* iduShmMgr::getPtrOfAddrCheck( idShmAddr aAddr )
{
    SChar *sPtr = getPtrOfAddr( aAddr );
    IDE_ASSERT( sPtr != NULL );
    return sPtr;
}

inline iduShmProcMgrInfo * iduShmMgr::getProcMgrInfo()
{
    return &mSSegment->mProcMgrInfo;
}

inline void iduShmMgr::setShmCleanModeAtShutdown( iduShmProcInfo * aProcInfo )
{
    if( iduProperty::getShmMemoryPolicy() == 0 /* Remove */     &&
        aProcInfo->mType                  == IDU_PROC_TYPE_DAEMON )
    {
        mIsShmAreaFreeAtShutdown = ID_TRUE;
    }
    else
    {
        mIsShmAreaFreeAtShutdown = ID_FALSE;
    }
}

inline idBool iduShmMgr::isShmAreaFreeAtShutdown()
{
    return mIsShmAreaFreeAtShutdown;
}

inline ULong iduShmMgr::getChunkSize( ULong aReqAllocSize )
{
    ULong   sChunkSize;
    ULong   sShmAlignSize;
    UInt    sAlignUpSize;
    UInt    sSize;

    sChunkSize    = iduProperty::getShmChunkSize();
    sSize         = aReqAllocSize;

    // Round-Up Split Policy로 인해 추가되는 공간을 ChunkSize에 추가한다.
    if( sSize > ( 1 << IDU_SHM_SND_LVL_IDX_COUNT_LOG2 ) )
    {
        /* Round-Up Split Policy를 사용한다. 이유는 논문을 참조하기 바란다.*/
        sAlignUpSize = ( 1 << ( getMSBit( aReqAllocSize ) -
                                IDU_SHM_SND_LVL_IDX_COUNT_LOG2 ) ) - 1;
        sSize += sAlignUpSize;
    }

    if( sChunkSize < ( sSize + IDU_SHM_DATA_SEG_OVERHEAD ) )
    {
        sShmAlignSize = iduProperty::getShmAlignSize();

        sChunkSize  = (( sSize + sShmAlignSize - 1 ) / sShmAlignSize ) * sShmAlignSize;
        sChunkSize += IDU_SHM_DATA_SEG_OVERHEAD;
    }

    return sChunkSize;
}

inline idBool iduShmMgr::validateFBMatrix()
{
    UInt i, j;
    UInt sSegID;

    for( i = 0; i < IDU_SHM_FST_LVL_INDEX_COUNT; i++ )
    {
        for( j = 0; j < IDU_SHM_SND_LVL_INDEX_COUNT; j++ )
        {
            if( mSSegment->mFBMatrix[i][j] != IDU_SHM_NULL_ADDR )
            {
                sSegID = IDU_SHM_GET_ADDR_SEGID( mSSegment->mFBMatrix[i][j] );
                IDE_ASSERT( sSegID < mSSegment->mSegCount );
            }
        }
    }

    return ID_TRUE;
}

inline iduShmHeader * iduShmMgr::getSegShmHeader( UInt aSegID )
{
    if( mArrSegment[aSegID] == NULL )
    {
        IDE_ASSERT( attachSeg( aSegID, ID_FALSE /*aIsCleanPurpose*/  )
                    == IDE_SUCCESS );
    }

    return mArrSegment[aSegID];
}

inline void iduShmMgr::initSegInfo( ULong aSegID )
{
    idlOS::memset( mSSegment->mArrSegInfo + aSegID,
                   0,
                   ID_SIZEOF(iduStShmSegInfo) );
}

inline void iduShmMgr::dump( void * aDataBlock )
{
    iduShmBlockFooter * sPrevFooter;
    iduShmBlockHeader * sBlock;
    idShmAddr           sPrevBlockAddr;

    sBlock = getBlockFromDataPtr( aDataBlock );

    if ( sBlock != &mSSegment->mNullBlock )
    {
        sPrevFooter    = (iduShmBlockFooter*)aDataBlock - 1;
        sPrevBlockAddr = sPrevFooter->mHeaderAddr;
    }
    else
    {
        /* This field is meaningless and invalid for the initial null
         * block in the system segment header. */
        sPrevBlockAddr = IDU_SHM_NULL_ADDR;
    }

    idlOS::printf("Ptr:%p, PrvAddr:%"ID_xINT64_FMT", Type:%s, "
                  "Size:%"ID_UINT32_FMT", SelfAddr:%"ID_xINT64_FMT"\n",
                  aDataBlock,
                  sPrevBlockAddr,
                  sBlock->mType==(PERSISTENT)? "PERSISTENT":"PROVISIONAL",
                  sBlock->mSize,
                  sBlock->mAddrSelf);
}

inline ULong iduShmMgr::getTotalSegSize()
{
    ULong sTotalSegSize = 0;
    UInt  i;

    for( i = 0; i < mSSegment->mSegCount; i++ )
    {
         sTotalSegSize += mSSegment->mArrSegInfo[i].mSize;
    }

    return sTotalSegSize;
}

inline UInt iduShmMgr::getMaxBlockSize4Chunk( UInt aChunkSize )
{
    return ( aChunkSize - ID_SIZEOF(iduShmHeader) -
             IDU_SHM_BLOCK_START_OFFSET - IDU_SHM_ALIGN_SIZE -
             IDU_SHM_BLOCK_OVERHEAD * 2 );
}

#endif /* _O_IDU_SHM_MGR_H_ */

