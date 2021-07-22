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
 * $Id: qtcColumn.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *
 *     Column을 의미하는 Node
 *     Ex) T1.i1
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <qtc.h>
#include <smi.h>
#include <qmvQTC.h>
#include <qsxArray.h>
#include <qsvEnv.h>
#include <qcuSessionPkg.h>

extern mtdModule mtdList;

extern mtxModule mtxColumn; /* PROJ-2632 */

//-----------------------------------------
// Column 연산자의 이름에 대한 정보
//-----------------------------------------

static mtcName qtcNames[1] = {
    { NULL, 6, (void*)"COLUMN" }
};

//-----------------------------------------
// Column 연산자의 Module 에 대한 정보
//-----------------------------------------

static IDE_RC qtcColumnEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule qtc::columnModule = {
    1|                      // 하나의 Column 공간
    MTC_NODE_INDEX_USABLE|  // Index를 사용할 수 있음
    MTC_NODE_OPERATOR_MISC, // 기타 연산자
    ~0,                     // Indexable Mask : 의미 없음
    1.0,                    // default selectivity (비교 연산자 아님)
    qtcNames,               // 이름 정보
    NULL,                   // Counter 연산자 없음
    mtf::initializeDefault, // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,   // 서버 종료시 종료 함수, 없음
    qtcColumnEstimate       // Estimate 할 함수
};

//-----------------------------------------
// Column 연산자의 수행 함수의 정의
//-----------------------------------------

IDE_RC qtcCalculate_Column( mtcNode*  aNode,
                            mtcStack* aStack,
                            SInt      aRemain,
                            void*     aInfo,
                            mtcTemplate* aTemplate );

IDE_RC qtcCalculate_ArrayColumn( mtcNode*  aNode,
                                 mtcStack* aStack,
                                 SInt      aRemain,
                                 void*     aInfo,
                                 mtcTemplate* aTemplate );

//fix PROJ-1596
IDE_RC qtcCalculate_IndirectArrayColumn( mtcNode*  aNode,
                                         mtcStack* aStack,
                                         SInt      aRemain,
                                         void*     aInfo,
                                         mtcTemplate* aTemplate );

