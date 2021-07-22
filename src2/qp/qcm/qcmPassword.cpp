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
 * $Id: qcmPasswHistory.cpp 52008 2012-03-07 05:49:13Z jakejang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcm.h>
#include <qcg.h>
#include <qcmPassword.h>
#include <qcmUser.h>

const void * gQcmPasswordHistory;
const void * gQcmPasswordHistoryIndex[QCM_MAX_META_INDICES];

IDE_RC qcmReuseVerify::addPasswHistory( qcStatement * aStatement,
                                        UChar       * aUserPasswd,
                                        UInt          aUserID )
{

/***********************************************************************
 *
 * Description : PROJ-2207 Password policy support
 *      SYS_PASSWORD_HISTROY_ Record 삽입
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar          sSqlStr[QD_MAX_SQL_LENGTH];
    vSLong         sRowCnt;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_PASSWORD_HISTORY_ "
                     "(USER_ID, PASSWORD, PASSWORD_DATE) "
                     "VALUES ( "
                     "INTEGER'%"ID_UINT64_FMT"', "
                     "VARCHAR'%s', "
                     "SYSDATE ) ",
                     aUserID,
                     aUserPasswd );
    
    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                &sRowCnt )
             != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC  qcmReuseVerify::updatePasswdDate( qcStatement * aStatement,
                                          UChar       * aUserPasswd,
                                          UInt          aUserID )
{
/***********************************************************************
 *
 * Description : PROJ-2207 Password policy support
 *
 * Implementation :
 *        
 *         SYS_PASSWORD_HISTORY_ 의 PASSWORD_DATE 갱신
 *         
 ***********************************************************************/

    SChar          sSqlStr[QD_MAX_SQL_LENGTH];
    vSLong         sRowCnt;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_PASSWORD_HISTORY_ "
                     "SET "
                     "PASSWORD_DATE = SYSDATE "
                     "WHERE "
                     "USER_ID = INTEGER'%"ID_UINT64_FMT"' "
                     "AND PASSWORD = VARCHAR'%s' ",
                     aUserID,
                     aUserPasswd );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                &sRowCnt )
             != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmReuseVerify::checkReusePasswd( qcStatement * aStatement,
                                         UInt          aUserID,
                                         UChar       * aUserPasswd,
                                         idBool      * aExist)
{
/***********************************************************************
 *
 * Description : PROJ-2207 Password policy support
 *           SYS_PASSWORD_HISTORY_에 패스워드 존재 여부 체크         
 *        
 * Implementation :
 *
 ***********************************************************************/
    vSLong              sRowCount = 0;
    smiRange            sRange;
    qtcMetaRangeColumn  sFirstMetaRange;
    qtcMetaRangeColumn  sSecondMetaRange;
    ULong               sUserPasswdBuffer[ (ID_SIZEOF(UShort) + IDS_MAX_PASSWORD_BUFFER_LEN + 7) / 8 ];
    mtdCharType       * sUserPasswd;
    mtdIntegerType      sUserID;
    mtcColumn         * sFirstMtcColumn;
    mtcColumn         * sSceondMtcColumn;

    // 초기화
    sUserPasswd = (mtdCharType *) sUserPasswdBuffer;
    sUserID     = (mtdIntegerType ) aUserID;

    qtc::setVarcharValue( sUserPasswd,
                          NULL,
                          (SChar*)aUserPasswd,
                          0 );

    IDE_TEST( smiGetTableColumns( gQcmPasswordHistory,
                                  QCM_PASSWORD_HISTORY_USER_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmPasswordHistory,
                                  QCM_PASSWORD_HISTORY_PASSWORD,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    // 검색 조건
    qcm::makeMetaRangeDoubleColumn(
        &sFirstMetaRange,
        &sSecondMetaRange,
        sFirstMtcColumn,
        (const void*) & sUserID,
        sSceondMtcColumn,
        (const void*) sUserPasswd,
        &sRange);
        
    // 조회
    IDE_TEST( qcm::selectCount( QC_SMI_STMT(aStatement),
                                gQcmPasswordHistory,
                                &sRowCount,
                                smiGetDefaultFilter(),
                                &sRange,
                                gQcmPasswordHistoryIndex[QCM_PASSWORD_HISTORY_IDX1_ORDER] )
              != IDE_SUCCESS );

    if (sRowCount == 0)
    {
        *aExist = ID_FALSE;
    }
    else
    {
        *aExist = ID_TRUE;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmReuseVerify::actionVerifyFunction( qcStatement * aStatement,
                                             SChar       * aUserName,
                                             SChar       * aUserPasswd,
                                             SChar       * aVerifyFunction )
{

/***********************************************************************
 *
 * Description : PROJ-2207 Password policy support
 *      password_verify_function 설정한 function을 수행
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar         sBuffer[QD_MAX_SQL_LENGTH] = {0,};
    idBool        sRecordExist;
    SDouble       sCharBuffer[ (ID_SIZEOF(UShort) + QC_PASSWORD_OPT_USER_ERROR_LEN + 7) / 8 ];
    mtdCharType * sResult = (mtdCharType*)&sCharBuffer;
    SChar         sMsgBuffer[QC_PASSWORD_OPT_USER_ERROR_LEN + 1];

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "SELECT CAST(sys.%s(VARCHAR'%s',VARCHAR'%s') AS VARCHAR(1024)) "
                     "FROM "
                     "system_.sys_dummy_ ",
                     aVerifyFunction,
                     aUserName,
                     aUserPasswd);
    
    IDE_TEST_RAISE(qcg::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                               sBuffer,
                                               sResult,
                                               &sRecordExist,
                                               ID_TRUE ) != IDE_SUCCESS,
                   ERR_VERIFY_FUNCTION_SELECT );
    
    IDE_TEST_RAISE( sRecordExist == ID_FALSE,
                    ERR_NO_DATA_FOUND );
    
    IDE_TEST_RAISE( ( sResult->length == 0 ) ||
                    ( sResult->length > QC_PASSWORD_OPT_USER_ERROR_LEN ),
                    ERR_MSG_TOO_LONG );
    
    idlOS::memcpy( sMsgBuffer, sResult->value, sResult->length);
    sMsgBuffer[sResult->length] = '\0';
    
    IDE_TEST_RAISE( idlOS::strMatch( sMsgBuffer,
                                     sResult->length,
                                     "TRUE",
                                     4 ) != 0,
                    ERR_USER_DEFINED_ERROR );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_NO_DATA_FOUND);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_NO_DATA_FOUND));
    }
    IDE_EXCEPTION(ERR_VERIFY_FUNCTION_SELECT);
    {
        ideClearError();
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_VERIFY_FUNCTION_SELECT_ERROR));
    }
    IDE_EXCEPTION(ERR_MSG_TOO_LONG);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_MESSAGE_TOO_LONG));
    }
    IDE_EXCEPTION(ERR_USER_DEFINED_ERROR);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_USER_DEFINED_ERROR,
                                sMsgBuffer ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmReuseVerify::actionPasswordReuse( qcStatement  * aStatement,
                                            UChar        * aUserPasswd,
                                            qciUserInfo  * aUserPasswOpts )
{
/***********************************************************************
 *
 * Description : PROJ-2207 Password policy support
 *      PASSWORD_REUSE_MAX,
 *      PASSWORD_REUSE_TIME CHECK
 *    
 * Implementation :
 *      BUG-37433 에서 정책 변경.
 *      기존:
 *         PASSWORD_REUSE_MAX 설정된 값만 큼 패스워드 재사용 가능.
 *         PASSWORD_REUSE_TIME PASSWORD_REUSE_MAX 값 만큼 사용 되었을 때 기준으로
 *         PASSWORD_REUSE_TIME 더한 날자에 패스워드를 재사용 할수 있다.
 *      변경:
 *         PASSWORD_REUSE_MAX 설정한 값 만큼 패스워드를 변경 하면 패스워드를 재사용
 *         할 수 있다.
 *         PASSWORD_REUSE_TIME 패스워드를 재사용 할수 있는 날자.
 *      옵션:
 *         두개의 옵션이 다 존재 해야 옵션 적용 가능.
 *         하나만 있을 경우 무조건 패스워드 재사용 불가.
 ***********************************************************************/

    idBool sExist = ID_FALSE;
    UInt   sPasswordDate;

    if ( ( aUserPasswOpts->mAccLimitOpts.mPasswReuseMax != 0 ) &&
         ( aUserPasswOpts->mAccLimitOpts.mPasswReuseTime != 0 ) )
    {            
        /* SYS_PASSWORD_HISTORY_에 패스워드 존재 여부 체크 */
        IDE_TEST( qcmReuseVerify::checkReusePasswd( aStatement,
                                                    aUserPasswOpts->userID,
                                                    aUserPasswd,
                                                    &sExist )
                  != IDE_SUCCESS );
    
        if ( sExist == ID_FALSE )
        {
            /* SYS_PASSWORD_HISTORY_  패스워드 기록 */
            IDE_TEST( qcmReuseVerify::addPasswHistory( aStatement,
                                                       aUserPasswd,
                                                       aUserPasswOpts->userID )
                      != IDE_SUCCESS );
        }
        else
        {
            /* SYS_PASSWORD_HISTORY에서 변경 하고자 하는 PASSWORD DATE 를 구한다. */
            IDE_TEST( qcmReuseVerify::getPasswordDate( aStatement,
                                                       aUserPasswOpts->userID,
                                                       aUserPasswd,
                                                       &sPasswordDate )
                      != IDE_SUCCESS );

            /* sExist TRUE 인 패스워드는 PASSWORD_REUSE_TIME에 따라 재사용 여부 결정.
             * 변경 하고자 하는 패스워드의 PASSWORD DATE + PASSWORD_REUSE_TIME 기간이
             * 지났거나 같을때 패스워드 reuse count 채크 */
            if ( aUserPasswOpts->mAccLimitOpts.mCurrentDate >=
                 ( aUserPasswOpts->mAccLimitOpts.mPasswReuseTime + sPasswordDate ) ) 
            {
                /* SYS_PASSWORD_HISTORY에서 변경 하고자 하는 PASSWORD 이후 변경 된
                 * 패스워드의 count 와 reuse max 값을 비교하여 재사용 가능한
                 * 패스워드인지 구한다. */
                IDE_TEST( qcmReuseVerify::checkPasswordReuseCount(
                              aStatement,
                              aUserPasswOpts->userID,
                              aUserPasswd,
                              aUserPasswOpts->mAccLimitOpts.mPasswReuseMax )
                          != IDE_SUCCESS );
            }
            else
            {
                /* 변경 하고자 하는 패스워드가 PASSWORD_REUSE_TIME이 전인 것은
                 * 재 사용 패스워드 인경우 ERROR */
                IDE_RAISE( ERR_REUSE_COUNT );
            }
        }
    }
    else
    {
        /* SYS_PASSWORD_HISTORY_에 패스워드 존재 여부 체크 */
        IDE_TEST( qcmReuseVerify::checkReusePasswd( aStatement,
                                                    aUserPasswOpts->userID,
                                                    aUserPasswd,
                                                    &sExist )
                  != IDE_SUCCESS );
    
        if ( sExist == ID_FALSE )
        {
            /* SYS_PASSWORD_HISTORY_  패스워드 기록 */
            IDE_TEST( qcmReuseVerify::addPasswHistory( aStatement,
                                                       aUserPasswd,
                                                       aUserPasswOpts->userID )
                      != IDE_SUCCESS );
        }
        else
        {
            /* 재 사용 패스워드 인경우 ERROR */
            IDE_RAISE( ERR_REUSE_COUNT );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_REUSE_COUNT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_REUSE_COUNT));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
} 
 
IDE_RC qcmReuseVerify::getPasswordDate( qcStatement * aStatement,
                                        UInt          aUserID,
                                        UChar       * aUserPasswd,
                                        UInt        * aPasswordDate )
{
/***********************************************************************
 *
 * Description : PROJ-2207 Password policy support
 *     BUG-37443
 *           SYS_PASSWORD_HISTORY_ 에서 변경 하고자 하는 패스워드의
 *           일수를 구한다.
 *        
 * Implementation :
 *
 ***********************************************************************/
    
    smiRange                sRange;
    qtcMetaRangeColumn      sFirstMetaRange;
    qtcMetaRangeColumn      sSecondMetaRange;
    ULong                   sUserPasswdBuffer[ (ID_SIZEOF(UShort) + IDS_MAX_PASSWORD_BUFFER_LEN + 7) / 8 ];
    mtdCharType           * sUserPasswd;
    mtdIntegerType          sUserID;
    mtcColumn             * sMtcColumn;
    SInt                    sStage = 0;
    const void            * sRow;
    smiCursorProperties     sProperty;
    scGRID                  sRid;
    smiTableCursor          sCursor;
    mtdDateType           * sPasswordDate;
    mtdDateType             sStartDate;
    mtdBigintType           sResult;
    mtcColumn             * sFirstMtcColumn;
    mtcColumn             * sSceondMtcColumn;

    // 초기화
    sUserPasswd = (mtdCharType *) sUserPasswdBuffer;
    sUserID     = (mtdIntegerType ) aUserID;

    // Get Start Date
    IDE_TEST(  mtdDateInterface::makeDate( &sStartDate,
                                           QD_PASSWORD_POLICY_START_DATE_YEAR,
                                           QD_PASSWORD_POLICY_START_DATE_MON,
                                           QD_PASSWORD_POLICY_START_DATE_DAY,
                                           0,
                                           0,
                                           0,
                                           0 )
               != IDE_SUCCESS );

    qtc::setVarcharValue( sUserPasswd,
                          NULL,
                          (SChar*)aUserPasswd,
                          0 );

    IDE_TEST( smiGetTableColumns( gQcmPasswordHistory,
                                  QCM_PASSWORD_HISTORY_USER_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmPasswordHistory,
                                  QCM_PASSWORD_HISTORY_PASSWORD,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    // 검색 조건
    qcm::makeMetaRangeDoubleColumn(
        &sFirstMetaRange,
        &sSecondMetaRange,
        sFirstMtcColumn,
        (const void*) & sUserID,
        sSceondMtcColumn,
        (const void*) sUserPasswd,
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sProperty, NULL );

    IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                            gQcmPasswordHistory,
                            gQcmPasswordHistoryIndex[QCM_PASSWORD_HISTORY_IDX1_ORDER],
                            smiGetRowSCN( gQcmPasswordHistory ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    IDE_TEST_RAISE(sRow == NULL, ERR_META_CRASH);

    IDE_TEST( smiGetTableColumns( gQcmPasswordHistory,
                                  QCM_PASSWORD_HISTORY_PASSWORD_DATE,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    
    sPasswordDate = (mtdDateType*)((SChar*)sRow + sMtcColumn->column.offset);

    IDE_TEST_RAISE( MTD_DATE_IS_NULL( sPasswordDate ) == 1,
                    ERR_META_CRASH);
    
    IDE_TEST( mtdDateInterface::dateDiff( &sResult,
                                          &sStartDate,
                                          sPasswordDate,
                                          MTD_DATE_DIFF_DAY )
              != IDE_SUCCESS );
    
    sStage = 0;
    
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    *aPasswordDate = (UInt)sResult;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmReuseVerify::checkPasswordReuseCount( qcStatement * aStatement,
                                                UInt          aUserID,
                                                UChar       * aUserPasswd,
                                                UInt          aPasswdReuseMax )
{
/***********************************************************************
 *
 * Description : PROJ-2207 Password policy support
 *   BUG-37443
 *           SYS_PASSWORD_HISTORY_에서 변경 하고지 하는 패스워드
 *           이후 패스워드 변경 된 횟 수를 구하여  PASSWORD_REUSE_MAX와 비교
 *           하여 패스워드 재사용 가능 여부를 체크한다.
 *        
 * Implementation :
 *
 ***********************************************************************/
    
    smiRange                sRange;
    qtcMetaRangeColumn      sRangeColumn;
    mtdIntegerType          sUserID;
    mtcColumn             * sMtcColumn;
    SInt                    sStage = 0;
    const void            * sRow;
    smiCursorProperties     sProperty;
    scGRID                  sRid;
    smiTableCursor          sCursor;
    mtdCharType           * sPasswdStr;
    UInt                    sReuseCount = 0;
    UInt                    sUserPasswdLen;
    idBool                  sIsExist = ID_FALSE;
    idBool                  sIsPasswdReuse = ID_FALSE;
    mtcColumn             * sFirstMtcColumn;

    IDE_DASSERT( aUserPasswd != NULL );
    
    // 초기화
    sUserID = (mtdIntegerType) aUserID;
    sUserPasswdLen = idlOS::strlen((SChar*)aUserPasswd);

    IDE_TEST( smiGetTableColumns( gQcmPasswordHistory,
                                  QCM_PASSWORD_HISTORY_USER_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    // 검색 조건
    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn *)
        sFirstMtcColumn,
        (const void*) & sUserID,
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sProperty, NULL );

    IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                            gQcmPasswordHistory,
                            gQcmPasswordHistoryIndex[QCM_PASSWORD_HISTORY_IDX2_ORDER],
                            smiGetRowSCN( gQcmPasswordHistory ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        IDE_TEST( smiGetTableColumns( gQcmPasswordHistory,
                                      QCM_PASSWORD_HISTORY_PASSWORD,
                                      (const smiColumn**)&sMtcColumn )
                  != IDE_SUCCESS );

        sPasswdStr  = (mtdCharType*)((SChar*)sRow + sMtcColumn->column.offset);
        
        /*  reuse count  */
        if ( sIsExist == ID_FALSE )
        {
            if ( idlOS::strMatch( (SChar*)aUserPasswd,
                                  sUserPasswdLen,
                                  (SChar*)sPasswdStr->value,
                                  sPasswdStr->length ) == 0 ) 
            {
                /* 변경 하고자 하는 패스워드 시점 부터 reuse count 하기위해 초기화 */
                sIsExist = ID_TRUE;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            sReuseCount++;

            /* 패스워드 재사용이 가능한지 비교 */
            if ( aPasswdReuseMax == sReuseCount )
            {
                sIsPasswdReuse = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing To Do */
            }
        }
            
        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }
    
    sStage = 0;
    
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    if ( sIsPasswdReuse == ID_TRUE )
    {
        /* 패스워드 재사용 되는 패스워드의 PASSWORD_DATE 갱신 */
        IDE_TEST( qcmReuseVerify::updatePasswdDate( aStatement,
                                                    aUserPasswd,
                                                    aUserID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* 패스워드 REUSE COUNT 변경 횟수가 작을 경우 */
        IDE_RAISE( ERR_REUSE_COUNT );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_REUSE_COUNT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_REUSE_COUNT));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}
