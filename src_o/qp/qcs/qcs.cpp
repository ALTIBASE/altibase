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

#include <idl.h>
#include <smi.h>
#include <qcg.h>
#include <qcm.h>
#include <qcuProperty.h>
#include <qcs.h>
#include <qcsModule.h>

IDE_RC qcs::initialize( idvSQL * aStatistics )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *    1. meta 확인 (module name, module version, ecc policy name)
 *    2. openModule (모듈 열기)
 *    3. verifyModule (모듈에 따른 인증)
 *    4. initializeModule (모듈에 따른 초기화)
 *
 ***********************************************************************/

    SChar           sModuleVersion[QCS_MODULE_VERSION_SIZE + 1]      = { 0, };
    SChar           sMetaModuleName[QCS_MODULE_NAME_SIZE + 1]        = { 0, };
    SChar           sMetaModuleVersion[QCS_MODULE_VERSION_SIZE + 1]  = { 0, };
    SChar           sMetaECCPolicyName[QCS_POLICY_NAME_SIZE + 1]     = { 0, };
    UChar           sMetaECCPolicyCode[QCS_ECC_POLICY_CODE_SIZE + 1] = { 0, };
    UShort          sMetaECCPolicyCodeSize;
    idBool          sIsExist;
    idBool          sIsOpened = ID_FALSE;
    idBool          sIsInitialized = ID_FALSE;
    idBool          sIsValid;
    
    smiTrans        sTrans;
    smiStatement    sSmiStmt;
    smiStatement  * sDummySmiStmt;
    idBool          sTxInited = ID_FALSE;
    idBool          sTxBegined = ID_FALSE;
    idBool          sStmtBegined = ID_FALSE;
    
    //----------------------------------------------
    // property 확인
    //----------------------------------------------
    
    IDE_TEST_RAISE( ( idlOS::strlen( QCU_SECURITY_MODULE_NAME ) == 0 ) ||
                    ( idlOS::strlen( QCU_SECURITY_ECC_POLICY_NAME ) == 0 ),
                    ERR_FAIL_TO_OPEN_SECURITY_MODULE );
    
    //----------------------------------------------
    // meta 확인
    //----------------------------------------------
    
    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sTxInited = ID_TRUE;
    
    IDE_TEST( sTrans.begin( &sDummySmiStmt, aStatistics )
              != IDE_SUCCESS);
    sTxBegined = ID_TRUE;

    IDE_TEST( sSmiStmt.begin( aStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    sStmtBegined = ID_TRUE;

    IDE_TEST( getMetaSecurity( & sSmiStmt,
                               & sIsExist,
                               sMetaModuleName,
                               sMetaModuleVersion,
                               sMetaECCPolicyName,
                               sMetaECCPolicyCode,
                               & sMetaECCPolicyCodeSize )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sIsExist == ID_FALSE, NORMAL_EXIT );

    //----------------------------------------------
    // initialize 시작
    //----------------------------------------------
    
    ideLog::log( IDE_QP_0,"\n ==> Initialize Security Module" );
    
    //----------------------------------------------
    // meta 검사
    //----------------------------------------------

    ideLog::log( IDE_QP_0,"\n ==> Check Security Properties" );

    // module name 확인
    IDE_TEST_RAISE( idlOS::strMatch( sMetaModuleName,
                                     idlOS::strlen( sMetaModuleName ),
                                     QCU_SECURITY_MODULE_NAME,
                                     idlOS::strlen( QCU_SECURITY_MODULE_NAME ) ) != 0,
                    ERR_MISMATCH_SECURITY_MODULE_NAME );

    // ecc policy 확인
    IDE_TEST_RAISE( idlOS::strMatch( sMetaECCPolicyName,
                                     idlOS::strlen( sMetaECCPolicyName ),
                                     QCU_SECURITY_ECC_POLICY_NAME,
                                     idlOS::strlen( QCU_SECURITY_ECC_POLICY_NAME ) ) != 0,
                    ERR_MISMATCH_SECURITY_MODULE_ECC_POLICY );

    ideLog::log( IDE_QP_0,"\n ... Security Module Name : [%s], "
                 "ECC Policy Name : [%s]",
                 QCU_SECURITY_MODULE_NAME,
                 QCU_SECURITY_ECC_POLICY_NAME );
    
    //----------------------------------------------
    // 모듈 열기
    //----------------------------------------------

    ideLog::log( IDE_QP_0,"\n ==> Open External Security Module" );
    
    IDE_TEST( qcsModule::openModule( QCU_SECURITY_MODULE_NAME,
                                     QCU_SECURITY_MODULE_LIBRARY )
              != IDE_SUCCESS );
    
    sIsOpened = ID_TRUE;

    //----------------------------------------------
    // 모듈 인증
    //----------------------------------------------

    ideLog::log( IDE_QP_0,"\n ==> Verify External Security Module" );
    
    IDE_TEST( qcsModule::verifyModule() != IDE_SUCCESS );

    //----------------------------------------------
    // 모듈 초기화
    //----------------------------------------------

    ideLog::log( IDE_QP_0,"\n ==> Initialize External Security Module" );
    
    IDE_TEST( qcsModule::initializeModule( QCU_SECURITY_ECC_POLICY_NAME )
              != IDE_SUCCESS );
    
    sIsInitialized = ID_TRUE;

    ideLog::log( IDE_QP_0,"\n ... ECC Policy Name : [%s]",
                 QCU_SECURITY_ECC_POLICY_NAME );

    //----------------------------------------------
    // 모듈 version 검사
    //----------------------------------------------

    ideLog::log( IDE_QP_0,"\n ==> Verify Module Version" );

    IDE_TEST( qcsModule::getModuleVersion( sModuleVersion )
              != IDE_SUCCESS );

    IDE_DASSERT( sModuleVersion[0] != '\0' );

    IDE_TEST_RAISE( idlOS::strMatch( sMetaModuleVersion,
                                     idlOS::strlen( sMetaModuleVersion ),
                                     sModuleVersion,
                                     idlOS::strlen( sModuleVersion ) ) != 0,
                    ERR_MISMATCH_SECURITY_MODULE_VERSION );

    ideLog::log( IDE_QP_0,"\n ... Security Module Version : [%s]",
                 sModuleVersion );
    
    //----------------------------------------------
    // ECC policy 검사
    //----------------------------------------------

    ideLog::log( IDE_QP_0,"\n ==> Verify ECC Policy" );
    
    IDE_TEST( qcsModule::verifyECCPolicyCode( sMetaECCPolicyCode,
                                              sMetaECCPolicyCodeSize,
                                              & sIsValid )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sIsValid == ID_FALSE,
                    ERR_INVALID_ECC_POLICY );
    
    //----------------------------------------------
    // policy 검사
    //----------------------------------------------

    ideLog::log( IDE_QP_0,"\n ==> Verify Encrypted Column Policy" );
    
    IDE_TEST( verifyEncryptedColumnPolicy( & sSmiStmt )
              != IDE_SUCCESS );    
    
    ideLog::log( IDE_QP_0," ... [SUCCESS]" );
    
    //----------------------------------------------
    // 종료
    //----------------------------------------------
    
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    sStmtBegined = ID_FALSE;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );

    sTxBegined = ID_FALSE;
    IDE_TEST( sTrans.commit()
              != IDE_SUCCESS );

    sTxInited = ID_FALSE;
    IDE_TEST( sTrans.destroy( aStatistics )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_FAIL_TO_OPEN_SECURITY_MODULE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_FAIL_TO_OPEN_SECURITY_MODULE));
    }
    IDE_EXCEPTION(ERR_MISMATCH_SECURITY_MODULE_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MISMATCH_SECURITY_MODULE,
                                sMetaModuleName,
                                sMetaModuleVersion,
                                sMetaECCPolicyName ));

        ideLog::log( IDE_QP_0,"\n Mismatch Security Module Name: [%s]",
                     sMetaModuleName );
    }
    IDE_EXCEPTION(ERR_MISMATCH_SECURITY_MODULE_VERSION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MISMATCH_SECURITY_MODULE,
                                sMetaModuleName,
                                sMetaModuleVersion,
                                sMetaECCPolicyName ));
        
        ideLog::log( IDE_QP_0,"\n Mismatch Security Module Version: [%s]",
                     sMetaModuleVersion );
    }
    IDE_EXCEPTION(ERR_MISMATCH_SECURITY_MODULE_ECC_POLICY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MISMATCH_SECURITY_MODULE,
                                sMetaModuleName,
                                sMetaModuleVersion,
                                sMetaECCPolicyName ));

        ideLog::log( IDE_QP_0,"\n Mismatch ECC Policy Name: [%s]",
                     sMetaECCPolicyName );
    }
    IDE_EXCEPTION(ERR_INVALID_ECC_POLICY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_ECC_POLICY,
                                QCU_SECURITY_ECC_POLICY_NAME));
    }
    IDE_EXCEPTION_END;
    
    if ( sStmtBegined == ID_TRUE )
    {
        (void) sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS );
    }

    if ( sTxBegined == ID_TRUE )
    {
        (void) sTrans.commit();
    }

    if ( sTxInited == ID_TRUE )
    {
        (void) sTrans.destroy( aStatistics );
    }
    else
    {
        /* Nothing to do */
    }
    
    if ( sIsInitialized == ID_TRUE )
    {
        (void) qcsModule::finalizeModule();
    }
    
    if ( sIsOpened == ID_TRUE )
    {
        (void) qcsModule::closeModule();
    }

    return IDE_FAILURE;
}

