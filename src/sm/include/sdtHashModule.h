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
 * $Id: $
 **********************************************************************/

#ifndef _O_SDT_HASH_MODULE_H_
#define _O_SDT_HASH_MODULE_H_ 1

#include <smDef.h>
#include <smiDef.h>
#include <smiTempTable.h>
#include <sdtHashDef.h>
#include <smuMemStack.h>
#include <sdtWAExtentMgr.h>

typedef struct sdtHashSlot
{
    scPageID  mPageID;
    scOffset  mOffset;
    UShort    mHashSet;
    ULong     mRowPtrOrHashSet;
}sdtHashSlot;

typedef struct sdtSubHashKey
{
    scPageID mPageID;
    scOffset mOffset;
    UShort   mHashHigh;
}sdtSubHashKey;

typedef struct sdtSubHash
{
    UChar    mHashLow;
    UChar    mKeyCount;
    UShort   mNextOffset;
    scPageID mNextPageID;

    sdtSubHashKey mKey[1];
}sdtSubHash;

#define SDT_SUB_HASH_SIZE (ID_SIZEOF(sdtSubHash) - ID_SIZEOF(sdtSubHashKey))

#define SDT_SUBHASH_MAX_KEYCOUNT            (255)
#define SDT_ALLOC_SUBHASH_MIN_KEYCOUNT      (64)

/******************************** SMALL HASH SET ***********************************
 * HashValue 상위 4bit를 이용해 In-Memory에서의 Hash Set 을 구성한다.             *
 ***********************************************************************************/
#define SDT_EXIST_SMALL_HASHSET( aHashSlot, aHashValue )                \
    (( (aHashSlot)->mHashSet & ( 0x0001 << ( (aHashValue) >> 28 ) )) != 0 )

#define SDT_NOT_EXIST_SMALL_HASHSET( aHashSlot, aHashValue )            \
    (( (aHashSlot)->mHashSet & ( 0x0001 << ( (aHashValue) >> 28 ) )) == 0 )

#define SDT_ADD_SMALL_HASHSET( aHashSlot, aHashValue )                  \
    (aHashSlot)->mHashSet |= ( 0x0001 << ( (aHashValue) >> 28 ) );

/******************************** LARGE HASH SET ***********************************
 * HashValue 상위 6bit를 이용해 Out-Memory에서의 Hash Set2를 구성한다.             *
 * 다음 4bit를 이용해 Hash Set1을 구성한다.                                        *
 ***********************************************************************************/
#define SDT_EXIST_LARGE_HASHSET( aHashSlot, aHashValue )                                            \
    (( 0 != ( (aHashSlot)->mRowPtrOrHashSet & ((ULong)(0x0000000000000001) <<  ((aHashValue) >> 26 )))) && \
     ( 0 != ( (aHashSlot)->mHashSet        & ((UShort)(0x00000001) << (((aHashValue) >> 22 ) & 0x0000000F )))))

#define SDT_NOT_EXIST_LARGE_HASHSET( aHashSlot, aHashValue )                                        \
    (( 0 == ((aHashSlot)->mRowPtrOrHashSet & ((ULong)(0x0000000000000001) <<  ((aHashValue) >> 26 )))) || \
     ( 0 == ( (aHashSlot)->mHashSet        & ((UShort)(0x00000001) << (((aHashValue) >> 22 ) & 0x0000000F )))))

/* Sub Hash에 저장된 상위 2Byte정보로 확인 */
#define SDT_ADD_LARGE_HASHSET( aHashSlot, aHashHigh )                                               \
    (aHashSlot)->mRowPtrOrHashSet |= ((ULong)(0x0000000000000001) << ( (aHashHigh) >> 10 ));        \
    (aHashSlot)->mHashSet         |= ((UShort)(0x00000001) << (((aHashHigh) >> 6 ) & 0x0000000F ));

/******************************** HASH VALUE ***************************************/
/* 검증용 */
#define SDT_MAKE_HASH_LOW( aHashValue )                 \
    (UChar)( ((aHashValue) & 0x0000FF00 ) >> 8 )

/* Sub Hash에는 상위 2Byte정보만 저장 되어 있다.*/
#define SDT_MAKE_HASH_HIGH( aHashValue )        \
    (UShort)( (aHashValue) >> 16 )

/******************************** FINE HASH SLOT ********************************/
#define SDT_GET_HASH_MAP_IDX( aHashValue, aHashSlotCount )      \
    ( (aHashValue) % (aHashSlotCount) )

#define SDT_GET_HASHSLOT_IN_WAPAGE( aWCBPtr, aMapIdx )    \
    (((sdtHashSlot*)(aWCBPtr)->mWAPagePtr) + ( (aMapIdx) % (SD_PAGE_SIZE / ID_SIZEOF(sdtHashSlot))))

#define SDT_GET_HASHSLOT_WCB( aWASegment, aMapIdx )    \
    ((aWASegment)->mWCBMap + ( sMapIdx / (SD_PAGE_SIZE / ID_SIZEOF(sdtHashSlot))))

/******************************** Single Row ***********************************/
#define SDT_IS_SINGLE_ROW( aOffset )            \
    (((aOffset) & 0x8000 ) != 0 )

#define SDT_IS_NOT_SINGLE_ROW( aOffset )        \
    (((aOffset) & 0x8000 ) == 0 )

#define SDT_SET_SINGLE_ROW( aOffset )           \
    ( (aOffset) |= 0x8000 )

#define SDT_DEL_SINGLE_FLAG_IN_OFFSET( aOffset )        \
    ((aOffset) % SD_PAGE_SIZE )

/******************************** fix Page ***********************************/
#define SDT_IS_FIXED_PAGE( aWCBPtr )  ( (aWCBPtr)->mFix > 0 )
#define SDT_IS_NOT_FIXED_PAGE( aWCBPtr )  ( (aWCBPtr)->mFix == 0 )


class sdtHashModule
{
public:
    /************************ class init/dest ***********************/

    static IDE_RC initializeStatic();

    static void   destroyStatic();

    /************************ property ***********************/
    static IDE_RC resetWASegmentSize( ULong aNewWorkAreaSize );
    static IDE_RC resizeWASegmentPool( UInt   aNewWASegmentCount );
    static void resetTempHashGroupPageCount();
    static UInt calcTempHashGroupExtentCount( UInt aMaxWAExtentCount );
    static void resetTempSubHashGroupPageCount();
    static UInt calcTempSubHashGroupExtentCount( UInt aMaxWAExtentCount );

    static ULong calcOptimalWorkAreaSize( UInt  aRowPageCount );
    static ULong calcSubOptimalWorkAreaSizeByRowCnt( ULong aRowCount );
    static ULong calcSubOptimalWorkAreaSizeBySubHashExtCnt( UInt aSubHashExtentCount );

    static void lock()
    {
        (void)mMutex.lock( NULL );
    }
    static void unlock()
    {
        (void)mMutex.unlock();
    }

    static void initHashTempHeader( smiTempTableHeader * aHeader);
    static void initHashGrp( sdtHashSegHdr     * aWASeg );


    /************************ for view ***********************/
    static void estimateHashSize( smiTempTableHeader * aHeader);
    static UInt getSubHashPageCount()
    {
        return mSubHashPageCount;
    };
    static UInt getHashSlotPageCount()
    {
        return mHashSlotPageCount;
    };
    static UInt getMaxWAExtentCount()
    {
        return mMaxWAExtentCount ;
    };
    static UInt getMaxWAPageCount( sdtHashSegHdr* aWASegment )
    {
        return aWASegment->mMaxWAExtentCount * SDT_WAEXTENT_PAGECOUNT;
    };

    static ULong getNPageHashMapPoolSize()
    {
        return (ULong)mNHashMapPool.getNodeSize() * mNHashMapPool.getNodeCount();
    };

    static ULong getSegPoolSize()
    {
        return (ULong)mWASegmentPool.getNodeSize() * mWASegmentPool.getNodeCount();
    };

    static UInt getWASegmentUsedPageCount(sdtHashSegHdr* aWASegment )
    {
        return ( aWASegment->mAllocWAPageCount + SDT_WAEXTENT_PAGECOUNT - 1 )
            / SDT_WAEXTENT_PAGECOUNT * SDT_WAEXTENT_PAGECOUNT;
    };
    static UInt   getNExtentCount( sdtHashSegHdr* aWASegment )
    {
        return getRowNExtentCount( aWASegment ) + getSubHashNExtentCount( aWASegment );
    }
    /************************ Interface ***********************/
    static IDE_RC createHashSegment( smiTempTableHeader * aHeader,
                                     ULong                aInitWorkAreaSize );

    static IDE_RC dropHashSegment( sdtHashSegHdr* aWASegment );

    static IDE_RC truncateHashSegment( sdtHashSegHdr* aWASegment );

    static IDE_RC clearHashSegment(sdtHashSegHdr* aWASegment );

