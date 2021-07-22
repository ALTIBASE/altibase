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
 * $Id: qmoTransitivity.h 23857 2007-11-02 02:36:53Z sungminee $
 *
 * Description :
 *     PROJ-1404 Transitive Predicate Generation을 위한 자료 구조 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_TRANSITIVITY_H_
#define _O_QMO_TRANSITIVITY_H_ 1

#include <qc.h>
#include <qmoDependency.h>

#define QMO_OPERATION_MATRIX_SIZE   (12)

//---------------------------------------------------
// 지원하는 Operator 배열을 위한 자료구죠
//---------------------------------------------------

typedef struct qmoTransOperatorModule
{
    mtfModule     * module;
    UInt            transposeModuleId; // 역으로 변환했을 때의 연산자
} qmoTransOperatorModule;

//---------------------------------------------------
// Predicate의 Operand를 위한 자료구조
//---------------------------------------------------

typedef struct qmoTransOperand
{
    qtcNode          * operandFirst;
    qtcNode          * operandSecond;   // between, like escape인 경우
    
    UInt               id;         // operand Id (0~n)

    idBool             isIndexColumn;   // index column
    qcDepInfo          depInfo;
    
    qmoTransOperand  * next;
    
} qmoTransOperand;

//---------------------------------------------------
// Predicate의 Operator를 위한 자료구죠
//---------------------------------------------------

typedef struct qmoTransOperator
{
    qmoTransOperand  * left;
    qmoTransOperand  * right;
    
    UInt               id;         // operator Id

    qmoTransOperator * next;
    
} qmoTransOperator;

//---------------------------------------------------
// Transitive Predicate을 위한 자료 구조
//---------------------------------------------------

typedef struct qmoTransPredicate
{
    qmoTransOperator  * operatorList;
    qmoTransOperand   * operandList;

    qmoTransOperand   * operandSet;
    UInt                operandSetSize;

    qmoTransOperand  ** operandSetArray;   // operand set array

    qmoTransOperator  * genOperatorList;   // generated operator list
    qcDepInfo           joinDepInfo;
} qmoTransPredicate;

//---------------------------------------------------
// Transitive Predicate Matrix를 위한 자료 구조
//---------------------------------------------------

typedef struct qmoTransMatrixInfo
{
    UInt   mOperator;
    idBool mIsNewAdd;
} qmoTransMatrixInfo;

typedef struct qmoTransMatrix
{
    qmoTransPredicate  * predicate;  /* predicate */
    qmoTransMatrixInfo** matrix;     /* transitive predicate matrix */
    UInt                 size;       /* matrix (size X size) */
} qmoTransMatrix;


class qmoTransMgr
{
public:

    // Transitve Predicate 생성 함수
    static IDE_RC  processTransitivePredicate( qcStatement  * aStatement,
                                               qmsQuerySet  * aQuerySet,
                                               qtcNode      * aNode,
                                               idBool         aIsOnCondition,
                                               qtcNode     ** aTransitiveNode );

    // Transitve Predicate 생성 함수
    static IDE_RC  processTransitivePredicate( qcStatement   * aStatement,
                                               qmsQuerySet   * aQuerySet,
                                               qtcNode       * aNode,
                                               qmoPredicate  * aPredicate,
                                               qtcNode      ** aTransitiveNode );

    // qtcNode형태의 predicate을 qtcNode형태로 연결
    static IDE_RC  linkPredicate( qtcNode      * aTransitiveNode,
                                  qtcNode     ** aNode );

    // qtcNode형태의 predicate을 qmoPredicate형태로 연결
    static IDE_RC  linkPredicate( qcStatement   * aStatement,
                                  qtcNode       * aTransitiveNode,
                                  qmoPredicate ** aPredicate );

    // qmoPredicate형태의 predicate을 qmoPredicate으로 연결
    static IDE_RC  linkPredicate( qmoPredicate  * aTransitivePred,
                                  qmoPredicate ** aPredicate );

    // 논리적으로 동일한 predicate이 존재하는지 검사
    static IDE_RC  isExistEquivalentPredicate( qcStatement  * aStatement,
                                               qmoPredicate * aPredicate,
                                               qmoPredicate * aPredicateList,
                                               idBool       * aIsExist );
    
private:

    // 지원하는 12개의 연산자
    static const   qmoTransOperatorModule operatorModule[];

    // Transitive Operation Matrix
    static const   UInt operationMatrix[QMO_OPERATION_MATRIX_SIZE][QMO_OPERATION_MATRIX_SIZE];

    // 초기화 함수
    static IDE_RC  init( qcStatement        * aStatement,
                         qmoTransPredicate ** aTransPredicate );

    // qtcNode형태의 predicate으로 operator list와 operand list를 구성
    static IDE_RC  addBasePredicate( qcStatement       * aStatement,
                                     qmsQuerySet       * aQuerySet,
                                     qmoTransPredicate * aTransPredicate,
                                     qtcNode           * aNode );

