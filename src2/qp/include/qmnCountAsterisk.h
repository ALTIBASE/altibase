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
 * $Id: qmnCountAsterisk.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     CoUNT (*) Plan Node
 *
 *     관계형 모델에서 COUNT(*)만을 처리하기 위한 특수한 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_COUNT_ASTERISK_H_
#define _O_QMN_COUNT_ASTERISK_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qmnScan.h>

//-----------------
// Code Node Flags
//-----------------

/* qmncCUNT.flag                                     */
// Count(*) 처리 방법
// Table Handle을 이용한 방법과 Cursor를 이용한 방법

#define QMNC_CUNT_METHOD_MASK              (0x00000001)
#define QMNC_CUNT_METHOD_HANDLE            (0x00000000)
#define QMNC_CUNT_METHOD_CURSOR            (0x00000001)

/* qmncCUNT.flag                                     */
// 참조되는 table이 fixed table 또는 performance view인지의 정보를 나타냄.
// fixed table 또는 performance view에 대해서는
// table에 대한 IS LOCK을 잡지 않도록 하기 위함.
#define QMNC_CUNT_TABLE_FV_MASK            (0x00000002)
#define QMNC_CUNT_TABLE_FV_FALSE           (0x00000000)
#define QMNC_CUNT_TABLE_FV_TRUE            (0x00000002)

/* qmncCUNT.flag                                     */
// To fix BUG-12742
// index를 반드시 사용해야 하는지 여부를 나타낸다.
#define QMNC_CUNT_FORCE_INDEX_SCAN_MASK    (0x00000004)
#define QMNC_CUNT_FORCE_INDEX_SCAN_FALSE   (0x00000000)
#define QMNC_CUNT_FORCE_INDEX_SCAN_TRUE    (0x00000004)

//-----------------
// Data Node Flags
//-----------------

/* qmndCUNT.flag                                     */
// First Initialization Done
#define QMND_CUNT_INIT_DONE_MASK           (0x00000001)
#define QMND_CUNT_INIT_DONE_FALSE          (0x00000000)
#define QMND_CUNT_INIT_DONE_TRUE           (0x00000001)

/* qmndCUNT.flag                                     */
// Cursor Status
#define QMND_CUNT_CURSOR_MASK              (0x00000002)
#define QMND_CUNT_CURSOR_CLOSED            (0x00000000)
#define QMND_CUNT_CURSOR_OPENED            (0x00000002)

/* qmndCUNT.flag                                     */
// Isolation Level에 의하여 접근 방법이 바뀔 수 있다.
// Table Handle을 이용한 방법과 Cursor를 이용한 방법
#define QMND_CUNT_METHOD_MASK              (0x00000004)
#define QMND_CUNT_METHOD_HANDLE            (0x00000000)
#define QMND_CUNT_METHOD_CURSOR            (0x00000004)

/* qmndCUNT.flag                                     */
// plan의 selected method가 세팅되었는지 여부를 구한다.
#define QMND_CUNT_SELECTED_METHOD_SET_MASK   (0x00000008)
#define QMND_CUNT_SELECTED_METHOD_SET_TRUE   (0x00000008)
#define QMND_CUNT_SELECTED_METHOD_SET_FALSE  (0x00000000)

typedef struct qmncCUNT
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // CUNT 고유 정보
    //---------------------------------

    qtcNode      * countNode;           // Count Asterisk Node

    UShort         tupleRowID;          // Tuple ID
    UShort         depTupleRowID;       // Dependent Tuple Row ID

    qmsTableRef  * tableRef;
    smSCN          tableSCN;            // Table SCN

    void         * dumpObject;          // BUG-16651 Dump Object

    UInt                lockMode;       // Lock Mode
    smiCursorProperties cursorProperty; // Cursor Property

    //---------------------------------
    // Predicate 종류
    //---------------------------------

    qmncScanMethod       method;
    //---------------------------------
    // Display 관련 정보
    //---------------------------------

    qmsNamePosition      tableOwnerName;     // Table Owner Name
    qmsNamePosition      tableName;          // Table Name
    qmsNamePosition      aliasName;          // Alias Name

    qmoScanDecisionFactor * sdf;
} qmncCUNT;

typedef struct qmndCUNT
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan            plan;
    doItFunc            doIt;
    UInt              * flag;

    //---------------------------------
    // CUNT 고유 정보
    //---------------------------------

    SLong                       countValue;      // COUNT(*)의 값
    mtcTuple                  * depTuple;        // dependent tuple
    UInt                        depValue;        // dependency value

    smiTableCursor              cursor;          // Cursor
    smiCursorProperties         cursorProperty;  // Cursor 관련 정보

    //---------------------------------
    // Predicate 종류
    //---------------------------------

    smiRange                  * fixKeyRangeArea; // Fixed Key Range 영역
    smiRange                  * fixKeyRange;     // Fixed Key Range
    UInt                        fixKeyRangeSize; // Fixed Key Range 크기

    smiRange                  * fixKeyFilterArea; //Fixed Key Filter 영역
    smiRange                  * fixKeyFilter;    // Fixed Key Filter
    UInt                        fixKeyFilterSize;// Fixed Key Filter 크기
    
    smiRange                  * varKeyRangeArea; // Variable Key Range 영역
    smiRange                  * varKeyRange;     // Variable Key Range
    UInt                        varKeyRangeSize; // Variable Key Range 크기

    smiRange                  * varKeyFilterArea; // Variable Key Filter 영역
    smiRange                  * varKeyFilter;    // Variable Key Filter
    UInt                        varKeyFilterSize;// Variable Key Filter 크기

    smiRange                  * keyRange;       // 최종 Key Range
    smiRange                  * keyFilter;      // 최종 Key Filter
    smiRange                  * ridRange;

    // Filter 관련 CallBack 정보
    smiCallBack                 callBack;        // Filter CallBack
    qtcSmiCallBackDataAnd       callBackDataAnd; //
    qtcSmiCallBackData          callBackData[3]; // 세 종류의 Filter가 가능함.

    qmndScanFixedTable          fixedTable;         // BUG-42639 Monitoring Query
    smiFixedTableProperties     fixedTableProperty; // BUG-42639 Mornitoring Query
} qmndCUNT;

