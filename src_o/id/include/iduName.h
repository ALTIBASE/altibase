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
 * $Id: iduName.h 20179 2007-01-30 00:51:39Z leekmo $
 **********************************************************************/

#ifndef _O_IDU_NAME_H_
#define _O_IDU_NAME_H_ 1

#include <idl.h>

/***********************************************************************
 *
 * Description : To Fix BUG-17430
 *
 *    Object Name 이 갖는 String에 대한 조작을 담당
 *
 * 용어 정의 :
 *
 *    ##################################
 *    # Name String에 대한 용어 정의
 *    ##################################
 *
 *    ## Name String 의 구분
 *
 *       - Oridinary  Name : 문자열 그대로 취급
 *       - Identifier Name : 대문자로 변경되어 처리되는 이름
 *
 *    ## Object Name의 해석
 *
 *       - Quoted Name : Ordinary Name으로 해석
 *       - Non-Quoted Name : 옵션에 따라 달리 해석
 *          ; SQL_ATTR_META_ID 설정 : Ordinary Name으로 해석
 *          ; SQL_ATTR_META_ID 미설정 : Identifier Name으로 해석
 *          ; 현재 미지원으로 Identifier Name으로 해석함 (BUG-17771)
 *
 * Implementation :
 *
 **********************************************************************/

// CLI 등의 함수 내에서 사용할 이름으로 생성
void iduNameMakeNameInFunc( SChar * aDstName,
                            SChar * aSrcName,
                            SInt    aSrcLen );

// SQL 문장 내에서 사용할 이름으로 생성
void iduNameMakeNameInSQL( SChar * aDstName,
                           SChar * aSrcName,
                           SInt    aSrcLen );

// Quoted Name을 생성
void iduNameMakeQuotedName( SChar * aDstName,
                            SChar * aSrcName,
                            SInt    aSrcLen );

// Quoted Name인지 판단
idBool iduNameIsQuotedName( SChar * aSrcName, SInt aSrcLen );

#endif // _O_IDU_NAME_H_

