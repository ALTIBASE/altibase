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
 
/******************************************************************************
 * $Id$
 *
 * Description : PROJ-2242 Filter Subsumption Transformation
 *
 *       - TRUE, FALSE 에 대한 AND, OR 를 적용하여 subsumption 수행
 *       - QTC_NODE_JOIN_OPERATOR_EXIST 일 경우 수행 안함
 *       - subquery, host variable, GEOMETRY type arguments 제외
 *       - __OPTIMIZER_CONSTANT_FILTER_SUBSUMPTION property 로 동작
 *
 * 용어 설명 :
 *
 * 약어 : CFS (Constant Filter Subsumption)
 *
 *****************************************************************************/

#ifndef _Q_QMO_CFS_TRANSFORM_H_
#define _Q_QMO_CFS_TRANSFORM_H_ 1

#include <ide.h>
#include <qtcDef.h>
#include <qmsParseTree.h>

typedef enum qmoCFSCompareResult
{
    QMO_CFS_COMPARE_RESULT_BOTH = 1,
    QMO_CFS_COMPARE_RESULT_WIN,
    QMO_CFS_COMPARE_RESULT_LOSE
} qmoCFSCompareResult;

//-----------------------------------------------------------
// CFS Transform 관리 함수
//-----------------------------------------------------------

class qmoCFSTransform
{
public:

    // 최초 NNF 형태의 모든 조건절에 대한 CFS transformation
    static IDE_RC doTransform4NNF( qcStatement * aStatement,
                                   qmsQuerySet * aQuerySet );

private:

    // From 절의 onCondition 에 대한 CFS transformation (재귀)
    static IDE_RC doTransform4From( qcStatement * aStatement,
                                    qmsFrom     * aFrom );

    // NNF 형태의 predicate list 에 대한 CFS transformation
    static IDE_RC doTransform( qcStatement * aStatement,
                               qtcNode    ** aNode );

    // NNF 형태의 constant predicate 에 대한 CFS transformation
    static IDE_RC constantFilterSubsumption( qcStatement * aStatement,
                                             qtcNode    ** aNode,
                                             idBool        aIsRoot );

};

#endif  /* _Q_QMO_CFS_TRANSFORM_H_ */
