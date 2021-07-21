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
 * $Id: qmoOuterJoinOper.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     PROJ-1653 Outer Join Operator (+)
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_OUTER_JOIN_OPER_H_
#define _O_QMO_OUTER_JOIN_OPER_H_ 1


#include <qc.h>
#include <qmoCnfMgr.h>
#include <qmsParseTree.h>

//---------------------------------------------------
// Outer Join Operator 가 존재하는 형식 정의
//
// QMO_JOIN_OPER_NONE  : 초기값
// QMO_JOIN_OPER_FALSE : 테이블에 (+)가 없음.
// QMO_JOIN_OPER_BOTH  : t1.i1(+) - t1.i2 = 1 과 같이 한 테이블에 붙기도 하고 안붙기도 했음.
//                       predicate level 에서는 QMO_JOIN_OPER_BOTH 는 에러가 된다.
//---------------------------------------------------

#define QMO_JOIN_OPER_MASK   0x03
#define QMO_JOIN_OPER_NONE   0x00
#define QMO_JOIN_OPER_FALSE  0x01
#define QMO_JOIN_OPER_TRUE   0x02
#define QMO_JOIN_OPER_BOTH   0x03   // QMO_JOIN_OPER_BOTH
                                    // = QMO_JOIN_OPER_FALSE|QMO_JOIN_OPER_TRUE

//---------------------------------------------------
// Predicate 당 Outer Join Operator 가 있는 테이블은 1 개만 가능
// (List,Range Type Predicate 제외)
//---------------------------------------------------
#define QMO_JOIN_OPER_TABLE_CNT_PER_PRED              (1)

//---------------------------------------------------
// Outer Join Predicate 의 dependency table 은 2 개이하
// (List,Range Type Predicate 제외)
//---------------------------------------------------
#define QMO_JOIN_OPER_DEPENDENCY_TABLE_CNT_PER_PRED   (2)


