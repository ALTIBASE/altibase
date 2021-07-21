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
 * $Id: qsxPkg.cpp 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qsxPkg.h>
#include <qcm.h>
#include <qcmPkg.h>
#include <qcg.h>

IDE_RC qsxPkg::createPkgObjectInfo(
    qsOID                aPkgOID,
    qsxPkgObjectInfo ** aPkgObjectInfo)
{
    qsxPkgObjectInfo  * sPkgObjectInfo = NULL;
    SChar               sLatchName[IDU_MUTEX_NAME_LEN];
    UInt                sStage = 0;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_QSX,
                                       ID_SIZEOF(qsxPkgObjectInfo),
                                       (void**) & sPkgObjectInfo )
                    != IDE_SUCCESS, err_nomem_qsx_Pkg_object_info );
    sStage = 1;

    // BUG-29598
    idlOS::snprintf( sLatchName,
                     IDU_MUTEX_NAME_LEN,
                     "PKG_%"ID_vULONG_FMT"_OBJECT_LATCH",
                     aPkgOID );
    
    IDE_TEST( sPkgObjectInfo->latch.initialize( sLatchName )
              != IDE_SUCCESS );
    sStage = 2;

    // BUG-36291
    idlOS::snprintf( sLatchName,
                     IDU_MUTEX_NAME_LEN,
                     "PKG_%"ID_vULONG_FMT"_STATUS_LATCH",
                     aPkgOID );
    
    IDE_TEST( sPkgObjectInfo->latchForStatus.initialize( sLatchName )
              != IDE_SUCCESS );
    sStage = 3;

    sPkgObjectInfo->isAvailable = ID_TRUE;
    sPkgObjectInfo->pkgInfo     = NULL;

    *aPkgObjectInfo = sPkgObjectInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_nomem_qsx_Pkg_object_info );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_NOT_ENOUGH_MEMORY_ARG2,
                                 "Unable to allocate a new qsxPkgObjectInfo",
                                 ID_SIZEOF(qsxPkgObjectInfo) ));
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            (void) sPkgObjectInfo->latchForStatus.destroy();
            /* fall through */
        case 2:
            (void) sPkgObjectInfo->latch.destroy();
            /* fall through */
        case 1:
            (void) iduMemMgr::free( sPkgObjectInfo );
            sPkgObjectInfo = NULL;
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}


