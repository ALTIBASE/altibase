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
 

#include <sdp.h>
#include <sdcRow.h>
#include <sdcUndoRecord.h>

/***********************************************************************
 * Description : TableOID 반환
 *
 * [ 설명 ]
 * Undo 레코드 헤더 다음에 저장된 TableOID를 반환한다.
 * Data Row의 첫번째 Head Row Piece에 대한 SDC_UNDO_INSERT_ROW_PIECE와
 * SDC_UNDO_DELETE_ROW_PIECE, SDC_UNDO_UPDATE_LOBDESC 타입에만 저장된다.
 *
 **********************************************************************/
void sdcUndoRecord::getTableOID( UChar    * aUndoRecHdr,
                                 smOID    * aTableOID )
{
    IDE_ASSERT( aUndoRecHdr != NULL );

    SDC_GET_UNDOREC_HDR_FIELD( aUndoRecHdr,
                               SDC_UNDOREC_HDR_TABLEOID,
                               aTableOID );
}


/***********************************************************************
 *
 * Description :
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
UChar* sdcUndoRecord::parseUpdateColCount( UChar          *aUndoRecHdr,
                                           UShort         *aColCount )
{
    sdcUndoRecType   sType;
    UChar          * sCurrLogPtr = aUndoRecHdr;

    IDE_ASSERT( aUndoRecHdr != NULL );
    IDE_ASSERT( aColCount   != NULL );

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &sType );

    if ( sType != SDC_UNDO_UPDATE_ROW_PIECE )
    {
        ideLog::log( IDE_SERVER_0,
                     "Type: %u",
                     sType );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr(aUndoRecHdr),
                        SD_PAGE_SIZE );
                     
        IDE_ASSERT( 0 );
    }

    sCurrLogPtr += SDC_UNDOREC_HDR_SIZE;
    sCurrLogPtr += ID_SIZEOF(scGRID);

    sCurrLogPtr += (1);   // skip flag(1)
    sCurrLogPtr += (2);   // skip size(2)

    ID_READ_AND_MOVE_PTR( sCurrLogPtr,
                          aColCount,
                          ID_SIZEOF(UShort) );

    return sCurrLogPtr;
}


/***********************************************************************
 *
 * Description :
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
UChar* sdcUndoRecord::parseUpdateChangeSize( UChar          *aUndoRecHdr,
                                             SShort         *aChangeSize )
{
    sdcUndoRecType   sType;
    UChar          * sCurrLogPtr = aUndoRecHdr;

    IDE_DASSERT( aUndoRecHdr != NULL );
    IDE_DASSERT( aChangeSize != NULL );

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &sType );

    if ( (sType != SDC_UNDO_UPDATE_ROW_PIECE)          &&
         (sType != SDC_UNDO_OVERWRITE_ROW_PIECE)       &&
         (sType != SDC_UNDO_CHANGE_ROW_PIECE_LINK)     &&
         (sType != SDC_UNDO_DELETE_FIRST_COLUMN_PIECE) &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE)          &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Type: %u",
                     sType );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr(aUndoRecHdr),
                        SD_PAGE_SIZE );

        IDE_ASSERT( 0 );
    }

    sCurrLogPtr += SDC_UNDOREC_HDR_SIZE;
    sCurrLogPtr += ID_SIZEOF(scGRID);

    if ( !(( sType == SDC_UNDO_DELETE_ROW_PIECE ) ||
           ( sType == SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE)) )
    {
        sCurrLogPtr += (1);   // skip flag(1)
    }

    ID_READ_AND_MOVE_PTR( sCurrLogPtr,
                          aChangeSize,
                          ID_SIZEOF(SShort) );

    return sCurrLogPtr;
}


/***********************************************************************
 *
 * Description :
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
UChar* sdcUndoRecord::parseRowGRID( UChar     *aUndoRecHdr,
                                    scGRID    *aRowGRID )
{
    sdcUndoRecType    sType;
    UChar           * sCurrLogPtr;

    IDE_ASSERT( aUndoRecHdr != NULL );
    IDE_ASSERT( aRowGRID    != NULL );

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &sType );

    if ( (sType != SDC_UNDO_INSERT_ROW_PIECE)            &&
         (sType != SDC_UNDO_INSERT_ROW_PIECE_FOR_UPDATE) &&
         (sType != SDC_UNDO_UPDATE_ROW_PIECE)            &&
         (sType != SDC_UNDO_OVERWRITE_ROW_PIECE)         &&
         (sType != SDC_UNDO_CHANGE_ROW_PIECE_LINK)       &&
         (sType != SDC_UNDO_DELETE_FIRST_COLUMN_PIECE)   &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE) &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE)            &&
         (sType != SDC_UNDO_LOCK_ROW) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Type: %u",
                     sType );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr(aUndoRecHdr),
                        SD_PAGE_SIZE );

        IDE_ASSERT( 0 );
    }

    sCurrLogPtr  = aUndoRecHdr;
    sCurrLogPtr += SDC_UNDOREC_HDR_SIZE;

    ID_READ_AND_MOVE_PTR( sCurrLogPtr,
                          aRowGRID,
                          ID_SIZEOF(scGRID) );

    return sCurrLogPtr;
}


/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *
 *
 **********************************************************************/
