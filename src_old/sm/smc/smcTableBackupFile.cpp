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
 * $Id: smcTableBackupFile.cpp 83763 2018-08-22 01:58:35Z emlee $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <smu.h>
#include <smcDef.h>
#include <smcReq.h>
#include <smcTable.h>
#include <smcTableBackupFile.h>

smcTableBackupFile::smcTableBackupFile()
{
}

smcTableBackupFile::~smcTableBackupFile()
{
}

IDE_RC smcTableBackupFile::initialize(SChar *aStrFileName)
{

    /* Initialize Memer */
    mBeginPos = 0;
    mEndPos   = 0;
    mFileSize = 0;
    mFirstWriteOffset = 0;
    mLastWriteOffset = 0;
    mCreate   = ID_FALSE;
    mOpen     = ID_FALSE;

    mBufferSize = smuProperty::getTableBackupFileBufferSize() * 1024;

    /* Allocate Buffer */
    mBuffer   = NULL;

    if(mBufferSize != 0)
    {
        /* smcTableBackupFile_initialize_calloc_Buffer.tc */
        IDU_FIT_POINT("smcTableBackupFile::initialize::calloc::Buffer");
        IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMC,
                                   1,
                                   mBufferSize,
                                   (void**)&mBuffer,
                                   IDU_MEM_FORCE)
                 != IDE_SUCCESS);

        IDE_ASSERT(mBufferSize > 0);
    }

    IDE_TEST(mFile.initialize( IDU_MEM_SM_SMC,
                               1, /* Max Open FD Count */
                               IDU_FIO_STAT_OFF,
                               IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);
    
    IDE_TEST(mFile.setFileName(aStrFileName) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smcTableBackupFile::destroy()
{

    IDE_ASSERT(mOpen == ID_FALSE);

    /* Free Memory */
    if(mBuffer != NULL)
    {
        IDE_TEST(iduMemMgr::free(mBuffer) != IDE_SUCCESS);
        
        mBuffer = NULL;
    }

    IDE_TEST(mFile.destroy() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smcTableBackupFile::open(idBool aCreate)
{

    IDE_ASSERT(mOpen == ID_FALSE);
    
    mBeginPos = 0;
    mEndPos   = 0;
    mFileSize = 0;
    mLastWriteOffset = 0;
    mFirstWriteOffset = 0;
    
    mCreate   = ID_FALSE;
    mOpen     = ID_FALSE;

    if(aCreate == ID_TRUE)
    {
        /* Create File */
        IDE_TEST( mFile.createUntilSuccess( smLayerCallback::setEmergency )
                  != IDE_SUCCESS );
        mCreate = ID_TRUE;
    }
    
    IDE_TEST(mFile.open() != IDE_SUCCESS);
    mOpen = ID_TRUE;

    if(mCreate == ID_FALSE)
    {
        IDE_TEST(mFile.read(NULL, /* idvSQL* */
                            0,
                            mBuffer,
                            mBufferSize,
                            NULL)
                 != IDE_SUCCESS);
    }

    IDE_TEST(mFile.getFileSize(&mFileSize)
             != IDE_SUCCESS);
    
    mBeginPos = 0;
    mEndPos   = mBufferSize;
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC smcTableBackupFile::close()
{

    ULong sFileSize;
    
    if(mLastWriteOffset != 0)
    {
        IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                           mBeginPos + mFirstWriteOffset,
                                           mBuffer,
                                           mLastWriteOffset - mFirstWriteOffset,
                                           smLayerCallback::setEmergency )
                  != IDE_SUCCESS );

        sFileSize = mBeginPos + mLastWriteOffset;
        mFileSize = mFileSize < sFileSize ? sFileSize : mFileSize;
        
        mFirstWriteOffset = 0;
        mLastWriteOffset = 0;
    }

    IDE_TEST( mFile.syncUntilSuccess( smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    IDE_TEST(mFile.sync() != IDE_SUCCESS);
    
    mOpen = ID_FALSE;
    IDE_TEST(mFile.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC smcTableBackupFile::read(ULong  aWhere,
                                void  *aBuffer,
                                SInt   aSize)
{

    SInt  sReadSize;
    ULong sFileSize;

    sReadSize = aSize;

    while(sReadSize != 0)
    {
        if(aSize > mBufferSize)
        {
            /*
              File Buffer                   |-------|
              Read Pos |-----------------|

              File Buffer |-------|
              Read Pos             |-----------------|
              
            */
            
            if(((aWhere < mBeginPos) && (aWhere + aSize) < mBeginPos) || (aWhere > mEndPos))
            {
                IDE_TEST(mFile.read(NULL, /* idvSQL* */
                                    aWhere,
                                    aBuffer,
                                    aSize,
                                    NULL) != IDE_SUCCESS);
                break;
            }
            else
            {
                /*
                  File Buffer      |-------|
                  Read Pos |-----------------|
                */

                if(aWhere <= mBeginPos && ((aWhere + aSize) >= mEndPos) )
                {
                    IDE_TEST(mFile.read( NULL, /* idvSQL* */
                                         aWhere,
                                         aBuffer,
                                         mBeginPos - aWhere,
                                         NULL )
                         != IDE_SUCCESS);
                    
                    IDE_TEST(mFile.read(NULL, /* idvSQL* */
                                        mEndPos,
                                        (SChar*)aBuffer + mEndPos - aWhere,
                                        aWhere + aSize - mEndPos, NULL)
                             != IDE_SUCCESS);
                    
                    idlOS::memcpy((SChar*)aBuffer + mBeginPos - aWhere,
                                  mBuffer,
                                  mBufferSize);
                }
                else
                {
                    /*
                      File Buffer             |-------|
                      Read Pos |-----------------|
                    */
                    
                    if(aWhere <= mBeginPos && ((aWhere + aSize) <= mEndPos) )
                    {
                        IDE_TEST(mFile.read( NULL, /* idvSQL* */
                                             aWhere,
                                             aBuffer,
                                             mBeginPos - aWhere,
                                             NULL )
                                 != IDE_SUCCESS);
                        
                        idlOS::memcpy((SChar*)aBuffer + mBeginPos - aWhere,
                                      mBuffer,
                                      aWhere + aSize - mBeginPos);
                    }
                    else
                    {
                        
                        /*
                          File Buffer    |-------|
                          Read Pos        |-----------------|
                        */
                        if( (aWhere >= mBeginPos) &&  (aWhere <= mEndPos) )
                        {
                            IDE_TEST(mFile.read(NULL, /* idvSQL* */
                                                mEndPos,
                                                (SChar*)aBuffer + mEndPos - aWhere,
                                                aWhere + aSize - mEndPos, NULL)
                                     != IDE_SUCCESS);
                            
                            idlOS::memcpy(aBuffer,
                                          mBuffer + aWhere - mBeginPos,
                                          mEndPos - aWhere);

                        }
                        else
                        {
                            IDE_ASSERT(0);
                        }
                    }
                }
            }
            
            break;
        }
        else
        {
            if(((aWhere >= mBeginPos) && (aWhere <= mEndPos)) ||
               (((aWhere + aSize) >= mBeginPos) && ((aWhere + aSize) <= mEndPos)))
            {
                if(aWhere <= mBeginPos)
                {
                    /*
                      File Buffer      |-----------------------|
                      Read Pos |---------|
                    */
            
                    sReadSize = aWhere + aSize - mBeginPos;
            
                    idlOS::memcpy((SChar*)aBuffer + mBeginPos - aWhere,
                                  mBuffer,
                                  sReadSize);
                    aSize = mBeginPos - aWhere;
                }
                else
                {
                    if((aWhere + aSize) >= mEndPos)
                    {
                        /*
                          File Buffer |-----------------------|
                          Read Pos                         |---------|
                        */
                        idlOS::memcpy(aBuffer,
                                      mBuffer + aWhere - mBeginPos,
                                      mEndPos - aWhere);

                        aBuffer = (SChar*)aBuffer + mEndPos - aWhere;
                        aSize  = aWhere + aSize - mEndPos;
                        aWhere = mEndPos;
                    }
                    else
                    {
                        /*
                          File Buffer |-----------------------|
                          Read Pos           |---------|
                        */
        
                        idlOS::memcpy(aBuffer,
                                      mBuffer + aWhere - mBeginPos,
                                      aSize);
                        break;
                    }
                }
            }
            
            if(mLastWriteOffset != 0)
            {
                IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                                   mBeginPos + mFirstWriteOffset,
                                                   mBuffer,
                                                   mLastWriteOffset - mFirstWriteOffset,
                                                   smLayerCallback::setEmergency )
                          != IDE_SUCCESS );

                sFileSize = mBeginPos + mLastWriteOffset;
                mFileSize = mFileSize < sFileSize ? sFileSize : mFileSize;

                mFirstWriteOffset = 0;
                mLastWriteOffset = 0;
            }

            if((mCreate == ID_FALSE) || (mFileSize > aWhere))
            {
                IDE_TEST(mFile.read(NULL, /* idvSQL* */
                                    aWhere,
                                    mBuffer,
                                    mBufferSize,
                                    NULL)
                         != IDE_SUCCESS);
            }

            mBeginPos = aWhere;
            mEndPos   = aWhere + mBufferSize;
        }

        idlOS::memcpy(aBuffer,
                      mBuffer + aWhere - mBeginPos,
                      aSize);
        
        break;
    } /* while */
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC smcTableBackupFile::write(ULong  aWhere,
                                 void  *aBuffer,
                                 SInt   aSize)
{

    SInt   sWriteSize;
    SInt   sWriteLstOffset;
    SInt   sWriteFstOffset;
    ULong  sFileSize;

    IDE_ASSERT(aSize > 0);

    sWriteSize = aSize;

    while(1)
    {
        if((aSize > mBufferSize) || mBufferSize == 0)
        {
            /*
              File Buffer                   |-------|
              Write Pos |-----------------|

              File Buffer |-------|
              Write Pos             |-----------------|
              
            */

            if(((aWhere < mBeginPos) && (aWhere + aSize) < mBeginPos) ||
               (aWhere > mEndPos) || (aSize == 0))
            {
                IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                                   aWhere,
                                                   aBuffer,
                                                   aSize,
                                                   smLayerCallback::setEmergency )
                          != IDE_SUCCESS );

                /* 새로운 파일 크기를 정한다. */
                sFileSize = aWhere + aSize;
                mFileSize = mFileSize < sFileSize ? sFileSize : mFileSize;

                break;
            }
            else
            {
                /*
                  File Buffer      |-------|
                  Write Pos |-----------------|
                */

                if(aWhere <= mBeginPos && ((aWhere + aSize) >= mEndPos) )
                {
                    IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                                       aWhere,
                                                       aBuffer,
                                                       mBeginPos - aWhere,
                                                       smLayerCallback::setEmergency )
                              != IDE_SUCCESS );

                    IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                                       mEndPos,
                                                       (SChar*)aBuffer + mEndPos - aWhere,
                                                       aWhere + aSize - mEndPos,
                                                       smLayerCallback::setEmergency )
                              != IDE_SUCCESS );

                    sFileSize = aWhere + aSize;
                    mFileSize = mFileSize < sFileSize ? sFileSize : mFileSize;

                    idlOS::memcpy(mBuffer, (SChar*)aBuffer + mBeginPos - aWhere, mBufferSize);
                    mFirstWriteOffset = 0;
                    mLastWriteOffset = mBufferSize;
                }
                else
                {

                    /*
                      File Buffer             |-------|
                      Write Pos |-----------------|
                    */
                    
                    if(aWhere <= mBeginPos && ((aWhere + aSize) <= mEndPos) )
                    {
                        IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                                           aWhere,
                                                           aBuffer,
                                                           mBeginPos - aWhere,
                                                           smLayerCallback::setEmergency )
                                  != IDE_SUCCESS );
                        idlOS::memcpy(mBuffer,
                                      (SChar*)aBuffer + mBeginPos - aWhere,
                                      aWhere + aSize - mBeginPos);

                        mFirstWriteOffset = 0;
                        sWriteSize = aWhere + aSize - mBeginPos;
                        
                        mLastWriteOffset = mLastWriteOffset < sWriteSize ? sWriteSize : mLastWriteOffset;
                    }
                    else
                    {
                        
                        /*
                          File Buffer    |-------|
                          Write Pos        |-----------------|
                        */
                        if( (aWhere >= mBeginPos) && (aWhere <= mEndPos) )
                        {
                            IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                                               mEndPos,
                                                               (SChar*)aBuffer + mEndPos - aWhere,
                                                               aWhere + aSize - mEndPos,
                                                               smLayerCallback::setEmergency )
                                      != IDE_SUCCESS );

                            sFileSize = aWhere + aSize;
                            mFileSize = mFileSize < sFileSize ? sFileSize : mFileSize;
                            
                            idlOS::memcpy(mBuffer + aWhere - mBeginPos,
                                          aBuffer,
                                          mEndPos - aWhere);

                            sWriteFstOffset = aWhere - mBeginPos;
                            mFirstWriteOffset = mFirstWriteOffset > sWriteFstOffset ? sWriteFstOffset : mFirstWriteOffset;
                            mLastWriteOffset = mBufferSize;
                        }
                        else
                        {
                            IDE_ASSERT(0);
                        }
                    }
                }
            }
            
            break;
        }
        else
        {
            if(((aWhere >= mBeginPos) && (aWhere <= mEndPos)) ||
               (((aWhere + aSize) >= mBeginPos) && ((aWhere + aSize) <= mEndPos)))
            {
                if(aWhere <= mBeginPos)
                {
                    /*
                      File Buffer      |-----------------------|
                      Write Pos |---------|
                    */
                    
                    sWriteSize = aWhere + aSize - mBeginPos;
                    
                    idlOS::memcpy(mBuffer, (SChar*)aBuffer + mBeginPos - aWhere, sWriteSize);
                    aSize = mBeginPos - aWhere;
                    
                    mFirstWriteOffset = 0;
                    mLastWriteOffset = mLastWriteOffset < sWriteSize ? sWriteSize : mLastWriteOffset;
                }
                else
                {
                    if((aWhere + aSize) >= mEndPos)
                    {
                        /*
                          File Buffer |-----------------------|
                          Write Pos                      |---------|
                        */
                        idlOS::memcpy(mBuffer + aWhere - mBeginPos, aBuffer, mEndPos - aWhere);
                        sWriteFstOffset = aWhere - mBeginPos;

                        aBuffer = (SChar*)aBuffer + mEndPos - aWhere;
                        aSize = aWhere + aSize - mEndPos;
                        aWhere = mEndPos;
                        mLastWriteOffset = mBufferSize;
                        mFirstWriteOffset =
                            mFirstWriteOffset > sWriteFstOffset ? sWriteFstOffset : mFirstWriteOffset;
                    }
                    else
                    {
                        /*
                          File Buffer |-----------------------|
                          Write Pos           |---------|
                        */
                        
                        idlOS::memcpy(mBuffer + aWhere - mBeginPos, aBuffer, aSize);
                        
                        sWriteFstOffset = aWhere - mBeginPos;
                        sWriteLstOffset = aWhere - mBeginPos + aSize;
                        
                        mFirstWriteOffset = mFirstWriteOffset >  sWriteFstOffset ? sWriteFstOffset : mFirstWriteOffset;
                        mLastWriteOffset = mLastWriteOffset < sWriteLstOffset ? sWriteLstOffset : mLastWriteOffset;
                        
                        break;
                    }
                }
            }
            
            if(mLastWriteOffset != 0)
            {
                IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                                   mBeginPos + mFirstWriteOffset,
                                                   mBuffer,
                                                   mLastWriteOffset - mFirstWriteOffset,
                                                   smLayerCallback::setEmergency )
                          != IDE_SUCCESS );
                
                sFileSize = mBeginPos + mLastWriteOffset;
                mFileSize = mFileSize < sFileSize ? sFileSize : mFileSize;

                mFirstWriteOffset = 0;
                mLastWriteOffset = 0;
            }

            if((mCreate == ID_FALSE) || (mFileSize > aWhere))
            {
                IDE_ASSERT( aSize <= mBufferSize );

                if( ( mBufferSize - aSize ) != 0 )
                {
                    IDE_TEST(mFile.read(NULL, /* idvSQL* */
                                        aWhere + aSize,
                                        mBuffer + aSize,
                                        mBufferSize - aSize, NULL)
                             != IDE_SUCCESS);
                }
            }

            mBeginPos = aWhere;
            mEndPos   = aWhere + mBufferSize;
        }

        idlOS::memcpy(mBuffer + aWhere - mBeginPos,
                      aBuffer,
                      aSize);

        sWriteFstOffset = aWhere - mBeginPos;
        sWriteLstOffset = aWhere - mBeginPos + aSize;
        
        mFirstWriteOffset = mFirstWriteOffset >  sWriteFstOffset? sWriteFstOffset : mFirstWriteOffset;
        mLastWriteOffset = mLastWriteOffset < sWriteLstOffset ? sWriteLstOffset : mLastWriteOffset;
                    
        break;
    } /* while */
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