IDE_RC qsxPkg::destroyPkgObjectInfo(
    qsOID             /* aPkgOID */,
    qsxPkgObjectInfo ** aPkgObjectInfo)
{
    IDE_DASSERT( *aPkgObjectInfo != NULL );

    // iduLatch::destroy는 반드시 성공함. 
    (void)(*aPkgObjectInfo)->latchForStatus.destroy();

    (void)(*aPkgObjectInfo)->latch.destroy();

    IDE_TEST( iduMemMgr::free( *aPkgObjectInfo )
              != IDE_SUCCESS);

    *aPkgObjectInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxPkg::disablePkgObjectInfo(
    qsOID          aPkgOID )
{
    qsxPkgObjectInfo  * sPkgObjectInfo;

    IDE_TEST( smiObject::getObjectTempInfo( smiGetTable( aPkgOID ),
                                            (void**)&sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

    sPkgObjectInfo->isAvailable = ID_FALSE;
    sPkgObjectInfo->pkgInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxPkg::getPkgObjectInfo( qsOID               aPkgOID,
                                 qsxPkgObjectInfo ** aPkgObjectInfo )
{
    IDE_TEST( smiObject::getObjectTempInfo( smiGetTable( aPkgOID ),
                                            (void**)aPkgObjectInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxPkg::setPkgObjectInfo( qsOID              aPkgOID,
                                 qsxPkgObjectInfo * aPkgObjectInfo )
{
    smiObject::setObjectTempInfo( smiGetTable( aPkgOID ),
                                  aPkgObjectInfo );

    return IDE_SUCCESS;
}


IDE_RC qsxPkg::createPkgInfo(
    qsOID            aPkgOID,
    qsxPkgInfo    ** aPkgInfo )
{
    qsxPkgInfo * sPkgInfo = NULL;
    UInt         sStage = 0;

    IDE_TEST_RAISE( iduMemMgr::calloc(
                        IDU_MEM_QSX,
                        1,
                        idlOS::align8((UInt)ID_SIZEOF(qsxPkgInfo)),
                        (void**)&sPkgInfo)
                    != IDE_SUCCESS, err_nomem_qsx_Pkg_info);
    sStage = 1;

    sPkgInfo->pkgOID         = aPkgOID;
    sPkgInfo->qmsMem         = NULL;
    sPkgInfo->isValid        = ID_FALSE;
    sPkgInfo->modifyCount    = 0;
    sPkgInfo->privilegeCount = 0;
    sPkgInfo->granteeID      = NULL;
    sPkgInfo->planTree       = NULL;
    
    *aPkgInfo = sPkgInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_nomem_qsx_Pkg_info);
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_NOT_ENOUGH_MEMORY_ARG2,
                                 "Unable to allocate a new qsxPkgInfo",
                                 ID_SIZEOF(qsxPkgInfo) ));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            (void)iduMemMgr::free(sPkgInfo);
            sPkgInfo = NULL;
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::destroyPkgInfo(
    qsxPkgInfo ** aPkgInfo)
{
    IDE_DASSERT( *aPkgInfo != NULL );

    if ( (*aPkgInfo)->granteeID != NULL )
    {
        IDE_TEST(iduMemMgr::free((*aPkgInfo)->granteeID)
                 != IDE_SUCCESS);
        (*aPkgInfo)->granteeID = NULL;
    }
    
    // BUG-37296
    if( (*aPkgInfo)->qmsMem != NULL )
    {
        IDE_TEST( (*aPkgInfo)->qmsMem->destroy() != IDE_SUCCESS );

        IDE_TEST( qcg::freeIduVarMemList( (*aPkgInfo)->qmsMem ) != IDE_SUCCESS );
        (*aPkgInfo)->qmsMem = NULL;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( iduMemMgr::free( *aPkgInfo )
              != IDE_SUCCESS );

    *aPkgInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxPkg::getPkgInfo( qsOID          aPkgOID,
                           qsxPkgInfo  ** aPkgInfo )
{
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxPkgObjectInfo  * sPkgObjectInfo;

    IDE_TEST( getPkgObjectInfo( aPkgOID,
                                & sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

    *aPkgInfo = sPkgObjectInfo->pkgInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxPkg::setPkgInfo( qsOID          aPkgOID,
                           qsxPkgInfo   * aPkgInfo )
{
    qsxPkgObjectInfo  * sPkgObjectInfo;

    IDE_TEST( getPkgObjectInfo( aPkgOID,
                                & sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

    sPkgObjectInfo->pkgInfo = aPkgInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxPkg::setPkgText(
    smiStatement * aSmiStmt,
    qsOID          aPkgOID,
    SChar        * aPkgText,
    SInt           aPkgTextLen)
{
    IDE_TEST( smiObject::setObjectInfo( aSmiStmt,
                                        smiGetTable( aPkgOID ),
                                        (void*) aPkgText,
                                        aPkgTextLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxPkg::getPkgText(
    qcStatement * aStatement,
    qsOID         aPkgOID,
    SChar      ** aPkgText,
    SInt        * aPkgTextLen)
{
    UInt         sPkgTextLen;
    const void * sTableHandle;

    sTableHandle = smiGetTable( aPkgOID );

    smiObject::getObjectInfoSize( smiGetTable( aPkgOID ), &sPkgTextLen );

    if ( *aPkgText == NULL )
    {
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc((sPkgTextLen + 1),
                                               (void**)aPkgText)
                 != IDE_SUCCESS);
    }

    smiObject::getObjectInfo( sTableHandle, (void**) aPkgText );
    IDE_TEST_RAISE( *aPkgText == NULL, ERR_Pkg_NOT_FOUND );

    (*aPkgTextLen) = sPkgTextLen;

    // make null terminating string : PROJ-1500
    *((*aPkgText) + sPkgTextLen) = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_Pkg_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsxPkg::getPkgText",
                                  "Package not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxPkg::latchS( qsOID          aPkgOID )
{
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxPkgObjectInfo  * sPkgObjectInfo;
    UInt                sStage = 0;

    IDE_TEST( getPkgObjectInfo( aPkgOID,
                                & sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

    IDE_TEST( sPkgObjectInfo->latch.lockRead(
                                NULL, /* idvSQL* */
                                NULL /* idvWeArgs* */ ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST_RAISE( sPkgObjectInfo->isAvailable != ID_TRUE,
                    err_latch_info_is_null );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_latch_info_is_null );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_NO_SUCH_LATCH_PKG_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void) sPkgObjectInfo->latch.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::latchX( qsOID          aPkgOID,
                       idBool         aTryLock  )
{
    static PDL_Time_Value   sSleep;
    qsxPkgObjectInfo      * sPkgObjectInfo;
    idBool                  sSuccess = ID_FALSE;
    SInt                    sRetryCount;
    UInt                    sStage = 0;

    sSleep.initialize( 0, RETRY_SLEEP_USEC );

    IDE_TEST( getPkgObjectInfo( aPkgOID,
                                & sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

    if( aTryLock == ID_TRUE )
    {
        for ( sRetryCount = 0;
              sRetryCount < MAX_RETRY_COUNT;
              sRetryCount ++ )
        {
            IDE_TEST( sPkgObjectInfo->latch.tryLockWrite( &sSuccess )
                      != IDE_SUCCESS );

            if ( sSuccess == ID_TRUE )
            {
                sStage = 1;

                IDE_TEST_RAISE( sPkgObjectInfo->isAvailable != ID_TRUE,
                                err_latch_info_is_null );
                break;
            }
            else
            {
                idlOS::sleep( sSleep );
            }
        }

        IDE_TEST_RAISE( sSuccess == ID_FALSE, err_resource_busy );
    }
    else
    {
        IDE_TEST( sPkgObjectInfo->latch.lockWrite(
                                    NULL, /* idvSQL* */
                                    NULL /* idvWeArgs* */ )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_latch_info_is_null );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_NO_SUCH_LATCH_PKG_OBJECT ) );
    }
    IDE_EXCEPTION( err_resource_busy );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_RESOURCE_BUSY, "try later" ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void) sPkgObjectInfo->latch.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::latchXForRecompile( qsOID          aPkgOID )
{
/***********************************************************************
 *
 * Description :
 *   BUG-18854
 *   프로시저 실행 중 발생한 recompile에 한해서
 *   X latch 를 잡을 때 실패하면 rebuild에러를 올려서
 *   abort resource busy에러와 구분한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    static PDL_Time_Value   sSleep;
    qsxPkgObjectInfo      * sPkgObjectInfo;
    idBool                  sSuccess = ID_FALSE;
    SInt                    sRetryCount;
    UInt                    sStage = 0;

    sSleep.initialize( 0, RETRY_SLEEP_USEC );

    IDE_TEST( getPkgObjectInfo( aPkgOID,
                                & sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

    for ( sRetryCount = 0;
          sRetryCount < MAX_RETRY_COUNT;
          sRetryCount ++ )
    {
        IDE_TEST( sPkgObjectInfo->latch.tryLockWrite( &sSuccess )
                  != IDE_SUCCESS );

        if ( sSuccess == ID_TRUE )
        {
            sStage = 1;

            IDE_TEST_RAISE( sPkgObjectInfo->isAvailable != ID_TRUE,
                            err_latch_info_is_null );
            break;
        }
        else
        {
            idlOS::sleep( sSleep );
        }
    }

    IDE_TEST_RAISE( sSuccess == ID_FALSE, err_resource_busy );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_latch_info_is_null );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_NO_SUCH_LATCH_PKG_OBJECT ) );
    }
    IDE_EXCEPTION( err_resource_busy );
    {
        IDE_SET( ideSetErrorCode( qpERR_REBUILD_QCU_RESOURCE_BUSY ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void) sPkgObjectInfo->latch.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::unlatch( qsOID aPkgOID )
{
    qsxPkgObjectInfo  * sPkgObjectInfo;

    IDE_TEST( qsxPkg::getPkgObjectInfo( aPkgOID,
                                        &sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

    IDE_TEST( sPkgObjectInfo->latch.unlock()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxPkg::makeStatusValid( qcStatement * aStatement,
                                qsOID         aPkgOID )
{
    qsxPkgObjectInfo * sObjInfo;
    UInt               sState = 0;

    (void) qsxPkg::getPkgObjectInfo( aPkgOID , &sObjInfo );

    IDE_TEST( qsxPkg::latchXForStatus( aPkgOID )
              != IDE_SUCCESS);
    sState = 1;

    // latch를 잡았다면 pkgInfo가 반드시 있어야 한다.
    IDE_ERROR( sObjInfo->pkgInfo != NULL );

    if( sObjInfo->pkgInfo->sessionID == QCG_GET_SESSION_ID( aStatement ) )
    {
        sObjInfo->pkgInfo->isValid = ID_TRUE;

        IDE_TEST( qcmPkg::pkgUpdateStatus( aStatement,
                                           aPkgOID,
                                           QCM_PKG_VALID )
                  != IDE_SUCCESS );
    }
    else
    {
        // 다른 session 에서 PSM을 invalid 시킨 경우
        // valid 상태로 변경하지 않는다.
    }

    sState = 0;
    IDE_TEST( qsxPkg::unlatchForStatus( aPkgOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxPkg::unlatchForStatus( aPkgOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::makeStatusValidTx( qcStatement * aStatement,
                                  qsOID         aPkgOID )
{
    qsxPkgObjectInfo * sObjInfo;
    UInt               sState = 0;

    (void) qsxPkg::getPkgObjectInfo( aPkgOID , &sObjInfo );

    IDE_TEST( qsxPkg::latchXForStatus( aPkgOID )
              != IDE_SUCCESS);
    sState = 1;

    // latch를 잡았다면 pkgInfo가 반드시 있어야 한다.
    IDE_ERROR( sObjInfo->pkgInfo != NULL );

    if( sObjInfo->pkgInfo->sessionID == QCG_GET_SESSION_ID( aStatement ) )
    {
        sObjInfo->pkgInfo->isValid = ID_TRUE;

        IDE_TEST( qcmPkg::pkgUpdateStatusTx( aStatement,
                                             aPkgOID,
                                             QCM_PKG_VALID )
                  != IDE_SUCCESS );
    }
    else
    {
        // 다른 session 에서 PSM을 invalid 시킨 경우
        // valid 상태로 변경하지 않는다.
    }

    sState = 0;
    IDE_TEST( qsxPkg::unlatchForStatus( aPkgOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxPkg::unlatchForStatus( aPkgOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::makeStatusInvalid( qcStatement * aStatement,
                                  qsOID         aPkgOID )
{
    qsxPkgObjectInfo * sObjInfo;
    UInt               sState = 0;
 
    (void) qsxPkg::getPkgObjectInfo( aPkgOID , &sObjInfo );

    IDE_TEST( qsxPkg::latchXForStatus( aPkgOID )
              != IDE_SUCCESS);
    sState = 1;

    sObjInfo->pkgInfo->isValid = ID_FALSE;
    sObjInfo->pkgInfo->sessionID = QCG_GET_SESSION_ID( aStatement );

    IDE_TEST( qcmPkg::pkgUpdateStatus( aStatement,
                                       aPkgOID,
                                       QCM_PKG_INVALID )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( qsxPkg::unlatchForStatus( aPkgOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxPkg::unlatchForStatus( aPkgOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::makeStatusInvalidTx( qcStatement * aStatement,
                                    qsOID         aPkgOID )
{
    qsxPkgObjectInfo * sObjInfo;
    UInt               sState = 0;
 
    (void) qsxPkg::getPkgObjectInfo( aPkgOID , &sObjInfo );

    IDE_TEST( qsxPkg::latchXForStatus( aPkgOID )
              != IDE_SUCCESS);
    sState = 1;

    sObjInfo->pkgInfo->isValid = ID_FALSE;
    sObjInfo->pkgInfo->sessionID = QCG_GET_SESSION_ID( aStatement );

    IDE_TEST( qcmPkg::pkgUpdateStatusTx( aStatement,
                                         aPkgOID,
                                         QCM_PKG_INVALID )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( qsxPkg::unlatchForStatus( aPkgOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxPkg::unlatchForStatus( aPkgOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::makeStatusInvalidForRecompile( qcStatement * aStatement,
                                              qsOID         aPkgOID )
{
    qsxPkgObjectInfo * sObjInfo;
    UInt               sState = 0;
 
    (void) qsxPkg::getPkgObjectInfo( aPkgOID , &sObjInfo );

    IDE_TEST( qsxPkg::latchXForStatus( aPkgOID )
              != IDE_SUCCESS);
    sState = 1;

    sObjInfo->pkgInfo->isValid = ID_FALSE;
    sObjInfo->pkgInfo->sessionID = QCG_GET_SESSION_ID( aStatement );

    sState = 0;
    IDE_TEST( qsxPkg::unlatchForStatus( aPkgOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxPkg::unlatchForStatus( aPkgOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::latchXForStatus( qsOID          aPkgOID )
{
    static PDL_Time_Value   sSleep;
    qsxPkgObjectInfo      * sPkgObjectInfo;
    idBool                  sSuccess = ID_FALSE;
    SInt                    sRetryCount;
    UInt                    sStage = 0;

    sSleep.initialize( 0, RETRY_SLEEP_USEC );

    IDE_TEST( getPkgObjectInfo( aPkgOID,
                                & sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

retry:

    for ( sRetryCount = 0;
          sRetryCount < MAX_RETRY_COUNT;
          sRetryCount ++ )
    {
        IDE_TEST( sPkgObjectInfo->latchForStatus.tryLockWrite( &sSuccess )
                  != IDE_SUCCESS );

        if ( sSuccess == ID_TRUE )
        {
            sStage = 1;

            IDE_TEST_RAISE( sPkgObjectInfo->isAvailable != ID_TRUE,
                            err_latch_info_is_null );
            break;
        }
        else
        {
            idlOS::sleep( sSleep );
        }
    }

    if( sSuccess == ID_FALSE )
    {
        goto retry;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_latch_info_is_null );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_NO_SUCH_LATCH_PKG_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void) sPkgObjectInfo->latchForStatus.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxPkg::unlatchForStatus( qsOID          aPkgOID )
{
    qsxPkgObjectInfo  * sPkgObjectInfo;

    IDE_TEST( qsxPkg::getPkgObjectInfo( aPkgOID,
                                        &sPkgObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sPkgObjectInfo != NULL );

    IDE_TEST( sPkgObjectInfo->latchForStatus.unlock()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxPkg::findSubprogramPlanTree(
    qsxPkgInfo       * aPkgInfo,
    UInt               aSubprogramID,
    qsProcParseTree ** aSubprogramPlanTree )
{
    qsPkgParseTree   * sPkgPlanTree        = NULL;
    qsProcParseTree  * sProcPlanTree       = NULL;
    qsProcParseTree  * sSubprogramPlanTree = NULL;
    qsPkgStmts       * sPkgStmt;
    qsPkgSubprograms * sSubprogram;

    sPkgPlanTree = aPkgInfo->planTree;

    for ( sPkgStmt = sPkgPlanTree->block->subprograms;
          sPkgStmt != NULL;
          sPkgStmt = sPkgStmt->next )
    {
        if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
        {
            sSubprogram = (qsPkgSubprograms*)sPkgStmt;
            sSubprogramPlanTree = sSubprogram->parseTree;

            if ( sSubprogram->subprogramID == aSubprogramID )
            {
                if ( (sSubprogramPlanTree->block == NULL) &&
                     ( (sSubprogramPlanTree->procType != QS_EXTERNAL_C) &&
                       (sSubprogramPlanTree->procType != QS_INTERNAL_C) ) )
                {
                    // subprogram declaration
                    // Nothing to do.
                }
                else
                {
                    sProcPlanTree = sSubprogramPlanTree;
                    break;
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_RAISE( sProcPlanTree == NULL,
                    err_plan_tree_not_found);

    *aSubprogramPlanTree = sProcPlanTree;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_plan_tree_not_found);
    {
        IDE_SET(ideSetErrorCode(
                qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                "[qsxRelatedProc::findPlanTree] planTree is not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2446 ONE SOURCE FOR SUPPOTING PACKAGE */
IDE_RC qsxPkg::createPkgObjAndInfoCallback( smiStatement * aSmiStmt,
                                            qsOID          aPkgOID )
{
    IDE_ASSERT( aSmiStmt != NULL );
    IDE_ASSERT( aPkgOID != SM_NULL_OID );

    return qsx::qsxLoadPkgByPkgOID( aSmiStmt,
                                    aPkgOID,
                                    ID_TRUE );
}
