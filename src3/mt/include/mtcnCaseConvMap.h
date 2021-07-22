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
 * $Id: idnCharMap.h 24174 2007-11-19 01:23:49Z copyrei $
 *
 * Description :
 *     gNlsCaseCharMap : uppercase, lowercase 변환 문자표
 *
 *     uppercase/lowercase 유니코드 범위 : U+0000 ~ U+2D26
 *
 **********************************************************************/

#ifndef _O_MTCASECONVMAP_H_
#define _O_MTCASECONVMAP_H_ 1

// 0x2D26 이후의 유니코드에 대해서는 대소문자 변환이 의미가 없음
#define MTN_NLS_CASE_UNICODE_MAX        (0x2D26)  /* 11558 */

typedef struct mtnNlsCaseConvMap
{
    acp_uint16_t      upper;
    acp_uint16_t      lower;
} mtnNlsCaseConvMap;

#endif /* _O_MTCASECONVMAP_H_ */
