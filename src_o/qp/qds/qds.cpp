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
 * $Id: qds.cpp 90270 2021-03-21 23:20:18Z bethy $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qds.h>
#include <qcm.h>
#include <qcmUser.h>
#include <qcg.h>
#include <qmv.h>
#include <smErrorCode.h>
#include <qdpPrivilege.h>
#include <qcuSqlSourceInfo.h>
#include <qdd.h>
#include <qdpDrop.h>
#include <qcmView.h>
#include <qdbComment.h>
#include <qcmAudit.h>
#include <qdx.h>
#include <qdn.h>
#include <qcmTableSpace.h>
#include <qdpRole.h>

/***********************************************************************
 * VALIDATE
 **********************************************************************/
// CREATE SEQUENCE
IDE_RC qds::validateCreate(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    CREATE SEQUENCE ... 의 validation  수행
 *
 * Implementation :
 *    1. ParseTree 의 cacheValue < 0 이면 에러 ( create 문장에서 명시하지
 *       않았으면 파싱할 때 디폴트 값으로 20 을 부여하게 됨 )
 *    2. IncrementValue 가 0 이면 에러
 *    3. MinValue >= MaxValue 이면 에러
 *    4. IncrementValue 가 MinValue, MaxValue 의 차이보다 크면 에러
 *    5. startValue < MinValue 이면 에러
 *    6. startValue > MaxValue 이면 에러
 *    7. 같은 이름의 시퀀스가 존재하는지 체크
 *    8. CreateSequence 권한이 있는지 체크
 *
 ***********************************************************************/

#define IDE_FN "qds::validateCreate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::validateCreate"));

    qdSequenceParseTree     * sParseTree;
    SLong                     sStartValue;
    SLong                     sMinValue;
    SLong                     sMaxValue;
    SLong                     sCacheValue;
    SLong                     sIncrementValue;
    SLong                     sABSofIncrementValue;
    SLong                     sDiffMinMaxValue;
    idBool                    sExist = ID_FALSE;
    qsOID                     sProcID;
    SChar                     sSeqTableNameStr[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qcNamePosition            sSeqTableName;
    qcuSqlSourceInfo          sqlInfo;
    UInt                      sUserID;

    /* TASK-7217 Sharded sequence */
    UInt                      sScale;
    SLong                     sABSofMax;
    SLong                     sMultiplier;

    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->sequenceName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            &(sParseTree->sequenceName) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    // check table exist.
    // To fix BUG-11086 duplicate sequence name 체크시
    // tableMeta에서 찾음
    if (gQcmSynonyms == NULL)
    {
        // in createdb phase -> there are no synonym meta table.
        IDE_TEST( qdbCommon::checkDuplicatedObject(
                      aStatement,
                      sParseTree->userName,
                      sParseTree->sequenceName,
                      &(sParseTree->userID) )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST(
            qcm::existObject(
                aStatement,
                ID_FALSE,
                sParseTree->userName,
                sParseTree->sequenceName,
                QS_OBJECT_MAX,
                &(sParseTree->userID),
                &sProcID,
                &sExist)
            != IDE_SUCCESS);
        IDE_TEST_RAISE(sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME);
    }

    // PROJ-2365 sequence table
    if ( sParseTree->enableSeqTable == ID_TRUE )
    {
        if ( sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN > QC_MAX_OBJECT_NAME_LEN )
        {
            sqlInfo.setSourceInfo( aStatement, &(sParseTree->sequenceName) );
            
            IDE_RAISE( ERR_SEQUENCE_NAME_LEN );
        }
        else
        {
            // Nothing to do.
        }

        // make sequence table name : SEQ1$SEQ
        QC_STR_COPY( sSeqTableNameStr, sParseTree->sequenceName );
        idlOS::strncat( sSeqTableNameStr,
                        QDS_SEQ_TABLE_SUFFIX_STR,
                        QDS_SEQ_TABLE_SUFFIX_LEN );
        
        sSeqTableName.stmtText = sSeqTableNameStr;
        sSeqTableName.offset   = 0;
        sSeqTableName.size     = sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN;

        IDE_TEST( qcm::existObject( aStatement,
                                    ID_FALSE,
                                    sParseTree->userName,
                                    sSeqTableName,
                                    QS_OBJECT_MAX,
                                    &sUserID,
                                    &sProcID,
                                    &sExist )
                  != IDE_SUCCESS );
        
        IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );
        
        *sParseTree->sequenceOptions->cycleOption &= ~SMI_SEQUENCE_TABLE_MASK;
        *sParseTree->sequenceOptions->cycleOption |= SMI_SEQUENCE_TABLE_TRUE;
    }
    else
    {
        *sParseTree->sequenceOptions->cycleOption &= ~SMI_SEQUENCE_TABLE_MASK;
        *sParseTree->sequenceOptions->cycleOption |= SMI_SEQUENCE_TABLE_FALSE;
    }

    // check grant
    IDE_TEST( qdpRole::checkDDLCreateSequencePriv( aStatement,
                                                   sParseTree->userID )
              != IDE_SUCCESS );

    sCacheValue = *(sParseTree->sequenceOptions->cacheValue);
    IDE_TEST_RAISE(sCacheValue < 0, ERR_CACHE_VALUE);

    sIncrementValue = *(sParseTree->sequenceOptions->incrementValue);
    IDE_TEST_RAISE(sIncrementValue == 0, ERR_INCREMENT_VALUE);

    if (sIncrementValue > 0)
    {
        sABSofIncrementValue = sIncrementValue;
    }
    else
    {
        sABSofIncrementValue = sIncrementValue * -1;
    }

    sMinValue = *(sParseTree->sequenceOptions->minValue);
    sMaxValue = *(sParseTree->sequenceOptions->maxValue);

    // BUG-26361
    IDE_TEST_RAISE(sMinValue >= sMaxValue, ERR_MIN_VALUE);

    /* TASK-7217 Sharded sequence */
    if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_LOCALITY_MASK) == QDS_SEQ_OPT_LOCALITY_SHARD )
    {
        IDE_TEST_RAISE(sMinValue < QDS_SHARD_SEQUENCE_MIN_VALUE,
                       ERR_OVERFLOW_SHARD_MINVALUE);
        IDE_TEST_RAISE(sMaxValue > QDS_SHARD_SEQUENCE_MAX_VALUE,
                       ERR_OVERFLOW_SHARD_MAXVALUE);

        if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_SCALE_FIXED_MASK) == QDS_SEQ_OPT_SCALE_FIXED_FALSE )
        {
            // SHARD VARIABLE인 경우 SCALE을 계산해서 설정한다.
            //   sABSofMax에 절대값이 더 큰 수를 구한다.
            //     1. Max, Min 모두 양수 => Max가 무조건 더 크다.
            //     2. Max 양수, Min 음수 => Max, Min * -1 중 큰 수가 더 큰 숫자이다.
            //     3. Max, Min 모두 음수 => Max보다 Min * -1이 더 크다.
            //     4. Max가 0            => Min은 무조건 음수이고, Min * -1이 더 크다.
            //     5. Min이 0            => Max는 무조건 양수이고, Max가 더 크다.
            sABSofMax = (sMaxValue > (sMinValue*-1)) ? sMaxValue:(sMinValue*-1);

            // sABSofMax보다 큰 가장 작은 10의 배수를 구한다.
            for ( sScale = 1, sMultiplier = 10; sScale <= SMI_SEQUENCE_SCALE_MASK && (sABSofMax >= sMultiplier) ; sScale++ )

            {
                sMultiplier = sMultiplier * 10;
            }

            *sParseTree->sequenceOptions->cycleOption &= ~SMI_SEQUENCE_SCALE_FIXED_MASK;
            *sParseTree->sequenceOptions->cycleOption |= SMI_SEQUENCE_SCALE_FIXED_FALSE;
        }
        else
        {
            // SHARD FIXED(default)인 경우 SCALE MAXIMUM을 설정한다.
            sScale = SMI_SEQUENCE_SCALE_MASK;

            *sParseTree->sequenceOptions->cycleOption &= ~SMI_SEQUENCE_SCALE_FIXED_MASK;
            *sParseTree->sequenceOptions->cycleOption |= SMI_SEQUENCE_SCALE_FIXED_TRUE;
        }

        *sParseTree->sequenceOptions->cycleOption &= ~SMI_SEQUENCE_SCALE_MASK;
        *sParseTree->sequenceOptions->cycleOption |= (sScale & SMI_SEQUENCE_SCALE_MASK);

        *sParseTree->sequenceOptions->cycleOption &= ~SMI_SEQUENCE_LOCALITY_MASK;
        *sParseTree->sequenceOptions->cycleOption |= SMI_SEQUENCE_LOCALITY_SHARD;
    }
    else
    {
        IDE_TEST_RAISE(sMinValue < QDS_SEQUENCE_MIN_VALUE,
                       ERR_OVERFLOW_MINVALUE);
        IDE_TEST_RAISE(sMaxValue > QDS_SEQUENCE_MAX_VALUE,
                       ERR_OVERFLOW_MAXVALUE);

        *sParseTree->sequenceOptions->cycleOption &= ~SMI_SEQUENCE_LOCALITY_MASK;
        *sParseTree->sequenceOptions->cycleOption |= SMI_SEQUENCE_LOCALITY_LOCAL;
    }

    sDiffMinMaxValue = sMaxValue - sMinValue;

    if (sDiffMinMaxValue < 0)
    {
        sDiffMinMaxValue = sDiffMinMaxValue * -1;
    }

    IDE_TEST_RAISE(sABSofIncrementValue > sDiffMinMaxValue,
                   ERR_INVALID_INCREMENT_VALUE);

    sStartValue = *(sParseTree->sequenceOptions->startValue);
    IDE_TEST_RAISE(sStartValue < sMinValue, ERR_START_LESS_MIN);
    IDE_TEST_RAISE(sStartValue > sMaxValue, ERR_START_MORE_MAX);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION(ERR_CACHE_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_CACHE_VALUE));
    }
    IDE_EXCEPTION(ERR_INCREMENT_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_INCREMENT_VALUE));
    }
    IDE_EXCEPTION(ERR_INVALID_INCREMENT_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_INVALID_INCREMENT_VALUE));
    }
    IDE_EXCEPTION(ERR_MIN_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_MIN_VALUE));
    }
    IDE_EXCEPTION(ERR_START_LESS_MIN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_START_LESS_MIN));
    }
    IDE_EXCEPTION(ERR_START_MORE_MAX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_START_MORE_MAX));
    }
    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_OVERFLOW_MAXVALUE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_OVERFLOW_MAXVALUE,
                                QDS_SEQUENCE_MAX_VALUE_STR));
    }
    IDE_EXCEPTION( ERR_OVERFLOW_MINVALUE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_OVERFLOW_MINVALUE,
                                QDS_SEQUENCE_MIN_VALUE_STR));
    }
    IDE_EXCEPTION( ERR_OVERFLOW_SHARD_MAXVALUE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_OVERFLOW_MAXVALUE,
                                QDS_SHARD_SEQUENCE_MAX_VALUE_STR));
    }
    IDE_EXCEPTION( ERR_OVERFLOW_SHARD_MINVALUE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_OVERFLOW_MINVALUE,
                                QDS_SHARD_SEQUENCE_MIN_VALUE_STR));
    }
    IDE_EXCEPTION( ERR_SEQUENCE_NAME_LEN )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                                sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// ALTER SEQUENCE OPTIONS