    inline static IDE_RC insert( smiTempTableHeader * aHeader,
                                 smiValue   * aValue,
                                 UInt         aHashValue,
                                 scGRID     * aGRID,
                                 idBool     * aResult );
    static IDE_RC append( sdtHashSegHdr     * aWASegment,
                          sdtHashInsertInfo * aTRPInfo,
                          sdtHashInsertResult * aTRInsertResult );

    inline static IDE_RC openHashCursor( smiHashTempCursor * aCursor,
                                         UChar            ** aRow,
                                         scGRID            * aRowGRID );
    inline static IDE_RC openUpdateHashCursor( smiHashTempCursor * aCursor,
                                               UChar            ** aRow,
                                               scGRID            * aRowGRID );
    static IDE_RC openFullScanCursor( smiHashTempCursor * aCursor,
                                      UChar            ** aRow,
                                      scGRID            * aRowGRID );

    static IDE_RC initHashCursor( smiHashTempCursor  * aCursor,
                                  UInt                 aCursorType );
    inline static IDE_RC fetchHashNext( smiHashTempCursor * aCursor,
                                        UChar            ** aRow,
                                        scGRID            * aRowGRID );

    inline static IDE_RC fetchFullNext( smiHashTempCursor  * aCursor,
                                        UChar             ** aRow,
                                        scGRID             * aRowGRID  );

    inline static IDE_RC fetchFromGRID( smiTempTableHeader * aHeader,
                                        scGRID               aGRID,
                                        void               * aDestRowBuf );

    inline static idBool checkHashSlot( smiHashTempCursor* aHashCursor,
                                        UInt               aHashValue );

    inline static idBool checkUpdateHashSlot( smiHashTempCursor* aHashCursor,
                                              UInt               aHashValue );

    inline static void   setHitFlag( smiHashTempCursor * aCursor );
    inline static idBool isHitFlagged( smiHashTempCursor * aCursor );

    inline static IDE_RC update( smiHashTempCursor * aTempCursor,
                                 smiValue          * aValueList );

private:
    /************************ Insert ***********************/
    static IDE_RC appendRowPiece( sdtWCB              * aWCBPtr,
                                  sdtHashInsertInfo   * aTRPInfo,
                                  sdtHashInsertResult * aTRInsertResult );

    static IDE_RC chkUniqueInsertableInMem( smiTempTableHeader  * aHeader,
                                            const smiValue      * aValueList,
                                            UInt                  aHashValue,
                                            sdtHashTRPHdr       * aTRPHeader,
                                            idBool              * aResult );

    static IDE_RC chkUniqueInsertableOutMem( smiTempTableHeader  * aHeader,
                                             const smiValue      * aValueList,
                                             UInt                  aHashValue,
                                             sdtHashSlot         * aHashSlot,
                                             idBool              * aResult );
    static IDE_RC chkUniqueInsertableSingleRow( smiTempTableHeader  * aHeader,
                                                sdtHashSegHdr       * aWASegment,
                                                const smiValue      * aValueList,
                                                scPageID              aSingleRowPageID,
                                                scOffset              aSingleRowOffset,
                                                idBool              * aResult );
    static IDE_RC chkUniqueInsertableSubHash( smiTempTableHeader  * aHeader,
                                              sdtHashSegHdr       * aWASegment,
                                              const smiValue      * aValueList,
                                              UInt                  aHashValue,
                                              scPageID              aSubHashPageID,
                                              scOffset              aSubHashOffset,
                                              idBool              * aResult );
    static IDE_RC allocNewPage( sdtHashSegHdr     * aWASegment,
                                sdtHashInsertInfo * aTRPInfo,
                                sdtWCB           ** aNewWCBPtr );

    static void setInsertHintWCB( sdtHashSegHdr * aSegHdr,
                                  sdtWCB        * aHintWCBPtr );

    static idBool compareRowAndValue( smiTempTableHeader * aHeader,
                                      const void         * aRow,
                                      const smiValue     * aValueList );

    static IDE_RC reserveSubHash( sdtHashSegHdr * aWASegment,
                                  sdtHashGroup  * aGroup,
                                  SInt            aReqMinSlotCount,
                                  sdtSubHash   ** aSubHashPtr,
                                  UInt          * aSlotMaxCount,
                                  scPageID      * aPageID,
                                  scOffset      * aOffset );

    static IDE_RC mergeSubHash( sdtHashSegHdr * aWASegment );
    static IDE_RC mergeSubHashOneSlot( sdtHashSegHdr* aWASegment,
                                       sdtHashSlot  * aHashSlot,
                                       UInt           aSlotIdx );
    static IDE_RC buildSubHash( sdtHashSegHdr * aWASegment );
    static void addSubHashPageFreeOffset( sdtHashSegHdr * aWASegment,
                                          scOffset        aFreeOffset )
    {
        ((sdtHashPageHdr*)aWASegment->mSubHashWCBPtr->mWAPagePtr)->mFreeOffset += aFreeOffset;
    };

    static IDE_RC prepareOutMemoryHash( sdtHashSegHdr * aWASegment );

    static IDE_RC checkExtentTerm4Insert( sdtHashSegHdr    * aWASegment,
                                          sdtHashInsertInfo* aTRPInfo );
    static IDE_RC lackTotalWAPage( sdtHashSegHdr    * aWASegment,
                                   sdtHashInsertInfo* aTRPInfo );

    /*********************** Fetch **************************/
    static void initToHashScan( sdtHashSegHdr     * aWASegment );
    static void initToFullScan( sdtHashSegHdr     * aWASegment );
    static IDE_RC getNextNPageForFullScan( smiHashTempCursor  * aCursor );
    inline static IDE_RC getHashSlotPtr( sdtHashSegHdr  * aWASegment,
                                         UInt    aHashValue,
                                         void ** aSlotPtr );

    inline static IDE_RC fetch( sdtHashSegHdr   * aWASegment,
                                UChar           * aRowBuffer,
                                sdtHashScanInfo * aTRPInfo );

    static IDE_RC fetchChainedRowPiece( sdtHashSegHdr   * aWASegment,
                                        UChar           * aRowBuffer,
                                        sdtHashScanInfo * aTRPInfo );

    static IDE_RC filteringAndFetch( smiHashTempCursor * aTempCursor,
                                     sdtHashTRPHdr     * aTRPHeader,
                                     UChar             * aRow,
                                     idBool            * aResult);

    inline static IDE_RC fetchHashScanInternalPtr(smiHashTempCursor * aCursor,
                                                  sdtHashTRPHdr     * aTRPHeader,
                                                  UChar            ** aRow,
                                                  scGRID            * aRowGRID );
    inline static IDE_RC fetchHashUpdate( smiHashTempCursor * aCursor,
                                          UChar            ** aRow,
                                          scGRID            * aRowGRID );
    static IDE_RC fetchHashScanInternalGRID(smiHashTempCursor * aCursor,
                                            UChar            ** aRow,
                                            scGRID            * aRowGRID );
    static IDE_RC fetchHashScanInternalSubHash( smiHashTempCursor * aCursor,
                                                UChar            ** aRow,
                                                scGRID            * aRowGRID );
    static IDE_RC buildSubHashAndResetInsertInfo( sdtHashSegHdr    * aWASegment,
                                                  sdtHashInsertInfo* aTRPInfo );
    static UInt bSearchSubHash( sdtSubHash * aSubHash, UInt aHashValue );
    static void quickSortSubHash( sdtSubHashKey arr[], SInt left, SInt right );
    /*********************** Update **************************/
    static IDE_RC updateChainedRowPiece( smiHashTempCursor * aTempCursor,
                                         smiValue          * aValueList );

    /******************************** Segment *******************************/

    static IDE_RC expandFreeWAExtent( sdtHashSegHdr* aWASeg );
    static void   clearAllWCBs( sdtHashSegHdr* aWASegment );

    inline static void initWCB( sdtWCB       * aWCBPtr );

    inline static void initTempPage( sdtHashPageHdr  * aPageHdr,
                                     sdtTempPageType   aType,
                                     scPageID          aSelfPID );

    static IDE_RC getFreeWAPage( sdtHashSegHdr * aWASegment,
                                 sdtHashGroup  * aGroup,
                                 sdtWCB       ** aWCBPtr );

    static UInt calcWAExtentCount( ULong aHashAreaSize )
    {
        return (UInt)(( aHashAreaSize + SDT_WAEXTENT_SIZE - 1 ) / SDT_WAEXTENT_SIZE ) ;
    }
    static UInt getWAPageCount()
    {
        return getMaxWAExtentCount() * SDT_WAEXTENT_PAGECOUNT ;
    }

    static UInt calcWAPageCount( ULong aHashAreaSize )
    {
        return calcWAExtentCount( aHashAreaSize) * SDT_WAEXTENT_PAGECOUNT ;
    }
    static ULong getWASegmentSize()
    {
        return calcWASegmentSize( getWAPageCount() );
    }
    static ULong calcWASegmentSize( ULong aWAPageCount )
    {
        return SDT_HASH_SEGMENT_HEADER_SIZE + ( aWAPageCount * ID_SIZEOF(sdtWCB) );
    }

