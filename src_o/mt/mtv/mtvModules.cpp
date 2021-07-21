/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtvModules.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtd.h>
#include <mtl.h>
#include <mtv.h>

extern mtvModule mtvBigint2Double;
extern mtvModule mtvBigint2Integer;
extern mtvModule mtvBigint2Numeric;
extern mtvModule mtvBigint2Varchar; // PROJ-1361
extern mtvModule mtvChar2Varchar;   // PROJ-1361 
extern mtvModule mtvDate2Varchar;   // PROJ-1361 
extern mtvModule mtvDouble2Bigint;
extern mtvModule mtvDouble2Float;
extern mtvModule mtvDouble2Interval;
extern mtvModule mtvDouble2Numeric;
extern mtvModule mtvDouble2Real;
extern mtvModule mtvFloat2Double;
extern mtvModule mtvFloat2Numeric;
extern mtvModule mtvFloat2Varchar;  // PROJ-1361 
extern mtvModule mtvInteger2Bigint;
extern mtvModule mtvInteger2Double;
extern mtvModule mtvInteger2Smallint;
extern mtvModule mtvInterval2Double;
extern mtvModule mtvNull2Bigint;
extern mtvModule mtvNull2Blob;
extern mtvModule mtvNull2Boolean;
extern mtvModule mtvNull2Char;      // PROJ-1361
extern mtvModule mtvNull2Clob;      // PROJ-1362
extern mtvModule mtvNull2Date;
extern mtvModule mtvNull2Double;
extern mtvModule mtvNull2Float;
extern mtvModule mtvNull2Integer;
extern mtvModule mtvNull2Interval;
extern mtvModule mtvNull2Numeric;
extern mtvModule mtvNull2Real;
extern mtvModule mtvNull2Smallint;
extern mtvModule mtvNull2Varchar;  // PROJ-1361 
extern mtvModule mtvNull2Byte;
extern mtvModule mtvNull2Nibble;
extern mtvModule mtvNull2Bit;      // PROJ-1571
extern mtvModule mtvNull2Varbit;   // PROJ-1571
extern mtvModule mtvNumeric2Bigint;
extern mtvModule mtvNumeric2Double;
extern mtvModule mtvNumeric2Float;
extern mtvModule mtvReal2Double;
extern mtvModule mtvSmallint2Integer;
extern mtvModule mtvVarchar2Char;  // PROJ-1361
extern mtvModule mtvVarchar2Date;  // PROJ-1361
extern mtvModule mtvVarchar2Float; // PROJ-1361

// PROJ-1365
extern mtvModule mtvDouble2Varchar;

// PROJ-1362
extern mtvModule mtvVarchar2Blob;
extern mtvModule mtvVarchar2Clob;
extern mtvModule mtvVarbyte2Blob;     // BUG-40973
extern mtvModule mtvBinary2Blob;      // PR-15636
extern mtvModule mtvBlobLocator2Blob;
extern mtvModule mtvClobLocator2Clob;
extern mtvModule mtvBlob2BlobLocator;
extern mtvModule mtvClob2ClobLocator;
extern mtvModule mtvNibble2Varchar;
extern mtvModule mtvVarchar2Nibble;

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern mtvModule mtvClob2Varchar;

// PROJ-1571
extern mtvModule mtvVarbit2Varchar;
extern mtvModule mtvBit2Varbit;
extern mtvModule mtvVarbit2Bit;

// PROJ-1579
extern mtvModule mtvNchar2Char;
extern mtvModule mtvChar2Nchar;
extern mtvModule mtvNvarchar2Varchar;
extern mtvModule mtvVarchar2Nvarchar;
extern mtvModule mtvNchar2Nvarchar;
extern mtvModule mtvNvarchar2Nchar;
extern mtvModule mtvNull2Nchar;
extern mtvModule mtvNull2Nvarchar;

// PROJ-2002 Column Security
extern mtvModule mtvChar2Echar;       
extern mtvModule mtvEchar2Char;
extern mtvModule mtvVarchar2Evarchar;
extern mtvModule mtvEvarchar2Varchar;
extern mtvModule mtvNull2Echar;       
extern mtvModule mtvNull2Evarchar;
extern mtvModule mtvNull2Varbyte; // BUG-40973

// PROJ-2163 Revise binding to improve stability
extern mtvModule mtvUndef2Bigint;
extern mtvModule mtvUndef2Bit;
extern mtvModule mtvUndef2Blob;
extern mtvModule mtvUndef2Boolean;
extern mtvModule mtvUndef2Byte;
extern mtvModule mtvUndef2Char;
extern mtvModule mtvUndef2Clob;
extern mtvModule mtvUndef2Date;
extern mtvModule mtvUndef2Double;
extern mtvModule mtvUndef2Echar;
extern mtvModule mtvUndef2Evarchar;
extern mtvModule mtvUndef2Float;
extern mtvModule mtvUndef2Integer;
extern mtvModule mtvUndef2Interval;
extern mtvModule mtvUndef2Nchar;
extern mtvModule mtvUndef2Nibble;
extern mtvModule mtvUndef2Null;
extern mtvModule mtvUndef2Numeric;
extern mtvModule mtvUndef2Nvarchar;
extern mtvModule mtvUndef2Real;
extern mtvModule mtvUndef2Smallint;
extern mtvModule mtvUndef2Varbit;
extern mtvModule mtvUndef2Varbyte;
extern mtvModule mtvUndef2Varchar;
extern mtvModule mtvUndef2List;
extern mtvModule mtvUndef2Number;
extern mtvModule mtvUndef2File;
extern mtvModule mtvUndef2BlobLocator;
extern mtvModule mtvUndef2ClobLocator;
extern mtvModule mtvUndef2Binary;

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern mtvModule mtvClobLocator2Clob;

