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
 * $Id$
 *
 * Description :
 *     PROJ-1877 Alter Column Modify에 대한 자료 구조 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QDB_DEF_H_
#define _O_QDB_DEF_H_ 1

#include <qc.h>

// BUG-42920 DDL display data move progress
#define QDB_PROGRESS_ROWS_COUNT   (100000)

#define QDB_CONVERT_MATRIX_SIZE   (19)

//----------------------------------------------------------------
// PROJ-1877 Alter Table Modify Column에 대한 간략한 설명을 추가합니다.
//
// [type 변경]
//
// type 변경은 크게 두 가지로 구분된다.
//
// 1. type 변경이지만 자료구조가 동일하여 length 변경과 동일한 경우
//    8가지 case
//    nchar->nvarchar
//    nvarchar->nchar
//    char->varchar
//    varchar->char
//    bit->varbit
//    varbit->bit
//    float->numeric
//    numeric->float
//
//    이를 좀 더 분류해보면 크게 세가지로 구분할 수 있다.
//    a. padding type에서 non-padding type으로 변환
//       3가지 case
//       nchar->nvarchar
//       char->varchar
//       bit->varbit
//
//       length 확대는 실시간 ddl로 가능하다.
//       length 축소는 모두 null인 경우 실시간 ddl로 수행이 가능하며
//       null이 아니라면 ddl이 실패한다.
//
//    b. non-padding type에서 padding type으로 변환
//       3가지 case
//       nvarchar->nchar
//       varchar->char
//       varbit->bit
//
//       length 확대는 pad 문자를 붙어야 하므로 recreate
//       length 축소는 length 조건을 만족하는 경우 실시간 ddl이 가능하며
//       length 조건을 만족하지 않는 경우 ddl이 실패한다.
//
//    c. float type에서 numeric type으로 변환
//       1가지 case
//       float->numeric
//
//       length 확대와 축소를 schema로 구분할 수 없으며
//       float의 값이 numeric의 length를 만족하는 경우 실시간 ddl이 가능하다.
//       그렇지 않은 경우 ddl이 실패한다.
//       단, data loss 옵션을 사용한 경우 length 조건을 만족하지 않더라도
//       float->numeric conversion을 통해 recreate한다.
//       이때 round-off가 발생하여 data loss가 발생할 수 있다.
//
//    d. numeric type에서 float type으로 변환
//       1가지 case
//       numeric->float
//
//       length 확대는 실시간 ddl로 가능하다.
//       length 축소는 length 조건을 만족하는 경우만 실시간 ddl이 가능하다.
//       단, data loss 옵션을 사용한 경우 length 조건을 만족하지 않더라도
//       numeric->float conversion을 통해 recreate한다.
//       이때 round-off가 발생하여 data loss가 발생할 수 있다.
//
// 2. type 변경이고 자료구조가 다른 경우
//    나머지 모든 경우
//    예)
//    char(4)->integer
//    varchar(8)->date
//
//    type 변경은 모두 recreate이며
//    data loss가 발생하는 type 변경인 경우 모두 null이어야 가능하다.
//    단, data loss 옵션을 사용한 경우 null이 아니어도 가능하며
//    type conversion을 통해 recreate한다.
//    이때 숫자형 type의 경우 round-off가 발생하여 data loss가 발생할 수 있으며
//    date type의 경우 default_date_format property에 의해 data loss가 발생할 수 있다.    
//
// [length 변경]
//
// type 변경없이 precision이나 scale을 변경한다.
//    10가지 case
//    nchar
//    nvarchar
//    char
//    varchar
//    bit
//    varbit
//    float
//    numeric
//    byte
//    nibble
//
//    이를 좀 더 분류해보면 크게 네가지로 구분할 수 있다.
//    a. padding type의 length 변경
//       4가지 case
//       nchar
//       char
//       bit
//       byte
//
//       length 확대는 모두 null이라면 실시간 ddl이 가능하나
//       그렇지 않은 경우는 recreate
//       length 축소는 모두 null이라면 실시간 ddl이 가능하지만
//       그렇지 않은 경우는 ddl이 실패한다.
//
//    b. non-padding type의 length 변경
//       4가지 case
//       nvarchar
//       varchar
//       varbit
//       nibble
//
//       length 확대는 실시간 ddl이 가능하다.
//       length 축소는 length 조건을 만족하는 경우에만 실시간 ddl이
//       가능하고, length 조건을 만족하지 않는 경우는 ddl이 실패한다.
//
//    c. float type의 length 변경
//       1가지 case
//       float
//
//       length 확대는 실시간 ddl이 가능하다.
//       length 축소는 length 조건을 만족하는 경우에만 실시간 ddl이
//       가능하고, length 조건을 만족하지 않는 경우는 ddl이 실패한다.
//       단, data loss 옵션을 사용한 경우 length 조건을 만족하지 않더라도
//       float canonize를 통해 recreate한다.
//       이때 round-off가 발생하여 data loss가 발생할 수 있다.
//
//    d. numeric type의 length 변경
//       1가지 case
//       numeric
//
//       length 확대는 실시간 ddl이 가능하다.
//       length 축소는 모두 null이 아니라면 ddl이 실패한다.
//       단, data loss 옵션을 사용한 경우 length 조건을 만족하지 않더라도
//       numeric canonize를 통해 recreate한다.
//       이때 round-off가 발생하여 data loss가 발생할 수 있다.
//
// [주의사항]
//
// 1. 실시간 ddl이라함은 ddl의 execution이 실시간(real-time)을 말하는 것으로
//    ddl의 validation이 실시간이 아닐 수 도 있다.
//
//    예를 들면 i1 varchar(5)를 varchar(3)으로 변경하는 ddl은
//    execution 자체는 실시간이지만, i1의 length가 모두 varchar(3)을
//    만족하는지 검사하기 위한 validation과정은 실시간이 아니다.
//    결국 사용자는 실시간이 아니라고 느낄 수 있지만,
//    개념적으로는 실시간 ddl이라고 간주한다. 또한 비실시간 ddl에 비해서
//    매우 빠르게 ddl을 수행한다. (scan 한번의 시간 정도)
//
//    실시간 ddl
//    - 실시간 validation + 실시간 execution  
//    - 비실시간 validation + 실시간 execution
//
//    비실시간 ddl
//    - 실시간 validation + 비실시간 execution  
//    - 비실시간 validation + 비실시간 execution
//
// 2. 실시간 ddl이 아직은 table의 record에 대해서만 가능한 것으로
//    index의 key에 대한 실시간 ddl은 아직 불가능하다.
//    그러므로 컬럼에 index가 있는 경우 해당 index는 재생성되어야 하며
//    index 재생성으로 인해 ddl의 execution이 실시간(real-time)으로
//    수행되지 않을 수 있다.
//
//    개념적으로 본다면 index가 있는 경우 ddl의 execution은 비실시간으로
//    수행되므로 비실시간 ddl로 구분한다.
//
// 3. data loss를 허용하는 옵션을 사용하여 type이나 length를 변경하는 경우
//    경우에 따라 modify가 실패하는 경우가 발생할 수 있다.
//
//    예를 들면 i1 float(2)를 i1 integer로 modify하려고 할때 i1에 unique index가
//    있는 경우 i1에 저장된 값에 따라서 unique index 생성이 실패할 수 있다.
//    이것은 ddl 수행전에 validation 과정에서 검사되면 좋겠지만,
//    현재는 index 생성과정에서 발견되어 ddl은 rollback이 된다.
//    (1.1, 1.2) -> (1, 1) unique violation error
//
//----------------------------------------------------------------

