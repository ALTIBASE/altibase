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
 * $Id: qsxLibrary.cpp 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idx.h>
#include <qsxLibrary.h>

acl_mem_pool_t     gLibraryMemPool;
acl_hash_table_t   gLibraryHashTable;
UInt               gBucketCount;

IDE_RC qsxLibrary::initializeStatic()
{
    UInt sStage = 0;

    IDE_TEST_RAISE( ACP_RC_NOT_SUCCESS( aclMemPoolCreate( &gLibraryMemPool,
                                                          ID_SIZEOF(qsxLibraryNode),
                                                          QSX_LIBRARY_HASH_POOL_SIZE,
                                                          1 /* ParallelFactor */ ) ),
                    ERR_ALLOC_MEM_POOL );
    sStage = 1;

    gBucketCount = 32 * 2 + 1;
    IDE_TEST_RAISE( ACP_RC_NOT_SUCCESS( aclHashCreate( &gLibraryHashTable,
                                                       gBucketCount,
                                                       QSX_LIBRARY_HASH_KEY_SIZE,
                                                       aclHashHashCString,
                                                       aclHashCompCString,
                                                       ACP_TRUE ) ),
                    ERR_ALLOC_HASH_TABLE );
    sStage = 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALLOC_MEM_POOL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_MEM_POOL_ALLOC_FAILED ) );
    }
    IDE_EXCEPTION( ERR_ALLOC_HASH_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_HASH_TABLE_ALLOC_FAILED ) );
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
    case 1:
        {
            aclMemPoolDestroy( &gLibraryMemPool );
            break;
        }
    default:
        {
            // Nothing to do.
            break;
        }
    }

    return IDE_FAILURE;
}

void qsxLibrary::finalizeStatic()
{
    qsxLibraryNode      * sLibraryNode;
    acl_hash_traverse_t   sHashTraverse;
    acp_rc_t              sRC;

    sRC = aclHashTraverseOpen( &sHashTraverse,
                               &gLibraryHashTable,
                               ACP_TRUE );
    if ( ACP_RC_IS_SUCCESS( sRC ) )
    {
        while ( ACP_RC_NOT_EOF( sRC ) )
        {
            /* ACP_RC_SUCCESS, ACP_RC_EOF 둘 중 하나이다. */
            sRC = aclHashTraverseNext( &sHashTraverse, (void **)&sLibraryNode );

            if ( ACP_RC_IS_SUCCESS( sRC ) )
            {
                /* Server를 종료하는 상황이므로, 실패해도 굳이 Server를 비정상 종료시키지 않는다. */
                (void)sLibraryNode->mMutex.destroy();

                aclMemPoolFree( &gLibraryMemPool, (void *)sLibraryNode );
            }
            else
            {
                IDE_DASSERT( sRC == ACP_RC_EOF );
            }
        }

        aclHashTraverseClose( &sHashTraverse );

        aclHashDestroy( &gLibraryHashTable );

        /* Hash Table을 제거한 후에 Memory Pool을 제거해야 한다. */
        aclMemPoolDestroy( &gLibraryMemPool );
    }
    else
    {
        /* Server가 비정상 종료할 수 있으므로, 아무것도 하지 않는다. */
    }

    return;
}

