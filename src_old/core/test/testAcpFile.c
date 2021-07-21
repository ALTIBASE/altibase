/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id: testAcpFile.c 11323 2010-06-23 05:50:16Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpCStr.h>
#include <acpFile.h>
#include <acpSys.h>
#include <acpDir.h>

#define CONTENT_BUFFER_SIZE 1024

#define CONTENT      "I LOVE ALTIBASE!!! WE LOVE ALTIBASE!! ALTIBASE FOREVER!!"
#define CONTENT_LEN  acpCStrLen(CONTENT, CONTENT_BUFFER_SIZE)


#define TEST_ACP_FILE_PERM (                    \
        ACP_S_IRUSR | ACP_S_IWUSR |             \
        ACP_S_IRGRP | ACP_S_IWGRP |             \
        ACP_S_IROTH | ACP_S_IWOTH)

static acp_char_t *gPath1 = "test_acp_file1.dat";
static acp_char_t *gPath2 = "test_acp_file2.dat";

static void testOpen(void)
{
    acp_char_t sBuffer[CONTENT_BUFFER_SIZE];
    acp_size_t sSize   = 0;
    acp_file_t sFile;
    acp_file_t sFile2;
    acp_rc_t   sRC;

    /*
     * acpFileOpen ACP_O_CREAT | ACP_O_EXCL
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR | ACP_O_CREAT, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileOpen(&sFile,
                      gPath1,
                      ACP_O_RDWR | ACP_O_CREAT | ACP_O_EXCL,
                      0);
    ACT_CHECK(ACP_RC_IS_EEXIST(sRC));

#if !defined(ALTI_CFG_OS_WINDOWS)
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_EACCES(sRC));
#endif

    sRC = acpFileRemove(gPath1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpFileOpen ACP_O_WRONLY
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_WRONLY | ACP_O_CREAT,
                      TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileWrite(&sFile, CONTENT, CONTENT_LEN, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_SET, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, sizeof(sBuffer), NULL);
    ACT_CHECK(ACP_RC_IS_EACCES(sRC));

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpFileOpen ACP_O_RDONLY
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDONLY, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, sizeof(sBuffer), NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, sizeof(sBuffer), NULL);
    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    sRC = acpFileWrite(&sFile, CONTENT, CONTENT_LEN, NULL);
    ACT_CHECK(ACP_RC_IS_EACCES(sRC));

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpFileOpen ACP_O_APPEND
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR | ACP_O_APPEND,
                      TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, sizeof(sBuffer), &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSize == CONTENT_LEN);

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_SET, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileWrite(&sFile, CONTENT, CONTENT_LEN, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_SET, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, sizeof(sBuffer), &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSize == CONTENT_LEN * 2);

    sRC = acpFileDup(&sFile, &sFile2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile2, 0, ACP_SEEK_SET, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileWrite(&sFile2, CONTENT,
                       acpCStrLen(CONTENT, CONTENT_LEN), NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile2, 0, ACP_SEEK_SET, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile2, sBuffer, sizeof(sBuffer), &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSize == CONTENT_LEN * 3);

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileClose(&sFile2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpFileOpen ACP_O_TRUNC
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR | ACP_O_TRUNC,
                      TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileWrite(&sFile, CONTENT, CONTENT_LEN, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_SET, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, sizeof(sBuffer), &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSize == CONTENT_LEN);

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpFileOpen ACP_O_SYNC
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR | ACP_O_SYNC,
                      TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileWrite(&sFile, CONTENT, CONTENT_LEN, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_SET, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, sizeof(sBuffer), &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSize == CONTENT_LEN);

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static void testReadWrite(void)
{
    acp_char_t sBuffer[CONTENT_BUFFER_SIZE];
    acp_size_t sSize   = 0;
    acp_file_t sFile;
    acp_rc_t   sRC;

    /*
     * acpFileRead/acpFileWrite more than 2GB
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, (acp_size_t)ACP_SINT32_MAX + 1, &sSize);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpFileWrite(&sFile, sBuffer, (acp_size_t)ACP_SINT32_MAX + 1, &sSize);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpFileSync(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static void testStat(void)
{
    acp_file_t sFile;
    acp_stat_t sStat;
    acp_time_t sTime;
    acp_rc_t   sRC;

    /*
     * Get Current Time
     */
    sTime = acpTimeNow();

    /*
     * acpFileStat
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDONLY, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileStat(&sFile, &sStat);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(((sStat.mPermission & ACP_S_IRUSR) != 0) &&
             ((sStat.mPermission & ACP_S_IWUSR) != 0));

    ACT_CHECK(sStat.mType == ACP_FILE_REG);
    ACT_CHECK(sStat.mSize == (acp_offset_t)CONTENT_LEN);
    ACT_CHECK(sStat.mAccessTime >= acpTimeToSec(sTime));
    ACT_CHECK(sStat.mModifyTime >= acpTimeToSec(sTime));
    ACT_CHECK(sStat.mChangeTime >= acpTimeToSec(sTime));

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileStatAtPath(gPath1, &sStat, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(((sStat.mPermission & ACP_S_IRUSR) != 0) &&
             ((sStat.mPermission & ACP_S_IWUSR) != 0));

    ACT_CHECK(sStat.mType == ACP_FILE_REG);
    ACT_CHECK(sStat.mSize == (acp_offset_t)CONTENT_LEN);
    ACT_CHECK(sStat.mAccessTime >= acpTimeToSec(sTime));
    ACT_CHECK(sStat.mModifyTime >= acpTimeToSec(sTime));
    ACT_CHECK(sStat.mChangeTime >= acpTimeToSec(sTime));
}

static void testTruncate(void)
{
    acp_file_t   sFile;
    acp_stat_t   sStat;
    acp_time_t   sTime;
    acp_offset_t sOffset = 0;
    acp_rc_t     sRC;

    /*
     * Get Current Time
     */
    sTime = acpTimeNow();

    /*
     * acpFileTruncate
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, 512, ACP_SEEK_SET, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileTruncate(&sFile, 1024);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_CUR, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sOffset == 512);

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileStatAtPath(gPath1, &sStat, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(((sStat.mPermission & ACP_S_IRUSR) != 0) &&
              ((sStat.mPermission & ACP_S_IWUSR) != 0));
    ACT_CHECK(sStat.mType == ACP_FILE_REG);
    ACT_CHECK(sStat.mSize == (acp_offset_t)1024);
    ACT_CHECK(sStat.mAccessTime >= acpTimeToSec(sTime));
    ACT_CHECK(sStat.mModifyTime >= acpTimeToSec(sTime));
    ACT_CHECK(sStat.mChangeTime >= acpTimeToSec(sTime));

    sRC = acpFileTruncateAtPath(gPath1, 2048);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileStatAtPath(gPath1, &sStat, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(((sStat.mPermission & ACP_S_IRUSR) != 0) &&
              ((sStat.mPermission & ACP_S_IWUSR) != 0));
    ACT_CHECK(sStat.mType == ACP_FILE_REG);
    ACT_CHECK(sStat.mSize == (acp_offset_t)2048);
    ACT_CHECK(sStat.mAccessTime >= acpTimeToSec(sTime));
    ACT_CHECK(sStat.mModifyTime >= acpTimeToSec(sTime));
    ACT_CHECK(sStat.mChangeTime >= acpTimeToSec(sTime));
}

static void testSeek(void)
{
    acp_char_t   sBuffer[CONTENT_BUFFER_SIZE];
    acp_size_t   sSize   = 0;
    acp_file_t   sFile;
    acp_offset_t sOffset = 0;
    acp_rc_t     sRC;

    /*
     * acpFileSeek
     */
    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_SET, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sOffset == (acp_offset_t)0);

    sRC = acpFileSeek(&sFile, 100, ACP_SEEK_CUR, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sOffset == (acp_offset_t)100);

    sRC = acpFileSeek(&sFile, 200, ACP_SEEK_CUR, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sOffset == (acp_offset_t)300);

    sRC = acpFileSeek(&sFile, -100, ACP_SEEK_CUR, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sOffset == (acp_offset_t)200);

    sRC = acpFileWrite(&sFile, CONTENT, CONTENT_LEN, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, -48, ACP_SEEK_END, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sOffset == (acp_offset_t)2000);

    sRC = acpFileSeek(&sFile, 200, ACP_SEEK_SET, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sOffset == (acp_offset_t)200);

    sRC = acpFileRead(&sFile, sBuffer,
                      acpCStrLen(CONTENT, CONTENT_LEN), &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSize == CONTENT_LEN);
    ACT_CHECK(acpCStrCmp(sBuffer,
                          CONTENT,
                          acpCStrLen(CONTENT, CONTENT_LEN))
              == 0);

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static void testDup(void)
{
    acp_char_t   sBuffer[CONTENT_BUFFER_SIZE];
    acp_size_t   sSize   = 0;
    acp_file_t   sFile;
    acp_file_t   sFile2;
    acp_offset_t sOffset = 0;
    acp_rc_t     sRC;

    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileDup(&sFile, &sFile2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile2, 200, ACP_SEEK_SET, &sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sOffset == (acp_offset_t)200);

    sRC = acpFileRead(&sFile2, sBuffer,
                      acpCStrLen(CONTENT, CONTENT_LEN), &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSize == CONTENT_LEN);
    ACT_CHECK(acpCStrCmp(sBuffer,
                          CONTENT,
                          acpCStrLen(CONTENT, CONTENT_LEN))
              == 0);

    sRC = acpFileClose(&sFile2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static void testRename(void)
{
    acp_rc_t sRC;

    sRC = acpFileRename(gPath2, gPath1);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    sRC = acpFileRename(gPath1, gPath2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static void testRemove(void)
{
    acp_rc_t sRC;

    sRC = acpFileRemove(gPath1);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    sRC = acpFileRemove(gPath2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static acp_sint32_t testThreadLock(void *aFile)
{
    acp_file_t *sFile = aFile;
    acp_rc_t    sRC;

    /*
     * BUG-19458 [MDW/CORE] core unit test acpFile fail on es30
     *
     * acpFileTryLock and acpFileLock are implemented by fcntl(2) and
     * LinuxThread (before Kernel 2.6, Pthreads of GNU C library
     * are supported by using this library.) returns different
     * a different values * in each thread whenever we call getpid(2).
     *
     * As a result,
     *
     * Threads do not share a common session ID and process group ID.
     * Threads do not share record locks created using fcntl(2).
     *
     * (reference document)
     * http://linux.die.net/man/7/pthreads
     */
    sRC = acpFileTryLock(sFile);
#if defined(ALTI_CFG_OS_WINDOWS)
    ACT_CHECK(ACP_RC_IS_EBUSY(sRC));
#elif defined(ALTI_CFG_OS_LINUX) &&                              \
    ((ALTI_CFG_OS_MAJOR <= 2) && (ALTI_CFG_OS_MINOR < 6))
    ACT_CHECK(ACP_RC_IS_EBUSY(sRC));
#else
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpFileTryLock should return SUCCESS but (%d)", sRC));
#endif

    return 0;
}

static void testLock(void)
{
    acp_thr_t  sThr;
    acp_file_t sFile;
    acp_rc_t   sRC;

    sRC = acpFileOpen(&sFile, gPath1, ACP_O_RDWR, TEST_ACP_FILE_PERM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileLock(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCreate(&sThr, NULL, testThreadLock, &sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrJoin(&sThr, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileUnlock(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileTryLock(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileUnlock(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

void testKey(acp_char_t* aPath)
{
    acp_rc_t sRC;
    acp_key_t sKey1;
    acp_key_t sKey2;
    acp_char_t sHome[ACP_PATH_MAX_LENGTH + 1];

    /* no path error */
    sRC = acpFileGetKey("/NoFileLikeThis", 12, &sKey1);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

    /* no project id error */
    sRC = acpFileGetKey(aPath, 0, &sKey1);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

    sRC = acpFileGetKey(aPath, 25, &sKey1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileGetKey(aPath, 25, &sKey2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* the same path, same id must generate same key */
    ACT_CHECK(sKey1 == sKey2);

    sRC = acpFileGetKey(aPath, 26, &sKey2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sKey1 != sKey2);

    /* Get home directory and get key */
    sRC = acpDirGetHome(sHome, ACP_PATH_MAX_LENGTH);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpFileGetKey(sHome, 1, &sKey1);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
            ("Could not get key value for %s : %d\n",
             sHome, (acp_sint32_t)(sRC))
            );
    sRC = acpFileGetKey(sHome, 1, &sKey2);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
            ("Could not get key value for %s : %d\n",
             sHome, (acp_sint32_t)(sRC))
            );
    ACT_CHECK(sKey1 == sKey2);

    sRC = acpFileGetKey(sHome, 1, &sKey1);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
            ("Could not get key value for %s : %d\n",
             sHome, (acp_sint32_t)(sRC))
            );
    sRC = acpFileGetKey(sHome, 2, &sKey2);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
            ("Could not get key value for %s : %d\n",
             sHome, (acp_sint32_t)(sRC))
            );
    ACT_CHECK(sKey1 != sKey2);

#if defined(ALTI_CFG_OS_WINDOWS)
    sRC = acpFileGetKey("C:\\", 1, &sKey1);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
            ("Could not get key value for C:\\ : %d\n",
             sHome, (acp_sint32_t)(sRC))
            );
    sRC = acpFileGetKey("NODRIVE:\\", 1, &sKey1);
    ACT_CHECK_DESC(ACP_RC_NOT_SUCCESS(sRC),
            ("Key must not be got for NODRIVE:\\\n", sHome));
    sRC = acpFileGetKey("C:\\NoDirectoryLikeThis\\", 1, &sKey1);
    ACT_CHECK_DESC(ACP_RC_NOT_SUCCESS(sRC),
            ("Key must not be got for C:\\NoDirectoryLikeThis\\\n", sHome));
#endif
}

static void testAcpFilePipeOpen(void)
{
    acp_rc_t     sRC;
    acp_file_t   sPipe[2];

    sRC = acpFilePipe(sPipe);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileClose(&sPipe[ACP_PIPE_READ]);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileClose(&sPipe[ACP_PIPE_WRITE]);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static void testAcpFilePipeReadWrite(void)
{
    acp_rc_t   sRC;
    acp_file_t sPipe[2];
    acp_size_t sContentLen = acpCStrLen(CONTENT, CONTENT_BUFFER_SIZE);
    acp_size_t sReadSize   = 0;
    acp_size_t sWriteSize   = 0;
    acp_char_t sBuffer[CONTENT_BUFFER_SIZE];

    sRC = acpFilePipe(sPipe);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileWrite(&sPipe[ACP_PIPE_WRITE],
                       CONTENT,
                       sContentLen,
                       &sWriteSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK_DESC(sContentLen == sWriteSize,
                   ("sReadSize should be %d but %d",
                    (acp_sint32_t)sContentLen, (acp_sint32_t)sWriteSize));

    sRC = acpFileRead(&sPipe[ACP_PIPE_READ],
                      sBuffer,
                      CONTENT_BUFFER_SIZE,
                      &sReadSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK_DESC(sContentLen == sReadSize,
                   ("sReadSize should be %d but %d",
                    (acp_sint32_t)sContentLen, (acp_sint32_t)sReadSize));

    /*
     * Check if we can read same data that was written to pipe.
     */
    ACT_CHECK(0 == acpMemCmp(sBuffer, CONTENT, sContentLen));

    /*
     * Try to write data to read handler of pipe and read from write handler.
     * It should fail.
     */
    sRC = acpFileWrite(&sPipe[ACP_PIPE_READ],
                       CONTENT,
                       sContentLen,
                       &sWriteSize);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

    sRC = acpFileRead(&sPipe[ACP_PIPE_WRITE],
                      sBuffer,
                      CONTENT_BUFFER_SIZE,
                      &sReadSize);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

    /*
     * Close file handler and try to read and write it. It should fail.
     */
    sRC = acpFileClose(&sPipe[ACP_PIPE_READ]);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sPipe[ACP_PIPE_READ],
                      sBuffer,
                      CONTENT_BUFFER_SIZE,
                      &sReadSize);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

    sRC = acpFileClose(&sPipe[ACP_PIPE_WRITE]);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileWrite(&sPipe[ACP_PIPE_WRITE],
                       CONTENT,
                       sContentLen,
                       &sWriteSize);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    ACP_UNUSED(aArgc);

    ACT_TEST_BEGIN();

    testOpen();
    testReadWrite();
    testStat();
    testTruncate();
    testSeek();
    testDup();
    testLock();
    testRename();
    testRemove();

    testKey(aArgv[0]);

    testAcpFilePipeOpen();
    testAcpFilePipeReadWrite();

    ACT_TEST_END();

    return 0;
}
