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

#include <qcsModule.h>
#include <qcsAltibase.h>
#include <qcsDAmo.h>
#include <qcsCubeOne.h>
#include <qmsParseTree.h>
#include <qcmTableInfo.h>
#include <qtc.h>
#include <qcg.h>

static PDL_SHLIB_HANDLE   gQcsHandle;
    
static idBool             gQcsIsOpened;
static idBool             gQcsIsInitialized;
static SChar              gQcsModuleName[QCS_MODULE_NAME_SIZE + 1];
static SChar              gQcsModuleVersion[QCS_MODULE_VERSION_SIZE + 1];
static SChar              gQcsECCPolicy[QCS_POLICY_NAME_SIZE + 1];

static qcsModuleProvider  gQcsModuleProvider;


IDE_RC qcsModule::initialize( void )
{
    gQcsHandle        = PDL_SHLIB_INVALID_HANDLE;

    gQcsIsOpened      = ID_FALSE;
    gQcsIsInitialized = ID_FALSE;

    gQcsModuleName[0]    = '\0';
    gQcsModuleVersion[0] = '\0';
    gQcsECCPolicy[0]     = '\0';

    gQcsModuleProvider = QCS_MODULE_NONE;

    return IDE_SUCCESS;
}

IDE_RC qcsModule::finalize( void )
{
    if ( gQcsIsOpened == ID_TRUE )
    {
        if ( gQcsHandle != PDL_SHLIB_INVALID_HANDLE )
        {
            (void) idlOS::dlclose( gQcsHandle );

            gQcsHandle = PDL_SHLIB_INVALID_HANDLE;
        }
        else
        {
            // Nothing to do.
        }

        gQcsIsOpened      = ID_FALSE;
        gQcsIsInitialized = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    gQcsModuleProvider = QCS_MODULE_NONE;
    
    return IDE_SUCCESS;
}

IDE_RC qcsModule::openModule( SChar  * aModule,
                              SChar  * aModuleLibrary )
{
    SChar   * sError;
    SChar     sErrorMsg[QCS_ERROR_MSG_SIZE + 1];
    
    IDE_DASSERT( aModule != NULL );
    IDE_DASSERT( aModuleLibrary != NULL );
    
    //--------------------------------------------
    // 이미 module이 open된 경우 close한다.
    //--------------------------------------------
    
    (void) finalize();
    
    //--------------------------------------------
    // module을 open한다.
    //--------------------------------------------

    if ( idlOS::strcasecmp( aModule, "altibase" ) == 0 )
    {
        //--------------------------------------------
        // altibase 내부 모듈 open
        //--------------------------------------------
        
        idlOS::snprintf( gQcsModuleName,
                         QCS_MODULE_NAME_SIZE + 1,
                         "%s",
                         aModule );
        
        gQcsModuleProvider = QCS_MODULE_ALTIBASE;

        gQcsIsOpened = ID_TRUE;
    }
    else
    {
        //--------------------------------------------
        // x. 지원하는 module이 아니면 에러
        //--------------------------------------------
        
        IDE_RAISE( ERR_INVALID_MODULE );        
    }
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DLOPEN );
    {
        sError = idlOS::dlerror();
        
        if ( sError != NULL )
        {
            idlOS::snprintf( sErrorMsg, QCS_ERROR_MSG_SIZE + 1,
                             "%s open error %s",
                             aModule, sError );
        }
        else
        {
            sErrorMsg[0] = '\0';
        }
        
        IDE_SET( ideSetErrorCode( qpERR_ABORT_DLOPEN, sErrorMsg ) );
        //fix BUG-18226.
        errno = 0;
    }
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    if ( gQcsHandle != PDL_SHLIB_INVALID_HANDLE )
    {
        (void) idlOS::dlclose( gQcsHandle );

        gQcsHandle = PDL_SHLIB_INVALID_HANDLE;
    }
    
    if ( gQcsIsOpened == ID_TRUE )
    {
        gQcsIsOpened      = ID_FALSE;
        gQcsIsInitialized = ID_FALSE;
    }
    
    gQcsModuleProvider = QCS_MODULE_NONE;
    
    return IDE_FAILURE;
}

