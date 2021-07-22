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
 

#include <iduFile.h>
#include <iduMemMgr.h>
#include <smiDef.h>
#include <sddReq.h>
#include <sddDWFile.h>
#include <sddDiskMgr.h>
#include <smErrorCode.h>
#include <smuProperty.h>

// DWFile header의 첫 부분에 들어갈 magic number이다.
// 이 값은 741이다.
#define SDD_DWFILE_MAGIC_NUMBER \
          ((UInt)('A'+'L'+'T'+'I'+'D'+'W'+'F'+'I'+'L'+'E'))

typedef struct sddDWFileHeader
{
    UInt    mMagicNumber;
    UInt    mSMVersionNumber;
    UInt    mFileID;
    UInt    mPageSize;
    UInt    mPageCount;
} sddDWFileHeader;

IDE_RC sddDWFile::create(UInt           aFileID,
                         UInt           aPageSize,
                         UInt           aPageCount,
                         sdLayerState   aType)
{
    UInt    sDWDirCount = smuProperty::getDWDirCount();
    SChar   sFileName[SMI_MAX_DWFILE_NAME_LEN+16];
    SInt    sState = 0;

    mFileID          = aFileID;
    mPageSize        = aPageSize;
    mPageCount       = aPageCount;
    mFileInitialized = ID_FALSE;
    mFileOpened      = ID_FALSE;

    if( aType == SD_LAYER_BUFFER_POOL )
    {
        sFileName[0] = '\0';
        idlOS::sprintf(sFileName,
                       "%s%c%s%d.dwf",
                       smuProperty::getDWDir(aFileID % sDWDirCount),
                       IDL_FILE_SEPARATOR,
                       SDD_DWFILE_NAME_PREFIX,
                       aFileID);
    }
    else
    {
        sFileName[0] = '\0';
        idlOS::sprintf(sFileName,
                       "%s%c%s%d.dwf",
                       smuProperty::getDWDir(aFileID % sDWDirCount),
                       IDL_FILE_SEPARATOR,
                       SDD_SBUFFER_DWFILE_NAME_PREFIX,
                       aFileID);
    }

    IDE_TEST(mFile.initialize(IDU_MEM_SM_SDD,
                              1, /* Max FD Cnt */
                              IDU_FIO_STAT_OFF,
                              IDV_WAIT_INDEX_NULL)
             != IDE_SUCCESS);

    mFileInitialized = ID_TRUE;

    IDE_TEST(mFile.setFileName(sFileName) != IDE_SUCCESS);

    if (idf::access(sFileName, F_OK) != 0)
    {
        // 파일이 없을때만 파일을 생성한다.
        IDE_TEST( mFile.createUntilSuccess( smLayerCallback::setEmergency )
                  != IDE_SUCCESS );
    }
    sState = 1;

    IDE_TEST(mFile.open((smuProperty::getIOType() == 0) ? ID_FALSE : ID_TRUE)
             != IDE_SUCCESS);
    mFileOpened = ID_TRUE;
    sState = 2;

    IDE_TEST(writeHeader() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        switch (sState)
        {
            case 2:
                IDE_ASSERT(mFile.close() == IDE_SUCCESS);
            case 1:
                IDE_ASSERT(idf::unlink(sFileName) == IDE_SUCCESS);
            default:
                break;
        }
    }
    return IDE_FAILURE;
}

