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
#ifndef _O_QCS_MODULE_H_
#define  _O_QCS_MODULE_H_  1

#include <qc.h>

# define QCS_POLICY_CODE_TEXT       ((UChar*)"test")
# define QCS_POLICY_CODE_TEXT_SIZE  (4)

# define QCS_ERROR_MSG_SIZE         (1024)
# define QCS_OK                     (0)

# define QCS_IS_DEFAULT_POLICY( aColumn ) \
    ( ( (aColumn)->mColumnAttr.mEncAttr.mPolicy[0] == '\0' ) ? ID_TRUE : ID_FALSE )

# if defined(VC_WIN32) || defined(VC_WIN64)
#  define QCS_API  __stdcall
# else
#  define QCS_API  
# endif

typedef enum
{
    QCS_MODULE_NONE = 1,
    QCS_MODULE_ALTIBASE,
    QCS_MODULE_DAMO,
    QCS_MODULE_CUBEONE
} qcsModuleProvider;

# define QCS_HASH_TYPE_MAX_SIZE  (10)
# define QCS_HASH_BUFFER_SIZE    (1024)

class qcsModule
{    
private:
    
    //-------------------------------------------------
    // 암복호화 함수
    //-------------------------------------------------
    
    static IDE_RC encrypt( mtcEncryptInfo * aInfo,
                           SChar          * aPolicyName,
                           UChar          * aPlain,
                           UShort           aPlainSize,
                           UChar          * aCipher,
                           UShort         * aCipherSize );
    
    static IDE_RC decrypt( mtcEncryptInfo * aInfo,
                           SChar          * aPolicyName,
                           UChar          * aCipher,
                           UShort           aCipherSize,
                           UChar          * aPlain,
                           UShort         * aPlainSize );
    
    static IDE_RC encodeECC( mtcECCInfo * aInfo,
                             UChar      * aPlain,
                             UShort       aPlainSize,
                             UChar      * aCipher,
                             UShort     * aCipherSize );
    
    //--------------------------------------------------
    // 기타함수
    //--------------------------------------------------

    static IDE_RC byteToString( UChar   * aByte,
                                UShort    aByteSize,
                                UChar   * aString,
                                UShort  * aStringSize );
    
    static IDE_RC stringToByte( UChar   * aString,
                                UShort    aStringSize,
                                UChar   * aByte,
                                UShort  * aByteSize );
    
public:

    //-------------------------------------------------
    // 초기화 함수
    //-------------------------------------------------

    static IDE_RC initialize( void );

    static IDE_RC finalize( void );
    
    //--------------------------------------------------
    // [보안 모듈 초기화 순서]
    // 1. openModule (모듈 open)
    // 2. verifyModule (모듈에 따른 인증)
    // 3. initializeModule (모듈에 따른 초기화, ECC policy 검사)
    //--------------------------------------------------

    //--------------------------------------------------
    // 보안 모듈의 초기화 함수
    //--------------------------------------------------

    static IDE_RC openModule( SChar  * aModule,
                              SChar  * aModuleLibrary );
    
    static IDE_RC closeModule( void );
    
    static IDE_RC verifyModule( void );

    static IDE_RC initializeModule( SChar  * aECCPolicyName );

    static IDE_RC finalizeModule( void );

    //--------------------------------------------------
    // 보안 모듈의 초기화 관련 함수
    //--------------------------------------------------
    
    static IDE_RC getModuleName( SChar  * aModuleName );
    
    static IDE_RC getModuleVersion( SChar  * aModuleVersion );
    
    static idBool isInitialized( void );
    
    //--------------------------------------------------
    // 보안 메타 검증 함수
    //--------------------------------------------------

    static IDE_RC getECCPolicyName( SChar  * aECCPolicyName );
    
    static IDE_RC getECCPolicyCode( UChar   * aECCPolicyCode,
                                    UShort  * aECCPolicyCodeSize );
    
    static IDE_RC verifyECCPolicyCode( UChar   * aECCPolicyCode,
                                       UShort    aECCPolicyCodeSize,
                                       idBool  * aIsValid );
    
    static IDE_RC getPolicyCode( SChar   * aPolicyName,
                                 UChar   * aPolicyCode,
                                 UShort  * aPolicyCodeLength );
    
