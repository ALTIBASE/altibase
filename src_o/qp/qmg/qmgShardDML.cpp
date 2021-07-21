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
 * Description : ShardDML Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcg.h>
#include <qmo.h>
#include <qcgPlan.h>
#include <qmgShardDML.h>
#include <qmoOneNonPlan.h>

IDE_RC qmgShardDML::init( qcStatement      * aStatement,
                          qcNamePosition   * aShardQuery,
                          sdiAnalyzeInfo   * aShardAnalysis,
                          qcShardParamInfo * aShardParamInfo,  /* TASK-7219 Non-shard DML */
                          UShort             aShardParamCount,
                          qmgGraph        ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgShardDML Graph의 초기화
 *
 * Implementation :
 *
 *            - alloc qmgShardDML
 *            - init qmgShardDML, qmgGraph
 *
 ***********************************************************************/

    qmgSHARDDML    * sMyGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmgShardDML::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aShardQuery != NULL );

    //---------------------------------------------------
    // ShardDML Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgShardDML을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgSHARDDML ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_SHARD_DML;

    sMyGraph->graph.optimize = qmgShardDML::optimize;
    sMyGraph->graph.makePlan = qmgShardDML::makePlan;
    sMyGraph->graph.printGraph = qmgShardDML::printGraph;

    sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
    sMyGraph->graph.flag |=  QMG_GRAPH_TYPE_DISK;

    //---------------------------------------------------
    // ShardDML 고유 정보의 초기화
    //---------------------------------------------------

    sMyGraph->shardQuery.stmtText = aShardQuery->stmtText;
    sMyGraph->shardQuery.offset   = aShardQuery->offset;
    sMyGraph->shardQuery.size     = aShardQuery->size;

    sMyGraph->shardAnalysis = aShardAnalysis;
    sMyGraph->shardParamInfo = aShardParamInfo; /* TASK-7219 Non-shard DML */
    sMyGraph->shardParamCount = aShardParamCount;

    sMyGraph->flag = QMG_SHARDDML_FLAG_CLEAR;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardDML::optimize( qcStatement * aStatement,
                              qmgGraph    * aGraph )
{
/***********************************************************************
 *
 * Description : qmgShardDML 의 최적화
 *
 * Implementation :
 *
 *      - 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgSHARDDML  * sMyGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmgShardDML::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgSHARDDML *)aGraph;

    //---------------------------------------------------
    // 공통 비용 정보 설정 (recordSize, inputRecordCnt)
    //---------------------------------------------------

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt = 0;

    // recordSize
    sMyGraph->graph.costInfo.recordSize = 1;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = 1;

    // outputRecordCnt
    sMyGraph->graph.costInfo.outputRecordCnt = 0;

    // myCost
    sMyGraph->graph.costInfo.myAccessCost = 0;
    sMyGraph->graph.costInfo.myDiskCost = 0;
    sMyGraph->graph.costInfo.myAllCost = 0;

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost = 0;
    sMyGraph->graph.costInfo.totalDiskCost = 0;
    sMyGraph->graph.costInfo.totalAllCost = 0;

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;

    return IDE_SUCCESS;
}

IDE_RC qmgShardDML::makePlan( qcStatement    * aStatement,
                              const qmgGraph * aParent,
                              qmgGraph       * aGraph )
{
/***********************************************************************
 *
 *  Description : qmgShardDML 로 부터 Plan을 생성한다.
 *
 *  Implementation :
 *
 ************************************************************************/

    qmgSHARDDML * sMyGraph = NULL;
    qmnPlan     * sSDEX    = NULL;

    IDU_FIT_POINT_FATAL( "qmgShardDML::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph     != NULL );

    sMyGraph = (qmgSHARDDML*)aGraph;

    //---------------------------------------------------
    // Current CNF의 등록
    //---------------------------------------------------

    // PROJ-2179
    // UPDATE 구문의 경우 parent가 NULL일 수 있다.
    if ( aParent != NULL )
    {
        sMyGraph->graph.myPlan = aParent->myPlan;
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1071 Parallel Query
    sMyGraph->graph.flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    sMyGraph->graph.flag |=  QMG_PARALLEL_IMPOSSIBLE_TRUE;

    //-----------------------
    // make SDEX
    //-----------------------

    IDE_TEST( qmoOneNonPlan::initSDEX( aStatement,
                                       &sSDEX )
              != IDE_SUCCESS );
   
    IDE_TEST( qmoOneNonPlan::makeSDEX( aStatement,
                                       &(sMyGraph->shardQuery),
                                       sMyGraph->shardAnalysis,
                                       sMyGraph->shardParamInfo, /* TASK-7219 Non-shard DML */
                                       sMyGraph->shardParamCount,
                                       sSDEX )
              != IDE_SUCCESS );

    sMyGraph->graph.myPlan = sSDEX;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardDML::printGraph( qcStatement  * aStatement,
                                qmgGraph     * aGraph,
                                ULong          aDepth,
                                iduVarString * aString )
{
/***********************************************************************
 *
 * Description : Graph를 구성하는 공통 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgSHARDDML  * sMyGraph = NULL;
    UInt           i;

    IDU_FIT_POINT_FATAL( "qmgShardDML::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph     != NULL );
    IDE_FT_ASSERT( aString    != NULL );

    sMyGraph = (qmgSHARDDML*)aGraph;

    //-----------------------------------
    // Graph의 시작 출력
    //-----------------------------------

    if (aDepth == 0)
    {
        QMG_PRINT_LINE_FEED(i, aDepth, aString);
        iduVarStringAppend(aString,
                           "----------------------------------------------------------");
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // Graph 공통 정보의 출력
    //-----------------------------------

    IDE_TEST( qmg::printGraph( aStatement,
                               aGraph,
                               aDepth,
                               aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // shard 정보 출력
    //-----------------------------------

    IDE_TEST( printShardInfo( aStatement,
                              sMyGraph->shardAnalysis,
                              &(sMyGraph->shardQuery),
                              aDepth,
                              aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // Graph의 마지막 출력
    //-----------------------------------

    if (aDepth == 0)
    {
        QMG_PRINT_LINE_FEED(i, aDepth, aString);
        iduVarStringAppend(aString,
                           "----------------------------------------------------------\n\n");
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardDML::printShardInfo( qcStatement    * aStatement,
                                    sdiAnalyzeInfo * aAnalyzeInfo,
                                    qcNamePosition * aQuery,
                                    ULong            aDepth,
                                    iduVarString   * aString )
{
/***********************************************************************
 *
 * Description : Graph를 구성하는 공통 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiClientInfo    * sClientInfo;
    sdiConnectInfo   * sConnectInfo;
    sdiTableInfoList * sTableInfoList;
    UInt               i;

    /* PROJ-2646 New shard analyzer */
    UShort             sShardValuePtrIndex = 0;
    UShort             sSubValuePtrIndex = 0;
    UShort             sBindParamCount = 0;

    IDU_FIT_POINT_FATAL( "qmgShardDML::printShardInfo::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_FT_ASSERT( aStatement  != NULL );
    IDE_FT_ASSERT( aQuery      != NULL );
    IDE_FT_ASSERT( aString     != NULL );
    IDE_FT_ASSERT( aStatement->session != NULL );

    //-----------------------------------
    // 초기화
    //-----------------------------------

    // shard linker 검사 & 초기화
    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    //-----------------------------------
    // shard analysis 정보 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Shard Analysis Information ==" );

    if ( aAnalyzeInfo != NULL )
    {
        //----------------------------------
        // bind param ID
        //----------------------------------

        for ( sShardValuePtrIndex = 0;
              sShardValuePtrIndex < aAnalyzeInfo->mValuePtrCount;
              sShardValuePtrIndex++ )
        {
            if ( ( aAnalyzeInfo->mValuePtrArray[sShardValuePtrIndex]->mType == 1 ) || // const value
                 ( sdi::getSplitType( aAnalyzeInfo->mSplitMethod ) != SDI_SPLIT_TYPE_DIST ) ) // split clone, solo, none
            {
                // Nothing to do.
            }
            else
            {
                QMG_PRINT_LINE_FEED( i, aDepth, aString );
                iduVarStringAppendFormat(
                    aString, "BIND PARAMETER   : %"ID_UINT32_FMT,
                    (UInt)aAnalyzeInfo->mValuePtrArray[sShardValuePtrIndex]->mValue.mBindParamId + 1 );

                sBindParamCount++;
            }
        }

        if ( aAnalyzeInfo->mSubKeyExists == ID_TRUE )
        {
            for ( sSubValuePtrIndex = 0;
                  sSubValuePtrIndex < aAnalyzeInfo->mSubValuePtrCount;
                  sSubValuePtrIndex++ )
            {
                if ( ( aAnalyzeInfo->mSubValuePtrArray[sSubValuePtrIndex]->mType == 1 ) || // const value
                     ( sdi::getSplitType( aAnalyzeInfo->mSubSplitMethod ) != SDI_SPLIT_TYPE_DIST ) ) // split clone, solo, none
                {
                    // Nothing to do.
                }
                else
                {
                    QMG_PRINT_LINE_FEED( i, aDepth, aString );
                    iduVarStringAppendFormat(
                        aString, "BIND PARAMETER   : %"ID_UINT32_FMT,
                        (UInt)aAnalyzeInfo->mSubValuePtrArray[sSubValuePtrIndex]->mValue.mBindParamId + 1 );
                }
            }
        }
        else
        {
            // Nothing to do.
        }

        if ( sBindParamCount == 0 )
        {
            QMG_PRINT_LINE_FEED( i, aDepth, aString );
            iduVarStringAppend( aString, "BIND PARAMETER   : N/A" );
        }
        else
        {
            // Nothing to do.
        }

        //----------------------------------
        // default node
        //----------------------------------

        if ( aAnalyzeInfo->mDefaultNodeId != ID_UINT_MAX )
        {
            sConnectInfo = sdi::findConnect( sClientInfo,
                                             aAnalyzeInfo->mDefaultNodeId );
            IDE_FT_ASSERT( sConnectInfo != NULL );

            QMG_PRINT_LINE_FEED( i, aDepth, aString );
            iduVarStringAppend( aString, "DEFAULT DATA NODE: " );
            iduVarStringAppend( aString, sConnectInfo->mNodeName );
        }
        else
        {
            QMG_PRINT_LINE_FEED( i, aDepth, aString );
            iduVarStringAppend( aString, "DEFAULT DATA NODE: N/A" );
        }

        //----------------------------------
        // split method & range
        //----------------------------------

        IDE_TEST( printSplitInfo( aStatement,
                                  aAnalyzeInfo,
                                  aDepth,
                                  aString )
                  != IDE_SUCCESS );

        IDE_TEST( printRangeInfo( aStatement,
                                  aAnalyzeInfo,
                                  sClientInfo,
                                  aDepth,
                                  aString )
                  != IDE_SUCCESS );

        //----------------------------------
        // shard object name
        //----------------------------------

        for ( sTableInfoList = aAnalyzeInfo->mTableInfoList;
              sTableInfoList != NULL;
              sTableInfoList = sTableInfoList->mNext )
        {
            QMG_PRINT_LINE_FEED( i, aDepth, aString );
            iduVarStringAppendFormat( aString, "OBJECT NAME      : %s.%s (%c)",
                                      sTableInfoList->mTableInfo->mUserName,
                                      sTableInfoList->mTableInfo->mObjectName,
                                      sTableInfoList->mTableInfo->mObjectType );
        }
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // shard query 정보 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Shard Query Information ==" );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "%.*s",
                              aQuery->size,
                              aQuery->stmtText + aQuery->offset );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardDML::printSplitInfo(  qcStatement    * /*aStatement*/,
                                     sdiAnalyzeInfo * aAnalyzeInfo,
                                     ULong            aDepth,
                                     iduVarString   * aString )
{
    UInt i;

    IDU_FIT_POINT_FATAL( "qmgShardDML::printSplitInfo::__FT__" );

    IDE_FT_ASSERT( aAnalyzeInfo != NULL );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );

    switch ( aAnalyzeInfo->mSplitMethod )
    {
        case SDI_SPLIT_HASH:
            iduVarStringAppend( aString, "SPLIT METHOD     : HASH" );
            break;
        case SDI_SPLIT_RANGE:
            iduVarStringAppend( aString, "SPLIT METHOD     : RANGE" );
            break;
        case SDI_SPLIT_LIST:
            iduVarStringAppend( aString, "SPLIT METHOD     : LIST" );
            break;
        case SDI_SPLIT_CLONE:
            iduVarStringAppend( aString, "SPLIT METHOD     : CLONE" );
            break;
        case SDI_SPLIT_SOLO:
            iduVarStringAppend( aString, "SPLIT METHOD     : SOLO" );
            break;
        case SDI_SPLIT_NODES:
        case SDI_SPLIT_NONE:
            iduVarStringAppend( aString, "SPLIT METHOD     : N/A" );
            break;
        default:
            IDE_FT_ASSERT(0);
            break;
    }

    if ( aAnalyzeInfo->mSubKeyExists == ID_TRUE )
    {
        switch ( aAnalyzeInfo->mSubSplitMethod )
        {
            case SDI_SPLIT_HASH:
                iduVarStringAppend( aString, ", HASH" );
                break;
            case SDI_SPLIT_RANGE:
                iduVarStringAppend( aString, ", RANGE" );
                break;
            case SDI_SPLIT_LIST:
                iduVarStringAppend( aString, ", LIST" );
                break;
            case SDI_SPLIT_CLONE:
            case SDI_SPLIT_SOLO:
            default:
                IDE_DASSERT(0);
                break;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}


IDE_RC qmgShardDML::printRangeInfo( qcStatement    * /*aStatement*/,
                                    sdiAnalyzeInfo * aAnalyzeInfo,
                                    sdiClientInfo  * aClientInfo,
                                    ULong            aDepth,
                                    iduVarString   * aString )
{
    sdiConnectInfo * sConnectInfo;
    sdiRangeInfo   * sRangeInfo;
    UInt             i;
    UInt             j;

    IDU_FIT_POINT_FATAL( "qmgShardDML::printShardRangeInfo::__FT__" );

    IDE_FT_ASSERT( aAnalyzeInfo != NULL );

    sRangeInfo = &(aAnalyzeInfo->mRangeInfo);

    for ( j = 0; j < sRangeInfo->mCount; j++ )
    {
        sConnectInfo = sdi::findConnect( aClientInfo,
                                         sRangeInfo->mRanges[j].mNodeId );
        IDE_FT_ASSERT( sConnectInfo != NULL );

        QMG_PRINT_LINE_FEED( i, aDepth, aString );

        switch ( aAnalyzeInfo->mSplitMethod )
        {
            case SDI_SPLIT_HASH :
                iduVarStringAppendFormat(
                    aString, "  %s : <%"ID_UINT32_FMT,  /* BUG-46288 */
                    sConnectInfo->mNodeName,
                    (UInt)sRangeInfo->mRanges[j].mValue.mHashMax );
                break;
            case SDI_SPLIT_RANGE :
                if ( aAnalyzeInfo->mKeyDataType == MTD_SMALLINT_ID )
                {
                    iduVarStringAppendFormat(
                        aString, "  %s : <%"ID_INT32_FMT,
                        sConnectInfo->mNodeName,
                        (SInt)sRangeInfo->mRanges[j].mValue.mSmallintMax );
                }
                else if ( aAnalyzeInfo->mKeyDataType == MTD_INTEGER_ID )
                {
                    iduVarStringAppendFormat(
                        aString, "  %s : <%"ID_INT32_FMT,
                        sConnectInfo->mNodeName,
                        sRangeInfo->mRanges[j].mValue.mIntegerMax );
                }
                else if ( aAnalyzeInfo->mKeyDataType == MTD_BIGINT_ID )
                {
                    iduVarStringAppendFormat(
                        aString, "  %s : <%"ID_INT64_FMT,
                        sConnectInfo->mNodeName,
                        sRangeInfo->mRanges[j].mValue.mBigintMax );
                }
                else if ( ( aAnalyzeInfo->mKeyDataType == MTD_CHAR_ID ) ||
                          ( aAnalyzeInfo->mKeyDataType == MTD_VARCHAR_ID ) )
                {
                    iduVarStringAppendFormat(
                        aString, "  %s : <'%.*s'",
                        sConnectInfo->mNodeName,
                        sRangeInfo->mRanges[j].mValue.mCharMax.length,
                        sRangeInfo->mRanges[j].mValue.mCharMax.value );
                }
                else
                {
                    IDE_FT_ASSERT(0);
                }
                break;
            case SDI_SPLIT_LIST :
                if ( aAnalyzeInfo->mKeyDataType == MTD_SMALLINT_ID )
                {
                    iduVarStringAppendFormat(
                        aString, "  %s : =%"ID_INT32_FMT,
                        sConnectInfo->mNodeName,
                        (SInt)sRangeInfo->mRanges[j].mValue.mSmallintMax );
                }
                else if ( aAnalyzeInfo->mKeyDataType == MTD_INTEGER_ID )
                {
                    iduVarStringAppendFormat(
                        aString, "  %s : =%"ID_INT32_FMT,
                        sConnectInfo->mNodeName,
                        sRangeInfo->mRanges[j].mValue.mIntegerMax );
                }
                else if ( aAnalyzeInfo->mKeyDataType == MTD_BIGINT_ID )
                {
                    iduVarStringAppendFormat(
                        aString, "  %s : =%"ID_INT64_FMT,
                        sConnectInfo->mNodeName,
                        sRangeInfo->mRanges[j].mValue.mBigintMax );
                }
                else if ( ( aAnalyzeInfo->mKeyDataType == MTD_CHAR_ID ) ||
                          ( aAnalyzeInfo->mKeyDataType == MTD_VARCHAR_ID ) )
                {
                    iduVarStringAppendFormat(
                        aString, "  %s : ='%.*s'",
                        sConnectInfo->mNodeName,
                        sRangeInfo->mRanges[j].mValue.mCharMax.length,
                        sRangeInfo->mRanges[j].mValue.mCharMax.value );
                }
                else
                {
                    IDE_FT_ASSERT(0);
                }
                break;
            case SDI_SPLIT_CLONE:
            case SDI_SPLIT_SOLO:
                iduVarStringAppendFormat(
                    aString, "  %s",
                    sConnectInfo->mNodeName );
                break;
            default:
                IDE_DASSERT(0);
                break;
        }

        if ( aAnalyzeInfo->mSubKeyExists == ID_TRUE )
        {
            switch ( aAnalyzeInfo->mSubSplitMethod )
            {
                case SDI_SPLIT_HASH:
                    iduVarStringAppendFormat(
                        aString, ", <%"ID_UINT32_FMT,  /* BUG-46288 */
                        (UInt)sRangeInfo->mRanges[j].mSubValue.mHashMax );
                    break;
                case SDI_SPLIT_RANGE:
                    if ( aAnalyzeInfo->mSubKeyDataType == MTD_SMALLINT_ID )
                    {
                        iduVarStringAppendFormat(
                            aString, ", <%"ID_INT32_FMT,
                            (SInt)sRangeInfo->mRanges[j].mSubValue.mSmallintMax );
                    }
                    else if ( aAnalyzeInfo->mSubKeyDataType == MTD_INTEGER_ID )
                    {
                        iduVarStringAppendFormat(
                            aString, ", <%"ID_INT32_FMT,
                            sRangeInfo->mRanges[j].mSubValue.mIntegerMax );
                    }
                    else if ( aAnalyzeInfo->mSubKeyDataType == MTD_BIGINT_ID )
                    {
                        iduVarStringAppendFormat(
                            aString, ", <%"ID_INT64_FMT,
                            sRangeInfo->mRanges[j].mSubValue.mBigintMax );
                    }
                    else if ( ( aAnalyzeInfo->mSubKeyDataType == MTD_CHAR_ID ) ||
                              ( aAnalyzeInfo->mSubKeyDataType == MTD_VARCHAR_ID ) )
                    {
                        iduVarStringAppendFormat(
                            aString, ", <'%.*s'",
                            sRangeInfo->mRanges[j].mSubValue.mCharMax.length,
                            sRangeInfo->mRanges[j].mSubValue.mCharMax.value );
                    }
                    else
                    {
                        IDE_DASSERT(0);
                    }
                    break;
                case SDI_SPLIT_LIST:
                    if ( aAnalyzeInfo->mSubKeyDataType == MTD_SMALLINT_ID )
                    {
                        iduVarStringAppendFormat(
                            aString, ", =%"ID_INT32_FMT,
                            (SInt)sRangeInfo->mRanges[j].mSubValue.mSmallintMax );
                    }
                    else if ( aAnalyzeInfo->mSubKeyDataType == MTD_INTEGER_ID )
                    {
                        iduVarStringAppendFormat(
                            aString, ", =%"ID_INT32_FMT,
                            sRangeInfo->mRanges[j].mSubValue.mIntegerMax );
                    }
                    else if ( aAnalyzeInfo->mSubKeyDataType == MTD_BIGINT_ID )
                    {
                        iduVarStringAppendFormat(
                            aString, ", =%"ID_INT64_FMT,
                            sRangeInfo->mRanges[j].mSubValue.mBigintMax );
                    }
                    else if ( ( aAnalyzeInfo->mSubKeyDataType == MTD_CHAR_ID ) ||
                              ( aAnalyzeInfo->mSubKeyDataType == MTD_VARCHAR_ID ) )
                    {
                        iduVarStringAppendFormat(
                            aString, ", ='%.*s'",
                            sRangeInfo->mRanges[j].mSubValue.mCharMax.length,
                            sRangeInfo->mRanges[j].mSubValue.mCharMax.value );
                    }
                    else
                    {
                        IDE_DASSERT(0);
                    }
                    break;
                case SDI_SPLIT_CLONE:
                case SDI_SPLIT_SOLO:
                    IDE_DASSERT(0);
                    break;
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
    
}
