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
 * $Id: smcCatalogTable.h 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

#ifndef _O_SMC_CATALOG_TABLE_H_
#define _O_SMC_CATALOG_TABLE_H_ 1


#include <smDef.h>
#include <smcDef.h>

class smcCatalogTable
{
public:

    /* DB 생성시에 Catalog Table과 Temp Catalog Table 생성 */
    static IDE_RC createCatalogTable();

    /* Server shutdown시 Catalog Table에 대해 메모리 해제 수행 */
    static IDE_RC finalizeCatalogTable();

    /* Server startup시 Catalog Table에 대해 초기화 수행 */
    static IDE_RC initialize();

    /* Server shutdown시 Catalog Table에 대해 메모리 해제 수행 */
    static IDE_RC destroy();

    /* Temp Catalog Table Offset를 return */
    static UInt getCatTempTableOffset();

    /* restart recovery시에
       disk table의 header를 초기화하고 해당 table의
       모든 index runtime header를
       rebuild 한다.*/
    static IDE_RC refineDRDBTables();

   static IDE_RC doAction4EachTBL(idvSQL            * aStatistics,
                                   smcAction4TBL       aAction,
                                   void              * aActionArg );

private:

   // Create DB시 Catalog Table을 생성한다.
    static IDE_RC createCatalog( void*   aCatTableHeader,
                                 UShort  aOffset );

    // Shutdown시 Catalog Table의 해제작업을 수행
    static IDE_RC finCatalog( void* sCatTableHeader );

    //  Catalog Table안의 Used Slot에 대해 Lock Item과 Runtime Item해제
    static IDE_RC finAllocedTableSlots( smcTableHeader * aCatTblHdr );
};

#endif /* _O_SMC_CATALOG_TABLE_H_ */
