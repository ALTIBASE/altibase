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
 * $Id: qcmUser.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QCM_USER_H_
#define _O_QCM_USER_H_ 1

#include    <qc.h>
#include    <qcm.h>
#include    <qtc.h>
#include    <qci.h>

class qcmUser
{
public:
    static IDE_RC getUserID(
        qcStatement     *aStatement,
        qcNamePosition   aUserName,
        UInt            *aUserID);

    static IDE_RC getUserID(
        qcStatement     *aStatement,
        SChar           *aUserName,
        UInt             aUserNameSize,
        UInt            *aUserID);

    static IDE_RC getUserID( idvSQL       * aStatistics,
                             smiStatement * aSmiStatement,
                             SChar        * aUserName,
                             UInt           aUserNameSize,
                             UInt         * aUserID );

    static IDE_RC getUserPassword(
        qcStatement     *aStatement,
        UInt             aUserID,
        SChar           *aPassword);

    static IDE_RC getUserName(
        qcStatement     *aStatement,
        UInt             aUserID,
        SChar           *aUserName);

    static IDE_RC getNextUserID(
        qcStatement *aStatement,
        UInt        *aUserID);

    static IDE_RC getDefaultTBS(
        qcStatement     * aStatement,
        UInt              aUserID,
        scSpaceID       * aTBSID,
        scSpaceID       * aTempTBSID = NULL);

    static IDE_RC findTableListByUserID(
        qcStatement      * aStatement,
        UInt               aUserID,
        qcmTableInfoList **aTableInfoList);

    static IDE_RC findIndexListByUserID(
        qcStatement      * aStatement,
        UInt               aUserID,
        qcmIndexInfoList **aIndexInfoList);

    static IDE_RC findSequenceListByUserID(
        qcStatement         * aStatement,
        UInt                  aUserID,
        qcmSequenceInfoList **aSequenceInfoList);

    static IDE_RC findProcListByUserID(
        qcStatement      * aStatement,
        UInt               aUserID,
        qcmProcInfoList  **aProcInfoList);

    static IDE_RC findTriggerListByUserID(
        qcStatement        * aStatement,
        UInt                 aUserID,
        qcmTriggerInfoList **aTriggerInfoList);
    
    // BUG-24957
    // meta table에서 user name을 가져온다.
    static IDE_RC getMetaUserName( smiStatement * aSmiStmt,
                                   UInt           aUserID,
                                   SChar        * aUserName );

    // PROJ-1073 Package
    static IDE_RC findPkgListByUserID( qcStatement     * aStatement,
                                       UInt              aUserID,
                                       qcmPkgInfoList  **aPkgInfoList);

    /* PROJ-2207 Password policy support */
    static IDE_RC getCurrentDate(
        qcStatement     * aStatement,
        qciUserInfo     * aResult );

    static IDE_RC getPasswPolicyInfo(
        qcStatement     * aStatement,
        UInt              aUserID,
        qciUserInfo     * aResult );
    
    static IDE_RC updateFailedCount(
        smiStatement * aStatement,
        qciUserInfo  * aUserInfo );

    static IDE_RC updateLockDate(
        smiStatement * aStatement,
        qciUserInfo  * aUserInfo );

    /* PROJ-1812 ROLE */
    static IDE_RC getUserIDAndType(
        qcStatement     * aStatement,
        qcNamePosition    aUserName,
        UInt            * aUserID,
        qdUserType      * aUserType );

    static IDE_RC getUserIDAndTypeByName(
        qcStatement         * aStatement,
        SChar               * aUserName,
        UInt                  aUserNameSize,
        UInt                * aUserID,
        qdUserType          * aUserType );
    
    static IDE_RC getUserPasswordAndDefaultTBS(
        qcStatement     * aStatement,
        UInt              aUserID,
        SChar           * aPassword,
        scSpaceID       * aTBSID,
        scSpaceID       * aTempTBSID,
        qciDisableTCP   * aDisableTCP );
    
private:
    static IDE_RC searchUserID( smiStatement * aSmiStmt,
                                SInt           aUserID,
                                idBool       * aExist );
};

typedef struct qcmUserIDAndType
{
    UInt  userID;
    SChar userType[2];
} qcmUserIDAndType;

#endif /* _O_QCM_USER_H_ */
