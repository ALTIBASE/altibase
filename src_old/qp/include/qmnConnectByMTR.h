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
 * $Id $
 *
 * Description :
 *     CMTR(Connect By MaTeRialization) Node
 *
 *     관계형 모델에서 Hierarchy를 위해 Materialization을 수행하는 Node이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_CONNECT_BY_MTR_H_
#define _O_QMN_CONNECT_BY_MTR_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmcSortTempTable.h>
#include <qmnDef.h>

// First Initialization Done
#define QMND_CMTR_INIT_DONE_MASK           (0x00000001)
#define QMND_CMTR_INIT_DONE_FALSE          (0x00000000)
#define QMND_CMTR_INIT_DONE_TRUE           (0x00000001)

// when Prepare-Execute, Execute is Done
#define QMND_CMTR_PRINTED_MASK             (0x00000002)
#define QMND_CMTR_PRINTED_FALSE            (0x00000000)
#define QMND_CMTR_PRINTED_TRUE             (0x00000002)

typedef struct qmncCMTR
{
    /* Code 영역 공통 정보 */
    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    /* CMTR 관련 정보 */
    qmcMtrNode   * myNode;

    /* Data 영역 관련 정보 */
    UInt           mtrNodeOffset;

    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache
} qmncCMTR;

typedef struct qmndCMTR
{
    /* Data 영역 공통 정보 */
    qmndPlan         plan;
    doItFunc         doIt;      
    UInt           * flag;      

    /* CMTR 고유 정보 */
    qmdMtrNode     * mtrNode;
    qmdMtrNode    ** priorNode;
    UInt             priorCount;
    qmcdSortTemp   * sortMgr;
    UInt             rowSize;
    void           * mtrRow;

    /* PROJ-2462 Result Cache */
    qmndResultCache  resultData;
} qmndCMTR;

class qmnCMTR
{
public:

    /* 초기화 */
    static IDE_RC init( qcTemplate * aTemplate,
                        qmnPlan    * aPlan );

    /* 수행 함수(호출되면 안됨) */
    static IDE_RC doIt( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag );

    static IDE_RC padNull( qcTemplate * aTemplate,
                           qmnPlan    * aPlan );

    /* Plan 정보 출력 */
    static IDE_RC printPlan( qcTemplate   * aTemplate,
                             qmnPlan      * aPlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode );

    /* HIER에서 Null Row를 획득하기 위해 호출 */
    static IDE_RC getNullRow( qcTemplate       * aTemplate,
                              qmnPlan          * aPlan,
                              void             * aRow );

    /* HIER에서 Row Size를 획득하기 위해 호출 */
    static IDE_RC getNullRowSize( qcTemplate       * aTemplate,
                                  qmnPlan          * aPlan,
                                  UInt             * aRowSize );

    /* HIER에서 tuple을 획득하기 위해 호출 */
    static IDE_RC getTuple( qcTemplate       * aTemplate,
                            qmnPlan          * aPlan,
                            mtcTuple        ** aTuple );
    
    static IDE_RC setPriorNode( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qtcNode    * aPrior );

    static IDE_RC comparePriorNode( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan,
                                    void       * aPriorRow,
                                    void       * aSearchRow,
                                    idBool     * aResult );
private:

    /* 최초 초기화 */
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncCMTR   * aCodePlan,
                             qmndCMTR   * aDataPlan );

    /* 저장 Column의 초기화 */
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncCMTR   * aCodePlan,
                               qmndCMTR   * aDataPlan );

    /* Temp Table의 초기화 */
    static IDE_RC initTempTable( qcTemplate * aTemplate,
                                 qmncCMTR   * aCodePlan,
                                 qmndCMTR   * aDataPlan );

    /* Child의 수행 결과를 저장 */
    static IDE_RC storeChild( qcTemplate * aTemplate,
                              qmncCMTR   * aCodePlan,
                              qmndCMTR   * aDataPlan );

    /* 저장 Row를 구성 */
    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndCMTR   * aDataPlan );
};

#endif /* _O_QMN_CONNECT_BY_MTR_H_ */

