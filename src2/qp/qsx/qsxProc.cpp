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
 * $Id: qsxProc.cpp 90664 2021-04-21 06:14:15Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qsxProc.h>
#include <qcm.h>
#include <qcmProc.h>
#include <qcg.h>

IDE_RC qsxProc::createProcObjectInfo(
    qsOID                aProcOID,
    qsxProcObjectInfo ** aProcObjectInfo)
{
#define IDE_FN "qsxProc::createProcObjectInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcObjectInfo  * sProcObjectInfo = NULL;
    SChar                sLatchName[IDU_MUTEX_NAME_LEN];
    UInt                 sStage = 0;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_QSX,
                                       ID_SIZEOF(qsxProcObjectInfo),
                                       (void**) & sProcObjectInfo )
                    != IDE_SUCCESS, err_nomem_qsx_proc_object_info );
    sStage = 1;

    // BUG-29598
    idlOS::snprintf( sLatchName,
                     IDU_MUTEX_NAME_LEN,
                     "PSM_%"ID_vULONG_FMT"_OBJECT_LATCH",
                     aProcOID );

    IDE_TEST( sProcObjectInfo->latch.initialize( sLatchName )
              != IDE_SUCCESS );
    sStage = 2;

    // BUG-36291
    idlOS::snprintf( sLatchName,
                     IDU_MUTEX_NAME_LEN,
                     "PSM_%"ID_vULONG_FMT"_STATUS_LATCH",
                     aProcOID );

    IDE_TEST( sProcObjectInfo->latchForStatus.initialize( sLatchName )
              != IDE_SUCCESS );
    sStage = 3;

    sProcObjectInfo->isAvailable = ID_TRUE;
    sProcObjectInfo->procInfo = NULL;

    *aProcObjectInfo = sProcObjectInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_nomem_qsx_proc_object_info );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_NOT_ENOUGH_MEMORY_ARG2,
                                 "Unable to allocate a new qsxProcObjectInfo",
                                 ID_SIZEOF(qsxProcObjectInfo) ));
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            (void) sProcObjectInfo->latchForStatus.destroy();
            /* fall through */
        case 2:
            (void) sProcObjectInfo->latch.destroy();
            /* fall through */
        case 1:
            (void) iduMemMgr::free( sProcObjectInfo );
            sProcObjectInfo = NULL;
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxProc::destroyProcObjectInfo(
    qsxProcObjectInfo ** aProcObjectInfo)
{
#define IDE_FN "qsxProc::destroyProcObjectInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( *aProcObjectInfo != NULL );

    // iduLatch::destroy는 반드시 성공함.
    (void)(*aProcObjectInfo)->latchForStatus.destroy();

    (void)(*aProcObjectInfo)->latch.destroy();

    IDE_TEST( iduMemMgr::free( *aProcObjectInfo )
              != IDE_SUCCESS);

    *aProcObjectInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxProc::disableProcObjectInfo(
    qsOID          aProcOID )
{
#define IDE_FN "qsxProc::disableProcObjectInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcObjectInfo  * sProcObjectInfo;

    IDE_TEST( smiObject::getObjectTempInfo( smiGetTable( aProcOID ),
                                            (void**)&sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

    sProcObjectInfo->isAvailable = ID_FALSE;
    sProcObjectInfo->procInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxProc::getProcObjectInfo( qsOID                aProcOID,
                                   qsxProcObjectInfo ** aProcObjectInfo )
{
#define IDE_FN "qsxProc::getProcObjectInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( smiObject::getObjectTempInfo( smiGetTable( aProcOID ),
                                            (void**)aProcObjectInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#undef IDE_FN
}


IDE_RC qsxProc::setProcObjectInfo( qsOID               aProcOID,
                                   qsxProcObjectInfo * aProcObjectInfo )
{
#define IDE_FN "qsxProc::setProcObjectInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    smiObject::setObjectTempInfo( smiGetTable( aProcOID ),
                                  aProcObjectInfo );

    return IDE_SUCCESS;

#undef IDE_FN
}


IDE_RC qsxProc::createProcInfo(
    qsOID            aProcOID,
    qsxProcInfo   ** aProcInfo )
{
#define IDE_FN "qsxProc::createProcInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcInfo * sProcInfo = NULL;
    UInt          sStage = 0;

    IDU_FIT_POINT_RAISE( "qsxProc::createProcInfo::calloc::sProcInfo",
                          err_nomem_qsx_proc_info );

    IDE_TEST_RAISE( iduMemMgr::calloc(
                        IDU_MEM_QSX,
                        1,
                        idlOS::align8((UInt)ID_SIZEOF(qsxProcInfo)),
                        (void**)&sProcInfo)
                    != IDE_SUCCESS, err_nomem_qsx_proc_info);
    sStage = 1;

    sProcInfo->procOID        = aProcOID;

    sProcInfo->isValid        = ID_FALSE;

    // TASK-7244 Set shard split method to PSM info
    sProcInfo->shardSplitMethod = SDI_SPLIT_NONE;

    sProcInfo->qmsMem         = NULL;
    sProcInfo->planTree       = NULL;
    sProcInfo->relatedObjects = NULL;
    sProcInfo->privilegeCount = 0;
    sProcInfo->modifyCount    = 0;
    sProcInfo->granteeID      = NULL;

    sProcInfo->cache          = NULL;
    
    *aProcInfo = sProcInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_nomem_qsx_proc_info);
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_NOT_ENOUGH_MEMORY_ARG2,
                                 "Unable to allocate a new qsxProcInfo",
                                 ID_SIZEOF(qsxProcInfo) ));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            (void)iduMemMgr::free(sProcInfo);
            sProcInfo = NULL;
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxProc::destroyProcInfo(
    qsxProcInfo ** aProcInfo)
{
#define IDE_FN "qsxProc::destroyProcInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( *aProcInfo != NULL );

    if ( (*aProcInfo)->granteeID != NULL )
    {
        IDE_TEST(iduMemMgr::free((*aProcInfo)->granteeID)
                 != IDE_SUCCESS);
        (*aProcInfo)->granteeID = NULL;
    }

    // BUG-36203 PSM Optimize
    if( (*aProcInfo)->cache != NULL )
    {
        IDE_TEST( qsx::freeTemplateCache( *aProcInfo ) != IDE_SUCCESS );
        (*aProcInfo)->cache = NULL;
    }
    else
    {
        // Nothing to do.
    }

    // BUG-37296
    if( (*aProcInfo)->qmsMem != NULL )
    {
        IDE_TEST( (*aProcInfo)->qmsMem->destroy() != IDE_SUCCESS );

        IDE_TEST( qcg::freeIduVarMemList( (*aProcInfo)->qmsMem ) != IDE_SUCCESS );
        (*aProcInfo)->qmsMem = NULL;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( iduMemMgr::free( *aProcInfo )
              != IDE_SUCCESS );

    *aProcInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxProc::getProcInfo( qsOID            aProcOID,
                             qsxProcInfo   ** aProcInfo )
{
#define IDE_FN "qsxProc::getProcInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcObjectInfo  * sProcObjectInfo;

    IDE_TEST( getProcObjectInfo( aProcOID,
                                 & sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

    *aProcInfo = sProcObjectInfo->procInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxProc::setProcInfo( qsOID          aProcOID,
                             qsxProcInfo  * aProcInfo )
{
#define IDE_FN "qsxProc::setProcInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcObjectInfo  * sProcObjectInfo;

    IDE_TEST( getProcObjectInfo( aProcOID,
                                 & sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

    sProcObjectInfo->procInfo = aProcInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxProc::setProcText(
    smiStatement * aSmiStmt,
    qsOID          aProcOID,
    SChar        * aProcText,
    SInt           aProcTextLen)
{
#define IDE_FN "qsxProc::setProcText"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));


    IDE_TEST( smiObject::setObjectInfo( aSmiStmt,
                                        smiGetTable( aProcOID ),
                                        (void*) aProcText,
                                        aProcTextLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxProc::getProcText(
    qcStatement * aStatement,
    qsOID         aProcOID,
    SChar      ** aProcText,
    SInt        * aProcTextLen)
{
#define IDE_FN "qsxProc::getProcText"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt sProcTextLen;

    const void * sTableHandle;
    sTableHandle = smiGetTable( aProcOID );

    smiObject::getObjectInfoSize( smiGetTable( aProcOID ), &sProcTextLen );
    if ( *aProcText == NULL )
    {
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc((sProcTextLen + 1),
                                               (void**)aProcText)
                 != IDE_SUCCESS);
    }
    smiObject::getObjectInfo( sTableHandle, (void**) aProcText );
    IDE_TEST_RAISE( *aProcText == NULL, ERR_PROC_NOT_FOUND );

    (*aProcTextLen) = sProcTextLen;

    // make null terminating string : PROJ-1500
    *((*aProcText) + sProcTextLen) = 0;


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PROC_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsxProc::getProcText",
                                  "Procedure not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxProc::latchS( qsOID           aProcOID )
{
#define IDE_FN "qsxProc::latchS"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcObjectInfo  * sProcObjectInfo;
    UInt                 sStage = 0;

    IDE_TEST( getProcObjectInfo( aProcOID,
                                 & sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

    IDE_TEST( sProcObjectInfo->latch.lockRead(
                                NULL, /* idvSQL* */
                                NULL /* idvWeArgs* */ ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST_RAISE( sProcObjectInfo->isAvailable != ID_TRUE,
                    err_latch_info_is_null );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_latch_info_is_null );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_NO_SUCH_LATCH_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void) sProcObjectInfo->latch.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxProc::latchX( qsOID          aProcOID,
                        idBool         aTryLock  )
{
#define IDE_FN "qsxProc::latchX"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    static PDL_Time_Value   sSleep;
    qsxProcObjectInfo     * sProcObjectInfo;
    idBool                  sSuccess = ID_FALSE;
    SInt                    sRetryCount;
    UInt                    sStage = 0;

    sSleep.initialize( 0, RETRY_SLEEP_USEC );

    IDE_TEST( getProcObjectInfo( aProcOID,
                                 & sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

    if( aTryLock == ID_TRUE )
    {
        for ( sRetryCount = 0;
              sRetryCount < MAX_RETRY_COUNT;
              sRetryCount ++ )
        {
            IDE_TEST( sProcObjectInfo->latch.tryLockWrite( &sSuccess )
                      != IDE_SUCCESS );

            if ( sSuccess == ID_TRUE )
            {
                sStage = 1;

                IDE_TEST_RAISE( sProcObjectInfo->isAvailable != ID_TRUE,
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
        IDE_TEST( sProcObjectInfo->latch.lockWrite(
                                    NULL, /* idvSQL* */
                                    NULL /* idvWeArgs* */ )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_latch_info_is_null );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_NO_SUCH_LATCH_OBJECT ) );
    }
    IDE_EXCEPTION( err_resource_busy );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_RESOURCE_BUSY, "try later" ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void) sProcObjectInfo->latch.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxProc::latchXForRecompile( qsOID          aProcOID )
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
    qsxProcObjectInfo     * sProcObjectInfo;
    idBool                  sSuccess = ID_FALSE;
    SInt                    sRetryCount;
    UInt                    sStage = 0;

    sSleep.initialize( 0, RETRY_SLEEP_USEC );

    IDE_TEST( getProcObjectInfo( aProcOID,
                                 & sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

    for ( sRetryCount = 0;
          sRetryCount < MAX_RETRY_COUNT;
          sRetryCount ++ )
    {
        IDE_TEST( sProcObjectInfo->latch.tryLockWrite( &sSuccess )
                  != IDE_SUCCESS );

        if ( sSuccess == ID_TRUE )
        {
            sStage = 1;

            IDE_TEST_RAISE( sProcObjectInfo->isAvailable != ID_TRUE,
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
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_NO_SUCH_LATCH_OBJECT ) );
    }
    IDE_EXCEPTION( err_resource_busy );
    {
        IDE_SET( ideSetErrorCode( qpERR_REBUILD_QCU_RESOURCE_BUSY ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void) sProcObjectInfo->latch.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxProc::unlatch( qsOID          aProcOID )
{
#define IDE_FN "qsxProc::unlatch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxProcObjectInfo  * sProcObjectInfo;

    IDE_TEST( qsxProc::getProcObjectInfo( aProcOID,
                                          &sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

    IDE_TEST( sProcObjectInfo->latch.unlock()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxProc::makeStatusValid( qcStatement * aStatement,
                                 qsOID         aProcOID )
{
    qsxProcObjectInfo * sObjInfo;
    UInt                sState = 0;

    (void) qsxProc::getProcObjectInfo( aProcOID ,
                                       &sObjInfo );

    IDE_TEST( qsxProc::latchXForStatus( aProcOID )
              != IDE_SUCCESS );
    sState = 1;

    // latch를 잡았다면 procInfo가 반드시 있어야 한다.
    IDE_ERROR( sObjInfo->procInfo != NULL );

    if( sObjInfo->procInfo->sessionID == QCG_GET_SESSION_ID( aStatement ) )
    {
        sObjInfo->procInfo->isValid = ID_TRUE;

        IDE_TEST( qcmProc::procUpdateStatus( aStatement,
                                             aProcOID,
                                             QCM_PROC_VALID )
                  != IDE_SUCCESS );
    }
    else
    {
        // 다른 session 에서 PSM을 invalid 시킨 경우
        // valid 상태로 변경하지 않는다.
    }

    sState = 0;
    IDE_TEST( qsxProc::unlatchForStatus( aProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxProc::unlatchForStatus( aProcOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxProc::makeStatusValidTx( qcStatement * aStatement,
                                   qsOID         aProcOID )
{
    qsxProcObjectInfo * sObjInfo;
    UInt                sState = 0;

    (void) qsxProc::getProcObjectInfo( aProcOID,
                                       &sObjInfo );

    IDE_TEST( qsxProc::latchXForStatus( aProcOID )
              != IDE_SUCCESS );
    sState = 1;

    // latch를 잡았다면 procInfo가 반드시 있어야 한다.
    IDE_ERROR( sObjInfo->procInfo != NULL );

    if( sObjInfo->procInfo->sessionID == QCG_GET_SESSION_ID( aStatement ) )
    {
        sObjInfo->procInfo->isValid = ID_TRUE;

        IDE_TEST( qcmProc::procUpdateStatusTx( aStatement,
                                               aProcOID,
                                               QCM_PROC_VALID )
                  != IDE_SUCCESS );
    }
    else
    {
        // 다른 session 에서 PSM을 invalid 시킨 경우
        // valid 상태로 변경하지 않는다.
    }

    sState = 0;
    IDE_TEST( qsxProc::unlatchForStatus( aProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxProc::unlatchForStatus( aProcOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxProc::makeStatusInvalid( qcStatement * aStatement,
                                   qsOID         aProcOID )
{
    IDE_TEST( makeStatusInvalidInternal( aStatement,
                                         aProcOID,
                                         (SChar*)NULL )  // shard split method
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// TASK-7244 Set shard split method to PSM info
IDE_RC qsxProc::makeStatusInvalidAndSetShardSplitMethod ( qcStatement * aStatement,
                                                          qsOID         aProcOID,
                                                          SChar       * aShardSplitMethodStr )
{
    IDE_TEST( makeStatusInvalidInternal( aStatement,
                                         aProcOID,
                                         aShardSplitMethodStr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxProc::makeStatusInvalidInternal( qcStatement * aStatement,
                                           qsOID         aProcOID,
                                           SChar       * aShardSplitMethodStr )
{
    qsxProcObjectInfo * sObjInfo;
    UInt                sState = 0;

    (void) qsxProc::getProcObjectInfo( aProcOID,
                                       &sObjInfo );

    IDE_TEST( qsxProc::latchXForStatus( aProcOID )
              != IDE_SUCCESS );
    sState = 1;

    sObjInfo->procInfo->isValid   = ID_FALSE;
    sObjInfo->procInfo->sessionID = QCG_GET_SESSION_ID( aStatement );

    // TASK-7244 Set shard split method to PSM info
    if ( aShardSplitMethodStr != NULL )
    {
        switch( aShardSplitMethodStr[0] )
        {
        case 'C':
            sObjInfo->procInfo->shardSplitMethod = SDI_SPLIT_CLONE;
            break;
        case 'H':
            sObjInfo->procInfo->shardSplitMethod = SDI_SPLIT_HASH;
            break;
        case 'L':
            sObjInfo->procInfo->shardSplitMethod = SDI_SPLIT_LIST;
            break;
        case 'R':
            sObjInfo->procInfo->shardSplitMethod = SDI_SPLIT_RANGE;
            break;
        case 'S':
            sObjInfo->procInfo->shardSplitMethod = SDI_SPLIT_SOLO;
            break;
        // BUG-48911
        // A shard split method in a procedure info is lost due to a related object modified.
        case 'N':
            sObjInfo->procInfo->shardSplitMethod = SDI_SPLIT_NONE;
            break;
        default:
            IDE_RAISE( ERR_UNEXPECTED );
            break;
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qcmProc::procUpdateStatus( aStatement,
                                         aProcOID,
                                         QCM_PROC_INVALID )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( qsxProc::unlatchForStatus( aProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsxProc::makeStatusInvalidInternal",
                                  "Invalid shard split method." ) );
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxProc::unlatchForStatus( aProcOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxProc::makeStatusInvalidTx( qcStatement * aStatement,
                                     qsOID         aProcOID )
{
    qsxProcObjectInfo * sObjInfo;
    UInt                sState = 0;

    (void) qsxProc::getProcObjectInfo( aProcOID,
                                       &sObjInfo );

    IDE_TEST( qsxProc::latchXForStatus( aProcOID )
              != IDE_SUCCESS );
    sState = 1;

    sObjInfo->procInfo->isValid   = ID_FALSE;
    sObjInfo->procInfo->sessionID = QCG_GET_SESSION_ID( aStatement );

    IDE_TEST( qcmProc::procUpdateStatusTx( aStatement,
                                           aProcOID,
                                           QCM_PROC_INVALID )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( qsxProc::unlatchForStatus( aProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxProc::unlatchForStatus( aProcOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxProc::makeStatusInvalidForRecompile( qcStatement * aStatement,
                                               qsOID         aProcOID )
{
    qsxProcObjectInfo * sObjInfo;
    UInt                sState = 0;

    (void) qsxProc::getProcObjectInfo( aProcOID,
                                       &sObjInfo );

    IDE_TEST( qsxProc::latchXForStatus( aProcOID )
              != IDE_SUCCESS );
    sState = 1;

    sObjInfo->procInfo->isValid   = ID_FALSE;
    sObjInfo->procInfo->sessionID = QCG_GET_SESSION_ID( aStatement );

    sState = 0;
    IDE_TEST( qsxProc::unlatchForStatus( aProcOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)qsxProc::unlatchForStatus( aProcOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxProc::latchXForStatus( qsOID          aProcOID )
{
    static PDL_Time_Value   sSleep;
    qsxProcObjectInfo     * sProcObjectInfo;
    idBool                  sSuccess = ID_FALSE;
    SInt                    sRetryCount;
    UInt                    sStage = 0;

    sSleep.initialize( 0, RETRY_SLEEP_USEC );

    IDE_TEST( getProcObjectInfo( aProcOID,
                                 & sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

retry:

    for ( sRetryCount = 0;
          sRetryCount < MAX_RETRY_COUNT;
          sRetryCount ++ )
    {
        IDE_TEST( sProcObjectInfo->latchForStatus.tryLockWrite( &sSuccess )
                  != IDE_SUCCESS );

        if ( sSuccess == ID_TRUE )
        {
            sStage = 1;

            IDE_TEST_RAISE( sProcObjectInfo->isAvailable != ID_TRUE,
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
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_NO_SUCH_LATCH_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void)sProcObjectInfo->latchForStatus.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxProc::unlatchForStatus( qsOID          aProcOID )
{
    qsxProcObjectInfo  * sProcObjectInfo;

    IDE_TEST( qsxProc::getProcObjectInfo( aProcOID,
                                          &sProcObjectInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sProcObjectInfo != NULL );

    IDE_TEST( sProcObjectInfo->latchForStatus.unlock()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2446 ONE SOURCE */
IDE_RC qsxProc::createProcObjAndInfoCallback( smiStatement * aSmiStmt,
                                              qsOID       aProcOID )
{
    IDE_ASSERT( aSmiStmt != NULL );
    IDE_ASSERT( aProcOID != SM_NULL_OID );

    return qsx::qsxLoadProcByProcOID( aSmiStmt,
                                      aProcOID,
                                      ID_TRUE,      // aCreateProcInfo
                                      ID_TRUE );    // aIsUseTx
}
