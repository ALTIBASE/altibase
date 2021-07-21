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
 * $Id: qmnProject.h 90527 2021-04-09 04:25:41Z jayce.park $
 *
 * Description :
 *     PROJ(PROJection) Node
 *
 *     관계형 모델에서 projection을 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_PROJ_H_
#define _O_QMN_PROJ_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qmcMemSortTempTable.h>

//-----------------
// Code Node Flags
//-----------------

// qmncPROJ.flag
// Limit을 가지는지에 대한 여부
# define QMNC_PROJ_LIMIT_MASK               (0x00000001)
# define QMNC_PROJ_LIMIT_FALSE              (0x00000000)
# define QMNC_PROJ_LIMIT_TRUE               (0x00000001)

// qmncPROJ.flag
// Top Projection에서 사용되는 지의 여부
#define QMNC_PROJ_TOP_MASK                  (0x00000002)
#define QMNC_PROJ_TOP_FALSE                 (0x00000000)
#define QMNC_PROJ_TOP_TRUE                  (0x00000002)

// qmncPROJ.flag
// Indexable MIN-MAX 최적화가 적용되었는지의 여부
#define QMNC_PROJ_MINMAX_MASK               (0x00000004)
#define QMNC_PROJ_MINMAX_FALSE              (0x00000000)
#define QMNC_PROJ_MINMAX_TRUE               (0x00000004)

/*
 * PROJ-1071 Parallel Query
 * qmncPROJ.flag
 * subquery 의 top 인지..
 */
#define QMNC_PROJ_QUERYSET_TOP_MASK         (0x00000008)
#define QMNC_PROJ_QUERYSET_TOP_FALSE        (0x00000000)
#define QMNC_PROJ_QUERYSET_TOP_TRUE         (0x00000008)

#define QMNC_PROJ_TOP_RESULT_CACHE_MASK     (0x00000010)
#define QMNC_PROJ_TOP_RESULT_CACHE_FALSE    (0x00000000)
#define QMNC_PROJ_TOP_RESULT_CACHE_TRUE     (0x00000010)

//-----------------
// Data Node Flags
//-----------------

// qmndPROJ.flag
# define QMND_PROJ_INIT_DONE_MASK           (0x00000001)
# define QMND_PROJ_INIT_DONE_FALSE          (0x00000000)
# define QMND_PROJ_INIT_DONE_TRUE           (0x00000001)

// qmndPROJ.flag
// when ::doIt is executed 
#define QMND_PROJ_FIRST_DONE_MASK           (0x00000002)
#define QMND_PROJ_FIRST_DONE_FALSE          (0x00000000)
#define QMND_PROJ_FIRST_DONE_TRUE           (0x00000002)


/*---------------------------------------------------------------------
 *  Example)
 *      SELECT i2, i4 FROM T1 WHERE i2 > 3;
 *      ---------------------
 *      | i1 | i2 | i3 | i4 |
 *      ~~~~~~~~~~~~~~~~~~~~~
 *       mtc->mtc->mtc->mtc
 *        |    |    |    |
 *       qtc  qtc  qtc  qtc
 *             |         |
 *            qmc  ->   qmc
 *
 *  in qmncPROJ
 *      myTarget : i2 -> i4
 *  in qmndPROJ
 ----------------------------------------------------------------------*/


typedef struct qmncPROJ  
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan               plan;
    UInt                  flag;
    UInt                  planID;

    //---------------------------------
    // Target 관련 정보
    //---------------------------------
    UInt                  targetCount;  // Target Column의 개수
    qmsTarget           * myTarget;     // Target 정보
    qcParseSeqCaches    * nextValSeqs;  // Sequence Next Value 정보
    qtcNode             * level;        // LEVEL Pseudo Column 정보

    /* PROJ-1715 Hierarchy Pseudo Column */
    qtcNode             * isLeaf;

    // PROJ-2551 simple query 최적화
    idBool                isSimple;    // simple target
    qmnValueInfo        * simpleValues;
    UInt                * simpleValueSizes;
    UInt                * simpleResultOffsets;
    UInt                  simpleResultSize;
    
    //---------------------------------
    // Limitation 관련 정보
    //---------------------------------
    qmsLimit            * limit;

    //---------------------------------
    // LOOP 관련 정보
    //---------------------------------
    qtcNode             * loopNode;
    qtcNode             * loopLevel;        // LOOP_LEVEL Pseudo Column 정보    

    //---------------------------------
    // Data 영역 관련 정보
    //---------------------------------
    
    UInt                  myTargetOffset; // myTarget의 영역
    
} qmncPROJ;

