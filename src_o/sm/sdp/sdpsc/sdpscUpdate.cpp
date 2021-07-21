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
 * $Id: sdpscUpdate.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

# include <sdb.h>

# include <sdpscSH.h>
# include <sdpscED.h>
# include <sdpscAllocPage.h>

# include <sdpscUpdate.h>

/***********************************************************************
 *
 * Description : Segment Header 초기화를 REDO한다.
 *
 *
 * aStatistics    - [IN] 통계정보
 * aData          - [IN] Log Pointer
 * aLength        - [IN] Log 길이
 * aPagePtr       - [IN] 페이지 Pointer
 * aRedoInfo      - [IN] Redo 정보
 * aMtx           - [IN] Mini Transaction Pointer
 *
 ***********************************************************************/
IDE_RC sdpscUpdate::redo_SDPSC_INIT_SEGHDR( SChar       * aData,
                                            UInt          aLength,
                                            UChar       * aPagePtr,
                                            sdrRedoInfo * /*aRedoInfo*/,
                                            sdrMtx*         aMtx )
{
    sdpSegType  sSegType;
    UInt        sPageCntInExt;
    scPageID    sSegHdrPID;
    SChar     * sDataPtr;
    UShort      sMaxExtCntInExtDir;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR( aLength  == (ID_SIZEOF( sSegType ) +
                            ID_SIZEOF( sSegHdrPID ) +
                            ID_SIZEOF( sPageCntInExt ) +
                            ID_SIZEOF( sMaxExtCntInExtDir ) ) );

    sDataPtr = aData;

    idlOS::memcpy( &sSegType, sDataPtr, ID_SIZEOF(sSegType) );
    sDataPtr += ID_SIZEOF(sSegType);

    idlOS::memcpy( &sSegHdrPID, sDataPtr, ID_SIZEOF(sSegHdrPID) );
    sDataPtr += ID_SIZEOF(sSegHdrPID);

    idlOS::memcpy( &sPageCntInExt, sDataPtr, ID_SIZEOF(sPageCntInExt) );
    sDataPtr += ID_SIZEOF(sPageCntInExt);

    idlOS::memcpy( &sMaxExtCntInExtDir,
                   sDataPtr,
                   ID_SIZEOF(sMaxExtCntInExtDir) );

    sdpscSegHdr::initSegHdr( (sdpscSegMetaHdr*)aPagePtr, 
                             sSegHdrPID,
                             sSegType,
                             sPageCntInExt,
                             sMaxExtCntInExtDir );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDir 페이지 초기화를 REDO한다.
 *
 * aStatistics    - [IN] 통계정보
 * aData          - [IN] Log Pointer
 * aLength        - [IN] Log 길이
 * aPagePtr       - [IN] 페이지 Pointer
 * aRedoInfo      - [IN] Redo 정보
 * aMtx           - [IN] Mini Transaction Pointer
 *
 ***********************************************************************/
IDE_RC sdpscUpdate::redo_SDPSC_INIT_EXTDIR( SChar       * aData,
                                            UInt          aLength,
                                            UChar       * aPagePtr,
                                            sdrRedoInfo * /*aRedoInfo*/,
                                            sdrMtx*         aMtx )
{
    SChar    * sDataPtr;
    scOffset   sMapOffset;
    UShort     sMaxExtDescCnt;
    scPageID   sNxtExtDir;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR( aLength  == (ID_SIZEOF( scPageID ) + 
                             ID_SIZEOF( scOffset ) +
                             ID_SIZEOF( UShort )) );

    sDataPtr = aData;

    idlOS::memcpy( &sNxtExtDir, sDataPtr, ID_SIZEOF( scPageID ) );
    sDataPtr += ID_SIZEOF( scPageID );

    idlOS::memcpy( &sMapOffset, sDataPtr, ID_SIZEOF( scOffset ) );
    sDataPtr += ID_SIZEOF( scOffset );

    idlOS::memcpy( &sMaxExtDescCnt, sDataPtr, ID_SIZEOF( UShort ) );

    sdpscExtDir::initCntlHdr( (sdpscExtDirCntlHdr*)aPagePtr,
                              sNxtExtDir,
                              sMapOffset,
                              sMaxExtDescCnt );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDesc을 ExtDir 페이지에 추가한다.
 *
 * aStatistics    - [IN] 통계정보
 * aData          - [IN] Log Pointer
 * aLength        - [IN] Log 길이
 * aPagePtr       - [IN] 페이지 Pointer
 * aRedoInfo      - [IN] Redo 정보
 * aMtx           - [IN] Mini Transaction Pointer
 *
 ***********************************************************************/
IDE_RC sdpscUpdate::redo_SDPSC_ADD_EXTDESC_TO_EXTDIR( SChar       * aData,
                                                      UInt          aLength,
                                                      UChar       * aPagePtr,
                                                      sdrRedoInfo * /*aRedoInfo*/,
                                                      sdrMtx*         aMtx )
{
    sdpscExtDesc         sExtDesc;
    sdpscExtDirCntlHdr * sCntlHdr;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR( aLength  == ID_SIZEOF( sdpscExtDesc ));

    idlOS::memcpy(&sExtDesc, aData, aLength);

    sCntlHdr = (sdpscExtDirCntlHdr*)aPagePtr;

    sdpscExtDir::addExtDesc( sCntlHdr, sCntlHdr->mExtCnt, &sExtDesc );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
