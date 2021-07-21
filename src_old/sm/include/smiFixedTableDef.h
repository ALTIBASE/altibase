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
 * $Id: smiFixedTableDef.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMI_FIXED_TABLE_DEF_H_
# define _O_SMI_FIXED_TABLE_DEF_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smcDef.h>
#include <smpDef.h>
#include <iduFixedTable.h>

// 이 객체가 해쉬에 저장되어, 이름을 통해 접근함.
struct smcTableHeader;

/*
 * 아래의 smiFixedTableHeader type이 필요한 이유는
 * Fixed Table에서 사용될 테이블 헤더는
 * 순수하게 메모리 공간에서 할당된 이후에
 * 부가의 정보를 더 저장해야 하기 때문에 아래와 같이
 * 다시 선언하였다.
 * 이렇게 선언된 테이블 헤더는 QP에 의해 일반적인
 * 테이블 헤더 처럼 사용되지만, SM에서는
 * 뒤의 부가인자를 추가적으로 사용하게 된다.
 */
typedef struct smiFixedTableHeader
{
    smpSlotHeader      mSlotHeader;
    smcTableHeader     mHeader;
    iduFixedTableDesc *mDesc;
    void              *mNullRow;
} smiFixedTableHeader;

typedef struct smiFixedTableNode
{
    SChar               *mName;   //  same with  mDesc->mName
    smiFixedTableHeader *mHeader; //  Table 헤더
} smiFixedTableNode;

typedef struct smiFixedTableRecord
{
    smiFixedTableRecord *mNext;
    
}smiFixedTableRecord;



#endif /* _O_SMI_FIXED_TABLE_DEF_H_ */