//----------------------------------------------------------------
// PROJ-1877
// Modify Column 수행시,
// Table Column의 modify method 방식
//----------------------------------------------------------------

typedef enum
{
    // 아무것도 하지 않아도 alter table modify column 기능수행
    QD_TBL_COL_MODIFY_METHOD_NONE = 0,

    // column 관련 meta 변경만으로 alter table modify column 기능수행
    QD_TBL_COL_MODIFY_METHOD_ALTER_META,
    
    // table 재생성으로 alter table modify column 기능수행
    QD_TBL_COL_MODIFY_METHOD_RECREATE_TABLE
    
} qdTblColModifyMethod;

//----------------------------------------------------------------
// PROJ-1911
// Modify Column 수행 시, 
// Index Column의 modify method 방식 
//----------------------------------------------------------------

typedef enum
{
    // index의 칼럼에 대한 meta 변경만으로
    // alter table modify column 기능수행
    QD_IDX_COL_MODIFY_METHOD_ALTER_META = 0,

    // index 재생성으로 alter table modify column 기능수행
    QD_IDX_COL_MODIFY_METHOD_RECREATE_INDEX
} qdIdxColModifyMethod;

typedef struct qdbIdxColModify
{
    UInt                   indexId;
    qdIdxColModifyMethod   method;
}qdIdxColModify;