IDE_RC qcsModule::closeModule( void )
{
    SChar   * sError;
    SChar     sErrorMsg[QCS_ERROR_MSG_SIZE + 1];

    if ( gQcsIsOpened == ID_TRUE )
    {
        if ( gQcsHandle != PDL_SHLIB_INVALID_HANDLE )
        {
            IDE_TEST_RAISE( idlOS::dlclose( gQcsHandle ) != QCS_OK,
                            ERR_DLCLOSE );
            
            gQcsHandle = PDL_SHLIB_INVALID_HANDLE;
        }
        else
        {
            // Nothing to do.
        }

        gQcsIsOpened      = ID_FALSE;
        gQcsIsInitialized = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    gQcsModuleName[0]    = '\0';
    gQcsModuleVersion[0] = '\0';
    gQcsECCPolicy[0]     = '\0';

    gQcsModuleProvider = QCS_MODULE_NONE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DLCLOSE );
    {
        sError = idlOS::dlerror();
        
        if ( sError != NULL )
        {
            idlOS::snprintf( sErrorMsg, QCS_ERROR_MSG_SIZE + 1,
                             "close error %s",
                             sError );
        }
        else
        {
            sErrorMsg[0] = '\0';
        }
        
        IDE_SET( ideSetErrorCode( qpERR_ABORT_DLCLOSE, sErrorMsg ) );
        //fix BUG-18226.
        errno = 0;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::verifyModule( void )
{
    IDE_TEST_RAISE( gQcsIsOpened == ID_FALSE,
                    ERR_INVALID_MODULE );

    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::initializeModule( SChar  * aECCPolicyName )
{
    IDE_TEST_RAISE( gQcsIsOpened == ID_FALSE,
                    ERR_INVALID_MODULE );
                    
    IDE_TEST_RAISE( ( aECCPolicyName[0] == '\0' ) ||
                    ( idlOS::strlen( aECCPolicyName ) > QCS_POLICY_NAME_SIZE ),
                    ERR_NOT_EXIST_ECC_POLICY );
    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::initializeModule( aECCPolicyName,
                                                     gQcsModuleVersion )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }

    idlOS::snprintf( gQcsECCPolicy,
                     QCS_POLICY_NAME_SIZE + 1,
                     "%s",
                     aECCPolicyName );
    
    gQcsIsInitialized = ID_TRUE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_ECC_POLICY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NOT_EXIST_ECC_POLICY,
                                  aECCPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::finalizeModule( void )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );
                    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::finalizeModule() != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qcsModule::isInitialized( void )
{
    if ( gQcsIsInitialized == ID_TRUE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}   

IDE_RC qcsModule::getPolicyInfo( SChar   * aPolicyName,
                                 idBool  * aIsExist,
                                 idBool  * aIsSalt,
                                 idBool  * aIsEncodeECC )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );
                    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::getPolicyInfo( aPolicyName,
                                                  aIsExist,
                                                  aIsSalt,
                                                  aIsEncodeECC )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
                                 
IDE_RC qcsModule::getEncryptedSize( SChar  * aPolicyName,
                                    UInt     aSize,
                                    UInt   * aEncryptedSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );
                    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::getEncryptedSize( aPolicyName,
                                                     aSize,
                                                     aEncryptedSize )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }

    IDE_TEST_RAISE( aSize + MTD_ECHAR_DECRYPT_BLOCK_SIZE < *aEncryptedSize,
                    ERR_POLICY_BLOCK_SIZE );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION( ERR_POLICY_BLOCK_SIZE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_POLICY_BLOCK_SIZE_TOO_BIG,
                                  aPolicyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getECCSize( UInt    aSize,
                              UInt  * aECCSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );
                    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::getEncryptedSize( gQcsECCPolicy,
                                                     aSize,
                                                     aECCSize )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getColumnPolicy( SChar  * aTableOwnerName,
                                   SChar  * aTableName,
                                   SChar  * aColumnName,
                                   SChar  * aPolicyName )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );
                    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::getColumnPolicy( aTableOwnerName,
                                                    aTableName,
                                                    aColumnName,
                                                    aPolicyName )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::setColumnPolicy( SChar  * aTableOwnerName,
                                   SChar  * aTableName,
                                   SChar  * aColumnName,
                                   SChar  * aPolicyName )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );
                    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::setColumnPolicy( aTableOwnerName,
                                                    aTableName,
                                                    aColumnName,
                                                    aPolicyName )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::unsetColumnPolicy( SChar  * aTableOwnerName,
                                     SChar  * aTableName,
                                     SChar  * aColumnName )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );
                    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::unsetColumnPolicy( aTableOwnerName,
                                                      aTableName,
                                                      aColumnName )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::encrypt( mtcEncryptInfo * aInfo,
                           SChar          * aPolicyName,
                           UChar          * aPlain,
                           UShort           aPlainSize,
                           UChar          * aCipher,
                           UShort         * aCipherSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    IDE_ASSERT( aPlainSize != 0 );
    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::encrypt( aPolicyName,
                                            aPlain,
                                            aPlainSize,
                                            aCipher,
                                            aCipherSize )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }

    IDE_ASSERT( *aCipherSize != 0 );

