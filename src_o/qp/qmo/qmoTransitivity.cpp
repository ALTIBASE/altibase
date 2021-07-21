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
 * $Id: qmoTransitivity.cpp 23857 2007-10-23 02:36:53Z sungminee $
 **********************************************************************/

#include <idl.h>
#include <qtc.h>
#include <qmoPredicate.h>
#include <qmoTransitivity.h>
#include <qcg.h>

extern mtfModule mtfEqual;
extern mtfModule mtfNotEqual;
extern mtfModule mtfLessThan;
extern mtfModule mtfLessEqual;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfBetween;
extern mtfModule mtfNotBetween;
extern mtfModule mtfLike;
extern mtfModule mtfNotLike;
extern mtfModule mtfEqualAny;
extern mtfModule mtfNotEqualAll;
extern mtfModule mtfList;

// 12개의 연산자를 지원한다.
const qmoTransOperatorModule qmoTransMgr::operatorModule[] = {
    { & mtfEqual       , 0           }, // 0
    { & mtfNotEqual    , 1           }, // 1
    { & mtfLessThan    , 4           }, // 2
    { & mtfLessEqual   , 5           }, // 3
    { & mtfGreaterThan , 2           }, // 4
    { & mtfGreaterEqual, 3           }, // 5
    { & mtfBetween     , ID_UINT_MAX }, // 6
    { & mtfNotBetween  , ID_UINT_MAX }, // 7
    { & mtfLike        , ID_UINT_MAX }, // 8
    { & mtfNotLike     , ID_UINT_MAX }, // 9
    { & mtfEqualAny    , ID_UINT_MAX }, // 10
    { & mtfNotEqualAll , ID_UINT_MAX }, // 11
    { NULL             , ID_UINT_MAX }  // 12
};

// ID_UINT_MAX를 사용하면 읽기가 어려워 잠시 짧은 단어로 define한다.
#define XX     ((UInt)0xFFFFFFFF)  // ID_UINT_MAX

/*
 * DOC-31095 Transitive Predicate Generation.odt p52
 * <transitive operation matrix>
 *
 * BUG-29444: not equal과 between 연산자에 의해 생성되는
 *            transitive predicate에 오류
 *
 * a <> b AND b BETWEEN x and y 인 경우
 * a NOT BETWEEN x and y 가 추가되도록 되어있었는데 이는 잘못된 predicate 임
 *
 * 이와 더불어 a <> b 인 경우 between, like, in 등도 모두 성립하지 않는다
 * 따라서 matrix 두번째 행을 첫번째 열만 빼고 모두 XX 로 변경
 */
const UInt qmoTransMgr::operationMatrix[QMO_OPERATION_MATRIX_SIZE][QMO_OPERATION_MATRIX_SIZE] = {
    {   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 },
    {   1, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX },
    {   2, XX,  2,  2, XX, XX, XX, XX, XX, XX, XX, XX },
    {   3, XX,  2,  3, XX, XX, XX, XX, XX, XX, XX, XX },
    {   4, XX, XX, XX,  4,  4, XX, XX, XX, XX, XX, XX },
    {   5, XX, XX, XX,  4,  5, XX, XX, XX, XX, XX, XX },
    {  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX },
    {  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX },
    {  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX },
    {  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX },
    {  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX },
    {  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX }
};

#undef XX


