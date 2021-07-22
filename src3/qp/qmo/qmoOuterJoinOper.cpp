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
 * $Id$
 *
 * Description :
 *     PROJ-1653 Outer Join Operator (+)
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qcuSqlSourceInfo.h>
#include <qmsParseTree.h>
#include <qmo.h>
#include <qtc.h>
#include <qmoOuterJoinOper.h>

extern mtfModule mtfAnd;
extern mtfModule mtfOr;
extern mtfModule mtfEqual;
extern mtfModule mtfList;
extern mtfModule mtfBetween;
extern mtfModule mtfNotBetween;
extern mtfModule mtfEqualAll;
extern mtfModule mtfEqualAny;
extern mtfModule mtfGreaterEqualAll;
extern mtfModule mtfGreaterEqualAny;
extern mtfModule mtfGreaterThanAll;
extern mtfModule mtfGreaterThanAny;
extern mtfModule mtfLessEqualAll;
extern mtfModule mtfLessEqualAny;
extern mtfModule mtfLessThanAll;
extern mtfModule mtfLessThanAny;
extern mtfModule mtfNotEqual;
extern mtfModule mtfNotEqualAll;
extern mtfModule mtfNotEqualAny;
extern mtdModule mtdList;

extern mtfModule mtfGreaterThan;
extern mtfModule mtfLessThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfLessEqual;

/***********************************************************************
 *  ** 참고 사항
 *
 *  Outer Join Operator 에 대한 오라클과 알티베이스의 제약사항 차이점
 *
 *  1. 오라클은 "(", "+", ")" 문자 사이에 white space 가 올 수 있다.
 *     알티베이스는 안된다. 세문자가 하나의 토큰이다.
 *
 *     가. 오라클
 *         where t1.i1( + ) = 1 -- (O)
 *         where t1.i1(
 *                   + ) = 1 -- (O)
 *     나. 알티베이스
 *         where t1.i1(+) = 1 -- (O)
 *         where t1.i1( +) = 1 -- (X)
 *
 *  2. 오라클의 경우는 equal 연산자에서 list argument 를 사용할 수 없다.
 *     알티베이스는 가능하다.
 *
 *     가. 오라클
 *         where (t1.i1(+),2) = (1,2) -- (X)
 *         where (t1.i1(+),2) = (t2.i1,2) -- (X)
 *     나. 알티베이스
 *         where (t1.i1(+),2) = (1,2) -- (O)
 *         where (t1.i1(+),2) = (t2.i1,2) -- (O)
 *
 *  3. 오라클의 경우는 order by, group by, on 절 등에 outer join operator 가 사용할 수 없다.
 *     알티베이스의 경우는 있어도 의미없는 경우는 굳이 에러처리를 하지 않는다.
 *
 *  4. 오라클의 경우 subquery 와 outer join 될 수 없다. (하지만, rule 이 명확하지 않다.)
 *     알티베이는 특수노드의 변환이 발생해야할 경우에 predicate 에 subquery 가 있으면 안된다.
 *     (즉, Between/List Arg Style 의 Predicate 에서, (+)가 존재하고
 *     dependency table 이 2 개 이상일 때 subquery 와 함께 사용되면 에러이다.)
 *
 *     가. 오라클
 *         where t1.i1(+) = (select 1 from dual) -- (X)
 *         where t1.i1(+) - (select 1 from dual) = 0 -- (O)
 *         where t1.i1=t2.i1(+) and t1.i1(+)=(select 1 from dual) -- (X)
 *         where ((select 1 from dual),2)=((t1.i1(+),t2.i1)) -- (O)
 *         where t1.i1(+)-(select 1 from dual)=0 or t1.i1(+)=2 -- (X)
 *     나. 알티베이스
 *         where t1.i1(+) = (select 1 from dual) -- (O)
 *         where t1.i1(+) - (select 1 from dual) = 0 -- (O)
 *         where t1.i1=t2.i1(+) and t1.i1(+)=(select 1 from dual) -- (O)
 *         where ((select 1 from dual),2)=((t1.i1(+),t2.i1)) -- (X)
 *         where t1.i1(+)-(select 1 from dual)=0 or t1.i1(+)=2 -- (O)
 ***********************************************************************/


