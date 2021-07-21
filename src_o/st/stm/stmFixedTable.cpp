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
 * $Id: stmFixedTable.cpp 16545 2006-06-07 01:06:51Z laufer $
 **********************************************************************/

#include <ideErrorMgr.h>
#include <stm.h>
#include <stmFixedTable.h>

// Fixed Table의 레코드를 정의한다.
stmUnitsFixedTbl     gLinearUnits2StrTbl[STM_NUM_LINEAR_UNITS] =
{
    { "M",                   "Meter",                         1.0 },        
    { "METER",               "Meter",                         1.0 },        
    { "KM",                  "Kilometer",                     1000.0 },     
    { "KILOMETER",           "Kilometer",                     1000.0 },     
    { "CM",                  "Centimeter",                    0.01 },       
    { "CENTIMETER",          "Centimeter",                    0.01 },       
    { "MM",                  "Millemeter",                    0.001 },      
    { "MILLIMETER",          "Millemeter",                    0.001 },      
    { "MILE",                "Mile",                          1609.344 },   
    { "NAUT_MILE",           "Nautical Mile",                 1852.0 },     
    { "SURVEY_FOOT",         "U.S. Foot",                     0.30480061 }, 
    { "FOOT",                "International Foot",            0.3048 },     
    { "INCH",                "Inch",                          0.0254 },     
    { "YARD",                "Yard",                          0.9144 },     
    { "CHAIN",               "Chain",                         20.1168 },    
    { "ROD",                 "Rod",                           5.0292 },     
    { "LINK",                "Link",                          0.201166195 },
    { "MOD_USFT",            "Modified American Foot",        0.304812253 },
    { "CL_FT",               "Clarke's Foot",                 0.304797265 },
    { "IND_FT",              "Indian Foot",                   0.304799518 },
    { "LINK_BEN",            "Link (Benoit)",                 0.201167651 },
    { "LINK_SRS",            "Link (Sears)",                  0.201167651 },
    { "CHN_BEN",             "Chain (Benoit)",                20.1167825 }, 
    { "CHN_SRS",             "Chain (Sears)",                 20.1167651 }, 
    { "IND_YARD",            "Yard (Indian)",                 0.914398554 },
    { "SRS_YARD",            "Yard (Sears)",                  0.914398415 },
    { "FATHOM",              "Fathom",                        1.8288 },     
    { "British foot (1936)", "British foot (1936)",           0.304800749 },
    { "",                    "metre",                         1.0 },        
    { "",                    "foot",                          0.3048 },     
    { "",                    "US survey foot",                0.30480061 }, 
    { "",                    "Clarke's foot",                 0.304797265 },
    { "",                    "fathom",                        1.8288 },     
    { "",                    "nautical mile",                 1852.0 },     
    { "",                    "German legal metre",            1.0000136 },  
    { "",                    "US survey chain",               20.1168402 }, 
    { "",                    "US survey link",                0.201168402 },
    { "",                    "US survey mile",                1609.34722 }, 
    { "",                    "kilometre",                     1000.0 },     
    { "",                    "Clarke's yard",                 0.914391795 },
    { "",                    "Clarke's chain",                20.1166195 }, 
    { "",                    "Clarke's link",                 0.201166195 },
    { "",                    "British yard (Sears 1922)",     0.914398415 },
    { "",                    "British foot (Sears 1922)",     0.304799472 },
    { "",                    "British chain (Sears 1922)",    20.1167651 }, 
    { "",                    "British link (Sears 1922)",     0.201167651 },
    { "",                    "British yard (Benoit 1895 A)",  0.9143992 },  
    { "",                    "British foot (Benoit 1895 A)",  0.304799733 },
    { "",                    "British chain (Benoit 1895 A)", 20.1167824 }, 
    { "",                    "British link (Benoit 1895 A)",  0.201167824 },
    { "",                    "British yard (Benoit 1895 B)",  0.914399204 },
    { "",                    "British foot (Benoit 1895 B)",  0.304799735 },
    { "",                    "British chain (Benoit 1895 B)", 20.1167825 }, 
    { "",                    "British link (Benoit 1895 B)",  0.201167825 },
    { "",                    "British foot (1865)",           0.304800833 },
    { "",                    "Indian foot",                   0.30479951 }, 
    { "",                    "Indian foot (1937)",            0.30479841 }, 
    { "",                    "Indian foot (1962)",            0.3047996 },  
    { "",                    "Indian foot (1975)",            0.3047995 },  
    { "",                    "Indian yard",                   0.914398531 },
    { "",                    "Indian yard (1937)",            0.91439523 }, 
    { "",                    "Indian yard (1962)",            0.9143988 },  
    { "",                    "Indian yard (1975)",            0.9143985 },  
    { "",                    "Statute mile",                  1609.344 },   
    { "",                    "Gold Coast foot",               0.30479971 }, 
    { "",                    "Bin width 330 US survey feet",  100.584201 }, 
    { "",                    "Bin width 165 US survey feet",  50.2921006 }, 
    { "",                    "Bin width 82.5 US survey feet", 25.1460503 }, 
    { "",                    "Bin width 37.5 metres",         37.5 },       
    { "",                    "Bin width 25 metres",           25.0 },         
    { "",                    "Bin width 12.5 metres",         12.5 },       
    { "",                    "Bin width 6.25 metres",         6.25 },       
    { "",                    "Bin width 3.125 metres",        3.125 }       
};

