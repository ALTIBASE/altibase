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
 * $Id: sdpscSH.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment의 Segment Header의 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPSC_SH_H_
# define _O_SDPSC_SH_H_ 1

# include <sdpDef.h>
# include <sdpPhyPage.h>

# include <sdpscDef.h>
# include <sdpscED.h>

class sdpscSegHdr
{

public:

    /* Segment Header 페이지를 초기화한다. */
    static void initSegHdr ( sdpscSegMetaHdr * aSegHdreader,
                             scPageID          aSegHdrPID,
                             sdpSegType        aSegType,
                             UInt              aPageCntInExt,
                             UShort            aMaxExtCntInExtDir );

    /* [ INTERFACE ] segment 상태를 반환한다. */
    static IDE_RC getSegState( idvSQL        *aStatistics,
                               scSpaceID      aSpaceID,
                               scPageID       aSegPID,
                               sdpSegState   *aSegState );

    /* Segment Header 페이지 생성 */
    static IDE_RC createAndInitPage( idvSQL                * aStatistics,
                                     sdrMtxStartInfo       * aStartInfo,
                                     scSpaceID               aSpaceID,
                                     sdpscExtDesc          * aFstExtDesc,
                                     sdpSegType              aSegType,
                                     UShort                  aMaxExtCntInExtDir );

    static IDE_RC addNewExtDir( idvSQL             * aStatistics,
                                sdrMtx             * aMtx,
                                scSpaceID            aSpaceID,
                                scPageID             aSegHdrPID,
                                scPageID             aCurExtDirPID,
                                scPageID             aNewExtDirPID,
                                UInt               * aTotExtCntOfSeg );

    static IDE_RC removeExtDir( idvSQL             * aStatistics,
                                sdrMtx             * aMtx,
                                scSpaceID            aSpaceID,
                                scPageID             aSegHdrPID,
                                scPageID             aPrvPIDOfExtDir,
                                scPageID             aRemExtDirPID,
                                scPageID             aNxtPIDOfNewExtDir,
                                UShort               aTotExtCntOfRemExtDir,
                                UInt               * aTotExtCntOfSeg );


    /* Segment Header 페이지에 새로운 ExtDir 연결 */
    static IDE_RC addNewExtDir( sdrMtx             * aMtx,
                                sdpscSegMetaHdr    * aSegHdr,
                                sdpscExtDirCntlHdr * aCurExtDirCntlHdr,
                                sdpscExtDirCntlHdr * aNewExtDirCntlHdr );

    /* Segment Header 페이지에 ExtDir 제거 */
    static IDE_RC removeExtDir( sdrMtx             * aMtx,
                                sdpscSegMetaHdr    * aSegHdr,
                                sdpscExtDirCntlHdr * aPrvExtDirCntlHdr,
                                scPageID             aRemExtDir,
                                scPageID             aNewNxtExtDir );

    /* [ INTERFACE ] Sequential Scan Before First에서 Segment 정보 반환 */
    static IDE_RC getSegInfo( idvSQL        * aStatistics,
                              scSpaceID       aSpaceID,
                              scPageID        aSegPID,
                              void          * aTableHeader,
                              sdpSegInfo    * aSegInfo );

    static IDE_RC getFmtPageCnt( idvSQL        *aStatistics,
                                 scSpaceID      aSpaceID,
                                 sdpSegHandle  *aSegHandle,
                                 ULong         *aFmtPageCnt );

    /* Segment Header 페이지에서 마지막 Lst ExtDir PID를 반환한다. */
    static inline scPageID getLstExtDir( sdpscSegMetaHdr * aSegHdr );

    /* Segment Header 페이지에서 첫번째 ExtDir PID를 반환한다. */
    static inline scPageID getFstExtDir( sdpscSegMetaHdr * aSegHdr );

    /* ExtDir의 개수를 반환한다. */
    static inline UInt     getExtDirCnt( sdpscSegMetaHdr * aSegHdr );

    /* ExtDir Control Header 반환 */
    static inline sdpscExtDirCntlHdr * getExtDirCntlHdr( sdpscSegMetaHdr * aSegHdr );

    /* Extent Control Header 반환 */
    static inline sdpscExtCntlHdr * getExtCntlHdr( sdpscSegMetaHdr * aSegHdr );

