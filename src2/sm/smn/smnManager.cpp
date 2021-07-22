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
 * $Id: smnManager.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <ide.h>
#include <idu.h>
#include <smErrorCode.h>

#include <smDef.h>
#include <smp.h>
#include <svp.h>
#include <sdp.h>
#include <smc.h>
#include <smm.h>
#include <smn.h>
#include <smr.h>
#include <sdn.h>
#include <smnbModule.h>
#include <smnReq.h>
#include <smpManager.h>
#include <smpFixedPageList.h>
#include <sctTableSpaceMgr.h>
#include <svcRecord.h>
#include <svm.h>

smnFreeNodeList smnManager::mBaseFreeNodeList;
iduMemPool      smnManager::mDiskTempPagePool;
idBool          smnManager::mIsInvalidUniqueness; // BUG-47736


IDE_RC smnManager::initialize( void )
{
    mBaseFreeNodeList.mNext = &mBaseFreeNodeList;
    mBaseFreeNodeList.mPrev = &mBaseFreeNodeList;

    // Global Index 초기화
    idlOS::memset( gSmnAllIndex,
                   0x00,
                   ID_SIZEOF(smnIndexType*) * SMI_INDEXTYPE_COUNT );

    IDE_TEST( mDiskTempPagePool.initialize( IDU_MEM_SM_SDN,
                                            (SChar*)"DISK_BTREE_TEMP_PAGE_POOL",
                                            2,
                                            SD_PAGE_SIZE,
                                            128,
                                            IDU_AUTOFREE_CHUNK_LIMIT,	/* ChunkLimit */
                                            ID_TRUE,					/* UseMutex */	
                                            SD_PAGE_SIZE,				/* AlignByte */
                                            ID_FALSE,					/* ForcePooling */
                                            ID_TRUE,					/* GarbageCollection */
                                            ID_TRUE, 					/* HWCacheLine */
                                            IDU_MEMPOOL_TYPE_TIGHT )    /* mempoool type */
              != IDE_SUCCESS );

    //-------------------------------
    // Built In Index Type 추가
    //-------------------------------

    IDE_TEST( appendIndexType( & gSmnSequentialScan ) != IDE_SUCCESS );
    IDE_TEST( appendIndexType( & gSmnBTreeIndex ) != IDE_SUCCESS );
    IDE_TEST( appendIndexType( & gSmnGRIDScan ) != IDE_SUCCESS );

    mIsInvalidUniqueness = ID_FALSE; // BUG-47736

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smnManager::destroy( void )
{

    smnIndexModule * sIndexModule;
    SInt             i;
    SInt             j;

    IDE_TEST( mDiskTempPagePool.destroy() != IDE_SUCCESS );

    for ( i = 0; i < SMI_INDEXTYPE_COUNT; i++)
    {
        if ( gSmnAllIndex[i] == NULL )
        {
            continue;
        }
        else
        {
            for ( j = 0; j < SMI_INDEXIBLE_TABLE_TYPES; j++)
            {
                sIndexModule = gSmnAllIndex[i]->mModule[j];
                if ( sIndexModule != NULL )
                {
                    IDE_TEST( sIndexModule->mReleaseIteratorMem( sIndexModule )
                              != IDE_SUCCESS );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smnManager::appendIndexType( smnIndexType * aIndexType )
{
    SInt i;
    smnIndexModule * sIndexModule;

    IDE_DASSERT( aIndexType != NULL );
    IDE_ASSERT( aIndexType->mTypeID < SMI_INDEXTYPE_COUNT );
    IDE_ASSERT( gSmnAllIndex[aIndexType->mTypeID] == NULL );

    gSmnAllIndex[aIndexType->mTypeID] = aIndexType;

    for ( i = 0; i < SMI_INDEXIBLE_TABLE_TYPES; i++)
    {
        sIndexModule = aIndexType->mModule[i];
        if ( sIndexModule != NULL )
        {
            // To Fix PR-15202
            // Free Node 메모리의 준비 및 해제는
            // Startup 시 다양한 단계에서 호출될 수 있다.
            // Iterator Memory 와 Free Node Memory 준비 단계는
            // 통합해서는 안된다.
            IDE_TEST( sIndexModule->mPrepareIteratorMem( sIndexModule )
                        != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnManager::updateIndexModule(smnIndexModule *aIndexModule)
{
    UInt sType;
    UInt sIndexTypeID;
    UInt sTableTypeID;

    IDE_DASSERT(aIndexModule != NULL);
    sType = aIndexModule->mType;
    sIndexTypeID = SMN_MAKE_INDEX_TYPE_ID(sType);
    sTableTypeID = SMN_MAKE_TABLE_TYPE_ID(sType);

    IDE_ASSERT(sIndexTypeID < SMI_INDEXTYPE_COUNT);
    IDE_ASSERT(gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID] == NULL);

    /* Index Module update */
    gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID] = aIndexModule;

    IDE_TEST(aIndexModule->mPrepareIteratorMem(aIndexModule)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// added for A4
// smaPhysical ager가 제거됨에 따라
// 추가된 함수이며, free node list를 할당한다.
// 인덱스 유형(memory b+ tree,R tree)에 따라
// free node list를 각각 가지게 된다.
// 이 자료는 logical ager에 의해 사용된다.
IDE_RC smnManager::allocFreeNodeList( smnFreeNodeFunc   aFreeNodeFunc,
                                      void**            aFreeNodeList)
{
    smnFreeNodeList   * sFreeNodeList;
    UInt                sState  = 0;
    /*
     * BUG-26931     [5.3.3 release] CodeSonar::Ignored Return Value
     */
    /* smnManager_allocFreeNodeList_malloc_FreeNodeList.tc */
    IDU_FIT_POINT("smnManager::allocFreeNodeList::malloc::FreeNodeList");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                 ID_SIZEOF(smnFreeNodeList),
                                 (void**)&sFreeNodeList,
                                 IDU_MEM_FORCE)
            != IDE_SUCCESS );
    sState = 1;

    sFreeNodeList->mFreeNodeFunc    = aFreeNodeFunc;
    sFreeNodeList->mHead            =  NULL;
    sFreeNodeList->mTail            =  NULL;
    sFreeNodeList->mAddCnt          = 0;
    sFreeNodeList->mHandledCnt      = 0;

    // circual double link list insert.
    sFreeNodeList->mPrev        =  &mBaseFreeNodeList;
    sFreeNodeList->mNext        =  mBaseFreeNodeList.mNext;
    mBaseFreeNodeList.mNext     =  sFreeNodeList;
    sFreeNodeList->mNext->mPrev = sFreeNodeList;

    *aFreeNodeList = sFreeNodeList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sFreeNodeList ) == IDE_SUCCESS );
            sFreeNodeList = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

void smnManager::destroyFreeNodeList(void*  aFreeNodeList)
{
    smnFreeNodeList*  sFreeNodeList;

    sFreeNodeList = (smnFreeNodeList*)aFreeNodeList;

    sFreeNodeList->mPrev->mNext = sFreeNodeList->mNext;
    sFreeNodeList->mNext->mPrev = sFreeNodeList->mPrev;
    sFreeNodeList->mNext = NULL;
    sFreeNodeList->mPrev = NULL;
    IDE_ASSERT( iduMemMgr::free( aFreeNodeList ) == IDE_SUCCESS );

}

IDE_RC smnManager::prepareIdxFreePages( )
{
    // To Fix PR-15202
    // 옛날 코드 형태로 원복

    smnIndexModule * sIndexModule;
    SInt i;
    SInt sMemTableId = SMN_GET_BASE_TABLE_TYPE_ID(SMI_TABLE_MEMORY);

    for ( i = 0; i < SMI_INDEXTYPE_COUNT; i++ )
    {
        if ( gSmnAllIndex[i] != NULL )
        {
            sIndexModule = gSmnAllIndex[i]->mModule[sMemTableId];
            if ( sIndexModule != NULL )
            {
                if ( (sIndexModule->mFlag & SMN_FREE_NODE_LIST_MASK)
                     == SMN_FREE_NODE_LIST_ENABLE )
                {
                    IDE_DASSERT( sIndexModule->mPrepareFreeNodeMem != NULL );

                    IDE_TEST( sIndexModule->mPrepareFreeNodeMem(sIndexModule)
                              != IDE_SUCCESS );

                    /* Volatile table index 모듈에 대해서는
                       mPrepareFreeNodeMem을 호출할 필요가 없다.
                       memory와 자원을 공유해서 사용하기 때문이다. */
                }
                else
                {
                    IDE_DASSERT( sIndexModule->mPrepareFreeNodeMem == NULL );
                }
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // Nothing To Do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// smmManager가 smnManager보다 먼저 destroy되므로 각 index의
// Free node list가 해제되지 않아 문제가 발생한다. BUG-11148
// smmManager::destroy() 에서 본 함수를 호출하여 모든 index의
// Free node list를 해제시킨다.
IDE_RC smnManager::releaseIdxFreePages( )
{

    smnIndexModule * sIndexModule;
    SInt             i;
    SInt             sMemTableId = SMN_GET_BASE_TABLE_TYPE_ID(SMI_TABLE_MEMORY);

    for ( i = 0; i < SMI_INDEXTYPE_COUNT; i++ )
    {
        if ( gSmnAllIndex[i] != NULL )
        {
            sIndexModule = gSmnAllIndex[i]->mModule[sMemTableId];
            if ( sIndexModule != NULL )
            {
                if ( (sIndexModule->mFlag & SMN_FREE_NODE_LIST_MASK)
                     == SMN_FREE_NODE_LIST_ENABLE )
                {
                    IDE_DASSERT( sIndexModule->mReleaseFreeNodeMem != NULL );

                    IDE_TEST( sIndexModule->mReleaseFreeNodeMem( sIndexModule )
                            != IDE_SUCCESS );

                    /* Volatile table index 모듈에 대해서는
                       mReleaseFreeNodeMem을 호출할 필요가 없다.
                       memory와 자원을 공유해서 사용하기 때문이다. */
                }
                else
                {
                    IDE_DASSERT( sIndexModule->mReleaseFreeNodeMem == NULL );
                }
            }
        }
        else
        {
            // Nothing To Do
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smnManager::initializeJobInfo(smnpJobItem* aJobItem)
{
    SInt    sState = 0;

    aJobItem->mNext = NULL;
    aJobItem->mPrev = NULL;
    aJobItem->mJobProcessCnt = 0;
    aJobItem->mIsFinish = ID_FALSE;
    aJobItem->mIsUnique = ID_FALSE;
    aJobItem->mArrNode = NULL;
    aJobItem->mIsFirst = ID_TRUE;
    aJobItem->mIsPersIndex = ID_FALSE;

    IDE_TEST(aJobItem->mMutex.initialize((SChar*)"SMNP_MGR_MUTEX",
                                         IDU_MUTEX_KIND_POSIX,
                                         IDV_WAIT_INDEX_NULL)
             != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(aJobItem->mStack.initialize(IDU_MEM_SM_SMN,
                                         ID_SIZEOF(smnSortStack))
             != IDE_SUCCESS);
    sState = 2;

    IDE_TEST_RAISE( aJobItem->mCV.initialize((SChar *)"SMNP_MGR_COND") != IDE_SUCCESS,
                    err_cond_var_init );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            (void)aJobItem->mStack.destroy();
        case 1:
            (void)aJobItem->mMutex.destroy();
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC smnManager::destroyJobInfo(smnpJobItem* aJobItem)
{
    if ( aJobItem->mArrNode != NULL)
    {
        IDE_TEST(iduMemMgr::free(aJobItem->mArrNode)
                 != IDE_SUCCESS);

        aJobItem->mArrNode = NULL;
    }
    IDE_TEST_RAISE(aJobItem->mCV.destroy() != IDE_SUCCESS,
                   err_cond_var_destroy);

    IDE_TEST(aJobItem->mMutex.destroy() != IDE_SUCCESS);
    IDE_TEST(aJobItem->mStack.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_destroy);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnManager::removeAllIndexFile()
{
    DIR                  *s_pDIR    = NULL;
    struct  dirent       *s_pDirEnt = NULL;
    struct  dirent       *s_pResDirEnt;
    SChar                 s_FullFileName[SM_MAX_FILE_NAME];
    SChar                 s_FileName[SM_MAX_FILE_NAME];
    SChar                 s_backupDir[SM_MAX_FILE_NAME];
    UInt                  s_nLen;
    SInt                  s_rc;

    //To fix BUG-5049
    SChar                 sPrefix[SM_MAX_FILE_NAME];
    idlOS::snprintf( sPrefix, SM_MAX_FILE_NAME,
                     "%s%s", "ALTIBASE_INDEX_",
                     smmDatabase::getDicMemBase()->mDBname );
    s_nLen = idlOS::strlen( sPrefix );

    idlOS::snprintf(s_backupDir, SM_MAX_FILE_NAME,
                    "%s%c", smuProperty::getDBDir(0), IDL_FILE_SEPARATOR);


    /* TC/FIT/Limit/sm/smn/smnManager_removeAllIndexFile_malloc.sql */
    /* smnManager_removeAllIndexFile.tc */
    IDU_FIT_POINT_RAISE( "smnManager::removeAllIndexFile::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMN,
                               ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                               (void**)&s_pDirEnt) != IDE_SUCCESS,
                   insufficient_memory );

    /* smnManager_removeAllIndexFile.tc */
    IDU_FIT_POINT_RAISE( "smnManager::removeAllIndexFile::opendir", err_open_dir );
    s_pDIR = idf::opendir(s_backupDir);
    IDE_TEST_RAISE(s_pDIR == NULL, err_open_dir);

    s_pResDirEnt = NULL;
    errno = 0;

    /* smnManager_removeAllIndexFile.tc */
    IDU_FIT_POINT_RAISE( "smnManager::removeAllIndexFile::readdir_r", err_read_dir );
    s_rc = idf::readdir_r(s_pDIR, s_pDirEnt, &s_pResDirEnt);
    IDE_TEST_RAISE( (s_rc != 0) && (errno != 0), err_read_dir );
    errno = 0;

    while (s_pResDirEnt != NULL)
    {
        idlOS::strcpy(s_FileName, (const char*)s_pResDirEnt->d_name);
        if (idlOS::strcmp(s_FileName, ".") == 0 ||
            idlOS::strcmp(s_FileName, "..") == 0)
        {
            s_pResDirEnt = NULL;
    	    errno = 0;
            s_rc = idf::readdir_r(s_pDIR, s_pDirEnt, &s_pResDirEnt);
            IDE_TEST_RAISE( (s_rc != 0) && (errno != 0), err_read_dir );
            errno = 0;
            continue;
        }

        if ( idlOS::strlen(s_FileName) >= s_nLen)
        {
            //To fix BUG-5049
            if ( idlOS::strncmp(s_FileName, sPrefix, s_nLen) == 0)
            {
                idlOS::snprintf(s_FullFileName, SM_MAX_FILE_NAME,
                                "%s%c%s", s_backupDir,
                                IDL_FILE_SEPARATOR, s_FileName);
                IDE_TEST_RAISE(idf::unlink(s_FullFileName) != 0,
                               err_file_unlink);

                //==========================================================
                // To Fix BUG-13924
                //===========================================================

                ideLog::log(SM_TRC_LOG_LEVEL_MINDEX,
                            SM_TRC_MRECORD_FILE_UNLINK,
                            s_FullFileName);

            }
        }

        s_pResDirEnt = NULL;
        errno = 0;
        s_rc = idf::readdir_r(s_pDIR, s_pDirEnt, &s_pResDirEnt);
        IDE_TEST_RAISE( (s_rc != 0) && (errno != 0), err_read_dir );
        errno = 0;
    }//while

    idf::closedir( s_pDIR );
    s_pDIR = NULL;

    IDE_TEST( iduMemMgr::free( s_pDirEnt ) != IDE_SUCCESS );
    s_pDirEnt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_file_unlink);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_FileDelete, s_FullFileName));
    }
    IDE_EXCEPTION(err_open_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotOpenDir, s_backupDir ) );
    }
    IDE_EXCEPTION(err_read_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotReadDir, s_backupDir ) );
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if ( s_pDIR != NULL )
    {
        idf::closedir( s_pDIR );

        s_pDIR = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( s_pDirEnt != NULL )
    {
        (void)iduMemMgr::free( s_pDirEnt );

        s_pDirEnt = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC smnManager::createIndex( idvSQL             * aStatistics,
                                void               * aTrans,
                                smcTableHeader     * aTable,
                                smnIndexHeader     * aIndex,
                                idBool               aIsRestartBuild,
                                idBool               aIsNeedValidation,
                                idBool               aIsPersistent,
                                smuJobThreadMgr    * aThreadMgr,
                                smiSegAttr         * aSegAttr,
                                smiSegStorageAttr  * aSegStoAttr,
                                smnIndexHeader    ** aRebuildIndexHeader )
{
    UInt                    sTableType;
    UInt                    sState  = 0;
    idBool                  sBuildIndex;
    smrRTOI                 sRTOI;
    smnRebuildMode          sRebuildMode;
    smnRebuildIndexInfo   * sRebuildIndexInfo;

    /* 본 함수의 수행방식은 REBUILD_INDEX_PARALLEL_MODE 프로퍼티에 따라 차이가 있다.
     * 해당 프로퍼티가 TABLE MODE일 경우
     * 직접 인덱스 헤더를 생성 한 후 인덱스를 생성한다.
     * 해당 프로퍼티가 INDEX MODE일 경우
     * 직접 인덱스 헤더를 생성 후 인덱스 생성은 thread 통해 수행한다. */
    sRebuildMode = smuProperty::getRebuildIndexParallelMode() == 0 
        ? SMN_INDEX_REBUILD_TABLE_MODE : SMN_INDEX_REBUILD_INDEX_MODE;
    if ( aThreadMgr != NULL )
    {
        IDE_ASSERT( sRebuildMode == SMN_INDEX_REBUILD_INDEX_MODE );
    }
    else
    {
        //do nothing...
    }

    IDE_TEST(initIndex(aStatistics,
                       aTable,
                       (void *)aIndex,
                       aSegAttr,
                       aSegStoAttr,
                       (void **)aRebuildIndexHeader,
                       0) /* SmoNo */
             != IDE_SUCCESS);

    // unpin된 테이블이나 디스크 테이블의 경우는
    // rebuild index를 하지 않는다.

    /* BUG-14053
       System 메모리 부족시 Server Start시 Meta Table의 Index만을
       Create하고 Start하는 프로퍼티 추가.
       INDEX_REBUILD_AT_STARTUP = 1: Start시 모든 Index Rebuild
       INDEX_REBUILD_AT_STARTUP = 0: Start시 Meta Table의 Index만
       Rebuild
    */
    sTableType  = SMI_GET_TABLE_TYPE( aTable );
    sBuildIndex = ID_FALSE;

    smrRecoveryMgr::initRTOI( &sRTOI );
    sRTOI.mCause    = SMR_RTOI_CAUSE_PROPERTY;
    sRTOI.mType     = SMR_RTOI_TYPE_INDEX;
    sRTOI.mTableOID = aTable->mSelfOID;
    sRTOI.mIndexID  = aIndex->mId;
    sRTOI.mState    = SMR_RTOI_STATE_CHECKED;
       
    if ( ( smuProperty::getIndexRebuildAtStartup() == 1) )
    {
        if ( sTableType == SMI_TABLE_MEMORY )
        {
            sBuildIndex = ID_TRUE;
        }

        /* BUG-24034: [SN: Volatile] Server Start시 Volatile Table의 Index의 Header만
         * 만들고 Build Index를 호출하면 안된다.
         *
         * buildIndex함수안에서 Volatile Table의 NullRow를 참조하는데 이 NullRow는
         * service단계에서 만들어지기 때문에 참조하면 안된다. 그리고 Volatile은 Ser
         * ver start시 Table만 있고 데이타는 없기때문에 buildIndex를 호출할 필요가
         * 없다. */
        if ( ( sTableType == SMI_TABLE_VOLATILE ) && ( aIsRestartBuild == ID_FALSE ) )
        {
            sBuildIndex = ID_TRUE;
        }
    }
    if ( sTableType == SMI_TABLE_META )
    {
        sBuildIndex = ID_TRUE;
    }
    if ( smrRecoveryMgr::isIgnoreObjectByProperty( &sRTOI ) == ID_TRUE ) 
    {
        IDE_TEST( smrRecoveryMgr::startupFailure( &sRTOI, 
                                                  ID_FALSE )  // is redo
                  != IDE_SUCCESS);
        sBuildIndex = ID_FALSE;
    }

    if ( sBuildIndex == ID_TRUE )
    {
        if ( ( aThreadMgr != NULL ) && (sRebuildMode == SMN_INDEX_REBUILD_INDEX_MODE ) )
        {
            /* 인덱스 생성 정보를 thread로 넘기기 위한 정보가 저장되는 공간을 생성한다.
             * 이 공간은 thread가 작업을 끝내면 thread에 의해 제거된다. */
            /* smnManager_createIndex_malloc_RebuildIndexInfo.tc */
            IDU_FIT_POINT("smnManager::createIndex::malloc::RebuildIndexInfo");
            IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                         ID_SIZEOF( smnRebuildIndexInfo ),
                                         (void**)&sRebuildIndexInfo,
                                         IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
            sState  = 1;

            sRebuildIndexInfo->mTable       = aTable;
            sRebuildIndexInfo->mIndexCursor = aIndex;

            IDE_TEST( smuJobThread::addJob( aThreadMgr, sRebuildIndexInfo ) 
                      != IDE_SUCCESS );

            /* BUG-37667
             * cpptest 통과를 위해 sState 추가 하여 예외처리 하였음.
             * 하지만 addJob 함수는 항상 IDE_SUCCESS를 반환하므로 실제로는 의미 없음
             * 그리고 workerThread 가 free를 포함한 buildIndexParallel()를 수행 */
            sState  = 0;
        }
        else
        {
            IDE_TEST( buildIndex( aStatistics,
                                  aTrans,
                                  aTable,
                                  aIndex,
                                  aIsNeedValidation,
                                  aIsPersistent,
                                  0,    //  aParallelDegree
                                  0,    //  aBuildFlag,
                                  0 )   //  aTotalRecCnt
                      != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free(sRebuildIndexInfo) );
    }

    return IDE_FAILURE;

}

IDE_RC smnManager::initIndex( idvSQL             * aStatistics,
                              smcTableHeader     * aTable,
                              void*                aIndex,
                              smiSegAttr         * aSegAttr,
                              smiSegStorageAttr  * aSegStorageAttr,
                              void**               aRebuildIndexHeader,
                              ULong                aSmoNo )
{

    smnIndexModule *  sIndexModule;
    smnIndexHeader *  sIndex = (smnIndexHeader *)aIndex;
    smnIndexHeader ** sRebuildIndexHeader =
                          (smnIndexHeader **)aRebuildIndexHeader;
    UInt              sIndexTypeID = sIndex->mType;
    UInt              sTableTypeID = SMN_GET_BASE_TABLE_TYPE_ID(aTable->mFlag);

    sIndex->mModule = NULL;

    sIndexModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];
    IDE_TEST_RAISE( sIndexModule == NULL, ERR_NOT_FOUND );

    IDE_ASSERT( SMN_MAKE_INDEX_TYPE_ID(sIndexModule->mType) == sIndexTypeID );

    IDE_TEST( sIndexModule->mCreate( aStatistics,
                                     aTable,
                                     sIndex,
                                     aSegAttr,
                                     aSegStorageAttr,
                                     &sIndex->mInsert,
                                     sRebuildIndexHeader,
                                     aSmoNo )
              != IDE_SUCCESS );
    sIndex->mModule = sIndexModule;

    return IDE_SUCCESS;
    /* 해당 type의 인덱스를 찾을 수 없습니다. */
    IDE_EXCEPTION( ERR_NOT_FOUND );
    IDE_SET( ideSetErrorCode( smERR_FATAL_smnNotSupportedIndex ) );
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC  smnManager::reInitIndex(idvSQL* aStatistics, void*    aIndex)
{

    smnIndexHeader *sIndex;

    IDE_ASSERT( aIndex != NULL);

    sIndex= (smnIndexHeader *)aIndex;
    IDE_ASSERT( sIndex->mModule != NULL);

    IDE_TEST(sIndex->mModule->mReInit(aStatistics, sIndex)
             != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

void smnManager::releasePageLatch( smcTableHeader   * aTable,
                                   scPageID           aPageID )
{
    if ( SMN_GET_BASE_TABLE_TYPE( aTable->mFlag ) == SMI_TABLE_VOLATILE )
    {
        (void)svmManager::releasePageLatch( aTable->mSpaceID, aPageID );
    }
    else
    {
        (void)smmManager::releasePageLatch( aTable->mSpaceID, aPageID );
    }
}

IDE_RC smnManager::getNextPageForMemTable( smcTableHeader  * aTable,
                                           scPageID        * aPageID,
                                           idBool          * aLocked )
{
    scPageID     sPageID = *aPageID;

    if ( sPageID == SM_NULL_PID )
    {
        sPageID = smpManager::getFirstAllocPageID(
            &(aTable->mFixed.mMRDB) );
    }
    else
    {
        // 현재 page에 S래치 해제
        (*aLocked) = ID_FALSE;
        IDE_TEST( smmManager::releasePageLatch( aTable->mSpaceID,
                                                sPageID )
                  != IDE_SUCCESS );

        sPageID = smpManager::getNextAllocPageID(
            aTable->mSpaceID,
            &(aTable->mFixed.mMRDB),
            sPageID );
    }

    if ( sPageID != SM_NULL_PID )
    {
        // 다음 page에 S래치 획득
        IDE_TEST( smmManager::holdPageSLatch( aTable->mSpaceID,
                                              sPageID )
                  != IDE_SUCCESS );
        (*aLocked) = ID_TRUE;
    }

    *aPageID = sPageID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( (*aLocked) == ID_TRUE )
    {
        (void)smmManager::releasePageLatch( aTable->mSpaceID, sPageID );
        (*aLocked) = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC smnManager::getNextPageForVolTable( smcTableHeader  * aTable,
                                           scPageID        * aPageID,
                                           idBool          * aLocked )
{
    scPageID     sPageID = *aPageID;

    if ( sPageID == SM_NULL_PID )
    {
        sPageID = svpManager::getFirstAllocPageID(
            &(aTable->mFixed.mVRDB) );
    }
    else
    {
        // 현재 page에 S래치 해제
        (*aLocked) = ID_FALSE;
        IDE_TEST( svmManager::releasePageLatch( aTable->mSpaceID,
                                                sPageID )
                  != IDE_SUCCESS );

        sPageID = svpManager::getNextAllocPageID(
            aTable->mSpaceID,
            &(aTable->mFixed.mVRDB),
            sPageID );
    }

    if ( sPageID != SM_NULL_PID )
    {
        // 다음 page에 S래치 획득
        IDE_TEST( svmManager::holdPageSLatch( aTable->mSpaceID,
                                              sPageID )
                  != IDE_SUCCESS );
        (*aLocked) = ID_TRUE;
    }

    *aPageID = sPageID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( (*aLocked) == ID_TRUE )
    {
        (void)svmManager::releasePageLatch( aTable->mSpaceID, sPageID );
        (*aLocked) = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC smnManager::getNextRowForMemTable( smcTableHeader   * aTable,
                                          scPageID           aPageID,
                                          SChar           ** aFence,
                                          SChar           ** aRow,
                                          idBool             aIsNeedValidation )
{
    smOID               sOID;
    smSCN               sSCN;
    smTID               sDummyTID;
    smpSlotHeader     * sRow = (smpSlotHeader*)*aRow;
    vULong              sSize;
    vULong              sLast;

    sSize   = aTable->mFixed.mMRDB.mSlotSize;

    if ( sRow == NULL ) // 첫번째 row
    {
        // 첫번째 row 획득
        sOID = SM_MAKE_OID( aPageID, SMP_PERS_PAGE_BODY_OFFSET );
        IDE_ASSERT( smmManager::getOIDPtr( aTable->mSpaceID,
                                           sOID,
                                           (void**)&sRow )
                    == IDE_SUCCESS );
        sLast   = sSize * ( aTable->mFixed.mMRDB.mSlotCount );
        *aFence = (SChar*)sRow + sLast;
    }
    else
    {
        // access된 row의 다음 row를 획득
        sRow = (smpSlotHeader*)( (SChar*)sRow + sSize );
    }

    *aRow = NULL;

    for ( ; (SChar*)sRow < *aFence;
         sRow = (smpSlotHeader*)((SChar*)sRow + sSize) )
    {
        if ( SM_SCN_IS_FREE_ROW( sRow->mLimitSCN ) )
        {
            SMX_GET_SCN_AND_TID( sRow->mCreateSCN, sSCN, sDummyTID );

            IDE_TEST_RAISE( ( aIsNeedValidation == ID_TRUE &&
                              SM_SCN_IS_INFINITE( sSCN ) ) &&
                            SM_SCN_IS_NOT_DELETED( sSCN ),
                            ERR_UNCOMMITED_ROW );

            if ( SM_SCN_IS_NOT_DELETED( sSCN ) &&
                !SM_SCN_IS_NULL_ROW( sSCN ) )
            {
                *aRow = (SChar*)sRow;
                break;
            }
        }
        else
        {
            /* BUG-32926 [SM] when server restart, Prepared row in the rebuilding
             *           memory index process should be read.
             * XA는 2 phase commit으로 transaction이 prepared로 되면,
             * Infinite SCN을 가진 Row 또는 Next version을 가지는 Row 가
             * 존재할 수 있다.
             * 따라서 server startup시 Next가 NULL OID가 아니더라도 읽어야 하며
             * Infinite SCN을 가지는 Row 또한 읽어야 한다.
             * 그리고 server startup시에만 aIsNeedValidation이 ID_FALSE로
             * 내려 온다. */
            if ( aIsNeedValidation == ID_FALSE )
            {
                if ( SM_SCN_IS_NOT_DELETED(sRow->mCreateSCN) )
                {
                    *aRow = (SChar*)sRow;
                    break;
                }
                else
                {
                    /* nothing */
                }
            }
            else
            {
                /* create table 작업은 테이블 lock(x)를 잡고 수행되므로 uncommit 이 있을수 없다. */
                IDE_TEST_RAISE( SM_SCN_IS_INFINITE( sRow->mLimitSCN ), 
                                ERR_UNCOMMITED_ROW );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNCOMMITED_ROW );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_Uncommitted_Row_Found ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnManager::getNextRowForVolTable( smcTableHeader   * aTable,
                                          scPageID           aPageID,
                                          SChar           ** aFence,
                                          SChar           ** aRow,
                                          idBool             aIsNeedValidation )
{
    smOID               sOID;
    smSCN               sSCN;
    smTID               sDummyTID;
    smpSlotHeader     * sRow = (smpSlotHeader*)*aRow;
    vULong              sSize;
    vULong              sLast;

    sSize   = aTable->mFixed.mVRDB.mSlotSize;

    if ( sRow == NULL ) // 첫번째 row
    {
        // 첫번째 row 획득
        sOID    = SM_MAKE_OID( aPageID, SMP_PERS_PAGE_BODY_OFFSET );
        IDE_ASSERT( smmManager::getOIDPtr( aTable->mSpaceID,
                                           sOID,
                                           (void**)&sRow )
                    == IDE_SUCCESS );
        sLast   = sSize * ( aTable->mFixed.mVRDB.mSlotCount );
        *aFence = (SChar*)sRow + sLast;
    }
    else
    {
        // 다음 row를 획득
        sRow = (smpSlotHeader*)( (SChar*)sRow + sSize );
    }

    *aRow = NULL;

    for ( ; (SChar*)sRow < *aFence;
         sRow = (smpSlotHeader*)((SChar*)sRow + sSize) )
    {
        if ( SM_SCN_IS_FREE_ROW( sRow->mLimitSCN ) )
        {
            SMX_GET_SCN_AND_TID( sRow->mCreateSCN, sSCN, sDummyTID );

            IDE_TEST_RAISE( ( aIsNeedValidation == ID_TRUE &&
                              SM_SCN_IS_INFINITE( sSCN )) &&
                            SM_SCN_IS_NOT_DELETED( sSCN ),
                            ERR_UNCOMMITED_ROW );

            if ( SM_SCN_IS_NOT_DELETED( sSCN ) &&
                !SM_SCN_IS_NULL_ROW( sSCN ) )
            {
                *aRow = (SChar*)sRow;
                break;
            }
        }            
        else
        {
            if ( aIsNeedValidation != ID_FALSE )
            {
                /* create table 작업은 테이블 lock(x)를 잡고 수행되므로 uncommit 이 있을수 없다. */
                IDE_TEST_RAISE( SM_SCN_IS_INFINITE( sRow->mLimitSCN ), 
                                ERR_UNCOMMITED_ROW );
            }
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNCOMMITED_ROW );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_Uncommitted_Row_Found ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*******************************************************************************
 * Description: Index 구조 개선을 위해 Build Index를 하나로 통합합니다.
 *
 * Related Issues:
 *      BUG-25279 Btree For Spatial과 Disk Btree의 자료구조 및 로깅 분리
 *
 * aStatistics          - [IN] idvSQL
 * aTrans               - [IN] smxTrans
 * aTable               - [IN] 대상 index가 속한 테이블의 table header
 * aIndex               - [IN] 대상 index header
 * aIsNeedValidation    - [IN] Uncommitted row validation 여부, MRVR Only
 * aIsPersistent        - [IN] MRVR Only
 * aParallelDegree      - [IN] index parallel build degree
 * aBuildFlag           - [IN] Index build 옵션, Disk Only
 * aTotalRecCnt         - [IN] 대상 table의 record count, Disk Only
 ******************************************************************************/
IDE_RC smnManager::buildIndex( idvSQL              * aStatistics,
                               void                * aTrans,
                               smcTableHeader      * aTable,
                               smnIndexHeader      * aIndex,
                               idBool                aIsNeedValidation,
                               idBool                aIsPersistent,
                               UInt                  aParallelDegree,
                               UInt                  aBuildFlag,
                               UInt                  aTotalRecCnt )
{
    smnGetPageFunc   sGetPageFunc;
    smnGetRowFunc    sGetRowFunc;
    SChar          * sNullPtr;
    idBool           sIsNeedValidation;
    idBool           sIsPersistent;

    sGetRowFunc  = NULL;
    sNullPtr     = NULL;
    sGetPageFunc = NULL;
    sIsNeedValidation = aIsNeedValidation;
    sIsPersistent     = aIsPersistent;

    IDE_ERROR( aIndex->mModule != NULL );

    switch( SMI_GET_TABLE_TYPE( aTable ) )
    {
        case SMI_TABLE_DISK    :
            sIsPersistent = ID_TRUE;
            break;
        case SMI_TABLE_MEMORY  :
        case SMI_TABLE_META:
            sGetPageFunc = smnManager::getNextPageForMemTable;
            sGetRowFunc  = smnManager::getNextRowForMemTable;
            IDE_ERROR( smmManager::getOIDPtr( aTable->mSpaceID,
                                              aTable->mNullOID,
                                              (void**)&sNullPtr )
                       == IDE_SUCCESS );
            break;
        case SMI_TABLE_VOLATILE:
            sGetPageFunc = smnManager::getNextPageForVolTable;
            sGetRowFunc  = smnManager::getNextRowForVolTable;
            IDE_ERROR( smmManager::getOIDPtr( aTable->mSpaceID,
                                              aTable->mNullOID,
                                              (void**)&sNullPtr )
                       == IDE_SUCCESS );
            break;
        case SMI_TABLE_TEMP_LEGACY:
        default:
            break;
    }

    IDE_TEST( aIndex->mModule->mBuild( aStatistics,
                                       aTrans,
                                       aTable,
                                       aIndex,
                                       sGetPageFunc,
                                       sGetRowFunc,
                                       sNullPtr,
                                       sIsNeedValidation,
                                       sIsPersistent,
                                       aParallelDegree,
                                       aBuildFlag,
                                       aTotalRecCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smnManager::setIndexCreated( void* aIndexHeader, idBool aIsCreated )
{

    IDE_DASSERT( aIndexHeader != NULL );

    *(idBool*)((smnIndexHeader*)aIndexHeader)->mHeader = aIsCreated;

}

idBool smnManager::getUniqueCheck(const void* aIndexHeader)
{

    const smnIndexHeader *  sIndex;

    IDE_ASSERT(aIndexHeader != NULL);

    sIndex = (const smnIndexHeader *)aIndexHeader;

    if ( ( sIndex->mFlag & SMI_INDEX_UNIQUE_MASK )==SMI_INDEX_UNIQUE_ENABLE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }


}

IDE_RC smnManager::dropIndex( smcTableHeader * aTable,
                              void           * aIndexHeader )
{
    smnIndexModule *  sIndexModule;
    smnIndexHeader *  sIndex = (smnIndexHeader *)aIndexHeader;
    UInt              sIndexTypeID = sIndex->mType;
    UInt              sTableTypeID = SMN_GET_BASE_TABLE_TYPE_ID(aTable->mFlag);

    IDE_ERROR( sTableTypeID != 
               SMN_GET_BASE_TABLE_TYPE_ID( SMI_TABLE_TEMP_LEGACY ) );
    if ( sIndexTypeID == SMI_BUILTIN_B_TREE2_INDEXTYPE_ID)
    {
        if ( (sTableTypeID == SMN_GET_BASE_TABLE_TYPE_ID( SMI_TABLE_MEMORY)) ||
           (sTableTypeID == SMN_GET_BASE_TABLE_TYPE_ID( SMI_TABLE_VOLATILE)))
        {
            sIndexTypeID = SMI_BUILTIN_B_TREE_INDEXTYPE_ID;
        }
    }

    sIndexModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];
    IDE_TEST_RAISE( sIndexModule == NULL, ERR_NOT_FOUND );
    IDE_ASSERT( SMN_MAKE_INDEX_TYPE_ID(sIndexModule->mType) == sIndexTypeID );

    IDE_TEST( sIndexModule->mDrop( sIndex )
              != IDE_SUCCESS );
    sIndex->mModule = NULL;

    return IDE_SUCCESS;

    /* 해당 type의 인덱스를 찾을 수 없습니다. */
    IDE_EXCEPTION( ERR_NOT_FOUND );
    IDE_SET( ideSetErrorCode( smERR_FATAL_smnNotSupportedIndex ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description : 디스크 인덱스의 무결성 검사를 수행여부 판단
 *
 * (1) __SM_CHECK_DISK_INDEX_INTEGRITY = 1
 *    디스크 BTREE만 ID_TRUE
 *
 * (2) __SM_CHECK_DISK_INDEX_INTEGRITY = 3 모드에서
 *     __SM_VERIFY_DISK_INDEX_COUNT 만큼
 *     __SM_VERIFY_DISK_INDEX_NAME 에 해당하는 Index만 ID_TRUE 반환
 *
 *  aIndexHeader - [IN] Index Header Ptr
 *
 **********************************************************************/
idBool smnManager::isIndexToVerifyIntegrity( const void * aIndexHeader )
{
    UInt   i;
    idBool sResult;
    UInt   sIndexCnt;
    const  smnIndexHeader * sIndexHeader =
              (const smnIndexHeader*)aIndexHeader;

    sResult = ID_FALSE;

    /* BUG-27774 __SM_CHECK_DISK_INDEX_INTEGRITY 프로퍼티 활성화시 B-Tree에
     * 대해서만 무결성 검증을 수행해야함 */
    IDE_TEST_RAISE( sIndexHeader->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
                    cont_finish );

    /* Level 1이나 Level 3이 들어올 수 있는데 Level 1인 경우에는
     * 무조건 모든 디스크 B-TREE 인덱스에 대해 무결성검사를 수행한다. */
    sResult = ID_TRUE;

    IDE_TEST_RAISE( smuProperty::getCheckDiskIndexIntegrity()
                    != SMU_CHECK_DSKINDEX_INTEGRITY_LEVEL3,
                    cont_finish );

    sResult   = ID_FALSE;
    sIndexCnt = smuProperty::getVerifyDiskIndexCount();

    for ( i = 0; i < sIndexCnt; i++ )
    {
        if ( idlOS::strncmp( sIndexHeader->mName,
                             smuProperty::getVerifyDiskIndexName(i),
                             (SMN_MAX_INDEX_NAME_SIZE + 8)) == 0 )
        {
            sResult = ID_TRUE;
            break;
        }
    }

    /* BUG-40385 sResult 값에 따라 Failure 리턴일 수 있으므로,
     * 위에 IDE_TEST_RAISE -> IDE_TEST_CONT 로 변환하지 않는다. */
    IDE_EXCEPTION_CONT( cont_finish );

    return sResult;
}


/***********************************************************************
 *
 * Description : 서버구동시에 인덱스 무결성을 __SM_CHECK_DISK_INDEX_INTEGRITY
 *               프로퍼티에 따라서 검증한다.
 *
 **********************************************************************/
IDE_RC smnManager::verifyIndexIntegrityAtStartUp()
{
    SChar    sStrBuffer[128];

    IDE_TEST_CONT( smuProperty::getCheckDiskIndexIntegrity()
                    == SMU_CHECK_DSKINDEX_INTEGRITY_DISABLED,
                    skip_check_index_integrity );

    idlOS::snprintf(
            sStrBuffer,
            128,
            "  [SM] [BEGIN : CHECK DISK INDEX INTEGRITY (prop. : %"ID_UINT32_FMT")]\n",
            smuProperty::getCheckDiskIndexIntegrity() );
    IDE_CALLBACK_SEND_SYM( sStrBuffer );

    switch( smuProperty::getCheckDiskIndexIntegrity() )
    {
        case SMU_CHECK_DSKINDEX_INTEGRITY_LEVEL2:

            IDE_TEST( smLayerCallback::verifyIndex4ActiveTrans(
                          NULL ) /*aStatistics*/
                      != IDE_SUCCESS );
            break;

        case SMU_CHECK_DSKINDEX_INTEGRITY_LEVEL3:
        case SMU_CHECK_DSKINDEX_INTEGRITY_LEVEL1:

            IDE_TEST( smcCatalogTable::doAction4EachTBL(
                        NULL, /* aStatistics */
                        smcTable::verifyIndexIntegrity,
                        NULL /* aActionArg */)
                    != IDE_SUCCESS );
            break;

        default:
            IDE_ASSERT(0);
    }

    IDE_CALLBACK_SEND_SYM("  [SM] [END : CHECK DISK INDEX INTEGRITY]\n");

    IDE_EXCEPTION_CONT( skip_check_index_integrity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void smnManager::createIndexesParallel( void * aParam )
{
    smcTableHeader * sTable;

    sTable = (smcTableHeader*)aParam;

    IDE_ASSERT( smnManager::createIndexes( NULL, /* aStatistics */
                                           NULL, /* aTrans */
                                           sTable,
                                           ID_TRUE, /* aIsRestartBuild */
                                           ID_FALSE, /* aIsNeedValiation */
                                           ID_TRUE, /* aIsPersistent */
                                           NULL, /* smiSegAttr */
                                           NULL, /* smiSegStorageAttr * */
                                           NULL ) /* aThreadMgr */
                == IDE_SUCCESS );
}

/* ------------------------------------------------
 * Description:
 * Index Level Parallel Index Rebuilding시 시용되는 함수.
 * 이 함수는 다중 thread로 호출되어, 여러 Index를 동시에 build 한다.
 * ----------------------------------------------*/
void smnManager::buildIndexParallel( void * aParam )
{
    smnRebuildIndexInfo     * sRebuildInfo;
    smcTableHeader          * sTable;
    smnIndexHeader          * sIndex;
#ifdef DEBUG
    PDL_Time_Value            sTvStart;
    PDL_Time_Value            sTvEnd;
    SChar                     sStartTimeStr[128];
    SChar                     sEndTimeStr[128];

    sTvStart = idlOS::gettimeofday();
#endif


    sRebuildInfo = (smnRebuildIndexInfo *) aParam;

    IDE_ASSERT( sRebuildInfo != NULL );
    sTable = sRebuildInfo->mTable;
    sIndex = sRebuildInfo->mIndexCursor;
    
    IDE_ASSERT( sTable != NULL );
    IDE_ASSERT( sIndex != NULL );

    IDE_TEST_RAISE( buildIndex( NULL, /* aStatistics */
                                NULL, /* aTrans */
                                sTable,
                                sIndex,
                                ID_FALSE, /* aIsNeedValidation */
                                ID_TRUE,  /* aIsPersistent */
                                0, /* aParallelDegree */
                                0, /* aBuildFlag */
                                0  /* aTotalRecCnt */ )
                    != IDE_SUCCESS, ERR_BUILD_INDEX ); 
    
    /* createIndex에서 생성한 인덱스 생성 정보가 저장된 공간을 free한다. */
    ( void )iduMemMgr::free( sRebuildInfo );

#ifdef DEBUG
    sTvEnd = idlOS::gettimeofday();

    smuUtility::getTimeString( sTvStart.sec(), sizeof(sStartTimeStr), sStartTimeStr );
    smuUtility::getTimeString( sTvEnd.sec(), sizeof(sEndTimeStr), sEndTimeStr );

    ideLog::log( IDE_SM_0,
        "INDEX-ID %"ID_UINT32_FMT" INDEX-NAME %s START %s.%03"ID_UINT64_FMT
        " END %s.%03"ID_UINT64_FMT" TAKE %06"ID_UINT64_FMT,
        sIndex->mId, sIndex->mName,
        sStartTimeStr, sTvStart.msec() % 1000,
        sEndTimeStr, sTvEnd.msec() % 1000,
        ( sTvEnd - sTvStart ).msec() );
#endif

    return;

    IDE_EXCEPTION( ERR_BUILD_INDEX )
    {
        // BUG-47581 build memory index 에서 예외 발생 상황에 대한 정보 보강.
        // 실패한 Index에 대한 정보 출력
        ideLog::log( IDE_ERR_0,
                     "ERR-%05"ID_xPOINTER_FMT"(errno=%"ID_UINT32_FMT"): %s\n",
                     E_ERROR_CODE(ideGetErrorCode()),
                     ideGetSystemErrno(),
                     ideGetErrorMsg(ideGetErrorCode()) );

        ideLog::log( IDE_ERR_0,
                     "Index creation failed [smnManager::buildIndexParallel()]\n"
                     "IndexID: %"ID_UINT32_FMT", NAME: %s, "
                     "TableOID: %"ID_vULONG_FMT"\n",
                     sIndex->mId,
                     sIndex->mName,
                     sTable->mSelfOID );

        /* BUG-42169 rebuild index시 uniqueness가 깨진 인덱스가 발견될 경우
         * 해당 인덱스에 대한 trc로그를 남기고 그외 인덱스에 대해서는 작업을 계속 진행한다.*/
        if ( ideGetErrorCode() == smERR_ABORT_smnUniqueViolation )
        {    
            /* 인덱스 생성 실패 원인이 uniqueness 중복일 경우
             * 다른 인덱스에 대해서는 작업을 계속 진행한다.*/
            mIsInvalidUniqueness = ID_TRUE;  // BUG-47736
        }    
        else 
        {
            /* uniqueness외에 다른 이유로 인덱스 생성이 실패할 경우
             * 긴급 복구 모드가 아니라면 서버를 정지한다. */
            IDE_ASSERT( smuProperty::getEmergencyStartupPolicy() 
                        != SMR_RECOVERY_NORMAL );
        }    
    }    
    IDE_EXCEPTION_END;

    /* createIndex에서 생성한 인덱스 생성 정보 저장 공간을 free 한다. */
    ( void )iduMemMgr::free( sRebuildInfo );

    return; 
}

IDE_RC smnManager::rebuildIndexes()
{
    smcTableHeader    * sTable;
    smpSlotHeader     * sSlotHeader;
    SChar             * sNxtRowPtr;
    SChar             * sCurRowPtr;
    smSCN               sSCN;
    SChar               strBuffer[256];
    smuJobThreadMgr     sThreadMgr;
    smnRebuildMode      sRebuildMode;

    /* TASK-6006 : REBUILD_INDEX_PARALLEL_MODE 프로퍼티 값이
     * TRUE일 경우 INDEX 단위로 PARALLEL REBUILD를 수행하고
     * FALSE일 경우 TABLE 단위로 PARALLEL REBUILD를 수행한다. */
    sRebuildMode = smuProperty::getRebuildIndexParallelMode() == 0 
        ? SMN_INDEX_REBUILD_TABLE_MODE : SMN_INDEX_REBUILD_INDEX_MODE;
    if ( sRebuildMode == SMN_INDEX_REBUILD_INDEX_MODE )
    {
        IDE_TEST( smuJobThread::initialize(
                buildIndexParallel,
                smuProperty::getIndexRebuildParallelFactorAtStartup(),
                smuProperty::getIndexRebuildParallelFactorAtStartup() * 4,
                &sThreadMgr )
            != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( smuJobThread::initialize( 
                createIndexesParallel,
                smuProperty::getIndexRebuildParallelFactorAtStartup(),
                smuProperty::getIndexRebuildParallelFactorAtStartup() * 2,
                &sThreadMgr )
            != IDE_SUCCESS );
    }

    ideLog::log(IDE_SERVER_0, "     BEGIN INDEX BUILDING\n");

    idlOS::snprintf(strBuffer, 256,
                    "  [SM] Rebuilding Indices [Total Count:%"ID_UINT32_FMT"] ",
                    (UInt)smcTable::mTotalIndex);
    IDE_CALLBACK_SEND_SYM(strBuffer);

    /* ------------------------------------------------
     * [1] Catalog Table의 Index Rebuild
     * ----------------------------------------------*/
    IDE_TEST( smnManager::createIndexes(
                                NULL, /* aStatistics */
                                NULL, /* aTrans */
                                (smcTableHeader*)smmManager::m_catTableHeader,
                                ID_TRUE, /* aIsRestartBuild */
                                ID_FALSE, /* aIsNeedValidation */
                                ID_TRUE, /* aIsPersistent */
                                NULL, /* smiSegAttr */
                                NULL, /* smiSegStorageAttr* */
                                NULL ) /* aThreadMgr */
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * [2] Meta & Normal Table의 Index Rebuild
     * ----------------------------------------------*/
    IDE_TEST( smcRecord::nextOIDall((smcTableHeader *)smmManager::m_catTableHeader,
                                    NULL, &sNxtRowPtr )
              != IDE_SUCCESS );

    while ( sNxtRowPtr != NULL )
    {
        sSlotHeader = (smpSlotHeader *)sNxtRowPtr;
        sTable      = (smcTableHeader *)(sSlotHeader + 1);
        SM_GET_SCN( (smSCN*)&sSCN, &(sSlotHeader->mCreateSCN) );

        // disk table에 index rebuild는 skip
        // --> disk GC가 함.
        if ( SMI_TABLE_TYPE_IS_DISK( sTable ) == ID_FALSE )
        {
            // Do Selective Loaded Table Index Creation
            // Index Build를 해야하는 경우를 추려냄
            // => DROP/DISCARD/OFFLINE Tablespace에 속한
            //    Table이 아니어야 한다
            if ( sctTableSpaceMgr::hasState( sTable->mSpaceID,
                                             SCT_SS_SKIP_INDEXBUILD )
                 == ID_FALSE )
            {
                IDE_ASSERT( SMP_SLOT_IS_NOT_DROP( sSlotHeader ) );
                IDE_ASSERT( !SM_SCN_IS_DELETED( sSCN ) );

                if ( sRebuildMode == SMN_INDEX_REBUILD_INDEX_MODE )
                {
                    IDE_TEST( createIndexes( NULL, /* aStatistics */
                                             NULL, /* aTrans */
                                             sTable,
                                             ID_TRUE, /* aIsRestartBuild */
                                             ID_FALSE, /* aIsNeedValidation */
                                             ID_TRUE, /* aIsPersistent */
                                             NULL, /* aSegAttr */
                                             NULL, /* aSegStoAttr */
                                             &sThreadMgr )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( smuJobThread::addJob( &sThreadMgr, sTable ) 
                              != IDE_SUCCESS );
                }
            }
        }

        sCurRowPtr = sNxtRowPtr;

        IDE_TEST( smcRecord::nextOIDall(
                      (smcTableHeader *)smmManager::m_catTableHeader,
                      sCurRowPtr,
                      &sNxtRowPtr )
                  != IDE_SUCCESS );
    }

    IDE_TEST( smuJobThread::finalize( &sThreadMgr ) != IDE_SUCCESS );

    /* BUG-47736 Uniqueness 에 문제가 있으면 서버를 띄우면 안된다.
     * INDEX_REBUILD_AT_STARTUP == 0 에서는 문제가 있어도 띄워야 한다.
     * 1. 일반적인 memory index : Property 0 에서 index build 없으므로 uniqueness 변경 없음
     * 2. meta table index : Property 0 에서도 build index 하므로 uniqueness 변경 있음
     *                       uniqueness와 property 둘 다 확인해야 한다. */
    IDE_TEST(( smuProperty::getIndexRebuildAtStartup() == 1 ) &&
             ( mIsInvalidUniqueness == ID_TRUE ));

    IDE_CALLBACK_SEND_MSG(" [SUCCESS]");

    /* Remove Index File */
    IDE_TEST( removeAllIndexFile() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0, "     END INDEX BUILDING\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( mIsInvalidUniqueness == ID_TRUE )
    {
        // BUG-47736
        IDE_CALLBACK_SEND_MSG(" [FAILURE]");
        IDE_CALLBACK_SEND_MSG("                           : Unique Violation Error Occurs");
    }

    return IDE_FAILURE;
}

IDE_RC smnManager::destroyIndexes()
{

    smcTableHeader *sTable;
    smpSlotHeader  *sSlotHeader;
    SChar          *sNxtRowPtr;
    SChar          *sCurRowPtr;

    /* ------------------------------------------------
     * [2] Meta & Normal Table의 Index Drop
     * ----------------------------------------------*/
    IDE_TEST( smcRecord::nextOIDall((smcTableHeader *)smmManager::m_catTableHeader,
                                    NULL, &sNxtRowPtr )
              != IDE_SUCCESS );

    while ( sNxtRowPtr != NULL )
    {
        sSlotHeader = (smpSlotHeader *)sNxtRowPtr;
        sTable      = (smcTableHeader *)(sSlotHeader + 1);

        // Do Selective Loaded Table Index Creation
        if ( SMP_SLOT_IS_NOT_DROP( sSlotHeader ) ||
            // fix BUG7959
            // disk GC는 shutdown시 drop table pending 연산을
            // 못할수 있기 때문에 여기서 index runtime header
            // memory를 해제해야 한다.
            ( SMP_SLOT_IS_DROP( sSlotHeader ) &&
              SMI_TABLE_TYPE_IS_DISK( sTable ) == ID_TRUE ) )
        {
            IDE_TEST( dropIndexes( sTable ) != IDE_SUCCESS );
        }

        sCurRowPtr = sNxtRowPtr;

        IDE_TEST( smcRecord::nextOIDall( (smcTableHeader *)smmManager::m_catTableHeader,
                                         sCurRowPtr, &sNxtRowPtr )
                  != IDE_SUCCESS );
    }

    /* ------------------------------------------------
     * [1] Catalog Table의 Index Drop
     * ----------------------------------------------*/
    IDE_TEST( dropIndexes( (smcTableHeader *)smmManager::m_catTableHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smnManager::createIndexes(idvSQL*             aStatistics,
                                 void*               aTrans,
                                 smcTableHeader    * aTable,
                                 idBool              aIsRestartBuild,
                                 idBool              aIsNeedValidation,
                                 smiSegAttr        * aSegAttr,
                                 smiSegStorageAttr * aSegStoAttr )
{
    IDE_TEST( smnManager::createIndexes( aStatistics,
                                         aTrans,
                                         aTable,
                                         aIsRestartBuild,
                                         aIsNeedValidation,
                                         ID_FALSE, /* aIsPersistent */
                                         aSegAttr,
                                         aSegStoAttr,
                                         NULL ) /* aThreadMgr */
                  != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smnManager::createIndexes(idvSQL*             aStatistics,
                                 void*               aTrans,
                                 smcTableHeader    * aTable,
                                 idBool              aIsRestartBuild,
                                 idBool              aIsNeedValidation,
                                 idBool              aIsPersistent,
                                 smiSegAttr        * aSegAttr,
                                 smiSegStorageAttr * aSegStoAttr,
                                 smuJobThreadMgr   * aThreadMgr )
{
    smnIndexHeader      * sIndexHeader;
    UInt                  i;
    UInt                  sIndexCnt;

    sIndexCnt =  smcTable::getIndexCount(aTable);

    for ( i=0;i < sIndexCnt; i++)
    {
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex(aTable,i);
        
        /* BUG-37062 
         * disable된 index를 restart후 drop시 쓰레기 mHeader에 접근해 사망
         * IndexRuntimeHeader와 Module 정보를 초기화 해준다 */
        sIndexHeader->mModule = NULL;
        /* PROJ-2162 RestartRiskReduction
         * CreateIndex 및 initIndex실패할 경우 IndexRuntimeHeader가
         * 생성되지 않는다. 이때 mHeader가 쓰레기값일 수 있기 때문에
         * 초기화해준다. */
        sIndexHeader->mHeader = NULL;

        if ( (sIndexHeader->mFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_DISABLE)
        {
            continue;
        }

        if ( smnManager::createIndex( aStatistics,
                                      aTrans,
                                      aTable,
                                      sIndexHeader,
                                      aIsRestartBuild,
                                      aIsNeedValidation,
                                      aIsPersistent,
                                      aThreadMgr,
                                      aSegAttr,
                                      aSegStoAttr )
             != IDE_SUCCESS )
        {
            /* RestartRecovery시의 Build가 아니거나,
             * 긴급복구모드가 아니면 서버 종료시킴. */
            IDE_TEST( ( aIsRestartBuild == ID_FALSE ) ||
                      ( smuProperty::getEmergencyStartupPolicy() 
                        == SMR_RECOVERY_NORMAL ) );
            IDE_TEST( smrRecoveryMgr::refineFailureWithIndex( 
                        sIndexHeader->mTableOID,
                        sIndexHeader->mId )
                    != IDE_SUCCESS);
            IDE_CALLBACK_SEND_SYM("F");
        }
        else
        {
            if ( ( smuProperty::forceIndexPersistenceMode() != SMN_INDEX_PERSISTENCE_NOUSE ) &&
                ( ( sIndexHeader->mFlag & SMI_INDEX_PERSISTENT_MASK ) ==
                  SMI_INDEX_PERSISTENT_ENABLE ) )
            {
                IDE_CALLBACK_SEND_SYM("P");
            }
            else
            {
                IDE_CALLBACK_SEND_SYM(".");
            }
        }
    }//for

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnManager::dropIndexes( smcTableHeader * aTable )
{
    smnIndexHeader* sIndexHeader;
    UInt            i;
    UInt            sIndexCnt;

    sIndexCnt =  smcTable::getIndexCount(aTable);

    for ( i=0;i < sIndexCnt; i++)
    {
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex(aTable,i);
        IDE_TEST( smnManager::dropIndex( aTable, sIndexHeader )
                  != IDE_SUCCESS );
    }//for

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description: 주어진 table header에 포함된 모든 index를 비활성화 한다.
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 *
 * aStatistics  - [IN] idvSQL
 * aTrans       - [IN] smxTrans
 * aTableHeader - [IN] 모든 index를 비활성화 할 대상 table header
 *********************************************************************/
IDE_RC smnManager::disableAllIndex( idvSQL          * aStatistics,
                                    void            * aTrans,
                                    smcTableHeader  * aTableHeader )
{
    smnIndexHeader  * sIndexHeader;
    scGRID          * sIndexSegGRID;
    UInt              sIndexIdx;
    UInt              sIndexCnt = smcTable::getIndexCount( aTableHeader );
    UInt              sTableType = aTableHeader->mFlag & SMI_TABLE_TYPE_MASK;


    for ( sIndexIdx = 0; sIndexIdx < sIndexCnt; sIndexIdx++ )
    {
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex( aTableHeader,
                                                                 sIndexIdx );

        if ( sTableType == SMI_TABLE_DISK )
        {
            sIndexSegGRID = getIndexSegGRIDPtr(sIndexHeader);

            if ( SC_GRID_IS_NOT_NULL(*sIndexSegGRID) )
            {
                IDE_TEST( sdpSegment::freeIndexSeg4Entry(
                                            aStatistics,
                                            SC_MAKE_SPACE( *sIndexSegGRID ),
                                            aTrans,
                                            sIndexHeader->mSelfOID,
                                            SDR_MTX_LOGGING)
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }

            IDU_FIT_POINT( "1.BUG-33803@smnManager::disableAllIndex" );
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( dropIndex( aTableHeader,
                             sIndexHeader )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description: 대상 테이블의 모든 index를 활성화 한다.
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 *
 *
 * aStatistics      - [IN] idvSQL
 * aTrans           - [IN] smxTrans
 * aTableHeader     - [IN] 모든 index를 활성화할 대상 table의 header
 ******************************************************************************/
IDE_RC smnManager::enableAllIndex( idvSQL          * aStatistics,
                                   void            * aTrans,
                                   smcTableHeader  * aTableHeader )
{
    smnIndexHeader    * sIndexHeader;
    ULong               sTotalRecCount = 0;
    UInt                sBuildFlag = SMI_INDEX_BUILD_DEFAULT;
    UInt                sIndexIdx;
    UInt                sIndexCnt = smcTable::getIndexCount(aTableHeader);
    UInt                sTableType = aTableHeader->mFlag & SMI_TABLE_TYPE_MASK;

    for ( sIndexIdx = 0 ; sIndexIdx < sIndexCnt; sIndexIdx++ )
    {
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex( aTableHeader,
                                                                 sIndexIdx );

        IDE_ERROR( SC_GRID_IS_NULL(sIndexHeader->mIndexSegDesc) );

        /* PROJ-2162 RestartRiskReduction
         * CreateIndex 및 initIndex실패할 경우 IndexRuntimeHeader가
         * 생성되지 않는다. 이때 mHeader가 쓰레기값일 수 있기 때문에
         * 초기화해준다. */
        sIndexHeader->mHeader = NULL;

        if ( sTableType == SMI_TABLE_DISK )
        {
            sBuildFlag = SMI_INDEX_BUILD_DISK_DEFAULT;

            // create index segment
            IDE_TEST( sdpSegment::allocIndexSeg4Entry(
                                                aStatistics,
                                                aTrans,
                                                aTableHeader->mSpaceID,
                                                aTableHeader->mSelfOID,
                                                sIndexHeader->mSelfOID,
                                                sIndexHeader->mId,
                                                sIndexHeader->mType,
                                                sBuildFlag,
                                                SDR_MTX_LOGGING,
                                                &sIndexHeader->mSegAttr,
                                                &sIndexHeader->mSegStorageAttr )
                      != IDE_SUCCESS );

            IDE_TEST( smcTable::getRecordCount( aTableHeader, &sTotalRecCount )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( initIndex( aStatistics,
                             aTableHeader,
                             sIndexHeader,
                             &sIndexHeader->mSegAttr,
                             &sIndexHeader->mSegStorageAttr,
                             NULL,  /* aRebuildIndexHeader */
                             0)     /* SmoNo */
                  != IDE_SUCCESS);

        /* all index enable 도중 abort 발생 시, 이미 create 끝난 index들을
         * 다시 drop 해주도록 NTA log 기록 */
        IDE_TEST( smrLogMgr::writeNTALogRec(
                              aStatistics,
                              aTrans,
                              smLayerCallback::getTransLstUndoNxtLSNPtr( aTrans ),
                              aTableHeader->mSpaceID,
                              SMR_OP_INIT_INDEX,
                              aTableHeader->mSelfOID,
                              sIndexHeader->mSelfOID )
                  != IDE_SUCCESS );

        IDU_FIT_POINT( "smnManager::enableAllIndex::buildIndex:sIndexHeader1" );

        IDE_TEST( buildIndex( aStatistics,
                              aTrans,
                              aTableHeader,
                              sIndexHeader,
                              ID_TRUE,  /* aIsNeedValidation */
                              ID_TRUE,  /* aIsPersistent */
                              0,        /* aParallelDegree */
                              sBuildFlag,
                              sTotalRecCount )
                  != IDE_SUCCESS);

        IDU_FIT_POINT( "smnManager::enableAllIndex::buildIndex:sIndexHeader2" );
    } //for

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smnManager::indexOperation( idvSQL*, void*, void*, void*, smSCN,
                                   SChar*, SChar*, idBool, smSCN, void*, SChar**, ULong, idBool )
{
    // PR-14912
    // initIndexHeader에서 기본으로 이 함수를 mInsert 에 assign.
    // 그 이후에 index runtime header가 제대로
    // 초기화되지 않는다면 cursor close시에 해당
    // 함수로 진입하게 된다. assert 처리!!
    IDE_ASSERT( 0 );

    return IDE_SUCCESS;

}

/* PROJ-1594 Volatile TBS */
IDE_RC smnManager::lockVolRow( smiIterator* aIterator)
{
    smpSlotHeader* sVolRow;

    smSCN  sSCN;
    smTID  sTID;

    updatedVolRow( aIterator );

    sVolRow = (smpSlotHeader*)(aIterator->curRecPtr);

    IDE_ASSERT( sVolRow != NULL );

    SMX_GET_SCN_AND_TID( sVolRow->mCreateSCN, sSCN, sTID );

    if ( ( sTID != aIterator->tid ) || ( SM_SCN_IS_NOT_INFINITE( sSCN ) ) )
    {
        IDE_TEST( svcRecord::lockRow( aIterator->trans,
                                      aIterator->SCN,
                                      (smcTableHeader*)aIterator->table,
                                      aIterator->curRecPtr,
                                      aIterator->properties->mLockWaitMicroSec,
                                      aIterator->mStatement->isForbiddenToRetry() )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnManager::lockMemRow( smiIterator* aIterator)
{
    smpSlotHeader* sRow;

    smSCN sRowSCN;
    smTID sRowTID;
    smSCN sNxtSCN;
    smTID sNxtTID;

    updatedMemRow( aIterator );

    sRow = (smpSlotHeader*)(aIterator->curRecPtr);

    IDE_ASSERT( sRow != NULL );

    SMX_GET_SCN_AND_TID( sRow->mCreateSCN, sRowSCN, sRowTID );
    SMX_GET_SCN_AND_TID( sRow->mLimitSCN,  sNxtSCN, sNxtTID );

    if ( ( SM_SCN_IS_LOCK_ROW( sNxtSCN ) ) &&
         ( sNxtTID == aIterator->tid ) )
    {
        /* do nothing */
    }
    else
    {
        if ( ( sRowTID != aIterator->tid ) || SM_SCN_IS_NOT_INFINITE(sRowSCN) )
        {
            IDE_TEST( smcRecord::lockRow( aIterator->trans,
                                          aIterator->SCN,
                                          (smcTableHeader*)aIterator->table,
                                          aIterator->curRecPtr,
                                          aIterator->properties->mLockWaitMicroSec,
                                          aIterator->mStatement->isForbiddenToRetry() )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*Lock row연산을 수행하기전에 호출하는 함수. lock을 걸고자 하는 row의 마지막 버전을 찾아서
 * Iterater에 세팅한다. lock row연산은 가장 마지막 버전의 mNext에 세팅된다.
 */
void smnManager::updatedMemRow( smiIterator* aIterator )
{
    smpSlotHeader* sRow;
    scSpaceID      sSpaceID = ((smcTableHeader*)aIterator->table)->mSpaceID;
    ULong          sNextOID;
    smSCN          sRowSCN;
    smTID          sRowTID;

    sRow = (smpSlotHeader*)(aIterator->curRecPtr);

    /*현재 row가 마지막 version이 아닐 조건은 mNext가 Lock또는 DeleteSCN이 설정되지 않았고,
     * mNext에 OID가 설정되어있는데, 이것이 NULL이 아닐때이다.*/
    sNextOID = SMP_SLOT_GET_NEXT_OID( sRow );
    if ( SM_IS_VALID_OID( sNextOID ) )
    {
        IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                           sNextOID,
                                           (void**)&sRow )
                    == IDE_SUCCESS );

        SMX_GET_SCN_AND_TID( sRow->mCreateSCN, sRowSCN, sRowTID );

        /*다른 트랜잭션에 의해서 다음 버전이 생겼다면 더이상 살펴보지 않는다. 왜냐하면,
         * 자신이 연산을 하기에 앞서 먼저 연산을 한 트랜잭션이 있으므로, 현재 연산은
         * 더이상 진행되지 않을것이다.*/
        if ( ( sRowTID == aIterator->tid ) && ( SM_SCN_IS_INFINITE(sRowSCN) ) ) 
        {
            aIterator->curRecPtr = (SChar*)sRow;

            sNextOID = SMP_SLOT_GET_NEXT_OID( sRow );

            while ( SM_IS_VALID_OID( sNextOID ) )
            {
                IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                                   sNextOID,
                                                   (void**)&aIterator->curRecPtr )
                            == IDE_SUCCESS );
                sRow = (smpSlotHeader*)aIterator->curRecPtr;

                sNextOID = SMP_SLOT_GET_NEXT_OID( sRow );
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
}

/*Lock row연산을 수행하기전에 호출하는 함수. lock을 걸고자 하는 row의 마지막 버전을 찾아서
 * Iterater에 세팅한다. lock row연산은 가장 마지막 버전의 mNext에 세팅된다.
 */
void smnManager::updatedVolRow( smiIterator* aIterator )
{
    smpSlotHeader* sRow;
    scSpaceID      sSpaceID = ((smcTableHeader*)aIterator->table)->mSpaceID;
    ULong          sNextOID;
    smSCN          sRowSCN;
    smTID          sRowTID;

    sRow = (smpSlotHeader*)(aIterator->curRecPtr);

    /*현재 row가 마지막 version이 아닐 조건은 mNext가 Lock또는 DeleteSCN이 설정되지 않았고,
     * mNext에 OID가 설정되어있는데, 이것이 NULL이 아닐때이다.*/
    sNextOID = SMP_SLOT_GET_NEXT_OID( sRow );
    if ( SM_IS_VALID_OID( sNextOID ) )
    {
        IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                           sNextOID,
                                           (void**)&sRow )
                    == IDE_SUCCESS );

        SMX_GET_SCN_AND_TID( sRow->mCreateSCN, sRowSCN, sRowTID );

        /*다른 트랜잭션에 의해서 다음 버전이 생겼다면 더이상 살펴보지 않는다. 왜냐하면,
         * 자신이 연산을 하기에 앞서 먼저 연산을 한 트랜잭션이 있으므로, 현재 연산은
         * 더이상 진행되지 않을것이다.*/
        if ( ( sRowTID == aIterator->tid ) && ( SM_SCN_IS_INFINITE(sRowSCN) ) )
        {
            aIterator->curRecPtr = (SChar*)sRow;

            sNextOID = SMP_SLOT_GET_NEXT_OID( sRow );

            while ( SM_IS_VALID_OID( sNextOID ) )
            {
                IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                                   sNextOID,
                                                   (void**)&aIterator->curRecPtr )
                            == IDE_SUCCESS );
                sRow = (smpSlotHeader*)aIterator->curRecPtr;

                sNextOID = SMP_SLOT_GET_NEXT_OID( sRow );
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
}

void smnManager::updatedRow(smiIterator *aIterator)
{
    if ( SMI_TABLE_TYPE_IS_VOLATILE( ((smcTableHeader*)aIterator->table) ) == ID_TRUE )
    {
        updatedVolRow(aIterator);
    }
    else
    {
        updatedMemRow(aIterator);
    }
}

IDE_RC smnManager::lockRow(smiIterator *aIterator)
{
    if ( SMI_TABLE_TYPE_IS_VOLATILE( ((smcTableHeader*)aIterator->table) ) == ID_TRUE )
    {
        return lockVolRow(aIterator);
    }
    else
    {
        return lockMemRow(aIterator);
    }
}

IDE_RC smnManager::prepareRebuildIndexs(smcTableHeader*  aTable,
                                        smnIndexHeader** aRebuildIndexList)
{

    smnIndexHeader* sIndexCursor;
    SInt            sIndexNo;
    smnIndexHeader** sIndexHeaderPtr;
    UInt            i;
    UInt            sIndexCnt;

    sIndexCnt =  smcTable::getIndexCount( aTable );
    idlOS::memset( aRebuildIndexList,
                   0x00,
                   ID_SIZEOF(smnIndexHeader*) * SMC_INDEX_TYPE_COUNT );

    for ( i = 0 ; i < sIndexCnt ; i++ )
    {

        sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( aTable, i );

        sIndexNo = sIndexCursor->mType;

        if ( (sIndexCursor->mFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_DISABLE )
        {
            continue;
        }

        if ( ( ( sIndexCursor->mFlag & SMI_INDEX_PERSISTENT_MASK) == SMI_INDEX_PERSISTENT_ENABLE ) &&
             ( smrRecoveryMgr::isABShutDown() == ID_FALSE ) &&
             ( smuProperty::forceIndexPersistenceMode() != SMN_INDEX_PERSISTENCE_NOUSE ) )
        {
            sIndexHeaderPtr = NULL;
        }
        else
        {
            sIndexHeaderPtr = aRebuildIndexList + sIndexNo;
        }

        //Alloc and Init Index Header
        IDE_TEST( smnManager::initIndex( NULL,
                                         aTable,
                                         (void *)sIndexCursor,
                                         NULL, /* smiSegAttr */
                                         NULL, /* smiSegStoAttr * */
                                         (void **)sIndexHeaderPtr,
                                         0 )   /* SmoNo */
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

SInt smnManager::getIndexModuleNo(UInt aTableType, UInt aIndexType)
{

    IDE_DASSERT( SMN_GET_BASE_TABLE_TYPE_ID(aTableType) <
                 SMI_INDEXIBLE_TABLE_TYPES );
    IDE_DASSERT( aIndexType < SMI_INDEXTYPE_COUNT );

    return SMN_MAKE_INDEX_MODULE_ID(aTableType, aIndexType );

}

void * smnManager::getIndexModule(UInt aTableType, UInt aIndexType)
{

    IDE_DASSERT( SMN_GET_BASE_TABLE_TYPE_ID(aTableType) <
                 SMI_INDEXIBLE_TABLE_TYPES );
    IDE_DASSERT( aIndexType < SMI_INDEXTYPE_COUNT );

    return gSmnAllIndex[aIndexType]->mModule[SMN_GET_BASE_TABLE_TYPE_ID(aTableType)];

}


UInt * smnManager::getFlagPtrOfIndexHeader( void   * a_pIndexHeader )
{

    return &(((smnIndexHeader*)a_pIndexHeader)->mFlag);

}

smOID smnManager::getTableOIDOfIndexHeader( void * a_pIndexHeader )
{

    return (((smnIndexHeader*)a_pIndexHeader)->mTableOID);

}

UInt smnManager::getIndexIDOfIndexHeader( void * a_pIndexHeader )
{

    return (((smnIndexHeader*)a_pIndexHeader)->mId);

}

IDE_RC smnManager::indexInsertWithoutUniqueCheck( void   * a_pTrans,
                                                  void   * aTable,
                                                  void   * a_pIndexHeader,
                                                  SChar  * a_pRow,
                                                  SChar  * a_pNull )
{
    return (((smnIndexHeader*)a_pIndexHeader)->mInsert( NULL,         /* idvSQL* */
                                                        a_pTrans,
                                                        aTable,
                                                        a_pIndexHeader,
                                                        SM_SCN_INIT,  /* aInfiniteSCN = No Use */ 
                                                        a_pRow,
                                                        a_pNull,
                                                        ID_FALSE,      /* aUniqueCheck = ID_FALSE */
                                                        SM_SCN_INIT,   /* statement SCN = No Use */
                                                        NULL,          /* aRowSID */
                                                        NULL,          /* aExistUniqueRow */
                                                        ID_ULONG_MAX,  /* aInsertWaitTime */ 
                                                        ID_FALSE ));   /* aForbiddenToRetry */
}

/*
 * smcRecordUpdate.cpp 에서만 호출
 * Memory Index에 대한 Function
 */
IDE_RC smnManager::indexDeleteFunc( void   * a_pIndexHeader,
                                    SChar  * a_pRow,
                                    idBool   aIgnoreNotFoundKey,
                                    idBool * aExistKey )
{

    smSCN      sUnUseSCN;

    SM_INIT_SCN( &sUnUseSCN );

    return ((smnIndexHeader*)a_pIndexHeader)->mModule->mFreeSlot(
                                                     a_pIndexHeader,
                                                     a_pRow,
                                                     aIgnoreNotFoundKey,
                                                     aExistKey );
}

void smnManager::initIndexHeader( void                * aIndexHeader,
                                  smOID                 aTableSelfOID,
                                  SChar               * aName,
                                  UInt                  aID,
                                  UInt                  aType,
                                  UInt                  aFlag,
                                  const smiColumnList * aColumns,
                                  smiSegAttr          * aSegAttr,
                                  smiSegStorageAttr   * aSegStoAttr,
                                  ULong                 aDirectKeyMaxSize )
{

    smiColumn       * sColumn;
    smiColumnList   * sColumns;
    smnIndexHeader  * sIndexHeader;

    sIndexHeader = (smnIndexHeader*)aIndexHeader;

    sIndexHeader->mTableOID     = aTableSelfOID;
    // smpVarPageList::allocSlot 이후에 설정해야 함.
    sIndexHeader->mSelfOID      = SM_NULL_OID;
    sIndexHeader->mModule           = NULL;
    sIndexHeader->mColumnCount      = 0;
    idlOS::strncpy( sIndexHeader->mName, aName, SMN_MAX_INDEX_NAME_SIZE );

    /* BUG-42266 remove valgrind warning */
    sIndexHeader->mName[SMN_MAX_INDEX_NAME_SIZE] = '\0';

    sIndexHeader->mId               = aID;
    sIndexHeader->mType             = aType;
    sIndexHeader->mFlag             = aFlag;
    sIndexHeader->mDropFlag         = SMN_INDEX_DROP_FALSE;
    sIndexHeader->mHeader           = NULL;
    sIndexHeader->mInsert           = (smnInsertFunc)smnManager::indexOperation;
    SC_MAKE_NULL_GRID( sIndexHeader->mIndexSegDesc );
    idlOS::memset( &sIndexHeader->mStat, 0, ID_SIZEOF( smiIndexStat ) );

    //BUG-47885: qp에서 index 순서를 올바로 볼 수 있도록 ID 설정
    sIndexHeader->mStat.mId         = aID;
    sIndexHeader->mStat.mDummy      = 0;

    sColumns = (smiColumnList *)aColumns;

    while ( sColumns != NULL )
    {
        /* BUGBUG : column 갯수의 validation이 필요함 */
        /* BUG-27516 [5.3.3 release] Klocwork SM (5)
         * Column Count는 0 ~ SMI_MAX_IDX_COLUMNS-1사이의 값만
         * 소유해야 합니다.
         */
        IDE_ASSERT( sIndexHeader->mColumnCount < SMI_MAX_IDX_COLUMNS );

        sColumn = (smiColumn*)sColumns->column;

        sIndexHeader->mColumns[sIndexHeader->mColumnCount] =
            sColumn->id;

        sIndexHeader->mColumnFlags[sIndexHeader->mColumnCount] =
            (sColumn->flag & SMI_COLUMN_ORDER_MASK);
        // PROJ-1705
        sIndexHeader->mColumnFlags[sIndexHeader->mColumnCount] |=
            (sColumn->flag & SMI_COLUMN_TYPE_MASK);
        sIndexHeader->mColumnFlags[sIndexHeader->mColumnCount] |=
            (sColumn->flag & SMI_COLUMN_STORAGE_MASK);
        sIndexHeader->mColumnFlags[sIndexHeader->mColumnCount] |=
            (sColumn->flag & SMI_COLUMN_LENGTH_TYPE_MASK);

        // PROJ-2429 Dictionary based data compress for on-disk DB
        if ( sctTableSpaceMgr::isDiskTableSpace( sColumn->colSpace ) == ID_TRUE )
        {
            sIndexHeader->mColumnFlags[sIndexHeader->mColumnCount] |=
                                    (sColumn->flag & SMI_COLUMN_COMPRESSION_MASK);
        }
        else
        {
            IDE_ASSERT( ( sctTableSpaceMgr::isMemTableSpace( sColumn->colSpace ) == ID_TRUE ) ||
                        ( sctTableSpaceMgr::isVolatileTableSpace( sColumn->colSpace ) == ID_TRUE ) ); 
            // nothing to do
        }

        sIndexHeader->mColumnOffsets[sIndexHeader->mColumnCount] = sColumn->offset;
        sColumns = sColumns->next;
        sIndexHeader->mColumnCount++;

    }

    sIndexHeader->mSegAttr = *aSegAttr;
    sIndexHeader->mSegStorageAttr = *aSegStoAttr;

    /* PROJ-2433
     * aDirectKEyMaxSize가 0이면, property __MEM_BTREE_DEFAULT_MAX_KEY_SIZE 으로 초기화 */
    if ( ( aFlag & SMI_INDEX_DIRECTKEY_MASK ) == SMI_INDEX_DIRECTKEY_TRUE )
    {
        if ( aDirectKeyMaxSize == 0 )
        {
            sIndexHeader->mMaxKeySize = SMNB_DEFAULT_MAX_KEY_SIZE();
        }
        else
        {
            sIndexHeader->mMaxKeySize = aDirectKeyMaxSize;
        }
    }
    else
    {
        sIndexHeader->mMaxKeySize = 0;
    }
}

IDE_RC smnManager::initIndexMetaPage( UChar            * aMetaPtr,
                                      UInt               aType,
                                      UInt               aBuildFlag,
                                      sdrMtx           * aMtx )
{
    SInt             sDiskTableId;
    smnIndexModule * sIndexModule;

    sDiskTableId = SMN_GET_BASE_TABLE_TYPE_ID(SMI_TABLE_DISK);

    sIndexModule = gSmnAllIndex[aType]->mModule[sDiskTableId];

    IDE_TEST( sIndexModule->mInitMeta( aMetaPtr,
                                       aBuildFlag,
                                       (void*)aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt   smnManager::getColumnCountOfIndexHeader ( void *a_pIndexHeader )
{

    return ((smnIndexHeader *)a_pIndexHeader)->mColumnCount;

}

UInt*  smnManager::getColumnIDPtrOfIndexHeader ( void *a_pIndexHeader,
                                                 UInt a_nIndex )
{

    return &( ((smnIndexHeader *)a_pIndexHeader)->mColumns[a_nIndex] );

}

UInt  smnManager::getFlagOfIndexHeader ( void *a_pIndexHeader )
{

    return (((smnIndexHeader *)a_pIndexHeader)->mFlag);

}

void  smnManager::setFlagOfIndexHeader ( void *a_pIndexHeader, UInt a_Flag )
{

   ((smnIndexHeader *)a_pIndexHeader)->mFlag = a_Flag;

}

idBool smnManager::getIsConsistentOfIndexHeader ( void *aIndexHeader )
{
    smnIndexHeader   * sHeader;
    smnRuntimeHeader * sRuntimeHeader;
    idBool             sRet = ID_FALSE;

    IDE_ASSERT( aIndexHeader != NULL );

    sHeader        = (smnIndexHeader*)aIndexHeader;
    sRuntimeHeader = (smnRuntimeHeader*)sHeader->mHeader;

    /* Redo 시점에서는 Index Consistent정보를 알 수 없다. 무조건 Valid하다고
     * 판단한다. */
    if ( smrRecoveryMgr::isRestart() == ID_TRUE )
    {
        sRet = ID_TRUE;
    }
    else
    {
        /* BuildIndex가 실패하면, runtimeHeader가 없다. 이 상태는
         * inconsistent하다고 간주한다. */
        if ( sRuntimeHeader != NULL )
        {
            sRuntimeHeader =
                (smnRuntimeHeader*)((smnIndexHeader*)aIndexHeader)->mHeader;

            sRet = sRuntimeHeader->mIsConsistent;
        }
        else
        {
            sRet = ID_FALSE;
        }
    }

    return sRet;
}

void  smnManager::setIsConsistentOfIndexHeader ( void   * aIndexHeader, 
                                                 idBool   aFlag )
{
    smcTableHeader   * sTableHeader;
    smnIndexModule   * sModule;
    smnIndexHeader   * sHeader;
    smnRuntimeHeader * sRuntimeHeader;
    UInt               sIndexTypeID;
    UInt               sTableTypeID;
    SChar             * sOutBuffer4Dump;

    IDE_ASSERT( aIndexHeader != NULL );

    sHeader        = (smnIndexHeader*)aIndexHeader;
    sRuntimeHeader = (smnRuntimeHeader*)sHeader->mHeader;

    IDE_ASSERT( smcTable::getTableHeaderFromOID( 
                                            sHeader->mTableOID,
                                            (void**)&sTableHeader )
                == IDE_SUCCESS );

    sIndexTypeID = sHeader->mType;
    sTableTypeID = SMN_GET_BASE_TABLE_TYPE_ID(sTableHeader->mFlag);

    /* Inconsistent해지는 Index 정보 dump */
    if ( iduMemMgr::calloc( IDU_MEM_SM_SMN, 
                            1,
                            ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                            (void**)&sOutBuffer4Dump )
         == IDE_SUCCESS )
    {
        if ( smnManager::dumpCommonHeader( sHeader,
                                           sOutBuffer4Dump,
                                           IDE_DUMP_DEST_LIMIT )
             == IDE_SUCCESS )
        {
            ideLog::log( IDE_DUMP_0, "%s\n", sOutBuffer4Dump );
        }
        else
        {
            /* nothing to do */
        }
        (void) iduMemMgr::free( sOutBuffer4Dump );
    }
    else
    {
        /* nothing to do */
    }

    IDE_DASSERT( sTableHeader->mSelfOID == sHeader->mTableOID );

    /* DRDBIndex Refine전에는 Index에 Inconsistent 설정을 하지 않는다. */
    if ( ( smrRecoveryMgr::isRestart() == ID_FALSE ) ||
         ( smrRecoveryMgr::isRefineDRDBIdx() == ID_TRUE ) )
    {
        /* BuildIndex가 실패하면, runtimeHeader가 없다. 이 상태는
         * inconsistent하다고 간주한다. */
        if ( sRuntimeHeader != NULL )
        {
            sModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];
            if ( sModule->mSetIndexConsistency != NULL )
            {
                sModule->mSetIndexConsistency( sHeader,
                                               aFlag );
            } 
            else
            {
                /* nothing to do */
            }
            sRuntimeHeader->mIsConsistent = aFlag;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
}


IDE_RC smnManager::deleteRowFromIndexForSVC( SChar          * aRow,
                                             smcTableHeader * aHeader,
                                             ULong            aModifyIdxBit )
{
    return deleteRowFromIndex( aRow,
                               aHeader,
                               aModifyIdxBit );
}

// smcTable::updateInplace에서 호출. memory table에서만 사용...
IDE_RC smnManager::deleteRowFromIndex( SChar          * a_pRow,
                                       smcTableHeader * a_header,
                                       ULong            aModifyIdxBit )
{

    smnIndexHeader        * sIndexCursor;
    smnIndexHeader        * sDropBegin = NULL;
    smnIndexHeader        * sDropFence = NULL;
    UInt                    i;
    UInt                    sIndexCnt;
    ULong                   sBitMask;
    smSCN                   sUnUseSCN;
    idBool                  sIsExistFreeKey;

    sIndexCnt =  smcTable::getIndexCount(a_header);

    SM_INIT_SCN(&sUnUseSCN);

    sBitMask   = ((ULong)1 << 63);
    sDropBegin = (smnIndexHeader*)(a_header->mDropIndexLst);
    sDropFence = sDropBegin + sizeof(smnIndexHeader)*a_header->mDropIndex;

    for ( i = 0 ; i < sIndexCnt ; i++ )
    {
        if (( aModifyIdxBit & sBitMask ) != 0 )
        {
            sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex(a_header,i);

            if ( (sIndexCursor->mFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE )
            {
                IDE_TEST( sIndexCursor->mModule->mFreeSlot(
                              sIndexCursor,
                              a_pRow,
                              ID_FALSE, /*aIgnoreNotFoundKey*/
                              &sIsExistFreeKey )
                          != IDE_SUCCESS );

                IDE_ERROR_RAISE( sIsExistFreeKey == ID_TRUE, ERR_CORRUPTED_INDEX );
            }
        }
        sBitMask = sBitMask >> 1;
    }//for i

    for ( sIndexCursor  = sDropBegin;
          sIndexCursor != sDropFence;
          sIndexCursor++ )
    {
        IDE_TEST( sIndexCursor->mModule->mFreeSlot( sIndexCursor,
                                                    a_pRow,
                                                    ID_FALSE, /*aIgnoreNotFoundKey*/
                                                    &sIsExistFreeKey )
                  != IDE_SUCCESS );

        IDE_ERROR_RAISE( sIsExistFreeKey == ID_TRUE, ERR_CORRUPTED_INDEX );
    } // sIndexCursor

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Slot Position" );
    }
    IDE_EXCEPTION_END;

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        smnManager::setIsConsistentOfIndexHeader( sIndexCursor, ID_FALSE );
    }

    return IDE_FAILURE;

}

idBool smnManager::isNullModuleOfIndexHeader(void *a_pIndexHeader)
{

    if ( ((smnIndexHeader *)a_pIndexHeader)->mModule == NULL )
    {
        return ID_TRUE;
    }

    return ID_FALSE;

}

scSpaceID smnManager::getIndexSpaceID( void * aIndexHeader )
{
    return SC_MAKE_SPACE(((smnIndexHeader*)aIndexHeader)->mIndexSegDesc);
}

UShort smnManager::getIndexDropFlag( void * aIndexHeader )
{
    return ((smnIndexHeader*)aIndexHeader)->mDropFlag;
}

smiSegStorageAttr* smnManager::getIndexSegStoAttrPtr( void * aIndexHeader )
{
    return (&((smnIndexHeader*)aIndexHeader)->mSegStorageAttr);
}

smiSegAttr* smnManager::getIndexSegAttrPtr( void * aIndexHeader )
{
    return (&((smnIndexHeader*)aIndexHeader)->mSegAttr);
}

void smnManager::setIndexSegStoAttrPtr( void              * aIndexHeader,
                                        smiSegStorageAttr   aSegStoAttr )
{
    sdpSegmentDesc * sSegDescPtr;

    IDE_ASSERT( aIndexHeader != NULL );

    ((smnIndexHeader*)aIndexHeader)->mSegStorageAttr = aSegStoAttr;
    sSegDescPtr = (sdpSegmentDesc*)getSegDescByIdxPtr( aIndexHeader );
    sSegDescPtr->mSegHandle.mSegStoAttr = aSegStoAttr;
}

void smnManager::setIndexSegAttrPtr( void       * aIndexHeader,
                                     smiSegAttr   aSegAttr )
{
    sdpSegmentDesc * sSegDescPtr;

    IDE_ASSERT( aIndexHeader != NULL );

    ((smnIndexHeader*)aIndexHeader)->mSegAttr = aSegAttr;
    sSegDescPtr = (sdpSegmentDesc*)getSegDescByIdxPtr( aIndexHeader );
    sSegDescPtr->mSegHandle.mSegAttr = aSegAttr;
}

UInt * smnManager::getIndexFlagPtr( void * aIndexHeader )
{

    return (&((smnIndexHeader*)aIndexHeader)->mFlag);

}


SChar*  smnManager::getNameOfIndexHeader(void *aIndexHeader)
{
    return (SChar*) ((smnIndexHeader*)aIndexHeader)->mName;
}

void    smnManager::setNameOfIndexHeader(void *aIndexHeader, SChar *aName)
{
    smnIndexHeader  *   sIndexHeader = NULL;

    sIndexHeader = (smnIndexHeader*)aIndexHeader;

    idlOS::strncpy( sIndexHeader->mName, aName, SMN_MAX_INDEX_NAME_SIZE );

    /* BUG-42266 remove valgrind warning */
    sIndexHeader->mName[SMN_MAX_INDEX_NAME_SIZE] = '\0';
}

UInt    smnManager::getIndexNameOffset()
{
    return offsetof(smnIndexHeader,mName);
}


idBool smnManager::checkIdxIntersectCols( void* aIndexHeader,
                                          UInt   aColumnCount,
                                          UInt * aColumns )
{

    SInt             i;
    SInt             j;
    smnIndexHeader * sIndex = (smnIndexHeader*)aIndexHeader;

    for ( i = 0 ; i < (SInt)sIndex->mColumnCount ; i++ )
    {
        for ( j = 0; j < (SInt)aColumnCount; j++ )
        {
            if ( sIndex->mColumns[i] == aColumns[j] )
            {
                return ID_TRUE;
            }
            else
            {
                /* nothing to do */
            }
        }
    }

    return ID_FALSE;

}

void smnManager::setInitIndexPtr( void* aIndexHeader )
{

    smnIndexHeader * sIndex = (smnIndexHeader*)aIndexHeader;

    sIndex->mHeader = NULL;

    return;

}

/***********************************************
 * BUG-24403
 *
 * [IN]  aStatistics,
 * [IN]  aTable,
 * [OUT] aMaxSmoNo
 *
***********************************************/
void smnManager::getMaxSmoNoOfAllIndexes( smcTableHeader * aTable,
                                          ULong          * aMaxSmoNo )
{
    smnIndexHeader* sIndexHeader;
    UInt            i;
    UInt            sIndexCnt;
    ULong           sCurSmoNo = 0;

    IDE_DASSERT( aTable != NULL );

    sIndexCnt =  smcTable::getIndexCount( aTable );

    for ( i = 0 ; i < sIndexCnt ; i++ )
    {
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex( aTable, i );
        getIndexSmoNo( sIndexHeader,
                       &sCurSmoNo );

        if ( sCurSmoNo > *aMaxSmoNo )
        {
            *aMaxSmoNo = sCurSmoNo;
        }
        else
        {
            /* nothing to do */
        }
    }
}

/***********************************************
 * BUG-24403
 *
 * [IN]  aStatistics,
 * [IN]  aTable,
 * [IN]  aIndexHeader,
 * [OUT] aSmoNo
 *
***********************************************/
void smnManager::getIndexSmoNo( smnIndexHeader * aIndexHeader,
                                ULong          * aSmoNo )
{
    smnIndexHeader * sIndexHeader = aIndexHeader;

    IDE_DASSERT( aIndexHeader != NULL );

    *aSmoNo = 0;

    if ( sIndexHeader->mModule->mGetSmoNo != NULL )
    {
        sIndexHeader->mModule->mGetSmoNo( sIndexHeader->mHeader,
                                          aSmoNo );
    }
    else
    {
        /* nothing to do */
    }
}

UInt smnManager::getMaxIndexCnt()
{
    return SMN_MAX_IDX_COUNT;
}

idBool smnManager::isPrimaryIndex( void* aIndexHeader )
{
    UInt sIndexFlag = ((smnIndexHeader*)aIndexHeader)->mFlag;

    if ( ( sIndexFlag & SMI_INDEX_TYPE_MASK ) 
         == SMI_INDEX_TYPE_PRIMARY_KEY )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/*
 * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
 * Disk Index Segment 연산 인터페이스 모듈 반환
 */
void * smnManager::getSegDescByIdxPtr( void * aIndex )
{
    smnIndexHeader   * sIndex;
    sIndex = (smnIndexHeader*)aIndex;

    // BUG-25279 Btree For Spatial과 Disk Btree의 자료구조 및 로깅 분리
    // Disk TableSpace의 변환시에만 호출되기 때문에, Disk Index에 속하는
    // Btree와 Rtree일 경우에만 의미가 있다.
    IDE_TEST( ( sIndex->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID ) &&
              ( sIndex->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID ) );

    return &( ((sdnRuntimeHeader*)sIndex->mHeader)->mSegmentDesc );

    IDE_EXCEPTION_END;

    ideLog::log( SM_TRC_LOG_LEVEL_FATAL,
                 SM_TRC_MINDEX_INDEX_TYPE_FATAL,
                 sIndex->mType );
    IDE_ASSERT(0);

    return NULL;
}

IDE_RC smnManager::dumpCommonHeader( smnIndexHeader * aHeader,
                                     SChar          * aOutBuf,
                                     UInt             aOutSize )
{
    UInt i;

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "Index Common Header :\n"
                     "Pointer                  : 0x%"ID_xPOINTER_FMT"\n"
                     "mIndexSegDesc.mSpaceID   : %"ID_UINT32_FMT"\n"
                     "mIndexSegDesc.mOffset    : %"ID_UINT32_FMT"\n"
                     "mIndexSegDesc.mPageID    : %"ID_UINT32_FMT"\n"
                     "mTableOID                : 0x%"ID_xINT64_FMT"\n"
                     "mSelfOID                 : 0x%"ID_xINT64_FMT"\n"
                     "mName                    : %s\n"
                     "mId                      : %"ID_UINT32_FMT"\n"
                     "mType                    : %"ID_UINT32_FMT"\n"
                     "mFlag                    : %"ID_UINT32_FMT"\n"
                     "mDropFlag                : %"ID_UINT32_FMT"\n"
                     "mMaxKeySize              : %"ID_UINT32_FMT"\n"
                     "mColumnCount             : %"ID_UINT32_FMT"\n",
                     aHeader,
                     aHeader->mIndexSegDesc.mSpaceID,
                     aHeader->mIndexSegDesc.mOffset,
                     aHeader->mIndexSegDesc.mPageID,
                     aHeader->mTableOID,
                     aHeader->mSelfOID,
                     aHeader->mName,
                     aHeader->mId,
                     aHeader->mType,
                     aHeader->mFlag,
                     aHeader->mDropFlag,
                     aHeader->mMaxKeySize, /* PROJ-2433 */
                     aHeader->mColumnCount );

    for ( i = 0 ; i < aHeader->mColumnCount ; i ++ )
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "[%2"ID_UINT32_FMT"] %8"ID_UINT32_FMT
                             " %4"ID_UINT32_FMT
                             " %5"ID_UINT32_FMT"\n",
                             i,
                             aHeader->mColumns[ i ],
                             aHeader->mColumnFlags[ i ],
                             aHeader->mColumnOffsets[ i ] );
    }

    return IDE_SUCCESS;
}

void smnManager::logCommonHeader( smnIndexHeader * aHeader )
{
    SChar      * sOutBuffer4Dump;

    if ( aHeader != NULL )
    {
        if ( iduMemMgr::calloc( IDU_MEM_SM_SMN, 1,
                                ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                (void**)&sOutBuffer4Dump )
             == IDE_SUCCESS )
        {
            if ( smnManager::dumpCommonHeader( (smnIndexHeader*)aHeader,
                                               sOutBuffer4Dump,
                                               IDE_DUMP_DEST_LIMIT )
                 == IDE_SUCCESS )
            {
                ideLog::log( IDE_ERR_0, "%s\n", sOutBuffer4Dump );
            }
            else
            {
                /* dump fail */
            }

            (void) iduMemMgr::free( sOutBuffer4Dump );
        }
        else
        {
            /* alloc fail */
        }
    }
    else
    {
        ideLog::log( IDE_ERR_0,
                     "smnIndexHeader* is NULL(smnManager::logCommonHeader())\n" );
    }

}

IDE_RC smnManager::initIndexStatistics( smnIndexHeader   * aHeader,
                                        smnRuntimeHeader * aRunHeader,
                                        UInt               aIndexID )
{
    SChar                 sBuffer[IDU_MUTEX_NAME_LEN];

    IDE_ASSERT( aHeader    != NULL );
    IDE_ASSERT( aRunHeader != NULL );

    idlOS::snprintf( sBuffer,
                     ID_SIZEOF(sBuffer),
                     "INDEX_STAT_MUTEX_%"ID_UINT32_FMT,
                     aIndexID );

    IDE_TEST( aRunHeader->mStatMutex.initialize( sBuffer,
                                                 IDU_MUTEX_KIND_NATIVE,
                                                 IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );

    aHeader->mStat.mId      = aIndexID;  // BUG-47946
    aRunHeader->mAtomicA    = 0;
    aRunHeader->mAtomicB    = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnManager::destIndexStatistics( smnIndexHeader   * aIndexHeader,
                                        smnRuntimeHeader * aRunHeader )
{

    IDE_ASSERT( aRunHeader   != NULL );

    IDE_TEST( aRunHeader->mStatMutex.destroy() != IDE_SUCCESS );

    aIndexHeader->mLatestStat = NULL;   /* BUG-42095 */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnManager::doKeyReorganization( smnIndexHeader * aIndexHeader )
{
    IDE_ERROR( aIndexHeader != NULL );
    
    IDE_TEST( smnbBTree::keyReorganization( (smnbHeader*)aIndexHeader->mHeader ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