//---------------------------------------------------
// qtcNode.depInfo.depJoinOper[i] 값을 받아서
// Outer Join Operator 의 유무를 검사
//---------------------------------------------------
# define QMO_JOIN_OPER_EXISTS( joinOper )                                   \
    (                                                                       \
       ( ( (joinOper & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_TRUE )          \
        || ( (joinOper & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_BOTH ) )      \
       ? ID_TRUE : ID_FALSE                                                 \
    )

//---------------------------------------------------
// Outer Join Operator 의 유무가 서로 반대인지 검사
//---------------------------------------------------
# define QMO_IS_JOIN_OPER_COUNTER( joinOper1, joinOper2 )                  \
    (                                                                      \
       (                                                                   \
         ( ( (joinOper1 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_TRUE )      \
          || ( (joinOper1 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_BOTH ) )  \
        && ( (joinOper2 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_FALSE )     \
       )                                                                   \
       ||                                                                  \
       (                                                                   \
         ( ( (joinOper2 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_TRUE )      \
          || ( (joinOper2 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_BOTH ) )  \
        && ( (joinOper1 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_FALSE )     \
       )                                                                   \
       ? ID_TRUE : ID_FALSE                                                \
    )

//---------------------------------------------------
// Outer Join Operator 의 유무가 서로 같은지 검사.
// QMO_JOIN_OPER_TRUE 와 QMO_JOIN_OPER_BOTH 는 둘다 Predicate 내에서
// Outer Join Operator 가 있다는 것.
//---------------------------------------------------
# define QMO_JOIN_OPER_EQUALS( joinOper1, joinOper2 )                      \
    (                                                                      \
       (                                                                   \
           ( ( (joinOper1 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_TRUE )    \
            || ( (joinOper1 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_BOTH ) )\
        && ( ( (joinOper2 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_TRUE )    \
            || ( (joinOper2 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_BOTH ) )\
       )                                                                   \
       ||                                                                  \
       (                                                                   \
           ( (joinOper1 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_FALSE )     \
        && ( (joinOper2 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_FALSE )     \
       )                                                                   \
       ||                                                                  \
       (                                                                   \
           ( (joinOper1 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_NONE )      \
        && ( (joinOper1 & QMO_JOIN_OPER_MASK) == QMO_JOIN_OPER_NONE )      \
       )                                                                   \
       ? ID_TRUE : ID_FALSE                                                \
    )

#define QMO_IS_ANY_TYPE_MODULE( module )                                   \
    (                                                                      \
        ((module) == & mtfEqualAny )                                       \
     || ((module) == & mtfGreaterEqualAny )                                \
     || ((module) == & mtfGreaterEqualAny )                                \
     || ((module) == & mtfGreaterThanAny )                                 \
     || ((module) == & mtfLessEqualAny )                                   \
     || ((module) == & mtfLessThanAny )                                    \
     || ((module) == & mtfNotEqualAny )                                    \
       ? ID_TRUE : ID_FALSE                                                \
    )

#define QMO_IS_ALL_TYPE_MODULE( module )                                   \
    (                                                                      \
        ((module) == & mtfEqual )                                          \
     || ((module) == & mtfEqualAll )                                       \
     || ((module) == & mtfGreaterEqualAll )                                \
     || ((module) == & mtfGreaterThanAll )                                 \
     || ((module) == & mtfLessEqualAll )                                   \
     || ((module) == & mtfLessThanAll )                                    \
     || ((module) == & mtfNotEqual )                                       \
     || ((module) == & mtfNotEqualAll )                                    \
       ? ID_TRUE : ID_FALSE                                                \
    )

#define QMO_SRC_IS_LIST_MODULE( module )                                   \
    (                                                                      \
        ((module) == & mtfEqual )                                          \
     || ((module) == & mtfEqualAll )                                       \
     || ((module) == & mtfEqualAny )                                       \
     || ((module) == & mtfNotEqual )                                       \
     || ((module) == & mtfNotEqualAll )                                    \
     || ((module) == & mtfNotEqualAny )                                    \
       ? ID_TRUE : ID_FALSE                                                \
    )


//---------------------------------------------------
// Outer Join Operator 를 사용하는 Join Predicate 을
// 관리하기 위한 자료구조
//---------------------------------------------------

typedef struct qmoJoinOperPred
{
    qtcNode          * node;   // OR나 비교연산자 단위의 predicate

    qmoJoinOperPred  * next;   // qmoJoinOperPred 의 연결
} qmoJoinOperPred;


//---------------------------------------------------
// Join Relation 을 관리하기 위한 자료구조
//---------------------------------------------------

typedef struct qmoJoinOperRel
{
    qtcNode          * node;   // = qmoJoinOperPred->node
    qcDepInfo          depInfo;
} qmoJoinOperRel;


//---------------------------------------------------
// Outer Join Operator 를 사용하는 Join Predicate 들을 
// 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmoJoinOperGroup
{
    qmoJoinOperPred     * joinPredicate;    // join predicate

    qmoJoinOperRel      * joinRelation;     // array
    UInt                  joinRelationCnt;
    UInt                  joinRelationCntMax;

    // join group 내의 모든 table들의 dependencies를 ORing 한 값
    qcDepInfo             depInfo;
} qmoJoinOperGroup;


//---------------------------------------------------
// Outer Join Operator 를 사용하는 자료구조를 ANSI Style 로 변경하는
// 과정에서 Classifying, Grouping, Relationing 을 수행하기 위해
// 필요한 임시 성격의 자료 구조.
// 변환이 완료된 후 Optimization 이후부터는 더 이상 이 자료구조는
// 사용되지 않는다.
//
// *** Outer Join Operator 를 변환하는 과정에서 수행하는
//     Join Grouping 과 Join Relationing 은 이후 optimization 
//     과정에서 수행하는 그것과 개념은 비슷하지만 전혀 별개이니
//     혼동하지 않기를 바란다.
//     그래서, 자료구조명과 함수명은 되도록 앞에 joinOper 라는
//     것을 붙여서 사용했고, 그 안의 변수명들은 모두 joinOper 를
//     붙일 수는 없어서 개념에 의한 용도를 중심으로 naming 했다.
//---------------------------------------------------

typedef struct qmoJoinOperTrans
{
    qtcNode       * normalCNF;   // normalized 된 CNF 를 가리킴(=qmoCNF->normalCNF)

    //-------------------------------------------------------------------
    //  transform 에 유용하도록 단순하게 설계된 아래의 자료구조를 이용한다.
    //  Predicate 분류 시 Outer Join Operator 가 있으면 oneTablePred/joinOperPred 에
    //  연결하고, 없거나 constant predicate 이면 generalPred 에 연결한다.
    //  generalPred 에 연결된 Predicate 은 transform 후에
    //  그대로 where 절(normalCNF)에 남아있게 된다.
    //  따라서, Grouping 할 대상도 아니다.
    //
    //   - oneTablePred : Outer Join Operator + one table dependency 조건의 pred list
    //   - joinOperPred : Outer Join Operator + two table 조건 이상의 pred list
    //   - generalPred  : constant predicate 포함하여 Outer Join Operator 가
    //                    없는 모든 Predicate
    //
    //   - maxJoinOperGroupCnt : 최대 joinOperGroupCnt = BaseTableCnt
    //   - joinOperGroupCnt    : 실제 joinOperGroupCnt
    //   - joinOperGroup       : joinOperGroup 배열
    //-------------------------------------------------------------------

    qmoJoinOperPred   * oneTablePred;
    qmoJoinOperPred   * joinOperPred;

    qmoJoinOperPred   * generalPred;

    UInt                maxJoinOperGroupCnt;
    UInt                joinOperGroupCnt;
    qmoJoinOperGroup  * joinOperGroup;

} qmoJoinOperTrans;


//---------------------------------------------------
// Outer Join Operator (+) 기능 관련하여 신규로 추가된 함수들을
// 모아놓은 class
//---------------------------------------------------
class qmoOuterJoinOper
{
private:

    // qmoJoinOperTrans 자료구조 초기화
    static IDE_RC    initJoinOperTrans( qmoJoinOperTrans * aTrans,
                                        qtcNode          * aNormalForm );

    // qmoJoinOperGroup 자료구조 초기화
    static IDE_RC    initJoinOperGroup( qcStatement      * aStatement,
                                        qmoJoinOperGroup * aJoinGroup,
                                        UInt               aJoinGroupCnt );

    // LIST Type 의 비교연산자를 일반연산자로 변환 시 변환할 일반연산자 string return
    static IDE_RC    getCompModule4List( mtfModule    * aModule,
                                         mtfModule   ** aCompModule );

    // avaliable 한 Join Relation 인지 검사
    static IDE_RC    isAvailableJoinOperRel( qcDepInfo  * aDepInfo,
                                             idBool     * aIsTrue );

    // make new OR Node Tree
    static IDE_RC    makeNewORNodeTree( qcStatement  * aStatement,
                                        qmsQuerySet  * aQuerySet,
                                        qtcNode      * aInNode,
                                        qtcNode      * aOldSrc,
                                        qtcNode      * aOldTrg,
                                        qtcNode     ** aNewSrc,
                                        qtcNode     ** aNewTrg,
                                        mtfModule    * aCompModule,
                                        qtcNode     ** aORNode,
                                        idBool         aEstimate );

    // make new Operand Node Tree
    static IDE_RC    makeNewOperNodeTree( qcStatement  * aStatement,
                                          qtcNode      * aInNode,
                                          qtcNode      * aOldSrc,
                                          qtcNode      * aOldTrg,
                                          qtcNode     ** aNewSrc,
                                          qtcNode     ** aNewTrg,
                                          mtfModule    * aCompModule,
                                          qtcNode     ** aOperNode );

    // normalize 된 Tree 에서 LIST/RANGE Type 의 특수비교연산자 Predicate 들을
    // 일반 비교연산자 형태로 변환
    static IDE_RC    transNormalCNF4SpecialPred( qcStatement       * aStatement,
                                                 qmsQuerySet       * aQuerySet,
                                                 qmoJoinOperTrans  * aCNF );

    // 변환된 normalCNF Tree 의 Predicate 들을 순회하며 제약사항 검사
    static IDE_RC    validateJoinOperConstraints( qcStatement * aStatement,
                                                  qmsQuerySet * aQuerySet,
                                                  qtcNode     * aNormalCNF );

    // normalCNF 의 Join 관계를 고려하여 From Tree 와 normalCNF Tree 변환
    static IDE_RC    transformStruct2ANSIStyle( qcStatement       * aStatement,
                                                qmsQuerySet       * aQuerySet,
                                                qmoJoinOperTrans  * aTrans );


    // constant predicate 인지 검사
    static IDE_RC    isConstJoinOperPred( qmsQuerySet * aQuerySet,
                                          qtcNode     * aPredicate,
                                          idBool      * aIsTrue );

    // one table predicate 인지 검사
    static IDE_RC    isOneTableJoinOperPred( qmsQuerySet * aQuerySet,
                                             qtcNode     * aPredicate,
                                             idBool      * aIsTrue );

    // qmoJoinOperPred 자료구조 생성
    static IDE_RC    createJoinOperPred( qcStatement      * aStatement,
                                         qtcNode          * aNode,
                                         qmoJoinOperPred ** aNewPredicate );

    // constant/onetable/join predicate 분류
    static IDE_RC    classifyJoinOperPredicate( qcStatement       * aStatement,
                                                qmsQuerySet       * aQuerySet,
                                                qmoJoinOperTrans  * aTrans );

    // Join Predicate 을 Grouping, Relationing, Ordering
    static IDE_RC    joinOperOrdering( qcStatement       * aStatement,
                                       qmoJoinOperTrans  * aTrans );

    // Join Predicate 을 Grouping
    static IDE_RC    joinOperGrouping( qmoJoinOperTrans  * aTrans );

    // Join Predicate 을 Relationing
    static IDE_RC    joinOperRelationing( qcStatement      * aStatement,
                                          qmoJoinOperGroup * aJoinGroup );

    // Outer Join Operator 가  포함된 Join Predicate 들을 ANSI 형태로 변환
    static IDE_RC    transformJoinOper( qcStatement      * aStatement,
                                        qmsQuerySet      * aQuerySet,
                                        qmoJoinOperTrans * aTrans );

    // Outer Join Operator 관련 From 절의 변환
    static IDE_RC    transformJoinOper2From( qcStatement      * aStatement,
                                             qmsQuerySet      * aQuerySet,
                                             qmoJoinOperTrans * aTrans,
                                             qmsFrom          * sNewFrom,
                                             qmoJoinOperRel   * sJoinRel,
                                             qmoJoinOperPred  * sJoinPred );

    // Outer Join Operator 관련 onCondition 절의 변환
    static IDE_RC    movePred2OnCondition( qcStatement      * aStatement,
                                           qmoJoinOperTrans * aTrans,
                                           qmsFrom          * aNewFrom,
                                           qmoJoinOperRel   * aJoinGroupRel,
                                           qmoJoinOperPred  * aJoinGroupPred );

    // From Tree 를 순회하며 New From 노드를 Merge
    static IDE_RC    merge2ANSIStyleFromTree( qmsFrom     * aFromFrom,
                                              qmsFrom     * aToFrom,
                                              idBool        aFirstIsLeft,
                                              qcDepInfo   * aDepInfo,
                                              qmsFrom     * aNewFrom );

    // normalCNF 에서 RANGE Type 의 Predicate 경우 변환 수행
    static IDE_RC    transJoinOperBetweenStyle( qcStatement * aStatement,
                                                qmsQuerySet * aQuerySet,
                                                qtcNode     * aInNode,
                                                qtcNode    ** aTransStart,
                                                qtcNode    ** aTransEnd );

    // normalCNF 에서 LIST Type 의 Predicate 경우 변환 수행
    static IDE_RC    transJoinOperListArgStyle( qcStatement * aStatement,
                                                qmsQuerySet * aQuerySet,
                                                qtcNode     * aInNode,
                                                qtcNode    ** aTransStart,
                                                qtcNode    ** aTransEnd );
 
public:

    static IDE_RC    transform2ANSIJoin( qcStatement  * aStatement,
                                         qmsQuerySet  * aQuerySet,
                                         qtcNode     ** aNormalForm );
};

#endif /* _O_QMO_OUTER_JOIN_OPER_H_ */