IDE_RC sddDWFile::writeHeader()
{
    SChar           *sBufferPtr;
    SChar           *sAlignedBufferPtr;
    sddDWFileHeader  sHeader;
    UInt             sState = 0;

    IDE_TEST(iduFile::allocBuff4DirectIO(IDU_MEM_SM_SDD,
                                         mPageSize * (mPageCount + 1),
                                         (void**)&sBufferPtr,
                                         (void**)&sAlignedBufferPtr)
             != IDE_SUCCESS);
    sState = 1;

    idlOS::memset(sAlignedBufferPtr, 0, mPageSize * (mPageCount + 1));

    // DWFile header를 초기화한다.
    sHeader.mMagicNumber     = SDD_DWFILE_MAGIC_NUMBER;
    sHeader.mFileID          = mFileID;
    sHeader.mSMVersionNumber = smLayerCallback::getSmVersionIDFromLogAnchor();
    sHeader.mPageSize        = mPageSize;
    sHeader.mPageCount       = mPageCount;

    idlOS::memcpy(sAlignedBufferPtr, &sHeader, ID_SIZEOF(sddDWFileHeader));

    IDE_TEST(mFile.write(NULL, 0, sAlignedBufferPtr, mPageSize * (mPageCount + 1))
             != IDE_SUCCESS);

    IDE_TEST(mFile.sync() != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(iduMemMgr::free(sBufferPtr) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT(iduMemMgr::free(sBufferPtr) == IDE_SUCCESS);
        sBufferPtr = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC sddDWFile::load(SChar *aFileName, idBool *aRightDWFile)
{
    mFileID          = ID_UINT_MAX;
    mPageSize        = ID_UINT_MAX;
    mPageCount       = ID_UINT_MAX;
    mFileInitialized = ID_FALSE;
    mFileOpened      = ID_FALSE;

    *aRightDWFile    = ID_FALSE;

    IDE_TEST_RAISE(idf::access(aFileName, F_OK) != 0,
                   error_file_not_exist);

    IDE_TEST(mFile.initialize(IDU_MEM_SM_SDD,
                              1, /* Max FD Cnt */
                              IDU_FIO_STAT_OFF,
                              IDV_WAIT_INDEX_NULL)
             != IDE_SUCCESS);

    mFileInitialized = ID_TRUE;

    IDE_TEST(mFile.setFileName(aFileName) != IDE_SUCCESS);

    IDE_TEST(mFile.open(ID_FALSE) != IDE_SUCCESS);
    mFileOpened = ID_TRUE;

    readHeader(aRightDWFile);

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_file_not_exist);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistFile, aFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sddDWFile::readHeader(idBool *aRightDWFile)
{
    sddDWFileHeader sHeader;

    if (mFile.read(NULL,
                   0,
                   (void*)&sHeader,
                   ID_SIZEOF(sddDWFileHeader))
        != IDE_SUCCESS)
    {
        *aRightDWFile = ID_FALSE;
    }
    else
    {
        if (sHeader.mMagicNumber != SDD_DWFILE_MAGIC_NUMBER)
        {
            *aRightDWFile = ID_FALSE;
        }
        else
        {
            if ( sHeader.mSMVersionNumber !=
                 smLayerCallback::getSmVersionIDFromLogAnchor() )
            {
                *aRightDWFile = ID_FALSE;
            }
            else
            {
                *aRightDWFile = ID_TRUE;
                mFileID = sHeader.mFileID;
                mPageSize = sHeader.mPageSize;
                mPageCount = sHeader.mPageCount;
            }
        }
    }
}

IDE_RC sddDWFile::destroy()
{
    if (mFileOpened == ID_TRUE)
    {
        IDE_TEST(mFile.close() != IDE_SUCCESS);
    }

    if (mFileInitialized == ID_TRUE)
    {
        IDE_TEST(mFile.destroy() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sddDWFile::write(idvSQL *aStatistics,
                        UChar  *aIOB,
                        UInt    aPageCount)
{
    IDE_DASSERT(aPageCount <= mPageCount);

    IDE_TEST(mFile.write(aStatistics,
                         mPageSize,
                         aIOB,
                         (size_t)mPageSize * aPageCount)
             != IDE_SUCCESS);

    IDU_FIT_POINT( "sddDWFile::write::syncWait" );
    IDE_TEST(mFile.syncUntilSuccess( smLayerCallback::setEmergency ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sddDWFile::read(idvSQL *aStatistics,
                       SChar  *aIOB,
                       UInt    aPageIndex)
{
    IDE_TEST(mFile.read(aStatistics,
                        mPageSize + (size_t)mPageSize * aPageIndex,
                        aIOB,
                        mPageSize)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

