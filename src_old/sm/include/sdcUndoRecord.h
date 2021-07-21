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
 


#ifndef _O_SDC_UNDO_RECORD_H_
#define _O_SDC_UNDO_RECORD_H_ 1

#include <smDef.h>
#include <sdcDef.h>

class sdcUndoRecord
{
public:

    static UChar* parseUpdateColCount( UChar         *aUndoRecPtr,
                                       UShort        *aColCount );

    static UChar* parseUpdateChangeSize( UChar         *aUndoRecPtr,
                                         SShort        *aChangeSize );

    static UChar* parseRowGRID( UChar     *aUndoRecHdr,
                                scGRID    *aRowGRID );

    static inline void parseUndoRecHdr( UChar             * aUndoRecHdr,
                                        sdcUndoRecHdrInfo * aUndoRecHdrInfo );

    static UChar* getUndoRecBodyStartPtr( UChar    *aUndoRecHdr );

    static UChar* getRowImageLogStartPtr( UChar    *aUndoRecHdr );

    static void parseRowHdr( UChar            *aUndoRecHdr,
                             sdcRowHdrInfo    *aRowHdrInfo );

    static void getTableOID( UChar    * aUndoRecHdr,
                             smOID    * aTableOID );

    static idBool isExplicitLockRec( const UChar *aUndoRecHdr );

    static void setInvalid( UChar    *aUndoRecHdr );

    static inline UChar * writeUndoRecHdr(
                             UChar          * aUndoRecHdr,
                             sdcUndoRecType   aType,
                             sdcUndoRecFlag   aFlag,
                             smOID            aTableOID );
};

inline UChar * sdcUndoRecord::writeUndoRecHdr(
                      UChar          * aUndoRecHdr,
                      sdcUndoRecType   aType,
                      sdcUndoRecFlag   aFlag,
                      smOID            aTableOID )
{
    IDE_ASSERT( aUndoRecHdr != NULL );

    SDC_SET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  aType );

    SDC_SET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_FLAG,
                                  aFlag );

    SDC_SET_UNDOREC_HDR_FIELD( aUndoRecHdr,
                               SDC_UNDOREC_HDR_TABLEOID,
                               &aTableOID );

    return ( aUndoRecHdr + SDC_UNDOREC_HDR_SIZE );
}

inline void sdcUndoRecord::parseUndoRecHdr(
                      UChar             * aUndoRecHdr,
                      sdcUndoRecHdrInfo * aUndoRecHdrInfo )
{
    IDE_ASSERT( aUndoRecHdr != NULL );

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_TYPE,
                                  &aUndoRecHdrInfo->mType );

    SDC_GET_UNDOREC_HDR_1B_FIELD( aUndoRecHdr,
                                  SDC_UNDOREC_HDR_FLAG,
                                  &aUndoRecHdrInfo->mFlag );

    SDC_GET_UNDOREC_HDR_FIELD( aUndoRecHdr,
                               SDC_UNDOREC_HDR_TABLEOID,
                               &aUndoRecHdrInfo->mTableOID );
}

#endif // _O_SDC_UNDO_RECORD_H_