    /* Segment Control Header 반환 */
    static inline sdpscSegCntlHdr * getSegCntlHdr( sdpscSegMetaHdr * aSegHdr );

    /* 페이지의 임의의 포인터에서 Logical Header Ptr를 반환 */
    static inline sdpscSegMetaHdr* getHdrPtr( UChar * aPagePtr );

    static inline void  calcExtRID2ExtInfo( scPageID             aSegPID,
                                            sdpscSegMetaHdr    * aSegHdr,
                                            sdRID                aExtRID,
                                            scPageID           * aExtDir,
                                            SShort             * aDescIdx );

    static inline void  calcExtInfo2ExtRID( scPageID             aSegPID,
                                            sdpscSegMetaHdr    * aSegHdr,
                                            scPageID             aExtDir,
                                            SShort               aDescIdx,
                                            sdRID              * aExtRID );
    // 최대 저장할 수 있는 Ext Desc의 개수
    static inline UShort getMaxExtDescCnt( sdpSegType aSegType );

    /* ExtDir에 기록할 수 있는 ExtDesc 개수 반환 */
    static inline UShort getFreeDescCntOfExtDir(
                            sdpscExtDirCntlHdr * aCntlHdr,
                            sdpSegType           aSegType );

    static IDE_RC fixAndGetHdr4Write( idvSQL           * aStatistics,
                                      sdrMtx           * aMtx,
                                      scSpaceID          aSpaceID,
                                      scPageID           aSegHdrPID,
                                      sdpscSegMetaHdr ** aSegHdr );

    static IDE_RC fixAndGetHdr4Read( idvSQL            * aStatistics,
                                     sdrMtx            * aMtx,
                                     scSpaceID           aSpaceID,
                                     scPageID            aSegHdrPID,
                                     sdpscSegMetaHdr  ** aSegHdr );

    static IDE_RC setTotExtCnt( sdrMtx          * aMtx,
                                sdpscExtCntlHdr * aExtCntlHdr,
                                UInt              aTotExtDescCnt );

private :

    static IDE_RC setTotExtDirCnt( sdrMtx          * aMtx,
                                   sdpscExtCntlHdr * aExtCntlHdr,
                                   UInt              aTotExtDirCnt );

    static IDE_RC setLstExtDir( sdrMtx          * aMtx,
                                sdpscExtCntlHdr * aExtCntlHdr,
                                scPageID          aLstExtDirPID );
};

/*
 * 페이지의 임의의 포인터에서 Logical Header Ptr를 반환한다.
 */
inline sdpscSegMetaHdr* sdpscSegHdr::getHdrPtr( UChar * aPagePtr )
{
    IDE_ASSERT( aPagePtr != NULL );
    return (sdpscSegMetaHdr*)sdpPhyPage::getLogicalHdrStartPtr( aPagePtr );
}

/* Segment Header에서 Segment Control Header의 Ptr를 반환한다. */
inline sdpscSegCntlHdr * sdpscSegHdr::getSegCntlHdr( sdpscSegMetaHdr * aSegHdr )
{
    return (sdpscSegCntlHdr*)&(aSegHdr->mSegCntlHdr);
}

/* Segment Header에서 Segment Control Header의 Ptr를 반환한다. */
inline sdpscExtCntlHdr * sdpscSegHdr::getExtCntlHdr( sdpscSegMetaHdr * aSegHdr )
{
    return (sdpscExtCntlHdr*)&(aSegHdr->mExtCntlHdr);
}

/* ExtDir Control Header 반환 */
inline sdpscExtDirCntlHdr * sdpscSegHdr::getExtDirCntlHdr( sdpscSegMetaHdr * aSegHdr )
{
    return (sdpscExtDirCntlHdr*)&(aSegHdr->mExtDirCntlHdr);
}

