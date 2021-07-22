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
 * $Id: qcmDNUser.cpp 21926 2007-05-28 02:03:26Z mhjeong $
 **********************************************************************/

#include <idl.h>
#include <iduMemory.h>
#include <ide.h>
#include <qcm.h>
#include <qcmCache.h>
#include <qcmDNUser.h>

const void   * gQcmDNUsers;
const void   * gQcmDNUsersIndex [ QCM_MAX_META_INDICES ];
    
qcmTableInfo * gQcmDNUsersTempInfo;

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmDNUser::isPermittedUsername()
 *
 * Argument :
 *    aStatement   (IN)  - qcStatement
 *    aSmiStmt     (IN)  - smiStatement for opening smiCursor.
 *    aUserName    (IN)  - username
 *    aIsPermitted (OUT) - ID_TRUE: Permitted, ID_FALSE: Denied
 * 
 * Description :
 *    This function is called to check privilege information of an user.
 * ---------------------------------------------------------------------------*/
//----------------------------------
// select count(*)
//   from system_.sys_dn_users_
//  where user_name = :aUserName
//    and user_dn is null
//    and user_aid is null
//    and start_period >= sysdate
//    and end_period <= sysdate
//----------------------------------
IDE_RC qcmDNUser::isPermittedUsername(
qcStatement   * aStatement,
smiStatement  * /*_ aSmiStmt _*/,
SChar         * aUserName,
idBool        * aIsPermitted)
{
    UInt                    sStage = 0;
    const void            * sUserDNTable;
    const void            * sRow;
    void                  * sValue1Temp;
    void                  * sValue2Temp;
    smiTableCursor          sCursor;
    mtcColumn             * sMtcColumnUserName;
    mtcColumn             * sMtcColumnUserDN;
    mtcColumn             * sMtcColumnUserAID;
    mtcColumn             * sMtcColumnStartDate;
    mtcColumn             * sMtcColumnEndDate;
    mtdCharType           * sUserNameStr;
    idBool                  sFoundUser = ID_FALSE;
    smSCN                   sSCN;

    UInt                    sUserNameSize;

    UChar                   sSearchingUserName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier

    mtdDateType             sSysDate;
    const mtdModule       * sDateModule;
    const mtdModule       * sVarcharModule;
    PDL_Time_Value          sTimevalue;
    struct tm               sLocaltime;
    time_t                  sTime;
    mtdValueInfo            sValueInfo1;
    mtdValueInfo            sValueInfo2;
    mtdValueInfo            sValueInfo3;
    mtdValueInfo            sValueInfo4;

    *aIsPermitted = ID_FALSE;

    /*
     * [1] make sysdate and username for search.
     */
    sTimevalue = idlOS::gettimeofday();
    sTime      = (time_t)sTimevalue.sec();
    idlOS::localtime_r( &sTime, &sLocaltime );
    IDE_TEST( mtdDateInterface::makeDate( &sSysDate,
                                          (SShort)(sLocaltime.tm_year + 1900),
                                          sLocaltime.tm_mon + 1,
                                          sLocaltime.tm_mday,
                                          sLocaltime.tm_hour,
                                          sLocaltime.tm_min,
                                          sLocaltime.tm_sec,
                                          sTimevalue.usec() )
              != IDE_SUCCESS );

    IDE_TEST_RAISE(aUserName == NULL, ERR_NULL_USER_NAME);
    sUserNameSize = idlOS::strlen(aUserName);
    IDE_TEST_RAISE(sUserNameSize == 0, ERR_NULL_USER_NAME);

    idlOS::strncpy( (SChar*) sSearchingUserName , aUserName, sUserNameSize);
    sSearchingUserName[sUserNameSize] = '\0';

    /*
     * [2] get table handle
     */
    IDE_TEST(qcm::getTableHandleByName(
                 QC_SMI_STMT( aStatement ),
                 QC_SYSTEM_USER_ID,
                 (UChar*)QCM_DN_USERS,
                 idlOS::strlen((SChar*)QCM_DN_USERS),
                 (void**)&sUserDNTable,
                 &sSCN)
             != IDE_SUCCESS);


    /*
     * [3] get mtcColumns
     */
    IDE_TEST( smiGetTableColumns( sUserDNTable,
                                  QCM_DN_USERS_USER_NAME_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnUserName )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sUserDNTable,
                                  QCM_DN_USERS_USER_DN_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnUserDN )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sUserDNTable,QCM_DN_USERS_USER_AID_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnUserAID )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sUserDNTable,
                                  QCM_DN_USERS_START_PERIOD_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnStartDate )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sUserDNTable,
                                  QCM_DN_USERS_END_PERIOD_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnEndDate )
              != IDE_SUCCESS );

    /*
     * [4] get mtdModules
     */
    IDE_TEST(mtd::moduleById(&sDateModule, sMtcColumnStartDate->type.dataTypeId)
             != IDE_SUCCESS);
    IDE_TEST(mtd::moduleById(&sVarcharModule, sMtcColumnUserName->type.dataTypeId)
             != IDE_SUCCESS);

    /*
     * [5] cursor initialize
     */
    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN(&sCursorProperty, aStatement->mStatistics);

    /*
     * [6] cursor open for full table scan
     */
    IDE_TEST(sCursor.open(
                 QC_SMI_STMT( aStatement ),
                 sUserDNTable,
                 NULL,
                 smiGetRowSCN(sUserDNTable),
                 NULL,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    /*
     * [7] fetch a row
     */
    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    IDE_TEST_RAISE(sRow == NULL, ERR_NOT_EXIST_USER);
        
    /*
     * [8] check predicate and fetch until a match is found.
     */
    /*
     * select count(*)
     *   from system_.sys_dn_users_
     *  where user_name = :aUserName
     *    and user_dn is null
     *    and user_aid is null
     *    and start_period >= sysdate
     *    and end_period <= sysdate
     */
    while (sRow != NULL && sFoundUser != ID_TRUE)
    {
        sUserNameStr = (mtdCharType*)((UChar *)sRow + sMtcColumnUserName->column.offset);

        if (sUserNameSize == sUserNameStr->length)
        {
            if (idlOS::strncasecmp( (SChar*) sSearchingUserName,
                                (SChar*) &(sUserNameStr->value),
                                sUserNameStr->length) == 0)
            {
                sValueInfo1.column = sMtcColumnStartDate;
                sValueInfo1.value  = mtc::value( sMtcColumnStartDate,
                                                 sRow,
                                                 MTD_OFFSET_USE );
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = sMtcColumnStartDate;
                sValueInfo2.value  = (const void*)&sSysDate;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                sValueInfo3.column = sMtcColumnEndDate;
                sValueInfo3.value  = mtc::value( sMtcColumnEndDate,
                                                 sRow,
                                                 MTD_OFFSET_USE );
                sValueInfo3.flag   = MTD_OFFSET_USELESS;

                sValueInfo4.column = sMtcColumnEndDate;
                sValueInfo4.value  = (const void*)&sSysDate;
                sValueInfo4.flag   = MTD_OFFSET_USELESS;

                sValue1Temp = (void*)mtc::value( sMtcColumnUserDN,
                                                 sRow,
                                                 MTD_OFFSET_USE );

                sValue2Temp = (void*)mtc::value( sMtcColumnUserAID,
                                                 sRow,
                                                 MTD_OFFSET_USE );

                if( (sDateModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                         &sValueInfo2 ) <= 0)
                    &&
                    (sDateModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo3,
                                                                         &sValueInfo4 ) >= 0)
                    &&
                    (sVarcharModule->isNull(sMtcColumnUserDN,
                                            sValue1Temp) == ID_TRUE) &&
                    (sVarcharModule->isNull(sMtcColumnUserAID,
                                            sValue2Temp) == ID_TRUE) )
                {
                    *aIsPermitted = ID_TRUE;
                    sFoundUser    = ID_TRUE;
                }
            }
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    /*
     * [9] close the cursor
     */
    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_USER_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_USER));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_USER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_USER));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmDNUser::getUsername()
 *  
 * Argument :
 *    aStatement   (IN) - qcStatement
 *    aSmiStmt     (IN) - smiStatement for opening smiCursor.
 *    aUsrDN       (IN) - user DN(distinguished name)
 *    aUserName    (INOUT) - output buffer
 *    aUserNameLen (INOUT) - output buffer size
 *  
 * Description :
 *    This function is called to get user name from user dn.
 * ---------------------------------------------------------------------------*/
