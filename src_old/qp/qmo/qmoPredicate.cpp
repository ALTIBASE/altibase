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
 * $Id: qmoPredicate.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     Predicate Manager
 *
 *     비교 연산자를 포함하는 Predicate들에 대한 전반적인 관리를
 *     담당한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qtc.h>
#include <qmoPredicate.h>
#include <qmoNormalForm.h>
#include <qmoSubquery.h>
#include <qmoSelectivity.h>
#include <qmoJoinMethod.h>
#include <qmoTransitivity.h>
#include <qmgProjection.h>
#include <qcsModule.h>

extern mtfModule mtfLike;
extern mtfModule mtfEqual;
extern mtfModule mtfEqualAny;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfLessThan;
extern mtfModule mtfLessEqual;
extern mtfModule mtfList;
extern mtfModule mtfNotEqual;
extern mtfModule mtfNotEqualAny;
extern mtfModule mtfEqualAll;
extern mtfModule mtfNotEqualAll;
extern mtfModule mtfBetween;
extern mtfModule mtfNotBetween;
extern mtfModule mtfIsNull;
extern mtfModule mtfIsNotNull;
extern mtfModule mtfInlist;
extern mtfModule mtfNvlEqual;
extern mtfModule mtfNvlNotEqual;

static inline idBool isMultiColumnSubqueryNode( qtcNode * aNode )
{
    if( QTC_IS_SUBQUERY( aNode ) == ID_TRUE &&
        qtcNodeI::isOneColumnSubqueryNode( aNode ) == ID_FALSE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

static inline idBool isOneColumnSubqueryNode( qtcNode * aNode )
{
    if( QTC_IS_SUBQUERY( aNode ) == ID_TRUE &&
        qtcNodeI::isOneColumnSubqueryNode( aNode ) == ID_TRUE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

extern "C" SInt
compareFilter( const void * aElem1,
               const void * aElem2 )
{
/***********************************************************************
 *
 * Description : filter ordering을 위한 compare 함수
 *
 * Implementation :
 *
 *     인자로 넘어온 두 predicate->selectivity를 비교한다.
 *
 ***********************************************************************/

    //--------------------------------------
    // filter ordering을 위한 selectivity 비교
    //--------------------------------------

    if( (*(qmoPredicate**)aElem1)->mySelectivity >
        (*(qmoPredicate**)aElem2)->mySelectivity )
    {
        return 1;
    }
    else if( (*(qmoPredicate**)aElem1)->mySelectivity <
             (*(qmoPredicate**)aElem2)->mySelectivity )
    {
        return -1;
    }
    else
    {
        if( (*(qmoPredicate**)aElem1)->idx >
            (*(qmoPredicate**)aElem2)->idx )
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
}

void
qmoPred::setCompositeKeyUsableFlag( qmoPredicate * aPredicate )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                composite key와 관련한 predicate의 flag를 세팅한다.
 *
 *  Implementation :
 *     관련 구조 : composite index, range partition key(composite key)
 *     연속적인 key column 컬럼여부와 equal 비교연산자 존재 여부 설정.
 *     연속적인 key column 사용할 수 있는 predicate은,
 *       (1) equal(=)과 IN연산자이며,
 *       (2) 컬럼에 predicate이 하나만 있는 경우이다.
 *
 ***********************************************************************/

    idBool         sIsExistEqual;
    idBool         sIsExistOnlyEqual;
    qmoPredicate * sMorePredicate;
    qtcNode      * sCompareNode;

    //--------------------------------------
    // 연속적인 key column 사용가능여부와
    // 동일컬럼리스트에 equal(in) 비교연산자 존재여부 설정
    // 1. 연속적인 key column 사용가능 여부
    //    keyRange 추출시 필요한 정보.
    // 2. 동일컬럼리스트에 equal(in) 비교연산자 존재여부
    //    selection graph의 composite index에 대한 selectivity 보정시 필요.
    // To Fix PR-11731
    //    IS NULL 연산자 역시 연속적인 key column 사용이 가능하다.
    // To Fix PR-11491
    //    =ALL 연산자 역시 연속적인 key column 사용이 가능하다.
    //--------------------------------------

    sIsExistEqual     = ID_FALSE;
    sIsExistOnlyEqual = ID_TRUE;

    for( sMorePredicate = aPredicate;
         sMorePredicate != NULL;
         sMorePredicate = sMorePredicate->more )
    {
        if( ( sMorePredicate->node->node.lflag
              & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sMorePredicate->node->node.arguments);

            while( sCompareNode != NULL )
            {
                // To Fix PR-11731
                // IS NULL 연산자 역시 연속적인 key column 사용이 가능함.
                // BUG-11491 =ALL도 추가함.
                if( ( sCompareNode->node.module == &mtfEqual ) ||
                    ( sCompareNode->node.module == &mtfEqualAny ) ||
                    ( sCompareNode->node.module == &mtfEqualAll ) ||
                    ( sCompareNode->node.module == &mtfIsNull ) )
                {
                    sIsExistEqual = ID_TRUE;
                }
                else
                {
                    sIsExistOnlyEqual = ID_FALSE;
                }

                sCompareNode = (qtcNode *)(sCompareNode->node.next);
            }
        }
        else
        {
            sCompareNode = sMorePredicate->node;

            // To Fix PR-11731
            // IS NULL 연산자 역시 연속적인 key column 사용이 가능함.
            // BUG-11491 =ALL도 추가함.
            if( ( sCompareNode->node.module == &mtfEqual ) ||
                ( sCompareNode->node.module == &mtfEqualAny ) ||
                ( sCompareNode->node.module == &mtfEqualAll ) ||
                ( sCompareNode->node.module == &mtfIsNull ) )
            {
                sIsExistEqual = ID_TRUE;
            }
            else
            {
                sIsExistOnlyEqual = ID_FALSE;
            }
        }
    }


    //--------------------------------------
    // 첫번째 qmoPredicate에 flag 정보 저장
    //--------------------------------------

    if( sIsExistEqual == ID_TRUE )
    {
        aPredicate->flag &= ~QMO_PRED_EQUAL_IN_MASK;
        aPredicate->flag |= QMO_PRED_EQUAL_IN_EXIST;
    }
    else
    {
        aPredicate->flag &= ~QMO_PRED_EQUAL_IN_MASK;
        aPredicate->flag |= QMO_PRED_EQUAL_IN_ABSENT;
    }

    if( sIsExistOnlyEqual == ID_TRUE )
    {
        aPredicate->flag &= ~QMO_PRED_NEXT_KEY_USABLE_MASK;
        aPredicate->flag |= QMO_PRED_NEXT_KEY_USABLE;
    }
    else
    {
        aPredicate->flag &= ~QMO_PRED_NEXT_KEY_USABLE_MASK;
        aPredicate->flag |= QMO_PRED_NEXT_KEY_UNUSABLE;
    }
}


IDE_RC
qmoPred::relocatePredicate4PartTable(
    qcStatement       * aStatement,
    qmoPredicate      * aInPredicate,
    qcmPartitionMethod  aPartitionMethod,
    qcDepInfo         * aTableDependencies,
    qcDepInfo         * aOuterDependencies,
    qmoPredicate     ** aOutPredicate )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *               - partition graph의 predicate 재배치
 *
 *     partition graph에서는 해당 테이블에 대한
 *     OR 또는 비교연산자 단위의 predicate연결정보를 가진다.
 *     predicate관리자는 이를
 *     (1) 동일 컬럼을 포함하는 predicate의 분류
 *     (2) 하나의 인덱스로부터 얻어낼 수 있는 predicate의 분류를
 *     용이하게 하기 위해 predicate을 재배치한다.
 *
 *     [predicate의 재배치 기준]
 *     1. 인덱스사용이 가능한 one column predicate들을 컬럼별로 분리배치
 *     2. 인덱스사용이 가능한 multi column predicate(LIST)들을 분리배치
 *     3. 인덱스사용이 불가능한 predicate들을 분리배치
 *
 * Implementation :
 *
 *     (1) 각 predicate들에 대해서 indexable predicate인지를 검사해서,
 *         컬럼별로 분리배치
 *     (2) indexable column에 대한 IN(subquery)에 대한 처리는 하지 않음.
 *     (3) selectivity를 구하는 작업은 하지 않음.
 *
 *     통계정보를 이용하지 않는다. 오로지 partition keyrange를
 *     추출하기 위함임.
 *     재배치만은 하므로, selectivity를 지정해 주는 역할은
 *     qmgSelection::optimizePartition에서 하게 된다.
 *
 ***********************************************************************/
    qmoPredicate * sInPredicate;
    qmoPredicate * sRelocatePred;
    qmoPredicate * sNextPredicate;
    qmoPredicate * sPredicate;

    IDU_FIT_POINT_FATAL( "qmoPred::relocatePredicate4PartTable::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement         != NULL );
    IDE_DASSERT( aInPredicate       != NULL );
    IDE_DASSERT( aTableDependencies != NULL );
    IDE_DASSERT( aOutPredicate      != NULL );

    //--------------------------------------
    // predicate 재배치
    //--------------------------------------

    sInPredicate = aInPredicate;

    // Base Table의 Predicate에 대한 분류 :
    // indexable predicate을 판단하고, columnID 설정
    IDE_TEST( classifyPartTablePredicate( aStatement,
                                          sInPredicate,
                                          aPartitionMethod,
                                          aTableDependencies,
                                          aOuterDependencies )
              != IDE_SUCCESS );
    sRelocatePred = sInPredicate;
    sInPredicate = sInPredicate->next;
    // 연결된 sInPredicate의 next 연결을 끊는다.
    sRelocatePred->next = NULL;

    while ( sInPredicate != NULL )
    {
        IDE_TEST( classifyPartTablePredicate( aStatement,
                                              sInPredicate,
                                              aPartitionMethod,
                                              aTableDependencies,
                                              aOuterDependencies )
                  != IDE_SUCCESS );

        // 컬럼별로 연결관계 만들기
        // 동일 컬럼이 있는 경우, 동일 컬럼의 마지막 predicate.more에
        // 동일 컬럼이 없는 경우, sRelocate의 마지막 predicate.next에
        // (1) 새로운 predicate(sInPredicate)을 연결하고,
        // (2) sInPredicate = sInPredicate->next
        // (3) 연결된 predicate의 next 연결관계를 끊음.

        sNextPredicate = sInPredicate->next;

        IDE_TEST( linkPred4ColumnID( sRelocatePred,
                                     sInPredicate )
                  != IDE_SUCCESS );

        sInPredicate = sNextPredicate;
    }

    for ( sPredicate  = sRelocatePred;
          sPredicate != NULL;
          sPredicate  = sPredicate->next )
    {
        setCompositeKeyUsableFlag( sPredicate );
    }

    *aOutPredicate = sRelocatePred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::relocatePredicate( qcStatement      * aStatement,
                            qmoPredicate     * aInPredicate,
                            qcDepInfo        * aTableDependencies,
                            qcDepInfo        * aOuterDependencies,
                            qmoStatistics    * aStatiscalData,
                            qmoPredicate    ** aOutPredicate )
{
/***********************************************************************
 *
 * Description : - selection graph의 predicate 재배치
 *               - selectivity 값 세팅
 *               - Host optimization 여부 세팅
 *
 *     selection graph에서는 해당 테이블에 대한
 *     OR 또는 비교연산자 단위의 predicate연결정보를 가진다.
 *     predicate관리자는 이를
 *     (1) 동일 컬럼을 포함하는 predicate의 분류
 *     (2) 하나의 인덱스로부터 얻어낼 수 있는 predicate의 분류를
 *     용이하게 하기 위해 predicate을 재배치한다.
 *
 *     [predicate의 재배치 기준]
 *     1. 인덱스사용이 가능한 one column predicate들을 컬럼별로 분리배치
 *     2. 인덱스사용이 가능한 multi column predicate(LIST)들을 분리배치
 *     3. 인덱스사용이 불가능한 predicate들을 분리배치
 *
 * Implementation :
 *
 *     (1) 각 predicate들에 대해서 indexable predicate인지를 검사해서,
 *         컬럼별로 분리배치
 *     (2) indexable column에 대한 IN(subquery)에 대한 처리
 *     (3) one column에 대한 대표 selectivity를 구해서,
 *         qmoPredicate->more의 연결리스트 중,
 *         첫번째 qmoPredicate.totalSelectivity에 이 값을 저장한다.
 *
 ***********************************************************************/

    qmoPredicate * sInPredicate;
    qmoPredicate * sRelocatePred;
    qmoPredicate * sPredicate;
    qmoPredicate * sNextPredicate;

    IDU_FIT_POINT_FATAL( "qmoPred::relocatePredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement         != NULL );
    IDE_DASSERT( aInPredicate       != NULL );
    IDE_DASSERT( aTableDependencies != NULL );
    IDE_DASSERT( aStatiscalData     != NULL );
    IDE_DASSERT( aOutPredicate      != NULL );

    //--------------------------------------
    // predicate 재배치
    //--------------------------------------

    sInPredicate = aInPredicate;

    // Base Table의 Predicate에 대한 분류 :
    // indexable predicate을 판단하고, columnID와 selectivity 설정
    IDE_TEST( classifyTablePredicate( aStatement,
                                      sInPredicate,
                                      aTableDependencies,
                                      aOuterDependencies,
                                      aStatiscalData )
              != IDE_SUCCESS );
    sRelocatePred = sInPredicate;
    sInPredicate  = sInPredicate->next;
    // 연결된 sInPredicate의 next 연결을 끊는다.
    sRelocatePred->next = NULL;

    while ( sInPredicate != NULL )
    {
        IDE_TEST( classifyTablePredicate( aStatement,
                                          sInPredicate,
                                          aTableDependencies,
                                          aOuterDependencies,
                                          aStatiscalData )
                  != IDE_SUCCESS );

        // 컬럼별로 연결관계 만들기
        // 동일 컬럼이 있는 경우, 동일 컬럼의 마지막 predicate.more에
        // 동일 컬럼이 없는 경우, sRelocate의 마지막 predicate.next에
        // (1) 새로운 predicate(sInPredicate)을 연결하고,
        // (2) sInPredicate = sInPredicate->next
        // (3) 연결된 predicate의 next 연결관계를 끊음.

        sNextPredicate = sInPredicate->next;

        IDE_TEST( linkPred4ColumnID( sRelocatePred,
                                     sInPredicate )
                  != IDE_SUCCESS );

        sInPredicate = sNextPredicate;
    }

    //--------------------------------------
    // 재배치 완료후, indexable column에 대한 IN(subquery)의 처리
    // IN(subquery)는 다른 컬럼과 같이 keyRange를 구성하지 못하고,
    // 단독으로 구성해야하므로, keyRange추출전에 다음과 같이 처리한다.
    // IN(subquery)가 존재하는 컬럼인 경우,
    // selectivity가 좋은 predicate을 찾는다.
    // (1) 찾은 predicate이 IN(subquery)이면,
    //     IN(subquery)이외의 predicate은 non-indexable column 리스트에 연결
    // (2) 찾은 predicate이 IN(subquery)가 아니면,
    //     IN(subquery) predicate들을 non-indexable column 리스트에 연결
    //--------------------------------------

    IDE_TEST( processIndexableInSubQ( & sRelocatePred )
              != IDE_SUCCESS );

    //--------------------------------------
    // 재배치 완료후,
    // 대표 selectivity와
    // one column에 대한
    //     (1) 다음인덱스 사용가능여부
    //     (2) equal(in)연산자 존재여부에 대한 정보 설정
    //--------------------------------------

    for ( sPredicate  = sRelocatePred;
          sPredicate != NULL;
          sPredicate  = sPredicate->next )
    {
        // PROJ-2242
        IDE_TEST( qmoSelectivity::setTotalSelectivity( aStatement,
                                                       aStatiscalData,
                                                       sPredicate )
                  != IDE_SUCCESS );

        setCompositeKeyUsableFlag( sPredicate );
    }

    *aOutPredicate = sRelocatePred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::classifyJoinPredicate( qcStatement  * aStatement,
                                qmoPredicate * aPredicate,
                                qmgGraph     * aLeftChildGraph,
                                qmgGraph     * aRightChildGraph,
                                qcDepInfo    * aFromDependencies )
{
/***********************************************************************
 *
 * Description : Join predicate에 대한 분류
 *
 * Implementation :
 *
 *     join predicate으로 분류된 predicate에 대해,
 *     어떤 join method의 predicate으로 사용될 수 있는지의 정보와
 *     사용가능 join method의 수행 가능 방향성에 대한 정보를
 *     qmoPredicate.flag에 설정한다.
 *
 *     1. joinable prediate인지를 검사.
 *        OR 하위노드가 2개 이상인 경우는, index nested loop join만
 *        가능하므로, 2의(1)에 대해서만 처리한다.
 *        예) t1.i1=t2.i1 OR t1.i1=t2.i2
 *     2. joinable predicate이면,
 *        index nested loop join, sort join, hash join, merge join의
 *        사용가능여부와 join 수행가능방향에 대한 정보를 설정한다.
 *        (1) index nested loop join 사용가능 판단
 *        (2) OR 노드 하위가 하나인 경우만 hash/sort/merge join 사용가능 판단
 *            I)  sort join 사용가능 판단 : [=,>,>=,<,<=]모두 동일하게 처리됨
 *            II) hash join 사용가능 판단 : [=] hash 사용가능
 *            III) merge join 사용가능 판단
 *                [ >, >=, <, <= ] : merge join 사용 가능
 *                . =     : merge join 양방향 모두 수행가능
 *                . >, >= : left->right 로 수행가능
 *                . <, <= : right->left 로 수행가능
 *
 ***********************************************************************/

    idBool         sIsOnlyIndexNestedLoop;
    qtcNode      * sNode;
    qmoPredicate * sCurPredicate;

    IDU_FIT_POINT_FATAL( "qmoPred::classifyJoinPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement        != NULL );
    IDE_DASSERT( aPredicate        != NULL );
    IDE_DASSERT( aLeftChildGraph   != NULL );
    IDE_DASSERT( aRightChildGraph  != NULL );
    IDE_DASSERT( aFromDependencies != NULL );

    //--------------------------------------
    // join predicate 분류
    //--------------------------------------

    sCurPredicate = aPredicate;

    while ( sCurPredicate != NULL )
    {
        //--------------------------------------
        // joinable predicate인지를 판단한다.
        //--------------------------------------

        IDE_TEST( isJoinablePredicate( sCurPredicate,
                                       aFromDependencies,
                                       & aLeftChildGraph->depInfo,
                                       & aRightChildGraph->depInfo,
                                       &sIsOnlyIndexNestedLoop )
                  != IDE_SUCCESS );

        if ( ( sCurPredicate->flag & QMO_PRED_JOINABLE_PRED_MASK )
             == QMO_PRED_JOINABLE_PRED_TRUE )
        {
            //--------------------------------------
            // joinable predicate에 대한
            // join method와 join 수행가능방향 설정
            //--------------------------------------

            //--------------------------------------
            // indexable join predicate에 대한 정보 설정
            //-------------------------------------
            IDE_TEST( isIndexableJoinPredicate( aStatement,
                                                sCurPredicate,
                                                aLeftChildGraph,
                                                aRightChildGraph )
                      != IDE_SUCCESS );

            if ( sIsOnlyIndexNestedLoop == ID_TRUE )
            {
                // OR 노드 하위에 비교연산자가 여러개 있는 경우로,
                // index nested loop join method만 사용가능하므로 이하 skip

                // Nothing To Do
            }
            else
            {
                //--------------------------------------
                // 비교연산자 정보로 sort, hash, merge join에 대한 정보 설정
                //-------------------------------------

                // 비교연산자 노드를 찾는다.
                sNode = sCurPredicate->node;

                if ( ( sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                     == MTC_NODE_LOGICAL_CONDITION_TRUE )
                {
                    sNode = (qtcNode *)(sNode->node.arguments);
                }
                else
                {
                    // Nothing To Do
                }

                //-------------------------------------
                // sort joinable predicate에 대한 정보 설정
                // sort joinable predicate은
                // [ =, >, >=, <, <= ]에 동일하게 적용됨.
                //-------------------------------------

                IDE_TEST( isSortJoinablePredicate( sCurPredicate,
                                                   sNode,
                                                   aFromDependencies,
                                                   aLeftChildGraph,
                                                   aRightChildGraph )
                          != IDE_SUCCESS );

                //-------------------------------------
                // hash joinable predicate에 대한 정보 설정
                // [ = ] : hash 가능
                //-------------------------------------

                IDE_TEST( isHashJoinablePredicate( sCurPredicate,
                                                   sNode,
                                                   aFromDependencies,
                                                   aLeftChildGraph,
                                                   aRightChildGraph )
                          != IDE_SUCCESS );

                //-------------------------------------
                // merge joinable predicate에 대한 정보 설정
                // 1. 인덱스 생성 가능 여부 검사
                //    sort와 동일. 이미 sort에서 검사했으므로,
                //    QMO_PRED_SORT_JOINABLE_TRUE 이면, 인덱스 생성가능
                // 2. 인덱스 생성가능하면, 아래 정보 설정.
                //    [ =, >, >=, <, <= ] : merge 가능
                //    [ = ]    : 양방향 수행가능
                //    [ >,>= ] : left->right 수행가능
                //    [ <,<= ] : right->left 수행가능
                //-------------------------------------

                IDE_TEST( isMergeJoinablePredicate( sCurPredicate,
                                                    sNode,
                                                    aFromDependencies,
                                                    aLeftChildGraph,
                                                    aRightChildGraph )
                          != IDE_SUCCESS );
            }

            //---------------------------------------
            // 사용가능한 join method가 없는 경우,
            // non-joinable predicate으로 저장한다.
            // 예: t1.i1+1 = t2.i1+1 OR t1.i2+1 = t2.i2+1
            //     OR 노드 하위에 여러개의 비교연산자가 있어
            //     joinable predicate으로 판단되었으나,
            //     이후, index nested loop join method를 사용하지 못할 경우...
            //----------------------------------------
            if ( ( sCurPredicate->flag & QMO_PRED_JOINABLE_MASK )
                 == QMO_PRED_JOINABLE_FALSE )
            {
                sCurPredicate->flag &= ~QMO_PRED_JOINABLE_PRED_MASK;
                sCurPredicate->flag |= QMO_PRED_JOINABLE_PRED_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // Nothing To Do
        }

        sCurPredicate = sCurPredicate->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::makeJoinPushDownPredicate( qcStatement    * aStatement,
                                    qmoPredicate   * aNewPredicate,
                                    qcDepInfo      * aRightDependencies )
{
/***********************************************************************
 *
 * Description : Index Nested Loop Join의 push down joinable predicate
 *
 *   (1) Inner Join (2) Left Outer Join (3) Anti Outer Join 처리시
 *   join graph에서 selection graph로 join index 활용을 위해 join-push predicate로
 *   만든다.
 *
 *   *aNewPredicate : add될 joinable predicate의 연결리스트
 *                    joinable predicate도 predicate 재배치와 동일하게
 *                    컬럼별로 분리되어 있다.
 *
 *
 * Implementation :
 *
 *     1. joinable predicate에 indexArgument와 columID 설정
 *
 ***********************************************************************/

    UInt           sColumnID;
    qmoPredicate * sJoinPredicate;
    qmoPredicate * sPredicate;

    IDU_FIT_POINT_FATAL( "qmoPred::makeJoinPushDownPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNewPredicate != NULL );
    IDE_DASSERT( aRightDependencies != NULL );

    //--------------------------------------
    // joinable predicate의 indexArgument와 columnID 설정.
    //--------------------------------------

    for ( sJoinPredicate  = aNewPredicate;
          sJoinPredicate != NULL;
          sJoinPredicate  = sJoinPredicate->next )
    {
        for ( sPredicate  = sJoinPredicate;
              sPredicate != NULL;
              sPredicate  = sPredicate->more )
        {
            // indexArgument 설정
            IDE_TEST( setIndexArgument( sPredicate->node,
                                        aRightDependencies )
                      != IDE_SUCCESS );
            // columnID 설정
            IDE_TEST( getColumnID( aStatement,
                                   sPredicate->node,
                                   ID_TRUE,
                                   & sColumnID )
                      != IDE_SUCCESS );

            sPredicate->id = sColumnID;

            // To fix BUG-17575
            sPredicate->flag &= ~QMO_PRED_JOIN_PRED_MASK;
            sPredicate->flag |= QMO_PRED_JOIN_PRED_TRUE;

            sPredicate->flag &= ~QMO_PRED_INDEX_NESTED_JOINABLE_MASK;
            sPredicate->flag |= QMO_PRED_INDEX_NESTED_JOINABLE_TRUE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::makeNonJoinPushDownPredicate( qcStatement   * aStatement,
                                       qmoPredicate  * aNewPredicate,
                                       qcDepInfo     * aRightDependencies,
                                       UInt            aDirection )
{
/***********************************************************************
 *
 * Description : push down non-joinable predicate
 *
 *     Index Nested Loop Join의 경우,
 *     join graph에서 selection graph로 non-joinable predicate을 내리게 되며,
 *     Full Nested Loop Join의 경우,
 *     right graph가 selection graph이면, join predicate을 내리게 된다.
 *
 *     *aNewPredicate : add될 join predicate의 연결리스트
 *                      non-joinable predicate은 컬럼별로 분리되어 있지 않다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt           sJoinDirection;
    UInt           sColumnID;
    idBool         sIsIndexable;
    qmoPredicate * sJoinPredicate;

    IDU_FIT_POINT_FATAL( "qmoPred::makeNonJoinPushDownPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNewPredicate != NULL );
    IDE_DASSERT( aRightDependencies != NULL );

    //--------------------------------------
    // non-joinable predicate에 대한 indexArgument와 columnID 설정
    //  . indexable     : indexArgument 와 columnID(smiColumn.id) 설정
    //  . non-indexable : columnID(QMO_COLUMNID_NON_INDEXABLE) 설정
    //--------------------------------------

    // non-joinable predicate의 indexable여부를 판단하기 위해,
    // direction정보를 인자로 받는다.
    sJoinDirection = QMO_PRED_CLEAR;

    if ( ( aDirection & QMO_JOIN_METHOD_DIRECTION_MASK )
         == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
    {
        sJoinDirection = QMO_PRED_INDEX_LEFT_RIGHT;
    }
    else
    {
        sJoinDirection = QMO_PRED_INDEX_RIGHT_LEFT;
    }

    // non-joinable predicate은 컬럼별로 분리되어 있지 않다.

    for ( sJoinPredicate  = aNewPredicate;
          sJoinPredicate != NULL;
          sJoinPredicate  = sJoinPredicate->next )
    {
        // BUG-11519 fix
        // index joinable이 true이고
        // join 수행방향이 sJoinDirection과 같거나 양방향이면
        // indexable을 true로 세팅한다.
        if ( ( sJoinPredicate->flag & QMO_PRED_INDEX_JOINABLE_MASK )
             == QMO_PRED_INDEX_JOINABLE_TRUE &&
             ( ( sJoinPredicate->flag & QMO_PRED_INDEX_DIRECTION_MASK )
               == sJoinDirection ||
               ( sJoinPredicate->flag & QMO_PRED_INDEX_DIRECTION_MASK )
               == QMO_PRED_INDEX_BIDIRECTION ) )
        {
            // indexable predicate에 대한 indexArgument 설정
            IDE_TEST( setIndexArgument( sJoinPredicate->node,
                                        aRightDependencies )
                      != IDE_SUCCESS );

            sIsIndexable = ID_TRUE;
        }
        else
        {
            sIsIndexable = ID_FALSE;
        }

        // columnID 설정
        IDE_TEST( getColumnID( aStatement,
                               sJoinPredicate->node,
                               sIsIndexable,
                               & sColumnID )
                  != IDE_SUCCESS );

        sJoinPredicate->id = sColumnID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isRownumPredicate( qmoPredicate  * aPredicate,
                            idBool        * aIsTrue )
{
/***********************************************************************
 *
 * Description
 *     PROJ-1405
 *     rownum predicate의 판단
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoPred::isRownumPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aIsTrue    != NULL );

    //--------------------------------------
    // rownum predicate의 판단
    //--------------------------------------

    if ( ( aPredicate->node->lflag & QTC_NODE_ROWNUM_MASK )
         == QTC_NODE_ROWNUM_EXIST )
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
qmoPred::isConstantPredicate( qmoPredicate  * aPredicate       ,
                              qcDepInfo     * aFromDependencies,
                              idBool        * aIsTrue            )
{
/***********************************************************************
 *
 * Description : constant predicate의 판단
 *
 *     constant predicate은 FROM절의 table과 상관없는 predicate이다.
 *     예)  1 = 1
 *
 *   <LEVEL, PRIOR column 처리시>
 *   LEVEL = 1, PRIOR I1 = 1과 같은 경우는 처리되는 곳에 따라,
 *   constant predicate이 아님에도 불구하고, constant predicate으로
 *   분류될 수 있다. 따라서, graph에서는 다음과 같이 처리해야 한다.
 *
 *   1. LEVEL pseudo column
 *      (1) hierarchy query인 경우
 *          . where절 처리시 :
 *            LEVEL = 1 과 같은 경우는, filter로 처리되어야 한다.
 *            (constant filter로 처리되면 안됨.)
 *            즉, constant filter이면, LEVEL column이 존재하는지를 검사해서,
 *            LEVEL column이 존재하면, filter로 처리되도록 해야 함.
 *          .connect by절 처리시 :
 *            LEVEL = 1 과 같은 경우는, level filter로 처리되어야 한다.
 *            즉, constant filter이면, LEVEL column이 존재하는지를 검사해서,
 *            LEVEL column이 존재하면, level filter로 처리되도록 해야 함.
 *
 *      (2) hierarchy query가 아닌 경우,
 *         .where절 처리시, constant predicate으로 처리.
 *
 *   2. PRIOR column
 *      (1) hierarchy query인 경우
 *          constant filter이면, PRIOR column이 존재하는지를 검사해서,
 *          PRIOR column이 존재하면, filter로 처리되도록 해야 함.
 *          (constant filter가 아님.)
 *      (2) hierarchy query가 아닌 경우, PRIOR column을 사용할 수 없다.
 *
 * Implementation :
 *
 *    최상위 노드에서 아래의 조건을 검사.
 *    ( predicate의 dependencies & FROM의 dependencies ) == 0
 *
 ***********************************************************************/

    qcDepInfo  sAndDependencies;

    IDU_FIT_POINT_FATAL( "qmoPred::isConstantPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate        != NULL );
    IDE_DASSERT( aFromDependencies != NULL );
    IDE_DASSERT( aIsTrue           != NULL );

    //--------------------------------------
    // constant predicate의 판단
    // dependencies는 최상위 노드에서 판단한다.
    //--------------------------------------

    // predicate의 dependencies & FROM의 dependencies
    qtc::dependencyAnd ( & aPredicate->node->depInfo,
                         aFromDependencies,
                         & sAndDependencies );

    // (predicate의 dependencies & FROM의 dependencies)의 결과가 0인지 검사
    if ( qtc::dependencyEqual( & sAndDependencies,
                               & qtc::zeroDependencies ) == ID_TRUE )
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
qmoPred::isOneTablePredicate( qmoPredicate  * aPredicate         ,
                              qcDepInfo     * aFromDependencies  ,
                              qcDepInfo     * aTableDependencies ,
                              idBool        * aIsTrue             )
{
/***********************************************************************
 *
 * Description : one table predicate의 판단
 *
 *     one table predicate: FROM절의 table 중 하나에만 속하는 predicate
 *     예) T1.i1 = 1
 *
 * Implementation :
 *
 *    최상위 노드에서 아래의 조건을 검사.
 *    (  ( predicate의 dependencies & FROM의 dependencies )
 *       & ~(FROM의 해당 table의 dependencies)    ) == 0
 *
 ***********************************************************************/

    qcDepInfo  sAndDependencies;

    IDU_FIT_POINT_FATAL( "qmoPred::isOneTablePredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate          != NULL );
    IDE_DASSERT( aFromDependencies   != NULL );
    IDE_DASSERT( aTableDependencies  != NULL );
    IDE_DASSERT( aIsTrue             != NULL );

    //--------------------------------------
    // one table predicate 판단
    // dependencies는 최상위 노드에서 판단한다.
    //--------------------------------------

    // predicate의 dependencies & FROM의 dependencies
    qtc::dependencyAnd( & aPredicate->node->depInfo,
                        aFromDependencies,
                        & sAndDependencies );

    if ( qtc::dependencyContains( aTableDependencies,
                                  & sAndDependencies ) == ID_TRUE )
    {
        // (1) select * from t1
        //     where i1 in ( select * from t2
        //                   where t1.i1=t2.i1 );
        // (2) select * from t1
        //     where exists( select * from t2
        //                   where t1.i1=t2.i1 );
        // (1),(2)의 질의문의 경우,
        // subquery의  t1.i1=t2.i1은 variable predicate으로 분류되어야 한다.

        *aIsTrue = ID_TRUE;

        if ( qtc::dependencyContains( aFromDependencies,
                                      & aPredicate->node->depInfo ) == ID_TRUE )
        {
            // BUG-28470
            // Inner Join된 테이블들은 논리적으로 하나의 테이블로 본다.
            // 그 때문에 where절에서 Inner Join된 테이블들을 사용한 Predicate은
            // One Table Predicate으로 분류되지만 Selectivity 계산 시에
            // 레코드를 참조할 수 없으므로 Variable Predicate으로 분류되어야 한다.
            // 예) select * from t1 inner join t2 on t1.i1=t2.i1
            //     where  t1.i2 > substr(t2.i2)
            if ( ( aTableDependencies->depCount > 1 )
                 && ( aPredicate->node->depInfo.depCount > 1 ) )
            {
                // variable predicate으로 분류되어야 함.
                aPredicate->flag &= ~QMO_PRED_VALUE_MASK;
                aPredicate->flag |= QMO_PRED_VARIABLE;
            }
            else
            {
                // Nothing To Do
                // fixed predicate
            }
        }
        else
        {
            // variable predicate으로 분류되어야 함.
            aPredicate->flag &= ~QMO_PRED_VALUE_MASK;
            aPredicate->flag |= QMO_PRED_VARIABLE;
        }
    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::optimizeSubqueries( qcStatement  * aStatement,
                             qmoPredicate * aPredicate,
                             idBool         aTryKeyRange )
{
/***********************************************************************
 *
 * Description : qmoPredicate->node에 존재하는 모든 subquery에 대한 처리
 *
 *  selection graph에서 myPredicate에 달려있는 predicate들에 대해서,
 *  subquery에 대한 처리를 위해, 이 함수를 호출하게 된다.
 *
 * Implementation :
 *
 *     qmoPredicate->node에 존재하는 모든 subquery를 찾아서,
 *     이에 대해 최적화 팁 적용 및 graph를 생성한다.
 *
 ***********************************************************************/

    qmoPredicate * sPredicate;
    idBool         sIsConstantPred = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoPred::optimizeSubqueries::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate != NULL ); // selection graph의 myPredicate

    //--------------------------------------
    // predicate에 존재하는 모든 subquery의 처리
    //--------------------------------------

    if ( ( aPredicate->flag & QMO_PRED_CONSTANT_FILTER_MASK )
         == QMO_PRED_CONSTANT_FILTER_TRUE )
    {
        sIsConstantPred = ID_TRUE;
    }
    else
    {
        sIsConstantPred = ID_FALSE;
    }

    for ( sPredicate  = aPredicate;
          sPredicate != NULL;
          sPredicate  = sPredicate->next )
    {
        IDE_TEST( optimizeSubqueryInNode( aStatement,
                                          sPredicate->node,
                                          aTryKeyRange,
                                          sIsConstantPred )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::optimizeSubqueryInNode( qcStatement  * aStatement,
                                 qtcNode      * aNode,
                                 idBool         aTryKeyRange,
                                 idBool         aConstantPred )
{
/***********************************************************************
 *
 * Description : qtcNode에 존재하는 모든 subquery에 대한 처리
 *
 * Implementation :
 *
 *     qtcNode에 존재하는 모든 subquery를 찾아서,
 *     이에 대해 최적화 팁 적용 및 graph를 생성한다.
 *
 *     이 함수는 qmoPred::optimizeSubqueries()
 *               qmgGrouping::optimize() 에서 호출된다.
 *
 *     qmgGrouping에서는 aggr, group, having에 대한 subquery 처리를 위해
 *     이 함수를 호출한다.
 *     (1) aggr, group은 조건절이 아님. (2) having은 조건절임.
 *
 ***********************************************************************/

    qtcNode      * sNode;

    IDU_FIT_POINT_FATAL( "qmoPred::optimizeSubqueryInNode::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );

    //--------------------------------------
    // qtcNode에 존재하는 모든 subquery의 처리
    //--------------------------------------

    if ( ( aNode->lflag & QTC_NODE_SUBQUERY_MASK )
         == QTC_NODE_SUBQUERY_EXIST )
    {
        if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
             == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sNode = (qtcNode *)(aNode->node.arguments);

            while ( sNode != NULL )
            {
                IDE_TEST( optimizeSubqueryInNode( aStatement,
                                                  sNode,
                                                  aTryKeyRange,
                                                  aConstantPred )
                          != IDE_SUCCESS );

                sNode = (qtcNode *)(sNode->node.next);
            }
        }
        else
        {
            // grouping graph에서 aggr, group에 대한 처리

            // To Fix BUG-9522
            sNode = aNode;

            // fix BUG-12934
            // constant filter인 경우, store and search를 하지 않기 위해
            // 임시정보 저장
            if ( aConstantPred == ID_TRUE )
            {
                sNode->lflag &= ~QTC_NODE_CONSTANT_FILTER_MASK;
                sNode->lflag |= QTC_NODE_CONSTANT_FILTER_TRUE;
            }
            else
            {
                // Nothing To Do
            }

            IDE_TEST( qmoSubquery::optimize( aStatement,
                                             sNode,
                                             aTryKeyRange )
                      != IDE_SUCCESS );

            // 임시로 저장된 정보 제거
            sNode->lflag &= ~QTC_NODE_CONSTANT_FILTER_MASK;
            sNode->lflag |= QTC_NODE_CONSTANT_FILTER_FALSE;
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::separateJoinPred( qmoPredicate  * aPredicate,
                           qmoPredInfo   * aJoinablePredInfo,
                           qmoPredicate ** aJoinPred,
                           qmoPredicate ** aNonJoinPred )
{
/***********************************************************************
 *
 * Description : joinable predicate과 non-joinable predicate을 분리한다.
 *
 * 1. 인자로 넘어온 join predicate list와 joinable predicate Info의
 *    연결관계는 다음과 같다.
 *                                 aPredicate [p1]-[p2]-[p3]-[p4]-[p5]
 *                      ________________________|    |              |
 *                      |                            |              |
 *                      |                            |              |
 *  aJoinablePredInfo [Info1]-->[Info2]______________|              |
 *                                |                                 |
 *                                |                                 |
 *                               \ /                                |
 *                             [Info2]______________________________|
 *
 * 2. 분리배치된 모습
 *    (1) joinable Predicate    (2) non-joinable predicate
 *        [p1]-[p2]                 [p3]->[p4]
 *              |
 *             [p5]
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredInfo  * sPredInfo;
    qmoPredInfo  * sMorePredInfo;
    qmoPredicate * sJoinPredList = NULL;
    qmoPredicate * sLastPredList;
    qmoPredicate * sPredicateList;
    qmoPredicate * sPredicate;
    qmoPredicate * sPrevPredicate;
    qmoPredicate * sNextPredicate;
    qmoPredicate * sTempList        = NULL;
    qmoPredicate * sMoreTempList    = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::separateJoinPred::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    if ( aJoinablePredInfo != NULL )
    {
        IDE_DASSERT( aPredicate != NULL );
    }
    else
    {
        // Nothing To Do
    }
    IDE_DASSERT( aJoinPred != NULL );
    IDE_DASSERT( aNonJoinPred != NULL );

    //--------------------------------------
    // joinable predicate 분리
    //--------------------------------------

    sPredicateList = aPredicate;  // aPredicate은 join predicate list

    for ( sPredInfo  = aJoinablePredInfo;
          sPredInfo != NULL;
          sPredInfo  = sPredInfo->next )
    {
        sTempList = NULL;

        for ( sMorePredInfo  = sPredInfo;
              sMorePredInfo != NULL;
              sMorePredInfo  = sMorePredInfo->more )
        {
            for ( sPrevPredicate  = NULL,
                      sPredicate  = sPredicateList;
                  sPredicate     != NULL;
                  sPrevPredicate  = sPredicate,
                      sPredicate  = sNextPredicate )
            {
                sNextPredicate = sPredicate->next;

                if ( sMorePredInfo->predicate == sPredicate )
                {
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }

            IDE_TEST_RAISE( sPredicate == NULL, ERR_INVALID_PREDICATE_LIST );

            if ( sTempList == NULL )
            {
                sTempList     = sPredicate;
                sMoreTempList = sTempList;
            }
            else
            {
                sMoreTempList->more = sPredicate;
                sMoreTempList       = sMoreTempList->more;
            }

            // join predicate list에서의 연결을 끊는다.
            if ( sPrevPredicate == NULL )
            {
                sPredicateList = sPredicate->next;
            }
            else
            {
                sPrevPredicate->next = sPredicate->next;
            }

            sPredicate->next = NULL;
        }

        if ( sJoinPredList == NULL )
        {
            sJoinPredList = sTempList;
            sLastPredList = sJoinPredList;
        }
        else
        {
            sLastPredList->next = sTempList;
            sLastPredList       = sLastPredList->next;
        }
    }

    *aJoinPred = sJoinPredList;

    //--------------------------------------
    // non-joinable predicate 의 연결
    // sPredicateList가 NULL일수도 있다.
    // 이 경우,연결과정에서 sPredicate이 NULL로 세팅됨.
    //--------------------------------------

    *aNonJoinPred = sPredicateList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_PREDICATE_LIST );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoPred::separateJoinPred",
                                  "invalid predicate list" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

qtcNode*
qmoPred::getColumnNodeOfJoinPred( qcStatement  * aStatement,
                                  qmoPredicate * aPredicate,
                                  qcDepInfo    * aDependencies )
{
/***********************************************************************
 *
 * Description : BUG-24673
 *               Join predicate에 대한 columnNode 를
 *               반환한다.
 *
 * Implementation :
 *
 *     join predicate에 필요한 preserved order검사시 사용한다.
 *
 ***********************************************************************/

    idBool    sIsFirstNode = ID_TRUE;
    idBool    sIsIndexable = ID_TRUE;
    qtcNode * sCompareNode;
    qtcNode * sCurNode;
    qtcNode * sColumnNode = NULL;
    qtcNode * sFirstColumnNode = NULL;

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aDependencies != NULL );

    //--------------------------------------
    // join predicate의 columnID 설정
    //--------------------------------------

    // join predicate 하위에 여러개의 비교연산자 노드가 올 수도 있으므로,
    // 이에 대한 고려가 필요함. ( 이 경우는 index nested loop join만 가능 )

    if( ( aPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
        == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        // CNF 인 경우

        sCompareNode = (qtcNode *)(aPredicate->node->node.arguments);

        while( sCompareNode != NULL )
        {
            sCurNode = (qtcNode*)(sCompareNode->node.arguments);

            // To Fix PR-8025
            // Dependency에 부합하는 Argument를 구함.
            // 인자로 넘어온 하위 selection graph와
            // dependencies가 동일한지 검사.
            if( qtc::dependencyEqual( aDependencies,
                                      & sCurNode->depInfo ) == ID_TRUE )
            {
                sCompareNode->indexArgument = 0;
            }
            else
            {
                if( ( sCompareNode->node.module == &mtfEqual )
                    || ( sCompareNode->node.module == &mtfNotEqual )
                    || ( sCompareNode->node.module == &mtfGreaterThan )
                    || ( sCompareNode->node.module == &mtfGreaterEqual )
                    || ( sCompareNode->node.module == &mtfLessThan )
                    || ( sCompareNode->node.module == &mtfLessEqual ) )
                {
                    sCompareNode->indexArgument = 1;
                    sCurNode = (qtcNode *) sCurNode->node.next;
                }
                else
                {
                    // =, !=, >, >=, <, <= 을 제외한 모든 비교연산자는
                    // 비교연산자의 argument에 column node가 존재한다.
                    // 따라서, 이 연산자들이 오는 경우,
                    // 해당 테이블의 컬럼이 아닌 경우,

                    sCurNode = NULL;
                }
            }

            if( sCurNode != NULL )
            {
                if ( QTC_IS_COLUMN( aStatement, sCurNode ) == ID_TRUE )
                {
                    // 비교연산자의 argument가 컬럼인 경우,
                    // columnNode를 구한다.
                    sColumnNode = sCurNode;    
                }
                else
                {
                    // Index를 사용할 수 없는 경우임.
                    sIsIndexable = ID_FALSE;
                }
            }
            else
            {
                sIsIndexable = ID_FALSE;
            }

            if( sIsIndexable == ID_TRUE )
            {
                if( sIsFirstNode == ID_TRUE )
                {
                    // OR 노드 하위의 첫번째 비교연산자 처리시,
                    // 이후 column 비교를 위해,
                    // sFirstColumn에 column를 저장.
                    sFirstColumnNode = sColumnNode;
                    sIsFirstNode = ID_FALSE;
                }
                else
                {
                    // Nothing To Do
                }
                
                // 일단 first, column node둘다 null이면 안된다.
                // null이라는 것은 indexable하지 않은 것.
                if( ( sFirstColumnNode != NULL ) &&
                    ( sColumnNode != NULL ) )
                {
                    // 서로 다른 컬럼이라면 indexable하지 않음
                    if( ( sFirstColumnNode->node.table ==
                          sColumnNode->node.table ) &&
                        ( sFirstColumnNode->node.column ==
                          sColumnNode->node.column ) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsIndexable = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    sIsIndexable = ID_FALSE;
                    break;
                }
            }
            else
            {
                break;
            }

            sCompareNode = (qtcNode *)(sCompareNode->node.next);
        }
    }
    else
    {
        // DNF 인 경우

        sCompareNode = aPredicate->node;
        sCurNode = (qtcNode*)(sCompareNode->node.arguments);

        // To Fix PR-8025
        // Dependency에 부합하는 Argument를 구함.
        // 인자로 넘어온 하위 selection graph와
        // dependencies가 동일한지 검사.
        if( qtc::dependencyEqual( aDependencies,
                                  & sCurNode->depInfo ) == ID_TRUE )
        {
            sCompareNode->indexArgument = 0;
        }
        else
        {
            if( ( sCompareNode->node.module == &mtfEqual )
                || ( sCompareNode->node.module == &mtfNotEqual )
                || ( sCompareNode->node.module == &mtfGreaterThan )
                || ( sCompareNode->node.module == &mtfGreaterEqual )
                || ( sCompareNode->node.module == &mtfLessThan )
                || ( sCompareNode->node.module == &mtfLessEqual ) )
            {
                sCompareNode->indexArgument = 1;
                sCurNode = (qtcNode *) sCurNode->node.next;
            }
            else
            {
                // =, !=, >, >=, <, <= 을 제외한 모든 비교연산자는
                // 비교연산자의 argument에 column node가 존재한다.
                // 따라서, 이 연산자들이 오는 경우,
                // 해당 테이블의 컬럼이 아닌 경우,

                sCurNode = NULL;
            }
        }

        if( sCurNode != NULL )
        {
            if ( QTC_IS_COLUMN( aStatement, sCurNode ) == ID_TRUE )
            {
                // 비교연산자의 argument가 컬럼인 경우,
                // columnID를 구한다.
                sColumnNode = sCurNode;
            }
            else
            {
                // Index를 사용할 수 없는 경우임.
                sIsIndexable = ID_FALSE;
            }
        }
        else
        {
            sIsIndexable = ID_FALSE;
        }
    }

    if( sIsIndexable == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sColumnNode = NULL;
    }

    return sColumnNode;
}


IDE_RC
qmoPred::setColumnIDToJoinPred( qcStatement  * aStatement,
                               qmoPredicate * aPredicate,
                               qcDepInfo    * aDependencies )
{
/***********************************************************************
 *
 * Description : Join predicate에 대한 columnID 정보를
 *               qmoPredicate.id에 저장한다.
 *
 * Implementation :
 *
 *     join order 결정과정에서 join graph의 하위가 selection graph일 경우,
 *     composite index에 대한 selectivity 보정을 하게 되는데,
 *     join predicate의 composite index의 컬럼포함여부를 검사하기 위해
 *     graph에서 호출하게 된다.
 *
 *     columnID 정보를 qmoPredicate.flag에 저장한다.
 *
 ***********************************************************************/

    idBool    sIsFirstNode = ID_TRUE;
    idBool    sIsIndexable = ID_TRUE;
    UInt      sFirstColumnID = QMO_COLUMNID_NON_INDEXABLE;
    UInt      sColumnID      = QMO_COLUMNID_NON_INDEXABLE;
    qtcNode * sCompareNode;
    qtcNode * sCurNode;

    IDU_FIT_POINT_FATAL( "qmoPred::setColumnIDToJoinPred::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aDependencies != NULL );

    //--------------------------------------
    // join predicate의 columnID 설정
    //--------------------------------------

    // join predicate 하위에 여러개의 비교연산자 노드가 올 수도 있으므로,
    // 이에 대한 고려가 필요함. ( 이 경우는 index nested loop join만 가능 )

    if ( ( aPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        // CNF 인 경우

        sCompareNode = (qtcNode *)(aPredicate->node->node.arguments);

        while ( sCompareNode != NULL )
        {
            sCurNode = (qtcNode*)(sCompareNode->node.arguments);

            // To Fix PR-8025
            // Dependency에 부합하는 Argument를 구함.
            // 인자로 넘어온 하위 selection graph와
            // dependencies가 동일한지 검사.
            if ( qtc::dependencyEqual( aDependencies,
                                       & sCurNode->depInfo ) == ID_TRUE )
            {
                sCompareNode->indexArgument = 0;
            }
            else
            {
                if ( ( sCompareNode->node.module == &mtfEqual )
                     || ( sCompareNode->node.module == &mtfNotEqual )
                     || ( sCompareNode->node.module == &mtfGreaterThan )
                     || ( sCompareNode->node.module == &mtfGreaterEqual )
                     || ( sCompareNode->node.module == &mtfLessThan )
                     || ( sCompareNode->node.module == &mtfLessEqual ) )
                {
                    sCompareNode->indexArgument = 1;
                    sCurNode = (qtcNode *) sCurNode->node.next;
                }
                else
                {
                    // =, !=, >, >=, <, <= 을 제외한 모든 비교연산자는
                    // 비교연산자의 argument에 column node가 존재한다.
                    // 따라서, 이 연산자들이 오는 경우,
                    // 해당 테이블의 컬럼이 아닌 경우,

                    sCurNode = NULL;
                }
            }

            if ( sCurNode != NULL )
            {
                if ( QTC_IS_COLUMN( aStatement, sCurNode ) == ID_TRUE )
                {
                    // 비교연산자의 argument가 컬럼인 경우,
                    // columnID를 구한다.
                    sColumnID =
                        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sCurNode->node.table].
                        columns[sCurNode->node.column].column.id;
                }
                else
                {
                    // Index를 사용할 수 없는 경우임.
                    sIsIndexable = ID_FALSE;
                }
            }
            else
            {
                sIsIndexable = ID_FALSE;
            }

            if ( sIsIndexable == ID_TRUE )
            {
                if ( sIsFirstNode == ID_TRUE )
                {
                    // OR 노드 하위의 첫번째 비교연산자 처리시,
                    // 이후 columnID 비교를 위해,
                    // sFirstColumnID에 columnID를 저장.
                    sFirstColumnID = sColumnID;
                    sIsFirstNode   = ID_FALSE;
                }
                else
                {
                    // Nothing To Do
                }

                if ( sFirstColumnID == sColumnID )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsIndexable = ID_FALSE;
                    break;
                }
            }
            else
            {
                break;
            }

            sCompareNode = (qtcNode *)(sCompareNode->node.next);
        }
    }
    else
    {
        // DNF 인 경우

        sCompareNode = aPredicate->node;
        sCurNode     = (qtcNode*)(sCompareNode->node.arguments);

        // To Fix PR-8025
        // Dependency에 부합하는 Argument를 구함.
        // 인자로 넘어온 하위 selection graph와
        // dependencies가 동일한지 검사.
        if ( qtc::dependencyEqual( aDependencies,
                                   & sCurNode->depInfo ) == ID_TRUE )
        {
            sCompareNode->indexArgument = 0;
        }
        else
        {
            if ( ( sCompareNode->node.module == &mtfEqual )
                 || ( sCompareNode->node.module == &mtfNotEqual )
                 || ( sCompareNode->node.module == &mtfGreaterThan )
                 || ( sCompareNode->node.module == &mtfGreaterEqual )
                 || ( sCompareNode->node.module == &mtfLessThan )
                 || ( sCompareNode->node.module == &mtfLessEqual ) )
            {
                sCompareNode->indexArgument = 1;
                sCurNode                    = (qtcNode *) sCurNode->node.next;
            }
            else
            {
                // =, !=, >, >=, <, <= 을 제외한 모든 비교연산자는
                // 비교연산자의 argument에 column node가 존재한다.
                // 따라서, 이 연산자들이 오는 경우,
                // 해당 테이블의 컬럼이 아닌 경우,

                sCurNode = NULL;
            }
        }

        if ( sCurNode != NULL )
        {
            if ( QTC_IS_COLUMN( aStatement, sCurNode ) == ID_TRUE )
            {
                // 비교연산자의 argument가 컬럼인 경우,
                // columnID를 구한다.
                sColumnID =
                    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sCurNode->node.table].
                    columns[sCurNode->node.column].column.id;
            }
            else
            {
                // Index를 사용할 수 없는 경우임.
                sIsIndexable = ID_FALSE;
            }
        }
        else
        {
            sIsIndexable = ID_FALSE;
        }
    }

    if ( sIsIndexable == ID_TRUE )
    {
        aPredicate->id = sColumnID;
    }
    else
    {
        aPredicate->id = QMO_COLUMNID_NON_INDEXABLE;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::isIndexable( qcStatement   * aStatement,
                      qmoPredicate  * aPredicate,
                      qcDepInfo     * aTableDependencies,
                      qcDepInfo     * aOuterDependencies,
                      idBool        * aIsIndexable )
{
/***********************************************************************
 *
 * Description : predicate의 indexable 여부 판단
 *
 *     <indexable predicate의 정의>
 *
 *     조건1. indexable operator 이어야 한다.
 *            system level operator 뿐만 아니라,
 *            user level operator(quantify비교연산자)도 포함된다.
 *
 *     조건2. column이 있어야 한다.
 *            예) i1=1(O), i1=i2(O), i1=i1+1(O), i1+1=1(O), 1=1(X),
 *                (i1,i2,i3)=(i1,i2,i3)(O), (i1,i2,i3)=(1,1,1)(O),
 *                (i1,i2,1 )=( 1, 1, 1)(X), (1,subquery)=(1,1)(X)
 *
 *     조건3. column에 연산이 없어야 한다.
 *
 *            예) i1=1(O), i1=i2(O), i1=i1+1(O), i1+1=1(X)
 *
 *     조건4. column이 한쪽에만 존재해야 한다.
 *            예) i1=1(O), i1=i1+1(X), i1=i2(X)
 *
 *     조건5. column에 conversion이 발생하지 않아야 한다.
 *            예) i1(integer)=smallint'1'(O), i1(integer)=3.5(X)
 *
 *     조건6. value에 대한 조건체크
 *            6-1. subquery가 오는 경우, subquery type이 A,N형이어야 한다.
 *                 예) a1 = (select i1 from t2 where i1=1)(O)
 *                     a1 = (select sum(i1) from t2)(O)
 *                     a1 = (select i1 from t2 where i1=al)(X)
 *                     a1 = (select sum(i1) from t2 where i1=a1)(X)
 *            6-2. LIKE에 대한 패턴문자는 일반문자로 시작하여야 한다.
 *                 예) i1 like 'a%'(O) , i1 like '\_a%' escape'\'(O)
 *                     i1 like '%a%'(X), i1 like '_bc'(X)
 *            6-3. host 변수가 오는 경우
 *                 예) i1=?(O), i1=?+1(O)
 *
 *     조건7. OR노드 하위에 동일한 컬럼만 있는 경우이어야 한다.
 *            단, subquery 가 오는 경우는 제외된다.
 *            예) i1=1 OR i1=2(O), (i1,i2)=(1,1) OR (i1,i2)=(2,2)(O),
 *                i1=1 OR i2=2(X),
 *                i1 in (subquery) OR i1 in (subquery)(X),
 *                i1=1 OR i1 in (subquery) (X)
 *                (i1=1 and i2=1) or i1=( subquery ) (X)
 *
 *     조건8. index가 있어야 한다.
 *            조건1~7까지로 indexable predicate이 판단되었으면,
 *            이에 해당하는 index가 있어야 한다.
 *
 *     < 질의 수행과정에서의 indexable predicate의 판단범위 >
 *
 *     (1) parsing & validation 과정에서의 판단범위
 *         조건1, 조건2, 조건3, 이 세가지 조건을 만족하면
 *         mtcNode.lflag에 MTC_NODE_INDEX_USABLE을 설정함.
 *
 *     (2) graph 생성과정에서의 판단범위
 *         조건4, 조건5, 조건6, 조건7
 *
 *     (3) plan tree 생성과정에서의 판단범위
 *         IN절의 subquery keyRange에 대한 조건과 조건8
 *
 *     (4) execution 과정에서의 판단범위
 *         host변수에 대한 binding이 끝난후,
 *         column에 대한 conversion발생여부와
 *         LIKE연산자의 패턴문자가 일반문자로 시작하는지의 여부
 *
 * Implementation : 이 함수는 graph 생성과정에서의 판단범위를 검사함.
 *
 *     1. 조건1,2,3의 판단
 *        mtcNode.flag가 MTC_NODE_INDEX_USABLE 인지를 검사
 *
 *     2. 조건4의 판단
 *        (1) operand의 dependency가 중복되지 않는지를 검사한다.
 *            (이 검사로 컬럼이 한쪽에만 존재한다는 것이 검증됨)
 *
 *            dependency 중복 판단방법은,
 *            ( ( 비교연산자하위의 두개 노드의 dependencies를 AND연산)
 *              & FROM의 해당 table의 dependency ) != 0
 *
 *        (2) (1)이 검증되면,
 *            컬럼이 모두 해당 table의 컬럼으로 구성되었는지를 검사.
 *
 *     3. 조건5의 판단
 *        (1) column
 *            value쪽에 host변수가 없는데도,
 *            column에 conversion이 발생했는지를 검사.
 *
 *        (2) LIST
 *            value쪽 LIST 하위 노드들을 모두 조사해서, host변수가 아니면서,
 *            value의 leftConversion이 존재하는지 검사.
 *
 *     4. 조건6의 판단
 *        (1) 6-1조건 : QTC_NODE_SUBQUERY_EXIST
 *        (2) 6-2조건 : subquery관리자로부터 type을 알아낸다.
 *        (3) 6-3조건 :
 *            value node의 tuple이 MTC_TUPLE_TYPE_CONSTANT인지를 검사.
 *
 *     5. 조건7의 판단
 *        (1) OR노드 하위에 1개의 비교연산자가 존재할 경우,
 *            .indexable predicate인지만 판단
 *        (2) OR노드 하위에 2개이상의 비교연산자가 존재할 경우,
 *            . subquery를 포함하지 않아야 한다.
 *            . 하위 비교연산자들이 모두 indexable이어야 한다.
 *            . 하위 노드들의 columID가 모두 같아야 한다.
 *              (단, 컬럼이 LIST인 경우는 non-indexable로 처리)
 *
 ***********************************************************************/

    UInt     sColumnID;
    UInt     sFirstColumnID;
    idBool   sIsFirstNode = ID_TRUE;
    idBool   sIsIndexablePred = ID_FALSE;
    idBool   sIsOrValueIndex = ID_FALSE;
    idBool   sIsDisable = ID_FALSE;
    qtcNode *sNode;

    IDU_FIT_POINT_FATAL( "qmoPred::isIndexable::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aIsIndexable != NULL );

    /* BUG-47986 */
    if ( QCU_OPTIMIZER_OR_VALUE_INDEX == 0 )
    {
        sIsDisable = ID_TRUE;
    }

    //--------------------------------------
    // 조건 7의 검사
    // OR노드 하위에 동일한 컬럼만 있는 경우,
    // 단, subquery 노드가 존재하는 경우는 제외된다.
    //--------------------------------------

    sNode = aPredicate->node;

    if ( ( sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        //--------------------------------------
        // CNF 인 경우로,
        // 인자로 넘어온 predicate의 최상위 노드는 OR노드이며,
        // OR 노드 하위에 여러개의 비교연산자가 올 수 있다.
        // OR 노드 하위에 비교연산자가 하나일때와 여러개일때의
        // 조건검사가 틀리므로, 이를 구분하여 처리한다.
        //--------------------------------------

        // sNode는 비교연산자 노드
        sNode = (qtcNode *)(sNode->node.arguments);

        if ( aPredicate->node->node.arguments->next == NULL )
        {

            // 1. OR 노드 하위에 비교연산자가 하나만 오는 경우,
            //    indexable인지만 판단하면 된다.
            IDE_TEST( isIndexableUnitPred( aStatement,
                                           sNode,
                                           aTableDependencies,
                                           aOuterDependencies,
                                           & sIsIndexablePred )
                      != IDE_SUCCESS );

            if ( ( sIsIndexablePred == ID_TRUE ) &&
                 ( aPredicate->node->node.arguments->module == &mtfEqual ) )
            {
                aPredicate->flag &= ~QMO_PRED_INDEXABLE_EQUAL_MASK;
                aPredicate->flag |= QMO_PRED_INDEXABLE_EQUAL_TRUE;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            // 2. OR 노드 하위에 비교연산자가 여러개 오는 경우,
            //   (1) subquery가 존재하지 않아야 한다.
            //   (2) 비교연산자가 모두 indexable predicate이어야 한다.
            //   (3) 비교연산자 노드의 columnID가 모두 동일해야 한다.
            //       (단, column이 LIST인 경우는 제외한다.)

            // subquery가 존재하지 않아야 한다.
            if ( ( aPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                sIsIndexablePred = ID_FALSE;
            }
            else
            {
                while ( sNode != NULL )
                {
                    // indexable predicate인지 검사
                    IDE_TEST( isIndexableUnitPred( aStatement,
                                                   sNode,
                                                   aTableDependencies,
                                                   aOuterDependencies,
                                                   &sIsIndexablePred )
                              != IDE_SUCCESS );

                    if ( sIsIndexablePred == ID_TRUE )
                    {
                        // columnID를 얻는다.
                        IDE_TEST( getColumnID( aStatement,
                                               sNode,
                                               ID_TRUE,
                                               & sColumnID )
                                  != IDE_SUCCESS );

                        if ( sIsFirstNode == ID_TRUE )
                        {
                            // OR 노드 하위의 첫번째 비교연산자 처리시,
                            // 이후의 columnID 비교를 위해,
                            // sFirstColumnID에 columnID를 저장.
                            sFirstColumnID = sColumnID;
                            sIsFirstNode   = ID_FALSE;
                        }
                        else
                        {
                            // Nothing To Do
                        }

                        // column이 LIST가 아닌 one column으로 구성되어 있고,
                        // 첫번째 비교연산자의 columnID와 같은지를 검사.
                        if ( ( sColumnID != QMO_COLUMNID_LIST ) &&
                             ( sColumnID == sFirstColumnID ) )
                        {
                            // Nothing To Do
                        }
                        else
                        {
                            sIsIndexablePred = ID_FALSE;
                            break;
                        }
                    }
                    else
                    {
                        // BUG-39036 select one or all value optimization
                        // keyRange or 상수값 의 형태일때 keyRange 생성이 실패하는것을 성공하게 한다.
                        // 단 ? = 1 과 같은 바인드 변수는 허용하지만 1 = 1 과 같은 형태는 허용하지 않는다.
                        // select * from t1 where :emp is null or i1 = :emp; ( o )
                        // SELECT * FROM t1 WHERE i1=1 OR 1=1; ( x )

                        // BUG-40878 koscom case
                        // SELECT count(*) FROM t1
                        // WHERE i2 = to_number('20150209') AND ((1 = 0  and i1  = :a1 ) or ( 0 = 0 and i7 = :a2 ));
                        // 위 질의에서 1 = 0  and i1  = :a1 을 indexable 로 판단하지만
                        // i2 = to_number('20150209') 과 결합하면서 결과가 틀려진다.
                        // 따라서 여러개의 predicate 가 오는 경우에는 indexable 로 판단하지 않는다.
                        if ( (qtc::haveDependencies( &sNode->depInfo ) == ID_TRUE) ||
                             (sNode->node.module == &qtc::valueModule) ||
                             (aPredicate->next != NULL) ||
                             (sIsDisable == ID_TRUE) )
                        {
                            sIsIndexablePred = ID_FALSE;
                            break;
                        }
                        else
                        {
                            sIsIndexablePred = ID_TRUE;
                            sIsOrValueIndex = ID_TRUE;
                        }
                    }

                    sNode = (qtcNode *)(sNode->node.next);
                }

                /* BUG-47509 */
                if ( sIsOrValueIndex == ID_TRUE )
                {
                    aPredicate->flag &= ~QMO_PRED_OR_VALUE_INDEX_MASK;
                    aPredicate->flag |= QMO_PRED_OR_VALUE_INDEX_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }
    else
    {
        //------------------------------------------
        // DNF 인 경우로,
        // 인자로 넘어온 predicate의 최상위 노드는 비교연산자 노드이다.
        // 아직 predicate의 연결관계가 끊어지지 않은 상태이므로,
        // 인자로 넘어온 비교연산자 노드 하나에 대해서만 처리해야 한다.
        //-------------------------------------------

        IDE_TEST( isIndexableUnitPred( aStatement,
                                       sNode,
                                       aTableDependencies,
                                       aOuterDependencies,
                                       & sIsIndexablePred )
                  != IDE_SUCCESS );

    }

    *aIsIndexable = sIsIndexablePred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qmoPred::isROWNUMColumn( qmsQuerySet * aQuerySet,
                         qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description : ROWNUM column의 판단
 *
 *     materialized node에서 ROWNUM column 처리 방식을 value형으로
 *     지정하기 위해서 해당 노드가 ROWNUM column인지를 판단한다.
 *
 * Implementation :
 *
 *      ROWNUM pseudo column은 qmsSFWGH::rownum 자료구조에
 *      qtcNode형태로 관리된다.
 *      이 qmsSFWGH::rownum 자료구조의 tupleID와 columnID를 이용한다.
 *
 *      인자로 넘어온 aNode의 tupleID, columnID가
 *      qmsSFWGH::rownum의 tupleID, columnID와 같으면,
 *      ROWNUM column으로 판단한다.
 *
 ***********************************************************************/

    qtcNode  * sNode;
    idBool     sResult;

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // ROWNUM Column의 판단
    // 검사대상 Node와 qmsSFWGH->rowum의 tupleID와 columnID가 같은지를 비교.
    //--------------------------------------

    sNode = aQuerySet->SFWGH->rownum;

    if( sNode != NULL )
    {
        // BUG-17949
        // select rownum ... group by rownum인 경우
        // SFWGH->rownum에 passNode가 달려있다.
        if( sNode->node.module == & qtc::passModule )
        {
            sNode = (qtcNode*) sNode->node.arguments;
        }
        else
        {
            // Nothing to do.
        }

        if( ( aNode->node.table == sNode->node.table ) &&
            ( aNode->node.column == sNode->node.column ) )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}


IDE_RC
qmoPred::setPriorNodeID( qcStatement  * aStatement,
                         qmsQuerySet  * aQuerySet,
                         qtcNode      * aNode       )
{
/***********************************************************************
 *
 * Description : prior column이 존재하는지 검사해서,
 *               prior column이 존재하면,  prior tuple로 변경시킨다.
 *
 *     hierarchy node가 생성되어야 prior tuple을 알 수 있으므로,
 *     plan node 생성시, prior column에 대한 tuple을 prior tuple로
 *     재설정한다.
 *     따라서, 모든 expression과 predicate이 존재하는
 *     곳에서 이 작업을 수행해야 한다. predicate 관리자는 이러한 코드가
 *     분산되지 않도록 module화하며, 모든 expression과 predicate이
 *     존재하는 곳에서는 이 함수를 호출하도록 한다.
 *
 * Implementation :
 *
 *     hierarchy가 존재하면,
 *     qtc::priorNodeSetWithNewTuple()호출
 *     [ 이 함수는, 해당 Node를 Traverse하면서,
 *       PRIOR Node의 Table ID를 새로운 Table ID로 변경한다. ]
 *
 *     prior column이 있다면, prior tuple로 변경시키고,
 *     prior column이 없다면, 아무 처리도 하지 않는다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoPred::setPriorNodeID::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // hierarchy가 존재하면, tuple을 변경한다.
    //--------------------------------------

    if ( aQuerySet->setOp == QMS_NONE )
    {
        // SET 절이 아닌 경우,
        IDE_FT_ASSERT( aQuerySet->SFWGH != NULL );

        if ( aQuerySet->SFWGH->hierarchy != NULL )
        {
            IDE_TEST( qtc::priorNodeSetWithNewTuple( aStatement,
                                                     & aNode,
                                                     aQuerySet->SFWGH->hierarchy->originalTable,
                                                     aQuerySet->SFWGH->hierarchy->priorTable )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // SET 절인 경우,
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::linkPredicate( qcStatement   * aStatement,
                        qmoPredicate  * aPredicate,
                        qtcNode      ** aNode       )
{
/***********************************************************************
 *
 * Description : keyRange, keyFilter로 추출된 qmoPredicate의
 *              연결정보로, 하나의 qtcNode의 그룹으로 연결정보를 만든다.
 *
 *     keyRange, keyFilter로 추출된 정보는 qmoPredicate의
 *     연결정보로 구성되어 있으며, 이 qmoPredicate->predicate에
 *     qtcNode형태의 predicate이 각각 분산되어 있다.
 *     keyRange 적용을 위해, 이 분산되어 있는 qtcNode의 predicate들을
 *     하나의 그룹으로 연결정보를 만든다.
 *
 * Implementation :
 *
 *     분산되어 있는 qtcNode의 predicate들을 단순 연결한다.
 *     1. 새로운 AND 노드 생성
 *     2. AND 노드 하위에 predicate들을 연결한다.
 *     3. AND 노드의 flag와 dependencies를 설정한다.
 *
 ***********************************************************************/

    qtcNode        * sANDNode[2];
    qcNamePosition   sNullPosition;
    qmoPredicate   * sPredicate;
    qmoPredicate   * sMorePredicate;
    qtcNode        * sNode = NULL;
    qtcNode        * sLastNode;

    IDU_FIT_POINT_FATAL( "qmoPred::linkPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // 새로운 AND 노드를 하나 생성한다.
    //--------------------------------------

    SET_EMPTY_POSITION( sNullPosition );

    IDE_TEST( qtc::makeNode( aStatement,
                             sANDNode,
                             & sNullPosition,
                             (const UChar*)"AND",
                             3 )
              != IDE_SUCCESS );

    //--------------------------------------
    // AND 노드 하위에
    // qmoPredicate->node에 흩어져 있는 predicate들을 연결한다.
    //--------------------------------------

    for ( sPredicate  = aPredicate;
          sPredicate != NULL;
          sPredicate  = sPredicate->next )
    {
        for ( sMorePredicate  = sPredicate;
              sMorePredicate != NULL;
              sMorePredicate  = sMorePredicate->more )
        {
            // 현재 qtcNode의 next연결관계를 끊는다.
            sMorePredicate->node->node.next = NULL;

            if ( sNode == NULL )
            {
                sNode     = sMorePredicate->node;
                sLastNode = sNode;
            }
            else
            {
                sLastNode->node.next = (mtcNode *)(sMorePredicate->node);
                sLastNode            = (qtcNode *)(sLastNode->node.next);
            }
        }
    }

    sANDNode[0]->node.arguments = (mtcNode *)&(sNode->node);

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sANDNode[0] )
              != IDE_SUCCESS );

    // 새로 생성된 AND 노드의 flag와 dependencies를 재설정하지 않아도 된다.
    // 참조 BUG-7557

    *aNode = sANDNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::linkFilterPredicate( qcStatement   * aStatement,
                              qmoPredicate  * aPredicate,
                              qtcNode      ** aNode       )
{
/***********************************************************************
 *
 * Description : filter로 추출된 qmoPredicate의 연결정보로,
 *               하나의 qtcNode의 그룹으로 연결정보를 만든다.
 *
 *     filter로 추출된 정보는 qmoPredicate의 연결정보로 구성되어 있으며,
 *     이 qmoPredicate->node에 qtcNode형태의 predicate이 각각
 *     분산되어 있다.
 *     filter 적용을 위해 분산되어 있는 qtcNode의 predicate들을
 *     하나의 그룹으로 연결정보를 만든다.
 *
 *     filter로 추출된 predicate들은 selectivity가 좋은 predicate이
 *     먼저 처리되도록 하여 선택되는 레코드의 수를 줄임으로써,
 *     성능을 향상시킨다. ( filter ordering )
 *
 * Implementation :
 *
 *     1. 각 predicate의 selectivity를 비교해서,
 *        selectivity가 좋은 순서대로 qmoPredicate을 정렬한다.
 *     2. 새로운 AND노드를 하나 생성하고,
 *        AND노드 하위에 1에서 정렬된 순서로 qtcNode를 연결한다.
 *     4. AND 노드의 flag와 dependencies를 설정한다.
 *
 ***********************************************************************/

    UInt            sIndex;
    UInt            sPredCnt = 0;
    qtcNode       * sNode;
    qtcNode       * sANDNode[2];
    qcNamePosition  sNullPosition;
    qmoPredicate  * sPredicate;
    qmoPredicate  * sMorePredicate;
    qmoPredicate ** sPredicateArray;

    IDU_FIT_POINT_FATAL( "qmoPred::linkFilterPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aPredicate != NULL );
    IDE_FT_ASSERT( aNode      != NULL );

    //--------------------------------------
    // filter ordering 적용
    // selectivity가 작은 순서대로 predicate들을 연결한다.
    //--------------------------------------

    //--------------------------------------
    // selectivity가 좋은 순서대로 정렬(qsort)하기 위해,
    // predicate의 배열을 만든다.
    //--------------------------------------

    // 인자로 넘어온 filter의 predicate 갯수를 구한다.
    for ( sPredicate  = aPredicate;
          sPredicate != NULL;
          sPredicate  = sPredicate->next )
    {
        for ( sMorePredicate  = sPredicate;
              sMorePredicate != NULL;
              sMorePredicate  = sMorePredicate->more )
        {
            sPredCnt ++;
        }
    }

    // qmoPredicate 배열을 만들기 위한 메모리할당(qsort를 위해)
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoPredicate* ) * sPredCnt,
                                               (void **)& sPredicateArray )
              != IDE_SUCCESS );

    // 할당받은 메모리에 인자로 받은 qmoPredicate 주소저장
    for ( sPredicate  = aPredicate, sIndex = 0;
          sPredicate != NULL;
          sPredicate  = sPredicate->next )
    {
        for ( sMorePredicate  = sPredicate;
              sMorePredicate != NULL;
              sMorePredicate  = sMorePredicate->more )
        {
            sMorePredicate->idx     = sIndex;
            sPredicateArray[sIndex] = sMorePredicate;
            sIndex++;
        }
    }

    //--------------------------------------
    // filter로 처리할 predicate의 갯수가 2이상이면,
    // selectivity가 작은 순서로 정렬한다.
    //--------------------------------------

    if ( sPredCnt > 1 )
    {
        idlOS::qsort( sPredicateArray,
                      sPredCnt,
                      ID_SIZEOF(qmoPredicate*),
                      compareFilter );
    }
    else
    {
        // Nothing To Do
    }

    //--------------------------------------
    // 새로운 AND 노드를 하나 생성한다.
    //--------------------------------------

    SET_EMPTY_POSITION( sNullPosition );

    IDE_TEST( qtc::makeNode( aStatement,
                             sANDNode,
                             & sNullPosition,
                             (const UChar*)"AND",
                             3 )
              != IDE_SUCCESS );

    //--------------------------------------
    // AND 노드 하위에 qsort로 정렬된 qtcNode를 연결한다.
    //--------------------------------------

    sANDNode[0]->node.arguments
        = (mtcNode *)(sPredicateArray[0]->node);
    sNode = (qtcNode *)(sANDNode[0]->node.arguments);

    sPredicateArray[0]->node->node.next = NULL; // 연결관계를 끊는다.

    for ( sIndex = 1;
          sIndex < sPredCnt;
          sIndex++ )
    {
        // 현재 predicate의 next 연결관계를 끊는다.
        sPredicateArray[sIndex]->node->node.next = NULL;
        // AND의 하위 노드로 연결.
        sNode->node.next = (mtcNode *)(sPredicateArray[sIndex]->node);
        sNode            = (qtcNode *)(sNode->node.next);
    }

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sANDNode[0] )
              != IDE_SUCCESS );

    *aNode = sANDNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPred::sortORNodeBySubQ( qcStatement   * aStatement,
                                  qtcNode       * aNode )
{
/***********************************************************************
 *
 * Description : Or node 의 인자들을 SubQ 와 NonSubQ 로 나눈다.
 *               NonSubQ 를 앞으로 모아서 먼저 수행하도록 한다.
 *
 * BUG-38971 subQuery filter 를 정렬할 필요가 있습니다.
 *
 ***********************************************************************/

    UInt        i;
    UInt        sNodeCnt;
    UInt        sSubQ;
    UInt        sNonSubQ;
    mtcNode   * sNode;
    mtcNode   * sOrNode;
    mtcNode  ** sNodePtrArray;

    IDU_FIT_POINT_FATAL( "qmoPred::sortORNodeBySubQ::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );

    sOrNode = (mtcNode*)aNode;

    // 인자로 넘어온 filter의 Node 갯수를 구한다.
    for ( sNode  = sOrNode->arguments, sNodeCnt = 0;
          sNode != NULL;
          sNode  = sNode->next )
    {
        sNodeCnt++;
    }

    // 배열을 만들기 위한 메모리할당
    IDE_TEST( QC_QME_MEM( aStatement )->cralloc( ID_SIZEOF( mtcNode* ) * sNodeCnt * 2,
                                                 (void **)& sNodePtrArray )
              != IDE_SUCCESS );

    // 할당받은 메모리에 인자로 받은 Node 주소저장
    for ( sNode  = sOrNode->arguments, sNonSubQ = 0, sSubQ = sNodeCnt;
          sNode != NULL;
          sNode  = sNode->next )
    {
        if ( ( ((qtcNode*)sNode)->lflag & QTC_NODE_SUBQUERY_MASK )
                == QTC_NODE_SUBQUERY_EXIST )
        {
            sNodePtrArray[sSubQ]    = sNode;
            sSubQ++;
        }
        else
        {
            sNodePtrArray[sNonSubQ] = sNode;
            sNonSubQ++;
        }
    }

    // Or 노드의 arguments 에 연결해준다.
    for ( i = 0;
          i < sNodeCnt * 2;
          i++ )
    {
        if ( sNodePtrArray[i] != NULL )
        {
            sNodePtrArray[i]->next = NULL;

            sOrNode->arguments = sNodePtrArray[i];
            sNode              = sNodePtrArray[i];
            break;
        }
        else
        {
            // Nothing To Do.
        }
    }

    // 나머지 노드들을 연결해준다.
    for ( i = i+1;
          i < sNodeCnt * 2;
          i++ )
    {
        if ( sNodePtrArray[i] != NULL )
        {
            sNodePtrArray[i]->next = NULL;

            sNode->next = sNodePtrArray[i];
            sNode       = sNodePtrArray[i];
        }
        else
        {
            // Nothing To Do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::removeIndexableFromFilter( qmoPredWrapper * aFilter )
{
    qmoPredicate   * sPredIter;
    qmoPredWrapper * sWrapperIter;

    IDU_FIT_POINT_FATAL( "qmoPred::removeIndexableFromFilter::__FT__" );

    // filter/subquery filter에 대해 subquery 최적화 팁 제거해야 함.
    for ( sWrapperIter  = aFilter;
          sWrapperIter != NULL;
          sWrapperIter  = sWrapperIter->next )
    {
        for ( sPredIter  = sWrapperIter->pred;
              sPredIter != NULL;
              sPredIter  = sPredIter->more )
        {
            sPredIter->id = QMO_COLUMNID_NON_INDEXABLE;

            if ( ( sPredIter->node->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                IDE_TEST( removeIndexableSubQTip( sPredIter->node )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::relinkPredicate( qmoPredWrapper * aWrapper )
{
    qmoPredWrapper * sWrapperIter;

    IDU_FIT_POINT_FATAL( "qmoPred::relinkPredicate::__FT__" );

    if ( aWrapper != NULL )
    {
        for ( sWrapperIter        = aWrapper;
              sWrapperIter->next != NULL;
              sWrapperIter        = sWrapperIter->next )
        {
            sWrapperIter->pred->next = sWrapperIter->next->pred;
        }
        sWrapperIter->pred->next = NULL;
    }
    else
    {
        // nothing to do...
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPred::removeMoreConnection( qmoPredWrapper * aWrapper, idBool aIfOnlyList )
{
    qmoPredWrapper * sWrapperIter;

    IDU_FIT_POINT_FATAL( "qmoPred::removeMoreConnection::__FT__" );

    for ( sWrapperIter  = aWrapper;
          sWrapperIter != NULL;
          sWrapperIter  = sWrapperIter->next )
    {
        if ( aIfOnlyList == ID_TRUE )
        {
            if ( sWrapperIter->pred->id == QMO_COLUMNID_LIST )
            {
                sWrapperIter->pred->more = NULL;
            }
            else
            {
                // Nothing to do...
            }
        }
        else
        {
            sWrapperIter->pred->more = NULL;
        }
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::fixPredToRangeAndFilter( qcStatement        * aStatement,
                                  qmsQuerySet        * aQuerySet,
                                  qmoPredWrapper    ** aKeyRange,
                                  qmoPredWrapper    ** aKeyFilter,
                                  qmoPredWrapper    ** aFilter,
                                  qmoPredWrapper    ** aLobFilter,
                                  qmoPredWrapper    ** aSubqueryFilter,
                                  qmoPredWrapperPool * aWrapperPool )
{
    IDU_FIT_POINT_FATAL( "qmoPred::fixPredToRangeAndFilter::__FT__" );

    IDE_DASSERT( aFilter != NULL );
    IDE_DASSERT( aLobFilter != NULL );
    IDE_DASSERT( aSubqueryFilter != NULL );

    // filter, lobFilter, subqueryFilter 별로
    // 연결관계를 재조정한다.
    IDE_TEST( relinkPredicate( *aFilter )
              != IDE_SUCCESS );
    IDE_TEST( relinkPredicate( *aLobFilter )
              != IDE_SUCCESS );
    IDE_TEST( relinkPredicate( *aSubqueryFilter )
              != IDE_SUCCESS );

    IDE_TEST( removeMoreConnection( *aFilter, ID_FALSE )
              != IDE_SUCCESS );
    IDE_TEST( removeMoreConnection( *aLobFilter, ID_FALSE )
              != IDE_SUCCESS );
    IDE_TEST( removeMoreConnection( *aSubqueryFilter, ID_FALSE )
              != IDE_SUCCESS );

    // filter, lobFilter, subqueryFilter에 대해
    // subquery 최적화 팁 제거해야 함.
    IDE_TEST( removeIndexableFromFilter( *aFilter )
              != IDE_SUCCESS );
    IDE_TEST( removeIndexableFromFilter( *aLobFilter )
              != IDE_SUCCESS );
    IDE_TEST( removeIndexableFromFilter( *aSubqueryFilter )
              != IDE_SUCCESS );

    if ( *aKeyRange != NULL )
    {
        // key range 별로 연결관계를 재조정한다.
        IDE_TEST( relinkPredicate( *aKeyRange )
                  != IDE_SUCCESS );
        IDE_TEST( removeMoreConnection( *aKeyRange, ID_TRUE )
                  != IDE_SUCCESS );

        IDE_TEST( process4Range( aStatement,
                                 aQuerySet,
                                 (*aKeyRange)->pred,
                                 aFilter,
                                 aWrapperPool )
                  != IDE_SUCCESS );

        if ( *aKeyFilter != NULL )
        {
            // key filter 별로 연결관계를 재조정한다.
            IDE_TEST( relinkPredicate( *aKeyFilter )
                      != IDE_SUCCESS );
            IDE_TEST( removeMoreConnection( *aKeyFilter, ID_TRUE )
                      != IDE_SUCCESS );

            IDE_TEST( process4Range( aStatement,
                                     aQuerySet,
                                     (*aKeyFilter)->pred,
                                     aFilter,
                                     aWrapperPool )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        // To fix BUG-15348
        // process4Range에서 like를 filter로 분류했을 경우
        // filter를 새로 연결해 주어야 함.
        IDE_TEST( relinkPredicate( *aFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::makePartKeyRangePredicate( qcStatement        * aStatement,
                                    qmsQuerySet        * aQuerySet,
                                    qmoPredicate       * aPredicate,
                                    qcmColumn          * aPartKeyColumns,
                                    qcmPartitionMethod   aPartitionMethod,
                                    qmoPredicate      ** aPartKeyRange )
{
    qmoPredWrapperPool  sWrapperPool;
    qmoPredWrapper    * sPartKeyRange;
    qmoPredWrapper    * sTempWrapper;
    qmoPredWrapper    * sRemain;
    qmoPredWrapper    * sLobFilter;
    qmoPredWrapper    * sSubqueryFilter;

    IDU_FIT_POINT_FATAL( "qmoPred::makePartKeyRangePredicate::__FT__" );

    sPartKeyRange   = NULL;
    sRemain         = NULL;
    sLobFilter      = NULL;
    sSubqueryFilter = NULL;
    sTempWrapper    = NULL;

    IDE_TEST( extractPartKeyRangePredicate( aStatement,
                                            aPredicate,
                                            aPartKeyColumns,
                                            aPartitionMethod,
                                            &sPartKeyRange,
                                            & sRemain,
                                            & sLobFilter,
                                            & sSubqueryFilter,
                                            & sWrapperPool )
              != IDE_SUCCESS );

    IDE_TEST( fixPredToRangeAndFilter( aStatement,
                                       aQuerySet,
                                       & sPartKeyRange,
                                       & sTempWrapper,
                                       & sTempWrapper,
                                       & sTempWrapper,
                                       & sTempWrapper,
                                       & sWrapperPool )
              != IDE_SUCCESS );

    if ( sPartKeyRange == NULL )
    {
        *aPartKeyRange = NULL;
    }
    else
    {
        *aPartKeyRange = sPartKeyRange->pred;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::makePartFilterPredicate( qcStatement        * aStatement,
                                  qmsQuerySet        * aQuerySet,
                                  qmoPredicate       * aPredicate,
                                  qcmColumn          * aPartKeyColumns,
                                  qcmPartitionMethod   aPartitionMethod,
                                  qmoPredicate      ** aPartFilter,
                                  qmoPredicate      ** aRemain,
                                  qmoPredicate      ** aSubqueryFilter )
{
    qmoPredWrapperPool  sWrapperPool;
    qmoPredWrapper    * sPartFilter;
    qmoPredWrapper    * sRemain;
    qmoPredWrapper    * sLobFilter;
    qmoPredWrapper    * sSubqueryFilter;
    qmoPredWrapper    * sTempWrapper;

    IDU_FIT_POINT_FATAL( "qmoPred::makePartFilterPredicate::__FT__" );

    sPartFilter     = NULL;
    sRemain         = NULL;
    sLobFilter      = NULL;
    sSubqueryFilter = NULL;
    sTempWrapper    = NULL;

    IDE_TEST( extractPartKeyRangePredicate( aStatement,
                                            aPredicate,
                                            aPartKeyColumns,
                                            aPartitionMethod,
                                            & sPartFilter,
                                            & sRemain,
                                            & sLobFilter,
                                            & sSubqueryFilter,
                                            & sWrapperPool )
              != IDE_SUCCESS );

    if ( sPartFilter == NULL )
    {
        *aPartFilter = NULL;
    }
    else
    {
        *aPartFilter = sPartFilter->pred;
    }

    // partFilter는 predicate연결관계를 이미 조정하였으므로,
    // remain, subqueryFilter만 연결관계 조정하면 된다.
    IDE_TEST( fixPredToRangeAndFilter( aStatement,
                                       aQuerySet,
                                       & sPartFilter,
                                       & sTempWrapper,
                                       & sRemain,
                                       & sLobFilter,
                                       & sSubqueryFilter,
                                       & sWrapperPool )
              != IDE_SUCCESS );

    if ( sRemain == NULL )
    {
        *aRemain = NULL;
    }
    else
    {
        *aRemain = sRemain->pred;
    }

    if ( sSubqueryFilter == NULL )
    {
        *aSubqueryFilter = NULL;
    }
    else
    {
        *aSubqueryFilter = sSubqueryFilter->pred;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::extractPartKeyRangePredicate( qcStatement         * aStatement,
                                       qmoPredicate        * aPredicate,
                                       qcmColumn           * aPartKeyColumns,
                                       qcmPartitionMethod    aPartitionMethod,
                                       qmoPredWrapper     ** aPartKeyRange,
                                       qmoPredWrapper     ** aRemain,
                                       qmoPredWrapper     ** aLobFilter,
                                       qmoPredWrapper     ** aSubqueryFilter,
                                       qmoPredWrapperPool  * aWrapperPool )
{
    qmoPredWrapper * sSource;

    IDU_FIT_POINT_FATAL( "qmoPred::extractPartKeyRangePredicate::__FT__" );

    *aPartKeyRange = NULL;

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aPartKeyColumns != NULL );

    //--------------------------------------
    // 결과값 초기화
    //--------------------------------------
    IDE_TEST( qmoPredWrapperI::initializeWrapperPool( QC_QMP_MEM( aStatement ),
                                                      aWrapperPool )
              != IDE_SUCCESS );

    *aPartKeyRange = NULL;

    IDE_TEST( qmoPredWrapperI::createWrapperList( aPredicate,
                                                  aWrapperPool,
                                                  & sSource )
              != IDE_SUCCESS );

    IDE_TEST( extractPartKeyRange4LIST( aStatement,
                                        aPartKeyColumns,
                                        & sSource,
                                        aPartKeyRange,
                                        aWrapperPool )
              != IDE_SUCCESS );

    IDE_TEST( extractPartKeyRange4Column( aPartKeyColumns,
                                          aPartitionMethod,
                                          & sSource,
                                          aPartKeyRange )
              != IDE_SUCCESS );

    IDE_TEST( separateFilters( QC_SHARED_TMPLATE( aStatement ),
                               sSource,
                               aRemain,
                               aLobFilter,
                               aSubqueryFilter,
                               aWrapperPool )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::extractPartKeyRange4LIST( qcStatement        * aStatement,
                                   qcmColumn          * aPartKeyColumns,
                                   qmoPredWrapper    ** aSource,
                                   qmoPredWrapper    ** aPartKeyRange,
                                   qmoPredWrapperPool * aWrapperPool )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *               LIST에 대한 partition keyRange를 추출한다.
 *
 * Implementation :
 *
 *   이 함수는 리스트 컬럼리스트만 인자로 받아서,
 *   하나의 리스트에 대해 partition keyRange를
 *   판단해서 해당하는 곳에 연결한다.
 *
 *      리스트의 파티션 프루닝/필터링 가능성 검사.
 *      (1) 리스트컬럼이 파티션 키 컬럼에 연속적으로 포함되면,
 *          partition keyRange로 분류
 *      (2) 그렇지 않다면 무조건 filter라고 세팅함.
 ***********************************************************************/

    qmoPredWrapper * sWrapperIter;
    qmoPredicate   * sPredIter;
    qmoPredType      sPredType;

    IDU_FIT_POINT_FATAL( "qmoPred::extractPartKeyRange4LIST::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPartKeyColumns != NULL );
    IDE_DASSERT( aSource != NULL );
    IDE_DASSERT( aPartKeyRange != NULL );

    //--------------------------------------
    // 리스트 컬럼리스트에 달려있는 각각의 컬럼에 대해
    // partition keyRange/filter에 대한 판단을 수행.
    //--------------------------------------

    for ( sWrapperIter  = *aSource;
          sWrapperIter != NULL;
          sWrapperIter  = sWrapperIter->next )
    {
        if ( sWrapperIter->pred->id == QMO_COLUMNID_LIST )
        {
            IDE_TEST( qmoPredWrapperI::extractWrapper( sWrapperIter,
                                                       aSource )
                      != IDE_SUCCESS );

            break;
        }
        else
        {
            // Nothing to do...
        }
    }

    //--------------------------------------
    // partition keyrange 추출
    //--------------------------------------

    if ( sWrapperIter != NULL )
    {
        for ( sPredIter  = sWrapperIter->pred;
              sPredIter != NULL;
              sPredIter  = sPredIter->more )
        {
            IDE_TEST( checkUsablePartitionKey4List( aStatement,
                                                    aPartKeyColumns,
                                                    sPredIter,
                                                    & sPredType )
                      != IDE_SUCCESS );

            if ( sPredType == QMO_KEYRANGE )
            {
                IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                    aPartKeyRange,
                                                    aWrapperPool )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        } // for
    }
    else // list predicate이 없는 경우 아무런 작업을 하지 않는다.
    {
        // Nothing to do...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::extractPartKeyRange4Column( qcmColumn        * aPartKeyColumns,
                                     qcmPartitionMethod aPartitionMethod,
                                     qmoPredWrapper  ** aSource,
                                     qmoPredWrapper  ** aPartKeyRange )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *               one column에 대한 partition keyRange를 추출한다.
 *
 *
 * Implementation :
 *
 *   partition key 컬럼순서대로 아래의 작업 수행
 *
 *   1. partition key 컬럼과 같은 컬럼의 컬럼리스트를 찾는다.
 *      (1) 아직 partition keyRange로 분류된 predicate이 없는 경우,
 *          keyRange에 찾은 predicate 연결
 *      (2) 이미 partition keyRange로 분류된 predicate이 존재하는 경우,
 *             partition keyRange 에 연결하고, 다음 partition key 컬럼으로 진행.
 *   2. 찾은 컬럼이
 *      (1) 다음 partition key 컬럼 사용 가능한 컬럼이면 ( equal(=), in )
 *      (2) 다음 partition key 컬럼 사용 불가능한 컬럼이면, (equal외의 predicate포함)
 *
 *
 ***********************************************************************/

    UInt             sPartKeyColumnID;
    qmoPredWrapper * sWrapperIter;
    qcmColumn      * sKeyColumn;
    idBool           sIsExtractable;

    IDU_FIT_POINT_FATAL( "qmoPred::extractPartKeyRange4Column::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPartKeyColumns != NULL );
    IDE_DASSERT( aSource != NULL );
    IDE_DASSERT( aPartKeyRange != NULL );

    //--------------------------------------
    // Partition KeyRange 추출
    //--------------------------------------

    // hash partition method인 경우 모든 partition key column을 포함해야 한다.
    // 또한 모두 equality 연산이어야 한다.
    if ( ( aPartitionMethod == QCM_PARTITION_METHOD_HASH ) ||
         ( aPartitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) )
    {
        for ( sKeyColumn  = aPartKeyColumns;
              sKeyColumn != NULL;
              sKeyColumn  = sKeyColumn->next )
        {
            // partition key 컬럼의 columnID를 구한다.
            sPartKeyColumnID = sKeyColumn->basicInfo->column.id;

            // partition key 컬럼과 같은 columnID를 찾는다.
            for ( sWrapperIter  = *aSource;
                  sWrapperIter != NULL;
                  sWrapperIter  = sWrapperIter->next )
            {
                if ( sPartKeyColumnID == sWrapperIter->pred->id )
                {
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }

            if ( sWrapperIter == NULL )
            {
                break;
            }
            else
            {
                if ( ( sWrapperIter->pred->flag &
                       QMO_PRED_NEXT_KEY_USABLE_MASK )
                     == QMO_PRED_NEXT_KEY_USABLE )
                {
                    // 이 컬럼은 equal(=)과 in predicate만으로 구성되어 있다.
                    // 다음 partition key 컬럼으로 진행.
                    // Nothing To Do
                }
                else
                {
                    // 이 컬럼은 equal(=)/in 이외의 predicate을 포함하고 있으므로,
                    // 다음 partition key 컬럼을 사용할 수 없다.

                    // 다음 partition key로의 작업을 진행하지 않는다.
                    break;

                }
            }
        }

        // 모두 대응되는 partition key 컬럼이 나와야 한다.
        if ( sKeyColumn == NULL )
        {
            sIsExtractable = ID_TRUE;
        }
        else
        {
            sIsExtractable = ID_FALSE;
        }
    }
    else
    {
        sIsExtractable = ID_TRUE;
    }


    //--------------------------------------
    // partition key 컬럼순서대로 partition key 컬럼과 동일한 컬럼을 찾아서
    // partition keyRange포함 여부를 결정한다.
    // 1. partition key 컬럼은 연속적으로 사용가능하여야 한다.
    //    예: partition key on T1(i1, i2, i3)
    //      (1) i1=1 and i2=1 and i3=1 ==> i1,i2,i3 모두 사용 가능
    //      (2) i1=1 and i3=1          ==> i1만 사용 가능
    // 2. subquery는 제외.
    //--------------------------------------

    if ( sIsExtractable == ID_TRUE )
    {
        for ( sKeyColumn  = aPartKeyColumns;
              sKeyColumn != NULL;
              sKeyColumn  = sKeyColumn->next )
        {
            // partition key 컬럼의 columnID를 구한다.
            sPartKeyColumnID = sKeyColumn->basicInfo->column.id;


            // partition key 컬럼과 같은 columnID를 찾는다.
            for ( sWrapperIter  = *aSource;
                  sWrapperIter != NULL;
                  sWrapperIter  = sWrapperIter->next )
            {
                if ( sPartKeyColumnID == sWrapperIter->pred->id )
                {
                    /* BUG-42172  If _PROWID pseudo column is compared with a column having
                     * PRIMARY KEY constraint, server stops abnormally.
                     */
                    if ( ( sWrapperIter->pred->node->lflag &
                           QTC_NODE_COLUMN_RID_MASK )
                         != QTC_NODE_COLUMN_RID_EXIST )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    // Nothing To Do
                }
            }

            if ( sWrapperIter == NULL )
            {
                // 현재 partition key 컬럼과 동일한 컬럼의 predicate이 존재하지 않는 경우
                break;
            }
            else
            {
                // 현재 partition key 컬럼과 동일한 컬럼의 predicate이 존재하는 경우

                IDE_TEST( qmoPredWrapperI::moveWrapper( sWrapperIter,
                                                        aSource,
                                                        aPartKeyRange )
                          != IDE_SUCCESS );

                if ( ( sWrapperIter->pred->flag &
                       QMO_PRED_NEXT_KEY_USABLE_MASK )
                     == QMO_PRED_NEXT_KEY_USABLE )
                {
                    // 이 컬럼은 equal(=)과 in predicate만으로 구성되어 있다.
                    // 다음 partition key 컬럼으로 진행.
                    // Nothing To Do
                }
                else
                {
                    // 이 컬럼은 equal(=)/in 이외의 predicate을 포함하고 있으므로,
                    // 다음 partition key 컬럼을 사용할 수 없다.

                    // 다음 partition key로의 작업을 진행하지 않는다.
                    break;

                }
            } // partition key 컬럼과 같은 컬럼을 가진 predicate이 있을 경우,
        } //  for()문
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
qmoPred::checkUsablePartitionKey4List( qcStatement   * aStatement,
                                       qcmColumn     * aPartKeyColumns,
                                       qmoPredicate  * aPredicate,
                                       qmoPredType   * aPredType )
{
/***********************************************************************
 *
 * Description : LIST 컬럼리스트의 partition key 사용여부 검사
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    UInt           sCount;
    UInt           sListCount;
    UInt           sPartKeyColumnID;
    UInt           sColumnID;
    idBool         sIsKeyRange;
    idBool         sIsExist;
    idBool         sIsNotExistIndexCol = ID_FALSE;
    qtcNode      * sCompareNode;
    qtcNode      * sColumnLIST;
    qtcNode      * sColumnNode;
    qcmColumn    * sColumn;

    IDU_FIT_POINT_FATAL( "qmoPred::checkUsablePartitionKey4List::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPartKeyColumns != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aPredType != NULL );

    //---------------------------------------
    // LIST의 인덱스 사용 가능성 검사
    // 인덱스 사용가능 컬럼에 LIST컬럼이 모두 포함되기만 하면 된다.
    // 1. 리스트컬럼이 인덱스 컬럼에 연속적으로 모두 포함되면,
    //     keyRange로 분류
    // 2. 연속적이지는 않지만, 리스트 컬럼이 모두 인덱스 컬럼에
    //    포함되면, keyFilter로 분류
    // 3. 1과2에 포함되지 않으면, filter로 분류
    //---------------------------------------

    if ( ( aPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sCompareNode = (qtcNode *)(aPredicate->node->node.arguments);
    }
    else
    {
        sCompareNode = aPredicate->node;
    }

    if( sCompareNode->indexArgument == 0 )
    {
        sColumnLIST = (qtcNode *)(sCompareNode->node.arguments);
    }
    else
    {
        sColumnLIST = (qtcNode *)(sCompareNode->node.arguments->next);
    }

    // LIST column의 개수 획득
    sListCount = sColumnLIST->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    // LIST의 인덱스 사용 가능성 검사.
    sCount      = 0;
    sIsKeyRange = ID_TRUE;
    for ( sColumn  = aPartKeyColumns;
          sColumn != NULL;
          sColumn  = sColumn->next )
    {
        sPartKeyColumnID = sColumn->basicInfo->column.id;

        for ( sColumnNode = (qtcNode *)(sColumnLIST->node.arguments);
              sColumnNode != NULL;
              sColumnNode  = (qtcNode *)(sColumnNode->node.next) )
        {
            sColumnID = QC_SHARED_TMPLATE(aStatement)->
                tmplate.rows[sColumnNode->node.table].
                columns[sColumnNode->node.column].column.id;

            if ( sPartKeyColumnID == sColumnID )
            {
                sCount++;
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( sColumnNode != NULL )
        {
            // Nothing To Do
            if ( sIsNotExistIndexCol == ID_TRUE )
            {
                sIsKeyRange = ID_FALSE;
                sIsNotExistIndexCol = ID_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            if ( sIsNotExistIndexCol == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                sIsNotExistIndexCol = ID_TRUE;
            }
        }
    }

    if ( sCount == sListCount )
    {
        // LIST의 모든 column이 index 내에 포함됨
        sIsExist = ID_TRUE;
    }
    else
    {
        // LIST의 column 중  index에 포함되지 않는 것이 존재함.
        // filter 처리 대항
        sIsExist = ID_FALSE;
    }

    if ( ( sIsExist == ID_TRUE ) &&
         ( sIsKeyRange == ID_TRUE ) )
    {
        *aPredType = QMO_KEYRANGE;
    }
    else
    {
        *aPredType = QMO_FILTER;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPred::makeRangeAndFilterPredicate( qcStatement   * aStatement,
                                      qmsQuerySet   * aQuerySet,
                                      idBool          aIsMemory,
                                      qmoPredicate  * aPredicate,
                                      qcmIndex      * aIndex,
                                      qmoPredicate ** aKeyRange,
                                      qmoPredicate ** aKeyFilter,
                                      qmoPredicate ** aFilter,
                                      qmoPredicate ** aLobFilter,
                                      qmoPredicate ** aSubqueryFilter )
{
    qmoPredWrapperPool  sWrapperPool;
    qmoPredWrapper    * sKeyRange;
    qmoPredWrapper    * sKeyFilter;
    qmoPredWrapper    * sFilter;
    qmoPredWrapper    * sLobFilter;
    qmoPredWrapper    * sSubqueryFilter;

    IDU_FIT_POINT_FATAL( "qmoPred::makeRangeAndFilterPredicate::__FT__" );

    IDE_TEST( extractRangeAndFilter( aStatement,
                                     QC_SHARED_TMPLATE( aStatement ),
                                     aIsMemory,
                                     ID_FALSE,
                                     aIndex,
                                     aPredicate,
                                     & sKeyRange,
                                     & sKeyFilter,
                                     & sFilter,
                                     & sLobFilter,
                                     & sSubqueryFilter,
                                     & sWrapperPool )
                 != IDE_SUCCESS );

    IDE_TEST( fixPredToRangeAndFilter( aStatement,
                                       aQuerySet,
                                       & sKeyRange,
                                       & sKeyFilter,
                                       & sFilter,
                                       & sLobFilter,
                                       & sSubqueryFilter,
                                       & sWrapperPool )
              != IDE_SUCCESS );

    // keyRange
    if ( sKeyRange == NULL )
    {
        *aKeyRange = NULL;
    }
    else
    {
        *aKeyRange = sKeyRange->pred;
    }

    // keyFilter
    if ( sKeyFilter == NULL )
    {
        *aKeyFilter = NULL;
    }
    else
    {
        *aKeyFilter = sKeyFilter->pred;
    }

    // filter
    if ( sFilter == NULL )
    {
        *aFilter = NULL;
    }
    else
    {
        *aFilter = sFilter->pred;
    }

    // lobFilter
    if ( sLobFilter == NULL )
    {
        *aLobFilter = NULL;
    }
    else
    {
        *aLobFilter = sLobFilter->pred;
    }

    // subqueryFilter
    if ( sSubqueryFilter == NULL )
    {
        *aSubqueryFilter = NULL;
    }
    else
    {
        *aSubqueryFilter = sSubqueryFilter->pred;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::extractRangeAndFilter( qcStatement        * aStatement,
                                qcTemplate         * aTemplate,
                                idBool               aIsMemory,
                                idBool               aInExecutionTime,
                                qcmIndex           * aIndex,
                                qmoPredicate       * aPredicate,
                                qmoPredWrapper    ** aKeyRange,
                                qmoPredWrapper    ** aKeyFilter,
                                qmoPredWrapper    ** aFilter,
                                qmoPredWrapper    ** aLobFilter,
                                qmoPredWrapper    ** aSubqueryFilter,
                                qmoPredWrapperPool * aWrapperPool )
{
/***********************************************************************
 *
 * Description : keyRange, keyFilter, filter, subqueryFilter를 추출한다.
 *
 *   사용되는 인덱스 정보를 보고,
 *   1. disk table 이면,
 *      keyRange, keyFilter, filter, subqueryFilter로 분리
 *   2. memory table 이면,
 *      keyRange, filter, subqueryFilter로 분리한다.
 *      (즉, keyFilter를 분리해내지 않는다.)
 *
 * Implementation :
 *
 *   1. KeyRange 추출
 *      1) LIST 컬럼에 대한 처리
 *          LIST 컬럼리스트에 연결된 predicate들에 대해서,
 *          keyRange, keyFilter, filter, subqueryFilter로 분리한다.
 *      2) one column에 대한 처리.
 *         (1) index nested loop join predicate이 존재하는 경우,
 *             join index 최적화에 의하여 join predicate을 우선 적용.
 *             참조 BUG-7098
 *             ( 만약, LIST에서 추출된 keyRange가 있다면,
 *               .IN subquery인 경우는 subqueryFilter로
 *               .IN subquery가 아닌 경우는 keyFilter로 분류되도록 한다. )
 *         (2) index nested loop join predicate이 존재하지 않는 경우,
 *           A. LIST 컬럼에 대한 keyRange가 존재하면,
 *              one column에 대한 keyRange는 추출하지 않고
 *           B. LIST 컬럼에 대한 keyRange가 존재하지 않으면,
 *              one column에 대한 keyRange 추출
 *   2. keyFilter 추출
 *      (1) 추출된 keyRange가 존재하면,
 *      (2) disk table에 대해서 keyFilter 추출
 *   3. filter, subquery filter  처리
 *
 ***********************************************************************/

    qmoPredWrapper * sSource;

    IDU_FIT_POINT_FATAL( "qmoPred::extractRangeAndFilter::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aKeyRange != NULL );
    IDE_DASSERT( aKeyFilter != NULL );
    IDE_DASSERT( aFilter != NULL );
    IDE_DASSERT( aLobFilter != NULL );
    IDE_DASSERT( aSubqueryFilter != NULL );

    //--------------------------------------
    // 결과값 초기화
    //--------------------------------------

    if ( aInExecutionTime == ID_TRUE )
    {
        IDE_TEST( qmoPredWrapperI::initializeWrapperPool( QC_QMX_MEM( aStatement ),
                                                          aWrapperPool )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qmoPredWrapperI::initializeWrapperPool( QC_QMP_MEM( aStatement ),
                                                          aWrapperPool )
                  != IDE_SUCCESS );
    }

    *aKeyRange       = NULL;
    *aKeyFilter      = NULL;
    *aFilter         = NULL;
    *aLobFilter      = NULL;
    *aSubqueryFilter = NULL;

    IDE_TEST( qmoPredWrapperI::createWrapperList( aPredicate,
                                                  aWrapperPool,
                                                  & sSource )
              != IDE_SUCCESS );

    //--------------------------------------
    // keyRange, keyFilter, filter, subuqeyrFilter 추출
    //--------------------------------------

    if ( aIndex == NULL )
    {
        // filter, subqueryFilter를 분리해서 넘긴다.
        IDE_TEST( separateFilters( aTemplate,
                                   sSource,
                                   aFilter,
                                   aLobFilter,
                                   aSubqueryFilter,
                                   aWrapperPool )
                  != IDE_SUCCESS );
    }
    else
    {
        //--------------------------------------------
        // LIST 컬럼에 대한 처리
        // : 현재 predicate 연결리스트에서 LIST 컬럼만 분리해서,
        //   LIST 컬럼에 대한 keyRange/keyFilter/filter/subqueryFilter를 분리
        //--------------------------------------------

        IDE_TEST( extractRange4LIST( aTemplate,
                                     aIndex,
                                     & sSource,
                                     aKeyRange,
                                     aKeyFilter,
                                     aFilter,
                                     aSubqueryFilter,
                                     aWrapperPool )
                  != IDE_SUCCESS );

        //--------------------------------------
        // keyRange 추출
        // 1. index nested loop join predicate이 있는 경우,
        //    index nested loop join predicate이 keyRange로 선택되도록 한다.
        // 2. index nested loop join predicate이 없는 경우,
        //    (1) LIST에서 선택된 keyRange가 있는 경우,
        //        one column에 대한 keyRange 추출 skip
        //    (2) LIST에서 선택된 keyRange가 없는 경우,
        //        one column에 대한 keyRange 추출
        //--------------------------------------

        IDE_TEST( extractKeyRange( aIndex,
                                   & sSource,
                                   aKeyRange,
                                   aKeyFilter,
                                   aSubqueryFilter )
                  != IDE_SUCCESS );

        //--------------------------------------
        // keyFilter 추출
        // : LIST 컬럼이 존재하는 경우, 이미 LIST 컬럼처리시 keyFilter 분류됨.
        //   따라서, one column에 대한 keyFilter만 추출하면 된다.
        //
        // 1. keyRange 존재유무
        //    (1) keyRange가 존재 : keyFilter 추출
        //    (2) keyRange가 존재하지 않으면, keyFilter의 용도가 무의미하므로,
        //        keyFilter를 추출하지 않는다.
        // 2. table의 종류
        //    (1) disk table   : keyFilter 추출(disk I/O를 줄이기 위함)
        //    (2) memory table : keyFilter 추출하지 않음.(인덱스사용의 극대화)
        //--------------------------------------

        IDE_TEST( extractKeyFilter( aIsMemory,
                                    aIndex,
                                    & sSource,
                                    aKeyRange,
                                    aKeyFilter,
                                    aFilter )
                  != IDE_SUCCESS );

        //--------------------------------------
        // Filter 추출
        //--------------------------------------

        // keyRange, keyFilter 추출후,
        // 남아있는 predicate들로 모두 filter로 분류한다.
        // 현재 남아있는 predicate은 다음의 두 종류이다.
        // (1) one column의 indexable predicate
        // (2) non-indexable predicate

        IDE_TEST( separateFilters( aTemplate,
                                   sSource,
                                   aFilter,
                                   aLobFilter,
                                   aSubqueryFilter,
                                   aWrapperPool )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::getColumnID( qcStatement   * aStatement,
                      qtcNode       * aNode,
                      idBool          aIsIndexable,
                      UInt          * aColumnID )
{
/***********************************************************************
 *
 * Description : columnID 설정
 *
 *     selection graph의 모든 predicate에 대한 columnID를 설정한다.
 *
 *     이 함수를 호출하는 곳은
 *     (1) qmoPred::classifyTablePredicate()
 *     (2) qmoPred::makeJoinPushDownPredicate()
 *     (3) qmoPred::addNonJoinablePredicate() 이며,
 *     columnID를 설정하는 함수를 공통으로 사용하기 위해서,
 *     predicate의 indexable여부를 인자로 받는다.
 *
 * Implementation :
 *
 *     1. indexable predicate 인 경우,
 *        (1) one column : 해당 columnID설정
 *        (2) LIST       : QMO_COLUMNID_LIST로 설정
 *
 *        OR 노드 하위에 여러개의 비교연산자가 있더라도,
 *        모두 동일 컬럼으로 구성되어 있다.
 *
 *     2. non-indexable predicate 인 경우,
 *        QMO_COLUMNID_NON_INDEXABLE로 설정
 *     (인자로 받을 수 있는 노드의 형태는 다음과 같다.)
 *
 *
 *     (1)  OR         (2)  OR                        (3) 비교연산자
 *          |               |                                 |
 *       비교연산자     비교연산자->...->비교연산자
 *          |               |                |
 *
 ***********************************************************************/

    qtcNode * sNode = aNode;
    qtcNode * sCurNode;

    IDU_FIT_POINT_FATAL( "qmoPred::getColumnID::__FT__" );

    // 초기값 설정
    *aColumnID = QMO_COLUMNID_NOT_FOUND;

    if ( aIsIndexable == ID_TRUE )
    {
        //--------------------------------------
        // indexable predicate에 대한
        // one column과 LIST의 columnID 설정
        //--------------------------------------

        //--------------------------------------
        // 비교연산자 노드를 찾는다.
        // 인자로 넘어온 최상위 노드가 논리연산자인 경우,
        // 비교연산자 노드를 찾기 위해, 논리연산자(OR) skip
        //--------------------------------------
        if ( ( sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
             == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sNode = (qtcNode *)(sNode->node.arguments);
        }

        // BUG-39036 select one or all value optimization
        // keyRange or 상수값 의 형태일때 keyRange 생성이 실패하는것을 성공하게 한다.
        // 상수값을 가지는 keyRange가 올 수 있으므로 노드를 순회해야 한다.
        while ( sNode != NULL )
        {
            if ( qtc::dependencyEqual( & sNode->depInfo,
                                       & qtc::zeroDependencies ) == ID_FALSE )
            {
                //--------------------------------------
                // indexArgument 정보로 columnID를 설정한다.
                //--------------------------------------

                // indexArgument 정보로 column을 찾는다.
                if ( sNode->indexArgument == 0 )
                {
                    sCurNode = (qtcNode *)( sNode->node.arguments );
                }
                else // (sNode->indexArgument == 1)
                {
                    sCurNode = (qtcNode *)( sNode->node.arguments->next );
                }

                // 찾은 column정보로 columnID 설정
                if ( ( sCurNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    // LIST의 columnID 설정
                    *aColumnID = QMO_COLUMNID_LIST;
                }
                else if ( QTC_IS_RID_COLUMN( sCurNode ) == ID_TRUE )
                {
                    /* BUG-41599 */
                    *aColumnID = QMO_COLUMNID_NON_INDEXABLE;
                }
                else
                {
                    // one column의 columnID 설정
                    *aColumnID =
                        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sCurNode->node.table].
                        columns[sCurNode->node.column].column.id;
                }

                break;
            }
            else
            {
                sNode = (qtcNode *)(sNode->node.next);
            }
        }
    }
    else
    {
        // non-indexable predicate에 대한 columnID 설정.
        *aColumnID = QMO_COLUMNID_NON_INDEXABLE;
    }

    // 위의 과정으로 columnID를 반드시 찾아야 한다.
    IDE_DASSERT( *aColumnID != QMO_COLUMNID_NOT_FOUND );

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::checkUsableIndex4List( qcTemplate    * aTemplate,
                                qcmIndex      * aIndex,
                                qmoPredicate  * aPredicate,
                                qmoPredType   * aPredType )
{
/***********************************************************************
 *
 * Description : LIST 컬럼리스트의 인덱스 사용여부 검사
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    UInt           sCount;
    UInt           sKeyColCount;
    UInt           sListCount;
    UInt           sIdxColumnID;
    UInt           sColumnID;
    idBool         sIsKeyRange;
    idBool         sIsExist;
    idBool         sIsNotExistIndexCol = ID_FALSE;
    qtcNode      * sCompareNode;
    qtcNode      * sColumnLIST;
    qtcNode      * sColumnNode;

    IDU_FIT_POINT_FATAL( "qmoPred::checkUsableIndex4List::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aPredType != NULL );

    //---------------------------------------
    // LIST의 인덱스 사용 가능성 검사
    // 인덱스 사용가능 컬럼에 LIST컬럼이 모두 포함되기만 하면 된다.
    // 1. 리스트컬럼이 인덱스 컬럼에 연속적으로 모두 포함되면,
    //     keyRange로 분류
    // 2. 연속적이지는 않지만, 리스트 컬럼이 모두 인덱스 컬럼에
    //    포함되면, keyFilter로 분류
    // 3. 1과2에 포함되지 않으면, filter로 분류
    //---------------------------------------

    if ( ( aPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sCompareNode = (qtcNode *)(aPredicate->node->node.arguments);
    }
    else
    {
        sCompareNode = aPredicate->node;
    }

    if ( sCompareNode->indexArgument == 0 )
    {
        sColumnLIST = (qtcNode *)(sCompareNode->node.arguments);
    }
    else
    {
        sColumnLIST = (qtcNode *)(sCompareNode->node.arguments->next);
    }

    // LIST column의 개수 획득
    sListCount = sColumnLIST->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    // LIST의 인덱스 사용 가능성 검사.
    sCount      = 0;
    sIsKeyRange = ID_TRUE;
    for ( sKeyColCount = 0;
          sKeyColCount < aIndex->keyColCount;
          sKeyColCount++ )
    {
        sIdxColumnID = aIndex->keyColumns[sKeyColCount].column.id;

        for ( sColumnNode = (qtcNode *)(sColumnLIST->node.arguments);
              sColumnNode != NULL;
              sColumnNode  = (qtcNode *)(sColumnNode->node.next) )
        {
            sColumnID = aTemplate->
                tmplate.rows[sColumnNode->node.table].
                columns[sColumnNode->node.column].column.id;

            if ( sIdxColumnID == sColumnID )
            {
                sCount++;
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( sColumnNode != NULL )
        {
            // Nothing To Do
            if ( sIsNotExistIndexCol == ID_TRUE )
            {
                sIsKeyRange = ID_FALSE;
                sIsNotExistIndexCol = ID_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            if ( sIsNotExistIndexCol == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                sIsNotExistIndexCol = ID_TRUE;
            }
        }
    }

    if ( sCount == sListCount )
    {
        // LIST의 모든 column이 index 내에 포함됨
        sIsExist = ID_TRUE;
    }
    else
    {
        // LIST의 column 중  index에 포함되지 않는 것이 존재함.
        // filter 처리 대항
        sIsExist = ID_FALSE;
    }

    if ( sIsExist == ID_TRUE )
    {
        if ( sIsKeyRange == ID_TRUE )
        {
            *aPredType = QMO_KEYRANGE;
        }
        else
        {
            *aPredType = QMO_KEYFILTER;
        }
    }
    else
    {
        *aPredType = QMO_FILTER;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::findChildGraph( qtcNode   * aCompareNode,
                         qcDepInfo * aFromDependencies,
                         qmgGraph  * aGraph1,
                         qmgGraph  * aGraph2,
                         qmgGraph ** aLeftColumnGraph,
                         qmgGraph ** aRightColumnGraph )
{
/***********************************************************************
 *
 * Description : 비교연산자 각 하위 노드에 해당하는 graph를 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcDepInfo sTempDependencies1;
    qcDepInfo sTempDependencies2;

    IDU_FIT_POINT_FATAL( "qmoPred::findChildGraph::__FT__" );

    //--------------------------------------
    // 비교연산자 각 하위 노드에 해당하는 graph를 찾는다.
    //--------------------------------------


    IDE_DASSERT( aCompareNode->node.arguments       != NULL );
    IDE_DASSERT( aCompareNode->node.arguments->next != NULL );

    //-------------------------------------------------------------------
    // aCompareNode의 dependencies는 outer column에 대한
    // dependencies를 제거한 후 검사하여야 한다.
    //-------------------------------------------------------------------

    qtc::dependencyAnd( & ((qtcNode *)(aCompareNode->node.arguments))->depInfo,
                        aFromDependencies,
                        & sTempDependencies1 );

    qtc::dependencyAnd( & ((qtcNode *)(aCompareNode->node.arguments->next))->depInfo,
                        aFromDependencies,
                        & sTempDependencies2 );


    //-------------------------------------------------------------------
    // aCompareNode->node.arguments의 dependencies는
    // aGraph1의 dependencies에 포함되거나
    // aGraph2의 dependencies에 포함되어야 한다.
    // 찾지 못할 경우는 어딘가의 버그임을 알리기 위해 ASSERT를 사용한다.
    //-------------------------------------------------------------------

    if ( ( qtc::dependencyContains( & aGraph1->depInfo,
                                    & sTempDependencies1 )
           == ID_TRUE ) &&
         ( qtc::dependencyContains( & aGraph2->depInfo,
                                    & sTempDependencies2 )
           == ID_TRUE ) )
    {
        *aLeftColumnGraph = aGraph1;
        *aRightColumnGraph = aGraph2;
    }
    else if ( ( qtc::dependencyContains( & aGraph2->depInfo,
                                         & sTempDependencies1 )
                == ID_TRUE ) &&
              ( qtc::dependencyContains( & aGraph1->depInfo,
                                         & sTempDependencies2 )
                == ID_TRUE ) )
    {
        // BUG-24981 joinable predicate의 비교연산자 각 하위 노드에 해당하는
        // graph(join하위그래프)를 찾는 과정에서 서버 비정상종료

        *aLeftColumnGraph = aGraph2;
        *aRightColumnGraph = aGraph1;
    }
    else // 둘다 아닌 경우에는 이 함수가 불리면 안된다.
    {
        IDE_RAISE( ERR_INVALID_GRAPH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoPred::findChildGraph",
                                  "Invalid graph" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::classifyTablePredicate( qcStatement      * aStatement,
                                 qmoPredicate     * aPredicate,
                                 qcDepInfo        * aTableDependencies,
                                 qcDepInfo        * aOuterDependencies,
                                 qmoStatistics    * aStatiscalData )
{
/***********************************************************************
 *
 * Description : Base Table의 Predicate에 대한 분류
 *
 *    predicate 재배치 과정에서 각 predicate에 대해
 *    indexable predicate을 판단하고, columnID와 selectivity를 설정한다.
 *    (대상: selection graph의 myPredicate)
 *
 * Implementation :
 *     1. indexable predicate인지를 판단.
 *     2. predicate의 fixed/variable 정보 저장.
 *        [variable predicate 판단기준]
 *        . join predicate
 *        . host 변수 존재
 *        . subquery 존재
 *        . prior column이 포함된 predicate( 예: prior i1=i1 )
 *     3. columnID 설정
 *        (1) indexable predicate 이면,
 *            . one column : smiColumn.id
 *            . LIST       : QMO_COLUMNID_LIST
 *        (2) non-indexable predicate 이면,
 *            columnID = QMO_COLUMNID_NON_INDEXABLE
 *     4. predicate에 대한 selectivity 설정.
 *
 ***********************************************************************/

    idBool sIsIndexable;

    IDU_FIT_POINT_FATAL( "qmoPred::classifyTablePredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate != NULL );

    //--------------------------------------
    // predicate에 대한 indexable 판단
    //--------------------------------------

    IDE_TEST( isIndexable( aStatement,
                           aPredicate,
                           aTableDependencies,
                           aOuterDependencies,
                           & sIsIndexable )
              != IDE_SUCCESS );

    //--------------------------------------
    // columnID 설정
    //--------------------------------------

    IDE_TEST( getColumnID( aStatement,
                           aPredicate->node,
                           sIsIndexable,
                           & aPredicate->id )
              != IDE_SUCCESS );

    //--------------------------------------
    // predicate에 대한 selectivity 설정.
    //--------------------------------------


    if ( aStatiscalData != NULL )
    {
        // fix BUG-12515
        // VIEW 안으로 push selection되고 난 후,
        // where절에 남아있는 predicate에 대해서는 selectivity를 구하지 않음.
        // predicate->mySelectivity = 1로 초기화 되어 있음.

        if ( ( aPredicate->flag & QMO_PRED_PUSH_REMAIN_MASK )
             == QMO_PRED_PUSH_REMAIN_FALSE )
        {
            // PROJ-2242
            IDE_TEST( qmoSelectivity::setMySelectivity( aStatement,
                                                        aStatiscalData,
                                                        aTableDependencies,
                                                        aPredicate )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing to do.
        // 통계정보가 없으므로 selectivity는 설정하지 않는다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::classifyPartTablePredicate( qcStatement        * aStatement,
                                     qmoPredicate       * aPredicate,
                                     qcmPartitionMethod   aPartitionMethod,
                                     qcDepInfo          * aTableDependencies,
                                     qcDepInfo          * aOuterDependencies )
{
/***********************************************************************
 *
 * Description : Base Table의 Predicate에 대한 분류
 *
 *    predicate 재배치 과정에서 각 predicate에 대해
 *    indexable predicate을 판단하고, columnID와 selectivity를 설정한다.
 *    (대상: selection graph의 myPredicate)
 *
 * Implementation :
 *     1. indexable predicate인지를 판단.\
 *     2. predicate의 fixed/variable 정보 저장.
 *        [variable predicate 판단기준]
 *        . join predicate
 *        . host 변수 존재
 *        . subquery 존재
 *        . prior column이 포함된 predicate( 예: prior i1=i1 )
 *     3. columnID 설정
 *        (1) indexable predicate 이면,
 *            . one column : smiColumn.id
 *            . LIST       : QMO_COLUMNID_LIST
 *        (2) non-indexable predicate 이면,
 *            columnID = QMO_COLUMNID_NON_INDEXABLE
 *     4. predicate에 대한 selectivity 설정.
 *
 ***********************************************************************/

    idBool sIsPartitionPrunable;

    IDU_FIT_POINT_FATAL( "qmoPred::classifyPartTablePredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate != NULL );

    //--------------------------------------
    // predicate에 대한 indexable 판단
    //--------------------------------------

    IDE_TEST( isPartitionPrunable( aStatement,
                                   aPredicate,
                                   aPartitionMethod,
                                   aTableDependencies,
                                   aOuterDependencies,
                                   & sIsPartitionPrunable )
              != IDE_SUCCESS );

    //--------------------------------------
    // columnID 설정
    //--------------------------------------

    IDE_TEST( getColumnID( aStatement,
                           aPredicate->node,
                           sIsPartitionPrunable,
                           & aPredicate->id )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::isPartitionPrunable( qcStatement        * aStatement,
                              qmoPredicate       * aPredicate,
                              qcmPartitionMethod   aPartitionMethod,
                              qcDepInfo          * aTableDependencies,
                              qcDepInfo          * aOuterDependencies,
                              idBool             * aIsPartitionPrunable )
{
/***********************************************************************
 *
 * Description : predicate의 indexable 여부 판단
 *
 *     <indexable predicate의 정의>
 *
 *     조건1. indexable operator 이어야 한다.
 *            system level operator 뿐만 아니라,
 *            user level operator(quantify비교연산자)도 포함된다.
 *
 *     조건2. column이 있어야 한다.
 *            예) i1=1(O), i1=i2(O), i1=i1+1(O), i1+1=1(O), 1=1(X),
 *                (i1,i2,i3)=(i1,i2,i3)(O), (i1,i2,i3)=(1,1,1)(O),
 *                (i1,i2,1 )=( 1, 1, 1)(X), (1,subquery)=(1,1)(X)
 *
 *     조건3. column에 연산이 없어야 한다.
 *
 *            예) i1=1(O), i1=i2(O), i1=i1+1(O), i1+1=1(X)
 *
 *     조건4. column이 한쪽에만 존재해야 한다.
 *            예) i1=1(O), i1=i1+1(X), i1=i2(X)
 *
 *     조건5. column에 conversion이 발생하지 않아야 한다.
 *            예) i1(integer)=smallint'1'(O), i1(integer)=3.5(X)
 *            sameGroupCompare로 인해 이 제약은 완화됨.
 *
 *     조건6. value에 대한 조건체크
 *            6-1. subquery는 무조건 안된다.
 *                 예) a1 = (select i1 from t2 where i1=1)(X)
 *                     a1 = (select sum(i1) from t2)(X)
 *                     a1 = (select i1 from t2 where i1=al)(X)
 *                     a1 = (select sum(i1) from t2 where i1=a1)(X)
 *            6-2. LIKE에 대한 패턴문자는 일반문자로 시작하여야 한다.
 *                 예) i1 like 'a%'(O) , i1 like '\_a%' escape'\'(O)
 *                     i1 like '%a%'(X), i1 like '_bc'(X)
 *            6-3. host 변수가 오는 경우는 무조건 안된다.
 *                 예) i1=?(X), i1=?+1(X)
 *
 *     조건7. OR노드 하위에 동일한 컬럼만 있는 경우이어야 한다.
 *            단, subquery 가 오는 경우는 제외된다.
 *            예) i1=1 OR i1=2(O), (i1,i2)=(1,1) OR (i1,i2)=(2,2)(O),
 *                i1=1 OR i2=2(X),
 *                i1 in (subquery) OR i1 in (subquery)(X),
 *                i1=1 OR i1 in (subquery) (X)
 *                (i1=1 and i2=1) or i1=( subquery ) (X)
 *
 *
 *     < 질의 수행과정에서의 partition prunable predicate 의 판단범위 >
 *
 *     (1) parsing & validation 과정에서의 판단범위
 *         조건1, 조건2, 조건3, 이 세가지 조건을 만족하면
 *         mtcNode.lflag에 MTC_NODE_INDEX_USABLE을 설정함.
 *
 *     (2) graph 생성과정에서의 판단범위
 *         조건4, 조건5, 조건6, 조건7
 *
 *     (3) plan tree 생성과정에서의 판단범위
 *         partition filter를 추출함.
 *
 *     (4) execution 과정에서의 판단범위
 *         binding이 끝난 후, 또는 doItFirst에서 partition filter를
 *         새로 생성한다.
 *
 * Implementation : 이 함수는 graph 생성과정에서의 판단범위를 검사함.
 *
 *     1. 조건1,2,3의 판단
 *        mtcNode.lflag가 MTC_NODE_INDEX_USABLE 인지를 검사
 *
 *     2. 조건4의 판단
 *        (1) operand의 dependency가 중복되지 않는지를 검사한다.
 *            (이 검사로 컬럼이 한쪽에만 존재한다는 것이 검증됨)
 *
 *            dependency 중복 판단방법은,
 *            ( ( 비교연산자하위의 두개 노드의 dependencies를 AND연산)
 *              & FROM의 해당 table의 dependency ) != 0
 *
 *        (2) (1)이 검증되면,
 *            컬럼이 모두 해당 table의 컬럼으로 구성되었는지를 검사.
 *
 *     3. 조건5의 판단
 *        (1) column
 *            value쪽에 host변수가 없는데도,
 *            column에 conversion이 발생했는지를 검사.
 *
 *        (2) LIST
 *            value쪽 LIST 하위 노드들을 모두 조사해서, host변수가 아니면서,
 *            value의 leftConversion이 존재하는지 검사.
 *
 *     4. 조건6의 판단
 *        (1) 6-1조건 : QTC_NODE_SUBQUERY_EXIST
 *        (2) 6-2조건 : ?
 *        (3) 6-3조건 :
 *            value node의 tuple이 MTC_TUPLE_TYPE_CONSTANT인지를 검사.
 *
 *     5. 조건7의 판단
 *        (1) OR노드 하위에 1개의 비교연산자가 존재할 경우,
 *            subquery가 오면 안됨.
 *            partition prunable predicate인지 판단
 *        (2) OR노드 하위에 2개이상의 비교연산자가 존재할 경우,
 *            . subquery를 포함하지 않아야 한다.
 *            . 하위 비교연산자들이 모두 partition prunable이어야 한다.
 *            . 하위 노드들의 columID가 모두 같아야 한다.
 *              (단, 컬럼이 LIST인 경우는 non으로 처리)
 *
 ***********************************************************************/
    UInt     sColumnID;
    UInt     sFirstColumnID;
    idBool   sIsFirstNode             = ID_TRUE;
    idBool   sIsPartitionPrunablePred = ID_FALSE;
    qtcNode *sNode;

    IDU_FIT_POINT_FATAL( "qmoPred::isPartitionPrunable::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aIsPartitionPrunable != NULL );

    sNode = aPredicate->node;

    if ( ( sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        //--------------------------------------
        // CNF 인 경우로,
        // 인자로 넘어온 predicate의 최상위 노드는 OR노드이며,
        // OR 노드 하위에 여러개의 비교연산자가 올 수 있다.
        // OR 노드 하위에 비교연산자가 하나일때와 여러개일때의
        // 조건검사가 틀리므로, 이를 구분하여 처리한다.
        //--------------------------------------

        // sNode는 비교연산자 노드
        sNode = (qtcNode *)(sNode->node.arguments);

        if ( aPredicate->node->node.arguments->next == NULL )
        {
            // 1. OR 노드 하위에 비교연산자가 하나만 오는 경우,
            // subquery가 존재하면 안된다.

            if ( ( aPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                sIsPartitionPrunablePred = ID_FALSE;
            }
            else
            {

                IDE_TEST( isPartitionPrunableOnePred( aStatement,
                                                      sNode,
                                                      aPartitionMethod,
                                                      aTableDependencies,
                                                      aOuterDependencies,
                                                      & sIsPartitionPrunablePred )
                          != IDE_SUCCESS );
            }

        }
        else
        {
            // 2. OR 노드 하위에 비교연산자가 여러개 오는 경우,
            //   (1) subquery가 존재하지 않아야 한다.
            //   (2) 비교연산자가 모두 indexable predicate이어야 한다.
            //   (3) 비교연산자 노드의 columnID가 모두 동일해야 한다.
            //       (단, column이 LIST인 경우는 제외한다.)

            // subquery가 존재하지 않아야 한다.
            if ( ( aPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                sIsPartitionPrunablePred = ID_FALSE;
            }
            else
            {
                while ( sNode != NULL )
                {
                    // partition prunable predicate인지 검사
                    IDE_TEST( isPartitionPrunableOnePred( aStatement,
                                                          sNode,
                                                          aPartitionMethod,
                                                          aTableDependencies,
                                                          aOuterDependencies,
                                                          & sIsPartitionPrunablePred )
                              != IDE_SUCCESS );

                    if ( sIsPartitionPrunablePred == ID_TRUE )
                    {
                        // columnID를 얻는다.
                        IDE_TEST( getColumnID( aStatement,
                                               sNode,
                                               ID_TRUE,
                                               & sColumnID )
                                  != IDE_SUCCESS );

                        if ( sIsFirstNode == ID_TRUE )
                        {
                            // OR 노드 하위의 첫번째 비교연산자 처리시,
                            // 이후의 columnID 비교를 위해,
                            // sFirstColumnID에 columnID를 저장.
                            sFirstColumnID = sColumnID;
                            sIsFirstNode   = ID_FALSE;
                        }
                        else
                        {
                            // Nothing To Do
                        }

                        // column이 LIST가 아닌 one column으로 구성되어 있고,
                        // 첫번째 비교연산자의 columnID와 같은지를 검사.
                        if ( ( sColumnID != QMO_COLUMNID_LIST )
                             && ( sColumnID == sFirstColumnID ) )
                        {
                            // Nothing To Do
                        }
                        else
                        {
                            sIsPartitionPrunablePred = ID_FALSE;
                        }
                    }
                    else
                    {
                        sIsPartitionPrunablePred = ID_FALSE;
                    }

                    if ( sIsPartitionPrunablePred == ID_TRUE )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        break;
                    }

                    sNode = (qtcNode *)(sNode->node.next);
                }

            }

        }
    }
    else
    {
        IDE_TEST( isPartitionPrunableOnePred( aStatement,
                                              sNode,
                                              aPartitionMethod,
                                              aTableDependencies,
                                              aOuterDependencies,
                                              & sIsPartitionPrunablePred )
                  != IDE_SUCCESS );
    }

    if ( sIsPartitionPrunablePred == ID_TRUE )
    {
        *aIsPartitionPrunable = ID_TRUE;
    }
    else
    {
        *aIsPartitionPrunable = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::isPartitionPrunableOnePred( qcStatement        * aStatement,
                                     qtcNode            * aNode,
                                     qcmPartitionMethod   aPartitionMethod,
                                     qcDepInfo          * aTableDependencies,
                                     qcDepInfo          * aOuterDependencies,
                                     idBool             * aIsPartitionPrunable )
{
/***********************************************************************
 *
 * Description : 비교연산자 단위로 partition prunable 여부를 판단한다.
 *
 * Implementation : qmoPred::isPartitionPrunable()의 주석 참조.
 *
 ************************************************************************/

    idBool    sIsTemp = ID_TRUE;
    idBool    sIsPartitionPrunablePred = ID_TRUE;
    qtcNode * sCompareNode;
    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoPred::isPartitionPrunableOnePred::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIsPartitionPrunable != NULL );

    //--------------------------------------
    // partition prunable 판단
    //--------------------------------------

    sCompareNode = aNode;

    while ( sIsTemp == ID_TRUE )
    {
        sIsTemp = ID_FALSE;

        if ( ( sCompareNode->node.lflag & MTC_NODE_INDEX_MASK )
             == MTC_NODE_INDEX_USABLE )
        {
            if ( ( sCompareNode->node.module == &mtfEqual )    ||
                 ( sCompareNode->node.module == &mtfEqualAll ) ||
                 ( sCompareNode->node.module == &mtfEqualAny ) ||
                 ( sCompareNode->node.module == &mtfIsNull ) )
            {
                // 비교연산자가 equal, IN, isnull 인 경우
                // hash, range, list partition method모두 가능.
                // Nothing To Do.

            }
            else
            {
                if ( ( aPartitionMethod == QCM_PARTITION_METHOD_HASH ) ||
                     ( aPartitionMethod == QCM_PARTITION_METHOD_LIST ) ||
                     ( aPartitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) )
                {
                    // list, hash는 equality연산과 isnull연산 빼고는 모두 불가능.
                    sIsPartitionPrunablePred = ID_FALSE;
                }
                else
                {
                    if ( ( sCompareNode->node.module == &mtfNotEqual )    ||
                         ( sCompareNode->node.module == &mtfNotEqualAny ) ||
                         ( sCompareNode->node.module == &mtfNotEqualAll ) )
                    {
                        // 비교연산자가 equal, IN 이 아닌 경우
                        // 컬럼쪽이 LIST이면, non-indexable로 분류
                        if ( ( sCompareNode->node.arguments->lflag &
                               MTC_NODE_INDEX_MASK ) == MTC_NODE_INDEX_USABLE )
                        {
                            sNode = (qtcNode *)(sCompareNode->node.arguments);
                        }
                        else
                        {
                            sNode = (qtcNode *)(sCompareNode->node.arguments->next);
                        }

                        if ( sNode->node.module == &mtfList )
                        {
                            sIsPartitionPrunablePred = ID_FALSE;
                        }
                        else
                        {
                            // Nothing To Do
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }

        }
        else
        {
            sIsPartitionPrunablePred = ID_FALSE;
        }

        if ( sIsPartitionPrunablePred == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }

        //--------------------------------------
        // 조건 4의 판단
        //   : column이 한쪽에만 존재해야 한다.
        //   (1) 두 operand의 dependency가 중복되지 않는지를 검사한다.
        //   (2) column이 모두 해당 table의 column으로 구성되었는지를 검사.
        //--------------------------------------
        IDE_TEST( isExistColumnOneSide( aStatement,
                                        sCompareNode,
                                        aTableDependencies,
                                        & sIsPartitionPrunablePred )
                  != IDE_SUCCESS );

        if ( sIsPartitionPrunablePred == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }


        if ( ( sCompareNode->node.module == &mtfIsNull ) ||
             ( sCompareNode->node.module == &mtfIsNotNull ) )
        {
            // IS NULL, IS NOT NULL 인 경우,
            // 예: i1 is null, i1 is not null
            // 이 경우는, value node를 가지지 않기 때문에,
            // (1) column에 conversion이 발생하지 않고,
            // (2) value에 대한 조건체크를 하지 않아도 된다.

            // fix BUG-15773
            // R-tree 인덱스가 설정된 레코드에 대해서
            // is null, is not null 연산자는 full scan해야 한다.
            if ( QC_SHARED_TMPLATE(aStatement)->tmplate.
                 rows[sCompareNode->node.arguments->table].
                 columns[sCompareNode->node.arguments->column].type.dataTypeId
                 ==  MTD_GEOMETRY_ID )
            {
                sIsPartitionPrunablePred = ID_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            //--------------------------------------
            // 조건 5의 판단
            //   : column과 value가 동일계열에 속하는 data type인지를 검사
            //--------------------------------------

            IDE_TEST( checkSameGroupType( aStatement,
                                          sCompareNode,
                                          & sIsPartitionPrunablePred )
                      != IDE_SUCCESS );

            if ( sIsPartitionPrunablePred == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                break;
            }

            //--------------------------------------
            // 조건 6의 판단
            //   : value에 대한 조건체크
            //   (1) host변수가 존재하면, indexable로 판단한다.
            //   (2) subquery가 오는 경우는 안됨
            //   (3) deterministic function이 오는 경우도 안 됨( BUG-39823 ).
            //   (4) LIKE에 대한 패턴문자는 일반문자로 시작하여야 한다.
            //--------------------------------------

            IDE_TEST( isPartitionPrunableValue( aStatement,
                                                sCompareNode,
                                                aOuterDependencies,
                                                & sIsPartitionPrunablePred )
                      != IDE_SUCCESS );

            if ( sIsPartitionPrunablePred == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                break;
            }
        }


    }

    if ( sIsPartitionPrunablePred == ID_TRUE )
    {
        *aIsPartitionPrunable = ID_TRUE;
    }
    else
    {
        *aIsPartitionPrunable = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::isPartitionPrunableValue( qcStatement * aStatement,
                                   qtcNode     * aNode,
                                   qcDepInfo   * aOuterDependencies,
                                   idBool      * aIsPartitionPrunable )
{
/***********************************************************************
 *
 * Description : indexable Predicate 판단시,
 *               value에 대한 조건 검사( 조건 6의 검사).
 *
 * Implementation :
 *
 *     1. host변수가 존재하면, indexable로 판단한다.
 *     2. subquery가 오는 경우 안됨.
 *     3. deterministion function인 경우도 안 됨( BUG-39823 ).
 *     4. LIKE에 대한 패턴문자는 일반문자로 시작하여야 한다.
 *
 ***********************************************************************/

    idBool     sIsTemp = ID_TRUE;
    idBool     sIsPartitionPrunableValue = ID_TRUE;
    qtcNode  * sCompareNode;
    qtcNode  * sValueNode;
    qtcNode  * sNode;

    IDU_FIT_POINT_FATAL( "qmoPred::isPartitionPrunableValue::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIsPartitionPrunable != NULL );

    //--------------------------------------
    // 조건6의 판단 : value의 조건 체크
    //--------------------------------------

    sCompareNode = aNode;

    if ( sCompareNode->indexArgument == 0 )
    {
        sValueNode = (qtcNode *)(sCompareNode->node.arguments->next);
    }
    else
    {
        sValueNode = (qtcNode *)(sCompareNode->node.arguments);
    }

    // PROJ-1492
    // 호스트 변수의 타입이 결정되더라도 그 값은 data binding전에는
    // 알 수 없다.
    if ( MTC_NODE_IS_DEFINED_VALUE( & sCompareNode->node ) == ID_FALSE )
    {
        // 1. host 변수가 존재하면, indexable로 판단한다.
        // Nothing To Do
    }
    else
    {
        while ( sIsTemp == ID_TRUE )
        {
            sIsTemp = ID_FALSE;

            for ( sNode  = sValueNode;
                  sNode != NULL;
                  sNode  = (qtcNode *)(sNode->node.next) )
            {
                // 2. subquery가 오는 경우 partition prunable하지 않다.
                if ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_SUBQUERY )
                {
                    sIsPartitionPrunableValue = ID_FALSE;
                    break;
                }
                else
                {
                    // subquery node가 아닌 경우
                    // Nothing To Do
                }

                /* BUG-39823
                   3. deterministic function node가 오는 경우,
                   partition prunable 하지 않다. */
                if ( ( ( sNode->lflag & QTC_NODE_PROC_FUNCTION_MASK )
                       == QTC_NODE_PROC_FUNCTION_TRUE ) &&
                     ( ( sNode->lflag & QTC_NODE_PROC_FUNC_DETERMINISTIC_MASK )
                       == QTC_NODE_PROC_FUNC_DETERMINISTIC_TRUE ) )
                {
                    sIsPartitionPrunableValue = ID_FALSE;
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            } // end of for()

            // 4. LIKE에 대한 패턴문자는 일반문자로 시작하여야 한다.
            if ( sCompareNode->node.module == &mtfLike )
            {
                // 일반문자로 시작하는 상수인지를 검사한다.
                if ( (QC_SHARED_TMPLATE( aStatement )->
                      tmplate.rows[sValueNode->node.table].lflag
                      & MTC_TUPLE_TYPE_MASK )
                     == MTC_TUPLE_TYPE_CONSTANT)
                {
                    IDE_TEST( isIndexableLIKE( aStatement,
                                               sCompareNode,
                                               & sIsPartitionPrunableValue )
                              != IDE_SUCCESS );

                    if ( sIsPartitionPrunableValue == ID_TRUE )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    // BUG-25594
                    // dynamic constant expression이면 indexable로 판단한다.
                    if ( qtc::isConstNode4LikePattern( aStatement,
                                                       sValueNode,
                                                       aOuterDependencies )
                         == ID_TRUE )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        sIsPartitionPrunableValue = ID_FALSE;                    
                        break;
                    }
                }
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    if ( sIsPartitionPrunableValue == ID_TRUE )
    {
        *aIsPartitionPrunable = ID_TRUE;
    }
    else
    {
        *aIsPartitionPrunable = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::isIndexableUnitPred( qcStatement * aStatement,
                              qtcNode     * aNode,
                              qcDepInfo   * aTableDependencies,
                              qcDepInfo   * aOuterDependencies,
                              idBool      * aIsIndexable )
{
/***********************************************************************
 *
 * Description : 비교연산자 단위로 indexable 여부를 판단한다.
 *
 * Implementation : qmoPred::isIndexable()의 주석 참조.
 *
 ************************************************************************/

    idBool    sIsTemp = ID_TRUE;
    idBool    sIsIndexableUnitPred = ID_TRUE;
    qtcNode * sCompareNode;
    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoPred::isIndexableUnitPred::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIsIndexable != NULL );

    //--------------------------------------
    // indexable 판단
    //--------------------------------------

    sCompareNode = aNode;

    while ( sIsTemp == ID_TRUE )
    {
        sIsTemp = ID_FALSE;

        //--------------------------------------
        // 조건 1, 2, 3의 판단
        //  (1) 조건 1 : indexable operator 이어야 한다.
        //  (2) 조건 2 : column이 있어야 한다.
        //  (3) 조건 3 : column에 연산이 없어야 한다.
        //--------------------------------------

        if ( ( sCompareNode->node.lflag & MTC_NODE_INDEX_MASK )
             == MTC_NODE_INDEX_USABLE )
        {
            // LIST 컬럼인 경우는,
            // equal(=)과 IN 연산자만 indexable operator로 판단한다.
            // 비교연산자가 equal과 IN이 아니라면,
            // 비교연산자 하위 컬럼쪽이 LIST 인지 검사.
            // LIST 컬럼이 올 수 있는 비교연산자는 다음과 같다.
            // [ =, !=, =ANY(IN), !=ANY, =ALL, !=ALL(NOT IN) ]
            if ( ( sCompareNode->node.module == &mtfEqual )    ||
                 ( sCompareNode->node.module == &mtfEqualAll ) ||
                 ( sCompareNode->node.module == &mtfEqualAny ) )
            {
                // 비교연산자가 equal, IN 인 경우
                // Nothing To Do
            }
            else if ( ( sCompareNode->node.module == &mtfNotEqual )    ||
                      ( sCompareNode->node.module == &mtfNotEqualAny ) ||
                      ( sCompareNode->node.module == &mtfNotEqualAll ) )
            {
                // 비교연산자가 equal, IN 이 아닌 경우
                // 컬럼쪽이 LIST이면, non-indexable로 분류

                if ( ( sCompareNode->node.arguments->lflag &
                       MTC_NODE_INDEX_MASK )
                     == MTC_NODE_INDEX_USABLE )
                {
                    sNode = (qtcNode *)(sCompareNode->node.arguments);
                }
                else
                {
                    sNode = (qtcNode *)(sCompareNode->node.arguments->next);
                }

                if ( sNode->node.module == &mtfList )
                {
                    sIsIndexableUnitPred = ID_FALSE;
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                /* Nothing to Do */
                // BETWEEN , NOT BETWEEN , IS NULL, IS NOT NULL
                // NVL_EQUAL, NOT NVL_EQUL 비교 연산자가 올수 있다.
            }
        }
        else
        {
            sIsIndexableUnitPred = ID_FALSE;
        }

        if ( sIsIndexableUnitPred == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }

        //--------------------------------------
        // 조건 4의 판단
        //   : column이 한쪽에만 존재해야 한다.
        //   (1) 두 operand의 dependency가 중복되지 않는지를 검사한다.
        //   (2) column이 모두 해당 table의 column으로 구성되었는지를 검사.
        //--------------------------------------

        IDE_TEST( isExistColumnOneSide( aStatement,
                                        sCompareNode,
                                        aTableDependencies,
                                        & sIsIndexableUnitPred )
                  != IDE_SUCCESS );

        if( sIsIndexableUnitPred == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }

        if( ( sCompareNode->node.module == &mtfIsNull ) ||
            ( sCompareNode->node.module == &mtfIsNotNull ) )
        {
            // IS NULL, IS NOT NULL 인 경우,
            // 예: i1 is null, i1 is not null
            // 이 경우는, value node를 가지지 않기 때문에,
            // (1) column에 conversion이 발생하지 않고,
            // (2) value에 대한 조건체크를 하지 않아도 된다.

            // fix BUG-15773
            // R-tree 인덱스가 설정된 레코드에 대해서
            // is null, is not null 연산자는 full scan해야 한다.
            if ( QC_SHARED_TMPLATE( aStatement )->tmplate.
                 rows[sCompareNode->node.arguments->table].
                 columns[sCompareNode->node.arguments->column].type.dataTypeId
                 == MTD_GEOMETRY_ID )
            {
                sIsIndexableUnitPred = ID_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
             if ( sCompareNode->node.module == &mtfInlist )
            {
                /* BUG-32622 inlist operator
                   INLIST 인 경우
                   예: inlist(i1, 'aa,bb,cc')
                   이 경우는 value node는 column과 다른 type이지만
                   column에 index가 있는 경우 강제로 index를 태우기때문에
                   항상 indexable하다고 판단한다.
                   (index를 못하는 경우는 에러를 반환한다.)
                */
                sNode = (qtcNode *)(sCompareNode->node.arguments);
                sNode->lflag &= ~QTC_NODE_CHECK_SAMEGROUP_MASK;
                sNode->lflag |= QTC_NODE_CHECK_SAMEGROUP_TRUE;
            }
            else
            {
                //--------------------------------------
                // 조건 5의 판단
                //   : column과 value가 동일계열에 속하는 data type인지를 검사
                //--------------------------------------

                IDE_TEST( checkSameGroupType( aStatement,
                                              sCompareNode,
                                              & sIsIndexableUnitPred )
                          != IDE_SUCCESS );

                if ( sIsIndexableUnitPred == ID_TRUE )
                {
                    // Nothing To Do
                }
                else
                {
                    break;
                }

                //--------------------------------------
                // 조건 6의 판단
                //   : value에 대한 조건체크
                //   (1) host변수가 존재하면, indexable로 판단한다.
                //   (2) subquery가 오는 경우, subquery type이 A, N형이어야 한다.
                //   (3) LIKE에 대한 패턴문자는 일반문자로 시작하여야 한다.
                //--------------------------------------

                IDE_TEST( isIndexableValue( aStatement,
                                            sCompareNode,
                                            aOuterDependencies,
                                            & sIsIndexableUnitPred )
                          != IDE_SUCCESS );

                if ( sIsIndexableUnitPred == ID_TRUE )
                {
                    // Nothing To Do
                }
                else
                {
                    break;
                }
            }
        }
    }

    *aIsIndexable = sIsIndexableUnitPred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isExistColumnOneSide( qcStatement * aStatement,
                               qtcNode     * aNode,
                               qcDepInfo   * aTableDependencies,
                               idBool      * aIsIndexable )
{
/***********************************************************************
 *
 * Description : indexable predicate 판단시,
 *               컬럼이 한쪽에만 존재하는지를 판단(조건4의 판단).
 *
 * Implementation :
 *
 *    1. 두 operand의 dependency가 중복되지 않아야 한다.
 *        dependencies 중복 =
 *               ( ( 비교연산자하위의 두개 노드의 dependencies를 AND연산 )
 *                 & FROM의 해당 table의 dependency ) != 0 )
 *
 *    2. column이 모두 해당 table의 column으로 구성되어야 한다.
 *       column이 한쪽에만 존재한다 하더라도,
 *       LIST 하위에 outer column, 상수, 비교연산자가 아직 존재할 수 있고,
 *       one column인 경우도 컬럼이 아닌 비교연산자일 수 있다.
 *       (1) LIST : outer column이 존재하지 않아야 한다.
 *                  모두 column이어야 한다.
 *       (2) one column : 모두 column이어야 한다.
 *
 ***********************************************************************/

    qcDepInfo sAndDependencies;
    qcDepInfo sResultDependencies;
    UInt      sIndexArgument;
    idBool    sIsTemp = ID_TRUE;
    idBool    sIsExistColumnOneSide = ID_TRUE;
    qtcNode * sCompareNode;
    qtcNode * sColumnNode;
    qtcNode * sCurNode = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::isExistColumnOneSide::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIsIndexable != NULL );

    //--------------------------------------
    // 조건4의 판단 : 컬럼이 한쪽에만 존재해야 한다.
    //--------------------------------------

    while ( sIsTemp == ID_TRUE )
    {
        sIsTemp = ID_FALSE;
        sCompareNode = aNode;

        //----------------------------
        // 1. 두 operand의 dependency가 중복되지 않아야 한다.
        //----------------------------

        // 1) IS NULL, IS NOT NULL 인 경우
        //    비교연산자 하위에 하나의 노드만 존재한다.
        //    따라서, dependency 중복 검사는 하지 않아도 됨.
        // 2) BETWEEN, NOT BETWEEN 인 경우
        //    비교연산자 노드 하위에 세개의 노드가 존재한다.
        //     [비교연산자노드]   i1 between 1 and 2
        //             |
        //            \ /
        //        [컬럼노드] -> [value노드] -> [value노드]
        //      따라서, 추가로 나머지 하나의 value 노드에 대한
        //      dependency 중복검사도 수행해야 한다.
        //
        //    NVL_EQUAL, NOT NVL_EQUAL
        //    비교 연산자 노드 하위에 세개의 노드가 존재한다.
        //    [컬럼노드] -> [value노드] -> [value노드]
        //     마지막 첫번째 value노드는 관계 없다.
        //
        // 3) 나머지 비교연산자들을 비교연산자 하위 두개 노드에 대한 검사.
        //

        for ( sCurNode  = (qtcNode *)(sCompareNode->node.arguments->next);
              sCurNode != NULL;
              sCurNode  = (qtcNode *)(sCurNode->node.next) )
        {
            // (1). 비교연산자 하위 두개 노드의 dependencies를 AND 연산
            qtc::dependencyAnd(
                & ((qtcNode*)(sCompareNode->node.arguments))->depInfo,
                & sCurNode->depInfo,
                & sAndDependencies );

            // (2). (1)의 결과와 FROM의 해당 table의 dependencies를 AND 연산
            qtc::dependencyAnd( & sAndDependencies,
                                aTableDependencies,
                                & sResultDependencies );

            // (3). (2)의 결과가 0가 아니면, dependency 중복으로
            // 비교연산자 하위 두개 노드 모두 같은 테이블의 컬럼이 존재한다.
            if ( qtc::dependencyEqual( & sResultDependencies,
                                       & qtc::zeroDependencies ) == ID_TRUE )
            {
                // 컬럼이 한쪽에만 존재하는 경우

                // between과 not between 비교연산자는
                // 나머지 하나의 value node에 대한 dependency 중복검사수행.
                if ( sCurNode->node.next != NULL )
                {
                    // To Fix PR-8728
                    // 연산자 노드의 모듈을 검사하여야 함.
                    if ( ( sCompareNode->node.module == &mtfBetween )    ||
                         ( sCompareNode->node.module == &mtfNotBetween ) ||
                         ( sCompareNode->node.module == &mtfNvlEqual )   ||
                         ( sCompareNode->node.module == &mtfNvlNotEqual ) )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                // NVL_EQUAL (expr1, expr2, expr3)
                // expr2는 관련이 없다.
                if ( sCurNode->node.next != NULL )
                {
                    if ( ( sCompareNode->node.module == &mtfNvlEqual ) ||
                         ( sCompareNode->node.module == &mtfNvlNotEqual ) )
                    {
                        /* Nothing to do */
                    }
                    else
                    {
                        sIsExistColumnOneSide = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    sIsExistColumnOneSide = ID_FALSE;
                    break;
                }
            }
        }

        if ( sIsExistColumnOneSide == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }

        //----------------------------
        // 2. column에 대한 추가 검증으로,
        //    해당 table의 컬럼만이 존재하는지를 검사.
        //----------------------------

        // (1). columnNode를 구한다.
        //      columnNode는 해당 table의 dependencies와 동일한 노드
        //      ( LIST의 경우, outer Column존재가 여기서 검사됨.)

        // 원래 있던 정책은 -------------------------------------------------------
        // indexable operator 중,
        // =, !=, <, >, <=, >= 만
        // 컬럼노드와 value노드의 위치가 바뀔 수 있다.
        // 따라서, 이 경우만 비교연산자 하위 두 노드를 모두 검사해서
        // 컬럼 노드를 구한다.
        //  예) i1 = 1 , 1 = i1 과 같이 사용 가능.
        // 이 외의 비교연산자는 컬럼노드가 비교연산자의 argument에만
        // 올 수 있으므로, 비교연산자의 argument만 검사.
        //  예) i1 between 1 and 2, i1 in ( 1, 2 )
        // AST가 추가 되면서 ------------------------------------------------------
        // PR-15291 "Geometry 연산자 중 파라미터의 순서에 따라 값이 틀린 연산자가 있습니다"
        // Geometry 객체도 컬럼노드와 value노드의 위치가 바뀔 수 있어야 하므로
        // MTC_NODE_INDEX_ARGUMENT_BOTH 플래그를 이용해 인덱스 수행을 결정한다.
        if ( qtc::dependencyEqual(
                 & ((qtcNode *)(sCompareNode->node.arguments))->depInfo,
                 aTableDependencies ) == ID_TRUE )
        {
            sIndexArgument = 0;
            sColumnNode = (qtcNode *)(sCompareNode->node.arguments);
        }
        else
        {
            // To Fix PR-15291
            if ( ( sCompareNode->node.module->lflag &
                   MTC_NODE_INDEX_ARGUMENT_MASK )
                 == MTC_NODE_INDEX_ARGUMENT_BOTH )
            {
                if ( qtc::dependencyEqual(
                         & ((qtcNode *)(sCompareNode->node.arguments->next))->depInfo,
                         aTableDependencies ) == ID_TRUE )
                {
                    sIndexArgument = 1;
                    sColumnNode =
                        (qtcNode*)(sCompareNode->node.arguments->next);
                }
                else
                {
                    sIsExistColumnOneSide = ID_FALSE;
                }
            }
            else
            {
                sIsExistColumnOneSide = ID_FALSE;
            }
        }

        if ( sIsExistColumnOneSide == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }

        // (2) columnNode가 해당 table의 dependencies와 동일하다 하더라도,
        //     .LIST       : 하위에 상수와 비교연산자가 있을 수 있고,
        //     .one column : 하위에 비교연산자가 있을 수 있으므로,
        //     모두 컬럼으로 구성되었는지를 검사.
        if ( ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
             == MTC_NODE_OPERATOR_LIST )
        {
            // LIST인 경우

            sCurNode = (qtcNode *)(sColumnNode->node.arguments);

            while ( sCurNode != NULL )
            {
                if ( QTC_IS_COLUMN( aStatement, sCurNode ) == ID_TRUE )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsExistColumnOneSide = ID_FALSE;
                    break;
                }
                sCurNode = (qtcNode *)(sCurNode->node.next);
            }
        }
        else
        {
            // one column인 경우

            if ( QTC_IS_COLUMN( aStatement, sColumnNode ) == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                sIsExistColumnOneSide = ID_FALSE;
            }
        }
    } // end of while()

    //--------------------------------------
    // indexArgument의 설정.
    //--------------------------------------

    if ( sIsExistColumnOneSide == ID_TRUE )
    {
        sCompareNode->indexArgument = sIndexArgument;
        *aIsIndexable = ID_TRUE;
    }
    else
    {
        *aIsIndexable = ID_FALSE;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::checkSameGroupType( qcStatement * aStatement,
                             qtcNode     * aNode,
                             idBool      * aIsIndexable )
{
/***********************************************************************
 *
 * Description : indexable predicate 판단시,
 *               column에 conversion이 발생했는지를 검사(조건 5의 검사).
 *
 * Implementation :
 *
 *    column과 value가 동일한 계열에 속하는 데이터타입인지를 판단한다.
 *
 * 예: select * from t1
 *   where (i1, i2) = ( (select i1 from t2), (select i2 from t2) );
 *
 *   [ = ]
 *     |
 *   [LIST]--------[LIST]  <------------------- sCurValue
 *     |             |
 *   [I1]--[I2]    [SUBQUERY,INDIRECT]--[SUBQUERY,INDIRECT] <--sValue
 *     |             |                       |
 *    sCurColumn   [t2.i1]              [t2.i2]
 *
 * 예: select * from t1
 *     where (i1, i2) = ((select max(i1) from t2), (select max(i2) from t2) );
 *
 *   [ = ]
 *     |
 *   [LIST]--------[LIST]  <------------------- sCurValue
 *     |             |
 *   [I1]--[I2]    [SUBQUERY,INDIRECT]--[SUBQUERY,INDIRECT] <--sValue
 *     |             |                       |
 *    sCurColumn   [max(i1)]              [max(i2)]
 *                   |                       |
 *                  [t2.i1]               [t2.i2]
 *
 * 예: where ( i1, i2 ) in ( (select i1, i2 from t1 where i1=1),
 *                           (select i1, i2 from t2 where i1=2) );
 *
 *   [ IN ]
 *     |
 *   [LIST]--------[LIST]  |------------------- sCurValue
 *     |             |     |
 *   [I1]--[I2]    [SUBQUERY]-------------[SUBQUERY]
 *     |             |                       |
 *    sCurColumn   [t1.i1]      [t2.i1]    [t2.i1]   [t2.i2] <--sValue
 *
 * 예: where ( i1, i2 ) in ( (select min(i1), min(i2) from t1),
 *                           (select max(i1), max(i2) from t2) );
 *
 *   [ IN ]
 *     |
 *   [LIST]--------[LIST]  |------------------- sCurValue
 *     |             |     |
 *   [I1]--[I2]    [SUBQUERY]-------------[SUBQUERY]
 *     |             |                       |
 *     |           [min(i1)]--[min(i2)]   [min(i1)]--[min(i2)] <---sValue
 *    sCurColumn     |             |         |          |
 *                 [t1.i1]      [t2.i1]    [t2.i1]   [t2.i2]
 *
 * 예:  where ( i1, i2 ) in ( select (select min(i1) from t1),
 *                                   (select min(i1) from t2)
 *                            from t2 );
 *   [ IN ]
 *     |
 *   [LIST]----------[LIST] <------------------- sCurValue
 *     |               |
 *   [I1]--[I2]      [INDIRECT]---[INDIRECT] <--- sValue
 *     |               |             |
 *     |             [min(i1)]    [min(i1)]
 *    sCurColumn       |             |
 *                   [t1.i1]      [t2.i1]
 *
 * 예: where ( i1, i2 ) in ( select (select (select min(i1) from t1) from t1),
 *                                  (select (select min(i1) from t2) from t2)
 *                           from t2 );
 *
 *   [ IN ]
 *     |
 *   [LIST]----------[LIST] <------------------- sCurValue
 *     |               |
 *   [I1]--[I2]      [INDIRECT]---[INDIRECT] <--- sValue
 *     |               |             |
 *     |             [INDIRECT]   [INDIRECT]
 *     |               |             |
 *     |             [min(i1)]    [min(i1)]
 *    sCurColumn       |             |
 *                   [t1.i1]      [t2.i1]
 *
 ***********************************************************************/

    idBool    sIsNotExist = ID_TRUE;
    qtcNode * sCompareNode;
    qtcNode * sColumnNode;
    qtcNode * sValueNode;
    qtcNode * sCurColumn;
    qtcNode * sCurValue;
    qtcNode * sValue;
    qtcNode * sValue2;
    qtcNode * sColumn;

    IDU_FIT_POINT_FATAL( "qmoPred::checkSameGroupType::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIsIndexable != NULL );

    //--------------------------------------
    // 조건5의 판단 : column에 conversion이 발생하지 않아야 한다.
    // column과 value가 동일계열에 속하는 data type인지를 검사
    //--------------------------------------

    sCompareNode = aNode;

    if ( sCompareNode->indexArgument == 0 )
    {
        sColumnNode = (qtcNode *)(sCompareNode->node.arguments);
        sValueNode  = (qtcNode *)(sCompareNode->node.arguments->next);
    }
    else
    {
        sColumnNode = (qtcNode *)(sCompareNode->node.arguments->next);
        sValueNode  = (qtcNode *)(sCompareNode->node.arguments);
    }

    // PROJ-1364
    // column node에 conversion이 존재하더라도
    // value와 동일계열의 data type이면, indexable predicate으로
    // 분류해서, index를 탈 수 있도록 한다.

    if ( ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_LIST )
    {
        // column node가 LIST인 경우,
        // (i1,i2,i3) = ( 1,2,3 )
        // (i1,i2,i3) in ( (1,1,1), (2,2,2) )
        // (i1,i2,i3) in ( (select i1,i2,i3 ...), (select i1,i2,i3 ...) )
        // (i1,i2) in ( select (select min(i1) from ... ),
        //                     (select min(i2) from ... )
        //              from ... )

        sCurColumn = (qtcNode *)(sColumnNode->node.arguments);

        if ( ( ( sValueNode->node.arguments->lflag &
                 MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_LIST )
             ||
             ( ( ( sValueNode->node.arguments->lflag &
                   MTC_NODE_OPERATOR_MASK )
                 == MTC_NODE_OPERATOR_SUBQUERY )
               &&
               ( ( sValueNode->node.arguments->lflag &
                   MTC_NODE_INDIRECT_MASK )
                 == MTC_NODE_INDIRECT_FALSE ) ) )
        {
            sCurValue = (qtcNode*)(sValueNode->node.arguments);
        }
        else
        {
            // (i1,i2,i3)=(1,2,3)
            sCurValue = sValueNode;
        }
    }
    else
    {
        // column node가 column인 경우
        // i1 = 1
        // i1 in ( 1, 2, 3 )
        // i1 in ( select i1 from ... )

        sCurColumn = sColumnNode;

        if ( ( ( sValueNode->node.lflag & MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_LIST )
             ||
             ( ( sValueNode->node.lflag & MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_SUBQUERY ) )
        {
            sCurValue = (qtcNode*)(sValueNode->node.arguments);
        }
        else
        {
            sCurValue = sValueNode;
        }
    }

    while ( ( sIsNotExist == ID_TRUE ) &&
            ( sCurValue != NULL ) && ( sCurValue != sColumnNode ) )
    {
        // (1=i1)의 경우, value->next가 존재함으로
        // columnNode와 valueNode가 같지 않음을 검사.

        // ( i1, i2 ) in ( ( 1,1 ), ( 2, 2 ) )
        // ( i1, i2 ) in ( select i1, i2 from ... )
        if ( ( ( sCurValue->node.lflag & MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_LIST )
             ||
             ( ( sCurValue->node.lflag & MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_SUBQUERY ) )
        {
            sValue  = (qtcNode*)(sCurValue->node.arguments);
        }
        else
        {
            sValue = sCurValue;
        }

        sColumn = sCurColumn;

        while ( ( sColumn != NULL ) && ( sColumn != sValueNode ) )
        {
            // (i1=1)의 경우, column->next가 존재함으로
            // columnNode와 valueNode가 같지 않음을 검사.

            // QTC_NODE_CHECK_SAMEGROUP_MASK를 설정하는 이유는,
            // qmoKeyRange::isIndexable() 함수내에서
            //  (1) host 변수가 binding 된 후,
            //  (2) sort temp table에 대한 keyRange 생성시,
            // 동일계열의 index 사용가능한지를 판단하게 되며,
            // 이때, prepare 단계에서 이미 판단된 predicate에 대해,
            // 중복 검사하지 않기 위해

            // fix BUG-12058 BUG-12061
            sColumn->lflag &= ~QTC_NODE_CHECK_SAMEGROUP_MASK;
            sColumn->lflag |= QTC_NODE_CHECK_SAMEGROUP_TRUE;

            // fix BUG-32079
            // sValue may be one column subquery node.
            // ex) 
            // select * from dual where ( dummy, dummy ) in ( ( 'X', ( select 'X' from dual ) ) ); 
            // select * from dual where ( dummy, dummy ) in ( ( ( select 'X' from dual ), ( select 'X' from dual ) ) ); 
            if ( isOneColumnSubqueryNode( sValue ) == ID_TRUE )
            {
                sValue2  = (qtcNode*)(sValue->node.arguments);
            }
            else
            {
                sValue2  = sValue;
            }

            if ( isSameGroupType( QC_SHARED_TMPLATE(aStatement),
                                  sColumn,
                                  sValue2 ) == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                sIsNotExist = ID_FALSE;
                break;
            }

            IDE_FT_ASSERT( sValue != NULL );

            sColumn = (qtcNode*)(sColumn->node.next);
            sValue  = (qtcNode*)(sValue->node.next);
        }

        sCurValue  = (qtcNode*)(sCurValue->node.next);
    }

    if ( sIsNotExist == ID_TRUE )
    {
        *aIsIndexable = ID_TRUE;
    }
    else
    {
        *aIsIndexable = ID_FALSE;
    }

    return IDE_SUCCESS;
}

idBool
qmoPred::isSameGroupType( qcTemplate  * aTemplate,
                          qtcNode     * aColumnNode,
                          qtcNode     * aValueNode )
{
/***********************************************************************
 *
 * Description : 컬럼에 conversion이 존재하는 경우,
 *               value node와 동일계열의 데이터타입인지를 판단.
 *
 * Implementation :
 *
 *    참조 : PROJ-1364
 *
 *    data type의 분류
 *
 *    -------------------------------------------------------------
 *    문자형계열 | CHAR, VARCHAR, NCHAR, NVARCHAR,
 *               | BIT, VARBIT, ECHAR, EVARCHAR
 *    -------------------------------------------------------------
 *    숫자형계열 | Native | 정수형 | BIGINT, INTEGER, SMALLINT
 *               |        |----------------------------------------
 *               |        | 실수형 | DOUBLE, REAL
 *               --------------------------------------------------
 *               | Non-   | 고정소수점형 | NUMERIC, DECIMAL,
 *               | Native |              | NUMBER(p), NUMBER(p,s)
 *               |        |----------------------------------------
 *               |        | 부정소수점형 | FLOAT, NUMBER
 *    -------------------------------------------------------------
 *    To fix BUG-15768
 *    기타 계열  | NIBBLE, BYTE 는 대표타입이 없음.
 *               | 따라서 동일계열 비교를 할 수 없음.
 *    -------------------------------------------------------------
 *
 ***********************************************************************/

    UInt         sColumnType = 0;
    UInt         sValueType  = 0;
    idBool       sIsSameGroupType = ID_TRUE;
    qtcNode    * sColumnConversionNode;
    qtcNode    * sCheckValue;
    mtcColumn  * sColumnColumn;
    mtcColumn  * sValueColumn;

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate   != NULL );
    IDE_DASSERT( aColumnNode != NULL );

    //--------------------------------------
    // column과 value가 동일 계열의 data type인지를 판단
    //--------------------------------------

    sColumnConversionNode = aColumnNode;

    // fix BUG-12061
    // 예:  where ( i1, i2 ) = ( (select max(i1) ..),
    //                           (select max(i2) ..) )
    //  aValueNode가 indirect node인 경우가 있으므로,
    //  이때, 실제 비교대상 노드를 찾는다.
    //  [ = ]
    //    |
    //  [LIST]-----------[LIST]    |----- aValueNode-----|
    //    |                 |      |                     |
    //   [I1]-[I2]       [SUBQUERY,INDIRECT]---[SUBQUERY-INDIRECT]
    //                      |                      |
    //                   [MAX(I1)]                [MAX(I2)]
    //                      |                      |
    //                     [I1]                   [I2]
    //
    // fix BUG-16047
    // conversion node가 달려있는 경우,
    // 실제 비교대상 노드는 conversion node이다.
    if( aValueNode == NULL )
    {
        sCheckValue = NULL;
    }
    else
    {
        sCheckValue = (qtcNode*)
            mtf::convertedNode( (mtcNode*) aValueNode,
                                & aTemplate->tmplate );
    }

    if( aColumnNode->node.module == &qtc::passModule )
    {
        // To Fix PR-8700
        // t1.i1 + 1 > t2.i1 + 1 과 같은 연산을 처리할 때,
        // 연산의 반복 수행을 방지하기 위해 Pass Node를 사용하여
        // 연결하게 되며, Pass Node는 반드시 Indirection된다.
        //
        //      [>]
        //       |
        //       V
        //      [+] ---> [Pass]
        //                 |
        //                 V
        //                [+]
        //
        // 따라서, Key Range 생성시에 Pass Node의 경우
        // Conversion이 발생하지 않더라도 Conversion이 발생한 것으로
        // 판단된다.  이러한 경우 Pass Node는 Conversion 과 전혀
        // 관계 없는 Node이며, Conversion이 발생하지 않은 것으로
        // 판단하여야 한다.

        // Nothing To Do
    }
    else
    {
        if( aColumnNode->node.conversion == NULL )
        {
            if( aValueNode != NULL )
            {
                // 예 : varchar_col in ( intger'1', bigint'1' )

                // BUG-21936
                // aValueNode의 leftCoversion이 고려되어야 한다.
                if( aValueNode->node.leftConversion != NULL )
                {
                    sColumnConversionNode =
                        (qtcNode*)(aValueNode->node.leftConversion);
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // 예 : i1 is null
                // Nothing To Do
            }
        }
        else
        {
            sColumnConversionNode = (qtcNode*)
                mtf::convertedNode( (mtcNode*)aColumnNode,
                                    & aTemplate->tmplate );
        }
    }

    if( aColumnNode == sColumnConversionNode )
    {
        // Nothing To Do
    }
    else
    {
        IDE_FT_ASSERT( sCheckValue != NULL );

        sColumnColumn = QTC_TMPL_COLUMN( aTemplate, aColumnNode );
        sValueColumn  = QTC_TMPL_COLUMN( aTemplate, sCheckValue );
        
        sColumnType = ( sColumnColumn->module->flag & MTD_GROUP_MASK );
        sValueType  = ( sValueColumn->module->flag & MTD_GROUP_MASK );

        if( sColumnType == sValueType )
        {
            if( sColumnType == MTD_GROUP_TEXT )
            {
                // PROJ-2002 Column Security
                // echar, evarchar는 text group이나 다음과 같은 경우
                // group compare가 불가능하다.
                //
                // -------------+-------------------------------------------
                //              |                column
                //              +----------+----------+----------+----------
                //              | char     | varchar  | echar    | evarchar
                // ---+---------+----------+----------+----------+----------
                //    | char    | char     | varchar  | echar    | evarchar 
                //    |         |          |          | (불가능) | (불가능)
                //  v +---------+----------+----------+----------+----------
                //  a | varchar | varchar  | varchar  | varchar  | evarchar 
                //  l |         |          |          | (불가능) | (불가능)
                //  u +---------+----------+----------+----------+----------
                //  e | echar   | echar    | varchar  | echar    | evarchar
                //    |         | (불가능) | (불가능) |          | (불가능)
                //    +---------+----------+----------+----------+----------
                //    | evarchar| evarchar | evarchar | evarchar | evarchar 
                //    |         | (불가능) | (불가능) | (불가능) |
                // ---+---------+----------+----------+----------+----------
                
                if ( ( sColumnColumn->module->id == MTD_ECHAR_ID ) ||
                     ( sColumnColumn->module->id == MTD_EVARCHAR_ID ) ||
                     ( sValueColumn->module->id == MTD_ECHAR_ID ) ||
                     ( sValueColumn->module->id == MTD_EVARCHAR_ID ) )
                {
                    // column이나 value에 암호 타입이 있다면
                    // 동일 계열 그룹 비교가 불가능.
                    sIsSameGroupType = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }
                
                // BUG-26283
                // nchar, nvarchar는 text group이나 다음과 같은 경우
                // group compare가 불가능하다.
                //
                // -------------+-------------------------------------------
                //              |                column
                //              +----------+----------+----------+----------
                //              | char     | varchar  | nchar    | nvarchar
                // ---+---------+----------+----------+----------+----------
                //    | char    | char     | varchar  | nchar    | nvarchar 
                //    |         |          |          |          |
                //  v +---------+----------+----------+----------+----------
                //  a | varchar | varchar  | varchar  | nvarchar | nvarchar 
                //  l |         |          |          |          | 
                //  u +---------+----------+----------+----------+----------
                //  e | nchar   | nchar    | nvarchar | nchar    | nvarchar
                //    |         | (불가능) | (불가능) |          |
                //    +---------+----------+----------+----------+----------
                //    | nvarchar| nvarchar | nvarchar | nvarchar | nvarchar 
                //    |         | (불가능) | (불가능) |          |
                // ---+---------+----------+----------+----------+----------
                
                if ( ( ( sColumnColumn->module->id == MTD_CHAR_ID ) ||
                       ( sColumnColumn->module->id == MTD_VARCHAR_ID ) )
                     &&
                     ( ( sValueColumn->module->id == MTD_NCHAR_ID ) ||
                       ( sValueColumn->module->id == MTD_NVARCHAR_ID ) ) )
                {
                    // column이 char/varchar이고 value가 nchar/nvarchar이면
                    // 동일 계열 그룹 비교가 불가능.
                    sIsSameGroupType = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }    
            }
            else
            {
                // Nothing To Do
            }
            
            if( sColumnType == MTD_GROUP_MISC )
            {
                // To fix BUG-15768
                // 기타 그룹은 대표타입이 존재하지 않음.
                // 따라서, 동일 계열 그룹 비교가 불가능.
                sIsSameGroupType = ID_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            sIsSameGroupType = ID_FALSE;
        }
    }

    return sIsSameGroupType;
}


IDE_RC
qmoPred::isIndexableValue( qcStatement * aStatement,
                           qtcNode     * aNode,
                           qcDepInfo   * aOuterDependencies,
                           idBool      * aIsIndexable )
{
/***********************************************************************
 *
 * Description : indexable Predicate 판단시,
 *               value에 대한 조건 검사( 조건 6의 검사).
 *
 * Implementation :
 *
 *     1. host변수가 존재하면, indexable로 판단한다.
 *     2. subquery가 오는 경우, subquery type이 A, N형이어야 한다.
 *        subquery최적화 팁 적용결과가
 *        subquery keyRange or IN절의 subquery keyRange 인지 검사.
 *     3. LIKE에 대한 패턴문자는 일반문자로 시작하여야 한다.
 *
 ***********************************************************************/

    idBool     sIsTemp = ID_TRUE;
    idBool     sIsIndexableValue = ID_TRUE;
    qtcNode  * sCompareNode;
    qtcNode  * sValueNode;
    qtcNode  * sNode;
    qmgPROJ  * sPROJGraph;

    IDU_FIT_POINT_FATAL( "qmoPred::isIndexableValue::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIsIndexable != NULL );

    //--------------------------------------
    // 조건6의 판단 : value의 조건 체크
    //--------------------------------------

    sCompareNode = aNode;

    if ( sCompareNode->indexArgument == 0 )
    {
        sValueNode = (qtcNode *)(sCompareNode->node.arguments->next);
    }
    else
    {
        sValueNode = (qtcNode *)(sCompareNode->node.arguments);
    }

    // PROJ-1492
    // 호스트 변수의 타입이 결정되더라도 그 값은 data binding전에는
    // 알 수 없다.
    if ( MTC_NODE_IS_DEFINED_VALUE( & sCompareNode->node ) == ID_FALSE )
    {
        // 1. host 변수가 존재하면, indexable로 판단한다.
        // Nothing To Do
    }
    else
    {
        while ( sIsTemp == ID_TRUE )
        {
            sIsTemp = ID_FALSE;

            // 2. subquery가 오는 경우, subquery type이 A, N형이어야 한다.
            //    subquery최적화 팁 적용결과가
            //    subquery keyRange or IN절의 subquery keyRange 인지 검사.
            //    단, between, not between인 경우는
            //    store and search 최적화 팁인지를 검사한다.

            for ( sNode  = sValueNode;
                  sNode != NULL;
                  sNode  = (qtcNode *)(sNode->node.next) )
            {
                if ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_SUBQUERY )
                {
                    sPROJGraph = (qmgPROJ *)(sNode->subquery->myPlan->graph);

                    if ( ( sCompareNode->node.module == &mtfBetween ) ||
                         ( sCompareNode->node.module == &mtfNotBetween ) )
                    {
                        // between, not between인 경우

                        if ( ( sPROJGraph->subqueryTipFlag
                               & QMG_PROJ_SUBQUERY_TIP_MASK )
                             == QMG_PROJ_SUBQUERY_TIP_STORENSEARCH )
                        {
                            // Nothing To Do
                        }
                        else
                        {
                            sIsIndexableValue = ID_FALSE;
                            break;
                        }
                    }
                    else
                    {
                        // between, not between이 아닌 경우
                        // value node가 하나이므로,
                        // 하나의 value node 검사 후, for문을 빠져 나간다.

                        if ( ( ( sPROJGraph->subqueryTipFlag
                                 & QMG_PROJ_SUBQUERY_TIP_MASK )
                               == QMG_PROJ_SUBQUERY_TIP_KEYRANGE )
                             || ( ( sPROJGraph->subqueryTipFlag
                                    & QMG_PROJ_SUBQUERY_TIP_MASK )
                                  == QMG_PROJ_SUBQUERY_TIP_IN_KEYRANGE ) )
                        {
                            // Nothing To Do
                        }
                        else
                        {
                            sIsIndexableValue = ID_FALSE;
                        }

                        break;
                    }
                }
                else
                {
                    // subquery node가 아닌 경우
                    // Nothing To Do
                }
            } // end of for()

            if ( sIsIndexableValue == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                break;
            }

            // 3. LIKE에 대한 패턴문자는 일반문자로 시작하여야 한다.
            if ( sCompareNode->node.module == &mtfLike )
            {
                // 일반문자로 시작하는 상수인지를 검사한다.
                if ( ( QC_SHARED_TMPLATE( aStatement )->
                       tmplate.rows[sValueNode->node.table].lflag
                       & MTC_TUPLE_TYPE_MASK)
                     == MTC_TUPLE_TYPE_CONSTANT)
                {
                    IDE_TEST( isIndexableLIKE( aStatement,
                                               sCompareNode,
                                               & sIsIndexableValue )
                              != IDE_SUCCESS );

                    if ( sIsIndexableValue == ID_TRUE )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    // BUG-25594
                    // dynamic constant expression이면 indexable로 판단한다.
                    if ( qtc::isConstNode4LikePattern( aStatement,
                                                       sValueNode,
                                                       aOuterDependencies )
                         == ID_TRUE )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        sIsIndexableValue = ID_FALSE;
                        break;
                    }
                }
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    if ( sIsIndexableValue == ID_TRUE )
    {
        *aIsIndexable = ID_TRUE;
    }
    else
    {
        *aIsIndexable = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isIndexableLIKE( qcStatement  * aStatement,
                          qtcNode      * aLikeNode,
                          idBool       * aIsIndexable )
{
/***********************************************************************
 *
 * Description : LIKE 연산자의 index 사용 가능 여부 판단
 *
 * Implementation :
 *
 *     LIKE의 패턴문자가 일반문자로 시작하는지를 검사한다.
 *     예) i1 like 'acd%' (O), i1 like '%de' (X), i1 like ? (O),
 *         i1 like '%%de' escape '%' (O), i1 like 'bc'||'d%' (O)
 *
 *     1. (1) value의 첫번째 문자가 %, _ 가 아니면, indexable로 판단.
 *        (2) value의 첫번째 문자가 %, _ 이면,
 *            escape 문자와 일치하는지를 검사해서,
 *            일치하면, indexable로 판단.
 *
 ***********************************************************************/

    const mtlModule * sLanguage;

    qcTemplate      * sTemplate;
    idBool            sIsIndexableLike = ID_TRUE;
    mtdEcharType    * sPatternEcharValue;
    mtdCharType     * sPatternValue;
    mtdCharType     * sEscapeValue;
    mtcColumn       * sColumn;
    void            * sValue;
    qtcNode         * sValueNode;
    qtcNode         * sEscapeNode;
    idBool            sIsEqual1;
    idBool            sIsEqual2;
    idBool            sIsEqual3;
    UChar             sSize;

    mtcColumn       * sEncColumn;
    UShort            sPatternLength;
    UChar           * sPattern;

    IDU_FIT_POINT_FATAL( "qmoPred::isIndexableLIKE::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aLikeNode != NULL );
    IDE_DASSERT( aIsIndexable != NULL );

    //--------------------------------------
    // LIKE 연산자에 대한 indexable 판단
    //--------------------------------------

    sTemplate   = QC_SHARED_TMPLATE(aStatement);
    sValueNode  = (qtcNode *)(aLikeNode->node.arguments->next);
    sEscapeNode = (qtcNode *)(sValueNode->node.next);

    // value node에서 패턴문자를 얻어온다.
    IDE_TEST( qtc::calculate( sValueNode, sTemplate )
              != IDE_SUCCESS );

    sColumn = sTemplate->tmplate.stack->column;
    sValue  = sTemplate->tmplate.stack->value;

    // To Fix PR-12999
    // Char가 Null 인 경우 sPatternEcharValue->value[0] 에는 아무 값도 존재하지 않음
    // 따라서, UMR을 방지하기 위해서는 Null 여부를 먼저 판단해야 함.
    if ( sColumn->module->isNull( sColumn, sValue )
         == ID_TRUE )
    {
        // Null 인 경우
        sIsIndexableLike = ID_FALSE;
    }
    else
    {
        sLanguage = sColumn->language;

        if ( (sColumn->module->id == MTD_ECHAR_ID) ||
             (sColumn->module->id == MTD_EVARCHAR_ID) )
        {
            // PROJ-2002 Column Security
            // pattern이 echar, evarchar인 경우

            sPatternEcharValue = (mtdEcharType*)sValue;

            //--------------------------------------------------
            // format string의 plain text를 얻는다.
            //--------------------------------------------------

            sEncColumn = QTC_STMT_COLUMN( aStatement, sValueNode );

            // 상수는 policy를 가질 수 없다.
            IDE_FT_ASSERT( sEncColumn->mColumnAttr.mEncAttr.mPolicy[0] == '\0' );

            sPattern       = sPatternEcharValue->mValue;
            sPatternLength = sPatternEcharValue->mCipherLength;
        }
        else
        {
            // pattern이 char, varchar, nchar, nvarchar의 경우

            sPatternValue = (mtdCharType*)sValue;

            sPattern       = sPatternValue->value;
            sPatternLength = sPatternValue->length;
        }

        sSize = mtl::getOneCharSize( sPattern,
                                     sPattern + sPatternLength,
                                     sLanguage );

        // '%' 문자인지 검사
        sIsEqual1 = mtc::compareOneChar( sPattern,
                                         sSize,
                                         sLanguage->specialCharSet[MTL_PC_IDX],
                                         sLanguage->specialCharSize );

        // '_' 문자인지 검사
        sIsEqual2 = mtc::compareOneChar( sPattern,
                                         sSize,
                                         sLanguage->specialCharSet[MTL_UB_IDX],
                                         sLanguage->specialCharSize );

        if ( ( sIsEqual1 != ID_TRUE ) && ( sIsEqual2 != ID_TRUE ) )
        {
            // 패턴문자의 첫번째 문자가 '%', '_'로 시작하지 않는다.
            // Nothing To Do
        }
        else
        {
            // 패턴문자의 첫번째 문자가 '%', '_'로 시작한다면,
            // escape 문자와 동일한지를 검사한다.
            if ( sEscapeNode != NULL )
            {
                IDE_TEST( qtc::calculate( sEscapeNode,
                                          sTemplate )
                          != IDE_SUCCESS );

                // escape 문자를 얻어온다.
                sEscapeValue = (mtdCharType*)sTemplate->tmplate.stack->value;

                sIsEqual3 = mtc::compareOneChar( sPattern,
                                                 sSize,
                                                 sEscapeValue->value,
                                                 sEscapeValue->length );

                if ( sIsEqual3 == ID_TRUE )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsIndexableLike = ID_FALSE;
                }
            }
            else  // escape 문자가 없는 경우
            {
                sIsIndexableLike = ID_FALSE;
            }
        }
    }

    if ( sIsIndexableLike == ID_TRUE )
    {
        *aIsIndexable = ID_TRUE;
    }
    else
    {
        *aIsIndexable = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::processIndexableInSubQ( qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 * Description : 재배치가 완료된 각 indexable column에 대한
 *               IN(subquery)에 대한 처리
 *
 * Implementation :
 *
 *     IN(subquery)는 다른 predicate과 함께 keyRange를 구성할 수 없고,
 *     단독으로만 keyRange를 구성해야하므로 predicate 분류가 끝나고 나면,
 *     indexable 컬럼에 대해 IN(subquery)가 존재하는 경우,
 *     다음과 같이 처리한다.
 *
 *     selectivity가 좋은 predicate을 찾는다.
 *     A. 찾은 predicate이 IN(subquery)이면,
 *        . 해당 컬럼리스트에 찾은 IN(subquery)만 남기고,
 *        . 나머지 predicate들은 non-indexable의 리스트에 달아준다.
 *     B. 찾은 predicate이 IN(subquery)가 아니면,
 *        . IN(subquery)들을 모두 찾아서 non-indexable의 리스트에 달아준다.
 *
 *     따라서, indexable column LIST에는 다음과 같은 두 가지의 경우가 생긴다.
 *             (1) IN(subquery) 하나만 존재
 *             (2) IN(subquery)가 없는 predicate들로만 구성
 *
 *     Example )
 *
 *
 *             [i1 = 1] ----> [i2 IN] ---> [N/A]
 *                |              |
 *             [i1 IN ]       [I2 = 1]
 *
 *                            ||
 *                            \/
 *
 *             [i1 = 1] ----> [i2 IN] ---> [N/A]
 *                                           |
 *                                         [i1 IN]
 *                                           |
 *                                         [i2 = 1]
 *
 ***********************************************************************/

    qmoPredicate * sPredicate;
    qmoPredicate * sNonIndexablePred;
    qmoPredicate * sLastNonIndexable;
    qmoPredicate * sCurPredicate;
    qmoPredicate * sNextPredicate;
    qmoPredicate * sMorePredicate;
    qmoPredicate * sMoreMorePred;
    qmoPredicate * sSelPredicate;   // selectivity가 좋은 predicate
    qmoPredicate * sTempPredicate = NULL;
    qmoPredicate * sLastTempPred  = NULL;
    qmoPredicate * sTemp = NULL;
    qmoPredicate * sLastTemp;

    IDU_FIT_POINT_FATAL( "qmoPred::processIndexableInSubQ::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aPredicate != NULL );

    //------------------------------------------
    // non-indexable 컬럼리스트를 찾는다.
    //------------------------------------------

    sPredicate = *aPredicate;

    for ( sCurPredicate = sPredicate;
          ( sCurPredicate != NULL ) &&
              ( sCurPredicate->id != QMO_COLUMNID_NON_INDEXABLE );
          sCurPredicate = sCurPredicate->next );

    if ( sCurPredicate == NULL )
    {
        // 인자로 넘어온 predicate 리스트에는
        // non-indexable predicate이 존재하지 않는 경우

        sNonIndexablePred = NULL;
    }
    else
    {
        // 인자로 넘어온 predicate 리스트에는
        // non-indexable predicate이 존재하는 경우
        sNonIndexablePred = sCurPredicate;

        for ( sLastNonIndexable        = sNonIndexablePred;
              sLastNonIndexable->more != NULL;
              sLastNonIndexable        = sLastNonIndexable->more ) ;
    }

    //------------------------------------------
    // 각 컬럼별로 IN(subquery)에 대한 처리
    //------------------------------------------

    for ( sCurPredicate  = sPredicate;
          sCurPredicate != NULL;
          sCurPredicate  = sNextPredicate )
    {
        //------------------------------------------
        // IN(subquery)가 존재하는지 확인.
        //------------------------------------------

        sTemp = NULL;

        sNextPredicate = sCurPredicate->next;

        if ( sCurPredicate->id == QMO_COLUMNID_NON_INDEXABLE )
        {
            sTemp = sCurPredicate;
        }
        else
        {
            // 컬럼리스트에 IN(subquery)를 포함한 predicate이 있는지 검사
            for ( sMorePredicate  = sCurPredicate;
                  sMorePredicate != NULL;
                  sMorePredicate  = sMorePredicate->more )
            {
                if ( ( sMorePredicate->flag & QMO_PRED_INSUBQUERY_MASK )
                     == QMO_PRED_INSUBQUERY_EXIST )
                {
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }

            if ( sMorePredicate == NULL )
            {
                // 컬럼리스트에 IN(subquery)를 포함한 predicate이 없는 경우
                sTemp = sCurPredicate;
            }
            else
            {
                // 컬럼리스트에 IN(subquery)를 포함한 predicate이 있는 경우

                //------------------------------------------
                // selectivity가 가장 좋은 predicate을 찾는다.
                //------------------------------------------

                sMorePredicate = sCurPredicate;
                sSelPredicate  = sMorePredicate;
                sMorePredicate = sMorePredicate->more;

                while ( sMorePredicate != NULL )
                {
                    if ( sSelPredicate->mySelectivity >
                         sMorePredicate->mySelectivity )
                    {
                        sSelPredicate = sMorePredicate;
                    }
                    else
                    {
                        // Nothing To Do
                    }

                    sMorePredicate = sMorePredicate->more;
                }

                if ( ( sSelPredicate->flag & QMO_PRED_INSUBQUERY_MASK )
                     == QMO_PRED_INSUBQUERY_EXIST )
                {
                    //------------------------------------------
                    // 찾은 predicate이 IN(subquery)이면,
                    // . 해당 컬럼리스트에  IN(subquery)만 남기고,
                    // . 나머지 predicate들은 non-indexable의 리스트에
                    //   달아준다.
                    //------------------------------------------

                    for ( sMorePredicate  = sCurPredicate;
                          sMorePredicate != NULL;
                          sMorePredicate  = sMoreMorePred )
                    {
                        sMoreMorePred = sMorePredicate->more;

                        if ( sSelPredicate == sMorePredicate )
                        {
                            sTemp = sMorePredicate;
                        }
                        else
                        {
                            // non-indexable 에 연결
                            // columnID 변경
                            sMorePredicate->id = QMO_COLUMNID_NON_INDEXABLE;

                            if ( ( sMorePredicate->node->lflag &
                                  QTC_NODE_SUBQUERY_MASK )
                                  == QTC_NODE_SUBQUERY_EXIST )
                            {
                                // subqueryKeyRange 최적화 팁을 삭제한다.
                                IDE_TEST( removeIndexableSubQTip( sMorePredicate->node ) != IDE_SUCCESS );
                            }
                            else
                            {
                                // Nothing To Do
                            }

                            if ( sNonIndexablePred == NULL )
                            {
                                sNonIndexablePred = sMorePredicate;
                                sLastNonIndexable = sNonIndexablePred;

                                // 인자로 넘어온 predicate 리스트에는
                                // non-indexable predicate이 존재하지 않는 경우
                                // predicate list에 연결되어야 한다.
                                if ( sTempPredicate == NULL )
                                {
                                    sTempPredicate = sNonIndexablePred;
                                    sLastTempPred = sTempPredicate;
                                }
                                else
                                {
                                    sLastTempPred->next = sNonIndexablePred;
                                    sLastTempPred = sLastTempPred->next;
                                }
                            }
                            else
                            {
                                sLastNonIndexable->more = sMorePredicate;
                                sLastNonIndexable = sLastNonIndexable->more;
                            }
                        }

                        sMorePredicate->more = NULL;
                    }
                } // selectivity가 좋은 predicate이 IN(subquery)인 경우의 처리
                else
                {
                    //------------------------------------------
                    // 찾은 predicate이 IN(subquery)가 아니면,
                    // . IN(subquery)들을 모두 찾아서
                    //   non-indexable의 리스트에 달아준다.
                    //------------------------------------------
                    for ( sMorePredicate  = sCurPredicate;
                          sMorePredicate != NULL;
                          sMorePredicate  = sMoreMorePred )
                    {
                        sMoreMorePred = sMorePredicate->more;

                        if ( ( sMorePredicate->flag
                               & QMO_PRED_INSUBQUERY_MASK )
                             != QMO_PRED_INSUBQUERY_EXIST )
                        {
                            if ( sTemp == NULL )
                            {
                                sTemp = sMorePredicate;
                                sLastTemp = sTemp;
                            }
                            else
                            {
                                sLastTemp->more = sMorePredicate;
                                sLastTemp = sLastTemp->more;
                            }
                        }
                        else
                        {
                            // non-indexable 에 연결
                            // IN(subquery)가 filter로 사용되므로,
                            // IN(subquery) 최적화 팁 flag를 삭제한다.
                            IDE_TEST( removeIndexableSubQTip( sMorePredicate->node ) != IDE_SUCCESS );
                            // columnID 변경
                            sMorePredicate->id = QMO_COLUMNID_NON_INDEXABLE;

                            if ( sNonIndexablePred == NULL )
                            {
                                sNonIndexablePred = sMorePredicate;
                                sLastNonIndexable = sNonIndexablePred;

                                // 인자로 넘어온 predicate 리스트에는
                                // non-indexable predicate이 존재하지 않는 경우
                                // predicate list에 연결되어야 한다.
                                if ( sTempPredicate == NULL )
                                {
                                    sTempPredicate = sNonIndexablePred;
                                    sLastTempPred = sTempPredicate;
                                }
                                else
                                {
                                    sLastTempPred->next = sNonIndexablePred;
                                    sLastTempPred = sLastTempPred->next;
                                }
                            }
                            else
                            {
                                sLastNonIndexable->more = sMorePredicate;
                                sLastNonIndexable = sLastNonIndexable->more;
                            }
                        }

                        sMorePredicate->more = NULL;
                    }
                }
            }
        }

        sCurPredicate->next = NULL;

        if ( sTempPredicate == NULL )
        {
            sTempPredicate = sTemp;
            sLastTempPred  = sTempPredicate;
        }
        else
        {
            sLastTempPred->next = sTemp;
            sLastTempPred       = sLastTempPred->next;
        }
    }

    *aPredicate = sTempPredicate;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isJoinablePredicate( qmoPredicate * aPredicate,
                              qcDepInfo    * aFromDependencies,
                              qcDepInfo    * aLeftChildDependencies,
                              qcDepInfo    * aRightChildDependencies,
                              idBool       * aIsOnlyIndexNestedLoop   )
{
/***********************************************************************
 *
 * Description : joinable predicate의 판단
 *
 * [joinable predicate]: predicate의 하위 노드가
 *                       JOIN graph의 child graph에 하나씩 대응되어야 함.
 *     예) t1.i1 = t2.i1 (O),  t1.i1+1 = t2.i1+1 (O),
 *         t1.i1=t2.i1 OR t1.i1=t2.i2 (O) : only index nested loop join
 *         t1.i1=1 OR t2.i1=3 (X) : join predicate이지만, non-joinable.
 *
 * Implementation :
 *
 *    joinable predicate을 판단해서, qmoPredicate.flag에 그 정보 설정.
 *
 *    OR 하위 노드가 2개이상인 경우는, 모두 joinable predicate이어야 한다.
 *     1. joinable predicate으로 사용될수 있는 비교연산자 판단.
 *        [ =, >, >=, <, <= ]
 *     2. 1의 조건이 만족되면,
 *        비교연산자 하위 두개 노드의 dependencies 정보로
 *        하나의 노드가 left child의 컬럼이라면,
 *        나머지 하나의 노드는 right child의 컬럼인지를 검사한다.
 *
 * [ aIsOnlyIndexNestedLoop 은 t1.i1=t2.i1 OR t1.i1=t2.i2와 같은 경우에
 *   true로 설정된다. ]
 *
 ***********************************************************************/

    idBool    sIsJoinable = ID_TRUE;
    idBool    sIsOnlyIndexNestedLoopPred = ID_FALSE;
    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoPred::isJoinablePredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aLeftChildDependencies != NULL );
    IDE_DASSERT( aRightChildDependencies != NULL );
    IDE_DASSERT( aIsOnlyIndexNestedLoop != NULL );

    //--------------------------------------
    // joinable predicate의 판단
    //--------------------------------------

    sNode = aPredicate->node;

    if ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_OR )
    {
        //-------------------------------------
        // CNF 인 경우로,
        // 인자로 넘어온 predicate의 최상위 노드는 OR노드이며,
        // OR 노드 하위에 여러개의 join predicate이 올 수 있다.
        // OR 노드 하위에 여러개의 join predicate이 오는 경우,
        // 모든 predicate이 joinable이어야 하며,
        // index nested loop join method만 사용할 수 있다.
        //--------------------------------------

        // sNode는 비교연산자 노드
        sNode = (qtcNode *)(sNode->node.arguments);

        if ( aPredicate->node->node.arguments->next == NULL )
        {
            // 1. OR 노드 하위에 비교연산자가 하나만 오는 경우
            //    joinable predicate인지만 판단하면 된다.
            IDE_TEST( isJoinableOnePred( sNode,
                                         aFromDependencies,
                                         aLeftChildDependencies,
                                         aRightChildDependencies,
                                         & sIsJoinable )
                      != IDE_SUCCESS );
        }
        else
        {
            // 2. OR 노드 하위에 비교연산자가 여러개 오는 경우
            //    하위 노드가 모두 joinable predicate이어야 한다.
            //    이 경우는, index nested loop join method만 사용할 수 있다.

            sIsOnlyIndexNestedLoopPred = ID_TRUE;

            while ( sNode != NULL )
            {
                IDE_TEST( isJoinableOnePred( sNode,
                                             aFromDependencies,
                                             aLeftChildDependencies,
                                             aRightChildDependencies,
                                             & sIsJoinable )
                          != IDE_SUCCESS );

                if ( sIsJoinable == ID_TRUE )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsOnlyIndexNestedLoopPred = ID_FALSE;
                    break;
                }

                sNode = (qtcNode *)(sNode->node.next);
            }
        }
    }
    else
    {
        //-------------------------------------
        // DNF인 경우로,
        // 인자로 넘어온 predicate의 최상위 노드는 비교연산자 노드이다.
        // 아직 predicate의 연결관계가 끊어지지 않은 상태이므로,
        // 인자로 넘어온 비교연산자 노드 하나에 대해서만 처리해야 한다.
        //--------------------------------------

        IDE_TEST( isJoinableOnePred( sNode,
                                     aFromDependencies,
                                     aLeftChildDependencies,
                                     aRightChildDependencies,
                                     & sIsJoinable )
                  != IDE_SUCCESS );
    }

    //-------------------------------------
    // qmoPredicate.flag에 joinable predicate 정보를 저장한다.
    //-------------------------------------

    if ( sIsJoinable == ID_TRUE )
    {
        aPredicate->flag &= ~QMO_PRED_JOINABLE_PRED_MASK;
        aPredicate->flag |= QMO_PRED_JOINABLE_PRED_TRUE;
    }
    else
    {
        aPredicate->flag &= ~QMO_PRED_JOINABLE_PRED_MASK;
        aPredicate->flag |= QMO_PRED_JOINABLE_PRED_FALSE;
    }

    *aIsOnlyIndexNestedLoop = sIsOnlyIndexNestedLoopPred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isJoinableOnePred( qtcNode     * aNode,
                            qcDepInfo   * aFromDependencies,
                            qcDepInfo   * aLeftChildDependencies,
                            qcDepInfo   * aRightChildDependencies,
                            idBool      * aIsJoinable )
{
/***********************************************************************
 *
 * Description : 하나의 비교연산자에 대한 joinable predicate 판단
 *
 *     qmoPred::isJoinablePredicate() 함수 주석 참조.
 *
 * Implementation :
 *
 *  비교연산자의 각 하위노드가
 *  left/right child graph에 하나씩 대응되는지 검사.
 *
 *  1. 비교연산자 각 하위노드에 대하여, 해당 table의 dependencies를 구한다.
 *     nodeDependencies = 노드의 dependencies & FROM의 dependencies
 *  2. 노드가 left/right graph에 속하는 컬럼인지 검사
 *     orDependencies = nodeDependencies | left/right graph의 dependencies
 *  3. orDependencies가 left/right graph의 dependencies와 동일하면,
 *     해당 노드는 left/right graph에 속하는 컬럼이라고 판단한다.
 *
 ***********************************************************************/

    qcDepInfo sFirstNodeDependencies;
    qcDepInfo sSecondNodeDependencies;
    qcDepInfo sLeftOrDependencies;
    qcDepInfo sRightOrDependencies;
    qcDepInfo sOrDependencies;
    idBool    sIsTemp = ID_TRUE;
    idBool    sIsJoinablePred = ID_TRUE;
    qtcNode * sCompareNode;
    qtcNode * sFirstNode;
    qtcNode * sSecondNode;

    IDU_FIT_POINT_FATAL( "qmoPred::isJoinableOnePred::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aIsJoinable != NULL );

    //--------------------------------------
    // joinable 판단
    //--------------------------------------

    sCompareNode = aNode;

    while ( sIsTemp == ID_TRUE )
    {
        sIsTemp = ID_FALSE;

        // fix BUG-12282
        //--------------------------------------
        // subquery를 포함하지 않아야 함.
        //--------------------------------------
        if ( ( sCompareNode->lflag & QTC_NODE_SUBQUERY_MASK )
             == QTC_NODE_SUBQUERY_EXIST )
        {
            sIsJoinablePred  = ID_FALSE;
            break;
        }
        else
        {
            // Nothing To Do
        }

        //--------------------------------------
        // joinable predicate으로 사용될수 있는 비교연산자 판단.
        // [ =, >, >=, <, <= ]
        //--------------------------------------
        // To Fix PR-15434
        if ( ( sCompareNode->node.module->lflag & MTC_NODE_INDEX_JOINABLE_MASK )
             == MTC_NODE_INDEX_JOINABLE_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            sIsJoinablePred = ID_FALSE;
            break;
        }

        //--------------------------------------
        // 비교연산자 하위 두개 노드의 dependency 정보로
        // 두개의 하위 노드가 join graph의 left/right child에
        // 하나씩 대응되는지 검사한다.
        //--------------------------------------

        sFirstNode = (qtcNode *)(sCompareNode->node.arguments);
        sSecondNode = (qtcNode *)(sCompareNode->node.arguments->next);

        //--------------------------------------
        // 비교 연산자 각 하위 노드들의 해당 table의 dependencies를 구한다.
        // (node의 dependencies) & FROM의 dependencies
        //--------------------------------------

        qtc::dependencyAnd( & sFirstNode->depInfo,
                            aFromDependencies,
                            & sFirstNodeDependencies );

        qtc::dependencyAnd( & sSecondNode->depInfo,
                            aFromDependencies,
                            & sSecondNodeDependencies );

        //--------------------------------------
        // 비교연산자 노드의 argument에 대해서,
        // left child, right child의 dependencies와 oring
        //--------------------------------------

        IDE_TEST( qtc::dependencyOr( & sFirstNodeDependencies,
                                     aLeftChildDependencies,
                                     & sLeftOrDependencies )
                  != IDE_SUCCESS );


        IDE_TEST( qtc::dependencyOr( & sFirstNodeDependencies,
                                     aRightChildDependencies,
                                     & sRightOrDependencies )
                  != IDE_SUCCESS );

        if ( qtc::dependencyEqual( & sLeftOrDependencies,
                                   aLeftChildDependencies ) == ID_TRUE )
        {
            //--------------------------------------
            // 비교연산자의 argument 노드가 left child graph에 속하는 컬럼
            //--------------------------------------

            IDE_TEST( qtc::dependencyOr( & sSecondNodeDependencies,
                                         aRightChildDependencies,
                                         & sOrDependencies )
                      != IDE_SUCCESS );


            if ( qtc::dependencyEqual( & sOrDependencies,
                                       aRightChildDependencies ) == ID_TRUE )
            {
                //--------------------------------------
                // 비교연산자의 argument->next 노드가
                // right child graph에 속하는 컬럼
                //--------------------------------------

                //sCompareNode->node.arguments가 left child graph에 포함되고,
                //sCompareNode->node.arguemtns->next가 right child graph에
                // 포함하므로, joinable predicate으로 판단

                // Nothing To Do
            }
            else
            {
                // non-joinable predicate
                sIsJoinablePred = ID_FALSE;
                break;
            }
        }
        else if ( qtc::dependencyEqual( & sRightOrDependencies,
                                        aRightChildDependencies ) == ID_TRUE )
        {
            //--------------------------------------
            // 비교연산자의 argument 노드가 right child graph에 속하는 컬럼
            //--------------------------------------

            IDE_TEST( qtc::dependencyOr( & sSecondNodeDependencies,
                                         aLeftChildDependencies,
                                         & sOrDependencies )
                      != IDE_SUCCESS );

            if ( qtc::dependencyEqual( & sOrDependencies,
                                       aLeftChildDependencies ) == ID_TRUE )
            {
                //--------------------------------------
                // 비교연산자의 argument->next 노드가
                // left child graph에 속하는 컬럼
                //--------------------------------------

                //sCompareNode->node.arguments가 right child graph에 포함되고,
                //sCompareNode->node.arguments->next가 left child graph에
                //포함되므로, joinable predicate으로 판단
                // Nothing To Do
            }
            else
            {
                // non-joinable predicate
                sIsJoinablePred = ID_FALSE;
                break;
            }
        }
        else
        {
            // non-joinable predicate
            sIsJoinablePred = ID_FALSE;
            break;
        }
    }

    if ( sIsJoinablePred == ID_TRUE )
    {
        *aIsJoinable = ID_TRUE;
    }
    else
    {
        *aIsJoinable = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isIndexableJoinPredicate( qcStatement  * aStatement,
                                   qmoPredicate * aPredicate,
                                   qmgGraph     * aLeftChildGraph,
                                   qmgGraph     * aRightChildGraph )
{
/***********************************************************************
 *
 * Description : indexable join predicate의 판단
 *
 *     < indexable join predicate의 정의 >
 *     indexable join predicate은 one table predicate과 거의 동일하며,
 *     차이점은 다음과 같다.
 *
 *     조건0.
 *            0-1. right child graph가 qmgSelection graph이고,
 *            0-2. joinable predicate이어야 하고,
 *
 *     조건1. joinable operator는 [=, >, >=, <, <=]로 제한적이고,
 *            [ joinable predicate검사에서 이미 걸러짐. ]
 *
 *     조건6. subquery를 포함하지 않는다.
 *
 * Implementation :
 *
 *     one table predicate과 차이나는 조건들을 미리 걸러내고,
 *     one table predicate과 동일한 isIndexable()함수를 사용한다.
 *
 *     인자로 넘어온 predicate은 joinable operator의 joinable predicate이므로,
 *     subquery존재여부와 right child graph가 qmgSelection 인지만 확인한다.
 *
 *     1. subquery가 존재하는지 검사.
 *
 *     2. left->right로의 검사.
 *        right child graph가 qmgSelection graph이고,
 *        indexable predicate이면,
 *
 *     3. right->left로의 검사.
 *        left child graph가 qmgSelection graph이고,
 *        indexable predicate이면,
 *
 *     indexable predicate으로 판단하고, index nested loop join과
 *     각각의 수행가능한 join방향을 qmoPredicate.flag에 설정한다.
 *
 ***********************************************************************/

    idBool     sIsIndexableJoinPred = ID_TRUE;
    qmgGraph * sRightGraph;

    IDU_FIT_POINT_FATAL( "qmoPred::isIndexableJoinPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate != NULL );

    //--------------------------------------
    // indexable join predicate의 판단
    //--------------------------------------

    aPredicate->flag &= ~QMO_PRED_INDEX_JOINABLE_MASK;
    aPredicate->flag &= ~QMO_PRED_INDEX_DIRECTION_MASK;

    //--------------------------------------
    // subquery가 존재하지 않아야 한다.
    // joinable predicate검사시, 이미 걸러짐.
    //--------------------------------------

    //--------------------------------------
    // join 방향을 left child -> right child로 수행시,
    // index joinable predicate으로 사용가능한지 검사.
    //--------------------------------------

    sRightGraph = aRightChildGraph;

    // PROJ-1502 PARTITIONED DISK TABLE
    // right child graph가 selection 또는 partition graph인지 검사.
    if ( ( sRightGraph->type == QMG_SELECTION ) ||
         ( sRightGraph->type == QMG_PARTITION ) )
    {
        IDE_TEST( isIndexable( aStatement,
                               aPredicate,
                               & sRightGraph->depInfo,
                               & qtc::zeroDependencies,
                               & sIsIndexableJoinPred )
                  != IDE_SUCCESS );

        // qmoPredicate.flag에
        // left->right로의 수행 가능 정보를 저장한다.
        if ( sIsIndexableJoinPred == ID_TRUE )
        {
            aPredicate->flag |= QMO_PRED_INDEX_JOINABLE_TRUE;
            aPredicate->flag |= QMO_PRED_INDEX_LEFT_RIGHT;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    //--------------------------------------
    // join 방향을 right child -> left child로 수행시,
    // index joinable predicate으로 사용가능한지 검사.
    //--------------------------------------

    sRightGraph = aLeftChildGraph;

    // PROJ-1502 PARTITIONED DISK TABLE
    // left child graph가 selection graph또는 partition graph인지 검사.
    if ( ( sRightGraph->type == QMG_SELECTION ) ||
         ( sRightGraph->type == QMG_PARTITION ) )
    {
        IDE_TEST( isIndexable( aStatement,
                               aPredicate,
                               & sRightGraph->depInfo,
                               & qtc::zeroDependencies,
                               & sIsIndexableJoinPred )
                  != IDE_SUCCESS );

        // qmoPredicate.flag에
        // right->left로의 수행 가능 정보를 저장한다.
        if ( sIsIndexableJoinPred == ID_TRUE )
        {
            aPredicate->flag |= QMO_PRED_INDEX_JOINABLE_TRUE;
            aPredicate->flag |= QMO_PRED_INDEX_RIGHT_LEFT;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isSortJoinablePredicate( qmoPredicate * aPredicate,
                                  qtcNode      * aNode,
                                  qcDepInfo    * aFromDependencies,
                                  qmgGraph     * aLeftChildGraph,
                                  qmgGraph     * aRightChildGraph )
{
/***********************************************************************
 *
 * Description : sort joinable predicate의 판단 ( key size limit 검사 )
 *
 * Implementation :
 *
 *     disk sort temp table을 사용해야 하는 경우,
 *     index 생성 가능한 컬럼인지에 대한 검사를 수행하고,
 *     index 생성 가능 컬럼이면, sort joinable predicate으로 판단한다.
 *
 *     1. index 생성 가능 컬럼판단
 *     2. sort column이 index 생성 가능 컬럼이면,
 *        qmoPredicate.flag에 sort join method 사용가능함을 저장.
 *
 *     TODO :
 *     host 변수를 포함하는 경우
 *     key size limit 검사를 execution time에 판단해야 한다.
 *
 ***********************************************************************/

    idBool      sIsSortJoinablePred = ID_TRUE;
    qtcNode   * sCompareNode;
    qmgGraph  * sLeftSortColumnGraph  = NULL;
    qmgGraph  * sRightSortColumnGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::isSortJoinablePredicate::__FT__" );

    //--------------------------------------
    // sort joinable 판단
    //--------------------------------------

    // sCompareNode는 비교연산자 노드
    sCompareNode = aNode;

    //--------------------------------------
    // BUG-15849
    // equal류 연산이지만, sort join을 사용할 수 없는 경우가 있다.
    // 예) stfContains, stfCrosses, stfEquals, stfIntersects,
    //     stfOverlaps, stfTouches, stfWithin
    //--------------------------------------
    if ( ( sCompareNode->node.module == &mtfEqual )
         || ( sCompareNode->node.module == &mtfGreaterThan )
         || ( sCompareNode->node.module == &mtfGreaterEqual )
         || ( sCompareNode->node.module == &mtfLessThan )
         || ( sCompareNode->node.module == &mtfLessEqual ) )
    {
        // 비교연산자가 equal류인 경우,

        if ( ( ( sCompareNode->node.arguments->lflag &
                 MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_LIST ) ||
             ( ( sCompareNode->node.arguments->next->lflag &
                 MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_LIST ) )
        {
            // BUGBUG: List type이 join key인 경우 처리 필요
            sIsSortJoinablePred = ID_FALSE;
        }
        else
        {
            // 비교연산자 각 하위 노드에 해당하는 graph를 찾는다.
            IDE_TEST( findChildGraph( sCompareNode,
                                      aFromDependencies,
                                      aLeftChildGraph,
                                      aRightChildGraph,
                                      &sLeftSortColumnGraph,
                                      & sRightSortColumnGraph )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // 비교연산자가 equal류가 아닌 경우,

        sIsSortJoinablePred = ID_FALSE;
    }

    if ( sIsSortJoinablePred == ID_TRUE )
    {
        aPredicate->flag &= ~QMO_PRED_SORT_JOINABLE_MASK;
        aPredicate->flag |= QMO_PRED_SORT_JOINABLE_TRUE;
    }
    else
    {
        aPredicate->flag &= ~QMO_PRED_SORT_JOINABLE_MASK;
        aPredicate->flag |= QMO_PRED_SORT_JOINABLE_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isHashJoinablePredicate( qmoPredicate * aPredicate,
                                  qtcNode      * aNode,
                                  qcDepInfo    * aFromDependencies,
                                  qmgGraph     * aLeftChildGraph,
                                  qmgGraph     * aRightChildGraph )
{
/***********************************************************************
 *
 * Description : Hash joinable predicate의 판단 ( key size limit 검사 )
 *
 * Implementation :
 *
 *     disk hash temp table을 사용해야 하는 경우,
 *     index 생성 가능한 컬럼인지에 대한 검사를 수행하고,
 *     index 생성 가능 컬럼이면, hash joinable predicate으로 판단한다.
 *
 *     1. hash column의 index 생성 가능 컬럼 판단
 *     2. hash column이 index 생성 가능 컬럼이면,
 *        qmoPredicate.flag에 hash join method 사용가능함을 설정.
 *
 *     TODO :
 *     (1) host 변수를 포함하는 경우
 *         key size limit 검사를 execution time에 판단해야 한다.
 *     (2) hash key column이 여러개인 경우
 *
 ***********************************************************************/

    idBool      sIsHashJoinablePred = ID_TRUE;
    qtcNode   * sCompareNode;
    qmgGraph  * sLeftHashColumnGraph  = NULL;
    qmgGraph  * sRightHashColumnGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::isHashJoinablePredicate::__FT__" );

    //--------------------------------------
    // Hash joinable 판단
    //--------------------------------------

    // TODO :
    // 이 로직에서는 hash column이 하나인 경우에 대해서만
    // index 생성가능 여부를 검사하는데,
    // hash column이 여러개 사용될 경우,
    // index 생성을 할 수 없을 경우가 생길 수 있다.

    // sCompareNode는 비교연산자 노드
    sCompareNode = aNode;

    if ( sCompareNode->node.module == &mtfEqual )
    {
        // 비교연산자가 equal인 경우,

        if ( ( ( sCompareNode->node.arguments->lflag &
                 MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_LIST ) ||
             ( ( sCompareNode->node.arguments->next->lflag &
                 MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_LIST ) )
        {
            // BUGBUG: List type이 join key인 경우 처리 필요
            sIsHashJoinablePred = ID_FALSE;
        }
        else
        {
            // 비교연산자 각 하위 노드에 해당하는 graph를 찾는다.
            IDE_TEST( findChildGraph( sCompareNode,
                                      aFromDependencies,
                                      aLeftChildGraph,
                                      aRightChildGraph,
                                      & sLeftHashColumnGraph,
                                      & sRightHashColumnGraph )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // 비교연산자가 equal이 아닌 경우,

        sIsHashJoinablePred = ID_FALSE;
    }

    if ( sIsHashJoinablePred == ID_TRUE )
    {
        aPredicate->flag &= ~QMO_PRED_HASH_JOINABLE_MASK;
        aPredicate->flag |= QMO_PRED_HASH_JOINABLE_TRUE;
    }
    else
    {
        aPredicate->flag &= ~QMO_PRED_HASH_JOINABLE_MASK;
        aPredicate->flag |= QMO_PRED_HASH_JOINABLE_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::isMergeJoinablePredicate( qmoPredicate * aPredicate,
                                   qtcNode      * aNode,
                                   qcDepInfo    * aFromDependencies,
                                   qmgGraph     * aLeftChildGraph,
                                   qmgGraph     * aRightChildGraph )
{
/***********************************************************************
 *
 * Description : merge joinable predicate의 판단 (방향성 검사)
 *
 * Implementation :
 *
 *      [ 인덱스 생성 가능 여부 검사 ]
 *      sort joinable 판단시와 동일하다.
 *      따라서, 이미 sort joinable 판단시 인덱스 생성 가능 여부가
 *      판단되었으므로, QMO_PRED_SORT_JOINABLE_TRUE이면,
 *      merge join method 사용 가능과 수행방향에 대한 정보를 설정한다.
 *
 *      merge join은 [=, >, >=, <, <=]비교연산자에 대해 수행가능하며,
 *      각 연산자에 대한 방향성 검사를 수행한다.
 *
 *      1) Semi/anti join의 경우
 *        연산자에 관계 없이 left->right만 허용
 *      2) Inner join의 경우
 *        1. equal(=) 비교연산자
 *           left->right, right->left 모두 수행가능
 *
 *        2. [ >, >=, <, <= ] 비교연산자
 *           (1) [ >, >= ] : left->right로 수행가능
 *           (2) [ <, <= ] : right->left로 수행가능
 *
 ***********************************************************************/

    qtcNode   * sCompareNode;
    qmgGraph  * sLeftMergeColumnGraph  = NULL;
    qmgGraph  * sRightMergeColumnGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::isMergeJoinablePredicate::__FT__" );

    //--------------------------------------
    // merge joinable 판단
    //--------------------------------------

    //--------------------------------------
    // merge join의 인덱스 생성가능 여부는 sort joinable 판단시와 동일하다.
    // 따라서, QMO_PRED_SORT_JOINABLE_TRUE 이면,
    // merge join 에서도 인덱스 생성 가능한 경우이므로,
    // merge join method 사용가능여부와 수행 방향 정보를 판단한다.
    //--------------------------------------

    if ( ( aPredicate->flag & QMO_PRED_SORT_JOINABLE_MASK )
         == QMO_PRED_SORT_JOINABLE_TRUE )
    {
        //-------------------------------------
        // 인덱스 생성가능한 경우로, merge join에 대한 정보를 설정.
        //-------------------------------------

        aPredicate->flag &= ~QMO_PRED_MERGE_JOINABLE_MASK;
        aPredicate->flag |= QMO_PRED_MERGE_JOINABLE_TRUE;

        sCompareNode = aNode;

        if ( sCompareNode->node.module == &mtfEqual )
        {
            // equal 비교연산자는 left->right, right->left 모두 수행가능하다.
            aPredicate->flag &= ~QMO_PRED_MERGE_DIRECTION_MASK;
            aPredicate->flag |= QMO_PRED_MERGE_LEFT_RIGHT;
            aPredicate->flag |= QMO_PRED_MERGE_RIGHT_LEFT;
        }
        else
        {
            // [ >, >=, <, <= ] 비교연산자
            // . [ >, >= ] : left->right로 수행가능
            // . [ <, <= ] : right->left로 수행가능

            // 비교연산자 각 하위 노드에 해당하는 graph를 찾는다.
            IDE_TEST( findChildGraph( sCompareNode,
                                      aFromDependencies,
                                      aLeftChildGraph,
                                      aRightChildGraph,
                                      & sLeftMergeColumnGraph,
                                      & sRightMergeColumnGraph )
                      != IDE_SUCCESS );

            if ( ( sCompareNode->node.module == &mtfGreaterThan )
                 || ( sCompareNode->node.module == &mtfGreaterEqual ) )
            {
                // >, >=

                if ( sLeftMergeColumnGraph == aLeftChildGraph )
                {
                    // 예:    [join]     ,         [>]
                    //          |                   |
                    //      [T1]   [T2]     [t1.i1]   [t2.i1]
                    // merge join 수행방향은 left->right
                    aPredicate->flag &= ~QMO_PRED_MERGE_DIRECTION_MASK;
                    aPredicate->flag |= QMO_PRED_MERGE_LEFT_RIGHT;
                }
                else
                {
                    // 예:    [join]     ,         [>]
                    //          |                   |
                    //      [T1]   [T2]     [t2.i1]   [t1.i1]
                    // merge join 수행방향은 right->left
                    aPredicate->flag &= ~QMO_PRED_MERGE_DIRECTION_MASK;
                    aPredicate->flag |= QMO_PRED_MERGE_RIGHT_LEFT;
                }
            }
            else
            {
                // <, <=

                if ( sLeftMergeColumnGraph == aLeftChildGraph )
                {
                    // 예:    [join]     ,         [<]
                    //          |                   |
                    //      [T1]   [T2]     [t1.i1]   [t2.i1]
                    // merge join 수행방향은 right->left
                    aPredicate->flag &= ~QMO_PRED_MERGE_DIRECTION_MASK;
                    aPredicate->flag |= QMO_PRED_MERGE_RIGHT_LEFT;
                }
                else
                {
                    // 예:    [join]     ,         [<]
                    //          |                   |
                    //      [T1]   [T2]     [t2.i1]   [t1.i1]
                    // merge join 수행방향은 left->right
                    aPredicate->flag &= ~QMO_PRED_MERGE_DIRECTION_MASK;
                    aPredicate->flag |= QMO_PRED_MERGE_LEFT_RIGHT;
                }
            }
        } // 비교연산자가 [ >, >=, <, <= ]인 경우
    } // QMO_PRED_SORT_JOINABLE_TRUE인 경우(즉, 인덱스 생성가능한 경우)
    else
    {
        //-------------------------------------
        // QMO_PRED_SORT_JOINABLE_FALSE인 경우(즉, 인덱스 생성 불가능한 경우)
        //-------------------------------------

        aPredicate->flag &= ~QMO_PRED_MERGE_JOINABLE_MASK;
        aPredicate->flag |= QMO_PRED_MERGE_JOINABLE_FALSE;

        aPredicate->flag &= ~QMO_PRED_MERGE_DIRECTION_MASK;
        aPredicate->flag |= QMO_PRED_MERGE_NON_DIRECTION;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::extractKeyRange( qcmIndex        * aIndex,
                          qmoPredWrapper ** aSource,
                          qmoPredWrapper ** aKeyRange,
                          qmoPredWrapper ** aKeyFilter,
                          qmoPredWrapper ** aSubqueryFilter )
{
/***********************************************************************
 *
 * Description : keyRange 추출
 *
 *     index nested loop join predicate이 존재할 경우,
 *     index nested loop join predicate이 keyRange로 선택되도록 한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoPred::extractKeyRange::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aSource != NULL );
    IDE_DASSERT( aKeyRange != NULL );
    IDE_DASSERT( aKeyFilter != NULL );
    IDE_DASSERT( aSubqueryFilter != NULL );

    //--------------------------------------
    // keyRange 추출
    //--------------------------------------

    if ( *aSource != NULL )
    {
        if ( ( (*aSource)->pred->flag & QMO_PRED_INDEX_NESTED_JOINABLE_MASK )
             == QMO_PRED_INDEX_NESTED_JOINABLE_TRUE )
        {
            //--------------------------------------
            // index nested loop join predicate이 존재하는 경우,
            // LIST 처리시 keyRange로 분류된 predicate이 있다면,
            // (1) IN subquery인 경우, subquery filter에 연결
            // (2) IN subquery가 아닌 경우, keyFilter에 연결하고
            // index nested loop join predicate이 keyRange로
            // 선택되도록 한다.
            //--------------------------------------

            if ( *aKeyRange == NULL )
            {
                // Nothing To Do
            }
            else
            {
                if ( ( (*aKeyRange)->pred->flag & QMO_PRED_INSUBQUERY_MASK )
                     == QMO_PRED_INSUBQUERY_EXIST )
                {
                    IDE_TEST( qmoPredWrapperI::moveAll( aKeyRange,
                                                        aSubqueryFilter )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( qmoPredWrapperI::moveAll( aKeyRange,
                                                        aKeyFilter )
                              != IDE_SUCCESS );
                }
            }

            IDE_TEST( extractKeyRange4Column( aIndex,
                                              aSource,
                                              aKeyRange )
                      != IDE_SUCCESS );
        }
        else
        {
            // index nested loop join predicate이 존재하지 않는 경우,

            //--------------------------------------
            // LIST 처리시, keyRange로 분류된 predicate이
            // (1) 존재하면, one column에 대한 keyRange는 추출하지 않고,
            // (2) 존재하지 않으면, one column에 대한 keyRange를 추출한다.
            //--------------------------------------

            if ( *aKeyRange == NULL )
            {
                //--------------------------------------
                // one column에 대한 keyRange 추출
                //--------------------------------------
                IDE_TEST( extractKeyRange4Column( aIndex,
                                                  aSource,
                                                  aKeyRange )
                          != IDE_SUCCESS );
            }
            else
            {
                // LIST 컬럼에 대한 keyRange가 있는 경우,
                // one column에 대한 keyRange 추출은 skip

                // Nothing To Do
            }
        }
    }
    else
    {
        // source가 없으므로 할게 없다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::extractKeyRange4Column( qcmIndex        * aIndex,
                                 qmoPredWrapper ** aSource,
                                 qmoPredWrapper ** aKeyRange )
{
/***********************************************************************
 *
 * Description : one column에 대한 keyRange를 추출한다.
 *
 *  [join index 최적화에 의하여 join predicate을 우선 적용.(BUG-7098)]
 *   join index 최적화에 의해 index nested joinable predicate이 있으면,
 *   이 join predicate이 one table predicate을 우선하여 keyRange로 추출해야
 *   한다. 현재 predicate 배치상 index joinable predicate이 다른 predicate보다
 *   앞에 위치해 있고, 인덱스는 joinable predicate의 column을 연속적으로
 *   사용할 수 있는 것이므로, 자연스럽게 joinable predicate이 우선적으로
 *   처리될 수 있으므로, 이에 대한 별도의 검사를 하지 않아도 된다.
 *
 *  [동일 컬럼의 컬럼리스트는 다음과 같이 구성되어 있다.]
 *   . IN(subquery) 하나만 존재
 *   . IN(subquery)를 포함하지 않는 predicate들의 연결
 *  이유는, IN(subquery)는 오직 predicate하나에 대해서만 keyRange를 구성할 수
 *  있으므로, keyRange 추출을 용이하게 하기 위해, predicate재배치가 끝나면,
 *  selectivity가 좋은 predicate을 기준으로 이와 같이 동일 컬럼의 리스트를
 *  구성한다. (참조 qmoPred::processIndexableInSubQ())
 *
 * Implementation :
 *
 *   인덱스 컬럼순서대로 아래의 작업 수행
 *
 *   1. 인덱스 컬럼과 같은 컬럼의 컬럼리스트를 찾는다.
 *      (1) 아직 keyRange로 분류된 predicate이 없는 경우,
 *          keyRange에 찾은 predicate 연결
 *          A. IN subquery 이면, 다음 인덱스 컬럼으로 진행하지 않는다.
 *          B. IN subquery가 아니면, 다음 인덱스 컬럼으로 진행.
 *      (2) 이미 keyRange로 분류된 predicate이 존재하는 경우,
 *          A. IN subquery 이면,
 *             keyRange 에 연결하지 않고,
 *             다음 인덱스 컬럼으로도 진행하지 않는다.
 *          B. IN subquery가 아니면,
 *             keyRange 에 연결하고, 다음 인덱스 컬럼으로 진행.
 *   2. 찾은 컬럼이
 *      (1) 다음 인덱스 컬럼 사용 가능한 컬럼이면 ( equal(=), in )
 *      (2) 다음 인덱스 컬럼 사용 불가능한 컬럼이면, (equal외의 predicate포함)
 *
 *
 ***********************************************************************/

    UInt             sIdxColumnID;
    UInt             sKeyColCount;
    qmoPredWrapper * sWrapperIter;
    idBool           sIsOrConstPred = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoPred::extractKeyRange4Column::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aSource != NULL );
    IDE_DASSERT( aKeyRange != NULL );

    //--------------------------------------
    // KeyRange 추출
    //--------------------------------------

    //--------------------------------------
    // 인덱스 컬럼순서대로 인덱스 컬럼과 동일한 컬럼을 찾아서
    // keyRange포함 여부를 결정한다.
    // 1. 인덱스 컬럼은 연속적으로 사용가능하여야 한다.
    //    예: index on T1(i1, i2, i3)
    //      (1) i1=1 and i2=1 and i3=1 ==> i1,i2,i3 모두 사용 가능
    //      (2) i1=1 and i3=1          ==> i1만 사용 가능
    // 2. IN subquery keyRange는 단독으로 구성해야 한다.
    //    예: index on T1(i1, i2, i3, i4, i5)
    //      (1) i1 in ( subquery ) and i2=1  ==> i1만 사용 가능
    //      (2) i1=1 and i2=1 and i3 in (subquery) and i4=1 and i5=1
    //          ==> i1, i2만 사용 가능
    //--------------------------------------

    for ( sKeyColCount = 0;
          sKeyColCount < aIndex->keyColCount;
          sKeyColCount++ )
    {
        // 인덱스 컬럼의 columnID를 구한다.
        sIdxColumnID = aIndex->keyColumns[sKeyColCount].column.id;

        //--------------------------------------------
        // join index 최적화에 의해 index nested joinable predicate이 있으면,
        // 이 join predicate이 one table predicate을 우선하여
        // keyRange로 추출해야 한다.
        // 현재 predicate 배치상 index joinable predicate이
        // 다른 predicate보다 앞에 위치해 있고,
        // 인덱스는 joinable predicate의 column을 연속적으로 사용할 수 있는
        // 것이므로, 자연스럽게 joinable predicate이 우선적으로 처리될 수
        // 있으므로, 이에 대한 별도의 검사를 하지 않아도 된다.
        //--------------------------------------------

        // 인덱스 컬럼과 같은 columnID를 찾는다.
        for ( sWrapperIter  = *aSource;
              sWrapperIter != NULL;
              sWrapperIter  = sWrapperIter->next )
        {
            if ( sIdxColumnID == sWrapperIter->pred->id )
            {
                /* BUG-42172  If _PROWID pseudo column is compared with a column having
                 * PRIMARY KEY constraint, server stops abnormally.
                 */
                if ( ( sWrapperIter->pred->node->lflag &
                       QTC_NODE_COLUMN_RID_MASK )
                     != QTC_NODE_COLUMN_RID_EXIST )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( sWrapperIter == NULL )
        {
            // 현재 인덱스 컬럼과 동일한 컬럼의 predicate이 존재하지 않는 경우
            break;
        }
        else
        {
            // 현재 인덱스 컬럼과 동일한 컬럼의 predicate이 존재하는 경우

            // 현재 predicate이 IN subquery keyRange이면,
            if ( ( sWrapperIter->pred->flag & QMO_PRED_INSUBQUERY_MASK )
                 == QMO_PRED_INSUBQUERY_EXIST )
            {
                // 첫번째 range로 분류된 거면 range에 추가하고
                // 그렇지 않으면 range에 추가하지 않는다.
                if ( ( *aKeyRange == NULL ) &&
                     ( aIndex->indexHandle != NULL ) )
                {
                    IDE_TEST( qmoPredWrapperI::moveWrapper( sWrapperIter,
                                                            aSource,
                                                            aKeyRange )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing To Do
                }

                // in subquery일 경우 더이상 진행하지 않는다.
                break;
            }
            else
            {
                /* BUG-47836 */
                if ( sIsOrConstPred == ID_FALSE )
                {
                    /* BUG-47509 */
                    if ( ( sWrapperIter->pred->flag & QMO_PRED_OR_VALUE_INDEX_MASK )
                         == QMO_PRED_OR_VALUE_INDEX_FALSE )
                    {
                        IDE_TEST( qmoPredWrapperI::moveWrapper( sWrapperIter,
                                                                aSource,
                                                                aKeyRange )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        if ( ( *aKeyRange == NULL ) &&
                             ( aIndex->indexHandle != NULL ) )
                        {
                            IDE_TEST( qmoPredWrapperI::moveWrapper( sWrapperIter,
                                                                    aSource,
                                                                    aKeyRange )
                                      != IDE_SUCCESS );
                            sIsOrConstPred = ID_TRUE;
                        }
                    }
                }
            }

            if ( ( sWrapperIter->pred->flag & QMO_PRED_NEXT_KEY_USABLE_MASK )
                 == QMO_PRED_NEXT_KEY_USABLE )
            {
                // 이 컬럼은 equal(=)과 in predicate만으로 구성되어 있다.
                // 다음 인덱스 컬럼으로 진행.
                // Nothing To Do
            }
            else
            {
                // 이 컬럼은 equal(=)/in 이외의 predicate을 포함하고 있으므로,
                // 다음 인덱스 컬럼을 사용할 수 없다.

                // 다음 인덱스로의 작업을 진행하지 않는다.
                break;

            }
        } // 인덱스 컬럼과 같은 컬럼을 가진 predicate이 있을 경우,
    } // index column for()문

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::extractKeyFilter( idBool            aIsMemory,
                           qcmIndex        * aIndex,
                           qmoPredWrapper ** aSource,
                           qmoPredWrapper ** aKeyRange,
                           qmoPredWrapper ** aKeyFilter,
                           qmoPredWrapper ** aFilter )
{
/***********************************************************************
 *
 * Description :  keyFilter를 추출
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoPred::extractKeyFilter::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aKeyRange != NULL );
    IDE_DASSERT( aKeyFilter != NULL );
    IDE_DASSERT( aFilter != NULL );

    //--------------------------------------
    // KeyFilter 추출
    //--------------------------------------

    if ( ( aIsMemory == ID_FALSE ) &&
         ( *aKeyRange != NULL ) )
    {
        if ( ( (*aKeyRange)->pred->flag & QMO_PRED_INSUBQUERY_MASK )
             == QMO_PRED_INSUBQUERY_ABSENT )
        {
            //----------------------------------
            // disk table이고,
            // keyRange가 존재하고,
            // keyRange가 IN subquery keyRange가 아닌 경우,
            // keyFilter 추출
            //----------------------------------

            // To fix BUG-27401
            // list에서 이미 keyfilter가 추출되어 있는 경우
            // keyfilter를 추출하지 않는다.
            if ( *aKeyFilter == NULL )
            {
                IDE_TEST( extractKeyFilter4Column( aIndex,
                                                   aSource,
                                                   aKeyFilter )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // 더 이상 처리할 predicate이 없는 경우
            // Nothing To Do
        }
    }
    else
    {
        //----------------------------------
        // keyRange가 존재하지 않거나,
        // memory table인 경우, keyFilter를 추출하지 않는다.
        // LIST 에서 추출한 keyFilter가 있는 경우,
        // filter에 연결한다.
        //----------------------------------

        IDE_TEST( qmoPredWrapperI::moveAll( aKeyFilter, aFilter )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::extractKeyFilter4Column( qcmIndex        * aIndex,
                                  qmoPredWrapper ** aSource,
                                  qmoPredWrapper ** aKeyFilter )
{
/***********************************************************************
 *
 * Description : one column에 대한 keyFilter를 추출한다.
 *
 *  memory table에 대해서는 keyFilter를 추출하지 않는다.
 *  keyFilter는 인덱스의 연속적인 사용과는 상관없이 인덱스에 포함되기만
 *  하면된다.
 *
 * Implementation :
 *
 *   disk table인 경우,
 *   1. 인덱스 컬럼과 동일한 컬럼의 컬럼리스트를 찾는다.
 *   2. 찾은 컬럼리스트에
 *      (1) IN(subquery)가 존재하면,
 *          아무 처리도 하지 않고, 다음 인덱스 컬럼 수행.
 *      (2) IN(subquery)가 존재하지 않으면,
 *          keyFilter 에 연결하고, 다음 인덱스 컬럼 수행.
 *
 ***********************************************************************/

    UInt             sIdxColumnID;
    UInt             sKeyColCount;
    qmoPredWrapper * sWrapperIter;

    IDU_FIT_POINT_FATAL( "qmoPred::extractKeyFilter4Column::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aSource != NULL );
    IDE_DASSERT( aKeyFilter != NULL );

    //--------------------------------------
    // KeyFilter 추출
    //--------------------------------------

    for ( sKeyColCount = 0;
          sKeyColCount < aIndex->keyColCount && *aSource != NULL;
          sKeyColCount++ )
    {
        // 인덱스 컬럼의 columnID를 구한다.
        sIdxColumnID = aIndex->keyColumns[sKeyColCount].column.id;

        // 인덱스 컬럼과 같은 columnID를 찾는다.
        for ( sWrapperIter  = *aSource;
              sWrapperIter != NULL;
              sWrapperIter  = sWrapperIter->next )
        {
            if ( sIdxColumnID == sWrapperIter->pred->id )
            {
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( sWrapperIter == NULL )
        {
            // 인덱스 컬럼과 동일한 컬럼의 predicate이 존재하지 않는 경우,
            // 다음 인덱스 컬럼으로 진행
            // keyFilter는 연속적인 인덱스 사용에 관계없이
            // 인덱스 컬럼에 속하기만 하면 된다.

            // Nothing To Do
        }
        else
        {
            // 인덱스 컬럼과 동일한 컬럼의 predicate이 존재하는 경우,

            if ( ( sWrapperIter->pred->flag & QMO_PRED_INSUBQUERY_MASK )
                 == QMO_PRED_INSUBQUERY_EXIST )
            {
                // 인덱스 컬럼과 같은 columnID를 가진 컬럼의 predicate이
                // IN subquery인 경우,
                // IN subquery는 keyFilter로 사용하지 못하므로, skip

                // Nothing To Do
            }
            else
            {
                // 인덱스 컬럼과 같은 columnID를 가진 컬럼의 predicate이
                // IN subquery가 아닌 경우,
                // 즉, keyFilter로 사용할 수 있는 predicate
                /* BUG-47986 */
                if ( ( sWrapperIter->pred->flag & QMO_PRED_JOIN_OR_VALUE_INDEX_MASK )
                     == QMO_PRED_JOIN_OR_VALUE_INDEX_FALSE )
                {
                    // keyFilter에 연결
                    IDE_TEST( qmoPredWrapperI::moveWrapper( sWrapperIter,
                                                            aSource,
                                                            aKeyFilter )
                              != IDE_SUCCESS );
                }
            }

            // fix BUG-10091
            // = 이외의 비교연산자가 존재하면
            // keyFilter 추출을 멈춘다.
            if ( ( sWrapperIter->pred->flag & QMO_PRED_NEXT_KEY_USABLE_MASK )
                 == QMO_PRED_NEXT_KEY_USABLE )
            {
                // Nothing To Do
            }
            else
            {
                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::extractRange4LIST( qcTemplate         * aTemplate,
                            qcmIndex           * aIndex,
                            qmoPredWrapper    ** aSource,
                            qmoPredWrapper    ** aKeyRange,
                            qmoPredWrapper    ** aKeyFilter,
                            qmoPredWrapper    ** aFilter,
                            qmoPredWrapper    ** aSubqueryFilter,
                            qmoPredWrapperPool * aWrapperPool )
{
/***********************************************************************
 *
 * Description : LIST에 대한
 *               keyRange, keyFilter, filter, subqueryFilter를 추출한다.
 *
 * Implementation :
 *
 *   이 함수는 리스트 컬럼리스트만 인자로 받아서,
 *   하나의 리스트에 대해 keyRange/keyFilter/filter/subqueryFilter를
 *   판단해서 해당하는 곳에 연결한다.
 *
 *      리스트의 인덱스 사용가능성 검사.
 *      (1) 리스트컬럼이 인덱스 컬럼에 연속적으로 포함되면, keyRange로 분류
 *      (2) 연속적이지는 않지만, 리스트 컬럼이 모두 인덱스 컬럼에 포함되면,
 *          keyFilter로 분류
 *      (3) (1)과 (2)가 아니면, filter로 분류
 *
 ***********************************************************************/

    qmoPredWrapper * sWrapperIter;
    qmoPredicate   * sPredIter;
    qmoPredType      sPredType;

    IDU_FIT_POINT_FATAL( "qmoPred::extractRange4LIST::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aSource != NULL );
    IDE_DASSERT( aKeyRange != NULL );
    IDE_DASSERT( aKeyFilter != NULL );
    IDE_DASSERT( aFilter != NULL );
    IDE_DASSERT( aSubqueryFilter != NULL );

    //--------------------------------------
    // 리스트 컬럼리스트에 달려있는 각각의 컬럼에 대해
    // keyRange/keyFilter/filter/IN(subquery)에 대한 판단을 수행.
    //--------------------------------------

    for ( sWrapperIter  = *aSource;
          sWrapperIter != NULL;
          sWrapperIter  = sWrapperIter->next )
    {
        if ( sWrapperIter->pred->id == QMO_COLUMNID_LIST )
        {
            IDE_TEST( qmoPredWrapperI::extractWrapper( sWrapperIter,
                                                       aSource )
                      != IDE_SUCCESS );

            break;
        }
        else
        {
            // Nothing to do...
        }
    }

    //--------------------------------------
    // KeyRange, KeyFilter, filter, subqueryFilter 추출
    //--------------------------------------

    if ( sWrapperIter != NULL )
    {
        for ( sPredIter  = sWrapperIter->pred;
              sPredIter != NULL;
              sPredIter  = sPredIter->more )
        {
            IDE_TEST( checkUsableIndex4List( aTemplate,
                                             aIndex,
                                             sPredIter,
                                             & sPredType )
                      != IDE_SUCCESS );

            if ( sPredType == QMO_KEYRANGE )
            {
                IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                    aKeyRange,
                                                    aWrapperPool )
                          != IDE_SUCCESS );
            }
            else
            {
                // keyFilter나 filter로 분류된 경우,
                // 이 경우, IN subquery는 subqueryFilter에 연결해야 한다.

                if ( ( sPredIter->flag & QMO_PRED_INSUBQUERY_MASK )
                     == QMO_PRED_INSUBQUERY_EXIST )
                {
                    // IN subquery이면, subqueryFilter에 연결
                    IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                        aSubqueryFilter,
                                                        aWrapperPool )
                              != IDE_SUCCESS );
                }
                else
                {
                    if ( sPredType == QMO_KEYFILTER )
                    {
                        // keyFilter로 분류된 경우로, keyFilter에 연결
                        IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                            aKeyFilter,
                                                            aWrapperPool )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // BUG-11328 fix
                        // 남아 있는 predicate에 대해
                        // filter 인지 subquery filter인지
                        // 판단해야 한다.
                        // 기존에는 이 블럭에서 filter로만 처리했다.
                        if ( ( sPredIter->node->lflag & QTC_NODE_SUBQUERY_MASK )
                             == QTC_NODE_SUBQUERY_EXIST )
                        {
                            IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                                aSubqueryFilter,
                                                                aWrapperPool )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                                aFilter,
                                                                aWrapperPool )
                                      != IDE_SUCCESS );
                        }
                    }
                }
            }
        } // for
    }
    else // list predicate이 없는 경우 아무런 작업을 하지 않는다.
    {
        // Nothing to do...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::process4Range( qcStatement        * aStatement,
                        qmsQuerySet        * aQuerySet,
                        qmoPredicate       * aRange,
                        qmoPredWrapper    ** aFilter,
                        qmoPredWrapperPool * aWrapperPool )
{
/***********************************************************************
 *
 * Description : keyRange와 keyFilter로 분류된 predicate에 대한 처리
 *
 *       (1) quantify 비교연산자에 대한 노드 변환 수행
 *       (2) LIKE 비교연산자에 대한 filter 생성
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate  * sPredicate;
    qmoPredicate  * sMorePredicate;
    qmoPredicate  * sLikeFilter = NULL;
    qmsProcessPhase sOrgPhase;

    IDU_FIT_POINT_FATAL( "qmoPred::process4Range::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aRange != NULL );
    IDE_DASSERT( aFilter != NULL );

    //------------------------------------------
    // Range에 대한 처리
    //------------------------------------------

    sOrgPhase = aQuerySet->processPhase;
    aQuerySet->processPhase = QMS_OPTIMIZE_NODE_TRANS;

    for ( sPredicate  = aRange;
          sPredicate != NULL;
          sPredicate  = sPredicate->next )
    {
        if ( sPredicate->id == QMO_COLUMNID_LIST )
        {
            for ( sMorePredicate  = sPredicate;
                  sMorePredicate != NULL;
                  sMorePredicate  = sMorePredicate->more )
            {
                //------------------------------------------
                // quantify 비교연산자에 대한 노드 변환
                //------------------------------------------

                IDE_TEST( nodeTransform( aStatement,
                                         sMorePredicate->node,
                                         & sMorePredicate->node,
                                         aQuerySet )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            if ( ( sPredicate->flag & QMO_PRED_NEXT_KEY_USABLE_MASK )
                 == QMO_PRED_NEXT_KEY_USABLE )
            {
                //------------------------------------------
                // one column predicate이면서, 다음인덱스 사용가능한 경우
                // 노드변환만 수행하면 됨.
                //------------------------------------------
                for ( sMorePredicate  = sPredicate;
                      sMorePredicate != NULL;
                      sMorePredicate  = sMorePredicate->more )
                {
                    //------------------------------------------
                    // quantify 비교연산자에 대한 노드 변환
                    //------------------------------------------
                    IDE_TEST( nodeTransform( aStatement,
                                             sMorePredicate->node,
                                             & sMorePredicate->node,
                                             aQuerySet )
                              != IDE_SUCCESS );

                }
            }
            else
            {
                //------------------------------------------
                // one column predicate이면서,
                // 다음인덱스 사용가능하지 않은 경우
                // (즉, equal/in 이외의 비교연산자가 있는 경우)
                //------------------------------------------
                for ( sMorePredicate  = sPredicate;
                      sMorePredicate != NULL;
                      sMorePredicate  = sMorePredicate->more )
                {
                    //------------------------------------------
                    // quantify 비교연산자에 대한 노드 변환
                    //------------------------------------------

                    IDE_TEST( nodeTransform( aStatement,
                                             sMorePredicate->node,
                                             & sMorePredicate->node,
                                             aQuerySet )
                              != IDE_SUCCESS );

                    //------------------------------------------
                    // To Fix PR-9679
                    // LIKE 등과 같이 Filter가 반드시 필요한
                    // 비교연산자에 대한 filter 생성
                    //------------------------------------------

                    IDE_TEST( makeFilterNeedPred( aStatement,
                                                  sMorePredicate,
                                                  & sLikeFilter )
                              != IDE_SUCCESS );

                    if ( sLikeFilter == NULL )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        IDE_TEST( qmoPredWrapperI::addPred( sLikeFilter,
                                                            aFilter,
                                                            aWrapperPool )
                                  != IDE_SUCCESS );
                    }
                }
            }
        }
    }

    aQuerySet->processPhase = sOrgPhase;

    //------------------------------------------
    // variable / fixed / IN subquery KeyRange 정보 저장
    //------------------------------------------

    IDE_TEST( setRangeInfo( aQuerySet, aRange )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::separateFilters( qcTemplate         * aTemplate,
                          qmoPredWrapper     * aSource,
                          qmoPredWrapper    ** aFilter,
                          qmoPredWrapper    ** aLobFilter,
                          qmoPredWrapper    ** aSubqueryFilter,
                          qmoPredWrapperPool * aWrapperPool )
{
/***********************************************************************
 *
 * Description : qmoPredicate 리스트에서 filter, subqueryFilter를 분리한다.
 *
 *   extractRangeAndFilter()함수에서
 *   1. 인덱스 정보가 없을 경우,
 *      인자로 넘어온 predicate을 filter와 subqueryFilter로 분리하기 위해서
 *   2. 인덱스 정보가 있을 경우,
 *      keyRange, keyFilter를 모두 추출하고 남은 predicate들을
 *      filter와 subqueryFilter로 분리하기 위해서
 *   이 함수를 호출하게 된다.
 *
 * Implementation :
 *
 *    1. indexable predicate에 대한
 *       IN(subquery), subquery KeyRange 최적화 팁 제거
 *    2. subqueryFilter, filter 분리
 *
 ***********************************************************************/

    qmoPredWrapper * sWrapperIter;
    qmoPredicate   * sPredIter;

    IDU_FIT_POINT_FATAL( "qmoPred::separateFilters::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aFilter != NULL );
    IDE_DASSERT( aLobFilter != NULL );
    IDE_DASSERT( aSubqueryFilter != NULL );

    //------------------------------------------
    // 인자로 넘어온 predicate list를 각각 조사해서,
    // filter와 subqueryFilter를 분리한다.
    //------------------------------------------

    for( sWrapperIter = aSource;
         sWrapperIter != NULL;
         sWrapperIter = sWrapperIter->next )
    {
        for( sPredIter = sWrapperIter->pred;
             sPredIter != NULL;
             sPredIter = sPredIter->more )
        {
            if( ( sPredIter->node->lflag & QTC_NODE_SUBQUERY_MASK )
                == QTC_NODE_SUBQUERY_EXIST )
            {
                //----------------------------
                // subquery를 포함하는 경우
                //----------------------------
                IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                    aSubqueryFilter,
                                                    aWrapperPool )
                          != IDE_SUCCESS );
            }
            else
            {
                //-------------------------------
                // subquery를 포함하지 않는 경우
                //-------------------------------

                if( (QTC_NODE_LOB_COLUMN_EXIST ==
                     (sPredIter->node->lflag & QTC_NODE_LOB_COLUMN_MASK)) &&
                    (aTemplate->stmt->myPlan->parseTree->stmtKind ==
                     QCI_STMT_SELECT_FOR_UPDATE) )
                {
                    /* BUG-25916
                     * clob을 select fot update 하던 도중 Assert 발생 */
                    /* lob 컬럼의 경우 필터처리시에 페이지참조(getPage)가 필요하다.
                     * 그런데 select for update시에는 record lock을 걸기위해
                     * 이미 페이지에 X lock이 잡힌 상태이기 때문에,
                     * lob 컬럼의 필터처리를 할때 S lock을 획득하려다가
                     * ASSERT로 죽는 문제가 발생한다.
                     * 그래서 이 경우(select for update && lob 컬럼이 있는 경우)
                     * lobFilter로 분류하고 SCAN위에 FILT 노드를 추가하여 처리한다. */
                    IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                        aLobFilter,
                                                        aWrapperPool )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( qmoPredWrapperI::addPred( sPredIter,
                                                        aFilter,
                                                        aWrapperPool )
                              != IDE_SUCCESS );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::setRangeInfo( qmsQuerySet  * aQuerySet,
                       qmoPredicate * aPredicate )
{
/***********************************************************************
 *
 * Description : keyRange/keyFilter로 추출된 predicate의 연결리스트 중
 *               좌측최상단에 fixed/variable/InSubqueryKeyRange에 대한
 *               정보를 저장한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate * sPredicate;
    qmoPredicate * sMorePredicate;

    IDU_FIT_POINT_FATAL( "qmoPred::setRangeInfo::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aPredicate != NULL );

    //------------------------------------------
    //  인자로 넘어온 predicate의 연결리스트 중 좌측최상단에
    //  fixed/variable/IN Subquery keyRange 정보 저장
    //------------------------------------------

    for( sPredicate = aPredicate;
         sPredicate != NULL;
         sPredicate = sPredicate->next )
    {
        for( sMorePredicate = sPredicate;
             sMorePredicate != NULL;
             sMorePredicate = sMorePredicate->more )
        {
            if( ( QMO_PRED_IS_VARIABLE( sMorePredicate ) == ID_TRUE ) ||
                ( ( sMorePredicate->flag & QMO_PRED_VALUE_MASK )
                  == QMO_PRED_VARIABLE ) )
            {
                aPredicate->flag &= ~QMO_PRED_VALUE_MASK;
                aPredicate->flag |= QMO_PRED_VARIABLE;

                if( ( sMorePredicate->flag & QMO_PRED_INSUBQUERY_MASK )
                    == QMO_PRED_INSUBQUERY_EXIST )
                {
                    aPredicate->flag &= ~QMO_PRED_INSUBQ_KEYRANGE_MASK;
                    aPredicate->flag |= QMO_PRED_INSUBQ_KEYRANGE_TRUE;
                }
                else
                {
                    // Nothing To Do
                }

                break;
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    //-------------------------------------
    // fixed 인 경우, LEVEL column이 포함된 경우,
    // hierarchy query 이면, variable keyRange로,
    // hierarchy query가 아니면, fixed keyRange로 분류되어야 한다.
    //--------------------------------------

    if( aQuerySet->SFWGH->hierarchy != NULL )
    {
        // hierarchy가 존재하는 경우

        if( ( aPredicate->flag & QMO_PRED_VALUE_MASK ) == QMO_PRED_FIXED )
        {
            for( sPredicate = aPredicate;
                 sPredicate != NULL;
                 sPredicate = sPredicate->next )
            {
                for( sMorePredicate = sPredicate;
                     sMorePredicate != NULL;
                     sMorePredicate = sMorePredicate->more )
                {
                    if( ( sMorePredicate->node->lflag
                          & QTC_NODE_LEVEL_MASK ) == QTC_NODE_LEVEL_EXIST )
                    {
                        aPredicate->flag &= ~QMO_PRED_VALUE_MASK;
                        aPredicate->flag |= QMO_PRED_VARIABLE;

                        break;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
            }
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // hierarchy가 존재하지 않는 경우,
        // Nothing To Do
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::makeFilterNeedPred( qcStatement   * aStatement,
                             qmoPredicate  * aPredicate,
                             qmoPredicate ** aLikeFilterPred )
{
/***********************************************************************
 *
 * Description :  Like와 같이 Range가 존재하더라도
 *                Filter가 필요한 경우 이에 대한 Filter를 생성한다.
 *
 *     LIKE 비교연산자는 keyRange로 추출되었다하더라도
 *     'ab%de', 'a_ce' 와 같은 패턴문자를 검색하기 위한 filter가 필요하다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate  * sPredicate;
    qtcNode       * sLikeNode;
    qtcNode       * sLikeFilterNode = NULL;
    qtcNode       * sLastNode = NULL;
    qtcNode       * sCompareNode;
    qtcNode       * sORNode[2];
    qcNamePosition  sNullPosition;
    idBool          sNeedFilter = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoPred::makeFilterNeedPred::__FT__" );

    //------------------------------------------
    // LIKE에 대한 filter 생성
    //------------------------------------------

    if( ( aPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
        == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        // CNF 인 경우

        // OR 노드 하위의 비교연산자 노드 중 LIKE비교연산자에 대한
        // filter 생성이 필요한지 검사
        sCompareNode = (qtcNode *)(aPredicate->node->node.arguments);

        while( sCompareNode != NULL )
        {
            if( ( sCompareNode->node.lflag & MTC_NODE_FILTER_MASK )
                == MTC_NODE_FILTER_NEED )
            {
                // To Fix BUG-12306
                IDE_TEST( mtf::checkNeedFilter( & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                                & sCompareNode->node,
                                                & sNeedFilter )
                          != IDE_SUCCESS );

                if ( sNeedFilter == ID_TRUE )
                {
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // Nothing To Do
            }

            sCompareNode = (qtcNode *)(sCompareNode->node.next);
        }

        if ( sNeedFilter == ID_TRUE )
        {
            // BUG-15482, BUG-24843
            // OR 노드 하위의 비교연산자 노드 중 LIKE비교연산자에
            // 하나라도 filter가 필요하면 LIKE를 포함한 모든 연산자에
            // 대해 filter를 생성해야 한다.
            sCompareNode = (qtcNode *)(aPredicate->node->node.arguments);

            while( sCompareNode != NULL )
            {
                // 비교연산자 하위 노드들 모두 복사
                IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                                 sCompareNode,
                                                 & sLikeNode,
                                                 ID_FALSE,
                                                 ID_FALSE,
                                                 ID_FALSE,
                                                 ID_FALSE )
                          != IDE_SUCCESS );

                if( sLastNode == NULL )
                {
                    sLikeFilterNode = sLikeNode;
                    sLastNode = sLikeFilterNode;
                }
                else
                {
                    sLastNode->node.next = (mtcNode *)&(sLikeNode->node);
                    sLastNode = (qtcNode *)(sLastNode->node.next);
                }

                sCompareNode = (qtcNode *)(sCompareNode->node.next);
            }

            // BUG-15482
            // 새로운 OR 노드를 하나 생성한다.
            SET_EMPTY_POSITION( sNullPosition );

            IDE_TEST( qtc::makeNode( aStatement,
                                     sORNode,
                                     & sNullPosition,
                                     (const UChar*)"OR",
                                     2 )
                      != IDE_SUCCESS );

            // OR 노드 하위에 sLikeFilterNode를 연결한다.
            sORNode[0]->node.arguments = (mtcNode *)sLikeFilterNode;

            IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                        sORNode[0] )
                      != IDE_SUCCESS );

            sLikeFilterNode = sORNode[0];
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // DNF 인 경우

        if ( ( aPredicate->node->node.lflag & MTC_NODE_FILTER_MASK )
            == MTC_NODE_FILTER_NEED )
        {
            // To Fix BUG-12306
            IDE_TEST( mtf::checkNeedFilter( & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                            & aPredicate->node->node,
                                            & sNeedFilter )
                      != IDE_SUCCESS );
            if( sNeedFilter == ID_TRUE )
            {
                IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                                 aPredicate->node,
                                                 & sLikeFilterNode,
                                                 ID_FALSE,
                                                 ID_FALSE,
                                                 ID_FALSE,
                                                 ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // nothing to do
        }
    }

    if( sLikeFilterNode != NULL )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoPredicate),
                                                 (void **)&sPredicate )
                  != IDE_SUCCESS );

        idlOS::memcpy( sPredicate, aPredicate, ID_SIZEOF(qmoPredicate) );

        sPredicate->node = sLikeFilterNode;
        sPredicate->flag = QMO_PRED_CLEAR;
        sPredicate->id   = QMO_COLUMNID_NON_INDEXABLE;
        sPredicate->more = NULL;
        sPredicate->next = NULL;

        *aLikeFilterPred = sPredicate;
    }
    else
    {
        *aLikeFilterPred = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoPred::nodeTransform( qcStatement  * aStatement,
                        qtcNode      * aSourceNode,
                        qtcNode     ** aTransformNode,
                        qmsQuerySet  * aQuerySet )
{
/***********************************************************************
 *
 * Description : keyRange 적용을 위한 노드 변환
 *
 *     LIST 와 quantify 비교연산자에 대해서, keyRange를 적용하기 위해
 *     system level operator로 노드변환시킨다.
 *
 *     OR 노드 하위에 one column인 경우, 여러개의 비교연산자가 올 수 있고,
 *     OR 노드 하위에 LIST 는 하나만 올 수 있다.
 *
 * Implementation :
 *
 *       1. 비교연산자 노드가 quantify인 경우 노드 변환
 *           예: i1 in ( 1, 2 ) or i1 in ( 3, 4)
 *               i1 in ( 1, 2 ) or i1 < 5
 *           예: i1 in ( 1, 2 )
 *       2. 비교연산자 노드가 quantify가 아닌 경우, LIST만 노드 변환
 *
 ***********************************************************************/

    idBool    sIsNodeTransformNeed;
    qtcNode * sTransformOneCond;
    qtcNode * sTransformNode = NULL;
    qtcNode * sLastNode      = NULL;
    qtcNode * sCompareNode;

    IDU_FIT_POINT_FATAL( "qmoPred::nodeTransform::__FT__" );

    if( ( aSourceNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
        == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        // CNF인 경우로, 최상위 노드가 OR 노드
        sCompareNode = (qtcNode *)(aSourceNode->node.arguments);

        //------------------------------------------
        // 하나의 비교연산자 단위로 노드 변환
        // 1. 비교연산자 노드가 quantify인 경우 노드 변환
        //    예: i1 in ( 1, 2 ) or i1 in ( 3, 4)
        //        i1 in ( 1, 2 ) or i1 < 5
        //    예: i1 in ( 1, 2 )
        // 2. 비교연산자 노드가 quantify가 아닌 경우, LIST만 노드 변환
        //------------------------------------------

        while( sCompareNode != NULL )
        {
            sIsNodeTransformNeed = ID_TRUE;

            if( ( sCompareNode->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK )
                == MTC_NODE_GROUP_COMPARISON_TRUE )
            {
                // quantify 비교연산자로노드변환 필요
                // Nothing To Do
            }
            else
            {
                // quantify 비교연산자가 아닌 경우, LIST만 노드 변환
                if( sCompareNode->indexArgument == 0 )
                {
                    if( ( sCompareNode->node.arguments->lflag &
                          MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_LIST )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        sIsNodeTransformNeed = ID_FALSE;
                    }
                }
                else
                {
                    if( ( sCompareNode->node.arguments->next->lflag &
                          MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_LIST )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        sIsNodeTransformNeed = ID_FALSE;
                    }
                }
            }

            // 노드 변환 수행.
            if( sIsNodeTransformNeed == ID_TRUE )
            {
                IDE_TEST( nodeTransform4OneCond( aStatement,
                                                 sCompareNode,
                                                 & sTransformOneCond,
                                                 aQuerySet )
                          != IDE_SUCCESS );
            }
            else
            {
                sTransformOneCond = sCompareNode;
            }

            // 처리된 비교연산자 연결리스트 만들기
            if( sTransformNode == NULL )
            {
                sTransformNode = sTransformOneCond;
                sLastNode = sTransformNode;
            }
            else
            {
                sLastNode->node.next = (mtcNode *)(sTransformOneCond);
                sLastNode = (qtcNode *)(sLastNode->node.next);
            }

            sCompareNode = (qtcNode *)(sCompareNode->node.next);
        }

        //--------------------------------------
        // 노드 변환환 노드들을 원래 OR노드 하위에 연결한다.
        // 예: i1 in (1,2) OR i1 = 7
        //     아래와 같이 노드 변환되는데, 이후, qtcNode로 연결할때
        //     최상위 노드의 next가 있는 경우를 고려해야 하므로,
        //     여기서, 원래 OR 노드 하위에 노드 변환된 노드를 하위로 연결한다.
        //
        //                                      OR
        //                                      |
        //      OR ------------ [=]             OR ------------ [=]
        //      |                |              |                |
        //     [=]    - [=]     [i1]-[7]  ==>  [=]    - [=]     [i1]-[7]
        //      |        |                      |        |
        //     [i1]-[1] [i1]-[2]               [i1]-[1] [i1]-[2]
        //--------------------------------------
        aSourceNode->node.arguments = (mtcNode *)&(sTransformNode->node);
        sTransformNode = aSourceNode;
    } // CNF 인 경우의 처리
    else
    {
        // DNF인 경우로, 최상위 노드가 비교연산자

        sIsNodeTransformNeed = ID_TRUE;

        if( ( aSourceNode->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK )
            == MTC_NODE_GROUP_COMPARISON_TRUE )
        {
            // quantify 비교연산자로노드변환 필요
            // Nothing To Do
        }
        else
        {
            // quantify 비교연산자가 아닌 경우, LIST만 노드 변환
            if( aSourceNode->indexArgument == 0 )
            {
                if( ( aSourceNode->node.arguments->lflag &
                      MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_LIST )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsNodeTransformNeed = ID_FALSE;
                }
            }
            else
            {
                if( ( aSourceNode->node.arguments->next->lflag &
                      MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_LIST )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsNodeTransformNeed = ID_FALSE;
                }
            }
        }

        // 노드 변환 수행.
        if( sIsNodeTransformNeed == ID_TRUE )
        {
            IDE_TEST( nodeTransform4OneCond( aStatement,
                                             aSourceNode,
                                             & sTransformNode,
                                             aQuerySet )
                      != IDE_SUCCESS );
        }
        else
        {
            sTransformNode = aSourceNode;
        }
    } // DNF 인 경우의 처리

    *aTransformNode = sTransformNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * qmoPredTrans 관련 설명
 *
 *    : nodeTransform() 함수를 수행하는 과정에서
 *      여러 내부 함수들이 호출되는데
 *      모두 노드들을 재구성하는 복잡한 로직으로 구현이 되어 있다.
 *      nodeTransform() 과 관련된 함수들이 호출되는 인터페이스를
 *      단순화하고, 함수 내용을 보다 쉽게 이해할 수 있게 하기 위해
 *      관련함수들을 리팩토링한다.
 *
 *      리팩토링의 요점은, nodeTransform 관련 함수들 사이의
 *      parameter의 복잡도를 줄이기 위해
 *      qmoPredTrans라는 구조체를 정의하고
 *      이 구조체를 operation할 수 있는 함수들을 모아 놓은
 *      qmoPredTransI라는 class를 정의한다.
 *
 *      qtcNode의 구조를 탐색하는 로직은 모두
 *      qmoPredTransI의 함수들이 처리하고
 *      nodeTransform 관련 함수들은 노드 변형에 관련된 로직만을 구현한다.
 *
 *      by kumdory, 2005-04-25
 *
 **********************************************************************/

void
qmoPredTransI::initPredTrans( qmoPredTrans  * aPredTrans,
                              qcStatement   * aStatement,
                              qtcNode       * aCompareNode,
                              qmsQuerySet   * aQuerySet )
{
/***********************************************************************
 *
 * Description :
 *      aPredTrans의 각 맴버를 초기화한다.
 *
 * Implementation :
 *      aStatement를 세팅하고,
 *      Logical node와 condition node를 만들기 위해
 *      compareNode의 오퍼레이션을 분석한다.
 *
 ***********************************************************************/

    IDE_DASSERT( aPredTrans != NULL );

    aPredTrans->statement = aStatement;
    setLogicalNCondition( aPredTrans, aCompareNode );

    // fix BUG-18242
    aPredTrans->myQuerySet = aQuerySet;
}

IDE_RC
qmoPredTransI::makeConditionNode( qmoPredTrans * aPredTrans,
                                  qtcNode      * aArgumentNode,
                                  qtcNode     ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *      "=", "<=" 같은 condition node 를 생성한다.
 *
 * Implementation :
 *      qmoPredTransI의 private 함수인 makePredNode를 사용한다.
 *      만들어진 condition node의 indexArgument 를 0으로 세팅해야 한다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoPredTransI::makeConditionNode::__FT__" );

    IDE_TEST( makePredNode( aPredTrans->statement,
                            aArgumentNode,
                            aPredTrans->condition,
                            aResultNode )
              != IDE_SUCCESS );

    // 생성된 condition node에 대해 estimation해준다.
    IDE_TEST( estimateConditionNode( aPredTrans,
                                     *aResultNode )
              != IDE_SUCCESS );

    // 비교 연산자 노드에 대해 indexArgument를 0으로 세팅한다.
    (*aResultNode)->indexArgument = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredTransI::makeLogicalNode( qmoPredTrans * aPredTrans,
                                qtcNode      * aArgumentNode,
                                qtcNode     ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *      "AND", "OR" 같은 logical operator node 를 생성한다.
 *
 * Implementation :
 *      qmoPredTransI의 private 함수인 makePredNode를 사용한다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoPredTransI::makeLogicalNode::__FT__" );

    IDE_TEST( makePredNode( aPredTrans->statement,
                            aArgumentNode,
                            aPredTrans->logical,
                            aResultNode )
              != IDE_SUCCESS );

    // 생성된 logical node에 대해 estimation해준다.
    IDE_TEST( estimateLogicalNode( aPredTrans,
                                   * aResultNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredTransI::makeAndNode( qmoPredTrans * aPredTrans,
                            qtcNode      * aArgumentNode,
                            qtcNode     ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *      "AND" node 를 생성한다.
 *
 * Implementation :
 *      qmoPredTransI의 private 함수인 makePredNode를 사용한다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoPredTransI::makeAndNode::__FT__" );

    IDE_TEST( makePredNode( aPredTrans->statement,
                            aArgumentNode,
                            (SChar*)"AND",
                            aResultNode )
              != IDE_SUCCESS );

    // 생성된 logical node에 대해 estimation해준다.
    IDE_TEST( estimateLogicalNode( aPredTrans,
                                   *aResultNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredTransI::makePredNode( qcStatement  * aStatement,
                             qtcNode      * aArgumentNode,
                             SChar        * aOperator,
                             qtcNode     ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *      각종 predicate node를 성성한다.
 *      priavate 함수이며, makeLogicalNode, makeConditionNode, makeAndNode에
 *      의해 호출된다.
 *
 * Implementation :
 *      aArgumentNode는 반드시 next가 달려 있어야 한다.
 *
 ***********************************************************************/

    qcNamePosition sNullPosition;
    qtcNode      * sResultNode[2];

    IDU_FIT_POINT_FATAL( "qmoPredTransI::makePredNode::__FT__" );

    IDE_DASSERT( aArgumentNode != NULL );

    SET_EMPTY_POSITION( sNullPosition );

    IDE_TEST( qtc::makeNode( aStatement,
                             sResultNode,
                             & sNullPosition,
                             (const UChar*)aOperator,
                             idlOS::strlen( aOperator ) )
              != IDE_SUCCESS );

    sResultNode[0]->node.arguments = (mtcNode*)&(aArgumentNode->node);

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sResultNode[0] )
              != IDE_SUCCESS );

    *aResultNode = sResultNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredTransI::copyNode( qmoPredTrans * aPredTrans,
                         qtcNode      * aNode,
                         qtcNode     ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *      qtcNode를 새로운 메모리 영역에 복사한다.
 *
 * Implementation :
 *      사실 이 함수는 qtc.cpp에 있어야 한다.
 *
 ***********************************************************************/

    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoPredTransI::copyNode::__FT__" );

    // 새로운 노드를 만들기 위한 메모리를 할당받는다.
    IDE_TEST( QC_QMP_MEM( aPredTrans->statement )->alloc( ID_SIZEOF( qtcNode ),
                                                          (void **)& sNode )
              != IDE_SUCCESS  );

    // 인자로 받은 column node와 value node를 복사.
    idlOS::memcpy( sNode, aNode, ID_SIZEOF(qtcNode) );

    sNode->node.next = NULL;

    *aResultNode = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredTransI::makeSubQWrapperNode( qmoPredTrans * aPredTrans,
                                    qtcNode      * aValueNode,
                                    qtcNode     ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *      subqueryWrapper 노드를 생성한다.
 *
 * Implementation :
 *      qtc에 이미 있는 함수지만
 *      qmoPred의 transformNode 계열 함수에서
 *      각종 노드를 만드는 동일한 함수 인터페이스를 제공하기 위해
 *      한번 wrapping 한다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoPredTransI::makeSubQWrapperNode::__FT__" );

    IDE_TEST( qtc::makeSubqueryWrapper( aPredTrans->statement,
                                        aValueNode,
                                        aResultNode )
                 != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredTransI::makeIndirectNode( qmoPredTrans * aPredTrans,
                                 qtcNode      * aValueNode,
                                 qtcNode     ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *      새로운 메모리 영역에 indirect node를 생성한다.
 *
 * Implementation :
 *      qtc에 있는 함수를 사용하지만, alloc은 여기서 해준다.
 *      makeSubQWrapperNode와 마찬가지로 동일한 인터페이스를
 *      제공하기 위해 한번 wrapping 한다.
 *
 ***********************************************************************/

    qtcNode * sIndNode;

    IDU_FIT_POINT_FATAL( "qmoPredTransI::makeIndirectNode::__FT__" );

    IDE_TEST( QC_QMP_MEM( aPredTrans->statement )->alloc( ID_SIZEOF( qtcNode ),
                                                          (void **)& sIndNode )
              != IDE_SUCCESS );

    IDE_TEST( qtc::makeIndirect( aPredTrans->statement,
                                 sIndNode,
                                 aValueNode )
              != IDE_SUCCESS );

    // PROJ-2415 Grouping Sets
    // clear subquery depInfo
    qtc::dependencyClear( & sIndNode->depInfo );

    *aResultNode = sIndNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredTransI::estimateConditionNode( qmoPredTrans * aPredTrans,
                                      qtcNode      * aConditionNode )
{
/***********************************************************************
 *
 * Description :
 *      qmoPred::nodeTransform4OneNode에 있던 estimate 부분을
 *      따로 뺀 함수이다.
 *
 * Implementation :
 *
 *   [ column node의 conversion 정보 설정 ] :
 *    quantify 비교연산자는 column에 대한 conversion 정보를
 *    value node의 leftConversion에 가지고 있다.
 *    따라서, value 노드에 leftConversion node가 있으면,
 *    이 leftConversion을 columnNode의 conversion으로 연결하고,
 *    valueNode의 leftConversion의 연결관계는 끊는다.
 *
 *   // fix BUG-10574
 *   // host변수를 포함하는 predicate에 대한 keyrange 적용 노드변환시,
 *   // 노드 변환이 수행되고 난 후,
 *   // host변수를 포함하지 않는 predicate에 대해, estimate 를 수행한다.
 *   //   예) i1 in ( 1, 1.0, ? )
 *   //       노드변환 수행
 *   //       ==> (i1=1) or (i1=1.0) or (i1=?)
 *   //           ------------------
 *   //           estimate 수행
 ***********************************************************************/

    qtcNode        * sArgColumnNode;
    qtcNode        * sArgValueNode;
    qtcNode        * sValueNode;

    IDU_FIT_POINT_FATAL( "qmoPredTransI::estimateConditionNode::__FT__" );

    sArgColumnNode = qtcNodeI::argumentNode( aConditionNode );
    sArgValueNode = qtcNodeI::nextNode( sArgColumnNode );

    // fix BUG-12061 BUG-12058
    // 예 : where ( i1, i2 ) = ( (select max(i1) ..),
    //                           (select max(i2) ..) ) 의 노드 변환 후,
    //      where ( i1, i2 ) in ( (select i1, i2 from ...),
    //                            (select i1, i2 from ...) ) 의 노드 변환 후,
    // 등등...
    // 위 질의문의 경우, 노드 변환시,
    // subqueryWrapper 등의 indirect node들이 연결되며,
    // value node의 left conversion이 존재할 경우 등을 위해
    // 실제로 참조하는 value node를 찾아야 함.
    // subquery를 포함한 노드변환 함수의 주석 참조

    for( sValueNode = sArgValueNode;
         ( (sValueNode->node.lflag & MTC_NODE_INDIRECT_MASK )
           == MTC_NODE_INDIRECT_TRUE ) ||
         ( (sValueNode->node.lflag & MTC_NODE_OPERATOR_MASK )
           == MTC_NODE_OPERATOR_SUBQUERY ) ;
         sValueNode = (qtcNode*)(sValueNode->node.arguments) ) ;

    if( sValueNode->node.leftConversion == NULL )
    {
        // Nothing To Do
    }
    else
    {
        sArgColumnNode->node.conversion = sValueNode->node.leftConversion;
        sValueNode->node.leftConversion = NULL;
    }

    IDE_TEST( qtc::estimateNodeWithoutArgument( aPredTrans->statement,
                                                aConditionNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredTransI::estimateLogicalNode( qmoPredTrans * aPredTrans,
                                    qtcNode      * aLogicalNode )
{

    IDU_FIT_POINT_FATAL( "qmoPredTransI::estimateLogicalNode::__FT__" );

    IDE_TEST( qtc::estimateNodeWithoutArgument( aPredTrans->statement,
                                                aLogicalNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmoPredTransI::setLogicalNCondition( qmoPredTrans * aPredTrans,
                                     qtcNode      * aCompareNode )
{
/***********************************************************************
 *
 * Description : keyRange 적용을 위한 노드 변환시 노드 변환전 비교연산자와
 *               대응되는 비교연산자와 논리연산자를 찾는다.
 *
 * Implementation :
 *
 *     1. =ANY        : I1 IN(1,2)    -> (I1=1)  OR  (I1=2)
 *     2. !=ANY       : I1 !=ANY(1,2) -> (I1!=1) OR  (I1!=2)
 *     3. >ANY, >=ANY : I1 >ANY(1,2)  -> (I1>1)  OR  (I1>2)
 *     4. <ANY, <=ANY : I1 <ANY(1,2)  -> (I1<1)  OR  (I1<2)
 *     5. =ALL        : I1 =ALL(1,2)  -> (I1=1)  AND (I1=2)
 *     6. !=ALL       : I1 !=ALL(1,2) -> (I1!=1) AND (I1!=2)
 *     7. >ALL, >=ALL : I1 >ALL(1,2)  -> (I1>1)  AND (I1>2)
 *     8. <ALL, <=ALL : I1 <ALL(1,2)  -> (I1<1)  AND (I1<2)
 *
 ***********************************************************************/

    //------------------------------------------
    // 대응되는 비교연산자를 찾는다.
    //------------------------------------------

    if( ( aCompareNode->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK )
        != MTC_NODE_GROUP_COMPARISON_TRUE )
    {
        // LIST형의 노드 변환 중 =, !=과 대응되는 비교연산자
        // 예: (i1,i2,i3)=(1,2,3), (i1,i2,i3)!=(1,2,3)
        switch ( aCompareNode->node.module->lflag &
                 ( MTC_NODE_OPERATOR_MASK ) )
        {
            case ( MTC_NODE_OPERATOR_EQUAL ) :
                // (=)
                idlOS::strcpy( aPredTrans->condition, "=" );
                idlOS::strcpy( aPredTrans->logical, "AND" );
                break;
            case ( MTC_NODE_OPERATOR_NOT_EQUAL ) :
                // (!=)
                idlOS::strcpy( aPredTrans->condition, "<>" );
                idlOS::strcpy( aPredTrans->logical, "AND" );
                break;
            default :
                aPredTrans->condition[0] = '\0';
                aPredTrans->logical[0]   = '\0';
                break;
        }

        // BUG-15816
        aPredTrans->isGroupComparison = ID_FALSE;
    }
    else
    {
        // one column과 LIST의 quantify 비교연산자와 대응되는 비교연산자
        switch ( aCompareNode->node.module->lflag &
                 ( MTC_NODE_OPERATOR_MASK | MTC_NODE_GROUP_MASK ) )
        {
            case ( MTC_NODE_OPERATOR_EQUAL | MTC_NODE_GROUP_ANY ) :
                // (=ANY)
                idlOS::strcpy( aPredTrans->condition, "=" );
                idlOS::strcpy( aPredTrans->logical, "OR" );
                break;
            case ( MTC_NODE_OPERATOR_NOT_EQUAL | MTC_NODE_GROUP_ANY ) :
                // (!=ANY)
                idlOS::strcpy( aPredTrans->condition, "<>" );
                idlOS::strcpy( aPredTrans->logical, "OR" );
                break;
            case ( MTC_NODE_OPERATOR_GREATER | MTC_NODE_GROUP_ANY ):
                // (>ANY)
                idlOS::strcpy( aPredTrans->condition, ">" );
                idlOS::strcpy( aPredTrans->logical, "OR" );
                break;
            case ( MTC_NODE_OPERATOR_GREATER_EQUAL | MTC_NODE_GROUP_ANY ) :
                // (>=ANY)
                idlOS::strcpy( aPredTrans->condition, ">=" );
                idlOS::strcpy( aPredTrans->logical, "OR" );
                break;
            case ( MTC_NODE_OPERATOR_LESS | MTC_NODE_GROUP_ANY ) :
                // (<ANY)
                idlOS::strcpy( aPredTrans->condition, "<" );
                idlOS::strcpy( aPredTrans->logical, "OR" );
                break;
            case ( MTC_NODE_OPERATOR_LESS_EQUAL | MTC_NODE_GROUP_ANY ) :
                // (<=ANY)
                idlOS::strcpy( aPredTrans->condition, "<=" );
                idlOS::strcpy( aPredTrans->logical, "OR" );
                break;
            case ( MTC_NODE_OPERATOR_EQUAL | MTC_NODE_GROUP_ALL ) :
                // (=ALL)
                idlOS::strcpy( aPredTrans->condition, "=" );
                idlOS::strcpy( aPredTrans->logical, "AND" );
                break;
            case ( MTC_NODE_OPERATOR_NOT_EQUAL | MTC_NODE_GROUP_ALL ) :
                // (!=ALL)
                idlOS::strcpy( aPredTrans->condition, "<>" );
                idlOS::strcpy( aPredTrans->logical, "AND" );
                break;
            case ( MTC_NODE_OPERATOR_GREATER | MTC_NODE_GROUP_ALL ):
                // (>ALL)
                idlOS::strcpy( aPredTrans->condition, ">" );
                idlOS::strcpy( aPredTrans->logical, "AND" );
                break;
            case ( MTC_NODE_OPERATOR_GREATER_EQUAL | MTC_NODE_GROUP_ALL ) :
                // (>=ALL)
                idlOS::strcpy( aPredTrans->condition, ">=" );
                idlOS::strcpy( aPredTrans->logical, "AND" );
                break;
            case ( MTC_NODE_OPERATOR_LESS | MTC_NODE_GROUP_ALL ) :
                // (<ALL)
                idlOS::strcpy( aPredTrans->condition, "<" );
                idlOS::strcpy( aPredTrans->logical, "AND" );
                break;
            case ( MTC_NODE_OPERATOR_LESS_EQUAL | MTC_NODE_GROUP_ALL ) :
                // (<=ALL)
                idlOS::strcpy( aPredTrans->condition, "<=" );
                idlOS::strcpy( aPredTrans->logical, "AND" );
                break;
            default :
                aPredTrans->condition[0] = '\0';
                aPredTrans->logical[0]   = '\0';
                break;
        }

        // BUG-15816
        aPredTrans->isGroupComparison = ID_TRUE;
    }
}

IDE_RC
qmoPred::nodeTransform4OneCond( qcStatement  * aStatement,
                                qtcNode      * aCompareNode,
                                qtcNode     ** aTransformNode,
                                qmsQuerySet  * aQuerySet )
{
/***********************************************************************
 *
 * Description : OR 노드 하위 하나의 비교연산자 노드 단위로 노드 변환
 *
 *     LIST 와 quantify 비교연산자에 대해서, keyRange를 적용하기 위해
 *     system level operator로 노드변환시킨다.
 *
 * Implementation :
 *
 *
 *
 ***********************************************************************/

    qtcNode      * sTransformNode = NULL;
    qmoPredTrans   sPredTrans;
    qtcNode      * sColumnNode;
    qtcNode      * sValueNode;

    IDU_FIT_POINT_FATAL( "qmoPred::nodeTransform4OneCond::__FT__" );

    // 노드 변형에 필요한 자료구조를 초기화한다.
    qmoPredTransI::initPredTrans( &sPredTrans,
                                  aStatement,
                                  aCompareNode,
                                  aQuerySet );

    // indexArgument 정보로 columnNode와 valueNode를 찾는다.
    if( aCompareNode->indexArgument == 0 )
    {
        sColumnNode = qtcNodeI::argumentNode( aCompareNode );
        sValueNode  = qtcNodeI::nextNode( sColumnNode );
    }
    else
    {
        sValueNode = qtcNodeI::argumentNode( aCompareNode );
        sColumnNode  = qtcNodeI::nextNode( sValueNode );
    }

    // value node는 list노드이거나 subquery node이다.
    // 단독 value 노드일 수는 없다.
    // 즉, where i1 in (1) 이라 하더라도 1은 list로 구성된다.
    if( qtcNodeI::isListNode( sValueNode ) == ID_TRUE )
    {
        IDE_TEST( nodeTransform4List( & sPredTrans,
                                      sColumnNode,
                                      sValueNode,
                                      & sTransformNode )
                  != IDE_SUCCESS );
    }
    else
    {
        // value node는 subquery 노드이다.
        IDE_TEST( nodeTransform4SubQ( & sPredTrans,
                                      aCompareNode,
                                      sColumnNode,
                                      sValueNode,
                                      & sTransformNode )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // 새로운 논리연산자를 생성하고, 하위에 변환된 노드를 연결한다.
    //------------------------------------------

    IDE_TEST( qmoPredTransI::makeLogicalNode( & sPredTrans,
                                              sTransformNode,
                                              aTransformNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::nodeTransform4List( qmoPredTrans * aPredTrans,
                             qtcNode      * aColumnNode,
                             qtcNode      * aValueNode,
                             qtcNode     ** aTransformNode )
{
/***********************************************************************
 *
 * Description : 하나의 리스트에 대해 노드 변환
 *
 * Implementation :
 *
 *    aValueNode는 list node이다.
 *    이 list node의 argument를 next로 순회하면서 각각에 대해 노드 변환을 한다.
 *
 *    aValueNode = ( a, b )에 대해,
 *      1) a, b가 각각 list일 경우
 *         - subqeury인 경우: multi column subquery라 부른다.
 *           (예) (i1, i2) in ( (select a1, a2 from ...), (select a1, a2 from ...) )
 *           (예) i1 in ( (select a1, a2 from ...), (select a1, a2 from ...) )
 *         - 일반 list인 경우
 *           (예) (i1, i2) in ( (1, 2), (3, 4) )
 *      2) a, b가 각각 one value일 경우
 *         - subquery인 경우: one column subquery라 부른다.
 *           (예) (i1, i2) in ( (select a1 from ...), (select a2 from ...) )
 *           (예) i1 in ( (select a1 from ...), (select a2 from ...) )
 *         - 일반 value인 경우
 *           (예) (i1, i2) in ( 1, 2 )
 *           (예) i1 in ( 1, 2 )
 *
 ************************************************************************/

    qtcNode            * sArgValueNode;
    qtcNode            * sTransformedNode;
    qtcNode            * sLastNode          = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::nodeTransform4List::__FT__" );

    *aTransformNode = NULL;

    // list의 첫번째 argument node 부터 노드 변환을 수행한다.
    sArgValueNode  = qtcNodeI::argumentNode( aValueNode );

    // column list, list of list value
    // column list, list of subquery value 처리
    if( qtcNodeI::isListNode( sArgValueNode ) == ID_TRUE ||
        isMultiColumnSubqueryNode( sArgValueNode ) == ID_TRUE )
    {
        while( sArgValueNode != NULL )
        {
            if( qtcNodeI::isListNode( sArgValueNode ) == ID_TRUE )
            {
                IDE_TEST( nodeTransform4List( aPredTrans,
                                              aColumnNode,
                                              sArgValueNode,
                                              & sTransformedNode )
                          != IDE_SUCCESS );
            }
            else if ( isMultiColumnSubqueryNode( sArgValueNode ) == ID_TRUE )
            {
                IDE_TEST( nodeTransform4OneRowSubQ( aPredTrans,
                                                    aColumnNode,
                                                    sArgValueNode,
                                                    & sTransformedNode )
                          != IDE_SUCCESS );
            }
            else
            {
                // sArgValueNode가 list이거나 subquery 였으면
                // 항상 이 둘 중의 하나여야 한다. 즉, value node이면 안된다.
                // (i1, i2) in ( (1,2), (1,(2,3)) ) 이런건 안된다는 의미임.
                // one column subquery도 올 수 없다.

                IDE_FT_ASSERT( 0 );
            }

            IDE_DASSERT( sTransformedNode != NULL );

            if( *aTransformNode == NULL )
            {
                *aTransformNode = sTransformedNode;
                sLastNode = sTransformedNode;
            }
            else
            {
                qtcNodeI::linkNode( sLastNode, sTransformedNode );
                sLastNode = sTransformedNode;
            }

            sArgValueNode  = qtcNodeI::nextNode( sArgValueNode );
        }
    }
    // column list, list value
    // one column, list value 처리
    else
    {
        if( qtcNodeI::isListNode( aColumnNode ) == ID_TRUE )
        {
            // (i1, i2) in (1,2) 같은 경우
            IDE_TEST( qmoPred::nodeTransformListColumnListValue( aPredTrans,
                                                                 aColumnNode,
                                                                 sArgValueNode,
                                                                 aTransformNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // i1 in (1) 같은 경우
            IDE_TEST( qmoPred::nodeTransformOneColumnListValue( aPredTrans,
                                                                aColumnNode,
                                                                sArgValueNode,
                                                                aTransformNode )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::nodeTransformListColumnListValue( qmoPredTrans * aPredTrans,
                                           qtcNode      * aColumnNode,
                                           qtcNode      * aValueNode,
                                           qtcNode     ** aTransformNode )
{
/***********************************************************************
 *
 *   Description :
 *         nodeTransform4List에서 value list를 처리하기 위한 함수
 *         (i1, i2) in (1,2) 같은 경우를 처리한다.
 *
 *   Implementation :
 *         column의 개수와 value의 개수가 같다고 가정하고,
 *         같지 않은 경우에는 validation시에 에러가 난다.
 *
 ***********************************************************************/

    qtcNode * sTransformedNode = NULL;
    qtcNode * sLastNode        = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::nodeTransformListColumnListValue::__FT__" );

    *aTransformNode = NULL;
    aColumnNode = qtcNodeI::argumentNode( aColumnNode );

    while( aValueNode != NULL && aColumnNode != NULL )
    {
        IDE_TEST( nodeTransformOneNode( aPredTrans,
                                        aColumnNode,
                                        aValueNode,
                                        & sTransformedNode )
                  != IDE_SUCCESS );

        IDE_DASSERT( sTransformedNode != NULL );

        if( *aTransformNode == NULL )
        {
            *aTransformNode = sTransformedNode;
            sLastNode = sTransformedNode;
        }
        else
        {
            qtcNodeI::linkNode( sLastNode, sTransformedNode );
            sLastNode = sTransformedNode;
        }

        aValueNode  = qtcNodeI::nextNode( aValueNode );
        aColumnNode = qtcNodeI::nextNode( aColumnNode );
    }

    // AND 논리연산자를 생성하고,
    // AND 하위에 변환된 노드를 연결한다.
    IDE_TEST( qmoPredTransI::makeAndNode( aPredTrans,
                                          * aTransformNode,
                                          aTransformNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::nodeTransformOneColumnListValue( qmoPredTrans * aPredTrans,
                                          qtcNode      * aColumnNode,
                                          qtcNode      * aValueNode,
                                          qtcNode     ** aTransformNode )
{
/***********************************************************************
 *
 *   Description :
 *         nodeTransform4List에서 value list를 처리하기 위한 함수
 *         i1 in (1) 같은 경우를 처리한다.
 *
 *   Implementation :
 *         column은 하나이다.
 *
 ***********************************************************************/

    qtcNode * sTransformedNode = NULL;
    qtcNode * sLastNode        = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::nodeTransformOneColumnListValue::__FT__" );

    *aTransformNode = NULL;

    while( aValueNode != NULL )
    {
        IDE_TEST( nodeTransformOneNode( aPredTrans,
                                        aColumnNode,
                                        aValueNode,
                                        & sTransformedNode )
                  != IDE_SUCCESS );

        IDE_DASSERT( sTransformedNode != NULL );

        if( *aTransformNode == NULL )
        {
            *aTransformNode = sTransformedNode;
            sLastNode = sTransformedNode;
        }
        else
        {
            qtcNodeI::linkNode( sLastNode, sTransformedNode );
            sLastNode = sTransformedNode;
        }

        aValueNode  = qtcNodeI::nextNode( aValueNode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::nodeTransformOneNode( qmoPredTrans * aPredTrans,
                               qtcNode      * aColumnNode,
                               qtcNode      * aValueNode,
                               qtcNode     ** aTransformNode )
{
/***********************************************************************
 *
 * Description : 하나의 column노드와 value노드에 대한 노드 변환 수행
 *
 *     인자로 받은 column node와 value node를 복사하고,
 *     노드변환에 알맞은 비교연산자(system level operator) 노드를 만들어서
 *     복사한 노드를 연결하여 새로운 predicate을 만든다.
 *
 *     [ column node의 conversion 정보 설정 ]
 *     quantify 비교연산자는 column에 대한 conversion 정보를
 *     value node의 leftConversion에 가지고 있다.
 *     예: i1(integer) in ( 1, 2, 3.5, ? )
 *         위의 예에서 value쪽의 3.5와 ?인 경우,
 *         leftConversion에 column conversion의 정보를 가지고 있다.
 *     노드 변환이 수행되면 system level operator를 사용하기 때문에
 *     column node가 conversion 정보를 가지고 있어야 한다.
 *     복사된 column node에는 conversion 정보가 없기 때문에,
 *     value node의 leftConversion 정보를 보고
 *     column node에 대한 conversion 정보를 만들어야 한다.
 *
 * Implementation :
 *
 *     1. 인자로 받은 column node와 value node를 복사.
 *     2. value node의 leftConversion 정보를 보고,
 *        column node의 conversion 정보 설정.
 *     3. 노드변환에 알맞는 비교연산자 노드를 생성하고,
 *        1의 노드를 하위노드로 연결
 *        (1) 비교연산자 노드 estimateNode
 *        (2) 비교연산자 노드 indexArgument 설정.
 *
 ***********************************************************************/

    qtcNode        * sConditionNode;
    qtcNode        * sColumnNode;
    qtcNode        * sValueNode;

    IDU_FIT_POINT_FATAL( "qmoPred::nodeTransformOneNode::__FT__" );

    // multi column subquery에 대해서는 이 함수에 들어와서는 안된다.
    // 이 경우는 nodeTransform4OneRowSubQ() 함수를 타야 한다.
    if( isMultiColumnSubqueryNode( aValueNode ) == ID_TRUE )
    {
        IDE_DASSERT( 0 );
    }

    //-----------------------------------
    // column, value node 복사
    //-----------------------------------
    IDE_TEST( qmoPredTransI::copyNode( aPredTrans,
                                       aColumnNode,
                                       & sColumnNode )
              != IDE_SUCCESS );

    // fix BUG-32079
    // one column subquery node has MTC_NODE_INDIRECT_TRUE flag and may have indirect conversion node at arguments.
    // ex)
    // var a integer;
    // create table a ( dummy char(1) primary key );
    // prepare select 'a' from a where dummy in ( :a, (select dummy from a) );
    IDE_TEST( qmoPredTransI::copyNode( aPredTrans,
                                       aValueNode,
                                       & sValueNode )
              != IDE_SUCCESS );

    qtcNodeI::linkNode( sColumnNode, sValueNode );

    // quantify 비교연산자에 대응되는 새로운 비교연산자를 만든다.
    IDE_TEST( qmoPredTransI::makeConditionNode( aPredTrans,
                                                sColumnNode,
                                                & sConditionNode )
              != IDE_SUCCESS );

    *aTransformNode = sConditionNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::nodeTransform4OneRowSubQ( qmoPredTrans * aPredTrans,
                                   qtcNode      * aColumnNode,
                                   qtcNode      * aValueNode,
                                   qtcNode     ** aTransformNode )
{
/***********************************************************************
 *
 * Description : value node가 one row SUBQUERY 인 경우의 노드 변환
 *
 * Implementation :
 *
 *     예) (i1,i2) in ( ( select a1, a2 from ... ), ( select a1, a2 from ... ) )
 *
 *    [IN]
 *      |
 *    [LIST]--------------->[LIST]
 *      |                     |
 *    [i1]-->[i2]           [SUBQ-a]-------->[SUBQ-b]
 *                            |                 |
 *                          [i1]->[i2]        [i1]->[i2]
 *
 *   [OR]
 *    |
 *   [AND]----------------------------->[AND]
 *    |                                   |
 *   [=]------------>[=]                 [=]-------------->[=]
 *    |               |                   |                 |
 *   [i1]->[ind]     [i2]->[ind]         [i1]->[ind]       [i2]->[ind]
 *           |               |                  |                  |
 *         [SUBQ-a]        [a.a2]             [SUBQ-b]           [b.a2]
 *
 ***********************************************************************/

    qtcNode * sFirstNode;
    qtcNode * sLastNode;
    qtcNode * sConditionNode;
    qtcNode * sIndNode;
    qtcNode * sCopiedColumnNode;
    qtcNode * sIteratorColumnNode;
    qtcNode * sIteratorValueNode;

    IDU_FIT_POINT_FATAL( "qmoPred::nodeTransform4OneRowSubQ::__FT__" );

    *aTransformNode = NULL;
    sFirstNode = NULL;
    sIteratorColumnNode = qtcNodeI::argumentNode( aColumnNode );
    sIteratorValueNode  = aValueNode;

    while( sIteratorColumnNode != NULL )
    {
        IDE_TEST( qmoPredTransI::copyNode( aPredTrans,
                                           sIteratorColumnNode,
                                           & sCopiedColumnNode )
                  != IDE_SUCCESS );

        IDE_TEST( qmoPredTransI::makeIndirectNode( aPredTrans,
                                                   sIteratorValueNode,
                                                   & sIndNode )
                  != IDE_SUCCESS );

        qtcNodeI::linkNode( sCopiedColumnNode, sIndNode );

        IDE_TEST( qmoPredTransI::makeConditionNode( aPredTrans,
                                                    sCopiedColumnNode,
                                                    & sConditionNode )
                  != IDE_SUCCESS );

        if( sFirstNode == NULL )
        {

            // fix BUG-13939
            // keyRange 생성시, in subquery or subquery keyRange일 경우,
            // subquery를 수행할 수 있도록 하기 위해
            // 노드변환후, subquery node가 연결된 비교연산자노드에
            // 그 정보를 저장한다.
            sConditionNode->lflag &= ~QTC_NODE_SUBQUERY_RANGE_MASK;
            sConditionNode->lflag |= QTC_NODE_SUBQUERY_RANGE_TRUE;

            sFirstNode = sConditionNode;
            sLastNode = sConditionNode;

            // sIteratorValueNode는 처음에 subquery를 가리키는 노드였다.
            // 두번째 컬럼부터는 subquery의 두번째 컬럼 노드를 가리켜야 한다.
            // 위 그림 참조.
            sIteratorValueNode = qtcNodeI::argumentNode( sIteratorValueNode );
        }
        else
        {
            qtcNodeI::linkNode( sLastNode, sConditionNode );
            sLastNode = sConditionNode;
        }

        sIteratorColumnNode = qtcNodeI::nextNode( sIteratorColumnNode );
        sIteratorValueNode  = qtcNodeI::nextNode( sIteratorValueNode );
    }

    IDE_TEST( qmoPredTransI::makeAndNode( aPredTrans,
                                          sFirstNode,
                                          aTransformNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::nodeTransform4SubQ( qmoPredTrans * aPredTrans,
                             qtcNode      * aCompareNode,
                             qtcNode      * aColumnNode,
                             qtcNode      * aValueNode,
                             qtcNode     ** aTransformNode )
{
/***********************************************************************
 *
 * Description : value node가 SUBQUERY 인 경우의 노드 변환
 *
 * Implementation :
 *
 *     예) i1 in ( select a1 from ... )
 *
 *        [IN]                        [=]
 *         |                           |
 *        [i1]-->[subquery]   ==>     [i1]-->[Wrapper]
 *                   |                           |
 *                  [a1]                    [subquery]
 *                                               |
 *                                             [a1]
 *
 *     예) (i1,i2) in ( select a1, a2 from ... )
 *
 *     [IN]                          [=] -------------------> [=]
 *      |                             |                        |
 *    [LIST]--->[subquery]           [i1] ----> [Wrapper]     [i2]--> [ind]
 *      |           |          ==>                 |                    |
 *    [i1]-->[i2] [a1]-->[a2]                  [Subquery]               |
 *                                                 |                    |
 *                                                [a1] -------------> [a2]
 *
 ***********************************************************************/

    idBool           sIsListColumn;
    qtcNode        * sSubqueryWrapperNode = NULL;
    qtcNode        * sConditionNode;
    qtcNode        * sLastNode;
    qtcNode        * sNewColumnNode;
    qtcNode        * sIndNode;

    IDU_FIT_POINT_FATAL( "qmoPred::nodeTransform4SubQ::__FT__" );

    // To Fix BUG-13308
    if( qtcNodeI::isListNode( aColumnNode ) == ID_TRUE )
    {
        sIsListColumn = ID_TRUE;
    }
    else
    {
        sIsListColumn = ID_FALSE;
    }

    if ( ( sIsListColumn == ID_TRUE ) &&
         ( ( aCompareNode->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK )
           == MTC_NODE_GROUP_COMPARISON_FALSE ) )
    {
        IDE_TEST( qmoPred::nodeTransform4OneRowSubQ( aPredTrans,
                                                     aColumnNode,
                                                     aValueNode,
                                                     aTransformNode )
                  != IDE_SUCCESS );
    }
    else
    {
        if( sIsListColumn == ID_TRUE )
        {
            aColumnNode = qtcNodeI::argumentNode( aColumnNode );
        }
        else
        {
            // nothing to do
        }

        //------------------------------------------
        // column node 복사
        //------------------------------------------
        IDE_TEST( qmoPredTransI::copyNode( aPredTrans,
                                           aColumnNode,
                                           & sNewColumnNode )
                  != IDE_SUCCESS );

        //------------------------------------------
        // value node에 대한 subqueryWrapperNode 생성
        //------------------------------------------
        IDE_TEST( qmoPredTransI::makeSubQWrapperNode( aPredTrans,
                                                      aValueNode,
                                                      & sSubqueryWrapperNode )
                  != IDE_SUCCESS );

        qtcNodeI::linkNode( sNewColumnNode, sSubqueryWrapperNode );

        // 노드변환전 비교연산자에 대응되는 새로운 비교연산자를 만든다.
        IDE_TEST( qmoPredTransI::makeConditionNode( aPredTrans,
                                                    sNewColumnNode,
                                                    & sConditionNode )
                  != IDE_SUCCESS );

        // fix BUG-13939
        // keyRange 생성시, in subquery or subquery keyRange일 경우,
        // subquery를 수행할 수 있도록 하기 위해
        // 노드변환후, subquery node가 연결된 비교연산자노드에
        // 그 정보를 저장한다.
        sConditionNode->lflag &= ~QTC_NODE_SUBQUERY_RANGE_MASK;
        sConditionNode->lflag |= QTC_NODE_SUBQUERY_RANGE_TRUE;

        *aTransformNode = sConditionNode;
        sLastNode = sConditionNode;

        if( sIsListColumn == ID_TRUE )
        {
            // 인자로 넘어온 컬럼노드가 LIST인 경우
            // 나머지 컬럼에 대한 노드변환 수행

            aColumnNode = qtcNodeI::nextNode( aColumnNode );
            aValueNode  = qtcNodeI::nextNode(
                qtcNodeI::argumentNode( aValueNode ) );

            while( aColumnNode != NULL )
            {
                //------------------------------------------
                // column node는 복사하고,
                // subquery part쪽은 InDirect 하위의 argument로 연결한다.
                //------------------------------------------

                // column node 처리
                IDE_TEST( qmoPredTransI::copyNode( aPredTrans,
                                                   aColumnNode,
                                                   & sNewColumnNode )
                          != IDE_SUCCESS );

                IDE_TEST( qmoPredTransI::makeIndirectNode( aPredTrans,
                                                           aValueNode,
                                                           & sIndNode )
                          != IDE_SUCCESS );

                qtcNodeI::linkNode( sNewColumnNode, sIndNode );

                // 노드변환전 비교연산자에 대응되는 새로운 비교연산자를 만든다.
                IDE_TEST( qmoPredTransI::makeConditionNode( aPredTrans,
                                                            sNewColumnNode,
                                                            & sConditionNode )
                          != IDE_SUCCESS );

                qtcNodeI::linkNode( sLastNode, sConditionNode );
                sLastNode = sConditionNode;

                aColumnNode = qtcNodeI::nextNode( aColumnNode );
                aValueNode  = qtcNodeI::nextNode( aValueNode );
            }

            IDE_TEST( qmoPredTransI::makeAndNode( aPredTrans,
                                                  *aTransformNode,
                                                  aTransformNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // 인자로 넘어온 컬럼노드가 LIST가 아닌 column인 경우
            // Nothing To Do
        }
    }

    // fix BUG-13969
    // subquery에 대한 노드 변환시,
    // subquery node에 대한 dependency는 포함하지 않는다.
    // column node의 dependency 정보만 상위노드로 올린다.
    //
    // 예: SELECT COUNT(*) FROM T1
    //      WHERE (I1,I2) IN ( SELECT I1, I2 FROM T2 );
    //           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    //  위 IN predicate의 dependency는 T1이다.
    //  노드변환시, 이 dependency정보가 (T1,T2)가 되므로,
    //  이 dependency정보를 원래 dependency정보인 (T1)으로 복원한다.
    //  컬럼노드의 dependency를 비교연산자의 dependency로 설정한다.
    //  노드변환시, column 노드는 비교연산자노드의 argument에 연결된다.

    // BUG-36575
    // subquery에 의존성이 없는 외부참조컬럼에 in-subquery key range가
    // 생성되더라도 외부참조컬럼의 값이 바뀌는 경우 rebuild할 수 있도록
    // column에 dependency를 설정한다.
    //
    // 예: select
    //        (select count(*) from t1 where i1 in (select i1 from t2 where i1=t3.i1))
    //                                       ^^^
    //     from t3;
    //
    // 위 i1의 dependency는 (t1,t3)이다.
    
    if( sIsListColumn == ID_TRUE )
    {
        sConditionNode = (qtcNode*)(((*aTransformNode)->node).arguments);
    }
    else
    {
        sConditionNode = NULL;
    }

    if ( sSubqueryWrapperNode != NULL )
    {
        for( sConditionNode = sConditionNode;
             sConditionNode != NULL;
             sConditionNode = (qtcNode*)(sConditionNode->node.next) )
        {
            qtc::dependencyOr( 
                &(((qtcNode*)(sConditionNode->node.arguments))->depInfo),
                &(sSubqueryWrapperNode->depInfo),
                &(sConditionNode->depInfo) );
        }

        // subquery의 outer dependency를 or한다.
        qtc::dependencyOr( 
            &((qtcNode*)(((*aTransformNode)->node).arguments))->depInfo,
            &(sSubqueryWrapperNode->depInfo),
            &((*aTransformNode)->depInfo) );
    }
    else
    {
        for( sConditionNode = sConditionNode;
             sConditionNode != NULL;
             sConditionNode = (qtcNode*)(sConditionNode->node.next) )
        {
            qtc::dependencySetWithDep(
                &(sConditionNode->depInfo),
                &(((qtcNode*)(sConditionNode->node.arguments))->depInfo) );
        }

        qtc::dependencySetWithDep(
            &((*aTransformNode)->depInfo),
            &((qtcNode*)(((*aTransformNode)->node).arguments))->depInfo );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::setIndexArgument( qtcNode       * aNode,
                           qcDepInfo     * aDependencies )
{
/***********************************************************************
 *
 * Description : indexArgument 설정
 *
 *     indexable predicate에 대해, indexArgument를 설정한다.
 *
 *     indexArgument는 다음과 같은 경우에 설정하게 된다.
 *     (1) indexable predicate 판단시.
 *     (2) Index Nested Loop Join predicate을 selection graph에 내릴때.
 *         이 경우는, indexable join predicate판단시 indexArgument가
 *         설정되기는 하지만, 이때의 정보는 정확한 정보가 아니기때문에,
 *         join method가 확정되고, selection graph로 join predicate을
 *         내릴때, 정확한 정보를 다시 설정하게 된다.
 *         또, Anti Outer Nested Loop Join시, AOJN 노드에 join predicate을
 *         연결할때도 indexArgument가 변경되어야 한다.
 *
 *     이 함수를 호출하는 곳은
 *     (1) qmoPred::isIndexable()
 *     (2) qmoPred::makeJoinPushDownPredicate()
 *     (3) qmoPred::makeNonJoinPushDownPredicate() 이며,
 *     indexArgument 설정하는 함수를 공통으로 사용하기 위해서,
 *     qtcNode를 인자로 받는다.
 *
 * Implementation :
 *
 *     dependencies 정보로 비교연산자 노드에 indexArgument정보를 설정.
 *
 *     (인자로 받을 수 있는 노드의 형태는 다음과 같다.)
 *
 *     (1)  OR         (2)  OR                        (3) 비교연산자
 *          |               |                                 |
 *       비교연산자     비교연산자->...->비교연산자
 *          |               |                |
 *
 ***********************************************************************/

    qtcNode * sNode = aNode;
    qtcNode * sCurNode;

    IDU_FIT_POINT_FATAL( "qmoPred::setIndexArgument::__FT__" );

    //------------------------------------------
    // 비교연산자 노드를 찾는다.
    // 비교연산자 노드를 찾기위해, 논리연산자(OR)노드 skip
    //------------------------------------------
    if( ( sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
           == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sNode = (qtcNode *)(sNode->node.arguments);

        //------------------------------------------
        // 최상위 노드가 OR 노드인 경우,
        // OR 노드 하위에 같은 컬럼의 비교연산자가 여러개 올 수 있다.
        //------------------------------------------
        for( sCurNode = sNode ;
             sCurNode != NULL;
             sCurNode = (qtcNode *)(sCurNode->node.next) )
        {
            //--------------------------------------
            // indexArgument 설정
            //--------------------------------------

            // dependencies정보로 컬럼을 찾아, indexArgument정보를 설정한다.
            if( qtc::dependencyEqual(
                    & ((qtcNode *)(sCurNode->node.arguments))->depInfo,
                    aDependencies ) == ID_TRUE )
            {
                sCurNode->indexArgument = 0;
            }
            else
            {
                sCurNode->indexArgument = 1;
            }
        }
    }
    else
    {
        //--------------------------------------
        // indexArgument 설정
        //--------------------------------------

        // dependencies정보로 컬럼을 찾아, indexArgument정보를 설정한다.
        if( qtc::dependencyEqual(
                & ((qtcNode *)(sNode->node.arguments))->depInfo,
                aDependencies ) == ID_TRUE )
        {
            sNode->indexArgument = 0;
        }
        else
        {
            sNode->indexArgument = 1;
        }
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::linkPred4ColumnID( qmoPredicate * aDestPred,
                            qmoPredicate * aSourcePred )
{
/***********************************************************************
 *
 * Description :  컬럼별로 연결관계를 만든다.
 *
 *     qmoPred::addNonJoinablePredicate()과 qmoPred::relocatePredicate()
 *     에서 이 함수를 호출한다.
 *
 * Implementation :
 *
 *     1. dest predicate에 source predicate과 동일 컬럼이 있는 경우,
 *        동일 컬럼의 마지막 qmoPredicate->more에 source predicate 연결
 *
 *     2. dest predicate에 source predicate과 동일 컬럼이 없는 경우,
 *        dest predicate 의 마지막 qmoPredicate->next에 source predicate연결
 *
 ***********************************************************************/

    idBool         sIsSameColumnID = ID_FALSE;
    qmoPredicate * sPredicate;
    qmoPredicate * sMorePredicate;

    IDU_FIT_POINT_FATAL( "qmoPred::linkPred4ColumnID::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aDestPred != NULL );
    IDE_DASSERT( aSourcePred != NULL );

    for( sPredicate = aDestPred;
         sPredicate != NULL;
         sPredicate = sPredicate->next )
    {
        if( sPredicate->id == aSourcePred->id )
        {
            sIsSameColumnID = ID_TRUE;
            break;
        }
        else
        {
            // Nothing To Do
        }
    }

    /* BUG-48526 */
    if ( ( ( aSourcePred->flag & QMO_PRED_OR_VALUE_INDEX_MASK )
           == QMO_PRED_OR_VALUE_INDEX_TRUE ) &&
         ( sIsSameColumnID == ID_TRUE ) )
    {
        aSourcePred->id = QMO_COLUMNID_NON_INDEXABLE;
        sIsSameColumnID = ID_FALSE;
    }

    if( sIsSameColumnID == ID_TRUE )
    {
        // 동일 컬럼이 존재하는 경우

        for( sMorePredicate = sPredicate;
             sMorePredicate->more != NULL;
             sMorePredicate = sMorePredicate->more ) ;

        sMorePredicate->more = aSourcePred;
        // 연결된 predicate의 next 연결관계를 끊는다.
        sMorePredicate->more->next = NULL;
    }
    else // ( sIsSameColumnID == ID_FALSE )
    {
        // 동일 컬럼이 존재하지 않는 경우

        for( sPredicate = aDestPred;
             sPredicate->next != NULL;
             sPredicate = sPredicate->next ) ;

        sPredicate->next = aSourcePred;
        // 연결된 predicate의 next 연결관계를 끊는다.
        sPredicate->next->next = NULL;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::removeIndexableSubQTip( qtcNode      * aNode  )
{
/***********************************************************************
 *
 * Description : indexable subqueryTip 삭제
 *
 *     IN(subquery) 또는 subquery keyRange 최적화 팁 적용 predicate이
 *     keyRange로 추출되지 못하고, filter로 빠지는 경우,
 *     설정되어 있는 팁 정보를 삭제하고,
 *     store and search 최적화 팁을 적용하도록 수정한다.
 *     subquery 최적화시, 이런 경우를 대비해  store and search 팁에 관한
 *     정보를 미리 설정해 놓았으므로, flag만 바꾸면 된다.
 *
 * Implementation :
 *
 *     1. indexArgument 정보로 subquery node를 찾는다.
 *     2. 해당 subquery 최적화 팁 정보를 삭제하고,
 *        store and search 최적화 팁 정보 설정.
 *
 ***********************************************************************/

    qtcNode  * sCompareNode;
    qtcNode  * sNode;
    qmgPROJ  * sPROJGraph;

    IDU_FIT_POINT_FATAL( "qmoPred::removeIndexableSubQTip::__FT__" );

    if( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
        == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sCompareNode = (qtcNode *)(aNode->node.arguments);
    }
    else
    {
        sCompareNode = aNode;
    }

    //------------------------------------------
    // subquery node를 찾는다.
    //------------------------------------------

    // 예) i1 = (select i1 from t2 where i1 = 1 );
    //     i1 = (select i1 from ...) or i1 = (select i1 fro ... )

    while( sCompareNode != NULL )
    {
        // fix BUG-11485
        // non-indexable, indexable predicate이
        // 모두 이 코드로 들어올 수 있으므로
        // indexArgument로 value node로 찾을 경우,
        // (1) indexArgument가 설정되지 않아 subquery node를 못 찾을 수 있고,
        // (2) indexArgument가 초기화 되지 않은 경우도 있을수 있어,
        // 비교연산자 하위 노드들의 연결리스트를 따라가면서,
        // subquery tip 적용을 제거한다.
        // in subquery/subquery keyRange가 적용된 predicate은
        // i1 OP subquery 와 같은 형태이므로
        // 이와 같이 처리하는것이 가능.
        sNode = (qtcNode*)(sCompareNode->node.arguments);

        while( sNode != NULL )
        {
            //------------------------------------------
            // subqueryTipFlag에
            // in subquery keyRange/subuqery keyRange에 대한 flag가 설정된경우,
            // indexable Subquery Tip을 제거하고,
            // store and search 최적화 팁을 적용하도록 flag를 재설정한다.
            // 단, [IN(=ANY), NOT IN(!=ALL), =ALL, !=ANY
            // 저장방식의 제약사항]으로
            //     인해 store and search 최적화를 수행할 수 없는 경우는
            //     (qmoSubquery::storeAndSearch() 함수 주석 참조)
            //     subquery 최적화 팁이 존재하지 않는걸로 설정한다.
            //-----------------------------------------

            if( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_SUBQUERY )
            {
                sPROJGraph = (qmgPROJ *)(sNode->subquery->myPlan->graph);

                if( ( ( sPROJGraph->subqueryTipFlag &
                        QMG_PROJ_SUBQUERY_TIP_MASK )
                      == QMG_PROJ_SUBQUERY_TIP_KEYRANGE )
                    ||( ( sPROJGraph->subqueryTipFlag &
                          QMG_PROJ_SUBQUERY_TIP_MASK )
                        == QMG_PROJ_SUBQUERY_TIP_IN_KEYRANGE ) )
                {
                    // fix PR-8936
                    // store and search를 적용할 수 없는 경우,
                    if( ( sPROJGraph->subqueryTipFlag &
                          QMG_PROJ_SUBQUERY_STORENSEARCH_MASK )
                        == QMG_PROJ_SUBQUERY_STORENSEARCH_NONE )
                    {
                        // store and search 최적화 팁을 적용할 수 없는 경우
                        // [ IN(=ANY), NOT IN(!=ALL), =ALL, !=ANY
                        //   저장방식의 제약사항 ]
                        // qmoSubquery::storeAndSearch() 함수 주석 참조

                        sPROJGraph->subqueryTipFlag &=
                            ~QMG_PROJ_SUBQUERY_TIP_MASK;
                        sPROJGraph->subqueryTipFlag |=
                            QMG_PROJ_SUBQUERY_TIP_NONE;
                    }
                    else
                    {
                        sPROJGraph->subqueryTipFlag &=
                            ~QMG_PROJ_SUBQUERY_TIP_MASK;
                        sPROJGraph->subqueryTipFlag
                            |= QMG_PROJ_SUBQUERY_TIP_STORENSEARCH;
                    }
                }
                else
                {
                    // Nothing To Do
                }
            }

            sNode = (qtcNode *)(sNode->node.next);
        }

        sCompareNode = (qtcNode *)(sCompareNode->node.next);
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoPred::copyPredicate4Partition( qcStatement   * aStatement,
                                  qmoPredicate  * aSource,
                                  qmoPredicate ** aResult,
                                  UShort          aSourceTable,
                                  UShort          aDestTable,
                                  idBool          aCopyVariablePred )
{
    qmoPredicate *sNextIter;
    qmoPredicate *sMoreIter;
    qmoPredicate *sNextHead;
    qmoPredicate *sMoreHead;
    qmoPredicate *sNextLast;
    qmoPredicate *sMoreLast;
    qmoPredicate *sNewPred;

    IDU_FIT_POINT_FATAL( "qmoPred::copyPredicate4Partition::__FT__" );

    if( aSource != NULL )
    {
        sNextHead = NULL;
        sNextLast = NULL;
        for( sNextIter = aSource;
             sNextIter != NULL;
             sNextIter = sNextIter->next )
        {
            sMoreHead = NULL;
            sMoreLast = NULL;
            for( sMoreIter = sNextIter;
                 sMoreIter != NULL;
                 sMoreIter = sMoreIter->more )
            {
                if( ( sMoreIter->node->lflag & QTC_NODE_SUBQUERY_MASK )
                    == QTC_NODE_SUBQUERY_EXIST )
                {
                    // Nothing to do.
                }
                else
                {
                    // variable predicate copy가 FALSE이고,
                    // predicate이 variable이면 copy하지 않음
                    if( ( aCopyVariablePred == ID_FALSE ) &&
                        QMO_PRED_IS_VARIABLE( sMoreIter ) == ID_TRUE )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        IDE_TEST( copyOnePredicate4Partition( aStatement,
                                                              sMoreIter,
                                                              &sNewPred,
                                                              aSourceTable,
                                                              aDestTable )
                                  != IDE_SUCCESS );

                        linkToMore( &sMoreHead, &sMoreLast, sNewPred );
                    }
                }
            }

            if( sMoreHead != NULL )
            {
                linkToNext( &sNextHead, &sNextLast, sMoreHead );
            }
            else
            {
                // Nothing to do.
            }
        }
        *aResult = sNextHead;
    }
    else
    {
        *aResult = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::copyOnePredicate4Partition( qcStatement   * aStatement,
                                     qmoPredicate  * aSource,
                                     qmoPredicate ** aResult,
                                     UShort          aSourceTable,
                                     UShort          aDestTable )
{
    IDU_FIT_POINT_FATAL( "qmoPred::copyOnePredicate4Partition::__FT__" );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoPredicate ),
                                             (void**)aResult )
              != IDE_SUCCESS );

    idlOS::memcpy( *aResult, aSource, ID_SIZEOF( qmoPredicate ) );

    IDE_TEST( qtc::cloneQTCNodeTree4Partition( QC_QMP_MEM( aStatement ),
                                               aSource->node,
                                               &(*aResult)->node,
                                               aSourceTable,
                                               aDestTable,
                                               ID_FALSE )
              != IDE_SUCCESS );

    // conversion node를 새로 만들어 준다.
    // BUG-27291
    // re-estimate가 가능하도록 aStatement를 인자에 추가한다.
    IDE_TEST( qtc::estimate( (*aResult)->node,
                             QC_SHARED_TMPLATE(aStatement),
                             aStatement,
                             NULL,
                             NULL,
                             NULL )
              != IDE_SUCCESS );

    (*aResult)->next = NULL;
    (*aResult)->more = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::deepCopyPredicate( iduVarMemList * aMemory,
                            qmoPredicate  * aSource,
                            qmoPredicate ** aResult )
{
    qmoPredicate *sNextIter;
    qmoPredicate *sMoreIter;
    qmoPredicate *sNextHead;
    qmoPredicate *sMoreHead;
    qmoPredicate *sNextLast;
    qmoPredicate *sMoreLast;
    qmoPredicate *sNewPred;

    IDU_FIT_POINT_FATAL( "qmoPred::deepCopyPredicate::__FT__" );

    if( aSource != NULL )
    {
        sNextHead = NULL;
        sNextLast = NULL;
        for( sNextIter = aSource;
             sNextIter != NULL;
             sNextIter = sNextIter->next )
        {
            sMoreHead = NULL;
            sMoreLast = NULL;
            for( sMoreIter = sNextIter;
                 sMoreIter != NULL;
                 sMoreIter = sMoreIter->more )
            {
                IDE_TEST( copyOnePredicate( aMemory, sMoreIter, &sNewPred )
                          != IDE_SUCCESS );

                linkToMore( &sMoreHead, &sMoreLast, sNewPred );
            }

            linkToNext( &sNextHead, &sNextLast, sMoreHead );
        }
        *aResult = sNextHead;
    }
    else
    {
        *aResult = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmoPred::linkToMore( qmoPredicate ** aHead,
                     qmoPredicate ** aLast,
                     qmoPredicate  * aNew )
{
    if( *aHead == NULL )
    {
        *aHead = aNew;
    }
    else
    {
        IDE_DASSERT( *aLast != NULL );
        (*aLast)->more = aNew;
    }
    *aLast = aNew;
}

void
qmoPred::linkToNext( qmoPredicate ** aHead,
                     qmoPredicate ** aLast,
                     qmoPredicate  * aNew )
{
    if( *aHead == NULL )
    {
        *aHead = aNew;
    }
    else
    {
        IDE_DASSERT( *aLast != NULL );
        (*aLast)->next = aNew;
    }
    *aLast = aNew;
}

IDE_RC
qmoPred::copyOnePredicate( iduVarMemList * aMemory,
                           qmoPredicate  * aSource,
                           qmoPredicate ** aResult )
{
    IDU_FIT_POINT_FATAL( "qmoPred::copyOnePredicate::__FT__" );

    IDE_TEST( aMemory->alloc( ID_SIZEOF( qmoPredicate ),
                              (void**)aResult )
              != IDE_SUCCESS );

    idlOS::memcpy( *aResult, aSource, ID_SIZEOF( qmoPredicate ) );

    IDE_TEST( qtc::cloneQTCNodeTree( aMemory,
                                     aSource->node,
                                     &(*aResult)->node,
                                     ID_FALSE,
                                     ID_FALSE,
                                     ID_FALSE,
                                     ID_FALSE )
              != IDE_SUCCESS );

    (*aResult)->next = NULL;
    (*aResult)->more = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// fix BUG-19211
// constant predicate까지 복사한다.
IDE_RC
qmoPred::copyOneConstPredicate( iduVarMemList * aMemory,
                                qmoPredicate  * aSource,
                                qmoPredicate ** aResult )
{
    IDU_FIT_POINT_FATAL( "qmoPred::copyOneConstPredicate::__FT__" );

    IDE_TEST( aMemory->alloc( ID_SIZEOF( qmoPredicate ),
                              (void**)aResult )
              != IDE_SUCCESS );

    idlOS::memcpy( *aResult, aSource, ID_SIZEOF( qmoPredicate ) );

    IDE_TEST( qtc::cloneQTCNodeTree( aMemory,
                                     aSource->node,
                                     &(*aResult)->node,
                                     ID_FALSE,
                                     ID_FALSE,
                                     ID_TRUE,
                                     ID_FALSE )
              != IDE_SUCCESS );

    (*aResult)->next = NULL;
    (*aResult)->more = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::createPredicate( iduVarMemList * aMemory,
                          qtcNode       * aNode,
                          qmoPredicate ** aNewPredicate )
{
    IDU_FIT_POINT_FATAL( "qmoPred::createPredicate::__FT__" );

    IDE_TEST( aMemory->alloc( ID_SIZEOF( qmoPredicate ),
                              (void**)aNewPredicate )
              != IDE_SUCCESS );

    (*aNewPredicate)->idx = 0;
    (*aNewPredicate)->flag = QMO_PRED_CLEAR;
    (*aNewPredicate)->id = 0;
    (*aNewPredicate)->node = aNode;
    (*aNewPredicate)->mySelectivity    = 1;
    (*aNewPredicate)->totalSelectivity = 1;
    (*aNewPredicate)->mySelectivityOffset = QMO_SELECTIVITY_OFFSET_NOT_USED;
    (*aNewPredicate)->totalSelectivityOffset = QMO_SELECTIVITY_OFFSET_NOT_USED;
    (*aNewPredicate)->next = NULL;
    (*aNewPredicate)->more = NULL;

    // transitive predicate인 경우 설정한다.
    if ( (aNode->lflag & QTC_NODE_TRANS_PRED_MASK)
         == QTC_NODE_TRANS_PRED_EXIST )
    {
        (*aNewPredicate)->flag &= ~QMO_PRED_TRANS_PRED_MASK;
        (*aNewPredicate)->flag |= QMO_PRED_TRANS_PRED_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qmoPred::checkPredicateForHostOpt( qmoPredicate * aPredicate )
{
/***********************************************************************
 *
 * Description : Host 변수를 포함한 질의 최적화를 수행할 것인지
 *               판단한다.
 *
 *            ** (주의) **
 *               execution시에 성능을 위해 predicate->flag에 정보를 세팅한다.
 *
 *               - 개별 predicate에 대해 execution 타임에 selectivity를
 *                 다시 구해야하는지의 여부
 *                 : QMO_PRED_HOST_OPTIMIZE_TRUE
 *
 *               - more list의 첫번째 predicate에 대해
 *                 total selectivity를 다시 구해야하는지의 여부
 *                 : QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE
 *
 *   다음의 조건을 만족해야 수행한다.
 *    - 호스트 변수가 존재해야 한다.
 *    - 호스트 변수를 포함한 predicate에 대해...
 *      - indexable이어야 한다.
 *      - list 이면 안된다.
 *      - <, >, <=, >=, between, not between 연산자이어야 한다.
 *    - predicate들 중에 in-subquery를 가지고 있으면 안된다.
 *
 ***********************************************************************/

    idBool         sResult = ID_FALSE;
    qmoPredicate * sNextIter;
    qmoPredicate * sMoreIter;

    for( sNextIter = aPredicate;
         sNextIter != NULL;
         sNextIter = sNextIter->next )
    {
        if( ( sNextIter->flag & QMO_PRED_HEAD_HOST_OPTIMIZE_MASK )
            == QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE )
        {
            sResult = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do...
        }
    }

    if( sResult == ID_TRUE )
    {
        for( sNextIter = aPredicate;
             sNextIter != NULL;
             sNextIter = sNextIter->next )
        {
            for( sMoreIter = sNextIter;
                 sMoreIter != NULL;
                 sMoreIter = sMoreIter->more )
            {
                if( ( sMoreIter->flag & QMO_PRED_INSUBQUERY_MASK )
                    == QMO_PRED_INSUBQUERY_EXIST )
                {
                    sResult = ID_FALSE;
                    break;
                }
                else
                {
                    // Nothing to do...
                }
            }
        }
    }
    else
    {
        // Nothing to do...
    }

    return sResult;
}

IDE_RC
qmoPredWrapperI::createWrapperList( qmoPredicate       * aPredicate,
                                    qmoPredWrapperPool * aWrapperPool,
                                    qmoPredWrapper    ** aResult )
{
    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::createWrapperList::__FT__" );

    *aResult = NULL;

    for( ;
         aPredicate != NULL;
         aPredicate = aPredicate->next )
    {
        IDE_TEST( addPred( aPredicate, aResult, aWrapperPool )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredWrapperI::extractWrapper( qmoPredWrapper  * aTarget,
                                 qmoPredWrapper ** aFrom )
{
    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::extractWrapper::__FT__" );

    IDE_DASSERT( aTarget != NULL );
    IDE_DASSERT( *aFrom != NULL );

    // aTarget이 aFrom의 처음일 때
    // aFrom의 위치를 바꿔야 한다.
    if( *aFrom == aTarget )
    {
        *aFrom = aTarget->next;
    }
    else
    {
        // Nothing to do...
    }

    // aTaget의 prev, next를 서로 연결하고
    // aTarget을 뺀다.
    if( aTarget->prev != NULL )
    {
        aTarget->prev->next = aTarget->next;
    }
    else
    {
        // Nothing to do...
    }

    if( aTarget->next != NULL )
    {
        aTarget->next->prev = aTarget->prev;
    }
    else
    {
        // Nothing to do...
    }

    aTarget->prev = NULL;
    aTarget->next = NULL;

    return IDE_SUCCESS;
}

IDE_RC
qmoPredWrapperI::addTo( qmoPredWrapper  * aTarget,
                        qmoPredWrapper ** aTo )
{
    qmoPredWrapper * sWrapperIter;

    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::addTo::__FT__" );

    IDE_DASSERT( aTarget != NULL );

    if( *aTo == NULL )
    {
        *aTo = aTarget;
    }
    else
    {
        for( sWrapperIter = *aTo;
             sWrapperIter->next != NULL;
             sWrapperIter = sWrapperIter->next ) ;

        sWrapperIter->next = aTarget;
        aTarget->prev = sWrapperIter;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPredWrapperI::moveWrapper( qmoPredWrapper  * aTarget,
                              qmoPredWrapper ** aFrom,
                              qmoPredWrapper ** aTo )
{
    IDE_DASSERT( aTarget != NULL );

    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::moveWrapper::__FT__" );

    IDE_TEST( extractWrapper( aTarget, aFrom ) != IDE_SUCCESS );

    IDE_TEST( addTo( aTarget, aTo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredWrapperI::moveAll( qmoPredWrapper ** aFrom,
                          qmoPredWrapper ** aTo )
{
    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::moveAll::__FT__" );

    if( *aFrom != NULL )
    {
        IDE_TEST( addTo( *aFrom, aTo ) != IDE_SUCCESS );
        *aFrom = NULL;
    }
    else
    {
        // Nothing to do...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredWrapperI::addPred( qmoPredicate       * aPredicate,
                          qmoPredWrapper    ** aWrapperList,
                          qmoPredWrapperPool * aWrapperPool )
{
    qmoPredWrapper * sNewWrapper;

    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::addPred::__FT__" );

    IDE_DASSERT( aWrapperList != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aWrapperPool != NULL );

    IDE_TEST( newWrapper( aWrapperPool, &sNewWrapper ) != IDE_SUCCESS );

    sNewWrapper->pred = aPredicate;
    sNewWrapper->prev = NULL;
    sNewWrapper->next = NULL;

    IDE_TEST( addTo( sNewWrapper, aWrapperList ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredWrapperI::newWrapper( qmoPredWrapperPool * aWrapperPool,
                             qmoPredWrapper    ** aNewWrapper )
{
    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::newWrapper::__FT__" );

    IDE_FT_ASSERT( aWrapperPool != NULL );

    if( aWrapperPool->used >= QMO_DEFAULT_WRAPPER_POOL_SIZE )
    {
        if( aWrapperPool->prepareMemory != NULL )
        {
            IDE_TEST( aWrapperPool->prepareMemory->alloc( ID_SIZEOF( qmoPredWrapper )
                                                          * QMO_DEFAULT_WRAPPER_POOL_SIZE,
                                                          (void**)&aWrapperPool->current )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_FT_ASSERT( aWrapperPool->executeMemory != NULL );

            IDE_TEST( aWrapperPool->executeMemory->alloc( ID_SIZEOF( qmoPredWrapper )
                                                          * QMO_DEFAULT_WRAPPER_POOL_SIZE,
                                                          (void**)&aWrapperPool->current )
                      != IDE_SUCCESS );
        }

        aWrapperPool->used = 0;
    }
    else
    {
        // nothing to do...
    }

    *aNewWrapper = (aWrapperPool->current)++;
    aWrapperPool->used++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPredWrapperI::initializeWrapperPool( iduVarMemList      * aMemory,
                                        qmoPredWrapperPool * aWrapperPool )
{
    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::initializeWrapperPool::__FT__" );

    aWrapperPool->prepareMemory = aMemory;
    aWrapperPool->executeMemory = NULL;
    aWrapperPool->current = &aWrapperPool->base[0];
    aWrapperPool->used = 0;

    return IDE_SUCCESS;
}

IDE_RC
qmoPredWrapperI::initializeWrapperPool( iduMemory          * aMemory,
                                        qmoPredWrapperPool * aWrapperPool )
{
    IDU_FIT_POINT_FATAL( "qmoPredWrapperI::initializeWrapperPool::__FT__" );

    aWrapperPool->prepareMemory = NULL;
    aWrapperPool->executeMemory = aMemory;
    aWrapperPool->current = &aWrapperPool->base[0];
    aWrapperPool->used = 0;

    return IDE_SUCCESS;
}

IDE_RC
qmoPred::separateRownumPred( qcStatement   * aStatement,
                             qmsQuerySet   * aQuerySet,
                             qmoPredicate  * aPredicate,
                             qmoPredicate ** aStopkeyPred,
                             qmoPredicate ** aFilterPred,
                             SLong         * aStopRecordCount )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1405 ROWNUM
 *     stopkey predicate과 filter predicate을 분리한다.
 *
 *     1. 인자로 넘어온 predicate list에서 stopkey predicate은
 *        다음과 같다.
 *
 *        aPredicate [p1]-[p2]-[p3]-[p4]-[p5]
 *                      ______________|
 *                      |
 *                      |
 *             stopkey predicate
 *
 *     2. 분리배치된 모습
 *       (1) stopkey Predicate    (2) filter predicate
 *           [p4]                     [p1]-[p2]-[p3]-[p5]
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate * sPredicate;
    qmoPredicate * sPrevPredicate;
    qmoPredicate * sStopkeyPredicate;
    qmoPredicate * sFilterPredicate;
    idBool         sIsStopkey;
    SLong          sStopRecordCount;

    IDU_FIT_POINT_FATAL( "qmoPred::separateRownumPred::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aStopkeyPred != NULL );
    IDE_DASSERT( aFilterPred  != NULL );

    //--------------------------------------
    // 초기화 작업
    //--------------------------------------

    sFilterPredicate = aPredicate;
    sStopkeyPredicate = NULL;
    sPrevPredicate = NULL;
    sStopRecordCount = 0;

    //--------------------------------------
    // stopkey predicate 분리
    //--------------------------------------

    for ( sPredicate = aPredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        IDE_TEST( isStopkeyPredicate( aStatement,
                                      aQuerySet,
                                      sPredicate->node,
                                      & sIsStopkey,
                                      & sStopRecordCount )
                  != IDE_SUCCESS );

        if ( sIsStopkey == ID_TRUE )
        {
            if ( sPrevPredicate == NULL )
            {
                sFilterPredicate = sPredicate->next;
                sStopkeyPredicate = sPredicate;
                sStopkeyPredicate->next = NULL;
            }
            else
            {
                sPrevPredicate->next = sPredicate->next;
                sStopkeyPredicate = sPredicate;
                sStopkeyPredicate->next = NULL;
            }
            break;
        }
        else
        {
            // Nothing to do.
        }

        sPrevPredicate = sPredicate;
    }

    *aStopkeyPred = sStopkeyPredicate;
    *aFilterPred = sFilterPredicate;
    *aStopRecordCount = sStopRecordCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPred::isStopkeyPredicate( qcStatement  * aStatement,
                             qmsQuerySet  * aQuerySet,
                             qtcNode      * aNode,
                             idBool       * aIsStopkey,
                             SLong        * aStopRecordCount )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1405 ROWNUM
 *     stopkey predicate인지 검사한다.
 *     predicate이 CNF로 처리될때만 stopkey predicate이 생성된다.
 *     DNF나 NNF로 처리되는 경우는 모두 filter predicate으로 생성된다.
 *
 *     stopkey가 될 수 있는 조건은 다음과 같다.
 *     1. ROWNUM <= 상수/호스트 변수
 *     2. ROWNUM < 상수/호스트 변수
 *     3. ROWNUM = 1
 *     4. 상수/호스트 변수 >= ROWNUM
 *     5. 상수/호스트 변수 > ROWNUM
 *     6. 1 = ROWNUM
 *
 *     예)
 *           OR
 *           |
 *           <
 *           |
 *        ROWNUM - 3
 *
 *     stopkey인 경우 aStopRecordCount은 다음의 값을 가진다.
 *     a. 호스트 변수 : -1
 *     b. 의미 없는 상수 : 0
 *     c. 의미 있는 상수 : 1이상
 *
 * Implementation :
 *     1. 최상위 노드는 OR 노드여야 한다.
 *     2. 비교연산자의 next는 NULL이어야 한다.
 *     3. 비교연산자의 인자들은 위 조건을 만족해야 한다.
 *
 ***********************************************************************/

    qtcNode         * sOrNode;
    qtcNode         * sCompareNode;
    qtcNode         * sRownumNode;
    qtcNode         * sValueNode;
    idBool            sIsStopkey;
    SLong             sConstValue;
    SLong             sStopRecordCount;
    SLong             sMinusOne;

    IDU_FIT_POINT_FATAL( "qmoPred::isStopkeyPredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // 초기화 작업
    //--------------------------------------

    sOrNode = aNode;
    sIsStopkey = ID_FALSE;
    sStopRecordCount = 0;
    sMinusOne = 0;

    //--------------------------------------
    // stopkey predicate 검사
    //--------------------------------------

    // 최상위 노드는 OR 노드여야 한다.
    if ( ( sOrNode->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_OR )
    {
        sCompareNode = (qtcNode*) sOrNode->node.arguments;

        // 비교연산자의 next는 NULL이어야 한다.
        if ( sCompareNode->node.next == NULL )
        {
            switch ( sCompareNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            {
                case MTC_NODE_OPERATOR_LESS:
                    // ROWNUM < 상수/호스트 변수

                    sMinusOne = 1;
                    /* fall through */

                case MTC_NODE_OPERATOR_LESS_EQUAL:
                    // ROWNUM <= 상수/호스트 변수

                    sRownumNode = (qtcNode*) sCompareNode->node.arguments;
                    sValueNode = (qtcNode*) sRownumNode->node.next;

                    if ( isROWNUMColumn( aQuerySet,
                                         sRownumNode ) == ID_TRUE )
                    {
                        if ( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement),
                                                sValueNode ) == ID_TRUE )
                        {
                            sIsStopkey = ID_TRUE;

                            if ( qtc::getConstPrimitiveNumberValue( QC_SHARED_TMPLATE(aStatement),
                                                                    sValueNode,
                                                                    & sConstValue )
                                 == ID_TRUE )
                            {
                                sStopRecordCount = sConstValue - sMinusOne;

                                if ( sStopRecordCount < 1 )
                                {
                                    // 의미없는 상수는 0과 같다.
                                    sStopRecordCount = 0;
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                sStopRecordCount = -1;
                            }
                        }
                        else
                        {
                            if ( ( QTC_IS_DYNAMIC_CONSTANT( sValueNode ) == ID_TRUE ) &&
                                 ( qtc::dependencyEqual( & sValueNode->depInfo,
                                                         & qtc::zeroDependencies )
                                   == ID_TRUE ) )
                            {
                                sIsStopkey = ID_TRUE;
                                sStopRecordCount = -1;
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
                    break;

                case MTC_NODE_OPERATOR_GREATER:
                    // 상수/호스트 변수 > ROWNUM

                    sMinusOne = 1;
                    /* fall through */

                case MTC_NODE_OPERATOR_GREATER_EQUAL:
                    // 상수/호스트 변수 >= ROWNUM

                    sValueNode = (qtcNode*) sCompareNode->node.arguments;
                    sRownumNode = (qtcNode*) sValueNode->node.next;

                    if ( isROWNUMColumn( aQuerySet,
                                         sRownumNode ) == ID_TRUE )
                    {
                        if ( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement),
                                                sValueNode ) == ID_TRUE )
                        {
                            sIsStopkey = ID_TRUE;

                            if ( qtc::getConstPrimitiveNumberValue( QC_SHARED_TMPLATE(aStatement),
                                                                    sValueNode,
                                                                    & sConstValue )
                                 == ID_TRUE )
                            {
                                sStopRecordCount = sConstValue - sMinusOne;

                                if ( sStopRecordCount < 1 )
                                {
                                    // 의미없는 상수는 0과 같다.
                                    sStopRecordCount = 0;
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                sStopRecordCount = -1;
                            }
                        }
                        else
                        {
                            if ( ( QTC_IS_DYNAMIC_CONSTANT( sValueNode ) == ID_TRUE ) &&
                                 ( qtc::dependencyEqual( & sValueNode->depInfo,
                                                         & qtc::zeroDependencies )
                                   == ID_TRUE ) )
                            {
                                sIsStopkey = ID_TRUE;
                                sStopRecordCount = -1;
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
                    break;

                case MTC_NODE_OPERATOR_EQUAL:

                    // ROWNUM = 1
                    sRownumNode = (qtcNode*) sCompareNode->node.arguments;
                    sValueNode = (qtcNode*) sRownumNode->node.next;

                    if ( isROWNUMColumn( aQuerySet,
                                         sRownumNode ) == ID_TRUE )
                    {
                        if ( qtc::getConstPrimitiveNumberValue( QC_SHARED_TMPLATE(aStatement),
                                                                sValueNode,
                                                                & sConstValue )
                             == ID_TRUE )
                        {
                            if ( sConstValue == (SLong) 1 )
                            {
                                sIsStopkey = ID_TRUE;
                                sStopRecordCount = 1;
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
                    else
                    {
                        // 1 = ROWNUM
                        sValueNode = (qtcNode*) sCompareNode->node.arguments;
                        sRownumNode = (qtcNode*) sValueNode->node.next;

                        if ( isROWNUMColumn( aQuerySet,
                                             sRownumNode ) == ID_TRUE )
                        {
                            if ( qtc::getConstPrimitiveNumberValue( QC_SHARED_TMPLATE(aStatement),
                                                                    sValueNode,
                                                                    & sConstValue )
                                 == ID_TRUE )
                            {
                                if ( sConstValue == (SLong) 1 )
                                {
                                    sIsStopkey = ID_TRUE;
                                    sStopRecordCount = 1;
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
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    break;

                default:
                    break;
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

    *aIsStopkey = sIsStopkey;
    *aStopRecordCount = sStopRecordCount;

    return IDE_SUCCESS;
}

IDE_RC
qmoPred::removeTransitivePredicate( qmoPredicate ** aPredicate,
                                    idBool          aOnlyJoinPred )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1404 Transitive Predicate Generation
 *     predicate list에서 생성된 transitive predicate을 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate  * sPredicate;
    qmoPredicate  * sMorePredicate;
    qmoPredicate  * sFirstPredicate = NULL;
    qmoPredicate  * sPrevPredicate = NULL;
    qmoPredicate  * sFirstMorePredicate = NULL;
    qmoPredicate  * sPrevMorePredicate = NULL;

    IDU_FIT_POINT_FATAL( "qmoPred::removeTransitivePredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPredicate != NULL );

    //--------------------------------------
    // transitive predicate 제거
    //--------------------------------------

    for ( sPredicate = *aPredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        sFirstMorePredicate = NULL;
        sPrevMorePredicate = NULL;
        
        for ( sMorePredicate = sPredicate;
              sMorePredicate != NULL;
              sMorePredicate = sMorePredicate->more )
        {
            if ( ( (sMorePredicate->flag & QMO_PRED_TRANS_PRED_MASK)
                   == QMO_PRED_TRANS_PRED_TRUE ) &&
                 ( 
                     ( (sMorePredicate->flag & QMO_PRED_JOIN_PRED_MASK)
                       == QMO_PRED_JOIN_PRED_TRUE )
                     ||
                     ( aOnlyJoinPred == ID_FALSE ) )
                 )
            {
                // Nothing to do.

                // transitive join predicate은 제거한다.
            }
            else
            {
                if ( sPrevMorePredicate == NULL )
                {
                    sFirstMorePredicate = sMorePredicate;
                    sPrevMorePredicate = sFirstMorePredicate;
                }
                else
                {
                    sPrevMorePredicate->more = sMorePredicate;
                    sPrevMorePredicate = sPrevMorePredicate->more;
                }   
            }
        }

        // predicate more list에서의 연결을 끊는다.
        if ( sPrevMorePredicate != NULL )
        {
            sPrevMorePredicate->more = NULL;
        }
        else
        {
            // Nothing to do.
        }

        // predicate list에 연결한다.
        if ( sFirstMorePredicate != NULL )
        {
            if ( sPrevPredicate == NULL )
            {
                sFirstPredicate = sFirstMorePredicate;
                sPrevPredicate = sFirstPredicate;
            }
            else
            {
                sPrevPredicate->next = sFirstMorePredicate;
                sPrevPredicate = sPrevPredicate->next;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    // predicate list에서의 연결을 끊는다.
    if ( sPrevPredicate != NULL )
    {
        sPrevPredicate->next = NULL;
    }
    else
    {
        // Nothing to do.
    }

    *aPredicate = sFirstPredicate;

    return IDE_SUCCESS;
}

IDE_RC
qmoPred::removeEquivalentTransitivePredicate( qcStatement   * aStatement,
                                              qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1404 Transitive Predicate Generation
 *     생성된 transitive predicate과 논리적으로 중복된 predicate이
 *     있는 경우 삭제한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate * sOutPredicate = NULL;
    qmoPredicate * sPrevPredicate = NULL;
    qmoPredicate * sPredicate;
    idBool         sIsExist;

    IDU_FIT_POINT_FATAL( "qmoPred::removeEquivalentTransitivePredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate != NULL );

    //--------------------------------------
    // 초기화 작업
    //--------------------------------------

    sPredicate = *aPredicate;

    //--------------------------------------
    // 중복된 transitive predicate 제거
    //--------------------------------------

    while ( sPredicate != NULL )
    {
        // 아직 relocate 전이다.
        IDE_DASSERT( sPredicate->more == NULL );

        sIsExist = ID_FALSE;

        if ( (sPredicate->flag & QMO_PRED_TRANS_PRED_MASK)
             == QMO_PRED_TRANS_PRED_TRUE )
        {
            IDE_TEST( qmoTransMgr::isExistEquivalentPredicate( aStatement,
                                                               sPredicate,
                                                               sOutPredicate,
                                                               & sIsExist )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        if ( sIsExist != ID_TRUE )
        {
            if ( sPrevPredicate == NULL )
            {
                sPrevPredicate = sPredicate;
                sOutPredicate = sPredicate;
            }
            else
            {
                sPrevPredicate->next = sPredicate;
                sPrevPredicate = sPredicate;
            }
        }
        else
        {
            // Nothing to do.
        }

        sPredicate = sPredicate->next;
    }

    if ( sPrevPredicate != NULL )
    {
        sPrevPredicate->next = NULL;
    }
    else
    {
        // Nothing to do.
    }

    *aPredicate = sOutPredicate;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPred::addNNFFilter4linkedFilter( qcStatement   * aStatement,
                                           qtcNode       * aNNFFilter,
                                           qtcNode      ** aNode      )
{
/***********************************************************************
 *
 * Description :
 *     BUG-35155 Partial CNF
 *     linkFilterPredicate 에서 만들어진 qtcNode 그룹에 NNF 필터를 추가한다.
 *
 * Implementation :
 *     최상위에 AND 노드가 있으므로 arguments 의 next 끝에 NNF 필터를 연결한다.
 *
 ***********************************************************************/

    qtcNode  * sNode;

    IDU_FIT_POINT_FATAL( "qmoPred::addNNFFilter4linkedFilter::__FT__" );

    // Attach nnfFilter to filter
    sNode = (qtcNode *)((*aNode)->node.arguments);
    while (sNode->node.next != NULL)
    {
        sNode = (qtcNode *)(sNode->node.next);
    }
    sNode->node.next = (mtcNode *)aNNFFilter;

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                *aNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmoPred::hasOnlyColumnCompInPredList( qmoPredInfo * aJoinPredList )
{
/****************************************************************************
 * 
 *  Description : BUG-39403 Inverse Index Join Method Predicate 제한
 *  
 *  입력된 Join Predicate List에서, 
 *  모든 Join Predicate가 다음을 만족하는지 검사한다.
 *
 *   - (Assertion) Join Predicate은 INDEX_JOINABLE 이어야 한다.
 *   - (Assertion) Join Predicate이 '=' 여야 한다.
 *   - Join Predicate의 Operand 모두 Column인지 검증한다.
 *  
 *  > 어느 하나라도 Column이 아닌 Expression을 가지고 있을 때, FALSE
 *  > 모든 Predicate이 Column 들만 비교하는 경우, TRUE
 *
 ***************************************************************************/

    qmoPredInfo  * sJoinPredMoreInfo = NULL;
    qmoPredInfo  * sJoinPredInfo     = NULL;
    qtcNode      * sNode             = NULL;
    idBool         sHasOnlyColumn    = ID_TRUE;

    for ( sJoinPredMoreInfo = aJoinPredList;
          sJoinPredMoreInfo != NULL;
          sJoinPredMoreInfo = sJoinPredMoreInfo->next )
    {
        for ( sJoinPredInfo = sJoinPredMoreInfo;
              sJoinPredInfo != NULL;
              sJoinPredInfo = sJoinPredInfo->more )
        {
            // 각각의 Join Predicate이 INDEX_JOINABLE 이어야 한다.
            IDE_DASSERT( ( sJoinPredInfo->predicate->flag & QMO_PRED_INDEX_JOINABLE_MASK )
                         == QMO_PRED_INDEX_JOINABLE_TRUE );

            // Predicate의 첫 Node를 가지고 온다.
            sNode = sJoinPredInfo->predicate->node;

            // CNF Form이므로, 각 Predicate은 OR Node이거나
            // Predicate 자체로 넘어온다.
            if ( ( sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                 == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                // sNode가 OR Node인 경우 (AND가 될 수 없다)
                IDE_DASSERT( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_OR )

                // OR Node의 하위 Node를 탐색한다.
                sNode = (qtcNode *)(sNode->node.arguments);

                // OR Node의 하위 Node를 모두 탐색해 검증
                while( sNode != NULL )
                {
                    sHasOnlyColumn = hasOnlyColumnCompInPredNode( sNode );

                    if ( sHasOnlyColumn == ID_FALSE )
                    {
                        break;
                    }
                    else
                    {
                        sNode = (qtcNode *)sNode->node.next;
                    }
                }
            }
            else
            {
                // sNode가 OR Node가 아닌 경우
                // 바로 검증한다.
                sHasOnlyColumn = hasOnlyColumnCompInPredNode( sNode );
            }

            // Expression을 가진 Join Predicat을 찾았다면
            // 다른 Join Predicate 탐색을 중단한다.
            if ( sHasOnlyColumn == ID_FALSE )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        // Expression을 가진 Join Predicat을 찾았다면
        // 다른 Join Predicate 탐색을 중단한다.
        if ( sHasOnlyColumn == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sHasOnlyColumn;
}

idBool qmoPred::hasOnlyColumnCompInPredNode( qtcNode * aNode )
{
/****************************************************************************
 * 
 *  Description : BUG-39403 Inverse Index Join Method Predicate 제한
 *  
 *  입력된 Join Predicate Node에서, 다음을 만족하는지 검사한다.
 *
 *   - (Assertion) Join Predicate이 '=' 여야 한다.
 *   - Join Predicate Node의 Operand 모두 Column인지 검증한다.
 *  
 *  > 어느 한 쪽이라도 Column이 아닌 Expression을 가지고 있을 때, FALSE
 *  > 해당 Predicate Node가 Column 들만 비교하는 경우, TRUE
 *
 ***************************************************************************/

    qtcNode      * sNodeArg       = NULL;
    idBool         sHasOnlyColumn = ID_TRUE;

    IDE_DASSERT( aNode->node.module == &mtfEqual );

    // left
    sNodeArg = (qtcNode *)aNode->node.arguments;
    if ( sNodeArg->node.module != &qtc::columnModule )
    {
        sHasOnlyColumn = ID_FALSE;
    }
    else
    {
        // right
        sNodeArg = (qtcNode *)aNode->node.arguments->next;
        if ( sNodeArg->node.module != &qtc::columnModule )
        {
            sHasOnlyColumn = ID_FALSE;
        }
        else
        {
            sHasOnlyColumn = ID_TRUE;
        }
    }

    return sHasOnlyColumn;
}

/* TASK-7219 */
IDE_RC qmoPred::isValidPushDownPredicate( qcStatement  * aStatement,
                                          qmgGraph     * aGraph,
                                          qmoPredicate * aPredicate,
                                          idBool       * aIsValid )
{
/****************************************************************************************
 *
 * Description : qmgSelection::doViewPushSelection 를 참조하여 Predicate 이 Push Down
 *               가능한 Valid한 대상인지 검사한다.
 *
 * Implementation : 1. Predicate이 Subquery가 아니여야 한다.
 *                  2. PUSH_PRED 힌트에 의해 내려온 Join Predicate인 경우
 *                  3. Predicate이 Indexable해야 하며
 *                  4. Predicate이 List가 아니여야 한다.
 *                  5. One Table Predicate인 경우, 외부참조 컬럼이 없어야 한다.
 *                  6. Valid 여부를 반환한다.
 *
 ****************************************************************************************/

    UInt   sColumnID    = ID_UINT_MAX;
    idBool sIsIndexable = ID_FALSE;
    idBool sIsValid     = ID_FALSE;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aGraph == NULL, ERR_NULL_GRAPH );
    IDE_TEST_RAISE( aPredicate == NULL, ERR_NULL_PREDICATE );

    /* 1. Predicate이 Subquery가 아니여야 한다. */
    if ( ( aPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK )
         == QTC_NODE_SUBQUERY_EXIST )
    {
        IDE_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. PUSH_PRED 힌트에 의해 내려온 Join Predicate인 경우 */
    if ( ( aPredicate->flag & QMO_PRED_PUSH_PRED_HINT_MASK )
         == QMO_PRED_PUSH_PRED_HINT_TRUE )
    {
        IDE_TEST( isIndexable( aStatement,
                               aPredicate,
                               &( aGraph->myFrom->depInfo ),
                               &( qtc::zeroDependencies ),
                               &( sIsIndexable ) )
                  != IDE_SUCCESS );

        /* 3. Predicate이 Indexable해야 하며 */
        if ( sIsIndexable != ID_TRUE )
        {
            IDE_RAISE( NORMAL_EXIT );
        }
        else
        {
            IDE_TEST( getColumnID( aStatement,
                                   aPredicate->node,
                                   ID_TRUE,
                                   &( sColumnID ) )
                      != IDE_SUCCESS );

            /* 4. Predicate이 List가 아니여야 한다. */
            if ( sColumnID == QMO_COLUMNID_LIST )
            {
                IDE_RAISE( NORMAL_EXIT );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* 5. One Table Predicate인 경우 */
        if ( qtc::getPosNextBitSet( &( aPredicate->node->depInfo ),
                                    qtc::getPosFirstBitSet(
                                        &( aPredicate->node->depInfo ) ) )
             != QTC_DEPENDENCIES_END )
        {
            /* 외부 참조를 가지는 경우, property에 따라 동작한다. */
            IDE_TEST_CONT( ( SDU_SHARD_TRANSFORM_MODE & SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_MASK  )
                           == SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_DISABLE,
                           NORMAL_EXIT );

            IDE_TEST( qmoPred::getColumnID( aStatement,
                                            aPredicate->node,
                                            ID_TRUE,
                                            & sColumnID )
                      != IDE_SUCCESS );

            /* Predicate이 List가 아니여야 한다. */
            if ( sColumnID != QMO_COLUMNID_LIST )
            {
                setOutRefColumnForQtcNode( aPredicate->node,
                                           &aPredicate->node->depInfo );
            }
            else
            {
                IDE_RAISE( NORMAL_EXIT );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    sIsValid = ID_TRUE;

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    /* 6. Valid 여부를 반환한다. */
    if ( aIsValid != NULL )
    {
        *aIsValid = sIsValid;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoPred::isValidPushDownPredicate",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoPred::isValidPushDownPredicate",
                                  "graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PREDICATE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoPred::isValidPushDownPredicate",
                                  "predicate is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-7219 Non-shard DML */
void qmoPred::setOutRefColumnForQtcNode( qtcNode   * aNode,
                                         qcDepInfo * aPredicateDepInfo )
{
    if ( aNode != NULL )
    {
        if ( aNode->node.module == &qtc::columnModule )
        {
            if ( qtc::getPosNextBitSet( aPredicateDepInfo,
                                        qtc::getPosFirstBitSet( &aNode->depInfo ) ) 
                != QTC_DEPENDENCIES_END )
            {
                aNode->lflag &= ~QTC_NODE_OUT_REF_COLUMN_MASK;
                aNode->lflag |= QTC_NODE_OUT_REF_COLUMN_TRUE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */

        }

        setOutRefColumnForQtcNode( (qtcNode*)aNode->node.next,
                                   aPredicateDepInfo );

        setOutRefColumnForQtcNode( (qtcNode*)aNode->node.arguments,
                                   aPredicateDepInfo );
    }
    else
    {
        /* Nothing to do. */
    }
}

