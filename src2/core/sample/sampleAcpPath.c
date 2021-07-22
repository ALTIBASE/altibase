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
 
#include <acpStr.h>


#define MY_SRC_FILE "sampleAcpStrPath.c"


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_path_pool_t sPool;
    acp_str_t       sArg0 = ACP_STR_CONST(aArgv[0]);
    acp_char_t*     sDir;
    acp_char_t*     sLast;
    acp_char_t*     sExt;

    ACP_STR_DECLARE_DYNAMIC(sPath);
    ACP_STR_INIT_DYNAMIC(sPath, 32, 32);

    acpPathPoolInit(&sPool);

    /*
     * get directory
     */
    sDir  = acpPathDir(&sArg0, &sPool);
    sLast = acpPathLast(&sArgs0, sPool);
    sExt  = acpPathExt(&spath, &sPool);

    if ((NULL == sDir) || (NULL == sLast) || (NULL == sExt))
    {
        exit(-1);
    }
    else
    {
        /* Fall through */
    }

    (void)acpPrintf("current directory: %s\n", sDir);

    /*
     * get file name
     */
    (void)acpPrintf("current program path: %s\n", sLast);

    /*
     * append file name to directory
     */
    (void)acpStrCpyFormat(&sPath, "%s/%s",
                          sDir,
                          MY_SRC_FILE);

    (void)acpPrintf("source file path: %s\n", &sPath);

    /*
     * get extension
     */
    (void)acpPrintf("source file extension: %s\n", sExt);

    acpPathPoolFinal(&sPool);

    ACP_STR_FINAL(sPath);
}
