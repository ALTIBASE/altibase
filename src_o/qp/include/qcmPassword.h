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
 * $Id: qcmUser.h 51834 2012-02-29 01:10:27Z jakejang $
 **********************************************************************/

#ifndef _O_QCM_PASSWROD_H_
#define _O_QCM_PASSWORD_H_ 1

#include    <qc.h>
#include    <qcm.h>
#include    <qtc.h>
#include    <qci.h>
#include    <qcmUser.h>

class qcmReuseVerify
{
public:
    static IDE_RC addPasswHistory( qcStatement * aStatement,
                                   UChar       * aUserPasswd,
                                   UInt          aUserID );

    static IDE_RC removePasswHistory( qcStatement * aStatement,
                                      UInt          aUserID );
    
    static IDE_RC checkReusePasswd( qcStatement      * aStatement,
                                    UInt               aUserID,
                                    UChar            * aUserPasswd,
                                    idBool           * aExist );
    
    static IDE_RC actionVerifyFunction( qcStatement * aStatement,
                                        SChar       * aUserName,
                                        SChar       * aUserPasswd,
                                        SChar       * aVerifyFunction );
    
    static IDE_RC actionPasswordReuse( qcStatement  * aStatement,
                                       UChar        * aUserPasswd,
                                       qciUserInfo  * aUserPasswOpts );

    /* BUG-37443 */
    static IDE_RC getPasswordDate( qcStatement * aStatement,
                                   UInt          aUserID,
                                   UChar       * aUserPasswd,
                                   UInt        * aPasswordDate );
    
    static IDE_RC checkPasswordReuseCount( qcStatement * aStatement,
                                           UInt          aUserID,
                                           UChar       * aUserPasswd,
                                           UInt          aPasswdReuseMax );

    static IDE_RC updatePasswdDate( qcStatement * aStatement,
                                    UChar       * aUserPasswd,
                                    UInt          aUserID );
};

#endif /* _O_QCM_PASSWORD_H_ */
