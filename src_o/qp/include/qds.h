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
 * $Id: qds.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QDS_H_
#define _O_QDS_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

#define QDS_SEQ_TABLE_SUFFIX_STR   ((SChar*)"$SEQ")
#define QDS_SEQ_TABLE_SUFFIX_LEN   (4)

// parameter may vary during implementation.
typedef struct qdsCallBackInfo
{
    // 외부에서 초기화해야함
    idvSQL              * mStatistics;
    void                * mTableHandle;
    smSCN                 mTableSCN;

    // callback 내부에서 사용함
    smiTrans              mSmiTrans;
    smiStatement          mSmiStmt;
    smiStatement        * mSmiStmtOrg;

    // callback 내부에서 사용함
    smiTableCursor        mCursor;
    smiCursorProperties   mProperty;
    mtcColumn           * mLastSyncSeqColumn;
    mtcColumn           * mLastTimeColumn;
    smiColumnList         mUpdateColumn[2];  // last_sync_seq, last_time
    
} qdsCallBackInfo;

// qd Sequence -- create sequence, alter sequence
class qds
{
public:
    // validate.
    static IDE_RC validateCreate(
        qcStatement * aStatement);
    static IDE_RC validateAlterOptions(
        qcStatement * aStatement);
    static IDE_RC validateAlterSeqTable(
        qcStatement * aStatement);
    static IDE_RC validateAlterFlushCache(
        qcStatement * aStatement);

    // execute
    static IDE_RC executeCreate(
        qcStatement * aStatement);
    static IDE_RC executeAlterOptions(
        qcStatement * aStatement);
    static IDE_RC executeAlterSeqTable(
        qcStatement * aStatement);
    static IDE_RC executeAlterFlushCache(
        qcStatement * aStatement);

    // BUG-17455
    static IDE_RC checkSequence(
        qcStatement      * aStatement,
        qcParseSeqCaches * aSequence,
        UInt               aReadFlag );

    static IDE_RC checkSequenceExist(
        qcStatement      * aStatement,
        qcNamePosition     aUserName,
        qcNamePosition     aSequenceName,
        UInt             * aUserID,
        qcmSequenceInfo  * aSequenceInfo,
        void            ** aSequenceHandle);

    static IDE_RC createSequenceTable(
        qcStatement     * aStatement,
        qcNamePosition    aSeqTableName,
        SLong             aStartValue,
        SLong             aIncValue,
        SLong             aMinValue,
        SLong             aMaxValue,
        SLong             aCacheValue,
        UInt              aCycleOption,
        SLong             aLastSeqValue,
        qcmTableInfo   ** aSeqTableInfo );
    
    static IDE_RC dropSequenceTable(
        qcStatement   * aStatement,
        qcmTableInfo  * aTableInfo );

    static IDE_RC selectCurrValTx(
        SLong  * aCurrVal,
        void   * aInfo );

    static IDE_RC updateLastValTx(
        SLong    aLastVal,
        void   * aInfo );
};

#endif // _O_QDS_H_
