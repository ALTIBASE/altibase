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
 * $Id: qtcSkip.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 *     SKIP에 해당하는 Data Type
 *     별도의 Data Type이 없는 개념을 갖는 노드들에 대한
 *     Column 정보를 Setting하기 위해 사용된다.
 *
 *     다음과 같은 용도를 위해 사용된다.
 *     1.  Indirect Node의 Column 정보
 *         - qtc::makeIndirect()
 *     2.  처리할 필요가 없는 Node에 대한 Column 정보
 *         - qtc::modifyQuantifedExpression()
 *         - WHERE I1 IN ( (SELECT A1 FROM T1) )
 *                       *                     *
 *     3. 처리가 필요 없는 Procedure Variable
 *         - qtc::makeProcVariable()
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <qtc.h>

//-----------------------------------------
// Skip 데이터 타입의 Module 에 대한 정보
//-----------------------------------------

static IDE_RC mtdEstimate( UInt * aColumnSize,
                           UInt * aArguments,
                           SInt * aPrecision,
                           SInt * aScale );

mtdModule qtc::skipModule = {
    NULL,           // 이름
    NULL,           // 컬럼 정보
    MTD_NULL_ID,    // data type id
    0,              // no
    {0,0,0,0,0,0,0,0}, // index type
    1,              // align 값
    0,              // flag
    0,              // max precision
    0,              // min scale
    0,              // max scale
    NULL,           // staticNull 값
    NULL,           // 서버 구동시 초기화 함수
    mtdEstimate,    // Estimation 함수
    NULL,           // 문자열 => Value 전환 함수
    NULL,           // 실제 Data의 크기 추출 함수
    NULL,           // 실제 Data의 precision을 얻는 함수
    NULL,           // NULL 값 추출 함수
    NULL,           // Hash 값 추출 함수
    NULL,           // NULL여부 판단 함수
    NULL,           // Boolean TRUE 판단 함수
    {
        mtd::compareNA,           // Logical Comparison
        mtd::compareNA
    },
    {                   // Key Comparison
        {
            mtd::compareNA,    // Ascending Key Comparison
            mtd::compareNA     // Descending Key Comparison
        },
        {
            mtd::compareNA,    // Ascending Key Comparison
            mtd::compareNA     // Descending Key Comparison
        },
        {
            mtd::compareNA,
            mtd::compareNA
        },
        {
            mtd::compareNA,
            mtd::compareNA
        }
    },
    NULL,           // Canonize 함수
    NULL,           // Endian 변경 함수
    NULL,           // Validation 함수
    NULL,           // Selectivity 함수
    NULL,           // Enconding 함수
    NULL,           // Decoding 함수
    NULL,           // Compfile Format 함수
    NULL,           // Oracle로부터 Value 추출 함수
    NULL,           // Meta 변경 함수

    // BUG-28934
    NULL,
    NULL,
    
    {
        // PROJ-1705
        NULL,           
        // PROJ-2429
        NULL
    },
    NULL,           
    NULL,            
    
    //PROJ-2399
    NULL
};

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * /*aArguments*/,
                    SInt * /*aPrecision*/,
                    SInt * /*aScale*/ )
{
/***********************************************************************
 *
 * Description :
 *
 *    Skip Data Type의 Column 정보를 Setting함
 *
 * Implementation :
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC mtdEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    /*
    aColumn->column.flag     = SMI_COLUMN_TYPE_FIXED;
    aColumn->column.size     = 0;
    aColumn->type.dataTypeId = qtc::skipModule.id;
    aColumn->flag            = 0;
    aColumn->precision       = 0;
    aColumn->scale           = 0;
    aColumn->module          = &qtc::skipModule;
    */

    *aColumnSize = 0;
    
    return IDE_SUCCESS;
    
#undef IDE_FN
}
