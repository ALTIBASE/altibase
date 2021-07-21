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
 * $Id: sdpstFT.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Fixed Table에 대한 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_FT_H_
# define _O_SDPST_FT_H_ 1

# include <sdpstDef.h>
# include <sdpstLfBMP.h>

# define SDPST_FT_VARCHAR_LEN   (3 + 1)

typedef IDE_RC (*sdpstFTDumpCallbackFunc)( void                 * aHeader,
                                           iduFixedTableMemory  * aMemory, 
                                           sdpSegType             aSegType,
                                           UChar                * aPagePtr,
                                           idBool               * aIsLast );

/*
 * Dump Segment Header
 */
typedef struct sdpstDumpSegHdrInfo
{
    scPageID      mSegPID;            // SEG_PID
    sdpSegState   mSegState;          // SEG_STATE
    SChar         mSegType;           // SEG_TYPE

    UInt          mTotExtCnt;         // TOT_EXT_CNT
    ULong         mTotPageCnt;        // TOT_PAGE_CNT
    ULong         mFmtPageCnt;        // FMT_PAGE_CNT
    ULong         mFreeIndexPageCnt;  // FREE_INDEX_PAGE_CNT
    UInt          mTotExtDirCnt;      // TOT_EXTDIR_CNT
    UInt          mTotRtBMPCnt;       // TOT_RTBMP_CNT

    scPageID      mLstExtDir;         // LAST_EXTDIR_PAGEID
    scPageID      mLstRtBMP;          // LAST_RTBMP_PAGEID
    scPageID      mLstItBMP;          // LAST_ITBMP_PAGEID
    scPageID      mLstLfBMP;          // LAST_LFBMP_PAGEID
    ULong         mLstSeqNo;          // LAST_SEQNO

    scPageID      mHPID;              // HWM_PID
    scPageID      mHExtDirPID;        // HWM_EXTDIR_PAGEID
    SShort        mHSlotNoInExtDir;   // HWM_EXTDESC_SLOTNO
    scPageID      mHVtBMP;            // HWM_VTBMP_PAGEID
    SShort        mHSlotNoInVtBMP;    // HWM_VTBMP_SLOTNO
    scPageID      mHRtBMP;            // HWM_RTBMP_PAGEID
    SShort        mHSlotNoInRtBMP;    // HWM_RTBMP_SLOTNO
    scPageID      mHItBMP;            // HWM_ITBMP_PAGEID
    SShort        mHSlotNoInItBMP;    // HWM_ITBMP_SLOTNO
    scPageID      mHLfBMP;            // HWM_LFBMP_PAGEID
    SShort        mHPBSNoInLfBMP;     // HWM_LFBMP_PBSNO
} sdpstDumpSegHdrInfo;

/*
 * Dump BMP Structure Info
 */
typedef struct sdpstDumpBMPStructureInfo
{
    SChar               mSegType;
    scPageID            mRtBMP;
    scPageID            mItBMP;
    scPageID            mLfBMP;
    UChar               mLfBMPMFNLStr[SDPST_FT_VARCHAR_LEN];
} sdpstDumpBMPStructureInfo;

/*
 * Dump BMP Hdr Info (common)
 */
typedef struct sdpstDumpBMPHdrInfo
{
    SChar               mSegType;
    scPageID            mPageID;
    sdpParentInfo       mParentInfo;
    UChar               mTypeStr[SDPST_FT_VARCHAR_LEN];
    UChar               mMFNLStr[SDPST_FT_VARCHAR_LEN];
    scPageID            mNxtRtBMP;
    UShort              mMFNLTbl[SDPST_MFNL_MAX];
    UShort              mSlotCnt;
    UShort              mFreeSlotCnt;
    UShort              mMaxSlotCnt;
    SShort              mFstFreeSlotNo;
} sdpstDumpBMPHdrInfo;

/*
 * Dump BMP Map Info (common)
 */
typedef struct sdpstDumpBMPBodyInfo
{
    SChar               mSegType;
    scPageID            mPageID;
    scPageID            mBMP;
    UChar               mMFNLStr[SDPST_FT_VARCHAR_LEN];
} sdpstDumpBMPBodyInfo;

