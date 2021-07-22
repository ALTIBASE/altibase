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
 * $Id: stmFixedTable.h 15866 2006-05-26 08:16:16Z laufer $
 **********************************************************************/

#ifndef _O_STM_FIXED_TABLE_H_
#define _O_STM_FIXED_TABLE_H_ 1

#include <mtdTypes.h>


#define STM_MAX_UNIT        (40)
#define STM_MAX_UNIT_NAME   (100)

#define STM_NUM_LINEAR_UNITS	(73)
#define STM_NUM_AREA_UNITS	    (24)
#define STM_NUM_ANGULAR_UNITS	(6)


typedef struct stmUnitsFixedTbl
{
    SChar       mUnit[STM_MAX_UNIT];
    SChar       mUnitName[STM_MAX_UNIT_NAME];
    SDouble     mConversionFactor;
} stmUnitsFixedTbl;


class stmFixedTable 
{
public:
    static IDE_RC buildRecordForLinearUnits( idvSQL              * /*aStatistics*/,
                                             void * aHeader, 
                                             void * aDumpObj,
                                             iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForAreaUnits( idvSQL              * /*aStatistics*/,
                                           void * aHeader,
                                           void * aDumpObj,
                                           iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForAngularUnits( idvSQL              * /*aStatistics*/,
                                              void * aHeader, 
                                              void * aDumpObj,
                                              iduFixedTableMemory * aMemory );

};

#endif /* _O_STM_FIXED_TABLE_H_ */