IDE_RC qcs::finalize( void )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *    1. finalizeModule (모듈 종료)
 *    2. closeModule (모듈 닫기)
 *
 ***********************************************************************/
    
    if ( qcsModule::isInitialized() == ID_TRUE )
    {
        //----------------------------------------------
        // 모듈 종료
        //----------------------------------------------

        ideLog::log( IDE_QP_0,"\n ==> Finalize Security Module" );
        
        IDE_TEST( qcsModule::finalizeModule() != IDE_SUCCESS );

        //----------------------------------------------
        // 모듈 닫기
        //----------------------------------------------
        
        (void) qcsModule::closeModule();
        
        ideLog::log( IDE_QP_0," ... [SUCCESS]" );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcs::startSecurity( qcStatement  * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *    최초 보안모듈을 설정하고 start하는 경우
 *      0. meta가 없음
 *      1. openModule (모듈 열기)
 *      2. verifyModule (모듈에 따른 인증)
 *      3. initializeModule (모듈에 따른 초기화)
 *      4. column에 사용된 policy 검사
 *      5. insert meta
 *
 *    server start시 보안모듈의 초기화가 실패하여 start하는 경우
 *      0. 보안모듈의 초기화가 안되어 있음
 *      1. openModule (모듈 열기)
 *      2. verifyModule (모듈에 따른 인증)
 *      3. initializeModule (모듈에 따른 초기화)
 *      4. column에 사용된 policy 검사
 *      5. update meta
 *
 ***********************************************************************/
    
    SChar    sModuleName[QCS_MODULE_NAME_SIZE + 1]            = { 0, };
    SChar    sModuleVersion[QCS_MODULE_VERSION_SIZE + 1]      = { 0, };
    SChar    sMetaModuleName[QCS_MODULE_NAME_SIZE + 1]        = { 0, };
    SChar    sMetaModuleVersion[QCS_MODULE_VERSION_SIZE + 1]  = { 0, };
    SChar    sMetaECCPolicyName[QCS_POLICY_NAME_SIZE + 1]     = { 0, };
    UChar    sMetaECCPolicyCode[QCS_ECC_POLICY_CODE_SIZE + 1] = { 0, };
    UShort   sMetaECCPolicyCodeSize;
    idBool   sIsExist;
    idBool   sIsOpened = ID_FALSE;
    idBool   sIsInitialized = ID_FALSE;
    idBool   sIsValid;
    
    //----------------------------------------------
    // start 시작
    //----------------------------------------------
    
    ideLog::log( IDE_QP_0,"\n ==> Start Security Module" );

    //----------------------------------------------
    // property 확인
    //----------------------------------------------

    IDE_TEST_RAISE( ( idlOS::strlen( QCU_SECURITY_MODULE_NAME ) == 0 ) ||
                    ( idlOS::strlen( QCU_SECURITY_ECC_POLICY_NAME ) == 0 ),
                    ERR_FAIL_TO_OPEN_SECURITY_MODULE );
    
    //----------------------------------------------
    // meta 확인
    //----------------------------------------------

    IDE_TEST( getMetaSecurity( QC_SMI_STMT( aStatement ),
                               & sIsExist,
                               sMetaModuleName,
                               sMetaModuleVersion,
                               sMetaECCPolicyName,
                               sMetaECCPolicyCode,
                               & sMetaECCPolicyCodeSize )
              != IDE_SUCCESS );

    if ( sIsExist == ID_TRUE )
    {
        // server start시 보안모듈의 초기화가 실패하여 start하는 경우
        IDE_TEST_RAISE( qcsModule::isInitialized() == ID_TRUE,
                        ERR_SECURITY_MODULE_ALREADY_STARTED );

        //----------------------------------------------
        // meta 검사
        //----------------------------------------------
        
        ideLog::log( IDE_QP_0,"\n ==> Check Security Properties" );

        // module name 확인
        IDE_TEST_RAISE( idlOS::strMatch( sMetaModuleName,
                                         idlOS::strlen( sMetaModuleName ),
                                         QCU_SECURITY_MODULE_NAME,
                                         idlOS::strlen( QCU_SECURITY_MODULE_NAME ) ) != 0,
                        ERR_MISMATCH_SECURITY_MODULE_NAME );

        // ecc policy 확인
        IDE_TEST_RAISE( idlOS::strMatch( sMetaECCPolicyName,
                                         idlOS::strlen( sMetaECCPolicyName ),
                                         QCU_SECURITY_ECC_POLICY_NAME,
                                         idlOS::strlen( QCU_SECURITY_ECC_POLICY_NAME ) ) != 0,
                        ERR_MISMATCH_SECURITY_MODULE_ECC_POLICY );
        
        ideLog::log( IDE_QP_0,"\n ... Security Module Name : [%s], "
                     "ECC Policy Name : [%s]",
                     QCU_SECURITY_MODULE_NAME,
                     QCU_SECURITY_ECC_POLICY_NAME );
    }
    else
    {
        // 최초 보안모듈을 설정하고 start하는 경우
        
        // Nothing to do.
    }

    //----------------------------------------------
    // 모듈 열기
    //----------------------------------------------

    ideLog::log( IDE_QP_0,"\n ==> Open External Security Module" );

    IDE_TEST( qcsModule::openModule( QCU_SECURITY_MODULE_NAME,
                                     QCU_SECURITY_MODULE_LIBRARY )
              != IDE_SUCCESS );

    sIsOpened = ID_TRUE;

    if ( sIsExist == ID_TRUE )
    {
        //----------------------------------------------------
        // server start시 보안모듈의 초기화가 실패하여 start하는 경우
        //----------------------------------------------------

        //----------------------------------------------
        // 모듈 인증
        //----------------------------------------------

        ideLog::log( IDE_QP_0,"\n ==> Verify External Security Module" );
    
        IDE_TEST( qcsModule::verifyModule() != IDE_SUCCESS );

        //----------------------------------------------
        // 모듈 초기화
        //----------------------------------------------

        ideLog::log( IDE_QP_0,"\n ==> Initialize External Security Module" );
    
        IDE_TEST( qcsModule::initializeModule( QCU_SECURITY_ECC_POLICY_NAME )
                  != IDE_SUCCESS );
        sIsInitialized = ID_TRUE;

        ideLog::log( IDE_QP_0,"\n ... ECC Policy Name : [%s]",
                     QCU_SECURITY_ECC_POLICY_NAME );

        //----------------------------------------------
        // 모듈 version 검사
        //----------------------------------------------
        
        ideLog::log( IDE_QP_0,"\n ==> Verify Module Version" );
        
        IDE_TEST( qcsModule::getModuleVersion( sModuleVersion )
                  != IDE_SUCCESS );
        
        IDE_DASSERT( sModuleVersion[0] != '\0' );

        IDE_TEST_RAISE( idlOS::strMatch( sMetaModuleVersion,
                                         idlOS::strlen( sMetaModuleVersion ),
                                         sModuleVersion,
                                         idlOS::strlen( sModuleVersion ) ) != 0,
                        ERR_MISMATCH_SECURITY_MODULE_VERSION );

        ideLog::log( IDE_QP_0,"\n ... Security Module Version : [%s]",
                     sModuleVersion );
    
        //----------------------------------------------
        // ECC policy 검사
        //----------------------------------------------

        ideLog::log( IDE_QP_0,"\n ==> Verify ECC Policy" );
    
        IDE_TEST( qcsModule::verifyECCPolicyCode( sMetaECCPolicyCode,
                                                  sMetaECCPolicyCodeSize,
                                                  & sIsValid )
                  != IDE_SUCCESS );
        
        IDE_TEST_RAISE( sIsValid == ID_FALSE,
                        ERR_INVALID_ECC_POLICY );
    
        //----------------------------------------------
        // policy 검사
        //----------------------------------------------

        ideLog::log( IDE_QP_0,"\n ==> Verify Encrypted Column Policy" );
        
        IDE_TEST( verifyEncryptedColumnPolicy( QC_SMI_STMT( aStatement ) )
                  != IDE_SUCCESS );
    }
    else
    {
        //----------------------------------------------------
        // 최초 보안모듈을 설정하고 start하는 경우
        //----------------------------------------------------

        IDE_TEST( qcsModule::getModuleName( sModuleName )
                  != IDE_SUCCESS );

        ideLog::log( IDE_QP_0,"\n ... Security Module Name : [%s], "
                     "ECC Policy Name : [%s]",
                     sModuleName,
                     QCU_SECURITY_ECC_POLICY_NAME );
        
        //----------------------------------------------
        // 모듈 인증
        //----------------------------------------------

        ideLog::log( IDE_QP_0,"\n ==> Verify External Security Module" );
    
        IDE_TEST( qcsModule::verifyModule() != IDE_SUCCESS );

        //----------------------------------------------
        // 모듈 초기화
        //----------------------------------------------

        ideLog::log( IDE_QP_0,"\n ==> Initialize External Security Module" );
    
        IDE_TEST( qcsModule::initializeModule( QCU_SECURITY_ECC_POLICY_NAME )
                  != IDE_SUCCESS );
        sIsInitialized = ID_TRUE;

        ideLog::log( IDE_QP_0,"\n ... ECC Policy Name : [%s]",
                     QCU_SECURITY_ECC_POLICY_NAME );

        IDE_TEST( qcsModule::getModuleVersion( sModuleVersion )
                  != IDE_SUCCESS );
        
        IDE_DASSERT( sModuleVersion[0] != '\0' );

        ideLog::log( IDE_QP_0,"\n ... Security Module Version : [%s]",
                     sModuleVersion );
        
        //----------------------------------------------
        // meta insert
        //----------------------------------------------

        IDE_TEST( qcsModule::getECCPolicyCode( sMetaECCPolicyCode,
                                               & sMetaECCPolicyCodeSize )
                  != IDE_SUCCESS );
        sMetaECCPolicyCode[sMetaECCPolicyCodeSize] = '\0';
                  
        IDE_TEST( insertSecurityModuleIntoMeta(
                      QC_SMI_STMT( aStatement ),
                      sModuleName,
                      sModuleVersion,
                      QCU_SECURITY_ECC_POLICY_NAME,
                      (SChar*) sMetaECCPolicyCode )
                  != IDE_SUCCESS );
    }
    
    ideLog::log( IDE_QP_0," ... [SUCCESS]" );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_FAIL_TO_OPEN_SECURITY_MODULE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_FAIL_TO_OPEN_SECURITY_MODULE));
    }
    IDE_EXCEPTION(ERR_SECURITY_MODULE_ALREADY_STARTED);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_SECURITY_MODULE_ALREADY_STARTED));
    }
    IDE_EXCEPTION(ERR_MISMATCH_SECURITY_MODULE_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MISMATCH_SECURITY_MODULE,
                                sMetaModuleName,
                                sMetaModuleVersion,
                                sMetaECCPolicyName ));

        ideLog::log( IDE_QP_0,"\n Mismatch Security Module Name: [%s]",
                     sMetaModuleName );
    }
    IDE_EXCEPTION(ERR_MISMATCH_SECURITY_MODULE_VERSION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MISMATCH_SECURITY_MODULE,
                                sMetaModuleName,
                                sMetaModuleVersion,
                                sMetaECCPolicyName ));
        
        ideLog::log( IDE_QP_0,"\n Mismatch Security Module Version: [%s]",
                     sMetaModuleVersion );
    }
    IDE_EXCEPTION(ERR_MISMATCH_SECURITY_MODULE_ECC_POLICY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MISMATCH_SECURITY_MODULE,
                                sMetaModuleName,
                                sMetaModuleVersion,
                                sMetaECCPolicyName ));

        ideLog::log( IDE_QP_0,"\n Mismatch ECC Policy Name: [%s]",
                     sMetaECCPolicyName );
    }
    IDE_EXCEPTION(ERR_INVALID_ECC_POLICY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_ECC_POLICY,
                                QCU_SECURITY_ECC_POLICY_NAME));
    }
    IDE_EXCEPTION_END;

    if ( sIsInitialized == ID_TRUE )
    {
        (void) qcsModule::finalizeModule();
    }
    
    if ( sIsOpened == ID_TRUE )
    {
        (void) qcsModule::closeModule();
    }

    return IDE_FAILURE;
}

