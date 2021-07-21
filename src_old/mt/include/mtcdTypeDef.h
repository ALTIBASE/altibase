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
 * $Id: mtdTypeDef.h 19015 2006-11-19 23:43:45Z sungminee $
 *
 * ul에서 참조하므로 상수만 정의하여야 함.
 *
 **********************************************************************/

#ifndef _O_MTCD_TYPE_DEF_H_
#define _O_MTCD_TYPE_DEF_H_  1

// PROJ-1364
// mtdModule.id
#define MTD_MAX_COUNT       35 // 전체 type의 개수

#define MTD_BIGINT_ID       (acp_uint32_t)-5                    // 1개
#define MTD_BINARY_ID       (acp_uint32_t)-2                    //
#define MTD_BLOB_ID         30                          //
#define MTD_BLOB_LOCATOR_ID 31    // PROJ-1362          //
#define MTD_CLOB_ID         40                          // 5개
#define MTD_CLOB_LOCATOR_ID 41    // PROJ-1362          //
#define MTD_BOOLEAN_ID      16                          //
#define MTD_BIT_ID          (acp_uint32_t)-7                    //
#define MTD_VARBIT_ID       (acp_uint32_t)-100  // VARBIT은 ODBC에 정의되어 있지 않다.
#define MTD_CHAR_ID         1                           // 10개
#define MTD_DATE_ID         9                           //
#define MTD_DOUBLE_ID       8                           //
#define MTD_FLOAT_ID        6                           //
#define MTD_BYTE_ID         20001                       //
#define MTD_NIBBLE_ID       20002                       // 15개
#define MTD_VARBYTE_ID      20003                       //
#define MTD_INTEGER_ID      4                           //
#define MTD_INTERVAL_ID     10                          //
#define MTD_LIST_ID         10001                       //
#define MTD_NULL_ID         0                           //
#define MTD_NUMBER_ID       10002                       // 20개
#define MTD_NUMERIC_ID      2                           //
#define MTD_REAL_ID         7                           //
#define MTD_SMALLINT_ID     5                           //
#define MTD_VARCHAR_ID      12                          //
#define MTD_NONE_ID         (acp_uint32_t)-999                  // 25개
#define MTS_FILETYPE_ID     30001                       //
#define MTS_CONNECT_TYPE_ID 30002
#define MTD_GEOMETRY_ID     10003                       //
#define MTD_NCHAR_ID        (acp_uint32_t)-8   // PROJ-1579     //
#define MTD_NVARCHAR_ID     (acp_uint32_t)-9   // PROJ-1579     //
#define MTD_ECHAR_ID        60         // PROJ-2002     // 30개
#define MTD_EVARCHAR_ID     61         // PROJ-2002     // 

// PROJ-1075 사용자 정의 타입 또는 rowtype id
#define MTD_UDT_ID_MIN           (1000001) // type이 udt인지 비교용
#define MTD_ROWTYPE_ID           (1000001)
#define MTD_RECORDTYPE_ID        (1000002)
#define MTD_ASSOCIATIVE_ARRAY_ID (1000003)
#define MTD_REF_CURSOR_ID        (1000004)
#define MTD_UDT_ID_MAX           (1000004) // type이 udt인지 비교용
// PROJ-1075
// User-defined type은 mtdModule의 no 가 없다.
// 따라서 pre-defined된 값을 사용한다.
#define MTD_UDT_MODULE_NO     (999)



#endif /* _O_MTCD_TYPE_DEF_H_ */
