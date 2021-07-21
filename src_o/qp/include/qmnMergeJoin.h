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
 * $Id: qmnMergeJoin.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     MGJN(MerGe JoiN) Node
 *
 *     관계형 모델에서 Merge Join 연산을 수행하는 Plan Node 이다.
 *
 *     Join Method중 Merge Join만을 담당하며, Left및 Right Child로는
 *     다음과 같은 Plan Node만이 존재할 수 있다.
 *
 *         - SCAN Node
 *         - SORT Node
 *         - MGJN Node
 *
 *     To Fix PR-11944
 *     Right 에는 MGJN이 존재할 수 없으며 다음과 같은 Node만이 존재할 수 있다.
 *
 *         - SCAN Node
 *         - SORT Node
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_MERGE_JOIN_H_
#define _O_QMN_MERGE_JOIN_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

// qmncMGJN.flag
// Left Child Plan의 종류
#define QMNC_MGJN_LEFT_CHILD_MASK          (0x00000007)
#define QMNC_MGJN_LEFT_CHILD_NONE          (0x00000000)
#define QMNC_MGJN_LEFT_CHILD_SCAN          (0x00000001)
#define QMNC_MGJN_LEFT_CHILD_SORT          (0x00000002)
#define QMNC_MGJN_LEFT_CHILD_MGJN          (0x00000003)
#define QMNC_MGJN_LEFT_CHILD_PCRD          (0x00000004)

// qmncMGJN.flag
// Right Child Plan의 종류
#define QMNC_MGJN_RIGHT_CHILD_MASK         (0x00000030)
#define QMNC_MGJN_RIGHT_CHILD_NONE         (0x00000000)
#define QMNC_MGJN_RIGHT_CHILD_SCAN         (0x00000010)
#define QMNC_MGJN_RIGHT_CHILD_SORT         (0x00000020)
#define QMNC_MGJN_RIGHT_CHILD_PCRD         (0x00000030)

// qmncMGJN.flag
// PROJ-1718 Subquery unnesting
// Join의 종류
#define QMNC_MGJN_TYPE_MASK                (0x00000300)
#define QMNC_MGJN_TYPE_INNER               (0x00000000)
#define QMNC_MGJN_TYPE_SEMI                (0x00000100)
#define QMNC_MGJN_TYPE_ANTI                (0x00000200)
#define QMNC_MGJN_TYPE_UNDEFINED           (0x00000300)

//-----------------
// Data Node Flags
//-----------------

// qmndMGJN.flag
// First Initialization Done
#define QMND_MGJN_INIT_DONE_MASK           (0x00000001)
#define QMND_MGJN_INIT_DONE_FALSE          (0x00000000)
#define QMND_MGJN_INIT_DONE_TRUE           (0x00000001)

// qmndMGJN.flag
// Cursor가 저장되어 있는 지의 여부
// 즉, 이전에 만족한 Data가 존재했었는지의 여부
#define QMND_MGJN_CURSOR_STORED_MASK       (0x00000002)
#define QMND_MGJN_CURSOR_STORED_FALSE      (0x00000000)
#define QMND_MGJN_CURSOR_STORED_TRUE       (0x00000002)

#define QMND_MGJN_RIGHT_DATA_MASK          (0x00000004)
#define QMND_MGJN_RIGHT_DATA_NONE          (0x00000000)
#define QMND_MGJN_RIGHT_DATA_EXIST         (0x00000004)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT * FROM T1,T2 WHERE T1.i1 = T2.i1 AND T1.i2 + T2.i2 > 0;
 *
 *                   [MGJN]
 *                    /  \
 *                   /    \
 *                [SCAN] [SCAN]
 *                  T1     T2
 *
 *   qmncMGJN 의 구성
 *
 *       - myNode : Join Predicate의 Right Column을 저장
 *                  Ex) T2.i1을 저장
 *
 *       - mergeJoinPredicate : Merge Joinable Predicate을 저장
 *                  Ex) T1.i1 = T2.i1
 *
 *       - storedJoinPredicate : Merge Joinable Predicate을 저장 Column과
 *                  비교할 수 있도록 저장
 *                  Ex) T1.i1 = Stored[T2.i1]
 *
 *       - compareLeftRight : Merge Joinable Predicate의 연산자가 등호일
 *                  경우 생성하며 대소 비교가 가능하도록 구성
 *                  Ex) T1.i1 >= T2.i1  (>=)로 생성해야 함
 *
 *       - joinFilter : Merge Joinable Predicate이 아닌 Join Filter
 *                  Ex) T1.i2 + T2.i2 > 0
 *
 *   Merge Join 알고리즘은 다음 문서를 참조
 *       - "4.2.2.2 Query Executor Design - Part 2.doc"
 *           : Merge Join 참조
 *       - "4.2.2.3 Query Executor Design - Part 3.doc"
 *           : MGJN Node 참조
 ----------------------------------------------------------------------*/

