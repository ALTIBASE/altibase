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

#include <qcs.h>
#include <qcsModule.h>
#include <qcsAltibase.h>

#define ALTIBASE_REVERSE_POLICY_NAME    ((SChar*)"reverse")
#define ALTIBASE_SHIFT_POLICY_NAME      ((SChar*)"shift")
#define ALTIBASE_DOUBLE_POLICY_NAME     ((SChar*)"double")

#define ALTIBASE_VERIFY_CODE            ((UChar*)"test")
#define ALTIBASE_VERIFY_CODE_SIZE       (4)

#define ALTIBASE_MODULE_VERSION         ((SChar*)"altibase 1.0")
#define ALTIBASE_MODULE_VERSION_SIZE    (12)

IDE_RC qcsAltibase::initializeModule( SChar  * aECCPolicyName,
                                      SChar  * aModuleVersion )
{
    IDE_DASSERT( aECCPolicyName != NULL );

    IDE_TEST_RAISE( idlOS::strMatch( aECCPolicyName,
                                     idlOS::strlen( aECCPolicyName ),
                                     ALTIBASE_DOUBLE_POLICY_NAME,
                                     idlOS::strlen( ALTIBASE_DOUBLE_POLICY_NAME ) ) != 0,
                    ERR_NOT_EXIST_ECC_POLICY );

    idlOS::snprintf( aModuleVersion,
                     QCS_MODULE_VERSION_SIZE + 1,
                     "%s",
                     ALTIBASE_MODULE_VERSION );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_ECC_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NOT_EXIST_ECC_POLICY,
                                  aECCPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsAltibase::finalizeModule( void )
{
    return IDE_SUCCESS;
}

IDE_RC qcsAltibase::getPolicyInfo( SChar   * aPolicyName,
                                   idBool  * aIsExist,
                                   idBool  * aIsSalt,
                                   idBool  * aIsEncodeECC )
{
    IDE_DASSERT( aPolicyName  != NULL );
    IDE_DASSERT( aIsExist     != NULL );
    IDE_DASSERT( aIsSalt      != NULL );
    IDE_DASSERT( aIsEncodeECC != NULL );

    if ( idlOS::strMatch( aPolicyName,
                          idlOS::strlen( aPolicyName ),
                          ALTIBASE_REVERSE_POLICY_NAME,
                          idlOS::strlen( ALTIBASE_REVERSE_POLICY_NAME ) ) == 0 )
    {
        *aIsExist     = ID_TRUE;
        *aIsSalt      = ID_FALSE;
        *aIsEncodeECC = ID_FALSE;
    }
    else if ( idlOS::strMatch( aPolicyName,
                               idlOS::strlen( aPolicyName ),
                               ALTIBASE_SHIFT_POLICY_NAME ,
                               idlOS::strlen( ALTIBASE_SHIFT_POLICY_NAME ) ) == 0 )
    {
        *aIsExist     = ID_TRUE;
        *aIsSalt      = ID_FALSE;
        *aIsEncodeECC = ID_FALSE;
    }
    else if ( idlOS::strMatch( aPolicyName,
                               idlOS::strlen( aPolicyName ),
                               ALTIBASE_DOUBLE_POLICY_NAME ,
                               idlOS::strlen( ALTIBASE_DOUBLE_POLICY_NAME ) ) == 0 )
    {
        *aIsExist     = ID_TRUE;
        *aIsSalt      = ID_FALSE;
        *aIsEncodeECC = ID_TRUE;
    }
    else
    {
        *aIsExist     = ID_FALSE;
        *aIsSalt      = ID_FALSE;
        *aIsEncodeECC = ID_FALSE;
    }
    
    return IDE_SUCCESS;
}

IDE_RC qcsAltibase::getEncryptedSize( SChar  * aPolicyName,
                                      UInt     aSize,
                                      UInt   * aEncryptedSize )
{
    IDE_DASSERT( aPolicyName != NULL );
    IDE_DASSERT( aEncryptedSize != NULL );

    if ( idlOS::strMatch( aPolicyName,
                          idlOS::strlen( aPolicyName ),
                          ALTIBASE_REVERSE_POLICY_NAME,
                          idlOS::strlen( ALTIBASE_REVERSE_POLICY_NAME ) ) == 0 )
    {
        *aEncryptedSize = aSize;
    }
    else if ( idlOS::strMatch( aPolicyName,
                               idlOS::strlen( aPolicyName ),
                               ALTIBASE_SHIFT_POLICY_NAME ,
                               idlOS::strlen( ALTIBASE_SHIFT_POLICY_NAME ) ) == 0 )
    {
        *aEncryptedSize = aSize;
    }
    else if ( idlOS::strMatch( aPolicyName,
                               idlOS::strlen( aPolicyName ),
                               ALTIBASE_DOUBLE_POLICY_NAME ,
                               idlOS::strlen( ALTIBASE_DOUBLE_POLICY_NAME ) ) == 0 )
    {
        *aEncryptedSize = aSize + aSize;
    }
    else
    {
        IDE_RAISE( ERR_NOT_EXIST_POLICY );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NOT_EXIST_POLICY,
                                  aPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsAltibase::getColumnPolicy( SChar  * /* aTableOwnerName */,
                                     SChar  * /* aTableName */,
                                     SChar  * /* aColumnName */,
                                     SChar  * /* aPolicyName */ )
{
    return IDE_FAILURE;
}

IDE_RC qcsAltibase::setColumnPolicy( SChar  * /* aTableOwnerName */,
                                     SChar  * /* aTableName */,
                                     SChar  * /* aColumnName */,
                                     SChar  * /* aPolicyName */ )
{
    return IDE_SUCCESS;
}

IDE_RC qcsAltibase::unsetColumnPolicy( SChar  * /* aTableOwnerName */,
                                       SChar  * /* aTableName */,
                                       SChar  * /* aColumnName */ )
{
    return IDE_SUCCESS;
}
    
IDE_RC qcsAltibase::encrypt( SChar   * aPolicyName,
                             UChar   * aPlain,
                             UShort    aPlainSize,
                             UChar   * aCipher,
                             UShort  * aCipherSize )
{
    UShort  i;
    UShort  j;
    
    IDE_DASSERT( aPolicyName != NULL );
    IDE_DASSERT( aPlain      != NULL );
    IDE_DASSERT( aCipher     != NULL );
    IDE_DASSERT( aCipherSize != NULL );

    if ( idlOS::strMatch( aPolicyName,
                          idlOS::strlen( aPolicyName ),
                          ALTIBASE_REVERSE_POLICY_NAME,
                          idlOS::strlen( ALTIBASE_REVERSE_POLICY_NAME ) ) == 0 )
    {
        for ( i = 0, j = aPlainSize - 1; i < aPlainSize; i++, j-- )
        {
            aCipher[j] = aPlain[i];
        }
        
        *aCipherSize = aPlainSize;
    }
    else if ( idlOS::strMatch( aPolicyName,
                               idlOS::strlen( aPolicyName ),
                               ALTIBASE_SHIFT_POLICY_NAME ,
                               idlOS::strlen( ALTIBASE_SHIFT_POLICY_NAME ) ) == 0 )
    {
        for ( i = 1, j = 0; i <= aPlainSize; i++, j++ )
        {
            aCipher[j] = aPlain[i % aPlainSize];
        }
        
        *aCipherSize = aPlainSize;
    }
    else
    {
        IDE_RAISE( ERR_NOT_EXIST_POLICY );
    }
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NOT_EXIST_POLICY,
                                  aPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsAltibase::decrypt( SChar   * aPolicyName,
                             UChar   * aCipher,
                             UShort    aCipherSize,
                             UChar   * aPlain,
                             UShort  * aPlainSize )
{
    UShort  i;
    UShort  j;
    
    IDE_DASSERT( aPolicyName != NULL );
    IDE_DASSERT( aPlain      != NULL );
    IDE_DASSERT( aCipher     != NULL );
    IDE_DASSERT( aPlainSize  != NULL );

    if ( idlOS::strMatch( aPolicyName,
                          idlOS::strlen( aPolicyName ),
                          ALTIBASE_REVERSE_POLICY_NAME,
                          idlOS::strlen( ALTIBASE_REVERSE_POLICY_NAME ) ) == 0 )
    {
        for ( i = 0, j = aCipherSize - 1; i < aCipherSize; i++, j-- )
        {
            aPlain[j] = aCipher[i];
        }
        
        *aPlainSize = aCipherSize;
    }
    else if ( idlOS::strMatch( aPolicyName,
                               idlOS::strlen( aPolicyName ),
                               ALTIBASE_SHIFT_POLICY_NAME ,
                               idlOS::strlen( ALTIBASE_SHIFT_POLICY_NAME ) ) == 0 )
    {
        for ( i = 0, j = 1; i < aCipherSize; i++, j++ )
        {
            aPlain[j % aCipherSize] = aCipher[i];
        }
        
        *aPlainSize = aCipherSize;
    }
    else
    {
        IDE_RAISE( ERR_NOT_EXIST_POLICY );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NOT_EXIST_POLICY,
                                  aPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsAltibase::encodeECC( SChar   * aPolicyName,
                               UChar   * aPlain,
                               UShort    aPlainSize,
                               UChar   * aCipher,
                               UShort  * aCipherSize )
{
    UShort  i;
    UShort  j;
    
    IDE_DASSERT( aPolicyName != NULL );
    IDE_DASSERT( aPlain      != NULL );
    IDE_DASSERT( aCipher     != NULL );
    IDE_DASSERT( aCipherSize != NULL );

    if ( idlOS::strMatch( aPolicyName,
                          idlOS::strlen( aPolicyName ),
                          ALTIBASE_DOUBLE_POLICY_NAME ,
                          idlOS::strlen( ALTIBASE_DOUBLE_POLICY_NAME ) ) == 0 )
    {
        for ( i = 0, j = 0; i < aPlainSize; i++ )
        {
            aCipher[j] = aPlain[i];
            j++;
            
            aCipher[j] = aPlain[i];
            j++;
        }
        
        *aCipherSize = aPlainSize + aPlainSize;
    }
    else
    {
        IDE_RAISE( ERR_NOT_EXIST_POLICY );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NOT_EXIST_POLICY,
                                  aPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsAltibase::getPolicyCode( SChar   * aPolicyName,
                                   UChar   * aPolicyCode,
                                   UShort  * aPolicyCodeLength )
{
    IDE_DASSERT( aPolicyName != NULL );

    if ( ( idlOS::strMatch( aPolicyName,
                            idlOS::strlen( aPolicyName ),
                            ALTIBASE_REVERSE_POLICY_NAME,
                            idlOS::strlen( ALTIBASE_REVERSE_POLICY_NAME ) ) == 0 ) ||
         ( idlOS::strMatch( aPolicyName,
                            idlOS::strlen( aPolicyName ),
                            ALTIBASE_SHIFT_POLICY_NAME ,
                            idlOS::strlen( ALTIBASE_SHIFT_POLICY_NAME ) ) == 0 ) )
    {
        IDE_TEST( encrypt( aPolicyName,
                           ALTIBASE_VERIFY_CODE,
                           ALTIBASE_VERIFY_CODE_SIZE,
                           aPolicyCode,
                           aPolicyCodeLength )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_RAISE( ERR_NOT_EXIST_POLICY );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NOT_EXIST_POLICY,
                                  aPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsAltibase::verifyPolicyCode( SChar   * aPolicyName,
                                      UChar   * aPolicyCode,
                                      UShort    aPolicyCodeLength,
                                      idBool  * aIsValid )
{
    UChar           sCipher[QCS_POLICY_CODE_SIZE + 1];
    UShort          sCipherSize;

    IDE_DASSERT( aPolicyName != NULL );
        
    if ( ( idlOS::strMatch( aPolicyName,
                            idlOS::strlen( aPolicyName ),
                            ALTIBASE_REVERSE_POLICY_NAME,
                            idlOS::strlen( ALTIBASE_REVERSE_POLICY_NAME ) ) == 0 ) ||
         ( idlOS::strMatch( aPolicyName,
                            idlOS::strlen( aPolicyName ),
                            ALTIBASE_SHIFT_POLICY_NAME ,
                            idlOS::strlen( ALTIBASE_SHIFT_POLICY_NAME ) ) == 0 ) )
    {
        IDE_TEST( encrypt( aPolicyName,
                           ALTIBASE_VERIFY_CODE,
                           ALTIBASE_VERIFY_CODE_SIZE,
                           sCipher,
                           & sCipherSize )
                  != IDE_SUCCESS );

        // validation
        IDE_TEST_RAISE( aPolicyCodeLength != sCipherSize,
                        ERR_INVALID_POLICY );

        if( idlOS::memcmp( (void*) aPolicyCode,
                           (void*) sCipher,
                           aPolicyCodeLength ) == 0 )
        {
            *aIsValid = ID_TRUE;
        }
        else
        {
            *aIsValid = ID_FALSE;
        }
    }
    else
    {
        IDE_RAISE( ERR_NOT_EXIST_POLICY );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NOT_EXIST_POLICY,
                                  aPolicyName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_POLICY,
                                  aPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

