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
* $Id: smmPLoadMgr.cpp 82075 2018-01-17 06:39:52Z jina.kim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smm.h>

IDE_RC smmPLoadMgr::initializePloadMgr(smmTBSNode *     aTBSNode,
                                       SInt             aThreadCount )
{
    SInt i;

    mTBSNode         = aTBSNode;
    mThreadCount     = aThreadCount;
    mCurrentDB       = mTBSNode->mTBSAttr.mMemAttr.mCurrentDB;
    mCurrFileNumber  = 0;
    mStartReadPageID = 0;

    mMaxFileNumber = smmManager::getRestoreDBFileCount( aTBSNode );

    /* TC/FIT/Limit/sm/smm/smmPLoadMgr_initializePloadMgr_calloc.sql */
    IDU_FIT_POINT_RAISE( "smmPLoadMgr::initializePloadMgr::calloc",
                          insufficient_memory );

    // client 메모리 할당 
    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_SM_SMM,
                               aThreadCount,
                               ID_SIZEOF(smmPLoadChild *),
                               (void**)&mChildArray) != IDE_SUCCESS,
                   insufficient_memory );
    
    for (i = 0; i < aThreadCount; i++)
    {
        /* TC/FIT/Limit/sm/smm/smmPLoadMgr_initializePloadMgr_malloc.sql */
        IDU_FIT_POINT_RAISE( "smmPLoadMgr::initializePloadMgr::malloc",
                              insufficient_memory );

        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMM,
                                   ID_SIZEOF(smmPLoadChild),
                                   (void**)&(mChildArray[i])) != IDE_SUCCESS,
                       insufficient_memory );
        
        mChildArray[i] = new (mChildArray[i]) smmPLoadChild();
    }
    
    // 매니저 초기화 
    IDE_TEST(smtPJMgr::initialize(aThreadCount,
                                  (smtPJChild **)mChildArray,
                                  &mSuccess)
                     != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC smmPLoadMgr::destroy()
{
    
    SInt i;

    IDE_ASSERT(smtPJMgr::destroy() == IDE_SUCCESS);
    
    for (i = 0; i < mThreadCount; i++)
    {
        IDE_TEST(mChildArray[i]->destroy() != IDE_SUCCESS);

        IDE_TEST(iduMemMgr::free(mChildArray[i]) != IDE_SUCCESS);
    }

    IDE_TEST(iduMemMgr::free(mChildArray) != IDE_SUCCESS);

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

IDE_RC smmPLoadMgr::assignJob(SInt    aReqChild,
                              idBool *aJobAssigned)
{
    
    smmDatabaseFile *s_DbFile;
    ULong            s_nFileSize = 0;

    ULong            sWrittenPageCount;
    scPageID         sPageCountPerFile;
    
    
    if ( mCurrFileNumber < mMaxFileNumber)
    {
        // 이 파일에 기록할 수 있는 Page의 수 
        sPageCountPerFile = smmManager::getPageCountPerFile( mTBSNode,
                                                             mCurrFileNumber );
        
        // DB 파일이 Disk에 존재한다면?
        if ( smmDatabaseFile::isDBFileOnDisk( mTBSNode,
                                              mCurrentDB,
                                              mCurrFileNumber ) == ID_TRUE )
        {
            IDE_TEST( smmManager::openAndGetDBFile( mTBSNode,
                                                    mCurrentDB,
                                                    mCurrFileNumber,
                                                    &s_DbFile )
                      != IDE_SUCCESS );
            
        
            // 실제 파일에 기록된 Page의 수를 계산한다.
            IDE_TEST( s_DbFile->getFileSize(&s_nFileSize) 
                      != IDE_SUCCESS );

            if ( s_nFileSize > SM_DBFILE_METAHDR_PAGE_SIZE )
            {
                sWrittenPageCount = (s_nFileSize-
                                SM_DBFILE_METAHDR_PAGE_SIZE)
                                / SM_PAGE_SIZE;
            
                mChildArray[aReqChild]->setFileToBeLoad(
                    mTBSNode,
                    mCurrFileNumber, 
                    mStartReadPageID,
                    mStartReadPageID + sPageCountPerFile - 1,
                    sWrittenPageCount );
                *aJobAssigned = ID_TRUE;
            }
            else
            {
                // To FIX BUG-18630
                // 8K보다 작은 DB파일이 존재할 경우 Restart Recovery실패함
                //
                // 8K보다 작은 크기의 DB파일에는
                // 데이터 페이지가 기록되지 않은 것이므로
                // DB파일 Restore를 SKIP한다.
                *aJobAssigned = ID_FALSE;
            }
        }
        else // DB파일이 존재하지 않는 경우
        {
            /*
             * To Fix BUG-13966  checkpoint중 DB파일 미처 생성하지 못한
             *                   상황에서 server 죽은 후, startup이 안됨 
             *
             * Checkpoint Thread 와 Chunk를 확장하는 Insert Transaction
             * 사이의 작업이 다음과 같이 수행되면,
             * Checkpoint도중 DB파일이 미처 생기지 않을 수 있다.
             *
             * 자세한 내용은 smmManager::loadSerial2의 주석을 참고
             */

            // do nothing !
            // Job이 Assign되지 않았으므로 Flag세팅!
            *aJobAssigned = ID_FALSE;
        }

        // PROJ-1490
        // DB파일안의 Free Page는 Disk로 내려가지도 않고
        // 메모리로 올라가지도 않는다.
        // 그러므로, DB파일의 크기와 DB파일에 저장되어야 할 Page수와는
        // 아무런 관계가 없다.
        //
        // 각 DB파일이 기록해야할 Page의 수를 계산하여
        // 각 DB파일의 로드 시작 Page ID를 계산한다.
        mStartReadPageID += sPageCountPerFile ;
        
        mCurrFileNumber++;
    }
    else
    {
        *aJobAssigned = ID_FALSE;
    }
    
    smmManager::setStartupPID(mTBSNode, mStartReadPageID);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}