    /*************************** Page 관련 Operation ***************************/
    static void fixWAPage( sdtWCB * aWCBPtr )
    {
        aWCBPtr->mFix++;
        IDE_DASSERT( aWCBPtr->mFix > 0 );
    };
    static void unfixWAPage( sdtWCB * aWCBPtr )
    {
        IDE_DASSERT( aWCBPtr->mFix > 0 );
        aWCBPtr->mFix--;
    };
    static IDE_RC getSlotPtr( sdtHashSegHdr * aWASegment,
                              sdtHashGroup  * aFetchGrp,
                              scPageID        aNPageID,
                              scOffset        aOffset,
                              UChar        ** aNSlotPtr,
                              sdtWCB       ** aFixedWCBPtr );
    inline static sdtWCB * getWCBPtr(sdtHashSegHdr * aWASegment,
                                     scPageID        aWPID )
    {
        IDE_DASSERT( getMaxWAPageCount( aWASegment ) > aWPID );
        return aWASegment->mWCBMap + aWPID ;
    };

    static  IDE_RC setWAPagePtr4HashSlot( sdtHashSegHdr * aWASegment,
                                          sdtWCB        * aWCBPtr );
    static  IDE_RC set64WAPagesPtr( sdtHashSegHdr * aWASegment,
                                    sdtWCB        * aWCBPtr );

    inline static scPageID  getNPID( sdtWCB * aWCBPtr )
    {
        return aWCBPtr->mNPageID;
    }

    static scPageID  getWPageID( sdtHashSegHdr* aWASegment,
                                 sdtWCB       * aWCBPtr )
    {
        return (scPageID)( aWCBPtr - aWASegment->mWCBMap );
    }

    static IDE_RC assignNPage(sdtHashSegHdr* aWASegment,
                              sdtWCB       * aWCBPtr,
                              scPageID       aNPageID);
    static IDE_RC unassignNPage(sdtHashSegHdr* aWASegment,
                                sdtWCB       * aWCBPtr );

    inline static UInt getNPageHashValue( sdtHashSegHdr* aWASegment,
                                          scPageID       aNPID)
    {
        return  ( aNPID ) % aWASegment->mNPageHashBucketCnt;
    };

    static sdtWCB * findWCB( sdtHashSegHdr* aWASegment,
                             scPageID       aNPID);

    static IDE_RC buildNPageHashMap( sdtHashSegHdr* aWASegment );


    static IDE_RC readNPage( sdtHashSegHdr* aWASegment,
                             sdtWCB       * aWCBPtr,
                             scPageID       aNPageID);

    static IDE_RC readNPages( sdtHashSegHdr* aWASegment,
                              scPageID       aFstNPageID,
                              UInt           aPageCount,
                              sdtWCB       * aFstWCBPtr,
                              idBool         aNeedFixPage );

    static IDE_RC preReadNExtents( sdtHashSegHdr * aWASegment,
                                   scPageID        aFstNPageID,
                                   UInt            aPageCount);
    static sdtWCB * getFreePage4PreRead( sdtHashSegHdr * aWASegment,
                                         scPageID        aFstNPageID,
                                         scPageID        aEndNPageID,
                                         sdtWCB        * aPrvWCBPtr );

   static IDE_RC writeNPage( sdtHashSegHdr* aWASegment,
                             sdtWCB       * aWCBPtr );

    static IDE_RC allocAndAssignNPage( sdtHashSegHdr    * aWASegment,
                                       sdtNExtFstPIDList* aNExtFstPIDList,
                                       sdtWCB           * aTargetWCBPtr );

    static UInt   getRowNExtentCount( sdtHashSegHdr* aWASegment )
    {
        return aWASegment->mNExtFstPIDList4Row.mCount;
    }
    static UInt   getSubHashNExtentCount( sdtHashSegHdr* aWASegment )
    {
        return aWASegment->mNExtFstPIDList4SubHash.mCount;
    }

    static UInt allocSlot( sdtHashPageHdr * aPageHdr,
                           UInt             aNeedSize,
                           UChar         ** aSlotPtr );

    static sdtTempPageType getType( sdtHashPageHdr *aPageHdr )
    {
        return (sdtTempPageType)(aPageHdr->mType);
    };

    /* FreeSpace가 끝나는 Offset */
    static UInt getFreeOffset( sdtHashPageHdr *aPageHdr )
    {
        return aPageHdr->mFreeOffset;
    };
    static void setFreeOffset( sdtHashPageHdr *aPageHdr,
                               scOffset        aFreeOffset )
    {
        aPageHdr->mFreeOffset = aFreeOffset;
    };

public:
    /***********************************************************
     * Dump용 함수들
     ***********************************************************/

    static void exportHashSegmentToFile( sdtHashSegHdr* aWASegment );
    static IDE_RC importHashSegmentFromFile( SChar         * aFileName,
                                             sdtHashSegHdr** aHashSegHdr,
                                             sdtHashSegHdr * aRawHashSegHdr,
                                             UChar        ** aPtr );
    static IDE_RC resetWCBInfo4Dump( sdtHashSegHdr * aWASegment,
                                     UChar         * aOldWASegPtr,
                                     UInt            aMaxPageCount,
                                     UChar        ** aPtr );
    static void dumpHashSegment( sdtHashSegHdr * aWASegment,
                                 SChar         * aOutBuf,
                                 UInt            aOutSize );
    static void dumpHashPageHeaders( sdtHashSegHdr  * aWASegment,
                                     SChar          * aOutBuf,
                                     UInt             aOutSize );

    static void dumpWAHashMap( sdtHashSegHdr * aWASeg,
                               SChar         * aOutBuf,
                               UInt            aOutSize );
    static void dumpWAHashPage( void     * aPagePtr,
                                scPageID   aWAPageID,
                                idBool     aTitle,
                                SChar    * aOutBuf,
                                UInt       aOutSize );
    static void dumpTempTRPHeader( void     * aTRPHeader,
                                   SChar    * aOutBuf,
                                   UInt       aOutSize );
    static void dumpTempTRPInfo4Insert( void       * aTRPInfo,
                                        SChar      * aOutBuf,
                                        UInt         aOutSize );
    static void dumpHashTempPage( sdtHashSegHdr  * aWASegment,
                                  scPageID         aWAPageID,
                                  SChar          * aOutBuf,
                                  UInt             aOutSize );
    static void dumpHashRowPage( void   * aPagePtr,
                                 SChar  * aOutBuf,
                                 UInt     aOutSize );

    static void dumpSubHash( sdtSubHash * aSubHash,
                             SChar      * aOutBuf,
                             UInt         aOutSize );

    static void dumpWCBs( void           * aWASegment,
                          SChar          * aOutBuf,
                          UInt             aOutSize );

    static void dumpWCB( void           * aWCB,
                         SChar          * aOutBuf,
                         UInt             aOutSize );

private:
    static iduMutex      mMutex;
    static smuMemStack   mWASegmentPool;
    static smuMemStack   mNHashMapPool;
    static UInt          mMaxWAExtentCount;
    static smiColumn     mBlankColumn;
    static UInt          mSubHashPageCount;
    static UInt          mHashSlotPageCount;
} ;

/**************************************************************************
 * Description : hash value에 해당하는 Hash slot을 가져온다.
 *               없으면 만들어서 가져온다.
 *               Total WA가 부족하면 malloc해서 가져온다.
 *
 * aWASegment  - [IN] Hash Temp Segment
 * aHashValue  - [IN] HashSlot위치를 나타낼 Hash Value
 * aSlotPtr    - [OUT] 반환할 Hash Slot
 ***************************************************************************/
