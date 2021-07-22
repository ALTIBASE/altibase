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

#ifndef _O_CMT_DEF_H_
#define _O_CMT_DEF_H_ 1


/*
 * CMT에서 사용하는 상수들을 정의한다.
 *
 * 프로토콜 버전업으로 상수를 추가할 때 기존 정의를 고치면 안된다.
 * 다음의 예처럼 상수를 추가한다.
 *
 * 예)
 *
 *    enum {
 *        CM_ID_NONE = 0,
 *        ...
 *        CM_ID_MAX_VER1
 *    };  <-- 프로토콜 버전1의 기존 정의
 *
 *    enum {
 *        CM_ID_NEW = CM_ID_MAX_VER1,
 *        ...
 *        CM_ID_MAX_VER2
 *    };  <-- 프로토콜 버전2의 새로운 상수 정의
 *
 *    #define CM_ID_MAX CM_ID_MAX_VER2  <-- 마지막 프로토콜 버전의 MAX로 정의
 */


/*
 * Data Type ID
 */

enum {
    CMT_ID_NONE = 0,
    CMT_ID_NULL,
    CMT_ID_SINT8,
    CMT_ID_UINT8,
    CMT_ID_SINT16,
    CMT_ID_UINT16,
    CMT_ID_SINT32,
    CMT_ID_UINT32,
    CMT_ID_SINT64,
    CMT_ID_UINT64,
    CMT_ID_FLOAT32,
    CMT_ID_FLOAT64,
    CMT_ID_DATETIME,
    CMT_ID_INTERVAL,
    CMT_ID_NUMERIC,
    CMT_ID_VARIABLE,
    CMT_ID_IN_VARIABLE,
    CMT_ID_BINARY,
    CMT_ID_IN_BINARY,
    CMT_ID_BIT,
    CMT_ID_IN_BIT,
    CMT_ID_NIBBLE,
    CMT_ID_IN_NIBBLE,
    CMT_ID_LOBLOCATOR,  /* BUG-18945 */
    CMT_ID_PTR,         /* PROJ-1920 */
    CMT_ID_REDUNDANCY,  /* PROJ-2256 */
    CMT_ID_MAX_VER1
};

#define CMT_ID_MAX CMT_ID_MAX_VER1


#endif