extern mtvModule mtvByte2Varbyte;    // BUG-40973
extern mtvModule mtvVarbyte2Byte;    // BUG-40973
extern mtvModule mtvVarbyte2Varchar; // BUG-40973
extern mtvModule mtvVarchar2Varbyte; // BUG-40973

mtvModule** mtv::mAllModule = NULL;
const mtvModule* mtv::mInternalModule[] = {
    &mtvBigint2Double,
    &mtvBigint2Integer,
    &mtvBigint2Numeric,
    &mtvBigint2Varchar, // PROJ-1361
    &mtvChar2Varchar,   // PROJ-1361
    &mtvDate2Varchar,   // PROJ-1361
    &mtvDouble2Bigint,
    &mtvDouble2Float,
    &mtvDouble2Interval,
    &mtvDouble2Numeric,
    &mtvDouble2Real,
    &mtvDouble2Varchar, // PROJ-1365
    &mtvFloat2Double,
    &mtvFloat2Numeric,
    &mtvFloat2Varchar, // PROJ-1361
    &mtvInteger2Bigint,
    &mtvInteger2Double,
    &mtvInteger2Smallint,
    &mtvInterval2Double,
    &mtvNull2Bigint,
    &mtvNull2Blob,
    &mtvNull2Boolean,
    &mtvNull2Char,     // PROJ-1361
    &mtvNull2Clob,     // PROJ-1362
    &mtvNull2Date,
    &mtvNull2Double,
    &mtvNull2Float,
    &mtvNull2Integer,
    &mtvNull2Interval,
    &mtvNull2Numeric,
    &mtvNull2Real,
    &mtvNull2Smallint,
    &mtvNull2Varchar,  // PROJ-1361
    &mtvNull2Byte,
    &mtvNull2Nibble,
    &mtvNull2Bit,      // PROJ-1571
    &mtvNull2Varbit,   // PROJ-1571
    &mtvNumeric2Bigint,
    &mtvNumeric2Double,
    &mtvNumeric2Float,
    &mtvReal2Double,
    &mtvSmallint2Integer,
    &mtvVarchar2Char, // PROJ-1361
    &mtvVarchar2Date, // PROJ-1361
    &mtvVarchar2Float,// PROJ-1361
    &mtvVarchar2Blob, // PROJ-1362
    &mtvVarchar2Clob, // PROJ-1362
    &mtvVarbyte2Blob, // PROJ-1362, BUG-40973
    &mtvBinary2Blob,       // BUG-15636
    &mtvBlobLocator2Blob,  // PROJ-1362
    &mtvClobLocator2Clob,  // PROJ-1362
    &mtvBlob2BlobLocator,  // PROJ-1362
    &mtvClob2ClobLocator,  // PROJ-1362
    &mtvClob2Varchar,      /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    &mtvNibble2Varchar,    // PROJ-1362
    &mtvVarchar2Nibble,    // PROJ-1362
    &mtvVarbit2Varchar,  // PROJ-1571
    &mtvBit2Varbit,      // PROJ-1571
    &mtvVarbit2Bit,      // PROJ-1571
    &mtvNchar2Char,      // PROJ-1579
    &mtvChar2Nchar,      // PROJ-1579
    &mtvNvarchar2Varchar,// PROJ-1579
    &mtvVarchar2Nvarchar,// PROJ-1579
    &mtvNchar2Nvarchar,  // PROJ-1579
    &mtvNvarchar2Nchar,  // PROJ-1579
    &mtvNull2Nchar,      // PROJ-1579
    &mtvNull2Nvarchar,   // PROJ-1579
    &mtvChar2Echar,               // PROJ-2002
    &mtvEchar2Char,               // PROJ-2002
    &mtvVarchar2Evarchar,         // PROJ-2002
    &mtvEvarchar2Varchar,         // PROJ-2002
    &mtvNull2Echar,               // PROJ-2002
    &mtvNull2Evarchar,            // PROJ-2002
    &mtvUndef2Bigint,             // PROJ-2163 : Undef2*
    &mtvUndef2Bit,
    &mtvUndef2Blob,
    &mtvUndef2Boolean,
    &mtvUndef2Byte,
    &mtvUndef2Char,
    &mtvUndef2Clob,
    &mtvUndef2Date,
    &mtvUndef2Double,
    &mtvUndef2Echar,
    &mtvUndef2Evarchar,
    &mtvUndef2Float,
    &mtvUndef2Integer,
    &mtvUndef2Interval,
    &mtvUndef2Nchar,
    &mtvUndef2Nibble,
    &mtvUndef2Null,
    &mtvUndef2Numeric,
    &mtvUndef2Nvarchar,
    &mtvUndef2Real,
    &mtvUndef2Smallint,
    &mtvUndef2Varbit,
    &mtvUndef2Varbyte,
    &mtvUndef2Varchar,
    &mtvUndef2List,
    &mtvUndef2Number,
    &mtvUndef2File,
    &mtvUndef2BlobLocator,
    &mtvUndef2ClobLocator,
    &mtvUndef2Binary,
    &mtvByte2Varbyte,    // BUG-40973
    &mtvVarbyte2Byte,    // BUG-40973
    &mtvNull2Varbyte,    // BUG-40973
    &mtvVarbyte2Varchar, // BUG-40973
    &mtvVarchar2Varbyte, // BUG-40973
    NULL
};
 
