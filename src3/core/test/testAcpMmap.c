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
 * $Id: testAcpMmap.c 5857 2009-06-02 10:39:22Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpMmap.h>


#define SIZE 1024

#define TEST_PERM_RWRWRW (      \
    ACP_S_IRUSR | ACP_S_IWUSR | \
    ACP_S_IRGRP | ACP_S_IWGRP | \
    ACP_S_IROTH | ACP_S_IWOTH)

static acp_char_t *gPath = "test_acp_mmap.dat";

acp_sint32_t main(void)
{
    acp_char_t    sBuffer[SIZE];
    acp_char_t   *sMem;
    acp_size_t    sSize = 0;
    acp_file_t    sFile;
    acp_mmap_t    sMmap;
    acp_rc_t      sRC;
    acp_sint32_t  i;

    ACT_TEST_BEGIN();

    sRC = acpFileOpen(&sFile, gPath, ACP_O_RDWR | ACP_O_CREAT,
                      TEST_PERM_RWRWRW);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileTruncate(&sFile, (acp_offset_t)SIZE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpMmapAttach(&sMmap,
                        &sFile,
                        (acp_offset_t)0,
                        (acp_size_t)SIZE,
                        ACP_MMAP_READ | ACP_MMAP_WRITE,
                        ACP_MMAP_SHARED);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sMem = acpMmapGetAddress(&sMmap);

    for (i = 0; i < SIZE; i++)
    {
        sMem[i] = i % 95 + 32;
    }

    sRC = acpMmapSync(&sMmap, 0, 0, ACP_MMAP_SYNC);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpMmapSync(&sMmap, 1000, 10, ACP_MMAP_SYNC);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_SET, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sFile, sBuffer, sizeof(sBuffer), &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSize == SIZE);

    for (i = 0; i < SIZE; i++)
    {
        ACT_CHECK_DESC(sMem[i] == i % 95 + 32,
                       ("memory at 0x%x does not match", i));
    }

    sRC = acpMmapDetach(&sMmap);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRemove(gPath);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_TEST_END();

    return 0;
}