typedef struct qmncMGJN
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // MGJN 고유 정보
    //---------------------------------

    qmcMtrNode    * myNode;               // 저장할 Right Column 정보

    // Merge Joinable Predicate 관련 정보
    qtcNode       * mergeJoinPred;        // Merge Join Predicate
    
    qtcNode       * storedMergeJoinPred;  // 저장 데이터와 비교하는 Predicate
    qtcNode       * compareLeftRight;     // 부등호 연산자인 경우의 대소비교

    qtcNode       * joinFilter;           // 기타 Join Predicate

    //---------------------------------
    // Data 영역 정보
    //---------------------------------

    UInt            mtrNodeOffset;        // 저장 Column의 위치
    
} qmncMGJN;

typedef struct qmndMGJN
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;        
    UInt              * flag;        

    //---------------------------------
    // MGJN 고유 정보
    //---------------------------------

    qmdMtrNode        * mtrNode;        // 저장 Right Column의 정보
    UInt                mtrRowSize;     // 저장 Right Column의 크기
    
} qmndMGJN;

class qmnMGJN
{
public:
    //------------------------
    // Base Function Pointer
    //------------------------

    // 초기화
    static IDE_RC init( qcTemplate * aTemplate,
                        qmnPlan    * aPlan );

    // 수행 함수
    static IDE_RC doIt( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag );

    // Null Padding
    static IDE_RC padNull( qcTemplate * aTemplate,
                           qmnPlan    * aPlan );

    // Plan 정보 출력
    static IDE_RC printPlan( qcTemplate   * aTemplate,
                             qmnPlan      * aPlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode );

    //------------------------
    // mapping by doIt() function pointer
    //------------------------

    // 호출되면 안됨
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    static IDE_RC doItInner( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 최초 수행 함수
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 수행 함수
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    static IDE_RC doItSemi( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    static IDE_RC doItAnti( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncMGJN   * aCodePlan,
                             qmndMGJN   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncMGJN   * aCodePlan,
                               qmndMGJN   * aDataPlan );

    //------------------------
    // 수행 관련 함수
    //------------------------

    // Merge Join Algorithm을 수행
    static IDE_RC mergeJoin( qcTemplate * aTemplate,
                             qmncMGJN   * aCodePlan,
                             qmndMGJN   * aDataPlan,
                             idBool       aRightExist,
                             qmcRowFlag * aFlag );

    // Merge Join을 결과가 있거나 없을 때까지 반복 수행
    static IDE_RC loopMerge( qcTemplate * aTemplate,
                             qmncMGJN   * aCodePlan,
                             qmndMGJN   * aDataPlan,
                             qmcRowFlag * aFlag );

    // Merge Join 조건을 검사하고 진행 여부를 결정.
    static IDE_RC checkMerge( qcTemplate * aTemplate,
                              qmncMGJN   * aCodePlan,
                              qmndMGJN   * aDataPlan,
                              idBool     * aContinueNeed,
                              qmcRowFlag * aFlag );
    
    // 최초 Stored Merge 조건 검사
    static IDE_RC checkFirstStoredMerge( qcTemplate * aTemplate,
                                         qmncMGJN   * aCodePlan,
                                         qmndMGJN   * aDataPlan,
                                         qmcRowFlag * aFlag );

    // Stored Merge Join 조건을 검사하고 진행 여부를 결정.
    static IDE_RC checkStoredMerge( qcTemplate * aTemplate,
                                    qmncMGJN   * aCodePlan,
                                    qmndMGJN   * aDataPlan,
                                    idBool     * aContinueNeed,
                                    qmcRowFlag * aFlag );
    
    // Merge Join Predicate 만족 시 Cursor 관리
    static IDE_RC manageCursor( qcTemplate * aTemplate,
                                qmncMGJN   * aCodePlan,
                                qmndMGJN   * aDataPlan );

    // 적절한 Left 또는 Right Row를 획득한다.
    static IDE_RC readNewRow( qcTemplate * aTemplate,
                              qmncMGJN   * aCodePlan,
                              idBool     * aReadLeft,
                              qmcRowFlag * aFlag );

    // Join Filter의 만족 여부 검사
    static IDE_RC checkJoinFilter( qcTemplate * aTemplate,
                                   qmncMGJN   * aCodePlan,
                                   idBool     * aResult );
    
    //------------------------
    // 커서 관리 함수
    //------------------------

    // Right Child의 커서를 저장
    static IDE_RC storeRightCursor( qcTemplate * aTemplate,
                                    qmncMGJN   * aCodePlan );

    // Right Child의 커서를 복원
    static IDE_RC restoreRightCursor( qcTemplate * aTemplate,
                                    qmncMGJN   * aCodePlan,
                                    qmndMGJN   * aDataPlan );

};



#endif /* _O_QMN_MERGE_JOIN_H_ */