IDE_RC
qmoTransMgr::init( qcStatement        * aStatement,
                   qmoTransPredicate ** aTransPredicate )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     qmoTransPredicate 구조체를 생성하고 초기화한다.
 *
 ***********************************************************************/
    
    qmoTransPredicate * sTransPredicate;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTransPredicate != NULL );

    //------------------------------------------
    // qmoTransPredicate 생성 및 초기화
    //------------------------------------------
    
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoTransPredicate),
                                             (void **) & sTransPredicate )
              != IDE_SUCCESS );

    sTransPredicate->operatorList  = NULL;
    sTransPredicate->operandList = NULL;
    sTransPredicate->operandSet  = NULL;
    sTransPredicate->operandSetSize = 0;
    sTransPredicate->operandSetArray = NULL;
    sTransPredicate->genOperatorList = NULL;
    qtc::dependencyClear( &sTransPredicate->joinDepInfo );

    *aTransPredicate = sTransPredicate;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::addBasePredicate( qcStatement       * aStatement,
                               qmsQuerySet       * aQuerySet,
                               qmoTransPredicate * aTransPredicate,
                               qtcNode           * aNode )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     qtcNode형태의 predicate으로 operator list와 operand list를
 *     구성한다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoTransMgr::addBasePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aTransPredicate != NULL );
    IDE_DASSERT( aNode != NULL );
    
    //------------------------------------------
    // operator list, operand list 생성
    //------------------------------------------

    IDE_TEST( createOperatorAndOperandList( aStatement,
                                            aQuerySet,
                                            aTransPredicate,
                                            aNode,
                                            ID_FALSE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::addBasePredicate( qcStatement       * aStatement,
                               qmsQuerySet       * aQuerySet,
                               qmoTransPredicate * aTransPredicate,
                               qmoPredicate      * aPredicate )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     qmoPredicate형태의 predicate으로 operator list와 operand list를
 *     구성한다.
 *
 ***********************************************************************/
    
    qmoPredicate      * sPredicate;
    qtcNode           * sNode;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::addBasePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aTransPredicate != NULL );
    IDE_DASSERT( aPredicate != NULL );
    
    //------------------------------------------
    // 초기화
    //------------------------------------------

    sPredicate = aPredicate;
    sNode = aPredicate->node;
    
    //------------------------------------------
    // operator list와 operand list 생성
    //------------------------------------------

    while ( sPredicate != NULL )
    {
        IDE_TEST( createOperatorAndOperandList( aStatement,
                                                aQuerySet,
                                                aTransPredicate,
                                                sNode,
                                                ID_TRUE )
                  != IDE_SUCCESS );

        sPredicate = sPredicate->more;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::processPredicate( qcStatement        * aStatement,
                               qmsQuerySet        * aQuerySet,
                               qmoTransPredicate  * aTransPredicate,
                               qmoTransMatrix    ** aTransMatrix,
                               idBool             * aIsApplicable )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     operator list와 operand list로 transitive predicate matrix를
 *     생성하여 계산한다.
 *
 ***********************************************************************/

    qmoTransPredicate * sTransPredicate;
    qmoTransMatrix    * sTransMatrix = NULL;
    idBool              sIsApplicable;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::processPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aTransPredicate != NULL );
    IDE_DASSERT( aTransMatrix != NULL );
    IDE_DASSERT( aIsApplicable != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sTransPredicate = aTransPredicate;

    //------------------------------------------
    // operand set 생성
    //------------------------------------------

    IDE_TEST( createOperandSet( aStatement,
                                sTransPredicate,
                                & sIsApplicable )
              != IDE_SUCCESS );

    if ( sIsApplicable == ID_TRUE )
    {
        //------------------------------------------
        // qmoTransMatrix 생성 및 초기화
        //------------------------------------------

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoTransMatrix),
                                                 (void **) & sTransMatrix )
                  != IDE_SUCCESS );

        sTransMatrix->predicate = sTransPredicate;

        //------------------------------------------
        // transitive predicate matrix 생성
        //------------------------------------------

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoTransMatrixInfo* ) * sTransPredicate->operandSetSize,
                                                   (void**)& sTransMatrix->matrix )
                  != IDE_SUCCESS);

        for (i = 0;
             i < sTransPredicate->operandSetSize;
             i++)
        {
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoTransMatrixInfo )
                                                       * sTransPredicate->operandSetSize,
                                                       (void**)&( sTransMatrix->matrix[i] ) )
                      != IDE_SUCCESS );
        }

        sTransMatrix->size = sTransPredicate->operandSetSize;

        //------------------------------------------
        // transitive predicate matrix 초기화
        //------------------------------------------

        IDE_TEST( initializeTransitiveMatrix( sTransMatrix )
                  != IDE_SUCCESS );

        //------------------------------------------
        // transitive predicate matrix 계산
        //------------------------------------------

        IDE_TEST( calculateTransitiveMatrix( aStatement,
                                             aQuerySet,
                                             sTransMatrix )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    *aTransMatrix = sTransMatrix;
    *aIsApplicable = sIsApplicable;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::generatePredicate( qcStatement     * aStatement,
                                qmsQuerySet     * aQuerySet,
                                qmoTransMatrix  * aTransMatrix,
                                qtcNode        ** aTransitiveNode )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     transitive predicate matrix로 transitive predicate을 생성한다.
 *
 ***********************************************************************/
    
    qtcNode  * sTransitiveNode;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::generatePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTransMatrix != NULL );

    //------------------------------------------
    // transitive predicate 생성
    //------------------------------------------

    IDE_TEST( generateTransitivePredicate( aStatement,
                                           aQuerySet,
                                           aTransMatrix,
                                           & sTransitiveNode )
              != IDE_SUCCESS );

    *aTransitiveNode = sTransitiveNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::linkPredicate( qtcNode      * aTransitiveNode,
                            qtcNode     ** aNode )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     qtcNode형태의 predicate을 qtcNode형태로 연결한다.
 *
 ***********************************************************************/

    qtcNode  * sNode;
    mtcNode  * sMtcNode;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::linkPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aNode != NULL );
    
    //------------------------------------------
    // 초기화
    //------------------------------------------

    sNode = *aNode;
    
    //------------------------------------------
    // 연결
    //------------------------------------------

    if ( aTransitiveNode != NULL )
    {
        if ( sNode == NULL )
        {
            *aNode = aTransitiveNode;
        }
        else
        {
            sMtcNode = & sNode->node;
            
            while ( sMtcNode != NULL )
            {
                if ( sMtcNode->next == NULL )
                {
                    sMtcNode->next = (mtcNode*) aTransitiveNode;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
                
                sMtcNode = sMtcNode->next;
            }
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
}
    
IDE_RC
qmoTransMgr::linkPredicate( qcStatement   * aStatement,
                            qtcNode       * aTransitiveNode,
                            qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     qtcNode형태의 predicate을 qmoPredicate형태로 연결한다.
 *
 ***********************************************************************/

    qmoPredicate  * sTransPredicate;
    qmoPredicate  * sNewPredicate;
    qtcNode       * sNode;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::linkPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate != NULL );
    
    //------------------------------------------
    // 초기화
    //------------------------------------------

    sTransPredicate = *aPredicate;
    sNode = aTransitiveNode;

    //------------------------------------------
    // 연결
    //------------------------------------------

    while ( sNode != NULL )
    {
        // qmoPredicate를 생성한다.
        IDE_TEST( qmoPred::createPredicate( QC_QMP_MEM(aStatement),
                                            sNode,
                                            & sNewPredicate )
                  != IDE_SUCCESS );

        // qmoPredicate을 연결한다.
        sNewPredicate->next = sTransPredicate;
        sTransPredicate = sNewPredicate;

        sNode = (qtcNode*) sNode->node.next;
    }

    *aPredicate = sTransPredicate;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::linkPredicate( qmoPredicate  * aTransitivePred,
                            qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     qmoPredicate형태의 predicate을 qmoPredicate으로 연결한다.
 *
 ***********************************************************************/

    qmoPredicate  * sPredicate;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::linkPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aPredicate != NULL );
    
    //------------------------------------------
    // 초기화
    //------------------------------------------

    sPredicate = *aPredicate;
    
    //------------------------------------------
    // 연결
    //------------------------------------------

    if ( aTransitivePred != NULL )
    {
        if ( sPredicate == NULL )
        {
            *aPredicate = aTransitivePred;
        }
        else
        {
            while ( sPredicate != NULL )
            {
                if ( sPredicate->next == NULL )
                {
                    sPredicate->next = aTransitivePred;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
                
                sPredicate = sPredicate->next;
            }
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
}

IDE_RC
qmoTransMgr::createOperatorAndOperandList( qcStatement       * aStatement,
                                           qmsQuerySet       * aQuerySet,
                                           qmoTransPredicate * aTransPredicate,
                                           qtcNode           * aNode,
                                           idBool              aOnlyOneNode )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     predicate들로 operator list와 operand list를 생성한다.
 *
 ***********************************************************************/
    
    qtcNode     * sNode;
    qtcNode     * sCompareNode;
    idBool        sCheckNext;
    idBool        sIsTransitiveForm;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::createOperatorAndOperandList::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aTransPredicate != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNode = aNode;

    //------------------------------------------
    // Operator List, Operand List 생성
    //------------------------------------------
    
    while ( sNode != NULL )
    {
        if ( (sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK)
             == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            // CNF 형태인 경우
            sCompareNode = (qtcNode*) sNode->node.arguments;
            sCheckNext = ID_TRUE;
        }
        else
        {
            // DNF 형태인 경우
            sCompareNode = sNode;
            sCheckNext = ID_FALSE;
        }

        IDE_TEST( isTransitiveForm( aStatement,
                                    aQuerySet,
                                    sCompareNode,
                                    sCheckNext,
                                    & sIsTransitiveForm )
                  != IDE_SUCCESS );

        if ( sIsTransitiveForm == ID_TRUE )
        {
            // qmoTransOperator 생성
            IDE_TEST( createOperatorAndOperand( aStatement,
                                                aTransPredicate,
                                                sCompareNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        if ( aOnlyOneNode == ID_TRUE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }

        sNode = (qtcNode*)sNode->node.next;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::createOperatorAndOperand( qcStatement       * aStatement,
                                       qmoTransPredicate * aTransPredicate,
                                       qtcNode           * aCompareNode )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     하나의 predicate을 하나의 operator 노드와 두 개의 operand 노드로
 *     나누어 생성한다.
 *
 ***********************************************************************/

    qmoTransOperator  * sOperator;
    qmoTransOperand   * sLeft;
    qmoTransOperand   * sRight;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::createOperatorAndOperand::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTransPredicate != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT(
        ( (aCompareNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) >= 2 ) &&
        ( (aCompareNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) <= 3 ) );

    //------------------------------------------
    // qmoTransOperand 생성
    //------------------------------------------

    //----------------
    // left
    //----------------
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoTransOperand),
                                             (void **) & sLeft )
              != IDE_SUCCESS );

    sLeft->operandFirst = (qtcNode*) aCompareNode->node.arguments;
    sLeft->operandSecond = NULL;    
    sLeft->id = ID_UINT_MAX;   // 아직 결정되지 않았음
    sLeft->next = NULL;
    
    IDE_DASSERT( sLeft->operandFirst != NULL );
    
    IDE_TEST( isIndexColumn( aStatement,
                             sLeft->operandFirst,
                             & sLeft->isIndexColumn )
              != IDE_SUCCESS );
    
    qtc::dependencySetWithDep( & sLeft->depInfo,
                               & sLeft->operandFirst->depInfo );
    
    //----------------
    // right
    //----------------
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoTransOperand),
                                             (void **) & sRight )
              != IDE_SUCCESS );

    if ( (aCompareNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 )
    {
        sRight->operandFirst = (qtcNode*) aCompareNode->node.arguments->next;
        sRight->operandSecond = NULL;
        sRight->id = ID_UINT_MAX;
        sRight->next = NULL;
        
        IDE_DASSERT( sRight->operandFirst != NULL );
        
        IDE_TEST( isIndexColumn( aStatement,
                                 sRight->operandFirst,
                                 & sRight->isIndexColumn )
                  != IDE_SUCCESS );
    
        qtc::dependencySetWithDep( & sRight->depInfo,
                                   & sRight->operandFirst->depInfo );
    }
    else
    {
        // like, between
        sRight->operandFirst = (qtcNode*) aCompareNode->node.arguments->next;
        sRight->operandSecond = (qtcNode*) aCompareNode->node.arguments->next->next;
        sRight->id = ID_UINT_MAX;
        sRight->isIndexColumn = ID_FALSE;
        sRight->next = NULL;

        IDE_DASSERT( sRight->operandFirst != NULL );
        IDE_DASSERT( sRight->operandSecond != NULL );
        
        IDE_TEST( qtc::dependencyOr( & sRight->operandFirst->depInfo,
                                     & sRight->operandSecond->depInfo,
                                     & sRight->depInfo )
                  != IDE_SUCCESS );
    }
    
    //------------------------------------------
    // qmoTransOperator 생성
    //------------------------------------------

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoTransOperator),
                                             (void **) & sOperator )
              != IDE_SUCCESS );

    sOperator->left = sLeft;
    sOperator->right = sRight;
    sOperator->id = ID_UINT_MAX;
    sOperator->next = NULL;

    for (i = 0; operatorModule[i].module != NULL; i++)
    {
        if ( aCompareNode->node.module == operatorModule[i].module )
        {
            sOperator->id = i;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_DASSERT( sOperator->id != ID_UINT_MAX );
    
    //------------------------------------------
    // qmoTransPredicate에 연결
    //------------------------------------------

    // operand list에 연결한다.
    sRight->next = aTransPredicate->operandList;
    sLeft->next = sRight;
    aTransPredicate->operandList = sLeft;
    
    // operator list에 연결한다.
    sOperator->next = aTransPredicate->operatorList;
    aTransPredicate->operatorList = sOperator;
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::isIndexColumn( qcStatement * aStatement,
                            qtcNode     * aNode,
                            idBool      * aIsIndexColumn )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     노드가 index를 가진 컬럼노드인지 검사한다.
 *     (composite index의 경우 첫번째 컬럼만 가능하다.)
 *
 ***********************************************************************/

    idBool      sIndexColumn = ID_FALSE;
    qmsFrom   * sFrom;
    qcmIndex  * sIndices;
    UInt        sIndexCnt;
    UInt        sColumnId;
    UInt        sColumnOrder;
    UInt        i;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::isIndexColumn::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );
        
    //------------------------------------------
    // index column 인지 검사
    //------------------------------------------

    if ( QTC_IS_COLUMN( aStatement, aNode ) == ID_TRUE )
    {
        sFrom = QC_SHARED_TMPLATE(aStatement)->tableMap[aNode->node.table].from;
        sIndices = sFrom->tableRef->tableInfo->indices;
        sIndexCnt = sFrom->tableRef->tableInfo->indexCount;

        for (i = 0; i < sIndexCnt; i++)
        {
            if ( sIndices[i].keyColCount > 0 )
            {
                sColumnId = sIndices[i].keyColumns[0].column.id;
                sColumnOrder = sColumnId & SMI_COLUMN_ID_MASK;

                if ( (UInt)aNode->node.column == sColumnOrder )
                {
                    sIndexColumn = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    *aIsIndexColumn = sIndexColumn;
    
    return IDE_SUCCESS;
}

IDE_RC
qmoTransMgr::isSameCharType( qcStatement * aStatement,
                             qtcNode     * aLeftNode,
                             qtcNode     * aRightNode,
                             idBool      * aIsSameType )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     문자 type의 컬럼이 동일 type인지 검사한다.
 *
 ***********************************************************************/

    qtcNode   * sLeftNode;
    qtcNode   * sRightNode;
    mtcColumn * sLeftColumn;
    mtcColumn * sRightColumn;
    idBool      sIsSameType = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::isSameCharType::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aLeftNode != NULL );
    IDE_DASSERT( aRightNode != NULL );
    IDE_DASSERT( aIsSameType != NULL );

    //------------------------------------------
    // 컬럼 혹은 컬럼 expr이 동일 type인지 검사
    //------------------------------------------

    if ( ( qtc::dependencyEqual( & aLeftNode->depInfo,
                                 & qtc::zeroDependencies ) == ID_FALSE ) &&
         ( qtc::dependencyEqual( & aRightNode->depInfo,
                                 & qtc::zeroDependencies ) == ID_FALSE ) )
    {
        // column operator column 형태인 경우

        // type이 정해진 경우

        if ( QTC_IS_LIST( aLeftNode ) != ID_TRUE )
        {
            // left가 column list가 아닌 경우

            if ( QTC_IS_LIST( aRightNode ) != ID_TRUE )
            {
                // right가 column list가 아닌 경우

                sLeftColumn = QTC_STMT_COLUMN( aStatement, aLeftNode );
                sRightColumn = QTC_STMT_COLUMN( aStatement, aRightNode );

                if ( ( (sLeftColumn->module->flag & MTD_GROUP_MASK)
                       == MTD_GROUP_TEXT ) &&
                     ( (sRightColumn->module->flag & MTD_GROUP_MASK)
                       == MTD_GROUP_TEXT ) )
                {
                    if ( sLeftColumn->module->id != sRightColumn->module->id )
                    {
                        sIsSameType = ID_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // BUG-36457
                    // 문자와 숫자 type으로 transitive predicate을 생성해서는 안된다.
                    if ( ( ( (sLeftColumn->module->flag & MTD_GROUP_MASK)
                             == MTD_GROUP_NUMBER ) &&
                           ( (sRightColumn->module->flag & MTD_GROUP_MASK)
                             == MTD_GROUP_TEXT ) )
                         ||
                         ( ( (sLeftColumn->module->flag & MTD_GROUP_MASK)
                             == MTD_GROUP_TEXT ) &&
                           ( (sRightColumn->module->flag & MTD_GROUP_MASK)
                             == MTD_GROUP_NUMBER ) ) )
                    {
                        sIsSameType = ID_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                // right가 column list인 경우
                // ex) i1 in (i2,i3,1)

                sRightNode = (qtcNode*) aRightNode->node.arguments;

                while ( sRightNode != NULL )
                {
                    IDE_TEST( isSameCharType( aStatement,
                                              aLeftNode,
                                              sRightNode,
                                              & sIsSameType )
                              != IDE_SUCCESS );

                    if ( sIsSameType == ID_FALSE )
                    {
                        break;
                    }
                    else
                    {
                        sRightNode = (qtcNode*) sRightNode->node.next;
                    }
                }                
            }
        }
        else
        {
            // left가 column list인 경우

            if ( QTC_IS_LIST( aRightNode ) != ID_TRUE )
            {
                // right가 column list가 아닌 경우

                // estimate한 후이므로 이런 경우는 없다.
                IDE_DASSERT( 0 );
            }
            else
            {
                // right가 column list인 경우

                sLeftNode = (qtcNode*) aLeftNode->node.arguments;
                sRightNode = (qtcNode*) aRightNode->node.arguments;

                if (QTC_IS_LIST(sRightNode) != ID_TRUE)
                {
                    // ex) (i1,i2) = (i3,i4)

                    while ( sLeftNode != NULL )
                    {
                        IDE_TEST( isSameCharType( aStatement,
                                                  sLeftNode,
                                                  sRightNode,
                                                  & sIsSameType )
                                  != IDE_SUCCESS );

                        if ( sIsSameType == ID_FALSE )
                        {
                            break;
                        }
                        else
                        {
                            sLeftNode = (qtcNode*) sLeftNode->node.next;
                            sRightNode = (qtcNode*) sRightNode->node.next;
                        }
                    }
                }
                else
                {
                    // ex) (i1,i2) in ((i3,i4), (i5,i6))

                    while (sRightNode != NULL)
                    {
                        IDE_TEST( isSameCharType(aStatement,
                                                 aLeftNode,
                                                 sRightNode,
                                                 &sIsSameType )
                                  != IDE_SUCCESS);

                        if (sIsSameType == ID_FALSE)
                        {
                            break;
                        }
                        else
                        {
                            sRightNode = (qtcNode*)sRightNode->node.next;
                        }
                    }
                }
            }
        }
    }
    else
    {
        // column과 value의 비교에서는 문자형 type이 달라도 좋다.

        // Nothing to do.
    }

    *aIsSameType = sIsSameType;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::isTransitiveForm( qcStatement * aStatement,
                               qmsQuerySet * aQuerySet,
                               qtcNode     * aCompareNode,
                               idBool        sCheckNext,
                               idBool      * aIsTransitiveForm )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1404 Transitive Predicate Generation
 *     transitive predicate을 생성할 base predicate을 찾는다.
 *
 *     12개의 연산자를 지원하며
 *         =, !=, <, <=, >, >=, (not) between, (not) like, (not) in
 *
 *     다음과 같은 format을 지원한다.
 *         1. column operator value
 *         2. column operator column
 *         3. column between value
 *         4. column like value
 *         5. column in value
 *         6. 1~5는 OR 없이 AND로만 연결되어야 한다.
 *
 * Implementation :
 *     (1) OR 없이 AND로만 연결되어야 한다.
 *     (2) constant 혹은 outer column expression은 지원하지 않는다.
 *     (3) PSM function을 포함하지 않아야 한다.
 *     (4) variable build-in function을 포함하지 않아야 한다.
 *     (5) subquery를 포함하지 않아야 한다.
 *     (6) rownum을 포함하지 않아야 한다.
 *     (7) 지원하는 12가지 연산자 중에 하나여야 한다.
 *     (8) between, like, in의 오른쪽에는 상수만이 와야 한다.
 *     (9) 의미없는 predicate은 추가하지 않는다.
 *         (예, i1 = i1)
 *     (10) 문자 type의 컬럼간의 비교는 동일 type만 가능하다.
 *         (예, 'char'i1 = 'varchar'i2 안됨)
 *
 ***********************************************************************/

    qtcNode   * sArgNode0;
    qtcNode   * sArgNode1;
    qtcNode   * sArgNode2;
    qcDepInfo   sDepInfo;
    idBool      sIsTransitiveForm = ID_TRUE;
    idBool      sIsEquivalent = ID_FALSE;
    idBool      sIsSameType = ID_FALSE;
    idBool      sFound = ID_FALSE;
    UInt        sModuleId;
    UInt        i;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::isTransitiveForm::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aIsTransitiveForm != NULL );

    //------------------------------------------
    // 적용가능한 predicate 찾기
    //------------------------------------------

    //------------------------------------------
    // 1. CNF인 경우 OR 하위로 하나의 연산자만 있어야 한다.
    //------------------------------------------
    
    if ( (sCheckNext == ID_TRUE) &&
         (aCompareNode->node.next != NULL) )
    {
        sIsTransitiveForm = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // 2. 상수(value operator value)이거나 외부 컬럼 expression은
    //    지원하지 않는다.
    //------------------------------------------

    if ( sIsTransitiveForm == ID_TRUE )
    {
        qtc::dependencyAnd( & aCompareNode->depInfo,
                            & aQuerySet->depInfo,
                            & sDepInfo );
        
        if( qtc::dependencyEqual( & sDepInfo,
                                  & qtc::zeroDependencies ) == ID_TRUE )
        {
            sIsTransitiveForm = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // 3. PSM function을 포함하지 않아야 한다.
    //------------------------------------------
    
    if ( (sIsTransitiveForm == ID_TRUE) &&
         ((aCompareNode->lflag & QTC_NODE_PROC_FUNCTION_MASK)
          == QTC_NODE_PROC_FUNCTION_TRUE) )
    {
        sIsTransitiveForm = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    //------------------------------------------
    // 4. variable build-in function을 포함하지 않아야 한다.
    //------------------------------------------
    
    if ( (sIsTransitiveForm == ID_TRUE) &&
         ((aCompareNode->lflag & QTC_NODE_VAR_FUNCTION_MASK)
          == QTC_NODE_VAR_FUNCTION_EXIST) )
    {
        sIsTransitiveForm = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    //------------------------------------------
    // 5. subquery를 포함하지 않아야 한다.
    //------------------------------------------
    
    if ( (sIsTransitiveForm == ID_TRUE) &&
         ((aCompareNode->lflag & QTC_NODE_SUBQUERY_MASK)
          == QTC_NODE_SUBQUERY_EXIST) )
    {
        sIsTransitiveForm = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    //------------------------------------------
    // 6. rownum을 포함하지 않아야 한다.
    //------------------------------------------
    
    if ( (sIsTransitiveForm == ID_TRUE) &&
         ((aCompareNode->lflag & QTC_NODE_ROWNUM_MASK)
          == QTC_NODE_ROWNUM_EXIST) )
    {
        sIsTransitiveForm = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    //------------------------------------------
    // 7. 지원하는 12가지 연산자 중에 하나여야 한다.
    //------------------------------------------
    
    if ( sIsTransitiveForm == ID_TRUE )
    {
        for (i = 0; operatorModule[i].module != NULL; i++)
        {
            if ( aCompareNode->node.module == operatorModule[i].module )
            {
                if ( ( ( aCompareNode->lflag & QTC_NODE_JOIN_TYPE_MASK ) == QTC_NODE_JOIN_TYPE_ANTI ) ||
                     ( ( ( aCompareNode->lflag & QTC_NODE_JOIN_TYPE_MASK ) == QTC_NODE_JOIN_TYPE_SEMI ) &&
                       ( QCU_OPTIMIZER_SEMI_JOIN_TRANSITIVITY_DISABLE == 1 ) ) )
                {
                    // 다음의 경우에 transitive predicate을 생성하면 안된다.
                    // 1) PROJ-1718 Anti join인 경우
                    // 2) BUG-43421 Semi join이고 Property __OPTIMIZER_SEMI_JOIN_TRANSITIVITY_DISABLE이 1인 경우
                }
                else
                {
                    sModuleId = i;
                    sFound = ID_TRUE;
                    break;
                }
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sFound == ID_TRUE )
        {
            IDE_DASSERT(
                ( (aCompareNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) >= 2 ) &&
                ( (aCompareNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) <= 3 ) );
            
            // 비교연산자의 왼쪽 인자
            sArgNode0 = (qtcNode*) aCompareNode->node.arguments;
    
            // 오른쪽 첫번째 인자와 두번째 인자
            sArgNode1 = (qtcNode*) sArgNode0->node.next;
            sArgNode2 = (qtcNode*) sArgNode1->node.next;
            
            //------------------------------------------
            // 8. between, like, in의 오른쪽에는 상수만이 와야 한다.
            //------------------------------------------
            
            if ( operatorModule[sModuleId].transposeModuleId == ID_UINT_MAX )
            {
                if ( qtc::dependencyEqual( & sArgNode1->depInfo,
                                           & qtc::zeroDependencies ) == ID_TRUE )
                {
                    // Nothing to do.
                }
                else
                {
                    sIsTransitiveForm = ID_FALSE;
                    IDE_CONT(NORMAL_EXIT);
                }
                
                if ( sArgNode2 != NULL )
                {
                    if ( qtc::dependencyEqual( & sArgNode2->depInfo,
                                               & qtc::zeroDependencies ) == ID_TRUE )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsTransitiveForm = ID_FALSE;
                        IDE_CONT(NORMAL_EXIT);
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }

            if ( sArgNode2 == NULL )
            {
                //------------------------------------------
                // 9. 의미없는 predicate은 추가하지 않는다.
                //------------------------------------------
                
                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       sArgNode0,
                                                       sArgNode1,
                                                       & sIsEquivalent )
                          != IDE_SUCCESS );
                
                if ( sIsEquivalent == ID_TRUE )
                {
                    sIsTransitiveForm = ID_FALSE;
                    IDE_CONT(NORMAL_EXIT);
                }
                else
                {
                    // Nothing to do.
                }
                
                //------------------------------------------
                // 10. 문자 type의 컬럼간의 비교는 동일 type만 가능하다.
                //------------------------------------------

                IDE_TEST( isSameCharType( aStatement,
                                          sArgNode0,
                                          sArgNode1,
                                          & sIsSameType )
                          != IDE_SUCCESS );

                if ( sIsSameType == ID_FALSE )
                {
                    sIsTransitiveForm = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sIsTransitiveForm = ID_FALSE;
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT(NORMAL_EXIT);

    *aIsTransitiveForm = sIsTransitiveForm;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
    
IDE_RC
qmoTransMgr::createOperandSet( qcStatement       * aStatement,
                               qmoTransPredicate * aTransPredicate,
                               idBool            * aIsApplicable )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1404 Transitive Predicate Generation
 *     operand list로 operand set을 생성한다.
 *     (operand set을 생성한후 operand list는 더이상 사용되지 않는다.)
 *
 * Implementation :
 *     (1) operand list를 돌며 중복을 검사하고 중복된 경우
 *         (1.1) 현재 노드를 참조하고 있는 operator list를 변경한다.
 *         (1.2) 현재 노드는 operand set을 구성하지 않는다.
 *     (2) 중복되지 않는 경우
 *         (2.1) 현재 노드로 operand set을 구성하고 id를 할당한다.
 *     (3) 생성된 operand set을 id로 직접 참조할 수 있도록
 *         operand array를 구성한다.
 *
 ***********************************************************************/

    qmoTransOperand   * sFirst;
    qmoTransOperand   * sSecond;
    qmoTransOperand   * sOperand;
    qmoTransOperand  ** sColSetArray;
    idBool              sIsEquivalent;
    idBool              sIsDuplicate;
    UInt                sDupCount = 0;
    UInt                sOperandId = 0;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::createOperandSet::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTransPredicate != NULL );
    IDE_DASSERT( aIsApplicable != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sFirst = aTransPredicate->operandList;

    //------------------------------------------
    // Column Set 생성
    //------------------------------------------

    while ( sFirst != NULL )
    {
        sSecond = sFirst->next;
        sIsDuplicate = ID_FALSE;

        while ( sSecond != NULL )
        {
            if ( ( sFirst->operandSecond == NULL ) &&
                 ( sSecond->operandSecond == NULL ) )
            {
                // 두번째 인자가 없는 column이나 value를 비교
                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       sFirst->operandFirst,
                                                       sSecond->operandFirst,
                                                       & sIsEquivalent )
                          != IDE_SUCCESS );

                if ( sIsEquivalent == ID_TRUE )
                {
                    // 현재노드를 참조하는 operator의 인자를 변경
                    IDE_TEST( changeOperand( aTransPredicate,
                                             sFirst,
                                             sSecond )
                              != IDE_SUCCESS );

                    sIsDuplicate = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                if ( ( sFirst->operandSecond != NULL ) &&
                     ( sSecond->operandSecond != NULL ) )
                {
                    // 두번째 인자가 있는 value를 비교
                    // (between x and y, like w escape z)
                    IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                           sFirst->operandFirst,
                                                           sSecond->operandFirst,
                                                           & sIsEquivalent )
                              != IDE_SUCCESS );

                    if ( sIsEquivalent == ID_TRUE )
                    {
                        IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                               sFirst->operandSecond,
                                                               sSecond->operandSecond,
                                                               & sIsEquivalent )
                                  != IDE_SUCCESS );

                        if ( sIsEquivalent == ID_TRUE )
                        {
                            // 현재노드를 참조하는 operator의 인자를 변경
                            IDE_TEST( changeOperand( aTransPredicate,
                                                     sFirst,
                                                     sSecond )
                                      != IDE_SUCCESS );

                            sIsDuplicate = ID_TRUE;
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }

            sSecond = sSecond->next;
        }

        if ( sIsDuplicate == ID_TRUE )
        {
            // operand가 중복된 경우
            sDupCount++;
        }
        else
        {
            // operand가 중복되지 않은 경우 operand set을 구성
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoTransOperand),
                                                     (void **) & sOperand )
                      != IDE_SUCCESS );

            idlOS::memcpy( sOperand, sFirst, ID_SIZEOF(qmoTransOperand) );

            // 생성한 operand set을 참조하도록 operator의 인자를 변경
            IDE_TEST( changeOperand( aTransPredicate,
                                     sFirst,
                                     sOperand )
                      != IDE_SUCCESS );

            // operand id 할당
            sOperand->id = sOperandId;
            sOperandId++;

            // operand set 구성
            sOperand->next = aTransPredicate->operandSet;
            aTransPredicate->operandSet = sOperand;
            aTransPredicate->operandSetSize = sOperandId;
        }

        sFirst = sFirst->next;
    }

    // operand set을 id로 직접 참조할 수 있도록 operand array 구성
    if ( sOperandId > 0 )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoTransOperand* ) * sOperandId,
                                                   (void **) & sColSetArray )
                  != IDE_SUCCESS );

        sOperand = aTransPredicate->operandSet;

        while ( sOperand != NULL )
        {
            IDE_DASSERT( sOperand->id < sOperandId );

            sColSetArray[sOperand->id] = sOperand;

            sOperand = sOperand->next;
        }

        aTransPredicate->operandSetArray = sColSetArray;
    }
    else
    {
        // Nothing to do.
    }

    if ( sDupCount > 0 )
    {
        *aIsApplicable = ID_TRUE;
    }
    else
    {
        // 중복된 operand가 없다면 transitive predicate이 생성되지 않는다.
        *aIsApplicable = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::changeOperand( qmoTransPredicate * aTransPredicate,
                            qmoTransOperand   * aFrom,
                            qmoTransOperand   * aTo )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     aFrom을 참조하는 operator list를 aTo로 바꾼다.
 *
 ***********************************************************************/

    qmoTransOperator  * sOperator;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::changeOperand::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aTransPredicate != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aTo != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sOperator = aTransPredicate->operatorList;

    //------------------------------------------
    // operand 변경
    //------------------------------------------

    while ( sOperator != NULL )
    {
        if ( sOperator->left == aFrom )
        {
            sOperator->left = aTo;
        }
        else
        {
            // Nothing to do.
        }

        if ( sOperator->right == aFrom )
        {
            sOperator->right = aTo;
        }
        else
        {
            // Nothing to do.
        }

        sOperator = sOperator->next;
    }
    
    return IDE_SUCCESS;
}

IDE_RC
qmoTransMgr::initializeTransitiveMatrix( qmoTransMatrix * aTransMatrix )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     transitive predicate matrix를 생성하고 초기화한다.
 *
 ***********************************************************************/

    qmoTransOperator    *sOperator;
    qmoTransMatrixInfo **sTransMatrix;
    UInt                 sSize;
    UInt                 i;
    UInt                 j;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::initializeTransitiveMatrix::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aTransMatrix != NULL );
    IDE_DASSERT( aTransMatrix->predicate != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sTransMatrix = aTransMatrix->matrix;
    sSize = aTransMatrix->size;
    
    sOperator = aTransMatrix->predicate->operatorList;

    //------------------------------------------
    // matrix 초기화
    //------------------------------------------
    
    for (i = 0; i < sSize; i++)
    {
        for (j = 0; j < sSize; j++)
        {
            sTransMatrix[i][j].mOperator = ID_UINT_MAX;
            sTransMatrix[i][j].mIsNewAdd = ID_FALSE;
        }
    }

    //------------------------------------------
    // predicate 초기화
    //------------------------------------------

    while ( sOperator != NULL )
    {
        i = sOperator->left->id;
        j = sOperator->right->id;

        IDE_DASSERT( i != j );
        
        sTransMatrix[i][j].mOperator = sOperator->id;

        // 좌우변을 바꾸었을때의 연산자 추가
        if ( operatorModule[sOperator->id].transposeModuleId != ID_UINT_MAX )
        {
            sTransMatrix[j][i].mOperator = operatorModule[sOperator->id].transposeModuleId;
        }
        else
        {
            // Nothing to do.
        }

        sOperator = sOperator->next;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoTransMgr::calculateTransitiveMatrix( qcStatement    * aStatement,
                                        qmsQuerySet    * aQuerySet,
                                        qmoTransMatrix * aTransMatrix )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     transitive predicate matrix를 계산한다.
 *
 ***********************************************************************/

    qmoTransPredicate   *sTransPredicate;
    qmoTransMatrixInfo **sTransMatrix;
    idBool               sIsBad;
    UInt                 sNewOperator;
    UInt                 sSize;
    UInt                 i;
    UInt                 j;
    UInt                 k;
    
    IDU_FIT_POINT_FATAL( "qmoTransMgr::calculateTransitiveMatrix::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aTransMatrix != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sTransPredicate = aTransMatrix->predicate;
    sTransMatrix    = aTransMatrix->matrix;
    sSize           = aTransMatrix->size;

    //------------------------------------------
    // transitive predicate matrix 계산
    //------------------------------------------

    /*
     * BUG-29646: transitive predicate 을 완벽하게 구하지 못합니다
     * 
     * i -> j, j -> k 관계가 있을 때, i -> k 관계를 생성한다
     * 이때, 중간 매개가 되는 j 를 loop 최상위에 두도록 한다 (Floyd-Warshall)
     *
     * transitive closure를 구하기 위해 중간에 bad 가 나오더라도 일단 진행하고
     * 나중에 bad 가 아닌것들에 대해서만 trasitive predicate을 생성한다
     */

    for (j = 0; j < sSize; j++)
    {
        for (i = 0; i < sSize; i++)
        {
            if (sTransMatrix[i][j].mOperator != ID_UINT_MAX)
            {
                for (k = 0; k < sSize; k++)
                {
                    if (sTransMatrix[j][k].mOperator != ID_UINT_MAX)
                    {
                        sNewOperator = operationMatrix[sTransMatrix[i][j].mOperator][sTransMatrix[j][k].mOperator];

                        /*
                         * matrix 가 일단 어떤 값으로 채워지면
                         * 다시 채우지 않는다
                         */
                        if ((sNewOperator != ID_UINT_MAX) &&
                            (sTransMatrix[i][k].mOperator == ID_UINT_MAX))
                        {
                            sTransMatrix[i][k].mOperator = sNewOperator;

                            if (i != k)
                            {
                                IDE_TEST(isBadTransitivePredicate(aStatement,
                                                                  aQuerySet,
                                                                  sTransPredicate,
                                                                  i,
                                                                  j,
                                                                  k,
                                                                  &sIsBad)
                                         != IDE_SUCCESS);

                                if (sIsBad == ID_FALSE)
                                {
                                    sTransMatrix[i][k].mIsNewAdd = ID_TRUE;
                                }
                                else
                                {
                                    /* Nothing to do. */
                                }
                            }
                            else
                            {
                                /* i == k */
                            }
                        }
                        else
                        {
                            /* Nothing to do. */
                        }
                    }
                }
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }

    for (i = 0; i < sSize; i++)
    {
        for (j = 0; j < sSize; j++)
        {
            if (sTransMatrix[i][j].mIsNewAdd == ID_TRUE)
            {
                IDE_TEST(createNewOperator(aStatement,
                                           sTransPredicate,
                                           i,
                                           j,
                                           sTransMatrix[i][j].mOperator)
                         != IDE_SUCCESS);
            }
            else
            {
                sTransMatrix[i][j].mOperator = ID_UINT_MAX;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::isBadTransitivePredicate( qcStatement       * aStatement,
                                       qmsQuerySet       * aQuerySet,
                                       qmoTransPredicate * aTransPredicate,
                                       UInt                aOperandId1,
                                       UInt                aOperandId2,
                                       UInt                aOperandId3,
                                       idBool            * aIsBad )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1404 Transitive Predicate Generation
 *     생성할 transitive predicate이 bad transitive predicate인지
 *     판단한다. (bad가 아니라고 해서 항상 good은 아니다.)
 *
 * Implementation :
 *
 *     DOC-31095 Transitive Predicate Generation.odt p54
 *     <Transitive Predicate 생성 rule> (번호순서를 바꿨음)
 *
 *        |  D1   D2   D3  |  D1   D3  |
 *    ----+----------------+-----------+---------------
 *      1 |  0    a    0   |   0    0  | (X)
 *    ----+----------------+-----------+---------------
 *      2 |  0    a    a   |   0    a  | (depCount=1, index)
 *      3 |  0    a    b   |   0    b  | (depCount=1, no index)
 *      4 |  a    a    0   |   a    0  | (depCount=1, index)
 *      5 |  a    b    0   |   a    0  | (depCount=1, no index)
 *    ----+----------------+-----------+---------------
 *      6 |  a    0    a   |   a    a  | (X)
 *      7 |  a    a    a   |   a    a  | (X)
 *      8 |  a    b    a   |   a    a  | (depCount=1)
 *    ----+----------------+-----------+---------------
 *      9 |  a    0    b   |   a    b  | (X)
 *     10 |  a    a    b   |   a    b  | (X)
 *     11 |  a    b    b   |   a    b  | (X)
 *     12 |  a    c    b   |   a    b  | (*)
 *
 *   (*) non-joinable join pred로 joinable join pred를 생성할 경우
 *
 ***********************************************************************/

    qmoTransOperand  * sOperand1;
    qmoTransOperand  * sOperand2;
    qmoTransOperand  * sOperand3;
    qcDepInfo        * sDep1;
    qcDepInfo        * sDep2;
    qcDepInfo        * sDep3;
    qcDepInfo          sAndDepInfo;
    qcDepInfo          sDepInfo;
    qcDepInfo          sDepTmp1;
    qcDepInfo          sDepTmp2;
    qcDepInfo          sDepTmp3;
    idBool             sIsBad = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::isBadTransitivePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aTransPredicate != NULL );
    IDE_DASSERT( aIsBad != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sOperand1 = aTransPredicate->operandSetArray[aOperandId1];
    sOperand2 = aTransPredicate->operandSetArray[aOperandId2];
    sOperand3 = aTransPredicate->operandSetArray[aOperandId3];

    sDep1 = & sOperand1->depInfo;
    sDep2 = & sOperand2->depInfo;
    sDep3 = & sOperand3->depInfo;    

    IDE_TEST( qtc::dependencyOr( sDep1,
                                 sDep3,
                                 & sDepInfo )
              != IDE_SUCCESS );

    /* BUG-42134 Created transitivity predicate of join predicate must be
     * reinforced.
     * OLD Rule이 선택되었다면 joinDepInfo를 clear해 이전처럼 동작하도록 한다.
     */
    if ( QCG_GET_SESSION_OPTIMIZER_TRANSITIVITY_OLD_RULE( aStatement ) == 1 )
    {
        qtc::dependencyClear( &aTransPredicate->joinDepInfo );
    }
    else
    {
        /* Nothing to do */
    }
    //------------------------------------------
    // outer column expression은 생성하지 않는다.
    //------------------------------------------
    
    qtc::dependencyAnd( & sDepInfo,
                        & aQuerySet->depInfo,
                        & sAndDepInfo );
    
    if( qtc::dependencyEqual( & sAndDepInfo,
                              & qtc::zeroDependencies ) == ID_TRUE )
    {
        sIsBad = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }
    
    //------------------------------------------
    // bad transitive predicate 검사
    //------------------------------------------

    if ( qtc::getCountBitSet( & sDepInfo ) == 0 )
    {
        //--------------------
        // case 1
        //--------------------
        
        sIsBad = ID_TRUE;
    }
    else
    {
        if ( (qtc::getCountBitSet( sDep1 ) == 0) ||
             (qtc::getCountBitSet( sDep3 ) == 0) )
        {
            //--------------------
            // case 2,3,4,5
            //--------------------
            
            if ( qtc::getCountBitSet( & sDepInfo ) != 1 )
            {
                sIsBad = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            // case 2,3,4,5, depCount=1
            if ( qtc::getCountBitSet( sDep1 ) == 0 )
            {
                if ( qtc::dependencyEqual( sDep2, sDep3 ) == ID_TRUE )
                {
                    // case 2
                    if ( sOperand3->isIndexColumn == ID_FALSE )
                    {
                        sIsBad = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.

                        // case 2, depCount=1, index
                    }
                }
                else
                {
                    // case 3, depCount=1
                    if ( sOperand3->isIndexColumn == ID_TRUE )
                    {
                        /* BUG-42134
                         * Index가 있을 경우 Join과 관련되서 dependency가
                         * 있다면 bad 처리를 하지 않는다
                         */
                        if ( aTransPredicate->joinDepInfo.depCount > 0 )
                        {
                            if ( ( qtc::dependencyContains( &aTransPredicate->joinDepInfo,
                                                            sDep2 ) == ID_TRUE ) ||
                                 ( qtc::dependencyContains( &aTransPredicate->joinDepInfo,
                                                            sDep3 ) == ID_TRUE ) )

                            {
                                sIsBad = ID_FALSE;
                            }
                            else
                            {
                                sIsBad = ID_TRUE;
                            }
                        }
                        else
                        {
                            sIsBad = ID_TRUE;
                        }
                    }
                    else
                    {
                        // Nothing to do.

                        // case 3, depCount=1, no index
                    }
                }
            }
            else
            {
                if ( qtc::dependencyEqual( sDep1, sDep2 ) == ID_TRUE )
                {
                    // case 4
                    if ( sOperand1->isIndexColumn == ID_FALSE )
                    {
                        sIsBad = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.

                        // case 4, depCount=1, index
                    }
                }
                else
                {
                    // case 5, depCount=1
                    if ( sOperand1->isIndexColumn == ID_TRUE )
                    {
                        /* BUG-42134
                         * Index가 있을 경우 Join과 관련되서 dependency가
                         * 있다면 bad 처리를 하지 않는다
                         */
                        if ( aTransPredicate->joinDepInfo.depCount > 0 )
                        {
                            if ( ( qtc::dependencyContains( &aTransPredicate->joinDepInfo,
                                                            sDep1 ) == ID_TRUE ) ||
                                 ( qtc::dependencyContains( &aTransPredicate->joinDepInfo,
                                                            sDep3 ) == ID_TRUE ) )
                            {
                                sIsBad = ID_FALSE;
                            }
                            else
                            {
                                sIsBad = ID_TRUE;
                            }
                        }
                        else
                        {
                            sIsBad = ID_TRUE;
                        }
                    }
                    else
                    {
                        // Nothing to do.

                        // case 5, depCount=1, no index
                    }
                }
            }
        }
        else
        {
            if ( qtc::dependencyEqual( sDep1, sDep3 ) == ID_TRUE )
            {
                //--------------------
                // case 6,7,8
                //--------------------

                if ( (qtc::getCountBitSet( sDep2 ) == 0) ||
                     (qtc::dependencyEqual( sDep1, sDep2 ) == ID_TRUE) )
                {
                    // case 6,7
                    sIsBad = ID_TRUE;
                }
                else
                {
                    // case 8
                    if ( qtc::getCountBitSet( & sDepInfo ) != 1 )
                    {
                        sIsBad = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.

                        // case 8, depCount=1
                    }
                }
            }
            else
            {
                //--------------------
                // case 9,10,11,12
                //--------------------
                
                if ( (qtc::getCountBitSet( sDep2 ) == 0) ||
                     (qtc::dependencyEqual( sDep1, sDep2 ) == ID_TRUE) ||
                     (qtc::dependencyEqual( sDep2, sDep3 ) == ID_TRUE) )
                {
                    // case 9,10,11
                    sIsBad = ID_TRUE;
                }
                else
                {
                    // case 12
                    qtc::dependencyAnd( sDep1, sDep2, & sDepTmp1 );
                    qtc::dependencyAnd( sDep2, sDep3, & sDepTmp2 );
                    qtc::dependencyAnd( sDep1, sDep3, & sDepTmp3 );
                    
                    if ( (qtc::getCountBitSet( & sDepTmp1 ) > 0) &&
                         (qtc::getCountBitSet( & sDepTmp2 ) > 0) &&
                         (qtc::getCountBitSet( & sDepTmp3 ) == 0) )
                    {
                        // case 12
                        // {a,c}, {c,b} non-joinable join predicate
                        // {a,b} joinable join predicate
                        // non-joinable join pred로 joinable join pred를 생성한다.
                    }
                    else
                    {
                        sIsBad = ID_TRUE;
                    }
                }
            }
        }
    }

    *aIsBad = sIsBad;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::createNewOperator( qcStatement       * aStatement,
                                qmoTransPredicate * aTransPredicate,
                                UInt                aLeftId,
                                UInt                aRightId,
                                UInt                aNewOperatorId )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     qmoTransOperator를 생성하고 aTransPredicate에 연결한다.
 *
 ***********************************************************************/

    qmoTransOperator * sOperator;
    qmoTransOperand  * sLeft;
    qmoTransOperand  * sRight;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::createNewOperator::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTransPredicate != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sLeft = aTransPredicate->operandSetArray[aLeftId];
    sRight = aTransPredicate->operandSetArray[aRightId];
    
    //------------------------------------------
    // qmoTransOperator 생성
    //------------------------------------------
    
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoTransOperator),
                                             (void **) & sOperator )
              != IDE_SUCCESS );

    sOperator->left = sLeft;
    sOperator->right = sRight;
    sOperator->id = aNewOperatorId;

    // genOperatorList에 연결한다.
    sOperator->next = aTransPredicate->genOperatorList;
    aTransPredicate->genOperatorList = sOperator;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::generateTransitivePredicate( qcStatement     * aStatement,
                                          qmsQuerySet     * aQuerySet,
                                          qmoTransMatrix  * aTransMatrix,
                                          qtcNode        ** aTransitiveNode )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     transitive predicate를 생성한다.
 *
 ***********************************************************************/

    qmoTransPredicate  * sTransPredicate;
    qmoTransOperator   * sOperator;
    qtcNode            * sTransitivePredicate;
    qtcNode            * sTransitiveNode = NULL;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::generateTransitivePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTransMatrix != NULL );
    IDE_DASSERT( aTransitiveNode != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sTransPredicate = aTransMatrix->predicate;
    sOperator = sTransPredicate->genOperatorList;

    //------------------------------------------
    // transitive predicate 생성
    //------------------------------------------

    while ( sOperator != NULL )
    {
        if ( (sOperator->left->id < sOperator->right->id) ||
             (operatorModule[sOperator->id].transposeModuleId == ID_UINT_MAX) )
        {
            // matrix의 L쪽과 transpose하지 않은 연산자에 대해서만 생성한다.
            IDE_TEST( createTransitivePredicate( aStatement,
                                                 aQuerySet,
                                                 sOperator,
                                                 & sTransitivePredicate )
                      != IDE_SUCCESS );
        
            // 연결한다.
            sTransitivePredicate->node.next = (mtcNode*) sTransitiveNode;
            sTransitiveNode = sTransitivePredicate;
        }
        else
        {
            // 논리적으로 중복된 predicate은 생성하지 않는다.
            
            // Nothing to do.
        }

        sOperator = sOperator->next;
    }
    
    *aTransitiveNode = sTransitiveNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::createTransitivePredicate( qcStatement        * aStatement,
                                        qmsQuerySet        * aQuerySet,
                                        qmoTransOperator   * aOperator,
                                        qtcNode           ** aTransitivePredicate )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     하나의 transitive predicate를 생성한다.
 *
 ***********************************************************************/

    qmoTransOperand  * sLeft;
    qmoTransOperand  * sRight;
    qtcNode          * sLeftArgNode1 = NULL;
    qtcNode          * sRightArgNode1 = NULL;
    qtcNode          * sRightArgNode2 = NULL;
    qtcNode          * sCompareNode[2];
    qtcNode          * sOrNode[2];
    qcNamePosition     sNullPosition;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::createTransitivePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aOperator != NULL );
    IDE_DASSERT( aTransitivePredicate != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sLeft = aOperator->left;
    sRight = aOperator->right;
    
    SET_EMPTY_POSITION( sNullPosition );    

    //------------------------------------------
    // left operand 복사 생성
    //------------------------------------------

    IDE_DASSERT( sLeft->operandSecond == NULL );
    
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                     sLeft->operandFirst,
                                     & sLeftArgNode1,
                                     ID_FALSE, // root의 next는 복사하지 않는다.
                                     ID_TRUE,  // conversion을 끊는다.
                                     ID_TRUE,  // constant node까지 복사한다.
                                     ID_TRUE ) // constant node를 원복한다.
              != IDE_SUCCESS );

    //------------------------------------------
    // right operand 복사 생성
    //------------------------------------------
    
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                     sRight->operandFirst,
                                     & sRightArgNode1,
                                     ID_FALSE,
                                     ID_TRUE,
                                     ID_TRUE,
                                     ID_TRUE )
              != IDE_SUCCESS );

    if ( sRight->operandSecond != NULL )
    {
        IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                         sRight->operandSecond,
                                         & sRightArgNode2,
                                         ID_FALSE,
                                         ID_TRUE,
                                         ID_TRUE,
                                         ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    //------------------------------------------
    // transitive predicate 생성
    //------------------------------------------

    // 새로운 operation 노드를 생성한다.
    IDE_TEST( qtc::makeNode( aStatement,
                             sCompareNode,
                             & sNullPosition,
                             operatorModule[aOperator->id].module )
              != IDE_SUCCESS );

    // argument를 연결한다.
    sRightArgNode1->node.next = (mtcNode*) sRightArgNode2;
    sLeftArgNode1->node.next = (mtcNode*) sRightArgNode1;

    if ( sRightArgNode2 == NULL )
    {
        sCompareNode[0]->node.arguments = (mtcNode*) sLeftArgNode1;
        sCompareNode[0]->node.lflag |= 2;
    }
    else
    {
        sCompareNode[0]->node.arguments = (mtcNode*) sLeftArgNode1;
        sCompareNode[0]->node.lflag |= 3;
    }

    // transitive predicate flag를 설정한다.
    sCompareNode[0]->lflag &= ~QTC_NODE_TRANS_PRED_MASK;
    sCompareNode[0]->lflag |= QTC_NODE_TRANS_PRED_EXIST;

    // 새로운 OR 노드를 하나 생성한다.
    IDE_TEST( qtc::makeNode( aStatement,
                             sOrNode,
                             &sNullPosition,
                             (const UChar*)"OR",
                             2 )
              != IDE_SUCCESS );
    
    // sCompareNode를 연결한다.
    sOrNode[0]->node.arguments = (mtcNode*) sCompareNode[0];
    sOrNode[0]->node.lflag |= 1;

    aQuerySet->processPhase = QMS_OPTIMIZE_TRANS_PRED;

    // estimate
    IDE_TEST( qtc::estimate( sOrNode[0],
                             QC_SHARED_TMPLATE(aStatement),
                             aStatement,
                             aQuerySet,
                             aQuerySet->SFWGH,
                             NULL )
              != IDE_SUCCESS);

    *aTransitivePredicate = sOrNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmoTransMgr::getJoinDependency( qmsFrom   * aFrom,
                                       qcDepInfo * aDepInfo )
{
    if ( aFrom->joinType != QMS_NO_JOIN )
    {
        if ( aFrom->joinType == QMS_LEFT_OUTER_JOIN )
        {
            if ( aFrom->left->joinType == QMS_NO_JOIN )
            {
                IDE_TEST( qtc::dependencyOr( aDepInfo,
                                             &aFrom->left->depInfo,
                                             aDepInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( getJoinDependency( aFrom->left, aDepInfo )
                  != IDE_SUCCESS );
        IDE_TEST( getJoinDependency( aFrom->right, aDepInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Noting to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::processTransitivePredicate( qcStatement  * aStatement,
                                         qmsQuerySet  * aQuerySet,
                                         qtcNode      * aNode,
                                         idBool         aIsOnCondition,
                                         qtcNode     ** aTransitiveNode )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate의 생성
 *
 * Implementation :
 *    (1) Predicate Formation
 *        지원하는 형태의 predicate을 뽑아 set을 구성한다.
 *    (2) Transitive Predicate Matrix
 *        Transitive Closure를 Matrix를 이용하여 계산한다.
 *    (3) Transitive Predicate Generation
 *        Transitive Predicate Rule로 Transitive Predicate를 생성한다.
 *
 ***********************************************************************/
    
    qmoTransPredicate * sTransitivePredicate;
    qmoTransMatrix    * sTransitiveMatrix;
    qtcNode           * sNode;
    qtcNode           * sTransitiveNode = NULL;
    idBool              sIsApplicable;
    qmsFrom           * sFrom;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::processTransitivePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aTransitiveNode != NULL );
    
    //------------------------------------------
    // 초기화
    //------------------------------------------

    sNode = aNode;
    
    //------------------------------------------
    // Transitive Predicate Generation
    //------------------------------------------

    if ( sNode != NULL )
    {
        IDE_TEST( init( aStatement,
                        & sTransitivePredicate )
                  != IDE_SUCCESS );

        IDE_TEST( addBasePredicate( aStatement,
                                    aQuerySet,
                                    sTransitivePredicate,
                                    sNode )
                  != IDE_SUCCESS );

        /* BUG-42134 Created transitivity predicate of join predicate must be
         * reinforced.
         */
        if ( aIsOnCondition == ID_FALSE )
        {
            for ( sFrom = aQuerySet->SFWGH->from; sFrom != NULL; sFrom = sFrom->next )
            {
                IDE_TEST( getJoinDependency( sFrom, &sTransitivePredicate->joinDepInfo )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( processPredicate( aStatement,
                                    aQuerySet,
                                    sTransitivePredicate,
                                    & sTransitiveMatrix,
                                    & sIsApplicable )
                  != IDE_SUCCESS );
        
        if ( sIsApplicable == ID_TRUE )
        {
            IDE_TEST( generatePredicate( aStatement,
                                         aQuerySet,
                                         sTransitiveMatrix,
                                         & sTransitiveNode )
                      != IDE_SUCCESS );

        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    *aTransitiveNode = sTransitiveNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::processTransitivePredicate( qcStatement   * aStatement,
                                         qmsQuerySet   * aQuerySet,
                                         qtcNode       * aNode,
                                         qmoPredicate  * aPredicate,
                                         qtcNode      ** aTransitiveNode )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate의 생성
 *
 * Implementation :
 *    (1) Predicate Formation
 *        지원하는 형태의 predicate을 뽑아 set을 구성한다.
 *    (2) Transitive Predicate Matrix
 *        Transitive Closure를 Matrix를 이용하여 계산한다.
 *    (3) Transitive Predicate Generation
 *        Transitive Predicate Rule로 Transitive Predicate를 생성한다.
 *
 ***********************************************************************/
    
    qmoTransPredicate * sTransitivePredicate;
    qmoTransMatrix    * sTransitiveMatrix;
    qmoPredicate      * sPredicate;
    qtcNode           * sTransitiveNode = NULL;
    idBool              sIsApplicable;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::processTransitivePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTransitiveNode != NULL );
    
    //------------------------------------------
    // 초기화
    //------------------------------------------

    sPredicate = aPredicate;
    
    //------------------------------------------
    // Transitive Predicate Generation
    //------------------------------------------

    if ( (sPredicate != NULL) && (aNode != NULL) )
    {
        IDE_TEST( init( aStatement,
                        & sTransitivePredicate )
                  != IDE_SUCCESS );

        IDE_TEST( addBasePredicate( aStatement,
                                    aQuerySet,
                                    sTransitivePredicate,
                                    aNode )
                  != IDE_SUCCESS );

        while ( sPredicate != NULL )
        {
            IDE_TEST( addBasePredicate( aStatement,
                                        aQuerySet,
                                        sTransitivePredicate,
                                        sPredicate )
                      != IDE_SUCCESS );

            sPredicate = sPredicate->next;
        }
        
        IDE_TEST( processPredicate( aStatement,
                                    aQuerySet,
                                    sTransitivePredicate,
                                    & sTransitiveMatrix,
                                    & sIsApplicable )
                  != IDE_SUCCESS );
        
        if ( sIsApplicable == ID_TRUE )
        {
            IDE_TEST( generatePredicate( aStatement,
                                         aQuerySet,
                                         sTransitiveMatrix,
                                         & sTransitiveNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    *aTransitiveNode = sTransitiveNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoTransMgr::isEquivalentPredicate( qcStatement  * aStatement,
                                    qmoPredicate * aPredicate1,
                                    qmoPredicate * aPredicate2,
                                    idBool       * aIsEquivalent )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     논리적으로 동일한 predicate인지 검사한다.
 *
 ***********************************************************************/

    qtcNode   * sCompareNode1;
    qtcNode   * sCompareNode2;
    idBool      sIsEquivalent = ID_FALSE;
    idBool      sFound1 = ID_FALSE;
    idBool      sFound2 = ID_FALSE;
    UInt        sModuleId1  = 0;
    UInt        sModuleId2  = 0;
    UInt        i;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::isEquivalentPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate1 != NULL );
    IDE_DASSERT( aPredicate2 != NULL );
    IDE_DASSERT( aIsEquivalent != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sCompareNode1 = aPredicate1->node;

    if ( ( sCompareNode1->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sCompareNode1 = (qtcNode*) sCompareNode1->node.arguments;
    }
    else
    {
        // Nothing to do.
    }

    sCompareNode2 = aPredicate2->node;

    if ( ( sCompareNode2->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sCompareNode2 = (qtcNode*) sCompareNode2->node.arguments;
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // 논리적으로 동일한 predicate인지 검사
    //------------------------------------------

    // OR노드 하위로 하나의 비교연산자만이 와야하며
    // 논리적으로 동일하려면 dependency도 같을 것이다.
    if ( ( sCompareNode1->node.next == NULL ) &&
         ( sCompareNode2->node.next == NULL ) &&
         ( qtc::dependencyEqual( & sCompareNode1->depInfo,
                                 & sCompareNode2->depInfo ) == ID_TRUE ) )
    {
        for ( i = 0; operatorModule[i].module != NULL; i++)
        {
            if ( sCompareNode1->node.module == operatorModule[i].module )
            {
                sModuleId1 = i;
                sFound1 = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            if ( sCompareNode2->node.module == operatorModule[i].module )
            {
                sModuleId2 = i;
                sFound2 = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( ( sFound1 == ID_TRUE ) &&
             ( sFound2 == ID_TRUE ) )
        {
            if ( sModuleId1 == sModuleId2 )
            {
                // {i1<1}과 {i1<1}의 비교
                // {i1=1}과 {i1=1}의 비교

                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       (qtcNode*) sCompareNode1->node.arguments,
                                                       (qtcNode*) sCompareNode2->node.arguments,
                                                       & sIsEquivalent )
                          != IDE_SUCCESS );

                if ( sIsEquivalent == ID_TRUE )
                {
                    IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                           (qtcNode*) sCompareNode1->node.arguments->next,
                                                           (qtcNode*) sCompareNode2->node.arguments->next,
                                                           & sIsEquivalent )
                              != IDE_SUCCESS );

                    // 인자가 3개인 경우는 좌우변을 바꿀 수 없다.
                    if ( ( sCompareNode1->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 3 )
                    {
                        if ( ( sCompareNode2->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 3 )
                        {
                            IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                                   (qtcNode*) sCompareNode1->node.arguments->next->next,
                                                                   (qtcNode*) sCompareNode2->node.arguments->next->next,
                                                                   & sIsEquivalent )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // 인자의 개수가 다르면 서로 다르다.

                            sIsEquivalent = ID_FALSE;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }

            if ( ( sIsEquivalent == ID_FALSE ) &&
                 ( operatorModule[sModuleId1].transposeModuleId == sModuleId2 ) )
            {
                // {i1<1}과 {1>i1}의 비교
                // {i1=1}과 {1=i1}의 비교

                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       (qtcNode*) sCompareNode1->node.arguments,
                                                       (qtcNode*) sCompareNode2->node.arguments->next,
                                                       & sIsEquivalent )
                          != IDE_SUCCESS );

                if ( sIsEquivalent == ID_TRUE )
                {
                    IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                           (qtcNode*) sCompareNode1->node.arguments->next,
                                                           (qtcNode*) sCompareNode2->node.arguments,
                                                           & sIsEquivalent )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // 단일 OR 노드가 아니라면 같지않다.
        
        // Nothing to do.
    }

    *aIsEquivalent = sIsEquivalent;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
    
IDE_RC
qmoTransMgr::isExistEquivalentPredicate( qcStatement  * aStatement,
                                         qmoPredicate * aPredicate,
                                         qmoPredicate * aPredicateList,
                                         idBool       * aIsExist )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     논리적으로 동일한 predicate이 존재하는지 검사한다.
 *
 ***********************************************************************/

    qmoPredicate  * sPredicate;
    qmoPredicate  * sMorePredicate;
    idBool          sIsEquivalent;
    idBool          sIsExist = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoTransMgr::isExistEquivalentPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aIsExist != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sPredicate = aPredicateList;

    //------------------------------------------
    // 논리적으로 동일한 predicate이 존재하는지 검사
    //------------------------------------------

    while ( sPredicate != NULL )
    {
        sMorePredicate = sPredicate;

        while ( sMorePredicate != NULL )
        {
            if ( aPredicate != sMorePredicate )
            {
                IDE_TEST( isEquivalentPredicate( aStatement,
                                                 aPredicate,
                                                 sMorePredicate,
                                                 & sIsEquivalent )
                          != IDE_SUCCESS );
                
                if ( sIsEquivalent == ID_TRUE )
                {
                    sIsExist = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }

            sMorePredicate = sMorePredicate->more;
        }

        if ( sIsExist == ID_TRUE )
        {
            break;
        }
        else
        {
            sPredicate = sPredicate->next;
        }
    }

    *aIsExist = sIsExist;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