    // qmoPredicate형태의 predicate으로 operator list와 operand list를 구성
    static IDE_RC  addBasePredicate( qcStatement       * aStatement,
                                     qmsQuerySet       * aQuerySet,
                                     qmoTransPredicate * aTransPredicate,
                                     qmoPredicate      * aPredicate );

    // operator list와 operand list로 transitive predicate matrix를 생성하고 계산
    static IDE_RC  processPredicate( qcStatement        * aStatement,
                                     qmsQuerySet        * aQuerySet,
                                     qmoTransPredicate  * aTransPredicate,
                                     qmoTransMatrix    ** aTransMatrix,
                                     idBool             * aIsApplicable );

    // transitive predicate matrix로 transitive predicate을 생성
    static IDE_RC  generatePredicate( qcStatement     * aStatement,
                                      qmsQuerySet     * aQuerySet,
                                      qmoTransMatrix  * aTransMatrix,
                                      qtcNode        ** aTransitiveNode );

    // predicate들로 operator list와 operand list를 생성
    static IDE_RC  createOperatorAndOperandList( qcStatement       * aStatement,
                                                 qmsQuerySet       * aQuerySet,
                                                 qmoTransPredicate * aTransPredicate,
                                                 qtcNode           * aNode,
                                                 idBool              aOnlyOneNode );

    // 하나의 predicate을 qmoTransOperator로 생성
    static IDE_RC  createOperatorAndOperand( qcStatement       * aStatement,
                                             qmoTransPredicate * aTransPredicate,
                                             qtcNode           * aCompareNode );

    // 노드가 index를 가진 컬럼노드인지 검사
    static IDE_RC  isIndexColumn( qcStatement * aStatement,
                                  qtcNode     * aNode,
                                  idBool      * aIsIndexColumn );

    // 문자 type의 컬럼이 동일 type인지 검사
    static IDE_RC  isSameCharType( qcStatement * aStatement,
                                   qtcNode     * aLeftNode,
                                   qtcNode     * aRightNode,
                                   idBool      * aIsSameType );

    // transitive predicate을 생성할 base predicate 검사
    static IDE_RC  isTransitiveForm( qcStatement * aStatement,
                                     qmsQuerySet * aQuerySet,
                                     qtcNode     * aCompareNode,
                                     idBool        aCheckNext,
                                     idBool      * aIsTransitiveForm );

    // operand list로 operand set을 생성
    static IDE_RC  createOperandSet( qcStatement       * aStatement,
                                     qmoTransPredicate * aTransPredicate,
                                     idBool            * aIsApplicable );

    // 현재노드를 참조하는 operator의 인자를 변경
    static IDE_RC  changeOperand( qmoTransPredicate * aTransPredicate,
                                  qmoTransOperand   * aFrom,
                                  qmoTransOperand   * aTo );

    // transitive predicate matrix를 생성하고 초기화
    static IDE_RC  initializeTransitiveMatrix( qmoTransMatrix * aTransMatrix );

    // transitive predicate matrix를 계산
    static IDE_RC  calculateTransitiveMatrix( qcStatement    * aStatement,
                                              qmsQuerySet    * aQuerySet,
                                              qmoTransMatrix * aTransMatrix );

    // 생성할 transitive predicate이 bad transitive predicate인지 판단
    static IDE_RC  isBadTransitivePredicate( qcStatement       * aStatement,
                                             qmsQuerySet       * aQuerySet,
                                             qmoTransPredicate * aTransPredicate,
                                             UInt                aOperandId1,
                                             UInt                aOperandId2,
                                             UInt                aOperandId3,
                                             idBool            * aIsBad );

    // qmoTransOperator를 생성
    static IDE_RC  createNewOperator( qcStatement       * aStatement,
                                      qmoTransPredicate * aTransPredicate,
                                      UInt                aLeftId,
                                      UInt                aRightId,
                                      UInt                aNewOperatorId );

    // transitive predicate를 생성
    static IDE_RC  generateTransitivePredicate( qcStatement     * aStatement,
                                                qmsQuerySet     * aQuerySet,
                                                qmoTransMatrix  * aTransMatrix,
                                                qtcNode        ** aTransitiveNode );

    // 하나의 transitive predicate를 생성
    static IDE_RC  createTransitivePredicate( qcStatement        * aStatement,
                                              qmsQuerySet        * aQuerySet,
                                              qmoTransOperator   * aOperator,
                                              qtcNode           ** aTransitivePredicate );

    // 논리적으로 동일한 predicate인지 검사
    static IDE_RC  isEquivalentPredicate( qcStatement  * aStatement,
                                          qmoPredicate * aPredicate1,
                                          qmoPredicate * aPredicate2,
                                          idBool       * aIsEquivalent );

    static IDE_RC getJoinDependency( qmsFrom   * aFrom,
                                     qcDepInfo * aDepInfo );
};

#endif /* _O_QMO_TRANSITIVITY_H_ */