UChar* sdcUndoRecord::getUndoRecBodyStartPtr( UChar    *aUndoRecHdr )
{
    UChar             * sUndoRecPtr;
    sdcUndoRecType      sType;

    IDE_ASSERT( aUndoRecHdr       != NULL );

    sUndoRecPtr = aUndoRecHdr;

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &sType );

    if ( (sType != SDC_UNDO_INSERT_ROW_PIECE)            &&
         (sType != SDC_UNDO_INSERT_ROW_PIECE_FOR_UPDATE) &&
         (sType != SDC_UNDO_UPDATE_ROW_PIECE)            &&
         (sType != SDC_UNDO_OVERWRITE_ROW_PIECE)         &&
         (sType != SDC_UNDO_CHANGE_ROW_PIECE_LINK)       &&
         (sType != SDC_UNDO_DELETE_FIRST_COLUMN_PIECE)   &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE) &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE)            &&
         (sType != SDC_UNDO_LOCK_ROW)                    &&
         (sType != SDC_UNDO_UPDATE_LOB_LEAF_KEY) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Type: %u",
                     sType );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr(aUndoRecHdr),
                        SD_PAGE_SIZE );

        IDE_ASSERT( 0 );
    }

    sUndoRecPtr += SDC_UNDOREC_HDR_SIZE;
    sUndoRecPtr += ID_SIZEOF(scGRID);

    return sUndoRecPtr;
}


/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *
 *
 **********************************************************************/
UChar* sdcUndoRecord::getRowImageLogStartPtr( UChar    *aUndoRecHdr )
{
    UChar             * sUndoRecPtr;
    sdcUndoRecType      sType;
    UChar               sColDescSetSize;

    IDE_DASSERT( aUndoRecHdr != NULL );

    sUndoRecPtr = aUndoRecHdr;

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &sType );

    if ( (sType != SDC_UNDO_UPDATE_ROW_PIECE)            &&
         (sType != SDC_UNDO_OVERWRITE_ROW_PIECE)         &&
         (sType != SDC_UNDO_CHANGE_ROW_PIECE_LINK)       &&
         (sType != SDC_UNDO_DELETE_FIRST_COLUMN_PIECE)   &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE) &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE)            &&
         (sType != SDC_UNDO_LOCK_ROW) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Type: %u",
                     sType );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr(aUndoRecHdr),
                        SD_PAGE_SIZE );

        IDE_ASSERT( 0 );
    }

    sUndoRecPtr = getUndoRecBodyStartPtr(sUndoRecPtr);

    switch (sType)
    {
        case SDC_UNDO_DELETE_ROW_PIECE:
        case SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE:
            break;

        case SDC_UNDO_OVERWRITE_ROW_PIECE:
        case SDC_UNDO_CHANGE_ROW_PIECE_LINK:
        case SDC_UNDO_DELETE_FIRST_COLUMN_PIECE:
            sUndoRecPtr += (1);     // skip flag(1)
            sUndoRecPtr += (2);     // skip size(2)
            break;

        case SDC_UNDO_LOCK_ROW:
            sUndoRecPtr += (1);     // skip flag(1)
            break;

        case SDC_UNDO_UPDATE_ROW_PIECE:
            sUndoRecPtr += (1);     // skip flag(1)
            sUndoRecPtr += (2);     // skip size(2)
            sUndoRecPtr += (2);     // skip column count(2)

            // read column desc set size(1)
            ID_READ_AND_MOVE_PTR( sUndoRecPtr,
                                  &sColDescSetSize,
                                  ID_SIZEOF(sColDescSetSize) );

            // skip column desc set(1~128)
            sUndoRecPtr += sColDescSetSize;
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return sUndoRecPtr;
}


