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

#ifndef _O_SMI_SORT_TEMP_TABLE_H_
#define _O_SMI_SORT_TEMP_TABLE_H_ 1

#include <idu.h>
#include <smDef.h>
#include <smiDef.h>
#include <smiMisc.h>
#include <sdtDef.h>
#include <sdtSortDef.h>

class smiSortTempTable
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
                          UInt                  aWorkGroupRatio,
                          const void         ** aTable );
    static IDE_RC getColumnCount( smiTempTableHeader  * aHeader,
                                  const smiColumnList * aColumnList );
    static IDE_RC buildColumnInfo( smiTempTableHeader  * aTableHeader,
                                   const smiColumnList * aColumnList,
                                   const smiColumnList * aKeyColumns );
    static IDE_RC buildKeyColumns( smiTempTableHeader  * aTableHeader,
                                   const smiColumnList * aKeyColumns );
    static IDE_RC resetKeyColumn( void                * aTable,
                                  const smiColumnList * aKeyColumns );
    static IDE_RC drop( void   * aTable );
    static IDE_RC clear( void   * aTable );
    static void clearHitFlag( void   * aTable );

    static IDE_RC insert( void     * aTable,
                          smiValue * aValue );

    static IDE_RC update( smiSortTempCursor* aCursor,
                          smiValue      * aValue );
    static void   setHitFlag( smiSortTempCursor* aCursor );
    static idBool isHitFlagged( smiSortTempCursor* aCursor );
    static IDE_RC sort( void * aTable );
    static IDE_RC scan( void * aTable );
    static IDE_RC fetchFromGRID( void          * aTable,
                                 scGRID          aGRID,
                                 void          * aDestRowBuf );

    static IDE_RC openCursor( void                * aTable,
                              UInt                  aFlag,
                              const smiColumnList * aColumns,
                              const smiRange      * aKeyRange,
                              const smiRange      * aKeyFilter,
                              const smiCallBack   * aRowFilter,
                              smiSortTempCursor     ** aCursor );
    static IDE_RC restartCursor( smiSortTempCursor      * aCursor,
                                 UInt                  aFlag,
                                 const smiRange      * aKeyRange,
                                 const smiRange      * aKeyFilter,
                                 const smiCallBack   * aRowFilter );
    static IDE_RC fetch( smiSortTempCursor * aCursor,
                         UChar         ** aRow,
                         scGRID         * aRowGRID );

    static IDE_RC storeCursor( smiSortTempCursor   * aCursor,
                               smiTempPosition ** aPosition );
    static IDE_RC restoreCursor( smiSortTempCursor   * aCursor,
                                 smiTempPosition  * aPosition,
                                 UChar           ** aRow,
                                 scGRID           * aRowGRID );
    static IDE_RC closeAllCursor(void * aTable );

    static void getDisplayInfo( void  * aTable,
                                ULong * aPageCount,
                                SLong * aRecordCount );

private:
    static IDE_RC resetCursor( smiSortTempCursor* aCursor );
    static IDE_RC closeCursor( smiSortTempCursor* aCursor );

    static void generateSortStats( smiTempTableHeader * aHeader,
                                   smiTempTableOpr      aOpr );

public:
    static void   dumpTempCursor( smiSortTempCursor * aTempCursor,
                                  SChar             * aOutBuf,
                                  UInt                aOutSize );

    static iduMemPool          mTempCursorPool;
    static iduMemPool          mTempPositionPool;
};

#endif /* _O_SMI_SORT_TEMP_TABLE_H_ */