IDE_RC qcs::stopSecurity( qcStatement  * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *    1. meta 확인
 *    2. encrypt column이 더이상 사용되지 않는지 검사
 *    3. finalizeModule (모듈 종료)
 *    4. closeModule (모듈 닫기)
 *    5. meta 삭제
 *
 ***********************************************************************/

    SChar    sMetaModuleVersion[QCS_MODULE_VERSION_SIZE + 1];
    SChar    sMetaModuleName[QCS_MODULE_NAME_SIZE + 1];
    SChar    sMetaECCPolicyName[QCS_POLICY_NAME_SIZE + 1];
    UChar    sMetaECCPolicyCode[QCS_ECC_POLICY_CODE_SIZE + 1];
    UShort   sMetaECCPolicyCodeSize;
    idBool   sIsExist;
    UInt     sCount;
    
    //----------------------------------------------
    // meta 확인
    //----------------------------------------------
    
    IDE_TEST( getMetaSecurity( QC_SMI_STMT( aStatement ),
                               & sIsExist,
                               sMetaModuleName,
                               sMetaModuleVersion,
                               sMetaECCPolicyName,
                               sMetaECCPolicyCode,
                               & sMetaECCPolicyCodeSize )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sIsExist == ID_FALSE, NORMAL_EXIT );
    
    //----------------------------------------------
    // encrypted column 확인
    //----------------------------------------------

    IDE_TEST( getEncryptedColumnCount(
                  QC_SMI_STMT( aStatement ),
                  & sCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sCount != 0,
                    ERR_EXIST_ENCRYPTED_COLUMN );
    
    //----------------------------------------------
    // 모듈 종료
    //----------------------------------------------
    
    ideLog::log( IDE_QP_0,"\n ==> Stop Security Module" );
        
    if ( qcsModule::isInitialized() == ID_TRUE )
    {
        IDE_TEST( qcsModule::finalizeModule() != IDE_SUCCESS );

        //----------------------------------------------
        // 모듈 닫기
        //----------------------------------------------
        
        (void) qcsModule::closeModule();
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------------------
    // meta 삭제
    //----------------------------------------------
    
    IDE_TEST( deleteSecurityModuleFromMeta(
                  QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    ideLog::log( IDE_QP_0," ... [SUCCESS]" );
    
    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_ENCRYPTED_COLUMN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_EXIST_ENCRYPTED_COLUMN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcs::getMetaSecurity( smiStatement  * aSmiStmt,
                             idBool        * aIsExist,
                             SChar         * aModuleName,
                             SChar         * aModuleVersion,
                             SChar         * aECCPolicyName,
                             UChar         * aECCPolicyCode,
                             UShort        * aECCPolicyCodeSize )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt              sStage  = 0;
    const void      * sRow;
    mtcColumn       * sModuleNameColInfo;
    mtcColumn       * sModuleVersionColInfo;
    mtcColumn       * sECCPolicyNameColInfo;
    mtcColumn       * sECCPolicyCodeColInfo;
    mtdCharType     * sCharStr;
    smiTableCursor    sCursor;
    scGRID            sRid; // Disk Table을 위한 Record IDentifier

    sCursor.initialize();

    // sModuleName
    IDE_TEST( smiGetTableColumns( gQcmSecurity,
                                  QCM_SECURITY_MODULE_NAME_COL_ORDER,
                                  (const smiColumn**)&sModuleNameColInfo )
              != IDE_SUCCESS );
    // sModuleVersion
    IDE_TEST( smiGetTableColumns( gQcmSecurity,
                                  QCM_SECURITY_MODULE_VERSION_COL_ORDER,
                                  (const smiColumn**)&sModuleVersionColInfo )
              != IDE_SUCCESS );
    
    // sECCPolicyName
    IDE_TEST( smiGetTableColumns( gQcmSecurity,
                                  QCM_SECURITY_MODULE_ECC_POLICY_NAME_COL_ORDER,
                                  (const smiColumn**)&sECCPolicyNameColInfo )
              != IDE_SUCCESS );
    
    // sECCPolicyCode
    IDE_TEST( smiGetTableColumns( gQcmSecurity,
                                  QCM_SECURITY_MODULE_ECC_POLICY_CODE_COL_ORDER,
                                  (const smiColumn**)&sECCPolicyCodeColInfo )
              != IDE_SUCCESS );
    
    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmSecurity,
                 NULL,
                 smiGetRowSCN(gQcmSecurity),
                 NULL,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & gMetaDefaultCursorProperty) != IDE_SUCCESS);
    sStage = 1;
    
    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    if ( sRow != NULL )
    {
        *aIsExist = ID_TRUE;
        
        // module name
        sCharStr = (mtdCharType*)
            ((UChar*)sRow + sModuleNameColInfo->column.offset);
        idlOS::memcpy( aModuleName,
                       (SChar*) &(sCharStr->value),
                       sCharStr->length );
        aModuleName[sCharStr->length] = '\0';

        // module name
        sCharStr = (mtdCharType*)
            ((UChar*)sRow + sModuleVersionColInfo->column.offset);
        idlOS::memcpy( aModuleVersion,
                       (SChar*) &(sCharStr->value),
                       sCharStr->length );
        aModuleVersion[sCharStr->length] = '\0';

        // ecc policy name
        sCharStr = (mtdCharType*)
            ((UChar*)sRow + sECCPolicyNameColInfo->column.offset);
        idlOS::memcpy( aECCPolicyName,
                       (SChar*) &(sCharStr->value),
                       sCharStr->length );
        aECCPolicyName[sCharStr->length] = '\0';
        
        // ecc policy code
        sCharStr = (mtdCharType*)
            ((UChar*)sRow + sECCPolicyCodeColInfo->column.offset);
        idlOS::memcpy( aECCPolicyCode,
                       (SChar*) &(sCharStr->value),
                       sCharStr->length );
        *aECCPolicyCodeSize = sCharStr->length;
    }
    else
    {
        *aIsExist = ID_FALSE;
    }
    
    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
    }
    
    return IDE_FAILURE;
}

IDE_RC qcs::verifyEncryptedColumnPolicy( smiStatement  * aSmiStmt )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *    1. SYS_ENCRYPTED_COLUMNS_에서 record를 fetch
 *    2. policy_code, ecc_policy_code를 검사
 *
 ***********************************************************************/
    
    UInt              sStage  = 0;
    const void      * sRow;
    mtcColumn       * sPolicyNameColInfo;
    mtcColumn       * sPolicyCodeColInfo;
    mtdCharType     * sCharStr;
    SChar             sPolicyName[QCS_POLICY_NAME_SIZE + 1];
    UChar             sPolicyCode[QCS_POLICY_CODE_SIZE + 1];
    UShort            sPolicyCodeSize;
    idBool            sIsValid;
    smiTableCursor    sCursor;
    scGRID            sRid; // Disk Table을 위한 Record IDentifier

    sCursor.initialize();

    // sPolicyName
    IDE_TEST( smiGetTableColumns( gQcmEncryptedColumns,
                                  QCM_ENCRYPTED_COLUMNS_POLICY_NAME_COL_ORDER,
                                  (const smiColumn**)&sPolicyNameColInfo )
              != IDE_SUCCESS );

    // sPolicyCode
    IDE_TEST( smiGetTableColumns( gQcmEncryptedColumns,
                                  QCM_ENCRYPTED_COLUMNS_POLICY_CODE_COL_ORDER,
                                  (const smiColumn**)&sPolicyCodeColInfo )
              != IDE_SUCCESS );
    
    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmEncryptedColumns,
                          NULL,
                          smiGetRowSCN(gQcmEncryptedColumns),
                          NULL,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          & gMetaDefaultCursorProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        // policy name
        sCharStr = (mtdCharType*)
            ((UChar*)sRow + sPolicyNameColInfo->column.offset);
        idlOS::memcpy( sPolicyName,
                       (SChar*) &(sCharStr->value),
                       sCharStr->length );
        sPolicyName[sCharStr->length] = '\0';

        // policy code
        sCharStr = (mtdCharType*)
            ((UChar*)sRow + sPolicyCodeColInfo->column.offset);
        idlOS::memcpy( sPolicyCode,
                       (SChar*) &(sCharStr->value),
                       sCharStr->length );
        sPolicyCodeSize = sCharStr->length;

        // log
        ideLog::log( IDE_QP_0,"\n     policy [%s]", sPolicyName );
        
        // validate policy
        IDE_TEST( qcsModule::verifyPolicyCode( sPolicyName,
                                               sPolicyCode,
                                               sPolicyCodeSize,
                                               & sIsValid )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsValid == ID_FALSE,
                        ERR_INVALID_POLICY );

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT)
                 != IDE_SUCCESS);
    }
    
    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_POLICY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_POLICY,
                                sPolicyName));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
    }
    
    return IDE_FAILURE;
}

