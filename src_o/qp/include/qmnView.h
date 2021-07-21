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
 * $Id: qmnView.h 82075 2018-01-17 06:39:52Z jina.kim $ 
 *
 * Description :
 *     VIEW(VIEW) Node
 *
 *     관계형 모델에서 가상 Table을 표현하기 위한 Node이다..
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_VIEW_H_
#define _O_QMN_VIEW_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------
// Data Node Flags
//-----------------

// qmndVIEW.flag
// First Initialization Done
#define QMND_VIEW_INIT_DONE_MASK           (0x00000001)
#define QMND_VIEW_INIT_DONE_FALSE          (0x00000000)
#define QMND_VIEW_INIT_DONE_TRUE           (0x00000001)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT * FROM (SELECT i1, i2 FROM T1)V1;
 *
 *           ---------------------
 *       T1  | i1 | i2 | i3 | i4 |
 *           ~~~~~~~~~~~~~~~~~~~~~
 *           -----------
 *       V1  | i1 | i2 |
 *           ~~~~~~~~~~~
 *            mtc->mtc
 *             |    |   
 *            qtc  qtc  
 *
 *  in qmncSCAN                                       
 *      myNode : qmc i1
 *      inTarget : T1.i1 -> T1.i2
 *  in qmndGRAG
 ----------------------------------------------------------------------*/

typedef struct qmncVIEW  
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // VIEW 관련 정보
    //---------------------------------

    qtcNode      * myNode;    // View의 Column 정보

    //---------------------------------
    // Display 관련 정보
    //---------------------------------
    
    qmsNamePosition viewOwnerName;
    qmsNamePosition viewName;
    qmsNamePosition aliasName;

} qmncVIEW;

typedef struct qmndVIEW
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan       plan;
    doItFunc       doIt;        
    UInt         * flag;        

    //---------------------------------
    // VIEW 고유 정보
    //---------------------------------

} qmndVIEW;

class qmnVIEW
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

    // 하위 PROJ 의 결과를 Record로 구성함.
    static IDE_RC doItProject(  qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );

private:

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncVIEW   * aCodePlan,
                             qmndVIEW   * aDataPlan );

    // Stack에 쌓인 정보를 이용하여 View Record를 구성함.
    static IDE_RC setTupleRow( qcTemplate * aTemplate,
                               qmncVIEW   * aCodePlan,
                               qmndVIEW   * aDataPlan );
};

#endif /* _O_QMN_VIEW_H_ */
