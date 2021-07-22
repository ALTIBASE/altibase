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

#ifndef _O_CMT_NUMERIC_H_
#define _O_CMT_NUMERIC_H_ 1

// BUG-20652
// 연산의 결과로 MT numeric type의 최대 precision은 40까지 생성될 수 있다.
#define CMT_NUMERIC_DATA_SIZE 17


typedef struct cmtNumeric
{
    UChar  mSize;
    UChar  mPrecision;
    SShort mScale;
    SChar  mSign;

    UChar  mData[CMT_NUMERIC_DATA_SIZE];
} cmtNumeric;


#endif
