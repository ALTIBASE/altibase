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

#ifndef _O_SMI_HASH_TEMP_TABLE_H_
#define _O_SMI_HASH_TEMP_TABLE_H_ 1

#include <idu.h>
#include <smDef.h>
#include <smiDef.h>
#include <smiMisc.h>
#include <sdtDef.h>
#include <sdtHashDef.h>
#include <sdtHashModule.h>

class smiHashTempTable
{
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC create( idvSQL              * aStatistics,
                          scSpaceID             aSpaceID,
                          ULong                 aInitWorkAreaSize,
                          smiStatement        * aStatement,
                          UInt                  aFlag,
                          const smiColumnList * aColumnList,
                          const smiColumnList * aKeyColumns,
                          const void         ** aTable );

    static IDE_RC getColumnCount( smiTempTableHeader  * aHeader,
                                  const smiColumnList * aColumnList );
    static IDE_RC buildColumnInfo( smiTempTableHeader  * aTableHeader,
                                   const smiColumnList * aColumnList,
                                   const smiColumnList * aKeyColumns );
    static IDE_RC buildKeyColumns( smiTempTableHeader  * aTableHeader,
                                   const smiColumnList * aKeyColumns );

    static IDE_RC drop( void   * aTable );
    static IDE_RC clear( void   * aTable );

    static IDE_RC insert( void     * aTable,
                          smiValue * aValue,
                          UInt       aHashValue,
                          scGRID   * aGRID,
                          idBool   * aResult );

    static IDE_RC update( smiHashTempCursor * aCursor,
                          smiValue          * aValue );
    static void setHitFlag( smiHashTempCursor * aCursor );
    static idBool isHitFlagged( smiHashTempCursor * aCursor );
    static void   clearHitFlag( void   * aTable);

    static IDE_RC fetchFromGRID( void    * aTable,
                                 scGRID    aGRID,
                                 void    * aDestRowBuf );
    static IDE_RC allocHashCursor(void                * aTable,
                                  const smiColumnList * aColumns,
                                  UInt                  aCursorType,
                                  smiHashTempCursor  ** aCursor );
    static void resetCursor( smiHashTempCursor  * aCursor );
    static IDE_RC openHashCursor( smiHashTempCursor   * aCursor,
                                  UInt                  aFlag,
                                  const smiCallBack   * aRowFilter,
                                  UChar              ** aRow,
                                  scGRID              * aRowGRID );
    static IDE_RC openUpdateHashCursor( smiHashTempCursor   * aCursor,
                                        UInt                  aFlag,
                                        const smiCallBack   * aRowFilter,
                                        UChar              ** aRow,
                                        scGRID              * aRowGRID );

    static IDE_RC openFullScanCursor( smiHashTempCursor   * aCursor,
                                      UInt                  aFlag,
                                      UChar              ** aRow,
                                      scGRID              * aRowGRID );

    static IDE_RC fetchFullNext( smiHashTempCursor  * aCursor,
                                 UChar             ** aRow,
                                 scGRID             * aRowGRID );
    static inline IDE_RC fetchHashNext( smiHashTempCursor  * aCursor,
                                        UChar         ** aRow,
                                        scGRID         * aRowGRID );
    static IDE_RC fetchHashNextInternal( smiHashTempCursor  * aCursor,
                                         UChar         ** aRow,
                                         scGRID         * aRowGRID );

    static IDE_RC closeAllCursor( smiTempTableHeader * aHeader );
    static IDE_RC getNullRow( void   * aTable,
                              UChar ** aNullRowPtr);
    static void   getDisplayInfo(void  * aTable,
                                 ULong * aPageCount,
                                 SLong * aRecordCount );

    static ULong getMaxHashBucketCount( void * aTable );
    static ULong getRowAreaSize( void * aTable );

    static void   generateHashStats( smiTempTableHeader * aHeader,
                                     smiTempTableOpr      aOpr );
    static void   dumpTempCursor( smiHashTempCursor * aTempCursor,
                                  SChar  * aOutBuf,
                                  UInt     aOutSize );

    static idBool checkHashSlot( smiHashTempCursor* aHashCursor,
                                 UInt               aHashValue );

    static idBool checkUpdateHashSlot( smiHashTempCursor* aHashCursor,
                                       UInt               aHashValue );

    static iduMemPool mTempCursorPool;
};

IDE_RC smiHashTempTable::fetchHashNext( smiHashTempCursor  * aCursor,
                                        UChar             ** aRow,
                                        scGRID             * aRowGRID )
{
    if ( aCursor->mChildPageID == SM_NULL_PID )
    {
        *aRow = NULL;
        SC_MAKE_NULL_GRID(*aRowGRID);

        return IDE_SUCCESS;
    }
    else
    {
        return fetchHashNextInternal( aCursor,
                                      aRow,
                                      aRowGRID );
    }
}

#endif /* _O_SMI_HASH_TEMP_TABLE_H_ */
