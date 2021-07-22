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
 * $Id: qmoUtil.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QMO_UTIL_H_
#define _Q_QMO_UTIL_H_ 1

#include <qc.h>
#include <qtc.h>

// BUG-19180
// printExpressionInPlan, printNodeFormat의 flag
// 하위 노드에서 이용할 상위 노드의 정보
#define QMO_PRINT_UPPER_NODE_MASK         (0x00000007)
#define QMO_PRINT_UPPER_NODE_NORMAL       (0x00000000)
#define QMO_PRINT_UPPER_NODE_COMPARE      (0x00000001)
#define QMO_PRINT_UPPER_NODE_MUL          (0x00000002)
#define QMO_PRINT_UPPER_NODE_DIV          (0x00000003)
#define QMO_PRINT_UPPER_NODE_ADD          (0x00000004)
#define QMO_PRINT_UPPER_NODE_SUB          (0x00000005)
#define QMO_PRINT_UPPER_NODE_MINUS        (0x00000006)

// BUG-19180
// printExpressionInPlan, printNodeFormat의 flag
// 하위 노드가 상위 노드의 오른쪽에 위치함을 나타냄
#define QMO_PRINT_RIGHT_NODE_MASK         (0x00000008)
#define QMO_PRINT_RIGHT_NODE_FALSE        (0x00000000)
#define QMO_PRINT_RIGHT_NODE_TRUE         (0x00000008)

class qmoUtil
{
public:
    static IDE_RC printPredInPlan(qcTemplate   * aTemplate,
                                  iduVarString * aString,
                                  ULong          aDepth,
                                  qtcNode      * aNode);

    static IDE_RC printExpressionInPlan(qcTemplate   * aTemplate,
                                        iduVarString * aString,
                                        qtcNode      * aNode,
                                        UInt           aParenthesisFlag );

    static IDE_RC unparseStatement( qcTemplate   * aTemplate,
                                    iduVarString * aString,
                                    qcStatement  * aStatement );

private:
    static IDE_RC printNodeFormat(qcTemplate   * aTemplate,
                                  iduVarString * aString,
                                  qtcNode      * aNode,
                                  UInt           aParenthesisFlag );

    static IDE_RC unparseQuerySet( qcTemplate   * aTemplate,
                                   iduVarString * aString,
                                   qmsQuerySet  * aQuerySet );

    static IDE_RC unparseFrom( qcTemplate   * aTemplate,
                               iduVarString * aString,
                               qmsFrom      * aFrom );

    static IDE_RC unparsePredicate( qcTemplate   * aTemplate,
                                    iduVarString * aString,
                                    qtcNode      * aNode,
                                    idBool         aIsRoot );
};

#endif  // _Q_QMO_UTIL_H_

