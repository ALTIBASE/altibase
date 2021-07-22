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
 * $Id: qmoListTransform.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description : BUG-36438 List Transformation
 *
 * 용어 설명 :
 *
 *****************************************************************************/
#ifndef _Q_QMO_LIST_TRANSFORM_H_
#define _Q_QMO_LIST_TRANSFORM_H_ 1

#include <ide.h>
#include <qtcDef.h>
#include <qmsParseTree.h>

#define QMO_LIST_TRANSFORM_ARGUMENTS_COUNT  2
#define QMO_LIST_TRANSFORM_DEPENDENCY_COUNT 2

//-----------------------------------------------------------
// LIST Transform 관리 함수
//-----------------------------------------------------------
class qmoListTransform
{
public:

    // 최초 NNF 형태의 모든 조건절에 대한 LIST transformation
    // - Where clause
    // - On condition
    // - Having clause
    static IDE_RC doTransform( qcStatement * aStatement,
                               qmsQuerySet * aQuerySet );

private:

    // From 절의 onCondition 에 대한 LIST transformation (재귀)
    static IDE_RC doTransform4From( qcStatement * aStatement,
                                    qmsFrom     * aFrom );

    // LIST transformation
    static IDE_RC listTransform( qcStatement * aStatement,
                                 qtcNode    ** aNode );

    // Predicate list 생성
    static IDE_RC makePredicateList( qcStatement  * aStatement,
                                     qtcNode      * aCompareNode,
                                     qtcNode     ** aResult );

    // 변환 조건 검사
    static IDE_RC checkCondition( qtcNode     * aNode,
                                  idBool      * aResult );

    // Predicate 생성
    static IDE_RC makePredicate( qcStatement  * aStatement,
                                 qtcNode      * aPredicate,
                                 qtcNode      * aOperand1,
                                 qtcNode      * aOperand2,
                                 qtcNode     ** aResult );
};

#endif  /* _Q_QMO_LIST_TRANSFORM_H_ */
