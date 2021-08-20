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
 * $Id$ 
 *
 * Description : SDSL(SharD SElect) Node
 *
 *     Shard data node 에 대한 scan을 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_SDSE_H_
#define _O_QMN_SDSE_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qmnShardDML.h>
#include <sdi.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------
// Data Node Flags
//-----------------

// qmndSDSE.flag
// First Initialization Done
#define QMND_SDSE_INIT_DONE_MASK           (0x00000001)
#define QMND_SDSE_INIT_DONE_FALSE          (0x00000000)
#define QMND_SDSE_INIT_DONE_TRUE           (0x00000001)

#define QMND_SDSE_ALL_FALSE_MASK           (0x00000002)
#define QMND_SDSE_ALL_FALSE_FALSE          (0x00000000)
#define QMND_SDSE_ALL_FALSE_TRUE           (0x00000002)

typedef struct qmncSDSE
{
    //---------------------------------
    // 공통 정보
    //---------------------------------

    qmnPlan          plan;
    UInt             flag;
    UInt             planID;

    //---------------------------------
    // 고유 정보
    //---------------------------------

    qmsTableRef    * tableRef;
    UShort           tupleRowID;

    qtcNode        * constantFilter;
    qtcNode        * subqueryFilter;
    qtcNode        * nnfFilter;
    qtcNode        * filter;
    qtcNode        * lobFilter;       // Lob Filter ( BUG-25916 ) 

    UInt             shardDataIndex;
    UInt             shardDataOffset;
    UInt             shardDataSize;

    UInt             mBuffer[SDI_NODE_MAX_COUNT];  // offset
    UInt             mOffset;       // offset
    UInt             mMaxByteSize;  // offset
    UInt             mBindParam;    // offset
    UInt             mOutBindParam; // offset

    /* TASK-7219 Non-shard DML */
    UInt             mOutRefBindData; // offset

    qcNamePosition * mQueryPos;
    sdiAnalyzeInfo * mShardAnalysis;
    qcShardParamInfo * mShardParamInfo; /* TASK-7219 Non-shard DML */
    UShort           mShardParamCount;

    UInt             mRowForTransformed; /* BUG-49154 */

} qmncSDSE;

typedef struct qmndSDSE
{
    //---------------------------------
    // 공통 정보
    //---------------------------------

    qmndPlan       plan;
    doItFunc       doIt;
    UInt         * flag;

    UInt           lobBindCount; // PROJ-2728

    //---------------------------------
    // Disk Table 관련 정보
    //---------------------------------

    void         * nullRow;  // Disk Table을 위한 null row
    scGRID         nullRID;

    //---------------------------------
    // 고유 정보
    //---------------------------------

    UShort         mCurrScanNode;
    UShort         mScanDoneCount;

    sdiDataNodes * mDataInfo;

} qmndSDSE;

class qmnSDSE
{
public:

    //------------------------
    // Base Function Pointer
    //------------------------

    static IDE_RC init( qcTemplate * aTemplate,
                        qmnPlan    * aPlan );

    //------------------------
    // 수행 함수
    // mapping by doIt() function pointer
    //------------------------

    static IDE_RC doIt( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag );

    static IDE_RC doItAllFalse( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );

    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    //------------------------
    // Null Padding
    //------------------------

    static IDE_RC padNull( qcTemplate * aTemplate,
                           qmnPlan    * aPlan );

    //------------------------
    // Plan 정보 출력
    //------------------------

    static IDE_RC printPlan( qcTemplate   * aTemplate,
                             qmnPlan      * aPlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode );

    //------------------------
    // TASK-7219 Non-shard DML
    // Transformed out ref column bind
    //------------------------

    static IDE_RC setTransformedOutRefBindValue( qcTemplate * aTemplate,
                                                 qmncSDSE   * aCodePlan );

private:

    //------------------------
    // 최초 초기화
    //------------------------

    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncSDSE   * aCodePlan,
                             qmndSDSE   * aDataPlan );
    
    static IDE_RC setParamInfo( qcTemplate   * aTemplate,
                                qmncSDSE     * aCodePlan,
                                sdiBindParam * aBindParams,
                                void         * aOutRefBindData,
                                UInt         * aLobBindCount );

    static IDE_RC setLobInfo( qcTemplate   * aTemplate,
                              qmncSDSE     * aCodePlan );

    /* BUG-49154 */
    static IDE_RC makeNullColumn( qcTemplate * aTemplate,
                                  qmndSDSE   * aDataPlan,
                                  void       * aRow );
};

#endif /* _O_QMN_SDSE_H_ */