IDE_RC
qmoOuterJoinOper::transform2ANSIJoin( qcStatement  * aStatement,
                                      qmsQuerySet  * aQuerySet,
                                      qtcNode     ** aNormalForm )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : (+) 가 있는 normalCNF 를 ANSI Join 형태로 변환
 *
 * Implementation :
 *
 *   아래의 세가지 과정을 통해
 *   Outer Join Operator(+) 가 있는 normalCNF 를 기존의 ANSI Outer
 *   Join 을 사용했을 때와 같은 자료구조 형태로 변환한다.
 *   즉, From Tree 를 Left Outer Join 노드를 사용하는 형태로
 *   변형하고, 그에 따르는 onCondition 노드들을 기존의 대응되는
 *   normalCNF Predicate 에서 분리하여 옮겨주는 등의 작업이다.
 *   그 이후의 Optimization 작업들은 이러한 변환된 From,
 *   normalCNF를 이용하여 수행하게 된다.
 *
 *   1. sNormalForm 의 노드 중에서 between/any,all,in 과 같은
 *      비교연산자는 단순 비교 연산자노드로 변환해야 한다.
 *      그렇지 않으면, 아래와 같이 다수 테이블이 얽힌
 *      Join Predicate 의 경우 onCondition 절로 변환하기가 어렵다.
 *
 *      (t1.i1(+),t2.i1(+),1) in ((t2.i1,t3.i1,t3.i1(+)))
 *
 *   2. Normalize 된 Tree 의 predicate 을 traverse 하면서
 *      Outer Join Operator 에 관련된 각종 제약사항을 검사한다.
 *      terminal node를 위한 제약사항은 qtc::estimateInternal
 *      함수에서 전체 node traverse 시에 걸러낸다.
 *      predicate 관계에 따른 제약사항은 transformStruct2ANSIStyle
 *      단계에서 검사한다.
 *
 *   3. normalized 된 where 절의 Outer Join Operator가 붙은 조건들을
 *      ANSI 형태의 From, normalCNF 절로 Transform
 *
 *  ** 참고
 *   위와 같은 변환 과정에서 join grouping, join relationing 이라는 용어가
 *   등장하는데, 이는 이후 optimization 단계에서 수행하는 그것과는
 *   별개이다.
 *   혼동을 피하기 위해 되도록 함수명 등은 joinOperGrouping, joinOperRelationing 과
 *   같이 joinOper 를 붙여서 사용하였다.
 *   주석에서조차 joinOper 를 붙여서 설명하는 것은 어색하기 때문에
 *   join grouping, relationing 등을 그대로 사용하였다.
 ***********************************************************************/

    qtcNode            * sNormalForm = NULL;
    qmoJoinOperTrans   * sTrans      = NULL;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::transform2ANSIJoin::__FT__" );

    IDE_FT_ASSERT( aStatement   != NULL );
    IDE_FT_ASSERT( aQuerySet    != NULL );
    IDE_FT_ASSERT( aNormalForm  != NULL );

    sNormalForm = (qtcNode *)*aNormalForm;

    // BUG-40042 oracle outer join property
    IDE_TEST_RAISE( QCU_OUTER_JOIN_OPER_TO_ANSI_JOIN == 0,
                    ERR_NOT_ALLOWED_OUTER_JOIN_OPER );

    // --------------------------------------------------------------
    // 변환에 필요한 qmoJoinOperTrans 구조체를 할당 및 초기화
    // --------------------------------------------------------------

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinOperTrans ),
                                             (void**) & sTrans )
              != IDE_SUCCESS );

    IDE_TEST( initJoinOperTrans( sTrans, sNormalForm )
              != IDE_SUCCESS );

    // --------------------------------------------------------------
    // in/between 같은 노드를 단순 Predicate 노드로 변환
    // --------------------------------------------------------------

    IDE_TEST( transNormalCNF4SpecialPred( aStatement,
                                          aQuerySet,
                                          sTrans )
              != IDE_SUCCESS );

    // --------------------------------------------------------------
    // Predicate 별로 제약사항 검사
    // --------------------------------------------------------------

    IDE_TEST( validateJoinOperConstraints( aStatement,
                                           aQuerySet,
                                           sTrans->normalCNF )
              != IDE_SUCCESS );

    // --------------------------------------------------------------
    // ANSI 형태로 자료 구조 변환 (From, normalCNF)
    // --------------------------------------------------------------

    IDE_TEST( transformStruct2ANSIStyle( aStatement,
                                         aQuerySet,
                                         sTrans )
              != IDE_SUCCESS );

    *aNormalForm = sTrans->normalCNF;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOWED_OUTER_JOIN_OPER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMO_NOT_ALLOWED_OUTER_JOIN_OPER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::initJoinOperTrans( qmoJoinOperTrans * aTrans,
                                     qtcNode          * aNormalForm )
{
/***********************************************************************
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : qmoJoinOperTrans 자료구조 초기화
 *
 * Implementation :
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::initJoinOperTrans::__FT__" );

    IDE_FT_ASSERT( aNormalForm   != NULL );

    aTrans->normalCNF      = aNormalForm;

    aTrans->oneTablePred   = NULL;
    aTrans->joinOperPred   = NULL;
    aTrans->generalPred    = NULL;
    aTrans->maxJoinOperGroupCnt = 0;
    aTrans->joinOperGroupCnt = 0;
    aTrans->joinOperGroup  = NULL;

    return IDE_SUCCESS;
}


IDE_RC
qmoOuterJoinOper::transNormalCNF4SpecialPred( qcStatement       * aStatement,
                                              qmsQuerySet       * aQuerySet,
                                              qmoJoinOperTrans  * aTrans )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : 특수비교 노드를 일반 비교 Predicate 노드로 변환
 *
 * Implementation :
 *    Outer Join Operator 를 사용하는 Predicate 들 중
 *
 *     - Between, NotBetween 과 같이 범위를 비교하는
 *       Between Style 의 비교연산자와,
 *     - Equal,EqualAny,EqualAll,NotEqual 과 같이 List Argument 를
 *       가지는 Style 의 비교연산자는
 *
 *    다수의 테이블에 dependency 를 가질 수 있고, Outer Join Operator 또한
 *    복잡한 형태로 등장할 수 있다.
 *
 *    즉, (=,>,<,...) 등과 같은 일반 비교연산자는 하나의 Predicate 에
 *    단 하나의 Outer Join Operator 와 두개 이하의 dependency table 밖에
 *    사용할 수 없다는 명확한 규칙이 존재한다.
 *    그러나, 위의 두가지 Style 의 비교연산자는 그러한 규칙을
 *    적용할 수 없는 복잡한 사용형태를 가질 수 있다.
 *
 *    이런 경우는 (+) Join 구문을 ANSI 형태로 transform 과정에서
 *    해당 Predicate 을 onCondition 절로 옮길 때 Predicate 을
 *    연산자 종류별로 예외처리 과정을 통해 모두 분류하여 각각에 맞는
 *    LEFT OUTER JOIN 절에 붙여줘야 한다.
 *
 *    예를 들어 아래의 예를 살펴보자.
 *    아래의 구문은 그대로 onCondition 절에 연결할 수 없다.
 *    dependency table 이 t1,t2,t3 세개가 존재하기 때문에,
 *    두개의 테이블간의 Join 관계를 표현하는 on 절에 올 수가 없는 것이다.
 *    그러므로, Predicate 을 여러개로 분리하지않는 한 변환이 불가능하다.
 *
 *      (t1.i1(+),t2.i1(+),1) in ((t2.i1,t3.i1,t3.i1(+)))
 *
 *    다음과 같이 변환한 후 Predicate 별로 각각에 맞는 OUTER JOIN 노드의
 *    onConditio 에 연결해줘야 한다.
 *
 *         t1.i1(+) = t2.i1
 *     and t2.i1(+) = t3.i1
 *     and 1 = t3.i1(+)
 *
 *    이를 transform 과정에서 수행하면 지나치게 복잡한 로직이 될 수 있다.
 *    불가능한건 아니지만 지나치게 복잡한 것이 문제다.
 *    위의 구문은 in 절을 예로 들었지만, any, all 연산자가 부등호와 함께
 *    사용되면 더욱 복잡해진다.
 *
 *    따라서, 제약사항 검사 및 transform 과정에서 모든 Join Predicate 이
 *    단순비교 연산자 형태라고 믿고 처리할 수 있도록 사전에 1 단계의
 *    변환작업을 수행할 필요가 있다.
 *    그것이 이 함수에서 하는 일이다.
 *
 *    이러한 변환작업을 추가로 수행하면, 다음과 같은 조건일 때 Predicate
 *    변환에 따라 노드수가 약간 늘어나므로 메모리 사용량 증가와 성능저하가
 *    있을 수 있다.
 *    하지만, 아래와 같이 사용되는 예는 거의 전무할 것으로 예상된다.
 *
 *    ( 2개 이상의 dependency table + 1 개 이상의 Outer Join Operator
 *                                  + 다수의 list argument )
 *
 *
 *    이 변환에서 가장 우려되는 것은 LIST Argument Style 의 Predicate 을
 *    풀어서 변환할 때 신규로 생성되는 노드가 엄청나게 많아지지 않을까이다.
 *    그러나, 사용자는 대부분 아래의 (1) 형태로 사용할 뿐
 *    (2),(3) 과 같이 사용하지는 않는다.
 *    (+)를 고려한 결과를 예측하여 쿼리를 만들어내기도 불가능할 뿐더러,
 *    (2),(3) 과 같은 구문은 O 사에서도 지원하지도 않는다.
 *
 *    (1) 의 경우는 이 함수에서 변환해줄 필요가 없으므로 우려하지 않아도 된다.
 *    왜냐하면, (1) 과 같은 형태는 Join Predicate 이 아니므로
 *    추후 transform 과정에서 대응되는 onCondition 에 그냥 연결만 해주면
 *    되기 때문이다.
 *
 *    (1) t1.i1(+) in (1,2,3,4)
 *    (2) (t1.i1(+),t2.i1) in ((t3.i1,t4.i1(+)),(t2.i1,t1.i2(+)))
 *    (3) t1.i1 in (t2.i1(+),1,2,3,4,5,6,7,8,...)
 *
 *
 *
 *    변환과정을 살펴보자. ((+) 기호는 생략했다.)
 *
 *    (AND)
 *      |
 *      V
 *    (OR)    --->    (OR)    ---------->    (OR)    --> ...
 *      |               |                      |
 *      V               V                      V
 *   (Equal)       (EqualAny)              (Between)
 *      |               |                      |
 *      V               V                      V
 *    (a1) -> (a2)   (list)   ->   (list)    (c1) -> (c2) -> (c3)
 *                      |            |
 *                      V            V
 *                    (a1) -> (a2) (list)
 *                                   |
 *                                   V
 *                                 (b1) -> (b2)
 *
 *
 *    위와 같은 CNF Tree 는 다음과 같은 쿼리를 사용할 때 생길 것이다.
 *
 *    where a1 = a2
 *      and (a1,a2) in ((b1,b2))
 *      and c1 between c2 and c3
 *
 *
 *    위의 구문을 아래와 같은 형태로 변환해야 한다.
 *
 *    where a1 = a2
 *      and (a1 = b1  and a2 = b2)
 *      and (c1 >= c2 and c1 <= c3)
 *
 *    이를 CNF Tree 에 반영하면,
 *
 *    (AND)
 *      |
 *      V
 *    (OR)   --->   (OR)   --->   (OR)  ----->  (OR)   -->   (OR)
 *      |             |             |             |            |
 *      V             V             V             V            V
 *   (Equal)       (Equal)       (Equal)    (GreaterEqual) (LessEqual)
 *      |             |             |             |            |
 *      V             V             V             V            V
 *    (a1) -> (a2)  (a1) -> (b1)  (a2) -> (b2)  (c1) -> (c2) (c1) -> (c3)
 *
 *
 *    위에서는 개념적으로 기존 특수노드의 자리에 대체하는 형태로
 *    설명했지만, 실제 구현 시에는 노드 리스트의 맨앞에
 *    연결해주면 된다.
 *
 *    만약 a1 in (b1,2,3) 과 같은 Predicate 은 다음과 같이 변환될 것이다.
 *    OR 의 argument 로 모두 연결되었다는 것이 위와 다른 점이다.
 *
 *    (AND)
 *      |
 *      V
 *    (OR)
 *      |
 *      V
 *   (Equal)   -->  (Equal)   -->   (Equal)
 *      |              |               |
 *      V              V               V
 *    (a1) -> (b1)    (a1) -> (2)     (a1) -> (3)
 *
 *
 *
 *    변환이 필요한 경우와 불필요한 경우의 분류
 *    (아래 언급된 특수비교연산자는 Between / LIST Argument Style 의
 *     비교 연산자 타입을 말한다.)
 *
 *    1. Predicate 변환 불필요
 *
 *       가. (+)를 사용하지 않는 모든 경우
 *       나. (+)가 1개이면서 dependency table 이 2개 미만인 모든 경우
 *       다. (+)와 dependency table 갯수에 상관없이 일반연산자인 경우
 *
 *    2. Predicate 변환 필요 (특수비교연산자를 사용할 경우만 해당)
 *
 *       가. (+) 가 2개 이상인 경우
 *       나. (+)가 1개이면서 dependency table 이 2개를 초과할 경우
 ***********************************************************************/

    qcuSqlSourceInfo   sqlInfo;
    qtcNode          * sSrcNode        = NULL;
    qtcNode          * sTrgNode        = NULL;
    qtcNode          * sCurNode        = NULL;
    qtcNode          * sTransStart     = NULL;
    qtcNode          * sTransEnd       = NULL;

    qtcNode          * sORStart        = NULL;
    qtcNode          * sCurOR          = NULL;
    qtcNode          * sPrevOR         = NULL;

    qtcNode          * sPredPrev       = NULL;
    qtcNode          * sPredCur        = NULL;

    qtcNode          * sTmpTransStart  = NULL;
    qtcNode          * sTmpTransEnd    = NULL;

    idBool             sIsTransFirst = ID_FALSE;
    idBool             sIsSpecialOperand = ID_FALSE;
    idBool             sIsTransformedOR = ID_FALSE;
    UInt               sJoinOperCount;
    UInt               sTargetListCount;
    UInt               sDepCount;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::transNormalCNF4SpecialPred::__FT__" );

    IDE_FT_ASSERT( aStatement   != NULL );
    IDE_FT_ASSERT( aQuerySet    != NULL );
    IDE_FT_ASSERT( aTrans       != NULL );


    sORStart = (qtcNode *)aTrans->normalCNF->node.arguments;

    for ( sCurOR  = sORStart;
          sCurOR != NULL;
          sCurOR  = (qtcNode *)sCurOR->node.next )
    {
        sIsTransformedOR = ID_FALSE;

        for ( sPredCur  = (qtcNode *)sCurOR->node.arguments;
              sPredCur != NULL;
              sPredPrev = sPredCur,
              sPredCur  = (qtcNode *)sPredCur->node.next )
        {
            //----------------------------------------------------
            //    Between / LIST Argument Style 비교연산자 종류를 살펴보자.
            //
            //    ex> t1.i1 between 1 and 10
            //          ^           ^      ^
            //          |           |      |
            //       source      target  target   (source 1 개, target 2 개)
            //
            //        t1.i1 in (1,2)
            //          ^        ^
            //          |        |
            //       source    target             (source 1 개, target list)
            //
            //       (t1.i1,t2.i1) in ((1,2))
            //             ^             ^
            //             |             |
            //          source        target      (source list, target list)
            //
            //
            //    BETWEEN Style
            //    ======================================================
            //    mtfModule          : mtcName       : source  : target
            //    ======================================================
            //    mtfBetween         : BETWEEN       :   1     :   2 (not list)
            //    mtfNotBetween      : NOT BETWEEN   :   1     :   2 (not list)
            //    ======================================================
            //
            //
            //    LIST Argument Style
            //    ======================================================
            //    mtfModule          : mtcName       : source  : target
            //    ======================================================
            //    mtfEqual           : =             : list    :  list
            //    mtfEqualAll        : =ALL          : list    :  list
            //                                       :   1     :  list
            //    mtfEqualAny        : =ANY          : list    :  list
            //                                       :   1     :  list
            //    mtfGreaterEqualAll : >=ALL         :   1     :  list
            //    mtfGreaterEqualAny : >=ANY         :   1     :  list
            //    mtfGreaterThanAll  : >ALL          :   1     :  list
            //    mtfGreaterThanAny  : >ANY          :   1     :  list
            //    mtfLessEqualAll    : <=ALL         :   1     :  list
            //    mtfLessEqualAny    : <=ANY         :   1     :  list
            //    mtfLessThanAll     : <ALL          :   1     :  list
            //    mtfLessThanAny     : <ANY          :   1     :  list
            //    mtfNotEqual        : <>            : list    :  list
            //    mtfNotEqualAll     : <>ALL         : list    :  list
            //    mtfNotEqualAny     : <>ANY         : list    :  list
            //                                       :   1     :  list
            //    ======================================================
            //
            //
            //    위의 다양한 연산 중에서 source 가 list 노드일 수 있는
            //    다음의 연산들은 (+)와 함께 사용할 때 제약사항을 두도록 한다.
            //    이는 사용자가 그렇게 사용하지도 않을 뿐더러, 불필요한 변환의
            //    어려움을 덜기 위해서이다.
            //
            //    mtfEqual           : =             : list    :  list
            //    mtfEqualAll        : =ALL          : list    :  list
            //    mtfEqualAny        : =ANY          : list    :  list
            //    mtfNotEqual        : <>            : list    :  list
            //    mtfNotEqualAll     : <>ALL         : list    :  list
            //    mtfNotEqualAny     : <>ANY         : list    :  list
            //
            //    ==> source 가 list 이면서 (+) 가 하나이상 사용되었고
            //        dependency table 이 2 개 이상 사용되었을 경우,
            //        target 의 list 는 argument 가 2 개 이상 사용될 수 없다.
            //        (mtfEqual 의 경우는 원래 target list 가 2 개 이상인
            //        구문 자체를 지원하지 않는다.)
            //
            //
            //
            //   위의 몇가지 연산자 별로 변환규칙을 만들어보자.
            //
            //   기본 규칙을 보면,
            //   sPredCur 를 변환한 후 list head 와 tale 을 sTransStart 와
            //   sTransEnd 가 가리키도록 한다.
            //   이 sTransStart 와 sTransEnd 를 기존의 normalCNF 에
            //   끼워넣고, 기존의 sPredCur 는 제거한다.
            //
            //
            //   (1) mtfBetween
            //
            //       ** 변환 전
            //       a1 between a2 and a3
            //
            //        (AND)
            //          |
            //          V
            //        (OR)  <- sCurOR
            //          |
            //          V
            //      (Between) <- sPredCur
            //          |
            //          V
            //        (a1) -> (a2) -> (a3)
            //
            //       ** 변환 후
            //       a1 >= a2 and a1 <= a3
            //
            //        (AND)
            //          |  _______ sTransStart ____ sTransEnd
            //          V /                   /
            //        (OR)       ---->    (OR)
            //          |                   |
            //          V                   V
            //     (GreaterEqual)      (LessEqual)
            //          |                   |
            //          V                   V
            //        (a1) -> (a2)        (a1) -> (a3)
            //
            //
            //   (2) mtfNotBetween
            //
            //       ** 변환 전
            //       a1 not between a2 and a3
            //
            //        (AND)
            //          |
            //          V
            //        (OR)
            //          |
            //          V
            //     (NotBetween)
            //          |
            //          V
            //        (a1) -> (a2) -> (a3)
            //
            //       ** 변환 후
            //       a1 < a2 and a1 > a3
            //
            //        (AND)
            //          |
            //          V
            //        (OR)
            //          |
            //          V
            //     (LessThan)   -->   (GreaterThan)
            //          |                   |
            //          V                   V
            //        (a1) -> (a2)        (a1) -> (a3)
            //
            //
            //   (3) mtfEqualAll
            //
            //       ** 변환 전
            //       (a1,a2) =ALL ((b1,b2))
            //
            //        (AND)
            //          |
            //          V
            //        (OR)
            //          |
            //          V
            //     (EqualAll)
            //          |
            //          V
            //       (list)   --->  (list)
            //          |              |
            //          V              V
            //        (a1) -> (a2)  (list) <= can't be more than 1 list
            //                         |
            //                         V
            //                       (b1) -> (b2)
            //
            //       ** 변환 후
            //       a1 = b1 and a2 = b2
            //
            //        (AND)
            //          |
            //          V
            //        (OR)    --->    (OR)
            //          |               |
            //          V               V
            //       (Equal)         (Equal)
            //          |               |
            //          V               V
            //        (a1) -> (a2)    (b1) -> (b2)
            //
            //
            //   (4) mtfEqualAny
            //
            //       ** 변환 전
            //       (a1,a2) =ANY ((b1,b2))
            //
            //        (AND)
            //          |
            //          V
            //        (OR)
            //          |
            //          V
            //     (EqualAny)
            //          |
            //          V
            //       (list)   --->  (list)
            //          |              |
            //          V              V
            //        (a1) -> (a2)  (list) <= can't be more than 1 list
            //                         |
            //                         V
            //                       (b1) -> (b2)
            //
            //       ** 변환 후
            //       a1 = b1 and a2 = b2
            //
            //        (AND)
            //          |
            //          V
            //        (OR)    --->    (OR)
            //          |               |
            //          V               V
            //       (Equal)         (Equal)
            //          |               |
            //          V               V
            //        (a1) -> (a2)    (b1) -> (b2)
            //
            //
            //   나머지는 각 함수에서...
            //----------------------------------------------------

            //---------------------------------------------
            // Outer Join Operator 가 있을 때만 변환을 생각하면 됨.
            //---------------------------------------------

            if ( ( sPredCur->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                    == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                //---------------------------------------------
                // Between / LIST Argument Style 비교연산자인지 검사
                //
                // MTC_NODE_GROUP_COMPARISON_TRUE
                //   - mtfEqualAll
                //   - mtfEqualAny
                //   - mtfGreaterEqualAll
                //   - mtfGreaterEqualAny
                //   - mtfGreaterThanAll
                //   - mtfGreaterThanAny
                //   - mtfLessEqualAll
                //   - mtfLessEqualAny
                //   - mtfLessThanAll
                //   - mtfLessThanAny
                //   - mtfNotEqualAll
                //   - mtfNotEqualAny
                //---------------------------------------------

                if ( ( sPredCur->node.module == & mtfBetween )
                  || ( sPredCur->node.module == & mtfNotBetween )
                  || ( sPredCur->node.module == & mtfEqual )
                  || ( sPredCur->node.module == & mtfNotEqual )
                  || ( ( sPredCur->node.module->lflag & MTC_NODE_GROUP_COMPARISON_MASK )
                         == MTC_NODE_GROUP_COMPARISON_TRUE ) )
                {
                    sIsSpecialOperand = ID_TRUE;
                }
                else
                {
                    sIsSpecialOperand = ID_FALSE;;
                }


                //---------------------------------------------
                // 특수연산자(Between / LIST Argument Style 비교연산자)인 경우
                //---------------------------------------------

                if ( sIsSpecialOperand == ID_TRUE )
                {
                    sTargetListCount = 0;

                    sSrcNode = (qtcNode *)sPredCur->node.arguments;
                    sTrgNode = (qtcNode *)sPredCur->node.arguments->next;

                    //---------------------------------------------
                    // Source 가 list 일 수 있는 연산자들
                    //---------------------------------------------

                    if ( QMO_SRC_IS_LIST_MODULE( sPredCur->node.module ) == ID_TRUE )
                    {
                        //---------------------------------------------
                        // mtfEqual,mtfNotEqual 연산자는 Source 가 list 일 때
                        // 외에는 변환해줄 일이 없으므로 그냥 continue 한다.
                        // (t1.i1,t2.i1(+)) = (1,2) 와 같이 사용될 때는
                        // 변환해주어야 한다.
                        //---------------------------------------------

                        if ( ( ( sPredCur->node.module == & mtfEqual )
                            || ( sPredCur->node.module == & mtfNotEqual ) )
                          && ( sSrcNode->node.module != & mtfList ) )
                        {
                            continue;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    //---------------------------------------------
                    // Source 가 list 일 수 없는 연산자들
                    //---------------------------------------------
                    else
                    {
                        //---------------------------------------------
                        // 나머지 특수 비교연산자들은 Source 쪽에
                        // list 가 올 수 없다.
                        //---------------------------------------------

                        sSrcNode = (qtcNode *)sPredCur->node.arguments;

                        if ( sSrcNode->node.module == & mtfList )
                        {
                            sqlInfo.setSourceInfo( aStatement,
                                                   & sPredCur->position );
                            IDE_RAISE( ERR_INVALID_OPERATION_WITH_LISTS );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }


                    //---------------------------------------------
                    // dependency table & Outer Join Operator 의 갯수 검사
                    //---------------------------------------------

                    sJoinOperCount = qtc::getCountJoinOperator( & sPredCur->depInfo );
                    sDepCount      = qtc::getCountBitSet( & sPredCur->depInfo );

                    if ( ( sJoinOperCount == QMO_JOIN_OPER_TABLE_CNT_PER_PRED )
                      && ( sDepCount < QMO_JOIN_OPER_DEPENDENCY_TABLE_CNT_PER_PRED ) )
                    {
                        //---------------------------------------------
                        // Outer Join Operator 의 갯수가 1 개이고,
                        // dependency table 갯수가 2 개 미만이면
                        // 변환이 불필요하다.
                        //---------------------------------------------

                        continue;
                    }
                    else
                    {
                        //---------------------------------------------
                        // Source 가 list 이면서 (+) 를 사용하고,
                        // dependency table 이 2 개 이상일 때는
                        // Target list 가 2 개 이상이 올 수 없다는 제약사항을 둔다.
                        // (mtfEqual,mtfNotEqual 은 제외)
                        //---------------------------------------------
                        if ( ( sPredCur->node.module != & mtfEqual )
                          && ( sPredCur->node.module != & mtfNotEqual ) )
                        {
                            if ( sSrcNode->node.module == & mtfList )
                            {
                                for ( sCurNode  = (qtcNode *)sTrgNode->node.arguments;
                                      sCurNode != NULL;
                                      sCurNode  = (qtcNode *)sCurNode->node.next )
                                {
                                    sTargetListCount++;
                                }

                                if ( sTargetListCount > 1 )
                                {
                                    sqlInfo.setSourceInfo( aStatement,
                                                           & sPredCur->position );
                                    IDE_RAISE( ERR_INVALID_OPERATION_WITH_LISTS );
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }

                        //---------------------------------------------
                        // 변환 대상 Predicate 에서 Subquery 를 사용하면
                        // 안되는 것으로 제약사항을 둔다.
                        //---------------------------------------------
                        if ( ( sPredCur->lflag & QTC_NODE_SUBQUERY_MASK )
                               == QTC_NODE_SUBQUERY_EXIST )
                        {
                            sqlInfo.setSourceInfo( aStatement,
                                                   & sPredCur->position );
                            IDE_RAISE(ERR_OUTER_JOINED_TO_SUBQUERY);
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        //---------------------------------------------
                        // Style 별로 변환 작업 수행
                        //
                        // sPredCur 를 전달하여 sPredCur 를 변환한다.
                        // sPredCur 를 변환한 결과는 sTransStart 와 sTransEnd 에
                        // 담아서 return 한다.
                        //---------------------------------------------

                        if ( ( sPredCur->node.module == & mtfBetween )
                          || ( sPredCur->node.module == & mtfNotBetween ) )
                        {
                            IDE_TEST( transJoinOperBetweenStyle( aStatement,
                                                                 aQuerySet,
                                                                 sPredCur,
                                                                 & sTransStart,
                                                                 & sTransEnd )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( transJoinOperListArgStyle( aStatement,
                                                                 aQuerySet,
                                                                 sPredCur,
                                                                 & sTransStart,
                                                                 & sTransEnd )
                                      != IDE_SUCCESS );
                        }

                        //---------------------------------------------
                        // 변환 완료한 원본 Predicate 은 OR 노드에서
                        // 연결 제거
                        //---------------------------------------------

                        if ( sPredCur == (qtcNode *)sCurOR->node.arguments )
                        {
                            sCurOR->node.arguments = sPredCur->node.next;
                        }
                        else
                        {
                            sPredPrev->node.next = sPredCur->node.next;
                        }

                        //---------------------------------------------
                        // OR 노드가 가지고 있는 모든 Predicate 이 변환되어서
                        // 더이상 argument 가 없다면 OR 노드는 기존 normalCNF에서
                        // 제거되어야 한다.
                        //---------------------------------------------

                        if ( sCurOR->node.arguments == NULL )
                        {
                            if ( sCurOR == (qtcNode *)aTrans->normalCNF->node.arguments )
                            {
                                aTrans->normalCNF->node.arguments = sCurOR->node.next;
                            }
                            else
                            {
                                sPrevOR->node.next = sCurOR->node.next;
                            }
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        //---------------------------------------------
                        // 변환된 결과 노드들은 일단 차례차례 별도로 모두 연결해둔다.
                        //
                        //  (OR) -> (OR) -> ... -> (OR) -> (OR) -> NULL
                        //   ^                              ^
                        //   |                              |
                        // sTmpTransStart               sTmpTransEnd
                        //---------------------------------------------

                        if ( sIsTransFirst == ID_FALSE )
                        {
                            //---------------------------------------------
                            // normalCNF 를 통틀어 처음 변환이 발생했을 때
                            //---------------------------------------------
                            sTmpTransStart = sTransStart;
                            sTmpTransEnd   = sTransEnd;

                            sIsTransFirst = ID_TRUE;
                        }
                        else
                        {
                            sTmpTransEnd->node.next = (mtcNode *)sTransStart;
                            sTmpTransEnd = sTransEnd;
                        }

                        //---------------------------------------------
                        // 현재의 OR 노드 이하에서 변환된 Predicate 이
                        // 있었다는 것을 표시해둠.
                        //---------------------------------------------

                        sIsTransformedOR = ID_TRUE;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                //---------------------------------------------
                // Nothing to do.
                //
                // (+) 가 없는 Predicate 은 추후 ANSI 형태로
                // 변환한 후에도 그대로 normalCNF 에 남아있게
                // 되므로 전혀 상관없다.
                //---------------------------------------------
            }
        } // for sPredCur

        //---------------------------------------------
        // 현재의 OR 노드 이하의 Predicate 중에서 변환이
        // 발생했다면 OR 노드에 대해 다시 estimation
        // 그리고, 제거되지 않은 마지막 OR 노드를 sPrevOR 에
        // 기억해둔다.
        // 이는 아래와 같은 연결에서 (OR 2)가 변환될 경우
        // (OR 1)을 기억해둠으로써 (OR 2)을 리스트의 연결에서
        // 끊기 위함이다.
        //
        // (OR 1) -> (OR 2) -> (OR 3)
        //---------------------------------------------
        if ( sIsTransformedOR == ID_TRUE )
        {
            if ( sCurOR->node.arguments != NULL )
            {
                sPrevOR = sCurOR;

                IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                         sCurOR )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sPrevOR = sCurOR;
        }
    } // for sCurOR


    //---------------------------------------------
    // 변환이 한번이라도 발생했다면 임시로 저장해둔
    // sTmpTransStart 와 sTmpTransEnd 가 있을 것이다.
    // 이를 기존의 normalCNF 의 맨앞에 연결해준다.
    //---------------------------------------------

    if ( sIsTransFirst == ID_TRUE )
    {
        if ( ( sTmpTransStart == NULL )
          || ( sTmpTransEnd == NULL )
          || ( aTrans->normalCNF == NULL ) )
        {
            IDE_RAISE( ERR_INVALID_TRANSFORM_RESULT );
        }
        else
        {
            sTmpTransEnd->node.next = aTrans->normalCNF->node.arguments;
            aTrans->normalCNF->node.arguments = (mtcNode *)sTmpTransStart;

            IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                     aTrans->normalCNF )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_TRANSFORM_RESULT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::transNormalCNF4SpecialPred",
                                  "normalCNF transform error" ));
    }
    IDE_EXCEPTION( ERR_INVALID_OPERATION_WITH_LISTS );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_OPERATION_WITH_LISTS,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_OUTER_JOINED_TO_SUBQUERY );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_OUTER_JOINED_TO_SUBQUERY,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::validateJoinOperConstraints( qcStatement * aStatement,
                                               qmsQuerySet * aQuerySet,
                                               qtcNode     * aNormalCNF )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *    transNormalCNF4SpecialPred 를 통해 변환이 완료된 Tree 의
 *    각 Predicate 에 대해 제약사항 검사
 *
 * Implementation :
 *
 *    Predicate 단위로 검사가 불가능한 제약사항은 Predicate 간의
 *    관계가 완성되는 transformStruct2ANSIStyle 내에서 수행한다.
 *
 *    (AND)
 *      |
 *      V
 *    (OR)  ->  (OR)  ->  (OR)  -> ...
 *      |         |         |
 *      V         V         V
 *    (Pred)    (Pred)    (Pred)   <--- 각각 제약사항 검사
 *
 *
 *   (+) 사용 시 제약사항을 검사하는 규칙을 알아보자.
 *
 *   모든 Predicate 은 하나의 결과를 다른 결과와 비교를
 *   하는 것이다. (즉, selectivity 가 존재한다.)
 *   이 말은 비교주체와 비교대상이 반드시 존재한다는 말이다.
 *   즉, =, between, in, >=, ... 등 모든 Predicate 연산자는
 *   모두 비교주체와 비교대상이 필요하다.
 *
 *   (+) 의 제약사항 관련하여 이러한 비교에서 중요한 것은
 *   Join 관계를 가질 때이다.
 *   즉, 비교주체나 비교대상이 constant value 가 아닌 경우를
 *   말한다.
 *   지금부터 비교주체를 Join Source, 비교대상을 Join Target
 *   이라고 지칭하고 설명하도록 한다.
 *   (보통 Inner/Outer 테이블이라는 용어를 사용하지만
 *    좀더 일반적인 쉬운 용어로 설명한다.)
 *
 *
 *   (+)를 사용하고 Join 관계를 가지는 Predicate 에서
 *   명확한 제약사항은 다음의 세가지 경우이다.
 *
 *
 *   (1) Join Source 와 Join Target 양쪽에 (+)를 사용할 수 없다.
 *       ex> t1.i1(+) = t2.i1(+), 1 = t2.i1(+)+t3.i1(+)
 *
 *   (2) (+) 를 사용하는 Join Source 에 대한 Join Target 테이블은
 *       2 개 이상일 수 없다.
 *       ex> t1.i1(+) = t2.i1+t3.i1, t1.i1 = t2.i1(+)+t3.i1
 *
 *   (3) (+) 를 사용하는 테이블이 자기자신과 Outer Join 될 수 없다.
 *       아래 두번째 ex 는 Join 이 연결을 거듭하여 결국 자신과
 *       연결되는 경우이다.
 *       ex> t1.i1(+) = t1.i2, t1.i2-t1.i1(+) = 10
 *       ex> t1.i1(+) = t2.i1 and t2.i1(+) = t3.i1 and t3.i1(+) = t1.i1
 *
 *
 *   위와 같이 Predicate 단위로 단순명료한 Rule 을 적용할 수 있는 것은
 *   이미 Between 류와 List Argument 를 가지는 비교연산자류 대해 이미
 *   변환작업을 한 상태로 검사를 하기 때문이다. (transNormalCNF4SpecialPred)
 ***********************************************************************/

    qcuSqlSourceInfo    sSqlInfo;
    qmsFrom           * sFrom    = NULL;
    qtcNode           * sCurOR   = NULL;
    qtcNode           * sCurPred = NULL;
    qtcNode           * sPred    = NULL;
    SInt                sJoinOperCount;
    SInt                sDepCount;
    UInt                sArgCount = 0;
    idBool              sIsArgCountMany = ID_FALSE;
    qcDepInfo           sCompareDep;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::validateJoinOperConstraints::__FT__" );

    IDE_FT_ASSERT( aStatement   != NULL );
    IDE_FT_ASSERT( aQuerySet    != NULL );
    IDE_FT_ASSERT( aNormalCNF   != NULL );


    //------------------------------------------------------
    // Outer Join Operator 는 ANSI Join 구문과 함께 사용될 수 없다.
    // 다만, 이는 Where 절에만 해당되고 다른 절에 포함된 것은
    // 그냥 무시하면 된다. (on 절도 마찬가지)
    //------------------------------------------------------

    for ( sFrom  = aQuerySet->SFWGH->from;
          sFrom != NULL;
          sFrom  = sFrom->next)
    {
        if ( sFrom->joinType != QMS_NO_JOIN )
        {
            sSqlInfo.setSourceInfo( aStatement,
                                   & aQuerySet->SFWGH->where->position );
            IDE_RAISE(ERR_JOIN_OPER_NOT_ALLOWED_WITH_ANSI_JOIN );
        }
        else
        {
            // Nothing to do.
        }
    }

    for ( sCurOR  = (qtcNode *)aNormalCNF->node.arguments;
          sCurOR != NULL;
          sCurOR  = (qtcNode *)sCurOR->node.next )
    {
        sPred = (qtcNode *)sCurOR->node.arguments;
        sArgCount = 1;

        while ( sPred->node.next != NULL )
        {
            ++sArgCount;
            sPred = (qtcNode *)sPred->node.next;
        }

        if ( sArgCount > 1 )
        {
            sIsArgCountMany = ID_TRUE;
        }
        else
        {
            sIsArgCountMany = ID_FALSE;
        }

        //------------------------------------------------------
        // OR 의 argument 노드가 다수일 때 dependency 정보의 비교를
        // 위해 저장해 둔다.
        // OR 하위의 각 predicate 들은 dependency 정보가 절대 달라서는 안된다.
        // 추후 ANSI 구문으로의 변환 시 OR 노드가 통째로 하나의 predicate 처럼
        // 움직일 수 있는 조건이어야 결과가 틀려지지 않는다.
        //------------------------------------------------------
        qtc::dependencySetWithDep( & sCompareDep, & ((qtcNode*)sCurOR->node.arguments)->depInfo );

        for ( sCurPred  = (qtcNode *)sCurOR->node.arguments;
              sCurPred != NULL;
              sCurPred  = (qtcNode *)sCurPred->node.next )
        {
            //------------------------------------------------------
            // Predicate Level 에서 Outer Join Operator 가 있는 것만
            // 제약사항을 검사하면 된다. (기본제약사항 검사)
            //------------------------------------------------------

            if ( ( sCurPred->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                   == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                //------------------------------------------------------
                // terminal predicate 에서 2 개 이상의 dependency table에
                // (+)를 사용했을 경우
                //------------------------------------------------------
                sJoinOperCount = qtc::getCountJoinOperator( &sCurPred->depInfo );

                if ( sJoinOperCount > QMO_JOIN_OPER_TABLE_CNT_PER_PRED )
                {
                    sSqlInfo.setSourceInfo( aStatement,
                                            & sCurPred->position );
                    IDE_RAISE(ERR_TOO_MANY_REFERENCE_OUTER_JOINED_TABLE);
                }
                else
                {
                    // Nothing to do.
                }

                //------------------------------------------------------
                // terminal predicate 에서 (+) 를 사용하면서
                // dependency table 이 2 개를 넘을 수 없다.
                //------------------------------------------------------
                sDepCount = qtc::getCountBitSet( & sCurPred->depInfo );

                if ( sDepCount > QMO_JOIN_OPER_DEPENDENCY_TABLE_CNT_PER_PRED )
                {
                    sSqlInfo.setSourceInfo( aStatement,
                                            & sCurPred->position );
                    IDE_RAISE(ERR_TOO_MANY_REFERENCE_OUTER_JOINED_TABLE);
                }
                else
                {
                    // Nothing to do.
                }

                //------------------------------------------------------
                // predicate 내에서 t1.i1(+) - t1.i2 = 1 과 같이 한 테이블이
                // 서로 Outer Join 의 대상이 될 수 없다.
                //------------------------------------------------------
                if( qtc::isOneTableOuterEachOther( &sCurPred->depInfo )
                    == ID_TRUE )
                {
                    sSqlInfo.setSourceInfo( aStatement,
                                            & sCurPred->position );
                    IDE_RAISE(ERR_NOT_ALLOW_OUTER_JOIN_EACH_OTHER);
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }

            //------------------------------------------------------
            // 1. OR 노드 이하에서 outer join operator 가 사용되지 않을
            //    경우와 argument 가 1 개일 경우는 전혀 상관없다.
            // 2. OR 노드 이하에서 outer join operator 가 사용되었을 경우,
            //    dependency table 이 2 개 이상일 수 없다.
            // 3. OR 노드 이하에서 outer join operator 가 사용된다면 dependency table 은
            //    1 개일 수 밖에 없으므로, OR 노드의 argument 들 각각은 모두
            //    outer join operator 가 붙은 같은 dependency table 이어야한다.
            //    dependency table 이 1 개이면서 각 predicate 에서 같은 dependency table 에
            //    대해 outer join operator 유무가 달라서는 안된다.
            //
            //    즉, 위의 규칙들을 간단히 요약하면,
            //
            //    "OR 이하 노드들은 모두 묶어서 하나의 prediicate 으로 취급해야 한다.
            //     다르게 말하면, ANSI outer join 구문으로 변경했을 때 OR 이하의
            //     특정 argument 는 on 절로 옮겨지고 어떤 argument 는 그대로 남고
            //     할 수는 없다는 말이다.
            //     모든 argument 가 함께 옮겨가겨나 남아있거나 둘 중의 하나다.
            //     이러한 Rule 을 위배하면 무조건 에러를 발생시키면 된다."
            //------------------------------------------------------
            if ( ( sCurOR->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                   == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                if ( sIsArgCountMany == ID_TRUE )
                {
                    sDepCount = qtc::getCountBitSet( & sCurPred->depInfo );

                    if ( sDepCount > 1 )
                    {
                        sSqlInfo.setSourceInfo( aStatement,
                                                & sCurPred->position );
                        IDE_RAISE(ERR_JOIN_OPER_NOT_ALLOWED_IN_OPERAND_OR);
                    }
                    else
                    {
                        if ( qtc::dependencyJoinOperEqual( & sCompareDep,
                                                           & sCurPred->depInfo )
                             == ID_FALSE )
                        {
                            sSqlInfo.setSourceInfo( aStatement,
                                                    & sCurPred->position );
                            IDE_RAISE(ERR_JOIN_OPER_NOT_ALLOWED_IN_OPERAND_OR);
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_JOIN_OPER_NOT_ALLOWED_WITH_ANSI_JOIN );
    {
        (void)sSqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_ALLOWED_WITH_ANSI_JOIN,
                                  sSqlInfo.getErrMessage() ) );
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_TOO_MANY_REFERENCE_OUTER_JOINED_TABLE );
    {
        (void)sSqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_TOO_MANY_REFERENCE_OUTER_JOINED_TABLE,
                                  sSqlInfo.getErrMessage() ) );
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_JOIN_OPER_NOT_ALLOWED_IN_OPERAND_OR );
    {
        (void)sSqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_ALLOWED_IN_OPERAND_OR,
                                  sSqlInfo.getErrMessage() ) );
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_OUTER_JOIN_EACH_OTHER );
    {
        (void)sSqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_ALLOW_OUTER_JOIN_EACH_OTHER,
                                  sSqlInfo.getErrMessage() ) );
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::transformStruct2ANSIStyle( qcStatement      * aStatement,
                                             qmsQuerySet      * aQuerySet,
                                             qmoJoinOperTrans * aTrans )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Outer Join Operator를 사용하는 where 절을 ANSI 형태로 변환
 *
 * Implementation :
 *
 *    이 함수에서는 normalCNF 의 모든 Predicate 을 Outer Join Operator 유무,
 *    Join/constant/onetable Predicate 여부, 각 Predicate 들의
 *    dependency table 들간의 관계를 파악하여 분류 및 Grouping,
 *    normalCNF 및 From Tree 변환을 수행한다.
 *
 *    이 함수에서는 다음과 같은 일을 수행한다.
 *
 *     1. Predicate 을 다음의 3 가지로 분류
 *        가. (+)없는 predicate           : qmoJoinOperTrans->generalPred
 *        나. (+)있고 one table predicate : qmoJoinOperTrans->oneTablePred
 *        다. (+)있고 join predicate      : qmoJoinOperTrans->joinOperPred
 *     2. qmoJoinOperTrans->joinOperPred 을 Grouping
 *        : qmoJoinOperTrans->joinOperGroup
 *     3. qmoJoinOperTrans->joinOperGroup 별로 dependency info 를 Relationing
 *        가. 중복 Relation 제거
 *        나. qmoJoinOperTrans->joinOperGroup->joinRelation 을
 *            depend(TupleId) 값에 따라 재정렬
 *     4. qmoJoinOperTrans 의 predicate, grouping, relation 정보를 이용하여
 *        ANSI 형태로 변환
 *
 *
 *    변환과정에서 아래의 자료구조를 어떻게 만들고 이용는지 살펴보자.
 *
 *    -------------------------------------------------------
 *    *  qmoJoinOperGroup 내의 joinRelation 은 실제적으로는 *
 *    *  Array 형태이다.                                    *
 *    *  설명을 용이하게 하기 위해 list 인 것처럼 설명한다. *
 *    -------------------------------------------------------
 *
 *    ================
 *    qmoJoinOperTrans    ----> (*pred1) -> (*pred2) -> ...
 *    ================    |
 *    | oneTablePred ------
 *    | joinOperPred ---------> (*pred5) -> (*pred4) -> (*pred3) -> ...
 *    | generalPred  ------
 *    |              |    |
 *    |              |    ----> (*pred6) -> (*pred7) -> ...
 *    | joinOperGroup------
 *    ================    |     ===================
 *                        ----> | qmoJoinOperGroup|  --> (*pred5) -> (*pred3) ->
 *                        |     ===================  |
 *                        |     | joinPredicate   ----
 *                        |     | joinRelation    -----> (*pred3) -> (*pred5) ->
 *                        |     ===================
 *                        |     ===================
 *                        ----> | qmoJoinOperGroup|  --> (*pred4) ->
 *                              ===================  |
 *                              | joinPredicate   ----
 *                              | joinRelation    -----> (*pred4) ->
 *                              ===================
 *
 *
 *    다음과 같은 조건을 가진 SQL 이 있다고 가정하자.
 *
 *    where pred1  -> one table (+) predicate
 *      and pred2  -> one table (+) predicate
 *      and pred3  -> (+) join predicate
 *      and pred4  -> (+) join predicate
 *      and pred5  -> (+) join predicate
 *      and pred6  -> none (+) predicate
 *      and pred7  -> none (+) predicate
 *
 *
 *    (1) Predicate 의 분류
 *        위의 각 predicate 에 대해 qtcNode 의 depInfo 정보를 통해
 *        다음과 같이 세개의 자료구조 리스트에 연결해둔다.
 *
 *        ================
 *        qmoJoinOperTrans    ----> (*pred1) -> (*pred2) -> NULL
 *        ================    |
 *        | oneTablePred ------
 *        | joinOperPred ---------> (*pred5) -> (*pred4) -> (*pred3) -> NULL
 *        | generalPred  ------
 *        |              |    |
 *        |              |    ----> (*pred6) -> (*pred7) -> NULL
 *        ================
 *
 *    (2) (+) 가 있는 Join Predicate 들을 Grouping
 *        위에서 분류한 리스트 중 (+) 가 존재하고 Join 관계를 가진 Predicate
 *        들을 가지고 있는 joinOperPred 리스트의 노드들을 Grouping 한다.
 *        (Join 관계가 있는 것들끼리 묶는다는 뜻이다)
 *
 *        joinOperPred 리스트의 노드를 순회하며 join 관계가 있는 것들을
 *        차례로 각각의 qmoJoinOperGroup 의 joinPredicate 에 연결하고
 *        원래의 joinOperPred 에서는 제거한다.
 *
 *        ================
 *        qmoJoinOperTrans
 *        ================
 *        |              |
 *        | joinOperPred ---------> NULL
 *        |              |
 *        | joinOperGroup----
 *        ================  |     ===================
 *                          ----> | qmoJoinOperGroup|  --> (*pred5) -> (*pred3) -> NULL
 *                          |     ===================  |
 *                          |     | joinPredicate   ----
 *                          |     | joinRelation    -----> NULL
 *                          |     ===================
 *                          |     ===================
 *                          ----> | qmoJoinOperGroup|  --> (*pred4) -> NULL
 *                                ===================  |
 *                                | joinPredicate   ----
 *                                | joinRelation    -----> NULL
 *                                ===================
 *
 *    (3) Join Relationning
 *        (2) 에서 Grouping 을 통해 완성한 각 Group 의 joinPredicate 의
 *        리스트노드를 순회하면서 depInfo->depend 값에 따라 정렬하고(Quick Sort),
 *        중복된 dependency 정보를 가지는 Predicate 의 경우는 뺀 후
 *        joinRelation 리스트에 연결한다.
 *                     
 *        ================
 *        qmoJoinOperTrans
 *        ================
 *        |              |
 *        | joinOperGroup----
 *        ================  |     ===================
 *                          ----> | qmoJoinOperGroup|  --> (*pred5) -> (*pred3) -> NULL
 *                          |     ===================  |
 *                          |     | joinPredicate   ----
 *                          |     | joinRelation    -----> (*pred3) -> (*pred5) -> NULL
 *                          |     ===================
 *                          |     ===================
 *                          ----> | qmoJoinOperGroup|  --> (*pred4) -> NULL
 *                                ===================  |
 *                                | joinPredicate   ----
 *                                | joinRelation    -----> (*pred4) -> NULL
 *                                ===================
 *
 *   (4) joinRelation 정보를 이용한 변환
 *       이제 실질적으로 변환작업을 해야한다.
 *       변환대상은 qmsSFWGH->from Tree 와 qmoJoinOperTrans->normalCNF Tree 이다.
 *       다음의 과정으로 변환작업을 수행한다.
 *
 *       (가) 첫번째 joinRelation 노드의 dependency 정보를 이용하여
 *            From 리스트에서 관련 노드를 찾는다.
 *
 *       (나) 찾아낸 From 노드들을 Left outer join 노드 Tree 형태로
 *            변환한다.
 *
 *       (다) 변환된 Left outer join 노드의 onCondition 에 연결할
 *            Predicate 을 모두 찾아서 연결해준다.
 *
 *            a. joinRelation 에 대응하는 joinPredicate 을 찾아서
 *               onCondition 에 연결 후 리스트에서 제거한다.
 *            b. (1)에서 분류해 두었던 oneTablePred 중에서 관련된
 *               것을 onCondtion 에 연결한 후 리스트에서 제거한다.
 *
 *       (라) 다음 joinRelation 노드에 대해 (가)의 과정부터 반복한다.
 *
 *       ** joinPredicate 및 oneTablePred 노드를 리스트에서 제거할 때는
 *          해당 노드가 가리키는 Predicate 노드 또한 normalCNF 에서
 *          제거해 주어야 한다.
 *          결국, where 조건을 통해 만들어진 모든 predicate 노드들을
 *          가지고 있던 normalCNF 는, 변환작업이 끝나면 Left Outer Join
 *          의 onCondition 으로 옮겨진 노드를 제외한 것들만 남게 된다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::transformStruct2ANSIStyle::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aTrans     != NULL );


    //------------------------------------------
    // 이 단계에서 max join group 을 알기는 힘들고
    // Query 의 dependency table 의 갯수를 max 로 잡는다.
    //------------------------------------------

    // BUG-43897 잘못된 predicate을 고려하여 충분히 만들어둔다.
    aTrans->maxJoinOperGroupCnt = qtc::getCountBitSet( &aQuerySet->SFWGH->depInfo ) * 4;

    //------------------------------------------
    // Predicate의 분류
    //------------------------------------------

    IDE_TEST( classifyJoinOperPredicate( aStatement, aQuerySet, aTrans )
              != IDE_SUCCESS );

    //------------------------------------------
    // Predicate 을 분류한 후 joinOperPred 이 없다면,
    // 아래의 과정은 불필요하다.
    // 즉, Outer Join Operator 를 사용했더라도,
    // constant Predicate 과 one table predicate 만
    // 있는 경우는 ANSI 구조로 변경할 이유가 없다.
    //------------------------------------------

    if ( aTrans->joinOperPred != NULL )
    {
        //------------------------------------------
        // joinOperGroup 배열 공간 확보 및 초기화
        //------------------------------------------

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoJoinOperGroup ) * aTrans->maxJoinOperGroupCnt,
                                                   (void **) & aTrans->joinOperGroup )
                  != IDE_SUCCESS );

        // joinOperGroup들의 초기화
        IDE_TEST( initJoinOperGroup( aStatement,
                                     aTrans->joinOperGroup,
                                     aTrans->maxJoinOperGroupCnt )
                  != IDE_SUCCESS );

        //------------------------------------------
        // Join Predicate 의 Grouping 및 Relationing, Ordering
        //------------------------------------------

        IDE_TEST( joinOperOrdering( aStatement, aTrans )
                  != IDE_SUCCESS );

        //------------------------------------------
        // Relationing 된 List 를 이용하여 ANSI 형태로 변환
        //  - From, normalCNF, On 절
        //------------------------------------------

        IDE_TEST( transformJoinOper( aStatement, aQuerySet, aTrans )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::isOneTableJoinOperPred( qmsQuerySet * aQuerySet,
                                          qtcNode     * aPredicate,
                                          idBool      * aIsTrue )
{
/***********************************************************************
 *
 * Description : one table predicate의 판단
 *
 *     one table predicate: dependency table 이 1 개인 predicate
 *     예) T1.i1 = 1
 *
 * Implementation :
 *
 ***********************************************************************/

    qcDepInfo  sAndDependencies;
    SInt       sDepCount = 0;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::isOneTableJoinOperPred::__FT__" );

    IDE_DASSERT( aPredicate        != NULL );
    IDE_DASSERT( aIsTrue           != NULL );

    // BUG-35468 외부 참조 컬럼의 depInfo는 제거한다.
    qtc::dependencyAnd( & aQuerySet->SFWGH->depInfo,
                        & aPredicate->depInfo,
                        & sAndDependencies );

    sDepCount = qtc::getCountBitSet( & sAndDependencies );

    if ( sDepCount == 1 )
    {
        *aIsTrue = ID_TRUE;
    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoOuterJoinOper::isConstJoinOperPred( qmsQuerySet * aQuerySet,
                                       qtcNode     * aPredicate,
                                       idBool      * aIsTrue )
{
/***********************************************************************
 *
 * Description : constant predicate의 판단
 *
 *     constant predicate은 dependency table 이 없는 Predicate 으로 정의
 *     예)  1 = 1
 *
 * Implementation :
 *
 ***********************************************************************/

    qcDepInfo  sAndDependencies;
    SInt       sDepCount = 0;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::isConstJoinOperPred::__FT__" );

    IDE_DASSERT( aPredicate        != NULL );
    IDE_DASSERT( aIsTrue           != NULL );

    //--------------------------------------
    // constant predicate의 판단
    // dependencies는 최상위 노드에서 판단한다.
    //--------------------------------------

    // BUG-36358 외부 참조 컬럼의 depInfo는 제거한다.
    qtc::dependencyAnd( & aQuerySet->SFWGH->depInfo,
                        & aPredicate->depInfo,
                        & sAndDependencies );

    sDepCount = qtc::getCountBitSet( & sAndDependencies );

    if ( sDepCount == 0 )
    {
        *aIsTrue = ID_TRUE;
    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoOuterJoinOper::createJoinOperPred( qcStatement      * aStatement,
                                      qtcNode          * aNode,
                                      qmoJoinOperPred ** aNewPredicate )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *    Outer Join Operator 를 포함하는 Join Predicate 을 관리할 자료구조 생성
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::createJoinOperPred::__FT__" );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoJoinOperPred ),
                                               (void**)aNewPredicate )
              != IDE_SUCCESS );

    (*aNewPredicate)->node = aNode;
    (*aNewPredicate)->next = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::initJoinOperGroup( qcStatement      * aStatement,
                                     qmoJoinOperGroup * aJoinGroup,
                                     UInt               aJoinGroupCnt )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Join Oper Group의 초기화
 *
 * Implementation :
 *    Join Oper Group 을 초기화하면서, 각 Group 내에
 *    joinRelation 을 Array 형태로 미리 할당하고 초기화한다.
 *
 ***********************************************************************/

    qmoJoinOperGroup * sCurJoinGroup = NULL;
    qmoJoinOperRel   * sCurJoinRel   = NULL;
    UInt               i, j;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::initJoinOperGroup::__FT__" );

    // 각 Join Oper Group의 초기화
    for ( i = 0;
          i < aJoinGroupCnt;
          i++ )
    {
        sCurJoinGroup = & aJoinGroup[i];

        sCurJoinGroup->joinPredicate = NULL;
        sCurJoinGroup->joinRelationCnt = 0;
        sCurJoinGroup->joinRelationCntMax = aJoinGroupCnt;

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoJoinOperRel ) * aJoinGroupCnt,
                                                   (void **)& sCurJoinGroup->joinRelation )
                  != IDE_SUCCESS );

        for ( j = 0;
              j < aJoinGroupCnt;
              j++ )
        {
            sCurJoinRel = & sCurJoinGroup->joinRelation[j];
            qtc::dependencyClear( & sCurJoinRel->depInfo );
        }

        qtc::dependencyClear( & sCurJoinGroup->depInfo );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::classifyJoinOperPredicate( qcStatement       * aStatement,
                                             qmsQuerySet       * aQuerySet,
                                             qmoJoinOperTrans  * aTrans )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : normalCNF의 Predicate 분류
 *
 * Implementation :
 *    qmoJoinOperTrans::normalCNF의 각 Predicate에 대하여 다음을 수행
 *
 *    (1) Constant Predicate 분류 -> generalPred 에 연결
 *    (2) Outer Join Operator + One Table Predicate의 분류 -> oneTablePred 에 연결
 *    (3) Outer Join Operator + Join Predicate의 분류 -> joinOperPred 에 연결
 *    (4) Outer Join Operator 가 없는 Predicate 분류 -> generalPred 에 연결
 *
 ***********************************************************************/

    qtcNode         * sNode    = NULL;
    qmoJoinOperPred * sNewPred = NULL;

    idBool            sIsConstant = ID_FALSE;
    idBool            sIsOneTable = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::classifyJoinOperPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTrans     != NULL );


    // Predicate 분류 대상 Node
    if ( aTrans->normalCNF != NULL )
    {
        sNode = (qtcNode *)aTrans->normalCNF->node.arguments;
    }
    else
    {
        sNode = NULL;
    }

    while ( sNode != NULL )
    {
        // qmoJoinOperPred 생성
        IDE_TEST( createJoinOperPred( aStatement,
                                      sNode,
                                      & sNewPred )
                  != IDE_SUCCESS );

        //-------------------------------------------------
        // Constant Predicate 검사
        // constant Predicate 이면 무조건 generalPred 에 연결
        //-------------------------------------------------

        IDE_TEST( isConstJoinOperPred( aQuerySet,
                                       (qtcNode*)sNewPred->node,
                                       & sIsConstant )
                  != IDE_SUCCESS );

        if ( sIsConstant == ID_TRUE )
        {
            sNewPred->next      = aTrans->generalPred;
            aTrans->generalPred = sNewPred;
        }
        else
        {
            //---------------------------------------------
            // Predicate Node 에 Outer Join Operator 가 있으면,
            // oneTable predicate 과 twoTable predicate 을 분류하여 저장.
            // Outer Join Operator 가 없으면 무조건 generalPred 에 연결.
            //---------------------------------------------

            if ( ( sNode->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                 == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                //---------------------------------------------
                // One Table Predicate 검사
                //---------------------------------------------

                IDE_TEST( isOneTableJoinOperPred( aQuerySet,
                                                  (qtcNode*)sNewPred->node,
                                                  & sIsOneTable )
                          != IDE_SUCCESS );

                if ( sIsOneTable == ID_TRUE )
                {
                    sNewPred->next       = aTrans->oneTablePred;
                    aTrans->oneTablePred = sNewPred;
                }
                else
                {
                    //---------------------------------------------
                    // oneTablePredicate이 아니면
                    // aTrans->joinOperPred에 연결
                    //---------------------------------------------

                    sNewPred->next       = aTrans->joinOperPred;
                    aTrans->joinOperPred = sNewPred;
                }
            }
            else
            {
                sNewPred->next      = aTrans->generalPred;
                aTrans->generalPred = sNewPred;
            }
        }
        sNode = (qtcNode*)sNode->node.next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOuterJoinOper::joinOperGrouping( qmoJoinOperTrans  * aTrans )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Join Group의 분류
 *
 * Implemenation :
 *      qmoJoinOperTrans::joinOperPred 에 있는 모든 Predicate 들을
 *      Join Group 별로 나누어서 qmoJoinOperTrans::joinOperGroup 의
 *      joinPredicate 에 연결해둔다.
 *
 ***********************************************************************/

    SInt                   sJoinGroupCnt;
    SInt                   sJoinGroupIndex;
    qmoJoinOperGroup     * sJoinGroup    = NULL;
    qmoJoinOperGroup     * sCurJoinGroup = NULL;
    qmoJoinOperPred      * sFirstPred    = NULL;
    qmoJoinOperPred      * sAddPred      = NULL;
    qmoJoinOperPred      * sCurPred      = NULL;
    qmoJoinOperPred      * sPrevPred     = NULL;

    qcDepInfo              sAndDependencies;
    idBool                 sIsInsert = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::joinOperGrouping::__FT__" );

    IDE_DASSERT( aTrans   != NULL );

    sJoinGroup       = aTrans->joinOperGroup;
    sJoinGroupIndex  = -1;
    sJoinGroupCnt    = 0;


    sFirstPred  = aTrans->joinOperPred;

    while ( sFirstPred != NULL )
    {

        //------------------------------------------
        // Join Group에 속하지 않는 첫번째 Predicate 존재 :
        //    Join Group에 속하지 않은 첫번째 Predicate의
        //    dependencies로 새로운 Join Group의 dependencies를 설정
        //------------------------------------------

        ++sJoinGroupIndex;

        if ( sJoinGroupIndex >= (SInt)aTrans->maxJoinOperGroupCnt )
        {
            IDE_RAISE( ERR_UNEXPECTED_MODULE );
        }
        else
        {
            // Nothing to do.
        }

        sCurJoinGroup = & sJoinGroup[sJoinGroupIndex];
        sJoinGroupCnt = sJoinGroupIndex + 1;

        IDE_TEST( qtc::dependencyOr( & sCurJoinGroup->depInfo,
                                     & sFirstPred->node->depInfo,
                                     & sCurJoinGroup->depInfo )
                  != IDE_SUCCESS );

        //------------------------------------------
        // 현재 JoinGroup과 연관된 모든 Join Predicate을 찾아
        // JoinGroup에 등록
        //------------------------------------------

        sCurPred  = sFirstPred;
        sPrevPred = NULL;
        sIsInsert = ID_FALSE;

        while ( sCurPred != NULL )
        {

            qtc::dependencyAnd( & sCurJoinGroup->depInfo,
                                & sCurPred->node->depInfo,
                                & sAndDependencies );

            if ( qtc::dependencyEqual( & sAndDependencies,
                                       & qtc::zeroDependencies )
                 == ID_FALSE )
            {
                //------------------------------------------
                // Join Group과 연관된 Predicate
                //------------------------------------------

                // Join Group에 새롭게 등록하는 Predicate이 존재
                sIsInsert = ID_TRUE;

                // Join Group의 등록
                IDE_TEST( qtc::dependencyOr( & sCurJoinGroup->depInfo,
                                             & sCurPred->node->depInfo,
                                             & sCurJoinGroup->depInfo )
                          != IDE_SUCCESS );

                // Predicate을 Join Predicate에서 연결을 끊고,
                // JoinGroup의 joinPredicate에 연결시킴

                if ( sCurJoinGroup->joinPredicate == NULL )
                {
                    sCurJoinGroup->joinPredicate = sCurPred;
                    sAddPred                     = sCurPred;
                }
                else
                {
                    IDE_FT_ASSERT( sAddPred != NULL );
                    sAddPred->next = sCurPred;
                    sAddPred       = sAddPred->next;
                }
                sCurPred       = sCurPred->next;
                sAddPred->next = NULL;

                if ( sPrevPred == NULL )
                {
                    sFirstPred = sCurPred;
                }
                else
                {
                    sPrevPred->next = sCurPred;
                }
            }
            else
            {
                // Join Group과 연관되지 않은 Predicate : nothing to do
                sPrevPred = sCurPred;
                sCurPred = sCurPred->next;
            }

            if ( ( sCurPred == NULL ) && ( sIsInsert == ID_TRUE ) )
            {
                //------------------------------------------
                // Join Group에 추가 등록한 Predicate이 존재하는 경우,
                // 추가된 Predicate과 연관된 Join Predicate이 존재할 수 있음
                //------------------------------------------

                sCurPred  = sFirstPred;
                sPrevPred = NULL;
                sIsInsert = ID_FALSE;
            }
            else
            {
                // 더 이상 연관된 Predicate 없음 : nothing to do
            }
        }
    }

    // 실제 Join Group 개수 설정
    aTrans->joinOperGroupCnt = sJoinGroupCnt;

    // aTrans의 joinOperPred을 joinGroup의 joinPredicate으로 모두 분류시킴
    aTrans->joinOperPred = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_MODULE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::joinOperGrouping",
                                  "join group count > max" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

extern "C" SInt
compareJoinOperRel( const void * aElem1,
                    const void * aElem2 )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Join Relation 의 Quick Sort
 *
 * Implementation :
 *    만약 다음과 같이 Join Relation 들이 등록되어 있을 경우 각
 *    table 값에 따라 정렬한다.
 *    (첫번째,두번째 table 값에 따라)
 *
 *    정렬전
 *    (2,3) -> (1,3) -> (2,5) -> (3,4) -> (1,2)
 *
 *    정렬후
 *    (1,2) -> (1,3) -> (2,3) -> (2,5) -> (3,4)
 *
 *   qmoJoinOperGroup->joinRelation 은 array.
 *
 ***********************************************************************/

    if( qtc::getDependTable( &((qmoJoinOperRel *)aElem1)->depInfo,0 ) >
        qtc::getDependTable( &((qmoJoinOperRel *)aElem2)->depInfo,0 ) )
    {
        return 1;
    }
    else if( qtc::getDependTable( &((qmoJoinOperRel *)aElem1)->depInfo,0 ) <
             qtc::getDependTable( &((qmoJoinOperRel *)aElem2)->depInfo,0 ) )
    {
        return -1;
    }
    else
    {
        if( qtc::getDependTable( &((qmoJoinOperRel *)aElem1)->depInfo,1 ) >
            qtc::getDependTable( &((qmoJoinOperRel *)aElem2)->depInfo,1 ) )
        {
            return 1;
        }
        else if( qtc::getDependTable( &((qmoJoinOperRel *)aElem1)->depInfo,1 ) <
                 qtc::getDependTable( &((qmoJoinOperRel *)aElem2)->depInfo,1 ) )
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
}

IDE_RC
qmoOuterJoinOper::joinOperRelationing( qcStatement          * aStatement,
                                       qmoJoinOperGroup     * aJoinGroup )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Join Group 내의 Join Relation 구성
 *
 * Implementation :
 *    하나의 Join Group 을 argument 로 받아서 해당 Join Group 의
 *    joinPredicate 에 연결된 모든 노드들을 중복검사, Sorting 을 통해
 *    joinRelation 에 순서대로 저장한다.
 *
 ***********************************************************************/

    qcuSqlSourceInfo     sqlInfo;
    qmoJoinOperPred    * sCurPred         = NULL;
    qmoJoinOperRel     * sCurRel          = NULL;
    qmoJoinOperRel     * sConnectRel      = NULL;
    qmoJoinOperRel     * sStartRel        = NULL;
    qmoJoinOperRel     * sAddRelPosition  = NULL;
    qtcNode            * sErrNode         = NULL;
    idBool               sExist           = ID_FALSE;
    qmoJoinOperRel       sTempRel;
    qcDepInfo            sAndDependencies;
    qcDepInfo            sConnectAndDep;
    qcDepInfo            sTotalRelDepOR;
    qcDepInfo            sCounterDep;
    UInt                 i, j;
    UChar                sJoinOper;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::joinOperRelationing::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aJoinGroup != NULL );

    //------------------------------------------
    // 참고>
    // aJoinGroup->joinPredicate 에 연결되어 있는
    // 모든 Predicate 은 dependency table 을 2개,
    // Outer Join operator 을 1 개 가지고 있다는 것이보장된다.
    //
    // 그렇지 않은 것들은 이미 Predicate 별 제약사항 검사
    // 과정에서 걸러진 상태이다.
    //------------------------------------------

    for ( sCurPred  = aJoinGroup->joinPredicate;
          sCurPred != NULL;
          sCurPred  = sCurPred->next )
    {
        //------------------------------------------
        // 이미 joinRelation 에 추가된 것과 depInfo 가 중복되는지 검사
        //------------------------------------------

        for ( i = 0 ; i < aJoinGroup->joinRelationCnt ; i++ )
        {
            sCurRel = (qmoJoinOperRel *)&aJoinGroup->joinRelation[i];

            if ( qtc::dependencyJoinOperEqual( & sCurRel->depInfo,
                                               & sCurPred->node->depInfo )
                 == ID_TRUE )
            {
                sExist = ID_TRUE;
                break;
            }
            else
            {
                // 중복된 Predicate 은 Relation 에 미포함
                sExist = ID_FALSE;
            }
        }

        if ( sExist == ID_FALSE )
        {
            IDE_FT_ASSERT( aJoinGroup->joinRelationCnt < aJoinGroup->joinRelationCntMax );

            //------------------------------------------
            // 새로운 Predicate 을 Relation 에 추가하고
            // dependency 정보도 Oring 추가
            //------------------------------------------
            sAddRelPosition = (qmoJoinOperRel *)&aJoinGroup->joinRelation[aJoinGroup->joinRelationCnt];
            qtc::dependencySetWithDep( & sAddRelPosition->depInfo, & sCurPred->node->depInfo );
            sAddRelPosition->node = sCurPred->node;

            ++aJoinGroup->joinRelationCnt;
        }
        else
        {
            // Nothing to do.
        }
    }

    //------------------------------------------
    // joinRelation 에 대한 Predicate 의 추가가 완료되었으므로
    // joinRelation 내에서의 table 값에 따라 정렬을 수행한다.
    // 정렬을 수행하는 이유는 뒤에서 변환수행 시
    // From 을 Left Outer Join Tree 로 변경하게 되는데,
    // 이 때 노드들의 연결점을 찾을 수 있도록 하기 위함이다.
    //
    //    정렬전
    //    (2,3) -> (1,3) -> (2,5) -> (3,4) -> (1,2)
    //
    //    정렬후
    //    (1,2) -> (1,3) -> (2,3) -> (2,5) -> (3,4)
    //------------------------------------------

    sStartRel = (qmoJoinOperRel *)&aJoinGroup->joinRelation[0];

    if ( aJoinGroup->joinRelationCnt > 1 )
    {
        idlOS::qsort( sStartRel,
                      aJoinGroup->joinRelationCnt,
                      ID_SIZEOF(qmoJoinOperRel),
                      compareJoinOperRel );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // sTotalRelDepOR 는 아래에서 차례대로 구성되는
    // Relation 들의 depInfo 정보를 차례로 Oring 해서
    // 가지고 있기 위한 것이다.
    //------------------------------------------
    qtc::dependencyClear( & sTotalRelDepOR );

    //------------------------------------------
    // Relation 은 중요한 필수조건이 하나 있다.
    // 같은 Join Group 내의 각 Relation 들은 반드시 연결점이 있다는 것이다.
    // 각 조건간에 연결점이 없다면 같은 Join Group 내에 포함될 수
    // 없을 것이다.
    // 이와 더불어 joinRelation 은 위에서와 같이 qsort 를 통해
    // 정렬되어 있기만 하면 안된다.
    // 각 멤버가 바로 앞 멤버와 연결점을 가져야 추후 변환과정에서
    // From Tree 를 만들 수 있다.
    //
    // 즉, 다음의 경우를 생각해보자.
    //    _      _    _
    // (1,4)->(2,3)->(2,4)
    //
    // qsort 후의 특정 Join Group 의 Relation 이 위와 같다고 할 때,
    // 이를 그대로 이용하면 두가지 문제점이 있다.
    //         _        _
    //   1. (1,4) 와 (2,3) 이 tuple id 에 의한 연결점이 없으므로
    //      추후 From Tree 를 구성할 때 연결점을 찾을 수 없어서
    //      ANSI 형태로 변환이 실패한다.
    //   2. 아래쪽에서 Join Looping 을 정확하게 검사하기 위해서는
    //      연결점을 고려한 정렬이 되어 있어야 한다.
    //                     _
    //      그렇지 않으면 (2,4) predicate 에 대해 검사를 할 때,
    //       _                 _
    //      (2,4) 의 반대인 (2,4) 가 이미 sTotalRelDepOR 에 있기 때문에
    //      에러를 발생시킨다. (정상적인 쿼리인데도 말이다.)
    //
    // 따라서, 위의 경우는 아래와 같이 정렬되어 있어야 한다.
    // (1,4) 와 (2,4)의 연결점은 4 이고, (2,4)와 (2,3)의 연결점은 2 이다.
    //    _    _        _
    // (1,4)->(2,4)->(2,3)
    //
    // 이렇게 바로 앞의 Relation 노드와 연결할 수 있는 것들은 모두
    // 이에 근거해서 정렬해두고, 그 다음 나머지들은 qsort 에 의해
    // 정렬된대로 두면 된다.
    //------------------------------------------
    for ( i = 0 ; i < aJoinGroup->joinRelationCnt ; i++ )
    {
        sCurRel = (qmoJoinOperRel *)&aJoinGroup->joinRelation[i];

        if ( i+1 < aJoinGroup->joinRelationCnt )
        {
            IDE_FT_ASSERT( i+1 < aJoinGroup->joinRelationCntMax );

            sConnectRel = (qmoJoinOperRel *)&aJoinGroup->joinRelation[i+1];

            qtc::dependencyAnd( & sCurRel->depInfo,
                                & sConnectRel->depInfo,
                                & sConnectAndDep );

            if( qtc::dependencyEqual( & sConnectAndDep,
                                      & qtc::zeroDependencies )
                == ID_TRUE )
            {

                if ( i+2 < aJoinGroup->joinRelationCnt )
                {
                    for ( j = i+2 ; j < aJoinGroup->joinRelationCnt ; j++ )
                    {
                        IDE_FT_ASSERT( i+2 < aJoinGroup->joinRelationCntMax );

                        sConnectRel = (qmoJoinOperRel *)&aJoinGroup->joinRelation[i+2];

                        qtc::dependencyAnd( & sCurRel->depInfo,
                                            & sConnectRel->depInfo,
                                            & sConnectAndDep );

                        if( qtc::dependencyEqual( & sConnectAndDep,
                                                  & qtc::zeroDependencies )
                            == ID_FALSE )
                        {
                            idlOS::memcpy( (void*) &sTempRel,
                                           (void*) &aJoinGroup->joinRelation[i+1],
                                           ID_SIZEOF(qmoJoinOperRel) );

                            idlOS::memcpy( (void*) &aJoinGroup->joinRelation[i+1],
                                           (void*) &aJoinGroup->joinRelation[j],
                                           ID_SIZEOF(qmoJoinOperRel) );

                            idlOS::memcpy( (void*) &aJoinGroup->joinRelation[j],
                                           (void*) &sTempRel,
                                           ID_SIZEOF(qmoJoinOperRel) );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }

        //------------------------------------------
        // 다음과 같이 하나의 테이블이 2 개 이상의 테이블과
        // Outer Join 되는 경우 에러
        //
        // where t1.i1(+) = t2.i1
        //   and t1.i1(+) = t3.i1
        //
        // 다음의 예를 보자.
        //  _        _    _      _
        // (1,2)->(1,5)->(2,3)->(4,5)
        //
        // 현재까지 sTotalRelDepOR 에 Oring 된 Predicate 이 위와 같다면,
        // 이 때까지의 sTotalRelDepOR 결과는 다음과 같을 것이다.
        // (F:QMO_JOIN_OPER_FALSE, T:QMO_JOIN_OPER_TRUE, B:QMO_JOIN_OPER_BOTH)
        //
        //   1  2  3  4  5
        //   B  B  F  T  B
        //         _
        // 여기에 (1,6) 을 추가하려고 하면 에러가 나야 한다.
        //
        //   1  2  3  4  5  (AND)  1  6  = 1
        //   B  B  F  T  B         T  F    T(B)
        //              _
        // 위의 결과는 (1) 이다.
        // 이런 결과가 나오면 이는 한테이블이 2 개 이상의 테이블과
        // Outer Join 관계에 있다는 것이므로 에러이다.
        //
        // 즉, 앞서 나온 predicate 에서는 1 에 대해 outer join operator 가
        // 붙어있는게 있었는데, 6 을 포함한 predicate 은 없었다는 의미가 되므로
        // 결국 t1.i1(+)=t3.i1 and t1.i1(+)=t6.i1 처럼 t1.i1(+) 이 두개의
        // 테이블에 outer join 되고 있다는 의미이다.
        //------------------------------------------

        qtc::dependencyClear( & sAndDependencies );

        qtc::dependencyJoinOperAnd( & sTotalRelDepOR,
                                    & sCurRel->depInfo,
                                    & sAndDependencies );

        if( qtc::dependencyEqual( & sAndDependencies,
                                  & qtc::zeroDependencies )
            == ID_FALSE )
        {
            sJoinOper = qtc::getDependJoinOper( &sAndDependencies, 0 );

            if ( QMO_JOIN_OPER_EXISTS( sJoinOper ) == ID_TRUE )
            {
                sErrNode = (qtcNode *) sCurRel->node->node.arguments;

                sqlInfo.setSourceInfo ( aStatement, & sErrNode->position );
                IDE_RAISE( ERR_ABORT_MAYBE_OUTER_JOINED_TO_AT_MOST_ONE_OTHER_TABLE );
            }
            else
            {
                // Nothing to do.
            }
        }

        //------------------------------------------
        // ## Join Looping
        //
        // 다음과 같이 Outer Join 관계가 looping 되어 결국 자신과
        // Outer Join 될 경우도 에러를 발생시켜야 한다.
        //
        // where t1.i1(+) = t2.i1
        //   and t2.i1(+) = t1.i1
        //
        // where t1.i1(+) = t2.i1
        //   and t2.i1(+) = t3.i1
        //   and t3.i1(+) = t1.i1
        //
        // 이런 경우는 어떻게 추출할까 ?
        //  _        _    _ 
        // (1,2)->(1,5)->(2,3)
        //
        // 위와 같은 Relation 에 대해 sTotalRelDepOR 에 ORing 된 상태일 때,
        // 다음의 Current Relation 에 대해 제약사항을 검사하려고 한다.
        //  _
        // (4,5)
        //
        // 과연 정상일까 ? 에러일까 ?
        //
        // 이 판단은 이전에 모두 Oring 되어 있는 sTotalRelDepOR 에
        //      _                            _
        // 4 와 5 가 존재하느냐로 판단한다. (4 와 5 의 반대)
        // 즉, Current Relation node 의 dependency table 들에 대해 (+) 의
        // 유무가 반대 상태인 것이 sTotalRelDepOR 에 존재하면 에러이다.
        //
        // 이는 다음과 같이 연산을 통해 확인하면 된다.
        //
        //                      (가)    (나)
        //   1  2  3  5  (AND)  4  5  =  5
        //   B  B  F  T         F  T     T
        //
        // 위에서 (가)와 (나)의 결과가 같으면 에러이다.
        // 위의 경우는 다르므로 정상적이라고 볼 수 있다.
        //                               _
        // 그럼 만약 다음 Relation node (3,5) 는 어떨까 ?
        //
        //                         (가)     (나)
        //   1  2  3  4  5  (AND)  3  5  =  3  5
        //   B  B  F  T  B         F  T     F  T
        //
        // 이 때는 (가)와 (나)의 결과가 같으므로 에러이다.
        //------------------------------------------

        qtc::getJoinOperCounter( &sCounterDep, &sCurRel->depInfo );

        qtc::dependencyClear( & sAndDependencies );

        qtc::dependencyJoinOperAnd( & sTotalRelDepOR,
                                    & sCounterDep,
                                    & sAndDependencies );

        if ( qtc::dependencyJoinOperEqual( & sAndDependencies,
                                           & sCounterDep )
             == ID_TRUE )
        {
            sErrNode = (qtcNode *) sCurRel->node->node.arguments;

            sqlInfo.setSourceInfo ( aStatement, & sErrNode->position );
            IDE_RAISE( ERR_ABORT_NOT_ALLOW_OUTER_JOIN_EACH_OTHER );
        }

        //------------------------------------------
        // 위의 제약사항들을 relation node 별로 검사하기 위해
        // 앞에서부터 차례로 각 node 의 dependency 정보를
        // sTotalRelDepOR 에 Oring 해둔다.
        //------------------------------------------
        IDE_TEST( qtc::dependencyOr( & sTotalRelDepOR,
                                     & sCurRel->depInfo,
                                     & sTotalRelDepOR )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_NOT_ALLOW_OUTER_JOIN_EACH_OTHER )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_ALLOW_OUTER_JOIN_EACH_OTHER,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_ABORT_MAYBE_OUTER_JOINED_TO_AT_MOST_ONE_OTHER_TABLE )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_MAYBE_OUTER_JOINED_TO_AT_MOST_ONE_OTHER_TABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::joinOperOrdering( qcStatement      * aStatement,
                                    qmoJoinOperTrans * aTrans )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Join Order의 결정
 *
 * Implemenation :
 *    (1) Join Group의 분류
 *    (2) 각 Join Group 내 Join Relation의 표현
 *    (3) 각 Join Group 내 Join Order의 결정
 *
 ***********************************************************************/

    qmoJoinOperGroup * sJoinGroup = NULL;
    UInt               sJoinGroupCnt;
    UInt               i;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::joinOperOrdering::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTrans     != NULL );


    //------------------------------------------
    // Join Grouping의 분류
    //------------------------------------------

    IDE_TEST( joinOperGrouping( aTrans ) != IDE_SUCCESS );

    sJoinGroup       = aTrans->joinOperGroup;
    sJoinGroupCnt    = aTrans->joinOperGroupCnt;

    for ( i = 0; i < sJoinGroupCnt; i++ )
    {
        //------------------------------------------
        // Join Group 하나씩을 전달하여
        // Join Group 내의 joinPredicate 에 연결된
        // 모든 Predicate 들을 중복검사, Sort 를 수행하여
        // joinRelation 에 연결한다.
        //------------------------------------------

        IDE_TEST( joinOperRelationing( aStatement, & sJoinGroup[i] )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::isAvailableJoinOperRel( qcDepInfo  * aDepInfo,
                                          idBool     * aIsTrue )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Outer Join Operator 를 가진 Join Relation 의 유효성검사
 *
 * Implemenation :
 *    Join Relation 의 dependency 정보가 유효하려면
 *    dependency table 은 2 개이면서 한쪽에만 (+)가
 *    있어야 한다.
 *
 ***********************************************************************/

    SInt   sJoinOperCnt;
    SInt   sDepCount;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::isAvailableJoinOperRel::__FT__" );

    sJoinOperCnt = qtc::getCountJoinOperator( aDepInfo );
    sDepCount    = qtc::getCountBitSet( aDepInfo );

    if ( ( sDepCount == QMO_JOIN_OPER_DEPENDENCY_TABLE_CNT_PER_PRED )
      && ( sJoinOperCnt == QMO_JOIN_OPER_TABLE_CNT_PER_PRED ) )
    {
        *aIsTrue = ID_TRUE;
    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoOuterJoinOper::movePred2OnCondition( qcStatement      * aStatement,
                                        qmoJoinOperTrans * aTrans,
                                        qmsFrom          * aNewFrom,
                                        qmoJoinOperRel   * aJoinGroupRel,
                                        qmoJoinOperPred  * aJoinGroupPred )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : onCondition 절 작성
 *
 * Implemenation :
 *   joinPredicate 과 oneTablePred 중에서 aNewFrom 과 관련있는 것을
 *   aNewFrom 의 onCondition 에 연결한다.
 *   각각 joinPredicate 과 oneTablePred list 에서 제거한 후
 *   제거한 노드가 가리키는 normalCNF 노드를 분리하여 onCondition 에 연결한다.
 *
 ***********************************************************************/

    qmoJoinOperPred  * sCurStart       = NULL;
    qmoJoinOperPred  * sCur            = NULL;
    qmoJoinOperPred  * sPrev           = NULL;
    qmoJoinOperPred  * sAdd            = NULL;
    qtcNode          * sNorStart       = NULL;
    qtcNode          * sNorCur         = NULL;
    qtcNode          * sNorPrev        = NULL;
    qtcNode          * sCurOnCond      = NULL;

    qcDepInfo          sAndDependencies;
    qcNamePosition     sNullPosition;
    qtcNode          * sTmpNode[2];
    qtcNode          * sANDNode        = NULL;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::movePred2OnCondition::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aNewFrom   != NULL );
    IDE_DASSERT( aJoinGroupRel   != NULL );
    IDE_DASSERT( aJoinGroupPred  != NULL );


    //------------------------------------------------------
    // qmoJoinOperTrans->joinOperGroup->joinPredicate 에서 aNewFrom 에
    // 합쳐진 것과 depInfo 가 같은 node 를 찾아서, 이것이 가리키는
    // nomalizedCNF node 를 찾아서 aNewFrom 의 onCondition 에
    // 연결한 후 normalCNF 에서는 제거한다.
    //------------------------------------------------------

    sCurStart = aJoinGroupPred;

    for ( sCur = aJoinGroupPred ;
          sCur != NULL ;
          sPrev = sCur, sCur = (qmoJoinOperPred *)sCur->next )
    {
        if ( qtc::dependencyJoinOperEqual( & sCur->node->depInfo,
                                           & aJoinGroupRel->depInfo )
             == ID_FALSE )
        {
            continue;
        }

        if ( sCur == sCurStart )
        {
            sCurStart = sCur->next;
            // aTrans->joinOperGroup->joinPredicate = sCur->next;
        }
        else
        {
            sPrev->next = sCur->next;
        }

        sAdd = sCur;

        sNorStart = (qtcNode *)aTrans->normalCNF->node.arguments;

        for ( sNorCur = (qtcNode *)aTrans->normalCNF->node.arguments ;
              sNorCur != NULL ;
              sNorPrev = sNorCur,sNorCur = (qtcNode *)sNorCur->node.next )
        {
            if ( sNorCur == sAdd->node )
            {
                if ( sNorCur == sNorStart )
                {
                    if ( sNorCur->node.next != NULL )
                    {
                        aTrans->normalCNF->node.arguments = sNorCur->node.next;
                    }
                    else
                    {
                        aTrans->normalCNF = NULL;
                    }

                    sNorCur->node.next = NULL;
                }
                else
                {
                    sNorPrev->node.next = sNorCur->node.next;
                    sNorCur->node.next = NULL;
                }

                if ( aNewFrom->onCondition == NULL )
                {
                    //------------------------------------------------------
                    // 떼어놓은 normalCNF 의 predicate 노드를 처음으로
                    // aNewFrom 노드의 onCondition 에 연결
                    //------------------------------------------------------

                    aNewFrom->onCondition = (qtcNode *)sNorCur->node.arguments;

                    //------------------------------------------------------
                    // 다음 연결을 위해 onCondition 의 마지막을 기억해둔다.
                    //------------------------------------------------------

                    sCurOnCond = aNewFrom->onCondition;
                }
                else
                {
                    //------------------------------------------------------
                    // onCondition 에 두개 이상의 노드가 달리게 되는 경우는
                    // AND 노드를 추가한 후 연결해줘야 한다.
                    // aNewFrom->onCondition 이 가리키는 노드가 AND 노드가
                    // 아닐 경우에만 해주면 된다.
                    //------------------------------------------------------

                    if ( aNewFrom->onCondition->node.module != &mtfAnd )
                    {
                        //------------------------------------------------------
                        // AND 노드를 생성한다.
                        //------------------------------------------------------
                        SET_EMPTY_POSITION(sNullPosition);

                        IDE_TEST( qtc::makeNode( aStatement,
                                                 sTmpNode,
                                                 & sNullPosition,
                                                 & mtfAnd )
                                  != IDE_SUCCESS);

                        sANDNode = sTmpNode[0];

                        // sANDNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
                        // sANDNode->node.lflag |= 1;

                        //------------------------------------------------------
                        // 생성한 AND 노드를 onCondition 의 맨처음에 끼워넣는다.
                        //------------------------------------------------------

                        sANDNode->node.arguments = (mtcNode *)aNewFrom->onCondition;
                        aNewFrom->onCondition = sANDNode;

                        //------------------------------------------------------
                        // sCurOnCond 는 이전에 추가된 노드를 기억하고 있으므로
                        // 그 뒤에 연결해주고 다시 마지막을 기억한다.
                        //------------------------------------------------------

                        sCurOnCond->node.next = sNorCur->node.arguments;
                        sCurOnCond = (qtcNode *)sCurOnCond->node.next;
                    }
                    else
                    {
                        //------------------------------------------------------
                        // 기억해둔 onCondition 의 마지막에 추가로 연결한다.
                        // 그리고, 추가로 연결한 노드를 다시 기억해둔다.
                        //------------------------------------------------------

                        sCurOnCond->node.next = sNorCur->node.arguments;
                        sCurOnCond = (qtcNode *)sCurOnCond->node.next;
                    }
                }

                break;
            }
        }
    }

    //------------------------------------------------------
    // qmoJoinOperTrans->oneTablePred 에서도 (+)가 붙은 같은 dependency table 을
    // 가진 Predicate 을 찾아서 제거한 후 onCondition 에 연결한다.
    // nomalizedCNF 에서도 해당 Predicate 를 찾아서 제거한 후
    // 이를 aNewFrom 의 onCondition 에 연결한다.
    //                                           _
    // 즉, Left Outer Join 의 dependency 정보가 (1,2) 라면,
    //  _
    // (1) 을 가진 one table predicate 노드만 해당 left outer join 의
    // on condition 에 연결한다.
    //------------------------------------------------------

    sCurStart = aTrans->oneTablePred;

    for ( sCur = aTrans->oneTablePred ;
          sCur != NULL ;
          sPrev = sCur, sCur = (qmoJoinOperPred *)sCur->next )
    {
        //------------------------------------------------------
        // 반드시 depend 정보 뿐 아니라 Outer Join Operator 여부도
        // 같은 것이 있는지를 찾아야 한다.
        //
        // where t1.i1(+) = t2.i1
        //   and t2.i1(+) = 1  <- oneTablePred
        //
        // 위와 같은 경우는 oneTablePred 에 있는 것은 변환 후에도
        // 그대로 normalCNF 에 남아있어야 한다.
        // onCondition 에 연결하면 결과가 달라진다.
        //------------------------------------------------------

        qtc::dependencyJoinOperAnd( & sCur->node->depInfo,
                                    & aJoinGroupRel->depInfo,
                                    & sAndDependencies );

        if( qtc::dependencyEqual( & sAndDependencies,
                                  & qtc::zeroDependencies )
            == ID_TRUE )
        {
            continue;
        }

        if ( sCur == sCurStart )
        {
            aTrans->oneTablePred = sCur->next;
        }
        else
        {
            sPrev->next = sCur->next;
        }

        sAdd = sCur;

        sNorStart = (qtcNode *)aTrans->normalCNF->node.arguments;

        for ( sNorCur = (qtcNode *)aTrans->normalCNF->node.arguments ;
              sNorCur != NULL ;
              sNorPrev = sNorCur, sNorCur = (qtcNode *)sNorCur->node.next )
        {
            if ( sNorCur == sAdd->node )
            {
                if ( sNorCur == sNorStart )
                {
                    if ( sNorCur->node.next != NULL )
                    {
                        aTrans->normalCNF->node.arguments = sNorCur->node.next;
                    }
                    else
                    {
                        aTrans->normalCNF = NULL;
                    }

                    sNorCur->node.next = NULL;
                }
                else
                {
                    sNorPrev->node.next = sNorCur->node.next;
                    sNorCur->node.next = NULL;
                }

                if ( aNewFrom->onCondition == NULL )
                {
                    //------------------------------------------------------
                    // 떼어놓은 normalCNF 의 predicate 노드를 처음으로
                    // aNewFrom 노드의 onCondition 에 연결
                    //------------------------------------------------------

                    aNewFrom->onCondition = sNorCur;

                    //------------------------------------------------------
                    // 다음 연결을 위해 onCondition 의 마지막을 기억해둔다.
                    //------------------------------------------------------

                    sCurOnCond = aNewFrom->onCondition;
                }
                else
                {
                    //------------------------------------------------------
                    // onCondition 에 두개 이상의 노드가 달리게 되는 경우는
                    // AND 노드를 추가한 후 연결해줘야 한다.
                    // aNewFrom->onCondition 이 가리키는 노드가 AND 노드가
                    // 아닐 경우에만 해주면 된다.
                    //------------------------------------------------------

                    if ( aNewFrom->onCondition->node.module != &mtfAnd )
                    {
                        //------------------------------------------------------
                        // AND 노드를 생성한다.
                        //------------------------------------------------------
                        SET_EMPTY_POSITION(sNullPosition);

                        IDE_TEST( qtc::makeNode( aStatement,
                                                 sTmpNode,
                                                 & sNullPosition,
                                                 & mtfAnd )
                                  != IDE_SUCCESS);

                        sANDNode = sTmpNode[0];

                        // sANDNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
                        // sANDNode->node.lflag |= 1;

                        //------------------------------------------------------
                        // 생성한 AND 노드를 onCondition 의 맨처음에 끼워넣는다.
                        //------------------------------------------------------

                        sANDNode->node.arguments = (mtcNode *)aNewFrom->onCondition;
                        aNewFrom->onCondition = sANDNode;

                        //------------------------------------------------------
                        // sCurOnCond 는 이전에 추가된 노드를 기억하고 있으므로
                        // 그 뒤에 연결해주고 다시 마지막을 기억한다.
                        //------------------------------------------------------

                        sCurOnCond->node.next = (mtcNode*)sNorCur;
                        sCurOnCond = (qtcNode *)sCurOnCond->node.next;
                    }
                    else
                    {
                        //------------------------------------------------------
                        // 기억해둔 onCondition 의 마지막에 추가로 연결한다.
                        // 그리고, 추가로 연결한 노드를 다시 기억해둔다.
                        //------------------------------------------------------

                        sCurOnCond->node.next = (mtcNode*)sNorCur;
                        sCurOnCond = (qtcNode *)sCurOnCond->node.next;
                    }
                }

                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    //------------------------------------------------------
    // 구조가 변경된 onCondition 과 normalCNF 에 대해 estimate
    //------------------------------------------------------

    if ( aNewFrom->onCondition != NULL )
    {
        if ( aNewFrom->onCondition->node.module == &mtfAnd )
        {
            IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                     (qtcNode *)( aNewFrom->onCondition ) )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( aTrans->normalCNF != NULL )
    {
        IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                 (qtcNode *)( aTrans->normalCNF ) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::transJoinOperBetweenStyle( qcStatement * aStatement,
                                             qmsQuerySet * aQuerySet,
                                             qtcNode     * aInNode,
                                             qtcNode    ** aTransStart,
                                             qtcNode    ** aTransEnd )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 * Implementation :
 *
 *    BETWEEN Style
 *    ======================================================
 *    mtfModule          : mtcName       : source  : target
 *    ======================================================
 *    mtfBetween         : BETWEEN       :   1     :   2 (not list)
 *    mtfNotBetween      : NOT BETWEEN   :   1     :   2 (not list)
 *    ======================================================
 *
 *    between 의 경우는 Source 와 Target 에 list 가 올 수 없다.
 *
 *   (1) mtfBetween
 *
 *       ** 변환 전
 *       a1 between a2 and a3
 *
 *        (AND)
 *          |
 *          V
 *        (OR)  <- aORNode
 *          |
 *          V
 *      (Between) <- aInNode
 *          |
 *          V
 *        (a1) -> (a2) -> (a3)
 *
 *       ** 변환 후
 *       a1 >= a2 and a1 <= a3
 *
 *        (AND)
 *          |  _______ aTransStart ______ aTransEnd
 *          V /                   /
 *        (OR)       ---->    (OR)
 *          |                   |
 *          V                   V
 *     (GreaterEqual)      (LessEqual)
 *          |                   |
 *          V                   V
 *        (a1) -> (a2)        (a1) -> (a3)
 *
 *
 *   (2) mtfNotBetween
 *
 *       ** 변환 전
 *       a1 not between a2 and a3
 *
 *        (AND)
 *          |
 *          V
 *        (OR)
 *          |
 *          V
 *     (NotBetween)
 *          |
 *          V
 *        (a1) -> (a2) -> (a3)
 *
 *       ** 변환 후
 *       a1 < a2 or a1 > a3
 *
 *        (AND)
 *          |
 *          V
 *        (OR)
 *          |
 *          V
 *     (LessThan)   --->  (GreaterThan)
 *          |                   |
 *          V                   V
 *        (a1) -> (a2)        (a1) -> (a3)
 ***********************************************************************/

    qtcNode       * sOldSrc        = NULL;
    qtcNode       * sOldTrg        = NULL;
    qtcNode       * sOldTrgStart   = NULL;
    qtcNode       * sNewSrc        = NULL;
    qtcNode       * sNewTrg        = NULL;

    qtcNode       * sORNode        = NULL;
    qtcNode       * sOperNode      = NULL;

    qtcNode       * sPredStart     = NULL;
    qtcNode       * sPredEnd       = NULL;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::transJoinOperBetweenStyle::__FT__" );

    IDE_DASSERT( aStatement  != NULL );
    IDE_DASSERT( aQuerySet   != NULL );
    IDE_DASSERT( aInNode     != NULL );
    IDE_DASSERT( aTransStart != NULL );
    IDE_DASSERT( aTransEnd   != NULL );

    if ( aInNode->node.module == & mtfBetween )
    {
        //---------------------------------------------
        // Between 연산자의 Argument 를 순회하며
        // src = trg1 and src = trg2 와 같은 형태로
        // predicate 을 분리한다.
        // 새로 구성하는 Predicate 의 구성노드들은
        // 신규로 생성하여 사용한다.
        //---------------------------------------------

        sOldSrc = (qtcNode *)aInNode->node.arguments;
        sOldTrgStart = (qtcNode *)sOldSrc->node.next;

        for ( sOldTrg  = (qtcNode *)sOldTrgStart;
              sOldTrg != NULL;
              sOldTrg  = (qtcNode *)sOldTrg->node.next )
        {
            //---------------------------------------------
            // 첫번째 Target 노드일 때
            //---------------------------------------------
            if ( sOldTrg == sOldTrgStart )
            {
                IDE_TEST( makeNewORNodeTree( aStatement,
                                             aQuerySet,
                                             aInNode,
                                             sOldSrc,
                                             sOldTrg,
                                             & sNewSrc,
                                             & sNewTrg,
                                             & mtfGreaterEqual,
                                             & sORNode,
                                             ID_TRUE )
                          != IDE_SUCCESS);
            }
            //---------------------------------------------
            // 두번째 Target 노드일 때
            // Target Node 는 두개밖에 없다.
            //---------------------------------------------
            else
            {
                IDE_TEST( makeNewORNodeTree( aStatement,
                                             aQuerySet,
                                             aInNode,
                                             sOldSrc,
                                             sOldTrg,
                                             & sNewSrc,
                                             & sNewTrg,
                                             & mtfLessEqual,
                                             & sORNode,
                                             ID_TRUE )
                          != IDE_SUCCESS);
            }

            if ( sOldTrg == (qtcNode *)sOldSrc->node.next )
            {
                sPredStart = sORNode;
            }
            else
            {
                IDE_DASSERT( sPredStart != NULL );

                sPredEnd = sORNode;
                sPredStart->node.next = (mtcNode *)sORNode;
            }

            /****************************************************
             * 다음과 같은 신규 Tree 가 완성되었다.
             *
             *             _______ sPredStart  _______ sPredEnd
             *            /                   /
             *        (OR)       ---->    (OR)
             *          |                   |
             *          V                   V
             *     (GreaterEqual)      (LessEqual)
             *          |                   |
             *          V                   V
             *        (a1) -> (a2)        (a1) -> (a3)
             ****************************************************/
        }
    }
    else if ( aInNode->node.module == & mtfNotBetween )
    {

        sOldSrc = (qtcNode *)aInNode->node.arguments;
        sOldTrgStart = (qtcNode *)sOldSrc->node.next;

        for ( sOldTrg  = sOldTrgStart;
              sOldTrg != NULL;
              sOldTrg  = (qtcNode *)sOldTrg->node.next )
        {
            //---------------------------------------------
            // 첫번째 Target 노드일 때
            //---------------------------------------------
            if ( sOldTrg == sOldTrgStart )
            {
                IDE_TEST( makeNewORNodeTree( aStatement,
                                             aQuerySet,
                                             aInNode,
                                             sOldSrc,
                                             sOldTrg,
                                             & sNewSrc,
                                             & sNewTrg,
                                             & mtfLessThan,
                                             & sORNode,
                                             ID_FALSE )
                          != IDE_SUCCESS);
            }
            //---------------------------------------------
            // 두번째 Target 노드일 때.
            // (Target Node 는 두개밖에 없다)
            //---------------------------------------------
            else
            {
                IDE_TEST( makeNewOperNodeTree( aStatement,
                                               aInNode,
                                               sOldSrc,
                                               sOldTrg,
                                               & sNewSrc,
                                               & sNewTrg,
                                               & mtfGreaterThan,
                                               & sOperNode )
                          != IDE_SUCCESS);

                sORNode->node.arguments->next = (mtcNode *)sOperNode;
                sOperNode->node.next = NULL;
            }

            //---------------------------------------------
            // OR 노드가 한개이기 때문에 Start 와 End 가 같다.
            //---------------------------------------------
            if ( sOldTrg == (qtcNode *)sOldSrc->node.next )
            {
                sPredStart = sORNode;
            }
            else
            {
                sPredEnd = sORNode;
            }

            /****************************************************
             * 다음과 같은 신규 Tree 가 완성되었다.
             *
             *             _______ sPredStart, sPredEnd
             *            /
             *        (OR)
             *          |
             *          V
             *     (LessThan)   --->  (GreaterThan)
             *          |                   |
             *          V                   V
             *        (a1) -> (a2)        (a1) -> (a3)
             ****************************************************/
        }

        //---------------------------------------------
        // estimate 수행.
        // OR 노드는 하나이기 때문에 마지막에 한번만 수행.
        //---------------------------------------------
        IDE_TEST( qtc::estimate( sORNode,
                                 QC_SHARED_TMPLATE(aStatement),
                                 aStatement,
                                 aQuerySet,
                                 aQuerySet->SFWGH,
                                 NULL )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_RAISE( ERR_UNEXPECTED_MODULE );
    }

    //---------------------------------------------
    // 변환한 Tree 의 Start OR 노드와 End OR 노드를
    // 반환한다.
    //---------------------------------------------

    *aTransStart = sPredStart;
    *aTransEnd   = sPredEnd;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_MODULE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::transJoinOperBetweenStyle",
                                  "Invalid comparison module" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::getCompModule4List( mtfModule    *  aModule,
                                      mtfModule    ** aOutModule )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 * Implementation :
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::getCompModule4List::__FT__" );

    if ( ( aModule == & mtfEqual )
      || ( aModule == & mtfEqualAll )
      || ( aModule == & mtfEqualAny ) )
    {
        *aOutModule = &mtfEqual;
    }
    else if ( ( aModule == & mtfGreaterEqualAll )
           || ( aModule == & mtfGreaterEqualAny ) )
    {
        *aOutModule = &mtfGreaterEqual;
    }
    else if ( ( aModule == & mtfGreaterThanAll )
           || ( aModule == & mtfGreaterThanAny ) )
    {
        *aOutModule = &mtfGreaterThan;
    }
    else if ( ( aModule == & mtfLessEqualAll )
           || ( aModule == & mtfLessEqualAny ) )
    {
        *aOutModule = &mtfLessEqual;
    }
    else if ( ( aModule == & mtfLessThanAll )
           || ( aModule == & mtfLessThanAny ) )
    {
        *aOutModule = &mtfLessThan;
    }
    else if ( ( aModule == & mtfNotEqual )
           || ( aModule == & mtfNotEqualAll )
           || ( aModule == & mtfNotEqualAny ) )
    {
        *aOutModule = &mtfNotEqual;
    }
    else
    {
        IDE_RAISE( ERR_UNEXPECTED_MODULE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_MODULE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::getCompModule4List",
                                  "Invalid comparison module" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::makeNewORNodeTree( qcStatement  * aStatement,
                                     qmsQuerySet  * aQuerySet,
                                     qtcNode      * aInNode,
                                     qtcNode      * aOldSrc,
                                     qtcNode      * aOldTrg,
                                     qtcNode     ** aNewSrc,
                                     qtcNode     ** aNewTrg,
                                     mtfModule    * aCompModule,
                                     qtcNode     ** aORNode,
                                     idBool         aEstimate )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 * Implementation :
 *  다음과 같은 Tree 가 생긴다.
 *
 *        (OR)
 *          |
 *          V
 *       (Equal)
 *          |
 *          V
 *        (a1) -> (b1)
 ***********************************************************************/

    qtcNode       * sTmpNode[2];
    qtcNode       * sORNode     = NULL;
    qtcNode       * sOperNode   = NULL;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::makeNewORNodeTree::__FT__" );

    //---------------------------------------------
    // sOldSrc 노드가 Root 가 되는 Tree 노드들을
    // 통째로 복사한다.
    //---------------------------------------------
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                     aOldSrc,
                                     aNewSrc,
                                     ID_FALSE,
                                     ID_TRUE,
                                     ID_FALSE,
                                     ID_FALSE )
              != IDE_SUCCESS );

    //---------------------------------------------
    // sOldTrg 노드가 Root 가 되는 Tree 노드들을
    // 통째로 복사한다.
    //---------------------------------------------
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                     aOldTrg,
                                     aNewTrg,
                                     ID_FALSE,
                                     ID_TRUE,
                                     ID_FALSE,
                                     ID_FALSE )
              != IDE_SUCCESS );


    (*aNewSrc)->node.next = (mtcNode *)(*aNewTrg);
    (*aNewTrg)->node.next = NULL;

    //---------------------------------------------
    // 변환 연산자 노드 할당
    //---------------------------------------------
    IDE_TEST( qtc::makeNode( aStatement,
                             sTmpNode,
                             & aInNode->position,
                             aCompModule )
              != IDE_SUCCESS );


    //---------------------------------------------
    // 모든 변환 연산자가 argument 를 2 개 필요로 하므로
    // 셋팅한다.
    //---------------------------------------------

    sOperNode = sTmpNode[0];
    sOperNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
    sOperNode->node.lflag |= 2;

    sOperNode->node.arguments = (mtcNode *)(*aNewSrc);
    sOperNode->node.next = NULL;

    IDE_TEST( qtc::makeNode( aStatement,
                             sTmpNode,
                             & aInNode->position,
                             & mtfOr )
              != IDE_SUCCESS );

    //---------------------------------------------
    // OR 가 필요로 하는 argument 는 1 개
    //---------------------------------------------

    sORNode = sTmpNode[0];
    sORNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
    sORNode->node.lflag |= 1;

    sORNode->node.arguments = (mtcNode *)sOperNode;
    sORNode->node.next = NULL;

    //---------------------------------------------
    // estimate 수행
    //---------------------------------------------

    if ( aEstimate == ID_TRUE )
    {
        IDE_TEST( qtc::estimate( sORNode,
                                 QC_SHARED_TMPLATE(aStatement),
                                 aStatement,
                                 aQuerySet,
                                 aQuerySet->SFWGH,
                                 NULL )
                 != IDE_SUCCESS );
    }

    *aORNode = sORNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOuterJoinOper::makeNewOperNodeTree( qcStatement  * aStatement,
                                       qtcNode      * aInNode,
                                       qtcNode      * aOldSrc,
                                       qtcNode      * aOldTrg,
                                       qtcNode     ** aNewSrc,
                                       qtcNode     ** aNewTrg,
                                       mtfModule    * aCompModule,
                                       qtcNode     ** aOperNode )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 * Implementation :
 *  다음과 같은 Tree 가 생긴다.
 *
 *       (Oper)
 *          |
 *          V
 *        (a1) -> (b1)
 ***********************************************************************/

    qtcNode       * sTmpNode[2];
    qtcNode       * sOperNode   = NULL;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::makeNewOperNodeTree::__FT__" );

    //---------------------------------------------
    // sOldSrc 노드가 Root 가 되는 Tree 노드들을
    // 통째로 복사한다.
    //---------------------------------------------
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                     aOldSrc,
                                     aNewSrc,
                                     ID_FALSE,
                                     ID_TRUE,
                                     ID_FALSE,
                                     ID_FALSE )
              != IDE_SUCCESS );

    //---------------------------------------------
    // sOldTrg 노드가 Root 가 되는 Tree 노드들을
    // 통째로 복사한다.
    //---------------------------------------------
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                     aOldTrg,
                                     aNewTrg,
                                     ID_FALSE,
                                     ID_TRUE,
                                     ID_FALSE,
                                     ID_FALSE )
              != IDE_SUCCESS );


    (*aNewSrc)->node.next = (mtcNode *)(*aNewTrg);
    (*aNewTrg)->node.next = NULL;

    //---------------------------------------------
    // 변환 연산자 노드 할당
    //---------------------------------------------
    IDE_TEST( qtc::makeNode( aStatement,
                             sTmpNode,
                              & aInNode->position,
                             aCompModule )
              != IDE_SUCCESS);


    //---------------------------------------------
    // 모든 변환 연산자가 argument 를 2 개 필요로 하므로
    // 셋팅한다.
    //---------------------------------------------

    sOperNode = sTmpNode[0];
    sOperNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
    sOperNode->node.lflag |= 2;

    sOperNode->node.arguments = (mtcNode *)(*aNewSrc);
    sOperNode->node.next = NULL;

    *aOperNode = sOperNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::transJoinOperListArgStyle( qcStatement * aStatement,
                                             qmsQuerySet * aQuerySet,
                                             qtcNode     * aInNode,
                                             qtcNode    ** aTransStart,
                                             qtcNode    ** aTransEnd )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 * Implementation :
 *    이 함수에서는 다음의 연산자들의 변환을 수행한다.
 *
 *    LIST Argument Style
 *    ======================================================
 *    mtfModule          : mtcName       : source  : target
 *    ======================================================
 *    mtfEqual           : =             : list    :  list
 *    mtfEqualAll        : =ALL          : list    :  list
 *                                       :   1     :  list
 *    mtfEqualAny        : =ANY          : list    :  list
 *                                       :   1     :  list
 *    mtfGreaterEqualAll : >=ALL         :   1     :  list
 *    mtfGreaterEqualAny : >=ANY         :   1     :  list
 *    mtfGreaterThanAll  : >ALL          :   1     :  list
 *    mtfGreaterThanAny  : >ANY          :   1     :  list
 *    mtfLessEqualAll    : <=ALL         :   1     :  list
 *    mtfLessEqualAny    : <=ANY         :   1     :  list
 *    mtfLessThanAll     : <ALL          :   1     :  list
 *    mtfLessThanAny     : <ANY          :   1     :  list
 *    mtfNotEqual        : <>            : list    :  list
 *    mtfNotEqualAll     : <>ALL         : list    :  list
 *    mtfNotEqualAny     : <>ANY         : list    :  list
 *                                       :   1     :  list
 *    ======================================================
 *
 *
 *   아래와 같은 방법으로 변환을 수행하면 된다.
 *   몇가지 연산자만 예로 들어 변환해보자.
 *   Between Style 과 다른 점이라면 list 가 올 수 있다는 것이고,
 *   list 의 argument 가 여러개 올 수 있다는 점이다.
 *
 *   (1) mtfEqualAll
 *
 *       ** 변환 전
 *       (a1,a2) =ALL ((b1,b2))
 *
 *        (AND)
 *          |
 *          V
 *        (OR) <- aORNode
 *          |
 *          V
 *     (EqualAll) <- aInNode
 *          |
 *          V
 *       (list)   --->  (list)
 *          |              |
 *          V              V
 *        (a1) -> (a2)  (list) <= can't be more than 1 list
 *                         |
 *                         V
 *                       (b1) -> (b2)
 *
 *       ** 변환 후
 *       a1 = b1 and a2 = b2
 *
 *        (AND)
 *          |
 *          V
 *        (OR)    --->    (OR)
 *          |               |
 *          V               V
 *       (Equal)         (Equal)
 *          |               |
 *          V               V
 *        (a1) -> (a2)    (b1) -> (b2)
 *
 *
 *   (2) mtfEqualAny
 *
 *       ** 변환 전
 *       (a1,a2) =ANY ((b1,b2))
 *
 *        (AND)
 *          |
 *          V
 *        (OR)
 *          |
 *          V
 *     (EqualAny)
 *          |
 *          V
 *       (list)   --->  (list)
 *          |              |
 *          V              V
 *        (a1) -> (a2)  (list)
 *                         |
 *                         V
 *                       (b1) -> (b2)
 *
 *       ** 변환 후
 *       a1 = b1 and a2 = b2
 *
 *        (AND)
 *          |
 *          V
 *        (OR)    --->    (OR)
 *          |               |
 *          V               V
 *       (Equal)         (Equal)
 *          |               |
 *          V               V
 *        (a1) -> (a2)    (b1) -> (b2)
 *
 *
 *   (3) mtfGreaterEqualAll
 *
 *       ** 변환 전
 *       a1 >=ALL (b1,b2)
 *
 *        (AND)
 *          |
 *          V
 *        (OR)
 *          |
 *          V
 *   (GreaterEqualAll)
 *          |
 *          V
 *        (a1)  --->  (list)
 *                      |
 *                      V
 *                    (b1) -> (b2)
 *
 *       ** 변환 후
 *       a1 >= b1 and a1 >= b2
 *
 *        (AND)
 *          |
 *          V
 *        (OR)    --->    (OR)
 *          |               |
 *          V               V
 *   (GreaterEqual)  (GreaterEqual)
 *          |               |
 *          V               V
 *        (a1) -> (b1)    (a1) -> (b2)
 *
 *
 *   (4) mtfGreaterEqualAny
 *
 *       ** 변환 전
 *       a1 >=ANY (b1,b2)
 *
 *        (AND)
 *          |
 *          V
 *        (OR)
 *          |
 *          V
 *   (GreaterEqualAll)
 *          |
 *          V
 *        (a1)  --->  (list)
 *                      |
 *                      V
 *                    (b1) -> (b2)
 *
 *       ** 변환 후
 *       a1 >= b1 or a1 >= b2
 *
 *        (AND)
 *          |
 *          V
 *        (OR)
 *          |
 *          V
 *   (GreaterEqual) ->  (GreaterEqual)
 *          |                 |
 *          V                 V
 *        (a1) -> (b1)      (a1) -> (b2)
 *
 *
 *
 *   위의 변환들을 참고로 하여 변환 규칙을 만들어보자.
 *   크게 ANY 와 ALL Type 으로 나누어 생각해볼 수 있다.
 *
 *   (1) ANY Type
 *
 *   ANY 는 List argument 들간의 비교를 OR 형태로 묶은 것이다.
 *   Source 가 List 일 때와 List 가 아닐때를 구분해서 생각해야 한다.
 *
 *   변환전
 *   가. (a1,a2) =any ((b1,b2),(c1,c2))
 *   나. (a1,a2) =any ((b1,b2)))
 *   다.  a1     =any (b1,b2)
 *
 *   변환후
 *   가. (a1=b1 and a2=b2) or (a1=c1 and a2=c2)
 *   나. (a1=b1 and a2=b2)
 *   다. (a1=b1) or (a1=b2)
 *
 *   위의 경우중 (가)의 경우는 고려사항에서 제외해도 된다.
 *   왜냐하면, 이미 앞에서 (+) 사용 시에 Source 쪽이 List 이면
 *   Target 쪽은 2 개 이상의 List 를 사용할 수 없도록 제약사항으로
 *   걸러냈기 때문이다.
 *
 *   따라서, =any, >=any, <>any 등 모두 두가지 변환형태로 압축할 수 있다.
 *   하나는 (pred) and (pred) 이고, 하나는 (pred) or (pred) 형태이다.
 *
 *         (OR)          (OR)
 *          |              |
 *          V              V
 *      (=,>=,<>)  ->  (=,>=,<>)
 *          |              |
 *          V              V
 *        (a1) -> (b1)   (a2) -> (b2)
 *
 *
 *         (OR)
 *          |
 *          V
 *      (=,>=,<>)  ->  (=,>=,<>)
 *          |              |
 *          V              V
 *        (a1) -> (b1)   (a1) -> (b2)
 *
 *   (2) ALL Type
 *
 *   ALL 은 List argument 들간의 비교를 AND 형태로 묶은 것이다.
 *
 *   변환전
 *   가. (a1,a2) =all ((b1,b2),(c1,c2))
 *   나. (a1,a2) =all ((b1,b2)))
 *   다.  a1     =all (b1,b2)
 *
 *   변환후
 *   가. (a1=b1 and a2=b2) and (a1=c1 and a2=c2)
 *   나. (a1=b1 and a2=b2)
 *   다. (a1=b1) and (a1=b2)
 *
 *   마찬가지로 (가)의 경우는 제약사항에서 걸러지므로 고려할 필요가 없다.
 *   ALL Type 의 경우는 ANY Type 과는 달리 모두 아래와 같은
 *   형태로 동일하게 변환할 수 있다.
 *   Source 가 list 인가 아닌가에 따라 * 가 붙은 곳이 a1 또는 a2 로
 *   달라질 뿐이다.
 *
 *
 *         (OR)          (OR)
 *          |              |
 *          V              V
 *      (=,>=,<>)  ->  (=,>=,<>)
 *          |              |
 *          V              V
 *        (a1) -> (b1)  *(a2) -> (b2)
 *
 *         (OR)          (OR)
 *          |              |
 *          V              V
 *      (=,>=,<>)  ->  (=,>=,<>)
 *          |              |
 *          V              V
 *        (a1) -> (b1)  *(a1) -> (b2)
 *
 ***********************************************************************/

    qtcNode       * sOldSrc     = NULL;
    qtcNode       * sOldSrcList = NULL;
    qtcNode       * sOldTrgList = NULL;
    qtcNode       * sOldTrg     = NULL;
    qtcNode       * sNewSrc     = NULL;
    qtcNode       * sNewTrg     = NULL;
    qtcNode       * sNewPrevOR  = NULL;;

    qtcNode       * sORNode     = NULL;
    qtcNode       * sOperNode   = NULL;
    qtcNode       * sPrevOperNode = NULL;

    qtcNode       * sPredStart  = NULL;
    qtcNode       * sPredEnd    = NULL;
    UInt            sORArgCount = 0;
    mtfModule     * sCompModule = NULL;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::transJoinOperListArgStyle::__FT__" );

    IDE_DASSERT( aStatement  != NULL );
    IDE_DASSERT( aQuerySet   != NULL );
    IDE_DASSERT( aInNode     != NULL );
    IDE_DASSERT( aTransStart != NULL );
    IDE_DASSERT( aTransEnd   != NULL );

    //---------------------------------------------
    // 1. ANY Type 연산자의 변환
    //---------------------------------------------
    if ( QMO_IS_ANY_TYPE_MODULE( aInNode->node.module ) == ID_TRUE )
    {
        //---------------------------------------------
        // 비교 Source 쪽이 list 이면, target 쪽은
        // list 가 2 개이상일 수 없기 때문에
        // 한개라고 생각하고 변환하면 된다.
        // (이 함수에 오기 전에 이미 걸러졌다.)
        //---------------------------------------------
        if ( aInNode->node.arguments->module == & mtfList )
        {
            sOldSrcList = (qtcNode *)aInNode->node.arguments;
            sOldTrgList = (qtcNode *)aInNode->node.arguments->next->arguments;

            //---------------------------------------------
            // Source List 의 argument 갯수와 Target List 의
            // argument 갯수는 항상 같으므로 각 대응되는
            // argument 들을 가지고 연산자로 묶으면 된다.
            //---------------------------------------------
            for ( sOldSrc  = (qtcNode *)sOldSrcList->node.arguments,
                  sOldTrg  = (qtcNode *)sOldTrgList->node.arguments;
                  sOldSrc != NULL;
                  sOldSrc  = (qtcNode *)sOldSrc->node.next,
                  sOldTrg  = (qtcNode *)sOldTrg->node.next )
            {

                //---------------------------------------------
                // 원래 module 을 주면 변환할 때 사용할 연산 module 을 리턴
                //---------------------------------------------
                IDE_TEST( getCompModule4List( (mtfModule *)aInNode->node.module,
                                              & sCompModule)
                          != IDE_SUCCESS);

                /**********************************************
                 *  Looping 을 돌때마다 다음과 같은 Tree 가 생긴다.
                 *  이러한 Tree 의 OR 노드들을 연결해나간다.
                 *
                 *        (OR)
                 *          |
                 *          V
                 *       (Equal)
                 *          |
                 *          V
                 *        (a1) -> (b1)
                 **********************************************/
                IDE_TEST( makeNewORNodeTree( aStatement,
                                             aQuerySet,
                                             aInNode,
                                             sOldSrc,
                                             sOldTrg,
                                             & sNewSrc,
                                             & sNewTrg,
                                             sCompModule,
                                             & sORNode,
                                             ID_TRUE )
                          != IDE_SUCCESS);

                if ( sOldSrc == (qtcNode *)sOldSrcList->node.arguments )
                {
                    sPredStart = sORNode;
                    sPredEnd = sORNode;
                }
                else
                {
                    sNewPrevOR->node.next = (mtcNode *)sORNode;
                    sPredEnd = sORNode;
                }

                sNewPrevOR = sORNode;
            }

            //---------------------------------------------
            // mtfList 노드는 이제 불필요한 노드가 된다.
            // 그런데, host 변수를 사용할 때 아래와 같이 설정해주지 않으면,
            // fixAfterBinding 단계에서 module 이 설정되지 않았다고
            // 에러를 발생시킨다.
            // 그리고, size 를 0 으로 해줌으로써 불필요한 row 사이즈
            // 할당을 막는다.
            //---------------------------------------------
            QTC_STMT_COLUMN( aStatement, sOldSrcList )->column.size = 0;
            QTC_STMT_COLUMN( aStatement, sOldSrcList )->module = & mtdList;

            QTC_STMT_COLUMN( aStatement, sOldTrgList )->column.size = 0;
            QTC_STMT_COLUMN( aStatement, sOldTrgList )->module = & mtdList;

            /**********************************************
             *  다음과 같은 Tree 가 완성되었다.
             *
             *            ____ sPrevStart  ____ sPrevEnd
             *           /                /
             *        (OR)    --->    (OR)
             *          |               |
             *          V               V
             *       (Equal)         (Equal)
             *          |               |
             *          V               V
             *        (a1) -> (b1)    (a2) -> (b2)
             **********************************************/
        }

        //---------------------------------------------
        // Source 가 list 가 아닐 때
        //---------------------------------------------
        else
        {
            sOldTrgList = (qtcNode *)aInNode->node.arguments->next;
            sORArgCount = 0;

            for ( sOldSrc  = (qtcNode *)aInNode->node.arguments,
                  sOldTrg  = (qtcNode *)sOldTrgList->node.arguments;
                  sOldTrg != NULL;
                  sOldTrg  = (qtcNode *)sOldTrg->node.next )
            {
                IDE_TEST( getCompModule4List( (mtfModule *)aInNode->node.module,
                                              & sCompModule)
                          != IDE_SUCCESS);

                if ( sOldTrg == (qtcNode *)sOldTrgList->node.arguments )
                {
                    IDE_TEST( makeNewORNodeTree( aStatement,
                                                 aQuerySet,
                                                 aInNode,
                                                 sOldSrc,
                                                 sOldTrg,
                                                 & sNewSrc,
                                                 & sNewTrg,
                                                 sCompModule,
                                                 & sORNode,
                                                 ID_FALSE )
                              != IDE_SUCCESS);

                    sORArgCount = 1;
                    sORNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
                    sORNode->node.lflag |= sORArgCount;

                    sPrevOperNode = (qtcNode *)sORNode->node.arguments;

                    //---------------------------------------------
                    // OR 노드는 하나만 생기기 때문에 Start 와 End 가 동일하다.
                    //---------------------------------------------
                    sPredStart = sORNode;
                    sPredEnd = sORNode;
                }
                else
                {
                    IDE_TEST( makeNewOperNodeTree( aStatement,
                                                   aInNode,
                                                   sOldSrc,
                                                   sOldTrg,
                                                   & sNewSrc,
                                                   & sNewTrg,
                                                   sCompModule,
                                                   & sOperNode )
                              != IDE_SUCCESS );

                    //---------------------------------------------
                    // 신규로 생성된 비교연산자 노드를 기존의 리스트에
                    // 연결한다.
                    //---------------------------------------------
                    sORArgCount++;
                    sORNode->node.lflag |= sORArgCount;

                    sPrevOperNode->node.next = (mtcNode *)sOperNode;
                    sOperNode->node.next = NULL;

                    sPrevOperNode = sOperNode;
                }
            }

            //---------------------------------------------
            // estimate 수행
            //---------------------------------------------
            IDE_TEST( qtc::estimate( sORNode,
                                     QC_SHARED_TMPLATE(aStatement),
                                     aStatement,
                                     aQuerySet,
                                     aQuerySet->SFWGH,
                                     NULL )
                      != IDE_SUCCESS );

            /**********************************************
             *  다음과 같은 Tree 가 완성되었다.
             *
             *            ____ sPrevStart, sPrevEnd
             *           /
             *        (OR)
             *          |
             *          V
             *       (Equal)   -->   (Equal)   --> ...
             *          |               |
             *          V               V
             *        (a1) -> (b1)    (a1) -> (b2)
             **********************************************/

            QTC_STMT_COLUMN( aStatement, sOldTrgList )->column.size = 0;
            QTC_STMT_COLUMN( aStatement, sOldTrgList )->module = & mtdList;
        }
    }

    //---------------------------------------------
    // 2. ALL Type 연산자의 변환
    //    mtfEqual, mtfNotEqual 연산자는 ALL Type 은 아니지만
    //    Source 에 list 가 오는 형태로 동일하게 처리해주면 된다.
    //---------------------------------------------
    else if ( QMO_IS_ALL_TYPE_MODULE( aInNode->node.module ) == ID_TRUE )
    {
        //---------------------------------------------
        // 비교 Source 쪽이 list 이면, target 쪽은
        // list 가 2 개이상일 수 없기 때문에
        // 한개라고 생각하고 변환하면 된다.
        // (이 함수에 오기 전에 이미 걸러졌다.)
        //---------------------------------------------
        if ( aInNode->node.arguments->module == & mtfList )
        {
            sOldSrcList = (qtcNode *)aInNode->node.arguments;

            if ( ( aInNode->node.module == & mtfEqual )
              || ( aInNode->node.module == & mtfNotEqual ) )
            {
                sOldTrgList = (qtcNode *)aInNode->node.arguments->next;
            }
            else
            {
                sOldTrgList = (qtcNode *)aInNode->node.arguments->next->arguments;
            }

            //---------------------------------------------
            // Source List 의 argument 갯수와 Target List 의
            // argument 갯수는 항상 같으므로 각 대응되는
            // argument 들을 가지고 연산자로 묶으면 된다.
            //---------------------------------------------
            for ( sOldSrc  = (qtcNode *)sOldSrcList->node.arguments,
                  sOldTrg  = (qtcNode *)sOldTrgList->node.arguments;
                  sOldSrc != NULL;
                  sOldSrc  = (qtcNode *)sOldSrc->node.next,
                  sOldTrg  = (qtcNode *)sOldTrg->node.next )
            {

                IDE_TEST( getCompModule4List( (mtfModule *)aInNode->node.module,
                                              & sCompModule )
                          != IDE_SUCCESS );

                /**********************************************
                 *  Looping 을 돌때마다 다음과 같은 Tree 가 생긴다.
                 *  이러한 Tree 의 OR 노드들을 연결해나간다.
                 *
                 *        (OR)
                 *          |
                 *          V
                 *       (Equal)
                 *          |
                 *          V
                 *        (a1) -> (b1)
                 **********************************************/
                IDE_TEST( makeNewORNodeTree( aStatement,
                                             aQuerySet,
                                             aInNode,
                                             sOldSrc,
                                             sOldTrg,
                                             & sNewSrc,
                                             & sNewTrg,
                                             sCompModule,
                                             & sORNode,
                                             ID_TRUE )
                          != IDE_SUCCESS );

                if ( sOldSrc == (qtcNode *)sOldSrcList->node.arguments )
                {
                    sPredStart = sORNode;
                    sPredEnd = sORNode;
                }
                else
                {
                    sNewPrevOR->node.next = (mtcNode *)sORNode;
                    sPredEnd = sORNode;
                }

                sNewPrevOR = sORNode;
            }

            /**********************************************
             *  다음과 같은 Tree 가 완성되었다.
             *
             *            ____ sPrevStart  ____ sPrevEnd
             *           /                /
             *        (OR)    --->    (OR)
             *          |               |
             *          V               V
             *       (Equal)         (Equal)
             *          |               |
             *          V               V
             *        (a1) -> (b1)    (a2) -> (b2)
             **********************************************/

            QTC_STMT_COLUMN( aStatement, sOldSrcList )->column.size = 0;
            QTC_STMT_COLUMN( aStatement, sOldSrcList )->module = & mtdList;

            QTC_STMT_COLUMN( aStatement, sOldTrgList )->column.size = 0;
            QTC_STMT_COLUMN( aStatement, sOldTrgList )->module = & mtdList;
        }

        //---------------------------------------------
        // Source 가 list 가 아닐 때
        //---------------------------------------------
        else
        {
            if ( ( aInNode->node.module == & mtfEqual )
              || ( aInNode->node.module == & mtfNotEqual ) )
            {
                IDE_RAISE( ERR_INVALID_MODULE );
            }
            else
            {
                // Nothing to do.
            }

            sOldTrgList = (qtcNode *)aInNode->node.arguments->next;

            for ( sOldSrc  = (qtcNode *)aInNode->node.arguments,
                  sOldTrg  = (qtcNode *)sOldTrgList->node.arguments;
                  sOldTrg != NULL;
                  sOldTrg  = (qtcNode *)sOldTrg->node.next )
            {

                IDE_TEST( getCompModule4List( (mtfModule *)aInNode->node.module,
                                              & sCompModule )
                          != IDE_SUCCESS );

                /**********************************************
                 *  Looping 을 돌때마다 다음과 같은 Tree 가 생긴다.
                 *  이러한 Tree 의 OR 노드들을 연결해나간다.
                 *
                 *        (OR)
                 *          |
                 *          V
                 *       (Equal)
                 *          |
                 *          V
                 *        (a1) -> (b1)
                 **********************************************/
                IDE_TEST( makeNewORNodeTree( aStatement,
                                             aQuerySet,
                                             aInNode,
                                             sOldSrc,
                                             sOldTrg,
                                             & sNewSrc,
                                             & sNewTrg,
                                             sCompModule,
                                             & sORNode,
                                             ID_TRUE )
                          != IDE_SUCCESS );

                if ( sOldTrg == (qtcNode *)sOldTrgList->node.arguments )
                {
                    sPredStart = sORNode;
                    sPredEnd = sORNode;
                }
                else
                {
                    sNewPrevOR->node.next = (mtcNode *)sORNode;
                    sPredEnd = sORNode;
                }

                sNewPrevOR = sORNode;
            }

            /**********************************************
             *  다음과 같은 Tree 가 완성되었다.
             *
             *            ____ sPrevStart  ____ sPrevEnd
             *           /                /
             *        (OR)    --->    (OR)
             *          |               |
             *          V               V
             *       (Equal)         (Equal)
             *          |               |
             *          V               V
             *        (a1) -> (b1)    (a1) -> (b2)
             **********************************************/

            QTC_STMT_COLUMN( aStatement, sOldTrgList )->column.size = 0;
            QTC_STMT_COLUMN( aStatement, sOldTrgList )->module = & mtdList;
        }
    }
    else
    {
        IDE_RAISE( ERR_INVALID_MODULE );
    }

    //---------------------------------------------
    // 변환한 Tree 의 Start OR 노드와 End OR 노드를
    // 반환한다.
    //---------------------------------------------

    *aTransStart = sPredStart;
    *aTransEnd   = sPredEnd;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_MODULE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::transJoinOperListArgStyle",
                                  "Invalid comparison module" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::merge2ANSIStyleFromTree( qmsFrom     * aFromFrom,
                                           qmsFrom     * aToFrom,
                                           idBool        aFirstIsLeft,
                                           qcDepInfo   * aFindDepInfo,
                                           qmsFrom     * aNewFrom )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 * Implemenation :
 *    aFromFrom 노드를 aToFrom 쪽으로 merge
 *    1. aFindDepInfo 를 이용하여 aToFrom 에서 연결점을 찾는다.
 *    2. 해당 연결점 대신 aNewFrom 을 연결한 후 left, right 를 연결한다.
 *
 *    aFirstIsLeft 는 첫번째 argument 인 aFromFrom 이 Left Outer 의
 *    left 인지여부를 나타낸다. (ID_TRUE/ID_FALSE)
 *
 ***********************************************************************/

    qmsFrom    * sFromFrom = NULL;
    qmsFrom    * sToFrom   = NULL;
    qcDepInfo    sAndDependencies;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::merge2ANSIStyleFromTree::__FT__" );

    IDE_DASSERT( aFromFrom  != NULL );
    IDE_DASSERT( aToFrom    != NULL );
    IDE_DASSERT( aNewFrom   != NULL );

    sFromFrom  = aFromFrom;
    sToFrom    = aToFrom;


    //------------------------------------------------------
    // 두 개의 From 노드간에는 연결점이 반드시 있어야 한다.
    // 이를 depInfo 를 통해 검사한다.
    //------------------------------------------------------

    qtc::dependencyAnd( aFindDepInfo,
                        & sToFrom->depInfo,
                        & sAndDependencies );

    IDE_TEST( qtc::dependencyEqual( & sAndDependencies,
                                    & qtc::zeroDependencies )
              == ID_TRUE );

    //------------------------------------------------------
    // left, right 가 모두 leaf node 일 때는 더이상
    // traverse 할 필요가 없다.
    //------------------------------------------------------

    if ( ( sToFrom->left->joinType == QMS_NO_JOIN )
      && ( sToFrom->right->joinType == QMS_NO_JOIN ) )
    {

        //------------------------------------------------------
        // 연결점이 left 노드인 경우
        //------------------------------------------------------

        if( qtc::dependencyEqual( aFindDepInfo,
                                  & sToFrom->left->depInfo )
            == ID_TRUE )
        {
            if ( aFirstIsLeft == ID_TRUE )
            {
                aNewFrom->left = sFromFrom;
                aNewFrom->right = sToFrom->left;
                sToFrom->left = aNewFrom;
            }
            else
            {
                aNewFrom->left = sToFrom->left;
                aNewFrom->right = sFromFrom;
                sToFrom->left = aNewFrom;
            }
        }

        //------------------------------------------------------
        // 연결점이 right 노드인 경우
        //------------------------------------------------------

        else if( qtc::dependencyEqual( aFindDepInfo,
                                       & sToFrom->right->depInfo )
                 == ID_TRUE )
        {
            if ( aFirstIsLeft == ID_TRUE )
            {
                aNewFrom->left = sFromFrom;
                aNewFrom->right = sToFrom->right;
                sToFrom->right = aNewFrom;
            }
            else
            {
                aNewFrom->left = sToFrom->right;
                aNewFrom->right = sFromFrom;
                sToFrom->right = aNewFrom;
            }
        }

        //------------------------------------------------------
        // 이도저도 아니면 에러
        //------------------------------------------------------

        else
        {
            IDE_RAISE( ERR_RELATION_CONNECT_POINT );
        }

        //------------------------------------------------------
        // 연결하는 노드의 dependency 정보 Oring
        //------------------------------------------------------

        IDE_TEST( qtc::dependencyOr( & aNewFrom->left->depInfo,
                                     & aNewFrom->right->depInfo,
                                     & aNewFrom->depInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & sToFrom->depInfo,
                                     & aNewFrom->depInfo,
                                     & sToFrom->depInfo )
                  != IDE_SUCCESS );
    }

    //------------------------------------------------------
    // right 쪽은 leaf 이고, left 쪽은 tree 인 경우
    //------------------------------------------------------

    else if ( ( sToFrom->left->joinType != QMS_NO_JOIN )
           && ( sToFrom->right->joinType == QMS_NO_JOIN ) )
    {
        //------------------------------------------------------
        // 연결점이 left 노드인 경우
        //------------------------------------------------------

        qtc::dependencyAnd( aFindDepInfo,
                            & sToFrom->left->depInfo,
                            & sAndDependencies );

        if ( qtc::dependencyEqual( & sAndDependencies,
                                   & qtc::zeroDependencies )
             == ID_FALSE )
        {
            IDE_TEST( merge2ANSIStyleFromTree( sFromFrom,
                                               sToFrom->left,
                                               aFirstIsLeft,
                                               aFindDepInfo,
                                               aNewFrom )
                      != IDE_SUCCESS );
        }

        //------------------------------------------------------
        // 연결점이 right 노드인 경우
        //------------------------------------------------------

        else
        {
            if ( aFirstIsLeft == ID_TRUE )
            {
                aNewFrom->left = sFromFrom;
                aNewFrom->right = sToFrom->right;
                sToFrom->right = aNewFrom;
            }
            else
            {
                aNewFrom->left = sToFrom->right;
                aNewFrom->right = sFromFrom;
                sToFrom->right = aNewFrom;
            }

            IDE_TEST( qtc::dependencyOr( & aNewFrom->left->depInfo,
                                         & aNewFrom->right->depInfo,
                                         & aNewFrom->depInfo )
                      != IDE_SUCCESS );
        }

        //------------------------------------------------------
        // 연결하는 노드의 dependency 정보 Oring
        //------------------------------------------------------

        IDE_TEST( qtc::dependencyOr( & sToFrom->depInfo,
                                     & aNewFrom->depInfo,
                                     & sToFrom->depInfo )
                  != IDE_SUCCESS );
    }

    //------------------------------------------------------
    // right 쪽은 tree 이고, left 쪽은 leaf 인 경우
    //------------------------------------------------------

    else if ( ( sToFrom->left->joinType == QMS_NO_JOIN )
           && ( sToFrom->right->joinType != QMS_NO_JOIN ) )
    {
        //------------------------------------------------------
        // 연결점이 right 노드인 경우
        //------------------------------------------------------

        qtc::dependencyAnd( aFindDepInfo,
                            & sToFrom->right->depInfo,
                            & sAndDependencies );

        if ( qtc::dependencyEqual( & sAndDependencies,
                                   & qtc::zeroDependencies )
             == ID_FALSE )
        {
            IDE_TEST( merge2ANSIStyleFromTree( sFromFrom,
                                               sToFrom->right,
                                               aFirstIsLeft,
                                               aFindDepInfo,
                                               aNewFrom )
                      != IDE_SUCCESS );
        }

        //------------------------------------------------------
        // 연결점이 left 노드인 경우
        //------------------------------------------------------

        else
        {
            if ( aFirstIsLeft == ID_TRUE )
            {
                aNewFrom->left = sFromFrom;
                aNewFrom->right = sToFrom->left;
                sToFrom->left = aNewFrom;
            }
            else
            {
                aNewFrom->left = sToFrom->left;
                aNewFrom->right = sFromFrom;
                sToFrom->left = aNewFrom;
            }

            IDE_TEST( qtc::dependencyOr( & aNewFrom->left->depInfo,
                                         & aNewFrom->right->depInfo,
                                         & aNewFrom->depInfo )
                      != IDE_SUCCESS );
        }

        //------------------------------------------------------
        // 연결하는 노드의 dependency 정보 Oring
        //------------------------------------------------------

        IDE_TEST( qtc::dependencyOr( & sToFrom->depInfo,
                                     & aNewFrom->depInfo,
                                     & sToFrom->depInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        //  ( ( sToFrom->left->joinType  != QMS_NO_JOIN )
        // && ( sToFrom->right->joinType != QMS_NO_JOIN ) )

        //------------------------------------------------------
        // 연결점이 left 노드인 경우
        //------------------------------------------------------

        qtc::dependencyAnd( aFindDepInfo,
                            & sToFrom->left->depInfo,
                            & sAndDependencies );

        if ( qtc::dependencyEqual( & sAndDependencies,
                                   & qtc::zeroDependencies )
             == ID_FALSE )
        {
            IDE_TEST( merge2ANSIStyleFromTree( sFromFrom,
                                               sToFrom->left,
                                               aFirstIsLeft,
                                               aFindDepInfo,
                                               aNewFrom )
                      != IDE_SUCCESS );
        }

        //------------------------------------------------------
        // 연결점이 right 노드인 경우
        //------------------------------------------------------

        else
        {
            IDE_TEST( merge2ANSIStyleFromTree( sFromFrom,
                                               sToFrom->right,
                                               aFirstIsLeft,
                                               aFindDepInfo,
                                               aNewFrom )
                      != IDE_SUCCESS );
        }

        //------------------------------------------------------
        // 연결하는 노드의 dependency 정보 Oring
        //------------------------------------------------------

        IDE_TEST( qtc::dependencyOr( & sToFrom->depInfo,
                                     & aNewFrom->depInfo,
                                     & sToFrom->depInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RELATION_CONNECT_POINT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::merge2ANSIStyleFromTree",
                                  "No connection point for from tree" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::transformJoinOper2From( qcStatement      * aStatement,
                                          qmsQuerySet      * aQuerySet,
                                          qmoJoinOperTrans * aTrans,
                                          qmsFrom          * aNewFrom,
                                          qmoJoinOperRel   * aJoinGroupRel,
                                          qmoJoinOperPred  * aJoinGroupPred )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Join Relation -> From Tree
 *
 * Implemenation :
 *
 *    아래 예를 통해 변환방법을 살펴보자.
 *
 *    select t1.*, t2.*, t3.*, t4.*, t5.*
 *      from t1, t2, t3, t4, t5
 *     where t1.i1    = t2.i1(+)
 *       and t1.i1(+) = t5.i1
 *       and t2.i1    = t4.i1(+)
 *       and t3.i1(+) = t5.i1;
 *    
 *    위의 SQL 에서 Join Relation 을 뽑아보면 다음과 같다.
 *    조건의 순서를 바꾸어도 아래와 같이 table 값 순서로 정렬된다.
 *    (Join Group 별로 이미 정렬되어 이 함수로 넘어온다.)
 *          _    _        _    _
 *       (1,2)->(1,5)->(2,4)->(3,5)
 *    
 *    위의 Join Relation 은 아래와 같이 tree 구조로 변경될 수 있다.
 *    
 *                left
 *              /      \
 *            left    left
 *           /   \   /    \
 *         left   1  2    4
 *         /  \
 *        5    3
 *    
 *    위의 개념 tree 는 아래의 과정을 통해 완성된다.
 *    모든 Join 은 Left Outer Join 으로만 생성하고,
 *    (+)가 붙지 않은 테이블을 좌측에 위치시킨다.
 *    Relation List 를 하나씩 가져와서 Tree 의 연결점을 찾아 연결해주는 과정이다.
 *
 *    하나의 Join Group 내에서 table 값으로 정렬된 Join Relation List 는
 *    각 List 들이 dependency 테이블을 통해 연결관계를 가질 수 밖에 없다.
 *    따라서, 결론적으로는 하나의 Join Group 내의
 *    Join Relation List 는 하나의 tree 로 완성된다.
 *    
 *            _
 *    (가) (1,2)
 *    
 *                left           --> on t1.i1=t2.i1
 *              /      \
 *             1        2
 *    
 *          _
 *    (나) (1,5)
 *    
 *                left
 *              /      \
 *            left      2        --> on t5.i1=t1.i1
 *           /    \
 *          5      1
 *    
 *            _
 *    (다) (2,4)
 *    
 *                left
 *              /      \
 *            left    left       --> on t2.i1=t4.i1
 *           /    \   /   \
 *          5      1 2     4
 *    
 *          _
 *    (라) (3,5)
 *    
 *                left
 *              /      \
 *            left    left
 *           /    \   /   \
 *         left    1 2     4     --> on t5.i1=t3.i1
 *        /   \
 *       5     3
 *    
 *    위의 개념적인 tree 구조를 ANSI 구문으로 표현하면 아래와 같다.
 *    실제 조회를 해보면 위에서 (+)를 사용한 구문의 결과와 정확히
 *    일치함을 확인할 수 있다.
 *    좌측부터 bottom up 형태로 구문에 표현해주면 된다.
 *    위의 tree 구조와 비교가 쉽도록 구문을 표현했다.
 *    
 *    select t1.*, t2.*, t3.*, t4.*, t5.*
 *      from                t5
 *                    left                         outer join
 *                          t3
 *                      on t5.i1=t3.i1
 *                left                             outer join
 *                     t1
 *                  on t5.i1=t1.i1
 *           left                                  outer join
 *                     t2
 *                left                             outer join
 *                     t4
 *                  on t2.i1=t4.i1
 *             on t1.i1=t2.i1;
 *
 ***********************************************************************/

    qmsFrom          * sFromStart      = NULL;
    qmsFrom          * sFromCur        = NULL;
    qmsFrom          * sFromLeft       = NULL;
    qmsFrom          * sFromRight      = NULL;
    qmsFrom          * sFromPrev       = NULL;
    qmsFrom          * sFromLeftPrev   = NULL;
    qmsFrom          * sFromRightPrev  = NULL;
    qtcNode          * sErrNode        = NULL;

    UShort             sLeft;
    UShort             sRight;
    qcDepInfo          sLeftDepInfo;
    qcDepInfo          sRightDepInfo;
    qcDepInfo          sAndDependencies;
    idBool             sFirstIsLeft;
    UChar              sJoinOper;

    qcuSqlSourceInfo   sqlInfo;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::transformJoinOper2From::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aNewFrom   != NULL );
    IDE_DASSERT( aJoinGroupRel   != NULL );
    IDE_DASSERT( aJoinGroupPred  != NULL );


    sFromStart = (qmsFrom *)aQuerySet->SFWGH->from;


    //------------------------------------------------------
    // Join Relation 의 두개 dependency table 에 대한
    // (+) 유무에 따라 Left, Right 테이블을 결정
    // (+) 가 없는 쪽이 Left
    //------------------------------------------------------

    sJoinOper = qtc::getDependJoinOper( &aJoinGroupRel->depInfo, 0 );

    if ( QMO_JOIN_OPER_EXISTS( sJoinOper )
         == ID_FALSE )
    {
        sLeft = qtc::getDependTable( &aJoinGroupRel->depInfo,0 );
        sRight = qtc::getDependTable( &aJoinGroupRel->depInfo,1 );
    }
    else
    {
        sRight = qtc::getDependTable( &aJoinGroupRel->depInfo,0 );
        sLeft = qtc::getDependTable( &aJoinGroupRel->depInfo,1 );
    }

    //------------------------------------------------------
    // aNewFrom 노드를 추가할 위치를 찾기 위해 traverse 시
    // 비교할 Left/Right table 값
    //------------------------------------------------------

    qtc::dependencySet( sLeft, &sLeftDepInfo );
    qtc::dependencySet( sRight, &sRightDepInfo );

    //------------------------------------------------------
    // 위의 Left, Right 정보를 이용하여 qmsFrom List 를
    // 탐색한 후 찾은 qmsFrom type 에 따라 다른 처리 수행
    //
    //  1. Left = QMS_NO_JOIN, Right = QMS_NO_JOIN
    //     => NewFrom(QMS_LEFT_OUTER_JOIN) 에 Left, Right 연결
    //
    //  2. Left != QMS_NO_JOIN, Right = QMS_NO_JOIN
    //     => Left 값을 이용하여 Tree 탐색하여 같은 dependency table
    //        정보를 가진 노드를 발견하면 NewFrom(QMS_LEFT_OUTER_JOIN)을
    //        해당 노드 대신 대체한 후 Left,Right 연결
    //
    //  3. Left = QMS_NO_JOIN, Right != QMS_NO_JOIN
    //     => Right 값을 이용하여 Tree 탐색하여 같은 dependency table
    //        정보를 가진 노드를 발견하면 NewFrom(QMS_LEFT_OUTER_JOIN)을
    //        해당 노드 대신 대체한 후 Left,Right 연결
    //
    //  4. Left != QMS_NO_JOIN, Right != QMS_NO_JOIN
    //     => Join Relation 은 하나의 Tree 로 완성되기 때문에
    //        이런 경우는 발생할 수 없다. (에러)
    //------------------------------------------------------

    //------------------------------------------------------
    // sLeftDepInfo 와 sRightDepInfo 정보를 이용하여
    // 각각의 테이블이 포함된 From node 를 찾는다.
    //
    //     sFromRightPrev sFromLeftPrev
    //          |             |
    //          v             v
    // from -> (5) -> (2) -> (3) -> (1) -> (4) -> null
    //                 ^             ^
    //                 |             |
    //             sFromRight    sFromLeft
    //
    // 또는, 일부 Join Relation 이 이미 tree 형태로 전환된 상태라면
    // 다음과 같은 상태일 수도 있다. (L 은 QMS_LEFT_OUTER_JOIN)
    //
    //              sFromRight
    //                 |
    //     sFromRightPrev sFromLeftPrev
    //          |      |      |
    //          v      v      v
    // from -> (5) -> (L) -> (3) -> (1) -> (4) -> null
    //               /   \           ^
    //             (7)   (6)         |
    //                           sFromLeft
    //------------------------------------------------------

    sFromLeft  = NULL;
    sFromRight = NULL;

    for ( sFromCur  = aQuerySet->SFWGH->from;
          sFromCur != NULL;
          sFromPrev = sFromCur, sFromCur = sFromCur->next )
    {
        qtc::dependencyAnd( & sFromCur->depInfo,
                            & sLeftDepInfo,
                            & sAndDependencies );

        if( qtc::dependencyEqual( & sAndDependencies,
                                  & qtc::zeroDependencies )
            == ID_FALSE )
        {
            sFromLeft = sFromCur;
            sFromLeftPrev = sFromPrev;
        }
        else
        {
            // Nothing to do.
        }

        qtc::dependencyClear( & sAndDependencies );

        qtc::dependencyAnd( & sFromCur->depInfo,
                            & sRightDepInfo,
                            & sAndDependencies );

        if( qtc::dependencyEqual( & sAndDependencies,
                                  & qtc::zeroDependencies )
            == ID_FALSE )
        {
            sFromRight = sFromCur;
            sFromRightPrev = sFromPrev;
        }
        else
        {
            // Nothing to do.
        }

        if ( ( sFromLeft != NULL ) && ( sFromRight != NULL ) )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( ( sFromLeft == NULL ) || ( sFromRight == NULL ) )
    {
        sErrNode = aQuerySet->SFWGH->where;
        sqlInfo.setSourceInfo ( aStatement, & sErrNode->position );

        IDE_RAISE( ERR_ABORT_NOT_ALLOW_OUTER_JOIN_INAPPROPRIATE_TABLE );
    }
    else
    {
        // Nothing to do.
    }

    /*------------------------------------------------------
     * Left, Right 노드를 from list 에서 제거
     *
     * from -> (3) -> (4) -> null
     *
     *   sFromRight sFromLeft
     *        |       |
     *        v       v
     *       (5)     (L)
     *              /   \
     *            (1)   (2)
     *-----------------------------------------------------*/

    //------------------------------------------------------
    // Left 와 Right 의 위치에 따라 처리
    //  0. Left 와 Right가 같은 경우 이는 NewFrom으로 구성된 트리에
    //     참조 테이블이 모두 있는 경우 (BUG-37693)
    //  1. Left 가 list 의 맨앞이고 그 다음 Right 가 바로 올때
    //  2. Right 가 list 의 맨앞이고 그 다음 Left 가 바로 올때
    //  3. Left 와 Right 가 list 의 중간에 올 때
    //------------------------------------------------------

    if ( sFromLeft == sFromRight )
    {
        /* BUG-37693
         * 이미 트리로 구성된 from에 모든 참조 테이블이 있는 경우로
         * 더이상 트리 구성이나 머지가 필요 없다.
         */
        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        /* Nothing to do */
    }

    if ( sFromLeft == sFromStart )
    {
        if ( sFromLeft->next == sFromRight )
        {
            aQuerySet->SFWGH->from = sFromRight->next;

            sFromLeft->next  = NULL;
            sFromRight->next = NULL;
        }
        else
        {
            aQuerySet->SFWGH->from = sFromLeft->next;
            sFromLeft->next        = NULL;

            IDE_TEST_RAISE( sFromRightPrev == NULL, ERR_INVALID_RIGHT_PRIV );
            sFromRightPrev->next = sFromRight->next;
            sFromRight->next     = NULL;
        }
    }
    else
    {
        if ( sFromRight == sFromStart )
        {
            if ( sFromRight->next == sFromLeft )
            {
                aQuerySet->SFWGH->from = sFromLeft->next;

                sFromLeft->next  = NULL;
                sFromRight->next = NULL;
            }
            else
            {
                aQuerySet->SFWGH->from = sFromRight->next;
                sFromRight->next       = NULL;

                IDE_TEST_RAISE( sFromLeftPrev == NULL, ERR_INVALID_LEFT_PRIV );
                sFromLeftPrev->next = sFromLeft->next;
                sFromLeft->next     = NULL;
            }
        }
        else
        {
            if ( sFromLeft->next == sFromRight )
            {
                IDE_TEST_RAISE( sFromLeftPrev == NULL, ERR_INVALID_LEFT_PRIV );
                sFromLeftPrev->next = sFromRight->next;

                sFromLeft->next  = NULL;
                sFromRight->next = NULL;
            }
            else
            {
                if ( sFromRight->next == sFromLeft )
                {
                    IDE_TEST_RAISE( sFromRightPrev == NULL,
                                    ERR_INVALID_RIGHT_PRIV );
                    sFromRightPrev->next = sFromLeft->next;

                    sFromLeft->next  = NULL;
                    sFromRight->next = NULL;
                }
                else
                {
                    IDE_TEST_RAISE( sFromLeftPrev == NULL,
                                    ERR_INVALID_LEFT_PRIV );
                    sFromLeftPrev->next = sFromLeft->next;
                    sFromLeft->next     = NULL;

                    IDE_TEST_RAISE( sFromRightPrev == NULL,
                                    ERR_INVALID_RIGHT_PRIV);
                    sFromRightPrev->next = sFromRight->next;
                    sFromRight->next     = NULL;
                }
            }
        }
    }

    //------------------------------------------------------
    // Left, Right 노드의 joinType 에 맞게 처리
    //------------------------------------------------------

    if ( ( sFromLeft->joinType  == QMS_NO_JOIN ) &&
         ( sFromRight->joinType == QMS_NO_JOIN ) )
    {
        /*------------------------------------------------------
         * From node list 가 아래와 같은 상태일 때의 처리이다.
         * sFromLeft 또는 sFromRight 가 list 의 맨앞일 때를 고려한다.
         *
         *     sFromRightPrev sFromLeftPrev
         *          |             |
         *          v             v
         * from -> (5) -> (2) -> (3) -> (1) -> (4) -> null
         *                 ^             ^
         *                 |             |
         *             sFromRight    sFromLeft
         *
         * 처리후의 From List 는 다음과 같이 변경된다.
         *
         * from -> (L) -> (5) -> (3) -> (4) -> null
         *        /   \
         *      (1)   (2)
         *-----------------------------------------------------*/

        //------------------------------------------------------
        // aNewFrom 에 Left, Right 노드 연결
        //------------------------------------------------------

        aNewFrom->left  = sFromLeft;
        aNewFrom->right = sFromRight;

        //------------------------------------------------------
        // 연결하는 노드의 dependency 정보 Oring
        //------------------------------------------------------

        IDE_TEST( qtc::dependencyOr( & sFromLeft->depInfo,
                                     & sFromRight->depInfo,
                                     & aNewFrom->depInfo )
                  != IDE_SUCCESS );

        //------------------------------------------------------
        // aNewFrom 의 onCondition 작성
        //------------------------------------------------------

        IDE_TEST( movePred2OnCondition( aStatement,
                                        aTrans,
                                        aNewFrom,
                                        aJoinGroupRel,
                                        aJoinGroupPred )
                  != IDE_SUCCESS );

        //------------------------------------------------------
        // 완성된 tree 를 from list 의 맨앞에 연결
        //------------------------------------------------------

        aNewFrom->next         = aQuerySet->SFWGH->from;
        aQuerySet->SFWGH->from = aNewFrom;
    }
    else
    {
        if ( ( sFromLeft->joinType  != QMS_NO_JOIN ) &&
             ( sFromRight->joinType == QMS_NO_JOIN ) )
        {
            /*------------------------------------------------------
             * From node list 가 아래와 같은 상태일 때의 처리이다.
             *    _
             * (5,2) 의 추가
             *
             *    sFromRight sFromLeft
             *          |      |
             *          v      v
             * from -> (5) -> (L) -> (3) -> (4) -> null
             *               /   \
             *             (1)   (2)
             *
             * 처리후의 From List 는 다음과 같이 변경된다.
             *
             * from -> (L) -> (3) -> (4) -> null
             *        /   \
             *      (1)   (L) <- aNewFrom
             *           /   \
             *         (5)   (2)
             *-----------------------------------------------------*/

            //------------------------------------------------------
            // tree 의 연결점을 찾아서 aNewFrom 을 삽입
            // Merge sFromRight -> sFromLeft
            //------------------------------------------------------

            sFirstIsLeft = ID_FALSE;

            IDE_TEST( merge2ANSIStyleFromTree( sFromRight,
                                               sFromLeft,
                                               sFirstIsLeft,
                                               & sLeftDepInfo,
                                               aNewFrom )
                      != IDE_SUCCESS );


            //------------------------------------------------------
            // aNewFrom 의 onCondition 작성
            //------------------------------------------------------

            IDE_TEST( movePred2OnCondition( aStatement,
                                            aTrans,
                                            aNewFrom,
                                            aJoinGroupRel,
                                            aJoinGroupPred )
                      != IDE_SUCCESS );

            //------------------------------------------------------
            // 완성된 tree 를 from list 의 맨앞에 연결
            //------------------------------------------------------

            sFromLeft->next        = aQuerySet->SFWGH->from;
            aQuerySet->SFWGH->from = sFromLeft;
        }
        else
        {
            if ( ( sFromLeft->joinType  == QMS_NO_JOIN ) &&
                 ( sFromRight->joinType != QMS_NO_JOIN ) )
            {
                /*------------------------------------------------------
                 * From node list 가 아래와 같은 상태일 때의 처리이다.
                 *
                 *  _
                 * (1,5) 의 추가
                 *
                 *    sFromRight sFromLeft
                 *          |      |
                 *          v      v
                 * from -> (L) -> (5) -> (3) -> (4) -> null
                 *        /   \
                 *      (1)   (2)
                 *
                 * 처리후의 From List 는 다음과 같이 변경된다.
                 *
                 * from -> (L) -> (3) -> (4) -> null
                 *        /   \
                 *      (L)   (2)
                 *     /   \
                 *   (5)   (1)
                 *       ^
                 *       |
                 *    aNewFrom
                 *-----------------------------------------------------*/

                //------------------------------------------------------
                // tree 의 연결점을 찾아서 aNewFrom 을 삽입
                //------------------------------------------------------

                sFirstIsLeft = ID_TRUE;

                IDE_TEST( merge2ANSIStyleFromTree( sFromLeft,
                                                   sFromRight,
                                                   sFirstIsLeft,
                                                   & sRightDepInfo,
                                                   aNewFrom )
                             != IDE_SUCCESS );


                //------------------------------------------------------
                // aNewFrom 의 onCondition 작성
                //------------------------------------------------------

                IDE_TEST( movePred2OnCondition( aStatement,
                                                aTrans,
                                                aNewFrom,
                                                aJoinGroupRel,
                                                aJoinGroupPred )
                          != IDE_SUCCESS );

                //------------------------------------------------------
                // 완성된 tree 를 from list 의 맨앞에 연결
                //------------------------------------------------------

                sFromRight->next = aQuerySet->SFWGH->from;
                aQuerySet->SFWGH->from = sFromRight;
            }
            else
            {
                //------------------------------------------------------
                //  ( ( sFromLeft->qmsJoinType  != QMS_NO_JOIN )
                // && ( sFromRight->qmsJoinType != QMS_NO_JOIN ) )
                //
                // Join Relation 은 하나의 From Tree 로 변환이 되기 때문에
                // 이런 경우는 있을 수 없음. -> 에러
                //------------------------------------------------------

                IDE_RAISE( ERR_INVALID_FROM_JOIN_TYPE );
            }
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_NOT_ALLOW_OUTER_JOIN_INAPPROPRIATE_TABLE )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_ALLOW_OUTER_JOIN_INAPPROPRIATE_TABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_FROM_JOIN_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::transformJoinOper2From",
                                  "Invalide join type" ));
    }
    IDE_EXCEPTION( ERR_INVALID_RIGHT_PRIV )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::transformJoinOper2From",
                                  "sFromRightPrev is null" ));
    }
    IDE_EXCEPTION( ERR_INVALID_LEFT_PRIV )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoOuterJoinOper::transformJoinOper2From",
                                  "sFromLeftPrev is null" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOuterJoinOper::transformJoinOper( qcStatement       * aStatement,
                                     qmsQuerySet       * aQuerySet,
                                     qmoJoinOperTrans  * aTrans )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description : Outer Join Operator 를 ANSI 형태로의 변환
 *
 * Implemenation :
 *    1. Join Relation List 를 순회하며 qmsFrom tree 구성
 *    2. qmsFrom tree 를 구성한 Join Predicate 이 가리키는 qtcNode 노드를
 *       onCondition 으로 이동
 *    3. oneTablePred 에서 같은 테이블에 (+)가 있는 것을 골라
 *       마찬가지로 onCondition 으로 이동
 *
 ***********************************************************************/

    qmoJoinOperGroup    * sJoinGroup    = NULL;
    qmoJoinOperGroup    * sCurJoinGroup = NULL;
    qmoJoinOperPred     * sJoinPred     = NULL;
    qmoJoinOperRel      * sJoinRel      = NULL;
    qmsFrom             * sNewFrom      = NULL;
    qtcNode             * sNorCur       = NULL;

    UInt                  sJoinGroupCnt;
    UInt                  i, j;
    idBool                sAvailRel     = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinOper::transformJoinOper::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aTrans     != NULL );

    sJoinGroup    = aTrans->joinOperGroup;
    sJoinGroupCnt = aTrans->joinOperGroupCnt;


    for ( i = 0 ;
          i < sJoinGroupCnt ;
          i++ )
    {
        sCurJoinGroup = & sJoinGroup[i];
        sJoinPred = sCurJoinGroup->joinPredicate;

        for ( j = 0; j < sCurJoinGroup->joinRelationCnt; j++ )
        {
            sJoinRel = (qmoJoinOperRel *) & sCurJoinGroup->joinRelation[j];

            //------------------------------------------------------
            // Join Relation 의 dependency 정보 유효성 검사
            //------------------------------------------------------

            IDE_TEST( isAvailableJoinOperRel( & sJoinRel->depInfo,
                                              & sAvailRel )
                      != IDE_SUCCESS);

            //------------------------------------------------------
            // Join Relation 의 두 테이블에 대해 From 자료구조를
            // Left Outer Join 구조로 바꾸기 위해 qmsFrom 구조체 할당
            //------------------------------------------------------

            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmsFrom ),
                                                     (void**)&sNewFrom )
                      != IDE_SUCCESS );

            //------------------------------------------------------
            // qmsFrom 구조체 초기화
            //------------------------------------------------------

            QCP_SET_INIT_QMS_FROM(sNewFrom);

            sNewFrom->joinType = QMS_LEFT_OUTER_JOIN;

            //------------------------------------------------------
            // Join Relation 하나를 전달하여 다음의 과정을 수행
            //
            //  1. Left Outer Join Tree 구성 (sNewFrom)
            //  2. aTrans 내의 joinPredicate 에서 depInfo 가 같은 것을
            //     찾아서 해당되는 normalCNF 노드를 찾은 다음 onCondition 에 연결.
            //     normalCNF 에서는 제거.
            //  3. aTrans 내의 oneTablePred 에 대해서도 2번과 같은 과정 수행
            //
            //  aTrans 를 전달하는 것은 normalCNF 와 oneTablePred 을 접근하기 위함.
            //------------------------------------------------------

            IDE_TEST( transformJoinOper2From( aStatement,
                                              aQuerySet,
                                              aTrans,
                                              sNewFrom,
                                              sJoinRel,
                                              sJoinPred )
                      != IDE_SUCCESS );
        }
    }

    //------------------------------------------------------
    // 정리된 normalCNF predicate node 를 순회하며
    // 최종적으로 dependency & joinOper Oring
    //------------------------------------------------------

    if ( aTrans->normalCNF != NULL )
    {
        for ( sNorCur  = (qtcNode *)aTrans->normalCNF->node.arguments;
              sNorCur != NULL ;
              sNorCur  = (qtcNode *)sNorCur->node.next )
        {
            IDE_TEST( qtc::dependencyOr( & aTrans->normalCNF->depInfo,
                                         & sNorCur->depInfo,
                                         & aTrans->normalCNF->depInfo )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