IDE_RC qcs::getEncryptedColumnCount( smiStatement  * aSmiStmt,
                                     UInt          * aCount )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *    1. SYS_ENCRYPTED_COLUMNS_에서 record를 fetch
 *    2. policy_code, ecc_policy_code를 검사
 *
 ***********************************************************************/
    
    UInt              sStage  = 0;
    const void      * sRow;
    smiTableCursor    sCursor;
    scGRID            sRid; // Disk Table을 위한 Record IDentifier
    UInt              sCount = 0;

    sCursor.initialize();

    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmEncryptedColumns,
                          NULL,
                          smiGetRowSCN(gQcmEncryptedColumns),
                          NULL,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &gMetaDefaultCursorProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        sCount++;
        
        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT)
                 != IDE_SUCCESS);
    }
    
    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    *aCount = sCount;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
    }
    
    return IDE_FAILURE;
}
    
IDE_RC qcs::insertSecurityModuleIntoMeta( smiStatement  * aSmiStmt,
                                          SChar         * aModuleName,
                                          SChar         * aModuleVersion,
                                          SChar         * aECCPolicyName,
                                          SChar         * aECCPolicyCode )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *
 ***********************************************************************/
    
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_SECURITY_ VALUES ( "
                     "VARCHAR'%s', VARCHAR'%s', VARCHAR'%s', VARCHAR'%s' )",
                     aModuleName,
                     aModuleVersion,
                     aECCPolicyName,
                     aECCPolicyCode );

    IDE_TEST( qcg::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
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

IDE_RC qcs::deleteSecurityModuleFromMeta( smiStatement  * aSmiStmt )
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SECURITY_" );

    IDE_TEST( qcg::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