IDE_RC sdtHashModule::getHashSlotPtr( sdtHashSegHdr  * aWASegment,
                                      UInt             aHashValue,
                                      void          ** aSlotPtr )
{
    UInt       sMapIdx;
    sdtWCB   * sWCBPtr;

    sMapIdx = SDT_GET_HASH_MAP_IDX( aHashValue,
                                    aWASegment->mHashSlotCount );

    sWCBPtr = SDT_GET_HASHSLOT_WCB( aWASegment,
                                    sMapIdx );

    if ( sWCBPtr->mWAPagePtr == NULL )
    {
        /* TOTAL WA가 부족해도 1개는 받아온다. */
        IDE_TEST( setWAPagePtr4HashSlot( aWASegment, sWCBPtr ) != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( sWCBPtr->mNxtUsedWCBPtr != NULL );
    }

    *aSlotPtr = (void*) SDT_GET_HASHSLOT_IN_WAPAGE( sWCBPtr, sMapIdx );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 데이터를 삽입한다.
 *               Unique 일 경우 값이 있는지 확인한다.
 *               Unique 더라도, insert 4 update 등의 경우에는
 *               Unique check를 생략 할 수 있다.
*
 * aHeader      - [IN] 대상 Temp Table Header
 * aValue       - [IN] 삽입할 Value
 * aHashValue   - [IN] 삽입할 HashValue
 * aGRID        - [OUT] 삽입한 위치
 * aResult      - [OUT] 삽입이 성공하였는가?( UniqueViolation Check용 )
 ***************************************************************************/
IDE_RC sdtHashModule::insert( smiTempTableHeader  * aHeader,
                              smiValue            * aValue,
                              UInt                  aHashValue,
                              scGRID              * aGRID,
                              idBool              * aResult )
{
    sdtHashSegHdr      * sWASeg = (sdtHashSegHdr*)aHeader->mWASegment;
    sdtHashInsertInfo    sTRPInfo4Insert;
    sdtHashInsertResult  sTRInsertResult;
    sdtHashSlot        * sHashSlot;

    IDE_DASSERT(( sWASeg->mOpenCursorType == SMI_HASH_CURSOR_NONE ) ||
                ( sWASeg->mOpenCursorType == SMI_HASH_CURSOR_HASH_UPDATE ));

    if( aHeader->mCheckCnt > SMI_TT_STATS_INTERVAL )
    {
        IDE_TEST( iduCheckSessionEvent( aHeader->mStatistics ) != IDE_SUCCESS);
        aHeader->mCheckCnt = 0;
    }

    aHeader->mTTState = SMI_TTSTATE_HASH_INSERT;

    IDE_TEST( getHashSlotPtr( sWASeg,
                              aHashValue,
                              (void**)&sHashSlot )
              != IDE_SUCCESS );

    if ( sWASeg->mUnique == SDT_HASH_UNIQUE )
    {
        /* group by는 동일 값이 있는지 먼저 확인하고 insert한다.
         * 그러므로 group by는 동일 값이 있으면 안되고 여기에서는 검증만 한다.
         * property로 검증을 무시 할 수 있다. */
        if (( sWASeg->mOpenCursorType == SMI_HASH_CURSOR_NONE ) ||
            (( sWASeg->mOpenCursorType == SMI_HASH_CURSOR_HASH_UPDATE ) &&
             ( smuProperty::getTmpCheckUnique4Update() == ID_TRUE )))
        {
            /* Insert가 완료 될 때 까지는 small hash set만 구성된다.*/
            if ( SDT_EXIST_SMALL_HASHSET( sHashSlot, aHashValue ) )
            {
                if ( sWASeg->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
                {
                    IDE_TEST( chkUniqueInsertableInMem( aHeader,
                                                        aValue,
                                                        aHashValue,
                                                        (sdtHashTRPHdr*)sHashSlot->mRowPtrOrHashSet,
                                                        aResult )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( chkUniqueInsertableOutMem( aHeader,
                                                         aValue,
                                                         aHashValue,
                                                         sHashSlot,
                                                         aResult )
                              != IDE_SUCCESS );
                }
                IDE_TEST_CONT( *aResult == ID_FALSE , ERR_UNIQUENESS_VIOLATION );
            }
            else
            {
                /* Hash Set에 없으면 없는 Row이다. 바로 Insert하면 된다.*/
            }
        }
    }
    aHeader->mRowCount++;
    *aResult = ID_TRUE;

    sTRPInfo4Insert.mTRPHeader.mTRFlag      = SDT_TRFLAG_CHILDGRID;
    sTRPInfo4Insert.mTRPHeader.mIsRow       = SDT_HASH_ROW_HEADER;
    sTRPInfo4Insert.mTRPHeader.mHitSequence = 0;
    sTRPInfo4Insert.mTRPHeader.mValueLength = 0; /* append하면서 설정됨 */
    sTRPInfo4Insert.mTRPHeader.mHashValue   = aHashValue;

    sTRPInfo4Insert.mTRPHeader.mNextRowPiecePtr = NULL;
    sTRPInfo4Insert.mTRPHeader.mChildRowPtr     = (sdtHashTRPHdr*)sHashSlot->mRowPtrOrHashSet;
    sTRPInfo4Insert.mTRPHeader.mChildPageID     = sHashSlot->mPageID;
    sTRPInfo4Insert.mTRPHeader.mChildOffset     = sHashSlot->mOffset;
    sTRPInfo4Insert.mTRPHeader.mNextPageID      = 0;
    sTRPInfo4Insert.mTRPHeader.mNextOffset      = 0;

    sTRPInfo4Insert.mColumnCount = aHeader->mColumnCount;
    sTRPInfo4Insert.mColumns     = aHeader->mColumns;
    sTRPInfo4Insert.mValueLength = aHeader->mRowSize;
    sTRPInfo4Insert.mValueList   = aValue;
    sTRInsertResult.mComplete    = ID_FALSE;

    IDE_TEST( append( sWASeg,
                      &sTRPInfo4Insert,
                      &sTRInsertResult )
              != IDE_SUCCESS );

    IDE_ERROR( sTRInsertResult.mComplete == ID_TRUE );
    IDE_DASSERT( sTRPInfo4Insert.mTRPHeader.mChildRowPtr == (sdtHashTRPHdr*)sHashSlot->mRowPtrOrHashSet );

    IDE_DASSERT( sTRInsertResult.mHeadRowpiecePtr->mChildPageID == sHashSlot->mPageID );
    IDE_DASSERT( sTRInsertResult.mHeadRowpiecePtr->mChildOffset == sHashSlot->mOffset );
    IDE_DASSERT( sTRInsertResult.mHeadRowpiecePtr->mChildRowPtr == (sdtHashTRPHdr*)sHashSlot->mRowPtrOrHashSet );

    /* 받아둔 hash slot에 설정 */
    SDT_ADD_SMALL_HASHSET( sHashSlot, aHashValue );
    sHashSlot->mPageID          = sTRInsertResult.mHeadPageID;
    sHashSlot->mOffset          = sTRInsertResult.mHeadOffset;
    sHashSlot->mRowPtrOrHashSet = (ULong)sTRInsertResult.mHeadRowpiecePtr;

    IDE_DASSERT(((sdtHashTRPHdr*)sHashSlot->mRowPtrOrHashSet)->mHashValue == aHashValue );

    SC_MAKE_GRID( *aGRID,
                  aHeader->mSpaceID,
                  sTRInsertResult.mHeadPageID,
                  sTRInsertResult.mHeadOffset );

    IDE_EXCEPTION_CONT( ERR_UNIQUENESS_VIOLATION );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smiTempTable::checkAndDump( aHeader );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : hash value에 해당하는 Hash slot이 존재하는지 확인하고
 *               있으면 가져온다.
 *               HASHSET으로 대략적인 유무를 확인 할 수 있어서, 만약
 *               hash value가 없을경우 Hash Set으로 60~90% 미리 알 수 있다.
 *               가장 많이 호출되는 함수로 성능에 신경 써야 한다.
 *               Cursor에 Hash Slot과 Hash Value를 세팅해서 반환한다.
 *
 * aHashCursor - [IN] 탐색에 사용 될 cursor
 * aHashValue  - [IN] 찾을 Hash value 값
 ***************************************************************************/
idBool sdtHashModule::checkHashSlot( smiHashTempCursor* aHashCursor,
                                     UInt               aHashValue )
{
    sdtWCB        * sWCBPtr;
    sdtHashSlot   * sHashSlot;
    UInt            sMapIdx;

    sMapIdx = SDT_GET_HASH_MAP_IDX( aHashValue,
                                    aHashCursor->mHashSlotCount );

    sWCBPtr = SDT_GET_HASHSLOT_WCB( (sdtHashSegHdr*)aHashCursor->mWASegment,
                                    sMapIdx );

    if ( sWCBPtr->mWAPagePtr == NULL )
    {
        return ID_FALSE;
    }

    IDE_DASSERT( sWCBPtr->mNxtUsedWCBPtr != NULL );

    sHashSlot = SDT_GET_HASHSLOT_IN_WAPAGE( sWCBPtr, sMapIdx );

    if ( aHashCursor->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
    {
        /* in memory 일 경우 small hash set 확인 */
        if ( SDT_NOT_EXIST_SMALL_HASHSET( sHashSlot, aHashValue ) )
        {
            return ID_FALSE;
        }
    }
    else
    {
        if ( SDT_IS_SINGLE_ROW( sHashSlot->mOffset ) )
        {
            /* Single Row일 경우 Hash value확인 */
            if ( (UInt)sHashSlot->mRowPtrOrHashSet != aHashValue )
            {
                return ID_FALSE;
            }
        }
        else
        {
            /* out memory 일 경우 large hash set 확인 */
            if ( SDT_NOT_EXIST_LARGE_HASHSET( sHashSlot, aHashValue ) )
            {
                /* Insert가 완료 될 때 까지는 small hash set만 구성되어 있다.*/
                IDE_DASSERT( ((sdtHashSegHdr*)aHashCursor->mWASegment)->mOpenCursorType != SMI_HASH_CURSOR_NONE );
                return ID_FALSE;
            }
        }
    }

    aHashCursor->mHashSlot  = sHashSlot;
    aHashCursor->mHashValue = aHashValue;

    return ID_TRUE;
}

/**************************************************************************
 * Description : hash value에 해당하는 Hash slot이 존재하는지 확인하고
 *               있으면 가져온다. update cursor전용이다.
 *               HASHSET으로 대략적인 유무를 확인 할 수 있어서, 만약
 *               hash value가 없을경우 Hash Set으로 60~90% 미리 알 수 있다.
 *               가장 많이 호출되는 함수로 성능에 신경 써야 한다.
 *               update cursor는 insert와 함께 동작하므로
 *               small hash set 밖에 사용 할 수 없다.
 *               정확도가 조금 떨어진다.
 *
 * aHashCursor - [IN] 탐색에 사용 될 cursor
 * aHashValue  - [IN] 찾을 Hash value 값
 ***************************************************************************/
idBool sdtHashModule::checkUpdateHashSlot( smiHashTempCursor* aHashCursor,
                                           UInt               aHashValue )
{
    sdtWCB      * sWCBPtr;
    sdtHashSlot * sHashSlot;
    UInt          sMapIdx;

    sMapIdx = SDT_GET_HASH_MAP_IDX( aHashValue,
                                    aHashCursor->mHashSlotCount );

    sWCBPtr = SDT_GET_HASHSLOT_WCB( (sdtHashSegHdr*)aHashCursor->mWASegment,
                                    sMapIdx );

    if ( sWCBPtr->mWAPagePtr != NULL )
    {
        IDE_DASSERT( sWCBPtr->mNxtUsedWCBPtr != NULL );

        sHashSlot = SDT_GET_HASHSLOT_IN_WAPAGE( sWCBPtr, sMapIdx );

        /* insert 4 update는 insert와 동시에 select하므로
         * insert 완료 후에 작성하는 large hash set이 없다.*/
        if ( SDT_EXIST_SMALL_HASHSET( sHashSlot, aHashValue ) )
        {
            /* fetch 중에 되도록 cursor에서 완료하고 wa segment를 사용하지 않으려고
             * 따로 빼 둔것인데, insert for update 에서는 fetch도중에 변경 될 수 있어서
             * 어쩔수 없이 여기에서 세팅한다.*/
            aHashCursor->mIsInMemory = ((sdtHashSegHdr*)aHashCursor->mWASegment)->mIsInMemory;

            /* offset이 single row로 나오면 hash value를 비교해본다.
             * single row 가 아니면 open cursor에서 확인한다. */
            if (( aHashCursor->mIsInMemory == SDT_WORKAREA_IN_MEMORY ) ||
                ( SDT_IS_NOT_SINGLE_ROW( sHashSlot->mOffset )) ||
                ( (UInt)sHashSlot->mRowPtrOrHashSet == aHashValue ))
            {
                aHashCursor->mHashSlot  = sHashSlot;
                aHashCursor->mHashValue = aHashValue;
                return ID_TRUE;
            }
        }
    }

    aHashCursor->mWCBPtr = NULL;

    return ID_FALSE;
}

/***************************************************************************
 * Description : Hash Cursor를 Open하고, Cursor에 설정되어 있는
 *               Hash Slot과 Hash Value를 기준으로 첫번 째 row를 가져온다.
 *               만약 이전에 Hash Scan이 아니었다면,
 *               WorkArea 구성을 Hash Scan에 어울리도록 변경한다.
 *               반드시 Insert 가 완료 된 후에야 Hash Cursor를 Open할 수 있으며
 *               한번 Hash Cursor가 open되면 이 후에 Insert 할 수 없다.
 *
 * aCursor  - [IN]  탐색에 사용되는 cursor
 * aRow     - [OUT] 찾은 Row
 * aRowGRID - [OUT] 찾은 Row의 GRID
 ***************************************************************************/
IDE_RC sdtHashModule::openHashCursor( smiHashTempCursor  * aCursor,
                                      UChar             ** aRow,
                                      scGRID             * aRowGRID )
{
    sdtHashSlot   * sHashSlot  = (sdtHashSlot*)aCursor->mHashSlot;
    sdtHashSegHdr * sWASeg;

    // Hash Cursor전용이다.
    IDE_DASSERT( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_HASHSCAN ) );

    IDE_DASSERT( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) );
    /* Hash에서 OrderScan은 불가능함 */
    IDE_DASSERT( SM_IS_FLAG_OFF( aCursor->mTCFlag, SMI_TCFLAG_ORDEREDSCAN ) );
    /* Row의 위치 : page를 fix 하지 않으므로 바뀔수 있어서 사용시 주의 필요 */

    sWASeg = (sdtHashSegHdr*)aCursor->mWASegment;
    if ( sWASeg->mOpenCursorType != SMI_HASH_CURSOR_HASH_SCAN )
    {
        if( aCursor->mIsInMemory == SDT_WORKAREA_OUT_MEMORY )
        {
            initToHashScan( sWASeg );
            aCursor->mTTHeader->mTTState = SMI_TTSTATE_HASH_FETCH_HASHSCAN;
        }
        sWASeg->mOpenCursorType = SMI_HASH_CURSOR_HASH_SCAN;
        aCursor->mTTHeader->mTTState = SMI_TTSTATE_HASH_FETCH_HASHSCAN;
    }

    aCursor->mRowPtr      = NULL;
    aCursor->mChildRowPtr = NULL;
    aCursor->mWAPagePtr   = NULL;

    if ( aCursor->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
    {
        /* InMemory 상태이다, Row Pointer로 탐색한다. */
        aCursor->mChildPageID = sHashSlot->mPageID;
        aCursor->mChildOffset = sHashSlot->mOffset;

        return fetchHashScanInternalPtr( aCursor,
                                         (sdtHashTRPHdr*)sHashSlot->mRowPtrOrHashSet,
                                         aRow,
                                         aRowGRID );
    }
    else
    {
        /* Out Memory 상태이다, ChildPageID, Offset으로 탐색한다. */
        if ( SDT_IS_SINGLE_ROW( sHashSlot->mOffset ) )
        {
            /* CheckHashSlot에서 HashValue를 이미 확인 하였다. */
            IDE_DASSERT( (UInt)sHashSlot->mRowPtrOrHashSet == aCursor->mHashValue );

            /* Single에서 Single flag가 기록되어 있음
             * DISCARD & Single에서만 변환하면 됨 */
            aCursor->mChildPageID = sHashSlot->mPageID;
            aCursor->mChildOffset = SDT_DEL_SINGLE_FLAG_IN_OFFSET( sHashSlot->mOffset );

            aCursor->mSubHashPtr = NULL;
            aCursor->mSubHashIdx = 0;

            return fetchHashScanInternalGRID( aCursor,
                                              aRow,
                                              aRowGRID );
        }
        else
        {
            /* Sub Hash를 기준으로 탐색한다. */
            IDE_DASSERT( SDT_EXIST_LARGE_HASHSET( sHashSlot, aCursor->mHashValue ) );

            aCursor->mSubHashPtr = NULL;
            aCursor->mSubHashIdx = 0;

            aCursor->mChildPageID = sHashSlot->mPageID;
            aCursor->mChildOffset = sHashSlot->mOffset;

            return fetchHashScanInternalSubHash( aCursor,
                                                 aRow,
                                                 aRowGRID );
        }
    }
}

/***************************************************************************
 * Description : Update용 hash scan이다. Hash Cursor와는 다르게 fetch,
 *               update, Insert가 교대로 함께 동작하므로 Pointer, Sub Hash,
 *               Single Row 등을 모두 연속적으로 탐색해야 하며
 *               Cursor open시에 Work Area의 구성이 변경되지 않는다.
 *               주로 Group By등에 사용된다.
 *               역시 첫번째 row도 함께 가져오며 Unique 전용 이므로 Next가 없다.
 *
 * aCursor    - [IN] Hash Temp Cursor
 * aRow       - [OUT] 찾은 Row,
 * aRowGRID   - [OUT] 찾은 Row의 GRID
 ***************************************************************************/
IDE_RC sdtHashModule::openUpdateHashCursor( smiHashTempCursor  * aCursor,
                                            UChar             ** aRow,
                                            scGRID             * aRowGRID )
{
    // Hash Cursor전용이다.
    IDE_DASSERT( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_HASHSCAN ) );

    IDE_DASSERT( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) );
    /* Hash에서 OrderScan은 불가능함 */
    IDE_DASSERT( SM_IS_FLAG_OFF( aCursor->mTCFlag, SMI_TCFLAG_ORDEREDSCAN ) );

    aCursor->mRowPtr      = NULL;
    aCursor->mChildRowPtr = NULL;

    /* Cache 정보들 : UniqueHash 는 사용하지 않는다. */
    aCursor->mWAPagePtr = NULL;

    return fetchHashUpdate( aCursor,
                            aRow,
                            aRowGRID );
}

/**************************************************************************
 * Description : 다음 Row를 받아온다. Hash Scan전용이다.
 *               Unique는 Next가 없으므로 사용되지 않는다..
 *
 * aCursor  - [IN] 대상 Cursor
 * aRow     - [OUT] 찾은 Row,
 * aRowGRID - [OUT] 찾은 Row의 GRID
 ***************************************************************************/
IDE_RC sdtHashModule::fetchHashNext( smiHashTempCursor * aCursor,
                                     UChar            ** aRow,
                                     scGRID            * aRowGRID )
{
    IDE_DASSERT( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_HASHSCAN ) );

    IDE_DASSERT( aCursor->mWAPagePtr == NULL );
    IDE_DASSERT( ((sdtHashSegHdr*)aCursor->mWASegment)->mOpenCursorType == SMI_HASH_CURSOR_HASH_SCAN );

    /* 앞서 읽은 row가 있고, 같은 value를 가진 Next Row를 찾으러 왔다. */
    IDE_DASSERT( aCursor->mChildPageID != SM_NULL_PID );

    if ( aCursor->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
    {
        return fetchHashScanInternalPtr( aCursor,
                                         (sdtHashTRPHdr*)aCursor->mChildRowPtr,
                                         aRow,
                                         aRowGRID );
    }
    else
    {
        return fetchHashScanInternalSubHash( aCursor,
                                             aRow,
                                             aRowGRID );
    }
    /* Fetch Next에는 Single Row 탐색이 없다.
     * Single Row는 Hash Slot에서 Row를 가리킬 때나,
     * 혹은 Unique Insert 확인 할 때나, updaet cursor에서,
     * In Memory Row의 마지막 row에서 Disk에 저장 된 Next Row를
     * 가리킬 경우에 존재한다.
     * 일반 적인 Hash Scan에서는 alloc cursor 시점에
     * In Memory Row를 남기지 않고 모두 Sub Hash로 만들어 버렸고
     * 또 Fetch First에서 Hash Slot에서 가리키는 Row 는 이미 읽었다.
     * 그러므로 Fetch Next에서는 Single Row는 나오지 않는다. */
}

/**************************************************************************
 * Description : Cursor정보를 기준으로 Row를 받아온다.
 *               InMemory 상태에서 Row Pointer를 따라가며 탐색한다.
 *               First, Next에 모두 사용된다. fetch update에서는 사용되지 않는다.
 *
 * aCursor     - [IN] 대상 Cursor
 * aTRPHeader  - [IN] 탐색 할 첫번째 Row
 * aRow        - [OUT] 찾은 Row
 * aRowGRID    - [OUT] 찾은 Row의 GRID
 ***************************************************************************/
IDE_RC sdtHashModule::fetchHashScanInternalPtr( smiHashTempCursor * aCursor,
                                                sdtHashTRPHdr     * aTRPHeader,
                                                UChar            ** aRow,
                                                scGRID            * aRowGRID )
{
    idBool          sResult = ID_FALSE;
    sdtHashTRPHdr * sPrvTRPHeader = NULL;
    UInt            sLoop;

    if( aCursor->mTTHeader->mCheckCnt > SMI_TT_STATS_INTERVAL )
    {
        IDE_TEST( iduCheckSessionEvent( aCursor->mTTHeader->mStatistics ) != IDE_SUCCESS);
        aCursor->mTTHeader->mCheckCnt = 0;
    }

    sLoop = 0;

    // 최초 탐색
    do
    {
        sLoop++;
        IDE_DASSERT( SM_IS_FLAG_ON( aTRPHeader->mTRFlag, SDT_TRFLAG_CHILDGRID ) );
        IDE_DASSERT( SM_IS_FLAG_ON( aTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

        if ( aTRPHeader->mHashValue == aCursor->mHashValue )
        {
            IDE_TEST( filteringAndFetch( aCursor,
                                         aTRPHeader,
                                         *aRow,
                                         &sResult )
                      != IDE_SUCCESS );

            if ( sResult == ID_TRUE )
            {
                break;
            }
        }
        else
        {
            // HashValue조차 다름
        }

        sPrvTRPHeader = aTRPHeader;
        aTRPHeader    = sPrvTRPHeader->mChildRowPtr ;

    } while( aTRPHeader != NULL );

    if ( sResult == ID_FALSE )
    {
        SC_MAKE_NULL_GRID(*aRowGRID);
        *aRow = NULL;
    }
    else
    {
        IDE_DASSERT( aTRPHeader != NULL );
        IDE_DASSERT( aCursor->mRowPtr != NULL );

        if ( sPrvTRPHeader != NULL )
        {
            SC_MAKE_GRID( *aRowGRID,
                          aCursor->mTTHeader->mSpaceID,
                          sPrvTRPHeader->mChildPageID,
                          sPrvTRPHeader->mChildOffset );
        }
        else
        {
            /* 첫번 째 Row가 정답인 경우 */
            SC_MAKE_GRID( *aRowGRID,
                          aCursor->mTTHeader->mSpaceID,
                          aCursor->mChildPageID,
                          aCursor->mChildOffset );
        }

        aCursor->mChildPageID = aTRPHeader->mChildPageID;
        aCursor->mChildOffset = aTRPHeader->mChildOffset;

        aCursor->mChildRowPtr = (UChar*)aTRPHeader->mChildRowPtr;
    }

    aCursor->mTTHeader->mStatsPtr->mExtraStat2 = IDL_MAX( aCursor->mTTHeader->mStatsPtr->mExtraStat2, sLoop );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smiTempTable::checkAndDump( aCursor->mTTHeader );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Cursor정보를 기준으로 Row를 받아온다.
 *               InMemory 상태에서 Row Pointer를 따라가며 탐색한다.
 *               fetch update 전용으로 사용된다.
 *
 * aCursor  - [IN] 대상 Cursor
 * aRow     - [OUT] 찾은 Row
 ***************************************************************************/
IDE_RC sdtHashModule::fetchHashUpdate( smiHashTempCursor * aCursor,
                                       UChar            ** aRow,
                                       scGRID            * aRowGRID )
{
    sdtHashTRPHdr * sTRPHeader;
    sdtHashTRPHdr * sPrvTRPHeader;
    sdtWCB        * sWCBPtr = NULL;
    idBool          sResult;
    UInt            sLoop;
    sdtHashSlot   * sHashSlot  = (sdtHashSlot*)aCursor->mHashSlot;

    /* hash slot 에 row가 반드시 있다. null pid나 offset은 아니다. */
    IDE_ASSERT( SDT_EXIST_SMALL_HASHSET( sHashSlot, aCursor->mHashValue ) );
    IDE_ERROR( ((sdtHashSegHdr*)aCursor->mWASegment)->mOpenCursorType == SMI_HASH_CURSOR_HASH_UPDATE );

    sLoop = 0;

    /* next row ptr이거나 single row의 hash value이다. */
    sTRPHeader = (sdtHashTRPHdr*)sHashSlot->mRowPtrOrHashSet;

    if (( SDT_IS_NOT_SINGLE_ROW( sHashSlot->mOffset )) &&
        ( sTRPHeader != NULL ))
    {
        sResult = ID_FALSE;
        sPrvTRPHeader = NULL;
        // 최초 탐색
        do
        {
            sLoop++;
            IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_CHILDGRID ) );
            IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

            if ( sTRPHeader->mHashValue == aCursor->mHashValue )
            {
                IDE_TEST( filteringAndFetch( aCursor,
                                             sTRPHeader,
                                             *aRow,
                                             &sResult )
                          != IDE_SUCCESS );

                if ( sResult == ID_TRUE )
                {
                    break;
                }
            }
            else
            {
                // HashValue조차 다름
            }

            sPrvTRPHeader = sTRPHeader;
            sTRPHeader    = sPrvTRPHeader->mChildRowPtr ;

            // Next Offset이 Single Row인지 확인
            if ( SDT_IS_SINGLE_ROW( sPrvTRPHeader->mChildOffset ) )
            {
                break;
            }

        } while( sTRPHeader != NULL );

        /* 찾았다. */
        if ( sResult == ID_TRUE )
        {
            IDE_DASSERT( aCursor->mRowPtr != NULL );

            aCursor->mChildPageID = sTRPHeader->mChildPageID;
            aCursor->mChildOffset = sTRPHeader->mChildOffset;

            if ( sPrvTRPHeader != NULL )
            {
                SC_MAKE_GRID( *aRowGRID,
                              aCursor->mTTHeader->mSpaceID,
                              sPrvTRPHeader->mChildPageID,
                              sPrvTRPHeader->mChildOffset );
            }
            else
            {
                /* 첫번째 ptr인 경우 */
                SC_MAKE_GRID( *aRowGRID,
                              aCursor->mTTHeader->mSpaceID,
                              sHashSlot->mPageID,
                              sHashSlot->mOffset );
            }

            if ( ((sdtHashSegHdr*)aCursor->mWASegment)->mNPageHashPtr != NULL )
            {
                /* fetchHashScanInternalPtr 는 mWCBPtr을 세팅하지 않는다.
                 * In Memory에서는 mWCBPtr을 설정하지 않아도 된다.
                 * update 등에서 wcb를 참고하여 dirty 및 write를 하게 된다.
                 */
                sWCBPtr = findWCB( (sdtHashSegHdr*)aCursor->mWASegment,
                                   aRowGRID->mPageID );
                IDE_ASSERT( sWCBPtr != NULL );
                /* mWCBPtr의 변경은 아랫쪽에서 한다. */
            }

            /* 찾았으니 나머지 작업 생략*/
            IDE_CONT( CONT_COMPLETE );
        }
        else
        {
            /* 찾지 못했으면 sPrvTRPHeader는 반드시 존재한다.*/
            aCursor->mChildPageID = sPrvTRPHeader->mChildPageID;
            aCursor->mChildOffset = sPrvTRPHeader->mChildOffset;
        }
    }
    else
    {
        /* single row를 확인해야 하는 경우 이거나
         * sub hash에서 뒤져야 하는 경우 */
        aCursor->mChildPageID = sHashSlot->mPageID;
        aCursor->mChildOffset = sHashSlot->mOffset;
    }

    if ( aCursor->mChildPageID != SD_NULL_PID )
    {
        if ( SDT_IS_SINGLE_ROW( aCursor->mChildOffset ) )
        {
            if ( (UInt)((ULong)sTRPHeader) == aCursor->mHashValue )
            {
                /* Single Row 하나 있는 경우 */
                aCursor->mChildOffset = SDT_DEL_SINGLE_FLAG_IN_OFFSET( aCursor->mChildOffset );

                return fetchHashScanInternalGRID( aCursor,
                                                  aRow,
                                                  aRowGRID );
            }
            else
            {
                /* single row이지만 mismatch
                 * HashSlot에서 바로 오는 경우라면 HashValue가 다를 리 없다.
                 * checkHashSlotUpdate에서 이미 확인했기 때문이다.
                 * 하지만, Row pointer list의 마지막 Row의 Next로 왔을수도 있다.
                 * 이 경우에는 HashValue를 확인해야 한다.*/
                aCursor->mChildPageID = SD_NULL_PID;
                aCursor->mChildOffset = 0;
                SC_MAKE_NULL_GRID(*aRowGRID);
                *aRow = NULL;
            }
        }
        else
        {
            aCursor->mSubHashPtr = NULL;
            aCursor->mSubHashIdx = 0;

            return fetchHashScanInternalSubHash( aCursor,
                                                 aRow,
                                                 aRowGRID );
        }
    }
    else
    {
        /* 찾지 못했다. */
        SC_MAKE_NULL_GRID(*aRowGRID);
        *aRow = NULL;
    }

    IDE_EXCEPTION_CONT( CONT_COMPLETE );

    aCursor->mWCBPtr = sWCBPtr;

    aCursor->mTTHeader->mStatsPtr->mExtraStat2 = IDL_MAX( aCursor->mTTHeader->mStatsPtr->mExtraStat2, sLoop );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smiTempTable::checkAndDump( aCursor->mTTHeader );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서로부터 모든 Row를 대상으로 FetchNext로 가져옵니다.
 *
 * aCursor  - [IN] 대상 Cursor
 * aRow     - [OUT] 대상 Row
 ***************************************************************************/
IDE_RC sdtHashModule::fetchFullNext( smiHashTempCursor * aCursor,
                                     UChar            ** aRow,
                                     scGRID            * aRowGRID )
{
    sdtHashPageHdr * sPageHdr;
    scOffset         sCurOffset;
    scOffset         sNxtOffset;
    sdtHashTRPHdr  * sTRPHeaderPtr;
    idBool           sResult      = ID_FALSE;
    idBool           sIsFixedPage = ID_FALSE;

    IDE_DASSERT( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FULLSCAN ) );
    IDE_ERROR( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FULLSCAN ) );
    IDE_DASSERT( ((sdtHashSegHdr*)aCursor->mWASegment)->mOpenCursorType == SMI_HASH_CURSOR_FULL_SCAN );

    if( aCursor->mTTHeader->mCheckCnt > SMI_TT_STATS_INTERVAL )
    {
        IDE_TEST( iduCheckSessionEvent( aCursor->mTTHeader->mStatistics ) != IDE_SUCCESS);
        aCursor->mTTHeader->mCheckCnt = 0;
    }

    while( aCursor->mWAPagePtr != NULL )
    {
        sPageHdr = (sdtHashPageHdr*)aCursor->mWAPagePtr;

        IDE_DASSERT_MSG((sPageHdr->mSelfNPID == aCursor->mChildPageID ),
                        "%d != %d\n",
                        sPageHdr->mSelfNPID ,
                        aCursor->mChildPageID);

        IDE_ASSERT( sPageHdr->mFreeOffset <= SD_PAGE_SIZE );

        /* Page안의 Slot들을 탐색함 */
        for( sCurOffset = aCursor->mChildOffset ;
             sCurOffset < sPageHdr->mFreeOffset ;
             sCurOffset = sNxtOffset )
        {
            IDE_DASSERT( sCurOffset + ID_SIZEOF(sdtHashTRPHdr) < SD_PAGE_SIZE );
            sTRPHeaderPtr = (sdtHashTRPHdr*)((UChar*)sPageHdr + sCurOffset );

            IDE_DASSERT( sTRPHeaderPtr->mValueLength < SD_PAGE_SIZE );
            IDE_DASSERT( sTRPHeaderPtr->mIsRow == SDT_HASH_ROW_HEADER );

            sNxtOffset = idlOS::align8( sCurOffset +
                                        ID_SIZEOF( sdtHashTRPHdr ) +
                                        sTRPHeaderPtr->mValueLength );

            if ( SM_IS_FLAG_OFF( sTRPHeaderPtr->mTRFlag, ( SDT_TRFLAG_HEAD | SDT_TRFLAG_CHILDGRID ) ) )
            {
                /* Header Row Piece가 아니면 제외한다. */
                continue;
            }

            if ( aCursor->mIsInMemory == SDT_WORKAREA_OUT_MEMORY )
            {
                sIsFixedPage = ID_TRUE;
                fixWAPage( (sdtWCB*)(aCursor->mWCBPtr) );
            }

            /* aCursor->mGRID 만 셋팅하면 된다.
             * mChileGRID, mChileRowPtr 은 필요없다*/
            IDE_TEST( filteringAndFetch( aCursor,
                                         sTRPHeaderPtr,
                                         *aRow,
                                         &sResult )
                      != IDE_SUCCESS );

            if ( sIsFixedPage == ID_TRUE )
            {
                sIsFixedPage = ID_FALSE;
                unfixWAPage( (sdtWCB*)(aCursor->mWCBPtr) );
            }

            if ( sResult == ID_TRUE )
            {
                /* 찾으면 바로 return한다. */
                aCursor->mChildOffset = sNxtOffset ;

                SC_MAKE_GRID( *aRowGRID,
                              aCursor->mTTHeader->mSpaceID,
                              aCursor->mChildPageID,
                              sCurOffset );

                IDE_CONT( SKIP );
            }
        }

        /* Next Page 를 가져온다. */
        if ( aCursor->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
        {
            /* InMemory영역이다. WCB Array를 기준으로 Next Page를 가져온다.*/
            aCursor->mWCBPtr = ((sdtWCB*)aCursor->mWCBPtr)+ 1;

            if ( aCursor->mWCBPtr < aCursor->mEndWCBPtr )
            {
                aCursor->mWAPagePtr   = ((sdtWCB*)aCursor->mWCBPtr)->mWAPagePtr;
                aCursor->mChildPageID = ((sdtWCB*)aCursor->mWCBPtr)->mNPageID;
                aCursor->mChildOffset = ID_SIZEOF(sdtHashPageHdr);
            }
            else
            {
                /* 마지막 page까지 읽었다. */
                aCursor->mWCBPtr    = NULL;
                aCursor->mWAPagePtr = NULL;
            }
        }
        else
        {
            /* Out Memory영역이다. Normal Extent를 기준으로 Next Page를 읽어온다.*/
            IDE_TEST( getNextNPageForFullScan( aCursor ) != IDE_SUCCESS );
        }
    }

    /*더이상 next가 없는 경우이다. 모두 정리 해 준다. */
    aCursor->mChildPageID = SM_NULL_PID ;
    aCursor->mChildOffset = 0 ;
    aCursor->mRowPtr      = NULL;
    aCursor->mSeq         = aCursor->mHashSlotCount;

    SC_MAKE_NULL_GRID(*aRowGRID);
    *aRow = NULL;

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smiTempTable::checkAndDump( aCursor->mTTHeader );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : GRID를 기준으로 row를 읽어온다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aGRID        - [IN] 읽어들일 Row의 GRID
 * aDestRowBuf  - [IN] 쪼개진 Column을 저장하기 위한 Buffer
 ***************************************************************************/
IDE_RC sdtHashModule::fetchFromGRID( smiTempTableHeader * aHeader,
                                     scGRID               aGRID,
                                     void               * aDestRowBuf )
{
    sdtHashSegHdr      * sWASeg  = (sdtHashSegHdr*)aHeader->mWASegment;
    sdtHashScanInfo      sTRPInfo;
    UChar              * sPtr;
    sdtWCB             * sWCBPtr;
    idBool               sIsFixedPage = ID_FALSE;

    IDE_DASSERT( ( SC_MAKE_SPACE( aGRID ) != SDT_SPACEID_WAMAP ) &&
                 ( SC_MAKE_SPACE( aGRID ) != SDT_SPACEID_WORKAREA ) );

    if ( sWASeg->mNPageHashPtr == NULL )
    {
        /* GRID를 기준으로 탐색하므로 Hash Map이 필요하다.
         * 없으면 만든다. */
        IDE_TEST( sdtHashModule::buildNPageHashMap( sWASeg ) != IDE_SUCCESS );
    }
    IDU_FIT_POINT( "BUG-49107@sdtHashModule::fetchFromGRID" );
    IDE_TEST( sdtHashModule::getSlotPtr( sWASeg,
                                         &sWASeg->mFetchGrp,
                                         aGRID.mPageID,
                                         aGRID.mOffset,
                                         &sPtr,
                                         &sWCBPtr )
              != IDE_SUCCESS );

    IDE_DASSERT( SM_IS_FLAG_ON( ((sdtHashTRPHdr*)sPtr)->mTRFlag, SDT_TRFLAG_HEAD ) );

    sTRPInfo.mTRPHeader      = (sdtHashTRPHdr*)sPtr;
    sTRPInfo.mFetchEndOffset = aHeader->mRowSize;

    if ( sWASeg->mIsInMemory == SDT_WORKAREA_OUT_MEMORY )
    {
        sdtHashModule::fixWAPage( sWCBPtr );
        sIsFixedPage = ID_TRUE;
    }

    IDE_TEST( sdtHashModule::fetch( sWASeg,
                                    (UChar*)aDestRowBuf,
                                    &sTRPInfo )
              != IDE_SUCCESS );

    if ( sIsFixedPage == ID_TRUE )
    {
        sIsFixedPage = ID_FALSE;
        sdtHashModule::unfixWAPage( sWCBPtr );
    }

    if ( aDestRowBuf != sTRPInfo.mValuePtr )
    {
        IDE_DASSERT( SM_IS_FLAG_OFF( ((sdtHashTRPHdr*)sPtr)->mTRFlag, SDT_TRFLAG_NEXTGRID ) );

        idlOS::memcpy( aDestRowBuf,
                       sTRPInfo.mValuePtr,
                       sTRPInfo.mValueLength );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smiTempTable::checkAndDump( aHeader );

    if ( sIsFixedPage == ID_TRUE )
    {
        sdtHashModule::unfixWAPage( sWCBPtr );
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * page에 존재하는 Row 전체를 rowInfo형태로 읽는다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aRowBuffer   - [IN] 쪼개진 Column을 저장하기 위한 Buffer
 * aTRPInfo     - [OUT] Fetch한 결과
 ***************************************************************************/
IDE_RC sdtHashModule::fetch( sdtHashSegHdr    * aWASegment,
                             UChar            * aRowBuffer,
                             sdtHashScanInfo  * aTRPInfo )
{
    UChar   sFlag = aTRPInfo->mTRPHeader->mTRFlag;

    IDE_DASSERT( SM_IS_FLAG_ON( sFlag, SDT_TRFLAG_HEAD ) );

    if ( SM_IS_FLAG_OFF( sFlag, SDT_TRFLAG_NEXTGRID ) )
    {
        aTRPInfo->mValuePtr    = ((UChar*)aTRPInfo->mTRPHeader) + SDT_HASH_TR_HEADER_SIZE_FULL ;
        aTRPInfo->mValueLength = aTRPInfo->mTRPHeader->mValueLength;

        return IDE_SUCCESS;
    }
    else
    {
        return fetchChainedRowPiece( aWASegment,
                                     aRowBuffer,
                                     aTRPInfo );
    }
}

/**************************************************************************
 * Description :
 * Row의 특정 Column을 갱신한다.
 * Update할 Row는 처음 삽입할때부터 충분한 공간을 확보했기 때문에 반드시
 * Row의 구조 변경 없이 성공한다
 *
 * aTempCursor    - [IN] 대상 커서
 * aValue         - [IN] 갱신할 Value
 ***************************************************************************/
IDE_RC sdtHashModule::update( smiHashTempCursor * aTempCursor,
                              smiValue          * aValueList )
{
    const smiColumnList * sUpdateColumn;
    const smiColumn    * sColumn;
    UChar              * sRowPos;
    sdtWCB             * sWCBPtr;
    sdtHashTRPHdr      * sTRPHeader;

    sTRPHeader = (sdtHashTRPHdr*)aTempCursor->mRowPtr;

    IDE_ERROR( sTRPHeader != NULL );
    IDE_ERROR( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

    /* full scan이거나, group by용 insert with scan이거나,
     * 다른 경우가 추가되면, write n page 에 대해서 고민해야 한다.*/

    if ( sTRPHeader->mValueLength >= aTempCursor->mUpdateEndOffset )
    {
        sRowPos = aTempCursor->mRowPtr + SDT_HASH_TR_HEADER_SIZE_FULL ;
        sUpdateColumn = aTempCursor->mUpdateColumns;

        while( sUpdateColumn != NULL )
        {
            sColumn = ( smiColumn*)sUpdateColumn->column;

            IDE_ERROR( (sColumn->offset + aValueList->length )
                       <= sTRPHeader->mValueLength ); // 넘어서면 잘못된 것이다.

            idlOS::memcpy( sRowPos + sColumn->offset,
                           ((UChar*)aValueList->value),
                           aValueList->length );

            aValueList++;
            sUpdateColumn = sUpdateColumn->next;
        }

        /* 1. in memory에서는 mWCBPtr이 null로 온다, out memory이거나,
         *    insert with scan 상황에서 마지막 extent까지 확장 한 경우에
         *    mWCBPtr에 설정이 되어서 오며, 이 때 에는 반드시 dirty 해줘야 한다.
         * 2. full scan에서는 dirty만 한다,
         *    순서대로 접근하므로, 나중에 extent단위로 모아서 Write 한다.
         * */
        if ( aTempCursor->mWCBPtr != NULL )
        {
            sWCBPtr = (sdtWCB*)aTempCursor->mWCBPtr;

            IDE_DASSERT( sWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );

            sWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
        }
        else
        {
            IDE_DASSERT( aTempCursor->mIsInMemory == SDT_WORKAREA_IN_MEMORY );
        }
    }
    else
    {
        IDE_TEST( updateChainedRowPiece( aTempCursor,
                                         aValueList ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smiTempTable::checkAndDump( aTempCursor->mTTHeader );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * HitFlag를 설정합니다.
 *
 * aCursor      - [IN] 갱신할 Row를 가리키는 커서
 * aHitSeq      - [IN] 설정할 Hit값
 ***************************************************************************/
void sdtHashModule::setHitFlag( smiHashTempCursor * aCursor )
{
    sdtHashTRPHdr * sTRPHeader = (sdtHashTRPHdr*)aCursor->mRowPtr;
    UInt            sHitSeq    = aCursor->mTTHeader->mHitSequence;
    sdtWCB        * sWCBPtr;

    IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );
    IDE_DASSERT( ID_SIZEOF( sTRPHeader->mHitSequence ) == ID_SIZEOF( sHitSeq ) );

    if ( sTRPHeader->mHitSequence != sHitSeq )
    {
        sTRPHeader->mHitSequence = sHitSeq;

        if ( aCursor->mWCBPtr != NULL )
        {
            sWCBPtr = (sdtWCB*)aCursor->mWCBPtr;

            IDE_DASSERT( sWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );

            sWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
        }
        else
        {
            IDE_ASSERT( aCursor->mIsInMemory == SDT_WORKAREA_IN_MEMORY );
        }
    }
    else
    {
        /* ( sTRPHeader->mHitSequence == aHitSeq )
         * 값이 같으면 처리 할 필요없다. */
    }
    return;
}

/**************************************************************************
 * Description : HitFlag를 검사하고 그 결과를 Return한다.
 *
 * aCursor  - [IN] 확인 할 현제 Row의 위치를 가리키는 Cursor
 ***************************************************************************/
idBool sdtHashModule::isHitFlagged( smiHashTempCursor * aCursor )
{
    IDE_DASSERT( ID_SIZEOF( ((sdtHashTRPHdr*)aCursor->mRowPtr)->mHitSequence ) ==
                 ID_SIZEOF( aCursor->mTTHeader->mHitSequence ) );

    if ( ((sdtHashTRPHdr*)aCursor->mRowPtr)->mHitSequence ==
         aCursor->mTTHeader->mHitSequence )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

#endif /* _O_SDT_HASH_MODULE_H_ */