stmUnitsFixedTbl     gAreaUnits2StrTbl[STM_NUM_AREA_UNITS] =
{
    { "SQ_MM",          "Square Millimeter",  0.000001 },
    { "SQ_KILOMETER",   "Square Kilometer",   1000000.0 },
    { "SQ_CENTIMETER",  "Square Centimeter",  0.0001 },
    { "SQ_MILLIMETER",  "Square Millimeter",  0.000001 },
    { "SQ_CH",          "Square Chain",       404.6856 },
    { "SQ_FT",          "Square Foot",        0.09290304 },
    { "SQ_IN",          "Square Inch",        0.00064516 },
    { "SQ_LI",          "Square Link",        0.04046856 },
    { "SQ_CHAIN",       "Square Chain",       404.6856 },
    { "SQ_FOOT",        "Square Foot",        0.09290304 },
    { "SQ_INCH",        "Square Inch",        0.00064516 },
    { "SQ_LINK",        "Square Link",        0.04046856 },
    { "SQ_MILE",        "Square Mile",        2589988.0 },
    { "SQ_ROD",         "Square Rod",         25.29285 },
    { "SQ_SURVEY_FOOT", "Square Survey Feet", 0.09290341 },
    { "SQ_YARD",        "Square Yard",        0.8361274 },
    { "ACRE",           "Acre",               4046.856 },
    { "HECTARE",        "Hectare",            10000.0 },
    { "PERCH",          "Perch",              25.29285 },
    { "ROOD",           "Rood",               1011.714 },
    { "SQ_M",           "Square Meter",       1.0 },
    { "SQ_METER",       "Square Meter",       1.0 },
    { "SQ_KM",          "Square Kilometer",   1000000.0 },
    { "SQ_CM",          "Square Centimeter",  0.0001 }
};

stmUnitsFixedTbl     gAngularUnits2StrTbl[STM_NUM_ANGULAR_UNITS] =
{
    { "RAD",    "Radian",         1.0 },
    { "DEGREE", "Decimal Degree", (MTD_PIE/180.0) },
    { "MINUTE", "Decimal Minute", ((MTD_PIE/180.0)/60.0) },
    { "SECOND", "Decimal Second", ((MTD_PIE/180.0)/3600.0) },
    { "GON",    "Gon",            (MTD_PIE/200.0) },
    { "GRAD",   "Grad",           (MTD_PIE/200.0) }
};

