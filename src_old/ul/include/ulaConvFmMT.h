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
 
/*
 * -----------------------------------------------------------------------------
 *  ALA_GetODBCCValue() 함수는
 *      ALA_Value 를 입력으로 받아서 사용자가 aODBCCTypeID 에 지정한 타입으로
 *      타입을 변환한 후 그 결과 데이터를
 *      aOutODBCCValueBuffer 가 가리키는 버퍼에 담아 주는 함수이다.
 *
 *      이 때, 타입의 변환은
 *
 *      mt --> cmt --> ulnColumn --> odbc
 *
 *      의 순서로 이루어진다.
 *
 *      그 중 ul 쪽의 변환은 uln 의 함수들을 이용해서 할 수 있으나
 *      PROJ-1000 Client C Porting 당시 서버쪽의 모듈은 C 로 포팅하지 않아서
 *      mt --> cmt 의 변환에 사용된 mmcSession 을 이용할 수 없는 상황이었다.
 *
 *      본 파일 (ulaConv.c) 은 mmcSession 이 수행하던 mt --> cmt 의
 *      변환 코드를 그래도 가져와서 C 로 포팅한 코드이다.
 *
 *      mmcConvFmMT.cpp 파일 참조.
 * -----------------------------------------------------------------------------
 */

#include <ulaConvNumeric.h>

ACI_RC ulaConvConvertFromMTToCMT(cmtAny           *aTarget,
                                 void             *aSource,
                                 acp_uint32_t      aSourceType,
                                 acp_uint32_t      aLobSize,
                                 ulaConvByteOrder  aByteOrder);

