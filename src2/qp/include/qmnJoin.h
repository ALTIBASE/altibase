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
 * $Id: qmnJoin.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     JOIN(JOIN) Node
 *
 *     관계형 모델에서 cartesian product를 수행하는 Plan Node 이다.
 *     다양한 Join Method들은 하위 노드의 형태에 따라 결정된다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Cartesian Product
 *         - Nested Loop Join 계열
 *         - Sort-based Join 계열
 *         - Hash-based Join 계열
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_JOIN_H_
#define _O_QMN_JOIN_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------
// Data Node Flags
//-----------------

// qmncJOIN.flag
// PROJ-1718 Subquery unnesting
// Join의 종류
#define QMNC_JOIN_TYPE_MASK                (0x00000003)
#define QMNC_JOIN_TYPE_INNER               (0x00000000)
#define QMNC_JOIN_TYPE_SEMI                (0x00000001)
#define QMNC_JOIN_TYPE_ANTI                (0x00000002)
#define QMNC_JOIN_TYPE_UNDEFINED           (0x00000003)

// qmndJOIN.flag
// First Initialization Done
#define QMND_JOIN_INIT_DONE_MASK           (0x00000001)
#define QMND_JOIN_INIT_DONE_FALSE          (0x00000000)
#define QMND_JOIN_INIT_DONE_TRUE           (0x00000001)

typedef struct qmncJOIN
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;
    qtcNode      * filter;

    // PROJ-2551 simple query 최적화
    idBool         isSimple;
    
} qmncJOIN;

typedef struct qmndJOIN
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;
    UInt              * flag;

    //---------------------------------
    // JOIN 고유 정보
    //---------------------------------

    setHitFlagFunc      setHitFlag;    // Child(HASH)에 따른 함수
    isHitFlaggedFunc    isHitFlagged;  // Child(HASH)에 따른 함수

} qmndJOIN;

class qmnJOIN
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

private:

    //------------------------
    // mapping by doIt() function pointer
    //------------------------

    // 호출되면 안됨
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // Inner join, Semi join (Inverse Index NL)
    // Left에서 한건, Right에서 한건 수행
    static IDE_RC doItLeft( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // Inner join, Semi join (Inverse Index NL)
    // 기존의 Left는 두고 Right에서 한건 수행
    static IDE_RC doItRight( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // Semi join
    static IDE_RC doItSemi( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // Anti join
    static IDE_RC doItAnti( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // Semi join (Inverse Sort), Anti join (Inverse Hash/Sort)
    static IDE_RC doItInverse( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // Semi join (Inverse Sort), Anti join (Inverse Hash/Sort)
    static IDE_RC doItInverseHitFirst( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

    // Semi join (Inverse Sort), Anti join (Inverse Hash/Sort)
    static IDE_RC doItInverseHitNext( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan,
                                      qmcRowFlag * aFlag );

    // Semi join (Inverse Sort), Anti join (Inverse Hash/Sort)
    static IDE_RC doItInverseNonHitFirst( qcTemplate * aTemplate,
                                          qmnPlan    * aPlan,
                                          qmcRowFlag * aFlag );

    // Semi join (Inverse Sort), Anti join (Inverse Hash/Sort)
    static IDE_RC doItInverseNonHitNext( qcTemplate * aTemplate,
                                         qmnPlan    * aPlan,
                                         qmcRowFlag * aFlag );

    static IDE_RC checkFilter( qcTemplate * aTemplate,
                               qmncJOIN   * aCodePlan,
                               UInt         aRightFlag,
                               idBool     * aRetry );

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qmncJOIN   * aCodePlan,
                             qmndJOIN   * aDataPlan );

};

#endif /* _O_QMN_JOIN_H_ */
