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
 * $Id: smcFT.h 19550 2007-02-07 03:09:40Z kimmkeun $
 **********************************************************************/

#ifndef _O_SMC_FT_H_
#define _O_SMC_FT_H_  (1)

# include <idu.h>
# include <smDef.h>
# include <sdcDef.h>

//-------------------------------
// D$MEM_TABLE_RECORD
//-------------------------------
typedef struct smcDumpMemTableRow
{
    scPageID  mPageID;                        // PAGE_ID
    UShort    mOffset;                        // OFFSET
    smTID     mCreateTID;                     // CREATE_TID
    SChar    *mCreateSCN;                     // CREATE_SCN
    smTID     mLimitTID;                      // LIMIT_TID
    SChar    *mLimitSCN;                      // LIMIT_SCN
    SChar    *mNext;                          // NEXT_OID_OR_SCN
    UShort    mFlag;                          // FLAG
    UChar     mUsedFlag;                      // USED FLAG
    UChar     mDropFlag;                      // DROP FLAG
    UChar     mSkipRefineFlag;                // SKIP REFINE FLAG
    SShort    mNthSlot;                       // NTH_SLOT
    SShort    mNthColumn;                     // NTH_COLUMN
    SChar     mValue[SM_DUMP_VALUE_LENGTH];   // VALUE24B
} smcDumpMemTableRow;

class smcFT
{
private:
    static IDE_RC buildEachRecordForTableInfo(
                                   void                *aHeader,
                                   smcTableHeader      *aTableHeader,
                                   iduFixedTableMemory *aMemory);

public:

    /* PROJ-1407 Temporary Table - Volatile table에 사용하기 위해서
     * public으로 위치이동 */
    static IDE_RC makeMemColValue24B( const smiColumn * aColumn,
                                      SChar           * aRowPtr,
                                      SChar           * aValue24B );

    //------------------------------------------
    // For D$MEM_TABLE_RECORD
    //------------------------------------------
    static IDE_RC buildRecordMemTableRecord( idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * aDumpObj,
                                             iduFixedTableMemory * aMemory );

    //------------------------------------------
    // For X$TABLE_INFO
    //------------------------------------------
    static IDE_RC buildRecordForTableInfo(idvSQL              * /*aStatistics*/,
                                          void      *aHeader,
                                          void      *aDumpObj,
                                          iduFixedTableMemory *aMemory);

    //------------------------------------------
    // For X$TEMP_TABLE_INFO (PROJ-1407 Temporary Table)
    //------------------------------------------
    static IDE_RC buildRecordForTempTableInfo(idvSQL              * /*aStatistics*/,
                                              void      *aHeader,
                                              void      *aDumpObj,
                                              iduFixedTableMemory *aMemory);

    //------------------------------------------
    // For X$CATALOG
    //------------------------------------------
    static IDE_RC buildRecordForCatalog(idvSQL              * /*aStatistics*/,
                                        void        *aHeader,
                                        void        *aDumpObj,
                                        iduFixedTableMemory *aMemory);

    //------------------------------------------
    // For X$TEMP_CATALOG (PROJ-1407 Temporary Table)
    //------------------------------------------
    static IDE_RC buildRecordForTempCatalog(idvSQL              * /*aStatistics*/,
                                            void        *aHeader,
                                            void        *aDumpObj,
                                            iduFixedTableMemory *aMemory);

    //------------------------------------------
    // For X$SEQ
    //------------------------------------------
    static  IDE_RC  buildRecordForSEQInfo(idvSQL      * /*aStatistics*/,
                                          void        *aHeader,
                                          void        *aDumpObj,
                                          iduFixedTableMemory *aMemory);

    //------------------------------------------
    // For X$TMS_TABLE_CACHE
    //------------------------------------------
    static IDE_RC buildRecordForTmsTableCache(idvSQL              * /*aStatistics*/,
                                              void        *aHeader,
                                              void        * /* aDumpObj */,
                                              iduFixedTableMemory *aMemory);

 
};

#endif /* _O_SMC_FT_H_ */
