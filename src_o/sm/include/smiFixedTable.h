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
 * $Id: smiFixedTable.h 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/
#ifndef _O_SMI_FIXED_TABLE_H_
# define _O_SMI_FIXED_TABLE_H_ 1

#include <smiFixedTableDef.h>
#include <smuHash.h>

class smiFixedTable
{
    static smuHashBase        mTableHeaderHash;
    static SShort             mSizeOfDataType[];
    
    static UInt genHashValueFunc(void *aKey);
    static SInt compareFunc(void* aLhs, void* aRhs);

    static void initColumnInformation(iduFixedTableDesc *);
    static void initFixedTableHeader(smiFixedTableHeader *, iduFixedTableDesc *);

    static IDE_RC buildOneRecord(iduFixedTableDesc *aTabDesc,
                                 UChar              *aRowBuf,
                                 UChar              *aObj);

    /* ------------------------------------------------
     * 레코드의 메모리를 순차적으로 증가하면서 할당받음.
     * API Sets
     *  - buildRecord()  : 하나씩 할당후 증가.
     * ----------------------------------------------*/
    
    static IDE_RC buildRecord(void      *aHeader,
                              iduFixedTableMemory *aMemory,
                              void      *aObj);

    /* BUG-43006 Fixed Table Indexing Filter */
    static void buildOneColumn( iduFixedTableDataType          aDataType,
                                UChar                        * aObj,
                                void                         * aSourcePtr,
                                UInt                           aSourceSize,
                                void                         * aTargetPtr,
                                iduFixedTableConvertCallback   aConvCallback );

    static void   setupEndOfRecord(UChar     *aCurRecord);
    

    
public:
    static IDE_RC initializeStatic( SChar *aNameType );
    static IDE_RC destroyStatic();


    // for QP to get FixedTable Schema
    
    static IDE_RC findTable(SChar *aName, void **aHeader);

    static idBool canUse(void *aHeader);
    static UInt   getColumnCount(void *aHeader);
    static SChar *getColumnName(void *aHeader, UInt aNum);
    static UInt   getColumnOffset(void *aHeader, UInt aNum);

    static UInt   getColumnLength(void *aHeader, UInt aNum); // 실제 데이타 크기
    static iduFixedTableDataType getColumnType(void *aHeader, UInt aNum);
    static void   setNullRow(void *aHeader, void *aNullRow);
    
    // for Interator : smcTable
    static UInt   getSlotSize(void *aHeader);
    static iduFixedTableBuildFunc getBuildFunc(void *aHeader);


    // for converting Functions
    
    static UInt convertSCN(void        *aBaseObj,
                             void        *aMember,
                             UChar       *aBuf,
                             UInt         aBufSize);

    static UInt convertTIMESTAMP(void        *aBaseObj,
                                   void        *aMember,
                                   UChar       *aBuf,
                                   UInt         aBufSize);

    static UInt convertAlignedTIMESTAMP( void       * aBaseObj,
                                         void       * aMember,
                                         UChar      * aBuf,
                                         UInt         aBufSize );

    static UInt convertShardPinToString( void       * aBaseObj,
                                         void       * aMember,
                                         UChar      * aBuf,
                                         UInt         aBufSize );


    // BUG-42639 Monitoring query
    static IDE_RC build( idvSQL               * aStatistics,
                         void                 * aHeader,
                         void                 * aDumpObject,
                         iduFixedTableMemory  * aFixedMemory,
                         UChar               ** aRecRecord,
                         UChar               ** aTraversePtr );

    static IDE_RC checkFilter( void   * aProperty,
                               void   * aRecord,
                               idBool * aResult );

    static void checkLimitAndMovePos( void   * aProperty,
                                      idBool * aLimitResult,
                                      idBool   aIsMovePos );

    static IDE_RC checkLastLimit( void   * aProperty,
                                  idBool * aIsLastLimitResult );

    static void * getRecordPtr( UChar * aCurRec );

    static idBool useTrans( void * aHeader );

    /* BUG-43006 Fixed Table Indexing Filter */
    static idBool checkKeyRange( iduFixedTableMemory   * aMemory,
                                 iduFixedTableColDesc  * aColDesc,
                                 void                 ** aObjects );
};

#endif /* _O_SMI_FIXED_TABLE_H_ */
