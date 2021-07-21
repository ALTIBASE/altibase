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
 * $Id: qmoNormalForm.h 89925 2021-02-03 04:40:48Z ahra.cho $
 *
 * Description :
 *     Normal Form Manager
 *
 *     비정규화된 Predicate들을 정규화된 형태로 변경시키는 역활을 한다.
 *     다음과 같은 정규화를 수행한다.
 *         - CNF (Conjunctive Normal Form)
 *         - DNF (Disjunctive Normal Form)
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _Q_QMO_NORMAL_FORM_H_
#define _Q_QMO_NORMAL_FORM_H_ 1

#include <ide.h>
#include <qtcDef.h>
#include <qmsParseTree.h>

//-----------------------------------------------------------
// To Fix PR-8379
// Normal Form 비교 연산자 노드 최대 개수
// Normal Form 변경시 아래의 개수가 넘는 경우라면,
// 아무리 비용 계산이 적게 나오더라도 지나치게 많은 Filter로
// 인해 오히려 성능이 낮게 된다.
// CNF와 DNF 모두 해당 값을 넘는다면 비용 비교가 이루어지기
// 때문에 값이 낮다고 큰 지장이 되지 않는다.
//-----------------------------------------------------------
// #define QMO_NORMALFORM_MAXIMUM                         (128)

//-----------------------------------------------------------
// Normal Form 관리 함수
//-----------------------------------------------------------

class qmoNormalForm
{
public:

    // CNF로만 처리될 수 있는 경우를 판단
    static IDE_RC normalizeCheckCNFOnly( qtcNode  * aNode,
                                         idBool   * aCNFonly );

    // DNF 로 정규화
    static IDE_RC normalizeDNF( qcStatement   * aStatement,
                                qtcNode       * aNode,
                                qtcNode      ** aDNF        );

    // CNF 로 정규화
    static IDE_RC normalizeCNF( qcStatement   * aStatement,
                                qtcNode       * aNode,
                                qtcNode      ** aCNF,
                                idBool          aIsWhere = ID_FALSE,
                                qmsHints      * aHints = NULL );
           
    // DNF로 정규화되었을때 만들어지는 비교연산자 노드의 개수 예측
    static IDE_RC estimateDNF( qtcNode  * aNode,
                               UInt     * aCount );

    // CNF로 정규화되었을때 만들어지는 비교연산자 노드의 개수 예측
    static IDE_RC estimateCNF( qtcNode  * aNode,
                               UInt     * aCount );

    // 정규화형태로 변환하면서 생긴 새로운 논리연산자 노드에 대한
    // flag와 dependency 설정
    // qmoPredicate::nodeTransform()에서도 호출
    static IDE_RC setFlagAndDependencies(qtcNode * aNode);    


    // 정규화된 형태에서 불필요한 AND, OR 노드를 제거
    static IDE_RC optimizeForm( qtcNode  * aInputNode,
                                qtcNode ** aOutputNode );

    // BUG-34295 Join ordering ANSI style query
    // 정규화 형태로 변환하는 과정에서 predicate 연결
    static IDE_RC addToMerge( qtcNode     * aPrevNF,
                              qtcNode     * aCurrNF,
                              qtcNode    ** aNFNode );

    // BUG-35155 Partial CNF
    // 가능한 범위내에서 CNF 대상 선정 및 정규화되었을때의 비교연산자 노드 개수 예측
    static void estimatePartialCNF( qtcNode  * aNode,
                                    UInt     * aCount,
                                    qtcNode  * aRoot,
                                    UInt       aNFMaximum );

    // CNF 대상에서 제외된 qtcNode 의 NNF 필터 생성
    static IDE_RC extractNNFFilter4CNF( qcStatement  * aStatement,
                                        qtcNode      * aNode,
                                        qtcNode     ** aNNF );

private:

    // Subquery를 포함할 경우, DNF를 사용하면 안됨
    // DNF 형태로 predicate 변환
    static IDE_RC makeDNF( qcStatement  * aStatement,
                           qtcNode      * aNode,
                           qtcNode     ** aDNF );

    // CNF 형태로 predicate 변환
    static IDE_RC makeCNF( qcStatement  * aStatement,
                           qtcNode      * aNode,
                           qtcNode     ** aCNF );
    
    // 정규화 형태로 변환하는 과정에서 predicate에 대한 배분법칙 수행
    static IDE_RC productToMerge( qcStatement * aStatement,
                                  qtcNode     * aPrevNF,
                                  qtcNode     * aCurrNF,
                                  qtcNode    ** aNFNode );

    // NNF 필터 생성
    static IDE_RC makeNNF4CNFByCopyNodeTree( qcStatement  * aStatement,
                                             qtcNode      * aNode,
                                             qtcNode     ** aNNF );

    // qtcNode 트리 복사
    static IDE_RC copyNodeTree( qcStatement  * aStatement,
                                qtcNode      * aNode,
                                qtcNode     ** aCopy );

    // NULL 을 지원하는 addToMerge
    static IDE_RC addToMerge2( qtcNode     * aPrevNF,
                               qtcNode     * aCurrNF,
                               qtcNode    ** aNFNode);

};

#endif  /* _Q_QMO_NORMAL_FORM_H_ */ 
