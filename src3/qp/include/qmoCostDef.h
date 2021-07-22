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
 * $Id: qmoCostDef.h 85333 2019-04-26 02:34:41Z et16 $
 *
 * Description :
 *
 *    Cost Factor 상수의 정의
 *
 *    비용 계산 시 적용되는 상수값을 여기에 정의함.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_COST_DEF_H_
#define _O_QMO_COST_DEF_H_ 1

// Join을 DNF 로 처리하는 것은 바람직하지 않다.
// 이에 대한 Penalty를 부여한다.
#define QMO_COST_DNF_JOIN_PENALTY                                  (10)

//----------------------------------------------
// 적용할 수 없는 경우의 비용
//----------------------------------------------
#define QMO_COST_INVALID_COST                                      (-1)

// 최소 버퍼 개수
#define QMO_COST_DISK_MINIMUM_BUFFER                               (10)

//----------------------------------------------------------------------
// 중간 결과 저장 방식
//----------------------------------------------------------------------

// RID 저장 방식과 Push Projection에 의한 Value 저장 방식으로 나뉜다.
typedef enum
{
    QMO_STORE_RID = 0,
    QMO_STORE_VAL,
    QMO_STORE_MAXMAX
} qmoStoreMethod;

//----------------------------------------------
// temp table type
//----------------------------------------------

#define QMO_COST_STORE_SORT_TEMP                                    (0)
#define QMO_COST_STORE_HASH_TEMP                                    (1)

#define QMO_COST_DEFAULT_NODE_CNT                                   (1)

#define QMO_COST_FIRST_ROWS_FACTOR_DEFAULT                        (1.0)

#define QMO_COST_EPS              (1e-8)
#define QMO_COST_IS_GREATER(x, y) (((x) > ((y) + QMO_COST_EPS)) ? ID_TRUE : ID_FALSE)
#define QMO_COST_IS_LESS(x, y)    (((x) < ((y) - QMO_COST_EPS)) ? ID_TRUE : ID_FALSE)
#define QMO_COST_IS_EQUAL(x, y)   ((((x)-(y) > -QMO_COST_EPS) && ((x)-(y) < QMO_COST_EPS)) ? ID_TRUE : ID_FALSE)

#endif /* _O_QMO_COST_DEF_H_ */
