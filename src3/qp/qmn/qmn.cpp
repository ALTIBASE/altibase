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
 * $Id: qmn.cpp 88800 2020-10-06 10:29:32Z hykim $
 *
 * Description :
 *     여러 Plan Node가 공통적으로 사용하는 기능을 통합함.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <qtc.h>
#include <qcg.h>
#include <qmn.h>
#include <qmoUtil.h>

IDE_RC
qmn::printSubqueryPlan( qcTemplate   * aTemplate,
                        qtcNode      * aSubQuery,
                        ULong          aDepth,
                        iduVarString * aString,
                        qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    해당 Expression및 Predicate에 Subquery가 있을 경우,
 *    Subquery에 대한 Plan Tree 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode * sNode;
    qmnPlan * sPlan;
    UInt      sArguCount;
    UInt      sArguNo;

    if ( ( aSubQuery->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_SUBQUERY )
    {
        printSpaceDepth(aString, aDepth + 1);

        // BUG-47591
        if ( ( aMode == QMN_DISPLAY_ALL ) &&
             ( aTemplate->forceSubqueryCacheDisable == QTC_CACHE_FORCE_SUBQUERY_CACHE_NONE ) &&
             ( QCG_GET_SESSION_TRCLOG_DETAIL_INFORMATION( aTemplate->stmt ) == 1 ) &&
             ( aTemplate->cacheObjects != NULL ) &&
             ( aSubQuery->node.info != ID_UINT_MAX ) )
        {
            if ( ( aTemplate->cacheObjects[aSubQuery->node.info].mHitCnt  != 0 ) &&
                 ( aTemplate->cacheObjects[aSubQuery->node.info].mMissCnt != 0 ) )
            {
                iduVarStringAppendFormat( aString,
                                          "::SUB-QUERY BEGIN ( HIT_COUNT: %"ID_INT32_FMT", MISS_COUNT: %"ID_INT32_FMT" )\n",
                                          aTemplate->cacheObjects[aSubQuery->node.info].mHitCnt,
                                          aTemplate->cacheObjects[aSubQuery->node.info].mMissCnt );
            }
            else
            {
                // multiple row subquery
                iduVarStringAppend( aString, "::SUB-QUERY BEGIN\n" );
            }
        }
        else
        {
            iduVarStringAppend( aString, "::SUB-QUERY BEGIN\n" );
        }

        sPlan = aSubQuery->subquery->myPlan->plan;
        IDE_TEST( qmnPROJ::printPlan( aTemplate,
                                      sPlan,
                                      aDepth + 1,
                                      aString,
                                      aMode ) != IDE_SUCCESS );

        printSpaceDepth(aString, aDepth + 1);
        iduVarStringAppend( aString, "::SUB-QUERY END\n" );
    }
    else
    {
        sArguCount = ( aSubQuery->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

        if ( sArguCount > 0 ) // This node has arguments.
        {
            if ( (aSubQuery->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                for (sNode = (qtcNode *)aSubQuery->node.arguments;
                     sNode != NULL;
                     sNode = (qtcNode *)sNode->node.next)
                {
                    IDE_TEST( printSubqueryPlan( aTemplate,
                                                 sNode,
                                                 aDepth,
                                                 aString,
                                                 aMode ) != IDE_SUCCESS );
                }
            }
            else
            {
                for (sArguNo = 0,
                         sNode = (qtcNode *)aSubQuery->node.arguments;
                     sArguNo < sArguCount && sNode != NULL;
                     sArguNo++,
                         sNode = (qtcNode *)sNode->node.next)
                {
                    IDE_TEST( printSubqueryPlan( aTemplate,
                                                 sNode,
                                                 aDepth,
                                                 aString,
                                                 aMode ) != IDE_SUCCESS );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmn::printResult( qcTemplate        * aTemplate,
                  ULong               aDepth,
                  iduVarString      * aString,
                  const qmcAttrDesc * aResultDesc )
{
    UInt i, j;
    const qmcAttrDesc * sItrAttr;

    for( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString, " " );
    }
    iduVarStringAppend( aString, "[ RESULT ]\n" );

    for( sItrAttr = aResultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        for( j = 0; j < aDepth; j++ )
        {
            iduVarStringAppend( aString, " " );
        }

        if( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            iduVarStringAppend( aString, "#" );
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-2469 Optimize View Materialization
        // 사용되지 않는 Result에 대해서 Plan에 표시한다.
        if ( ( sItrAttr->flag & QMC_ATTR_USELESS_RESULT_MASK ) == QMC_ATTR_USELESS_RESULT_TRUE )
        {
            iduVarStringAppend( aString, "~" );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( qmoUtil::printPredInPlan( aTemplate,
                                            aString,
                                            0,
                                            (qtcNode *)sItrAttr->expr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmn::makeKeyRangeAndFilter( qcTemplate         * aTemplate,
                            qmnCursorPredicate * aPredicate )
{
/***********************************************************************
 *
 * Description :
 *    Cursor를 열기 위한 Key Range, Key Filter, Filter를 구성한다.
 *
 * Implementation :
 *    - Key Range 구성
 *    - Key Filter 구성
 *    - Filter 구성
 *        - Variable Key Range, Variable Key Filter가 Filter로
 *          구분되었을 때, 이를 조합하여 하나의 Filter로 구성한다.
 *    - IN SUBQUERY가 Key Range로 사용될 경우 다음과 같은 사항을
 *      주의하여 처리한다.
 *        - Key Range 생성이 실패하는 것은 더 이상 Key Range가 없다는
 *          의미이므로 더 이상 record를 fetch해서는 안된다.
 *
 ***********************************************************************/

#define IDE_FN "qmn::makeKeyRangeAndFilter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode     * sKeyRangeFilter;  // Variable Key Range로부터 생성된 Filter
    qtcNode     * sKeyFilterFilter; // Variable Key Filter로부터 생성된 Filter

    //----------------------------
    // 적합성 검사
    //----------------------------

    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aPredicate != NULL );

    IDE_DASSERT( aPredicate->filterCallBack != NULL );
    IDE_DASSERT( aPredicate->callBackDataAnd != NULL );
    IDE_DASSERT( aPredicate->callBackData != NULL );

    IDE_DASSERT( aPredicate->fixKeyRangeOrg == NULL ||
                 aPredicate->varKeyRangeOrg == NULL );
    IDE_DASSERT( aPredicate->fixKeyFilterOrg == NULL ||
                 aPredicate->varKeyRangeOrg == NULL );

    sKeyRangeFilter = NULL;
    sKeyFilterFilter = NULL;

    //---------------------------------------------------------------
    // Key Range의 설정
    //---------------------------------------------------------------

    IDE_TEST( makeKeyRange( aTemplate,
                            aPredicate,
                            & sKeyRangeFilter ) != IDE_SUCCESS );

    //---------------------------------------------------------------
    // Key Filter의 설정
    //---------------------------------------------------------------

    IDE_TEST( makeKeyFilter( aTemplate,
                             aPredicate,
                             & sKeyFilterFilter ) != IDE_SUCCESS );

    //---------------------------------------------------------------
    // Filter의 설정
    //    A4에서는 다음과 같이 세 종류의 Filter가 생성될 수 있다.
    //       - Variable Key Range => Filter
    //       - Variable Key Filter => Filter
    //       - Normal Filter => Filter
    //    다음과 같은 Filter는 Cursor에 의해 처리되지 않는다.
    //       - Constant Filter : 전처리됨
    //       - Subquery Filter : 후처리됨
    //---------------------------------------------------------------

    // 다양한 Filter를 Storage Manager에서 처리할 수 있도록
    // CallBack을 구성한다.
    IDE_TEST( makeFilter( aTemplate,
                          aPredicate,
                          sKeyRangeFilter,
                          sKeyFilterFilter,
                          aPredicate->filter ) != IDE_SUCCESS );

    //---------------------------------------------------------------
    //  Key Range 및 Key Filter의 보정
    //      - Key Range가 존재할 경우 Partial Key를 설정
    //      - Key Range가 없을 경우 default key range 사용
    //      - Key Filter가 없을 경우 default key filter 사용
    //---------------------------------------------------------------

    // Key Range 추출이 실패한 경우, Default Key Range를 설정한다.
    if ( aPredicate->keyRange == NULL )
    {
        aPredicate->keyRange = smiGetDefaultKeyRange();
    }
    else
    {
        // Nothing To Do
    }

    // Key Filter 추출이 실패한 경우, Default Key Filter를 설정한다.
    if ( aPredicate->keyFilter == NULL )
    {
        // Key Filter와 Key Range는 동일한 형태를 갖는다.
        aPredicate->keyFilter = smiGetDefaultKeyRange();
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmn::makeKeyRange( qcTemplate         * aTemplate,
                   qmnCursorPredicate * aPredicate,
                   qtcNode           ** aFilter )
{
/***********************************************************************
 *
 * Description :
 *     Key Range를 생성한다.
 *
 * Implementation :
 *     Key Range의 설정
 *       - Fixed Key Range의 경우, Key Range 생성
 *       - Variable Key Range의 경우, Key Range 생성
 *       - 실패시, Filter로 처리함.
 *       - Not Null Key Range가 필요한 경우, Key Range 생성
 *
 ***********************************************************************/
#define IDE_FN "qmn::makeKeyRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt              sCompareType;
    qtcNode         * sAndNode;
    mtdBooleanType    sValue;

    if ( aPredicate->fixKeyRangeOrg != NULL )
    {
        //------------------------------------
        // Fixed Key Range가 존재하는 경우
        //------------------------------------

        IDE_DASSERT( aPredicate->index != NULL );
        IDE_DASSERT( aPredicate->fixKeyRangeArea != NULL );

        // PROJ-1872
        if ( ( aTemplate->tmplate.rows[aPredicate->tupleRowID].lflag
               & MTC_TUPLE_STORAGE_MASK )
             == MTC_TUPLE_STORAGE_DISK )
        {
            // Disk Table의 Key Range는
            // Stored 타입의 칼럼 value와 Mt 타입의 value간의
            // compare 
            sCompareType = MTD_COMPARE_STOREDVAL_MTDVAL;
        }
        else
        {
            /* BUG-43006 FixedTable Indexing Filter
             * FixedTable의 Index에는 indexHandle이 없을 수 있다
             */
            if ( aPredicate->index->indexHandle != NULL )
            {
                /*
                 * PROJ-2433
                 * Direct Key Index를 위한 key compare 함수 type 세팅
                 */
                if ( ( smiTable::getIndexInfo( aPredicate->index->indexHandle ) &
                     SMI_INDEX_DIRECTKEY_MASK ) == SMI_INDEX_DIRECTKEY_TRUE )
                {
                    sCompareType = MTD_COMPARE_INDEX_KEY_MTDVAL;
                }
                else
                {
                    sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
                }
            }
            else
            {
                sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
            }
        }

        IDE_TEST(
            qmoKeyRange::makeKeyRange( aTemplate,
                                       aPredicate->fixKeyRangeOrg,
                                       aPredicate->index->keyColCount,
                                       aPredicate->index->keyColumns,
                                       aPredicate->index->keyColsFlag,
                                       sCompareType,
                                       aPredicate->fixKeyRangeArea,
                                       aPredicate->fixKeyRangeSize,
                                       &(aPredicate->fixKeyRange),
                                       aFilter )
            != IDE_SUCCESS );

        // host 변수가 없으므로 fixed key를 생성 실패하는 경우는 없다.
        IDE_DASSERT( *aFilter == NULL );
        
        aPredicate->keyRange = aPredicate->fixKeyRange;
    }
    else
    {
        if ( aPredicate->varKeyRangeOrg != NULL )
        {
            //------------------------------------
            // Variable Key Range가 존재하는 경우
            //------------------------------------

            IDE_DASSERT( aPredicate->index != NULL );
            IDE_DASSERT( aPredicate->varKeyRangeArea != NULL );

            // PROJ-1872
            if ( ( aTemplate->tmplate.rows[aPredicate->tupleRowID].lflag
                   & MTC_TUPLE_STORAGE_MASK )
                 == MTC_TUPLE_STORAGE_DISK )
            {
                // Disk Table의 Key Range는
                // Stored 타입의 칼럼 value와 Mt 타입의 value간의
                // compare 
                sCompareType = MTD_COMPARE_STOREDVAL_MTDVAL;
            }
            else
            {
                /* BUG-43006 FixedTable Indexing Filter
                 * FixedTable의 Index에는 indexHandle이 없을 수 있다
                 */
                if ( aPredicate->index->indexHandle != NULL )
                {
                    /*
                     * PROJ-2433
                     * Direct Key Index를 위한 key compare 함수 type 세팅
                     */
                    if ( ( smiTable::getIndexInfo( aPredicate->index->indexHandle ) &
                         SMI_INDEX_DIRECTKEY_MASK ) == SMI_INDEX_DIRECTKEY_TRUE )
                    {
                        sCompareType = MTD_COMPARE_INDEX_KEY_MTDVAL;
                    }
                    else
                    {
                        sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
                    }
                }
                else
                {
                    sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
                }
            }

            // BUG-39036 select * from t1 where :emp is null or i1 = :emp;
            // 상수값이 존재하는 keyRange 는 실행시점에 수행을 통해서 제거한다.
            // 상수값이 true 이면 keyRange 를 제거
            for ( sAndNode = (qtcNode*)(aPredicate->varKeyRangeOrg->node.arguments);
                  sAndNode != NULL;
                  sAndNode = (qtcNode *)(sAndNode->node.next) )
            {
                if( qtc::haveDependencies( &sAndNode->depInfo ) == ID_FALSE )
                {
                    IDE_TEST( qtc::calculate( (qtcNode*)(sAndNode->node.arguments),
                                              aTemplate )
                              != IDE_SUCCESS );

                    sValue  = *(mtdBooleanType*)(aTemplate->tmplate.stack->value);

                    if ( sValue == MTD_BOOLEAN_TRUE )
                    {
                        aPredicate->varKeyRange           = NULL;
                        aPredicate->varKeyRangeOrg        = NULL;
                        aPredicate->varKeyRange4FilterOrg = NULL;
                        break;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
                else
                {
                    // Nothing To Do
                }
            }

            if ( aPredicate->varKeyRangeOrg != NULL )
            {
                IDE_TEST(
                    qmoKeyRange::makeKeyRange( aTemplate,
                                            aPredicate->varKeyRangeOrg,
                                            aPredicate->index->keyColCount,
                                            aPredicate->index->keyColumns,
                                            aPredicate->index->keyColsFlag,
                                            sCompareType,
                                            aPredicate->varKeyRangeArea,
                                            aPredicate->varKeyRangeSize,
                                            &(aPredicate->varKeyRange),
                                            aFilter )
                    != IDE_SUCCESS );

                if ( *aFilter != NULL )
                {
                    aPredicate->keyRange = NULL;

                    // Key Range 생성이 실패한 경우 미리
                    // 저장해 둔 Fix Key Range를 사용
                    *aFilter = aPredicate->varKeyRange4FilterOrg;
                }
                else
                {
                    // Key Range 생성이 성공한 경우

                    aPredicate->keyRange = aPredicate->varKeyRange;
                }
            }
            else
            {
                aPredicate->keyRange = NULL;
            }
        }
        else
        {
            //----------------------------------
            // NOT NULL RANGE 구성
            //----------------------------------    

            // Not Null Ranage 생성
            // (1) indexable min-max인데, keyrange가 없는 경우
            // (2) Merge Join 하위의 SCAN 일 경우 ( To Fix BUG-8747 )
            if ( (aPredicate->notNullKeyRange != NULL) &&
                 (aPredicate->index != NULL) )
            {    
                IDE_TEST( qmoKeyRange::makeNotNullRange( aPredicate,
                                                         aPredicate->index->keyColumns,
                                                         aPredicate->notNullKeyRange )
                          != IDE_SUCCESS );
                
                aPredicate->keyRange = aPredicate->notNullKeyRange;
            }
            else
            {
                //------------------------------------
                // Key Range가 없는 경우
                //------------------------------------
                
                aPredicate->keyRange = NULL;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmn::makeKeyFilter( qcTemplate         * aTemplate,
                    qmnCursorPredicate * aPredicate,
                    qtcNode           ** aFilter )
{
/***********************************************************************
 *
 * Description :
 *     Key Filter를 생성한다.
 *
 * Implementation :
 *     Key Filter의 설정
 *       - Key Range가 존재하는 경우에만 유효하다.
 *       - Fixed Key Filter인 경우, Key Filter 생성
 *       - Variable Key Filter의 경우, Key Filter 생성
 *
 ***********************************************************************/
#define IDE_FN "qmn::makeKeyFilter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt              sCompareType;
    qtcNode         * sAndNode;
    mtdBooleanType    sValue;

    if ( aPredicate->keyRange == NULL )
    {
        //------------------------------
        // Key Range가 없는 경우
        //------------------------------

        // 적합성 검사
        // key range가 없거나 실패하는 경우라면,
        // Fixed Key Filter는 존재할 수 없다.
        IDE_DASSERT( aPredicate->fixKeyFilterOrg == NULL );
        
        aPredicate->keyFilter = NULL;        
        *aFilter = aPredicate->varKeyFilter4FilterOrg;
    }
    else
    {
        //------------------------------
        // Key Range 가 존재하는 경우
        //------------------------------

        if ( aPredicate->fixKeyFilterOrg != NULL )
        {
            //------------------------------
            // Fixed Key Filter가 존재하는 경우
            //------------------------------

            // 적합성 검사
            // Fixed Key Filter와 Variable Key Range는 상존할 수 없다.
            IDE_DASSERT( aPredicate->varKeyRangeOrg == NULL );
            
            IDE_DASSERT( aPredicate->index != NULL );
            IDE_DASSERT( aPredicate->fixKeyFilterArea != NULL );

            // PROJ-1872
            if ( ( aTemplate->tmplate.rows[aPredicate->tupleRowID].lflag
                   & MTC_TUPLE_STORAGE_MASK )
                 == MTC_TUPLE_STORAGE_DISK )
            {
                // Disk Table의 Key Range는
                // Stored 타입의 칼럼 value와 Mt 타입의 value간의
                // compare 
                sCompareType = MTD_COMPARE_STOREDVAL_MTDVAL;
            }
            else
            {
                sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
            }

            IDE_TEST(
                qmoKeyRange::makeKeyFilter(
                    aTemplate,
                    aPredicate->fixKeyFilterOrg,
                    aPredicate->index->keyColCount,
                    aPredicate->index->keyColumns,
                    aPredicate->index->keyColsFlag,
                    sCompareType,
                    aPredicate->fixKeyFilterArea,
                    aPredicate->fixKeyFilterSize,
                    &(aPredicate->fixKeyFilter),
                    aFilter )
                != IDE_SUCCESS );

            // host 변수가 없으므로 fixed key를 생성 실패하는 경우는 없다.
            IDE_DASSERT( *aFilter == NULL );
            
            aPredicate->keyFilter = aPredicate->fixKeyFilter;
        }
        else
        {
            if ( aPredicate->varKeyFilterOrg != NULL )
            {
                //------------------------------
                // Variable Key Filter가 존재하는 경우
                //------------------------------

                IDE_DASSERT( aPredicate->index != NULL );
                IDE_DASSERT( aPredicate->varKeyFilterArea != NULL );

                // PROJ-1872
                if ( ( aTemplate->tmplate.rows[aPredicate->tupleRowID].lflag
                       & MTC_TUPLE_STORAGE_MASK )
                     == MTC_TUPLE_STORAGE_DISK )
                {
                    // Disk Table의 Key Range는
                    // Stored 타입의 칼럼 value와 Mt 타입의 value간의
                    // compare 
                    sCompareType = MTD_COMPARE_STOREDVAL_MTDVAL;
                }
                else
                {
                    sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
                }

                // BUG-48076 disk table에 with 구문 사용 시 or 절과 결합된 constant predicate가 존재할 경우 결과 오류 
                // 상수값이 존재하는 keyFilter 는 실행시점에 수행을 통해서 제거한다.
                // 상수값이 true 이면 keyFilter 를 제거
                for ( sAndNode = (qtcNode*)(aPredicate->varKeyFilterOrg->node.arguments);
                      sAndNode != NULL;
                      sAndNode = (qtcNode *)(sAndNode->node.next) )
                {
                    if( qtc::haveDependencies( &sAndNode->depInfo ) == ID_FALSE )
                    {
                        IDE_TEST( qtc::calculate( (qtcNode*)(sAndNode->node.arguments),
                                                  aTemplate )
                                  != IDE_SUCCESS );

                        sValue  = *(mtdBooleanType*)(aTemplate->tmplate.stack->value);

                        if ( sValue == MTD_BOOLEAN_TRUE )
                        {
                            aPredicate->varKeyFilter           = NULL;
                            aPredicate->varKeyFilterOrg        = NULL;
                            aPredicate->varKeyFilter4FilterOrg = NULL;
                            break;
                        }
                        else
                        {
                            // Nothing To Do
                        }
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }

                if ( aPredicate->varKeyFilterOrg != NULL )
                {
                    IDE_TEST(
                        qmoKeyRange::makeKeyFilter(
                            aTemplate,
                            aPredicate->varKeyFilterOrg,
                            aPredicate->index->keyColCount,
                            aPredicate->index->keyColumns,
                            aPredicate->index->keyColsFlag,
                            sCompareType,
                            aPredicate->varKeyFilterArea,
                            aPredicate->varKeyFilterSize,
                            &(aPredicate->varKeyFilter),
                            aFilter )
                        != IDE_SUCCESS );

                    if ( *aFilter != NULL )
                    {
                        // Key Filter 생성 실패
                        aPredicate->keyFilter = NULL;

                        // Key Filter 생성이 실패한 경우 미리
                        // 저장해 둔 Fix Key Filter를 사용
                        *aFilter = aPredicate->varKeyFilter4FilterOrg;
                    }
                    else
                    {
                        // Key Filter 생성 성공
                        aPredicate->keyFilter = aPredicate->varKeyFilter;
                    }
                }
                else
                {
                    aPredicate->keyFilter = NULL;
                }
            }
            else
            {
                //------------------------------
                // Key Filter가 없는 경우
                //------------------------------

                aPredicate->keyFilter = NULL;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmn::makeFilter( qcTemplate         * aTemplate,
                 qmnCursorPredicate * aPredicate,
                 qtcNode            * aFirstFilter,
                 qtcNode            * aSecondFilter,
                 qtcNode            * aThirdFilter )
{
/***********************************************************************
 *
 * Description :
 *    여러 Filter를 조합하여 Storage Manager에서 사용할 수 있도록
 *    CallBack 을 생성해 준다.
 * Implementation :
 *    입력된 Filter가 NULL이 아닐 경우, CallBackData를 구성하고
 *    최종적으로 CallBackAnd를 구성한다.
 *
 ***********************************************************************/

    UInt   sFilterCnt;
    idBool sIsOnlyThirdFilter = ID_TRUE; /* PROJ-2632 */

    if ( (aFirstFilter == NULL) &&
         (aSecondFilter == NULL) &&
         (aThirdFilter == NULL) )
    {
        //---------------------------------
        // Filter가 없는 경우
        //---------------------------------
        // Default Filter를 설정
        idlOS::memcpy( aPredicate->filterCallBack,
                       smiGetDefaultFilter(),
                       ID_SIZEOF(smiCallBack) );
    }
    else
    {
        //---------------------------------
        // Filter가 존재하는 경우
        //     - 존재하는 각 Filter마다 CallBack Data를 구성하고,
        //     - 각각의 CallBack Data를 연결
        //     - 마지막으로 CallBack을 구성한다.
        //---------------------------------

        // Filter가 존재하는 경우 CallBack Data를 구성한다.
        sFilterCnt = 0;
        if ( aFirstFilter != NULL )
        {
            qtc::setSmiCallBack( & aPredicate->callBackData[sFilterCnt],
                                 aFirstFilter,
                                 aTemplate,
                                 aPredicate->tupleRowID );
            sFilterCnt++;

            sIsOnlyThirdFilter = ID_FALSE; /* PROJ-2632 */
        }
        else
        {
            // Nothing to do.
        }

        if ( aSecondFilter != NULL )
        {
            qtc::setSmiCallBack( & aPredicate->callBackData[sFilterCnt],
                                 aSecondFilter,
                                 aTemplate,
                                 aPredicate->tupleRowID );
            sFilterCnt++;

            sIsOnlyThirdFilter = ID_FALSE; /* PROJ-2632 */
        }
        else
        {
            // Nothing to do.
        }

        if ( aThirdFilter != NULL )
        {
            qtc::setSmiCallBack( & aPredicate->callBackData[sFilterCnt],
                                 aThirdFilter,
                                 aTemplate,
                                 aPredicate->tupleRowID );
            sFilterCnt++;
        }
        else
        {
            sIsOnlyThirdFilter = ID_FALSE; /* PROJ-2632 */
        }

        // CallBack Data들을 연결한다.
        switch ( sFilterCnt )
        {
            case 1:
                {
                    // BUG-12514 fix
                    // Filter가 하나일 때는 callBackDataAnd를 만들지 않고
                    // 바로 callBackData를 넘겨준다.
                    break;
                }
            case 2:
                {
                    // 2개의 CallBack Data를 연결
                    qtc::setSmiCallBackAnd( aPredicate->callBackDataAnd,
                                            & aPredicate->callBackData[0],
                                            & aPredicate->callBackData[1],
                                            NULL );
                    break;
                }
            case 3:
                {
                    // 3개의 CallBack Data를 연결
                    qtc::setSmiCallBackAnd( aPredicate->callBackDataAnd,
                                            & aPredicate->callBackData[0],
                                            & aPredicate->callBackData[1],
                                            & aPredicate->callBackData[2] );
                    break;
                }
            default :
                {
                    IDE_DASSERT(0);
                    break;
                }
        } // end of switch

        // BUG-12514 fix
        // Filter가 하나일 때는 callBackDataAnd를 만들지 않고
        // 바로 callBackData를 넘겨준다.
        if( sFilterCnt > 1 )
        {
            aPredicate->filterCallBack->callback = qtc::smiCallBackAnd;
            aPredicate->filterCallBack->data = aPredicate->callBackDataAnd;
        }
        else
        {
            aPredicate->filterCallBack->callback = qtc::smiCallBack;
            aPredicate->filterCallBack->data = aPredicate->callBackData;
        }

        /* PROJ-2632 */
        if ( sIsOnlyThirdFilter == ID_TRUE )
        {
            if ( mtx::generateSerialExecute( &( aTemplate->tmplate ),
                                             aPredicate->mSerialFilterInfo,
                                             aPredicate->mSerialExecuteData,
                                             aPredicate->tupleRowID )
                 == IDE_SUCCESS )
            {
                aPredicate->filterCallBack->callback = mtx::doSerialFilterExecute;
                aPredicate->filterCallBack->data     = aPredicate->mSerialExecuteData;
            }
            else
            {
                aPredicate->mSerialExecuteData = NULL;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmn::getReferencedTableInfo( qmnPlan     * aPlan,
                             const void ** aTableHandle,
                             smSCN       * aTableSCN,
                             idBool      * aIsFixedTable )
{
    qmncSCAN * sScan;
    qmncCUNT * sCunt;

    if( aPlan->type == QMN_SCAN )
    {
        sScan = ( qmncSCAN * )aPlan;

        *aTableHandle = sScan->table;
        *aTableSCN    = sScan->tableSCN;

        if ( ( sScan->flag & QMNC_SCAN_TABLE_FV_MASK )
             == QMNC_SCAN_TABLE_FV_TRUE )
        {
            *aIsFixedTable = ID_TRUE;
        }
        else
        {
            *aIsFixedTable = ID_FALSE;
        }
    }
    else if( aPlan->type == QMN_CUNT )
    {
        sCunt = ( qmncCUNT * )aPlan;
        *aTableHandle = sCunt->tableRef->tableInfo->tableHandle;
        *aTableSCN    = sCunt->tableSCN;

        if ( ( sCunt->flag & QMNC_CUNT_TABLE_FV_MASK )
             == QMNC_CUNT_TABLE_FV_TRUE )
        {
            *aIsFixedTable = ID_TRUE;
        }
        else
        {
            *aIsFixedTable = ID_FALSE;
        }
    }
    else
    {
        IDE_RAISE( ERR_INVALID_PLAN_TYPE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_PLAN_TYPE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmn::getReferencedTableInfo",
                                  "Invalid plan type" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmn::notifyOfSelectedMethodSet( qcTemplate * aTemplate, qmnPlan * aPlan )
{
    if( aPlan->type == QMN_SCAN )
    {
        IDE_TEST( qmnSCAN::notifyOfSelectedMethodSet( aTemplate,
                                                      (qmncSCAN*)aPlan )
                  != IDE_SUCCESS );
    }
    else if( aPlan->type == QMN_CUNT )
    {
        IDE_TEST( qmnCUNT::notifyOfSelectedMethodSet( aTemplate,
                                                      (qmncCUNT*)aPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmn::makeNullRow( mtcTuple   * aTuple,
                  void       * aNullRow )
{
/***********************************************************************
 *
 * Description : PROJ-1705 
 *
 *    디스크테이블의 경우, null row를 sm에서 관리하지 않고,
 *    qp에서 필요한 시점에 null row를 생성/저장해두고 이를 사용
 * 
 * Implementation :
 *
 *
 ***********************************************************************/

    UShort            sColumnCnt;
    mtcColumn       * sColumn;
    UChar           * sColumnPtr;
    smcTableHeader  * sTableHeader;

    //----------------------------------------------
    // PROJ_1705_PEH_TODO
    // sm으로부터 필요한 컬럼의 데이타만 qp메모리영역으로 복사하게 되는데,
    // 이 qp 메모리영역도 필요한 컬럼에 대한 공간만 할당받게 코드수정시
    // 이 부분도 코드변경되어야 함.
    //----------------------------------------------    

    for( sColumnCnt = 0; sColumnCnt < aTuple->columnCount; sColumnCnt++ )
    {
        sColumn = &(aTuple->columns[sColumnCnt]);

        // PROJ-2429 Dictionary based data compress for on-disk DB
        if ( (sColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK) 
             == SMI_COLUMN_COMPRESSION_TRUE )
        {
            sColumnPtr = (UChar*)aNullRow + sColumn->column.offset;

            sTableHeader = 
                (smcTableHeader*)SMI_MISC_TABLE_HEADER(smiGetTable( sColumn->column.mDictionaryTableOID ));

            IDE_DASSERT( sTableHeader->mNullOID != SM_NULL_OID );

            idlOS::memcpy( sColumnPtr, &sTableHeader->mNullOID, ID_SIZEOF(smOID));
        }
        else
        {
            sColumnPtr = (UChar *)mtc::value( sColumn,
                                              aNullRow,
                                              MTD_OFFSET_USE ); 

            sColumn->module->null( sColumn, 
                                   sColumnPtr ); 
        }
    }

    return IDE_SUCCESS;

}

void qmn::printMTRinfo( iduVarString * aString,
                        ULong          aDepth,
                        qmcMtrNode   * aMtrNode,
                        const SChar  * aMtrName,
                        UShort         aSelfID,
                        UShort         aRefID1,
                        UShort         aRefID2 )
{
/***********************************************************************
 *
 * Description : PROJ-2242
 *    mtr node info 를 출력한다.
 *
 ***********************************************************************/

    ULong i;
    UInt  j;

    // BUG-37245
    // tuple id type 을 UShort 로 통일하되 default 출력값은 -1 로 유지한다.
    SInt  sNonMtrId = QMN_PLAN_PRINT_NON_MTR_ID;

    qmcMtrNode  * sMtrNode;

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString, " " );
    }

    iduVarStringAppendFormat( aString,
                              "[ %s NODE INFO, "
                              "SELF: %"ID_INT32_FMT", "
                              "REF1: %"ID_INT32_FMT", "
                              "REF2: %"ID_INT32_FMT" ]\n",
                              aMtrName,
                              ( aSelfID == ID_USHORT_MAX )? sNonMtrId: (SInt)aSelfID,
                              ( aRefID1 == ID_USHORT_MAX )? sNonMtrId: (SInt)aRefID1,
                              ( aRefID2 == ID_USHORT_MAX )? sNonMtrId: (SInt)aRefID2 );

    for( sMtrNode = aMtrNode, j = 0;
         sMtrNode != NULL;
         sMtrNode = sMtrNode->next, j++ )
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString, " " );
        }

        switch( sMtrNode->flag & QMC_MTR_TYPE_MASK )
        {
            case QMC_MTR_TYPE_DISK_TABLE:
                iduVarStringAppendFormat(
                    aString,
                    "sMtrNode[%"ID_INT32_FMT"] : src[%"ID_INT32_FMT", ROWID],"
                    "dst[%"ID_INT32_FMT", %"ID_INT32_FMT"]\n",
                    j, 
                    ( sMtrNode->srcNode->node.table  == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->srcNode->node.table,
                    ( sMtrNode->dstNode->node.table  == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->dstNode->node.table,
                    ( sMtrNode->dstNode->node.column == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->dstNode->node.column
                    );
                break;

            case QMC_MTR_TYPE_MEMORY_TABLE:
                iduVarStringAppendFormat(
                    aString,
                    "sMtrNode[%"ID_INT32_FMT"] : src[%"ID_INT32_FMT", ROWPTR],"
                    "dst[%"ID_INT32_FMT", %"ID_INT32_FMT"]\n",
                    j, 
                    ( sMtrNode->srcNode->node.table  == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->srcNode->node.table,
                    ( sMtrNode->dstNode->node.table  == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->dstNode->node.table,
                    ( sMtrNode->dstNode->node.column == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->dstNode->node.column
                    );
                break;

            case QMC_MTR_TYPE_MEMORY_KEY_COLUMN:
            case QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN:
                iduVarStringAppendFormat(
                    aString,
                    "sMtrNode[%"ID_INT32_FMT"] : src[%"ID_INT32_FMT", *%"ID_INT32_FMT"],"
                    "dst[%"ID_INT32_FMT", %"ID_INT32_FMT"]\n",
                    j, 
                    ( sMtrNode->srcNode->node.table  == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->srcNode->node.table,
                    ( sMtrNode->srcNode->node.column == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->srcNode->node.column,
                    ( sMtrNode->dstNode->node.table  == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->dstNode->node.table,
                    ( sMtrNode->dstNode->node.column == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->dstNode->node.column
                    );
                break;

            default:
                iduVarStringAppendFormat(
                    aString,
                    "sMtrNode[%"ID_INT32_FMT"] : src[%"ID_INT32_FMT", %"ID_INT32_FMT"],"
                    "dst[%"ID_INT32_FMT", %"ID_INT32_FMT"]\n",
                    j, 
                    ( sMtrNode->srcNode->node.table  == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->srcNode->node.table,
                    ( sMtrNode->srcNode->node.column == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->srcNode->node.column,
                    ( sMtrNode->dstNode->node.table  == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->dstNode->node.table,
                    ( sMtrNode->dstNode->node.column == ID_USHORT_MAX )? sNonMtrId: (SInt)sMtrNode->dstNode->node.column
                    );
                break;
        }
    }
}

void qmn::printJoinMethod( iduVarString * aString,
                           UInt           aQmnFlag )
{
/***********************************************************************
 *
 * Description : PROJ-2242
 *   JoinMethod를 출력한다.
 *
 ***********************************************************************/

    switch( (aQmnFlag & QMN_PLAN_JOIN_METHOD_TYPE_MASK) )
    {
        case QMN_PLAN_JOIN_METHOD_FULL_NL :
            iduVarStringAppend( aString,
                                "METHOD: NL" );
            break;
        case QMN_PLAN_JOIN_METHOD_INDEX :
        case QMN_PLAN_JOIN_METHOD_INVERSE_INDEX :
            iduVarStringAppend( aString,
                                "METHOD: INDEX_NL" );
            break;
        case QMN_PLAN_JOIN_METHOD_FULL_STORE_NL :
            iduVarStringAppend( aString,
                                "METHOD: STORE_NL" );
            break;
        case QMN_PLAN_JOIN_METHOD_ANTI :
            iduVarStringAppend( aString,
                                "METHOD: ANTI" );
            break;
        case QMN_PLAN_JOIN_METHOD_ONE_PASS_HASH :
        case QMN_PLAN_JOIN_METHOD_TWO_PASS_HASH :
        case QMN_PLAN_JOIN_METHOD_INVERSE_HASH  :
            iduVarStringAppend( aString,
                                "METHOD: HASH" );
            break;
        case QMN_PLAN_JOIN_METHOD_ONE_PASS_SORT :
        case QMN_PLAN_JOIN_METHOD_TWO_PASS_SORT :
        case QMN_PLAN_JOIN_METHOD_INVERSE_SORT  :
            iduVarStringAppend( aString,
                                "METHOD: SORT" );
            break;
        case QMN_PLAN_JOIN_METHOD_MERGE :
            iduVarStringAppend( aString,
                                "METHOD: MERGE" );
            break;
        default :
            IDE_DASSERT(0);
            break;
    }
}

void qmn::printCost( iduVarString * aString,
                     SDouble        aCost )
{
/***********************************************************************
 *
 * Description : PROJ-2242
 *   cost info를 출력한다.
 *
 ***********************************************************************/

    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
    {
        iduVarStringAppendFormat( aString,
                                  ", COST: %.2f )\n",
                                  aCost );
    }
    else
    {
        iduVarStringAppend( aString,
                            ", COST: BLOCKED )\n" );
    }
}

void qmn::printSpaceDepth(iduVarString* aString, ULong aDepth)
{
    ULong i;

    i = 0;
    while (i + 8 <= aDepth)
    {
        iduVarStringAppendLength(aString, "        ", 8);
        i += 8;
    }
    while (i + 4 <= aDepth)
    {
        iduVarStringAppendLength(aString, "    ", 4);
        i += 4;
    }
    while (i < aDepth)
    {
        iduVarStringAppendLength(aString, " ", 1);
        i++;
    }
}

void qmn::printResultCacheRef( iduVarString    * aString,
                               ULong             aDepth,
                               qcComponentInfo * aComInfo )
{
    ULong   sDepth = 0;
    UInt    i      = 0;

    if ( aComInfo->count > 0 )
    {
        for ( sDepth = 0; sDepth < aDepth; sDepth++ )
        {
            iduVarStringAppend( aString, " " );
        }

        iduVarStringAppendFormat( aString,
                                  "[ RESULT CACHE REF: " );
        for ( i = 0; i < aComInfo->count - 1; i++ )
        {
            iduVarStringAppendFormat( aString,
                                      "%"ID_INT32_FMT",", aComInfo->components[i] );
        }

        iduVarStringAppendFormat( aString,
                                  "%"ID_INT32_FMT" ]\n", aComInfo->components[i] );
    }
    else
    {
        /* Nothing to do */
    }
}

void qmn::printResultCacheInfo( iduVarString    * aString,
                                ULong             aDepth,
                                qmnDisplay        aMode,
                                idBool            aIsInit,
                                qmndResultCache * aResultData )
{
    ULong         i = 0;
    const SChar * sString[3] = {"INIT", "HIT", "MISS"};
    SChar       * sStatus    = NULL;

    if ( aIsInit == ID_TRUE )
    {
        IDE_TEST_CONT( aResultData->memory == NULL, normal_exit );
    }
    else
    {
        /* Nothing to do */
    }

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString, " " );
    }

    if ( aMode == QMN_DISPLAY_ALL )
    {
        if ( aIsInit == ID_TRUE )
        {
            sStatus = (SChar *)sString[aResultData->status];
            if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
            {
                iduVarStringAppendFormat( aString,
                                          "[ RESULT CACHE HIT: %"ID_INT32_FMT", "
                                          "MISS: %"ID_INT32_FMT", "
                                          "SIZE: %"ID_INT64_FMT", "
                                          "STATUS: %s ]\n",
                                          aResultData->hitCount,
                                          aResultData->missCount,
                                          aResultData->memory->getSize(),
                                          sStatus );
            }
            else
            {
                iduVarStringAppendFormat( aString,
                                          "[ RESULT CACHE HIT: %"ID_INT32_FMT", "
                                          "MISS: %"ID_INT32_FMT", "
                                          "SIZE: BLOCKED, "
                                          "STATUS: %s ]\n",
                                          aResultData->hitCount,
                                          aResultData->missCount,
                                          sStatus );
            }
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "[ RESULT CACHE HIT: 0, "
                                      "MISS: 0, "
                                      "SIZE: 0, "
                                      "STATUS: INIT ]\n" );
        }
    }
    else
    {
        iduVarStringAppendFormat( aString,
                                  "[ RESULT CACHE HIT: ??, "
                                  "MISS: ??, "
                                  "SIZE: ??, "
                                  "STATUS: ?? ]\n" );
    }

    IDE_EXCEPTION_CONT( normal_exit );
}

void qmn::setDisplayInfo( qmsTableRef      * aTableRef,
                          qmsNamePosition  * aTableOwnerName,
                          qmsNamePosition  * aTableName,
                          qmsNamePosition  * aAliasName )
{
    qcNamePosition     sTableOwnerNamePos;
    qcNamePosition     sTableNamePos;
    qcNamePosition     sAliasNamePos;

    sTableOwnerNamePos = aTableRef->userName;
    sTableNamePos      = aTableRef->tableName;
    sAliasNamePos      = aTableRef->aliasName;

    //table owner name 세팅
    if ( ( aTableRef->tableInfo->tableType == QCM_FIXED_TABLE ) ||
         ( aTableRef->tableInfo->tableType == QCM_DUMP_TABLE ) ||
         ( aTableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW ) )
    {
        // performance view는 tableInfo가 없다.
        aTableOwnerName->name = NULL;
        aTableOwnerName->size = QC_POS_EMPTY_SIZE;
    }
    else
    {
        aTableOwnerName->name = sTableOwnerNamePos.stmtText + sTableOwnerNamePos.offset;
        aTableOwnerName->size = sTableOwnerNamePos.size;
    }

    if ( ( sTableNamePos.stmtText != NULL ) && ( sTableNamePos.size > 0 ) )
    {
        //table name 세팅
        aTableName->name = sTableNamePos.stmtText + sTableNamePos.offset;
        aTableName->size = sTableNamePos.size;
    }
    else
    {
        aTableName->name = NULL;
        aTableName->size = QC_POS_EMPTY_SIZE;
    }


    //table name 과 alias name이 같은 경우
    if ( sTableNamePos.offset == sAliasNamePos.offset )
    {
        aAliasName->name = NULL;
        aAliasName->size = QC_POS_EMPTY_SIZE;
    }
    else
    {
        aAliasName->name = sAliasNamePos.stmtText + sAliasNamePos.offset;
        aAliasName->size = sAliasNamePos.size;
    }
}