//----------------------------------
// select user_name
//   from system_.sys_dn_users_
//  where user_dn = :mUsrDN
//    and start_date >= sysdate  <-- setSysdate()
//    and end_date <= sysdate
// ;  
//----------------------------------
#define QCM_DN_MAX_DN_SIZE 2048

IDE_RC qcmDNUser::getUsername(
qcStatement   * aStatement,
smiStatement  * /*_ aSmiStmt _*/,
SChar         * aUsrDN,
SChar         * aUserName,
UInt            aUserNameLen)
{
    UInt                    sStage = 0;
    smiRange                sRange;
    const void            * sUserDNTable;
    const void            * sRow;
    smiTableCursor          sCursor;
    mtcColumn             * sMtcColumnUserDN;
    mtcColumn             * sMtcColumnStartDate;
    mtcColumn             * sMtcColumnEndDate;
    mtcColumn             * sMtcColumnUserName;
    mtdCharType           * sUserDNStr;
    idBool                  sFoundUser = ID_FALSE;
    qtcMetaRangeColumn      sRangeColumn;
    smSCN                   sSCN;
    qcmColumn             * sColumn = NULL;
    mtdCharType *           sUserName;
    
    SDouble                 sUserDNBuffer[ (ID_SIZEOF(UShort) + QCM_DN_MAX_DN_SIZE + 7) / ID_SIZEOF(SDouble) ];
    mtdCharType           * sUserDN = ( mtdCharType * ) &sUserDNBuffer;

    UChar                   sSearchingUserDN[ QCM_DN_MAX_DN_SIZE + 1 ];
    
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    UInt                    sUsrDNSize;

    PDL_Time_Value          sTimevalue;
    struct tm               sLocaltime;
    time_t                  sTime;
    mtdDateType             sSysDate;
    const mtdModule       * sDateModule;
    mtdValueInfo            sValueInfo1;
    mtdValueInfo            sValueInfo2;
    mtdValueInfo            sValueInfo3;
    mtdValueInfo            sValueInfo4;

    sUsrDNSize = idlOS::strlen(aUsrDN);

    /*
     * [1] make sysdate and user dn for search.
     */
    sTimevalue = idlOS::gettimeofday();
    sTime      = (time_t)sTimevalue.sec();
    idlOS::localtime_r( &sTime, &sLocaltime );
    IDE_TEST( mtdDateInterface::makeDate( &sSysDate,
                                          (SShort)(sLocaltime.tm_year + 1900),
                                          sLocaltime.tm_mon + 1,
                                          sLocaltime.tm_mday,
                                          sLocaltime.tm_hour,
                                          sLocaltime.tm_min,
                                          sLocaltime.tm_sec,
                                          sTimevalue.usec() )
              != IDE_SUCCESS );
    
    IDE_TEST( sUsrDNSize > (QCM_DN_MAX_DN_SIZE -1) );
    idlOS::strncpy( (SChar*) sSearchingUserDN , aUsrDN, sUsrDNSize);
    sSearchingUserDN [ sUsrDNSize ] = '\0';
    
    /*
     * [2] get table handle
     */
    IDE_TEST(qcm::getTableHandleByName(
                 QC_SMI_STMT( aStatement ),
                 QC_SYSTEM_USER_ID,
                 (UChar*)QCM_DN_USERS,
                 idlOS::strlen((SChar*)QCM_DN_USERS),
                 (void**)&sUserDNTable,
                 &sSCN)
             != IDE_SUCCESS);

    /*
     * [3] get mtcColumns
     */
    IDE_TEST( smiGetTableColumns( sUserDNTable,
                                  QCM_DN_USERS_USER_DN_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnUserDN )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sUserDNTable,
                                  QCM_DN_USERS_START_PERIOD_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnStartDate )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sUserDNTable,
                                  QCM_DN_USERS_END_PERIOD_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnEndDate )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sUserDNTable,
                                  QCM_DN_USERS_USER_NAME_COL_ORDER,
                                  (const smiColumn**)&sMtcColumnUserName )
              != IDE_SUCCESS );

    /*
     * [4] get mtdModule
     */
    IDE_TEST(mtd::moduleById(&sDateModule, sMtcColumnStartDate->type.dataTypeId)
             != IDE_SUCCESS);

    /*
     * [5] cursor initialize
     */
    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, aStatement->mStatistics);
    
    /*
     * [6] fetch rows
     */
    if ( gQcmDNUsersIndex[QCM_DN_USER_DN_AID_NAME_IDX_ORDER] == NULL) 
    {// on createdb
        IDE_TEST(sCursor.open(
                     QC_SMI_STMT( aStatement ),
                     sUserDNTable,
                     NULL,
                     smiGetRowSCN(sUserDNTable),
                     NULL,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     QCM_META_CURSOR_FLAG,
                     SMI_SELECT_CURSOR,
                     & sCursorProperty) != IDE_SUCCESS);
        sStage = 1;

        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
        IDE_TEST_RAISE(sRow == NULL, ERR_NOT_EXIST_USER);

        while (sRow != NULL && sFoundUser != ID_TRUE)
        {
            sUserDNStr = (mtdCharType*)((UChar *)sRow +
                                        sMtcColumnUserDN->column.offset);

            if (idlOS::strMatch( (SChar*) sSearchingUserDN,
                                 sUsrDNSize,
                                 (SChar*) &(sUserDNStr->value),
                                 sUserDNStr->length ) == 0 )
            {
                sValueInfo1.column = sMtcColumnStartDate;
                sValueInfo1.value  = mtc::value( sMtcColumnStartDate,
                                                 sRow,
                                                 MTD_OFFSET_USE );
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = sMtcColumnStartDate;
                sValueInfo2.value  = (const void*)&sSysDate;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                sValueInfo3.column = sMtcColumnEndDate;
                sValueInfo3.value  = mtc::value( sMtcColumnEndDate,
                                                 sRow,
                                                 MTD_OFFSET_USE );
                sValueInfo3.flag   = MTD_OFFSET_USELESS;

                sValueInfo4.column = sMtcColumnEndDate;
                sValueInfo4.value  = (const void*)&sSysDate;
                sValueInfo4.flag   = MTD_OFFSET_USELESS;

                if( (sDateModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                         &sValueInfo2 ) <= 0)
                    &&
                    (sDateModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo3,
                                                                         &sValueInfo4 ) >= 0) )
                {
                    sUserName = (mtdCharType*)((UChar *)sRow +
                                    sMtcColumnUserName->column.offset);
                    IDE_TEST( sUserName->length > (aUserNameLen - 1) );
                    idlOS::strncpy(aUserName, (const SChar *)sUserName->value, sUserName->length);

                    // fix BUG-32700
                    aUserName[sUserName->length] = '\0';

                    sFoundUser = ID_TRUE;
                }
            }

            IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

        }
        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }
    else
    {
        //----------------------------------
        // select user_name
        //   from system_.sys_dn_users_
        //  where user_dn = :mUsrDN
        //    and start_date >= sysdate
        //    and end_date <= sysdate;
        //----------------------------------
        /*
         * [6.1] get qcmColumn
         */
        IDE_TEST(qcmCache::getColumnByName(
                     gQcmDNUsersTempInfo,
                     (SChar*) "USER_DN",
                     idlOS::strlen((SChar*) "USER_DN"),
                     &sColumn) != IDE_SUCCESS);

        IDE_ASSERT( sColumn != NULL );

        /*
         * [6.2] bind value
         */
        qtc::setVarcharValue( sUserDN,
                              sColumn->basicInfo,
                              aUsrDN,
                              sUsrDNSize );

        /*
         * [6.3] make smiRange keyrange
         */
        qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                        (const mtcColumn*)sColumn->basicInfo,
                                        (const void *) sUserDN,
                                        &sRange );

        /*
         * [6.4] cursor open for index scan with keyrange
         */
        IDE_TEST(sCursor.open(
                     QC_SMI_STMT( aStatement ),
                     gQcmDNUsersTempInfo->tableHandle,
                     gQcmDNUsersIndex[QCM_DN_USER_DN_AID_NAME_IDX_ORDER],
                     smiGetRowSCN(gQcmDNUsersTempInfo->tableHandle),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     QCM_META_CURSOR_FLAG,
                     SMI_SELECT_CURSOR,
                     & sCursorProperty) != IDE_SUCCESS);
        sStage = 1;

        /*
         * [6.5] fetch a row
         */
        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
        IDE_TEST_RAISE(sRow == NULL, ERR_NOT_EXIST_USER);

        /*
         * [6.6] check predicate and fetch until a match is found.
         */
        while (sRow != NULL && sFoundUser != ID_TRUE)
        {
            sValueInfo1.column = sMtcColumnStartDate;
            sValueInfo1.value  = mtc::value( sMtcColumnStartDate,
                                             sRow,
                                             MTD_OFFSET_USE );
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = sMtcColumnStartDate;
            sValueInfo2.value  = (const void*)&sSysDate;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sValueInfo3.column = sMtcColumnEndDate;
            sValueInfo3.value  = mtc::value( sMtcColumnEndDate,
                                             sRow,
                                             MTD_OFFSET_USE );
            sValueInfo3.flag   = MTD_OFFSET_USELESS;

            sValueInfo4.column = sMtcColumnEndDate;
            sValueInfo4.value  = (const void*)&sSysDate;
            sValueInfo4.flag   = MTD_OFFSET_USELESS;

            if( (sDateModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                     &sValueInfo2 ) <= 0)
                &&
                (sDateModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo3,
                                                                     &sValueInfo4 ) >= 0) )
            {
                sUserName = (mtdCharType*)((UChar *)sRow +
                                           sMtcColumnUserName->column.offset);
                idlOS::strncpy(aUserName, (const char *)sUserName->value, sUserName->length);

                // fix BUG-32700
                aUserName[sUserName->length] = '\0';

                sFoundUser = ID_TRUE;
            }
        }
        
        /*
         * [6.7] close cursor
         */
        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }
    IDE_TEST_RAISE(sFoundUser != ID_TRUE, ERR_NOT_EXIST_USER);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_USER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_USER));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

