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
 * Description :
 *     SDIN(SharD INsert) Node
 *
 *     관계형 모델에서 insert를 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_SHARD_INST_H_
#define _O_QMN_SHARD_INST_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qcmTableInfo.h>
#include <qmmParseTree.h>
#include <qmnShardDML.h>
#include <sdi.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------
// Data Node Flags
//-----------------

// qmndSDIN.flag
# define QMND_SDIN_INIT_DONE_MASK           (0x00000001)
# define QMND_SDIN_INIT_DONE_FALSE          (0x00000000)
# define QMND_SDIN_INIT_DONE_TRUE           (0x00000001)

// qmndSDIN.flag
# define QMND_SDIN_INSERT_MASK              (0x00000002)
# define QMND_SDIN_INSERT_FALSE             (0x00000000)
# define QMND_SDIN_INSERT_TRUE              (0x00000002)

typedef struct qmncSDIN
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan               plan;
    UInt                  flag;
    UInt                  planID;

    //---------------------------------
    // querySet 관련 정보
    //---------------------------------

    qmsTableRef         * tableRef;

    //---------------------------------
    // insert 관련 정보
    //---------------------------------

    idBool                isInsertSelect;

    // update columns
    qcmColumn           * columns;
    qcmColumn           * columnsForValues;

    // update values
    qmmMultiRows        * rows;       /* BUG-42764 Multi Row */
    UInt                  valueIdx;   // update smiValues index
    UInt                  canonizedTuple;
    void                * queueMsgIDSeq;

    // sequence 정보
    qcParseSeqCaches    * nextValSeqs;

    //---------------------------------
    // 고유 정보
    //---------------------------------

    UInt                  shardDataIndex;
    UInt                  shardDataOffset;
    UInt                  shardDataSize;

    UInt                  bindParam;    // offset
    UInt                  outBindParam; // offset

    qcNamePosition        shardQuery;
    sdiAnalyzeInfo      * shardAnalysis;
    UShort                shardParamCount;

    //---------------------------------
    // Display 관련 정보
    //---------------------------------

    qmsNamePosition       tableOwnerName;     // Table Owner Name
    qmsNamePosition       tableName;          // Table Name
    qmsNamePosition       aliasName;          // Alias Name

} qmncSDIN;

typedef struct qmndSDIN
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan              plan;
    doItFunc              doIt;
    UInt                * flag;

    //---------------------------------
    // INST 고유 정보
    //---------------------------------

    /* BUG-42764 Multi Row */
    qmmMultiRows        * rows;   // Current row
    //---------------------------------
    // lob 처리를 위한 정보
    //---------------------------------
    
    struct qmxLobInfo   * lobInfo;

    sdiDataNodes        * mDataInfo;

} qmndSDIN;

class qmnSDIN
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

    static void shardStmtPartialRollbackUsingSavepoint( qcTemplate    * aTemplate,
                                                        qmnPlan       * aPlan );
private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncSDIN   * aCodePlan,
                             qmndSDIN   * aDataPlan );

    static IDE_RC setParamInfo( qcTemplate   * aTemplate,
                                qmncSDIN     * aCodePlan,
                                sdiBindParam * aBindParams );

    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 최초 INST을 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 INST을 수행
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // 최초 INST을 수행
    static IDE_RC doItFirstMultiRows( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan,
                                      qmcRowFlag * aFlag );

    // 다음 INST을 수행
    static IDE_RC doItNextMultiRows( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan,
                                     qmcRowFlag * aFlag );

    // Insert One Record
    static IDE_RC insertOneRow( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );

    // Insert One Record
    static IDE_RC insertOnce( qcTemplate * aTemplate,
                              qmnPlan    * aPlan );

    static IDE_RC copySmiValueToTuple( qcTemplate      * aTemplate,
                                       qcmTableInfo    * aTableInfo,
                                       smiValue        * aInsertedRow,
                                       mtcTuple        * aTuple,
                                       sdiBindParam    * aBindParams,
                                       sdiOutBindParam * aOutBindParams,
                                       qmxLobInfo      * aLobInfo );
                                   

    /* PROJ-2728 Sharding LOB */
    static idBool existInLobInfoAndAdjustBindId(
                                       qmxLobInfo   * aLobInfo,
                                       SInt           aColumnOrder,
                                       SInt           aBindId );
};

#endif /* _O_QMN_SHARD_INST_H_ */