IDE_RC qtcCalculate_IndirectColumn( mtcNode*  aNode,
                                    mtcStack* aStack,
                                    SInt      aRemain,
                                    void*     aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute qtcExecute = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qtcCalculate_Column,  // COLUMN 연산 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtxColumn.mCommon,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

static const mtcExecute qtcExecuteArrayColumn = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qtcCalculate_ArrayColumn,  // COLUMN 연산 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtx::calculateNA,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

static const mtcExecute qtcExecuteIndirectArrayColumn = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qtcCalculate_IndirectArrayColumn,  // COLUMN 연산 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtx::calculateNA,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

static const mtcExecute qtcExecuteIndirectColumn = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qtcCalculate_IndirectColumn,  // COLUMN 연산 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtx::calculateNA,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

IDE_RC qtcColumnEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          mtcCallBack* aCallBack)
{
/***********************************************************************
 *
 * Description :
 *    Column 연산자에 대하여 Estimate 수행함.
 *    Column Node에 대한 Column 정보 및 Execute 정보를 Setting한다.
 *
 * Implementation :
 *
 *    Column의 ID를 할당받고, dependencies 및 execute 정보를 Setting한다.
 *
 ***********************************************************************/

    qtcNode           * sNode         = (qtcNode *)aNode;
    qtcCallBackInfo   * sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    qsVariables       * sArrayVariable = NULL;
    qtcColumnInfo     * sColumnInfo;

    //fix PROJ-1596
    mtcNode           * sRealNode  = NULL;
    qcStatement       * sStatement = ((qcTemplate *)aTemplate)->stmt;
    // Indirect Calculate Flag
    idBool              sIdcFlag   = ID_FALSE;
    mtcColumn           sRealColumn;
    mtcColumn           sColumn;
    qmsSFWGH          * sSFWGH;
    mtcNode           * sConvertedNode;
    qtcNode           * sSubQNode;
    // PROJ-1073 Package
    qsxPkgInfo        * sPkgInfo;
    mtcTemplate       * sMtcTmplate;

    const mtdModule   * sModule;
    qsTypes           * sRealType;
    qsTypes           * sType;

    sSFWGH = sCallBackInfo->SFWGH;

    // 최초 estimate 시에만 Column ID를 할당받게 하고,
    // 이후의 estimate 호출시에는 Column ID를 할당받지 않도록 한다.
    // 따라서, estimate() 에서만 CallBackInfo에 statement를 설정한다.
    if (sCallBackInfo->statement != NULL)
    {
        // 실제 Column인 경우 Column ID를 Setting한다.
        // Column이 아닌 경우에는 해당 Node에 적합한 Module로 변경된다.
        // 예를 들어, 다음과 같은 질의를 살펴 보자.
        //     SELECT f1 FROM T1;
        // Parsing 단계에서는 [f1]을 Column으로 판단하지만,
        // 이는 Column일수도 Function일 수도 있다.
        // 만약 Module이 변경된 경우라면, 내부에서 estimate가 수행된다.

        if( ( ( sNode->lflag & QTC_NODE_COLUMN_ESTIMATE_MASK ) ==
              QTC_NODE_COLUMN_ESTIMATE_TRUE )  ||
            ( ( sNode->lflag & QTC_NODE_PROC_VAR_ESTIMATE_MASK ) ==
              QTC_NODE_PROC_VAR_ESTIMATE_TRUE ) )
        {
            // procedure변수의 estimate를 이미 했거나,
            // partition column id를 지정하였으므로 column id를 새로 구하는 것은
            // 하지 않는다.
        }
        else
        {
            IDE_TEST(qmvQTC::setColumnID( sNode,
                                          aTemplate,
                                          aStack,
                                          aRemain,
                                          aCallBack,
                                          &sArrayVariable,
                                          &sIdcFlag,
                                          &sSFWGH )
                     != IDE_SUCCESS);
        }
    }

    // column module may have changed to function module
    if ( sNode->node.module == & qtc::columnModule )
    {
        if( ( sIdcFlag        == ID_FALSE ) ||
            ( aNode->objectID == 0 ) )
        {
            //---------------------------------------------------------------
            // [PR-7108]
            // LEVEL, SEQUENCE, SYSDATE 등은 Column Module이기는 하나
            // Dependency와 직접 관련된 Column이 아니다.
            // 또한, PRIOR Column은 실제 Column이기는 하나, 이 또한
            // Dependency와 관련이 없다.
            // 따라서, Dependencies를 결정하는 Column은 실제 Table Column이거나
            // View의 Column으로 제한한다.  그리고, PRIOR Column이 아니어야 한다.
            // 이렇게 하는 이유는 Plan Node의 대표 Dependency결정과
            // Indexable Predicate의 분류를 용이하게 하기 위함이다.
            // (주의) Store And Search등과 같이 Optimization 과정 중에 생기는
            //        VIEW의 Column은 절대 Dependencies를 가져서는 안된다.
            //---------------------------------------------------------------

            if ( ( (aTemplate->rows[aNode->table].lflag & MTC_TUPLE_TYPE_MASK)
                   == MTC_TUPLE_TYPE_TABLE )
                 ||
                 ( (aTemplate->rows[aNode->table].lflag & MTC_TUPLE_VIEW_MASK )
                   == MTC_TUPLE_VIEW_TRUE ) )
            {
                //-----------------------------------------------
                // PROJ-1473
                // 질의에 사용된 컬럼정보를 수집한다.
                // 레코드저장방식의 처리인 경우,
                // 디스크테이블과 뷰의 질의에 사용된 컬럼정보 수집.
                //-----------------------------------------------

                IDE_TEST( qtc::setColumnExecutionPosition( aTemplate,
                                                           sNode,
                                                           sSFWGH,
                                                           sCallBackInfo->SFWGH )
                          != IDE_SUCCESS );

                /* PROJ-2462 ResultCache */
                qtc::checkLobAndEncryptColumn( aTemplate, aNode );

                // To Fix PR-9050
                // PRIOR Column의 존재 여부는 qtcNode->flag으로 검사해야 함.
                if ( (sNode->lflag & QTC_NODE_PRIOR_MASK)
                     == QTC_NODE_PRIOR_ABSENT )
                {
                    qtc::dependencySet( sNode->node.table, & sNode->depInfo );
                }
                else
                {
                    qtc::dependencyClear( & sNode->depInfo );
                }
            }
            else
            {
                qtc::dependencyClear( & sNode->depInfo );
            }

            //---------------------------------------------------------------
            // [ORDER BY구문에서의 Column]
            // ORDER BY 구문에 나타나는 Column 중 실제 Column이 아닌 경우가
            // 존재한다.
            // 다음과 같은 예를 통해 살펴 보자.
            //    SELECT i1 a1, i1 + 1 a2, sum(i1) a3, count(*) a4 FROM T1
            //        ORDER BY a1, a2, a3, a4;
            // 위와 같이 ORDER BY절의 column의 경우,
            //     a1 은 실제 Column이며,
            //     a2, a3는 arguments가 존재하므로 실제 Column이 아니며,
            //     a3, a4는 aggregation 연산이기 때문에 실제 Column이 아니다.
            // 따라서, a2, a3, a4와 같은 경우는 column module의 execute를
            // 셋팅해서는 안된다.
            //---------------------------------------------------------------


            // PROJ-1075
            // array 변수를 참조하는 column인 경우
            // ex) V1[1].I1
            // V1의 table, column을 mtcExecute->info정보에 세팅한다.

            if( ( sNode->lflag & QTC_NODE_PROC_VAR_ESTIMATE_MASK ) ==
                QTC_NODE_PROC_VAR_ESTIMATE_FALSE )
            {
                // PROJ-2533 array() 인 경우 index가 필요합니다.
                IDE_TEST_RAISE( ( sNode->node.arguments == NULL ) &&
                                ( ( (sNode->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK ) ==
                                  QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST ),
                                ERR_INVALID_FUNCTION_ARGUMENT );

                if( ( sNode->node.arguments != NULL ) &&
                    ( sArrayVariable != NULL ) )
                {
                    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                                    ERR_INVALID_FUNCTION_ARGUMENT );

                    // typeInfo의 첫번째 컬럼이 index column임.
                    sModule = sArrayVariable->typeInfo->columns->basicInfo->module;

                    IDE_TEST( mtf::makeConversionNodes( aNode,
                                                        aNode->arguments,
                                                        aTemplate,
                                                        aStack + 1,
                                                        aCallBack,
                                                        &sModule )
                              != IDE_SUCCESS );

                    aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecuteArrayColumn;

                    // arrayVariable의 index column과 동일한 type으로 argument에 conversion node를 달아준다.
                    // execute의 info에 해당 array 변수의 table, column정보를 넘겨준다.
                    IDU_FIT_POINT( "qtcColumn::qtcColumnEstimate::alloc::ColumnInfo" );
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF( qtcColumnInfo ),
                                                (void**)&sColumnInfo )
                              != IDE_SUCCESS );

                    sColumnInfo->table = sArrayVariable->common.table;
                    sColumnInfo->column = sArrayVariable->common.column;
                    sColumnInfo->objectId = sArrayVariable->common.objectID;   // PROJ-1073 Package

                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = (void*)sColumnInfo;

                    sNode->lflag &= ~QTC_NODE_PROC_VAR_ESTIMATE_MASK;
                    sNode->lflag |= QTC_NODE_PROC_VAR_ESTIMATE_TRUE;
                }
                else
                {
                    if (sNode->node.arguments == NULL &&
                        QTC_IS_AGGREGATE(sNode) == ID_FALSE)
                    {
                        /* BUG-37981
                           sIdcFlag == ID_FALSE인데, sNode->nonde.objectID != 0일 때가 있다.
                           그런 경우는 이미 estimate한 상태로 이다. 그러므로, execute정보 설정이 되어있다.
                           따라서, objectID != 0이면, 아무것도 안해도 된다. */
                        if( sNode->node.objectID == QS_EMPTY_OID)
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecute;
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
        }
        else
        {
            // array 변수를 참조하는 column인 경우

            // PROJ-1073 Package
            /* Package spec의 template를 가져온다. */
            IDE_TEST( qsxPkg::getPkgInfo( aNode->objectID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );

            IDE_DASSERT( sPkgInfo != NULL );

            sMtcTmplate = (mtcTemplate *)(sPkgInfo->tmplate);

            IDE_DASSERT( sMtcTmplate != NULL );

            if( ( sNode->lflag & QTC_NODE_PROC_VAR_ESTIMATE_MASK ) ==
                QTC_NODE_PROC_VAR_ESTIMATE_FALSE )
            {
                // PROJ-2533 array() 인 경우 index가 필요합니다.
                IDE_TEST_RAISE( ( sNode->node.arguments == NULL ) &&
                                ( ( (sNode->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK ) ==
                                  QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST ),
                                ERR_INVALID_FUNCTION_ARGUMENT );

                if( sNode->node.arguments != NULL )
                {
                    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                                    ERR_INVALID_FUNCTION_ARGUMENT );

                    // typeInfo의 첫번째 컬럼이 index column임.
                    sModule = sArrayVariable->typeInfo->columns->basicInfo->module;

                    IDE_TEST( mtf::makeConversionNodes( aNode,
                                                        aNode->arguments,
                                                        aTemplate,
                                                        aStack + 1,
                                                        aCallBack,
                                                        &sModule )
                              != IDE_SUCCESS );

                    IDU_FIT_POINT( "qtcColumn::qtcColumnEstimate::alloc::RealNode1" );
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF( mtcNode ) * 2,
                                                (void **)&sRealNode ) != IDE_SUCCESS );

                    sRealNode[0].table     = aNode->table;
                    sRealNode[0].column    = aNode->column;
                    sRealNode[0].objectID  = aNode->objectID;

                    sRealNode[1].table     = sArrayVariable->common.table;
                    sRealNode[1].column    = sArrayVariable->common.column;
                    sRealNode[1].objectID  = sArrayVariable->common.objectID;

                    sRealColumn  = *QTC_TMPL_COLUMN ( (qcTemplate*)sMtcTmplate,
                                                      (qtcNode*)sRealNode );

                    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(sStatement),
                                               (qtcNode *)aNode,
                                               sStatement,
                                               QC_SHARED_TMPLATE(sStatement),
                                               MTC_TUPLE_TYPE_INTERMEDIATE,
                                               1 )
                              != IDE_SUCCESS );

                    // PROJ-1073 Package
                    if( aNode->orgNode != NULL )
                    {
                        aNode->orgNode->table = aNode->table;
                        aNode->orgNode->column = aNode->column;
                        aNode->orgNode = NULL;
                    }

                    *QTC_STMT_EXECUTE( sStatement, (qtcNode*)aNode )
                                                 = qtcExecuteIndirectArrayColumn;
                    QTC_STMT_EXECUTE( sStatement, (qtcNode*)aNode )->calculateInfo
                                                 = (void*)sRealNode;
                    *QTC_STMT_COLUMN ( sStatement, (qtcNode*)aNode ) 
                                                 = sRealColumn;

                    sNode->lflag &= ~QTC_NODE_PROC_VAR_ESTIMATE_MASK;
                    sNode->lflag |= QTC_NODE_PROC_VAR_ESTIMATE_TRUE;
                }
                else
                {
                    IDU_FIT_POINT( "qtcColumn::qtcColumnEstimate::alloc::ReadNode2" );
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF( mtcNode ),
                                                (void **)&sRealNode ) != IDE_SUCCESS );

                    sRealNode->table     = aNode->table;
                    sRealNode->column    = aNode->column;
                    sRealNode->objectID  = aNode->objectID;

                    sRealColumn  = *QTC_TMPL_COLUMN ( (qcTemplate*)sMtcTmplate,
                                                      (qtcNode*)sRealNode );

                    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(sStatement),
                                               (qtcNode *)aNode,
                                               sStatement,
                                               QC_SHARED_TMPLATE(sStatement),
                                               MTC_TUPLE_TYPE_INTERMEDIATE,
                                               1 )
                              != IDE_SUCCESS );

                    // PROJ-1073 Package
                    if( aNode->orgNode != NULL )
                    {
                        aNode->orgNode->table = aNode->table;
                        aNode->orgNode->column = aNode->column;
                        aNode->orgNode = NULL;
                    }

                    *QTC_STMT_EXECUTE( sStatement, (qtcNode*)aNode )
                                                 = qtcExecuteIndirectColumn;
                    QTC_STMT_EXECUTE( sStatement, (qtcNode*)aNode )->calculateInfo
                                                 = (void*)sRealNode;
                    /* BUG-39340
                       UDType형 변수의 경우, 변수가 선언된 package의 QMP_MEM에서 할당받아 생성된다.
                       따라서, 해당변수가 선언된 package가 recompile 될 경우, memory는 free된다.
                       그렇기 때문에 현 QMP_MEM에서 할당받아서 정보를 구성해야한다.  */
                    if( ( sRealColumn.module->id >= MTD_UDT_ID_MIN ) &&
                        ( sRealColumn.module->id <= MTD_UDT_ID_MAX ) )
                    {
                        sRealType = ((qtcModule *)(sRealColumn.module))->typeInfo;

                        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(sStatement),
                                                qsTypes,
                                                &sType )
                                  != IDE_SUCCESS );
                        idlOS::memcpy( sType , sRealType, ID_SIZEOF(qsTypes) );
                        sType->columns = NULL;

                        IDE_TEST( qcm::copyQcmColumns( QC_QMP_MEM(sStatement),
                                                       sRealType->columns,
                                                       &sType->columns,
                                                       sRealType->columnCount )
                                  != IDE_SUCCESS );

                        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(sStatement),
                                                qtcModule,
                                                &sType->typeModule )
                                  != IDE_SUCCESS );

                        idlOS::memcpy(sType->typeModule,
                                      sRealColumn.module,
                                      ID_SIZEOF(mtdModule) );

                        sType->typeModule->typeInfo = sType;

                        mtc::copyColumn( &sColumn, &sRealColumn );

                        sColumn.module = (mtdModule *)sType->typeModule;
                    }
                    else
                    {
                        mtc::copyColumn( &sColumn, &sRealColumn );
                    }
                    *QTC_STMT_COLUMN ( sStatement, (qtcNode*)aNode )
                                                 = sColumn;

                    sNode->lflag &= ~QTC_NODE_PROC_VAR_ESTIMATE_MASK;
                    sNode->lflag |= QTC_NODE_PROC_VAR_ESTIMATE_TRUE;
                }
            }
            else
            {
                // Nothing to do.
            }

            // PROJ-1362
            sNode->lflag &= ~QTC_NODE_BINARY_MASK;

            if ( qtc::isEquiValidType( sNode,
                                       &(QC_SHARED_TMPLATE(sStatement)->tmplate) )
                 == ID_FALSE )
            {
                sNode->lflag |= QTC_NODE_BINARY_EXIST;
            }
            else
            {
                // Nothing to do.
            }
        }

        aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    }
    else if ( sNode->node.module == & qtc::subqueryModule )
    {
        //--------------------------------------------------------
        // BUG-25839
        // order by절에서 참조한 target alias가 subquery인 경우
        // column정보가 stack에 올라오지 않아 subquery의 target정보를
        // 직접 stack에 올린다.
        //--------------------------------------------------------

        sConvertedNode = mtf::convertedNode( sNode->node.arguments,
                                             & sCallBackInfo->tmplate->tmplate );

        aStack->column = aTemplate->rows[sConvertedNode->table].columns
            + sConvertedNode->column;

        //--------------------------------------------------------
        // target alias가 subquery인 경우 expression에 의해 conversion
        // node가 생성되는 경우가 있으므로 assign node를 생성하여
        // 연결정보는 그대로 유지하면서 conversion node가 생성될 수
        // 있게 한다.
        //--------------------------------------------------------

        // 새로운 subquery node 생성
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(sStatement),
                                qtcNode,
                                & sSubQNode )
                  != IDE_SUCCESS);

        // sNode를 그대로 복사
        idlOS::memcpy( sSubQNode, sNode, ID_SIZEOF(qtcNode) );

        // sNode를 assign node로 변경한다.
        IDE_TEST( qtc::makeAssign( sStatement,
                                   sNode,
                                   sSubQNode )
                  != IDE_SUCCESS );
    }
    else
    {
        aStack->column = aTemplate->rows[aNode->table].columns
            + aNode->column;
    }

    // PROJ-2394
    // view의 target column으로 list type이 올 수 있다.
    // list type인 경우 stack의 value가 필요하여 smiColumn.value에 기록해두었다.
    if ( aStack->column->module == &mtdList )
    {
        aStack->value = aStack->column->column.value;
    }
    else
    {
        // Nothing to do.
    }

    // BUG-41311
    if ( ( sNode->lflag & QTC_NODE_LOOP_VALUE_MASK ) ==
         QTC_NODE_LOOP_VALUE_EXIST )
    {
        *aStack = sSFWGH->thisQuerySet->loopStack;
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-42639 Monitoring query */
    if ( ( ( sNode->lflag & QTC_NODE_PROC_FUNCTION_MASK )
           == QTC_NODE_PROC_FUNCTION_TRUE ) ||
         ( ( sNode->lflag & QTC_NODE_SEQUENCE_MASK )
           == QTC_NODE_SEQUENCE_EXIST ) )
    {
        QC_SHARED_TMPLATE(sStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
        QC_SHARED_TMPLATE(sStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtcCalculate_Column(
    mtcNode*     aNode,
    mtcStack*    aStack,
    SInt         aRemain,
    void*,
    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Column의 연산을 수행한다.
 *
 * Implementation :
 *
 *    Stack에 column정보와 Value 정보를 Setting한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtcCalculate_Column"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;

    /* PROJ-2180
       qp 에서는 mtc::value 를 사용한다.
       다만 여기에서는 성능을 위해서 valueForModule 를 사용한다. */
    aStack->value  = (void*)mtd::valueForModule(
                                        (smiColumn*)aStack->column,
                                        aTemplate->rows[aNode->table].row,
                                        MTD_OFFSET_USE,
                                        aStack->column->module->staticNull );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtcCalculate_ArrayColumn(
    mtcNode*     aNode,
    mtcStack*    aStack,
    SInt         aRemain,
    void*        aInfo,
    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-1075
 *    Array변수의 index연산을 수행한 후 Column 연산을 수행한다.
 *
 * Implementation :
 *    search연산을 수행한다. 만약 lvalue이면 search-insert를 수행한다.
 *    Stack에 column정보와 Value 정보를 Setting한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtcCalculate_ArrayColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxArrayInfo  * sArrayInfo;
    mtcColumn     * sArrayColumn;
    qtcColumnInfo * sColumnInfo;
    idBool          sFound;
    qtcNode       * sNode = (qtcNode *)aNode;

    //fix PROJ-1596
    mtcTemplate   * sTmplate   = NULL;

    sColumnInfo = (qtcColumnInfo*)aInfo;

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    // argument의 연산을 수행한다.
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        // Value Error
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        sTmplate = aTemplate;

        // array변수의 column, value를 얻어온다.
        sArrayColumn = sTmplate->rows[sColumnInfo->table].columns + sColumnInfo->column;

        sArrayInfo = *((qsxArrayInfo **)( (UChar*) sTmplate->rows[sColumnInfo->table].row
                                          + sArrayColumn->column.offset ));

        IDE_TEST_RAISE( sArrayInfo == NULL, ERR_INVALID_ARRAY );

        if( ( sNode->lflag & QTC_NODE_LVALUE_MASK ) == QTC_NODE_LVALUE_ENABLE )
        {
            // lvalue이므로 search-insert를 하기 위해 ID_TRUE를 넘김
            IDE_TEST( qsxArray::searchKey( ((qcTemplate*)aTemplate)->stmt,
                                           sArrayInfo,
                                           aStack[1].column,
                                           aStack[1].value,
                                           ID_TRUE,
                                           &sFound )
                      != IDE_SUCCESS );
        }
        else
        {
            // rvalue이므로 search를 하기 위해 ID_FALSE를 넘김
            IDE_TEST( qsxArray::searchKey( ((qcTemplate*)aTemplate)->stmt,
                                           sArrayInfo,
                                           aStack[1].column,
                                           aStack[1].value,
                                           ID_FALSE,
                                           &sFound )
                      != IDE_SUCCESS );
        }

        IDE_TEST_RAISE( sFound == ID_FALSE, ERR_NO_DATA_FOUND );

        aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
        aStack->value  = (void*)mtc::value( aStack->column,
                                            sArrayInfo->row,
                                            MTD_OFFSET_USE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_DATA_FOUND );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_NO_DATA_FOUND));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_INVALID_ARRAY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtcCalculate_IndirectColumn(
    mtcNode*     aNode,
    mtcStack*    aStack,
    SInt         aRemain,
    void*        /* aInfo */,
    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC qtcCalculate_IndirectColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcTemplate * sTmplate   = NULL;
    void        * sFakeValue;
    mtcColumn   * sFakeColumn;
    qcStatement * sStatement = ((qcTemplate *)aTemplate)->stmt;
    mtcNode     * sRealNode;

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    sFakeColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sFakeValue  = (void *)mtc::value( sFakeColumn,
                                      aTemplate->rows[aNode->table].row,
                                      MTD_OFFSET_USE );

    sRealNode = (mtcNode *) QTC_TUPLE_EXECUTE( (mtcTuple *)(aTemplate->rows+aNode->table),
                                               (qtcNode *)aNode )->calculateInfo;

    // PROJ-1073 Package
    /* Package spec의 template를 가져온다. */
    IDE_TEST( qcuSessionPkg::getTmplate( sStatement,
                                         sRealNode->objectID,
                                         aStack,
                                         aRemain,
                                         &sTmplate )
              != IDE_SUCCESS );

    // package의 variable 초기화 시킬 때 이면 null이 가능하다
    IDE_TEST( sTmplate == NULL );

    aStack->column = sTmplate->rows[sRealNode->table].columns + sRealNode->column;
    aStack->value  = (void *)mtc::value( aStack->column,
                                         sTmplate->rows[sRealNode->table].row,
                                         MTD_OFFSET_USE );

    idlOS::memcpy( sFakeValue,
                   aStack->value,
                   sFakeColumn->column.size );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtcCalculate_IndirectArrayColumn(
    mtcNode*     aNode,
    mtcStack*    aStack,
    SInt         aRemain,
    void*        aInfo,
    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-1075
 *    Array변수의 index연산을 수행한 후 Column 연산을 수행한다.
 *
 * Implementation :
 *    search연산을 수행한다. 만약 lvalue이면 search-insert를 수행한다.
 *    Stack에 column정보와 Value 정보를 Setting한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtcCalculate_IndirectArrayColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxArrayInfo  * sArrayInfo;
    mtcColumn     * sArrayColumn;
    idBool          sFound;
    qtcNode       * sNode      = (qtcNode *)aNode;
    mtcNode       * sRealNode  = NULL;

    mtcTemplate   * sTmplate   = NULL;
    qcStatement   * sStatement = ((qcTemplate *)aTemplate)->stmt;
    void          * sFakeValue;
    mtcColumn     * sFakeColumn;

    //sColumnInfo = (qtcColumnInfo*)aInfo;
    sRealNode = (mtcNode *)aInfo;

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    sFakeColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sFakeValue  = (void*)mtc::value( sFakeColumn,
                                     aTemplate->rows[aNode->table].row,
                                     MTD_OFFSET_USE );

    // argument의 연산을 수행한다.
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        // Value Error
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        // PROJ-1073 Package
        IDE_TEST( qcuSessionPkg::getTmplate( sStatement,
                                             sRealNode->objectID,
                                             aStack,
                                             aRemain,
                                             &sTmplate )
                  != IDE_SUCCESS );

        // array변수의 column, value를 얻어온다.
        sArrayColumn = sTmplate->rows[sRealNode[1].table].columns + sRealNode[1].column;

        sArrayInfo = *((qsxArrayInfo **)( (UChar*) sTmplate->rows[sRealNode[1].table].row
                                          + sArrayColumn->column.offset ));

        IDE_TEST_RAISE( sArrayInfo == NULL, ERR_INVALID_ARRAY );

        if( ( sNode->lflag & QTC_NODE_LVALUE_MASK ) == QTC_NODE_LVALUE_ENABLE )
        {
            // lvalue이므로 search-insert를 하기 위해 ID_TRUE를 넘김
            IDE_TEST( qsxArray::searchKey( ((qcTemplate*)aTemplate)->stmt,
                                           sArrayInfo,
                                           aStack[1].column,
                                           aStack[1].value,
                                           ID_TRUE,
                                           &sFound )
                      != IDE_SUCCESS );
        }
        else
        {
            // rvalue이므로 search를 하기 위해 ID_FALSE를 넘김
            IDE_TEST( qsxArray::searchKey(((qcTemplate*)aTemplate)->stmt,
                                           sArrayInfo,
                                           aStack[1].column,
                                           aStack[1].value,
                                           ID_FALSE,
                                           &sFound )
                      != IDE_SUCCESS );
        }

        IDE_TEST_RAISE( sFound == ID_FALSE, ERR_NO_DATA_FOUND );

        aStack->column = sTmplate->rows[sRealNode[0].table].columns + sRealNode[0].column;
        aStack->value  = (void*)mtc::value( aStack->column,
                                            sArrayInfo->row,
                                            MTD_OFFSET_USE );

        idlOS::memcpy( sFakeValue,
                       aStack->value,
                       sFakeColumn->column.size );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_DATA_FOUND );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_NO_DATA_FOUND));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_INVALID_ARRAY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

