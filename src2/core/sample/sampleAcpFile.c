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
 * $Id: sampleAcpFile.c 9116 2009-12-10 02:03:59Z gurugio $
 ******************************************************************************/


#include <acpFile.h>
#include <acpPrintf.h>
#include <acpInit.h>
#include <acpDir.h>
#include <acpMem.h>


acp_sint32_t main(void)
{
    acp_rc_t     sRC;

    acp_dir_t sDir;
    acp_char_t *sFileName;
    acp_char_t *sPath = ".";
    acp_key_t *sKey;
    acp_sint32_t sFileCount;
    acp_sint32_t i;
    acp_sint32_t j;
    acp_sint32_t sCollision;

    sRC = acpInitialize();
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);


    /* How many files are there? */
    sRC = acpDirOpen(&sDir, sPath);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    for (sFileCount = 0; ACP_RC_IS_SUCCESS(sRC); sFileCount++)
    {
        sRC = acpDirRead(&sDir, &sFileName);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            /* (void)acpPrintf("%s\n", sFileName); */
        }
        else
        {
            /* If this break do not exist,
             * sFileCount is over-increase
             */
            break;
        }
    }
    (void)acpPrintf("%d files in %s dir\n", sFileCount, sPath);


    sRC = acpMemCalloc((void **)&sKey, sFileCount, sizeof(acp_key_t));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    i = 0;
    (void)acpDirRewind(&sDir);

    /* print file-key */
    for (i=0; i < sFileCount; i++)
    {
        sRC = acpDirRead(&sDir, &sFileName);
        
        if (ACP_RC_IS_SUCCESS(sRC))
        {
            (void)acpFileGetKey(sFileName, 25, &sKey[i]);
            (void)acpPrintf("FILE: %s  -> KEY: %x\n", sFileName, 
                            (acp_sint32_t)sKey[i]);
        }
        else
        {
            break;
        }
    }   

    sCollision = 0;
    for (i = 0; i < sFileCount; i++)
    {
        for (j = i+1; j < sFileCount; j++)
        {
            if (sKey[i] == sKey[j])
            {
                sCollision++;
                (void)acpPrintf("KEY Collision:%x\n", (acp_sint32_t)sKey[i]);
            }
            else
            {
                /* do nothing */
            }
        }
    }

    if (sCollision == 0)
    {
        (void)acpPrintf("No Key Collision\n");
    }
    else
    {
        (void)acpPrintf("%d Keys Collisions\n", sCollision);
    }


    (void)acpDirClose(&sDir);
    (void)acpMemFree(sKey);


    (void)acpFinalize();

    return 0;


    ACP_EXCEPTION(PROG_FAIL)


    ACP_EXCEPTION_END;
        (void)acpFinalize();
    return 1;


}