//----------------------------------------------------------------
// PROJ-1877
// verify column command
//----------------------------------------------------------------

typedef enum
{
    //----------------------------------------------------------------
    // function: 초기값
    // action  : 아무것도 하지 않는다.
    //----------------------------------------------------------------
    QD_VERIFY_NONE = 0,

    //----------------------------------------------------------------
    // function: value가 모두 not null인가?
    //
    // action  : null to not null에 사용되며 null이 발견되는 경우
    //           DDL은 실패한다.
    //
    // example : create table t1(i1 integer null);
    //           alter table t1 modify (i1 not null);
    //           i1은 모두 not null이어야 alter가 가능하다. 그러나
    //           i1이 null인 경우 alter가 불가능하다.
    //----------------------------------------------------------------
    QD_VERIFY_NOT_NULL,
    
    //----------------------------------------------------------------
    // function: value가 모두 null 인가?
    //
    // action  : disk table에서 value가 모두 null인 경우 실시간
    //           완성으로 수행가능하다.
    //           null이 아닌 경우 DDL은 실패한다.
    //
    // example : create table t1(i1 char(3));
    //           alter table t1 modify (i1 char(1));
    //           i1이 모두 null이어야 alter가 가능하다. 그러나
    //           i1이 null이 아닌 경우 alter가 불가능하다.
    //----------------------------------------------------------------
    QD_VERIFY_NULL,
    
    //----------------------------------------------------------------
    // function: value가 모두 null 인가?
    //
    // action  : disk table에서 value가 모두 null인 경우 실시간
    //           완성으로 수행가능하다.
    //           null이 아닌 경우 비실시간으로 수행한다.
    //
    // example : create table t1(i1 char(3));
    //           alter table t1 modify (i1 char(5));
    //           i1이 모두 null인 경우 실시간 alter가 가능하다. 그러나
    //           i1이 null이 아닌 경우 recreate table을 수행한다.
    //----------------------------------------------------------------
    QD_VERIFY_NULL_ONLY,
    
    //----------------------------------------------------------------
    // function: value가 모두 null이거나 지정된 size이하인가?
    //
    // action  : value가 모두 null이 아니고 지정된 size이하가 아닌
    //           경우 DDL은 실패한다.
    //
    // example : create table t1(i1 varchar(5));
    //           alter table t1 modify (i1 varchar(3));
    //           i1이 모두 null이거나 길이가 3보다 작거나 같은 경우만
    //           alter가 가능하다. 그러나 i1의 길이가 3보다 큰 경우 alter가
    //           불가능하다.
    //----------------------------------------------------------------
    QD_VERIFY_NULL_OR_UNDER_SIZE,
    
    //----------------------------------------------------------------
    // function: value가 모두 null이거나 지정된 size인가?
    //
    // action  : disk table에서 value가 모두 null이거나 특정 size인
    //           경우 실시간 완성으로 수행가능하다.
    //           그리고 disk table에서 value가 모두 null이거나
    //           특정 size보다 작은 경우 비실시간으로 수행한다.
    //           그러나 size보다 큰 경우가 있으면 DDL은 실패한다.
    //
    // example : create table t1(i1 varchar(5));
    //           alter table t1 modify (i1 char(3));
    //           i1이 모두 null이거나 길이가 정확히 3인 경우 실시간 alter가
    //           가능하고, i1이 모두 null이거나 길이가 3보다 작거나 같은
    //           경우 recreate table을 수행한다. 그러나 i1의 길이가 3보다
    //           큰 경우 alter가 불가능하다.
    //----------------------------------------------------------------
    QD_VERIFY_NULL_OR_EXACT_OR_UNDER_SIZE,
    
    //----------------------------------------------------------------
    // function: value가 지정된 srid인가?
    //
    // action  : disk table에서 value가 지정된 srid이거나 0인
    //           경우 실시간 완성으로 수행가능하다.
    //           그리고 disk table에서 value가 모두 지정된 srid 혹은 0이
    //           아닌 경우 DDL은 실패한다.
    //
    // example : create table t1(i1 geometry);
    //           alter table t1 modify (i1 srid 100);
    //           i1이 모두 srid가 0이거나 100인 경우 실시간 alter가
    //           가능하고, i1이 0이나 100이 아닌 경우 alter가 불가능하다.
    //----------------------------------------------------------------
    QD_VERIFY_SRID
    
} qdVerifyCommand;