/* Segment Header의 ExtDir 영역의 최대 ExtDesc 개수 반환 */
inline UShort sdpscSegHdr::getMaxExtDescCnt( sdpSegType  aSegType )
{
    UShort sPropDescCnt;
    UShort sMaxDescCnt = (UShort)
        (((sdpPhyPage::getEmptyPageFreeSize()
         - ID_SIZEOF( sdpscSegMetaHdr ) ))
         / ID_SIZEOF( sdpscExtDesc ));

    switch( aSegType )
    {
        case SDP_SEG_TYPE_TSS:
            sPropDescCnt = smuProperty::getTSSegExtDescCntPerExtDir();
            IDE_ASSERT( sMaxDescCnt >= sPropDescCnt );
            break;

        case SDP_SEG_TYPE_UNDO:
            sPropDescCnt = smuProperty::getUDSegExtDescCntPerExtDir();
            IDE_ASSERT( sMaxDescCnt >= sPropDescCnt );
            break;

        default:
            IDE_ASSERT( 0 );
            break;
    }

    return sPropDescCnt;
}

/* ExtDir에 기록할 수 있는 ExtDesc 개수 반환 */
inline UShort sdpscSegHdr::getFreeDescCntOfExtDir( sdpscExtDirCntlHdr * aCntlHdr,
                                                   sdpSegType           aSegType )
{
    return (getMaxExtDescCnt( aSegType ) - aCntlHdr->mExtCnt);
}

/* Segment Header 페이지에서 마지막 Lst ExtDir PID를 반환한다. */
inline scPageID sdpscSegHdr::getLstExtDir( sdpscSegMetaHdr * aSegHdr )
{
    return getExtCntlHdr( aSegHdr )->mLstExtDir;
}

/* Segment Header 페이지에서 첫번째 ExtDir PID를 반환한다. */
inline scPageID sdpscSegHdr::getFstExtDir( sdpscSegMetaHdr * aSegHdr )
{
    return getExtDirCntlHdr( aSegHdr )->mNxtExtDir;
}

/* ExtDir의 개수를 반환한다. */
inline UInt  sdpscSegHdr::getExtDirCnt( sdpscSegMetaHdr  * aSegHdr )
{
    return getExtCntlHdr( aSegHdr )->mTotExtDirCnt;
}

/***********************************************************************
 * Description : ExtDir PID와 ExtDesc 순번으로 ExtDesc의 RID 변환
 ***********************************************************************/
inline void  sdpscSegHdr::calcExtInfo2ExtRID( scPageID             aSegPID,
                                              sdpscSegMetaHdr    * aSegHdr,
                                              scPageID             aExtDir,
                                              SShort               aDescIdx,
                                              sdRID              * aExtRID )
{
    IDE_ASSERT( aSegHdr != NULL );
    IDE_ASSERT( aExtRID != NULL );

    if ( aSegPID != aExtDir )
    {
        *aExtRID = SD_MAKE_RID( aExtDir,
                                sdpscExtDir::calcDescIdx2Offset(
                                    NULL,
                                    aDescIdx ) );
    }
    else
    {
        *aExtRID = SD_MAKE_RID( aExtDir,
                                sdpscExtDir::calcDescIdx2Offset(
                                    getExtDirCntlHdr(aSegHdr),
                                    aDescIdx ) );
    }
    return;
}

/***********************************************************************
 * Description : ExtDesc의 RID로 ExtDir PID와 ExtDesc 순번 변환
 ***********************************************************************/
inline void  sdpscSegHdr::calcExtRID2ExtInfo( scPageID             aSegPID,
                                              sdpscSegMetaHdr    * aSegHdr,
                                              sdRID                aExtRID,
                                              scPageID           * aExtDir,
                                              SShort             * aDescIdx )
{
    IDE_ASSERT( aSegHdr  != NULL );
    IDE_ASSERT( aExtRID  != SD_NULL_RID );
    IDE_ASSERT( aExtDir  != NULL );
    IDE_ASSERT( aDescIdx != NULL );

    if ( aSegPID != SD_MAKE_PID( aExtRID ) )
    {
        *aDescIdx = sdpscExtDir::calcOffset2DescIdx(
            NULL,
            SD_MAKE_OFFSET(aExtRID) );
    }
    else
    {
        *aDescIdx = sdpscExtDir::calcOffset2DescIdx(
            getExtDirCntlHdr(aSegHdr),
            SD_MAKE_OFFSET(aExtRID) );
    }
    *aExtDir = SD_MAKE_PID(aExtRID);
    return;
}

#endif // _O_SDPSC_SH_H_