/*
 * Dump LfBMP Hdr Info
 */
typedef struct sdpstDumpLfBMPHdrInfo
{
    sdpstDumpBMPHdrInfo mCommon;
    sdpstPageRange      mPageRange;
    UShort              mTotPageCnt;
    SShort              mFstDataPagePBSNo;
} sdpstDumpLfBMPHdrInfo;

/* 하나의 PBS은 3바이트로 표현한다. */
# define SDPST_MAX_BITSET_BUFF_LEN  (SDPST_PAGE_BITSET_TABLE_SIZE * 3)
/*
 * Dump Segment Leaf Bitmap for RangeSlot
 */
typedef struct sdpstDumpLfBMPRangeSlotInfo
{
    SChar               mSegType;
    scPageID            mPageID;

    scPageID            mFstPID;
    UShort              mLength;
    SShort              mFstPBSNo;

    scPageID            mExtDirPID;
    SShort              mSlotNoInExtDir;
    SShort              mRangeSlotNo;
} sdpstDumpLfBMPRangeSlotInfo;

/*
 * Dump Segment Leaf Bitmap for PBS Table
 */
typedef struct sdpstDumpLfBMPPBSTblInfo
{
    SChar               mSegType;
    scPageID            mPageID;
    UChar               mPBSStr[SDPST_MAX_BITSET_BUFF_LEN];
} sdpstDumpLfBMPPBSTblInfo;

/*
 * Dump Segment Cache
 */
typedef struct sdpstDumpSegCacheInfo
{
    SChar               mSegType;
    scPageID            mSegPID;
    scPageID            mTmpSegHeadPID;
    scPageID            mTmpSegTailPID;
    ULong               mSize;
    UInt                mPageCntInExt;
    scPageID            mMetaPID;
    smOID               mTableOID;
    ULong               mFmtPageCnt;
    SChar               mUseLstAllocPageHint;
    scPageID            mLstAllocPID;
    ULong               mLstAllocSeqNo;
} sdpstDumpSegCacheInfo;