IDE_RC qds::validateAlterOptions(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER SEQUENCE ... 의 validation  수행
 *
 * Implementation :
 *    1. 시퀀스가 존재하는지 체크
 *    2. 변경하려는 시퀀스가 메타 시퀀스이면 에러
 *    3. AlterSequence 권한이 있는지 체크
 *    4. 생성되어 있는 시퀀스의 옵션 구하기 => smiTable::getSequence
 *    5. 시퀀스의 현재값 구하기 => smiTable::readSequence
 *    6. IncrementValue 가 명시되고 0 이면 에러
 *    7. cacheValue 가 명시되고 0 보다 작으면 에러
 *    8. cycleOption 부여
 *    3. MinValue 가 명시되고 시퀀스의 현재 값보다 크면 에러
 *    3. MaxValue 가 명시되고 시퀀스의 현재 값보다 작으면 에러
 *    4. IncrementValue 가 MinValue, MaxValue 의 차이보다 크면 에러
 *
 ***********************************************************************/

#define IDE_FN "qds::validateAlterOptions"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::validateAlterOptions"));

    qdSequenceParseTree     * sParseTree;
    qcmSequenceInfo           sSequenceInfo;
    SLong                     sStartValue;
    SLong                     sCurrIncrementValue;
    SLong                     sCurrMinValue;
    SLong                     sCurrMaxValue;
    SLong                     sCurrCacheValue;
    SLong                     sCurrValue;
    SLong                     sMinValue;
    SLong                     sMaxValue;
    SLong                     sCacheValue;
    UInt                      sOption;
    SLong                     sIncrementValue;
    SLong                     sABSofIncrementValue;
    SLong                     sDiffMinMaxValue;
    IDE_RC                    sRC;
    UInt                      sErrorCode;

    /* TASK-7217 Sharded sequence */
    qcuSqlSourceInfo          sqlInfo;
    SLong                     sABSofMax;
    SLong                     sMultiplier;
    UInt                      sScale;
    idBool                    sIsShardSequence = ID_FALSE;

    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    // if specified sequence doesn't exist, then error
    IDE_TEST(checkSequenceExist(
                 aStatement,
                 sParseTree->userName,
                 sParseTree->sequenceName,
                 &(sParseTree->userID),
                 &sSequenceInfo,
                 &(sParseTree->sequenceHandle))
             != IDE_SUCCESS);

    // fix BUG-14394
    sParseTree->sequenceID = sSequenceInfo.sequenceID;

    if ( QCG_GET_SESSION_USER_ID(aStatement) != QC_SYSTEM_USER_ID )
    {
        // sParseTree->userID is a owner of table
        IDE_TEST_RAISE(sParseTree->userID == QC_SYSTEM_USER_ID,
                       ERR_NOT_DROP_META);
    }
    else
    {
        if ( ( aStatement->session->mQPSpecific.mFlag
               & QC_SESSION_ALTER_META_MASK )
             == QC_SESSION_ALTER_META_DISABLE )
        {
            IDE_RAISE(ERR_NOT_DROP_META);
        }
    }

    // BUG-16980
    // check grant
    IDE_TEST( qdpRole::checkDDLAlterSequencePriv( aStatement,
                                                  &sSequenceInfo )
              != IDE_SUCCESS );

    IDE_TEST(smiTable::getSequence( sParseTree->sequenceHandle,
                                    &sStartValue,
                                    &sCurrIncrementValue,
                                    &sCurrCacheValue,
                                    &sCurrMaxValue,
                                    &sCurrMinValue,
                                    &sOption)
             != IDE_SUCCESS);

    sRC = smiTable::readSequence(QC_SMI_STMT( aStatement ),
                                 sParseTree->sequenceHandle,
                                 SMI_SEQUENCE_CURR,
                                 &sCurrValue,
                                 NULL);
    if (sRC != IDE_SUCCESS)
    {
        sErrorCode = ideGetErrorCode();

        if (sErrorCode == smERR_ABORT_SequenceNotInitialized)
        {
            IDE_CLEAR();
            sCurrValue = sStartValue;
        }
        else
        {
            IDE_RAISE(ERR_SM);
        }
    }

    // PROJ-2365 sequence table
    // sequence table이 만들어진 경우 option 변경은 column name 변경이 필요하다.
    // replication이 걸린 경우 column name을 변경할 수 없다.
    IDE_TEST_RAISE( ( sOption & SMI_SEQUENCE_TABLE_MASK ) == SMI_SEQUENCE_TABLE_TRUE,
                    ERR_CANNOT_ALTER_SEQ_TABLE );

    if (sParseTree->sequenceOptions->incrementValue == NULL)
    {
        IDU_LIMITPOINT("qds::validateAlter::malloc1");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), SLong, &(sParseTree->sequenceOptions->incrementValue))
                 != IDE_SUCCESS);

        *(sParseTree->sequenceOptions->incrementValue) = sCurrIncrementValue;
        sIncrementValue = sCurrIncrementValue;
    }
    else
    {
        sIncrementValue = *(sParseTree->sequenceOptions->incrementValue);
        IDE_TEST_RAISE(sIncrementValue == 0, ERR_INCREMENT_VALUE);
    }

    if (sIncrementValue > 0)
    {
        sABSofIncrementValue = sIncrementValue;
    }
    else
    {
        sABSofIncrementValue = sIncrementValue * -1;
    }

    if (sParseTree->sequenceOptions->cacheValue == NULL)
    {
        IDU_LIMITPOINT("qds::validateAlter::malloc2");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), SLong, &(sParseTree->sequenceOptions->cacheValue))
                 != IDE_SUCCESS);

        *(sParseTree->sequenceOptions->cacheValue) = sCurrCacheValue;
    }
    else
    {
        sCacheValue = *(sParseTree->sequenceOptions->cacheValue);
        IDE_TEST_RAISE(sCacheValue < 0, ERR_CACHE_VALUE);
    }

    if (sParseTree->sequenceOptions->cycleOption == NULL)
    {
        IDU_LIMITPOINT("qds::validateAlter::malloc3");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), UInt, &(sParseTree->sequenceOptions->cycleOption))
                 != IDE_SUCCESS);

        *(sParseTree->sequenceOptions->cycleOption) = sOption;
    }
    else
    {
        *(sParseTree->sequenceOptions->cycleOption) |=
            (sOption & ~SMI_SEQUENCE_CIRCULAR_MASK);
    }

    if (sParseTree->sequenceOptions->minValue == NULL)
    {
        IDU_LIMITPOINT("qds::validateAlter::malloc4");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), SLong, &(sParseTree->sequenceOptions->minValue))
                 != IDE_SUCCESS);

        if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_NOMIN_MASK)
             == QDS_SEQ_OPT_NOMIN_TRUE )
        {
            if (sIncrementValue > 0) // ascending sequence
            {
                *(sParseTree->sequenceOptions->minValue) = 1;
            }
            else // descending sequence
            {
                *(sParseTree->sequenceOptions->minValue) = QDS_SEQUENCE_MIN_VALUE;
            }
        }
        else
        {
            *(sParseTree->sequenceOptions->minValue) = sCurrMinValue;
        }
    }

    if (sParseTree->sequenceOptions->maxValue == NULL)
    {
        IDU_LIMITPOINT("qds::validateAlter::malloc5");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), SLong, &(sParseTree->sequenceOptions->maxValue))
                 != IDE_SUCCESS);

        if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_NOMAX_MASK)
             == QDS_SEQ_OPT_NOMAX_TRUE )
        {
            if (sIncrementValue > 0) // ascending sequence
            {
                *(sParseTree->sequenceOptions->maxValue) = QDS_SEQUENCE_MAX_VALUE;
            }
            else // descending sequence
            {
                *(sParseTree->sequenceOptions->maxValue) = -1;
            }
        }
        else
        {
            *(sParseTree->sequenceOptions->maxValue) = sCurrMaxValue;
        }
    }

    sMinValue = *(sParseTree->sequenceOptions->minValue);
    sMaxValue = *(sParseTree->sequenceOptions->maxValue);

    /* TASK-7217 Sharded sequence
       1. RESTART only
       2. Without RESTART option (RESTART (X) AND START WITH (X))
       3. RESTART (START) WITH N
       4. START WITH N only => PARSER ERROR */
    if ( sParseTree->sequenceOptions->startValue == NULL )
    {
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), SLong, &(sParseTree->sequenceOptions->startValue))
                 != IDE_SUCCESS);

        if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_RESTART_MASK) == QDS_SEQ_OPT_RESTART_TRUE )
        {
            // 1. RESTART(0) AND START WITH(X)
            if ( sIncrementValue > 0 )
            {
                sCurrValue = sMinValue;
            }
            else
            {
                sCurrValue = sMaxValue;
            }
            *sParseTree->sequenceOptions->startValue = sCurrValue;
        }
        else
        {
            // 2. Without RESTART option (RESTART (X) AND START WITH (X))
        }
    }
    else
    {
        // 3. RESTART (START) WITH (0)
        // 4. RESTART (X) AND START WITH (0) => PARSER ERROR
        sCurrValue  = *sParseTree->sequenceOptions->startValue;
    }

    IDE_TEST_RAISE(sCurrValue < *(sParseTree->sequenceOptions->minValue),
                   ERR_CURR_LESS_MIN);
    IDE_TEST_RAISE(sCurrValue > *(sParseTree->sequenceOptions->maxValue),
                   ERR_CURR_MORE_MAX);

    // BUG-26361
    IDE_TEST_RAISE(sMinValue >= sMaxValue, ERR_MIN_VALUE);
    
    if ( ( sMinValue == QDS_SEQUENCE_MIN_VALUE ) ||
         ( sMaxValue == QDS_SEQUENCE_MAX_VALUE ) )
    {
        sDiffMinMaxValue = QDS_SEQUENCE_MAX_VALUE;
    }
    else
    {
        sDiffMinMaxValue = sMaxValue - sMinValue;
    }

    if (sDiffMinMaxValue < 0)
    {
        sDiffMinMaxValue = sDiffMinMaxValue * -1;
    }

    IDE_TEST_RAISE(sABSofIncrementValue > sDiffMinMaxValue,
                   ERR_INVALID_INCREMENT_VALUE);

    /* TASK-7217 Sharded sequence
     * Sequence의 Scale을 구한다.
     * 1. SCALE VARIABLE
     * 2. SCALE FIXED => default(15) */
    if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_SCALE_FIXED_MASK) == QDS_SEQ_OPT_SCALE_FIXED_FALSE )
    {
        sABSofMax = (sMaxValue > (sMinValue*-1)) ? sMaxValue:(sMinValue*-1);

        // sABSofMax보다 큰 가장 작은 10의 배수를 구한다.
        for ( sScale = 1, sMultiplier = 10; sScale <= SMI_SEQUENCE_SCALE_MASK && (sABSofMax >= sMultiplier) ; sScale++ )
        {
            sMultiplier = sMultiplier * 10;
        }
    }
    else
    {
        sScale = SMI_SEQUENCE_SCALE_MASK;
    }

    /* TASK-7217 Sharded sequence */
    if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_LOCALITY_MASK) != 0 )
    {
        // Locality를 변경하는 경우
        *(sParseTree->sequenceOptions->cycleOption) &= ~QDS_SEQ_OPT_LOCALITY_MASK;
        *(sParseTree->sequenceOptions->cycleOption) |= (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_LOCALITY_MASK);

        *(sParseTree->sequenceOptions->cycleOption) &= ~QDS_SEQ_OPT_SCALE_FIXED_MASK;
        *(sParseTree->sequenceOptions->cycleOption) |= (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_SCALE_FIXED_MASK);

        if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_LOCALITY_MASK) != QDS_SEQ_OPT_LOCALITY_SHARD )
        {
            sIsShardSequence = ID_FALSE;
            // LOCAL로 변경되는 경우 scale을 제거한다.
            // 1. LOCAL           -> LOCAL
            // 2. SHARD FIXED     -> LOCAL
            // 3. SHARD VARIAABLE -> LOCAL
            *(sParseTree->sequenceOptions->cycleOption) &= ~SMI_SEQUENCE_SCALE_MASK;
        }
        else //if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_LOCALITY_MASK) == QDS_SEQ_OPT_LOCALITY_SHARD )
        {
            sIsShardSequence = ID_TRUE;

            if ( (sOption & QDS_SEQ_OPT_LOCALITY_MASK) == QDS_SEQ_OPT_LOCALITY_SHARD )
            {
                // SHARD <-> SHARD 전환되면 sScale이 같아야 한다.
                // 1. FIXED    -> FIXED
                // 2. FIXED    -> VARIABLE
                // 3. VARIABLE -> FIXED
                // 4. VARIABLE -> VARIABLE
                IDE_TEST_RAISE ( sScale != (sOption & SMI_SEQUENCE_SCALE_MASK), ERR_CANNOT_ALTER_SHARDED_SEQUENCE );
            }
            else
            {
                // LOCAL에서 SHARD로 변경하는 경우 새로 계산한 scale을 설정한다.
                *sParseTree->sequenceOptions->cycleOption &= ~SMI_SEQUENCE_SCALE_MASK;
                *sParseTree->sequenceOptions->cycleOption |= (sScale & SMI_SEQUENCE_SCALE_MASK);
            }
        }
    }
    else
    {
        // Locality 변경이 없는 경우
        // 1. 현재 SHARD VARIABLE인 경우
        // 2. 현재 SHARD FIXED인 경우
        // 3. 현재 SHARD가 아닌경우
        if ( (sOption & QDS_SEQ_OPT_LOCALITY_MASK) == QDS_SEQ_OPT_LOCALITY_SHARD )
        {
            sIsShardSequence = ID_TRUE;
            if ( (sOption & QDS_SEQ_OPT_SCALE_FIXED_MASK) == QDS_SEQ_OPT_SCALE_FIXED_FALSE )
            {
                // 1. 현재 SHARD VARIABLE인 경우
                IDE_TEST_RAISE ( sScale != (sOption & SMI_SEQUENCE_SCALE_MASK), ERR_CANNOT_ALTER_SHARDED_SEQUENCE );
            }
            else
            {
                // 2. 현재 SHARD FIXED인 경우
            }
        }
        else
        {
            // 3. 현재 SHARD가 아닌 경우
            sIsShardSequence = ID_FALSE;
        }
    }

    if ( sIsShardSequence == ID_FALSE )
    {
        // check local min/maxvalue
        IDE_TEST_RAISE(sMinValue < QDS_SEQUENCE_MIN_VALUE,
                       ERR_OVERFLOW_MINVALUE);
        IDE_TEST_RAISE(sMaxValue > QDS_SEQUENCE_MAX_VALUE,
                       ERR_OVERFLOW_MAXVALUE);
    }
    else
    {
        // check shard min/maxvalue
        IDE_TEST_RAISE(sMinValue < QDS_SHARD_SEQUENCE_MIN_VALUE,
                       ERR_OVERFLOW_SHARD_MINVALUE);
        IDE_TEST_RAISE(sMaxValue > QDS_SHARD_SEQUENCE_MAX_VALUE,
                       ERR_OVERFLOW_SHARD_MAXVALUE);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_CANNOT_ALTER_SEQ_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_CANNOT_ALTER_SEQ_TABLE));
    }
    IDE_EXCEPTION(ERR_INVALID_INCREMENT_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_INVALID_INCREMENT_VALUE));
    }
    IDE_EXCEPTION(ERR_CACHE_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_CACHE_VALUE));
    }
    IDE_EXCEPTION(ERR_INCREMENT_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_INCREMENT_VALUE));
    }
    IDE_EXCEPTION(ERR_MIN_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_MIN_VALUE));
    }
    IDE_EXCEPTION(ERR_CURR_LESS_MIN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_CURR_LESS_MIN));
    }
    IDE_EXCEPTION(ERR_CURR_MORE_MAX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_CURR_MORE_MAX));
    }
    IDE_EXCEPTION(ERR_NOT_DROP_META);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDD_NO_DROP_META_TABLE));
    }
    IDE_EXCEPTION( ERR_OVERFLOW_MAXVALUE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_OVERFLOW_MAXVALUE,
                                QDS_SEQUENCE_MAX_VALUE_STR));
    }
    IDE_EXCEPTION( ERR_OVERFLOW_MINVALUE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_OVERFLOW_MINVALUE,
                                QDS_SEQUENCE_MIN_VALUE_STR));
    }
    IDE_EXCEPTION( ERR_OVERFLOW_SHARD_MAXVALUE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_OVERFLOW_MAXVALUE,
                                QDS_SHARD_SEQUENCE_MAX_VALUE_STR));
    }
    IDE_EXCEPTION( ERR_OVERFLOW_SHARD_MINVALUE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_OVERFLOW_MINVALUE,
                                QDS_SHARD_SEQUENCE_MIN_VALUE_STR));
    }
    IDE_EXCEPTION( ERR_CANNOT_ALTER_SHARDED_SEQUENCE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDS_CANNOT_ALTER_SHARDED_SEQUENCE));
    }
    IDE_EXCEPTION(ERR_SM);
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// ALTER SEQUENCE ENABLE/DISABLE ROW SYNC
IDE_RC qds::validateAlterSeqTable(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER SEQUENCE ... 의 validation  수행
 *
 * Implementation :
 *    1. 시퀀스가 존재하는지 체크
 *    2. 변경하려는 시퀀스가 메타 시퀀스이면 에러
 *    3. AlterSequence 권한이 있는지 체크
 *
 ***********************************************************************/

#define IDE_FN "qds::validateAlterSeqTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::validateAlterSeqTable"));

    qdSequenceParseTree     * sParseTree;
    qcmSequenceInfo           sSequenceInfo;
    SLong                     sStartValue;
    SLong                     sCurrIncrementValue;
    SLong                     sCurrMinValue;
    SLong                     sCurrMaxValue;
    SLong                     sCurrCacheValue;
    SLong                     sCurrValue;
    UInt                      sOption;
    IDE_RC                    sRC;
    UInt                      sErrorCode;
    qcuSqlSourceInfo          sqlInfo;

    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    // if specified sequence doesn't exist, then error
    IDE_TEST(checkSequenceExist(
                 aStatement,
                 sParseTree->userName,
                 sParseTree->sequenceName,
                 &(sParseTree->userID),
                 &sSequenceInfo,
                 &(sParseTree->sequenceHandle))
             != IDE_SUCCESS);

    // fix BUG-14394
    sParseTree->sequenceID = sSequenceInfo.sequenceID;

    if ( QCG_GET_SESSION_USER_ID(aStatement) != QC_SYSTEM_USER_ID )
    {
        // sParseTree->userID is a owner of table
        IDE_TEST_RAISE(sParseTree->userID == QC_SYSTEM_USER_ID,
                       ERR_NOT_DROP_META);
    }
    else
    {
        IDE_TEST_RAISE( ( aStatement->session->mQPSpecific.mFlag
                          & QC_SESSION_ALTER_META_MASK )
                        == QC_SESSION_ALTER_META_DISABLE,
                        ERR_NOT_DROP_META );
    }

    // BUG-16980
    // check grant
    IDE_TEST( qdpRole::checkDDLAlterSequencePriv( aStatement,
                                                  &sSequenceInfo )
              != IDE_SUCCESS );

    IDE_TEST(smiTable::getSequence( sParseTree->sequenceHandle,
                                    &sStartValue,
                                    &sCurrIncrementValue,
                                    &sCurrCacheValue,
                                    &sCurrMaxValue,
                                    &sCurrMinValue,
                                    &sOption)
             != IDE_SUCCESS);

    sRC = smiTable::readSequence(QC_SMI_STMT( aStatement ),
                                 sParseTree->sequenceHandle,
                                 SMI_SEQUENCE_CURR,
                                 &sCurrValue,
                                 NULL);
    if (sRC != IDE_SUCCESS)
    {
        sErrorCode = ideGetErrorCode();

        if (sErrorCode == smERR_ABORT_SequenceNotInitialized)
        {
            IDE_CLEAR();
            sCurrValue = sStartValue;
        }
        else
        {
            IDE_RAISE(ERR_SM);
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN > QC_MAX_OBJECT_NAME_LEN )
    {
        sqlInfo.setSourceInfo( aStatement, &(sParseTree->sequenceName) );
        
        IDE_RAISE( ERR_SEQUENCE_NAME_LEN );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_NOT_DROP_META);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDD_NO_DROP_META_TABLE));
    }
    IDE_EXCEPTION( ERR_SEQUENCE_NAME_LEN )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                                sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_SM);
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qds::validateAlterFlushCache(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER SEQUENCE ... 의 validation  수행
 *
 * Implementation :
 *    1. 시퀀스가 존재하는지 체크
 *    2. 변경하려는 시퀀스가 메타 시퀀스이면 에러
 *    3. AlterSequence 권한이 있는지 체크
 *
 ***********************************************************************/

#define IDE_FN "qds::validateAlterFlushCache"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::validateAlterFlushCache"));

    qdSequenceParseTree     * sParseTree;
    qcmSequenceInfo           sSequenceInfo;
    SLong                     sStartValue;
    SLong                     sCurrIncrementValue;
    SLong                     sCurrMinValue;
    SLong                     sCurrMaxValue;
    SLong                     sCurrCacheValue;
    SLong                     sCurrValue;
    UInt                      sOption;
    IDE_RC                    sRC;
    UInt                      sErrorCode;
    qcuSqlSourceInfo          sqlInfo;

    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    // if specified sequence doesn't exist, then error
    IDE_TEST(checkSequenceExist(
                 aStatement,
                 sParseTree->userName,
                 sParseTree->sequenceName,
                 &(sParseTree->userID),
                 &sSequenceInfo,
                 &(sParseTree->sequenceHandle))
             != IDE_SUCCESS);

    // fix BUG-14394
    sParseTree->sequenceID = sSequenceInfo.sequenceID;

    if ( QCG_GET_SESSION_USER_ID(aStatement) != QC_SYSTEM_USER_ID )
    {
        // sParseTree->userID is a owner of table
        IDE_TEST_RAISE(sParseTree->userID == QC_SYSTEM_USER_ID,
                       ERR_NOT_DROP_META);
    }
    else
    {
        IDE_TEST_RAISE( ( aStatement->session->mQPSpecific.mFlag
                          & QC_SESSION_ALTER_META_MASK )
                        == QC_SESSION_ALTER_META_DISABLE,
                        ERR_NOT_DROP_META );
    }

    // BUG-16980
    // check grant
    IDE_TEST( qdpRole::checkDDLAlterSequencePriv( aStatement,
                                                  &sSequenceInfo )
              != IDE_SUCCESS );

    IDE_TEST(smiTable::getSequence( sParseTree->sequenceHandle,
                                    &sStartValue,
                                    &sCurrIncrementValue,
                                    &sCurrCacheValue,
                                    &sCurrMaxValue,
                                    &sCurrMinValue,
                                    &sOption)
             != IDE_SUCCESS);

    sRC = smiTable::readSequence(QC_SMI_STMT( aStatement ),
                                 sParseTree->sequenceHandle,
                                 SMI_SEQUENCE_CURR,
                                 &sCurrValue,
                                 NULL);
    if (sRC != IDE_SUCCESS)
    {
        sErrorCode = ideGetErrorCode();

        if (sErrorCode == smERR_ABORT_SequenceNotInitialized)
        {
            IDE_CLEAR();
            sCurrValue = sStartValue;
        }
        else
        {
            IDE_RAISE(ERR_SM);
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_NOT_DROP_META);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDD_NO_DROP_META_TABLE));
    }
    IDE_EXCEPTION(ERR_SM);
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/***********************************************************************
 * EXECUTE
 **********************************************************************/

