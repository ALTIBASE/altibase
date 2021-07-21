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
 * $Id: qmnSetRecursive.h 88511 2020-09-08 07:02:11Z donovan.seo $
 *
 * Description :
 *
 *     SREC을 수행하는 Plan Node 이다.
 * 
 *     Left Child에 대한 Data와 Right Child에 대한 Data를
 *     recursive 하게 수행 하면서 결과를 리턴한다.
 *     Left Child와 Right Child의 VMTR을 서로 SWAP하면서 수행한다. 
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_SET_RECURSIVE_H_
#define _O_QMN_SET_RECURSIVE_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qmcMemSortTempTable.h>
#include <qmcSortTempTable.h>

//-----------------
// Code Node Flags
//-----------------


//-----------------
// Data Node Flags
//-----------------

// First Initialization Done
#define QMND_SREC_INIT_DONE_MASK       (0x00000001)
#define QMND_SREC_INIT_DONE_FALSE      (0x00000000)
#define QMND_SREC_INIT_DONE_TRUE       (0x00000001)

/* BUG-48116 recurisive with fatal */
#define QMND_SREC_RIGHT_INIT_DONE_MASK  (0x00000002)
#define QMND_SREC_RIGHT_INIT_DONE_FALSE (0x00000000)
#define QMND_SREC_RIGHT_INIT_DONE_TRUE  (0x00000002)

#define QMN_SWAP_SORT_TEMP( elem1, elem2 )                      \
    { sSortTemp = *elem1; *elem1 = *elem2; *elem2 = sSortTemp; }

typedef struct qmnSRECInfo
{
    //---------------------------------
    // Memory Temp Table 전용 정보
    //---------------------------------

    qmcdMemSortTemp    * memSortMgr;
    
    SLong                recordCnt;
    SLong                recordPos;

    //---------------------------------
    // mtrNode 정보
    //---------------------------------
    
    qmdMtrNode         * mtrNode;

} qmnSRECInfo;

typedef struct qmncSREC
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan           plan;
    UInt              flag;
    UInt              planID;

    //---------------------------------
    // SREC 고유 정보
    //---------------------------------

    qmnPlan         * recursiveChild;      // right query의 하위 recursive view scan
    
} qmncSREC;

typedef struct qmndSREC
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan          plan;
    doItFunc          doIt;
    UInt            * flag;
    
    //---------------------------------
    // SREC 정보
    //---------------------------------
    
    qmnSRECInfo       vmtrLeft;
    qmnSRECInfo       vmtrRight;

    // recursion level maximum
    vSLong            recursionLevel;

} qmndSREC;

class qmnSREC
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

    // 호출되어서는 안됨
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );
    
    static IDE_RC doItLeftFirst( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag );

    static IDE_RC doItLeftNext( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );
    
    static IDE_RC doItRightFirst( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan,
                                  qmcRowFlag * aFlag );

    static IDE_RC doItRightNext( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag );
    
private:

    // 최초 초기화
    static IDE_RC firstInit( qmncSREC   * aCodePlan,
                             qmndSREC   * aDataPlan );

    static IDE_RC setStackValue( qcTemplate * aTemplate,
                                 qmdMtrNode * aNode,
                                 void       * aSearchRow );
};

#endif /* _O_QMN_SET_RECURSIVE_H_ */