iduFixedTableColDesc gLinearUnitColDesc[]=
{
    {
        (SChar*)"UNIT",
        offsetof( stmUnitsFixedTbl, mUnit ),
        IDU_FT_SIZEOF( stmUnitsFixedTbl, mUnit ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"UNIT_NAME",
        offsetof( stmUnitsFixedTbl, mUnitName ),
        IDU_FT_SIZEOF( stmUnitsFixedTbl, mUnitName ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },               
    {
        (SChar*)"CONVERSION_FACTOR",
        offsetof( stmUnitsFixedTbl, mConversionFactor ),
        IDU_FT_SIZEOF_DOUBLE,
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0, NULL // for internal use
    },               
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

iduFixedTableDesc  gLinearUnitTableDesc =
{
    (SChar *)"X$ST_LINEAR_UNIT",
    stmFixedTable::buildRecordForLinearUnits,
    gLinearUnitColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

iduFixedTableColDesc gAreaUnitColDesc[]=
{
    {
        (SChar*)"UNIT",
        offsetof( stmUnitsFixedTbl, mUnit ),
        IDU_FT_SIZEOF( stmUnitsFixedTbl, mUnit ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"UNIT_NAME",
        offsetof( stmUnitsFixedTbl, mUnitName ),
        IDU_FT_SIZEOF( stmUnitsFixedTbl, mUnitName ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },               
    {
        (SChar*)"CONVERSION_FACTOR",
        offsetof( stmUnitsFixedTbl, mConversionFactor ),
        IDU_FT_SIZEOF_DOUBLE,
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0, NULL // for internal use
    },               
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

iduFixedTableDesc  gAreaUnitTableDesc =
{
    (SChar *)"X$ST_AREA_UNIT",
    stmFixedTable::buildRecordForAreaUnits,
    gAreaUnitColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

iduFixedTableColDesc gAngularUnitColDesc[]=
{
    {
        (SChar*)"UNIT",
        offsetof( stmUnitsFixedTbl, mUnit ),
        IDU_FT_SIZEOF( stmUnitsFixedTbl, mUnit ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"UNIT_NAME",
        offsetof( stmUnitsFixedTbl, mUnitName ),
        IDU_FT_SIZEOF( stmUnitsFixedTbl, mUnitName ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },               
    {
        (SChar*)"CONVERSION_FACTOR",
        offsetof( stmUnitsFixedTbl, mConversionFactor ),
        IDU_FT_SIZEOF_DOUBLE,
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0, NULL // for internal use
    },               
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

iduFixedTableDesc  gAngularUnitTableDesc =
{
    (SChar *)"X$ST_ANGULAR_UNIT",
    stmFixedTable::buildRecordForAngularUnits,
    gAngularUnitColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC stmFixedTable::buildRecordForLinearUnits( idvSQL              * /*aStatistics*/,
                                                 void * aHeader,
                                                 void * /* aDumpObj */,
                                                 iduFixedTableMemory * aMemory )
{
    UInt i;
    for( i = 0; i < STM_NUM_LINEAR_UNITS; i++ )
    {
        IDE_TEST( iduFixedTable::buildRecord( aHeader, aMemory,
                                              (void *)& gLinearUnits2StrTbl[i] ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC stmFixedTable::buildRecordForAreaUnits( idvSQL              * /*aStatistics*/,
                                               void * aHeader,
                                               void * /* aDumpObj */,
                                               iduFixedTableMemory * aMemory )
{
    UInt i;
    for( i = 0; i < STM_NUM_AREA_UNITS; i++ )
    {
        IDE_TEST( iduFixedTable::buildRecord( aHeader, aMemory,
                                              (void *)& gAreaUnits2StrTbl[i] ) != IDE_SUCCESS );
    } 
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
        
    return IDE_FAILURE;
}

IDE_RC stmFixedTable::buildRecordForAngularUnits( idvSQL              * /*aStatistics*/,
                                                  void * aHeader, 
                                                  void * /* aDumpObj */,
                                                  iduFixedTableMemory * aMemory )
{
    UInt i;
    for( i = 0; i < STM_NUM_ANGULAR_UNITS; i++ )
    {
        IDE_TEST( iduFixedTable::buildRecord( aHeader, aMemory,
                                              (void *)& gAngularUnits2StrTbl[i] ) != IDE_SUCCESS );
    } // end for
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