    static IDE_RC verifyPolicyCode( SChar  * aPolicyName,
                                    UChar  * aPolicyCode,
                                    UShort   aPolicyCodeLength,
                                    idBool * aIsValid );
    
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

    static IDE_RC getECCSize( UInt    aSize,
                              UInt  * aECCSize );

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

    //-------------------------------------------------
    // 보안 권한 체크 함수 
    //-------------------------------------------------
    
    // BUG-26877
    static IDE_RC checkSecurityPrivilege( qcStatement          * aStatement,
                                          struct qcmTableInfo  * aTableInfo,
                                          struct qcmColumn     * aColumn,
                                          idBool                 aAccessPriv,
                                          idBool                 aEncryptPriv,
                                          idBool                 aDecryptPriv,
                                          idBool               * aIsValid );

    //-------------------------------------------------
    // MT에서 암복호화시 사용되는 callback 함수
    //-------------------------------------------------
    
    static IDE_RC encryptCallback( mtcEncryptInfo  * aEncryptInfo,
                                   SChar           * aPolicyName,
                                   UChar           * aPlain,
                                   UShort            aPlainSize,
                                   UChar           * aCipher,
                                   UShort          * aCipherSize );

    static IDE_RC decryptCallback( mtcEncryptInfo  * aEncryptInfo,
                                   SChar           * aPolicyName,
                                   UChar           * aCipher,
                                   UShort            aCipherSize,
                                   UChar           * aPlain,
                                   UShort          * aPlainSize );

    static IDE_RC encodeECCCallback( mtcECCInfo   * aInfo,
                                     UChar        * aPlain,
                                     UShort         aPlainSize,
                                     UChar        * aCipher,
                                     UShort       * aCipherSize );

    static IDE_RC getDecryptInfoCallback( mtcTemplate     * aTemplate,
                                          UShort            aTable,
                                          UShort            aColumn,
                                          mtcEncryptInfo  * aEncryptInfo );

    static IDE_RC getECCInfoCallback( mtcTemplate * aTemplate,
                                      mtcECCInfo  * aInfo );

    //-------------------------------------------------
    // QP에서 암복호화시 사용되는 함수
    //-------------------------------------------------
    
    static IDE_RC getEncryptInfo( qcStatement          * aStatement,
                                  struct qcmTableInfo  * aTableInfo,
                                  UInt                   aColumnOrder,
                                  mtcEncryptInfo       * aEncryptInfo );

    // add column은 tableInfo에 컬럼이 없으므로 column order가 없다.
    static IDE_RC getEncryptInfo( qcStatement          * aStatement,
                                  struct qcmTableInfo  * aTableInfo,
                                  struct qcmColumn     * aColumn,
                                  mtcEncryptInfo       * aEncryptInfo );

    static IDE_RC getDecryptInfo( mtcTemplate     * aTemplate,
                                  UShort            aTable,
                                  UShort            aColumn,
                                  mtcEncryptInfo  * aDecryptInfo );

    static IDE_RC getECCInfo( qcStatement  * aStatement,
                              mtcECCInfo   * aInfo );

    static IDE_RC encryptColumn( qcStatement          * aStatement,
                                 struct qcmTableInfo  * aTableInfo,
                                 UInt                   aColumnOrder,
                                 mtcColumn            * aSrcColumn,
                                 void                 * aSrcValue,
                                 mtcColumn            * aDestColumn,
                                 void                 * aDestValue );
    
    static IDE_RC decryptColumn( qcStatement          * aStatement,
                                 struct qcmTableInfo  * aTableInfo,
                                 UInt                   aColumnOrder,
                                 mtcColumn            * aSrcColumn,
                                 void                 * aSrcValue,
                                 mtcColumn            * aDestColumn,
                                 void                 * aDestValue );
    
    //-------------------------------------------------
    // 해쉬함수
    //-------------------------------------------------

    static IDE_RC hashB64( SChar  * aHashType,
                           UChar  * aPlain,
                           UShort   aPlanSize,
                           UChar  * aHash,
                           UShort * aHashSize );
                           
    static IDE_RC hashStr( SChar  * aHashType,
                           UChar  * aPlain,
                           UShort   aPlanSize,
                           UChar  * aHash,
                           UShort * aHashSize );
};

#endif // _O_QCS_MODULE_H_