IDE_RC qsxLibrary::findLibraryFromHashTable( SChar           * aLibPath,
                                             qsxLibraryNode ** aLibrarynode )
{
    acp_rc_t sRC;

    /* It returns ACP_RC_ENOENT if not found */
    sRC = aclHashFind( &gLibraryHashTable,
                       (void *)aLibPath,
                       (void **)aLibrarynode);

    switch ( sRC )
    {
        case ACP_RC_SUCCESS :
            /* Nothing to do */
            break;

        case ACP_RC_ENOENT :
            *aLibrarynode = NULL;
            break;

        default :
            IDE_RAISE( ERR_FIND_HASH_TABLE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FIND_HASH_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_HASH_TABLE_FIND_FAILED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxLibrary::loadLibrary( qsxLibraryNode ** aLibraryNode,
                                void           ** aFunctionPtr,
                                SChar           * aLibPath,
                                SChar           * aFunctionName )
{
    qsxLibraryNode * sLibraryNode = NULL;
    SChar            sLibPath[IDX_LIB_PATH_MAXLEN];
    PDL_stat         sLibFileInfo;
    SInt             sRet;
    idBool           sIsLocked = ID_FALSE;

    IDE_TEST( idlOS::strlen(aLibPath) > IDX_LIB_NAME_MAXLEN );

    IDE_TEST( findLibraryFromHashTable( aLibPath, &sLibraryNode ) != IDE_SUCCESS );

    if ( sLibraryNode == NULL )
    {
        IDE_TEST( makeAndAddLibraryToHashTable( aLibPath,
                                                &sLibraryNode )
                  != IDE_SUCCESS );

        if ( sLibraryNode == NULL )
        {
            IDE_TEST( findLibraryFromHashTable( aLibPath, &sLibraryNode ) != IDE_SUCCESS );

            IDE_ERROR( sLibraryNode != NULL );
        }
    }

    IDE_TEST( sLibraryNode->mMutex.lock( NULL ) != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    if ( sLibraryNode->mHandle == PDL_SHLIB_INVALID_HANDLE )
    {
        IDE_ERROR( sLibraryNode->mRefCount == 0 );

        idlOS::snprintf( sLibPath,
                         IDX_LIB_PATH_MAXLEN,
                         "%s%s%s",
                         idxLocalSock::mHomePath,
                         IDX_LIB_DEFAULT_DIR,
                         aLibPath );
        sLibraryNode->mHandle = idlOS::dlopen( sLibPath, RTLD_LAZY );

        if ( sLibraryNode->mHandle != PDL_SHLIB_INVALID_HANDLE )
        {
            sLibraryNode->mRefCount++;

            sRet = idlOS::stat( sLibPath, &sLibFileInfo );

            if ( sRet == 0 )
            {
                sLibraryNode->mFileSize   = sLibFileInfo.st_size;
                sLibraryNode->mCreateTime = sLibFileInfo.st_mtime;
            }

            sLibraryNode->mOpenTime = (time_t)((idlOS::gettimeofday()).sec());
        }
    }

    if ( sLibraryNode->mFunctionPtr == NULL )
    {
        if ( sLibraryNode->mHandle != PDL_SHLIB_INVALID_HANDLE )
        {
            sLibraryNode->mFunctionPtr = idlOS::dlsym( sLibraryNode->mHandle, "entryfunction" );
        }
        else
        {
            sLibraryNode->mFunctionPtr = NULL;
        }
    }

    if ( sLibraryNode->mFunctionPtr != NULL)
    {
        *aFunctionPtr = idlOS::dlsym( sLibraryNode->mHandle, aFunctionName );

        if ( *aFunctionPtr != NULL )
        {
            sLibraryNode->mRefCount++;
        }
    }
    else
    {
        *aFunctionPtr = NULL;
    }

    sIsLocked = ID_FALSE;
    IDE_TEST( sLibraryNode->mMutex.unlock( )
              != IDE_SUCCESS );

    *aLibraryNode = sLibraryNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;;

    if ( (sIsLocked == ID_TRUE) &&
         (sLibraryNode != NULL) )
    {
        (void)sLibraryNode->mMutex.unlock( );
    }

    return IDE_FAILURE;
}

IDE_RC qsxLibrary::unloadLibrary( qsxLibraryNode * aLibraryNode,
                                  void          ** aFunctionPtr )
{
    idBool sIsLocked = ID_FALSE;

    IDE_TEST( aLibraryNode->mMutex.lock( NULL ) != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    if ( *aFunctionPtr != NULL )
    {
        *aFunctionPtr = NULL;
        aLibraryNode->mRefCount--;
    }

    if ( (aLibraryNode->mRefCount == 1) &&
         (aLibraryNode->mHandle != PDL_SHLIB_INVALID_HANDLE) )
    {
        idlOS::dlclose( aLibraryNode->mHandle );

        QSX_INIT_LIBRARY( aLibraryNode );
    }

    sIsLocked = ID_FALSE;
    IDE_TEST( aLibraryNode->mMutex.unlock( )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        (void)aLibraryNode->mMutex.unlock();
    }

    return IDE_FAILURE;
}

IDE_RC qsxLibrary::makeAndAddLibraryToHashTable( SChar           * aLibPath,
                                                 qsxLibraryNode ** aLibraryNode )
{
    qsxLibraryNode * sLibraryNode;
    SChar            sName[IDU_MUTEX_NAME_LEN];
    UInt             sStage = 0;
    idBool           sAlreadyExist;
    UInt             sKeyLen;

    IDE_TEST_RAISE( ACP_RC_NOT_SUCCESS( aclMemPoolAlloc( &gLibraryMemPool,
                                                         (void **)&sLibraryNode ) ),
                    ERR_ALLOC_MEM_POOL );
    sStage = 1;

    sKeyLen = idlOS::strlen(aLibPath);

    IDE_TEST_RAISE( sKeyLen >= QSX_LIBRARY_HASH_KEY_SIZE,
                    ERR_UNEXPECTED );

    idlOS::strncpy( sLibraryNode->mLibPath, aLibPath, sKeyLen );
    idlOS::memset( sLibraryNode->mLibPath + sKeyLen, '\0', QSX_LIBRARY_HASH_KEY_SIZE - sKeyLen );

    QSX_INIT_LIBRARY( sLibraryNode );

    (void)idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "INTERNAL_LIB_%s", aLibPath);
    IDE_TEST( sLibraryNode->mMutex.initialize( sName,
                                               IDU_MUTEX_KIND_POSIX,
                                               IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( addLibraryToHashTable( sLibraryNode, &sAlreadyExist ) != IDE_SUCCESS );

    if ( sAlreadyExist != ID_TRUE )
    {
        *aLibraryNode = sLibraryNode;
    }
    else
    {
        sStage = 1;
        IDE_TEST( sLibraryNode->mMutex.destroy() != IDE_SUCCESS );

        sStage = 0;
        aclMemPoolFree( &gLibraryMemPool, (void *)sLibraryNode );

        *aLibraryNode = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALLOC_MEM_POOL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_MEM_POOL_ALLOC_FAILED ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                  "File name size is too long" ) );
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
    case 2:
        (void)sLibraryNode->mMutex.destroy();
    case 1:
        aclMemPoolFree( &gLibraryMemPool, (void *)sLibraryNode );
        break;
    default:
        break;
    }

    return IDE_FAILURE;
}

IDE_RC qsxLibrary::addLibraryToHashTable( qsxLibraryNode * aLibraryNode,
                                          idBool         * aAlreadyExist )
{
    acp_rc_t    sRC;

    /* It returns ACP_RC_EEXIST if it already exist */
    sRC = aclHashAdd( &gLibraryHashTable,
                      (void *)aLibraryNode->mLibPath,
                      (void *)aLibraryNode );

    switch ( sRC )
    {
        case ACP_RC_SUCCESS :
            *aAlreadyExist = ID_FALSE;
            break;

        case ACP_RC_EEXIST :
            *aAlreadyExist = ID_TRUE;
            break;

        default :
            IDE_RAISE( ERR_ADD_HASH_TABLE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ADD_HASH_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_HASH_TABLE_ADD_FAILED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsxLibrary::callintProc( iduMemory      * aExeMem,
                         idxIntProcMsg  * aMsg,
                         void           * aArg )
{
    UInt                  i = 0;

    /* for dynamic function call */
    void                * sFuncPtr          = NULL;
    void               ** sFuncArgs         = NULL;
    void               ** sReturnArgPtr;
    void                * sReturnStrArg     = NULL;
    void                * sReturnOtherArg   = NULL;

    idxParamInfo        * sOriginalParams   = NULL;
    ideErrorMgr         * sErrorMgr         = ideGetErrorMgr();
    idBool                sIsTransientDisable = sErrorMgr->mFaultMgr.mIsTransientDisable;

    sFuncPtr = aMsg->mFuncPtr;

    IDE_TEST_RAISE( sFuncPtr == NULL, err_fail_to_call_function );

    if( aMsg->mParamCount > 0 )
    {
        IDE_TEST( aExeMem->alloc( ID_SIZEOF(idxParamInfo) * aMsg->mParamCount,
                                  (void **)&sOriginalParams ) != IDE_SUCCESS );

        idxProc::backupParamProperty( sOriginalParams,
                             aMsg->mParamInfos,
                             aMsg->mParamCount );

        IDE_TEST( aExeMem->alloc( ID_SIZEOF(void*) * aMsg->mParamCount,
                                  (void **)&sFuncArgs )
                  != IDE_SUCCESS );

        /* Prepare argument values & count output arguments */
        for( i = 0; i < aMsg->mParamCount; i++ )
        {
            if( aMsg->mParamInfos[i].mPropType == IDX_TYPE_PROP_NONE )
            {
                IDE_TEST_RAISE( idxProc::setParamPtr( &aMsg->mParamInfos[i], &sFuncArgs[i] )
                                    != IDE_SUCCESS,
                                err_set_paramptr_failure );
            }
            else
            {
                switch( aMsg->mParamInfos[i].mPropType )
                {
                    case IDX_TYPE_PROP_IND:
                        sFuncArgs[i] = &aMsg->mParamInfos[i].mIndicator;
                        break;
                    case IDX_TYPE_PROP_LEN:
                        sFuncArgs[i] = &aMsg->mParamInfos[i].mLength;
                        break;
                    case IDX_TYPE_PROP_MAX:
                        sFuncArgs[i] = &aMsg->mParamInfos[i].mMaxLength;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    IDE_FT_BEGIN();

    IDE_FT_VOLATILE_TRANSIENT_DISABLE(ID_FALSE);

    if( aMsg->mReturnInfo.mSize > 0 )
    {
        if( aMsg->mReturnInfo.mType != IDX_TYPE_CHAR )
        {
            idxProc::setParamPtr( &aMsg->mReturnInfo, &sReturnOtherArg );
            sReturnArgPtr = &sReturnOtherArg;
        }
        else
        {
            sReturnArgPtr = &sReturnStrArg;
        }
    }
    else
    {
        sReturnOtherArg = NULL;
        sReturnArgPtr   = &sReturnOtherArg;
    }

    /**********************************************************************
     * 5. Calling the function
     **********************************************************************/

    ((void (*)( SChar*, ...))sFuncPtr)( aMsg->mFuncName,
                                        aMsg->mParamCount,
                                        sFuncArgs,
                                        sReturnArgPtr,
                                        aArg );

    IDE_FT_VOLATILE_TRANSIENT_DISABLE(sIsTransientDisable);

    IDE_FT_END();

    /**********************************************************************
     * 6. Validation
     **********************************************************************/
    for( i = 0; i < aMsg->mParamCount; i++ )
    {
        if( aMsg->mParamInfos[i].mPropType != IDX_TYPE_PROP_NONE )
        {
            /* MAXLEN change */
            IDE_TEST_RAISE( aMsg->mParamInfos[i].mMaxLength
                                        != sOriginalParams[i].mMaxLength,
                                        ERR_INVALID_PROPERTY_MANIPULATION );

            /* LENGTH change in non-CHAR type */
            IDE_TEST_RAISE(
                ( aMsg->mParamInfos[i].mType != IDX_TYPE_CHAR )
                && ( aMsg->mParamInfos[i].mLength
                     != sOriginalParams[i].mLength ),
                ERR_INVALID_PROPERTY_MANIPULATION );

            /* LENGTH change in IN type */
            IDE_TEST_RAISE(
                ( aMsg->mParamInfos[i].mMode == IDX_MODE_IN )
                && ( aMsg->mParamInfos[i].mLength
                     != sOriginalParams[i].mLength ),
                ERR_INVALID_PROPERTY_MANIPULATION );

            /* Changed LENGTH is more than MAXLEN */
            IDE_TEST_RAISE(
                ( aMsg->mParamInfos[i].mLength
                  > sOriginalParams[i].mMaxLength ),
                ERR_INVALID_PROPERTY_MANIPULATION );

            /* minus LENGTH value */
            /* minus LENGTH value */
            IDE_TEST_RAISE(
                aMsg->mParamInfos[i].mLength < 0,
                ERR_INVALID_PROPERTY_MANIPULATION );

            /* INDICATOR change in IN mode */
            IDE_TEST_RAISE(
                ( aMsg->mParamInfos[i].mMode == IDX_MODE_IN )
                && ( aMsg->mParamInfos[i].mIndicator
                     != sOriginalParams[i].mIndicator ),
                ERR_INVALID_PROPERTY_MANIPULATION );

            /* Invalid INDICATOR value */
            IDE_TEST_RAISE(
                ( aMsg->mParamInfos[i].mIndicator != 0 )
                && ( aMsg->mParamInfos[i].mIndicator != 1 ),
                ERR_INVALID_PROPERTY_MANIPULATION );
        }
        else
        {
            /* Nothing to do. */
        }

        // BUG-39814 IN Mode CHAR/LOB don't have to re-send
        if ( ( aMsg->mParamInfos[i].mMode == IDX_MODE_IN ) &&
             ( ( aMsg->mParamInfos[i].mType == IDX_TYPE_CHAR ) ||
               ( aMsg->mParamInfos[i].mType == IDX_TYPE_BYTE ) ||
               ( aMsg->mParamInfos[i].mType == IDX_TYPE_LOB ) ) )
        {
            // 1 byte for padding
            aMsg->mParamInfos[i].mSize = idlOS::align8( ID_SIZEOF(idxParamInfo) + 1);
            aMsg->mParamInfos[i].mD.mPointer = NULL;
        }
        else
        {
            // Nothing to do.
        }
    }


    if( aMsg->mReturnInfo.mSize > 0
        && aMsg->mReturnInfo.mType == IDX_TYPE_CHAR )
    {
        // memcpy to real buffer
        if( sReturnStrArg != NULL )
        {
            idlOS::memcpy( aMsg->mReturnInfo.mD.mPointer,
                           sReturnStrArg,
                           aMsg->mReturnInfo.mMaxLength );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_fail_to_call_function )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_CALL_PROCEDURE_FAILURE ) );
    }
    IDE_EXCEPTION( err_set_paramptr_failure )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_CALL_PROCEDURE_FAILURE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_PROPERTY_MANIPULATION )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION ) );
    }
    IDE_EXCEPTION_SIGNAL()
    {
        ideLog::log(IDE_QP_0,
                    "A fault detected in user library. [Library : <%s>] [Function : <%s>]",
                    aMsg->mLibName, aMsg->mFuncName);

        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    IDE_FT_EXCEPTION_END();

    IDE_FT_VOLATILE_TRANSIENT_DISABLE(sIsTransientDisable);

    return IDE_FAILURE;
}

void qsxLibrary::getTimeString( SChar * aBuf, time_t aTime )
{
    struct tm sLocalTime;

    if ( aTime != 0 )
    {
        idlOS::localtime_r( &aTime, &sLocalTime );

        idlOS::snprintf( aBuf,
                         QSX_LIBRARY_TIME_STRING_SIZE,
                         "%4"ID_UINT32_FMT
                         "/%02"ID_UINT32_FMT
                         "/%02"ID_UINT32_FMT
                         " %02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT"",
                         sLocalTime.tm_year + 1900,
                         sLocalTime.tm_mon + 1,
                         sLocalTime.tm_mday,
                         sLocalTime.tm_hour,
                         sLocalTime.tm_min,
                         sLocalTime.tm_sec);
    }
    else
    {
        idlOS::sprintf( aBuf,
                        "0000/00/00 00:00:00" );
    }
}

IDE_RC qsxLibrary::buildRecordForLibrary( idvSQL * /* aStatistics */,
                                          void   *  aHeader,
                                          void   * /* aDumpObj */,
                                          iduFixedTableMemory * aMemory )
{
    qsxLibraryNode    * sLibraryNode = NULL;
    acl_hash_traverse_t sHashTraverse;
    qsLibraryRecord     sLibraryRecord;
    acp_rc_t            sRC;
    idBool              sIsLocked = ID_FALSE;

    sRC = aclHashTraverseOpen( &sHashTraverse,
                               &gLibraryHashTable,
                               ACP_FALSE );

    if ( ACP_RC_IS_SUCCESS( sRC ) )
    {
        while ( ACP_RC_NOT_EOF( sRC ) )
        {
            /* ACP_RC_SUCCESS, ACP_RC_EOF 둘 중 하나이다. */
            sRC = aclHashTraverseNext( &sHashTraverse, (void **)&sLibraryNode );

            if ( ACP_RC_IS_SUCCESS( sRC ) && (sLibraryNode != NULL) )
            {
                IDE_TEST( sLibraryNode->mMutex.lock( NULL ) != IDE_SUCCESS );
                sIsLocked = ID_TRUE;

                sLibraryRecord.mLibPath = sLibraryNode->mLibPath;
                sLibraryRecord.mRefCount = (sLibraryNode->mRefCount - 1);
                sLibraryRecord.mFileSize = sLibraryNode->mFileSize;

                getTimeString( sLibraryRecord.mOpenTimeString,
                               sLibraryNode->mOpenTime );

                getTimeString( sLibraryRecord.mCreateTimeString,
                               sLibraryNode->mCreateTime );

                IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                      aMemory,
                                                      (void*)&sLibraryRecord)
                          != IDE_SUCCESS );

                sIsLocked = ID_FALSE;
                IDE_TEST( sLibraryNode->mMutex.unlock( )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_DASSERT( sRC == ACP_RC_EOF );
            }

        }
        aclHashTraverseClose( &sHashTraverse );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( (sIsLocked == ID_TRUE) &&
         (sLibraryNode != NULL) )
    {
        (void)sLibraryNode->mMutex.unlock();
    }

    return IDE_FAILURE;
}

iduFixedTableColDesc gLibraryColDesc[] =
{
    {
        (SChar *)"FILE_SPEC",
        offsetof(qsLibraryRecord, mLibPath),
        4000,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REFERENCE_COUNT",
        offsetof(qsLibraryRecord, mRefCount),
        IDU_FT_SIZEOF_INTEGER,
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FILE_SIZE",
        offsetof(qsLibraryRecord, mFileSize),
        IDU_FT_SIZEOF_INTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CREATE_TIME",
        offsetof(qsLibraryRecord, mCreateTimeString),
        QSX_LIBRARY_TIME_STRING_SIZE,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OPEN_TIME",
        offsetof(qsLibraryRecord, mOpenTimeString),
        QSX_LIBRARY_TIME_STRING_SIZE,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0, NULL // for internal use
    }
};

iduFixedTableDesc gLibraryTableDesc =
{
    (SChar *)"X$LIBRARY",
    qsxLibrary::buildRecordForLibrary,
    gLibraryColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


