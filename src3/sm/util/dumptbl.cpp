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
 * $Id: dumptbl.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smc.h>

int main(int argc, char *argv[])
{
    iduFile sFile;
    ULong   sFileSize = 0;
    smcBackupFileHeader sFileHeader;
    smcBackupFileTail   sFileTail;

    if(argc != 2)
    {
        idlOS::printf("dumptbl table file name \n");

        return -1;
    }

    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);

    IDE_TEST(iduCond::initializeStatic() != IDE_SUCCESS);

    IDE_TEST(sFile.initialize(IDU_MEM_ID,
                              1, /* Max Open FD Count */
                              IDU_FIO_STAT_OFF,
                              IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);

    IDE_TEST(sFile.setFileName(argv[1]) != IDE_SUCCESS);
    IDE_TEST(sFile.open() != IDE_SUCCESS);

    idlOS::memset(&sFileHeader, 0x00, ID_SIZEOF(smcBackupFileHeader));

    IDE_TEST(sFile.read(NULL,
                        0,
                        (void*)&sFileHeader,
                        ID_SIZEOF(smcBackupFileHeader))
         != IDE_SUCCESS);

    idlOS::printf("\n\tType             : %s\n", sFileHeader.mType);
    idlOS::printf("\n\tProduct  Signature : %s\n", sFileHeader.mProductSignature);
    idlOS::printf("\tDatabase Name      : %s\n", sFileHeader.mDbname);
    idlOS::printf("\tVersion  ID        : %"ID_UINT32_FMT"\n", sFileHeader.mVersionID);
    idlOS::printf("\tCompile  Bits      : %"ID_UINT32_FMT"\n", sFileHeader.mCompileBit);
    idlOS::printf("\tBig      Endian    : %d\n", sFileHeader.mBigEndian);
    idlOS::printf("\tTable    OID       : %"ID_vULONG_FMT"\n", sFileHeader.mTableOID);

    IDE_TEST(sFile.getFileSize(&sFileSize) != IDE_SUCCESS);

    IDE_TEST(sFile.read(NULL,
                        sFileSize - ID_SIZEOF(smcBackupFileTail),
                        (void*)&sFileTail,
                        ID_SIZEOF(smcBackupFileTail))
         != IDE_SUCCESS);

    IDE_TEST_RAISE(sFileTail.mSize != sFileSize, err_invalid_version);

    idlOS::printf("\tBackup File Size   : %"ID_UINT64_FMT"\n\n", sFileTail.mSize);

    IDE_TEST(sFile.close() != IDE_SUCCESS);
    IDE_TEST(sFile.destroy() != IDE_SUCCESS);

    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS);
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );

    return 0;

    IDE_EXCEPTION(err_invalid_version);
    {
    idlOS::printf("Wrong File Size!!\n");
    }
    IDE_EXCEPTION_END;

    return -1;
}