IDE_RC qds::executeCreate(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    CREATE SEQUENCE ... 의 execution  수행
 *
 * Implementation :
 *    1. 시퀀스 ID 부여
 *    2. 시퀀스 이름 부여
 *    3. smiTable::createSequence
 *    4. SYS_TABLES_ 에 시퀀스 입력
 *    5. 메타 캐쉬 생성
 *
 ***********************************************************************/

#define IDE_FN "qds::executeCreate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::executeCreate"));

    qdSequenceParseTree     * sParseTree;
    const void              * sSequenceHandle;
    SChar                     sSqlStr[QCM_MAX_SQL_STR_LEN];
    SChar                     sSeqName [ QC_MAX_OBJECT_NAME_LEN + 1 ];
    smOID                     sSequenceOID;
    SChar                     sSeqTableNameStr[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qcNamePosition            sSeqTableName;
    qcmTableInfo            * sSeqTableInfo = NULL;
    UInt                      sSeqTblID;
    vSLong                    sRowCnt;

    SChar                     sTBSName[ SMI_MAX_TABLESPACE_NAME_LEN + 1 ];
    smiTableSpaceAttr         sTBSAttr;

    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST(*sParseTree->sequenceOptions->minValue >
             *sParseTree->sequenceOptions->maxValue);
    IDE_TEST(qcm::getNextTableID(aStatement, &sSeqTblID) != IDE_SUCCESS);

    QC_STR_COPY( sSeqName, sParseTree->sequenceName );

    IDE_TEST(smiTable::createSequence(
                 QC_SMI_STMT( aStatement ),
                 *sParseTree->sequenceOptions->startValue,
                 *sParseTree->sequenceOptions->incrementValue,
                 *sParseTree->sequenceOptions->cacheValue,
                 *sParseTree->sequenceOptions->maxValue,
                 *sParseTree->sequenceOptions->minValue,
                 *sParseTree->sequenceOptions->cycleOption,
                 & sSequenceHandle)
             != IDE_SUCCESS);

    sSequenceOID = smiGetTableId(sSequenceHandle);

    // PROJ-2365 sequence table
    if ( ( *sParseTree->sequenceOptions->cycleOption & SMI_SEQUENCE_TABLE_MASK )
         == SMI_SEQUENCE_TABLE_TRUE )
    {
        // make sequence table name : SEQ1$SEQ
        IDE_TEST_RAISE( sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN
                        > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SEQUENCE_NAME_LEN );
        
        QC_STR_COPY( sSeqTableNameStr, sParseTree->sequenceName );
        idlOS::strncat( sSeqTableNameStr,
                        QDS_SEQ_TABLE_SUFFIX_STR,
                        QDS_SEQ_TABLE_SUFFIX_LEN );
        
        sSeqTableName.stmtText = sSeqTableNameStr;
        sSeqTableName.offset   = 0;
        sSeqTableName.size     = sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN;
        
        // create sequence table
        IDE_TEST( createSequenceTable( aStatement,
                                       sSeqTableName,
                                       *sParseTree->sequenceOptions->startValue,
                                       *sParseTree->sequenceOptions->incrementValue,
                                       *sParseTree->sequenceOptions->minValue,
                                       *sParseTree->sequenceOptions->maxValue,
                                       *sParseTree->sequenceOptions->cacheValue,
                                       *sParseTree->sequenceOptions->cycleOption,
                                       *sParseTree->sequenceOptions->startValue,
                                       & sSeqTableInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
       DDL Statement Text의 로깅
    */
    if (QCU_DDL_SUPPLEMENTAL_LOG == 1)
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sSeqName )
                  != IDE_SUCCESS );
    }

    // PROJ-1502 PARTITIONED DISK TABLE
    // sequence는 partition될 수 없음.
    // PROJ-1407 Temporary Table
    // sequence는 temporary로 생성될 수 없음
    IDE_TEST( qcmTablespace::getTBSAttrByID( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                             & sTBSAttr )
              != IDE_SUCCESS );
    idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
    sTBSName[sTBSAttr.mNameLength] = '\0';
    
    idlOS::snprintf(sSqlStr, ID_SIZEOF(sSqlStr),
                    "INSERT INTO SYS_TABLES_ VALUES( "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "BIGINT'%"ID_INT64_FMT"', "
                    "INTEGER'0', "
                    "VARCHAR'%s', "
                    "'S', "
                    "INTEGER'0', "
                    "INTEGER'0', "
                    "BIGINT'0', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "VARCHAR'%s', "                  // 11 TBS_NAME
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "BIGINT'%"ID_INT64_FMT"', "
                    "BIGINT'%"ID_INT64_FMT"', "
                    "BIGINT'%"ID_INT64_FMT"', "
                    "BIGINT'%"ID_INT64_FMT"', "
                    "CHAR'F',"
                    "CHAR'N',"
                    "CHAR'N',"
                    "CHAR'W',"
                    "INTEGER'1'," // Parallel degree
                    "CHAR'Y',"    // USABLE
                    "INTEGER'0'," // SHARD_FLAG
                    "SYSDATE, SYSDATE )",
                    sParseTree->userID,
                    sSeqTblID,
                    (ULong)sSequenceOID,
                    sSeqName,
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    sTBSName,
                    QD_MEMORY_TABLE_DEFAULT_PCTFREE,
                    QD_MEMORY_TABLE_DEFAULT_PCTUSED,
                    QD_MEMORY_TABLE_DEFAULT_INITRANS,
                    QD_MEMORY_TABLE_DEFAULT_MAXTRANS,
                    (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS,
                    (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS,
                    (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS,
                    (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SEQUENCE_NAME_LEN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qds::executeCreate",
                                  "sequence name is too long" ) );
    }
    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if ( sSeqTableInfo != NULL )
    {
        (void) qcm::destroyQcmTableInfo( sSeqTableInfo );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qds::executeAlterOptions(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER SEQUENCE ... 의 execution  수행
 *
 * Implementation :
 *    1. 메타에서 시퀀스 핸들 구하기
 *    2. smiTable::alterSequence
 *
 ***********************************************************************/

#define IDE_FN "qds::executeAlterOptions"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::executeAlterOptions"));

    qdSequenceParseTree     * sParseTree;
    qcmSequenceInfo           sSequenceInfo;
    void                    * sSequenceHandle = NULL;
    SChar                     sSeqUserName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar                     sSeqTableNameStr[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qcNamePosition            sSeqTableName;

    // PROJ-2365 sequence table 정보
    qcmTableInfo            * sSeqTableInfo;
    void                    * sSeqTableHandle = NULL;
    smSCN                     sSeqTableSCN;
    SLong                     sLastSyncSeq;

    SChar                   * sSqlStr;
    vSLong                    sRowCnt;

    /* TASK-7217 Sharded sequence */
    idBool                    sIsRestart;
    
    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    /* TASK-7217 Sharded sequence */
    if ( (sParseTree->sequenceOptions->flag & QDS_SEQ_OPT_RESTART_MASK) == QDS_SEQ_OPT_RESTART_TRUE )
    {
        sIsRestart = ID_TRUE;
    }
    else
    {
        sIsRestart = ID_FALSE;
    }

    // PROJ-2365 sequence table
    if ( ( *sParseTree->sequenceOptions->cycleOption & SMI_SEQUENCE_TABLE_MASK )
         == SMI_SEQUENCE_TABLE_TRUE )
    {
        // make sequence table name : SEQ1$SEQ
        IDE_TEST_RAISE( sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN
                        > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SEQUENCE_NAME_LEN );
        
        QC_STR_COPY( sSeqTableNameStr, sParseTree->sequenceName );
        idlOS::strncat( sSeqTableNameStr,
                        QDS_SEQ_TABLE_SUFFIX_STR,
                        QDS_SEQ_TABLE_SUFFIX_LEN );
        
        sSeqTableName.stmtText = sSeqTableNameStr;
        sSeqTableName.offset   = 0;
        sSeqTableName.size     = sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN;

        IDE_TEST_RAISE( qcm::getTableHandleByName(
                            QC_SMI_STMT(aStatement),
                            sParseTree->userID,
                            (UChar*) sSeqTableName.stmtText,
                            sSeqTableName.size,
                            &sSeqTableHandle,
                            &sSeqTableSCN ) != IDE_SUCCESS,
                        ERR_NOT_EXIST_TABLE );
        
        IDE_TEST( smiGetTableTempInfo( sSeqTableHandle,
                                       (void**)&sSeqTableInfo )
                  != IDE_SUCCESS );
        
        // sequence table에 DDL lock
        // update만 하지만 논리적으로 sequence를 변경하는 것으므로 DDL lock을 획득한다.
        IDE_TEST( qcm::validateAndLockTable( aStatement,
                                             sSeqTableHandle,
                                             sSeqTableSCN,
                                             SMI_TABLE_LOCK_X )
                  != IDE_SUCCESS );
        
        // if specified tables is replicated, the ERR_DDL_WITH_REPLICATED_TABLE
        IDE_TEST_RAISE( sSeqTableInfo->replicationCount > 0,
                        ERR_DDL_WITH_REPLICATED_TABLE );

        //proj-1608:replicationCount가 0일 때 recovery count는 항상 0이어야 함
        IDE_DASSERT( sSeqTableInfo->replicationRecoveryCount == 0 );
    }
    else
    {
        // Nothing to do.
    }
    
    IDE_TEST(smiTable::alterSequence(
                 QC_SMI_STMT( aStatement ),
                 sParseTree->sequenceHandle,
                 *sParseTree->sequenceOptions->incrementValue,
                 *sParseTree->sequenceOptions->cacheValue,
                 *sParseTree->sequenceOptions->maxValue,
                 *sParseTree->sequenceOptions->minValue,
                 *sParseTree->sequenceOptions->cycleOption,
                 sIsRestart,
                 *sParseTree->sequenceOptions->startValue,
                 &sLastSyncSeq)
             != IDE_SUCCESS);
    
    // PROJ-2365 sequence table
    // last sync seq로 update한다.
    if ( ( *sParseTree->sequenceOptions->cycleOption & SMI_SEQUENCE_TABLE_MASK )
         == SMI_SEQUENCE_TABLE_TRUE )
    {
        IDE_TEST( qcmUser::getUserName( aStatement,
                                        sParseTree->userID,
                                        sSeqUserName )
                  != IDE_SUCCESS );
        
        IDU_LIMITPOINT( "qds::executeAlterOptions::malloc");
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          &sSqlStr )
                  != IDE_SUCCESS);

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE %s.%s SET LAST_SYNC_SEQ = BIGINT'%"ID_INT64_FMT"'",
                         sSeqUserName,
                         sSeqTableNameStr,
                         sLastSyncSeq );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt == 0, ERR_META_CRASH );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
       DDL Statement Text의 로깅
    */
    if (QCU_DDL_SUPPLEMENTAL_LOG == 1)
    {
        IDE_TEST(qcm::checkSequenceInfo(aStatement,
                                        sParseTree->userID,
                                        sParseTree->sequenceName,
                                        &sSequenceInfo,
                                        &sSequenceHandle)
                 != IDE_SUCCESS);

        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sSequenceInfo.name )
                  != IDE_SUCCESS );
    }

    // fix BUG-14394
    IDE_TEST(qdbCommon::updateTableSpecFromMeta(
            aStatement,
            sParseTree->userName,
            sParseTree->sequenceName,
            sParseTree->sequenceID,
            smiGetTableId(sParseTree->sequenceHandle),
            0,
            1) /* PROJ-1071 default parallel degree = 1 */
        != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE, "" ) );
    }
    IDE_EXCEPTION( ERR_DDL_WITH_REPLICATED_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDD_DDL_WITH_REPLICATED_TBL ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION( ERR_SEQUENCE_NAME_LEN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qds::executeAlterOptions",
                                  "sequence name is too long" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qds::executeAlterSeqTable(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER SEQUENCE ... 의 execution  수행
 *
 * Implementation :
 *    1. 메타에서 시퀀스 핸들 구하기
 *    2. smiTable::alterSequence
 *
 ***********************************************************************/

#define IDE_FN "qds::executeAlterSeqTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::executeAlterSeqTable"));

    qdSequenceParseTree     * sParseTree;
    qcmSequenceInfo           sSequenceInfo;
    void                    * sSequenceHandle = NULL;
    SLong                     sStartValue;
    SLong                     sCurrIncrementValue;
    SLong                     sCurrMinValue;
    SLong                     sCurrMaxValue;
    SLong                     sCurrCacheValue;
    UInt                      sOption;
    SChar                     sSeqTableNameStr[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qcNamePosition            sSeqTableName;
    qsOID                     sProcID;
    idBool                    sExist = ID_FALSE;
    UInt                      sUserID;
    
    // PROJ-2365 sequence table 정보
    qcmTableInfo            * sOldSeqTableInfo = NULL;
    qcmTableInfo            * sNewSeqTableInfo = NULL;
    void                    * sSeqTableHandle  = NULL;
    smSCN                     sSeqTableSCN;
    SLong                     sLastSyncSeq;

    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST(smiTable::getSequence( sParseTree->sequenceHandle,
                                    &sStartValue,
                                    &sCurrIncrementValue,
                                    &sCurrCacheValue,
                                    &sCurrMaxValue,
                                    &sCurrMinValue,
                                    &sOption)
             != IDE_SUCCESS);

    // make sequence table name : SEQ1$SEQ
    IDE_TEST_RAISE( sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN
                    > QC_MAX_OBJECT_NAME_LEN,
                    ERR_SEQUENCE_NAME_LEN );
        
    QC_STR_COPY( sSeqTableNameStr, sParseTree->sequenceName );
    idlOS::strncat( sSeqTableNameStr,
                    QDS_SEQ_TABLE_SUFFIX_STR,
                    QDS_SEQ_TABLE_SUFFIX_LEN );
        
    sSeqTableName.stmtText = sSeqTableNameStr;
    sSeqTableName.offset   = 0;
    sSeqTableName.size     = sParseTree->sequenceName.size + QDS_SEQ_TABLE_SUFFIX_LEN;
        
    // PROJ-2365 sequence table
    // 이미 생성된 sequence table에 대한 에러와 alter에 대한 에러
    if ( ( sOption & SMI_SEQUENCE_TABLE_MASK ) == SMI_SEQUENCE_TABLE_TRUE )
    {
        // sequence table이 존재해야한다.
        IDE_TEST_RAISE( qcm::getTableHandleByName(
                            QC_SMI_STMT(aStatement),
                            sParseTree->userID,
                            (UChar*) sSeqTableName.stmtText,
                            sSeqTableName.size,
                            &sSeqTableHandle,
                            &sSeqTableSCN ) != IDE_SUCCESS,
                        ERR_NOT_EXIST_TABLE );
        
        IDE_TEST( smiGetTableTempInfo( sSeqTableHandle,
                                       (void**)&sOldSeqTableInfo )
                  != IDE_SUCCESS );
        
        // 이미 enable되어 있다.
        IDE_TEST_RAISE( sParseTree->enableSeqTable == ID_TRUE, ERR_EXIST_TABLE );
        
        // sequence table에 DDL lock
        IDE_TEST( qcm::validateAndLockTable( aStatement,
                                             sSeqTableHandle,
                                             sSeqTableSCN,
                                             SMI_TABLE_LOCK_X )
                  != IDE_SUCCESS );

        // if specified tables is replicated, the ERR_DDL_WITH_REPLICATED_TABLE
        IDE_TEST_RAISE( sOldSeqTableInfo->replicationCount > 0,
                        ERR_DDL_WITH_REPLICATED_TABLE );

        //proj-1608:replicationCount가 0일 때 recovery count는 항상 0이어야 함
        IDE_DASSERT( sOldSeqTableInfo->replicationRecoveryCount == 0 );

        // drop sequence table
        IDE_TEST( dropSequenceTable( aStatement,
                                     sOldSeqTableInfo )
                  != IDE_SUCCESS );
        
        sOption &= ~SMI_SEQUENCE_TABLE_MASK;
        sOption |= SMI_SEQUENCE_TABLE_FALSE;
    }
    else
    {
        IDE_TEST( qcm::existObject( aStatement,
                                    ID_FALSE,
                                    sParseTree->userName,
                                    sSeqTableName,
                                    QS_OBJECT_MAX,
                                    &sUserID,
                                    &sProcID,
                                    &sExist )
                  != IDE_SUCCESS );

        // sequence table name이 사용중이면 에러.
        IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_TABLE );
        
        // 이미 disable되어 있다.
        IDE_TEST_RAISE( sParseTree->enableSeqTable == ID_FALSE, ERR_NOT_EXIST_TABLE );
        
        sOption &= ~SMI_SEQUENCE_TABLE_MASK;
        sOption |= SMI_SEQUENCE_TABLE_TRUE;
    }
    
    // alter sequence
    IDE_TEST(smiTable::alterSequence(
                 QC_SMI_STMT( aStatement ),
                 sParseTree->sequenceHandle,
                 sCurrIncrementValue,
                 sCurrCacheValue,
                 sCurrMaxValue,
                 sCurrMinValue,
                 sOption,
                 ID_FALSE,  // RESTART
                 0,         // RESTART가 FALSE이면 사용하지 않음.
                 &sLastSyncSeq)
             != IDE_SUCCESS);
    
    // PROJ-2365 sequence table
    if ( ( sOption & SMI_SEQUENCE_TABLE_MASK ) == SMI_SEQUENCE_TABLE_TRUE )
    {
        // create sequence table
        // last sync seq로 start value를 설정한다.
        IDE_TEST( createSequenceTable( aStatement,
                                       sSeqTableName,
                                       sStartValue,
                                       sCurrIncrementValue,
                                       sCurrMinValue,
                                       sCurrMaxValue,
                                       sCurrCacheValue,
                                       sOption,
                                       sLastSyncSeq,
                                       & sNewSeqTableInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
       DDL Statement Text의 로깅
    */
    if (QCU_DDL_SUPPLEMENTAL_LOG == 1)
    {
        IDE_TEST(qcm::checkSequenceInfo(aStatement,
                                        sParseTree->userID,
                                        sParseTree->sequenceName,
                                        &sSequenceInfo,
                                        &sSequenceHandle)
                 != IDE_SUCCESS);

        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sSequenceInfo.name )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // fix BUG-14394
    IDE_TEST(qdbCommon::updateTableSpecFromMeta(
                aStatement,
                sParseTree->userName,
                sParseTree->sequenceName,
                sParseTree->sequenceID,
                smiGetTableId(sParseTree->sequenceHandle),
                0,
                1)
             != IDE_SUCCESS);

    if ( sOldSeqTableInfo != NULL )
    {
        (void) qcm::destroyQcmTableInfo( sOldSeqTableInfo );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE, "" ) );
    }
    IDE_EXCEPTION( ERR_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_EXIST_OBJECT_NAME ) );
    }
    IDE_EXCEPTION( ERR_DDL_WITH_REPLICATED_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDD_DDL_WITH_REPLICATED_TBL ) );
    }
    IDE_EXCEPTION( ERR_SEQUENCE_NAME_LEN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qds::executeAlterSeqTable",
                                  "sequence name is too long" ) );
    }
    IDE_EXCEPTION_END;

    if ( sNewSeqTableInfo != NULL )
    {
        (void) qcm::destroyQcmTableInfo( sNewSeqTableInfo );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qds::executeAlterFlushCache(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER SEQUENCE ... 의 execution  수행
 *
 * Implementation :
 *    1. 메타에서 시퀀스 핸들 구하기
 *    2. smiTable::refineSequence
 *
 ***********************************************************************/

#define IDE_FN "qds::executeAlterFlushCache"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::executeAlterFlushCache"));

    qdSequenceParseTree     * sParseTree;
    qcmSequenceInfo           sSequenceInfo;
    void                    * sSequenceHandle = NULL;
    
    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    // refine sequence
    IDE_TEST(smiTable::refineSequence(
                 QC_SMI_STMT( aStatement ),
                 sParseTree->sequenceHandle)
             != IDE_SUCCESS);
    
    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
       DDL Statement Text의 로깅
    */
    if (QCU_DDL_SUPPLEMENTAL_LOG == 1)
    {
        IDE_TEST(qcm::checkSequenceInfo(aStatement,
                                        sParseTree->userID,
                                        sParseTree->sequenceName,
                                        &sSequenceInfo,
                                        &sSequenceHandle)
                 != IDE_SUCCESS);

        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sSequenceInfo.name )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // fix BUG-14394
    IDE_TEST(qdbCommon::updateTableSpecFromMeta(
                aStatement,
                sParseTree->userName,
                sParseTree->sequenceName,
                sParseTree->sequenceID,
                smiGetTableId(sParseTree->sequenceHandle),
                0,
                1)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qds::checkSequence(
    qcStatement      * aStatement,
    qcParseSeqCaches * aSequence,
    UInt               aReadFlag )
{
/***********************************************************************
 *
 * Description :
 *    BUG-17455
 *    1. 시퀀스 존재 여부 확인
 *    PROJ-2365
 *    2. sequence table이 변경되었는지 확인
 *    3. sequence table에 lock
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qds::checkSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::checkSequence"));

    SLong       sStartValue;
    SLong       sIncrementValue;
    SLong       sCacheValue;
    SLong       sMaxValue;
    SLong       sMinValue;
    UInt        sOption;

    // sequence가 drop되었다면 rebuild
    IDE_TEST_RAISE( smiTable::getSequence( aSequence->sequenceHandle,
                                           & sStartValue,
                                           & sIncrementValue,
                                           & sCacheValue,
                                           & sMaxValue,
                                           & sMinValue,
                                           & sOption ) != IDE_SUCCESS,
                    ERR_REBUILD_QCI_EXEC );

    // sequence table이 변경되었다면 rebuild
    if ( ( sOption & SMI_SEQUENCE_TABLE_MASK ) == SMI_SEQUENCE_TABLE_TRUE )
    {
        IDE_TEST_RAISE( aSequence->sequenceTable == ID_FALSE,
                        ERR_REBUILD_QCI_EXEC );
    }
    else
    {
        IDE_TEST_RAISE( aSequence->sequenceTable == ID_TRUE,
                        ERR_REBUILD_QCI_EXEC );
    }

    // sequence table에 update lock (currval인 경우는 필요없음)
    if ( ( ( aReadFlag & SMI_SEQUENCE_MASK ) == SMI_SEQUENCE_NEXT ) &&
         ( aSequence->sequenceTable == ID_TRUE ) )
    {
        IDE_TEST( qcm::validateAndLockTable( aStatement,
                                             aSequence->tableHandle,
                                             aSequence->tableSCN,
                                             SMI_TABLE_LOCK_IX )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REBUILD_QCI_EXEC );
    {
        IDE_SET(ideSetErrorCode(qpERR_REBUILD_QCI_EXEC));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qds::checkSequenceExist(
    qcStatement      * aStatement,
    qcNamePosition     aUserName,
    qcNamePosition     aSequenceName,
    UInt             * aUserID,
    qcmSequenceInfo  * aSequenceInfo,
    void            ** aSequenceHandle)
{
/***********************************************************************
 *
 * Description :
 *    유저명과 시퀀스 이름으로 user ID, 시퀀스 핸들 구하기
 *
 * Implementation :
 *    1. 유저명이 명시되지 않았으면 세션의 유저 ID 반환,
 *       그렇지 않으면 명시된 유저의 user ID 구하기
 *    2. user ID 와 시퀀스 이름으로 핸들 구하기
 *
 ***********************************************************************/

#define IDE_FN "qds::checkSequenceExist"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qds::checkSequenceExist"));

    UInt        sUserID;

    // check user existence
    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        sUserID = QCG_GET_SESSION_USER_ID(aStatement);
    }
    else
    {
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, &sUserID )
                 != IDE_SUCCESS);
    }

    IDE_TEST(qcm::checkSequenceInfo( aStatement,
                                     sUserID,
                                     aSequenceName,
                                     aSequenceInfo,
                                     aSequenceHandle )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(aSequenceInfo->sequenceType != QCM_SEQUENCE,
                   ERR_NOT_EXIST_SEQUENCE);

    *aUserID = sUserID;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_SEQUENCE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_SEQUENCE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qds::createSequenceTable( qcStatement     * aStatement,
                                 qcNamePosition    aSeqTableName,
                                 SLong             aStartValue,
                                 SLong             aIncValue,
                                 SLong             aMinValue,
                                 SLong             aMaxValue,
                                 SLong             aCacheValue,
                                 UInt              aCycleOption,
                                 SLong             aLastSeqValue,
                                 qcmTableInfo   ** aSeqTableInfo )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2362 sequence table을 생성한다.
 *
 * Implementation :
 *    sequence option을 handshake하기 위하여 column name에 option을 넣는다.(!)
 *
 *    create table seq1$seq( id             smallint primary key,
 *                           last_sync_seq  bigint   not null,
 *                           start_1        smallint not null,
 *                           inc_1          smallint not null,
 *                           min_-999       smallint not null,
 *                           max_999        smallint not null,
 *                           cache_20       smallint not null,
 *                           cycle_1        smallint not null,
 *                           last_time      timestamp );
 *
 *    insert seq_table values( 0, start_value, 0, 0, 0, 0, 0, 0 );
 *
 ***********************************************************************/
    
    qdSequenceParseTree   * sParseTree;
    
    qcmColumn               sColumns[9];
    mtcColumn               sMtcColumns[9];
    smiValue                sNewRow[9];
    UInt                    sColumnCount = 9;
    smiColumnList           sColumnList[1];
    qcNamePosition          sNullPosition;
    qcmTableInfo          * sTableInfo = NULL;
    void                  * sTableHandle;
    smSCN                   sSCN;
    smiSegAttr              sSegAttr;
    smiSegStorageAttr       sSegStorageAttr;
    UInt                    sTableID;
    smOID                   sTableOID;
    UInt                    i;
    
    SChar                   sObjName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    UInt                    sIndexID;
    UInt                    sIndexType;
    const void            * sIndexHandle;
    UInt                    sCreateFlag = 0;
    UInt                    sIndexFlag = 0;
    UInt                    sBuildFlag = 0;
    UInt                    sConstrID;
    
    smiTableCursor          sCursor;
    smiCursorProperties     sCursorProperty;
    void                  * sRow;
    scGRID                  sGRID;
    idBool                  sOpened = ID_FALSE;
    
    mtdBigintType           sLastSyncSeqValue;
    mtdSmallintType         sSmallintValue;
    ULong                   sBuffer[2];
    mtdByteType           * sLastTimeValue = (mtdByteType*) sBuffer;
    UInt                    sLastTimeValueSize;

    sParseTree = (qdSequenceParseTree *)aStatement->myPlan->parseTree;

    //------------------------------------
    // 초기화
    //------------------------------------

    QD_SEGMENT_OPTION_INIT( &sSegAttr, &sSegStorageAttr );
    
    sNullPosition.stmtText = NULL;
    sNullPosition.offset = 0;
    sNullPosition.size = 0;

    // BUG-43814
    idlOS::memset( &sMtcColumns, 0x00, ID_SIZEOF(mtcColumn) * sColumnCount );
    
    for ( i = 0; i < sColumnCount; i++ )
    {
        QCM_COLUMN_INIT( (&(sColumns[i])) );
        sMtcColumns[i].flag = 0;
        
        SET_POSITION( sColumns[i].namePos, sNullPosition );
        
        sColumns[i].basicInfo   = &(sMtcColumns[i]);
        sColumns[i].flag        = QCM_COLUMN_TYPE_FIXED;
        sColumns[i].inRowLength = ID_UINT_MAX;
        
        if ( i + 1 < sColumnCount )
        {
            sColumns[i].next = &(sColumns[i + 1]);
        }
        else
        {
            sColumns[i].next = NULL;
        }
    }
    
    //------------------------------------
    // make qcmColumns
    //------------------------------------

    i = 0;
    
    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_SMALLINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    // not null
    sMtcColumns[i].flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_NOTNULL_TRUE;
    
    idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "ID" );
    i++;
    
    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_BIGINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    // not null
    sMtcColumns[i].flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_NOTNULL_TRUE;
    
    idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "LAST_SYNC_SEQ" );
    i++;

    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_SMALLINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    // handshake용 not null
    sMtcColumns[i].flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_NOTNULL_TRUE;
    
    idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "START_%"ID_INT64_FMT, aStartValue );
    i++;
    
    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_SMALLINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    // handshake용 not null
    sMtcColumns[i].flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_NOTNULL_TRUE;
    
    idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "INC_%"ID_INT64_FMT, aIncValue );
    i++;
    
    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_SMALLINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    // handshake용 not null
    sMtcColumns[i].flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_NOTNULL_TRUE;
    
    idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "MIN_%"ID_INT64_FMT, aMinValue );
    i++;
    
    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_SMALLINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    // handshake용 not null
    sMtcColumns[i].flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_NOTNULL_TRUE;
    
    idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "MAX_%"ID_INT64_FMT, aMaxValue );
    i++;
    
    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_SMALLINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    // handshake용 not null
    sMtcColumns[i].flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_NOTNULL_TRUE;
    
    idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "CACHE_%"ID_INT64_FMT, aCacheValue );
    i++;
    
    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_SMALLINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    // handshake용 not null
    sMtcColumns[i].flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_NOTNULL_TRUE;
    
    if ( ( aCycleOption & SMI_SEQUENCE_CIRCULAR_MASK ) == SMI_SEQUENCE_CIRCULAR_ENABLE )
    {
        idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "CYCLE_1" );
    }
    else
    {
        idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "CYCLE_0" );
    }
    i++;
    
    IDE_TEST( mtc::initializeColumn(
                  &(sMtcColumns[i]),
                  MTD_BYTE_ID,
                  1,
                  QC_BYTE_PRECISION_FOR_TIMESTAMP,
                  0 )
              != IDE_SUCCESS );

    // conflict resolution용 컬럼
    sMtcColumns[i].flag &= ~MTC_COLUMN_TIMESTAMP_MASK;
    sMtcColumns[i].flag |= MTC_COLUMN_TIMESTAMP_TRUE;
    
    idlOS::snprintf( sColumns[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "LAST_TIME" );
    i++;
    
    IDE_DASSERT( i == sColumnCount );
    
    IDE_TEST( qdbCommon::validateColumnListForCreateInternalTable(
                  aStatement,
                  ID_TRUE,  // in execution time
                  SMI_TABLE_MEMORY,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA,
                  sColumns )
              != IDE_SUCCESS );
    
    //------------------------------------
    // create table
    //------------------------------------

    sCreateFlag &= ~QDV_HIDDEN_SEQUENCE_TABLE_MASK;
    sCreateFlag |= QDV_HIDDEN_SEQUENCE_TABLE_TRUE;
    
    IDE_TEST( qcm::getNextTableID( aStatement, &sTableID ) != IDE_SUCCESS );
    
    IDE_TEST( qdbCommon::createTableOnSM( aStatement,
                                          sColumns,
                                          sParseTree->userID,
                                          sTableID,
                                          ID_ULONG_MAX,
                                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA,
                                          sSegAttr,
                                          sSegStorageAttr,
                                          0, /* Table Flag Mask */ 
                                          0, /* Table Flag Value */
                                          1, /* Parallel Degree */
                                          &sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qdbCommon::insertTableSpecIntoMeta( aStatement,
                                                  ID_FALSE,
                                                  sCreateFlag,
                                                  aSeqTableName,
                                                  sParseTree->userID,
                                                  sTableOID,
                                                  sTableID,
                                                  sColumnCount,
                                                  ID_ULONG_MAX,
                                                  /* PROJ-2359 Table/Partition Access Option */
                                                  QCM_ACCESS_OPTION_READ_WRITE,
                                                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA,
                                                  sSegAttr,
                                                  sSegStorageAttr,
                                                  QCM_TEMPORARY_ON_COMMIT_NONE,
                                                  1,
                                                  QCM_SHARD_FLAG_TABLE_NONE )
              != IDE_SUCCESS );
    
    IDE_TEST( qdbCommon::insertColumnSpecIntoMeta( aStatement,
                                                   sParseTree->userID,
                                                   sTableID,
                                                   sColumns,
                                                   ID_FALSE )
              != IDE_SUCCESS );

    //------------------------------------
    // create primary key index
    //------------------------------------

    sMtcColumns[0].column.offset = 0;
    sMtcColumns[0].column.value = NULL;

    sColumnList[0].column = (smiColumn*) &(sMtcColumns[0]);
    sColumnList[0].next = NULL;
    
    sIndexFlag = SMI_INDEX_UNIQUE_ENABLE |
        SMI_INDEX_TYPE_PRIMARY_KEY |
        SMI_INDEX_USE_ENABLE |
        SMI_INDEX_PERSISTENT_DISABLE;

    sBuildFlag = SMI_INDEX_BUILD_UNCOMMITTED_ROW_DISABLE;

    sIndexType = smiGetDefaultIndexType();
    
    IDE_TEST( qcm::getNextIndexID( aStatement, &sIndexID ) != IDE_SUCCESS );

    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
    {
        idlOS::snprintf( sObjName, ID_SIZEOF(sObjName),
                         "%sIDX_ID_%"ID_INT32_FMT"",
                         QC_SYS_OBJ_NAME_HEADER,
                         sIndexID );
    }
    else
    {
        // SEQ1$SEQ
        QC_STR_COPY( sObjName, aSeqTableName );

        // SEQ1
        sObjName[aSeqTableName.size - QDS_SEQ_TABLE_SUFFIX_LEN] = '\0';
        
        // SEQ1$IDX
        idlOS::strncat( sObjName, "$IDX", 4 );
    }
    
    IDE_TEST( smiTable::createIndex( aStatement->mStatistics,
                                     QC_SMI_STMT( aStatement ),
                                     SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA,
                                     smiGetTable( sTableOID ),
                                     (SChar*)sObjName,
                                     sIndexID,
                                     sIndexType,
                                     sColumnList,
                                     sIndexFlag,
                                     0,
                                     sBuildFlag,
                                     sSegAttr,
                                     sSegStorageAttr,
                                     0, /* PROJ-2433 : sequence table은 direct key index를 이용안함 */
                                     &sIndexHandle )
              != IDE_SUCCESS );

    IDE_TEST( qdx::insertIndexIntoMeta( aStatement,
                                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA,
                                        sParseTree->userID,
                                        sTableID,
                                        sIndexID,
                                        sObjName,
                                        sIndexType,
                                        ID_TRUE,
                                        1,
                                        ID_TRUE,
                                        ID_FALSE,
                                        0,
                                        sIndexFlag )
                 != IDE_SUCCESS );

    IDE_TEST( qdx::insertIndexColumnIntoMeta(
                  aStatement,
                  sParseTree->userID,
                  sIndexID,
                  sMtcColumns[0].column.id,
                  0,
                  ID_TRUE,
                  sTableID )
              != IDE_SUCCESS );

    //------------------------------------
    // create primary key constraint
    //------------------------------------
    
    IDE_TEST( qcm::getNextConstrID( aStatement, &sConstrID ) != IDE_SUCCESS);

    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
    {
        idlOS::snprintf( sObjName, ID_SIZEOF(sObjName),
                         "%sCON_PRIMARY_ID_%"ID_INT32_FMT"",
                         QC_SYS_OBJ_NAME_HEADER,
                         sConstrID );
    }
    else
    {
        // SEQ1$SEQ
        QC_STR_COPY( sObjName, aSeqTableName );
        
        // SEQ1
        sObjName[aSeqTableName.size - QDS_SEQ_TABLE_SUFFIX_LEN] = '\0';

        // SEQ1$PK
        idlOS::strncat( sObjName, "$PK", 3 );
    }
    
    IDE_TEST( qdn::insertConstraintIntoMeta(
                  aStatement,
                  sParseTree->userID,
                  sTableID,
                  sConstrID,
                  sObjName,
                  QD_PRIMARYKEY,
                  sIndexID,
                  1,
                  0,
                  0,
                  0,
                  (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                  ID_TRUE ) // ConstraintState의 Validate
              != IDE_SUCCESS );
    
    IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                  aStatement,
                  sParseTree->userID,
                  sTableID,
                  sConstrID,
                  0,
                  sMtcColumns[0].column.id )
              != IDE_SUCCESS );

    //------------------------------------
    // create not null constraint
    //------------------------------------

    for ( i = 1; i <= 7; i++ )
    {
        IDE_TEST( qcm::getNextConstrID( aStatement, &sConstrID ) != IDE_SUCCESS);

        idlOS::snprintf( sObjName, ID_SIZEOF(sObjName),
                         "%sCON_NOT_NULL_ID_%"ID_INT32_FMT"",
                         QC_SYS_OBJ_NAME_HEADER,
                         sConstrID );
        
        IDE_TEST( qdn::insertConstraintIntoMeta(
                      aStatement,
                      sParseTree->userID,
                      sTableID,
                      sConstrID,
                      sObjName,
                      QD_NOT_NULL,
                      0,
                      1,
                      0,
                      0,
                      0,
                      (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                      ID_TRUE ) // ConstraintState의 Validate
                  != IDE_SUCCESS );

        IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                      aStatement,
                      sParseTree->userID,
                      sTableID,
                      sConstrID,
                      0,
                      sMtcColumns[i].column.id )
                  != IDE_SUCCESS );
    }
    
    //------------------------------------
    // create timestamp constraint
    //------------------------------------

    IDE_TEST( qcm::getNextConstrID( aStatement, &sConstrID ) != IDE_SUCCESS);

    idlOS::snprintf( sObjName, ID_SIZEOF(sObjName),
                     "%sCON_TIMESTAMP_%"ID_INT32_FMT"",
                     QC_SYS_OBJ_NAME_HEADER,
                     sConstrID );
    
    IDE_TEST( qdn::insertConstraintIntoMeta(
                  aStatement,
                  sParseTree->userID,
                  sTableID,
                  sConstrID,
                  sObjName,
                  QD_TIMESTAMP,
                  0,
                  1,
                  0,
                  0,
                  0,
                  (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                  ID_TRUE ) // ConstraintState의 Validate
              != IDE_SUCCESS );

    IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                  aStatement,
                  sParseTree->userID,
                  sTableID,
                  sConstrID,
                  0,
                  sMtcColumns[8].column.id )
              != IDE_SUCCESS );
    
    //------------------------------------
    // create cached meta
    //------------------------------------
        
    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     &sTableInfo,
                                     &sSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );
    
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sTableHandle,
                                         sSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    //------------------------------------
    // insert row
    //------------------------------------

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN(&sCursorProperty, aStatement->mStatistics);
    sCursorProperty.mIsUndoLogging = ID_FALSE;
    
    // make new row
    sLastSyncSeqValue = (mtdBigintType)aLastSeqValue;
    sSmallintValue    = (mtdSmallintType)0;

    sLastTimeValueSize = MTD_BYTE_TYPE_STRUCT_SIZE(QC_BYTE_PRECISION_FOR_TIMESTAMP);
    IDE_DASSERT( ID_SIZEOF(sBuffer) >= sLastTimeValueSize );
    IDE_TEST( qmx::setTimeStamp( sLastTimeValue->value ) != IDE_SUCCESS );
    sLastTimeValue->length = QC_BYTE_PRECISION_FOR_TIMESTAMP;
    
    for ( i = 0; i < sColumnCount; i++ )
    {
        if ( i == 1 )
        {
            // last_sync_seq column
            sNewRow[i].value  = (void*) &sLastSyncSeqValue;
            sNewRow[i].length = ID_SIZEOF(mtdBigintType);
        }
        else if ( i == 8 )
        {
            // last_time column
            sNewRow[i].value  = (void*) sLastTimeValue;
            sNewRow[i].length = sLastTimeValueSize;
        }
        else
        {
            sNewRow[i].value  = (void*) &sSmallintValue;
            sNewRow[i].length = ID_SIZEOF(mtdSmallintType);
        }
    }
    
    sCursor.initialize();
    
    IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                            sTableHandle,
                            NULL,
                            sSCN,
                            NULL,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            SMI_LOCK_WRITE |
                            SMI_TRAVERSE_FORWARD |
                            SMI_PREVIOUS_DISABLE,
                            SMI_INSERT_CURSOR,
                            & sCursorProperty )
              != IDE_SUCCESS );
    sOpened = ID_TRUE;

    IDE_TEST( sCursor.insertRow( sNewRow,
                                 & sRow,
                                 & sGRID )
             != IDE_SUCCESS );

    sOpened = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );
    
    //------------------------------------
    // 완료
    //------------------------------------
    
    *aSeqTableInfo = sTableInfo;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sOpened == ID_TRUE )
    {
        (void) sCursor.close();
    }
    else
    {
        // Nothing to do.
    }
    
    if ( sTableInfo != NULL )
    {
        (void) qcm::destroyQcmTableInfo( sTableInfo );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_FAILURE;
}

