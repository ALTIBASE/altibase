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
 * $Id: testAcpPath.c 10068 2010-02-19 02:26:39Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpPath.h>


#define TEST_UNIX_PATH_DIR "a/b/c/d"
#define TEST_UNIX_PATH_BASE "e"
#define TEST_UNIX_PATH_EXT "out"

#define TEST_WIN_PATH_DIR "a\\b\\c\\d"
#define TEST_WIN_PATH_BASE "e"
#define TEST_WIN_PATH_EXT "exe"


acp_sint32_t main(void)
{
    acp_path_pool_t sPathPool;

    acp_char_t *sWinPath =
        TEST_WIN_PATH_DIR"\\"TEST_WIN_PATH_BASE"."TEST_WIN_PATH_EXT;
    
    acp_char_t *sUnixPath =
        TEST_UNIX_PATH_DIR"/"TEST_UNIX_PATH_BASE"."TEST_UNIX_PATH_EXT;

    acp_char_t *sBrokenPath = "a/b\\c/d\\e.exe";

    acp_char_t *sFullPath; 
    

    ACT_TEST_BEGIN();

    acpPathPoolInit(&sPathPool);
    
    /* test cases for only APIs in acpPath.h, not all functions. */
    /* acpPathFull, acpPathDir, acpPathLast, acpPathBase, acpPathExt,
       acpPathMakeExe, */

#if defined(ALTI_CFG_OS_WINDOWS)
    sFullPath = acpPathFull(sWinPath, &sPathPool);
    ACT_CHECK(acpCStrCmp(sFullPath, sWinPath, 128) == 0);

    /* Path delimiter should be set to '\'. */
    sFullPath = acpPathFull(sUnixPath, &sPathPool);
    ACT_CHECK(acpCStrCmp(sFullPath,
                         TEST_WIN_PATH_DIR"\\"TEST_WIN_PATH_BASE"."
                         TEST_UNIX_PATH_EXT, 128) == 0);
    
    sFullPath = acpPathFull(sBrokenPath, &sPathPool);
    ACT_CHECK(acpCStrCmp(sFullPath, sWinPath, 128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathDir(sWinPath, &sPathPool),
                         TEST_WIN_PATH_DIR,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathLast(sWinPath, &sPathPool),
                         TEST_WIN_PATH_BASE"."TEST_WIN_PATH_EXT,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathBase(sWinPath, &sPathPool),
                         TEST_WIN_PATH_DIR"\\"TEST_WIN_PATH_BASE,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathExt(sWinPath, &sPathPool),
                         TEST_WIN_PATH_EXT,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathMakeExe(TEST_WIN_PATH_BASE, &sPathPool),
                         TEST_WIN_PATH_BASE"."TEST_WIN_PATH_EXT,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(
                  acpPathMakeExe(TEST_WIN_PATH_BASE"."TEST_WIN_PATH_EXT,
                                 &sPathPool),
                  TEST_WIN_PATH_BASE"."TEST_WIN_PATH_EXT,
                  128) == 0);
#else
    sFullPath = acpPathFull(sWinPath, &sPathPool);
    ACT_CHECK(acpCStrCmp(sFullPath, sWinPath, 128) == 0);

    sFullPath = acpPathFull(sUnixPath, &sPathPool);
    ACT_CHECK(acpCStrCmp(sFullPath, sUnixPath, 128) == 0);

    sFullPath = acpPathFull(sBrokenPath, &sPathPool);
    ACT_CHECK(acpCStrCmp(sFullPath, sBrokenPath, 128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathDir(sUnixPath, &sPathPool),
                         TEST_UNIX_PATH_DIR,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathLast(sUnixPath, &sPathPool),
                         TEST_UNIX_PATH_BASE"."TEST_UNIX_PATH_EXT,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathBase(sUnixPath, &sPathPool),
                         TEST_UNIX_PATH_DIR"/"TEST_UNIX_PATH_BASE,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathExt(sUnixPath, &sPathPool),
                         TEST_UNIX_PATH_EXT,
                         128) == 0);

    ACT_CHECK(acpCStrCmp(acpPathMakeExe(TEST_UNIX_PATH_BASE, &sPathPool),
                         TEST_UNIX_PATH_BASE,
                         128) == 0);
#endif

    acpPathPoolFinal(&sPathPool);
    
    ACT_TEST_END();

    return 0;
}


