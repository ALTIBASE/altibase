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

#ifndef _O_ULN_ANALYZE_STMT_H_
#define _O_ULN_ANALYZE_STMT_H_ 1

#include <ulnPrivate.h>

/**
 *  ulnAnalyzeStmtCreate
 *
 *  @aAnalyzeStmt : ulnAnalyzeStmt의 객체
 *  @aStmtStr     : Statement 스트링
 *  @aStmtStrLen  : Statement 길이
 *
 *  = Statement를 분석해 토큰 List를 생성한다.
 *  = 성공하면 ACI_SUCCESS, 실패하면 ACI_FAILURE
 */
ACI_RC ulnAnalyzeStmtCreate(ulnAnalyzeStmt **aAnalyzeStmt,
                            acp_char_t      *aStmtStr,
                            acp_sint32_t     aStmtStrLen);

/**
 *  ulnAnalyzeStmtReInit
 *
 *  = 기존에 할당된 메모리를 이용해 Statement를 분석하고
 *    토큰 List를 생성한다.
 */
ACI_RC ulnAnalyzeStmtReInit(ulnAnalyzeStmt **aAnalyzeStmt,
                            acp_char_t      *aStmtStr,
                            acp_sint32_t     aStmtStrLen);

/**
 *  ulnAnalyzeStmtDestroy
 *
 *  @aAnalyzeStmt : ulnAnalyzeStmt의 Object
 *
 *  = ulnAnalyzeStmt 객체를 소멸한다.
 */
void ulnAnalyzeStmtDestroy(ulnAnalyzeStmt **aAnalyzeStmt);

/**
 *  ulnAnalyzeStmtGetPosArr
 *
 *  @aAnalyzeStmt : ulnAnalyzeStmt의 객체
 *  @aToken  : 찾을 토큰 스트링
 *  @aPosArr : Statement 내 aToken의 포지션을 저장한 배열
 *  @aPosCnt : 포지션 개수
 *
 *  = aToken이 PosArr에 있으면 그 배열과 포지션을 반환해 준다.
 *  = aToken이 PosArr에 없거나 Statement에 :name, ?가 혼용되어 있으면
 *    NULL과 0을 반환해 준다. 즉 Position 바인딩을 한다.
 *  = 성공하면 ACI_SUCCESS, 실패하면 ACI_FAILURE
 */
ACI_RC ulnAnalyzeStmtGetPosArr(ulnAnalyzeStmt  *aAnalyzeStmt,
                               acp_char_t      *aToken,
                               acp_uint16_t   **aPosArr,
                               acp_uint16_t    *aPosCnt);

#endif /* _O_ULN_ANALYZE_STMT_H_ */