IDE_RC qds::dropSequenceTable( qcStatement   * aStatement,
                               qcmTableInfo  * aTableInfo )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2362 sequence table을 drop한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmTableInfo   * sTableInfo;
    UInt             sTableID;

    sTableInfo = aTableInfo;
    sTableID   = sTableInfo->tableID;

    // DELETE from qcm_constraints_ where table id = aTableID;
    IDE_TEST(qdd::deleteConstraintsFromMeta(aStatement, sTableID)
             != IDE_SUCCESS);

    IDE_TEST(qdd::deleteIndicesFromMeta(aStatement, sTableInfo)
             != IDE_SUCCESS);

    IDE_TEST(qdd::deleteTableFromMeta(aStatement, sTableID)
             != IDE_SUCCESS);

    IDE_TEST(qdpDrop::removePriv4DropTable(aStatement, sTableID)
             != IDE_SUCCESS);

    // related VIEW
    IDE_TEST(qcmView::setInvalidViewOfRelated(
                 aStatement,
                 sTableInfo->tableOwnerID,
                 sTableInfo->name,
                 idlOS::strlen(sTableInfo->name),
                 QS_TABLE)
             != IDE_SUCCESS);

    // BUG-21387 COMMENT
    IDE_TEST(qdbComment::deleteCommentTable(
                 aStatement,
                 sTableInfo->tableOwnerName,
                 sTableInfo->name )
             != IDE_SUCCESS);

    // PROJ-2223 audit
    IDE_TEST( qcmAudit::deleteObject(
                  aStatement,
                  sTableInfo->tableOwnerID,
                  sTableInfo->name )
              != IDE_SUCCESS );

    // sequence table은 항상 sys_mem_tbs에 생성된다.
    IDE_TEST( smiTable::dropTable( QC_SMI_STMT( aStatement ),
                                   sTableInfo->tableHandle,
                                   SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qds::selectCurrValTx( SLong  * aCurrVal,
                             void   * aInfo )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2362 sequence table
 *    readSequence에서 sequence table의 last_sync_seq를 select한다.
 *    항상 selectCurrValTx와 updateLastValTx가 순서대로 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qdsCallBackInfo  * sCallBack = (qdsCallBackInfo*) aInfo;
    smiStatement     * sDummySmiStmt;
    UInt               sSmiStmtFlag = 0;
    const void       * sRow = NULL;
    scGRID             sRid;
    SInt               sState = 0;

    IDE_ASSERT( sCallBack != NULL );

    // transaction initialze
    IDE_TEST( sCallBack->mSmiTrans.initialize()
              != IDE_SUCCESS );
    sState = 1;

    /* PROJ-2446 ONE SOURCE
     * sequenceTable= true인 경우 callback 의 mstatistics설정 되어 있다. */
    // transaction begin
    IDE_TEST( sCallBack->mSmiTrans.begin( & sDummySmiStmt, sCallBack->mStatistics )
              != IDE_SUCCESS );
    sState = 2;

    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    // statement begin
    IDE_TEST( sCallBack->mSmiStmt.begin( sCallBack->mStatistics, sDummySmiStmt, sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 3;

    // select
    IDE_TEST( smiGetTableColumns(  sCallBack->mTableHandle,
                                   1,
                                   (const smiColumn**)&sCallBack->mLastSyncSeqColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns(  sCallBack->mTableHandle,
                                   8,
                                   (const smiColumn**)&sCallBack->mLastTimeColumn )
              != IDE_SUCCESS );
    
    sCallBack->mUpdateColumn[0].column = (smiColumn*) &(sCallBack->mLastSyncSeqColumn->column);
    sCallBack->mUpdateColumn[0].next = &(sCallBack->mUpdateColumn[1]);
    sCallBack->mUpdateColumn[1].column = (smiColumn*) &(sCallBack->mLastTimeColumn->column);
    sCallBack->mUpdateColumn[1].next = NULL;
    
    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( & (sCallBack->mProperty), sCallBack->mStatistics );
    
    sCallBack->mCursor.initialize();
    
    IDE_TEST( sCallBack->mCursor.open( & (sCallBack->mSmiStmt),
                                       sCallBack->mTableHandle,
                                       NULL,
                                       sCallBack->mTableSCN,
                                       sCallBack->mUpdateColumn,
                                       smiGetDefaultKeyRange(),
                                       smiGetDefaultKeyRange(),
                                       smiGetDefaultFilter(),
                                       SMI_LOCK_WRITE | SMI_TRAVERSE_FORWARD |
                                       SMI_PREVIOUS_DISABLE,
                                       SMI_UPDATE_CURSOR,
                                       & (sCallBack->mProperty) )
              != IDE_SUCCESS );
    sState = 4;
    
    IDE_TEST( sCallBack->mCursor.beforeFirst()
              != IDE_SUCCESS );
    
    IDE_TEST( sCallBack->mCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, ERR_NOT_EXIST_TABLE );
    
    *aCurrVal = *(mtdBigintType*)((UChar *)sRow + sCallBack->mLastSyncSeqColumn->column.offset);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_TABLE ) );
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 4:
            (void) sCallBack->mCursor.close();
        case 3:
            (void) sCallBack->mSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void) sCallBack->mSmiTrans.rollback();
        case 1:
            sCallBack->mSmiTrans.destroy( sCallBack->mStatistics );
            break;
    }
    
    return IDE_FAILURE;
}