#if defined(DEBUG)
    ideLog::log( IDE_QP_0,"[SECURITY] Info : (%s)(%s)(%s)(%s)(%s)(%s), Encrypt : (%.*s)->(XXX)",
                 aInfo->ipAddr,
                 aInfo->userName,
                 aInfo->tableOwnerName,
                 aInfo->tableName,
                 aInfo->columnName,
                 aInfo->stmtType,
                 aPlainSize,
                 aPlain );
#endif
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::decrypt( mtcEncryptInfo * aInfo,
                           SChar          * aPolicyName,
                           UChar          * aCipher,
                           UShort           aCipherSize,
                           UChar          * aPlain,
                           UShort         * aPlainSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    IDE_ASSERT( aCipherSize != 0 );
    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::decrypt( aPolicyName,
                                            aCipher,
                                            aCipherSize,
                                            aPlain,
                                            aPlainSize )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    IDE_ASSERT( *aPlainSize != 0 );

#if defined(DEBUG)
    ideLog::log( IDE_QP_0,"[SECURITY] Info : (%s)(%s)(%s)(%s)(%s)(%s), Decrypt : (XXX)->(%.*s)",
                 aInfo->ipAddr,
                 aInfo->userName,
                 aInfo->tableOwnerName,
                 aInfo->tableName,
                 aInfo->columnName,
                 aInfo->stmtType,
                 *aPlainSize,
                 aPlain ); 
#endif
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::encodeECC( mtcECCInfo * aInfo,
                             UChar      * aPlain,
                             UShort       aPlainSize,
                             UChar      * aCipher,
                             UShort     * aCipherSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    IDE_ASSERT( aPlainSize != 0 );
    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::encodeECC( gQcsECCPolicy,
                                              aPlain,
                                              aPlainSize,
                                              aCipher,
                                              aCipherSize )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }

    IDE_ASSERT( *aCipherSize != 0 );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getModuleName( SChar  * aModuleName )
{
    IDE_TEST_RAISE( gQcsIsOpened == ID_FALSE,
                    ERR_INVALID_MODULE );
    
    idlOS::snprintf( aModuleName,
                     QCS_MODULE_NAME_SIZE + 1,
                     "%s",
                     gQcsModuleName );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getModuleVersion( SChar  * aModuleVersion )
{
    IDE_TEST_RAISE( gQcsIsOpened == ID_FALSE,
                    ERR_INVALID_MODULE );
    
    idlOS::snprintf( aModuleVersion,
                     QCS_MODULE_VERSION_SIZE + 1,
                     "%s",
                     gQcsModuleVersion );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getECCPolicyName( SChar  * aECCPolicyName )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    idlOS::snprintf( aECCPolicyName,
                     QCS_POLICY_NAME_SIZE + 1,
                     "%s",
                     gQcsECCPolicy );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getECCPolicyCode( UChar   * aECCPolicyCode,
                                    UShort  * aECCPolicyCodeSize )
{
    mtcECCInfo  sInfo;
    UChar       sCipher[QCS_ECC_POLICY_CODE_SIZE + 1];
    UShort      sCipherSize;
    
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    sInfo.sessionID = 0;  // internal session id
    
    // encode
    IDE_TEST( encodeECC( & sInfo,
                         QCS_POLICY_CODE_TEXT,
                         QCS_POLICY_CODE_TEXT_SIZE,
                         sCipher,
                         & sCipherSize )
              != IDE_SUCCESS );

    IDE_ASSERT( sCipherSize <= QCS_ECC_POLICY_CODE_SIZE );

    // byte to string
    IDE_TEST( byteToString( sCipher,
                            sCipherSize,
                            aECCPolicyCode,
                            aECCPolicyCodeSize )
              != IDE_SUCCESS );
    
    IDE_ASSERT( *aECCPolicyCodeSize <= QCS_ECC_POLICY_CODE_SIZE );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::verifyECCPolicyCode( UChar   * aECCPolicyCode,
                                       UShort    aECCPolicyCodeSize,
                                       idBool  * aIsValid )
{
    UChar   sECCPolicyCode[QCS_ECC_POLICY_CODE_SIZE + 1];
    UShort  sECCPolicyCodeSize;
    
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    // make ECC policy code
    IDE_TEST( getECCPolicyCode( sECCPolicyCode,
                                & sECCPolicyCodeSize )
              != IDE_SUCCESS );

    // validation
    if ( ( sECCPolicyCodeSize == aECCPolicyCodeSize ) &&
         ( idlOS::memcmp( (void*) sECCPolicyCode,
                          (void*) aECCPolicyCode,
                          aECCPolicyCodeSize ) == 0 ) )
    {
        *aIsValid = ID_TRUE;
    }
    else
    {
        *aIsValid = ID_FALSE;    
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getPolicyCode( SChar   * aPolicyName,
                                 UChar   * aPolicyCode,
                                 UShort  * aPolicyCodeLength )
{
    UChar           sCipher[QCS_POLICY_CODE_SIZE + 1];
    UShort          sCipherSize;

    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            IDE_TEST( qcsAltibase::getPolicyCode( aPolicyName,
                                                  sCipher,
                                                  & sCipherSize )
                      != IDE_SUCCESS );
            break;
            
        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    // byte to string
    IDE_TEST( byteToString( sCipher,
                            sCipherSize,
                            aPolicyCode,
                            aPolicyCodeLength )
              != IDE_SUCCESS );

    IDE_ASSERT( *aPolicyCodeLength <= QCS_POLICY_CODE_SIZE );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::verifyPolicyCode( SChar   * aPolicyName,
                                    UChar   * aPolicyCode,
                                    UShort    aPolicyCodeLength,
                                    idBool  * aIsValid )
{
    UChar           sCipher[QCS_POLICY_CODE_SIZE + 1];
    UShort          sCipherSize;
    
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    IDE_ASSERT( aPolicyCodeLength <= QCS_POLICY_CODE_SIZE );

    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            *aIsValid = ID_TRUE;            
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::byteToString( UChar   * aByte,
                                UShort    aByteSize,
                                UChar   * aString,
                                UShort  * aStringSize )
{
    SChar   sByteValue;
    SChar   sStringValue;
    SInt    sByteIterator;
    SInt    sStringIterator;
    
    for( sByteIterator = 0, sStringIterator = 0;
         sByteIterator < (SInt) aByteSize;
         sByteIterator++, sStringIterator += 2)
    {
        sByteValue = aByte[sByteIterator];

        sStringValue = (sByteValue & 0xF0) >> 4;
        aString[sStringIterator+0] =
            (sStringValue < 10) ? (sStringValue + '0') : (sStringValue + 'A' - 10);
        
        sStringValue = (sByteValue & 0x0F);
        aString[sStringIterator+1] =
            (sStringValue < 10) ? (sStringValue + '0') : (sStringValue + 'A' - 10);
    }

    *aStringSize = (UShort) sStringIterator;

    return IDE_SUCCESS;
}

IDE_RC qcsModule::stringToByte( UChar   * aString,
                                UShort    aStringSize,
                                UChar   * aByte,
                                UShort  * aByteSize )
{
    static const UChar sHex[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };
    
    SInt    sStringIterator;
    SInt    sByteIterator;

    IDE_TEST_RAISE( aStringSize % 2 != 0, ERR_INVALID_MODULE );

    for( sStringIterator = 0, sByteIterator = 0;
         sStringIterator < aStringSize;
         sStringIterator += 2, sByteIterator++ )
    {
        IDE_TEST_RAISE( sHex[aString[sStringIterator+0]] > 15,
                        ERR_INVALID_MODULE );
        IDE_TEST_RAISE( sHex[aString[sStringIterator+1]] > 15,
                        ERR_INVALID_MODULE );
            
        aByte[sByteIterator] =
            sHex[aString[sStringIterator+0]] * 16
            + sHex[aString[sStringIterator+1]];
    }

    *aByteSize = (UShort) sByteIterator;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::encryptCallback( mtcEncryptInfo  * aEncryptInfo,
                                   SChar           * aPolicyName,
                                   UChar           * aPlain,
                                   UShort            aPlainSize,
                                   UChar           * aCipher,
                                   UShort          * aCipherSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    IDE_TEST_RAISE( aPolicyName[0] == '\0',
                    ERR_INVALID_ENCRYPTED_COLUMN );
    
    IDE_TEST( encrypt( aEncryptInfo,
                       aPolicyName,
                       aPlain,
                       aPlainSize,
                       aCipher,
                       aCipherSize )
              != IDE_SUCCESS );
              
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_COLUMN );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::decryptCallback( mtcEncryptInfo  * aDecryptInfo,
                                   SChar           * aPolicyName,
                                   UChar           * aCipher,
                                   UShort            aCipherSize,
                                   UChar           * aPlain,
                                   UShort          * aPlainSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    IDE_TEST_RAISE( aPolicyName[0] == '\0',
                    ERR_INVALID_ENCRYPTED_COLUMN );

    IDE_TEST( decrypt( aDecryptInfo,
                       aPolicyName,
                       aCipher,
                       aCipherSize,
                       aPlain,
                       aPlainSize )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_COLUMN );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::encodeECCCallback( mtcECCInfo   * aECCInfo,
                                     UChar        * aPlain,
                                     UShort         aPlainSize,
                                     UChar        * aCipher,
                                     UShort       * aCipherSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    IDE_TEST( encodeECC( aECCInfo,
                         aPlain,
                         aPlainSize,
                         aCipher,
                         aCipherSize )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getDecryptInfoCallback( mtcTemplate     * aTemplate,
                                          UShort            aTable,
                                          UShort            aColumn,
                                          mtcEncryptInfo  * aDecryptInfo )
{
    IDE_TEST( getDecryptInfo( aTemplate,
                              aTable,
                              aColumn,
                              aDecryptInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getECCInfoCallback( mtcTemplate * aTemplate,
                                      mtcECCInfo  * aInfo )
{
    qcTemplate     * sTemplate;
    
    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aInfo != NULL );

    sTemplate = (qcTemplate*) aTemplate;
    
    IDE_TEST( getECCInfo( sTemplate->stmt,
                          aInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getEncryptInfo( qcStatement     * aStatement,
                                  qcmTableInfo    * aTableInfo,
                                  UInt              aColumnOrder,
                                  mtcEncryptInfo  * aEncryptInfo )
{
    IDE_TEST_RAISE( aTableInfo == NULL,
                    ERR_INVALID_ENCRYPTED_COLUMN );

    IDE_TEST_RAISE( aColumnOrder >= aTableInfo->columnCount,
                    ERR_INVALID_ENCRYPTED_COLUMN );

    IDE_TEST( getEncryptInfo( aStatement,
                              aTableInfo,
                              & aTableInfo->columns[aColumnOrder],
                              aEncryptInfo )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_COLUMN );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getEncryptInfo( qcStatement     * aStatement,
                                  qcmTableInfo    * aTableInfo,
                                  qcmColumn       * aColumn,
                                  mtcEncryptInfo  * aEncryptInfo )
{
    static SChar    * sSelectStr = (SChar*)"SELECT";
    static SChar    * sInsertStr = (SChar*)"INSERT";
    static SChar    * sUpdateStr = (SChar*)"UPDATE";
    static SChar    * sDeleteStr = (SChar*)"DELETE";
    static SChar    * sMoveStr   = (SChar*)"MOVE";
    static SChar    * sDDLStr    = (SChar*)"DDL";
    static SChar    * sEtcStr    = (SChar*)"ETC";

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableInfo != NULL );
    IDE_DASSERT( aColumn != NULL );
    IDE_DASSERT( aEncryptInfo != NULL );

    IDE_TEST_RAISE( aTableInfo == NULL,
                    ERR_INVALID_ENCRYPTED_COLUMN );

    IDE_TEST_RAISE( aColumn == NULL,
                    ERR_INVALID_ENCRYPTED_COLUMN );

    // session info
    aEncryptInfo->sessionID = QCG_GET_SESSION_ID( aStatement );
    aEncryptInfo->ipAddr    = QCG_GET_SESSION_LOGIN_IP( aStatement );
    aEncryptInfo->userName  = QCG_GET_SESSION_USER_NAME( aStatement );

    // statement info
    switch( aStatement->myPlan->parseTree->stmtKind )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
            aEncryptInfo->stmtType = sSelectStr;
            break;

        case QCI_STMT_INSERT:
            aEncryptInfo->stmtType = sInsertStr;
            break;

        case QCI_STMT_UPDATE:
            aEncryptInfo->stmtType = sUpdateStr;
            break;

        case QCI_STMT_DELETE:
            aEncryptInfo->stmtType = sDeleteStr;
            break;

        case QCI_STMT_MOVE:
            aEncryptInfo->stmtType = sMoveStr;
            break;

        case QCI_STMT_SCHEMA_DDL:
        case QCI_STMT_NON_SCHEMA_DDL:
            aEncryptInfo->stmtType = sDDLStr;
            break;

        default:
            aEncryptInfo->stmtType = sEtcStr;
            break;
    }    
    
    // column info
    aEncryptInfo->tableOwnerName = aTableInfo->tableOwnerName;
    aEncryptInfo->tableName      = aTableInfo->name;
    aEncryptInfo->columnName     = aColumn->name;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_COLUMN );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getDecryptInfo( mtcTemplate     * aTemplate,
                                  UShort            aTable,
                                  UShort            aColumn,
                                  mtcEncryptInfo  * aDecryptInfo )
{
    qcTemplate     * sTemplate;
    qmsFrom        * sFrom;
    qcmTableInfo   * sTableInfo;

    sTemplate = (qcTemplate*) aTemplate;
    sFrom = sTemplate->tableMap[aTable].from;
    
    IDE_TEST_RAISE( sFrom == NULL,
                    ERR_INVALID_ENCRYPTED_COLUMN );
    
    sTableInfo = sFrom->tableRef->tableInfo;
    
    IDE_TEST( getEncryptInfo( sTemplate->stmt,
                              sTableInfo,
                              (UInt) aColumn,
                              aDecryptInfo )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_COLUMN );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::getECCInfo( qcStatement  * aStatement,
                              mtcECCInfo   * aInfo )
{
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aInfo != NULL );

    // session info
    aInfo->sessionID = QCG_GET_SESSION_ID( aStatement );
    
    return IDE_SUCCESS;
}

IDE_RC qcsModule::encryptColumn( qcStatement   * aStatement,
                                 qcmTableInfo  * aTableInfo,
                                 UInt            aColumnOrder,
                                 mtcColumn     * aSrcColumn,
                                 void          * aSrcValue,
                                 mtcColumn     * aDestColumn,
                                 void          * aDestValue )
{
    mtdCharType    * sCharValue;
    mtdEcharType   * sEcharValue;
    mtcEncryptInfo   sInfo;
    mtcECCInfo       sECCInfo;
    UShort           sLength;

    IDE_DASSERT( aSrcColumn != NULL );
    IDE_ASSERT( aSrcValue != NULL );
    IDE_DASSERT( aDestColumn != NULL );
    IDE_DASSERT( aDestValue != NULL );
    
    // char->echar, varchar->evarchar가 아니라면 에러
    IDE_TEST_RAISE( ( (aSrcColumn->module->id != MTD_CHAR_ID) ||
                      (aDestColumn->module->id != MTD_ECHAR_ID) )
                    &&
                    ( (aSrcColumn->module->id != MTD_VARCHAR_ID) ||
                      (aDestColumn->module->id != MTD_EVARCHAR_ID) ),
                    ERR_INVALID_ENCRYPTED_COLUMN );
    
    // 동일한 precision이 아니라면 에러
    IDE_TEST_RAISE( aSrcColumn->precision != aDestColumn->precision,
                    ERR_INVALID_ENCRYPTED_COLUMN );

    sCharValue  = (mtdCharType*) aSrcValue;
    sEcharValue = (mtdEcharType*) aDestValue;
    
    if ( aDestColumn->policy[0] != '\0' )
    {
        if ( sCharValue->length > 0 )
        {
            // encrypt cipher value
            IDE_TEST( getEncryptInfo( aStatement,
                                      aTableInfo,
                                      aColumnOrder,
                                      & sInfo )
                      != IDE_SUCCESS );

            IDE_TEST( encryptCallback( & sInfo,
                                       aDestColumn->policy,
                                       sCharValue->value,
                                       sCharValue->length,
                                       sEcharValue->mValue,
                                       & sEcharValue->mCipherLength )
                      != IDE_SUCCESS );
            
            IDE_ASSERT( sEcharValue->mCipherLength <=
                        aDestColumn->encPrecision );

            // encode ecc value
            if ( aDestColumn->module->id == MTD_ECHAR_ID )
            {
                // sCharValue에서 space pading을 제외한 길이를 찾는다.
                for( sLength = sCharValue->length; sLength > 1; sLength-- )
                {
                    if( sCharValue->value[sLength - 1] != ' ' )
                    {
                        break;
                    }
                }
            }
            else
            {
                sLength = sCharValue->length;
            }

            IDE_TEST( getECCInfo( aStatement,
                                  & sECCInfo )
                      != IDE_SUCCESS );
            
            IDE_TEST( encodeECCCallback( & sECCInfo,
                                         sCharValue->value,
                                         sLength,
                                         sEcharValue->mValue +
                                         sEcharValue->mCipherLength,
                                         & sEcharValue->mEccLength )
                      != IDE_SUCCESS );
            
            IDE_ASSERT( sEcharValue->mCipherLength +
                        sEcharValue->mEccLength <=
                        aDestColumn->encPrecision );
        }
        else
        {
            sEcharValue->mCipherLength = 0;
            sEcharValue->mEccLength = 0;
        }
    }
    else
    {
        sEcharValue->mCipherLength = sCharValue->length;

        if ( sEcharValue->mCipherLength > 0 )
        {
            // copy cipher value
            idlOS::memcpy( sEcharValue->mValue,
                           sCharValue->value,
                           sEcharValue->mCipherLength );
            
            IDE_ASSERT( sEcharValue->mCipherLength <=
                        aDestColumn->encPrecision );
            
            if ( aDestColumn->module->id == MTD_ECHAR_ID )
            {
                // sCharValue에서 space pading을 제외한 길이를 찾는다.
                for( sLength = sCharValue->length; sLength > 1; sLength-- )
                {
                    if( sCharValue->value[sLength - 1] != ' ' )
                    {
                        break;
                    }
                }
            }
            else 
            {
                sLength = sCharValue->length;
            }

            IDE_TEST( getECCInfo( aStatement,
                                  & sECCInfo )
                      != IDE_SUCCESS );
            
            // encode ecc value
            IDE_TEST( encodeECCCallback( & sECCInfo,
                                         sCharValue->value,
                                         sLength,
                                         sEcharValue->mValue +
                                         sEcharValue->mCipherLength,
                                         & sEcharValue->mEccLength )
                      != IDE_SUCCESS );
            
            IDE_ASSERT( sEcharValue->mCipherLength +
                        sEcharValue->mEccLength <=
                        aDestColumn->encPrecision );
        }
        else
        {
            sEcharValue->mEccLength = 0;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_COLUMN );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
IDE_RC qcsModule::decryptColumn( qcStatement   * aStatement,
                                 qcmTableInfo  * aTableInfo,
                                 UInt            aColumnOrder,
                                 mtcColumn     * aSrcColumn,
                                 void          * aSrcValue,
                                 mtcColumn     * aDestColumn,
                                 void          * aDestValue )
{
    mtdEcharType   * sEcharValue;
    mtdCharType    * sCharValue;
    mtcEncryptInfo   sInfo;

    IDE_DASSERT( aSrcColumn != NULL );
    IDE_ASSERT( aSrcValue != NULL );
    IDE_DASSERT( aDestColumn != NULL );
    IDE_DASSERT( aDestValue != NULL );

    // echar->char, evarchar->varchar가 아니라면 에러
    IDE_TEST_RAISE( ( (aSrcColumn->module->id != MTD_ECHAR_ID) ||
                      (aDestColumn->module->id != MTD_CHAR_ID) )
                    &&
                    ( (aSrcColumn->module->id != MTD_EVARCHAR_ID) ||
                      (aDestColumn->module->id != MTD_VARCHAR_ID) ),
                    ERR_INVALID_ENCRYPTED_COLUMN );

    // 동일한 precision이 아니라면 에러
    IDE_TEST_RAISE( aSrcColumn->precision != aDestColumn->precision,
                    ERR_INVALID_ENCRYPTED_COLUMN );

    sEcharValue = (mtdEcharType*) aSrcValue;
    sCharValue  = (mtdCharType*) aDestValue;

    if ( aSrcColumn->policy[0] != '\0' )
    {
        // decrypt cipher value
        if ( sEcharValue->mCipherLength > 0 )
        {
            IDE_TEST( getEncryptInfo( aStatement,
                                      aTableInfo,
                                      aColumnOrder,
                                      & sInfo )
                      != IDE_SUCCESS );
            
            IDE_TEST( decryptCallback( & sInfo,
                                       aSrcColumn->policy,
                                       sEcharValue->mValue,
                                       sEcharValue->mCipherLength,
                                       sCharValue->value,
                                       & sCharValue->length )
                      != IDE_SUCCESS );
            
            IDE_ASSERT( sCharValue->length <=
                        aDestColumn->precision );
        }
        else
        {
            sCharValue->length = 0;
        }
    }
    else
    {
        sCharValue->length = sEcharValue->mCipherLength;

         // copy cipher value
        if ( sCharValue->length > 0 )
        {
            idlOS::memcpy( sCharValue->value,
                           sEcharValue->mValue,
                           sCharValue->length );
            
            IDE_ASSERT( sCharValue->length <=
                        aDestColumn->precision );
        }
        else
        {
            // Nothing to do.
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_COLUMN );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
IDE_RC qcsModule::checkSecurityPrivilege( qcStatement   * aStatement,
                                          qcmTableInfo  * aTableInfo,
                                          qcmColumn     * aColumn,
                                          idBool          aAccessPriv,
                                          idBool          aEncryptPriv,
                                          idBool          aDecryptPriv,
                                          idBool        * aIsValid )
{
    SChar  * sIpAddr;
    SChar  * sUserName;
    SChar  * sTableOwnerName;
    SChar  * sTableName;
    SChar  * sColumnName;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableInfo != NULL );
    IDE_DASSERT( aColumn != NULL );

    *aIsValid = ID_FALSE;
    
    IDE_TEST_RAISE( aTableInfo == NULL,
                    ERR_INVALID_ENCRYPTED_COLUMN );

    IDE_TEST_RAISE( aColumn == NULL,
                    ERR_INVALID_ENCRYPTED_COLUMN );

    // session info
    sIpAddr   = QCG_GET_SESSION_LOGIN_IP( aStatement );
    sUserName = QCG_GET_SESSION_USER_NAME( aStatement );

    // column info
    sTableOwnerName = aTableInfo->tableOwnerName;
    sTableName      = aTableInfo->name;
    sColumnName     = aColumn->name;
    
    switch ( gQcsModuleProvider )
    {
        case QCS_MODULE_ALTIBASE:
            *aIsValid = ID_TRUE;
            break;

        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_ENCRYPTED_COLUMN );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::hashB64( SChar  * aHashType,
                           UChar  * aPlain,
                           UShort   aPlainSize,
                           UChar  * aHash,
                           UShort * aHashSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    switch ( gQcsModuleProvider )
    {
        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcsModule::hashStr( SChar  * aHashType,
                           UChar  * aPlain,
                           UShort   aPlainSize,
                           UChar  * aHash,
                           UShort * aHashSize )
{
    IDE_TEST_RAISE( gQcsIsInitialized == ID_FALSE,
                    ERR_INVALID_MODULE );

    switch ( gQcsModuleProvider )
    {
        default:
            IDE_RAISE( ERR_INVALID_MODULE );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_MODULE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
