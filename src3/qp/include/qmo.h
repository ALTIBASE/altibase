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
 * $Id: qmo.h 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     Query Optimizer
 *
 *     Optimizer를 접근하는 최상위 Interface로 구성됨
 *     Graph 생성 및 Plan Tree를 생성한다.
 *
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_H_
#define _O_QMO_H_ 1

#include <qc.h>
#include <qmoDef.h>
#include <qmsParseTree.h>
#include <qmc.h>
#include <qmmParseTree.h>

//---------------------------------------------------
// Optimizer 수행 함수
//---------------------------------------------------

class qmo
{
public:

    // SELECT 구문에 대한 최적화
    static IDE_RC    optimizeSelect( qcStatement * aStatement );

    // INSERT 구문에 대한 최적화
    static IDE_RC    optimizeInsert( qcStatement * aStatement );
    static IDE_RC    optimizeMultiInsertSelect( qcStatement * aStatement );
    
    // UPDATE 구문에 대한 최적화
    static IDE_RC    optimizeUpdate( qcStatement * aStatement );

    // DELETE 구문에 대한 최적화
    static IDE_RC    optimizeDelete( qcStatement * aStatement );

    // MOVE 구문에 대한 최적화
    static IDE_RC    optimizeMove( qcStatement * aStatement );

    /* PROJ-1988 Implement MERGE statement */
    static IDE_RC	 optimizeMerge( qcStatement * aStatement );

    // Shard DML
    static IDE_RC    optimizeShardDML( qcStatement * aStatement );

    // Shard Insert
    static IDE_RC    optimizeShardInsert( qcStatement * aStatement );

    // Sub SELECT 구문에 대한 최적화 ( CREATE AS SELECT )
    static IDE_RC    optimizeCreateSelect( qcStatement * aStatement );


    // Graph의 생성 및 초기화
    // Optimizer 외부에서 호출하면 안됨.
    // optimizeSelect와 qmoSubquery::makeGraph가 호출
    static IDE_RC    makeGraph( qcStatement * aStatement );

    // PROJ-2205 Rownum in DML
    static IDE_RC    makeInsertGraph( qcStatement * aStatement );
    static IDE_RC    makeMultiInsertGraph( qcStatement * aStatement );

    // PROJ-2205 Rownum in DML
    static IDE_RC    makeUpdateGraph( qcStatement * aStatement );

    // PROJ-2205 Rownum in DML
    static IDE_RC    makeDeleteGraph( qcStatement * aStatement );

    // PROJ-2205 Rownum in DML
    static IDE_RC    makeMoveGraph( qcStatement * aStatement );

    // PROJ-2205 Rownum in DML
    static IDE_RC    makeMergeGraph( qcStatement * aStatement );

    // Shard Insert
    static IDE_RC    makeShardInsertGraph( qcStatement * aStatement );

    // PROJ-1413
    // Query Transformation의 수행
    // ParseTree로 Transformed ParseTree를 생성한다.
    static IDE_RC    doTransform( qcStatement * aStatement );
 
    static IDE_RC    doTransformSubqueries( qcStatement * aStatement,
                                            qtcNode     * aPredicate );

    // 현재 Join Graph에 해당하는 dependencies인지 검사
    static IDE_RC    currentJoinDependencies( qmgGraph  * aJoinGraph,
                                              qcDepInfo * aDependencies,
                                              idBool    * aIsCurrent );

    static IDE_RC    currentJoinDependencies4JoinOrder( qmgGraph  * aJoinGraph,
                                                        qcDepInfo * aDependencies,
                                                        idBool    * aIsCurrent );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // bind 후 아래 함수를 통해서 scan method를 재설정한다.
    static IDE_RC    optimizeForHost( qcStatement * aStatement );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // 위 함수를 통해서 재설정된 access method를 구한다.
    // execution시에 dataPlan에 의해 호출된다.
    static qmncScanMethod * getSelectedMethod(
        qcTemplate            * aTemplate,
        qmoScanDecisionFactor * aSDF,
        qmncScanMethod        * aDefaultMethod );

    // PROJ-2462 Result Cache Hint와 Property 를 통해서 Result Cache여부를
    // 표시한다.
    static IDE_RC setResultCacheFlag( qcStatement * aStatement );

    // PROJ-2462 Result Cache
    static IDE_RC initResultCacheStack( qcStatement   * aStatement,
                                        qmsQuerySet   * aQuerySet,
                                        UInt            aPlanID,
                                        idBool          aIsTopResult,
                                        idBool          aIsVMTR );

    // PROJ-2462 Result Cache
    static void makeResultCacheStack( qcStatement      * aStatement,
                                      qmsQuerySet      * aQuerySet,
                                      UInt               aPlanID,
                                      UInt               aPlanFlag,
                                      ULong              aTupleFlag,
                                      qmcMtrNode       * aMtrNode,
                                      qcComponentInfo ** aInfo,
                                      idBool             aIsTopResult );

    // PROJ-2462 Result Cache
    static void flushResultCacheStack( qcStatement * aStatement );

    static void addTupleID2ResultCacheStack( qcStatement * aStatement,
                                             UShort        aTupleID );

    /* PROJ-2714 Multiple Update Delete support */
    static IDE_RC optimizeMultiUpdate( qcStatement * aStatement );
    static IDE_RC optimizeMultiDelete( qcStatement * aStatement );

    /* TASK-7219 Non-shard DML */
    static IDE_RC removeOutRefPredPushedForce( qmoPredicate ** aPredicate );

private:
    // PROJ-2462 Result Cache
    static IDE_RC pushComponentInfo( qcTemplate    * aTemplate,
                                     iduVarMemList * aMemory,
                                     UInt            aPlanID ,
                                     idBool          aIsVMTR );

    // PROJ-2462 Result Cache
    static void popComponentInfo( qcTemplate       * aTemplate,
                                  idBool             aIsPossible,
                                  qcComponentInfo ** aInfo );

    static IDE_RC makeTopResultCacheGraph( qcStatement * aStatement );
    static void checkQuerySet( qmsQuerySet * aQuerySet, idBool * aIsPossible );
    static void checkFromTree( qmsFrom * aFrom, idBool * aIsPossible );

    /* BUG-44228  merge 실행시 disk table이고 hash join 인 경우 의도하지 않는 값으로 변경 됩니다. */
    static IDE_RC adjustValueNodeForMerge( qcStatement  * aStatement,
                                           qmcAttrDesc  * sResultDesc,
                                           qmmValueNode * sValueNode );

    /* BUG-44228  merge 실행시 disk table이고 hash join 인 경우 의도하지 않는 값으로 변경 됩니다. */
    static IDE_RC adjustArgumentNodeForMerge( qcStatement  * aStatement,
                                              mtcNode      * sSrcNode,
                                              mtcNode      * sDstNode );
};

#endif /* _O_QMO_H_ */