/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *
 *
 **********************************************************************/
void sdcUndoRecord::parseRowHdr( UChar            *aUndoRecHdr,
                                 sdcRowHdrInfo    *aRowHdrInfo )
{
    sdcUndoRecType    sType;
    UChar           * sRowImageLogStartPtr;

    IDE_DASSERT( aUndoRecHdr != NULL );
    IDE_DASSERT( aRowHdrInfo != NULL );

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &sType );

    if ( (sType != SDC_UNDO_UPDATE_ROW_PIECE)            &&
         (sType != SDC_UNDO_OVERWRITE_ROW_PIECE)         &&
         (sType != SDC_UNDO_CHANGE_ROW_PIECE_LINK)       &&
         (sType != SDC_UNDO_DELETE_FIRST_COLUMN_PIECE)   &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE) &&
         (sType != SDC_UNDO_DELETE_ROW_PIECE)            &&
         (sType != SDC_UNDO_LOCK_ROW) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Type: %u",
                     sType );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr(aUndoRecHdr),
                        SD_PAGE_SIZE );

        IDE_ASSERT( 0 );
    }

    sRowImageLogStartPtr = getRowImageLogStartPtr(aUndoRecHdr);

    sdcRow::getRowHdrInfo( sRowImageLogStartPtr, aRowHdrInfo );
}

/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *
 *
 **********************************************************************/
idBool sdcUndoRecord::isExplicitLockRec( const UChar *aUndoRecHdr )
{
    const UChar   * sUndoRecBodyPtr;
    UChar           sLogFlag;
    sdcUndoRecType  sType;

    IDE_DASSERT( aUndoRecHdr != NULL );

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &sType );

    if( sType != SDC_UNDO_LOCK_ROW )
    {
        return ID_FALSE;
    }
    else
    {
        sUndoRecBodyPtr = (const UChar*)
            getUndoRecBodyStartPtr((UChar*)aUndoRecHdr);

        ID_READ_VALUE( sUndoRecBodyPtr, &sLogFlag, ID_SIZEOF(sLogFlag) );

        if( (sLogFlag & SDC_UPDATE_LOG_FLAG_LOCK_TYPE_MASK) ==
            SDC_UPDATE_LOG_FLAG_LOCK_TYPE_EXPLICIT )
        {
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }
}


/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *
 *
 **********************************************************************/
void sdcUndoRecord::setInvalid( UChar    *aUndoRecHdr )
{
    sdcUndoRecFlag  sUndoRecFlag;

    IDE_DASSERT( aUndoRecHdr != NULL );

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_FLAG,
                                  &sUndoRecFlag );

    SDC_SET_UNDOREC_FLAG_OFF( sUndoRecFlag,
                              SDC_UNDOREC_FLAG_IS_VALID_TRUE );

    SDC_SET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_FLAG,
                                  sUndoRecFlag );
}

