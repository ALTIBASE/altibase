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
 * $Id: qmgShardSelect.h 76914 2016-08-30 04:16:27Z hykim $
 *
 * Description : Shard select graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_SHARD_SELECT_H_
#define _O_QMG_SHARD_SELECT_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmoOneNonPlan.h>

//---------------------------------------------------
// Shard graph의 Define 상수
//---------------------------------------------------

#define QMG_SHARD_FLAG_CLEAR                     (0x00000000)

//---------------------------------------------------
// Shard graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgShardSELT
{
    // 공통 정보
    qmgGraph           graph;

    // 고유 정보
    qcNamePosition     shardQuery;
    sdiAnalyzeInfo   * shardAnalysis;
    qcShardParamInfo * shardParamInfo; /* TASK-7219 Non-shard DML */
    UShort             shardParamOffset;
    UShort             shardParamCount;

    qmsLimit         * limit;            // 상위 PROJ 노드 생성시, limit start value 조정의 정보가 됨.
    qmoAccessMethod  * accessMethod;     // full scan
    UInt               accessMethodCnt;
    UInt               flag;

    /* TASK-7219 */
    SChar            * mShardQueryBuf;
    SInt               mShardQueryLength;
} qmgShardSELT;


/* TASK-7219 */
typedef struct qmgPushPred
{
    SInt          mId;        
    qtcNode     * mNode;
    qmgPushPred * mNext;
} qmgPushPred;

typedef struct qmgTransformInfo
{
    iduVarString      * mString;
    qmgShardSELT      * mGraph;
    qcStatement       * mStatement;
    qmsQuerySet       * mQuerySet;
    qmsTarget         * mTarget;
    qmgPushPred       * mPushPred;
    qtcNode           * mWhere;
    qmsSortColumns    * mOrderBy;
    qmsLimit          * mLimit;
    qmsForUpdate      * mForUpdate; /* BUG-48961 */
    qcParamOffsetInfo * mParamOffsetInfo;
    idBool              mNeedWrapSet;
    idBool              mUseShardKwd; /* TASK-7219 Shard Transformer Refactoring */
    idBool              mIsTransform;
    qcNamePosition      mPosition;
} qmgTransformInfo;

//---------------------------------------------------
// Shard graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgShardSelect
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmsFrom     * aFrom,
                         qmgGraph   ** aGraph );

    // Graph의 최적화 수행
    static IDE_RC optimize( qcStatement * aStatement,
                            qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC makePlan( qcStatement * aStatement,
                            const qmgGraph * aParent, 
                            qmgGraph * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC printGraph( qcStatement  * aStatement,
                              qmgGraph     * aGraph,
                              ULong          aDepth,
                              iduVarString * aString );

    static IDE_RC printAccessMethod( qcStatement     * aStatement,
                                     qmoAccessMethod * aAccessMethod,
                                     ULong             aDepth,
                                     iduVarString    * aString );

private:

    /* TASK-7219 */
    static IDE_RC copySelect( qmgTransformInfo * aInfo );

    static IDE_RC copyFrom( qmgTransformInfo * aInfo );

    static IDE_RC copyWhere( qmgTransformInfo * aInfo );

    static IDE_RC generateTargetText( qmgTransformInfo * aInfo,
                                      qmsTarget        * aTarget );

    static IDE_RC generatePredText( qmgTransformInfo * aInfo,
                                    qtcNode          * aNode );

    static IDE_RC pushProjectOptimize( qmgTransformInfo * aInfo );

    static IDE_RC pushSelectOptimize( qmgTransformInfo * aInfo );

    static IDE_RC pushOrderByOptimize( qmgTransformInfo * aInfo );

    static IDE_RC pushLimitOptimize( qmgTransformInfo * aInfo );

    static IDE_RC pushForUpdate( qmgTransformInfo * aInfo );

    static IDE_RC pushSeriesOptimize( qcStatement    * aStatement,
                                      const qmgGraph * aParent,
                                      qmgShardSELT   * aMyGraph );

    static IDE_RC setShardKeyValue( qmgTransformInfo * aInfo );

    static IDE_RC setShardParameter( qmgTransformInfo * aInfo );

    static IDE_RC setShardTupleColumn( qmgTransformInfo * aInfo );

    static IDE_RC makePushPredicate( qcStatement  * aStatement,
                                     qmgShardSELT * aMyGraph,
                                     qmsParseTree * aViewParseTree,
                                     qmsSFWGH     * aOuterQuery,
                                     SInt         * aRemain,
                                     qmoPredicate * aPredicate,
                                     qmgPushPred ** aHead,
                                     qmgPushPred ** aTail );

    static IDE_RC checkPushProject( qcStatement  * aStatement,
                                    qmgShardSELT * aMyGraph,
                                    qmsParseTree * aParseTree,
                                    qmsTarget   ** aTarget,
                                    idBool       * aNeedWrapSet );

    static IDE_RC checkPushSelect( qcStatement  * aStatement,
                                   qmgShardSELT * aMyGraph,
                                   qmsParseTree * aViewParseTree,
                                   qmsSFWGH     * aOuterQuery,
                                   qmgPushPred ** aPredicate,
                                   qtcNode     ** aWhere,
                                   idBool       * aNeedWrapSet );

    static IDE_RC checkPushLimit( qcStatement     * aStatement,
                                  const qmgGraph  * aParent,
                                  qmgShardSELT    * aMyGraph,
                                  qmsParseTree    * aParseTree,
                                  qmsSortColumns ** aOrderBy,
                                  qmsLimit       ** aLimit );

    static IDE_RC checkAndGetPushSeries( qcStatement      * aStatement,
                                         const qmgGraph   * aParent,
                                         qmgShardSELT     * aMyGraph,
                                         qmgTransformInfo * aInfo );
};

#endif /* _O_QMG_SHARD_SELECT_H_ */
