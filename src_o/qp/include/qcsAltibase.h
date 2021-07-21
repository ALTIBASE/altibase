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

#ifndef _O_QCS_ALTIBASE_H_
# define _O_QCS_ALTIBASE_H_ 1

# include <qc.h>
# include <qcs.h>
# include <qcsModule.h>

class qcsAltibase
{
public:

    //--------------------------------------------------
    // 보안 모듈의 초기화 관련 함수
    //--------------------------------------------------

    static IDE_RC initializeModule( SChar  * aECCPolicyName,
                                    SChar  * aModuleVersion );

    static IDE_RC finalizeModule( void );

    //--------------------------------------------------
    // 보안 모듈의 policy 관련 함수
    //--------------------------------------------------

    static IDE_RC getPolicyInfo( SChar   * aPolicyName,
                                 idBool  * aIsExist,
                                 idBool  * aIsSalt,
                                 idBool  * aIsEncodeECC );
                                 
    static IDE_RC getEncryptedSize( SChar  * aPolicyName,
                                    UInt     aSize,
                                    UInt   * aEncryptedSize );

    static IDE_RC getColumnPolicy( SChar  * aTableOwnerName,
                                   SChar  * aTableName,
                                   SChar  * aColumnName,
                                   SChar  * aPolicyName );

    static IDE_RC setColumnPolicy( SChar  * aTableOwnerName,
                                   SChar  * aTableName,
                                   SChar  * aColumnName,
                                   SChar  * aPolicyName );
    
    static IDE_RC unsetColumnPolicy( SChar  * aTableOwnerName,
                                     SChar  * aTableName,
                                     SChar  * aColumnName );
        
    //--------------------------------------------------
    // 보안 모듈의 암복호화 관련 함수
    //--------------------------------------------------

    static IDE_RC encrypt( SChar   * aPolicyName,
                           UChar   * aPlain,
                           UShort    aPlainSize,
                           UChar   * aCipher,
                           UShort  * aCipherSize );

    static IDE_RC decrypt( SChar   * aPolicyName,
                           UChar   * aCipher,
                           UShort    aCipherSize,
                           UChar   * aPlain,
                           UShort  * aPlainSize );
    
    static IDE_RC encodeECC( SChar   * aPolicyName,
                             UChar   * aPlain,
                             UShort    aPlainSize,
                             UChar   * aCipher,
                             UShort  * aCipherSize );
    
    //--------------------------------------------------
    // 보안 메타 검증 함수
    //--------------------------------------------------

    static IDE_RC getPolicyCode( SChar   * aPolicyName,
                                 UChar   * aPolicyCode,
                                 UShort  * aPolicyCodeLength );
    
    static IDE_RC verifyPolicyCode( SChar   * aPolicyName,
                                    UChar   * aPolicyCode,
                                    UShort    aPolicyCodeLength,
                                    idBool  * aIsValid );
};

#endif /* _O_QCS_ALTIBASE_H_ */