class qmnCUNT
{
public:
    //------------------------
    // Base Function Pointer
    //------------------------

    // 초기화
    static IDE_RC init( qcTemplate * aTemplate,
                        qmnPlan    * aPlan );

    // 종료
    static IDE_RC fini( qcTemplate * aTemplate,
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

    // COUNT(*)을 리턴
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 결과 없음을 리턴
    static IDE_RC doItLast( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );


    // PROJ-1446 Host variable을 포함한 질의 최적화
    // plan에게 data 영역에 selected method가 세팅되었음을 알려준다.
    static IDE_RC notifyOfSelectedMethodSet( qcTemplate * aTemplate,
                                             qmncCUNT   * aCodePlan );
private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncCUNT   * aCodePlan,
                             qmndCUNT   * aDataPlan );

    // Fixed Key Range를 위한 Range 공간 할당
    static IDE_RC allocFixKeyRange( qcTemplate * aTemplate,
                                    qmncCUNT   * aCodePlan,
                                    qmndCUNT   * aDataPlan );

    // Variable Key Filter를 위한 Range 공간 할당
    static IDE_RC allocFixKeyFilter( qcTemplate * aTemplate,
                                     qmncCUNT   * aCodePlan,
                                     qmndCUNT   * aDataPlan );

    // Variable Key Range를 위한 Range 공간 할당
    static IDE_RC allocVarKeyRange( qcTemplate * aTemplate,
                                    qmncCUNT   * aCodePlan,
                                    qmndCUNT   * aDataPlan );

    // Variable Key Filter를 위한 Range 공간 할당
    static IDE_RC allocVarKeyFilter( qcTemplate * aTemplate,
                                     qmncCUNT   * aCodePlan,
                                     qmndCUNT   * aDataPlan );

    // Rid Filter 를 위한 Range 공간 할당
    static IDE_RC allocRidRange( qcTemplate * aTemplate,
                                 qmncCUNT   * aCodePlan,
                                 qmndCUNT   * aDataPlan );

    // Dependent Tuple의 변경 여부 검사
    static IDE_RC checkDependency( qmndCUNT   * aDataPlan,
                                   idBool     * aDependent );

    // COUNT(*) 값 획득
    static IDE_RC getCountValue( qcTemplate * aTemplate,
                                 qmncCUNT   * aCodePlan,
                                 qmndCUNT   * aDataPlan );

    // Handle을 이용한 COUNT(*)값 획득
    static IDE_RC getCountByHandle( qcTemplate * aTemplate,
                                    qmncCUNT   * aCodePlan,
                                    qmndCUNT   * aDataPlan );

    // Cursor를 이용한 COUNT(*)값 획득
    static IDE_RC getCountByCursor( qcTemplate * aTemplate,
                                    qmncCUNT   * aCodePlan,
                                    qmndCUNT   * aDataPlan );

    /* BUG-42639 Monitoring query */
    static IDE_RC getCountByFixedTable( qcTemplate * aTemplate,
                                        qmncCUNT   * aCodePlan,
                                        qmndCUNT   * aDataPlan );
    //------------------------
    // Cursor 관련 함수
    //------------------------

    static IDE_RC makeKeyRangeAndFilter( qcTemplate * aTemplate,
                                         qmncCUNT   * aCodePlan,
                                         qmndCUNT   * aDataPlan );

    static IDE_RC makeRidRange( qcTemplate * aTemplate,
                                qmncCUNT   * aCodePlan,
                                qmndCUNT   * aDataPlan );

    // Cursor의 Open
    static IDE_RC openCursor( qcTemplate * aTemplate,
                              qmncCUNT   * aCodePlan,
                              qmndCUNT   * aDataPlan );

    //------------------------
    // Plan Display 관련 함수
    //------------------------

    // Access정보를 출력한다.
    static IDE_RC printAccessInfo( qmncCUNT     * aCodePlan,
                                   qmndCUNT     * aDataPlan,
                                   iduVarString * aString,
                                   qmnDisplay     aMode );

    // Predicate의 상세 정보를 출력한다.
    static IDE_RC printPredicateInfo( qcTemplate   * aTemplate,
                                      qmncCUNT     * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString );

    // Key Range의 상세 정보를 출력
    static IDE_RC printKeyRangeInfo( qcTemplate   * aTemplate,
                                     qmncCUNT     * aCodePlan,
                                     ULong          aDepth,
                                     iduVarString * aString );

    // Key Filter의 상세 정보를 출력
    static IDE_RC printKeyFilterInfo( qcTemplate   * aTemplate,
                                      qmncCUNT     * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString );

    // Filter의 상세 정보를 출력
    static IDE_RC printFilterInfo( qcTemplate   * aTemplate,
                                   qmncCUNT     * aCodePlan,
                                   ULong          aDepth,
                                   iduVarString * aString );


    // code plan의 qmncScanMethod를 사용할 지,
    // sdf의 candidate들 중의 qmncScanMethod를 사용할 지 판단한다.
    static qmncScanMethod * getScanMethod( qcTemplate * aTemplate,
                                           qmncCUNT   * aCodePlan );
};

#endif /* _O_QMN_COUNT_ASTERISK_H_ */
