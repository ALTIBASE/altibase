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
 * $Id: qcd.cpp 9909 2004-11-29 00:30:56Z leekmo $
 **********************************************************************/

#include <cm.h>
#include <idl.h>
#include <ide.h>
#include <qcd.h>
#include <qsxEnv.h>

void
qcd::initStmt( QCD_HSTMT * aHstmt )
{
    // handle statement 초기화
    *aHstmt = NULL;
}

IDE_RC
qcd::allocStmt( qcStatement * aStatement,
                QCD_HSTMT   * aHstmt /* OUT */ )
{
/***********************************************************************
 *
 *  Description : alloc mmStatement
 *
 *  Implementation :
 *             session, parent mmStatement를 넘겨서 mmStatement를 할당받음
 *
 ***********************************************************************/

    qciSQLAllocStmtContext sContext;

    *aHstmt = NULL;

    sContext.mmSession = aStatement->session->mMmSession;
    sContext.mmStatement = NULL;
    sContext.mmParentStatement = QC_MM_STMT(aStatement);
    sContext.dedicatedMode = ID_FALSE;

    IDE_TEST( qci::mInternalSQLCallback.mAllocStmt( &sContext )
              != IDE_SUCCESS );

    *aHstmt = sContext.mmStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::prepare( QCD_HSTMT     aHstmt,
              qcStatement * aQcStmt,
              qcStatement * aExecQcStmt,
              qciStmtType * aStmtType,
              SChar       * aSqlString,
              UInt          aSqlStringLen,
              idBool        aExecMode )
{
/***********************************************************************
 *
 *  Description : prepare
 *
 *  Implementation : direct-execute모드로 prepare(execMode = ID_TRUE)
 *
 ***********************************************************************/

    qciSQLPrepareContext sContext;

    sContext.mmStatement = aHstmt;
    sContext.sqlString = aSqlString;
    sContext.sqlStringLen = aSqlStringLen;
    sContext.stmtType = QCI_STMT_MASK_MAX;
    sContext.execMode = aExecMode;

    if ( (aQcStmt != NULL) && (aExecQcStmt != NULL) )
    {
        aExecQcStmt->spxEnv->mCallDepth = aQcStmt->spxEnv->mCallDepth;
    }

    IDE_TEST( qci::mInternalSQLCallback.mPrepare( &sContext )
              != IDE_SUCCESS );

    *aStmtType = sContext.stmtType;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::bindParamInfoSet( QCD_HSTMT      aHstmt,
                       mtcColumn    * aColumn,
                       UShort         aBindId,
                       qsInOutType    aInOutType )
{
/***********************************************************************
 *
 *  Description : parameter info를 하나 bind
 *
 *  Implementation :
 *
 ***********************************************************************/

    qciSQLParamInfoContext sContext;

    sContext.mmStatement = aHstmt;

    makeBindParamInfo( &sContext.bindParam,
                       aColumn,
                       aBindId,
                       aInOutType );

    IDE_TEST( qci::mInternalSQLCallback.mBindParamInfo( &sContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::bindParamData( QCD_HSTMT      aHstmt,
                    void         * aValue,
                    UInt           aValueSize,
                    UShort         aBindId,
                    iduMemory    * aMemory,
                    qciBindData ** aBindDataList,
                    qsInOutType    aInOutType )
{
/***********************************************************************
 *
 *  Description : parameter data를 하나 bind
 *
 *  Implementation :
 *
 *      BUG-47695
 *        addBindDataList 함수의 기능을 포함하도록 수정함
 *
 ***********************************************************************/

    qciSQLParamDataContext sContext;
    qciBindData * sBindData;
    qciBindData * sLastBindData;

    sContext.mmStatement = aHstmt;

    QCD_MAKE_BIND_DATA( &sContext.bindParamData,
                        aValue,
                        aValueSize,
                        aBindId,
                        NULL);

    IDE_TEST( qci::mInternalSQLCallback.mBindParamData( &sContext )
              != IDE_SUCCESS );

    if ( (aInOutType != QS_IN) &&
         (aBindDataList != NULL) )
    {
        IDE_TEST( aMemory->alloc( ID_SIZEOF(qciBindData),
                                  (void**)&sBindData )
                  != IDE_SUCCESS );

        QCD_MAKE_BIND_DATA( sBindData,
                            aValue,
                            aValueSize,
                            aBindId,
                            NULL );

        if( *aBindDataList == NULL )
        {
            *aBindDataList = sBindData;
        }
        else
        {
            for( sLastBindData = *aBindDataList;
                 sLastBindData->next != NULL;
                 sLastBindData = sLastBindData->next ) ;

            sLastBindData->next = sBindData;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::execute( QCD_HSTMT     aHstmt,
              qcStatement * aQcStmt,
              qciBindData * aOutBindParamDataList,
              vSLong      * aAffectedRowCount, /* OUT */
              idBool      * aResultSetExist, /* OUT */
              idBool      * aRecordExist /* OUT */,
              idBool        aIsFirst )
{
/***********************************************************************
 *
 *  Description : execute
 *
 *  Implementation : execute호출 후 resultset이 있는지 여부, affected rowcount
 *                   를 저장
 *
 ***********************************************************************/

    qciSQLExecuteContext   sContext;

    // BUG-37467
    qcTemplate  * sTmplate        = QC_PRIVATE_TMPLATE(aQcStmt);
    mtcStack    * sTmpStackBuffer = sTmplate->tmplate.stackBuffer;
    mtcStack    * sTmpStack       = sTmplate->tmplate.stack;
    UInt          sTmpStackCount  = sTmplate->tmplate.stackCount;
    UInt          sTmpStackRemain = sTmplate->tmplate.stackRemain;
    qcStatement * sTmpQcStmt      = sTmplate->stmt;
    qcStatement * sExecQcStmt     = NULL;

    // BUG-42322
    IDE_TEST( qcd::getQcStmt( aHstmt,
                              &sExecQcStmt )
                  != IDE_SUCCESS );

    qsxEnv::copyStack( sExecQcStmt->spxEnv, aQcStmt->spxEnv );
    // BUG-45322 execute immediate로 재귀호출을 하면 서버가 비정상 종료합니다.
    sExecQcStmt->spxEnv->mCallDepth = aQcStmt->spxEnv->mCallDepth;

    /* BUG-45678 */
    sExecQcStmt->spxEnv->mFlag = aQcStmt->spxEnv->mFlag;

    // BUG-45990
    QSX_ENV_SET_STMT_FLAG( sExecQcStmt );

    sContext.mmStatement = aHstmt;
    sContext.outBindParamDataList = aOutBindParamDataList;
    sContext.recordExist = ID_FALSE;
    sContext.resultSetCount = 0;
    sContext.isFirst = aIsFirst;

    IDE_TEST( qci::mInternalSQLCallback.mExecute( &sContext )
              != IDE_SUCCESS );

    *aRecordExist = sContext.recordExist;

    if( sContext.resultSetCount == 0 )
    {
        *aResultSetExist = ID_FALSE;
    }
    else
    {
        *aResultSetExist = ID_TRUE;
    }

    *aAffectedRowCount = sContext.affectedRowCount;

    /* sTmplate의 stack 및 stmt 원복 */
    QC_CONNECT_TEMPLATE_STACK( sTmplate,
                               sTmpStackBuffer,
                               sTmpStack,
                               sTmpStackCount,
                               sTmpStackRemain );
    sTmplate->stmt = sTmpQcStmt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* sTmplate의 stack 및 stmt 원복 */
    QC_CONNECT_TEMPLATE_STACK( sTmplate,
                               sTmpStackBuffer,
                               sTmpStack,
                               sTmpStackCount,
                               sTmpStackRemain );
    sTmplate->stmt = sTmpQcStmt;

    return IDE_FAILURE;
}

IDE_RC
qcd::fetch( qcStatement * aQcStmt,
            iduMemory   * aMemory,
            QCD_HSTMT     aHstmt,
            qciBindData * aBindColumnDataList, /* OUT */
            idBool      * aNextRecordExist /* OUT */ )
{
/***********************************************************************
 *
 *  Description : 레코드 한건 fetch+fetchColumn
 *
 *  Implementation :
 *
 ***********************************************************************/

    qciSQLFetchContext sContext;

    // BUG-38694
    qcTemplate  * sTmplate        = QC_PRIVATE_TMPLATE(aQcStmt);
    mtcStack    * sTmpStackBuffer = sTmplate->tmplate.stackBuffer;
    mtcStack    * sTmpStack       = sTmplate->tmplate.stack;
    UInt          sTmpStackCount  = sTmplate->tmplate.stackCount;
    UInt          sTmpStackRemain = sTmplate->tmplate.stackRemain;
    qcStatement * sTmpQcStmt      = sTmplate->stmt;
    qcStatement * sExecQcStmt     = NULL;

    // BUG-42322
    IDE_TEST( qcd::getQcStmt( aHstmt,
                              &sExecQcStmt )
                  != IDE_SUCCESS );

    sExecQcStmt->spxEnv->mPrevStackInfo = aQcStmt->spxEnv->mPrevStackInfo;

    sContext.memory = aMemory;
    sContext.mmStatement = aHstmt;
    sContext.bindColumnDataList = aBindColumnDataList;
    sContext.nextRecordExist = ID_FALSE;

    IDE_TEST( qci::mInternalSQLCallback.mFetch( &sContext )
              != IDE_SUCCESS );

    *aNextRecordExist = sContext.nextRecordExist;

    /* sTmplate의 stack 및 stmt 원복 */
    QC_CONNECT_TEMPLATE_STACK( sTmplate,
                               sTmpStackBuffer,
                               sTmpStack,
                               sTmpStackCount,
                               sTmpStackRemain );
    sTmplate->stmt = sTmpQcStmt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* sTmplate의 stack 및 stmt 원복 */
    QC_CONNECT_TEMPLATE_STACK( sTmplate,
                               sTmpStackBuffer,
                               sTmpStack,
                               sTmpStackCount,
                               sTmpStackRemain );
    sTmplate->stmt = sTmpQcStmt;

    return IDE_FAILURE;
}

IDE_RC
qcd::freeStmt( QCD_HSTMT   aHstmt,
               idBool      aFreeMode )
{
/***********************************************************************
 *
 *  Description : free mmStatement (=close)
 *
 *  Implementation :
 *
 ***********************************************************************/

    qciSQLFreeStmtContext sContext;

    if( aHstmt == NULL )
    {
        // Nothing to do.
    }
    else
    {
        sContext.mmStatement = aHstmt;
        sContext.freeMode = aFreeMode;

        IDE_TEST( qci::mInternalSQLCallback.mFreeStmt( &sContext )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qcd::makeBindParamInfo( qciBindParam * aBindParam,
                        mtcColumn    * aColumn,
                        UShort         aBindId,
                        qsInOutType    aInOutType )
{
    aBindParam->id        = aBindId;
    aBindParam->name      = NULL;
    aBindParam->type      = aColumn->type.dataTypeId;
    aBindParam->language  = aColumn->type.languageId;
    aBindParam->arguments = ((aColumn->flag)&MTC_COLUMN_ARGUMENT_COUNT_MASK);
    aBindParam->precision = aColumn->precision;
    aBindParam->scale     = aColumn->scale;
    aBindParam->data      = NULL;
    aBindParam->dataSize  = 0;

    switch( aInOutType )
    {
        case QS_IN:
            aBindParam->inoutType = CMP_DB_PARAM_INPUT;
            break;

        case QS_OUT:
            aBindParam->inoutType = CMP_DB_PARAM_OUTPUT;
            break;

        case QS_INOUT:
            aBindParam->inoutType = CMP_DB_PARAM_INPUT_OUTPUT;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }
}

IDE_RC
qcd::addBindColumnDataList( iduMemory    * aMemory,
                            qciBindData ** aBindDataList,
                            mtcColumn    * aColumn,
                            void         * aData,
                            UShort         aBindId )
{
    qciBindData * sBindData;
    qciBindData * sLastBindData;

    IDE_DASSERT( NULL != aMemory );
    IDE_DASSERT( NULL != aColumn );

    IDE_TEST( aMemory->alloc( ID_SIZEOF(qciBindData),
                              (void**)&sBindData )
              != IDE_SUCCESS );

    QCD_MAKE_BIND_DATA( sBindData,
                        aData,
                        0,
                        aBindId,
                        aColumn );

    if( *aBindDataList == NULL )
    {
        *aBindDataList = sBindData;
    }
    else
    {
        for( sLastBindData = *aBindDataList;
             sLastBindData->next != NULL;
             sLastBindData = sLastBindData->next ) ;

        sLastBindData->next = sBindData;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::checkBindParamCount( QCD_HSTMT   aHstmt,
                          UShort      aBindParamCount )
{
    qciSQLCheckBindContext sContext;

    sContext.mmStatement = aHstmt;
    sContext.bindCount = aBindParamCount;

    IDE_TEST( qci::mInternalSQLCallback.mCheckBindParamCount( &sContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::checkBindColumnCount( QCD_HSTMT   aHstmt,
                           UShort      aBindColumnCount )
{
    qciSQLCheckBindContext sContext;

    sContext.mmStatement = aHstmt;
    sContext.bindCount = aBindColumnCount;

    IDE_TEST( qci::mInternalSQLCallback.mCheckBindColumnCount( &sContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qcd::getQcStmt( QCD_HSTMT      aHstmt,
                qcStatement ** aQcStmt )
{
    qciSQLGetQciStmtContext sContext;

    sContext.mmStatement = aHstmt;

    IDE_TEST( qci::mInternalSQLCallback.mGetQciStmt( &sContext )
              != IDE_SUCCESS );

    *aQcStmt = &(sContext.mQciStatement->statement);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::endFetch( QCD_HSTMT aHstmt )
{
    qciSQLFetchEndContext sContext;

    sContext.mmStatement = aHstmt;

    IDE_TEST( qci::mInternalSQLCallback.mEndFetch( &sContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-41248 DBMS_SQL package
IDE_RC
qcd::allocStmtNoParent( void        * aMmSession,
                        idBool        aDedicatedMode,
                        QCD_HSTMT   * aHstmt /* OUT */ )
{
    qciSQLAllocStmtContext sContext;

    *aHstmt = NULL;

    sContext.mmSession = aMmSession;
    sContext.mmStatement = NULL;
    sContext.mmParentStatement = NULL;
    sContext.dedicatedMode = aDedicatedMode;

    IDE_TEST( qci::mInternalSQLCallback.mAllocStmt( &sContext )
              != IDE_SUCCESS );

    *aHstmt = sContext.mmStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::executeNoParent( QCD_HSTMT     aHstmt,
                      qciBindData * aOutBindParamDataList,
                      vSLong      * aAffectedRowCount, /* OUT */
                      idBool      * aResultSetExist, /* OUT */
                      idBool      * aRecordExist /* OUT */,
                      idBool        aIsFirst )
{
/***********************************************************************
 *
 *  Description : execute
 *
 *  Implementation : execute호출 후 resultset이 있는지 여부, affected rowcount
 *                   를 저장
 *
 ***********************************************************************/

    qciSQLExecuteContext   sContext;

    sContext.mmStatement = aHstmt;
    sContext.outBindParamDataList = aOutBindParamDataList;
    sContext.recordExist = ID_FALSE;
    sContext.resultSetCount = 0;
    sContext.isFirst = aIsFirst;

    IDE_TEST( qci::mInternalSQLCallback.mExecute( &sContext )
              != IDE_SUCCESS );

    *aRecordExist = sContext.recordExist;

    if( sContext.resultSetCount == 0 )
    {
        *aResultSetExist = ID_FALSE;
    }
    else
    {
        *aResultSetExist = ID_TRUE;
    }

    *aAffectedRowCount = sContext.affectedRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::bindParamInfoSetByName( QCD_HSTMT      aHstmt,
                             mtcColumn    * aColumn,
                             SChar        * aBindName,
                             qsInOutType    aInOutType )
{
/***********************************************************************
 *
 *  Description : parameter info를 하나 bind
 *
 *  Implementation :
 *
 ***********************************************************************/

    qciSQLParamInfoContext sContext;

    sContext.mmStatement = aHstmt;

    makeBindParamInfoByName( &sContext.bindParam,
                             aColumn,
                             aBindName,
                             aInOutType );

    IDE_TEST( qci::mInternalSQLCallback.mBindParamInfo( &sContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcd::bindParamDataByName( QCD_HSTMT     aHstmt,
                          void        * aValue,
                          UInt          aValueSize,
                          SChar       * aBindName)
{
/***********************************************************************
 *
 *  Description : parameter data를 하나 bind
 *
 *  Implementation :
 *
 ***********************************************************************/

    qciSQLParamDataContext sContext;

    sContext.mmStatement = aHstmt;

    makeBindDataByName( &sContext.bindParamData,
                        aValue,
                        aValueSize,
                        aBindName,
                        NULL );

    IDE_TEST( qci::mInternalSQLCallback.mBindParamData( &sContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qcd::makeBindParamInfoByName( qciBindParam * aBindParam,
                              mtcColumn    * aColumn,
                              SChar        * aBindName,
                              qsInOutType    aInOutType )
{
    aBindParam->id        = ID_USHORT_MAX;
    aBindParam->name      = aBindName;
    aBindParam->type      = aColumn->type.dataTypeId;
    aBindParam->language  = aColumn->type.languageId;
    aBindParam->arguments = ((aColumn->flag)&MTC_COLUMN_ARGUMENT_COUNT_MASK);
    aBindParam->precision = aColumn->precision;
    aBindParam->scale     = aColumn->scale;
    aBindParam->data      = NULL;
    aBindParam->dataSize  = 0;

    switch( aInOutType )
    {
        case QS_IN:
            aBindParam->inoutType = CMP_DB_PARAM_INPUT;
            break;

        case QS_OUT:
            aBindParam->inoutType = CMP_DB_PARAM_OUTPUT;
            break;

        case QS_INOUT:
            aBindParam->inoutType = CMP_DB_PARAM_INPUT_OUTPUT;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }
}

void
qcd::makeBindDataByName( qciBindData * aBindData,
                         void        * aValue,
                         UInt          aValueSize,
                         SChar       * aBindName,
                         mtcColumn   * aColumn)
{
    aBindData->id = ID_USHORT_MAX;
    aBindData->name = aBindName;
    aBindData->column = aColumn;
    aBindData->data = aValue;
    aBindData->size = aValueSize;
    aBindData->next = NULL;
}
