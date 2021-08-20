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
 * Description : SDSE(SharD SElect) Node
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <cm.h>
#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qmoUtil.h>
#include <qmnShardSelect.h>
#include <smi.h>
#include <qmxShard.h>

extern mtdModule mtdNull; /* TASK-7219 */

IDE_RC qmnSDSE::init( qcTemplate * aTemplate,
                      qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description : SDSE 노드의 초기화
 *
 * Implementation : 최초 초기화가 되지 않은 경우 최초 초기화 수행
 *
 ***********************************************************************/

    sdiClientInfo * sClientInfo = NULL;
    qmncSDSE      * sCodePlan = NULL;
    qmndSDSE      * sDataPlan = NULL;
    idBool          sJudge = ID_TRUE;

    //-------------------------------
    // 적합성 검사
    //-------------------------------

    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aPlan     != NULL );

    //-------------------------------
    // 기본 초기화
    //-------------------------------

    sCodePlan = (qmncSDSE*)aPlan;
    sDataPlan = (qmndSDSE*)(aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];

    // First initialization
    if ( ( *sDataPlan->flag & QMND_SDSE_INIT_DONE_MASK ) == QMND_SDSE_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //-------------------------------
    // 재수행을 위한 초기화
    //-------------------------------

    sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;

    sdi::setDataNodePrepared( sClientInfo, sDataPlan->mDataInfo );

    //-------------------------------
    // doIt함수 결정을 위한 Constant filter 의 judgement
    //-------------------------------
    if ( sCodePlan->constantFilter != NULL )
    {
        IDE_TEST( qtc::judge( &sJudge,
                              sCodePlan->constantFilter,
                              aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( sJudge == ID_TRUE )
    {
        //------------------------------------------------
        // 수행 함수 결정
        //------------------------------------------------
        sDataPlan->doIt = qmnSDSE::doItFirst;
        *sDataPlan->flag &= ~QMND_SDSE_ALL_FALSE_MASK;
        *sDataPlan->flag |=  QMND_SDSE_ALL_FALSE_FALSE;
    }
    else
    {
        sDataPlan->doIt = qmnSDSE::doItAllFalse;
        *sDataPlan->flag &= ~QMND_SDSE_ALL_FALSE_MASK;
        *sDataPlan->flag |= QMND_SDSE_ALL_FALSE_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDSE::firstInit( qcTemplate * aTemplate,
                           qmncSDSE   * aCodePlan,
                           qmndSDSE   * aDataPlan )
{
/***********************************************************************
 *
 * Description : Data 영역에 대한 할당
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiClientInfo    * sClientInfo = NULL;
    sdiDataNode        sDataNodeArg;
    sdiBindParam     * sBindParams = NULL;
    UShort             sTupleID;
    UInt               i;
    UInt               sLobBindCount = 0;

    /* TASK-7219 */
    mtcColumn * sColumn = NULL;
    UInt        sCount  = 0;

    /* BUG-49154 */
    UInt        sOffset = 0;

    // Tuple 위치의 결정
    sTupleID = aCodePlan->tupleRowID;
    aDataPlan->plan.myTuple = &aTemplate->tmplate.rows[sTupleID];

    // BUGBUG
    aDataPlan->plan.myTuple->lflag &= ~MTC_TUPLE_STORAGE_MASK;
    aDataPlan->plan.myTuple->lflag |=  MTC_TUPLE_STORAGE_DISK;

    aDataPlan->nullRow        = NULL;
    aDataPlan->mCurrScanNode  = 0;
    aDataPlan->mScanDoneCount = 0;

    //-------------------------------
    // 수행노드 초기화
    //-------------------------------

    IDE_TEST_RAISE( aTemplate->shardExecData.execInfo == NULL,
                    ERR_NO_SHARD_INFO );

    aDataPlan->mDataInfo = ((sdiDataNodes*)aTemplate->shardExecData.execInfo)
        + aCodePlan->shardDataIndex;

    // shard linker 검사 & 초기화
    IDE_TEST( sdi::checkShardLinker( aTemplate->stmt ) != IDE_SUCCESS );

    //-------------------------------
    // shard 수행을 위한 준비
    //-------------------------------

    sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;

    if ( aDataPlan->mDataInfo->mInitialized == ID_FALSE )
    {
        idlOS::memset( &sDataNodeArg, 0x00, ID_SIZEOF(sdiDataNode) );

        // data를 얻어오기 위한(tuple을 위한) buffer 공간 할당
        sDataNodeArg.mBufferLength = aDataPlan->plan.myTuple->rowOffset;
        for ( i = 0; i < SDI_NODE_MAX_COUNT; i++ )
        {
            sDataNodeArg.mBuffer[i] = (void*)( aTemplate->shardExecData.data + aCodePlan->mBuffer[i] );
            // 초기화
            idlOS::memset( sDataNodeArg.mBuffer[i], 0x00, sDataNodeArg.mBufferLength );
        }

        sDataNodeArg.mOffset = (UInt*)( aTemplate->shardExecData.data + aCodePlan->mOffset );
        sDataNodeArg.mMaxByteSize =
            (UInt*)( aTemplate->shardExecData.data + aCodePlan->mMaxByteSize );

        sDataNodeArg.mBindParamCount = aCodePlan->mShardParamCount;
        sDataNodeArg.mBindParams = (sdiBindParam*)
            ( aTemplate->shardExecData.data + aCodePlan->mBindParam );

        /* PROJ-2728 Sharding LOB */
        sDataNodeArg.mOutBindParams = (sdiOutBindParam*)
            ( aTemplate->shardExecData.data + aCodePlan->mOutBindParam );
        idlOS::memset( sDataNodeArg.mOutBindParams, 0x00,
                ID_SIZEOF(sdiOutBindParam) * aCodePlan->mShardParamCount );

        /* TASK-7219 Non-shard DML */
        sDataNodeArg.mOutRefBindData = ( void* )
            ( aTemplate->shardExecData.data + aCodePlan->mOutRefBindData );

        /* TASK-7219 */
        for ( i = 0, sColumn = aDataPlan->plan.myTuple->columns;
              i < aDataPlan->plan.myTuple->columnCount;
              i++, sColumn++ )
        {
            if ( ( sColumn->flag & MTC_COLUMN_NULL_TYPE_MASK ) == MTC_COLUMN_NULL_TYPE_TRUE )
            {
                /* Nothing to do */
            }
            else
            {
                /* BUG-49154 */
                sOffset = idlOS::align( sOffset,
                                        sColumn->module->align );

                sDataNodeArg.mOffset[ sCount ]      = sOffset;
                sDataNodeArg.mMaxByteSize[ sCount ] = sColumn->column.size;

                sOffset += sColumn->column.size;

                sCount++;
            }
        }

        if ( sCount == 0 )
        {
            sDataNodeArg.mOffset[ 0 ]      = 0;
            sDataNodeArg.mMaxByteSize[ 0 ] = mtdNull.actualSize( NULL, NULL );

            sCount = 1;
        }
        else
        {
            /* Nothing to do */
        }

        sDataNodeArg.mColumnCount = sCount;

        IDE_TEST( setParamInfo( aTemplate,
                                aCodePlan,
                                sDataNodeArg.mBindParams,
                                sDataNodeArg.mOutRefBindData,
                                &sLobBindCount )
                  != IDE_SUCCESS );

        sDataNodeArg.mRemoteStmt = NULL;

        sDataNodeArg.mSVPStep = SDI_SVP_STEP_DO_NOT_NEED_SAVEPOINT;

        /* BUG-49154 */
        if ( aCodePlan->mRowForTransformed != 0 )
        {
            sDataNodeArg.mRowForTransformed = (void*)( aTemplate->shardExecData.data
                                                       + aCodePlan->mRowForTransformed );

            IDE_TEST( makeNullColumn( aTemplate,
                                      aDataPlan,
                                      sDataNodeArg.mRowForTransformed )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sdi::initShardDataInfo( aTemplate,
                                          aCodePlan->mShardAnalysis,
                                          sClientInfo,
                                          aDataPlan->mDataInfo,
                                          & sDataNodeArg )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aCodePlan->mShardParamCount > 0 )
        {
            IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                          ID_SIZEOF(sdiBindParam) * aCodePlan->mShardParamCount,
                          (void**) & sBindParams )
                      != IDE_SUCCESS );

            IDE_TEST( setParamInfo( aTemplate,
                                    aCodePlan,
                                    sBindParams,
                                    aTemplate->shardExecData.data + aCodePlan->mOutRefBindData,
                                    &sLobBindCount )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( sdi::reuseShardDataInfo( aTemplate,
                                           sClientInfo,
                                           aDataPlan->mDataInfo,
                                           sBindParams,
                                           aCodePlan->mShardParamCount,
                                           SDI_SVP_STEP_DO_NOT_NEED_SAVEPOINT )
                  != IDE_SUCCESS );
    }

    aDataPlan->lobBindCount = sLobBindCount;

    *aDataPlan->flag &= ~QMND_SDSE_INIT_DONE_MASK;
    *aDataPlan->flag |=  QMND_SDSE_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_SHARD_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnSDSE::firstInit",
                                  "Shard Info is not found" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDSE::setParamInfo( qcTemplate   * aTemplate,
                              qmncSDSE     * aCodePlan,
                              sdiBindParam * aBindParams,
                              void         * aOutRefBindData,
                              UInt         * aLobBindCount )
{
    qciBindParamInfo * sAllParamInfo = NULL;
    qciBindParam     * sBindParam = NULL;

    /* TASK-7219 Non-shard DML */
    qcShardParamInfo * sBindParamInfo = NULL;

    UShort             i = 0;
    UInt               sLobBindCount = 0;
    mtdLobType       * sLobValue;

    /* TASK-7219 Non-shard DML */
    mtcTuple  * sTuple  = NULL;
    mtcColumn * sColumn = NULL;

    UInt        sOutRefBindDataOffset = 0;

    // PROJ-2653
    sAllParamInfo  = aTemplate->stmt->pBindParam;

    for ( i = 0; i < aCodePlan->mShardParamCount; i++ )
    {
        sBindParamInfo = aCodePlan->mShardParamInfo + i; /* TASK-7219 Non-shard DML */

        if ( sBindParamInfo->mIsOutRefColumnBind == ID_TRUE )
        {
            /* TASK-7219 Non-shard DML */
            sTuple =  & aTemplate->tmplate.rows[ sBindParamInfo->mOutRefTuple ];
            sColumn = sTuple->columns + sBindParamInfo->mOffset;

            aBindParams[i].mId        = i + 1;
            aBindParams[i].mInoutType = CMP_DB_PARAM_INPUT;
            aBindParams[i].mType      = sColumn->module->id;

            aBindParams[i].mData      =
                ( (UChar*)aOutRefBindData ) + sOutRefBindDataOffset;

            sOutRefBindDataOffset += sColumn->column.size;

            aBindParams[i].mDataSize  = sColumn->column.size;
            aBindParams[i].mPrecision = sColumn->precision;
            aBindParams[i].mScale     = sColumn->scale;

            /* BUG-46623 padding 변수를 0으로 초기화 해야 한다. */
            aBindParams[i].padding    = 0;

            if ( ( sColumn->module->id == MTD_BLOB_ID ) ||
                 ( sColumn->module->id == MTD_CLOB_ID ) ||
                 ( sColumn->module->id == MTD_BLOB_LOCATOR_ID ) ||
                 ( sColumn->module->id == MTD_CLOB_LOCATOR_ID ) )
            {
                IDE_RAISE( ERR_LOB_COLUMN_PUSHED_FORCE );
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            IDE_DASSERT( sBindParamInfo->mOffset < aTemplate->stmt->pBindParamCount );

            sBindParam = &sAllParamInfo[sBindParamInfo->mOffset].param;

            if ( ( sBindParam->inoutType == CMP_DB_PARAM_INPUT ) ||
                 ( sBindParam->inoutType == CMP_DB_PARAM_INPUT_OUTPUT ) )
            {
                IDE_DASSERT( sAllParamInfo[sBindParamInfo->mOffset].isParamInfoBound == ID_TRUE );
                IDE_DASSERT( sAllParamInfo[sBindParamInfo->mOffset].isParamDataBound == ID_TRUE );
            }
            else
            {
                // Nothing to do.
            }

            aBindParams[i].mId        = i + 1;
            aBindParams[i].mInoutType = sBindParam->inoutType;
            aBindParams[i].mType      = sBindParam->type;
            aBindParams[i].mData      = sBindParam->data;
            aBindParams[i].mDataSize  = sBindParam->dataSize;
            aBindParams[i].mPrecision = sBindParam->precision;
            aBindParams[i].mScale     = sBindParam->scale;

            /* BUG-46623 padding 변수를 0으로 초기화 해야 한다. */
            aBindParams[i].padding    = 0;

            /*
             * SELECT 구문의 LOB 타입 바인딩은 PSM 내부에서만 가능하다.
             * 단 SELECT 문은 매개변수(?)의 위치에 상관없이 locator 타입
             * 바인딩이 지원되지 않으므로 (0x2100C Conversion not applicable. 에러)
             * CHAR 또는 BINARY 타입으로 바인딩한다.
             *
             * CHAR/BINARY 타입은 바인딩 가능한 최대 크기 제약이 있다.
             * PSM 내부 LOB: LOB_OBJECT_BUFFER_SIZE 프로퍼티[32000, 104857600]
             * CHAR: 65,534 = MTD_CHAR_PRECISION_MAXIMUM
             * BINARY: 10,482,953(??) = LOB_OBJECT_BUFFER_SIZE 최대값
             */
            if ( ( sBindParam->type == MTD_BLOB_ID ) ||
                 ( sBindParam->type == MTD_CLOB_ID ) )
            {
                sLobBindCount++;

                sLobValue = (mtdLobType *) sBindParam->data;
                aBindParams[i].mData = &sLobValue->value;

                if ( sBindParam->type == MTD_CLOB_ID )
                {
                    aBindParams[i].mPrecision = IDL_MIN(
                        aBindParams[i].mPrecision, MTD_CHAR_PRECISION_MAXIMUM);
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else if ( ( sBindParam->type == MTD_BLOB_LOCATOR_ID ) ||
                      ( sBindParam->type == MTD_CLOB_LOCATOR_ID ) )
            {
                /* BUG-48181
                 * select의 경우,
                 *   * OUT 바인딩으로 값을 받아오는 것 자체가 무의미 하다(항상 NULL locator 반환됨).
                 *   * IN 바인딩을 위한 copy 역시 OUT 바인딩이 안 되기 때문에 불가능.
                 * 즉, addLobInfoForCopy, addLobInfoForOutBind가 필요없기 때문에
                 * sLobBindCount를 증가시키지 않아도 된다. */
                if ( sBindParam->inoutType == CMP_DB_PARAM_INPUT )
                {
                    aBindParams[i].mInoutType = CMP_DB_PARAM_OUTPUT;
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

    *aLobBindCount = sLobBindCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LOB_COLUMN_PUSHED_FORCE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnSDSE::setParamInfo",
                                  "LOB column was pushed force for shard view." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
        
}

IDE_RC qmnSDSE::doIt( qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * aFlag )
{
    qmndSDSE * sDataPlan = (qmndSDSE*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->doIt( aTemplate, aPlan, aFlag );
}

IDE_RC qmnSDSE::doItAllFalse( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description : Constant Filter 검사후에 결정되는 함수로 절대 만족하는
 *               Record가 존재하지 않는다.
 *
 * Implementation : 항상 record 없음을 리턴한다.
 *
 ***********************************************************************/

    qmncSDSE * sCodePlan = (qmncSDSE*)aPlan;
    qmndSDSE * sDataPlan = (qmndSDSE*)(aTemplate->tmplate.data + aPlan->offset);

    // 적합성 검사
    IDE_DASSERT( sCodePlan->constantFilter != NULL );
    IDE_DASSERT( ( *sDataPlan->flag & QMND_SDSE_ALL_FALSE_MASK ) == QMND_SDSE_ALL_FALSE_TRUE );

    // 데이터 없음을 Setting
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= QMC_ROW_DATA_NONE;

    return IDE_SUCCESS;
}

IDE_RC qmnSDSE::doItFirst( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description : data 영역에 대한 초기화를 수행하고
 *               data 를 가져오기 위한 함수를 호출한다.
 *
 * Implementation :
 *              - allocStmt
 *              - prepare
 *              - bindCol (PROJ-2638 에서는 제외)
 *              - execute
 *
 ***********************************************************************/

    qmncSDSE       * sCodePlan = (qmncSDSE *)aPlan;
    qmndSDSE       * sDataPlan = (qmndSDSE *)(aTemplate->tmplate.data + aPlan->offset);
    sdiClientInfo  * sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;

    // 비정상 종료 검사
    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics ) != IDE_SUCCESS );

    // DataPlan 초기화
    sDataPlan->mCurrScanNode  = 0;
    sDataPlan->mScanDoneCount = 0;

    if ( sDataPlan->lobBindCount > 0 &&
         sCodePlan->mShardParamCount > 0 )
    {
        IDE_TEST( setLobInfo( aTemplate,
                              sCodePlan )
                  != IDE_SUCCESS );
    }

    //-------------------------------
    // Transformed out ref column bind 
    //-------------------------------

    // TASK-7219 Non-shard DML
    IDE_TEST( setTransformedOutRefBindValue( aTemplate,
                                             sCodePlan )
              != IDE_SUCCESS );

    //-------------------------------
    // 수행노드 결정
    //-------------------------------

    IDE_TEST( sdi::decideShardDataInfo(
                  aTemplate,
                  &(aTemplate->tmplate.rows[aTemplate->tmplate.variableRow]),
                  sCodePlan->mShardAnalysis,
                  sClientInfo,
                  sDataPlan->mDataInfo,
                  sCodePlan->mQueryPos )
              != IDE_SUCCESS );

    /* PROJ-2733-DistTxInfo 분산정보를 설정하자. */ 
    sdi::calculateGCTxInfo( aTemplate,
                            sDataPlan->mDataInfo,
                            aTemplate->shardExecData.globalPSM,
                            sCodePlan->shardDataIndex );

    //-------------------------------
    // 수행
    //-------------------------------

    IDE_TEST( sdi::executeSelect( aTemplate->stmt,
                                  sClientInfo,
                                  sDataPlan->mDataInfo )
              != IDE_SUCCESS );

    IDE_TEST( doItNext( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDSE::doItNext( qcTemplate * aTemplate,
                          qmnPlan    * aPlan,
                          qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description : data 를 가져오는 함수를 수행한다.
 *
 *    특정 data node 의 buffer 가 먼저 비게될 경우를 감안하여,
 *    data node 를 한 번씩 돌아가면서 수행한다.
 *
 *    결과가 없는 data node 은 건너뛰며,
 *    모든 data node 의 doIt 결과가 QMC_ROW_DATA_NONE(no rows)이
 *    될 때 까지 수행한다.
 *
 * Implementation :
 *              - fetch
 *
 ***********************************************************************/

    qmncSDSE       * sCodePlan = (qmncSDSE*)aPlan;
    qmndSDSE       * sDataPlan = (qmndSDSE*)(aTemplate->tmplate.data + aPlan->offset);
    sdiClientInfo  * sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;
    sdiConnectInfo * sConnectInfo = NULL;
    sdiDataNode    * sDataNode = NULL;
    mtcColumn      * sColumn = NULL;
    idBool           sJudge = ID_FALSE;
    idBool           sExist = ID_FALSE;
    UInt             i;

    UInt             sRemoteStmtId;
    UInt             sMmSessId;
    UInt             sMmStmtId;
    UInt             sLocatorInfo = 0;
    UChar          * sRow = NULL;
    smLobLocator     sRemoteLobLocator;
    smLobLocator     sShardLobLocator;

    /* BUG-49154 */
    UChar          * sRemote = NULL;
    UInt             sCount  = 0;

    sMmSessId = qci::mSessionCallback.mGetSessionID(
            aTemplate->stmt->session->mMmSession );
    sMmStmtId = qci::mSessionCallback.mGetStmtId(
            QC_MM_STMT( aTemplate->stmt ) );

    //mmtCmsFetch.cpp:doFetch->qci::getFetchColumnInfo에서 설정되므로 여기에서 설정할 필요 없음
    //sLocatorInfo = MTC_LOB_LOCATOR_CLIENT_TRUE;

    while ( 1 )
    {
        if ( sDataPlan->mCurrScanNode == sClientInfo->mCount )
        {
            // 이전 doIt이 마지막 data node 에서 수행 되었다면,
            // 첫번째 data node 부터 다시 doIt하도록 한다.
            sDataPlan->mCurrScanNode = 0;
        }
        else
        {
            // Nothing to do.
        }

        sConnectInfo = &(sClientInfo->mConnectInfo[sDataPlan->mCurrScanNode]);
        sDataNode = &(sDataPlan->mDataInfo->mNodes[sDataPlan->mCurrScanNode]);

        // 이전 doIt의 결과가 없었던 data node 는 skip한다.
        if ( sDataNode->mState == SDI_NODE_STATE_EXECUTED )
        {
            sJudge = ID_FALSE;

            while ( sJudge == ID_FALSE )
            {
                // fetch client
                IDE_TEST( sdi::fetch( sConnectInfo, sDataNode, &sExist )
                          != IDE_SUCCESS );

                /* BUG-49154 */
                if ( sDataNode->mRowForTransformed != NULL )
                {
                    sRow    = (UChar*)sDataNode->mRowForTransformed;
                    sRemote = (UChar*)sDataNode->mBuffer[sDataPlan->mCurrScanNode];
                }
                else
                {
                    sRow = (UChar*)sDataNode->mBuffer[sDataPlan->mCurrScanNode];
                }

                // 잘못된 데이터가 fetch되는 경우를 방어한다.
                sColumn = sDataPlan->plan.myTuple->columns;
                for ( i = 0, sCount = 0; i < sDataPlan->plan.myTuple->columnCount; i++, sColumn++ )
                {
                    /* BUG-49154 */
                    if ( ( sColumn->flag & MTC_COLUMN_NULL_TYPE_MASK ) == MTC_COLUMN_NULL_TYPE_TRUE )
                    {
                        continue;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    if ( sRemote != NULL )
                    {
                        IDE_TEST_RAISE( sColumn->module->actualSize(
                                            sColumn,
                                            sRemote + sDataNode->mOffset[ sCount ] ) >
                                        sColumn->column.size,
                                        ERR_INVALID_DATA_FETCHED );

                        idlOS::memcpy( (SChar *)( sRow + sColumn->column.offset ),
                                       (SChar *)( sRemote + sDataNode->mOffset[ sCount ] ),
                                       sColumn->column.size );

                        sCount++;
                    }
                    else
                    {
                        IDE_TEST_RAISE( sColumn->module->actualSize(
                                            sColumn,
                                            sRow + sColumn->column.offset ) >
                                        sColumn->column.size,
                                        ERR_INVALID_DATA_FETCHED );
                    }

                    /* PROJ-2728 Sharding LOB */
                    if ( ( sExist == ID_TRUE ) &&
                         ( sColumn->module->id == MTD_BLOB_LOCATOR_ID ||
                           sColumn->module->id == MTD_CLOB_LOCATOR_ID ) )
                    {
                        sRemoteLobLocator = * (smLobLocator *) ( sRow + sColumn->column.offset );
                        sRemoteStmtId = sdi::getRemoteStmtId(sDataNode);

                        IDE_TEST( smiLob::openShardLobCursor(
                                    (QC_SMI_STMT(aTemplate->stmt))->getTrans(),
                                    sMmSessId,
                                    sMmStmtId, // mmcStatement ID
                                    sRemoteStmtId,
                                    sConnectInfo->mNodeId,
                                    sColumn->module->id,
                                    sRemoteLobLocator, 
                                    sLocatorInfo,
                                    SMI_LOB_TABLE_CURSOR_MODE,
                                    &sShardLobLocator )
                                  != IDE_SUCCESS );

                        * (smLobLocator *) ( sRow + sColumn->column.offset ) = sShardLobLocator;

                        IDE_TEST( aTemplate->cursorMgr->addOpenedLobCursor( sShardLobLocator )
                                  != IDE_SUCCESS );
                    }
                }

                //------------------------------
                // Data 존재 여부에 따른 처리
                //------------------------------

                if ( sExist == ID_TRUE )
                {
                    /* BUG-49154 */
                    if ( sDataNode->mRowForTransformed != NULL )
                    {
                        sDataPlan->plan.myTuple->row =
                            sDataNode->mRowForTransformed;
                    }
                    else
                    {
                        sDataPlan->plan.myTuple->row =
                            sDataNode->mBuffer[sDataPlan->mCurrScanNode];
                    }

                    // BUGBUG nullRID
                    SMI_MAKE_VIRTUAL_NULL_GRID( sDataPlan->plan.myTuple->rid );

                    sDataPlan->plan.myTuple->modify++;

                    if ( sCodePlan->filter != NULL )
                    {
                        IDE_TEST( qtc::judge( &sJudge,
                                              sCodePlan->filter,
                                              aTemplate )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        sJudge = ID_TRUE;
                    }

                    if ( sJudge == ID_TRUE )
                    {
                        if ( sCodePlan->subqueryFilter != NULL )
                        {
                            IDE_TEST( qtc::judge( &sJudge,
                                                  sCodePlan->subqueryFilter,
                                                  aTemplate )
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

                    if ( sJudge == ID_TRUE )
                    {
                        if ( sCodePlan->nnfFilter != NULL )
                        {
                            IDE_TEST( qtc::judge( &sJudge,
                                                  sCodePlan->nnfFilter,
                                                  aTemplate )
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

                    if ( sJudge == ID_TRUE )
                    {
                        *aFlag = QMC_ROW_DATA_EXIST;
                        sDataPlan->doIt = qmnSDSE::doItNext;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    /* BUG-49154 */
                    if ( sDataNode->mRowForTransformed != NULL )
                    {
                        sDataPlan->plan.myTuple->row =
                            sDataNode->mRowForTransformed;
                    }
                    else
                    {
                        sDataPlan->plan.myTuple->row =
                            sDataNode->mBuffer[sDataPlan->mCurrScanNode];
                    }

                    // a data node fetch complete
                    sDataNode->mState = SDI_NODE_STATE_FETCHED;
                    sDataPlan->mScanDoneCount++;
                    break;
                }
            }

            if ( sJudge == ID_TRUE )
            {
                IDE_DASSERT( *aFlag == QMC_ROW_DATA_EXIST );
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( sDataNode->mState < SDI_NODE_STATE_EXECUTED )
            {
                sDataPlan->mScanDoneCount++;
            }
            else
            {
                IDE_DASSERT( sDataNode->mState == SDI_NODE_STATE_FETCHED );
            }
        }

        if ( sDataPlan->mScanDoneCount == sClientInfo->mCount )
        {
            *aFlag = QMC_ROW_DATA_NONE;
            sDataPlan->doIt = qmnSDSE::doItFirst;
            break;
        }
        else
        {
            sDataPlan->mCurrScanNode++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_FETCHED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LENGTH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDSE::padNull( qcTemplate * aTemplate,
                         qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncSDSE * sCodePlan = (qmncSDSE*)aPlan;
    qmndSDSE * sDataPlan = (qmndSDSE*)(aTemplate->tmplate.data + aPlan->offset);

    if ( ( aTemplate->planFlag[sCodePlan->planID] & QMND_SDSE_INIT_DONE_MASK )
         == QMND_SDSE_INIT_DONE_FALSE )
    {
        // 초기화 되지 않은 경우 초기화 수행
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( ( sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK ) == QMN_PLAN_STORAGE_DISK )
    {
        //-----------------------------------
        // Disk Table인 경우
        //-----------------------------------

        // Record 저장을 위한 공간은 하나만 존재하며,
        // 이에 대한 pointer는 항상 유지되어야 한다.

        if ( sDataPlan->nullRow == NULL )
        {
            //-----------------------------------
            // Null Row를 가져온 적이 없는 경우
            //-----------------------------------

            // 적합성 검사
            IDE_DASSERT( sDataPlan->plan.myTuple->rowOffset > 0 );

            // Null Row를 위한 공간 할당
            IDE_TEST( aTemplate->stmt->qmxMem->cralloc( sDataPlan->plan.myTuple->rowOffset,
                                                        (void**) &sDataPlan->nullRow )
                      != IDE_SUCCESS );

            // PROJ-1705
            // 디스크테이블의 null row는 qp에서 생성/저장해두고 사용한다.
            IDE_TEST( qmn::makeNullRow( sDataPlan->plan.myTuple,
                                        sDataPlan->nullRow )
                      != IDE_SUCCESS );

            SMI_MAKE_VIRTUAL_NULL_GRID( sDataPlan->nullRID );
        }
        else
        {
            // 이미 Null Row를 가져왔음.
            // Nothing to do.
        }

        // Null Row 복사
        idlOS::memcpy( sDataPlan->plan.myTuple->row,
                       sDataPlan->nullRow,
                       sDataPlan->plan.myTuple->rowOffset );

        // Null RID의 복사
        idlOS::memcpy( &sDataPlan->plan.myTuple->rid,
                       &sDataPlan->nullRID,
                       ID_SIZEOF(scGRID) );
    }
    else
    {
        //-----------------------------------
        // Memory Table인 경우
        //-----------------------------------
        // data node 의 tuple은 항상 disk tuple이다.
        IDE_DASSERT( 1 );
    }

    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDSE::printPlan( qcTemplate   * aTemplate,
                           qmnPlan      * aPlan,
                           ULong          aDepth,
                           iduVarString * aString,
                           qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description : SDSE 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncSDSE * sCodePlan = (qmncSDSE*)aPlan;
    qmndSDSE * sDataPlan = (qmndSDSE*)(aTemplate->tmplate.data + aPlan->offset);
    sdiClientInfo * sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    //----------------------------
    // SDSE 노드 표시
    //----------------------------
    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        qmn::printSpaceDepth( aString, aDepth );
        iduVarStringAppend( aString, "SHARD-COORDINATOR [ " );
        iduVarStringAppendFormat( aString, "SELF: %"ID_INT32_FMT" ]\n",
                                  (SInt)sCodePlan->tupleRowID );
    }
    else
    {
        qmn::printSpaceDepth( aString, aDepth );
        iduVarStringAppend( aString, "SHARD-COORDINATOR\n" );
    }

    /* BUG-45899 */
    if ( sdi::isAnalysisInfoPrintable( aTemplate->stmt ) == ID_TRUE )
    {
        // non-shard query 에 한해 출력
        if ( ( sCodePlan->mQueryPos != NULL ) &&
             ( aTemplate->stmt->mShardPrintInfo.mQueryType == SDI_QUERY_TYPE_NONSHARD ) )
        {
            qmn::printSpaceDepth( aString, aDepth );
            iduVarStringAppend( aString, "[ DISTRIBUTION QUERY ]\n" );
            qmn::printSpaceDepth( aString, aDepth );
            iduVarStringAppendFormat( aString,
                                      "%.*s",
                                      sCodePlan->mQueryPos->size,
                                      sCodePlan->mQueryPos->stmtText + sCodePlan->mQueryPos->offset );
            iduVarStringAppend( aString, "\n" );
        }
    }

    //----------------------------
    // Predicate 정보의 상세 출력
    //----------------------------
    if ( ( QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE( aTemplate->stmt ) == 1 ) ||
         ( SDU_SHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE == 1 ) )
    {
        // Normal Filter 출력
        if ( sCodePlan->filter != NULL )
        {
            qmn::printSpaceDepth( aString, aDepth );
            iduVarStringAppend( aString, " [ FILTER ]\n" );
            IDE_TEST( qmoUtil::printPredInPlan( aTemplate,
                                                aString,
                                                aDepth + 1,
                                                sCodePlan->filter )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // Constant Filter
        if ( sCodePlan->constantFilter != NULL )
        {
            qmn::printSpaceDepth( aString, aDepth );
            iduVarStringAppend( aString, " [ CONSTANT FILTER ]\n" );
            IDE_TEST( qmoUtil::printPredInPlan( aTemplate,
                                                aString,
                                                aDepth + 1,
                                                sCodePlan->constantFilter )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // Subquery Filter
        if ( sCodePlan->subqueryFilter != NULL )
        {
            qmn::printSpaceDepth( aString, aDepth );
            iduVarStringAppend( aString, " [ SUBQUERY FILTER ]\n" );
            IDE_TEST( qmoUtil::printPredInPlan( aTemplate,
                                                aString,
                                                aDepth + 1,
                                                sCodePlan->subqueryFilter )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // NNF Filter
        if ( sCodePlan->nnfFilter != NULL )
        {
            qmn::printSpaceDepth( aString, aDepth );
            iduVarStringAppend( aString, " [ NOT-NORMAL-FORM FILTER ]\n" );
            IDE_TEST( qmoUtil::printPredInPlan( aTemplate,
                                                aString,
                                                aDepth + 1,
                                                sCodePlan->nnfFilter )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        if ( sClientInfo != NULL )
        {
            // 수행정보 출력
            IDE_DASSERT( QMND_SDSE_INIT_DONE_TRUE == QMND_SDEX_INIT_DONE_TRUE );

            IDE_TEST( qmnSDEX::printDataInfo( aTemplate,
                                              sClientInfo,
                                              sDataPlan->mDataInfo,
                                              aDepth + 1,
                                              aString,
                                              aMode,
                                              sDataPlan->flag )
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

    //----------------------------
    // Subquery 정보의 출력.
    //----------------------------
    // subquery는 constant filter, nnf filter, subquery filter에만 있다.
    // Constant Filter의 Subquery 정보 출력
    if ( sCodePlan->constantFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->constantFilter,
                                          aDepth,
                                          aString,
                                          aMode )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // Subquery Filter의 Subquery 정보 출력
    if ( sCodePlan->subqueryFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->subqueryFilter,
                                          aDepth,
                                          aString,
                                          aMode )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // NNF Filter의 Subquery 정보 출력
    if ( sCodePlan->nnfFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->nnfFilter,
                                          aDepth,
                                          aString,
                                          aMode )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------
    // Operator별 결과 정보 출력
    //----------------------------
    if ( QCU_TRCLOG_RESULT_DESC == 1 )
    {
        IDE_TEST( qmn::printResult( aTemplate,
                                    aDepth,
                                    aString,
                                    sCodePlan->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2728 Sharding LOB */
IDE_RC qmnSDSE::setLobInfo( qcTemplate   * aTemplate,
                            qmncSDSE     * aCodePlan )
{
    qciBindParamInfo  * sAllParamInfo  = NULL;
    qciBindParam      * sBindParam     = NULL;
    sdiOutBindParam   * sOutBindParams = NULL;
    mtdLobType        * sLobValue      = NULL;
    qcShardParamInfo  * sShardBindInfo = NULL;  /* TASK-7219 Non-shard DML */
    UShort              i;                   /* TASK-7219 */

    sAllParamInfo  = aTemplate->stmt->pBindParam;
    sOutBindParams = (sdiOutBindParam*) ( aTemplate->shardExecData.data + aCodePlan->mOutBindParam );

    for ( i = 0; i < aCodePlan->mShardParamCount; i++ )
    {
        sShardBindInfo = aCodePlan->mShardParamInfo + i; /* TASK-7219 Non-shard DML */

        IDE_DASSERT( sShardBindInfo->mOffset < aTemplate->stmt->pBindParamCount );

        sBindParam = &sAllParamInfo[sShardBindInfo->mOffset].param;

        if ( sBindParam->type == MTD_CLOB_ID ||
             sBindParam->type == MTD_BLOB_ID )
        {
            /* 실제로는 sdl::bindParam에서 
             * MTD_BLOB_ID -> MTD_BINARY_ID,
             * MTD_CLOB_ID -> MTD_VARCHAR_ID 로 바인딩된다 */
            sLobValue = (mtdLobType *) sBindParam->data;

            IDE_TEST_RAISE( sBindParam->type == MTD_CLOB_ID &&
                            sLobValue->length > MTD_CHAR_PRECISION_MAXIMUM,
                            ERR_CONVERSION_NOT_APPLICABLE );

            sOutBindParams[i].mIndicator = sLobValue->length;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDSE::setTransformedOutRefBindValue( qcTemplate * aTemplate,
                                               qmncSDSE   * aCodePlan )
{
    /* TASK-7219 Non-shard DML */
    UShort             i              = 0;
    qcShardParamInfo * sBindParamInfo = NULL;
    mtcTuple         * sTuple         = NULL;
    mtcColumn        * sColumn        = NULL;
    sdiBindParam     * sBindParams    = NULL;

    void             * sBindValue     = NULL;


    sBindParams = (sdiBindParam*)
        ( aTemplate->shardExecData.data + aCodePlan->mBindParam );

    for ( i = 0; i < aCodePlan->mShardParamCount; i++ )
    {
        sBindParamInfo = aCodePlan->mShardParamInfo + i;

        if ( sBindParamInfo->mIsOutRefColumnBind == ID_TRUE )
        {
            sTuple = & aTemplate->tmplate.rows[ sBindParamInfo->mOutRefTuple ];
            sColumn = sTuple->columns + sBindParamInfo->mOffset;

            /* Outer relation tuple read된 데이터 값을 가져온다. */
            sBindValue = (UChar*)mtc::value( sColumn,
                                             sTuple->row,
                                             MTD_OFFSET_USE );

            /* 가져온 값을 transformed bind data ptr위치에 복사한다. */
            idlOS::memcpy( sBindParams[i].mData,
                           sBindValue,
                           sBindParams[i].mDataSize );

        }
        else
        {
            /* Nothing to do. */
        }
    }

    return IDE_SUCCESS;
}

IDE_RC qmnSDSE::makeNullColumn( qcTemplate * aTemplate,
                                qmndSDSE   * aDataPlan,
                                void       * aRow )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort           sColumnCnt   = 0;
    mtcColumn      * sColumn      = NULL;
    UChar          * sColumnPtr   = NULL;
    smcTableHeader * sTableHeader = NULL;
    mtcTuple       * sTuple       = NULL;

    IDE_TEST_RAISE( aTemplate == NULL, ERR_NULL_TMPL );
    IDE_TEST_RAISE( aDataPlan == NULL, ERR_NULL_PLAN );

    sTuple = aDataPlan->plan.myTuple;

    for ( sColumnCnt = 0;
          sColumnCnt < sTuple->columnCount;
          sColumnCnt++ )
    {
        sColumn = &( sTuple->columns[ sColumnCnt ] );

        if ( ( sColumn->flag & MTC_COLUMN_NULL_TYPE_MASK )
             == MTC_COLUMN_NULL_TYPE_TRUE )
        {
            if ( ( sColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                 == SMI_COLUMN_COMPRESSION_TRUE )
            {
                sColumnPtr = (UChar *)( aRow ) + sColumn->column.offset;

                sTableHeader =
                    (smcTableHeader *)SMI_MISC_TABLE_HEADER( smiGetTable( sColumn->column.mDictionaryTableOID ) );

                IDE_DASSERT( sTableHeader->mNullOID != SM_NULL_OID );

                idlOS::memcpy( sColumnPtr,
                               &( sTableHeader->mNullOID ),
                               ID_SIZEOF( smOID ) );
            }
            else
            {
                sColumnPtr = (UChar *)mtc::value( sColumn,
                                                  aRow,
                                                  MTD_OFFSET_USE );

                sColumn->module->null( sColumn,
                                       sColumnPtr );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_TMPL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnSDSE::makeNullColumn",
                                  "template is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnSDSE::makeNullColumn",
                                  "datapaln is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