//----------------------------------------------------------------
// PROJ-1911
// modify column 수행 후, disk 저장 타입이 변경되는지 여부 
//----------------------------------------------------------------
typedef enum
{
    // Disk에 저장되지 않음 
    QD_CHANGE_STORED_TYPE_NONE,

    // Disk 저장 타입이 변경되지 않음
    // ex ) char->varchar, char->char 등등 
    QD_CHANGE_STORED_TYPE_FALSE,

    // Disk 저장 타입이 변경됨
    // ex) char->integer, varchar->date 등등 
    QD_CHANGE_STORED_TYPE_TRUE
} qdChangeStoredType;

//----------------------------------------------------------------
// PROJ-1877
// verify column list는 한번의 scan으로 modify 대상 컬럼을 한꺼번에
// 검사하여 alter가 불가능한지 검사하거나 modify method를 최종 결정한다.
//
// example : create table t1(i1 varchar(5), i2 varchar(5), i3 char(3));
//           alter table t1 modify (i1 varchar(3) not null, i2 char(3), i3 char(5));
//
//                      +----------------------------+  
//                      |i1                          |  
// verifyColumnList --> |QD_VERIFY_NULL_OR_UNDER_SIZE|
//                      |3                           |
//                      +----------------------------+
//                                |
//                      +------------------+
//                      |i1                |
//                      |QD_VERIFY_NOT_NULL|
//                      |0                 |
//                      +------------------+
//                                |
//                      +-------------------------------------+
//                      |i2                                   |
//                      |QD_VERIFY_NULL_OR_EXACT_OR_UNDER_SIZE|
//                      |3                                    |
//                      +-------------------------------------+
//                                |
//                      +-------------------+
//                      |i3                 |
//                      |QD_VERIFY_NULL_ONLY|
//                      |0                  |
//                      +-------------------+
//
//----------------------------------------------------------------

typedef struct qdVerifyColumn
{
    qcmColumn          * column;
    qdVerifyCommand      command;
    UInt                 size;
    SInt                 precision;
    SInt                 scale;
    qdChangeStoredType   changeStoredType;
    qdVerifyColumn     * next;
} qdVerifyColumn;

//----------------------------------------------------------------
// PROJ-1877
// convert for smiValue list
//----------------------------------------------------------------

typedef struct qdbConvertContext
{
    idBool               needConvert;
    mtvConvert         * convert;

    idBool               needCanonize;
    void               * canonBuf;

    // PROJ-2002 Column Security
    idBool               needEncrypt;
    idBool               needDecrypt;
    void               * encBuf;

    qdbConvertContext  * next;
    
} qdbConvertContext;

typedef struct qdbCallBackInfo
{
    // record 단위로 convert를 위한 메모리 관리
    iduMemoryStatus    * qmxMemStatus;
    iduMemory          * qmxMem;

    // column value의 convert를 위한 자료구조
    qcTemplate         * tmplate;
    qcmTableInfo       * tableInfo;
    qdbConvertContext  * convertContextList;

    // convert context를 찾기 위한 pointer
    qdbConvertContext  * convertContextPtr;

    // null smiValue list
    smiValue           * nullValues;

    /* PROJ-1090 Function-based Index */
    idBool               hasDefaultExpr;
    qmsTableRef        * srcTableRef;
    qcmColumn          * dstTblColumn;
    void               * rowBuffer;

    /* PROJ-2210 autoincrement column */
    qcStatement        * statement;
    qcmColumn          * srcColumns;
    qcmColumn          * dstColumns;

    // BUG-42920 DDL display data move progress
    qcmTableInfo       * partitionInfo; // partition name, partition 구분.
    ULong                progressRows;  // insert되어진 데이타의 개수.
    
} qdbCallBackInfo;

#endif /* _O_QDB_DEF_H_ */