IDE_RC qds::updateLastValTx( SLong    aLastVal,
                             void   * aInfo )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2362 sequence table
 *    readSequence에서 sequence table의 last_sync_seq를 update한다.
 *    항상 selectCurrValTx와 updateLastValTx가 순서대로 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qdsCallBackInfo  * sCallBack = (qdsCallBackInfo*) aInfo;
    mtdBigintType      sLastSyncSeqValue;
    ULong              sBuffer[2];
    mtdByteType      * sLastTimeValue = (mtdByteType*) sBuffer;
    UInt               sLastTimeValueSize;
    smiValue           sNewRow[2];
    SInt               sState = 4;
    
    IDE_ASSERT( sCallBack != NULL );
    
    // update
    sLastSyncSeqValue = (mtdBigintType)aLastVal;

    sLastTimeValueSize = MTD_BYTE_TYPE_STRUCT_SIZE(QC_BYTE_PRECISION_FOR_TIMESTAMP);
    IDE_DASSERT( ID_SIZEOF(sBuffer) >= sLastTimeValueSize );
    IDE_TEST( qmx::setTimeStamp( sLastTimeValue->value ) != IDE_SUCCESS );
    sLastTimeValue->length = QC_BYTE_PRECISION_FOR_TIMESTAMP;
    
    sNewRow[0].value = (void*) &sLastSyncSeqValue;
    sNewRow[0].length = ID_SIZEOF(mtdBigintType);
    
    sNewRow[1].value = (void*) sLastTimeValue;
    sNewRow[1].length = sLastTimeValueSize;
    
    IDE_TEST( sCallBack->mCursor.updateRow( sNewRow )
              != IDE_SUCCESS );
    
    sState = 3;
    IDE_TEST( sCallBack->mCursor.close() != IDE_SUCCESS );
    
    // statement close
    sState = 2;
    IDE_TEST( sCallBack->mSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );

    // transaction commit
    sState = 1;
    IDE_TEST( sCallBack->mSmiTrans.commit()
              != IDE_SUCCESS );

    // transaction destroy
    sState = 0;
    IDE_TEST( sCallBack->mSmiTrans.destroy( sCallBack->mStatistics )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 4:
            (void) sCallBack->mCursor.close();
        case 3:
            (void) sCallBack->mSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void) sCallBack->mSmiTrans.rollback();
        case 1:
            sCallBack->mSmiTrans.destroy( sCallBack->mStatistics );
            break;
    }

    return IDE_FAILURE;
}