typedef struct qmndPROJ
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan       plan;
    doItFunc       doIt;        
    UInt         * flag;        

    //---------------------------------
    // PROJ 고유 정보
    //---------------------------------

    ULong          tupleOffset; // Top Projection에서 실제 Record의 크기 관리

    //---------------------------------
    // Limitation 관련 정보
    //---------------------------------
    
    doItFunc       limitExec;    // only for limit option
    ULong          limitCurrent; // 현재 Limit 값
    ULong          limitStart;   // 시작 Limit 값
    ULong          limitEnd;     // 최종 Limit 값

    //---------------------------------
    // Loop 관련 정보
    //---------------------------------

    SLong        * loopCurrentPtr;
    SLong          loopCurrent; // 현재 Loop 값
    SLong          loopCount;   // 최종 Limit 값

    //---------------------------------
    // Null Row 관련 정보
    //---------------------------------

    UInt           rowSize;    // Row의 최대 Size
    void         * nullRow;    // Null Row
    mtcColumn    * nullColumn; // Null Row의 Column 정보

    // BUG-48776
    // Scalar sub-query result keeping
    void         * mKeepValue;
    mtcColumn    * mKeepColumn;

    // PROJ-2462 ResultCache
    // Top Result Cache가 사용될 경우 VMTR 상위에 PROJ
    // 생성된다 이때 PROJ 은 ViewSCAN 역활을 한다.
    qmcdMemSortTemp * memSortMgr;
    qmdMtrNode      * memSortRecord;
    SLong             recordCnt;
    SLong             recordPos;
    UInt              nullRowSize;
} qmndPROJ;

class qmnPROJ
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

    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // Non-Top Projection을 수행
    static IDE_RC doItProject( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // Top Projection을 수행
    static IDE_RC doItTopProject( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan,
                                  qmcRowFlag * aFlag );

    // Limit Projection을 수행
    static IDE_RC doItWithLimit( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag );

    //------------------------
    // Direct External Call only for root node
    //------------------------

    // Target의 최대 크기 획득
    static IDE_RC getRowSize( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              UInt       * aSize );

    // Target 정보 획득
    static IDE_RC getCodeTargetPtr( qmnPlan    * aPlan,
                                    qmsTarget ** aTarget );

    // Communication Buffer에 실제 쓰여진 크기
    static ULong  getActualSize( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan );

    // PROJ-1075 target column의 개수
    static UInt   getTargetCount( qmnPlan    * aPlan );

    // Target의 최대 크기 계산
    static IDE_RC getMaxRowSize( qcTemplate * aTemplate,
                                 qmncPROJ   * aCodePlan,
                                 UInt       * aSize );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncPROJ   * aCodePlan,
                             qmndPROJ   * aDataPlan );
    
    // Null Row의 생성
    static IDE_RC makeNullRow( qcTemplate * aTemplate,
                               qmncPROJ   * aCodePlan,
                               qmndPROJ   * aDataPlan );

    // LEVEL pseudo column의 값 초기화
    static IDE_RC initLevel( qcTemplate * aTemplate,
                             qmncPROJ   * aCodePlan );

    // Flag에 따른 수행 함수 결정
    static IDE_RC setDoItFunction( qmncPROJ   * aCodePlan,
                                   qmndPROJ   * aDataPlan );

    //------------------------
    // doIt 관련 함수
    //------------------------

    static IDE_RC readSequence( qcTemplate * aTemplate,
                                qmncPROJ   * aCodePlan,
                                qmndPROJ   * aDataPlan );

    static void setLoopCurrent( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    //------------------------
    // Plan Display 관련 함수
    //------------------------

    // Target 정보를 출력한다.
    static IDE_RC printTargetInfo( qcTemplate   * aTemplate,
                                   qmncPROJ     * aCodePlan,
                                   iduVarString * aString );

    // PROJ-2462 Result Cache
    static IDE_RC getVMTRInfo( qcTemplate * aTemplate,
                               qmncPROJ   * aCodePlan,
                               qmndPROJ   * aDataPlan );

    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmncPROJ   * aCodePlan,
                               qmndPROJ   * aDataPlan );
    //PROJ-2462 Result Cache
    static IDE_RC doItVMTR( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );
};

#endif /* _O_QMN_PROJ_H_ */