class sdpstFT
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    /* D$TABLE_TMS_SEG_HDR */
    static IDE_RC buildRecord4SegHdr( idvSQL              * /*aStatistics*/,
                                      void                * aHeader,
                                      void                * aDumpObj,
                                      iduFixedTableMemory * aMemory );

    /* D$TABLE_TMS_BMPHDRSTRUCTURE */
    static IDE_RC buildRecord4BMPStructure(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

    /* D$TABLE_TMS_RTBMPHDR */
    static IDE_RC buildRecord4RtBMPHdr(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

    /* D$TABLE_TMS_RTBMPBODY */
    static IDE_RC buildRecord4RtBMPBody(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

    /* D$TABLE_TMS_ITBMPHDR */
    static IDE_RC buildRecord4ItBMPHdr(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

    /* D$TABLE_TMS_ITBMPBODY */
    static IDE_RC buildRecord4ItBMPBody(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

    /* D$TABLE_TMS_LFBMPHDR */
    static IDE_RC buildRecord4LfBMPHdr(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

    /* D$TABLE_TMS_LFBMPRANGESLOT */
    static IDE_RC buildRecord4LfBMPRangeSlot(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );


    /* D$TABLE_TMS_LFBMPPBSTBL*/
    static IDE_RC buildRecord4LfBMPPBSTbl(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

    /* D$TABLE_TMS_SEGCACHE */
    static IDE_RC buildRecord4SegCache(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );



private:

    static void makeDumpBMPStructureInfo(
                                    sdpstBMPHdr                 * aBMPHdr,
                                    SShort                        aSlotNo,
                                    scPageID                      aParentPID,
                                    sdpSegType                    aSegType,
                                    sdpstDumpBMPStructureInfo   * aBMPInfo );

    static void makeDumpBMPHdrInfo( sdpstBMPHdr            * aBMPHdr,
                                    sdpSegType               aSegType,
                                    sdpstDumpBMPHdrInfo    * aBMPHdrInfo );


    static void makeDumpBMPBodyInfo( sdpstBMPHdr            * aBMPHdr,
                                     SShort                   aSlotNo,
                                     sdpSegType               aSegType,
                                     sdpstDumpBMPBodyInfo   * aBMPBodyInfo );

    static void makeDumpLfBMPHdrInfo( sdpstLfBMPHdr          * aLfBMPHdr,
                                      sdpSegType               aSegType,
                                      sdpstDumpLfBMPHdrInfo  * aLfBMPHdrInfo );

    static void makeDumpLfBMPRangeSlotInfo(
                             sdpstLfBMPHdr          * aLfBMPHdr,
                             SShort                   aSlotNo,
                             sdpSegType               aSegType,
                             sdpstDumpLfBMPRangeSlotInfo * aLfBMPBodyInfo);

    static void makeDumpLfBMPPBSTblInfo(
                             sdpstLfBMPHdr            * aLfBMPHdr,
                             sdpSegType                 aSegType,
                             sdpstDumpLfBMPPBSTblInfo * aLfBMPBodyInfo);

    static IDE_RC dumpSegHdr( scSpaceID             aSpaceID,
                              scPageID              aPageID,
                              sdpSegType            aSegType,
                              void                * aHeader,
                              iduFixedTableMemory * aMemory );


    static IDE_RC dumpBMPStructure( scSpaceID             aSpaceID,
                                    scPageID              aPageID,
                                    sdpSegType            aSegType,
                                    void                * aHeader,
                                    iduFixedTableMemory * aMemory );

    static IDE_RC dumpRtBMPHdr( scSpaceID             aSpaceID,
                                scPageID              aPageID,
                                sdpSegType            aSegType,
                                void                * aHeader,
                                iduFixedTableMemory * aMemory );

    static IDE_RC dumpRtBMPBody( scSpaceID             aSpaceID,
                                 scPageID              aPageID,
                                 sdpSegType            aSegType,
                                 void                * aHeader,
                                 iduFixedTableMemory * aMemory );

    static IDE_RC dumpItBMPHdr( scSpaceID             aSpaceID,
                                scPageID              aPageID,
                                sdpSegType            aSegType,
                                void                * aHeader,
                                iduFixedTableMemory * aMemory );

    static IDE_RC dumpItBMPBody( scSpaceID             aSpaceID,
                                 scPageID              aPageID,
                                 sdpSegType            aSegType,
                                 void                * aHeader,
                                 iduFixedTableMemory * aMemory );

    static IDE_RC dumpLfBMPHdr( scSpaceID             aSpaceID,
                                scPageID              aPageID,
                                sdpSegType            aSegType,
                                void                * aHeader,
                                iduFixedTableMemory * aMemory );

    static IDE_RC dumpLfBMPRangeSlot(
                            scSpaceID             aSpaceID,
                            scPageID              aPageID,
                            sdpSegType            aSegType,
                            void                * aHeader,
                            iduFixedTableMemory * aMemory );

    static IDE_RC dumpLfBMPPBSTbl(
                            scSpaceID             aSpaceID,
                            scPageID              aPageID,
                            sdpSegType            aSegType,
                            void                * aHeader,
                            iduFixedTableMemory * aMemory );

    static IDE_RC doDumpRtBMPHdrPage(
                            void                * aHeader,
                            iduFixedTableMemory * aMemory,
                            sdpSegType            aSegType,
                            UChar               * aPagePtr,
                            idBool              * aIsLastLimitResult );

    static IDE_RC doDumpRtBMPBodyPage(
                            void                * aHeader,
                            iduFixedTableMemory * aMemory,
                            sdpSegType            aSegType,
                            UChar               * aPagePtr,
                            idBool              * aIsLastLimitResult );

    static IDE_RC doDumpItBMPHdrPage(
                            void                * aHeader,
                            iduFixedTableMemory * aMemory,
                            sdpSegType            aSegType,
                            UChar               * aPagePtr,
                            idBool              * aIsLastLimitResult );

    static IDE_RC doDumpItBMPBodyPage(
                            void                * aHeader,
                            iduFixedTableMemory * aMemory,
                            sdpSegType            aSegType,
                            UChar               * aPagePtr,
                            idBool              * aIsLastLimitResult );

    static IDE_RC doDumpLfBMPHdrPage(
                            void                * aHeader,
                            iduFixedTableMemory * aMemory,
                            sdpSegType            aSegType,
                            UChar               * aPagePtr,
                            idBool              * aIsLastLimitResult );

    static IDE_RC doDumpLfBMPPage4RangeSlot(
                            void                * aHeader,
                            iduFixedTableMemory * aMemory,
                            sdpSegType            aSegType,
                            UChar               * aPagePtr,
                            idBool              * aIsLastLimitResult );

    static IDE_RC doDumpLfBMPPage4PBSTbl(
                            void                * aHeader,
                            iduFixedTableMemory * aMemory,
                            sdpSegType            aSegType,
                            UChar               * aPagePtr,
                            idBool              * aIsLastLimitResult );

    static IDE_RC doAction4EachPage( void                    * aHeader,
                                     iduFixedTableMemory     * aMemory,
                                     scSpaceID                 aSpaceID,
                                     scPageID                  aSegPID,
                                     sdpSegType                aSegType,
                                     sdpstFTDumpCallbackFunc   aDumpFunc,
                                     sdpPageType             * aPageTypeArray,
                                     UInt                      aPageTypeArrayCnt );

    static IDE_RC doAction4EachPage( void                    * aHeader,
                                     iduFixedTableMemory     * aMemory,
                                     scSpaceID                 aSpaceID,
                                     scPageID                  aSegPID,
                                     sdpSegType                aSegType,
                                     sdpstFTDumpCallbackFunc   aDumpFunc,
                                     sdpPageType               aPageType );


    static inline UChar toCharPageType( sdpstPBS  aPBS );
    static inline UChar toCharPageFN( sdpstPBS  aPBS );
    static inline void toStrBMPType( sdpstBMPType    aBMPType,
                                     UChar         * sBMPTypeStr);
    static inline void toStrMFNL( sdpstMFNL  aMFNL,
                                  UChar    * aMFNLStr );
};

inline UChar sdpstFT::toCharPageType( sdpstPBS  aPBS )
{
    if ( sdpstLfBMP::isDataPage(aPBS) == ID_TRUE )
    {
        return 'D';
    }
    else
    {
        return 'M';
    }
}

inline void sdpstFT::toStrMFNL( sdpstMFNL aMFNL, UChar * aMFNLStr )
{
    IDE_ASSERT( aMFNLStr != NULL );

    idlOS::memset( aMFNLStr, 0x00, SDPST_FT_VARCHAR_LEN );

    switch ( aMFNL )
    {
        case SDPST_MFNL_FUL:
            idlOS::memcpy( aMFNLStr, "FUL", 3 );
            break;
        case SDPST_MFNL_INS:
            idlOS::memcpy( aMFNLStr, "INS", 3 );
            break;
        case SDPST_MFNL_FMT:
            idlOS::memcpy( aMFNLStr, "FMT", 3 );
            break;
        case SDPST_MFNL_UNF:
            idlOS::memcpy( aMFNLStr, "UNF", 3 );
            break;
        default:
            idlOS::memcpy( aMFNLStr, "XXX", 3 ); /* 이런 경우는 없어야 한다. */
            break;
    }
}

inline UChar sdpstFT::toCharPageFN( sdpstPBS  aPBS )
{
    return (UChar)((UInt)sdpstLfBMP::convertPBSToMFNL( aPBS ) + '0');
}

inline void sdpstFT::toStrBMPType( sdpstBMPType     aBMPType,
                                    UChar          * aBMPTypeStr )
{
    IDE_ASSERT( aBMPTypeStr != NULL );

    idlOS::memset( aBMPTypeStr, 0x00, SDPST_FT_VARCHAR_LEN );

    switch ( aBMPType )
    {
        case SDPST_RTBMP:
            idlOS::memcpy( aBMPTypeStr, "RT", 2 );
            break;
        case SDPST_ITBMP:
            idlOS::memcpy( aBMPTypeStr, "IT", 2 );
            break;
        case SDPST_LFBMP:
            idlOS::memcpy( aBMPTypeStr, "LF", 2 );
            break;
        default:
            idlOS::memcpy( aBMPTypeStr, "XX", 2 ); /* 이런 경우는 없어야 한다. */
            break;
    }
}



#endif // _O_SDPST_FT_H_

