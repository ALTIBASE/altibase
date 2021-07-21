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
 * $Id: qmnViewScan.h 85333 2019-04-26 02:34:41Z et16 $
 *
 * Description :
 *     VSCN(View SCaN) Node
 *
 *     관계형 모델에서 Materialized View에 대한
 *     Selection을 수행하는 Node이다.
 *
 *     하위 VMTR 노드의 저장 매체에 따라 다른 동작을 하게 되며,
 *     Memory Temp Table일 경우 Memory Sort Temp Table 객체의
 *        interface를 직접 사용하여 접근하며,
 *     Disk Temp Table일 경우 table handle과 index handle을 얻어
 *        별도의 Cursor를 통해 Sequetial Access한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_VIEW_SCAN_H_
#define _O_QMN_VIEW_SCAN_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------
// Data Node Flags
//-----------------

// qmndVSCN.flag
// First Initialization Done
#define QMND_VSCN_INIT_DONE_MASK           (0x00000001)
#define QMND_VSCN_INIT_DONE_FALSE          (0x00000000)
#define QMND_VSCN_INIT_DONE_TRUE           (0x00000001)

#define QMND_VSCN_DISK_CURSOR_MASK         (0x00000002)
#define QMND_VSCN_DISK_CURSOR_CLOSED       (0x00000000)
#define QMND_VSCN_DISK_CURSOR_OPENED       (0x00000002)

typedef struct qmncVSCN
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // VSCN 관련 정보
    //---------------------------------

    UShort         tupleRowID;

    //---------------------------------
    // Display 관련 정보
    //---------------------------------

    qmsNamePosition  viewOwnerName;
    qmsNamePosition  viewName;
    qmsNamePosition  aliasName;

} qmncVSCN;

typedef struct qmndVSCN
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan             plan;
    doItFunc             doIt;
    UInt               * flag;

    //---------------------------------
    // VSCN 관련 정보
    //---------------------------------

    UInt                 nullRowSize;   // Null row Size
    void               * nullRow;       // Null Row
    scGRID               nullRID;       // Null Row의 RID

    //---------------------------------
    // Memory Temp Table 전용 정보
    //---------------------------------

    qmcdMemSortTemp    * memSortMgr;
    qmdMtrNode         * memSortRecord;
    SLong                recordCnt;     // VMTR의 Record 개수
    SLong                recordPos;     // 현재 Recrord 위치

    //---------------------------------
    // Disk Temp Table 전용 정보
    //---------------------------------

    void               * tableHandle;
    void               * indexHandle;

    smiSortTempCursor  * tempCursor;         // tempCursor
} qmndVSCN;

class qmnVSCN
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

    // Memory Temp Table 사용 시 최초 수행 함수
    static IDE_RC doItFirstMem( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );

    // Memory Temp Table 사용 시 다음 수행 함수
    static IDE_RC doItNextMem( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // Disk Temp Table 사용 시 최초 수행 함수
    static IDE_RC doItFirstDisk( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag );

    // Disk Temp Table 사용 시 최초 수행 함수
    static IDE_RC doItNextDisk( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );

    // PROJ-2582 recursive with
    static IDE_RC touchDependency( qcTemplate * aTemplate,
                                   qmnPlan    * aPlan );
    
private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncVSCN   * aCodePlan,
                             qmndVSCN   * aDataPlan );

    // PROJ-2415 Grouping Sets Clause
    // 최초 초기화 이후 초기화를 수행
    static IDE_RC initForChild( qcTemplate * aTemplate,
                                qmncVSCN   * aCodePlan,
                                qmndVSCN   * aDataPlan );
    
    // Tuple을 구성하는 Column의 offset 재조정
    static IDE_RC refineOffset( qcTemplate * aTemplate,
                                qmncVSCN   * aCodePlan,
                                qmndVSCN   * aDataPlan );

    // VMTR child를 수행
    static IDE_RC execChild( qcTemplate * aTemplate,
                             qmncVSCN   * aCodePlan );

    // Null Row를 획득.
    static IDE_RC getNullRow( qcTemplate * aTemplate,
                              qmncVSCN   * aCodePlan,
                              qmndVSCN   * aDataPlan );

    // tuple set 구성
    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmncVSCN   * aCodePlan,
                               qmndVSCN   * aDataPlan );

    //------------------------
    // Disk 관련 수행 함수
    //------------------------

    static IDE_RC openCursor( qmndVSCN   * aDataPlan );

};

#endif /* _O_QMN_VIEW_SCAN_H_ */
