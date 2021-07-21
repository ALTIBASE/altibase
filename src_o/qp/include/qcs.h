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
#ifndef _O_QCS_H_
#define  _O_QCS_H_  1

#include <qc.h>
#include <smi.h>

#define QCS_MODULE_NAME_SIZE       (24)
#define QCS_MODULE_VERSION_SIZE    (40)

#define QCS_MODULE_LIBRARY_SIZE    (IDP_MAX_VALUE_LEN)

#define QCS_POLICY_NAME_SIZE       (MTC_POLICY_NAME_SIZE)

#define QCS_POLICY_CODE_SIZE       (128)
#define QCS_ECC_POLICY_CODE_SIZE   (64)

class qcs
{    
public:

    static IDE_RC  initialize( idvSQL * aStatistics );

    static IDE_RC  finalize( void );

    static IDE_RC  startSecurity( qcStatement  * aStatment );

    static IDE_RC  stopSecurity( qcStatement  * aStatment );

private:

    static IDE_RC  getMetaSecurity(
        smiStatement  * aSmiStmt,
        idBool        * aIsExist,
        SChar         * aModuleName,
        SChar         * aModuleVersion,
        SChar         * aECCPolicyName,
        UChar         * aECCPolicyCode,
        UShort        * aECCPolicyCodeSize );
    
    static IDE_RC  getEncryptedColumnCount(
        smiStatement  * aSmiStmt,
        UInt          * aCount );

    static IDE_RC  verifyEncryptedColumnPolicy(
        smiStatement  * aSmiStmt );

    static IDE_RC  insertSecurityModuleIntoMeta(
        smiStatement  * aSmiStmt,
        SChar         * aModuleName,
        SChar         * aModuleVersion,
        SChar         * aECCPolicyName,
        SChar         * aECCPolicyCode );
    
    static IDE_RC  deleteSecurityModuleFromMeta(
        smiStatement  * aSmiStmt );
};

#endif // _O_QCS_H_
