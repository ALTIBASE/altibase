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
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdnCheck.h>
#include <qcuSqlSourceInfo.h>
#include <qcmCache.h>
#include <qcmPartition.h>
#include <qdbCommon.h>
#include <qdbAlter.h>
#include <qdn.h>
#include <qcpUtil.h>

IDE_RC qdnCheck::makeColumnNameListFromExpression(
                qcStatement         * aStatement,
                qcNamePosList      ** aColumnNameList,
                qtcNode             * aNode,
                qcNamePosition        aColumnName )
{
/***********************************************************************
 *
 * Description :
 *  Parsing 단계의 expression을 대상으로, 특정 컬럼의 Column Name List를 만든다.
 *      - subquery를 지원하지 않는다.
 *
 * Implementation :
 *  (1) qtc::makeColumn()에서 지정한 항목으로 Column인지 확인하고,
 *      Column Name이 일치하면 Column Name List를 구성한다.
 *  (2) arguments를 Recursive Call
 *  (3) next를 Recursive Call
 *
 ***********************************************************************/

    qcNamePosList * sColumnName;
    qcNamePosList * sLastColumnName = NULL;
    qcNamePosList * sNewColumnName  = NULL;

    /* qtc::makeColumn()에서 지정한 항목으로 Column인지 확인하고 */
    if ( (QC_IS_NULL_NAME( aNode->columnName ) == ID_FALSE) &&
         (aNode->node.module == &qtc::columnModule) )
    {
        /* Column Name이 일치하면 Column Name List를 구성한다. */
        if ( QC_IS_NAME_MATCHED( aNode->columnName, aColumnName ) )
        {
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qcNamePosList, &sNewColumnName )
                      != IDE_SUCCESS );
            SET_POSITION( sNewColumnName->namePos, aNode->columnName );
            sNewColumnName->next = NULL;

            if ( *aColumnNameList == NULL )
            {
                *aColumnNameList = sNewColumnName;
            }
            else
            {
                for ( sColumnName = *aColumnNameList;
                      sColumnName != NULL;
                      sColumnName = sColumnName->next )
                {
                    sLastColumnName = sColumnName;
                }
                sLastColumnName->next = sNewColumnName;
            }
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

    /* arguments를 Recursive Call */
    if ( aNode->node.arguments != NULL )
    {
        IDE_TEST( makeColumnNameListFromExpression(
                        aStatement,
                        aColumnNameList,
                        (qtcNode *)aNode->node.arguments,
                        aColumnName )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* next를 Recursive Call */
    if ( aNode->node.next != NULL )
    {
        IDE_TEST( makeColumnNameListFromExpression(
                        aStatement,
                        aColumnNameList,
                        (qtcNode *)aNode->node.next,
                        aColumnName )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdnCheck::renameColumnInExpression(
                qcStatement         * aStatement,
                SChar              ** aNewExpressionStr,
                qtcNode             * aNode,
                qcNamePosition        aOldColumnName,
                qcNamePosition        aNewColumnName )
{
/***********************************************************************
 *
 * Description :
 *  expression 내의 컬럼 이름을 변경하여, 새로운 expression 문자열을 만든다.
 *      - 따옴표를 이용한 컬럼 이름을 지원한다.
 *      - subquery를 지원하지 않는다.
 *
 * Implementation :
 *  (1) 따옴표를 고려하여, 새로운 컬럼 이름의 Offset과 Size를 구한다.
 *  (2) 이전 컬럼 이름으로 expression에서 Column Name List를 얻는다.
 *  (3) 새로운 expression 문자열의 크기를 구하고 메모리를 할당한다.
 *  (4) 새로운 expression을 만든다. (아래 반복)
 *    a. 따옴표를 고려하여, 이전 컬럼 이름의 Offset과 Size를 구한다.
 *    b. 이전 지점부터 현재 지점까지 복사한다.
 *    c. 새로운 컬럼 이름을 복사한다.
 *  (5) 마지막 컬럼 이름 이후의, 남은 expression 문자열을 복사한다.
 ***********************************************************************/

    qcNamePosList * sColumnNameList = NULL;
    qcNamePosList * sColumnName;
    SChar         * sNewExpressionStr = NULL;
    SInt            sNewExpressionStrSize;
    SInt            sNextCopyOffset;
    SInt            sOldColumnNameOffset;
    SInt            sOldColumnNameSize;
    SInt            sNewColumnNameOffset;
    SInt            sNewColumnNameSize;

    /* 따옴표를 고려하여, 새로운 컬럼 이름의 Offset과 Size를 구한다. */
    if ( (aNewColumnName.offset > 0) &&
         (aNewColumnName.size > 0) )
    {
        if ( aNewColumnName.stmtText[aNewColumnName.offset - 1] == '"' )
        {
            sNewColumnNameOffset = aNewColumnName.offset - 1;
            sNewColumnNameSize   = aNewColumnName.size + 2;
        }
        else
        {
            sNewColumnNameOffset = aNewColumnName.offset;
            sNewColumnNameSize   = aNewColumnName.size;
        }
    }
    else
    {
        sNewColumnNameOffset = aNewColumnName.offset;
        sNewColumnNameSize   = aNewColumnName.size;
    }

    /* 이전 컬럼 이름으로 expression에서 Column Name List를 얻는다. */
    IDE_TEST( makeColumnNameListFromExpression( aStatement,
                                                &sColumnNameList,
                                                aNode,
                                                aOldColumnName )
              != IDE_SUCCESS );

    if ( sColumnNameList != NULL )
    {
        /* 새로운 expression 문자열의 크기를 구하고 메모리를 할당한다. (NULL 포함) */
        sNewExpressionStrSize = aNode->position.size;
        for ( sColumnName = sColumnNameList;
              sColumnName != NULL;
              sColumnName = sColumnName->next )
        {
            if ( (sColumnName->namePos.offset > 0) &&
                 (sColumnName->namePos.size > 0) )
            {
                if ( sColumnName->namePos.stmtText[sColumnName->namePos.offset - 1] == '"' )
                {
                    sOldColumnNameSize = sColumnName->namePos.size + 2;
                }
                else
                {
                    sOldColumnNameSize = sColumnName->namePos.size;
                }
            }
            else
            {
                sOldColumnNameSize = sColumnName->namePos.size;
            }

            sNewExpressionStrSize += sNewColumnNameSize - sOldColumnNameSize;
        }
        sNewExpressionStrSize++;

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sNewExpressionStrSize,
                                                 (void**)&sNewExpressionStr )
                  != IDE_SUCCESS );
        idlOS::memset( sNewExpressionStr, 0x00, sNewExpressionStrSize );

        /* 새로운 expression을 만든다. */
        sNextCopyOffset = aNode->position.offset;
        for ( sColumnName = sColumnNameList;
              sColumnName != NULL;
              sColumnName = sColumnName->next )
        {
            /* 따옴표를 고려하여, 이전 컬럼 이름의 Offset과 Size를 구한다. */
            if ( (sColumnName->namePos.offset > 0) &&
                 (sColumnName->namePos.size > 0) )
            {
                if ( sColumnName->namePos.stmtText[sColumnName->namePos.offset - 1] == '"' )
                {
                    sOldColumnNameOffset = sColumnName->namePos.offset - 1;
                    sOldColumnNameSize   = sColumnName->namePos.size + 2;
                }
                else
                {
                    sOldColumnNameOffset = sColumnName->namePos.offset;
                    sOldColumnNameSize   = sColumnName->namePos.size;
                }
            }
            else
            {
                sOldColumnNameOffset = sColumnName->namePos.offset;
                sOldColumnNameSize   = sColumnName->namePos.size;
            }

            /* 이전 지점부터 현재 지점까지 복사하고, 다음에 복사할 지점을 설정한다. */
            if ( sNextCopyOffset < sOldColumnNameOffset )
            {
                (void)idlVA::appendString( sNewExpressionStr,
                                           sNewExpressionStrSize,
                                           &(aNode->position.stmtText[sNextCopyOffset]),
                                           (UInt)(sOldColumnNameOffset - sNextCopyOffset) );
            }
            else
            {
                /* Nothing to do */
            }
            sNextCopyOffset = sOldColumnNameOffset + sOldColumnNameSize;

            /* 새로운 컬럼 이름을 복사한다. */
            (void)idlVA::appendString( sNewExpressionStr,
                                       sNewExpressionStrSize,
                                       &(aNewColumnName.stmtText[sNewColumnNameOffset]),
                                       (UInt)sNewColumnNameSize );
        }

        /* 마지막 컬럼 이름 이후의, 남은 expression 문자열을 복사한다. */
        if ( sNextCopyOffset < (aNode->position.offset + aNode->position.size) )
        {
            (void)idlVA::appendString( sNewExpressionStr,
                                       sNewExpressionStrSize,
                                       &(aNode->position.stmtText[sNextCopyOffset]),
                                       (UInt)(aNode->position.offset
                                            + aNode->position.size
                                            - sNextCopyOffset) );
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

    *aNewExpressionStr = sNewExpressionStr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdnCheck::addCheckConstrSpecRelatedToColumn(
                qcStatement         * aStatement,
                qdConstraintSpec   ** aConstrSpec,
                qcmCheck            * aChecks,
                UInt                  aCheckCount,
                UInt                  aColumnID )
{
/***********************************************************************
 *
 * Description :
 *  특정 Column과 관련된 Check Constraint Spec List를 얻는다.
 *
 * Implementation :
 *  각 Check Constraint에 대해 아래를 반복한다.
 *  1. Check Constraint에 특정 Column이 있는지 확인한다.
 *  2. Check Constraint Spec List에 이미 있는 Check Constraint인지 확인한다.
 *  3. Check Constraint Spec List의 마지막에 추가한다.
 ***********************************************************************/

    qcmCheck         * sCheck;
    qdConstraintSpec * sCurrConstrSpec;
    qdConstraintSpec * sLastConstrSpec;
    qdConstraintSpec * sNewConstrSpec;
    qtcNode          * sNode[2];
    UInt               i;

    for ( i = 0; i < aCheckCount; i++ )
    {
        sCheck = &(aChecks[i]);

        if ( qdn::intersectColumn( sCheck->constraintColumn,
                                   sCheck->constraintColumnCount,
                                   &aColumnID,
                                   1 )
             != ID_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        /* 중복을 확인한다. */
        sLastConstrSpec = NULL;
        for ( sCurrConstrSpec = *aConstrSpec;
              sCurrConstrSpec != NULL;
              sCurrConstrSpec = sCurrConstrSpec->next )
        {
            if ( idlOS::strMatch( sCurrConstrSpec->constrName.stmtText + sCurrConstrSpec->constrName.offset,
                                  sCurrConstrSpec->constrName.size,
                                  sCheck->name,
                                  idlOS::strlen( sCheck->name ) ) == 0 )
            {
                break;
            }
            else
            {
                sLastConstrSpec = sCurrConstrSpec;
            }
        }

        if ( sCurrConstrSpec != NULL )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                qdConstraintSpec,
                                &sNewConstrSpec )
                  != IDE_SUCCESS );
        QD_SET_INIT_CONSTRAINT_SPEC( sNewConstrSpec );

        sNewConstrSpec->constrName.stmtText = sCheck->name;
        sNewConstrSpec->constrName.offset   = 0;
        sNewConstrSpec->constrName.size     = idlOS::strlen( sCheck->name );
        sNewConstrSpec->constrType          = QD_CHECK;
        sNewConstrSpec->constrColumnCount   = sCheck->constraintColumnCount;

        /* get_condition_statement가 DEFAULT를 기준으로 작성되어 있으므로,
         * DEFAULT의 expression을 얻는 함수를 호출한다.
         */
        IDE_TEST( qcpUtil::makeDefaultExpression(
                                aStatement,
                                sNode,
                                sCheck->checkCondition,
                                idlOS::strlen( sCheck->checkCondition ) )
                  != IDE_SUCCESS );
        sNewConstrSpec->checkCondition = sNode[0];

        /* adjust expression position */
        sNewConstrSpec->checkCondition->position.offset = 7; /* "RETURN " */
        sNewConstrSpec->checkCondition->position.size   = idlOS::strlen( sCheck->checkCondition );

        /* make constraint column */
        IDE_TEST( qcpUtil::makeConstraintColumnsFromExpression(
                                aStatement,
                                &(sNewConstrSpec->constraintColumns),
                                sNewConstrSpec->checkCondition )
                  != IDE_SUCCESS );

        if ( *aConstrSpec == NULL )
        {
            *aConstrSpec = sNewConstrSpec;
        }
        else
        {
            sLastConstrSpec->next = sNewConstrSpec;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdnCheck::makeCheckConstrSpecRelatedToTable(
                qcStatement         * aStatement,
                qdConstraintSpec   ** aConstrSpec,
                qcmCheck            * aChecks,
                UInt                  aCheckCount )
{
/***********************************************************************
 *
 * Description :
 *  Table과 관련된 Check Constraint Spec List를 얻는다.
 *
 * Implementation :
 *  1. qcmCheck 배열을 qdConstraintSpec List로 변환한다.
 ***********************************************************************/

    qcmCheck         * sCheck;
    qdConstraintSpec * sLastConstrSpec = NULL;
    qdConstraintSpec * sNewConstrSpec;
    qtcNode          * sNode[2];
    UInt               i;

    *aConstrSpec = NULL;

    for ( i = 0; i < aCheckCount; i++ )
    {
        sCheck = &(aChecks[i]);

        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                qdConstraintSpec,
                                &sNewConstrSpec )
                  != IDE_SUCCESS );
        QD_SET_INIT_CONSTRAINT_SPEC( sNewConstrSpec );

        sNewConstrSpec->constrName.stmtText = sCheck->name;
        sNewConstrSpec->constrName.offset   = 0;
        sNewConstrSpec->constrName.size     = idlOS::strlen( sCheck->name );
        sNewConstrSpec->constrType          = QD_CHECK;
        sNewConstrSpec->constrColumnCount   = sCheck->constraintColumnCount;

        /* get_condition_statement가 DEFAULT를 기준으로 작성되어 있으므로,
         * DEFAULT의 expression을 얻는 함수를 호출한다.
         */
        IDE_TEST( qcpUtil::makeDefaultExpression(
                                aStatement,
                                sNode,
                                sCheck->checkCondition,
                                idlOS::strlen( sCheck->checkCondition ) )
                  != IDE_SUCCESS );
        sNewConstrSpec->checkCondition = sNode[0];

        /* adjust expression position */
        sNewConstrSpec->checkCondition->position.offset = 7; /* "RETURN " */
        sNewConstrSpec->checkCondition->position.size   = idlOS::strlen( sCheck->checkCondition );

        /* make constraint column */
        IDE_TEST( qcpUtil::makeConstraintColumnsFromExpression(
                                aStatement,
                                &(sNewConstrSpec->constraintColumns),
                                sNewConstrSpec->checkCondition )
                  != IDE_SUCCESS );

        if ( *aConstrSpec == NULL )
        {
            *aConstrSpec = sNewConstrSpec;
        }
        else
        {
            sLastConstrSpec->next = sNewConstrSpec;
        }

        sLastConstrSpec = sNewConstrSpec;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdnCheck::renameColumnInCheckConstraint(
                qcStatement      * aStatement,
                qdConstraintSpec * aConstrList,
                qcmTableInfo     * aTableInfo,
                qcmColumn        * aOldColumn,
                qcmColumn        * aNewColumn )
{
/***********************************************************************
 *
 * Description :
 *  모든 Check Constraint의 Column Name을 변경한다.
 *
 * Implementation :
 *  각 Check Constraint에 대해 아래를 반복한다.
 *  (1) 새로운 Column Name으로 변경한 Check Condition을 구한다.
 *  (2) SYS_CONSTRAINTS_ 메타 테이블의 CHECK_CONDITION 컬럼을 변경한다.
 *
 ***********************************************************************/

    qdConstraintSpec   * sConstr;
    qcmCheck           * sCheck;
    SChar              * sCheckCondition = NULL;
    UInt                 i;

    for ( sConstr = aConstrList;
          sConstr != NULL;
          sConstr = sConstr->next )
    {
        if ( sConstr->constrType == QD_CHECK )
        {
            for ( i = 0; i < aTableInfo->checkCount; i++ )
            {
                sCheck = &(aTableInfo->checks[i]);

                if ( idlOS::strMatch( sConstr->constrName.stmtText + sConstr->constrName.offset,
                                      sConstr->constrName.size,
                                      sCheck->name,
                                      idlOS::strlen( sCheck->name ) ) == 0 )
                {
                    IDE_TEST( renameColumnInExpression(
                                            aStatement,
                                            &sCheckCondition,
                                            sConstr->checkCondition,
                                            aOldColumn->namePos,
                                            aNewColumn->namePos )
                              != IDE_SUCCESS );

                    IDE_TEST( qdbAlter::updateCheckCondition(
                                            aStatement,
                                            sCheck->constraintID,
                                            sCheckCondition )
                              != IDE_SUCCESS );
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdnCheck::setMtcColumnToCheckConstrList(
                qcStatement      * aStatement,
                qcmTableInfo     * aTableInfo,
                qdConstraintSpec * aConstrList )
{
/***********************************************************************
 *
 * Description :
 *  Check Constraint List의 Constraint Column에 mtcColumn를 설정한다.
 *
 * Implementation :
 *  각 Constraint에 대해 아래를 반복한다.
 *  1. Check Constraint이면, Constraint Column에 mtcColumn를 설정한다.
 *
 ***********************************************************************/

    qdConstraintSpec   * sConstr;
    qcmColumn          * sColumn;
    qcmColumn          * sColumnInfo;

    for ( sConstr = aConstrList;
          sConstr != NULL;
          sConstr = sConstr->next )
    {
        if ( sConstr->constrType == QD_CHECK )
        {
            for ( sColumn = sConstr->constraintColumns;
                  sColumn != NULL;
                  sColumn = sColumn->next )
            {
                IDE_TEST( qcmCache::getColumn( aStatement,
                                               aTableInfo,
                                               sColumn->namePos,
                                               &sColumnInfo )
                          != IDE_SUCCESS );

                sColumn->basicInfo = sColumnInfo->basicInfo;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdnCheck::verifyCheckConstraintList(
                qcTemplate       * aTemplate,
                qdConstraintSpec * aConstrList )
{
/***********************************************************************
 *
 * Description :
 *  데이터가 Check Constraint 제약 조건을 만족하는지 검사한다.
 *
 * Implementation :
 *  각 Check Constraint에 대해 아래를 반복한다.
 *  1. qtc::calculate()
 *  2. NULL 또는 TRUE이면 성공, FALSE이면 실패
 *
 ***********************************************************************/

    qdConstraintSpec    * sConstr = NULL;
    mtcColumn           * sColumn = NULL;
    void                * sValue  = NULL;
    idBool                sIsTrue = ID_FALSE;
    SChar                 sConstrName[QC_MAX_OBJECT_NAME_LEN + 1] = { '\0', };

    for ( sConstr = aConstrList;
          sConstr != NULL;
          sConstr = sConstr->next )
    {
        if ( sConstr->constrType == QD_CHECK )
        {
            IDE_TEST_RAISE( qtc::calculate( sConstr->checkCondition, aTemplate )
                            != IDE_SUCCESS, ERR_INVALID_EXPRESSION );

            sColumn = aTemplate->tmplate.stack[0].column;
            sValue  = aTemplate->tmplate.stack[0].value;

            if ( sColumn->module->isNull( sColumn,
                                          sValue ) == ID_TRUE )
            {
                /* Nothing to do */
            }
            else
            {
                IDE_TEST_RAISE( sColumn->module->isTrue( &sIsTrue,
                                                          sColumn,
                                                          sValue )
                                != IDE_SUCCESS, ERR_INVALID_EXPRESSION );

                IDE_TEST_RAISE( sIsTrue == ID_FALSE,
                                ERR_VIOLATE_CHECK_CONDITION );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_EXPRESSION )
    {
        if ( QC_IS_NULL_NAME( sConstr->constrName ) != ID_TRUE )
        {
            QC_STR_COPY( sConstrName, sConstr->constrName );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDN_INVALID_CHECK_CONSTRAINT_EXPRESSION,
                                  sConstrName ) );
    }
    IDE_EXCEPTION( ERR_VIOLATE_CHECK_CONDITION )
    {
        if ( QC_IS_NULL_NAME( sConstr->constrName ) != ID_TRUE )
        {
            QC_STR_COPY( sConstrName, sConstr->constrName );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDN_VIOLATE_CHECK_CONSTRAINT,
                                  sConstrName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdnCheck::verifyCheckConstraintListWithFullTableScan(
                qcStatement      * aStatement,
                qmsTableRef      * aTableRef,
                qdConstraintSpec * aCheckConstrList )
{
/***********************************************************************
 *
 * Description : PROJ-1107 Check Constraint 지원
 *  이미 생성된 Table에 Check Constraint 제약 조건을 추가할 때,
 *  기존의 Data에 대하여 check condition을 검사해야 한다.
 *
 * Implementation :
 *  1. 사전 작업
 *      - 파라미터 확인
 *      - 변수 초기화
 *      - 커서 초기화
 *      - Record 공간 확보 및 fetch column list 구성 (Disk Table)
 *  2. 각 파티션에 대해 Check Constraint 검사
 *      - Table Full Scan
 *      - Check Condition 연산 및 결과 확인
 *
 ***********************************************************************/

    qcTemplate           * sTemplate;
    mtcTuple             * sTableTuple     = NULL;
    mtcTuple             * sPartitionTuple = NULL;
    mtcTuple               sCopyTuple;
    iduMemoryStatus        sQmxMemStatus;

    // Cursor를 위한 지역변수
    smiTableCursor         sReadCursor;
    smiCursorProperties    sCursorProperty;
    idBool                 sCursorOpen = ID_FALSE;

    idBool                 sNeedToRecoverTuple = ID_FALSE;

    // Record 검색을 위한 지역 변수
    UInt                   sPartType = 0;
    UInt                   sRowSize  = 0;
    void                 * sTupleRow = NULL;
    void                 * sDiskRow  = NULL;

    qmsPartitionRef      * sPartitionRefList = NULL;
    qmsPartitionRef      * sPartitionRefNode = NULL;
    qmsPartitionRef        sFakePartitionRef;
    qcmTableInfo         * sPartInfo;
    qcmTableInfo         * sDiskPartInfo = NULL;
    smiFetchColumnList   * sFetchColumnList;

    //---------------------------------------------
    // 적합성 검사
    //---------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableRef != NULL );
    IDE_DASSERT( aCheckConstrList != NULL );
    IDE_DASSERT( aCheckConstrList->constrType == QD_CHECK );
    IDE_DASSERT( aCheckConstrList->checkCondition != NULL );

    //---------------------------------------------
    // 초기화
    //---------------------------------------------

    sTemplate = QC_PRIVATE_TMPLATE(aStatement);
    sTableTuple = & sTemplate->tmplate.rows[aTableRef->table];

    // PROJ-1502 PARTITIONED DISK TABLE
    if ( aTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        sPartitionRefList = aTableRef->partitionRef;
    }
    else
    {
        sFakePartitionRef.partitionInfo = aTableRef->tableInfo;
        sFakePartitionRef.table         = aTableRef->table;
        sFakePartitionRef.next          = NULL;

        sPartitionRefList = &sFakePartitionRef;
    }

    //---------------------------------------------
    // Check Condition 제약 조건 검사
    //---------------------------------------------

    //----------------------------
    // 커서 초기화
    //----------------------------

    // To fix BUG-14818
    sReadCursor.initialize();

    //----------------------------
    // Record 공간 확보
    //----------------------------

    for ( sPartitionRefNode = sPartitionRefList;
          sPartitionRefNode != NULL;
          sPartitionRefNode = sPartitionRefNode->next )
    {
        sPartInfo = sPartitionRefNode->partitionInfo;
        sPartType = sPartInfo->tableFlag & SMI_TABLE_TYPE_MASK;

        if ( sPartType == SMI_TABLE_DISK )
        {
            sDiskPartInfo = sPartInfo;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sDiskPartInfo != NULL )
    {
        // Disk Table인 경우
        // Record Read를 위한 공간을 할당한다.
        // To Fix BUG-12977 : parent의 rowsize가 아닌, 자신의 rowsize를
        //                    가지고 와야함
        IDE_TEST( qdbCommon::getDiskRowSize( sDiskPartInfo,
                                             & sRowSize )
                  != IDE_SUCCESS );

        // To fix BUG-14820
        // Disk-variable 컬럼의 rid비교를 위해 초기화 해야 함.
        IDE_TEST( aStatement->qmxMem->cralloc( sRowSize,
                                               (void **) & sDiskRow )
                  != IDE_SUCCESS );

        //--------------------------------------
        // PROJ-1705 fetch column list 구성
        //--------------------------------------

        // fetch column list를 초기화한다.
        qdbCommon::initFetchColumnList( & sFetchColumnList );

        IDE_TEST( qdbCommon::addCheckConstrListToFetchColumnList(
                        aStatement->qmxMem,
                        aCheckConstrList,
                        sDiskPartInfo->columns,
                        &sFetchColumnList )
                  != IDE_SUCCESS );
    }
    else
    {
        // Memory Table인 경우
        // Nothing to do.
    }

    /* PROJ-2464 hybrid partitioned table 지원 */
    qmx::copyMtcTupleForPartitionDML( &sCopyTuple, sTableTuple );
    sNeedToRecoverTuple = ID_TRUE;

    sTupleRow = sTableTuple->row;

    for ( sPartitionRefNode = sPartitionRefList;
          sPartitionRefNode != NULL;
          sPartitionRefNode = sPartitionRefNode->next )
    {
        sPartInfo = sPartitionRefNode->partitionInfo;
        sPartType = sPartInfo->tableFlag & SMI_TABLE_TYPE_MASK;

        /* PROJ-2464 hybrid partitioned table 지원 */
        sPartitionTuple = &(sTemplate->tmplate.rows[sPartitionRefNode->table]);

        qmx::adjustMtcTupleForPartitionDML( sTableTuple, sPartitionTuple );

        SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sCursorProperty, aStatement->mStatistics );

        if ( sPartType == SMI_TABLE_DISK )
        {
            sCursorProperty.mFetchColumnList = sFetchColumnList;
            sTableTuple->row = sDiskRow;
        }
        else
        {
            sTableTuple->row = NULL;
        }

        IDE_TEST( sReadCursor.open( QC_SMI_STMT( aStatement ),
                                    sPartInfo->tableHandle,
                                    NULL,
                                    smiGetRowSCN( sPartInfo->tableHandle ),
                                    NULL,
                                    smiGetDefaultKeyRange(),
                                    smiGetDefaultKeyRange(),
                                    smiGetDefaultFilter(),
                                    SMI_LOCK_READ|
                                    SMI_TRAVERSE_FORWARD|
                                    SMI_PREVIOUS_DISABLE,
                                    SMI_SELECT_CURSOR,
                                    & sCursorProperty )
                  != IDE_SUCCESS );

        sCursorOpen = ID_TRUE;

        IDE_TEST( sReadCursor.beforeFirst() != IDE_SUCCESS );

        //----------------------------
        // 반복 검사
        //----------------------------

        IDE_TEST( sReadCursor.readRow( (const void**) & (sTableTuple->row),
                                       & sTableTuple->rid,
                                       SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        while ( sTableTuple->row != NULL )
        {
            // Memory 재사용을 위하여 현재 위치 기록
            IDE_TEST_RAISE( aStatement->qmxMem->getStatus( & sQmxMemStatus ) != IDE_SUCCESS, ERR_MEM_OP );

            // Check Condition 연산 및 결과 확인
            IDE_TEST( verifyCheckConstraintList( sTemplate, aCheckConstrList )
                      != IDE_SUCCESS );

            // Memory 재사용을 위한 Memory 이동
            IDE_TEST_RAISE( aStatement->qmxMem->setStatus( & sQmxMemStatus ) != IDE_SUCCESS, ERR_MEM_OP );

            IDE_TEST( sReadCursor.readRow( (const void**) & (sTableTuple->row),
                                           & sTableTuple->rid,
                                           SMI_FIND_NEXT )
                      != IDE_SUCCESS );
        }

        sCursorOpen = ID_FALSE;

        IDE_TEST( sReadCursor.close() != IDE_SUCCESS );
    }

    /* PROJ-2464 hybrid partitioned table 지원 */
    sNeedToRecoverTuple = ID_FALSE;
    qmx::copyMtcTupleForPartitionDML( sTableTuple, &sCopyTuple );

    sTableTuple->row = sTupleRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEM_OP )
    {
        ideLog::log( IDE_ERR_0,
                     "Unexpected errors may have occurred:"
                     " qdnCheck::verifyCheckConstraintListWithFullTableScan"
                     " memory error" );
    }
    IDE_EXCEPTION_END;

    if ( sCursorOpen == ID_TRUE )
    {
        (void) sReadCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2464 hybrid partitioned table 지원 */
    if ( sNeedToRecoverTuple == ID_TRUE )
    {
        qmx::copyMtcTupleForPartitionDML( sTableTuple, &sCopyTuple );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}
